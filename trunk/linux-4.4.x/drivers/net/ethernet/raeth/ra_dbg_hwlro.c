/* Copyright  2016 MediaTek Inc.
 * Author: Nelson Chang <nelson.chang@mediatek.com>
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
#include "raether_hwlro.h"
#include "ra_dbg_proc.h"

/* HW LRO proc */
#define HW_LRO_RING_NUM 3
#define MAX_HW_LRO_AGGR 64

typedef int (*HWLRO_DBG_FUNC) (int par1, int par2);
unsigned int hw_lro_agg_num_cnt[HW_LRO_RING_NUM][MAX_HW_LRO_AGGR + 1];
unsigned int hw_lro_agg_size_cnt[HW_LRO_RING_NUM][16];
unsigned int hw_lro_tot_agg_cnt[HW_LRO_RING_NUM];
unsigned int hw_lro_tot_flush_cnt[HW_LRO_RING_NUM];

/* HW LRO flush reason proc */
#define HW_LRO_AGG_FLUSH        (1)
#define HW_LRO_AGE_FLUSH        (2)
#define HW_LRO_NOT_IN_SEQ_FLUSH (3)
#define HW_LRO_TIMESTAMP_FLUSH  (4)
#define HW_LRO_NON_RULE_FLUSH   (5)

unsigned int hw_lro_agg_flush_cnt[HW_LRO_RING_NUM];
unsigned int hw_lro_age_flush_cnt[HW_LRO_RING_NUM];
unsigned int hw_lro_seq_flush_cnt[HW_LRO_RING_NUM];
unsigned int hw_lro_timestamp_flush_cnt[HW_LRO_RING_NUM];
unsigned int hw_lro_norule_flush_cnt[HW_LRO_RING_NUM];

static struct proc_dir_entry *proc_rx_ring1, *proc_rx_ring2, *proc_rx_ring3;
static struct proc_dir_entry *proc_hw_lro_stats, *proc_hw_lro_auto_tlb;

int rx_lro_ring_read(struct seq_file *seq, void *v,
		     struct PDMA_rxdesc *rx_ring_p)
{
	struct PDMA_rxdesc *rx_ring;
	int i = 0;

	rx_ring =
	    kmalloc(sizeof(struct PDMA_rxdesc) * NUM_LRO_RX_DESC, GFP_KERNEL);
	if (!rx_ring) {
		seq_puts(seq, " allocate temp rx_ring fail.\n");
		return 0;
	}

	for (i = 0; i < NUM_LRO_RX_DESC; i++)
		memcpy(&rx_ring[i], &rx_ring_p[i], sizeof(struct PDMA_rxdesc));

	for (i = 0; i < NUM_LRO_RX_DESC; i++) {
		seq_printf(seq, "%d: %08x %08x %08x %08x\n", i,
			   *(int *)&rx_ring[i].rxd_info1,
			   *(int *)&rx_ring[i].rxd_info2,
			   *(int *)&rx_ring[i].rxd_info3,
			   *(int *)&rx_ring[i].rxd_info4);
	}

	kfree(rx_ring);
	return 0;
}

int rx_ring1_read(struct seq_file *seq, void *v)
{
	struct END_DEVICE *ei_local = netdev_priv(dev_raether);

	rx_lro_ring_read(seq, v, ei_local->rx_ring[1]);

	return 0;
}

int rx_ring2_read(struct seq_file *seq, void *v)
{
	struct END_DEVICE *ei_local = netdev_priv(dev_raether);

	rx_lro_ring_read(seq, v, ei_local->rx_ring[2]);

	return 0;
}

int rx_ring3_read(struct seq_file *seq, void *v)
{
	struct END_DEVICE *ei_local = netdev_priv(dev_raether);

	rx_lro_ring_read(seq, v, ei_local->rx_ring[3]);

	return 0;
}

static int rx_ring1_open(struct inode *inode, struct file *file)
{
	return single_open(file, rx_ring1_read, NULL);
}

static int rx_ring2_open(struct inode *inode, struct file *file)
{
	return single_open(file, rx_ring2_read, NULL);
}

static int rx_ring3_open(struct inode *inode, struct file *file)
{
	return single_open(file, rx_ring3_read, NULL);
}

static const struct file_operations rx_ring1_fops = {
	.owner = THIS_MODULE,
	.open = rx_ring1_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release
};

static const struct file_operations rx_ring2_fops = {
	.owner = THIS_MODULE,
	.open = rx_ring2_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release
};

static const struct file_operations rx_ring3_fops = {
	.owner = THIS_MODULE,
	.open = rx_ring3_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release
};

static int hw_lro_len_update(unsigned int agg_size)
{
	int len_idx;

	if (agg_size > 65000)
		len_idx = 13;
	else if (agg_size > 60000)
		len_idx = 12;
	else if (agg_size > 55000)
		len_idx = 11;
	else if (agg_size > 50000)
		len_idx = 10;
	else if (agg_size > 45000)
		len_idx = 9;
	else if (agg_size > 40000)
		len_idx = 8;
	else if (agg_size > 35000)
		len_idx = 7;
	else if (agg_size > 30000)
		len_idx = 6;
	else if (agg_size > 25000)
		len_idx = 5;
	else if (agg_size > 20000)
		len_idx = 4;
	else if (agg_size > 15000)
		len_idx = 3;
	else if (agg_size > 10000)
		len_idx = 2;
	else if (agg_size > 5000)
		len_idx = 1;
	else
		len_idx = 0;

	return len_idx;
}

void hw_lro_stats_update(unsigned int ring_num, struct PDMA_rxdesc *rx_ring)
{
	unsigned int agg_cnt = rx_ring->rxd_info2.LRO_AGG_CNT;
	unsigned int agg_size = (rx_ring->rxd_info2.PLEN1 << 14) |
				 rx_ring->rxd_info2.PLEN0;

	if ((ring_num > 0) && (ring_num < 4)) {
		hw_lro_agg_size_cnt[ring_num - 1]
				   [hw_lro_len_update(agg_size)]++;
		hw_lro_agg_num_cnt[ring_num - 1][agg_cnt]++;
		hw_lro_tot_flush_cnt[ring_num - 1]++;
		hw_lro_tot_agg_cnt[ring_num - 1] += agg_cnt;
	}
}

void hw_lro_flush_stats_update(unsigned int ring_num,
			       struct PDMA_rxdesc *rx_ring)
{
	unsigned int flush_reason = rx_ring->rxd_info2.REV;

	if ((ring_num > 0) && (ring_num < 4)) {
		if ((flush_reason & 0x7) == HW_LRO_AGG_FLUSH)
			hw_lro_agg_flush_cnt[ring_num - 1]++;
		else if ((flush_reason & 0x7) == HW_LRO_AGE_FLUSH)
			hw_lro_age_flush_cnt[ring_num - 1]++;
		else if ((flush_reason & 0x7) == HW_LRO_NOT_IN_SEQ_FLUSH)
			hw_lro_seq_flush_cnt[ring_num - 1]++;
		else if ((flush_reason & 0x7) == HW_LRO_TIMESTAMP_FLUSH)
			hw_lro_timestamp_flush_cnt[ring_num - 1]++;
		else if ((flush_reason & 0x7) == HW_LRO_NON_RULE_FLUSH)
			hw_lro_norule_flush_cnt[ring_num - 1]++;
	}
}
EXPORT_SYMBOL(hw_lro_flush_stats_update);

ssize_t hw_lro_stats_write(struct file *file, const char __user *buffer,
			   size_t count, loff_t *data)
{
	memset(hw_lro_agg_num_cnt, 0, sizeof(hw_lro_agg_num_cnt));
	memset(hw_lro_agg_size_cnt, 0, sizeof(hw_lro_agg_size_cnt));
	memset(hw_lro_tot_agg_cnt, 0, sizeof(hw_lro_tot_agg_cnt));
	memset(hw_lro_tot_flush_cnt, 0, sizeof(hw_lro_tot_flush_cnt));
	memset(hw_lro_agg_flush_cnt, 0, sizeof(hw_lro_agg_flush_cnt));
	memset(hw_lro_age_flush_cnt, 0, sizeof(hw_lro_age_flush_cnt));
	memset(hw_lro_seq_flush_cnt, 0, sizeof(hw_lro_seq_flush_cnt));
	memset(hw_lro_timestamp_flush_cnt, 0,
	       sizeof(hw_lro_timestamp_flush_cnt));
	memset(hw_lro_norule_flush_cnt, 0, sizeof(hw_lro_norule_flush_cnt));

	pr_info("clear hw lro cnt table\n");

	return count;
}

int hw_lro_stats_read(struct seq_file *seq, void *v)
{
	int i;
	struct END_DEVICE *ei_local = netdev_priv(dev_raether);

	seq_puts(seq, "HW LRO statistic dump:\n");

	/* Agg number count */
	seq_puts(seq, "Cnt:   RING1 | RING2 | RING3 | Total\n");
	for (i = 0; i <= MAX_HW_LRO_AGGR; i++) {
		seq_printf(seq, " %d :      %d        %d        %d        %d\n",
			   i, hw_lro_agg_num_cnt[0][i],
			   hw_lro_agg_num_cnt[1][i], hw_lro_agg_num_cnt[2][i],
			   hw_lro_agg_num_cnt[0][i] + hw_lro_agg_num_cnt[1][i] +
			   hw_lro_agg_num_cnt[2][i]);
	}

	/* Total agg count */
	seq_puts(seq, "Total agg:   RING1 | RING2 | RING3 | Total\n");
	seq_printf(seq, "                %d      %d      %d      %d\n",
		   hw_lro_tot_agg_cnt[0], hw_lro_tot_agg_cnt[1],
		   hw_lro_tot_agg_cnt[2],
		   hw_lro_tot_agg_cnt[0] + hw_lro_tot_agg_cnt[1] +
		   hw_lro_tot_agg_cnt[2]);

	/* Total flush count */
	seq_puts(seq, "Total flush:   RING1 | RING2 | RING3 | Total\n");
	seq_printf(seq, "                %d      %d      %d      %d\n",
		   hw_lro_tot_flush_cnt[0], hw_lro_tot_flush_cnt[1],
		   hw_lro_tot_flush_cnt[2],
		   hw_lro_tot_flush_cnt[0] + hw_lro_tot_flush_cnt[1] +
		   hw_lro_tot_flush_cnt[2]);

	/* Avg agg count */
	seq_puts(seq, "Avg agg:   RING1 | RING2 | RING3 | Total\n");
	seq_printf(seq, "                %d      %d      %d      %d\n",
		   (hw_lro_tot_flush_cnt[0]) ? hw_lro_tot_agg_cnt[0] /
		   hw_lro_tot_flush_cnt[0] : 0,
		   (hw_lro_tot_flush_cnt[1]) ? hw_lro_tot_agg_cnt[1] /
		   hw_lro_tot_flush_cnt[1] : 0,
		   (hw_lro_tot_flush_cnt[2]) ? hw_lro_tot_agg_cnt[2] /
		   hw_lro_tot_flush_cnt[2] : 0,
		   (hw_lro_tot_flush_cnt[0] + hw_lro_tot_flush_cnt[1] +
		    hw_lro_tot_flush_cnt[2]) ? ((hw_lro_tot_agg_cnt[0] +
						 hw_lro_tot_agg_cnt[1] +
						 hw_lro_tot_agg_cnt[2]) /
						(hw_lro_tot_flush_cnt[0] +
						 hw_lro_tot_flush_cnt[1] +
						 hw_lro_tot_flush_cnt[2])) : 0);

	/*  Statistics of aggregation size counts */
	seq_puts(seq, "HW LRO flush pkt len:\n");
	seq_puts(seq, " Length  | RING1  | RING2  | RING3  | Total\n");
	for (i = 0; i < 15; i++) {
		seq_printf(seq, "%d~%d: %d      %d      %d      %d\n", i * 5000,
			   (i + 1) * 5000, hw_lro_agg_size_cnt[0][i],
			   hw_lro_agg_size_cnt[1][i], hw_lro_agg_size_cnt[2][i],
			   hw_lro_agg_size_cnt[0][i] +
			   hw_lro_agg_size_cnt[1][i] +
			   hw_lro_agg_size_cnt[2][i]);
	}

	/* CONFIG_RAETH_HW_LRO_REASON_DBG */
	if (ei_local->features & FE_HW_LRO_DBG) {
		seq_puts(seq, "Flush reason:   RING1 | RING2 | RING3 | Total\n");
		seq_printf(seq, "AGG timeout:      %d      %d      %d      %d\n",
			   hw_lro_agg_flush_cnt[0], hw_lro_agg_flush_cnt[1],
			   hw_lro_agg_flush_cnt[2],
			   (hw_lro_agg_flush_cnt[0] + hw_lro_agg_flush_cnt[1] +
			    hw_lro_agg_flush_cnt[2])
		    );
		seq_printf(seq, "AGE timeout:      %d      %d      %d      %d\n",
			   hw_lro_age_flush_cnt[0], hw_lro_age_flush_cnt[1],
			   hw_lro_age_flush_cnt[2],
			   (hw_lro_age_flush_cnt[0] + hw_lro_age_flush_cnt[1] +
			    hw_lro_age_flush_cnt[2])
		    );
		seq_printf(seq, "Not in-sequence:  %d      %d      %d      %d\n",
			   hw_lro_seq_flush_cnt[0], hw_lro_seq_flush_cnt[1],
			   hw_lro_seq_flush_cnt[2],
			   (hw_lro_seq_flush_cnt[0] + hw_lro_seq_flush_cnt[1] +
			    hw_lro_seq_flush_cnt[2])
		    );
		seq_printf(seq, "Timestamp:        %d      %d      %d      %d\n",
			   hw_lro_timestamp_flush_cnt[0],
			   hw_lro_timestamp_flush_cnt[1],
			   hw_lro_timestamp_flush_cnt[2],
			   (hw_lro_timestamp_flush_cnt[0] +
			    hw_lro_timestamp_flush_cnt[1] +
			    hw_lro_timestamp_flush_cnt[2])
		    );
		seq_printf(seq, "No LRO rule:      %d      %d      %d      %d\n",
			   hw_lro_norule_flush_cnt[0],
			   hw_lro_norule_flush_cnt[1],
			   hw_lro_norule_flush_cnt[2],
			   (hw_lro_norule_flush_cnt[0] +
			    hw_lro_norule_flush_cnt[1] +
			    hw_lro_norule_flush_cnt[2])
		    );
	}

	return 0;
}

static int hw_lro_stats_open(struct inode *inode, struct file *file)
{
	return single_open(file, hw_lro_stats_read, NULL);
}

static const struct file_operations hw_lro_stats_fops = {
	.owner = THIS_MODULE,
	.open = hw_lro_stats_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = hw_lro_stats_write,
	.release = single_release
};

int hwlro_agg_cnt_ctrl(int par1, int par2)
{
	SET_PDMA_RXRING_MAX_AGG_CNT(ADMA_RX_RING1, par2);
	SET_PDMA_RXRING_MAX_AGG_CNT(ADMA_RX_RING2, par2);
	SET_PDMA_RXRING_MAX_AGG_CNT(ADMA_RX_RING3, par2);
	return 0;
}

int hwlro_agg_time_ctrl(int par1, int par2)
{
	SET_PDMA_RXRING_AGG_TIME(ADMA_RX_RING1, par2);
	SET_PDMA_RXRING_AGG_TIME(ADMA_RX_RING2, par2);
	SET_PDMA_RXRING_AGG_TIME(ADMA_RX_RING3, par2);
	return 0;
}

int hwlro_age_time_ctrl(int par1, int par2)
{
	SET_PDMA_RXRING_AGE_TIME(ADMA_RX_RING1, par2);
	SET_PDMA_RXRING_AGE_TIME(ADMA_RX_RING2, par2);
	SET_PDMA_RXRING_AGE_TIME(ADMA_RX_RING3, par2);
	return 0;
}

int hwlro_threshold_ctrl(int par1, int par2)
{
	/* bandwidth threshold setting */
	SET_PDMA_LRO_BW_THRESHOLD(par2);
	return 0;
}

int hwlro_ring_enable_ctrl(int par1, int par2)
{
	if (!par2) {
		pr_info("[hwlro_ring_enable_ctrl]Disable HW LRO rings\n");
		SET_PDMA_RXRING_VALID(ADMA_RX_RING0, 0);
		SET_PDMA_RXRING_VALID(ADMA_RX_RING1, 0);
		SET_PDMA_RXRING_VALID(ADMA_RX_RING2, 0);
		SET_PDMA_RXRING_VALID(ADMA_RX_RING3, 0);
	} else {
		pr_info("[hwlro_ring_enable_ctrl]Enable HW LRO rings\n");
		SET_PDMA_RXRING_VALID(ADMA_RX_RING0, 1);
		SET_PDMA_RXRING_VALID(ADMA_RX_RING1, 1);
		SET_PDMA_RXRING_VALID(ADMA_RX_RING2, 1);
		SET_PDMA_RXRING_VALID(ADMA_RX_RING3, 1);
	}

	return 0;
}

static const HWLRO_DBG_FUNC hw_lro_dbg_func[] = {
	[0] = hwlro_agg_cnt_ctrl,
	[1] = hwlro_agg_time_ctrl,
	[2] = hwlro_age_time_ctrl,
	[3] = hwlro_threshold_ctrl,
	[4] = hwlro_ring_enable_ctrl,
};

ssize_t hw_lro_auto_tlb_write(struct file *file, const char __user *buffer,
			      size_t count, loff_t *data)
{
	char buf[32];
	char *p_buf;
	int len = count;
	long x = 0, y = 0;
	char *p_token = NULL;
	char *p_delimiter = " \t";
	int ret;

	pr_info("[hw_lro_auto_tlb_write]write parameter len = %d\n\r",
		(int)len);
	if (len >= sizeof(buf)) {
		pr_info("input handling fail!\n");
		len = sizeof(buf) - 1;
		return -1;
	}

	if (copy_from_user(buf, buffer, len))
		return -EFAULT;

	buf[len] = '\0';
	pr_info("[hw_lro_auto_tlb_write]write parameter data = %s\n\r", buf);

	p_buf = buf;
	p_token = strsep(&p_buf, p_delimiter);
	if (!p_token)
		x = 0;
	else
		ret = kstrtol(p_token, 10, &x);

	p_token = strsep(&p_buf, "\t\n ");
	if (p_token) {
		ret = kstrtol(p_token, 10, &y);
		pr_info("y = %ld\n\r", y);
	}

	if (hw_lro_dbg_func[x] &&
	    (ARRAY_SIZE(hw_lro_dbg_func) > x)) {
		(*hw_lro_dbg_func[x]) (x, y);
	}

	return count;
}

void hw_lro_auto_tlb_dump(struct seq_file *seq, unsigned int index)
{
	int i;
	struct PDMA_LRO_AUTO_TLB_INFO pdma_lro_auto_tlb;
	unsigned int tlb_info[9];
	unsigned int dw_len, cnt, priority;
	unsigned int entry;

	if (index > 4)
		index = index - 1;
	entry = (index * 9) + 1;

	/* read valid entries of the auto-learn table */
	sys_reg_write(PDMA_FE_ALT_CF8, entry);

	/* seq_printf(seq, "\nEntry = %d\n", entry); */
	for (i = 0; i < 9; i++) {
		tlb_info[i] = sys_reg_read(PDMA_FE_ALT_SEQ_CFC);
		/* seq_printf(seq, "tlb_info[%d] = 0x%x\n", i, tlb_info[i]); */
	}
	memcpy(&pdma_lro_auto_tlb, tlb_info,
	       sizeof(struct PDMA_LRO_AUTO_TLB_INFO));

	dw_len = pdma_lro_auto_tlb.auto_tlb_info7.DW_LEN;
	cnt = pdma_lro_auto_tlb.auto_tlb_info6.CNT;

	if (sys_reg_read(ADMA_LRO_CTRL_DW0) & PDMA_LRO_ALT_SCORE_MODE)
		priority = cnt;		/* packet count */
	else
		priority = dw_len;	/* byte count */

	/* dump valid entries of the auto-learn table */
	if (index >= 4)
		seq_printf(seq, "\n===== TABLE Entry: %d (Act) =====\n", index);
	else
		seq_printf(seq, "\n===== TABLE Entry: %d (LRU) =====\n", index);
	if (pdma_lro_auto_tlb.auto_tlb_info8.IPV4) {
		seq_printf(seq, "SIP = 0x%x:0x%x:0x%x:0x%x (IPv4)\n",
			   pdma_lro_auto_tlb.auto_tlb_info4.SIP3,
			   pdma_lro_auto_tlb.auto_tlb_info3.SIP2,
			   pdma_lro_auto_tlb.auto_tlb_info2.SIP1,
			   pdma_lro_auto_tlb.auto_tlb_info1.SIP0);
	} else {
		seq_printf(seq, "SIP = 0x%x:0x%x:0x%x:0x%x (IPv6)\n",
			   pdma_lro_auto_tlb.auto_tlb_info4.SIP3,
			   pdma_lro_auto_tlb.auto_tlb_info3.SIP2,
			   pdma_lro_auto_tlb.auto_tlb_info2.SIP1,
			   pdma_lro_auto_tlb.auto_tlb_info1.SIP0);
	}
	seq_printf(seq, "DIP_ID = %d\n",
		   pdma_lro_auto_tlb.auto_tlb_info8.DIP_ID);
	seq_printf(seq, "TCP SPORT = %d | TCP DPORT = %d\n",
		   pdma_lro_auto_tlb.auto_tlb_info0.STP,
		   pdma_lro_auto_tlb.auto_tlb_info0.DTP);
	seq_printf(seq, "VLAN_VID_VLD = %d\n",
		   pdma_lro_auto_tlb.auto_tlb_info6.VLAN_VID_VLD);
	seq_printf(seq, "VLAN1 = %d | VLAN2 = %d | VLAN3 = %d | VLAN4 =%d\n",
		   (pdma_lro_auto_tlb.auto_tlb_info5.VLAN_VID0 & 0xfff),
		   ((pdma_lro_auto_tlb.auto_tlb_info5.VLAN_VID0 >> 12) & 0xfff),
		   ((pdma_lro_auto_tlb.auto_tlb_info6.VLAN_VID1 << 8) |
		   ((pdma_lro_auto_tlb.auto_tlb_info5.VLAN_VID0 >> 24)
		     & 0xfff)),
		   ((pdma_lro_auto_tlb.auto_tlb_info6.VLAN_VID1 >> 4) & 0xfff));
	seq_printf(seq, "TPUT = %d | FREQ = %d\n", dw_len, cnt);
	seq_printf(seq, "PRIORITY = %d\n", priority);
}

int hw_lro_auto_tlb_read(struct seq_file *seq, void *v)
{
	int i;
	unsigned int reg_val;
	unsigned int reg_op1, reg_op2, reg_op3, reg_op4;
	unsigned int agg_cnt, agg_time, age_time;

	seq_puts(seq, "Usage of /proc/mt76xx/hw_lro_auto_tlb:\n");
	seq_puts(seq, "echo [function] [setting] > /proc/mt76xx/hw_lro_auto_tlb\n");
	seq_puts(seq, "Functions:\n");
	seq_puts(seq, "[0] = hwlro_agg_cnt_ctrl\n");
	seq_puts(seq, "[1] = hwlro_agg_time_ctrl\n");
	seq_puts(seq, "[2] = hwlro_age_time_ctrl\n");
	seq_puts(seq, "[3] = hwlro_threshold_ctrl\n");
	seq_puts(seq, "[4] = hwlro_ring_enable_ctrl\n\n");

	/* Read valid entries of the auto-learn table */
	sys_reg_write(PDMA_FE_ALT_CF8, 0);
	reg_val = sys_reg_read(PDMA_FE_ALT_SEQ_CFC);

	seq_printf(seq,
		   "HW LRO Auto-learn Table: (PDMA_LRO_ALT_CFC_RSEQ_DBG=0x%x)\n",
		   reg_val);

	for (i = 7; i >= 0; i--) {
		if (reg_val & (1 << i))
			hw_lro_auto_tlb_dump(seq, i);
	}

	/* Read the agg_time/age_time/agg_cnt of LRO rings */
	seq_puts(seq, "\nHW LRO Ring Settings\n");
	for (i = 1; i <= 3; i++) {
		reg_op1 = sys_reg_read(LRO_RX_RING0_CTRL_DW1 + (i * 0x40));
		reg_op2 = sys_reg_read(LRO_RX_RING0_CTRL_DW2 + (i * 0x40));
		reg_op3 = sys_reg_read(LRO_RX_RING0_CTRL_DW3 + (i * 0x40));
		reg_op4 = sys_reg_read(ADMA_LRO_CTRL_DW2);
		agg_cnt =
		    ((reg_op3 & 0x03) << PDMA_LRO_AGG_CNT_H_OFFSET) |
		    ((reg_op2 >> PDMA_LRO_RING_AGG_CNT1_OFFSET) & 0x3f);
		agg_time = (reg_op2 >> PDMA_LRO_RING_AGG_OFFSET) & 0xffff;
		age_time =
		    ((reg_op2 & 0x03f) << PDMA_LRO_AGE_H_OFFSET) |
		    ((reg_op1 >> PDMA_LRO_RING_AGE1_OFFSET) & 0x3ff);
		seq_printf(seq,
			   "Ring[%d]: MAX_AGG_CNT=%d, AGG_TIME=%d, AGE_TIME=%d, Threshold=%d\n",
			   i, agg_cnt, agg_time, age_time, reg_op4);
	}
	seq_puts(seq, "\n");

	return 0;
}

static int hw_lro_auto_tlb_open(struct inode *inode, struct file *file)
{
	return single_open(file, hw_lro_auto_tlb_read, NULL);
}

static const struct file_operations hw_lro_auto_tlb_fops = {
	.owner = THIS_MODULE,
	.open = hw_lro_auto_tlb_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = hw_lro_auto_tlb_write,
	.release = single_release
};

int hwlro_debug_proc_init(struct proc_dir_entry *proc_reg_dir)
{
	proc_rx_ring1 =
	     proc_create(PROCREG_RXRING1, 0, proc_reg_dir, &rx_ring1_fops);
	if (!proc_rx_ring1)
		pr_info("!! FAIL to create %s PROC !!\n", PROCREG_RXRING1);

	proc_rx_ring2 =
	     proc_create(PROCREG_RXRING2, 0, proc_reg_dir, &rx_ring2_fops);
	if (!proc_rx_ring2)
		pr_info("!! FAIL to create %s PROC !!\n", PROCREG_RXRING2);

	proc_rx_ring3 =
	     proc_create(PROCREG_RXRING3, 0, proc_reg_dir, &rx_ring3_fops);
	if (!proc_rx_ring3)
		pr_info("!! FAIL to create %s PROC !!\n", PROCREG_RXRING3);

	proc_hw_lro_stats =
	     proc_create(PROCREG_HW_LRO_STATS, 0, proc_reg_dir,
			 &hw_lro_stats_fops);
	if (!proc_hw_lro_stats)
		pr_info("!! FAIL to create %s PROC !!\n", PROCREG_HW_LRO_STATS);

	proc_hw_lro_auto_tlb =
	     proc_create(PROCREG_HW_LRO_AUTO_TLB, 0, proc_reg_dir,
			 &hw_lro_auto_tlb_fops);
	if (!proc_hw_lro_auto_tlb)
		pr_info("!! FAIL to create %s PROC !!\n",
			PROCREG_HW_LRO_AUTO_TLB);

	return 0;
}
EXPORT_SYMBOL(hwlro_debug_proc_init);

void hwlro_debug_proc_exit(struct proc_dir_entry *proc_reg_dir)
{
	if (proc_rx_ring1)
		remove_proc_entry(PROCREG_RXRING1, proc_reg_dir);
	if (proc_rx_ring2)
		remove_proc_entry(PROCREG_RXRING2, proc_reg_dir);
	if (proc_rx_ring3)
		remove_proc_entry(PROCREG_RXRING3, proc_reg_dir);
	if (proc_hw_lro_stats)
		remove_proc_entry(PROCREG_HW_LRO_STATS, proc_reg_dir);
	if (proc_hw_lro_auto_tlb)
		remove_proc_entry(PROCREG_HW_LRO_AUTO_TLB, proc_reg_dir);
}
EXPORT_SYMBOL(hwlro_debug_proc_exit);
