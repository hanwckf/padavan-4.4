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

#include <sound/soc.h>
#include <sound/pcm_params.h>
#include "mt8521p-aud-global.h"
#ifndef AUD_PINCTRL_SUPPORTING
#include <mach/mt_gpio.h>
#endif
#include "mt8521p-dai.h"
#include "mt8521p-afe.h"
#include "mt8521p-dai-private.h"
#include "mt8521p-aud-gpio.h"

static struct audio_dmic *get_dmic_in(struct mt_dmic_all *dmic_all, int dai_id)
{
	int i;

	for (i = 0; i < AFE_DMIC_NUM; ++i) {
		if (dmic_all->dmic_in[i].dai_id == dai_id)
			return &dmic_all->dmic_in[i];
	}
	return NULL;
}

static enum afe_dmic_id get_dmic_id(int dai_id)
{
	switch (dai_id) {
	case MT_DAI_DMIC1_ID:
			return AFE_DMIC_1;
	case MT_DAI_DMIC2_ID:
		return AFE_DMIC_2;
	default:
		return AFE_DMIC_NUM;
	}
}

static int mt8521p_dmic_startup(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct mt_dai_private *priv;
	struct audio_dmic *in;
	enum afe_dmic_id id;

	dev_dbg(dai->dev, "%s() cpu_dai id %d\n", __func__, dai->id);
	if (substream->stream != SNDRV_PCM_STREAM_CAPTURE)
		return -EINVAL;
	priv = dev_get_drvdata(dai->dev);
	in = get_dmic_in(&priv->dmic_all, dai->id);
	if (!in)
		return -EINVAL;
	if (in->occupied)
		return -EINVAL;
	id = get_dmic_id(in->dai_id);
	dev_dbg(dai->dev, "%s() set gpio for dmic%d\n", __func__, id + 1);
#ifdef AUD_PINCTRL_SUPPORTING
	mt8521p_gpio_i2s2_select(I2S2_MODE4_DMIC);
#else
	if (id == AFE_DMIC_1) {
		mt_set_gpio_mode(GPIO38, GPIO_MODE_04);
		mt_set_gpio_mode(GPIO51, GPIO_MODE_04);
	} else if (id == AFE_DMIC_2) {
		mt_set_gpio_mode(GPIO50, GPIO_MODE_04);
		mt_set_gpio_mode(GPIO52, GPIO_MODE_04);
	} else {
		dev_err(dai->dev, "%s() invalid dmic id %u\n", __func__, id);
		return -EINVAL;
	}
#endif
	in->occupied = 1;
	afe_power_on_dmic(id, 1);
	return 0;
}

static void mt8521p_dmic_shutdown(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct mt_dai_private *priv;
	struct audio_dmic *in;
	enum afe_dmic_id id;

	dev_dbg(dai->dev, "%s() cpu_dai id %d\n", __func__, dai->id);
	priv = dev_get_drvdata(dai->dev);
	in = get_dmic_in(&priv->dmic_all, dai->id);
	if (!in)
		return;
	in->occupied = 0;
	id = get_dmic_id(in->dai_id);
	afe_power_on_dmic(id, 0);
}

static int dmic_configurate(const struct audio_dmic *in, int on)
{
	int ret;
	enum afe_dmic_id id;

	if (!in)
		return -EINVAL;
	id = get_dmic_id(in->dai_id);
	if (on) {
		struct afe_dmic_config config = {
			.one_wire_mode = 1,
			.iir_on = 0,
			.iir_mode = 0,
			.voice_mode = in->stream_fs
		};

		ret = afe_dmic_configurate(id, &config);
		if (ret < 0)
			return ret;
		ret = afe_dmic_enable(id, 1);
		if (ret < 0)
			return ret;
	} else {
		ret = afe_dmic_enable(id, 0);
		if (ret < 0)
			return ret;
	}
	return 0;
}

static int mt8521p_dmic_hw_params(struct snd_pcm_substream *substream,
				 struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	struct mt_dai_private *priv;
	struct audio_dmic *in;

	dev_dbg(dai->dev, "%s() cpu_dai id %d\n", __func__, dai->id);
	priv = dev_get_drvdata(dai->dev);
	in = get_dmic_in(&priv->dmic_all, dai->id);
	if (!in)
		return -EINVAL;
	in->stream_fs = fs_enum(params_rate(params));
	return dmic_configurate(in, 1);
}

static int mt8521p_dmic_hw_free(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct mt_dai_private *priv;
	struct audio_dmic *in;

	pr_debug("%s() cpu_dai id %d\n", __func__, dai->id);
	priv = dev_get_drvdata(dai->dev);
	in = get_dmic_in(&priv->dmic_all, dai->id);
	return dmic_configurate(in, 0);
}

static void init_mt_dmic_all(struct mt_dmic_all *dmic_all)
{
	if (!dmic_all->inited) {
		int dai_id;

		for (dai_id = MT_DAI_DMIC1_ID; dai_id <= MT_DAI_DMIC2_ID; ++dai_id)
			dmic_all->dmic_in[get_dmic_id(dai_id)].dai_id = dai_id;
		dmic_all->inited = 1;
	}
}

static int mt8521p_dmic_probe(struct snd_soc_dai *dai)
{
	struct mt_dai_private *priv;

	dev_dbg(dai->dev, "%s() cpu_dai id %d\n", __func__, dai->id);
	priv = dev_get_drvdata(dai->dev);
	init_mt_dmic_all(&priv->dmic_all);
	return 0;
}

static struct snd_soc_dai_ops mt8521p_dmic_ops = {
	.startup = mt8521p_dmic_startup,
	.shutdown = mt8521p_dmic_shutdown,
	.hw_params = mt8521p_dmic_hw_params,
	.hw_free = mt8521p_dmic_hw_free,
};
