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

static struct snd_soc_codec_driver dummy_codec_driver = {
};

static struct snd_soc_dai_driver dummy_codec_dai_driver = {
	.name = "dummy-codec-i2s",
	.playback = {
		     .stream_name = "dummy-codec-i2s-playback",
		     .channels_min = 1,
		     .channels_max = 10,
		     .rates = SNDRV_PCM_RATE_8000_192000,
		     .formats = (SNDRV_PCM_FMTBIT_S16_LE |
				 SNDRV_PCM_FMTBIT_S24_LE |
				 SNDRV_PCM_FMTBIT_S24_3LE |
				 SNDRV_PCM_FMTBIT_S32_LE |
				 SNDRV_PCM_FMTBIT_DSD_U8 |
				 SNDRV_PCM_FMTBIT_DSD_U16_LE),
		     },
	.capture = {
		    .stream_name = "dummy-codec-i2s-capture",
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
};

static int dummy_codec_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;

	#ifdef CONFIG_OF
		if (dev->of_node) {
			dev_set_name(dev, "%s", "dummy-codec");
			pr_notice("%s set dev name %s\n", __func__,
				  dev_name(dev));
		}
	#endif
	return snd_soc_register_codec(&pdev->dev, &dummy_codec_driver,
				      &dummy_codec_dai_driver, 1);
}

static int dummy_codec_remove(struct platform_device *pdev)
{
	snd_soc_unregister_codec(&pdev->dev);
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id dummy_codec_dt_match[] = {
	{.compatible = "mediatek,dummy-codec",},
	{}
};
#endif

static struct platform_driver dummy_codec = {
	.driver = {
		   .name = "dummy-codec",
		   .owner = THIS_MODULE,
		   #ifdef CONFIG_OF
		   .of_match_table = dummy_codec_dt_match,
		    #endif
		   },
	.probe = dummy_codec_probe,
	.remove = dummy_codec_remove
};

module_platform_driver(dummy_codec);

/* Module information */
MODULE_DESCRIPTION("dummy codec");
MODULE_LICENSE("GPL");
