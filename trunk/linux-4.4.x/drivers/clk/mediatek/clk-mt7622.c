/*
 * Copyright (c) 2017 MediaTek Inc.
 * Author: Chen Zhong <chen.zhong@mediatek.com>
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
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/slab.h>
#include <linux/mfd/syscon.h>

#include "clk-mtk.h"
#include "clk-gate.h"

#include <dt-bindings/clock/mt7622-clk.h>

static DEFINE_SPINLOCK(mt7622_clk_lock);

static const struct mtk_fixed_clk top_fixed_clks[] __initconst = {
	FIXED_CLK(CLK_TOP_TO_U2_PHY, "to_u2_phy", "clkxtal", 31250000),
	FIXED_CLK(CLK_TOP_TO_U2_PHY_1P, "to_u2_phy_1p", "clkxtal", 31250000),
	FIXED_CLK(CLK_TOP_PCIE0_PIPE_EN, "pcie0_pipe_en", "clkxtal", 125000000),
	FIXED_CLK(CLK_TOP_PCIE1_PIPE_EN, "pcie1_pipe_en", "clkxtal", 125000000),
	FIXED_CLK(CLK_TOP_SSUSB_TX250M, "ssusb_tx250m", "clkxtal", 250000000),
	FIXED_CLK(CLK_TOP_SSUSB_EQ_RX250M, "ssusb_eq_rx250m", "clkxtal", 250000000),
	FIXED_CLK(CLK_TOP_SSUSB_CDR_REF, "ssusb_cdr_ref", "clkxtal", 33333333),
	FIXED_CLK(CLK_TOP_SSUSB_CDR_FB, "ssusb_cdr_fb", "clkxtal", 50000000),
	FIXED_CLK(CLK_TOP_SATA_ASIC, "sata_asic", "clkxtal", 50000000),
	FIXED_CLK(CLK_TOP_SATA_RBC, "sata_rbc", "clkxtal", 50000000),
};

static const struct mtk_fixed_factor top_divs[] __initconst = {
	FACTOR(CLK_TOP_TO_USB3_SYS, "to_usb3_sys", "eth1pll", 1, 4),
	FACTOR(CLK_TOP_P1_1MHZ, "p1_1mhz", "eth1pll", 1, 500),
	FACTOR(CLK_TOP_4MHZ, "free_run_4mhz", "eth1pll", 1, 125),
	FACTOR(CLK_TOP_P0_1MHZ, "p0_1mhz", "eth1pll", 1, 500),
	FACTOR(CLK_TOP_TXCLK_SRC_PRE, "txclk_src_pre", "sgmiipll_d2", 1, 1),
	FACTOR(CLK_TOP_RTC, "rtc", "clkxtal", 1, 1024),
	FACTOR(CLK_TOP_MEMPLL, "mempll", "clkxtal", 32, 1),
	FACTOR(CLK_TOP_DMPLL, "dmpll_ck", "mempll", 1, 1),
	FACTOR(CLK_TOP_SYSPLL_D2, "syspll_d2", "mainpll", 1, 2),
	FACTOR(CLK_TOP_SYSPLL1_D2, "syspll1_d2", "mainpll", 1, 4),
	FACTOR(CLK_TOP_SYSPLL1_D4, "syspll1_d4", "mainpll", 1, 8),
	FACTOR(CLK_TOP_SYSPLL1_D8, "syspll1_d8", "mainpll", 1, 16),
	FACTOR(CLK_TOP_SYSPLL2_D4, "syspll2_d4", "mainpll", 1, 12),
	FACTOR(CLK_TOP_SYSPLL2_D8, "syspll2_d8", "mainpll", 1, 24),
	FACTOR(CLK_TOP_SYSPLL_D5, "syspll_d5", "mainpll", 1, 5),
	FACTOR(CLK_TOP_SYSPLL3_D2, "syspll3_d2", "mainpll", 1, 10),
	FACTOR(CLK_TOP_SYSPLL3_D4, "syspll3_d4", "mainpll", 1, 20),
	FACTOR(CLK_TOP_SYSPLL4_D2, "syspll4_d2", "mainpll", 1, 14),
	FACTOR(CLK_TOP_SYSPLL4_D4, "syspll4_d4", "mainpll", 1, 28),
	FACTOR(CLK_TOP_SYSPLL4_D16, "syspll4_d16", "mainpll", 1, 112),
	FACTOR(CLK_TOP_UNIVPLL, "univpll", "univ2pll", 1, 2),
	FACTOR(CLK_TOP_UNIVPLL_D2, "univpll_d2", "univpll", 1, 2),
	FACTOR(CLK_TOP_UNIVPLL1_D2, "univpll1_d2", "univpll", 1, 4),
	FACTOR(CLK_TOP_UNIVPLL1_D4, "univpll1_d4", "univpll", 1, 8),
	FACTOR(CLK_TOP_UNIVPLL1_D8, "univpll1_d8", "univpll", 1, 16),
	FACTOR(CLK_TOP_UNIVPLL1_D16, "univpll1_d16", "univpll", 1, 32),
	FACTOR(CLK_TOP_UNIVPLL2_D2, "univpll2_d2", "univpll", 1, 6),
	FACTOR(CLK_TOP_UNIVPLL2_D4, "univpll2_d4", "univpll", 1, 12),
	FACTOR(CLK_TOP_UNIVPLL2_D8, "univpll2_d8", "univpll", 1, 24),
	FACTOR(CLK_TOP_UNIVPLL2_D16, "univpll2_d16", "univpll", 1, 48),
	FACTOR(CLK_TOP_UNIVPLL_D5, "univpll_d5", "univpll", 1, 5),
	FACTOR(CLK_TOP_UNIVPLL3_D2, "univpll3_d2", "univpll", 1, 10),
	FACTOR(CLK_TOP_UNIVPLL3_D4, "univpll3_d4", "univpll", 1, 20),
	FACTOR(CLK_TOP_UNIVPLL3_D16, "univpll3_d16", "univpll", 1, 80),
	FACTOR(CLK_TOP_UNIVPLL_D7, "univpll_d7", "univpll", 1, 7),
	FACTOR(CLK_TOP_UNIVPLL_D80_D4, "univpll_d80_d4", "univpll", 1, 320),
	FACTOR(CLK_TOP_UNIV48M, "univ48m", "univpll", 1, 25),
	FACTOR(CLK_TOP_SGMIIPLL, "sgmiipll_ck", "sgmipll", 1, 1),
	FACTOR(CLK_TOP_SGMIIPLL_D2, "sgmiipll_d2", "sgmipll", 1, 2),
	FACTOR(CLK_TOP_AUD1PLL, "aud1pll_ck", "aud1pll", 1, 1),
	FACTOR(CLK_TOP_AUD2PLL, "aud2pll_ck", "aud2pll", 1, 1),
	FACTOR(CLK_TOP_AUD_I2S2_MCK, "aud_i2s2_mck", "i2s2_mck_sel", 1, 2),
	FACTOR(CLK_TOP_TO_USB3_REF, "to_usb3_ref", "univpll2_d4", 1, 4),
	FACTOR(CLK_TOP_PCIE1_MAC_EN, "pcie1_mac_en", "univpll1_d4", 1, 1),
	FACTOR(CLK_TOP_PCIE0_MAC_EN, "pcie0_mac_en", "univpll1_d4", 1, 1),
	FACTOR(CLK_TOP_ETH_500M, "eth_500m", "eth1pll", 1, 1),
};

static const char * const axi_parents[] __initconst = {
	"clkxtal",
	"syspll1_d2",
	"syspll_d5",
	"syspll1_d4",
	"univpll_d5",
	"univpll2_d2",
	"univpll_d7"
};

static const char * const mem_parents[] __initconst = {
	"clkxtal",
	"dmpll_ck"
};

static const char * const ddrphycfg_parents[] __initconst = {
	"clkxtal",
	"syspll1_d8"
};

static const char * const eth_parents[] __initconst = {
	"clkxtal",
	"syspll1_d2",
	"univpll1_d2",
	"syspll1_d4",
	"univpll_d5",
	"clk_null",
	"univpll_d7"
};

static const char * const pwm_parents[] __initconst = {
	"clkxtal",
	"univpll2_d4"
};

static const char * const f10m_ref_parents[] __initconst = {
	"clkxtal",
	"syspll4_d16"
};

static const char * const nfi_infra_parents[] __initconst = {
	"clkxtal",
	"clkxtal",
	"clkxtal",
	"clkxtal",
	"clkxtal",
	"clkxtal",
	"clkxtal",
	"clkxtal",
	"univpll2_d8",
	"syspll1_d8",
	"univpll1_d8",
	"syspll4_d2",
	"univpll2_d4",
	"univpll3_d2",
	"syspll1_d4"
};

static const char * const flash_parents[] __initconst = {
	"clkxtal",
	"univpll_d80_d4",
	"syspll2_d8",
	"syspll3_d4",
	"univpll3_d4",
	"univpll1_d8",
	"syspll2_d4",
	"univpll2_d4"
};

static const char * const uart_parents[] __initconst = {
	"clkxtal",
	"univpll2_d8"
};

static const char * const spi0_parents[] __initconst = {
	"clkxtal",
	"syspll3_d2",
	"clkxtal",
	"syspll2_d4",
	"syspll4_d2",
	"univpll2_d4",
	"univpll1_d8",
	"clkxtal"
};

static const char * const spi1_parents[] __initconst = {
	"clkxtal",
	"syspll3_d2",
	"clkxtal",
	"syspll4_d4",
	"syspll4_d2",
	"univpll2_d4",
	"univpll1_d8",
	"clkxtal"
};

static const char * const msdc30_0_parents[] __initconst = {
	"clkxtal",
	"univpll2_d16",
	"univ48m"
};

static const char * const a1sys_hp_parents[] __initconst = {
	"clkxtal",
	"aud1pll_ck",
	"aud2pll_ck",
	"clkxtal"
};

static const char * const intdir_parents[] __initconst = {
	"clkxtal",
	"syspll_d2",
	"univpll_d2",
	"sgmiipll_ck"
};

static const char * const aud_intbus_parents[] __initconst = {
	"clkxtal",
	"syspll1_d4",
	"syspll4_d2",
	"syspll3_d2"
};

static const char * const pmicspi_parents[] __initconst = {
	"clkxtal",
	"clk_null",
	"clk_null",
	"clk_null",
	"clk_null",
	"univpll2_d16"
};

static const char * const atb_parents[] __initconst = {
	"clkxtal",
	"syspll1_d2",
	"syspll_d5"
};

static const char * const audio_parents[] __initconst = {
	"clkxtal",
	"syspll3_d4",
	"syspll4_d4",
	"univpll1_d16"
};

static const char * const usb20_parents[] __initconst = {
	"clkxtal",
	"univpll3_d4",
	"syspll1_d8",
	"clkxtal"
};

static const char * const aud1_parents[] __initconst = {
	"clkxtal",
	"aud1pll_ck"
};

static const char * const aud2_parents[] __initconst = {
	"clkxtal",
	"aud2pll_ck"
};

static const char * const asm_l_parents[] __initconst = {
	"clkxtal",
	"syspll_d5",
	"univpll2_d2",
	"univpll2_d4"
};

static const char * const apll1_ck_parents[] __initconst = {
	"aud1_sel",
	"aud2_sel"
};

static struct mtk_composite top_muxes[] __initdata = {
	/* CLK_CFG_0 */
	MUX_GATE(CLK_TOP_AXI_SEL, "axi_sel", axi_parents, 0x040, 0, 3, 7),
	MUX_GATE(CLK_TOP_MEM_SEL, "mem_sel", mem_parents, 0x040, 8, 1, 15),
	MUX_GATE(CLK_TOP_DDRPHYCFG_SEL, "ddrphycfg_sel", ddrphycfg_parents, 0x040, 16, 1, 23),
	MUX_GATE(CLK_TOP_ETH_SEL, "eth_sel", eth_parents, 0x040, 24, 3, 31),
	/* CLK_CFG_1 */
	MUX_GATE(CLK_TOP_PWM_SEL, "pwm_sel", pwm_parents, 0x050, 0, 2, 7),
	MUX_GATE(CLK_TOP_F10M_REF_SEL, "f10m_ref_sel", f10m_ref_parents, 0x050, 8, 1, 15),
	MUX_GATE(CLK_TOP_NFI_INFRA_SEL, "nfi_infra_sel", nfi_infra_parents, 0x050, 16, 4, 23),
	MUX_GATE(CLK_TOP_FLASH_SEL, "flash_sel", flash_parents, 0x050, 24, 3, 31),
	/* CLK_CFG_2 */
	MUX_GATE(CLK_TOP_UART_SEL, "uart_sel", uart_parents, 0x060, 0, 1, 7),
	MUX_GATE(CLK_TOP_SPI0_SEL, "spi0_sel", spi0_parents, 0x060, 8, 3, 15),
	MUX_GATE(CLK_TOP_SPI1_SEL, "spi1_sel", spi1_parents, 0x060, 16, 3, 23),
	MUX_GATE(CLK_TOP_MSDC50_0_SEL, "msdc50_0_sel", uart_parents, 0x060, 24, 3, 31),
	/* CLK_CFG_3 */
	MUX_GATE(CLK_TOP_MSDC30_0_SEL, "msdc30_0_sel", msdc30_0_parents, 0x070, 0, 3, 7),
	MUX_GATE(CLK_TOP_MSDC30_1_SEL, "msdc30_1_sel", msdc30_0_parents, 0x070, 8, 3, 15),
	MUX_GATE(CLK_TOP_A1SYS_HP_SEL, "a1sys_hp_sel", a1sys_hp_parents, 0x070, 16, 2, 23),
	MUX_GATE(CLK_TOP_A2SYS_HP_SEL, "a2sys_hp_sel", a1sys_hp_parents, 0x070, 24, 2, 31),
	/* CLK_CFG_4 */
	MUX_GATE(CLK_TOP_INTDIR_SEL, "intdir_sel", intdir_parents, 0x080, 0, 2, 7),
	MUX_GATE(CLK_TOP_AUD_INTBUS_SEL, "aud_intbus_sel", aud_intbus_parents, 0x080, 8, 2, 15),
	MUX_GATE(CLK_TOP_PMICSPI_SEL, "pmicspi_sel", pmicspi_parents, 0x080, 16, 3, 23),
	MUX_GATE(CLK_TOP_SCP_SEL, "scp_sel", ddrphycfg_parents, 0x080, 24, 2, 31),
	/* CLK_CFG_5 */
	MUX_GATE(CLK_TOP_ATB_SEL, "atb_sel", atb_parents, 0x090, 0, 2, 7),
	MUX_GATE(CLK_TOP_HIF_SEL, "hif_sel", eth_parents, 0x090, 8, 3, 15),
	MUX_GATE(CLK_TOP_AUDIO_SEL, "audio_sel", audio_parents, 0x090, 16, 2, 23),
	MUX_GATE(CLK_TOP_U2_SEL, "usb20_sel", usb20_parents, 0x090, 24, 2, 31),
	/* CLK_CFG_6 */
	MUX_GATE(CLK_TOP_AUD1_SEL, "aud1_sel", aud1_parents, 0x0A0, 0, 1, 7),
	MUX_GATE(CLK_TOP_AUD2_SEL, "aud2_sel", aud2_parents, 0x0A0, 8, 1, 15),
	MUX_GATE(CLK_TOP_IRRX_SEL, "irrx_sel", f10m_ref_parents, 0x0A0, 16, 1, 23),
	MUX_GATE(CLK_TOP_IRTX_SEL, "irtx_sel", f10m_ref_parents, 0x0A0, 24, 1, 31),
	/* CLK_CFG_7 */
	MUX_GATE(CLK_TOP_ASM_L_SEL, "asm_l_sel", asm_l_parents, 0x0B0, 0, 2, 7),
	MUX_GATE(CLK_TOP_ASM_M_SEL, "asm_m_sel", asm_l_parents, 0x0B0, 8, 2, 15),
	MUX_GATE(CLK_TOP_ASM_H_SEL, "asm_h_sel", asm_l_parents, 0x0B0, 16, 2, 23),
	/* CLK_AUDDIV_0 */
	MUX(CLK_TOP_APLL1_SEL, "apll1_ck_sel", apll1_ck_parents, 0x120, 6, 1),
	MUX(CLK_TOP_APLL2_SEL, "apll2_ck_sel", apll1_ck_parents, 0x120, 7, 1),
	MUX(CLK_TOP_I2S0_MCK_SEL, "i2s0_mck_sel", apll1_ck_parents, 0x120, 8, 1),
	MUX(CLK_TOP_I2S1_MCK_SEL, "i2s1_mck_sel", apll1_ck_parents, 0x120, 9, 1),
	MUX(CLK_TOP_I2S2_MCK_SEL, "i2s2_mck_sel", apll1_ck_parents, 0x120, 10, 1),
	MUX(CLK_TOP_I2S3_MCK_SEL, "i2s3_mck_sel", apll1_ck_parents, 0x120, 11, 1),
};

static const char * const peribus_ck_parents[] __initconst = {
	"syspll1_d8",
	"syspll1_d4"
};

static struct mtk_composite peri_muxes[] __initdata = {
	/* PERI_GLOBALCON_CKSEL */
	MUX(CLK_PERIBUS_SEL, "peribus_ck_sel", peribus_ck_parents, 0x05C, 0, 1),
};

static const char * const infra_mux1_parents[] __initconst = {
	"clkxtal",
	"armpll",
	"main_core_en",
	"armpll"
};

static struct mtk_composite infra_muxes[] __initdata = {
	/* INFRA_TOPCKGEN_CKMUXSEL */
	MUX(CLK_INFRA_MUX1_SEL, "infra_mux1_sel", infra_mux1_parents, 0x000, 2, 2),
};

static const struct mtk_clk_divider top_adj_divs[] = {
	DIV_ADJ(CLK_TOP_APLL1_DIV, "apll1_ck_div", "apll1_ck_sel", 0x120, 24, 3),
	DIV_ADJ(CLK_TOP_APLL2_DIV, "apll2_ck_div", "apll2_ck_sel", 0x120, 28, 3),
	DIV_ADJ(CLK_TOP_I2S0_MCK_DIV, "i2s0_mck_div", "i2s0_mck_sel", 0x124, 0, 7),
	DIV_ADJ(CLK_TOP_I2S1_MCK_DIV, "i2s1_mck_div", "i2s1_mck_sel", 0x124, 8, 7),
	DIV_ADJ(CLK_TOP_I2S2_MCK_DIV, "i2s2_mck_div", "aud_i2s2_mck", 0x124, 16, 7),
	DIV_ADJ(CLK_TOP_I2S3_MCK_DIV, "i2s3_mck_div", "i2s3_mck_sel", 0x124, 24, 7),
	DIV_ADJ(CLK_TOP_A1SYS_HP_DIV, "a1sys_div", "a1sys_hp_sel", 0x128, 8, 7),
	DIV_ADJ(CLK_TOP_A2SYS_HP_DIV, "a2sys_div", "a2sys_hp_sel", 0x128, 24, 7),
};

static const struct mtk_gate_regs top0_cg_regs = {
	.set_ofs = 0x120,
	.clr_ofs = 0x120,
	.sta_ofs = 0x120,
};

static const struct mtk_gate_regs top1_cg_regs = {
	.set_ofs = 0x128,
	.clr_ofs = 0x128,
	.sta_ofs = 0x128,
};

#define GATE_TOP0(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &top0_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_no_setclr,	\
	}

#define GATE_TOP1(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &top1_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_no_setclr,	\
	}

static const struct mtk_gate top_clks[] __initconst = {
	/* TOP0 */
	GATE_TOP0(CLK_TOP_APLL1_DIV_PD, "apll1_ck_div_pd", "apll1_ck_div", 0),
	GATE_TOP0(CLK_TOP_APLL2_DIV_PD, "apll2_ck_div_pd", "apll2_ck_div", 1),
	GATE_TOP0(CLK_TOP_I2S0_MCK_DIV_PD, "i2s0_mck_div_pd", "i2s0_mck_div", 2),
	GATE_TOP0(CLK_TOP_I2S1_MCK_DIV_PD, "i2s1_mck_div_pd", "i2s1_mck_div", 3),
	GATE_TOP0(CLK_TOP_I2S2_MCK_DIV_PD, "i2s2_mck_div_pd", "i2s2_mck_div", 4),
	GATE_TOP0(CLK_TOP_I2S3_MCK_DIV_PD, "i2s3_mck_div_pd", "i2s3_mck_div", 5),
	/* TOP1 */
	GATE_TOP1(CLK_TOP_A1SYS_HP_DIV_PD, "a1sys_div_pd", "a1sys_div", 0),
	GATE_TOP1(CLK_TOP_A2SYS_HP_DIV_PD, "a2sys_div_pd", "a2sys_div", 16),
};

static const struct mtk_gate_regs infra_cg_regs = {
	.set_ofs = 0x40,
	.clr_ofs = 0x44,
	.sta_ofs = 0x48,
};

#define GATE_INFRA(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &infra_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr,	\
	}

static const struct mtk_gate infra_clks[] __initconst = {
	GATE_INFRA(CLK_INFRA_DBGCLK_PD, "infra_dbgclk_pd", "axi_sel", 0),
	GATE_INFRA(CLK_INFRA_TRNG_PD, "infra_trng_pd", "axi_sel", 2),
	GATE_INFRA(CLK_INFRA_AUDIO_PD, "infra_audio_pd", "aud_intbus_sel", 5),
	GATE_INFRA(CLK_INFRA_IRRX_PD, "infra_irrx_pd", "irrx_sel", 16),
	GATE_INFRA(CLK_INFRA_APXGPT_PD, "infra_apxgpt_pd", "f10m_ref_sel", 18),
	GATE_INFRA(CLK_INFRA_PMIC_PD, "infra_pmic_pd", "pmicspi_sel", 22),
};

static const struct mtk_gate_regs peri0_cg_regs = {
	.set_ofs = 0x8,
	.clr_ofs = 0x10,
	.sta_ofs = 0x18,
};

static const struct mtk_gate_regs peri1_cg_regs = {
	.set_ofs = 0xC,
	.clr_ofs = 0x14,
	.sta_ofs = 0x1C,
};

#define GATE_PERI0(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &peri0_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr,	\
	}

#define GATE_PERI1(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &peri1_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr,	\
	}

static const struct mtk_gate peri_clks[] __initconst = {
	/* PERI0 */
	GATE_PERI0(CLK_PERI_THERM_PD, "peri_therm_pd", "axi_sel", 1),
	GATE_PERI0(CLK_PERI_PWM1_PD, "peri_pwm1_pd", "clkxtal", 2),
	GATE_PERI0(CLK_PERI_PWM2_PD, "peri_pwm2_pd", "clkxtal", 3),
	GATE_PERI0(CLK_PERI_PWM3_PD, "peri_pwm3_pd", "clkxtal", 4),
	GATE_PERI0(CLK_PERI_PWM4_PD, "peri_pwm4_pd", "clkxtal", 5),
	GATE_PERI0(CLK_PERI_PWM5_PD, "peri_pwm5_pd", "clkxtal", 6),
	GATE_PERI0(CLK_PERI_PWM6_PD, "peri_pwm6_pd", "clkxtal", 7),
	GATE_PERI0(CLK_PERI_PWM7_PD, "peri_pwm7_pd", "clkxtal", 8),
	GATE_PERI0(CLK_PERI_PWM_PD, "peri_pwm_pd", "clkxtal", 9),
	GATE_PERI0(CLK_PERI_AP_DMA_PD, "peri_ap_dma_pd", "axi_sel", 12),
	GATE_PERI0(CLK_PERI_MSDC30_0_PD, "peri_msdc30_0", "msdc30_0_sel", 13),
	GATE_PERI0(CLK_PERI_MSDC30_1_PD, "peri_msdc30_1", "msdc30_1_sel", 14),
	GATE_PERI0(CLK_PERI_UART0_PD, "peri_uart0_pd", "axi_sel", 17),
	GATE_PERI0(CLK_PERI_UART1_PD, "peri_uart1_pd", "axi_sel", 18),
	GATE_PERI0(CLK_PERI_UART2_PD, "peri_uart2_pd", "axi_sel", 19),
	GATE_PERI0(CLK_PERI_UART3_PD, "peri_uart3_pd", "axi_sel", 20),
	GATE_PERI0(CLK_PERI_UART4_PD, "peri_uart4_pd", "axi_sel", 21),
	GATE_PERI0(CLK_PERI_BTIF_PD, "peri_btif_pd", "axi_sel", 22),
	GATE_PERI0(CLK_PERI_I2C0_PD, "peri_i2c0_pd", "axi_sel", 23),
	GATE_PERI0(CLK_PERI_I2C1_PD, "peri_i2c1_pd", "axi_sel", 24),
	GATE_PERI0(CLK_PERI_I2C2_PD, "peri_i2c2_pd", "axi_sel", 25),
	GATE_PERI0(CLK_PERI_SPI1_PD, "peri_spi1_pd", "spi1_sel", 26),
	GATE_PERI0(CLK_PERI_AUXADC_PD, "peri_auxadc_pd", "clkxtal", 27),
	GATE_PERI0(CLK_PERI_SPI0_PD, "peri_spi0_pd", "spi0_sel", 28),
	GATE_PERI0(CLK_PERI_SNFI_PD, "peri_snfi_pd", "nfi_infra_sel", 29),
	GATE_PERI0(CLK_PERI_NFI_PD, "peri_nfi_pd", "axi_sel", 30),
	GATE_PERI0(CLK_PERI_NFIECC_PD, "peri_nfiecc_pd", "axi_sel", 31),
	/* PERI1 */
	GATE_PERI1(CLK_PERI_FLASH_PD, "peri_flash_pd", "flash_sel", 1),
	GATE_PERI1(CLK_PERI_IRTX_PD, "peri_irtx_pd", "irtx_sel", 2),
};

static const struct mtk_gate_regs apmixed_cg_regs = {
	.set_ofs = 0x8,
	.clr_ofs = 0x8,
	.sta_ofs = 0x8,
};

#define GATE_APMIXED(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &apmixed_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_no_setclr_inv,	\
	}

static const struct mtk_gate apmixed_clks[] __initconst = {
	GATE_APMIXED(CLK_APMIXED_MAIN_CORE_EN, "main_core_en", "mainpll", 5),
};

static const struct mtk_gate_regs audio0_cg_regs = {
	.set_ofs = 0x0,
	.clr_ofs = 0x0,
	.sta_ofs = 0x0,
};

static const struct mtk_gate_regs audio1_cg_regs = {
	.set_ofs = 0x10,
	.clr_ofs = 0x10,
	.sta_ofs = 0x10,
};

static const struct mtk_gate_regs audio2_cg_regs = {
	.set_ofs = 0x14,
	.clr_ofs = 0x14,
	.sta_ofs = 0x14,
};

static const struct mtk_gate_regs audio3_cg_regs = {
	.set_ofs = 0x634,
	.clr_ofs = 0x634,
	.sta_ofs = 0x634,
};

#define GATE_AUDIO0(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &audio0_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_no_setclr,	\
	}

#define GATE_AUDIO1(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &audio1_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_no_setclr,	\
	}

#define GATE_AUDIO2(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &audio2_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_no_setclr,	\
	}

#define GATE_AUDIO3(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &audio3_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_no_setclr,	\
	}

static const struct mtk_gate audio_clks[] __initconst = {
	/* AUDIO0 */
	GATE_AUDIO0(CLK_AUDIO_AFE, "audio_afe", "rtc", 2),
	GATE_AUDIO0(CLK_AUDIO_HDMI, "audio_hdmi", "apll1_ck_sel", 20),
	GATE_AUDIO0(CLK_AUDIO_SPDF, "audio_spdf", "apll1_ck_sel", 21),
	GATE_AUDIO0(CLK_AUDIO_APLL, "audio_apll", "apll1_ck_sel", 23),
	/* AUDIO1 */
	GATE_AUDIO1(CLK_AUDIO_I2SIN1, "audio_i2sin1", "a1sys_hp_sel", 0),
	GATE_AUDIO1(CLK_AUDIO_I2SIN2, "audio_i2sin2", "a1sys_hp_sel", 1),
	GATE_AUDIO1(CLK_AUDIO_I2SIN3, "audio_i2sin3", "a1sys_hp_sel", 2),
	GATE_AUDIO1(CLK_AUDIO_I2SIN4, "audio_i2sin4", "a1sys_hp_sel", 3),
	GATE_AUDIO1(CLK_AUDIO_I2SO1, "audio_i2so1", "a1sys_hp_sel", 6),
	GATE_AUDIO1(CLK_AUDIO_I2SO2, "audio_i2so2", "a1sys_hp_sel", 7),
	GATE_AUDIO1(CLK_AUDIO_I2SO3, "audio_i2so3", "a1sys_hp_sel", 8),
	GATE_AUDIO1(CLK_AUDIO_I2SO4, "audio_i2so4", "a1sys_hp_sel", 9),
	GATE_AUDIO1(CLK_AUDIO_ASRCI1, "audio_asrci1", "asm_h_sel", 12),
	GATE_AUDIO1(CLK_AUDIO_ASRCI2, "audio_asrci2", "asm_h_sel", 13),
	GATE_AUDIO1(CLK_AUDIO_ASRCO1, "audio_asrco1", "asm_h_sel", 14),
	GATE_AUDIO1(CLK_AUDIO_ASRCO2, "audio_asrco2", "asm_h_sel", 15),
	GATE_AUDIO1(CLK_AUDIO_INTDIR, "audio_intdir", "intdir_sel", 20),
	GATE_AUDIO1(CLK_AUDIO_A1SYS, "audio_a1sys", "a1sys_hp_sel", 21),
	GATE_AUDIO1(CLK_AUDIO_A2SYS, "audio_a2sys", "a2sys_hp_sel", 22),
	/* AUDIO2 */
	GATE_AUDIO2(CLK_AUDIO_UL1, "audio_ul1", "a1sys_hp_sel", 0),
	GATE_AUDIO2(CLK_AUDIO_UL2, "audio_ul2", "a1sys_hp_sel", 1),
	GATE_AUDIO2(CLK_AUDIO_UL3, "audio_ul3", "a1sys_hp_sel", 2),
	GATE_AUDIO2(CLK_AUDIO_UL4, "audio_ul4", "a1sys_hp_sel", 3),
	GATE_AUDIO2(CLK_AUDIO_UL5, "audio_ul5", "a1sys_hp_sel", 4),
	GATE_AUDIO2(CLK_AUDIO_UL6, "audio_ul6", "a1sys_hp_sel", 5),
	GATE_AUDIO2(CLK_AUDIO_DL1, "audio_dl1", "a1sys_hp_sel", 6),
	GATE_AUDIO2(CLK_AUDIO_DL2, "audio_dl2", "a1sys_hp_sel", 7),
	GATE_AUDIO2(CLK_AUDIO_DL3, "audio_dl3", "a1sys_hp_sel", 8),
	GATE_AUDIO2(CLK_AUDIO_DL4, "audio_dl4", "a1sys_hp_sel", 9),
	GATE_AUDIO2(CLK_AUDIO_DL5, "audio_dl5", "a1sys_hp_sel", 10),
	GATE_AUDIO2(CLK_AUDIO_DL6, "audio_dl6", "a1sys_hp_sel", 11),
	GATE_AUDIO2(CLK_AUDIO_DLMCH, "audio_dlmch", "a1sys_hp_sel", 12),
	GATE_AUDIO2(CLK_AUDIO_ARB1, "audio_arb1", "a1sys_hp_sel", 13),
	GATE_AUDIO2(CLK_AUDIO_AWB, "audio_awb", "a1sys_hp_sel", 14),
	GATE_AUDIO2(CLK_AUDIO_AWB2, "audio_awb2", "a1sys_hp_sel", 15),
	GATE_AUDIO2(CLK_AUDIO_DAI, "audio_dai", "a1sys_hp_sel", 16),
	GATE_AUDIO2(CLK_AUDIO_MOD, "audio_mod", "a1sys_hp_sel", 17),
	/* AUDIO3 */
	GATE_AUDIO3(CLK_AUDIO_ASRCI3, "audio_asrci3", "asm_h_sel", 2),
	GATE_AUDIO3(CLK_AUDIO_ASRCI4, "audio_asrci4", "asm_h_sel", 3),
	GATE_AUDIO3(CLK_AUDIO_ASRCO3, "audio_asrco3", "asm_h_sel", 6),
	GATE_AUDIO3(CLK_AUDIO_ASRCO4, "audio_asrco4", "asm_h_sel", 7),
	GATE_AUDIO3(CLK_AUDIO_MEM_ASRC1, "audio_mem_asrc1", "asm_h_sel", 10),
	GATE_AUDIO3(CLK_AUDIO_MEM_ASRC2, "audio_mem_asrc2", "asm_h_sel", 11),
	GATE_AUDIO3(CLK_AUDIO_MEM_ASRC3, "audio_mem_asrc3", "asm_h_sel", 12),
	GATE_AUDIO3(CLK_AUDIO_MEM_ASRC4, "audio_mem_asrc4", "asm_h_sel", 13),
	GATE_AUDIO3(CLK_AUDIO_MEM_ASRC5, "audio_mem_asrc5", "asm_h_sel", 14),
};

static const struct mtk_gate_regs ssusb_cg_regs = {
	.set_ofs = 0x30,
	.clr_ofs = 0x30,
	.sta_ofs = 0x30,
};

#define GATE_SSUSB(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &ssusb_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_no_setclr_inv,	\
	}

static const struct mtk_gate ssusb_clks[] __initconst = {
	GATE_SSUSB(CLK_SSUSB_U2_PHY_1P_EN, "ssusb_u2_phy_1p", "to_u2_phy_1p", 0),
	GATE_SSUSB(CLK_SSUSB_U2_PHY_EN, "ssusb_u2_phy_en", "to_u2_phy", 1),
	GATE_SSUSB(CLK_SSUSB_REF_EN, "ssusb_ref_en", "to_usb3_ref", 5),
	GATE_SSUSB(CLK_SSUSB_SYS_EN, "ssusb_sys_en", "to_usb3_sys", 6),
	GATE_SSUSB(CLK_SSUSB_MCU_EN, "ssusb_mcu_en", "axi_sel", 7),
	GATE_SSUSB(CLK_SSUSB_DMA_EN, "ssusb_dma_en", "hif_sel", 8),
};

static const struct mtk_gate_regs pcie_cg_regs = {
	.set_ofs = 0x30,
	.clr_ofs = 0x30,
	.sta_ofs = 0x30,
};

#define GATE_PCIE(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &pcie_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_no_setclr_inv,	\
	}

static const struct mtk_gate pcie_clks[] __initconst = {
	GATE_PCIE(CLK_PCIE_P1_AUX_EN, "pcie_p1_aux_en", "p1_1mhz", 12),
	GATE_PCIE(CLK_PCIE_P1_OBFF_EN, "pcie_p1_obff_en", "free_run_4mhz", 13),
	GATE_PCIE(CLK_PCIE_P1_AHB_EN, "pcie_p1_ahb_en", "axi_sel", 14),
	GATE_PCIE(CLK_PCIE_P1_AXI_EN, "pcie_p1_axi_en", "hif_sel", 15),
	GATE_PCIE(CLK_PCIE_P1_MAC_EN, "pcie_p1_mac_en", "pcie1_mac_en", 16),
	GATE_PCIE(CLK_PCIE_P1_PIPE_EN, "pcie_p1_pipe_en", "pcie1_pipe_en", 17),
	GATE_PCIE(CLK_PCIE_P0_AUX_EN, "pcie_p0_aux_en", "p0_1mhz", 18),
	GATE_PCIE(CLK_PCIE_P0_OBFF_EN, "pcie_p0_obff_en", "free_run_4mhz", 19),
	GATE_PCIE(CLK_PCIE_P0_AHB_EN, "pcie_p0_ahb_en", "axi_sel", 20),
	GATE_PCIE(CLK_PCIE_P0_AXI_EN, "pcie_p0_axi_en", "hif_sel", 21),
	GATE_PCIE(CLK_PCIE_P0_MAC_EN, "pcie_p0_mac_en", "pcie0_mac_en", 22),
	GATE_PCIE(CLK_PCIE_P0_PIPE_EN, "pcie_p0_pipe_en", "pcie0_pipe_en", 23),
	GATE_PCIE(CLK_SATA_AHB_EN, "sata_ahb_en", "axi_sel", 26),
	GATE_PCIE(CLK_SATA_AXI_EN, "sata_axi_en", "hif_sel", 27),
	GATE_PCIE(CLK_SATA_ASIC_EN, "sata_asic_en", "sata_asic", 28),
	GATE_PCIE(CLK_SATA_RBC_EN, "sata_rbc_en", "sata_rbc", 29),
	GATE_PCIE(CLK_SATA_PM_EN, "sata_pm_en", "univpll2_d4", 30),
};

static const struct mtk_gate_regs eth_cg_regs = {
	.set_ofs = 0x30,
	.clr_ofs = 0x30,
	.sta_ofs = 0x30,
};

#define GATE_ETH(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &eth_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_no_setclr_inv,	\
	}

static const struct mtk_gate eth_clks[] __initconst = {
	GATE_ETH(CLK_ETH_HSDMA_EN, "eth_hsdma_en", "eth_sel", 5),
	GATE_ETH(CLK_ETH_ESW_EN, "eth_esw_en", "eth_500m", 6),
	GATE_ETH(CLK_ETH_GP2_EN, "eth_gp2_en", "txclk_src_pre", 7),
	GATE_ETH(CLK_ETH_GP1_EN, "eth_gp1_en", "txclk_src_pre", 8),
	GATE_ETH(CLK_ETH_GP0_EN, "eth_gp0_en", "txclk_src_pre", 9),
};

static const struct mtk_gate_regs sgmii_cg_regs = {
	.set_ofs = 0xE4,
	.clr_ofs = 0xE4,
	.sta_ofs = 0xE4,
};

#define GATE_SGMII(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &sgmii_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_no_setclr_inv,	\
	}

static const struct mtk_gate sgmii_clks[] __initconst = {
	GATE_SGMII(CLK_SGMII_TX250M_EN, "sgmii_tx250m_en", "ssusb_tx250m", 2),
	GATE_SGMII(CLK_SGMII_RX250M_EN, "sgmii_rx250m_en", "ssusb_eq_rx250m", 3),
	GATE_SGMII(CLK_SGMII_CDR_REF, "sgmii_cdr_ref", "ssusb_cdr_ref", 4),
	GATE_SGMII(CLK_SGMII_CDR_FB, "sgmii_cdr_fb", "ssusb_cdr_fb", 5),
};

#define MT7622_PLL_FMAX		(2500UL * MHZ)

#define CON0_MT7622_RST_BAR	BIT(27)

#define PLL_B(_id, _name, _reg, _pwr_reg, _en_mask, _flags, _pcwbits,	\
			_pd_reg, _pd_shift, _tuner_reg, _pcw_reg,	\
			_pcw_shift, _div_table, _parent_name) {		\
		.id = _id,						\
		.name = _name,						\
		.reg = _reg,						\
		.pwr_reg = _pwr_reg,					\
		.en_mask = _en_mask,					\
		.flags = _flags,					\
		.rst_bar_mask = CON0_MT7622_RST_BAR,			\
		.fmax = MT7622_PLL_FMAX,				\
		.pcwbits = _pcwbits,					\
		.pd_reg = _pd_reg,					\
		.pd_shift = _pd_shift,					\
		.tuner_reg = _tuner_reg,				\
		.pcw_reg = _pcw_reg,					\
		.pcw_shift = _pcw_shift,				\
		.div_table = _div_table,				\
		.parent_name = _parent_name,				\
	}

#define PLL(_id, _name, _reg, _pwr_reg, _en_mask, _flags, _pcwbits,	\
			_pd_reg, _pd_shift, _tuner_reg, _pcw_reg,	\
			_pcw_shift, _parent_name)				\
		PLL_B(_id, _name, _reg, _pwr_reg, _en_mask, _flags, _pcwbits, \
			_pd_reg, _pd_shift, _tuner_reg, _pcw_reg, _pcw_shift, \
			NULL, _parent_name)

static const struct mtk_pll_data plls[] = {
	PLL(CLK_APMIXED_ARMPLL, "armpll", 0x0200, 0x020C, 0x00000001,
	    0, 21, 0x0204, 24, 0, 0x0204, 0, "clksrc_pll"),
	PLL(CLK_APMIXED_MAINPLL, "mainpll", 0x0210, 0x021C, 0x00000001,
	    HAVE_RST_BAR, 21, 0x0214, 24, 0, 0x0214, 0, "clksrc_pll"),
	PLL(CLK_APMIXED_UNIV2PLL, "univ2pll", 0x0220, 0x022C, 0x00000001,
	    HAVE_RST_BAR, 7, 0x0224, 24, 0, 0x0224, 14, "clksrc_pll"),
	PLL(CLK_APMIXED_ETH1PLL, "eth1pll", 0x0300, 0x0310, 0x00000001,
	    0, 21, 0x0300, 1, 0, 0x0304, 0, "clksrc_pll"),
	PLL(CLK_APMIXED_ETH2PLL, "eth2pll", 0x0314, 0x0320, 0x00000001,
	    0, 21, 0x0314, 1, 0, 0x0318, 0, "clksrc_pll"),
	PLL(CLK_APMIXED_AUD1PLL, "aud1pll", 0x0324, 0x0330, 0x00000001,
	    0, 31, 0x0324, 1, 0, 0x0328, 0, "clksrc_pll"),
	PLL(CLK_APMIXED_AUD2PLL, "aud2pll", 0x0334, 0x0340, 0x00000001,
	    0, 31, 0x0334, 1, 0, 0x0338, 0, "clksrc_pll"),
	PLL(CLK_APMIXED_TRGPLL, "trgpll", 0x0344, 0x0354, 0x00000001,
	    0, 21, 0x0344, 1, 0, 0x0348, 0, "clksrc_pll"),
	PLL(CLK_APMIXED_SGMIPLL, "sgmipll", 0x0358, 0x0368, 0x00000001,
	    0, 21, 0x0358, 1, 0, 0x035C, 0, "clksrc_pll"),
};

static struct clk_onecell_data *mt7622_top_clk_data __initdata;
static struct clk_onecell_data *mt7622_pll_clk_data __initdata;

static void __init mtk_clk_enable_critical(void)
{
	if (!mt7622_top_clk_data || !mt7622_pll_clk_data)
		return;

	clk_prepare_enable(mt7622_pll_clk_data->clks[CLK_APMIXED_ARMPLL]);
	clk_prepare_enable(mt7622_top_clk_data->clks[CLK_TOP_AXI_SEL]);
	clk_prepare_enable(mt7622_top_clk_data->clks[CLK_TOP_MEM_SEL]);
	clk_prepare_enable(mt7622_top_clk_data->clks[CLK_TOP_DDRPHYCFG_SEL]);
}

static void __init mtk_topckgen_init(struct device_node *node)
{
	int r;
	void __iomem *base;

	base = of_iomap(node, 0);
	if (!base) {
		pr_err("%s(): ioremap failed\n", __func__);
		return;
	}

	mt7622_top_clk_data = mtk_alloc_clk_data(CLK_TOP_NR_CLK);

	mtk_clk_register_fixed_clks(top_fixed_clks, ARRAY_SIZE(top_fixed_clks), mt7622_top_clk_data);
	mtk_clk_register_factors(top_divs, ARRAY_SIZE(top_divs), mt7622_top_clk_data);
	mtk_clk_register_composites(top_muxes, ARRAY_SIZE(top_muxes), base, &mt7622_clk_lock, mt7622_top_clk_data);
	mtk_clk_register_dividers(top_adj_divs, ARRAY_SIZE(top_adj_divs), base, &mt7622_clk_lock, mt7622_top_clk_data);
	mtk_clk_register_gates(node, top_clks, ARRAY_SIZE(top_clks), mt7622_top_clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, mt7622_top_clk_data);

	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);

	mtk_clk_enable_critical();
}
CLK_OF_DECLARE(mtk_topckgen, "mediatek,mt7622-topckgen", mtk_topckgen_init);

static void __init mtk_infracfg_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	int r;
	void __iomem *base;

	base = of_iomap(node, 0);
	if (!base) {
		pr_err("%s(): ioremap failed\n", __func__);
		return;
	}

	clk_data = mtk_alloc_clk_data(CLK_INFRA_NR_CLK);

	mtk_clk_register_composites(infra_muxes, ARRAY_SIZE(infra_muxes), base, &mt7622_clk_lock, clk_data);
	mtk_clk_register_gates(node, infra_clks, ARRAY_SIZE(infra_clks), clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);

	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);
}
CLK_OF_DECLARE(mtk_infracfg, "mediatek,mt7622-infracfg", mtk_infracfg_init);

static void __init mtk_pericfg_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	int r;
	void __iomem *base;

	base = of_iomap(node, 0);
	if (!base) {
		pr_err("%s(): ioremap failed\n", __func__);
		return;
	}

	clk_data = mtk_alloc_clk_data(CLK_PERI_NR_CLK);

	mtk_clk_register_composites(peri_muxes, ARRAY_SIZE(peri_muxes), base, &mt7622_clk_lock, clk_data);
	mtk_clk_register_gates(node, peri_clks, ARRAY_SIZE(peri_clks), clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);

	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);
}
CLK_OF_DECLARE(mtk_pericfg, "mediatek,mt7622-pericfg", mtk_pericfg_init);

static void __init mtk_apmixedsys_init(struct device_node *node)
{
	int r;

	mt7622_pll_clk_data = mtk_alloc_clk_data(CLK_APMIXED_NR_CLK);

	mtk_clk_register_plls(node, plls, ARRAY_SIZE(plls), mt7622_pll_clk_data);
	mtk_clk_register_gates(node, apmixed_clks, ARRAY_SIZE(apmixed_clks), mt7622_pll_clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, mt7622_pll_clk_data);

	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);

	mtk_clk_enable_critical();
}
CLK_OF_DECLARE(mtk_apmixedsys, "mediatek,mt7622-apmixedsys", mtk_apmixedsys_init);

static void __init mtk_audiosys_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	int r;

	clk_data = mtk_alloc_clk_data(CLK_AUDIO_NR_CLK);

	mtk_clk_register_gates(node, audio_clks, ARRAY_SIZE(audio_clks), clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);

	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);
}
CLK_OF_DECLARE(mtk_audiosys, "mediatek,mt7622-audiosys", mtk_audiosys_init);

static void __init mtk_ssusbsys_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	int r;

	clk_data = mtk_alloc_clk_data(CLK_SSUSB_NR_CLK);

	mtk_clk_register_gates(node, ssusb_clks, ARRAY_SIZE(ssusb_clks), clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);

	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);
}
CLK_OF_DECLARE(mtk_ssusbsys, "mediatek,mt7622-ssusbsys", mtk_ssusbsys_init);

static void __init mtk_pciesys_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	int r;

	clk_data = mtk_alloc_clk_data(CLK_PCIE_NR_CLK);

	mtk_clk_register_gates(node, pcie_clks, ARRAY_SIZE(pcie_clks), clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);

	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);

	mtk_register_reset_controller(node, 1, 0x34);
}
CLK_OF_DECLARE(mtk_pciesys, "mediatek,mt7622-pciesys", mtk_pciesys_init);

static void __init mtk_ethsys_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	int r;

	clk_data = mtk_alloc_clk_data(CLK_ETH_NR_CLK);

	mtk_clk_register_gates(node, eth_clks, ARRAY_SIZE(eth_clks), clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);

	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);

	mtk_register_reset_controller(node, 1, 0x34);
}
CLK_OF_DECLARE(mtk_ethsys, "mediatek,mt7622-ethsys", mtk_ethsys_init);

static void __init mtk_sgmiisys_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	int r;

	clk_data = mtk_alloc_clk_data(CLK_SGMII_NR_CLK);

	mtk_clk_register_gates(node, sgmii_clks, ARRAY_SIZE(sgmii_clks), clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);

	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);
}
CLK_OF_DECLARE(mtk_sgmiisys, "mediatek,mt7622-sgmiisys", mtk_sgmiisys_init);

