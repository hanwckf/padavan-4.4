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

#ifndef __MT_FHREG_H__
#define __MT_FHREG_H__

#include <linux/bitops.h>

/*--------------------------------------------------------------------------*/
/* Common Macro                                                             */
/*--------------------------------------------------------------------------*/

#define OFFSET_FHDMA_CFG	           0x0000
#define OFFSET_FHDMA_2G1BASE           0x0004
#define OFFSET_FHDMA_2G2BASE           0x0008
#define OFFSET_FHDMA_INTMDBASE         0x000C
#define OFFSET_FHDMA_EXTMDBASE         0x0010
#define OFFSET_FHDMA_BTBASE            0x0014
#define OFFSET_FHDMA_WFBASE            0x0018
#define OFFSET_FHDMA_FMBASE            0x001C
#define OFFSET_FHSRAM_CON	           0x0020
#define OFFSET_FHSRAM_WR               0x0024
#define OFFSET_FHSRAM_RD               0x0028
#define OFFSET_FHCTL_CFG	           0x002C

#define OFFSET_FHCTL_CON	           0x0030

#define OFFSET_FHCTL_2G1_CH            0x0034
#define OFFSET_FHCTL_2G2_CH            0x0038
#define OFFSET_FHCTL_INTMD_CH          0x003C
#define OFFSET_FHCTL_EXTMD_CH          0x0040
#define OFFSET_FHCTL_BT_CH             0x0044
#define OFFSET_FHCTL_WF_CH             0x0048
#define OFFSET_FHCTL_FM_CH             0x004C
#define OFFSET_FHCTL0_CFG	           0x0050
#define OFFSET_FHCTL0_UPDNLMT	       0x0054
#define OFFSET_FHCTL0_DDS	           0x0058
#define OFFSET_FHCTL0_DVFS	           0x005C
#define OFFSET_FHCTL0_MON	           0x0060

#define OFFSET_FHCTL1_CFG	           0x0064
#define OFFSET_FHCTL1_UPDNLMT	       0x0068
#define OFFSET_FHCTL1_DDS	           0x006C
#define OFFSET_FHCTL1_DVFS	           0x0070
#define OFFSET_FHCTL1_MON	           0x0074

#define OFFSET_FHCTL2_CFG	           0x0078
#define OFFSET_FHCTL2_UPDNLMT	       0x007C
#define OFFSET_FHCTL2_DDS	           0x0080
#define OFFSET_FHCTL2_DVFS	           0x0084
#define OFFSET_FHCTL2_MON	           0x0088

#define OFFSET_FHCTL3_CFG	           0x008C
#define OFFSET_FHCTL3_UPDNLMT	       0x0090
#define OFFSET_FHCTL3_DDS	           0x0094
#define OFFSET_FHCTL3_DVFS	           0x0098
#define OFFSET_FHCTL3_MON	           0x009C

#define OFFSET_FHCTL4_CFG	           0x00A0
#define OFFSET_FHCTL4_UPDNLMT          0x00A4
#define OFFSET_FHCTL4_DDS	           0x00A8
#define OFFSET_FHCTL4_DVFS	           0x00AC
#define OFFSET_FHCTL4_MON	           0x00B0

#define OFFSET_FHCTL5_CFG	           0x00B4
#define OFFSET_FHCTL5_UPDNLMT	       0x00B8
#define OFFSET_FHCTL5_DDS	           0x00BC
#define OFFSET_FHCTL5_DVFS	           0x00C0
#define OFFSET_FHCTL5_MON              0x00C4

/* 8127 FHCTL MB */
#define OFFSET_FHCTL6_CFG	           0x00C8
#define OFFSET_FHCTL6_UPDNLMT	       0x00CC
#define OFFSET_FHCTL6_DDS	           0x00D0
#define OFFSET_FHCTL6_DVFS	           0x00D4
#define OFFSET_FHCTL6_MON              0x00D8

#define OFFSET_FHCTL7_CFG	           0x00DC
#define OFFSET_FHCTL7_UPDNLMT	       0x00E0
#define OFFSET_FHCTL7_DDS	           0x00E4
#define OFFSET_FHCTL7_DVFS	           0x00E8
#define OFFSET_FHCTL7_MON              0x00EC
/* 8127 FHCTL ME */

#define FH_SFSTRX_DYS  (0xFU<<20)
#define FH_SFSTRX_DTS  (0xFU<<16)
#define FH_FHCTLX_SRHMODE  (0x1U<<5)
#define FH_SFSTRX_BP  (0x1U<<4)
#define FH_SFSTRX_EN  (0x1U<<2)
#define FH_FRDDSX_EN  (0x1U<<1)
#define FH_FHCTLX_EN  (0x1U<<0)
#define FH_FRDDSX_DNLMT   (0xFFU<<16)
#define FH_FRDDSX_UPLMT  (0xFFU)
#define FH_FHCTLX_PLL_TGL_ORG  (0x1U<<31)
#define FH_FHCTLX_PLL_ORG  (0xFFFFFU)
#define FH_FHCTLX_PAUSE  (0x1U<<31)
#define FH_FHCTLX_PRD  (0x1U<<30)
#define FH_SFSTRX_PRD  (0x1U<<29)
#define FH_FRDDSX_PRD (0x1U<<28)
#define FH_FHCTLX_STATE  (0xFU<<24)
#define FH_FHCTLX_PLL_CHG  (0x1U<<21)
#define FH_FHCTLX_PLL_DDS  (0xFFFFFU)

#ifndef CONFIG_OF
/* #define REG_ADDR(x)                 ((volatile u32*)(FHCTL_BASE + OFFSET_##x)) */
#define REG_ADDR(x)                 (FHCTL_BASE + OFFSET_##x)

#define REG_FHDMA_CFG	           REG_ADDR(FHDMA_CFG)
#define REG_FHDMA_2G1BASE        REG_ADDR(FHDMA_2G1BASE)
#define REG_FHDMA_2G2BASE        REG_ADDR(FHDMA_2G2BASE)
#define REG_FHDMA_INTMDBASE      REG_ADDR(FHDMA_INTMDBASE)
#define REG_FHDMA_EXTMDBASE      REG_ADDR(FHDMA_EXTMDBASE)
#define REG_FHDMA_BTBASE         REG_ADDR(FHDMA_BTBASE)
#define REG_FHDMA_WFBASE         REG_ADDR(FHDMA_WFBASE)
#define REG_FHDMA_FMBASE         REG_ADDR(FHDMA_FMBASE)
#define REG_FHSRAM_CON	         REG_ADDR(FHSRAM_CON)
#define REG_FHSRAM_WR            REG_ADDR(FHSRAM_WR)
#define REG_FHSRAM_RD            REG_ADDR(FHSRAM_RD)
#define REG_FHCTL_CFG	           REG_ADDR(FHCTL_CFG)
#define REG_FHCTL_CON	           REG_ADDR(FHCTL_CON)
#define REG_FHCTL_2G1_CH         REG_ADDR(FHCTL_2G1_CH)
#define REG_FHCTL_2G2_CH         REG_ADDR(FHCTL_2G2_CH)
#define REG_FHCTL_INTMD_CH       REG_ADDR(FHCTL_INTMD_CH)
#define REG_FHCTL_EXTMD_CH       REG_ADDR(FHCTL_EXTMD_CH)
#define REG_FHCTL_BT_CH          REG_ADDR(FHCTL_BT_CH)
#define REG_FHCTL_WF_CH          REG_ADDR(FHCTL_WF_CH)
#define REG_FHCTL_FM_CH          REG_ADDR(FHCTL_FM_CH)
#define REG_FHCTL0_CFG	         REG_ADDR(FHCTL0_CFG)
#define REG_FHCTL0_UPDNLMT	     REG_ADDR(FHCTL0_UPDNLMT)
#define REG_FHCTL0_DDS	         REG_ADDR(FHCTL0_DDS)
#define REG_FHCTL0_DVFS	         REG_ADDR(FHCTL0_DVFS)
#define REG_FHCTL0_MON	         REG_ADDR(FHCTL0_MON)
#define REG_FHCTL1_CFG	         REG_ADDR(FHCTL1_CFG)
#define REG_FHCTL1_UPDNLMT	     REG_ADDR(FHCTL1_UPDNLMT)
#define REG_FHCTL1_DDS	         REG_ADDR(FHCTL1_DDS)
#define REG_FHCTL1_DVFS	         REG_ADDR(FHCTL1_DVFS)
#define REG_FHCTL1_MON	         REG_ADDR(FHCTL1_MON)
#define REG_FHCTL2_CFG	         REG_ADDR(FHCTL2_CFG)
#define REG_FHCTL2_UPDNLMT	     REG_ADDR(FHCTL2_UPDNLMT)
#define REG_FHCTL2_DDS	         REG_ADDR(FHCTL2_DDS)
#define REG_FHCTL2_DVFS	         REG_ADDR(FHCTL2_DVFS)
#define REG_FHCTL2_MON	         REG_ADDR(FHCTL2_MON)
#define REG_FHCTL3_CFG	         REG_ADDR(FHCTL3_CFG)
#define REG_FHCTL3_UPDNLMT	     REG_ADDR(FHCTL3_UPDNLMT)
#define REG_FHCTL3_DDS	         REG_ADDR(FHCTL3_DDS)
#define REG_FHCTL3_DVFS	         REG_ADDR(FHCTL3_DVFS)
#define REG_FHCTL3_MON	         REG_ADDR(FHCTL3_MON)
#define REG_FHCTL4_CFG	         REG_ADDR(FHCTL4_CFG)
#define REG_FHCTL4_UPDNLMT       REG_ADDR(FHCTL4_UPDNLMT)
#define REG_FHCTL4_DDS	         REG_ADDR(FHCTL4_DDS)
#define REG_FHCTL4_DVFS	         REG_ADDR(FHCTL4_DVFS)
#define REG_FHCTL4_MON	         REG_ADDR(FHCTL4_MON)
#define REG_FHCTL5_CFG	         REG_ADDR(FHCTL5_CFG)
#define REG_FHCTL5_UPDNLMT	     REG_ADDR(FHCTL5_UPDNLMT)
#define REG_FHCTL5_DDS	         REG_ADDR(FHCTL5_DDS)
#define REG_FHCTL5_DVFS	         REG_ADDR(FHCTL5_DVFS)
#define REG_FHCTL5_MON           REG_ADDR(FHCTL5_MON)
/* 8127 FHCTL MB */
#define REG_FHCTL6_CFG	         REG_ADDR(FHCTL6_CFG)
#define REG_FHCTL6_UPDNLMT	     REG_ADDR(FHCTL6_UPDNLMT)
#define REG_FHCTL6_DDS	         REG_ADDR(FHCTL6_DDS)
#define REG_FHCTL6_DVFS	         REG_ADDR(FHCTL6_DVFS)
#define REG_FHCTL6_MON           REG_ADDR(FHCTL6_MON)
#define REG_FHCTL7_CFG	         REG_ADDR(FHCTL7_CFG)
#define REG_FHCTL7_UPDNLMT	     REG_ADDR(FHCTL7_UPDNLMT)
#define REG_FHCTL7_DDS	         REG_ADDR(FHCTL7_DDS)
#define REG_FHCTL7_DVFS	         REG_ADDR(FHCTL7_DVFS)
#define REG_FHCTL7_MON           REG_ADDR(FHCTL7_MON)
/* 8127 FHCTL ME */
#endif

static inline unsigned int uffs(unsigned int x)
{
	unsigned int r = 1;

	if (!x)
		return 0;
	if (!(x & 0xffff)) {
		x >>= 16;
		r += 16;
	}
	if (!(x & 0xff)) {
		x >>= 8;
		r += 8;
	}
	if (!(x & 0xf)) {
		x >>= 4;
		r += 4;
	}
	if (!(x & 3)) {
		x >>= 2;
		r += 2;
	}
	if (!(x & 1)) {
		x >>= 1;
		r += 1;
	}
	return r;
}

#define fh_read8(reg)           readb(reg)
#define fh_read16(reg)          readw(reg)
#define fh_read32(reg)          readl((void __iomem *)reg)
#define fh_write8(reg, val)      mt_reg_sync_writeb((val), (reg))
#define fh_write16(reg, val)     mt_reg_sync_writew((val), (reg))
#define fh_write32(reg, val)     mt_reg_sync_writel((val), (reg))

/* #define fh_set_bits(reg,bs)     ((*(volatile u32*)(reg)) |= (u32)(bs)) */
/* #define fh_clr_bits(reg,bs)     ((*(volatile u32*)(reg)) &= ~((u32)(bs))) */

#define fh_set_field(reg, field, val) \
	do {	\
		volatile unsigned int tv = fh_read32(reg);	\
		tv &= ~(field); \
		tv |= ((val) << (uffs((unsigned int)field) - 1)); \
		fh_write32(reg, tv); \
	} while (0)
#define fh_get_field(reg, field, val) \
	do {	\
		volatile unsigned int tv = fh_read32(reg);	\
		val = ((tv & (field)) >> (uffs((unsigned int)field) - 1)); \
	} while (0)



#endif				/* #ifndef __MT_FHREG_H__ */
