/*
 * Copyright (c) 2018 MediaTek Inc.
 * Author: Wenzhen Yu <Wenzhen Yu@mediatek.com>
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
#include <linux/delay.h>
#include <linux/mfd/syscon.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

#include "clk-gate.h"
#include "clk-mtk.h"

#include <dt-bindings/clock/leopard-clk.h>

#define GATE_ETH(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &eth_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_no_setclr_inv,	\
	}

#define GATE_SGMII(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &sgmii_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_no_setclr_inv,	\
	}

#define GATE_SGMII2(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &sgmii2_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_no_setclr_inv,	\
	}

static const struct mtk_gate_regs eth_cg_regs = {
	.set_ofs = 0x30,
	.clr_ofs = 0x30,
	.sta_ofs = 0x30,
};

static const struct mtk_gate_regs sgmii_cg_regs = {
	.set_ofs = 0xE4,
	.clr_ofs = 0xE4,
	.sta_ofs = 0xE4,
};

static const struct mtk_gate_regs sgmii2_cg_regs = {
	.set_ofs = 0xE4,
	.clr_ofs = 0xE4,
	.sta_ofs = 0xE4,
};

static const struct mtk_gate eth_clks[] __initconst = {
	GATE_ETH(CLK_ETH_FE_EN, "eth_fe_en", "eth2pll", 6),
	GATE_ETH(CLK_ETH_GP2_EN, "eth_gp2_en", "txclk_src_pre", 7),
	GATE_ETH(CLK_ETH_GP1_EN, "eth_gp1_en", "txclk_src_pre", 8),
	GATE_ETH(CLK_ETH_GP0_EN, "eth_gp0_en", "txclk_src_pre", 9),
	GATE_ETH(CLK_ETH_ESW_EN, "eth_esw_en", "eth_500m", 16),
};

static const struct mtk_gate sgmii_clks[] __initconst = {
	GATE_SGMII(CLK_SGMII0_TX_EN, "sgmii_tx_en", "ssusb_tx250m", 2),
	GATE_SGMII(CLK_SGMII0_RX_EN, "sgmii_rx_en", "ssusb_eq_rx250m", 3),
	GATE_SGMII(CLK_SGMII0_CDR_REF, "sgmii_cdr_ref", "ssusb_cdr_ref", 4),
	GATE_SGMII(CLK_SGMII0_CDR_FB, "sgmii_cdr_fb", "ssusb_cdr_fb", 5),
};

static const struct mtk_gate sgmii2_clks[] __initconst = {
	GATE_SGMII2(CLK_SGMII1_TX_EN, "sgmii2_tx_en", "ssusb_tx250m", 2),
	GATE_SGMII2(CLK_SGMII1_RX_EN, "sgmii2_rx_en", "ssusb_eq_rx250m", 3),
	GATE_SGMII2(CLK_SGMII1_CDR_REF, "sgmii2_cdr_ref", "ssusb_cdr_ref", 4),
	GATE_SGMII2(CLK_SGMII1_CDR_FB, "sgmii2_cdr_fb", "ssusb_cdr_fb", 5),
};

static void __init mtk_ethsys_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	int r;

	clk_data = mtk_alloc_clk_data(CLK_ETH_NR_CLK);

	mtk_clk_register_gates(node, eth_clks, ARRAY_SIZE(eth_clks), clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);

	if (r)
		return;
	mtk_register_reset_controller(node, 1, 0x34);
}
CLK_OF_DECLARE(mtk_ethsys, "mediatek,leopard-ethsys", mtk_ethsys_init);

static void __init mtk_sgmiisys_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	int r;

	clk_data = mtk_alloc_clk_data(CLK_SGMII0_NR_CLK);

	mtk_clk_register_gates(node, sgmii_clks, ARRAY_SIZE(sgmii_clks),
		clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get,
		clk_data);

	if (r)
		return;
	mtk_register_reset_controller(node, 1, 0x34);
}
CLK_OF_DECLARE(mtk_sgmiisys, "mediatek,leopard-sgmiisys_0",
	mtk_sgmiisys_init);

static void __init mtk_sgmiisys2_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	int r;

	clk_data = mtk_alloc_clk_data(CLK_SGMII1_NR_CLK);

	mtk_clk_register_gates(node, sgmii2_clks, ARRAY_SIZE(sgmii2_clks),
		clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get,
		clk_data);

	if (r)
		return;
	mtk_register_reset_controller(node, 1, 0x34);
}
CLK_OF_DECLARE(mtk_sgmiisys2, "mediatek,leopard-sgmiisys_1",
	mtk_sgmiisys2_init);
