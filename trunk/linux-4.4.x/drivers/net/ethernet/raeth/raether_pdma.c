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

int fe_pdma_wait_dma_idle(void)
{
	unsigned int reg_val;
	unsigned int loop_cnt = 0;

	while (1) {
		if (loop_cnt++ > 1000)
			break;
		reg_val = sys_reg_read(PDMA_GLO_CFG);
		if ((reg_val & RX_DMA_BUSY)) {
			pr_warn("\n  RX_DMA_BUSY !!! ");
			continue;
		}
		if ((reg_val & TX_DMA_BUSY)) {
			pr_warn("\n  TX_DMA_BUSY !!! ");
			continue;
		}
		return 0;
	}

	return -1;
}

int fe_pdma_rx_dma_init(struct net_device *dev)
{
	int i;
	unsigned int skb_size;
	struct END_DEVICE *ei_local = netdev_priv(dev);
	dma_addr_t dma_addr;

	skb_size = SKB_DATA_ALIGN(MAX_RX_LENGTH + NET_IP_ALIGN + NET_SKB_PAD) +
		   SKB_DATA_ALIGN(sizeof(struct skb_shared_info));

	/* Initial RX Ring 0 */
	ei_local->rx_ring[0] = dma_alloc_coherent(dev->dev.parent,
						num_rx_desc *
						sizeof(struct PDMA_rxdesc),
						&ei_local->phy_rx_ring[0],
						GFP_ATOMIC | __GFP_ZERO);
	pr_debug("\nphy_rx_ring[0] = 0x%08x, rx_ring[0] = 0x%p\n",
		 (unsigned int)ei_local->phy_rx_ring[0],
		 (void *)ei_local->rx_ring[0]);

	for (i = 0; i < num_rx_desc; i++) {
		ei_local->netrx_skb_data[0][i] =
			raeth_alloc_skb_data(skb_size, GFP_KERNEL);
		if (!ei_local->netrx_skb_data[0][i]) {
			pr_err("rx skbuff buffer allocation failed!");
			goto no_rx_mem;
		}

		memset(&ei_local->rx_ring[0][i], 0, sizeof(struct PDMA_rxdesc));
		ei_local->rx_ring[0][i].rxd_info2.DDONE_bit = 0;
		ei_local->rx_ring[0][i].rxd_info2.LS0 = 0;
		ei_local->rx_ring[0][i].rxd_info2.PLEN0 = MAX_RX_LENGTH;
		dma_addr = dma_map_single(dev->dev.parent,
					  ei_local->netrx_skb_data[0][i] +
					  NET_SKB_PAD,
					  MAX_RX_LENGTH,
					  DMA_FROM_DEVICE);
		ei_local->rx_ring[0][i].rxd_info1.PDP0 = dma_addr;
		if (unlikely
		    (dma_mapping_error
		     (dev->dev.parent,
		      ei_local->rx_ring[0][i].rxd_info1.PDP0))) {
			pr_err("[%s]dma_map_single() failed...\n", __func__);
			goto no_rx_mem;
		}
	}

	/* Tell the adapter where the RX rings are located. */
	sys_reg_write(RX_BASE_PTR0, phys_to_bus((u32)ei_local->phy_rx_ring[0]));
	sys_reg_write(RX_MAX_CNT0, cpu_to_le32((u32)num_rx_desc));
	sys_reg_write(RX_CALC_IDX0, cpu_to_le32((u32)(num_rx_desc - 1)));

	sys_reg_write(PDMA_RST_CFG, PST_DRX_IDX0);

	return 0;

no_rx_mem:
	return -ENOMEM;
}

int fe_pdma_tx_dma_init(struct net_device *dev)
{
	int i;
	struct END_DEVICE *ei_local = netdev_priv(dev);

	for (i = 0; i < num_tx_desc; i++)
		ei_local->skb_free[i] = 0;

	ei_local->tx_ring_full = 0;
	ei_local->free_idx = 0;
	ei_local->tx_ring0 =
	    dma_alloc_coherent(dev->dev.parent,
			       num_tx_desc * sizeof(struct PDMA_txdesc),
			       &ei_local->phy_tx_ring0,
			       GFP_ATOMIC | __GFP_ZERO);
	pr_debug("\nphy_tx_ring = 0x%08x, tx_ring = 0x%p\n",
		 (unsigned int)ei_local->phy_tx_ring0,
		 (void *)ei_local->tx_ring0);

	for (i = 0; i < num_tx_desc; i++) {
		memset(&ei_local->tx_ring0[i], 0, sizeof(struct PDMA_txdesc));
		ei_local->tx_ring0[i].txd_info2.LS0_bit = 1;
		ei_local->tx_ring0[i].txd_info2.DDONE_bit = 1;
	}

	/* Tell the adapter where the TX rings are located. */
	sys_reg_write(TX_BASE_PTR0, phys_to_bus((u32)ei_local->phy_tx_ring0));
	sys_reg_write(TX_MAX_CNT0, cpu_to_le32((u32)num_tx_desc));
	sys_reg_write(TX_CTX_IDX0, 0);
#ifdef CONFIG_RAETH_RW_PDMAPTR_FROM_VAR
	ei_local->tx_cpu_owner_idx0 = 0;
#endif
	sys_reg_write(PDMA_RST_CFG, PST_DTX_IDX0);

	return 0;
}

void fe_pdma_rx_dma_deinit(struct net_device *dev)
{
	struct END_DEVICE *ei_local = netdev_priv(dev);
	int i;

	/* free RX Ring */
	dma_free_coherent(dev->dev.parent,
			  num_rx_desc * sizeof(struct PDMA_rxdesc),
			  ei_local->rx_ring[0], ei_local->phy_rx_ring[0]);

	/* free RX data */
	for (i = 0; i < num_rx_desc; i++) {
		raeth_free_skb_data(ei_local->netrx_skb_data[0][i]);
		ei_local->netrx_skb_data[0][i] = NULL;
	}
}

void fe_pdma_tx_dma_deinit(struct net_device *dev)
{
	struct END_DEVICE *ei_local = netdev_priv(dev);
	int i;

	/* free TX Ring */
	if (ei_local->tx_ring0)
		dma_free_coherent(dev->dev.parent,
				  num_tx_desc *
				  sizeof(struct PDMA_txdesc),
				  ei_local->tx_ring0,
				  ei_local->phy_tx_ring0);

	/* free TX data */
	for (i = 0; i < num_tx_desc; i++) {
		if ((ei_local->skb_free[i] != 0) &&
		    (ei_local->skb_free[i] != (struct sk_buff *)0xFFFFFFFF))
			dev_kfree_skb_any(ei_local->skb_free[i]);
	}
}

void set_fe_pdma_glo_cfg(void)
{
	unsigned int dma_glo_cfg = 0;

	dma_glo_cfg =
	    (TX_WB_DDONE | RX_DMA_EN | TX_DMA_EN | PDMA_BT_SIZE_16DWORDS |
	     MULTI_EN | ADMA_RX_BT_SIZE_32DWORDS);
	dma_glo_cfg |= (RX_2B_OFFSET);

	sys_reg_write(PDMA_GLO_CFG, dma_glo_cfg);
}

/* @brief cal txd number for a page
 *
 *  @parm size
 *
 *  @return frag_txd_num
 */
static inline unsigned int pdma_cal_frag_txd_num(unsigned int size)
{
	unsigned int frag_txd_num = 0;

	if (size == 0)
		return 0;
	while (size > 0) {
		if (size > MAX_PTXD_LEN) {
			frag_txd_num++;
			size -= MAX_PTXD_LEN;
		} else {
			frag_txd_num++;
			size = 0;
		}
	}
	return frag_txd_num;
}

int fe_fill_tx_desc(struct net_device *dev,
		    unsigned long *tx_cpu_owner_idx,
		    struct sk_buff *skb,
		    int gmac_no)
{
	struct END_DEVICE *ei_local = netdev_priv(dev);
	struct PDMA_txdesc *tx_ring = &ei_local->tx_ring0[*tx_cpu_owner_idx];
	struct PDMA_TXD_INFO2_T txd_info2_tmp;
	struct PDMA_TXD_INFO4_T txd_info4_tmp;

	tx_ring->txd_info1.SDP0 = virt_to_phys(skb->data);
	txd_info2_tmp.SDL0 = skb->len;
	txd_info4_tmp.FPORT = gmac_no;
	txd_info4_tmp.TSO = 0;

	if (ei_local->features & FE_CSUM_OFFLOAD) {
		if (skb->ip_summed == CHECKSUM_PARTIAL)
			txd_info4_tmp.TUI_CO = 7;
		else
			txd_info4_tmp.TUI_CO = 0;
	}

	if (ei_local->features & FE_HW_VLAN_TX) {
		if (skb_vlan_tag_present(skb))
			txd_info4_tmp.VLAN_TAG =
				0x10000 | skb_vlan_tag_get(skb);
		else
			txd_info4_tmp.VLAN_TAG = 0;
	}
#if defined(CONFIG_RA_HW_NAT) || defined(CONFIG_RA_HW_NAT_MODULE)
	if (IS_MAGIC_TAG_PROTECT_VALID_HEAD(skb)) {
		if (FOE_MAGIC_TAG_HEAD(skb) == FOE_MAGIC_PPE) {
			if (ra_sw_nat_hook_rx) {
				/* PPE */
				txd_info4_tmp.FPORT = 4;
				FOE_MAGIC_TAG(skb) = 0;
			}
		}
	} else if (IS_MAGIC_TAG_PROTECT_VALID_TAIL(skb)) {
		if (FOE_MAGIC_TAG_TAIL(skb) == FOE_MAGIC_PPE) {
			if (ra_sw_nat_hook_rx) {
				/* PPE */
				txd_info4_tmp.FPORT = 4;
				FOE_MAGIC_TAG(skb) = 0;
			}
		}
	}
#endif

	txd_info2_tmp.LS0_bit = 1;
	txd_info2_tmp.DDONE_bit = 0;

	tx_ring->txd_info4 = txd_info4_tmp;
	tx_ring->txd_info2 = txd_info2_tmp;

	return 0;
}

static int fe_fill_tx_tso_data(struct END_DEVICE *ei_local,
			       unsigned int frag_offset,
			       unsigned int frag_size,
			       unsigned long *tx_cpu_owner_idx,
			       unsigned int nr_frags,
			       int gmac_no)
{
	struct PSEUDO_ADAPTER *p_ad;
	unsigned int size;
	unsigned int frag_txd_num;
	struct PDMA_txdesc *tx_ring;

	frag_txd_num = pdma_cal_frag_txd_num(frag_size);
	tx_ring = &ei_local->tx_ring0[*tx_cpu_owner_idx];

	while (frag_txd_num > 0) {
		if (frag_size < MAX_PTXD_LEN)
			size = frag_size;
		else
			size = MAX_PTXD_LEN;

		if (ei_local->skb_txd_num % 2 == 0) {
			*tx_cpu_owner_idx =
			    (*tx_cpu_owner_idx + 1) % num_tx_desc;
			tx_ring = &ei_local->tx_ring0[*tx_cpu_owner_idx];

			while (tx_ring->txd_info2.DDONE_bit == 0) {
				if (gmac_no == 2) {
					p_ad =
					    netdev_priv(ei_local->pseudo_dev);
					p_ad->stat.tx_errors++;
				} else {
					ei_local->stat.tx_errors++;
				}
			}
			tx_ring->txd_info1.SDP0 = frag_offset;
			tx_ring->txd_info2.SDL0 = size;
			if (((nr_frags == 0)) && (frag_txd_num == 1))
				tx_ring->txd_info2.LS0_bit = 1;
			else
				tx_ring->txd_info2.LS0_bit = 0;
			tx_ring->txd_info2.DDONE_bit = 0;
			tx_ring->txd_info4.FPORT = gmac_no;
		} else {
			tx_ring->txd_info3.SDP1 = frag_offset;
			tx_ring->txd_info2.SDL1 = size;
			if (((nr_frags == 0)) && (frag_txd_num == 1))
				tx_ring->txd_info2.LS1_bit = 1;
			else
				tx_ring->txd_info2.LS1_bit = 0;
		}
		frag_offset += size;
		frag_size -= size;
		frag_txd_num--;
		ei_local->skb_txd_num++;
	}

	return 0;
}

static int fe_fill_tx_tso_frag(struct net_device *netdev,
			       struct sk_buff *skb,
			       unsigned long *tx_cpu_owner_idx,
			       int gmac_no)
{
	struct END_DEVICE *ei_local = netdev_priv(netdev);
	struct PSEUDO_ADAPTER *p_ad;
	unsigned int size;
	unsigned int frag_txd_num;
	struct skb_frag_struct *frag;
	unsigned int nr_frags;
	unsigned int frag_offset, frag_size;
	struct PDMA_txdesc *tx_ring;
	int i = 0, j = 0, unmap_idx = 0;

	nr_frags = skb_shinfo(skb)->nr_frags;
	tx_ring = &ei_local->tx_ring0[*tx_cpu_owner_idx];

	for (i = 0; i < nr_frags; i++) {
		frag = &skb_shinfo(skb)->frags[i];
		frag_offset = frag->page_offset;
		frag_size = frag->size;
		frag_txd_num = pdma_cal_frag_txd_num(frag_size);

		while (frag_txd_num > 0) {
			if (frag_size < MAX_PTXD_LEN)
				size = frag_size;
			else
				size = MAX_PTXD_LEN;

			if (ei_local->skb_txd_num % 2 == 0) {
				*tx_cpu_owner_idx =
					(*tx_cpu_owner_idx + 1) % num_tx_desc;
				tx_ring =
					&ei_local->tx_ring0[*tx_cpu_owner_idx];

				while (tx_ring->txd_info2.DDONE_bit == 0) {
					if (gmac_no == 2) {
						p_ad =
						    netdev_priv
						    (ei_local->pseudo_dev);
						p_ad->stat.tx_errors++;
					} else {
						ei_local->stat.tx_errors++;
					}
				}

				tx_ring->txd_info1.SDP0 =
				    dma_map_page(netdev->dev.parent,
						 frag->page.p, frag_offset,
						 size, DMA_TO_DEVICE);
				if (unlikely
				    (dma_mapping_error
				     (netdev->dev.parent,
				      tx_ring->txd_info1.SDP0))) {
					pr_err
					    ("[%s]dma_map_page() failed\n",
					     __func__);
					goto err_dma;
				}

				tx_ring->txd_info2.SDL0 = size;

				if ((frag_txd_num == 1) &&
				    (i == (nr_frags - 1)))
					tx_ring->txd_info2.LS0_bit = 1;
				else
					tx_ring->txd_info2.LS0_bit = 0;
				tx_ring->txd_info2.DDONE_bit = 0;
				tx_ring->txd_info4.FPORT = gmac_no;
			} else {
				tx_ring->txd_info3.SDP1 =
				    dma_map_page(netdev->dev.parent,
						 frag->page.p, frag_offset,
						 size, DMA_TO_DEVICE);
				if (unlikely
				    (dma_mapping_error
				     (netdev->dev.parent,
				      tx_ring->txd_info3.SDP1))) {
					pr_err
					    ("[%s]dma_map_page() failed\n",
					     __func__);
					goto err_dma;
				}
				tx_ring->txd_info2.SDL1 = size;
				if ((frag_txd_num == 1) &&
				    (i == (nr_frags - 1)))
					tx_ring->txd_info2.LS1_bit = 1;
				else
					tx_ring->txd_info2.LS1_bit = 0;
			}
			frag_offset += size;
			frag_size -= size;
			frag_txd_num--;
			ei_local->skb_txd_num++;
		}
	}

	return 0;

err_dma:
	/* unmap dma */
	j = *tx_cpu_owner_idx;
	unmap_idx = i;
	for (i = 0; i < unmap_idx; i++) {
		frag = &skb_shinfo(skb)->frags[i];
		frag_offset = frag->page_offset;
		frag_size = frag->size;
		frag_txd_num = pdma_cal_frag_txd_num(frag_size);

		while (frag_txd_num > 0) {
			if (frag_size < MAX_PTXD_LEN)
				size = frag_size;
			else
				size = MAX_PTXD_LEN;
			if (ei_local->skb_txd_num % 2 == 0) {
				j = (j + 1) % num_tx_desc;
				dma_unmap_page(netdev->dev.parent,
					       ei_local->tx_ring0[j].
					       txd_info1.SDP0,
					       ei_local->tx_ring0[j].
					       txd_info2.SDL0, DMA_TO_DEVICE);
				/* reinit txd */
				ei_local->tx_ring0[j].txd_info2.LS0_bit = 1;
				ei_local->tx_ring0[j].txd_info2.DDONE_bit = 1;
			} else {
				dma_unmap_page(netdev->dev.parent,
					       ei_local->tx_ring0[j].
					       txd_info3.SDP1,
					       ei_local->tx_ring0[j].
					       txd_info2.SDL1, DMA_TO_DEVICE);
				/* reinit txd */
				ei_local->tx_ring0[j].txd_info2.LS1_bit = 1;
			}
			frag_offset += size;
			frag_size -= size;
			frag_txd_num--;
			ei_local->skb_txd_num++;
		}
	}

	return -1;
}

int fe_fill_tx_desc_tso(struct net_device *dev,
			unsigned long *tx_cpu_owner_idx,
			struct sk_buff *skb,
			int gmac_no)
{
	struct END_DEVICE *ei_local = netdev_priv(dev);
	struct iphdr *iph = NULL;
	struct ipv6hdr *ip6h = NULL;
	struct tcphdr *th = NULL;
	unsigned int nr_frags = skb_shinfo(skb)->nr_frags;
	unsigned int len, offset;
	int err;
	struct PDMA_txdesc *tx_ring = &ei_local->tx_ring0[*tx_cpu_owner_idx];

	tx_ring->txd_info4.FPORT = gmac_no;
	tx_ring->txd_info4.TSO = 0;

	if (skb->ip_summed == CHECKSUM_PARTIAL)
		tx_ring->txd_info4.TUI_CO = 7;
	else
		tx_ring->txd_info4.TUI_CO = 0;

	if (ei_local->features & FE_HW_VLAN_TX) {
		if (skb_vlan_tag_present(skb))
			tx_ring->txd_info4.VLAN_TAG =
				0x10000 | skb_vlan_tag_get(skb);
		else
			tx_ring->txd_info4.VLAN_TAG = 0;
	}
#if defined(CONFIG_RA_HW_NAT) || defined(CONFIG_RA_HW_NAT_MODULE)
	if (IS_MAGIC_TAG_PROTECT_VALID_HEAD(skb)) {
		if (FOE_MAGIC_TAG_HEAD(skb) == FOE_MAGIC_PPE) {
			if (ra_sw_nat_hook_rx) {
				/* PPE */
				tx_ring->txd_info4.FPORT = 4;
				FOE_MAGIC_TAG(skb) = 0;
			}
		}
	} else if (IS_MAGIC_TAG_PROTECT_VALID_TAIL(skb)) {
		if (FOE_MAGIC_TAG_TAIL(skb) == FOE_MAGIC_PPE) {
			if (ra_sw_nat_hook_rx) {
				/* PPE */
				tx_ring->txd_info4.FPORT = 4;
				FOE_MAGIC_TAG(skb) = 0;
			}
		}
	}
#endif
	ei_local->skb_txd_num = 1;

	/* skb data handle */
	len = skb->len - skb->data_len;
	offset = virt_to_phys(skb->data);
	tx_ring->txd_info1.SDP0 = offset;
	if (len < MAX_PTXD_LEN) {
		tx_ring->txd_info2.SDL0 = len;
		tx_ring->txd_info2.LS0_bit = nr_frags ? 0 : 1;
		len = 0;
	} else {
		tx_ring->txd_info2.SDL0 = MAX_PTXD_LEN;
		tx_ring->txd_info2.LS0_bit = 0;
		len -= MAX_PTXD_LEN;
		offset += MAX_PTXD_LEN;
	}

	if (len > 0)
		fe_fill_tx_tso_data(ei_local, offset, len,
				    tx_cpu_owner_idx, nr_frags, gmac_no);

	/* skb fragments handle */
	if (nr_frags > 0) {
		err = fe_fill_tx_tso_frag(dev, skb, tx_cpu_owner_idx, gmac_no);
		if (unlikely(err))
			return err;
	}

	/* fill in MSS info in tcp checksum field */
	if (skb_shinfo(skb)->gso_segs > 1) {
		/* TCP over IPv4 */
		iph = (struct iphdr *)skb_network_header(skb);
		if ((iph->version == 4) && (iph->protocol == IPPROTO_TCP)) {
			th = (struct tcphdr *)skb_transport_header(skb);
			tx_ring->txd_info4.TSO = 1;
			th->check = htons(skb_shinfo(skb)->gso_size);
			dma_sync_single_for_device(dev->dev.parent,
						   virt_to_phys(th),
						   sizeof(struct tcphdr),
						   DMA_TO_DEVICE);
		}

		/* TCP over IPv6 */
		if (ei_local->features & FE_TSO_V6) {
			ip6h = (struct ipv6hdr *)skb_network_header(skb);
			if ((ip6h->nexthdr == NEXTHDR_TCP) &&
			    (ip6h->version == 6)) {
				th = (struct tcphdr *)skb_transport_header(skb);
				tx_ring->txd_info4.TSO = 1;
				th->check = htons(skb_shinfo(skb)->gso_size);
				dma_sync_single_for_device(dev->dev.parent,
							   virt_to_phys(th),
							   sizeof(struct
								  tcphdr),
							   DMA_TO_DEVICE);
			}
		}
	}
	tx_ring->txd_info2.DDONE_bit = 0;

	return 0;
}

static inline int rt2880_pdma_eth_send(struct net_device *dev,
				       struct sk_buff *skb, int gmac_no,
				       unsigned int num_of_frag)
{
	unsigned int length = skb->len;
	struct END_DEVICE *ei_local = netdev_priv(dev);
#ifdef CONFIG_RAETH_RW_PDMAPTR_FROM_VAR
	unsigned long tx_cpu_owner_idx0 = ei_local->tx_cpu_owner_idx0;
#else
	unsigned long tx_cpu_owner_idx0 = sys_reg_read(TX_CTX_IDX0);
#endif
	struct PSEUDO_ADAPTER *p_ad;
	int err;

	while (ei_local->tx_ring0[tx_cpu_owner_idx0].txd_info2.DDONE_bit == 0) {
		if (gmac_no == 2) {
			if (ei_local->pseudo_dev) {
				p_ad = netdev_priv(ei_local->pseudo_dev);
				p_ad->stat.tx_errors++;
			} else {
				pr_err
				    ("pseudo_dev is still not initialize ");
				pr_err
				    ("but receive packet from GMAC2\n");
			}
		} else {
			ei_local->stat.tx_errors++;
		}
	}

	if (num_of_frag > 1)
		err = fe_fill_tx_desc_tso(dev, &tx_cpu_owner_idx0,
					  skb, gmac_no);
	else
		err = fe_fill_tx_desc(dev, &tx_cpu_owner_idx0, skb, gmac_no);
	if (err)
		return err;

	tx_cpu_owner_idx0 = (tx_cpu_owner_idx0 + 1) % num_tx_desc;
	while (ei_local->tx_ring0[tx_cpu_owner_idx0].txd_info2.DDONE_bit == 0) {
		if (gmac_no == 2) {
			p_ad = netdev_priv(ei_local->pseudo_dev);
			p_ad->stat.tx_errors++;
		} else {
			ei_local->stat.tx_errors++;
		}
	}
#ifdef CONFIG_RAETH_RW_PDMAPTR_FROM_VAR
	ei_local->tx_cpu_owner_idx0 = tx_cpu_owner_idx0;
#endif
	/* make sure that all changes to the dma ring are flushed before we
	 * continue
	 */
	wmb();

	sys_reg_write(TX_CTX_IDX0, cpu_to_le32((u32)tx_cpu_owner_idx0));

	if (gmac_no == 2) {
		p_ad = netdev_priv(ei_local->pseudo_dev);
		p_ad->stat.tx_packets++;
		p_ad->stat.tx_bytes += length;
	} else {
		ei_local->stat.tx_packets++;
		ei_local->stat.tx_bytes += length;
	}

	return length;
}

int ei_pdma_start_xmit(struct sk_buff *skb, struct net_device *dev, int gmac_no)
{
	struct END_DEVICE *ei_local = netdev_priv(dev);
	unsigned long tx_cpu_owner_idx;
	unsigned int tx_cpu_owner_idx_next, tx_cpu_owner_idx_next2;
	unsigned int num_of_txd, num_of_frag;
	unsigned int nr_frags = skb_shinfo(skb)->nr_frags, i;
	struct skb_frag_struct *frag;
	struct PSEUDO_ADAPTER *p_ad;
	unsigned int tx_cpu_cal_idx;

#if defined(CONFIG_RA_HW_NAT)  || defined(CONFIG_RA_HW_NAT_MODULE)
	if (ra_sw_nat_hook_tx) {
#if defined(CONFIG_RA_HW_NAT) || defined(CONFIG_RA_HW_NAT_MODULE)
		if (FOE_MAGIC_TAG(skb) != FOE_MAGIC_PPE)
#endif
			if (ra_sw_nat_hook_tx(skb, gmac_no) != 1) {
				dev_kfree_skb_any(skb);
				return 0;
			}
	}
#endif

	dev->trans_start = jiffies;	/* save the timestamp */
	spin_lock(&ei_local->page_lock);
	dma_sync_single_for_device(dev->dev.parent, virt_to_phys(skb->data),
				   skb->len, DMA_TO_DEVICE);

#ifdef CONFIG_RAETH_RW_PDMAPTR_FROM_VAR
	tx_cpu_owner_idx = ei_local->tx_cpu_owner_idx0;
#else
	tx_cpu_owner_idx = sys_reg_read(TX_CTX_IDX0);
#endif

	if (ei_local->features & FE_TSO) {
		num_of_txd = pdma_cal_frag_txd_num(skb->len - skb->data_len);
		if (nr_frags != 0) {
			for (i = 0; i < nr_frags; i++) {
				frag = &skb_shinfo(skb)->frags[i];
				num_of_txd += pdma_cal_frag_txd_num(frag->size);
			}
		}
		num_of_frag = num_of_txd;
		num_of_txd = (num_of_txd + 1) >> 1;
	} else {
		num_of_frag = 1;
		num_of_txd = 1;
	}

	tx_cpu_owner_idx_next = (tx_cpu_owner_idx + num_of_txd) % num_tx_desc;

	if ((ei_local->skb_free[tx_cpu_owner_idx_next] == 0) &&
	    (ei_local->skb_free[tx_cpu_owner_idx] == 0)) {
		if (rt2880_pdma_eth_send(dev, skb, gmac_no, num_of_frag) < 0) {
			dev_kfree_skb_any(skb);
			if (gmac_no == 2) {
				p_ad = netdev_priv(ei_local->pseudo_dev);
				p_ad->stat.tx_dropped++;
			} else {
				ei_local->stat.tx_dropped++;
			}
			goto tx_err;
		}

		tx_cpu_owner_idx_next2 =
		    (tx_cpu_owner_idx_next + 1) % num_tx_desc;

		if (ei_local->skb_free[tx_cpu_owner_idx_next2] != 0)
			ei_local->tx_ring_full = 1;
	} else {
		if (gmac_no == 2) {
			p_ad = netdev_priv(ei_local->pseudo_dev);
			p_ad->stat.tx_dropped++;
		} else {
			ei_local->stat.tx_dropped++;
		}

		dev_kfree_skb_any(skb);
		spin_unlock(&ei_local->page_lock);
		return NETDEV_TX_OK;
	}

	/* SG: use multiple TXD to send the packet (only have one skb) */
	tx_cpu_cal_idx = (tx_cpu_owner_idx + num_of_txd - 1) % num_tx_desc;
	ei_local->skb_free[tx_cpu_cal_idx] = skb;
	while (--num_of_txd)
		/* MAGIC ID */
		ei_local->skb_free[(--tx_cpu_cal_idx) % num_tx_desc] =
			(struct sk_buff *)0xFFFFFFFF;

tx_err:
	spin_unlock(&ei_local->page_lock);
	return NETDEV_TX_OK;
}

int ei_pdma_xmit_housekeeping(struct net_device *netdev, int budget)
{
	struct END_DEVICE *ei_local = netdev_priv(netdev);
	struct PDMA_txdesc *tx_desc;
	unsigned long skb_free_idx;
	int tx_processed = 0;

	tx_desc = ei_local->tx_ring0;
	skb_free_idx = ei_local->free_idx;

	while (budget &&
	       (ei_local->skb_free[skb_free_idx] != 0) &&
	       (tx_desc[skb_free_idx].txd_info2.DDONE_bit == 1)) {
		if (ei_local->skb_free[skb_free_idx] !=
		    (struct sk_buff *)0xFFFFFFFF)
			dev_kfree_skb_any(ei_local->skb_free[skb_free_idx]);

		ei_local->skb_free[skb_free_idx] = 0;
		skb_free_idx = (skb_free_idx + 1) % num_tx_desc;
		budget--;
		tx_processed++;
	}

	ei_local->tx_ring_full = 0;
	ei_local->free_idx = skb_free_idx;

	return tx_processed;
}

