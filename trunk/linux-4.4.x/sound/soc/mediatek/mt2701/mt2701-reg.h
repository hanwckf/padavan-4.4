/*
 * mt2701-reg.h  --  Mediatek 2701 audio driver reg definition
 *
 * Copyright (c) 2016 MediaTek Inc.
 * Author: Garlic Tseng <garlic.tseng@mediatek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _MT2701_REG_H_
#define _MT2701_REG_H_

#include <linux/delay.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/pm_runtime.h>
#include <sound/soc.h>
#include "mt2701-afe-common.h"

/*****************************************************************************
 *                  R E G I S T E R       D E F I N I T I O N
 *****************************************************************************/
#define AUDIO_TOP_CON0 0x0000
#define AUDIO_TOP_CON1 0x0004
#define AUDIO_TOP_CON2 0x0008
#define AUDIO_TOP_CON4 0x0010
#define AUDIO_TOP_CON5 0x0014
#define AFE_DAIBT_CON0 0x001c
#define AFE_MRGIF_CON 0x003c
#define ASMI_TIMING_CON1 0x0100
#define ASMO_TIMING_CON1 0x0104
#define AFE_SGEN_CON0 0x01f0
#define PWR1_ASM_CON1 0x0108
#define AFE_TDNO1_CON1 0x0290
#define AFE_TDMO1_CON2 0x0294
#define AFE_TDMO1_CONN_CON1 0x0298
#define AFE_TDMO1_CONN_CON0 0x029C
#define AFE_TDNO2_CON1 0x02A0
#define AFE_TDMO2_CON2 0x02A4
#define AFE_TDMO2_CONN_CON1 0x02A8
#define AFE_TDMO2_CONN_CON0 0x02AC
#define AFE_TDNIN_CON1 0x02B8
#define AFE_TDMIN_CON2 0x02BC
#define ASYS_TOP_CON 0x0600
#define ASYS_I2SIN1_CON 0x0604
#define ASYS_I2SIN2_CON 0x0608
#define ASYS_I2SIN3_CON 0x060c
#define ASYS_I2SIN4_CON 0x0610
#define ASYS_I2SIN5_CON 0x0614
#define ASYS_I2SO1_CON 0x061C
#define ASYS_I2SO2_CON 0x0620
#define ASYS_I2SO3_CON 0x0624
#define ASYS_I2SO4_CON 0x0628
#define ASYS_I2SO5_CON 0x062c
#define AFE_PCM_INTF_CON1 0x063C
#define AFE_PCM_INTF_CON2 0x0640
#define PWR2_TOP_CON 0x0634
#define AFE_CONN0 0x06c0
#define AFE_CONN1 0x06c4
#define AFE_CONN2 0x06c8
#define AFE_CONN3 0x06cc
#define AFE_CONN4 0x06d0
#define AFE_CONN5 0x06d4
#define AFE_CONN10 0x06e8
#define AFE_CONN11 0x06ec
#define AFE_CONN14 0x06f8
#define AFE_CONN15 0x06fc
#define AFE_CONN16 0x0700
#define AFE_CONN17 0x0704
#define AFE_CONN18 0x0708
#define AFE_CONN19 0x070c
#define AFE_CONN20 0x0710
#define AFE_CONN21 0x0714
#define AFE_CONN22 0x0718
#define AFE_CONN23 0x071c
#define AFE_CONN24 0x0720
#define AFE_CONN25 0x0724
#define AFE_CONN26 0x0728
#define AFE_CONN32 0x0740
#define AFE_CONN41 0x0764
#define ASYS_IRQ1_CON 0x0780
#define ASYS_IRQ2_CON 0x0784
#define ASYS_IRQ3_CON 0x0788
#define ASYS_IRQ4_CON 0x078c
#define ASYS_IRQ5_CON 0x0790
#define ASYS_IRQ6_CON 0x0794
#define ASYS_IRQ_CLR 0x07c0
#define ASYS_IRQ_STATUS 0x07c4
#define PWR2_ASM_CON1 0x1070
#define AFE_DAC_CON0 0x1200
#define AFE_DAC_CON1 0x1204
#define AFE_DAC_CON2 0x1208
#define AFE_DAC_CON3 0x120c
#define AFE_DAC_CON4 0x1210
#define AFE_MEMIF_HD_CON1 0x121c
#define AFE_MEMIF_PBUF_SIZE 0x1238
#define AFE_MEMIF_HD_CON0 0x123c
#define AFE_DL1_BASE 0x1240
#define AFE_DL1_CUR 0x1244
#define AFE_DL2_BASE 0x1250
#define AFE_DL2_CUR 0x1254
#define AFE_DL3_BASE 0x1260
#define AFE_DL3_CUR 0x1264
#define AFE_DL4_BASE 0x1270
#define AFE_DL4_CUR 0x1274
#define AFE_DL5_BASE 0x1280
#define AFE_DL5_CUR 0x1284
#define AFE_DLMCH_BASE 0x12a0
#define AFE_DLMCH_CUR 0x12a4
#define AFE_ARB1_BASE 0x12b0
#define AFE_ARB1_CUR 0x12b4
#define AFE_VUL_BASE 0x1300
#define AFE_VUL_CUR 0x130c
#define AFE_UL2_BASE 0x1310
#define AFE_UL2_END 0x1318
#define AFE_UL2_CUR 0x131c
#define AFE_UL3_BASE 0x1320
#define AFE_UL3_END 0x1328
#define AFE_UL3_CUR 0x132c
#define AFE_UL4_BASE 0x1330
#define AFE_UL4_END 0x1338
#define AFE_UL4_CUR 0x133c
#define AFE_UL5_BASE 0x1340
#define AFE_UL5_END 0x1348
#define AFE_UL5_CUR 0x134c
#define AFE_AWB2_BASE 0x12e0
#define AFE_AWB2_END 0x12e8
#define AFE_AWB2_CUR 0x12ec
#define AFE_DAI_BASE 0x1370
#define AFE_DAI_CUR 0x137c
#define AFE_PCMI_BASE 0x1330
#define AFE_PCMI_CUR 0x1334
#define AFE_PCMO_BASE 0x12c0
#define AFE_PCMO_CUR 0x12c4
#define AFE_MEMIF_BASE_MSB 0x0304
#define AFE_MEMIF_END_MSB 0x0308
#define AFE_TDM_G1_CON1 0x0290
#define AFE_TDM_G1_CON2 0x0294
#define AFE_TDM_G1_CONN_CON0 0x029c
#define AFE_TDM_G1_CONN_CON1 0x0298
#define AFE_TDM_G1_BASE 0x1280
#define AFE_TDM_G1_CUR 0x1284
#define AFE_TDM_G1_END 0x1288
#define AFE_TDM_G2_CON1 0x02A0
#define AFE_TDM_G2_CON2 0x02A4
#define AFE_TDM_G2_CONN_CON0 0x02Ac
#define AFE_TDM_G2_CONN_CON1 0x02A8
#define AFE_TDM_G2_BASE 0x1290
#define AFE_TDM_G2_CUR 0x1294
#define AFE_TDM_G2_END 0x1298
#define AFE_TDM_AGENT_CFG 0x02c0
#define AFE_TDM_IN_CON1 0x02B8
#define AFE_TDM_IN_CON2 0x02BC
#define AFE_TDM_IN_BASE 0x1360
#define AFE_TDM_IN_CUR 0x136c
#define AFE_TDM_IN_END 0x1368
#define AFE_TDM_AGENT_CFG 0x02c0
#define AFE_MCH_OUT_CFG 0x0300
#define AFE_ASRC_PCMO_CON0   0x0ac0
#define AFE_ASRC_PCMO_CON1   0x0ac4
#define AFE_ASRC_PCMO_CON2   0x0ac8
#define AFE_ASRC_PCMO_CON3   0x0acc
#define AFE_ASRC_PCMO_CON4   0x0ad0
#define AFE_ASRC_PCMO_CON5   0x0ad4
#define AFE_ASRC_PCMO_CON6   0x0ad8
#define AFE_ASRC_PCMO_CON7   0x0adc
#define AFE_ASRC_PCMO_CON10  0x0ae8
#define AFE_ASRC_PCMO_CON11  0x0aec
#define AFE_ASRC_PCMO_CON13  0x0af4
#define AFE_ASRC_PCMO_CON14  0x0af8
#define AFE_ASRC_PCMI_CON0   0x0a80
#define AFE_ASRC_PCMI_CON1   0x0a84
#define AFE_ASRC_PCMI_CON2   0x0a88
#define AFE_ASRC_PCMI_CON3   0x0a8c
#define AFE_ASRC_PCMI_CON4   0x0a90
#define AFE_ASRC_PCMI_CON5   0x0a94
#define AFE_ASRC_PCMI_CON6   0x0a98
#define AFE_ASRC_PCMI_CON7   0x0a9c
#define AFE_ASRC_PCMI_CON10  0x0aa8
#define AFE_ASRC_PCMI_CON11  0x0aac
#define AFE_ASRC_PCMI_CON13  0x0ab4
#define AFE_ASRC_PCMI_CON14  0x0ab8


#define FPGA_CFG0 0x04e0
#define FPGA_CFG1 0x04e4
#define FPGA_CFG2 0x04e8
#define FPGA_CFG3 0x04ec


/* AUDIO_TOP_CON0 (0x0000) */
#define AUDIO_TOP_CON0_A1SYS_A2SYS_ON	(0x3 << 0)
#define AUDIO_TOP_CON0_PDN_AFE		(0x1 << 2)
#define AUDIO_TOP_CON0_PDN_APLL_CK	(0x1 << 23)

/* AUDIO_TOP_CON1 (0x0004) */
#define AUDIO_TOP_CON1_TDMIN_CLK_AGENT_PDN     (0x1<<15)
#define AUDIO_TOP_CON1_TDMIN_PLL_SEL           (0x1<<16)
#define AUDIO_TOP_CON1_TDMIN_PLL_SEL_APLL1     (0x0<<16)
#define AUDIO_TOP_CON1_TDMIN_PLL_SEL_APLL2     (0x1<<16)
#define AUDIO_TOP_CON1_TDMIN_BCK_EN           (0x1<<17)
#define AUDIO_TOP_CON1_TDMIN_BCK_INV           (0x1<<18)
/* tdmin_bck = clock source * (1 / x+1)*/
#define AUDIO_TOP_CON1_TDMIN_BCK_PLL_DIV       (0xff<<24)
#define AUDIO_TOP_CON1_TDMIN_BCK_PLL_DIV_SET(x)       ((x)<<24)

/* AUDIO_TOP_CON4 (0x0010) */
#define AUDIO_TOP_CON4_I2SO1_PWN	(0x1 << 6)
#define AUDIO_TOP_CON4_PDN_A1SYS	(0x1 << 21)
#define AUDIO_TOP_CON4_PDN_A2SYS	(0x1 << 22)
#define AUDIO_TOP_CON4_PDN_AFE_CONN	(0x1 << 23)
#define AUDIO_TOP_CON4_PDN_PCM          (0x1 << 24)
#define AUDIO_TOP_CON4_PDN_MRGIF	(0x1 << 25)


/* AFE_DAIBT_CON0 (0x001c) */
#define AFE_DAIBT_CON0_DAIBT_EN		(0x1 << 0)
#define AFE_DAIBT_CON0_BT_FUNC_EN	(0x1 << 1)
#define AFE_DAIBT_CON0_BT_FUNC_RDY	(0x1 << 3)
#define AFE_DAIBT_CON0_BT_WIDE_MODE_EN	(0x1 << 9)
#define AFE_DAIBT_CON0_MRG_USE		(0x1 << 12)

/* PWR1_ASM_CON1 (0x0108) */
#define PWR1_ASM_CON1_INIT_VAL		(0x492)

/* AFE_MRGIF_CON (0x003c) */
#define AFE_MRGIF_CON_MRG_EN		(0x1 << 0)
#define AFE_MRGIF_CON_MRG_I2S_EN	(0x1 << 16)
#define AFE_MRGIF_CON_I2S_MODE_MASK	(0xf << 20)
#define AFE_MRGIF_CON_I2S_MODE_32K	(0x4 << 20)

/* ASYS_I2SO1_CON (0x061c) */
#define ASYS_I2SO1_CON_FS		(0x1f << 8)
#define ASYS_I2SO1_CON_FS_SET(x)	((x) << 8)
#define ASYS_I2SO1_CON_MULTI_CH		(0x1 << 16)
#define ASYS_I2SO1_CON_SIDEGEN		(0x1 << 30)
#define ASYS_I2SO1_CON_I2S_EN		(0x1 << 0)
/* 0:EIAJ 1:I2S */
#define ASYS_I2SO1_CON_I2S_MODE		(0x1 << 3)
#define ASYS_I2SO1_CON_WIDE_MODE	(0x1 << 1)
#define ASYS_I2SO1_CON_WIDE_MODE_SET(x)	((x) << 1)

/* PWR2_TOP_CON (0x0634) */
#define PWR2_TOP_CON_INIT_VAL		(0xffe1ffff)

/* ASYS_IRQ_CLR (0x07c0) */
#define ASYS_IRQ_CLR_ALL		(0xffffffff)

/* PWR2_ASM_CON1 (0x1070) */
#define PWR2_ASM_CON1_INIT_VAL		(0x492492)

/* AFE_DAC_CON0 (0x1200) */
#define AFE_DAC_CON0_AFE_ON		(0x1 << 0)

/* AFE_MEMIF_PBUF_SIZE (0x1238) */
#define AFE_MEMIF_PBUF_SIZE_DLM_MASK		(0x1 << 29)
#define AFE_MEMIF_PBUF_SIZE_PAIR_INTERLEAVE	(0x0 << 29)
#define AFE_MEMIF_PBUF_SIZE_FULL_INTERLEAVE	(0x1 << 29)
#define DLMCH_BIT_WIDTH_MASK			(0x1 << 28)
#define AFE_MEMIF_PBUF_SIZE_DLM_CH_MASK		(0xf << 24)
#define AFE_MEMIF_PBUF_SIZE_DLM_CH(x)		((x) << 24)
#define AFE_MEMIF_PBUF_SIZE_DLM_BYTE_MASK	(0x3 << 12)
#define AFE_MEMIF_PBUF_SIZE_DLM_32BYTES		(0x1 << 12)

/* I2S in/out register bit control */
#define ASYS_I2S_CON_FS			(0x1f << 8)
#define ASYS_I2S_CON_FS_SET(x)		((x) << 8)
#define ASYS_I2S_CON_RESET		(0x1 << 30)
#define ASYS_I2S_CON_I2S_EN		(0x1 << 0)
#define ASYS_I2S_CON_I2S_COUPLE_MODE	(0x1 << 17)
/* 0:EIAJ 1:I2S */
#define ASYS_I2S_CON_I2S_MODE		(0x1 << 3)
#define ASYS_I2S_CON_I2S_MODE_SET(x)	(x << 3)
#define ASYS_I2S_CON_RIGHT_J		(0x1 << 14)
#define ASYS_I2S_CON_RIGHT_J_SET(x)	(x << 14)
#define ASYS_I2S_CON_WIDE_MODE		(0x1 << 1)
#define ASYS_I2S_CON_WIDE_MODE_SET(x)	((x) << 1)
#define ASYS_I2S_IN_PHASE_FIX		(0x1 << 31)
#define ASYS_I2S_CON_INV_LRCK		(0x1 << 5)
#define ASYS_I2S_CON_INV_LRCK_SET(x)	(x << 5)
#define ASYS_I2S_CON_INV_BCK		(0x1 << 6)
#define ASYS_I2S_CON_INV_BCK_SET(x)	(x << 6)

/* TDM in/out register bit control */
#define AFE_TDM_CON_LRCK_WIDTH              (0x1ff << 23)
#define AFE_TDM_CON_LRCK_WIDTH_SET(x)       ((x) << 23)
#define AFE_TDM_CON_INOUT_SYNC     (0x1 << 18)
#define AFE_TDM_CON_INOUT_SYNC_SET(x)   ((x) << 18)
#define AFE_TDM_CON_IN_BCK        (0x1 << 19)
#define AFE_TDM_CON_IN_BCK_SET(x)  ((x) << 19)
#define AFE_TDM_CON_CH              (0x7 << 12)
#define AFE_TDM_CON_CH_SET(x)       ((x) << 12)
#define AFE_TDM_CON_WLEN           (0x3 << 8)
#define AFE_TDM_CON_WLEN_SET(x)    ((x) << 8)
#define AFE_TDM_CON_LRCK_DELAY             (0x1 << 5)
#define AFE_TDM_CON_LRCK_DELAY_SET(x)    ((x) << 5)
#define AFE_TDM_CON_DELAY             (0x1 << 3)
#define AFE_TDM_CON_DELAY_SET(x)    ((x) << 3)
#define AFE_TDM_CON_INV_LRCK             (0x1 << 2)
#define AFE_TDM_CON_INV_LRCK_SET(x)    ((x) << 2)
#define AFE_TDM_CON_INV_BCK             (0x1 << 1)
#define AFE_TDM_CON_INV_BCK_SET(x)    ((x) << 1)
#define AFE_TDM_CON_EN             (0x1 << 0)

/* TDM in channel mask */
#define AFE_TDM_IN_DISABLE_OUT         (0xff)
#define AFE_TDM_IN_DISABLE_OUT_SET(x)  (x)
#define AFE_TDM_IN_MASK_ODD            (0xff << 8)
#define AFE_TDM_IN_MASK_ODD_SET(x)     (x<<8)

/* FPGA0*/
#define DAC_2CH_MCLK_DIVIDER_MASK    (0xFF<<8)
#define DAC_2CH_MLK_DIVIDER_POS              8
#define DAC_2CH_MCLK_CLK_DOMAIN_MASK (0x1<<16)
#define DAC_2CH_MCLK_CLK_DOMAIN_48   (0x0<<16)
#define DAC_2CH_MCLK_CLK_DOMAIN_44   (0x1<<16)
#define XCLK_DIVIDER_MASK            (0xFF<<24)
#define XCLK_DIVIDER_POS                    24

/* PCM_INTF */
#define AFE_PCM_EX_MODEM          (0x1 << 17)
#define AFE_PCM_EX_MODEM_SET(x)       ((x) << 17)
#define AFE_PCM_24BIT          (0x1 << 16)
#define AFE_PCM_24BIT_SET(x)       ((x) << 16)
#define AFE_PCM_WLEN           (0x3 << 14)
#define AFE_PCM_WLEN_SET(x)       ((x) << 14)
#define AFE_PCM_BYP_ASRC           (0x1 << 6)
#define AFE_PCM_BYP_ASRC_SET(x)       ((x) << 6)
#define AFE_PCM_SLAVE              (0x1 << 5)
#define AFE_PCM_SLAVE_SET(x)       ((x) << 5)
#define AFE_PCM_MODE              (0x3 << 3)
#define AFE_PCM_MODE_SET(x)       ((x) << 3)
#define AFE_PCM_FMT              (0x3 << 1)
#define AFE_PCM_FMT_SET(x)       ((x) << 1)
#define AFE_PCM_EN              (0x1 << 0)
#define AFE_PCM_EN_SET(x)       ((x) << 0)

/* AFE_ASRC_PCMO_CON */
#define AFE_PCM_ASRC_O16BIT          (0x1 << 19)
#define AFE_PCM_ASRC_O16BIT_SET(x)       ((x) << 19)
#define AFE_PCM_ASRC_MONO          (0x1 << 16)
#define AFE_PCM_ASRC_MONO_SET(x)       ((x) << 16)
#define AFE_PCM_ASRC_OFS          (0x3 << 14)
#define AFE_PCM_ASRC_OFS_SET(x)       ((x) << 14)
#define AFE_PCM_ASRC_IFS          (0x3 << 12)
#define AFE_PCM_ASRC_IFS_SET(x)       ((x) << 12)
#define AFE_PCM_ASRC_IIR          (0x1 << 11)
#define AFE_PCM_ASRC_IIR_SET(x)       ((x) << 11)
#define AFE_PCM_ASRC_PALETTE          (0xffffff)
#define AFE_PCM_ASRC_PALETTE_SET(x)       ((x))
#define AFE_PCM_ASRC_TH          (0xffffff)
#define AFE_PCM_ASRC_TH_SET(x)       ((x))
#define AFE_PCM_ASRC_CLR         (0x1 << 4)
#define AFE_PCM_ASRC_EN         (0x1 << 0)
#define AFE_PCM_ASRC_EN_SET(x)       ((x))

#define AFE_END_ADDR 0x15e0
#endif
