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
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/ioctl.h>
#include <linux/signal.h>
#include <sound/memalloc.h>
#include "mt8521p-aud-global.h"
#ifdef CONFIG_MTK_LEGACY_CLOCK
#include <mach/mt_clkmgr.h>
#endif
#include <linux/bitops.h>
#include "mt8521p-afe.h"
#include "mt8521p-afe-clk.h"

#define ASRC_CMD_TRIGGER         _IOW(0, 0, int)
#define ASRC_CMD_CHANNELS        _IOW(0, 1, int)
#define ASRC_CMD_INPUT_FREQ      _IOW(0, 2, int)
#define ASRC_CMD_INPUT_BITWIDTH  _IOW(0, 3, int)
#define ASRC_CMD_OUTPUT_FREQ     _IOW(0, 4, int)
#define ASRC_CMD_OUTPUT_BITWIDTH _IOW(0, 5, int)
#define ASRC_CMD_IS_DRAIN        _IOR(0, 6, int)
#define ASRC_CMD_SIGNAL_REASON   _IOR(0, 7, int)
#define ASRC_CMD_INPUT_BUFSIZE   _IOW(0, 8, int)
#define ASRC_CMD_OUTPUT_BUFSIZE  _IOW(0, 9, int)

#define ASRC_SIGNAL_REASON_IBUF_EMPTY  (0)
#define ASRC_SIGNAL_REASON_OBUF_FULL   (1)

#define ASRC_BUFFER_SIZE_MAX   (1 * 1024 * 1024)
#define INPUT  (0)
#define OUTPUT (1)
static struct snd_dma_buffer asrc_buffers[MEM_ASRC_NUM][2];

static int asrc_allocate_buffer(enum afe_mem_asrc_id id, int dir, size_t bytes)
{
	int ret;

	if (unlikely(id >= MEM_ASRC_NUM))
		return -EINVAL;
	if (unlikely(dir != INPUT && dir != OUTPUT))
		return -EINVAL;
	ret = snd_dma_alloc_pages(SNDRV_DMA_TYPE_DEV, 0, bytes, &asrc_buffers[id][dir]);
	if (ret < 0) {
		pr_err("%s() error: snd_dma_alloc_pages failed for asrc[%d]'s %s_buffer, ret=%d\n",
			__func__, id, dir ? "output" : "input", ret);
	}
	return ret;
}

static void asrc_free_buffer(enum afe_mem_asrc_id id, int dir)
{
	struct snd_dma_buffer *b;

	if (unlikely(id >= MEM_ASRC_NUM))
		return;
	if (unlikely(dir != INPUT && dir != OUTPUT))
		return;
	b = &asrc_buffers[id][dir];
	snd_dma_free_pages(b);
	memset(b, 0, sizeof(struct snd_dma_buffer));
}

struct asrc_buf {
	unsigned char *area;	/* virtual pointer */
	dma_addr_t addr;	/* physical address */
	size_t bytes;		/* buffer size in bytes */
};

struct asrc_private {
	enum afe_mem_asrc_id id;
	int running;
	int stereo;
	struct asrc_buf input_dmab;
	u32 input_freq;
	u32 input_bitwidth;
	int i_empty_cleared;
	int i_amount_cleared;
	struct asrc_buf output_dmab;
	u32 output_freq;
	u32 output_bitwidth;
	int o_full_cleared;
	int o_amount_cleared;
	struct fasync_struct *fasync_queue;
	unsigned long signal_reason;
};

#ifdef CONFIG_MTK_LEGACY_CLOCK
static const enum cg_clk_id asrc_clks[MEM_ASRC_NUM] = {
	MT_CG_AUDIO_MEM_ASRC1, /*PWR2_TOP_CON[10]*/
	MT_CG_AUDIO_MEM_ASRC2, /*PWR2_TOP_CON[11]*/
	MT_CG_AUDIO_MEM_ASRC3, /*PWR2_TOP_CON[12]*/
	MT_CG_AUDIO_MEM_ASRC4, /*PWR2_TOP_CON[13]*/
	MT_CG_AUDIO_MEM_ASRC5, /*PWR2_TOP_CON[14]*/
};
#endif

static size_t ibuf_hw_avail(enum afe_mem_asrc_id id, size_t buf_size)
{
	u32 rp, wp;

	rp = afe_mem_asrc_get_ibuf_rp(id);
	wp = afe_mem_asrc_get_ibuf_wp(id);
	if (wp >= rp)
		return wp - rp;
	else
		return buf_size + wp - rp;
}

static size_t obuf_hw_avail(enum afe_mem_asrc_id id, size_t buf_size)
{
	u32 rp, wp;

	rp = afe_mem_asrc_get_obuf_rp(id);
	wp = afe_mem_asrc_get_obuf_wp(id);
	if (rp >= wp)
		return rp - wp;
	else
		return buf_size + rp - wp;
}

static irqreturn_t asrc_isr(int irq, void *dev)
{
	struct asrc_private *priv = dev;
	enum afe_mem_asrc_id id = priv->id;
	u32 status = afe_mem_asrc_irq_status(id);

	pr_debug("---------INTERRUPT--------- ibuf_hw_avail=0x%x, obuf_hw_avail=0x%x\n",
		ibuf_hw_avail(id, priv->input_dmab.bytes),
		obuf_hw_avail(id, priv->output_dmab.bytes));
#if IBUF_AMOUNT_NOTIFY
	if (afe_mem_asrc_irq_is_enabled(id, IBUF_AMOUNT_INT) && (status & IBUF_AMOUNT_INT)) {
		if (!priv->i_amount_cleared) {
			pr_debug("clear IBUF_AMOUNT_INT\n");
			afe_mem_asrc_irq_clear(id, IBUF_AMOUNT_INT);
			priv->i_amount_cleared = 1;
		} else {
			afe_mem_asrc_irq_enable(id, IBUF_AMOUNT_INT, 0);
			pr_debug("notify IBUF_AMOUNT_INT\n");
			priv->i_amount_cleared = 0;
			set_bit(ASRC_SIGNAL_REASON_IBUF_EMPTY, &priv->signal_reason);
			if (priv->fasync_queue)
				kill_fasync(&priv->fasync_queue, SIGIO, POLL_OUT);
		}
	}
#endif
	if (afe_mem_asrc_irq_is_enabled(id, IBUF_EMPTY_INT) && (status & IBUF_EMPTY_INT)) {
		if (!priv->i_empty_cleared) {
			pr_debug("clear IBUF_EMPTY_INT\n");
			afe_mem_asrc_irq_clear(id, IBUF_EMPTY_INT);
			priv->i_empty_cleared = 1;
		} else {
		afe_mem_asrc_irq_enable(id, IBUF_EMPTY_INT, 0);
			pr_debug("notify IBUF_EMPTY_INT\n");
			priv->i_empty_cleared = 0;
			set_bit(ASRC_SIGNAL_REASON_IBUF_EMPTY, &priv->signal_reason);
			if (priv->fasync_queue)
				kill_fasync(&priv->fasync_queue, SIGIO, POLL_OUT);
		}
	}
	if (afe_mem_asrc_irq_is_enabled(id, OBUF_OV_INT) && (status & OBUF_OV_INT)) {
		if (!priv->o_full_cleared) {
			pr_debug("clear OBUF_OV_INT\n");
			afe_mem_asrc_irq_clear(id, OBUF_OV_INT);
			priv->o_full_cleared = 1;
		} else {
		afe_mem_asrc_irq_enable(id, OBUF_OV_INT, 0);
			pr_debug("notify OBUF_OV_INT\n");
			priv->o_full_cleared = 0;
			set_bit(ASRC_SIGNAL_REASON_OBUF_FULL, &priv->signal_reason);
			if (priv->fasync_queue)
				kill_fasync(&priv->fasync_queue, SIGIO, POLL_IN);
		}
	}
#if OBUF_AMOUNT_NOTIFY
	if (afe_mem_asrc_irq_is_enabled(id, OBUF_AMOUNT_INT) && (status & OBUF_AMOUNT_INT)) {
		if (!priv->o_amount_cleared) {
			pr_debug("clear OBUF_AMOUNT_INT\n");
			afe_mem_asrc_irq_clear(id, OBUF_AMOUNT_INT);
			priv->o_amount_cleared = 1;
		} else {
			afe_mem_asrc_irq_enable(id, OBUF_AMOUNT_INT, 0);
			pr_debug("notify OBUF_AMOUNT_INT\n");
			priv->o_amount_cleared = 0;
			set_bit(ASRC_SIGNAL_REASON_OBUF_FULL, &priv->signal_reason);
			if (priv->fasync_queue)
				kill_fasync(&priv->fasync_queue, SIGIO, POLL_IN);
		}
	}
#endif
	return IRQ_HANDLED;

}

static int asrc_fasync(int fd, struct file *filp, int on)
{
	struct asrc_private *priv = filp->private_data;

	if (!priv)
		return -EFAULT;
	pr_debug("%s()\n", __func__);
	return fasync_helper(fd, filp, on, &priv->fasync_queue);
}

static enum afe_mem_asrc_id get_asrc_id(const struct file *filp);

static int asrc_open(struct inode *node, struct file *filp)
{
	static const char *isr_names[MEM_ASRC_NUM] = {
		"asrc1-isr", "asrc2-isr", "asrc3-isr", "asrc4-isr", "asrc5-isr"
	};
	struct asrc_private *priv;
	enum afe_mem_asrc_id id = get_asrc_id(filp);

	pr_debug("%s() id=%u\n", __func__, id);
	if (id >= MEM_ASRC_NUM) {
		pr_err("%s() error: invalid id %u\n", __func__, id);
		goto open_error;
	}
	priv = kzalloc(sizeof(struct asrc_private), GFP_KERNEL);
	/*
	* Possible unnecessary 'out of memory' message
	* if (!priv) {
	*	pr_err("%s() error: kzalloc asrc_private failed\n", __func__);
	*	goto open_error;
	* }
	*/
	if (afe_mem_asrc_register_irq(id, asrc_isr, isr_names[id], priv) != 0)
		goto register_irq_error;
	priv->id = id;
	filp->private_data = priv;
#ifdef CONFIG_MTK_LEGACY_CLOCK
	enable_pll(UNIVPLL, "AUDIO");
	enable_clock(MT_CG_AUDIO_ASRC_BRG, "AUDIO"); /*PWR2_TOP_CON[16]*/
	enable_clock(asrc_clks[id], "AUDIO");
#else
	mt_afe_unipll_clk_on();
	afe_power_on_mem_asrc_brg(1);
	afe_power_on_mem_asrc(id, 1);
#endif
	return 0;
register_irq_error:
	kzfree(priv);
open_error:
	return -EINVAL;
}

static int asrc_release(struct inode *node, struct file *filp)
{
	enum afe_mem_asrc_id id;
	struct asrc_private *priv = filp->private_data;

	pr_debug("%s()\n", __func__);
	if (!priv)
		return -ENOMEM;
	id = priv->id;
	if (id >= MEM_ASRC_NUM) {
		pr_err("%s() error: invalid id %u\n", __func__, id);
		return -EINVAL;
	}
	afe_mem_asrc_enable(id, 0);
#ifdef CONFIG_MTK_LEGACY_CLOCK
	disable_clock(asrc_clks[id], "AUDIO");
	disable_clock(MT_CG_AUDIO_ASRC_BRG, "AUDIO");
	disable_pll(UNIVPLL, "AUDIO");
#else
	afe_power_on_mem_asrc(id, 0);
	afe_power_on_mem_asrc_brg(0);
	mt_afe_unipll_clk_off();
#endif
	afe_mem_asrc_unregister_irq(id, priv);
	asrc_fasync(-1, filp, 0);
	kzfree(priv);
	filp->private_data = NULL;
	return 0;
}

static int is_ibuf_empty(enum afe_mem_asrc_id id)
{
	u32 wp, rp;

	wp = afe_mem_asrc_get_ibuf_wp(id);
	rp = afe_mem_asrc_get_ibuf_rp(id);
	return wp == rp;
}

static int is_obuf_empty(enum afe_mem_asrc_id id, size_t bytes)
{
	u32 wp, rp;

	wp = afe_mem_asrc_get_obuf_wp(id);
	rp = afe_mem_asrc_get_obuf_rp(id);
	if (wp > rp)
		return wp - rp == 16;
	else
		return wp + bytes - rp == 16;
}

static long asrc_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct asrc_private *priv = filp->private_data;
	enum afe_mem_asrc_id id;

	pr_debug("%s() cmd=0x%08x, arg=0x%lx\n", __func__, cmd, arg);
	if (!priv)
		return -ENOMEM;
	id = priv->id;
	if (id >= MEM_ASRC_NUM) {
		pr_err("%s() error: invalid id %u\n", __func__, id);
		return -EINVAL;
	}
	switch (cmd) {
	case ASRC_CMD_CHANNELS:
		priv->stereo = (arg == 1) ? 0 : 1;
		break;
	case ASRC_CMD_INPUT_FREQ:
		priv->input_freq = arg;
		afe_mem_asrc_set_ibuf_freq(id, arg);
		break;
	case ASRC_CMD_INPUT_BITWIDTH:
		priv->input_bitwidth = arg;
		break;
	case ASRC_CMD_INPUT_BUFSIZE: {
		struct snd_dma_buffer *b;

		if (priv->running) {
			pr_err("%s() error: can't set input buffer size while running\n", __func__);
			return -EPERM;
		}
		b = &asrc_buffers[id][INPUT];
		priv->input_dmab.area = b->area;
		priv->input_dmab.addr = b->addr;
		if (arg > b->bytes) {
			pr_err("%s() error: too large input buffer size 0x%lx (MAX=0x%x)\n", __func__, arg, b->bytes);
			priv->input_dmab.bytes = b->bytes;
			return -ENOMEM;
		}
		priv->input_dmab.bytes = arg;
		break;
	}
	case ASRC_CMD_OUTPUT_FREQ:
		priv->output_freq = arg;
		afe_mem_asrc_set_obuf_freq(id, arg);
		break;
	case ASRC_CMD_OUTPUT_BITWIDTH:
		priv->output_bitwidth = arg;
		break;
	case ASRC_CMD_OUTPUT_BUFSIZE: {
		struct snd_dma_buffer *b;

		if (priv->running) {
			pr_err("%s() error: can't set output buffer size while running\n", __func__);
			return -EPERM;
		}
		b = &asrc_buffers[id][OUTPUT];
		priv->output_dmab.area = b->area;
		priv->output_dmab.addr = b->addr;
		if (arg > b->bytes) {
			pr_err("%s() error: too large output buffer size 0x%lx (MAX=0x%x)\n", __func__, arg, b->bytes);
			priv->output_dmab.bytes = b->bytes;
			return -ENOMEM;
		}
		priv->output_dmab.bytes = arg;
		break;
	}
	case ASRC_CMD_TRIGGER: {
		int ret;

		if (arg) {
			struct afe_mem_asrc_config config = {
				.input_buffer = {
					.base = priv->input_dmab.addr,
					.size = priv->input_dmab.bytes,
					.freq = priv->input_freq,
					.bitwidth = priv->input_bitwidth
				},
				.output_buffer = {
					.base = priv->output_dmab.addr,
					.size = priv->output_dmab.bytes,
					.freq = priv->output_freq,
					.bitwidth = priv->output_bitwidth
				},
				.stereo = priv->stereo,
				.tracking_mode = MEM_ASRC_NO_TRACKING
			};
			ret = afe_mem_asrc_configurate(id, &config);
			if (ret != 0) {
				pr_err("%s() error: configurate asrc return %d\n", __func__, ret);
				return -EINVAL;
			}
			ret = afe_mem_asrc_enable(id, 1);
			if (ret != 0) {
				pr_err("%s() error: enable asrc return %d\n", __func__, ret);
				return -EINVAL;
			}
			priv->running = 1;
		} else {
			priv->running = 0;
			ret = afe_mem_asrc_enable(id, 0);
			if (ret != 0) {
				pr_err("%s() error: disable asrc return %d\n", __func__, ret);
				return -EINVAL;
			}
		}
		break;
	}
	case ASRC_CMD_IS_DRAIN: {
		int is_drain;

		if (!arg)
			return -EFAULT;
		if (priv->running)
			is_drain = is_ibuf_empty(id) && is_obuf_empty(id, priv->output_dmab.bytes);
		else
			is_drain = 1;
		if (copy_to_user((void __user *)(arg), (void *)(&is_drain), sizeof(is_drain)) != 0)
			return -EFAULT;
		break;
	}
	case ASRC_CMD_SIGNAL_REASON: {
		unsigned long reason;

		if (!arg)
			return -EFAULT;
		reason = (test_and_clear_bit(ASRC_SIGNAL_REASON_IBUF_EMPTY, &priv->signal_reason)
			  << ASRC_SIGNAL_REASON_IBUF_EMPTY)
			 | (test_and_clear_bit(ASRC_SIGNAL_REASON_OBUF_FULL, &priv->signal_reason)
			    << ASRC_SIGNAL_REASON_OBUF_FULL);
		if (copy_to_user((void __user *)(arg), (void *)(&reason), sizeof(reason)) != 0)
			return -EFAULT;
		break;
	}
	default:
		pr_err("%s() error: unknown asrc cmd 0x%08x\n", __func__, cmd);
		return -EINVAL;
	}
	return 0;
}

static ssize_t asrc_read(struct file *filp, char __user *buf, size_t count, loff_t *offset)
{
	struct asrc_private *priv = filp->private_data;
	enum afe_mem_asrc_id id;
	struct asrc_buf *dmab;
	u32 wp, rp, copy;
	unsigned char *copy_start;

	pr_debug("%s()\n", __func__);
	if (unlikely(!priv))
		return 0;
	id = priv->id;
	if (unlikely(id >= MEM_ASRC_NUM)) {
		pr_err("%s() error: invalid id %u\n", __func__, id);
		return 0;
	}
	if (!priv->running) {
		pr_warn("%s() warning: asrc[%u] is not running\n", __func__, id);
		return 0;
	}
	dmab = &priv->output_dmab;
	wp = afe_mem_asrc_get_obuf_wp(id);
	rp = afe_mem_asrc_get_obuf_rp(id);
	copy_start = dmab->area + (rp - dmab->addr);
	count = count / 16 * 16;
	/* pr_debug("%s() wp=0x%08x, rp=0x%08x\n", __func__, wp, rp); */
	if (rp < wp) {
		u32 delta = wp - rp;

		copy = delta - 16 < count ? delta - 16 : count;
		if (copy == 0)
			return 0;

		if (copy_to_user(buf, copy_start, copy) != 0) {
			pr_err("%s() error: L%d copy_to_user\n", __func__, __LINE__);
			return 0;
		}
		afe_mem_asrc_set_obuf_rp(id, rp + copy);
		afe_mem_asrc_irq_enable(id,
			OBUF_OV_INT
#if OBUF_AMOUNT_NOTIFY
			| OBUF_AMOUNT_INT
#endif
			, 1);
		pr_debug("%s() copy=%u (line %d)\n", __func__, copy, __LINE__);
	} else {		/* rp >= wp */
		u32 delta = wp + dmab->bytes - rp;

		copy = delta - 16 < count ? delta - 16 : count;
		if (copy == 0)
			return 0;
		if (rp + copy < dmab->addr + dmab->bytes) {
			if (copy_to_user(buf, copy_start, copy) != 0) {
				pr_err("%s() error: L%d copy_to_user\n", __func__, __LINE__);
				return 0;
			}
			afe_mem_asrc_set_obuf_rp(id, rp + copy);
			afe_mem_asrc_irq_enable(id,
				OBUF_OV_INT
#if OBUF_AMOUNT_NOTIFY
				| OBUF_AMOUNT_INT
#endif
				, 1);
			pr_debug("%s() copy=%u (line %d)\n", __func__, copy, __LINE__);

		} else {
			u32 s1 = dmab->addr + dmab->bytes - rp;
			u32 s2 = copy - s1;

			if (copy_to_user(buf, copy_start, s1) != 0) {
				pr_err("%s() error: L%d copy_to_user\n", __func__, __LINE__);
				return 0;
			}
			if (s2) {
				if (copy_to_user(buf + s1, dmab->area, s2) != 0) {
					pr_err("%s() error: L%d copy_to_user\n", __func__,
					       __LINE__);
					afe_mem_asrc_set_obuf_rp(id, dmab->addr);
					afe_mem_asrc_irq_enable(id,
						OBUF_OV_INT
#if OBUF_AMOUNT_NOTIFY
						| OBUF_AMOUNT_INT
#endif
						, 1);
					pr_debug("%s() s1=%u (line %d)\n", __func__, s1, __LINE__);
					return s1;
				}
			}
			afe_mem_asrc_set_obuf_rp(id, dmab->addr + s2);
			afe_mem_asrc_irq_enable(id,
				OBUF_OV_INT
#if OBUF_AMOUNT_NOTIFY
				| OBUF_AMOUNT_INT
#endif
				, 1);
			pr_debug("%s() copy=%u (line %d)\n", __func__, copy, __LINE__);
		}
	}
	return copy;
}

static ssize_t asrc_write(struct file *filp, const char __user *buf, size_t count, loff_t *offset)
{
	struct asrc_private *priv = filp->private_data;
	enum afe_mem_asrc_id id;
	struct asrc_buf *dmab;
	u32 wp, rp, copy;
	unsigned char *copy_start;

	pr_debug("%s()\n", __func__);
	if (unlikely(!priv))
		return 0;
	id = priv->id;
	if (unlikely(id >= MEM_ASRC_NUM)) {
		pr_err("%s() error: invalid id %u\n", __func__, id);
		return 0;
	}
	if (!priv->running) {
		pr_warn("%s() warning: asrc[%u] is not running\n", __func__, id);
		return 0;
	}
	dmab = &priv->input_dmab;
	wp = afe_mem_asrc_get_ibuf_wp(id);
	rp = afe_mem_asrc_get_ibuf_rp(id);
	/* pr_debug("%s() wp=0x%08x, rp=0x%08x\n", __func__, wp, rp); */
	copy_start = dmab->area + (wp - dmab->addr);
	count = count / 16 * 16;
	if (wp < rp) {
		u32 delta = rp - wp;

		copy = delta - 16 < count ? delta - 16 : count;
		if (copy == 0)
			return 0;
		if (copy_from_user(copy_start, buf, copy) != 0) {
			pr_err("%s() error: L%d copy_from_user\n", __func__, __LINE__);
			return 0;
		}
		afe_mem_asrc_set_ibuf_wp(id, wp + copy);
		afe_mem_asrc_irq_enable(id,
			IBUF_EMPTY_INT
#if IBUF_AMOUNT_NOTIFY
			| IBUF_AMOUNT_INT
#endif
			, 1);
		pr_debug("%s() copy=%u (line %d)\n", __func__, copy, __LINE__);
	} else {		/* wp >= rp */
		u32 delta = rp + dmab->bytes - wp;

		copy = delta - 16 < count ? delta - 16 : count;
		if (copy == 0)
			return 0;
		if (wp + copy < dmab->addr + dmab->bytes) {
			if (copy_from_user(copy_start, buf, copy) != 0) {
				pr_err("%s() error: L%d copy_from_user\n", __func__, __LINE__);
				return 0;
			}
			afe_mem_asrc_set_ibuf_wp(id, wp + copy);
			afe_mem_asrc_irq_enable(id,
				IBUF_EMPTY_INT
#if IBUF_AMOUNT_NOTIFY
				| IBUF_AMOUNT_INT
#endif
				, 1);
			pr_debug("%s() copy=%u (line %d)\n", __func__, copy, __LINE__);
		} else {
			u32 s1 = dmab->addr + dmab->bytes - wp;
			u32 s2 = copy - s1;

			if (copy_from_user(copy_start, buf, s1) != 0) {
				pr_err("%s() error: L%d copy_from_user\n", __func__, __LINE__);
				return 0;
			}
			if (s2) {
				if (copy_from_user(dmab->area, buf + s1, s2) != 0) {
					pr_err("%s() error: L%d copy_from_user\n", __func__, __LINE__);
					afe_mem_asrc_set_ibuf_wp(id, dmab->addr);
					afe_mem_asrc_irq_enable(id,
						IBUF_EMPTY_INT
#if IBUF_AMOUNT_NOTIFY
						| IBUF_AMOUNT_INT
#endif
						, 1);
					pr_debug("%s() s1=%u (line %d)\n", __func__, s1, __LINE__);
					return s1;
				}
			}
			afe_mem_asrc_set_ibuf_wp(id, dmab->addr + s2);
			afe_mem_asrc_irq_enable(id,
				IBUF_EMPTY_INT
#if IBUF_AMOUNT_NOTIFY
				| IBUF_AMOUNT_INT
#endif
				, 1);
			pr_debug("%s() copy=%u (line %d)\n", __func__, copy, __LINE__);
		}
	}
	return copy;
}

static const struct file_operations asrc_fops[MEM_ASRC_NUM] = {
	[MEM_ASRC_1 ... MEM_ASRC_5] = {
		.owner = THIS_MODULE,
		.open = asrc_open,
		.release = asrc_release,
		.unlocked_ioctl = asrc_ioctl,
		.write = asrc_write,
		.read = asrc_read,
		.flush = NULL,
		.fasync = asrc_fasync,
		.mmap = NULL
	}
};

static enum afe_mem_asrc_id get_asrc_id(const struct file *filp)
{
	if (filp) {
		enum afe_mem_asrc_id id;

		for (id = MEM_ASRC_1; id < MEM_ASRC_NUM; ++id) {
			if (filp->f_op == &asrc_fops[id])
				return id;
		}
	}
	return MEM_ASRC_NUM;
}

static struct miscdevice asrc_devices[MEM_ASRC_NUM] = {
	[MEM_ASRC_1] = {
		.minor = MISC_DYNAMIC_MINOR,
		.name = "asrc1",
		.fops = &asrc_fops[MEM_ASRC_1],
	},
	[MEM_ASRC_2] = {
		.minor = MISC_DYNAMIC_MINOR,
		.name = "asrc2",
		.fops = &asrc_fops[MEM_ASRC_2],
	},
	[MEM_ASRC_3] = {
		.minor = MISC_DYNAMIC_MINOR,
		.name = "asrc3",
		.fops = &asrc_fops[MEM_ASRC_3],
	},
	[MEM_ASRC_4] = {
		.minor = MISC_DYNAMIC_MINOR,
		.name = "asrc4",
		.fops = &asrc_fops[MEM_ASRC_4],
	},
	[MEM_ASRC_5] = {
		.minor = MISC_DYNAMIC_MINOR,
		.name = "asrc5",
		.fops = &asrc_fops[MEM_ASRC_5],
	}
};

static int asrc_mod_init(void)
{
	int ret;
	enum afe_mem_asrc_id id;

	pr_debug("%s()\n", __func__);
	for (id = MEM_ASRC_1; id < MEM_ASRC_NUM; ++id) {
		ret = misc_register(&asrc_devices[id]);
		if (ret)
			pr_err("%s() error: misc_register asrc[%u] failed %d\n", __func__, id, ret);
		else {
			ret = asrc_allocate_buffer(id, INPUT, ASRC_BUFFER_SIZE_MAX);
			if (ret != 0)
				continue;
			ret = asrc_allocate_buffer(id, OUTPUT, ASRC_BUFFER_SIZE_MAX);
			if (ret != 0)
				continue;
		}
	}
	return 0;
}

static void asrc_mod_exit(void)
{
	enum afe_mem_asrc_id id;

	pr_debug("%s()\n", __func__);
	for (id = MEM_ASRC_1; id < MEM_ASRC_NUM; ++id) {
		misc_deregister(&asrc_devices[id]);
		asrc_free_buffer(id, INPUT);
		asrc_free_buffer(id, OUTPUT);
	}
}
module_init(asrc_mod_init);
module_exit(asrc_mod_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("memory asrc driver");
