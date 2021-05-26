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
#include <linux/delay.h>
/*#include <mach/mt_gpio.h>*/
#include "mt8521p-afe.h"
#include "mt8521p-aud-global.h"
#include "mt8521p-aud-gpio.h"

static int i2s_passthrough_info(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 1;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 1;
	return 0;
}

static int i2s_passthrough_get(struct snd_kcontrol *kcontrol,
			       struct snd_ctl_elem_value *ucontrol)
{
	struct demo_private *priv =
	snd_soc_card_get_drvdata(snd_kcontrol_chip(kcontrol));

	if (!priv)
		return -EINVAL;
	ucontrol->value.integer.value[0] = priv->factory_mode.i2s_passthrough;
	return 0;
}

static int i2s_passthrough_put(struct snd_kcontrol *kcontrol,
			       struct snd_ctl_elem_value *ucontrol)
{
	int passthrough = ucontrol->value.integer.value[0];
	struct demo_private *priv =
	snd_soc_card_get_drvdata(snd_kcontrol_chip(kcontrol));

	if (!priv)
		return -EINVAL;
	if (passthrough < 0 || passthrough > 1) {
		pr_warn("%s() warning: invalid i2s passthrough path\n",
			__func__);
		return -EINVAL;
	}
	{
		enum afe_i2s_in_id id;
		struct afe_i2s_in_config config = {
			.fpga_test_loop3 = 0,
			.fpga_test_loop = 0,
			.fpga_test_loop2 = 0,
			.use_asrc = 0,
			.dsd_mode = 0,
			.slave = 0,
			.fmt = FMT_64CYCLE_32BIT_I2S,
			.mode = FS_48000HZ
		};
		for (id = AFE_I2S_IN_1; id <= AFE_I2S_IN_6; ++id) {
			afe_i2s_in_configurate(id, &config);
			afe_i2s_in_enable(id, passthrough);
		}
	}
	{
		enum afe_i2s_out_id id;
		struct afe_i2s_out_config config = {
			.fpga_test_loop = 0,
			.data_from_sine = 0,
			.use_asrc = 0,
			.dsd_mode = 0,
			.couple_mode = 1,
			.one_heart_mode = 0,
			.slave = 0,
			.fmt = FMT_64CYCLE_32BIT_I2S,
			.mode = FS_48000HZ
		};
		for (id = AFE_I2S_OUT_1; id <= AFE_I2S_OUT_6; ++id) {
			afe_i2s_out_configurate(id, &config);
			pr_err("%s enable I2S %d\n", __func__, id);
			afe_i2s_out_enable(id, passthrough);
		}
	}
	itrcon_connect(I00, O15, passthrough);
	itrcon_connect(I01, O16, passthrough);
	itrcon_connect(I02, O17, passthrough);
	itrcon_connect(I03, O18, passthrough);
	itrcon_connect(I04, O19, passthrough);
	itrcon_connect(I05, O20, passthrough);
	itrcon_connect(I06, O21, passthrough);
	itrcon_connect(I07, O22, passthrough);
	itrcon_connect(I08, O23, passthrough);
	itrcon_connect(I09, O24, passthrough);
	itrcon_connect(I10, O25, passthrough);
	itrcon_connect(I11, O26, passthrough);
	priv->factory_mode.i2s_passthrough = passthrough;
	return 0;
}

static int spdif_passthrough_info(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 1;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 2;
	return 0;
}

static int spdif_passthrough_get(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol)
{
	struct demo_private *priv =
	snd_soc_card_get_drvdata(snd_kcontrol_chip(kcontrol));

	if (!priv)
		return -EINVAL;
	ucontrol->value.integer.value[0] =
	priv->factory_mode.spdif_passthrough;
	return 0;
}

static int spdif_passthrough_put(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol)
{
	int passthrough = ucontrol->value.integer.value[0];
	struct demo_private *priv =
	snd_soc_card_get_drvdata(snd_kcontrol_chip(kcontrol));

	if (!priv)
		return -EINVAL;
	if (passthrough < SPDIF_OUT2_SOURCE_IEC2 ||
	    passthrough > SPDIF_OUT2_SOURCE_COAXIAL_IN) {
		pr_warn("%s() warning: invalid spdif passthrough path\n",
			__func__);
		return -EINVAL;
	}
	afe_spdif_out2_source_sel((enum spdif_out2_source)passthrough);
	priv->factory_mode.spdif_passthrough = passthrough;
	return 0;
}

static int dmic_passthrough_info(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 1;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 1;
	return 0;
}

static int dmic_passthrough_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct demo_private *priv =
	snd_soc_card_get_drvdata(snd_kcontrol_chip(kcontrol));

	if (!priv)
		return -EINVAL;
	ucontrol->value.integer.value[0] = priv->factory_mode.dmic_passthrough;
	return 0;
}

static int dmic_passthrough_put(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	int passthrough = ucontrol->value.integer.value[0];
	struct demo_private *priv =
	snd_soc_card_get_drvdata(snd_kcontrol_chip(kcontrol));

	if (!priv)
		return -EINVAL;
	if (passthrough < 0 || passthrough > 1) {
		pr_warn("%s() warning: invalid dmic passthrough path\n",
			__func__);
		return -EINVAL;
	}
	{
		enum afe_dmic_id id;
		struct afe_dmic_config config = {
			.one_wire_mode = 1,
			.iir_on = 0,
			.iir_mode = 0,
			.voice_mode = FS_48000HZ
		};
		for (id = AFE_DMIC_1; id <= AFE_DMIC_2; ++id) {
			afe_power_on_dmic(id, passthrough);
			afe_dmic_configurate(id, &config);
			afe_dmic_enable(id, passthrough);
		}
	}
	{
		enum afe_i2s_out_id id;
		struct afe_i2s_out_config config = {
			.fpga_test_loop = 0,
			.data_from_sine = 0,
			.use_asrc = 0,
			.dsd_mode = 0,
			.couple_mode = 1,
			.one_heart_mode = 0,
			.slave = 0,
			.fmt = FMT_64CYCLE_32BIT_I2S,
			.mode = FS_48000HZ
		};
		for (id = AFE_I2S_OUT_1; id <= AFE_I2S_OUT_2; ++id) {
			afe_i2s_out_configurate(id, &config);
			pr_err("%s enable I2S %d\n", __func__, id);
			afe_i2s_out_enable(id, passthrough);
		}
	}
	itrcon_connect(I31, O15, passthrough);
	itrcon_connect(I32, O16, passthrough);
	itrcon_connect(I33, O17, passthrough);
	itrcon_connect(I34, O18, passthrough);
	priv->factory_mode.dmic_passthrough = passthrough;
	return 0;
}

static const struct snd_kcontrol_new demo_controls[] = {
	{
		.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
		.name = "I2S In/Out Pass-through",
		.info = i2s_passthrough_info,
		.get = i2s_passthrough_get,
		.put = i2s_passthrough_put
	},
	{
		.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
		.name = "SPDIF In/Out Pass-through",
		.info = spdif_passthrough_info,
		.get = spdif_passthrough_get,
		.put = spdif_passthrough_put
	},
	{
		.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
		.name = "DMIC/I2S-Out Pass-through",
		.info = dmic_passthrough_info,
		.get = dmic_passthrough_get,
		.put = dmic_passthrough_put
	},
};
