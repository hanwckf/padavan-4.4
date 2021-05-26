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
#ifndef RA_RSS_H
#define RA_RSS_H

#include "raeth_reg.h"

#define NUM_RSS_RX_DESC   1024
#define MAX_RX_RING_NUM_2RING 2

/******RSS define*******/
#define PDMA_RSS_EN             BIT(0)
#define PDMA_RSS_BUSY		BIT(1)
#define PDMA_RSS_CFG_REQ	BIT(2)
#define PDMA_RSS_CFG_RDY	BIT(3)
#define PDMA_RSS_INDR_TBL_SIZE		BITS(4, 6)
#define PDMA_RSS_IPV6_TYPE		BITS(8, 10)
#define PDMA_RSS_IPV4_TYPE		BITS(12, 14)
#define PDMA_RSS_IPV6_TUPLE_EN		BITS(16, 20)
#define PDMA_RSS_IPV4_TUPLE_EN		BITS(24, 28)

#define PDMA_RSS_EN_OFFSET        (0)
#define PDMA_RSS_BUSY_OFFSET      (1)
#define PDMA_RSS_CFG_REQ_OFFSET	  (2)
#define PDMA_RSS_CFG_RDY_OFFSET	  (3)
#define PDMA_RSS_INDR_TBL_SIZE_OFFSET	(4)
#define PDMA_RSS_IPV6_TYPE_OFFSET	(8)
#define PDMA_RSS_IPV4_TYPE_OFFSET	(12)
#define PDMA_RSS_IPV6_TUPLE_EN_OFFSET	(16)
#define PDMA_RSS_IPV4_TUPLE_EN_OFFSET	(24)

#define SET_PDMA_RSS_EN(x) \
{ \
unsigned int reg_val = sys_reg_read(ADMA_RSS_GLO_CFG); \
reg_val &= ~(PDMA_RSS_EN);   \
reg_val |= ((x) & 0x1) << PDMA_RSS_EN_OFFSET;  \
sys_reg_write(ADMA_RSS_GLO_CFG, reg_val); \
}

#define SET_PDMA_RSS_CFG_REQ(x) \
{ \
unsigned int reg_val = sys_reg_read(ADMA_RSS_GLO_CFG); \
reg_val &= ~(PDMA_RSS_CFG_REQ);   \
reg_val |= ((x) & 0x1) << PDMA_RSS_CFG_REQ_OFFSET;  \
sys_reg_write(ADMA_RSS_GLO_CFG, reg_val); \
}

#define SET_PDMA_RSS_IPV4_TYPE(x) \
{ \
unsigned int reg_val = sys_reg_read(ADMA_RSS_GLO_CFG); \
reg_val &= ~(PDMA_RSS_IPV4_TYPE);   \
reg_val |= ((x) & 0x7) << PDMA_RSS_IPV4_TYPE_OFFSET;  \
sys_reg_write(ADMA_RSS_GLO_CFG, reg_val); \
}

#define SET_PDMA_RSS_IPV6_TYPE(x) \
{ \
unsigned int reg_val = sys_reg_read(ADMA_RSS_GLO_CFG); \
reg_val &= ~(PDMA_RSS_IPV6_TYPE);   \
reg_val |= ((x) & 0x7) << PDMA_RSS_IPV6_TYPE_OFFSET;  \
sys_reg_write(ADMA_RSS_GLO_CFG, reg_val); \
}

#define SET_PDMA_RSS_IPV4_TUPLE_TYPE(x) \
{ \
unsigned int reg_val = sys_reg_read(ADMA_RSS_GLO_CFG); \
reg_val &= ~(PDMA_RSS_IPV4_TYPE);   \
reg_val |= ((x) & 0x7) << PDMA_RSS_IPV4_TUPLE_EN_OFFSET;  \
sys_reg_write(ADMA_RSS_GLO_CFG, reg_val); \
}

#define SET_PDMA_RSS_IPV6_TUPLE_TYPE(x) \
{ \
unsigned int reg_val = sys_reg_read(ADMA_RSS_GLO_CFG); \
reg_val &= ~(PDMA_RSS_IPV6_TYPE);   \
reg_val |= ((x) & 0x7) << PDMA_RSS_IPV6_TUPLE_EN_OFFSET;  \
sys_reg_write(ADMA_RSS_GLO_CFG, reg_val); \
}

#define SET_PDMA_RSS_INDR_TBL_SIZE(x) \
{ \
unsigned int reg_val = sys_reg_read(ADMA_RSS_GLO_CFG); \
reg_val &= ~(PDMA_RSS_INDR_TBL_SIZE);   \
reg_val |= ((x) & 0x7) << PDMA_RSS_INDR_TBL_SIZE_OFFSET;  \
sys_reg_write(ADMA_RSS_GLO_CFG, reg_val); \
}

#define SET_PDMA_RSS_CR_VALUE(x, y) \
{ \
unsigned int reg_val = y; \
sys_reg_write(x, reg_val); \
}

#endif
