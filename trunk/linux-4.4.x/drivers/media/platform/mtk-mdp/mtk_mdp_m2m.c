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

#include <linux/device.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/pm_runtime.h>
#include <linux/workqueue.h>
#include <media/v4l2-ioctl.h>

#include "mtk_mdp_core.h"
#include "mtk_mdp_m2m.h"
#include "mtk_mdp_regs.h"
#include "mtk_mdp_vpu.h"


/**
 *  struct mtk_mdp_pix_limit - image pixel size limits
 *  @org_w: source pixel width
 *  @org_h: source pixel height
 *  @target_rot_dis_w: pixel dst scaled width with the rotator is off
 *  @target_rot_dis_h: pixel dst scaled height with the rotator is off
 *  @target_rot_en_w: pixel dst scaled width with the rotator is on
 *  @target_rot_en_h: pixel dst scaled height with the rotator is on
 */
struct mtk_mdp_pix_limit {
	u16 org_w;
	u16 org_h;
	u16 target_rot_dis_w;
	u16 target_rot_dis_h;
	u16 target_rot_en_w;
	u16 target_rot_en_h;
};

/**
 *  struct mtk_mdp_pix_align - alignement of image
 *  @org_w: source alignment of width
 *  @org_h: source alignment of height
 *  @target_w: dst alignment of width
 *  @target_h: dst alignment of height
 */
struct mtk_mdp_pix_align {
	u16 org_w;
	u16 org_h;
	u16 target_w;
	u16 target_h;
};

static struct mtk_mdp_pix_align mtk_mdp_size_align = {
	.org_w			= 16,
	.org_h			= 16,
	.target_w		= 2,
	.target_h		= 2,
};

/* align size for normal raster scan pixel format */
static struct mtk_mdp_pix_align mtk_mdp_rs_align = {
	.org_w			= 2,
	.org_h			= 2,
	.target_w		= 2,
	.target_h		= 2,
};

static const struct mtk_mdp_fmt mtk_mdp_formats[] = {
	{
		.name		= "YUV420 MT21C. 2p, Y/CbCr",
		.pixelformat	= V4L2_PIX_FMT_MT21C,
		.depth		= { 8, 4 },
		.row_depth	= { 8, 4 },
		.num_planes	= 2,
		.num_comp	= 2,
		.align		= &mtk_mdp_size_align,
		.flags		= MTK_MDP_FMT_FLAG_OUTPUT,
	}, {
		.name		= "YUV420 MT21. 2p, Y/CbCr",
		.pixelformat	= V4L2_PIX_FMT_MT21,
		.depth		= { 8, 4 },
		.row_depth	= { 8, 4 },
		.num_planes	= 2,
		.num_comp	= 2,
		.align		= &mtk_mdp_size_align,
		.flags		= MTK_MDP_FMT_FLAG_OUTPUT,
	}, {
		.name		= "YUV420 MT21S contig. Y/CbCr",
		.pixelformat	= V4L2_PIX_FMT_MT21S,
		.depth		= { 12 },
		.row_depth	= { 8, 4 },
		.num_planes	= 1,
		.num_comp	= 2,
		.align		= &mtk_mdp_size_align,
		.flags		= MTK_MDP_FMT_FLAG_OUTPUT,
	}, {
		.name		= "YUV420 non-contig. 2p, Y/CbCr",
		.pixelformat	= V4L2_PIX_FMT_NV12M,
		.depth		= { 8, 4 },
		.row_depth	= { 8, 4 },
		.num_planes	= 2,
		.num_comp	= 2,
		.flags		= MTK_MDP_FMT_FLAG_OUTPUT |
				  MTK_MDP_FMT_FLAG_CAPTURE,
		.align		= &mtk_mdp_rs_align,
	}, {
		.name		= "YUV420 contig, Y/CbCr",
		.pixelformat	= V4L2_PIX_FMT_NV12,
		.depth		= { 12 },
		.row_depth	= { 8, 4 },
		.num_planes	= 1,
		.num_comp	= 2,
		.flags		= MTK_MDP_FMT_FLAG_OUTPUT |
				  MTK_MDP_FMT_FLAG_CAPTURE,
		.align		= &mtk_mdp_rs_align,
	}, {
		.name		= "YUV420 non-contig. 3p, Y/Cb/Cr",
		.pixelformat	= V4L2_PIX_FMT_YUV420M,
		.depth		= { 8, 2, 2 },
		.row_depth	= { 8, 2, 2 },
		.num_planes	= 3,
		.num_comp	= 3,
		.flags		= MTK_MDP_FMT_FLAG_OUTPUT |
				  MTK_MDP_FMT_FLAG_CAPTURE,
		.align		= &mtk_mdp_rs_align,
	}, {
		.name		= "YVU420 contig, YCrCb",
		.pixelformat	= V4L2_PIX_FMT_YVU420,
		.depth		= { 12 },
		.row_depth	= { 8, 2, 2 },
		.num_planes	= 1,
		.num_comp	= 3,
		.flags		= MTK_MDP_FMT_FLAG_OUTPUT |
				  MTK_MDP_FMT_FLAG_CAPTURE,
		.align		= &mtk_mdp_rs_align,
	}, {
		.name		= "YUV 4:2:2 contig, Y/Cb/Cr",
		.pixelformat	= V4L2_PIX_FMT_YUV422P,
		.depth		= { 16 },
		.row_depth	= { 16 },
		.num_planes	= 1,
		.num_comp	= 3,
		.flags		= MTK_MDP_FMT_FLAG_OUTPUT |
				  MTK_MDP_FMT_FLAG_CAPTURE,
		.align		= &mtk_mdp_rs_align,
	}, {
		.name		= "YUV 4:2:2 contig, Y/CbCr",
		.pixelformat	= V4L2_PIX_FMT_NV16,
		.depth		= { 16 },
		.row_depth	= { 8, 8 },
		.num_planes	= 1,
		.num_comp	= 2,
		.flags		= MTK_MDP_FMT_FLAG_OUTPUT |
				  MTK_MDP_FMT_FLAG_CAPTURE,
		.align		= &mtk_mdp_rs_align,
	}, {
		.name		= "YUV 4:2:2 non-contig. 2p, Y/CbCr",
		.pixelformat	= V4L2_PIX_FMT_NV16M,
		.depth		= { 8, 8 },
		.row_depth	= { 8, 8 },
		.num_planes	= 2,
		.num_comp	= 2,
		.flags		= MTK_MDP_FMT_FLAG_OUTPUT |
				  MTK_MDP_FMT_FLAG_CAPTURE,
		.align		= &mtk_mdp_rs_align,
	}, {
		.name		= "YUV 4:2:2.  1p, YCbYCr",
		.pixelformat	= V4L2_PIX_FMT_YUYV,
		.depth		= { 16 },
		.row_depth	= { 16 },
		.num_planes	= 1,
		.num_comp	= 1,
		.flags		= MTK_MDP_FMT_FLAG_OUTPUT |
				  MTK_MDP_FMT_FLAG_CAPTURE,
		.align		= &mtk_mdp_rs_align,
	}, {
		.name		= "YUV 4:2:2.  1p, CbYCrY",
		.pixelformat	= V4L2_PIX_FMT_UYVY,
		.depth		= { 16 },
		.row_depth	= { 16 },
		.num_planes	= 1,
		.num_comp	= 1,
		.flags		= MTK_MDP_FMT_FLAG_OUTPUT |
				  MTK_MDP_FMT_FLAG_CAPTURE,
		.align		= &mtk_mdp_rs_align,
	}, {
		.name		= "ARGB888.  1p, ARGB8888",
		.pixelformat	= V4L2_PIX_FMT_ARGB32,
		.depth		= { 32 },
		.row_depth  = { 32 },
		.num_planes = 1,
		.num_comp	= 1,
		.flags		= MTK_MDP_FMT_FLAG_OUTPUT |
				  MTK_MDP_FMT_FLAG_CAPTURE,
		.align		= &mtk_mdp_rs_align,
	}, {
		.name		= "BGRA888.  1p, BGRA8888",
		.pixelformat	= V4L2_PIX_FMT_ABGR32,
		.depth		= { 32 },
		.row_depth	= { 32 },
		.num_planes	= 1,
		.num_comp	= 1,
		.flags		= MTK_MDP_FMT_FLAG_OUTPUT |
				  MTK_MDP_FMT_FLAG_CAPTURE,
		.align		= &mtk_mdp_rs_align,
	}
};

static struct mtk_mdp_pix_limit mtk_mdp_size_max = {
	.target_rot_dis_w	= 4096,
	.target_rot_dis_h	= 4096,
	.target_rot_en_w	= 4096,
	.target_rot_en_h	= 4096,
};

static struct mtk_mdp_pix_limit mtk_mdp_size_min = {
	.org_w			= 16,
	.org_h			= 16,
	.target_rot_dis_w	= 16,
	.target_rot_dis_h	= 16,
	.target_rot_en_w	= 16,
	.target_rot_en_h	= 16,
};

static struct mtk_mdp_variant mtk_mdp_default_variant = {
	.pix_max		= &mtk_mdp_size_max,
	.pix_min		= &mtk_mdp_size_min,
	.pix_align		= &mtk_mdp_size_align,
	.h_scale_up_max		= 32,
	.v_scale_up_max		= 32,
	.h_scale_down_max	= 32,
	.v_scale_down_max	= 128,
};

static const struct mtk_mdp_fmt *mtk_mdp_find_fmt(u32 pixelformat, u32 type)
{
	u32 i, flag;

	flag = V4L2_TYPE_IS_OUTPUT(type) ? MTK_MDP_FMT_FLAG_OUTPUT :
					   MTK_MDP_FMT_FLAG_CAPTURE;

	for (i = 0; i < ARRAY_SIZE(mtk_mdp_formats); ++i) {
		if (!(mtk_mdp_formats[i].flags & flag))
			continue;
		if (mtk_mdp_formats[i].pixelformat == pixelformat)
			return &mtk_mdp_formats[i];
	}
	return NULL;
}

static const struct mtk_mdp_fmt *mtk_mdp_find_fmt_by_index(u32 index, u32 type)
{
	u32 i, flag, num = 0;

	flag = V4L2_TYPE_IS_OUTPUT(type) ? MTK_MDP_FMT_FLAG_OUTPUT :
					   MTK_MDP_FMT_FLAG_CAPTURE;

	for (i = 0; i < ARRAY_SIZE(mtk_mdp_formats); ++i) {
		if (!(mtk_mdp_formats[i].flags & flag))
			continue;
		if (index == num)
			return &mtk_mdp_formats[i];
		num++;
	}
	return NULL;
}

static void mtk_mdp_bound_align_image(u32 *w, unsigned int wmin,
				      unsigned int wmax, unsigned int align_w,
				      u32 *h, unsigned int hmin,
				      unsigned int hmax, unsigned int align_h)
{
	int org_w, org_h, step_w, step_h;
	int walign, halign;

	org_w = *w;
	org_h = *h;
	walign = ffs(align_w) - 1;
	halign = ffs(align_h) - 1;
	v4l_bound_align_image(w, wmin, wmax, walign, h, hmin, hmax, halign, 0);

	step_w = 1 << walign;
	step_h = 1 << halign;
	if (*w < org_w && (*w + step_w) <= wmax)
		*w += step_w;
	if (*h < org_h && (*h + step_h) <= hmax)
		*h += step_h;
}

static const struct mtk_mdp_fmt *mtk_mdp_try_fmt_mplane(struct mtk_mdp_ctx *ctx,
							struct v4l2_format *f)
{
	struct mtk_mdp_dev *mdp = ctx->mdp_dev;
	struct mtk_mdp_variant *variant = mdp->variant;
	struct v4l2_pix_format_mplane *pix_mp = &f->fmt.pix_mp;
	const struct mtk_mdp_fmt *fmt;
	u32 max_w, max_h, align_w, align_h;
	u32 min_w, min_h, org_w, org_h;
	int i;

	fmt = mtk_mdp_find_fmt(pix_mp->pixelformat, f->type);
	if (!fmt) {
		dev_dbg(&ctx->mdp_dev->pdev->dev,
			"pixelformat format 0x%X invalid\n",
			pix_mp->pixelformat);
		return NULL;
	}

	pix_mp->field = V4L2_FIELD_NONE;

	max_w = variant->pix_max->target_rot_dis_w;
	max_h = variant->pix_max->target_rot_dis_h;

	if (fmt->align == NULL) {
		align_w = variant->pix_align->org_w;
		align_h = variant->pix_align->org_h;
	} else {
		align_w = fmt->align->org_w;
		align_h = fmt->align->org_h;
	}

	if (V4L2_TYPE_IS_OUTPUT(f->type)) {
		min_w = variant->pix_min->org_w;
		min_h = variant->pix_min->org_h;
	} else {
		min_w = variant->pix_min->target_rot_dis_w;
		min_h = variant->pix_min->target_rot_dis_h;
	}

	mtk_mdp_dbg(2, "[%d] type:%d, wxh:%dx%d, align:%dx%d, max:%dx%d",
		    ctx->id, f->type, pix_mp->width, pix_mp->height,
		    align_w, align_h, max_w, max_h);
	/*
	 * To check if image size is modified to adjust parameter against
	 * hardware abilities
	 */
	org_w = pix_mp->width;
	org_h = pix_mp->height;

	mtk_mdp_bound_align_image(&pix_mp->width, min_w, max_w, align_w,
				  &pix_mp->height, min_h, max_h, align_h);

	if (org_w != pix_mp->width || org_h != pix_mp->height)
		mtk_mdp_dbg(1, "[%d] size change:%dx%d to %dx%d", ctx->id,
			    org_w, org_h, pix_mp->width, pix_mp->height);
	pix_mp->num_planes = fmt->num_planes;

	for (i = 0; i < pix_mp->num_planes; ++i) {
		int bpl = (pix_mp->width * fmt->row_depth[i]) / 8;
		int sizeimage = (pix_mp->width * pix_mp->height *
			fmt->depth[i]) / 8;

		pix_mp->plane_fmt[i].bytesperline = bpl;
		if (pix_mp->plane_fmt[i].sizeimage < sizeimage)
			pix_mp->plane_fmt[i].sizeimage = sizeimage;
		mtk_mdp_dbg(2, "[%d] p%d, bpl:%d, sizeimage:%d", ctx->id, i,
			    bpl, pix_mp->plane_fmt[i].sizeimage);
	}

	return fmt;
}

struct mtk_mdp_frame *mtk_mdp_ctx_get_frame(struct mtk_mdp_ctx *ctx,
					    enum v4l2_buf_type type)
{
	if (V4L2_TYPE_IS_OUTPUT(type))
		return &ctx->s_frame;
	return &ctx->d_frame;
}

static void mtk_mdp_check_crop_change(u32 new_w, u32 new_h, u32 *w, u32 *h)
{
	if (new_w != *w || new_h != *h) {
		mtk_mdp_dbg(1, "size change:%dx%d to %dx%d",
			    *w, *h, new_w, new_h);

		*w = new_w;
		*h = new_h;
	}
}

static int mtk_mdp_try_crop(struct mtk_mdp_ctx *ctx, u32 type,
			    struct v4l2_rect *r)
{
	struct mtk_mdp_frame *frame;
	struct mtk_mdp_dev *mdp = ctx->mdp_dev;
	struct mtk_mdp_variant *variant = mdp->variant;
	u32 align_w, align_h, new_w, new_h;
	u32 min_w, min_h, max_w, max_h;

	if (r->top < 0 || r->left < 0) {
		dev_err(&ctx->mdp_dev->pdev->dev,
			"doesn't support negative values for top & left\n");
		return -EINVAL;
	}

	mtk_mdp_dbg(2, "[%d] type:%d, set wxh:%dx%d", ctx->id, type,
		    r->width, r->height);

	frame = mtk_mdp_ctx_get_frame(ctx, type);
	max_w = frame->width;
	max_h = frame->height;
	new_w = r->width;
	new_h = r->height;

	if (V4L2_TYPE_IS_OUTPUT(type)) {
		align_w = 1;
		align_h = 1;
		min_w = 8;
		min_h = 8;
	} else {
		align_w = variant->pix_align->target_w;
		align_h = variant->pix_align->target_h;
		if (ctx->ctrls.rotate->val == 90 ||
		    ctx->ctrls.rotate->val == 270) {
			max_w = frame->height;
			max_h = frame->width;
			min_w = variant->pix_min->target_rot_en_w;
			min_h = variant->pix_min->target_rot_en_h;
			new_w = r->height;
			new_h = r->width;
		} else {
			min_w = variant->pix_min->target_rot_dis_w;
			min_h = variant->pix_min->target_rot_dis_h;
		}
	}

	mtk_mdp_dbg(2, "[%d] align:%dx%d, min:%dx%d, new:%dx%d", ctx->id,
		    align_w, align_h, min_w, min_h, new_w, new_h);

	mtk_mdp_bound_align_image(&new_w, min_w, max_w, align_w,
				  &new_h, min_h, max_h, align_h);

	if (!V4L2_TYPE_IS_OUTPUT(type) &&
		(ctx->ctrls.rotate->val == 90 ||
		ctx->ctrls.rotate->val == 270))
		mtk_mdp_check_crop_change(new_h, new_w,
					  &r->width, &r->height);
	else
		mtk_mdp_check_crop_change(new_w, new_h,
					  &r->width, &r->height);

	/* adjust left/top if cropping rectangle is out of bounds */
	/* Need to add code to algin left value with 2's multiple */
	if (r->left + new_w > max_w)
		r->left = max_w - new_w;
	if (r->top + new_h > max_h)
		r->top = max_h - new_h;

	if (r->left & 1)
		r->left -= 1;

	mtk_mdp_dbg(2, "[%d] crop l,t,w,h:%d,%d,%d,%d, max:%dx%d", ctx->id,
		    r->left, r->top, r->width,
		    r->height, max_w, max_h);
	return 0;
}

static inline struct mtk_mdp_ctx *fh_to_ctx(struct v4l2_fh *fh)
{
	return container_of(fh, struct mtk_mdp_ctx, fh);
}

static inline struct mtk_mdp_ctx *ctrl_to_ctx(struct v4l2_ctrl *ctrl)
{
	return container_of(ctrl->handler, struct mtk_mdp_ctx, ctrl_handler);
}

void mtk_mdp_ctx_state_lock_set(struct mtk_mdp_ctx *ctx, u32 state)
{
	mutex_lock(&ctx->slock);
	ctx->state |= state;
	mutex_unlock(&ctx->slock);
}

static void mtk_mdp_ctx_state_lock_clear(struct mtk_mdp_ctx *ctx, u32 state)
{
	mutex_lock(&ctx->slock);
	ctx->state &= ~state;
	mutex_unlock(&ctx->slock);
}

static bool mtk_mdp_ctx_state_is_set(struct mtk_mdp_ctx *ctx, u32 mask)
{
	bool ret;

	mutex_lock(&ctx->slock);
	ret = (ctx->state & mask) == mask;
	mutex_unlock(&ctx->slock);
	return ret;
}

static void mtk_mdp_ctx_lock(struct vb2_queue *vq)
{
	struct mtk_mdp_ctx *ctx = vb2_get_drv_priv(vq);

	mutex_lock(&ctx->mdp_dev->lock);
}

static void mtk_mdp_ctx_unlock(struct vb2_queue *vq)
{
	struct mtk_mdp_ctx *ctx = vb2_get_drv_priv(vq);

	mutex_unlock(&ctx->mdp_dev->lock);
}

static void mtk_mdp_set_frame_size(struct mtk_mdp_frame *frame, int width,
				   int height)
{
	frame->width = width;
	frame->height = height;
	frame->crop.width = width;
	frame->crop.height = height;
	frame->crop.left = 0;
	frame->crop.top = 0;
}

static int mtk_mdp_m2m_start_streaming(struct vb2_queue *q, unsigned int count)
{
	struct mtk_mdp_ctx *ctx = q->drv_priv;
	int ret;

	ret = pm_runtime_get_sync(&ctx->mdp_dev->pdev->dev);
	if (ret < 0)
		mtk_mdp_dbg(1, "[%d] pm_runtime_get_sync failed:%d",
			    ctx->id, ret);

	return 0;
}

static void *mtk_mdp_m2m_buf_remove(struct mtk_mdp_ctx *ctx,
				    enum v4l2_buf_type type)
{
	if (V4L2_TYPE_IS_OUTPUT(type))
		return v4l2_m2m_src_buf_remove(ctx->m2m_ctx);
	else
		return v4l2_m2m_dst_buf_remove(ctx->m2m_ctx);
}

static void mtk_mdp_m2m_stop_streaming(struct vb2_queue *q)
{
	struct mtk_mdp_ctx *ctx = q->drv_priv;
	struct vb2_v4l2_buffer *vb;

	vb = mtk_mdp_m2m_buf_remove(ctx, q->type);
	while (vb != NULL) {
		v4l2_m2m_buf_done(vb, VB2_BUF_STATE_ERROR);
		vb = mtk_mdp_m2m_buf_remove(ctx, q->type);
	}

	pm_runtime_put(&ctx->mdp_dev->pdev->dev);
}

static void mtk_mdp_m2m_job_abort(void *priv)
{
}

/* The color format (num_planes) must be already configured. */
static void mtk_mdp_prepare_addr(struct mtk_mdp_ctx *ctx,
				 struct vb2_buffer *vb,
				 struct mtk_mdp_frame *frame,
				 struct mtk_mdp_addr *addr)
{
	u32 pix_size, planes, i;

	pix_size = frame->width * frame->height;
	planes = min_t(u32, frame->fmt->num_planes, ARRAY_SIZE(addr->addr));
	for (i = 0; i < planes; i++)
		addr->addr[i] = vb2_dma_contig_plane_dma_addr(vb, i);

	if (planes == 1 && frame->fmt->num_comp > 1) {
		if (frame->fmt->pixelformat == V4L2_PIX_FMT_YVU420) {
			addr->addr[1] = (dma_addr_t)(addr->addr[0] + pix_size);
			addr->addr[2] = (dma_addr_t)(addr->addr[1] +
					(pix_size >> 2));
		} else if (frame->fmt->pixelformat == V4L2_PIX_FMT_NV12) {
			addr->addr[1] = (dma_addr_t)(addr->addr[0] + pix_size);
			addr->addr[2] = 0;
		} else if (frame->fmt->pixelformat == V4L2_PIX_FMT_NV16) {
			addr->addr[1] = (dma_addr_t)(addr->addr[0] + pix_size);
			addr->addr[2] = 0;
		} else if (frame->fmt->pixelformat == V4L2_PIX_FMT_MT21S) {
			addr->addr[1] = (dma_addr_t)(addr->addr[0] + pix_size);
			addr->addr[2] = 0;
		} else {
			dev_err(&ctx->mdp_dev->pdev->dev,
				"Invalid pixelformat:0x%x\n",
				frame->fmt->pixelformat);
		}
	}
	mtk_mdp_dbg(3, "[%d] planes:%d, size:%d, addr:%p,%p,%p",
		    ctx->id, planes, pix_size, (void *)addr->addr[0],
		    (void *)addr->addr[1], (void *)addr->addr[2]);
}

static void mtk_mdp_m2m_get_bufs(struct mtk_mdp_ctx *ctx)
{
	struct mtk_mdp_frame *s_frame, *d_frame;
	struct vb2_v4l2_buffer *src_vb, *dst_vb;
	struct vb2_buffer *src_vbuf, *dst_vbuf;

	s_frame = &ctx->s_frame;
	d_frame = &ctx->d_frame;

	src_vb = v4l2_m2m_next_src_buf(ctx->m2m_ctx);
	mtk_mdp_prepare_addr(ctx, &(src_vb->vb2_buf), s_frame, &s_frame->addr);

	dst_vb = v4l2_m2m_next_dst_buf(ctx->m2m_ctx);
	mtk_mdp_prepare_addr(ctx, &(dst_vb->vb2_buf), d_frame, &d_frame->addr);

	src_vbuf = &src_vb->vb2_buf;
	dst_vbuf = &dst_vb->vb2_buf;
	dst_vb->timestamp = src_vb->timestamp;
}

static void mtk_mdp_process_done(void *priv, int vb_state)
{
	struct mtk_mdp_dev *mdp = priv;
	struct mtk_mdp_ctx *ctx;
	struct vb2_v4l2_buffer *src_vb, *dst_vb;
	struct vb2_buffer *src_vbuf = 0, *dst_vbuf = 0;

	ctx = v4l2_m2m_get_curr_priv(mdp->m2m_dev);
	if (!ctx)
		return;

	src_vb = v4l2_m2m_src_buf_remove(ctx->m2m_ctx);
	src_vbuf = &src_vb->vb2_buf;
	dst_vb = v4l2_m2m_dst_buf_remove(ctx->m2m_ctx);
	dst_vbuf = &dst_vb->vb2_buf;

	dst_vb->timestamp = src_vb->timestamp;
	dst_vb->timecode = src_vb->timecode;
	dst_vb->flags &= ~V4L2_BUF_FLAG_TSTAMP_SRC_MASK;
	dst_vb->flags |= src_vb->flags & V4L2_BUF_FLAG_TSTAMP_SRC_MASK;

	v4l2_m2m_buf_done(src_vb, vb_state);
	v4l2_m2m_buf_done(dst_vb, vb_state);
	v4l2_m2m_job_finish(ctx->mdp_dev->m2m_dev, ctx->m2m_ctx);
}

static void mtk_mdp_m2m_worker(struct work_struct *work)
{
	struct mtk_mdp_ctx *ctx =
				container_of(work, struct mtk_mdp_ctx, work);
	struct mtk_mdp_dev *mdp = ctx->mdp_dev;
	enum vb2_buffer_state buf_state = VB2_BUF_STATE_ERROR;
	int ret;

	if (mtk_mdp_ctx_state_is_set(ctx, MTK_MDP_CTX_ERROR)) {
		dev_err(&mdp->pdev->dev, "ctx is in error state");
		goto worker_end;
	}

	mtk_mdp_m2m_get_bufs(ctx);

	mtk_mdp_hw_set_input_addr(ctx, &ctx->s_frame.addr);
	mtk_mdp_hw_set_output_addr(ctx, &ctx->d_frame.addr);

	mtk_mdp_hw_set_in_size(ctx);
	mtk_mdp_hw_set_in_image_format(ctx);

	mtk_mdp_hw_set_out_size(ctx);
	mtk_mdp_hw_set_out_image_format(ctx);

	mtk_mdp_hw_set_rotation(ctx);
	mtk_mdp_hw_set_global_alpha(ctx);

	ret = mtk_mdp_vpu_process(&ctx->vpu);
	if (ret) {
		dev_err(&mdp->pdev->dev, "processing failed: %d", ret);
		goto worker_end;
	}

	buf_state = VB2_BUF_STATE_DONE;

worker_end:
	mtk_mdp_process_done(mdp, buf_state);
}

static void mtk_mdp_m2m_device_run(void *priv)
{
	struct mtk_mdp_ctx *ctx = priv;

	queue_work(ctx->mdp_dev->job_wq, &ctx->work);
}

static int mtk_mdp_m2m_queue_setup(struct vb2_queue *vq,
			const void *fmt,
			unsigned int *num_buffers, unsigned int *num_planes,
			unsigned int sizes[], void *allocators[])
{
	struct mtk_mdp_ctx *ctx = vb2_get_drv_priv(vq);
	struct mtk_mdp_frame *frame;
	int i;

	frame = mtk_mdp_ctx_get_frame(ctx, vq->type);

	*num_planes = frame->fmt->num_planes;
	for (i = 0; i < frame->fmt->num_planes; i++) {
		sizes[i] = frame->payload[i];
		allocators[i] = ctx->mdp_dev->alloc_ctx;
	}
	return 0;
}

static int mtk_mdp_m2m_buf_prepare(struct vb2_buffer *vb)
{
	struct mtk_mdp_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);
	struct mtk_mdp_frame *frame;
	int i;

	frame = mtk_mdp_ctx_get_frame(ctx, vb->vb2_queue->type);

	if (!V4L2_TYPE_IS_OUTPUT(vb->vb2_queue->type)) {
		for (i = 0; i < frame->fmt->num_planes; i++)
			vb2_set_plane_payload(vb, i, frame->payload[i]);
	}

	return 0;
}

static void mtk_mdp_m2m_buf_queue(struct vb2_buffer *vb)
{
	struct mtk_mdp_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);

	v4l2_m2m_buf_queue(ctx->m2m_ctx, to_vb2_v4l2_buffer(vb));
}

static struct vb2_ops mtk_mdp_m2m_qops = {
	.queue_setup	 = mtk_mdp_m2m_queue_setup,
	.buf_prepare	 = mtk_mdp_m2m_buf_prepare,
	.buf_queue	 = mtk_mdp_m2m_buf_queue,
	.wait_prepare	 = mtk_mdp_ctx_unlock,
	.wait_finish	 = mtk_mdp_ctx_lock,
	.stop_streaming	 = mtk_mdp_m2m_stop_streaming,
	.start_streaming = mtk_mdp_m2m_start_streaming,
};

static int mtk_mdp_m2m_querycap(struct file *file, void *fh,
				struct v4l2_capability *cap)
{
	struct mtk_mdp_ctx *ctx = fh_to_ctx(fh);
	struct mtk_mdp_dev *mdp = ctx->mdp_dev;

	strlcpy(cap->driver, MTK_MDP_MODULE_NAME, sizeof(cap->driver));
	strlcpy(cap->card, mdp->pdev->name, sizeof(cap->card));
	strlcpy(cap->bus_info, "platform", sizeof(cap->bus_info));
	cap->device_caps = V4L2_CAP_STREAMING | V4L2_CAP_VIDEO_M2M_MPLANE;
	cap->capabilities = cap->device_caps | V4L2_CAP_DEVICE_CAPS;

	return 0;
}

static int mtk_mdp_enum_fmt_mplane(struct v4l2_fmtdesc *f, u32 type)
{
	const struct mtk_mdp_fmt *fmt;

	fmt = mtk_mdp_find_fmt_by_index(f->index, type);
	if (!fmt)
		return -EINVAL;

	strlcpy(f->description, fmt->name, sizeof(f->description));
	f->pixelformat = fmt->pixelformat;

	return 0;
}

static int mtk_mdp_m2m_enum_fmt_mplane_vid_cap(struct file *file, void *priv,
				       struct v4l2_fmtdesc *f)
{
	return mtk_mdp_enum_fmt_mplane(f, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);
}

static int mtk_mdp_m2m_enum_fmt_mplane_vid_out(struct file *file, void *priv,
				       struct v4l2_fmtdesc *f)
{
	return mtk_mdp_enum_fmt_mplane(f, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE);
}

static int mtk_mdp_g_fmt_mplane(struct mtk_mdp_ctx *ctx, struct v4l2_format *f)
{
	struct mtk_mdp_frame *frame;
	struct v4l2_pix_format_mplane *pix_mp;
	int i;

	mtk_mdp_dbg(2, "[%d] type:%d", ctx->id, f->type);

	frame = mtk_mdp_ctx_get_frame(ctx, f->type);
	pix_mp = &f->fmt.pix_mp;

	pix_mp->width = frame->width;
	pix_mp->height = frame->height;
	pix_mp->field = V4L2_FIELD_NONE;
	pix_mp->pixelformat = frame->fmt->pixelformat;
	pix_mp->num_planes = frame->fmt->num_planes;

	for (i = 0; i < pix_mp->num_planes; ++i) {
		pix_mp->plane_fmt[i].bytesperline = (frame->width *
			frame->fmt->depth[i]) / 8;
		pix_mp->plane_fmt[i].sizeimage = (frame->width *
			frame->height * frame->fmt->depth[i]) / 8;

		mtk_mdp_dbg(2, "[%d] p%d, bpl:%d, sizeimage:%d", ctx->id, i,
			    pix_mp->plane_fmt[i].bytesperline,
			    pix_mp->plane_fmt[i].sizeimage);
	}

	return 0;
}

static int mtk_mdp_m2m_g_fmt_mplane(struct file *file, void *fh,
				    struct v4l2_format *f)
{
	struct mtk_mdp_ctx *ctx = fh_to_ctx(fh);

	return mtk_mdp_g_fmt_mplane(ctx, f);
}

static int mtk_mdp_m2m_try_fmt_mplane(struct file *file, void *fh,
				      struct v4l2_format *f)
{
	struct mtk_mdp_ctx *ctx = fh_to_ctx(fh);

	if (!mtk_mdp_try_fmt_mplane(ctx, f))
		return -EINVAL;
	return 0;
}

static int mtk_mdp_m2m_s_fmt_mplane(struct file *file, void *fh,
				    struct v4l2_format *f)
{
	struct mtk_mdp_ctx *ctx = fh_to_ctx(fh);
	struct vb2_queue *vq;
	struct mtk_mdp_frame *frame;
	struct v4l2_pix_format_mplane *pix;
	int i;

	mtk_mdp_dbg(2, "[%d] type:%d", ctx->id, f->type);

	frame = mtk_mdp_ctx_get_frame(ctx, f->type);
	frame->fmt = mtk_mdp_try_fmt_mplane(ctx, f);
	if (!frame->fmt) {
		mtk_mdp_err("try_fmt failed");
		return -EINVAL;
	}

	vq = v4l2_m2m_get_vq(ctx->m2m_ctx, f->type);
	if (vb2_is_streaming(vq)) {
		dev_info(&ctx->mdp_dev->pdev->dev, "queue %d busy", f->type);
		return -EBUSY;
	}

	pix = &f->fmt.pix_mp;
	for (i = 0; i < frame->fmt->num_planes; i++) {
		frame->payload[i] = pix->plane_fmt[i].sizeimage;
		frame->pitch[i] = pix->plane_fmt[i].bytesperline;
	}

	mtk_mdp_set_frame_size(frame, pix->width, pix->height);

	if (V4L2_TYPE_IS_OUTPUT(f->type))
		mtk_mdp_ctx_state_lock_set(ctx, MTK_MDP_SRC_FMT);
	else
		mtk_mdp_ctx_state_lock_set(ctx, MTK_MDP_DST_FMT);

	mtk_mdp_dbg(2, "[%d] type:%d, frame:%dx%d", ctx->id, f->type,
		    frame->width, frame->height);

	return 0;
}

static int mtk_mdp_m2m_reqbufs(struct file *file, void *fh,
			       struct v4l2_requestbuffers *reqbufs)
{
	struct mtk_mdp_ctx *ctx = fh_to_ctx(fh);
	struct mtk_mdp_dev *mdp = ctx->mdp_dev;

	if (reqbufs->type != V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE &&
	    reqbufs->type != V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		dev_err(&mdp->pdev->dev, "Wrong buffer type %d",
			reqbufs->type);
		return -EINVAL;
	}

	if (reqbufs->count == 0) {
		if (reqbufs->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE)
			mtk_mdp_ctx_state_lock_clear(ctx, MTK_MDP_SRC_FMT);
		else
			mtk_mdp_ctx_state_lock_clear(ctx, MTK_MDP_DST_FMT);
	}

	return v4l2_m2m_reqbufs(file, ctx->m2m_ctx, reqbufs);
}

static int mtk_mdp_m2m_expbuf(struct file *file, void *fh,
			      struct v4l2_exportbuffer *eb)
{
	struct mtk_mdp_ctx *ctx = fh_to_ctx(fh);

	return v4l2_m2m_expbuf(file, ctx->m2m_ctx, eb);
}

static int mtk_mdp_m2m_querybuf(struct file *file, void *fh,
				struct v4l2_buffer *buf)
{
	struct mtk_mdp_ctx *ctx = fh_to_ctx(fh);

	return v4l2_m2m_querybuf(file, ctx->m2m_ctx, buf);
}

static int mtk_mdp_m2m_qbuf(struct file *file, void *fh,
			    struct v4l2_buffer *buf)
{
	struct mtk_mdp_ctx *ctx = fh_to_ctx(fh);

	return v4l2_m2m_qbuf(file, ctx->m2m_ctx, buf);
}

static int mtk_mdp_m2m_dqbuf(struct file *file, void *fh,
			     struct v4l2_buffer *buf)
{
	struct mtk_mdp_ctx *ctx = fh_to_ctx(fh);

	return v4l2_m2m_dqbuf(file, ctx->m2m_ctx, buf);
}

static int mtk_mdp_m2m_streamon(struct file *file, void *fh,
				enum v4l2_buf_type type)
{
	struct mtk_mdp_ctx *ctx = fh_to_ctx(fh);

	/* The source and target color format need to be set */
	if (V4L2_TYPE_IS_OUTPUT(type)) {
		if (!mtk_mdp_ctx_state_is_set(ctx, MTK_MDP_SRC_FMT))
			return -EINVAL;
	} else if (!mtk_mdp_ctx_state_is_set(ctx, MTK_MDP_DST_FMT)) {
		return -EINVAL;
	}

	return v4l2_m2m_streamon(file, ctx->m2m_ctx, type);
}

static int mtk_mdp_m2m_streamoff(struct file *file, void *fh,
			    enum v4l2_buf_type type)
{
	struct mtk_mdp_ctx *ctx = fh_to_ctx(fh);

	return v4l2_m2m_streamoff(file, ctx->m2m_ctx, type);
}

static inline bool mtk_mdp_is_target_compose(u32 target)
{
	if (target == V4L2_SEL_TGT_COMPOSE_DEFAULT
	    || target == V4L2_SEL_TGT_COMPOSE_BOUNDS
	    || target == V4L2_SEL_TGT_COMPOSE)
		return true;
	return false;
}

static inline bool mtk_mdp_is_target_crop(u32 target)
{
	if (target == V4L2_SEL_TGT_CROP_DEFAULT
	    || target == V4L2_SEL_TGT_CROP_BOUNDS
	    || target == V4L2_SEL_TGT_CROP)
		return true;
	return false;
}

static int mtk_mdp_m2m_g_selection(struct file *file, void *fh,
				   struct v4l2_selection *s)
{
	struct mtk_mdp_frame *frame;
	struct mtk_mdp_ctx *ctx = fh_to_ctx(fh);
	bool valid = false;

	if (s->type == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
		if (mtk_mdp_is_target_compose(s->target))
			valid = true;
	} else if (s->type == V4L2_BUF_TYPE_VIDEO_OUTPUT) {
		if (mtk_mdp_is_target_crop(s->target))
			valid = true;
	}
	if (!valid) {
		mtk_mdp_dbg(1, "[%d] invalid type:%d,%u", ctx->id, s->type,
			    s->target);
		return -EINVAL;
	}

	frame = mtk_mdp_ctx_get_frame(ctx, s->type);

	switch (s->target) {
	case V4L2_SEL_TGT_COMPOSE_DEFAULT:
	case V4L2_SEL_TGT_COMPOSE_BOUNDS:
	case V4L2_SEL_TGT_CROP_BOUNDS:
	case V4L2_SEL_TGT_CROP_DEFAULT:
		s->r.left = 0;
		s->r.top = 0;
		s->r.width = frame->width;
		s->r.height = frame->height;
		return 0;

	case V4L2_SEL_TGT_COMPOSE:
	case V4L2_SEL_TGT_CROP:
		s->r.left = frame->crop.left;
		s->r.top = frame->crop.top;
		s->r.width = frame->crop.width;
		s->r.height = frame->crop.height;
		return 0;
	}

	return -EINVAL;
}

static int mtk_mdp_check_scaler_ratio(struct mtk_mdp_variant *var, int src_w,
				      int src_h, int dst_w, int dst_h, int rot)
{
	int tmp_w, tmp_h;

	if (rot == 90 || rot == 270) {
		tmp_w = dst_h;
		tmp_h = dst_w;
	} else {
		tmp_w = dst_w;
		tmp_h = dst_h;
	}

	if ((src_w / tmp_w) > var->h_scale_down_max ||
	    (src_h / tmp_h) > var->v_scale_down_max ||
	    (tmp_w / src_w) > var->h_scale_up_max ||
	    (tmp_h / src_h) > var->v_scale_up_max)
		return -EINVAL;

	return 0;
}

static int mtk_mdp_m2m_s_selection(struct file *file, void *fh,
				   struct v4l2_selection *s)
{
	struct mtk_mdp_frame *frame;
	struct mtk_mdp_ctx *ctx = fh_to_ctx(fh);
	struct v4l2_rect new_r;
	struct mtk_mdp_variant *variant = ctx->mdp_dev->variant;
	int ret;
	bool valid = false;

	if (s->type == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
		if (s->target == V4L2_SEL_TGT_COMPOSE)
			valid = true;
	} else if (s->type == V4L2_BUF_TYPE_VIDEO_OUTPUT) {
		if (s->target == V4L2_SEL_TGT_CROP)
			valid = true;
	}
	if (!valid) {
		mtk_mdp_dbg(1, "[%d] invalid type:%d,%u", ctx->id, s->type,
			    s->target);
		return -EINVAL;
	}

	new_r = s->r;
	ret = mtk_mdp_try_crop(ctx, s->type, &new_r);
	if (ret)
		return ret;

	if (mtk_mdp_is_target_crop(s->target))
		frame = &ctx->s_frame;
	else
		frame = &ctx->d_frame;

	/* Check to see if scaling ratio is within supported range */
	if (mtk_mdp_ctx_state_is_set(ctx, MTK_MDP_DST_FMT | MTK_MDP_SRC_FMT)) {
		if (V4L2_TYPE_IS_OUTPUT(s->type)) {
			ret = mtk_mdp_check_scaler_ratio(variant, new_r.width,
				new_r.height, ctx->d_frame.crop.width,
				ctx->d_frame.crop.height,
				ctx->ctrls.rotate->val);
		} else {
			ret = mtk_mdp_check_scaler_ratio(variant,
				ctx->s_frame.crop.width,
				ctx->s_frame.crop.height, new_r.width,
				new_r.height, ctx->ctrls.rotate->val);
		}

		if (ret) {
			dev_info(&ctx->mdp_dev->pdev->dev,
				"Out of scaler range");
			return -EINVAL;
		}
	}

	s->r = new_r;
	frame->crop = new_r;

	return 0;
}

static const struct v4l2_ioctl_ops mtk_mdp_m2m_ioctl_ops = {
	.vidioc_querycap		= mtk_mdp_m2m_querycap,
	.vidioc_enum_fmt_vid_cap_mplane	= mtk_mdp_m2m_enum_fmt_mplane_vid_cap,
	.vidioc_enum_fmt_vid_out_mplane	= mtk_mdp_m2m_enum_fmt_mplane_vid_out,
	.vidioc_g_fmt_vid_cap_mplane	= mtk_mdp_m2m_g_fmt_mplane,
	.vidioc_g_fmt_vid_out_mplane	= mtk_mdp_m2m_g_fmt_mplane,
	.vidioc_try_fmt_vid_cap_mplane	= mtk_mdp_m2m_try_fmt_mplane,
	.vidioc_try_fmt_vid_out_mplane	= mtk_mdp_m2m_try_fmt_mplane,
	.vidioc_s_fmt_vid_cap_mplane	= mtk_mdp_m2m_s_fmt_mplane,
	.vidioc_s_fmt_vid_out_mplane	= mtk_mdp_m2m_s_fmt_mplane,
	.vidioc_reqbufs			= mtk_mdp_m2m_reqbufs,
	.vidioc_expbuf                  = mtk_mdp_m2m_expbuf,
	.vidioc_querybuf		= mtk_mdp_m2m_querybuf,
	.vidioc_qbuf			= mtk_mdp_m2m_qbuf,
	.vidioc_dqbuf			= mtk_mdp_m2m_dqbuf,
	.vidioc_streamon		= mtk_mdp_m2m_streamon,
	.vidioc_streamoff		= mtk_mdp_m2m_streamoff,
	.vidioc_g_selection		= mtk_mdp_m2m_g_selection,
	.vidioc_s_selection		= mtk_mdp_m2m_s_selection
};

static int mtk_mdp_m2m_queue_init(void *priv, struct vb2_queue *src_vq,
				  struct vb2_queue *dst_vq)
{
	struct mtk_mdp_ctx *ctx = priv;
	int ret;

	memset(src_vq, 0, sizeof(*src_vq));
	src_vq->type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	src_vq->io_modes = VB2_MMAP | VB2_USERPTR | VB2_DMABUF;
	src_vq->drv_priv = ctx;
	src_vq->ops = &mtk_mdp_m2m_qops;
	src_vq->mem_ops = &vb2_dma_contig_memops;
	src_vq->buf_struct_size = sizeof(struct v4l2_m2m_buffer);
	src_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;

	ret = vb2_queue_init(src_vq);
	if (ret)
		return ret;

	memset(dst_vq, 0, sizeof(*dst_vq));
	dst_vq->type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	dst_vq->io_modes = VB2_MMAP | VB2_USERPTR | VB2_DMABUF;
	dst_vq->drv_priv = ctx;
	dst_vq->ops = &mtk_mdp_m2m_qops;
	dst_vq->mem_ops = &vb2_dma_contig_memops;
	dst_vq->buf_struct_size = sizeof(struct v4l2_m2m_buffer);
	dst_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;

	return vb2_queue_init(dst_vq);
}

static int mtk_mdp_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct mtk_mdp_ctx *ctx = ctrl_to_ctx(ctrl);
	struct mtk_mdp_dev *mdp = ctx->mdp_dev;
	struct mtk_mdp_variant *variant = mdp->variant;
	u32 state = MTK_MDP_DST_FMT | MTK_MDP_SRC_FMT;
	int ret = 0;

	if (ctrl->flags & V4L2_CTRL_FLAG_INACTIVE)
		return 0;

	switch (ctrl->id) {
	case V4L2_CID_HFLIP:
		ctx->hflip = ctrl->val;
		break;
	case V4L2_CID_VFLIP:
		ctx->vflip = ctrl->val;
		break;
	case V4L2_CID_ROTATE:
		if (mtk_mdp_ctx_state_is_set(ctx, state)) {
			ret = mtk_mdp_check_scaler_ratio(variant,
					ctx->s_frame.crop.width,
					ctx->s_frame.crop.height,
					ctx->d_frame.crop.width,
					ctx->d_frame.crop.height,
					ctx->ctrls.rotate->val);

			if (ret)
				return -EINVAL;
		}

		ctx->rotation = ctrl->val;
		break;
	case V4L2_CID_ALPHA_COMPONENT:
		ctx->d_frame.alpha = ctrl->val;
		break;
	}

	return 0;
}

static const struct v4l2_ctrl_ops mtk_mdp_ctrl_ops = {
	.s_ctrl = mtk_mdp_s_ctrl,
};

static int mtk_mdp_ctrls_create(struct mtk_mdp_ctx *ctx)
{
	v4l2_ctrl_handler_init(&ctx->ctrl_handler, MTK_MDP_MAX_CTRL_NUM);

	ctx->ctrls.rotate = v4l2_ctrl_new_std(&ctx->ctrl_handler,
			&mtk_mdp_ctrl_ops, V4L2_CID_ROTATE, 0, 270, 90, 0);
	ctx->ctrls.hflip = v4l2_ctrl_new_std(&ctx->ctrl_handler,
					     &mtk_mdp_ctrl_ops,
					     V4L2_CID_HFLIP,
					     0, 1, 1, 0);
	ctx->ctrls.vflip = v4l2_ctrl_new_std(&ctx->ctrl_handler,
					     &mtk_mdp_ctrl_ops,
					     V4L2_CID_VFLIP,
					     0, 1, 1, 0);
	ctx->ctrls.global_alpha = v4l2_ctrl_new_std(&ctx->ctrl_handler,
						    &mtk_mdp_ctrl_ops,
						    V4L2_CID_ALPHA_COMPONENT,
						    0, 255, 1, 0);
	ctx->ctrls_rdy = ctx->ctrl_handler.error == 0;

	if (ctx->ctrl_handler.error) {
		int err = ctx->ctrl_handler.error;

		v4l2_ctrl_handler_free(&ctx->ctrl_handler);
		dev_err(&ctx->mdp_dev->pdev->dev,
			"Failed to create control handlers\n");
		return err;
	}

	return 0;
}

static int mtk_mdp_m2m_open(struct file *file)
{
	struct mtk_mdp_dev *mdp = video_drvdata(file);
	struct video_device *vfd = video_devdata(file);
	struct mtk_mdp_ctx *ctx = NULL;
	int ret;

	if (mutex_lock_interruptible(&mdp->lock))
		return -ERESTARTSYS;

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx) {
		ret = -ENOMEM;
		goto err_ctx_alloc;
	}

	mutex_init(&ctx->slock);
	ctx->id = mdp->id_counter++;
	v4l2_fh_init(&ctx->fh, vfd);
	file->private_data = &ctx->fh;
	ret = mtk_mdp_ctrls_create(ctx);
	if (ret)
		goto error_ctrls;

	/* Use separate control handler per file handle */
	ctx->fh.ctrl_handler = &ctx->ctrl_handler;
	v4l2_fh_add(&ctx->fh);
	INIT_LIST_HEAD(&ctx->list);

	ctx->mdp_dev = mdp;
	/* Default color format */
	ctx->s_frame.fmt = mtk_mdp_find_fmt_by_index(0,
					V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE);
	ctx->d_frame.fmt = mtk_mdp_find_fmt_by_index(0,
					V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);

	INIT_WORK(&ctx->work, mtk_mdp_m2m_worker);
	ctx->m2m_ctx = v4l2_m2m_ctx_init(mdp->m2m_dev, ctx,
					 mtk_mdp_m2m_queue_init);
	if (IS_ERR(ctx->m2m_ctx)) {
		dev_err(&mdp->pdev->dev, "Failed to initialize m2m context");
		ret = PTR_ERR(ctx->m2m_ctx);
		goto error_m2m_ctx;
	}
	ctx->fh.m2m_ctx = ctx->m2m_ctx;
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
		ret = -EINVAL;
		goto err_load_vpu;
	}
	list_add(&ctx->list, &mdp->ctx_list);

	ret = cmdq_pkt_create(&ctx->cmdq_handle);
	if (ret) {
		dev_err(&mdp->pdev->dev, "Create cmdq ptk failed %d\n", ret);
		goto err_create_cmdq_pkt;
	}

	mutex_unlock(&mdp->lock);

	mtk_mdp_dbg(0, "%s [%d]", dev_name(&mdp->pdev->dev), ctx->id);

	return 0;

err_create_cmdq_pkt:
err_load_vpu:
	mdp->ctx_num--;
	v4l2_m2m_ctx_release(ctx->m2m_ctx);
error_m2m_ctx:
	v4l2_ctrl_handler_free(&ctx->ctrl_handler);
error_ctrls:
	v4l2_fh_del(&ctx->fh);
	v4l2_fh_exit(&ctx->fh);
	kfree(ctx);
err_ctx_alloc:
	mutex_unlock(&mdp->lock);

	return ret;
}

static int mtk_mdp_m2m_release(struct file *file)
{
	struct mtk_mdp_ctx *ctx = fh_to_ctx(file->private_data);
	struct mtk_mdp_dev *mdp = ctx->mdp_dev;

	flush_workqueue(mdp->job_wq);
	mutex_lock(&mdp->lock);
	v4l2_m2m_ctx_release(ctx->m2m_ctx);
	v4l2_ctrl_handler_free(&ctx->ctrl_handler);
	v4l2_fh_del(&ctx->fh);
	v4l2_fh_exit(&ctx->fh);
	mtk_mdp_vpu_deinit(&ctx->vpu);
	mdp->ctx_num--;
	list_del_init(&ctx->list);
	cmdq_pkt_destroy(ctx->cmdq_handle);

	mtk_mdp_dbg(0, "%s [%d]", dev_name(&mdp->pdev->dev), ctx->id);
	mtk_mdp_dbg(0, "ctx_num=%d", mdp->ctx_num);

	mutex_unlock(&mdp->lock);
	kfree(ctx);

	return 0;
}

static unsigned int mtk_mdp_m2m_poll(struct file *file,
				     struct poll_table_struct *wait)
{
	struct mtk_mdp_ctx *ctx = fh_to_ctx(file->private_data);
	struct mtk_mdp_dev *mdp = ctx->mdp_dev;
	int ret;

	if (mutex_lock_interruptible(&mdp->lock))
		return -ERESTARTSYS;

	ret = v4l2_m2m_poll(file, ctx->m2m_ctx, wait);
	mutex_unlock(&mdp->lock);

	return ret;
}

static int mtk_mdp_m2m_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct mtk_mdp_ctx *ctx = fh_to_ctx(file->private_data);
	struct mtk_mdp_dev *mdp = ctx->mdp_dev;
	int ret;

	if (mutex_lock_interruptible(&mdp->lock))
		return -ERESTARTSYS;

	ret = v4l2_m2m_mmap(file, ctx->m2m_ctx, vma);
	mutex_unlock(&mdp->lock);

	return ret;
}

static const struct v4l2_file_operations mtk_mdp_m2m_fops = {
	.owner		= THIS_MODULE,
	.open		= mtk_mdp_m2m_open,
	.release	= mtk_mdp_m2m_release,
	.poll		= mtk_mdp_m2m_poll,
	.unlocked_ioctl	= video_ioctl2,
	.mmap		= mtk_mdp_m2m_mmap,
};

static struct v4l2_m2m_ops mtk_mdp_m2m_ops = {
	.device_run	= mtk_mdp_m2m_device_run,
	.job_abort	= mtk_mdp_m2m_job_abort,
};

int mtk_mdp_register_m2m_device(struct mtk_mdp_dev *mdp)
{
	struct device *dev = &mdp->pdev->dev;
	int ret;

	mdp->variant = &mtk_mdp_default_variant;
#if MTK_MDP_SYNC
	mdp->vdev.device_caps = V4L2_CAP_VIDEO_M2M_MPLANE | V4L2_CAP_STREAMING;
#endif
	mdp->vdev.fops = &mtk_mdp_m2m_fops;
	mdp->vdev.ioctl_ops = &mtk_mdp_m2m_ioctl_ops;
	mdp->vdev.release = video_device_release_empty;
	mdp->vdev.lock = &mdp->lock;
	mdp->vdev.vfl_dir = VFL_DIR_M2M;
	mdp->vdev.v4l2_dev = &mdp->v4l2_dev;
	snprintf(mdp->vdev.name, sizeof(mdp->vdev.name), "%s:m2m",
		 MTK_MDP_MODULE_NAME);
	video_set_drvdata(&mdp->vdev, mdp);

	mdp->m2m_dev = v4l2_m2m_init(&mtk_mdp_m2m_ops);
	if (IS_ERR(mdp->m2m_dev)) {
		dev_err(dev, "failed to initialize v4l2-m2m device\n");
		ret = PTR_ERR(mdp->m2m_dev);
		goto err_m2m_init;
	}

	ret = video_register_device(&mdp->vdev, VFL_TYPE_GRABBER, 2);
	if (ret) {
		dev_err(dev, "failed to register video device\n");
		goto err_vdev_register;
	}

	v4l2_info(&mdp->v4l2_dev, "driver registered as /dev/video%d",
		  mdp->vdev.num);
	return 0;

err_vdev_register:
	v4l2_m2m_release(mdp->m2m_dev);
err_m2m_init:
	video_device_release(&mdp->vdev);

	return ret;
}

void mtk_mdp_unregister_m2m_device(struct mtk_mdp_dev *mdp)
{
	video_device_release(&mdp->vdev);
	v4l2_m2m_release(mdp->m2m_dev);
}
