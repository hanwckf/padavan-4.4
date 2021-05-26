/*
 * Copyright (c) 2016 MediaTek Inc.
 * Author: Jie Qiu <jie.qiu@mediatek.com>
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
#include <linux/debugfs.h>
#include <linux/kernel.h>
#include <linux/mfd/syscon.h>
#include <linux/of_platform.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/phy/phy.h>

#define HDMI_CON0	0x00
#define RG_HDMITX_DRV_IBIAS		(0)
#define RG_HDMITX_DRV_IBIAS_MASK	(0x3F << 0)
#define RG_HDMITX_EN_SER		(12)
#define RG_HDMITX_EN_SER_MASK		(0x0F << 12)
#define RG_HDMITX_EN_SLDO		(16)
#define RG_HDMITX_EN_SLDO_MASK		(0x0F << 16)
#define RG_HDMITX_EN_PRED		(20)
#define RG_HDMITX_EN_PRED_MASK		(0x0F << 20)
#define RG_HDMITX_EN_IMP		(24)
#define RG_HDMITX_EN_IMP_MASK		(0x0F << 24)
#define RG_HDMITX_EN_DRV		(28)
#define RG_HDMITX_EN_DRV_MASK		(0x0F << 28)

#define HDMI_CON1	0x04
#define RG_HDMITX_PRED_IBIAS		(18)
#define RG_HDMITX_PRED_IBIAS_MASK	(0x0F << 18)
#define RG_HDMITX_PRED_IMP		(0x01 << 22)
#define RG_HDMITX_DRV_IMP		(26)
#define RG_HDMITX_DRV_IMP_MASK		(0x3F << 26)

#define HDMI_CON2	0x08
#define RG_HDMITX_EN_TX_CKLDO		(0x01 << 0)
#define RG_HDMITX_EN_TX_POSDIV		(0x01 << 1)
#define RG_HDMITX_TX_POSDIV		(3)
#define RG_HDMITX_TX_POSDIV_MASK	(0x03 << 3)
#define RG_HDMITX_EN_MBIAS		(0x01 << 6)
#define RG_HDMITX_MBIAS_LPF_EN		(0x01 << 7)

#define HDMI_CON4	0x10
#define RG_HDMITX_RESERVE_MASK		(0xFFFFFFFF << 0)

#define HDMI_CON6	0x18
#define RG_HTPLL_BR			(0)
#define RG_HTPLL_BR_MASK		(0x03 << 0)
#define RG_HTPLL_BC			(2)
#define RG_HTPLL_BC_MASK		(0x03 << 2)
#define RG_HTPLL_BP			(4)
#define RG_HTPLL_BP_MASK		(0x0F << 4)
#define RG_HTPLL_IR			(8)
#define RG_HTPLL_IR_MASK		(0x0F << 8)
#define RG_HTPLL_IC			(12)
#define RG_HTPLL_IC_MASK		(0x0F << 12)
#define RG_HTPLL_POSDIV			(16)
#define RG_HTPLL_POSDIV_MASK		(0x03 << 16)
#define RG_HTPLL_PREDIV			(18)
#define RG_HTPLL_PREDIV_MASK		(0x03 << 18)
#define RG_HTPLL_FBKSEL			(20)
#define RG_HTPLL_FBKSEL_MASK		(0x03 << 20)
#define RG_HTPLL_RLH_EN			(0x01 << 22)
#define RG_HTPLL_FBKDIV			(24)
#define RG_HTPLL_FBKDIV_MASK		(0x7F << 24)
#define RG_HTPLL_EN			(0x01 << 31)

#define HDMI_CON7	0x1c
#define RG_HTPLL_AUTOK_EN		(0x01 << 23)
#define RG_HTPLL_DIVEN			(28)
#define RG_HTPLL_DIVEN_MASK		(0x07 << 28)

struct mtk_hdmi_phy {
	struct device *dev;
	struct clk_hw hw;
	struct clk *pll;
	struct clk_hw pll_hw;
	void __iomem *regs;
	unsigned long rate;
	u32 ibias;
	u32 ibias_up;
	u8 drv_imp_clk;
	u8 drv_imp_d2;
	u8 drv_imp_d1;
	u8 drv_imp_d0;
};

static inline struct mtk_hdmi_phy *to_mtk_hdmi_pll(struct clk_hw *hw)
{
	return container_of(hw, struct mtk_hdmi_phy, pll_hw);
}

static inline void mtk_hdmi_pll_mask(struct mtk_hdmi_phy *hdmi_phy, u32 offset,
			      u32 val, u32 mask)
{
	u32 tmp = readl(hdmi_phy->regs  + offset) & ~mask;

	tmp |= (val & mask);
	writel(tmp, hdmi_phy->regs + offset);
}

static int mtk_hdmi_pll_enable(struct clk_hw *hw)
{
	struct mtk_hdmi_phy *hdmi_phy = to_mtk_hdmi_pll(hw);

	mtk_hdmi_pll_mask(hdmi_phy, HDMI_CON7, RG_HTPLL_AUTOK_EN,
			  RG_HTPLL_AUTOK_EN);
	mtk_hdmi_pll_mask(hdmi_phy, HDMI_CON6, 0, RG_HTPLL_RLH_EN);
	mtk_hdmi_pll_mask(hdmi_phy, HDMI_CON6, (0x3 << RG_HTPLL_POSDIV),
			  RG_HTPLL_POSDIV_MASK);
	mtk_hdmi_pll_mask(hdmi_phy, HDMI_CON2, RG_HDMITX_EN_MBIAS,
			  RG_HDMITX_EN_MBIAS);
	udelay(100);
	mtk_hdmi_pll_mask(hdmi_phy, HDMI_CON6, RG_HTPLL_EN, RG_HTPLL_EN);
	mtk_hdmi_pll_mask(hdmi_phy, HDMI_CON2, RG_HDMITX_EN_TX_CKLDO,
			  RG_HDMITX_EN_TX_CKLDO);
	mtk_hdmi_pll_mask(hdmi_phy, HDMI_CON0, (0xf << RG_HDMITX_EN_SLDO),
			  RG_HDMITX_EN_SLDO_MASK);
	udelay(100);
	mtk_hdmi_pll_mask(hdmi_phy, HDMI_CON2, RG_HDMITX_MBIAS_LPF_EN,
			  RG_HDMITX_MBIAS_LPF_EN);
	mtk_hdmi_pll_mask(hdmi_phy, HDMI_CON2, RG_HDMITX_EN_TX_POSDIV,
			  RG_HDMITX_EN_TX_POSDIV);
	mtk_hdmi_pll_mask(hdmi_phy, HDMI_CON0, (0xf << RG_HDMITX_EN_SER),
			  RG_HDMITX_EN_SER_MASK);
	mtk_hdmi_pll_mask(hdmi_phy, HDMI_CON0, (0xf << RG_HDMITX_EN_PRED),
			  RG_HDMITX_EN_PRED_MASK);
	mtk_hdmi_pll_mask(hdmi_phy, HDMI_CON0, (0xf << RG_HDMITX_EN_DRV),
			  RG_HDMITX_EN_DRV_MASK);
	udelay(100);
	return 0;
}

static void mtk_hdmi_pll_disable(struct clk_hw *hw)
{
	struct mtk_hdmi_phy *hdmi_phy = to_mtk_hdmi_pll(hw);

	mtk_hdmi_pll_mask(hdmi_phy, HDMI_CON0, 0, RG_HDMITX_EN_DRV_MASK);
	mtk_hdmi_pll_mask(hdmi_phy, HDMI_CON0, 0, RG_HDMITX_EN_PRED_MASK);
	mtk_hdmi_pll_mask(hdmi_phy, HDMI_CON0, 0, RG_HDMITX_EN_SER_MASK);
	mtk_hdmi_pll_mask(hdmi_phy, HDMI_CON2, 0, RG_HDMITX_EN_TX_POSDIV);
	mtk_hdmi_pll_mask(hdmi_phy, HDMI_CON2, 0, RG_HDMITX_MBIAS_LPF_EN);
	udelay(100);
	mtk_hdmi_pll_mask(hdmi_phy, HDMI_CON0, 0, RG_HDMITX_EN_SLDO_MASK);
	mtk_hdmi_pll_mask(hdmi_phy, HDMI_CON2, 0, RG_HDMITX_EN_TX_CKLDO);
	mtk_hdmi_pll_mask(hdmi_phy, HDMI_CON6, 0, RG_HTPLL_EN);
	udelay(100);
	mtk_hdmi_pll_mask(hdmi_phy, HDMI_CON2, 0, RG_HDMITX_EN_MBIAS);
	mtk_hdmi_pll_mask(hdmi_phy, HDMI_CON6, 0, RG_HTPLL_POSDIV_MASK);
	mtk_hdmi_pll_mask(hdmi_phy, HDMI_CON6, 0, RG_HTPLL_RLH_EN);
	mtk_hdmi_pll_mask(hdmi_phy, HDMI_CON7, 0, RG_HTPLL_AUTOK_EN);
	udelay(100);
}

static int mtk_hdmi_pll_set_rate(struct clk_hw *hw, unsigned long rate,
				    unsigned long parent_rate)
{
	struct mtk_hdmi_phy *hdmi_phy = to_mtk_hdmi_pll(hw);
	u32 pos_div;

	if (rate <= 64000000)
		pos_div = 3;
	else if (rate <= 12800000)
		pos_div = 1;
	else
		pos_div = 1;

	mtk_hdmi_pll_mask(hdmi_phy, HDMI_CON6, (0x3 << RG_HTPLL_PREDIV),
			  RG_HTPLL_PREDIV_MASK);
	mtk_hdmi_pll_mask(hdmi_phy, HDMI_CON6, (0x3 << RG_HTPLL_POSDIV),
			  RG_HTPLL_POSDIV_MASK);
	mtk_hdmi_pll_mask(hdmi_phy, HDMI_CON6, (0x1 << RG_HTPLL_IC),
			  RG_HTPLL_IC_MASK);
	mtk_hdmi_pll_mask(hdmi_phy, HDMI_CON6, (0x1 << RG_HTPLL_IR),
			  RG_HTPLL_IR_MASK);
	mtk_hdmi_pll_mask(hdmi_phy, HDMI_CON2, (pos_div << RG_HDMITX_TX_POSDIV),
			  RG_HDMITX_TX_POSDIV_MASK);
	mtk_hdmi_pll_mask(hdmi_phy, HDMI_CON6, (1 << RG_HTPLL_FBKSEL),
			  RG_HTPLL_FBKSEL_MASK);
	mtk_hdmi_pll_mask(hdmi_phy, HDMI_CON6, (19 << RG_HTPLL_FBKDIV),
			  RG_HTPLL_FBKDIV_MASK);
	mtk_hdmi_pll_mask(hdmi_phy, HDMI_CON7, (0x2 << RG_HTPLL_DIVEN),
			  RG_HTPLL_DIVEN_MASK);
	mtk_hdmi_pll_mask(hdmi_phy, HDMI_CON6, (0xc << RG_HTPLL_BP),
			  RG_HTPLL_BP_MASK);
	mtk_hdmi_pll_mask(hdmi_phy, HDMI_CON6, (0x2 << RG_HTPLL_BC),
			  RG_HTPLL_BC_MASK);
	mtk_hdmi_pll_mask(hdmi_phy, HDMI_CON6, (0x1 << RG_HTPLL_BR),
			  RG_HTPLL_BR_MASK);

	mtk_hdmi_pll_mask(hdmi_phy, HDMI_CON1, 0, RG_HDMITX_PRED_IMP);
	mtk_hdmi_pll_mask(hdmi_phy, HDMI_CON1, (0x3 << RG_HDMITX_PRED_IBIAS),
			  RG_HDMITX_PRED_IBIAS_MASK);
	mtk_hdmi_pll_mask(hdmi_phy, HDMI_CON0, (0x0 << RG_HDMITX_EN_IMP),
			  RG_HDMITX_EN_IMP_MASK);
	mtk_hdmi_pll_mask(hdmi_phy, HDMI_CON1, (0x28 << RG_HDMITX_DRV_IMP),
			  RG_HDMITX_DRV_IMP_MASK);
	mtk_hdmi_pll_mask(hdmi_phy, HDMI_CON4, 0x28, RG_HDMITX_RESERVE_MASK);
	mtk_hdmi_pll_mask(hdmi_phy, HDMI_CON0, (0xa << RG_HDMITX_DRV_IBIAS),
			  RG_HDMITX_DRV_IBIAS_MASK);
	return 0;
}

static long mtk_hdmi_pll_round_rate(struct clk_hw *hw, unsigned long rate,
					unsigned long *parent_rate)
{
	struct mtk_hdmi_phy *hdmi_phy = to_mtk_hdmi_pll(hw);

	hdmi_phy->rate = rate;
	return rate;
}

static unsigned long mtk_hdmi_pll_recalc_rate(struct clk_hw *hw,
			  unsigned long parent_rate)
{
	struct mtk_hdmi_phy *hdmi_phy = to_mtk_hdmi_pll(hw);

	return hdmi_phy->rate;
}


static const struct clk_ops mtk_hdmi_pll_ops = {
	.enable = mtk_hdmi_pll_enable,
	.disable = mtk_hdmi_pll_disable,
	.set_rate = mtk_hdmi_pll_set_rate,
	.round_rate = mtk_hdmi_pll_round_rate,
	.recalc_rate = mtk_hdmi_pll_recalc_rate,
};

static void mtk_hdmi_phy_enable_tmds(struct mtk_hdmi_phy *hdmi_phy)
{
	mtk_hdmi_pll_mask(hdmi_phy, HDMI_CON7, RG_HTPLL_AUTOK_EN,
			  RG_HTPLL_AUTOK_EN);
	mtk_hdmi_pll_mask(hdmi_phy, HDMI_CON6, 0, RG_HTPLL_RLH_EN);
	mtk_hdmi_pll_mask(hdmi_phy, HDMI_CON6, (0x3 << RG_HTPLL_POSDIV),
			  RG_HTPLL_POSDIV_MASK);
	mtk_hdmi_pll_mask(hdmi_phy, HDMI_CON2, RG_HDMITX_EN_MBIAS,
			  RG_HDMITX_EN_MBIAS);
	udelay(100);
	mtk_hdmi_pll_mask(hdmi_phy, HDMI_CON6, RG_HTPLL_EN, RG_HTPLL_EN);
	mtk_hdmi_pll_mask(hdmi_phy, HDMI_CON2, RG_HDMITX_EN_TX_CKLDO,
			  RG_HDMITX_EN_TX_CKLDO);
	mtk_hdmi_pll_mask(hdmi_phy, HDMI_CON0, (0xf << RG_HDMITX_EN_SLDO),
			  RG_HDMITX_EN_SLDO_MASK);
	udelay(100);
	mtk_hdmi_pll_mask(hdmi_phy, HDMI_CON2, RG_HDMITX_MBIAS_LPF_EN,
			  RG_HDMITX_MBIAS_LPF_EN);
	mtk_hdmi_pll_mask(hdmi_phy, HDMI_CON2, RG_HDMITX_EN_TX_POSDIV,
			  RG_HDMITX_EN_TX_POSDIV);
	mtk_hdmi_pll_mask(hdmi_phy, HDMI_CON0, (0xf << RG_HDMITX_EN_SER),
			  RG_HDMITX_EN_SER_MASK);
	mtk_hdmi_pll_mask(hdmi_phy, HDMI_CON0, (0xf << RG_HDMITX_EN_PRED),
			  RG_HDMITX_EN_PRED_MASK);
	mtk_hdmi_pll_mask(hdmi_phy, HDMI_CON0, (0xf << RG_HDMITX_EN_DRV),
			  RG_HDMITX_EN_DRV_MASK);
	udelay(100);
}

static void mtk_hdmi_phy_disable_tmds(struct mtk_hdmi_phy *hdmi_phy)
{
	mtk_hdmi_pll_mask(hdmi_phy, HDMI_CON0, 0, RG_HDMITX_EN_DRV_MASK);
	mtk_hdmi_pll_mask(hdmi_phy, HDMI_CON0, 0, RG_HDMITX_EN_PRED_MASK);
	mtk_hdmi_pll_mask(hdmi_phy, HDMI_CON0, 0, RG_HDMITX_EN_SER_MASK);
	mtk_hdmi_pll_mask(hdmi_phy, HDMI_CON2, 0, RG_HDMITX_EN_TX_POSDIV);
	mtk_hdmi_pll_mask(hdmi_phy, HDMI_CON2, 0, RG_HDMITX_MBIAS_LPF_EN);
	udelay(100);
	mtk_hdmi_pll_mask(hdmi_phy, HDMI_CON0, 0, RG_HDMITX_EN_SLDO_MASK);
	mtk_hdmi_pll_mask(hdmi_phy, HDMI_CON2, 0, RG_HDMITX_EN_TX_CKLDO);
	mtk_hdmi_pll_mask(hdmi_phy, HDMI_CON6, 0, RG_HTPLL_EN);
	udelay(100);
	mtk_hdmi_pll_mask(hdmi_phy, HDMI_CON2, 0, RG_HDMITX_EN_MBIAS);
	mtk_hdmi_pll_mask(hdmi_phy, HDMI_CON6, 0, RG_HTPLL_POSDIV_MASK);
	mtk_hdmi_pll_mask(hdmi_phy, HDMI_CON6, 0, RG_HTPLL_RLH_EN);
	mtk_hdmi_pll_mask(hdmi_phy, HDMI_CON7, 0, RG_HTPLL_AUTOK_EN);
	udelay(100);
}

static int mtk_hdmi_phy_power_on(struct phy *phy)
{
	struct mtk_hdmi_phy *hdmi_phy = phy_get_drvdata(phy);
	int ret;

	ret = clk_prepare_enable(hdmi_phy->pll);
	if (ret < 0)
		return ret;

	mtk_hdmi_phy_enable_tmds(hdmi_phy);

	return 0;
}

static int mtk_hdmi_phy_power_off(struct phy *phy)
{
	struct mtk_hdmi_phy *hdmi_phy = phy_get_drvdata(phy);

	mtk_hdmi_phy_disable_tmds(hdmi_phy);
	clk_disable_unprepare(hdmi_phy->pll);

	return 0;
}

static const struct phy_ops mtk_hdmi_phy_ops = {
	.power_on = mtk_hdmi_phy_power_on,
	.power_off = mtk_hdmi_phy_power_off,
	.owner = THIS_MODULE,
};

static int mtk_hdmi_phy_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_hdmi_phy *hdmi_phy;
	struct resource *mem;
	struct clk *ref_clk;
	const char *ref_clk_name;
	struct clk_init_data clk_init = {
		.ops = &mtk_hdmi_pll_ops,
		.num_parents = 1,
		.parent_names = (const char * const *)&ref_clk_name,
		.flags = CLK_SET_RATE_PARENT | CLK_SET_RATE_GATE,
	};
	struct phy *phy;
	struct phy_provider *phy_provider;
	int ret;

	hdmi_phy = devm_kzalloc(dev, sizeof(*hdmi_phy), GFP_KERNEL);
	if (!hdmi_phy)
		return -ENOMEM;

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	hdmi_phy->regs = devm_ioremap_resource(dev, mem);
	if (IS_ERR(hdmi_phy->regs)) {
		ret = PTR_ERR(hdmi_phy->regs);
		dev_err(dev, "Failed to get memory resource: %d\n", ret);
		return ret;
	}

	ref_clk = devm_clk_get(dev, "pll_ref");
	if (IS_ERR(ref_clk)) {
		ret = PTR_ERR(ref_clk);
		dev_err(&pdev->dev, "Failed to get PLL reference clock: %d\n",
			ret);
		return ret;
	}
	ref_clk_name = __clk_get_name(ref_clk);

	ret = of_property_read_string(dev->of_node, "clock-output-names",
				      &clk_init.name);
	if (ret < 0) {
		dev_err(dev, "Failed to read clock-output-names: %d\n", ret);
		return ret;
	}

	hdmi_phy->pll_hw.init = &clk_init;
	hdmi_phy->pll = devm_clk_register(dev, &hdmi_phy->pll_hw);
	if (IS_ERR(hdmi_phy->pll)) {
		ret = PTR_ERR(hdmi_phy->pll);
		dev_err(dev, "Failed to register PLL: %d\n", ret);
		return ret;
	}

	ret = of_property_read_u32(dev->of_node, "mediatek,ibias",
				   &hdmi_phy->ibias);
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to get ibias: %d\n", ret);
		return ret;
	}

	ret = of_property_read_u32(dev->of_node, "mediatek,ibias_up",
				   &hdmi_phy->ibias_up);
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to get ibias up: %d\n", ret);
		return ret;
	}

	dev_info(dev, "Using default TX DRV impedance: 4.2k/36\n");
	hdmi_phy->drv_imp_clk = 0x30;
	hdmi_phy->drv_imp_d2 = 0x30;
	hdmi_phy->drv_imp_d1 = 0x30;
	hdmi_phy->drv_imp_d0 = 0x30;

	phy = devm_phy_create(dev, NULL, &mtk_hdmi_phy_ops);
	if (IS_ERR(phy)) {
		dev_err(dev, "Failed to create HDMI PHY\n");
		return PTR_ERR(phy);
	}
	phy_set_drvdata(phy, hdmi_phy);

	phy_provider = devm_of_phy_provider_register(dev, of_phy_simple_xlate);
	if (IS_ERR(phy_provider))
		return PTR_ERR(phy_provider);

	hdmi_phy->dev = dev;
	return of_clk_add_provider(dev->of_node, of_clk_src_simple_get,
				   hdmi_phy->pll);
}

static int mtk_hdmi_phy_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id mtk_hdmi_phy_match[] = {
	{ .compatible = "mediatek,mt2701-hdmi-phy", },
	{},
};

struct platform_driver mtk_hdmi_phy_driver = {
	.probe = mtk_hdmi_phy_probe,
	.remove = mtk_hdmi_phy_remove,
	.driver = {
		.name = "mediatek-hdmi-phy",
		.of_match_table = mtk_hdmi_phy_match,
	},
};
MODULE_AUTHOR("Jie Qiu <jie.qiu@mediatek.com>");
MODULE_DESCRIPTION("MediaTek HDMI i2c Driver");
MODULE_LICENSE("GPL v2");
