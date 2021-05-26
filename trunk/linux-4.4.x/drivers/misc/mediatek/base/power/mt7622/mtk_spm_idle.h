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

#ifndef _MT_SPM_IDLE_
#define _MT_SPM_IDLE_

#include <linux/kernel.h>
#include "mtk_spm.h"
#include "mtk_spm_sleep.h"

#define TAG     "SPM-Idle"

#define spm_idle_err(fmt, args...)		pr_emerg(TAG fmt, ##args)
#define spm_idle_warn(fmt, args...)		pr_warn(TAG fmt, ##args)
#define spm_idle_dbg(fmt, args...)		pr_notice(TAG fmt, ##args)
#define spm_idle_info(fmt, args...)		pr_info(TAG fmt, ##args)
#define spm_idle_ver(fmt, args...)		pr_debug(TAG fmt, ##args)


/*
 * for Deep Idle
 */
void spm_dpidle_before_wfi(void);		 /* can be redefined */
void spm_dpidle_after_wfi(void);		 /* can be redefined */
wake_reason_t spm_go_to_dpidle(u32 spm_flags, u32 spm_data, u32 dump_log);
wake_reason_t spm_go_to_sleep_dpidle(u32 spm_flags, u32 spm_data);
int spm_set_dpidle_wakesrc(u32 wakesrc, bool enable, bool replace);
void spm_deepidle_init(void);
/* #define SPM_DEEPIDLE_PROFILE_TIME */
#ifdef SPM_DEEPIDLE_PROFILE_TIME
#define SPM_PROFILE_APXGPT GPT2
extern unsigned int dpidle_profile[4];
#endif

#define DEEPIDLE_LOG_NONE      0
#define DEEPIDLE_LOG_REDUCED   1
#define DEEPIDLE_LOG_FULL      2

/*
 * for Screen On Deep Idle
 */
void soidle_before_wfi(int cpu);
void soidle_after_wfi(int cpu);
void spm_go_to_sodi(u32 spm_flags, u32 spm_data);
void spm_sodi_lcm_video_mode(bool IsLcmVideoMode);
void spm_sodi_mempll_pwr_mode(bool pwr_mode);
void spm_enable_sodi(bool);
bool spm_get_sodi_en(void);
void spm_sodi_init(void);
/* #define SPM_SODI_PROFILE_TIME */
#ifdef SPM_SODI_PROFILE_TIME
#define SPM_SODI_PROFILE_APXGPT GPT2
#endif

/*
 * for Multi Core Deep Idle
 */
enum spm_mcdi_lock_id {
	SPM_MCDI_IDLE = 0,
	SPM_MCDI_VCORE_DVFS,
	SPM_MCDI_EARLY_SUSPEND,
};
int spm_mcdi_init(void);
void spm_mcdi_wakeup_all_cores(void);
void spm_mcdi_switch_on_off(enum spm_mcdi_lock_id id, int mcdi_en);
bool spm_mcdi_wfi(int core_id);
bool spm_mcdi_can_enter(void);
bool spm_is_cpu_irq_occur(int core_id);

#endif
