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
#include <linux/i2c.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
/*#include <mach/mt_gpio.h>*/
#include <linux/delay.h>

struct i2c_client *i2c_client_point;
u8 txbuf[1];
u8 rxbuf[1];

static int i2c_write_bytes(struct i2c_client *client, u16 addr, u8 *txbuf, int len)
{
	u8 buffer[8];
	u16 offset = 0;
	struct i2c_msg msg = {
		.addr = 0x4c,
		.flags = 0,
		.buf = buffer,
	};

	if (txbuf == NULL)
		return -1;
	buffer[0] = addr;
	memcpy(&buffer[1], &txbuf[offset], len);
	msg.len = len + 1;
	if (i2c_transfer(client->adapter, &msg, 1) < 0) {
		pr_err("I2C write 0x%X%X length=%d failed\n", buffer[0], buffer[1], len);
		return -1;
	}
	return 0;
}

#if 0
static int i2c_read_bytes(struct i2c_client *client, u16 addr, u8 *rxbuf, int len)
{
	u8 buffer[2];
	u16 left = len;
	u16 offset = 0;
	struct i2c_msg msg[2] = {
		{
			.addr = 0x4c,
			.flags = 0,
			.len = left,
			.buf = buffer,
		},
		{
			.addr = 0x4c,
			.flags = I2C_M_RD,
		},
	};

	if (rxbuf == NULL)
		return -1;
	buffer[0] = addr;
	msg[1].buf = &rxbuf[offset];
	msg[1].len = left;
	if (i2c_transfer(client->adapter, &msg[0], 2) != 2) {
		pr_err("I2C read 0x%X length=%d failed", addr + offset, len);
		return -1;
	}
	return 0;
}
#endif

static int pcm1795_dai_set_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	int ret = 0;

	pr_debug("%s() cpu_dai id = %d, fmt = 0x%x, rate = %d\n", __func__, dai->id, fmt, dai->rate);
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		snd_soc_update_bits(dai->codec, 18, 0x7 << 4, 0x4 << 4);
		snd_soc_update_bits(dai->codec, 20, 0x1 << 5, 0x0 << 5);
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
		snd_soc_update_bits(dai->codec, 18, 0x7 << 4, 0x1 << 4);
		snd_soc_update_bits(dai->codec, 20, 0x1 << 5, 0x0 << 5);
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		snd_soc_update_bits(dai->codec, 18, 0x7 << 4, 0x3 << 4);
		snd_soc_update_bits(dai->codec, 20, 0x1 << 5, 0x0 << 5);
		break;
	case SND_SOC_DAIFMT_PDM:
		txbuf[0] = 0x22; /*  when fs=176400 dsd  mode 128 times, or when fs = 88200 dsd 64 times 0x20 */
		pr_debug("%s() set pcm1795 register adapter = %p\n", __func__,
			 i2c_client_point->adapter);
		ret = i2c_write_bytes(i2c_client_point, 20, txbuf, 1);
		if (ret < 0)
			pr_err("%s() set pcm1795 register err\n", __func__);
		else
			pr_debug("%s() set pcm1795 register success\n", __func__);
#if 0				/* test code read pcm1795  register 20 value */
		ret = i2c_read_bytes(i2c_client_point, 20, rxbuf, 1);
		if (ret < 0)
			pr_debug("%s() set pcm1795 register err\n", __func__);
		pr_debug("%s() rxbuf = 0x%x\n", __func__, rxbuf[0]);
#endif
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static struct snd_soc_dai_ops pcm1795_dai_ops = {
	.set_fmt = pcm1795_dai_set_fmt,
};

static int pcm1795_probe(struct snd_soc_codec *codec)
{
	return 0;
}

static struct snd_soc_codec_driver pcm1795_driver = {
	.probe = pcm1795_probe,
};

static struct snd_soc_dai_driver pcm1795_dai_driver = {
	.name = "pcm1795-i2s",
	.playback = {
		.stream_name = "pcm1795-i2s-playback",
		.channels_min = 2,
		.channels_max = 2,
		.rates = SNDRV_PCM_RATE_8000_192000,
		.formats = (SNDRV_PCM_FMTBIT_S16_LE
		| SNDRV_PCM_FMTBIT_S32_LE
		| SNDRV_PCM_FMTBIT_DSD_U8 | SNDRV_PCM_FMTBIT_DSD_U16_LE),
	},
	.ops = &pcm1795_dai_ops,
};

static int pcm1795_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret;

	pr_debug("%s() client->dev's name is %s\n", __func__, dev_name(&client->dev));
	ret = snd_soc_register_codec(&client->dev, &pcm1795_driver, &pcm1795_dai_driver, 1);
	if (ret < 0) {
		pr_err("%s() call snd_soc_register_codec() return = %d\n", __func__, ret);
		return -1;
	}
	i2c_client_point = client;
	return ret;
}

static int pcm1795_i2c_remove(struct i2c_client *client)
{
	snd_soc_unregister_codec(&client->dev);
	return 0;
}

static const struct i2c_device_id pcm1795_i2c_id[] = {
	{"pcm1795", 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, pcm1795_i2c_id);

static struct i2c_driver pcm1795_i2c_driver = {
	.probe = pcm1795_i2c_probe,
	.remove = pcm1795_i2c_remove,

	.id_table = pcm1795_i2c_id,
	.driver = {
		.name = "pcm1795",
		.owner = THIS_MODULE,
	},
};

module_i2c_driver(pcm1795_i2c_driver);

MODULE_DESCRIPTION("ASoC PCM1795 codec driver");
MODULE_LICENSE("GPL");
