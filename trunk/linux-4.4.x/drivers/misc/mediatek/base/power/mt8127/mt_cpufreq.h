/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef _MT_CPUFREQ_H
#define _MT_CPUFREQ_H

#include <linux/module.h>

/****************************
* PMIC Wrapper DVFS Register
*****************************/
/* Regulator framework */

/* not support at mt2701 yet */
#define FULLY_VERSION_DVFS	0
/*****************
* extern function
******************/
extern int mt_cpufreq_state_set(int enabled);
extern void mt_cpufreq_thermal_protect(unsigned int limited_power);
void mt_cpufreq_enable_by_ptpod(void);
unsigned int mt_cpufreq_disable_by_ptpod(void);
extern unsigned int mt_cpufreq_max_frequency_by_DVS(unsigned int num);
void mt_cpufreq_return_default_DVS_by_ptpod(void);
extern bool mt_cpufreq_earlysuspend_status_get(void);

/******************************
* Extern Function Declaration
*******************************/
extern int spm_dvfs_ctrl_volt(u32 value);
#if FULLY_VERSION_DVFS
extern int mtk_cpufreq_register(struct mt_cpu_power_info *freqs, int num);
#endif
extern void hp_limited_cpu_num(int num);
extern u32 PTP_get_ptp_level(void);

extern unsigned int mt_get_cpu_freq(void);
extern void dbs_freq_thermal_limited(unsigned int limited, unsigned int freq);
#ifdef CPUFREQ_SDIO_TRANSFER
extern int sdio_start_ot_transfer(void);
extern int sdio_stop_transfer(void);
extern void cpufreq_min_sampling_rate_change(unsigned int sample_rate);
extern bool is_vcore_ss_corner(void);
#endif

/*****************
* dummy type define for kernel 3.10
******************/
enum mt_cpu_dvfs_id {

#if 0
#ifdef /* MTK_FORCE_CLUSTER1 */
	MT_CPU_DVFS_BIG,
	MT_CPU_DVFS_LITTLE,
#else
	MT_CPU_DVFS_LITTLE,
	MT_CPU_DVFS_BIG,
#endif
#else
	MT_CPU_DVFS_LITTLE,
	MT_CPU_DVFS_BIG,
#endif

	NR_MT_CPU_DVFS,
};

#endif
