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

#ifndef __MTK_CPUFREQ_H__
#define __MTK_CPUFREQ_H__
#include <linux/kernel.h>

typedef void (*cpuVoltsampler_func) (enum mt_cpu_dvfs_id, unsigned int mv);

extern void mt_cpufreq_setvolt_registerCB(cpuVoltsampler_func pCB);
/* CPUFREQ */
extern void aee_record_cpufreq_cb(unsigned int step);
#endif	/* __MTK_CPUFREQ_H__ */
