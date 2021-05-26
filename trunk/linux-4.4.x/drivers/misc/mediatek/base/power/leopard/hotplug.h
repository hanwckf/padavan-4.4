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

#ifndef _HOTPLUG
#define _HOTPLUG

#include <linux/kernel.h>
#include <linux/atomic.h>

/* power on/off cpu*/
#define CONFIG_HOTPLUG_WITH_POWER_CTRL

/* global variable */
extern atomic_t hotplug_cpu_count;

extern void __disable_dcache(void);
extern void __enable_dcache(void);
extern void __inner_clean_dcache_L2(void);
extern void inner_dcache_flush_L1(void);
extern void inner_dcache_flush_L2(void);
extern void __switch_to_smp(void);
extern void __switch_to_amp(void);
extern void __disable_dcache__inner_flush_dcache_L1(void);
extern void
__disable_dcache__inner_flush_dcache_L1__inner_clean_dcache_L2(void);
extern void
__disable_dcache__inner_flush_dcache_L1__inner_flush_dcache_L2(void);

/* mt cpu hotplug callback for smp_operations */
extern int mt_cpu_kill(unsigned int cpu);
extern void mt_cpu_die(unsigned int cpu);
extern int mt_cpu_disable(unsigned int cpu);

#endif
