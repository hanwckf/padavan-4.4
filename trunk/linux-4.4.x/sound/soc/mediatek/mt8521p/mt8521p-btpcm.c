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
/*#include <mach/mt_gpio.h>*/
#include "mt8521p-aud-global.h"
#ifdef CONFIG_MTK_LEGACY_CLOCK
#include <mach/mt_clkmgr.h>
#endif
#include "mt8521p-dai.h"
#include "mt8521p-afe.h"
#include "mt8521p-dai-private.h"
#include "mt8521p-aud-gpio.h"


static int mt8521p_btpcm_set_sysclk(struct snd_soc_dai *dai, int clk_id, unsigned int freq, int dir)
{
	struct mt_dai_private *priv;

	pr_debug("%s() cpu_dai id %d\n", __func__, dai->id);
	priv = dev_get_drvdata(dai->dev);
	priv->btpcm.fs = freq / 8000 - 1;
	return 0;
}

static int mt8521p_btpcm_set_clkdiv(struct snd_soc_dai *dai, int div_id, int div)
{
	struct mt_dai_private *priv;

	pr_debug("%s() cpu_dai id %d\n", __func__, dai->id);
	priv = dev_get_drvdata(dai->dev);
	if (div_id == DIV_ID_BCK_TO_LRCK)
		priv->btpcm.clkdiv = div;
	return 0;

}

static int mt8521p_btpcm_set_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	struct mt_dai_private *priv;

	pr_debug("%s() cpu_dai id %d\n", __func__, dai->id);
	priv = dev_get_drvdata(dai->dev);
	priv->btpcm.format = fmt;
	return 0;
}

static inline enum afe_pcm_format dai_fmt_to_btpcm_fmt(int fmt)
{
	switch (fmt) {
	case SND_SOC_DAIFMT_I2S:
		return PCM_FMT_I2S;
	case SND_SOC_DAIFMT_LEFT_J:
		return PCM_FMT_EIAJ;
	case SND_SOC_DAIFMT_DSP_A:
	default:
		return PCM_FMT_MODE_A;
	case SND_SOC_DAIFMT_DSP_B:
		return PCM_FMT_MODE_B;
	}
}

static int mt8521p_btpcm_hw_params(struct snd_pcm_substream *substream,
				  struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	struct mt_dai_private *priv;
	enum afe_pcm_mode modemaster;
	enum afe_pcm_format fmt;
	snd_pcm_format_t stream_fmt;
	int stream_fs;
	int slave;
	int wordlength;

	pr_debug("%s() cpu_dai id %d\n", __func__, dai->id);
	stream_fmt = params_format(params);
	stream_fs = params_rate(params);
	if ((stream_fs != 8000) && (stream_fs != 16000)) {
		pr_err("%s() btpcm not supprt this stream_fs %d\n", __func__, stream_fs);
		return -EINVAL;
	}

	priv = dev_get_drvdata(dai->dev);
	modemaster = (stream_fs / 8000) - 1;
	fmt = dai_fmt_to_btpcm_fmt(priv->btpcm.format & SND_SOC_DAIFMT_FORMAT_MASK);

	if ((priv->btpcm.format & SND_SOC_DAIFMT_MASTER_MASK) == SND_SOC_DAIFMT_CBM_CFM)
		slave = 1;
	else
		slave = 0;


	/* configure asrc */
	if (slave) {
		enum afe_pcm_mode mode = priv->btpcm.fs;

		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			afe_power_on_sample_asrc_tx(SAMPLE_ASRC_PCM_OUT, 1);
			{
				struct afe_sample_asrc_config asrc_config = {
					.o16bit = 0,
					.mono = 0,
					.input_fs = fs_enum(stream_fs),
					.output_fs = btpcm_fs_to_afe_fs(mode),
					.tracking = 1
				};
				afe_sample_asrc_tx_configurate(SAMPLE_ASRC_PCM_OUT, &asrc_config);
				afe_sample_asrc_tx_enable(SAMPLE_ASRC_PCM_OUT, 1);
			}
		} else {
			afe_power_on_sample_asrc_rx(SAMPLE_ASRC_PCM_IN, 1);
			{
				struct afe_sample_asrc_config asrc_config = {
					.o16bit = 0,
					.mono = 0,
					.input_fs = btpcm_fs_to_afe_fs(mode),
					.output_fs = fs_enum(stream_fs),
					.tracking = 1
				};
				afe_sample_asrc_rx_configurate(SAMPLE_ASRC_PCM_IN, &asrc_config);
				afe_sample_asrc_rx_enable(SAMPLE_ASRC_PCM_IN, 1);
			}
		}
	}

	if (priv->btpcm.clkdiv == 32)
		wordlength = 0;
	else
		wordlength = 1;


	{
		struct afe_btpcm_config config = {
			.mode = modemaster,
			.fmt = fmt,
			.slave = slave,
			.extloopback = 0,
			.wlen = wordlength
		};
		afe_btpcm_configurate(&config);
		afe_btpcm_enable(1);
	}

	return 0;
}

static int mt8521p_btpcm_startup(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct mt_dai_private *priv;

	pr_debug("%s() cpu_dai id %d\n", __func__, dai->id);
	priv = dev_get_drvdata(dai->dev);
	if (priv->btpcm.occupied[substream->stream]) {
		pr_err(
		       "%s() error: can't open MT8521p_DAI_BTPCM_ID(dir %d) because it has been occupied\n",
		       __func__, substream->stream);
		return -EINVAL;
	}
	if (priv->daibt.occupied[substream->stream]) {
		pr_err("%s() error: can't open MT8521p_DAI_BTPCM_ID(dir %d) because MT8521p_DAI_MRGIF_BT_ID(dir %d) has been occupied\n",
		       __func__, substream->stream, substream->stream);
		return -EINVAL;
	}
	#ifdef AUD_PINCTRL_SUPPORTING
	mt8521p_gpio_pcm_select(PCM_MODE1_PCM);
	#else
	mt_set_gpio_mode(GPIO18, GPIO_MODE_01);
	mt_set_gpio_mode(GPIO19, GPIO_MODE_01);
	mt_set_gpio_mode(GPIO20, GPIO_MODE_01);
	mt_set_gpio_mode(GPIO21, GPIO_MODE_01);
	#endif
	if (!priv->btpcm.occupied[0] && !priv->btpcm.occupied[1]) {
		#ifdef CONFIG_MTK_LEGACY_CLOCK
		enable_clock(MT_CG_AUDIO_AFE_PCMIF, "AUDIO"); /*AUDIO_TOP_CON4[24]*/
		enable_clock(MT_CG_AUDIO_ASRC11, "AUDIO"); /*AUDIO_TOP_CON4[16]*/
		enable_clock(MT_CG_AUDIO_ASRC12, "AUDIO"); /*AUDIO_TOP_CON4[17]*/
		#endif
		afe_power_on_btpcm(1);
	}
	substream->stream == SNDRV_PCM_STREAM_PLAYBACK
		? afe_o31_pcm_tx_sel_pcmtx(1)
		: afe_i26_pcm_rx_sel_pcmrx(1);
	priv->btpcm.occupied[substream->stream] = 1;
	return 0;
}

static void mt8521p_btpcm_shutdown(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct mt_dai_private *priv;

	priv = dev_get_drvdata(dai->dev);
	pr_debug("%s() cpu_dai id %d\n", __func__, dai->id);
	/* if the other direction stream is not occupied */
	if (!priv->btpcm.occupied[!substream->stream]) {
		afe_btpcm_enable(0);
		afe_power_on_btpcm(0);
		#ifdef CONFIG_MTK_LEGACY_CLOCK
		disable_clock(MT_CG_AUDIO_AFE_PCMIF, "AUDIO");
		disable_clock(MT_CG_AUDIO_ASRC11, "AUDIO");
		disable_clock(MT_CG_AUDIO_ASRC12, "AUDIO");
		#else
		afe_power_on_sample_asrc_tx(SAMPLE_ASRC_PCM_OUT, 0);
		afe_power_on_sample_asrc_rx(SAMPLE_ASRC_PCM_IN, 0);
		#endif
	}
	priv->btpcm.occupied[substream->stream] = 0;
}

static struct snd_soc_dai_ops mt8521p_btpcm_ops = {
	.set_sysclk = mt8521p_btpcm_set_sysclk,
	.set_clkdiv = mt8521p_btpcm_set_clkdiv,
	.set_fmt = mt8521p_btpcm_set_fmt,
	.startup = mt8521p_btpcm_startup,
	.shutdown = mt8521p_btpcm_shutdown,
	.hw_params = mt8521p_btpcm_hw_params,
};

