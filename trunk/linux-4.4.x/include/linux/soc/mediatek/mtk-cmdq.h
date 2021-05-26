/*
 * Copyright (c) 2015 MediaTek Inc.
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

#ifndef __MTK_CMDQ_H__
#define __MTK_CMDQ_H__

#include <linux/mailbox_client.h>
#include <linux/mailbox/mtk-cmdq-mailbox.h>

/* display events in command queue(CMDQ) */
enum cmdq_event {
	/* MDP start of frame(SOF) events */
	CMDQ_EVENT_MDP_RDMA0_SOF,
	CMDQ_EVENT_MDP_RDMA1_SOF,
	CMDQ_EVENT_MDP_DSI0_TE_SOF,
	CMDQ_EVENT_MDP_DSI1_TE_SOF,
	CMDQ_EVENT_MDP_MVW_SOF,
	CMDQ_EVENT_MDP_TDSHP0_SOF,
	CMDQ_EVENT_MDP_TDSHP1_SOF,
	CMDQ_EVENT_MDP_WDMA_SOF,
	CMDQ_EVENT_MDP_WROT0_SOF,
	CMDQ_EVENT_MDP_WROT1_SOF,
	CMDQ_EVENT_MDP_CROP_SOF,
	/* Display start of frame(SOF) events */
	CMDQ_EVENT_DISP_OVL0_SOF,
	CMDQ_EVENT_DISP_OVL1_SOF,
	CMDQ_EVENT_DISP_RDMA0_SOF,
	CMDQ_EVENT_DISP_RDMA1_SOF,
	CMDQ_EVENT_DISP_RDMA2_SOF,
	CMDQ_EVENT_DISP_WDMA0_SOF,
	CMDQ_EVENT_DISP_WDMA1_SOF,
	/* MDP end of frame(EOF) events */
	CMDQ_EVENT_MDP_RDMA0_EOF,
	CMDQ_EVENT_MDP_RDMA1_EOF,
	CMDQ_EVENT_MDP_RSZ0_EOF,
	CMDQ_EVENT_MDP_RSZ1_EOF,
	CMDQ_EVENT_MDP_RSZ2_EOF,
	CMDQ_EVENT_MDP_TDSHP0_EOF,
	CMDQ_EVENT_MDP_TDSHP1_EOF,
	CMDQ_EVENT_MDP_WDMA_EOF,
	CMDQ_EVENT_MDP_WROT0_W_EOF,
	CMDQ_EVENT_MDP_WROT0_R_EOF,
	CMDQ_EVENT_MDP_WROT1_W_EOF,
	CMDQ_EVENT_MDP_WROT1_R_EOF,
	CMDQ_EVENT_MDP_CROP_EOF,
	/* Display end of frame(EOF) events */
	CMDQ_EVENT_DISP_OVL0_EOF,
	CMDQ_EVENT_DISP_OVL1_EOF,
	CMDQ_EVENT_DISP_RDMA0_EOF,
	CMDQ_EVENT_DISP_RDMA1_EOF,
	CMDQ_EVENT_DISP_RDMA2_EOF,
	CMDQ_EVENT_DISP_WDMA0_EOF,
	CMDQ_EVENT_DISP_WDMA1_EOF,
	/* Mutex end of frame(EOF) events */
	CMDQ_EVENT_MUTEX0_STREAM_EOF,
	CMDQ_EVENT_MUTEX1_STREAM_EOF,
	CMDQ_EVENT_MUTEX2_STREAM_EOF,
	CMDQ_EVENT_MUTEX3_STREAM_EOF,
	CMDQ_EVENT_MUTEX4_STREAM_EOF,
	/* Display underrun events */
	CMDQ_EVENT_DISP_RDMA0_UNDERRUN,
	CMDQ_EVENT_DISP_RDMA1_UNDERRUN,
	CMDQ_EVENT_DISP_RDMA2_UNDERRUN,
	/* Keep this at the end */
	CMDQ_MAX_EVENT,
};

/* General Purpose Register */
enum cmdq_gpr_reg {
	/* Value Reg, we use 32-bit */
	/* Address Reg, we use 64-bit */
	/* Note that R0-R15 and P0-P7 actullay share same memory */
	/* and R1 cannot be used. */

	CMDQ_DATA_REG_JPEG = 0x00,	/* R0 */
	CMDQ_DATA_REG_JPEG_DST = 0x11,	/* P1 */

	CMDQ_DATA_REG_PQ_COLOR = 0x04,	/* R4 */
	CMDQ_DATA_REG_PQ_COLOR_DST = 0x13,	/* P3 */

	CMDQ_DATA_REG_2D_SHARPNESS_0 = 0x05,	/* R5 */
	CMDQ_DATA_REG_2D_SHARPNESS_0_DST = 0x14,	/* P4 */

	CMDQ_DATA_REG_2D_SHARPNESS_1 = 0x0a,	/* R10 */
	CMDQ_DATA_REG_2D_SHARPNESS_1_DST = 0x16,	/* P6 */

	CMDQ_DATA_REG_DEBUG = 0x0b,	/* R11 */
	CMDQ_DATA_REG_DEBUG_DST = 0x17,	/* P7 */

	/* sentinel value for invalid register ID */
	CMDQ_DATA_REG_INVALID = -1,
};

struct cmdq_pkt;

struct cmdq_base {
	int	subsys;
	u32	base;
};

struct cmdq_client {
	struct mbox_client client;
	struct mbox_chan *chan;
};

/**
 * cmdq_register_device() - register device which needs CMDQ
 * @dev:	device for CMDQ to access its registers
 *
 * Return: cmdq_base pointer or NULL for failed
 */
struct cmdq_base *cmdq_register_device(struct device *dev);

/**
 * cmdq_mbox_create() - create CMDQ mailbox client and channel
 * @dev:	device of CMDQ mailbox client
 * @index:	index of CMDQ mailbox channel
 *
 * Return: CMDQ mailbox client pointer
 */
struct cmdq_client *cmdq_mbox_create(struct device *dev, int index);

/**
 * cmdq_mbox_destroy() - destroy CMDQ mailbox client and channel
 * @client:	the CMDQ mailbox client
 */
void cmdq_mbox_destroy(struct cmdq_client *client);

/**
 * cmdq_pkt_create() - create a CMDQ packet
 * @pkt_ptr:	CMDQ packet pointer to retrieve cmdq_pkt
 *
 * Return: 0 for success; else the error code is returned
 */
int cmdq_pkt_create(struct cmdq_pkt **pkt_ptr);

/**
 * cmdq_pkt_destroy() - destroy the CMDQ packet
 * @pkt:	the CMDQ packet
 */
void cmdq_pkt_destroy(struct cmdq_pkt *pkt);

/**
 * cmdq_pkt_realloc_cmd_buffer() - reallocate command buffer for CMDQ packet
 * @pkt:	the CMDQ packet
 * @size:	the request size
 * Return: 0 for success; else the error code is returned
 */
int cmdq_pkt_realloc_cmd_buffer(struct cmdq_pkt *pkt, size_t size);

/**
 * cmdq_pkt_read() - append read command to the CMDQ packet
 * @pkt:	the CMDQ packet
 * @base:	the CMDQ base
 * @offset:	register offset from module base
 * @writeAddress:
 * @valueRegId:
 * @destRegId:
 * Return: 0 for success; else the error code is returned
 */
int cmdq_pkt_read(struct cmdq_pkt *pkt,
			struct cmdq_base *base, u32 offset, u32 writeAddress,
			enum cmdq_gpr_reg valueRegId,
			enum cmdq_gpr_reg destRegId);

/**
 * cmdq_pkt_write() - append write command to the CMDQ packet
 * @pkt:	the CMDQ packet
 * @value:	the specified target register value
 * @base:	the CMDQ base
 * @offset:	register offset from module base
 *
 * Return: 0 for success; else the error code is returned
 */
int cmdq_pkt_write(struct cmdq_pkt *pkt, u32 value,
		   struct cmdq_base *base, u32 offset);

/**
 * cmdq_pkt_write_mask() - append write command with mask to the CMDQ packet
 * @pkt:	the CMDQ packet
 * @value:	the specified target register value
 * @base:	the CMDQ base
 * @offset:	register offset from module base
 * @mask:	the specified target register mask
 *
 * Return: 0 for success; else the error code is returned
 */
int cmdq_pkt_write_mask(struct cmdq_pkt *pkt, u32 value,
			struct cmdq_base *base, u32 offset, u32 mask);

/**
 * cmdq_pkt_poll() - append polling command with mask to the CMDQ packet
 * @pkt:	the CMDQ packet
 * @value:	the specified target register value
 * @base:	the CMDQ base
 * @offset:	register offset from module base
 *
 * Return: 0 for success; else the error code is returned
 */
int cmdq_pkt_poll(struct cmdq_pkt *pkt, u32 value,
			 struct cmdq_base *base, u32 offset);

/**
 * cmdq_pkt_poll_t() - append polling command with mask to the CMDQ packet
 * @pkt:	the CMDQ packet
 * @value:	the specified target register value
 * @base:	the CMDQ base
 * @offset:	register offset from module base
 * @mask:	the specified target register mask
 *
 * Return: 0 for success; else the error code is returned
 */
int cmdq_pkt_poll_mask(struct cmdq_pkt *pkt, u32 value,
			struct cmdq_base *base, uint32_t offset, uint32_t mask);

/**
 * cmdq_pkt_wfe() - append wait for event command to the CMDQ packet
 * @pkt:	the CMDQ packet
 * @event:	the desired event type to "wait and CLEAR"
 *
 * Return: 0 for success; else the error code is returned
 */
int cmdq_pkt_wfe(struct cmdq_pkt *pkt, enum cmdq_event event);

/**
 * cmdq_pkt_clear_event() - append clear event command to the CMDQ packet
 * @pkt:	the CMDQ packet
 * @event:	the desired event to be cleared
 *
 * Return: 0 for success; else the error code is returned
 */
int cmdq_pkt_clear_event(struct cmdq_pkt *pkt, enum cmdq_event event);

/**
 * cmdq_pkt_flush() - trigger CMDQ to execute the CMDQ packet
 * @client:	the CMDQ mailbox client
 * @pkt:	the CMDQ packet
 *
 * Return: 0 for success; else the error code is returned
 *
 * Trigger CMDQ to execute the CMDQ packet. Note that this is a
 * synchronous flush function. When the function returned, the recorded
 * commands have been done.
 */
int cmdq_pkt_flush(struct cmdq_client *client, struct cmdq_pkt *pkt);

/**
 * cmdq_pkt_flush_async() - trigger CMDQ to asynchronously execute the CMDQ
 *                          packet and call back at the end of done packet
 * @client:	the CMDQ mailbox client
 * @pkt:	the CMDQ packet
 * @cb:		called at the end of done packet
 * @data:	this data will pass back to cb
 *
 * Return: 0 for success; else the error code is returned
 *
 * Trigger CMDQ to asynchronously execute the CMDQ packet and call back
 * at the end of done packet. Note that this is an ASYNC function. When the
 * function returned, it may or may not be finished.
 */
int cmdq_pkt_flush_async(struct cmdq_client *client, struct cmdq_pkt *pkt,
			 cmdq_async_flush_cb cb, void *data);

/* debug */
extern const u32 cmdq_event_value[CMDQ_MAX_EVENT];
u32 cmdq_subsys_id_to_base(int id);

#endif	/* __MTK_CMDQ_H__ */
