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
#ifndef RA_DBG_PROC_H
#define RA_DBG_PROC_H

#include <linux/ctype.h>
#include <linux/proc_fs.h>
#include "raeth_config.h"

extern struct net_device *dev_raether;

void dump_qos(void);
void dump_reg(struct seq_file *s);
void dump_cp0(void);

int debug_proc_init(void);
void debug_proc_exit(void);

int tso_len_update(int tso_len);
int num_of_txd_update(int num_of_txd);
void rtk_hal_dump_mib(void);
#ifdef CONFIG_RAETH_LRO
int lro_stats_update(struct net_lro_mgr *lro_mgr, bool all_flushed);
#endif
extern unsigned int M2Q_table[64];
extern struct QDMA_txdesc *free_head;
extern struct SFQ_table *sfq0;
extern struct SFQ_table *sfq1;
extern struct SFQ_table *sfq2;
extern struct SFQ_table *sfq3;
extern int init_schedule;
extern int working_schedule;
struct raeth_int_t {
	unsigned int RX_COHERENT_CNT;
	unsigned int RX_DLY_INT_CNT;
	unsigned int TX_COHERENT_CNT;
	unsigned int TX_DLY_INT_CNT;
	unsigned int RING3_RX_DLY_INT_CNT;
	unsigned int RING2_RX_DLY_INT_CNT;
	unsigned int RING1_RX_DLY_INT_CNT;
	unsigned int RXD_ERROR_CNT;
	unsigned int ALT_RPLC_INT3_CNT;
	unsigned int ALT_RPLC_INT2_CNT;
	unsigned int ALT_RPLC_INT1_CNT;
	unsigned int RX_DONE_INT3_CNT;
	unsigned int RX_DONE_INT2_CNT;
	unsigned int RX_DONE_INT1_CNT;
	unsigned int RX_DONE_INT0_CNT;
	unsigned int TX_DONE_INT3_CNT;
	unsigned int TX_DONE_INT2_CNT;
	unsigned int TX_DONE_INT1_CNT;
	unsigned int TX_DONE_INT0_CNT;
};

int int_stats_update(unsigned int int_status);

#define DUMP_EACH_PORT(base)					\
	for (i = 0; i < 7; i++) {					\
		mii_mgr_read(31, (base) + (i * 0x100), &pkt_cnt); \
		seq_printf(seq, "%8u ", pkt_cnt);			\
	}							\

/* HW LRO functions */
int hwlro_debug_proc_init(struct proc_dir_entry *proc_reg_dir);
void hwlro_debug_proc_exit(struct proc_dir_entry *proc_reg_dir);

int rss_debug_proc_init(struct proc_dir_entry *proc_reg_dir);
void rss_debug_proc_exit(struct proc_dir_entry *proc_reg_dir);

/* HW IO-Coherent functions */
#ifdef	CONFIG_RAETH_HW_IOCOHERENT
void hwioc_debug_proc_init(struct proc_dir_entry *proc_reg_dir);
void hwioc_debug_proc_exit(struct proc_dir_entry *proc_reg_dir);
#else
static inline void hwioc_debug_proc_init(struct proc_dir_entry *proc_reg_dir)
{
}

static inline void hwioc_debug_proc_exit(struct proc_dir_entry *proc_reg_dir)
{
}
#endif /* CONFIG_RAETH_HW_IOCOHERENT */

#endif
