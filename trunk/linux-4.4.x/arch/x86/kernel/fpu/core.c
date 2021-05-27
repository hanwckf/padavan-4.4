/*
 *  Copyright (C) 1994 Linus Torvalds
 *
 *  Pentium III FXSR, SSE support
 *  General FPU state handling cleanups
 *	Gareth Hughes <gareth@valinux.com>, May 2000
 */
#include <asm/fpu/internal.h>
#include <asm/fpu/regset.h>
#include <asm/fpu/signal.h>
#include <asm/traps.h>

#include <linux/hardirq.h>

/*
 * Represents the initial FPU state. It's mostly (but not completely) zeroes,
 * depending on the FPU hardware format:
 */
union fpregs_state init_fpstate __read_mostly;

/*
 * Track whether the kernel is using the FPU state
 * currently.
 *
 * This flag is used:
 *
 *   - by IRQ context code to potentially use the FPU
 *     if it's unused.
 *
 *   - to debug kernel_fpu_begin()/end() correctness
 */
static DEFINE_PER_CPU(bool, in_kernel_fpu);

/*
 * Track which context is using the FPU on the CPU:
 */
DEFINE_PER_CPU(struct fpu *, fpu_fpregs_owner_ctx);

static void kernel_fpu_disable(void)
{
	WARN_ON_FPU(this_cpu_read(in_kernel_fpu));
	this_cpu_write(in_kernel_fpu, true);
}

static void kernel_fpu_enable(void)
{
	WARN_ON_FPU(!this_cpu_read(in_kernel_fpu));
	this_cpu_write(in_kernel_fpu, false);
}

static bool kernel_fpu_disabled(void)
{
	return this_cpu_read(in_kernel_fpu);
}

static bool interrupted_kernel_fpu_idle(void)
{
	return !kernel_fpu_disabled();
}

/*
 * Were we in user mode (or vm86 mode) when we were
 * interrupted?
 *
 * Doing kernel_fpu_begin/end() is ok if we are running
 * in an interrupt context from user mode - we'll just
 * save the FPU state as required.
 */
static bool interrupted_user_mode(void)
{
	struct pt_regs *regs = get_irq_regs();
	return regs && user_mode(regs);
}

/*
 * Can we use the FPU in kernel mode with the
 * whole "kernel_fpu_begin/end()" sequence?
 *
 * It's always ok in process context (ie "not interrupt")
 * but it is sometimes ok even from an irq.
 */
bool irq_fpu_usable(void)
{
	return !in_interrupt() ||
		interrupted_user_mode() ||
		interrupted_kernel_fpu_idle();
}
EXPORT_SYMBOL(irq_fpu_usable);

void __kernel_fpu_begin(void)
{
	struct fpu *fpu = &current->thread.fpu;

	WARN_ON_FPU(!irq_fpu_usable());

	kernel_fpu_disable();

	if (fpu->fpregs_active) {
		/*
		 * Ignore return value -- we don't care if reg state
		 * is clobbered.
		 */
		copy_fpregs_to_fpstate(fpu);
	} else {
		this_cpu_write(fpu_fpregs_owner_ctx, NULL);
	}
}
EXPORT_SYMBOL(__kernel_fpu_begin);

void __kernel_fpu_end(void)
{
	struct fpu *fpu = &current->thread.fpu;

	if (fpu->fpregs_active)
		copy_kernel_to_fpregs(&fpu->state);

	kernel_fpu_enable();
}
EXPORT_SYMBOL(__kernel_fpu_end);

void kernel_fpu_begin(void)
{
	preempt_disable();
	__kernel_fpu_begin();
}
EXPORT_SYMBOL_GPL(kernel_fpu_begin);

void kernel_fpu_end(void)
{
	__kernel_fpu_end();
	preempt_enable();
}
EXPORT_SYMBOL_GPL(kernel_fpu_end);

/*
 * CR0::TS save/restore functions:
 */
int irq_ts_save(void)
{
	/*
	 * If in process context and not atomic, we can take a spurious DNA fault.
	 * Otherwise, doing clts() in process context requires disabling preemption
	 * or some heavy lifting like kernel_fpu_begin()
	 */
	if (!in_atomic())
		return 0;

	if (read_cr0() & X86_CR0_TS) {
		clts();
		return 1;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(irq_ts_save);

void irq_ts_restore(int TS_state)
{
	if (TS_state)
		stts();
}
EXPORT_SYMBOL_GPL(irq_ts_restore);

/*
 * Save the FPU state (mark it for reload if necessary):
 *
 * This only ever gets called for the current task.
 */
void fpu__save(struct fpu *fpu)
{
	WARN_ON_FPU(fpu != &current->thread.fpu);

	preempt_disable();
	if (fpu->fpregs_active) {
		if (!copy_fpregs_to_fpstate(fpu)) {
			copy_kernel_to_fpregs(&fpu->state);
		}
	}
	preempt_enable();
}
EXPORT_SYMBOL_GPL(fpu__save);

/*
 * Legacy x87 fpstate state init:
 */
static inline void fpstate_init_fstate(struct fregs_state *fp)
{
	fp->cwd = 0xffff037fu;
	fp->swd = 0xffff0000u;
	fp->twd = 0xffffffffu;
	fp->fos = 0xffff0000u;
}

void fpstate_init(union fpregs_state *state)
{
	if (!cpu_has_fpu) {
		fpstate_init_soft(&state->soft);
		return;
	}

	memset(state, 0, xstate_size);

	if (cpu_has_fxsr)
		fpstate_init_fxstate(&state->fxsave);
	else
		fpstate_init_fstate(&state->fsave);
}
EXPORT_SYMBOL_GPL(fpstate_init);

/*
 * Copy the current task's FPU state to a new task's FPU context.
 *
 * In both the 'eager' and the 'lazy' case we save hardware registers
 * directly to the destination buffer.
 */
static void fpu_copy(struct fpu *dst_fpu, struct fpu *src_fpu)
{
	WARN_ON_FPU(src_fpu != &current->thread.fpu);

	/*
	 * Don't let 'init optimized' areas of the XSAVE area
	 * leak into the child task:
	 */
	memset(&dst_fpu->state.xsave, 0, xstate_size);

	/*
	 * Save current FPU registers directly into the child
	 * FPU context, without any memory-to-memory copying.
	 *
	 * If the FPU context got destroyed in the process (FNSAVE
	 * done on old CPUs) then copy it back into the source
	 * context and mark the current task for lazy restore.
	 *
	 * We have to do all this with preemption disabled,
	 * mostly because of the FNSAVE case, because in that
	 * case we must not allow preemption in the window
	 * between the FNSAVE and us marking the context lazy.
	 *
	 * It shouldn't be an issue as even FNSAVE is plenty
	 * fast in terms of critical section length.
	 */
	preempt_disable();
	if (!copy_fpregs_to_fpstate(dst_fpu)) {
		memcpy(&src_fpu->state, &dst_fpu->state, xstate_size);

		copy_kernel_to_fpregs(&src_fpu->state);
	}
	preempt_enable();
}

int fpu__copy(struct fpu *dst_fpu, struct fpu *src_fpu)
{
	dst_fpu->fpregs_active = 0;
	dst_fpu->last_cpu = -1;

	if (src_fpu->fpstate_active && cpu_has_fpu)
		fpu_copy(dst_fpu, src_fpu);

	return 0;
}

/*
 * Activate the current task's in-memory FPU context,
 * if it has not been used before:
 */
void fpu__activate_curr(struct fpu *fpu)
{
	WARN_ON_FPU(fpu != &current->thread.fpu);

	if (!fpu->fpstate_active) {
		fpstate_init(&fpu->state);

		/* Safe to do for the current task: */
		fpu->fpstate_active = 1;
	}
}
EXPORT_SYMBOL_GPL(fpu__activate_curr);

/*
 * This function must be called before we read a task's fpstate.
 *
 * If the task has not used the FPU before then initialize its
 * fpstate.
 *
 * If the task has used the FPU before then save it.
 */
void fpu__activate_fpstate_read(struct fpu *fpu)
{
	/*
	 * If fpregs are active (in the current CPU), then
	 * copy them to the fpstate:
	 */
	if (fpu->fpregs_active) {
		fpu__save(fpu);
	} else {
		if (!fpu->fpstate_active) {
			fpstate_init(&fpu->state);

			/* Safe to do for current and for stopped child tasks: */
			fpu->fpstate_active = 1;
		}
	}
}

/*
 * This function must be called before we write a task's fpstate.
 *
 * If the task has used the FPU before then unlazy it.
 * If the task has not used the FPU before then initialize its fpstate.
 *
 * After this function call, after registers in the fpstate are
 * modified and the child task has woken up, the child task will
 * restore the modified FPU state from the modified context. If we
 * didn't clear its lazy status here then the lazy in-registers
 * state pending on its former CPU could be restored, corrupting
 * the modifications.
 */
void fpu__activate_fpstate_write(struct fpu *fpu)
{
	/*
	 * Only stopped child tasks can be used to modify the FPU
	 * state in the fpstate buffer:
	 */
	WARN_ON_FPU(fpu == &current->thread.fpu);

	if (fpu->fpstate_active) {
		/* Invalidate any lazy state: */
		fpu->last_cpu = -1;
	} else {
		fpstate_init(&fpu->state);

		/* Safe to do for stopped child tasks: */
		fpu->fpstate_active = 1;
	}
}

/*
 * 'fpu__restore()' is called to copy FPU registers from
 * the FPU fpstate to the live hw registers and to activate
 * access to the hardware registers, so that FPU instructions
 * can be used afterwards.
 *
 * Must be called with kernel preemption disabled (for example
 * with local interrupts disabled, as it is in the case of
 * do_device_not_available()).
 */
void fpu__restore(struct fpu *fpu)
{
	fpu__activate_curr(fpu);

	/* Avoid __kernel_fpu_begin() right after fpregs_activate() */
	kernel_fpu_disable();
	fpregs_activate(fpu);
	copy_kernel_to_fpregs(&fpu->state);
	kernel_fpu_enable();
}
EXPORT_SYMBOL_GPL(fpu__restore);

/*
 * Drops current FPU state: deactivates the fpregs and
 * the fpstate. NOTE: it still leaves previous contents
 * in the fpregs in the eager-FPU case.
 *
 * This function can be used in cases where we know that
 * a state-restore is coming: either an explicit one,
 * or a reschedule.
 */
void fpu__drop(struct fpu *fpu)
{
	preempt_disable();

	if (fpu->fpregs_active) {
		/* Ignore delayed exceptions from user space */
		asm volatile("1: fwait\n"
			     "2:\n"
			     _ASM_EXTABLE(1b, 2b));
		fpregs_deactivate(fpu);
	}

	fpu->fpstate_active = 0;

	preempt_enable();
}

/*
 * Clear FPU registers by setting them up from
 * the init fpstate:
 */
static inline void copy_init_fpstate_to_fpregs(void)
{
	if (use_xsave())
		copy_kernel_to_xregs(&init_fpstate.xsave, -1);
	else if (static_cpu_has(X86_FEATURE_FXSR))
		copy_kernel_to_fxregs(&init_fpstate.fxsave);
	else
		copy_kernel_to_fregs(&init_fpstate.fsave);
}

/*
 * Clear the FPU state back to init state.
 *
 * Called by sys_execve(), by the signal handler code and by various
 * error paths.
 */
void fpu__clear(struct fpu *fpu)
{
	WARN_ON_FPU(fpu != &current->thread.fpu); /* Almost certainly an anomaly */

	if (!static_cpu_has(X86_FEATURE_FPU)) {
		/* FPU state will be reallocated lazily at the first use. */
		fpu__drop(fpu);
	} else {
		if (!fpu->fpstate_active) {
			fpu__activate_curr(fpu);
			user_fpu_begin();
		}
		copy_init_fpstate_to_fpregs();
	}
}

/*
 * x87 math exception handling:
 */

static inline unsigned short get_fpu_cwd(struct fpu *fpu)
{
	if (cpu_has_fxsr) {
		return fpu->state.fxsave.cwd;
	} else {
		return (unsigned short)fpu->state.fsave.cwd;
	}
}

static inline unsigned short get_fpu_swd(struct fpu *fpu)
{
	if (cpu_has_fxsr) {
		return fpu->state.fxsave.swd;
	} else {
		return (unsigned short)fpu->state.fsave.swd;
	}
}

static inline unsigned short get_fpu_mxcsr(struct fpu *fpu)
{
	if (cpu_has_xmm) {
		return fpu->state.fxsave.mxcsr;
	} else {
		return MXCSR_DEFAULT;
	}
}

int fpu__exception_code(struct fpu *fpu, int trap_nr)
{
	int err;

	if (trap_nr == X86_TRAP_MF) {
		unsigned short cwd, swd;
		/*
		 * (~cwd & swd) will mask out exceptions that are not set to unmasked
		 * status.  0x3f is the exception bits in these regs, 0x200 is the
		 * C1 reg you need in case of a stack fault, 0x040 is the stack
		 * fault bit.  We should only be taking one exception at a time,
		 * so if this combination doesn't produce any single exception,
		 * then we have a bad program that isn't synchronizing its FPU usage
		 * and it will suffer the consequences since we won't be able to
		 * fully reproduce the context of the exception
		 */
		cwd = get_fpu_cwd(fpu);
		swd = get_fpu_swd(fpu);

		err = swd & ~cwd;
	} else {
		/*
		 * The SIMD FPU exceptions are handled a little differently, as there
		 * is only a single status/control register.  Thus, to determine which
		 * unmasked exception was caught we must mask the exception mask bits
		 * at 0x1f80, and then use these to mask the exception bits at 0x3f.
		 */
		unsigned short mxcsr = get_fpu_mxcsr(fpu);
		err = ~(mxcsr >> 7) & mxcsr;
	}

	if (err & 0x001) {	/* Invalid op */
		/*
		 * swd & 0x240 == 0x040: Stack Underflow
		 * swd & 0x240 == 0x240: Stack Overflow
		 * User must clear the SF bit (0x40) if set
		 */
		return FPE_FLTINV;
	} else if (err & 0x004) { /* Divide by Zero */
		return FPE_FLTDIV;
	} else if (err & 0x008) { /* Overflow */
		return FPE_FLTOVF;
	} else if (err & 0x012) { /* Denormal, Underflow */
		return FPE_FLTUND;
	} else if (err & 0x020) { /* Precision */
		return FPE_FLTRES;
	}

	/*
	 * If we're using IRQ 13, or supposedly even some trap
	 * X86_TRAP_MF implementations, it's possible
	 * we get a spurious trap, which is not an error.
	 */
	return 0;
}
