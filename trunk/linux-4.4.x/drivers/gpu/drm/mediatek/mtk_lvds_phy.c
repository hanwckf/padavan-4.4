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

#define LVDSTX_CTL1	0x00
#define LVDSTX_CTL2	0x04
#define RG_LVDSTX_TVO	(0xf << 0)
#define RG_LVDSTX_TVCM	(0xf << 4)
#define RG_LVDSTX_TSTCLK_SEL	(0x3 << 8)
#define RG_LVDSTX_TSTCLKDIV_EN	BIT(10)
#define RG_LVDSTX_TSTCLK_EN	BIT(11)
#define RG_LVDSTX_TSTCLKDIV_SEL	(3 << 12)
#define RG_LVDSTX_MPX_SEL	(3 << 14)
#define RG_LVDSTX_BIAS_SEL	(3 << 16)
#define RG_LVDSTX_R_TERM	(3 << 18)
#define RG_LVDSTX_SEL_CKTST	BIT(20)
#define RG_LVDSTX_SEL_MERGE	BIT(21)
#define RG_LVDSTX_LDO_EN	BIT(22)
#define RG_LVDSTX_BIAS_EN	BIT(23)
#define RG_LVDSTX_SER_ABIST_EN	BIT(24)
#define RG_LVDSTX_SER_ABEDG_EN	BIT(25)
#define RG_LVDSTX_SER_BIST_TOG	BIT(26)

#define LVDSTX_CTL3	0x08
#define RG_LVDSTX_VOUTABIST_EN	(0x1f << 0)
#define RG_LVDSTX_EXT_EN	(0x1f << 5)
#define RG_LVDSTX_DRV_EN	(0x1f << 10)
#define RG_LVDSTX_SER_DIN_SEL	BIT(16)
#define RG_LVDSTX_SER_CLKDIG_INV	BIT(17)

#define LVDSTX_CTL4	0x0c
#define RG_LVDSTX_TSTPAD_EN	BIT(20)
#define RG_LVDSTX_ABIST_EN	BIT(21)
#define RG_LVDSTX_MPX_EN	BIT(22)
#define RG_LVDSTX_LDOLPF_EN	BIT(23)
#define RG_LVDSTX_TEST_BYPASSBUF	BIT(24)
#define RG_LVDSTX_BIASLPF_EN	BIT(25)
#define RG_LVDSTX_SER_ABMUX_SEL	(7 << 26)
#define RG_LVDSTX_SER_PEM_EN	BIT(29)
#define RG_LVDSTX_LVROD	(3 << 30)

#define LVDSTX_CTL5	0x10
#define RG_LVDSTX_MIPICK_SEL	BIT(4)
#define RG_LVDSTX_INCK_SEL	BIT(5)
#define RG_LVDSTX_SWITCH_EN	BIT(6)

#define VOPLL_CTL1		0x14
#define RG_VPLL_TXMUXDIV2_EN	BIT(0)
#define RG_VPLL_FBKSEL	(3 << 6)
#define RG_VPLL_FBKDIV	(0x7f << 12)

#define VOPLL_CTL2		0x18
#define RG_VPLL_EN	BIT(7)
#define RG_VPLL_TXDIV1	(3 << 8)
#define RG_VPLL_TXDIV2	(3 << 10)
#define RG_VPLL_LVDS_EN	BIT(12)
#define RG_VPLL_LVDS_DPIX_DIV2	BIT(13)
#define RG_VPLL_TTLDIV	(3 << 16)
#define RG_VPLL_TXDIV5_EN	BIT(21)
#define RG_VPLL_BIAS_EN	BIT(24)
#define RG_VPLL_BIASLPF_EN	BIT(25)

#define VOPLL_CTL3		0x1c
#define LVDS_ISO_EN		BIT(8)
#define DA_LVDSTX_PWR_ON	BIT(9)

struct mtk_lvds_tx {
	struct device *dev;
	void __iomem *regs;
	void __iomem *tx1_regs;
	void __iomem *tx2_regs;
	u32 data_rate;
	bool dual_lvds;

	struct clk_hw pll_hw;
	struct clk *pll;
};

static inline struct mtk_lvds_tx *mtk_lvds_tx_from_clk_hw(struct clk_hw *hw)
{
	return container_of(hw, struct mtk_lvds_tx, pll_hw);
}

static long mtk_lvds_tx_pll_round_rate(struct clk_hw *hw, unsigned long rate,
				       unsigned long *prate)
{
	return clamp_val(rate, 50000000, 1250000000);
}

static int mtk_lvds_tx_pll_set_rate(struct clk_hw *hw, unsigned long rate,
				    unsigned long parent_rate)
{
	struct mtk_lvds_tx *lvds_tx = mtk_lvds_tx_from_clk_hw(hw);
	u32 reg;

	if (rate > 75000000)
		lvds_tx->dual_lvds = true;
	else
		lvds_tx->dual_lvds = false;

	writel(DA_LVDSTX_PWR_ON, lvds_tx->tx1_regs + VOPLL_CTL3);
	writel(DA_LVDSTX_PWR_ON, lvds_tx->tx2_regs + VOPLL_CTL3);
	reg = RG_VPLL_TXMUXDIV2_EN | 1 << 6 | 0x1c << 12 | 1 << 20;
	writel(reg, lvds_tx->tx1_regs + VOPLL_CTL1);
	writel(reg, lvds_tx->tx2_regs + VOPLL_CTL1);

	reg = RG_VPLL_EN | 1 << 8 | (lvds_tx->dual_lvds ? 0 : 1) << 10 |
	      RG_VPLL_LVDS_EN | RG_VPLL_LVDS_DPIX_DIV2 |
	      (lvds_tx->dual_lvds ? 1 : 0) << 16 | RG_VPLL_TXDIV5_EN |
	      RG_VPLL_BIAS_EN | RG_VPLL_BIASLPF_EN;
	writel(reg, lvds_tx->tx1_regs + VOPLL_CTL2);
	writel(reg, lvds_tx->tx2_regs + VOPLL_CTL2);

	return 0;
}

static unsigned long mtk_lvds_tx_pll_recalc_rate(struct clk_hw *hw,
						 unsigned long parent_rate)
{
	struct mtk_lvds_tx *lvds_tx = mtk_lvds_tx_from_clk_hw(hw);

	return lvds_tx->data_rate;
}

static const struct clk_ops mtk_lvds_tx_pll_ops = {
	.round_rate = mtk_lvds_tx_pll_round_rate,
	.set_rate = mtk_lvds_tx_pll_set_rate,
	.recalc_rate = mtk_lvds_tx_pll_recalc_rate,
};

static int mtk_lvds_tx_power_on_signal(struct phy *phy)
{
	struct mtk_lvds_tx *lvds_tx = phy_get_drvdata(phy);
	u32 reg;

	reg = 7 | 0xb << 4 | 3 << 8 | RG_LVDSTX_TSTCLKDIV_EN |
	      RG_LVDSTX_TSTCLK_EN | 1 << 16 | RG_LVDSTX_LDO_EN |
	      RG_LVDSTX_BIAS_EN;
	writel(reg, lvds_tx->tx1_regs + LVDSTX_CTL2);

	if (lvds_tx->dual_lvds)
		writel(reg, lvds_tx->tx2_regs + LVDSTX_CTL2);

	reg = 0x1f << 5 | 0x1f << 10;
	writel(reg, lvds_tx->tx1_regs + LVDSTX_CTL3);

	if (lvds_tx->dual_lvds)
		writel(reg, lvds_tx->tx2_regs + LVDSTX_CTL3);

	reg = RG_LVDSTX_LDOLPF_EN | RG_LVDSTX_BIASLPF_EN;
	writel(reg, lvds_tx->tx1_regs + LVDSTX_CTL4);

	if (lvds_tx->dual_lvds)
		writel(reg, lvds_tx->tx2_regs + LVDSTX_CTL4);

	return 0;
}

static int mtk_lvds_tx_power_on(struct phy *phy)
{
	struct mtk_lvds_tx *lvds_tx = phy_get_drvdata(phy);
	int ret;

	/* Power up core and enable PLL */
	ret = clk_prepare_enable(lvds_tx->pll);
	if (ret < 0)
		return ret;

	/* Enable DSI Lane LDO outputs, disable pad tie low */
	mtk_lvds_tx_power_on_signal(phy);

	return 0;
}

static void mtk_lvds_tx_power_off_signal(struct phy *phy)
{
	struct mtk_lvds_tx *lvds_tx = phy_get_drvdata(phy);

	writel(0, lvds_tx->tx1_regs + LVDSTX_CTL3);

	if (lvds_tx->dual_lvds)
		writel(0, lvds_tx->tx2_regs + LVDSTX_CTL3);
}

static int mtk_lvds_tx_power_off(struct phy *phy)
{
	struct mtk_lvds_tx *lvds_tx = phy_get_drvdata(phy);

	/* Enable pad tie low, disable DSI Lane LDO outputs */
	mtk_lvds_tx_power_off_signal(phy);

	/* Disable PLL and power down core */
	clk_disable_unprepare(lvds_tx->pll);

	return 0;
}

static const struct phy_ops mtk_lvds_tx_ops = {
	.power_on = mtk_lvds_tx_power_on,
	.power_off = mtk_lvds_tx_power_off,
	.owner = THIS_MODULE,
};

static int mtk_lvds_tx_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_lvds_tx *lvds_tx;
	struct resource *mem;
	struct clk *ref_clk;
	const char *ref_clk_name;
	struct clk_init_data clk_init = {
		.ops = &mtk_lvds_tx_pll_ops,
		.num_parents = 1,
		.parent_names = (const char * const *)&ref_clk_name,
		.flags = CLK_SET_RATE_GATE,
	};
	struct phy *phy;
	struct phy_provider *phy_provider;
	int ret;

	lvds_tx = devm_kzalloc(dev, sizeof(*lvds_tx), GFP_KERNEL);
	if (!lvds_tx)
		return -ENOMEM;

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	lvds_tx->tx1_regs = devm_ioremap_resource(dev, mem);
	if (IS_ERR(lvds_tx->tx1_regs)) {
		ret = PTR_ERR(lvds_tx->tx1_regs);
		dev_err(dev, "Failed to get lvds1 memory resource: %d\n", ret);
		return ret;
	}

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	lvds_tx->tx2_regs = devm_ioremap_resource(dev, mem);
	if (IS_ERR(lvds_tx->regs)) {
		ret = PTR_ERR(lvds_tx->regs);
		dev_err(dev, "Failed to get lvds2 memory resource: %d\n", ret);
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

	lvds_tx->pll_hw.init = &clk_init;
	lvds_tx->pll = devm_clk_register(dev, &lvds_tx->pll_hw);
	if (IS_ERR(lvds_tx->pll)) {
		ret = PTR_ERR(lvds_tx->pll);
		dev_err(dev, "Failed to register PLL: %d\n", ret);
		return ret;
	}

	phy = devm_phy_create(dev, NULL, &mtk_lvds_tx_ops);
	if (IS_ERR(phy)) {
		ret = PTR_ERR(phy);
		dev_err(dev, "Failed to create lvds D-PHY: %d\n", ret);
		return ret;
	}
	phy_set_drvdata(phy, lvds_tx);

	phy_provider = devm_of_phy_provider_register(dev, of_phy_simple_xlate);
	if (IS_ERR(phy)) {
		ret = PTR_ERR(phy_provider);
		dev_err(dev, "Failed to phy_provider: %d\n", ret);
		return ret;
	}

	lvds_tx->dev = dev;

	return of_clk_add_provider(dev->of_node, of_clk_src_simple_get,
				   lvds_tx->pll);
}

static int mtk_lvds_tx_remove(struct platform_device *pdev)
{
	of_clk_del_provider(pdev->dev.of_node);
	return 0;
}

static const struct of_device_id mtk_lvds_tx_match[] = {
	{ .compatible = "mediatek,mt8173-lvds-tx" },
	{},
};

struct platform_driver mtk_lvds_tx_driver = {
	.probe = mtk_lvds_tx_probe,
	.remove = mtk_lvds_tx_remove,
	.driver = {
		.name = "mediatek-mt8173-lvds-tx",
		.of_match_table = mtk_lvds_tx_match,
	},
};
