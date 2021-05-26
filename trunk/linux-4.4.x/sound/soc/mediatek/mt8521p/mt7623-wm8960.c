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

#include <linux/module.h>
#include <sound/soc.h>
#include "mt8521p-aud-gpio.h"
#include "mt8521p-afe-debug.h"
#include "mt8521p-afe.h"
#include "mt8521p-dai.h"
#include "wm8960.h"

struct mt7623_wm8960_private {
};

static int pcm_master_data_rate_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;

	/* codec slave, mt8521p master */
	unsigned int fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_CBS_CFS |
			SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CONT;
	unsigned int mclk_rate;
	unsigned int rate = params_rate(params);	/* data rate */
	unsigned int div_mclk_to_bck = rate > 192000 ? 2 : 4;
	unsigned int div_bck_to_lrck = 64;

	pr_debug("%s() rate = %d\n", __func__, rate);
	mclk_rate = rate * div_bck_to_lrck * div_mclk_to_bck;
	/* codec clkdiv */
	snd_soc_dai_set_clkdiv(codec_dai, WM8960_DACDIV, WM8960_DAC_DIV_1);
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

static int hdmi_tx_hw_params(struct snd_pcm_substream *substream,
			     struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	/* codec slave, mt8521p master */
	unsigned int fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_CBS_CFS |
			   SND_SOC_DAIFMT_CONT;
	/* codec slave */
	snd_soc_dai_set_fmt(codec_dai, fmt);
	return 0;
}

static struct snd_soc_ops stream_pcm_master_data_rate_ops = {
	.hw_params = pcm_master_data_rate_hw_params
};

static struct snd_soc_ops stream_hdmi_tx_master_ops = {
	.hw_params = hdmi_tx_hw_params
};

static struct snd_soc_dai_link mt7623_wm8960_dai_links[] = {
	{
		.name = "demo-hdmi-rawpcm-out",
		.stream_name = "hdmi-rawpcm-out",
		.platform_name = "mt8521p-audio",
		.cpu_dai_name = "mt8521p-hdmi-rawpcm-out",
		.codec_dai_name = "spdif-hifi",
		.codec_name = "hdmi-audio-codec",
		.ops = &stream_hdmi_tx_master_ops
	},
	{
		.name = "demo-audio-tx-rx",
		.stream_name = "audio-tx-rx",
		.platform_name = "mt8521p-audio",
		.cpu_dai_name = "mt8521p-i2s1",
		.codec_dai_name = "wm8960-hifi",
		.codec_name = "wm8960.2-001a",
		.dai_fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_CBS_CFS | SND_SOC_DAIFMT_GATED,
		.ops = &stream_pcm_master_data_rate_ops
	},
};

static struct snd_soc_card mt7623_wm8960_card = {
	.name = "mt7623-wm8960-card",
	.dai_link = mt7623_wm8960_dai_links,
	.num_links = ARRAY_SIZE(mt7623_wm8960_dai_links),
};

static int mt7623_wm8960_probe(struct platform_device *pdev)
{
	struct snd_soc_card *card = &mt7623_wm8960_card;
	struct device *dev = &pdev->dev;
	int ret;
	struct mt7623_wm8960_private *priv =
		devm_kzalloc(&pdev->dev, sizeof(struct mt7623_wm8960_private), GFP_KERNEL);

	pr_debug("%s()\n", __func__);
	if (priv == NULL)
		return -ENOMEM;

	ret = mt_afe_platform_init(dev);
	if (ret) {
		pr_err("%s mt_afe_platform_init fail %d\n", __func__, ret);
		return ret;
	}
	mt_afe_debug_init();

	#ifdef AUD_PINCTRL_SUPPORTING
	mt8521p_gpio_probe(dev);
	#endif

	card->dev = &pdev->dev;
	snd_soc_card_set_drvdata(card, priv);
	return snd_soc_register_card(card);
}

static int mt7623_wm8960_remove(struct platform_device *pdev)
{
	struct snd_soc_card *card = platform_get_drvdata(pdev);

	mt_afe_platform_deinit(&pdev->dev);
	devm_kfree(&pdev->dev, snd_soc_card_get_drvdata(card));
	return snd_soc_unregister_card(card);
}

#ifdef CONFIG_OF
static const struct of_device_id mt7623_wm8960_dt_match[] = {
	{.compatible = "mediatek,mt7623-wm8960-machine",},
	{}
};
#endif

static struct platform_driver mt7623_wm8960 = {
	.driver = {
		.name = "mt7623-wm8960-machine",
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = mt7623_wm8960_dt_match,
#endif
	},
	.probe = mt7623_wm8960_probe,
	.remove = mt7623_wm8960_remove
};

module_platform_driver(mt7623_wm8960);

/* Module information */
MODULE_DESCRIPTION("mt7623-wm8960 asoc driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("mt7623-wm8960 asoc card");
