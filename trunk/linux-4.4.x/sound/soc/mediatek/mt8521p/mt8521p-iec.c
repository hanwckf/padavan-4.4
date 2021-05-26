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

static const struct snd_pcm_hardware iec_hardware = {
	.info = (SNDRV_PCM_INFO_MMAP
	       | SNDRV_PCM_INFO_INTERLEAVED
	       | SNDRV_PCM_INFO_RESUME
	       | SNDRV_PCM_INFO_MMAP_VALID),
	.formats = (SNDRV_PCM_FMTBIT_S16_LE
		  | SNDRV_PCM_FMTBIT_S24_3LE
		  | SNDRV_PCM_FMTBIT_IEC958_SUBFRAME_LE),
	.rates = (SNDRV_PCM_RATE_32000
		| SNDRV_PCM_RATE_44100
		| SNDRV_PCM_RATE_48000
		| SNDRV_PCM_RATE_88200
		| SNDRV_PCM_RATE_96000
		| SNDRV_PCM_RATE_176400
		| SNDRV_PCM_RATE_192000),
	.rate_min = 32000,
	.rate_max = 768000,
	.channels_min = 2,
	.channels_max = 2,
	.buffer_bytes_max = (1024 * 1024 * 16),
	.period_bytes_min = (2 * 1024),
	.period_bytes_max = (64 * 1024),
	.periods_min = 2,
	.periods_max = 256,
	.fifo_size = 0,
};

static void iec_isr(struct mt_stream *s)
{
	u32 nsadr;

	if (unlikely(!s))
		return;
	if (unlikely(s->id != MT_STREAM_IEC1 && s->id != MT_STREAM_IEC2)) {
		pr_debug("%s() invalid stream id %d\n", __func__, s->id);
		return;
	}
	if (!afe_iec_burst_info_is_ready(s->id)) {
		pr_debug("%s() burst info is not ready\n", __func__);
		return;
	}
	nsadr = afe_iec_nsadr(s->id);
	nsadr += afe_iec_burst_len(s->id);
	if (nsadr >= afe_spdif_end(s->id))
		nsadr = afe_spdif_base(s->id);
	afe_iec_set_nsadr(s->id, nsadr);
	afe_iec_burst_info_clear_ready(s->id);
	snd_pcm_period_elapsed(s->substream);
}

static int iec_open(struct snd_pcm_substream *substream)
{
	int ret = 0;
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct mt_stream *s = runtime->private_data;

	if (unlikely(!s))
		return -EINVAL;
	snd_soc_set_runtime_hwparams(substream, &iec_hardware);
	/* Ensure that buffer size is a multiple of period size */
	ret = snd_pcm_hw_constraint_integer(runtime, SNDRV_PCM_HW_PARAM_PERIODS);
	if (ret < 0) {
		pr_err("%s() error: snd_pcm_hw_constraint_integer fail %d\n", __func__, ret);
		return -EINVAL;
	}
	afe_iec_power_on(s->id, 1);
	return ret;
}

static int iec_close(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct mt_stream *s = runtime->private_data;

	if (unlikely(!s))
		return -EINVAL;
	afe_iec_power_on(s->id, 0);
	return 0;
}

static int iec_hw_params(struct snd_pcm_substream *substream,
				  struct snd_pcm_hw_params *hw_params)
{
	int ret = 0;
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct mt_stream *s = runtime->private_data;
	struct snd_dma_buffer *dma_buf = &substream->dma_buffer;
	unsigned int rate = params_rate(hw_params);

	dma_buf->dev.type = SNDRV_DMA_TYPE_DEV;
	dma_buf->dev.dev = substream->pcm->card->dev;
	ret = snd_pcm_lib_malloc_pages(substream, params_buffer_bytes(hw_params));
	if (ret < 0) {
		pr_err("%s() error: snd_pcm_lib_malloc_pages fail %d\n", __func__, ret);
		return ret;
	}
	if (unlikely(!s))
		return -EINVAL;
	afe_iec_clock_on(s->id, 128 * rate);
	return ret;
}

static int iec_hw_free(struct snd_pcm_substream *substream)
{
	struct mt_stream *s = substream->runtime->private_data;

	if (unlikely(!s))
		return -EINVAL;
	afe_iec_clock_off(s->id);
	return snd_pcm_lib_free_pages(substream);
}

static int iec_prepare(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct mt_stream *s = runtime->private_data;

	pr_debug("%s() format=%d, rate=%u, channels=%u, sample_bits=%u\n",
		__func__, runtime->format, runtime->rate, runtime->channels, runtime->sample_bits);
	pr_debug("%s() period_size=%lu, periods=%u, buffer_size=%lu\n",
		__func__, runtime->period_size, runtime->periods, runtime->buffer_size);
	pr_debug("%s() dma_addr=0x%llx, dma_area=0x%p, dma_bytes=%zu\n",
		__func__, (unsigned long long)runtime->dma_addr, runtime->dma_area, runtime->dma_bytes);
	if (frames_to_bytes(runtime, runtime->buffer_size) % 16 != 0) {
		pr_err("%s() buffer-size is not multiple of 16 bytes\n", __func__);
		return -EINVAL;
	}
	if (unlikely(!s))
		return -EINVAL;
	s->pointer = 0;
	afe_iec_configurate(s->id, 0, runtime);
	return 0;
}

static int iec_trigger(struct snd_pcm_substream *substream, int cmd)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct mt_stream *s = runtime->private_data;

	if (unlikely(!s))
		return -EINVAL;
	pr_debug("%s() cmd=%d\n", __func__, cmd);
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
		afe_iec_enable(s->id);
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
		afe_iec_disable(s->id);
		break;
	default:
		pr_err("%s() invalid cmd\n", __func__);
		return -EINVAL;
	}
	return 0;
}

static snd_pcm_uframes_t iec_pointer(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct mt_stream *s = runtime->private_data;
	snd_pcm_uframes_t offset;
	u32 consumed, hw_offset, hw_cur;

	hw_cur = afe_spdif_cur(s->id);
	if (unlikely(hw_cur == 0))
		hw_cur = runtime->dma_addr;
	hw_offset = hw_cur - runtime->dma_addr;
	if (hw_offset > s->pointer)
		consumed = hw_offset - s->pointer;
	else
		consumed = runtime->dma_bytes + hw_offset - s->pointer;
	s->pointer += consumed;
	s->pointer %= runtime->dma_bytes;

	offset = bytes_to_frames(runtime, s->pointer);
	if (unlikely(offset >= runtime->buffer_size))
		offset = 0;
	return offset;
}

static struct snd_pcm_ops iec_ops = {
	.open = iec_open,
	.close = iec_close,
	.hw_params = iec_hw_params,
	.hw_free = iec_hw_free,
	.prepare = iec_prepare,
	.trigger = iec_trigger,
	.pointer = iec_pointer
};
