/*
 * mt7622-reg.h  --  Mediatek 7622 audio driver reg definition
 *
 * Copyright (c) 2017 MediaTek Inc.
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

#ifndef _MT7622_REG_H_
#define _MT7622_REG_H_

#include <linux/delay.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/pm_runtime.h>
#include <sound/soc.h>
#include "mt7622-afe-common.h"

/*****************************************************************************
 *                  R E G I S T E R       D E F I N I T I O N
 *****************************************************************************/
#define AUDIO_TOP_CON0 0x0000
#define AUDIO_TOP_CON1 0x0004
#define AUDIO_TOP_CON4 0x0010
#define AUDIO_TOP_CON5 0x0014
#define AFE_DAIBT_CON0 0x001c
#define AFE_MRGIF_CON 0x003c
#define AFE_TDMOUT_CLKDIV_CFG 0x0078
#define AFE_TDMIN_CLKDIV_CFG 0x007C


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

#define AFE_IRQ_CON 0x03A0
#define AFE_IRQ_STATUS 0x03A4
#define AFE_IRQ_CLR 0x03A8
#define AFE_IRQ_CNT1 0x03AC
#define AFE_IRQ_CNT2 0x03B0
#define AFE_IRQ_MON2 0x03B8
#define AFE_IRQ_CNT5 0x03BC
#define AFE_IRQ1_CNT_MON 0x03C0
#define AFE_IRQ2_CNT_MON 0x03C4
#define AFE_IRQ1_EN_CNT_MON 0x03C8


/* TDM Memory interface */
#define AFE_TDM_OUT_CON0 0x0370
#define AFE_TDM_OUT_BASE 0x0374
#define AFE_TDM_OUT_CUR 0x0378
#define AFE_TDM_OUT_END 0x037C

#define AFE_TDM_IN_BASE 0x13D0
#define AFE_TDM_IN_CUR  0x13D4
#define AFE_TDM_IN_END  0x13D8

#define AFE_TDM_CON1 0x0054
#define AFE_TDM_IN_CON1 0x0060

#define AFE_IRQ_CNT11 0x171C
#define AFE_IRQ_CNT12 0x1720
#define AFE_IRQ_ACC1_CNT  0x1700
#define AFE_IRQ_ACC2_CNT  0x1704
#define AFE_IRQ_ACC1_CNT_MON1 0x1708
#define AFE_IRQ_ACC1_CNT_MON2 0x170C
#define AFE_IRQ_ACC2_CNT_MON  0x1718
#define AFE_IRQ11_CNT_MON  0x1724
#define AFE_IRQ12_CNT_MON   0x1728

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
#define PWR2_TOP_CON 0x0634
#define AFE_CONN0 0x06c0
#define AFE_CONN1 0x06c4
#define AFE_CONN2 0x06c8
#define AFE_CONN3 0x06cc
#define AFE_CONN4 0x06d0
#define AFE_CONN5 0x06d4
#define AFE_CONN6 0x06d8
#define AFE_CONN7 0x06dc
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
#define AFE_CONN41 0x0764
#define ASYS_IRQ1_CON 0x0780
#define ASYS_IRQ2_CON 0x0784
#define ASYS_IRQ3_CON 0x0788
#define ASYS_IRQ4_CON 0x078c
#define ASYS_IRQ5_CON 0x0790
#define ASYS_IRQ6_CON 0x0794
#define ASYS_IRQ7_CON 0x0798
#define ASYS_IRQ8_CON 0x079C
#define ASYS_IRQ9_CON 0x07A0
#define ASYS_IRQ10_CON 0x07A4
#define ASYS_IRQ11_CON 0x07A8
#define ASYS_IRQ_CLR 0x07c0
#define ASYS_IRQ_STATUS 0x07c4
#define PWR2_ASM_CON1 0x1070
#define AFE_DAC_CON0 0x1200
#define AFE_DAC_CON1 0x1204
#define AFE_DAC_CON2 0x1208
#define AFE_DAC_CON3 0x120c
#define AFE_DAC_CON4 0x1210
#define AFE_DAC_MON0 0x1218
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
#define AFE_DAI_BASE 0x1370
#define AFE_DAI_CUR 0x137c
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
#define AFE_TDM_AGENT_CFG 0x02c0
#define AFE_MCH_OUT_CFG 0x0300

#define FPGA_CFG0 0x04e0
#define FPGA_CFG1 0x04e4
#define FPGA_CFG2 0x04e8
#define FPGA_CFG3 0x04ec


/* AUDIO_TOP_CON0 (0x0000) */
#define AUDIO_TOP_CON0_A1SYS_A2SYS_ON	(0x3 << 0)
#define AUDIO_TOP_CON0_PDN_AFE		(0x1 << 2)
#define AUDIO_TOP_CON0_PDN_APLL_CK	(0x1 << 23)

/* tdmin_bck = clock source * (1 / x+1)*/
#define AUDIO_TOP_CON1_TDMIN_BCK_PLL_DIV       (0xff<<24)
#define AUDIO_TOP_CON1_TDMIN_BCK_PLL_DIV_SET(x)       ((x)<<24)

/* AFE_TDMOUT_CLKDIV_CFG (0x0078) */
#define TDMOUT_PDN_M             (0x1 << 0)
#define TDMOUT_PDN_B             (0x1 << 1)
#define TDMOUT_MCK_SEL           (0x1 << 2)
#define TDMOUT_INV_M             (0x1 << 4)
#define TDMOUT_INV_B             (0x1 << 5)
#define TDMOUT_CLK_DIV_M         (0xFFF << 8)
#define TDMOUT_CLK_DIV_B         (0xFFF << 20)

/* AFE_TDMIN_CLKDIV_CFG (0x007c) */
#define TDMIN_PDN_M              (0x1 << 0)
#define TDMIN_PDN_B              (0x1 << 1)
#define TDMIN_MCK_SEL            (0x1 << 2)
#define TDMIN_INV_M              (0x1 << 4)
#define TDMIN_INV_B              (0x1 << 5)
#define TDMIN_CLK_DIV_M          (0xFFF << 8)
#define TDMIN_CLK_DIV_B          (0xFFF << 20)



#define AUDIO_TOP_CON2_TDMOUT1_PLL_SEL           (0x1<<0)
#define AUDIO_TOP_CON2_TDMOUT1_PLL_SEL_APLL1     (0x0<<0)
#define AUDIO_TOP_CON2_TDMOUT1_PLL_SEL_APLL2     (0x1<<0)
#define AUDIO_TOP_CON2_TDMOUT1_BCK_EN            (0x1<<1)
#define AUDIO_TOP_CON2_TDMOUT1_BCK_INV           (0x1<<2)
#define AUDIO_TOP_CON2_TDMOUT1_CLK_AGENT_PDN     (0x1<<3)
/* tdmin_bck = clock source * (1 / x+1)*/
#define AUDIO_TOP_CON2_TDMOUT1_BCK_PLL_DIV       (0xff<<8)
#define AUDIO_TOP_CON2_TDMOUT1_BCK_PLL_DIV_SET(x)       ((x)<<8)
#define AUDIO_TOP_CON2_TDMOUT2_CLK_AGENT_PDN     (0x1<<4)
#define AUDIO_TOP_CON2_TDMOUT2_PLL_SEL           (0x1<<16)
#define AUDIO_TOP_CON2_TDMOUT2_PLL_SEL_APLL1     (0x0<<16)
#define AUDIO_TOP_CON2_TDMOUT2_PLL_SEL_APLL2     (0x1<<16)
#define AUDIO_TOP_CON2_TDMOUT2_BCK_EN               (0x1<<17)
#define AUDIO_TOP_CON2_TDMOUT2_BCK_INV           (0x1<<18)
/* tdmin_bck = clock source * (1 / x+1)*/
#define AUDIO_TOP_CON2_TDMOUT2_BCK_PLL_DIV       (0xff<<24)
#define AUDIO_TOP_CON2_TDMOUT2_BCK_PLL_DIV_SET(x)       ((x)<<24)

/* AUDIO_TOP_CON4 (0x0010) */
#define AUDIO_TOP_CON4_I2SO1_PWN	(0x1 << 6)
#define AUDIO_TOP_CON4_PDN_A1SYS	(0x1 << 21)
#define AUDIO_TOP_CON4_PDN_A2SYS	(0x1 << 22)
#define AUDIO_TOP_CON4_PDN_AFE_CONN	(0x1 << 23)
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

/*AFE_MEMIF_HD_CON0 (0x123c) */
#define DLMCH_DL6_SEL      (0x1<<23)
#define DLMCH_DL5_SEL      (0x1<<22)
#define DLMCH_DL4_SEL      (0x1<<21)
#define DLMCH_DL3_SEL      (0x1<<20)
#define DLMCH_DL2_SEL      (0x1<<19)
#define DLMCH_DL1_SEL      (0x1<<18)

/* I2S in/out register bit control */
#define ASYS_I2S_CON_FS			(0x1f << 8)
#define ASYS_I2S_CON_FS_SET(x)		((x) << 8)
#define ASYS_I2S_CON_RESET		(0x1 << 30)
#define ASYS_I2S_CON_I2S_EN	    (0x1 << 0)
#define ASYS_I2S_CON_I2S_COUPLE_MODE	(0x1 << 17)
#define ASYS_I2S_CON_ONE_HEART_MODE    (0x1<<16)
#define ASYS_I2S_CON_SLAVE_MODE    (0x1<<2)
#define ASYS_I2S_CON_SLAVE_MODE_SET(x)    ((x)<<2)

/* 0:EIAJ 1:I2S */
#define ASYS_I2S_CON_I2S_MODE		(0x1 << 3)
#define ASYS_I2S_CON_WIDE_MODE		(0x1 << 1)
#define ASYS_I2S_CON_WIDE_MODE_SET(x)	((x) << 1)
#define ASYS_I2S_IN_PHASE_FIX		(0x1 << 31)

/* TDM in/out register bit control */
#define AFE_TDM_CON_LRCK_WIDTH              (0x1ff << 23)
#define AFE_TDM_CON_LRCK_WIDTH_SET(x)       ((x) << 23)
#define AFE_TDM_OUT_LEFT_ALIGN              (0x1 << 4)
#define AFE_TDM_CON_LEFT_ALIGN_SET(x)       ((x) << 4)
#define AFE_TDM_CON_CH              (0x7 << 12)
#define AFE_TDM_CON_CH_SET(x)       ((x) << 12)
#define AFE_TDM_CON_WLEN           (0x3 << 8)
#define AFE_TDM_CON_WLEN_SET(x)    ((x) << 8)
#define AFE_TDM_CON_DELAY             (0x1 << 3)
#define AFE_TDM_CON_DELAY_SET(x)    ((x) << 3)
#define AFE_TDM_OUT_LRCK_INVERSE     (0x1 << 2)
#define AFE_TDM_OUT_LRCK_INVERSE_SET(x)    ((x) << 2)
#define AFE_TDM_OUT_BCK_INVERSE      (0x1 << 1)
#define AFE_TDM_OUT_BCK_INVERSE_SET(x)    ((x) << 1)


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

#define AFE_END_ADDR 0x15e0
#endif
