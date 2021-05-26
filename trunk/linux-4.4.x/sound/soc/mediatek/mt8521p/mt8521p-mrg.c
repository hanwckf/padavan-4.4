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
#include "mt8521p-dai.h"
#include "mt8521p-afe.h"
#include "mt8521p-dai-private.h"
/*#include <mach/mt_gpio.h>*/
#include "mt8521p-aud-gpio.h"
#include "mt8521p-aud-global.h"

static void mrg_power(int on)
{
	if (on) {
		#ifdef AUD_PINCTRL_SUPPORTING
		mt8521p_gpio_pcm_select(PCM_MODE2_MRG);
		#else
		mt_set_gpio_mode(GPIO18, GPIO_MODE_02);
		mt_set_gpio_mode(GPIO19, GPIO_MODE_02);
		mt_set_gpio_mode(GPIO20, GPIO_MODE_02);
		mt_set_gpio_mode(GPIO21, GPIO_MODE_02);
		#endif
		#ifdef CONFIG_MTK_LEGACY_CLOCK
		enable_clock(MT_CG_AUDIO_AFE_MRGIF, "AUDIO");/* AUDIO_TOP_CON4[25] */
		#endif
	} else {
		#ifdef CONFIG_MTK_LEGACY_CLOCK
		disable_clock(MT_CG_AUDIO_AFE_MRGIF, "AUDIO");
		#endif
	}
	afe_power_on_mrg(on);
}

static int mt8521p_btmrg_startup(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct mt_dai_private *priv;

	pr_debug("%s() cpu_dai id %d\n", __func__, dai->id);
	priv = dev_get_drvdata(dai->dev);
	if (priv->daibt.occupied[substream->stream]) {
		pr_err(
		       "%s() error: can't open MT8521p_DAI_MRGIF_BT_ID(dir %d) because it has been occupied\n",
		       __func__, substream->stream);
		return -EINVAL;
	}
	if (priv->btpcm.occupied[substream->stream]) {
		pr_err("%s() error: can't open MT8521p_DAI_MRGIF_BT_ID(dir %d) because MT8521p_DAI_BTPCM_ID(dir %d) has been occupied\n",
		       __func__, substream->stream, substream->stream);
		return -EINVAL;
	}

	mrg_power(1);

	substream->stream == SNDRV_PCM_STREAM_PLAYBACK
		? afe_o31_pcm_tx_sel_pcmtx(0)
		: afe_i26_pcm_rx_sel_pcmrx(0);
	priv->daibt.occupied[substream->stream] = 1;
	return 0;
}

static int mt8521p_btmrg_hw_params(struct snd_pcm_substream *substream,
				  struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	struct mt_dai_private *priv;
	int stream_fs;
	int daibt_mode;

	pr_debug("%s() cpu_dai id %d\n", __func__, dai->id);
	stream_fs = params_rate(params);
	if ((stream_fs != 8000) && (stream_fs != 16000)) {
		pr_err("%s() btmgr not supprt this stream_fs %d\n", __func__, stream_fs);
		return -EINVAL;
	}
	priv = dev_get_drvdata(dai->dev);
	daibt_mode = (stream_fs / 8000) - 1;	/* 8K:0,16K:1 */

	if (!(priv->i2smrg.occupied[substream->stream]))
		afe_merge_i2s_set_mode(MRG_I2S_FS_32K);

	{
		struct afe_daibt_config config = {
			.daibt_c = 1,
			.daibt_ready = 1,
			.afe_daibt_input = DAIBT_INPUT_SEL_MERGERIF,
			.daibt_mode = daibt_mode
		};
		afe_daibt_configurate(&config);
		afe_daibt_set_enable(1);
	}

	afe_merge_i2s_enable(1);
	afe_merge_set_enable(1);

	return 0;
}

static void mt8521p_btmrg_shutdown(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct mt_dai_private *priv;

	priv = dev_get_drvdata(dai->dev);
	pr_debug("%s() cpu_dai id %d\n", __func__, dai->id);
	/* if the other direction stream is not occupied */
	if (!priv->daibt.occupied[!substream->stream]) {
		afe_daibt_set_enable(0);
		if (!priv->i2smrg.occupied[SNDRV_PCM_STREAM_PLAYBACK]
		 && !priv->i2smrg.occupied[SNDRV_PCM_STREAM_CAPTURE]) {
			afe_merge_set_enable(0);
			afe_merge_i2s_enable(0);
			mrg_power(0);
		}
	}

	priv->daibt.occupied[substream->stream] = 0;
}

static struct snd_soc_dai_ops mt8521p_btmrg_ops = {
	.startup = mt8521p_btmrg_startup,
	.shutdown = mt8521p_btmrg_shutdown,
	.hw_params = mt8521p_btmrg_hw_params,
};

static int mt8521p_i2smrg_startup(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct mt_dai_private *priv;
	struct mt_i2s_all *i2s_all;

	priv = dev_get_drvdata(dai->dev);

	i2s_all = &priv->i2s_all;
	pr_debug("%s() cpu_dai id %d\n", __func__, dai->id);
	if (i2s_all->i2s_out[AFE_I2S_OUT_6].occupied) {
		pr_err(
		       "%s() error: can't open MT8521p_DAI_MRGIF_I2S_ID because MT8521p_DAI_I2S6_ID has been occupied\n",
		       __func__);
		return -EINVAL;
	}
	if (priv->i2smrg.occupied) {
		pr_err(
		       "%s() error: can't open MT8521p_DAI_MRGIF_I2S_ID because it has been occupied\n",
		       __func__);
		return -EINVAL;
	}
	mrg_power(1);
	afe_merge_i2s_set_mode(MRG_I2S_FS_32K);
	priv->i2smrg.occupied[substream->stream] = 1;
	return 0;
}

static int mt8521p_i2smrg_hw_params(struct snd_pcm_substream *substream,
				   struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	struct mt_dai_private *priv;
	int stream_fs;
	enum afe_mrg_i2s_mode i2s_mode;
	enum afe_sampling_rate asmotiming_fs;

	pr_debug("%s() cpu_dai id %d\n", __func__, dai->id);
	stream_fs = params_rate(params);
	if ((stream_fs != 32000) && (stream_fs != 44100) && (stream_fs != 48000)) {
		pr_err("%s() btmgr not supprt this stream_fs %d\n", __func__, stream_fs);
		return -EINVAL;
	}
	priv = dev_get_drvdata(dai->dev);
	switch (stream_fs) {
	case 32000:
		i2s_mode = MRG_I2S_FS_32K;
		asmotiming_fs = FS_32000HZ;
		break;
	case 44100:
		i2s_mode = MRG_I2S_FS_44K;
		asmotiming_fs = FS_44100HZ;
		break;
	case 48000:
		i2s_mode = MRG_I2S_FS_48K;
		asmotiming_fs = FS_48000HZ;
		break;
	default:
		i2s_mode = MRG_I2S_FS_32K;
		asmotiming_fs = FS_32000HZ;
		break;
	}
	afe_merge_i2s_set_mode(i2s_mode);
	afe_merge_i2s_enable(1);
	afe_merge_set_enable(1);
	afe_asmo_timing_set(AFE_I2S_OUT_6, asmotiming_fs);
	afe_i2s_out_enable(AFE_I2S_OUT_6, 1);
	return 0;
}

static void mt8521p_i2smrg_shutdown(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct mt_dai_private *priv;

	pr_debug("%s() cpu_dai id %d\n", __func__, dai->id);
	priv = dev_get_drvdata(dai->dev);
	/* if the other direction stream is not occupied */
	if (!priv->i2smrg.occupied[!substream->stream]) {
		if (!priv->daibt.occupied[SNDRV_PCM_STREAM_PLAYBACK]
		 && !priv->daibt.occupied[SNDRV_PCM_STREAM_CAPTURE]) {
			afe_merge_set_enable(0);
			afe_merge_i2s_enable(0);
			mrg_power(0);
		}
	}
	priv->i2smrg.occupied[substream->stream] = 0;
}

static struct snd_soc_dai_ops mt8521p_i2smrg_ops = {
	.startup = mt8521p_i2smrg_startup,
	.hw_params = mt8521p_i2smrg_hw_params,
	.shutdown = mt8521p_i2smrg_shutdown,
};

