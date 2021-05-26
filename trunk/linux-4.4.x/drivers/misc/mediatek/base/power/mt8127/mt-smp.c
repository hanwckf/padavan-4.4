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
#include <linux/init.h>
#include <linux/smp.h>
#include <linux/spinlock.h>
#include <linux/jiffies.h>
#include <linux/delay.h>
#include <asm/fiq_glue.h>
#include <mt-plat/sync_write.h>
#include <mach/mt_spm_mtcmos.h>
#if defined(CONFIG_TRUSTY)
#include <mach/mt_secure_api.h>
#include <mach/mt_trusty_api.h>
#endif
#if defined(CONFIG_MTK_IN_HOUSE_TEE_SUPPORT)
#include <trustzone/kree/tz_pm.h>
#include <mach/mtk_boot_share_page.h>
#endif

#include "mt-smp.h"
#include "smp.h"
#include "hotplug.h"

#define INFRACFG_AO_BASE	0x10001000
#define MCUCFG_BASE		0x10200000
#define SRAMROM_BASE		0x10202000

#define SLAVE_JUMP_REG		(SRAMROM_BASE+0x34)
#define SLAVE1_MAGIC_REG	(SRAMROM_BASE+0x38)
#define SLAVE2_MAGIC_REG	(SRAMROM_BASE+0x3c)
#define SLAVE3_MAGIC_REG	(SRAMROM_BASE+0x40)

#define SLAVE1_MAGIC_NUM	0x534C4131
#define SLAVE2_MAGIC_NUM	0x4C415332
#define SLAVE3_MAGIC_NUM	0x41534C33

#if defined(CONFIG_MTK_IN_HOUSE_TEE_SUPPORT)
/* Secure world location, using SRAM now. */
#define NS_SLAVE_JUMP_REG	(BOOT_SHARE_BASE+0x3fc)
#define NS_SLAVE_MAGIC_REG	(BOOT_SHARE_BASE+0x3f8)
#define NS_SLAVE_BOOT_ADDR	(BOOT_SHARE_BASE+0x3f4)
#endif


#define SW_ROM_PD		BIT(31)

static DEFINE_SPINLOCK(boot_lock);

static unsigned int is_secondary_cpu_first_boot;
static const unsigned int secure_magic_reg[] = {SLAVE1_MAGIC_REG, SLAVE2_MAGIC_REG, SLAVE3_MAGIC_REG};
static const unsigned int secure_magic_num[] = {SLAVE1_MAGIC_NUM, SLAVE2_MAGIC_NUM, SLAVE3_MAGIC_NUM};
typedef int (*spm_mtcmos_ctrl_func)(int state, int chkWfiBeforePdn);
static const spm_mtcmos_ctrl_func secure_ctrl_func[] = {spm_mtcmos_ctrl_cpu1,
							spm_mtcmos_ctrl_cpu2,
							spm_mtcmos_ctrl_cpu3};

static void __iomem *infracfg_ao_base;
static void __iomem *mcucfg_base;
static void __iomem *sramrom_base;
/*
 * Write pen_release in a way that is guaranteed to be visible to all
 * observers, irrespective of whether they're taking part in coherency
 * or not.  This is necessary for the hotplug code to work reliably.
 */
static void write_pen_release(int val)
{
	pen_release = val;

	smp_wmb();
	__cpuc_flush_dcache_area((void *)&pen_release, sizeof(pen_release));
	outer_clean_range(__pa(&pen_release), __pa(&pen_release + 1));
}

void mt_smp_secondary_init(unsigned int cpu)
{
	HOTPLUG_INFO("platform_secondary_init, cpu: %d\n", cpu);

	/*
	 * let the primary processor know we're out of the
	 * pen, then head off into the C entry point
	 */
	write_pen_release(-1);

	fiq_glue_resume();

	/*
	 * Synchronise with the boot thread.
	 */
	spin_lock(&boot_lock);
	spin_unlock(&boot_lock);
}

static void mt_wakeup_cpu(int cpu)
{
#ifdef CONFIG_HOTPLUG_WITH_POWER_CTRL
	if (is_secondary_cpu_first_boot) {
		mt_reg_sync_writel(secure_magic_num[cpu-1], sramrom_base + 0x34 + 0x4 * cpu);
#if defined(CONFIG_MTK_IN_HOUSE_TEE_SUPPORT)
		*((unsigned int *)NS_SLAVE_MAGIC_REG) = secure_magic_num[cpu-1];
#endif
		arch_send_wakeup_ipi_mask(cpumask_of(cpu));
		HOTPLUG_INFO("mt_wakeup_cpu: first boot!(%d)\n", cpu);
		is_secondary_cpu_first_boot--;
	} else {
		mt_reg_sync_writel(virt_to_phys(mt_secondary_startup), infracfg_ao_base + 0x800);
#if defined(CONFIG_MTK_IN_HOUSE_TEE_SUPPORT)
		*((unsigned int *)NS_SLAVE_BOOT_ADDR) = virt_to_phys(mt_secondary_startup);
#endif
		(*secure_ctrl_func[cpu-1])(STA_POWER_ON, 1);
	}
#endif
}

int mt_smp_boot_secondary(unsigned int cpu, struct task_struct *idle)
{
	unsigned long timeout;

	atomic_inc(&hotplug_cpu_count);

	/*
	 * Set synchronisation state between this boot processor
	 * and the secondary one
	 */
	spin_lock(&boot_lock);

	HOTPLUG_INFO("mt_smp_boot_secondary, cpu: %d\n", cpu);
	/*
	 * The secondary processor is waiting to be released from
	 * the holding pen - release it, then wait for it to flag
	 * that it has been released by resetting pen_release.
	 *
	 * Note that "pen_release" is the hardware CPU ID, whereas
	 * "cpu" is Linux's internal ID.
	 */
	/*
	 * This is really belt and braces; we hold unintended secondary
	 * CPUs in the holding pen until we're ready for them.  However,
	 * since we haven't sent them a soft interrupt, they shouldn't
	 * be there.
	 */
	write_pen_release(cpu);

	switch (cpu) {
	case 1:
	case 2:
	case 3:
#if defined(CONFIG_TRUSTONIC_TEE_SUPPORT)
		mt_secure_call(MC_FC_SET_RESET_VECTOR, virt_to_phys(mt_secondary_startup), cpu, 0);
		(*secure_ctrl_func[cpu-1])(STA_POWER_ON, 1);
#elif defined(CONFIG_TRUSTY)
		mt_trusty_call(SMC_FC_CPU_ON, virt_to_phys(mt_secondary_startup), cpu, 0);
		(*secure_ctrl_func[cpu-1])(STA_POWER_ON, 1);
#else
		mt_wakeup_cpu(cpu);
#endif
			break;
		default:
			break;
	}

	/*
	 * Now the secondary core is starting up let it run its
	 * calibrations, then wait for it to finish
	 */
	spin_unlock(&boot_lock);

	timeout = jiffies + (1 * HZ);
	while (time_before(jiffies, timeout)) {
		/* */
		smp_rmb();
		if (pen_release == -1)
			break;

		udelay(10);
	}

	if (pen_release != -1) {
		/* Dump debug info */
		writel_relaxed(cpu + 8, mcucfg_base + 0x80);
		pr_emerg("CPU%u, debug event: 0x%08x, debug monitor: 0x%08x\n",
			  cpu, readl(mcucfg_base + 0x80), readl(mcucfg_base + 0x84));
		on_each_cpu((smp_call_func_t) dump_stack, NULL, 0);

		atomic_dec(&hotplug_cpu_count);
		return -EINVAL;
	}

	return 0;
}

void __init mt_smp_init_cpus(void)
{
	pr_emerg("@@@### num_possible_cpus(): %u ###@@@\n", num_possible_cpus());
	pr_emerg("@@@### num_present_cpus(): %u ###@@@\n", num_present_cpus());

	is_secondary_cpu_first_boot = num_possible_cpus() - 1;
}

void __init mt_smp_prepare_cpus(unsigned int max_cpus)
{
	unsigned int ncores;

	infracfg_ao_base = ioremap(INFRACFG_AO_BASE, 0x1000);
	mcucfg_base = ioremap(MCUCFG_BASE, 0x1000);
	sramrom_base = ioremap(SRAMROM_BASE, 0x1000);

	/* enable bootrom power down mode */
	writel_relaxed(readl(infracfg_ao_base + 0x804) | SW_ROM_PD,
		       infracfg_ao_base + 0x804);

	/*
	 * write the address of slave startup into boot address register
	 * for bootrom power down mode
	 */
	writel_relaxed(virt_to_phys(mt_secondary_startup),
		       infracfg_ao_base + 0x800);

#if !defined(CONFIG_TRUSTONIC_TEE_SUPPORT)
	/* write the address of slave startup into the system-wide flags register */
	writel_relaxed(virt_to_phys(mt_secondary_startup), sramrom_base + 0x34);
#endif
#if defined(CONFIG_MTK_IN_HOUSE_TEE_SUPPORT)
	*((unsigned int *)NS_SLAVE_JUMP_REG) = virt_to_phys(mt_secondary_startup);
#endif

	/* initial spm_mtcmos memory map */
	spm_mtcmos_cpu_init();

	/* power off extra cores */
	ncores = num_possible_cpus();
	if (ncores == 3) {
		spm_mtcmos_ctrl_cpu3(STA_POWER_DOWN, 0);
	} else if (ncores == 2) {
		spm_mtcmos_ctrl_cpu3(STA_POWER_DOWN, 0);
		spm_mtcmos_ctrl_cpu2(STA_POWER_DOWN, 0);
	}
}

static struct smp_operations mt8127_smp_ops __initdata = {
	.smp_init_cpus = mt_smp_init_cpus,
	.smp_prepare_cpus = mt_smp_prepare_cpus,
	.smp_secondary_init = mt_smp_secondary_init,
	.smp_boot_secondary = mt_smp_boot_secondary,
#ifdef CONFIG_HOTPLUG_CPU
	.cpu_kill = mt_cpu_kill,
	.cpu_die = mt_cpu_die,
	.cpu_disable = mt_cpu_disable,
#endif
};
CPU_METHOD_OF_DECLARE(mt8127_smp, "mediatek,mt8127-smp", &mt8127_smp_ops);
