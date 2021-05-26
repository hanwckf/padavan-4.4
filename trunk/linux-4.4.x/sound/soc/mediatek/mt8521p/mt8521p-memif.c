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
#ifdef CONFIG_MTK_LEGACY_CLOCK
#include <mach/mt_clkmgr.h>
#endif
#include "mt8521p-dai.h"
#include "mt8521p-afe.h"
#include "mt8521p-private.h"
#include "mt8521p-afe-reg.h"

static const struct snd_pcm_hardware memif_hardware = {
	.info = SNDRV_PCM_INFO_MMAP
	      | SNDRV_PCM_INFO_INTERLEAVED
	      | SNDRV_PCM_INFO_RESUME
	      | SNDRV_PCM_INFO_MMAP_VALID,
	.formats = SNDRV_PCM_FMTBIT_S16_LE
		 | SNDRV_PCM_FMTBIT_S24_LE
		 | SNDRV_PCM_FMTBIT_S24_3LE
		 | SNDRV_PCM_FMTBIT_S32_LE,
	.period_bytes_min = 16,
	.period_bytes_max = 1024 * 256,
	.periods_min = 2,
	.periods_max = 1024,
	.buffer_bytes_max = 1 * 1024 * 1024,
	.fifo_size = 0,
};

static const struct snd_pcm_hardware memif_hardware_i2smx = {
	.info = SNDRV_PCM_INFO_INTERLEAVED
	      | SNDRV_PCM_INFO_RESUME,
	.formats = SNDRV_PCM_FMTBIT_S16_LE
		 | SNDRV_PCM_FMTBIT_S24_LE
		 | SNDRV_PCM_FMTBIT_S32_LE,
	.period_bytes_min = 16,
	.period_bytes_max = 1024 * 256 * 2,
	.periods_min = 2,
	.periods_max = 1024,
	.buffer_bytes_max = 1 * 1024 * 1024,
	.fifo_size = 0,
};

static enum afe_mem_interface get_memif(struct mt_stream *s)
{
	if (!s)
		return AFE_MEM_NONE;
	switch (s->id) {
	case MT_STREAM_DL1:
		return AFE_MEM_DL1;
	case MT_STREAM_DL2:
		return AFE_MEM_DL2;
	case MT_STREAM_DL3:
		return AFE_MEM_DL3;
	case MT_STREAM_DL4:
		return AFE_MEM_DL4;
	case MT_STREAM_DL5:
		return AFE_MEM_DL5;
	case MT_STREAM_DL6:
		return AFE_MEM_DL6;
	case MT_STREAM_DLM:
		return AFE_MEM_DLMCH;
	case MT_STREAM_UL1:
		return AFE_MEM_UL1;
	case MT_STREAM_UL2:
		return AFE_MEM_UL2;
	case MT_STREAM_UL3:
		return AFE_MEM_UL3;
	case MT_STREAM_UL4:
		return AFE_MEM_UL4;
	case MT_STREAM_UL5:
		return AFE_MEM_UL5;
	case MT_STREAM_UL6:
		return AFE_MEM_UL6;
	case MT_STREAM_ARB1:
		return AFE_MEM_ARB1;
	case MT_STREAM_DSDR:
		return AFE_MEM_DSDR;
	case MT_STREAM_DAI:
		return AFE_MEM_DAI;
	case MT_STREAM_MOD_PCM:
		return AFE_MEM_MOD_PCM;
	case MT_STREAM_AWB:
		return AFE_MEM_AWB;
	case MT_STREAM_AWB2:
		return AFE_MEM_AWB2;
	case MT_STREAM_DSDW:
		return AFE_MEM_DSDW;
	case MT_STREAM_MULTIIN:
		return AFE_MEM_ULMCH;
	default:
		return AFE_MEM_NONE;
	}
}

#ifdef CONFIG_MTK_LEGACY_CLOCK
static enum cg_clk_id get_clk(enum afe_mem_interface memif)
{
	switch (memif) {
	case AFE_MEM_DL1:
		return MT_CG_AUDIO_MMIF_DL1; /* AUDIO_TOP_CON5[6] */
	case AFE_MEM_DL2:
		return MT_CG_AUDIO_MMIF_DL2; /* AUDIO_TOP_CON5[7] */
	case AFE_MEM_DL3:
		return MT_CG_AUDIO_MMIF_DL3; /* AUDIO_TOP_CON5[8] */
	case AFE_MEM_DL4:
		return MT_CG_AUDIO_MMIF_DL4; /* AUDIO_TOP_CON5[9] */
	case AFE_MEM_DL5:
		return MT_CG_AUDIO_MMIF_DL5; /* AUDIO_TOP_CON5[10] */
	case AFE_MEM_DL6:
		return MT_CG_AUDIO_MMIF_DL6; /* AUDIO_TOP_CON5[11] */
	case AFE_MEM_DLMCH:
		return MT_CG_AUDIO_MMIF_DLMCH; /* AUDIO_TOP_CON5[12] */
	case AFE_MEM_ARB1:
		return MT_CG_AUDIO_MMIF_ARB1; /* AUDIO_TOP_CON5[13] */
	case AFE_MEM_UL1:
		return MT_CG_AUDIO_MMIF_UL1; /* AUDIO_TOP_CON5[0] */
	case AFE_MEM_UL2:
		return MT_CG_AUDIO_MMIF_UL2; /* AUDIO_TOP_CON5[1] */
	case AFE_MEM_UL3:
		return MT_CG_AUDIO_MMIF_UL3; /* AUDIO_TOP_CON5[2] */
	case AFE_MEM_UL4:
		return MT_CG_AUDIO_MMIF_UL4; /* AUDIO_TOP_CON5[3] */
	case AFE_MEM_UL5:
		return MT_CG_AUDIO_MMIF_UL5;/* AUDIO_TOP_CON5[4] */
	case AFE_MEM_UL6:
		return MT_CG_AUDIO_MMIF_UL6;/* AUDIO_TOP_CON5[5] */
	case AFE_MEM_DAI:
		return MT_CG_AUDIO_MMIF_DAI;/* AUDIO_TOP_CON5[16] */
	case AFE_MEM_AWB:
		return MT_CG_AUDIO_MMIF_AWB1;/* AUDIO_TOP_CON5[14] */
	case AFE_MEM_AWB2:
		return MT_CG_AUDIO_MMIF_AWB2;/* AUDIO_TOP_CON5[15] */
	case AFE_MEM_DSDR:
	case AFE_MEM_ULMCH:
	case AFE_MEM_DSDW:
	case AFE_MEM_MOD_PCM:
	default:
		return NR_CLKS;
	}
}
#endif

static int memif_open(struct snd_pcm_substream *substream)
{
	struct mt_stream *s = substream->runtime->private_data;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	enum afe_mem_interface memif = get_memif(s);
	/*
	 * [Programming Guide]
	 * disable memif power down
	 */
#ifdef CONFIG_MTK_LEGACY_CLOCK
	enum cg_clk_id clk = get_clk(memif);

	if (clk != NR_CLKS) {
		enable_clock(clk, "AUDIO");
		if (clk == MT_CG_AUDIO_MMIF_DLMCH) {
			enable_clock(MT_CG_AUDIO_MMIF_DL1, "AUDIO");/* AUDIO_TOP_CON5[6] */
			enable_clock(MT_CG_AUDIO_MMIF_DL2, "AUDIO");/* AUDIO_TOP_CON5[7] */
			enable_clock(MT_CG_AUDIO_MMIF_DL3, "AUDIO");/* AUDIO_TOP_CON5[8] */
			enable_clock(MT_CG_AUDIO_MMIF_DL4, "AUDIO");/* AUDIO_TOP_CON5[9] */
			enable_clock(MT_CG_AUDIO_MMIF_DL5, "AUDIO");/* AUDIO_TOP_CON5[10] */
		}
	}
#else
	memif_enable_clk(memif, 1);
#endif
	if (rtd->cpu_dai->id == MT_DAI_I2SMX_ID && substream->stream == SNDRV_PCM_STREAM_CAPTURE)
		snd_soc_set_runtime_hwparams(substream, &memif_hardware_i2smx);
	else
		snd_soc_set_runtime_hwparams(substream, &memif_hardware);
	return 0;
}

static int memif_close(struct snd_pcm_substream *substream)
{
	struct mt_stream *s = substream->runtime->private_data;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_pcm_runtime *runtime = substream->runtime;
	int memif_num = (runtime->channels + 1)/2;
	enum afe_mem_interface memif = get_memif(s);
	/*
	 * [Programming Guide]
	 * enab;e memif power down
	 */
#ifdef CONFIG_MTK_LEGACY_CLOCK
	enum cg_clk_id clk = get_clk(memif);

	if (clk != NR_CLKS) {
		disable_clock(clk, "AUDIO");
		if (clk == MT_CG_AUDIO_MMIF_DLMCH) {
			disable_clock(MT_CG_AUDIO_MMIF_DL1, "AUDIO");
			disable_clock(MT_CG_AUDIO_MMIF_DL2, "AUDIO");
			disable_clock(MT_CG_AUDIO_MMIF_DL3, "AUDIO");
			disable_clock(MT_CG_AUDIO_MMIF_DL4, "AUDIO");
			disable_clock(MT_CG_AUDIO_MMIF_DL5, "AUDIO");
		}
	}
#else
	if (rtd->cpu_dai->id == MT_DAI_I2SMX_ID && substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		enum afe_mem_interface tmp_memif;

		for (tmp_memif = AFE_MEM_UL1; tmp_memif < AFE_MEM_UL1 + memif_num; ++tmp_memif)
			memif_enable_clk(tmp_memif, 0);
	}
	memif_enable_clk(memif, 0);
#endif
	return 0;
}

static int memif_hw_params(struct snd_pcm_substream *substream, struct snd_pcm_hw_params *params)
{
	struct mt_stream *s = substream->runtime->private_data;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	unsigned int channels = params_channels(params);
	unsigned int buffer_bytes = params_buffer_bytes(params);
	int ret;

	pr_debug("%s()\n", __func__);
	if ((rtd->cpu_dai->id == MT_DAI_I2SMX_ID) && (channels%2)) {
		if (buffer_bytes % channels != 0) {
			pr_err("%s() error: buffer_bytes(0x%08x) is not multiple times of channels(%d)\n",
				__func__, buffer_bytes, channels);
			return -EINVAL;
		}
	}
	if (afe_sram_enable &&
		(rtd->cpu_dai->id == MT_DAI_I2SM_ID) &&
		(buffer_bytes < afe_sram_max_size)) {
		struct snd_dma_buffer *dma_buf = &substream->dma_buffer;

		dma_buf->dev.type = SNDRV_DMA_TYPE_DEV;
		dma_buf->dev.dev = substream->pcm->card->dev;
		dma_buf->area = (unsigned char *)afe_sram_address;
		dma_buf->addr = afe_sram_phy_address;
		dma_buf->bytes = buffer_bytes;
		snd_pcm_set_runtime_buffer(substream, dma_buf);
		s->use_sram = 1;
	} else {
		ret = snd_pcm_lib_malloc_pages(substream, params_buffer_bytes(params));
		if (ret < 0) {
			pr_err("%s() error: allocation of memory failed\n", __func__);
			return ret;
		}
		s->use_sram = 0;
	}

	return 0;
}

static int memif_hw_free(struct snd_pcm_substream *substream)
{
	struct mt_stream *s = substream->runtime->private_data;
	int ret = 0;
	pr_debug("%s()\n", __func__);
	if (s->use_sram)
		snd_pcm_set_runtime_buffer(substream, NULL);
	else
		ret = snd_pcm_lib_free_pages(substream);
	return ret;
}

static int memif_prepare(struct snd_pcm_substream *substream)
{
	int ret;
	struct mt_stream *s = substream->runtime->private_data;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	enum afe_mem_interface memif = get_memif(s);
	enum afe_sampling_rate fs;
	enum afe_channel_mode channel = STEREO;
	enum afe_dlmch_ch_num dlmch_ch_num = DLMCH_0CH;
	enum afe_dsd_width dsd_width = DSD_WIDTH_32BIT;
	enum afe_multilinein_chnum ch_num = AFE_MULTILINEIN_2CH;
	enum afe_i2s_format fmt = FMT_64CYCLE_16BIT_I2S;
	int hd_audio = 1;
	int dsd_mode = 0;
	struct snd_pcm_runtime *runtime = substream->runtime;
	int dai_id = rtd->cpu_dai->id;

	if (s->use_i2s_slave_clock) {
		if (dai_id >= MT_DAI_I2S1_ID && dai_id <= MT_DAI_I2S6_ID)
			fs = FS_I2S1 + (dai_id - MT_DAI_I2S1_ID);
		else
			fs = FS_I2S1;
	} else {
		fs = fs_enum(runtime->rate);
		/*afe_hwgain_gainmode_set(AFE_HWGAIN_1, fs);*/
	}

	channel = (runtime->channels == 1) ? MONO : STEREO;
	if (runtime->channels > 0 && runtime->channels <= 10)
		dlmch_ch_num = (enum afe_dlmch_ch_num)runtime->channels;
	else {
		pr_err("%s() error: unsupported channel %u\n", __func__, runtime->channels);
		return -EINVAL;
	}

	switch (runtime->format) {
	case SNDRV_PCM_FORMAT_S16_LE:
		hd_audio = 0;
		break;
	case SNDRV_PCM_FORMAT_S32_LE:
		hd_audio = 1;
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
		hd_audio = 1;
		break;
	case SNDRV_PCM_FORMAT_S24_3LE:
		hd_audio = 1;
		break;
	case SNDRV_PCM_FORMAT_DSD_U8:
		hd_audio = 1;
		dsd_width = DSD_WIDTH_8BIT;
		break;
	case SNDRV_PCM_FORMAT_DSD_U16_LE:
		hd_audio = 1;
		dsd_width = DSD_WIDTH_16BIT;
		break;
	default:
		pr_err("%s() error: unsupported format %d\n", __func__, runtime->format);
		return -EINVAL;
	}

	/* configurate memory interface */
	{
		int i;
		int memif_num = 0;
		ssize_t buffer_bytes = frames_to_bytes(runtime, runtime->buffer_size);
		enum afe_mem_interface memifs[AFE_MEM_NUM];

		if (dai_id == MT_DAI_I2SMX_ID) {
			if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
				memif_num = (runtime->channels + 1)/2;
				for (i = 0; i < memif_num; ++i)
					memifs[i] = AFE_MEM_UL1 + i;
				channel = STEREO;
				buffer_bytes = (buffer_bytes/runtime->channels)*2;
			}
		} else {
			memifs[0] = memif;
			memif_num = 1;
		}
		/*
		 * [Programming Guide]
		 * configurate each memif
		 */
		for (i = 0; i < memif_num; ++i) {
			struct afe_memif_config config = {
				.fs = fs,
				.hd_audio = (memif == AFE_MEM_ULMCH) ? 1 : hd_audio,
				.dsd_width = dsd_width,
				.first_bit = MSB_FIRST,
				.daimod_fs = DAIMOD_8000HZ,
				.channel = channel,
				.dlmch_ch_num = dlmch_ch_num,
				.mono_sel = MONO_USE_L,
				.dup_write = DUP_WR_DISABLE,
				.buffer = {
					.base = runtime->dma_addr + buffer_bytes*i,
					.size = buffer_bytes
				}
			};
			/*
			 * [Programming Guide]
			 * Uplink and downlink configuration
			 */
			ret = afe_memif_configurate(memifs[i], &config);
			if (ret < 0) {
				pr_err("%s() error: afe_memif_configurate return %d\n", __func__, ret);
				return ret;
			}
			if (memif == AFE_MEM_ULMCH) {
				memset(runtime->dma_area, 0, frames_to_bytes(runtime, runtime->buffer_size));
				if (runtime->format == SNDRV_PCM_FORMAT_DSD_U8) {
					dsd_mode = 1;
					fmt = FMT_64CYCLE_32BIT_I2S;
				}
				if (runtime->format == SNDRV_PCM_FORMAT_S16_LE)
					fmt = FMT_64CYCLE_16BIT_I2S;
				else if (runtime->format == SNDRV_PCM_FORMAT_S24_3LE)
					fmt = FMT_64CYCLE_32BIT_I2S;
				else if (runtime->format == SNDRV_PCM_FORMAT_S32_LE)
					fmt = FMT_64CYCLE_32BIT_I2S;
				if ((runtime->channels == 2) || (runtime->channels == 6) || (runtime->channels == 8)) {
					if (runtime->channels == 2)
						ch_num = AFE_MULTILINEIN_2CH;
					else if (runtime->channels == 6)
						ch_num = AFE_MULTILINEIN_6CH;
					else if (runtime->channels == 8)
						ch_num = AFE_MULTILINEIN_8CH;
				} else {
					pr_err("%s() error: multiline need set 2ch,6ch or 8ch mode!\n", __func__);
					return -EINVAL;
				}

				{
					struct afe_multilinein_config config = {
						.dsd_mode = dsd_mode,
						.endian = AFE_MULTILINEIN_LITTILE_ENDIAN,
						.fmt = fmt,
						.ch_num = ch_num,
						.intr_period = AFE_MULTILINEIN_INTR_PERIOD_256,
						.mss = AFE_MULTILINE_FROM_RX,
						.dsdWidth = dsd_width
					};
					/*
					 * [Programming Guide]
					 * [SPDIF IN]MPHONE_MULTI Config
					 */
					afe_multilinein_configurate(&config);
				}
			}
		}
	}
	s->pointer = 0;
	/* configurate irq */
	if (s->irq) {
		struct audio_irq_config config = {
			.mode = fs,
			.init_val = runtime->period_size
		};
		/*
		 * [Programming Guide]
		 * irq configuration
		 */
		audio_irq_configurate(s->irq->id, &config);
	}
	return 0;
}

static int memif_trigger(struct snd_pcm_substream *substream, int cmd)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct mt_stream *s = runtime->private_data;
	enum afe_mem_interface memif = get_memif(s);
	struct mt_irq *irq = s->irq;

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
		/* silence the buffer that has been processed by hw */
		runtime->silence_threshold = 0;
		runtime->silence_size = runtime->boundary;
		if (rtd->cpu_dai->id == MT_DAI_I2SMX_ID) {
			if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
				int memif_num = (runtime->channels + 1)/2;

				if (memif_num > 1) {
					int i;
					enum afe_mem_interface memifs[AFE_MEM_NUM];

					for (i = 0; i < memif_num; ++i)
						memifs[i] = AFE_MEM_UL1 + i;
					/*
					 * [Programming Guide]
					 * if multi-ch in, turn on memif UL2~ULx first
					 */
					afe_memifs_enable(memifs, memif_num, 1);
				}
			}
		}
		/*
		 * [Programming Guide]
		 * turn on memif
		 */
		afe_memif_enable(memif, 1);
		/*afe_hwgain_enable(AFE_HWGAIN_1, 1);*/
		if (irq)
			audio_irq_enable(irq->id, 1);
		return 0;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
		if (irq) {
			audio_irq_enable(irq->id, 0);
			/*
			 * To make sure the interrupt signal is disabled,
			 * we must clear the status again after disabling irq.
			 */
			if (irq->id >= IRQ_ASYS_IRQ1)
				asys_irq_clear(0x1 << (irq->id - IRQ_ASYS_IRQ1));
			else
				afe_irq_clear(0x1 << (irq->id - IRQ_AFE_IRQ1));
		}
		/*
		 * [Programming Guide]
		 * turn off memif
		 */
		afe_memif_enable(memif, 0);
		if (rtd->cpu_dai->id == MT_DAI_I2SMX_ID) {
			int memif_num = (runtime->channels + 1)/2;

			if (memif_num > 1) {
				int i;
				enum afe_mem_interface memifs[AFE_MEM_NUM];

				for (i = 0; i < memif_num; ++i)
					memifs[i] = AFE_MEM_UL1 + i;
				/*
				 * [Programming Guide]
				 * if multi-ch in, turn off memif UL2~ULx then
				 */
				afe_memifs_enable(memifs, memif_num, 0);
			}
		}

		/*afe_hwgain_enable(AFE_HWGAIN_1, 0);*/
		return 0;
	default:
		return -EINVAL;
	}
}

static snd_pcm_uframes_t memif_pointer(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	snd_pcm_uframes_t offset = 0;
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct mt_stream *s = runtime->private_data;
	enum afe_mem_interface memif = get_memif(s);
	u32 base, cur;
	/*
	 * [Programming Guide]
	 * [I2S] get memif current pointer, and should compute for multi-ch in
	 */
	if (afe_memif_base(memif, &base) == 0 && afe_memif_pointer(memif, &cur) == 0)
		s->pointer = cur - base;
	else
		s->pointer = 0;

	if (rtd->cpu_dai->id == MT_DAI_I2SMX_ID) {
		if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
			int channels = runtime->channels;
			int memif_num = (channels+1)/2;
			ssize_t buffer_bytes = frames_to_bytes(runtime, runtime->buffer_size);
			ssize_t memif_bytes = (buffer_bytes/memif_num) & (~(0xF));
			snd_pcm_uframes_t memif_size = bytes_to_frames(runtime, (memif_bytes)/2*channels);

			offset = bytes_to_frames(runtime, (s->pointer)/2*channels);
			if (unlikely(offset >= memif_size))
				offset = 0;
		}
	} else {
		offset = bytes_to_frames(runtime, s->pointer);
		if (unlikely(offset >= runtime->buffer_size))
			offset = 0;
	}
	return offset;
}

static int memif_copy(struct snd_pcm_substream *substream, int channel,
		    snd_pcm_uframes_t pos,
		    void __user *buf, snd_pcm_uframes_t count)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_pcm_runtime *runtime = substream->runtime;
	/*
	 * [Programming Guide]
	 * [I2S] if multi-ch in, copy data to user from each UL for full-interleave
	 */
	if (rtd->cpu_dai->id == MT_DAI_I2SMX_ID && substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		int f;
		int channels = runtime->channels;
		int memif_num = (channels + 1) / 2;
		ssize_t memif_bytes = frames_to_bytes(runtime, runtime->buffer_size) / channels * 2;
		ssize_t single_sample_size = samples_to_bytes(runtime, 1);
		ssize_t pair_sample_size = single_sample_size * 2;
		char *hwbuf = runtime->dma_area + frames_to_bytes(runtime, pos) / channels * 2;

		for (f = 0; f < count; ++f) {
			int m;
			char *hbuf = hwbuf + f * pair_sample_size;
			char __user *ubuf = buf + frames_to_bytes(runtime, f);

			for (m = 0; m < memif_num; ++m) {
				char *hb = hbuf + m * memif_bytes;
				char __user *ub = ubuf + m * pair_sample_size;

				if ((m == memif_num-1) && channels%2) {
					if (copy_to_user(ub, hb, single_sample_size))
						return -EINVAL;
				} else {
					if (copy_to_user(ub, hb, pair_sample_size))
						return -EINVAL;
				}
			}
		}
	} else {
		char *hwbuf = runtime->dma_area + frames_to_bytes(runtime, pos);

		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			if (copy_from_user(hwbuf, buf, frames_to_bytes(runtime, count)))
				return -EINVAL;
		} else {
			if (copy_to_user(buf, hwbuf, frames_to_bytes(runtime, count)))
				return -EINVAL;
		}
	}
	return 0;
}

static struct snd_pcm_ops memif_ops = {
	.open = memif_open,
	.close = memif_close,
	.hw_params = memif_hw_params,
	.hw_free = memif_hw_free,
	.prepare = memif_prepare,
	.trigger = memif_trigger,
	.pointer = memif_pointer,
	.copy = memif_copy,
};

/*
 * [Programming Guide]
 * the irq callback for all pcm read write, include spdif in ULMCH
 */
static void memif_isr(struct mt_stream *s)
{
	if (likely(s))
		snd_pcm_period_elapsed(s->substream);
}
