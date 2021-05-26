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
#include "ra_mac.h"

/* HW LRO Force port */
int set_fe_lro_ring1_cfg(struct net_device *dev)
{
	unsigned int ip;

	pr_debug("set_fe_lro_ring1_cfg()\n");

	/* 1. Set RX ring mode to force port */
	SET_PDMA_RXRING_MODE(ADMA_RX_RING1, PDMA_RX_FORCE_PORT);

	/* 2. Configure lro ring */
	/* 2.1 set src/destination TCP ports */
	SET_PDMA_RXRING_TCP_SRC_PORT(ADMA_RX_RING1, 1122);
	SET_PDMA_RXRING_TCP_DEST_PORT(ADMA_RX_RING1, 3344);
	/* 2.2 set src/destination IPs */
	str_to_ip(&ip, "10.10.10.3");
	sys_reg_write(LRO_RX_RING1_SIP_DW0, ip);
	str_to_ip(&ip, "10.10.10.254");
	sys_reg_write(LRO_RX_RING1_DIP_DW0, ip);
	/* 2.3 IPv4 force port mode */
	SET_PDMA_RXRING_IPV4_FORCE_MODE(ADMA_RX_RING1, 1);
	/* 2.4 IPv6 force port mode */
	SET_PDMA_RXRING_IPV6_FORCE_MODE(ADMA_RX_RING1, 1);

	/* 3. Set Age timer: 10 msec. */
	SET_PDMA_RXRING_AGE_TIME(ADMA_RX_RING1, HW_LRO_AGE_TIME);

	/* 4. Valid LRO ring */
	SET_PDMA_RXRING_VALID(ADMA_RX_RING1, 1);

	return 0;
}

int set_fe_lro_ring2_cfg(struct net_device *dev)
{
	unsigned int ip;

	pr_debug("set_fe_lro_ring2_cfg()\n");

	/* 1. Set RX ring mode to force port */
	SET_PDMA_RXRING_MODE(ADMA_RX_RING2, PDMA_RX_FORCE_PORT);

	/* 2. Configure lro ring */
	/* 2.1 set src/destination TCP ports */
	SET_PDMA_RXRING_TCP_SRC_PORT(ADMA_RX_RING2, 5566);
	SET_PDMA_RXRING_TCP_DEST_PORT(ADMA_RX_RING2, 7788);
	/* 2.2 set src/destination IPs */
	str_to_ip(&ip, "10.10.10.3");
	sys_reg_write(LRO_RX_RING2_SIP_DW0, ip);
	str_to_ip(&ip, "10.10.10.254");
	sys_reg_write(LRO_RX_RING2_DIP_DW0, ip);
	/* 2.3 IPv4 force port mode */
	SET_PDMA_RXRING_IPV4_FORCE_MODE(ADMA_RX_RING2, 1);
	/* 2.4 IPv6 force port mode */
	SET_PDMA_RXRING_IPV6_FORCE_MODE(ADMA_RX_RING2, 1);

	/* 3. Set Age timer: 10 msec. */
	SET_PDMA_RXRING_AGE_TIME(ADMA_RX_RING2, HW_LRO_AGE_TIME);

	/* 4. Valid LRO ring */
	SET_PDMA_RXRING_VALID(ADMA_RX_RING2, 1);

	return 0;
}

int set_fe_lro_ring3_cfg(struct net_device *dev)
{
	unsigned int ip;

	pr_debug("set_fe_lro_ring3_cfg()\n");

	/* 1. Set RX ring mode to force port */
	SET_PDMA_RXRING_MODE(ADMA_RX_RING3, PDMA_RX_FORCE_PORT);

	/* 2. Configure lro ring */
	/* 2.1 set src/destination TCP ports */
	SET_PDMA_RXRING_TCP_SRC_PORT(ADMA_RX_RING3, 9900);
	SET_PDMA_RXRING_TCP_DEST_PORT(ADMA_RX_RING3, 99);
	/* 2.2 set src/destination IPs */
	str_to_ip(&ip, "10.10.10.3");
	sys_reg_write(LRO_RX_RING3_SIP_DW0, ip);
	str_to_ip(&ip, "10.10.10.254");
	sys_reg_write(LRO_RX_RING3_DIP_DW0, ip);
	/* 2.3 IPv4 force port mode */
	SET_PDMA_RXRING_IPV4_FORCE_MODE(ADMA_RX_RING3, 1);
	/* 2.4 IPv6 force port mode */
	SET_PDMA_RXRING_IPV6_FORCE_MODE(ADMA_RX_RING3, 1);

	/* 3. Set Age timer: 10 msec. */
	SET_PDMA_RXRING_AGE_TIME(ADMA_RX_RING3, HW_LRO_AGE_TIME);

	/* 4. Valid LRO ring */
	SET_PDMA_RXRING_VALID(ADMA_RX_RING3, 1);

	return 0;
}

int set_fe_lro_glo_cfg(struct net_device *dev)
{
	unsigned int reg_val = 0;

	pr_debug("set_fe_lro_glo_cfg()\n");

	/* 1 Set max AGG timer: 10 msec. */
	SET_PDMA_LRO_MAX_AGG_TIME(HW_LRO_AGG_TIME);

	/* 2. Set max LRO agg count */
	SET_PDMA_LRO_MAX_AGG_CNT(HW_LRO_MAX_AGG_CNT);

	/* PDMA prefetch enable setting */
	SET_PDMA_LRO_RXD_PREFETCH_EN(ADMA_RXD_PREFETCH_EN |
				     ADMA_MULTI_RXD_PREFETCH_EN);

	/* 2.1 IPv4 checksum update enable */
	SET_PDMA_LRO_IPV4_CSUM_UPDATE_EN(1);

	/* 3. Polling relinguish */
	while (1) {
		if (sys_reg_read(ADMA_LRO_CTRL_DW0) & PDMA_LRO_RELINGUISH)
			pr_warn("Polling HW LRO RELINGUISH...\n");
		else
			break;
	}

	/* 4. Enable LRO */
	reg_val = sys_reg_read(ADMA_LRO_CTRL_DW0);
	reg_val |= PDMA_LRO_EN;
	sys_reg_write(ADMA_LRO_CTRL_DW0, reg_val);

	return 0;
}

void fe_set_hw_lro_my_ip(char *lan_ip_addr)
{
	unsigned int lan_ip;

	str_to_ip(&lan_ip, lan_ip_addr);
	pr_info("[%s]lan_ip_addr = %s (lan_ip = 0x%x)\n",
		__func__, lan_ip_addr, lan_ip);

	/* Set my IP_1: LAN IP */
	sys_reg_write(LRO_RX_RING0_DIP_DW0, lan_ip);
	sys_reg_write(LRO_RX_RING0_DIP_DW1, 0);
	sys_reg_write(LRO_RX_RING0_DIP_DW2, 0);
	sys_reg_write(LRO_RX_RING0_DIP_DW3, 0);
	SET_PDMA_RXRING_MYIP_VALID(ADMA_RX_RING0, 1);
}

/* HW LRO Auto-learn */
int set_fe_lro_auto_cfg(struct net_device *dev)
{
	struct END_DEVICE *ei_local = netdev_priv(dev);
	unsigned int reg_val = 0;

	pr_debug("set_fe_lro_auto_cfg()\n");

	fe_set_hw_lro_my_ip(ei_local->lan_ip4_addr);

	/* Set RX ring1~3 to auto-learn modes */
	SET_PDMA_RXRING_MODE(ADMA_RX_RING1, PDMA_RX_AUTO_LEARN);
	SET_PDMA_RXRING_MODE(ADMA_RX_RING2, PDMA_RX_AUTO_LEARN);
	SET_PDMA_RXRING_MODE(ADMA_RX_RING3, PDMA_RX_AUTO_LEARN);

	/* Valid LRO ring */
	SET_PDMA_RXRING_VALID(ADMA_RX_RING0, 1);
	SET_PDMA_RXRING_VALID(ADMA_RX_RING1, 1);
	SET_PDMA_RXRING_VALID(ADMA_RX_RING2, 1);
	SET_PDMA_RXRING_VALID(ADMA_RX_RING3, 1);

	/* Set AGE timer */
	SET_PDMA_RXRING_AGE_TIME(ADMA_RX_RING1, HW_LRO_AGE_TIME);
	SET_PDMA_RXRING_AGE_TIME(ADMA_RX_RING2, HW_LRO_AGE_TIME);
	SET_PDMA_RXRING_AGE_TIME(ADMA_RX_RING3, HW_LRO_AGE_TIME);

	/* Set max AGG timer */
	SET_PDMA_RXRING_AGG_TIME(ADMA_RX_RING1, HW_LRO_AGG_TIME);
	SET_PDMA_RXRING_AGG_TIME(ADMA_RX_RING2, HW_LRO_AGG_TIME);
	SET_PDMA_RXRING_AGG_TIME(ADMA_RX_RING3, HW_LRO_AGG_TIME);

	/* Set max LRO agg count */
	SET_PDMA_RXRING_MAX_AGG_CNT(ADMA_RX_RING1, HW_LRO_MAX_AGG_CNT);
	SET_PDMA_RXRING_MAX_AGG_CNT(ADMA_RX_RING2, HW_LRO_MAX_AGG_CNT);
	SET_PDMA_RXRING_MAX_AGG_CNT(ADMA_RX_RING3, HW_LRO_MAX_AGG_CNT);

	/* IPv6 LRO enable */
	SET_PDMA_LRO_IPV6_EN(1);

	/* IPv4 checksum update enable */
	SET_PDMA_LRO_IPV4_CSUM_UPDATE_EN(1);

	/* TCP push option check disable */
	/* SET_PDMA_LRO_IPV4_CTRL_PUSH_EN(0); */

	/* PDMA prefetch enable setting */
	SET_PDMA_LRO_RXD_PREFETCH_EN(ADMA_RXD_PREFETCH_EN |
				     ADMA_MULTI_RXD_PREFETCH_EN);

	/* switch priority comparison to packet count mode */
	SET_PDMA_LRO_ALT_SCORE_MODE(PDMA_LRO_ALT_PKT_CNT_MODE);

	/* bandwidth threshold setting */
	SET_PDMA_LRO_BW_THRESHOLD(HW_LRO_BW_THRE);

	/* auto-learn score delta setting */
	sys_reg_write(LRO_ALT_SCORE_DELTA, HW_LRO_REPLACE_DELTA);

	/* Set ALT timer to 20us: (unit: 20us) */
	SET_PDMA_LRO_ALT_REFRESH_TIMER_UNIT(HW_LRO_TIMER_UNIT);
	/* Set ALT refresh timer to 1 sec. (unit: 20us) */
	SET_PDMA_LRO_ALT_REFRESH_TIMER(HW_LRO_REFRESH_TIME);

	/* the least remaining room of SDL0 in RXD for lro aggregation */
	SET_PDMA_LRO_MIN_RXD_SDL(HW_LRO_SDL_REMAIN_ROOM);

	/* Polling relinguish */
	while (1) {
		if (sys_reg_read(ADMA_LRO_CTRL_DW0) & PDMA_LRO_RELINGUISH)
			pr_warn("Polling HW LRO RELINGUISH...\n");
		else
			break;
	}

	/* Enable HW LRO */
	reg_val = sys_reg_read(ADMA_LRO_CTRL_DW0);
	reg_val |= PDMA_LRO_EN;

	/*enable cpu reason black list*/
	reg_val |= PDMA_LRO_CRSN_BNW;
	sys_reg_write(ADMA_LRO_CTRL_DW0, reg_val);

	/*no use PPE cpu reason 0xff*/
	sys_reg_write(ADMA_LRO_CTRL_DW1, 0xffffffff);

	return 0;
}

int fe_hw_lro_init(struct net_device *dev)
{
	struct END_DEVICE *ei_local = netdev_priv(dev);
	int skb_size;
	int i, j;

	skb_size = SKB_DATA_ALIGN(MAX_LRO_RX_LENGTH + NET_IP_ALIGN) +
		   SKB_DATA_ALIGN(sizeof(struct skb_shared_info));

	/* Initial RX Ring 1 ~ 3 */
	for (i = 1; i < MAX_RX_RING_NUM; i++) {
		ei_local->rx_ring[i] =
			dma_alloc_coherent(dev->dev.parent,
					   NUM_LRO_RX_DESC *
					   sizeof(struct PDMA_rxdesc),
					   &ei_local->phy_rx_ring[i],
					   GFP_ATOMIC | __GFP_ZERO);
		for (j = 0; j < NUM_LRO_RX_DESC; j++) {
			ei_local->netrx_skb_data[i][j] =
				raeth_alloc_skb_data(skb_size, GFP_KERNEL);

			if (!ei_local->netrx_skb_data[i][j]) {
				pr_err("rx skbuff buffer allocation failed!\n");
				goto no_rx_mem;
			}

			memset(&ei_local->rx_ring[i][j], 0,
			       sizeof(struct PDMA_rxdesc));
			ei_local->rx_ring[i][j].rxd_info2.DDONE_bit = 0;
			ei_local->rx_ring[i][j].rxd_info2.LS0 = 0;
			ei_local->rx_ring[i][j].rxd_info2.PLEN0 =
			    SET_ADMA_RX_LEN0(MAX_LRO_RX_LENGTH);
			ei_local->rx_ring[i][j].rxd_info2.PLEN1 =
			    SET_ADMA_RX_LEN1(MAX_LRO_RX_LENGTH >> 14);
			ei_local->rx_ring[i][j].rxd_info1.PDP0 =
			    dma_map_single(dev->dev.parent,
					   ei_local->netrx_skb_data[i][j] +
					   NET_SKB_PAD,
					   MAX_LRO_RX_LENGTH, DMA_FROM_DEVICE);
			if (unlikely
			    (dma_mapping_error
			     (dev->dev.parent,
			      ei_local->rx_ring[i][j].rxd_info1.PDP0))) {
				pr_err("[%s]dma_map_single() failed...\n",
				       __func__);
				goto no_rx_mem;
			}
		}
		pr_info("\nphy_rx_ring[%d] = 0x%08x, rx_ring[%d] = 0x%p\n",
			i, (unsigned int)ei_local->phy_rx_ring[i],
			i, (void __iomem *)ei_local->rx_ring[i]);
	}

	sys_reg_write(RX_BASE_PTR3, phys_to_bus((u32)ei_local->phy_rx_ring[3]));
	sys_reg_write(RX_MAX_CNT3, cpu_to_le32((u32)NUM_LRO_RX_DESC));
	sys_reg_write(RX_CALC_IDX3, cpu_to_le32((u32)(NUM_LRO_RX_DESC - 1)));
	sys_reg_write(PDMA_RST_CFG, PST_DRX_IDX3);
	sys_reg_write(RX_BASE_PTR2, phys_to_bus((u32)ei_local->phy_rx_ring[2]));
	sys_reg_write(RX_MAX_CNT2, cpu_to_le32((u32)NUM_LRO_RX_DESC));
	sys_reg_write(RX_CALC_IDX2, cpu_to_le32((u32)(NUM_LRO_RX_DESC - 1)));
	sys_reg_write(PDMA_RST_CFG, PST_DRX_IDX2);
	sys_reg_write(RX_BASE_PTR1, phys_to_bus((u32)ei_local->phy_rx_ring[1]));
	sys_reg_write(RX_MAX_CNT1, cpu_to_le32((u32)NUM_LRO_RX_DESC));
	sys_reg_write(RX_CALC_IDX1, cpu_to_le32((u32)(NUM_LRO_RX_DESC - 1)));
	sys_reg_write(PDMA_RST_CFG, PST_DRX_IDX1);

	if (ei_local->features & FE_HW_LRO_FPORT) {
		set_fe_lro_ring1_cfg(dev);
		set_fe_lro_ring2_cfg(dev);
		set_fe_lro_ring3_cfg(dev);
		set_fe_lro_glo_cfg(dev);
	} else {
		set_fe_lro_auto_cfg(dev);
	}

	return 0;

no_rx_mem:
	return -ENOMEM;
}

void fe_hw_lro_deinit(struct net_device *dev)
{
	struct END_DEVICE *ei_local = netdev_priv(dev);
	int i, j;

	for (i = 1; i < MAX_RX_RING_NUM; i++) {
		/* free RX Ring */
		dma_free_coherent(dev->dev.parent,
				  NUM_LRO_RX_DESC * sizeof(struct PDMA_rxdesc),
				  ei_local->rx_ring[i],
				  ei_local->phy_rx_ring[i]);
		/* free RX data */
		for (j = 0; j < NUM_LRO_RX_DESC; j++) {
			raeth_free_skb_data(ei_local->netrx_skb_data[i][j]);
			ei_local->netrx_skb_data[i][j] = NULL;
		}
	}
}

static inline void hw_lro_rx_desc_init(struct END_DEVICE *ei_local,
				       struct PDMA_rxdesc *rx_ring,
				       unsigned int rx_ring_no,
				       dma_addr_t dma_addr)
{
	if (rx_ring_no != 0) {
		/* lro ring */
		rx_ring->rxd_info2.PLEN0 =
		    SET_ADMA_RX_LEN0(MAX_LRO_RX_LENGTH);
		rx_ring->rxd_info2.PLEN1 =
		    SET_ADMA_RX_LEN1(MAX_LRO_RX_LENGTH >> 14);
	} else
		/* normal ring */
		rx_ring->rxd_info2.PLEN0 = MAX_RX_LENGTH;

	rx_ring->rxd_info1.PDP0 = dma_addr;
	rx_ring->rxd_info2.LS0 = 0;
	rx_ring->rxd_info2.DDONE_bit = 0;
}

static int get_hw_lro_rx_ring(struct END_DEVICE *ei_local,
			      unsigned int rx_idx[])
{
	int i;

	for (i = 0; i < MAX_RX_RING_NUM; i++)
		if (ei_local->rx_ring[i][rx_idx[i]].rxd_info2.DDONE_bit == 1)
			return i;

	return 0;
}

static inline void __iomem *get_rx_cal_idx_reg(unsigned int rx_ring_no)
{
	return (void __iomem *)(RAETH_RX_CALC_IDX0 + (rx_ring_no << 4));
}

int fe_hw_lro_recv(struct net_device *dev,
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
	void __iomem *rx_calc_idx_reg;
	int rx_processed = 0;

	/* get cpu owner indexes of rx rings */
	rx_dma_owner_lro[0] = (ei_local->rx_calc_idx[0] + 1) % num_rx_desc;
	rx_dma_owner_lro[1] = (ei_local->rx_calc_idx[1] + 1) % NUM_LRO_RX_DESC;
	rx_dma_owner_lro[2] = (ei_local->rx_calc_idx[2] + 1) % NUM_LRO_RX_DESC;
	rx_dma_owner_lro[3] = (ei_local->rx_calc_idx[3] + 1) % NUM_LRO_RX_DESC;

	rx_ring_no =  get_hw_lro_rx_ring(ei_local, rx_dma_owner_lro);
	rx_dma_owner_idx = rx_dma_owner_lro[rx_ring_no];
	rx_ring = &ei_local->rx_ring[rx_ring_no][rx_dma_owner_idx];
	rx_data = ei_local->netrx_skb_data[rx_ring_no][rx_dma_owner_idx];
	rx_calc_idx_reg = get_rx_cal_idx_reg(rx_ring_no);

	for (;;) {
		dma_addr_t dma_addr;

		if ((rx_processed++ > budget) ||
		    (rx_ring->rxd_info2.DDONE_bit == 0))
			break;

		/* prefetch the next handling RXD */
		if (rx_ring_no == 0) {
			rx_dma_owner_lro[rx_ring_no] =
				(rx_dma_owner_idx + 1) % num_rx_desc;
			skb_size =
			   SKB_DATA_ALIGN(MAX_RX_LENGTH + NET_IP_ALIGN +
					  NET_SKB_PAD) +
			   SKB_DATA_ALIGN(sizeof(struct skb_shared_info));
			map_size = MAX_RX_LENGTH;
		} else {
			rx_dma_owner_lro[rx_ring_no] =
				(rx_dma_owner_idx + 1) % NUM_LRO_RX_DESC;
			skb_size =
			   SKB_DATA_ALIGN(MAX_LRO_RX_LENGTH + NET_IP_ALIGN +
					  NET_SKB_PAD) +
			   SKB_DATA_ALIGN(sizeof(struct skb_shared_info));
			map_size = MAX_LRO_RX_LENGTH;
		}

		rx_ring_no_next =  get_hw_lro_rx_ring(ei_local,
						      rx_dma_owner_lro);
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
			pr_err("skb not available...\n");
			goto skb_err;
		}

		dma_addr = dma_map_single(dev->dev.parent,
					  new_data + NET_SKB_PAD,
					  map_size,
					  DMA_FROM_DEVICE);

		if (unlikely(dma_mapping_error(dev->dev.parent, dma_addr))) {
			pr_err("[%s]dma_map_single() failed...\n", __func__);
			raeth_free_skb_data(new_data);
			goto skb_err;
		}

		rx_skb = raeth_build_skb(rx_data, skb_size);

		if (unlikely(!rx_skb)) {
			put_page(virt_to_head_page(rx_data));
			pr_err("build_skb failed\n");
			goto skb_err;
		}
		skb_reserve(rx_skb, NET_SKB_PAD + NET_IP_ALIGN);

		length = (rx_ring->rxd_info2.PLEN1 << 14) |
			 rx_ring->rxd_info2.PLEN0;
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
				pr_err
				    ("pseudo_dev is still not initialize ");
				pr_err
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

		/* HW LRO aggregation statistics */
		if (ei_local->features & FE_HW_LRO_DBG) {
			hw_lro_stats_update(rx_ring_no, rx_ring);
			hw_lro_flush_stats_update(rx_ring_no, rx_ring);
		}

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
			if (ei_local->features & FE_INT_NAPI)
			/* napi_gro_receive(napi, rx_skb); */
				netif_receive_skb(rx_skb);
			else
				netif_rx(rx_skb);

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
		hw_lro_rx_desc_init(ei_local,
				    rx_ring,
				    rx_ring_no,
				    dma_addr);
		ei_local->netrx_skb_data[rx_ring_no][rx_dma_owner_idx] =
			new_data;

		/* make sure that all changes to the dma ring are flushed before
		  * we continue
		  */
		wmb();

		sys_reg_write(rx_calc_idx_reg, rx_dma_owner_idx);
		ei_local->rx_calc_idx[rx_ring_no] = rx_dma_owner_idx;

		/* use prefetched variable */
		rx_dma_owner_idx = rx_dma_owner_idx_next;
		rx_ring_no = rx_ring_no_next;
		rx_ring = rx_ring_next;
		rx_data = rx_data_next;
		rx_calc_idx_reg = get_rx_cal_idx_reg(rx_ring_no);
	}	/* for */

	return rx_processed;

skb_err:
	/* rx packet from GE2 */
	if (rx_ring->rxd_info4.SP == 2)
		p_ad->stat.rx_dropped++;
	else
		ei_local->stat.rx_dropped++;

	/* Discard the rx packet */
	hw_lro_rx_desc_init(ei_local,
			    rx_ring,
			    rx_ring_no,
			    rx_ring->rxd_info1.PDP0);
	sys_reg_write(rx_calc_idx_reg, rx_dma_owner_idx);
	ei_local->rx_calc_idx[rx_ring_no] = rx_dma_owner_idx;

	return (budget + 1);
}
