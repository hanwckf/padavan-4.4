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

static int pcm5102a_probe(struct snd_soc_codec *codec)
{
	pr_debug("%s()\n", __func__);
	return 0;
}

static struct snd_soc_codec_driver pcm5102a_driver = {
	.probe = pcm5102a_probe,
};

static struct snd_soc_dai_driver pcm5102a_dai_driver = {
	.name = "pcm5102a-i2s",
	.playback = {
		     .stream_name = "pcm5102a-i2s-playback",
		     .channels_min = 1,
		     .channels_max = 2,
		     .rates = SNDRV_PCM_RATE_8000_192000,
		     .formats = (SNDRV_PCM_FMTBIT_S16_LE |
				 SNDRV_PCM_FMTBIT_S32_LE),
		     },
};

static int pcm5102a_platform_probe(struct platform_device *pdev)
{
	int ret;
	struct device *dev = &pdev->dev;

	#ifdef CONFIG_OF
	if (dev->of_node) {
		dev_set_name(dev, "%s", "pcm5102a");
		pr_notice("%s set dev name %s\n", __func__, dev_name(dev));
	}
	#endif
	ret = snd_soc_register_codec(&pdev->dev, &pcm5102a_driver,
				     &pcm5102a_dai_driver, 1);
	pr_debug("%s() call snd_soc_register_codec() return %d\n",
		 __func__, ret);
	return ret;
}

static int pcm5102a_platform_remove(struct platform_device *pdev)
{
	pr_debug("%s()\n", __func__);
	snd_soc_unregister_codec(&pdev->dev);
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id pcm5102a_platform_dt_match[] = {
	{.compatible = "mediatek,pcm5102a",},
	{}
};
#endif

static struct platform_driver pcm5102a_platform_driver = {
	.driver = {
		   .name = "pcm5102a",
		   .owner = THIS_MODULE,
		   #ifdef CONFIG_OF
		   .of_match_table = pcm5102a_platform_dt_match,
		   #endif
		   },
	.probe = pcm5102a_platform_probe,
	.remove = pcm5102a_platform_remove
};

module_platform_driver(pcm5102a_platform_driver);

/* Module information */
MODULE_DESCRIPTION("ASoC pcm5102a codec driver");
MODULE_LICENSE("GPL");
