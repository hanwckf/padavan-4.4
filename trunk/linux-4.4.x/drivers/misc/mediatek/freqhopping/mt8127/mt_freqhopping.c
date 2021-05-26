/*
 * Copyright (C) 2011 MediaTek, Inc.
 *
 * Author: Holmes Chiou <holmes.chiou@mediatek.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/miscdevice.h>	/* L318_Need_Related_File */
#include <linux/vmalloc.h>
#include <linux/dma-mapping.h>
/* #include <board-custom.h> */
#include <linux/seq_file.h>

#include "mach/mt_freqhopping.h"
#include "mt_fhreg.h"
#include "sync_write.h"
#if 0				/* L318_Need_Related_File */
#include "mach/mt_clkmgr.h"
#include "mach/mt_typedefs.h"
#include "mach/mt_gpio.h"
#include "mach/mt_gpufreq.h"
#include "mach/mt_cpufreq.h"
#include "mach/mt_sleep.h"
#endif				/* L318_Need_Related_File */

#include <mt_freqhopping_drv.h>

/* porting, remove latter */
/* mt_reg_base.h */
#define EMI_BASE (0xF0203000)


/* mt_pm_init.c */
unsigned int mt_get_emi_freq(void)
{
	return 300;
}

/* porting, remove latter */

/* #define FH_MSG printk , TODO */
/* #define FH_BUG_ON(x) printk("BUGON %s:%d %s:%d",__FUNCTION__,__LINE__,current->comm,current->pid), TODO */
/* #define FH_BUG_ON(...) , TODO */

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>

static void __iomem *freqhopping_base;	/* 10209E00 */
static void __iomem *apmixed_base;	/* 0xF0209000 */
static void __iomem *g_ddrphy_base;	/* 0xF000F000 */


#ifdef CONFIG_ARM64
#define REG_ADDR(x)                 ((unsigned long)freqhopping_base   + (x))
#define APMIXEDSYS_BASE             ((unsigned long)apmixed_base)
#define DDRPHY_BASE                 ((unsigned long)g_ddrphy_base)
#else
#define REG_ADDR(x)                 ((unsigned int)freqhopping_base   + (x))
#define APMIXEDSYS_BASE             ((unsigned int)apmixed_base)
#define DDRPHY_BASE                 ((unsigned int)g_ddrphy_base)
#endif

#define REG_FHDMA_CFG	         REG_ADDR(0x0000)
#define REG_FHDMA_2G1BASE        REG_ADDR(0x0004)
#define REG_FHDMA_2G2BASE        REG_ADDR(0x0008)
#define REG_FHDMA_INTMDBASE      REG_ADDR(0x000C)
#define REG_FHDMA_EXTMDBASE      REG_ADDR(0x0010)
#define REG_FHDMA_BTBASE         REG_ADDR(0x0014)
#define REG_FHDMA_WFBASE         REG_ADDR(0x0018)
#define REG_FHDMA_FMBASE         REG_ADDR(0x001C)
#define REG_FHSRAM_CON	         REG_ADDR(0x0020)
#define REG_FHSRAM_WR            REG_ADDR(0x0024)
#define REG_FHSRAM_RD            REG_ADDR(0x0028)
#define REG_FHCTL_CFG	         REG_ADDR(0x002C)
#define REG_FHCTL_CON	         REG_ADDR(0x0030)
#define REG_FHCTL_2G1_CH         REG_ADDR(0x0034)
#define REG_FHCTL_2G2_CH         REG_ADDR(0x0038)
#define REG_FHCTL_INTMD_CH       REG_ADDR(0x003C)
#define REG_FHCTL_EXTMD_CH       REG_ADDR(0x0040)
#define REG_FHCTL_BT_CH          REG_ADDR(0x0044)
#define REG_FHCTL_WF_CH          REG_ADDR(0x0048)
#define REG_FHCTL_FM_CH          REG_ADDR(0x004C)
#define REG_FHCTL0_CFG	         REG_ADDR(0x0050)
#define REG_FHCTL0_UPDNLMT	     REG_ADDR(0x0054)
#define REG_FHCTL0_DDS	         REG_ADDR(0x0058)
#define REG_FHCTL0_DVFS	         REG_ADDR(0x005C)
#define REG_FHCTL0_MON	         REG_ADDR(0x0060)
#define REG_FHCTL1_CFG	         REG_ADDR(0x0064)
#define REG_FHCTL1_UPDNLMT	     REG_ADDR(0x0068)
#define REG_FHCTL1_DDS	         REG_ADDR(0x006C)
#define REG_FHCTL1_DVFS	         REG_ADDR(0x0070)
#define REG_FHCTL1_MON	         REG_ADDR(0x0074)
#define REG_FHCTL2_CFG	         REG_ADDR(0x0078)
#define REG_FHCTL2_UPDNLMT	     REG_ADDR(0x007C)
#define REG_FHCTL2_DDS	         REG_ADDR(0x0080)
#define REG_FHCTL2_DVFS	         REG_ADDR(0x0084)
#define REG_FHCTL2_MON	         REG_ADDR(0x0088)
#define REG_FHCTL3_CFG	         REG_ADDR(0x008C)
#define REG_FHCTL3_UPDNLMT	     REG_ADDR(0x0090)
#define REG_FHCTL3_DDS	         REG_ADDR(0x0094)
#define REG_FHCTL3_DVFS	         REG_ADDR(0x0098)
#define REG_FHCTL3_MON	         REG_ADDR(0x009C)
#define REG_FHCTL4_CFG	         REG_ADDR(0x00A0)
#define REG_FHCTL4_UPDNLMT       REG_ADDR(0x00A4)
#define REG_FHCTL4_DDS	         REG_ADDR(0x00A8)
#define REG_FHCTL4_DVFS	         REG_ADDR(0x00AC)
#define REG_FHCTL4_MON	         REG_ADDR(0x00B0)
#define REG_FHCTL5_CFG	         REG_ADDR(0x00B4)
#define REG_FHCTL5_UPDNLMT	     REG_ADDR(0x00B8)
#define REG_FHCTL5_DDS	         REG_ADDR(0x00BC)
#define REG_FHCTL5_DVFS	         REG_ADDR(0x00C0)
#define REG_FHCTL5_MON           REG_ADDR(0x00C4)
#define REG_FHCTL6_CFG	         REG_ADDR(0x00C8)
#define REG_FHCTL6_UPDNLMT	     REG_ADDR(0x00CC)
#define REG_FHCTL6_DDS	         REG_ADDR(0x00D0)
#define REG_FHCTL6_DVFS	         REG_ADDR(0x00D4)
#define REG_FHCTL6_MON           REG_ADDR(0x00D8)
#define REG_FHCTL7_CFG	         REG_ADDR(0x00DC)
#define REG_FHCTL7_UPDNLMT	     REG_ADDR(0x00E0)
#define REG_FHCTL7_DDS	         REG_ADDR(0x00E4)
#define REG_FHCTL7_DVFS	         REG_ADDR(0x00E8)
#define REG_FHCTL7_MON           REG_ADDR(0x00EC)

#define PLL_HP_CON0         (APMIXEDSYS_BASE + 0x0014)

#define ARMPLL_CON0         (APMIXEDSYS_BASE + 0x0200)
#define ARMPLL_CON1         (APMIXEDSYS_BASE + 0x0204)
#define ARMPLL_PWR_CON0     (APMIXEDSYS_BASE + 0x020C)

#define MAINPLL_CON0        (APMIXEDSYS_BASE + 0x0210)
#define MAINPLL_CON1        (APMIXEDSYS_BASE + 0x0214)
#define MAINPLL_PWR_CON0    (APMIXEDSYS_BASE + 0x021C)

#define MMPLL_CON0          (APMIXEDSYS_BASE + 0x0230)
#define MMPLL_CON1          (APMIXEDSYS_BASE + 0x0234)
#define MMPLL_PWR_CON0      (APMIXEDSYS_BASE + 0x023C)

#define MSDCPLL_CON0        (APMIXEDSYS_BASE + 0x0240)
#define MSDCPLL_CON1        (APMIXEDSYS_BASE + 0x0244)
#define MSDCPLL_PWR_CON0    (APMIXEDSYS_BASE + 0x024C)

#define VENCPLL_CON0        (DDRPHY_BASE + 0x800)
#define VENCPLL_CON1        (DDRPHY_BASE + 0x804)
#define VENCPLL_PWR_CON0    (DDRPHY_BASE + 0x80C)

#define TVDPLL_CON0         (APMIXEDSYS_BASE + 0x0250)
#define TVDPLL_CON1         (APMIXEDSYS_BASE + 0x0254)
#define TVDPLL_PWR_CON0     (APMIXEDSYS_BASE + 0x025C)

#endif


#define MT_FH_CLK_GEN		0

#define USER_DEFINE_SETTING_ID	1

static DEFINE_SPINLOCK(freqhopping_lock);

#if FULLY_VERSION_FHCTL
/* current DRAMC@mempll */
static unsigned int g_curr_dramc = 266;	/* default @266MHz ==> LPDDR2/DDR3 data rate 1066 */

/* mempll mode */
static unsigned int g_pll_mode;
#endif

#define PERCENT_TO_DDSLMT(dDS, pERCENT_M10)  ((dDS * pERCENT_M10 >> 5) / 100)
#if MT_FH_CLK_GEN
static unsigned int g_curr_clkgen = MT658X_FH_PLL_TOTAL_NUM + 1;	/* default clkgen ==> no clkgen output */
#endif

static unsigned char g_mempll_fh_table[8];

/* static unsigned int g_initialize = 0; */
/* static unsigned int g_clk_en = 0; */
unsigned int g_initialize;
unsigned int g_clk_en;

#ifndef PER_PROJECT_FH_SETTING

#define LOW_DRAMC_DDS		0x000E55EC
#define LOW_DRAMC_INT		57	/* 200 */
#define LOW_DRAMC_FRACTION	5612	/* 200 */
#define LOW_DRAMC		200	/* 233.5 */
#define LOW_DRAMC_FREQ		200000

/* TODO: fill in the default freq & corresponding setting_id */
static fh_pll_t g_fh_pll[MT658X_FH_PLL_TOTAL_NUM] = {	/* keep track the status of each PLL */
	{FH_FH_DISABLE, FH_PLL_ENABLE, 0, 0, 0},	/* ARMPLL   default SSC disable */
#if (MAINPLL_SSC)
	{FH_FH_ENABLE_SSC, FH_PLL_ENABLE, 0, 1612000, 0},	/* MAINPLL  default SSC disable */
#else
	{FH_FH_DISABLE, FH_PLL_ENABLE, 0, 1612000, 0},	/* MAINPLL  default SSC disable */
#endif
#if (MEMPLL_SSC)
	{FH_FH_ENABLE_SSC, FH_PLL_ENABLE, 0, 266000, 0},	/* MEMPLL   default SSC enable */
#else
	{FH_FH_DISABLE, FH_PLL_ENABLE, 0, 266000, 0},	/* MEMPLL   default SSC enable */
#endif
	{FH_FH_ENABLE_SSC, FH_PLL_ENABLE, 0, 1599000, 0},	/* MSDCPLL  default SSC enable */
	{FH_FH_ENABLE_SSC, FH_PLL_ENABLE, 0, 1188000, 0},	/* MMPLL   default SSC enable */
	{FH_FH_ENABLE_SSC, FH_PLL_ENABLE, 0, 1200000, 0},	/* VENCPLL  default SSC enable */
	/* 8127 FHCTL MB */
	{FH_FH_DISABLE, FH_PLL_ENABLE, 0, 1485000, 0},	/* TVDPLL  default SSC disable */
	/* 8127 FHCTL ME */
};



/* ARMPLL */
static const struct freqhopping_ssc mt_ssc_armpll_setting[MT_SSC_NR_PREDEFINE_SETTING] = {
	{0, 0, 0, 0, 0, 0},	/* Means disable */
	{0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},	/* Means User-Define */
	{0, 0, 0, 0, 0, 0}	/* EOF */
};

static const struct freqhopping_ssc mt_ssc_mainpll_setting[MT_SSC_NR_PREDEFINE_SETTING] = {
	{0, 0, 0, 0, 0, 0},	/* Means disable */
	{0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},	/* Means User-Define */
#if 0
	{1612000, 2, 7, 0, 4, 0xA8000},	/* 0~-4% */
#else
	{1612000, 0, 9, 0, 8, 0xA8000},	/* 0~-8% */
#endif
	{0, 0, 0, 0, 0, 0}	/* EOF */
};

static const struct freqhopping_ssc mt_ssc_mempll_setting[MT_SSC_NR_PREDEFINE_SETTING] = {
	{0, 0, 0, 0, 0, 0},	/* Means disable */
	{0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},	/* Means User-Define */
	/* 266MHz , 0.27us, 0.007812500 , 0 ~ -8%   range:0x00131A2D ~ 0x1256AD(0x00131A2D-(0x061C x 32)) */
	{266000, 2, 7, 0, 4, 0x0014030c},
	/* 200MHz , 0.27us, 0.007812500, 0 ~ -8% */
	{200000, 2, 7, 0, 4, 0x000f04b0},
	{0, 0, 0, 0, 0, 0}	/* EOF */
};

static const struct freqhopping_ssc mt_ssc_msdcpll_setting[MT_SSC_NR_PREDEFINE_SETTING] = {
	{0, 0, 0, 0, 0, 0},	/* Means disable */
	{0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},	/* Means User-Define */
	{1599000, 0, 9, 0, 8, 0xF6275},	/* 0~-8%   range:0xF6275 ~ 0xE2795(0xF6275-(0x09d7 x 32)) */
	{0, 0, 0, 0, 0, 0}	/* EOF */

};

static const struct freqhopping_ssc mt_ssc_mmpll_setting[MT_SSC_NR_PREDEFINE_SETTING] = {
	{0, 0, 0, 0, 0, 0},	/* Means disable */
	{0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},	/* Means User-Define */
	{1188000, 0, 9, 0, 8, 0x134000},	/* 0~-8%   range:0x134000 ~ 0x11B5C0 (0x134000-(0x0c52 x 32)) */
	{0, 0, 0, 0, 0, 0}	/* EOF */
};

static const struct freqhopping_ssc mt_ssc_vencpll_setting[MT_SSC_NR_PREDEFINE_SETTING] = {
	{0, 0, 0, 0, 0, 0},	/* Means disable */
	{0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},	/* Means User-Define */
	{1200000, 0, 9, 0, 4, 0xB6000},	/* 0~-4%  range:0xB6000 ~ 0xAEBA0 (0xB6000-(0x03a3 x 32)) */
	{0, 0, 0, 0, 0, 0}	/* EOF */
};

/* 8127 FHCTL MB */
static const struct freqhopping_ssc mt_ssc_tvdpll_setting[MT_SSC_NR_PREDEFINE_SETTING] = {
	{0, 0, 0, 0, 0, 0},	/* Means disable */
	{0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},	/* Means User-Define */
	{148500, 0, 9, 0, 4, 0xB6C4F},	/* 0~-4% */
	{0, 0, 0, 0, 0, 0}	/* EOF */
};

static const struct freqhopping_ssc mt_ssc_lvdspll_setting[MT_SSC_NR_PREDEFINE_SETTING] = {
	{0, 0, 0, 0, 0, 0},	/* Means disable */
	{0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},	/* Means User-Define */
	{150000, 0, 9, 0, 4, 0xB89D9},	/* 0~-4% */
	{0, 0, 0, 0, 0, 0}	/* EOF */
};

/* 8127 FHCTL ME */

static struct freqhopping_ssc mt_ssc_fhpll_userdefined[MT_FHPLL_MAX] = {
	{0, 1, 1, 2, 2, 0},	/* ARMPLL */
	{0, 1, 1, 2, 2, 0},	/* MAINPLL */
	{0, 1, 1, 2, 2, 0},	/* MEMPLL */
	{0, 1, 1, 2, 2, 0},	/* MSDCPLL */
	{0, 1, 1, 2, 2, 0},	/* MMPLL */
	{0, 1, 1, 2, 2, 0}	/* VENCPLL */
};

#else				/* PER_PROJECT_FH_SETTING */

PER_PROJECT_FH_SETTING
#endif				/* PER_PROJECT_FH_SETTING */
#define PLL_STATUS_ENABLE 1
#define PLL_STATUS_DISABLE 0
static void mt_fh_hal_default_conf(void)
{
	FH_MSG("EN: %s", __func__);
#if (MEMPLL_SSC)
	freqhopping_config(MT658X_FH_MEM_PLL, 266000, true);	/* Enable MEMPLL SSC */
#endif
	/*freqhopping_config(MT658X_FH_MM_PLL, 1188000, true);*/	/* TODO: test only */
	/* freqhopping_config(MT658X_FH_VENC_PLL, 1200000, true);*/	/* TODO: test only */
#if (MAINPLL_SSC)
	freqhopping_config(MT658X_FH_MAIN_PLL, 1612000, true);	/* TODO: test only */
#endif
	freqhopping_config(MT658X_FH_MSDC_PLL, 1599000, true);	/* TODO: test only */
}


static void mt_fh_hal_switch_register_to_FHCTL_control(enum FH_PLL_ID pll_id, int i_control)
{
	if (pll_id == MT658X_FH_ARM_PLL)	/* FHCTL0 */
		fh_set_field(PLL_HP_CON0, (0x1U << 0), i_control);
	else if (pll_id == MT658X_FH_MAIN_PLL)	/* FHCTL1 */
		fh_set_field(PLL_HP_CON0, (0x1U << 1), i_control);
	else if (pll_id == MT658X_FH_MEM_PLL)	/* FHCTL2 */
		fh_set_field(PLL_HP_CON0, (0x1U << 3), i_control);
	else if (pll_id == MT658X_FH_MSDC_PLL)	/* FHCTL3 */
		fh_set_field(PLL_HP_CON0, (0x1U << 4), i_control);
	else if (pll_id == MT658X_FH_MM_PLL)	/* FHCTL4 */
		fh_set_field(PLL_HP_CON0, (0x1U << 2), i_control);
	else if (pll_id == MT658X_FH_VENC_PLL)	/* FHCTL5 */
		fh_set_field(PLL_HP_CON0, (0x1U << 5), i_control);
	/* 8127 FHCTL MB */
	else if (pll_id == MT658X_FH_TVD_PLL)	/* FHCTL6 */
		fh_set_field(PLL_HP_CON0, (0x1U << 6), i_control);
	/* 8127 FHCTL ME */
}

static int mt_fh_hal_sync_ncpo_to_FHCTL_DDS(enum FH_PLL_ID pll_id)
{
	if (pll_id == MT658X_FH_ARM_PLL)	/* FHCTL0 */
		fh_write32(REG_FHCTL0_DDS, (fh_read32(ARMPLL_CON1) & 0x1FFFFF) | (1U << 31));
	else if (pll_id == MT658X_FH_MAIN_PLL)	/* FHCTL1 */
		fh_write32(REG_FHCTL1_DDS, (fh_read32(MAINPLL_CON1) & 0x1FFFFF) | (1U << 31));
	else if (pll_id == MT658X_FH_MEM_PLL)	/* FHCTL2 */
		fh_write32(REG_FHCTL2_DDS,
			   ((fh_read32(DDRPHY_BASE + 0x624) >> 11) & 0x1FFFFF) | (1U << 31));
	else if (pll_id == MT658X_FH_MSDC_PLL)	/* FHCTL3 */
		fh_write32(REG_FHCTL3_DDS, (fh_read32(MSDCPLL_CON1) & 0x1FFFFF) | (1U << 31));
	else if (pll_id == MT658X_FH_MM_PLL)	/* FHCTL4 */
		fh_write32(REG_FHCTL4_DDS, (fh_read32(MMPLL_CON1) & 0x1FFFFF) | (1U << 31));
	else if (pll_id == MT658X_FH_VENC_PLL)	/* FHCTL5 */
		fh_write32(REG_FHCTL5_DDS, (fh_read32(VENCPLL_CON1) & 0x1FFFFF) | (1U << 31));
	/* 8127 FHCTL MB */
	else if (pll_id == MT658X_FH_TVD_PLL)	/* FHCTL6 */
		fh_write32(REG_FHCTL6_DDS, (fh_read32(TVDPLL_CON1) & 0x1FFFFF) | (1U << 31));
	/* 8127 FHCTL ME */
	else {
		FH_MSG("Incorrect PLL");
		return 0;
	}
	return 1;
}

static void update_fhctl_status(const int pll_id, const int enable)
{
	unsigned int i = 0;
	int enabled_num = 0;
	static unsigned int pll_status[] = {
		PLL_STATUS_DISABLE,	/* ARMPLL */
		PLL_STATUS_DISABLE,	/* MAINPLL */
		PLL_STATUS_DISABLE,	/* MEMPLL */
		PLL_STATUS_DISABLE,	/* MSDCPLL */
		PLL_STATUS_DISABLE,	/* TVDPLL */
		PLL_STATUS_DISABLE,	/* LVDSPLL */
		PLL_STATUS_DISABLE	/* TVDPLL */
	};

	/* FH_MSG("PLL#%d ori status is %d, you hope to change to %d\n", pll_id, pll_status[pll_id], enable) ; */
	/* FH_MSG("PL%d:%d->%d", pll_id, pll_status[pll_id], enable) ; */
	if (pll_status[pll_id] == enable) {
		/* FH_MSG("no ch") ;//no change */
		return;
	}

	pll_status[pll_id] = enable;

	for (i = MT658X_FH_MINIMUMM_PLL; i <= MT658X_FH_MAXIMUMM_PLL; i++) {
		if (pll_status[i] == PLL_STATUS_ENABLE)
			enabled_num++;
	}

	/* FH_MSG("PLen#=%d",enabled_num) ; */

	if ((g_clk_en == 0) && (enabled_num >= 1)) {
		/* wait_for_sophie enable_clock_ext_locked(MT_CG_PERI1_FHCTL, "FREQHOP") ; */
		/* register change.                 enable_clock(MT_CG_PERI1_FHCTL, "FREQHOP") ; */
		g_clk_en = 1;
	} else if ((g_clk_en == 1) && (enabled_num == 0)) {
		/* wait_for_sophie disable_clock_ext_locked(MT_CG_PERI1_FHCTL, "FREQHOP") ; */
		/* register change.                 disable_clock(MT_CG_PERI1_FHCTL, "FREQHOP") ; */
		g_clk_en = 0;
	}
}



static int __mt_enable_freqhopping(unsigned int pll_id, const struct freqhopping_ssc *ssc_setting)
{
	/* unsigned int  pll_hp=0; */
	unsigned long flags = 0;

	/* FH_MSG("EN: en_fh"); */
	FH_MSG("EN: %s:: %x u: %x df: %d dt: %d dds:%x", __func__, ssc_setting->lowbnd,
	       ssc_setting->upbnd, ssc_setting->df, ssc_setting->dt, ssc_setting->dds);

	update_fhctl_status(pll_id, PLL_STATUS_ENABLE);
	mb();			/* update fhctl status */


	/* lock @ __freqhopping_ctrl_lock() */
	/* spin_lock_irqsave(&freqhopping_lock, flags); */

	g_fh_pll[pll_id].fh_status = FH_FH_ENABLE_SSC;


	/* TODO: should we check the following here ?? */
	/* if(unlikely(FH_PLL_STATUS_ENABLE == g_fh_pll[pll_id].fh_status)){ */
	/* Do nothing due to this not allowable flow */
	/* We shall DISABLE and then re-ENABLE for the new setting or another round */
	/* FH_MSG("ENABLE the same FH",pll_id); */
	/* WARN_ON(1); */
	/* spin_unlock_irqrestore(&freqhopping_lock, flags); */
	/* return 1; */
	/* }else { */

	local_irq_save(flags);

	/* Set the relative parameter registers (dt/df/upbnd/downbnd) */
	/* Enable the fh bit */
	fh_set_field(REG_FHCTL0_CFG + (0x14 * pll_id), FH_SFSTRX_DYS, ssc_setting->df);
	fh_set_field(REG_FHCTL0_CFG + (0x14 * pll_id), FH_SFSTRX_DTS, ssc_setting->dt);

	/* fh_write32(REG_FHCTL0_UPDNLMT+(0x14*pll_id), (((ssc_setting->lowbnd) << 16) | (ssc_setting->upbnd))); */
	/* FH_MSG("REG_FHCTL%d_UPDNLMT: 0x%08x", pll_id, fh_read32(REG_FHCTL2_UPDNLMT)); */


	/* fh_write32(REG_FHCTL0_DDS+(0x14*pll_id), (ssc_setting->dds)|(1U<<31)); */
	/* FH_MSG("FHCTL%d_DDS: 0x%08x", pll_id, (fh_read32(REG_FHCTL0_DDS+(pll_id*0x14))&0x1FFFFF)); */

	mt_fh_hal_sync_ncpo_to_FHCTL_DDS(pll_id);
	/* FH_MSG("FHCTL%d_DDS: 0x%08x", pll_id, (fh_read32(REG_FHCTL0_DDS+(pll_id*0x14))&0x1FFFFF)); */

	fh_write32(REG_FHCTL0_UPDNLMT + (pll_id * 0x14),
		   (PERCENT_TO_DDSLMT
		    ((fh_read32(REG_FHCTL0_DDS + (pll_id * 0x14)) & 0x1FFFFF),
		     ssc_setting->lowbnd) << 16));
	/* FH_MSG("REG_FHCTL%d_UPDNLMT: 0x%08x", pll_id, fh_read32(REG_FHCTL2_UPDNLMT)); */
	/* FH_MSG("ssc_setting->lowbnd: 0x%x    ssc_setting->upbnd: 0x%x", ssc_setting->lowbnd, ssc_setting->upbnd); */

	mt_fh_hal_switch_register_to_FHCTL_control(pll_id, 1);
	/* pll_hp = fh_read32(PLL_HP_CON0) | (1 << pll_id); */
	/* fh_write32( PLL_HP_CON0,  pll_hp ); */

	mb();			/* switch register */

	fh_set_field(REG_FHCTL0_CFG + (0x14 * pll_id), FH_FRDDSX_EN, 1);
	fh_set_field(REG_FHCTL0_CFG + (0x14 * pll_id), FH_FHCTLX_EN, 1);
	/* lock @ __freqhopping_ctrl_lock() */
	/* spin_unlock_irqrestore(&freqhopping_lock, flags); */

	local_irq_restore(flags);

	/* FH_MSG("Exit"); */
	return 0;
}

static int __mt_disable_freqhopping(unsigned int pll_id, const struct freqhopping_ssc *ssc_setting)
{
	unsigned long flags = 0;
/* unsigned int    pll_hp=0; */
	unsigned int fh_dds = 0;
	unsigned int pll_dds = 0;


	/* FH_MSG("EN: _dis_fh"); */

	/* lock @ __freqhopping_ctrl_lock() */
	/* spin_lock_irqsave(&freqhopping_lock, flags); */

	local_irq_save(flags);
	/* Set the relative registers */
	fh_set_field(REG_FHCTL0_CFG + (0x14 * pll_id), FH_FRDDSX_EN, 0);
	fh_set_field(REG_FHCTL0_CFG + (0x14 * pll_id), FH_FHCTLX_EN, 0);
	mb();			/* set the relative registers */
	local_irq_restore(flags);


	if (pll_id == MT658X_FH_MEM_PLL) {	/* for mempll */
		unsigned int i = 0;

		pll_dds = (fh_read32(DDRPHY_BASE + 0x624)) >> 11;
		fh_dds = (fh_read32(REG_FHCTL2_MON)) & 0x1FFFFF;

		FH_MSG(">p:f< %x:%x", pll_dds, fh_dds);

		while ((pll_dds != fh_dds) && (i < 100)) {

			if (unlikely(i > 100)) {
				FH_BUG_ON(i > 100);
				break;
			}

			udelay(10);
			fh_dds = (fh_read32(REG_FHCTL2_MON)) & 0x1FFFFF;
			i++;
		}

		FH_MSG("<p:f:i> %x:%x:%d", pll_dds, fh_dds, i);
	} else {
		/* TODO:  sync&wait DDS back to PLLCON1@other PLLs */
		/* FH_MSG("n-mempll"); */
	}


	local_irq_save(flags);
	mt_fh_hal_switch_register_to_FHCTL_control(pll_id, 0);
	/* pll_hp = fh_read32(PLL_HP_CON0) & ~(1 << pll_id); */
	/* fh_write32( (PLL_HP_CON0),  pll_hp ); */

	g_fh_pll[pll_id].fh_status = FH_FH_DISABLE;
	local_irq_restore(flags);

	/* lock @ __freqhopping_ctrl_lock() */
	/* spin_unlock_irqrestore(&freqhopping_lock, flags); */

	mb();			/* update fhctl status */
	update_fhctl_status(pll_id, PLL_STATUS_DISABLE);


	/* FH_MSG("Exit"); */

	return 0;
}


/* freq is in KHz, return at which number of entry in mt_ssc_xxx_setting[] */
static noinline int __freq_to_index(enum FH_PLL_ID pll_id, int freq)
{
	unsigned int retVal = 0;
	unsigned int i = 2;	/* 0 is disable, 1 is user defines, so start from 2 */

	/* FH_MSG("EN: %s , pll_id: %d, freq: %d",__func__,pll_id,freq); */
	/* FH_MSG("EN: , id: %d, f: %d",pll_id,freq); */

	switch (pll_id) {

		/* TODO: use Marco or something to make the code less redudant */
	case MT658X_FH_ARM_PLL:
		while (i < MT_SSC_NR_PREDEFINE_SETTING) {
			if (freq == mt_ssc_armpll_setting[i].freq) {
				retVal = i;
				break;
			}
			i++;
		}
		break;


	case MT658X_FH_MAIN_PLL:
		while (i < MT_SSC_NR_PREDEFINE_SETTING) {
			if (freq == mt_ssc_mainpll_setting[i].freq) {
				retVal = i;
				break;
			}
			i++;
		}
		break;

	case MT658X_FH_MEM_PLL:
		while (i < MT_SSC_NR_PREDEFINE_SETTING) {
			if (freq == mt_ssc_mempll_setting[i].freq) {
				retVal = i;
				break;
			}
			i++;
		}
		break;

	case MT658X_FH_MSDC_PLL:
		while (i < MT_SSC_NR_PREDEFINE_SETTING) {
			if (freq == mt_ssc_msdcpll_setting[i].freq) {
				retVal = i;
				break;
			}
			i++;
		}
		break;

	case MT658X_FH_MM_PLL:
		while (i < MT_SSC_NR_PREDEFINE_SETTING) {
			if (freq == mt_ssc_mmpll_setting[i].freq) {
				retVal = i;
				break;
			}
			i++;
		}
		break;

	case MT658X_FH_VENC_PLL:
		while (i < MT_SSC_NR_PREDEFINE_SETTING) {
			if (freq == mt_ssc_vencpll_setting[i].freq) {
				retVal = i;
				break;
			}
			i++;
		}
		break;

		/* 8127 FHCTL MB */
	case MT658X_FH_TVD_PLL:
		while (i < MT_SSC_NR_PREDEFINE_SETTING) {
			if (freq == mt_ssc_tvdpll_setting[i].freq) {
				retVal = i;
				break;
			}
			i++;
		}

		/* 8127 FHCTL ME */

		/* fallthrough */
	case MT658X_FH_PLL_TOTAL_NUM:
		FH_MSG("Error MT658X_FH_PLL_TOTAL_NUM!");
		break;


	};

	return retVal;
}

static int __freqhopping_ctrl(struct freqhopping_ioctl *fh_ctl, bool enable)
{
	const struct freqhopping_ssc *pSSC_setting = NULL;
	unsigned int ssc_setting_id = 0;
	int retVal = 1;

	/* FH_MSG("EN: _fh_ctrl %d:%d",fh_ctl->pll_id,enable); */
	/* FH_MSG("%s fhpll_id: %d, enable: %d",(enable)?"enable":"disable",fh_ctl->pll_id,enable); */

	/* Check the out of range of frequency hopping PLL ID */
	FH_BUG_ON(fh_ctl->pll_id > MT658X_FH_MAXIMUMM_PLL);
	FH_BUG_ON(fh_ctl->pll_id < MT658X_FH_MINIMUMM_PLL);

	if (fh_ctl->pll_id == MT658X_FH_MAIN_PLL)
		g_fh_pll[fh_ctl->pll_id].curr_freq = 1612000;
	else if (fh_ctl->pll_id == MT658X_FH_MEM_PLL)
		g_fh_pll[fh_ctl->pll_id].curr_freq = 266000;
	else if (fh_ctl->pll_id == MT658X_FH_MSDC_PLL)
		g_fh_pll[fh_ctl->pll_id].curr_freq = 1599000;
	else if (fh_ctl->pll_id == MT658X_FH_MM_PLL)
		g_fh_pll[fh_ctl->pll_id].curr_freq = 1188000;
	else if (fh_ctl->pll_id == MT658X_FH_VENC_PLL)
		g_fh_pll[fh_ctl->pll_id].curr_freq = 1200000;
	else if (fh_ctl->pll_id == MT658X_FH_ARM_PLL)
		g_fh_pll[fh_ctl->pll_id].curr_freq = 0;

	/* 8127 FHCTL MB */
	else if (fh_ctl->pll_id == MT658X_FH_TVD_PLL)
		g_fh_pll[fh_ctl->pll_id].curr_freq = 148500;
	/* 8127 FHCTL ME */
	else
		FH_BUG_ON("Incorrect pll_id.");

	if ((enable == true) && (g_fh_pll[fh_ctl->pll_id].fh_status == FH_FH_ENABLE_SSC)) {

		/* The FH already enabled @ this PLL */

		/* FH_MSG("re-en FH"); */

		/* disable FH first, will be enable later */
		__mt_disable_freqhopping(fh_ctl->pll_id, pSSC_setting);
	} else if ((enable == false) && (g_fh_pll[fh_ctl->pll_id].fh_status == FH_FH_DISABLE)) {

		/* The FH already been disabled @ this PLL, do nothing & return */

		/* FH_MSG("re-dis FH"); */
		retVal = 0;
		goto Exit;
	}

	/* ccyeh fh_status set @ __mt_enable_freqhopping() __mt_disable_freqhopping() */
	/* g_fh_pll[fh_ctl->pll_id].fh_status = enable?FH_FH_ENABLE_SSC:FH_FH_DISABLE; */

	if (enable == true) {	/* enable freq. hopping @ fh_ctl->pll_id */

		if (g_fh_pll[fh_ctl->pll_id].pll_status == FH_PLL_DISABLE) {
			/* FH_MSG("pll is dis"); */

			/* update the fh_status & don't really enable the SSC */
			g_fh_pll[fh_ctl->pll_id].fh_status = FH_FH_ENABLE_SSC;
			retVal = 0;
			goto Exit;
		} else {
			/* FH_MSG("pll is en"); */
			if (g_fh_pll[fh_ctl->pll_id].user_defined == true) {
				FH_MSG("use u-def");

				pSSC_setting = &mt_ssc_fhpll_userdefined[fh_ctl->pll_id];
				g_fh_pll[fh_ctl->pll_id].setting_id = USER_DEFINE_SETTING_ID;
			} else {
				/* FH_MSG("n-user def"); */

				if (g_fh_pll[fh_ctl->pll_id].curr_freq != 0) {
					ssc_setting_id = g_fh_pll[fh_ctl->pll_id].setting_id =
					    __freq_to_index(fh_ctl->pll_id,
							    g_fh_pll[fh_ctl->pll_id].curr_freq);
				} else {
					ssc_setting_id = 0;
				}


				/* FH_MSG("sid %d",ssc_setting_id); */
				if (ssc_setting_id == 0) {
					FH_MSG("!!! No corresponding setting found !!!");

					/* just disable FH & exit */
					__mt_disable_freqhopping(fh_ctl->pll_id, pSSC_setting);
					goto Exit;
				}

				switch (fh_ctl->pll_id) {
				case MT658X_FH_MAIN_PLL:
					FH_BUG_ON(ssc_setting_id >
						  (sizeof(mt_ssc_mainpll_setting) /
						   sizeof(struct freqhopping_ssc)));
					pSSC_setting = &mt_ssc_mainpll_setting[ssc_setting_id];
					break;
				case MT658X_FH_ARM_PLL:
					FH_BUG_ON(ssc_setting_id >
						  (sizeof(mt_ssc_armpll_setting) /
						   sizeof(struct freqhopping_ssc)));
					pSSC_setting = &mt_ssc_armpll_setting[ssc_setting_id];
					break;
				case MT658X_FH_MSDC_PLL:
					FH_BUG_ON(ssc_setting_id >
						  (sizeof(mt_ssc_msdcpll_setting) /
						   sizeof(struct freqhopping_ssc)));
					pSSC_setting = &mt_ssc_msdcpll_setting[ssc_setting_id];
					break;
				case MT658X_FH_MM_PLL:
					FH_BUG_ON(ssc_setting_id >
						  (sizeof(mt_ssc_mmpll_setting) /
						   sizeof(struct freqhopping_ssc)));
					pSSC_setting = &mt_ssc_mmpll_setting[ssc_setting_id];
					break;
				case MT658X_FH_VENC_PLL:
					FH_BUG_ON(ssc_setting_id >
						  (sizeof(mt_ssc_vencpll_setting) /
						   sizeof(struct freqhopping_ssc)));
					pSSC_setting = &mt_ssc_vencpll_setting[ssc_setting_id];
					break;
				case MT658X_FH_MEM_PLL:
					FH_BUG_ON(ssc_setting_id >
						  (sizeof(mt_ssc_mempll_setting) /
						   sizeof(struct freqhopping_ssc)));
					pSSC_setting = &mt_ssc_mempll_setting[ssc_setting_id];
					break;
					/* 8127 FHCTL MB */
				case MT658X_FH_TVD_PLL:
					FH_BUG_ON(ssc_setting_id >
						  (sizeof(mt_ssc_tvdpll_setting) /
						   sizeof(struct freqhopping_ssc)));
					pSSC_setting = &mt_ssc_tvdpll_setting[ssc_setting_id];
					break;
					/* 8127 FHCTL ME */
				}
			}	/* user defined */

			if (pSSC_setting == NULL) {
				FH_MSG("!!! pSSC_setting is NULL !!!");
				/* just disable FH & exit */
				__mt_disable_freqhopping(fh_ctl->pll_id, pSSC_setting);
				goto Exit;
			}

			if (__mt_enable_freqhopping(fh_ctl->pll_id, pSSC_setting) == 0) {
				retVal = 0;
				FH_MSG("en ok");
			} else {
				FH_MSG("__mt_enable_freqhopping fail.");
			}
		}

	} else {		/* disable req. hopping @ fh_ctl->pll_id */
		if (__mt_disable_freqhopping(fh_ctl->pll_id, pSSC_setting) == 0) {
			retVal = 0;
			FH_MSG("dis ok");
		} else {
			FH_MSG("__mt_disable_freqhopping fail.");
		}
	}

Exit:

	/* FH_MSG("Exit"); */
	return retVal;
}





#if FULLY_VERSION_FHCTL
/* mempll 266->293MHz using FHCTL */
static int mt_h2oc_mempll(void)
{
	unsigned long flags;
	unsigned int fh_dds = 0;
	unsigned int pll_dds = 0;
	unsigned int i = 0;

	FH_MSG("EN: %s:%d", __func__, g_curr_dramc);

	/* Please note that the DFS can・t be applied in systems with DDR3 DRAM!
	 * It・s only for the mobile DRAM (LPDDR*)!
	 */

	if ((get_dram_type() != LPDDR2) && (get_dram_type() != LPDDR3)) {
		FH_MSG("Not LPDDR*");
		return -1;
	}

	if (g_curr_dramc != 266) {
		FH_MSG("g_curr_dramc != 266)");
		return -1;
	}

	FH_MSG("dds: %x", (fh_read32(DDRPHY_BASE + 0x624) >> 11));

	if ((fh_read32(DDRPHY_BASE + 0x624) >> 11) != 0x131a2d) {
		FH_BUG_ON((fh_read32(DDRPHY_BASE + 0x624) >> 11) != 0x131a2d);
		FH_MSG("DDS != 0x131a2d");
		return -1;
	}
	/* TODO: provelock issue spin_lock(&freqhopping_lock); */
	spin_lock_irqsave(&freqhopping_lock, flags);

	/* disable SSC when OC */
	__mt_disable_freqhopping(2, NULL);

	if (g_fh_pll[MT658X_FH_MEM_PLL].fh_status == FH_FH_ENABLE_SSC) {	/* only when SSC is enable */

		/* Turn off MEMPLL hopping */
		fh_write32(REG_FHCTL2_CFG, 0x70700000);

		/* udelay(800); */

		pll_dds = (fh_read32(DDRPHY_BASE + 0x624)) >> 11;
		fh_dds = (fh_read32(REG_FHCTL2_MON)) & 0x1FFFFF;

		FH_MSG(">p:f< %x:%x", pll_dds, fh_dds);

		while ((pll_dds != fh_dds) && (i < 100)) {

			if (unlikely(i > 100)) {
				FH_BUG_ON(i > 100);
				break;
			}

			udelay(10);
			fh_dds = (fh_read32(REG_FHCTL2_MON)) & 0x1FFFFF;
			i++;
		}

		FH_MSG("<p:f:i> %x:%x:%d", pll_dds, fh_dds, i);
	}

	FH_MSG("=> 0x%x", fh_read32(REG_FHCTL2_MON));

	/* TODO: provelock issue local_irq_save(flags); */

	/* --------------------------------------------- */
	/* Overwrite MEMPLL NCPO setting in hopping SRAM */
	/* --------------------------------------------- */
	/* Turn on SRAM CE, point to SRAM address of MEMPLL NCPO value 0 */
	fh_write32(REG_FHSRAM_CON, (0x1 << 9 | 0x80));

	/* Target Freq. : NCPO INT : 84.165 (26MHz*84.165 = 2188.29 MHz, DRAMC = 293MHz) */
	/* INT : 84 => 0x54 << 14 */
	/* FRACTION : 0.165 => 0x0A8F */
	fh_write32(REG_FHSRAM_WR, ((0x54 << 14) | 0x0A8F));

	/* ------------------------------------------------- */
	/* Turn on MEMPLL hopping and set slope < 2.5MHz/us */
	/* ------------------------------------------------- */
	/* Only turn on 2G1 hopping */
	fh_write32(REG_FHCTL_CFG, (1 << 8));

	/* Use 2G1 hopping table to DFS */
	/* SW need fill 64・b1 in 2G1 DRAM address (channel number = 0) */
	fh_write32(REG_FHDMA_2G1BASE, __pa(g_mempll_fh_table));

	/* sync NCPO value */
	fh_write32(REG_FHCTL2_DDS, (fh_read32(DDRPHY_BASE + 0x624) >> 11) | (1U << 31));

	mt_fh_hal_switch_register_to_FHCTL_control(MT658X_FH_MEM_PLL, 1);
	/* fh_write32( PLL_HP_CON0,  (fh_read32(PLL_HP_CON0) | (1 << 2)) ); */

	mb();			/* sync NCPO value */

	/* Configure hopping slope */
	/* sfstr_dts[3:0] = 4・d0(0.27us) */
	/* sfstr_dys[3:0] = 4・d9(0.023437500) */
	/* slope = 26MHz * 0.007812500 / 0.27us = 0.75MHz/us < 2.5MHz/us */
	fh_write32(REG_FHCTL2_CFG, ((0 << 24) | (7 << 28) | 5));

	/* ------------------------------------------------- */
	/* Trigger 2G1 Hopping on channel number 0 */
	/* ------------------------------------------------- */
	fh_write32(REG_FHCTL_2G1_CH, 0);

	g_curr_dramc = 293;

	/* TODO: provelock issue local_irq_restore(flags); */

	FH_MSG("-MON- 0x%x", fh_read32(REG_FHCTL2_MON));

	/* TODO: provelock issue local_irq_save(flags); */
	/* clear NCPO */
	fh_write32(DDRPHY_BASE + 0x624, (fh_read32(DDRPHY_BASE + 0x624) & 0x7FF));
	/* set NCPO */
	fh_write32(DDRPHY_BASE + 0x624,
		   (fh_read32(DDRPHY_BASE + 0x624) | (((0x54 << 14) | 0x0A8F) << 11)));

	/* sync NCPO value */
	fh_write32(REG_FHCTL2_DDS, (fh_read32(DDRPHY_BASE + 0x624) >> 11) | (1U << 31));

	/* Turn off MEMPLL hopping */
	fh_write32(REG_FHCTL2_CFG, 0x70700000);

	/* TODO: provelock issue local_irq_restore(flags); */

	if (g_fh_pll[MT658X_FH_MEM_PLL].fh_status == FH_FH_ENABLE_SSC) {
		FH_MSG("to SSC");

	} else {
		/* switch back to APMIXED */
		FH_MSG("to APM");
		mt_fh_hal_switch_register_to_FHCTL_control(MT658X_FH_MEM_PLL, 0);
		/* fh_write32( PLL_HP_CON0,  (fh_read32(PLL_HP_CON0) & ~(1 << 2)) ); */
	}

	FH_MSG("+MON+ 0x%x", fh_read32(REG_FHCTL2_MON));

	g_fh_pll[MT658X_FH_MEM_PLL].curr_freq = 293000;
	g_fh_pll[MT658X_FH_MEM_PLL].pll_status = FH_PLL_ENABLE;

	/* TODO: provelock issue spin_unlock(&freqhopping_lock); */
	spin_unlock_irqrestore(&freqhopping_lock, flags);

	freqhopping_config(MT658X_FH_MEM_PLL, 293000, true);	/* update freq. */

	return 0;
}

/* mempll 293->266MHz using FHCTL */
static int mt_oc2h_mempll(void)
{
	unsigned long flags;
	unsigned int fh_dds = 0;
	unsigned int pll_dds = 0;
	unsigned int i = 0;

	FH_MSG("EN: %s:%d", __func__, g_curr_dramc);

	/* Please note that the DFS can・t be applied in systems with DDR3 DRAM!
	 * It・s only for the mobile DRAM (LPDDR*)!
	 */

	if ((get_dram_type() != LPDDR2) && (get_dram_type() != LPDDR3)) {
		FH_MSG("Not LPDDR*");
		return -1;
	}

	if (g_curr_dramc != 293) {
		FH_MSG("g_curr_dramc != 293)");
		return -1;
	}

	FH_MSG("dds: %x", (fh_read32(DDRPHY_BASE + 0x624) >> 11));

	if ((fh_read32(DDRPHY_BASE + 0x624) >> 11) != 0x150A8F) {
		FH_BUG_ON((fh_read32(DDRPHY_BASE + 0x624) >> 11) != 0x150A8F);
		FH_MSG("DDS != 0x150A8F");
		return 0;
	}
	/* TODO: provelock issue spin_lock(&freqhopping_lock); */
	spin_lock_irqsave(&freqhopping_lock, flags);

	if (g_fh_pll[MT658X_FH_MEM_PLL].fh_status == FH_FH_ENABLE_SSC) {	/* only when SSC is enable */

		/* Turn off MEMPLL hopping */
		fh_write32(REG_FHCTL2_CFG, 0x70700000);

		/* udelay(800); */

		pll_dds = (fh_read32(DDRPHY_BASE + 0x624)) >> 11;
		fh_dds = (fh_read32(REG_FHCTL2_MON)) & 0x1FFFFF;

		FH_MSG(">p:f< %x:%x", pll_dds, fh_dds);

		while ((pll_dds != fh_dds) && (i < 100)) {

			if (unlikely(i > 100)) {
				FH_BUG_ON(i > 100);
				break;
			}

			udelay(10);
			fh_dds = (fh_read32(REG_FHCTL2_MON)) & 0x1FFFFF;
			i++;
		}

		FH_MSG("<p:f:i> %x:%x:%d", pll_dds, fh_dds, i);
	}


	FH_MSG("=> 0x%x", fh_read32(REG_FHCTL2_MON));

	/* TODO: provelock issue local_irq_save(flags); */

	/* --------------------------------------------- */
	/* Overwrite MEMPLL NCPO setting in hopping SRAM */
	/* --------------------------------------------- */
	/* Turn on SRAM CE, point to SRAM address of MEMPLL NCPO value 0 */
	fh_write32(REG_FHSRAM_CON, (0x1 << 9 | 0x80));

	/* Target Freq. : NCPO INT : 76.409 (26MHz*76.409 = 1986.63MHz, DRAMC = 266MHz) */
	/* INT : 76 => 0x4C << 14 */
	/* FRACTION : 0.409 => 0x1A2D */
	fh_write32(REG_FHSRAM_WR, ((0x4C << 14) | 0x1A2D));


	/* ------------------------------------------------- */
	/* Turn on MEMPLL hopping and set slope < 2.5MHz/us */
	/* ------------------------------------------------- */
	/* Only turn on 2G1 hopping */
	fh_write32(REG_FHCTL_CFG, (1 << 8));

	/* Use 2G1 hopping table to DFS */
	/* SW need fill 64・b1 in 2G1 DRAM address (channel number = 0) */
	fh_write32(REG_FHDMA_2G1BASE, __pa(g_mempll_fh_table));

	/* sync NCPO value */
	fh_write32(REG_FHCTL2_DDS, (fh_read32(DDRPHY_BASE + 0x624) >> 11) | (1U << 31));

	mt_fh_hal_switch_register_to_FHCTL_control(MT658X_FH_MEM_PLL, 1);
	/* fh_write32( PLL_HP_CON0,  (fh_read32(PLL_HP_CON0) | (1 << 2)) ); */

	mb();			/* sync NCPO value */

	/* Configure hopping slope */
	/* sfstr_dts[3:0] = 4・d0(0.27us) */
	/* sfstr_dys[3:0] = 4・d9(0.023437500) */
	/* slope = 26MHz * 0.007812500 / 0.27us = 0.75MHz/us < 2.5MHz/us */
	fh_write32(REG_FHCTL2_CFG, ((0 << 24) | (7 << 28) | 5));

	/* ------------------------------------------------- */
	/* Trigger 2G1 Hopping on channel number 0 */
	/* ------------------------------------------------- */
	fh_write32(REG_FHCTL_2G1_CH, 0);

	g_curr_dramc = 266;

	/* TODO: provelock issue local_irq_restore(flags); */

	FH_MSG("-MON- 0x%x", fh_read32(REG_FHCTL2_MON));

	/* TODO: provelock issue local_irq_save(flags); */
	/* clear NCPO */
	fh_write32(DDRPHY_BASE + 0x624, (fh_read32(DDRPHY_BASE + 0x624) & 0x7FF));
	/* set NCPO */
	fh_write32(DDRPHY_BASE + 0x624,
		   (fh_read32(DDRPHY_BASE + 0x624) | (((0x4C << 14) | 0x1A2D) << 11)));

	/* sync NCPO value */
	fh_write32(REG_FHCTL2_DDS, (fh_read32(DDRPHY_BASE + 0x624) >> 11) | (1U << 31));

	/* Turn off MEMPLL hopping */
	fh_write32(REG_FHCTL2_CFG, 0x70700000);
	/* TODO: provelock issue local_irq_restore(flags); */

	if (g_fh_pll[MT658X_FH_MEM_PLL].fh_status == FH_FH_ENABLE_SSC) {
		FH_MSG("to SSC");

	} else {
		/* switch back to APMIXED */
		FH_MSG("to APM");
		mt_fh_hal_switch_register_to_FHCTL_control(MT658X_FH_MEM_PLL, 0);
		/* fh_write32( PLL_HP_CON0,  (fh_read32(PLL_HP_CON0) & ~(1 << 2)) ); */
	}

	FH_MSG("+MON+ 0x%x", fh_read32(REG_FHCTL2_MON));
	g_fh_pll[MT658X_FH_MEM_PLL].curr_freq = 266000;
	g_fh_pll[MT658X_FH_MEM_PLL].pll_status = FH_PLL_ENABLE;

	/* TODO: provelock issue spin_unlock(&freqhopping_lock); */
	spin_unlock_irqrestore(&freqhopping_lock, flags);

	freqhopping_config(MT658X_FH_MEM_PLL, 266000, true);	/* update freq. */

	return 0;
}

/* mempll 200->266MHz using FHCTL */
static int mt_fh_hal_l2h_mempll(void)
{				/* mempll low to high (200->266MHz) */
	unsigned long flags;
	unsigned int fh_dds = 0;
	unsigned int pll_dds = 0;
	unsigned int i = 0;

	FH_MSG("EN: %s:%d", __func__, g_curr_dramc);

	/* Please note that the DFS can・t be applied in systems with DDR3 DRAM!
	* It・s only for the mobile DRAM (LPDDR*)!
	*/

	if ((get_dram_type() != LPDDR2) && (get_dram_type() != LPDDR3)) {
		FH_MSG("Not LPDDR*");
		return -1;
	}

	if (g_curr_dramc == 266)
		return -1;

	FH_MSG("dds: %x", (fh_read32(DDRPHY_BASE + 0x624) >> 11));

	if ((fh_read32(DDRPHY_BASE + 0x624) >> 11) != LOW_DRAMC_DDS) {
		FH_BUG_ON((fh_read32(DDRPHY_BASE + 0x624) >> 11) != LOW_DRAMC_DDS);
		FH_MSG("DDS != 0x%X", LOW_DRAMC_DDS);
		return 0;
	}
	/* TODO: provelock issue spin_lock(&freqhopping_lock); */
	spin_lock_irqsave(&freqhopping_lock, flags);

	if (g_fh_pll[MT658X_FH_MEM_PLL].fh_status == FH_FH_ENABLE_SSC) {	/* only when SSC is enable */

		/* Turn off MEMPLL hopping */
		fh_write32(REG_FHCTL2_CFG, 0x70700000);

		/* udelay(800); */

		pll_dds = (fh_read32(DDRPHY_BASE + 0x624)) >> 11;
		fh_dds = (fh_read32(REG_FHCTL2_MON)) & 0x1FFFFF;

		FH_MSG(">p:f< %x:%x", pll_dds, fh_dds);

		while ((pll_dds != fh_dds) && (i < 100)) {

			if (unlikely(i > 100)) {
				FH_BUG_ON(i > 100);
				break;
			}

			udelay(10);
			fh_dds = (fh_read32(REG_FHCTL2_MON)) & 0x1FFFFF;
			i++;
		}

		FH_MSG("<p:f:i> %x:%x:%d", pll_dds, fh_dds, i);
	}


	FH_MSG("=> 0x%x", fh_read32(REG_FHCTL2_MON));

	/* TODO: provelock issue local_irq_save(flags); */

	/* --------------------------------------------- */
	/* Overwrite MEMPLL NCPO setting in hopping SRAM */
	/* --------------------------------------------- */
	/* Turn on SRAM CE, point to SRAM address of MEMPLL NCPO value 0 */
	fh_write32(REG_FHSRAM_CON, (0x1 << 9 | 0x80));

	/* Target Freq. : NCPO INT : 76.409 (26MHz*76.409 = 1986.63MHz, DRAMC = 266MHz) */
	/* INT : 76 => 0x4C << 14 */
	/* FRACTION : 0.409 => 0x1A2D */
	fh_write32(REG_FHSRAM_WR, ((0x4C << 14) | 0x1A2D));


	/* ------------------------------------------------- */
	/* Turn on MEMPLL hopping and set slope < 2.5MHz/us */
	/* ------------------------------------------------- */
	/* Only turn on 2G1 hopping */
	fh_write32(REG_FHCTL_CFG, (1 << 8));

	/* Use 2G1 hopping table to DFS */
	/* SW need fill 64・b1 in 2G1 DRAM address (channel number = 0) */
	fh_write32(REG_FHDMA_2G1BASE, __pa(g_mempll_fh_table));

	/* sync NCPO value */
	fh_write32(REG_FHCTL2_DDS, (fh_read32(DDRPHY_BASE + 0x624) >> 11) | (1U << 31));
	mt_fh_hal_switch_register_to_FHCTL_control(MT658X_FH_MEM_PLL, 1);
	/* fh_write32( PLL_HP_CON0,  (fh_read32(PLL_HP_CON0) | (1 << 2)) ); */

	mb();			/* sync NCPO value */

	/* Configure hopping slope */
	/* sfstr_dts[3:0] = 4・d0(0.27us) */
	/* sfstr_dys[3:0] = 4・d9(0.023437500) */
	/* slope = 26MHz * 0.007812500 / 0.27us = 0.75MHz/us < 2.5MHz/us */
	fh_write32(REG_FHCTL2_CFG, ((0 << 24) | (7 << 28) | 5));

	/* ------------------------------------------------- */
	/* Trigger 2G1 Hopping on channel number 0 */
	/* ------------------------------------------------- */
	fh_write32(REG_FHCTL_2G1_CH, 0);

	g_curr_dramc = 266;

	/* TODO: provelock issue local_irq_restore(flags); */

	FH_MSG("-MON- 0x%x", fh_read32(REG_FHCTL2_MON));

	/* TODO: provelock issue local_irq_save(flags); */
	/* clear NCPO */
	fh_write32(DDRPHY_BASE + 0x624, (fh_read32(DDRPHY_BASE + 0x624) & 0x7FF));
	/* set NCPO */
	fh_write32(DDRPHY_BASE + 0x624,
		   (fh_read32(DDRPHY_BASE + 0x624) | (((0x4C << 14) | 0x1A2D) << 11)));

	/* sync NCPO value */
	fh_write32(REG_FHCTL2_DDS, (fh_read32(DDRPHY_BASE + 0x624) >> 11) | (1U << 31));

	/* Turn off MEMPLL hopping */
	fh_write32(REG_FHCTL2_CFG, 0x70700000);
	/* TODO: provelock issue local_irq_restore(flags); */

	if (g_fh_pll[MT658X_FH_MEM_PLL].fh_status == FH_FH_ENABLE_SSC) {
		FH_MSG("to SSC");

	} else {
		/* switch back to APMIXED */
		FH_MSG("to APM");
		mt_fh_hal_switch_register_to_FHCTL_control(MT658X_FH_MEM_PLL, 0);
		/* fh_write32( PLL_HP_CON0,  (fh_read32(PLL_HP_CON0) & ~(1 << 2)) ); */
	}

	FH_MSG("+MON+ 0x%x", fh_read32(REG_FHCTL2_MON));
	g_fh_pll[MT658X_FH_MEM_PLL].curr_freq = 266000;
	g_fh_pll[MT658X_FH_MEM_PLL].pll_status = FH_PLL_ENABLE;

	/* TODO: provelock issue spin_unlock(&freqhopping_lock); */
	spin_unlock_irqrestore(&freqhopping_lock, flags);

	freqhopping_config(MT658X_FH_MEM_PLL, 266000, true);	/* update freq. */

	return 0;
}

/* mempll 266->200MHz using FHCTL */
static int mt_fh_hal_h2l_mempll(void)
{				/* mempll low to high (200->266MHz) */
	unsigned long flags;
	unsigned int fh_dds = 0;
	unsigned int pll_dds = 0;
	unsigned int i = 0;

	FH_MSG("EN: %s:%d", __func__, g_curr_dramc);

	/* Please note that the DFS can・t be applied in systems with DDR3 DRAM!
	 * It・s only for the mobile DRAM (LPDDR*)!
	 */

	if ((get_dram_type() != LPDDR2) && (get_dram_type() != LPDDR3)) {
		FH_MSG("Not LPDDR*");
		return -1;
	}

	if (g_curr_dramc == LOW_DRAMC)
		return -1;

	FH_MSG("dds: %x", (fh_read32(DDRPHY_BASE + 0x624) >> 11));

	if ((fh_read32(DDRPHY_BASE + 0x624) >> 11) != 0x131a2d) {
		FH_BUG_ON((fh_read32(DDRPHY_BASE + 0x624) >> 11) != 0x131a2d);
		FH_MSG("DDS != 0x131a2d");
		return 0;
	}
	/* TODO: provelock issue spin_lock(&freqhopping_lock); */
	spin_lock_irqsave(&freqhopping_lock, flags);

	if (g_fh_pll[MT658X_FH_MEM_PLL].fh_status == FH_FH_ENABLE_SSC) {	/* only when SSC is enable */

		/* Turn off MEMPLL hopping */
		fh_write32(REG_FHCTL2_CFG, 0x70700000);

		/* udelay(800); */

		pll_dds = (fh_read32(DDRPHY_BASE + 0x624)) >> 11;
		fh_dds = (fh_read32(REG_FHCTL2_MON)) & 0x1FFFFF;

		FH_MSG(">p:f< %x:%x", pll_dds, fh_dds);

		while ((pll_dds != fh_dds) && (i < 100)) {

			if (unlikely(i > 100)) {
				FH_BUG_ON(i > 100);
				break;
			}

			udelay(10);
			fh_dds = (fh_read32(REG_FHCTL2_MON)) & 0x1FFFFF;
			i++;
		}

		FH_MSG("<p:f:i> %x:%x:%d", pll_dds, fh_dds, i);
	}

	FH_MSG("=> 0x%x", fh_read32(REG_FHCTL2_MON));

	/* TODO: provelock issue local_irq_save(flags); */

	/* --------------------------------------------- */
	/* Overwrite MEMPLL NCPO setting in hopping SRAM */
	/* --------------------------------------------- */
	/* Turn on SRAM CE, point to SRAM address of MEMPLL NCPO value 0 */
	fh_write32(REG_FHSRAM_CON, (0x1 << 9 | 0x80));

	/* >>>Example<<< */
	/* Target Freq. : NCPO INT : 59.89 (26MHz*59.89 = 1557.14 MHz, DRAMC = 208.5MHz) */
	/* INT : 59 => 0x3B << 14 */
	/* FRACTION : 0.89 => 0x38F5 */
	fh_write32(REG_FHSRAM_WR, LOW_DRAMC_DDS);


	/* ------------------------------------------------- */
	/* Turn on MEMPLL hopping and set slope < 2.5MHz/us */
	/* ------------------------------------------------- */
	/* Only turn on 2G1 hopping */
	fh_write32(REG_FHCTL_CFG, (1 << 8));

	/* Use 2G1 hopping table to DFS */
	/* SW need fill 64・b1 in 2G1 DRAM address (channel number = 0) */
	fh_write32(REG_FHDMA_2G1BASE, __pa(g_mempll_fh_table));

	/* sync NCPO value */
	fh_write32(REG_FHCTL2_DDS, (fh_read32(DDRPHY_BASE + 0x624) >> 11) | (1U << 31));
	mt_fh_hal_switch_register_to_FHCTL_control(MT658X_FH_MEM_PLL, 1);
	/* fh_write32( PLL_HP_CON0,  (fh_read32(PLL_HP_CON0) | (1 << 2)) ); */

	mb();			/* sync NCPO value */

	/* Configure hopping slope */
	/* sfstr_dts[3:0] = 4・d0(0.27us) */
	/* sfstr_dys[3:0] = 4・d9(0.023437500) */
	/* slope = 26MHz * 0.007812500 / 0.27us = 0.75MHz/us < 2.5MHz/us */
	fh_write32(REG_FHCTL2_CFG, ((0 << 24) | (7 << 28) | 5));

	/* ------------------------------------------------- */
	/* Trigger 2G1 Hopping on channel number 0 */
	/* ------------------------------------------------- */
	fh_write32(REG_FHCTL_2G1_CH, 0);

	g_curr_dramc = LOW_DRAMC;

	/* TODO: provelock issue local_irq_restore(flags); */

	FH_MSG("-MON- 0x%x", fh_read32(REG_FHCTL2_MON));

	/* TODO: provelock issue local_irq_save(flags); */

	/* clear NCPO */
	fh_write32(DDRPHY_BASE + 0x624, (fh_read32(DDRPHY_BASE + 0x624) & 0x7FF));
	/* set NCPO */
	fh_write32(DDRPHY_BASE + 0x624,
		   (fh_read32(DDRPHY_BASE + 0x624) |
		    (((LOW_DRAMC_INT << 14) | LOW_DRAMC_FRACTION) << 11)));

	/* sync NCPO value */
	fh_write32(REG_FHCTL2_DDS, (fh_read32(DDRPHY_BASE + 0x624) >> 11) | (1U << 31));

	/* Turn off MEMPLL hopping */
	fh_write32(REG_FHCTL2_CFG, 0x70700000);

	/* TODO: provelock issue local_irq_restore(flags); */

	if (g_fh_pll[MT658X_FH_MEM_PLL].fh_status == FH_FH_ENABLE_SSC) {
		FH_MSG("to SSC");

	} else {
		/* switch back to APMIXED */
		FH_MSG("to APM");
		mt_fh_hal_switch_register_to_FHCTL_control(MT658X_FH_MEM_PLL, 0);
		/* fh_write32( PLL_HP_CON0,  (fh_read32(PLL_HP_CON0) & ~(1 << 2)) ); */
	}

	FH_MSG("+MON+ 0x%x", fh_read32(REG_FHCTL2_MON));

	g_fh_pll[MT658X_FH_MEM_PLL].curr_freq = LOW_DRAMC_FREQ;
	g_fh_pll[MT658X_FH_MEM_PLL].pll_status = FH_PLL_ENABLE;

	/* TODO: provelock issue spin_unlock(&freqhopping_lock); */
	spin_unlock_irqrestore(&freqhopping_lock, flags);

	freqhopping_config(MT658X_FH_MEM_PLL, LOW_DRAMC_FREQ, true);	/* update freq. */

	return 0;
}
#else
/* mempll 200->266MHz using FHCTL */
static int mt_fh_hal_l2h_mempll(void)
{
	return 0;
}


/* mempll 266->200MHz using FHCTL */
static int mt_fh_hal_h2l_mempll(void)
{
	return 0;
}
#endif

static int mt_fh_hal_dvfs(enum FH_PLL_ID pll_id, unsigned int dds_value)
{
	unsigned int fh_dds = 0;
	unsigned int i = 0;
	unsigned long flags = 0;
	unsigned int ilog = 1;

	FH_MSG("EN: %s:", __func__);

	local_irq_save(flags);

	/* 1. sync ncpo to DDS of FHCTL */
	if (!(mt_fh_hal_sync_ncpo_to_FHCTL_DDS(pll_id)))
		return 0;
	if (ilog) {
		FH_MSG("1. sync ncpo to DDS of FHCTL");
		FH_MSG("FHCTL%d_DDS: 0x%08x", pll_id,
		       (fh_read32(REG_FHCTL0_DDS + (pll_id * 0x14)) & 0x1FFFFF));
	}
	/* 2. enable DVFS and Hopping control */
	fh_set_field((REG_FHCTL0_CFG + (pll_id * 0x14)), FH_SFSTRX_EN, 1);	/* enable dvfs mode */
	fh_set_field((REG_FHCTL0_CFG + (pll_id * 0x14)), FH_FHCTLX_EN, 1);	/* enable hopping control */

	if (ilog)
		FH_MSG("2. enable DVFS and Hopping control");
	/* 3. switch to hopping control */
	mt_fh_hal_switch_register_to_FHCTL_control(pll_id, 1);
	mb();			/* switch to hopping control */

	if (ilog) {
		FH_MSG("3. switch to hopping control");
		FH_MSG("PLL_HP_CON0: 0x%08x", (fh_read32(PLL_HP_CON0) & 0x3F));
	}
	/* 4. set DFS DDS */
	fh_write32((REG_FHCTL0_DVFS + (pll_id * 0x14)), (dds_value) | (1U << 31));	/* set dds */

	if (ilog) {
		FH_MSG("4. set DFS DDS");
		FH_MSG("FHCTL%d_DDS: 0x%08x", pll_id,
		       (fh_read32(REG_FHCTL0_DDS + (pll_id * 0x14)) & 0x1FFFFF));
		FH_MSG("FHCTL%d_DVFS: 0x%08x", pll_id,
		       (fh_read32(REG_FHCTL0_DVFS + (pll_id * 0x14)) & 0x1FFFFF));
	}
	/* 4.1 ensure jump to target DDS */
	fh_dds = (fh_read32(REG_FHCTL0_MON + (pll_id * 0x14))) & 0x1FFFFF;
	while ((dds_value != fh_dds) && (i < 100)) {
		if (unlikely(i > 100)) {
			FH_BUG_ON(i > 100);
			break;
		}

		udelay(10);
		fh_dds = (fh_read32(REG_FHCTL0_MON + (pll_id * 0x14))) & 0x1FFFFF;
		i++;
	}

	if (ilog) {
		FH_MSG("4.1 ensure jump to target DDS");
		FH_MSG("i: %d", i);
	}
	/* 5. write back to ncpo */

	if (ilog)
		FH_MSG("5. write back to ncpo");
	if (pll_id == MT658X_FH_ARM_PLL) {	/* FHCTL0 */
		fh_write32(ARMPLL_CON1,
			   (fh_read32(REG_FHCTL0_DVFS) & 0x1FFFFF) | (fh_read32(ARMPLL_CON1) &
								      0xFFE00000) | (1U << 31));

		if (ilog)
			FH_MSG("ARMPLL_CON1: 0x%08x", (fh_read32(ARMPLL_CON1) & 0x1FFFFF));
	} else if (pll_id == MT658X_FH_MAIN_PLL) {	/* FHCTL1 */
		fh_write32(MAINPLL_CON1,
			   (fh_read32(REG_FHCTL1_DVFS) & 0x1FFFFF) | (fh_read32(MAINPLL_CON1) &
								      0xFFE00000) | (1U << 31));

		if (ilog)
			FH_MSG("MAINPLL_CON1: 0x%08x", (fh_read32(MAINPLL_CON1) & 0x1FFFFF));
	} else if (pll_id == MT658X_FH_MEM_PLL) {	/* FHCTL2 */
		fh_write32((DDRPHY_BASE + 0x624),
			   ((fh_read32(REG_FHCTL2_DVFS) & 0x1FFFFF) << 11) |
			   ((fh_read32(DDRPHY_BASE + 0x624)) & 0x7FF));

		if (ilog) {
			FH_MSG("(DDRPHY_BASE+0x624): 0x%08x   >>11: 0x%08x",
			       (fh_read32(DDRPHY_BASE + 0x624)),
			       (fh_read32(DDRPHY_BASE + 0x624) >> 11));
		}
		if ((fh_read32(DDRPHY_BASE + 0x624)) & 0x1)
			fh_write32((DDRPHY_BASE + 0x624),
				   ((fh_read32(DDRPHY_BASE + 0x624)) & 0xFFFFFFFE));
		else
			fh_write32((DDRPHY_BASE + 0x624), ((fh_read32(DDRPHY_BASE + 0x624)) | 0x1));

		if (ilog) {
			FH_MSG("(DDRPHY_BASE+0x624): 0x%08x   >>11: 0x%08x",
			       (fh_read32(DDRPHY_BASE + 0x624)),
			       (fh_read32(DDRPHY_BASE + 0x624) >> 11));
		}
	} else if (pll_id == MT658X_FH_MSDC_PLL) {	/* FHCTL3 */
		fh_write32(MSDCPLL_CON1,
			   (fh_read32(REG_FHCTL3_DVFS) & 0x1FFFFF) | (fh_read32(MSDCPLL_CON1) &
								      0xFFE00000) | (1U << 31));

		if (ilog)
			FH_MSG("MSDCPLL_CON1: 0x%08x", (fh_read32(MSDCPLL_CON1) & 0x1FFFFF));
	} else if (pll_id == MT658X_FH_MM_PLL) {	/* FHCTL4 */
		fh_write32(MMPLL_CON1,
			   (fh_read32(REG_FHCTL4_DVFS) & 0x1FFFFF) | (fh_read32(MMPLL_CON1) &
								      0xFFE00000) | (1U << 31));

		if (ilog)
			FH_MSG("MMPLL_CON1: 0x%08x", (fh_read32(MMPLL_CON1) & 0x1FFFFF));
	} else if (pll_id == MT658X_FH_VENC_PLL) {	/* FHCTL5 */
		fh_write32(VENCPLL_CON1,
			   (fh_read32(REG_FHCTL5_DVFS) & 0x1FFFFF) | (fh_read32(VENCPLL_CON1) &
								      0xFFE00000) | (1U << 31));

		if (ilog)
			FH_MSG("VENCPLL_CON1: 0x%08x", (fh_read32(VENCPLL_CON1) & 0x1FFFFF));
	}
	/* 8127 FHCTL MB */
	else if (pll_id == MT658X_FH_TVD_PLL) {	/* FHCTL3 */
		fh_write32(TVDPLL_CON1,
			   (fh_read32(REG_FHCTL3_DVFS) & 0x1FFFFF) | (fh_read32(TVDPLL_CON1) &
								      0xFFE00000) | (1U << 31));

		if (ilog)
			FH_MSG("TVDPLL_CON1: 0x%08x", (fh_read32(TVDPLL_CON1) & 0x1FFFFF));
	}
	/* 8127 FHCTL ME */

	/* 6. switch to register control */
	mt_fh_hal_switch_register_to_FHCTL_control(pll_id, 0);
	mb();			/* switch to register control */

	if (ilog) {
		FH_MSG("6. switch to register control");
		FH_MSG("PLL_HP_CON0: 0x%08x", (fh_read32(PLL_HP_CON0) & 0x3F));
	}

	local_irq_restore(flags);
	return 0;
}

static int mt_fh_hal_dfs_armpll(unsigned int current_freq, unsigned int target_freq)
{				/* armpll dfs mdoe */
	unsigned long flags;
	unsigned int target_dds = 0;

	FH_MSG("EN: %s:", __func__);
	FH_MSG("current freq:%d target freq:%d", current_freq, target_freq);
	FH_MSG("current dds(ARMPLL_CON1): 0x%x", (fh_read32(ARMPLL_CON1) & 0x1FFFFF));

	/* TODO: provelock issue spin_lock(&freqhopping_lock); */
	spin_lock_irqsave(&freqhopping_lock, flags);

	fh_set_field(REG_FHCTL0_CFG, FH_FRDDSX_EN, 0);	/* disable SSC mode */
	fh_set_field(REG_FHCTL0_CFG, FH_SFSTRX_EN, 0);	/* disable dvfs mode */
	fh_set_field(REG_FHCTL0_CFG, FH_FHCTLX_EN, 0);	/* disable hopping control */
	target_dds =
	    (((target_freq / 100) *
	      (((fh_read32(ARMPLL_CON1) & 0x1FFFFF) * 1000) / (current_freq / 100))) / 1000);
	FH_MSG("target dds: 0x%x", target_dds);

	mt_fh_hal_dvfs(MT658X_FH_ARM_PLL, target_dds);

	fh_set_field(REG_FHCTL0_CFG, FH_FRDDSX_EN, 0);	/* disable SSC mode */
	fh_set_field(REG_FHCTL0_CFG, FH_SFSTRX_EN, 0);	/* disable dvfs mode */
	fh_set_field(REG_FHCTL0_CFG, FH_FHCTLX_EN, 0);	/* disable hopping control */
	spin_unlock_irqrestore(&freqhopping_lock, flags);

	return 0;
}

#if FULLY_VERSION_FHCTL
static int mt_fh_hal_l2h_dvfs_mempll(void)
{				/* mempll low to high (200->266MHz) */
	unsigned long flags;
	unsigned int fh_dds = 0;
	unsigned int pll_dds = 0;
	unsigned int i = 0;

	FH_MSG("EN: %s:", __func__);
	/* Please note that the DFS can・t be applied in systems with DDR3 DRAM!
	 * It・s only for the mobile DRAM (LPDDR*)!
	 */

	if ((get_dram_type() != LPDDR2) && (get_dram_type() != LPDDR3)) {
		FH_MSG("Not LPDDR*");
		return -1;
	}

	FH_MSG("dds: 0x%x", (fh_read32(DDRPHY_BASE + 0x624) >> 11));

	g_pll_mode = (fh_read32(DDRPHY_BASE + 0x60C) & 0x200);
	FH_MSG("g_pll_mode(0:1pll, 1:3pll): %x", g_pll_mode);

	if (g_pll_mode == 0) {	/* 1pll mode */
		if ((fh_read32(DDRPHY_BASE + 0x624) >> 11) == 0x14030C) {
			FH_MSG("Already @533MHz");
			return 0;
		}
		FH_MSG("Jump to 533MHz");
	} else if (g_pll_mode == 0x200) {	/* 3pll mode */
		if ((fh_read32(DDRPHY_BASE + 0x624) >> 11) == 0x131A2E) {
			FH_MSG("Already @533MHz");
			return 0;
		}
		FH_MSG("Jump to 533MHz");
	}
	/* TODO: provelock issue spin_lock(&freqhopping_lock); */
	spin_lock_irqsave(&freqhopping_lock, flags);

	if (g_fh_pll[MT658X_FH_MEM_PLL].fh_status == FH_FH_ENABLE_SSC) {	/* only when SSC is enable */
		/* Turn off MEMPLL hopping */
		fh_set_field(REG_FHCTL2_CFG, FH_FRDDSX_EN, 0);	/* disable SSC mode */
		fh_set_field(REG_FHCTL2_CFG, FH_SFSTRX_EN, 0);	/* disable dvfs mode */
		fh_set_field(REG_FHCTL2_CFG, FH_FHCTLX_EN, 0);	/* disable hopping control */

		/* udelay(800); */

		pll_dds = (fh_read32(REG_FHCTL2_DDS)) & 0x1FFFFF;
		fh_dds = (fh_read32(REG_FHCTL2_MON)) & 0x1FFFFF;

		FH_MSG(">p:f< %x:%x", pll_dds, fh_dds);

		while ((pll_dds != fh_dds) && (i < 100)) {

			if (unlikely(i > 100)) {
				FH_BUG_ON(i > 100);
				break;
			}

			udelay(10);
			fh_dds = (fh_read32(REG_FHCTL2_MON)) & 0x1FFFFF;
			i++;
		}

		FH_MSG("<p:f:i> %x:%x:%d", pll_dds, fh_dds, i);

	}

	if (g_pll_mode == 0)	/* 1pll mode */
		mt_fh_hal_dvfs(MT658X_FH_MEM_PLL, 0x14030C);	/* dfs 266.5MHz */
	else if (g_pll_mode == 0x200)	/* 3pll mode */
		mt_fh_hal_dvfs(MT658X_FH_MEM_PLL, 0x131A2E);	/* dfs 266.5MHz */

	g_curr_dramc = 266;

	if (g_fh_pll[MT658X_FH_MEM_PLL].fh_status == FH_FH_ENABLE_SSC) {
		fh_set_field(REG_FHCTL2_CFG, FH_FRDDSX_EN, 0);	/* disable SSC mode */
		fh_set_field(REG_FHCTL2_CFG, FH_SFSTRX_EN, 0);	/* disable dvfs mode */
		fh_set_field(REG_FHCTL2_CFG, FH_FHCTLX_EN, 0);	/* disable hopping control */

		if (!(mt_fh_hal_sync_ncpo_to_FHCTL_DDS(MT658X_FH_MEM_PLL)))
			return 0;
		FH_MSG("1. sync ncpo to DDS of FHCTL");
		FH_MSG("FHCTL2_DDS: 0x%08x", (fh_read32(REG_FHCTL2_DDS) & 0x1FFFFF));

		fh_set_field(REG_FHCTL2_CFG, FH_SFSTRX_DYS, mt_ssc_mempll_setting[2].df);
		fh_set_field(REG_FHCTL2_CFG, FH_SFSTRX_DTS, mt_ssc_mempll_setting[2].dt);

		fh_write32(REG_FHCTL2_UPDNLMT,
			   (PERCENT_TO_DDSLMT
			    ((fh_read32(REG_FHCTL2_DDS) & 0x1FFFFF),
			     mt_ssc_mempll_setting[2].lowbnd) << 16));
		FH_MSG("REG_FHCTL2_UPDNLMT: 0x%08x", fh_read32(REG_FHCTL2_UPDNLMT));

		mt_fh_hal_switch_register_to_FHCTL_control(MT658X_FH_MEM_PLL, 1);

		fh_set_field(REG_FHCTL2_CFG, FH_FRDDSX_EN, 1);	/* enable SSC mode */
		fh_set_field(REG_FHCTL2_CFG, FH_FHCTLX_EN, 1);	/* enable hopping control */
		FH_MSG("REG_FHCTL2_CFG: 0x%08x", fh_read32(REG_FHCTL2_CFG));

	}
	/* TODO: provelock issue spin_unlock(&freqhopping_lock); */
	spin_unlock_irqrestore(&freqhopping_lock, flags);

	return 0;
}

static int mt_fh_hal_h2l_dvfs_mempll(void)
{				/* mempll high to low(266->200MHz) */
	unsigned long flags;
	unsigned int fh_dds = 0;
	unsigned int pll_dds = 0;
	unsigned int i = 0;

	FH_MSG("EN: %s:", __func__);
	/* Please note that the DFS can・t be applied in systems with DDR3 DRAM!
	 * It・s only for the mobile DRAM (LPDDR*)!
	 */

	if ((get_dram_type() != LPDDR2) && (get_dram_type() != LPDDR3)) {
		FH_MSG("Not LPDDR*");
		return -1;
	}

	FH_MSG("dds: 0x%x", (fh_read32(DDRPHY_BASE + 0x624) >> 11));

	g_pll_mode = (fh_read32(DDRPHY_BASE + 0x60C) & 0x200);
	FH_MSG("g_pll_mode(0:1pll, 1:3pll): %x", g_pll_mode);

	if (g_pll_mode == 0) {	/* 1pll mode */
		if ((fh_read32(DDRPHY_BASE + 0x624) >> 11) == 0x0F04B0) {
			FH_MSG("Already @400MHz");
			return 0;
		}
		FH_MSG("Jump to 400MHz");
	} else if (g_pll_mode == 0x200) {	/* 3pll mode */
		if ((fh_read32(DDRPHY_BASE + 0x624) >> 11) == 0x0E55EE) {
			FH_MSG("Already @400MHz");
			return 0;
		}
		FH_MSG("Jump to 400MHz");
	}
	/* TODO: provelock issue spin_lock(&freqhopping_lock); */
	spin_lock_irqsave(&freqhopping_lock, flags);

	if (g_fh_pll[MT658X_FH_MEM_PLL].fh_status == FH_FH_ENABLE_SSC) {	/* only when SSC is enable */
		/* Turn off MEMPLL hopping */
		fh_set_field(REG_FHCTL2_CFG, FH_FRDDSX_EN, 0);	/* disable SSC mode */
		fh_set_field(REG_FHCTL2_CFG, FH_SFSTRX_EN, 0);	/* disable dvfs mode */
		fh_set_field(REG_FHCTL2_CFG, FH_FHCTLX_EN, 0);	/* disable hopping control */

		/* udelay(800); */

		pll_dds = (fh_read32(REG_FHCTL2_DDS)) & 0x1FFFFF;
		fh_dds = (fh_read32(REG_FHCTL2_MON)) & 0x1FFFFF;

		FH_MSG(">p:f< %x:%x", pll_dds, fh_dds);

		while ((pll_dds != fh_dds) && (i < 100)) {

			if (unlikely(i > 100)) {
				FH_BUG_ON(i > 100);
				break;
			}

			udelay(10);
			fh_dds = (fh_read32(REG_FHCTL2_MON)) & 0x1FFFFF;
			i++;
		}

		FH_MSG("<p:f:i> %x:%x:%d", pll_dds, fh_dds, i);
	}

	if (g_pll_mode == 0)	/* 1pll mode */
		mt_fh_hal_dvfs(MT658X_FH_MEM_PLL, 0x0F04B0);	/* dfs 200MHz */
	else if (g_pll_mode == 0x200)	/* 3pll mode */
		mt_fh_hal_dvfs(MT658X_FH_MEM_PLL, 0x0E55EE);	/* dfs 200MHz */

	g_curr_dramc = 200;

	if (g_fh_pll[MT658X_FH_MEM_PLL].fh_status == FH_FH_ENABLE_SSC) {
		fh_set_field(REG_FHCTL2_CFG, FH_FRDDSX_EN, 0);	/* disable SSC mode */
		fh_set_field(REG_FHCTL2_CFG, FH_SFSTRX_EN, 0);	/* disable dvfs mode */
		fh_set_field(REG_FHCTL2_CFG, FH_FHCTLX_EN, 0);	/* disable hopping control */

		if (!(mt_fh_hal_sync_ncpo_to_FHCTL_DDS(MT658X_FH_MEM_PLL)))
			return 0;
		FH_MSG("Enable mempll SSC mode");
		FH_MSG("1. sync ncpo to DDS of FHCTL");
		FH_MSG("FHCTL2_DDS: 0x%08x", (fh_read32(REG_FHCTL2_DDS) & 0x1FFFFF));

		fh_set_field(REG_FHCTL2_CFG, FH_SFSTRX_DYS, mt_ssc_mempll_setting[3].df);
		fh_set_field(REG_FHCTL2_CFG, FH_SFSTRX_DTS, mt_ssc_mempll_setting[3].dt);

		fh_write32(REG_FHCTL2_UPDNLMT,
			   (PERCENT_TO_DDSLMT
			    ((fh_read32(REG_FHCTL2_DDS) & 0x1FFFFF),
			     mt_ssc_mempll_setting[3].lowbnd) << 16));
		FH_MSG("REG_FHCTL2_UPDNLMT: 0x%08x", fh_read32(REG_FHCTL2_UPDNLMT));

		mt_fh_hal_switch_register_to_FHCTL_control(MT658X_FH_MEM_PLL, 1);

		fh_set_field(REG_FHCTL2_CFG, FH_FRDDSX_EN, 1);	/* enable SSC mode */
		fh_set_field(REG_FHCTL2_CFG, FH_FHCTLX_EN, 1);	/* enable hopping control */

		FH_MSG("REG_FHCTL2_CFG: 0x%08x", fh_read32(REG_FHCTL2_CFG));

	}
	/* TODO: provelock issue spin_unlock(&freqhopping_lock); */
	spin_unlock_irqrestore(&freqhopping_lock, flags);

	return 0;
}

static int mt_h2oc_dfs_mempll(void)
{
	unsigned long flags;
	unsigned int fh_dds = 0;
	unsigned int pll_dds = 0;
	unsigned int i = 0;

	FH_MSG("EN: %s:", __func__);
	/* Please note that the DFS can・t be applied in systems with DDR3 DRAM!
	 * It・s only for the mobile DRAM (LPDDR*)!
	 */

	if ((get_dram_type() != LPDDR2) && (get_dram_type() != LPDDR3)) {
		FH_MSG("Not LPDDR*");
		return -1;
	}

	FH_MSG("dds: 0x%x", (fh_read32(DDRPHY_BASE + 0x624) >> 11));

	g_pll_mode = (fh_read32(DDRPHY_BASE + 0x60C) & 0x200);
	FH_MSG("g_pll_mode(0:1pll, 1:3pll): %x", g_pll_mode);

	if (g_pll_mode == 0) {	/* 1pll mode */
		if ((fh_read32(DDRPHY_BASE + 0x624) >> 11) == 0x160077) {
			FH_MSG("Already @1172MHz");
			return 0;
		}
		FH_MSG("Jump to 1172MHz");
	} else if (g_pll_mode == 0x200) {	/* 3pll mode */
		if ((fh_read32(DDRPHY_BASE + 0x624) >> 11) == 0x150071) {
			FH_MSG("Already @1172MHz");
			return 0;
		}
		FH_MSG("Jump to 1172MHz");
	}
	/* TODO: provelock issue spin_lock(&freqhopping_lock); */
	spin_lock_irqsave(&freqhopping_lock, flags);

	if (g_fh_pll[MT658X_FH_MEM_PLL].fh_status == FH_FH_ENABLE_SSC) {	/* only when SSC is enable */
		/* Turn off MEMPLL hopping */
		fh_set_field(REG_FHCTL2_CFG, FH_FRDDSX_EN, 0);	/* disable SSC mode */
		fh_set_field(REG_FHCTL2_CFG, FH_SFSTRX_EN, 0);	/* disable dvfs mode */
		fh_set_field(REG_FHCTL2_CFG, FH_FHCTLX_EN, 0);	/* disable hopping control */

		/* udelay(800); */

		pll_dds = (fh_read32(REG_FHCTL2_DDS)) & 0x1FFFFF;
		fh_dds = (fh_read32(REG_FHCTL2_MON)) & 0x1FFFFF;

		FH_MSG(">p:f< %x:%x", pll_dds, fh_dds);

		while ((pll_dds != fh_dds) && (i < 100)) {

			if (unlikely(i > 100)) {
				FH_BUG_ON(i > 100);
				break;
			}

			udelay(10);
			fh_dds = (fh_read32(REG_FHCTL2_MON)) & 0x1FFFFF;
			i++;
		}

		FH_MSG("<p:f:i> %x:%x:%d", pll_dds, fh_dds, i);

	}

	if (g_pll_mode == 0)	/* 1pll mode */
		mt_fh_hal_dvfs(MT658X_FH_MEM_PLL, 0x160077);	/* dfs293MHz */
	else if (g_pll_mode == 0x200)	/* 3pll mode */
		mt_fh_hal_dvfs(MT658X_FH_MEM_PLL, 0x150071);	/* dfs 293MHz */

	if (g_fh_pll[MT658X_FH_MEM_PLL].fh_status == FH_FH_ENABLE_SSC) {
		fh_set_field(REG_FHCTL2_CFG, FH_FRDDSX_EN, 0);	/* disable SSC mode */
		fh_set_field(REG_FHCTL2_CFG, FH_SFSTRX_EN, 0);	/* disable dvfs mode */
		fh_set_field(REG_FHCTL2_CFG, FH_FHCTLX_EN, 0);	/* disable hopping control */

		if (!(mt_fh_hal_sync_ncpo_to_FHCTL_DDS(MT658X_FH_MEM_PLL)))
			return 0;
		FH_MSG("1. sync ncpo to DDS of FHCTL");
		FH_MSG("FHCTL2_DDS: 0x%08x", (fh_read32(REG_FHCTL2_DDS) & 0x1FFFFF));

		fh_set_field(REG_FHCTL2_CFG, FH_SFSTRX_DYS, mt_ssc_mempll_setting[2].df);
		fh_set_field(REG_FHCTL2_CFG, FH_SFSTRX_DTS, mt_ssc_mempll_setting[2].dt);

		fh_write32(REG_FHCTL2_UPDNLMT,
			   (PERCENT_TO_DDSLMT
			    ((fh_read32(REG_FHCTL2_DDS) & 0x1FFFFF),
			     mt_ssc_mempll_setting[2].lowbnd) << 16));
		FH_MSG("REG_FHCTL2_UPDNLMT: 0x%08x", fh_read32(REG_FHCTL2_UPDNLMT));

		mt_fh_hal_switch_register_to_FHCTL_control(MT658X_FH_MEM_PLL, 1);

		fh_set_field(REG_FHCTL2_CFG, FH_FRDDSX_EN, 1);	/* enable SSC mode */
		fh_set_field(REG_FHCTL2_CFG, FH_FHCTLX_EN, 1);	/* enable hopping control */
		FH_MSG("REG_FHCTL2_CFG: 0x%08x", fh_read32(REG_FHCTL2_CFG));

	}
	/* TODO: provelock issue spin_unlock(&freqhopping_lock); */
	spin_unlock_irqrestore(&freqhopping_lock, flags);

	return 0;
}

static int mt_fh_hal_dram_overclock(int clk)
{
	FH_MSG("EN: %s clk:%d", __func__, clk);
	if (1) {		/* DFS mode */
		if (clk == 200) {
			g_curr_dramc = 200;
			return mt_fh_hal_h2l_dvfs_mempll();
		}

		if (clk == 266) {
			g_curr_dramc = 266;
			return mt_fh_hal_l2h_dvfs_mempll();
		}

		if (clk == 293) {
			g_curr_dramc = 293;
			return mt_h2oc_dfs_mempll();
		}
	} else {
		if (clk == LOW_DRAMC) {	/* target freq: 208.5MHz */
			if (g_curr_dramc != 266) {	/* 266 -> 208.5 only */
				FH_BUG_ON(1);
				return -1;
			} else {	/* let's move from 266 to 208.5 */
				return mt_fh_hal_h2l_mempll();
			}
		}

		if (clk == 293) {	/* target freq: 293MHz */
			if (g_curr_dramc != 266) {	/* 266 -> 293 only */
				FH_BUG_ON(1);
				return -1;
			} else {	/* let's move from 266 to 293 */
				return mt_h2oc_mempll();
			}
		}

		if (clk == 266) {	/* //target freq: 293MHz */
			if (g_curr_dramc == 266) {	/* cannot be 266 -> 266 */
				FH_BUG_ON(1);
				return -1;
			} else if (g_curr_dramc == LOW_DRAMC) {	/* 208 -> 266 */
				return mt_fh_hal_l2h_mempll();
			} else if (g_curr_dramc == 293) {	/* 293 -> 266 */
				return mt_oc2h_mempll();
			}
		}
	}
	FH_BUG_ON(1);
	return -1;
}

static int mt_fh_hal_get_dramc(void)
{
	return g_curr_dramc;
}

#else
static int mt_fh_hal_l2h_dvfs_mempll(void)
{
	return 0;
}
static int mt_fh_hal_h2l_dvfs_mempll(void)
{
	return 0;
}
static int mt_h2oc_dfs_mempll(void)
{
	return 0;
}
static int mt_fh_hal_dram_overclock(int clk)
{
	return 0;
}

static int mt_fh_hal_get_dramc(void)
{
	return 0;
}
#endif


static void mt_fh_hal_popod_save(void)
{
	unsigned int fh_dds = 0;
	unsigned int pll_dds = 0;
	unsigned int i = 0;

	FH_MSG("EN: %s", __func__);

	/* disable maipll SSC mode */
	if (g_fh_pll[MT658X_FH_MAIN_PLL].fh_status == FH_FH_ENABLE_SSC) {	/* only when SSC is enable */
		/* Turn off MAINPLL hopping */
		fh_set_field(REG_FHCTL1_CFG, FH_FRDDSX_EN, 0);	/* disable SSC mode */
		fh_set_field(REG_FHCTL1_CFG, FH_SFSTRX_EN, 0);	/* disable dvfs mode */
		fh_set_field(REG_FHCTL1_CFG, FH_FHCTLX_EN, 0);	/* disable hopping control */

		/* udelay(800); */

		pll_dds = (fh_read32(REG_FHCTL1_DDS)) & 0x1FFFFF;
		fh_dds = (fh_read32(REG_FHCTL1_MON)) & 0x1FFFFF;

		FH_MSG("Org pll_dds:%x fh_dds:%x", pll_dds, fh_dds);

		while ((pll_dds != fh_dds) && (i < 100)) {

			if (unlikely(i > 100)) {
				FH_BUG_ON(i > 100);
				break;
			}

			udelay(10);
			fh_dds = (fh_read32(REG_FHCTL1_MON)) & 0x1FFFFF;
			i++;
		}

		FH_MSG("pll_dds:%x  fh_dds:%x   i:%d", pll_dds, fh_dds, i);

		/* write back to ncpo */
		fh_write32(MAINPLL_CON1,
			   (fh_read32(REG_FHCTL1_DDS) & 0x1FFFFF) | (fh_read32(MAINPLL_CON1) &
								     0xFFE00000) | (1U << 31));
		FH_MSG("MAINPLL_CON1: 0x%08x", (fh_read32(MAINPLL_CON1) & 0x1FFFFF));

		/* switch to register control */
		mt_fh_hal_switch_register_to_FHCTL_control(MT658X_FH_MAIN_PLL, 0);
		mb();		/* switch to register control */
		FH_MSG("switch to register control PLL_HP_CON0: 0x%08x",
		       (fh_read32(PLL_HP_CON0) & 0x3F));

	}

}

static void mt_fh_hal_popod_restore(void)
{

	FH_MSG("EN: %s", __func__);

	/* enable maipll SSC mode */
	if (g_fh_pll[MT658X_FH_MAIN_PLL].fh_status == FH_FH_ENABLE_SSC) {
		fh_set_field(REG_FHCTL1_CFG, FH_FRDDSX_EN, 0);	/* disable SSC mode */
		fh_set_field(REG_FHCTL1_CFG, FH_SFSTRX_EN, 0);	/* disable dvfs mode */
		fh_set_field(REG_FHCTL1_CFG, FH_FHCTLX_EN, 0);	/* disable hopping control */

		if (!(mt_fh_hal_sync_ncpo_to_FHCTL_DDS(MT658X_FH_MAIN_PLL)))
			return;
		FH_MSG("Enable mainpll SSC mode");
		FH_MSG("sync ncpo to DDS of FHCTL");
		FH_MSG("FHCTL1_DDS: 0x%08x", (fh_read32(REG_FHCTL1_DDS) & 0x1FFFFF));

		fh_set_field(REG_FHCTL1_CFG, FH_SFSTRX_DYS, mt_ssc_mainpll_setting[2].df);
		fh_set_field(REG_FHCTL1_CFG, FH_SFSTRX_DTS, mt_ssc_mainpll_setting[2].dt);

		fh_write32(REG_FHCTL1_UPDNLMT,
			   (PERCENT_TO_DDSLMT
			    ((fh_read32(REG_FHCTL1_DDS) & 0x1FFFFF),
			     mt_ssc_mainpll_setting[2].lowbnd) << 16));
		FH_MSG("REG_FHCTL1_UPDNLMT: 0x%08x", fh_read32(REG_FHCTL1_UPDNLMT));

		mt_fh_hal_switch_register_to_FHCTL_control(MT658X_FH_MAIN_PLL, 1);

		fh_set_field(REG_FHCTL1_CFG, FH_FRDDSX_EN, 1);	/* enable SSC mode */
		fh_set_field(REG_FHCTL1_CFG, FH_FHCTLX_EN, 1);	/* enable hopping control */

		FH_MSG("REG_FHCTL1_CFG: 0x%08x", fh_read32(REG_FHCTL1_CFG));

	}

}

#if FULLY_VERSION_FHCTL
/* static int freqhopping_dramc_proc_read(char *page, char **start, off_t off, int count, int *eof, void *data) */
static int freqhopping_dramc_proc_read(struct seq_file *m, void *v)
{
	/* char *p = page; */
	/* int len = 0; */

	FH_MSG("EN: %s", __func__);

	seq_printf(m, "DRAMC: %dMHz\r\n", g_curr_dramc);
	seq_printf(m, "mt_get_emi_freq(): %dHz\r\n", mt_get_emi_freq());
	seq_printf(m, "get_dram_type(): %d\r\n", get_dram_type());
	/* EMI_CONA = EMI_BASE_ADDR + 0x0000,
	 * EMI_BASE_ADDR defined at emi_mpu.c,
	 * not export, don't print this log
	 * seq_printf(m, "rank: 0x%x\r\n", (fh_read32(EMI_CONA) & 0x20000));
	 */

#if 0
	*start = page + off;

	len = p - page;

	if (len > off)
		len -= off;
	else
		len = 0;

	return len < count ? len : count;
#endif
	return 0;
}


static int freqhopping_dramc_proc_write(struct file *file, const char *buffer, unsigned long count,
					void *data)
{
	int len = 0, freq = 0;
	char dramc[32];

	FH_MSG("EN: proc");

	len = (count < (sizeof(dramc) - 1)) ? count : (sizeof(dramc) - 1);

	if (copy_from_user(dramc, buffer, len)) {
		FH_MSG("copy_from_user fail!");
		return 1;
	}

	dramc[len] = '\0';

	if (!kstrtoint(dramc, 10, &freq)) {
		if ((freq == 266) || (freq == 200)) {
			FH_MSG("dramc:%d ", freq);
			(freq == 266) ? mt_fh_hal_l2h_mempll() : mt_fh_hal_h2l_mempll();
		} else if (freq == 293) {
			mt_fh_hal_dram_overclock(293);
		} else {
			FH_MSG("must be 200/266/293!");
		}

#if 0
		if (freq == 266)
			FH_MSG("==> %d", mt_fh_hal_dram_overclock(266));
		else if (freq == 293)
			FH_MSG("==> %d", mt_fh_hal_dram_overclock(293));
		else if (freq == LOW_DRAMC)
			FH_MSG("==> %d", mt_fh_hal_dram_overclock(208));

#endif

		return count;
	}
	FH_MSG("  bad argument!!");

	return -EINVAL;
}
#else
static int freqhopping_dramc_proc_read(struct seq_file *m, void *v)
{
	return 0;
}
static int freqhopping_dramc_proc_write(struct file *file, const char *buffer, unsigned long count,
					void *data)
{
	return 0;
}
#endif

/* static int freqhopping_dvfs_proc_read(char *page, char **start, off_t off, int count, int *eof, void *data) */
static int freqhopping_dvfs_proc_read(struct seq_file *m, void *v)
{
	/* char *p = page; */
	/* int len = 0; */
	int i = 0;

	FH_MSG("EN: %s", __func__);

	seq_puts(m, "DVFS:\r\n");
	seq_puts(m, "CFG: 0x3 is SSC mode;  0x5 is DVFS mode \r\n");
	for (i = 0; i < MT_FHPLL_MAX; i++) {
		seq_printf(m, "FHCTL%d:   CFG:0x%08x    DVFS:0x%08x\r\n", i,
			   fh_read32(REG_FHCTL0_CFG + (i * 0x14)),
			   fh_read32(REG_FHCTL0_DVFS + (i * 0x14)));
	}

#if 0
	*start = page + off;

	len = p - page;

	if (len > off)
		len -= off;
	else
		len = 0;

	return len < count ? len : count;
#endif
	return 0;
}


static int freqhopping_dvfs_proc_write(struct file *file, const char *buffer, unsigned long count,
				       void *data)
{
	int ret;
	char kbuf[256];
	unsigned long len = 0;
	unsigned int p1, p2, p3, p4, p5;
/* struct          freqhopping_ioctl fh_ctl; */

	p1 = 0;
	p2 = 0;
	p3 = 0;
	p4 = 0;
	p5 = 0;

	FH_MSG("EN: %s", __func__);

	len = min(count, (unsigned long)(sizeof(kbuf) - 1));

	if (count == 0)
		return -1;
	if (count > 255)
		count = 255;

	ret = copy_from_user(kbuf, buffer, count);
	if (ret < 0)
		return -1;

	kbuf[count] = '\0';

	if (sscanf(kbuf, "%d %d %d %d %d", &p1, &p2, &p3, &p4, &p5) == 1)
		;
	else
		FH_MSG("bad argument!!");

	FH_MSG("EN: p1=%d p2=%d p3=%d", p1, p2, p3);

	if (p1 == MT658X_FH_MEM_PLL) {
		if (p2 == 533)
			mt_fh_hal_l2h_dvfs_mempll();
		else if (p2 == 400)
			mt_fh_hal_h2l_dvfs_mempll();
		else if (p2 == 586)
			mt_h2oc_dfs_mempll();
		else
			FH_MSG("not define %d freq @mempll", p2);
	} else if (p1 == MT658X_FH_ARM_PLL) {
		mt_fh_hal_dfs_armpll(p2, p3);
	} else if (p1 == 4370) {
		FH_MSG("EN: pllid=%d dt=%d df=%d lowbnd=%d", p2, p3, p4, p5);
		fh_set_field(REG_FHCTL0_CFG + (p2 * 0x14), FH_FRDDSX_EN, 0);	/* disable SSC mode */
		fh_set_field(REG_FHCTL0_CFG + (p2 * 0x14), FH_SFSTRX_EN, 0);	/* disable dvfs mode */
		fh_set_field(REG_FHCTL0_CFG + (p2 * 0x14), FH_FHCTLX_EN, 0);	/* disable hopping control */

		if (!(mt_fh_hal_sync_ncpo_to_FHCTL_DDS(p2)))
			return 0;
		FH_MSG("Enable FHCTL%d SSC mode", p2);
		FH_MSG("1. sync ncpo to DDS of FHCTL");
		FH_MSG("FHCTL%d_DDS: 0x%08x", p2,
		       (fh_read32(REG_FHCTL0_DDS + (p2 * 0x14)) & 0x1FFFFF));

		fh_set_field(REG_FHCTL0_CFG + (p2 * 0x14), FH_SFSTRX_DYS, p4);
		fh_set_field(REG_FHCTL0_CFG + (p2 * 0x14), FH_SFSTRX_DTS, p3);

		fh_write32(REG_FHCTL0_UPDNLMT + (p2 * 0x14),
			   (PERCENT_TO_DDSLMT
			    ((fh_read32(REG_FHCTL0_DDS + (p2 * 0x14)) & 0x1FFFFF), p5) << 16));
		FH_MSG("REG_FHCTL%d_UPDNLMT: 0x%08x", p2,
		       fh_read32(REG_FHCTL0_UPDNLMT + (p2 * 0x14)));

		mt_fh_hal_switch_register_to_FHCTL_control(p2, 1);

		fh_set_field(REG_FHCTL0_CFG + (p2 * 0x14), FH_FRDDSX_EN, 1);	/* enable SSC mode */
		fh_set_field(REG_FHCTL0_CFG + (p2 * 0x14), FH_FHCTLX_EN, 1);	/* enable hopping control */

		FH_MSG("REG_FHCTL%d_CFG: 0x%08x", p2, fh_read32(REG_FHCTL0_CFG + (p2 * 0x14)));
	} else if (p1 == 2222) {
		if (p2 == 0)	/* disable */
			mt_fh_hal_popod_save();
		else if (p2 == 1)	/* enable */
			mt_fh_hal_popod_restore();
	} else
		mt_fh_hal_dvfs(p1, p2);

	return count;
}



/* static int freqhopping_dumpregs_proc_read(char *page, char **start, off_t off, int count, int *eof, void *data) */
static int freqhopping_dumpregs_proc_read(struct seq_file *m, void *v)
{
	/* char  *p = page; */
	/* int   len = 0; */
	int i = 0;

	FH_MSG("EN: %s", __func__);

	seq_puts(m, "FHDMA_CFG:\r\n");

	seq_printf(m, "REG_FHDMA_CFG: 0x%08x 0x%08x 0x%08x 0x%08x\r\n",
		   fh_read32(REG_FHDMA_CFG),
		   fh_read32(REG_FHDMA_2G1BASE),
		   fh_read32(REG_FHDMA_2G2BASE), fh_read32(REG_FHDMA_INTMDBASE));

	seq_printf(m, "REG_FHDMA_EXTMDBASE: 0x%08x 0x%08x 0x%08x 0x%08x\r\n",
		   fh_read32(REG_FHDMA_EXTMDBASE),
		   fh_read32(REG_FHDMA_BTBASE),
		   fh_read32(REG_FHDMA_WFBASE), fh_read32(REG_FHDMA_FMBASE));

	seq_printf(m, "REG_FHSRAM_CON: 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\r\n",
		   fh_read32(REG_FHSRAM_CON),
		   fh_read32(REG_FHSRAM_WR),
		   fh_read32(REG_FHSRAM_RD), fh_read32(REG_FHCTL_CFG), fh_read32(REG_FHCTL_CON));

	seq_printf(m, "REG_FHCTL_2G1_CH: 0x%08x 0x%08x 0x%08x 0x%08x\r\n",
		   fh_read32(REG_FHCTL_2G1_CH),
		   fh_read32(REG_FHCTL_2G2_CH),
		   fh_read32(REG_FHCTL_INTMD_CH), fh_read32(REG_FHCTL_EXTMD_CH));

	seq_printf(m, "REG_FHCTL_BT_CH: 0x%08x 0x%08x 0x%08x \r\n\r\n",
		   fh_read32(REG_FHCTL_BT_CH),
		   fh_read32(REG_FHCTL_WF_CH), fh_read32(REG_FHCTL_FM_CH));


	for (i = 0; i < MT_FHPLL_MAX; i++) {

		seq_printf(m, "FHCTL%d_CFG:\r\n", i);
		seq_printf(m, "0x%08x 0x%08x 0x%08x 0x%08x\r\n",
			   fh_read32(REG_FHCTL0_CFG + (i * 0x14)),
			   fh_read32(REG_FHCTL0_UPDNLMT + (i * 0x14)),
			   fh_read32(REG_FHCTL0_DDS + (i * 0x14)),
			   fh_read32(REG_FHCTL0_MON + (i * 0x14)));
	}


	seq_printf(m, "\r\nPLL_HP_CON0:\r\n0x%08x\r\n", fh_read32(PLL_HP_CON0));

	seq_printf(m,
		   "\r\nPLL_CON0 :\r\nARM:0x%08x MAIN:0x%08x MSDC:0x%08x MM:0x%08x VENC:0x%08x\r\n",
		   fh_read32(ARMPLL_CON0), fh_read32(MAINPLL_CON0), fh_read32(MSDCPLL_CON0),
		   fh_read32(MMPLL_CON0), fh_read32(VENCPLL_CON0));

	seq_printf(m,
		   "\r\nPLL_CON1 :\r\nARM:0x%08x MAIN:0x%08x MSDC:0x%08x MM:0x%08x VENC:0x%08x\r\n",
		   fh_read32(ARMPLL_CON1), fh_read32(MAINPLL_CON1), fh_read32(MSDCPLL_CON1),
		   fh_read32(MMPLL_CON1), fh_read32(VENCPLL_CON1));


	seq_printf(m,
		   "\r\nMEMPLL :\r\nMEMPLL9: 0x%08x MEMPLL10: 0x%08x MEMPLL11: 0x%08x MEMPLL12: 0x%08x\r\n",
		   fh_read32(DDRPHY_BASE + 0x624), fh_read32(DDRPHY_BASE + 0x628),
		   fh_read32(DDRPHY_BASE + 0x62C)
		   , fh_read32(DDRPHY_BASE + 0x630));	/* TODO: Hard code for now... */
	seq_printf(m,
		   "\r\nMEMPLL :\r\nMEMPLL13: 0x%08x MEMPLL14: 0x%08x MEMPLL15: 0x%08x MEMPLL16: 0x%08x\r\n",
		   fh_read32(DDRPHY_BASE + 0x634), fh_read32(DDRPHY_BASE + 0x638),
		   fh_read32(DDRPHY_BASE + 0x63C)
		   , fh_read32(DDRPHY_BASE + 0x640));	/* TODO: Hard code for now... */

#if 0
	*start = page + off;

	len = p - page;

	if (len > off)
		len -= off;
	else
		len = 0;

	return len < count ? len : count;
#endif
	return 0;
}





#if MT_FH_CLK_GEN

/* static int freqhopping_clkgen_proc_read(char *page, char **start, off_t off, int count, int *eof, void *data) */
static int freqhopping_clkgen_proc_read(struct seq_file *m, void *v)
{
	/* char *p = page; */
	/* int len = 0; */

	FH_MSG("EN: %s", __func__);

	if (g_curr_clkgen > MT658X_FH_PLL_TOTAL_NUM)
		seq_puts(m, "no clkgen output.\r\n");
	else
		seq_printf(m, "clkgen:%d\r\n", g_curr_clkgen);

	seq_printf(m,
		   "\r\nMBIST :\r\nMBIST_CFG_2: 0x%08x MBIST_CFG_6: 0x%08x MBIST_CFG_7: 0x%08x\r\n",
		   fh_read32(MBIST_CFG_2), fh_read32(MBIST_CFG_6), fh_read32(MBIST_CFG_7));

	seq_printf(m, "\r\nCLK_CFG_3: 0x%08x\r\n", fh_read32(CLK_CFG_3));

	seq_printf(m, "\r\nTOP_CKMUXSEL: 0x%08x\r\n", fh_read32(TOP_CKMUXSEL));

	seq_printf(m, "\r\nGPIO: 0x%08x 0x%08x 0x%08x 0x%08x\r\n",
		   fh_read32(GPIO_BASE + 0xC60),
		   fh_read32(GPIO_BASE + 0xC70),
		   fh_read32(GPIO_BASE + 0xCD0), fh_read32(GPIO_BASE + 0xD90));


	seq_printf(m, "\r\nDDRPHY_BASE :\r\n0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\r\n",
		   fh_read32(DDRPHY_BASE + 0x600),
		   fh_read32(DDRPHY_BASE + 0x604),
		   fh_read32(DDRPHY_BASE + 0x608),
		   fh_read32(DDRPHY_BASE + 0x60C),
		   fh_read32(DDRPHY_BASE + 0x614), fh_read32(DDRPHY_BASE + 0x61C));
#if 0
	*start = page + off;

	len = p - page;

	if (len > off)
		len -= off;
	else
		len = 0;

	return len < count ? len : count;
#endif
	return 0;
}


static int freqhopping_clkgen_proc_write(struct file *file, const char *buffer, unsigned long count,
					 void *data)
{
	int len = 0, pll_id = 0;
	char clkgen[32];

	FH_MSG("EN: %s", __func__);

	len = (count < (sizeof(clkgen) - 1)) ? count : (sizeof(clkgen) - 1);

	/* if you want to measue the clk by evb, you should turn on GPIO34. */
	/* mt_set_gpio_mode(GPIO34, GPIO_MODE_03); */

	if (copy_from_user(clkgen, buffer, len)) {
		FH_MSG("copy_from_user fail!");
		return 1;
	}

	clkgen[len] = '\0';

	if (!kstrtoint(clkgen, 10, &pll_id)) {
		if (pll_id == MT658X_FH_ARM_PLL) {
			fh_write32(MBIST_CFG_2, 0x00000009);	/* divide by 9+1 */
			fh_write32(CLK_CFG_3, 0x00000001);	/* enable it */
			udelay(1000);
			fh_write32(CLK_CFG_3, 0x00000001);	/* pll_pre_clk [don't care @ ARMPLL] */
			udelay(1000);
			fh_write32(CLK_CFG_3, 0x00000001);	/* pll_clk_sel [don't care @ ARMPLL] */
			udelay(1000);
			fh_write32(CLK_CFG_3, 0x00000401);	/* abist_clk_sel [0100: armpll_occ_mon] */
			udelay(1000);

		} else if (pll_id == MT658X_FH_MAIN_PLL) {
			fh_write32(MBIST_CFG_2, 0x00000009);	/* divide by 9+1 */
			fh_write32(CLK_CFG_3, 0x00000001);	/* enable it */
			udelay(1000);
			fh_write32(CLK_CFG_3, 0x000F0001);	/* pll_pre_clk [1111: AD_MAIN_H230P3M] */
			udelay(1000);
			fh_write32(CLK_CFG_3, 0x000F0001);	/* pll_clk_sel [0000: pll_pre_clk] */
			udelay(1000);
			fh_write32(CLK_CFG_3, 0x000F0F01);	/* abist_clk_sel [1111: pll_clk_out] */
			udelay(1000);

		} else if (pll_id == MT658X_FH_MEM_PLL) {

			fh_write32(DDRPHY_BASE + 0x600,
				   ((fh_read32(DDRPHY_BASE + 0x600)) | 0x1 << 5));


			fh_write32(DDRPHY_BASE + 0x60C,
				   ((fh_read32(DDRPHY_BASE + 0x60C)) | 0x1 << 21));
			fh_write32(DDRPHY_BASE + 0x614,
				   ((fh_read32(DDRPHY_BASE + 0x614)) | 0x1 << 21));
			fh_write32(DDRPHY_BASE + 0x61C,
				   ((fh_read32(DDRPHY_BASE + 0x61C)) | 0x1 << 21));

			fh_write32(DDRPHY_BASE + 0x60C, ((fh_read32(DDRPHY_BASE + 0x60C)) & ~0x7));
			fh_write32(DDRPHY_BASE + 0x60C, ((fh_read32(DDRPHY_BASE + 0x60C)) | 0x2));
			fh_write32(DDRPHY_BASE + 0x614, ((fh_read32(DDRPHY_BASE + 0x614)) & ~0x7));
			fh_write32(DDRPHY_BASE + 0x614, ((fh_read32(DDRPHY_BASE + 0x614)) | 0x2));
			fh_write32(DDRPHY_BASE + 0x61C, ((fh_read32(DDRPHY_BASE + 0x61C)) & ~0x7));
			fh_write32(DDRPHY_BASE + 0x61C, ((fh_read32(DDRPHY_BASE + 0x61C)) | 0x2));

			fh_write32(DDRPHY_BASE + 0x604,
				   ((fh_read32(DDRPHY_BASE + 0x604)) | 0x1 << 3));
			fh_write32(DDRPHY_BASE + 0x604,
				   ((fh_read32(DDRPHY_BASE + 0x604)) | 0x1 << 7));
			fh_write32(DDRPHY_BASE + 0x604,
				   ((fh_read32(DDRPHY_BASE + 0x604)) | 0x1 << 4));
			fh_write32(DDRPHY_BASE + 0x604,
				   ((fh_read32(DDRPHY_BASE + 0x604)) | 0x1 << 9));

#if 0
			fh_write32(DDRPHY_BASE + 0x608,
				   ((fh_read32(DDRPHY_BASE + 0x608)) & ~0x000E0000));
#endif
			fh_write32(DDRPHY_BASE + 0x608,
				   ((fh_read32(DDRPHY_BASE + 0x608)) | 0x00040000));

			fh_write32(DDRPHY_BASE + 0x608,
				   ((fh_read32(DDRPHY_BASE + 0x608)) & ~0xF0000000));
			fh_write32(DDRPHY_BASE + 0x608,
				   ((fh_read32(DDRPHY_BASE + 0x608)) | 0x80000000));

			/* fh_write32(MBIST_CFG_2, 0x00000001); //divide by 1+1 */
			fh_write32(CLK_CFG_3, 0x00000001);	/* enable it */
			udelay(1000);
			fh_write32(CLK_CFG_3, 0x00000001);	/* pll_pre_clk [don't care @ ARMPLL] */
			udelay(1000);
			fh_write32(CLK_CFG_3, 0x00000001);	/* pll_clk_sel [don't care @ ARMPLL] */
			udelay(1000);
			fh_write32(CLK_CFG_3, 0x00000501);	/* abist_clk_sel [0101: AD_MEMPLL_MONCLK] */
			udelay(1000);

		} else if (pll_id == MT658X_FH_MSDC_PLL) {

			fh_write32(MBIST_CFG_2, 0x00000009);	/* divide by 9+1 */

			fh_write32(CLK_CFG_3, 0x00000001);	/* enable it */
			udelay(1000);
			fh_write32(CLK_CFG_3, 0x00080001);	/* pll_pre_clk [1000: AD_MSDCPLL_H208M] */
			udelay(1000);
			fh_write32(CLK_CFG_3, 0x00080001);	/* pll_clk_sel [0000: pll_pre_clk] */
			udelay(1000);
			fh_write32(CLK_CFG_3, 0x00080F01);	/* abist_clk_sel [1111: pll_clk_out] */
			udelay(1000);

		} else if (pll_id == MT658X_FH_MM_PLL) {
			fh_write32(MBIST_CFG_2, 0x00000009);	/* divide by 9+1 */

			fh_write32(CLK_CFG_3, 0x00000001);	/* enable it */
			udelay(1000);
			fh_write32(CLK_CFG_3, 0x00090001);	/* pll_pre_clk [1001: AD_TVHDMI_H_CK] */
			udelay(1000);
			fh_write32(CLK_CFG_3, 0x00090001);	/* pll_clk_sel [0000: pll_pre_clk] */
			udelay(1000);
			fh_write32(CLK_CFG_3, 0x00090F01);	/* abist_clk_sel [1111: pll_clk_out] */
			udelay(1000);

		} else if (pll_id == MT658X_FH_VENC_PLL) {
			fh_write32(MBIST_CFG_2, 0x00000009);	/* divide by 9+1 */

			fh_write32(CLK_CFG_3, 0x00000001);	/* enable it */
			udelay(1000);
			fh_write32(CLK_CFG_3, 0x000A0001);	/* pll_pre_clk [1010: AD_LVDS_H180M_CK] */
			udelay(1000);
			fh_write32(CLK_CFG_3, 0x000A0001);	/* pll_clk_sel [0000: pll_pre_clk] */
			udelay(1000);
			fh_write32(CLK_CFG_3, 0x000A0F01);	/* abist_clk_sel [1111: pll_clk_out] */
			udelay(1000);

		}
	} else {
		FH_MSG("  bad argument!!");
	}

	g_curr_clkgen = pll_id;

	return count;

	/* return -EINVAL; */
}

#endif				/* MT_FH_CLK_GEN */
/* TODO: __init void mt_freqhopping_init(void) */
static void mt_fh_hal_init(void)
{
	int i;
	unsigned long flags;
#ifdef CONFIG_OF
	struct device_node *fh_node = NULL;
	struct device_node *apmix_node = NULL;
#endif

	FH_MSG("EN: %s", __func__);

	if (g_initialize == 1) {
		FH_MSG("already init!");
		return;
	}

#ifdef CONFIG_OF
	fh_node = of_find_compatible_node(NULL, NULL, "mediatek,mt2701-fhctl");
	if (fh_node) {
		/* Setup IO addresses */
		freqhopping_base = of_iomap(fh_node, 0);
		if (!freqhopping_base) {
			FH_MSG("freqhopping_base = 0x%lx\n", (unsigned long)freqhopping_base);
			return;
		}
	}

	apmix_node = of_find_compatible_node(NULL, NULL, "mediatek,mt2701-apmixedsys");
	if (apmix_node) {
		/* Setup IO addresses */
		apmixed_base = of_iomap(apmix_node, 0);
		if (!apmixed_base) {
			FH_MSG("apmixed_base = 0x%lx\n", (unsigned long)apmixed_base);
			return;
		}
	}

#endif

	/* init hopping table for mempll 200<->266 */
	memset(g_mempll_fh_table, 0, sizeof(g_mempll_fh_table));


	for (i = 0; i < MT_FHPLL_MAX; i++) {

		/* TODO: use the g_fh_pll[] to init the FHCTL */
		spin_lock_irqsave(&freqhopping_lock, flags);

		g_fh_pll[i].setting_id = 0;

		fh_write32(REG_FHCTL0_CFG + (i * 0x14), 0x00000000);	/* No SSC and FH enabled */
		fh_write32(REG_FHCTL0_UPDNLMT + (i * 0x14), 0x00000000);	/* clear all the settings */
		fh_write32(REG_FHCTL0_DDS + (i * 0x14), 0x00000000);	/* clear all the settings */

		/* TODO: do we need this */
		/* fh_write32(REG_FHCTL0_MON+(i*0x10), 0x00000000); //clear all the settings */

		spin_unlock_irqrestore(&freqhopping_lock, flags);
	}

	/* TODO: update each PLL status (go through g_fh_pll) */
	/* TODO: wait for sophie's table & query the EMI clock */
	/* TODO: ask sophie to call this init function during her init call (mt_clkmgr_init() ??) */
	/* TODO: call __freqhopping_ctrl() to init each pll */


	g_initialize = 1;
	/* register change.              enable_clock(MT_CG_PERI1_FHCTL, "FREQHOP") ; */

}

static void mt_fh_hal_lock(unsigned long *flags)
{
	spin_lock_irqsave(&freqhopping_lock, *flags);
}

static void mt_fh_hal_unlock(unsigned long *flags)
{
	spin_unlock_irqrestore(&freqhopping_lock, *flags);
}

static int mt_fh_hal_get_init(void)
{
	return g_initialize;
}

static int mt_fh_hal_is_support_DFS_mode(void)
{
	return true;
}

/* TODO: module_init(mt_freqhopping_init); */
/* TODO: module_init(mt_freqhopping_init); */
/* TODO: module_exit(cpufreq_exit); */

static struct mt_fh_hal_driver g_fh_hal_drv;

struct mt_fh_hal_driver *mt_get_fh_hal_drv(void)
{
	memset(&g_fh_hal_drv, 0, sizeof(g_fh_hal_drv));

	g_fh_hal_drv.fh_pll = g_fh_pll;
	g_fh_hal_drv.fh_usrdef = mt_ssc_fhpll_userdefined;
	g_fh_hal_drv.pll_cnt = MT658X_FH_PLL_TOTAL_NUM;
	g_fh_hal_drv.mempll = MT658X_FH_MEM_PLL;
	g_fh_hal_drv.mainpll = MT658X_FH_MAIN_PLL;
	g_fh_hal_drv.msdcpll = MT658X_FH_MSDC_PLL;
	g_fh_hal_drv.mmpll = MT658X_FH_MM_PLL;
	g_fh_hal_drv.vencpll = MT658X_FH_VENC_PLL;
	/* 8127 FHCTL ME */

	g_fh_hal_drv.mt_fh_hal_init = mt_fh_hal_init;

#if MT_FH_CLK_GEN
	g_fh_hal_drv.proc.clk_gen_read = freqhopping_clkgen_proc_read;
	g_fh_hal_drv.proc.clk_gen_write = freqhopping_clkgen_proc_write;
#endif

	g_fh_hal_drv.proc.dramc_read = freqhopping_dramc_proc_read;
	g_fh_hal_drv.proc.dramc_write = freqhopping_dramc_proc_write;
	g_fh_hal_drv.proc.dumpregs_read = freqhopping_dumpregs_proc_read;

	g_fh_hal_drv.proc.dvfs_read = freqhopping_dvfs_proc_read;
	g_fh_hal_drv.proc.dvfs_write = freqhopping_dvfs_proc_write;

	g_fh_hal_drv.mt_fh_hal_ctrl = __freqhopping_ctrl;
	g_fh_hal_drv.mt_fh_lock = mt_fh_hal_lock;
	g_fh_hal_drv.mt_fh_unlock = mt_fh_hal_unlock;
	g_fh_hal_drv.mt_fh_get_init = mt_fh_hal_get_init;

	g_fh_hal_drv.mt_fh_popod_restore = mt_fh_hal_popod_restore;
	g_fh_hal_drv.mt_fh_popod_save = mt_fh_hal_popod_save;

	g_fh_hal_drv.mt_l2h_mempll = mt_fh_hal_l2h_mempll;
	g_fh_hal_drv.mt_h2l_mempll = mt_fh_hal_h2l_mempll;
	g_fh_hal_drv.mt_l2h_dvfs_mempll = mt_fh_hal_l2h_dvfs_mempll;
	g_fh_hal_drv.mt_h2l_dvfs_mempll = mt_fh_hal_h2l_dvfs_mempll;
	g_fh_hal_drv.mt_dfs_armpll = mt_fh_hal_dfs_armpll;
	g_fh_hal_drv.mt_is_support_DFS_mode = mt_fh_hal_is_support_DFS_mode;
	g_fh_hal_drv.mt_dram_overclock = mt_fh_hal_dram_overclock;
	g_fh_hal_drv.mt_get_dramc = mt_fh_hal_get_dramc;
	g_fh_hal_drv.mt_fh_default_conf = mt_fh_hal_default_conf;

	return &g_fh_hal_drv;
}

static int __init mt_freqhopping_drv_init(void)
{
	g_initialize = 0;
	g_clk_en = 0;

	mt_freqhopping_init();

	return 0;
}


subsys_initcall(mt_freqhopping_drv_init);

/* TODO: module_exit(cpufreq_exit); */
