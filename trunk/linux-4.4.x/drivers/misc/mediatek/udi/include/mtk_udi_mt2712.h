/*
* Copyright (C) 2017 MediaTek Inc.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See http://www.gnu.org/licenses/gpl-2.0.html for more details.
*/

#ifndef __MTK_UDI_MT2712__
#define __MTK_UDI_MT2712__

#include <linux/kernel.h>

/* UDI pin mux ADDR */
/* 1. Write 0x100055C0 = 0x00002400 to switch TCK, TDI pins */
/* 2. Write 0x100055D0 = 0x00000092 to switch TMS, TDO, NTRST pins */

#ifdef CONFIG_OF
#define DEVICE_GPIO "mediatek,mt2712-pctl-a-syscfg"
static void __iomem  *udipin_base;         /* 0x10005000 0x1000, UDI pinmux reg */
#endif

#ifdef __KERNEL__
#define UDIPIN_BASE				(udipin_base)
#else
#define UDIPIN_BASE				0x10005000
#endif

/* 0x10005000 0x1000, UDI pinmux reg */
#define UDIPIN_UDI_MUX1			(UDIPIN_BASE+0x5C0)
#define UDIPIN_UDI_MUX1_VALUE		(0x00002400)
#define UDIPIN_UDI_MUX2			(UDIPIN_BASE+0x5D0)
#define UDIPIN_UDI_MUX2_VALUE		(0x00000092)

#endif /* __MTK_UDI_MT2712__ */

