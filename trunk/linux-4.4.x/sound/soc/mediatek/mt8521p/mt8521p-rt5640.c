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

/* #define platform_clock_widget */

#ifdef platform_clock_widget
#include "mt8521p-afe-clk.h"
#endif

struct mt8521p_rt5640_private {
};

#ifdef platform_clock_widget
static int platform_clock_control(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *k, int  event)
{
	if (SND_SOC_DAPM_EVENT_ON(event)) {
		pr_debug("%s() ON\n", __func__);
		mt_turn_on_i2sout_clock(0, 1);
		mt_turn_on_i2sin_clock(0, 1);
		mt_i2s_power_on_mclk(0, 1);
	} else {
		pr_debug("%s() OFF\n", __func__);
		mt_i2s_power_on_mclk(0, 0);
		mt_turn_on_i2sin_clock(0, 0);
		mt_turn_on_i2sout_clock(0, 0);
	}

	return 0;
}
#endif

static const struct snd_soc_dapm_widget mt8521p_rt5640_widgets[] = {
	SND_SOC_DAPM_HP("Headphone", NULL),
	SND_SOC_DAPM_LINE("Line Out", NULL),

	SND_SOC_DAPM_MIC("Internal Mic", NULL),
	SND_SOC_DAPM_LINE("Line In", NULL),
	#ifdef platform_clock_widget
	SND_SOC_DAPM_SUPPLY("Platform Clock", SND_SOC_NOPM, 0, 0,
		platform_clock_control, SND_SOC_DAPM_PRE_PMU |
		SND_SOC_DAPM_POST_PMD),
	#endif
};

static const struct snd_soc_dapm_route mt8521p_rt5640_audio_map[] = {
	{"Headphone", NULL, "HPOL"},
	{"Headphone", NULL, "HPOR"},
	{"Line Out", NULL, "LOUTL"},
	{"Line Out", NULL, "LOUTR"},

	{"IN2P", NULL, "Line In"},
	{"IN2N", NULL, "Line In"},
	#ifdef platform_clock_widget
	{"Headphone", NULL, "Platform Clock"},
	{"Line Out", NULL, "Platform Clock"},
	{"Line In", NULL, "Platform Clock"},
	{"Internal Mic", NULL, "Platform Clock"},
	#endif
};

static const struct snd_soc_dapm_route mt8521p_rt5640_intmic_dmic1_map[] = {
	{"DMIC1", NULL, "Internal Mic"},
};

static const struct snd_soc_dapm_route mt8521p_rt5640_intmic_dmic2_map[] = {
	{"DMIC2", NULL, "Internal Mic"},
};

static const struct snd_soc_dapm_route mt8521p_rt5640_intmic_in1_map[] = {
	{"Internal Mic", NULL, "MICBIAS1"},
	{"IN1P", NULL, "Internal Mic"},
};


static const struct snd_kcontrol_new mt8521p_rt5640_controls[] = {
	SOC_DAPM_PIN_SWITCH("Headphone"),
	SOC_DAPM_PIN_SWITCH("Internal Mic"),
	SOC_DAPM_PIN_SWITCH("Line Out"),
	SOC_DAPM_PIN_SWITCH("Line In"),
};


static int pcm_master_data_rate_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;

	/* codec slave, mt8521p master */
	unsigned int fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_CBS_CFS | SND_SOC_DAIFMT_CONT;
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

static int mt8521p_rt5640_init(struct snd_soc_pcm_runtime *runtime)
{
	int ret;
	struct snd_soc_card *card = runtime->card;

	#ifdef CONFIG_MT8521p_RT5640_DMIC1
	ret = snd_soc_dapm_add_routes(&card->dapm, mt8521p_rt5640_intmic_dmic1_map,
					ARRAY_SIZE(mt8521p_rt5640_intmic_dmic1_map));
	#elif defined(CONFIG_MT8521p_RT5640_DMIC2)
	ret = snd_soc_dapm_add_routes(&card->dapm, mt8521p_rt5640_intmic_dmic2_map,
					ARRAY_SIZE(mt8521p_rt5640_intmic_dmic2_map));
	#else /* AMIC support */
	ret = snd_soc_dapm_add_routes(&card->dapm, mt8521p_rt5640_intmic_in1_map,
					ARRAY_SIZE(mt8521p_rt5640_intmic_in1_map));
	#endif

	return ret;
}

/* #include "mt8521p-machine-controls.c" */
static struct snd_soc_dai_link mt8521p_rt5640_dai_links[] = {
	{
		.name = "demo-pcm-out0",
		.stream_name = "pcm-out0",
		.platform_name = "mt8521p-audio",
		.cpu_dai_name = "mt8521p-i2s1",
		/*
		* .codec_dai_name = "pcm5102a-i2s",
		* .codec_name = "pcm5102a",
		*/
		.codec_dai_name = "rt5640-aif1",
		.codec_name = "rt5640.2-001c",
		.dai_fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_CBS_CFS | SND_SOC_DAIFMT_GATED,
		/*.ignore_pmdown_time  = 1,*/
		.init = mt8521p_rt5640_init,
		.ops = &stream_pcm_master_data_rate_ops
	},
	/*
	* {
	*	.name = "demo-hdmi-pcm-out",
	*	.stream_name = "hdmi-pcm-out",
	*	.platform_name = "mt8521p-audio",
	*	.cpu_dai_name = "mt8521p-hdmi-pcm-out",
	*	.codec_dai_name = "dummy-codec-i2s",
	*	.codec_name = "dummy-codec",
	* },
	*/
};

static struct snd_soc_card mt8521p_rt5640_soc_card = {
	.name = "mt8521p-rt5640-card",
	.dai_link = mt8521p_rt5640_dai_links,
	.num_links = ARRAY_SIZE(mt8521p_rt5640_dai_links),
	.controls = mt8521p_rt5640_controls,
	.num_controls = ARRAY_SIZE(mt8521p_rt5640_controls),
	.dapm_widgets = mt8521p_rt5640_widgets,
	.num_dapm_widgets = ARRAY_SIZE(mt8521p_rt5640_widgets),
	.dapm_routes = mt8521p_rt5640_audio_map,
	.num_dapm_routes = ARRAY_SIZE(mt8521p_rt5640_audio_map),
};

static int mt8521p_rt5640_machine_probe(struct platform_device *pdev)
{
	struct snd_soc_card *card = &mt8521p_rt5640_soc_card;
	struct device *dev = &pdev->dev;
	int ret;
	struct mt8521p_rt5640_private *priv =
		devm_kzalloc(&pdev->dev, sizeof(struct mt8521p_rt5640_private), GFP_KERNEL);

	pr_debug("%s()\n", __func__);


	if (priv == NULL)
		return -ENOMEM;
	#if 0
	ret = snd_soc_add_card_controls(card, mt8521p_rt5640_controls,
					ARRAY_SIZE(mt8521p_rt5640_controls));
	if (ret) {
		dev_err(card->dev, "unable to add card controls\n");
		return ret;
	}
	#endif

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

static int mt8521p_rt5640_machine_remove(struct platform_device *pdev)
{
	struct snd_soc_card *card = platform_get_drvdata(pdev);

	mt_afe_platform_deinit(&pdev->dev);
	devm_kfree(&pdev->dev, snd_soc_card_get_drvdata(card));
	return snd_soc_unregister_card(card);
}

#ifdef CONFIG_OF
static const struct of_device_id mt8521p_rt5640_machine_dt_match[] = {
	{.compatible = "mediatek,mt8521p-rt5640-machine",},
	{}
};
#endif

static struct platform_driver mt8521p_rt5640_machine = {
	.driver = {
		.name = "mt8521p-rt5640",
		.owner = THIS_MODULE,
		   #ifdef CONFIG_OF
		   .of_match_table = mt8521p_rt5640_machine_dt_match,
		   #endif
	},
	.probe = mt8521p_rt5640_machine_probe,
	.remove = mt8521p_rt5640_machine_remove
};

module_platform_driver(mt8521p_rt5640_machine);

/* Module information */
MODULE_DESCRIPTION("mt8521p rt5640 machine driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("mt8521p rt5640 soc card");
