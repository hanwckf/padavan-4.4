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


/*****************************************************************************
 *                     C O M P I L E R   F L A G S
 *****************************************************************************/


/*****************************************************************************
 *                E X T E R N A L   R E F E R E N C E S
 *****************************************************************************/
#include <linux/gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/of_gpio.h>
#include <linux/device.h>

#include "mt8521p-aud-gpio.h"

#ifdef AUD_PINCTRL_SUPPORTING

struct pinctrl *pinctrlaud;

enum audio_system_gpio_type {
	GPIO_DEFAULT = 0,
	GPIO_I2S0_LRCK_MODE0_GPIO,
	GPIO_I2S0_LRCK_MODE1_I2S0,
	GPIO_I2S2_MODE1_I2S2,
	GPIO_I2S2_MODE4_DMIC,
	GPIO_PCM_MODE1_PCM,
	GPIO_PCM_MODE2_MRG,
	GPIO_SPDIF_IN0_MODE0_GPIO,
	GPIO_SPDIF_IN0_MODE1_SPDIF,
	GPIO_SPDIF_IN1_MODE0_GPIO,
	GPIO_SPDIF_IN1_MODE1_SPDIF,
	GPIO_NUM
};


struct audio_gpio_attr {
	const char *name;
	bool gpio_prepare;
	struct pinctrl_state *gpioctrl;
};

static struct audio_gpio_attr aud_gpios[GPIO_NUM] = {
	[GPIO_DEFAULT] = {"default", false, NULL},
	[GPIO_I2S0_LRCK_MODE0_GPIO] = {"audi2s0lrck-mode0-gpio", true, NULL},
	[GPIO_I2S0_LRCK_MODE1_I2S0] = {"audi2s0lrck-mode1-i2s0", true, NULL},
	[GPIO_I2S2_MODE1_I2S2] = {"audi2s2-mode1-i2s2", false, NULL},
	[GPIO_I2S2_MODE4_DMIC] = {"audi2s2-mode4-dmic", false, NULL},
	[GPIO_PCM_MODE1_PCM] = {"audpcm-mode1-pcm", false, NULL},
	[GPIO_PCM_MODE2_MRG] = {"audpcm-mode2-mrg", false, NULL},
	[GPIO_SPDIF_IN0_MODE0_GPIO] = {"audspdifin0-mode0-gpio", false, NULL},
	[GPIO_SPDIF_IN0_MODE1_SPDIF] = {"audspdifin0-mode1-spdif", false, NULL},
	[GPIO_SPDIF_IN1_MODE0_GPIO] = {"audspdifin1-mode0-gpio", false, NULL},
	[GPIO_SPDIF_IN1_MODE1_SPDIF] = {"audspdifin1-mode1-spdif", false, NULL},
};

int aud_i2s0_lrck_gpio = -1;

void mt8521p_gpio_probe(void *dev)
{
	int ret;
	int i = 0;
	struct device *pdev = dev;

	pr_warn("%s\n", __func__);

	pinctrlaud = devm_pinctrl_get(dev);
	if (IS_ERR(pinctrlaud)) {
		ret = PTR_ERR(pinctrlaud);
		pr_err("Cannot find pinctrlaud!\n");
		return;
	}

	for (i = 0; i < ARRAY_SIZE(aud_gpios); i++) {
		aud_gpios[i].gpioctrl = pinctrl_lookup_state(pinctrlaud, aud_gpios[i].name);
		if (IS_ERR(aud_gpios[i].gpioctrl)) {
			ret = PTR_ERR(aud_gpios[i].gpioctrl);
			pr_debug("%s pinctrl_lookup_state %s fail %d\n", __func__, aud_gpios[i].name,
			       ret); /* Note use pr_err since not all platform use this gpio*/
			continue;
		}
		aud_gpios[i].gpio_prepare = true;
	}

	aud_i2s0_lrck_gpio = of_get_named_gpio(pdev->of_node, "aud-i2s0-lrck-gpio", 0);
	if (gpio_is_valid(aud_i2s0_lrck_gpio)) {
		ret = devm_gpio_request(dev, aud_i2s0_lrck_gpio,
					"p1-aud-i2s0-lrck-gpio");
		if (ret)
			pr_debug("%s devm_gpio_request fail %d\n", __func__, ret);

	}

}

int mt8521p_gpio_i2s0_lrck_select(enum I2S0_LRCK_PIN_SEL pin_select)
{
	int retval = 0;

	if (pin_select == I2S0_MODE0_GPIO) {
		if (aud_gpios[GPIO_I2S0_LRCK_MODE0_GPIO].gpio_prepare) {
			retval =
			    pinctrl_select_state(pinctrlaud, aud_gpios[GPIO_I2S0_LRCK_MODE0_GPIO].gpioctrl);
			if (retval)
				pr_err("could not set aud_gpios[GPIO_I2S0_LRCK_MODE0_GPIO] pins\n");
		} else
			pr_err("does not prepare aud_gpios[GPIO_I2S0_LRCK_MODE0_GPIO] pins\n");

	} else if (pin_select == I2S0_MODE1_I2S0) {
		if (aud_gpios[GPIO_I2S0_LRCK_MODE1_I2S0].gpio_prepare) {
			retval =
			    pinctrl_select_state(pinctrlaud, aud_gpios[GPIO_I2S0_LRCK_MODE1_I2S0].gpioctrl);
			if (retval)
				pr_err("could not set aud_gpios[GPIO_I2S0_LRCK_MODE1_I2S0] pins\n");
		} else
			pr_err("does not prepare aud_gpios[GPIO_I2S0_LRCK_MODE1_I2S0] pins\n");
	} else
		pr_err("I2S0_LRCK_PIN_SEL setting Error\n");
	return retval;
}

int mt8521p_gpio_i2s2_select(enum I2S2_PIN_SEL pin_select)
{
	int retval = 0;

	if (pin_select == I2S2_MODE1_I2S2) {
		if (aud_gpios[GPIO_I2S2_MODE1_I2S2].gpio_prepare) {
			retval =
			    pinctrl_select_state(pinctrlaud, aud_gpios[GPIO_I2S2_MODE1_I2S2].gpioctrl);
			if (retval)
				pr_err("could not set aud_gpios[GPIO_I2S2_MODE1_I2S2] pins\n");
		} else
			pr_err("does not prepare aud_gpios[GPIO_I2S2_MODE1_I2S2] pins\n");
	} else if (pin_select == I2S2_MODE4_DMIC) {
		if (aud_gpios[GPIO_I2S2_MODE4_DMIC].gpio_prepare) {
			retval =
			    pinctrl_select_state(pinctrlaud, aud_gpios[GPIO_I2S2_MODE4_DMIC].gpioctrl);
			if (retval)
				pr_err("could not set aud_gpios[GPIO_I2S2_MODE4_DMIC] pins\n");
		} else
			pr_err("does not prepare aud_gpios[GPIO_I2S2_MODE4_DMIC] pins\n");

	} else
		pr_err("I2S2_PIN_SEL setting Error\n");

	return retval;
}

int mt8521p_gpio_pcm_select(enum PCM_PIN_SEL pin_select)
{
	int retval = 0;

	if (pin_select == PCM_MODE1_PCM) {
		if (aud_gpios[GPIO_PCM_MODE1_PCM].gpio_prepare) {
			retval =
			    pinctrl_select_state(pinctrlaud, aud_gpios[GPIO_PCM_MODE1_PCM].gpioctrl);
			if (retval)
				pr_err("could not set aud_gpios[GPIO_PCM_MODE1_PCM] pins\n");
		} else
			pr_err("does not prepare aud_gpios[GPIO_PCM_MODE1_PCM] pins\n");
	} else if (pin_select == PCM_MODE2_MRG) {
		if (aud_gpios[GPIO_PCM_MODE2_MRG].gpio_prepare) {
			retval =
			    pinctrl_select_state(pinctrlaud, aud_gpios[GPIO_PCM_MODE2_MRG].gpioctrl);
			if (retval)
				pr_err("could not set aud_gpios[GPIO_I2S2_MODE4_DMIC] pins\n");
		} else
			pr_err("does not prepare aud_gpios[GPIO_I2S2_MODE4_DMIC] pins\n");

	}
	return retval;
}


int mt8521p_gpio_spdif_in0_select(enum SPDIF_PIN_SEL pin_select)
{
	int retval = 0;

	if (pin_select == SPDIF_MODE0_GPIO) {
		if (aud_gpios[GPIO_SPDIF_IN0_MODE0_GPIO].gpio_prepare) {
			retval =
			    pinctrl_select_state(pinctrlaud, aud_gpios[GPIO_SPDIF_IN0_MODE0_GPIO].gpioctrl);
			if (retval)
				pr_err("could not set aud_gpios[GPIO_SPDIF_IN0_MODE0_GPIO] pins\n");
		} else
			pr_err("does not prepare aud_gpios[GPIO_SPDIF_IN0_MODE0_GPIO] pins\n");
	} else if (pin_select == SPDIF_MODE1_SPDIF) {
		if (aud_gpios[GPIO_SPDIF_IN0_MODE1_SPDIF].gpio_prepare) {
			retval =
			    pinctrl_select_state(pinctrlaud, aud_gpios[GPIO_SPDIF_IN0_MODE1_SPDIF].gpioctrl);
			if (retval)
				pr_err("could not set aud_gpios[GPIO_SPDIF_IN0_MODE1_SPDIF] pins\n");
		} else
			pr_err("does not prepare aud_gpios[GPIO_SPDIF_IN0_MODE1_SPDIF] pins\n");

	}
	return retval;
}

int mt8521p_gpio_spdif_in1_select(enum SPDIF_PIN_SEL pin_select)
{
	int retval = 0;

	if (pin_select == SPDIF_MODE0_GPIO) {
		if (aud_gpios[GPIO_SPDIF_IN1_MODE0_GPIO].gpio_prepare) {
			retval =
			    pinctrl_select_state(pinctrlaud, aud_gpios[GPIO_SPDIF_IN1_MODE0_GPIO].gpioctrl);
			if (retval)
				pr_err("could not set aud_gpios[GPIO_SPDIF_IN1_MODE0_GPIO] pins\n");
		} else
			pr_err("does not prepare aud_gpios[GPIO_SPDIF_IN1_MODE0_GPIO] pins\n");
	} else if (pin_select == SPDIF_MODE1_SPDIF) {
		if (aud_gpios[GPIO_SPDIF_IN1_MODE1_SPDIF].gpio_prepare) {
			retval =
			    pinctrl_select_state(pinctrlaud, aud_gpios[GPIO_SPDIF_IN1_MODE1_SPDIF].gpioctrl);
			if (retval)
				pr_err("could not set aud_gpios[GPIO_SPDIF_IN1_MODE1_SPDIF] pins\n");
		} else
			pr_err("does not prepare aud_gpios[GPIO_SPDIF_IN1_MODE1_SPDIF] pins\n");

	}
	return retval;
}


#endif
