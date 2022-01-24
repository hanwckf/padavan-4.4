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
#ifndef RAETH_CONFIG_H
#define RAETH_CONFIG_H

/* compile flag for features */
#define DELAY_INT

#define CONFIG_RAETH_RW_PDMAPTR_FROM_VAR
/*#define CONFIG_QDMA_QOS_WEB*/
/*#define CONFIG_QDMA_QOS_MARK*/

#if !defined(CONFIG_SOC_MT7621)
#define CONFIG_RAETH_NAPI
#define CONFIG_RAETH_TX_RX_INT_SEPARATION
/*#define CONFIG_RAETH_NAPI_TX_RX*/
#define CONFIG_RAETH_NAPI_RX_ONLY
#endif

#if defined(CONFIG_SOC_MT7621)
#define CONFIG_GE1_RGMII_FORCE_1000
//#define CONFIG_GE1_RGMII_FORCE_1200
#define CONFIG_RA_NETWORK_TASKLET_BH
#endif
/*CONFIG_RA_NETWORK_TASKLET_BH*/
/*CONFIG_RA_NETWORK_WORKQUEUE_BH*/
/*CONFIG_RAETH_SPECIAL_TAG*/
#define CONFIG_RAETH_CHECKSUM_OFFLOAD
#if !defined(CONFIG_SOC_MT7621)
#define CONFIG_RAETH_HW_LRO
#endif
/* #define CONFIG_RAETH_HW_LRO_FORCE */
/* #define CONFIG_RAETH_HW_LRO_DVT */
#define CONFIG_RAETH_HW_VLAN_TX
/*CONFIG_RAETH_HW_VLAN_RX*/
#define CONFIG_RAETH_TSO
/*#define CONFIG_RAETH_ETHTOOL*/
/*#define CONFIG_RAETH_QDMA*/
/*CONFIG_RAETH_QDMATX_QDMARX*/
/*CONFIG_HW_SFQ*/
#define CONFIG_RAETH_HW_IOCOHERENT
#define	CONFIG_RAETH_GMAC2
/*#define CONFIG_RAETH_RSS_4RING*/
/*#define CONFIG_RAETH_RSS_2RING*/
/* definitions */
#ifdef	DELAY_INT
#define FE_DLY_INT	BIT(0)
#else
#define FE_DLY_INT	(0)
#endif
#ifdef	CONFIG_RAETH_HW_LRO
#define FE_HW_LRO	BIT(1)
#else
#define FE_HW_LRO	(0)
#endif
#ifdef	CONFIG_RAETH_HW_LRO_FORCE
#define FE_HW_LRO_FPORT	BIT(2)
#else
#define FE_HW_LRO_FPORT	(0)
#endif
#ifdef	CONFIG_RAETH_LRO
#define FE_SW_LRO	BIT(3)
#else
#define FE_SW_LRO	(0)
#endif
#ifdef	CONFIG_RAETH_QDMA
#define FE_QDMA		BIT(4)
#else
#define FE_QDMA		(0)
#endif
#ifdef	CONFIG_RAETH_NAPI
#define FE_INT_NAPI	BIT(5)
#else
#define FE_INT_NAPI	(0)
#endif
#ifdef	CONFIG_RA_NETWORK_WORKQUEUE_BH
#define FE_INT_WORKQ	BIT(6)
#else
#define FE_INT_WORKQ	(0)
#endif
#ifdef	CONFIG_RA_NETWORK_TASKLET_BH
#define FE_INT_TASKLET	BIT(7)
#else
#define FE_INT_TASKLET	(0)
#endif
#ifdef	CONFIG_RAETH_TX_RX_INT_SEPARATION
#define FE_IRQ_SEPARATE	BIT(8)
#else
#define FE_IRQ_SEPARATE	(0)
#endif
#define FE_GE2_SUPPORT	BIT(9)
#ifdef	CONFIG_RAETH_ETHTOOL
#define FE_ETHTOOL	BIT(10)
#else
#define FE_ETHTOOL	(0)
#endif
#ifdef	CONFIG_RAETH_CHECKSUM_OFFLOAD
#define FE_CSUM_OFFLOAD	BIT(11)
#else
#define FE_CSUM_OFFLOAD	(0)
#endif
#ifdef	CONFIG_RAETH_TSO
#define FE_TSO		BIT(12)
#else
#define FE_TSO		(0)
#endif
#ifdef	CONFIG_RAETH_TSOV6
#define FE_TSO_V6	BIT(13)
#else
#define FE_TSO_V6	(0)
#endif
#ifdef	CONFIG_RAETH_HW_VLAN_TX
#define FE_HW_VLAN_TX	BIT(14)
#else
#define FE_HW_VLAN_TX	(0)
#endif
#ifdef	CONFIG_RAETH_HW_VLAN_RX
#define FE_HW_VLAN_RX	BIT(15)
#else
#define FE_HW_VLAN_RX	(0)
#endif
#ifdef	CONFIG_RAETH_QDMA
#define FE_QDMA_TX	BIT(16)
#else
#define FE_QDMA_TX	(0)
#endif
#ifdef	CONFIG_RAETH_QDMATX_QDMARX
#define FE_QDMA_RX	BIT(17)
#else
#define FE_QDMA_RX	(0)
#endif
#ifdef	CONFIG_HW_SFQ
#define FE_HW_SFQ	BIT(18)
#else
#define FE_HW_SFQ	(0)
#endif
#define FE_HW_IOCOHERENT BIT(19)

#ifdef	CONFIG_MTK_FPGA
#define FE_FPGA_MODE	BIT(20)
#else
#define FE_FPGA_MODE	(0)
#endif

#ifdef CONFIG_RAETH_RSS_4RING
#define FE_RSS_4RING	BIT(20)
#else
#define FE_RSS_4RING	(0)
#endif

#ifdef CONFIG_RAETH_RSS_2RING
#define FE_RSS_2RING	BIT(2)
#else
#define FE_RSS_2RING	(0)
#endif

#ifdef	CONFIG_RAETH_HW_LRO_REASON_DBG
#define FE_HW_LRO_DBG	BIT(21)
#else
#define FE_HW_LRO_DBG	(0)
#endif
#ifdef CONFIG_RAETH_INT_DBG
#define FE_RAETH_INT_DBG	BIT(22)
#else
#define FE_RAETH_INT_DBG	(0)
#endif
#ifdef CONFIG_USER_SNMPD
#define USER_SNMPD	BIT(23)
#else
#define USER_SNMPD	(0)
#endif
#ifdef CONFIG_TASKLET_WORKQUEUE_SW
#define TASKLET_WORKQUEUE_SW	BIT(24)
#else
#define TASKLET_WORKQUEUE_SW	(0)
#endif
#if defined(CONFIG_RA_HW_NAT) || defined(CONFIG_RA_HW_NAT_MODULE)
#define FE_HW_NAT	BIT(25)
#else
#define FE_HW_NAT	(0)
#endif
#ifdef	CONFIG_RAETH_NAPI_TX_RX
#define FE_INT_NAPI_TX_RX	BIT(26)
#else
#define FE_INT_NAPI_TX_RX	(0)
#endif
#ifdef	CONFIG_QDMA_MQ
#define QDMA_MQ       BIT(27)
#else
#define QDMA_MQ       (0)
#endif
#ifdef	CONFIG_RAETH_NAPI_RX_ONLY
#define FE_INT_NAPI_RX_ONLY	BIT(28)
#else
#define FE_INT_NAPI_RX_ONLY	(0)
#endif
#ifdef	CONFIG_QDMA_SUPPORT_QOS
#define FE_QDMA_FQOS	BIT(29)
#else
#define FE_QDMA_FQOS	(0)
#endif

#ifdef	CONFIG_QDMA_QOS_WEB
#define QDMA_QOS_WEB	BIT(30)
#else
#define QDMA_QOS_WEB	(0)
#endif

#ifdef	CONFIG_QDMA_QOS_MARK
#define QDMA_QOS_MARK	BIT(31)
#else
#define QDMA_QOS_MARK	(0)
#endif

#define MT7626_FE	(7626)
#define MT7623_FE	(7623)
#define MT7622_FE	(7622)
#define MT7621_FE	(7621)
#define LEOPARD_FE		(1985)

#define GMAC2 BIT(0)
#define LAN_WAN_SUPPORT BIT(1)
#define WAN_AT_P0 BIT(2)
#define WAN_AT_P4 BIT(3)
#if defined(CONFIG_GE1_RGMII_FORCE_1000)
#define    GE1_RGMII_FORCE_1000		BIT(4)
#define    GE1_TRGMII_FORCE_2000	(0)
#define    GE1_TRGMII_FORCE_2600	(0)
#define    MT7530_TRGMII_PLL_25M	(0x0A00)
#define    MT7530_TRGMII_PLL_40M	(0x0640)
#elif defined(CONFIG_GE1_TRGMII_FORCE_2000)
#define    GE1_TRGMII_FORCE_2000	BIT(5)
#define    GE1_RGMII_FORCE_1000		(0)
#define    GE1_TRGMII_FORCE_2600	(0)
#define    MT7530_TRGMII_PLL_25M	(0x1400)
#define    MT7530_TRGMII_PLL_40M	(0x0C80)
#elif defined(CONFIG_GE1_TRGMII_FORCE_2600)
#define    GE1_TRGMII_FORCE_2600	BIT(6)
#define    GE1_RGMII_FORCE_1000		(0)
#define    GE1_TRGMII_FORCE_2000	(0)
#define    MT7530_TRGMII_PLL_25M	(0x1A00)
#define    MT7530_TRGMII_PLL_40M	(0x1040)
#define    TRGMII
#else
#define    GE1_RGMII_FORCE_1000		(0)
#define    GE1_TRGMII_FORCE_2000	(0)
#define    GE1_TRGMII_FORCE_2600	(0)
#define    MT7530_TRGMII_PLL_25M	(0)
#define    MT7530_TRGMII_PLL_40M	(0)
#endif

#define    GE1_RGMII_AN    BIT(7)
#define    GE1_SGMII_AN    BIT(8)
#define    GE1_SGMII_FORCE_2500    BIT(9)
#define    GE1_RGMII_ONE_EPHY    BIT(10)
#define    RAETH_ESW    BIT(11)
#define    GE1_RGMII_NONE    BIT(12)
#define    GE2_RGMII_FORCE_1000    BIT(13)
#define    GE2_RGMII_AN    BIT(14)
#define    GE2_INTERNAL_GPHY    BIT(15)
#define    GE2_SGMII_AN    BIT(16)
#define    GE2_SGMII_FORCE_2500    BIT(17)
#define    MT7622_EPHY    BIT(18)
#define    RAETH_SGMII	BIT(19)
#define    GE2_RAETH_SGMII	BIT(20)
#define    LEOPARD_EPHY	BIT(21)
#define    SGMII_SWITCH	BIT(22)
#define    LEOPARD_EPHY_GMII BIT(23)
/* /#ifndef CONFIG_MAC_TO_GIGAPHY_MODE_ADDR */
/* #define CONFIG_MAC_TO_GIGAPHY_MODE_ADDR (0) */
/* #endif */
/* #ifndef CONFIG_MAC_TO_GIGAPHY_MODE_ADDR2 */
/* #define CONFIG_MAC_TO_GIGAPHY_MODE_ADDR2 (0) */
/* #endif */

/* macros */
#define fe_features_config(end_device)	\
{					\
end_device->features = 0;		\
end_device->features |= FE_DLY_INT;	\
end_device->features |= FE_HW_LRO;	\
end_device->features |= FE_HW_LRO_FPORT;\
end_device->features |= FE_HW_LRO_DBG;	\
end_device->features |= FE_SW_LRO;	\
end_device->features |= FE_QDMA;	\
end_device->features |= FE_INT_NAPI;	\
end_device->features |= FE_INT_WORKQ;	\
end_device->features |= FE_INT_TASKLET;	\
end_device->features |= FE_IRQ_SEPARATE;\
end_device->features |= FE_ETHTOOL;	\
end_device->features |= FE_CSUM_OFFLOAD;\
end_device->features |= FE_TSO;		\
end_device->features |= FE_TSO_V6;	\
end_device->features |= FE_HW_VLAN_TX;	\
end_device->features |= FE_HW_VLAN_RX;	\
end_device->features |= FE_QDMA_TX;	\
end_device->features |= FE_QDMA_RX;	\
end_device->features |= FE_HW_SFQ;	\
end_device->features |= FE_FPGA_MODE;	\
end_device->features |= FE_HW_NAT;	\
end_device->features |= FE_INT_NAPI_TX_RX; \
end_device->features |= FE_INT_NAPI_RX_ONLY; \
end_device->features |= FE_QDMA_FQOS;	\
end_device->features |= QDMA_QOS_WEB;	\
end_device->features |= QDMA_QOS_MARK;	\
end_device->features |= FE_RSS_4RING;	\
end_device->features |= FE_RSS_2RING;	\
}

#define fe_architecture_config(end_device)              \
{                                                       \
end_device->architecture = 0;                           \
end_device->architecture |= GE1_TRGMII_FORCE_2000;      \
end_device->architecture |= GE1_TRGMII_FORCE_2600;      \
}
#endif
