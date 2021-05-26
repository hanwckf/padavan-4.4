/* Copyright  2016 MediaTek Inc.
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
#include "ra_ioctl.h"
#include "raether_qdma.h"

/* skb->mark to queue mapping table */
struct QDMA_txdesc *free_head;

/* ioctl */
unsigned int M2Q_table[64] = { 0 };
EXPORT_SYMBOL(M2Q_table);
unsigned int lan_wan_separate;
EXPORT_SYMBOL(lan_wan_separate);
struct sk_buff *magic_id = (struct sk_buff *)0xFFFFFFFF;

/* CONFIG_HW_SFQ */
unsigned int web_sfq_enable;
#define HW_SFQ_UP 3
#define HW_SFQ_DL 1

#define sfq_debug 0
struct SFQ_table *sfq0;
struct SFQ_table *sfq1;
struct SFQ_table *sfq2;
struct SFQ_table *sfq3;

#define KSEG1                   0xa0000000
#define PHYS_TO_VIRT(x)         phys_to_virt(x)
#define VIRT_TO_PHYS(x)         virt_to_phys(x)
/* extern void set_fe_dma_glo_cfg(void); */
struct parse_result sfq_parse_result;

/**
 *
 * @brief: get the TXD index from its address
 *
 * @param: cpu_ptr
 *
 * @return: TXD index
*/

/**
 * @brief cal txd number for a page
 *
 * @parm size
 *
 * @return frag_txd_num
 */

static inline unsigned int cal_frag_txd_num(unsigned int size)
{
	unsigned int frag_txd_num = 0;

	if (size == 0)
		return 0;
	while (size > 0) {
		if (size > MAX_QTXD_LEN) {
			frag_txd_num++;
			size -= MAX_QTXD_LEN;
		} else {
			frag_txd_num++;
			size = 0;
		}
	}
	return frag_txd_num;
}

/**
 * @brief get free TXD from TXD queue
 *
 * @param free_txd
 *
 * @return
 */
static inline int get_free_txd(struct END_DEVICE *ei_local, int ring_no)
{
	unsigned int tmp_idx;

	tmp_idx = ei_local->free_txd_head[ring_no];
	ei_local->free_txd_head[ring_no] = ei_local->txd_pool_info[tmp_idx];
	atomic_sub(1, &ei_local->free_txd_num[ring_no]);
	return tmp_idx;
}

static inline unsigned int get_phy_addr(struct END_DEVICE *ei_local,
					unsigned int idx)
{
	return ei_local->phy_txd_pool + (idx * QTXD_LEN);
}

/**
 * @brief add free TXD into TXD queue
 *
 * @param free_txd
 *
 * @return
 */
static inline void put_free_txd(struct END_DEVICE *ei_local, int free_txd_idx)
{
	ei_local->txd_pool_info[ei_local->free_txd_tail[0]] = free_txd_idx;
	ei_local->free_txd_tail[0] = free_txd_idx;
}

void init_pseudo_link_list(struct END_DEVICE *ei_local)
{
	int i;

	for (i = 0; i < gmac1_txq_num; i++) {
		atomic_set(&ei_local->free_txd_num[i], gmac1_txq_txd_num);
		ei_local->free_txd_head[i] = gmac1_txq_txd_num * i;
		ei_local->free_txd_tail[i] = gmac1_txq_txd_num * (i + 1) - 1;
	}
	for (i = 0; i < gmac2_txq_num; i++) {
		atomic_set(&ei_local->free_txd_num[i + gmac1_txq_num],
			   gmac2_txq_txd_num);
		ei_local->free_txd_head[i + gmac1_txq_num] =
		    gmac1_txd_num + gmac2_txq_txd_num * i;
		ei_local->free_txd_tail[i + gmac1_txq_num] =
		    gmac1_txd_num + gmac2_txq_txd_num * (i + 1) - 1;
	}
}

static inline int ring_no_mapping(int txd_idx)
{
	int i;

	if (txd_idx < gmac1_txd_num) {
		for (i = 0; i < gmac1_txq_num; i++) {
			if (txd_idx < (gmac1_txq_txd_num * (i + 1)))
				return i;
		}
	}

	txd_idx -= gmac1_txd_num;
	for (i = 0; i < gmac2_txq_num; i++) {
		if (txd_idx < (gmac2_txq_txd_num * (i + 1)))
			return (i + gmac1_txq_num);
	}
	pr_err("txd index out of range\n");
	return 0;
}

/*define qdma initial alloc*/
/**
 * @brief
 *
 * @param net_dev
 *
 * @return  0: fail
 *	    1: success
 */
bool qdma_tx_desc_alloc(void)
{
	struct net_device *dev = dev_raether;
	struct END_DEVICE *ei_local = netdev_priv(dev);
	unsigned int txd_idx;
	int i = 0;

	ei_local->txd_pool =
	    dma_alloc_coherent(&ei_local->qdma_pdev->dev,
			       QTXD_LEN * num_tx_desc,
			       &ei_local->phy_txd_pool, GFP_KERNEL);
	pr_err("txd_pool=%p phy_txd_pool=%p\n", ei_local->txd_pool,
	       (void *)ei_local->phy_txd_pool);

	if (!ei_local->txd_pool) {
		pr_err("adapter->txd_pool allocation failed!\n");
		return 0;
	}
	pr_err("ei_local->skb_free start address is 0x%p.\n",
	       ei_local->skb_free);
	/* set all txd_pool_info to 0. */
	for (i = 0; i < num_tx_desc; i++) {
		ei_local->skb_free[i] = 0;
		ei_local->txd_pool_info[i] = i + 1;
		ei_local->txd_pool[i].txd_info3.LS_bit = 1;
		ei_local->txd_pool[i].txd_info3.OWN_bit = 1;
	}

	init_pseudo_link_list(ei_local);

	/* get free txd from txd pool */
	txd_idx = get_free_txd(ei_local, 0);
	ei_local->tx_cpu_idx = txd_idx;
	/* add null TXD for transmit */
	sys_reg_write(QTX_CTX_PTR, get_phy_addr(ei_local, txd_idx));
	sys_reg_write(QTX_DTX_PTR, get_phy_addr(ei_local, txd_idx));

	/* get free txd from txd pool */
	txd_idx = get_free_txd(ei_local, 0);
	ei_local->rls_cpu_idx = txd_idx;
	/* add null TXD for release */
	sys_reg_write(QTX_CRX_PTR, get_phy_addr(ei_local, txd_idx));
	sys_reg_write(QTX_DRX_PTR, get_phy_addr(ei_local, txd_idx));

	/*Reserve 4 TXD for each physical queue */
	if (ei_local->chip_name == MT7623_FE || ei_local->chip_name == MT7621_FE ||
	    ei_local->chip_name == LEOPARD_FE) {
		for (i = 0; i < NUM_PQ; i++)
			sys_reg_write(QTX_CFG_0 + QUEUE_OFFSET * i,
				      (NUM_PQ_RESV | (NUM_PQ_RESV << 8)));
	}

	sys_reg_write(QTX_SCH_1, 0x80000000);
	if (ei_local->chip_name == MT7622_FE) {
		for (i = 0; i < NUM_PQ; i++) {
			if (i <= 15) {
				sys_reg_write(QDMA_PAGE, 0);
				sys_reg_write(QTX_CFG_0 + QUEUE_OFFSET * i,
					      (NUM_PQ_RESV |
					       (NUM_PQ_RESV << 8)));
			} else if (i > 15 && i <= 31) {
				sys_reg_write(QDMA_PAGE, 1);
				sys_reg_write(QTX_CFG_0 +
					      QUEUE_OFFSET * (i - 16),
					      (NUM_PQ_RESV |
					       (NUM_PQ_RESV << 8)));
			} else if (i > 31 && i <= 47) {
				sys_reg_write(QDMA_PAGE, 2);
				sys_reg_write(QTX_CFG_0 +
					      QUEUE_OFFSET * (i - 32),
					      (NUM_PQ_RESV |
					       (NUM_PQ_RESV << 8)));
			} else if (i > 47 && i <= 63) {
				sys_reg_write(QDMA_PAGE, 3);
				sys_reg_write(QTX_CFG_0 +
					      QUEUE_OFFSET * (i - 48),
					      (NUM_PQ_RESV |
					       (NUM_PQ_RESV << 8)));
			}
		}
		sys_reg_write(QDMA_PAGE, 0);
	}

	return 1;
}

bool sfq_init(struct net_device *dev)
{
	struct END_DEVICE *ei_local = netdev_priv(dev_raether);
	unsigned int reg_val;
	dma_addr_t sfq_phy0;
	dma_addr_t sfq_phy1;
	dma_addr_t sfq_phy2;
	dma_addr_t sfq_phy3;
	struct SFQ_table *sfq0 = NULL;
	struct SFQ_table *sfq1 = NULL;
	struct SFQ_table *sfq2 = NULL;
	struct SFQ_table *sfq3 = NULL;

	dma_addr_t sfq_phy4;
	dma_addr_t sfq_phy5;
	dma_addr_t sfq_phy6;
	dma_addr_t sfq_phy7;
	struct SFQ_table *sfq4 = NULL;
	struct SFQ_table *sfq5 = NULL;
	struct SFQ_table *sfq6 = NULL;
	struct SFQ_table *sfq7 = NULL;

	int i = 0;

	reg_val = sys_reg_read(VQTX_GLO);
	reg_val = reg_val | VQTX_MIB_EN;
	/* Virtual table extends to 32bytes */
	sys_reg_write(VQTX_GLO, reg_val);
	reg_val = sys_reg_read(VQTX_GLO);
	if (ei_local->chip_name == MT7622_FE || ei_local->chip_name == LEOPARD_FE) {
		sys_reg_write(VQTX_NUM,
			      (VQTX_NUM_0) | (VQTX_NUM_1) | (VQTX_NUM_2) |
			      (VQTX_NUM_3) | (VQTX_NUM_4) | (VQTX_NUM_5) |
			      (VQTX_NUM_6) | (VQTX_NUM_7));
	} else {
		sys_reg_write(VQTX_NUM,
			      (VQTX_NUM_0) | (VQTX_NUM_1) | (VQTX_NUM_2) |
			      (VQTX_NUM_3));
	}

	/* 10 s change hash algorithm */
	sys_reg_write(VQTX_HASH_CFG, 0xF002710);

	if (ei_local->chip_name == MT7622_FE || ei_local->chip_name == LEOPARD_FE)
		sys_reg_write(VQTX_VLD_CFG, 0xeca86420);
	else
		sys_reg_write(VQTX_VLD_CFG, 0xc840);
	sys_reg_write(VQTX_HASH_SD, 0x0D);
	sys_reg_write(QDMA_FC_THRES, 0x9b9b4444);
	sys_reg_write(QDMA_HRED1, 0);
	sys_reg_write(QDMA_HRED2, 0);
	sys_reg_write(QDMA_SRED1, 0);
	sys_reg_write(QDMA_SRED2, 0);
	if (ei_local->chip_name == MT7622_FE || ei_local->chip_name == LEOPARD_FE) {
		sys_reg_write(VQTX_0_3_BIND_QID,
			      (VQTX_0_BIND_QID) | (VQTX_1_BIND_QID) |
			      (VQTX_2_BIND_QID) | (VQTX_3_BIND_QID));
		sys_reg_write(VQTX_4_7_BIND_QID,
			      (VQTX_4_BIND_QID) | (VQTX_5_BIND_QID) |
			      (VQTX_6_BIND_QID) | (VQTX_7_BIND_QID));
		pr_err("VQTX_0_3_BIND_QID =%x\n",
		       sys_reg_read(VQTX_0_3_BIND_QID));
		pr_err("VQTX_4_7_BIND_QID =%x\n",
		       sys_reg_read(VQTX_4_7_BIND_QID));
	}

	sfq0 = dma_alloc_coherent(&ei_local->qdma_pdev->dev,
				  VQ_NUM0 * sizeof(struct SFQ_table), &sfq_phy0,
				  GFP_KERNEL);

	memset(sfq0, 0x0, VQ_NUM0 * sizeof(struct SFQ_table));
	for (i = 0; i < VQ_NUM0; i++) {
		sfq0[i].sfq_info1.VQHPTR = 0xdeadbeef;
		sfq0[i].sfq_info2.VQTPTR = 0xdeadbeef;
	}
	sfq1 = dma_alloc_coherent(&ei_local->qdma_pdev->dev,
				  VQ_NUM1 * sizeof(struct SFQ_table), &sfq_phy1,
				  GFP_KERNEL);
	memset(sfq1, 0x0, VQ_NUM1 * sizeof(struct SFQ_table));
	for (i = 0; i < VQ_NUM1; i++) {
		sfq1[i].sfq_info1.VQHPTR = 0xdeadbeef;
		sfq1[i].sfq_info2.VQTPTR = 0xdeadbeef;
	}

	sfq2 = dma_alloc_coherent(&ei_local->qdma_pdev->dev,
				  VQ_NUM2 * sizeof(struct SFQ_table), &sfq_phy2,
				  GFP_KERNEL);
	memset(sfq2, 0x0, VQ_NUM2 * sizeof(struct SFQ_table));
	for (i = 0; i < VQ_NUM2; i++) {
		sfq2[i].sfq_info1.VQHPTR = 0xdeadbeef;
		sfq2[i].sfq_info2.VQTPTR = 0xdeadbeef;
	}

	sfq3 = dma_alloc_coherent(&ei_local->qdma_pdev->dev,
				  VQ_NUM3 * sizeof(struct SFQ_table), &sfq_phy3,
				  GFP_KERNEL);
	memset(sfq3, 0x0, VQ_NUM3 * sizeof(struct SFQ_table));
	for (i = 0; i < VQ_NUM3; i++) {
		sfq3[i].sfq_info1.VQHPTR = 0xdeadbeef;
		sfq3[i].sfq_info2.VQTPTR = 0xdeadbeef;
	}
	if (unlikely((!sfq0)) || unlikely((!sfq1)) ||
	    unlikely((!sfq2)) || unlikely((!sfq3))) {
		pr_err("QDMA SFQ0~3 VQ not available...\n");
		return 1;
	}
	if (ei_local->chip_name == MT7622_FE || ei_local->chip_name == LEOPARD_FE) {
		sfq4 =
		    dma_alloc_coherent(&ei_local->qdma_pdev->dev,
				       VQ_NUM4 * sizeof(struct SFQ_table),
				       &sfq_phy4, GFP_KERNEL);
		memset(sfq4, 0x0, VQ_NUM4 * sizeof(struct SFQ_table));
		for (i = 0; i < VQ_NUM4; i++) {
			sfq4[i].sfq_info1.VQHPTR = 0xdeadbeef;
			sfq4[i].sfq_info2.VQTPTR = 0xdeadbeef;
		}
		sfq5 =
		    dma_alloc_coherent(&ei_local->qdma_pdev->dev,
				       VQ_NUM5 * sizeof(struct SFQ_table),
				       &sfq_phy5, GFP_KERNEL);
		memset(sfq5, 0x0, VQ_NUM5 * sizeof(struct SFQ_table));
		for (i = 0; i < VQ_NUM5; i++) {
			sfq5[i].sfq_info1.VQHPTR = 0xdeadbeef;
			sfq5[i].sfq_info2.VQTPTR = 0xdeadbeef;
		}
		sfq6 =
		    dma_alloc_coherent(&ei_local->qdma_pdev->dev,
				       VQ_NUM6 * sizeof(struct SFQ_table),
				       &sfq_phy6, GFP_KERNEL);
		memset(sfq6, 0x0, VQ_NUM6 * sizeof(struct SFQ_table));
		for (i = 0; i < VQ_NUM6; i++) {
			sfq6[i].sfq_info1.VQHPTR = 0xdeadbeef;
			sfq6[i].sfq_info2.VQTPTR = 0xdeadbeef;
		}
		sfq7 =
		    dma_alloc_coherent(&ei_local->qdma_pdev->dev,
				       VQ_NUM7 * sizeof(struct SFQ_table),
				       &sfq_phy7, GFP_KERNEL);
		memset(sfq7, 0x0, VQ_NUM7 * sizeof(struct SFQ_table));
		for (i = 0; i < VQ_NUM7; i++) {
			sfq7[i].sfq_info1.VQHPTR = 0xdeadbeef;
			sfq7[i].sfq_info2.VQTPTR = 0xdeadbeef;
		}
		if (unlikely((!sfq4)) || unlikely((!sfq5)) ||
		    unlikely((!sfq6)) || unlikely((!sfq7))) {
			pr_err("QDMA SFQ4~7 VQ not available...\n");
			return 1;
		}
	}

	pr_err("*****sfq_phy0 is 0x%p!!!*******\n", (void *)sfq_phy0);
	pr_err("*****sfq_phy1 is 0x%p!!!*******\n", (void *)sfq_phy1);
	pr_err("*****sfq_phy2 is 0x%p!!!*******\n", (void *)sfq_phy2);
	pr_err("*****sfq_phy3 is 0x%p!!!*******\n", (void *)sfq_phy3);
	pr_err("*****sfq_virt0 is 0x%p!!!*******\n", sfq0);
	pr_err("*****sfq_virt1 is 0x%p!!!*******\n", sfq1);
	pr_err("*****sfq_virt2 is 0x%p!!!*******\n", sfq2);
	pr_err("*****sfq_virt3 is 0x%p!!!*******\n", sfq3);
	if (ei_local->chip_name == MT7622_FE || ei_local->chip_name == LEOPARD_FE) {
		pr_err("*****sfq_phy4 is 0x%p!!!*******\n", (void *)sfq_phy4);
		pr_err("*****sfq_phy5 is 0x%p!!!*******\n", (void *)sfq_phy5);
		pr_err("*****sfq_phy6 is 0x%p!!!*******\n", (void *)sfq_phy6);
		pr_err("*****sfq_phy7 is 0x%p!!!*******\n", (void *)sfq_phy7);
		pr_err("*****sfq_virt4 is 0x%p!!!*******\n", sfq4);
		pr_err("*****sfq_virt5 is 0x%p!!!*******\n", sfq5);
		pr_err("*****sfq_virt6 is 0x%p!!!*******\n", sfq6);
		pr_err("*****sfq_virt7 is 0x%p!!!*******\n", sfq7);
	}

	sys_reg_write(VQTX_TB_BASE0, (u32)sfq_phy0);
	sys_reg_write(VQTX_TB_BASE1, (u32)sfq_phy1);
	sys_reg_write(VQTX_TB_BASE2, (u32)sfq_phy2);
	sys_reg_write(VQTX_TB_BASE3, (u32)sfq_phy3);
	if (ei_local->chip_name == MT7622_FE || ei_local->chip_name == LEOPARD_FE) {
		sys_reg_write(VQTX_TB_BASE4, (u32)sfq_phy4);
		sys_reg_write(VQTX_TB_BASE5, (u32)sfq_phy5);
		sys_reg_write(VQTX_TB_BASE6, (u32)sfq_phy6);
		sys_reg_write(VQTX_TB_BASE7, (u32)sfq_phy7);
	}

	return 0;
}

bool fq_qdma_init(struct net_device *dev)
{
	struct END_DEVICE *ei_local = netdev_priv(dev);
	/* struct QDMA_txdesc *free_head = NULL; */
	dma_addr_t phy_free_head;
	dma_addr_t phy_free_tail;
	unsigned int *free_page_head = NULL;
	dma_addr_t phy_free_page_head;
	int i;

	free_head = dma_alloc_coherent(&ei_local->qdma_pdev->dev,
				       NUM_QDMA_PAGE *
				       QTXD_LEN, &phy_free_head, GFP_KERNEL);

	if (unlikely(!free_head)) {
		pr_err("QDMA FQ decriptor not available...\n");
		return 0;
	}
	memset(free_head, 0x0, QTXD_LEN * NUM_QDMA_PAGE);

	free_page_head =
	    dma_alloc_coherent(&ei_local->qdma_pdev->dev,
			       NUM_QDMA_PAGE * QDMA_PAGE_SIZE,
			       &phy_free_page_head, GFP_KERNEL);

	if (unlikely(!free_page_head)) {
		pr_err("QDMA FQ page not available...\n");
		return 0;
	}
	for (i = 0; i < NUM_QDMA_PAGE; i++) {
		free_head[i].txd_info1.SDP =
		    (phy_free_page_head + (i * QDMA_PAGE_SIZE));
		if (i < (NUM_QDMA_PAGE - 1)) {
			free_head[i].txd_info2.NDP =
			    (phy_free_head + ((i + 1) * QTXD_LEN));
		}
		free_head[i].txd_info3.SDL = QDMA_PAGE_SIZE;
	}
	phy_free_tail =
	    (phy_free_head + (u32)((NUM_QDMA_PAGE - 1) * QTXD_LEN));

	pr_err("phy_free_head is 0x%p!!!\n", (void *)phy_free_head);
	pr_err("phy_free_tail_phy is 0x%p!!!\n", (void *)phy_free_tail);
	sys_reg_write(QDMA_FQ_HEAD, (u32)phy_free_head);
	sys_reg_write(QDMA_FQ_TAIL, (u32)phy_free_tail);
	sys_reg_write(QDMA_FQ_CNT, ((num_tx_desc << 16) | NUM_QDMA_PAGE));
	sys_reg_write(QDMA_FQ_BLEN, QDMA_PAGE_SIZE << 16);
	pr_info("gmac1_txd_num:%d; gmac2_txd_num:%d; num_tx_desc:%d\n",
		gmac1_txd_num, gmac2_txd_num, num_tx_desc);
	ei_local->free_head = free_head;
	ei_local->phy_free_head = phy_free_head;
	ei_local->free_page_head = free_page_head;
	ei_local->phy_free_page_head = phy_free_page_head;
	ei_local->tx_ring_full = 0;
	return 1;
}

int sfq_prot;

#if (sfq_debug)
int udp_source_port;
int tcp_source_port;
int ack_packt;
#endif
int sfq_parse_layer_info(struct sk_buff *skb)
{
	struct vlan_hdr *vh_sfq = NULL;
	struct ethhdr *eth_sfq = NULL;
	struct iphdr *iph_sfq = NULL;
	struct ipv6hdr *ip6h_sfq = NULL;
	struct tcphdr *th_sfq = NULL;
	struct udphdr *uh_sfq = NULL;

	memset(&sfq_parse_result, 0, sizeof(sfq_parse_result));
	eth_sfq = (struct ethhdr *)skb->data;
	ether_addr_copy(sfq_parse_result.dmac, eth_sfq->h_dest);
	ether_addr_copy(sfq_parse_result.smac, eth_sfq->h_source);
	/* memcpy(sfq_parse_result.dmac, eth_sfq->h_dest, ETH_ALEN); */
	/* memcpy(sfq_parse_result.smac, eth_sfq->h_source, ETH_ALEN); */
	sfq_parse_result.eth_type = eth_sfq->h_proto;

	if (sfq_parse_result.eth_type == htons(ETH_P_8021Q)) {
		sfq_parse_result.vlan1_gap = VLAN_HLEN;
		vh_sfq = (struct vlan_hdr *)(skb->data + ETH_HLEN);
		sfq_parse_result.eth_type = vh_sfq->h_vlan_encapsulated_proto;
	} else {
		sfq_parse_result.vlan1_gap = 0;
	}

	/* set layer4 start addr */
	if ((sfq_parse_result.eth_type == htons(ETH_P_IP)) ||
	    (sfq_parse_result.eth_type == htons(ETH_P_PPP_SES) &&
	     sfq_parse_result.ppp_tag == htons(PPP_IP))) {
		iph_sfq =
		    (struct iphdr *)(skb->data + ETH_HLEN +
				     (sfq_parse_result.vlan1_gap));

		/* prepare layer3/layer4 info */
		memcpy(&sfq_parse_result.iph, iph_sfq, sizeof(struct iphdr));
		if (iph_sfq->protocol == IPPROTO_TCP) {
			th_sfq =
			    (struct tcphdr *)(skb->data + ETH_HLEN +
					      (sfq_parse_result.vlan1_gap) +
					      (iph_sfq->ihl * 4));
			memcpy(&sfq_parse_result.th, th_sfq,
			       sizeof(struct tcphdr));
#if (sfq_debug)
			tcp_source_port = ntohs(sfq_parse_result.th.source);
			udp_source_port = 0;
			/* tcp ack packet */
			if (ntohl(sfq_parse_result.iph.saddr) == 0xa0a0a04)
				ack_packt = 1;
			else
				ack_packt = 0;
#endif
			sfq_prot = 2;	/* IPV4_HNAPT */
			if (iph_sfq->frag_off & htons(IP_MF | IP_OFFSET))
				return 1;
		} else if (iph_sfq->protocol == IPPROTO_UDP) {
			uh_sfq =
			    (struct udphdr *)(skb->data + ETH_HLEN +
					      (sfq_parse_result.vlan1_gap) +
					      iph_sfq->ihl * 4);
			memcpy(&sfq_parse_result.uh, uh_sfq,
			       sizeof(struct udphdr));
#if (sfq_debug)
			udp_source_port = ntohs(sfq_parse_result.uh.source);
			tcp_source_port = 0;
			ack_packt = 0;
#endif
			sfq_prot = 2;	/* IPV4_HNAPT */
			if (iph_sfq->frag_off & htons(IP_MF | IP_OFFSET))
				return 1;
		} else {
			sfq_prot = 1;
		}
	} else if (sfq_parse_result.eth_type == htons(ETH_P_IPV6) ||
		   (sfq_parse_result.eth_type == htons(ETH_P_PPP_SES) &&
		    sfq_parse_result.ppp_tag == htons(PPP_IPV6))) {
		ip6h_sfq =
		    (struct ipv6hdr *)(skb->data + ETH_HLEN +
				       (sfq_parse_result.vlan1_gap));
		if (ip6h_sfq->nexthdr == NEXTHDR_TCP) {
			sfq_prot = 4;	/* IPV6_5T */
#if (sfq_debug)
			if (ntohl(sfq_parse_result.ip6h.saddr.s6_addr32[3]) ==
			    8)
				ack_packt = 1;
			else
				ack_packt = 0;
#endif
		} else if (ip6h_sfq->nexthdr == NEXTHDR_UDP) {
#if (sfq_debug)
			ack_packt = 0;
#endif
			sfq_prot = 4;	/* IPV6_5T */

		} else {
			sfq_prot = 3;	/* IPV6_3T */
		}
	}
	return 0;
}

int rt2880_qdma_eth_send(struct END_DEVICE *ei_local, struct net_device *dev,
			 struct sk_buff *skb, int gmac_no, int ring_no)
{
	unsigned int length = skb->len;
	struct QDMA_txdesc *cpu_ptr, *prev_cpu_ptr;
	struct QDMA_txdesc dummy_desc;
	struct PSEUDO_ADAPTER *p_ad;
	unsigned long flags;
	unsigned int next_txd_idx, qidx;

	cpu_ptr = &dummy_desc;
	/* 2. prepare data */
	dma_sync_single_for_device(&ei_local->qdma_pdev->dev,
				   virt_to_phys(skb->data),
				   skb->len, DMA_TO_DEVICE);
	/* cpu_ptr->txd_info1.SDP = VIRT_TO_PHYS(skb->data); */
	cpu_ptr->txd_info1.SDP = virt_to_phys(skb->data);
	cpu_ptr->txd_info3.SDL = skb->len;
	if (ei_local->chip_name == MT7622_FE || ei_local->chip_name == LEOPARD_FE)
		cpu_ptr->txd_info4.SDL = ((skb->len) >> 14);
	if (ei_local->features & FE_HW_SFQ) {
		sfq_parse_layer_info(skb);
		cpu_ptr->txd_info4.VQID0 = 1;	/* 1:HW hash 0:CPU */
		cpu_ptr->txd_info3.PROT = sfq_prot;
		/* no vlan */
		cpu_ptr->txd_info3.IPOFST = 14 + (sfq_parse_result.vlan1_gap);
	}
	cpu_ptr->txd_info4.FPORT = gmac_no;

	if (ei_local->features & FE_CSUM_OFFLOAD) {
		if (skb->ip_summed == CHECKSUM_PARTIAL)
			cpu_ptr->txd_info4.TUI_CO = 7;
		else
			cpu_ptr->txd_info4.TUI_CO = 0;
	}

	if (ei_local->features & FE_HW_VLAN_TX) {
		if (skb_vlan_tag_present(skb)) {
			cpu_ptr->txd_info4.VLAN_TAG =
			    0x10000 | skb_vlan_tag_get(skb);
			    cpu_ptr->txd_info3.QID = skb_vlan_tag_get(skb);
		} else {
			cpu_ptr->txd_info3.QID = ring_no;
			cpu_ptr->txd_info4.VLAN_TAG = 0;
		}
	}
	cpu_ptr->txd_info4.QID = 0;
	/* cpu_ptr->txd_info3.QID = ring_no; */

	if ((ei_local->features & QDMA_QOS_MARK) && (skb->mark != 0)) {
		if (skb->mark < 64) {
			qidx = M2Q_table[skb->mark];
			cpu_ptr->txd_info4.QID = ((qidx & 0x30) >> 4);
			cpu_ptr->txd_info3.QID = (qidx & 0x0f);
		} else {
			pr_debug("skb->mark out of range\n");
			cpu_ptr->txd_info3.QID = 0;
			cpu_ptr->txd_info4.QID = 0;
		}
	}
	/* QoS Web UI used */
	if ((ei_local->features & QDMA_QOS_WEB) && (lan_wan_separate == 1)) {
		if (web_sfq_enable == 1 && (skb->mark == 2)) {
			if (gmac_no == 1)
				cpu_ptr->txd_info3.QID = HW_SFQ_DL;
			else
				cpu_ptr->txd_info3.QID = HW_SFQ_UP;
		} else if (gmac_no == 2) {
			cpu_ptr->txd_info3.QID += 8;
		}
	}
#if defined(CONFIG_RA_HW_NAT) || defined(CONFIG_RA_HW_NAT_MODULE)
	if (IS_MAGIC_TAG_PROTECT_VALID_HEAD(skb)) {
		if (FOE_MAGIC_TAG_HEAD(skb) == FOE_MAGIC_PPE) {
			if (ra_sw_nat_hook_rx) {
				cpu_ptr->txd_info4.FPORT = 4;	/* PPE */
				FOE_MAGIC_TAG(skb) = 0;
			}
		}
	} else if (IS_MAGIC_TAG_PROTECT_VALID_TAIL(skb)) {
		if (FOE_MAGIC_TAG_TAIL(skb) == FOE_MAGIC_PPE) {
			if (ra_sw_nat_hook_rx) {
				cpu_ptr->txd_info4.FPORT = 4;	/* PPE */
				FOE_MAGIC_TAG(skb) = 0;
			}
		}
	}
#endif

	/* dma_sync_single_for_device(NULL, virt_to_phys(skb->data), */
	/* skb->len, DMA_TO_DEVICE); */
	cpu_ptr->txd_info3.SWC_bit = 1;

	/* 5. move CPU_PTR to new TXD */
	cpu_ptr->txd_info4.TSO = 0;
	cpu_ptr->txd_info3.LS_bit = 1;
	cpu_ptr->txd_info3.OWN_bit = 0;
	next_txd_idx = get_free_txd(ei_local, ring_no);
	cpu_ptr->txd_info2.NDP = get_phy_addr(ei_local, next_txd_idx);
	spin_lock_irqsave(&ei_local->page_lock, flags);
	prev_cpu_ptr = ei_local->txd_pool + ei_local->tx_cpu_idx;
	/* update skb_free */
	ei_local->skb_free[ei_local->tx_cpu_idx] = skb;
	/* update tx cpu idx */
	ei_local->tx_cpu_idx = next_txd_idx;
	/* update txd info */
	prev_cpu_ptr->txd_info1 = dummy_desc.txd_info1;
	prev_cpu_ptr->txd_info2 = dummy_desc.txd_info2;
	prev_cpu_ptr->txd_info4 = dummy_desc.txd_info4;
	prev_cpu_ptr->txd_info3 = dummy_desc.txd_info3;
	/* NOTE: add memory barrier to avoid
	 * DMA access memory earlier than memory written
	 */
	wmb();
	/* update CPU pointer */
	sys_reg_write(QTX_CTX_PTR,
		      get_phy_addr(ei_local, ei_local->tx_cpu_idx));
	spin_unlock_irqrestore(&ei_local->page_lock, flags);

	if (ei_local->features & FE_GE2_SUPPORT) {
		if (gmac_no == 2) {
			if (ei_local->pseudo_dev) {
				p_ad = netdev_priv(ei_local->pseudo_dev);
				p_ad->stat.tx_packets++;

				p_ad->stat.tx_bytes += length;
			}
		} else {
			ei_local->stat.tx_packets++;
			ei_local->stat.tx_bytes += skb->len;
		}
	} else {
		ei_local->stat.tx_packets++;
		ei_local->stat.tx_bytes += skb->len;
	}
	if (ei_local->features & FE_INT_NAPI) {
		if (ei_local->tx_full == 1) {
			ei_local->tx_full = 0;
			netif_wake_queue(dev);
		}
	}

	return length;
}

int rt2880_qdma_eth_send_tso(struct END_DEVICE *ei_local,
			     struct net_device *dev, struct sk_buff *skb,
			     int gmac_no, int ring_no)
{
	unsigned int length = skb->len;
	struct QDMA_txdesc *cpu_ptr, *prev_cpu_ptr;
	struct QDMA_txdesc dummy_desc;
	struct QDMA_txdesc init_dummy_desc;
	int ctx_idx;
	struct iphdr *iph = NULL;
	struct QDMA_txdesc *init_cpu_ptr;
	struct tcphdr *th = NULL;
	struct skb_frag_struct *frag;
	unsigned int nr_frags = skb_shinfo(skb)->nr_frags;
	unsigned int len, size, frag_txd_num, qidx;
	dma_addr_t offset;
	unsigned long flags;
	int i;
	int init_qid, init_qid1;
	struct ipv6hdr *ip6h = NULL;
	struct PSEUDO_ADAPTER *p_ad;

	init_cpu_ptr = &init_dummy_desc;
	cpu_ptr = &init_dummy_desc;

	len = length - skb->data_len;
	dma_sync_single_for_device(&ei_local->qdma_pdev->dev,
				   virt_to_phys(skb->data),
				   len,
				   DMA_TO_DEVICE);
	offset = virt_to_phys(skb->data);
	cpu_ptr->txd_info1.SDP = offset;
	if (len > MAX_QTXD_LEN) {
		cpu_ptr->txd_info3.SDL = 0x3FFF;
		if (ei_local->chip_name == MT7622_FE || ei_local->chip_name == LEOPARD_FE)
			cpu_ptr->txd_info4.SDL = 0x3;
		cpu_ptr->txd_info3.LS_bit = 0;
		len -= MAX_QTXD_LEN;
		offset += MAX_QTXD_LEN;
	} else {
		cpu_ptr->txd_info3.SDL = (len & 0x3FFF);
		if (ei_local->chip_name == MT7622_FE || ei_local->chip_name == LEOPARD_FE)
			cpu_ptr->txd_info4.SDL = len >> 14;
		cpu_ptr->txd_info3.LS_bit = nr_frags ? 0 : 1;
		len = 0;
	}
	if (ei_local->features & FE_HW_SFQ) {
		sfq_parse_layer_info(skb);

		cpu_ptr->txd_info4.VQID0 = 1;
		cpu_ptr->txd_info3.PROT = sfq_prot;
		/* no vlan */
		cpu_ptr->txd_info3.IPOFST = 14 + (sfq_parse_result.vlan1_gap);
	}
	if (gmac_no == 1)
		cpu_ptr->txd_info4.FPORT = 1;
	else
		cpu_ptr->txd_info4.FPORT = 2;

	cpu_ptr->txd_info4.TSO = 0;
	cpu_ptr->txd_info4.QID = 0;
	/* cpu_ptr->txd_info3.QID = ring_no; */
	if ((ei_local->features & QDMA_QOS_MARK) && (skb->mark != 0)) {
		if (skb->mark < 64) {
			qidx = M2Q_table[skb->mark];
			cpu_ptr->txd_info4.QID = ((qidx & 0x30) >> 4);
			cpu_ptr->txd_info3.QID = (qidx & 0x0f);
		} else {
			pr_debug("skb->mark out of range\n");
			cpu_ptr->txd_info3.QID = 0;
			cpu_ptr->txd_info4.QID = 0;
		}
	}
	if (ei_local->features & FE_CSUM_OFFLOAD) {
		if (skb->ip_summed == CHECKSUM_PARTIAL)
			cpu_ptr->txd_info4.TUI_CO = 7;
		else
			cpu_ptr->txd_info4.TUI_CO = 0;
	}

	if (ei_local->features & FE_HW_VLAN_TX) {
		if (skb_vlan_tag_present(skb)) {
			cpu_ptr->txd_info4.VLAN_TAG =
			    0x10000 | skb_vlan_tag_get(skb);
			cpu_ptr->txd_info3.QID = skb_vlan_tag_get(skb);
		} else {
			cpu_ptr->txd_info3.QID = ring_no;
			cpu_ptr->txd_info4.VLAN_TAG = 0;
		}
	}
	if ((ei_local->features & FE_GE2_SUPPORT) && (lan_wan_separate == 1)) {
		if (web_sfq_enable == 1 && (skb->mark == 2)) {
			if (gmac_no == 1)
				cpu_ptr->txd_info3.QID = HW_SFQ_DL;
			else
				cpu_ptr->txd_info3.QID = HW_SFQ_UP;
		} else if (gmac_no == 2) {
			cpu_ptr->txd_info3.QID += 8;
		}
	}
	/*debug multi tx queue */
	init_qid = cpu_ptr->txd_info3.QID;
	init_qid1 = cpu_ptr->txd_info4.QID;
#if defined(CONFIG_RA_HW_NAT) || defined(CONFIG_RA_HW_NAT_MODULE)
	if (IS_MAGIC_TAG_PROTECT_VALID_HEAD(skb)) {
		if (FOE_MAGIC_TAG_HEAD(skb) == FOE_MAGIC_PPE) {
			if (ra_sw_nat_hook_rx) {
				cpu_ptr->txd_info4.FPORT = 4;	/* PPE */
				FOE_MAGIC_TAG(skb) = 0;
			}
		}
	} else if (IS_MAGIC_TAG_PROTECT_VALID_TAIL(skb)) {
		if (FOE_MAGIC_TAG_TAIL(skb) == FOE_MAGIC_PPE) {
			if (ra_sw_nat_hook_rx) {
				cpu_ptr->txd_info4.FPORT = 4;	/* PPE */
				FOE_MAGIC_TAG(skb) = 0;
			}
		}
	}
#endif

	cpu_ptr->txd_info3.SWC_bit = 1;

	ctx_idx = get_free_txd(ei_local, ring_no);
	cpu_ptr->txd_info2.NDP = get_phy_addr(ei_local, ctx_idx);
	/*prev_cpu_ptr->txd_info1 = dummy_desc.txd_info1;
	 *prev_cpu_ptr->txd_info2 = dummy_desc.txd_info2;
	 *prev_cpu_ptr->txd_info3 = dummy_desc.txd_info3;
	 *prev_cpu_ptr->txd_info4 = dummy_desc.txd_info4;
	 */
	if (len > 0) {
		frag_txd_num = cal_frag_txd_num(len);
		for (frag_txd_num = frag_txd_num; frag_txd_num > 0;
		     frag_txd_num--) {
			if (len < MAX_QTXD_LEN)
				size = len;
			else
				size = MAX_QTXD_LEN;

			cpu_ptr = (ei_local->txd_pool + (ctx_idx));
			dummy_desc.txd_info1 = cpu_ptr->txd_info1;
			dummy_desc.txd_info2 = cpu_ptr->txd_info2;
			dummy_desc.txd_info3 = cpu_ptr->txd_info3;
			dummy_desc.txd_info4 = cpu_ptr->txd_info4;
			prev_cpu_ptr = cpu_ptr;
			cpu_ptr = &dummy_desc;
			cpu_ptr->txd_info3.QID = init_qid;
			cpu_ptr->txd_info4.QID = init_qid1;
			cpu_ptr->txd_info1.SDP = offset;
			cpu_ptr->txd_info3.SDL = (size & 0x3FFF);
			if (ei_local->chip_name == MT7622_FE ||
			    ei_local->chip_name == LEOPARD_FE)
				cpu_ptr->txd_info4.SDL = size >> 14;
			if ((nr_frags == 0) && (frag_txd_num == 1))
				cpu_ptr->txd_info3.LS_bit = 1;
			else
				cpu_ptr->txd_info3.LS_bit = 0;
			cpu_ptr->txd_info3.OWN_bit = 0;
			cpu_ptr->txd_info3.SWC_bit = 1;
			if (cpu_ptr->txd_info3.LS_bit == 1)
				ei_local->skb_free[ctx_idx] = skb;
			else
				ei_local->skb_free[ctx_idx] = magic_id;
			ctx_idx = get_free_txd(ei_local, ring_no);
			cpu_ptr->txd_info2.NDP =
			    get_phy_addr(ei_local, ctx_idx);
			prev_cpu_ptr->txd_info1 = dummy_desc.txd_info1;
			prev_cpu_ptr->txd_info2 = dummy_desc.txd_info2;
			prev_cpu_ptr->txd_info3 = dummy_desc.txd_info3;
			prev_cpu_ptr->txd_info4 = dummy_desc.txd_info4;
			offset += size;
			len -= size;
		}
	}

	for (i = 0; i < nr_frags; i++) {
		/* 1. set or get init value for current fragment */
		offset = 0;
		frag = &skb_shinfo(skb)->frags[i];
		len = frag->size;
		frag_txd_num = cal_frag_txd_num(len);
		for (frag_txd_num = frag_txd_num;
		     frag_txd_num > 0; frag_txd_num--) {
			/* 2. size will be assigned to SDL
			 * and can't be larger than MAX_TXD_LEN
			 */
			if (len < MAX_QTXD_LEN)
				size = len;
			else
				size = MAX_QTXD_LEN;

			/* 3. Update TXD info */
			cpu_ptr = (ei_local->txd_pool + (ctx_idx));
			dummy_desc.txd_info1 = cpu_ptr->txd_info1;
			dummy_desc.txd_info2 = cpu_ptr->txd_info2;
			dummy_desc.txd_info3 = cpu_ptr->txd_info3;
			dummy_desc.txd_info4 = cpu_ptr->txd_info4;
			prev_cpu_ptr = cpu_ptr;
			cpu_ptr = &dummy_desc;
			cpu_ptr->txd_info3.QID = init_qid;
			cpu_ptr->txd_info4.QID = init_qid1;
			cpu_ptr->txd_info1.SDP =
			    dma_map_page(&ei_local->qdma_pdev->dev,
					 frag->page.p,
					 frag->page_offset +
					 offset, size, DMA_TO_DEVICE);
			if (unlikely(dma_mapping_error
					(&ei_local->qdma_pdev->dev,
					 cpu_ptr->txd_info1.SDP)))
				pr_err("[%s]dma_map_page() failed...\n",
				       __func__);

			cpu_ptr->txd_info3.SDL = (size & 0x3FFF);
			if (ei_local->chip_name == MT7622_FE ||
			    ei_local->chip_name == LEOPARD_FE)
				cpu_ptr->txd_info4.SDL = size >> 14;

			if ((i == (nr_frags - 1)) && (frag_txd_num == 1))
				cpu_ptr->txd_info3.LS_bit = 1;
			else
				cpu_ptr->txd_info3.LS_bit = 0;
			cpu_ptr->txd_info3.OWN_bit = 0;
			cpu_ptr->txd_info3.SWC_bit = 1;
			/* 4. Update skb_free for housekeeping */
			if (cpu_ptr->txd_info3.LS_bit == 1)
				ei_local->skb_free[ctx_idx] = skb;
			else
				ei_local->skb_free[ctx_idx] = magic_id;

			/* 5. Get next TXD */
			ctx_idx = get_free_txd(ei_local, ring_no);
			cpu_ptr->txd_info2.NDP =
			    get_phy_addr(ei_local, ctx_idx);
			prev_cpu_ptr->txd_info1 = dummy_desc.txd_info1;
			prev_cpu_ptr->txd_info2 = dummy_desc.txd_info2;
			prev_cpu_ptr->txd_info3 = dummy_desc.txd_info3;
			prev_cpu_ptr->txd_info4 = dummy_desc.txd_info4;
			/* 6. Update offset and len. */
			offset += size;
			len -= size;
		}
	}

	if (skb_shinfo(skb)->gso_segs > 1) {
		/* TsoLenUpdate(skb->len); */

		/* TCP over IPv4 */
		iph = (struct iphdr *)skb_network_header(skb);
		if ((iph->version == 4) && (iph->protocol == IPPROTO_TCP)) {
			th = (struct tcphdr *)skb_transport_header(skb);

			init_cpu_ptr->txd_info4.TSO = 1;

			th->check = htons(skb_shinfo(skb)->gso_size);

			dma_sync_single_for_device(&ei_local->qdma_pdev->dev,
						   virt_to_phys(th),
						   sizeof(struct
							  tcphdr),
						   DMA_TO_DEVICE);
		}
		if (ei_local->features & FE_TSO_V6) {
			ip6h = (struct ipv6hdr *)skb_network_header(skb);
			if ((ip6h->nexthdr == NEXTHDR_TCP) &&
			    (ip6h->version == 6)) {
				th = (struct tcphdr *)skb_transport_header(skb);
				init_cpu_ptr->txd_info4.TSO = 1;
				th->check = htons(skb_shinfo(skb)->gso_size);
				dma_sync_single_for_device(&ei_local->qdma_pdev->dev,
							   virt_to_phys(th),
							   sizeof(struct
								  tcphdr),
							   DMA_TO_DEVICE);
			}
		}

		if (ei_local->features & FE_HW_SFQ) {
			init_cpu_ptr->txd_info4.VQID0 = 1;
			init_cpu_ptr->txd_info3.PROT = sfq_prot;
			/* no vlan */
			init_cpu_ptr->txd_info3.IPOFST =
			    14 + (sfq_parse_result.vlan1_gap);
		}
	}
	/* dma_cache_sync(NULL, skb->data, skb->len, DMA_TO_DEVICE); */

	init_cpu_ptr->txd_info3.OWN_bit = 0;
	spin_lock_irqsave(&ei_local->page_lock, flags);
	prev_cpu_ptr = ei_local->txd_pool + ei_local->tx_cpu_idx;
	ei_local->skb_free[ei_local->tx_cpu_idx] = magic_id;
	ei_local->tx_cpu_idx = ctx_idx;
	prev_cpu_ptr->txd_info1 = init_dummy_desc.txd_info1;
	prev_cpu_ptr->txd_info2 = init_dummy_desc.txd_info2;
	prev_cpu_ptr->txd_info4 = init_dummy_desc.txd_info4;
	prev_cpu_ptr->txd_info3 = init_dummy_desc.txd_info3;

	/* NOTE: add memory barrier to avoid
	 * DMA access memory earlier than memory written
	 */
	wmb();
	sys_reg_write(QTX_CTX_PTR,
		      get_phy_addr(ei_local, ei_local->tx_cpu_idx));
	spin_unlock_irqrestore(&ei_local->page_lock, flags);

	if (ei_local->features & FE_GE2_SUPPORT) {
		if (gmac_no == 2) {
			if (ei_local->pseudo_dev) {
				p_ad = netdev_priv(ei_local->pseudo_dev);
				p_ad->stat.tx_packets++;
				p_ad->stat.tx_bytes += length;
			}
		} else {
			ei_local->stat.tx_packets++;
			ei_local->stat.tx_bytes += skb->len;
		}
	} else {
		ei_local->stat.tx_packets++;
		ei_local->stat.tx_bytes += skb->len;
	}
	if (ei_local->features & FE_INT_NAPI) {
		if (ei_local->tx_full == 1) {
			ei_local->tx_full = 0;
			netif_wake_queue(dev);
		}
	}

	return length;
}

/* QDMA functions */
int fe_qdma_wait_dma_idle(void)
{
	unsigned int reg_val;

	while (1) {
		reg_val = sys_reg_read(QDMA_GLO_CFG);
		if ((reg_val & RX_DMA_BUSY)) {
			pr_err("\n  RX_DMA_BUSY !!! ");
			continue;
		}
		if ((reg_val & TX_DMA_BUSY)) {
			pr_err("\n  TX_DMA_BUSY !!! ");
			continue;
		}
		return 0;
	}

	return -1;
}

int fe_qdma_rx_dma_init(struct net_device *dev)
{
	int i;
	struct END_DEVICE *ei_local = netdev_priv(dev);
	unsigned int skb_size;
	/* Initial QDMA RX Ring */

	skb_size = SKB_DATA_ALIGN(MAX_RX_LENGTH + NET_IP_ALIGN + NET_SKB_PAD) +
		   SKB_DATA_ALIGN(sizeof(struct skb_shared_info));

	ei_local->qrx_ring =
	    dma_alloc_coherent(&ei_local->qdma_pdev->dev,
			       NUM_QRX_DESC * sizeof(struct PDMA_rxdesc),
			       &ei_local->phy_qrx_ring,
			       GFP_ATOMIC | __GFP_ZERO);
	for (i = 0; i < NUM_QRX_DESC; i++) {
		ei_local->netrx0_skb_data[i] =
		    raeth_alloc_skb_data(skb_size, GFP_KERNEL);
		if (!ei_local->netrx0_skb_data[i]) {
			pr_err("rx skbuff buffer allocation failed!");
			goto no_rx_mem;
		}

		memset(&ei_local->qrx_ring[i], 0, sizeof(struct PDMA_rxdesc));
		ei_local->qrx_ring[i].rxd_info2.DDONE_bit = 0;
		ei_local->qrx_ring[i].rxd_info2.LS0 = 0;
		ei_local->qrx_ring[i].rxd_info2.PLEN0 = MAX_RX_LENGTH;
		ei_local->qrx_ring[i].rxd_info1.PDP0 =
		    dma_map_single(&ei_local->qdma_pdev->dev,
				   ei_local->netrx0_skb_data[i] +
				   NET_SKB_PAD,
				   MAX_RX_LENGTH,
				   DMA_FROM_DEVICE);
		if (unlikely
		    (dma_mapping_error
		     (&ei_local->qdma_pdev->dev,
		      ei_local->qrx_ring[i].rxd_info1.PDP0))) {
			pr_err("[%s]dma_map_single() failed...\n", __func__);
			goto no_rx_mem;
		}
	}
	pr_err("\nphy_qrx_ring = 0x%p, qrx_ring = 0x%p\n",
	       (void *)ei_local->phy_qrx_ring, ei_local->qrx_ring);

	/* Tell the adapter where the RX rings are located. */
	sys_reg_write(QRX_BASE_PTR_0,
		      phys_to_bus((u32)ei_local->phy_qrx_ring));
	sys_reg_write(QRX_MAX_CNT_0, cpu_to_le32((u32)NUM_QRX_DESC));
	sys_reg_write(QRX_CRX_IDX_0, cpu_to_le32((u32)(NUM_QRX_DESC - 1)));

	sys_reg_write(QDMA_RST_CFG, PST_DRX_IDX0);
	ei_local->rx_ring[0] = ei_local->qrx_ring;

	return 0;

no_rx_mem:
	return -ENOMEM;
}

int fe_qdma_tx_dma_init(struct net_device *dev)
{
	bool pass;
	struct END_DEVICE *ei_local = netdev_priv(dev_raether);

	if (ei_local->features & FE_HW_SFQ)
		sfq_init(dev);
	/*tx desc alloc, add a NULL TXD to HW */
	pass = qdma_tx_desc_alloc();
	if (!pass)
		return -1;

	pass = fq_qdma_init(dev);
	if (!pass)
		return -1;

	return 0;
}

void fe_qdma_rx_dma_deinit(struct net_device *dev)
{
	struct END_DEVICE *ei_local = netdev_priv(dev);
	int i;

	/* free RX Ring */
	dma_free_coherent(&ei_local->qdma_pdev->dev,
			  NUM_QRX_DESC * sizeof(struct PDMA_rxdesc),
			  ei_local->qrx_ring, ei_local->phy_qrx_ring);

	/* free RX skb */
	for (i = 0; i < NUM_QRX_DESC; i++) {
		raeth_free_skb_data(ei_local->netrx0_skb_data[i]);
		ei_local->netrx0_skb_data[i] = NULL;
	}
}

void fe_qdma_tx_dma_deinit(struct net_device *dev)
{
	struct END_DEVICE *ei_local = netdev_priv(dev);
	int i;

	/* free TX Ring */
	if (ei_local->txd_pool)
		dma_free_coherent(&ei_local->qdma_pdev->dev,
				  num_tx_desc * QTXD_LEN,
				  ei_local->txd_pool, ei_local->phy_txd_pool);
	if (ei_local->free_head)
		dma_free_coherent(&ei_local->qdma_pdev->dev,
				  NUM_QDMA_PAGE * QTXD_LEN,
				  ei_local->free_head, ei_local->phy_free_head);
	if (ei_local->free_page_head)
		dma_free_coherent(&ei_local->qdma_pdev->dev,
				  NUM_QDMA_PAGE * QDMA_PAGE_SIZE,
				  ei_local->free_page_head,
				  ei_local->phy_free_page_head);

	/* free TX data */
	for (i = 0; i < num_tx_desc; i++) {
		if ((ei_local->skb_free[i] != (struct sk_buff *)0xFFFFFFFF) &&
		    (ei_local->skb_free[i] != 0))
			dev_kfree_skb_any(ei_local->skb_free[i]);
	}
}

void set_fe_qdma_glo_cfg(void)
{
	unsigned int reg_val;
	unsigned int dma_glo_cfg = 0;
	struct END_DEVICE *ei_local = netdev_priv(dev_raether);

	reg_val = sys_reg_read(QDMA_GLO_CFG);
	reg_val &= 0x000000FF;

	sys_reg_write(QDMA_GLO_CFG, reg_val);
	reg_val = sys_reg_read(QDMA_GLO_CFG);

	/* Enable randon early drop and set drop threshold automatically */
	if (!(ei_local->features & FE_HW_SFQ))
		sys_reg_write(QDMA_FC_THRES, 0x4444);
	sys_reg_write(QDMA_HRED2, 0x0);

	dma_glo_cfg =
	    (TX_WB_DDONE | RX_DMA_EN | TX_DMA_EN | PDMA_BT_SIZE_16DWORDS);
	dma_glo_cfg |= (RX_2B_OFFSET);
	sys_reg_write(QDMA_GLO_CFG, dma_glo_cfg);

	pr_err("Enable QDMA TX NDP coherence check and re-read mechanism\n");
	reg_val = sys_reg_read(QDMA_GLO_CFG);
	reg_val = reg_val | 0x400 | 0x100000;
	sys_reg_write(QDMA_GLO_CFG, reg_val);
	pr_err("***********QDMA_GLO_CFG=%x\n", sys_reg_read(QDMA_GLO_CFG));
}

int ei_qdma_start_xmit(struct sk_buff *skb, struct net_device *dev, int gmac_no)
{
	struct END_DEVICE *ei_local = netdev_priv(dev);
	unsigned int num_of_txd = 0;
	unsigned int nr_frags = skb_shinfo(skb)->nr_frags, i;
	struct skb_frag_struct *frag;
	struct PSEUDO_ADAPTER *p_ad;
	int ring_no;

	ring_no = skb->queue_mapping + (gmac_no - 1) * gmac1_txq_num;

#if defined(CONFIG_RA_HW_NAT)  || defined(CONFIG_RA_HW_NAT_MODULE)
	if (ra_sw_nat_hook_tx) {
		if (ra_sw_nat_hook_tx(skb, gmac_no) != 1) {
			dev_kfree_skb_any(skb);
			return 0;
		}
	}
#endif

	dev->trans_start = jiffies;	/* save the timestamp */
	/*spin_lock_irqsave(&ei_local->page_lock, flags); */

	/* check free_txd_num before calling rt288_eth_send() */

	if (ei_local->features & FE_TSO) {
		num_of_txd += cal_frag_txd_num(skb->len - skb->data_len);
		if (nr_frags != 0) {
			for (i = 0; i < nr_frags; i++) {
				frag = &skb_shinfo(skb)->frags[i];
				num_of_txd += cal_frag_txd_num(frag->size);
			}
		}
	} else {
		num_of_txd = 1;
	}

/* if ((ei_local->free_txd_num > num_of_txd + 1)) { */
	if (likely(atomic_read(&ei_local->free_txd_num[ring_no]) >
		   (num_of_txd + 1))) {
		if (num_of_txd == 1)
			rt2880_qdma_eth_send(ei_local, dev, skb,
					     gmac_no, ring_no);
		else
			rt2880_qdma_eth_send_tso(ei_local, dev, skb,
						 gmac_no, ring_no);
	} else {
		if (ei_local->features & FE_GE2_SUPPORT) {
			if (gmac_no == 2) {
				if (ei_local->pseudo_dev) {
					p_ad =
					    netdev_priv(ei_local->pseudo_dev);
					p_ad->stat.tx_dropped++;
				}
			} else {
				ei_local->stat.tx_dropped++;
			}
		} else {
			ei_local->stat.tx_dropped++;
		}
		/* kfree_skb(skb); */
		dev_kfree_skb_any(skb);
		/* spin_unlock_irqrestore(&ei_local->page_lock, flags); */
		return 0;
	}
	/* spin_unlock_irqrestore(&ei_local->page_lock, flags); */
	return 0;
}

int ei_qdma_xmit_housekeeping(struct net_device *netdev, int budget)
{
	struct END_DEVICE *ei_local = netdev_priv(netdev);

	dma_addr_t dma_ptr;
	struct QDMA_txdesc *cpu_ptr = NULL;
	dma_addr_t tmp_ptr;
	unsigned int ctx_offset = 0;
	unsigned int dtx_offset = 0;
	unsigned int rls_cnt[TOTAL_TXQ_NUM] = { 0 };
	int ring_no;
	int i;

	dma_ptr = (dma_addr_t)sys_reg_read(QTX_DRX_PTR);
	ctx_offset = ei_local->rls_cpu_idx;
	dtx_offset = (dma_ptr - ei_local->phy_txd_pool) / QTXD_LEN;
	cpu_ptr = (ei_local->txd_pool + (ctx_offset));
	while (ctx_offset != dtx_offset) {
		/* 1. keep cpu next TXD */
		tmp_ptr = (dma_addr_t)cpu_ptr->txd_info2.NDP;
		ring_no = ring_no_mapping(ctx_offset);
		rls_cnt[ring_no]++;
		/* 2. release TXD */
		ei_local->txd_pool_info[ei_local->free_txd_tail[ring_no]] =
		    ctx_offset;
		ei_local->free_txd_tail[ring_no] = ctx_offset;
		/* atomic_add(1, &ei_local->free_txd_num[ring_no]); */
		/* 3. update ctx_offset and free skb memory */
		ctx_offset = (tmp_ptr - ei_local->phy_txd_pool) / QTXD_LEN;
		if (ei_local->features & FE_TSO) {
			if (ei_local->skb_free[ctx_offset] != magic_id) {
				dev_kfree_skb_any(ei_local->skb_free
						  [ctx_offset]);
			}
		} else {
			dev_kfree_skb_any(ei_local->skb_free[ctx_offset]);
		}
		ei_local->skb_free[ctx_offset] = 0;
		/* 4. update cpu_ptr */
		cpu_ptr = (ei_local->txd_pool + ctx_offset);
	}
	for (i = 0; i < TOTAL_TXQ_NUM; i++) {
		if (rls_cnt[i] > 0)
			atomic_add(rls_cnt[i], &ei_local->free_txd_num[i]);
	}
	/* atomic_add(rls_cnt, &ei_local->free_txd_num[0]); */
	ei_local->rls_cpu_idx = ctx_offset;
	netif_wake_queue(netdev);
	if (ei_local->features & FE_GE2_SUPPORT)
		netif_wake_queue(ei_local->pseudo_dev);
	ei_local->tx_ring_full = 0;
	sys_reg_write(QTX_CRX_PTR,
		      (ei_local->phy_txd_pool + (ctx_offset * QTXD_LEN)));

	return 0;
}

int ei_qdma_ioctl(struct net_device *dev, struct ifreq *ifr,
		  struct qdma_ioctl_data *data)
{
	int ret = 0;
	struct END_DEVICE *ei_local = netdev_priv(dev);
	unsigned int cmd;

	cmd = data->cmd;

	switch (cmd) {
	case RAETH_QDMA_REG_READ:

		if (data->off > REG_HQOS_MAX) {
			ret = -EINVAL;
			break;
		}

		if (ei_local->chip_name == MT7622_FE) {	/* harry */
			unsigned int page = 0;

			/* q16~q31: 0x100 <= data->off < 0x200
			 * q32~q47: 0x200 <= data->off < 0x300
			 * q48~q63: 0x300 <= data->off < 0x400
			 */
			if (data->off >= 0x100 && data->off < 0x200) {
				page = 1;
				data->off = data->off - 0x100;
			} else if (data->off >= 0x200 && data->off < 0x300) {
				page = 2;
				data->off = data->off - 0x200;
			} else if (data->off >= 0x300 && data->off < 0x400) {
				page = 3;
				data->off = data->off - 0x300;
			} else {
				page = 0;
			}
			/*magic number for ioctl identify CR 0x1b101a14*/
			if (data->off == 0x777) {
				page = 0;
				data->off = 0x214;
			}

			sys_reg_write(QDMA_PAGE, page);
			/* pr_debug("page=%d, data->off =%x\n", page, data->off); */
		}

		data->val = sys_reg_read(QTX_CFG_0 + data->off);
		pr_info("read reg off:%x val:%x\n", data->off, data->val);
		ret = copy_to_user(ifr->ifr_data, data, sizeof(*data));
		sys_reg_write(QDMA_PAGE, 0);
		if (ret) {
			pr_info("ret=%d\n", ret);
			ret = -EFAULT;
		}
		break;
	case RAETH_QDMA_REG_WRITE:

		if (data->off > REG_HQOS_MAX) {
			ret = -EINVAL;
			break;
		}

		if (ei_local->chip_name == MT7622_FE) {	/* harry */
			unsigned int page = 0;
			/*QoS must enable QDMA drop packet policy*/
			sys_reg_write(QDMA_FC_THRES, 0x83834444);
			/* q16~q31: 0x100 <= data->off < 0x200
			 * q32~q47: 0x200 <= data->off < 0x300
			 * q48~q63: 0x300 <= data->off < 0x400
			 */
			if (data->off >= 0x100 && data->off < 0x200) {
				page = 1;
				data->off = data->off - 0x100;
			} else if (data->off >= 0x200 && data->off < 0x300) {
				page = 2;
				data->off = data->off - 0x200;
			} else if (data->off >= 0x300 && data->off < 0x400) {
				page = 3;
				data->off = data->off - 0x300;
			} else {
				page = 0;
			}
			/*magic number for ioctl identify CR 0x1b101a14*/
			if (data->off == 0x777) {
				page = 0;
				data->off = 0x214;
			}
			sys_reg_write(QDMA_PAGE, page);
			/*pr_info("data->val =%x\n", data->val);*/
			sys_reg_write(QTX_CFG_0 + data->off, data->val);
			sys_reg_write(QDMA_PAGE, 0);
		} else {
			sys_reg_write(QTX_CFG_0 + data->off, data->val);
		}
		/* pr_ino("write reg off:%x val:%x\n", data->off, data->val); */
		break;
	case RAETH_QDMA_QUEUE_MAPPING:
		if ((data->off & 0x100) == 0x100) {
			lan_wan_separate = 1;
			data->off &= 0xff;
		} else {
			lan_wan_separate = 0;
			data->off &= 0xff;
		}
		M2Q_table[data->off] = data->val;
		break;
	case RAETH_QDMA_SFQ_WEB_ENABLE:
		if (ei_local->features & FE_HW_SFQ) {
			if ((data->val) == 0x1)
				web_sfq_enable = 1;
			else
				web_sfq_enable = 0;
		} else {
			ret = -EINVAL;
		}
		break;
	default:
		ret = 1;
		break;
	}

	return ret;
}
