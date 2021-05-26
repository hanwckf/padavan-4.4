/*
 * MediaTek MT7621 PCIe host support
 *
 * Copyright (C) 2018 MediaTek Inc.
 * Author: Ryder Lee <ryder.lee@mediatek.com>
 *	   Weijie Gao <weijie.gao@mediatek.com>
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

#include <asm/addrspace.h>
#include <linux/irq.h>
#include <linux/pci.h>
#include <linux/reset.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/bitops.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <asm/mach-ralink/mt7621.h>
#include <asm/mach-ralink/ralink_regs.h>

#define PCIE_IO_PORT_BASE		0x1e160000
#define PCIE_IO_PORT_SIZE		0x10000

#define PCIE_IO_MEM_BASE		0x60000000
#define PCIE_IO_MEM_SIZE		0x10000000

#define PCIE_CONFIG_REG			0x00
#define PCIE_P2P_BR_DEVNUM_SHIFT(p)	(16 + (p) * 4)
#define PCIE_P2P_BR_DEVNUM0_SHIFT	PCIE_P2P_BR_DEVNUM_SHIFT(0)
#define PCIE_P2P_BR_DEVNUM1_SHIFT	PCIE_P2P_BR_DEVNUM_SHIFT(1)
#define PCIE_P2P_BR_DEVNUM2_SHIFT	PCIE_P2P_BR_DEVNUM_SHIFT(2)
#define PCIE_P2P_BR_DEVNUM_MASK		0xf
#define PCIE_P2P_BR_DEVNUM_MASK_FULL	(0xfff << PCIE_P2P_BR_DEVNUM0_SHIFT)
#define PCIE_EP_RESET_L			BIT(1)

#define PCIE_INT_ENABLE_REG		0x0c
#define PCIE_INT_PORT(p)		BIT(20 + (p))
#define PCIE_INT_PORT0			PCIE_INT_PORT(0)
#define PCIE_INT_PORT1			PCIE_INT_PORT(1)
#define PCIE_INT_PORT2			PCIE_INT_PORT(2)

#define PCIE_CFGADDR_REG		0x20
#define PCIE_EXTREGNUM_SHIFT		24
#define PCIE_EXTREGNUM_MASK		0xf
#define PCIE_BUSNUM_SHIFT		16
#define PCIE_BUSNUM_MASK		0xff
#define PCIE_DEVICENUM_SHIFT		11
#define PCIE_DEVICENUM_MASK		0x1f
#define PCIE_FUNNUM_SHIFT		8
#define PCIE_FUNNUM_MASK		0x7
#define PCIE_REGNUM_SHIFT		2
#define PCIE_REGNUM_MASK		0x3f

#define PCIE_CFGDATA_REG		0x24

#define PCIE_MEMBASE_REG		0x28
#define PCIE_IOBASE_REG			0x2c

/* PCIe Port registers */
#define PCIE_PORT_REG_BASE(p)		(0x1000 * ((p) + 2))

#define PCIE_BAR0SETUP_REG(p)		(PCIE_PORT_REG_BASE(p) + 0x10)
#define PCIE_BAR1SETUP_REG(p)		(PCIE_PORT_REG_BASE(p) + 0x14)
#define PCIE_BARMSK_SHIFT		16
#define PCIE_BARMSK_MASK		0xffff
#define PCIE_BAR_ENABLE			BIT(0)

#define PCIE_IMBASEBAR0_REG(p)		(PCIE_PORT_REG_BASE(p) + 0x18)
#define PCIE_IMBASEBAR0_SHIFT		15
#define PCIE_IMBASEBAR0_MASK		0x1ffff

#define PCIE_PORT_CLASS_REG(p)		(PCIE_PORT_REG_BASE(p) + 0x34)
#define PCIE_CCODE_SHIFT		8
#define PCIE_CCODE_MASK			0xffffff
#define PCIE_REVID_SHIFT		0
#define PCIE_REVID_MASK			0xff

#define PCIE_PORT_STATUS_REG(p)		(PCIE_PORT_REG_BASE(p) + 0x50)
#define PCIE_PORT_LINK_UP		BIT(0)

/* PCIe PHY registers */
#define PCIEPHY_P0P1_CTL_REG		0x9000
#define PCIEPHY_P2_CTL_REG		0xA000

#define PE1_PIPE_RST			BIT(12)
#define PE1_PIPE_CMD_FRC		BIT(4)

#define PE1_FORCE_H_XTAL_TYPE		BIT(8)
#define PE1_H_XTAL_TYPE_SHIFT		9
#define PE1_H_XTAL_TYPE_MASK		0x3

#define PE1_FORCE_PHY_EN		BIT(4)
#define PE1_PHY_EN			BIT(5)

#define PE1_H_PLL_PREDIV_SHIFT		6
#define PE1_H_PLL_PREDIV_MASK		0x3

#define PE1_H_PLL_FBKSEL_SHIFT		4
#define PE1_H_PLL_FBKSEL_MASK		0x3

#define PE1_H_LCDDS_PCW_NCPO_SHIFT	0
#define PE1_H_LCDDS_PCW_NCPO_MASK	0xffffffff

#define PE1_H_LCDDS_SSC_PRD_SHIFT	0
#define PE1_H_LCDDS_SSC_PRD_MASK	0xffff

#define PE1_H_LCDDS_SSC_DELTA_SHIFT	0
#define PE1_H_LCDDS_SSC_DELTA_MASK	0xfff

#define PE1_H_LCDDS_SSC_DELTA1_SHIFT	16
#define PE1_H_LCDDS_SSC_DELTA1_MASK	0xfff

#define PE1_LCDDS_CLK_PH_INV		BIT(5)

#define PE1_H_PLL_BC_SHIFT		22
#define PE1_H_PLL_BC_MASK		0x3
#define PE1_H_PLL_BP_SHIFT		18
#define PE1_H_PLL_BP_MASK		0xf
#define PE1_H_PLL_IR_SHIFT		12
#define PE1_H_PLL_IR_MASK		0xf
#define PE1_H_PLL_IC_SHIFT		8
#define PE1_H_PLL_IC_MASK		0xf

#define PE1_H_PLL_BR_SHIFT		16
#define PE1_H_PLL_BR_MASK		0x7

#define PE1_PLL_DIVEN_SHIFT		1
#define PE1_PLL_DIVEN_MASK		0x7

#define PE1_MSTCKDIV_SHIFT		6
#define PE1_MSTCKDIV_MASK		0x3
#define PE1_FORCE_MSTCKDIV		BIT(5)

#define PE1_CRTMSEL_SHIFT		17
#define PE1_CRTMSEL_MASK		0xf
#define PE1_FORCE_CRTMSEL		BIT(16)

struct mt7621_pci_controller {
	void __iomem *base;

	int is_mt7621_e2;

	int irq[3];
	int irq_map[3];
	uint32_t link_status;
	unsigned reset_gpio[3];
	struct reset_control *rstctrl[3];
	spinlock_t lock;

	struct pci_controller pci_controller;
};

static struct resource mt7621_pci_mem_res = {
	.name = "PCI MEM space",
	.start = PCIE_IO_MEM_BASE,
	.end = PCIE_IO_MEM_BASE + PCIE_IO_MEM_SIZE - 1,
	.flags = IORESOURCE_MEM,
};

static struct resource mt7621_pci_io_res = {
	.name = "PCI IO space",
	.start = PCIE_IO_PORT_BASE,
	.end = PCIE_IO_PORT_BASE + PCIE_IO_PORT_SIZE - 1,
	.flags = IORESOURCE_IO,
};

static void mt7621_pci_wr(struct mt7621_pci_controller *mpc, u32 reg, u32 val)
{
	__raw_writel(val, mpc->base + reg);
}

static u32 mt7621_pci_rd(struct mt7621_pci_controller *mpc, u32 reg)
{
	return __raw_readl(mpc->base + reg);
}

static void mt7621_pci_rmw(struct mt7621_pci_controller *mpc,
				u32 reg, u32 clr, u32 set)
{
	u32 val = __raw_readl(mpc->base + reg);

	val &= ~clr;
	val |= set;
	__raw_writel(val, mpc->base + reg);
}

static inline struct mt7621_pci_controller *
pci_bus_to_mt7621_pci_controller(struct pci_bus *bus)
{
	struct pci_controller *hose = (struct pci_controller *) bus->sysdata;

	return container_of(hose, struct mt7621_pci_controller,
		pci_controller);
}

static uint32_t pci_to_cfgaddr(uint32_t bus, uint32_t slot, uint32_t func,
				int where)
{
	u32 regm = (where >> 8) & 0xf;
	u32 regl = (where >> 2) & 0x3f;

	return ((regm & PCIE_EXTREGNUM_MASK) << PCIE_EXTREGNUM_SHIFT) |
		((bus & PCIE_BUSNUM_MASK) << PCIE_BUSNUM_SHIFT) |
		((slot & PCIE_DEVICENUM_MASK) << PCIE_DEVICENUM_SHIFT) |
		((func & PCIE_FUNNUM_MASK) << PCIE_FUNNUM_SHIFT) |
		((regl & PCIE_REGNUM_MASK) << PCIE_REGNUM_SHIFT) |
		0x80000000;
}

static int __mt7621_pci_config_read(struct mt7621_pci_controller *mpc,
					uint32_t bus, uint32_t slot,
					uint32_t func, int where, int size,
					uint32_t *value)
{
	unsigned long flags;
	u32 cfgaddr = pci_to_cfgaddr(bus, slot, func, where);
	u32 data;

	spin_lock_irqsave(&mpc->lock, flags);
	mt7621_pci_wr(mpc, PCIE_CFGADDR_REG, cfgaddr);
	data = mt7621_pci_rd(mpc, PCIE_CFGDATA_REG);
	spin_unlock_irqrestore(&mpc->lock, flags);

	switch (size) {
	case 1:
		*value = (data >> ((where & 0x3) << 3)) & 0xff;
		break;
	case 2:
		*value = (data >> ((where & 0x3) << 3)) & 0xffff;
		break;
	case 4:
		*value = data;
		break;
	}

	return PCIBIOS_SUCCESSFUL;
}

static int __mt7621_pci_config_write(struct mt7621_pci_controller *mpc,
					uint32_t bus, uint32_t slot,
					uint32_t func, int where, int size,
					uint32_t value)
{
	unsigned long flags;
	u32 cfgaddr = pci_to_cfgaddr(bus, slot, func, where);
	u32 data;

	spin_lock_irqsave(&mpc->lock, flags);
	mt7621_pci_wr(mpc, PCIE_CFGADDR_REG, cfgaddr);
	data = mt7621_pci_rd(mpc, PCIE_CFGDATA_REG);

	switch (size) {
	case 1:
		data = (data & ~(0xff << ((where & 0x3) << 3))) |
		       (value << ((where & 0x3) << 3));
		break;
	case 2:
		data = (data & ~(0xffff << ((where & 0x3) << 3))) |
		       (value << ((where & 0x3) << 3));
		break;
	case 4:
		data = value;
		break;
	}

	mt7621_pci_wr(mpc, PCIE_CFGDATA_REG, data);
	spin_unlock_irqrestore(&mpc->lock, flags);

	return PCIBIOS_SUCCESSFUL;
}

static int mt7621_pci_config_read(struct pci_bus *bus, unsigned int devfn,
				int where, int size, uint32_t *value)
{
	struct mt7621_pci_controller *mpc;
	u32 slot = PCI_SLOT(devfn);
	u32 func = PCI_FUNC(devfn);

	mpc = pci_bus_to_mt7621_pci_controller(bus);

	return __mt7621_pci_config_read(mpc,
		bus->number, slot, func, where, size, value);
}

static int mt7621_pci_config_write(struct pci_bus *bus, unsigned int devfn,
				int where, int size, uint32_t value)
{
	struct mt7621_pci_controller *mpc;
	u32 slot = PCI_SLOT(devfn);
	u32 func = PCI_FUNC(devfn);

	mpc = pci_bus_to_mt7621_pci_controller(bus);

	return __mt7621_pci_config_write(mpc,
		bus->number, slot, func, where, size, value);
}

static struct pci_ops mt7621_pci_ops = {
	.read =  mt7621_pci_config_read,
	.write = mt7621_pci_config_write,
};

static void mt7621_pci_bypass_pipe_rst(struct mt7621_pci_controller *mpc)
{
	/* PCIe port 0 */
	mt7621_pci_rmw(mpc, PCIEPHY_P0P1_CTL_REG + 0x2c,
			0, PE1_PIPE_RST);
	mt7621_pci_rmw(mpc, PCIEPHY_P0P1_CTL_REG + 0x2c,
			0, PE1_PIPE_CMD_FRC);

	/* PCIe port 1 */
	mt7621_pci_rmw(mpc, PCIEPHY_P0P1_CTL_REG + 0x12c,
			0, PE1_PIPE_RST);
	mt7621_pci_rmw(mpc, PCIEPHY_P0P1_CTL_REG + 0x12c,
			0, PE1_PIPE_CMD_FRC);

	/* PCIe port 2 */
	mt7621_pci_rmw(mpc, PCIEPHY_P2_CTL_REG + 0x2c,
			0, PE1_PIPE_RST);
	mt7621_pci_rmw(mpc, PCIEPHY_P2_CTL_REG + 0x2c,
			0, PE1_PIPE_CMD_FRC);
}

static void __mt7621_pci_phy_ssc_config(struct mt7621_pci_controller *mpc,
					u32 reg_base, int dual_port)
{
	u32 xtal_mode;

	xtal_mode = (rt_sysc_r32(SYSC_REG_SYSTEM_CONFIG0)
			>> XTAL_MODE_SEL_SHIFT) & XTAL_MODE_SEL_MASK;

	/* Set PCIe port to disable SSC */
	mt7621_pci_rmw(mpc, reg_base + 0x400,
		PE1_H_XTAL_TYPE_MASK << PE1_H_XTAL_TYPE_SHIFT,
		PE1_FORCE_H_XTAL_TYPE);
	/* Force port 0 enable control & disable port 0 */
	mt7621_pci_rmw(mpc, reg_base + 0x000, PE1_PHY_EN, PE1_FORCE_PHY_EN);

	if (dual_port) {
		/* Force port 1 enable control & disable port 1 */
		mt7621_pci_rmw(mpc, reg_base + 0x100,
			PE1_PHY_EN, PE1_FORCE_PHY_EN);
	}

	if (xtal_mode >= 3 && xtal_mode <= 5) {
		/* 40MHz Xtal */
		/* Pre-divider ratio (for host mode) */
		mt7621_pci_rmw(mpc, reg_base + 0x490,
			PE1_H_PLL_PREDIV_MASK << PE1_H_PLL_PREDIV_SHIFT,
			1 << PE1_H_PLL_PREDIV_SHIFT);
		/* SSC option tune from -5000ppm to -1000ppm */
		mt7621_pci_rmw(mpc, reg_base + 0x4a8,
			PE1_H_LCDDS_SSC_DELTA_MASK |
			(PE1_H_LCDDS_SSC_DELTA1_MASK << PE1_H_LCDDS_SSC_DELTA1_SHIFT),
			0x1a | (0x1a << PE1_H_LCDDS_SSC_DELTA1_SHIFT));
	} else if (xtal_mode >= 6) {
		/* 25MHz Xtal */
		/* Pre-divider ratio (for host mode) */
		mt7621_pci_rmw(mpc, reg_base + 0x490,
			PE1_H_PLL_PREDIV_MASK << PE1_H_PLL_PREDIV_SHIFT, 0);
		/* Feedback clock select */
		mt7621_pci_rmw(mpc, reg_base + 0x4bc,
			PE1_H_PLL_FBKSEL_MASK << PE1_H_PLL_FBKSEL_SHIFT,
			1 << PE1_H_PLL_FBKSEL_MASK);
		/* DDS NCPO PCW (for host mode) */
		mt7621_pci_rmw(mpc, reg_base + 0x49c,
			PE1_H_LCDDS_PCW_NCPO_MASK, 0x18000000);
		/* DDS SSC dither period control */
		mt7621_pci_rmw(mpc, reg_base + 0x4a4,
			PE1_H_LCDDS_SSC_PRD_MASK, 0x18d);
		/* DDS SSC dither amplitude control */
		mt7621_pci_rmw(mpc, reg_base + 0x4a8,
			PE1_H_LCDDS_SSC_DELTA_MASK |
			(PE1_H_LCDDS_SSC_DELTA1_MASK << PE1_H_LCDDS_SSC_DELTA1_SHIFT),
			0x4a | (0x4a << PE1_H_LCDDS_SSC_DELTA1_SHIFT));
		/* SSC option tune from -5000ppm to -1000ppm */
		mt7621_pci_rmw(mpc, reg_base + 0x4a8,
			PE1_H_LCDDS_SSC_DELTA_MASK |
			(PE1_H_LCDDS_SSC_DELTA1_MASK << PE1_H_LCDDS_SSC_DELTA1_SHIFT),
			0x11 | (0x11 << PE1_H_LCDDS_SSC_DELTA1_SHIFT));
	} else {
		/* 20MHz Xtal */
		/* Pre-divider ratio (for host mode) */
		mt7621_pci_rmw(mpc, reg_base + 0x490,
			PE1_H_PLL_PREDIV_MASK << PE1_H_PLL_PREDIV_SHIFT, 0);
		/* SSC option tune from -5000ppm to -1000ppm */
		mt7621_pci_rmw(mpc, reg_base + 0x4a8,
			PE1_H_LCDDS_SSC_DELTA_MASK |
			(PE1_H_LCDDS_SSC_DELTA1_MASK << PE1_H_LCDDS_SSC_DELTA1_SHIFT),
			0x1a | (0x1a << PE1_H_LCDDS_SSC_DELTA1_SHIFT));
	}

	/* DDS clock inversion */
	mt7621_pci_rmw(mpc, reg_base + 0x4a0, 0, PE1_LCDDS_CLK_PH_INV);
	mt7621_pci_rmw(mpc, reg_base + 0x490,
		(PE1_H_PLL_BC_MASK << PE1_H_PLL_BC_SHIFT) |
		(PE1_H_PLL_BP_MASK << PE1_H_PLL_BP_SHIFT) |
		(PE1_H_PLL_IR_MASK << PE1_H_PLL_IR_SHIFT) |
		(PE1_H_PLL_IC_MASK << PE1_H_PLL_IC_SHIFT),
		(2 << PE1_H_PLL_BC_SHIFT) |
		(6 << PE1_H_PLL_BP_SHIFT) |
		(2 << PE1_H_PLL_IR_SHIFT) |
		(1 << PE1_H_PLL_IC_SHIFT));
	mt7621_pci_rmw(mpc, reg_base + 0x4ac,
		PE1_H_PLL_BR_MASK << PE1_H_PLL_BR_SHIFT, 0);
	mt7621_pci_rmw(mpc, reg_base + 0x490,
		PE1_PLL_DIVEN_MASK << PE1_PLL_DIVEN_SHIFT,
		2 << PE1_PLL_DIVEN_SHIFT);

	if (xtal_mode >= 3 && xtal_mode <= 5) {
		/* 40MHz Xtal */
		/* Value of PCIE_MSTCKDIV when force mode enable */
		mt7621_pci_rmw(mpc, reg_base + 0x414,
			PE1_MSTCKDIV_MASK << PE1_MSTCKDIV_SHIFT,
			1 << PE1_MSTCKDIV_SHIFT);
		/* Force mode enable of PCIE_MSTCKDIV */
		mt7621_pci_rmw(mpc, reg_base + 0x414, 0, PE1_FORCE_MSTCKDIV);
	}

	/* Enable PHY and disable force mode */
	mt7621_pci_rmw(mpc, reg_base + 0x040,
		PE1_CRTMSEL_MASK << PE1_CRTMSEL_SHIFT,
		(7 << PE1_CRTMSEL_SHIFT) | PE1_FORCE_CRTMSEL);
	mt7621_pci_rmw(mpc, reg_base + 0x000, PE1_FORCE_PHY_EN, PE1_PHY_EN);

	if (dual_port) {
		mt7621_pci_rmw(mpc, reg_base + 0x140,
			PE1_CRTMSEL_MASK << PE1_CRTMSEL_SHIFT,
			(7 << PE1_CRTMSEL_SHIFT) | PE1_FORCE_CRTMSEL);
		mt7621_pci_rmw(mpc, reg_base + 0x100,
			PE1_FORCE_PHY_EN, PE1_PHY_EN);
	}
}

static void mt7621_pci_phy_ssc_config(struct mt7621_pci_controller *mpc)
{
	/* PCIe port 0/1 */
	__mt7621_pci_phy_ssc_config(mpc, PCIEPHY_P0P1_CTL_REG, 1);

	/* PCIe port 2 */
	__mt7621_pci_phy_ssc_config(mpc, PCIEPHY_P2_CTL_REG, 0);
}

static void setup_cm_memory_region(struct resource *mem_resource)
{
	resource_size_t mask;

	if (mips_cm_numiocu()) {
		/*
		 * FIXME: hardware doesn't accept mask values with 1s after
		 * 0s (e.g. 0xffef), so it would be great to warn if that's
		 * about to happen
		 */
		mask = ~(mem_resource->end - mem_resource->start);

		write_gcr_reg1_base(mem_resource->start);
		write_gcr_reg1_mask(mask | CM_GCR_REGn_MASK_CMTGT_IOCU0);
		pr_info("PCI coherence region base: 0x%08lx, mask/settings: 0x%08lx\n",
		       read_gcr_reg1_base(), read_gcr_reg1_mask());
	}
}

static void mt7621_pci_reset_assert(
	struct mt7621_pci_controller *mpc, u32 idx)
{
	if (idx > 2)
		return;

	if (mpc->is_mt7621_e2)
		reset_control_assert(mpc->rstctrl[idx]);
	else
		reset_control_deassert(mpc->rstctrl[idx]);
}

static void mt7621_pci_reset_deassert(
	struct mt7621_pci_controller *mpc, u32 idx)
{
	if (idx > 2)
		return;

	if (mpc->is_mt7621_e2)
		reset_control_deassert(mpc->rstctrl[idx]);
	else
		reset_control_assert(mpc->rstctrl[idx]);
}

static void mt7621_pcie_init(struct mt7621_pci_controller *mpc)
{
	int i;
	u32 val, n;
	u32 num_linked_up = 0;
	u32 p2p_br_devnum[3];

	for (i = 0; i < 3; i++) {
		/* PCIe RC reset assert */
		mt7621_pci_reset_assert(mpc, i);

		/* PCIe EP reset assert */
		if (gpio_is_valid(mpc->reset_gpio[i]))
			gpio_set_value(mpc->reset_gpio[i], 0);
	}

	mdelay(100);

	/* PCIe RC reset deassert */
	for (i = 0; i < 3; i++)
		mt7621_pci_reset_deassert(mpc, i);

	if (mpc->is_mt7621_e2)
		mt7621_pci_bypass_pipe_rst(mpc);

	mt7621_pci_phy_ssc_config(mpc);

	/* PCIe EP reset deassert */
	for (i = 0; i < 3; i++) {
		if (gpio_is_valid(mpc->reset_gpio[i]))
			gpio_set_value(mpc->reset_gpio[i], 1);
	}

	mdelay(100);

	/* Check port link status */
	for (i = 0; i < 3; i++) {
		val = mt7621_pci_rd(mpc, PCIE_PORT_STATUS_REG(i));
		if (val & PCIE_PORT_LINK_UP) {
			mpc->link_status |= BIT(i);
			mt7621_pci_rmw(mpc,
				PCIE_INT_ENABLE_REG, 0, PCIE_INT_PORT(i));
			rt_sysc_m32(0, BIT(SYSC_CLK1_PCI0_SHIFT + i),
				SYSC_REG_CLOCK_CONFIG1);
			num_linked_up++;
		} else {
			pr_info("PCIe port %d link down\n", i);
			mt7621_pci_reset_assert(mpc, i);
			rt_sysc_m32(BIT(SYSC_CLK1_PCI0_SHIFT + i), 0,
				SYSC_REG_CLOCK_CONFIG1);
		}
	}

	if (!num_linked_up) {
		pr_notice("No PCIe card found\n");
		return;
	}

	/* Assign Device number of Virtual PCI-PCI bridges */
	n = 0;

	for (i = 0; i < 3; i++)
		if (mpc->link_status & BIT(i))
			p2p_br_devnum[i] = n++;

	for (i = 0; i < 3; i++)
		if ((mpc->link_status & BIT(i)) == 0)
			p2p_br_devnum[i] = n++;

	mt7621_pci_rmw(mpc, PCIE_CONFIG_REG,
			PCIE_P2P_BR_DEVNUM_MASK_FULL,
			(p2p_br_devnum[0] << PCIE_P2P_BR_DEVNUM0_SHIFT) |
			(p2p_br_devnum[1] << PCIE_P2P_BR_DEVNUM1_SHIFT) |
			(p2p_br_devnum[2] << PCIE_P2P_BR_DEVNUM2_SHIFT));

	/* Assign IRQs */
	n = 0;

	for (i = 0; i < 3; i++)
		if (mpc->link_status & BIT(i))
			mpc->irq_map[n++] = mpc->irq[i];

	for (i = n; i < 3; i++)
		mpc->irq_map[i] = -1;

	/* Setup MEMWIN and IOWIN */
	mt7621_pci_wr(mpc, PCIE_MEMBASE_REG, 0xffffffff);
	mt7621_pci_wr(mpc, PCIE_IOBASE_REG, PCIE_IO_PORT_BASE);

	/* Setup PCIe ports */
	for (i = 0; i < 3; i++) {
		if ((mpc->link_status & BIT(i)) == 0)
			continue;

		mt7621_pci_wr(mpc, PCIE_BAR0SETUP_REG(i),
			(PCIE_BARMSK_MASK << PCIE_BARMSK_SHIFT) | PCIE_BAR_ENABLE);
		mt7621_pci_wr(mpc, PCIE_IMBASEBAR0_REG(i), 0);
		mt7621_pci_wr(mpc, PCIE_PORT_CLASS_REG(i),
			(0x60400 << PCIE_CCODE_SHIFT) | (1 << PCIE_REVID_SHIFT));
	}

	for (i = 0; i < num_linked_up; i++) {
		__mt7621_pci_config_read(mpc, 0, i, 0, PCI_COMMAND, 4, &val);
		val |= PCI_COMMAND_MASTER;
		__mt7621_pci_config_write(mpc, 0, i, 0, PCI_COMMAND, 4, val);

		__mt7621_pci_config_read(mpc, 0, i, 0, 0x70c, 4, &val);
		val &= ~(0xff << 8);
		val |= 0x50 << 8;
		__mt7621_pci_config_write(mpc, 0, i, 0, 0x70c, 4, val);
	}
}

int __init pcibios_map_irq(const struct pci_dev *dev, u8 slot, u8 pin)
{
	struct mt7621_pci_controller *mpc;
	u16 cmd;
	int irq = 0;

	mpc = pci_bus_to_mt7621_pci_controller(dev->bus);

	switch (dev->bus->number) {
	case 0:
		__mt7621_pci_config_write(mpc, 0, slot, 0,
					PCI_BASE_ADDRESS_0, 4, 0);
		break;
	case 1:
	case 2:
	case 3:
		if (slot >= dev->bus->number) {
			pr_notice("pcibios_map_irq: invalid slot %d for bus %d\n",
				slot, dev->bus->number);
			return -1;
		}

		irq = mpc->irq_map[slot];
		if (irq < 0) {
			pr_notice("pcibios_map_irq: slot %d for bus %d is link-down\n",
				slot, dev->bus->number);
			return -1;
		}

		break;
	default:
		pr_notice("pcibios_map_irq: invalid bus number %d\n", dev->bus->number);
		return -1;
	}

	pci_write_config_byte(dev, PCI_CACHE_LINE_SIZE, 0x14);
	pci_write_config_byte(dev, PCI_LATENCY_TIMER, 0xFF);

	pci_read_config_word(dev, PCI_COMMAND, &cmd);
	cmd = cmd | PCI_COMMAND_MASTER | PCI_COMMAND_IO | PCI_COMMAND_MEMORY;
	pci_write_config_word(dev, PCI_COMMAND, cmd);

	return irq;
}

static int mt7621_pci_probe(struct platform_device *pdev)
{
	struct mt7621_pci_controller *mpc;
	struct resource *r;
	char name[32];
	u32 rev;
	int ret, i;

	mpc = devm_kzalloc(&pdev->dev,
			   sizeof(struct mt7621_pci_controller),
			   GFP_KERNEL);
	if (!mpc)
		return -ENOMEM;

	spin_lock_init(&mpc->lock);

	rev = rt_sysc_r32(SYSC_REG_CHIP_REV);
	if (((rev >> CHIP_REV_VER_SHIFT) & CHIP_REV_VER_MASK) == 1 &&
	    (rev & CHIP_REV_ECO_MASK) == 1)
		mpc->is_mt7621_e2 = 1;

	r = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	mpc->base = devm_ioremap_resource(&pdev->dev, r);
	if (IS_ERR(mpc->base))
		return PTR_ERR(mpc->base);

	mpc->pci_controller.pci_ops = &mt7621_pci_ops;
	mpc->pci_controller.io_resource = &mt7621_pci_io_res;
	mpc->pci_controller.mem_resource = &mt7621_pci_mem_res;
	mpc->pci_controller.io_map_base =
		(unsigned long) ioremap(PCIE_IO_PORT_BASE, PCIE_IO_PORT_SIZE);

	set_io_port_base(mpc->pci_controller.io_map_base);

	ioport_resource.start = PCIE_IO_PORT_BASE;
	ioport_resource.end = PCIE_IO_PORT_BASE + PCIE_IO_PORT_SIZE - 1;

	for (i = 0; i < 3; i++) {
		mpc->irq[i] = platform_get_irq(pdev, i);
		if (mpc->irq[i] < 0) {
			dev_notice(&pdev->dev,
				"PCIe%d IRQ resource not found\n", i);
			return -ENXIO;
		}

		snprintf(name, sizeof(name), "pcie%d", i);
		mpc->rstctrl[i] = devm_reset_control_get(&pdev->dev, name);
		if (IS_ERR(mpc->rstctrl[i])) {
			dev_notice(&pdev->dev,
				"PCIe%d reset control not found\n", i);
			return PTR_ERR(mpc->rstctrl[i]);
		}
		if (!mpc->rstctrl[i]) {
			dev_notice(&pdev->dev,
				"PCIe%d reset control is not valid\n", i);
			return -EINVAL;
		}

		mpc->reset_gpio[i] = of_get_named_gpio(pdev->dev.of_node,
					"reset-gpios", i);

		if (gpio_is_valid(mpc->reset_gpio[i])) {
			snprintf(name, sizeof(name), "pcie%d-reset", i);
			ret = devm_gpio_request_one(&pdev->dev,
				mpc->reset_gpio[i], GPIOF_OUT_INIT_HIGH, name);
			if (ret) {
				dev_notice(&pdev->dev,
					"Unable to request GPIO%d for %s\n",
					mpc->reset_gpio[i], name);
				return ret;
			}
		} else {
			dev_notice(&pdev->dev,
				"Failed to get gpio for PCIe%d\n", i);
		}
	}

	mt7621_pcie_init(mpc);

	for (i = 0; i < 3; i++) {
		if ((mpc->link_status & BIT(i)) == 0)
			if (gpio_is_valid(mpc->reset_gpio[i]))
				devm_gpio_free(&pdev->dev, mpc->reset_gpio[i]);
	}

	if (mpc->link_status) {
		setup_cm_memory_region(mpc->pci_controller.mem_resource);
		register_pci_controller(&mpc->pci_controller);
	}

	return 0;
}

int pcibios_plat_dev_init(struct pci_dev *dev)
{
	return 0;
}

static const struct of_device_id mt7621_pci_ids[] = {
	{ .compatible = "mediatek,mt7621-pci" },
	{},
};
MODULE_DEVICE_TABLE(of, mt7621_pci_ids);

static struct platform_driver mt7621_pci_driver = {
	.probe = mt7621_pci_probe,
	.driver = {
		.name = "mt7621-pci",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(mt7621_pci_ids),
	},
};

static int __init mt7621_pci_init(void)
{
	return platform_driver_register(&mt7621_pci_driver);
}

arch_initcall(mt7621_pci_init);
