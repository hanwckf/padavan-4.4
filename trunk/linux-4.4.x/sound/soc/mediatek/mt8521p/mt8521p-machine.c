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

struct demo_factory_mode {
	int i2s_passthrough;
	int spdif_passthrough;
	int dmic_passthrough;
};

struct demo_private {
	struct demo_factory_mode factory_mode;
};

#include "mt8521p-machine-links.c"
#include "mt8521p-machine-controls.c"

static struct snd_soc_card demo_soc_card = {
	.name = "demo-soc-card",
	.dai_link = demo_dai_links,
	.num_links = ARRAY_SIZE(demo_dai_links),
	.controls = demo_controls,
	.num_controls = ARRAY_SIZE(demo_controls),
};

static int demo_machine_probe(struct platform_device *pdev)
{
	struct snd_soc_card *card = &demo_soc_card;
	struct device *dev = &pdev->dev;
	int ret;
	struct demo_private *priv =
		devm_kzalloc(&pdev->dev, sizeof(struct demo_private), GFP_KERNEL);

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

static int demo_machine_remove(struct platform_device *pdev)
{
	struct snd_soc_card *card = platform_get_drvdata(pdev);

	mt_afe_platform_deinit(&pdev->dev);
	devm_kfree(&pdev->dev, snd_soc_card_get_drvdata(card));
	return snd_soc_unregister_card(card);
}

#ifdef CONFIG_OF
static const struct of_device_id demo_machine_dt_match[] = {
	{.compatible = "mediatek,mt8521p-soc-machine",},
	{}
};
#endif

static struct platform_driver demo_machine = {
	.driver = {
		.name = "mt8521p-soc-machine",
		.owner = THIS_MODULE,
		   #ifdef CONFIG_OF
		   .of_match_table = demo_machine_dt_match,
		   #endif
	},
	.probe = demo_machine_probe,
	.remove = demo_machine_remove
};

module_platform_driver(demo_machine);

/* Module information */
MODULE_DESCRIPTION("mt8521p demo machine driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("mt8521p demo soc card");
