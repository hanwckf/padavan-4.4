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

#include "mt8521p-afe-reg.h"
#include "mt8521p-afe.h"

int iec1_chst_flag;
int iec2_chst_flag;

static int spdif_rx_info(struct snd_kcontrol *kcontrol,
			 struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 1;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 2;
	return 0;
}

static int spdif_rx_get(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	const volatile struct afe_dir_info *state = afe_spdifrx_state();
	int rate;
	int i;

	switch (state->rate) {
	case 0x7:
		rate = 32000;
		break;
	case 0x8:
		rate = 44100;
		break;
	case 0x9:
		rate = 48000;
		break;
	case 0xb:
		rate = 88200;
		break;
	case 0xc:
		rate = 96000;
		break;
	case 0xe:
		rate = 176400;
		break;
	case 0xf:
		rate = 192000;
		break;
	default:
		rate = 0;
		break;
	}
	memcpy((void *)ucontrol->value.bytes.data, (void *)&rate, sizeof(rate));
	memcpy((void *)ucontrol->value.bytes.data + sizeof(rate),
	       (void *)state->u_bit, sizeof(state->u_bit));
	memcpy((void *)ucontrol->value.bytes.data + sizeof(rate) +
	       sizeof(state->u_bit), (void *)state->c_bit,
	       sizeof(state->c_bit));
	pr_notice("%s() rate=0x%X\n", __func__, rate);
	for (i = 0; i < 4; i++)
		pr_debug("%s() ucontrol->value.bytes.data[%d]=0x%02X\n",
			 __func__,
			i, ucontrol->value.bytes.data[i]);
	for (i = 4; i < 48 + 4; i++)
		pr_debug("%s() ucontrol->value.bytes.data[%d]=0x%02X\n",
			 __func__,
			i, ucontrol->value.bytes.data[i]);
	for (i = 4 + 48; i < 76; i++)
		pr_debug("%s() ucontrol->value.bytes.data[%d]=0x%02X\n",
			 __func__, i, ucontrol->value.bytes.data[i]);
	return 0;
}

static struct snd_kcontrol *snd_ctl_find_name(struct snd_card *card,
					      unsigned char *name)
{
	struct snd_kcontrol *kctl;

	if (snd_BUG_ON(!card || !name))
		return NULL;
	list_for_each_entry(kctl, &card->controls, list) {
		if (!strncmp(kctl->id.name, name, sizeof(kctl->id.name)))
			return kctl;
	}
	return NULL;
}

static void spdif_rx_ctl_notify(void)
{
	struct snd_card *card = snd_card_test;
	struct snd_kcontrol *kctl;

	kctl = snd_ctl_find_name(card, "SPDIF In");
	if (!kctl) {
		pr_err("%s() can not get name\n", __func__);
		return;
	}
	if (!kctl->id.name) {
		pr_err("%s() can not get name\n", __func__);
		return;
	}
	snd_ctl_notify(card, SNDRV_CTL_EVENT_MASK_VALUE, &kctl->id);
}

static int spdif_rx_put(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	/* 0:stop, 1:start opt, 2:start arc */
	enum afe_spdifrx_port port =
	(enum afe_spdifrx_port)ucontrol->value.integer.value[0];

	if (port != SPDIFRX_PORT_NONE &&
	    port != SPDIFRX_PORT_OPT &&
	    port != SPDIFRX_PORT_ARC)
		return -EINVAL;
	pr_debug("%s() port=%d\n", __func__, port);
	if (port == SPDIFRX_PORT_NONE)
		afe_spdifrx_stop();
	else
		afe_spdifrx_start(port, spdif_rx_ctl_notify);
	return 0;
}

#define DUMMY_SPK_VOL_MIN  0
#define DUMMY_SPK_VOL_MAX  100
#define DUMMY_SPK_VOL_NUM  101

volatile unsigned int g_dummy_spk_vol = DUMMY_SPK_VOL_MAX;

static int dummy_speaker_info_volume(struct snd_kcontrol *kctl,
				     struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 1;
	uinfo->value.integer.min = DUMMY_SPK_VOL_MIN;
	uinfo->value.integer.max = DUMMY_SPK_VOL_MAX;
	return 0;
}

static int dummy_speaker_get_volume(struct snd_kcontrol *kctl,
				    struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = g_dummy_spk_vol;
	pr_debug("dummy_speaker_get_volume() got vol: %d\n",
		 (unsigned int)(ucontrol->value.integer.value[0]));
	return 0;
}

static int dummy_speaker_put_volume(struct snd_kcontrol *kctl,
				    struct snd_ctl_elem_value *ucontrol)
{
	if (DUMMY_SPK_VOL_MAX <
	    (unsigned int)(ucontrol->value.integer.value[0])) {
		pr_debug("dummy_speaker_put_volume() get invalid vol: %d\n",
			 (unsigned int)(ucontrol->value.integer.value[0]));
		g_dummy_spk_vol = DUMMY_SPK_VOL_MAX;
	} else {
		g_dummy_spk_vol =
		(unsigned int)(ucontrol->value.integer.value[0]);
	}
	pr_debug("dummy_speaker_put_volume() set vol Indx: %d\n",
		 g_dummy_spk_vol);
	return 0;
}

#define DUMMY_SPK_UNMUTE  0
#define DUMMY_SPK_MUTE    1
volatile unsigned int g_dummy_spk_muted = DUMMY_SPK_UNMUTE;

static int dummy_speaker_info_switch(struct snd_kcontrol *kcontrol,
				     struct snd_ctl_elem_info *uinfo)
{
	if (!uinfo) {
		pr_err("%s() receives invalid uinfo\n", __func__);
		return -1;
	}
	uinfo->type = SNDRV_CTL_ELEM_TYPE_BOOLEAN;
	uinfo->count = 1;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 1;
	return 0;
}

static int dummy_speaker_get_switch(struct snd_kcontrol *kcontrol,
				    struct snd_ctl_elem_value *ucontrol)
{
	if (!ucontrol) {
		pr_err("%s() dummy_speaker_get_switch() receives invalid ucontrol\n",
		       __func__);
		return -1;
	}
	ucontrol->value.integer.value[0] = g_dummy_spk_muted;
	pr_debug("Get ALSA switch value: %d\n",
		 (unsigned int)g_dummy_spk_muted);
	return 0;
}

static int dummy_speaker_put_switch(struct snd_kcontrol *kcontrol,
				    struct snd_ctl_elem_value *ucontrol)
{
	switch (ucontrol->value.integer.value[0]) {
	case DUMMY_SPK_UNMUTE:
		pr_debug("dummy_speaker_put_switch() receives unmute cmd\n");
		g_dummy_spk_muted = ucontrol->value.integer.value[0];
		break;
	case DUMMY_SPK_MUTE:
		pr_debug("dummy_speaker_put_switch() receives mute cmd\n");
		g_dummy_spk_muted = ucontrol->value.integer.value[0];
		break;
	default:
		pr_warn("dummy_speaker_put_switch() receives invalid value: %d\n",
			(unsigned int)(ucontrol->value.integer.value[0]));
		break;
	}
	return 0;
}

struct spdif_info {
	spinlock_t reg_lock; /*spinlock for spdif info*/
	unsigned int ch_status0;
	unsigned int ch_status1;
};

static struct spdif_info spdifinfo;

static int spdif_out_info(struct snd_kcontrol *kcontrol,
			  struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_IEC958;
	uinfo->count = 5;
	return 0;
}

static int spdif_out_get(struct snd_kcontrol *kcontrol,
			 struct snd_ctl_elem_value *ucontrol)
{
	/*spin_lock_irq(&spdifinfo->reg_lock);*/
	ucontrol->value.iec958.status[0] = (spdifinfo.ch_status0 >> 0) & 0xff;
	ucontrol->value.iec958.status[1] = (spdifinfo.ch_status0 >> 8) & 0xff;
	ucontrol->value.iec958.status[2] = (spdifinfo.ch_status0 >> 16) & 0xff;
	ucontrol->value.iec958.status[3] = (spdifinfo.ch_status0 >> 24) & 0xff;
	ucontrol->value.iec958.status[4] = (spdifinfo.ch_status1 >> 0) & 0xff;
	ucontrol->value.iec958.status[5] = (spdifinfo.ch_status1 >> 8) & 0xff;
	ucontrol->value.iec958.status[6] = (spdifinfo.ch_status1 >> 16) & 0xff;
	ucontrol->value.iec958.status[7] = (spdifinfo.ch_status1 >> 24) & 0xff;
	pr_debug("%s() ch0=0x%x, ch1=0x%x\n", __func__, spdifinfo.ch_status0,
		 spdifinfo.ch_status1);
	/*spin_unlock_irq(&spdifinfo->reg_lock);*/
	return 0;
}

static int spdif_out_put(struct snd_kcontrol *kcontrol,
			 struct snd_ctl_elem_value *ucontrol)
{
	unsigned int ch_status0_temp = 0;
	unsigned int ch_status1_temp = 0;
	int change = 0;

	iec2_chst_flag = 1;
	ch_status0_temp = ((u32)ucontrol->value.iec958.status[0] << 0) |
			  ((u32)ucontrol->value.iec958.status[1] << 8) |
			  ((u32)ucontrol->value.iec958.status[2] << 16) |
			  ((u32)ucontrol->value.iec958.status[3] << 24);
	ch_status1_temp = ((u32)ucontrol->value.iec958.status[4] << 0) |
			  ((u32)ucontrol->value.iec958.status[5] << 8) |
			  ((u32)ucontrol->value.iec958.status[6] << 16) |
			  ((u32)ucontrol->value.iec958.status[7] << 24);
	/*spin_lock_irq(&spdifinfo->reg_lock);*/
	change = ((spdifinfo.ch_status0 != ch_status0_temp) ||
		 (spdifinfo.ch_status1 != ch_status1_temp));
	spdifinfo.ch_status0 = ch_status0_temp;
	spdifinfo.ch_status1 = ch_status1_temp;
	pr_debug("%s() ch0=0x%x, ch1=0x%x, change=%d\n",
		 __func__, spdifinfo.ch_status0, spdifinfo.ch_status1, change);
	if (change) {
		afe_msk_write(AFE_IEC2_CHL_STAT0, spdifinfo.ch_status0,
			      0xffffffff);
		afe_msk_write(AFE_IEC2_CHR_STAT0, spdifinfo.ch_status0,
			      0xffffffff);
		afe_msk_write(AFE_IEC2_CHL_STAT1, spdifinfo.ch_status1,
			      0x0000ffff);
		afe_msk_write(AFE_IEC2_CHR_STAT1, spdifinfo.ch_status1,
			      0x0000ffff);
	} else {
		pr_err("%s() spdif no change\n", __func__);
	}
	/*spin_unlock_irq(&spdifinfo->reg_lock);*/
	return change;
}

struct hdmi_info {
	spinlock_t reg_lock; /*spinlock for HDMI info*/
	unsigned int ch_status0;
	unsigned int ch_status1;
};

static struct hdmi_info hdmiinfo;

static int hdmi_out_info(struct snd_kcontrol *kcontrol,
			 struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_IEC958;
	uinfo->count = 5;
	return 0;
}

static int hdmi_out_get(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	/*spin_lock_irq(&spdifinfo->reg_lock);*/
	ucontrol->value.iec958.status[0] = (hdmiinfo.ch_status0 >> 0) & 0xff;
	ucontrol->value.iec958.status[1] = (hdmiinfo.ch_status0 >> 8) & 0xff;
	ucontrol->value.iec958.status[2] = (hdmiinfo.ch_status0 >> 16) & 0xff;
	ucontrol->value.iec958.status[3] = (hdmiinfo.ch_status0 >> 24) & 0xff;
	ucontrol->value.iec958.status[4] = (hdmiinfo.ch_status1 >> 0) & 0xff;
	ucontrol->value.iec958.status[5] = (hdmiinfo.ch_status1 >> 8) & 0xff;
	ucontrol->value.iec958.status[6] = (hdmiinfo.ch_status1 >> 16) & 0xff;
	ucontrol->value.iec958.status[7] = (hdmiinfo.ch_status1 >> 24) & 0xff;
	pr_debug("%s() ch0=0x%x, ch1=0x%x\n", __func__, hdmiinfo.ch_status0,
		 hdmiinfo.ch_status1);
	/*spin_unlock_irq(&spdifinfo->reg_lock);*/
	return 0;
}

static int hdmi_out_put(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	unsigned int ch_status0_temp = 0;
	unsigned int ch_status1_temp = 0;
	int change = 0;

	iec1_chst_flag = 1;
	ch_status0_temp = ((u32)ucontrol->value.iec958.status[0] << 0) |
			  ((u32)ucontrol->value.iec958.status[1] << 8) |
			  ((u32)ucontrol->value.iec958.status[2] << 16) |
			  ((u32)ucontrol->value.iec958.status[3] << 24);
	ch_status1_temp = ((u32)ucontrol->value.iec958.status[4] << 0) |
			  ((u32)ucontrol->value.iec958.status[5] << 8) |
			  ((u32)ucontrol->value.iec958.status[6] << 16) |
			  ((u32)ucontrol->value.iec958.status[7] << 24);
	/*spin_lock_irq(&spdifinfo->reg_lock);*/
	change = ((hdmiinfo.ch_status0 != ch_status0_temp) ||
		 (hdmiinfo.ch_status1 != ch_status1_temp));
	hdmiinfo.ch_status0 = ch_status0_temp;
	hdmiinfo.ch_status1 = ch_status1_temp;
	pr_debug("%s() ch0=0x%x, ch1=0x%x, change=%d\n",
		 __func__, hdmiinfo.ch_status0, hdmiinfo.ch_status1, change);
	if (change) {
		afe_msk_write(AFE_IEC_CHL_STAT0, hdmiinfo.ch_status0,
			      0xffffffff);
		afe_msk_write(AFE_IEC_CHR_STAT0, hdmiinfo.ch_status0,
			      0xffffffff);
		afe_msk_write(AFE_IEC_CHL_STAT1, hdmiinfo.ch_status1,
			      0x0000ffff);
		afe_msk_write(AFE_IEC_CHR_STAT1, hdmiinfo.ch_status1,
			      0x0000ffff);
	} else {
		pr_err("%s() hdmi no change\n", __func__);
	}
	/*spin_unlock_irq(&spdifinfo->reg_lock);*/
	return change;
}

struct hwgain_info {
	u32 hgctl_0;
	u32 hgctl_1;
};

struct hwgain_info hwgaininfo;

static int hwgain_control_info(struct snd_kcontrol *kcontrol,
			       struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 2;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 0x80000;
	return 0;
}

static int hwgain_control_get(struct snd_kcontrol *kcontrol,
			      struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = hwgaininfo.hgctl_0;
	ucontrol->value.integer.value[1] = hwgaininfo.hgctl_1;
	return 0;
}

/*
*   hgctl0
*     bit0: hwgainID 0:AFE_HWGAIN_1, 1:AFE_HWGAIN_2
*     bit1: hwgain: enable/disable
*     bit2~bit9: hwgainsample per step (0~255)
*     bit10~bit11: hwgainsetpdb: 0:0.125dB, 1:0.25dB, 2:0.5dB
*
*   hgctl1
*     hwgain: 0x0[-inf.dB]~0x80000[0dB]
*/
static int hwgain_control_put(struct snd_kcontrol *kcontrol,
			      struct snd_ctl_elem_value *ucontrol)
{
	u32 hgctl_0 = ucontrol->value.integer.value[0];
	u32 hgctl_1 = ucontrol->value.integer.value[1];
	enum afe_hwgain_id hgid = hgctl_0 & 0x1;
	int hgenable = (hgctl_0 & 0x2) >> 1;

	afe_hwgain_configurate(hgctl_0, hgctl_1);
	afe_hwgain_enable(hgid, hgenable);
	hwgaininfo.hgctl_0 = hgctl_0;
	hwgaininfo.hgctl_1 = hgctl_1;
	return 0;
}

static int loopbackcmd;

static int loopback_control_info(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_BOOLEAN;
	uinfo->count = 1;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 1;
	return 0;
}

static int loopback_control_get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = loopbackcmd;

	return 0;
}

static int loopback_control_put(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	loopbackcmd = ucontrol->value.integer.value[0];
	pr_debug("%s() loopbackcmd : %d\n", __func__, loopbackcmd);
	afe_loopback_set(loopbackcmd);
	return 0;
}

static int sram_enable_get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = afe_sram_enable;
	pr_notice("%s Mch-I2S memif output sram enable:%d\n", __func__, afe_sram_enable);

	return 0;
}

static int sram_enable_put(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	afe_sram_enable = ucontrol->value.integer.value[0];
	pr_notice("%s Mch-I2S memif output sram enable is set to:%d\n", __func__, afe_sram_enable);

	return 0;
}


static const struct snd_kcontrol_new mt8521p_soc_controls[] = {
	{
		.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
		.name = "SPDIF In",
		.info = spdif_rx_info,
		.get = spdif_rx_get,
		.put = spdif_rx_put
	},
	{
		.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
		.name = "Master Volume",
		.index = 0,
		.info = dummy_speaker_info_volume,
		.get = dummy_speaker_get_volume,
		.put = dummy_speaker_put_volume,
		.private_value = 2
	},
	{
		.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
		.name = "Master Switch",
		.info = dummy_speaker_info_switch,
		.get = dummy_speaker_get_switch,
		.put = dummy_speaker_put_switch
	},
	{
		.iface = SNDRV_CTL_ELEM_IFACE_PCM,
		.name = SNDRV_CTL_NAME_IEC958("spdif-set-chst ", PLAYBACK,
					      DEFAULT),
		.info = spdif_out_info,
		.get = spdif_out_get,
		.put = spdif_out_put
	},
	{
		.iface = SNDRV_CTL_ELEM_IFACE_PCM,
		.name = SNDRV_CTL_NAME_IEC958("hdmi-set-chst ", PLAYBACK,
					      DEFAULT),
		.info = hdmi_out_info,
		.get = hdmi_out_get,
		.put = hdmi_out_put
	},
	{
		.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
		.name = "HWGAIN",
		.info = hwgain_control_info,
		.get = hwgain_control_get,
		.put = hwgain_control_put
	},
	{
		.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
		.name = "Audio Loopback",
		.info = loopback_control_info,
		.get = loopback_control_get,
		.put = loopback_control_put
	},
	SOC_SINGLE_BOOL_EXT("SRAM Enable",
			    0,
			    sram_enable_get,
			    sram_enable_put),
};

