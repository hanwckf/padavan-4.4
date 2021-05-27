/*
 * Copyright (c) 2016 MediaTek Inc.
 * Author: PC Chen <pc.chen@mediatek.com>
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

#ifndef _VDEC_VCU_IF_H_
#define _VDEC_VCU_IF_H_

#include <linux/dma-buf.h>
#include "mtk_vcu.h"

/**
 * struct vdec_vcu_inst - VCU instance for video codec
 * @ipi_id      : ipi id for each decoder
 * @vsi         : driver structure allocated by VCU side and shared to AP side
 *                for control and info share
 * @failure     : VCU execution result status, 0: success, others: fail
 * @inst_addr	: VCU decoder instance address
 * @signaled    : 1 - Host has received ack message from VCU, 0 - not received
 * @ctx         : context for v4l2 layer integration
 * @dev	        : platform device of VCU
 * @wq          : wait queue to wait VCU message ack
 * @handler     : ipi handler for each decoder
 */
struct vdec_vcu_inst {
	enum ipi_id id;
	void *vsi;
	int32_t failure;
	uint32_t inst_addr;
	unsigned int signaled;
	struct mtk_vcodec_ctx *ctx;
	struct platform_device *dev;
	wait_queue_head_t wq;
	ipi_handler_t handler;
};

/**
 * vcu_dec_init - init decoder instance and allocate required resource in VCU.
 *
 * @vcu: instance for vdec_vcu_inst
 */
int vcu_dec_init(struct vdec_vcu_inst *vcu);

/**
 * vcu_dec_start - start decoding, basically the function will be invoked once
 *                 every frame.
 *
 * @vcu : instance for vdec_vcu_inst
 * @data: meta data to pass bitstream info to VCU decoder
 * @len : meta data length
 */
int vcu_dec_start(struct vdec_vcu_inst *vcu, uint32_t *data, unsigned int len);

/**
 * vcu_dec_end - end decoding, basically the function will be invoked once
 *               when HW decoding done interrupt received successfully. The
 *               decoder in VCU will continute to do referene frame management
 *               and check if there is a new decoded frame available to display.
 *
 * @vcu : instance for vdec_vcu_inst
 */
int vcu_dec_end(struct vdec_vcu_inst *vcu);

/**
 * vcu_dec_deinit - deinit decoder instance and resource freed in VCU.
 *
 * @vcu: instance for vdec_vcu_inst
 */
int vcu_dec_deinit(struct vdec_vcu_inst *vcu);

/**
 * vcu_dec_reset - reset decoder, use for flush decoder when end of stream or
 *                 seek. Remainig non displayed frame will be pushed to display.
 *
 * @vcu: instance for vdec_vcu_inst
 */
int vcu_dec_reset(struct vdec_vcu_inst *vcu);

/**
 * vcu_dec_ipi_handler - Handler for VCU ipi message.
 *
 * @data: ipi message
 * @len : length of ipi message
 * @priv: callback private data which is passed by decoder when register.
 */
int vcu_dec_ipi_handler(void *data, unsigned int len, void *priv);
int vcu_dec_set_param(struct vdec_vcu_inst *vcu, unsigned int id,
		      void *param, unsigned int size);
int get_mapped_fd(struct dma_buf *dmabuf);
#endif
