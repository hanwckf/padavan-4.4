/*
 * Copyright (c) 2014-2015 MediaTek Inc.
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

#ifndef __MT_MSDC_DEUBG__
#define __MT_MSDC_DEUBG__

extern u32 sdio_pro_enable;
/* for a type command, e.g. CMD53, 2 blocks */
struct cmd_profile {
	u32 max_tc;    /* Max tick count */
	u32 min_tc;
	u32 tot_tc;    /* total tick count */
	u32 tot_bytes;
	u32 count;     /* the counts of the command */
};

/* dump when total_tc and total_bytes */
struct sdio_profile {
	u32 total_tc;         /* total tick count of CMD52 and CMD53 */
	u32 total_tx_bytes;   /* total bytes of CMD53 Tx */
	u32 total_rx_bytes;   /* total bytes of CMD53 Rx */

	/*CMD52*/
	struct cmd_profile cmd52_tx;
	struct cmd_profile cmd52_rx;

	/*CMD53 in byte unit */
	struct cmd_profile cmd53_tx_byte[512];
	struct cmd_profile cmd53_rx_byte[512];

	/*CMD53 in block unit */
	struct cmd_profile cmd53_tx_blk[100];
	struct cmd_profile cmd53_rx_blk[100];
};

enum msdc_dbg {
	SD_TOOL_ZONE = 0,
	SD_TOOL_DMA_SIZE  = 1,
	SD_TOOL_PM_ENABLE = 2,
	SD_TOOL_SDIO_PROFILE = 3,
};

/* Debug message event */
#define DBG_EVT_NONE        (0)     /* No event */
#define DBG_EVT_DMA         BIT(0)  /* DMA related event */
#define DBG_EVT_CMD         BIT(1)  /* MSDC CMD related event */
#define DBG_EVT_RSP         BIT(2)  /* MSDC CMD RSP related event */
#define DBG_EVT_INT         BIT(3)  /* MSDC INT event */
#define DBG_EVT_CFG         BIT(4)  /* MSDC CFG event */
#define DBG_EVT_FUC         BIT(5)  /* Function event */
#define DBG_EVT_OPS         BIT(6)  /* Read/Write operation event */
#define DBG_EVT_FIO         BIT(7)  /* FIFO operation event */
#define DBG_EVT_WRN         BIT(8)  /* Warning event */
#define DBG_EVT_PWR         BIT(9)  /* Power event */
#define DBG_EVT_ALL         (0xffffffff)

#define DBG_EVT_MASK        (DBG_EVT_ALL)

extern unsigned int sd_debug_zone[4];
#define TAG "msdc"
void msdc_debug_proc_init(void);

u32 msdc_time_calc(u32 old_L32, u32 old_H32, u32 new_L32, u32 new_H32);
void msdc_performance(u32 opcode, u32 sizes, u32 bRx, u32 ticks);

#endif
