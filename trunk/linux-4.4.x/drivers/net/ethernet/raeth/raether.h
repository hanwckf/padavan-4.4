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
#ifndef RA2882ETHEND_H
#define RA2882ETHEND_H

#include "raeth_config.h"
#include "raeth_reg.h"
#include "ra_dbg_proc.h"
#include "ra_ioctl.h"

#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/pci.h>
#include <linux/init.h>
#include <linux/skbuff.h>
#include <linux/if_vlan.h>
#include <linux/if_ether.h>
#include <linux/fs.h>
#include <linux/mii.h>
#include <linux/uaccess.h>
#if defined(CONFIG_RAETH_TSO)
#include <linux/tcp.h>
#include <net/ipv6.h>
#include <linux/ip.h>
#include <net/ip.h>
#include <net/tcp.h>
#include <linux/in.h>
#include <linux/ppp_defs.h>
#include <linux/if_pppox.h>
#endif
#include <linux/netdevice.h>
#include <linux/if_vlan.h>
#include <linux/ppp_defs.h>

/* LRO support */
#include <linux/inet_lro.h>

#include <linux/delay.h>
#include <linux/sched.h>

#include <asm-generic/pci-dma-compat.h>

#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/of_net.h>
#include <linux/of_irq.h>
#include <linux/of_gpio.h>
#include <linux/mfd/syscon.h>
#include <linux/regmap.h>
#include <linux/clk.h>
#include <linux/regulator/consumer.h>

#include <linux/dma-mapping.h>

#if defined(CONFIG_MACH_MT7623)
#include <linux/delay.h>
#endif
#include <linux/kthread.h>
#include <linux/prefetch.h>

#include <linux/platform_device.h>
#include <linux/pm_runtime.h>

#if defined(CONFIG_RA_HW_NAT)  || defined(CONFIG_RA_HW_NAT_MODULE)
#include <net/ra_nat.h>
#endif

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define ETH_GPIO_BASE	0x10005000

#if defined(CONFIG_QDMA_MQ)
#define GMAC1_TXQ_NUM 3
#define GMAC1_TXQ_TXD_NUM 512
#define GMAC1_TXD_NUM (GMAC1_TXQ_NUM * GMAC1_TXQ_TXD_NUM)
#define GMAC2_TXQ_NUM 1
#define GMAC2_TXQ_TXD_NUM 128
#define GMAC2_TXD_NUM (GMAC2_TXQ_NUM * GMAC2_TXQ_TXD_NUM)
#define NUM_TX_DESC (GMAC1_TXD_NUM + GMAC2_TXD_NUM)
#define TOTAL_TXQ_NUM (GMAC1_TXQ_NUM + GMAC2_TXQ_NUM)
#else
#define TOTAL_TXQ_NUM 2
#endif

#if defined(CONFIG_MACH_MT7623)
#define NUM_RX_DESC     2048
#define NUM_QRX_DESC 16
#define NUM_PQ_RESV 4
#define FFA 2048
#define QUEUE_OFFSET 0x10
#else
#define NUM_QRX_DESC 16
#define NUM_PQ_RESV 4
#define FFA 512
#define QUEUE_OFFSET 0x10
#endif

#if defined(CONFIG_PINCTRL_MT7622)
#define NUM_PQ 64
#else
#define NUM_PQ 16
#endif
/* #define NUM_TX_MAX_PROCESS NUM_TX_DESC */
#define NUM_RX_MAX_PROCESS 16

#define MAX_RX_RING_NUM	4
#define NUM_LRO_RX_DESC	16

#define	MAX_RX_LENGTH	1536

#define DEV_NAME        "eth2"
#define DEV2_NAME       "eth3"

#if defined(CONFIG_MACH_MT7623)
#define GMAC0_OFFSET    0xE000
#define GMAC2_OFFSET    0xE006
#else
#define GMAC0_OFFSET    0x28
#define GMAC2_OFFSET    0x22
#endif

#if defined(CONFIG_MACH_MT7623)
#define IRQ_ENET0       232
#define IRQ_ENET1       231
#define IRQ_ENET2       230
#else
/* NOTE(Nelson): prom version started from 20150806 */
#define IRQ_ENET0       255
#define IRQ_ENET1       256
#define IRQ_ENET2       257
#endif
#define MTK_NAPI_WEIGHT	64

#define RAETH_VERSION	"STD_v0.1"

/* MT7623 PSE reset workaround */
#define	FE_RESET_POLLING_MS	(5000)

/*LEOPARD POLLING*/
#define PHY_POLLING_MS		(1000)
#define FE_DEFAULT_LAN_IP	"192.168.1.1"
#define IP4_ADDR_LEN		16

#if defined(CONFIG_SOC_MT7621)
#define MT_TRIGGER_LOW	0
#else
#define MT_TRIGGER_LOW	IRQF_TRIGGER_LOW
#endif

/* This enum allows us to identify how the clock is defined on the array of the
 * clock in the order
 */
enum mtk_clks_map {
	MTK_CLK_ETHIF,
	MTK_CLK_ESW,
	MTK_CLK_GP0,
	MTK_CLK_GP1,
	MTK_CLK_GP2,
	MTK_CLK_SGMII_TX250M,
	MTK_CLK_SGMII_RX250M,
	MTK_CLK_SGMII_CDR_REF,
	MTK_CLK_SGMII_CDR_FB,
	MTK_CLK_SGMII1_TX250M,
	MTK_CLK_SGMII1_RX250M,
	MTK_CLK_SGMII1_CDR_REF,
	MTK_CLK_SGMII1_CDR_FB,
	MTK_CLK_TRGPLL,
	MTK_CLK_SGMIPLL,
	MTK_CLK_ETH1PLL,
	MTK_CLK_ETH2PLL,
	MTK_CLK_FE,
	MTK_CLK_SGMII_TOP,
	MTK_CLK_MAX
};

struct END_DEVICE {
	struct device *dev;
	unsigned int tx_cpu_owner_idx0;
#ifdef CONFIG_RAETH_RW_PDMAPTR_FROM_VAR
	unsigned int rx_calc_idx[MAX_RX_RING_NUM];
#endif
	unsigned int tx_ring_full;
	unsigned int tx_full;	/* NOTE(Nelso): unused, can remove */

	/* PDMA TX  PTR */
	dma_addr_t phy_tx_ring0;

	/* QDMA TX  PTR */
	struct platform_device *qdma_pdev;
	/* struct sk_buff *free_skb[NUM_TX_DESC]; */
	struct sk_buff **free_skb;
	unsigned int tx_dma_ptr;
	unsigned int tx_cpu_ptr;
	unsigned int tx_cpu_idx;
	unsigned int rls_cpu_idx;
	/* atomic_t  free_txd_num[TOTAL_TXQ_NUM]; */
	atomic_t  *free_txd_num;
	/* unsigned int free_txd_head[TOTAL_TXQ_NUM]; */
	/* unsigned int free_txd_tail[TOTAL_TXQ_NUM]; */
	unsigned int *free_txd_head;
	unsigned int *free_txd_tail;
	struct QDMA_txdesc *txd_pool;
	dma_addr_t phy_txd_pool;
	/* unsigned int txd_pool_info[NUM_TX_DESC]; */
	unsigned int *txd_pool_info;
	struct QDMA_txdesc *free_head;
	unsigned int phy_free_head;
	unsigned int *free_page_head;
	dma_addr_t phy_free_page_head;
	struct PDMA_rxdesc *qrx_ring;
	dma_addr_t phy_qrx_ring;

	/* TSO */
	unsigned int skb_txd_num;

	/* MT7623 workaround */
	struct work_struct reset_task;

	/* workqueue_bh */
	struct work_struct rx_wq;

	/* tasklet_bh */
	struct tasklet_struct rx_tasklet;

	/* struct sk_buff *skb_free[NUM_TX_DESC]; */
	struct sk_buff **skb_free;
	unsigned int free_idx;

	struct net_device_stats stat;	/* The new statistics table. */
	spinlock_t page_lock;	/* spin_lock for cr access critial section */
	spinlock_t irq_lock;	/* spin_lock for isr critial section */
	spinlock_t mdio_lock;   /* spin_lock for mdio reg access */
	struct PDMA_txdesc *tx_ring0;
	struct PDMA_rxdesc *rx_ring[MAX_RX_RING_NUM];
	dma_addr_t phy_rx_ring[MAX_RX_RING_NUM];

	/* void *netrx_skb_data[MAX_RX_RING_NUM][NUM_RX_DESC]; */
	void **netrx_skb_data[MAX_RX_RING_NUM];

	/* struct sk_buff *netrx0_skbuf[NUM_RX_DESC]; */
	/*struct sk_buff **netrx0_skbuf;*/
	void **netrx0_skb_data;
	/* napi */
	struct napi_struct napi;
	struct napi_struct napi_rx;
	struct napi_struct napi_rx_rss0;
	struct napi_struct napi_rx_rss1;
	struct napi_struct napi_rx_rss2;
	struct napi_struct napi_rx_rss3;
	struct napi_struct napi_tx;
	struct net_device dummy_dev;

	/* clock control */
	struct clk	*clks[MTK_CLK_MAX];

	/* gsw device node */
	struct device_node *switch_np;

	/* GE1 support */
	struct net_device *netdev;
	/* GE2 support */
	struct net_device *pseudo_dev;
	unsigned int is_pseudo;

	struct mii_if_info mii_info;
	struct lro_counters lro_counters;
	struct net_lro_mgr lro_mgr;
	struct net_lro_desc lro_arr[8];
	struct vlan_group *vlgrp;

	/* virtual base addr from device tree */
	void __iomem *ethdma_sysctl_base;

	unsigned int irq0;
	unsigned int irq1;
	unsigned int irq2;
	unsigned int irq3;
	unsigned int esw_irq;
	void __iomem *fe_tx_int_status;
	void __iomem *fe_tx_int_enable;
	void __iomem *fe_rx_int_status;
	void __iomem *fe_rx_int_enable;

	unsigned int features;
	unsigned int chip_name;
	unsigned int architecture;

	/* IP address */
	char lan_ip4_addr[IP4_ADDR_LEN];

	/* Function pointers */
	int (*ei_start_xmit)(struct sk_buff *skb, struct net_device *netdev,
			     int gmac_no);
	int (*ei_xmit_housekeeping)(struct net_device *netdev, int budget);
	int (*ei_eth_recv)(struct net_device *dev,
			   struct napi_struct *napi,
			   int budget);
	int (*ei_eth_recv_rss0)(struct net_device *dev,
				struct napi_struct *napi,
			   int budget);
	int (*ei_eth_recv_rss1)(struct net_device *dev,
				struct napi_struct *napi,
			   int budget);
	int (*ei_eth_recv_rss2)(struct net_device *dev,
				struct napi_struct *napi,
			   int budget);
	int (*ei_eth_recv_rss3)(struct net_device *dev,
				struct napi_struct *napi,
			   int budget);
	int (*ei_fill_tx_desc)(struct net_device *dev,
			       unsigned long *tx_cpu_owner_idx,
			       struct sk_buff *skb, int gmac_no);

	/* MT7623 PSE reset workaround */
	struct task_struct *kreset_task;
	struct task_struct *kphy_poll_task;
	unsigned int fe_reset_times;
	unsigned int tx_mask;
	unsigned int rx_mask;
	unsigned int *rls_cnt;
};

#ifdef CONFIG_INET_LRO
static inline void ei_lro_flush_all(struct net_lro_mgr *lro_mgr)
{
	lro_flush_all(lro_mgr);
}
#else
static inline void ei_lro_flush_all(struct net_lro_mgr *lro_mgr)
{
}
#endif

struct net_device_stats *ra_get_stats(struct net_device *dev);

#if defined (CONFIG_RAETH_ESW_CONTROL)
void esw_ioctl_uninit(void);
int esw_ioctl_init(void);
int esw_ioctl_init_post(void);
#endif

int ei_open(struct net_device *dev);
int ei_close(struct net_device *dev);

int ra2882eth_init(void);
void ra2882eth_cleanup_module(void);

u32 mii_mgr_read(u32 phy_addr, u32 phy_register, u32 *read_data);
u32 mii_mgr_write(u32 phy_addr, u32 phy_register, u32 write_data);
u32 mii_mgr_cl45_set_address(u32 port_num, u32 dev_addr, u32 reg_addr);
u32 mii_mgr_read_cl45(u32 port_num, u32 dev_addr, u32 reg_addr,
		      u32 *read_data);
u32 mii_mgr_write_cl45(u32 port_num, u32 dev_addr, u32 reg_addr,
		       u32 write_data);

/* HNAT functions */
#if defined(CONFIG_RA_NAT_NONE)
static int (*ra_sw_nat_hook_rx)(struct sk_buff *skb);
static int (*ra_sw_nat_hook_tx)(struct sk_buff *skb, int gmac_no);
#else
extern int (*ra_sw_nat_hook_rx)(struct sk_buff *skb);
extern int (*ra_sw_nat_hook_tx)(struct sk_buff *skb, int gmac_no);
#endif

/* PDMA functions */
int fe_pdma_wait_dma_idle(void);
int fe_pdma_rx_dma_init(struct net_device *dev);
int fe_pdma_tx_dma_init(struct net_device *dev);
void fe_pdma_rx_dma_deinit(struct net_device *dev);
void fe_pdma_tx_dma_deinit(struct net_device *dev);
void set_fe_pdma_glo_cfg(void);
int ei_pdma_start_xmit(struct sk_buff *skb, struct net_device *dev,
		       int gmac_no);
int ei_pdma_xmit_housekeeping(struct net_device *netdev,
			      int budget);
int fe_fill_tx_desc(struct net_device *dev,
		    unsigned long *tx_cpu_owner_idx,
		    struct sk_buff *skb,
		    int gmac_no);
int fe_fill_tx_desc_tso(struct net_device *dev,
			unsigned long *tx_cpu_owner_idx,
			struct sk_buff *skb,
			int gmac_no);
void fe_set_sw_lro_my_ip(char *lan_ip_addr);

/* QDMA functions */
int fe_qdma_wait_dma_idle(void);
int fe_qdma_rx_dma_init(struct net_device *dev);
int fe_qdma_tx_dma_init(struct net_device *dev);
void fe_qdma_rx_dma_deinit(struct net_device *dev);
void fe_qdma_tx_dma_deinit(struct net_device *dev);
void set_fe_qdma_glo_cfg(void);
int ei_qdma_start_xmit(struct sk_buff *skb, struct net_device *dev,
		       int gmac_no);
int ei_qdma_xmit_housekeeping(struct net_device *netdev, int budget);
int ei_qdma_ioctl(struct net_device *dev, struct ifreq *ifr,
		  struct qdma_ioctl_data *ioctl_data);
int ephy_ioctl(struct net_device *dev, struct ifreq *ifr,
	       struct ephy_ioctl_data *ioctl_data);
/* HW LRO functions */
int fe_hw_lro_init(struct net_device *dev);
void fe_hw_lro_deinit(struct net_device *dev);
int fe_hw_lro_recv(struct net_device *dev,
		   struct napi_struct *napi,
		   int budget);
void fe_set_hw_lro_my_ip(char *lan_ip_addr);

int fe_rss_4ring_init(struct net_device *dev);
void fe_rss_4ring_deinit(struct net_device *dev);
int fe_rss_2ring_init(struct net_device *dev);
void fe_rss_2ring_deinit(struct net_device *dev);
int fe_rss0_recv(struct net_device *dev,
		 struct napi_struct *napi,
		   int budget);
int fe_rss1_recv(struct net_device *dev,
		 struct napi_struct *napi,
		   int budget);
int fe_rss2_recv(struct net_device *dev,
		 struct napi_struct *napi,
		   int budget);
int fe_rss3_recv(struct net_device *dev,
		 struct napi_struct *napi,
		   int budget);
static inline void *raeth_alloc_skb_data(size_t size, gfp_t flags)
{
#ifdef CONFIG_ETH_SLAB_ALLOC_SKB
	return kmalloc(size, flags);
#else
	return netdev_alloc_frag(size);
#endif
}

static inline void raeth_free_skb_data(void *addr)
{
#ifdef CONFIG_ETH_SLAB_ALLOC_SKB
	kfree(addr);
#else
	skb_free_frag(addr);
#endif
}

static inline struct sk_buff *raeth_build_skb(void *data,
					      unsigned int frag_size)
{
#ifdef CONFIG_ETH_SLAB_ALLOC_SKB
	return build_skb(data, 0);
#else
	return build_skb(data, frag_size);
#endif
}

extern u32 gmac1_txq_num;
extern u32 gmac1_txq_txd_num;
extern u32 gmac1_txd_num;
extern u32 gmac2_txq_num;
extern u32 gmac2_txq_txd_num;
extern u32 gmac2_txd_num;
extern u32 num_rx_desc;
extern u32 num_tx_max_process;
extern u32 num_tx_desc;
extern u32 total_txq_num;
extern u32 mac_to_gigaphy_mode_addr;
extern u32 mac_to_gigaphy_mode_addr2;
#endif
