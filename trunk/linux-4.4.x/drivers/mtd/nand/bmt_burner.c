/*
 * Copyright (c) 2017 MediaTek Inc.
 * Author: Xiangsheng Hou <xiangsheng.hou@mediatek.com>
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

#include <linux/slab.h>
#include <linux/gfp.h>
#include <linux/kernel.h>
#include <linux/mtd/nand.h>
#include <linux/module.h>
#include "mtk_snand.h"
#include "bmt.h"

#define BBT_LOG(fmt, ...) pr_debug("[BBT][%s|%d] "fmt"\n", __func__, __LINE__, ##__VA_ARGS__)

/* Maximum 8k blocks */
#define BB_TABLE_MAX	0x1000U
#define BMT_TABLE_MAX	(BB_TABLE_MAX * BBPOOL_RATIO / 100)
#define BMT_TBL_DEF_VAL	0x0

/*
 * Burner Bad Block Table
 * --------- Only support SLC Nand Chips!!!!!!!!!!! ----------
 */
static struct mtd_info *mtd_bmt;
struct bbbt {
	char signature[3];
	/* This version is used to distinguish the legacy and new algorithm */
#define BBMT_VERSION		2
	unsigned char version;
	/* Below 2 tables will be written in SLC */
	unsigned short bb_tbl[BB_TABLE_MAX];
	struct bbmt {
		unsigned short block;
#define NO_MAPPED		0
#define NORMAL_MAPPED	1
#define BMT_MAPPED		2
		unsigned short mapped;
	} bmt_tbl[BMT_TABLE_MAX];
};

static struct bmt_desc {
	unsigned int pg_size;
	unsigned int blk_size;
	unsigned short pg_shift;
	unsigned short blk_shift;
	unsigned short spare_buf_size;
	/* bbt logical address */
	unsigned short pool_lba;
	/* bbt physical address */
	unsigned short pool_pba;
	/* Maximum count of bad blocks that the vendor guaranteed */
	unsigned short bb_max;
	/* Total blocks of the Nand Chip */
	unsigned short total_blks;
	/* The block(n) BMT is located at (bmt_tbl[n]) */
	unsigned short bmt_blk_idx;
	/* How many pages needs to store 'struct bbbt' */
	unsigned int bmt_pgs;
	struct bbbt *bbt;
	struct mtk_snand_host *host;
} bmtd = {0};

static bool bmt_ready;
static unsigned char *nand_bbt_buf;
static unsigned char *nand_data_buf;
static unsigned char *nand_spare_buf;

/* -------- Unit conversions -------- */
static inline unsigned short pg_blk(unsigned int page)
{
	return (unsigned short)(page >> (bmtd.blk_shift - bmtd.pg_shift));
}

static inline unsigned int blk_pg(unsigned short block)
{
	return (unsigned int)(block << (bmtd.blk_shift - bmtd.pg_shift));
}

static inline unsigned long long pg_byte(unsigned int page)
{
	return (unsigned long long)page << bmtd.pg_shift;
}

static inline unsigned int byte_pg(unsigned long long byte)
{
	return (unsigned int)(byte >> bmtd.pg_shift);
}

static inline unsigned long long blk_byte(unsigned short block)
{
	return (unsigned long long)block << bmtd.blk_shift;
}

static inline unsigned short byte_blk(unsigned long long byte)
{
	return (unsigned short)(byte >> bmtd.blk_shift);
}

static inline unsigned int fdm_sz_per_pg(void)
{
	return (1 << (bmtd.pg_shift - bmtd.host->hw->nand_sec_shift)) *
			bmtd.host->hw->nand_fdm_size;
}

/* -------- Nand operations wrapper -------- */
static inline int bbt_nand_read(unsigned int page, unsigned char *dat,
		unsigned char *fdm)
{
	int ret;

	ret = mtk_nand_exec_read_page(mtd_bmt, page, bmtd.pg_size, dat, fdm);
	return (ret == ERR_RTN_SUCCESS) ? 0 : -1;
}

static inline int bbt_nand_write(unsigned int page, unsigned char *dat,
		unsigned char *fdm)
{
	return mtk_nand_exec_write_page(mtd_bmt, page, bmtd.pg_size, dat, fdm, 0);
}

static inline int bbt_nand_erase(unsigned short block)
{
	int status;

	status = mtk_nand_erase_hw(mtd_bmt, blk_pg(block));
	return (status & NAND_STATUS_FAIL) ? -1 : 0;
}

/* -------- Bad Blocks Management -------- */
void set_bad_index_to_oob(u8 *oob, u16 index)
{
	memcpy(oob + OOB_INDEX_OFFSET, &index, sizeof(index));
}

static int read_bmt(unsigned short block, unsigned char *dat,
		unsigned char *fdm)
{
	unsigned int i, page;

	for (i = 0, page = blk_pg(block); i < bmtd.bmt_pgs; i++) {
		if (bbt_nand_read(page + i, dat + pg_byte(i),
						  fdm + i * fdm_sz_per_pg()))
			return -1;
	}

	return 0;
}

static int write_bmt(unsigned short block, unsigned char *dat,
		unsigned char *fdm)
{
	unsigned int i, page;

	for (i = 0, page = blk_pg(block); i < bmtd.bmt_pgs; i++) {
		if (bbt_nand_write(page + i, dat + pg_byte(i),
						   fdm + i * fdm_sz_per_pg()))
			return -1;
	}

	return 0;
}

static unsigned short find_valid_block(unsigned short block)
{
	int ret;
	int loop = 0;

	if (block >= bmtd.total_blks)
		return 0;

	ret = bbt_nand_read(blk_pg(block), nand_data_buf, nand_spare_buf);
	/* Read the 1st byte of FDM to judege whether it's a bad
	 * or not
	 */
	if (ret || nand_spare_buf[0] != 0xff) {
		pr_info("nand: found bad block 0x%x\n", block);
		if (loop >= bmtd.bb_max) {
			/* Continuous bad blocks should not large than maximum bad blocks
			 * in the other hand, recurse too many times may lead to
			 * stack overflow
			 */
			pr_info("nand: FATAL ERR: too many bad blocks!!\n");
			return 0;
		}
		loop++;
		return find_valid_block(block + 1);
	}

	loop = 0;
	return block;
}

/* Find out all bad blocks, and fill in the mapping table */
static int scan_bad_blocks(struct bbbt *bbt)
{
	int i;
	unsigned short block = 0;

	/* First time download, the block0 MUST NOT be a bad block,
	 * this is guaranteed by vendor
	 */
	bbt->bb_tbl[0] = 0;

	/*
	 * Construct the mapping table of Normal data area(non-PMT/BMTPOOL)
	 * G - Good block; B - Bad block
	 *			---------------------------
	 * physical |G|G|B|G|B|B|G|G|G|G|B|G|B|
	 *			---------------------------
	 * What bb_tbl[i] looks like:
	 *   physical block(i):
	 *			 0 1 2 3 4 5 6 7 8 9 a b c
	 *   mapped block(bb_tbl[i]):
	 *			 0 1 3 6 7 8 9 b ......
	 * ATTENTION:
	 *		If new bad block ocurred(n), search bmt_tbl to find
	 *		a available block(x), and fill in the bb_tbl[n] = x;
	 */
	for (i = 1; i < bmtd.pool_lba; i++) {
		bbt->bb_tbl[i] = find_valid_block(bbt->bb_tbl[i - 1] + 1);
		BBT_LOG("bb_tbl[0x%x] = 0x%x", i, bbt->bb_tbl[i]);
		if (bbt->bb_tbl[i] == 0)
			return -1;
	}

	/* Physical Block start Address of BMT pool */
	bmtd.pool_pba = bbt->bb_tbl[i - 1] + 1;
	if (bmtd.pool_pba >= bmtd.total_blks - 2) {
		pr_info("nand: FATAL ERR: Too many bad blocks!!\n");
		return -1;
	}

	BBT_LOG("pool_pba=0x%x", bmtd.pool_pba);
	i = 0;
	block = bmtd.pool_pba;
	/*
	 * The bmt table is used for runtime bad block mapping
	 * G - Good block; B - Bad block
	 *			---------------------------
	 * physical |G|G|B|G|B|B|G|G|G|G|B|G|B|
	 *			---------------------------
	 *   block:	 0 1 2 3 4 5 6 7 8 9 a b c
	 * What bmt_tbl[i] looks like in initial state:
	 *   i:
	 *			 0 1 2 3 4 5 6 7
	 *   bmt_tbl[i].block:
	 *			 0 1 3 6 7 8 9 b
	 *   bmt_tbl[i].mapped:
	 *			 N N N N N N N B
	 *		N - Not mapped(Available)
	 *		M - Mapped
	 *		B - BMT
	 * ATTENTION:
	 *		BMT always in the last valid block in pool
	 */
	while ((block = find_valid_block(block)) != 0) {
		bbt->bmt_tbl[i].block = block;
		bbt->bmt_tbl[i].mapped = NO_MAPPED;
		BBT_LOG("bmt_tbl[%d].block = 0x%x", i, block);
		block++;
		i++;
	}

	/* i - How many available blocks in pool, which is the length of bmt_tbl[]
	 * bmtd.bmt_blk_idx - bmt_tbl[bmtd.bmt_blk_idx].block => the BMT block
	 */
	bmtd.bmt_blk_idx = i - 1;
	bbt->bmt_tbl[bmtd.bmt_blk_idx].mapped = BMT_MAPPED;

	if (i < 1) {
		pr_info("nand: FATAL ERR: no space to store BMT!!\n");
		return -1;
	}

	pr_info("[BBT] %d available blocks in BMT pool\n", i);

	return 0;
}

static bool is_valid_bmt(unsigned char *buf, unsigned char *fdm)
{
	struct bbbt *bbt = (struct bbbt *)buf;

	if (memcmp(bbt->signature + MAIN_SIGNATURE_OFFSET, "BMT", 3) == 0 &&
		memcmp(fdm + OOB_SIGNATURE_OFFSET, "bmt", 3) == 0) {
		if (bbt->version == BBMT_VERSION)
			return true;
	}
	BBT_LOG("[BBT] BMT Version not match,upgrage preloader and uboot please!");
	return false;
}

static unsigned short get_bmt_index(struct bbmt *bmt)
{
	int i = 0;

	while (bmt[i].block != BMT_TBL_DEF_VAL) {
		if (bmt[i].mapped == BMT_MAPPED)
			return i;
		i++;
	}
	return 0;
}

static struct bbbt *scan_bmt(unsigned short block)
{
	if (block < bmtd.pool_lba)
		return NULL;

	if (read_bmt(block, nand_bbt_buf, nand_spare_buf))
		return scan_bmt(block - 1);

	if (is_valid_bmt(nand_bbt_buf, nand_spare_buf)) {
		bmtd.bmt_blk_idx = get_bmt_index(((struct bbbt *)nand_bbt_buf)->bmt_tbl);
		if (bmtd.bmt_blk_idx == 0) {
			pr_info("[BBT] FATAL ERR: bmt block index is wrong!\n");
			return NULL;
		}
		pr_info("[BBT] BMT.v2 is found at 0x%x\n", block);
		return (struct bbbt *)nand_bbt_buf;
	} else
		return scan_bmt(block - 1);
}

/* Write the Burner Bad Block Table to Nand Flash
 * n - write BMT to bmt_tbl[n]
 */
static unsigned short upload_bmt(struct bbbt *bbt, int n)
{
	unsigned short block;

	if (n < 0 || bbt->bmt_tbl[n].mapped == NORMAL_MAPPED) {
		pr_info("nand: FATAL ERR: no space to store BMT!\n");
		return (unsigned short)-1;
	}

	block = bbt->bmt_tbl[n].block;
	BBT_LOG("n = 0x%x, block = 0x%x", n, block);
	if (bbt_nand_erase(block)) {
		bbt->bmt_tbl[n].block = 0;
		/* erase failed, try the previous block: bmt_tbl[n - 1].block */
		n = upload_bmt(bbt, n - 1);
	} else {
		/* The signature offset is fixed set to 0,
		 * oob signature offset is fixed set to 1
		 */
		memset(nand_spare_buf, 0xff, bmtd.spare_buf_size);
		memcpy(nand_spare_buf + OOB_SIGNATURE_OFFSET, "bmt", 3);
		memcpy(bbt->signature + MAIN_SIGNATURE_OFFSET, "BMT", 3);
		bbt->version = BBMT_VERSION;

		if (write_bmt(block, (unsigned char *)bbt, nand_spare_buf)) {
			bbt->bmt_tbl[n].block = 0;
			/* write failed, try the previous block in bmt_tbl[n - 1] */
			n = upload_bmt(bbt, n - 1);
		}
	}

	/* Return the current index(n) of BMT pool (bmt_tbl[n]) */
	return n;
}

static inline bool is_bmt_ready(void)
{
	return bmt_ready;
}

static unsigned short find_valid_block_in_pool(struct bbbt *bbt)
{
	int i;

	if (bmtd.bmt_blk_idx == 0)
		goto error;

	for (i = 0; i < bmtd.bmt_blk_idx; i++) {
		if (bbt->bmt_tbl[i].block != 0 && bbt->bmt_tbl[i].mapped == NO_MAPPED) {
			bbt->bmt_tbl[i].mapped = NORMAL_MAPPED;
			return bbt->bmt_tbl[i].block;
		}
	}

error:
	pr_info("nand: FATAL ERR: BMT pool is run out!\n");
	return 0;
}

static int write_mapped_block(unsigned long long offset, unsigned short block,
		unsigned char *dat, unsigned char *oob)
{
	/* page offset in block */
	unsigned int page = byte_pg(offset & (bmtd.blk_size - 1));

	/* mapping block */
	page += blk_pg(block);

	if (bbt_nand_erase(block)) {
		pr_info("nand: warn: Erase block 0x%x failed\n", block);
		return -1;
	}

	if (bbt_nand_write(page, dat, oob)) {
		pr_info("nand: warn: Write page 0x%x failed\n", page);
		return -1;
	}

	return 0;
}

/* We met a bad block, mark it as bad and map it to a valid block in pool,
 * if it's a write failure, we need to write the data to mapped block
 */
bool update_bmt(unsigned long long offset, update_reason_t reason,
		unsigned char *dat, unsigned char *oob)
{
	unsigned short block, mapped_blk;
	struct bbbt *bbt;

	if (!is_bmt_ready())
		return false;

	bbt = bmtd.bbt;
	mapped_blk = find_valid_block_in_pool(bbt);
	if (mapped_blk == 0)
		return false;

	block = byte_blk(offset);

	/* Map new bad block to available block in pool */
	bbt->bb_tbl[block] = mapped_blk;

	if (reason == UPDATE_WRITE_FAIL) {
		/* Data should be written into mapped block */
		if (write_mapped_block(offset, mapped_blk, dat, oob))
			return update_bmt(offset, reason, dat, oob);
	} else {
		if (bbt_nand_erase(mapped_blk)) {
			pr_info("nand: warn: Erase block 0x%x failed\n", block);
			return update_bmt(offset, reason, dat, oob);
		}
	}

	bmtd.bmt_blk_idx = upload_bmt(bbt, bmtd.bmt_blk_idx);

	return true;
}

unsigned short get_mapping_block_index(int block)
{
	int mapping_block;

	if (is_bmt_ready()) {
		if (block < bmtd.pool_lba)
			mapping_block = bmtd.bbt->bb_tbl[block];
		else
			mapping_block = block;
		BBT_LOG("0x%x mapped to 0x%x", block, mapping_block);
	} else {
		pr_info("nand: Warning: the BMT table has not been initialized\n");
		mapping_block = (unsigned short)block;
	}
	return mapping_block;
}

/* total_blocks - The total count of blocks that the Nand Chip has */
int init_bmt(struct mtk_snand_host *host, unsigned short total_blocks,
		unsigned short pmt_block)
{
	int ret = 0;
	struct bbbt *bbt;
	unsigned int bufsz;
	unsigned int block;
	struct nand_chip *chip = &host->nand_chip;

	mtd_bmt = &chip->mtd;
	bmt_ready = false;

	/*
	 *  ---------------------------------------
	 * | PMT(2blks) | BMT POOL(totalblks * 2%) |
	 *  ---------------------------------------
	 * ^            ^
	 * |            |
	 * pmt_block	pmt_block + 2blocks(pool_lba)
	 *
	 * ATTETION!!!!!!
	 *     The blocks ahead of the boundary block are stored in bb_tbl
	 *     and blocks behind are stored in bmt_tbl
	 */
	bmtd.pool_lba = (unsigned short)(pmt_block + 2);
	bmtd.blk_size = 1 << chip->phys_erase_shift;
	bmtd.blk_shift = chip->phys_erase_shift;
	bmtd.pg_size = 1 << chip->page_shift;
	bmtd.pg_shift = chip->page_shift;
	bmtd.total_blks = total_blocks;
	bmtd.bb_max = bmtd.total_blks * BBPOOL_RATIO / 100;
	bmtd.host = host;
	bmtd.spare_buf_size = fdm_sz_per_pg() * blk_pg(1);

	/* 3 buffers we need */
	bufsz = round_up(sizeof(struct bbbt),
			max_t(typeof(PAGE_SIZE), bmtd.pg_size, PAGE_SIZE));
	bmtd.bmt_pgs = byte_pg(bufsz);

	/* gfp - addr & len both are PAGE_SIZE aligned, which could do DMA */
	nand_bbt_buf = (unsigned char *)__get_free_pages(GFP_KERNEL,
			get_order((unsigned long)bufsz));
	nand_data_buf = (unsigned char *)__get_free_pages(GFP_KERNEL,
			get_order((unsigned long)bmtd.pg_size));
	/* PIO mode, do not care alignment, use kmalloc */
	nand_spare_buf = kmalloc(bmtd.spare_buf_size, GFP_KERNEL);

	if (!nand_bbt_buf || !nand_data_buf || !nand_spare_buf) {
		pr_info("nand: FATAL ERR: allocate buffer failed!\n");
		ret = -1;
		goto error;
	}

	memset(nand_bbt_buf, 0xff, bufsz);
	memset(nand_data_buf, 0xff, bmtd.pg_size);
	memset(nand_spare_buf, 0xff, bmtd.spare_buf_size);

	BBT_LOG("bbtbuf=0x%p(0x%x) dat=0x%p(0x%x) spare=0x%p(0x%x)",
			nand_bbt_buf, bufsz, nand_data_buf, bmtd.pg_size,
			nand_spare_buf, bmtd.spare_buf_size);
	BBT_LOG("pool_lba=0x%x total_blks=0x%x bb_max=0x%x",
			bmtd.pool_lba, bmtd.total_blks, bmtd.bb_max);

	/* Scanning start from the first page of the last block
	 * of whole flash
	 */
	bbt = scan_bmt(bmtd.total_blks - 1);
	if (!bbt) {
		/* BMT not found */
		if (bmtd.total_blks > BB_TABLE_MAX + BMT_TABLE_MAX) {
			pr_info("nand: FATAL: Too many blocks, can not support!\n");
			ret = -1;
			goto error;
		}

		bbt = (struct bbbt *)nand_bbt_buf;
		memset(bbt->bmt_tbl, BMT_TBL_DEF_VAL, sizeof(bbt->bmt_tbl));

		if (scan_bad_blocks(bbt)) {
			ret = -1;
			goto error;
		}

		/* BMT always in the last valid block in pool */
		bmtd.bmt_blk_idx = upload_bmt(bbt, bmtd.bmt_blk_idx);
		block = bbt->bmt_tbl[bmtd.bmt_blk_idx].block;
		pr_notice("[BBT] BMT.v2 is written into PBA:0x%x\n", block);

		if (bmtd.bmt_blk_idx == 0)
			pr_info("nand: Warning: no available block in BMT pool!\n");
		else if (bmtd.bmt_blk_idx == (unsigned short)-1) {
			ret = -1;
			goto error;
		}
	}

	bmtd.bbt = bbt;
	bmt_ready = true;

error:
	/* DO NOT free the nand_bbt_buf!!!! */
	if (!bmt_ready && nand_bbt_buf)
		free_pages((unsigned long)nand_bbt_buf, get_order((unsigned long)bufsz));
	if (!bmt_ready && nand_data_buf)
		free_pages((unsigned long)nand_data_buf,
				get_order((unsigned long)bmtd.pg_size));
	if (!bmt_ready && nand_spare_buf)
		kfree(nand_spare_buf);

	return ret;
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Xiangsheng Hou <xiangsheng.hou@mediatek.com>");
MODULE_DESCRIPTION("Bad Block mapping management v2 for MediaTek NAND Flash Driver");

