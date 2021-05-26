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

#ifndef _MT8521P_DAI_PRIVATE_H_
#define _MT8521P_DAI_PRIVATE_H_

#include "mt8521p-afe.h"

extern struct snd_card *snd_card_test;

struct audio_i2s {
	int dai_id;
	int mclk_rate;
	int div_mclk_to_bck;
	int div_bck_to_lrck;
	int format; /*
		* SND_SOC_DAIFMT_FORMAT_MASK
		* | SND_SOC_DAIFMT_CLOCK_MASK
		* | SND_SOC_DAIFMT_INV_MASK
		* | SND_SOC_DAIFMT_MASTER_MASK
		* | SLAVE_USE_ASRC_MASK
		*/
	enum afe_sampling_rate stream_fs;
	snd_pcm_format_t stream_fmt;
	int on;
	int occupied;
	int channels;
};

struct mt_i2s_all {
	struct audio_i2s i2s_out[AFE_I2S_OUT_NUM];
	struct audio_i2s i2s_out_mch;
	struct audio_i2s i2s_in_mchx;
	struct audio_i2s i2s_in[AFE_I2S_IN_NUM];
	struct audio_i2s i2s_in1_ck1ck2;
	struct audio_i2s i2s_in2_ck1ck2;
	int inited;
};

struct mt_btpcm {
	enum afe_pcm_mode fs;
	int format;		/* SND_SOC_DAIFMT_FORMAT_MASK */
	int clkdiv;
	int occupied[2];
};

struct mt_daibt {
	int occupied[2];
};

struct audio_dmic {
	int dai_id;
	enum afe_sampling_rate stream_fs;
	int occupied;
};

struct mt_dmic_all {
	struct audio_dmic dmic_in[AFE_DMIC_NUM];
	int inited;
};

struct mt_i2smrg {
	int occupied[2];
};
struct mt_dai_private {
	struct mt_i2s_all i2s_all;
	struct mt_daibt daibt;
	struct mt_i2smrg i2smrg;
	struct mt_btpcm btpcm;
	struct mt_dmic_all dmic_all;
};

#endif
