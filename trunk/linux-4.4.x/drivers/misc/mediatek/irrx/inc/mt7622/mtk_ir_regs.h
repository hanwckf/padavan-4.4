/**
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
**/
#ifndef __MTK_IR_REGS_H__
#define __MTK_IR_REGS_H__
#include <linux/types.h>
#include <asm/memory.h>

/* #define IR_IO_VIRT_TO_PHYS(v) (0x10000000 | ((v) & 0x0fffffff)) */
/* #define IR_IO_PHYS_TO_VIRT(p) (0xf0000000 | ((p) & 0x0fffffff)) */

#define IRRX_CLK_FREQUENCE    (32*1000)	/* 32KHZ */

/**************************************************
****************IRRX register define********************
**************************************************/
#define IRRX_COUNT_HIGH_REG			0x0000
#define IRRX_CH_BITCNT_MASK         0x0000003f
#define IRRX_CH_BITCNT_BITSFT       0

#define IRRX_COUNT_MID_REG         0x0004
#define IRRX_COUNT_LOW_REG         0x0008

#define IRRX_CONFIG_HIGH_REG     0x000c

#define IRRX_CH_IGB0         ((u32)(1 << 14))
#define IRRX_CH_CHKEN        ((u32)(1 << 13))	/* enable puse width */
#define IRRX_CH_DISCH        ((u32)(1 << 7))
#define IRRX_CH_DISCL        ((u32)(1 << 6))
#define IRRX_CH_ORDINV       ((u32)(1 << 4))
#define IRRX_CH_IRI          ((u32)(1 << 1))
#define IRRX_CH_HWIR         ((u32)(1 << 0))

#define IRRX_CH_END_7        ((u32)(0x07 << 16))
#define IRRX_CH_END_15       ((u32)(0x0f << 16))	/* [22:16] */
#define IRRX_CH_END_23		 ((u32)(0x17 << 16))
#define IRRX_CH_END_31		 ((u32)(0x1f << 16))
#define IRRX_CH_END_39		 ((u32)(0x27 << 16))
#define IRRX_CH_END_47		 ((u32)(0x2f << 16))
#define IRRX_CH_END_55		 ((u32)(0x07 << 16))
#define IRRX_CH_END_63		 ((u32)(0x0f << 16))

/* //////////////////////////// */
#define IRRX_CONFIG_LOW_REG       0x0010	/* IRCFGL */
#define IRRX_SAPERIOD_MASK        ((u32)0xffffff<<0)	/* [23:0]   sampling period */
#define IRRX_SAPERIOD_OFFSET      ((u32)0)

#define IRRX_THRESHOLD_REG        0x0014
#define IRRX_ICLR_MASK            ((u32)1<<7)	/* interrupt clear reset ir */
#define IRRX_ICLR_OFFSET          ((u32)7)

#define IRRX_DGDEL_MASK           ((u32)3<<8)	/* de-glitch select */
#define IRRX_DGDEL_MASK_MT8167    ((u32)0x1f<<8) /* MT8167 */
#define IRRX_DGDEL_OFFSET         ((u32)8)

#define IRRX_IRCLR                0x0018
#define IRRX_IRCLR_MASK           ((u32)0x1<<0)
#define IRRX_IRCLR_OFFSET         ((u32)0)

#define IRRX_IRINT_CLR            0x0020
#define IRRX_INTCLR_MASK          ((u32)0x1<<0)
#define IRRX_INTCLR_OFFSET        ((u32)0)

#define IRRX_IRCFGLL              0x0024
#define IRRX_CHK_PERIOD_MASK      ((u32)0xffffff<<0)	/* [23:0]   ir pulse width sample period */
#define IRRX_CHK_PERIOD_OFFSET    ((u32)0)

#define IRRX_CHKDATA0               0x0030
#define IRRX_CHKDATA1               0x0034
#define IRRX_CHKDATA2               0x0038
#define IRRX_CHKDATA3               0x003C
#define IRRX_CHKDATA4               0x0040
#define IRRX_CHKDATA5               0x0044
#define IRRX_CHKDATA6               0x0048
#define IRRX_CHKDATA7               0x004C
#define IRRX_CHKDATA8               0x0050
#define IRRX_CHKDATA9               0x0054
#define IRRX_CHKDATA10              0x0058
#define IRRX_CHKDATA11              0x005C
#define IRRX_CHKDATA12              0x0060
#define IRRX_CHKDATA13              0x0064
#define IRRX_CHKDATA14              0x0068
#define IRRX_CHKDATA15              0x006C
#define IRRX_CHKDATA16              0x0070
#endif				/* __IRRX_VRF_HW_H__ */
