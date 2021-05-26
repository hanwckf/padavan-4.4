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

#ifndef _MT_SPM_SLEEP_
#define _MT_SPM_SLEEP_

#include <linux/irq.h>
#include <linux/irqdesc.h>
#include <linux/irqchip/mtk-gic-extend.h>
#include <linux/kernel.h>
#include "mtk_spm.h"

/*
 * for suspend
 */
extern int spm_set_sleep_wakesrc(u32 wakesrc, bool enable, bool replace);
extern u32 spm_get_sleep_wakesrc(void);
extern wake_reason_t spm_go_to_sleep(u32 spm_flags, u32 spm_data);

extern void unmask_irq(struct irq_desc *desc);

extern bool spm_is_conn_sleep(void);
extern void spm_set_wakeup_src_check(void);
extern bool spm_check_wakeup_src(void);
extern void spm_poweron_config_set(void);
extern bool spm_set_suspned_pcm_init_flag(u32 *suspend_flags);

extern void spm_output_sleep_option(void);

/* record last wakesta */
extern u32 spm_get_last_wakeup_src(void);
extern u32 spm_get_last_wakeup_misc(void);
extern bool mt_xo_has_ext_crystal(void);
#endif
