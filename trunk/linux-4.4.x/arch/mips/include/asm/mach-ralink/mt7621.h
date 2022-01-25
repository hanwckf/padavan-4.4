/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 *
 * Parts of this file are based on Ralink's 2.6.21 BSP
 *
 * Copyright (C) 2008-2011 Gabor Juhos <juhosg@openwrt.org>
 * Copyright (C) 2008 Imre Kaloz <kaloz@openwrt.org>
 * Copyright (C) 2013 John Crispin <blogic@openwrt.org>
 */

#ifndef _MT7621_REGS_H_
#define _MT7621_REGS_H_

#define MT7621_PALMBUS_BASE		0x1C000000
#define MT7621_PALMBUS_SIZE		0x03FFFFFF

#define MT7621_SYSC_BASE		0x1E000000

#define SYSC_REG_CHIP_NAME0		0x00
#define SYSC_REG_CHIP_NAME1		0x04
#define SYSC_REG_CHIP_REV		0x0c
#define SYSC_REG_SYSTEM_CONFIG0		0x10
#define SYSC_REG_SYSTEM_CONFIG1		0x14
#define SYSC_REG_CLOCK_CONFIG1		0x30

#define CHIP_CPU_ID_MASK		0x1
#define CHIP_CPU_ID_SHIFT		17
#define CHIP_REV_PKG_MASK		0x1
#define CHIP_REV_PKG_SHIFT		16
#define CHIP_REV_VER_MASK		0xf
#define CHIP_REV_VER_SHIFT		8
#define CHIP_REV_ECO_MASK		0xf

#define XTAL_MODE_SEL_MASK		0x07
#define XTAL_MODE_SEL_SHIFT		6

#define SYSC_CLK1_PCI0_SHIFT		24
#define SYSC_CLK1_PCI1_SHIFT		25
#define SYSC_CLK1_PCI2_SHIFT		26

#define MT7621_DRAM_BASE                0x0
#define MT7621_DRAM_SIZE_MIN		32
#define MT7621_DRAM_SIZE_MAX		512

#define MT7621_CHIP_NAME0		0x3637544D
#define MT7621_CHIP_NAME1		0x20203132

#define MIPS_GIC_IRQ_BASE           (MIPS_CPU_IRQ_BASE + 8)

#endif
