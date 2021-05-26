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
#include <linux/of_gpio.h>

#include "mt8521p-aud-gpio.h"
#include "mt8521p-afe-debug.h"
#include "mt8521p-afe.h"
#include "mt8521p-dai.h"

struct mt8521p_cs42448_private {
	int i2s1_in_mux;
	int i2s1_in_mux_gpio_sel_1;
	int i2s1_in_mux_gpio_sel_2;

};

static const char * const i2sin_mux_switch_text[] = {"ADC_SDOUT2", "ADC_SDOUT3",
						     "I2S_IN_1", "I2S_IN_2"};

static const struct soc_enum i2sin_mux_enum = SOC_ENUM_SINGLE_EXT(4,
							 i2sin_mux_switch_text);

static int i2sin1_mux_get(struct snd_kcontrol *kcontrol,
			  struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_card *card = snd_kcontrol_chip(kcontrol);
	struct mt8521p_cs42448_private *priv = snd_soc_card_get_drvdata(card);

	ucontrol->value.integer.value[0] = priv->i2s1_in_mux;
	pr_debug("%s i2s1_in_mux = %d\n", __func__, priv->i2s1_in_mux);
	return 0;
}

static int i2sin1_mux_set(struct snd_kcontrol *kcontrol,
			  struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_card *card = snd_kcontrol_chip(kcontrol);
	struct mt8521p_cs42448_private *priv = snd_soc_card_get_drvdata(card);

	if (ucontrol->value.integer.value[0] == priv->i2s1_in_mux)
		return 0;

	switch (ucontrol->value.integer.value[0]) {
	case 0:
		gpio_set_value(priv->i2s1_in_mux_gpio_sel_1, 0);
		gpio_set_value(priv->i2s1_in_mux_gpio_sel_2, 0);
		break;
	case 1:
		gpio_set_value(priv->i2s1_in_mux_gpio_sel_1, 1);
		gpio_set_value(priv->i2s1_in_mux_gpio_sel_2, 0);
		break;
	case 2:
		gpio_set_value(priv->i2s1_in_mux_gpio_sel_1, 0);
		gpio_set_value(priv->i2s1_in_mux_gpio_sel_2, 1);
		break;
	case 3:
		gpio_set_value(priv->i2s1_in_mux_gpio_sel_1, 1);
		gpio_set_value(priv->i2s1_in_mux_gpio_sel_2, 1);
		break;
	default:
		pr_err("%s invalid setting\n", __func__);
	}

	priv->i2s1_in_mux = ucontrol->value.integer.value[0];
	pr_debug("%s i2sin_mux = %d\n", __func__, priv->i2s1_in_mux);
	return 0;
}

static const struct snd_soc_dapm_widget mt8521p_asoc_card_dapm_widgets[] = {
	SND_SOC_DAPM_LINE("Line Out Jack", NULL),
	SND_SOC_DAPM_MIC("AMIC", NULL),
	SND_SOC_DAPM_LINE("Tuner In", NULL),
	SND_SOC_DAPM_LINE("Satellite Tuner In", NULL),
	SND_SOC_DAPM_LINE("AUX In", NULL),
};

static const struct snd_kcontrol_new mt8521p_cs42448_controls[] = {
	SOC_DAPM_PIN_SWITCH("Line Out Jack"),
	SOC_DAPM_PIN_SWITCH("AMIC"),
	SOC_DAPM_PIN_SWITCH("Tuner In"),
	SOC_DAPM_PIN_SWITCH("Satellite Tuner In"),
	SOC_DAPM_PIN_SWITCH("AUX In"),
	SOC_ENUM_EXT("I2SIN1_MUX_Switch", i2sin_mux_enum, i2sin1_mux_get,
		     i2sin1_mux_set),
};

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
	unsigned int div_mclk_to_bck = rate >= 192000 ? 2 : 4;
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

static struct snd_soc_ops stream_i2s_master_ops = {
	.hw_params = pcm_master_data_rate_hw_params
};

static struct snd_soc_ops stream_hdmi_tx_master_ops = {
	.hw_params = hdmi_tx_hw_params
};

static struct snd_soc_dai_link mt8521p_cs42448_dai_links[] = {
	{
		.name = "cs42448-pcm-out-multich",
		.stream_name = "pcm-out-multich",
		.platform_name = "mt8521p-audio",
		.cpu_dai_name = "mt8521p-i2sm",
		.codec_dai_name = "cs42448",
		.ops = &stream_i2s_master_ops
	},
	{
		/* 8521p multi-ch in*/
		.name = "cs42448-pcm-inmx",
		.stream_name = "pcm-inmx",
		.platform_name = "mt8521p-audio",
		.cpu_dai_name = "mt8521p-i2smx",
		.codec_dai_name = "cs42448",
		.dai_fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_CBS_CFS |
			   SND_SOC_DAIFMT_GATED,
		.ops = &stream_i2s_master_ops,
		.capture_only = true
	},
	{
		.name = "spdif-in",
		.stream_name = "multi-in",
		.platform_name = "mt8521p-audio",
		.cpu_dai_name = "mt8521p-multi-in",
		/* dummy codec is temporary, please change to real codec */
		.codec_dai_name = "dummy-codec-i2s",
		.codec_name = "dummy-codec"
	},
	{
		/* 8521p multi-ch in*/
		.name = "demo-pcm-inmx",
		.stream_name = "pcm-inmx",
		.platform_name = "mt8521p-audio",
		.cpu_dai_name = "mt8521p-i2smx",
		.codec_dai_name = "dummy-codec-i2s",
		.codec_name = "dummy-codec",
		.dai_fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_CBS_CFS |
			   SND_SOC_DAIFMT_GATED,
		.ops = &stream_i2s_master_ops,
		.capture_only = true
	},
	{
		.name = "demo-hdmi-8ch-i2s-out",
		.stream_name = "hdmi-pcm-out",
		.platform_name = "mt8521p-audio",
		.cpu_dai_name = "mt8521p-hdmi-pcm-out",
			.codec_dai_name = "i2s-hifi",
			.codec_name = "hdmi-audio-codec",
		.ops = &stream_hdmi_tx_master_ops
	},
	{
		.name = "demo-hdmi-rawpcm-out",
		.stream_name = "hdmi-rawpcm-out",
		.platform_name = "mt8521p-audio",
		.cpu_dai_name = "mt8521p-hdmi-rawpcm-out",
		.codec_dai_name = "spdif-hifi",
		.codec_name = "hdmi-audio-codec",
		.ops = &stream_hdmi_tx_master_ops
	},
};

static struct snd_soc_card mt8521p_cs42448_soc_card = {
	.name = "mt8521p-cs42448",
	.dai_link = mt8521p_cs42448_dai_links,
	.num_links = ARRAY_SIZE(mt8521p_cs42448_dai_links),
	.controls = mt8521p_cs42448_controls,
	.num_controls = ARRAY_SIZE(mt8521p_cs42448_controls),
	.dapm_widgets = mt8521p_asoc_card_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(mt8521p_asoc_card_dapm_widgets),
};

static int mt8521p_cs42448_machine_probe(struct platform_device *pdev)
{
	struct snd_soc_card *card = &mt8521p_cs42448_soc_card;
	struct device *dev = &pdev->dev;
	int ret, i;
	struct device_node *codec_node;
	struct mt8521p_cs42448_private *priv =
		devm_kzalloc(&pdev->dev, sizeof(struct mt8521p_cs42448_private), GFP_KERNEL);

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
	codec_node = of_parse_phandle(pdev->dev.of_node,
					  "mediatek,audio-codec", 0);
	if (!codec_node) {
		dev_err(&pdev->dev,
			"Property 'audio-codec' missing or invalid\n");
		return -EINVAL;
	}
	for (i = 0; i < card->num_links; i++) {
		if (mt8521p_cs42448_dai_links[i].codec_name)
			continue;
		mt8521p_cs42448_dai_links[i].codec_of_node = codec_node;
	}

	card->dev = &pdev->dev;

	ret = snd_soc_of_parse_audio_routing(card, "audio-routing");
	if (ret) {
		dev_err(&pdev->dev, "failed to parse audio-routing: %d\n", ret);
		return ret;
	}

	priv->i2s1_in_mux_gpio_sel_1 = of_get_named_gpio(dev->of_node, "i2s1-in-sel-gpio1", 0);
	if (gpio_is_valid(priv->i2s1_in_mux_gpio_sel_1)) {
		ret = devm_gpio_request(dev, priv->i2s1_in_mux_gpio_sel_1,
					"i2s1_in_mux_gpio_sel_1");
		if (ret)
			pr_debug("%s devm_gpio_request fail %d\n", __func__, ret);

		gpio_direction_output(priv->i2s1_in_mux_gpio_sel_1, 0);

	}

	priv->i2s1_in_mux_gpio_sel_2 = of_get_named_gpio(dev->of_node, "i2s1-in-sel-gpio2", 0);
	if (gpio_is_valid(priv->i2s1_in_mux_gpio_sel_2)) {
		ret = devm_gpio_request(dev, priv->i2s1_in_mux_gpio_sel_2,
					"i2s1_in_mux_gpio_sel_2");
		if (ret)
			pr_debug("%s devm_gpio_request fail2 %d\n", __func__, ret);

		gpio_direction_output(priv->i2s1_in_mux_gpio_sel_2, 0);

	}
	snd_soc_card_set_drvdata(card, priv);
	return snd_soc_register_card(card);
}


static int mt8521p_cs42448_machine_remove(struct platform_device *pdev)
{
	struct snd_soc_card *card = platform_get_drvdata(pdev);

	mt_afe_platform_deinit(&pdev->dev);
	devm_kfree(&pdev->dev, snd_soc_card_get_drvdata(card));
	return snd_soc_unregister_card(card);
}

#ifdef CONFIG_OF
static const struct of_device_id mt8521p_cs42448_machine_dt_match[] = {
	{.compatible = "mediatek,mt8521p-cs42448-machine",},
	{}
};
#endif

static struct platform_driver mt8521p_cs42448_machine = {
	.driver = {
		.name = "mt8521p-cs42448",
		.owner = THIS_MODULE,
		   #ifdef CONFIG_OF
		   .of_match_table = mt8521p_cs42448_machine_dt_match,
		   #endif
	},
	.probe = mt8521p_cs42448_machine_probe,
	.remove = mt8521p_cs42448_machine_remove
};

module_platform_driver(mt8521p_cs42448_machine);

/* Module information */
MODULE_DESCRIPTION("mt8521p cs42448 machine driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("mt8521p cs42448 soc card");
