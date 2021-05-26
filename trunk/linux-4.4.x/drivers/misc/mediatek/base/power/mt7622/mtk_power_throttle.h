/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#ifndef MT_POWERTHROTTLE_H
#define MT_POWERTHROTTLE_H

struct mt_cpu_freq_info {
	unsigned int cpufreq_khz;
	unsigned int cpufreq_volt;  /* mv * 1000 */
	const unsigned int cpufreq_volt_org;    /* mv * 1000 */
};

#define OP(khz, volt) {            \
	.cpufreq_khz = khz,             \
	.cpufreq_volt = volt,           \
	.cpufreq_volt_org = volt,       \
}

extern struct mt_cpu_freq_info opp_tbl_default[];
extern int setup_power_table_tk(void);
extern void dump_power_table(void);
extern void mt_cpufreq_thermal_protect(unsigned int limited_power);

enum mt_cpu_dvfs_id {
	MT_CPU_DVFS_LITTLE,
	NR_MT_CPU_DVFS,
};

extern void mt_cpufreq_enable_by_ptpod(enum mt_cpu_dvfs_id id);
extern unsigned int mt_cpufreq_disable_by_ptpod(enum mt_cpu_dvfs_id id);

#endif

