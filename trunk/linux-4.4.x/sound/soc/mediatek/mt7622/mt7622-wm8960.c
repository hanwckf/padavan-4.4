/*
 * mt7622-wm8960.c  --  MT7622 WM8960 ALSA SoC machine driver
 *
 * Copyright (c) 2017 MediaTek Inc.
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

#include <linux/module.h>
#include <sound/soc.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/of_gpio.h>

#include "mt7622-afe-common.h"

static const struct snd_soc_dapm_widget mt7622_wm8960_widgets[] = {
	SND_SOC_DAPM_HP("Headphone", NULL),
	SND_SOC_DAPM_MIC("AMIC", NULL),
};

static const struct snd_kcontrol_new mt7622_wm8960_controls[] = {
	SOC_DAPM_PIN_SWITCH("Headphone"),
	SOC_DAPM_PIN_SWITCH("AMIC"),
};

static int mt7622_wm8960_be_ops_hw_params(struct snd_pcm_substream *substream,
				   struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	unsigned int mclk_rate;
	unsigned int rate = params_rate(params);
	unsigned int div_mclk_over_bck = rate > 192000 ? 2 : 4;
	unsigned int div_bck_over_lrck = 64;

	mclk_rate = rate * div_bck_over_lrck * div_mclk_over_bck;

	/* mt7622 mclk */
	snd_soc_dai_set_sysclk(cpu_dai, 0, mclk_rate, SND_SOC_CLOCK_OUT);

	/* codec mclk */
	snd_soc_dai_set_sysclk(codec_dai, 0, mclk_rate, SND_SOC_CLOCK_IN);
	return 0;
}

static struct snd_soc_ops mt7622_wm8960_be_ops = {
	.hw_params = mt7622_wm8960_be_ops_hw_params
};

static struct snd_soc_dai_link mt7622_wm8960_dai_links[] = {
	/* FE <--> Front End DAI links*/
	{
		.name = "mt7622-wm8960-playback",
		.stream_name = "mt7622-wm8960-playback",
		.cpu_dai_name = "PCMO0",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.trigger = {
			SND_SOC_DPCM_TRIGGER_POST,
			SND_SOC_DPCM_TRIGGER_POST
		},
		.dynamic = 1,
		.dpcm_playback = 1,
	},
	{
		.name = "mt7622-wm8960-capture",
		.stream_name = "mt7622-wm8960-capture",
		.cpu_dai_name = "PCM0",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.trigger = {
			SND_SOC_DPCM_TRIGGER_POST,
			SND_SOC_DPCM_TRIGGER_POST
		},
		.dynamic = 1,
		.dpcm_capture = 1,
	},
	/* BE <--> Back End DAI links*/
	{
		.name = "wm8960-codec",
		.cpu_dai_name = "I2S0",
		.no_pcm = 1,
		.codec_dai_name = "wm8960-hifi",
		.dai_fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_CBS_CFS
		| SND_SOC_DAIFMT_GATED,
		.ops = &mt7622_wm8960_be_ops,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
	},
};

static struct snd_soc_card mt7622_wm8960_soc_card = {
	.name = "mt7622-wm8960",
	.owner = THIS_MODULE,
	.dai_link = mt7622_wm8960_dai_links,
	.num_links = ARRAY_SIZE(mt7622_wm8960_dai_links),
	.controls = mt7622_wm8960_controls,
	.num_controls = ARRAY_SIZE(mt7622_wm8960_controls),
	.dapm_widgets = mt7622_wm8960_widgets,
	.num_dapm_widgets = ARRAY_SIZE(mt7622_wm8960_widgets),
};

static int mt7622_wm8960_machine_probe(struct platform_device *pdev)
{
	struct snd_soc_card *card = &mt7622_wm8960_soc_card;
	int ret;
	int i;
	struct device_node *platform_node, *codec_node;
	struct device *dev = &pdev->dev;

	platform_node = of_parse_phandle(pdev->dev.of_node, "mediatek,platform", 0);
	if (!platform_node) {
		dev_err(&pdev->dev, "Property 'platform' missing or invalid\n");
		return -EINVAL;
	}
	for (i = 0; i < card->num_links; i++) {
		if (mt7622_wm8960_dai_links[i].platform_name)
			continue;
		mt7622_wm8960_dai_links[i].platform_of_node = platform_node;
	}

	card->dev = dev;

	codec_node = of_parse_phandle(pdev->dev.of_node, "mediatek,audio-codec", 0);
	if (!codec_node) {
		dev_err(&pdev->dev, "Property 'audio-codec' missing or invalid\n");
		return -EINVAL;
	}
	for (i = 0; i < card->num_links; i++) {
		if (mt7622_wm8960_dai_links[i].codec_name)
			continue;
		mt7622_wm8960_dai_links[i].codec_of_node = codec_node;
	}

	ret = snd_soc_of_parse_audio_routing(card, "audio-routing");
	if (ret) {
		dev_err(&pdev->dev, "failed to parse audio-routing: %d\n", ret);
		return ret;
	}

	ret = devm_snd_soc_register_card(&pdev->dev, card);

	if (ret)
		dev_err(&pdev->dev, "%s snd_soc_register_card fail %d\n", __func__, ret);

	return ret;
}

#ifdef CONFIG_OF
static const struct of_device_id mt7622_wm8960_machine_dt_match[] = {
	{.compatible = "mediatek,mt7622-wm8960-machine",},
	{}
};
#endif

static struct platform_driver mt7622_wm8960_machine_driver = {
	.driver = {
		.name = "mt7622-wm8960-machine",
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = mt7622_wm8960_machine_dt_match,
#endif
	},
	.probe = mt7622_wm8960_machine_probe,
};

module_platform_driver(mt7622_wm8960_machine_driver);

/* Module information */
MODULE_DESCRIPTION("MT7622 WM8960 ALSA SoC machine driver");
MODULE_AUTHOR("Unknown");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("mt7622 wm8960 soc card");
