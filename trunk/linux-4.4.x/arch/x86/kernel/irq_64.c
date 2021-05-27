/*
 *	Copyright (C) 1992, 1998 Linus Torvalds, Ingo Molnar
 *
 * This file contains the lowest level x86_64-specific interrupt
 * entry and irq statistics code. All the remaining irq logic is
 * done by the generic kernel/irq/ code and in the
 * x86_64-specific irq controller code. (e.g. i8259.c and
 * io_apic.c.)
 */

#include <linux/kernel_stat.h>
#include <linux/interrupt.h>
#include <linux/seq_file.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/ftrace.h>
#include <linux/uaccess.h>
#include <linux/smp.h>
#include <asm/io_apic.h>
#include <asm/idle.h>
#include <asm/apic.h>

int sysctl_panic_on_stackoverflow;

/*
 * Probabilistic stack overflow check:
 *
 * Regular device interrupts can enter on the following stacks:
 *
 * - User stack
 *
 * - Kernel task stack
 *
 * - Interrupt stack if a device driver reenables interrupts
 *   which should only happen in really old drivers.
 *
 * - Debug IST stack
 *
 * All other contexts are invalid.
 */
static inline void stack_overflow_check(struct pt_regs *regs)
{
#ifdef CONFIG_DEBUG_STACKOVERFLOW
#define STACK_TOP_MARGIN	128
	struct orig_ist *oist;
	u64 irq_stack_top, irq_stack_bottom;
	u64 estack_top, estack_bottom;
	u64 curbase = (u64)task_stack_page(current);

	if (user_mode(regs))
		return;

	if (regs->sp >= curbase + sizeof(struct thread_info) +
				  sizeof(struct pt_regs) + STACK_TOP_MARGIN &&
	    regs->sp <= curbase + THREAD_SIZE)
		return;

	irq_stack_top = (u64)this_cpu_ptr(irq_stack_union.irq_stack) +
			STACK_TOP_MARGIN;
	irq_stack_bottom = (u64)__this_cpu_read(irq_stack_ptr);
	if (regs->sp >= irq_stack_top && regs->sp <= irq_stack_bottom)
		return;

	oist = this_cpu_ptr(&orig_ist);
	estack_bottom = (u64)oist->ist[DEBUG_STACK];
	estack_top = estack_bottom - DEBUG_STKSZ + STACK_TOP_MARGIN;
	if (regs->sp >= estack_top && regs->sp <= estack_bottom)
		return;

	WARN_ONCE(1, "do_IRQ(): %s has overflown the kernel stack (cur:%Lx,sp:%lx,irq stk top-bottom:%Lx-%Lx,exception stk top-bottom:%Lx-%Lx)\n",
		current->comm, curbase, regs->sp,
		irq_stack_top, irq_stack_bottom,
		estack_top, estack_bottom);

	if (sysctl_panic_on_stackoverflow)
		panic("low stack detected by irq handler - check messages\n");
#endif
}

bool handle_irq(struct irq_desc *desc, struct pt_regs *regs)
{
	stack_overflow_check(regs);

	if (IS_ERR_OR_NULL(desc))
		return false;

	generic_handle_irq_desc(desc);
	return true;
}
