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

#ifndef _MT8521P_PRIVATE_H_
#define _MT8521P_PRIVATE_H_

#if defined(CONFIG_HAS_EARLYSUSPEND)
#include <linux/earlysuspend.h>
#endif
#include <sound/soc.h>
#include "mt8521p-afe.h"


enum mt_stream_id {
	/* playback streams */
	MT_STREAM_DL1,
	MT_STREAM_DL2,
	MT_STREAM_DL3,
	MT_STREAM_DL4,
	MT_STREAM_DL5,
	MT_STREAM_DL6,
	MT_STREAM_DLM,
	MT_STREAM_ARB1,
	MT_STREAM_DSDR,
	MT_STREAM_8CH_I2S_OUT,
	MT_STREAM_IEC1,
	MT_STREAM_IEC2,
	/* capture streams */
	MT_STREAM_UL1,
	MT_STREAM_UL2,
	MT_STREAM_UL3,
	MT_STREAM_UL4,
	MT_STREAM_UL5,
	MT_STREAM_UL6,
	MT_STREAM_DAI,
	MT_STREAM_MOD_PCM,
	MT_STREAM_AWB,
	MT_STREAM_AWB2,
	MT_STREAM_DSDW,
	MT_STREAM_MULTIIN,
	MT_STREAM_NUM
};

struct mt_irq;

struct mt_stream {
	enum mt_stream_id id;
	const char *name;
	struct snd_pcm_ops *ops;
	int occupied;
	u32 pointer;
	struct snd_pcm_substream *substream;
	struct mt_irq *irq;
	int use_i2s_slave_clock;
	int use_sram;
};

struct mt_irq {
	enum audio_irq_id id;
	struct mt_stream *s;
	void (*isr)(struct mt_stream *s);
};

struct mt_dai {
	struct mt_stream *s;
	itrcon_action itrcon;
};

struct mt_private {
	struct mt_stream streams[MT_STREAM_NUM];
	struct mt_irq irqs[IRQ_NUM];
	struct mt_dai dais[MT_DAI_NUM][2];
#if defined(CONFIG_HAS_EARLYSUSPEND)
	struct early_suspend es;
#endif
	int has_es;
};

#endif
