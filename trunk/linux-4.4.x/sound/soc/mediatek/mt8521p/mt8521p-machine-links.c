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

/*#include <mach/upmu_hw.h>*/
/*#include <mach/mt_gpio.h>*/
#include <linux/delay.h>
#include "mt8521p-dai.h"
#include "mt8521p-private.h"
#include "mt8521p-aud-gpio.h"
#include "mt8521p-aud-global.h"

static int pcm_master_data_rate_hw_params(struct snd_pcm_substream *substream,
					  struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;

	/* codec slave, mt8521p master */
	unsigned int fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_CBS_CFS |
			   SND_SOC_DAIFMT_CONT;
	unsigned int mclk_rate;
	unsigned int rate = params_rate(params);	/* data rate */
	unsigned int div_mclk_to_bck = rate > 192000 ? 2 : 4;
	unsigned int div_bck_to_lrck = 64;

	pr_debug("%s() rate = %d\n", __func__, rate);
	mclk_rate = rate * div_bck_to_lrck * div_mclk_to_bck;
	/* codec mclk */
	snd_soc_dai_set_sysclk(codec_dai, 0, mclk_rate, SND_SOC_CLOCK_IN);
	/* codec slave */
	snd_soc_dai_set_fmt(codec_dai, fmt);
	/* mt8521p mclk */
	snd_soc_dai_set_sysclk(cpu_dai, 0, mclk_rate, SND_SOC_CLOCK_OUT);
	/* mt8521p bck */
	snd_soc_dai_set_clkdiv(cpu_dai, DIV_ID_MCLK_TO_BCK, div_mclk_to_bck);
	/* mt8521p lrck */
	snd_soc_dai_set_clkdiv(cpu_dai, DIV_ID_BCK_TO_LRCK, div_bck_to_lrck);
	/* mt8521p master */
	snd_soc_dai_set_fmt(cpu_dai, fmt);
	return 0;
}

static struct snd_soc_ops stream_pcm_master_data_rate_ops = {
	.hw_params = pcm_master_data_rate_hw_params
};

#ifdef CONFIG_SND_SOC_MT8521P_EVB
/* Digital audio interface glue - connects codec <---> CPU */
static struct snd_soc_dai_link demo_dai_links[] = {
	{
		.name = "demo-pcm-out0",
		.stream_name = "pcm-out0",
		.platform_name = "mt8521p-audio",
		.cpu_dai_name = "mt8521p-i2s1",
		.codec_dai_name = "pcm5102a-i2s",
		.codec_name = "pcm5102a",
		.dai_fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_CBS_CFS |
			   SND_SOC_DAIFMT_GATED,
		.ops = &stream_pcm_master_data_rate_ops
	},
	{
		.name = "demo-pcm-in1",
		.stream_name = "pcm-in1",
		.platform_name = "mt8521p-audio",
		.cpu_dai_name = "mt8521p-i2s1",
		.codec_dai_name = "pcm1861-i2s",
		.codec_name = "pcm1861",
		.dai_fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_CBS_CFS |
			   SND_SOC_DAIFMT_GATED,
		.ops = &stream_pcm_master_data_rate_ops
	},
	{
		.name = "demo-pcm-in1-ck1ck2",
		.stream_name = "pcm-in1-ck1ck2",
		.platform_name = "mt8521p-audio",
		.cpu_dai_name = "mt8521p-i2s1-ck1ck2",
		.codec_dai_name = "pcm1861-i2s",
		.codec_name = "pcm1861",
		.dai_fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_CBS_CFS |
			   SND_SOC_DAIFMT_GATED,
		.ops = &stream_pcm_master_data_rate_ops
	},
	{
		.name = "demo-pcm-in2-ck1ck2",
		.stream_name = "pcm-in2-ck1ck2",
		.platform_name = "mt8521p-audio",
		.cpu_dai_name = "mt8521p-i2s2-ck1ck2",
		.codec_dai_name = "pcm1861-i2s",
		.codec_name = "pcm1861",
		.dai_fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_CBS_CFS |
			   SND_SOC_DAIFMT_GATED,
		.ops = &stream_pcm_master_data_rate_ops
	},
	{
		.name = "demo-pcm-inmx",
		.stream_name = "pcm-inmx",
		.platform_name = "mt8521p-audio",
		.cpu_dai_name = "mt8521p-i2smx",
		.codec_dai_name = "dummy-codec-i2s",
		.codec_name = "dummy-codec",
		.ops = &stream_pcm_master_data_rate_ops
	},
	{
		.name = "demo-hdmi-pcm-out",
		.stream_name = "hdmi-pcm-out",
		.platform_name = "mt8521p-audio",
		.cpu_dai_name = "mt8521p-hdmi-pcm-out",
		/* dummy codec is temporary, please change to real codec */
		.codec_dai_name = "dummy-codec-i2s",
		.codec_name = "dummy-codec",
	},
	{
		.name = "demo-multi-in",
		.stream_name = "multi-in",
		.platform_name = "mt8521p-audio",
		.cpu_dai_name = "mt8521p-multi-in",
		/* dummy codec is temporary, please change to real codec */
		.codec_dai_name = "dummy-codec-i2s",
		.codec_name = "dummy-codec",
	},
	{
		.name = "demo-pcm-out-multich",
		.stream_name = "pcm-out-multich",
		.platform_name = "mt8521p-audio",
		.cpu_dai_name = "mt8521p-i2sm",
		.codec_dai_name = "dummy-codec-i2s",
		.codec_name = "dummy-codec",
		.ops = &stream_pcm_master_data_rate_ops
	}
};
#endif
