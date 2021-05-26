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

#ifndef _MT_SPM_
#define _MT_SPM_

#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/irqdesc.h>
#include <linux/irqchip/mtk-gic-extend.h>

extern void __iomem *spm_base;
extern u32 spm_irq_0;
extern u32 spm_irq_1;
extern u32 spm_irq_2;
extern u32 spm_irq_3;
extern u32 spm_rtc_cnt;

#undef SPM_BASE
#define SPM_BASE spm_base

/* #include <mach/mt_irq.h> */
#include <mt-plat/sync_write.h>
#include <mt-plat/mtk_io.h>

/**************************************
 * Config and Parameter
 **************************************/
#define SPM_IRQ0_ID		spm_irq_0
#define SPM_IRQ1_ID		spm_irq_1
#define SPM_IRQ2_ID		spm_irq_2
#define SPM_IRQ3_ID		spm_irq_3

/**************************************
 * Define and Declare
 **************************************/
#define SPM_POWERON_CONFIG_SET		(SPM_BASE + 0x000)
#define SPM_POWER_ON_VAL0		(SPM_BASE + 0x010)
#define SPM_POWER_ON_VAL1		(SPM_BASE + 0x014)
#define SPM_CLK_SETTLE			(SPM_BASE + 0x100)
#define SPM_CA7_CPU0_PWR_CON		(SPM_BASE + 0x200)
#define SPM_CA7_DBG_PWR_CON		(SPM_BASE + 0x204)
#define SPM_CA7_CPUTOP_PWR_CON		(SPM_BASE + 0x208)
#define SPM_VDE_PWR_CON			(SPM_BASE + 0x210)
#define SPM_MFG_PWR_CON			(SPM_BASE + 0x214)
#define SPM_CA7_CPU1_PWR_CON		(SPM_BASE + 0x218)
#define SPM_CA7_CPU2_PWR_CON		(SPM_BASE + 0x21c)
#define SPM_CA7_CPU3_PWR_CON		(SPM_BASE + 0x220)
#define SPM_VEN_PWR_CON			(SPM_BASE + 0x230)
#define SPM_IFR_PWR_CON			(SPM_BASE + 0x234)
#define SPM_ISP_PWR_CON			(SPM_BASE + 0x238)
#define SPM_DIS_PWR_CON			(SPM_BASE + 0x23c)
#define SPM_DPY_PWR_CON			(SPM_BASE + 0x240)
#define SPM_CA7_CPUTOP_L2_PDN		(SPM_BASE + 0x244)
#define SPM_CA7_CPUTOP_L2_SLEEP		(SPM_BASE + 0x248)
#define SPM_CA7_CPU0_L1_PDN		(SPM_BASE + 0x25c)
#define SPM_CA7_CPU1_L1_PDN		(SPM_BASE + 0x264)
#define SPM_CA7_CPU2_L1_PDN		(SPM_BASE + 0x26c)
#define SPM_CA7_CPU3_L1_PDN		(SPM_BASE + 0x274)
#define SPM_GCPU_SRAM_CON		(SPM_BASE + 0x27c)
#define SPM_CONN_PWR_CON		(SPM_BASE + 0x280)
#define SPM_MCU_PWR_CON			(SPM_BASE + 0x290)
#define SPM_IFR_SRAMROM_CON		(SPM_BASE + 0x294)
#define SPM_MJC_PWR_CON			(SPM_BASE + 0x298)
#define SPM_AUDIO_PWR_CON		(SPM_BASE + 0x29c)
#define SPM_MFG_2D_PWR_CON      (SPM_BASE + 0x2c0)
#define SPM_MFG_ASYNC_PWR_CON		(SPM_BASE + 0x2c4)
#define SPM_ARMPLL_DIV_PWR_CON  (SPM_BASE + 0x2cc)
#define SPM_INFRA_MD_PWR_CON    (SPM_BASE + 0x2d8)
#define SPM_CPU_EXT_ISO         (SPM_BASE + 0x2dc)
#define SPM_ETH_PWR_CON		        (SPM_BASE + 0x2e0)
#define SPM_WB_PWR_CON		        (SPM_BASE + 0x2ec)
#define SPM_PCM_CON0			(SPM_BASE + 0x310)
#define SPM_PCM_CON1			(SPM_BASE + 0x314)
#define SPM_PCM_IM_PTR			(SPM_BASE + 0x318)
#define SPM_PCM_IM_LEN			(SPM_BASE + 0x31c)
#define SPM_PCM_REG_DATA_INI		(SPM_BASE + 0x320)
#define SPM_PCM_EVENT_VECTOR0		(SPM_BASE + 0x340)
#define SPM_PCM_EVENT_VECTOR1		(SPM_BASE + 0x344)
#define SPM_PCM_EVENT_VECTOR2		(SPM_BASE + 0x348)
#define SPM_PCM_EVENT_VECTOR3		(SPM_BASE + 0x34c)
#define SPM_PCM_MAS_PAUSE_MASK		(SPM_BASE + 0x354)
#define SPM_PCM_PWR_IO_EN		(SPM_BASE + 0x358)
#define SPM_PCM_TIMER_VAL		(SPM_BASE + 0x35c)
#define SPM_PCM_TIMER_OUT		(SPM_BASE + 0x360)
#define SPM_PCM_REG0_DATA		(SPM_BASE + 0x380)
#define SPM_PCM_REG1_DATA		(SPM_BASE + 0x384)
#define SPM_PCM_REG2_DATA		(SPM_BASE + 0x388)
#define SPM_PCM_REG3_DATA		(SPM_BASE + 0x38c)
#define SPM_PCM_REG4_DATA		(SPM_BASE + 0x390)
#define SPM_PCM_REG5_DATA		(SPM_BASE + 0x394)
#define SPM_PCM_REG6_DATA		(SPM_BASE + 0x398)
#define SPM_PCM_REG7_DATA		(SPM_BASE + 0x39c)
#define SPM_PCM_REG8_DATA		(SPM_BASE + 0x3a0)
#define SPM_PCM_REG9_DATA		(SPM_BASE + 0x3a4)
#define SPM_PCM_REG10_DATA		(SPM_BASE + 0x3a8)
#define SPM_PCM_REG11_DATA		(SPM_BASE + 0x3ac)
#define SPM_PCM_REG12_DATA		(SPM_BASE + 0x3b0)
#define SPM_PCM_REG13_DATA		(SPM_BASE + 0x3b4)
#define SPM_PCM_REG14_DATA		(SPM_BASE + 0x3b8)
#define SPM_PCM_REG15_DATA		(SPM_BASE + 0x3bc)
#define SPM_PCM_EVENT_REG_STA		(SPM_BASE + 0x3c0)
#define SPM_PCM_FSM_STA			(SPM_BASE + 0x3c4)
#define SPM_PCM_IM_HOST_RW_PTR		(SPM_BASE + 0x3c8)
#define SPM_PCM_IM_HOST_RW_DAT		(SPM_BASE + 0x3cc)
#define SPM_PCM_EVENT_VECTOR4		(SPM_BASE + 0x3d0)
#define SPM_PCM_EVENT_VECTOR5		(SPM_BASE + 0x3d4)
#define SPM_PCM_EVENT_VECTOR6		(SPM_BASE + 0x3d8)
#define SPM_PCM_EVENT_VECTOR7		(SPM_BASE + 0x3dc)
#define SPM_PCM_SW_INT_SET		(SPM_BASE + 0x3e0)
#define SPM_PCM_SW_INT_CLEAR		(SPM_BASE + 0x3e4)
#define SPM_CLK_CON			(SPM_BASE + 0x400)
#define SPM_SLEEP_DUAL_VCORE_PWR_CON	(SPM_BASE + 0x404)
#define SPM_SLEEP_PTPOD2_CON		(SPM_BASE + 0x408)
#define SPM_APMCU_PWRCTL		(SPM_BASE + 0x600)
#define SPM_AP_DVFS_CON_SET		(SPM_BASE + 0x604)
#define SPM_AP_STANBY_CON		(SPM_BASE + 0x608)
#define SPM_PWR_STATUS			(SPM_BASE + 0x60c)
#define SPM_PWR_STATUS_2ND		(SPM_BASE + 0x610)
#define SPM_SLEEP_MDBSI_CON		(SPM_BASE + 0x614)
#define SPM_RF_CLK_CFG		        (SPM_BASE + 0x620)
#define SPM_RF_CFG_SET		      (SPM_BASE + 0x624)
#define SPM_RF_CLK_CFG_CLR		    (SPM_BASE + 0x628)
#define SPM_BSI_DO_SR		      (SPM_BASE + 0x62c)
#define SPM_BSI_D1_SR		      (SPM_BASE + 0x630)
#define SPM_BSI_D2_SR		      (SPM_BASE + 0x634)
#define SPM_AP_SEMA		        (SPM_BASE + 0x638)
#define SPM_SPM_SEMA		      (SPM_BASE + 0x63c)
#define SPM_SLEEP_TIMER_STA		(SPM_BASE + 0x720)
#define SPM_SLEEP_TWAM_CON		(SPM_BASE + 0x760)
#define SPM_SLEEP_TWAM_STATUS0		(SPM_BASE + 0x764)
#define SPM_SLEEP_TWAM_STATUS1		(SPM_BASE + 0x768)
#define SPM_SLEEP_TWAM_STATUS2		(SPM_BASE + 0x76c)
#define SPM_SLEEP_TWAM_STATUS3		(SPM_BASE + 0x770)
#define SPM_SLEEP_TWAM_CURR_STATUS0	(SPM_BASE + 0x774)
#define SPM_SLEEP_TWAM_CURR_STATUS1	(SPM_BASE + 0x778)
#define SPM_SLEEP_TWAM_CURR_STATUS2	(SPM_BASE + 0x77C)
#define SPM_SLEEP_TWAM_CURR_STATUS3	(SPM_BASE + 0x780)
#define SPM_SLEEP_TWAM_TIMER_OUT	(SPM_BASE + 0x784)
#define SPM_SLEEP_TWAM_WINDOW_LEN	(SPM_BASE + 0x788)
#define SPM_SLEEP_IDLE_SEL		(SPM_BASE + 0x78C)
#define SPM_SLEEP_WAKEUP_EVENT_MASK	(SPM_BASE + 0x810)
#define SPM_SLEEP_CPU_WAKEUP_EVENT	(SPM_BASE + 0x814)
#define SPM_PCM_WDT_TIMER_VAL		(SPM_BASE + 0x824)
#define SPM_PCM_WDT_TIMER_OUT		(SPM_BASE + 0x828)
#define SPM_SLEEP_ISR_MASK		(SPM_BASE + 0x900)
#define SPM_SLEEP_ISR_STATUS		(SPM_BASE + 0x904)
#define SPM_SLEEP_ISR_RAW_STA		(SPM_BASE + 0x910)
#define SPM_SLEEP_WAKEUP_MISC		(SPM_BASE + 0x918)
#define SPM_SLEEP_BUS_PROTECT_RDY	(SPM_BASE + 0x91c)
#define SPM_SLEEP_SUBSYS_IDLE_STA	(SPM_BASE + 0x920)
#define SPM_PCM_RESERVE			(SPM_BASE + 0xb00)
#define SPM_PCM_RESERVE2		(SPM_BASE + 0xb04)
#define SPM_PCM_FLAGS			(SPM_BASE + 0xb08)
#define SPM_PCM_SRC_REQ			(SPM_BASE + 0xb0c)
#define SPM_PCM_RESERVE3		(SPM_BASE + 0xb14)
#define SPM_PCM_RESERVE4		(SPM_BASE + 0xb18)
#define SPM_PCM_MMDDR_MASK		(SPM_BASE + 0xb1c)
#define SPM_PCM_DEBUG_CON		(SPM_BASE + 0xb20)
#define SPM_PCM_WDT_LATCH		(SPM_BASE + 0xb24)
#define SPM_CA7_CPU0_IRQ_MASK		(SPM_BASE + 0xb30)
#define SPM_CA7_CPU1_IRQ_MASK		(SPM_BASE + 0xb34)
#define SPM_CA7_CPU2_IRQ_MASK		(SPM_BASE + 0xb38)
#define SPM_CA7_CPU3_IRQ_MASK		(SPM_BASE + 0xb3c)
#define SPM_PCM_PASR_DPD_0		(SPM_BASE + 0xb60)
#define SPM_PCM_PASR_DPD_1		(SPM_BASE + 0xb64)
#define SPM_PCM_PASR_DPD_2		(SPM_BASE + 0xb68)
#define SPM_PCM_PASR_DPD_3		(SPM_BASE + 0xb6c)
#define SPM_SLEEP_CA7_WFI0_EN		(SPM_BASE + 0xf00)
#define SPM_SLEEP_CA7_WFI1_EN		(SPM_BASE + 0xf04)
#define SPM_SLEEP_CA7_WFI2_EN		(SPM_BASE + 0xf08)
#define SPM_SLEEP_CA7_WFI3_EN		(SPM_BASE + 0xf0c)

#define SPM_PROJECT_CODE	0xb16

#define SPM_REGWR_EN		(1U << 0)
#define SPM_REGWR_CFG_KEY	(SPM_PROJECT_CODE << 16)

#define SPM_CPU_PDN_DIS		(1U << 0)
#define SPM_INFRA_PDN_DIS	(1U << 1)
#define SPM_DDRPHY_PDN_DIS	(1U << 2)

/* Wakeup Source*/
#define SPM_WAKE_SRC_LIST	{	\
	SPM_WAKE_SRC(0, SPM_MERGE),	/* PCM timer, TWAM or CPU */	\
	SPM_WAKE_SRC(3, WDT),		\
	SPM_WAKE_SRC(4, GPT),		\
	SPM_WAKE_SRC(5, EINT),	\
	SPM_WAKE_SRC(6, CONN_WDT),		\
	SPM_WAKE_SRC(7, PCIE),	\
	SPM_WAKE_SRC(9, GMAC),	\
	SPM_WAKE_SRC(10, CONN2AP),	\
	SPM_WAKE_SRC(11, F26M_WAKE),	\
	SPM_WAKE_SRC(12, F26M_SLEEP),	\
	SPM_WAKE_SRC(13, PCM_WDT),	\
	SPM_WAKE_SRC(14, USB_CD),	\
	SPM_WAKE_SRC(15, USB_PDN),	\
	SPM_WAKE_SRC(18, SEJ), \
	SPM_WAKE_SRC(19, UART0),	\
	SPM_WAKE_SRC(20, AFE),		\
	SPM_WAKE_SRC(21, THERM),	\
	SPM_WAKE_SRC(22, CIRQ),		\
	SPM_WAKE_SRC(24, SYSPWREQ),	\
	SPM_WAKE_SRC(27, RTC),		\
	SPM_WAKE_SRC(29, CPU_IRQ),	 \
	SPM_WAKE_SRC(30, APSRC_WAKE),	\
	SPM_WAKE_SRC(31, APSRC_SLEEP)	\
}

enum SPM_WAKE_SRC {
	WAKE_SRC_SPM_MERGE = (1U << 0),	/* WAKE_ID_SPM_MERGE */
	WAKE_SRC_WDT = (1U << 3),	/* WAKE_ID_WDT */
	WAKE_SRC_GPT = (1U << 4),	/* WAKE_ID_GPT */
	WAKE_SRC_EINT = (1U << 5),	/* WAKE_ID_EINT */
	WAKE_SRC_CONN_WDT = (1U << 6),	/* WAKE_ID_CONN_WDT */
	WAKE_SRC_PCIE = (1U << 7),	/* WAKE_ID_PCEI */
	WAKE_SRC_GMAC = (1U << 9),	/* WAKE_ID_GMAC */
	WAKE_SRC_CONN2AP = (1U << 10),	/* WAKE_ID_CONN2AP */
	WAKE_SRC_F26M_WAKE = (1U << 11),	/* WAKE_ID_F26M_WAKE */
	WAKE_SRC_F26M_SLEE = (1U << 12),	/* WAKE_ID_F26M_SLEEP */
	WAKE_SRC_PCM_WDT = (1U << 13),	/* WAKE_ID_PCM_WDT */
	WAKE_SRC_USB_CD = (1U << 14),	/* WAKE_ID_USB_CD */
	WAKE_SRC_USB_PDN = (1U << 15),	/* WAKE_ID_USB_PDN */
	WAKE_SRC_SEJ = (1U << 16),	/* WAKE_ID_SEJ */
	WAKE_SRC_UART0 = (1U << 19),	/* WAKE_ID_UART0 */
	WAKE_SRC_AFE = (1U << 20),	/* WAKE_ID_AFE */
	WAKE_SRC_THERM = (1U << 21),	/* WAKE_ID_THERM */
	WAKE_SRC_CIRQ = (1U << 22),	/* WAKE_ID_CIRQ */
	WAKE_SRC_SYSPWREQ = (1U << 24),	/* WAKE_ID_SYSPWREQ */
	WAKE_SRC_IRRX = (1U << 25),	/* WAKE_ID_IRRX */
	WAKE_SRC_RTC = (1U << 27),	/* WAKE_ID_RTC */
	WAKE_SRC_CPU_IRQ = (1U << 29),	/* WAKE_ID_CPU_IRQ */
	WAKE_SRC_APSRC_WAKE = (1U << 30),	/* WAKE_ID_APSRC_WAKE */
	WAKE_SRC_APSRC_SLEEP = (1U << 31),	/* WAKE_ID_APSRC_SLEEP */
};

/* enum SPM_WAKE_SRC_LIST; */

typedef enum {
	WR_NONE = 0,
	WR_UART_BUSY = 1,
	WR_PCM_ASSERT = 2,
	WR_PCM_TIMER = 3,
	WR_WAKE_SRC = 4,
	WR_UNKNOWN = 5,
} wake_reason_t;

struct twam_sig {
	u32 sig0;		/* signal 0: config or status */
	u32 sig1;		/* signal 1: config or status */
	u32 sig2;		/* signal 2: config or status */
	u32 sig3;		/* signal 3: config or status */
};

typedef void (*twam_handler_t) (struct twam_sig *twamsig);

/* for ANC in talking */
extern void spm_mainpll_on_request(const char *drv_name);
extern void spm_mainpll_on_unrequest(const char *drv_name);

/* for TWAM in MET */
extern void spm_twam_register_handler(twam_handler_t handler);
extern void spm_twam_enable_monitor(const struct twam_sig *twamsig,
							bool speed_mode);
extern void spm_twam_disable_monitor(void);
extern void spm_twam_set_idle_select(unsigned int sel);
extern void spm_twam_set_window_length(unsigned int len);
extern void spm_twam_set_mon_type(struct twam_sig *mon);

extern uint32_t mt_xo_get_current_capid(void);

/* to replace mt_irq_unmask_for_sleep */
extern void unmask_irq(struct irq_desc *desc);

extern u32 spm_get_wake_period(int pwake_time, wake_reason_t last_wr);

extern unsigned int spm_get_cpu_pwr_status(void);

/**************************************
 * Macro and Inline
 **************************************/
#define spm_read(addr)			__raw_readl(IOMEM(addr))
#define spm_write(addr, val)		mt_reg_sync_writel(val, addr)

#define is_cpu_pdn(flags)		(!((flags) & SPM_CPU_PDN_DIS))
#define is_infra_pdn(flags)		(!((flags) & SPM_INFRA_PDN_DIS))
#define is_ddrphy_pdn(flags)		(!((flags) & SPM_DDRPHY_PDN_DIS))
#define is_dualvcore_pdn(flags)		(!((flags) & SPM_DUALVCORE_PDN_DIS))

#define get_high_cnt(sigsta)		((sigsta) & 0x3ff)
#define get_high_percent(sigsta)	\
	((get_high_cnt(sigsta) * 100 + 511) / 1023)

#endif
