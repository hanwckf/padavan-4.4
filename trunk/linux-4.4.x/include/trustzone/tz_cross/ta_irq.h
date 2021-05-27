/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

/* IRQ/FIQ TA functions
 */

#ifndef __TRUSTZONE_TA_IRQ__
#define __TRUSTZONE_TA_IRQ__

#define TZ_TA_IRQ_UUID   "0be96652-d723-11e2-b1a4-d485645c4310"

/* Command for IRQ TA */
#define TZCMD_IRQ_SET_FIQ         0
#define TZCMD_IRQ_ENABLE_FIQ      1
#define TZCMD_IRQ_GET_INTACK      2
#define TZCMD_IRQ_EOI             3
#define TZCMD_IRQ_TRIGGER_SGI     4
#define TZCMD_IRQ_MASK_ALL        5
#define TZCMD_IRQ_MASK_RESTORE    6
#define TZCMD_IRQ_QUERY_FIQ       7


/* TZ Flags for TZCMD_IRQ_SET_FIQ */
#define TZ_IRQF_EDGE_SENSITIVE    0
#define TZ_IRQF_LEVEL_SENSITIVE   1
#define TZ_IRQF_LOW               0
#define TZ_IRQF_HIGH              2

#endif				/* __TRUSTZONE_TA_IRQ__ */
