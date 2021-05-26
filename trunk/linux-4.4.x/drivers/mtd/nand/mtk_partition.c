/*
 * Copyright (C) 2016  MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/efi.h>
#include "mtk_partition.h"

#ifdef MTK_PARTITION_GPT

#define GPT_HEADER_SIGNATURE 0x5452415020494645ULL
#define GPT_HEADER_REVISION_V1 0x00010000

typedef struct _gpt_header {
	__le64 signature;
	__le32 revision;
	__le32 header_size;
	__le32 header_crc32;
	__le32 reserved1;
	__le64 my_lba;
	__le64 alternate_lba;
	__le64 first_usable_lba;
	__le64 last_usable_lba;
	efi_guid_t disk_guid;
	__le64 partition_entry_lba;
	__le32 num_partition_entries;
	__le32 sizeof_partition_entry;
	__le32 partition_entry_array_crc32;

	/* The rest of the logical block is reserved by UEFI and must be zero.
	 * EFI standard handles this by:
	 *
	 * uint8_t		reserved2[ BlockSize - 92 ];
	 */
} __packed gpt_header;

typedef struct _gpt_entry_attributes {
	u64 required_to_function:1;
	u64 reserved:47;
	u64 type_guid_specific:16;
} __packed gpt_entry_attributes;

typedef struct _gpt_entry {
	efi_guid_t partition_type_guid;
	efi_guid_t unique_partition_guid;
	__le64 starting_lba;
	__le64 ending_lba;
	gpt_entry_attributes attributes;
	efi_char16_t partition_name[72 / sizeof(efi_char16_t)];
} __packed gpt_entry;

static struct mtd_partition g_gpt_Partition[MTK_GPT_MAX_PART];
#endif

#ifdef MTK_PARTITION_FIX
#define MTK_PARTITION_NUM	0
static struct mtd_partition g_exist_Partition[MTK_PARTITION_NUM];
#endif

int mtk_partition_register(struct mtd_info *mtd, int start_blk)
{
#ifdef MTK_PARTITION_GPT
	struct nand_chip *chip = mtd_to_nand(mtd);
	u8 *buf;
	int i, j, header_found, current_part = 0, ret = 0;
	loff_t addr;
	size_t retlen;
	struct mtd_partition *mtk_part = NULL;
	u32 part_size;

	buf = kmalloc(mtd->writesize + mtd->oobsize, GFP_KERNEL);
	if (!buf)
		return -1;
	part_size = MTK_GPT_MAX_PART * sizeof(struct mtd_partition);
	mtk_part = kmalloc(part_size, GFP_KERNEL);
	if (!mtk_part)
		return -1;
	memset(mtk_part, 0, part_size);

	memset(g_gpt_Partition, 0x0, sizeof(g_gpt_Partition));
	for (i = start_blk; i <= MTK_GPT_END_BLK; i++) {
		header_found = 0;
		current_part = 0;
		for (j = 0; j < (1 << (chip->phys_erase_shift - chip->page_shift)); j++) {
			addr = (i << chip->phys_erase_shift) + (j << chip->page_shift);
			if (mtd->_read(mtd, addr, mtd->writesize, &retlen,  buf) < 0) {
				ret = -1;
				break;
			}
			/* header found, looking for entry */
			if (header_found) {
				gpt_entry *gpt_e;
				u32 part_count, name_len;
				u8 *part_name, *p;
				efi_char16_t *efi_name;
				u64 start_page, end_page;

				part_count = 0;

				gpt_e = (gpt_entry *)buf;
				while (gpt_e->partition_type_guid.b[0] != 0) {
					start_page = le64_to_cpu(gpt_e->starting_lba);
					end_page = le64_to_cpu(gpt_e->ending_lba);
					mtk_part[current_part].offset = start_page << chip->page_shift;
					mtk_part[current_part].size = (end_page - start_page + 1)
									<< chip->page_shift;
					efi_name = gpt_e->partition_name;
					name_len = 0;
					while (*efi_name & 0xff) {
						efi_name++;
						name_len++;
					}

					part_name = kmalloc(name_len+1, GFP_KERNEL);
					if (!part_name) {
						ret = -1;
						break;
					}
					memset(part_name, 0, name_len + 1);

					p = part_name;
					efi_name = gpt_e->partition_name;
					while (*efi_name & 0xff) {
						*p = (u8) *efi_name & 0xff;
						efi_name++;
						p++;

					}
					mtk_part[current_part].name = part_name;

					gpt_e++;
					current_part++;
					part_count++;
					if ((part_count * sizeof(gpt_entry)) == mtd->writesize)
						break;
				}
				if (!part_count)
					break;
			} else {
				gpt_header *gpt_hdr;

				gpt_hdr = (gpt_header *)buf;
				if (le64_to_cpu(gpt_hdr->signature) == GPT_HEADER_SIGNATURE) {
					header_found = 1;
					current_part = 0;
				}
			}

		}
		if ((header_found && current_part) || (ret < 0))
			break;
	}
	kfree(buf);

	if ((i > MTK_GPT_END_BLK) || (ret < 0)) {

		if (mtk_part) {
			while (current_part >= 0) {
				kfree(mtk_part[current_part].name);
				current_part--;
			}
			kfree(mtk_part);
		}
		return ret;
	}

	return mtd_device_register(mtd, mtk_part, current_part);
#endif

#ifdef MTK_PARTITION_FIX
	/* fix partition */
	return mtd_device_register(mtd, g_exist_Partition, MTK_PARTITION_NUM);
#endif
	return 0;
}
EXPORT_SYMBOL(mtk_partition_register);

