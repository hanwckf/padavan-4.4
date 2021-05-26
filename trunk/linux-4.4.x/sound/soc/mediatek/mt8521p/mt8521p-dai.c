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

#include <linux/dma-mapping.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <sound/soc.h>
#include "mt8521p-dai.h"
#include "mt8521p-dai-private.h"
#include "mt8521p-i2s.c"
#include "mt8521p-dmic.c"
#include "mt8521p-btpcm.c"
#include "mt8521p-mrg.c"
#include "mt8521p-aud-global.h"
#ifdef AUD_K318_MIGRATION
#include "mt8521p-audio-controls.c"
#endif


static struct snd_soc_component_driver mt8521p_soc_component_driver = {
	.name = "mt8521p-dai",
	#ifdef AUD_K318_MIGRATION
	.controls = mt8521p_soc_controls,
	.num_controls = ARRAY_SIZE(mt8521p_soc_controls)
	#endif
};

static struct snd_soc_dai_driver mt8521p_soc_dai_drivers[] = {
	{
		.name = "mt8521p-i2s1",
		.id = MT_DAI_I2S1_ID,
		.probe = mt8521p_i2s_probe,
		.ops = &mt8521p_i2s_ops,
		.playback = {
			.stream_name = "mt8521p-i2s-out1",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = (SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S24_LE
				| SNDRV_PCM_FMTBIT_S32_LE
				| SNDRV_PCM_FMTBIT_DSD_U8
				| SNDRV_PCM_FMTBIT_DSD_U16_LE)
		},
		.capture = {
			.stream_name = "mt8521p-i2s-in1",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = (SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S24_LE
				| SNDRV_PCM_FMTBIT_S32_LE
				| SNDRV_PCM_FMTBIT_DSD_U8
				| SNDRV_PCM_FMTBIT_DSD_U16_LE)
		}
	},
	{
		.name = "mt8521p-i2s2",
		.id = MT_DAI_I2S2_ID,
		.probe = mt8521p_i2s_probe,
		.ops = &mt8521p_i2s_ops,
		.playback = {
			.stream_name = "mt8521p-i2s-out2",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = (SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S24_LE
				| SNDRV_PCM_FMTBIT_S32_LE
				| SNDRV_PCM_FMTBIT_DSD_U8
				| SNDRV_PCM_FMTBIT_DSD_U16_LE)
		},
		.capture = {
			.stream_name = "mt8521p-i2s-in2",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = (SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S24_LE
				| SNDRV_PCM_FMTBIT_S32_LE
				| SNDRV_PCM_FMTBIT_DSD_U8
				| SNDRV_PCM_FMTBIT_DSD_U16_LE)
		}
	},
	{
		.name = "mt8521p-i2s3",
		.id = MT_DAI_I2S3_ID,
		.probe = mt8521p_i2s_probe,
		.ops = &mt8521p_i2s_ops,
		.playback = {
			.stream_name = "mt8521p-i2s-out3",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = (SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S24_LE
				| SNDRV_PCM_FMTBIT_S32_LE
				| SNDRV_PCM_FMTBIT_DSD_U8
				| SNDRV_PCM_FMTBIT_DSD_U16_LE)
		},
		.capture = {
			.stream_name = "mt8521p-i2s-in3",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = (SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S24_LE
				| SNDRV_PCM_FMTBIT_S32_LE
				| SNDRV_PCM_FMTBIT_DSD_U8
				| SNDRV_PCM_FMTBIT_DSD_U16_LE)
		}
	},
	{
		.name = "mt8521p-i2s4",
		.id = MT_DAI_I2S4_ID,
		.probe = mt8521p_i2s_probe,
		.ops = &mt8521p_i2s_ops,
		.playback = {
			.stream_name = "mt8521p-i2s-out4",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = (SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S24_LE
				| SNDRV_PCM_FMTBIT_S32_LE
				| SNDRV_PCM_FMTBIT_DSD_U8
				| SNDRV_PCM_FMTBIT_DSD_U16_LE)
		},
		.capture = {
			.stream_name = "mt8521p-i2s-in4",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = (SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S24_LE
				| SNDRV_PCM_FMTBIT_S32_LE
				| SNDRV_PCM_FMTBIT_DSD_U8
				| SNDRV_PCM_FMTBIT_DSD_U16_LE)
		}
	},
	{
		.name = "mt8521p-i2s5",
		.id = MT_DAI_I2S5_ID,
		.probe = mt8521p_i2s_probe,
		.ops = &mt8521p_i2s_ops,
		.playback = {
			.stream_name = "mt8521p-i2s-out5",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = (SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S24_LE
				| SNDRV_PCM_FMTBIT_S32_LE
				| SNDRV_PCM_FMTBIT_DSD_U8
				| SNDRV_PCM_FMTBIT_DSD_U16_LE)
		},
		.capture = {
			.stream_name = "mt8521p-i2s-in5",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = (SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S24_LE
				| SNDRV_PCM_FMTBIT_S32_LE
				| SNDRV_PCM_FMTBIT_DSD_U8
				| SNDRV_PCM_FMTBIT_DSD_U16_LE)
		}
	},
	{
		.name = "mt8521p-i2s6",
		.id = MT_DAI_I2S6_ID,
		.probe = mt8521p_i2s_probe,
		.ops = &mt8521p_i2s_ops,
		.playback = {
			.stream_name = "mt8521p-i2s-out6",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = (SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S24_LE
				| SNDRV_PCM_FMTBIT_S32_LE
				| SNDRV_PCM_FMTBIT_DSD_U8
				| SNDRV_PCM_FMTBIT_DSD_U16_LE)
		},
		.capture = {
			.stream_name = "mt8521p-i2s-in6",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = (SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S24_LE
				| SNDRV_PCM_FMTBIT_S32_LE
				| SNDRV_PCM_FMTBIT_DSD_U8
				| SNDRV_PCM_FMTBIT_DSD_U16_LE)
		}
	},
	{
		.name = "mt8521p-i2sm",
		.id = MT_DAI_I2SM_ID,
		.probe = mt8521p_i2s_probe,
		.ops = &mt8521p_i2s_ops,
		.playback = {
			.stream_name = "mt8521p-i2s-outm",
			.channels_min = 1,
			.channels_max = 10,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = (SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S24_LE
				| SNDRV_PCM_FMTBIT_S32_LE)
		},
	},
	{
		.name = "mt8521p-btpcm",
		.id = MT_DAI_BTPCM_ID,
		.ops = &mt8521p_btpcm_ops,
		.playback = {
			.stream_name = "mt8521p-btpcm-out",
			.channels_min = 1,
			.channels_max = 1,
			.rates = SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000,
			.formats = (SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S32_LE)
		},
		.capture = {
			.stream_name = "mt8521p-btpcm-in",
			.channels_min = 1,
			.channels_max = 1,
			.rates = SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000,
			.formats = (SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S32_LE)
		}
	},
	{
		   .name =  "mt8521p-hdmi-pcm-out",
		   .id = MT_DAI_HDMI_OUT_I2S_ID,
		   .ops = NULL,
		   .playback = {
			   .stream_name = "hdmi-pcm-out",
			   .channels_min = 1,
			   .channels_max = 8,
			   .rates = SNDRV_PCM_RATE_8000_192000,
			   .formats = (SNDRV_PCM_FMTBIT_S16_LE
				|SNDRV_PCM_FMTBIT_DSD_U8
				|SNDRV_PCM_FMTBIT_S24_LE
				|SNDRV_PCM_FMTBIT_S32_LE)
		 },
	},
	{
		   .name =  "mt8521p-hdmi-rawpcm-out",
		   .id = MT_DAI_HDMI_OUT_IEC_ID,
		   .ops =  NULL,
		   .playback = {
			   .stream_name = "hdmi-rawpcm-out",
			   .channels_min = 2,
			   .channels_max = 2,
			   .rates = (SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_44100 |
				     SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_88200 |
				     SNDRV_PCM_RATE_96000 | SNDRV_PCM_RATE_176400 |
				     SNDRV_PCM_RATE_192000),
			   .formats = SNDRV_PCM_FMTBIT_S16_LE
		   },
	},
	{
		.name = "mt8521p-spdif-out",
		.id = MT_DAI_SPDIF_OUT_ID,
		.ops = NULL,
		.playback = {
			.stream_name = "mt8521p-spdif-out",
			.channels_min = 2,
			.channels_max = 2,
			.rates = (SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_44100 |
				  SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_88200 |
				  SNDRV_PCM_RATE_96000 | SNDRV_PCM_RATE_176400 |
				  SNDRV_PCM_RATE_192000),
			.formats = (SNDRV_PCM_FMTBIT_S16_LE
				 | SNDRV_PCM_FMTBIT_S24_3LE)
		},
	},
	{
		.name = "mt8521p-multi-in",
		.id = MT_DAI_MULTI_IN_ID,
		.ops = NULL,
		.capture = {
			.stream_name = "mt8521p-multi-in",
			.channels_min = 1,
			.channels_max = 8,
			.rates = SNDRV_PCM_RATE_32000
				| SNDRV_PCM_RATE_44100
				| SNDRV_PCM_RATE_48000
				| SNDRV_PCM_RATE_88200
				| SNDRV_PCM_RATE_96000
				| SNDRV_PCM_RATE_176400
				| SNDRV_PCM_RATE_192000,
			.formats = (SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S24_LE
				| SNDRV_PCM_FMTBIT_S24_3LE
				| SNDRV_PCM_FMTBIT_DSD_U8
				| SNDRV_PCM_FMTBIT_S32_LE)
		},
	},
	{
		.name = "mt8521p-mrgbt",
		.id = MT_DAI_MRGIF_BT_ID,
		.ops = &mt8521p_btmrg_ops,
		.playback = {
			.stream_name = "mt8521p-mrgbt-out",
			.channels_min = 1,
			.channels_max = 1,
			.rates = (SNDRV_PCM_RATE_8000
				| SNDRV_PCM_RATE_16000),
			.formats = (SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S32_LE)
		},
		.capture = {
			.stream_name = "mt8521p-mrgbt-in",
			.channels_min = 1,
			.channels_max = 1,
			.rates = (SNDRV_PCM_RATE_8000
				| SNDRV_PCM_RATE_16000),
			.formats = (SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S32_LE)
		},
	},

	{
		.name = "mt8521p-mrgi2s",
		.id = MT_DAI_MRGIF_I2S_ID,
		.ops = &mt8521p_i2smrg_ops,
		.playback = {
			.stream_name = "mt8521p-mrgi2s-out",
			.channels_min = 1,
			.channels_max = 2,
			.rates = (SNDRV_PCM_RATE_32000
				| SNDRV_PCM_RATE_44100
				|SNDRV_PCM_RATE_48000),
			.formats = (SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S32_LE)
		},
		.capture = {
			.stream_name = "mt8521p-mrgi2s-in",
			.channels_min = 1,
			.channels_max = 2,
			.rates = (SNDRV_PCM_RATE_32000
				| SNDRV_PCM_RATE_44100
				|SNDRV_PCM_RATE_48000),
			.formats = (SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S32_LE)
		},
	},
	{
		.name = "mt8521p-dmic1",
		.id = MT_DAI_DMIC1_ID,
		.probe = mt8521p_dmic_probe,
		.ops = &mt8521p_dmic_ops,
		.capture = {
			.stream_name = "mt8521p-dmic-in1",
			.channels_min = 2,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000
				| SNDRV_PCM_RATE_16000
				| SNDRV_PCM_RATE_32000
				| SNDRV_PCM_RATE_44100
				| SNDRV_PCM_RATE_48000,
			.formats = (SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S32_LE)
		},
	},
	{
		.name = "mt8521p-dmic2",
		.id = MT_DAI_DMIC2_ID,
		.probe = mt8521p_dmic_probe,
		.ops = &mt8521p_dmic_ops,
		.capture = {
			.stream_name = "mt8521p-dmic-in2",
			.channels_min = 2,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000
				| SNDRV_PCM_RATE_16000
				| SNDRV_PCM_RATE_32000
				| SNDRV_PCM_RATE_44100
				| SNDRV_PCM_RATE_48000,
			.formats = (SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S32_LE)
		},
	},
	{
		.name = "mt8521p-i2s1-ck1ck2",
		.id = MT_DAI_I2S1_CK1CK2_ID,
		.probe = mt8521p_i2s_probe,
		.ops = &mt8521p_i2s_ops,
		.capture = {
			.stream_name = "mt8521p-i2s-in1-ck1ck2",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = (SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S24_LE
				| SNDRV_PCM_FMTBIT_S32_LE)
		},
	},
	{
		.name = "mt8521p-i2s2-ck1ck2",
		.id = MT_DAI_I2S2_CK1CK2_ID,
		.probe = mt8521p_i2s_probe,
		.ops = &mt8521p_i2s_ops,
		.capture = {
			.stream_name = "mt8521p-i2s-in2-ck1ck2",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = (SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S24_LE
				| SNDRV_PCM_FMTBIT_S32_LE)
		},
	},
	{
		.name = "mt8521p-i2smx",
		.id = MT_DAI_I2SMX_ID,
		.probe = mt8521p_i2s_probe,
		.ops = &mt8521p_i2s_ops,
		.capture = {
			.stream_name = "mt8521p-i2s-inmx",
			.channels_min = 1,
			.channels_max = 4,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = (SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S32_LE)
		}
	}

	/* add other dai here */
};

static int mt8521p_dai_probe(struct platform_device *pdev)
{
	struct mt_dai_private *priv;
	struct device *dev = &pdev->dev;

	pr_debug("%s()\n", __func__);
	#ifdef CONFIG_OF
	if (dev->of_node) {
		dev_set_name(dev, "%s", "mt8521p-dai");
		pr_notice("%s set dev name %s\n", __func__, dev_name(dev));
	}
	#endif

	priv = devm_kzalloc(&pdev->dev, sizeof(struct mt_dai_private), GFP_KERNEL);
	/*
	* Possible unnecessary 'out of memory' message
	* if (!priv) {
	*	dev_err(&pdev->dev, "%s() can't allocate memory\n", __func__);
	*	return -ENOMEM;
	* }
	*/
	dev_set_drvdata(&pdev->dev, priv);
	return snd_soc_register_component(&pdev->dev
					 , &mt8521p_soc_component_driver
					 , mt8521p_soc_dai_drivers
					 , ARRAY_SIZE(mt8521p_soc_dai_drivers));
}

static int mt8521p_dai_remove(struct platform_device *pdev)
{
	struct mt_dai_private *priv;

	pr_debug("%s()\n", __func__);
	priv = dev_get_drvdata(&pdev->dev);
	devm_kfree(&pdev->dev, priv);
	snd_soc_unregister_component(&pdev->dev);
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id mt8521p_dai_dt_match[] = {
	{.compatible = "mediatek,mt8521p-dai",},
	{}
};
#endif

static struct platform_driver mt8521p_dai = {
	.driver = {
	.name = "mt8521p-dai",
	.owner = THIS_MODULE,
	#ifdef CONFIG_OF
	.of_match_table = mt8521p_dai_dt_match,
	#endif
	},
	.probe = mt8521p_dai_probe,
	.remove = mt8521p_dai_remove
};

module_platform_driver(mt8521p_dai);

MODULE_DESCRIPTION("mt8521p dai driver");
MODULE_LICENSE("GPL");
