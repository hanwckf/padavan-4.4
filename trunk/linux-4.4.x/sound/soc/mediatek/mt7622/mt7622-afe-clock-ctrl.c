/*
 * mt7622-afe-clock-ctrl.c  --  Mediatek 7622 afe clock ctrl
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

#include "mt7622-afe-common.h"
#include "mt7622-afe-clock-ctrl.h"

static const char *aud_clks[MT7622_CLOCK_NUM] = {
	[MT7622_AUD_CLK_INFRA_AUDIO_PD] = "infra_audio_pd",
	[MT7622_AUD_A1SYS_HP_SEL] = "top_a1sys_hp_sel",
	[MT7622_AUD_A2SYS_HP_SEL] = "top_a2sys_hp_sel",
	[MT7622_AUD_A1SYS_HP_DIV] = "top_a1sys_div",
	[MT7622_AUD_A2SYS_HP_DIV] = "top_a2sys_div",
	[MT7622_AUD_AUD1PLL] = "top_aud1pll_ck",
	[MT7622_AUD_AUD2PLL] = "top_aud2pll_ck",
	[MT7622_AUD_A1SYS_HP_DIV_PD] = "top_a1sys_div_pd",
	[MT7622_AUD_A2SYS_HP_DIV_PD] = "top_a2sys_div_pd",
	[MT7622_AUD_AUDINTBUS] = "top_aud_intbus_sel",
	[MT7622_AUD_SYSPLL1_D4] = "top_syspll1_d4",
	[MT7622_AUD_AUDINTDIR] = "top_intdir_sel",
	[MT7622_AUD_UNIVPLL_D2] = "top_univpll_d2",
	[MT7622_AUD_APLL1_SEL] = "top_apll1_ck_sel",
	[MT7622_AUD_APLL2_SEL] = "top_apll2_ck_sel",
	[MT7622_AUD_I2S0_MCK_SEL] = "top_i2s0_mck_sel",
	[MT7622_AUD_I2S1_MCK_SEL] = "top_i2s1_mck_sel",
	[MT7622_AUD_I2S2_MCK_SEL] = "top_i2s2_mck_sel",
	[MT7622_AUD_I2S3_MCK_SEL] = "top_i2s3_mck_sel",
	[MT7622_AUD_APLL1_DIV] = "top_apll1_ck_div",
	[MT7622_AUD_APLL2_DIV] = "top_apll2_ck_div",
	[MT7622_AUD_I2S0_MCK_DIV] = "top_i2s0_mck_div",
	[MT7622_AUD_I2S1_MCK_DIV] = "top_i2s1_mck_div",
	[MT7622_AUD_I2S2_MCK_DIV] = "top_i2s2_mck_div",
	[MT7622_AUD_I2S3_MCK_DIV] = "top_i2s3_mck_div",
	[MT7622_AUD_APLL1_DIV_PD] = "top_apll1_ck_div_pd",
	[MT7622_AUD_APLL2_DIV_PD] = "top_apll2_ck_div_pd",
	[MT7622_AUD_I2S0_MCK_DIV_PD] = "top_i2s0_mck_div_pd",
	[MT7622_AUD_I2S1_MCK_DIV_PD] = "top_i2s1_mck_div_pd",
	[MT7622_AUD_I2S2_MCK_DIV_PD] = "top_i2s2_mck_div_pd",
	[MT7622_AUD_I2S3_MCK_DIV_PD] = "top_i2s3_mck_div_pd",
	[MT7622_AUD_AUD1_SEL] = "top_aud1_sel",
	[MT7622_AUD_AUD2_SEL] = "top_aud2_sel",
	[MT7622_AUD_ASM_H_SEL] = "top_asm_h_sel",
	[MT7622_AUD_ASM_M_SEL] = "top_asm_m_sel",
	[MT7622_AUD_SYSPLL_D5] = "top_syspll_d5",
	[MT7622_AUD_UNIVPLL2_D2] = "top_univpll2_d2",
	[MT7622_AUD_AUDIO_AFE] = "top_audio_afe",
	[MT7622_AUD_AUDIO_APLL] = "top_audio_apll",
	[MT7622_AUD_AUDIO_A1SYS] = "top_audio_a1sys",
	[MT7622_AUD_AUDIO_A2SYS] = "top_audio_a2sys",

};

int mt7622_init_clock(struct mtk_base_afe *afe)
{
	struct mt7622_afe_private *afe_priv = afe->platform_priv;
	int i = 0;

	for (i = 0; i < MT7622_CLOCK_NUM; i++) {
		afe_priv->clocks[i] = devm_clk_get(afe->dev, aud_clks[i]);
		if (IS_ERR(aud_clks[i])) {
			dev_warn(afe->dev, "%s devm_clk_get %s fail\n",
				 __func__, aud_clks[i]);
			return PTR_ERR(aud_clks[i]);
		}
	}

	return 0;
}

int mt7622_afe_enable_clock(struct mtk_base_afe *afe)
{
#ifndef EARLY_PORTING
	int ret = 0;

	ret = mt7622_turn_on_a1sys_clock(afe);
	if (ret) {
		dev_err(afe->dev, "%s turn_on_a1sys_clock fail %d\n",
			__func__, ret);
		return ret;
	}

	ret = mt7622_turn_on_a2sys_clock(afe);
	if (ret) {
		dev_err(afe->dev, "%s turn_on_a2sys_clock fail %d\n",
			__func__, ret);
		mt7622_turn_off_a1sys_clock(afe);
		return ret;
	}

	ret = mt7622_turn_on_afe_clock(afe);
	if (ret) {
		dev_err(afe->dev, "%s turn_on_afe_clock fail %d\n",
			__func__, ret);
		mt7622_turn_off_a1sys_clock(afe);
		mt7622_turn_off_a2sys_clock(afe);
		return ret;
	}
#endif

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

void mt7622_afe_disable_clock(struct mtk_base_afe *afe)
{
	int retry = 0;
	u32 value;

	regmap_update_bits(afe->regmap, ASYS_TOP_CON,
			   AUDIO_TOP_CON0_A1SYS_A2SYS_ON, 0);
	regmap_update_bits(afe->regmap, AFE_DAC_CON0,
			   AFE_DAC_CON0_AFE_ON, 0);
	regmap_read(afe->regmap, AFE_DAC_MON0, &value);

	while ((value & 0x1) && ++retry < 100000)
		udelay(10);

	if (retry)
		dev_err(afe->dev, "%s retry %d\n", __func__, retry);

#ifndef EARLY_PORTING
	mt7622_turn_off_afe_clock(afe);
	mt7622_turn_off_a1sys_clock(afe);
	mt7622_turn_off_a2sys_clock(afe);
#endif
}

int mt7622_turn_on_a1sys_clock(struct mtk_base_afe *afe)
{
	struct mt7622_afe_private *afe_priv = afe->platform_priv;
	int ret = 0;
	/* Set Mux */
	ret = clk_prepare_enable(afe_priv->clocks[MT7622_AUD_A1SYS_HP_SEL]);
	if (ret) {
		dev_err(afe->dev, "%s clk_prepare_enable %s fail %d\n",
			__func__, aud_clks[MT7622_AUD_A1SYS_HP_SEL], ret);
		goto A1SYS_CLK_AUD_MUX1_SEL_ERR;
	}
	ret = clk_set_parent(afe_priv->clocks[MT7622_AUD_A1SYS_HP_SEL],
			     afe_priv->clocks[MT7622_AUD_AUD1PLL]);
	if (ret) {
		dev_err(afe->dev, "%s clk_set_parent %s-%s fail %d\n", __func__,
			aud_clks[MT7622_AUD_A1SYS_HP_SEL],
			aud_clks[MT7622_AUD_AUD1PLL], ret);
		goto A1SYS_CLK_AUD_MUX1_SEL_ERR;
	}

	/* Set Divider */
	ret = clk_prepare_enable(afe_priv->clocks[MT7622_AUD_A1SYS_HP_DIV]);
	if (ret) {
		dev_err(afe->dev, "%s clk_prepare_enable %s fail %d\n",
			__func__,
			aud_clks[MT7622_AUD_A1SYS_HP_DIV],
			ret);
		goto A1SYS_CLK_AUD_MUX1_DIV_ERR;
	}

	ret = clk_set_rate(afe_priv->clocks[MT7622_AUD_A1SYS_HP_DIV],
			   MT7622_AUD_AUD_MUX1_DIV_RATE);
	if (ret) {
		dev_err(afe->dev, "%s clk_set_parent %s-%d fail %d\n", __func__,
			aud_clks[MT7622_AUD_A1SYS_HP_DIV],
			MT7622_AUD_AUD_MUX1_DIV_RATE, ret);
		goto A1SYS_CLK_AUD_MUX1_DIV_ERR;
	}

	/* Enable clock gate */
	ret = clk_prepare_enable(afe_priv->clocks[MT7622_AUD_A1SYS_HP_DIV_PD]);
	if (ret) {
		dev_err(afe->dev, "%s clk_prepare_enable %s fail %d\n",
			__func__, aud_clks[MT7622_AUD_A1SYS_HP_DIV_PD], ret);
		goto A1SYS_CLK_AUD_48K_ERR;
	}

	/* Enable infra audio */
	ret = clk_prepare_enable(afe_priv->clocks[MT7622_AUD_CLK_INFRA_AUDIO_PD]);
	if (ret) {
		dev_err(afe->dev, "%s clk_prepare_enable %s fail %d\n",
			__func__, aud_clks[MT7622_AUD_CLK_INFRA_AUDIO_PD], ret);
		goto A1SYS_CLK_INFRA_ERR;
	}

	return 0;

A1SYS_CLK_INFRA_ERR:
	clk_disable_unprepare(afe_priv->clocks[MT7622_AUD_CLK_INFRA_AUDIO_PD]);
A1SYS_CLK_AUD_48K_ERR:
	clk_disable_unprepare(afe_priv->clocks[MT7622_AUD_A1SYS_HP_DIV_PD]);
A1SYS_CLK_AUD_MUX1_DIV_ERR:
	clk_disable_unprepare(afe_priv->clocks[MT7622_AUD_A1SYS_HP_DIV]);
A1SYS_CLK_AUD_MUX1_SEL_ERR:
	clk_disable_unprepare(afe_priv->clocks[MT7622_AUD_A1SYS_HP_SEL]);

	return ret;
}

void mt7622_turn_off_a1sys_clock(struct mtk_base_afe *afe)
{
	struct mt7622_afe_private *afe_priv = afe->platform_priv;

	clk_disable_unprepare(afe_priv->clocks[MT7622_AUD_CLK_INFRA_AUDIO_PD]);
	clk_disable_unprepare(afe_priv->clocks[MT7622_AUD_A1SYS_HP_DIV_PD]);
	clk_disable_unprepare(afe_priv->clocks[MT7622_AUD_A1SYS_HP_DIV]);
	clk_disable_unprepare(afe_priv->clocks[MT7622_AUD_A1SYS_HP_SEL]);
}

int mt7622_turn_on_a2sys_clock(struct mtk_base_afe *afe)
{
	struct mt7622_afe_private *afe_priv = afe->platform_priv;
	int ret = 0;
	/* Set Mux */
	ret = clk_prepare_enable(afe_priv->clocks[MT7622_AUD_A2SYS_HP_SEL]);
	if (ret) {
		dev_err(afe->dev, "%s clk_prepare_enable %s fail %d\n",
			__func__, aud_clks[MT7622_AUD_A2SYS_HP_SEL], ret);
		goto A2SYS_CLK_AUD_MUX2_SEL_ERR;
	}

	ret = clk_set_parent(afe_priv->clocks[MT7622_AUD_A2SYS_HP_SEL],
			     afe_priv->clocks[MT7622_AUD_AUD2PLL]);
	if (ret) {
		dev_err(afe->dev, "%s clk_set_parent %s-%s fail %d\n", __func__,
			aud_clks[MT7622_AUD_A2SYS_HP_SEL],
			aud_clks[MT7622_AUD_AUD2PLL], ret);
		goto A2SYS_CLK_AUD_MUX2_SEL_ERR;
	}

	/* Set Divider */
	ret = clk_prepare_enable(afe_priv->clocks[MT7622_AUD_A2SYS_HP_DIV]);
	if (ret) {
		dev_err(afe->dev, "%s clk_prepare_enable %s fail %d\n",
			__func__, aud_clks[MT7622_AUD_A2SYS_HP_DIV], ret);
		goto A2SYS_CLK_AUD_MUX2_DIV_ERR;
	}

	ret = clk_set_rate(afe_priv->clocks[MT7622_AUD_A2SYS_HP_DIV],
			   MT7622_AUD_AUD_MUX2_DIV_RATE);
	if (ret) {
		dev_err(afe->dev, "%s clk_set_parent %s-%d fail %d\n", __func__,
			aud_clks[MT7622_AUD_A2SYS_HP_DIV],
			MT7622_AUD_AUD_MUX2_DIV_RATE, ret);
		goto A2SYS_CLK_AUD_MUX2_DIV_ERR;
	}

	/* Enable clock gate */
	ret = clk_prepare_enable(afe_priv->clocks[MT7622_AUD_A2SYS_HP_DIV_PD]);
	if (ret) {
		dev_err(afe->dev, "%s clk_prepare_enable %s fail %d\n",
			__func__, aud_clks[MT7622_AUD_A2SYS_HP_DIV_PD], ret);
		goto A2SYS_CLK_AUD_44K_ERR;
	}

	/* Enable infra audio */
	ret = clk_prepare_enable(afe_priv->clocks[MT7622_AUD_CLK_INFRA_AUDIO_PD]);
	if (ret) {
		dev_err(afe->dev, "%s clk_prepare_enable %s fail %d\n",
			__func__, aud_clks[MT7622_AUD_CLK_INFRA_AUDIO_PD], ret);
		goto A2SYS_CLK_INFRA_ERR;
	}
	return 0;

A2SYS_CLK_INFRA_ERR:
	clk_disable_unprepare(afe_priv->clocks[MT7622_AUD_CLK_INFRA_AUDIO_PD]);
A2SYS_CLK_AUD_44K_ERR:
	clk_disable_unprepare(afe_priv->clocks[MT7622_AUD_A2SYS_HP_DIV_PD]);
A2SYS_CLK_AUD_MUX2_DIV_ERR:
	clk_disable_unprepare(afe_priv->clocks[MT7622_AUD_A2SYS_HP_DIV]);
A2SYS_CLK_AUD_MUX2_SEL_ERR:
	clk_disable_unprepare(afe_priv->clocks[MT7622_AUD_A2SYS_HP_SEL]);

	return ret;
}

void mt7622_turn_off_a2sys_clock(struct mtk_base_afe *afe)
{
	struct mt7622_afe_private *afe_priv = afe->platform_priv;

	clk_disable_unprepare(afe_priv->clocks[MT7622_AUD_CLK_INFRA_AUDIO_PD]);
	clk_disable_unprepare(afe_priv->clocks[MT7622_AUD_A2SYS_HP_DIV_PD]);
	clk_disable_unprepare(afe_priv->clocks[MT7622_AUD_A2SYS_HP_DIV]);
	clk_disable_unprepare(afe_priv->clocks[MT7622_AUD_A2SYS_HP_SEL]);
}

int mt7622_turn_on_afe_clock(struct mtk_base_afe *afe)
{
	struct mt7622_afe_private *afe_priv = afe->platform_priv;
	int ret;

	/* enable INFRA_SYS */
	ret = clk_prepare_enable(afe_priv->clocks[MT7622_AUD_CLK_INFRA_AUDIO_PD]);
	if (ret) {
		dev_err(afe->dev, "%s clk_prepare_enable %s fail %d\n",
			__func__, aud_clks[MT7622_AUD_CLK_INFRA_AUDIO_PD], ret);
		goto AFE_AUD_INFRA_ERR;
	}

	/* Set MT7622_AUD_AUDINTBUS to MT7622_AUD_SYSPLL1_D4_140MHz */
	ret = clk_prepare_enable(afe_priv->clocks[MT7622_AUD_AUDINTBUS]);
	if (ret) {
		dev_err(afe->dev, "%s clk_prepare_enable %s fail %d\n",
			__func__, aud_clks[MT7622_AUD_AUDINTBUS], ret);
		goto AFE_AUD_AUDINTBUS_ERR;
	}

	ret = clk_set_parent(afe_priv->clocks[MT7622_AUD_AUDINTBUS],
			     afe_priv->clocks[MT7622_AUD_SYSPLL1_D4]);
	if (ret) {
		dev_err(afe->dev, "%s clk_set_parent %s-%s fail %d\n", __func__,
			aud_clks[MT7622_AUD_AUDINTBUS],
			aud_clks[MT7622_AUD_SYSPLL1_D4], ret);
		goto AFE_AUD_AUDINTBUS_ERR;
	}

	/* Set MT7622_AUD_AUDINTDIR to MT7622_AUD_UNIVPLL_D2_600MHZ */
	ret = clk_prepare_enable(afe_priv->clocks[MT7622_AUD_AUDINTDIR]);
	if (ret) {
		dev_err(afe->dev, "%s clk_prepare_enable %s fail %d\n",
			__func__, aud_clks[MT7622_AUD_AUDINTDIR], ret);
		goto AFE_AUD_AUDINTDIR_ERR;
	}

	ret = clk_set_parent(afe_priv->clocks[MT7622_AUD_AUDINTDIR],
			     afe_priv->clocks[MT7622_AUD_UNIVPLL_D2]);
	if (ret) {
		dev_err(afe->dev, "%s clk_set_parent %s-%s fail %d\n", __func__,
			aud_clks[MT7622_AUD_AUDINTDIR],
			aud_clks[MT7622_AUD_UNIVPLL_D2], ret);
		goto AFE_AUD_AUDINTDIR_ERR;
	}

	/* Set APLL1_MUX for TDM and IEC (147M) */
	ret = clk_prepare_enable(afe_priv->clocks[MT7622_AUD_APLL1_SEL]);
	if (ret) {
		dev_err(afe->dev, "%s clk_prepare_enable %s fail %d\n",
			__func__, aud_clks[MT7622_AUD_APLL1_SEL], ret);
		goto AFE_AUD_APLL1_SEL_ERR;
	}
	ret = clk_set_parent(afe_priv->clocks[MT7622_AUD_APLL1_SEL],
			     afe_priv->clocks[MT7622_AUD_AUD1_SEL]);
	if (ret) {
		dev_err(afe->dev, "%s clk_set_parent %s-%s fail %d\n",
			__func__, aud_clks[MT7622_AUD_APLL1_SEL], aud_clks[MT7622_AUD_AUD1_SEL], ret);
		goto AFE_AUD_APLL1_SEL_ERR;
	}

	ret = clk_prepare_enable(afe_priv->clocks[MT7622_AUD_APLL1_DIV]);
	if (ret) {
		dev_err(afe->dev, "%s clk_prepare_enable %s fail %d\n",
			__func__, aud_clks[MT7622_AUD_APLL1_DIV], ret);
		goto AFE_AUD_APLL1_DIV_ERR;
	}
	ret = clk_set_rate(afe_priv->clocks[MT7622_AUD_APLL1_DIV], MT7622_PLL_DOMAIN_0_RATE);
	if (ret) {
		dev_err(afe->dev, "%s clk_set_rate %s fail %d\n", __func__,
			aud_clks[MT7622_AUD_APLL1_DIV], ret);
		goto AFE_AUD_APLL1_DIV_ERR;
	}

	/* Set APLL2_MUX for TDM and IEC (135M) */
	ret = clk_prepare_enable(afe_priv->clocks[MT7622_AUD_APLL2_SEL]);
	if (ret) {
		dev_err(afe->dev, "%s clk_prepare_enable %s fail %d\n",
			__func__, aud_clks[MT7622_AUD_APLL2_SEL], ret);
		goto AFE_AUD_APLL2_SEL_ERR;
	}
	ret = clk_set_parent(afe_priv->clocks[MT7622_AUD_APLL2_SEL],
			     afe_priv->clocks[MT7622_AUD_AUD2_SEL]);
	if (ret) {
		dev_err(afe->dev, "%s clk_set_parent %s-%s fail %d\n",
			__func__, aud_clks[MT7622_AUD_APLL2_SEL], aud_clks[MT7622_AUD_AUD2_SEL], ret);
		goto AFE_AUD_APLL2_SEL_ERR;
	}

	ret = clk_prepare_enable(afe_priv->clocks[MT7622_AUD_APLL2_DIV]);
	if (ret) {
		dev_err(afe->dev, "%s clk_prepare_enable %s fail %d\n",
			__func__, aud_clks[MT7622_AUD_APLL2_DIV], ret);
		goto AFE_AUD_APLL2_DIV_ERR;
	}

	ret = clk_set_rate(afe_priv->clocks[MT7622_AUD_APLL2_DIV], MT7622_PLL_DOMAIN_1_RATE);
	if (ret) {
		dev_err(afe->dev, "%s clk_set_rate %s fail %d\n", __func__,
			aud_clks[MT7622_AUD_APLL2_DIV], ret);
		goto AFE_AUD_APLL2_DIV_ERR;
	}

	/* enable afe pdn*/
	ret = clk_prepare_enable(afe_priv->clocks[MT7622_AUD_AUDIO_AFE]);
	if (ret) {
		dev_err(afe->dev, "%s clk_prepare_enable %s fail %d\n",
			__func__, aud_clks[MT7622_AUD_AUDIO_AFE], ret);
		goto AFE_AUD_AFE_PDN_ERR;
	}

	ret = clk_prepare_enable(afe_priv->clocks[MT7622_AUD_AUDIO_APLL]);
	if (ret) {
		dev_err(afe->dev, "%s clk_prepare_enable %s fail %d\n",
			__func__, aud_clks[MT7622_AUD_AUDIO_APLL], ret);
		goto AFE_AUD_APLL_PDN_ERR;
	}

	ret = clk_prepare_enable(afe_priv->clocks[MT7622_AUD_AUDIO_A1SYS]);
	if (ret) {
		dev_err(afe->dev, "%s clk_prepare_enable %s fail %d\n",
			__func__, aud_clks[MT7622_AUD_AUDIO_A1SYS], ret);
		goto AFE_AUD_A1SYS_PDN_ERR;
	}

	ret = clk_prepare_enable(afe_priv->clocks[MT7622_AUD_AUDIO_A2SYS]);
	if (ret) {
		dev_err(afe->dev, "%s clk_prepare_enable %s fail %d\n",
			__func__, aud_clks[MT7622_AUD_AUDIO_A2SYS], ret);
		goto AFE_AUD_A2SYS_PDN_ERR;
	}
	regmap_update_bits(afe->regmap, AUDIO_TOP_CON4,
			   AUDIO_TOP_CON4_PDN_AFE_CONN, 0);

	return 0;
AFE_AUD_A2SYS_PDN_ERR:
	clk_disable_unprepare(afe_priv->clocks[MT7622_AUD_AUDIO_A2SYS]);
AFE_AUD_A1SYS_PDN_ERR:
	clk_disable_unprepare(afe_priv->clocks[MT7622_AUD_AUDIO_A1SYS]);
AFE_AUD_APLL_PDN_ERR:
	clk_disable_unprepare(afe_priv->clocks[MT7622_AUD_AUDIO_APLL]);
AFE_AUD_AFE_PDN_ERR:
	clk_disable_unprepare(afe_priv->clocks[MT7622_AUD_AUDIO_AFE]);
AFE_AUD_APLL2_DIV_ERR:
	clk_disable_unprepare(afe_priv->clocks[MT7622_AUD_APLL2_DIV]);
AFE_AUD_APLL2_SEL_ERR:
	clk_disable_unprepare(afe_priv->clocks[MT7622_AUD_APLL2_SEL]);
AFE_AUD_APLL1_DIV_ERR:
	clk_disable_unprepare(afe_priv->clocks[MT7622_AUD_APLL1_DIV]);
AFE_AUD_APLL1_SEL_ERR:
	clk_disable_unprepare(afe_priv->clocks[MT7622_AUD_APLL1_SEL]);
AFE_AUD_AUDINTDIR_ERR:
	clk_disable_unprepare(afe_priv->clocks[MT7622_AUD_AUDINTDIR]);
AFE_AUD_AUDINTBUS_ERR:
	clk_disable_unprepare(afe_priv->clocks[MT7622_AUD_AUDINTBUS]);
AFE_AUD_INFRA_ERR:
	clk_disable_unprepare(afe_priv->clocks[MT7622_AUD_CLK_INFRA_AUDIO_PD]);

	return ret;
}

void mt7622_turn_off_afe_clock(struct mtk_base_afe *afe)
{
	struct mt7622_afe_private *afe_priv = afe->platform_priv;

	clk_disable_unprepare(afe_priv->clocks[MT7622_AUD_AUDIO_A2SYS]);
	clk_disable_unprepare(afe_priv->clocks[MT7622_AUD_AUDIO_A1SYS]);
	clk_disable_unprepare(afe_priv->clocks[MT7622_AUD_AUDIO_APLL]);
	clk_disable_unprepare(afe_priv->clocks[MT7622_AUD_AUDIO_AFE]);
	clk_disable_unprepare(afe_priv->clocks[MT7622_AUD_APLL2_DIV]);
	clk_disable_unprepare(afe_priv->clocks[MT7622_AUD_APLL2_SEL]);
	clk_disable_unprepare(afe_priv->clocks[MT7622_AUD_APLL1_DIV]);
	clk_disable_unprepare(afe_priv->clocks[MT7622_AUD_APLL1_SEL]);
	clk_disable_unprepare(afe_priv->clocks[MT7622_AUD_AUDINTDIR]);
	clk_disable_unprepare(afe_priv->clocks[MT7622_AUD_AUDINTBUS]);
	clk_disable_unprepare(afe_priv->clocks[MT7622_AUD_CLK_INFRA_AUDIO_PD]);
	regmap_update_bits(afe->regmap, AUDIO_TOP_CON4,
			   AUDIO_TOP_CON4_PDN_AFE_CONN,
			   AUDIO_TOP_CON4_PDN_AFE_CONN);
}

void mt7622_i2s_mclk_configuration(struct mtk_base_afe *afe, int id, int domain,
				   int mclk)
{
	struct mt7622_afe_private *afe_priv = afe->platform_priv;
	int ret;
	int aud_src_div_id = MT7622_AUD_I2S1_MCK_DIV;
	int aud_src_clk_id = MT7622_AUD_I2S1_MCK_SEL;

	/* Set I2S1 MCLK (domain) */
	ret = clk_prepare_enable(afe_priv->clocks[aud_src_clk_id]);
	if (ret)
		dev_err(afe->dev, "%s clk_prepare_enable %s fail %d\n",
			__func__, aud_clks[aud_src_clk_id], ret);

	if (domain == 0) {
		ret = clk_set_parent(afe_priv->clocks[aud_src_clk_id],
				     afe_priv->clocks[MT7622_AUD_AUD1_SEL]);
		if (ret)
			dev_err(afe->dev, "%s clk_set_parent %s-%s fail %d\n",
				__func__, aud_clks[aud_src_clk_id],
				aud_clks[MT7622_AUD_AUD1_SEL], ret);
	} else {
		ret = clk_set_parent(afe_priv->clocks[aud_src_clk_id],
				     afe_priv->clocks[MT7622_AUD_AUD2_SEL]);
		if (ret)
			dev_err(afe->dev, "%s clk_set_parent %s-%s fail %d\n",
				__func__, aud_clks[aud_src_clk_id],
				aud_clks[MT7622_AUD_AUD2_SEL], ret);
	}
	clk_disable_unprepare(afe_priv->clocks[aud_src_clk_id]);

	/* Set I2S1 MCLK (divider) */
	ret = clk_prepare_enable(afe_priv->clocks[aud_src_div_id]);
	if (ret)
		dev_err(afe->dev, "%s clk_prepare_enable %s fail %d\n",
			__func__, aud_clks[aud_src_div_id], ret);

	ret = clk_set_rate(afe_priv->clocks[aud_src_div_id], mclk);
	if (ret)
		dev_err(afe->dev, "%s clk_set_rate %s-%d fail %d\n", __func__,
			aud_clks[aud_src_div_id], mclk, ret);
	clk_disable_unprepare(afe_priv->clocks[aud_src_div_id]);
}

void mt7622_tdm_mclk_configuration(struct mtk_base_afe *afe, int tdm_id, int fs)
{

	struct mt7622_afe_private *afe_priv = afe->platform_priv;
	int mclk_rate, clk_domain;
	int ret;
	int aud_src_div_id, aud_src_clk_id;

	/* get mclk rate*/
	if (tdm_id == MT7622_TDMO)
		mclk_rate = fs * (fs >= 176400 ? 128 : 256);
	else {
		if (fs < 108000)
			mclk_rate = fs * 256;
		else if (fs < 216000)
			mclk_rate = fs * 128;
		else
			dev_err(afe->dev, "Unsupport sample rate (%d)!\n", fs);
	}

	/* mclk domain */
	if (MT7622_PLL_DOMAIN_0_RATE % mclk_rate == 0)
		clk_domain = 0;
	else if (MT7622_PLL_DOMAIN_1_RATE % mclk_rate == 0)
		clk_domain = 1;
	else
		dev_err(afe->dev, "%s() bad mclk rate %d\n", __func__, mclk_rate);

	if (tdm_id == 0) {
		aud_src_div_id = MT7622_AUD_I2S3_MCK_DIV;
		aud_src_clk_id = MT7622_AUD_I2S3_MCK_SEL;
	} else {
		aud_src_div_id = MT7622_AUD_I2S2_MCK_DIV;
		aud_src_clk_id = MT7622_AUD_I2S2_MCK_SEL;
	}

	/* Set TDM MCLK (domain) */
	ret = clk_prepare_enable(afe_priv->clocks[aud_src_clk_id]);
	if (ret)
		dev_err(afe->dev, "%s clk_prepare_enable %s fail %d\n",
			__func__, aud_clks[aud_src_clk_id], ret);

	if (clk_domain == 0) {
		ret = clk_set_parent(afe_priv->clocks[aud_src_clk_id], afe_priv->clocks[MT7622_AUD_AUD1_SEL]);
		if (ret)
			dev_err(afe->dev, "%s clk_set_parent %s-%s fail %d\n",
				__func__, aud_clks[aud_src_clk_id], aud_clks[MT7622_AUD_AUD1_SEL], ret);
	} else {
		ret = clk_set_parent(afe_priv->clocks[aud_src_clk_id], afe_priv->clocks[MT7622_AUD_AUD2_SEL]);
		if (ret)
			dev_err(afe->dev, "%s clk_set_parent %s-%s fail %d\n",
				__func__, aud_clks[aud_src_clk_id], aud_clks[MT7622_AUD_AUD2_SEL], ret);
	}
	clk_disable_unprepare(afe_priv->clocks[aud_src_clk_id]);

	/* Set TDM MCLK (divider) */
	ret = clk_prepare_enable(afe_priv->clocks[aud_src_div_id]);
	if (ret)
		dev_err(afe->dev, "%s clk_prepare_enable %s fail %d\n",
			__func__, aud_clks[aud_src_div_id], ret);

	ret = clk_set_rate(afe_priv->clocks[aud_src_div_id], mclk_rate);
	if (ret)
		dev_err(afe->dev, "%s clk_set_rate %s-%d fail %d\n",
			__func__, aud_clks[aud_src_div_id], mclk_rate, ret);
	clk_disable_unprepare(afe_priv->clocks[aud_src_div_id]);
}

void mt7622_tdm_clk_configuration(struct mtk_base_afe *afe, int tdm_id,
				  struct snd_pcm_hw_params *params)
{
	int apll_sel, apll_div;
	unsigned int fs_rate = params_rate(params);
	int channels = params_channels(params);
	int bit_width = params_width(params);
	int bck, mclk;
#ifdef EARLY_PORTING
	int dram_width_apll1 =  2 * 192000 * 16;
	int dram_width_apll2 =  2 * 176400 * 16;
	int dram_width;
#endif
	bck = fs_rate * channels * bit_width;
	mclk = fs_rate * 512;

	if (!(AUDIO_CLOCK_147M % bck))
		apll_sel = 1;
	else
		apll_sel = 2;

#ifdef EARLY_PORTING
	int mclk_div, total_div;
	/* FPGA setting */
	regmap_update_bits(afe->regmap, FPGA_CFG0, 0xffffffff, 0);

	if (tdm_id == MT7622_TDMO)
		regmap_update_bits(afe->regmap, FPGA_CFG2, 1 << 0, 1 << 0);
	else if (tdm_id == MT7622_TDMI)
		regmap_update_bits(afe->regmap, FPGA_CFG1, 0x1, 0x1);

	/* dram bandwiidth for FPGA */
	dram_width = (apll_sel == 1) ? dram_width_apll1 : dram_width_apll2;
	if (bck > dram_width)
		total_div = bck / dram_width;
	else
		total_div = 1;

	regmap_update_bits(afe->regmap, FPGA_CFG0, 0xff << 24, (total_div - 1) << 24);

	if (apll_sel == 1) {
		if (mclk > AUDIO_CLOCK_147M)
			mclk_div = 1;
		else
			mclk_div = AUDIO_CLOCK_147M / mclk;

		regmap_update_bits(afe->regmap, FPGA_CFG3, 0xffff, (mclk_div - 1) << 8 | (mclk_div - 1) << 0);
	} else {
		if (mclk > AUDIO_CLOCK_135M)
			mclk_div = 1;
		else
			mclk_div = AUDIO_CLOCK_135M / mclk;

		regmap_update_bits(afe->regmap, FPGA_CFG3, 0xffff, (mclk_div - 1) << 8 | (mclk_div - 1) << 0);
		regmap_update_bits(afe->regmap, FPGA_CFG3, 1 << 29 | 1 << 30, 1 << 29 | 1 << 30);
	}
#endif
	/* Audio Top Config */
	if (!(AUDIO_CLOCK_147M % bck)) {
		if (bck > AUDIO_CLOCK_147M)
			apll_div = 1;
		else
			apll_div = AUDIO_CLOCK_147M / bck;

		if (tdm_id == MT7622_TDMO) {
			regmap_update_bits(afe->regmap, AFE_TDMOUT_CLKDIV_CFG, TDMOUT_MCK_SEL, 1 << 2);
			regmap_update_bits(afe->regmap, AFE_TDMOUT_CLKDIV_CFG, TDMOUT_CLK_DIV_B, (apll_div - 1) << 20);
		} else {
			regmap_update_bits(afe->regmap, AFE_TDMIN_CLKDIV_CFG, TDMIN_MCK_SEL, 1 << 2);
			regmap_update_bits(afe->regmap, AFE_TDMIN_CLKDIV_CFG, TDMIN_CLK_DIV_B, (apll_div - 1) << 20);

		}
	} else {
		if (bck > AUDIO_CLOCK_135M)
			apll_div = 1;
		else
			apll_div = AUDIO_CLOCK_135M / bck;

		if (tdm_id == MT7622_TDMO) {
			regmap_update_bits(afe->regmap, AFE_TDMOUT_CLKDIV_CFG, TDMOUT_MCK_SEL, 0 << 2);
			regmap_update_bits(afe->regmap, AFE_TDMOUT_CLKDIV_CFG, TDMOUT_CLK_DIV_B, (apll_div - 1) << 20);
		} else {
			regmap_update_bits(afe->regmap, AFE_TDMIN_CLKDIV_CFG, TDMIN_MCK_SEL, 0 << 2);
			regmap_update_bits(afe->regmap, AFE_TDMIN_CLKDIV_CFG, TDMIN_CLK_DIV_B, (apll_div - 1) << 20);
		}
	}

	/*enable div_m and pd*/
	if (tdm_id == MT7622_TDMO) {
		regmap_update_bits(afe->regmap, AFE_TDMOUT_CLKDIV_CFG, TDMOUT_CLK_DIV_M, 0 << 8);
		regmap_update_bits(afe->regmap, AFE_TDMOUT_CLKDIV_CFG, (TDMOUT_PDN_M | TDMOUT_PDN_B),
				   (0 << 0 | 0 << 1));
	} else {
		regmap_update_bits(afe->regmap, AFE_TDMIN_CLKDIV_CFG, TDMIN_CLK_DIV_M, 0 << 8);
		regmap_update_bits(afe->regmap, AFE_TDMIN_CLKDIV_CFG, (TDMIN_PDN_M | TDMIN_PDN_B), (0 << 0 | 0 << 1));
	}
}

MODULE_DESCRIPTION("MT7622 afe clock control");
MODULE_AUTHOR("Unknown");
MODULE_LICENSE("GPL v2");
