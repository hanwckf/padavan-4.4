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

#ifndef __MTK_MDP_DRM__H__
#define __MTK_MDP_DRM_H__


void mtk_mdp_drm_init(struct mtk_mdp_dev *mdp_dev);
void mtk_mdp_drm_deinit(void);
struct mtk_mdp_ctx *mtk_mdp_create_ctx(void);
int mtk_mdp_destroy_ctx(struct mtk_mdp_ctx *ctx);
void mtk_mdp_set_input_addr(struct mtk_mdp_ctx *ctx,
				struct mtk_mdp_addr *addr);
void mtk_mdp_set_output_addr(struct mtk_mdp_ctx *ctx,
				struct mtk_mdp_addr *addr);
void mtk_mdp_set_in_size(struct mtk_mdp_ctx *ctx,
				struct mtk_mdp_frame *frame);
void mtk_mdp_set_in_image_format(struct mtk_mdp_ctx *ctx,
				struct mtk_mdp_frame *frame);
void mtk_mdp_set_out_size(struct mtk_mdp_ctx *ctx,
				struct mtk_mdp_frame *frame);
void mtk_mdp_set_out_image_format(struct mtk_mdp_ctx *ctx,
				struct mtk_mdp_frame *frame);
void mtk_mdp_set_rotation(struct mtk_mdp_ctx *ctx,
				int rotate, int hflip, int vflip);
void mtk_mdp_set_global_alpha(struct mtk_mdp_ctx *ctx,
				int alpha);


#endif /* __MTK_MDP_DRM_H__ */
