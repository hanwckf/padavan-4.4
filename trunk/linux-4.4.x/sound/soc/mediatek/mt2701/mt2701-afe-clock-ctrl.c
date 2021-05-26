/*
 * mt2701-afe-clock-ctrl.c  --  Mediatek 2701 afe clock ctrl
 *
 * Copyright (c) 2016 MediaTek Inc.
 * Author: Garlic Tseng <garlic.tseng@mediatek.com>
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

#include <sound/soc.h>
#include <linux/regmap.h>
#include <linux/pm_runtime.h>
#include <sound/pcm_params.h>

#include "mt2701-afe-common.h"
#include "mt2701-afe-clock-ctrl.h"

static const char *aud_clks[MT2701_CLOCK_NUM] = {
	[MT2701_AUD_INFRA_SYS_AUDIO] = "infra_sys_audio_clk",
	[MT2701_AUD_AUD_MUX1_SEL] = "top_audio_mux1_sel",
	[MT2701_AUD_AUD_MUX2_SEL] = "top_audio_mux2_sel",
	[MT2701_AUD_AUD_MUX1_DIV] = "top_audio_mux1_div",
	[MT2701_AUD_AUD_MUX2_DIV] = "top_audio_mux2_div",
	[MT2701_AUD_AUD_48K_TIMING] = "top_audio_48k_timing",
	[MT2701_AUD_AUD_44K_TIMING] = "top_audio_44k_timing",
	[MT2701_AUD_AUDPLL_MUX_SEL] = "top_audpll_mux_sel",
	[MT2701_AUD_APLL_SEL] = "top_apll_sel",
	[MT2701_AUD_AUD1PLL_98M] = "top_aud1_pll_98M",
	[MT2701_AUD_AUD2PLL_90M] = "top_aud2_pll_90M",
	[MT2701_AUD_HADDS2PLL_98M] = "top_hadds2_pll_98M",
	[MT2701_AUD_HADDS2PLL_294M] = "top_hadds2_pll_294M",
	[MT2701_AUD_AUDPLL] = "top_audpll",
	[MT2701_AUD_AUDPLL_D4] = "top_audpll_d4",
	[MT2701_AUD_AUDPLL_D8] = "top_audpll_d8",
	[MT2701_AUD_AUDPLL_D16] = "top_audpll_d16",
	[MT2701_AUD_AUDPLL_D24] = "top_audpll_d24",
	[MT2701_AUD_AUDINTBUS] = "top_audintbus_sel",
	[MT2701_AUD_CLK_26M] = "clk_26m",
	[MT2701_AUD_SYSPLL1_D4] = "top_syspll1_d4",
	[MT2701_AUD_AUD_K1_SRC_SEL] = "top_aud_k1_src_sel",
	[MT2701_AUD_AUD_K2_SRC_SEL] = "top_aud_k2_src_sel",
	[MT2701_AUD_AUD_K3_SRC_SEL] = "top_aud_k3_src_sel",
	[MT2701_AUD_AUD_K4_SRC_SEL] = "top_aud_k4_src_sel",
	[MT2701_AUD_AUD_K5_SRC_SEL] = "top_aud_k5_src_sel",
	[MT2701_AUD_AUD_K6_SRC_SEL] = "top_aud_k6_src_sel",
	[MT2701_AUD_AUD_K1_SRC_DIV] = "top_aud_k1_src_div",
	[MT2701_AUD_AUD_K2_SRC_DIV] = "top_aud_k2_src_div",
	[MT2701_AUD_AUD_K3_SRC_DIV] = "top_aud_k3_src_div",
	[MT2701_AUD_AUD_K4_SRC_DIV] = "top_aud_k4_src_div",
	[MT2701_AUD_AUD_K5_SRC_DIV] = "top_aud_k5_src_div",
	[MT2701_AUD_AUD_K6_SRC_DIV] = "top_aud_k6_src_div",
	[MT2701_AUD_AUD_I2S1_MCLK] = "top_aud_i2s1_mclk",
	[MT2701_AUD_AUD_I2S2_MCLK] = "top_aud_i2s2_mclk",
	[MT2701_AUD_AUD_I2S3_MCLK] = "top_aud_i2s3_mclk",
	[MT2701_AUD_AUD_I2S4_MCLK] = "top_aud_i2s4_mclk",
	[MT2701_AUD_AUD_I2S5_MCLK] = "top_aud_i2s5_mclk",
	[MT2701_AUD_AUD_I2S6_MCLK] = "top_aud_i2s6_mclk",
	[MT2701_AUD_ASM_M_SEL] = "top_asm_m_sel",
	[MT2701_AUD_ASM_H_SEL] = "top_asm_h_sel",
	[MT2701_AUD_UNIVPLL2_D4] = "top_univpll2_d4",
	[MT2701_AUD_UNIVPLL2_D2] = "top_univpll2_d2",
	[MT2701_AUD_SYSPLL_D5] = "top_syspll_d5",
};

static const char *aud_clks_2712[MT2712_CLOCK_NUM] = {
	[MT2712_AUD_AUD_INTBUS_SEL] = "aud_intbus_sel",
	[MT2712_AUD_AUD_APLL1_SEL] = "aud_apll1_sel",
	[MT2712_AUD_AUD_APLL2_SEL] = "aud_apll2_sel",
	[MT2712_AUD_A1SYS_HP_SEL] = "a1sys_hp_sel",
	[MT2712_AUD_A2SYS_HP_SEL] = "a2sys_hp_sel",
	[MT2712_AUD_APLL_SEL] = "apll_sel",
	[MT2712_AUD_APLL2_SEL] = "apll2_sel",
	[MT2712_AUD_I2SO1_SEL] = "i2so1_sel",
	[MT2712_AUD_I2SO2_SEL] = "i2so2_sel",
	[MT2712_AUD_I2SO3_SEL] = "i2so3_sel",
	[MT2712_AUD_TDMO0_SEL] = "tdmo0_sel",
	[MT2712_AUD_TDMO1_SEL] = "tdmo1_sel",
	[MT2712_AUD_I2SI1_SEL] = "i2si1_sel",
	[MT2712_AUD_I2SI2_SEL] = "i2si2_sel",
	[MT2712_AUD_I2SI3_SEL] = "i2si3_sel",
	[MT2712_AUD_APLL_DIV0] = "apll_div0",
	[MT2712_AUD_APLL_DIV1] = "apll_div1",
	[MT2712_AUD_APLL_DIV2] = "apll_div2",
	[MT2712_AUD_APLL_DIV3] = "apll_div3",
	[MT2712_AUD_APLL_DIV4] = "apll_div4",
	[MT2712_AUD_APLL_DIV5] = "apll_div5",
	[MT2712_AUD_APLL_DIV6] = "apll_div6",
	[MT2712_AUD_APLL_DIV7] = "apll_div7",
	[MT2712_AUD_DIV_PDN0] = "apll_div_pdn0",
	[MT2712_AUD_DIV_PDN1] = "apll_div_pdn1",
	[MT2712_AUD_DIV_PDN2] = "apll_div_pdn2",
	[MT2712_AUD_DIV_PDN3] = "apll_div_pdn3",
	[MT2712_AUD_DIV_PDN4] = "apll_div_pdn4",
	[MT2712_AUD_DIV_PDN5] = "apll_div_pdn5",
	[MT2712_AUD_DIV_PDN6] = "apll_div_pdn6",
	[MT2712_AUD_DIV_PDN7] = "apll_div_pdn7",
	[MT2712_AUD_APMIXED_APLL1] = "apll1",
	[MT2712_AUD_APMIXED_APLL2] = "apll2",
	[MT2712_AUD_EXT_I_1] = "clkaud_ext_i_1",
	[MT2712_AUD_EXT_I_2] = "clkaud_ext_i_2",
	[MT2712_AUD_CLK_26M] = "clk26m",
	[MT2712_AUD_SYSPLL1_D4] = "syspll1_d4",
	[MT2712_AUD_SYSPLL1_D2] = "syspll4_d2",
	[MT2712_AUD_UNIVPLL3_D2] = "univpll3_d2",
	[MT2712_AUD_UNIVPLL2_D8] = "univpll2_d8",
	[MT2712_AUD_SYSPLL3_D2] = "syspll3_d2",
	[MT2712_AUD_SYSPLL3_D4] = "syspll3_d4",
	[MT2712_AUD_APLL1] = "apll1_ck",
	[MT2712_AUD_APLL1_D2] = "apll1_d2",
	[MT2712_AUD_APLL1_D4] = "apll1_d4",
	[MT2712_AUD_APLL1_D8] = "apll1_d8",
	[MT2712_AUD_APLL1_D16] = "apll1_d16",
	[MT2712_AUD_APLL2] = "apll2_ck",
	[MT2712_AUD_APLL2_D2] = "apll2_d2",
	[MT2712_AUD_APLL2_D4] = "apll2_d4",
	[MT2712_AUD_APLL2_D8] = "apll2_d8",
	[MT2712_AUD_APLL2_D16] = "apll2_d16",
};


int mt2701_init_clock(struct mtk_base_afe *afe)
{
	struct mt2701_afe_private *afe_priv = afe->platform_priv;
	int i = 0;

	for (i = 0; i < MT2701_CLOCK_NUM; i++) {
		afe_priv->clocks[i] = devm_clk_get(afe->dev, aud_clks[i]);
		if (IS_ERR(aud_clks[i])) {
			dev_warn(afe->dev, "%s devm_clk_get %s fail\n",
				 __func__, aud_clks[i]);
			return PTR_ERR(aud_clks[i]);
		}
	}

	return 0;
}

int mt2701_afe_enable_clock(struct mtk_base_afe *afe)
{
	int ret = 0;

	ret = mt2701_turn_on_a1sys_clock(afe);
	if (ret) {
		dev_err(afe->dev, "%s turn_on_a1sys_clock fail %d\n",
			__func__, ret);
		return ret;
	}

	ret = mt2701_turn_on_a2sys_clock(afe);
	if (ret) {
		dev_err(afe->dev, "%s turn_on_a2sys_clock fail %d\n",
			__func__, ret);
		mt2701_turn_off_a1sys_clock(afe);
		return ret;
	}

	ret = mt2701_turn_on_afe_clock(afe);
	if (ret) {
		dev_err(afe->dev, "%s turn_on_afe_clock fail %d\n",
			__func__, ret);
		mt2701_turn_off_a1sys_clock(afe);
		mt2701_turn_off_a2sys_clock(afe);
		return ret;
	}

	regmap_update_bits(afe->regmap, ASYS_TOP_CON,
			   AUDIO_TOP_CON0_A1SYS_A2SYS_ON,
			   AUDIO_TOP_CON0_A1SYS_A2SYS_ON);
	regmap_update_bits(afe->regmap, AFE_DAC_CON0,
			   AFE_DAC_CON0_AFE_ON,
			   AFE_DAC_CON0_AFE_ON);
	regmap_write(afe->regmap, PWR2_TOP_CON,
		     PWR2_TOP_CON_INIT_VAL);
	regmap_write(afe->regmap, PWR1_ASM_CON1,
		     PWR1_ASM_CON1_INIT_VAL);
	regmap_write(afe->regmap, PWR2_ASM_CON1,
		     PWR2_ASM_CON1_INIT_VAL);

	return 0;
}

void mt2701_afe_disable_clock(struct mtk_base_afe *afe)
{
	mt2701_turn_off_afe_clock(afe);
	mt2701_turn_off_a1sys_clock(afe);
	mt2701_turn_off_a2sys_clock(afe);

	regmap_update_bits(afe->regmap, ASYS_TOP_CON,
			   AUDIO_TOP_CON0_A1SYS_A2SYS_ON, 0);
	regmap_update_bits(afe->regmap, AFE_DAC_CON0,
			   AFE_DAC_CON0_AFE_ON, 0);
}

int mt2701_turn_on_a1sys_clock(struct mtk_base_afe *afe)
{
	struct mt2701_afe_private *afe_priv = afe->platform_priv;
	int ret = 0;

	/* Set Mux */
	ret = clk_prepare_enable(afe_priv->clocks[MT2701_AUD_AUD_MUX1_SEL]);
	if (ret) {
		dev_err(afe->dev, "%s clk_prepare_enable %s fail %d\n",
			__func__, aud_clks[MT2701_AUD_AUD_MUX1_SEL], ret);
		goto A1SYS_CLK_AUD_MUX1_SEL_ERR;
	}

	ret = clk_set_parent(afe_priv->clocks[MT2701_AUD_AUD_MUX1_SEL],
			     afe_priv->clocks[MT2701_AUD_AUD1PLL_98M]);
	if (ret) {
		dev_err(afe->dev, "%s clk_set_parent %s-%s fail %d\n", __func__,
			aud_clks[MT2701_AUD_AUD_MUX1_SEL],
			aud_clks[MT2701_AUD_AUD1PLL_98M], ret);
		goto A1SYS_CLK_AUD_MUX1_SEL_ERR;
	}

	/* Set Divider */
	ret = clk_prepare_enable(afe_priv->clocks[MT2701_AUD_AUD_MUX1_DIV]);
	if (ret) {
		dev_err(afe->dev, "%s clk_prepare_enable %s fail %d\n",
			__func__,
			aud_clks[MT2701_AUD_AUD_MUX1_DIV],
			ret);
		goto A1SYS_CLK_AUD_MUX1_DIV_ERR;
	}

	ret = clk_set_rate(afe_priv->clocks[MT2701_AUD_AUD_MUX1_DIV],
			   MT2701_AUD_AUD_MUX1_DIV_RATE);
	if (ret) {
		dev_err(afe->dev, "%s clk_set_parent %s-%d fail %d\n", __func__,
			aud_clks[MT2701_AUD_AUD_MUX1_DIV],
			MT2701_AUD_AUD_MUX1_DIV_RATE, ret);
		goto A1SYS_CLK_AUD_MUX1_DIV_ERR;
	}

	/* Enable clock gate */
	ret = clk_prepare_enable(afe_priv->clocks[MT2701_AUD_AUD_48K_TIMING]);
	if (ret) {
		dev_err(afe->dev, "%s clk_prepare_enable %s fail %d\n",
			__func__, aud_clks[MT2701_AUD_AUD_48K_TIMING], ret);
		goto A1SYS_CLK_AUD_48K_ERR;
	}

	/* Enable infra audio */
	ret = clk_prepare_enable(afe_priv->clocks[MT2701_AUD_INFRA_SYS_AUDIO]);
	if (ret) {
		dev_err(afe->dev, "%s clk_prepare_enable %s fail %d\n",
			__func__, aud_clks[MT2701_AUD_INFRA_SYS_AUDIO], ret);
		goto A1SYS_CLK_INFRA_ERR;
	}

	return 0;

A1SYS_CLK_INFRA_ERR:
	clk_disable_unprepare(afe_priv->clocks[MT2701_AUD_INFRA_SYS_AUDIO]);
A1SYS_CLK_AUD_48K_ERR:
	clk_disable_unprepare(afe_priv->clocks[MT2701_AUD_AUD_48K_TIMING]);
A1SYS_CLK_AUD_MUX1_DIV_ERR:
	clk_disable_unprepare(afe_priv->clocks[MT2701_AUD_AUD_MUX1_DIV]);
A1SYS_CLK_AUD_MUX1_SEL_ERR:
	clk_disable_unprepare(afe_priv->clocks[MT2701_AUD_AUD_MUX1_SEL]);

	return ret;
}

void mt2701_turn_off_a1sys_clock(struct mtk_base_afe *afe)
{
	struct mt2701_afe_private *afe_priv = afe->platform_priv;

	clk_disable_unprepare(afe_priv->clocks[MT2701_AUD_INFRA_SYS_AUDIO]);
	clk_disable_unprepare(afe_priv->clocks[MT2701_AUD_AUD_48K_TIMING]);
	clk_disable_unprepare(afe_priv->clocks[MT2701_AUD_AUD_MUX1_DIV]);
	clk_disable_unprepare(afe_priv->clocks[MT2701_AUD_AUD_MUX1_SEL]);
}

int mt2701_turn_on_a2sys_clock(struct mtk_base_afe *afe)
{
	struct mt2701_afe_private *afe_priv = afe->platform_priv;
	int ret = 0;

	/* Set Mux */
	ret = clk_prepare_enable(afe_priv->clocks[MT2701_AUD_AUD_MUX2_SEL]);
	if (ret) {
		dev_err(afe->dev, "%s clk_prepare_enable %s fail %d\n",
			__func__, aud_clks[MT2701_AUD_AUD_MUX2_SEL], ret);
		goto A2SYS_CLK_AUD_MUX2_SEL_ERR;
	}

	ret = clk_set_parent(afe_priv->clocks[MT2701_AUD_AUD_MUX2_SEL],
			     afe_priv->clocks[MT2701_AUD_AUD2PLL_90M]);
	if (ret) {
		dev_err(afe->dev, "%s clk_set_parent %s-%s fail %d\n", __func__,
			aud_clks[MT2701_AUD_AUD_MUX2_SEL],
			aud_clks[MT2701_AUD_AUD2PLL_90M], ret);
		goto A2SYS_CLK_AUD_MUX2_SEL_ERR;
	}

	/* Set Divider */
	ret = clk_prepare_enable(afe_priv->clocks[MT2701_AUD_AUD_MUX2_DIV]);
	if (ret) {
		dev_err(afe->dev, "%s clk_prepare_enable %s fail %d\n",
			__func__, aud_clks[MT2701_AUD_AUD_MUX2_DIV], ret);
		goto A2SYS_CLK_AUD_MUX2_DIV_ERR;
	}

	ret = clk_set_rate(afe_priv->clocks[MT2701_AUD_AUD_MUX2_DIV],
			   MT2701_AUD_AUD_MUX2_DIV_RATE);
	if (ret) {
		dev_err(afe->dev, "%s clk_set_parent %s-%d fail %d\n", __func__,
			aud_clks[MT2701_AUD_AUD_MUX2_DIV],
			MT2701_AUD_AUD_MUX2_DIV_RATE, ret);
		goto A2SYS_CLK_AUD_MUX2_DIV_ERR;
	}

	/* Enable clock gate */
	ret = clk_prepare_enable(afe_priv->clocks[MT2701_AUD_AUD_44K_TIMING]);
	if (ret) {
		dev_err(afe->dev, "%s clk_prepare_enable %s fail %d\n",
			__func__, aud_clks[MT2701_AUD_AUD_44K_TIMING], ret);
		goto A2SYS_CLK_AUD_44K_ERR;
	}

	/* Enable infra audio */
	ret = clk_prepare_enable(afe_priv->clocks[MT2701_AUD_INFRA_SYS_AUDIO]);
	if (ret) {
		dev_err(afe->dev, "%s clk_prepare_enable %s fail %d\n",
			__func__, aud_clks[MT2701_AUD_INFRA_SYS_AUDIO], ret);
		goto A2SYS_CLK_INFRA_ERR;
	}

	return 0;

A2SYS_CLK_INFRA_ERR:
	clk_disable_unprepare(afe_priv->clocks[MT2701_AUD_INFRA_SYS_AUDIO]);
A2SYS_CLK_AUD_44K_ERR:
	clk_disable_unprepare(afe_priv->clocks[MT2701_AUD_AUD_44K_TIMING]);
A2SYS_CLK_AUD_MUX2_DIV_ERR:
	clk_disable_unprepare(afe_priv->clocks[MT2701_AUD_AUD_MUX2_DIV]);
A2SYS_CLK_AUD_MUX2_SEL_ERR:
	clk_disable_unprepare(afe_priv->clocks[MT2701_AUD_AUD_MUX2_SEL]);

	return ret;
}

void mt2701_turn_off_a2sys_clock(struct mtk_base_afe *afe)
{
	struct mt2701_afe_private *afe_priv = afe->platform_priv;

	clk_disable_unprepare(afe_priv->clocks[MT2701_AUD_INFRA_SYS_AUDIO]);
	clk_disable_unprepare(afe_priv->clocks[MT2701_AUD_AUD_44K_TIMING]);
	clk_disable_unprepare(afe_priv->clocks[MT2701_AUD_AUD_MUX2_DIV]);
	clk_disable_unprepare(afe_priv->clocks[MT2701_AUD_AUD_MUX2_SEL]);
}

int mt2701_turn_on_afe_clock(struct mtk_base_afe *afe)
{
	struct mt2701_afe_private *afe_priv = afe->platform_priv;
	int ret;

	/* enable INFRA_SYS */
	ret = clk_prepare_enable(afe_priv->clocks[MT2701_AUD_INFRA_SYS_AUDIO]);
	if (ret) {
		dev_err(afe->dev, "%s clk_prepare_enable %s fail %d\n",
			__func__, aud_clks[MT2701_AUD_INFRA_SYS_AUDIO], ret);
		goto AFE_AUD_INFRA_ERR;
	}

	/* Set MT2701_AUD_AUDINTBUS to MT2701_AUD_SYSPLL1_D4 */
	ret = clk_prepare_enable(afe_priv->clocks[MT2701_AUD_AUDINTBUS]);
	if (ret) {
		dev_err(afe->dev, "%s clk_prepare_enable %s fail %d\n",
			__func__, aud_clks[MT2701_AUD_AUDINTBUS], ret);
		goto AFE_AUD_AUDINTBUS_ERR;
	}

	ret = clk_set_parent(afe_priv->clocks[MT2701_AUD_AUDINTBUS],
			     afe_priv->clocks[MT2701_AUD_SYSPLL1_D4]);
	if (ret) {
		dev_err(afe->dev, "%s clk_set_parent %s-%s fail %d\n", __func__,
			aud_clks[MT2701_AUD_AUDINTBUS],
			aud_clks[MT2701_AUD_SYSPLL1_D4], ret);
		goto AFE_AUD_AUDINTBUS_ERR;
	}

	/* Set MT2701_AUD_ASM_H_SEL to MT2701_AUD_UNIVPLL2_D2 */
	ret = clk_prepare_enable(afe_priv->clocks[MT2701_AUD_ASM_H_SEL]);
	if (ret) {
		dev_err(afe->dev, "%s clk_prepare_enable %s fail %d\n",
			__func__, aud_clks[MT2701_AUD_ASM_H_SEL], ret);
		goto AFE_AUD_ASM_H_ERR;
	}

	ret = clk_set_parent(afe_priv->clocks[MT2701_AUD_ASM_H_SEL],
			     afe_priv->clocks[MT2701_AUD_UNIVPLL2_D2]);
	if (ret) {
		dev_err(afe->dev, "%s clk_set_parent %s-%s fail %d\n", __func__,
			aud_clks[MT2701_AUD_ASM_H_SEL],
			aud_clks[MT2701_AUD_UNIVPLL2_D2], ret);
		goto AFE_AUD_ASM_H_ERR;
	}

	/* Set MT2701_AUD_ASM_M_SEL to MT2701_AUD_UNIVPLL2_D4 */
	ret = clk_prepare_enable(afe_priv->clocks[MT2701_AUD_ASM_M_SEL]);
	if (ret) {
		dev_err(afe->dev, "%s clk_prepare_enable %s fail %d\n",
			__func__, aud_clks[MT2701_AUD_ASM_M_SEL], ret);
		goto AFE_AUD_ASM_M_ERR;
	}

	ret = clk_set_parent(afe_priv->clocks[MT2701_AUD_ASM_M_SEL],
			     afe_priv->clocks[MT2701_AUD_UNIVPLL2_D4]);
	if (ret) {
		dev_err(afe->dev, "%s clk_set_parent %s-%s fail %d\n", __func__,
			aud_clks[MT2701_AUD_ASM_M_SEL],
			aud_clks[MT2701_AUD_UNIVPLL2_D4], ret);
		goto AFE_AUD_ASM_M_ERR;
	}

	regmap_update_bits(afe->regmap, AUDIO_TOP_CON0,
			   AUDIO_TOP_CON0_PDN_AFE, 0);
	regmap_update_bits(afe->regmap, AUDIO_TOP_CON0,
			   AUDIO_TOP_CON0_PDN_APLL_CK, 0);
	regmap_update_bits(afe->regmap, AUDIO_TOP_CON4,
			   AUDIO_TOP_CON4_PDN_A1SYS, 0);
	regmap_update_bits(afe->regmap, AUDIO_TOP_CON4,
			   AUDIO_TOP_CON4_PDN_A2SYS, 0);
	regmap_update_bits(afe->regmap, AUDIO_TOP_CON4,
			   AUDIO_TOP_CON4_PDN_AFE_CONN, 0);

	return 0;

AFE_AUD_ASM_M_ERR:
	clk_disable_unprepare(afe_priv->clocks[MT2701_AUD_ASM_M_SEL]);
AFE_AUD_ASM_H_ERR:
	clk_disable_unprepare(afe_priv->clocks[MT2701_AUD_ASM_H_SEL]);
AFE_AUD_AUDINTBUS_ERR:
	clk_disable_unprepare(afe_priv->clocks[MT2701_AUD_AUDINTBUS]);
AFE_AUD_INFRA_ERR:
	clk_disable_unprepare(afe_priv->clocks[MT2701_AUD_INFRA_SYS_AUDIO]);

	return ret;
}

void mt2701_turn_off_afe_clock(struct mtk_base_afe *afe)
{
	struct mt2701_afe_private *afe_priv = afe->platform_priv;

	clk_disable_unprepare(afe_priv->clocks[MT2701_AUD_INFRA_SYS_AUDIO]);

	clk_disable_unprepare(afe_priv->clocks[MT2701_AUD_AUDINTBUS]);
	clk_disable_unprepare(afe_priv->clocks[MT2701_AUD_ASM_H_SEL]);
	clk_disable_unprepare(afe_priv->clocks[MT2701_AUD_ASM_M_SEL]);

	regmap_update_bits(afe->regmap, AUDIO_TOP_CON0,
			   AUDIO_TOP_CON0_PDN_AFE, AUDIO_TOP_CON0_PDN_AFE);
	regmap_update_bits(afe->regmap, AUDIO_TOP_CON0,
			   AUDIO_TOP_CON0_PDN_APLL_CK,
			   AUDIO_TOP_CON0_PDN_APLL_CK);
	regmap_update_bits(afe->regmap, AUDIO_TOP_CON4,
			   AUDIO_TOP_CON4_PDN_A1SYS,
			   AUDIO_TOP_CON4_PDN_A1SYS);
	regmap_update_bits(afe->regmap, AUDIO_TOP_CON4,
			   AUDIO_TOP_CON4_PDN_A2SYS,
			   AUDIO_TOP_CON4_PDN_A2SYS);
	regmap_update_bits(afe->regmap, AUDIO_TOP_CON4,
			   AUDIO_TOP_CON4_PDN_AFE_CONN,
			   AUDIO_TOP_CON4_PDN_AFE_CONN);
}

void mt2701_mclk_configuration(struct mtk_base_afe *afe, int id, int domain,
			       int mclk)
{
	struct mt2701_afe_private *afe_priv = afe->platform_priv;
	int ret;
	int aud_src_div_id = MT2701_AUD_AUD_K1_SRC_DIV + id;
	int aud_src_clk_id = MT2701_AUD_AUD_K1_SRC_SEL + id;
	int aud_src_en_id = MT2701_AUD_AUD_I2S1_MCLK + id;

	/* Set MCLK Kx_SRC_SEL(domain) */
	ret = clk_prepare_enable(afe_priv->clocks[aud_src_clk_id]);
	if (ret)
		dev_err(afe->dev, "%s clk_prepare_enable %s fail %d\n",
			__func__, aud_clks[aud_src_clk_id], ret);

	if (domain == 0) {
		ret = clk_set_parent(afe_priv->clocks[aud_src_clk_id],
				     afe_priv->clocks[MT2701_AUD_AUD_MUX1_SEL]);
		if (ret)
			dev_err(afe->dev, "%s clk_set_parent %s-%s fail %d\n",
				__func__, aud_clks[aud_src_clk_id],
				aud_clks[MT2701_AUD_AUD_MUX1_SEL], ret);
	} else {
		ret = clk_set_parent(afe_priv->clocks[aud_src_clk_id],
				     afe_priv->clocks[MT2701_AUD_AUD_MUX2_SEL]);
		if (ret)
			dev_err(afe->dev, "%s clk_set_parent %s-%s fail %d\n",
				__func__, aud_clks[aud_src_clk_id],
				aud_clks[MT2701_AUD_AUD_MUX2_SEL], ret);
	}
	clk_disable_unprepare(afe_priv->clocks[aud_src_clk_id]);

	/* Set MCLK Kx_SRC_DIV(divider) */
	ret = clk_prepare_enable(afe_priv->clocks[aud_src_div_id]);
	if (ret)
		dev_err(afe->dev, "%s clk_prepare_enable %s fail %d\n",
			__func__, aud_clks[aud_src_div_id], ret);

	ret = clk_set_rate(afe_priv->clocks[aud_src_div_id], mclk);
	if (ret)
		dev_err(afe->dev, "%s clk_set_rate %s-%d fail %d\n", __func__,
			aud_clks[aud_src_div_id], mclk, ret);
	clk_disable_unprepare(afe_priv->clocks[aud_src_div_id]);

	/* enable mclk */
	ret = clk_prepare_enable(afe_priv->clocks[aud_src_en_id]);
	if (ret)
		dev_err(afe->dev, "Failed to enable mclk for I2S: %d\n",
			id);
}

void mt2701_turn_off_mclk(struct mtk_base_afe *afe, int id)
{
	struct mt2701_afe_private *afe_priv = afe->platform_priv;

	clk_disable_unprepare(afe_priv->clocks[MT2701_AUD_AUD_I2S1_MCLK + id]);
}


int mt2712_init_clock(struct mtk_base_afe *afe)
{
	struct mt2701_afe_private *afe_priv = afe->platform_priv;
	int i = 0;

	for (i = 0; i < MT2712_CLOCK_NUM; i++) {
		afe_priv->clocks[i] = devm_clk_get(afe->dev, aud_clks_2712[i]);
		if (IS_ERR(aud_clks_2712[i])) {
			dev_warn(afe->dev, "%s devm_clk_get %s fail\n",
				 __func__, aud_clks_2712[i]);
			return PTR_ERR(aud_clks_2712[i]);
		}
	}

	return 0;
}

int mt2712_afe_enable_clock(struct mtk_base_afe *afe)
{
	int ret = 0;

	ret = mt2712_turn_on_a1sys_clock(afe);
	if (ret) {
		dev_err(afe->dev, "%s turn_on_a1sys_clock fail %d\n",
			__func__, ret);
		return ret;
	}

	ret = mt2712_turn_on_a2sys_clock(afe);
	if (ret) {
		dev_err(afe->dev, "%s turn_on_a2sys_clock fail %d\n",
			__func__, ret);
		mt2712_turn_off_a1sys_clock(afe);
		return ret;
	}

	ret = mt2712_turn_on_afe_clock(afe);
	if (ret) {
		dev_err(afe->dev, "%s turn_on_afe_clock fail %d\n",
			__func__, ret);
		mt2712_turn_off_a1sys_clock(afe);
		mt2712_turn_off_a2sys_clock(afe);
		return ret;
	}

	ret = mt2712_turn_on_tdm_clock(afe);
	if (ret) {
		dev_err(afe->dev, "%s turn_on_afe_clock fail %d\n",
			__func__, ret);
		mt2712_turn_off_a1sys_clock(afe);
		mt2712_turn_off_a2sys_clock(afe);
		mt2712_turn_off_afe_clock(afe);
		return ret;
	}

	regmap_update_bits(afe->regmap, ASYS_TOP_CON,
			   AUDIO_TOP_CON0_A1SYS_A2SYS_ON,
			   AUDIO_TOP_CON0_A1SYS_A2SYS_ON);
	regmap_update_bits(afe->regmap, AFE_DAC_CON0,
			   AFE_DAC_CON0_AFE_ON,
			   AFE_DAC_CON0_AFE_ON);
	regmap_write(afe->regmap, PWR2_TOP_CON,
		     PWR2_TOP_CON_INIT_VAL);
	regmap_write(afe->regmap, PWR1_ASM_CON1,
		     PWR1_ASM_CON1_INIT_VAL);
	regmap_write(afe->regmap, PWR2_ASM_CON1,
		     PWR2_ASM_CON1_INIT_VAL);

	return 0;
}

void mt2712_afe_disable_clock(struct mtk_base_afe *afe)
{
	mt2712_turn_off_afe_clock(afe);
	mt2712_turn_off_a1sys_clock(afe);
	mt2712_turn_off_a2sys_clock(afe);
	mt2712_turn_off_tdm_clock(afe);

	regmap_update_bits(afe->regmap, ASYS_TOP_CON,
			   AUDIO_TOP_CON0_A1SYS_A2SYS_ON, 0);
	regmap_update_bits(afe->regmap, AFE_DAC_CON0,
			   AFE_DAC_CON0_AFE_ON, 0);
}

int mt2712_turn_on_a1sys_clock(struct mtk_base_afe *afe)
{
	struct mt2701_afe_private *afe_priv = afe->platform_priv;
	int ret = 0;

	/* Set Div Mux */
	ret = clk_prepare_enable(afe_priv->clocks[MT2712_AUD_A1SYS_HP_SEL]);
	if (ret) {
		dev_err(afe->dev, "%s clk_prepare_enable %s fail %d\n",
			__func__, aud_clks_2712[MT2712_AUD_A1SYS_HP_SEL], ret);
		goto A1SYS_CLK_AUD_HP_MUX1_SEL_ERR;
	}

	ret = clk_set_parent(afe_priv->clocks[MT2712_AUD_A1SYS_HP_SEL],
			     afe_priv->clocks[MT2712_AUD_APLL1_D4]);
	if (ret) {
		dev_err(afe->dev, "%s clk_set_parent %s-%s fail %d\n", __func__,
			aud_clks_2712[MT2712_AUD_A1SYS_HP_SEL],
			aud_clks_2712[MT2712_AUD_APLL1], ret);
		goto A1SYS_CLK_AUD_HP_MUX1_SEL_ERR;
	}

	/* Set Mux */
	ret = clk_prepare_enable(afe_priv->clocks[MT2712_AUD_AUD_APLL1_SEL]);
	if (ret) {
		dev_err(afe->dev, "%s clk_prepare_enable %s fail %d\n",
			 __func__, aud_clks_2712[MT2712_AUD_AUD_APLL1_SEL], ret);
		goto A1SYS_CLK_AUD_MUX1_SEL_ERR;
	}

	ret = clk_set_parent(afe_priv->clocks[MT2712_AUD_AUD_APLL1_SEL],
			afe_priv->clocks[MT2712_AUD_APMIXED_APLL1]);
	if (ret) {
		dev_err(afe->dev, "%s clk_set_parent %s-%s fail %d\n", __func__,
			aud_clks_2712[MT2712_AUD_AUD_APLL1_SEL],
			aud_clks_2712[MT2712_AUD_APMIXED_APLL1], ret);
		goto A1SYS_CLK_AUD_MUX1_SEL_ERR;
	}

	return 0;

A1SYS_CLK_AUD_MUX1_SEL_ERR:
	clk_disable_unprepare(afe_priv->clocks[MT2712_AUD_AUD_APLL1_SEL]);
A1SYS_CLK_AUD_HP_MUX1_SEL_ERR:
	clk_disable_unprepare(afe_priv->clocks[MT2712_AUD_A1SYS_HP_SEL]);

	return ret;
}

void mt2712_turn_off_a1sys_clock(struct mtk_base_afe *afe)
{
	struct mt2701_afe_private *afe_priv = afe->platform_priv;

	clk_disable_unprepare(afe_priv->clocks[MT2712_AUD_AUD_APLL1_SEL]);
	clk_disable_unprepare(afe_priv->clocks[MT2712_AUD_A1SYS_HP_SEL]);
}

int mt2712_turn_on_a2sys_clock(struct mtk_base_afe *afe)
{
	struct mt2701_afe_private *afe_priv = afe->platform_priv;
	int ret = 0;

	/* Set Div Mux */
	ret = clk_prepare_enable(afe_priv->clocks[MT2712_AUD_A2SYS_HP_SEL]);
	if (ret) {
		dev_err(afe->dev, "%s clk_prepare_enable %s fail %d\n",
			__func__, aud_clks_2712[MT2712_AUD_A2SYS_HP_SEL], ret);
		goto A2SYS_CLK_AUD_HP_MUX2_SEL_ERR;
	}

	ret = clk_set_parent(afe_priv->clocks[MT2712_AUD_A2SYS_HP_SEL],
			     afe_priv->clocks[MT2712_AUD_APLL2_D4]);
	if (ret) {
		dev_err(afe->dev, "%s clk_set_parent %s-%s fail %d\n", __func__,
			aud_clks_2712[MT2712_AUD_A2SYS_HP_SEL],
			aud_clks_2712[MT2712_AUD_APLL2], ret);
		goto A2SYS_CLK_AUD_HP_MUX2_SEL_ERR;
	}

	/* Set Mux */
	ret = clk_prepare_enable(afe_priv->clocks[MT2712_AUD_AUD_APLL2_SEL]);
	if (ret) {
		dev_err(afe->dev, "%s clk_prepare_enable %s fail %d\n",
			 __func__, aud_clks_2712[MT2712_AUD_AUD_APLL2_SEL], ret);
		goto A1SYS_CLK_AUD_MUX2_SEL_ERR;
	}

	ret = clk_set_parent(afe_priv->clocks[MT2712_AUD_AUD_APLL2_SEL],
			afe_priv->clocks[MT2712_AUD_APMIXED_APLL2]);
	if (ret) {
		dev_err(afe->dev, "%s clk_set_parent %s-%s fail %d\n", __func__,
			aud_clks_2712[MT2712_AUD_AUD_APLL2_SEL],
			aud_clks_2712[MT2712_AUD_APMIXED_APLL2], ret);
		goto A1SYS_CLK_AUD_MUX2_SEL_ERR;
	}

	return 0;

A1SYS_CLK_AUD_MUX2_SEL_ERR:
	clk_disable_unprepare(afe_priv->clocks[MT2712_AUD_AUD_APLL2_SEL]);
A2SYS_CLK_AUD_HP_MUX2_SEL_ERR:
	clk_disable_unprepare(afe_priv->clocks[MT2712_AUD_A2SYS_HP_SEL]);

	return ret;
}

void mt2712_turn_off_a2sys_clock(struct mtk_base_afe *afe)
{
	struct mt2701_afe_private *afe_priv = afe->platform_priv;

	clk_disable_unprepare(afe_priv->clocks[MT2712_AUD_AUD_APLL2_SEL]);
	clk_disable_unprepare(afe_priv->clocks[MT2712_AUD_A2SYS_HP_SEL]);
}

int mt2712_turn_on_afe_clock(struct mtk_base_afe *afe)
{
	struct mt2701_afe_private *afe_priv = afe->platform_priv;
	int ret = 0;

	/* Set MT2712_AUD_AUDINTBUS to MT2701_AUD_SYSPLL1_D4 */
	ret = clk_prepare_enable(afe_priv->clocks[MT2712_AUD_AUD_INTBUS_SEL]);
	if (ret) {
		dev_err(afe->dev, "%s clk_prepare_enable %s fail %d\n",
			__func__, aud_clks_2712[MT2712_AUD_AUD_INTBUS_SEL], ret);
		goto AFE_AUD_AUDINTBUS_ERR;
	}

	ret = clk_set_parent(afe_priv->clocks[MT2712_AUD_AUD_INTBUS_SEL],
			afe_priv->clocks[MT2712_AUD_SYSPLL1_D4]);
	if (ret) {
		dev_err(afe->dev, "%s clk_set_parent %s-%s fail %d\n", __func__,
			aud_clks_2712[MT2712_AUD_AUD_INTBUS_SEL],
			aud_clks_2712[MT2712_AUD_SYSPLL1_D4], ret);
		goto AFE_AUD_AUDINTBUS_ERR;
	}


	regmap_update_bits(afe->regmap, AUDIO_TOP_CON0,
			   AUDIO_TOP_CON0_PDN_AFE, 0);
	regmap_update_bits(afe->regmap, AUDIO_TOP_CON0,
			   AUDIO_TOP_CON0_PDN_APLL_CK, 0);
	regmap_update_bits(afe->regmap, AUDIO_TOP_CON4,
			   AUDIO_TOP_CON4_PDN_A1SYS, 0);
	regmap_update_bits(afe->regmap, AUDIO_TOP_CON4,
			   AUDIO_TOP_CON4_PDN_A2SYS, 0);
	regmap_update_bits(afe->regmap, AUDIO_TOP_CON4,
			   AUDIO_TOP_CON4_PDN_AFE_CONN, 0);

	return 0;
AFE_AUD_AUDINTBUS_ERR:
	clk_disable_unprepare(afe_priv->clocks[MT2712_AUD_AUD_INTBUS_SEL]);
	return ret;

}

void mt2712_turn_off_afe_clock(struct mtk_base_afe *afe)
{
	struct mt2701_afe_private *afe_priv = afe->platform_priv;

	clk_disable_unprepare(afe_priv->clocks[MT2712_AUD_AUD_INTBUS_SEL]);

	regmap_update_bits(afe->regmap, AUDIO_TOP_CON0,
			   AUDIO_TOP_CON0_PDN_AFE, AUDIO_TOP_CON0_PDN_AFE);
	regmap_update_bits(afe->regmap, AUDIO_TOP_CON0,
			   AUDIO_TOP_CON0_PDN_APLL_CK,
			   AUDIO_TOP_CON0_PDN_APLL_CK);
	regmap_update_bits(afe->regmap, AUDIO_TOP_CON4,
			   AUDIO_TOP_CON4_PDN_A1SYS,
			   AUDIO_TOP_CON4_PDN_A1SYS);
	regmap_update_bits(afe->regmap, AUDIO_TOP_CON4,
			   AUDIO_TOP_CON4_PDN_A2SYS,
			   AUDIO_TOP_CON4_PDN_A2SYS);
	regmap_update_bits(afe->regmap, AUDIO_TOP_CON4,
			   AUDIO_TOP_CON4_PDN_AFE_CONN,
			   AUDIO_TOP_CON4_PDN_AFE_CONN);
}

void mt2712_mclk_configuration(struct mtk_base_afe *afe, int id, int domain,
			       int mclk)
{
	struct mt2701_afe_private *afe_priv = afe->platform_priv;
	int ret;
	int aud_src_clk_id = MT2712_AUD_I2SO1_SEL + id;
	int aud_src_div_id = MT2712_AUD_APLL_DIV0 + id;
	int aud_src_pdn_id = MT2712_AUD_DIV_PDN0 + id;

	/* Set MCLK Kx_SRC_SEL(domain) */
	ret = clk_prepare_enable(afe_priv->clocks[aud_src_clk_id]);
	if (ret)
		dev_err(afe->dev, "%s clk_prepare_enable %s fail %d\n",
			__func__, aud_clks_2712[aud_src_clk_id], ret);

	if (domain == 0) {
		ret = clk_set_parent(afe_priv->clocks[aud_src_clk_id],
				     afe_priv->clocks[MT2712_AUD_APLL1]);
		if (ret)
			dev_err(afe->dev, "%s clk_set_parent %s-%s fail %d\n",
				__func__, aud_clks_2712[aud_src_clk_id],
				aud_clks_2712[MT2712_AUD_APLL1], ret);
	} else {
		ret = clk_set_parent(afe_priv->clocks[aud_src_clk_id],
				     afe_priv->clocks[MT2712_AUD_APLL2]);
		if (ret)
			dev_err(afe->dev, "%s clk_set_parent %s-%s fail %d\n",
				__func__, aud_clks[aud_src_clk_id],
				aud_clks_2712[MT2712_AUD_APLL2], ret);
	}

	/* Set MCLK Kx_SRC_DIV(divider) */
	ret = clk_prepare_enable(afe_priv->clocks[aud_src_div_id]);
	if (ret)
		dev_err(afe->dev, "%s clk_prepare_enable %s fail %d\n",
			__func__, aud_clks_2712[aud_src_div_id], ret);

	ret = clk_set_rate(afe_priv->clocks[aud_src_div_id], mclk);
	if (ret)
		dev_err(afe->dev, "%s clk_set_rate %s-%d fail %d\n", __func__,
			aud_clks_2712[aud_src_div_id], mclk, ret);

	ret = clk_prepare_enable(afe_priv->clocks[aud_src_pdn_id]);
	if (ret)
		dev_err(afe->dev, "%s clk_prepare_enable %s fail %d\n",
			__func__, aud_clks_2712[aud_src_pdn_id], ret);

}

void mt2712_turn_off_mclk(struct mtk_base_afe *afe, int id)
{
	struct mt2701_afe_private *afe_priv = afe->platform_priv;

	clk_disable_unprepare(afe_priv->clocks[MT2712_AUD_I2SO1_SEL + id]);
	clk_disable_unprepare(afe_priv->clocks[MT2712_AUD_APLL_DIV0 + id]);
	clk_disable_unprepare(afe_priv->clocks[MT2712_AUD_DIV_PDN0 + id]);
}


int mt2712_turn_on_tdm_clock(struct mtk_base_afe *afe)
{
	struct mt2701_afe_private *afe_priv = afe->platform_priv;
	int ret = 0;

	/* Set apll1 Mux */
	ret = clk_prepare_enable(afe_priv->clocks[MT2712_AUD_APLL_SEL]);
	if (ret) {
		dev_err(afe->dev, "%s clk_prepare_enable %s fail %d\n",
			__func__, aud_clks_2712[MT2712_AUD_APLL_SEL], ret);
		goto A2SYS_CLK_AUD_HP_MUX2_SEL_ERR;
	}

	ret = clk_set_parent(afe_priv->clocks[MT2712_AUD_APLL_SEL],
			     afe_priv->clocks[MT2712_AUD_APLL1]);
	if (ret) {
		dev_err(afe->dev, "%s clk_set_parent %s-%s fail %d\n", __func__,
			aud_clks_2712[MT2712_AUD_APLL_SEL],
			aud_clks_2712[MT2712_AUD_APLL1], ret);
		goto A2SYS_CLK_AUD_HP_MUX2_SEL_ERR;
	}

	/* Set apll2 Mux */
	ret = clk_prepare_enable(afe_priv->clocks[MT2712_AUD_APLL2_SEL]);
	if (ret) {
		dev_err(afe->dev, "%s clk_prepare_enable %s fail %d\n",
			 __func__, aud_clks_2712[MT2712_AUD_AUD_APLL2_SEL], ret);
		goto A1SYS_CLK_AUD_MUX2_SEL_ERR;
	}

	ret = clk_set_parent(afe_priv->clocks[MT2712_AUD_APLL2_SEL],
				     afe_priv->clocks[MT2712_AUD_APLL2]);
	if (ret) {
		dev_err(afe->dev, "%s clk_set_parent %s-%s fail %d\n", __func__,
			aud_clks_2712[MT2712_AUD_APLL2_SEL],
			aud_clks_2712[MT2712_AUD_APLL2], ret);
		goto A1SYS_CLK_AUD_MUX2_SEL_ERR;
	}

	return 0;

A1SYS_CLK_AUD_MUX2_SEL_ERR:
	clk_disable_unprepare(afe_priv->clocks[MT2712_AUD_APLL2_SEL]);
A2SYS_CLK_AUD_HP_MUX2_SEL_ERR:
	clk_disable_unprepare(afe_priv->clocks[MT2712_AUD_APLL_SEL]);

	return ret;
}
void mt2712_turn_off_tdm_clock(struct mtk_base_afe *afe)
{
	struct mt2701_afe_private *afe_priv = afe->platform_priv;

	clk_disable_unprepare(afe_priv->clocks[MT2712_AUD_APLL2_SEL]);
	clk_disable_unprepare(afe_priv->clocks[MT2712_AUD_APLL_SEL]);
}

int mt2701_tdm_clk_configuration(struct mtk_base_afe *afe, int tdm_id,
						struct snd_pcm_hw_params *params, bool coclk)
{
	struct mt2701_afe_private *afe_priv = (struct mt2701_afe_private *) afe->platform_priv;
	struct mt2701_tdm_data *tdm_data = (struct mt2701_tdm_data *)afe_priv->tdm_path[tdm_id].tdm_data;
	int apll_sel, apll_div;
	unsigned int fs_rate = params_rate(params);
	int channels = params_channels(params);
	int bit_width = params_width(params);
	int bck, mclk;
	int ret = 0;

	bck = fs_rate * channels * bit_width;
	mclk = afe_priv->tdm_path[tdm_id].mclk_rate;

	if (tdm_id == MT2701_TDMI || coclk) {
		if (bck > MT2701_TDM_IN_CLK_THR)
			return -1;
	} else {
		if (bck > MT2701_TDM_OUT_CLK_THR)
			return -1;
	}

	if (!(AUDIO_CLOCK_196M % bck)) {
		apll_sel = 1;
		apll_div = AUDIO_CLOCK_196M / bck;
	} else if (!(AUDIO_CLOCK_180M % bck)) {
		apll_sel = 2;
		apll_div = AUDIO_CLOCK_180M / bck;
	} else {
		return -1;
	}

	if (apll_div > MT2701_TDM_MAX_CLK_DIV) {
		/* set APLL divider 2!! need implement!! */
		apll_div = apll_div >> 1;
	}
	/* use I2SI0 mclk as TDMIN mclk */
	afe_priv->clk_ctrl->mclk_configuration(afe, MCLK_TDM_OFFSET + tdm_id, apll_sel-1, mclk);

	regmap_update_bits(afe->regmap, tdm_data->tdm_bck_reg, 1<<tdm_data->tdm_pll_sel_shift,
			(apll_sel-1)<<tdm_data->tdm_pll_sel_shift);
	regmap_update_bits(afe->regmap, tdm_data->tdm_bck_reg, 0xff<<tdm_data->tdm_plldiv_shift,
			(apll_div-1)<<tdm_data->tdm_plldiv_shift);
	return ret;

}


MODULE_DESCRIPTION("MT2701 afe clock control");
MODULE_AUTHOR("Garlic Tseng <garlic.tseng@mediatek.com>");
MODULE_LICENSE("GPL v2");
