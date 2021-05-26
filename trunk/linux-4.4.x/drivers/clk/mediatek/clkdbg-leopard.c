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
#include <linux/io.h>

#include "clkdbg.h"

#define DUMP_INIT_STATE		0

/*
 * clkdbg dump_regs
 */

enum {
	infrasys,
	perisys,
	scpsys,
	apmixed,
	fhctl,
	topckgen,
	ssusb,
	pcie,
	ethdma,
	sgmii0,
	sgmii1,
};

#define REGBASE_V(_phys, _id_name) { .phys = _phys, .name = #_id_name }

/*
 * checkpatch.pl ERROR:COMPLEX_MACRO
 *
 * #define REGBASE(_phys, _id_name) [_id_name] = REGBASE_V(_phys, _id_name)
 */

static struct regbase rb[] = {
	[infrasys] = REGBASE_V(0x10000000, infrasys),
	[perisys] = REGBASE_V(0x10002000, perisys),
	[scpsys]   = REGBASE_V(0x10006000, scpsys),
	[apmixed]  = REGBASE_V(0x10209000, apmixed),
	[fhctl]     = REGBASE_V(0x10209f00, fhctl),
	[topckgen] = REGBASE_V(0x10210000, topckgen),
	[ssusb]   = REGBASE_V(0x1A000000, ssusb),
	[pcie]    = REGBASE_V(0x1A100800, pcie),
	[ethdma]   = REGBASE_V(0x1B000000, ethdma),
	[sgmii0]  = REGBASE_V(0x1B128000, sgmii0),
	[sgmii1]  = REGBASE_V(0x1B130000, sgmii1),
};

#define REGNAME(_base, _ofs, _name)	\
	{ .base = &rb[_base], .ofs = _ofs, .name = #_name }

static struct regname rn[] = {
	REGNAME(infrasys, 0x040, INFRA_GLOBALCON_PDN0),
	REGNAME(infrasys, 0x044, INFRA_GLOBALCON_PDN1),
	REGNAME(infrasys, 0x048, INFRA_PDN_STA),
	REGNAME(perisys, 0x008, PERI_GLOBALCON_PDN0_SET),
	REGNAME(perisys, 0x00C, PERI_GLOBALCON_PDN1_SET),
	REGNAME(perisys, 0x010, PERI_GLOBALCON_PDN0_CLR),
	REGNAME(perisys, 0x014, PERI_GLOBALCON_PDN1_CLR),
	REGNAME(perisys, 0x018, PERI_GLOBALCON_PDN0_STA),
	REGNAME(perisys, 0x01C, PERI_GLOBALCON_PDN1_STA),
	REGNAME(scpsys, 0x2E0, SPM_ETH_PWR_CON),
	REGNAME(scpsys, 0x2E4, SPM_HIF0_PWR_CON),
	REGNAME(scpsys, 0x2E8, SPM_HIF1_PWR_CON),
	REGNAME(scpsys, 0x60c, SPM_PWR_STATUS),
	REGNAME(scpsys, 0x610, SPM_PWR_STATUS_2ND),
	REGNAME(apmixed, 0x200, ARMPLL_CON0),
	REGNAME(apmixed, 0x204, ARMPLL_CON1),
	REGNAME(apmixed, 0x20C, ARMPLL_PWR_CON0),
	REGNAME(apmixed, 0x210, MAINPLL_CON0),
	REGNAME(apmixed, 0x214, MAINPLL_CON1),
	REGNAME(apmixed, 0x21C, MAINPLL_PWR_CON0),
	REGNAME(apmixed, 0x220, UNIVPLL_CON0),
	REGNAME(apmixed, 0x224, UNIVPLL_CON1),
	REGNAME(apmixed, 0x22C, UNIVPLL_PWR_CON0),
	REGNAME(apmixed, 0x300, ETH1PLL_CON0),
	REGNAME(apmixed, 0x304, ETH1PLL_CON1),
	REGNAME(apmixed, 0x310, ETH1PLL_PWR_CON0),
	REGNAME(apmixed, 0x314, ETH2PLL_CON0),
	REGNAME(apmixed, 0x318, ETH2PLL_CON1),
	REGNAME(apmixed, 0x320, ETH2PLL_PWR_CON0),
	REGNAME(apmixed, 0x358, SGMIPLL_CON0),
	REGNAME(apmixed, 0x35C, SGMIPLL_CON1),
	REGNAME(apmixed, 0x368, SGMIPLL_PWR_CON0),
	REGNAME(topckgen, 0x040, CLK_CFG_0),
	REGNAME(topckgen, 0x050, CLK_CFG_1),
	REGNAME(topckgen, 0x060, CLK_CFG_2),
	REGNAME(topckgen, 0x070, CLK_CFG_3),
	REGNAME(topckgen, 0x080, CLK_CFG_4),
	REGNAME(topckgen, 0x090, CLK_CFG_5),
	REGNAME(topckgen, 0x0A0, CLK_CFG_6),
	REGNAME(topckgen, 0x0B0, CLK_CFG_7),
	REGNAME(topckgen, 0x0c0, CLK_CFG_8),
	REGNAME(ssusb, 0x30, USB23_CSR_O_CLKCFG1),
	REGNAME(pcie, 0x30, COMBO_CSR_O_CLKCFG1),
	REGNAME(ethdma, 0x30, ETHDMACTL_CLKCFG1),
	REGNAME(sgmii0, 0xE4, SGMII0_QPHY_CTRL),
	REGNAME(sgmii1, 0xE4, SGMII1_QPHY_CTRL),
	{NULL, 0, NULL}
};

static const struct regname *get_all_regnames(void)
{
	return rn;
}

static void __init init_regbase(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(rb); i++)
		rb[i].virt = ioremap(rb[i].phys, PAGE_SIZE);
}

/*
 * clkdbg fmeter
 */

#include <linux/delay.h>

#ifndef GENMASK
#define GENMASK(h, l)	(((1U << ((h) - (l) + 1)) - 1) << (l))
#endif

#define ALT_BITS(o, h, l, v) \
	(((o) & ~GENMASK(h, l)) | (((v) << (l)) & GENMASK(h, l)))

#define clk_readl(addr)		readl(addr)
#define clk_writel(addr, val)	\
	do { writel(val, addr); wmb(); } while (0) /* sync write */
#define clk_writel_mask(addr, mask, val)	\
	clk_writel(addr, (clk_readl(addr) & ~(mask)) | (val))

#define ABS_DIFF(a, b)	((a) > (b) ? (a) - (b) : (b) - (a))

enum FMETER_TYPE {
	FT_NULL,
	ABIST,
	CKGEN
};

#define FMCLK(_t, _i, _n) { .type = _t, .id = _i, .name = _n }

static const struct fmeter_clk fclks[] = {
	FMCLK(ABIST,   1, "clkph_MCK_o"),
	FMCLK(ABIST,   2, "AD_MEMPLL2_CKOUT0_PRE_ISO"),
	FMCLK(ABIST,   3, "AD_MAIN_H546M_CK"),
	FMCLK(ABIST,   4, "AD_MAIN_H364M_CK"),
	FMCLK(ABIST,   5, "AD_MAIN_H218P4M_CK"),
	FMCLK(ABIST,   6, "AD_MAIN_H156M_CK"),
	FMCLK(ABIST,   7, "AD_UNIV_624M_CK"),
	FMCLK(ABIST,   8, "AD_UNIV_416M_CK"),
	FMCLK(ABIST,   9, "AD_UNIV_249P6M_CK"),
	FMCLK(ABIST,  10, "AD_UNIV_178P3M_CK"),
	FMCLK(ABIST,  11, "f_funivpll_d80_ck"),
	FMCLK(ABIST,  12, "AD_UNIV_48M_CK"),
	FMCLK(ABIST,  13, "AD_SGMIIPLL_CK"),
	FMCLK(ABIST,  14, "AD_AUD1PLL_CK"),
	FMCLK(ABIST,  15, "AD_AUD2PLL_CK"),
	FMCLK(ABIST,  16, "AD_ARMPLL_TOP_TST_CK"),
	FMCLK(ABIST,  17, "AD_USB_48M_CK"),
	FMCLK(ABIST,  18, "AD_MAINPLL_CORE_CK"),
	FMCLK(ABIST,  19, "AD_TRGPLL_CK"),
	FMCLK(ABIST,  20, "AD_MEM_26M_CK"),
	FMCLK(ABIST,  21, "AD_PLLGP_TST_CK"),
	FMCLK(ABIST,  22, "AD_ETH1PLL_CK"),
	FMCLK(ABIST,  23, "AD_ETH2PLL_CK"),
	FMCLK(ABIST,  24, "AD_UNIVPLL_CK"),
	FMCLK(ABIST,  25, "AD_MEM2MIPI_26M_CK"),
	FMCLK(ABIST,  26, "AD_MEMPLL_MONCLK"),
	FMCLK(ABIST,  27, "AD_MEMPLL2_MONCLK"),
	FMCLK(ABIST,  28, "AD_MEMPLL3_MONCLK"),
	FMCLK(ABIST,  29, "AD_MEMPLL4_MONCLK"),
	FMCLK(ABIST,  30, "AD_MEMPLL_REFCLK_BUF"),
	FMCLK(ABIST,  31, "AD_MEMPLL_FBCLK_BUF"),
	FMCLK(ABIST,  32, "AD_MEMPLL2_REFCLK_BUF"),
	FMCLK(ABIST,  33, "AD_MEMPLL2_FBCLK_BUF"),
	FMCLK(ABIST,  34, "AD_MEMPLL3_REFCLK_BUF"),
	FMCLK(ABIST,  35, "AD_MEMPLL3_FBCLK_BUF"),
	FMCLK(ABIST,  36, "AD_MEMPLL4_REFCLK_BUF"),
	FMCLK(ABIST,  37, "AD_MEMPLL4_FBCLK_BUF"),
	FMCLK(ABIST,  38, "AD_MEMPLL_TSTDIV2_CK"),
	FMCLK(ABIST,  39, "XTAL"),
	FMCLK(ABIST,  40, "trng_freq_debug_in0"),
	FMCLK(ABIST,  41, "trng_freq_debug_in1"),
	FMCLK(CKGEN,   1, "hf_faxi_ck"),
	FMCLK(CKGEN,   2, "hf_fmem_ck"),
	FMCLK(CKGEN,   3, "hf_fddrphycfg_ck"),
	FMCLK(CKGEN,   4, "hf_feth_ck"),
	FMCLK(CKGEN,   5, "f_fpwm_ck"),
	FMCLK(CKGEN,   6, "f_fsgmii_ref_0_ck"),
	FMCLK(CKGEN,   7, "hf_fspinfi_infra_bclk_ck"),
	FMCLK(CKGEN,   8, "hf_fflash_ck"),
	FMCLK(CKGEN,   9, "f_fuart_ck"),
	FMCLK(CKGEN,  10, "hf_fspi0_ck"),
	FMCLK(CKGEN,  11, "hf_fspi1_ck"),
	FMCLK(CKGEN,  12, "hf_fmsdc50_0_ck"),
	FMCLK(CKGEN,  13, "hf_fmsdc30_0_ck"),
	FMCLK(CKGEN,  14, "hf_fmsdc30_1_ck"),
	FMCLK(CKGEN,  15, "hf_fap2wbmcu_ck"),
	FMCLK(CKGEN,  16, "hf_fap2wbhif_ck"),
	FMCLK(CKGEN,  17, "hf_audio_ck"),
	FMCLK(CKGEN,  18, "hf_faud_intbus_ck"),
	FMCLK(CKGEN,  19, "hf_fpmicspi_ck"),
	FMCLK(CKGEN,  20, "hf_fscp_ck"),
	FMCLK(CKGEN,  21, "hf_fatb_ck"),
	FMCLK(CKGEN,  22, "hf_fhif_ck"),
	FMCLK(CKGEN,  23, "f_fsata_ck"),
	FMCLK(CKGEN,  24, "hf_fusb20_ck"),
	FMCLK(CKGEN,  25, "f_faud1_ck"),
	FMCLK(CKGEN,  26, "f_faud2_ck"),
	FMCLK(CKGEN,  27, "hf_firrx_ck"),
	FMCLK(CKGEN,  28, "hf_firtx_ck"),
	FMCLK(CKGEN,  29, "hf_fsata_mcu_ck"),
	FMCLK(CKGEN,  30, "hf_fpcie0_mcu_ck"),
	FMCLK(CKGEN,  31, "hf_fpcie1_mcu_ck"),
	FMCLK(CKGEN,  32, "hf_fssusb_mcu_ck"),
	FMCLK(CKGEN,  33, "hf_fcrypto_ck"),
	FMCLK(CKGEN,  34, "f_fsgmii_ref_1_ck"),
	FMCLK(CKGEN,  35, "hf_f10m_ck"),
	FMCLK(CKGEN,  36, "f_faud26m_ck"),
	FMCLK(CKGEN,  37, "f_fsata_ref_ck"),
	FMCLK(CKGEN,  38, "f_fpcie_ref_ck"),
	FMCLK(CKGEN,  39, "f_fssusb_ref_ck"),
	FMCLK(CKGEN,  40, "f_funivpll3_d16_ck"),
	FMCLK(CKGEN,  41, "f_fauxadc_ck"),
	FMCLK(CKGEN,  42, "f_f75k_ck"),
	FMCLK(CKGEN,  43, "f_fefuse_ck"),
	FMCLK(CKGEN,  44, "f_fapmixed_ck"),
	FMCLK(CKGEN,  45, "f_frtc_apmixed_ck"),
	FMCLK(CKGEN,  46, "f_fmsdc_ext_ck"),
	FMCLK(CKGEN,  47, "hf_fpmicspi_ck_scan"),
	FMCLK(CKGEN,  48, "hf_fspi0_pad_ck"),
	FMCLK(CKGEN,  49, "hf_fspi1_pad_ck"),
	FMCLK(CKGEN,  50, "hf_fddrphycfg_ck_scan"),
	FMCLK(CKGEN,  51, "f_fddrphyperi_ck_scan"),
	FMCLK(CKGEN,  52, "f_frtc_fddrphyperi_ck"),
	FMCLK(CKGEN,  53, "hf_faxi_ck_scan"),
	FMCLK(CKGEN,  54, "hd_qaxidcm_ck"),
	FMCLK(CKGEN,  55, "f_fckbus_ck_scan"),
	FMCLK(CKGEN,  56, "f_fckrtc_ck_scan"),
	FMCLK(CKGEN,  57, "f_fclkmux_ck"),
	FMCLK(CKGEN,  58, "f_fxtal_ck"),
	FMCLK(CKGEN,  59, "f_fxtal_ck_cg"),
	FMCLK(CKGEN,  60, "f_fxtal_ck_scan"),
	FMCLK(CKGEN,  61, "f_frtc_ck"),
	FMCLK(CKGEN,  62, "f_frtc_ck_scan"),
	FMCLK(CKGEN,  63, "hf_fpcie1_mcu_ck_scan"),
	{}
};

#define FHCTL_HP_EN		(rb[fhctl].virt + 0x000)
#define CLK_CFG_M0		(rb[topckgen].virt + 0x100)
#define CLK_CFG_M1		(rb[topckgen].virt + 0x104) /* all is CFG_9 for leop */
#define CLK_MISC_CFG_1	(rb[topckgen].virt + 0x214)
#define CLK_MISC_CFG_2	(rb[topckgen].virt + 0x218)
#define CLK26CALI_0		(rb[topckgen].virt + 0x220)
#define CLK26CALI_1		(rb[topckgen].virt + 0x224)
#define CLK26CALI_2		(rb[topckgen].virt + 0x228)

#define RG_FRMTR_WINDOW     1023

static void set_fmeter_divider_arm(u32 k1)
{
	u32 v = clk_readl(CLK_MISC_CFG_1);

	v = ALT_BITS(v, 15, 8, k1);
	clk_writel(CLK_MISC_CFG_1, v);
}

static void set_fmeter_divider(u32 k1)
{
	u32 v = clk_readl(CLK_MISC_CFG_1);

	v = ALT_BITS(v, 7, 0, k1);
	v = ALT_BITS(v, 31, 24, k1);
	clk_writel(CLK_MISC_CFG_1, v);
}

static u8 wait_fmeter_done(u32 tri_bit)
{
	static int max_wait_count;
	int wait_count = (max_wait_count > 0) ? (max_wait_count * 2 + 2) : 100;
	int i;

	/* wait fmeter */
	for (i = 0; i < wait_count && (clk_readl(CLK26CALI_0) & tri_bit); i++)
		udelay(20);

	if (!(clk_readl(CLK26CALI_0) & tri_bit)) {
		max_wait_count = max(max_wait_count, i);
		return 1;
	}

	return 0;
}
static u32 fmeter_freq(enum FMETER_TYPE type, int k1, int clk)
{
	void __iomem *clk_cfg_reg = CLK_CFG_M1;
	void __iomem *cnt_reg = (type == CKGEN) ? CLK26CALI_2 : CLK26CALI_1;
	u32 cksw_mask = (type == CKGEN) ? GENMASK(21, 16) : GENMASK(13, 8);
	u32 cksw_val = (type == CKGEN) ? (clk << 16) : (clk << 8);
	u32 tri_bit = (type == CKGEN) ? BIT(4) : BIT(0);
	u32 clk_exc = (type == CKGEN) ? BIT(5) : BIT(2);
	u32 clk_misc_cfg_1, clk_misc_cfg_2, clk_cfg_val, cnt, freq = 0;

	/* setup fmeter */
	clk_setl(CLK26CALI_0, BIT(7));	/* enable fmeter_en */
	clk_clrl(CLK26CALI_0, clk_exc);	/* set clk_exc */
	clk_writel_mask(cnt_reg, GENMASK(25, 16), RG_FRMTR_WINDOW << 16);	/* load_cnt */

	clk_misc_cfg_1 = clk_readl(CLK_MISC_CFG_1);	/* backup CLK_MISC_CFG_1 value */
	clk_misc_cfg_2 = clk_readl(CLK_MISC_CFG_2);	/* backup CLK_MISC_CFG_2 value */
	clk_cfg_val = clk_readl(clk_cfg_reg);		/* backup clk_cfg_reg value */

	set_fmeter_divider(k1);			/* set divider (0 = /1) */
	set_fmeter_divider_arm(k1);
	clk_writel_mask(clk_cfg_reg, cksw_mask, cksw_val);	/* select cksw */

	clk_setl(CLK26CALI_0, tri_bit);	/* start fmeter */

	if (wait_fmeter_done(tri_bit)) {
		cnt = clk_readl(cnt_reg) & 0xFFFF;
		freq = (cnt * 40000) * (k1 + 1) / (RG_FRMTR_WINDOW + 1); /* (KHz) ; freq = counter * 26M / 1024 */
	}

	/* restore register settings */
	clk_writel(clk_cfg_reg, clk_cfg_val);
	clk_writel(CLK_MISC_CFG_2, clk_misc_cfg_2);
	clk_writel(CLK_MISC_CFG_1, clk_misc_cfg_1);

	clk_clrl(CLK26CALI_0, BIT(7));	/* disable fmeter_en */

	return freq;
}

static u32 measure_stable_fmeter_freq(enum FMETER_TYPE type, int k1, int clk)
{
	u32 last_freq = 0;
	u32 freq = fmeter_freq(type, k1, clk);
	u32 maxfreq = max(freq, last_freq);

	while (maxfreq > 0 && ABS_DIFF(freq, last_freq) * 100 / maxfreq > 10) {
		last_freq = freq;
		freq = fmeter_freq(type, k1, clk);
		maxfreq = max(freq, last_freq);
	}

	return freq;
}

static const struct fmeter_clk *get_all_fmeter_clks(void)
{
	return fclks;
}

struct bak {
	u32 fhctl_hp_en;
};

static void *prepare_fmeter(void)
{
	static struct bak regs;

	regs.fhctl_hp_en = clk_readl(FHCTL_HP_EN);

	clk_writel(FHCTL_HP_EN, 0x0);		/* disable PLL hopping */
	udelay(10);

	return &regs;
}

static void unprepare_fmeter(void *data)
{
	struct bak *regs = data;

	/* restore old setting */
	clk_writel(FHCTL_HP_EN, regs->fhctl_hp_en);
}

static u32 fmeter_freq_op(const struct fmeter_clk *fclk)
{
	if (fclk->type)
		return measure_stable_fmeter_freq(fclk->type, 0, fclk->id);

	return 0;
}

/*
 * clkdbg dump_state
 */

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

/*
 * clkdbg pwr_status
 */

static const char * const *get_pwr_names(void)
{
	static const char * const pwr_names[] = {
		[24] = "ETHSYS",
		[25] = "HIF0",
		[26] = "HIF1",
	};

	return pwr_names;
}

/*
 * init functions
 */

static struct clkdbg_ops clkdbg_leopard_ops = {
	.get_all_fmeter_clks = get_all_fmeter_clks,
	.prepare_fmeter = prepare_fmeter,
	.unprepare_fmeter = unprepare_fmeter,
	.fmeter_freq = fmeter_freq_op,
	.get_all_regnames = get_all_regnames,
	.get_all_clk_names = get_all_clk_names,
	.get_pwr_names = get_pwr_names,
};

static int __init clkdbg_leopard_init(void)
{
	/*
	 * if (!of_machine_is_compatible("mediatek,leopard"))
	 *	 return -ENODEV;
     */
	init_regbase();

	set_clkdbg_ops(&clkdbg_leopard_ops);

#if DUMP_INIT_STATE
	print_regs();
	print_fmeter_all();
#endif /* DUMP_INIT_STATE */

	return 0;
}
device_initcall(clkdbg_leopard_init);
