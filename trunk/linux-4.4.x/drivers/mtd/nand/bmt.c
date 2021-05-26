/*
 * Copyright (c) 2017 MediaTek Inc.
 * Author: Bayi Cheng <bayi.cheng@mediatek.com>
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

#include <linux/module.h>
#include "bmt.h"
#include "mtk_snand.h"

typedef struct {
	char signature[3];
	u8 version;
	u8 bad_count;	/* bad block count in pool */
	u8 mapped_count;
	u8 checksum;
	u8 reseverd[13];
} phys_bmt_header;

typedef struct {
	phys_bmt_header header;
	bmt_entry table[MAX_BMT_SIZE];
} phys_bmt_struct;

typedef struct {
	char signature[3];
} bmt_oob_data;

static const char MAIN_SIGNATURE[]	= "BMT";
static const char OOB_SIGNATURE[]	= "bmt";
#define SIGNATURE_SIZE		(3)

#define MAX_DAT_SIZE		0x1000
#define MAX_OOB_SIZE		0x80

static struct mtd_info	*mtd_bmt;
static struct nand_chip	*nand_chip_bmt;
#define BLOCK_SIZE_BMT		(1 << nand_chip_bmt->phys_erase_shift)
#define PAGE_SIZE_BMT		(1 << nand_chip_bmt->page_shift)

#define OFFSET(block)		((block) * BLOCK_SIZE_BMT)
#define PAGE_ADDR(block)	((block) * BLOCK_SIZE_BMT / PAGE_SIZE_BMT)

/*********************************************************************
* Flash is splited into 2 parts, system part is for normal system
* system usage, size is system_block_count, another is replace pool
*    +-------------------------------------------------+
*    |     system_block_count     |   bmt_block_count  |
*    +-------------------------------------------------+
*********************************************************************/
static u32 total_block_count;
static u32 system_block_count;
static int bmt_block_count;

static int page_per_block;

static u32 bmt_block_index;
static bmt_struct bmt;

static u8 dat_buf[MAX_DAT_SIZE];
static u8 oob_buf[MAX_OOB_SIZE];
static bool pool_erased;

/***************************************************************
  * Interface adaptor for preloader/uboot/kernel
  * These interfaces operate on physical address, read/write
  *  physical data.
  ***************************************************************/
int nand_read_page_bmt(u32 page, u8 *dat, u8 *oob)
{
	return mtk_nand_exec_read_page(mtd_bmt, page, PAGE_SIZE_BMT, dat, oob);
}

bool nand_block_bad_bmt(u32 offset)
{
	return mtk_nand_block_bad_hw(mtd_bmt, offset);
}

bool nand_erase_bmt(u32 offset)
{
	int status;

	if (offset < 0x20000)
		pr_warn("erase offset: 0x%x\n", offset);
	/* as nand_chip structure doesn't have a erase function defined */
	status = mtk_nand_erase_hw(mtd_bmt, offset / PAGE_SIZE_BMT);
	if (status & NAND_STATUS_FAIL)
		return false;
	else
		return true;
}

int mark_block_bad_bmt(u32 offset)
{
	return mtk_nand_block_markbad_hw(mtd_bmt, offset);
}

bool nand_write_page_bmt(u32 page, u8 *dat, u8 *oob)
{
	if (mtk_nand_exec_write_page(mtd_bmt, page, PAGE_SIZE_BMT, dat, oob, 0))
		return false;
	else
		return true;
}

static void dump_bmt_info(bmt_struct *bmt)
{
	int i;

	pr_warn("BMT v%d. total %d mapping:\n",
		bmt->version, bmt->mapped_count);
	for (i = 0; i < bmt->mapped_count; i++)
		pr_warn("\t0x%x -> 0x%x\n", bmt->table[i].bad_index,
			bmt->table[i].mapped_index);
}

static bool match_bmt_signature(u8 *dat, u8 *oob)
{

	if (memcmp(dat + MAIN_SIGNATURE_OFFSET, MAIN_SIGNATURE, SIGNATURE_SIZE))
		return false;

	if (memcmp(oob + OOB_SIGNATURE_OFFSET, OOB_SIGNATURE, SIGNATURE_SIZE))
		pr_warn("main signature match, oob signature doesn't match, but ignore\n");

	return true;
}

static u8 cal_bmt_checksum(phys_bmt_struct *phys_table, int bmt_size)
{
	int i;
	u8 checksum = 0;
	u8 *dat = (u8 *) phys_table;

	checksum += phys_table->header.version;
	checksum += phys_table->header.mapped_count;

	dat += sizeof(phys_bmt_header);
	for (i = 0; i < bmt_size * sizeof(bmt_entry); i++)
		checksum += dat[i];

	return checksum;
}


static int is_block_mapped(int index)
{
	int i;

	for (i = 0; i < bmt.mapped_count; i++) {
		if (index == bmt.table[i].mapped_index)
			return i;
	}
	return -1;
}

static bool is_page_used(u8 *dat, u8 *oob)
{
	return ((oob[OOB_INDEX_OFFSET] != 0xFF) || (oob[OOB_INDEX_OFFSET + 1]
		!= 0xFF));
}

static bool valid_bmt_data(phys_bmt_struct *phys_table)
{
	int i;
	u8 checksum = cal_bmt_checksum(phys_table, bmt_block_count);

	/* checksum correct */
	if (phys_table->header.checksum != checksum) {
		pr_warn("BMT Data checksum error: %x %x\n",
		phys_table->header.checksum, checksum);
		return false;
	}

	pr_warn("BMT Checksum is: 0x%x\n", phys_table->header.checksum);
	for (i = 0; i < phys_table->header.mapped_count; i++) {
		if (phys_table->table[i].bad_index >= total_block_count ||
		    phys_table->table[i].mapped_index >= total_block_count ||
		    phys_table->table[i].mapped_index < system_block_count) {
			pr_warn("index error: bad_index: %d, mapped_index: %d\n",
				phys_table->table[i].bad_index,
				phys_table->table[i].mapped_index);
			return false;
		}
	}

	pr_warn("Valid BMT, version v%d\n", phys_table->header.version);
	return true;
}

static void fill_nand_bmt_buffer(bmt_struct *bmt, u8 *dat, u8 *oob)
{
	phys_bmt_struct phys_bmt;

	dump_bmt_info(bmt);

	/* fill phys_bmt_struct structure with bmt_struct */
	memset(&phys_bmt, 0xFF, sizeof(phys_bmt));

	memcpy(phys_bmt.header.signature, MAIN_SIGNATURE, SIGNATURE_SIZE);
	phys_bmt.header.version = BMT_VERSION;
	phys_bmt.header.mapped_count = bmt->mapped_count;
	memcpy(phys_bmt.table, bmt->table, sizeof(bmt_entry) * bmt_block_count);

	phys_bmt.header.checksum = cal_bmt_checksum(&phys_bmt, bmt_block_count);

	memcpy(dat + MAIN_SIGNATURE_OFFSET, &phys_bmt, sizeof(phys_bmt));
	memcpy(oob + OOB_SIGNATURE_OFFSET, OOB_SIGNATURE, SIGNATURE_SIZE);
}

/* return valid index if found BMT, else return 0 */
static int load_bmt_data(int start, int pool_size)
{
	int bmt_index = start + pool_size - 1;
	phys_bmt_struct phys_table;
	int i;

	pr_warn("[%s]: begin to search BMT from block 0x%x\n",
		__func__, bmt_index);
	for (bmt_index = start + pool_size - 1; bmt_index >= start;
	     bmt_index--) {
		if (nand_block_bad_bmt(OFFSET(bmt_index))) {
			pr_warn("Skip bad block: %d\n", bmt_index);
			continue;
		}

		if (!nand_read_page_bmt(PAGE_ADDR(bmt_index),
					dat_buf, oob_buf)) {
			pr_warn("Error found when read block %d\n",
				bmt_index);
			continue;
		}

		if (!match_bmt_signature(dat_buf, oob_buf))
			continue;

		pr_warn("Match bmt signature @ block: 0x%x\n", bmt_index);
		memcpy(&phys_table, dat_buf + MAIN_SIGNATURE_OFFSET,
		       sizeof(phys_table));

		if (!valid_bmt_data(&phys_table)) {
			pr_warn("BMT data is not correct %d\n", bmt_index);
			continue;
		} else {
			bmt.mapped_count = phys_table.header.mapped_count;
			bmt.version = phys_table.header.version;
			/* bmt.bad_count = phys_table.header.bad_count; */
			memcpy(bmt.table, phys_table.table, bmt.mapped_count
			       * sizeof(bmt_entry));

			pr_warn("bmt found at block: %d, mapped block: %d\n",
				bmt_index, bmt.mapped_count);

			for (i = 0; i < bmt.mapped_count; i++) {
				if (!nand_block_bad_bmt(OFFSET
					(bmt.table[i].bad_index))) {
					pr_warn("block 0x%x is not mark bad\n",
						bmt.table[i].bad_index);
					mark_block_bad_bmt(OFFSET
						(bmt.table[i].bad_index));
				}
			}
			return bmt_index;
		}
	}

	pr_warn("bmt block not found!\n");
	return 0;
}

/*************************************************************************
 * Find an available block and erase.
 * start_from_end: if true, find available block from end of flash.
 *                 else, find from the beginning of the pool
 * need_erase: if true, all unmapped blocks in the pool will be erased
 *************************************************************************/
static int find_available_block(bool start_from_end)
{
	int i;
	int block = system_block_count;
	int direction;

	pr_warn("find_available_block, pool_erase: %d\n", pool_erased);

	if (!pool_erased) {
		pr_warn("Erase all un-mapped blocks in pool\n");
		for (i = 0; i < bmt_block_count; i++) {
			if (block == bmt_block_index) {
				pr_warn("Skip bmt block 0x%x\n", block);
				continue;
			}

			if (nand_block_bad_bmt(OFFSET(block + i))) {
				pr_warn("Skip bad block 0x%x\n", block + i);
				continue;
			}

			if (is_block_mapped(block + i) >= 0) {
				pr_warn("Skip mapped block 0x%x\n", block + i);
				continue;
			}

			if (!nand_erase_bmt(OFFSET(block + i))) {
				pr_warn("Erase block 0x%x failed\n", block + i);
				mark_block_bad_bmt(OFFSET(block + i));
			}
		}

		pool_erased = 1;
	}

	if (start_from_end) {
		block = total_block_count - 1;
		direction = -1;
	} else {
		block = system_block_count;
		direction = 1;
	}

	for (i = 0; i < bmt_block_count; i++, block += direction) {
		if (block == bmt_block_index) {
			pr_warn("Skip bmt block 0x%x\n", block);
			continue;
		}

		if (nand_block_bad_bmt(OFFSET(block))) {
			pr_warn("Skip bad block 0x%x\n", block);
			continue;
		}

		if (is_block_mapped(block) >= 0) {
			pr_warn("Skip mapped block 0x%x\n", block);
			continue;
		}

		pr_warn("Find block 0x%x available\n", block);
		return block;
	}

	return 0;
}

static unsigned short get_bad_index_from_oob(u8 *oob_buf)
{
	unsigned short index;

	memcpy(&index, oob_buf + OOB_INDEX_OFFSET, OOB_INDEX_SIZE);

	return index;
}

void set_bad_index_to_oob(u8 *oob, u16 index)
{
	memcpy(oob + OOB_INDEX_OFFSET, &index, sizeof(index));
}

static int migrate_from_bad(int offset, u8 *write_dat, u8 *write_oob)
{
	int page;
	int error_block = offset / BLOCK_SIZE_BMT;
	int error_page = (offset / PAGE_SIZE_BMT) % page_per_block;
	int to_index;

	memcpy(oob_buf, write_oob, MAX_OOB_SIZE);
	to_index = find_available_block(false);

	if (!to_index) {
		pr_warn("Cannot find an available block for BMT\n");
		return 0;
	}

	/* migrate error page first */
	pr_warn("Write error page: 0x%x\n", error_page);
	if (!write_dat) {
		nand_read_page_bmt(PAGE_ADDR(error_block) + error_page,
				   dat_buf, NULL);
		write_dat = dat_buf;
	}
	/*
	 * if error_block is already a mapped block,
	 * original mapping index is in OOB.
	 */
	if (error_block < system_block_count)
		set_bad_index_to_oob(oob_buf, error_block);

	if (!nand_write_page_bmt(PAGE_ADDR(to_index) + error_page,
				 write_dat, oob_buf)) {
		pr_warn("Write to page 0x%x fail\n", PAGE_ADDR(to_index)
			+ error_page);
		mark_block_bad_bmt(OFFSET(to_index));
		return migrate_from_bad(offset, write_dat, write_oob);
	}

	for (page = 0; page < page_per_block; page++) {
		if (page != error_page) {
			nand_read_page_bmt(PAGE_ADDR(error_block) + page,
					   dat_buf, oob_buf);
			if (is_page_used(dat_buf, oob_buf)) {
				if (error_block < system_block_count)
					set_bad_index_to_oob(oob_buf,
							     error_block);

					pr_warn("\tmigrate P 0x%x to P 0x%x\n",
						PAGE_ADDR(error_block) + page,
						PAGE_ADDR(to_index) + page);
				if (!nand_write_page_bmt(PAGE_ADDR(to_index)
							 + page, dat_buf,
							 oob_buf)) {
					pr_warn("Write to page 0x%x fail\n",
						PAGE_ADDR(to_index) + page);
					mark_block_bad_bmt(OFFSET(to_index));
					return migrate_from_bad(offset,
						write_dat, write_oob);
				}
			}
		}
	}
	pr_warn("Migrate from 0x%x to 0x%x done!\n", error_block, to_index);
	return to_index;
}

static bool write_bmt_to_flash(u8 *dat, u8 *oob)
{
	bool need_erase = true;

	pr_warn("Try to write BMT\n");
	if (bmt_block_index == 0) {
		/*
		 * if we don't have index, we don't need to erase found
		 * block as it has been erased in find_available_block()
		 */
		need_erase = false;
		bmt_block_index = find_available_block(true);
		if (!bmt_block_index) {
			pr_warn("Cannot find an available block for BMT\n");
			return false;
		}
	}

	pr_warn("Find BMT block: 0x%x\n", bmt_block_index);
	/* write bmt to flash */
	if (need_erase) {
		if (!nand_erase_bmt(OFFSET(bmt_block_index))) {
			pr_warn("BMT block erase fail, mark bad: 0x%x\n",
				bmt_block_index);
			mark_block_bad_bmt(OFFSET(bmt_block_index));
			/* bmt.bad_count++; */

			bmt_block_index = 0;
			/* recursive call */
			return write_bmt_to_flash(dat, oob);
		}
	}

	if (!nand_write_page_bmt(PAGE_ADDR(bmt_block_index), dat, oob)) {
		pr_warn("Write BMT data fail, need to write again\n");
		mark_block_bad_bmt(OFFSET(bmt_block_index));
		/* bmt.bad_count++; */

		bmt_block_index = 0;
		return write_bmt_to_flash(dat, oob);    /* recursive call */
	}

	pr_warn("Write BMT data to block 0x%x success\n", bmt_block_index);
	return true;
}

bmt_struct *reconstruct_bmt(bmt_struct *bmt)
{
	int i;
	int index = system_block_count;
	unsigned short bad_index;
	int mapped;

	/* init everything in BMT struct */
	bmt->version = BMT_VERSION;
	bmt->bad_count = 0;
	bmt->mapped_count = 0;
	memset(bmt->table, 0, bmt_block_count * sizeof(bmt_entry));

	for (i = 0; i < bmt_block_count; i++, index++) {
		if (nand_block_bad_bmt(OFFSET(index))) {
			pr_warn("Skip bad block: 0x%x\n", index);
			/* bmt->bad_count++; */
			continue;
		}

		pr_warn("read page: 0x%x\n", PAGE_ADDR(index));
		nand_read_page_bmt(PAGE_ADDR(index), dat_buf, oob_buf);

		bad_index = get_bad_index_from_oob(oob_buf);
		if (bad_index >= system_block_count) {
			pr_warn("get bad index: 0x%x\n", bad_index);
			if (bad_index != 0xFFFF)
				pr_warn("Invalid bad index found in 0x%x, bad index 0x%x\n",
					index, bad_index);
			continue;
		}

		pr_warn("Block 0x%x is mapped to bad block: 0x%x\n", index,
			bad_index);

		if (!nand_block_bad_bmt(OFFSET(bad_index))) {
			pr_warn("0x%x is not marked as bad, invalid mapping\n",
				bad_index);
			continue;
		}
		mapped = is_block_mapped(bad_index);
		if (mapped >= 0)
			bmt->table[mapped].mapped_index = index;
		else {
			/* add mapping to BMT */
			bmt->table[bmt->mapped_count].bad_index = bad_index;
			bmt->table[bmt->mapped_count].mapped_index = index;
			bmt->mapped_count++;
		}
		pr_warn("Add mapping: 0x%x -> 0x%x to BMT\n", bad_index, index);
	}

	pr_warn("Scan replace pool done, mapped block: %d\n",
		bmt->mapped_count);
	/* fill NAND BMT buffer */
	memset(oob_buf, 0xFF, sizeof(oob_buf));
	fill_nand_bmt_buffer(bmt, dat_buf, oob_buf);

	/* write BMT back */
	if (!write_bmt_to_flash(dat_buf, oob_buf))
		pr_warn("TRAGEDY: cannot find a place to write BMT!!!!\n");

	return bmt;
}

bmt_struct *init_bmt(struct nand_chip *chip, int size)
{
	struct mtk_snand_host *host;

	if (size > 0 && size < MAX_BMT_SIZE) {
		pr_warn("Init bmt table, size: %d\n", size);
		bmt_block_count = size;
	} else {
		pr_warn("Invalid bmt table size: %d\n", size);
		return NULL;
	}
	nand_chip_bmt = chip;
	system_block_count = chip->chipsize >> chip->phys_erase_shift;
	total_block_count = bmt_block_count + system_block_count;
	page_per_block = BLOCK_SIZE_BMT / PAGE_SIZE_BMT;
	host = (struct mtk_snand_host *)chip->priv;
	mtd_bmt = nand_to_mtd(chip);

	pr_warn("mtd_bmt: %p, nand_chip_bmt: %p\n",
		 mtd_bmt, nand_chip_bmt);
	pr_warn("bmt count: %d, system count: %d\n",
		 bmt_block_count, system_block_count);

	pool_erased = 0;
	memset(bmt.table, 0, size * sizeof(bmt_entry));
	bmt_block_index = load_bmt_data(system_block_count, size);
	if (bmt_block_index) {
		pr_warn("Load bmt data success @ block 0x%x\n",
			bmt_block_index);
		dump_bmt_info(&bmt);
	} else {
		pr_warn("Load bmt data fail, need re-construct!\n");
		if (reconstruct_bmt(&bmt))
			return &bmt;
		else
			return NULL;
	}

	return &bmt;
}
EXPORT_SYMBOL_GPL(init_bmt);

bool update_bmt(u32 offset, update_reason_t reason, u8 *dat, u8 *oob)
{
	int map_index;
	int orig_bad_block = -1;
	int i;
	int bad_index = offset / BLOCK_SIZE_BMT;

	if (reason == UPDATE_WRITE_FAIL) {
		pr_warn("Write fail, need to migrate\n");
		map_index = migrate_from_bad(offset, dat, oob);
		if (!map_index) {
			pr_warn("migrate fail\n");
			return false;
		}
	} else {
		map_index = find_available_block(false);
		if (!map_index) {
			pr_warn("Cannot find block in pool\n");
			return false;
		}
	}

	if (bad_index >= system_block_count) {
		for (i = 0; i < bmt_block_count; i++) {
			if (bmt.table[i].mapped_index == bad_index) {
				orig_bad_block = bmt.table[i].bad_index;
				break;
			}
		}
		/* bmt.bad_count++; */
		pr_warn("Mapped block becomes bad, orig block is 0x%x\n",
			orig_bad_block);

		bmt.table[i].mapped_index = map_index;
	} else {
		bmt.table[bmt.mapped_count].mapped_index = map_index;
		bmt.table[bmt.mapped_count].bad_index = bad_index;
		bmt.mapped_count++;
	}

	memset(oob_buf, 0xFF, sizeof(oob_buf));
	fill_nand_bmt_buffer(&bmt, dat_buf, oob_buf);
	if (!write_bmt_to_flash(dat_buf, oob_buf))
		return false;

	mark_block_bad_bmt(offset);

	return true;
}
EXPORT_SYMBOL_GPL(update_bmt);

u16 get_mapping_block_index(int index)
{
	int i;

	if (index > system_block_count)
		return index;

	for (i = 0; i < bmt.mapped_count; i++) {
		if (bmt.table[i].bad_index == index)
			return bmt.table[i].mapped_index;
	}

	return index;
}
EXPORT_SYMBOL_GPL(get_mapping_block_index);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("MediaTek");
MODULE_DESCRIPTION("Bad Block mapping management for MediaTek NAND Flash Driver");

