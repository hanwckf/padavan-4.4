/*
 * Copyright (c) 2015 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _MT_SPM_SLEEP_
#define _MT_SPM_SLEEP_

#include <linux/kernel.h>
#include <linux/irq.h>
#include <linux/irqdesc.h>
#include <linux/irqchip/mtk-gic-extend.h>

typedef enum {
	WR_NONE = 0,
	WR_UART_BUSY = 1,
	WR_PCM_ASSERT = 2,
	WR_PCM_TIMER = 3,
	WR_PCM_WDT = 4,
	WR_WAKE_SRC = 5,
	WR_UNKNOWN = 6,
} wake_reason_t;

/*
 * for suspend
 */
extern int spm_set_sleep_wakesrc(u32 wakesrc, bool enable, bool replace);
extern wake_reason_t spm_go_to_sleep(bool cpu_pdn, bool infra_pdn,
			int pwake_time);
extern wake_reason_t spm_go_to_sleep_dpidle(bool cpu_pdn, u16 pwrlevel,
			int pwake_time);


/*
 * for deep idle
 */
extern void spm_dpidle_before_wfi(void);	/* can be redefined */
extern void spm_dpidle_after_wfi(void);	/* can be redefined */
extern wake_reason_t spm_go_to_dpidle(bool cpu_pdn, u16 pwrlevel);

/*
 * for ultra-deep idle
 */
#define SPM_ULTRA_DP_ENABLED				0
#define SPM_LOCK_UNLOCK_SYNC				0

/* to replace mt_irq_unmask_for_sleep */
extern void unmask_irq(struct irq_desc *desc);
extern bool spm_is_md_sleep(void);
extern bool spm_is_conn_sleep(void);

extern void spm_output_sleep_option(void);

extern int get_dynamic_period(int first_use, int first_wakeup_time,
			int battery_capacity_level);

extern void mtk_uart_restore(void);
extern void dump_uart_reg(void);

extern spinlock_t spm_lock;
#endif
