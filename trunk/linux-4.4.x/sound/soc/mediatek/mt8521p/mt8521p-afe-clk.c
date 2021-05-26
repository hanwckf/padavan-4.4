/* Copyright (c) 2011-2013, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include "mt8521p-afe-reg.h"
#include "mt8521p-aud-global.h"

enum audio_system_clock_type {
	AUDCLK_INFRA_SYS_AUDIO, /*INFRA_PDN[5]*/
	AUDCLK_TOP_AUD_MUX1_SEL,
	AUDCLK_TOP_AUD_MUX2_SEL,
	AUDCLK_TOP_AUD_MUX1_DIV,
	AUDCLK_TOP_AUD_MUX2_DIV,
	AUDCLK_TOP_AUD_48K_TIMING,
	AUDCLK_TOP_AUD_44K_TIMING,
	AUDCLK_TOP_AUDPLL_MUX_SEL,
	AUDCLK_TOP_APLL_SEL,
	AUDCLK_TOP_AUD1PLL_98M,
	AUDCLK_TOP_AUD2PLL_90M,
	AUDCLK_TOP_HADDS2PLL_98M,
	AUDCLK_TOP_HADDS2PLL_294M,
	AUDCLK_TOP_AUDPLL,
	AUDCLK_TOP_AUDPLL_D4,
	AUDCLK_TOP_AUDPLL_D8,
	AUDCLK_TOP_AUDPLL_D16,
	AUDCLK_TOP_AUDPLL_D24,
	AUDCLK_TOP_AUDINTBUS,
	AUDCLK_CLK_26M,
	AUDCLK_TOP_SYSPLL1_D4,
	AUDCLK_TOP_AUD_K1_SRC_SEL,
	AUDCLK_TOP_AUD_K2_SRC_SEL,
	AUDCLK_TOP_AUD_K3_SRC_SEL,
	AUDCLK_TOP_AUD_K4_SRC_SEL,
	AUDCLK_TOP_AUD_K5_SRC_SEL,
	AUDCLK_TOP_AUD_K6_SRC_SEL,
	AUDCLK_TOP_AUD_K1_SRC_DIV,
	AUDCLK_TOP_AUD_K2_SRC_DIV,
	AUDCLK_TOP_AUD_K3_SRC_DIV,
	AUDCLK_TOP_AUD_K4_SRC_DIV,
	AUDCLK_TOP_AUD_K5_SRC_DIV,
	AUDCLK_TOP_AUD_K6_SRC_DIV,
	AUDCLK_TOP_AUD_I2S1_MCLK,
	AUDCLK_TOP_AUD_I2S2_MCLK,
	AUDCLK_TOP_AUD_I2S3_MCLK,
	AUDCLK_TOP_AUD_I2S4_MCLK,
	AUDCLK_TOP_AUD_I2S5_MCLK,
	AUDCLK_TOP_AUD_I2S6_MCLK,
	AUDCLK_TOP_ASM_M_SEL,
	AUDCLK_TOP_ASM_H_SEL,
	AUDCLK_TOP_UNIVPLL2_D4,
	AUDCLK_TOP_UNIVPLL2_D2,
	AUDCLK_TOP_SYSPLL_D5,
	AUDCLK_TOP_UNIVPLL_D2,
	AUDCLK_TOP_INTDIR_SEL,
	CLOCK_NUM
};

struct audio_clock_attr {
	const char *name;
	const bool prepare_once;
	bool is_prepared;
	struct clk *clock;
};

static struct audio_clock_attr aud_clks[CLOCK_NUM] = {
	[AUDCLK_INFRA_SYS_AUDIO] = {"infra_sys_audio_clk", true, false, NULL},
	[AUDCLK_TOP_AUD_MUX1_SEL] = {"top_audio_mux1_sel", false, false, NULL},
	[AUDCLK_TOP_AUD_MUX2_SEL] = {"top_audio_mux2_sel", false, false, NULL},
	[AUDCLK_TOP_AUD_MUX1_DIV] = {"top_audio_mux1_div", false, false, NULL},
	[AUDCLK_TOP_AUD_MUX2_DIV] = {"top_audio_mux2_div", false, false, NULL},
	[AUDCLK_TOP_AUD_48K_TIMING] = {"top_audio_48k_timing", true,
				       false, NULL},
	[AUDCLK_TOP_AUD_44K_TIMING] = {"top_audio_44k_timing", true,
				       false, NULL},
	[AUDCLK_TOP_AUDPLL_MUX_SEL] = {"top_audpll_mux_sel", false,
				       false, NULL},
	[AUDCLK_TOP_APLL_SEL] = {"top_apll_sel", false, false, NULL},
	[AUDCLK_TOP_AUD1PLL_98M] = {"top_aud1_pll_98M", false, false, NULL},
	[AUDCLK_TOP_AUD2PLL_90M] = {"top_aud2_pll_90M", false, false, NULL},
	[AUDCLK_TOP_HADDS2PLL_98M] = {"top_hadds2_pll_98M", false, false, NULL},
	[AUDCLK_TOP_HADDS2PLL_294M] = {"top_hadds2_pll_294M", false,
				       false, NULL},
	[AUDCLK_TOP_AUDPLL] = {"top_audpll", false, false, NULL},
	[AUDCLK_TOP_AUDPLL_D4] = {"top_audpll_d4", false, false, NULL},
	[AUDCLK_TOP_AUDPLL_D8] = {"top_audpll_d8", false, false, NULL},
	[AUDCLK_TOP_AUDPLL_D16] = {"top_audpll_d16", false, false, NULL},
	[AUDCLK_TOP_AUDPLL_D24] = {"top_audpll_d24", false, false, NULL},
	[AUDCLK_TOP_AUDINTBUS] = {"top_audintbus_sel", false, false, NULL},
	[AUDCLK_CLK_26M] = {"clk_26m", false, false, NULL},
	[AUDCLK_TOP_SYSPLL1_D4] = {"top_syspll1_d4", false, false, NULL},
	[AUDCLK_TOP_AUD_K1_SRC_SEL] = {"top_aud_k1_src_sel", false,
				       false, NULL},
	[AUDCLK_TOP_AUD_K2_SRC_SEL] = {"top_aud_k2_src_sel", false,
				       false, NULL},
	[AUDCLK_TOP_AUD_K3_SRC_SEL] = {"top_aud_k3_src_sel", false,
				       false, NULL},
	[AUDCLK_TOP_AUD_K4_SRC_SEL] = {"top_aud_k4_src_sel", false,
				       false, NULL},
	[AUDCLK_TOP_AUD_K5_SRC_SEL] = {"top_aud_k5_src_sel", false,
				       false, NULL},
	[AUDCLK_TOP_AUD_K6_SRC_SEL] = {"top_aud_k6_src_sel", false,
				       false, NULL},
	[AUDCLK_TOP_AUD_K1_SRC_DIV] = {"top_aud_k1_src_div", false,
				       false, NULL},
	[AUDCLK_TOP_AUD_K2_SRC_DIV] = {"top_aud_k2_src_div", false,
				       false, NULL},
	[AUDCLK_TOP_AUD_K3_SRC_DIV] = {"top_aud_k3_src_div", false,
				       false, NULL},
	[AUDCLK_TOP_AUD_K4_SRC_DIV] = {"top_aud_k4_src_div", false,
				       false, NULL},
	[AUDCLK_TOP_AUD_K5_SRC_DIV] = {"top_aud_k5_src_div", false,
				       false, NULL},
	[AUDCLK_TOP_AUD_K6_SRC_DIV] = {"top_aud_k6_src_div", false,
				       false, NULL},
	[AUDCLK_TOP_AUD_I2S1_MCLK] = {"top_aud_i2s1_mclk", true, false, NULL},
	[AUDCLK_TOP_AUD_I2S2_MCLK] = {"top_aud_i2s2_mclk", true, false, NULL},
	[AUDCLK_TOP_AUD_I2S3_MCLK] = {"top_aud_i2s3_mclk", true, false, NULL},
	[AUDCLK_TOP_AUD_I2S4_MCLK] = {"top_aud_i2s4_mclk", true, false, NULL},
	[AUDCLK_TOP_AUD_I2S5_MCLK] = {"top_aud_i2s5_mclk", true, false, NULL},
	[AUDCLK_TOP_AUD_I2S6_MCLK] = {"top_aud_i2s6_mclk", true, false, NULL},
	[AUDCLK_TOP_ASM_M_SEL] = {"top_asm_m_sel", false, false, NULL},
	[AUDCLK_TOP_ASM_H_SEL] = {"top_asm_h_sel", false, false, NULL},
	[AUDCLK_TOP_UNIVPLL2_D4] = {"top_univpll2_d4", false, false, NULL},
	[AUDCLK_TOP_UNIVPLL2_D2] = {"top_univpll2_d2", false, false, NULL},
	[AUDCLK_TOP_SYSPLL_D5] = {"top_syspll_d5", false, false, NULL},
	[AUDCLK_TOP_UNIVPLL_D2] = {"top_univpll_d2", false, false, NULL},
	[AUDCLK_TOP_INTDIR_SEL] = {"top_intdir_sel", false, false, NULL},
};

int aud_a1sys_hp_ck_cntr;
int aud_a2sys_hp_ck_cntr;
int aud_f_apll_clk_cntr;
int aud_unipll_clk_cntr;
int aud_afe_clk_cntr;
int aud_i2s_clk_cntr;
int aud_spdif_dir_clk_cntr;

static DEFINE_MUTEX(afe_clk_mutex);

void mclk_configuration(int id, int domain, int mclk)
{
	int ret;
	/* Set MCLK Kx_SRC_SEL(domain) */
	ret =
	clk_prepare_enable(aud_clks[AUDCLK_TOP_AUD_K1_SRC_SEL + id].clock);
	if (ret)
		pr_err("%s clk_prepare_enable %s fail %d\n", __func__,
		       aud_clks[AUDCLK_TOP_AUD_K1_SRC_SEL + id].name, ret);

	if (domain == 0) {
		ret =
		clk_set_parent(aud_clks[AUDCLK_TOP_AUD_K1_SRC_SEL + id].clock,
			       aud_clks[AUDCLK_TOP_AUD_MUX1_SEL].clock);
		if (ret)
			pr_err("%s clk_set_parent %s-%s fail %d\n", __func__,
			       aud_clks[AUDCLK_TOP_AUD_K1_SRC_SEL + id].name,
			       aud_clks[AUDCLK_TOP_AUD_MUX1_SEL].name, ret);
	} else {
		ret =
		clk_set_parent(aud_clks[AUDCLK_TOP_AUD_K1_SRC_SEL + id].clock,
			       aud_clks[AUDCLK_TOP_AUD_MUX2_SEL].clock);
		if (ret)
			pr_err("%s clk_set_parent %s-%s fail %d\n", __func__,
			       aud_clks[AUDCLK_TOP_AUD_K1_SRC_SEL + id].name,
			       aud_clks[AUDCLK_TOP_AUD_MUX2_SEL].name, ret);
	}
	clk_disable_unprepare(aud_clks[AUDCLK_TOP_AUD_K1_SRC_SEL + id].clock);

	/* Set MCLK Kx_SRC_DIV(divider) */
	ret =
	clk_prepare_enable(aud_clks[AUDCLK_TOP_AUD_K1_SRC_DIV + id].clock);
	if (ret)
		pr_err("%s clk_prepare_enable %s fail %d\n",
		       __func__, aud_clks[AUDCLK_TOP_AUD_K1_SRC_DIV + id].name,
		       ret);

	ret = clk_set_rate(aud_clks[AUDCLK_TOP_AUD_K1_SRC_DIV + id].clock,
			   mclk);
	if (ret)
		pr_err("%s clk_set_rate %s-%d fail %d\n",
		       __func__, aud_clks[AUDCLK_TOP_AUD_K1_SRC_DIV + id].name,
		       mclk, ret);
	clk_disable_unprepare(aud_clks[AUDCLK_TOP_AUD_K1_SRC_DIV + id].clock);
}

void turn_on_i2sin_clock(int id, int on)
{
	int pdn = !on;

	if (id < 0 || id > 5) {
		pr_err("%s() error: i2s id %d\n", __func__, id);
		return;
	}
	/*MT_CG_AUDIO_I2SIN1,	AUDIO_TOP_CON4[0]*/
	/*MT_CG_AUDIO_I2SIN2,	AUDIO_TOP_CON4[1]*/
	/*MT_CG_AUDIO_I2SIN3,	AUDIO_TOP_CON4[2]*/
	/*MT_CG_AUDIO_I2SIN4,	AUDIO_TOP_CON4[3]*/
	/*MT_CG_AUDIO_I2SIN5,	AUDIO_TOP_CON4[4]*/
	/*MT_CG_AUDIO_I2SIN6,	AUDIO_TOP_CON4[5]*/
	 afe_msk_write(AUDIO_TOP_CON4, pdn << id, 1 << id);
}

void turn_on_i2sout_clock(int id, int on)
{
	int pdn = !on;

	pr_debug("%s\n", __func__);
	if (id < 0 || id > 5) {
		pr_err("%s() error: i2s id %d\n", __func__, id);
		return;
	}
	/*MT_CG_AUDIO_I2SO1,	AUDIO_TOP_CON4[6]*/
	/*MT_CG_AUDIO_I2SO2,	AUDIO_TOP_CON4[7]*/
	/*MT_CG_AUDIO_I2SO3,	AUDIO_TOP_CON4[8]*/
	/*MT_CG_AUDIO_I2SO4,	AUDIO_TOP_CON4[9]*/
	/*MT_CG_AUDIO_I2SO5,	AUDIO_TOP_CON4[10]*/
	/*MT_CG_AUDIO_I2SO6,	AUDIO_TOP_CON4[11]*/
	afe_msk_write(AUDIO_TOP_CON4, pdn << (id + 6), 1 << (id + 6));
}

void turn_on_afe_clock(void)
{
	int ret;

	/*MT_CG_INFRA_AUDIO, INFRA_PDN_STA[5]*/
	ret = clk_enable(aud_clks[AUDCLK_INFRA_SYS_AUDIO].clock);
	if (ret)
		pr_err("%s clk_enable %s fail %d\n",
		       __func__, aud_clks[AUDCLK_INFRA_SYS_AUDIO].name, ret);

	/* Set AUDCLK_TOP_AUDINTBUS to AUDCLK_TOP_SYSPLL1_D4 */
	ret = clk_prepare_enable(aud_clks[AUDCLK_TOP_AUDINTBUS].clock);
	if (ret)
		pr_err("%s clk_prepare_enable %s fail %d\n",
		       __func__, aud_clks[AUDCLK_TOP_AUDINTBUS].name, ret);

	ret = clk_set_parent(aud_clks[AUDCLK_TOP_AUDINTBUS].clock,
			     aud_clks[AUDCLK_TOP_SYSPLL1_D4].clock);
	if (ret)
		pr_err("%s clk_set_parent %s-%s fail %d\n",
		       __func__, aud_clks[AUDCLK_TOP_AUDINTBUS].name,
		       aud_clks[AUDCLK_TOP_SYSPLL1_D4].name, ret);

	/* Set AUDCLK_TOP_ASM_H_SEL to AUDCLK_TOP_UNIVPLL2_D2*/
	ret = clk_prepare_enable(aud_clks[AUDCLK_TOP_ASM_H_SEL].clock);
	if (ret)
		pr_err("%s clk_prepare_enable %s fail %d\n",
		       __func__, aud_clks[AUDCLK_TOP_ASM_H_SEL].name, ret);

	ret = clk_set_parent(aud_clks[AUDCLK_TOP_ASM_H_SEL].clock,
			     aud_clks[AUDCLK_TOP_UNIVPLL2_D2].clock);
	if (ret)
		pr_err("%s clk_set_parent %s-%s fail %d\n",
		       __func__, aud_clks[AUDCLK_TOP_ASM_H_SEL].name,
		       aud_clks[AUDCLK_TOP_UNIVPLL2_D2].name, ret);

	if (ret)
		pr_err("%s clk_enable %s fail %d\n",
		       __func__, aud_clks[AUDCLK_TOP_ASM_H_SEL].name, ret);

	/* Set AUDCLK_TOP_ASM_M_SEL to AUDCLK_TOP_UNIVPLL2_D4*/
	ret = clk_prepare_enable(aud_clks[AUDCLK_TOP_ASM_M_SEL].clock);
	if (ret)
		pr_err("%s clk_prepare_enable %s fail %d\n",
		       __func__, aud_clks[AUDCLK_TOP_ASM_M_SEL].name, ret);

	ret = clk_set_parent(aud_clks[AUDCLK_TOP_ASM_M_SEL].clock,
			     aud_clks[AUDCLK_TOP_UNIVPLL2_D4].clock);
	if (ret)
		pr_err("%s clk_set_parent %s-%s fail %d\n",
		       __func__, aud_clks[AUDCLK_TOP_ASM_M_SEL].name,
		       aud_clks[AUDCLK_TOP_UNIVPLL2_D4].name, ret);

	/*MT_CG_AUDIO_AFE,		AUDIO_TOP_CON0[2]*/
	afe_msk_write(AUDIO_TOP_CON0, 0, PDN_AFE);
	/*MT_CG_AUDIO_APLL,		AUDIO_TOP_CON0[23]*/
	afe_msk_write(AUDIO_TOP_CON0, 0, PDN_APLL_CK);
	/*MT_CG_AUDIO_A1SYS,		AUDIO_TOP_CON4[21]*/
	afe_msk_write(AUDIO_TOP_CON4, 0, PDN_A1SYS);
	/*MT_CG_AUDIO_A2SYS,		AUDIO_TOP_CON4[22]*/
	afe_msk_write(AUDIO_TOP_CON4, 0, PDN_A2SYS);
	/*MT_CG_AUDIO_AFE_CONN,	AUDIO_TOP_CON4[23]*/
	afe_msk_write(AUDIO_TOP_CON4, 0, PDN_AFE_CONN);
}

void turn_off_afe_clock(void)
{
	/*MT_CG_AUDIO_AFE,		AUDIO_TOP_CON0[2]*/
	afe_msk_write(AUDIO_TOP_CON0, PDN_AFE, PDN_AFE);
	/*MT_CG_AUDIO_APLL,		AUDIO_TOP_CON0[23]*/
	afe_msk_write(AUDIO_TOP_CON0, PDN_APLL_CK, PDN_APLL_CK);
	/*MT_CG_AUDIO_A1SYS,		AUDIO_TOP_CON4[21]*/
	afe_msk_write(AUDIO_TOP_CON4, PDN_A1SYS, PDN_A1SYS);
	/*MT_CG_AUDIO_A2SYS,		AUDIO_TOP_CON4[22]*/
	afe_msk_write(AUDIO_TOP_CON4, PDN_A2SYS, PDN_A2SYS);
	/*MT_CG_AUDIO_AFE_CONN,	AUDIO_TOP_CON4[23]*/
	afe_msk_write(AUDIO_TOP_CON4, PDN_AFE_CONN, PDN_AFE_CONN);

	clk_disable_unprepare(aud_clks[AUDCLK_TOP_ASM_M_SEL].clock);
	clk_disable_unprepare(aud_clks[AUDCLK_TOP_ASM_H_SEL].clock);
	clk_disable_unprepare(aud_clks[AUDCLK_TOP_AUDINTBUS].clock);
	/*MT_CG_INFRA_AUDIO,*/
	clk_disable(aud_clks[AUDCLK_INFRA_SYS_AUDIO].clock);
}

void turn_on_a1sys_hp_ck(void)
{
	int ret = 0;

	pr_debug("%s\n", __func__);
#ifdef COMMON_CLOCK_FRAMEWORK_API
	/* Set Mux */
	ret = clk_prepare_enable(aud_clks[AUDCLK_TOP_AUD_MUX1_SEL].clock);
	if (ret)
		pr_err("%s clk_prepare_enable %s fail %d\n",
		       __func__, aud_clks[AUDCLK_TOP_AUD_MUX1_SEL].name, ret);

	ret = clk_set_parent(aud_clks[AUDCLK_TOP_AUD_MUX1_SEL].clock,
			     aud_clks[AUDCLK_TOP_AUD1PLL_98M].clock);
	if (ret)
		pr_err("%s clk_set_parent %s-%s fail %d\n",
		       __func__, aud_clks[AUDCLK_TOP_AUD_MUX1_SEL].name,
		       aud_clks[AUDCLK_TOP_AUD1PLL_98M].name, ret);

	/* Set Divider */
	ret = clk_prepare_enable(aud_clks[AUDCLK_TOP_AUD_MUX1_DIV].clock);
	if (ret)
		pr_err("%s clk_prepare_enable %s fail %d\n",
		       __func__, aud_clks[AUDCLK_TOP_AUD_MUX1_DIV].name, ret);
	ret = clk_set_rate(aud_clks[AUDCLK_TOP_AUD_MUX1_DIV].clock,
			   98304000 / 2);
	if (ret)
		pr_err("%s clk_set_parent %s-%d fail %d\n",
		       __func__, aud_clks[AUDCLK_TOP_AUD_MUX1_DIV].name,
		       98304000 / 2, ret);

	/* Enable clock gate */
	ret = clk_enable(aud_clks[AUDCLK_TOP_AUD_48K_TIMING].clock);
	if (ret)
		pr_err("%s clk_enable %s fail %d\n",
		       __func__, aud_clks[AUDCLK_TOP_AUD_48K_TIMING].name, ret);
	/* Enable infra audio */
	ret = clk_enable(aud_clks[AUDCLK_INFRA_SYS_AUDIO].clock);
	if (ret)
		pr_err("%s clk_enable %s fail %d\n",
		       __func__, aud_clks[AUDCLK_INFRA_SYS_AUDIO].name, ret);
#endif
}

void turn_off_a1sys_hp_ck(void)
{
	pr_debug("%s\n", __func__);
#ifdef COMMON_CLOCK_FRAMEWORK_API
	clk_disable(aud_clks[AUDCLK_INFRA_SYS_AUDIO].clock);
	clk_disable(aud_clks[AUDCLK_TOP_AUD_48K_TIMING].clock);
	clk_disable_unprepare(aud_clks[AUDCLK_TOP_AUD_MUX1_DIV].clock);
	clk_disable_unprepare(aud_clks[AUDCLK_TOP_AUD_MUX1_SEL].clock);
#endif
}

void turn_on_a2sys_hp_ck(void)
{
	int ret = 0;

	pr_debug("%s\n", __func__);
#ifdef COMMON_CLOCK_FRAMEWORK_API

	/* Set Mux */
	ret = clk_prepare_enable(aud_clks[AUDCLK_TOP_AUD_MUX2_SEL].clock);
	if (ret)
		pr_err("%s clk_prepare_enable %s fail %d\n",
		       __func__, aud_clks[AUDCLK_TOP_AUD_MUX2_SEL].name, ret);

	ret = clk_set_parent(aud_clks[AUDCLK_TOP_AUD_MUX2_SEL].clock,
			     aud_clks[AUDCLK_TOP_AUD2PLL_90M].clock);
	if (ret)
		pr_err("%s clk_set_parent %s-%s fail %d\n",
		       __func__, aud_clks[AUDCLK_TOP_AUD_MUX2_SEL].name,
		aud_clks[AUDCLK_TOP_AUD2PLL_90M].name, ret);
	/* Set Divider */
	ret = clk_prepare_enable(aud_clks[AUDCLK_TOP_AUD_MUX2_DIV].clock);
	if (ret)
		pr_err("%s clk_prepare_enable %s fail %d\n",
		       __func__, aud_clks[AUDCLK_TOP_AUD_MUX2_DIV].name, ret);
	ret = clk_set_rate(aud_clks[AUDCLK_TOP_AUD_MUX2_DIV].clock,
			   90316800 / 2);
	if (ret)
		pr_err("%s clk_set_parent %s-%d fail %d\n",
		       __func__, aud_clks[AUDCLK_TOP_AUD_MUX2_DIV].name,
		       90316800 / 2, ret);

	/* Enable clock gate */
	ret = clk_enable(aud_clks[AUDCLK_TOP_AUD_44K_TIMING].clock);
	if (ret)
		pr_err("%s clk_enable %s fail %d\n",
		       __func__, aud_clks[AUDCLK_TOP_AUD_44K_TIMING].name, ret);
	/* Enable infra audio */
	ret = clk_enable(aud_clks[AUDCLK_INFRA_SYS_AUDIO].clock);
	if (ret)
		pr_err("%s clk_enable %s fail %d\n",
		       __func__, aud_clks[AUDCLK_INFRA_SYS_AUDIO].name, ret);
#endif
}

void turn_off_a2sys_hp_ck(void)
{
	pr_debug("%s\n", __func__);
#ifdef COMMON_CLOCK_FRAMEWORK_API
	clk_disable(aud_clks[AUDCLK_INFRA_SYS_AUDIO].clock);
	clk_disable(aud_clks[AUDCLK_TOP_AUD_44K_TIMING].clock);
	clk_disable_unprepare(aud_clks[AUDCLK_TOP_AUD_MUX2_DIV].clock);
	clk_disable_unprepare(aud_clks[AUDCLK_TOP_AUD_MUX2_SEL].clock);
#endif
}

void turn_on_f_apll_ck(unsigned int mux_select, unsigned int divider)
{
	int ret = 0;

	pr_debug("%s\n", __func__);
#ifdef COMMON_CLOCK_FRAMEWORK_API
	/*
	* Set AUDCLK_TOP_AUDPLL_MUX_SEL =
	* CLK_TOP_AUD1PLL_98M /CLK_TOP_AUD2PLL_90M/CLK_TOP_HADDS2PLL_98M /
	* CLK_TOP_HADDS2PLL_294M
	*/
	ret = clk_prepare_enable(aud_clks[AUDCLK_TOP_AUDPLL_MUX_SEL].clock);
	if (ret)
		pr_err("%s clk_prepare %s fail %d\n",
		       __func__, aud_clks[AUDCLK_TOP_AUDPLL_MUX_SEL].name, ret);

	ret =
	clk_set_parent(aud_clks[AUDCLK_TOP_AUDPLL_MUX_SEL].clock,
		       aud_clks[AUDCLK_TOP_AUD1PLL_98M + mux_select].clock);
	if (ret)
		pr_err("%s clk_set_parent %s-%s fail %d\n",
		       __func__, aud_clks[AUDCLK_TOP_AUDPLL_MUX_SEL].name,
		       aud_clks[AUDCLK_TOP_AUD1PLL_98M + mux_select].name, ret);

	/*
	* Set AUDCLK_TOP_APLL_SEL =
	* AUDCLK_TOP_AUDPLL /AUDCLK_TOP_AUDPLL_D4/AUDCLK_TOP_AUDPLL_D8 /
	* AUDCLK_TOP_AUDPLL_D16/AUDCLK_TOP_AUDPLL_D24
	*/
	ret = clk_prepare_enable(aud_clks[AUDCLK_TOP_APLL_SEL].clock);
	if (ret)
		pr_err("%s clk_prepare %s fail %d\n",
		       __func__, aud_clks[AUDCLK_TOP_APLL_SEL].name, ret);

	if (divider >= 1 && divider <= 5) {
		ret =
		clk_set_parent(aud_clks[AUDCLK_TOP_APLL_SEL].clock,
			       aud_clks[AUDCLK_TOP_AUDPLL + divider - 1].clock);
		if (ret)
			pr_err("%s clk_set_parent %s-%s fail %d\n",
			       __func__, aud_clks[AUDCLK_TOP_APLL_SEL].name,
			aud_clks[AUDCLK_TOP_AUDPLL + divider - 1].name, ret);
	} else {
		pr_err("%s bad divider %u\n", __func__, divider);
	}

	ret = clk_enable(aud_clks[AUDCLK_INFRA_SYS_AUDIO].clock);
	if (ret)
		pr_err("%s clk_enable %s fail %d\n",
		       __func__, aud_clks[AUDCLK_INFRA_SYS_AUDIO].name, ret);
#endif
}

void turn_off_f_apll_clock(void)
{
	pr_debug("%s\n", __func__);
#ifdef COMMON_CLOCK_FRAMEWORK_API
	clk_disable(aud_clks[AUDCLK_INFRA_SYS_AUDIO].clock);
	clk_disable_unprepare(aud_clks[AUDCLK_TOP_AUDPLL_MUX_SEL].clock);
	clk_disable_unprepare(aud_clks[AUDCLK_TOP_APLL_SEL].clock);
#endif
}

void turn_on_unipll_clock(void)
{
#ifdef COMMON_CLOCK_FRAMEWORK_API
	/* TODO*/
#endif
}

void turn_off_unipll_clock(void)
{
#ifdef COMMON_CLOCK_FRAMEWORK_API
	/* TODO*/
#endif
}

int mt_afe_init_clock(void *dev)
{
	int ret = 0;

#ifdef COMMON_CLOCK_FRAMEWORK_API
	unsigned int i;

	pr_debug("%s\n", __func__);

	for (i = 0; i < ARRAY_SIZE(aud_clks); i++) {
		aud_clks[i].clock = devm_clk_get(dev, aud_clks[i].name);
		if (IS_ERR(aud_clks[i].clock)) {
			ret = PTR_ERR(aud_clks[i].clock);
			pr_err("%s devm_clk_get %s fail %d\n",
			       __func__, aud_clks[i].name, ret);
			break;
		}
	}

	if (ret)
		return ret;

	for (i = 0; i < ARRAY_SIZE(aud_clks); i++) {
		if (aud_clks[i].prepare_once) {
			ret = clk_prepare(aud_clks[i].clock);
			if (ret) {
				pr_err("%s clk_prepare %s fail %d\n",
				       __func__, aud_clks[i].name, ret);
				break;
			}
			aud_clks[i].is_prepared = true;
		}
	}
#endif
	return ret;
}

void mt_afe_deinit_clock(void *dev)
{
#ifdef COMMON_CLOCK_FRAMEWORK_API
	size_t i;

	pr_debug("%s\n", __func__);
	for (i = 0; i < ARRAY_SIZE(aud_clks); i++) {
		if (aud_clks[i].clock && !IS_ERR(aud_clks[i].clock) &&
		    aud_clks[i].is_prepared) {
			clk_unprepare(aud_clks[i].clock);
			aud_clks[i].is_prepared = false;
		}
	}
#endif
}

int mt_i2s_power_on_mclk(int id, int on)
{
	int ret = 0;

	pr_debug("%s, id=%d\n", __func__, id);
	mutex_lock(&afe_clk_mutex);
#ifdef COMMON_CLOCK_FRAMEWORK_API
	/*afe_i2s_power_on_mclk(id, on);*/
	if (on) {
		ret = clk_enable(aud_clks[AUDCLK_TOP_AUD_I2S1_MCLK + id].clock);
		if (ret)
			pr_err("%s clk_enable %s fail %d\n", __func__,
			       aud_clks[AUDCLK_TOP_AUD_I2S1_MCLK + id].name,
			       ret);
	} else {
		clk_disable(aud_clks[AUDCLK_TOP_AUD_I2S1_MCLK + id].clock);
	}
#endif
	mutex_unlock(&afe_clk_mutex);
	return ret;
}

void mt_afe_a1sys_hp_ck_on(void)
{
	pr_debug("%s aud_a1sys_hp_ck_cntr:%d\n",
		 __func__, aud_a1sys_hp_ck_cntr);
	mutex_lock(&afe_clk_mutex);

	if (aud_a1sys_hp_ck_cntr == 0)
		turn_on_a1sys_hp_ck();
	aud_a1sys_hp_ck_cntr++;
	mutex_unlock(&afe_clk_mutex);
}

void mt_afe_a1sys_hp_ck_off(void)
{
	pr_debug("%s aud_a1sys_hp_ck_cntr(%d)\n",
		 __func__, aud_a1sys_hp_ck_cntr);
	mutex_lock(&afe_clk_mutex);

	aud_a1sys_hp_ck_cntr--;
	if (aud_a1sys_hp_ck_cntr == 0)
		turn_off_a1sys_hp_ck();
	if (aud_a1sys_hp_ck_cntr < 0) {
		pr_err("%s aud_a1sys_hp_ck_cntr:%d<0\n",
		       __func__, aud_a1sys_hp_ck_cntr);
		aud_a1sys_hp_ck_cntr = 0;
	}

	mutex_unlock(&afe_clk_mutex);
}

void mt_afe_a2sys_hp_ck_on(void)
{
	pr_debug("%s aud_a2sys_hp_ck_cntr:%d\n",
		 __func__, aud_a2sys_hp_ck_cntr);
	mutex_lock(&afe_clk_mutex);

	if (aud_a2sys_hp_ck_cntr == 0)
		turn_on_a2sys_hp_ck();

	aud_a2sys_hp_ck_cntr++;
	mutex_unlock(&afe_clk_mutex);
}

void mt_afe_a2sys_hp_ck_off(void)
{
	pr_debug("%s aud_a1sys_hp_ck_cntr(%d)\n",
		 __func__, aud_a2sys_hp_ck_cntr);
	mutex_lock(&afe_clk_mutex);

	aud_a2sys_hp_ck_cntr--;
	if (aud_a2sys_hp_ck_cntr == 0)
		turn_off_a2sys_hp_ck();

	if (aud_a2sys_hp_ck_cntr < 0) {
		pr_err("%s aud_a2sys_hp_ck_cntr:%d<0\n",
		       __func__, aud_a2sys_hp_ck_cntr);
		aud_a2sys_hp_ck_cntr = 0;
	}
	mutex_unlock(&afe_clk_mutex);
}

void mt_afe_f_apll_ck_on(unsigned int mux_select, unsigned int divider)
{
	pr_debug("%s aud_f_apll_clk_cntr:%d\n", __func__, aud_f_apll_clk_cntr);
	mutex_lock(&afe_clk_mutex);
	if (aud_f_apll_clk_cntr == 0)
		turn_on_f_apll_ck(mux_select, divider);

	aud_f_apll_clk_cntr++;
	mutex_unlock(&afe_clk_mutex);
}

void mt_afe_f_apll_ck_off(void)
{
	pr_debug("%s aud_f_apll_clk_cntr(%d)\n", __func__, aud_f_apll_clk_cntr);
	mutex_lock(&afe_clk_mutex);
	aud_f_apll_clk_cntr--;
	if (aud_f_apll_clk_cntr == 0)
		turn_off_f_apll_clock();
	if (aud_f_apll_clk_cntr < 0) {
		pr_err("%s aud_f_apll_clk_cntr:%d<0\n",
		       __func__, aud_f_apll_clk_cntr);
		aud_f_apll_clk_cntr = 0;
	}
	mutex_unlock(&afe_clk_mutex);
}

void turn_on_spdif_dir_ck(void)
{
	int ret = 0;

	pr_debug("%s\n", __func__);
#ifdef COMMON_CLOCK_FRAMEWORK_API
	ret = clk_prepare_enable(aud_clks[AUDCLK_TOP_INTDIR_SEL].clock);
	if (ret)
		pr_err("%s clk_prepare_enable %s fail %d\n",
		__func__, aud_clks[AUDCLK_TOP_INTDIR_SEL].name, ret);
	ret = clk_set_parent(aud_clks[AUDCLK_TOP_INTDIR_SEL].clock,
			     aud_clks[AUDCLK_TOP_UNIVPLL_D2].clock);
	if (ret)
		pr_err("%s clk_set_parent %s-%s fail %d\n", __func__,
		       aud_clks[AUDCLK_TOP_INTDIR_SEL].name,
		       aud_clks[AUDCLK_TOP_UNIVPLL_D2].name, ret);
#endif
}

void turn_off_spdif_dir_ck(void)
{
	pr_debug("%s\n", __func__);
#ifdef COMMON_CLOCK_FRAMEWORK_API
	clk_disable_unprepare(aud_clks[AUDCLK_TOP_INTDIR_SEL].clock);
#endif
}

void mt_afe_spdif_dir_clk_on(void)
{
	mutex_lock(&afe_clk_mutex);
	pr_debug("%s aud_spdif_dir_clk_cntr:%d\n", __func__, aud_unipll_clk_cntr);

	if (aud_spdif_dir_clk_cntr == 0)
		turn_on_spdif_dir_ck();

	aud_spdif_dir_clk_cntr++;
	mutex_unlock(&afe_clk_mutex);
}

void mt_afe_spdif_dir_clk_off(void)
{
	pr_debug("%s aud_spdif_dir_clk_cntr(%d)\n", __func__, aud_unipll_clk_cntr);
	mutex_lock(&afe_clk_mutex);

	aud_spdif_dir_clk_cntr--;
	if (aud_spdif_dir_clk_cntr == 0)
		turn_off_spdif_dir_ck();

	if (aud_spdif_dir_clk_cntr < 0) {
		pr_err("%s aud_spdif_dir_clk_cntr:%d<0\n",
		       __func__, aud_spdif_dir_clk_cntr);
		aud_spdif_dir_clk_cntr = 0;
	}
	mutex_unlock(&afe_clk_mutex);
}

void mt_afe_unipll_clk_on(void)
{
	mutex_lock(&afe_clk_mutex);
	pr_debug("%s aud_unipll_clk_cntr:%d\n", __func__, aud_unipll_clk_cntr);

	if (aud_unipll_clk_cntr == 0)
		turn_on_unipll_clock();

	aud_unipll_clk_cntr++;
	mutex_unlock(&afe_clk_mutex);
}

void mt_afe_unipll_clk_off(void)
{
	pr_debug("%s aud_unipll_clk_cntr(%d)\n", __func__, aud_unipll_clk_cntr);
	mutex_lock(&afe_clk_mutex);

	aud_unipll_clk_cntr--;
	if (aud_unipll_clk_cntr == 0)
		turn_off_unipll_clock();

	if (aud_unipll_clk_cntr < 0) {
		pr_err("%s aud_unipll_clk_cntr:%d<0\n",
		       __func__, aud_unipll_clk_cntr);
		aud_unipll_clk_cntr = 0;
	}
	mutex_unlock(&afe_clk_mutex);
}

void mt_afe_main_clk_on(void)
{
	pr_debug("%s aud_afe_clk_cntr:%d\n", __func__, aud_afe_clk_cntr);
	mutex_lock(&afe_clk_mutex);
	if (aud_afe_clk_cntr == 0)
		turn_on_afe_clock();

	aud_afe_clk_cntr++;

	mutex_unlock(&afe_clk_mutex);
}

void mt_afe_main_clk_off(void)
{
	pr_debug("%s aud_afe_clk_cntr:%d\n", __func__, aud_afe_clk_cntr);
	mutex_lock(&afe_clk_mutex);

	aud_afe_clk_cntr--;
	if (aud_afe_clk_cntr == 0) {
		turn_off_afe_clock();
	} else if (aud_afe_clk_cntr < 0) {
		pr_err("%s aud_afe_clk_cntr:%d<0\n",
		       __func__, aud_afe_clk_cntr);
		aud_afe_clk_cntr = 0;
	}

	mutex_unlock(&afe_clk_mutex);
}

void mt_turn_on_i2sout_clock(int id, int on)
{
	pr_debug("%s id:%d on:%d\n", __func__, id, on);
	turn_on_i2sout_clock(id, on);
}

void mt_turn_on_i2sin_clock(int id, int on)
{
	pr_debug("%s id:%d on:%d\n", __func__, id, on);
	turn_on_i2sin_clock(id, on);
}

void mt_mclk_set(int id, int domain, int mclk)
{
	pr_debug("%s id:%d domain:%d, mclk:%d\n", __func__, id, domain, mclk);
	mclk_configuration(id, domain, mclk);
}
