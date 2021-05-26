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

#ifndef _MT_SPM_
#define _MT_SPM_

#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/interrupt.h>

#ifdef CONFIG_OF
extern void __iomem *spm_base;
extern u32 spm_irq_0;
extern u32 spm_irq_1;

#undef SPM_BASE
#define SPM_BASE spm_base
#else
#include <mach/mt_reg_base.h>
#endif

#include <mt-plat/sync_write.h>
#include "mt_lpae.h"

/**************************************
 * Config and Parameter
 **************************************/

/* FIXME: for bring up */
#ifdef CONFIG_OF
#define SPM_IRQ0_ID		spm_irq_0
#define SPM_IRQ1_ID		spm_irq_1
#else
#define SPM_BASE		SLEEP_BASE

#define SPM_IRQ0_ID		117	/* SLEEP_IRQ_BIT0_ID */
#define SPM_IRQ1_ID		118	/* SLEEP_IRQ_BIT1_ID */
#endif

/*
 * for SPM register control
 */
#define SPM_POWERON_CONFIG_SET      (SPM_BASE + 0x0000)
#define SPM_POWER_ON_VAL0           (SPM_BASE + 0x0010)
#define SPM_POWER_ON_VAL1           (SPM_BASE + 0x0014)
#define SPM_CLK_SETTLE              (SPM_BASE + 0x0100)
#define SPM_FC0_PWR_CON             (SPM_BASE + 0x0200)
#define SPM_CPU_PWR_CON             (SPM_BASE + 0x0208)
#define SPM_VDE_PWR_CON             (SPM_BASE + 0x0210)
#define SPM_MFG_PWR_CON             (SPM_BASE + 0x0214)
#define SPM_FC1_PWR_CON             (SPM_BASE + 0x0218)
#define SPM_FC2_PWR_CON             (SPM_BASE + 0x021c)
#define SPM_FC3_PWR_CON             (SPM_BASE + 0x0220)
#define SPM_IFR_PWR_CON             (SPM_BASE + 0x0234)
#define SPM_ISP_PWR_CON             (SPM_BASE + 0x0238)
#define SPM_DIS_PWR_CON             (SPM_BASE + 0x023c)
#define SPM_DPY_PWR_CON             (SPM_BASE + 0x0240)
#define SPM_CPU_L2_DAT_PDN          (SPM_BASE + 0x0244)
#define SPM_CPU_L2_DAT_SLEEP_B      (SPM_BASE + 0x0248)
#define SPM_MP_CORE0_AUX            (SPM_BASE + 0x024c)
#define SPM_MP_CORE1_AUX            (SPM_BASE + 0x0250)
#define SPM_MP_CORE2_AUX            (SPM_BASE + 0x0254)
#define SPM_MP_CORE3_AUX            (SPM_BASE + 0x0258)
#define SPM_CPU_FC0_L1_PDN          (SPM_BASE + 0x025c)
#define SPM_CPU_FC1_L1_PDN          (SPM_BASE + 0x0264)
#define SPM_CPU_FC2_L1_PDN          (SPM_BASE + 0x026c)
#define SPM_CPU_FC3_L1_PDN          (SPM_BASE + 0x0274)
#define SPM_IFR_FH_SRAM_CTRL        (SPM_BASE + 0x027c)
#define SPM_CONN_PWR_CON            (SPM_BASE + 0x0280)
#define SPM_MD_PWR_CON              (SPM_BASE + 0x0284)
#define SPM_MCU_PWR_CON             (SPM_BASE + 0x0290)
#define SPM_IFR_SRAMROM_CON         (SPM_BASE + 0x0294)
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
#define SPM_SLEEP_TIMER_STA         (SPM_BASE + 0x0720)
#define SPM_SLEEP_TWAM_CON          (SPM_BASE + 0x0760)
#define SPM_SLEEP_TWAM_STATUS0      (SPM_BASE + 0x0764)
#define SPM_SLEEP_TWAM_STATUS1      (SPM_BASE + 0x0768)
#define SPM_SLEEP_TWAM_STATUS2      (SPM_BASE + 0x076c)
#define SPM_SLEEP_TWAM_STATUS3      (SPM_BASE + 0x0770)
#define SPM_SLEEP_WAKEUP_EVENT_MASK (SPM_BASE + 0x0810)
#define SPM_SLEEP_CPU_WAKEUP_EVENT  (SPM_BASE + 0x0814)
#define SPM_PCM_WDT_TIMER_VAL       (SPM_BASE + 0x0824)
#define SPM_SLEEP_ISR_MASK          (SPM_BASE + 0x0900)
#define SPM_SLEEP_ISR_STATUS        (SPM_BASE + 0x0904)
#define SPM_SLEEP_ISR_RAW_STA       (SPM_BASE + 0x0910)
#define SPM_SLEEP_WAKEUP_MISC       (SPM_BASE + 0x0918)
#define SPM_PCM_RESERVE             (SPM_BASE + 0x0b00)
#define SPM_PCM_SRC_REQ             (SPM_BASE + 0x0b04)
#define SPM_SLEEP_CPU_IRQ_MASK      (SPM_BASE + 0x0b10)
#define SPM_PCM_DEBUG_CON           (SPM_BASE + 0x0b20)
#define SPM_CORE0_WFI_SEL           (SPM_BASE + 0x0f00)
#define SPM_CORE1_WFI_SEL           (SPM_BASE + 0x0f04)
#define SPM_CORE2_WFI_SEL           (SPM_BASE + 0x0f08)
#define SPM_CORE3_WFI_SEL           (SPM_BASE + 0x0f0c)

#define SPM_PROJECT_CODE            0xb16

#define CON0_PCM_KICK               (1U << 0)
#define CON0_IM_KICK                (1U << 1)
#define CON0_IM_SLEEP_DVS           (1U << 3)
#define CON0_EVENT_VEC0_EN          (1U << 4)
#define CON0_EVENT_VEC1_EN          (1U << 5)
#define CON0_EVENT_VEC2_EN          (1U << 6)
#define CON0_EVENT_VEC3_EN          (1U << 7)
#define CON0_EVENT_VEC4_EN          (1U << 8)
#define CON0_EVENT_VEC5_EN          (1U << 9)
#define CON0_EVENT_VEC6_EN          (1U << 10)
#define CON0_EVENT_VEC7_EN          (1U << 11)
#define CON0_PCM_SW_RESET           (1U << 15)
#define CON0_CFG_KEY                (SPM_PROJECT_CODE << 16)

#define CON1_IM_SLAVE               (1U << 0)
#define CON1_MIF_APBEN              (1U << 3)
#define CON1_PCM_TIMER_EN           (1U << 5)
#define CON1_IM_NONRP_EN            (1U << 6)
#define CON1_PCM_WDT_EN             (1U << 8)
#define CON1_PCM_WDT_WAKE_MODE      (1U << 9)
#define CON1_SPM_SRAM_SLP_B         (1U << 10)
#define CON1_SPM_SRAM_ISO_B         (1U << 11)
#define CON1_CFG_KEY                (SPM_PROJECT_CODE << 16)

#define PCM_PWRIO_EN_R0             (1U << 0)
#define PCM_PWRIO_EN_R7             (1U << 7)
#define PCM_RF_SYNC_R0              (1U << 16)
#define PCM_RF_SYNC_R7              (1U << 23)

#define R7_UART_CLK_OFF_REQ         (1U << 0)
#define R7_WDT_KICK_P               (1U << 22)

#define R13_CONN_SRCCLKENI          (1U << 1)
#define R13_MD_SRCCLKENI            (1U << 4)
#define R13_UART_CLK_OFF_ACK        (1U << 20)

#define PCM_SW_INT0                 (1U << 0)
#define PCM_SW_INT1                 (1U << 1)
#define PCM_SW_INT2                 (1U << 2)
#define PCM_SW_INT3                 (1U << 3)
#define PCM_SW_INT_ALL              (PCM_SW_INT3 | PCM_SW_INT2 |	\
					PCM_SW_INT1 | PCM_SW_INT0)
#define CC_SYSCLK0_EN_0             (1U << 0)
#define CC_SYSCLK0_EN_1             (1U << 1)
#define CC_SYSSETTLE_SEL            (1U << 4)
#define CC_LOCK_INFRA_DCM           (1U << 5)
#define CC_SRCLKENA_MASK            (1U << 6)
#define CC_CXO32K_RM_EN_MD          (1U << 9)
#define CC_CXO32K_RM_EN_CONN        (1U << 10)
#define CC_CLKSQ0_SEL               (1U << 11)
#define CC_DISABLE_SODI             (1U << 13)
#define CC_DISABLE_DORM_PWR         (1U << 14)
#define CC_DISABLE_INFRA_PWR        (1U << 15)
#define CC_SRCLKEN0_EN              (1U << 16)

#define TWAM_CON_EN                 (1U << 0)
#define TWAM_CON_SPEED_EN           (1U << 4)

#define WAKE_SRC_TS                 (1U << 1)
#define WAKE_SRC_KP                 (1U << 2)
#define WAKE_SRC_WDT                (1U << 3)
#define WAKE_SRC_GPT                (1U << 4)
#define WAKE_SRC_EINT               (1U << 5)
#define WAKE_SRC_CONN_WDT           (1U << 6)
#define WAKE_SRC_CEC                (1U << 7)
#define WAKE_SRC_IRRX               (1U << 8)
#define WAKE_SRC_LOW_BAT            (1U << 9)
#define WAKE_SRC_CONN               (1U << 10)
#define WAKE_SRC_USB_CD             (1U << 14)
#define WAKE_SRC_USB_PDN            (1U << 16)
#define WAKE_SRC_DBGSYS             (1U << 18)
#define WAKE_SRC_UART0              (1U << 19)
#define WAKE_SRC_AFE                (1U << 20)
#define WAKE_SRC_THERM              (1U << 21)
#define WAKE_SRC_CIRQ               (1U << 22)
#define WAKE_SRC_CM4SYS             (1U << 23)
#define WAKE_SRC_SYSPWREQ           (1U << 24)
#define WAKE_SRC_ETHERNET           (1U << 25)
#define WAKE_SRC_CPU0_IRQ           (1U << 26)
#define WAKE_SRC_CPU1_IRQ           (1U << 27)
#define WAKE_SRC_CPU2_IRQ           (1U << 28)
#define WAKE_SRC_CPU3_IRQ           (1U << 29)

#define ISR_TWAM                    (1U << 2)
#define ISR_PCM_RETURN              (1U << 3)
#define ISR_PCM_IRQ0                (1U << 8)
#define ISR_PCM_IRQ1                (1U << 9)
#define ISR_PCM_IRQ2                (1U << 10)
#define ISR_PCM_IRQ3                (1U << 11)

#define ISRM_PCM_IRQ_AUX            (ISR_PCM_IRQ3 | ISR_PCM_IRQ2 | ISR_PCM_IRQ1)
#define ISRM_ALL_EXC_TWAM           (ISR_PCM_IRQ3 | ISR_PCM_IRQ2 |	\
					ISR_PCM_IRQ1 | ISR_PCM_IRQ0 |	\
					ISR_PCM_RETURN)
#define ISRM_ALL                    (ISR_PCM_IRQ3 | ISR_PCM_IRQ2 |	\
					ISR_PCM_IRQ1 | ISR_PCM_IRQ0 |	\
					ISR_PCM_RETURN | ISR_TWAM)

#define ISRC_ALL_EXC_TWAM           (ISR_PCM_RETURN)
#define ISRC_ALL                    (ISR_PCM_RETURN | ISR_TWAM)

#define EVENT_VEC(event, resume, imme, pc)	\
	(((event) << 0) | ((resume) << 5) | ((imme) << 6) | ((pc) << 16))

#define spm_read(addr)			\
	__raw_readl((void __force __iomem *)(addr))
#define spm_write(addr, val)		mt_reg_sync_writel(val, addr)

#define spm_is_wakesrc_invalid(wakesrc)     (!!((u32)(wakesrc) & 0xc082b881))

#define spm_emerg(fmt, args...)		pr_emerg("[SPM] " fmt, ##args)
#define spm_alert(fmt, args...)		pr_alert("[SPM] " fmt, ##args)
#define spm_crit(fmt, args...)		pr_crit("[SPM] " fmt, ##args)
#define spm_err(fmt, args...)		pr_err("[SPM] " fmt, ##args)
#define spm_warn(fmt, args...)		pr_warn("[SPM] " fmt, ##args)
#define spm_notice(fmt, args...)	pr_notice("[SPM] " fmt, ##args)
#define spm_info(fmt, args...)		pr_info("[SPM] " fmt, ##args)
#define spm_debug(fmt, args...)		pr_info("[SPM] " fmt, ##args)	/* pr_debug show nothing */

typedef struct {
	const u32 *base;	/* code array base */
	const u16 size;		/* code array size */
	const u16 sess;		/* session number */
	u32 vec0;		/* event vector 0 config */
	u32 vec1;		/* event vector 1 config */
	u32 vec2;		/* event vector 2 config */
	u32 vec3;		/* event vector 3 config */
	u32 vec4;		/* event vector 4 config */
	u32 vec5;		/* event vector 5 config */
	u32 vec6;		/* event vector 6 config */
	u32 vec7;		/* event vector 7 config */
} pcm_desc_t;

typedef struct {
	u32 sig0;		/* signal 0: config or status */
	u32 sig1;		/* signal 1: config or status */
	u32 sig2;		/* signal 2: config or status */
	u32 sig3;		/* signal 3: config or status */
} twam_sig_t;

typedef void (*twam_handler_t) (twam_sig_t *twamsig);

static inline u32 spm_get_base_phys(const u32 *base)
{
	phys_addr_t pa = virt_to_phys(base);

	MAPPING_DRAM_ACCESS_ADDR(pa);	/* for 4GB mode */
	return (u32) pa;

}

extern void spm_go_to_normal(void);

/*
 * for TWAM to integrate with MET
 */
extern void spm_twam_register_handler(twam_handler_t handler);
extern void spm_twam_enable_monitor(twam_sig_t *twamsig, bool speed_mode);
extern void spm_twam_disable_monitor(void);


/*
 * for PCM WDT to replace RGU WDT
 */
/* extern int spm_wdt_register_fiq(fiq_isr_handler rgu_wdt_handler); */
extern int spm_wdt_register_irq(irq_handler_t rgu_wdt_handler);
extern void spm_wdt_set_timeout(u32 sec);
extern void spm_wdt_enable_timer(void);
extern void spm_wdt_restart_timer(void);
extern void spm_wdt_restart_timer_nolock(void);
extern void spm_wdt_disable_timer(void);


extern int spm_dvfs_ctrl_volt(u32 value);

#endif
