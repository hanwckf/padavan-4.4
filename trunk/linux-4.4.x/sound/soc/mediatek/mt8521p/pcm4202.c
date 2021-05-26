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
#include <linux/init.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>

static int pcm4202_probe(struct snd_soc_codec *codec)
{
	pr_debug("%s()\n", __func__);
	return 0;
}

static struct snd_soc_codec_driver pcm4202_driver = {
	.probe = pcm4202_probe,
};

static struct snd_soc_dai_driver pcm4202_dai_driver = {
	.name = "pcm4202-i2s",
	.capture = {
		    .stream_name = "pcm4202-i2s-capture",
		    .channels_min = 2,
		    .channels_max = 2,
		    .rates = SNDRV_PCM_RATE_8000_192000,
		    .formats = (SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S32_LE
				| SNDRV_PCM_FMTBIT_DSD_U8 | SNDRV_PCM_FMTBIT_DSD_U16_LE),
		    },
};

static int pcm4202_platform_probe(struct platform_device *pdev)
{
	int ret;
	struct device *dev = &pdev->dev;

	#ifdef CONFIG_OF
	if (dev->of_node) {
		dev_set_name(dev, "%s", "pcm4202");
		pr_notice("%s set dev name %s\n", __func__, dev_name(dev));
	}
	#endif
	ret = snd_soc_register_codec(&pdev->dev, &pcm4202_driver, &pcm4202_dai_driver, 1);
	pr_debug("%s() call snd_soc_register_codec() return %d\n", __func__, ret);
	return ret;
}

static int pcm4202_platform_remove(struct platform_device *pdev)
{
	pr_debug("%s()\n", __func__);
	snd_soc_unregister_codec(&pdev->dev);
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id pcm4202_platform_dt_match[] = {
	{.compatible = "mediatek,pcm4202",},
	{}
};
#endif

static struct platform_driver pcm4202_platform_driver = {
	.driver = {
		   .name = "pcm4202",
		   .owner = THIS_MODULE,
		   #ifdef CONFIG_OF
		   .of_match_table = pcm4202_platform_dt_match,
		   #endif
		   },
	.probe = pcm4202_platform_probe,
	.remove = pcm4202_platform_remove
};

module_platform_driver(pcm4202_platform_driver);

/* Module information */
MODULE_DESCRIPTION("ASoC pcm4202 codec driver");
MODULE_LICENSE("GPL");
