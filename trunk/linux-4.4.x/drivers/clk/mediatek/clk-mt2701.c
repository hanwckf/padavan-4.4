/*
 * Copyright (c) 2014 MediaTek Inc.
 * Author: Shunli Wang <shunli.wang@mediatek.com>
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
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>

#include "clk-mtk.h"
#include "clk-gate.h"
#include "clk-cpumux.h"

#include <dt-bindings/clock/mt2701-clk.h>

/*
 * For some clocks, we don't care what their actual rates are. And these
 * clocks may change their rate on different products or different scenarios.
 * So we model these clocks' rate as 0, to denote it's not an actual rate.
 */
#define DUMMY_RATE		0

static DEFINE_SPINLOCK(lock);

static const struct mtk_fixed_clk top_fixed_clks[] __initconst = {
	FIXED_CLK(CLK_TOP_DPI, "dpi_ck", "clk26m",
		108 * MHZ),
	FIXED_CLK(CLK_TOP_DMPLL, "dmpll_ck", "clk26m",
		400 * MHZ),
	FIXED_CLK(CLK_TOP_VENCPLL, "vencpll_ck", "clk26m",
		295750000),
	FIXED_CLK(CLK_TOP_HDMI_0_PIX340M, "hdmi_0_pix340m", "clk26m",
		340 * MHZ),
	FIXED_CLK(CLK_TOP_HDMI_0_DEEP340M, "hdmi_0_deep340m", "clk26m",
		340 * MHZ),
	FIXED_CLK(CLK_TOP_HDMI_0_PLL340M, "hdmi_0_pll340m", "clk26m",
		340 * MHZ),
	FIXED_CLK(CLK_TOP_HADDS2_FB, "hadds2_fbclk", "clk26m",
		27 * MHZ),
	FIXED_CLK(CLK_TOP_WBG_DIG_416M, "wbg_dig_ck_416m", "clk26m",
		416 * MHZ),
	FIXED_CLK(CLK_TOP_DSI0_LNTC_DSI, "dsi0_lntc_dsi", "clk26m",
		143 * MHZ),
	FIXED_CLK(CLK_TOP_HDMI_SCL_RX, "hdmi_scl_rx", "clk26m",
		27 * MHZ),
	FIXED_CLK(CLK_TOP_AUD_EXT1, "aud_ext1", "clk26m",
		DUMMY_RATE),
	FIXED_CLK(CLK_TOP_AUD_EXT2, "aud_ext2", "clk26m",
		DUMMY_RATE),
	FIXED_CLK(CLK_TOP_NFI1X_PAD, "nfi1x_pad", "clk26m",
		DUMMY_RATE),
};

static const struct mtk_fixed_factor top_fixed_divs[] __initconst = {
	FACTOR(CLK_TOP_SYSPLL, "syspll_ck", "mainpll", 1, 1),
	FACTOR(CLK_TOP_SYSPLL_D2, "syspll_d2", "mainpll", 1, 2),
	FACTOR(CLK_TOP_SYSPLL_D3, "syspll_d3", "mainpll", 1, 3),
	FACTOR(CLK_TOP_SYSPLL_D5, "syspll_d5", "mainpll", 1, 5),
	FACTOR(CLK_TOP_SYSPLL_D7, "syspll_d7", "mainpll", 1, 7),
	FACTOR(CLK_TOP_SYSPLL1_D2, "syspll1_d2", "syspll_d2", 1, 2),
	FACTOR(CLK_TOP_SYSPLL1_D4, "syspll1_d4", "syspll_d2", 1, 4),
	FACTOR(CLK_TOP_SYSPLL1_D8, "syspll1_d8", "syspll_d2", 1, 8),
	FACTOR(CLK_TOP_SYSPLL1_D16, "syspll1_d16", "syspll_d2", 1, 16),
	FACTOR(CLK_TOP_SYSPLL2_D2, "syspll2_d2", "syspll_d3", 1, 2),
	FACTOR(CLK_TOP_SYSPLL2_D4, "syspll2_d4", "syspll_d3", 1, 4),
	FACTOR(CLK_TOP_SYSPLL2_D8, "syspll2_d8", "syspll_d3", 1, 8),
	FACTOR(CLK_TOP_SYSPLL3_D2, "syspll3_d2", "syspll_d5", 1, 2),
	FACTOR(CLK_TOP_SYSPLL3_D4, "syspll3_d4", "syspll_d5", 1, 4),
	FACTOR(CLK_TOP_SYSPLL4_D2, "syspll4_d2", "syspll_d7", 1, 2),
	FACTOR(CLK_TOP_SYSPLL4_D4, "syspll4_d4", "syspll_d7", 1, 4),

	FACTOR(CLK_TOP_UNIVPLL, "univpll_ck", "univpll", 1, 1),
	FACTOR(CLK_TOP_UNIVPLL_D2, "univpll_d2", "univpll", 1, 2),
	FACTOR(CLK_TOP_UNIVPLL_D3, "univpll_d3", "univpll", 1, 3),
	FACTOR(CLK_TOP_UNIVPLL_D5, "univpll_d5", "univpll", 1, 5),
	FACTOR(CLK_TOP_UNIVPLL_D7, "univpll_d7", "univpll", 1, 7),
	FACTOR(CLK_TOP_UNIVPLL_D26, "univpll_d26", "univpll", 1, 26),
	FACTOR(CLK_TOP_UNIVPLL_D52, "univpll_d52", "univpll", 1, 52),
	FACTOR(CLK_TOP_UNIVPLL_D108, "univpll_d108", "univpll", 1, 108),
	FACTOR(CLK_TOP_USB_PHY48M, "usb_phy48m_ck", "univpll", 1, 26),
	FACTOR(CLK_TOP_UNIVPLL1_D2, "univpll1_d2", "univpll_d2", 1, 2),
	FACTOR(CLK_TOP_UNIVPLL1_D4, "univpll1_d4", "univpll_d2", 1, 4),
	FACTOR(CLK_TOP_UNIVPLL1_D8, "univpll1_d8", "univpll_d2", 1, 8),
	FACTOR(CLK_TOP_8BDAC, "8bdac_ck", "univpll_d2", 1, 1),
	FACTOR(CLK_TOP_UNIVPLL2_D2, "univpll2_d2", "univpll_d3", 1, 2),
	FACTOR(CLK_TOP_UNIVPLL2_D4, "univpll2_d4", "univpll_d3", 1, 4),
	FACTOR(CLK_TOP_UNIVPLL2_D8, "univpll2_d8", "univpll_d3", 1, 8),
	FACTOR(CLK_TOP_UNIVPLL2_D16, "univpll2_d16", "univpll_d3", 1, 16),
	FACTOR(CLK_TOP_UNIVPLL2_D32, "univpll2_d32", "univpll_d3", 1, 32),
	FACTOR(CLK_TOP_UNIVPLL3_D2, "univpll3_d2", "univpll_d5", 1, 2),
	FACTOR(CLK_TOP_UNIVPLL3_D4, "univpll3_d4", "univpll_d5", 1, 4),
	FACTOR(CLK_TOP_UNIVPLL3_D8, "univpll3_d8", "univpll_d5", 1, 8),

	FACTOR(CLK_TOP_MSDCPLL, "msdcpll_ck", "msdcpll", 1, 1),
	FACTOR(CLK_TOP_MSDCPLL_D2, "msdcpll_d2", "msdcpll", 1, 2),
	FACTOR(CLK_TOP_MSDCPLL_D4, "msdcpll_d4", "msdcpll", 1, 4),
	FACTOR(CLK_TOP_MSDCPLL_D8, "msdcpll_d8", "msdcpll", 1, 8),

	FACTOR(CLK_TOP_MMPLL, "mmpll_ck", "mmpll", 1, 1),
	FACTOR(CLK_TOP_MMPLL_D2, "mmpll_d2", "mmpll", 1, 2),

	FACTOR(CLK_TOP_DMPLL_D2, "dmpll_d2", "dmpll_ck", 1, 2),
	FACTOR(CLK_TOP_DMPLL_D4, "dmpll_d4", "dmpll_ck", 1, 4),
	FACTOR(CLK_TOP_DMPLL_X2, "dmpll_x2", "dmpll_ck", 1, 1),

	FACTOR(CLK_TOP_TVDPLL, "tvdpll_ck", "tvdpll", 1, 1),
	FACTOR(CLK_TOP_TVDPLL_D2, "tvdpll_d2", "tvdpll", 1, 2),
	FACTOR(CLK_TOP_TVDPLL_D4, "tvdpll_d4", "tvdpll", 1, 4),

	FACTOR(CLK_TOP_VDECPLL, "vdecpll_ck", "vdecpll", 1, 1),
	FACTOR(CLK_TOP_TVD2PLL, "tvd2pll_ck", "tvd2pll", 1, 1),
	FACTOR(CLK_TOP_TVD2PLL_D2, "tvd2pll_d2", "tvd2pll", 1, 2),

	FACTOR(CLK_TOP_MIPIPLL, "mipipll", "dpi_ck", 1, 1),
	FACTOR(CLK_TOP_MIPIPLL_D2, "mipipll_d2", "dpi_ck", 1, 2),
	FACTOR(CLK_TOP_MIPIPLL_D4, "mipipll_d4", "dpi_ck", 1, 4),

	FACTOR(CLK_TOP_HDMIPLL, "hdmipll_ck", "hdmitx_dig_cts", 1, 1),
	FACTOR(CLK_TOP_HDMIPLL_D2, "hdmipll_d2", "hdmitx_dig_cts", 1, 2),
	FACTOR(CLK_TOP_HDMIPLL_D3, "hdmipll_d3", "hdmitx_dig_cts", 1, 3),

	FACTOR(CLK_TOP_ARMPLL_1P3G, "armpll_1p3g_ck", "armpll", 1, 1),

	FACTOR(CLK_TOP_AUDPLL, "audpll", "audpll_sel", 1, 1),
	FACTOR(CLK_TOP_AUDPLL_D4, "audpll_d4", "audpll_sel", 1, 4),
	FACTOR(CLK_TOP_AUDPLL_D8, "audpll_d8", "audpll_sel", 1, 8),
	FACTOR(CLK_TOP_AUDPLL_D16, "audpll_d16", "audpll_sel", 1, 16),
	FACTOR(CLK_TOP_AUDPLL_D24, "audpll_d24", "audpll_sel", 1, 24),

	FACTOR(CLK_TOP_AUD1PLL_98M, "aud1pll_98m_ck", "aud1pll", 1, 3),
	FACTOR(CLK_TOP_AUD2PLL_90M, "aud2pll_90m_ck", "aud2pll", 1, 3),
	FACTOR(CLK_TOP_HADDS2PLL_98M, "hadds2pll_98m", "hadds2pll", 1, 3),
	FACTOR(CLK_TOP_HADDS2PLL_294M, "hadds2pll_294m", "hadds2pll", 1, 1),
	FACTOR(CLK_TOP_ETHPLL_500M, "ethpll_500m_ck", "ethpll", 1, 1),
	FACTOR(CLK_TOP_CLK26M_D8, "clk26m_d8", "clk26m", 1, 8),
	FACTOR(CLK_TOP_32K_INTERNAL, "32k_internal", "clk26m", 1, 793),
	FACTOR(CLK_TOP_32K_EXTERNAL, "32k_external", "rtc32k", 1, 1),
	FACTOR(CLK_TOP_AXISEL_D4, "axisel_d4", "axi_sel", 1, 4),
};

static const char * const axi_parents[] __initconst = {
	"clk26m",
	"syspll1_d2",
	"syspll_d5",
	"syspll1_d4",
	"univpll_d5",
	"univpll2_d2",
	"mmpll_d2",
	"dmpll_d2"
};

static const char * const mem_parents[] __initconst = {
	"clk26m",
	"dmpll_ck"
};

static const char * const ddrphycfg_parents[] __initconst = {
	"clk26m",
	"syspll1_d8"
};

static const char * const mm_parents[] __initconst = {
	"clk26m",
	"vencpll_ck",
	"syspll1_d2",
	"syspll1_d4",
	"univpll_d5",
	"univpll1_d2",
	"univpll2_d2",
	"dmpll_ck"
};

static const char * const pwm_parents[] __initconst = {
	"clk26m",
	"univpll2_d4",
	"univpll3_d2",
	"univpll1_d4",
};

static const char * const vdec_parents[] __initconst = {
	"clk26m",
	"vdecpll_ck",
	"syspll_d5",
	"syspll1_d4",
	"univpll_d5",
	"univpll2_d2",
	"vencpll_ck",
	"msdcpll_d2",
	"mmpll_d2"
};

static const char * const mfg_parents[] __initconst = {
	"clk26m",
	"mmpll_ck",
	"dmpll_x2_ck",
	"msdcpll_ck",
	"clk26m",
	"syspll_d3",
	"univpll_d3",
	"univpll1_d2"
};

static const char * const camtg_parents[] __initconst = {
	"clk26m",
	"univpll_d26",
	"univpll2_d2",
	"syspll3_d2",
	"syspll3_d4",
	"msdcpll_d2",
	"mmpll_d2"
};

static const char * const uart_parents[] __initconst = {
	"clk26m",
	"univpll2_d8"
};

static const char * const spi_parents[] __initconst = {
	"clk26m",
	"syspll3_d2",
	"syspll4_d2",
	"univpll2_d4",
	"univpll1_d8"
};

static const char * const usb20_parents[] __initconst = {
	"clk26m",
	"univpll1_d8",
	"univpll3_d4"
};

static const char * const msdc30_parents[] __initconst = {
	"clk26m",
	"msdcpll_d2",
	"syspll2_d2",
	"syspll1_d4",
	"univpll1_d4",
	"univpll2_d4"
};

static const char * const audio_parents[] __initconst = {
	"clk26m",
	"syspll1_d16"
};

static const char * const aud_intbus_parents[] __initconst = {
	"clk26m",
	"syspll1_d4",
	"syspll3_d2",
	"syspll4_d2",
	"univpll3_d2",
	"univpll2_d4"
};

static const char * const pmicspi_parents[] __initconst = {
	"clk26m",
	"syspll1_d8",
	"syspll2_d4",
	"syspll4_d2",
	"syspll3_d4",
	"syspll2_d8",
	"syspll1_d16",
	"univpll3_d4",
	"univpll_d26",
	"dmpll_d2",
	"dmpll_d4"
};

static const char * const scp_parents[] __initconst = {
	"clk26m",
	"syspll1_d8",
	"dmpll_d2",
	"dmpll_d4"
};

static const char * const dpi0_parents[] __initconst = {
	"clk26m",
	"mipipll",
	"mipipll_d2",
	"mipipll_d4",
	"clk26m",
	"tvdpll_ck",
	"tvdpll_d2",
	"tvdpll_d4"
};

static const char * const dpi1_parents[] __initconst = {
	"clk26m",
	"tvdpll_ck",
	"tvdpll_d2",
	"tvdpll_d4"
};

static const char * const tve_parents[] __initconst = {
	"clk26m",
	"mipipll",
	"mipipll_d2",
	"mipipll_d4",
	"clk26m",
	"tvdpll_ck",
	"tvdpll_d2",
	"tvdpll_d4"
};

static const char * const hdmi_parents[] __initconst = {
	"clk26m",
	"hdmitx_dig_cts",
	"hdmipll_ck",
	"hdmipll_d2",
	"hdmipll_d3"
};

static const char * const apll_parents[] __initconst = {
	"clk26m",
	"audpll",
	"audpll_d4",
	"audpll_d8",
	"audpll_d16",
	"audpll_d24",
	"clk26m",
	"clk26m"
};

static const char * const rtc_parents[] __initconst = {
	"32k_internal",
	"32k_external",
	"clk26m",
	"univpll3_d8"
};

static const char * const nfi2x_parents[] __initconst = {
	"clk26m",
	"syspll2_d2",
	"syspll_d7",
	"univpll3_d2",
	"syspll2_d4",
	"univpll3_d4",
	"syspll4_d4",
	"clk26m"
};

static const char * const emmc_hclk_parents[] __initconst = {
	"clk26m",
	"syspll1_d2",
	"syspll1_d4",
	"syspll2_d2"
};

static const char * const flash_parents[] __initconst = {
	"clk26m_d8",
	"clk26m",
	"syspll2_d8",
	"syspll3_d4",
	"univpll3_d4",
	"syspll4_d2",
	"syspll2_d4",
	"univpll2_d4"
};

static const char * const di_parents[] __initconst = {
	"clk26m",
	"tvd2pll_ck",
	"tvd2pll_d2",
	"clk26m"
};

static const char * const nr_osd_parents[] __initconst = {
	"clk26m",
	"vencpll_ck",
	"syspll1_d2",
	"syspll1_d4",
	"univpll_d5",
	"univpll1_d2",
	"univpll2_d2",
	"dmpll_ck"
};

static const char * const hdmirx_bist_parents[] __initconst = {
	"clk26m",
	"syspll_d3",
	"clk26m",
	"syspll1_d16",
	"syspll4_d2",
	"syspll1_d4",
	"vencpll_ck",
	"clk26m"
};

static const char * const intdir_parents[] __initconst = {
	"clk26m",
	"mmpll_ck",
	"syspll_d2",
	"univpll_d2"
};

static const char * const asm_parents[] __initconst = {
	"clk26m",
	"univpll2_d4",
	"univpll2_d2",
	"syspll_d5"
};

static const char * const ms_card_parents[] __initconst = {
	"clk26m",
	"univpll3_d8",
	"syspll4_d4"
};

static const char * const ethif_parents[] __initconst = {
	"clk26m",
	"syspll1_d2",
	"syspll_d5",
	"syspll1_d4",
	"univpll_d5",
	"univpll1_d2",
	"dmpll_ck",
	"dmpll_d2"
};

static const char * const hdmirx_parents[] __initconst = {
	"clk26m",
	"univpll_d52"
};

static const char * const cmsys_parents[] __initconst = {
	"clk26m",
	"syspll1_d2",
	"univpll1_d2",
	"univpll_d5",
	"syspll_d5",
	"syspll2_d2",
	"syspll1_d4",
	"syspll3_d2",
	"syspll2_d4",
	"syspll1_d8",
	"clk26m",
	"clk26m",
	"clk26m",
	"clk26m",
	"clk26m"
};

static const char * const clk_8bdac_parents[] __initconst = {
	"32k_internal",
	"8bdac_ck",
	"clk26m",
	"clk26m"
};

static const char * const aud2dvd_parents[] __initconst = {
	"a1sys_hp_ck",
	"a2sys_hp_ck"
};

static const char * const padmclk_parents[] __initconst = {
	"clk26m",
	"univpll_d26",
	"univpll_d52",
	"univpll_d108",
	"univpll2_d8",
	"univpll2_d16",
	"univpll2_d32"
};

static const char * const aud_mux_parents[] __initconst = {
	"clk26m",
	"aud1pll_98m_ck",
	"aud2pll_90m_ck",
	"hadds2pll_98m",
	"audio_ext1_ck",
	"audio_ext2_ck"
};

static const char * const aud_src_parents[] __initconst = {
	"aud_mux1_sel",
	"aud_mux2_sel"
};

static const char * const cpu_parents[] __initconst = {
	"clk26m",
	"armpll",
	"mainpll",
	"mmpll"
};

static const struct mtk_composite cpu_muxes[] __initconst = {
	MUX(CLK_INFRA_CPUSEL, "infra_cpu_sel", cpu_parents, 0x0000, 2, 2),
};

static const struct mtk_composite top_muxes[] __initconst = {
	MUX_GATE(CLK_TOP_AXI_SEL, "axi_sel", axi_parents,
		0x0040, 0, 3, INVALID_MUX_GATE_BIT),
	MUX_GATE(CLK_TOP_MEM_SEL, "mem_sel", mem_parents,
		0x0040, 8, 1, 15),
	MUX_GATE(CLK_TOP_DDRPHYCFG_SEL, "ddrphycfg_sel", ddrphycfg_parents,
		0x0040, 16, 1, 23),
	MUX_GATE(CLK_TOP_MM_SEL, "mm_sel", mm_parents,
		0x0040, 24, 3, 31),

	MUX_GATE(CLK_TOP_PWM_SEL, "pwm_sel", pwm_parents,
		0x0050, 0, 2, 7),
	MUX_GATE(CLK_TOP_VDEC_SEL, "vdec_sel", vdec_parents,
		0x0050, 8, 4, 15),
	MUX_GATE(CLK_TOP_MFG_SEL, "mfg_sel", mfg_parents,
		0x0050, 16, 3, 23),
	MUX_GATE(CLK_TOP_CAMTG_SEL, "camtg_sel", camtg_parents,
		0x0050, 24, 3, 31),
	MUX_GATE(CLK_TOP_UART_SEL, "uart_sel", uart_parents,
		0x0060, 0, 1, 7),

	MUX_GATE(CLK_TOP_SPI0_SEL, "spi0_sel", spi_parents,
		0x0060, 8, 3, 15),
	MUX_GATE(CLK_TOP_USB20_SEL, "usb20_sel", usb20_parents,
		0x0060, 16, 2, 23),
	MUX_GATE(CLK_TOP_MSDC30_0_SEL, "msdc30_0_sel", msdc30_parents,
		0x0060, 24, 3, 31),

	MUX_GATE(CLK_TOP_MSDC30_1_SEL, "msdc30_1_sel", msdc30_parents,
		0x0070, 0, 3, 7),
	MUX_GATE(CLK_TOP_MSDC30_2_SEL, "msdc30_2_sel", msdc30_parents,
		0x0070, 8, 3, 15),
	MUX_GATE(CLK_TOP_AUDIO_SEL, "audio_sel", msdc30_parents,
		0x0070, 16, 1, 23),
	MUX_GATE(CLK_TOP_AUDINTBUS_SEL, "aud_intbus_sel", aud_intbus_parents,
		0x0070, 24, 3, 31),

	MUX_GATE(CLK_TOP_PMICSPI_SEL, "pmicspi_sel", pmicspi_parents,
		0x0080, 0, 4, 7),
	MUX_GATE(CLK_TOP_SCP_SEL, "scp_sel", scp_parents,
		0x0080, 8, 2, 15),
	MUX_GATE(CLK_TOP_DPI0_SEL, "dpi0_sel", dpi0_parents,
		0x0080, 16, 3, 23),
	MUX_GATE(CLK_TOP_DPI1_SEL, "dpi1_sel", dpi1_parents,
		0x0080, 24, 2, 31),

	MUX_GATE(CLK_TOP_TVE_SEL, "tve_sel", tve_parents,
		0x0090, 0, 3, 7),
	MUX_GATE(CLK_TOP_HDMI_SEL, "hdmi_sel", hdmi_parents,
		0x0090, 8, 2, 15),
	MUX_GATE(CLK_TOP_APLL_SEL, "apll_sel", apll_parents,
		0x0090, 16, 3, 23),

	MUX_GATE(CLK_TOP_RTC_SEL, "rtc_sel", rtc_parents,
		0x00A0, 0, 2, 7),
	MUX_GATE(CLK_TOP_NFI2X_SEL, "nfi2x_sel", nfi2x_parents,
		0x00A0, 8, 3, 15),
	MUX_GATE(CLK_TOP_EMMC_HCLK_SEL, "emmc_hclk_sel", emmc_hclk_parents,
		0x00A0, 24, 2, 31),

	MUX_GATE(CLK_TOP_FLASH_SEL, "flash_sel", flash_parents,
		0x00B0, 0, 3, 7),
	MUX_GATE(CLK_TOP_DI_SEL, "di_sel", di_parents,
		0x00B0, 8, 2, 15),
	MUX_GATE(CLK_TOP_NR_SEL, "nr_sel", nr_osd_parents,
		0x00B0, 16, 3, 23),
	MUX_GATE(CLK_TOP_OSD_SEL, "osd_sel", nr_osd_parents,
		0x00B0, 24, 3, 31),

	MUX_GATE(CLK_TOP_HDMIRX_BIST_SEL, "hdmirx_bist_sel",
		hdmirx_bist_parents, 0x00C0, 0, 3, 7),
	MUX_GATE(CLK_TOP_INTDIR_SEL, "intdir_sel", intdir_parents,
		0x00C0, 8, 2, 15),
	MUX_GATE(CLK_TOP_ASM_I_SEL, "asm_i_sel", asm_parents,
		0x00C0, 16, 2, 23),
	MUX_GATE(CLK_TOP_ASM_M_SEL, "asm_m_sel", asm_parents,
		0x00C0, 24, 3, 31),

	MUX_GATE(CLK_TOP_ASM_H_SEL, "asm_h_sel", asm_parents,
		0x00D0, 0, 2, 7),
	MUX_GATE(CLK_TOP_MS_CARD_SEL, "ms_card_sel", ms_card_parents,
		0x00D0, 16, 2, 23),
	MUX_GATE(CLK_TOP_ETHIF_SEL, "ethif_sel", ethif_parents,
		0x00D0, 24, 3, 31),

	MUX_GATE(CLK_TOP_HDMIRX26_24_SEL, "hdmirx26_24_sel", hdmirx_parents,
		0x00E0, 0, 1, 7),
	MUX_GATE(CLK_TOP_MSDC30_3_SEL, "msdc30_3_sel", msdc30_parents,
		0x00E0, 8, 3, 15),
	MUX_GATE(CLK_TOP_CMSYS_SEL, "cmsys_sel", cmsys_parents,
		0x00E0, 16, 4, 23),

	MUX_GATE(CLK_TOP_SPI1_SEL, "spi2_sel", spi_parents,
		0x00E0, 24, 3, 31),
	MUX_GATE(CLK_TOP_SPI2_SEL, "spi1_sel", spi_parents,
		0x00F0, 0, 3, 7),
	MUX_GATE(CLK_TOP_8BDAC_SEL, "8bdac_sel", clk_8bdac_parents,
		0x00F0, 8, 2, 15),
	MUX_GATE(CLK_TOP_AUD2DVD_SEL, "aud2dvd_sel", aud2dvd_parents,
		0x00F0, 16, 1, 23),

	MUX(CLK_TOP_PADMCLK_SEL, "padmclk_sel", padmclk_parents,
		0x0100, 0, 3),

	MUX(CLK_TOP_AUD_MUX1_SEL, "aud_mux1_sel", aud_mux_parents,
		0x012c, 0, 3),
	MUX(CLK_TOP_AUD_MUX2_SEL, "aud_mux2_sel", aud_mux_parents,
		0x012c, 3, 3),
	MUX(CLK_TOP_AUDPLL_MUX_SEL, "audpll_sel", aud_mux_parents,
		0x012c, 6, 3),
	MUX_GATE(CLK_TOP_AUD_K1_SRC_SEL, "aud_k1_src_sel", aud_src_parents,
		0x012c, 15, 1, 23),
	MUX_GATE(CLK_TOP_AUD_K2_SRC_SEL, "aud_k2_src_sel", aud_src_parents,
		0x012c, 16, 1, 24),
	MUX_GATE(CLK_TOP_AUD_K3_SRC_SEL, "aud_k3_src_sel", aud_src_parents,
		0x012c, 17, 1, 25),
	MUX_GATE(CLK_TOP_AUD_K4_SRC_SEL, "aud_k4_src_sel", aud_src_parents,
		0x012c, 18, 1, 26),
	MUX_GATE(CLK_TOP_AUD_K5_SRC_SEL, "aud_k5_src_sel", aud_src_parents,
		0x012c, 19, 1, 27),
	MUX_GATE(CLK_TOP_AUD_K6_SRC_SEL, "aud_k6_src_sel", aud_src_parents,
		0x012c, 20, 1, 28),
};

static const struct mtk_clk_divider top_adj_divs[] __initconst = {
	DIV_ADJ(CLK_TOP_AUD_EXTCK1_DIV, "audio_ext1_ck", "aud_ext1",
		0x0120, 0, 8),
	DIV_ADJ(CLK_TOP_AUD_EXTCK2_DIV, "audio_ext2_ck", "aud_ext2",
		0x0120, 8, 8),
	DIV_ADJ(CLK_TOP_AUD_MUX1_DIV, "aud_mux1_div", "aud_mux1_sel",
		0x0120, 16, 8),
	DIV_ADJ(CLK_TOP_AUD_MUX2_DIV, "aud_mux2_div", "aud_mux2_sel",
		0x0120, 24, 8),
	DIV_ADJ(CLK_TOP_AUD_K1_SRC_DIV, "aud_k1_src_div", "aud_k1_src_sel",
		0x0124, 0, 8),
	DIV_ADJ(CLK_TOP_AUD_K2_SRC_DIV, "aud_k2_src_div", "aud_k2_src_sel",
		0x0124, 8, 8),
	DIV_ADJ(CLK_TOP_AUD_K3_SRC_DIV, "aud_k3_src_div", "aud_k3_src_sel",
		0x0124, 16, 8),
	DIV_ADJ(CLK_TOP_AUD_K4_SRC_DIV, "aud_k4_src_div", "aud_k4_src_sel",
		0x0124, 24, 8),
	DIV_ADJ(CLK_TOP_AUD_K5_SRC_DIV, "aud_k5_src_div", "aud_k5_src_sel",
		0x0128, 0, 8),
	DIV_ADJ(CLK_TOP_AUD_K6_SRC_DIV, "aud_k6_src_div", "aud_k6_src_sel",
		0x0128, 8, 8),
};

static const struct mtk_gate_regs top_aud_cg_regs __initconst = {
	.sta_ofs = 0x012C,
};

#define GATE_TOP_AUD(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &top_aud_cg_regs,		\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_no_setclr,	\
	}

static const struct mtk_gate top_clks[] __initconst = {
	GATE_TOP_AUD(CLK_TOP_AUD_48K_TIMING, "a1sys_hp_ck", "aud_mux1_div",
		21),
	GATE_TOP_AUD(CLK_TOP_AUD_44K_TIMING, "a2sys_hp_ck", "aud_mux2_div",
		22),
	GATE_TOP_AUD(CLK_TOP_AUD_I2S1_MCLK, "aud_i2s1_mclk", "aud_k1_src_div",
		23),
	GATE_TOP_AUD(CLK_TOP_AUD_I2S2_MCLK, "aud_i2s2_mclk", "aud_k2_src_div",
		24),
	GATE_TOP_AUD(CLK_TOP_AUD_I2S3_MCLK, "aud_i2s3_mclk", "aud_k3_src_div",
		25),
	GATE_TOP_AUD(CLK_TOP_AUD_I2S4_MCLK, "aud_i2s4_mclk", "aud_k4_src_div",
		26),
	GATE_TOP_AUD(CLK_TOP_AUD_I2S5_MCLK, "aud_i2s5_mclk", "aud_k5_src_div",
		27),
	GATE_TOP_AUD(CLK_TOP_AUD_I2S6_MCLK, "aud_i2s6_mclk", "aud_k6_src_div",
		28),
};

static struct clk_onecell_data *top_clk_data __initdata;
static struct clk_onecell_data *pll_clk_data __initdata;

static void __init mtk_clk_enable_critical(void)
{
	if (!top_clk_data || !pll_clk_data)
		return;

	clk_prepare_enable(pll_clk_data->clks[CLK_APMIXED_ARMPLL]);
	clk_prepare_enable(top_clk_data->clks[CLK_TOP_AXI_SEL]);
	clk_prepare_enable(top_clk_data->clks[CLK_TOP_MEM_SEL]);
	clk_prepare_enable(top_clk_data->clks[CLK_TOP_DDRPHYCFG_SEL]);
	clk_prepare_enable(top_clk_data->clks[CLK_TOP_RTC_SEL]);
}

static void __init mtk_topckgen_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	void __iomem *base;
	int r;

	base = of_iomap(node, 0);
	if (!base) {
		pr_err("%s(): ioremap failed\n", __func__);
		return;
	}

	top_clk_data = clk_data = mtk_alloc_clk_data(CLK_TOP_NR);

	mtk_clk_register_fixed_clks(top_fixed_clks, ARRAY_SIZE(top_fixed_clks),
								clk_data);

	mtk_clk_register_factors(top_fixed_divs, ARRAY_SIZE(top_fixed_divs),
								clk_data);

	mtk_clk_register_composites(top_muxes, ARRAY_SIZE(top_muxes),
				base, &lock, clk_data);

	mtk_clk_register_dividers(top_adj_divs, ARRAY_SIZE(top_adj_divs),
				base, &lock, clk_data);

	mtk_clk_register_gates(node, top_clks, ARRAY_SIZE(top_clks),
						clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);

	mtk_clk_enable_critical();
}
CLK_OF_DECLARE(mtk_topckgen, "mediatek,mt2701-topckgen", mtk_topckgen_init);

static const struct mtk_gate_regs infra_cg_regs __initconst = {
	.set_ofs = 0x0040,
	.clr_ofs = 0x0044,
	.sta_ofs = 0x0048,
};

#define GATE_ICG(_id, _name, _parent, _shift) {		\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &infra_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr,	\
	}

static const struct mtk_gate infra_clks[] __initconst = {
	GATE_ICG(CLK_INFRA_DBG, "dbgclk", "axi_sel", 0),
	GATE_ICG(CLK_INFRA_SMI, "smi_ck", "mm_sel", 1),
	GATE_ICG(CLK_INFRA_QAXI_CM4, "cm4_ck", "axi_sel", 2),
	GATE_ICG(CLK_INFRA_AUD_SPLIN_B, "audio_splin_bck", "hadds2pll_294m", 4),
	GATE_ICG(CLK_INFRA_AUDIO, "audio_ck", "clk26m", 5),
	GATE_ICG(CLK_INFRA_EFUSE, "efuse_ck", "clk26m", 6),
	GATE_ICG(CLK_INFRA_L2C_SRAM, "l2c_sram_ck", "mm_sel", 7),
	GATE_ICG(CLK_INFRA_M4U, "m4u_ck", "mem_sel", 8),
	GATE_ICG(CLK_INFRA_CONNMCU, "connsys_bus", "wbg_dig_ck_416m", 12),
	GATE_ICG(CLK_INFRA_TRNG, "trng_ck", "axi_sel", 13),
	GATE_ICG(CLK_INFRA_RAMBUFIF, "rambufif_ck", "mem_sel", 14),
	GATE_ICG(CLK_INFRA_CPUM, "cpum_ck", "mem_sel", 15),
	GATE_ICG(CLK_INFRA_KP, "kp_ck", "axi_sel", 16),
	GATE_ICG(CLK_INFRA_CEC, "cec_ck", "rtc_sel", 18),
	GATE_ICG(CLK_INFRA_IRRX, "irrx_ck", "axi_sel", 19),
	GATE_ICG(CLK_INFRA_PMICSPI, "pmicspi_ck", "pmicspi_sel", 22),
	GATE_ICG(CLK_INFRA_PMICWRAP, "pmicwrap_ck", "axi_sel", 23),
	GATE_ICG(CLK_INFRA_DDCCI, "ddcci_ck", "axi_sel", 24),
};

static const struct mtk_fixed_factor infra_fixed_divs[] __initconst = {
	FACTOR(CLK_INFRA_CLK_13M, "clk13m", "clk26m", 1, 2),
};

static void __init mtk_infrasys_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	int r;

	clk_data = mtk_alloc_clk_data(CLK_INFRA_NR);

	mtk_clk_register_gates(node, infra_clks, ARRAY_SIZE(infra_clks),
						clk_data);
	mtk_clk_register_factors(infra_fixed_divs, ARRAY_SIZE(infra_fixed_divs),
						clk_data);

	mtk_clk_register_cpumuxes(node, cpu_muxes, ARRAY_SIZE(cpu_muxes),
				  clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);

	mtk_register_reset_controller(node, 2, 0x30);
}
CLK_OF_DECLARE(mtk_infrasys, "mediatek,mt2701-infracfg", mtk_infrasys_init);

static const struct mtk_gate_regs peri0_cg_regs __initconst = {
	.set_ofs = 0x0008,
	.clr_ofs = 0x0010,
	.sta_ofs = 0x0018,
};

static const struct mtk_gate_regs peri1_cg_regs __initconst = {
	.set_ofs = 0x000c,
	.clr_ofs = 0x0014,
	.sta_ofs = 0x001c,
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
	GATE_PERI0(CLK_PERI_USB0_MCU, "usb0_mcu_ck", "axi_sel", 31),
	GATE_PERI0(CLK_PERI_ETH, "eth_ck", "clk26m", 30),
	GATE_PERI0(CLK_PERI_SPI0, "spi0_ck", "spi0_sel", 29),
	GATE_PERI0(CLK_PERI_AUXADC, "auxadc_ck", "clk26m", 28),
	GATE_PERI0(CLK_PERI_I2C3, "i2c3_ck", "clk26m", 27),
	GATE_PERI0(CLK_PERI_I2C2, "i2c2_ck", "axi_sel", 26),
	GATE_PERI0(CLK_PERI_I2C1, "i2c1_ck", "axi_sel", 25),
	GATE_PERI0(CLK_PERI_I2C0, "i2c0_ck", "axi_sel", 24),
	GATE_PERI0(CLK_PERI_BTIF, "bitif_ck", "axi_sel", 23),
	GATE_PERI0(CLK_PERI_UART3, "uart3_ck", "axi_sel", 22),
	GATE_PERI0(CLK_PERI_UART2, "uart2_ck", "axi_sel", 21),
	GATE_PERI0(CLK_PERI_UART1, "uart1_ck", "axi_sel", 20),
	GATE_PERI0(CLK_PERI_UART0, "uart0_ck", "axi_sel", 19),
	GATE_PERI0(CLK_PERI_NLI, "nli_ck", "axi_sel", 18),
	GATE_PERI0(CLK_PERI_MSDC50_3, "msdc50_3_ck", "emmc_hclk_sel", 17),
	GATE_PERI0(CLK_PERI_MSDC30_3, "msdc30_3_ck", "msdc30_3_sel", 16),
	GATE_PERI0(CLK_PERI_MSDC30_2, "msdc30_2_ck", "msdc30_2_sel", 15),
	GATE_PERI0(CLK_PERI_MSDC30_1, "msdc30_1_ck", "msdc30_1_sel", 14),
	GATE_PERI0(CLK_PERI_MSDC30_0, "msdc30_0_ck", "msdc30_0_sel", 13),
	GATE_PERI0(CLK_PERI_AP_DMA, "ap_dma_ck", "axi_sel", 12),
	GATE_PERI0(CLK_PERI_USB1, "usb1_ck", "usb20_sel", 11),
	GATE_PERI0(CLK_PERI_USB0, "usb0_ck", "usb20_sel", 10),
	GATE_PERI0(CLK_PERI_PWM, "pwm_ck", "axi_sel", 9),
	GATE_PERI0(CLK_PERI_PWM7, "pwm7_ck", "axisel_d4", 8),
	GATE_PERI0(CLK_PERI_PWM6, "pwm6_ck", "axisel_d4", 7),
	GATE_PERI0(CLK_PERI_PWM5, "pwm5_ck", "axisel_d4", 6),
	GATE_PERI0(CLK_PERI_PWM4, "pwm4_ck", "axisel_d4", 5),
	GATE_PERI0(CLK_PERI_PWM3, "pwm3_ck", "axisel_d4", 4),
	GATE_PERI0(CLK_PERI_PWM2, "pwm2_ck", "axisel_d4", 3),
	GATE_PERI0(CLK_PERI_PWM1, "pwm1_ck", "axisel_d4", 2),
	GATE_PERI0(CLK_PERI_THERM, "therm_ck", "axi_sel", 1),
	GATE_PERI0(CLK_PERI_NFI, "nfi_ck", "nfi2x_sel", 0),

	GATE_PERI1(CLK_PERI_FCI, "fci_ck", "ms_card_sel", 11),
	GATE_PERI1(CLK_PERI_SPI2, "spi2_ck", "spi2_sel", 10),
	GATE_PERI1(CLK_PERI_SPI1, "spi1_ck", "spi1_sel", 9),
	GATE_PERI1(CLK_PERI_HOST89_DVD, "host89_dvd_ck", "aud2dvd_sel", 8),
	GATE_PERI1(CLK_PERI_HOST89_SPI, "host89_spi_ck", "spi0_sel", 7),
	GATE_PERI1(CLK_PERI_HOST89_INT, "host89_int_ck", "axi_sel", 6),
	GATE_PERI1(CLK_PERI_FLASH, "flash_ck", "nfi2x_sel", 5),
	GATE_PERI1(CLK_PERI_NFI_PAD, "nfi_pad_ck", "nfi1x_pad", 4),
	GATE_PERI1(CLK_PERI_NFI_ECC, "nfi_ecc_ck", "nfi1x_pad", 3),
	GATE_PERI1(CLK_PERI_GCPU, "gcpu_ck", "axi_sel", 2),
	GATE_PERI1(CLK_PERI_USB_SLV, "usbslv_ck", "axi_sel", 1),
	GATE_PERI1(CLK_PERI_USB1_MCU, "usb1_mcu_ck", "axi_sel", 0),
};

static const char * const uart_ck_sel_parents[] __initconst = {
	"clk26m",
	"uart_sel",
};

static const struct mtk_composite peri_muxs[] __initconst = {
	MUX(CLK_PERI_UART0_SEL, "uart0_ck_sel", uart_ck_sel_parents,
		0x40c, 0, 1),
	MUX(CLK_PERI_UART1_SEL, "uart1_ck_sel", uart_ck_sel_parents,
		0x40c, 1, 1),
	MUX(CLK_PERI_UART2_SEL, "uart2_ck_sel", uart_ck_sel_parents,
		0x40c, 2, 1),
	MUX(CLK_PERI_UART3_SEL, "uart3_ck_sel", uart_ck_sel_parents,
		0x40c, 3, 1),
};

static void __init mtk_pericfg_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	void __iomem *base;
	int r;

	base = of_iomap(node, 0);
	if (!base) {
		pr_err("%s(): ioremap failed\n", __func__);
		return;
	}

	clk_data = mtk_alloc_clk_data(CLK_PERI_NR);

	mtk_clk_register_gates(node, peri_clks, ARRAY_SIZE(peri_clks),
						clk_data);

	mtk_clk_register_composites(peri_muxs, ARRAY_SIZE(peri_muxs), base,
			&lock, clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);

	mtk_register_reset_controller(node, 2, 0x0);
}
CLK_OF_DECLARE(mtk_pericfg, "mediatek,mt2701-pericfg", mtk_pericfg_init);

static const struct mtk_gate_regs disp0_cg_regs __initconst = {
	.set_ofs = 0x0104,
	.clr_ofs = 0x0108,
	.sta_ofs = 0x0100,
};

static const struct mtk_gate_regs disp1_cg_regs __initconst = {
	.set_ofs = 0x0114,
	.clr_ofs = 0x0118,
	.sta_ofs = 0x0110,
};

#define GATE_DISP0(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &disp0_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr,	\
	}

#define GATE_DISP1(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &disp1_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr,	\
	}

static const struct mtk_gate mm_clks[] __initconst = {
	GATE_DISP0(CLK_MM_SMI_COMMON, "mm_smi_comm", "mm_sel", 0),
	GATE_DISP0(CLK_MM_SMI_LARB0, "mm_smi_larb0", "mm_sel", 1),
	GATE_DISP0(CLK_MM_CMDQ, "mm_cmdq", "mm_sel", 2),
	GATE_DISP0(CLK_MM_MUTEX, "mm_mutex", "mm_sel", 3),
	GATE_DISP0(CLK_MM_DISP_COLOR, "mm_disp_color", "mm_sel", 4),
	GATE_DISP0(CLK_MM_DISP_BLS, "mm_disp_bls", "mm_sel", 5),
	GATE_DISP0(CLK_MM_DISP_WDMA, "mm_disp_wdma", "mm_sel", 6),
	GATE_DISP0(CLK_MM_DISP_RDMA, "mm_disp_rdma", "mm_sel", 7),
	GATE_DISP0(CLK_MM_DISP_OVL, "mm_disp_ovl", "mm_sel", 8),
	GATE_DISP0(CLK_MM_MDP_TDSHP, "mm_mdp_tdshp", "mm_sel", 9),
	GATE_DISP0(CLK_MM_MDP_WROT, "mm_mdp_wrot", "mm_sel", 10),
	GATE_DISP0(CLK_MM_MDP_WDMA, "mm_mdp_wdma", "mm_sel", 11),
	GATE_DISP0(CLK_MM_MDP_RSZ1, "mm_mdp_rsz1", "mm_sel", 12),
	GATE_DISP0(CLK_MM_MDP_RSZ0, "mm_mdp_rsz0", "mm_sel", 13),
	GATE_DISP0(CLK_MM_MDP_RDMA, "mm_mdp_rdma", "mm_sel", 14),
	GATE_DISP0(CLK_MM_MDP_BLS_26M, "mm_mdp_bls_26m", "pwm_sel", 15),
	GATE_DISP0(CLK_MM_CAM_MDP, "mm_cam_mdp", "mm_sel", 16),
	GATE_DISP0(CLK_MM_FAKE_ENG, "mm_fake_eng", "mm_sel", 17),
	GATE_DISP0(CLK_MM_MUTEX_32K, "mm_mutex_32k", "rtc_sel", 18),
	GATE_DISP0(CLK_MM_DISP_RDMA1, "mm_disp_rdma1", "mm_sel", 19),
	GATE_DISP0(CLK_MM_DISP_UFOE, "mm_disp_ufoe", "mm_sel", 20),
	GATE_DISP1(CLK_MM_DSI_ENGINE, "mm_dsi_eng", "mm_sel", 0),
	GATE_DISP1(CLK_MM_DSI_DIG, "mm_dsi_dig", "dsi0_lntc_dsi", 1),
	GATE_DISP1(CLK_MM_DPI_DIGL, "mm_dpi_digl", "dpi0_sel", 2),
	GATE_DISP1(CLK_MM_DPI_ENGINE, "mm_dpi_eng", "mm_sel", 3),
	GATE_DISP1(CLK_MM_DPI1_DIGL, "mm_dpi1_digl", "dpi1_sel", 4),
	GATE_DISP1(CLK_MM_DPI1_ENGINE, "mm_dpi1_eng", "mm_sel", 5),
	GATE_DISP1(CLK_MM_TVE_OUTPUT, "mm_tve_output", "tve_sel", 6),
	GATE_DISP1(CLK_MM_TVE_INPUT, "mm_tve_input", "dpi0_sel", 7),
	GATE_DISP1(CLK_MM_HDMI_PIXEL, "mm_hdmi_pixel", "dpi1_sel", 8),
	GATE_DISP1(CLK_MM_HDMI_PLL, "mm_hdmi_pll", "hdmi_sel", 9),
	GATE_DISP1(CLK_MM_HDMI_AUDIO, "mm_hdmi_audio", "apll_sel", 10),
	GATE_DISP1(CLK_MM_HDMI_SPDIF, "mm_hdmi_spdif", "apll_sel", 11),
	GATE_DISP1(CLK_MM_TVE_FMM, "mm_tve_fmm", "mm_sel", 14),
};

static void __init mtk_mmsys_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	int r;

	clk_data = mtk_alloc_clk_data(CLK_MM_NR);

	mtk_clk_register_gates(node, mm_clks, ARRAY_SIZE(mm_clks),
						clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);
}

static const struct mtk_gate_regs img_cg_regs __initconst = {
	.set_ofs = 0x0004,
	.clr_ofs = 0x0008,
	.sta_ofs = 0x0000,
};

#define GATE_IMG(_id, _name, _parent, _shift) {		\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &img_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr,	\
	}

static const struct mtk_gate img_clks[] __initconst = {
	GATE_IMG(CLK_IMG_SMI_COMM, "img_smi_comm", "mm_sel", 0),
	GATE_IMG(CLK_IMG_RESZ, "img_resz", "mm_sel", 1),
	GATE_IMG(CLK_IMG_JPGDEC_SMI, "img_jpgdec_smi", "mm_sel", 5),
	GATE_IMG(CLK_IMG_JPGDEC, "img_jpgdec", "mm_sel", 6),
	GATE_IMG(CLK_IMG_VENC_LT, "img_venc_lt", "mm_sel", 8),
	GATE_IMG(CLK_IMG_VENC, "img_venc", "mm_sel", 9),
};

static void __init mtk_imgsys_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	int r;

	clk_data = mtk_alloc_clk_data(CLK_IMG_NR);

	mtk_clk_register_gates(node, img_clks, ARRAY_SIZE(img_clks),
						clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);
}

static const struct mtk_gate_regs vdec0_cg_regs __initconst = {
	.set_ofs = 0x0000,
	.clr_ofs = 0x0004,
	.sta_ofs = 0x0000,
};

static const struct mtk_gate_regs vdec1_cg_regs __initconst = {
	.set_ofs = 0x0008,
	.clr_ofs = 0x000c,
	.sta_ofs = 0x0008,
};

#define GATE_VDEC0(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &vdec0_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr_inv,	\
	}

#define GATE_VDEC1(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &vdec1_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr_inv,	\
	}

static const struct mtk_gate vdec_clks[] __initconst = {
	GATE_VDEC0(CLK_VDEC_CKGEN, "vdec_cken", "vdec_sel", 0),
	GATE_VDEC1(CLK_VDEC_LARB, "vdec_larb_cken", "mm_sel", 0),
};

static void __init mtk_vdecsys_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	int r;

	clk_data = mtk_alloc_clk_data(CLK_VDEC_NR);

	mtk_clk_register_gates(node, vdec_clks, ARRAY_SIZE(vdec_clks),
						clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);
}

static const struct mtk_gate_regs hif_cg_regs __initconst = {
	.sta_ofs = 0x0030,
};

#define GATE_HIF(_id, _name, _parent, _shift) {		\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &hif_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_no_setclr_inv,	\
	}

static const struct mtk_gate hif_clks[] __initconst = {
	GATE_HIF(CLK_HIFSYS_USB0PHY, "usb0_phy_clk", "ethpll_500m_ck", 21),
	GATE_HIF(CLK_HIFSYS_USB1PHY, "usb1_phy_clk", "ethpll_500m_ck", 22),
	GATE_HIF(CLK_HIFSYS_PCIE0, "pcie0_clk", "ethpll_500m_ck", 24),
	GATE_HIF(CLK_HIFSYS_PCIE1, "pcie1_clk", "ethpll_500m_ck", 25),
	GATE_HIF(CLK_HIFSYS_PCIE2, "pcie2_clk", "ethpll_500m_ck", 26),
};

static void __init mtk_hifsys_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	int r;

	clk_data = mtk_alloc_clk_data(CLK_HIFSYS_NR);

	mtk_clk_register_gates(node, hif_clks, ARRAY_SIZE(hif_clks),
						clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);

	mtk_register_reset_controller(node, 1, 0x34);
}

static const struct mtk_gate_regs eth_cg_regs __initconst = {
	.sta_ofs = 0x0030,
};

#define GATE_ETH(_id, _name, _parent, _shift) {		\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &eth_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_no_setclr_inv,	\
	}

static const struct mtk_gate eth_clks[] __initconst = {
	GATE_ETH(CLK_ETHSYS_HSDMA, "hsdma_clk", "ethif_sel", 5),
	GATE_ETH(CLK_ETHSYS_ESW, "esw_clk", "ethpll_500m_ck", 6),
	GATE_ETH(CLK_ETHSYS_GP2, "gp2_clk", "trgpll", 7),
	GATE_ETH(CLK_ETHSYS_GP1, "gp1_clk", "ethpll_500m_ck", 8),
	GATE_ETH(CLK_ETHSYS_PCM, "pcm_clk", "ethif_sel", 11),
	GATE_ETH(CLK_ETHSYS_GDMA, "gdma_clk", "ethif_sel", 14),
	GATE_ETH(CLK_ETHSYS_I2S, "i2s_clk", "ethif_sel", 17),
	GATE_ETH(CLK_ETHSYS_CRYPTO, "crypto_clk", "ethif_sel", 29),
};

static void __init mtk_ethsys_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	int r;

	clk_data = mtk_alloc_clk_data(CLK_ETHSYS_NR);

	mtk_clk_register_gates(node, eth_clks, ARRAY_SIZE(eth_clks),
						clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);

	mtk_register_reset_controller(node, 1, 0x34);
}

static const struct mtk_gate_regs bdp0_cg_regs __initconst = {
	.set_ofs = 0x0104,
	.clr_ofs = 0x0108,
	.sta_ofs = 0x0100,
};

static const struct mtk_gate_regs bdp1_cg_regs __initconst = {
	.set_ofs = 0x0114,
	.clr_ofs = 0x0118,
	.sta_ofs = 0x0110,
};

#define GATE_BDP0(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &bdp0_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr_inv,	\
	}

#define GATE_BDP1(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &bdp1_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr_inv,	\
	}

static const struct mtk_gate bdp_clks[] __initconst = {
	GATE_BDP0(CLK_BDP_BRG_BA, "brg_baclk", "mm_sel", 0),
	GATE_BDP0(CLK_BDP_BRG_DRAM, "brg_dram", "mm_sel", 1),
	GATE_BDP0(CLK_BDP_LARB_DRAM, "larb_dram", "mm_sel", 2),
	GATE_BDP0(CLK_BDP_WR_VDI_PXL, "wr_vdi_pxl", "hdmi_0_deep340m", 3),
	GATE_BDP0(CLK_BDP_WR_VDI_DRAM, "wr_vdi_dram", "mm_sel", 4),
	GATE_BDP0(CLK_BDP_WR_B, "wr_bclk", "mm_sel", 5),
	GATE_BDP0(CLK_BDP_DGI_IN, "dgi_in", "dpi1_sel", 6),
	GATE_BDP0(CLK_BDP_DGI_OUT, "dgi_out", "dpi1_sel", 7),
	GATE_BDP0(CLK_BDP_FMT_MAST_27, "fmt_mast_27", "dpi1_sel", 8),
	GATE_BDP0(CLK_BDP_FMT_B, "fmt_bclk", "mm_sel", 9),
	GATE_BDP0(CLK_BDP_OSD_B, "osd_bclk", "mm_sel", 10),
	GATE_BDP0(CLK_BDP_OSD_DRAM, "osd_dram", "mm_sel", 11),
	GATE_BDP0(CLK_BDP_OSD_AGENT, "osd_agent", "osd_sel", 12),
	GATE_BDP0(CLK_BDP_OSD_PXL, "osd_pxl", "dpi1_sel", 13),
	GATE_BDP0(CLK_BDP_RLE_B, "rle_bclk", "mm_sel", 14),
	GATE_BDP0(CLK_BDP_RLE_AGENT, "rle_agent", "mm_sel", 15),
	GATE_BDP0(CLK_BDP_RLE_DRAM, "rle_dram", "mm_sel", 16),
	GATE_BDP0(CLK_BDP_F27M, "f27m", "di_sel", 17),
	GATE_BDP0(CLK_BDP_F27M_VDOUT, "f27m_vdout", "di_sel", 18),
	GATE_BDP0(CLK_BDP_F27_74_74, "f27_74_74", "di_sel", 19),
	GATE_BDP0(CLK_BDP_F2FS, "f2fs", "di_sel", 20),
	GATE_BDP0(CLK_BDP_F2FS74_148, "f2fs74_148", "di_sel", 21),
	GATE_BDP0(CLK_BDP_FB, "fbclk", "mm_sel", 22),
	GATE_BDP0(CLK_BDP_VDO_DRAM, "vdo_dram", "mm_sel", 23),
	GATE_BDP0(CLK_BDP_VDO_2FS, "vdo_2fs", "di_sel", 24),
	GATE_BDP0(CLK_BDP_VDO_B, "vdo_bclk", "mm_sel", 25),
	GATE_BDP0(CLK_BDP_WR_DI_PXL, "wr_di_pxl", "di_sel", 26),
	GATE_BDP0(CLK_BDP_WR_DI_DRAM, "wr_di_dram", "mm_sel", 27),
	GATE_BDP0(CLK_BDP_WR_DI_B, "wr_di_bclk", "mm_sel", 28),
	GATE_BDP0(CLK_BDP_NR_PXL, "nr_pxl", "nr_sel", 29),
	GATE_BDP0(CLK_BDP_NR_DRAM, "nr_dram", "mm_sel", 30),
	GATE_BDP0(CLK_BDP_NR_B, "nr_bclk", "mm_sel", 31),
	GATE_BDP1(CLK_BDP_RX_F, "rx_fclk", "hadds2_fbclk", 0),
	GATE_BDP1(CLK_BDP_RX_X, "rx_xclk", "clk26m", 1),
	GATE_BDP1(CLK_BDP_RXPDT, "rxpdtclk", "hdmi_0_pix340m", 2),
	GATE_BDP1(CLK_BDP_RX_CSCL_N, "rx_cscl_n", "clk26m", 3),
	GATE_BDP1(CLK_BDP_RX_CSCL, "rx_cscl", "clk26m", 4),
	GATE_BDP1(CLK_BDP_RX_DDCSCL_N, "rx_ddcscl_n", "hdmi_scl_rx", 5),
	GATE_BDP1(CLK_BDP_RX_DDCSCL, "rx_ddcscl", "hdmi_scl_rx", 6),
	GATE_BDP1(CLK_BDP_RX_VCO, "rx_vcoclk", "hadds2pll_294m", 7),
	GATE_BDP1(CLK_BDP_RX_DP, "rx_dpclk", "hdmi_0_pll340m", 8),
	GATE_BDP1(CLK_BDP_RX_P, "rx_pclk", "hdmi_0_pll340m", 9),
	GATE_BDP1(CLK_BDP_RX_M, "rx_mclk", "hadds2pll_294m", 10),
	GATE_BDP1(CLK_BDP_RX_PLL, "rx_pllclk", "hdmi_0_pix340m", 11),
	GATE_BDP1(CLK_BDP_BRG_RT_B, "brg_rt_bclk", "mm_sel", 12),
	GATE_BDP1(CLK_BDP_BRG_RT_DRAM, "brg_rt_dram", "mm_sel", 13),
	GATE_BDP1(CLK_BDP_LARBRT_DRAM, "larbrt_dram", "mm_sel", 14),
	GATE_BDP1(CLK_BDP_TMDS_SYN, "tmds_syn", "hdmi_0_pll340m", 15),
	GATE_BDP1(CLK_BDP_HDMI_MON, "hdmi_mon", "hdmi_0_pll340m", 16),
};

static void __init mtk_bdpsys_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	int r;

	clk_data = mtk_alloc_clk_data(CLK_BDP_NR);

	mtk_clk_register_gates(node, bdp_clks, ARRAY_SIZE(bdp_clks),
						clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);
}

#define MT8590_PLL_FMAX		(2000 * MHZ)
#define CON0_MT8590_RST_BAR	BIT(27)

#define PLL(_id, _name, _reg, _pwr_reg, _en_mask, _flags, _pcwbits, _pd_reg, \
			_pd_shift, _tuner_reg, _pcw_reg, _pcw_shift) {	\
		.id = _id,						\
		.name = _name,						\
		.reg = _reg,						\
		.pwr_reg = _pwr_reg,					\
		.en_mask = _en_mask,					\
		.flags = _flags,					\
		.rst_bar_mask = CON0_MT8590_RST_BAR,			\
		.fmax = MT8590_PLL_FMAX,				\
		.pcwbits = _pcwbits,					\
		.pd_reg = _pd_reg,					\
		.pd_shift = _pd_shift,					\
		.tuner_reg = _tuner_reg,				\
		.pcw_reg = _pcw_reg,					\
		.pcw_shift = _pcw_shift,				\
	}

static const struct mtk_pll_data apmixed_plls[] = {
	PLL(CLK_APMIXED_ARMPLL, "armpll", 0x200, 0x20c, 0x80000001, 0,
				21, 0x204, 24, 0x0, 0x204, 0),
	PLL(CLK_APMIXED_MAINPLL, "mainpll", 0x210, 0x21c, 0xf0000001,
		  HAVE_RST_BAR, 21, 0x210, 4, 0x0, 0x214, 0),
	PLL(CLK_APMIXED_UNIVPLL, "univpll", 0x220, 0x22c, 0xf3000001,
		  HAVE_RST_BAR, 7, 0x220, 4, 0x0, 0x224, 14),
	PLL(CLK_APMIXED_MMPLL, "mmpll", 0x230, 0x23c, 0x00000001, 0,
				21, 0x230, 4, 0x0, 0x234, 0),
	PLL(CLK_APMIXED_MSDCPLL, "msdcpll", 0x240, 0x24c, 0x00000001, 0,
				21, 0x240, 4, 0x0, 0x244, 0),
	PLL(CLK_APMIXED_TVDPLL, "tvdpll", 0x250, 0x25c, 0x00000001, 0,
				21, 0x250, 4, 0x0, 0x254, 0),
	PLL(CLK_APMIXED_AUD1PLL, "aud1pll", 0x270, 0x27c, 0x00000001, 0,
				31, 0x270, 4, 0x0, 0x274, 0),
	PLL(CLK_APMIXED_TRGPLL, "trgpll", 0x280, 0x28c, 0x00000001, 0,
				31, 0x280, 4, 0x0, 0x284, 0),
	PLL(CLK_APMIXED_ETHPLL, "ethpll", 0x290, 0x29c, 0x00000001, 0,
				31, 0x290, 4, 0x0, 0x294, 0),
	PLL(CLK_APMIXED_VDECPLL, "vdecpll", 0x2a0, 0x2ac, 0x00000001, 0,
				31, 0x2a0, 4, 0x0, 0x2a4, 0),
	PLL(CLK_APMIXED_HADDS2PLL, "hadds2pll", 0x2b0, 0x2bc, 0x00000001, 0,
				31, 0x2b0, 4, 0x0, 0x2b4, 0),
	PLL(CLK_APMIXED_AUD2PLL, "aud2pll", 0x2c0, 0x2cc, 0x00000001, 0,
				31, 0x2c0, 4, 0x0, 0x2c4, 0),
	PLL(CLK_APMIXED_TVD2PLL, "tvd2pll", 0x2d0, 0x2dc, 0x00000001, 0,
				21, 0x2d0, 4, 0x0, 0x2d4, 0),
};

static void __init mtk_apmixedsys_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	struct clk *clk;
	void __iomem *base;
	int r;

	base = of_iomap(node, 0);
	if (!base) {
		pr_err("%s(): ioremap failed\n", __func__);
		return;
	}

	pll_clk_data = clk_data = mtk_alloc_clk_data(CLK_APMIXED_NR);
	if (!clk_data)
		return;

	mtk_clk_register_plls(node, apmixed_plls, ARRAY_SIZE(apmixed_plls),
								clk_data);

	clk = clk_register_divider(NULL, "hdmi_ref", "tvdpll", 0, base + 0x40,
				   16, 3, CLK_DIVIDER_POWER_OF_TWO, NULL);

	clk_data->clks[CLK_APMIXED_HDMI_REF] = clk;

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);

	mtk_clk_enable_critical();
}
CLK_OF_DECLARE(mtk_apmixedsys, "mediatek,mt2701-apmixedsys",
							mtk_apmixedsys_init);

static const struct of_device_id of_clk_match_tbl[] = {
	{
		.compatible = "mediatek,mt2701-mmsys",
		.data = mtk_mmsys_init,
	}, {
		.compatible = "mediatek,mt2701-imgsys",
		.data = mtk_imgsys_init,
	}, {
		.compatible = "mediatek,mt2701-vdecsys",
		.data = mtk_vdecsys_init,
	}, {
		.compatible = "mediatek,mt2701-hifsys",
		.data = mtk_hifsys_init,
	}, {
		.compatible = "mediatek,mt2701-ethsys",
		.data = mtk_ethsys_init,
	}, {
		.compatible = "mediatek,mt2701-bdpsys",
		.data = mtk_bdpsys_init,
	}, {
		/* sentinel */
	}
};

static int __init clk_probe(struct platform_device *pdev)
{
	void (*clk_init)(struct device_node *);
	const struct of_device_id *of_id;

	of_id = of_match_node(of_clk_match_tbl, pdev->dev.of_node);
	if (!of_id || !of_id->data)
		return -EINVAL;

	clk_init = of_id->data;
	clk_init(pdev->dev.of_node);

	return 0;
}

static struct platform_driver clk_drv = {
	.driver = {
		.name = "mtk-clk",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(of_clk_match_tbl),
	},
};

builtin_platform_driver_probe(clk_drv, clk_probe);
