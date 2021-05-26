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

#ifndef _MT_SPM_
#define _MT_SPM_

#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <mt-plat/sync_write.h>

#ifndef CONFIG_OF
#include <mach/mt_reg_base.h>
#define SPM_BASE		SLEEP_BASE
#else
extern void __iomem *spm_base;
#undef SPM_BASE
#define SPM_BASE		(spm_base)
#endif

/*
 * for SPM register control
 */
#define SPM_POWERON_CONFIG_SET      (SPM_BASE + 0x0000)
#define SPM_POWER_ON_VAL0           (SPM_BASE + 0x0010)
#define SPM_POWER_ON_VAL1           (SPM_BASE + 0x0014)
#define SPM_CLK_SETTLE              (SPM_BASE + 0x0100)
#define SPM_MP0_CPU0_PWR_CON        (SPM_BASE + 0x0200)
#define SPM_MP0_CPUTOP_PWR_CON      (SPM_BASE + 0x0208)
#define SPM_MP0_CPU1_PWR_CON        (SPM_BASE + 0x0218)
#define SPM_IFR_PWR_CON             (SPM_BASE + 0x0234)
#define SPM_DPY_PWR_CON             (SPM_BASE + 0x0240)
#define SPM_MP0_CPUTOP_L2_PDN       (SPM_BASE + 0x0244)
#define SPM_MP0_CPUTOP_L2_SLEEP     (SPM_BASE + 0x0248)
#define SPM_MP0_CPU0_L1_PDN         (SPM_BASE + 0x025c)
#define SPM_MP0_CPU1_L1_PDN         (SPM_BASE + 0x0264)
#define SPM_MCU_PWR_CON             (SPM_BASE + 0x0290)
#define SPM_IFR_SRAMROM_CON         (SPM_BASE + 0x0294)
#define SPM_CPU_EXT_ISO             (SPM_BASE + 0x02dc)
#define SPM_ETH_PWR_CON             (SPM_BASE + 0x02e0)
#define SPM_HIF0_PWR_CON            (SPM_BASE + 0x02e4)
#define SPM_HIF1_PWR_CON            (SPM_BASE + 0x02e8)
#define SPM_WB_PWR_CON              (SPM_BASE + 0x02ec)
#define SPM_PCM_CON0                (SPM_BASE + 0x0310)
#define SPM_PCM_CON1                (SPM_BASE + 0x0314)
#define SPM_PCM_IM_PTR              (SPM_BASE + 0x0318)
#define SPM_PCM_IM_LEN              (SPM_BASE + 0x031c)
#define SPM_PCM_REG_DATA_INI        (SPM_BASE + 0x0320)
#define SPM_PCM_EVENT_VECTOR0       (SPM_BASE + 0x0340)
#define SPM_PCM_EVENT_VECTOR1       (SPM_BASE + 0x0344)
#define SPM_PCM_EVENT_VECTOR2       (SPM_BASE + 0x0348)
#define SPM_PCM_EVENT_VECTOR3       (SPM_BASE + 0x034c)
#define SPM_PCM_MAS_PAUSE_MASK      (SPM_BASE + 0x0354)
#define SPM_PCM_PWR_IO_EN           (SPM_BASE + 0x0358)
#define SPM_PCM_TIMER_VAL           (SPM_BASE + 0x035c)
#define SPM_PCM_TIMER_OUT           (SPM_BASE + 0x0360)
#define SPM_PCM_REG0_DATA           (SPM_BASE + 0x0380)
#define SPM_PCM_REG1_DATA           (SPM_BASE + 0x0384)
#define SPM_PCM_REG2_DATA           (SPM_BASE + 0x0388)
#define SPM_PCM_REG3_DATA           (SPM_BASE + 0x038c)
#define SPM_PCM_REG4_DATA           (SPM_BASE + 0x0390)
#define SPM_PCM_REG5_DATA           (SPM_BASE + 0x0394)
#define SPM_PCM_REG6_DATA           (SPM_BASE + 0x0398)
#define SPM_PCM_REG7_DATA           (SPM_BASE + 0x039c)
#define SPM_PCM_REG8_DATA           (SPM_BASE + 0x03a0)
#define SPM_PCM_REG9_DATA           (SPM_BASE + 0x03a4)
#define SPM_PCM_REG10_DATA          (SPM_BASE + 0x03a8)
#define SPM_PCM_REG11_DATA          (SPM_BASE + 0x03ac)
#define SPM_PCM_REG12_DATA          (SPM_BASE + 0x03b0)
#define SPM_PCM_REG13_DATA          (SPM_BASE + 0x03b4)
#define SPM_PCM_REG14_DATA          (SPM_BASE + 0x03b8)
#define SPM_PCM_REG15_DATA          (SPM_BASE + 0x03bc)
#define SPM_PCM_EVENT_REG_STA       (SPM_BASE + 0x03c0)
#define SPM_PCM_FSM_STA             (SPM_BASE + 0x03c4)
#define SPM_PCM_IM_HOST_RW_PTR      (SPM_BASE + 0x03c8)
#define SPM_PCM_IM_HOST_RW_DAT      (SPM_BASE + 0x03cc)
#define SPM_PCM_EVENT_VECTOR4       (SPM_BASE + 0x03d0)
#define SPM_PCM_EVENT_VECTOR5       (SPM_BASE + 0x03d4)
#define SPM_PCM_EVENT_VECTOR6       (SPM_BASE + 0x03d8)
#define SPM_PCM_EVENT_VECTOR7       (SPM_BASE + 0x03dc)
#define SPM_PCM_SW_INT_SEL          (SPM_BASE + 0x03e0)
#define SPM_PCM_SW_INT_CLEAR        (SPM_BASE + 0x03e4)
#define SPM_CLK_CON                 (SPM_BASE + 0x0400)
#define SPM_APMCU_PWRCTL            (SPM_BASE + 0x0600)
#define SPM_AP_DVFS_CON_SET         (SPM_BASE + 0x0604)
#define SPM_AP_STANBY_CON           (SPM_BASE + 0x0608)
#define SPM_PWR_STATUS              (SPM_BASE + 0x060c)
#define SPM_PWR_STATUS_S            (SPM_BASE + 0x0610)
#define SPM_AP_SEMA                 (SPM_BASE + 0x0638)
#define SPM_SPM_SEMA                (SPM_BASE + 0x063c)
#define SPM_SLEEP_TIMER_STA         (SPM_BASE + 0x0720)
#define SPM_SLEEP_TWAM_CON          (SPM_BASE + 0x0760)
#define SPM_SLEEP_TWAM_LAST_STATUS0 (SPM_BASE + 0x0764)
#define SPM_SLEEP_TWAM_LAST_STATUS1 (SPM_BASE + 0x0768)
#define SPM_SLEEP_TWAM_LAST_STATUS2 (SPM_BASE + 0x076c)
#define SPM_SLEEP_TWAM_LAST_STATUS3 (SPM_BASE + 0x0770)
#define SPM_SLEEP_TWAM_CURR_STATUS0 (SPM_BASE + 0x0774)
#define SPM_SLEEP_TWAM_CURR_STATUS1 (SPM_BASE + 0x0778)
#define SPM_SLEEP_TWAM_CURR_STATUS2 (SPM_BASE + 0x077C)
#define SPM_SLEEP_TWAM_CURR_STATUS3 (SPM_BASE + 0x0780)
#define SPM_SLEEP_TWAM_TIMER_OUT    (SPM_BASE + 0x0784)
#define SPM_SLEEP_TWAM_WINDOW_LEN   (SPM_BASE + 0x0788)
#define SPM_SLEEP_WAKEUP_EVENT_MASK (SPM_BASE + 0x0810)
#define SPM_SLEEP_CPU_WAKEUP_EVENT  (SPM_BASE + 0x0814)
#define SPM_PCM_WDT_TIMER_VAL       (SPM_BASE + 0x0824)
#define SPM_PCM_WDT_TIMER_OUT       (SPM_BASE + 0x0828)
#define SPM_SLEEP_ISR_MASK          (SPM_BASE + 0x0900)
#define SPM_SLEEP_ISR_STATUS        (SPM_BASE + 0x0904)
#define SPM_SLEEP_ISR_RAW_STA       (SPM_BASE + 0x0910)
#define SPM_SLEEP_WAKEUP_MISC       (SPM_BASE + 0x0918)
#define SPM_SLEEP_BUS_PROTECT_RDY   (SPM_BASE + 0x091c)
#define SPM_SLEEP_SUBSYS_IDLE_STA   (SPM_BASE + 0x0920)
#define SPM_PCM_RESERVE             (SPM_BASE + 0x0b00)
#define SPM_PCM_RESERVE2            (SPM_BASE + 0x0b04)
#define SPM_PCM_FLAGS               (SPM_BASE + 0x0b08)
#define SPM_PCM_SRC_REQ             (SPM_BASE + 0x0b0c)
#define SPM_PCM_DEBUG_CON           (SPM_BASE + 0x0b20)
#define SPM_MP0_CPU0_IRQ_MASK       (SPM_BASE + 0x0b30)
#define SPM_MP0_CPU1_IRQ_MASK       (SPM_BASE + 0x0b34)
#define SPM_MP0_CPU2_IRQ_MASK       (SPM_BASE + 0x0b38)
#define SPM_MP0_CPU3_IRQ_MASK       (SPM_BASE + 0x0b3c)
#define SPM_PCM_PASR_DPD_0          (SPM_BASE + 0x0b60)
#define SPM_PCM_PASR_DPD_1          (SPM_BASE + 0x0b64)
#define SPM_PCM_PASR_DPD_2          (SPM_BASE + 0x0b68)
#define SPM_PCM_PASR_DPD_3          (SPM_BASE + 0x0b6c)
#define SPM_SLEEP_MP0_WFI0_EN       (SPM_BASE + 0x0f00)
#define SPM_SLEEP_MP0_WFI1_EN       (SPM_BASE + 0x0f04)
#define SPM_SLEEP_MP0_WFI2_EN       (SPM_BASE + 0x0f08)
#define SPM_SLEEP_MP0_WFI3_EN       (SPM_BASE + 0x0f0c)

#define SPM_PROJECT_CODE            0xb16

#define spm_read(addr)			\
	__raw_readl((void __force __iomem *)(addr))

#define spm_write(addr, val)		mt_reg_sync_writel(val, addr)

#endif /* _MT_SPM_ */
