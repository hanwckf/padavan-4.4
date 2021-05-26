/*
 * MTK spi nand controller
 *
 * Copyright (c) 2017 Mediatek
 * Authors:	Bayi Cheng		<bayi.cheng@mediatek.com>
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#ifndef __MTK_SNAND_K_H__
#define __MTK_SNAND_K_H__

/***************************************************************
*                                                              *
* Interface BMT need to use                                    *
*                                                              *
***************************************************************/
extern bool mtk_nand_exec_read_page(struct mtd_info *mtd, u32 row,
				    u32 page_size, u8 *dat, u8 *oob);
extern int mtk_nand_block_bad_hw(struct mtd_info *mtd, loff_t ofs);
extern int mtk_nand_erase_hw(struct mtd_info *mtd, int page);
extern int mtk_nand_block_markbad_hw(struct mtd_info *mtd, loff_t ofs);
extern int mtk_nand_exec_write_page(struct mtd_info *mtd, u32 row,
				    u32 page_size, u8 *dat, u8 *oob, u8 oobraw);

struct mtk_snand_host_hw {
	unsigned int nfi_bus_width;		    /* NFI_BUS_WIDTH */
	unsigned int nfi_access_timing;		/* NFI_ACCESS_TIMING */
	unsigned int nfi_cs_num;			/* NFI_CS_NUM */
	unsigned int nand_sec_size;			/* NAND_SECTOR_SIZE */
	unsigned int nand_sec_shift;		/* NAND_SECTOR_SHIFT */
	unsigned int nand_ecc_size;
	unsigned int nand_ecc_bytes;
	unsigned int nand_ecc_mode;
	unsigned int nand_fdm_size;
};

struct mtk_snand_host {
	struct nand_chip		nand_chip;
	struct mtd_info			mtd;
	struct mtk_snand_host_hw	*hw;
};

/* For SLC Nand, 98% valid block is guaranteed */
#define BBPOOL_RATIO	2
#define ERR_RTN_SUCCESS   1
#define ERR_RTN_FAIL	  0
#define ERR_RTN_BCH_FAIL -1

#endif
