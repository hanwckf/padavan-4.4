/*
 * Copyright (c) 2015 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/errno.h>
#include <asm/cacheflush.h>
#include <mt-plat/sync_write.h>
#include <mach/mt_spm_mtcmos.h>
#if defined(CONFIG_TRUSTY)
#include <mach/mt_trusty_api.h>
#endif
#ifdef CONFIG_MTK_IN_HOUSE_TEE_SUPPORT
#include <trustzone/kree/tz_pm.h>
#endif

#include "mt-smp.h"
#include "smp.h"
#include "hotplug.h"

atomic_t hotplug_cpu_count = ATOMIC_INIT(1);

#ifndef CONFIG_MTK_IN_HOUSE_TEE_SUPPORT
static inline void cpu_enter_lowpower(unsigned int cpu)
{
#if defined(CONFIG_TRUSTY)
	mt_trusty_call(SMC_FC_CPU_OFF, 0, cpu, 0);
#endif

	__disable_dcache__inner_flush_dcache_L1__inner_clean_dcache_L2();

	/* Execute a CLREX instruction */
	__asm__ __volatile__("clrex");

	/*
	 * Switch the processor from SMP mode to AMP mode by
	 * clearing the ACTLR SMP bit
	 */
	__switch_to_amp();
}

static inline void cpu_leave_lowpower(unsigned int cpu)
{
	/* Set the ACTLR.SMP bit to 1 for SMP mode */
	__switch_to_smp();

	/* Enable dcache */
	__enable_dcache();
}

static inline void platform_do_lowpower(unsigned int cpu, int *spurious)
{
	/* Just enter wfi for now. TODO: Properly shut off the cpu. */
	for (;;) {

		/*
		 * Execute an ISB instruction to ensure that all of the CP15
		 * register changes from the previous steps have been
		 * committed
		 */
		isb();

		/*
		 * Execute a DSB instruction to ensure that all cache, TLB and
		 * branch predictor maintenance operations issued by any
		 * processor in the multiprocessor device before the SMP bit
		 * was cleared have completed
		 */
		dsb();

		/*
		 * here's the WFI
		 */
		__asm__ __volatile__("wfi");

		if (pen_release == cpu) {
			/*
			 * OK, proper wakeup, we're done
			 */
			break;
		}

		/*
		 * Getting here, means that we have come out of WFI without
		 * having been woken up - this shouldn't happen
		 *
		 * Just note it happening - when we're woken, we can report
		 * its occurrence.
		 */
		(*spurious)++;
	}
}
#else /* defined(CONFIG_MTK_IN_HOUSE_TEE_SUPPORT) */
static inline void cpu_enter_lowpower(unsigned int cpu) { return; }
static inline void cpu_leave_lowpower(unsigned int cpu) { return; }
static inline void platform_do_lowpower(unsigned int cpu, int *spurious)
{
	/* TEE PM to do low power */
	kree_pm_cpu_lowpower(&pen_release, cpu);
	/*
	 * Getting here, means that we have come out of WFI without
	 * having been woken up - this shouldn't happen
	 *
	 * Just note it happening - when we're woken, we can report
	 * its occurrence.
	 */
	(*spurious)++;
}
#endif

/*
 * mt_cpu_kill:
 * @cpu:
 * Return TBD.
 */
int mt_cpu_kill(unsigned int cpu)
{
	HOTPLUG_INFO("mt_cpu_kill, cpu: %d\n", cpu);

#ifdef CONFIG_HOTPLUG_WITH_POWER_CTRL
	switch (cpu) {
	case 1:
		spm_mtcmos_ctrl_cpu1(STA_POWER_DOWN, 1);
		break;
	case 2:
		spm_mtcmos_ctrl_cpu2(STA_POWER_DOWN, 1);
		break;
	case 3:
		spm_mtcmos_ctrl_cpu3(STA_POWER_DOWN, 1);
		break;
	default:
		break;
	}
#endif
	atomic_dec(&hotplug_cpu_count);

	return 1;
}

/*
 * mt_cpu_die: shutdown a CPU
 * @cpu:
 */
void mt_cpu_die(unsigned int cpu)
{
	int spurious = 0;

	HOTPLUG_INFO("mt_cpu_die, cpu: %d\n", cpu);
	/*
	 * we're ready for shutdown now, so do it
	 */
	cpu_enter_lowpower(cpu);
	platform_do_lowpower(cpu, &spurious);

	/*
	 * bring this CPU back into the world of cache
	 * coherency, and then restore interrupts
	 */
	cpu_leave_lowpower(cpu);

	if (spurious)
		HOTPLUG_INFO("spurious wakeup call, cpu: %d, spurious: %d\n",
			      cpu, spurious);
}

/*
 * mt_cpu_disable:
 * @cpu:
 * Return error code.
 */
int mt_cpu_disable(unsigned int cpu)
{
	/*
	 * we don't allow CPU 0 to be shutdown (it is still too special
	 * e.g. clock tick interrupts)
	 */
	HOTPLUG_INFO("mt_cpu_disable, cpu: %d\n", cpu);

	return cpu == 0 ? -EPERM : 0;
}
