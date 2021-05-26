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

#include <linux/clk-provider.h>
#include <linux/syscore_ops.h>
#include <linux/version.h>

#define WARN_ON_CHECK_PLL_FAIL		0
#define CLKDBG_CCF_API_4_4	1

#define TAG	"[clkchk] "

#define clk_warn(fmt, args...)	pr_warn(TAG fmt, ##args)

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
		"aud1pll",
		"aud2pll",
		"trgpll",
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
		"mempll",
		"dmpll_ck",
		"syspll_d2",
		"syspll1_d2",
		"syspll1_d4",
		"syspll1_d8",
		"syspll2_d4",
		"syspll2_d8",
		"syspll_d5",
		"syspll3_d2",
		"syspll3_d4",
		"syspll4_d2",
		"syspll4_d4",
		"syspll4_d16",
		"univpll",
		"univpll_d2",
		"univpll1_d2",
		"univpll1_d4",
		"univpll1_d8",
		"univpll1_d16",
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
		"sgmiipll_ck",
		"sgmiipll_d2",
		"aud1pll_ck",
		"aud2pll_ck",
		"aud_i2s2_mck",
		"to_usb3_ref",
		"pcie1_mac_en",
		"pcie0_mac_en",
		"eth_500m",
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
		"a1sys_hp_sel",
		"a2sys_hp_sel",
		"intdir_sel",
		"aud_intbus_sel",
		"pmicspi_sel",
		"scp_sel",
		"atb_sel",
		"hif_sel",
		"audio_sel",
		"usb20_sel",
		"aud1_sel",
		"aud2_sel",
		"irrx_sel",
		"irtx_sel",
		"asm_l_sel",
		"asm_m_sel",
		"asm_h_sel",
		"apll1_ck_sel",
		"apll2_ck_sel",
		"i2s0_mck_sel",
		"i2s1_mck_sel",
		"i2s2_mck_sel",
		"i2s3_mck_sel",
		"apll1_ck_div",
		"apll2_ck_div",
		"i2s0_mck_div",
		"i2s1_mck_div",
		"i2s2_mck_div",
		"i2s3_mck_div",
		"a1sys_div",
		"a2sys_div",
		"apll1_ck_div_pd",
		"apll2_ck_div_pd",
		"i2s0_mck_div_pd",
		"i2s1_mck_div_pd",
		"i2s2_mck_div_pd",
		"i2s3_mck_div_pd",
		"a1sys_div_pd",
		"a2sys_div_pd",
		/* infracfg */
		"infra_mux1_sel",
		"infra_dbgclk_pd",
		"infra_audio_pd",
		"infra_irrx_pd",
		"infra_apxgpt_pd",
		"infra_pmic_pd",
		/* pericfg */
		"peribus_ck_sel",
		"peri_therm_pd",
		"peri_pwm1_pd",
		"peri_pwm2_pd",
		"peri_pwm3_pd",
		"peri_pwm4_pd",
		"peri_pwm5_pd",
		"peri_pwm6_pd",
		"peri_pwm7_pd",
		"peri_pwm_pd",
		"peri_ap_dma_pd",
		"peri_msdc30_0",
		"peri_msdc30_1",
		"peri_uart0_pd",
		"peri_uart1_pd",
		"peri_uart2_pd",
		"peri_uart3_pd",
		"peri_uart4_pd",
		"peri_btif_pd",
		"peri_i2c0_pd",
		"peri_i2c1_pd",
		"peri_i2c2_pd",
		"peri_spi1_pd",
		"peri_auxadc_pd",
		"peri_spi0_pd",
		"peri_snfi_pd",
		"peri_nfi_pd",
		"peri_nfiecc_pd",
		"peri_flash_pd",
		"peri_irtx_pd",
		/* audiosys */
		"audio_afe",
		"audio_hdmi",
		"audio_spdf",
		"audio_apll",
		"audio_i2sin1",
		"audio_i2sin2",
		"audio_i2sin3",
		"audio_i2sin4",
		"audio_i2so1",
		"audio_i2so2",
		"audio_i2so3",
		"audio_i2so4",
		"audio_asrci1",
		"audio_asrci2",
		"audio_asrco1",
		"audio_asrco2",
		"audio_intdir",
		"audio_a1sys",
		"audio_a2sys",
		"audio_ul1",
		"audio_ul2",
		"audio_ul3",
		"audio_ul4",
		"audio_ul5",
		"audio_ul6",
		"audio_dl1",
		"audio_dl2",
		"audio_dl3",
		"audio_dl4",
		"audio_dl5",
		"audio_dl6",
		"audio_dlmch",
		"audio_arb1",
		"audio_awb",
		"audio_awb2",
		"audio_dai",
		"audio_mod",
		"audio_asrci3",
		"audio_asrci4",
		"audio_asrco3",
		"audio_asrco4",
		"audio_mem_asrc1",
		"audio_mem_asrc2",
		"audio_mem_asrc3",
		"audio_mem_asrc4",
		"audio_mem_asrc5",
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
		"sata_ahb_en",
		"sata_axi_en",
		"sata_asic_en",
		"sata_rbc_en",
		"sata_pm_en",
		/* ethsys */
		"eth_hsdma_en",
		"eth_esw_en",
		"eth_gp2_en",
		"eth_gp1_en",
		"eth_gp0_en",
		/* sgmiisys */
		"sgmii_tx250m_en",
		"sgmii_rx250m_en",
		"sgmii_cdr_ref",
		"sgmii_cdr_fb",
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
		"aud1pll",
		"aud2pll",
		"trgpll",
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
	if (!of_machine_is_compatible("mediatek,mt7622"))
		return -ENODEV;

	register_syscore_ops(&clkchk_syscore_ops);

	return 0;
}
subsys_initcall(clkchk_init);
