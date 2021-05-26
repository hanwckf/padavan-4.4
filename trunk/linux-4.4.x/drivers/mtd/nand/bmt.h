/*
 * MTK bad block mapping table
 *
 * Copyright (c) 2017 Mediatek
 * Authors:	Bayi Cheng		<bayi.cheng@mediatek.com>
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#ifndef __BMT_H__
#define __BMT_H__

#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include "mtk_snand.h"

#define MAX_BMT_SIZE		(0x80)
#define BMT_VERSION		(1) /* initial version */

#define MAIN_SIGNATURE_OFFSET	(0)
#define OOB_SIGNATURE_OFFSET	(1)
#define OOB_INDEX_OFFSET	(29)
#define OOB_INDEX_SIZE		(2)
#define FAKE_INDEX		(0xAAAA)

typedef struct _bmt_entry_ {
	u16 bad_index;	/* bad block index */
	u16 mapped_index;
} bmt_entry;

typedef enum {
	UPDATE_ERASE_FAIL,
	UPDATE_WRITE_FAIL,
	UPDATE_UNMAPPED_BLOCK,
	UPDATE_REASON_COUNT,
} update_reason_t;

typedef struct {
	bmt_entry table[MAX_BMT_SIZE];
	u8 version;
	u8 mapped_count;	/* mapped block count in pool */
	u8 bad_count;
} bmt_struct;

/***************************************************************
*                                                              *
* Different function interface for preloader/uboot/kernel      *
*                                                              *
***************************************************************/
void set_bad_index_to_oob(u8 *oob, u16 index);
int init_bmt(struct mtk_snand_host *host, unsigned short total_blocks,
		unsigned short pmt_block);
bool update_bmt(unsigned long long offset, update_reason_t reason,
		unsigned char *dat, unsigned char *oob);

unsigned short get_mapping_block_index(int index);

#endif
