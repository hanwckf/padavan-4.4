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
#include "raether.h"
#include "ra_dbg_proc.h"
#include "ra_ethtool.h"

int txd_cnt[MAX_SKB_FRAGS / 2 + 1];
int tso_cnt[16];

#define MAX_AGGR 64
#define MAX_DESC  8
int lro_stats_cnt[MAX_AGGR + 1];
int lro_flush_cnt[MAX_AGGR + 1];
int lro_len_cnt1[16];
/* int lro_len_cnt2[16]; */
int aggregated[MAX_DESC];
int lro_aggregated;
int lro_flushed;
int lro_nodesc;
int force_flush;
int tot_called1;
int tot_called2;

struct raeth_int_t raeth_int;
struct proc_dir_entry *proc_reg_dir;
static struct proc_dir_entry *proc_gmac, *proc_sys_cp0, *proc_tx_ring,
*proc_rx_ring, *proc_skb_free;
static struct proc_dir_entry *proc_gmac2;
static struct proc_dir_entry *proc_ra_snmp;
static struct proc_dir_entry *proc_num_of_txd, *proc_tso_len;
static struct proc_dir_entry *proc_lro_stats;
static struct proc_dir_entry *proc_sche;
static struct proc_dir_entry *proc_int_dbg;
static struct proc_dir_entry *proc_set_lan_ip;
/*extern unsigned int M2Q_table[64];
 * extern struct QDMA_txdesc *free_head;
 * extern struct SFQ_table *sfq0;
 * extern struct SFQ_table *sfq1;
 * extern struct SFQ_table *sfq2;
 * extern struct SFQ_table *sfq3;
 */

static int ra_snmp_seq_show(struct seq_file *seq, void *v)
{
	struct END_DEVICE *ei_local = netdev_priv(dev_raether);

	if (ei_local->features & USER_SNMPD) {
		seq_printf(seq, "rx counters: %x %x %x %x %x %x %x\n",
			   sys_reg_read(GDMA_RX_GBCNT0),
			   sys_reg_read(GDMA_RX_GPCNT0),
			   sys_reg_read(GDMA_RX_OERCNT0),
			   sys_reg_read(GDMA_RX_FERCNT0),
			   sys_reg_read(GDMA_RX_SERCNT0),
			   sys_reg_read(GDMA_RX_LERCNT0),
			   sys_reg_read(GDMA_RX_CERCNT0));
		seq_printf(seq, "fc config: %x %x %p %x\n",
			   sys_reg_read(CDMA_FC_CFG),
			   sys_reg_read(GDMA1_FC_CFG),
			   PDMA_FC_CFG, sys_reg_read(PDMA_FC_CFG));
		seq_printf(seq, "ports: %x %x %x %x %x %x\n",
			   sys_reg_read(PORT0_PKCOUNT),
			   sys_reg_read(PORT1_PKCOUNT),
			   sys_reg_read(PORT2_PKCOUNT),
			   sys_reg_read(PORT3_PKCOUNT),
			   sys_reg_read(PORT4_PKCOUNT),
			   sys_reg_read(PORT5_PKCOUNT));
	}

	return 0;
}

static int ra_snmp_seq_open(struct inode *inode, struct file *file)
{
	struct END_DEVICE *ei_local = netdev_priv(dev_raether);

	if (ei_local->features & USER_SNMPD)
		return single_open(file, ra_snmp_seq_show, NULL);
	else
		return 0;
}

static const struct file_operations ra_snmp_seq_fops = {
	.owner = THIS_MODULE,
	.open = ra_snmp_seq_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release
};

/*Routine Name : get_idx(mode, index)
 * Description: calculate ring usage for tx/rx rings
 * Mode 1 : Tx Ring
 * Mode 2 : Rx Ring
 */
int get_ring_usage(int mode, int i)
{
	unsigned long tx_ctx_idx, tx_dtx_idx, tx_usage;
	unsigned long rx_calc_idx, rx_drx_idx, rx_usage;

	struct PDMA_rxdesc *rxring;
	struct PDMA_txdesc *txring;

	struct END_DEVICE *ei_local = netdev_priv(dev_raether);

	if (mode == 2) {
		/* cpu point to the next descriptor of rx dma ring */
		rx_calc_idx = *(unsigned long *)RX_CALC_IDX0;
		rx_drx_idx = *(unsigned long *)RX_DRX_IDX0;
		rxring = (struct PDMA_rxdesc *)RX_BASE_PTR0;

		rx_usage =
		    (rx_drx_idx - rx_calc_idx - 1 + num_rx_desc) % num_rx_desc;
		if (rx_calc_idx == rx_drx_idx) {
			if (rxring[rx_drx_idx].rxd_info2.DDONE_bit == 1)
				tx_usage = num_rx_desc;
			else
				tx_usage = 0;
		}
		return rx_usage;
	}

	switch (i) {
	case 0:
		tx_ctx_idx = *(unsigned long *)TX_CTX_IDX0;
		tx_dtx_idx = *(unsigned long *)TX_DTX_IDX0;
		txring = ei_local->tx_ring0;
		break;
	default:
		pr_debug("get_tx_idx failed %d %d\n", mode, i);
		return 0;
	};

	tx_usage = (tx_ctx_idx - tx_dtx_idx + num_tx_desc) % num_tx_desc;
	if (tx_ctx_idx == tx_dtx_idx) {
		if (txring[tx_ctx_idx].txd_info2.DDONE_bit == 1)
			tx_usage = 0;
		else
			tx_usage = num_tx_desc;
	}
	return tx_usage;
}

void dump_reg(struct seq_file *s)
{
	int fe_int_enable;
	int rx_usage;
	int dly_int_cfg;
	int rx_base_ptr0;
	int rx_max_cnt0;
	int rx_calc_idx0;
	int rx_drx_idx0;
	struct END_DEVICE *ei_local = netdev_priv(dev_raether);
	int tx_usage = 0;
	int tx_base_ptr[4];
	int tx_max_cnt[4];
	int tx_ctx_idx[4];
	int tx_dtx_idx[4];
	int i;

	fe_int_enable = sys_reg_read(FE_INT_ENABLE);
	rx_usage = get_ring_usage(2, 0);

	dly_int_cfg = sys_reg_read(DLY_INT_CFG);

	if (!(ei_local->features & FE_QDMA)) {
		tx_usage = get_ring_usage(1, 0);

		tx_base_ptr[0] = sys_reg_read(TX_BASE_PTR0);
		tx_max_cnt[0] = sys_reg_read(TX_MAX_CNT0);
		tx_ctx_idx[0] = sys_reg_read(TX_CTX_IDX0);
		tx_dtx_idx[0] = sys_reg_read(TX_DTX_IDX0);

		tx_base_ptr[1] = sys_reg_read(TX_BASE_PTR1);
		tx_max_cnt[1] = sys_reg_read(TX_MAX_CNT1);
		tx_ctx_idx[1] = sys_reg_read(TX_CTX_IDX1);
		tx_dtx_idx[1] = sys_reg_read(TX_DTX_IDX1);

		tx_base_ptr[2] = sys_reg_read(TX_BASE_PTR2);
		tx_max_cnt[2] = sys_reg_read(TX_MAX_CNT2);
		tx_ctx_idx[2] = sys_reg_read(TX_CTX_IDX2);
		tx_dtx_idx[2] = sys_reg_read(TX_DTX_IDX2);

		tx_base_ptr[3] = sys_reg_read(TX_BASE_PTR3);
		tx_max_cnt[3] = sys_reg_read(TX_MAX_CNT3);
		tx_ctx_idx[3] = sys_reg_read(TX_CTX_IDX3);
		tx_dtx_idx[3] = sys_reg_read(TX_DTX_IDX3);
	}

	rx_base_ptr0 = sys_reg_read(RX_BASE_PTR0);
	rx_max_cnt0 = sys_reg_read(RX_MAX_CNT0);
	rx_calc_idx0 = sys_reg_read(RX_CALC_IDX0);
	rx_drx_idx0 = sys_reg_read(RX_DRX_IDX0);

	seq_printf(s, "\n\nFE_INT_ENABLE  : 0x%08x\n", fe_int_enable);

	if (!(ei_local->features & FE_QDMA))
		seq_printf(s, "TxRing PktCnt: %d/%d\n", tx_usage, num_tx_desc);

	seq_printf(s, "RxRing PktCnt: %d/%d\n\n", rx_usage, num_rx_desc);
	seq_printf(s, "DLY_INT_CFG    : 0x%08x\n", dly_int_cfg);

	if (!(ei_local->features & FE_QDMA)) {
		for (i = 0; i < 4; i++) {
			seq_printf(s, "TX_BASE_PTR%d   : 0x%08x\n", i,
				   tx_base_ptr[i]);
			seq_printf(s, "TX_MAX_CNT%d    : 0x%08x\n", i,
				   tx_max_cnt[i]);
			seq_printf(s, "TX_CTX_IDX%d	: 0x%08x\n", i,
				   tx_ctx_idx[i]);
			seq_printf(s, "TX_DTX_IDX%d	: 0x%08x\n", i,
				   tx_dtx_idx[i]);
		}
	}

	seq_printf(s, "RX_BASE_PTR0   : 0x%08x\n", rx_base_ptr0);
	seq_printf(s, "RX_MAX_CNT0    : 0x%08x\n", rx_max_cnt0);
	seq_printf(s, "RX_CALC_IDX0   : 0x%08x\n", rx_calc_idx0);
	seq_printf(s, "RX_DRX_IDX0    : 0x%08x\n", rx_drx_idx0);

	if (ei_local->features & FE_ETHTOOL)
		seq_printf(s,
			   "The current PHY address selected by ethtool is %d\n",
			   get_current_phy_address());
}

int reg_read_main(struct seq_file *seq, void *v)
{
	dump_reg(seq);
	return 0;
}

static void *seq_skb_free_start(struct seq_file *seq, loff_t *pos)
{
	if (*pos < num_tx_desc)
		return pos;
	return NULL;
}

static void *seq_skb_free_next(struct seq_file *seq, void *v, loff_t *pos)
{
	(*pos)++;
	if (*pos >= num_tx_desc)
		return NULL;
	return pos;
}

static void seq_skb_free_stop(struct seq_file *seq, void *v)
{
	/* Nothing to do */
}

static int seq_skb_free_show(struct seq_file *seq, void *v)
{
	int i = *(loff_t *)v;
	struct END_DEVICE *ei_local = netdev_priv(dev_raether);

	seq_printf(seq, "%d: %08x\n", i, *(int *)&ei_local->skb_free[i]);

	return 0;
}

static const struct seq_operations seq_skb_free_ops = {
	.start = seq_skb_free_start,
	.next = seq_skb_free_next,
	.stop = seq_skb_free_stop,
	.show = seq_skb_free_show
};

static int skb_free_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &seq_skb_free_ops);
}

static const struct file_operations skb_free_fops = {
	.owner = THIS_MODULE,
	.open = skb_free_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release
};

int qdma_read_64queue(struct seq_file *seq, void *v)
{
	struct END_DEVICE *ei_local = netdev_priv(dev_raether);

	if (ei_local->features & FE_QDMA) {
		unsigned int temp, i;
		unsigned int sw_fq, hw_fq;
		unsigned int min_en, min_rate, max_en, max_rate, sch, weight;
		unsigned int queue, tx_des_cnt, hw_resv, sw_resv, queue_head,
		    queue_tail, queue_no;
		struct net_device *dev = dev_raether;
		struct END_DEVICE *ei_local = netdev_priv(dev);

		seq_puts(seq, "==== General Information ====\n");
		temp = sys_reg_read(QDMA_FQ_CNT);
		sw_fq = (temp & 0xFFFF0000) >> 16;
		hw_fq = (temp & 0x0000FFFF);
		seq_printf(seq, "SW TXD: %d/%d; HW TXD: %d/%d\n", sw_fq,
			   num_tx_desc, hw_fq, NUM_QDMA_PAGE);
		seq_printf(seq, "SW TXD virtual start address: 0x%p\n",
			   ei_local->txd_pool);
		seq_printf(seq, "HW TXD virtual start address: 0x%p\n\n",
			   free_head);

		seq_puts(seq, "==== Scheduler Information ====\n");
		temp = sys_reg_read(QDMA_TX_SCH);
		max_en = (temp & 0x00000800) >> 11;
		max_rate = (temp & 0x000007F0) >> 4;
		for (i = 0; i < (temp & 0x0000000F); i++)
			max_rate *= 10;
		seq_printf(seq, "SCH1 rate control:%d. Rate is %dKbps.\n",
			   max_en, max_rate);
		max_en = (temp & 0x08000000) >> 27;
		max_rate = (temp & 0x07F00000) >> 20;
		for (i = 0; i < (temp & 0x000F0000); i++)
			max_rate *= 10;
		seq_printf(seq, "SCH2 rate control:%d. Rate is %dKbps.\n\n",
			   max_en, max_rate);

		seq_puts(seq, "==== Physical Queue Information ====\n");
		sys_reg_write(QDMA_PAGE, 0);
		for (queue = 0; queue < 64; queue++) {
			if (queue < 16) {
				sys_reg_write(QDMA_PAGE, 0);
				queue_no = queue;
			} else if (queue > 15 && queue <= 31) {
				sys_reg_write(QDMA_PAGE, 1);
				queue_no = queue % 16;
			} else if (queue > 31 && queue <= 47) {
				sys_reg_write(QDMA_PAGE, 2);
				queue_no = queue % 32;
			} else if (queue > 47 && queue <= 63) {
				sys_reg_write(QDMA_PAGE, 3);
				queue_no = queue % 48;
			}

			temp = sys_reg_read(QTX_CFG_0 + 0x10 * queue_no);
			tx_des_cnt = (temp & 0xffff0000) >> 16;
			hw_resv = (temp & 0xff00) >> 8;
			sw_resv = (temp & 0xff);
			temp = sys_reg_read(QTX_CFG_0 + (0x10 * queue_no) + 0x4);
			sch = (temp >> 31) + 1;
			min_en = (temp & 0x8000000) >> 27;
			min_rate = (temp & 0x7f00000) >> 20;
			for (i = 0; i < (temp & 0xf0000) >> 16; i++)
				min_rate *= 10;
			max_en = (temp & 0x800) >> 11;
			max_rate = (temp & 0x7f0) >> 4;
			for (i = 0; i < (temp & 0xf); i++)
				max_rate *= 10;
			weight = (temp & 0xf000) >> 12;
			queue_head = sys_reg_read(QTX_HEAD_0 + 0x10 * queue_no);
			queue_tail = sys_reg_read(QTX_TAIL_0 + 0x10 * queue_no);

			seq_printf(seq, "Queue#%d Information:\n", queue);
			seq_printf(seq,
				   "%d packets in the queue; head address is 0x%08x, tail address is 0x%08x.\n",
				   tx_des_cnt, queue_head, queue_tail);
			seq_printf(seq,
				   "HW_RESV: %d; SW_RESV: %d; SCH: %d; Weighting: %d\n",
				   hw_resv, sw_resv, sch, weight);
			seq_printf(seq,
				   "Min_Rate_En is %d, Min_Rate is %dKbps; Max_Rate_En is %d, Max_Rate is %dKbps.\n\n",
				   min_en, min_rate, max_en, max_rate);
		}
		if (ei_local->features & FE_HW_SFQ) {
			seq_puts(seq, "==== Virtual Queue Information ====\n");
			seq_printf(seq,
				   "VQTX_TB_BASE_0:0x%p;VQTX_TB_BASE_1:0x%p;VQTX_TB_BASE_2:0x%p;VQTX_TB_BASE_3:0x%p\n",
				   sfq0, sfq1, sfq2, sfq3);
			temp = sys_reg_read(VQTX_NUM);
			seq_printf(seq,
				   "VQTX_NUM_0:0x%01x;VQTX_NUM_1:0x%01x;VQTX_NUM_2:0x%01x;VQTX_NUM_3:0x%01x\n\n",
				   temp & 0xF, (temp & 0xF0) >> 4,
				   (temp & 0xF00) >> 8, (temp & 0xF000) >> 12);
		}

		seq_puts(seq, "==== Flow Control Information ====\n");
		temp = sys_reg_read(QDMA_FC_THRES);
		seq_printf(seq,
			   "SW_DROP_EN:%x; SW_DROP_FFA:%d; SW_DROP_MODE:%d\n",
			   (temp & 0x1000000) >> 24, (temp & 0x2000000) >> 25,
			   (temp & 0x30000000) >> 28);
		seq_printf(seq,
			   "WH_DROP_EN:%x; HW_DROP_FFA:%d; HW_DROP_MODE:%d\n",
			   (temp & 0x10000) >> 16, (temp & 0x20000) >> 17,
			   (temp & 0x300000) >> 20);
		seq_printf(seq, "SW_DROP_FSTVQ_MODE:%d;SW_DROP_FSTVQ:%d\n",
			   (temp & 0xC0000000) >> 30,
			   (temp & 0x08000000) >> 27);
		seq_printf(seq, "HW_DROP_FSTVQ_MODE:%d;HW_DROP_FSTVQ:%d\n",
			   (temp & 0xC00000) >> 22, (temp & 0x080000) >> 19);

		seq_puts(seq, "\n==== FSM Information\n");
		temp = sys_reg_read(QDMA_DMA);
		seq_printf(seq, "VQTB_FSM:0x%01x\n", (temp & 0x0F000000) >> 24);
		seq_printf(seq, "FQ_FSM:0x%01x\n", (temp & 0x000F0000) >> 16);
		seq_printf(seq, "TX_FSM:0x%01x\n", (temp & 0x00000F00) >> 8);
		seq_printf(seq, "RX_FSM:0x%01x\n\n", (temp & 0x0000000f));

		seq_puts(seq, "==== M2Q Information ====\n");
		for (i = 0; i < 64; i += 8) {
			seq_printf(seq,
				   " (%d,%d)(%d,%d)(%d,%d)(%d,%d)(%d,%d)(%d,%d)(%d,%d)(%d,%d)\n",
				   i, M2Q_table[i], i + 1, M2Q_table[i + 1],
				   i + 2, M2Q_table[i + 2], i + 3,
				   M2Q_table[i + 3], i + 4, M2Q_table[i + 4],
				   i + 5, M2Q_table[i + 5], i + 6,
				   M2Q_table[i + 6], i + 7, M2Q_table[i + 7]);
		}

		return 0;
	} else {
		return 0;
	}
}

int qdma_read(struct seq_file *seq, void *v)
{
	struct END_DEVICE *ei_local = netdev_priv(dev_raether);

	if (ei_local->features & FE_QDMA) {
		unsigned int temp, i;
		unsigned int sw_fq, hw_fq;
		unsigned int min_en, min_rate, max_en, max_rate, sch, weight;
		unsigned int queue, tx_des_cnt, hw_resv, sw_resv, queue_head,
		    queue_tail;
		struct net_device *dev = dev_raether;
		struct END_DEVICE *ei_local = netdev_priv(dev);

		seq_puts(seq, "==== General Information ====\n");
		temp = sys_reg_read(QDMA_FQ_CNT);
		sw_fq = (temp & 0xFFFF0000) >> 16;
		hw_fq = (temp & 0x0000FFFF);
		seq_printf(seq, "SW TXD: %d/%d; HW TXD: %d/%d\n", sw_fq,
			   num_tx_desc, hw_fq, NUM_QDMA_PAGE);
		seq_printf(seq, "SW TXD virtual start address: 0x%p\n",
			   ei_local->txd_pool);
		seq_printf(seq, "HW TXD virtual start address: 0x%p\n\n",
			   free_head);

		seq_puts(seq, "==== Scheduler Information ====\n");
		temp = sys_reg_read(QDMA_TX_SCH);
		max_en = (temp & 0x00000800) >> 11;
		max_rate = (temp & 0x000007F0) >> 4;
		for (i = 0; i < (temp & 0x0000000F); i++)
			max_rate *= 10;
		seq_printf(seq, "SCH1 rate control:%d. Rate is %dKbps.\n",
			   max_en, max_rate);
		max_en = (temp & 0x08000000) >> 27;
		max_rate = (temp & 0x07F00000) >> 20;
		for (i = 0; i < (temp & 0x000F0000); i++)
			max_rate *= 10;
		seq_printf(seq, "SCH2 rate control:%d. Rate is %dKbps.\n\n",
			   max_en, max_rate);

		seq_puts(seq, "==== Physical Queue Information ====\n");
		for (queue = 0; queue < 16; queue++) {
			temp = sys_reg_read(QTX_CFG_0 + 0x10 * queue);
			tx_des_cnt = (temp & 0xffff0000) >> 16;
			hw_resv = (temp & 0xff00) >> 8;
			sw_resv = (temp & 0xff);
			temp = sys_reg_read(QTX_CFG_0 + (0x10 * queue) + 0x4);
			sch = (temp >> 31) + 1;
			min_en = (temp & 0x8000000) >> 27;
			min_rate = (temp & 0x7f00000) >> 20;
			for (i = 0; i < (temp & 0xf0000) >> 16; i++)
				min_rate *= 10;
			max_en = (temp & 0x800) >> 11;
			max_rate = (temp & 0x7f0) >> 4;
			for (i = 0; i < (temp & 0xf); i++)
				max_rate *= 10;
			weight = (temp & 0xf000) >> 12;
			queue_head = sys_reg_read(QTX_HEAD_0 + 0x10 * queue);
			queue_tail = sys_reg_read(QTX_TAIL_0 + 0x10 * queue);

			seq_printf(seq, "Queue#%d Information:\n", queue);
			seq_printf(seq,
				   "%d packets in the queue; head address is 0x%08x, tail address is 0x%08x.\n",
				   tx_des_cnt, queue_head, queue_tail);
			seq_printf(seq,
				   "HW_RESV: %d; SW_RESV: %d; SCH: %d; Weighting: %d\n",
				   hw_resv, sw_resv, sch, weight);
			seq_printf(seq,
				   "Min_Rate_En is %d, Min_Rate is %dKbps; Max_Rate_En is %d, Max_Rate is %dKbps.\n\n",
				   min_en, min_rate, max_en, max_rate);
		}
		if (ei_local->features & FE_HW_SFQ) {
			seq_puts(seq, "==== Virtual Queue Information ====\n");
			seq_printf(seq,
				   "VQTX_TB_BASE_0:0x%p;VQTX_TB_BASE_1:0x%p;VQTX_TB_BASE_2:0x%p;VQTX_TB_BASE_3:0x%p\n",
				   sfq0, sfq1, sfq2, sfq3);
			temp = sys_reg_read(VQTX_NUM);
			seq_printf(seq,
				   "VQTX_NUM_0:0x%01x;VQTX_NUM_1:0x%01x;VQTX_NUM_2:0x%01x;VQTX_NUM_3:0x%01x\n\n",
				   temp & 0xF, (temp & 0xF0) >> 4,
				   (temp & 0xF00) >> 8, (temp & 0xF000) >> 12);
		}

		seq_puts(seq, "==== Flow Control Information ====\n");
		temp = sys_reg_read(QDMA_FC_THRES);
		seq_printf(seq,
			   "SW_DROP_EN:%x; SW_DROP_FFA:%d; SW_DROP_MODE:%d\n",
			   (temp & 0x1000000) >> 24, (temp & 0x2000000) >> 25,
			   (temp & 0x30000000) >> 28);
		seq_printf(seq,
			   "WH_DROP_EN:%x; HW_DROP_FFA:%d; HW_DROP_MODE:%d\n",
			   (temp & 0x10000) >> 16, (temp & 0x20000) >> 17,
			   (temp & 0x300000) >> 20);
		seq_printf(seq, "SW_DROP_FSTVQ_MODE:%d;SW_DROP_FSTVQ:%d\n",
			   (temp & 0xC0000000) >> 30,
			   (temp & 0x08000000) >> 27);
		seq_printf(seq, "HW_DROP_FSTVQ_MODE:%d;HW_DROP_FSTVQ:%d\n",
			   (temp & 0xC00000) >> 22, (temp & 0x080000) >> 19);

		seq_puts(seq, "\n==== FSM Information\n");
		temp = sys_reg_read(QDMA_DMA);
		seq_printf(seq, "VQTB_FSM:0x%01x\n", (temp & 0x0F000000) >> 24);
		seq_printf(seq, "FQ_FSM:0x%01x\n", (temp & 0x000F0000) >> 16);
		seq_printf(seq, "TX_FSM:0x%01x\n", (temp & 0x00000F00) >> 8);
		seq_printf(seq, "RX_FSM:0x%01x\n\n", (temp & 0x0000000f));

		seq_puts(seq, "==== M2Q Information ====\n");
		for (i = 0; i < 64; i += 8) {
			seq_printf(seq,
				   " (%d,%d)(%d,%d)(%d,%d)(%d,%d)(%d,%d)(%d,%d)(%d,%d)(%d,%d)\n",
				   i, M2Q_table[i], i + 1, M2Q_table[i + 1],
				   i + 2, M2Q_table[i + 2], i + 3,
				   M2Q_table[i + 3], i + 4, M2Q_table[i + 4],
				   i + 5, M2Q_table[i + 5], i + 6,
				   M2Q_table[i + 6], i + 7, M2Q_table[i + 7]);
		}

		return 0;
	} else {
		return 0;
	}
}

static int qdma_open(struct inode *inode, struct file *file)
{
	return single_open(file, qdma_read, NULL);
}

static const struct file_operations qdma_fops = {
	.owner = THIS_MODULE,
	.open = qdma_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release
};

int tx_ring_read(struct seq_file *seq, void *v)
{
	struct END_DEVICE *ei_local = netdev_priv(dev_raether);
	struct PDMA_txdesc *tx_ring;
	int i = 0;

	tx_ring = kmalloc_array(num_tx_desc, sizeof(*tx_ring), GFP_KERNEL);

	if (!tx_ring)
		/*seq_puts(seq, " allocate temp tx_ring fail.\n"); */
		return 0;

	for (i = 0; i < num_tx_desc; i++)
		tx_ring[i] = ei_local->tx_ring0[i];

	for (i = 0; i < num_tx_desc; i++) {
		seq_printf(seq, "%d: %08x %08x %08x %08x\n", i,
			   *(int *)&tx_ring[i].txd_info1,
			   *(int *)&tx_ring[i].txd_info2,
			   *(int *)&tx_ring[i].txd_info3,
			   *(int *)&tx_ring[i].txd_info4);
	}

	kfree(tx_ring);
	return 0;
}

static int tx_ring_open(struct inode *inode, struct file *file)
{
	struct END_DEVICE *ei_local = netdev_priv(dev_raether);

	if (!(ei_local->features & FE_QDMA)) {
		return single_open(file, tx_ring_read, NULL);
	} else if (ei_local->features & FE_QDMA) {
		if (ei_local->chip_name == MT7622_FE)
			return single_open(file, qdma_read_64queue, NULL);
		else
			return single_open(file, qdma_read, NULL);
	} else {
		return 0;
	}
}

static const struct file_operations tx_ring_fops = {
	.owner = THIS_MODULE,
	.open = tx_ring_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release
};

int rx_ring_read(struct seq_file *seq, void *v)
{
	struct END_DEVICE *ei_local = netdev_priv(dev_raether);
	struct PDMA_rxdesc *rx_ring;
	int i = 0;

	rx_ring = kmalloc_array(num_rx_desc, sizeof(*rx_ring), GFP_KERNEL);
	if (!rx_ring)
		/*seq_puts(seq, " allocate temp rx_ring fail.\n"); */
		return 0;

	for (i = 0; i < num_rx_desc; i++) {
		memcpy(&rx_ring[i], &ei_local->rx_ring[0][i],
		       sizeof(struct PDMA_rxdesc));
	}

	for (i = 0; i < num_rx_desc; i++) {
		seq_printf(seq, "%d: %08x %08x %08x %08x\n", i,
			   *(int *)&rx_ring[i].rxd_info1,
			   *(int *)&rx_ring[i].rxd_info2,
			   *(int *)&rx_ring[i].rxd_info3,
			   *(int *)&rx_ring[i].rxd_info4);
	}

	kfree(rx_ring);
	return 0;
}

static int rx_ring_open(struct inode *inode, struct file *file)
{
	return single_open(file, rx_ring_read, NULL);
}

static const struct file_operations rx_ring_fops = {
	.owner = THIS_MODULE,
	.open = rx_ring_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release
};

int num_of_txd_update(int num_of_txd)
{
	struct END_DEVICE *ei_local = netdev_priv(dev_raether);

	if (ei_local->features & FE_TSO)
		txd_cnt[num_of_txd]++;
	return 0;
}

static void *seq_tso_txd_num_start(struct seq_file *seq, loff_t *pos)
{
	struct END_DEVICE *ei_local = netdev_priv(dev_raether);

	if (ei_local->features & FE_TSO) {
		seq_puts(seq, "TXD | Count\n");
		if (*pos < (MAX_SKB_FRAGS / 2 + 1))
			return pos;
	}
	return NULL;
}

static void *seq_tso_txd_num_next(struct seq_file *seq, void *v, loff_t *pos)
{
	struct END_DEVICE *ei_local = netdev_priv(dev_raether);

	if (ei_local->features & FE_TSO) {
		(*pos)++;
		if (*pos >= (MAX_SKB_FRAGS / 2 + 1))
			return NULL;
		return pos;
	} else {
		return NULL;
	}
}

static void seq_tso_txd_num_stop(struct seq_file *seq, void *v)
{
	/* Nothing to do */
}

static int seq_tso_txd_num_show(struct seq_file *seq, void *v)
{
	struct END_DEVICE *ei_local = netdev_priv(dev_raether);

	if (ei_local->features & FE_TSO) {
		int i = *(loff_t *)v;

		seq_printf(seq, "%d: %d\n", i, txd_cnt[i]);
	}
	return 0;
}

ssize_t num_of_txd_write(struct file *file, const char __user *buffer,
			 size_t count, loff_t *data)
{
	struct END_DEVICE *ei_local = netdev_priv(dev_raether);

	if (ei_local->features & FE_TSO) {
		memset(txd_cnt, 0, sizeof(txd_cnt));
		pr_debug("clear txd cnt table\n");
		return count;
	} else {
		return 0;
	}
}

int tso_len_update(int tso_len)
{
	struct END_DEVICE *ei_local = netdev_priv(dev_raether);

	if (ei_local->features & FE_TSO) {
		if (tso_len > 70000)
			tso_cnt[14]++;
		else if (tso_len > 65000)
			tso_cnt[13]++;
		else if (tso_len > 60000)
			tso_cnt[12]++;
		else if (tso_len > 55000)
			tso_cnt[11]++;
		else if (tso_len > 50000)
			tso_cnt[10]++;
		else if (tso_len > 45000)
			tso_cnt[9]++;
		else if (tso_len > 40000)
			tso_cnt[8]++;
		else if (tso_len > 35000)
			tso_cnt[7]++;
		else if (tso_len > 30000)
			tso_cnt[6]++;
		else if (tso_len > 25000)
			tso_cnt[5]++;
		else if (tso_len > 20000)
			tso_cnt[4]++;
		else if (tso_len > 15000)
			tso_cnt[3]++;
		else if (tso_len > 10000)
			tso_cnt[2]++;
		else if (tso_len > 5000)
			tso_cnt[1]++;
		else
			tso_cnt[0]++;
	}
	return 0;
}

ssize_t tso_len_write(struct file *file, const char __user *buffer,
		      size_t count, loff_t *data)
{
	struct END_DEVICE *ei_local = netdev_priv(dev_raether);

	if (ei_local->features & FE_TSO) {
		memset(tso_cnt, 0, sizeof(tso_cnt));
		pr_debug("clear tso cnt table\n");
		return count;
	} else {
		return 0;
	}
}

static void *seq_tso_len_start(struct seq_file *seq, loff_t *pos)
{
	struct END_DEVICE *ei_local = netdev_priv(dev_raether);

	if (ei_local->features & FE_TSO) {
		seq_puts(seq, " Length  | Count\n");
		if (*pos < 15)
			return pos;
	}
	return NULL;
}

static void *seq_tso_len_next(struct seq_file *seq, void *v, loff_t *pos)
{
	struct END_DEVICE *ei_local = netdev_priv(dev_raether);

	if (ei_local->features & FE_TSO) {
		(*pos)++;
		if (*pos >= 15)
			return NULL;
		return pos;
	} else {
		return NULL;
	}
}

static void seq_tso_len_stop(struct seq_file *seq, void *v)
{
	/* Nothing to do */
}

static int seq_tso_len_show(struct seq_file *seq, void *v)
{
	struct END_DEVICE *ei_local = netdev_priv(dev_raether);

	if (ei_local->features & FE_TSO) {
		int i = *(loff_t *)v;

		seq_printf(seq, "%d~%d: %d\n", i * 5000, (i + 1) * 5000,
			   tso_cnt[i]);
	}
	return 0;
}

static const struct seq_operations seq_tso_txd_num_ops = {
	.start = seq_tso_txd_num_start,
	.next = seq_tso_txd_num_next,
	.stop = seq_tso_txd_num_stop,
	.show = seq_tso_txd_num_show
};

static int tso_txd_num_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &seq_tso_txd_num_ops);
}

static const struct file_operations tso_txd_num_fops = {
	.owner = THIS_MODULE,
	.open = tso_txd_num_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = num_of_txd_write,
	.release = seq_release
};

static const struct seq_operations seq_tso_len_ops = {
	.start = seq_tso_len_start,
	.next = seq_tso_len_next,
	.stop = seq_tso_len_stop,
	.show = seq_tso_len_show
};

static int tso_len_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &seq_tso_len_ops);
}

static const struct file_operations tso_len_fops = {
	.owner = THIS_MODULE,
	.open = tso_len_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = tso_len_write,
	.release = seq_release
};

static int lro_len_update(struct net_lro_desc *lro_desc)
{
	struct END_DEVICE *ei_local = netdev_priv(dev_raether);

	if (ei_local->features & FE_SW_LRO) {
		int len_idx;

		if (lro_desc->ip_tot_len > 65000)
			len_idx = 13;
		else if (lro_desc->ip_tot_len > 60000)
			len_idx = 12;
		else if (lro_desc->ip_tot_len > 55000)
			len_idx = 11;
		else if (lro_desc->ip_tot_len > 50000)
			len_idx = 10;
		else if (lro_desc->ip_tot_len > 45000)
			len_idx = 9;
		else if (lro_desc->ip_tot_len > 40000)
			len_idx = 8;
		else if (lro_desc->ip_tot_len > 35000)
			len_idx = 7;
		else if (lro_desc->ip_tot_len > 30000)
			len_idx = 6;
		else if (lro_desc->ip_tot_len > 25000)
			len_idx = 5;
		else if (lro_desc->ip_tot_len > 20000)
			len_idx = 4;
		else if (lro_desc->ip_tot_len > 15000)
			len_idx = 3;
		else if (lro_desc->ip_tot_len > 10000)
			len_idx = 2;
		else if (lro_desc->ip_tot_len > 5000)
			len_idx = 1;
		else
			len_idx = 0;

		return len_idx;
	} else {
		return 0;
	}
}

void lro_flush_aggr_cnt(struct net_lro_mgr *lro_mgr)
{
	struct net_lro_desc *tmp;
	int j = 0;
	int i = 0;
	int len_idx;

	for (i = 0; i < MAX_DESC; i++) {
		tmp = &lro_mgr->lro_arr[i];
		if (tmp->pkt_aggr_cnt != 0) {
			for (j = 0; j <= MAX_AGGR; j++) {
				if (tmp->pkt_aggr_cnt == j)
					lro_flush_cnt[j]++;
			}
			len_idx = lro_len_update(tmp);
			lro_len_cnt1[len_idx]++;
			tot_called1++;
		}
		aggregated[i] = 0;
	}
}

void lro_agg_stats_cnt(struct net_lro_mgr *lro_mgr)
{
	struct net_lro_desc *tmp;
	int j = 0;
	int i = 0;

	for (i = 0; i < MAX_DESC; i++) {
		tmp = &lro_mgr->lro_arr[i];
		if ((aggregated[i] != tmp->pkt_aggr_cnt) &&
		    (tmp->pkt_aggr_cnt == 0)) {
			aggregated[i]++;
			for (j = 0; j <= MAX_AGGR; j++) {
				if (aggregated[i] == j)
					lro_stats_cnt[j]++;
			}
			aggregated[i] = 0;
			/* len_idx = lro_len_update(tmp); */
			/* lro_len_cnt2[len_idx]++; */
			tot_called2++;
		}
	}
}

void lro_else_stats_cnt(struct net_lro_mgr *lro_mgr)
{
	struct net_lro_desc *tmp;
	int j = 0;
	int i = 0;

	for (i = 0; i < MAX_DESC; i++) {
		tmp = &lro_mgr->lro_arr[i];
		if ((aggregated[i] != 0) && (tmp->pkt_aggr_cnt == 0)) {
			for (j = 0; j <= MAX_AGGR; j++) {
				if (aggregated[i] == j)
					lro_stats_cnt[j]++;
			}
			aggregated[i] = 0;
			/* len_idx = lro_len_update(tmp); */
			/* lro_len_cnt2[len_idx]++; */
			force_flush++;
			tot_called2++;
		}
	}
}

void lro_no_agg(struct net_lro_mgr *lro_mgr)
{
	struct net_lro_desc *tmp;
	int i = 0;

	for (i = 0; i < MAX_DESC; i++) {
		tmp = &lro_mgr->lro_arr[i];
		if (tmp->active) {
			if (aggregated[i] != tmp->pkt_aggr_cnt)
				aggregated[i] = tmp->pkt_aggr_cnt;
			else
				aggregated[i] = 0;
		}
	}
}

int lro_stats_update(struct net_lro_mgr *lro_mgr, bool all_flushed)
{
	struct END_DEVICE *ei_local = netdev_priv(dev_raether);

	if (ei_local->features & FE_SW_LRO) {
		int tmp_stats;

		if (all_flushed) {
			lro_flush_aggr_cnt(lro_mgr);
		} else {
			if (lro_flushed != lro_mgr->stats.flushed) {
				tmp_stats = lro_mgr->stats.aggregated;
				if (lro_aggregated != tmp_stats)
					lro_agg_stats_cnt(lro_mgr);
				else
					lro_else_stats_cnt(lro_mgr);
			} else {
				if (lro_aggregated != lro_mgr->stats.aggregated)
					lro_no_agg(lro_mgr);
			}
		}

		lro_aggregated = lro_mgr->stats.aggregated;
		lro_flushed = lro_mgr->stats.flushed;
		lro_nodesc = lro_mgr->stats.no_desc;

		return 0;
	} else {
		return 0;
	}
}

ssize_t lro_stats_write(struct file *file, const char __user *buffer,
			size_t count, loff_t *data)
{
	struct END_DEVICE *ei_local = netdev_priv(dev_raether);

	if (ei_local->features & FE_SW_LRO) {
		memset(lro_stats_cnt, 0, sizeof(lro_stats_cnt));
		memset(lro_flush_cnt, 0, sizeof(lro_flush_cnt));
		memset(lro_len_cnt1, 0, sizeof(lro_len_cnt1));
		/* memset(lro_len_cnt2, 0, sizeof(lro_len_cnt2)); */
		memset(aggregated, 0, sizeof(aggregated));
		lro_aggregated = 0;
		lro_flushed = 0;
		lro_nodesc = 0;
		force_flush = 0;
		tot_called1 = 0;
		tot_called2 = 0;
		pr_debug("clear lro  cnt table\n");

		return count;
	} else {
		return 0;
	}
}

int lro_stats_read(struct seq_file *seq, void *v)
{
	struct END_DEVICE *ei_local = netdev_priv(dev_raether);

	if (ei_local->features & FE_SW_LRO) {
		int i;
		int tot_cnt = 0;
		int tot_aggr = 0;
		int ave_aggr = 0;

		seq_puts(seq, "LRO statistic dump:\n");
		seq_puts(seq, "Cnt:   Kernel | Driver\n");
		for (i = 0; i <= MAX_AGGR; i++) {
			tot_cnt = tot_cnt + lro_stats_cnt[i] + lro_flush_cnt[i];
			seq_printf(seq, " %d :      %d        %d\n", i,
				   lro_stats_cnt[i], lro_flush_cnt[i]);
			tot_aggr =
			    tot_aggr + i * (lro_stats_cnt[i] +
					    lro_flush_cnt[i]);
		}
		ave_aggr = lro_aggregated / lro_flushed;
		seq_printf(seq, "Total aggregated pkt: %d\n", lro_aggregated);
		seq_printf(seq, "Flushed pkt: %d  %d\n", lro_flushed,
			   force_flush);
		seq_printf(seq, "Average flush cnt:  %d\n", ave_aggr);
		seq_printf(seq, "No descriptor pkt: %d\n\n\n", lro_nodesc);

		seq_puts(seq, "Driver flush pkt len:\n");
		seq_puts(seq, " Length  | Count\n");
		for (i = 0; i < 15; i++) {
			seq_printf(seq, "%d~%d: %d\n", i * 5000,
				   (i + 1) * 5000, lro_len_cnt1[i]);
		}
		seq_printf(seq, "Kernel flush: %d;  Driver flush: %d\n",
			   tot_called2, tot_called1);
		return 0;
	} else {
		return 0;
	}
}

static int lro_stats_open(struct inode *inode, struct file *file)
{
	struct END_DEVICE *ei_local = netdev_priv(dev_raether);

	if (ei_local->features & FE_SW_LRO)
		return single_open(file, lro_stats_read, NULL);
	else
		return 0;
}

static const struct file_operations lro_stats_fops = {
	.owner = THIS_MODULE,
	.open = lro_stats_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = lro_stats_write,
	.release = single_release
};

static struct proc_dir_entry *proc_esw_cnt;
static struct proc_dir_entry *proc_eth_cnt;

void internal_gsw_cnt_read(struct seq_file *seq)
{
	unsigned int pkt_cnt = 0;
	int i = 0;

	seq_printf(seq,
		   "===================== %8s %8s %8s %8s %8s %8s %8s\n",
		   "Port0", "Port1", "Port2", "Port3", "Port4",
		   "Port5", "Port6");
	seq_puts(seq, "Tx Drop Packet      :");
	DUMP_EACH_PORT(0x4000);
	seq_puts(seq, "\n");
	seq_puts(seq, "Tx CRC Error        :");
	DUMP_EACH_PORT(0x4004);
	seq_puts(seq, "\n");
	seq_puts(seq, "Tx Unicast Packet   :");
	DUMP_EACH_PORT(0x4008);
	seq_puts(seq, "\n");
	seq_puts(seq, "Tx Multicast Packet :");
	DUMP_EACH_PORT(0x400C);
	seq_puts(seq, "\n");
	seq_puts(seq, "Tx Broadcast Packet :");
	DUMP_EACH_PORT(0x4010);
	seq_puts(seq, "\n");
	seq_puts(seq, "Tx Collision Event  :");
	DUMP_EACH_PORT(0x4014);
	seq_puts(seq, "\n");
	seq_puts(seq, "Tx Pause Packet     :");
	DUMP_EACH_PORT(0x402C);
	seq_puts(seq, "\n");
	seq_puts(seq, "Rx Drop Packet      :");
	DUMP_EACH_PORT(0x4060);
	seq_puts(seq, "\n");
	seq_puts(seq, "Rx Filtering Packet :");
	DUMP_EACH_PORT(0x4064);
	seq_puts(seq, "\n");
	seq_puts(seq, "Rx Unicast Packet   :");
	DUMP_EACH_PORT(0x4068);
	seq_puts(seq, "\n");
	seq_puts(seq, "Rx Multicast Packet :");
	DUMP_EACH_PORT(0x406C);
	seq_puts(seq, "\n");
	seq_puts(seq, "Rx Broadcast Packet :");
	DUMP_EACH_PORT(0x4070);
	seq_puts(seq, "\n");
	seq_puts(seq, "Rx Alignment Error  :");
	DUMP_EACH_PORT(0x4074);
	seq_puts(seq, "\n");
	seq_puts(seq, "Rx CRC Error     :");
	DUMP_EACH_PORT(0x4078);
	seq_puts(seq, "\n");
	seq_puts(seq, "Rx Undersize Error  :");
	DUMP_EACH_PORT(0x407C);
	seq_puts(seq, "\n");
	seq_puts(seq, "Rx Fragment Error   :");
	DUMP_EACH_PORT(0x4080);
	seq_puts(seq, "\n");
	seq_puts(seq, "Rx Oversize Error   :");
	DUMP_EACH_PORT(0x4084);
	seq_puts(seq, "\n");
	seq_puts(seq, "Rx Jabber Error     :");
	DUMP_EACH_PORT(0x4088);
	seq_puts(seq, "\n");
	seq_puts(seq, "Rx Pause Packet     :");
	DUMP_EACH_PORT(0x408C);
	mii_mgr_write(31, 0x4fe0, 0xf0);
	mii_mgr_write(31, 0x4fe0, 0x800000f0);

	seq_puts(seq, "\n");
}

void pse_qdma_drop_cnt(void)
{
	u8 i;

	pr_info("       <<PSE DROP CNT>>\n");
	pr_info("| FQ_PCNT_MIN : %010u |\n",
		(sys_reg_read(FE_PSE_FREE) & 0xff0000) >> 16);
	pr_info("| FQ_PCNT     : %010u |\n",
		sys_reg_read(FE_PSE_FREE) & 0x00ff);
	pr_info("| FE_DROP_FQ  : %010u |\n",
		sys_reg_read(FE_DROP_FQ));
	pr_info("| FE_DROP_FC  : %010u |\n",
		sys_reg_read(FE_DROP_FC));
	pr_info("| FE_DROP_PPE : %010u |\n",
		sys_reg_read(FE_DROP_PPE));
	pr_info("\n       <<QDMA PKT/DROP CNT>>\n");

	sys_reg_write(QTX_MIB_IF, 0x90000000);
	for (i = 0; i < NUM_PQ; i++) {
		if (i <= 15) {
			sys_reg_write(QDMA_PAGE, 0);
			pr_info("QDMA Q%d PKT CNT: %010u, DROP CNT: %010u\n", i,
				sys_reg_read(QTX_CFG_0 + i * 16),
				sys_reg_read(QTX_SCH_0 + i * 16));
		} else if (i > 15 && i <= 31) {
			sys_reg_write(QDMA_PAGE, 1);
			pr_info("QDMA Q%d PKT CNT: %010u, DROP CNT: %010u\n", i,
				sys_reg_read(QTX_CFG_0 + (i - 16) * 16),
				sys_reg_read(QTX_SCH_0 + (i - 16) * 16));
		} else if (i > 31 && i <= 47) {
			sys_reg_write(QDMA_PAGE, 2);
			pr_info("QDMA Q%d PKT CNT: %010u, DROP CNT: %010u\n", i,
				sys_reg_read(QTX_CFG_0 + (i - 32) * 16),
				sys_reg_read(QTX_SCH_0 + (i - 32) * 16));
		} else if (i > 47 && i <= 63) {
			sys_reg_write(QDMA_PAGE, 3);
			pr_info("QDMA Q%d PKT CNT: %010u, DROP CNT: %010u\n", i,
				sys_reg_read(QTX_CFG_0 + (i - 48) * 16),
				sys_reg_read(QTX_SCH_0 + (i - 48) * 16));
		}
	}
	sys_reg_write(QDMA_PAGE, 0);
	sys_reg_write(QTX_MIB_IF, 0x0);
}

void external_gsw_cnt_read(void)
{
	rtk_hal_dump_mib();
}

void embedded_sw_cnt_read(struct seq_file *seq)
{
	seq_puts(seq, "\n       <<CPU>>\n");
	seq_puts(seq, "           |\n");
	seq_puts(seq, "                      ^\n");
	seq_printf(seq, "                      | Port6 Rx:%08u Good Pkt\n",
		   sys_reg_read(ETHDMASYS_ETH_SW_BASE + 0xE0) & 0xFFFF);
	seq_printf(seq, "                      | Port6 Tx:%08u Good Pkt\n",
		   sys_reg_read(ETHDMASYS_ETH_SW_BASE + 0xE0) >> 16);
	seq_puts(seq, "+---------------------v-------------------------+\n");
	seq_puts(seq, "|            P6                |\n");
	seq_puts(seq, "|           <<10/100 Embedded Switch>>         |\n");
	seq_puts(seq, "|     P0    P1    P2     P3     P4     P5       |\n");
	seq_puts(seq, "+-----------------------------------------------+\n");
	seq_puts(seq, "       |     |     |     |       |      |\n");
	seq_printf(seq,
		   "Port0 Good Pkt Cnt: RX=%08u Tx=%08u (Bad Pkt Cnt: Rx=%08u Tx=%08u)\n",
		   sys_reg_read(ETHDMASYS_ETH_SW_BASE + 0xE8) & 0xFFFF,
		   sys_reg_read(ETHDMASYS_ETH_SW_BASE + 0x150) & 0xFFFF,
		   sys_reg_read(ETHDMASYS_ETH_SW_BASE + 0xE8) >> 16,
		   sys_reg_read(ETHDMASYS_ETH_SW_BASE + 0x150) >> 16);

	seq_printf(seq,
		   "Port1 Good Pkt Cnt: RX=%08u Tx=%08u (Bad Pkt Cnt: Rx=%08u Tx=%08u)\n",
		   sys_reg_read(ETHDMASYS_ETH_SW_BASE + 0xEC) & 0xFFFF,
		   sys_reg_read(ETHDMASYS_ETH_SW_BASE + 0x154) & 0xFFFF,
		   sys_reg_read(ETHDMASYS_ETH_SW_BASE + 0xEC) >> 16,
		   sys_reg_read(ETHDMASYS_ETH_SW_BASE + 0x154) >> 16);

	seq_printf(seq,
		   "Port2 Good Pkt Cnt: RX=%08u Tx=%08u (Bad Pkt Cnt: Rx=%08u Tx=%08u)\n",
		   sys_reg_read(ETHDMASYS_ETH_SW_BASE + 0xF0) & 0xFFFF,
		   sys_reg_read(ETHDMASYS_ETH_SW_BASE + 0x158) & 0xFFFF,
		   sys_reg_read(ETHDMASYS_ETH_SW_BASE + 0xF0) >> 16,
		   sys_reg_read(ETHDMASYS_ETH_SW_BASE + 0x158) >> 16);

	seq_printf(seq,
		   "Port3 Good Pkt Cnt: RX=%08u Tx=%08u (Bad Pkt Cnt: Rx=%08u Tx=%08u)\n",
		   sys_reg_read(ETHDMASYS_ETH_SW_BASE + 0xF4) & 0xFFFF,
		   sys_reg_read(ETHDMASYS_ETH_SW_BASE + 0x15C) & 0xFFFF,
		   sys_reg_read(ETHDMASYS_ETH_SW_BASE + 0xF4) >> 16,
		   sys_reg_read(ETHDMASYS_ETH_SW_BASE + 0x15c) >> 16);

	seq_printf(seq,
		   "Port4 Good Pkt Cnt: RX=%08u Tx=%08u (Bad Pkt Cnt: Rx=%08u Tx=%08u)\n",
		   sys_reg_read(ETHDMASYS_ETH_SW_BASE + 0xF8) & 0xFFFF,
		   sys_reg_read(ETHDMASYS_ETH_SW_BASE + 0x160) & 0xFFFF,
		   sys_reg_read(ETHDMASYS_ETH_SW_BASE + 0xF8) >> 16,
		   sys_reg_read(ETHDMASYS_ETH_SW_BASE + 0x160) >> 16);

	seq_printf(seq,
		   "Port5 Good Pkt Cnt: RX=%08u Tx=%08u (Bad Pkt Cnt: Rx=%08u Tx=%08u)\n",
		   sys_reg_read(ETHDMASYS_ETH_SW_BASE + 0xFC) & 0xFFFF,
		   sys_reg_read(ETHDMASYS_ETH_SW_BASE + 0x164) & 0xFFFF,
		   sys_reg_read(ETHDMASYS_ETH_SW_BASE + 0xFC) >> 16,
		   sys_reg_read(ETHDMASYS_ETH_SW_BASE + 0x164) >> 16);
}

int eth_cnt_read(struct seq_file *seq, void *v)
{
	pse_qdma_drop_cnt();
	return 0;
}

int esw_cnt_read(struct seq_file *seq, void *v)
{
	struct END_DEVICE *ei_local = netdev_priv(dev_raether);

	seq_puts(seq, "                  <<CPU>>\n");
	seq_puts(seq, "+-----------------------------------------------+\n");
	seq_puts(seq, "|		  <<PSE>>		        |\n");
	seq_puts(seq, "+-----------------------------------------------+\n");
	seq_puts(seq, "+-----------------------------------------------+\n");
	seq_puts(seq, "|		  <<GDMA>>		        |\n");

	seq_printf(seq,
		   "| GDMA1_RX_GBCNT  : %010u (Rx Good Bytes)	|\n",
		   sys_reg_read(RALINK_FRAME_ENGINE_BASE + 0x2400));
	seq_printf(seq,
		   "| GDMA1_RX_GPCNT  : %010u (Rx Good Pkts)	|\n",
		   sys_reg_read(RALINK_FRAME_ENGINE_BASE + 0x2408));
	seq_printf(seq,
		   "| GDMA1_RX_OERCNT : %010u (overflow error)	|\n",
		   sys_reg_read(RALINK_FRAME_ENGINE_BASE + 0x2410));
	seq_printf(seq, "| GDMA1_RX_FERCNT : %010u (FCS error)	|\n",
		   sys_reg_read(RALINK_FRAME_ENGINE_BASE + 0x2414));
	seq_printf(seq, "| GDMA1_RX_SERCNT : %010u (too short)	|\n",
		   sys_reg_read(RALINK_FRAME_ENGINE_BASE + 0x2418));
	seq_printf(seq, "| GDMA1_RX_LERCNT : %010u (too long)	|\n",
		   sys_reg_read(RALINK_FRAME_ENGINE_BASE + 0x241C));
	seq_printf(seq,
		   "| GDMA1_RX_CERCNT : %010u (checksum error)	|\n",
		   sys_reg_read(RALINK_FRAME_ENGINE_BASE + 0x2420));
	seq_printf(seq,
		   "| GDMA1_RX_FCCNT  : %010u (flow control)	|\n",
		   sys_reg_read(RALINK_FRAME_ENGINE_BASE + 0x2424));
	seq_printf(seq,
		   "| GDMA1_TX_SKIPCNT: %010u (about count)	|\n",
		   sys_reg_read(RALINK_FRAME_ENGINE_BASE + 0x2428));
	seq_printf(seq,
		   "| GDMA1_TX_COLCNT : %010u (collision count)	|\n",
		   sys_reg_read(RALINK_FRAME_ENGINE_BASE + 0x242C));
	seq_printf(seq,
		   "| GDMA1_TX_GBCNT  : %010u (Tx Good Bytes)	|\n",
		   sys_reg_read(RALINK_FRAME_ENGINE_BASE + 0x2430));
	seq_printf(seq,
		   "| GDMA1_TX_GPCNT  : %010u (Tx Good Pkts)	|\n",
		   sys_reg_read(RALINK_FRAME_ENGINE_BASE + 0x2438));
	seq_puts(seq, "|						|\n");
	seq_printf(seq,
		   "| GDMA2_RX_GBCNT  : %010u (Rx Good Bytes)	|\n",
		   sys_reg_read(RALINK_FRAME_ENGINE_BASE + 0x2440));
	seq_printf(seq,
		   "| GDMA2_RX_GPCNT  : %010u (Rx Good Pkts)	|\n",
		   sys_reg_read(RALINK_FRAME_ENGINE_BASE + 0x2448));
	seq_printf(seq,
		   "| GDMA2_RX_OERCNT : %010u (overflow error)	|\n",
		   sys_reg_read(RALINK_FRAME_ENGINE_BASE + 0x2450));
	seq_printf(seq, "| GDMA2_RX_FERCNT : %010u (FCS error)	|\n",
		   sys_reg_read(RALINK_FRAME_ENGINE_BASE + 0x2454));
	seq_printf(seq, "| GDMA2_RX_SERCNT : %010u (too short)	|\n",
		   sys_reg_read(RALINK_FRAME_ENGINE_BASE + 0x2458));
	seq_printf(seq, "| GDMA2_RX_LERCNT : %010u (too long)	|\n",
		   sys_reg_read(RALINK_FRAME_ENGINE_BASE + 0x245C));
	seq_printf(seq,
		   "| GDMA2_RX_CERCNT : %010u (checksum error)	|\n",
		   sys_reg_read(RALINK_FRAME_ENGINE_BASE + 0x2460));
	seq_printf(seq,
		   "| GDMA2_RX_FCCNT  : %010u (flow control)	|\n",
		   sys_reg_read(RALINK_FRAME_ENGINE_BASE + 0x2464));
	seq_printf(seq,
		   "| GDMA2_TX_SKIPCNT: %010u (skip)		|\n",
		   sys_reg_read(RALINK_FRAME_ENGINE_BASE + 0x2468));
	seq_printf(seq, "| GDMA2_TX_COLCNT : %010u (collision)	|\n",
		   sys_reg_read(RALINK_FRAME_ENGINE_BASE + 0x246C));
	seq_printf(seq,
		   "| GDMA2_TX_GBCNT  : %010u (Tx Good Bytes)	|\n",
		   sys_reg_read(RALINK_FRAME_ENGINE_BASE + 0x2470));
	seq_printf(seq,
		   "| GDMA2_TX_GPCNT  : %010u (Tx Good Pkts)	|\n",
		   sys_reg_read(RALINK_FRAME_ENGINE_BASE + 0x2478));

	seq_puts(seq, "+-----------------------------------------------+\n");

	seq_puts(seq, "\n");

	if ((ei_local->chip_name == MT7623_FE) || ei_local->chip_name == MT7621_FE)
		internal_gsw_cnt_read(seq);
	if (ei_local->architecture & (GE1_SGMII_FORCE_2500 | GE2_SGMII_FORCE_2500))
		external_gsw_cnt_read();
	else if (ei_local->architecture & RAETH_ESW)
		embedded_sw_cnt_read(seq);

	return 0;
}

static int switch_count_open(struct inode *inode, struct file *file)
{
	return single_open(file, esw_cnt_read, NULL);
}

static int eth_count_open(struct inode *inode, struct file *file)
{
	return single_open(file, eth_cnt_read, NULL);
}

static const struct file_operations switch_count_fops = {
	.owner = THIS_MODULE,
	.open = switch_count_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release
};

static const struct file_operations eth_count_fops = {
	.owner = THIS_MODULE,
	.open = eth_count_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release
};

/* proc write procedure */
static ssize_t change_phyid(struct file *file,
			    const char __user *buffer, size_t count,
			    loff_t *data)
{
	int val = 0;
	struct END_DEVICE *ei_local = netdev_priv(dev_raether);

	if (ei_local->features & (FE_ETHTOOL | FE_GE2_SUPPORT)) {
		char buf[32];
		struct net_device *cur_dev_p;
		struct END_DEVICE *ei_local;
		char if_name[64];
		unsigned int phy_id;

		if (count > 32)
			count = 32;
		memset(buf, 0, 32);
		if (copy_from_user(buf, buffer, count))
			return -EFAULT;

		/* determine interface name */
		strncpy(if_name, DEV_NAME, sizeof(if_name) - 1);	/* "eth2" by default */
		if (isalpha(buf[0])) {
			val = sscanf(buf, "%4s %1d", if_name, &phy_id);
			if (val == -1)
				return -EFAULT;
		} else {
			phy_id = kstrtol(buf, 10, NULL);
		}
		cur_dev_p = dev_get_by_name(&init_net, DEV_NAME);

		if (!cur_dev_p)
			return -EFAULT;

		ei_local = netdev_priv(cur_dev_p);

		ei_local->mii_info.phy_id = (unsigned char)phy_id;
		return count;
	} else {
		return 0;
	}
}

static ssize_t change_gmac2_phyid(struct file *file,
				  const char __user *buffer,
				  size_t count, loff_t *data)
{
	int val = 0;
	struct END_DEVICE *ei_local = netdev_priv(dev_raether);

	if (ei_local->features & (FE_ETHTOOL | FE_GE2_SUPPORT)) {
		char buf[32];
		struct net_device *cur_dev_p;
		struct PSEUDO_ADAPTER *p_pseudo_ad;
		char if_name[64];
		unsigned int phy_id;

		if (count > 32)
			count = 32;
		memset(buf, 0, 32);
		if (copy_from_user(buf, buffer, count))
			return -EFAULT;
		/* determine interface name */
		strncpy(if_name, DEV2_NAME, sizeof(if_name) - 1);	/* "eth3" by default */
		if (isalpha(buf[0])) {
			val = sscanf(buf, "%4s %1d", if_name, &phy_id);
			if (val == -1)
				return -EFAULT;
		} else {
			phy_id = kstrtol(buf, 10, NULL);
		}
		cur_dev_p = dev_get_by_name(&init_net, DEV2_NAME);

		if (!cur_dev_p)
			return -EFAULT;
		p_pseudo_ad = netdev_priv(cur_dev_p);
		p_pseudo_ad->mii_info.phy_id = (unsigned char)phy_id;
		return count;
	} else {
		return 0;
	}
}

static const struct file_operations gmac2_fops = {
	.owner = THIS_MODULE,
	.write = change_gmac2_phyid
};

static int gmac_open(struct inode *inode, struct file *file)
{
	return single_open(file, reg_read_main, NULL);
}

static const struct file_operations gmac_fops = {
	.owner = THIS_MODULE,
	.open = gmac_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = change_phyid,
	.release = single_release
};

/* #if defined(TASKLET_WORKQUEUE_SW) */

static int schedule_read(struct seq_file *seq, void *v)
{
	struct END_DEVICE *ei_local = netdev_priv(dev_raether);

	if (ei_local->features & TASKLET_WORKQUEUE_SW) {
		if (init_schedule == 1)
			seq_printf(seq,
				   "Initialize Raeth with workqueque<%d>\n",
				   init_schedule);
		else
			seq_printf(seq,
				   "Initialize Raeth with tasklet<%d>\n",
				   init_schedule);
		if (working_schedule == 1)
			seq_printf(seq,
				   "Raeth is running at workqueque<%d>\n",
				   working_schedule);
		else
			seq_printf(seq,
				   "Raeth is running at tasklet<%d>\n",
				   working_schedule);
	}

	return 0;
}

static ssize_t schedule_write(struct file *file,
			      const char __user *buffer, size_t count,
			      loff_t *data)
{
	struct END_DEVICE *ei_local = netdev_priv(dev_raether);

	if (ei_local->features & TASKLET_WORKQUEUE_SW) {
		char buf[2];
		int old;

		if (copy_from_user(buf, buffer, count))
			return -EFAULT;
		old = init_schedule;
		init_schedule = kstrtol(buf, 10, NULL);
		pr_debug
		    ("ChangeRaethInitScheduleFrom <%d> to <%d>\n",
		     old, init_schedule);
		pr_debug("Not running schedule at present !\n");

		return count;
	} else {
		return 0;
	}
}

static int schedule_switch_open(struct inode *inode, struct file *file)
{
	return single_open(file, schedule_read, NULL);
}

static const struct file_operations schedule_sw_fops = {
	.owner = THIS_MODULE,
	.open = schedule_switch_open,
	.read = seq_read,
	.write = schedule_write,
	.llseek = seq_lseek,
	.release = single_release
};

int int_stats_update(unsigned int int_status)
{
	struct END_DEVICE *ei_local = netdev_priv(dev_raether);

	if (ei_local->features & FE_RAETH_INT_DBG) {
		if (int_status & (RX_COHERENT | TX_COHERENT | RXD_ERROR)) {
			if (int_status & RX_COHERENT)
				raeth_int.RX_COHERENT_CNT++;
			if (int_status & TX_COHERENT)
				raeth_int.TX_COHERENT_CNT++;
			if (int_status & RXD_ERROR)
				raeth_int.RXD_ERROR_CNT++;
		}
		if (int_status &
		    (RX_DLY_INT | RING1_RX_DLY_INT | RING2_RX_DLY_INT |
		     RING3_RX_DLY_INT)) {
			if (int_status & RX_DLY_INT)
				raeth_int.RX_DLY_INT_CNT++;
			if (int_status & RING1_RX_DLY_INT)
				raeth_int.RING1_RX_DLY_INT_CNT++;
			if (int_status & RING2_RX_DLY_INT)
				raeth_int.RING2_RX_DLY_INT_CNT++;
			if (int_status & RING3_RX_DLY_INT)
				raeth_int.RING3_RX_DLY_INT_CNT++;
		}
		if (int_status & (TX_DLY_INT))
			raeth_int.TX_DLY_INT_CNT++;
		if (int_status &
		    (RX_DONE_INT0 | RX_DONE_INT1 | RX_DONE_INT2 |
		     RX_DONE_INT3)) {
			if (int_status & RX_DONE_INT0)
				raeth_int.RX_DONE_INT0_CNT++;
			if (int_status & RX_DONE_INT1)
				raeth_int.RX_DONE_INT1_CNT++;
			if (int_status & RX_DONE_INT2)
				raeth_int.RX_DONE_INT2_CNT++;
			if (int_status & RX_DONE_INT3)
				raeth_int.RX_DONE_INT3_CNT++;
		}
		if (int_status &
		    (TX_DONE_INT0 | TX_DONE_INT1 | TX_DONE_INT2 |
		     TX_DONE_INT3)) {
			if (int_status & TX_DONE_INT0)
				raeth_int.TX_DONE_INT0_CNT++;
			if (int_status & TX_DONE_INT1)
				raeth_int.TX_DONE_INT1_CNT++;
			if (int_status & TX_DONE_INT2)
				raeth_int.TX_DONE_INT2_CNT++;
			if (int_status & TX_DONE_INT3)
				raeth_int.TX_DONE_INT3_CNT++;
		}
		if (int_status &
		    (ALT_RPLC_INT1 | ALT_RPLC_INT2 | ALT_RPLC_INT3)) {
			if (int_status & ALT_RPLC_INT1)
				raeth_int.ALT_RPLC_INT1_CNT++;
			if (int_status & ALT_RPLC_INT2)
				raeth_int.ALT_RPLC_INT2_CNT++;
			if (int_status & ALT_RPLC_INT3)
				raeth_int.ALT_RPLC_INT3_CNT++;
		}
	}
	return 0;
}

static int int_dbg_read(struct seq_file *seq, void *v)
{
	struct END_DEVICE *ei_local = netdev_priv(dev_raether);

	if (ei_local->features & FE_RAETH_INT_DBG) {
		seq_puts(seq, "Raether Interrupt Statistics\n");
		seq_printf(seq, "RX_COHERENT = %d\n",
			   raeth_int.RX_COHERENT_CNT);
		seq_printf(seq, "RX_DLY_INT = %d\n", raeth_int.RX_DLY_INT_CNT);
		seq_printf(seq, "TX_COHERENT = %d\n",
			   raeth_int.TX_COHERENT_CNT);
		seq_printf(seq, "TX_DLY_INT = %d\n", raeth_int.TX_DLY_INT_CNT);
		seq_printf(seq, "RING3_RX_DLY_INT = %d\n",
			   raeth_int.RING3_RX_DLY_INT_CNT);
		seq_printf(seq, "RING2_RX_DLY_INT = %d\n",
			   raeth_int.RING2_RX_DLY_INT_CNT);
		seq_printf(seq, "RING1_RX_DLY_INT = %d\n",
			   raeth_int.RING1_RX_DLY_INT_CNT);
		seq_printf(seq, "RXD_ERROR = %d\n", raeth_int.RXD_ERROR_CNT);
		seq_printf(seq, "ALT_RPLC_INT3 = %d\n",
			   raeth_int.ALT_RPLC_INT3_CNT);
		seq_printf(seq, "ALT_RPLC_INT2 = %d\n",
			   raeth_int.ALT_RPLC_INT2_CNT);
		seq_printf(seq, "ALT_RPLC_INT1 = %d\n",
			   raeth_int.ALT_RPLC_INT1_CNT);
		seq_printf(seq, "RX_DONE_INT3 = %d\n",
			   raeth_int.RX_DONE_INT3_CNT);
		seq_printf(seq, "RX_DONE_INT2 = %d\n",
			   raeth_int.RX_DONE_INT2_CNT);
		seq_printf(seq, "RX_DONE_INT1 = %d\n",
			   raeth_int.RX_DONE_INT1_CNT);
		seq_printf(seq, "RX_DONE_INT0 = %d\n",
			   raeth_int.RX_DONE_INT0_CNT);
		seq_printf(seq, "TX_DONE_INT3 = %d\n",
			   raeth_int.TX_DONE_INT3_CNT);
		seq_printf(seq, "TX_DONE_INT2 = %d\n",
			   raeth_int.TX_DONE_INT2_CNT);
		seq_printf(seq, "TX_DONE_INT1 = %d\n",
			   raeth_int.TX_DONE_INT1_CNT);
		seq_printf(seq, "TX_DONE_INT0 = %d\n",
			   raeth_int.TX_DONE_INT0_CNT);

		memset(&raeth_int, 0, sizeof(raeth_int));
	}
	return 0;
}

static int int_dbg_open(struct inode *inode, struct file *file)
{
	struct END_DEVICE *ei_local = netdev_priv(dev_raether);

	if (ei_local->features & FE_RAETH_INT_DBG) {
		/* memset(&raeth_int, 0, sizeof(raeth_int)); */
		return single_open(file, int_dbg_read, NULL);
	} else {
		return 0;
	}
}

static ssize_t int_dbg_write(struct file *file, const char __user *buffer,
			     size_t count, loff_t *data)
{
	return 0;
}

static const struct file_operations int_dbg_sw_fops = {
	.owner = THIS_MODULE,
	.open = int_dbg_open,
	.read = seq_read,
	.write = int_dbg_write
};

static int set_lan_ip_read(struct seq_file *seq, void *v)
{
	struct END_DEVICE *ei_local = netdev_priv(dev_raether);

	seq_printf(seq, "ei_local->lan_ip4_addr = %s\n",
		   ei_local->lan_ip4_addr);

	return 0;
}

static int set_lan_ip_open(struct inode *inode, struct file *file)
{
	return single_open(file, set_lan_ip_read, NULL);
}

static ssize_t set_lan_ip_write(struct file *file,
				const char __user *buffer, size_t count,
				loff_t *data)
{
	struct END_DEVICE *ei_local = netdev_priv(dev_raether);
	char ip_tmp[IP4_ADDR_LEN];

	if (count > IP4_ADDR_LEN)
		return -EFAULT;

	if (copy_from_user(ip_tmp, buffer, count))
		return -EFAULT;

	strncpy(ei_local->lan_ip4_addr, ip_tmp, count);

	pr_info("[%s]LAN IP = %s\n", __func__, ei_local->lan_ip4_addr);

	if (ei_local->features & FE_SW_LRO)
		fe_set_sw_lro_my_ip(ei_local->lan_ip4_addr);

	if (ei_local->features & FE_HW_LRO)
		fe_set_hw_lro_my_ip(ei_local->lan_ip4_addr);

	return count;
}

static const struct file_operations set_lan_ip_fops = {
	.owner = THIS_MODULE,
	.open = set_lan_ip_open,
	.read = seq_read,
	.write = set_lan_ip_write
};

int debug_proc_init(void)
{
	struct END_DEVICE *ei_local = netdev_priv(dev_raether);

	if (!proc_reg_dir)
		proc_reg_dir = proc_mkdir(PROCREG_DIR, NULL);

	if (ei_local->features & FE_HW_LRO)
		hwlro_debug_proc_init(proc_reg_dir);
	else if (ei_local->features & (FE_RSS_4RING | FE_RSS_2RING))
		rss_debug_proc_init(proc_reg_dir);

	if (ei_local->features & FE_HW_IOCOHERENT)
		hwioc_debug_proc_init(proc_reg_dir);
	proc_gmac = proc_create(PROCREG_GMAC, 0, proc_reg_dir, &gmac_fops);
	if (!proc_gmac)
		pr_debug("!! FAIL to create %s PROC !!\n", PROCREG_GMAC);

	if (ei_local->features & (FE_ETHTOOL | FE_GE2_SUPPORT)) {
		proc_gmac2 =
		    proc_create(PROCREG_GMAC2, 0, proc_reg_dir, &gmac2_fops);
		if (!proc_gmac2)
			pr_debug("!! FAIL to create %s PROC !!\n",
				 PROCREG_GMAC2);
	}
	proc_skb_free =
	    proc_create(PROCREG_SKBFREE, 0, proc_reg_dir, &skb_free_fops);
	if (!proc_skb_free)
		pr_debug("!! FAIL to create %s PROC !!\n", PROCREG_SKBFREE);
	proc_tx_ring = proc_create(PROCREG_TXRING, 0, proc_reg_dir,
				   &tx_ring_fops);
	if (!proc_tx_ring)
		pr_debug("!! FAIL to create %s PROC !!\n", PROCREG_TXRING);
	proc_rx_ring = proc_create(PROCREG_RXRING, 0,
				   proc_reg_dir, &rx_ring_fops);
	if (!proc_rx_ring)
		pr_debug("!! FAIL to create %s PROC !!\n", PROCREG_RXRING);

	if (ei_local->features & FE_TSO) {
		proc_num_of_txd =
		    proc_create(PROCREG_NUM_OF_TXD, 0, proc_reg_dir,
				&tso_txd_num_fops);
		if (!proc_num_of_txd)
			pr_debug("!! FAIL to create %s PROC !!\n",
				 PROCREG_NUM_OF_TXD);
		proc_tso_len =
		    proc_create(PROCREG_TSO_LEN, 0, proc_reg_dir,
				&tso_len_fops);
		if (!proc_tso_len)
			pr_debug("!! FAIL to create %s PROC !!\n",
				 PROCREG_TSO_LEN);
	}

	if (ei_local->features & FE_SW_LRO) {
		proc_lro_stats =
		    proc_create(PROCREG_LRO_STATS, 0, proc_reg_dir,
				&lro_stats_fops);
		if (!proc_lro_stats)
			pr_debug("!! FAIL to create %s PROC !!\n",
				 PROCREG_LRO_STATS);
	}

	if (ei_local->features & USER_SNMPD) {
		proc_ra_snmp =
		    proc_create(PROCREG_SNMP, S_IRUGO, proc_reg_dir,
				&ra_snmp_seq_fops);
		if (!proc_ra_snmp)
			pr_debug("!! FAIL to create %s PROC !!\n",
				 PROCREG_SNMP);
	}
	proc_esw_cnt =
	    proc_create(PROCREG_ESW_CNT, 0, proc_reg_dir, &switch_count_fops);
	if (!proc_esw_cnt)
		pr_debug("!! FAIL to create %s PROC !!\n", PROCREG_ESW_CNT);

	if (ei_local->chip_name == MT7622_FE || ei_local->chip_name == LEOPARD_FE) {
		proc_eth_cnt =
			proc_create(PROCREG_ETH_CNT, 0, proc_reg_dir, &eth_count_fops);
		if (!proc_eth_cnt)
			pr_debug("!! FAIL to create %s PROC !!\n", PROCREG_ETH_CNT);
	}

	if (ei_local->features & TASKLET_WORKQUEUE_SW) {
		proc_sche =
		    proc_create(PROCREG_SCHE, 0, proc_reg_dir,
				&schedule_sw_fops);
		if (!proc_sche)
			pr_debug("!! FAIL to create %s PROC !!\n",
				 PROCREG_SCHE);
	}

	if (ei_local->features & FE_RAETH_INT_DBG) {
		proc_int_dbg =
		    proc_create(PROCREG_INT_DBG, 0, proc_reg_dir,
				&int_dbg_sw_fops);
		if (!proc_int_dbg)
			pr_debug("!! FAIL to create %s PROC !!\n",
				 PROCREG_INT_DBG);
	}

	/* Set LAN IP address */
	proc_set_lan_ip =
	    proc_create(PROCREG_SET_LAN_IP, 0, proc_reg_dir, &set_lan_ip_fops);
	if (!proc_set_lan_ip)
		pr_debug("!! FAIL to create %s PROC !!\n", PROCREG_SET_LAN_IP);

	pr_debug("PROC INIT OK!\n");
	return 0;
}

void debug_proc_exit(void)
{
	struct END_DEVICE *ei_local = netdev_priv(dev_raether);

	if (ei_local->features & FE_HW_LRO)
		hwlro_debug_proc_exit(proc_reg_dir);
	else if (ei_local->features & (FE_RSS_4RING | FE_RSS_2RING))
		rss_debug_proc_exit(proc_reg_dir);

	if (ei_local->features & FE_HW_IOCOHERENT)
		hwioc_debug_proc_exit(proc_reg_dir);

	if (proc_sys_cp0)
		remove_proc_entry(PROCREG_CP0, proc_reg_dir);

	if (proc_gmac)
		remove_proc_entry(PROCREG_GMAC, proc_reg_dir);

	if (ei_local->features & (FE_ETHTOOL | FE_GE2_SUPPORT)) {
		if (proc_gmac)
			remove_proc_entry(PROCREG_GMAC, proc_reg_dir);
	}

	if (proc_skb_free)
		remove_proc_entry(PROCREG_SKBFREE, proc_reg_dir);

	if (proc_tx_ring)
		remove_proc_entry(PROCREG_TXRING, proc_reg_dir);

	if (proc_rx_ring)
		remove_proc_entry(PROCREG_RXRING, proc_reg_dir);

	if (ei_local->features & FE_TSO) {
		if (proc_num_of_txd)
			remove_proc_entry(PROCREG_NUM_OF_TXD, proc_reg_dir);

		if (proc_tso_len)
			remove_proc_entry(PROCREG_TSO_LEN, proc_reg_dir);
	}

	if (ei_local->features & FE_SW_LRO) {
		if (proc_lro_stats)
			remove_proc_entry(PROCREG_LRO_STATS, proc_reg_dir);
	}

	if (ei_local->features & USER_SNMPD) {
		if (proc_ra_snmp)
			remove_proc_entry(PROCREG_SNMP, proc_reg_dir);
	}

	if (proc_esw_cnt)
		remove_proc_entry(PROCREG_ESW_CNT, proc_reg_dir);

	if (ei_local->chip_name == MT7622_FE || ei_local->chip_name == LEOPARD_FE) {
		if (proc_eth_cnt)
			remove_proc_entry(PROCREG_ETH_CNT, proc_reg_dir);
	}

	/* if (proc_reg_dir) */
	/* remove_proc_entry(PROCREG_DIR, 0); */

	pr_debug("proc exit\n");
}
EXPORT_SYMBOL(proc_reg_dir);
