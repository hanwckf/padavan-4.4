#include <linux/init.h>

#include <linux/mm.h>
#include <linux/spinlock.h>
#include <linux/smp.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/cpu.h>
#include <linux/debugfs.h>

#include <asm/tlbflush.h>
#include <asm/mmu_context.h>
#include <asm/nospec-branch.h>
#include <asm/cache.h>
#include <asm/apic.h>
#include <asm/uv/uv.h>
#include <asm/kaiser.h>

/*
 *	TLB flushing, formerly SMP-only
 *		c/o Linus Torvalds.
 *
 *	These mean you can really definitely utterly forget about
 *	writing to user space from interrupts. (Its not allowed anyway).
 *
 *	Optimizations Manfred Spraul <manfred@colorfullife.com>
 *
 *	More scalable flush, from Andi Kleen
 *
 *	Implement flush IPI by CALL_FUNCTION_VECTOR, Alex Shi
 */

/*
 * Use bit 0 to mangle the TIF_SPEC_IB state into the mm pointer which is
 * stored in cpu_tlb_state.last_user_mm_ibpb.
 */
#define LAST_USER_MM_IBPB	0x1UL

atomic64_t last_mm_ctx_id = ATOMIC64_INIT(1);

struct flush_tlb_info {
	struct mm_struct *flush_mm;
	unsigned long flush_start;
	unsigned long flush_end;
};

static void load_new_mm_cr3(pgd_t *pgdir)
{
	unsigned long new_mm_cr3 = __pa(pgdir);

	if (kaiser_enabled) {
		/*
		 * We reuse the same PCID for different tasks, so we must
		 * flush all the entries for the PCID out when we change tasks.
		 * Flush KERN below, flush USER when returning to userspace in
		 * kaiser's SWITCH_USER_CR3 (_SWITCH_TO_USER_CR3) macro.
		 *
		 * invpcid_flush_single_context(X86_CR3_PCID_ASID_USER) could
		 * do it here, but can only be used if X86_FEATURE_INVPCID is
		 * available - and many machines support pcid without invpcid.
		 *
		 * If X86_CR3_PCID_KERN_FLUSH actually added something, then it
		 * would be needed in the write_cr3() below - if PCIDs enabled.
		 */
		BUILD_BUG_ON(X86_CR3_PCID_KERN_FLUSH);
		kaiser_flush_tlb_on_return_to_user();
	}

	/*
	 * Caution: many callers of this function expect
	 * that load_cr3() is serializing and orders TLB
	 * fills with respect to the mm_cpumask writes.
	 */
	write_cr3(new_mm_cr3);
}

/*
 * We cannot call mmdrop() because we are in interrupt context,
 * instead update mm->cpu_vm_mask.
 */
void leave_mm(int cpu)
{
	struct mm_struct *active_mm = this_cpu_read(cpu_tlbstate.active_mm);
	if (this_cpu_read(cpu_tlbstate.state) == TLBSTATE_OK)
		BUG();
	if (cpumask_test_cpu(cpu, mm_cpumask(active_mm))) {
		cpumask_clear_cpu(cpu, mm_cpumask(active_mm));
		load_new_mm_cr3(swapper_pg_dir);
		/*
		 * This gets called in the idle path where RCU
		 * functions differently.  Tracing normally
		 * uses RCU, so we have to call the tracepoint
		 * specially here.
		 */
		trace_tlb_flush_rcuidle(TLB_FLUSH_ON_TASK_SWITCH, TLB_FLUSH_ALL);
	}
}
EXPORT_SYMBOL_GPL(leave_mm);

void switch_mm(struct mm_struct *prev, struct mm_struct *next,
	       struct task_struct *tsk)
{
	unsigned long flags;

	local_irq_save(flags);
	switch_mm_irqs_off(prev, next, tsk);
	local_irq_restore(flags);
}

static inline unsigned long mm_mangle_tif_spec_ib(struct task_struct *next)
{
	unsigned long next_tif = task_thread_info(next)->flags;
	unsigned long ibpb = (next_tif >> TIF_SPEC_IB) & LAST_USER_MM_IBPB;

	return (unsigned long)next->mm | ibpb;
}

static void cond_ibpb(struct task_struct *next)
{
	if (!next || !next->mm)
		return;

	/*
	 * Both, the conditional and the always IBPB mode use the mm
	 * pointer to avoid the IBPB when switching between tasks of the
	 * same process. Using the mm pointer instead of mm->context.ctx_id
	 * opens a hypothetical hole vs. mm_struct reuse, which is more or
	 * less impossible to control by an attacker. Aside of that it
	 * would only affect the first schedule so the theoretically
	 * exposed data is not really interesting.
	 */
	if (static_branch_likely(&switch_mm_cond_ibpb)) {
		unsigned long prev_mm, next_mm;

		/*
		 * This is a bit more complex than the always mode because
		 * it has to handle two cases:
		 *
		 * 1) Switch from a user space task (potential attacker)
		 *    which has TIF_SPEC_IB set to a user space task
		 *    (potential victim) which has TIF_SPEC_IB not set.
		 *
		 * 2) Switch from a user space task (potential attacker)
		 *    which has TIF_SPEC_IB not set to a user space task
		 *    (potential victim) which has TIF_SPEC_IB set.
		 *
		 * This could be done by unconditionally issuing IBPB when
		 * a task which has TIF_SPEC_IB set is either scheduled in
		 * or out. Though that results in two flushes when:
		 *
		 * - the same user space task is scheduled out and later
		 *   scheduled in again and only a kernel thread ran in
		 *   between.
		 *
		 * - a user space task belonging to the same process is
		 *   scheduled in after a kernel thread ran in between
		 *
		 * - a user space task belonging to the same process is
		 *   scheduled in immediately.
		 *
		 * Optimize this with reasonably small overhead for the
		 * above cases. Mangle the TIF_SPEC_IB bit into the mm
		 * pointer of the incoming task which is stored in
		 * cpu_tlbstate.last_user_mm_ibpb for comparison.
		 */
		next_mm = mm_mangle_tif_spec_ib(next);
		prev_mm = this_cpu_read(cpu_tlbstate.last_user_mm_ibpb);

		/*
		 * Issue IBPB only if the mm's are different and one or
		 * both have the IBPB bit set.
		 */
		if (next_mm != prev_mm &&
		    (next_mm | prev_mm) & LAST_USER_MM_IBPB)
			indirect_branch_prediction_barrier();

		this_cpu_write(cpu_tlbstate.last_user_mm_ibpb, next_mm);
	}

	if (static_branch_unlikely(&switch_mm_always_ibpb)) {
		/*
		 * Only flush when switching to a user space task with a
		 * different context than the user space task which ran
		 * last on this CPU.
		 */
		if (this_cpu_read(cpu_tlbstate.last_user_mm) != next->mm) {
			indirect_branch_prediction_barrier();
			this_cpu_write(cpu_tlbstate.last_user_mm, next->mm);
		}
	}
}

void switch_mm_irqs_off(struct mm_struct *prev, struct mm_struct *next,
			struct task_struct *tsk)
{
	unsigned cpu = smp_processor_id();

	if (likely(prev != next)) {
		/*
		 * Avoid user/user BTB poisoning by flushing the branch
		 * predictor when switching between processes. This stops
		 * one process from doing Spectre-v2 attacks on another.
		 */
		cond_ibpb(tsk);

		this_cpu_write(cpu_tlbstate.state, TLBSTATE_OK);
		this_cpu_write(cpu_tlbstate.active_mm, next);
		cpumask_set_cpu(cpu, mm_cpumask(next));

		/*
		 * Re-load page tables.
		 *
		 * This logic has an ordering constraint:
		 *
		 *  CPU 0: Write to a PTE for 'next'
		 *  CPU 0: load bit 1 in mm_cpumask.  if nonzero, send IPI.
		 *  CPU 1: set bit 1 in next's mm_cpumask
		 *  CPU 1: load from the PTE that CPU 0 writes (implicit)
		 *
		 * We need to prevent an outcome in which CPU 1 observes
		 * the new PTE value and CPU 0 observes bit 1 clear in
		 * mm_cpumask.  (If that occurs, then the IPI will never
		 * be sent, and CPU 0's TLB will contain a stale entry.)
		 *
		 * The bad outcome can occur if either CPU's load is
		 * reordered before that CPU's store, so both CPUs must
		 * execute full barriers to prevent this from happening.
		 *
		 * Thus, switch_mm needs a full barrier between the
		 * store to mm_cpumask and any operation that could load
		 * from next->pgd.  TLB fills are special and can happen
		 * due to instruction fetches or for no reason at all,
		 * and neither LOCK nor MFENCE orders them.
		 * Fortunately, load_cr3() is serializing and gives the
		 * ordering guarantee we need.
		 *
		 */
		load_new_mm_cr3(next->pgd);

		trace_tlb_flush(TLB_FLUSH_ON_TASK_SWITCH, TLB_FLUSH_ALL);

		/* Stop flush ipis for the previous mm */
		cpumask_clear_cpu(cpu, mm_cpumask(prev));

		/* Load per-mm CR4 state */
		load_mm_cr4(next);

#ifdef CONFIG_MODIFY_LDT_SYSCALL
		/*
		 * Load the LDT, if the LDT is different.
		 *
		 * It's possible that prev->context.ldt doesn't match
		 * the LDT register.  This can happen if leave_mm(prev)
		 * was called and then modify_ldt changed
		 * prev->context.ldt but suppressed an IPI to this CPU.
		 * In this case, prev->context.ldt != NULL, because we
		 * never set context.ldt to NULL while the mm still
		 * exists.  That means that next->context.ldt !=
		 * prev->context.ldt, because mms never share an LDT.
		 */
		if (unlikely(prev->context.ldt != next->context.ldt))
			load_mm_ldt(next);
#endif
	} else {
		this_cpu_write(cpu_tlbstate.state, TLBSTATE_OK);
		BUG_ON(this_cpu_read(cpu_tlbstate.active_mm) != next);

		if (!cpumask_test_cpu(cpu, mm_cpumask(next))) {
			/*
			 * On established mms, the mm_cpumask is only changed
			 * from irq context, from ptep_clear_flush() while in
			 * lazy tlb mode, and here. Irqs are blocked during
			 * schedule, protecting us from simultaneous changes.
			 */
			cpumask_set_cpu(cpu, mm_cpumask(next));

			/*
			 * We were in lazy tlb mode and leave_mm disabled
			 * tlb flush IPI delivery. We must reload CR3
			 * to make sure to use no freed page tables.
			 *
			 * As above, load_cr3() is serializing and orders TLB
			 * fills with respect to the mm_cpumask write.
			 */
			load_new_mm_cr3(next->pgd);
			trace_tlb_flush(TLB_FLUSH_ON_TASK_SWITCH, TLB_FLUSH_ALL);
			load_mm_cr4(next);
			load_mm_ldt(next);
		}
	}
}

/*
 * The flush IPI assumes that a thread switch happens in this order:
 * [cpu0: the cpu that switches]
 * 1) switch_mm() either 1a) or 1b)
 * 1a) thread switch to a different mm
 * 1a1) set cpu_tlbstate to TLBSTATE_OK
 *	Now the tlb flush NMI handler flush_tlb_func won't call leave_mm
 *	if cpu0 was in lazy tlb mode.
 * 1a2) update cpu active_mm
 *	Now cpu0 accepts tlb flushes for the new mm.
 * 1a3) cpu_set(cpu, new_mm->cpu_vm_mask);
 *	Now the other cpus will send tlb flush ipis.
 * 1a4) change cr3.
 * 1a5) cpu_clear(cpu, old_mm->cpu_vm_mask);
 *	Stop ipi delivery for the old mm. This is not synchronized with
 *	the other cpus, but flush_tlb_func ignore flush ipis for the wrong
 *	mm, and in the worst case we perform a superfluous tlb flush.
 * 1b) thread switch without mm change
 *	cpu active_mm is correct, cpu0 already handles flush ipis.
 * 1b1) set cpu_tlbstate to TLBSTATE_OK
 * 1b2) test_and_set the cpu bit in cpu_vm_mask.
 *	Atomically set the bit [other cpus will start sending flush ipis],
 *	and test the bit.
 * 1b3) if the bit was 0: leave_mm was called, flush the tlb.
 * 2) switch %%esp, ie current
 *
 * The interrupt must handle 2 special cases:
 * - cr3 is changed before %%esp, ie. it cannot use current->{active_,}mm.
 * - the cpu performs speculative tlb reads, i.e. even if the cpu only
 *   runs in kernel space, the cpu could load tlb entries for user space
 *   pages.
 *
 * The good news is that cpu_tlbstate is local to each cpu, no
 * write/read ordering problems.
 */

/*
 * TLB flush funcation:
 * 1) Flush the tlb entries if the cpu uses the mm that's being flushed.
 * 2) Leave the mm if we are in the lazy tlb mode.
 */
static void flush_tlb_func(void *info)
{
	struct flush_tlb_info *f = info;

	inc_irq_stat(irq_tlb_count);

	if (f->flush_mm && f->flush_mm != this_cpu_read(cpu_tlbstate.active_mm))
		return;

	count_vm_tlb_event(NR_TLB_REMOTE_FLUSH_RECEIVED);
	if (this_cpu_read(cpu_tlbstate.state) == TLBSTATE_OK) {
		if (f->flush_end == TLB_FLUSH_ALL) {
			local_flush_tlb();
			trace_tlb_flush(TLB_REMOTE_SHOOTDOWN, TLB_FLUSH_ALL);
		} else {
			unsigned long addr;
			unsigned long nr_pages =
				(f->flush_end - f->flush_start) / PAGE_SIZE;
			addr = f->flush_start;
			while (addr < f->flush_end) {
				__flush_tlb_single(addr);
				addr += PAGE_SIZE;
			}
			trace_tlb_flush(TLB_REMOTE_SHOOTDOWN, nr_pages);
		}
	} else
		leave_mm(smp_processor_id());

}

void native_flush_tlb_others(const struct cpumask *cpumask,
				 struct mm_struct *mm, unsigned long start,
				 unsigned long end)
{
	struct flush_tlb_info info;

	info.flush_mm = mm;
	info.flush_start = start;
	info.flush_end = end;

	count_vm_tlb_event(NR_TLB_REMOTE_FLUSH);
	if (end == TLB_FLUSH_ALL)
		trace_tlb_flush(TLB_REMOTE_SEND_IPI, TLB_FLUSH_ALL);
	else
		trace_tlb_flush(TLB_REMOTE_SEND_IPI,
				(end - start) >> PAGE_SHIFT);

	if (is_uv_system()) {
		unsigned int cpu;

		cpu = smp_processor_id();
		cpumask = uv_flush_tlb_others(cpumask, mm, start, end, cpu);
		if (cpumask)
			smp_call_function_many(cpumask, flush_tlb_func,
								&info, 1);
		return;
	}
	smp_call_function_many(cpumask, flush_tlb_func, &info, 1);
}

/*
 * See Documentation/x86/tlb.txt for details.  We choose 33
 * because it is large enough to cover the vast majority (at
 * least 95%) of allocations, and is small enough that we are
 * confident it will not cause too much overhead.  Each single
 * flush is about 100 ns, so this caps the maximum overhead at
 * _about_ 3,000 ns.
 *
 * This is in units of pages.
 */
static unsigned long tlb_single_page_flush_ceiling __read_mostly = 33;

void flush_tlb_mm_range(struct mm_struct *mm, unsigned long start,
				unsigned long end, unsigned long vmflag)
{
	unsigned long addr;
	/* do a global flush by default */
	unsigned long base_pages_to_flush = TLB_FLUSH_ALL;

	preempt_disable();

	if ((end != TLB_FLUSH_ALL) && !(vmflag & VM_HUGETLB))
		base_pages_to_flush = (end - start) >> PAGE_SHIFT;
	if (base_pages_to_flush > tlb_single_page_flush_ceiling)
		base_pages_to_flush = TLB_FLUSH_ALL;

	if (current->active_mm != mm) {
		/* Synchronize with switch_mm. */
		smp_mb();

		goto out;
	}

	if (!current->mm) {
		leave_mm(smp_processor_id());

		/* Synchronize with switch_mm. */
		smp_mb();

		goto out;
	}

	/*
	 * Both branches below are implicit full barriers (MOV to CR or
	 * INVLPG) that synchronize with switch_mm.
	 */
	if (base_pages_to_flush == TLB_FLUSH_ALL) {
		count_vm_tlb_event(NR_TLB_LOCAL_FLUSH_ALL);
		local_flush_tlb();
	} else {
		/* flush range by one by one 'invlpg' */
		for (addr = start; addr < end;	addr += PAGE_SIZE) {
			count_vm_tlb_event(NR_TLB_LOCAL_FLUSH_ONE);
			__flush_tlb_single(addr);
		}
	}
	trace_tlb_flush(TLB_LOCAL_MM_SHOOTDOWN, base_pages_to_flush);
out:
	if (base_pages_to_flush == TLB_FLUSH_ALL) {
		start = 0UL;
		end = TLB_FLUSH_ALL;
	}
	if (cpumask_any_but(mm_cpumask(mm), smp_processor_id()) < nr_cpu_ids)
		flush_tlb_others(mm_cpumask(mm), mm, start, end);
	preempt_enable();
}

static void do_flush_tlb_all(void *info)
{
	count_vm_tlb_event(NR_TLB_REMOTE_FLUSH_RECEIVED);
	__flush_tlb_all();
	if (this_cpu_read(cpu_tlbstate.state) == TLBSTATE_LAZY)
		leave_mm(smp_processor_id());
}

void flush_tlb_all(void)
{
	count_vm_tlb_event(NR_TLB_REMOTE_FLUSH);
	on_each_cpu(do_flush_tlb_all, NULL, 1);
}

static void do_kernel_range_flush(void *info)
{
	struct flush_tlb_info *f = info;
	unsigned long addr;

	/* flush range by one by one 'invlpg' */
	for (addr = f->flush_start; addr < f->flush_end; addr += PAGE_SIZE)
		__flush_tlb_single(addr);
}

void flush_tlb_kernel_range(unsigned long start, unsigned long end)
{

	/* Balance as user space task's flush, a bit conservative */
	if (end == TLB_FLUSH_ALL ||
	    (end - start) > tlb_single_page_flush_ceiling * PAGE_SIZE) {
		on_each_cpu(do_flush_tlb_all, NULL, 1);
	} else {
		struct flush_tlb_info info;
		info.flush_start = start;
		info.flush_end = end;
		on_each_cpu(do_kernel_range_flush, &info, 1);
	}
}

static ssize_t tlbflush_read_file(struct file *file, char __user *user_buf,
			     size_t count, loff_t *ppos)
{
	char buf[32];
	unsigned int len;

	len = sprintf(buf, "%ld\n", tlb_single_page_flush_ceiling);
	return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}

static ssize_t tlbflush_write_file(struct file *file,
		 const char __user *user_buf, size_t count, loff_t *ppos)
{
	char buf[32];
	ssize_t len;
	int ceiling;

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;

	buf[len] = '\0';
	if (kstrtoint(buf, 0, &ceiling))
		return -EINVAL;

	if (ceiling < 0)
		return -EINVAL;

	tlb_single_page_flush_ceiling = ceiling;
	return count;
}

static const struct file_operations fops_tlbflush = {
	.read = tlbflush_read_file,
	.write = tlbflush_write_file,
	.llseek = default_llseek,
};

static int __init create_tlb_single_page_flush_ceiling(void)
{
	debugfs_create_file("tlb_single_page_flush_ceiling", S_IRUSR | S_IWUSR,
			    arch_debugfs_dir, NULL, &fops_tlbflush);
	return 0;
}
late_initcall(create_tlb_single_page_flush_ceiling);
