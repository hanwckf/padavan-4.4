/* Copyright  2016 MediaTek Inc.
 * Author: Nelson Chang <nelson.chang@mediatek.com>
 * Author: Carlos Huang <carlos.huang@mediatek.com>
 * Author: Harry Huang <harry.huang@mediatek.com>
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
#ifndef HNAT_DBG_PROC_H
#define HNAT_DBG_PROC_H

#include <linux/ctype.h>
#include <linux/proc_fs.h>
#define HNAT_PROCREG_DIR             "hnat"
#define PROCREG_CPU_REASON              "cpu_reason"
#define PROCREG_PPE_ENTRY               "hnat_entry"
#define PROCREG_PPE_SETTING             "hnat_setting"
#define PROCREG_PPE_MULTICAST		"hnat_multicast"

extern unsigned int dbg_cpu_reason_cnt[32];
extern int hwnat_dbg_entry;
extern struct foe_entry *ppe_foe_base;

int hnat_debug_proc_init(void);
void hnat_debug_proc_exit(void);
void dbg_dump_entry(uint32_t index);
void dbg_dump_cr(void);
#endif
