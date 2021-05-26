/*
 *  Mediatek MT7623 SoC PCIE support
 *
 *  Copyright (C) 2015 Mediatek
 *  Copyright (C) 2015 John Crispin <blogic@openwrt.org>
 *  Copyright (C) 2015 Ziv Huang <ziv.huang@mediatek.com>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version 2 as published
 *  by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <asm/irq.h>
#include <asm/mach/pci.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_pci.h>
#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/reset.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/pm_runtime.h>
#include <linux/clk.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>

#define MEMORY_BASE	0x80000000

#define SYSCFG1			0x14
#define RSTCTL			0x34
#define PCICFG			0x00
#define PCIINT			0x08
#define PCIENA			0x0c
#define CFGADDR			0x20
#define CFGDATA			0x24
#define MEMBASE			0x28
#define IOBASE			0x2c

#define BAR0SETUP		0x10
#define IMBASEBAR0		0x18
#define PCIE_CLASS		0x34
#define PCIE_SISTAT		0x50

#define MTK_PCIE_HIGH_PERF	BIT(14)
#define PCIEP0_BASE		0x2000
#define PCIEP1_BASE		0x3000
#define PCIEP2_BASE		0x4000

#define PHY_P0_CTL		0x9000
#define PHY_P1_CTL		0xa000
#define PHY_P2_CTL		0x4000

#define RSTCTL_PCIE0_RST	BIT(24)
#define RSTCTL_PCIE1_RST	BIT(25)
#define RSTCTL_PCIE2_RST	BIT(26)
#define MAX_PORT_NUM		3

static struct mtk_pcie_port {
	int id;
	int enable;
	u32 base;
	u32 phy_base;
	u32 perst_n;
	u32 reset;
	u32 interrupt_en;
	int irq;
	u32 link;
} mtk_pcie_port[] = {
	{ 0, 0, PCIEP0_BASE, PHY_P0_CTL, BIT(1), RSTCTL_PCIE0_RST, BIT(20) },
	{ 1, 0, PCIEP1_BASE, PHY_P1_CTL, BIT(2), RSTCTL_PCIE1_RST, BIT(21) },
	{ 2, 0, PCIEP2_BASE, PHY_P2_CTL, BIT(3), RSTCTL_PCIE2_RST, BIT(22) },
};

struct mtk_pcie {
	struct device *dev;
	void __iomem *sys_base;		/* HIF SYSCTL registers */
	void __iomem *pcie_base;	/* PCIE registers */
	void __iomem *usb_base;		/* USB registers */

	struct resource io;
	struct resource pio;
	struct resource mem;
	struct resource prefetch;
	struct resource busn;

	u32 io_bus_addr;
	u32 mem_bus_addr;

	struct clk *clk;
	int pcie_card_link;
};

struct mtk_pcie *pcie_dbg;

#define mtk_foreach_port(p)		\
		for ((p) = mtk_pcie_port;	\
		(p) != &mtk_pcie_port[ARRAY_SIZE(mtk_pcie_port)]; (p)++)

static const struct mtk_phy_init {
	uint32_t reg;
	uint32_t mask;
	uint32_t val;
} mtk_phy_init[] = {
	{ 0xc00, 0x33000, 0x22000 },
	{ 0xb04, 0xe0000000, 0x40000000 },
	{ 0xb00, 0xe, 0x4 },
	{ 0xc3C, 0xffff0000, 0x3c0000 },
	{ 0xc48, 0xffff, 0x36 },
	{ 0xc0c, 0x30000000, 0x10000000 },
	{ 0xc08, 0x3800c0, 0xc0 },
	{ 0xc10, 0xf0000, 0x20000 },
	{ 0xc0c, 0xf000, 0x1000 },
	{ 0xc14, 0xf0000, 0xa0000 },
	{ 0xa28, 0x3fe00, 0x2000 },
	{ 0xa2c, 0x1ff, 0x10 },
};

static struct mtk_pcie *sys_to_pcie(struct pci_sys_data *sys)
{
	return sys->private_data;
}

static void sys_w32(struct mtk_pcie *pcie, u32 val, unsigned reg)
{
	iowrite32(val, pcie->sys_base + reg);
}

static u32 sys_r32(struct mtk_pcie *pcie, unsigned reg)
{
	return ioread32(pcie->sys_base + reg);
}

static void pcie_w32(struct mtk_pcie *pcie, u32 val, unsigned reg)
{
	iowrite32(val, pcie->pcie_base + reg);
}

static void pcie_w16(struct mtk_pcie *pcie, u16 val, unsigned reg)
{
	iowrite16(val, pcie->pcie_base + reg);
}

static void pcie_w8(struct mtk_pcie *pcie, u8 val, unsigned reg)
{
	iowrite8(val, pcie->pcie_base + reg);
}

static u32 pcie_r32(struct mtk_pcie *pcie, unsigned reg)
{
	return ioread32(pcie->pcie_base + reg);
}

static u32 pcie_r16(struct mtk_pcie *pcie, unsigned reg)
{
	return ioread16(pcie->pcie_base + reg);
}

static u32 pcie_r8(struct mtk_pcie *pcie, unsigned reg)
{
	return ioread8(pcie->pcie_base + reg);
}

static void pcie_m32(struct mtk_pcie *pcie, u32 mask, u32 val, unsigned reg)
{
	u32 v = pcie_r32(pcie, reg);

	v &= mask;
	v |= val;
	pcie_w32(pcie, v, reg);
}

static int pcie_config_read(struct pci_bus *bus, unsigned int devfn, int where,
			    int size, u32 *val)
{
	struct mtk_pcie *pcie = sys_to_pcie(bus->sysdata);
	unsigned int slot = PCI_SLOT(devfn);
	u8 func = PCI_FUNC(devfn);
	u32 address;
	u32 data;
	u32 num = 0;

	if (bus)
		num = bus->number;

	address = (((where & 0xf00) >> 8) << 24) |
		(num << 16) |
		(slot << 11) |
		(func << 8) |
		(where & 0xfc);

	pcie_m32(pcie, 0xf0000000, address, CFGADDR);
	data = pcie_r32(pcie, CFGDATA);

	if (size == 4)
		*val = pcie_r32(pcie, CFGDATA);
	else if (size == 2)
		*val = pcie_r16(pcie, CFGDATA + (where & 2));
	else if (size == 1)
		*val = pcie_r8(pcie, CFGDATA + (where & 3));
	else
		return PCIBIOS_BAD_REGISTER_NUMBER;
	return PCIBIOS_SUCCESSFUL;
}

static int pcie_config_write(struct pci_bus *bus, unsigned int devfn,
			     int where, int size, u32 val)
{
	struct mtk_pcie *pcie = sys_to_pcie(bus->sysdata);
	unsigned int slot = PCI_SLOT(devfn);
	u8 func = PCI_FUNC(devfn);
	u32 address;
	u32 num = 0;

	if (bus)
		num = bus->number;

	address = (((where & 0xf00) >> 8) << 24) |
		(num << 16) | (slot << 11) | (func << 8) | (where & 0xfc);
	pcie_m32(pcie, 0xf0000000, address, CFGADDR);

	if (size == 4)
		pcie_w32(pcie, val, CFGDATA);
	else if (size == 2)
		pcie_w16(pcie, val, CFGDATA + (where & 2));
	else if (size == 1)
		pcie_w8(pcie, val, CFGDATA + (where & 3));
	else
		return PCIBIOS_BAD_REGISTER_NUMBER;

	return PCIBIOS_SUCCESSFUL;
}

static struct pci_ops mtk_pcie_ops = {
	.read   = pcie_config_read,
	.write  = pcie_config_write,
};

static int __init mtk_pcie_setup(int nr, struct pci_sys_data *sys)
{
	struct mtk_pcie *pcie = sys_to_pcie(sys);

	if (!pcie->pcie_card_link)
		return -1;

	pci_add_resource_offset(&sys->resources, &pcie->mem, sys->mem_offset);
	pci_add_resource(&sys->resources, &pcie->busn);
	pci_ioremap_io(0, pcie->io.start);
	return 1;
}

static struct pci_bus * __init mtk_pcie_scan_bus(int nr,
						 struct pci_sys_data *sys)
{
	struct mtk_pcie *pcie = sys_to_pcie(sys);
	struct pci_bus *bus;

	if (!pcie->pcie_card_link)
		return NULL;

	bus = pci_create_root_bus(pcie->dev, sys->busnr, &mtk_pcie_ops, sys,
				  &sys->resources);
	if (!bus)
		return NULL;

	pci_scan_child_bus(bus);

	return bus;
}

static int __init mtk_pcie_map_irq(const struct pci_dev *dev, u8 slot, u8 pin)
{
	struct mtk_pcie *pcie = sys_to_pcie(dev->bus->sysdata);
	struct mtk_pcie_port *port;
	int irq = -1;

	if (!pcie->pcie_card_link)
		return irq;

	mtk_foreach_port(port)
		if (port->id == slot)
			irq = port->irq;

	dev_dbg(pcie->dev, "%s: bus=0x%x, slot = 0x%x, pin=0x%x\n",
			__func__, dev->bus->number, slot, pin);
	dev_dbg(pcie->dev, "dev->irq=0x%x, irq=0x%x\n",
			dev->irq, irq);
	return irq;
}

static void mtk_pcie_configure_phy(struct mtk_pcie_port *port)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(mtk_phy_init); i++) {
		void __iomem *phy_addr = (void __iomem *)port->phy_base +
						mtk_phy_init[i].reg;
		u32 val = ioread32(phy_addr);

		val &= ~mtk_phy_init[i].mask;
		val |= mtk_phy_init[i].val;
		iowrite32(val, phy_addr);
	}
	usleep_range(5000, 6000);
}

static void mtk_pcie_configure_rc(struct mtk_pcie *pcie,
				  struct mtk_pcie_port *port,
				  struct pci_bus *bus)
{
	u32 val = 0;

	pcie_config_write(bus, (port->id)<<3, PCI_BASE_ADDRESS_0,
				4, MEMORY_BASE);

	pcie_config_read(bus, (port->id)<<3, PCI_BASE_ADDRESS_0,
				4, &val);
	dev_info(pcie->dev, "BAR0 at bus %d, slot %d, val 0x%x\n",
			0, port->id, val);
	/* Configre RC Credit */
	val = 0;

	pcie_config_read(bus, (port->id)<<3, 0x73c, 4, &val);
	val &= ~(0x9fff)<<16;
	val |= 0x806c<<16;
	pcie_config_write(bus, (port->id)<<3, 0x73c, 4, val);
	/* Configre RC FTS number */
	pcie_config_read(bus, (port->id)<<3, 0x70c, 4, &val);
	val &= ~(0xff3) << 8;
	val |= 0x50 << 8;
	pcie_config_write(bus, (port->id)<<3, 0x70c, 4, val);
}

static void mtk_pcie_preinit(struct mtk_pcie *pcie)
{
	struct mtk_pcie_port *port;
	u32 val = 0;
	struct pci_bus bus;
	struct pci_sys_data sys;

	memset(&bus, 0, sizeof(bus));
	memset(&sys, 0, sizeof(sys));
	bus.sysdata = (void *)&sys;
	sys.private_data = (void *)pcie;

	pcibios_min_io = 0;
	pcibios_min_mem = 0;

	/* PCIe RC Reset */
	val = 0;
	mtk_foreach_port(port)
		if (port->enable)
			val |= port->reset;
	sys_w32(pcie, sys_r32(pcie, RSTCTL) | val, RSTCTL);
	usleep_range(1000, 2000);
	dev_dbg(pcie->dev, "%s: RSTCTL=0x%x\n", __func__,
			sys_r32(pcie, RSTCTL));
	sys_w32(pcie, sys_r32(pcie, RSTCTL) & ~val, RSTCTL);
	usleep_range(1000, 2000);
	dev_dbg(pcie->dev, "%s: RSTCTL=0x%x\n", __func__,
			sys_r32(pcie, RSTCTL));

	/* Configure PCIe PHY */

	mtk_foreach_port(port)
		if (port->enable)
			mtk_pcie_configure_phy(port);

	/* PCIe EP reset */
	val = 0;
	mtk_foreach_port(port)
		if (port->enable)
			val |= port->perst_n;
	pcie_w32(pcie, pcie_r32(pcie, PCICFG) | val, PCICFG);
	usleep_range(1000, 2000);
	dev_dbg(pcie->dev, "%s: PCICFG=0x%x\n", __func__,
			pcie_r32(pcie, PCICFG));
	pcie_w32(pcie, pcie_r32(pcie, PCICFG) & ~val, PCICFG);
	usleep_range(1000, 2000);
	dev_dbg(pcie->dev, "%s: PCICFG=0x%x\n", __func__,
			pcie_r32(pcie, PCICFG));
	msleep(100);

	/* check the link status */
	val = 0;
	mtk_foreach_port(port) {
		if (port->enable) {
			if ((pcie_r32(pcie, port->base + PCIE_SISTAT) & 0x1))
				port->link = 1;
			else
				val |= port->reset;
		}
	}
	sys_w32(pcie, sys_r32(pcie, RSTCTL) | val, RSTCTL);
	dev_dbg(pcie->dev, "%s: RSTCTL=0x%x\n", __func__,
			sys_r32(pcie, RSTCTL));

	mtk_foreach_port(port) {
		if (port->link) {
			pcie->pcie_card_link++;
			dev_info(pcie->dev, "%s: PCIE%d link up\n", __func__,
					port->id);
		}
	}
	dev_dbg(pcie->dev, "%s: PCIe Link count=%d\n", __func__,
		pcie->pcie_card_link);
	if (!pcie->pcie_card_link)
		return;

	pcie_w32(pcie, pcie->mem_bus_addr, MEMBASE);
	pcie_w32(pcie, pcie->io_bus_addr, IOBASE);

	mtk_foreach_port(port) {
		if (port->link) {
			pcie_m32(pcie, 0xffffffff, port->interrupt_en, PCIENA);
			pcie_w32(pcie, 0x7fff0001, port->base + BAR0SETUP);
			pcie_w32(pcie, MEMORY_BASE, port->base + IMBASEBAR0);
			pcie_w32(pcie, 0x06040001, port->base + PCIE_CLASS);
			dev_info(pcie->dev, "PCIE%d Setup OK\n", port->id);
		}
	}

	mtk_foreach_port(port)
		if (port->link)
			mtk_pcie_configure_rc(pcie, port, &bus);
}

static void mtk_pcie_set_usb_pcie_co_phy(struct mtk_pcie *pcie)
{
	if (mtk_pcie_port[2].enable == 1) {
		u32 val;
		/* USB-PCIe co-phy switch */
		val = sys_r32(pcie, SYSCFG1);

		dev_dbg(pcie->dev, "%s: PCIe/USB3 combo PHY mode SYSCFG1=%x\n",
			__func__, val);
		val &= ~(0x300000);
		sys_w32(pcie, val, SYSCFG1);
		dev_dbg(pcie->dev, "%s: PCIe/USB3 combo PHY mode SYSCFG1=%x\n",
			__func__, val);
	}
}

static void mtk_pcie_fill_port(struct mtk_pcie *pcie)
{
	int i;

	for (i = 0; i < 2; i++)
		mtk_pcie_port[i].phy_base += (u32)pcie->pcie_base;

	mtk_pcie_port[2].phy_base += (u32)pcie->usb_base;
}

static int mtk_pcie_enable(struct mtk_pcie *pcie)
{
	struct hw_pci hw;

	mtk_pcie_preinit(pcie);
	memset(&hw, 0, sizeof(hw));

	hw.nr_controllers = 1;
	hw.private_data = (void **)&pcie;
	hw.setup = mtk_pcie_setup;
	hw.map_irq = mtk_pcie_map_irq;
	hw.scan = mtk_pcie_scan_bus;
	pci_common_init_dev(pcie->dev, &hw);

	return 0;
}

static int mtk_pcie_parse_dt(struct mtk_pcie *pcie)
{
	struct device_node *np = pcie->dev->of_node, *port;
	struct of_pci_range_parser parser;
	struct of_pci_range range;
	struct resource res;
	int err;

	if (of_pci_range_parser_init(&parser, np)) {
		dev_err(pcie->dev, "missing \"ranges\" property\n");
		return -EINVAL;
	}

	for_each_of_pci_range(&parser, &range) {
		err = of_pci_range_to_resource(&range, np, &res);
		if (err < 0)
			return err;

		switch (res.flags & IORESOURCE_TYPE_BITS) {
		case IORESOURCE_IO:
			memcpy(&pcie->pio, &res, sizeof(res));
			pcie->pio.start = (resource_size_t)range.pci_addr;
			pcie->pio.end = (resource_size_t)
				(range.pci_addr + range.size - 1);
			pcie->io_bus_addr = (resource_size_t)range.cpu_addr;

			pcie->io.start = range.cpu_addr;
			pcie->io.end = range.cpu_addr + range.size - 1;
			pcie->io.flags = IORESOURCE_MEM;
			pcie->io.name = "I/O";
			break;

		case IORESOURCE_MEM:
			if (res.flags & IORESOURCE_PREFETCH) {
				memcpy(&pcie->prefetch, &res, sizeof(res));
				pcie->prefetch.name = "prefetchable";
				pcie->prefetch.start =
					(resource_size_t)range.pci_addr;
				pcie->prefetch.end = (resource_size_t)
					(range.pci_addr + range.size - 1);
			} else {
				memcpy(&pcie->mem, &res, sizeof(res));
				pcie->mem.name = "non-prefetchable";
				pcie->mem.start = (resource_size_t)
					range.pci_addr;
				pcie->prefetch.end = (resource_size_t)
					(range.pci_addr + range.size - 1);
				pcie->mem_bus_addr = (resource_size_t)
					range.cpu_addr;
			}
			break;
		}
	}

	err = of_pci_parse_bus_range(np, &pcie->busn);
	if (err < 0) {
		dev_err(pcie->dev, "failed to parse ranges property: %d\n",
			err);
		pcie->busn.name = np->name;
		pcie->busn.start = 0;
		pcie->busn.end = 0xff;
		pcie->busn.flags = IORESOURCE_BUS;
	}

	/* parse root ports */
	for_each_child_of_node(np, port) {
		unsigned int index;

		err = of_pci_get_devfn(port);
		if (err < 0) {
			dev_err(pcie->dev, "failed to parse address: %d\n",
				err);
			return err;
		}

		index = PCI_SLOT(err);

		if (index > MAX_PORT_NUM) {
			dev_err(pcie->dev, "invalid port number: %d\n", index);
			return -EINVAL;
		}

		index--;
		if (!of_device_is_available(port))
			continue;

		mtk_pcie_port[index].enable = 1;
	}
	return 0;
}

static int mtk_pcie_get_resources(struct mtk_pcie *pcie)
{
	struct platform_device *pdev = to_platform_device(pcie->dev);
	struct mtk_pcie_port *port;
	struct resource *sys_res;
	struct resource *pcie_res;
	struct resource *usb_res;
	int ret;

	pcie->clk = devm_clk_get(&pdev->dev, "pcie");
	if (IS_ERR(pcie->clk)) {
		dev_err(&pdev->dev, "Failed to get pcie clk\n");
		return PTR_ERR(pcie->clk);
	}

	ret = clk_prepare_enable(pcie->clk);
	if (ret)
		return ret;

	sys_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	pcie->sys_base = devm_ioremap_resource(&pdev->dev, sys_res);
	if (IS_ERR(pcie->sys_base)) {
		ret = PTR_ERR(pcie->sys_base);
		goto fail_clk_pcie;
	}

	pcie_res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	pcie->pcie_base = devm_ioremap_resource(&pdev->dev, pcie_res);
	if (IS_ERR(pcie->pcie_base)) {
		ret = PTR_ERR(pcie->pcie_base);
		goto fail_clk_pcie;
	}

	if (mtk_pcie_port[2].enable) {
		usb_res = platform_get_resource(pdev, IORESOURCE_MEM, 2);
		pcie->usb_base = devm_ioremap_resource(&pdev->dev, usb_res);
		if (IS_ERR(pcie->usb_base)) {
			ret = PTR_ERR(pcie->usb_base);
			goto fail_clk_pcie;
		}
	}
	mtk_pcie_set_usb_pcie_co_phy(pcie);
	mtk_pcie_fill_port(pcie);
	mtk_foreach_port(port)
		port->irq = platform_get_irq(pdev, port->id);

	return 0;

fail_clk_pcie:
	clk_disable_unprepare(pcie->clk);
	pm_runtime_put_sync(&pdev->dev);
	pm_runtime_disable(&pdev->dev);
	return ret;
}

static int mtk_pcie_probe(struct platform_device *pdev)
{
	struct mtk_pcie *pcie;
	int ret;

	pcie = devm_kzalloc(&pdev->dev, sizeof(*pcie), GFP_KERNEL);
	if (!pcie)
		return -ENOMEM;

	pcie->dev = &pdev->dev;
	ret = mtk_pcie_parse_dt(pcie);
	if (ret < 0)
		return ret;

	pm_runtime_enable(&pdev->dev);
	pm_runtime_get_sync(&pdev->dev);

	ret = mtk_pcie_get_resources(pcie);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to request resources: %d\n", ret);
		return ret;
	}

	ret = mtk_pcie_enable(pcie);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to enable PCIe ports: %d\n", ret);
		goto release_resource;
	}

	platform_set_drvdata(pdev, pcie);
	pcie_dbg = pcie;
	dev_info(&pdev->dev, "==>mtk_pcie_probe done\n");
	return 0;

release_resource:
	clk_disable_unprepare(pcie->clk);
	pm_runtime_put_sync(&pdev->dev);
	pm_runtime_disable(&pdev->dev);
	return ret;
}

static const struct of_device_id mtk_pcie_ids[] = {
	{ .compatible = "mediatek,mt2701-pcie" },
	{ .compatible = "mediatek,mt7623-pcie" },
	{},
};
MODULE_DEVICE_TABLE(of, mtk_pcie_ids);

static struct platform_driver mtk_pcie_driver = {
	.probe = mtk_pcie_probe,
	.driver = {
		.name = "mt2701-pcie",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(mtk_pcie_ids),
	},
};

static ssize_t pcie_debug_proc_write(struct file *file, const char *buf,
		size_t count, loff_t *data)
{
	unsigned char cmd_buf[32];
	unsigned int cmd, addr;
	u32 value;
	int i, ret;

	if (count == 0)
		return -1;
	if (count > 31)
		count = 31;
	memset(cmd_buf, 0, 32);
	ret = copy_from_user(cmd_buf, buf, count);
	if (ret < 0)
		return -1;
	ret = sscanf(cmd_buf, "%d %x %x", &cmd, &addr, &value);

	switch (cmd) {
	case 1:
		if (addr>>16 == 0x1a00)
			value = sys_r32(pcie_dbg, addr & 0xffff);
		else if ((addr>>16 == 0x1a14) || (addr>>16 == 0x1a24))
			value = pcie_r32(pcie_dbg, addr & 0xffff);
		else
			return -1;
		pr_err("[Read]0x%X: 0x%x\n", addr, value);
		break;
	case 2:
		if (addr>>16 == 0x1a00) {
			sys_w32(pcie_dbg, value, addr & 0xffff);
			value = sys_r32(pcie_dbg, addr & 0xffff);
		} else if ((addr>>16 == 0x1a14) || (addr>>16 == 0x1a24)) {
			pcie_w32(pcie_dbg, value, addr & 0xffff);
			value = pcie_r32(pcie_dbg, addr & 0xffff);
		} else {
			return -1;
		}
		pr_err("[Write]0x%X: 0x%x\n", addr, value);
		break;
	case 3:
		for (i = 0; i < 0x100; ) {
			unsigned int dst = addr + i;

			if ((dst>>16) == 0x1a00) {
				pr_err("0x%08X: %08x %08x %08x %08x\n",
					dst,
					sys_r32(pcie_dbg, (dst) & 0xffff),
					sys_r32(pcie_dbg, (dst+0x4)&0xffff),
					sys_r32(pcie_dbg, (dst+0x8)&0xffff),
					sys_r32(pcie_dbg, (dst+0xC)&0xffff));
			} else if ((dst>>16) == 0x1a14 ||
				   (dst>>16) == 0x1a24) {
				pr_err("0x%08X: %08x %08x %08x %08x\n",
					dst,
					pcie_r32(pcie_dbg, (dst)&0xffff),
					pcie_r32(pcie_dbg, (dst+0x4)&0xffff),
					pcie_r32(pcie_dbg, (dst+0x8)&0xffff),
					pcie_r32(pcie_dbg, (dst+0xC)&0xffff));
			}
			i += 0x10;
		}
		break;
	default:
		pr_err("!!! unknown command !!!\n");
		return -1;
	}

	return count;
}

static int pcie_debug_proc_show(struct seq_file *m, void *v)
{
	seq_puts(m, "=== PCIe Host register access ===\n");
	seq_puts(m, "read: echo 1 [address] >/proc/pcie_host_debug\n");
	seq_puts(m, "\tex. echo 1 0x1a14000 >/proc/pcie_host_debug\n");
	seq_puts(m, "write: echo 2 [address] [value] >/proc/pcie_host_debug\n");
	seq_puts(m, "\tex. echo 2 0x1a14000 0x12345678 >/proc/pcie_host_debug\n");
	seq_puts(m, "dump: echo 3 [address] >/proc/pcie_host_debug\n");
	seq_puts(m, "\tex. echo 3 0x1a14000 >/proc/pcie_host_debug\n");
	return 0;
}

static int pcie_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, pcie_debug_proc_show, inode->i_private);
}

static const struct file_operations pcie_proc_fops = {
	.open = pcie_proc_open,
	.write = pcie_debug_proc_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int __init mtk_pcie_init(void)
{
	struct proc_dir_entry *prEntry;

	prEntry = proc_create("pcie_host_debug", 0660, NULL, &pcie_proc_fops);
	return platform_driver_register(&mtk_pcie_driver);
}

module_init(mtk_pcie_init);
