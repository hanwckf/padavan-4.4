/*
 * Copyright (c) 2015 MediaTek Inc.
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

#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/phy/phy.h>

static const struct of_device_id ext_mipi_phy_match[] = {
	{ .compatible = "mediatek,ext-mipi-phy" },
	{},
};

struct ext_mipi_phy {
	struct device *dev;
	void __iomem *regs;
	u32 data_rate;

	struct clk_hw pll_hw;
	struct clk *pll;
};

static inline struct ext_mipi_phy *ext_mipi_phy_from_clk_hw(struct clk_hw *hw)
{
	return container_of(hw, struct ext_mipi_phy, pll_hw);
}

static void ext_mipi_phy_write60384(struct ext_mipi_phy *ext_phy, u8 dev_addr,
				    u8 write_addr, u8 write_data)
{
		dev_dbg(ext_phy->dev, "MIPITX_Write60384:0x%x,0x%x,0x%x\n",
			dev_addr, write_addr, write_data);
		writel(0x2, ext_phy->regs + 0x14);
		writel(0x1, ext_phy->regs + 0x18);
		writel(dev_addr << 0x1, ext_phy->regs + 0x04);
		writel(write_addr, ext_phy->regs + 0x0);
		writel(write_data, ext_phy->regs + 0x0);
		writel(0x1, ext_phy->regs + 0x24);
		while ((readl(ext_phy->regs + 0xC) & 0x1) != 0x1)
			;
		writel(0xFF, ext_phy->regs + 0xC);

		writel(0x1, ext_phy->regs + 0x14);
		writel(0x1, ext_phy->regs + 0x18);
		writel(dev_addr << 0x1, ext_phy->regs + 0x04);
		writel(write_addr, ext_phy->regs + 0x0);
		writel(0x1, ext_phy->regs + 0x24);
		while ((readl(ext_phy->regs + 0xC) & 0x1) != 0x1)
			;
		writel(0xFF, ext_phy->regs + 0xC);

		writel(0x1, ext_phy->regs + 0x14);
		writel(0x1, ext_phy->regs + 0x18);
		writel((dev_addr << 0x1) + 1, ext_phy->regs + 0x04);
		writel(0x1, ext_phy->regs + 0x24);
		while ((readl(ext_phy->regs + 0xC) & 0x1) != 0x1)
			;
		writel(0xFF, ext_phy->regs + 0xC);

		dev_dbg(ext_phy->dev, "phy wr  data = 0x%x, rd data = 0x%x\n",
			write_data, readl(ext_phy->regs));
		if (readl(ext_phy->regs) == write_data)
			dev_dbg(ext_phy->dev, "MIPI write success\n");
		else
			dev_dbg(ext_phy->dev, "MIPI write fail\n");
}

static int ext_mipi_phy_pll_prepare(struct clk_hw *hw)
{
	struct ext_mipi_phy *ext_phy = ext_mipi_phy_from_clk_hw(hw);
	u8 txdiv, txdiv0, txdiv1;
	u64 pcw;

	dev_dbg(ext_phy->dev, "prepare: %u Hz\n", ext_phy->data_rate);

	if (ext_phy->data_rate > 1250000000) {
		return -EINVAL;
	} else if (ext_phy->data_rate >= 500000000) {
		txdiv = 1;
		txdiv0 = 0;
		txdiv1 = 0;
	} else if (ext_phy->data_rate >= 250000000) {
		txdiv = 2;
		txdiv0 = 1;
		txdiv1 = 0;
	} else if (ext_phy->data_rate >= 125000000) {
		txdiv = 4;
		txdiv0 = 2;
		txdiv1 = 0;
	} else if (ext_phy->data_rate > 62000000) {
		txdiv = 8;
		txdiv0 = 2;
		txdiv1 = 1;
	} else if (ext_phy->data_rate >= 50000000) {
		txdiv = 16;
		txdiv0 = 2;
		txdiv1 = 2;
	} else {
		return -EINVAL;
	}

	ext_mipi_phy_write60384(ext_phy, 0x18, 0x00, 0x10);
	ext_mipi_phy_write60384(ext_phy, 0x20, 0x43, 0x01);
	ext_mipi_phy_write60384(ext_phy, 0x20, 0x05, 0x01);
	ext_mipi_phy_write60384(ext_phy, 0x20, 0x22, 0x01);

	ext_mipi_phy_write60384(ext_phy, 0x30, 0x40, 0x82);

	ext_mipi_phy_write60384(ext_phy, 0x30, 0x44, 0x83);

	usleep_range(30, 100);

	ext_mipi_phy_write60384(ext_phy, 0x30, 0x00, 0x03);

	ext_mipi_phy_write60384(ext_phy, 0x30, 0x68, 0x03);
	ext_mipi_phy_write60384(ext_phy, 0x30, 0x68, 0x01);

	ext_mipi_phy_write60384(ext_phy, 0x30, 0x50, txdiv0 << 3 | txdiv1 << 5);

	ext_mipi_phy_write60384(ext_phy, 0x30, 0x58, pcw & 0xFF);
	ext_mipi_phy_write60384(ext_phy, 0x30, 0x59, pcw >> 8 & 0xFF);
	ext_mipi_phy_write60384(ext_phy, 0x30, 0x5a, pcw >> 16 & 0xFF);
	ext_mipi_phy_write60384(ext_phy, 0x30, 0x5b, pcw >> 24 & 0xFF);

	ext_mipi_phy_write60384(ext_phy, 0x30, 0x54, 0x01);

	ext_mipi_phy_write60384(ext_phy, 0x30, 0x50, txdiv0 << 3 | txdiv1 << 5 |
				1);

	usleep_range(20, 100);

	ext_mipi_phy_write60384(ext_phy, 0x30, 0x28, 0x00);

	ext_mipi_phy_write60384(ext_phy, 0x30, 0x64, 0x20);

	return 0;
}

static void ext_mipi_phy_pll_unprepare(struct clk_hw *hw)
{
}

static long ext_mipi_phy_pll_round_rate(struct clk_hw *hw, unsigned long rate,
					unsigned long *prate)
{
	return clamp_val(rate, 50000000, 1250000000);
}

static int ext_mipi_phy_pll_set_rate(struct clk_hw *hw, unsigned long rate,
				     unsigned long parent_rate)
{
	struct ext_mipi_phy *ext_phy = ext_mipi_phy_from_clk_hw(hw);

	dev_dbg(ext_phy->dev, "set rate: %lu Hz\n", rate);

	ext_phy->data_rate = rate;

	return 0;
}

static unsigned long ext_mipi_phy_pll_recalc_rate(struct clk_hw *hw,
						  unsigned long parent_rate)
{
	struct ext_mipi_phy *ext_phy = ext_mipi_phy_from_clk_hw(hw);

	return ext_phy->data_rate;
}

static const struct clk_ops ext_mipi_phy_pll_ops = {
	.prepare = ext_mipi_phy_pll_prepare,
	.unprepare = ext_mipi_phy_pll_unprepare,
	.round_rate = ext_mipi_phy_pll_round_rate,
	.set_rate = ext_mipi_phy_pll_set_rate,
	.recalc_rate = ext_mipi_phy_pll_recalc_rate,
};

static int ext_mipi_phy_power_on_signal(struct phy *phy)
{
	struct ext_mipi_phy *ext_phy = phy_get_drvdata(phy);

	ext_mipi_phy_write60384(ext_phy, 0x30, 0x04, 0x11);
	ext_mipi_phy_write60384(ext_phy, 0x30, 0x08, 0x01);
	ext_mipi_phy_write60384(ext_phy, 0x30, 0x0C, 0x01);
	ext_mipi_phy_write60384(ext_phy, 0x30, 0x10, 0x01);
	ext_mipi_phy_write60384(ext_phy, 0x30, 0x14, 0x01);

	ext_mipi_phy_write60384(ext_phy, 0x30, 0x41, 0x08);

	return 0;
}

static int ext_mipi_phy_power_on(struct phy *phy)
{
	struct ext_mipi_phy *ext_phy = phy_get_drvdata(phy);
	int ret;

	/* Power up core and enable PLL */
	ret = clk_prepare_enable(ext_phy->pll);
	if (ret < 0)
		return ret;

	/* Enable DSI Lane LDO outputs, disable pad tie low */
	ext_mipi_phy_power_on_signal(phy);

	return 0;
}

static void ext_mipi_phy_power_off_signal(struct phy *phy)
{
}

static int ext_mipi_phy_power_off(struct phy *phy)
{
	struct ext_mipi_phy *ext_phy = phy_get_drvdata(phy);

	/* Enable pad tie low, disable DSI Lane LDO outputs */
	ext_mipi_phy_power_off_signal(phy);

	/* Disable PLL and power down core */
	clk_disable_unprepare(ext_phy->pll);

	return 0;
}

static const struct phy_ops ext_mipi_phy_ops = {
	.power_on = ext_mipi_phy_power_on,
	.power_off = ext_mipi_phy_power_off,
	.owner = THIS_MODULE,
};

static int ext_mipi_phy_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct ext_mipi_phy *ext_phy;
	struct resource *mem;
	struct clk *ref_clk;
	const char *ref_clk_name;
	struct clk_init_data clk_init = {
		.ops = &ext_mipi_phy_pll_ops,
		.num_parents = 1,
		.parent_names = (const char * const *)&ref_clk_name,
		.flags = CLK_SET_RATE_GATE,
	};
	struct phy *phy;
	struct phy_provider *phy_provider;
	int ret;

	ext_phy = devm_kzalloc(dev, sizeof(*ext_phy), GFP_KERNEL);
	if (!ext_phy)
		return -ENOMEM;

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	ext_phy->regs = devm_ioremap_resource(dev, mem);
	if (IS_ERR(ext_phy->regs)) {
		ret = PTR_ERR(ext_phy->regs);
		dev_err(dev, "Failed to get memory resource: %d\n", ret);
		return ret;
	}

	ref_clk = devm_clk_get(dev, NULL);
	if (IS_ERR(ref_clk)) {
		ret = PTR_ERR(ref_clk);
		dev_err(dev, "Failed to get reference clock: %d\n", ret);
		return ret;
	}
	ref_clk_name = __clk_get_name(ref_clk);

	ret = of_property_read_string(dev->of_node, "clock-output-names",
				      &clk_init.name);
	if (ret < 0) {
		dev_err(dev, "Failed to read clock-output-names: %d\n", ret);
		return ret;
	}

	ext_phy->pll_hw.init = &clk_init;
	ext_phy->pll = devm_clk_register(dev, &ext_phy->pll_hw);
	if (IS_ERR(ext_phy->pll)) {
		ret = PTR_ERR(ext_phy->pll);
		dev_err(dev, "Failed to register PLL: %d\n", ret);
		return ret;
	}

	phy = devm_phy_create(dev, NULL, &ext_mipi_phy_ops);
	if (IS_ERR(phy)) {
		ret = PTR_ERR(phy);
		dev_err(dev, "Failed to create MIPI D-PHY: %d\n", ret);
		return ret;
	}
	phy_set_drvdata(phy, ext_phy);

	phy_provider = devm_of_phy_provider_register(dev, of_phy_simple_xlate);
	if (IS_ERR(phy)) {
		ret = PTR_ERR(phy_provider);
		return ret;
	}

	ext_phy->dev = dev;

	return of_clk_add_provider(dev->of_node, of_clk_src_simple_get,
				   ext_phy->pll);
}

static int ext_mipi_phy_remove(struct platform_device *pdev)
{
	of_clk_del_provider(pdev->dev.of_node);
	return 0;
}

struct platform_driver ext_mipi_phy_driver = {
	.probe = ext_mipi_phy_probe,
	.remove = ext_mipi_phy_remove,
	.driver = {
		.name = "mediatek-mipi-tx",
		.of_match_table = ext_mipi_phy_match,
	},
};
