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

#include <linux/cpuidle.h>
/* #include <linux/irqchip/mtk-gic.h> */
#include <linux/of_address.h>
#include <linux/of.h>
#include <linux/psci.h>

#include <asm/cacheflush.h>
#include <asm/cpuidle.h>
#include <asm/irqflags.h>
#include <asm/neon.h>
#include <asm/suspend.h>

/* #include <mt-plat/mt_dbg.h> */
/* #include <mt-plat/mtk_io.h> */
/* #include <mt-plat/sync_write.h> */
#if defined(CONFIG_MTK_RAM_CONSOLE) || defined(CONFIG_TRUSTONIC_TEE_SUPPORT)
/* #include <mt_secure_api.h> */
#endif

#include "mtk_cpuidle.h"
#include "mtk_spm.h"
#define TAG "[MTK_CPUIDLE] "

#define dormant_err(fmt, args...)	pr_err(TAG fmt, ##args)
#define dormant_warn(fmt, args...)	pr_warn(TAG fmt, ##args)
#define dormant_debug(fmt, args...)	pr_debug(TAG fmt, ##args)

#if 0 /* def CONFIG_MTK_RAM_CONSOLE */
unsigned long *sleep_aee_rec_cpu_dormant_pa;
unsigned long *sleep_aee_rec_cpu_dormant_va;
#endif

#if 0 /* no mtk gic */
static unsigned long gic_id_base;
static unsigned int kp_irq_bit;
static unsigned int conn_wdt_irq_bit;
static unsigned int lowbattery_irq_bit;
#endif

/* TODO: check this part */
#define CPUIDLE_CPU_IDLE_STA SPM_SLEEP_TIMER_STA
#define CPUIDLE_CPU_IDLE_STA_OFFSET 16
#if 0 /* no mtk gic */
#define CPUIDLE_SPM_WAKEUP_MISC SPM_SLEEP_WAKEUP_MISC
#define CPUIDLE_SPM_WAKEUP_STA SPM_SLEEP_ISR_RAW_STA
#define CPUIDLE_WAKE_SRC_R12_KP_IRQ_B WAKE_SRC_KP
#define CPUIDLE_WAKE_SRC_R12_CONN_WDT_IRQ_B WAKE_SRC_CONN_WDT
#define CPUIDLE_WAKE_SRC_R12_LOWBATTERY_IRQ_B WAKE_SRC_LOW_BAT
#endif
#define MT_CPUIDLE_TIME_PROFILING 0

#if MT_CPUIDLE_TIME_PROFILING
#define MT_CPUIDLE_TIMESTAMP_COUNT 16
#else
#define MT_CPUIDLE_TIMESTAMP_COUNT 0
#endif
unsigned long long mt_cpuidle_timestamp[CONFIG_NR_CPUS][MT_CPUIDLE_TIMESTAMP_COUNT];
unsigned long long timer_data[CONFIG_NR_CPUS][8];
unsigned long dbg_data[40];

static int mt_dormant_initialized;

#if 0 /* def CONFIG_MTK_RAM_CONSOLE */
#define MT_CPUIDLE_FOOTPRINT_LOG(cid, idx) (sleep_aee_rec_cpu_dormant_va[cid] |= (1 << idx))
#define MT_CPUIDLE_FOOTPRINT_LOG_CLEAR(cid) (sleep_aee_rec_cpu_dormant_va[cid] = 0)
#else
#define MT_CPUIDLE_FOOTPRINT_LOG(cid, idx)
#define MT_CPUIDLE_FOOTPRINT_LOG_CLEAR(cid)
#endif

#if MT_CPUIDLE_TIME_PROFILING
#define MT_CPUIDLE_TIMESTAMP_LOG(cpuid, idx) (mt_cpuidle_timestamp[cpuid][idx] = arch_counter_get_cntvct())
#else
#define MT_CPUIDLE_TIMESTAMP_LOG(cpuid, idx)
#endif

#define read_cpu_idx()						\
	({							\
		((read_cluster_id() >> 6) | read_cpu_id());	\
	})

unsigned int __weak *mt_save_dbg_regs(unsigned int *p, unsigned int cpuid)
{
	return p;
}
void __weak mt_restore_dbg_regs(unsigned int *p, unsigned int cpuid) { }
void __weak mt_copy_dbg_regs(int to, int from) { }
void __weak mt_save_banked_registers(unsigned int *container) { }
void __weak mt_restore_banked_registers(unsigned int *container) { }
void __weak mt_gic_cpu_init_for_low_power(void) { }
#if 0
void __weak switch_armpll_ll_hwmode(int enable) { }
void __weak switch_armpll_l_hwmode(int enable) { }
#endif

#define MAX_CORES 4
#define MAX_CLUSTER 1

inline void read_id(int *cpu_id, int *cluster_id)
{
	*cpu_id = read_cpu_id();
	*cluster_id = read_cluster_id();
}

struct core_context {
	unsigned int banked_regs[32];
	unsigned long long timestamp[5];
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

#define system_cluster(system, clusterid)	(&((struct system_context *)system)->cluster[clusterid])
#define cluster_core(cluster, cpuid)		(&((struct cluster_context *)cluster)->core[cpuid])

struct system_context dormant_data[1];

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

#if 0 /* no mtk gic */
struct interrupt_distributor {
	volatile unsigned int control;			/* 0x000 */
	const unsigned int controller_type;
	const unsigned int implementer;
	const char padding1[116];
	volatile unsigned int security[32];		/* 0x080 */
	struct set_and_clear_regs enable;		/* 0x100 */
	struct set_and_clear_regs pending;		/* 0x200 */
	struct set_and_clear_regs active;		/* 0x300 */
	volatile unsigned int priority[256];		/* 0x400 */
	volatile unsigned int target[256];		/* 0x800 */
	volatile unsigned int configuration[64];	/* 0xC00 */
	const char padding3[256];			/* 0xD00 */
	volatile unsigned int non_security_access_control[64]; /* 0xE00 */
	volatile unsigned int software_interrupt;	/* 0xF00 */
	volatile unsigned int sgi_clr_pending[4];	/* 0xF10 */
	volatile unsigned int sgi_set_pending[4];	/* 0xF20 */
	const char padding4[176];

	unsigned const int peripheral_id[4];		/* 0xFE0 */
	unsigned const int primecell_id[4];		/* 0xFF0 */
};

static void restore_gic_spm_irq(struct interrupt_distributor *id, int wake_src, int *irq_bit)
{
	int i, j;

	if (spm_read(CPUIDLE_SPM_WAKEUP_STA) & wake_src) {
		i = *irq_bit / 32;
		j = *irq_bit % 32;
		id->pending.set[i] |= (1 << j);
	}
}

static void restore_edge_gic_spm_irq(unsigned long gic_distributor_address)
{
	struct interrupt_distributor *id = (struct interrupt_distributor *) gic_distributor_address;
	unsigned int backup;

	backup = id->control;
	id->control = 0;

	/* Set the pending bit for spm wakeup source that is edge triggerd */
	restore_gic_spm_irq(id, CPUIDLE_WAKE_SRC_R12_KP_IRQ_B, &kp_irq_bit);
	restore_gic_spm_irq(id, CPUIDLE_WAKE_SRC_R12_CONN_WDT_IRQ_B, &conn_wdt_irq_bit);
	restore_gic_spm_irq(id, CPUIDLE_WAKE_SRC_R12_LOWBATTERY_IRQ_B, &lowbattery_irq_bit);

	id->control = backup;
}
#endif

static void mt_cluster_restore(int flags)
{
#if 0 /* defined(CONFIG_ARM_GIC_V3) */
	mt_gic_cpu_init_for_low_power();
#endif
}

void mt_cpu_save(void)
{
	struct core_context *core;
	unsigned int sleep_sta;
	int cpu_idx;

	cpu_idx = read_cpu_id();

	core = GET_CORE_DATA();

	mt_save_generic_timer((unsigned int *)timer_data[cpu_idx], 0x0);
	stop_generic_timer();

	/* TODO: check this part */
	sleep_sta = (spm_read(CPUIDLE_CPU_IDLE_STA) >> CPUIDLE_CPU_IDLE_STA_OFFSET) & 0xff;

	if ((sleep_sta | (1 << cpu_idx)) == 0xff) /* last core */
		mt_save_dbg_regs((unsigned int *)dbg_data, cpu_idx);

	mt_save_banked_registers(core->banked_regs);
}

void mt_cpu_restore(void)
{
	struct core_context *core;
	unsigned int sleep_sta;
	int cpu_idx;

	cpu_idx = read_cpu_id();

	core = GET_CORE_DATA();

	mt_restore_banked_registers(core->banked_regs);

	/* TODO: check this part */
	sleep_sta = (spm_read(CPUIDLE_CPU_IDLE_STA) >> CPUIDLE_CPU_IDLE_STA_OFFSET) & 0xff;
	sleep_sta = (sleep_sta | (1 << cpu_idx));

	if (sleep_sta == 0xff) /* first core */
		mt_restore_dbg_regs((unsigned int *)dbg_data, cpu_idx);
	else
		mt_copy_dbg_regs(cpu_idx, __builtin_ffs(~sleep_sta) - 1);

	mt_restore_generic_timer((unsigned int *)timer_data, 0x0);
}

void mt_platform_save_context(int flags)
{
	mt_cpu_save();
}

void mt_platform_restore_context(int flags)
{
	mt_cluster_restore(flags);
	mt_cpu_restore();

#if 0 /* no mtk gic */
	if (IS_DORMANT_GIC_OFF(flags))
		restore_edge_gic_spm_irq(gic_id_base);
#endif
}

#if !defined(CONFIG_ARM64)
int mt_cpu_dormant_psci(unsigned long flags)
{
	int ret = 1;
	int cpuid, clusterid;
	unsigned int state;

	read_id(&cpuid, &clusterid);

	state = 0x201 << 16;

	if (psci_ops.cpu_suspend)
		ret = psci_ops.cpu_suspend(state, virt_to_phys(cpu_resume));

	WARN_ON(1);

	return ret;
}
#endif

int mt_cpu_dormant(unsigned long flags)
{
	int i, cpu_idx;

	if (!mt_dormant_initialized)
		return MT_CPU_DORMANT_BYPASS;

	cpu_idx = read_cpu_idx();

	MT_CPUIDLE_FOOTPRINT_LOG(cpu_idx, 0);
	MT_CPUIDLE_TIMESTAMP_LOG(cpu_idx, 0);

	MT_CPUIDLE_FOOTPRINT_LOG(cpu_idx, 1);
	kernel_neon_begin();

	MT_CPUIDLE_FOOTPRINT_LOG(cpu_idx, 2);
	mt_platform_save_context(flags);

	MT_CPUIDLE_FOOTPRINT_LOG(cpu_idx, 3);
#if !defined(CONFIG_ARM64)
	cpu_suspend(2, mt_cpu_dormant_psci);
#else
	arm_cpuidle_suspend(2);
#endif
	MT_CPUIDLE_FOOTPRINT_LOG(cpu_idx, 12);
	mt_platform_restore_context(flags);

	MT_CPUIDLE_FOOTPRINT_LOG(cpu_idx, 13);
	local_fiq_enable();

	MT_CPUIDLE_FOOTPRINT_LOG(cpu_idx, 14);
	kernel_neon_end();

	MT_CPUIDLE_FOOTPRINT_LOG(cpu_idx, 15);

	MT_CPUIDLE_FOOTPRINT_LOG_CLEAR(cpu_idx);
	MT_CPUIDLE_TIMESTAMP_LOG(cpu_idx, 15);
#if MT_CPUIDLE_TIME_PROFILING
	request_uart_to_wakeup();
#endif
	for (i = 0; i < MT_CPUIDLE_TIMESTAMP_COUNT; i++)
		dormant_err("CPU%d,Timestamp%d,%llu\n", cpu_idx, i, mt_cpuidle_timestamp[cpu_idx][i]);

	return 0;
}

#if 0 /* no mtk gic */
static unsigned long get_dts_node_address(char *node_compatible, int index)
{
	unsigned long node_address = 0;
	struct device_node *node;

	if (!node_compatible)
		return 0;

	node = of_find_compatible_node(NULL, NULL, node_compatible);
	if (!node)
		dormant_err("error: cannot find node [%s]\n", node_compatible);

	node_address = (unsigned long)of_iomap(node, index);
	if (!node_address)
		dormant_err("error: cannot iomap [%s]\n", node_compatible);

	of_node_put(node);

	return node_address;
}

static u32 get_dts_node_irq_bit(char *node_compatible, const int int_size, int int_offset)
{
	struct device_node *node;
	u32 node_interrupt[int_size];
	unsigned int irq_bit;

	if (!node_compatible)
		return 0;

	node = of_find_compatible_node(NULL, NULL, node_compatible);
	if (!node)
		dormant_err("error: cannot find node [%s]\n", node_compatible);

	if (of_property_read_u32_array(node, "interrupts", node_interrupt, int_size))
		dormant_err("error: cannot property_read [%s]\n", node_compatible);
	/* irq[0] = 0 => spi */
	irq_bit = ((1 - node_interrupt[int_offset]) << 5) + node_interrupt[int_offset+1];
	of_node_put(node);
	dormant_debug("compatible = %s, irq_bit = %u\n", node_compatible, irq_bit);

	return irq_bit;
}

static void get_dts_nodes_address(void)
{
	gic_id_base = get_dts_node_address("arm,gic-400", 0);
}

static void get_dts_nodes_irq_bit(void)
{
	kp_irq_bit = get_dts_node_irq_bit("mediatek,mt2712-keypad", 3, 0);
	conn_wdt_irq_bit = get_dts_node_irq_bit("mediatek,mt2712-consys", 6, 3);
	lowbattery_irq_bit = get_dts_node_irq_bit("mediatek,mt2712-auxadc", 3, 0);
}

static int mt_dormant_dts_map(void)
{
	get_dts_nodes_address();
	get_dts_nodes_irq_bit();

	return 0;
}
#endif

static int __init mt_cpu_dormant_init(void)
{
	int i, k;

	if (mt_dormant_initialized == 1)
		return MT_CPU_DORMANT_BYPASS;

#if 0 /* no mtk gic */
	mt_dormant_dts_map();
#endif

	for (i = 0; i < num_possible_cpus(); i++)
		for (k = 0; k < MT_CPUIDLE_TIMESTAMP_COUNT; k++)
			mt_cpuidle_timestamp[i][k] = 0;
#if MT_CPUIDLE_TIME_PROFILING
	kernel_smc_msg(0, 1, virt_to_phys(mt_cpuidle_timestamp));
#endif

#if 0 /* def CONFIG_MTK_RAM_CONSOLE */
	sleep_aee_rec_cpu_dormant_va = aee_rr_rec_cpu_dormant();
	sleep_aee_rec_cpu_dormant_pa = aee_rr_rec_cpu_dormant_pa();

	WARN_ON(!sleep_aee_rec_cpu_dormant_va || !sleep_aee_rec_cpu_dormant_pa);

	kernel_smc_msg(0, 2, (long) sleep_aee_rec_cpu_dormant_pa);
#endif

	mt_dormant_initialized = 1;

	return 0;
}
postcore_initcall(mt_cpu_dormant_init);

