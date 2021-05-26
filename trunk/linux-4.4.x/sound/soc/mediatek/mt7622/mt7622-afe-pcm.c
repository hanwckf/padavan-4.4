/*
 * Mediatek ALSA SoC AFE platform driver for 7622
 *
 * Copyright (c) 2017 MediaTek Inc.
 * Author: Garlic Tseng <garlic.tseng@mediatek.com>
 *             Ir Lian <ir.lian@mediatek.com>
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

#include <linux/delay.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/pm_runtime.h>
#include <sound/soc.h>
#include <sound/pcm_params.h>

#include "mt7622-afe-common.h"

#include "mt7622-afe-clock-ctrl.h"
#include "../common/mtk-afe-platform-driver.h"
#include "../common/mtk-afe-fe-dai.h"

#define AFE_IRQ_STATUS_BITS	0xff

static const struct snd_pcm_hardware mt7622_afe_hardware = {
	.info = SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_INTERLEAVED
	| SNDRV_PCM_INFO_RESUME | SNDRV_PCM_INFO_MMAP_VALID,
	.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE
	| SNDRV_PCM_FMTBIT_S32_LE,
	.period_bytes_min = 16,
	.period_bytes_max = 1024 * 256,
	.periods_min = 2,
	.periods_max = 1024,
	.buffer_bytes_max = 1 * 1024 * 1024,
	.fifo_size = 0,
};

struct mt7622_afe_rate {
	unsigned int rate;
	unsigned int regvalue;
};

static const struct mt7622_afe_rate mt7622_afe_i2s_rates[] = {
	{ .rate = 8000, .regvalue = 0 },
	{ .rate = 12000, .regvalue = 1 },
	{ .rate = 16000, .regvalue = 2 },
	{ .rate = 24000, .regvalue = 3 },
	{ .rate = 32000, .regvalue = 4 },
	{ .rate = 48000, .regvalue = 5 },
	{ .rate = 96000, .regvalue = 6 },
	{ .rate = 192000, .regvalue = 7 },
	{ .rate = 384000, .regvalue = 8 },
	{ .rate = 7350, .regvalue = 16 },
	{ .rate = 11025, .regvalue = 17 },
	{ .rate = 14700, .regvalue = 18 },
	{ .rate = 22050, .regvalue = 19 },
	{ .rate = 29400, .regvalue = 20 },
	{ .rate = 44100, .regvalue = 21 },
	{ .rate = 88200, .regvalue = 22 },
	{ .rate = 176400, .regvalue = 23 },
	{ .rate = 352800, .regvalue = 24 },
};

static int mt7622_dai_num_to_i2s(struct mtk_base_afe *afe, int num)
{
	int val = num - MT7622_IO_I2S;

	if (val < 0 || val >= MT7622_I2S_NUM) {
		dev_err(afe->dev, "%s, num not available, num %d, val %d\n",
			__func__, num, val);
		return -EINVAL;
	}
	return val;
}

static int mt7622_afe_i2s_config(unsigned int *mask, unsigned int *val, int fs, int dir, int stream)
{
	if (dir) {
		/* capture -->config i2s in */
		struct afe_i2s_in_config configI2sIn;

		memset((void *)&configI2sIn, 0, sizeof(configI2sIn));
		configI2sIn.fpga_test_loop3 = 0;
		configI2sIn.fpga_test_loop = 0;
		configI2sIn.fpga_test_loop2 = 0;
		configI2sIn.use_asrc = 0;
		configI2sIn.dsd_mode = 0;
		configI2sIn.sampleRate = fs;
		configI2sIn.ws_invert = 0;
		configI2sIn.fmt = 1;
		configI2sIn.slave = 0;
		configI2sIn.wlen = 1;

		*val = (configI2sIn.fpga_test_loop3 << 23)
		       | (configI2sIn.fpga_test_loop << 21)
		       | (configI2sIn.fpga_test_loop2 << 20)
		       | (configI2sIn.use_asrc << 19)
		       | (configI2sIn.dsd_mode << 18)
		       | (configI2sIn.sampleRate << 8)
		       | (configI2sIn.ws_invert << 5)
		       | (configI2sIn.fmt << 3)
		       | (configI2sIn.slave << 2)
		       | (configI2sIn.wlen << 1);

		*mask = (0x1 << 26)	/* i2s-in slave LJ,RJ sample shift */
			| (0x1 << 23)	/* fpga_test_loop3 */
			| (0x1 << 21)	/* fpga_test_loop */
			| (0x1 << 20)	/* fpga_test_loop2 */
			| (0x1 << 19)	/* use_asrc */
			| (0x1 << 18)	/* dsdMode */
			| (0x1 << 13)	/* RJ 16bit/24bit */
			| (0x1F << 8)	/* mode */
			| (0x1 << 2)	/* slave */
			| (0x1 << 1)	/* cycle */
			| (0x1 << 3) | (0x1 << 14)/* fmt */
			| (0x1 << 5);		/* LR Invert */


	} else {
		/* playback -->config i2s out */
		struct afe_i2s_out_config configI2sOut;

		memset((void *)&configI2sOut, 0, sizeof(configI2sOut));
		configI2sOut.fpga_test_loop = 0;
		configI2sOut.data_from_sine = 0;
		configI2sOut.dsd_mode = 0;
		configI2sOut.couple_mode = 0;
		if (stream)
			configI2sOut.one_heart_mode = 0;
		else
			configI2sOut.one_heart_mode = 1;
		configI2sOut.sampleRate = fs;
		configI2sOut.ws_invert = 0;
		configI2sOut.fmt = 1;
		configI2sOut.slave = 0;
		configI2sOut.wlen = 1;

		*val = (configI2sOut.fpga_test_loop << 21)
		       | (configI2sOut.data_from_sine << 25)
		       | (configI2sOut.dsd_mode << 18)
		       | (configI2sOut.couple_mode << 17)
		       | (configI2sOut.one_heart_mode << 16)
		       | (configI2sOut.sampleRate << 8)
		       | (configI2sOut.ws_invert << 5)
		       | (configI2sOut.fmt << 3)
		       | (configI2sOut.slave << 2)
		       | (configI2sOut.wlen << 1);

		*mask = (0x1 << 21)	/* fpga_test_loop */
			| (0x1 << 25)	/* data_from_sine */
			| (0x1 << 18)	/* dsd_mode */
			| (0x1 << 17)	/* couple_mode */
			| (0x1 << 16)	/* one_heart_mode */
			| (0x1F << 8)	/* mode */
			| (0x1 << 5)	/* LR Invert */
			| (0x1 << 2)	/* slave */
			| (0x1 << 13)	/* RJ 16bit/24bit */
			| (0x1 << 1)	/* cycle */
			| (0x1 << 3) | (0x1 << 14);	/* fmt */
	}

	return -EINVAL;
}


static int mt7622_afe_i2s_fs(unsigned int sample_rate)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(mt7622_afe_i2s_rates); i++)
		if (mt7622_afe_i2s_rates[i].rate == sample_rate)
			return mt7622_afe_i2s_rates[i].regvalue;

	return -EINVAL;
}

static int mt7622_afe_i2s_startup(struct snd_pcm_substream *substream,
				  struct snd_soc_dai *dai)
{
#ifndef EARLY_PORTING
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct mt7622_afe_private *afe_priv = afe->platform_priv;
	int i2s_num = mt7622_dai_num_to_i2s(afe, dai->id);
	int clk_num = MT7622_AUD_I2S1_MCK_DIV_PD;
	int ret = 0;

	if (i2s_num < 0)
		return i2s_num;

	/* enable mclk */
	ret = clk_prepare_enable(afe_priv->clocks[clk_num]);
	if (ret)
		dev_err(dai->dev, "Failed to enable mclk for I2S: %d\n", i2s_num);

	return ret;
#else
	return 0;
#endif

}

static int mt7622_afe_i2s_path_shutdown(struct snd_pcm_substream *substream,
					struct snd_soc_dai *dai,
					int dir_invert)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct mt7622_afe_private *afe_priv = afe->platform_priv;
	int i2s_num = mt7622_dai_num_to_i2s(afe, dai->id);
	struct mt7622_i2s_path *i2s_path;
	const struct mt7622_i2s_data *i2s_data;
	int stream_dir = substream->stream;

	if (i2s_num < 0)
		return i2s_num;

	i2s_path = &afe_priv->i2s_path[i2s_num];

	if (dir_invert)	{
		if (stream_dir == SNDRV_PCM_STREAM_PLAYBACK)
			stream_dir = SNDRV_PCM_STREAM_CAPTURE;
		else
			stream_dir = SNDRV_PCM_STREAM_PLAYBACK;
	}
	i2s_data = i2s_path->i2s_data[stream_dir];

	i2s_path->on[stream_dir]--;
	if (i2s_path->on[stream_dir] < 0) {
		dev_warn(afe->dev, "i2s_path->on: %d, dir: %d\n",
			 i2s_path->on[stream_dir], stream_dir);
		i2s_path->on[stream_dir] = 0;
	}
	if (i2s_path->on[stream_dir])
		return 0;

	/* disable i2s */
	regmap_update_bits(afe->regmap, i2s_data->i2s_ctrl_reg,
			   ASYS_I2S_CON_I2S_EN, 0);
	regmap_update_bits(afe->regmap, AUDIO_TOP_CON4,
			   1 << i2s_data->i2s_pwn_shift,
			   1 << i2s_data->i2s_pwn_shift);
	return 0;
}

static void mt7622_afe_i2s_shutdown(struct snd_pcm_substream *substream,
				    struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct mt7622_afe_private *afe_priv = afe->platform_priv;
	int i2s_num = mt7622_dai_num_to_i2s(afe, dai->id);
	struct mt7622_i2s_path *i2s_path;
	int clk_num = MT7622_AUD_I2S1_MCK_DIV_PD;

	if (i2s_num < 0)
		return;

	i2s_path = &afe_priv->i2s_path[i2s_num];

	if (i2s_path->occupied[substream->stream])
		i2s_path->occupied[substream->stream] = 0;
	else
		goto I2S_UNSTART;

	mt7622_afe_i2s_path_shutdown(substream, dai, 0);

	if (of_find_property(afe->dev->of_node, "6-pin-i2s", NULL) == NULL) {
		/* need to disable i2s-out path when disable i2s-in */
		if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
			mt7622_afe_i2s_path_shutdown(substream, dai, 1);
	}

I2S_UNSTART:
	/* disable mclk */
	clk_disable_unprepare(afe_priv->clocks[clk_num]);
}

static int mt7622_i2s_path_prepare_enable(struct snd_pcm_substream *substream,
		struct snd_soc_dai *dai,
		int dir_invert)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct mt7622_afe_private *afe_priv = afe->platform_priv;
	int i2s_num = mt7622_dai_num_to_i2s(afe, dai->id);
	struct mt7622_i2s_path *i2s_path;
	const struct mt7622_i2s_data *i2s_data;
	struct snd_pcm_runtime * const runtime = substream->runtime;
	int reg, fs; /* now we support bck 64bits only */
	int stream_dir = substream->stream;
	unsigned int mask = 0, val = 0;

	if (i2s_num < 0)
		return i2s_num;

	i2s_path = &afe_priv->i2s_path[i2s_num];

	if (dir_invert) {
		if (stream_dir == SNDRV_PCM_STREAM_PLAYBACK)
			stream_dir = SNDRV_PCM_STREAM_CAPTURE;
		else
			stream_dir = SNDRV_PCM_STREAM_PLAYBACK;
	}
	i2s_data = i2s_path->i2s_data[stream_dir];

	/* no need to enable if already done */
	i2s_path->on[stream_dir]++;

	if (i2s_path->on[stream_dir] != 1)
		return 0;

	fs = mt7622_afe_i2s_fs(runtime->rate);
	mt7622_afe_i2s_config(&mask, &val, fs, dir_invert, substream->stream);

	if (stream_dir == SNDRV_PCM_STREAM_CAPTURE) {
		mask |= ASYS_I2S_IN_PHASE_FIX;
		val |= ASYS_I2S_IN_PHASE_FIX;
	}

	regmap_update_bits(afe->regmap, i2s_data->i2s_ctrl_reg, mask, val);

	if (stream_dir == SNDRV_PCM_STREAM_PLAYBACK)
		reg = ASMO_TIMING_CON1;
	else
		reg = ASMI_TIMING_CON1;

	regmap_update_bits(afe->regmap, reg,
			   i2s_data->i2s_asrc_fs_mask
			   << i2s_data->i2s_asrc_fs_shift,
			   fs << i2s_data->i2s_asrc_fs_shift);

	/* enable i2s */
	regmap_update_bits(afe->regmap, AUDIO_TOP_CON4,
			   1 << i2s_data->i2s_pwn_shift,
			   0 << i2s_data->i2s_pwn_shift);

	/* reset i2s hw status before enable */
	regmap_update_bits(afe->regmap, i2s_data->i2s_ctrl_reg,
			   ASYS_I2S_CON_RESET, ASYS_I2S_CON_RESET);
	udelay(1);
	regmap_update_bits(afe->regmap, i2s_data->i2s_ctrl_reg,
			   ASYS_I2S_CON_RESET, 0);
	udelay(1);
	regmap_update_bits(afe->regmap, i2s_data->i2s_ctrl_reg,
			   ASYS_I2S_CON_I2S_EN, ASYS_I2S_CON_I2S_EN);

	return 0;
}

static int mt7622_afe_i2s_prepare(struct snd_pcm_substream *substream,
				  struct snd_soc_dai *dai)
{
	int clk_domain;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct mt7622_afe_private *afe_priv = afe->platform_priv;
	int i2s_num = mt7622_dai_num_to_i2s(afe, dai->id);
	struct mt7622_i2s_path *i2s_path;
	int mclk_rate;

	if (i2s_num < 0)
		return i2s_num;

	i2s_path = &afe_priv->i2s_path[i2s_num];
	mclk_rate = i2s_path->mclk_rate;

	if (i2s_path->occupied[substream->stream])
		return -EBUSY;
	i2s_path->occupied[substream->stream] = 1;

	if (MT7622_PLL_DOMAIN_0_RATE % mclk_rate == 0)
		clk_domain = 0;
	else if (MT7622_PLL_DOMAIN_1_RATE % mclk_rate == 0)
		clk_domain = 1;
	else {
		dev_err(dai->dev, "%s() bad mclk rate %d\n",
			__func__, mclk_rate);
		return -EINVAL;
	}
#ifndef EARLY_PORTING
	mt7622_i2s_mclk_configuration(afe, i2s_num, clk_domain, mclk_rate);
#endif
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		mt7622_i2s_path_prepare_enable(substream, dai, 0);
	else {
		/* need to enable i2s-out path when enable i2s-in */
		/* prepare for another direction "out" */
		mt7622_i2s_path_prepare_enable(substream, dai, 1);
		/* prepare for "in" */
		mt7622_i2s_path_prepare_enable(substream, dai, 0);
	}

	return 0;
}

#ifdef EARLY_PORTING
static int mt7622_afe_i2s_hw_params(struct snd_pcm_substream *substream,
				    struct snd_pcm_hw_params *params,
				    struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	int stream_fs = params_rate(params);

	regmap_update_bits(afe->regmap, FPGA_CFG0, 0 << XCLK_DIVIDER_POS, XCLK_DIVIDER_MASK);

	if (stream_fs == 192000 || stream_fs == 176400)
		regmap_update_bits(afe->regmap, FPGA_CFG0, 1 << XCLK_DIVIDER_POS, XCLK_DIVIDER_MASK);
	else if (stream_fs == 384000 || stream_fs == 352800)
		regmap_update_bits(afe->regmap, FPGA_CFG0, 3 << XCLK_DIVIDER_POS, XCLK_DIVIDER_MASK);

	if (mt7622_afe_i2s_fs(stream_fs) < 9)
		regmap_update_bits(afe->regmap, FPGA_CFG0, DAC_2CH_MCLK_CLK_DOMAIN_48, DAC_2CH_MCLK_CLK_DOMAIN_MASK);
	else
		regmap_update_bits(afe->regmap, FPGA_CFG0, DAC_2CH_MCLK_CLK_DOMAIN_44, DAC_2CH_MCLK_CLK_DOMAIN_MASK);

	return 0;
}
#endif

static int mt7622_afe_i2s_set_sysclk(struct snd_soc_dai *dai, int clk_id,
				     unsigned int freq, int dir)
{
	struct mtk_base_afe *afe = dev_get_drvdata(dai->dev);
	struct mt7622_afe_private *afe_priv = afe->platform_priv;
	int i2s_num = mt7622_dai_num_to_i2s(afe, dai->id);

	if (i2s_num < 0)
		return i2s_num;

	/* mclk */
	if (dir == SND_SOC_CLOCK_IN) {
		dev_warn(dai->dev, "%s() warning: mt7622 doesn't support mclk input\n", __func__);
		return -EINVAL;
	}
	afe_priv->i2s_path[i2s_num].mclk_rate = freq;
	return 0;
}

static int mt7622_simple_fe_startup(struct snd_pcm_substream *substream,
				    struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	int stream_dir = substream->stream;
	int memif_num = rtd->cpu_dai->id;
	struct mtk_base_afe_memif *memif_tmp;

	/* can't run single DL & DLM at the same time */
	if (stream_dir == SNDRV_PCM_STREAM_PLAYBACK) {
		memif_tmp = &afe->memif[MT7622_MEMIF_DLM];
		if (memif_tmp->substream) {
			dev_warn(afe->dev, "%s memif is not available, stream_dir %d, memif_num %d\n",
				 __func__, stream_dir, memif_num);
			return -EBUSY;
		}
	}
	return mtk_afe_fe_startup(substream, dai);
}

static int mt7622_simple_fe_hw_params(struct snd_pcm_substream *substream,
				      struct snd_pcm_hw_params *params,
				      struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	int stream_dir = substream->stream;

	/* single DL use PAIR_INTERLEAVE */
	if (stream_dir == SNDRV_PCM_STREAM_PLAYBACK) {
		regmap_update_bits(afe->regmap,
				   AFE_MEMIF_PBUF_SIZE,
				   AFE_MEMIF_PBUF_SIZE_DLM_MASK,
				   AFE_MEMIF_PBUF_SIZE_PAIR_INTERLEAVE);
	}
	return mtk_afe_fe_hw_params(substream, params, dai);
}


static int mt7622_dlm_fe_startup(struct snd_pcm_substream *substream,
				 struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct mtk_base_afe_memif *memif_tmp;
	const struct mtk_base_memif_data *memif_data;
	int i;

	for (i = MT7622_MEMIF_DL1; i < MT7622_MEMIF_DL_SINGLE_NUM; ++i) {
		memif_tmp = &afe->memif[i];
		if (memif_tmp->substream)
			return -EBUSY;
	}

	/* enable agent for all signal DL (due to hw design) */
	for (i = MT7622_MEMIF_DL1; i < MT7622_MEMIF_DL_SINGLE_NUM; ++i) {
		memif_data = afe->memif[i].data;
		regmap_update_bits(afe->regmap,
				   memif_data->agent_disable_reg,
				   1 << memif_data->agent_disable_shift,
				   0 << memif_data->agent_disable_shift);
	}

	return mtk_afe_fe_startup(substream, dai);
}

static void mt7622_dlm_fe_shutdown(struct snd_pcm_substream *substream,
				   struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	const struct mtk_base_memif_data *memif_data;
	int i;

	for (i = MT7622_MEMIF_DL1; i < MT7622_MEMIF_DL_SINGLE_NUM; ++i) {
		memif_data = afe->memif[i].data;
		regmap_update_bits(afe->regmap,
				   memif_data->agent_disable_reg,
				   1 << memif_data->agent_disable_shift,
				   1 << memif_data->agent_disable_shift);
	}
	return mtk_afe_fe_shutdown(substream, dai);
}

static int mt7622_dlm_fe_hw_params(struct snd_pcm_substream *substream,
				   struct snd_pcm_hw_params *params,
				   struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	int channels = params_channels(params);

	switch (channels) {
	case DLMCH_11CH:
	case DLMCH_12CH:
		regmap_update_bits(afe->regmap, AFE_MEMIF_HD_CON0, DLMCH_DL6_SEL, DLMCH_DL6_SEL);
	/* fallthrough */
	case DLMCH_9CH:
	case DLMCH_10CH:
		regmap_update_bits(afe->regmap, AFE_MEMIF_HD_CON0, DLMCH_DL5_SEL, DLMCH_DL5_SEL);
	/* fallthrough */
	case DLMCH_7CH:
	case DLMCH_8CH:
		regmap_update_bits(afe->regmap, AFE_MEMIF_HD_CON0, DLMCH_DL4_SEL, DLMCH_DL4_SEL);
	/* fallthrough */
	case DLMCH_5CH:
	case DLMCH_6CH:
		regmap_update_bits(afe->regmap, AFE_MEMIF_HD_CON0, DLMCH_DL3_SEL, DLMCH_DL3_SEL);
	/* fallthrough */
	case DLMCH_3CH:
	case DLMCH_4CH:
		regmap_update_bits(afe->regmap, AFE_MEMIF_HD_CON0, DLMCH_DL2_SEL, DLMCH_DL2_SEL);
	/* fallthrough */
	case DLMCH_1CH:
	case DLMCH_2CH:
		regmap_update_bits(afe->regmap, AFE_MEMIF_HD_CON0, DLMCH_DL1_SEL, DLMCH_DL1_SEL);
		break;
	default:
		break;
	}

	regmap_update_bits(afe->regmap,
			   AFE_MEMIF_PBUF_SIZE,
			   AFE_MEMIF_PBUF_SIZE_DLM_BYTE_MASK,
			   AFE_MEMIF_PBUF_SIZE_DLM_32BYTES);
	regmap_update_bits(afe->regmap,
			   AFE_MEMIF_PBUF_SIZE,
			   AFE_MEMIF_PBUF_SIZE_DLM_CH_MASK,
			   AFE_MEMIF_PBUF_SIZE_DLM_CH(channels));

	return mtk_afe_fe_hw_params(substream, params, dai);
}

static int mt7622_dlm_fe_trigger(struct snd_pcm_substream *substream,
				 int cmd, struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	/* if you want to use DLMCH, DL1 should be enabled too */
	struct mtk_base_afe_memif *memif_tmp = &afe->memif[MT7622_MEMIF_DL1];

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
		regmap_update_bits(afe->regmap, memif_tmp->data->enable_reg,
				   1 << memif_tmp->data->enable_shift,
				   1 << memif_tmp->data->enable_shift);
		mtk_afe_fe_trigger(substream, cmd, dai);
		return 0;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
		mtk_afe_fe_trigger(substream, cmd, dai);
		regmap_update_bits(afe->regmap, memif_tmp->data->enable_reg,
				   1 << memif_tmp->data->enable_shift, 0);

		return 0;
	default:
		return -EINVAL;
	}
}

static int mt7622_dai_num_to_tdm(struct mtk_base_afe *afe, int num)
{
	int val = num - MT7622_MEMIF_DLTDM;

	if (val < 0 || val >= MT7622_TDM_NUM) {
		dev_err(afe->dev, "%s, num not available, num %d, val %d\n",
			__func__, num, val);
		return -EINVAL;
	}
	return val;
}

int mt7622_tdm_map_channel(int channel)
{
	if (channel > 16)
		return 0;
	else if (channel > 12)
		return TDM_16CH;
	else if (channel > 8)
		return TDM_12CH;
	else if (channel > 4)
		return TDM_8CH;
	else if (channel > 2)
		return TDM_4CH;
	else
		return TDM_2CH;
}

int afe_tdm_in_mask_configurate(int channel)
{
	u32 odd_mask = 0x0; /* ch0,1: 7th bit; ch14,15: 1st bit */
	u32 disable_out = 0x0; /* ch0,1: 7th bit; ch14,15: 1st bit */
	u32 reg_val_to_write = 0x0;
	int i;
	int ch_pair_num = channel / 2;
	int ch_odd = channel & 1;

	for (i = 0; i < 8 - ch_pair_num; i++)
		disable_out |= 1 << i;

	if (ch_odd)
		odd_mask = 1 << (8 - ch_pair_num);

	reg_val_to_write |= AFE_TDM_IN_DISABLE_OUT_SET(disable_out) & AFE_TDM_IN_DISABLE_OUT;
	reg_val_to_write |= AFE_TDM_IN_MASK_ODD_SET(odd_mask) & AFE_TDM_IN_MASK_ODD;

	return reg_val_to_write;
}

void mt7622_tdm_format_configuration(struct mtk_base_afe *afe, int tdm_num,
				     struct snd_pcm_hw_params *params)
{
	struct mt7622_afe_private *afe_priv = afe->platform_priv;
	struct mt7622_tdm_data *tdm_data = (struct mt7622_tdm_data *) afe_priv->tdm_path[tdm_num].tdm_data;
	int channels = params_channels(params);
	int bit_width = params_width(params);
	int fmt = TDM_I2S;
	u32 value1;

	/* TDM config */
	if (bit_width == 32) {
		regmap_update_bits(afe->regmap, tdm_data->tdm_ctrl_reg, AFE_TDM_CON_WLEN, AFE_TDM_CON_WLEN_SET(3));
		regmap_update_bits(afe->regmap, tdm_data->tdm_ctrl_reg, 3 << tdm_data->tdm_lrck_cycle_shift,
				   2 << tdm_data->tdm_lrck_cycle_shift);
	} else if (bit_width == 16) {
		regmap_update_bits(afe->regmap, tdm_data->tdm_ctrl_reg, AFE_TDM_CON_WLEN, AFE_TDM_CON_WLEN_SET(1));
		regmap_update_bits(afe->regmap, tdm_data->tdm_ctrl_reg, 3 << tdm_data->tdm_lrck_cycle_shift,
				   0 << tdm_data->tdm_lrck_cycle_shift);
	} else if (bit_width == 24) {
		regmap_update_bits(afe->regmap, tdm_data->tdm_ctrl_reg, AFE_TDM_CON_WLEN, AFE_TDM_CON_WLEN_SET(2));
		regmap_update_bits(afe->regmap, tdm_data->tdm_ctrl_reg, 3 << tdm_data->tdm_lrck_cycle_shift,
				   1 << tdm_data->tdm_lrck_cycle_shift);
	}

	regmap_update_bits(afe->regmap, tdm_data->tdm_ctrl_reg, AFE_TDM_CON_CH,
			   AFE_TDM_CON_CH_SET(mt7622_tdm_map_channel(channels)));

	regmap_update_bits(afe->regmap, tdm_data->tdm_ctrl_reg, AFE_TDM_CON_LRCK_WIDTH,
			   AFE_TDM_CON_LRCK_WIDTH_SET(bit_width - 1));

	/* set for I2S mode or ELJ mode */
	if (fmt == TDM_I2S) {
		if (tdm_num == MT7622_TDMI && channels != 2) {
			regmap_update_bits(afe->regmap, tdm_data->tdm_ctrl_reg, AFE_TDM_CON_DELAY,
					   AFE_TDM_CON_DELAY_SET(0));
		} else
			regmap_update_bits(afe->regmap, tdm_data->tdm_ctrl_reg, AFE_TDM_CON_DELAY,
					   AFE_TDM_CON_DELAY_SET(1));
		regmap_update_bits(afe->regmap, tdm_data->tdm_ctrl_reg, AFE_TDM_OUT_LRCK_INVERSE,
				   AFE_TDM_OUT_LRCK_INVERSE_SET(1));
	} else if (fmt == TDM_LJ) {
		regmap_update_bits(afe->regmap, tdm_data->tdm_ctrl_reg, AFE_TDM_CON_DELAY, AFE_TDM_CON_DELAY_SET(0));
		regmap_update_bits(afe->regmap, tdm_data->tdm_ctrl_reg, AFE_TDM_OUT_LRCK_INVERSE,
				   AFE_TDM_OUT_LRCK_INVERSE_SET(0));
	}

	/* tdm_bck_inv: inverse BCK for I2S and LJ fmt*/
	regmap_update_bits(afe->regmap, tdm_data->tdm_ctrl_reg, AFE_TDM_OUT_BCK_INVERSE,
			   AFE_TDM_OUT_BCK_INVERSE_SET(1));

	/*TDM memif configurations */
	if (tdm_num == MT7622_TDMO) {
		if (bit_width != 16)
			regmap_update_bits(afe->regmap, tdm_data->tdm_out_reg,
					   1 << tdm_data->tdm_out_bit_width_shift,
					   1 << tdm_data->tdm_out_bit_width_shift);
		else
			regmap_update_bits(afe->regmap, tdm_data->tdm_out_reg,
					   1 << tdm_data->tdm_out_bit_width_shift,
					   0 << tdm_data->tdm_out_bit_width_shift);

		regmap_update_bits(afe->regmap, tdm_data->tdm_out_reg, 0x1f << tdm_data->tdm_out_ch_num_shift,
				   channels << tdm_data->tdm_out_ch_num_shift);
		/* LEFT_ALIGN */
		regmap_update_bits(afe->regmap, tdm_data->tdm_ctrl_reg, AFE_TDM_OUT_LEFT_ALIGN,
				   AFE_TDM_CON_LEFT_ALIGN_SET(1));
	}

	/*enable TDM*/
	regmap_update_bits(afe->regmap, tdm_data->tdm_ctrl_reg, 1 << tdm_data->tdm_on_shift,
			   1 << tdm_data->tdm_on_shift);

	regmap_read(afe->regmap, tdm_data->tdm_ctrl_reg, &value1);
}

static int mt7622_afe_tdm_hw_params(struct snd_pcm_substream *substream,
				    struct snd_pcm_hw_params *params,
				    struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	int tdm_num = mt7622_dai_num_to_tdm(afe, dai->id);
	struct mtk_base_afe_memif *memif = &afe->memif[rtd->cpu_dai->id];
	int hd_audio = 0;
	unsigned int fs = params_rate(params);

	/*TDM MCLK*/
	mt7622_tdm_mclk_configuration(afe, tdm_num, fs);

	/*TDM BCK*/
	mt7622_tdm_clk_configuration(afe, tdm_num, params);
	mtk_afe_fe_hw_params(substream, params, dai);

	/* set hd mode */
	switch (params->msbits) {
	case 16:
		hd_audio = 0;
		break;
	case 24:
	case 32:
		hd_audio = 1;
		break;
	default:
		dev_err(afe->dev, "%s() error: unsupported format %d\n",
			__func__, params->msbits);
		break;
	}

	regmap_update_bits(afe->regmap, memif->data->hd_reg,
			   1 << memif->data->hd_shift,
			   hd_audio << memif->data->hd_shift);

	mt7622_tdm_format_configuration(afe, tdm_num, params);
	return 0;
}

static int mt7622_afe_tdm_startup(struct snd_pcm_substream *substream,
				  struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct mt7622_afe_private *afe_priv = afe->platform_priv;
	int tdm_num = mt7622_dai_num_to_tdm(afe, dai->id);
	int ret = 0;
	int clk_num, apll1_bck, apll2_bck;

	if (tdm_num < 0)
		return tdm_num;

	if (tdm_num == MT7622_TDMO)
		clk_num = MT7622_AUD_I2S3_MCK_DIV_PD;
	else
		clk_num = MT7622_AUD_I2S2_MCK_DIV_PD;

	apll1_bck = MT7622_AUD_APLL1_DIV_PD;
	apll2_bck = MT7622_AUD_APLL2_DIV_PD;

	/* enable mclk */
	ret = clk_prepare_enable(afe_priv->clocks[clk_num]);
	if (ret) {
		dev_err(dai->dev, "Failed to enable mclk for TDM: %d ret=%d\n", tdm_num, ret);
		return ret;
	}

	/* enable apll1_ck for tdm bck*/
	ret = clk_prepare_enable(afe_priv->clocks[apll1_bck]);
	if (ret) {
		dev_err(dai->dev, "Failed to enable apll1_ck for TDM: %d ret=%d\n", tdm_num, ret);
		return ret;
	}

	/* enable apll2_ck for tdm bck*/
	ret = clk_prepare_enable(afe_priv->clocks[apll2_bck]);
	if (ret) {
		dev_err(dai->dev, "Failed to enable apll2_ck for TDM: %d ret=%d\n", tdm_num, ret);
		return ret;
	}

	return mtk_afe_fe_startup(substream, dai);
}

static void mt7622_afe_tdm_shutdown(struct snd_pcm_substream *substream,
				    struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct mt7622_afe_private *afe_priv = afe->platform_priv;
	int tdm_num = mt7622_dai_num_to_tdm(afe, dai->id);
	struct mt7622_tdm_data *tdm_data = (struct mt7622_tdm_data *) afe_priv->tdm_path[tdm_num].tdm_data;
	int clk_num, apll1_bck, apll2_bck;

	if (tdm_num < 0)
		return;

	if (tdm_num == MT7622_TDMO)
		clk_num = MT7622_AUD_I2S3_MCK_DIV_PD;
	else
		clk_num = MT7622_AUD_I2S2_MCK_DIV_PD;

	apll1_bck = MT7622_AUD_APLL1_DIV_PD;
	apll2_bck = MT7622_AUD_APLL2_DIV_PD;

	/*disable TDM*/
	regmap_update_bits(afe->regmap, tdm_data->tdm_ctrl_reg, 1 << tdm_data->tdm_on_shift,
			   0 << tdm_data->tdm_on_shift);

	/*disabel TDM pd*/
	if (tdm_num == MT7622_TDMO)
		regmap_update_bits(afe->regmap, AFE_TDMOUT_CLKDIV_CFG, (TDMOUT_PDN_M | TDMOUT_PDN_B),
				   (1 << 0 | 1 << 1));
	else
		regmap_update_bits(afe->regmap, AFE_TDMIN_CLKDIV_CFG, (TDMIN_PDN_M | TDMIN_PDN_B),
				   (1 << 0 | 1 << 1));

	/* disable mclk */
	clk_disable_unprepare(afe_priv->clocks[clk_num]);

	/* disable apll1_ck/apll2_ck for tdm bck */
	clk_disable_unprepare(afe_priv->clocks[apll1_bck]);
	clk_disable_unprepare(afe_priv->clocks[apll2_bck]);

	return mtk_afe_fe_shutdown(substream, dai);
}


static int mt7622_memif_fs(struct snd_pcm_substream *substream,
			   unsigned int rate)
{
	return mt7622_afe_i2s_fs(rate);
}

static int mt7622_irq_fs(struct snd_pcm_substream *substream, unsigned int rate)
{
	return mt7622_afe_i2s_fs(rate);
}

/* FE DAIs */
static const struct snd_soc_dai_ops mt7622_single_memif_dai_ops = {
	.startup	= mt7622_simple_fe_startup,
	.shutdown	= mtk_afe_fe_shutdown,
	.hw_params	= mt7622_simple_fe_hw_params,
	.hw_free	= mtk_afe_fe_hw_free,
	.prepare	= mtk_afe_fe_prepare,
	.trigger	= mtk_afe_fe_trigger,

};

static const struct snd_soc_dai_ops mt7622_dlm_memif_dai_ops = {
	.startup	= mt7622_dlm_fe_startup,
	.shutdown	= mt7622_dlm_fe_shutdown,
	.hw_params	= mt7622_dlm_fe_hw_params,
	.hw_free	= mtk_afe_fe_hw_free,
	.prepare	= mtk_afe_fe_prepare,
	.trigger	= mt7622_dlm_fe_trigger,
};


/* I2S BE DAIs */
static const struct snd_soc_dai_ops mt7622_afe_i2s_ops = {
	.startup	= mt7622_afe_i2s_startup,
	.shutdown	= mt7622_afe_i2s_shutdown,
#ifdef EARLY_PORTING
	.hw_params  = mt7622_afe_i2s_hw_params,
#endif
	.prepare	= mt7622_afe_i2s_prepare,
	.set_sysclk	= mt7622_afe_i2s_set_sysclk,
};

/* TDM DAIs */
static const struct snd_soc_dai_ops mt7622_afe_tdm_ops = {
	.startup    = mt7622_afe_tdm_startup,
	.shutdown   = mt7622_afe_tdm_shutdown,
	.hw_params  = mt7622_afe_tdm_hw_params,
	.hw_free    = mtk_afe_fe_hw_free,
	.trigger    = mtk_afe_fe_trigger,
};

static struct snd_soc_dai_driver mt7622_afe_pcm_dais[] = {
	/* FE DAIs: memory intefaces to CPU */
	{
		.name = "PCMO0",
		.id = MT7622_MEMIF_DL1,
		.suspend = mtk_afe_dai_suspend,
		.resume = mtk_afe_dai_resume,
		.playback = {
			.stream_name = "DL1",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000_384000,
			.formats = (SNDRV_PCM_FMTBIT_S16_LE
			| SNDRV_PCM_FMTBIT_S24_LE
			| SNDRV_PCM_FMTBIT_S32_LE)

		},
		.ops = &mt7622_single_memif_dai_ops,
	},
	{
		.name = "PCM0",
		.id = MT7622_MEMIF_UL1,
		.suspend = mtk_afe_dai_suspend,
		.resume = mtk_afe_dai_resume,
		.capture = {
			.stream_name = "UL1",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000_384000,
			.formats = (SNDRV_PCM_FMTBIT_S16_LE
			| SNDRV_PCM_FMTBIT_S24_LE
			| SNDRV_PCM_FMTBIT_S32_LE)
		},
		.ops = &mt7622_single_memif_dai_ops,
	},
	{
		.name = "PCM_multi",
		.id = MT7622_MEMIF_DLM,
		.suspend = mtk_afe_dai_suspend,
		.resume = mtk_afe_dai_resume,
		.playback = {
			.stream_name = "DLM",
			.channels_min = 1,
			.channels_max = 8,
			.rates = SNDRV_PCM_RATE_8000_384000,
			.formats = (SNDRV_PCM_FMTBIT_S16_LE
			| SNDRV_PCM_FMTBIT_S24_LE
			| SNDRV_PCM_FMTBIT_S32_LE)

		},
		.ops = &mt7622_dlm_memif_dai_ops,
	},
	/* BE DAIs */
	{
		.name = "I2S0",
		.id = MT7622_IO_I2S,
		.playback = {
			.stream_name = "I2S0 Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000_384000,
			.formats = (SNDRV_PCM_FMTBIT_S16_LE
			| SNDRV_PCM_FMTBIT_S24_LE
			| SNDRV_PCM_FMTBIT_S32_LE)

		},
		.capture = {
			.stream_name = "I2S0 Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000_384000,
			.formats = (SNDRV_PCM_FMTBIT_S16_LE
			| SNDRV_PCM_FMTBIT_S24_LE
			| SNDRV_PCM_FMTBIT_S32_LE)

		},
		.ops = &mt7622_afe_i2s_ops,
		.symmetric_rates = 1,
	},
	{
		.name = "I2S1",
		.id = MT7622_IO_2ND_I2S,
		.playback = {
			.stream_name = "I2S1 Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000_384000,
			.formats = (SNDRV_PCM_FMTBIT_S16_LE
			| SNDRV_PCM_FMTBIT_S24_LE
			| SNDRV_PCM_FMTBIT_S32_LE)
		},
		.capture = {
			.stream_name = "I2S1 Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000_384000,
			.formats = (SNDRV_PCM_FMTBIT_S16_LE
			| SNDRV_PCM_FMTBIT_S24_LE
			| SNDRV_PCM_FMTBIT_S32_LE)
		},
		.ops = &mt7622_afe_i2s_ops,
		.symmetric_rates = 1,
	},
	{
		.name = "I2S2",
		.id = MT7622_IO_3RD_I2S,
		.playback = {
			.stream_name = "I2S2 Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000_384000,
			.formats = (SNDRV_PCM_FMTBIT_S16_LE
			| SNDRV_PCM_FMTBIT_S24_LE
			| SNDRV_PCM_FMTBIT_S32_LE)
		},
		.capture = {
			.stream_name = "I2S2 Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000_384000,
			.formats = (SNDRV_PCM_FMTBIT_S16_LE
			| SNDRV_PCM_FMTBIT_S24_LE
			| SNDRV_PCM_FMTBIT_S32_LE)
		},
		.ops = &mt7622_afe_i2s_ops,
		.symmetric_rates = 1,
	},
	{
		.name = "I2S3",
		.id = MT7622_IO_4TH_I2S,
		.playback = {
			.stream_name = "I2S3 Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000_384000,
			.formats = (SNDRV_PCM_FMTBIT_S16_LE
			| SNDRV_PCM_FMTBIT_S24_LE
			| SNDRV_PCM_FMTBIT_S32_LE)
		},
		.capture = {
			.stream_name = "I2S3 Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000_384000,
			.formats = (SNDRV_PCM_FMTBIT_S16_LE
			| SNDRV_PCM_FMTBIT_S24_LE
			| SNDRV_PCM_FMTBIT_S32_LE)
		},
		.ops = &mt7622_afe_i2s_ops,
		.symmetric_rates = 1,
	},
	/***
	* TDM DAI : TDM is not follow FE/BE DAI structure,
	* because it is isolate agent which can't
	* connect to other module by interconnection.
	***/
	{
		.name = "TDM_OUT0",
		.id = MT7622_MEMIF_DLTDM,
		.suspend = mtk_afe_dai_suspend,
		.resume = mtk_afe_dai_resume,
		.playback = {
			.stream_name = "TDM0 Playback",
			.channels_min = 1,
			.channels_max = 16,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = (SNDRV_PCM_FMTBIT_S16_LE
			| SNDRV_PCM_FMTBIT_S24_LE
			| SNDRV_PCM_FMTBIT_S32_LE)
		},
		.ops = &mt7622_afe_tdm_ops,
	},
	{
		.name = "TDM_IN",
		.id = MT7622_MEMIF_ULTDM,
		.suspend = mtk_afe_dai_suspend,
		.resume = mtk_afe_dai_resume,
		.capture = {
			.stream_name = "TDM Capture",
			.channels_min = 1,
			.channels_max = 16,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = (SNDRV_PCM_FMTBIT_S16_LE
			| SNDRV_PCM_FMTBIT_S24_LE
			| SNDRV_PCM_FMTBIT_S32_LE)
		},
		.ops = &mt7622_afe_tdm_ops,
	},
};

static const struct snd_kcontrol_new mt7622_afe_o00_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I00 Switch", AFE_CONN0, 0, 1, 0),
};

static const struct snd_kcontrol_new mt7622_afe_o01_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I01 Switch", AFE_CONN1, 1, 1, 0),
};

static const struct snd_kcontrol_new mt7622_afe_o02_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I02 Switch", AFE_CONN2, 2, 1, 0),
};

static const struct snd_kcontrol_new mt7622_afe_o03_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I03 Switch", AFE_CONN3, 3, 1, 0),
};

static const struct snd_kcontrol_new mt7622_afe_o04_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I04 Switch", AFE_CONN4, 4, 1, 0),
};

static const struct snd_kcontrol_new mt7622_afe_o05_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I05 Switch", AFE_CONN5, 5, 1, 0),
};

static const struct snd_kcontrol_new mt7622_afe_o06_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I06 Switch", AFE_CONN6, 6, 1, 0),
};

static const struct snd_kcontrol_new mt7622_afe_o07_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I07 Switch", AFE_CONN7, 7, 1, 0),
};

static const struct snd_kcontrol_new mt7622_afe_o15_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I12 Switch", AFE_CONN15, 12, 1, 0),
};

static const struct snd_kcontrol_new mt7622_afe_o16_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I13 Switch", AFE_CONN16, 13, 1, 0),
};

static const struct snd_kcontrol_new mt7622_afe_o17_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I14 Switch", AFE_CONN17, 14, 1, 0),
};

static const struct snd_kcontrol_new mt7622_afe_o18_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I15 Switch", AFE_CONN18, 15, 1, 0),
};

static const struct snd_kcontrol_new mt7622_afe_o19_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I16 Switch", AFE_CONN19, 16, 1, 0),
};

static const struct snd_kcontrol_new mt7622_afe_o20_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I17 Switch", AFE_CONN20, 17, 1, 0),
};

static const struct snd_kcontrol_new mt7622_afe_o21_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I18 Switch", AFE_CONN21, 18, 1, 0),
};

static const struct snd_kcontrol_new mt7622_afe_o22_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I19 Switch", AFE_CONN22, 19, 1, 0),
};

static const struct snd_kcontrol_new mt7622_afe_multi_ch_out_i2s0[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("Multich I2S0 Out Switch",
	ASYS_I2SO1_CON, 26, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("Multich DL1 Switch",
	AFE_MCH_OUT_CFG, 0, 1, 0),
};

static const struct snd_kcontrol_new mt7622_afe_multi_ch_out_i2s1[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("Multich I2S1 Out Switch",
	ASYS_I2SO2_CON, 26, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("Multich DL2 Switch",
	AFE_MCH_OUT_CFG, 1, 1, 0),
};

static const struct snd_kcontrol_new mt7622_afe_multi_ch_out_i2s2[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("Multich I2S2 Out Switch",
	PWR2_TOP_CON, 17, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("Multich DL3 Switch",
	AFE_MCH_OUT_CFG, 2, 1, 0),
};

static const struct snd_kcontrol_new mt7622_afe_multi_ch_out_i2s3[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("Multich I2S3 Out Switch",
	PWR2_TOP_CON, 18, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("Multich DL4 Switch",
	AFE_MCH_OUT_CFG, 3, 1, 0),
};

static const struct snd_kcontrol_new mt7622_afe_multi_ch_out_asrc0[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("Asrc0 out Switch", AUDIO_TOP_CON4, 14, 1,
	1),
};

static const struct snd_kcontrol_new mt7622_afe_multi_ch_out_asrc1[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("Asrc1 out Switch", AUDIO_TOP_CON4, 15, 1,
	1),
};

static const struct snd_kcontrol_new mt7622_afe_multi_ch_out_asrc2[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("Asrc2 out Switch", PWR2_TOP_CON, 6, 1,
	1),
};

static const struct snd_kcontrol_new mt7622_afe_multi_ch_out_asrc3[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("Asrc3 out Switch", PWR2_TOP_CON, 7, 1,
	1),
};

static const struct snd_soc_dapm_widget mt7622_afe_pcm_widgets[] = {
	/* inter-connections */
	SND_SOC_DAPM_MIXER("I00", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_MIXER("I01", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_MIXER("I02", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_MIXER("I03", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_MIXER("I04", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_MIXER("I05", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_MIXER("I06", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_MIXER("I07", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_MIXER("I12", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_MIXER("I13", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_MIXER("I14", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_MIXER("I15", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_MIXER("I16", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_MIXER("I17", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_MIXER("I18", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_MIXER("I19", SND_SOC_NOPM, 0, 0, NULL, 0),

	SND_SOC_DAPM_MIXER("O00", SND_SOC_NOPM, 0, 0, mt7622_afe_o00_mix,
	ARRAY_SIZE(mt7622_afe_o00_mix)),
	SND_SOC_DAPM_MIXER("O01", SND_SOC_NOPM, 0, 0, mt7622_afe_o01_mix,
	ARRAY_SIZE(mt7622_afe_o01_mix)),
	SND_SOC_DAPM_MIXER("O02", SND_SOC_NOPM, 0, 0, mt7622_afe_o02_mix,
	ARRAY_SIZE(mt7622_afe_o02_mix)),
	SND_SOC_DAPM_MIXER("O03", SND_SOC_NOPM, 0, 0, mt7622_afe_o03_mix,
	ARRAY_SIZE(mt7622_afe_o03_mix)),
	SND_SOC_DAPM_MIXER("O04", SND_SOC_NOPM, 0, 0, mt7622_afe_o04_mix,
	ARRAY_SIZE(mt7622_afe_o04_mix)),
	SND_SOC_DAPM_MIXER("O05", SND_SOC_NOPM, 0, 0, mt7622_afe_o05_mix,
	ARRAY_SIZE(mt7622_afe_o05_mix)),
	SND_SOC_DAPM_MIXER("O06", SND_SOC_NOPM, 0, 0, mt7622_afe_o06_mix,
	ARRAY_SIZE(mt7622_afe_o06_mix)),
	SND_SOC_DAPM_MIXER("O07", SND_SOC_NOPM, 0, 0, mt7622_afe_o07_mix,
	ARRAY_SIZE(mt7622_afe_o07_mix)),

	SND_SOC_DAPM_MIXER("O15", SND_SOC_NOPM, 0, 0, mt7622_afe_o15_mix,
	ARRAY_SIZE(mt7622_afe_o15_mix)),
	SND_SOC_DAPM_MIXER("O16", SND_SOC_NOPM, 0, 0, mt7622_afe_o16_mix,
	ARRAY_SIZE(mt7622_afe_o16_mix)),
	SND_SOC_DAPM_MIXER("O17", SND_SOC_NOPM, 0, 0, mt7622_afe_o17_mix,
	ARRAY_SIZE(mt7622_afe_o17_mix)),
	SND_SOC_DAPM_MIXER("O18", SND_SOC_NOPM, 0, 0, mt7622_afe_o18_mix,
	ARRAY_SIZE(mt7622_afe_o18_mix)),
	SND_SOC_DAPM_MIXER("O19", SND_SOC_NOPM, 0, 0, mt7622_afe_o19_mix,
	ARRAY_SIZE(mt7622_afe_o19_mix)),
	SND_SOC_DAPM_MIXER("O20", SND_SOC_NOPM, 0, 0, mt7622_afe_o20_mix,
	ARRAY_SIZE(mt7622_afe_o20_mix)),
	SND_SOC_DAPM_MIXER("O21", SND_SOC_NOPM, 0, 0, mt7622_afe_o21_mix,
	ARRAY_SIZE(mt7622_afe_o21_mix)),
	SND_SOC_DAPM_MIXER("O22", SND_SOC_NOPM, 0, 0, mt7622_afe_o22_mix,
	ARRAY_SIZE(mt7622_afe_o22_mix)),


	SND_SOC_DAPM_MIXER("I12I13", SND_SOC_NOPM, 0, 0,
	mt7622_afe_multi_ch_out_i2s0,
	ARRAY_SIZE(mt7622_afe_multi_ch_out_i2s0)),
	SND_SOC_DAPM_MIXER("I14I15", SND_SOC_NOPM, 0, 0,
	mt7622_afe_multi_ch_out_i2s1,
	ARRAY_SIZE(mt7622_afe_multi_ch_out_i2s1)),
	SND_SOC_DAPM_MIXER("I16I17", SND_SOC_NOPM, 0, 0,
	mt7622_afe_multi_ch_out_i2s2,
	ARRAY_SIZE(mt7622_afe_multi_ch_out_i2s2)),
	SND_SOC_DAPM_MIXER("I18I19", SND_SOC_NOPM, 0, 0,
	mt7622_afe_multi_ch_out_i2s3,
	ARRAY_SIZE(mt7622_afe_multi_ch_out_i2s3)),


	SND_SOC_DAPM_MIXER("ASRC_O0", SND_SOC_NOPM, 0, 0,
	mt7622_afe_multi_ch_out_asrc0,
	ARRAY_SIZE(mt7622_afe_multi_ch_out_asrc0)),
	SND_SOC_DAPM_MIXER("ASRC_O1", SND_SOC_NOPM, 0, 0,
	mt7622_afe_multi_ch_out_asrc1,
	ARRAY_SIZE(mt7622_afe_multi_ch_out_asrc1)),
	SND_SOC_DAPM_MIXER("ASRC_O2", SND_SOC_NOPM, 0, 0,
	mt7622_afe_multi_ch_out_asrc2,
	ARRAY_SIZE(mt7622_afe_multi_ch_out_asrc2)),
	SND_SOC_DAPM_MIXER("ASRC_O3", SND_SOC_NOPM, 0, 0,
	mt7622_afe_multi_ch_out_asrc3,
	ARRAY_SIZE(mt7622_afe_multi_ch_out_asrc3)),
};

static const struct snd_soc_dapm_route mt7622_afe_pcm_routes[] = {

	{"I2S0 Playback", NULL, "O15"},
	{"I2S0 Playback", NULL, "O16"},

	{"I2S1 Playback", NULL, "O17"},
	{"I2S1 Playback", NULL, "O18"},
	{"I2S2 Playback", NULL, "O19"},
	{"I2S2 Playback", NULL, "O20"},
	{"I2S3 Playback", NULL, "O21"},
	{"I2S3 Playback", NULL, "O22"},

	{"UL1", NULL, "O00"},
	{"UL1", NULL, "O01"},
	{"UL2", NULL, "O02"},
	{"UL2", NULL, "O03"},
	{"UL3", NULL, "O04"},
	{"UL3", NULL, "O05"},

	{"I00", NULL, "I2S0 Capture"},
	{"I01", NULL, "I2S0 Capture"},

	{"I02", NULL, "I2S1 Capture"},
	{"I03", NULL, "I2S1 Capture"},

	{"I04", NULL, "I2S2 Capture"},
	{"I05", NULL, "I2S2 Capture"},

	{"ASRC_O0", "Asrc0 out Switch", "DLM"},
	{"ASRC_O1", "Asrc1 out Switch", "DLM"},
	{"ASRC_O2", "Asrc2 out Switch", "DLM"},
	{"ASRC_O3", "Asrc3 out Switch", "DLM"},

	{"I12I13", "Multich I2S0 Out Switch", "ASRC_O0"},
	{"I14I15", "Multich I2S1 Out Switch", "ASRC_O1"},
	{"I16I17", "Multich I2S2 Out Switch", "ASRC_O2"},
	{"I18I19", "Multich I2S3 Out Switch", "ASRC_O3"},

	{"I12I13", NULL, "DL1"},
	{"I14I15", NULL, "DL2"},
	{"I16I17", NULL, "DL3"},
	{"I18I19", NULL, "DL4"},


	{ "I12", NULL, "I12I13" },
	{ "I13", NULL, "I12I13" },
	{ "I14", NULL, "I14I15" },
	{ "I15", NULL, "I14I15" },
	{ "I16", NULL, "I16I17" },
	{ "I17", NULL, "I16I17" },
	{ "I18", NULL, "I18I19" },
	{ "I19", NULL, "I18I19" },

	{ "O00", "I00 Switch", "I00" },
	{ "O01", "I01 Switch", "I01" },
	{ "O02", "I02 Switch", "I02" },
	{ "O03", "I03 Switch", "I03" },
	{ "O04", "I04 Switch", "I04" },
	{ "O05", "I05 Switch", "I05" },
	{ "O06", "I06 Switch", "I06" },
	{ "O07", "I07 Switch", "I07" },
	{ "O15", "I12 Switch", "I12" },
	{ "O16", "I13 Switch", "I13" },
	{ "O17", "I14 Switch", "I14" },
	{ "O18", "I15 Switch", "I15" },
	{ "O19", "I16 Switch", "I16" },
	{ "O20", "I17 Switch", "I17" },
	{ "O21", "I18 Switch", "I18" },
	{ "O22", "I19 Switch", "I19" },

};

static const struct snd_soc_component_driver mt7622_afe_pcm_dai_component = {
	.name = "mt7622-afe-pcm-dai",
	.dapm_widgets = mt7622_afe_pcm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(mt7622_afe_pcm_widgets),
	.dapm_routes = mt7622_afe_pcm_routes,
	.num_dapm_routes = ARRAY_SIZE(mt7622_afe_pcm_routes),
};

static const struct mtk_base_memif_data memif_data[MT7622_MEMIF_NUM] = {
	[MT7622_MEMIF_DL1] = {
		.name = "DL1",
		.id = MT7622_MEMIF_DL1,
		.reg_ofs_base = AFE_DL1_BASE,
		.reg_ofs_cur = AFE_DL1_CUR,
		.fs_reg = AFE_DAC_CON1,
		.fs_shift = 0,
		.fs_maskbit = 0x1f,
		.mono_reg = AFE_DAC_CON3,
		.mono_shift = 16,
		.enable_reg = AFE_DAC_CON0,
		.enable_shift = 1,
		.hd_reg = AFE_MEMIF_HD_CON0,
		.hd_shift = 0,
		.agent_disable_reg = AUDIO_TOP_CON5,
		.agent_disable_shift = 6,
		.msb_reg = -1,
		.msb_shift = -1,
		.msb2_reg = -1,
		.msb2_shift = -1,
	},
	[MT7622_MEMIF_DL2] = {
		.name = "DL2",
		.id = MT7622_MEMIF_DL2,
		.reg_ofs_base = AFE_DL2_BASE,
		.reg_ofs_cur = AFE_DL2_CUR,
		.fs_reg = AFE_DAC_CON1,
		.fs_shift = 5,
		.fs_maskbit = 0x1f,
		.mono_reg = AFE_DAC_CON3,
		.mono_shift = 17,
		.enable_reg = AFE_DAC_CON0,
		.enable_shift = 2,
		.hd_reg = AFE_MEMIF_HD_CON0,
		.hd_shift = 2,
		.agent_disable_reg = AUDIO_TOP_CON5,
		.agent_disable_shift = 7,
		.msb_reg = -1,
		.msb_shift = -1,
		.msb2_reg = -1,
		.msb2_shift = -1,
	},
	[MT7622_MEMIF_DL3] = {
		.name = "DL3",
		.id = MT7622_MEMIF_DL3,
		.reg_ofs_base = AFE_DL3_BASE,
		.reg_ofs_cur = AFE_DL3_CUR,
		.fs_reg = AFE_DAC_CON1,
		.fs_shift = 10,
		.fs_maskbit = 0x1f,
		.mono_reg = AFE_DAC_CON3,
		.mono_shift = 18,
		.enable_reg = AFE_DAC_CON0,
		.enable_shift = 3,
		.hd_reg = AFE_MEMIF_HD_CON0,
		.hd_shift = 4,
		.agent_disable_reg = AUDIO_TOP_CON5,
		.agent_disable_shift = 8,
		.msb_reg = -1,
		.msb_shift = -1,
		.msb2_reg = -1,
		.msb2_shift = -1,
	},
	[MT7622_MEMIF_DL4] = {
		.name = "DL4",
		.id = MT7622_MEMIF_DL4,
		.reg_ofs_base = AFE_DL4_BASE,
		.reg_ofs_cur = AFE_DL4_CUR,
		.fs_reg = AFE_DAC_CON1,
		.fs_shift = 15,
		.fs_maskbit = 0x1f,
		.mono_reg = AFE_DAC_CON3,
		.mono_shift = 19,
		.enable_reg = AFE_DAC_CON0,
		.enable_shift = 4,
		.hd_reg = AFE_MEMIF_HD_CON0,
		.hd_shift = 6,
		.agent_disable_reg = AUDIO_TOP_CON5,
		.agent_disable_shift = 9,
		.msb_reg = -1,
		.msb_shift = -1,
		.msb2_reg = -1,
		.msb2_shift = -1,
	},
	[MT7622_MEMIF_DLM] = {
		.name = "DLM",
		.id = MT7622_MEMIF_DLM,
		.reg_ofs_base = AFE_DLMCH_BASE,
		.reg_ofs_cur = AFE_DLMCH_CUR,
		.fs_reg = AFE_DAC_CON1,
		.fs_shift = 0,
		.fs_maskbit = 0x1f,
		.mono_reg = -1,
		.mono_shift = -1,
		.enable_reg = AFE_DAC_CON0,
		.enable_shift = 7,
		.hd_reg = AFE_MEMIF_PBUF_SIZE,
		.hd_shift = 28,
		.agent_disable_reg = AUDIO_TOP_CON5,
		.agent_disable_shift = 12,
		.msb_reg = -1,
		.msb_shift = -1,
		.msb2_reg = -1,
		.msb2_shift = -1,
	},
	[MT7622_MEMIF_UL1] = {
		.name = "UL1",
		.id = MT7622_MEMIF_UL1,
		.reg_ofs_base = AFE_VUL_BASE,
		.reg_ofs_cur = AFE_VUL_CUR,
		.fs_reg = AFE_DAC_CON2,
		.fs_shift = 0,
		.fs_maskbit = 0x1f,
		.mono_reg = AFE_DAC_CON4,
		.mono_shift = 0,
		.enable_reg = AFE_DAC_CON0,
		.enable_shift = 10,
		.hd_reg = AFE_MEMIF_HD_CON1,
		.hd_shift = 0,
		.agent_disable_reg = AUDIO_TOP_CON5,
		.agent_disable_shift = 0,
		.msb_reg = -1,
		.msb_shift = -1,
		.msb2_reg = -1,
		.msb2_shift = -1,
	},
	[MT7622_MEMIF_UL2] = {
		.name = "UL2",
		.id = MT7622_MEMIF_UL2,
		.reg_ofs_base = AFE_UL2_BASE,
		.reg_ofs_cur = AFE_UL2_CUR,
		.fs_reg = AFE_DAC_CON2,
		.fs_shift = 5,
		.fs_maskbit = 0x1f,
		.mono_reg = AFE_DAC_CON4,
		.mono_shift = 2,
		.enable_reg = AFE_DAC_CON0,
		.enable_shift = 11,
		.hd_reg = AFE_MEMIF_HD_CON1,
		.hd_shift = 2,
		.agent_disable_reg = AUDIO_TOP_CON5,
		.agent_disable_shift = 1,
		.msb_reg = -1,
		.msb_shift = -1,
		.msb2_reg = -1,
		.msb2_shift = -1,
	},
	[MT7622_MEMIF_UL3] = {
		.name = "UL3",
		.id = MT7622_MEMIF_UL3,
		.reg_ofs_base = AFE_UL3_BASE,
		.reg_ofs_cur = AFE_UL3_CUR,
		.fs_reg = AFE_DAC_CON2,
		.fs_shift = 10,
		.fs_maskbit = 0x1f,
		.mono_reg = AFE_DAC_CON4,
		.mono_shift = 4,
		.enable_reg = AFE_DAC_CON0,
		.enable_shift = 12,
		.hd_reg = AFE_MEMIF_HD_CON0,
		.hd_shift = 0,
		.agent_disable_reg = AUDIO_TOP_CON5,
		.agent_disable_shift = 2,
		.msb_reg = -1,
		.msb_shift = -1,
		.msb2_reg = -1,
		.msb2_shift = -1,
	},
	[MT7622_MEMIF_UL4] = {
		.name = "UL4",
		.id = MT7622_MEMIF_UL4,
		.reg_ofs_base = AFE_UL4_BASE,
		.reg_ofs_cur = AFE_UL4_CUR,
		.fs_reg = AFE_DAC_CON2,
		.fs_shift = 15,
		.fs_maskbit = 0x1f,
		.mono_reg = AFE_DAC_CON4,
		.mono_shift = 6,
		.enable_reg = AFE_DAC_CON0,
		.enable_shift = 13,
		.hd_reg = AFE_MEMIF_HD_CON0,
		.hd_shift = 6,
		.agent_disable_reg = AUDIO_TOP_CON5,
		.agent_disable_shift = 3,
		.msb_reg = -1,
		.msb_shift = -1,
		.msb2_reg = -1,
		.msb2_shift = -1,
	},
	[MT7622_MEMIF_DLTDM] = {
		.name = "DLTDM",
		.id = MT7622_MEMIF_DLTDM,
		.reg_ofs_base = AFE_TDM_OUT_BASE,
		.reg_ofs_cur = AFE_TDM_OUT_CUR,
		.fs_reg = -1,
		.fs_shift = -1,
		.fs_maskbit = 0x0,
		.mono_reg = -1,
		.mono_shift = -1,
		.enable_reg = AFE_TDM_OUT_CON0,
		.enable_shift = 0,
		.hd_reg = -1,
		.hd_shift = -1,
		.agent_disable_reg = -1,
		.agent_disable_shift = -1,
		.msb_reg = -1,
		.msb_shift = -1,
		.msb2_reg = -1,
		.msb2_shift = -1,
	},
	[MT7622_MEMIF_ULTDM] = {
		.name = "ULTDM",
		.id = MT7622_MEMIF_ULTDM,
		.reg_ofs_base = AFE_TDM_IN_BASE,
		.reg_ofs_cur = AFE_TDM_IN_CUR,
		.fs_reg = -1,
		.fs_shift = -1,
		.fs_maskbit = 0x0,
		.mono_reg = -1,
		.mono_shift = -1,
		.enable_reg = AFE_DAC_CON0,
		.enable_shift = 25,
		.hd_reg = AFE_MEMIF_HD_CON1,
		.hd_shift = 18,
		.agent_disable_reg = -1,
		.agent_disable_shift = -1,
		.msb_reg = -1,
		.msb_shift = -1,
		.msb2_reg = -1,
		.msb2_shift = -1,
	},
};

static const struct mtk_base_irq_data irq_data[MT7622_IRQ_ASYS_END] = {
	{
		.id = MT7622_IRQ_ASYS_IRQ1_DL1,
		.irq_cnt_reg = ASYS_IRQ1_CON,
		.irq_cnt_shift = 0,
		.irq_cnt_maskbit = 0xffffff,
		.irq_fs_reg = ASYS_IRQ1_CON,
		.irq_fs_shift = 24,
		.irq_fs_maskbit = 0x1f,
		.irq_en_reg = ASYS_IRQ1_CON,
		.irq_en_shift = 31,
		.irq_clr_reg = ASYS_IRQ_CLR,
		.irq_clr_shift = 0,
	},
	{
		.id = MT7622_IRQ_ASYS_IRQ2_DL2,
		.irq_cnt_reg = ASYS_IRQ2_CON,
		.irq_cnt_shift = 0,
		.irq_cnt_maskbit = 0xffffff,
		.irq_fs_reg = ASYS_IRQ2_CON,
		.irq_fs_shift = 24,
		.irq_fs_maskbit = 0x1f,
		.irq_en_reg = ASYS_IRQ2_CON,
		.irq_en_shift = 31,
		.irq_clr_reg = ASYS_IRQ_CLR,
		.irq_clr_shift = 1,
	},
	{
		.id = MT7622_IRQ_ASYS_IRQ3_DL3,
		.irq_cnt_reg = ASYS_IRQ3_CON,
		.irq_cnt_shift = 0,
		.irq_cnt_maskbit = 0xffffff,
		.irq_fs_reg = ASYS_IRQ3_CON,
		.irq_fs_shift = 24,
		.irq_fs_maskbit = 0x1f,
		.irq_en_reg = ASYS_IRQ3_CON,
		.irq_en_shift = 31,
		.irq_clr_reg = ASYS_IRQ_CLR,
		.irq_clr_shift = 2,
	},
	{
		.id = MT7622_IRQ_ASYS_IRQ4_DL4,
		.irq_cnt_reg = ASYS_IRQ4_CON,
		.irq_cnt_shift = 0,
		.irq_cnt_maskbit = 0xffffff,
		.irq_fs_reg = ASYS_IRQ4_CON,
		.irq_fs_shift = 24,
		.irq_fs_maskbit = 0x1f,
		.irq_en_reg = ASYS_IRQ4_CON,
		.irq_en_shift = 31,
		.irq_clr_reg = ASYS_IRQ_CLR,
		.irq_clr_shift = 3,
	},
	{
		.id = MT7622_IRQ_ASYS_IRQ5_DLM,
		.irq_cnt_reg = ASYS_IRQ5_CON,
		.irq_cnt_shift = 0,
		.irq_cnt_maskbit = 0xffffff,
		.irq_fs_reg = ASYS_IRQ5_CON,
		.irq_fs_shift = 24,
		.irq_fs_maskbit = 0x1f,
		.irq_en_reg = ASYS_IRQ5_CON,
		.irq_en_shift = 31,
		.irq_clr_reg = ASYS_IRQ_CLR,
		.irq_clr_shift = 4,
	},
	{
		.id = MT7622_IRQ_ASYS_IRQ6_UL1,
		.irq_cnt_reg = ASYS_IRQ6_CON,
		.irq_cnt_shift = 0,
		.irq_cnt_maskbit = 0xffffff,
		.irq_fs_reg = ASYS_IRQ6_CON,
		.irq_fs_shift = 24,
		.irq_fs_maskbit = 0x1f,
		.irq_en_reg = ASYS_IRQ6_CON,
		.irq_en_shift = 31,
		.irq_clr_reg = ASYS_IRQ_CLR,
		.irq_clr_shift = 5,
	},
	{
		.id = MT7622_IRQ_ASYS_IRQ7_UL2,
		.irq_cnt_reg = ASYS_IRQ7_CON,
		.irq_cnt_shift = 0,
		.irq_cnt_maskbit = 0xffffff,
		.irq_fs_reg = ASYS_IRQ7_CON,
		.irq_fs_shift = 24,
		.irq_fs_maskbit = 0x1f,
		.irq_en_reg = ASYS_IRQ7_CON,
		.irq_en_shift = 31,
		.irq_clr_reg = ASYS_IRQ_CLR,
		.irq_clr_shift = 6,
	},
	{
		.id = MT7622_IRQ_ASYS_IRQ8_UL3,
		.irq_cnt_reg = ASYS_IRQ8_CON,
		.irq_cnt_shift = 0,
		.irq_cnt_maskbit = 0xffffff,
		.irq_fs_reg = ASYS_IRQ8_CON,
		.irq_fs_shift = 24,
		.irq_fs_maskbit = 0x1f,
		.irq_en_reg = ASYS_IRQ8_CON,
		.irq_en_shift = 31,
		.irq_clr_reg = ASYS_IRQ_CLR,
		.irq_clr_shift = 7,
	},
	{
		.id = MT7622_IRQ_ASYS_IRQ9_UL4,
		.irq_cnt_reg = ASYS_IRQ9_CON,
		.irq_cnt_shift = 0,
		.irq_cnt_maskbit = 0xffffff,
		.irq_fs_reg = ASYS_IRQ9_CON,
		.irq_fs_shift = 24,
		.irq_fs_maskbit = 0x1f,
		.irq_en_reg = ASYS_IRQ9_CON,
		.irq_en_shift = 31,
		.irq_clr_reg = ASYS_IRQ_CLR,
		.irq_clr_shift = 8,
	},
	{
		.id = MT7622_IRQ_ASYS_IRQ10_TDMOUT,
		.irq_cnt_reg = ASYS_IRQ10_CON,
		.irq_cnt_shift = 0,
		.irq_cnt_maskbit = 0xffffff,
		.irq_fs_reg = ASYS_IRQ10_CON,
		.irq_fs_shift = 24,
		.irq_fs_maskbit = 0x1f,
		.irq_en_reg = ASYS_IRQ10_CON,
		.irq_en_shift = 31,
		.irq_clr_reg = ASYS_IRQ_CLR,
		.irq_clr_shift = 9,
	},
	{
		.id = MT7622_IRQ_ASYS_IRQ11_TDMIN,
		.irq_cnt_reg = ASYS_IRQ11_CON,
		.irq_cnt_shift = 0,
		.irq_cnt_maskbit = 0xffffff,
		.irq_fs_reg = ASYS_IRQ11_CON,
		.irq_fs_shift = 24,
		.irq_fs_maskbit = 0x1f,
		.irq_en_reg = ASYS_IRQ11_CON,
		.irq_en_shift = 31,
		.irq_clr_reg = ASYS_IRQ_CLR,
		.irq_clr_shift = 10,
	},
};

static const struct mt7622_i2s_data mt7622_i2s_data[MT7622_I2S_NUM][2] = {
	{
		{
			.i2s_ctrl_reg = ASYS_I2SO1_CON,
			.i2s_pwn_shift = 6,
			.i2s_asrc_fs_shift = 0,
			.i2s_asrc_fs_mask = 0x1f,

		},
		{
			.i2s_ctrl_reg = ASYS_I2SIN1_CON,
			.i2s_pwn_shift = 0,
			.i2s_asrc_fs_shift = 0,
			.i2s_asrc_fs_mask = 0x1f,

		},
	},
	{
		{
			.i2s_ctrl_reg = ASYS_I2SO2_CON,
			.i2s_pwn_shift = 7,
			.i2s_asrc_fs_shift = 5,
			.i2s_asrc_fs_mask = 0x1f,

		},
		{
			.i2s_ctrl_reg = ASYS_I2SIN2_CON,
			.i2s_pwn_shift = 1,
			.i2s_asrc_fs_shift = 5,
			.i2s_asrc_fs_mask = 0x1f,

		},
	},
	{
		{
			.i2s_ctrl_reg = ASYS_I2SO3_CON,
			.i2s_pwn_shift = 8,
			.i2s_asrc_fs_shift = 10,
			.i2s_asrc_fs_mask = 0x1f,

		},
		{
			.i2s_ctrl_reg = ASYS_I2SIN3_CON,
			.i2s_pwn_shift = 2,
			.i2s_asrc_fs_shift = 10,
			.i2s_asrc_fs_mask = 0x1f,

		},
	},
	{
		{
			.i2s_ctrl_reg = ASYS_I2SO4_CON,
			.i2s_pwn_shift = 9,
			.i2s_asrc_fs_shift = 15,
			.i2s_asrc_fs_mask = 0x1f,

		},
		{
			.i2s_ctrl_reg = ASYS_I2SIN4_CON,
			.i2s_pwn_shift = 3,
			.i2s_asrc_fs_shift = 15,
			.i2s_asrc_fs_mask = 0x1f,

		},
	},
};

static const struct mt7622_tdm_data mt7622_tdm_data[MT7622_TDM_NUM] = {
	{
		.tdm_ctrl_reg = AFE_TDM_CON1,
		.tdm_lrck_cycle_shift = 10,
		.tdm_on_shift = 0,
		.tdm_out_reg = AFE_TDM_OUT_CON0,
		.tdm_out_bit_width_shift = 1,
		.tdm_out_ch_num_shift = 4,
	},
	{
		.tdm_ctrl_reg = AFE_TDM_IN_CON1,
		.tdm_lrck_cycle_shift = 16,
		.tdm_on_shift = 0,
		.tdm_out_reg = -1,
		.tdm_out_bit_width_shift = -1,
		.tdm_out_ch_num_shift = -1,
	},
};

static const struct regmap_config mt7622_afe_regmap_config = {
	.reg_bits = 32,
	.reg_stride = 4,
	.val_bits = 32,
	.max_register = AFE_END_ADDR,
	.cache_type = REGCACHE_NONE,
};

static irqreturn_t mt7622_asys_isr(int irq_id, void *dev)
{
	int id;
	struct mtk_base_afe *afe = dev;
	struct mtk_base_afe_memif *memif;
	struct mtk_base_afe_irq *irq;
	u32 status;

	regmap_read(afe->regmap, ASYS_IRQ_STATUS, &status);
	regmap_write(afe->regmap, ASYS_IRQ_CLR, status);

	for (id = MT7622_MEMIF_DL1; id < MT7622_MEMIF_NUM; ++id) {
		memif = &afe->memif[id];

		if (memif->irq_usage < 0)
			continue;
		irq = &afe->irqs[memif->irq_usage];
		if (status & 1 << (irq->irq_data->irq_clr_shift))
			snd_pcm_period_elapsed(memif->substream);
	}

	return IRQ_HANDLED;
}

static irqreturn_t mt7622_afe_isr(int irq_id, void *dev)
{
	int id;
	struct mtk_base_afe *afe = dev;
	struct mtk_base_afe_memif *memif;
	struct mtk_base_afe_irq *irq;
	u32 status;

	regmap_read(afe->regmap, AFE_IRQ_STATUS, &status);
	regmap_write(afe->regmap, AFE_IRQ_CLR, status);

	for (id = MT7622_MEMIF_DLTDM; id < MT7622_MEMIF_NUM; ++id) {
		memif = &afe->memif[id];
		if (memif->irq_usage < 0)
			continue;
		irq = &afe->irqs[memif->irq_usage];
		if (status & 1 << (irq->irq_data->irq_clr_shift))
			snd_pcm_period_elapsed(memif->substream);
	}

	return IRQ_HANDLED;
}

static int mt7622_afe_runtime_suspend(struct device *dev)
{
	struct mtk_base_afe *afe = dev_get_drvdata(dev);

	mt7622_afe_disable_clock(afe);
	return 0;
}

static int mt7622_afe_runtime_resume(struct device *dev)
{
	struct mtk_base_afe *afe = dev_get_drvdata(dev);

	return mt7622_afe_enable_clock(afe);
}

static int mt7622_afe_pcm_dev_probe(struct platform_device *pdev)
{
	int ret, i;
	unsigned int ays_irq_id, afe_irq_id;
	struct mtk_base_afe *afe;
	struct mt7622_afe_private *afe_priv;
	struct resource *res;
	struct device *dev;

	ret = 0;
	afe = devm_kzalloc(&pdev->dev, sizeof(*afe), GFP_KERNEL);
	if (!afe)
		return -ENOMEM;
	afe->platform_priv = devm_kzalloc(&pdev->dev, sizeof(*afe_priv),
					  GFP_KERNEL);
	if (!afe->platform_priv)
		return -ENOMEM;
	afe_priv = afe->platform_priv;

	afe->dev = &pdev->dev;
	dev = afe->dev;

	/* asys irq request irq number:145*/
	ays_irq_id = platform_get_irq(pdev, 0);
	if (!ays_irq_id) {
		dev_err(dev, "%s no irq found\n", dev->of_node->name);
		return -ENXIO;
	}

	ret = devm_request_irq(dev, ays_irq_id, mt7622_asys_isr,
			       IRQF_TRIGGER_NONE, "asys-isr", (void *)afe);
	if (ret) {
		dev_err(dev, "could not request_irq for asys-isr\n");
		return ret;
	}

	/* asys irq request irq number:144*/
	afe_irq_id = platform_get_irq(pdev, 1);
	if (!afe_irq_id) {
		dev_err(dev, "%s no irq found\n", dev->of_node->name);
		return -ENXIO;
	}

	ret = devm_request_irq(dev, afe_irq_id, mt7622_afe_isr,
			       IRQF_TRIGGER_NONE, "afe-isr", (void *)afe);
	if (ret) {
		dev_err(dev, "could not request_irq for afe-isr\n");
		return ret;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	afe->base_addr = devm_ioremap_resource(&pdev->dev, res);

	if (IS_ERR(afe->base_addr))
		return PTR_ERR(afe->base_addr);

	afe->regmap = devm_regmap_init_mmio(&pdev->dev, afe->base_addr,
					    &mt7622_afe_regmap_config);
	if (IS_ERR(afe->regmap))
		return PTR_ERR(afe->regmap);

	/* memif initialize */
	afe->memif_size = MT7622_MEMIF_NUM;
	afe->memif = devm_kcalloc(dev, afe->memif_size, sizeof(*afe->memif),
				  GFP_KERNEL);

	if (!afe->memif)
		return -ENOMEM;

	for (i = MT7622_MEMIF_DL1; i < afe->memif_size; i++) {
		afe->memif[i].data = &memif_data[i];
		afe->memif[i].irq_usage = i;
		afe->memif[i].const_irq = 1;
	}

	/* irq initialize */
	afe->irqs_size = MT7622_IRQ_ASYS_END;
	afe->irqs = devm_kcalloc(dev, afe->irqs_size, sizeof(*afe->irqs),
				 GFP_KERNEL);

	if (!afe->irqs)
		return -ENOMEM;

	for (i = 0; i < afe->irqs_size; i++) {
		afe->irqs[i].irq_data = &irq_data[i];
		afe->irqs[i].irq_occupyed = true;
	}

	/* I2S initialize */
	for (i = 0; i < MT7622_I2S_NUM; i++) {
		afe_priv->i2s_path[i].i2s_data[I2S_OUT]
			= &mt7622_i2s_data[i][I2S_OUT];
		afe_priv->i2s_path[i].i2s_data[I2S_IN]
			= &mt7622_i2s_data[i][I2S_IN];
	}

	/* TDM initialize */
	for (i = 0; i < MT7622_TDM_NUM; i++) {
		afe_priv->tdm_path[i].tdm_data
			= &mt7622_tdm_data[i];
		afe_priv->tdm_path[i].tdm_data
			= &mt7622_tdm_data[i];
	}

	afe->mtk_afe_hardware = &mt7622_afe_hardware;
	afe->memif_fs = mt7622_memif_fs;
	afe->irq_fs = mt7622_irq_fs;

	afe->reg_back_up_list = mt7622_afe_backup_list;
	afe->reg_back_up_list_num = ARRAY_SIZE(mt7622_afe_backup_list);
	afe->runtime_resume = mt7622_afe_runtime_resume;
	afe->runtime_suspend = mt7622_afe_runtime_suspend;

	/* initial audio related clock */
	ret = mt7622_init_clock(afe);
	if (ret) {
		dev_err(dev, "init clock error\n");
		return ret;
	}

	platform_set_drvdata(pdev, afe);
	pm_runtime_enable(&pdev->dev);
	if (!pm_runtime_enabled(&pdev->dev))
		goto err_pm_disable;
	pm_runtime_get_sync(&pdev->dev);

	ret = snd_soc_register_platform(&pdev->dev, &mtk_afe_pcm_platform);
	if (ret) {
		dev_warn(dev, "err_platform\n");
		goto err_platform;
	}

	ret = snd_soc_register_component(&pdev->dev,
					 &mt7622_afe_pcm_dai_component,
					 mt7622_afe_pcm_dais,
					 ARRAY_SIZE(mt7622_afe_pcm_dais));
	if (ret)
		goto err_dai_component;

	mt7622_afe_runtime_resume(&pdev->dev);

	return 0;

err_dai_component:
	snd_soc_unregister_component(&pdev->dev);

err_platform:
	snd_soc_unregister_platform(&pdev->dev);

err_pm_disable:
	pm_runtime_disable(&pdev->dev);

	return ret;
}

static int mt7622_afe_pcm_dev_remove(struct platform_device *pdev)
{
	struct mtk_base_afe *afe = platform_get_drvdata(pdev);

	pm_runtime_disable(&pdev->dev);
	if (!pm_runtime_status_suspended(&pdev->dev))
		mt7622_afe_runtime_suspend(&pdev->dev);
	pm_runtime_put_sync(&pdev->dev);

	snd_soc_unregister_component(&pdev->dev);
	snd_soc_unregister_platform(&pdev->dev);
	/* disable afe clock */
	mt7622_afe_disable_clock(afe);
	return 0;
}

static const struct of_device_id mt7622_afe_pcm_dt_match[] = {
	{ .compatible = "mediatek,mt7622-audio", },
	{ .compatible = "mediatek,mt7622-audio", },
	{},
};
MODULE_DEVICE_TABLE(of, mt7622_afe_pcm_dt_match);

static const struct dev_pm_ops mt7622_afe_pm_ops = {
	.suspend = mt7622_afe_runtime_suspend,
	.resume = mt7622_afe_runtime_resume,
};

static struct platform_driver mt7622_afe_pcm_driver = {
	.driver = {
		.name = "mt7622-audio",
		.of_match_table = mt7622_afe_pcm_dt_match,
#ifdef CONFIG_PM
		.pm = &mt7622_afe_pm_ops,
#endif
	},
	.probe = mt7622_afe_pcm_dev_probe,
	.remove = mt7622_afe_pcm_dev_remove,
};

module_platform_driver(mt7622_afe_pcm_driver);

MODULE_DESCRIPTION("Mediatek ALSA SoC AFE platform driver for 7622");
MODULE_AUTHOR("Unknown");
MODULE_LICENSE("GPL v2");

