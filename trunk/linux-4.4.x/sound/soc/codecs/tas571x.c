/*
 * TAS571x amplifier audio driver
 *
 * Copyright (C) 2015 Google, Inc.
 * Copyright (c) 2013 Daniel Mack <zonque@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/gpio/consumer.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/regmap.h>
#include <linux/regulator/consumer.h>
#include <linux/stddef.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/tlv.h>

#include "tas571x.h"

#define TAS571X_MAX_SUPPLIES		6

struct tas571x_chip {
	const char			*const *supply_names;
	int				num_supply_names;
	const struct snd_kcontrol_new	*controls;
	int				num_controls;
	const struct regmap_config	*regmap_config;
	int				vol_reg_size;
};

struct tas571x_private {
	const struct tas571x_chip	*chip;
	struct regmap			*regmap;
	struct regulator_bulk_data	supplies[TAS571X_MAX_SUPPLIES];
	struct clk			*mclk;
	unsigned int			format;
	struct gpio_desc		*reset_gpio;
	struct gpio_desc		*pdn_gpio;
	struct snd_soc_codec_driver	codec_driver;
};

static int tas571x_register_size(struct tas571x_private *priv, unsigned int reg)
{
	switch (reg) {
	case TAS571X_MVOL_REG:
	case TAS571X_CH1_VOL_REG:
	case TAS571X_CH2_VOL_REG:
		return priv->chip->vol_reg_size;
	default:
		return 1;
	}
}

static int tas571x_reg_write(void *context, unsigned int reg,
			     unsigned int value)
{
	struct i2c_client *client = context;
	struct tas571x_private *priv = i2c_get_clientdata(client);
	unsigned int i, size;
	uint8_t buf[5];
	int ret;

	size = tas571x_register_size(priv, reg);
	buf[0] = reg;

	for (i = size; i >= 1; --i) {
		buf[i] = value;
		value >>= 8;
	}

	ret = i2c_master_send(client, buf, size + 1);
	if (ret == size + 1)
		return 0;
	else if (ret < 0)
		return ret;
	else
		return -EIO;
}

static int tas571x_reg_read(void *context, unsigned int reg,
			    unsigned int *value)
{
	struct i2c_client *client = context;
	struct tas571x_private *priv = i2c_get_clientdata(client);
	uint8_t send_buf, recv_buf[4];
	struct i2c_msg msgs[2];
	unsigned int size;
	unsigned int i;
	int ret;

	size = tas571x_register_size(priv, reg);
	send_buf = reg;

	msgs[0].addr = client->addr;
	msgs[0].len = sizeof(send_buf);
	msgs[0].buf = &send_buf;
	msgs[0].flags = 0;

	msgs[1].addr = client->addr;
	msgs[1].len = size;
	msgs[1].buf = recv_buf;
	msgs[1].flags = I2C_M_RD;

	ret = i2c_transfer(client->adapter, msgs, ARRAY_SIZE(msgs));
	if (ret < 0)
		return ret;
	else if (ret != ARRAY_SIZE(msgs))
		return -EIO;

	*value = 0;

	for (i = 0; i < size; i++) {
		*value <<= 8;
		*value |= recv_buf[i];
	}

	return 0;
}

static int tas571x_set_dai_fmt(struct snd_soc_dai *dai, unsigned int format)
{
	struct tas571x_private *priv = snd_soc_codec_get_drvdata(dai->codec);

	priv->format = format;

	return 0;
}

static int tas571x_hw_params(struct snd_pcm_substream *substream,
			     struct snd_pcm_hw_params *params,
			     struct snd_soc_dai *dai)
{
	struct tas571x_private *priv = snd_soc_codec_get_drvdata(dai->codec);
	u32 val;

	switch (priv->format & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_RIGHT_J:
		val = 0x00;
		break;
	case SND_SOC_DAIFMT_I2S:
		val = 0x03;
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		val = 0x06;
		break;
	default:
		return -EINVAL;
	}

	if (params_width(params) >= 24)
		val += 2;
	else if (params_width(params) >= 20)
		val += 1;

	return regmap_update_bits(priv->regmap, TAS571X_SDI_REG,
				  TAS571X_SDI_FMT_MASK, val);
}

static int tas571x_set_bias_level(struct snd_soc_codec *codec,
				  enum snd_soc_bias_level level)
{
	struct tas571x_private *priv = snd_soc_codec_get_drvdata(codec);
	int ret;

	switch (level) {
	case SND_SOC_BIAS_ON:
		break;
	case SND_SOC_BIAS_PREPARE:
		break;
	case SND_SOC_BIAS_STANDBY:
		if (snd_soc_codec_get_bias_level(codec) == SND_SOC_BIAS_OFF) {
			if (!IS_ERR(priv->mclk)) {
				ret = clk_prepare_enable(priv->mclk);
				if (ret) {
					dev_err(codec->dev,
						"Failed to enable master clock: %d\n",
						ret);
					return ret;
				}
			}

			gpiod_set_value(priv->pdn_gpio, 0);
			usleep_range(5000, 6000);

			regcache_cache_only(priv->regmap, false);
			ret = regcache_sync(priv->regmap);
			if (ret)
				return ret;
		}
		break;
	case SND_SOC_BIAS_OFF:
		regcache_cache_only(priv->regmap, true);
		gpiod_set_value(priv->pdn_gpio, 1);

		if (!IS_ERR(priv->mclk))
			clk_disable_unprepare(priv->mclk);
		break;
	}

	return 0;
}

static const struct snd_soc_dai_ops tas571x_dai_ops = {
	.set_fmt	= tas571x_set_dai_fmt,
	.hw_params	= tas571x_hw_params,
};

static const char *const tas5711_supply_names[] = {
	"AVDD",
	"DVDD",
	"PVDD_A",
	"PVDD_B",
	"PVDD_C",
	"PVDD_D",
};

static const DECLARE_TLV_DB_SCALE(tas5711_volume_tlv, -10350, 50, 1);

static const struct snd_kcontrol_new tas5711_controls[] = {
	SOC_SINGLE_TLV("Master Volume",
		       TAS571X_MVOL_REG,
		       0, 0xff, 1, tas5711_volume_tlv),
	SOC_DOUBLE_R_TLV("Speaker Volume",
			 TAS571X_CH1_VOL_REG,
			 TAS571X_CH2_VOL_REG,
			 0, 0xff, 1, tas5711_volume_tlv),
	SOC_DOUBLE("Speaker Switch",
		   TAS571X_SOFT_MUTE_REG,
		   TAS571X_SOFT_MUTE_CH1_SHIFT, TAS571X_SOFT_MUTE_CH2_SHIFT,
		   1, 1),
};

static const struct reg_default tas5711_reg_defaults[] = {
	{ 0x04, 0x05 },
	{ 0x05, 0x40 },
	{ 0x06, 0x00 },
	{ 0x07, 0xff },
	{ 0x08, 0x30 },
	{ 0x09, 0x30 },
	{ 0x1b, 0x82 },
};

static const struct regmap_config tas5711_regmap_config = {
	.reg_bits			= 8,
	.val_bits			= 32,
	.max_register			= 0xff,
	.reg_read			= tas571x_reg_read,
	.reg_write			= tas571x_reg_write,
	.reg_defaults			= tas5711_reg_defaults,
	.num_reg_defaults		= ARRAY_SIZE(tas5711_reg_defaults),
	.cache_type			= REGCACHE_RBTREE,
};

static const struct tas571x_chip tas5711_chip = {
	.supply_names			= tas5711_supply_names,
	.num_supply_names		= ARRAY_SIZE(tas5711_supply_names),
	.controls			= tas5711_controls,
	.num_controls			= ARRAY_SIZE(tas5711_controls),
	.regmap_config			= &tas5711_regmap_config,
	.vol_reg_size			= 1,
};

static const char *const tas5717_supply_names[] = {
	"AVDD",
	"DVDD",
	"HPVDD",
	"PVDD_AB",
	"PVDD_CD",
};

static const DECLARE_TLV_DB_SCALE(tas5717_volume_tlv, -10375, 25, 0);

static const struct snd_kcontrol_new tas5717_controls[] = {
	/* MVOL LSB is ignored - see comments in tas571x_i2c_probe() */
	SOC_SINGLE_TLV("Master Volume",
		       TAS571X_MVOL_REG, 1, 0x1ff, 1,
		       tas5717_volume_tlv),
	SOC_DOUBLE_R_TLV("Speaker Volume",
			 TAS571X_CH1_VOL_REG, TAS571X_CH2_VOL_REG,
			 1, 0x1ff, 1, tas5717_volume_tlv),
	SOC_DOUBLE("Speaker Switch",
		   TAS571X_SOFT_MUTE_REG,
		   TAS571X_SOFT_MUTE_CH1_SHIFT, TAS571X_SOFT_MUTE_CH2_SHIFT,
		   1, 1),
};

static const struct reg_default tas5717_reg_defaults[] = {
	{ 0x04, 0x05 },
	{ 0x05, 0x40 },
	{ 0x06, 0x00 },
	{ 0x07, 0x03ff },
	{ 0x08, 0x00c0 },
	{ 0x09, 0x00c0 },
	{ 0x1b, 0x82 },
};

static const struct regmap_config tas5717_regmap_config = {
	.reg_bits			= 8,
	.val_bits			= 32,
	.max_register			= 0xff,
	.reg_read			= tas571x_reg_read,
	.reg_write			= tas571x_reg_write,
	.reg_defaults			= tas5717_reg_defaults,
	.num_reg_defaults		= ARRAY_SIZE(tas5717_reg_defaults),
	.cache_type			= REGCACHE_RBTREE,
};

/* This entry is reused for tas5719 as the software interface is identical. */
static const struct tas571x_chip tas5717_chip = {
	.supply_names			= tas5717_supply_names,
	.num_supply_names		= ARRAY_SIZE(tas5717_supply_names),
	.controls			= tas5717_controls,
	.num_controls			= ARRAY_SIZE(tas5717_controls),
	.regmap_config			= &tas5717_regmap_config,
	.vol_reg_size			= 2,
};

static const struct snd_soc_dapm_widget tas571x_dapm_widgets[] = {
	SND_SOC_DAPM_DAC("DACL", NULL, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_DAC("DACR", NULL, SND_SOC_NOPM, 0, 0),

	SND_SOC_DAPM_OUTPUT("OUT_A"),
	SND_SOC_DAPM_OUTPUT("OUT_B"),
	SND_SOC_DAPM_OUTPUT("OUT_C"),
	SND_SOC_DAPM_OUTPUT("OUT_D"),
};

static const struct snd_soc_dapm_route tas571x_dapm_routes[] = {
	{ "DACL",  NULL, "Playback" },
	{ "DACR",  NULL, "Playback" },

	{ "OUT_A", NULL, "DACL" },
	{ "OUT_B", NULL, "DACL" },
	{ "OUT_C", NULL, "DACR" },
	{ "OUT_D", NULL, "DACR" },
};

static const struct snd_soc_codec_driver tas571x_codec = {
	.set_bias_level = tas571x_set_bias_level,
	.idle_bias_off = true,

	.dapm_widgets = tas571x_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(tas571x_dapm_widgets),
	.dapm_routes = tas571x_dapm_routes,
	.num_dapm_routes = ARRAY_SIZE(tas571x_dapm_routes),
};

static struct snd_soc_dai_driver tas571x_dai = {
	.name = "tas571x-hifi",
	.playback = {
		.stream_name = "Playback",
		.channels_min = 2,
		.channels_max = 2,
		.rates = SNDRV_PCM_RATE_8000_48000,
		.formats = SNDRV_PCM_FMTBIT_S32_LE |
			   SNDRV_PCM_FMTBIT_S24_LE |
			   SNDRV_PCM_FMTBIT_S16_LE,
	},
	.ops = &tas571x_dai_ops,
};

static const struct of_device_id tas571x_of_match[];

static int tas571x_i2c_probe(struct i2c_client *client,
			     const struct i2c_device_id *id)
{
	struct tas571x_private *priv;
	struct device *dev = &client->dev;
	const struct of_device_id *of_id;
	int i, ret;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;
	i2c_set_clientdata(client, priv);

	of_id = of_match_device(tas571x_of_match, dev);
	if (!of_id) {
		dev_err(dev, "Unknown device type\n");
		return -EINVAL;
	}
	priv->chip = of_id->data;

	priv->mclk = devm_clk_get(dev, "mclk");
	if (IS_ERR(priv->mclk) && PTR_ERR(priv->mclk) != -ENOENT) {
		dev_err(dev, "Failed to request mclk: %ld\n",
			PTR_ERR(priv->mclk));
		return PTR_ERR(priv->mclk);
	}

	BUG_ON(priv->chip->num_supply_names > TAS571X_MAX_SUPPLIES);
	for (i = 0; i < priv->chip->num_supply_names; i++)
		priv->supplies[i].supply = priv->chip->supply_names[i];

	ret = devm_regulator_bulk_get(dev, priv->chip->num_supply_names,
				      priv->supplies);
	if (ret) {
		dev_err(dev, "Failed to get supplies: %d\n", ret);
		return ret;
	}
	ret = regulator_bulk_enable(priv->chip->num_supply_names,
				    priv->supplies);
	if (ret) {
		dev_err(dev, "Failed to enable supplies: %d\n", ret);
		return ret;
	}

	priv->regmap = devm_regmap_init(dev, NULL, client,
					priv->chip->regmap_config);
	if (IS_ERR(priv->regmap))
		return PTR_ERR(priv->regmap);

	priv->pdn_gpio = devm_gpiod_get_optional(dev, "pdn", GPIOD_OUT_LOW);
	if (IS_ERR(priv->pdn_gpio)) {
		dev_err(dev, "error requesting pdn_gpio: %ld\n",
			PTR_ERR(priv->pdn_gpio));
		return PTR_ERR(priv->pdn_gpio);
	}

	priv->reset_gpio = devm_gpiod_get_optional(dev, "reset",
						   GPIOD_OUT_HIGH);
	if (IS_ERR(priv->reset_gpio)) {
		dev_err(dev, "error requesting reset_gpio: %ld\n",
			PTR_ERR(priv->reset_gpio));
		return PTR_ERR(priv->reset_gpio);
	} else if (priv->reset_gpio) {
		/* pulse the active low reset line for ~100us */
		usleep_range(100, 200);
		gpiod_set_value(priv->reset_gpio, 0);
		usleep_range(12000, 20000);
	}

	ret = regmap_write(priv->regmap, TAS571X_OSC_TRIM_REG, 0);
	if (ret)
		return ret;

	ret = regmap_update_bits(priv->regmap, TAS571X_SYS_CTRL_2_REG,
				 TAS571X_SYS_CTRL_2_SDN_MASK, 0);
	if (ret)
		return ret;

	memcpy(&priv->codec_driver, &tas571x_codec, sizeof(priv->codec_driver));
	priv->codec_driver.controls = priv->chip->controls;
	priv->codec_driver.num_controls = priv->chip->num_controls;

	if (priv->chip->vol_reg_size == 2) {
		/*
		 * The master volume defaults to 0x3ff (mute), but we ignore
		 * (zero) the LSB because the hardware step size is 0.125 dB
		 * and TLV_DB_SCALE_ITEM has a resolution of 0.01 dB.
		 */
		ret = regmap_update_bits(priv->regmap, TAS571X_MVOL_REG, 1, 0);
		if (ret)
			return ret;
	}

	regcache_cache_only(priv->regmap, true);
	gpiod_set_value(priv->pdn_gpio, 1);

	return snd_soc_register_codec(&client->dev, &priv->codec_driver,
				      &tas571x_dai, 1);
}

static int tas571x_i2c_remove(struct i2c_client *client)
{
	struct tas571x_private *priv = i2c_get_clientdata(client);

	snd_soc_unregister_codec(&client->dev);
	regulator_bulk_disable(priv->chip->num_supply_names, priv->supplies);

	return 0;
}

static const struct of_device_id tas571x_of_match[] = {
	{ .compatible = "ti,tas5711", .data = &tas5711_chip, },
	{ .compatible = "ti,tas5717", .data = &tas5717_chip, },
	{ .compatible = "ti,tas5719", .data = &tas5717_chip, },
	{ }
};
MODULE_DEVICE_TABLE(of, tas571x_of_match);

static const struct i2c_device_id tas571x_i2c_id[] = {
	{ "tas5711", 0 },
	{ "tas5717", 0 },
	{ "tas5719", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, tas571x_i2c_id);

static struct i2c_driver tas571x_i2c_driver = {
	.driver = {
		.name = "tas571x",
		.of_match_table = of_match_ptr(tas571x_of_match),
	},
	.probe = tas571x_i2c_probe,
	.remove = tas571x_i2c_remove,
	.id_table = tas571x_i2c_id,
};
module_i2c_driver(tas571x_i2c_driver);

MODULE_DESCRIPTION("ASoC TAS571x driver");
MODULE_AUTHOR("Kevin Cernekee <cernekee@chromium.org>");
MODULE_LICENSE("GPL");
