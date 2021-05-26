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
#include "raether_rss.h"
#include "raether_hwlro.h"
#include "ra_mac.h"

static struct proc_dir_entry *proc_rss_ring1, *proc_rss_ring2, *proc_rss_ring3;

int fe_rss_4ring_init(struct net_device *dev)
{
	struct END_DEVICE *ei_local = netdev_priv(dev);
	int skb_size;
	int i, j;

	skb_size = SKB_DATA_ALIGN(MAX_RX_LENGTH + NET_IP_ALIGN) +
		   SKB_DATA_ALIGN(sizeof(struct skb_shared_info));

	/* Initial RX Ring 1 ~ 3 */
	for (i = 1; i < MAX_RX_RING_NUM; i++) {
		ei_local->rx_ring[i] =
			dma_alloc_coherent(dev->dev.parent,
					   NUM_RSS_RX_DESC *
					   sizeof(struct PDMA_rxdesc),
					   &ei_local->phy_rx_ring[i],
					   GFP_ATOMIC | __GFP_ZERO);
		for (j = 0; j < NUM_RSS_RX_DESC; j++) {
			ei_local->netrx_skb_data[i][j] =
				raeth_alloc_skb_data(skb_size, GFP_KERNEL);

			if (!ei_local->netrx_skb_data[i][j]) {
				pr_info("rx skbuff buffer allocation failed!\n");
				goto no_rx_mem;
			}

			memset(&ei_local->rx_ring[i][j], 0,
			       sizeof(struct PDMA_rxdesc));
			ei_local->rx_ring[i][j].rxd_info2.DDONE_bit = 0;
			ei_local->rx_ring[i][j].rxd_info2.LS0 = 0;
			ei_local->rx_ring[i][j].rxd_info2.PLEN0 =
			    SET_ADMA_RX_LEN0(MAX_RX_LENGTH);
			ei_local->rx_ring[i][j].rxd_info1.PDP0 =
			    dma_map_single(dev->dev.parent,
					   ei_local->netrx_skb_data[i][j] +
					   NET_SKB_PAD,
					   MAX_RX_LENGTH, DMA_FROM_DEVICE);
			if (unlikely
			    (dma_mapping_error
			     (dev->dev.parent,
			      ei_local->rx_ring[i][j].rxd_info1.PDP0))) {
				pr_info("[%s]dma_map_single() failed...\n",
					__func__);
				goto no_rx_mem;
			}
		}
		pr_info("\nphy_rx_ring[%d] = 0x%08x, rx_ring[%d] = 0x%p\n",
			i, (unsigned int)ei_local->phy_rx_ring[i],
			i, (void __iomem *)ei_local->rx_ring[i]);
	}

	sys_reg_write(RX_BASE_PTR3, phys_to_bus((u32)ei_local->phy_rx_ring[3]));
	sys_reg_write(RX_MAX_CNT3, cpu_to_le32((u32)NUM_RSS_RX_DESC));
	sys_reg_write(RX_CALC_IDX3, cpu_to_le32((u32)(NUM_RSS_RX_DESC - 1)));
	sys_reg_write(PDMA_RST_CFG, PST_DRX_IDX3);
	sys_reg_write(RX_BASE_PTR2, phys_to_bus((u32)ei_local->phy_rx_ring[2]));
	sys_reg_write(RX_MAX_CNT2, cpu_to_le32((u32)NUM_RSS_RX_DESC));
	sys_reg_write(RX_CALC_IDX2, cpu_to_le32((u32)(NUM_RSS_RX_DESC - 1)));
	sys_reg_write(PDMA_RST_CFG, PST_DRX_IDX2);
	sys_reg_write(RX_BASE_PTR1, phys_to_bus((u32)ei_local->phy_rx_ring[1]));
	sys_reg_write(RX_MAX_CNT1, cpu_to_le32((u32)NUM_RSS_RX_DESC));
	sys_reg_write(RX_CALC_IDX1, cpu_to_le32((u32)(NUM_RSS_RX_DESC - 1)));
	sys_reg_write(PDMA_RST_CFG, PST_DRX_IDX1);

	/* 1. Set RX ring1~3 to pse modes */
	SET_PDMA_RXRING_MODE(ADMA_RX_RING1, PDMA_RX_PSE_MODE);
	SET_PDMA_RXRING_MODE(ADMA_RX_RING2, PDMA_RX_PSE_MODE);
	SET_PDMA_RXRING_MODE(ADMA_RX_RING3, PDMA_RX_PSE_MODE);

	/* 2. Enable non-lro multiple rx */
	SET_PDMA_NON_LRO_MULTI_EN(1);  /* MRX EN */

	/*Hash Type*/
	SET_PDMA_RSS_IPV4_TYPE(7);
	SET_PDMA_RSS_IPV6_TYPE(7);
	/* 3. Select the size of indirection table */
	SET_PDMA_RSS_CR_VALUE(ADMA_RSS_INDR_TABLE_DW0, 0x39393939);
	SET_PDMA_RSS_CR_VALUE(ADMA_RSS_INDR_TABLE_DW1, 0x93939393);
	SET_PDMA_RSS_CR_VALUE(ADMA_RSS_INDR_TABLE_DW2, 0x39399393);
	SET_PDMA_RSS_CR_VALUE(ADMA_RSS_INDR_TABLE_DW3, 0x93933939);
	SET_PDMA_RSS_CR_VALUE(ADMA_RSS_INDR_TABLE_DW4, 0x39393939);
	SET_PDMA_RSS_CR_VALUE(ADMA_RSS_INDR_TABLE_DW5, 0x93939393);
	SET_PDMA_RSS_CR_VALUE(ADMA_RSS_INDR_TABLE_DW6, 0x39399393);
	SET_PDMA_RSS_CR_VALUE(ADMA_RSS_INDR_TABLE_DW7, 0x93933939);
	/* 4. Pause */
	SET_PDMA_RSS_CFG_REQ(1);

	/* 5. Enable RSS */
	SET_PDMA_RSS_EN(1);

	/* 6. Release pause */
	SET_PDMA_RSS_CFG_REQ(0);

	return 0;

no_rx_mem:
	return -ENOMEM;
}

void fe_rss_4ring_deinit(struct net_device *dev)
{
	struct END_DEVICE *ei_local = netdev_priv(dev);
	int i, j;

	for (i = 1; i < MAX_RX_RING_NUM; i++) {
		/* free RX Ring */
		dma_free_coherent(dev->dev.parent,
				  NUM_RSS_RX_DESC * sizeof(struct PDMA_rxdesc),
				  ei_local->rx_ring[i],
				  ei_local->phy_rx_ring[i]);
		/* free RX data */
		for (j = 0; j < NUM_RSS_RX_DESC; j++) {
			raeth_free_skb_data(ei_local->netrx_skb_data[i][j]);
			ei_local->netrx_skb_data[i][j] = NULL;
		}
	}
}

int fe_rss_2ring_init(struct net_device *dev)
{
	struct END_DEVICE *ei_local = netdev_priv(dev);
	int skb_size;
	int i, j;

	skb_size = SKB_DATA_ALIGN(MAX_RX_LENGTH + NET_IP_ALIGN) +
		   SKB_DATA_ALIGN(sizeof(struct skb_shared_info));

	for (i = 1; i < MAX_RX_RING_NUM_2RING; i++) {
		ei_local->rx_ring[i] =
			dma_alloc_coherent(dev->dev.parent,
					   NUM_RSS_RX_DESC *
					   sizeof(struct PDMA_rxdesc),
					   &ei_local->phy_rx_ring[i],
					   GFP_ATOMIC | __GFP_ZERO);
		for (j = 0; j < NUM_RSS_RX_DESC; j++) {
			ei_local->netrx_skb_data[i][j] =
				raeth_alloc_skb_data(skb_size, GFP_KERNEL);

			if (!ei_local->netrx_skb_data[i][j]) {
				pr_info("rx skbuff buffer allocation failed!\n");
				goto no_rx_mem;
			}

			memset(&ei_local->rx_ring[i][j], 0,
			       sizeof(struct PDMA_rxdesc));
			ei_local->rx_ring[i][j].rxd_info2.DDONE_bit = 0;
			ei_local->rx_ring[i][j].rxd_info2.LS0 = 0;
			ei_local->rx_ring[i][j].rxd_info2.PLEN0 =
			    SET_ADMA_RX_LEN0(MAX_RX_LENGTH);
			ei_local->rx_ring[i][j].rxd_info1.PDP0 =
			    dma_map_single(dev->dev.parent,
					   ei_local->netrx_skb_data[i][j] +
					   NET_SKB_PAD,
					   MAX_RX_LENGTH, DMA_FROM_DEVICE);
			if (unlikely
			    (dma_mapping_error
			     (dev->dev.parent,
			      ei_local->rx_ring[i][j].rxd_info1.PDP0))) {
				pr_info("[%s]dma_map_single() failed...\n",
					__func__);
				goto no_rx_mem;
			}
		}
		pr_info("\nphy_rx_ring[%d] = 0x%08x, rx_ring[%d] = 0x%p\n",
			i, (unsigned int)ei_local->phy_rx_ring[i],
			i, (void __iomem *)ei_local->rx_ring[i]);
	}

	sys_reg_write(RX_BASE_PTR1, phys_to_bus((u32)ei_local->phy_rx_ring[1]));
	sys_reg_write(RX_MAX_CNT1, cpu_to_le32((u32)NUM_RSS_RX_DESC));
	sys_reg_write(RX_CALC_IDX1, cpu_to_le32((u32)(NUM_RSS_RX_DESC - 1)));
	sys_reg_write(PDMA_RST_CFG, PST_DRX_IDX1);

	/* 1. Set RX ring1~3 to pse modes */
	SET_PDMA_RXRING_MODE(ADMA_RX_RING1, PDMA_RX_PSE_MODE);

	/* 2. Enable non-lro multiple rx */
	SET_PDMA_NON_LRO_MULTI_EN(1);  /* MRX EN */

	/*Hash Type*/
	SET_PDMA_RSS_IPV4_TYPE(7);
	SET_PDMA_RSS_IPV6_TYPE(7);
	/* 3. Select the size of indirection table */
	SET_PDMA_RSS_CR_VALUE(ADMA_RSS_INDR_TABLE_DW0, 0x44444444);
	SET_PDMA_RSS_CR_VALUE(ADMA_RSS_INDR_TABLE_DW1, 0x44444444);
	SET_PDMA_RSS_CR_VALUE(ADMA_RSS_INDR_TABLE_DW2, 0x44444444);
	SET_PDMA_RSS_CR_VALUE(ADMA_RSS_INDR_TABLE_DW3, 0x44444444);
	SET_PDMA_RSS_CR_VALUE(ADMA_RSS_INDR_TABLE_DW4, 0x44444444);
	SET_PDMA_RSS_CR_VALUE(ADMA_RSS_INDR_TABLE_DW5, 0x44444444);
	SET_PDMA_RSS_CR_VALUE(ADMA_RSS_INDR_TABLE_DW6, 0x44444444);
	SET_PDMA_RSS_CR_VALUE(ADMA_RSS_INDR_TABLE_DW7, 0x44444444);
	/* 4. Pause */
	SET_PDMA_RSS_CFG_REQ(1);

	/* 5. Enable RSS */
	SET_PDMA_RSS_EN(1);

	/* 6. Release pause */
	SET_PDMA_RSS_CFG_REQ(0);

	return 0;

no_rx_mem:
	return -ENOMEM;
}

void fe_rss_2ring_deinit(struct net_device *dev)
{
	struct END_DEVICE *ei_local = netdev_priv(dev);
	int i, j;

	for (i = 1; i < MAX_RX_RING_NUM_2RING; i++) {
		/* free RX Ring */
		dma_free_coherent(dev->dev.parent,
				  NUM_RSS_RX_DESC * sizeof(struct PDMA_rxdesc),
				  ei_local->rx_ring[i],
				  ei_local->phy_rx_ring[i]);
		/* free RX data */
		for (j = 0; j < NUM_RSS_RX_DESC; j++) {
			raeth_free_skb_data(ei_local->netrx_skb_data[i][j]);
			ei_local->netrx_skb_data[i][j] = NULL;
		}
	}
}

static inline void hw_rss_rx_desc_init(struct END_DEVICE *ei_local,
				       struct PDMA_rxdesc *rx_ring,
				       unsigned int rx_ring_no,
				       dma_addr_t dma_addr)
{
	rx_ring->rxd_info2.PLEN0 = MAX_RX_LENGTH;
	rx_ring->rxd_info1.PDP0 = dma_addr;
	rx_ring->rxd_info2.LS0 = 0;
	rx_ring->rxd_info2.DDONE_bit = 0;
}

static inline void __iomem *get_rx_cal_idx_reg(unsigned int rx_ring_no)
{
	return (void __iomem *)(RAETH_RX_CALC_IDX0 + (rx_ring_no << 4));
}

int fe_rss0_recv(struct net_device *dev,
		 struct napi_struct *napi,
		   int budget)
{
	struct END_DEVICE *ei_local = netdev_priv(dev);
	struct PSEUDO_ADAPTER *p_ad = netdev_priv(ei_local->pseudo_dev);
	struct sk_buff *rx_skb;
	struct PDMA_rxdesc *rx_ring, *rx_ring_next;
	void *rx_data, *rx_data_next, *new_data;
	unsigned int length = 0;
	unsigned int rx_ring_no = 0, rx_ring_no_next = 0;
	unsigned int rx_dma_owner_idx, rx_dma_owner_idx_next;
	unsigned int rx_dma_owner_lro[MAX_RX_RING_NUM];
	unsigned int skb_size, map_size;
	/* void __iomem *rx_calc_idx_reg; */
	int rx_processed = 0;

	/* get cpu owner indexes of rx rings */
	rx_dma_owner_lro[0] = (ei_local->rx_calc_idx[0] + 1) % num_rx_desc;

	rx_ring_no =  0;
	rx_dma_owner_idx = rx_dma_owner_lro[rx_ring_no];
	rx_ring = &ei_local->rx_ring[rx_ring_no][rx_dma_owner_idx];
	rx_data = ei_local->netrx_skb_data[rx_ring_no][rx_dma_owner_idx];
	/* rx_calc_idx_reg = get_rx_cal_idx_reg(rx_ring_no); */

	for (;;) {
		dma_addr_t dma_addr;

		if ((rx_processed++ > budget) ||
		    (rx_ring->rxd_info2.DDONE_bit == 0))
			break;

		/* prefetch the next handling RXD */

		rx_dma_owner_lro[rx_ring_no] =
				(rx_dma_owner_idx + 1) % num_rx_desc;
		skb_size =
			   SKB_DATA_ALIGN(MAX_RX_LENGTH + NET_IP_ALIGN +
					  NET_SKB_PAD) +
			  SKB_DATA_ALIGN(sizeof(struct skb_shared_info));
		map_size = MAX_RX_LENGTH;

		/* rx_ring_no_next =  get_rss_rx_ring(ei_local, rx_dma_owner_lro, group); */
		rx_ring_no_next =  rx_ring_no;
		rx_dma_owner_idx_next = rx_dma_owner_lro[rx_ring_no_next];

		rx_ring_next =
			&ei_local->rx_ring
				[rx_ring_no_next][rx_dma_owner_idx_next];
		rx_data_next =
			ei_local->netrx_skb_data
				[rx_ring_no_next][rx_dma_owner_idx_next];
		prefetch(rx_ring_next);

		/* We have to check the free memory size is big enough
		 * before pass the packet to cpu
		 */
		new_data = raeth_alloc_skb_data(skb_size, GFP_ATOMIC);

		if (unlikely(!new_data)) {
			pr_info("skb not available...\n");
			goto skb_err;
		}

		dma_addr = dma_map_single(dev->dev.parent,
					  new_data + NET_SKB_PAD,
					  map_size,
					  DMA_FROM_DEVICE);

		if (unlikely(dma_mapping_error(dev->dev.parent, dma_addr))) {
			pr_info("[%s]dma_map_single() failed...\n", __func__);
			raeth_free_skb_data(new_data);
			goto skb_err;
		}

		rx_skb = raeth_build_skb(rx_data, skb_size);

		if (unlikely(!rx_skb)) {
			put_page(virt_to_head_page(rx_data));
			pr_info("build_skb failed\n");
			goto skb_err;
		}
		skb_reserve(rx_skb, NET_SKB_PAD + NET_IP_ALIGN);

		length = rx_ring->rxd_info2.PLEN0;
		dma_unmap_single(dev->dev.parent,
				 rx_ring->rxd_info1.PDP0,
				 length, DMA_FROM_DEVICE);

		prefetch(rx_skb->data);

		/* skb processing */
		skb_put(rx_skb, length);

		/* rx packet from GE2 */
		if (rx_ring->rxd_info4.SP == 2) {
			if (ei_local->pseudo_dev) {
				rx_skb->dev = ei_local->pseudo_dev;
				rx_skb->protocol =
				    eth_type_trans(rx_skb,
						   ei_local->pseudo_dev);
			} else {
				pr_info
				    ("pseudo_dev is still not initialize ");
				pr_info
				    ("but receive packet from GMAC2\n");
			}
		} else {
			rx_skb->dev = dev;
			rx_skb->protocol = eth_type_trans(rx_skb, dev);
		}

		/* rx checksum offload */
		if (likely(rx_ring->rxd_info4.L4VLD))
			rx_skb->ip_summed = CHECKSUM_UNNECESSARY;
		else
			rx_skb->ip_summed = CHECKSUM_NONE;

#if defined(CONFIG_RA_HW_NAT)  || defined(CONFIG_RA_HW_NAT_MODULE)
		if (ra_sw_nat_hook_rx) {
			if (IS_SPACE_AVAILABLE_HEAD(rx_skb)) {
				*(uint32_t *)(FOE_INFO_START_ADDR_HEAD(rx_skb)) =
					*(uint32_t *)&rx_ring->rxd_info4;
				FOE_ALG_HEAD(rx_skb) = 0;
				FOE_MAGIC_TAG_HEAD(rx_skb) = FOE_MAGIC_GE;
				FOE_TAG_PROTECT_HEAD(rx_skb) = TAG_PROTECT;
			}
			if (IS_SPACE_AVAILABLE_TAIL(rx_skb)) {
				*(uint32_t *)(FOE_INFO_START_ADDR_TAIL(rx_skb) + 2) =
					*(uint32_t *)&rx_ring->rxd_info4;
				FOE_ALG_TAIL(rx_skb) = 0;
				FOE_MAGIC_TAG_TAIL(rx_skb) = FOE_MAGIC_GE;
				FOE_TAG_PROTECT_TAIL(rx_skb) = TAG_PROTECT;
			}
		}
#endif
		if (ei_local->features & FE_HW_VLAN_RX) {
			if (rx_ring->rxd_info2.TAG)
				__vlan_hwaccel_put_tag(rx_skb,
						       htons(ETH_P_8021Q),
						       rx_ring->rxd_info3.VID);
		}
/* ra_sw_nat_hook_rx return 1 --> continue
 * ra_sw_nat_hook_rx return 0 --> FWD & without netif_rx
 */
#if defined(CONFIG_RA_HW_NAT)  || defined(CONFIG_RA_HW_NAT_MODULE)
		if ((!ra_sw_nat_hook_rx) ||
		    (ra_sw_nat_hook_rx && ra_sw_nat_hook_rx(rx_skb))) {
#endif
			if (ei_local->features & FE_INT_NAPI) {
			/* napi_gro_receive(napi, rx_skb); */
				netif_receive_skb(rx_skb);
			} else {
				netif_rx(rx_skb);
			}

#if defined(CONFIG_RA_HW_NAT)  || defined(CONFIG_RA_HW_NAT_MODULE)
		}
#endif

		if (rx_ring->rxd_info4.SP == 2) {
			p_ad->stat.rx_packets++;
			p_ad->stat.rx_bytes += length;
		} else {
			ei_local->stat.rx_packets++;
			ei_local->stat.rx_bytes += length;
		}

		/* Init RX desc. */
		hw_rss_rx_desc_init(ei_local,
				    rx_ring,
				    rx_ring_no,
				    dma_addr);
		ei_local->netrx_skb_data[rx_ring_no][rx_dma_owner_idx] =
			new_data;

		/* make sure that all changes to the dma ring are flushed before
		  * we continue
		  */
		wmb();
		sys_reg_write(RAETH_RX_CALC_IDX0, rx_dma_owner_idx);
		ei_local->rx_calc_idx[rx_ring_no] = rx_dma_owner_idx;

		/* use prefetched variable */
		rx_dma_owner_idx = rx_dma_owner_idx_next;
		rx_ring_no = rx_ring_no_next;
		rx_ring = rx_ring_next;
		rx_data = rx_data_next;
		/* rx_calc_idx_reg = get_rx_cal_idx_reg(rx_ring_no); */
	}	/* for */

	return rx_processed;

skb_err:
	/* rx packet from GE2 */
	if (rx_ring->rxd_info4.SP == 2)
		p_ad->stat.rx_dropped++;
	else
		ei_local->stat.rx_dropped++;

	/* Discard the rx packet */
	hw_rss_rx_desc_init(ei_local,
			    rx_ring,
			    rx_ring_no,
			    rx_ring->rxd_info1.PDP0);
	sys_reg_write(RAETH_RX_CALC_IDX0, rx_dma_owner_idx);
	ei_local->rx_calc_idx[rx_ring_no] = rx_dma_owner_idx;

	return (budget + 1);
}

int fe_rss1_recv(struct net_device *dev,
		 struct napi_struct *napi,
		   int budget)
{
	struct END_DEVICE *ei_local = netdev_priv(dev);
	struct PSEUDO_ADAPTER *p_ad = netdev_priv(ei_local->pseudo_dev);
	struct sk_buff *rx_skb;
	struct PDMA_rxdesc *rx_ring, *rx_ring_next;
	void *rx_data, *rx_data_next, *new_data;
	unsigned int length = 0;
	unsigned int rx_ring_no = 0, rx_ring_no_next = 0;
	unsigned int rx_dma_owner_idx, rx_dma_owner_idx_next;
	unsigned int rx_dma_owner_lro[MAX_RX_RING_NUM];
	unsigned int skb_size, map_size;
	/* void __iomem *rx_calc_idx_reg; */
	int rx_processed = 0;

	/* get cpu owner indexes of rx rings */
	rx_dma_owner_lro[1] = (ei_local->rx_calc_idx[1] + 1) % NUM_RSS_RX_DESC;

	rx_ring_no = 1;
	rx_dma_owner_idx = rx_dma_owner_lro[rx_ring_no];
	rx_ring = &ei_local->rx_ring[rx_ring_no][rx_dma_owner_idx];
	rx_data = ei_local->netrx_skb_data[rx_ring_no][rx_dma_owner_idx];
	/* rx_calc_idx_reg = get_rx_cal_idx_reg(rx_ring_no); */

	for (;;) {
		dma_addr_t dma_addr;

		if ((rx_processed++ > budget) ||
		    (rx_ring->rxd_info2.DDONE_bit == 0))
			break;

		/* prefetch the next handling RXD */

		rx_dma_owner_lro[rx_ring_no] =
				(rx_dma_owner_idx + 1) % NUM_RSS_RX_DESC;
		skb_size =
			   SKB_DATA_ALIGN(MAX_RX_LENGTH + NET_IP_ALIGN +
					  NET_SKB_PAD) +
			  SKB_DATA_ALIGN(sizeof(struct skb_shared_info));
		map_size = MAX_RX_LENGTH;

		/* rx_ring_no_next =  get_rss_rx_ring(ei_local, rx_dma_owner_lro, group); */
		rx_ring_no_next =  rx_ring_no;
		rx_dma_owner_idx_next = rx_dma_owner_lro[rx_ring_no_next];

		rx_ring_next =
			&ei_local->rx_ring
				[rx_ring_no_next][rx_dma_owner_idx_next];
		rx_data_next =
			ei_local->netrx_skb_data
				[rx_ring_no_next][rx_dma_owner_idx_next];
		prefetch(rx_ring_next);

		/* We have to check the free memory size is big enough
		 * before pass the packet to cpu
		 */
		new_data = raeth_alloc_skb_data(skb_size, GFP_ATOMIC);

		if (unlikely(!new_data)) {
			pr_info("skb not available...\n");
			goto skb_err;
		}

		dma_addr = dma_map_single(dev->dev.parent,
					  new_data + NET_SKB_PAD,
					  map_size,
					  DMA_FROM_DEVICE);

		if (unlikely(dma_mapping_error(dev->dev.parent, dma_addr))) {
			pr_info("[%s]dma_map_single() failed...\n", __func__);
			raeth_free_skb_data(new_data);
			goto skb_err;
		}

		rx_skb = raeth_build_skb(rx_data, skb_size);

		if (unlikely(!rx_skb)) {
			put_page(virt_to_head_page(rx_data));
			pr_info("build_skb failed\n");
			goto skb_err;
		}
		skb_reserve(rx_skb, NET_SKB_PAD + NET_IP_ALIGN);

		length = rx_ring->rxd_info2.PLEN0;
		dma_unmap_single(dev->dev.parent,
				 rx_ring->rxd_info1.PDP0,
				 length, DMA_FROM_DEVICE);

		prefetch(rx_skb->data);

		/* skb processing */
		skb_put(rx_skb, length);

		/* rx packet from GE2 */
		if (rx_ring->rxd_info4.SP == 2) {
			if (ei_local->pseudo_dev) {
				rx_skb->dev = ei_local->pseudo_dev;
				rx_skb->protocol =
				    eth_type_trans(rx_skb,
						   ei_local->pseudo_dev);
			} else {
				pr_info
				    ("pseudo_dev is still not initialize ");
				pr_info
				    ("but receive packet from GMAC2\n");
			}
		} else {
			rx_skb->dev = dev;
			rx_skb->protocol = eth_type_trans(rx_skb, dev);
		}

		/* rx checksum offload */
		if (likely(rx_ring->rxd_info4.L4VLD))
			rx_skb->ip_summed = CHECKSUM_UNNECESSARY;
		else
			rx_skb->ip_summed = CHECKSUM_NONE;

#if defined(CONFIG_RA_HW_NAT)  || defined(CONFIG_RA_HW_NAT_MODULE)
		if (ra_sw_nat_hook_rx) {
			*(uint32_t *)(FOE_INFO_START_ADDR_HEAD(rx_skb)) =
				*(uint32_t *)&rx_ring->rxd_info4;
			*(uint32_t *)(FOE_INFO_START_ADDR_TAIL(rx_skb) + 2) =
				*(uint32_t *)&rx_ring->rxd_info4;
			FOE_ALG_HEAD(rx_skb) = 0;
			FOE_ALG_TAIL(rx_skb) = 0;
			FOE_MAGIC_TAG_HEAD(rx_skb) = FOE_MAGIC_GE;
			FOE_MAGIC_TAG_TAIL(rx_skb) = FOE_MAGIC_GE;
			FOE_TAG_PROTECT_HEAD(rx_skb) = TAG_PROTECT;
			FOE_TAG_PROTECT_TAIL(rx_skb) = TAG_PROTECT;
		}
#endif
		if (ei_local->features & FE_HW_VLAN_RX) {
			if (rx_ring->rxd_info2.TAG)
				__vlan_hwaccel_put_tag(rx_skb,
						       htons(ETH_P_8021Q),
						       rx_ring->rxd_info3.VID);
		}
/* ra_sw_nat_hook_rx return 1 --> continue
 * ra_sw_nat_hook_rx return 0 --> FWD & without netif_rx
 */
#if defined(CONFIG_RA_HW_NAT)  || defined(CONFIG_RA_HW_NAT_MODULE)
		if ((!ra_sw_nat_hook_rx) ||
		    (ra_sw_nat_hook_rx && ra_sw_nat_hook_rx(rx_skb))) {
#endif
			if (ei_local->features & FE_INT_NAPI) {
			/* napi_gro_receive(napi, rx_skb); */
				netif_receive_skb(rx_skb);
			} else {
				netif_rx(rx_skb);
			}

#if defined(CONFIG_RA_HW_NAT)  || defined(CONFIG_RA_HW_NAT_MODULE)
		}
#endif

		if (rx_ring->rxd_info4.SP == 2) {
			p_ad->stat.rx_packets++;
			p_ad->stat.rx_bytes += length;
		} else {
			ei_local->stat.rx_packets++;
			ei_local->stat.rx_bytes += length;
		}

		/* Init RX desc. */
		hw_rss_rx_desc_init(ei_local,
				    rx_ring,
				    rx_ring_no,
				    dma_addr);
		ei_local->netrx_skb_data[rx_ring_no][rx_dma_owner_idx] =
			new_data;

		/* make sure that all changes to the dma ring are flushed before
		  * we continue
		  */
		wmb();
		sys_reg_write(RAETH_RX_CALC_IDX1, rx_dma_owner_idx);
		ei_local->rx_calc_idx[rx_ring_no] = rx_dma_owner_idx;

		/* use prefetched variable */
		rx_dma_owner_idx = rx_dma_owner_idx_next;
		rx_ring_no = rx_ring_no_next;
		rx_ring = rx_ring_next;
		rx_data = rx_data_next;
		/* rx_calc_idx_reg = get_rx_cal_idx_reg(rx_ring_no); */
	}	/* for */

	return rx_processed;

skb_err:
	/* rx packet from GE2 */
	if (rx_ring->rxd_info4.SP == 2)
		p_ad->stat.rx_dropped++;
	else
		ei_local->stat.rx_dropped++;

	/* Discard the rx packet */
	hw_rss_rx_desc_init(ei_local,
			    rx_ring,
			    rx_ring_no,
			    rx_ring->rxd_info1.PDP0);
	sys_reg_write(RAETH_RX_CALC_IDX1, rx_dma_owner_idx);
	ei_local->rx_calc_idx[rx_ring_no] = rx_dma_owner_idx;

	return (budget + 1);
}

int fe_rss2_recv(struct net_device *dev,
		 struct napi_struct *napi,
		   int budget)
{
	struct END_DEVICE *ei_local = netdev_priv(dev);
	struct PSEUDO_ADAPTER *p_ad = netdev_priv(ei_local->pseudo_dev);
	struct sk_buff *rx_skb;
	struct PDMA_rxdesc *rx_ring, *rx_ring_next;
	void *rx_data, *rx_data_next, *new_data;
	unsigned int length = 0;
	unsigned int rx_ring_no = 0, rx_ring_no_next = 0;
	unsigned int rx_dma_owner_idx, rx_dma_owner_idx_next;
	unsigned int rx_dma_owner_lro[MAX_RX_RING_NUM];
	unsigned int skb_size, map_size;
	/* void __iomem *rx_calc_idx_reg; */
	int rx_processed = 0;

	/* get cpu owner indexes of rx rings */
	rx_dma_owner_lro[2] = (ei_local->rx_calc_idx[2] + 1) % NUM_RSS_RX_DESC;

	rx_ring_no =  2;
	rx_dma_owner_idx = rx_dma_owner_lro[rx_ring_no];
	rx_ring = &ei_local->rx_ring[rx_ring_no][rx_dma_owner_idx];
	rx_data = ei_local->netrx_skb_data[rx_ring_no][rx_dma_owner_idx];
	/* rx_calc_idx_reg = get_rx_cal_idx_reg(rx_ring_no); */

	for (;;) {
		dma_addr_t dma_addr;

		if ((rx_processed++ > budget) ||
		    (rx_ring->rxd_info2.DDONE_bit == 0))
			break;

		/* prefetch the next handling RXD */

		rx_dma_owner_lro[rx_ring_no] =
				(rx_dma_owner_idx + 1) % NUM_RSS_RX_DESC;
		skb_size =
			   SKB_DATA_ALIGN(MAX_RX_LENGTH + NET_IP_ALIGN +
					  NET_SKB_PAD) +
			  SKB_DATA_ALIGN(sizeof(struct skb_shared_info));
		map_size = MAX_RX_LENGTH;

		/* rx_ring_no_next =  get_rss_rx_ring(ei_local, rx_dma_owner_lro, group); */
		rx_ring_no_next =  rx_ring_no;
		rx_dma_owner_idx_next = rx_dma_owner_lro[rx_ring_no_next];

		rx_ring_next =
			&ei_local->rx_ring
				[rx_ring_no_next][rx_dma_owner_idx_next];
		rx_data_next =
			ei_local->netrx_skb_data
				[rx_ring_no_next][rx_dma_owner_idx_next];
		prefetch(rx_ring_next);

		/* We have to check the free memory size is big enough
		 * before pass the packet to cpu
		 */
		new_data = raeth_alloc_skb_data(skb_size, GFP_ATOMIC);

		if (unlikely(!new_data)) {
			pr_info("skb not available...\n");
			goto skb_err;
		}

		dma_addr = dma_map_single(dev->dev.parent,
					  new_data + NET_SKB_PAD,
					  map_size,
					  DMA_FROM_DEVICE);

		if (unlikely(dma_mapping_error(dev->dev.parent, dma_addr))) {
			pr_info("[%s]dma_map_single() failed...\n", __func__);
			raeth_free_skb_data(new_data);
			goto skb_err;
		}

		rx_skb = raeth_build_skb(rx_data, skb_size);

		if (unlikely(!rx_skb)) {
			put_page(virt_to_head_page(rx_data));
			pr_info("build_skb failed\n");
			goto skb_err;
		}
		skb_reserve(rx_skb, NET_SKB_PAD + NET_IP_ALIGN);

		length = rx_ring->rxd_info2.PLEN0;
		dma_unmap_single(dev->dev.parent,
				 rx_ring->rxd_info1.PDP0,
				 length, DMA_FROM_DEVICE);

		prefetch(rx_skb->data);

		/* skb processing */
		skb_put(rx_skb, length);

		/* rx packet from GE2 */
		if (rx_ring->rxd_info4.SP == 2) {
			if (ei_local->pseudo_dev) {
				rx_skb->dev = ei_local->pseudo_dev;
				rx_skb->protocol =
				    eth_type_trans(rx_skb,
						   ei_local->pseudo_dev);
			} else {
				pr_info
				    ("pseudo_dev is still not initialize ");
				pr_info
				    ("but receive packet from GMAC2\n");
			}
		} else {
			rx_skb->dev = dev;
			rx_skb->protocol = eth_type_trans(rx_skb, dev);
		}

		/* rx checksum offload */
		if (likely(rx_ring->rxd_info4.L4VLD))
			rx_skb->ip_summed = CHECKSUM_UNNECESSARY;
		else
			rx_skb->ip_summed = CHECKSUM_NONE;

#if defined(CONFIG_RA_HW_NAT)  || defined(CONFIG_RA_HW_NAT_MODULE)
		if (ra_sw_nat_hook_rx) {
			*(uint32_t *)(FOE_INFO_START_ADDR_HEAD(rx_skb)) =
				*(uint32_t *)&rx_ring->rxd_info4;
			*(uint32_t *)(FOE_INFO_START_ADDR_TAIL(rx_skb) + 2) =
				*(uint32_t *)&rx_ring->rxd_info4;
			FOE_ALG_HEAD(rx_skb) = 0;
			FOE_ALG_TAIL(rx_skb) = 0;
			FOE_MAGIC_TAG_HEAD(rx_skb) = FOE_MAGIC_GE;
			FOE_MAGIC_TAG_TAIL(rx_skb) = FOE_MAGIC_GE;
			FOE_TAG_PROTECT_HEAD(rx_skb) = TAG_PROTECT;
			FOE_TAG_PROTECT_TAIL(rx_skb) = TAG_PROTECT;
		}
#endif
		if (ei_local->features & FE_HW_VLAN_RX) {
			if (rx_ring->rxd_info2.TAG)
				__vlan_hwaccel_put_tag(rx_skb,
						       htons(ETH_P_8021Q),
						       rx_ring->rxd_info3.VID);
		}
/* ra_sw_nat_hook_rx return 1 --> continue
 * ra_sw_nat_hook_rx return 0 --> FWD & without netif_rx
 */
#if defined(CONFIG_RA_HW_NAT)  || defined(CONFIG_RA_HW_NAT_MODULE)
		if ((!ra_sw_nat_hook_rx) ||
		    (ra_sw_nat_hook_rx && ra_sw_nat_hook_rx(rx_skb))) {
#endif
			if (ei_local->features & FE_INT_NAPI) {
			/* napi_gro_receive(napi, rx_skb); */
				netif_receive_skb(rx_skb);
			} else {
				netif_rx(rx_skb);
			}

#if defined(CONFIG_RA_HW_NAT)  || defined(CONFIG_RA_HW_NAT_MODULE)
		}
#endif

		if (rx_ring->rxd_info4.SP == 2) {
			p_ad->stat.rx_packets++;
			p_ad->stat.rx_bytes += length;
		} else {
			ei_local->stat.rx_packets++;
			ei_local->stat.rx_bytes += length;
		}

		/* Init RX desc. */
		hw_rss_rx_desc_init(ei_local,
				    rx_ring,
				    rx_ring_no,
				    dma_addr);
		ei_local->netrx_skb_data[rx_ring_no][rx_dma_owner_idx] =
			new_data;

		/* make sure that all changes to the dma ring are flushed before
		  * we continue
		  */
		wmb();

		sys_reg_write(RAETH_RX_CALC_IDX2, rx_dma_owner_idx);
		ei_local->rx_calc_idx[rx_ring_no] = rx_dma_owner_idx;

		/* use prefetched variable */
		rx_dma_owner_idx = rx_dma_owner_idx_next;
		rx_ring_no = rx_ring_no_next;
		rx_ring = rx_ring_next;
		rx_data = rx_data_next;
		/* rx_calc_idx_reg = get_rx_cal_idx_reg(rx_ring_no); */
	}	/* for */

	return rx_processed;

skb_err:
	/* rx packet from GE2 */
	if (rx_ring->rxd_info4.SP == 2)
		p_ad->stat.rx_dropped++;
	else
		ei_local->stat.rx_dropped++;

	/* Discard the rx packet */
	hw_rss_rx_desc_init(ei_local,
			    rx_ring,
			    rx_ring_no,
			    rx_ring->rxd_info1.PDP0);
	sys_reg_write(RAETH_RX_CALC_IDX2, rx_dma_owner_idx);
	ei_local->rx_calc_idx[rx_ring_no] = rx_dma_owner_idx;

	return (budget + 1);
}

int fe_rss3_recv(struct net_device *dev,
		 struct napi_struct *napi,
		   int budget)
{
	struct END_DEVICE *ei_local = netdev_priv(dev);
	struct PSEUDO_ADAPTER *p_ad = netdev_priv(ei_local->pseudo_dev);
	struct sk_buff *rx_skb;
	struct PDMA_rxdesc *rx_ring, *rx_ring_next;
	void *rx_data, *rx_data_next, *new_data;
	unsigned int length = 0;
	unsigned int rx_ring_no = 0, rx_ring_no_next = 0;
	unsigned int rx_dma_owner_idx, rx_dma_owner_idx_next;
	unsigned int rx_dma_owner_lro[MAX_RX_RING_NUM];
	unsigned int skb_size, map_size;
	/* void __iomem *rx_calc_idx_reg; */
	int rx_processed = 0;

	/* get cpu owner indexes of rx rings */
	rx_dma_owner_lro[3] = (ei_local->rx_calc_idx[3] + 1) % NUM_RSS_RX_DESC;
	rx_ring_no =  3;
	rx_dma_owner_idx = rx_dma_owner_lro[rx_ring_no];
	rx_ring = &ei_local->rx_ring[rx_ring_no][rx_dma_owner_idx];
	rx_data = ei_local->netrx_skb_data[rx_ring_no][rx_dma_owner_idx];
	/* rx_calc_idx_reg = get_rx_cal_idx_reg(rx_ring_no); */

	for (;;) {
		dma_addr_t dma_addr;

		if ((rx_processed++ > budget) ||
		    (rx_ring->rxd_info2.DDONE_bit == 0))
			break;

		/* prefetch the next handling RXD */

		rx_dma_owner_lro[rx_ring_no] =
				(rx_dma_owner_idx + 1) % NUM_RSS_RX_DESC;
		skb_size =
			   SKB_DATA_ALIGN(MAX_RX_LENGTH + NET_IP_ALIGN +
					  NET_SKB_PAD) +
			  SKB_DATA_ALIGN(sizeof(struct skb_shared_info));
		map_size = MAX_RX_LENGTH;

		/* rx_ring_no_next =  get_rss_rx_ring(ei_local, rx_dma_owner_lro, group); */
		rx_ring_no_next =  rx_ring_no;
		rx_dma_owner_idx_next = rx_dma_owner_lro[rx_ring_no_next];

		rx_ring_next =
			&ei_local->rx_ring
				[rx_ring_no_next][rx_dma_owner_idx_next];
		rx_data_next =
			ei_local->netrx_skb_data
				[rx_ring_no_next][rx_dma_owner_idx_next];
		prefetch(rx_ring_next);

		/* We have to check the free memory size is big enough
		 * before pass the packet to cpu
		 */
		new_data = raeth_alloc_skb_data(skb_size, GFP_ATOMIC);

		if (unlikely(!new_data)) {
			pr_info("skb not available...\n");
			goto skb_err;
		}

		dma_addr = dma_map_single(dev->dev.parent,
					  new_data + NET_SKB_PAD,
					  map_size,
					  DMA_FROM_DEVICE);

		if (unlikely(dma_mapping_error(dev->dev.parent, dma_addr))) {
			pr_info("[%s]dma_map_single() failed...\n", __func__);
			raeth_free_skb_data(new_data);
			goto skb_err;
		}

		rx_skb = raeth_build_skb(rx_data, skb_size);

		if (unlikely(!rx_skb)) {
			put_page(virt_to_head_page(rx_data));
			pr_info("build_skb failed\n");
			goto skb_err;
		}
		skb_reserve(rx_skb, NET_SKB_PAD + NET_IP_ALIGN);

		length = rx_ring->rxd_info2.PLEN0;
		dma_unmap_single(dev->dev.parent,
				 rx_ring->rxd_info1.PDP0,
				 length, DMA_FROM_DEVICE);

		prefetch(rx_skb->data);

		/* skb processing */
		skb_put(rx_skb, length);

		/* rx packet from GE2 */
		if (rx_ring->rxd_info4.SP == 2) {
			if (ei_local->pseudo_dev) {
				rx_skb->dev = ei_local->pseudo_dev;
				rx_skb->protocol =
				    eth_type_trans(rx_skb,
						   ei_local->pseudo_dev);
			} else {
				pr_info
				    ("pseudo_dev is still not initialize ");
				pr_info
				    ("but receive packet from GMAC2\n");
			}
		} else {
			rx_skb->dev = dev;
			rx_skb->protocol = eth_type_trans(rx_skb, dev);
		}

		/* rx checksum offload */
		if (likely(rx_ring->rxd_info4.L4VLD))
			rx_skb->ip_summed = CHECKSUM_UNNECESSARY;
		else
			rx_skb->ip_summed = CHECKSUM_NONE;

#if defined(CONFIG_RA_HW_NAT)  || defined(CONFIG_RA_HW_NAT_MODULE)
		if (ra_sw_nat_hook_rx) {
			*(uint32_t *)(FOE_INFO_START_ADDR_HEAD(rx_skb)) =
				*(uint32_t *)&rx_ring->rxd_info4;
			*(uint32_t *)(FOE_INFO_START_ADDR_TAIL(rx_skb) + 2) =
				*(uint32_t *)&rx_ring->rxd_info4;
			FOE_ALG_HEAD(rx_skb) = 0;
			FOE_ALG_TAIL(rx_skb) = 0;
			FOE_MAGIC_TAG_HEAD(rx_skb) = FOE_MAGIC_GE;
			FOE_MAGIC_TAG_TAIL(rx_skb) = FOE_MAGIC_GE;
			FOE_TAG_PROTECT_HEAD(rx_skb) = TAG_PROTECT;
			FOE_TAG_PROTECT_TAIL(rx_skb) = TAG_PROTECT;
		}
#endif
		if (ei_local->features & FE_HW_VLAN_RX) {
			if (rx_ring->rxd_info2.TAG)
				__vlan_hwaccel_put_tag(rx_skb,
						       htons(ETH_P_8021Q),
						       rx_ring->rxd_info3.VID);
		}
/* ra_sw_nat_hook_rx return 1 --> continue
 * ra_sw_nat_hook_rx return 0 --> FWD & without netif_rx
 */
#if defined(CONFIG_RA_HW_NAT)  || defined(CONFIG_RA_HW_NAT_MODULE)
		if ((!ra_sw_nat_hook_rx) ||
		    (ra_sw_nat_hook_rx && ra_sw_nat_hook_rx(rx_skb))) {
#endif
			if (ei_local->features & FE_INT_NAPI) {
			/* napi_gro_receive(napi, rx_skb); */
				netif_receive_skb(rx_skb);
			} else {
				netif_rx(rx_skb);
			}

#if defined(CONFIG_RA_HW_NAT)  || defined(CONFIG_RA_HW_NAT_MODULE)
		}
#endif

		if (rx_ring->rxd_info4.SP == 2) {
			p_ad->stat.rx_packets++;
			p_ad->stat.rx_bytes += length;
		} else {
			ei_local->stat.rx_packets++;
			ei_local->stat.rx_bytes += length;
		}

		/* Init RX desc. */
		hw_rss_rx_desc_init(ei_local,
				    rx_ring,
				    rx_ring_no,
				    dma_addr);
		ei_local->netrx_skb_data[rx_ring_no][rx_dma_owner_idx] =
			new_data;

		/* make sure that all changes to the dma ring are flushed before
		  * we continue
		  */
		wmb();

		sys_reg_write(RAETH_RX_CALC_IDX3, rx_dma_owner_idx);
		ei_local->rx_calc_idx[rx_ring_no] = rx_dma_owner_idx;

		/* use prefetched variable */
		rx_dma_owner_idx = rx_dma_owner_idx_next;
		rx_ring_no = rx_ring_no_next;
		rx_ring = rx_ring_next;
		rx_data = rx_data_next;
		/* rx_calc_idx_reg = get_rx_cal_idx_reg(rx_ring_no); */
	}	/* for */

	return rx_processed;

skb_err:
	/* rx packet from GE2 */
	if (rx_ring->rxd_info4.SP == 2)
		p_ad->stat.rx_dropped++;
	else
		ei_local->stat.rx_dropped++;

	/* Discard the rx packet */
	hw_rss_rx_desc_init(ei_local,
			    rx_ring,
			    rx_ring_no,
			    rx_ring->rxd_info1.PDP0);
	sys_reg_write(RAETH_RX_CALC_IDX3, rx_dma_owner_idx);
	ei_local->rx_calc_idx[rx_ring_no] = rx_dma_owner_idx;

	return (budget + 1);
}

int rx_rss_ring_read(struct seq_file *seq, void *v,
		     struct PDMA_rxdesc *rx_ring_p)
{
	struct PDMA_rxdesc *rx_ring;
	int i = 0;

	rx_ring =
	    kmalloc(sizeof(struct PDMA_rxdesc) * NUM_RSS_RX_DESC, GFP_KERNEL);
	if (!rx_ring) {
		seq_puts(seq, " allocate temp rx_ring fail.\n");
		return 0;
	}

	for (i = 0; i < NUM_RSS_RX_DESC; i++)
		memcpy(&rx_ring[i], &rx_ring_p[i], sizeof(struct PDMA_rxdesc));

	for (i = 0; i < NUM_RSS_RX_DESC; i++) {
		seq_printf(seq, "%d: %08x %08x %08x %08x\n", i,
			   *(int *)&rx_ring[i].rxd_info1,
			   *(int *)&rx_ring[i].rxd_info2,
			   *(int *)&rx_ring[i].rxd_info3,
			   *(int *)&rx_ring[i].rxd_info4);
	}

	kfree(rx_ring);
	return 0;
}

int rss_ring1_read(struct seq_file *seq, void *v)
{
	struct END_DEVICE *ei_local = netdev_priv(dev_raether);

	rx_rss_ring_read(seq, v, ei_local->rx_ring[1]);

	return 0;
}

int rss_ring2_read(struct seq_file *seq, void *v)
{
	struct END_DEVICE *ei_local = netdev_priv(dev_raether);

	rx_rss_ring_read(seq, v, ei_local->rx_ring[2]);

	return 0;
}

int rss_ring3_read(struct seq_file *seq, void *v)
{
	struct END_DEVICE *ei_local = netdev_priv(dev_raether);

	rx_rss_ring_read(seq, v, ei_local->rx_ring[3]);

	return 0;
}

static int rx_ring1_open(struct inode *inode, struct file *file)
{
	return single_open(file, rss_ring1_read, NULL);
}

static int rx_ring2_open(struct inode *inode, struct file *file)
{
	return single_open(file, rss_ring2_read, NULL);
}

static int rx_ring3_open(struct inode *inode, struct file *file)
{
	return single_open(file, rss_ring3_read, NULL);
}

static const struct file_operations rss_ring1_fops = {
	.owner = THIS_MODULE,
	.open = rx_ring1_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release
};

static const struct file_operations rss_ring2_fops = {
	.owner = THIS_MODULE,
	.open = rx_ring2_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release
};

static const struct file_operations rss_ring3_fops = {
	.owner = THIS_MODULE,
	.open = rx_ring3_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release
};

int rss_debug_proc_init(struct proc_dir_entry *proc_reg_dir)
{
	proc_rss_ring1 =
	     proc_create(PROCREG_RXRING1, 0, proc_reg_dir, &rss_ring1_fops);
	if (!proc_rss_ring1)
		pr_info("!! FAIL to create %s PROC !!\n", PROCREG_RXRING1);

	proc_rss_ring2 =
	     proc_create(PROCREG_RXRING2, 0, proc_reg_dir, &rss_ring2_fops);
	if (!proc_rss_ring2)
		pr_info("!! FAIL to create %s PROC !!\n", PROCREG_RXRING2);

	proc_rss_ring3 =
	     proc_create(PROCREG_RXRING3, 0, proc_reg_dir, &rss_ring3_fops);
	if (!proc_rss_ring3)
		pr_info("!! FAIL to create %s PROC !!\n", PROCREG_RXRING3);

	return 0;
}
EXPORT_SYMBOL(rss_debug_proc_init);

void rss_debug_proc_exit(struct proc_dir_entry *proc_reg_dir)
{
	if (proc_rss_ring1)
		remove_proc_entry(PROCREG_RXRING1, proc_reg_dir);
	if (proc_rss_ring2)
		remove_proc_entry(PROCREG_RXRING2, proc_reg_dir);
	if (proc_rss_ring3)
		remove_proc_entry(PROCREG_RXRING3, proc_reg_dir);
}
EXPORT_SYMBOL(rss_debug_proc_exit);
