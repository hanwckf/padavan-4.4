/*
 * Copyright (c) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/errno.h>

#include "mtk_mdp_core.h"
#include "mtk_mdp_vpu.h"
#include "mtk_mdp_drm.h"
#include "mtk_mdp_type.h"
#include "uapi/drm/drm_fourcc.h"

static struct mtk_mdp_dev *g_mdp_dev;

static int mtk_mdp_map_drm_color_format(int drm_format)
{
	switch (drm_format) {
	case DRM_FORMAT_C8:
		return DP_COLOR_BAYER8;
	case DRM_FORMAT_RGB565:
		return DP_COLOR_BGR565;
	case DRM_FORMAT_BGR565:
		return DP_COLOR_RGB565;
	case DRM_FORMAT_RGB888:
		return DP_COLOR_BGR888;
	case DRM_FORMAT_BGR888:
		return DP_COLOR_RGB888;
	case DRM_FORMAT_ARGB8888:
	case DRM_FORMAT_XRGB8888:
		return DP_COLOR_RGBA8888;
	case DRM_FORMAT_ABGR8888:
		return DP_COLOR_BGRA8888;
	case DRM_FORMAT_RGBA8888:
		return DP_COLOR_ARGB8888;
	case DRM_FORMAT_BGRA8888:
		return DP_COLOR_ABGR8888;
	case DRM_FORMAT_YUYV:
		return DP_COLOR_YVYU;
	case DRM_FORMAT_YVYU:
		return DP_COLOR_YUYV;
	case DRM_FORMAT_UYVY:
		return DP_COLOR_VYUY;
	case DRM_FORMAT_VYUY:
		return DP_COLOR_UYVY;
	case DRM_FORMAT_NV12:
		return DP_COLOR_NV21;
	case DRM_FORMAT_NV21:
		return DP_COLOR_NV12;
	case DRM_FORMAT_NV16:
		return DP_COLOR_NV61;
	case DRM_FORMAT_NV61:
		return DP_COLOR_NV16;
	case DRM_FORMAT_NV24:
		return DP_COLOR_NV24;
	case DRM_FORMAT_NV42:
		return DP_COLOR_NV42;
	/*case DRM_FORMAT_MT21: */
	/*	return DP_COLOR_420_BLKP;*/
	case DRM_FORMAT_YUV420:
		return DP_COLOR_YV12;
	case DRM_FORMAT_YVU420:
		return DP_COLOR_I420;
	case DRM_FORMAT_YUV422:
		return DP_COLOR_I422;
	case DRM_FORMAT_YVU422:
		return DP_COLOR_YV16;
	case DRM_FORMAT_YUV444:
		return DP_COLOR_I444;
	case DRM_FORMAT_YVU444:
		return DP_COLOR_YV24;
	default:
		return DP_COLOR_UNKNOWN;
	}
	return DP_COLOR_UNKNOWN;
}

void mtk_mdp_drm_init(struct mtk_mdp_dev *mdp_dev)
{
	g_mdp_dev = mdp_dev;
	mtk_mdp_dbg(0, "mdp_dev=%p", g_mdp_dev);
}

void mtk_mdp_drm_deinit(void)
{
	g_mdp_dev = NULL;
}

struct mtk_mdp_ctx *mtk_mdp_create_ctx()
{
	struct mtk_mdp_dev *mdp = g_mdp_dev;
	struct mtk_mdp_ctx *ctx = NULL;
	int ret;

	if (mdp == NULL) {
		mtk_mdp_err("mtk-mdp is not initialized");
		return NULL;
	}

	if (mutex_lock_interruptible(&mdp->lock)) {
		mtk_mdp_err("lock failed");
		return NULL;
	}

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		goto err_ctx_alloc;

	mutex_init(&ctx->slock);
	ctx->id = mdp->id_counter++;
	ctx->mdp_dev = mdp;
	if (mdp->ctx_num++ == 0) {
		ret = vpu_load_firmware(mdp->vpu_dev);
		if (ret < 0) {
			dev_err(&mdp->pdev->dev,
				"vpu_load_firmware failed %d\n", ret);
			goto err_load_vpu;
		}

		ret = mtk_mdp_vpu_register(mdp->pdev);
		if (ret < 0) {
			dev_err(&mdp->pdev->dev,
				"mdp_vpu register failed %d\n", ret);
			goto err_load_vpu;
		}
	}

	mtk_mdp_dbg(0, "ctx_num=%d", mdp->ctx_num);

	ret = mtk_mdp_vpu_init(&ctx->vpu);
	if (ret < 0) {
		dev_err(&mdp->pdev->dev, "Initialize vpu failed %d\n", ret);
		goto err_load_vpu;
	}

	mutex_unlock(&mdp->lock);

	mtk_mdp_dbg(0, "%s [%d]", dev_name(&mdp->pdev->dev), ctx->id);

	return ctx;

err_load_vpu:
	mdp->ctx_num--;
	kfree(ctx);
err_ctx_alloc:
	mutex_unlock(&mdp->lock);

	return NULL;
}
EXPORT_SYMBOL(mtk_mdp_create_ctx);

int mtk_mdp_destroy_ctx(struct mtk_mdp_ctx *ctx)
{
	struct mtk_mdp_dev *mdp = ctx->mdp_dev;

	mutex_lock(&mdp->lock);
	mtk_mdp_vpu_deinit(&ctx->vpu);
	mdp->ctx_num--;

	mtk_mdp_dbg(0, "%s [%d]", dev_name(&mdp->pdev->dev), ctx->id);
	mtk_mdp_dbg(0, "ctx_num=%d", mdp->ctx_num);

	mutex_unlock(&mdp->lock);
	kfree(ctx);

	return 0;
}
EXPORT_SYMBOL(mtk_mdp_destroy_ctx);

void mtk_mdp_set_input_addr(struct mtk_mdp_ctx *ctx,
				struct mtk_mdp_addr *addr)
{
	struct mdp_buffer *src_buf = &ctx->vpu.vsi->src_buffer;
	int i;

	for (i = 0; i < ARRAY_SIZE(addr->addr); i++)
		src_buf->addr_mva[i] = (uint64_t)addr->addr[i];
}
EXPORT_SYMBOL(mtk_mdp_set_input_addr);

void mtk_mdp_set_output_addr(struct mtk_mdp_ctx *ctx,
				struct mtk_mdp_addr *addr)
{
	struct mdp_buffer *dst_buf = &ctx->vpu.vsi->dst_buffer;
	int i;

	for (i = 0; i < ARRAY_SIZE(addr->addr); i++)
		dst_buf->addr_mva[i] = (uint64_t)addr->addr[i];
}
EXPORT_SYMBOL(mtk_mdp_set_output_addr);

void mtk_mdp_set_in_size(struct mtk_mdp_ctx *ctx,
				struct mtk_mdp_frame *frame)
{
	struct mdp_config *config = &ctx->vpu.vsi->src_config;

	/* Set input pixel offset */
	config->crop_x = frame->crop.left;
	config->crop_y = frame->crop.top;

	/* Set input cropped size */
	config->crop_w = frame->crop.width;
	config->crop_h = frame->crop.height;

	/* Set input original size */
	config->x = 0;
	config->y = 0;
	config->w = frame->width;
	config->h = frame->height;
}
EXPORT_SYMBOL(mtk_mdp_set_in_size);

void mtk_mdp_set_in_image_format(struct mtk_mdp_ctx *ctx,
				struct mtk_mdp_frame *frame)
{
	unsigned int i;
	struct mdp_config *config = &ctx->vpu.vsi->src_config;
	struct mdp_buffer *src_buf = &ctx->vpu.vsi->src_buffer;

	src_buf->plane_num = frame->fmt->num_planes;
	config->format = mtk_mdp_map_drm_color_format(frame->fmt->pixelformat);
	config->w_stride = 0; /* MDP will calculate it by color format. */
	config->h_stride = 0; /* MDP will calculate it by color format. */

	for (i = 0; i < src_buf->plane_num; i++)
		src_buf->plane_size[i] = frame->payload[i];
}
EXPORT_SYMBOL(mtk_mdp_set_in_image_format);

void mtk_mdp_set_out_size(struct mtk_mdp_ctx *ctx,
				struct mtk_mdp_frame *frame)
{
	struct mdp_config *config = &ctx->vpu.vsi->dst_config;

	config->crop_x = frame->crop.left;
	config->crop_y = frame->crop.top;
	config->crop_w = frame->crop.width;
	config->crop_h = frame->crop.height;
	config->x = 0;
	config->y = 0;
	config->w = frame->width;
	config->h = frame->height;
}
EXPORT_SYMBOL(mtk_mdp_set_out_size);

void mtk_mdp_set_out_image_format(struct mtk_mdp_ctx *ctx,
				struct mtk_mdp_frame *frame)
{
	unsigned int i;
	struct mdp_config *config = &ctx->vpu.vsi->dst_config;
	struct mdp_buffer *dst_buf = &ctx->vpu.vsi->dst_buffer;

	dst_buf->plane_num = frame->fmt->num_planes;
	config->format = mtk_mdp_map_drm_color_format(frame->fmt->pixelformat);
	config->w_stride = 0; /* MDP will calculate it by color format. */
	config->h_stride = 0; /* MDP will calculate it by color format. */
	for (i = 0; i < dst_buf->plane_num; i++)
		dst_buf->plane_size[i] = frame->payload[i];
}
EXPORT_SYMBOL(mtk_mdp_set_out_image_format);

void mtk_mdp_set_rotation(struct mtk_mdp_ctx *ctx,
				int rotate, int hflip, int vflip)
{
	struct mdp_config_misc *misc = &ctx->vpu.vsi->misc;

	misc->orientation = rotate;
	misc->hflip = hflip;
	misc->vflip = vflip;
}
EXPORT_SYMBOL(mtk_mdp_set_rotation);

void mtk_mdp_set_global_alpha(struct mtk_mdp_ctx *ctx,
				int alpha)
{
	struct mdp_config_misc *misc = &ctx->vpu.vsi->misc;

	misc->alpha = alpha;
}
EXPORT_SYMBOL(mtk_mdp_set_global_alpha);
