/*
 * Copyright (c) 2018 MediaTek Inc.
 * Author: Wenzhen Yu <wenzhen.yu@mediatek.com>
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

#include <linux/clk-provider.h>
#include <linux/syscore_ops.h>
#include <linux/version.h>

#define WARN_ON_CHECK_PLL_FAIL		0
#define CLKDBG_CCF_API_4_4	1

#define TAG	"[clkchk] "

#define clk_warn(fmt, args...)	pr_info(TAG fmt, ##args)

#if !CLKDBG_CCF_API_4_4

/* backward compatible */

static const char *clk_hw_get_name(const struct clk_hw *hw)
{
	return __clk_get_name(hw->clk);
}

static bool clk_hw_is_prepared(const struct clk_hw *hw)
{
	return __clk_is_prepared(hw->clk);
}

static bool clk_hw_is_enabled(const struct clk_hw *hw)
{
	return __clk_is_enabled(hw->clk);
}

#endif /* !CLKDBG_CCF_API_4_4 */

static const char * const *get_all_clk_names(void)
{
	static const char * const clks[] = {
		/* plls */
		"armpll",
		"mainpll",
		"univ2pll",
		"eth1pll",
		"eth2pll",
		"sgmipll",
		/* topckgen */
		"to_u2_phy",
		"to_u2_phy_1p",
		"pcie0_pipe_en",
		"pcie1_pipe_en",
		"ssusb_tx250m",
		"ssusb_eq_rx250m",
		"ssusb_cdr_ref",
		"ssusb_cdr_fb",
		"sata_asic",
		"sata_rbc",
		"to_usb3_sys",
		"p1_1mhz",
		"free_run_4mhz",
		"p0_1mhz",
		"txclk_src_pre",
		"rtc",
		"pwm_qtr_26m",
		"cpum_tck_in",
		"to_usb3_da_top",
		"mempll",
		"dmpll_ck",
		"dmpll_d4",
		"dmpll_d8",
		"syspll_d2",
		"syspll1_d2",
		"syspll1_d4",
		"syspll1_d8",
		"syspll1_d16",
		"syspll2_d2",
		"syspll2_d4",
		"syspll2_d8",
		"syspll_d5",
		"syspll3_d2",
		"syspll3_d4",
		"syspll_d7",
		"syspll4_d2",
		"syspll4_d4",
		"syspll4_d16",
		"univpll",
		"univpll1_d2",
		"univpll1_d4",
		"univpll1_d8",
		"univpll_d3",
		"univpll2_d2",
		"univpll2_d4",
		"univpll2_d8",
		"univpll2_d16",
		"univpll_d5",
		"univpll3_d2",
		"univpll3_d4",
		"univpll3_d16",
		"univpll_d7",
		"univpll_d80_d4",
		"univ48m",
		"sgmiipll_d2",
		"clkxtal_d4",
		"hd_faxi",
		"faxi",
		"f_faud_intbus",
		"ap2wbhif_hclk",
		"infrao_10m",
		"msdc30_1",
		"spi",
		"sf",
		"flash",
		"to_usb3_ref",
		"to_usb3_mcu",
		"to_usb3_dma",
		"from_top_ahb",
		"from_top_axi",
		"pcie1_mac_en",
		"pcie0_mac_en",
		"axi_sel",
		"mem_sel",
		"ddrphycfg_sel",
		"eth_sel",
		"pwm_sel",
		"f10m_ref_sel",
		"nfi_infra_sel",
		"flash_sel",
		"uart_sel",
		"spi0_sel",
		"spi1_sel",
		"msdc50_0_sel",
		"msdc30_0_sel",
		"msdc30_1_sel",
		"ap2wbmcu_sel",
		"ap2wbhif_sel",
		"audio_sel",
		"aud_intbus_sel",
		"pmicspi_sel",
		"scp_sel",
		"atb_sel",
		"hif_sel",
		"sata_sel",
		"usb20_sel",
		"aud1_sel",
		"aud2_sel",
		"irrx_sel",
		"irtx_sel",
		"sata_mcu_sel",
		"pcie0_mcu_sel",
		"pcie1_mcu_sel",
		"ssusb_mcu_sel",
		"crypto_sel",
		"sgmii_ref_1_sel",
		"gpt10m_sel",
		/* infracfg */
		"infra_dbgclk_pd",
		"infra_trng_pd",
		"infra_devapc_pd",
		"infra_apxgpt_pd",
		"infra_sej_pd",
		"infra_mux1_sel",
		/* pericfg */
		"peri_pwm1_pd",
		"peri_pwm2_pd",
		"peri_pwm3_pd",
		"peri_pwm4_pd",
		"peri_pwm5_pd",
		"peri_pwm6_pd",
		"peri_pwm7_pd",
		"peri_pwm_pd",
		"peri_ap_dma_pd",
		"peri_msdc30_1",
		"peri_uart0_pd",
		"peri_uart1_pd",
		"peri_uart2_pd",
		"peri_uart3_pd",
		"peri_btif_pd",
		"peri_i2c0_pd",
		"peri_spi0_pd",
		"peri_snfi_pd",
		"peri_nfi_pd",
		"peri_nfiecc_pd",
		"peri_flash_pd",
		"peribus_ck_sel",
		/* ssusbsys */
		"ssusb_u2_phy_1p",
		"ssusb_u2_phy_en",
		"ssusb_ref_en",
		"ssusb_sys_en",
		"ssusb_mcu_en",
		"ssusb_dma_en",
		/* pciesys */
		"pcie_p1_aux_en",
		"pcie_p1_obff_en",
		"pcie_p1_ahb_en",
		"pcie_p1_axi_en",
		"pcie_p1_mac_en",
		"pcie_p1_pipe_en",
		"pcie_p0_aux_en",
		"pcie_p0_obff_en",
		"pcie_p0_ahb_en",
		"pcie_p0_axi_en",
		"pcie_p0_mac_en",
		"pcie_p0_pipe_en",
		/* ethsys */
		"eth_fe_en",
		"eth_gp2_en",
		"eth_gp1_en",
		"eth_gp0_en",
		"eth_500m",
		/* sgmiisys */
		"sgmii0_tx_en",
		"sgmii0_rx_en",
		"sgmii0_cdr_ref",
		"sgmii0_cdr_fb",
		"sgmii1_tx_en",
		"sgmii1_rx_en",
		"sgmii1_cdr_ref",
		"sgmii1_cdr_fb",
		/* end */
		NULL
	};

	return clks;
}

static const char *ccf_state(struct clk_hw *hw)
{
	if (__clk_get_enable_count(hw->clk))
		return "enabled";

	if (clk_hw_is_prepared(hw))
		return "prepared";

	return "disabled";
}

static void print_enabled_clks(void)
{
	const char * const *cn = get_all_clk_names();

	clk_warn("enabled clks:\n");

	for (; *cn; cn++) {
		struct clk *c = __clk_lookup(*cn);
		struct clk_hw *c_hw = __clk_get_hw(c);
		struct clk_hw *p_hw;

		if (IS_ERR_OR_NULL(c) || !c_hw)
			continue;

		p_hw = clk_hw_get_parent(c_hw);

		if (!p_hw)
			continue;

		if (!clk_hw_is_prepared(c_hw) && !__clk_get_enable_count(c))
			continue;

		clk_warn("[%-17s: %8s, %3d, %3d, %10ld, %17s]\n",
			clk_hw_get_name(c_hw),
			ccf_state(c_hw),
			clk_hw_is_prepared(c_hw),
			__clk_get_enable_count(c),
			clk_hw_get_rate(c_hw),
			p_hw ? clk_hw_get_name(p_hw) : "- ");
	}
}

static void check_pll_off(void)
{
	static const char * const off_pll_names[] = {
		"univ2pll",
		"eth1pll",
		"eth2pll",
		"sgmipll",
		NULL
	};

	static struct clk *off_plls[ARRAY_SIZE(off_pll_names)];

	struct clk **c;
	int invalid = 0;
	char buf[128] = {0};
	int n = 0;

	if (!off_plls[0]) {
		const char * const *pn;

		for (pn = off_pll_names, c = off_plls; *pn; pn++, c++)
			*c = __clk_lookup(*pn);
	}

	for (c = off_plls; *c; c++) {
		struct clk_hw *c_hw = __clk_get_hw(*c);

		if (!c_hw)
			continue;

		if (!clk_hw_is_prepared(c_hw) && !clk_hw_is_enabled(c_hw))
			continue;

		n += snprintf(buf + n, sizeof(buf) - n, "%s ",
				clk_hw_get_name(c_hw));

		invalid++;
	}

	if (invalid) {
		clk_warn("unexpected unclosed PLL: %s\n", buf);
		print_enabled_clks();

#if WARN_ON_CHECK_PLL_FAIL
		WARN_ON(1);
#endif
	}
}

static int clkchk_syscore_suspend(void)
{
	check_pll_off();

	return 0;
}

static void clkchk_syscore_resume(void)
{
}

static struct syscore_ops clkchk_syscore_ops = {
	.suspend = clkchk_syscore_suspend,
	.resume = clkchk_syscore_resume,
};

static int __init clkchk_init(void)
{
	/*
	 * if (!of_machine_is_compatible("mediatek,mt7622"))
	 *	 return -ENODEV;
	 */
	register_syscore_ops(&clkchk_syscore_ops);

	return 0;
}
subsys_initcall(clkchk_init);
