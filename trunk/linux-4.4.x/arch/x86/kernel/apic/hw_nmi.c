/*
 *  HW NMI watchdog support
 *
 *  started by Don Zickus, Copyright (C) 2010 Red Hat, Inc.
 *
 *  Arch specific calls to support NMI watchdog
 *
 *  Bits copied from original nmi.c file
 *
 */
#include <asm/apic.h>
#include <asm/nmi.h>

#include <linux/cpumask.h>
#include <linux/kdebug.h>
#include <linux/notifier.h>
#include <linux/kprobes.h>
#include <linux/nmi.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/seq_buf.h>

#ifdef CONFIG_HARDLOCKUP_DETECTOR
u64 hw_nmi_get_sample_period(int watchdog_thresh)
{
	return (u64)(cpu_khz) * 1000 * watchdog_thresh;
}
#endif

#ifdef arch_trigger_all_cpu_backtrace
static void nmi_raise_cpu_backtrace(cpumask_t *mask)
{
	apic->send_IPI_mask(mask, NMI_VECTOR);
}

void arch_trigger_all_cpu_backtrace(bool include_self)
{
	nmi_trigger_all_cpu_backtrace(include_self, nmi_raise_cpu_backtrace);
}

static int
arch_trigger_all_cpu_backtrace_handler(unsigned int cmd, struct pt_regs *regs)
{
	if (nmi_cpu_backtrace(regs))
		return NMI_HANDLED;

	return NMI_DONE;
}
NOKPROBE_SYMBOL(arch_trigger_all_cpu_backtrace_handler);

static int __init register_trigger_all_cpu_backtrace(void)
{
	register_nmi_handler(NMI_LOCAL, arch_trigger_all_cpu_backtrace_handler,
				0, "arch_bt");
	return 0;
}
early_initcall(register_trigger_all_cpu_backtrace);
#endif
