/*
 * Copyright (c) 2014 MediaTek Inc.
 * Author: Andrew-sh.Cheng <andrew-sh.cheng@mediatek.com>

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/cpu.h>
#include <linux/cpufreq.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/kthread.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/jiffies.h>
#include <linux/version.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/ratelimit.h>
#include "mt_hotplug_strategy.h"

#include "sync_write.h"

#include "mach/mt_freqhopping.h"
#include "mt_cpufreq.h"
#include <mach/mtk_thermal.h>

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/regulator/consumer.h>
#endif

#include <trustzone/tz_cross/ta_efuse.h>
#include <trustzone/tz_cross/efuse_info.h>

/* For duplicate mt_ptp.c functions */
u32 __attribute__ ((weak)) PTP_get_ptp_level(void)
{
	unsigned int spd_bin_resv = 0, segment = 0, ret = 0;
	unsigned char efuse_data[4];

	tee_fuse_read(M_HW_RES4, efuse_data, 4); /* M_HW_RES4 */
	segment = (efuse_data[3] >> 7) & 0x1;
	if (segment == 0) {
		tee_fuse_read(M_HW_RES4, efuse_data, 4); /* M_HW_RES4 */
		spd_bin_resv = efuse_data[0] | (efuse_data[1] << 8) | (efuse_data[2] << 16) | (efuse_data[3] << 24);
	} else {
		tee_fuse_read(M_HW_RES4, efuse_data, 4); /* M_HW_RES4 */
		spd_bin_resv = ((efuse_data[2]) | (efuse_data[3] << 8)) & 0x7FFF;
	}

	if (segment == 1) {
		if (spd_bin_resv == 0x1000)
			ret = 0; /* 1.3G */
		else if (spd_bin_resv == 0x1001)
			ret = 5; /* 1.222G */
		else if (spd_bin_resv == 0x1003)
			ret = 6; /* 1.118G */
		else
			ret = 0; /* 1.3G */
	} else if (spd_bin_resv != 0) {
		if (spd_bin_resv == 0x1000)
			ret = 3; /* 1.5G, 8127T */
		else
			ret = 0; /* 1.3G, */
	} else { /* free */
		tee_fuse_read(M_HW2_RES2, efuse_data, 4); /* M_HW2_RES2 */
		spd_bin_resv = efuse_data[0] & 0x7;
		switch (spd_bin_resv) {
		case 1:
			ret = 3; /* 1.5G */
			break;
		default:
			ret = 0; /* 1.3G */
			break;
		}
	}
	return ret;
}



/**************************************************
* Define register write function
***************************************************/
#define cpufreq_read(addr)                  __raw_readl(addr)
#define mt_cpufreq_reg_write(val, addr)        mt_reg_sync_writel(val, addr)

#define MT_BUCK_ADJUST
#define MT_CPUFREQ_FHCTL
#define FHCTL_CHANGE_FREQ (1000000)	/* KHz, If cross 1GHz when DFS, not used FHCTL. */

/***************************
* Operate Point Definition
****************************/
#define DVFS_F0_1   (1690000)	/* KHz */
#define DVFS_F0_2   (1599000)	/* KHz */
#define DVFS_F0_3   (1508000)	/* KHz */
#define DVFS_F0_4   (1391000)	/* KHz */
#define DVFS_F0     (1300000)	/* KHz */
#define DVFS_F1_0   (1222000)	/* KHz */
#define DVFS_F1     (1196000)	/* KHz */
#define DVFS_F1_1   (1118000)	/* KHz */
#define DVFS_F2     (1040000)	/* KHz */
#define DVFS_F3     (747500)	/* KHz */
#define DVFS_F4     (598000)	/* KHz */

#define DVFS_V0     (1300)	/* mV */
#define DVFS_V1     (1250)	/* mV */
#define DVFS_V2     (1150)	/* mV */

#define OP(khz, volt)       \
{                           \
	.cpufreq_khz = khz,     \
	.cpufreq_volt = volt,   \
}

struct mt_cpu_freq_info {
	unsigned int cpufreq_khz;
	unsigned int cpufreq_volt;
};

static struct mt_cpu_freq_info mt8127_freqs_e1[] = {
	OP(DVFS_F0, DVFS_V0),
	OP(DVFS_F1, DVFS_V1),
	OP(DVFS_F2, DVFS_V2),
	OP(DVFS_F3, DVFS_V2),
	OP(DVFS_F4, DVFS_V2),
};

static struct mt_cpu_freq_info mt8127_freqs_e1_1[] = {
	OP(DVFS_F0_1, DVFS_V0),
	OP(DVFS_F1, DVFS_V1),
	OP(DVFS_F2, DVFS_V2),
	OP(DVFS_F3, DVFS_V2),
	OP(DVFS_F4, DVFS_V2),
};

static struct mt_cpu_freq_info mt8127_freqs_e1_2[] = {
	OP(DVFS_F0_2, DVFS_V0),
	OP(DVFS_F1, DVFS_V1),
	OP(DVFS_F2, DVFS_V2),
	OP(DVFS_F3, DVFS_V2),
	OP(DVFS_F4, DVFS_V2),
};

static struct mt_cpu_freq_info mt8127_freqs_e1_3[] = {
	OP(DVFS_F0_3, DVFS_V0),
	OP(DVFS_F1, DVFS_V1),
	OP(DVFS_F2, DVFS_V2),
	OP(DVFS_F3, DVFS_V2),
	OP(DVFS_F4, DVFS_V2),
};

static struct mt_cpu_freq_info mt8127_freqs_e1_4[] = {
	OP(DVFS_F0_4, DVFS_V0),
	OP(DVFS_F1, DVFS_V1),
	OP(DVFS_F2, DVFS_V2),
	OP(DVFS_F3, DVFS_V2),
	OP(DVFS_F4, DVFS_V2),
};

static struct mt_cpu_freq_info mt8127_freqs_e1_5[] = {
	OP(DVFS_F1_0, DVFS_V1),
	OP(DVFS_F2, DVFS_V2),
	OP(DVFS_F3, DVFS_V2),
	OP(DVFS_F4, DVFS_V2),
};

static struct mt_cpu_freq_info mt8127_freqs_e1_6[] = {
	OP(DVFS_F1_1, DVFS_V1),
	OP(DVFS_F2, DVFS_V2),
	OP(DVFS_F3, DVFS_V2),
	OP(DVFS_F4, DVFS_V2),
};

#define ENABLE_DVS	1 /* if want to enable DVS, need to set regulator in DTS at the same time */

/***************************
* Device Node
****************************/
#if ENABLE_DVS
struct regulator *reg_vproc;	/* 0x226 */
#endif
static void __iomem *infracfg_ao_base;	/* 0x10001000 */
static void __iomem *apmixed_base;	/* 0x10209000 */
static void __iomem *clk_cksys_base;	/* 0x10209000 */
static void __iomem *pwrap_base;

#define INFRACFG_AO_BASE     infracfg_ao_base
#define TOP_CKMUXSEL            (INFRACFG_AO_BASE + 0x00)
#define TOP_CKDIV1              (INFRACFG_AO_BASE + 0x08)
#define CLK_CFG_8               (INFRACFG_AO_BASE + 0x0100)

#define APMIXED_BASE     apmixed_base
#define ARMPLL_CON0         (APMIXED_BASE + 0x0200)
#define ARMPLL_CON1         (APMIXED_BASE + 0x0204)

#define CLK_CKSYS_BASE	clk_cksys_base
#define CLK_MISC_CFG_1  (CLK_CKSYS_BASE + 0x214)
#define CLK26CALI_0     (CLK_CKSYS_BASE + 0x220)
#define CLK26CALI_1     (CLK_CKSYS_BASE + 0x224)

#define PWRAP_BASE_ADDR     pwrap_base	/* 0xF000D000 */
#define PMIC_WRAP_DVFS_ADR0     (PWRAP_BASE_ADDR + 0xE4)
#define PMIC_WRAP_DVFS_WDATA0   (PWRAP_BASE_ADDR + 0xE8)
#define PMIC_WRAP_DVFS_ADR1     (PWRAP_BASE_ADDR + 0xEC)
#define PMIC_WRAP_DVFS_WDATA1   (PWRAP_BASE_ADDR + 0xF0)
#define PMIC_WRAP_DVFS_ADR2     (PWRAP_BASE_ADDR + 0xF4)
#define PMIC_WRAP_DVFS_WDATA2   (PWRAP_BASE_ADDR + 0xF8)
#define PMIC_WRAP_DVFS_ADR3     (PWRAP_BASE_ADDR + 0xFC)
#define PMIC_WRAP_DVFS_WDATA3   (PWRAP_BASE_ADDR + 0x100)
#define PMIC_WRAP_DVFS_ADR4     (PWRAP_BASE_ADDR + 0x104)
#define PMIC_WRAP_DVFS_WDATA4   (PWRAP_BASE_ADDR + 0x108)
#define PMIC_WRAP_DVFS_ADR5     (PWRAP_BASE_ADDR + 0x10C)
#define PMIC_WRAP_DVFS_WDATA5   (PWRAP_BASE_ADDR + 0x110)
#define PMIC_WRAP_DVFS_ADR6     (PWRAP_BASE_ADDR + 0x114)
#define PMIC_WRAP_DVFS_WDATA6   (PWRAP_BASE_ADDR + 0x118)
#define PMIC_WRAP_DVFS_ADR7     (PWRAP_BASE_ADDR + 0x11C)
#define PMIC_WRAP_DVFS_WDATA7   (PWRAP_BASE_ADDR + 0x120)

#define TOP_CKMUXSEL_CLKSQ   0x0
#define TOP_CKMUXSEL_ARMPLL  0x1
#define TOP_CKMUXSEL_MAINPLL 0x2
#define TOP_CKMUXSEL_UNIVPLL 0x3

#define PLL_SETTLE_TIME (30)	/* us */
#define PMIC_SETTLE_TIME (40)	/* us */

/* pmic volt by PTP-OD */
static unsigned int mt_cpufreq_pmic_volt[8] = { 0 };

#define RAMP_DOWN_TIMES (0)
int g_ramp_down_count;

bool mt_cpufreq_debug;	/* /proc/cpufreq/cpufreq_debug */
bool mt_cpufreq_ready;
bool mt_cpufreq_pause;	/* /proc/cpufreq/cpufreq_state */
bool mt_cpufreq_ptpod_disable;
int	g_dvfs_disable_count;

unsigned int g_cur_freq;
unsigned int g_cur_cpufreq_volt;
unsigned int g_limited_max_ncpu;
unsigned int g_limited_max_freq;
unsigned int g_limited_min_freq;
unsigned int g_cpufreq_get_ptp_level;

/* for Thermal */
unsigned int g_thermal_protect_limited_power;
unsigned int g_cpu_power_table_num;
struct mtk_cpu_power_info *mt_cpu_power;

/* ptpod */
unsigned int g_max_freq_by_ptp = DVFS_F0;	/* default 1.3GHz */

bool mt_cpufreq_freq_table_allocated;
struct mt_cpu_freq_info *mt_cpu_freqs;
unsigned int mt_cpu_freqs_num;
struct cpufreq_frequency_table *mt_cpu_freqs_table;

/* static DEFINE_SPINLOCK(mt_cpufreq_lock); */

DEFINE_MUTEX(cpufreq_mutex);
bool is_in_cpufreq;
/* mt8173-TBD, mark spm_mcdi_wakeup_all_cores(), wait for it ready */
#define cpufreq_lock(flags) \
	do { \
		/* to fix compile warning */  \
		flags = (unsigned long)&flags; \
		mutex_lock(&cpufreq_mutex); \
		is_in_cpufreq = 1; \
	} while (0)

#define cpufreq_unlock(flags) \
	do { \
		/* to fix compile warning */  \
		flags = (unsigned long)&flags; \
		is_in_cpufreq = 0;\
		mutex_unlock(&cpufreq_mutex); \
	} while (0)

/***************************
* debug message
****************************/
#undef TAG

#define TAG     "[Power/cpufreq] "

#define cpufreq_err(fmt, args...)       \
	pr_err(TAG"[ERROR]"fmt, ##args)
#define cpufreq_warn(fmt, args...)      \
	pr_warn(TAG"[WARNING]"fmt, ##args)
#define cpufreq_info(fmt, args...)      \
	pr_warn(TAG""fmt, ##args)
#define cpufreq_dbg(fmt, args...)       \
	do {								\
		if (mt_cpufreq_debug)			\
			cpufreq_info(fmt, ##args);	   \
	} while (0)
#define cpufreq_ver(fmt, args...)       \
	do {								\
		if (mt_cpufreq_debug)			\
			pr_debug(TAG""fmt, ##args);    \
	} while (0)





















/***********************************************
* MT8127 E1 Raw Data: 1.3Ghz @ 1.15V @ TT 125C
************************************************/
#define P_MCU_L         (346)	/* MCU Leakage Power */
#define P_MCU_T         (1115)	/* MCU Total Power */
#define P_CA7_L         (61)	/* CA7 Leakage Power */
#define P_CA7_T         (240)	/* Single CA7 Core Power */

#define P_MCL99_105C_L  (658)	/* MCL99 Leakage Power @ 105C */
#define P_MCL99_25C_L   (93)	/* MCL99 Leakage Power @ 25C */
#define P_MCL50_105C_L  (316)	/* MCL50 Leakage Power @ 105C */
#define P_MCL50_25C_L   (35)	/* MCL50 Leakage Power @ 25C */

#define T_105           (105)	/* Temperature 105C */
#define T_60            (60)	/* Temperature 60C */
#define T_25            (25)	/* Temperature 25C */

#define P_MCU_D ((P_MCU_T - P_MCU_L) - 4 * (P_CA7_T - P_CA7_L))	/* MCU dynamic power except of CA7 cores */

#define P_TOTAL_CORE_L ((P_MCL99_105C_L  * 42165) / 100000)	/* Total leakage at T_65 */
#define P_EACH_CORE_L  ((P_TOTAL_CORE_L * ((P_CA7_L * 1000) / P_MCU_L)) / 1000)	/* 1 core leakage at T_65 */

#define P_CA7_D_1_CORE ((P_CA7_T - P_CA7_L) * 1)	/* CA7 dynamic power for 1 cores turned on */
#define P_CA7_D_2_CORE ((P_CA7_T - P_CA7_L) * 2)	/* CA7 dynamic power for 2 cores turned on */
#define P_CA7_D_3_CORE ((P_CA7_T - P_CA7_L) * 3)	/* CA7 dynamic power for 3 cores turned on */
#define P_CA7_D_4_CORE ((P_CA7_T - P_CA7_L) * 4)	/* CA7 dynamic power for 4 cores turned on */

#define A_1_CORE (P_MCU_D + P_CA7_D_1_CORE)	/* MCU dynamic power for 1 cores turned on */
#define A_2_CORE (P_MCU_D + P_CA7_D_2_CORE)	/* MCU dynamic power for 2 cores turned on */
#define A_3_CORE (P_MCU_D + P_CA7_D_3_CORE)	/* MCU dynamic power for 3 cores turned on */
#define A_4_CORE (P_MCU_D + P_CA7_D_4_CORE)	/* MCU dynamic power for 4 cores turned on */

/*****************************
* Thermal
******************************/
static void mt_cpufreq_power_calculation(int index, int ncpu)
{
	int multi = 0, p_dynamic = 0, p_leakage = 0, freq_ratio = 0, volt_square_ratio = 0;
	int possiblecpu = 0;

	possiblecpu = num_possible_cpus();

	volt_square_ratio =
	    (((mt_cpu_freqs[index].cpufreq_volt * 100) / 1150) *
	     ((mt_cpu_freqs[index].cpufreq_volt * 100) / 1150)) / 100;
	freq_ratio = (mt_cpu_freqs[index].cpufreq_khz / 1300);
	cpufreq_dbg("freq_ratio = %d, volt_square_ratio %d\n", freq_ratio, volt_square_ratio);

	multi =
	    ((mt_cpu_freqs[index].cpufreq_volt * 100) / 1150) *
	    ((mt_cpu_freqs[index].cpufreq_volt * 100) / 1150) *
	    ((mt_cpu_freqs[index].cpufreq_volt * 100) / 1150);

	switch (ncpu) {
	case 0:
		/* 1 core */
		p_dynamic = (((A_1_CORE * freq_ratio) / 1000) * volt_square_ratio) / 100;
		p_leakage = ((P_TOTAL_CORE_L * (multi)) / (100 * 100 * 100)) - 3 * P_EACH_CORE_L;
		cpufreq_dbg("p_dynamic = %d, p_leakage = %d\n", p_dynamic, p_leakage);
		break;
	case 1:
		/* 2 core */
		p_dynamic = (((A_2_CORE * freq_ratio) / 1000) * volt_square_ratio) / 100;
		p_leakage = ((P_TOTAL_CORE_L * (multi)) / (100 * 100 * 100)) - 2 * P_EACH_CORE_L;
		cpufreq_dbg("p_dynamic = %d, p_leakage = %d\n", p_dynamic, p_leakage);
		break;
	case 2:
		/* 3 core */
		p_dynamic = (((A_3_CORE * freq_ratio) / 1000) * volt_square_ratio) / 100;
		p_leakage = ((P_TOTAL_CORE_L * (multi)) / (100 * 100 * 100)) - 1 * P_EACH_CORE_L;
		cpufreq_dbg("p_dynamic = %d, p_leakage = %d\n", p_dynamic, p_leakage);
		break;
	case 3:
		/* 4 core */
		p_dynamic = (((A_4_CORE * freq_ratio) / 1000) * volt_square_ratio) / 100;
		p_leakage = (P_TOTAL_CORE_L * (multi)) / (100 * 100 * 100);
		cpufreq_dbg("p_dynamic = %d, p_leakage = %d\n", p_dynamic, p_leakage);
		break;
	default:
		break;
	}

	mt_cpu_power[index * possiblecpu + ncpu].cpufreq_ncpu = ncpu + 1;
	mt_cpu_power[index * possiblecpu + ncpu].cpufreq_khz = mt_cpu_freqs[index].cpufreq_khz;
	mt_cpu_power[index * possiblecpu + ncpu].cpufreq_power = p_dynamic + p_leakage;

	cpufreq_dbg("mt_cpu_power[%d]: cpufreq_ncpu = %d, cpufreq_khz = %d, cpufreq_power = %d\n",
		    (index * possiblecpu + ncpu),
		    mt_cpu_power[index * possiblecpu + ncpu].cpufreq_ncpu,
		    mt_cpu_power[index * possiblecpu + ncpu].cpufreq_khz,
		    mt_cpu_power[index * possiblecpu + ncpu].cpufreq_power);

}

static void mt_setup_power_table(int num)
{
	int i = 0, j = 0, ncpu = 0, count = 0;
	struct mtk_cpu_power_info temp_power_info;
	int possiblecpu = 0;
	unsigned int mt_cpufreq_power_efficiency[20][10];

	possiblecpu = num_possible_cpus();

	memset((void *)mt_cpufreq_power_efficiency, 0, sizeof(unsigned int) * 20 * 10);

	mt_cpu_power = kzalloc((num * possiblecpu) * sizeof(struct mtk_cpu_power_info), GFP_KERNEL);

	/* Init power table to 0 */
	for (i = 0; i < num; i++) {
		for (j = 0; j < possiblecpu; j++) {
			mt_cpu_power[i * possiblecpu + j].cpufreq_ncpu = 0;
			mt_cpu_power[i * possiblecpu + j].cpufreq_khz = 0;
			mt_cpu_power[i * possiblecpu + j].cpufreq_power = 0;
		}
	}

	/* Setup power efficiency array */
	for (i = 0; i < num; i++) {
		for (j = 0; j < possiblecpu; j++) {
			ncpu = j + 1;

			if (ncpu == possiblecpu)
				continue;	/* Keep power table of max cores. */

			if (((mt_cpu_freqs_table[i].frequency == DVFS_F0) && (ncpu == 3))
			    || ((mt_cpu_freqs_table[i].frequency == DVFS_F0) && (ncpu == 2))) {
				mt_cpufreq_power_efficiency[i][j] = 1;
				count++;
			}
		}
	}

	g_cpu_power_table_num = num * possiblecpu - count;	/* Need to check, if condition core num change. */

	/* Calculate power and fill in power table */
	for (i = 0; i < num; i++) {
		for (j = 0; j < possiblecpu; j++) {
			if (mt_cpufreq_power_efficiency[i][j] == 0)
				mt_cpufreq_power_calculation(i, j);
		}
	}

	/* Sort power table */
	for (i = (num * possiblecpu - 1); i > 0; i--) {
		for (j = 1; j <= i; j++) {
			if (mt_cpu_power[j - 1].cpufreq_power < mt_cpu_power[j].cpufreq_power) {
				temp_power_info.cpufreq_khz = mt_cpu_power[j - 1].cpufreq_khz;
				temp_power_info.cpufreq_ncpu = mt_cpu_power[j - 1].cpufreq_ncpu;
				temp_power_info.cpufreq_power = mt_cpu_power[j - 1].cpufreq_power;

				mt_cpu_power[j - 1].cpufreq_khz = mt_cpu_power[j].cpufreq_khz;
				mt_cpu_power[j - 1].cpufreq_ncpu = mt_cpu_power[j].cpufreq_ncpu;
				mt_cpu_power[j - 1].cpufreq_power = mt_cpu_power[j].cpufreq_power;

				mt_cpu_power[j].cpufreq_khz = temp_power_info.cpufreq_khz;
				mt_cpu_power[j].cpufreq_ncpu = temp_power_info.cpufreq_ncpu;
				mt_cpu_power[j].cpufreq_power = temp_power_info.cpufreq_power;
			}
		}
	}

	for (i = 0; i < (num * possiblecpu); i++) {
		cpufreq_dbg("mt_cpu_power[%d].cpufreq_khz = %d, ", i, mt_cpu_power[i].cpufreq_khz);
		cpufreq_dbg("mt_cpu_power[%d].cpufreq_ncpu = %d, ", i, mt_cpu_power[i].cpufreq_ncpu);
		cpufreq_dbg("mt_cpu_power[%d].cpufreq_power = %d\n", i, mt_cpu_power[i].cpufreq_power);
	}
}

void mt_cpufreq_thermal_protect(unsigned int limited_power)
{
	int i = 0, ncpu = 0, found = 0;

	struct cpufreq_policy *policy;

	cpufreq_dbg("mt_cpufreq_thermal_protect, limited_power:%d\n", limited_power);

	g_thermal_protect_limited_power = limited_power;

	policy = cpufreq_cpu_get(0);

	if (!policy)
		goto no_policy;

	ncpu = num_possible_cpus();

	if (limited_power == 0) {
		g_limited_max_ncpu = num_possible_cpus();
		g_limited_max_freq = g_max_freq_by_ptp;

		cpufreq_driver_target(policy, g_limited_max_freq, CPUFREQ_RELATION_L);
		hps_set_cpu_num_limit(LIMIT_THERMAL, g_limited_max_ncpu, 0);

		cpufreq_dbg("thermal limit g_limited_max_freq = %d, g_limited_max_ncpu = %d\n",
			    g_limited_max_freq, g_limited_max_ncpu);
	} else {
		while (ncpu) {
			for (i = 0; i < g_cpu_power_table_num; i++) {
				if (mt_cpu_power[i].cpufreq_ncpu == ncpu) {
					if (mt_cpu_power[i].cpufreq_power <= limited_power) {
						g_limited_max_ncpu = mt_cpu_power[i].cpufreq_ncpu;
						g_limited_max_freq = mt_cpu_power[i].cpufreq_khz;

						found = 1;
						break;
					}
				}
			}

			if (found)
				break;

			ncpu--;
		}

		if (!found) {
			cpufreq_dbg("Not found suitable DVFS OPP, limit to lowest OPP!\n");
			g_limited_max_ncpu = mt_cpu_power[g_cpu_power_table_num - 1].cpufreq_ncpu;
			g_limited_max_freq = mt_cpu_power[g_cpu_power_table_num - 1].cpufreq_khz;
		}

		cpufreq_dbg("thermal limit g_limited_max_freq = %d, g_limited_max_ncpu = %d\n",
			    g_limited_max_freq, g_limited_max_ncpu);

		hps_set_cpu_num_limit(LIMIT_THERMAL, g_limited_max_ncpu, 0);

		cpufreq_driver_target(policy, g_limited_max_freq, CPUFREQ_RELATION_L);
	}

	cpufreq_cpu_put(policy);

no_policy:
	return;
}

static unsigned int mt_thermal_limited_verify(unsigned int target_freq)
{
	int i = 0, index = 0;

	if (g_thermal_protect_limited_power == 0)
		return target_freq;

	for (i = 0; i < g_cpu_power_table_num; i++) {
		if (mt_cpu_power[i].cpufreq_ncpu == g_limited_max_ncpu
		    && mt_cpu_power[i].cpufreq_khz == g_limited_max_freq) {
			index = i;
			break;
		}
	}

	for (index = i; index < g_cpu_power_table_num; index++) {
		if (mt_cpu_power[index].cpufreq_ncpu == num_online_cpus()) {
			if (target_freq >= mt_cpu_power[index].cpufreq_khz) {
				cpufreq_dbg("target_freq = %d, ncpu = %d\n",
					     mt_cpu_power[index].cpufreq_khz, num_online_cpus());
				target_freq = mt_cpu_power[index].cpufreq_khz;
				break;
			}
		}
	}

	return target_freq;
}

/*****************************
* cpufreq_driver function pointer
* mt_cpufreq_target()
******************************/
static void mt_cpufreq_volt_set(unsigned int target_volt)
{
#ifdef CONFIG_OF
#if ENABLE_DVS
	int ret = 0;

	ret = regulator_set_voltage(reg_vproc, target_volt * 1000,
		target_volt * 1000 + 10000 - 1);
	if (ret) {
		cpufreq_err("regulator_set_voltage(%u) return %d\n", target_volt, ret);
		return;
	}
#endif
#else
	if ((g_cpufreq_get_ptp_level >= 0) && (g_cpufreq_get_ptp_level <= 4)) {
		switch (target_volt) {
		case DVFS_V0:
			cpufreq_dbg("switch to DVS0: %d mV\n", DVFS_V0);
			spm_dvfs_ctrl_volt(0);
			break;
		case DVFS_V1:
			cpufreq_dbg("switch to DVS1: %d mV\n", DVFS_V1);
			spm_dvfs_ctrl_volt(1);
			break;
		case DVFS_V2:
			cpufreq_dbg("switch to DVS2: %d mV\n", DVFS_V2);

			spm_dvfs_ctrl_volt(2);
			break;
		default:
			break;
		}
	} else if ((g_cpufreq_get_ptp_level >= 5) && (g_cpufreq_get_ptp_level <= 6)) {
		switch (target_volt) {
		case DVFS_V1:
			cpufreq_dbg("switch to DVS0: %d mV\n", DVFS_V1);
			spm_dvfs_ctrl_volt(0);
			break;
		case DVFS_V2:
			cpufreq_dbg("switch to DVS2: %d mV\n", DVFS_V2);

			spm_dvfs_ctrl_volt(1);
			break;
		default:
			break;

		}
	} else {
		switch (target_volt) {
		case DVFS_V0:
			cpufreq_dbg("switch to DVS0: %d mV\n", DVFS_V0);
			spm_dvfs_ctrl_volt(0);
			break;
		case DVFS_V1:
			cpufreq_dbg("switch to DVS1: %d mV\n", DVFS_V1);
			spm_dvfs_ctrl_volt(1);
			break;
		case DVFS_V2:
			cpufreq_dbg("switch to DVS2: %d mV\n", DVFS_V2);

			spm_dvfs_ctrl_volt(2);
			break;
		default:
			break;
		}
	}
#endif
}

static int mt_cpufreq_keep_max_freq(unsigned int freq_old, unsigned int freq_new)
{
	if (mt_cpufreq_pause)
		return 1;

	/* check if system is going to ramp down */
	if (freq_new < freq_old)
		g_ramp_down_count++;
	else
		g_ramp_down_count = 0;

	if (g_ramp_down_count < RAMP_DOWN_TIMES)
		return 1;

	return 0;
}

static void mt_cpu_clock_switch(unsigned int sel)
{
	unsigned int ckmuxsel = 0;

	ckmuxsel = cpufreq_read(TOP_CKMUXSEL) & ~0xC;

	switch (sel) {
	case TOP_CKMUXSEL_CLKSQ:
		mt_cpufreq_reg_write((ckmuxsel | 0x00), TOP_CKMUXSEL);
		break;
	case TOP_CKMUXSEL_ARMPLL:
		mt_cpufreq_reg_write((ckmuxsel | 0x04), TOP_CKMUXSEL);
		break;
	case TOP_CKMUXSEL_MAINPLL:
		mt_cpufreq_reg_write((ckmuxsel | 0x08), TOP_CKMUXSEL);
		break;
	case TOP_CKMUXSEL_UNIVPLL:
		mt_cpufreq_reg_write((ckmuxsel | 0x0C), TOP_CKMUXSEL);
		break;
	default:
		break;
	}
}

unsigned int mt_get_cpu_freq(void)
{
	int output = 0;
	unsigned int temp, clk26cali_0, clk_cfg_8, clk_misc_cfg_1;

	/* Use CCF to replace this function */
	return 0;

	clk26cali_0 = cpufreq_read(CLK26CALI_0);
	mt_cpufreq_reg_write(clk26cali_0 | 0x80, CLK26CALI_0);	/* enable fmeter_en */

	clk_misc_cfg_1 = cpufreq_read(CLK_MISC_CFG_1);
	mt_cpufreq_reg_write(0xFFFF0300, CLK_MISC_CFG_1);	/* select divider */

	clk_cfg_8 = cpufreq_read(CLK_CFG_8);
	mt_cpufreq_reg_write((39 << 8), CLK_CFG_8);	/* select abist_cksw */

	temp = cpufreq_read(CLK26CALI_0);
	mt_cpufreq_reg_write(temp | 0x1, CLK26CALI_0);	/* start fmeter */

	/* wait frequency meter finish */
	while (cpufreq_read(CLK26CALI_0) & 0x1) {
		cpufreq_dbg("wait for cpu frequency meter finish, CLK26CALI = 0x%x\n",
		       cpufreq_read(CLK26CALI_0));
		mdelay(10);
	}

	temp = cpufreq_read(CLK26CALI_1) & 0xFFFF;

	output = ((temp * 26000) / 1024) * 4;	/* Khz */

	mt_cpufreq_reg_write(clk_cfg_8, CLK_CFG_8);
	mt_cpufreq_reg_write(clk_misc_cfg_1, CLK_MISC_CFG_1);
	mt_cpufreq_reg_write(clk26cali_0, CLK26CALI_0);

	/* printk("CLK26CALI = 0x%x, cpu frequency = %d Khz\n", temp, output); */

	return output;
}

static void mt_cpufreq_set(unsigned int freq_old, unsigned int freq_new, unsigned int target_volt)
{
	unsigned int armpll = 0;

	if (freq_new >= 1001000) {
		armpll = 0x8009A000;
		armpll = armpll + ((freq_new - 1001000) / 13000) * 0x2000;
	} else if (freq_new >= 500500) {
		armpll = 0x8109A000;
		armpll = armpll + ((freq_new - 500500) / 6500) * 0x2000;
	} else if (freq_new >= 250250) {
		armpll = 0x8209A000;
		armpll = armpll + ((freq_new - 250250) / 3250) * 0x2000;
	} else if (freq_new >= 125125) {
		armpll = 0x8309A000;
		armpll = armpll + ((freq_new - 125125) / 1625) * 0x2000;
	} else {
		armpll = 0x8409A000;
		armpll = armpll + ((freq_new - 62562) / 812) * 0x2000;
	}

	/* YP comment no need to call enable/disable pll. mainpll will always on until suspend. */
	/* enable_pll(MAINPLL, "CPU_DVFS"); */
	if (freq_new > freq_old) {
		#ifdef MT_BUCK_ADJUST
		mt_cpufreq_volt_set(target_volt);
		udelay(PMIC_SETTLE_TIME);
		#endif

		#ifdef MT_CPUFREQ_FHCTL
		if (((freq_new > FHCTL_CHANGE_FREQ) && (freq_old > FHCTL_CHANGE_FREQ))
		    || ((freq_new < FHCTL_CHANGE_FREQ) && (freq_old < FHCTL_CHANGE_FREQ))) {
			mt_dfs_armpll(freq_old, freq_new);
			cpufreq_dbg("=== FHCTL: freq_new = %d > freq_old = %d ===\n", freq_new,
				     freq_old);

			cpufreq_dbg("=== FHCTL: freq meter = %d ===\n", mt_get_cpu_freq());
		} else {
		#endif
			mt_cpufreq_reg_write(0x0A, TOP_CKDIV1);
			mt_cpu_clock_switch(TOP_CKMUXSEL_MAINPLL);

			mt_cpufreq_reg_write(armpll, ARMPLL_CON1);

			mb();	/* PLL_SETTLE_TIME */
			udelay(PLL_SETTLE_TIME);

			mt_cpu_clock_switch(TOP_CKMUXSEL_ARMPLL);
			mt_cpufreq_reg_write(0x00, TOP_CKDIV1);

		#ifdef MT_CPUFREQ_FHCTL
		}
		#endif
	} else {
		#ifdef MT_CPUFREQ_FHCTL
		if (((freq_new > FHCTL_CHANGE_FREQ) && (freq_old > FHCTL_CHANGE_FREQ))
		    || ((freq_new < FHCTL_CHANGE_FREQ) && (freq_old < FHCTL_CHANGE_FREQ))) {
			mt_dfs_armpll(freq_old, freq_new);
			cpufreq_dbg("=== FHCTL: freq_new = %d < freq_old = %d ===\n", freq_new,
				     freq_old);

			cpufreq_dbg("=== FHCTL: freq meter = %d ===\n", mt_get_cpu_freq());
		} else {
		#endif

			mt_cpufreq_reg_write(0x0A, TOP_CKDIV1);
			mt_cpu_clock_switch(TOP_CKMUXSEL_MAINPLL);

			mt_cpufreq_reg_write(armpll, ARMPLL_CON1);

			mb();	/* PLL_SETTLE_TIME */
			udelay(PLL_SETTLE_TIME);

			mt_cpu_clock_switch(TOP_CKMUXSEL_ARMPLL);
			mt_cpufreq_reg_write(0x00, TOP_CKDIV1);

		#ifdef MT_CPUFREQ_FHCTL
		}
		#endif

		#ifdef MT_BUCK_ADJUST
		mt_cpufreq_volt_set(target_volt);
		#endif
	}
	/* disable_pll(MAINPLL, "CPU_DVFS"); */

	g_cur_freq = freq_new;
	g_cur_cpufreq_volt = target_volt;

	cpufreq_dbg("ARMPLL_CON0 = 0x%x, ARMPLL_CON1 = 0x%x, g_cur_freq = %d\n",
		     cpufreq_read(ARMPLL_CON0), cpufreq_read(ARMPLL_CON1), g_cur_freq);
}

/*****************************
* cpufreq_driver function pointer
* mt_cpufreq_init()
******************************/
static int mt_setup_freqs_table(struct cpufreq_policy *policy, struct mt_cpu_freq_info *freqs,
				int num)
{
	struct cpufreq_frequency_table *table;
	int i, ret;

	if (mt_cpufreq_freq_table_allocated == false) {
		table = kzalloc((num + 1) * sizeof(*table), GFP_KERNEL);
		if (table == NULL)
			return -ENOMEM;

		for (i = 0; i < num; i++) {
			table[i].driver_data = i;
			table[i].frequency = freqs[i].cpufreq_khz;
		}
		table[num].driver_data = i;
		table[num].frequency = CPUFREQ_TABLE_END;

		mt_cpu_freqs = freqs;
		mt_cpu_freqs_num = num;
		mt_cpu_freqs_table = table;

		mt_cpufreq_freq_table_allocated = true;
	}

	ret = cpufreq_frequency_table_cpuinfo(policy, mt_cpu_freqs_table);
	if (!ret)
		policy->freq_table = mt_cpu_freqs_table;

	if (mt_cpu_power == NULL)
		mt_setup_power_table(num);

	return 0;
}

/*****************************
* cpufreq_driver function pointer
******************************/
static int mt_cpufreq_init(struct cpufreq_policy *policy)
{
	int ret = -EINVAL;

	if (policy->cpu >= num_possible_cpus())
		return -EINVAL;

	policy->shared_type = CPUFREQ_SHARED_TYPE_ANY;
	cpumask_setall(policy->cpus);

    /*******************************************************
    * 1 us, assumed, will be overwrited by min_sampling_rate
    ********************************************************/
	policy->cpuinfo.transition_latency = 1000;

    /*********************************************
    * set default policy and cpuinfo, unit : Khz
    **********************************************/
	policy->cpuinfo.max_freq = g_max_freq_by_ptp;
	policy->cpuinfo.min_freq = DVFS_F4;

	policy->cur = DVFS_F2;	/* Default 1.05GHz in preloader */
	policy->max = g_max_freq_by_ptp;
	policy->min = DVFS_F4;

	if (g_cpufreq_get_ptp_level == 0)
		ret = mt_setup_freqs_table(policy, mt8127_freqs_e1, ARRAY_SIZE(mt8127_freqs_e1));
	else if (g_cpufreq_get_ptp_level == 1)
		ret =
		    mt_setup_freqs_table(policy, mt8127_freqs_e1_1, ARRAY_SIZE(mt8127_freqs_e1_1));
	else if (g_cpufreq_get_ptp_level == 2)
		ret =
		    mt_setup_freqs_table(policy, mt8127_freqs_e1_2, ARRAY_SIZE(mt8127_freqs_e1_2));
	else if (g_cpufreq_get_ptp_level == 3)
		ret =
		    mt_setup_freqs_table(policy, mt8127_freqs_e1_3, ARRAY_SIZE(mt8127_freqs_e1_3));
	else if (g_cpufreq_get_ptp_level == 4)
		ret =
		    mt_setup_freqs_table(policy, mt8127_freqs_e1_4, ARRAY_SIZE(mt8127_freqs_e1_4));
	else if (g_cpufreq_get_ptp_level == 5)
		ret =
		    mt_setup_freqs_table(policy, mt8127_freqs_e1_5, ARRAY_SIZE(mt8127_freqs_e1_5));
	else if (g_cpufreq_get_ptp_level == 6)
		ret =
		    mt_setup_freqs_table(policy, mt8127_freqs_e1_6, ARRAY_SIZE(mt8127_freqs_e1_6));
	else
		ret = mt_setup_freqs_table(policy, mt8127_freqs_e1, ARRAY_SIZE(mt8127_freqs_e1));

	if (ret) {
		pr_err("Power/DVFS, failed to setup frequency table\n");
		return ret;
	}

	return 0;
}

static int mt_cpufreq_verify(struct cpufreq_policy *policy)
{
	cpufreq_dbg("call mt_cpufreq_verify!\n");
	return cpufreq_frequency_table_verify(policy, mt_cpu_freqs_table);
}

static int mt_cpufreq_target(struct cpufreq_policy *policy, unsigned int target_freq,
			     unsigned int relation)
{
	int i, idx;
	unsigned long flags;
	int ret = 0;

	struct mt_cpu_freq_info next;
	struct cpufreq_freqs freqs;

	if (!mt_cpufreq_ready)
		return -EINVAL;

	if (policy->cpu >= num_possible_cpus())
		return -EINVAL;

	ret = 0;

    /******************************
    * look up the target frequency
    *******************************/
	if (cpufreq_frequency_table_target(policy, mt_cpu_freqs_table, target_freq, relation, &idx))
		return -EINVAL;

	#ifdef MT_DVFS_RANDOM_TEST
	idx = mt_cpufreq_idx_get(5);
	#endif

	if (g_cpufreq_get_ptp_level == 0)
		next.cpufreq_khz = mt8127_freqs_e1[idx].cpufreq_khz;
	else if (g_cpufreq_get_ptp_level == 1)
		next.cpufreq_khz = mt8127_freqs_e1_1[idx].cpufreq_khz;
	else if (g_cpufreq_get_ptp_level == 2)
		next.cpufreq_khz = mt8127_freqs_e1_2[idx].cpufreq_khz;
	else if (g_cpufreq_get_ptp_level == 3)
		next.cpufreq_khz = mt8127_freqs_e1_3[idx].cpufreq_khz;
	else if (g_cpufreq_get_ptp_level == 4)
		next.cpufreq_khz = mt8127_freqs_e1_4[idx].cpufreq_khz;
	else if (g_cpufreq_get_ptp_level == 5)
		next.cpufreq_khz = mt8127_freqs_e1_5[idx].cpufreq_khz;
	else if (g_cpufreq_get_ptp_level == 6)
		next.cpufreq_khz = mt8127_freqs_e1_6[idx].cpufreq_khz;
	else
		next.cpufreq_khz = mt8127_freqs_e1[idx].cpufreq_khz;

	#ifdef MT_DVFS_RANDOM_TEST
	cpufreq_dbg("idx = %d, freqs.old = %d, freqs.new = %d\n", idx, policy->cur,
		     next.cpufreq_khz);
	#endif

	freqs.old = policy->cur;
	freqs.new = next.cpufreq_khz;
	freqs.cpu = policy->cpu;

	#ifndef MT_DVFS_RANDOM_TEST
	if (mt_cpufreq_keep_max_freq(freqs.old, freqs.new))
		freqs.new = policy->max;

    /************************************************
    * DVFS keep at 1.05GHz/1.15V in earlysuspend when max freq overdrive.
    *************************************************/
	#ifdef CONFIG_HAS_EARLYSUSPEND
	if (mt_cpufreq_limit_max_freq_early_suspend == true) {
		freqs.new = DVFS_F2;
		cpufreq_dbg("mt_cpufreq_limit_max_freq_early_suspend, freqs.new = %d\n",
			     freqs.new);
	}
	#endif

	freqs.new = mt_thermal_limited_verify(freqs.new);

	if (freqs.new < g_limited_min_freq) {
		cpufreq_dbg("cannot switch CPU frequency to %d Mhz due to voltage limitation\n",
			     g_limited_min_freq / 1000);
		freqs.new = g_limited_min_freq;
	}
	#endif

    /************************************************
    * DVFS keep at 1.05Ghz/1.15V when PTPOD initial
    *************************************************/
	if (mt_cpufreq_ptpod_disable) {
		freqs.new = DVFS_F2;
		cpufreq_dbg("PTPOD, freqs.new = %d\n", freqs.new);
	}

    /************************************************
    * target frequency == existing frequency, skip it
    *************************************************/
	if (freqs.old == freqs.new) {
		cpufreq_dbg
		    ("CPU frequency from %d MHz to %d MHz (skipped) due to same frequency\n",
		     freqs.old / 1000, freqs.new / 1000);
		return 0;
	}

    /**************************************
    * search for the corresponding voltage
    ***************************************/
	next.cpufreq_volt = 0;

	for (i = 0; i < mt_cpu_freqs_num; i++) {
		cpufreq_dbg("freqs.new = %d, mt_cpu_freqs[%d].cpufreq_khz = %d\n", freqs.new, i,
			     mt_cpu_freqs[i].cpufreq_khz);
		if (freqs.new == mt_cpu_freqs[i].cpufreq_khz) {
			next.cpufreq_volt = mt_cpu_freqs[i].cpufreq_volt;
			cpufreq_dbg("next.cpufreq_volt = %d, mt_cpu_freqs[%d].cpufreq_volt = %d\n",
				     next.cpufreq_volt, i, mt_cpu_freqs[i].cpufreq_volt);
			break;
		}
	}

	if (next.cpufreq_volt == 0) {
		cpufreq_dbg("Error!! Cannot find corresponding voltage at %d Mhz\n",
			     freqs.new / 1000);
		return 0;
	}

	/* fix notify transition hang issue for Linux-3.18 */
	if (policy) {
		freqs.cpu = policy->cpu;
		cpufreq_freq_transition_begin(policy, &freqs);
	}

	cpufreq_lock(flags);
	/* spin_lock_irqsave(&mt_cpufreq_lock, flags); */

    /******************************
    * set to the target freeuency
    *******************************/
	mt_cpufreq_set(freqs.old, freqs.new, next.cpufreq_volt);

	/* spin_unlock_irqrestore(&mt_cpufreq_lock, flags); */
	cpufreq_unlock(flags);

	/* fix notify transition hang issue for Linux-3.18 */
	if (policy)
		cpufreq_freq_transition_end(policy, &freqs, 0);

	return 0;
}

static unsigned int mt_cpufreq_get(unsigned int cpu)
{
	cpufreq_dbg("call mt_cpufreq_get: %d!\n", g_cur_freq);
	return g_cur_freq;
}

static struct freq_attr *mt_cpufreq_attr[] = {
	&cpufreq_freq_attr_scaling_available_freqs,
	NULL,
};


static struct cpufreq_driver mt_cpufreq_driver = {
	.flags = CPUFREQ_ASYNC_NOTIFICATION,
	.init = mt_cpufreq_init,
	.verify = mt_cpufreq_verify,
	.target = mt_cpufreq_target,
	.get = mt_cpufreq_get,
	.name = "mt-cpufreq",
	.attr = mt_cpufreq_attr,
};

static const struct of_device_id mtcpufreq_of_match[] = {
	{.compatible = "mediatek,mt2701-cpufreq",},
	{},
};

MODULE_DEVICE_TABLE(of, mtcpufreq_of_match);

void __iomem *mtcpufreq_base;

/*******************************************
* cpufrqe platform driver callback function
********************************************/
static int mt_cpufreq_pdrv_probe(struct platform_device *pdev)

{
	int ret;

#ifdef CONFIG_OF
#if ENABLE_DVS
	reg_vproc = devm_regulator_get(&pdev->dev, "reg-vproc");
	if (IS_ERR(reg_vproc)) {
		ret = PTR_ERR(reg_vproc);
		dev_err(&pdev->dev, "Failed to request reg-vproc: %d\n", ret);
		return ret;
	}
	ret = regulator_enable(reg_vproc);
#else
	ret = 0;
#endif
#endif

    /************************************************
    * Check PTP level to define default max freq
    *************************************************/
	g_cpufreq_get_ptp_level = PTP_get_ptp_level();

	if (g_cpufreq_get_ptp_level == 0)
		g_max_freq_by_ptp = DVFS_F0;
	else if (g_cpufreq_get_ptp_level == 1)
		g_max_freq_by_ptp = DVFS_F0_1;
	else if (g_cpufreq_get_ptp_level == 2)
		g_max_freq_by_ptp = DVFS_F0_2;
	else if (g_cpufreq_get_ptp_level == 3)
		g_max_freq_by_ptp = DVFS_F0_3;
	else if (g_cpufreq_get_ptp_level == 4)
		g_max_freq_by_ptp = DVFS_F0_4;
	else if (g_cpufreq_get_ptp_level == 5)
		g_max_freq_by_ptp = DVFS_F1_0;
	else if (g_cpufreq_get_ptp_level == 6)
		g_max_freq_by_ptp = DVFS_F1_1;
	else
		g_max_freq_by_ptp = DVFS_F0;

    /************************************************
    * voltage scaling need to wait PMIC driver ready
    *************************************************/
	mt_cpufreq_ready = true;

	g_cur_freq = DVFS_F2;	/* Default 1.05GHz in preloader */
	g_cur_cpufreq_volt = DVFS_V2;	/* Default 1.15V */
	g_limited_max_freq = g_max_freq_by_ptp;
	g_limited_min_freq = DVFS_F4;

	cpufreq_dbg("Power/DVFS, mediatek cpufreq initialized\n");

	/* mt6323 */
	mt_cpufreq_reg_write(0x0220, PMIC_WRAP_DVFS_ADR0);
	mt_cpufreq_reg_write(0x0220, PMIC_WRAP_DVFS_ADR1);
	mt_cpufreq_reg_write(0x0220, PMIC_WRAP_DVFS_ADR2);
	mt_cpufreq_reg_write(0x0220, PMIC_WRAP_DVFS_ADR3);
	mt_cpufreq_reg_write(0x0220, PMIC_WRAP_DVFS_ADR4);
	mt_cpufreq_reg_write(0x0220, PMIC_WRAP_DVFS_ADR5);
	mt_cpufreq_reg_write(0x0220, PMIC_WRAP_DVFS_ADR6);
	mt_cpufreq_reg_write(0x0220, PMIC_WRAP_DVFS_ADR7);


	if ((g_cpufreq_get_ptp_level >= 0) && (g_cpufreq_get_ptp_level <= 4)) {
		mt_cpufreq_reg_write(0x60, PMIC_WRAP_DVFS_WDATA0);	/* 1.30V VPROC */
		mt_cpufreq_reg_write(0x50, PMIC_WRAP_DVFS_WDATA1);	/* 1.20V VPROC */
		mt_cpufreq_reg_write(0x48, PMIC_WRAP_DVFS_WDATA2);	/* 1.15V VPROC */
		mt_cpufreq_reg_write(0x48, PMIC_WRAP_DVFS_WDATA3);	/* 1.15V VPROC */

		/* For PTP-OD */
		mt_cpufreq_pmic_volt[0] = 0x60;	/* 1.30V VPROC */
		mt_cpufreq_pmic_volt[1] = 0x50;	/* 1.20V VPROC */
		mt_cpufreq_pmic_volt[2] = 0x48;	/* 1.15V VPROC */
		mt_cpufreq_pmic_volt[3] = 0x48;	/* 1.15V VPROC */


	} else if ((g_cpufreq_get_ptp_level >= 5) && (g_cpufreq_get_ptp_level <= 6)) {
		mt_cpufreq_reg_write(0x50, PMIC_WRAP_DVFS_WDATA0);	/* 1.20V VPROC */
		mt_cpufreq_reg_write(0x48, PMIC_WRAP_DVFS_WDATA1);	/* 1.15V VPROC */
		mt_cpufreq_reg_write(0x48, PMIC_WRAP_DVFS_WDATA2);	/* 1.15V VPROC */
		mt_cpufreq_reg_write(0x48, PMIC_WRAP_DVFS_WDATA3);	/* 1.15V VPROC */

		/* For PTP-OD */
		mt_cpufreq_pmic_volt[0] = 0x50;	/* 1.20V VPROC */
		mt_cpufreq_pmic_volt[1] = 0x48;	/* 1.15V VPROC */
		mt_cpufreq_pmic_volt[2] = 0x48;	/* 1.15V VPROC */
		mt_cpufreq_pmic_volt[3] = 0x48;	/* 1.15V VPROC */

	} else {

		mt_cpufreq_reg_write(0x60, PMIC_WRAP_DVFS_WDATA0);	/* 1.30V VPROC */
		mt_cpufreq_reg_write(0x50, PMIC_WRAP_DVFS_WDATA1);	/* 1.20V VPROC */
		mt_cpufreq_reg_write(0x48, PMIC_WRAP_DVFS_WDATA2);	/* 1.15V VPROC */
		mt_cpufreq_reg_write(0x48, PMIC_WRAP_DVFS_WDATA3);	/* 1.15V VPROC */

		/* For PTP-OD */
		mt_cpufreq_pmic_volt[0] = 0x60;	/* 1.30V VPROC */
		mt_cpufreq_pmic_volt[1] = 0x50;	/* 1.20V VPROC */
		mt_cpufreq_pmic_volt[2] = 0x48;	/* 1.15V VPROC */
		mt_cpufreq_pmic_volt[3] = 0x48;	/* 1.15V VPROC */

	}

	/* DVFS table(4)-(7) for SPM control in deep idle */
	mt_cpufreq_reg_write(0x38, PMIC_WRAP_DVFS_WDATA4);	/* 1.05V VPROC, for spm control in deep idle */
	mt_cpufreq_reg_write(0x48, PMIC_WRAP_DVFS_WDATA5);	/* 1.15V VPROC, for spm control in deep idle */
	mt_cpufreq_reg_write(0x38, PMIC_WRAP_DVFS_WDATA6);	/* 1.05V VPROC, for spm control in deep idle */
	mt_cpufreq_reg_write(0x48, PMIC_WRAP_DVFS_WDATA7);	/* 1.15V VPROC, for spm control in deep idle */

	/* For PTP-OD */
	mt_cpufreq_pmic_volt[4] = 0x38;	/* 1.05V VPROC, for spm control in deep idle */
	mt_cpufreq_pmic_volt[5] = 0x48;	/* 1.15V VPROC, for spm control in deep idle */
	mt_cpufreq_pmic_volt[6] = 0x38;	/* 1.05V VPROC, for spm control in deep idle */
	mt_cpufreq_pmic_volt[7] = 0x48;	/* 1.15V VPROC, for spm control in deep idle */



#ifndef CONFIG_OF
	if ((g_cpufreq_get_ptp_level >= 0) && (g_cpufreq_get_ptp_level <= 4))
		spm_dvfs_ctrl_volt(2);	/* default set to 1.15V */
	else if ((g_cpufreq_get_ptp_level >= 5) && (g_cpufreq_get_ptp_level <= 6))
		spm_dvfs_ctrl_volt(1);	/* default set to 1.15V */
	else
		spm_dvfs_ctrl_volt(2);	/* default set to 1.15V */


#endif
	ret = cpufreq_register_driver(&mt_cpufreq_driver);

	return ret;
}


/***************************************
* this function should never be called
****************************************/
static int mt_cpufreq_pdrv_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver mt_cpufreq_pdrv = {
	.probe = mt_cpufreq_pdrv_probe,
	.remove = mt_cpufreq_pdrv_remove,
	.suspend = NULL,
	.resume = NULL,
	.driver = {
		.name = "mt-cpufreq",
		.of_match_table = of_match_ptr(mtcpufreq_of_match),
		.owner = THIS_MODULE,
	},
};

/*****************************
* set CPU DVFS status
******************************/
int mt_cpufreq_state_set(int enabled)
{
	if (enabled) {
		if (!mt_cpufreq_pause) {
			cpufreq_dbg("Power/DVFS, cpufreq already enabled\n");
			return 0;
		}

		/*************
		* enable DVFS
		**************/
		g_dvfs_disable_count--;
		cpufreq_dbg("enable DVFS: g_dvfs_disable_count = %d\n", g_dvfs_disable_count);

		/***********************************************
		* enable DVFS if no any module still disable it
		************************************************/
		if (g_dvfs_disable_count <= 0)
			mt_cpufreq_pause = false;
		else
			cpufreq_dbg("someone still disable cpufreq, cannot enable it\n");
	} else {
		/**************
		* disable DVFS
		***************/
		g_dvfs_disable_count++;
		cpufreq_dbg("disable DVFS: g_dvfs_disable_count = %d\n", g_dvfs_disable_count);

		if (mt_cpufreq_pause) {
			cpufreq_dbg("Power/DVFS, cpufreq already disabled\n");
			return 0;
		}

		mt_cpufreq_pause = true;
	}

	return 0;
}
EXPORT_SYMBOL(mt_cpufreq_state_set);


/* file nodes under /proc/cpufreq/ */
static int mt_cpufreq_debug_show(struct seq_file *s, void *v)
{
	if (mt_cpufreq_debug)
		seq_puts(s, "cpufreq debug enabled\n");
	else
		seq_puts(s, "cpufreq debug disabled\n");

	return 0;
}

static int mt_cpufreq_debug_open(struct inode *inode, struct file *file)
{
	return single_open(file, mt_cpufreq_debug_show, NULL);
}

static ssize_t mt_cpufreq_debug_write(struct file *file, const char *buffer, size_t count,
				      loff_t *data)
{
	int debug = 0;

	char desc[31];
	int len = 0;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;

	desc[len] = '\0';

	if (!kstrtoint(desc, 10, &debug)) {
		if (debug == 0) {
			mt_cpufreq_debug = 0;
			return count;
		} else if (debug == 1) {
			mt_cpufreq_debug = 1;
			return count;
		}
		cpufreq_dbg("bad argument!! should be 0 or 1 [0: disable, 1: enable]\n");
	} else {
		cpufreq_dbg("bad argument!! should be 0 or 1 [0: disable, 1: enable]\n");
	}

	return -EINVAL;
}

static int mt_cpufreq_limited_power_show(struct seq_file *s, void *v)
{
	seq_printf(s, "g_limited_max_freq = %d, g_limited_max_ncpu = %d\n", g_limited_max_freq,
		   g_limited_max_ncpu);

	return 0;
}

static int mt_cpufreq_limited_power_open(struct inode *inode, struct file *file)
{
	return single_open(file, mt_cpufreq_limited_power_show, NULL);
}

static ssize_t mt_cpufreq_limited_power_write(struct file *file, const char *buffer, size_t count,
					      loff_t *data)
{
	unsigned int power = 0;

	char desc[31];
	int len = 0;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;

	desc[len] = '\0';

	if (!kstrtouint(desc, 10, &power)) {
		mt_cpufreq_thermal_protect(power);
		return count;
	}
	cpufreq_dbg("bad argument!! please provide the maximum limited power\n");

	return -EINVAL;
}

static int mt_cpufreq_state_show(struct seq_file *s, void *v)
{
	if (!mt_cpufreq_pause)
		seq_puts(s, "DVFS enabled\n");
	else
		seq_puts(s, "DVFS disabled\n");

	return 0;
}

static int mt_cpufreq_state_open(struct inode *inode, struct file *file)
{
	return single_open(file, mt_cpufreq_state_show, NULL);
}

static ssize_t mt_cpufreq_state_write(struct file *file, const char *buffer, size_t count,
				      loff_t *data)
{
	int enabled = 0;

	char desc[31];
	int len = 0;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;

	desc[len] = '\0';

	if (!kstrtoint(desc, 10, &enabled)) {
		if (enabled == 1)
			mt_cpufreq_state_set(1);
		else if (enabled == 0)
			mt_cpufreq_state_set(0);
		else
			cpufreq_dbg("bad argument!! argument should be \"1\" or \"0\"\n");
	} else
		cpufreq_dbg("bad argument!! argument should be \"1\" or \"0\"\n");

	return count;
}

static int mt_cpufreq_power_dump_show(struct seq_file *s, void *v)
{
	int i;

	for (i = 0; i < g_cpu_power_table_num; i++) {
		seq_printf(s, "mt_cpu_power[%d].cpufreq_khz = %d\n", i,
			   mt_cpu_power[i].cpufreq_khz);
		seq_printf(s, "mt_cpu_power[%d].cpufreq_ncpu = %d\n", i,
			   mt_cpu_power[i].cpufreq_ncpu);
		seq_printf(s, "mt_cpu_power[%d].cpufreq_power = %d\n", i,
			   mt_cpu_power[i].cpufreq_power);
	}

	seq_puts(s, "done\n");

	return 0;
}

static int mt_cpufreq_power_dump_open(struct inode *inode, struct file *file)
{
	return single_open(file, mt_cpufreq_power_dump_show, NULL);
}

static int mt_cpufreq_ptpod_freq_volt_show(struct seq_file *s, void *v)
{
	int i = 0;

	for (i = 0; i < mt_cpu_freqs_num; i++) {
		seq_printf(s, "default freq = %d, volt = %d\n", mt_cpu_freqs[i].cpufreq_khz,
			   mt_cpu_freqs[i].cpufreq_volt);
	}
	return 0;
}

static int mt_cpufreq_ptpod_freq_volt_open(struct inode *inode, struct file *file)
{
	return single_open(file, mt_cpufreq_ptpod_freq_volt_show, NULL);
}

static const struct file_operations mt_cpufreq_debug_fops = {
	.owner = THIS_MODULE,
	.write = mt_cpufreq_debug_write,
	.open = mt_cpufreq_debug_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static const struct file_operations mt_cpufreq_limited_power_fops = {
	.owner = THIS_MODULE,
	.write = mt_cpufreq_limited_power_write,
	.open = mt_cpufreq_limited_power_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static const struct file_operations mt_cpufreq_state_fops = {
	.owner = THIS_MODULE,
	.write = mt_cpufreq_state_write,
	.open = mt_cpufreq_state_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static const struct file_operations mt_cpufreq_power_dump_fops = {
	.owner = THIS_MODULE,
	.open = mt_cpufreq_power_dump_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static const struct file_operations mt_cpufreq_ptpod_freq_volt_fops = {
	.owner = THIS_MODULE,
	.open = mt_cpufreq_ptpod_freq_volt_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int hotplug_cpu_check_callback(struct notifier_block *nfb,
				      unsigned long action, void *hcpu)
{
	int cpu = (int)hcpu;

	switch (action & (~CPU_TASKS_FROZEN)) {
	case CPU_DOWN_PREPARE:
		if (cpu == 0) {
			pr_err_ratelimited("CPU0 hotplug is not supported\n");
			return NOTIFY_BAD;
		}
		break;
	default:
		break;
	}

	return NOTIFY_OK;
}

static struct notifier_block hotplug_cpu_check_notifier = {
	.notifier_call = hotplug_cpu_check_callback,
	.priority = INT_MAX,
};

/***********************************************************
* cpufreq initialization to register cpufreq platform driver
************************************************************/
static int __init mt_cpufreq_pdrv_init(void)
{
	int	i;
	int ret = 0;

	struct device_node *node = NULL;
	struct deviceNodeName {
		void __iomem **dts_node;
		const char *name;
	} deviceNodeNames[] = {
		{ &infracfg_ao_base, "mediatek,mt2701-infracfg"},
		{ &apmixed_base, "mediatek,mt2701-apmixedsys"},
		{ &clk_cksys_base, "mediatek,mt2701-topckgen"},
		{ &pwrap_base, "mediatek,mt2701-pwrap"},
	};

	struct proc_dir_entry *mt_cpufreq_dir = NULL;
	struct proc_dir_entry *mt_entry = NULL;

	g_ramp_down_count = 0;
	mt_cpufreq_debug = false;	/* /proc/cpufreq/cpufreq_debug */
	mt_cpufreq_ready = false;
	mt_cpufreq_pause = false;	/* /proc/cpufreq/cpufreq_state */
	mt_cpufreq_ptpod_disable = false;
	g_dvfs_disable_count = 0;
	g_cpufreq_get_ptp_level = 0;
	g_thermal_protect_limited_power = 0;
	g_cpu_power_table_num = 0;
	mt_cpu_power = NULL;
	mt_cpufreq_freq_table_allocated = false;

	for (i = 0; i < ARRAY_SIZE(deviceNodeNames); i++) {
		node = of_find_compatible_node(NULL, NULL, deviceNodeNames[i].name);
		if (node) {
			*(deviceNodeNames[i].dts_node) = of_iomap(node, 0);
			if (!(*(deviceNodeNames[i].dts_node))) {
				cpufreq_err("%s[%d], %s = 0x%lx\n", __func__, __LINE__,
					deviceNodeNames[i].name, (unsigned long)(*(deviceNodeNames[i].dts_node)));
				return 0;
			}
			cpufreq_info("%s[%d], %s = 0x%lx\n", __func__, __LINE__,
				deviceNodeNames[i].name, (unsigned long)(*(deviceNodeNames[i].dts_node)));
		} else {
			cpufreq_err("%s[%d], %s NOT FOUND\n", __func__, __LINE__, deviceNodeNames[i].name);
			return 0;
		}
	}

	/* file nodes under /proc/cpufreq/ */
	mt_cpufreq_dir = proc_mkdir("cpufreq", NULL);
	if (!mt_cpufreq_dir) {
		pr_err("[%s]: mkdir /proc/cpufreq failed\n", __func__);
	} else {
		mt_entry = proc_create("cpufreq_debug", S_IRUGO | S_IWUSR | S_IWGRP, mt_cpufreq_dir,
			&mt_cpufreq_debug_fops);
		if (!mt_entry)
			pr_err("[%s]: mkdir /proc/cpufreq/cpufreq_debug failed\n", __func__);

		mt_entry = proc_create("cpufreq_limited_power", S_IRUGO | S_IWUSR | S_IWGRP,
			mt_cpufreq_dir, &mt_cpufreq_limited_power_fops);
		if (!mt_entry)
			pr_err("[%s]: mkdir /proc/cpufreq/cpufreq_limited_power failed\n", __func__);

		mt_entry = proc_create("cpufreq_state", S_IRUGO | S_IWUSR | S_IWGRP,
			mt_cpufreq_dir, &mt_cpufreq_state_fops);
		if (!mt_entry)
			pr_err("[%s]: mkdir /proc/cpufreq/cpufreq_state failed\n", __func__);

		mt_entry = proc_create("cpufreq_power_dump", S_IRUGO | S_IWUSR | S_IWGRP,
			mt_cpufreq_dir, &mt_cpufreq_power_dump_fops);
		if (!mt_entry)
			pr_err("[%s]: mkdir /proc/cpufreq/cpufreq_power_dump failed\n", __func__);

		mt_entry = proc_create("cpufreq_ptpod_freq_volt", S_IRUGO | S_IWUSR | S_IWGRP,
				mt_cpufreq_dir, &mt_cpufreq_ptpod_freq_volt_fops);
		if (!mt_entry)
			pr_err("[%s]: mkdir /proc/cpufreq/cpufreq_ptpod_freq_volt failed\n", __func__);
	}

	ret = platform_driver_register(&mt_cpufreq_pdrv);
	if (ret) {
		pr_err("Power/DVFS, failed to register cpufreq driver\n");
		return ret;
	}
	pr_err("Power/DVFS, cpufreq driver registration done\n");
	pr_err("Power/DVFS, g_cpufreq_get_ptp_level = %d\n", g_cpufreq_get_ptp_level);

	register_hotcpu_notifier(&hotplug_cpu_check_notifier);

	return 0;
}

static void __exit mt_cpufreq_pdrv_exit(void)
{
	cpufreq_unregister_driver(&mt_cpufreq_driver);
}

late_initcall(mt_cpufreq_pdrv_init);
module_exit(mt_cpufreq_pdrv_exit);

MODULE_DESCRIPTION("MediaTek CPU Frequency Scaling driver");
MODULE_LICENSE("GPL");
