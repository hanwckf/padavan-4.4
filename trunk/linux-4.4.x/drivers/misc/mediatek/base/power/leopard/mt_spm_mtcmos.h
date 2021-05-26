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

#include <linux/kernel.h>

#define STA_POWER_DOWN  0
#define STA_POWER_ON    1

/*
 * 1. for CPU MTCMOS: CPU1 & DBG0
 * 2. call spm_mtcmos_cpu_lock/unlock() before/after any operations
 */
extern int spm_mtcmos_cpu_init(void);
extern void spm_mtcmos_cpu_lock(unsigned long *flags);
extern void spm_mtcmos_cpu_unlock(unsigned long *flags);
extern int spm_mtcmos_ctrl_cpu1(int state, int check_wfi_before_pdn);
