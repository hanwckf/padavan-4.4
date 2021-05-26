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
#include <linux/debugfs.h>
#include "mt2712-codec.h"
#include <linux/mfd/syscon.h>	/* Add for APMIXED reg map */
#include <linux/of_platform.h>

#define DEBUG_AADC_SGEN 0

static const struct snd_soc_dapm_widget mt2712_codec_widgets[] = {
	 SND_SOC_DAPM_INPUT("RX"),
};

static const struct snd_soc_dapm_route mt2712_codec_routes[] = {
	 { "mt2712-codec-aadc-capture", NULL, "RX" },
};

static int mt2712_aadc_hw_params(struct snd_pcm_substream *substream,
				  struct snd_pcm_hw_params *params,
				  struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mt2712_codec_priv *codec_data = snd_soc_codec_get_drvdata(rtd->codec);
	int rate = params_rate(params);

	dev_dbg(rtd->codec->dev, "%s()\n", __func__);

	switch (rate) {
	case 8000:
		regmap_update_bits(codec_data->regmap, ABB_AFE_CON11,
			   AFIFO_RATE, AFIFO_RATE_SET(0));
		regmap_update_bits(codec_data->regmap, ABB_AFE_CON1,
			   ABB_UL_RATE, ABB_UL_RATE_SET(0));
		regmap_update_bits(codec_data->regmap, AFE_ADDA_UL_SRC_CON0,
			   ULSRC_VOICE_MODE, ULSRC_VOICE_MODE_SET(0));
		break;
	case 16000:
		regmap_update_bits(codec_data->regmap, ABB_AFE_CON11,
			   AFIFO_RATE, AFIFO_RATE_SET(2));
		regmap_update_bits(codec_data->regmap, ABB_AFE_CON1,
			   ABB_UL_RATE, ABB_UL_RATE_SET(0));
		regmap_update_bits(codec_data->regmap, AFE_ADDA_UL_SRC_CON0,
			   ULSRC_VOICE_MODE, ULSRC_VOICE_MODE_SET(1));
		break;
	case 32000:
		regmap_update_bits(codec_data->regmap, ABB_AFE_CON11,
			   AFIFO_RATE, AFIFO_RATE_SET(4));
		regmap_update_bits(codec_data->regmap, ABB_AFE_CON1,
			   ABB_UL_RATE, ABB_UL_RATE_SET(0));
		regmap_update_bits(codec_data->regmap, AFE_ADDA_UL_SRC_CON0,
			   ULSRC_VOICE_MODE, ULSRC_VOICE_MODE_SET(2));
		break;
	case 48000:
		regmap_update_bits(codec_data->regmap, ABB_AFE_CON11,
			   AFIFO_RATE, AFIFO_RATE_SET(5));
		regmap_update_bits(codec_data->regmap, ABB_AFE_CON1,
			   ABB_UL_RATE, ABB_UL_RATE_SET(1));
		regmap_update_bits(codec_data->regmap, AFE_ADDA_UL_SRC_CON0,
			   ULSRC_VOICE_MODE, ULSRC_VOICE_MODE_SET(3));
		break;
	}
	regmap_update_bits(codec_data->regmap, ABB_AFE_CON11,
		 AFIFO_SRPT, AFIFO_SRPT_SET(3));
	regmap_update_bits(codec_data->regmap, ABB_AFE_CON0,
		 ABB_PDN_I2SO1, ABB_PDN_I2SO1_SET(0));
	regmap_update_bits(codec_data->regmap, ABB_AFE_CON0,
		 ABB_PDN_I2SI1, ABB_PDN_I2SI1_SET(0));
	regmap_update_bits(codec_data->regmap, ABB_AFE_CON0,
		 ABB_UL_EN, ABB_UL_EN_SET(1));
	regmap_update_bits(codec_data->regmap, ABB_AFE_CON0,
		 ABB_AFE_EN, ABB_AFE_EN_SET(1));
	regmap_update_bits(codec_data->regmap, AFE_ADDA_UL_SRC_CON0,
		 ULSRC_ON, ULSRC_ON_SET(1));

#if DEBUG_AADC_SGEN
	/* Sgen debug setting */
	regmap_update_bits(codec_data->regmap, AFE_ADDA_UL_SRC_CON1,
		UL_SRC_CH1_AMP_MASK, 6 << UL_SRC_CH1_AMP_POS);
	regmap_update_bits(codec_data->regmap, AFE_ADDA_UL_SRC_CON1,
		UL_SRC_CH1_FREQ_MASK, 2 << UL_SRC_CH1_FREQ_POS);
	regmap_update_bits(codec_data->regmap, AFE_ADDA_UL_SRC_CON1,
		UL_SRC_CH2_AMP_MASK, 6 << UL_SRC_CH2_AMP_POS);
	regmap_update_bits(codec_data->regmap, AFE_ADDA_UL_SRC_CON1,
		UL_SRC_CH2_FREQ_MASK, 1 << UL_SRC_CH2_FREQ_POS);
	/* Turn on sgen */
	regmap_update_bits(codec_data->regmap, AFE_ADDA_UL_SRC_CON1,
		UL_SRC_MUTE_MASK, 0 << UL_SRC_MUTE_POS);
	regmap_update_bits(codec_data->regmap, AFE_ADDA_UL_SRC_CON1,
		UL_SRC_SGEN_EN_MASK, 1 << UL_SRC_SGEN_EN_POS);

	dev_warn(rtd->codec->dev, "%s: AADC data from sgen now.\n", __func__);
#else
	/* Enable CIC filter for analog src */
	regmap_update_bits(codec_data->regmap, AFE_ADDA_UL_DL_CON0,
		ADDA_adda_afe_on_MASK, 1 << ADDA_adda_afe_on_POS);

	/* Analog part setting */
	regmap_update_bits(codec_data->regmap_modules[REGMAP_APMIXEDSYS],
		AADC_CON3, 0xFFFFFFFF, 0x40024001);
	regmap_update_bits(codec_data->regmap_modules[REGMAP_APMIXEDSYS],
		AADC_CON0, 0xFFFFFFFF, 0x03F9809E);
#endif


	return 0;
}

static void mt2712_aadc_shutdown(struct snd_pcm_substream *substream,
				  struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mt2712_codec_priv *codec_data = snd_soc_codec_get_drvdata(rtd->codec);

	regmap_update_bits(codec_data->regmap, AFE_ADDA_UL_SRC_CON0,
		 ULSRC_ON, ULSRC_ON_SET(0));
	regmap_update_bits(codec_data->regmap, ABB_AFE_CON0,
		 ABB_UL_EN, ABB_UL_EN_SET(0));
}


static struct snd_soc_dai_ops mt2712_aadc_dai_ops = {
	.hw_params = mt2712_aadc_hw_params,
	.shutdown = mt2712_aadc_shutdown,
};


static struct snd_soc_dai_driver mt2712_codec_dai_driver[] = {
	{
	.name = "mt2712-codec-aadc",
	.capture = {
		    .stream_name = "mt2712-codec-aadc-capture",
		    .channels_min = 1,
		    .channels_max = 10,
		    .rates = SNDRV_PCM_RATE_8000_192000,
		    .formats = (SNDRV_PCM_FMTBIT_S16_LE	|
				SNDRV_PCM_FMTBIT_S32_LE |
				SNDRV_PCM_FMTBIT_S24_LE	|
				SNDRV_PCM_FMTBIT_S24_3LE |
				SNDRV_PCM_FMTBIT_DSD_U8 |
				SNDRV_PCM_FMTBIT_DSD_U16_LE),
		    },
	.ops = &mt2712_aadc_dai_ops,
	},
};

static const struct regmap_config mt2712_codec_regmap_config = {
	.reg_bits = 32,
	.reg_stride = 4,
	.val_bits = 32,
	.max_register = ADDA_END_ADDR,
	.cache_type = REGCACHE_NONE,
};

static struct regmap *mt2712_codec_get_regmap_from_dt(const char *phandle_name,
		struct mt2712_codec_priv *codec_data)
{
	struct device_node *self_node = NULL, *node = NULL;
	struct platform_device *platdev = NULL;
	struct device *dev = codec_data->codec->dev;
	struct regmap *regmap = NULL;

	self_node = of_find_compatible_node(NULL, NULL,
		"mediatek,mt2712-codec");
	if (!self_node) {
		dev_err(dev, "%s failed to find %s node\n",
			__func__, "mt2712-codec");
		return NULL;
	}
	dev_dbg(dev, "%s found %s node\n", __func__, "mt2712-codec");
		node = of_parse_phandle(self_node, phandle_name, 0);
	if (!node) {
		dev_err(dev, "%s failed to find %s node\n",
			__func__, phandle_name);
		return NULL;
	}
	dev_dbg(dev, "%s found %s\n", __func__, phandle_name);

	platdev = of_find_device_by_node(node);
	if (!platdev) {
		dev_err(dev, "%s failed to get platform device of %s\n",
			__func__, phandle_name);
		return NULL;
	}
	dev_dbg(dev, "%s found platform device of %s\n",
		__func__, phandle_name);

	regmap = dev_get_regmap(&platdev->dev, NULL);
	if (regmap) {
		dev_dbg(dev, "%s found regmap of %s\n", __func__, phandle_name);
		return regmap;
	}

	regmap = syscon_regmap_lookup_by_phandle(dev->of_node, phandle_name);
	if (!IS_ERR(regmap)) {
		dev_dbg(dev, "%s found regmap of syscon node %s\n",
			__func__, phandle_name);
		return regmap;
	}
	dev_err(dev, "%s failed to get regmap of syscon node %s\n",
		__func__, phandle_name);

	return NULL;
}

static const char * const modules_dt_regmap_str[REGMAP_NUMS] = {
	/*"mediatek,afe-regmap",*/
	"mediatek,apmixedsys-regmap",
};

static int mt2712_codec_parse_dt(struct mt2712_codec_priv *codec_data)
{
	struct device *dev = codec_data->codec->dev;
	int ret = 0;
	int i;

	for (i = 0; i < REGMAP_NUMS; i++) {
		codec_data->regmap_modules[i] = mt2712_codec_get_regmap_from_dt(
			modules_dt_regmap_str[i],
			codec_data);
		if (!codec_data->regmap_modules[i]) {
			dev_err(dev, "%s failed to get %s\n",
				__func__, modules_dt_regmap_str[i]);
			devm_kfree(dev, codec_data);
			ret = -EPROBE_DEFER;
			break;
		}
	}

	return ret;
}

static int mt2712_codec_probe(struct snd_soc_codec *codec)
{
	struct mt2712_codec_priv *codec_data = snd_soc_codec_get_drvdata(codec);

	dev_dbg(codec->dev, "%s():\n", __func__);

	codec_data->codec = codec;

	return mt2712_codec_parse_dt(codec_data);
}

static struct snd_soc_codec_driver mt2712_codec_driver = {
	.probe = mt2712_codec_probe,
	.component_driver = {
		.dapm_widgets           = mt2712_codec_widgets,
		.num_dapm_widgets       = ARRAY_SIZE(mt2712_codec_widgets),
		.dapm_routes            = mt2712_codec_routes,
		.num_dapm_routes        = ARRAY_SIZE(mt2712_codec_routes),
	},
};

static int mt2712_codec_dev_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mt2712_codec_priv *codec_data = NULL;
	struct resource *res;

	if (dev->of_node) {
		dev_set_name(dev, "%s", "mt2712-codec");
		pr_notice("%s set dev name %s\n", __func__,
			dev_name(dev));
	}

	codec_data = devm_kzalloc(&pdev->dev, sizeof(*codec_data), GFP_KERNEL);
	dev_set_drvdata(dev, codec_data);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	codec_data->base_addr = devm_ioremap_resource(&pdev->dev, res);
	codec_data->regmap = devm_regmap_init_mmio(&pdev->dev, codec_data->base_addr,
		&mt2712_codec_regmap_config);

	if (IS_ERR(codec_data->regmap)) {
		dev_err(dev, "%s failed to get regmap of codec\n", __func__);
		devm_kfree(dev, codec_data);
		codec_data->regmap = NULL;
		return -EINVAL;
	}

	return snd_soc_register_codec(&pdev->dev, &mt2712_codec_driver,
				      mt2712_codec_dai_driver, 1);
}

static int mt2712_codec_dev_remove(struct platform_device *pdev)
{
	snd_soc_unregister_codec(&pdev->dev);
	return 0;
}

static const struct of_device_id mt2712_codec_dt_match[] = {
	{.compatible = "mediatek,mt2712-codec",},
	{}
};

static struct platform_driver mt2712_codec = {
	.driver = {
		   .name = "mt2712-codec",
		   .owner = THIS_MODULE,
		   .of_match_table = mt2712_codec_dt_match,
		   },
	.probe = mt2712_codec_dev_probe,
	.remove = mt2712_codec_dev_remove
};

module_platform_driver(mt2712_codec);

/* Module information */
MODULE_DESCRIPTION("mt2712 codec");
MODULE_LICENSE("GPL");
