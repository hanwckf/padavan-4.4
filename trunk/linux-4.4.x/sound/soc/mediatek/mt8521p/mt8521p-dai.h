/* Copyright (c) 2011-2013, The Linux Foundation. All rights reserved.
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


#ifndef _MT8521P_DAI_H_
#define _MT8521P_DAI_H_

#define MT_DAI_NONE              0
#define MT_DAI_I2S1_ID           1
#define MT_DAI_I2S2_ID           2
#define MT_DAI_I2S3_ID           3
#define MT_DAI_I2S4_ID           4
#define MT_DAI_I2S5_ID           5
#define MT_DAI_I2S6_ID           6
#define MT_DAI_I2SM_ID           7
#define MT_DAI_I2S1_CK1CK2_ID    8  /* i2s1 data + i2s1 ck + i2s2 ck */
#define MT_DAI_I2S2_CK1CK2_ID    9  /* i2s2 data + i2s1 ck + i2s2 ck */
#define MT_DAI_SPDIF_OUT_ID     10
#define MT_DAI_MULTI_IN_ID      11
#define MT_DAI_HDMI_OUT_I2S_ID  12
#define MT_DAI_HDMI_OUT_IEC_ID  13
#define MT_DAI_BTPCM_ID         14
#define MT_DAI_DMIC1_ID         15
#define MT_DAI_DMIC2_ID         16
#define MT_DAI_MRGIF_I2S_ID     17
#define MT_DAI_MRGIF_BT_ID      18

/* X means software reorder for pair interleave and full interleave*/
#define MT_DAI_I2SMX_ID         19

#define MT_DAI_NUM              20


/*
 * Please define I2SM_NUM to one of
 * 1 (2ch MT_DAI_I2S1_ID),
 * 2 (4ch MT_DAI_I2S1+2_ID),
 * 3 (6ch MT_DAI_I2S1+2+3_ID),
 * 4 (8ch MT_DAI_I2S1+2+3+4_ID),
 * and 5 (10ch MT_DAI_I2S1+2+3+4+5_ID).
 */
#define I2SM_NUM  5
/*
 * Please define I2SM_AUXILIARY_STEREO_ID to one of
 * [MT_DAI_I2S1_ID+I2SM_NUM ... MT_DAI_I2S6_ID].
 */
#define I2SM_AUXILIARY_STEREO_ID  MT_DAI_I2S6_ID

#if (I2SM_AUXILIARY_STEREO_ID > MT_DAI_I2S6_ID) \
|| (MT_DAI_I2S1_ID + I2SM_NUM > I2SM_AUXILIARY_STEREO_ID)
#error Invalid I2SM_AUXILIARY_STEREO_ID. Please fix it.
#endif

#define DIV_ID_MCLK_TO_BCK  (0)
#define DIV_ID_BCK_TO_LRCK  (1)

#define SLAVE_USE_ASRC_MASK  (1U<<31)
#define SLAVE_USE_ASRC_YES   (1U<<31)
#define SLAVE_USE_ASRC_NO    (0U<<31)

#endif
