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
#ifndef RA_HWLRO_H
#define RA_HWLRO_H

#include "raeth_reg.h"

#define	HW_LRO_TIMER_UNIT   1
#define	HW_LRO_REFRESH_TIME 50000
#define	HW_LRO_MAX_AGG_CNT	64
#define	HW_LRO_AGG_DELTA	1
#define	MAX_LRO_RX_LENGTH	(PAGE_SIZE * 3)
#define	HW_LRO_AGG_TIME		10	/* 200us */
#define	HW_LRO_AGE_TIME		50	/* 1ms */
#define	HW_LRO_BW_THRE	        3000
#define	HW_LRO_REPLACE_DELTA    1000
#define	HW_LRO_SDL_REMAIN_ROOM	1522

struct PDMA_LRO_AUTO_TLB_INFO0_T {
	unsigned int DTP:16;
	unsigned int STP:16;
};

struct PDMA_LRO_AUTO_TLB_INFO1_T {
	unsigned int SIP0:32;
};

struct PDMA_LRO_AUTO_TLB_INFO2_T {
	unsigned int SIP1:32;
};

struct PDMA_LRO_AUTO_TLB_INFO3_T {
	unsigned int SIP2:32;
};

struct PDMA_LRO_AUTO_TLB_INFO4_T {
	unsigned int SIP3:32;
};

struct PDMA_LRO_AUTO_TLB_INFO5_T {
	unsigned int VLAN_VID0:32;
};

struct PDMA_LRO_AUTO_TLB_INFO6_T {
	unsigned int VLAN_VID1:16;
	unsigned int VLAN_VID_VLD:4;
	unsigned int CNT:12;
};

struct PDMA_LRO_AUTO_TLB_INFO7_T {
	unsigned int DW_LEN:32;
};

struct PDMA_LRO_AUTO_TLB_INFO8_T {
	unsigned int DIP_ID:2;
	unsigned int IPV6:1;
	unsigned int IPV4:1;
	unsigned int RESV:27;
	unsigned int VALID:1;
};

struct PDMA_LRO_AUTO_TLB_INFO {
	struct PDMA_LRO_AUTO_TLB_INFO0_T auto_tlb_info0;
	struct PDMA_LRO_AUTO_TLB_INFO1_T auto_tlb_info1;
	struct PDMA_LRO_AUTO_TLB_INFO2_T auto_tlb_info2;
	struct PDMA_LRO_AUTO_TLB_INFO3_T auto_tlb_info3;
	struct PDMA_LRO_AUTO_TLB_INFO4_T auto_tlb_info4;
	struct PDMA_LRO_AUTO_TLB_INFO5_T auto_tlb_info5;
	struct PDMA_LRO_AUTO_TLB_INFO6_T auto_tlb_info6;
	struct PDMA_LRO_AUTO_TLB_INFO7_T auto_tlb_info7;
	struct PDMA_LRO_AUTO_TLB_INFO8_T auto_tlb_info8;
};

#define PDMA_LRO_EN             BIT(0)
#define PDMA_LRO_IPV6_EN        BIT(1)
#define PDMA_LRO_CRSN_BNW       BIT(6)
#define PDMA_LRO_IPV4_CSUM_UPDATE_EN    BIT(7)
#define PDMA_LRO_IPV4_CTRL_PUSH_EN	BIT(23)
#define PDMA_LRO_RXD_PREFETCH_EN        BITS(3, 4)
#define PDMA_NON_LRO_MULTI_EN   BIT(2)
#define PDMA_LRO_DLY_INT_EN             BIT(5)
#define PDMA_LRO_FUSH_REQ               BITS(26, 28)
#define PDMA_LRO_RELINGUISH     BITS(29, 31)
#define PDMA_LRO_FREQ_PRI_ADJ   BITS(16, 19)
#define PDMA_LRO_TPUT_PRE_ADJ           BITS(8, 11)
#define PDMA_LRO_TPUT_PRI_ADJ           BITS(12, 15)
#define PDMA_LRO_ALT_SCORE_MODE         BIT(21)
#define PDMA_LRO_RING_AGE1      BITS(22, 31)
#define PDMA_LRO_RING_AGE2      BITS(0, 5)
#define PDMA_LRO_RING_AGG               BITS(10, 25)
#define PDMA_LRO_RING_AGG_CNT1          BITS(26, 31)
#define PDMA_LRO_RING_AGG_CNT2          BITS(0, 1)
#define PDMA_LRO_ALT_TICK_TIMER         BITS(16, 20)
#define PDMA_LRO_LRO_MIN_RXD_SDL0       BITS(16, 31)

#define PDMA_LRO_DLY_INT_EN_OFFSET          (5)
#define PDMA_LRO_TPUT_PRE_ADJ_OFFSET        (8)
#define PDMA_LRO_FREQ_PRI_ADJ_OFFSET    (16)
#define PDMA_LRO_LRO_MIN_RXD_SDL0_OFFSET    (16)
#define PDMA_LRO_TPUT_PRI_ADJ_OFFSET        (12)
#define PDMA_LRO_ALT_SCORE_MODE_OFFSET      (21)
#define PDMA_LRO_FUSH_REQ_OFFSET            (26)
#define PDMA_NON_LRO_MULTI_EN_OFFSET        (2)
#define PDMA_LRO_IPV6_EN_OFFSET             (1)
#define PDMA_LRO_RXD_PREFETCH_EN_OFFSET     (3)
#define PDMA_LRO_IPV4_CSUM_UPDATE_EN_OFFSET (7)
#define PDMA_LRO_IPV4_CTRL_PUSH_EN_OFFSET   (23)
#define PDMA_LRO_ALT_TICK_TIMER_OFFSET      (16)

#define PDMA_LRO_TPUT_OVERFLOW_ADJ  BITS(12, 31)
#define PDMA_LRO_CNT_OVERFLOW_ADJ   BITS(0, 11)

#define PDMA_LRO_TPUT_OVERFLOW_ADJ_OFFSET   (12)
#define PDMA_LRO_CNT_OVERFLOW_ADJ_OFFSET    (0)

#define PDMA_LRO_ALT_BYTE_CNT_MODE  (0)
#define PDMA_LRO_ALT_PKT_CNT_MODE   (1)

/* LRO_RX_RING1_CTRL_DW1 offsets  */
#define PDMA_LRO_AGE_H_OFFSET           (10)
#define PDMA_LRO_RING_AGE1_OFFSET       (22)
#define PDMA_LRO_RING_AGG_CNT1_OFFSET   (26)
/* LRO_RX_RING1_CTRL_DW2 offsets  */
#define PDMA_RX_MODE_OFFSET             (6)
#define PDMA_RX_PORT_VALID_OFFSET       (8)
#define PDMA_RX_MYIP_VALID_OFFSET       (9)
#define PDMA_LRO_RING_AGE2_OFFSET       (0)
#define PDMA_LRO_RING_AGG_OFFSET        (10)
#define PDMA_LRO_RING_AGG_CNT2_OFFSET   (0)
/* LRO_RX_RING1_CTRL_DW3 offsets  */
#define PDMA_LRO_AGG_CNT_H_OFFSET       (6)
/* LRO_RX_RING1_STP_DTP_DW offsets */
#define PDMA_RX_TCP_SRC_PORT_OFFSET     (16)
#define PDMA_RX_TCP_DEST_PORT_OFFSET    (0)
/* LRO_RX_RING1_CTRL_DW0 offsets */
#define PDMA_RX_IPV4_FORCE_OFFSET       (1)
#define PDMA_RX_IPV6_FORCE_OFFSET       (0)

#define ADMA_MULTI_RXD_PREFETCH_EN	BIT(3)
#define ADMA_RXD_PREFETCH_EN		BIT(4)

#define SET_PDMA_LRO_MAX_AGG_CNT(x) \
{ \
unsigned int reg_val = sys_reg_read(ADMA_LRO_CTRL_DW3); \
reg_val &= ~0xff;   \
reg_val |= ((x) & 0xff);  \
sys_reg_write(ADMA_LRO_CTRL_DW3, reg_val); \
}

#define SET_PDMA_LRO_FLUSH_REQ(x) \
{ \
unsigned int reg_val = sys_reg_read(ADMA_LRO_CTRL_DW0); \
reg_val &= ~PDMA_LRO_FUSH_REQ;   \
reg_val |= ((x) & 0x7) << PDMA_LRO_FUSH_REQ_OFFSET;  \
sys_reg_write(ADMA_LRO_CTRL_DW0, reg_val); \
}

#define SET_PDMA_LRO_IPV6_EN(x) \
{ \
unsigned int reg_val = sys_reg_read(ADMA_LRO_CTRL_DW0); \
reg_val &= ~PDMA_LRO_IPV6_EN;   \
reg_val |= ((x) & 0x1) << PDMA_LRO_IPV6_EN_OFFSET;  \
sys_reg_write(ADMA_LRO_CTRL_DW0, reg_val); \
}

#define SET_PDMA_LRO_RXD_PREFETCH_EN(x) \
{ \
unsigned int reg_val = sys_reg_read(ADMA_LRO_CTRL_DW0); \
reg_val &= ~PDMA_LRO_RXD_PREFETCH_EN;   \
reg_val |= (x);  \
sys_reg_write(ADMA_LRO_CTRL_DW0, reg_val); \
}

#define SET_PDMA_LRO_IPV4_CSUM_UPDATE_EN(x) \
{ \
unsigned int reg_val = sys_reg_read(ADMA_LRO_CTRL_DW0); \
reg_val &= ~PDMA_LRO_IPV4_CSUM_UPDATE_EN;   \
reg_val |= ((x) & 0x1) << PDMA_LRO_IPV4_CSUM_UPDATE_EN_OFFSET;  \
sys_reg_write(ADMA_LRO_CTRL_DW0, reg_val); \
}

#define SET_PDMA_LRO_IPV4_CTRL_PUSH_EN(x) \
{ \
unsigned int reg_val = sys_reg_read(ADMA_LRO_CTRL_DW0); \
reg_val &= ~PDMA_LRO_IPV4_CTRL_PUSH_EN;   \
reg_val |= ((x) & 0x1) << PDMA_LRO_IPV4_CTRL_PUSH_EN_OFFSET;  \
sys_reg_write(ADMA_LRO_CTRL_DW0, reg_val); \
}

#define SET_PDMA_NON_LRO_MULTI_EN(x) \
{ \
unsigned int reg_val = sys_reg_read(ADMA_LRO_CTRL_DW0); \
reg_val &= ~(PDMA_NON_LRO_MULTI_EN);   \
reg_val |= ((x) & 0x1) << PDMA_NON_LRO_MULTI_EN_OFFSET;  \
sys_reg_write(ADMA_LRO_CTRL_DW0, reg_val); \
}

#define SET_PDMA_LRO_FREQ_PRI_ADJ(x) \
{ \
unsigned int reg_val = sys_reg_read(ADMA_LRO_CTRL_DW0); \
reg_val &= ~PDMA_LRO_FREQ_PRI_ADJ;   \
reg_val |= ((x) & 0xf) << PDMA_LRO_FREQ_PRI_ADJ_OFFSET;  \
sys_reg_write(ADMA_LRO_CTRL_DW0, reg_val); \
}

#define SET_PDMA_LRO_TPUT_PRE_ADJ(x) \
{ \
unsigned int reg_val = sys_reg_read(ADMA_LRO_CTRL_DW0); \
reg_val &= ~PDMA_LRO_TPUT_PRE_ADJ;   \
reg_val |= ((x) & 0xf) << PDMA_LRO_TPUT_PRE_ADJ_OFFSET;  \
sys_reg_write(ADMA_LRO_CTRL_DW0, reg_val); \
}

#define SET_PDMA_LRO_TPUT_PRI_ADJ(x) \
{ \
unsigned int reg_val = sys_reg_read(ADMA_LRO_CTRL_DW0); \
reg_val &= ~PDMA_LRO_TPUT_PRI_ADJ;   \
reg_val |= ((x) & 0xf) << PDMA_LRO_TPUT_PRI_ADJ_OFFSET;  \
sys_reg_write(ADMA_LRO_CTRL_DW0, reg_val); \
}

#define SET_PDMA_LRO_ALT_SCORE_MODE(x) \
{ \
unsigned int reg_val = sys_reg_read(ADMA_LRO_CTRL_DW0); \
reg_val &= ~PDMA_LRO_ALT_SCORE_MODE;   \
reg_val |= ((x) & 0x1) << PDMA_LRO_ALT_SCORE_MODE_OFFSET;  \
sys_reg_write(ADMA_LRO_CTRL_DW0, reg_val); \
}

#define SET_PDMA_LRO_DLY_INT_EN(x) \
{ \
unsigned int reg_val = sys_reg_read(ADMA_LRO_CTRL_DW0); \
reg_val &= ~PDMA_LRO_DLY_INT_EN;   \
reg_val |= ((x) & 0x1) << PDMA_LRO_DLY_INT_EN_OFFSET;  \
sys_reg_write(ADMA_LRO_CTRL_DW0, reg_val); \
}

#define SET_PDMA_LRO_BW_THRESHOLD(x) \
{ \
unsigned int reg_val = sys_reg_read(ADMA_LRO_CTRL_DW2); \
reg_val = (x);  \
sys_reg_write(ADMA_LRO_CTRL_DW2, reg_val); \
}

#define SET_PDMA_LRO_MIN_RXD_SDL(x) \
{ \
unsigned int reg_val = sys_reg_read(ADMA_LRO_CTRL_DW3); \
reg_val &= ~PDMA_LRO_LRO_MIN_RXD_SDL0;   \
reg_val |= ((x) & 0xffff) << PDMA_LRO_LRO_MIN_RXD_SDL0_OFFSET;  \
sys_reg_write(ADMA_LRO_CTRL_DW3, reg_val); \
}

#define SET_PDMA_LRO_TPUT_OVERFLOW_ADJ(x) \
{ \
unsigned int reg_val = sys_reg_read(PDMA_LRO_ATL_OVERFLOW_ADJ); \
reg_val &= ~PDMA_LRO_TPUT_OVERFLOW_ADJ;   \
reg_val |= ((x) & 0xfffff) << PDMA_LRO_TPUT_OVERFLOW_ADJ_OFFSET;  \
sys_reg_write(PDMA_LRO_ATL_OVERFLOW_ADJ, reg_val); \
}

#define SET_PDMA_LRO_CNT_OVERFLOW_ADJ(x) \
{ \
unsigned int reg_val = sys_reg_read(PDMA_LRO_ATL_OVERFLOW_ADJ); \
reg_val &= ~PDMA_LRO_CNT_OVERFLOW_ADJ;   \
reg_val |= ((x) & 0xfff) << PDMA_LRO_CNT_OVERFLOW_ADJ_OFFSET;  \
sys_reg_write(PDMA_LRO_ATL_OVERFLOW_ADJ, reg_val); \
}

#define SET_PDMA_LRO_ALT_REFRESH_TIMER_UNIT(x) \
{ \
unsigned int reg_val = sys_reg_read(LRO_ALT_REFRESH_TIMER); \
reg_val &= ~PDMA_LRO_ALT_TICK_TIMER;   \
reg_val |= ((x) & 0x1f) << PDMA_LRO_ALT_TICK_TIMER_OFFSET;  \
sys_reg_write(LRO_ALT_REFRESH_TIMER, reg_val); \
}

#define SET_PDMA_LRO_ALT_REFRESH_TIMER(x) \
{ \
unsigned int reg_val = sys_reg_read(LRO_ALT_REFRESH_TIMER); \
reg_val &= ~0xffff;   \
reg_val |= ((x) & 0xffff);  \
sys_reg_write(LRO_ALT_REFRESH_TIMER, reg_val); \
}

#define SET_PDMA_LRO_MAX_AGG_TIME(x) \
{ \
unsigned int reg_val = sys_reg_read(LRO_MAX_AGG_TIME); \
reg_val &= ~0xffff;   \
reg_val |= ((x) & 0xffff);  \
sys_reg_write(LRO_MAX_AGG_TIME, reg_val); \
}

#define SET_PDMA_RXRING_MODE(x, y) \
{ \
unsigned int reg_val = sys_reg_read(LRO_RX_RING0_CTRL_DW2 + ((x) << 6)); \
reg_val &= ~(0x3 << PDMA_RX_MODE_OFFSET);   \
reg_val |= (y) << PDMA_RX_MODE_OFFSET;  \
sys_reg_write(LRO_RX_RING0_CTRL_DW2 + ((x) << 6), reg_val); \
}

#define SET_PDMA_RXRING_MYIP_VALID(x, y) \
{ \
unsigned int reg_val = sys_reg_read(LRO_RX_RING0_CTRL_DW2 + ((x) << 6)); \
reg_val &= ~(0x1 << PDMA_RX_MYIP_VALID_OFFSET); \
reg_val |= ((y) & 0x1) << PDMA_RX_MYIP_VALID_OFFSET;    \
sys_reg_write(LRO_RX_RING0_CTRL_DW2 + ((x) << 6), reg_val); \
}

#define SET_PDMA_RXRING_VALID(x, y) \
{ \
unsigned int reg_val = sys_reg_read(LRO_RX_RING0_CTRL_DW2 + ((x) << 6)); \
reg_val &= ~(0x1 << PDMA_RX_PORT_VALID_OFFSET); \
reg_val |= ((y) & 0x1) << PDMA_RX_PORT_VALID_OFFSET;    \
sys_reg_write(LRO_RX_RING0_CTRL_DW2 + ((x) << 6), reg_val); \
}

#define SET_PDMA_RXRING_TCP_SRC_PORT(x, y) \
{ \
unsigned int reg_val = sys_reg_read(LRO_RX_RING1_STP_DTP_DW + \
				    (((x) - 1) << 6)); \
reg_val &= ~(0xffff << PDMA_RX_TCP_SRC_PORT_OFFSET);    \
reg_val |= (y) << PDMA_RX_TCP_SRC_PORT_OFFSET;    \
sys_reg_write(LRO_RX_RING1_STP_DTP_DW + (((x) - 1) << 6), reg_val); \
}

#define SET_PDMA_RXRING_TCP_DEST_PORT(x, y) \
{ \
unsigned int reg_val = sys_reg_read(LRO_RX_RING1_STP_DTP_DW + \
				    (((x) - 1) << 6)); \
reg_val &= ~(0xffff << PDMA_RX_TCP_DEST_PORT_OFFSET);    \
reg_val |= (y) << PDMA_RX_TCP_DEST_PORT_OFFSET;    \
sys_reg_write(LRO_RX_RING1_STP_DTP_DW + (((x) - 1) << 6), reg_val); \
}

#define SET_PDMA_RXRING_IPV4_FORCE_MODE(x, y) \
{ \
unsigned int reg_val = sys_reg_read(LRO_RX_RING1_CTRL_DW0 + (((x) - 1) << 6)); \
reg_val &= ~(0x1 << PDMA_RX_IPV4_FORCE_OFFSET);    \
reg_val |= (y) << PDMA_RX_IPV4_FORCE_OFFSET;    \
sys_reg_write(LRO_RX_RING1_CTRL_DW0 + (((x) - 1) << 6), reg_val); \
}

#define SET_PDMA_RXRING_IPV6_FORCE_MODE(x, y) \
{ \
unsigned int reg_val = sys_reg_read(LRO_RX_RING1_CTRL_DW0 + (((x) - 1) << 6)); \
reg_val &= ~(0x1 << PDMA_RX_IPV6_FORCE_OFFSET);    \
reg_val |= (y) << PDMA_RX_IPV6_FORCE_OFFSET;    \
sys_reg_write(LRO_RX_RING1_CTRL_DW0 + (((x) - 1) << 6), reg_val); \
}

#define SET_PDMA_RXRING_AGE_TIME(x, y) \
{ \
unsigned int reg_val1 = sys_reg_read(LRO_RX_RING0_CTRL_DW1 + ((x) << 6)); \
unsigned int reg_val2 = sys_reg_read(LRO_RX_RING0_CTRL_DW2 + ((x) << 6)); \
reg_val1 &= ~PDMA_LRO_RING_AGE1;    \
reg_val2 &= ~PDMA_LRO_RING_AGE2;    \
reg_val1 |= ((y) & 0x3ff) << PDMA_LRO_RING_AGE1_OFFSET;    \
reg_val2 |= (((y) >> PDMA_LRO_AGE_H_OFFSET) & 0x03f) << \
	    PDMA_LRO_RING_AGE2_OFFSET;\
sys_reg_write(LRO_RX_RING0_CTRL_DW1 + ((x) << 6), reg_val1); \
sys_reg_write(LRO_RX_RING0_CTRL_DW2 + ((x) << 6), reg_val2); \
}

#define SET_PDMA_RXRING_AGG_TIME(x, y) \
{ \
unsigned int reg_val = sys_reg_read(LRO_RX_RING0_CTRL_DW2 + ((x) << 6)); \
reg_val &= ~PDMA_LRO_RING_AGG;    \
reg_val |= ((y) & 0xffff) << PDMA_LRO_RING_AGG_OFFSET;    \
sys_reg_write(LRO_RX_RING0_CTRL_DW2 + ((x) << 6), reg_val); \
}

#define SET_PDMA_RXRING_MAX_AGG_CNT(x, y) \
{ \
unsigned int reg_val1 = sys_reg_read(LRO_RX_RING1_CTRL_DW2 + \
				     (((x) - 1) << 6)); \
unsigned int reg_val2 = sys_reg_read(LRO_RX_RING1_CTRL_DW3 + \
				     (((x) - 1) << 6)); \
reg_val1 &= ~PDMA_LRO_RING_AGG_CNT1;    \
reg_val2 &= ~PDMA_LRO_RING_AGG_CNT2;    \
reg_val1 |= ((y) & 0x3f) << PDMA_LRO_RING_AGG_CNT1_OFFSET;    \
reg_val2 |= (((y) >> PDMA_LRO_AGG_CNT_H_OFFSET) & 0x03) << \
	     PDMA_LRO_RING_AGG_CNT2_OFFSET;    \
sys_reg_write(LRO_RX_RING1_CTRL_DW2 + (((x) - 1) << 6), reg_val1); \
sys_reg_write(LRO_RX_RING1_CTRL_DW3 + (((x) - 1) << 6), reg_val2); \
}

/* HW LRO debug functions */
void hw_lro_stats_update(unsigned int ring_num,
			 struct PDMA_rxdesc *rx_ring);
void hw_lro_flush_stats_update(unsigned int ring_num,
			       struct PDMA_rxdesc *rx_ring);

#endif
