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
#include <sound/soc.h>
#include <sound/pcm_params.h>
#include "mt8521p-aud-global.h"
#include "mt8521p-dai.h"
#include "mt8521p-afe.h"
#include "mt8521p-private.h"
#include "mt8521p-afe-reg.h"

static const struct snd_pcm_hardware hdmi_i2s_hardware = {
	.info = (SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_INTERLEAVED |
		 SNDRV_PCM_INFO_RESUME | SNDRV_PCM_INFO_MMAP_VALID),
	.formats = (SNDRV_PCM_FMTBIT_S32_LE | SNDRV_PCM_FMTBIT_S16_LE),
	.rates = (SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_44100 |
		  SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_88200 |
		  SNDRV_PCM_RATE_96000 | SNDRV_PCM_RATE_176400 |
		  SNDRV_PCM_RATE_192000),
	.rate_min = 32000,
	.rate_max = 192000,
	.channels_min = 2,
	.channels_max = 8,
	.buffer_bytes_max = (1024 * 1024 * 16),
	.period_bytes_max = (64 * 1024),
	.period_bytes_min = (2 * 1024),
	.periods_min = 2,
	.periods_max = 256,
	.fifo_size = 0,
};

static void hdmi_i2s_isr(struct mt_stream *s)
{
	if (likely(s))
		snd_pcm_period_elapsed(s->substream);
}

static int hdmi_i2s_open(struct snd_pcm_substream *substream)
{
	int ret;
	struct snd_pcm_runtime *runtime = substream->runtime;

	snd_soc_set_runtime_hwparams(substream, &hdmi_i2s_hardware);
	/* Ensure that buffer size is a multiple of period size */
	ret = snd_pcm_hw_constraint_integer(runtime,
					    SNDRV_PCM_HW_PARAM_PERIODS);
	if (ret < 0) {
		pr_err("%s() snd_pcm_hw_constraint_integer fail %d\n",
		       __func__, ret);
		return ret;
	}
	afe_msk_write(AUDIO_TOP_CON0, 0, PDN_HDMI_CK);
	return ret;
}

static int hdmi_i2s_close(struct snd_pcm_substream *substream)
{
	afe_msk_write(AUDIO_TOP_CON0, PDN_HDMI_CK, PDN_HDMI_CK);
	return 0;
}

static int hdmi_i2s_hw_params(struct snd_pcm_substream *substream,
			      struct snd_pcm_hw_params *hw_params)
{
	int ret;
	struct snd_dma_buffer *dma_buf = &substream->dma_buffer;
	unsigned int rate = params_rate(hw_params);
	unsigned int channels = params_channels(hw_params);

	dma_buf->dev.type = SNDRV_DMA_TYPE_DEV;
	dma_buf->dev.dev = substream->pcm->card->dev;
	ret = snd_pcm_lib_malloc_pages(substream,
				       params_buffer_bytes(hw_params));
	if (ret < 0) {
		pr_err("%s() snd_pcm_lib_malloc_pages fail %d\n",
		       __func__, ret);
		return ret;
	}
	afe_hdmi_i2s_clock_on(rate, 128 * rate, PCM_OUTPUT_32BIT, 0);
	afe_hdmi_i2s_set_interconnection(channels, 1);
	return ret;
}

static int hdmi_i2s_hw_free(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;

	afe_hdmi_i2s_set_interconnection(runtime->channels, 0);
	afe_hdmi_i2s_clock_off();
	return snd_pcm_lib_free_pages(substream);
}

static int hdmi_i2s_prepare(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct mt_stream *s = runtime->private_data;
	enum afe_sampling_rate fs = fs_enum(runtime->rate);

	pr_debug("%s() format=%d, rate=%u, channels=%u\n"
		 "period_size=%lu, periods=%u, buffer_size=%lu\n"
		 "dma_addr=0x%llx, dma_area=0x%p, dma_bytes=%zu\n",
		 __func__, runtime->format, runtime->rate, runtime->channels,
		 runtime->period_size, runtime->periods, runtime->buffer_size,
		 (unsigned long long)runtime->dma_addr, runtime->dma_area,
		 runtime->dma_bytes);
	if (frames_to_bytes(runtime, runtime->buffer_size) % 16 != 0) {
		pr_err("%s()  buffer-size is not multiple of 16 bytes\n",
		       __func__);
		return -EINVAL;
	}

	s->pointer = 0;
	init_hdmi_dma_buffer(runtime->dma_addr, runtime->dma_bytes);

	switch (runtime->format) {
	case SNDRV_PCM_FORMAT_S16_LE:
		set_hdmi_out_control(runtime->channels, HDMI_OUT_BIT_WIDTH_16);
		set_hdmi_i2s();
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
	case SNDRV_PCM_FORMAT_S32_LE:
		set_hdmi_out_control(runtime->channels, HDMI_OUT_BIT_WIDTH_32);
		set_hdmi_i2s();
		break;
	case SNDRV_PCM_FORMAT_DSD_U8:
		set_hdmi_out_dsd_control(runtime->channels, HDMI_OUT_DSD_8BIT);
		set_hdmi_i2s_dsd();
		break;
	case SNDRV_PCM_FORMAT_DSD_U16_LE:
		set_hdmi_out_dsd_control(runtime->channels, HDMI_OUT_DSD_16BIT);
		set_hdmi_i2s_dsd();
		break;
	default:
		pr_err("%s() invalid format %d\n", __func__, runtime->format);
		break;
	}

	if (s->irq) {
		struct audio_irq_config config = {
			.mode = fs,
			.init_val = runtime->period_size
		};

		audio_irq_configurate(s->irq->id, &config);
	}
	return 0;
}

static int hdmi_i2s_trigger(struct snd_pcm_substream *substream, int cmd)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct mt_stream *s = runtime->private_data;

	pr_debug("%s() cmd=%d\n", __func__, cmd);
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
		set_hdmi_out_control_enable(true);
		set_hdmi_i2s_enable(true);
		if (s->irq)
			audio_irq_enable(s->irq->id, 1);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
		if (s->irq) {
			enum audio_irq_id irq_id = s->irq->id;

			audio_irq_enable(irq_id, 0);
			/*
			 * To make sure the interrupt signal is disabled,
			 * we must clear the status again after disabling irq.
			 */
			if (irq_id >= IRQ_ASYS_IRQ1)
				asys_irq_clear(0x1 << (irq_id - IRQ_ASYS_IRQ1));
			else
				afe_irq_clear(0x1 << (irq_id - IRQ_AFE_IRQ1));
		}
		set_hdmi_i2s_enable(false);
		set_hdmi_out_control_enable(false);
		break;
	default:
		pr_err("%s() invalid cmd\n", __func__);
		return -EINVAL;
	}
	return 0;
}

static snd_pcm_uframes_t hdmi_i2s_pointer(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct mt_stream *s = runtime->private_data;
	snd_pcm_uframes_t offset;
	u32 consumed, hw_offset, hw_cur;

	hw_cur = ((afe_read(AFE_HDMI_OUT_CON0) & HDMI_OUT_ON_MASK) ==
		  HDMI_OUT_ON) ? afe_read(AFE_HDMI_OUT_CUR) : 0;
	if (unlikely(hw_cur == 0))
		hw_cur = runtime->dma_addr;
	hw_offset = hw_cur - runtime->dma_addr;
	if (hw_offset > s->pointer)
		consumed = hw_offset - s->pointer;
	else
		consumed = runtime->dma_bytes + hw_offset - s->pointer;
	if ((consumed & 0x1f) != 0)
		pr_err("%s() DMA address is not aligned 32 bytes\n", __func__);
	s->pointer += consumed;
	s->pointer %= runtime->dma_bytes;

	offset = bytes_to_frames(runtime, s->pointer);
	if (unlikely(offset >= runtime->buffer_size))
		offset = 0;
	return offset;
}

static struct snd_pcm_ops hdmi_i2s_ops = {
	.open = hdmi_i2s_open,
	.close = hdmi_i2s_close,
	.hw_params = hdmi_i2s_hw_params,
	.hw_free = hdmi_i2s_hw_free,
	.prepare = hdmi_i2s_prepare,
	.trigger = hdmi_i2s_trigger,
	.pointer = hdmi_i2s_pointer
};
