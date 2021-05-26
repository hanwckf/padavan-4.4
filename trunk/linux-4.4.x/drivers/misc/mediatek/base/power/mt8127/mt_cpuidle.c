/*
* Copyright (c) 2015 MediaTek Inc.
* Author: Cheng-En Chung <cheng-en.chung@mediatek.com>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*/

#include <asm/cacheflush.h>
#include <asm/irqflags.h>
#include <asm/neon.h>
#include <asm/suspend.h>

#include <linux/of_address.h>
#include <linux/of.h>
#include <linux/kernel.h>
#include <linux/atomic.h>
/* #include <mt-plat/mt_dbg.h> */
/* #include <mt-plat/mt_io.h> */
#include <mt-plat/sync_write.h>

#include "mt_cpuidle.h"
#include "mt_spm.h"
#include "smp.h"

/* #include <mach/irqs.h> */

#include <mach/mt_spm_mtcmos.h>

#if defined(CONFIG_MTK_RAM_CONSOLE) || defined(CONFIG_TRUSTONIC_TEE_SUPPORT)
#include <mach/mt_secure_api.h>
#endif
#if defined(CONFIG_TRUSTY)
#include <mach/mt_trusty_api.h>
#endif
#if defined(CONFIG_MTK_IN_HOUSE_TEE_SUPPORT)
#include <mach/mtk_boot_share_page.h>
#include <trustzone/kree/tz_pm.h>
#endif

#define TAG "[Power-Dormant] "

#define dormant_err(fmt, args...)	pr_emerg(TAG fmt, ##args)
#define dormant_warn(fmt, args...)	pr_warn(TAG fmt, ##args)
#define dormant_dbg(fmt, args...)	pr_debug(TAG fmt, ##args)


#ifdef CONFIG_MTK_RAM_CONSOLE
unsigned long *sleep_aee_rec_cpu_dormant_pa;
unsigned long *sleep_aee_rec_cpu_dormant_va;
#endif

static unsigned long mcucfg_base;
static unsigned long infracfg_ao_base;
static unsigned long gic_id_base;
static unsigned long gic_ci_base;

/* static unsigned int kp_irq_bit; */
/* static unsigned int conn_wdt_irq_bit; */
/* static unsigned int lowbattery_irq_bit; */
/* static unsigned int md1_wdt_bit; */

#define MAX_CORES 4
#define MAX_CLUSTER 1

#define MCUCFG_NODE		"mediatek,mt8127-mcucfg"
#define INFRACFG_AO_NODE	"mediatek,mt8127-infracfg"
#define GIC_NODE		"arm,cortex-a7-gic"
#if 0
#define KP_NODE			"mediatek,mt6580-keypad"
#define CONSYS_NODE		"mediatek,mt6580-consys"
#define AUXADC_NODE		"mediatek,mt6735-auxadc"
#define MDCLDMA_NODE		"mediatek,ap_ccif0"
#endif
#define DMT_MCUCFG_BASE		mcucfg_base
#define DMT_INFRACFG_AO_BASE	infracfg_ao_base
#define DMT_GIC_DIST_BASE	gic_id_base
#define DMT_GIC_CPU_BASE	gic_ci_base
#if 0
#define DMT_KP_IRQ_BIT		kp_irq_bit
#define DMT_CONN_WDT_IRQ_BIT	conn_wdt_irq_bit
#define DMT_LOWBATTERY_IRQ_BIT	lowbattery_irq_bit
#define DMT_MD1_WDT_BIT		md1_wdt_bit
#endif
#define MP0_CA7L_CACHE_CONFIG	(DMT_MCUCFG_BASE + 0)
#define MP1_CA7L_CACHE_CONFIG	(DMT_MCUCFG_BASE + 0x200)
#define L2RSTDISABLE		(1 << 4)
#define SW_ROM_PD		BIT(31)

#define DMT_BOOTROM_PWR_CTRL	((void *) (DMT_INFRACFG_AO_BASE + 0x804))
#define DMT_BOOTROM_BOOT_ADDR	((void *) (DMT_INFRACFG_AO_BASE + 0x800))
#define NS_SLAVE_BOOT_ADDR		(BOOT_SHARE_BASE + 1012)

#define reg_read(addr)		__raw_readl(IOMEM(addr))
#define reg_write(addr, val)	mt_reg_sync_writel(val, addr)
#define _and(a, b)		((a) & (b))
#define _or(a, b)		((a) | (b))
#define _aor(a, b, c)		_or(_and(a, b), (c))

struct core_context {
	volatile u64 timestamp[5];
	unsigned long timer_data[8];
};

struct cluster_context {
	struct core_context core[MAX_CORES] ____cacheline_aligned;
	unsigned long dbg_data[40];
	int l2rstdisable;
	int l2rstdisable_rfcnt;
};

struct system_context {
	struct cluster_context cluster[MAX_CLUSTER];
	struct _data_poc {
		void (*cpu_resume_phys)(void);
		unsigned long l2ctlr;
	} poc ____cacheline_aligned;
};

struct system_context dormant_data[1];
static int mt_dormant_initialized;

#define SPM_CORE_ID() core_idx()
#define SPM_IS_CPU_IRQ_OCCUR(core_id)						\
	({									\
		(!!(spm_read(SPM_SLEEP_WAKEUP_MISC) & ((0x101<<(core_id)))));	\
	})

#ifdef CONFIG_MTK_RAM_CONSOLE
/* #define DORMANT_LOG(cid, pattern) (sleep_aee_rec_cpu_dormant_va[cid] = pattern) */
#define DORMANT_LOG(cid, pattern)
#else
#define DORMANT_LOG(cid, pattern)
#endif

#define core_idx()							\
	({								\
		((read_cluster_id() >> 6) | read_cpu_id());		\
	})

inline void read_id(int *cpu_id, int *cluster_id)
{
	*cpu_id = read_cpu_id();
	*cluster_id = read_cluster_id();
}

#define system_cluster(system, clusterid)	(&((struct system_context *)system)->cluster[clusterid])
#define cluster_core(cluster, cpuid)		(&((struct cluster_context *)cluster)->core[cpuid])

void *_get_data(int core_or_cluster)
{
	int cpuid, clusterid;
	struct cluster_context *cluster;
	struct core_context *core;

	read_id(&cpuid, &clusterid);

	cluster = system_cluster(dormant_data, clusterid);
	if (core_or_cluster == 1)
		return (void *)cluster;

	core = cluster_core(cluster, cpuid);

	return (void *)core;
}

#define GET_CORE_DATA()		((struct core_context *)_get_data(0))
#define GET_CLUSTER_DATA()	((struct cluster_context *)_get_data(1))

void stop_generic_timer(void)
{
	write_cntpctl(read_cntpctl() & ~1);
}

void start_generic_timer(void)
{
	write_cntpctl(read_cntpctl() | 1);
}

struct set_and_clear_regs {
	volatile unsigned int set[32], clear[32];
};

unsigned int __weak *mt_save_dbg_regs(unsigned int *p, unsigned int cpuid)
{
	return p;
}

void __weak mt_restore_dbg_regs(unsigned int *p, unsigned int cpuid)
{
}

void __weak mt_copy_dbg_regs(int to, int from)
{
}

void __weak mt_save_banked_registers(unsigned int *container)
{
}

void __weak mt_restore_banked_registers(unsigned int *container)
{
}

struct interrupt_distributor {
	volatile unsigned int control;	/* 0x000 */
	const unsigned int controller_type;
	const unsigned int implementer;
	const char padding1[116];
	volatile unsigned int security[32];	/* 0x080 */
	struct set_and_clear_regs enable;	/* 0x100 */
	struct set_and_clear_regs pending;	/* 0x200 */
	struct set_and_clear_regs active;	/* 0x300 */
	volatile unsigned int priority[256];	/* 0x400 */
	volatile unsigned int target[256];	/* 0x800 */
	volatile unsigned int configuration[64];	/* 0xC00 */
	const char padding3[256];	/* 0xD00 */
	volatile unsigned int non_security_access_control[64];	/* 0xE00 */
	volatile unsigned int software_interrupt;	/* 0xF00 */
	volatile unsigned int sgi_clr_pending[4];	/* 0xF10 */
	volatile unsigned int sgi_set_pending[4];	/* 0xF20 */
	const char padding4[176];

	unsigned const int peripheral_id[4];	/* 0xFE0 */
	unsigned const int primecell_id[4];	/* 0xFF0 */
};

struct cpu_interface {
	volatile unsigned int control;	/* 0x00 */
	volatile unsigned int priority_mask;	/* 0x04 */
	volatile unsigned int binary_point;	/* 0x08 */

	volatile unsigned const int interrupt_ack;	/* 0x0c */
	volatile unsigned int end_of_interrupt;	/* 0x10 */

	volatile unsigned const int running_priority;	/* 0x14 */
	volatile unsigned const int highest_pending;	/* 0x18 */
	volatile unsigned int aliased_binary_point;	/* 0x1c */

	volatile unsigned const int aliased_interrupt_ack;	/* 0x20 */
	volatile unsigned int alias_end_of_interrupt;	/* 0x24 */
	volatile unsigned int aliased_highest_pending;	/* 0x28 */
};

struct gic_cpu_context {
	unsigned int gic_cpu_if_regs[32];	/* GIC context local to the CPU */
	unsigned int gic_dist_if_pvt_regs[32];	/* GIC SGI/PPI context local to the CPU */
	unsigned int gic_dist_if_regs[512];	/* GIC distributor context to be saved by the last cpu. */
};

struct gic_cpu_context gic_data[1];
#define gic_data_base() ((struct gic_cpu_context *)&gic_data[0])

/*
 * Saves the GIC CPU interface context
 * Requires 3 words of memory
 */
static void save_gic_interface(u32 *pointer, unsigned gic_interface_address)
{
	struct cpu_interface *ci = (struct cpu_interface *)gic_interface_address;

	pointer[0] = ci->control;
	pointer[1] = ci->priority_mask;
	pointer[2] = ci->binary_point;
	pointer[3] = ci->aliased_binary_point;
	pointer[4] = ci->aliased_highest_pending;
}

/*
 * Saves this CPU's banked parts of the distributor
 * Returns non-zero if an SGI/PPI interrupt is pending (after saving all required context)
 * Requires 19 words of memory
 */
static void save_gic_distributor_private(u32 *pointer, unsigned gic_distributor_address)
{
	struct interrupt_distributor *id = (struct interrupt_distributor *)gic_distributor_address;
	unsigned int *ptr = 0x0;

	/*  Save SGI,PPI enable status */
	*pointer = id->enable.set[0];
	++pointer;
	/*  Save SGI,PPI priority status */
	pointer = copy_words(pointer, id->priority, 8);
	/*  Save SGI,PPI target status */
	pointer = copy_words(pointer, id->target, 8);
	/*  Save just the PPI configurations (SGIs are not configurable) */
	*pointer = id->configuration[1];
	++pointer;
	/*  Save SGI,PPI security status */
	*pointer = id->security[0];
	++pointer;

	/*  Save SGI Non-security status (PPI is read-only) */
	*pointer = id->non_security_access_control[0] & 0x0ffff;
	++pointer;

	/*  Save SGI,PPI pending status */
	*pointer = id->pending.set[0];
	++pointer;

	/*
	 * IPIs are different and can be replayed just by saving
	 * and restoring the set/clear pending registers
	 */
	ptr = pointer;
	copy_words(pointer, id->sgi_set_pending, 4);
	pointer += 8;

	/*
	 * Clear the pending SGIs on this cpuif so that they don't
	 * interfere with the wfi later on.
	 */
	copy_words(id->sgi_clr_pending, ptr, 4);
}

/*
 * Saves the shared parts of the distributor
 * Requires 1 word of memory, plus 20 words for each block of 32 SPIs (max 641 words)
 * Returns non-zero if an SPI interrupt is pending (after saving all required context)
 */
static void save_gic_distributor_shared(u32 *pointer, unsigned gic_distributor_address)
{
	struct interrupt_distributor *id = (struct interrupt_distributor *)gic_distributor_address;
	unsigned num_spis, *saved_pending;

	/* Calculate how many SPIs the GIC supports */
	num_spis = 32 * (id->controller_type & 0x1f);

	/* TODO: add nonsecure stuff */

	/* Save rest of GIC configuration */
	if (num_spis) {
		pointer = copy_words(pointer, id->enable.set + 1, num_spis / 32);
		pointer = copy_words(pointer, id->priority + 8, num_spis / 4);
		pointer = copy_words(pointer, id->target + 8, num_spis / 4);
		pointer = copy_words(pointer, id->configuration + 2, num_spis / 16);
		pointer = copy_words(pointer, id->security + 1, num_spis / 32);
		saved_pending = pointer;
		pointer = copy_words(pointer, id->pending.set + 1, num_spis / 32);

		pointer = copy_words(pointer, id->non_security_access_control + 1, num_spis / 16);
	}

	/* Save control register */
	*pointer = id->control;
}

static void restore_gic_interface(u32 *pointer, unsigned gic_interface_address)
{
	struct cpu_interface *ci = (struct cpu_interface *)gic_interface_address;

	ci->priority_mask = pointer[1];
	ci->binary_point = pointer[2];
	ci->aliased_binary_point = pointer[3];

	ci->aliased_highest_pending = pointer[4];

	/* Restore control register last */
	ci->control = pointer[0];
}

static void restore_gic_distributor_private(u32 *pointer, unsigned gic_distributor_address)
{
	struct interrupt_distributor *id = (struct interrupt_distributor *)gic_distributor_address;
	unsigned tmp;
	/* unsigned ctr, prev_val = 0, prev_ctr = 0; */

	/* First disable the distributor so we can write to its config registers */
	tmp = id->control;
	id->control = 0;
	/* Restore SGI,PPI enable status */
	id->enable.set[0] = *pointer;
	++pointer;
	/* Restore SGI,PPI priority  status */
	copy_words(id->priority, pointer, 8);
	pointer += 8;
	/* Restore SGI,PPI target status */
	copy_words(id->target, pointer, 8);
	pointer += 8;
	/* Restore just the PPI configurations (SGIs are not configurable) */
	id->configuration[1] = *pointer;
	++pointer;
	/* Restore SGI,PPI security status */
	id->security[0] = *pointer;
	++pointer;

	/* restore SGI Non-security status (PPI is read-only) */
	id->non_security_access_control[0] =
	    (id->non_security_access_control[0] & 0x0ffff0000) | (*pointer);
	++pointer;

	/*  Restore SGI,PPI pending status */
	id->pending.set[0] = *pointer;
	++pointer;

	/*
	 * Restore pending SGIs
	 */
	copy_words(id->sgi_set_pending, pointer, 4);
	pointer += 4;

	id->control = tmp;
}

static void restore_gic_spm_irq(unsigned long gic_distributor_address)
{
	struct interrupt_distributor *id = (struct interrupt_distributor *)gic_distributor_address;
	unsigned int backup;
/*	int i, j; */

	backup = id->control;
	id->control = 0;

	/* Set the pending bit for spm wakeup source that is edge triggerd */
#if 0
	if (reg_read(SPM_SLEEP_ISR_RAW_STA) & WAKE_SRC_KP) {
		i = DMT_KP_IRQ_BIT / GIC_PRIVATE_SIGNALS;
		j = DMT_KP_IRQ_BIT % GIC_PRIVATE_SIGNALS;
		id->pending.set[i] |= (1 << j);
	}
	if (reg_read(SPM_SLEEP_ISR_RAW_STA) & WAKE_SRC_CONN_WDT) {
		i = DMT_CONN_WDT_IRQ_BIT / GIC_PRIVATE_SIGNALS;
		j = DMT_CONN_WDT_IRQ_BIT % GIC_PRIVATE_SIGNALS;
		id->pending.set[i] |= (1 << j);
	}
	if (reg_read(SPM_SLEEP_ISR_RAW_STA) & WAKE_SRC_LOW_BAT) {
		i = DMT_LOWBATTERY_IRQ_BIT / GIC_PRIVATE_SIGNALS;
		j = DMT_LOWBATTERY_IRQ_BIT % GIC_PRIVATE_SIGNALS;
		id->pending.set[i] |= (1 << j);
	}
	if (reg_read(SPM_SLEEP_ISR_RAW_STA) & WAKE_SRC_MD1_WDT) {
		i = DMT_MD1_WDT_BIT / GIC_PRIVATE_SIGNALS;
		j = DMT_MD1_WDT_BIT % GIC_PRIVATE_SIGNALS;
		id->pending.set[i] |= (1 << j);
	}
#endif
	id->control = backup;
}

static void restore_gic_distributor_shared(u32 *pointer, unsigned gic_distributor_address)
{
	struct interrupt_distributor *id = (struct interrupt_distributor *)gic_distributor_address;
	unsigned num_spis;

	/* First disable the distributor so we can write to its config registers */
	id->control = 0;

	/* Calculate how many SPIs the GIC supports */
	num_spis = 32 * ((id->controller_type) & 0x1f);

	/* Restore rest of GIC configuration */
	if (num_spis) {
		copy_words(id->enable.set + 1, pointer, num_spis / 32);
		pointer += num_spis / 32;
		copy_words(id->priority + 8, pointer, num_spis / 4);
		pointer += num_spis / 4;
		copy_words(id->target + 8, pointer, num_spis / 4);
		pointer += num_spis / 4;
		copy_words(id->configuration + 2, pointer, num_spis / 16);
		pointer += num_spis / 16;
		copy_words(id->security + 1, pointer, num_spis / 32);
		pointer += num_spis / 32;
		copy_words(id->pending.set + 1, pointer, num_spis / 32);
		pointer += num_spis / 32;

		copy_words(id->non_security_access_control + 1, pointer, num_spis / 16);
		pointer += num_spis / 16;

		restore_gic_spm_irq(gic_distributor_address);

	}

	/* We assume the I and F bits are set in the CPSR so that we will not respond to interrupts! */
	/* Restore control register */
	id->control = *pointer;
}

static void gic_cpu_save(void)
{
	save_gic_interface(gic_data_base()->gic_cpu_if_regs, DMT_GIC_CPU_BASE);
	/*
	 * TODO:
	 * Is it safe for the secondary cpu to save its context
	 * while the GIC distributor is on. Should be as its
	 * banked context and the cpu itself is the only one
	 * who can change it. Still have to consider cases e.g
	 * SGIs/Localtimers becoming pending.
	 */
	/* Save distributoer interface private context */
	save_gic_distributor_private(gic_data_base()->gic_dist_if_pvt_regs, DMT_GIC_DIST_BASE);
}

static void gic_dist_save(void)
{
	/* Save distributoer interface global context */
	save_gic_distributor_shared(gic_data_base()->gic_dist_if_regs, DMT_GIC_DIST_BASE);
}

static void gic_dist_restore(void)
{
	/*restores the global context  */
	restore_gic_distributor_shared(gic_data_base()->gic_dist_if_regs, DMT_GIC_DIST_BASE);
}

void gic_cpu_restore(void)
{
	/*restores the private context  */
	restore_gic_distributor_private(gic_data_base()->gic_dist_if_pvt_regs, DMT_GIC_DIST_BASE);
	/* Restore GIC context */
	restore_gic_interface(gic_data_base()->gic_cpu_if_regs, DMT_GIC_CPU_BASE);
}

DEFINE_SPINLOCK(mp0_l2rstd_lock);
DEFINE_SPINLOCK(mp1_l2rstd_lock);

static inline void mp0_l2rstdisable(int flags)
{
	unsigned int read_back;
	int reg_val;

	spin_lock(&mp0_l2rstd_lock);	/* avoid MCDI racing on */

	read_back = reg_read(MP0_CA7L_CACHE_CONFIG);
	reg_val = _aor(read_back, ~L2RSTDISABLE, IS_DORMANT_INNER_OFF(flags) ? 0 : L2RSTDISABLE);

	reg_write(MP0_CA7L_CACHE_CONFIG, reg_val);

	if (GET_CLUSTER_DATA()->l2rstdisable_rfcnt++ == 0)
		GET_CLUSTER_DATA()->l2rstdisable = read_back & L2RSTDISABLE;

	spin_unlock(&mp0_l2rstd_lock);
}

static inline void mp1_l2rstdisable(int flags)
{
	unsigned int read_back;
	int reg_val;

	spin_lock(&mp1_l2rstd_lock);	/* avoid MCDI racing on */

	read_back = reg_read(MP1_CA7L_CACHE_CONFIG);
	reg_val = _aor(read_back, ~L2RSTDISABLE, IS_DORMANT_INNER_OFF(flags) ? 0 : L2RSTDISABLE);

	reg_write(MP1_CA7L_CACHE_CONFIG, reg_val);

	if (GET_CLUSTER_DATA()->l2rstdisable_rfcnt++ == 0)
		GET_CLUSTER_DATA()->l2rstdisable = read_back & L2RSTDISABLE;

	spin_unlock(&mp1_l2rstd_lock);
}

static inline void mp0_l2rstdisable_restore(int flags)
{
	unsigned int read_back;
	int reg_val;

	spin_lock(&mp0_l2rstd_lock);	/* avoid MCDI racing on */
	GET_CLUSTER_DATA()->l2rstdisable_rfcnt--;
	if (GET_CLUSTER_DATA()->l2rstdisable_rfcnt == 0) {
		read_back = reg_read(MP0_CA7L_CACHE_CONFIG);
		reg_val = _aor(read_back, ~L2RSTDISABLE, GET_CLUSTER_DATA()->l2rstdisable);

		reg_write(MP0_CA7L_CACHE_CONFIG, reg_val);
	}

	spin_unlock(&mp0_l2rstd_lock);	/* avoid MCDI racing on */
}

static inline void mp1_l2rstdisable_restore(int flags)
{
	unsigned int read_back;
	int reg_val;

	spin_lock(&mp1_l2rstd_lock);	/* avoid MCDI racing on */
	GET_CLUSTER_DATA()->l2rstdisable_rfcnt--;
	if (GET_CLUSTER_DATA()->l2rstdisable_rfcnt == 0) {
		read_back = reg_read(MP1_CA7L_CACHE_CONFIG);
		reg_val = _aor(read_back, ~L2RSTDISABLE, GET_CLUSTER_DATA()->l2rstdisable);

		reg_write(MP1_CA7L_CACHE_CONFIG, reg_val);
	}
	spin_unlock(&mp1_l2rstd_lock);	/* avoid MCDI racing on */
}

static void mt_cluster_save(int flags)
{
	if (read_cluster_id() == 0)
		mp0_l2rstdisable(flags);
	else
		mp1_l2rstdisable(flags);
}

static void mt_cluster_restore(int flags)
{
	if (read_cluster_id() == 0)
		mp0_l2rstdisable_restore(flags);
	else
		mp1_l2rstdisable_restore(flags);
}

void mt_cpu_save(void)
{
	struct core_context *core;
	struct cluster_context *cluster;
	unsigned int sleep_sta;
	int cpuid, clusterid;

	read_id(&cpuid, &clusterid);

	core = GET_CORE_DATA();

	mt_save_generic_timer((unsigned int *)core->timer_data, 0x0);
	stop_generic_timer();

	if (clusterid == 0)
		sleep_sta = (spm_read(SPM_SLEEP_TIMER_STA) >> 16) & 0x0f;
	else
		sleep_sta = (spm_read(SPM_SLEEP_TIMER_STA) >> 20) & 0x0f;

	if ((sleep_sta | (1 << cpuid)) == 0x0f) {	/* last core */
		cluster = GET_CLUSTER_DATA();
		mt_save_dbg_regs((unsigned int *)cluster->dbg_data, cpuid + (clusterid * 4));
	}
}

void mt_cpu_restore(void)
{
	struct core_context *core;
	struct cluster_context *cluster;
	unsigned int sleep_sta;
	int cpuid, clusterid;

	read_id(&cpuid, &clusterid);

	core = GET_CORE_DATA();

	if (clusterid == 0)
		sleep_sta = (spm_read(SPM_SLEEP_TIMER_STA) >> 16) & 0x0f;
	else
		sleep_sta = (spm_read(SPM_SLEEP_TIMER_STA) >> 20) & 0x0f;

	sleep_sta = (sleep_sta | (1 << cpuid));

	if (sleep_sta == 0x0f) {	/* first core */
		cluster = GET_CLUSTER_DATA();
		mt_restore_dbg_regs((unsigned int *)cluster->dbg_data, cpuid + (clusterid * 4));
	} else {
		int any = __builtin_ffs(~sleep_sta) - 1;

		mt_copy_dbg_regs(cpuid + (clusterid * 4), any + (clusterid * 4));
	}

	mt_restore_generic_timer((unsigned int *)core->timer_data, 0x0);
}

void mt_platform_save_context(int flags)
{
	mt_cpu_save();
	mt_cluster_save(flags);

	if (IS_DORMANT_GIC_OFF(flags)) {
		gic_cpu_save();
		gic_dist_save();
	}
}

void mt_platform_restore_context(int flags)
{
	mt_cluster_restore(flags);
	mt_cpu_restore();

	if (IS_DORMANT_GIC_OFF(flags)) {
		gic_dist_restore();
		gic_cpu_restore();
	}
}

int mt_cpu_dormant_reset(unsigned long flags)
{
	int ret = 1;	/* dormant abort */

	int cpuid, clusterid;

	read_id(&cpuid, &clusterid);

	disable_dcache_safe(!!IS_DORMANT_INNER_OFF(flags));

	if ((unlikely(IS_DORMANT_BREAK_CHECK(flags)) &&
	     unlikely(SPM_IS_CPU_IRQ_OCCUR(SPM_CORE_ID())))) {
		ret = 2;	/* dormant break */
		goto _break0;
	}

#if defined(CONFIG_MTK_IN_HOUSE_TEE_SUPPORT)
	DORMANT_LOG(clusterid * 4 + cpuid, 0x301);

	kree_pm_cpu_dormant(3); /* SHUTDOWN_MODE */

	DORMANT_LOG(clusterid * 4 + cpuid, 0x302);
#else
	amp();

	DORMANT_LOG(clusterid * 4 + cpuid, 0x301);

	wfi();

	smp();

	DORMANT_LOG(clusterid * 4 + cpuid, 0x302);
#endif
_break0:

	enable_dcache();

	DORMANT_LOG(clusterid * 4 + cpuid, 0x303);

	return ret;
}

#define get_data_nommu(va)				\
	({						\
		register int data = 0;			\
		register unsigned long pva = (unsigned long)(void *)(&(va)); \
		mt_get_data_nommu(data, pva);		\
		data;					\
	})

__naked void cpu_resume_wrapper(void)
{
	register int val;

#ifdef CONFIG_MTK_RAM_CONSOLE
	reg_write(get_data_nommu(sleep_aee_rec_cpu_dormant_pa), 0x401);
#endif

	/*
	 * restore L2 SRAM latency:
	 * This register can only be written when the L2 memory system is
	 * idle. ARM recommends that you write to this register after a
	 * powerup reset before the MMU is enabled and before any AXI4 or
	 * ACP traffic has begun.
	 */
	val = get_data_nommu(dormant_data[0].poc.l2ctlr);
	if (val) {
		val &= 0x3ffff;
		mt_restore_l2ctlr(val);
	}

	/* jump to cpu_resume() */
	mt_goto_cpu_resume(&(dormant_data[0].poc.cpu_resume_phys));
}

static int mt_cpu_dormant_abort(unsigned long index)
{
	int cpuid, clusterid;

	read_id(&cpuid, &clusterid);

#ifdef CONFIG_TRUSTONIC_TEE_SUPPORT
	if (cpuid == 0)
		mt_secure_call(MC_FC_SLEEP_CANCELLED, 0, 0, 0);
#elif defined(CONFIG_TRUSTY)
	if (cpuid == 0)
		mt_trusty_call(SMC_FC_CPU_DORMANT_CANCEL, 0, 0, 0);
#endif

	/* restore l2rstdisable setting */
	if (read_cluster_id() == 0)
		mp0_l2rstdisable_restore(index);
	else
		mp1_l2rstdisable_restore(index);

	start_generic_timer();

	return 0;
}

int mt_cpu_dormant(unsigned long flags)
{
	int ret;
	int cpuid, clusterid;

	if (!mt_dormant_initialized)
		return MT_CPU_DORMANT_BYPASS;

	read_id(&cpuid, &clusterid);

	DORMANT_LOG(clusterid * MAX_CORES + cpuid, 0x101);

	WARN_ON(!irqs_disabled());

	/* to mark as cpu clobs vfp register. */
	kernel_neon_begin();

	/* dormant break */
	if (IS_DORMANT_BREAK_CHECK(flags) && SPM_IS_CPU_IRQ_OCCUR(SPM_CORE_ID())) {
		ret = MT_CPU_DORMANT_BREAK_V(IRQ_PENDING_1);
		goto dormant_exit;
	}

	mt_platform_save_context(flags);

	DORMANT_LOG(clusterid * MAX_CORES + cpuid, 0x102);

	/* dormant break */
	if (IS_DORMANT_BREAK_CHECK(flags) && SPM_IS_CPU_IRQ_OCCUR(SPM_CORE_ID())) {
		mt_cpu_dormant_abort(flags);
		ret = MT_CPU_DORMANT_BREAK_V(IRQ_PENDING_2);
		goto dormant_exit;
	}

	/* cpu power down and cpu reset flow with idmap. */
	/* set reset vector */
#ifdef CONFIG_TRUSTONIC_TEE_SUPPORT
	mt_secure_call(MC_FC_SLEEP, virt_to_phys(cpu_resume), cpuid, 0);
#elif defined(CONFIG_TRUSTY)
	mt_trusty_call(SMC_FC_CPU_DORMANT, virt_to_phys(cpu_resume), cpuid, 0);
#else
	writel_relaxed(virt_to_phys(cpu_resume), DMT_BOOTROM_BOOT_ADDR);
	dormant_data[0].poc.cpu_resume_phys = (void (*)(void))(long)virt_to_phys(cpu_resume);
#endif

#if defined(CONFIG_MTK_IN_HOUSE_TEE_SUPPORT)
	*((unsigned int *)NS_SLAVE_BOOT_ADDR) = virt_to_phys(cpu_resume);
#endif

	DORMANT_LOG(clusterid * MAX_CORES + cpuid, 0x103);

	ret = cpu_suspend(flags, mt_cpu_dormant_reset);

	DORMANT_LOG(clusterid * MAX_CORES + cpuid, 0x601);

	if (IS_DORMANT_INNER_OFF(flags)) {
		reg_write(DMT_BOOTROM_BOOT_ADDR, virt_to_phys(cpu_wake_up_errata_802022));

#ifdef CONFIG_TRUSTONIC_TEE_SUPPORT
		mt_secure_call(MC_FC_SET_RESET_VECTOR, virt_to_phys(cpu_wake_up_errata_802022), 1,
			       0);
		if (num_possible_cpus() == 4) {
			mt_secure_call(MC_FC_SET_RESET_VECTOR,
				       virt_to_phys(cpu_wake_up_errata_802022), 2, 0);
			mt_secure_call(MC_FC_SET_RESET_VECTOR,
				       virt_to_phys(cpu_wake_up_errata_802022), 3, 0);
		}
#elif defined(CONFIG_TRUSTY)
		mt_trusty_call(SMC_FC_CPU_ON, virt_to_phys(cpu_wake_up_errata_802022), 1, 0);
		if (num_possible_cpus() == 4) {
			mt_trusty_call(SMC_FC_CPU_ON, virt_to_phys(cpu_wake_up_errata_802022), 2,
				       0);
			mt_trusty_call(SMC_FC_CPU_ON, virt_to_phys(cpu_wake_up_errata_802022), 3,
				       0);
		}
#endif
#if 0				/* disable before mtcmos ready */
		spm_mtcmos_ctrl_cpu1(STA_POWER_ON, 1);
		if (num_possible_cpus() == 4) {
			spm_mtcmos_ctrl_cpu2(STA_POWER_ON, 1);
			spm_mtcmos_ctrl_cpu3(STA_POWER_ON, 1);

			spm_mtcmos_ctrl_cpu3(STA_POWER_DOWN, 1);
			spm_mtcmos_ctrl_cpu2(STA_POWER_DOWN, 1);
		}
		spm_mtcmos_ctrl_cpu1(STA_POWER_DOWN, 1);
#endif
#ifdef CONFIG_TRUSTONIC_TEE_SUPPORT
		mt_secure_call(MC_FC_ERRATA_808022, 0, 0, 0);
#elif defined(CONFIG_TRUSTY)
		mt_trusty_call(SMC_FC_CPU_ERRATA_802022, 0, 0, 0);
#endif

#if defined(CONFIG_MTK_IN_HOUSE_TEE_SUPPORT)
		kree_pm_cpu_dormant_workaround_wake(1);

		spm_mtcmos_ctrl_cpu1(STA_POWER_ON, 1);
		if (num_possible_cpus() == 4) {
			spm_mtcmos_ctrl_cpu2(STA_POWER_ON, 1);
			spm_mtcmos_ctrl_cpu3(STA_POWER_ON, 1);

			spm_mtcmos_ctrl_cpu3(STA_POWER_DOWN, 1);
			spm_mtcmos_ctrl_cpu2(STA_POWER_DOWN, 1);
		}
		spm_mtcmos_ctrl_cpu1(STA_POWER_DOWN, 1);

		/* set to normal boot address after workaround finish */
		kree_pm_cpu_dormant_workaround_wake(0);
#endif
	}

	switch (ret) {
	case 0:		/* back from dormant reset */
		mt_platform_restore_context(flags);
		ret = MT_CPU_DORMANT_RESET;
		break;

	case 1:		/* back from dormant abort, */
		mt_cpu_dormant_abort(flags);
		ret = MT_CPU_DORMANT_ABORT;
		break;
	case 2:
		mt_cpu_dormant_abort(flags);
		ret = MT_CPU_DORMANT_BREAK_V(IRQ_PENDING_3);
		break;
	default:		/* back from dormant break, do nothing for return */
		dormant_err("EOPNOTSUPP\n");
		break;
	}

	DORMANT_LOG(clusterid * MAX_CORES + cpuid, 0x602);

	local_fiq_enable();

dormant_exit:

	kernel_neon_end();

	DORMANT_LOG(clusterid * MAX_CORES + cpuid, 0x0);

	return ret & 0x0ff;
}

static int mt_dormant_dts_map(void)
{
	struct device_node *node;
#if 0
	u32 kp_interrupt[3];
	u32 consys_interrupt[6];
	u32 auxadc_interrupt[3];
	u32 mdcldma_interrupt[6];
#endif
	node = of_find_compatible_node(NULL, NULL, MCUCFG_NODE);
	if (!node) {
		dormant_err("error: cannot find node " MCUCFG_NODE);
		WARN_ON(1);
	}
	mcucfg_base = (unsigned long)of_iomap(node, 0);
	if (!mcucfg_base) {
		dormant_err("error: cannot iomap " MCUCFG_NODE);
		WARN_ON(1);
	}
	of_node_put(node);

	node = of_find_compatible_node(NULL, NULL, INFRACFG_AO_NODE);
	if (!node) {
		dormant_err("error: cannot find node " INFRACFG_AO_NODE);
		WARN_ON(1);
	}
	infracfg_ao_base = (unsigned long)of_iomap(node, 0);
	if (!infracfg_ao_base) {
		dormant_err("error: cannot iomap " INFRACFG_AO_NODE);
		WARN_ON(1);
	}
	of_node_put(node);

	node = of_find_compatible_node(NULL, NULL, GIC_NODE);
	if (!node) {
		dormant_err("error: cannot find node " GIC_NODE);
		WARN_ON(1);
	}
	gic_id_base = (unsigned long)of_iomap(node, 0);
	gic_ci_base = (unsigned long)of_iomap(node, 1);
	if (!gic_id_base || !gic_ci_base) {
		dormant_err("error: cannot iomap " GIC_NODE);
		WARN_ON(1);
	}
	of_node_put(node);
#if 0
	node = of_find_compatible_node(NULL, NULL, KP_NODE);
	if (!node) {
		dormant_err("error: cannot find node " KP_NODE);
		WARN_ON(1);
	}
	if (of_property_read_u32_array(node, "interrupts", kp_interrupt, ARRAY_SIZE(kp_interrupt))) {
		dormant_err("error: cannot property_read " KP_NODE);
		WARN_ON(1);
	}
	kp_irq_bit = ((1 - kp_interrupt[0]) << 5) + kp_interrupt[1];	/* irq[0] = 0 => spi */
	of_node_put(node);
	dormant_dbg("kp_irq_bit = %u\n", kp_irq_bit);

	node = of_find_compatible_node(NULL, NULL, CONSYS_NODE);
	if (!node) {
		dormant_err("error: cannot find node " CONSYS_NODE);
		WARN_ON(1);
	}
	if (of_property_read_u32_array(node, "interrupts",
				       consys_interrupt, ARRAY_SIZE(consys_interrupt))) {
		dormant_err("error: cannot property_read " CONSYS_NODE);
		WARN_ON(1);
	}
	conn_wdt_irq_bit = ((1 - consys_interrupt[3]) << 5) + consys_interrupt[4];	/* irq[0] = 0 => spi */
	of_node_put(node);
	dormant_dbg("conn_wdt_irq_bit = %u\n", conn_wdt_irq_bit);

	node = of_find_compatible_node(NULL, NULL, AUXADC_NODE);
	if (!node) {
		dormant_err("error: cannot find node " AUXADC_NODE);
		WARN_ON(1);
	}
	if (of_property_read_u32_array(node, "interrupts",
				       auxadc_interrupt, ARRAY_SIZE(auxadc_interrupt))) {
		dormant_err("error: cannot property_read " AUXADC_NODE);
		WARN_ON(1);
	}
	lowbattery_irq_bit = ((1 - auxadc_interrupt[0]) << 5) + auxadc_interrupt[1];	/* irq[0] = 0 => spi */
	of_node_put(node);
	dormant_dbg("lowbattery_irq_bit = %u\n", lowbattery_irq_bit);

	node = of_find_compatible_node(NULL, NULL, MDCLDMA_NODE);
	if (!node) {
		dormant_err("error: cannot find node " MDCLDMA_NODE);
		WARN_ON(1);
	}
	if (of_property_read_u32_array(node, "interrupts",
				       mdcldma_interrupt, ARRAY_SIZE(mdcldma_interrupt))) {
		dormant_err("error: cannot property_read " MDCLDMA_NODE);
		WARN_ON(1);
	}
	md1_wdt_bit = ((1 - mdcldma_interrupt[3]) << 5) + mdcldma_interrupt[4];	/* irq[0] = 0 => spi */
	of_node_put(node);
	dormant_dbg("md1_wdt_bit = %u\n", md1_wdt_bit);
#endif
	return 0;
}

static int __init mt_cpu_dormant_init(void)
{
	int cpuid, clusterid;

	read_id(&cpuid, &clusterid);

	if (mt_dormant_initialized == 1)
		return MT_CPU_DORMANT_BYPASS;

	mt_dormant_dts_map();

	/* enable bootrom power down mode */
	writel_relaxed(readl(DMT_BOOTROM_PWR_CTRL) | SW_ROM_PD, DMT_BOOTROM_PWR_CTRL);

	mt_save_l2ctlr(dormant_data[0].poc.l2ctlr);

	mt_dormant_initialized = 1;

	return 0;
}
postcore_initcall(mt_cpu_dormant_init);
