/*
 * Copyright (c) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef _DT_BINDINGS_RESET_CONTROLLER_MT7622
#define _DT_BINDINGS_RESET_CONTROLLER_MT7622

/* TOPRGU resets */
#define MT7622_TOPRGU_MM_RST		1
#define MT7622_TOPRGU_MFG_RST		2
#define MT7622_TOPRGU_VENC_RST		3
#define MT7622_TOPRGU_VDEC_RST		4
#define MT7622_TOPRGU_IMG_RST		5
#define MT7622_TOPRGU_MD_RST		7
#define MT7622_TOPRGU_CONN_RST		9
#define MT7622_TOPRGU_C2K_SW_RST	14
#define MT7622_TOPRGU_C2K_RST		15

/* PCIe/SATA subsys resets */
#define MT7622_SATA_PHY_REG_RST		12
#define MT7622_SATA_PHY_SW_RST		13
#define MT7622_SATA_AXI_BUS_RST		15
#define MT7622_PCIE1_CORE_RST		19
#define MT7622_PCIE1_MMIO_RST		20
#define MT7622_PCIE1_HRST		21
#define MT7622_PCIE1_USER_RST		22
#define MT7622_PCIE1_PIPE_RST		23
#define MT7622_PCIE0_CORE_RST		27
#define MT7622_PCIE0_MMIO_RST		28
#define MT7622_PCIE0_HRST		29
#define MT7622_PCIE0_USER_RST		30
#define MT7622_PCIE0_PIPE_RST		31

#endif  /* _DT_BINDINGS_RESET_CONTROLLER_MT7622 */
