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

#include <linux/dma-mapping.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <sound/soc.h>
#include <sound/pcm_params.h>
#include <linux/io.h>
#include "mt8521p-aud-global.h"
#ifdef CONFIG_MTK_LEGACY_CLOCK
#include <mach/mt_clkmgr.h>
#else
#include "mt8521p-afe-clk.h"
#endif
#include "mt8521p-afe-reg.h"
#include "mt8521p-afe.h"

static void *bitstream_sram;
static u32 hw_offset;
struct snd_pcm_substream *lp_substream;

#define BITSTREAM_SRAM_ADDR_PHYS ((u32)0x11340000)
#define BITSTREAM_SRAM_SIZE ((u32)128 * 1024)

static void enter_region(volatile struct lp_region *r, enum lp_cpu_id id)
{
	enum lp_cpu_id other = 1 - id;

	r->interested[id] = 1;
	r->turn = id;
	while (r->turn == id && r->interested[other])
		pr_debug("[AP]%s() waiting ...\n", __func__);
}

static void leave_region(volatile struct lp_region *r, enum lp_cpu_id id)
{
	r->interested[id] = 0;
}

/* extern to deep-idle */
int lp_switch_mode(unsigned int mode)
{
	volatile struct lp_mode *m;

	if (!bitstream_sram)
		return -ENODEV;
	m = &((volatile struct lp_info *)bitstream_sram)->m;
	pr_debug("%s() enter mode=%u\n", __func__, mode);
	enter_region(&m->region, LP_CPU_AP);
	m->mode = mode;
	leave_region(&m->region, LP_CPU_AP);
	pr_debug("%s() leave\n", __func__);
	return 0;
}

static const struct snd_pcm_hardware mt8521p_lp_pcm_hardware = {
	.info             = SNDRV_PCM_INFO_INTERLEAVED,
	.formats          = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S32_LE,
	.period_bytes_min = 3072,
	.period_bytes_max = 3072 * 10,
	.periods_min      = 8,
	.periods_max      = 1024,
	.buffer_bytes_max = 2 * 1024 * 1024,
};

static int mt8521p_lp_pcm_open(struct snd_pcm_substream *substream)
{
	volatile struct lp_info *lp;

	pr_debug("%s()\n", __func__);
	substream->runtime->private_data = bitstream_sram;
	lp = substream->runtime->private_data;
	snd_soc_set_runtime_hwparams(substream, &mt8521p_lp_pcm_hardware);
	afe_power_mode(LOW_POWER_MODE);
	lp_substream = substream;
	return 0;
}

static int mt8521p_lp_pcm_close(struct snd_pcm_substream *substream)
{
	pr_debug("%s()\n", __func__);
	lp_substream = NULL;
	afe_power_mode(NORMAL_POWER_MODE);
	return 0;
}

static int mt8521p_lp_pcm_hw_params(struct snd_pcm_substream *substream,
				    struct snd_pcm_hw_params *params)
{
	int ret;

	ret = snd_pcm_lib_malloc_pages(substream, params_buffer_bytes(params));
	if (ret < 0) {
		pr_err("%s() error: allocation of memory failed\n", __func__);
		return ret;
	}
	return 0;
}

static int mt8521p_lp_pcm_hw_free(struct snd_pcm_substream *substream)
{
	pr_debug("%s()\n", __func__);
	return snd_pcm_lib_free_pages(substream);
}

static int mt8521p_lp_pcm_prepare(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;

	pr_debug("%s() period_size=%u, periods=%u, buffer_size=%u\n",
		 __func__, (unsigned int)runtime->period_size,
		 runtime->periods, (unsigned int)runtime->buffer_size);

	lp_configurate((volatile struct lp_info *)runtime->private_data,
		       runtime->dma_addr,
		       frames_to_bytes(runtime, runtime->buffer_size),
		       runtime->rate,
		       runtime->channels,
		       snd_pcm_format_width(runtime->format));
	{
		struct audio_irq_config config = {
			.mode = FS_48000HZ, /* fs_enum(runtime->rate), */
			/* about 10ms */
			.init_val = 480  /* runtime->buffer_size * 3 / 4 */
		};

		audio_irq_configurate(IRQ_AFE_IRQ1, &config);
	}
	return 0;
}

static int mt8521p_lp_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
	volatile struct lp_info *lp =
	(volatile struct lp_info *)substream->runtime->private_data;
	unsigned int lp_cmd[2][2] = {
		[SNDRV_PCM_STREAM_PLAYBACK] = {
			[SNDRV_PCM_TRIGGER_STOP] = LP_CMD_PLAYBACK_STOP,
			[SNDRV_PCM_TRIGGER_START] = LP_CMD_PLAYBACK_START
		},
		[SNDRV_PCM_STREAM_CAPTURE] = {
			[SNDRV_PCM_TRIGGER_STOP] = LP_CMD_CAPTURE_STOP,
			[SNDRV_PCM_TRIGGER_START] = LP_CMD_CAPTURE_START
		}
	};

	pr_debug("%s()\n", __func__);
	if (cmd > SNDRV_PCM_TRIGGER_START)
		return -EINVAL;
	audio_irq_enable(IRQ_AFE_IRQ1, cmd == SNDRV_PCM_TRIGGER_START ? 1 : 0);
	return lp_cmd_excute(lp, lp_cmd[substream->stream][cmd]);
}

static snd_pcm_uframes_t mt8521p_lp_pcm_pointer(
		struct snd_pcm_substream *substream)
{
	snd_pcm_uframes_t offset;
	struct snd_pcm_runtime *runtime = substream->runtime;

	offset = bytes_to_frames(runtime, hw_offset);
	if (unlikely(offset >= runtime->buffer_size))
		offset = 0;
	return offset;
}

void lp_audio_isr(void)
{
	if (!IS_ERR_OR_NULL(lp_substream)) {
		struct snd_pcm_runtime *runtime = lp_substream->runtime;

		volatile struct lp_info *lp =
			(volatile struct lp_info *)runtime->private_data;

		hw_offset = lp_hw_offset(lp);

		enter_region(&lp->buf.appl_region, LP_CPU_AP);
		lp->buf.appl_ofs = frames_to_bytes(runtime,
			runtime->control->appl_ptr % runtime->buffer_size);
		if (lp->buf.appl_ofs == 0) {
			lp->buf.appl_is_in_buf_end =
			(runtime->control->appl_ptr ==
			 runtime->hw_ptr_base) ? 0 : 1;
		}
		leave_region(&lp->buf.appl_region, LP_CPU_AP);

		snd_pcm_period_elapsed(lp_substream);
	}
}

static struct snd_pcm_ops mt8521p_lp_pcm_ops = {
	.open = mt8521p_lp_pcm_open,
	.close = mt8521p_lp_pcm_close,
	.ioctl = snd_pcm_lib_ioctl,
	.hw_params = mt8521p_lp_pcm_hw_params,
	.hw_free = mt8521p_lp_pcm_hw_free,
	.prepare = mt8521p_lp_pcm_prepare,
	.trigger = mt8521p_lp_pcm_trigger,
	.pointer = mt8521p_lp_pcm_pointer,
};

static int mt8521p_lp_pcm_new(struct snd_soc_pcm_runtime *rtd)
{
	pr_debug("%s()\n", __func__);
	return snd_pcm_lib_preallocate_pages_for_all(
		rtd->pcm,
		SNDRV_DMA_TYPE_DEV,
		NULL,
		2 * 1024 * 1024,
		2 * 1024 * 1024);
}

static void mt8521p_lp_pcm_free(struct snd_pcm *pcm)
{
	pr_debug("%s()\n", __func__);
	snd_pcm_lib_preallocate_free_for_all(pcm);
}

static int mt8521p_lp_pcm_probe(struct snd_soc_platform *platform)
{
	pr_debug("%s()\n", __func__);
	bitstream_sram = ioremap(BITSTREAM_SRAM_ADDR_PHYS,
				 BITSTREAM_SRAM_SIZE);
	return 0;
}

static int mt8521p_lp_pcm_remove(struct snd_soc_platform *platform)
{
	pr_debug("%s()\n", __func__);
	iounmap(bitstream_sram);
	bitstream_sram = NULL;
	return 0;
}

static struct snd_soc_platform_driver mt8521p_lp_soc_platform_driver = {
	.probe = mt8521p_lp_pcm_probe,
	.remove = mt8521p_lp_pcm_remove,
	.pcm_new = mt8521p_lp_pcm_new,
	.pcm_free = mt8521p_lp_pcm_free,
	.ops = &mt8521p_lp_pcm_ops,
};

#define CLK_CMSYS_SEL_MASK         (0xF << 16)
#define CLK_CMSYS_SEL_CLK26M       (0x0 << 16)  /* 26MHz */
#define CLK_CMSYS_SEL_UNIVPLL1_D2  (0x2 << 16)  /* 312MHz */
#define CLK_CMSYS_SEL_SYSPLL_D5    (0x4 << 16)  /* 218.4MHz */

static int mt8521p_lp_audio_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;

	pr_debug("%s()\n", __func__);
	#ifdef CONFIG_OF
	if (dev->of_node) {
		dev_set_name(dev, "%s", "mt8521p-lp-audio");
		pr_notice("%s set dev name %s\n", __func__, dev_name(dev));
	}
	#endif
	#ifdef CONFIG_MTK_LEGACY_CLOCK
	enable_pll(UNIVPLL, "AUDIO"); /* for CM4 */
	#else
	mt_afe_unipll_clk_on();
	#endif
	cmsys_write(0, 0x6);
	cmsys_write(8, 0x14);

	#ifdef CONFIG_MTK_LEGACY_CLOCK
	phy_msk_write(CLK_CFG_14, CLK_CMSYS_SEL_UNIVPLL1_D2,
		      CLK_CMSYS_SEL_MASK);
	#else
	/* todo */
	#endif

	cmsys_write(8, 0x15);
	cmsys_write(0, 0x7);

	return snd_soc_register_platform(&pdev->dev,
					 &mt8521p_lp_soc_platform_driver);
}

static int mt8521p_lp_audio_remove(struct platform_device *pdev)
{
	pr_debug("%s()\n", __func__);
	snd_soc_unregister_platform(&pdev->dev);

	cmsys_write(8, 0x14);
	cmsys_write(0, 0x6);

	#ifdef CONFIG_MTK_LEGACY_CLOCK
	disable_pll(UNIVPLL, "AUDIO"); /* for CM4 */
	#else
	mt_afe_unipll_clk_off();
	#endif
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id mt8521p_lp_audio_dt_match[] = {
	{.compatible = "mediatek,mt8521p-lp-audio",},
	{}
};
#endif
static struct platform_driver mt8521p_lp_audio = {
	.driver = {
		.name = "mt8521p-lp-audio",
		.owner = THIS_MODULE,
		#ifdef CONFIG_OF
		.of_match_table = mt8521p_lp_audio_dt_match,
		#endif
		},
	.probe = mt8521p_lp_audio_probe,
	.remove = mt8521p_lp_audio_remove
};

module_platform_driver(mt8521p_lp_audio);

MODULE_DESCRIPTION("mt8521p low-power audio driver");
MODULE_LICENSE("GPL");
