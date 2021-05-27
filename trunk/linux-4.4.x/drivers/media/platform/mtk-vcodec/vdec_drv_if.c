/*
 * Copyright (c) 2016 MediaTek Inc.
 * Author: PC Chen <pc.chen@mediatek.com>
 *         Tiffany Lin <tiffany.lin@mediatek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/slab.h>

#include "vdec_drv_if.h"
#include "mtk_vcodec_dec.h"
#include "vdec_drv_base.h"
#include "mtk_vcodec_dec_pm.h"
#include "mtk_vcu.h"

struct vdec_common_if *get_dec_common_if(void);

int vdec_if_init(struct mtk_vcodec_ctx *ctx, unsigned int fourcc)
{
	int ret = 0;

	switch (fourcc) {
	case V4L2_PIX_FMT_H264:
	case V4L2_PIX_FMT_H265:
	case V4L2_PIX_FMT_MPEG1:
	case V4L2_PIX_FMT_MPEG2:
	case V4L2_PIX_FMT_MPEG4:
	case V4L2_PIX_FMT_H263:
	case V4L2_PIX_FMT_S263:
	case V4L2_PIX_FMT_XVID:
	case V4L2_PIX_FMT_DIVX3:
	case V4L2_PIX_FMT_DIVX4:
	case V4L2_PIX_FMT_DIVX5:
	case V4L2_PIX_FMT_DIVX6:
	case V4L2_PIX_FMT_VP8:
	case V4L2_PIX_FMT_VP9:
	case V4L2_PIX_FMT_WMV1:
	case V4L2_PIX_FMT_WMV2:
	case V4L2_PIX_FMT_WMV3:
	case V4L2_PIX_FMT_WVC1:
	case V4L2_PIX_FMT_WMVA:
	case V4L2_PIX_FMT_RV30:
	case V4L2_PIX_FMT_RV40:
		ctx->dec_if = get_dec_common_if();
		break;
	default:
		return -EINVAL;
	}

	mtk_vdec_lock(ctx);
	mtk_vcodec_dec_clock_on(&ctx->dev->pm);
	ret = ctx->dec_if->init(ctx, &ctx->drv_handle);
	mtk_vcodec_dec_clock_off(&ctx->dev->pm);
	mtk_vdec_unlock(ctx);

	return ret;
}

int vdec_if_decode(struct mtk_vcodec_ctx *ctx, struct mtk_vcodec_mem *bs,
		   struct vdec_fb *fb, bool *res_chg)
{
	int ret = 0;

	if (bs) {
		if ((bs->dma_addr & 63) != 0) {
			mtk_v4l2_err("bs dma_addr should 64 byte align");
			return -EINVAL;
		}
	}

	if (fb) {
		if (((fb->planes[0].dma_addr & 511) != 0) ||
		    ((fb->planes[1].dma_addr & 511) != 0)) {
			mtk_v4l2_err("frame buffer dma_addr should 512 byte align");
			return -EINVAL;
		}
	}

	if (ctx->drv_handle == 0)
		return -EIO;

	mtk_vdec_lock(ctx);

	mtk_vcodec_set_curr_ctx(ctx->dev, ctx);
	mtk_vcodec_dec_clock_on(&ctx->dev->pm);
	enable_irq(ctx->dev->dec_irq);
	ret = ctx->dec_if->decode(ctx->drv_handle, bs, fb, res_chg);
	disable_irq(ctx->dev->dec_irq);
	mtk_vcodec_dec_clock_off(&ctx->dev->pm);
	mtk_vcodec_set_curr_ctx(ctx->dev, NULL);

	mtk_vdec_unlock(ctx);

	return ret;
}

int vdec_if_get_param(struct mtk_vcodec_ctx *ctx, enum vdec_get_param_type type,
		      void *out)
{
	int ret = 0;

	if (ctx->drv_handle == 0)
		return -EIO;

	mtk_vdec_lock(ctx);
	ret = ctx->dec_if->get_param(ctx->drv_handle, type, out);
	mtk_vdec_unlock(ctx);

	return ret;
}

int vdec_if_set_param(struct mtk_vcodec_ctx *ctx, enum vdec_set_param_type type,
		      void *in)
{
	int ret = 0;

	mtk_vdec_lock(ctx);
	ret = ctx->dec_if->set_param(ctx->drv_handle, type, in);
	mtk_vdec_unlock(ctx);

	return ret;
}

void vdec_if_deinit(struct mtk_vcodec_ctx *ctx)
{
	if (ctx->drv_handle == 0)
		return;

	mtk_vdec_lock(ctx);
	mtk_vcodec_dec_clock_on(&ctx->dev->pm);
	ctx->dec_if->deinit(ctx->drv_handle);
	mtk_vcodec_dec_clock_off(&ctx->dev->pm);
	mtk_vdec_unlock(ctx);

	ctx->drv_handle = 0;
}

