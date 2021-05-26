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
#ifndef __DRIVERS_MTD_NAND_MTK_PARTITION_H__
#define __DRIVERS_MTD_NAND_MTK_PARTITION_H__
#include <linux/mtd/mtd.h>

#define MTK_PARTITION

/* define either MTK_PARTITION_FIX or MTK_PARTITION_GPT */
#undef	MTK_PARTITION_FIX
#define	MTK_PARTITION_GPT
#define MTK_GPT_START_BLK		(8)
#define MTK_GPT_END_BLK			(11)
#define MTK_GPT_MAX_PART		(30)

int mtk_partition_register(struct mtd_info *mtd, int start_blk);

#endif
