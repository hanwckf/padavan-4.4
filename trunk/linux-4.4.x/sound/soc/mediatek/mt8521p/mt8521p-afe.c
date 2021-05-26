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

#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <sound/pcm_params.h>
#include "mt8521p-aud-global.h"
#ifdef CONFIG_MTK_LEGACY_CLOCK
#include <mach/mt_clkmgr.h>
#else
#include "mt8521p-afe-clk.h"
#endif
#include <linux/of_address.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/device.h>
/*#include <mach/mt_gpio.h>*/
#include "mt8521p-afe.h"
#include "mt8521p-afe-reg.h"
#include "mt8521p-aud-gpio.h"
#include "mt8521p-dai.h"
#include "mt8521p-private.h"
#include "mt8521p-afe-reg.h"
#include "mt8521p-dai-private.h"
#include "mt8521p-private.h"

#ifdef AUDIO_MEM_IOREMAP
/* address for ioremap audio hardware register */
void *afe_base_address;
void *afe_sram_address;
void *topckgen_base_address;
void *cmsys_base_address;
void *infracfg_base_address;
void *pctrl_base_address;
unsigned int afe_sram_phy_address;
unsigned int afe_sram_max_size;
int afe_sram_enable;

#endif
#ifdef CONFIG_OF
static unsigned int afe_irq_id;
static unsigned int asys_irq_id;
#endif

static bool afe_loopback;

void mt_afe_reg_unmap(void)
{
#ifdef AUDIO_MEM_IOREMAP

	if (afe_base_address) {
		iounmap(afe_base_address);
		afe_base_address = NULL;
	}
	if (afe_sram_address) {
		iounmap(afe_sram_address);
		afe_sram_address = NULL;
		afe_sram_max_size = 0;
	}
	if (topckgen_base_address) {
		iounmap(topckgen_base_address);
		topckgen_base_address = NULL;
	}
	if (cmsys_base_address) {
		iounmap(cmsys_base_address);
		cmsys_base_address = NULL;
	}
#endif

}

int mt_afe_reg_remap(void *dev)
{
	int ret = 0;
#ifdef AUDIO_IOREMAP_FROM_DT
	struct device *pdev = dev;
	struct resource res;
	struct device_node *node;

	/* AFE register base */
	ret = of_address_to_resource(pdev->of_node, 0, &res);
	if (ret) {
		pr_err("%s of_address_to_resource#0 fail %d\n", __func__, ret);
		goto exit;
	}

	afe_base_address = ioremap_nocache(res.start, resource_size(&res));
	if (!afe_base_address) {
		pr_err("%s ioremap_nocache#0 addr:0x%llx size:0x%llx fail\n",
		       __func__, (unsigned long long)res.start,
		       (unsigned long long)resource_size(&res));
		ret = -ENXIO;
		goto exit;
	}
	pr_debug("afe_base_address = 0x%p from 0x%x\n", afe_base_address, res.start);

	/* audio SRAM base */
	ret = of_address_to_resource(pdev->of_node, 1, &res);
	if (ret) {
		pr_err("%s of_address_to_resource#1 fail %d\n", __func__, ret);
		goto exit;
	}

	afe_sram_address = ioremap_nocache(res.start, resource_size(&res));
	if (!afe_sram_address) {
		pr_err("%s ioremap_nocache#1 addr:0x%llx size:0x%llx fail\n",
		       __func__, (unsigned long long)res.start,
		       (unsigned long long)resource_size(&res));
		ret = -ENXIO;
		goto exit;
	}
	afe_sram_phy_address = res.start;
	afe_sram_max_size = resource_size(&res);

	/* TOPCKGEN register base */
	node = of_find_compatible_node(NULL, NULL, "mediatek,mt2701-topckgen"); /* need to check */
	if (!node) {
		pr_warn("%s of_find_compatible_node(mediatek,mt2701-topckgen) fail\n", __func__);
		topckgen_base_address = ioremap_nocache(TOPCKGEN_BASE_ADDR, 0x1000);
	} else {
		topckgen_base_address = of_iomap(node, 0);
	}

	if (!topckgen_base_address) {
		pr_err("%s ioremap topckgen_base_address fail\n", __func__);
		ret = -ENODEV;
		goto exit;
	}
	pr_debug("topckgen_base_address = 0x%p\n", topckgen_base_address);

	/* todo for cmsys: need to find the device tree node for CMSYS*/
	cmsys_base_address = ioremap_nocache(CMSYS_BASE_ADDR, 0x1000);
	pr_debug("cmsys_base_address = 0x%p\n", cmsys_base_address);

	infracfg_base_address = ioremap_nocache(INFRACFG_BASE_ADDR, 0x1000);
	pr_debug("infracfg_base_address = 0x%p\n", infracfg_base_address);

	pctrl_base_address = ioremap_nocache(PCTRL_BASE_ADDR, 0x2000);
	pr_debug("pctrl_base_address = 0x%p\n", pctrl_base_address);
	pr_debug("afe_sram_address = 0x%p from 0x%x\n", afe_sram_address, afe_sram_phy_address);

exit:
	if (ret)
		mt_afe_reg_unmap();
#else
	afe_sram_address = ioremap_nocache(AFE_INTERNAL_SRAM_PHYS_BASE, AFE_INTERNAL_SRAM_SIZE);
	pr_warn("afe_sram_address = 0x%x\n", afe_sram_address);
	afe_base_address = ioremap_nocache(AUDIO_HW_PHYS_BASE_ADDR, 0x2000);
	pr_warn("afe_base_address = 0x%x\n", afe_base_address);
	/*pr_warn("afe_base_address = 0x%x\n", IO_PHYS_TO_VIRT((u32)(AUDIO_HW_PHYS_BASE_ADDR)));*/
	/*spm_base_address = ioremap_nocache(SPM_BASE, 0x1000);*/
	topckgen_base_address = ioremap_nocache(TOPCKGEN_BASE_ADDR, 0x1000);
	pr_warn("topckgen_base_address = 0x%x\n", topckgen_base_address);
	/*apmixedsys_base_address = ioremap_nocache(APMIXEDSYS_BASE, 0x1000);*/
	cmsys_base_address = ioremap_nocache(CMSYS_BASE_ADDR, 0x1000);
	pr_warn("cmsys_base_address = 0x%x\n", cmsys_base_address);
#endif
	return ret;
}

int mt_afe_platform_init(void *dev)
{
	struct device *pdev = dev;
	int ret = 0;

#ifdef CONFIG_OF
	unsigned int irq_id;

	if (!pdev->of_node) {
		pr_warn("%s invalid of_node\n", __func__);
		return -ENODEV;
	}

	irq_id = irq_of_parse_and_map(pdev->of_node, 0);
	if (irq_id)
		afe_irq_id = irq_id;
	else
		pr_warn("%s irq_of_parse_and_map invalid irq\n", __func__);

	irq_id = irq_of_parse_and_map(pdev->of_node, 1);
	if (irq_id)
		asys_irq_id = irq_id;
	else
		pr_warn("%s irq_of_parse_and_map invalid irq\n", __func__);


#endif
	ret = mt_afe_reg_remap(dev);
	if (ret)
		return ret;

	ret = mt_afe_init_clock(dev);
	if (ret)
		return ret;

	return ret;
}

void mt_afe_platform_deinit(void *dev)
{
	mt_afe_reg_unmap();
	mt_afe_deinit_clock(dev);
}

#ifdef CONFIG_OF
unsigned int mt_afe_get_afe_irq_id(void)
{
	return afe_irq_id;
}

unsigned int mt_afe_get_asys_irq_id(void)
{
	return asys_irq_id;
}
#endif

static inline void afe_set_bit(u32 addr, int bit)
{
#if 0
	enum afe_mem_asrc_id id;
	u32 val = (0x1 << bit);
	u32 msk = (0x1 << bit);

	for (id = MEM_ASRC_1; id < MEM_ASRC_NUM; ++id) {
		if (addr == REG_ASRC_GEN_CONF + id * 0x100) {
			pr_debug("%s() raise bit 24 of REG_ASRC_GEN_CONF + %d * 0x100\n", __func__, id);
			val |= (0x1 << 24);
			msk |= (0x1 << 24);
			break;
		}
	}
	afe_msk_write(addr, val, msk);
#else
	afe_msk_write(addr, 0x1 << bit, 0x1 << bit);
#endif
}

static inline void afe_clear_bit(u32 addr, int bit)
{
	afe_msk_write(addr, 0x0, 0x1 << bit);
}

static inline void afe_write_bits(u32 addr, u32 value, int bits, int len)
{
	u32 u4TargetBitField = ((0x1 << len) - 1) << bits;
	u32 u4TargetValue = (value << bits) & u4TargetBitField;
	u32 u4CurrValue;

	u4CurrValue = afe_read(addr);
	afe_write(addr, ((u4CurrValue & (~u4TargetBitField)) | u4TargetValue));
}

static inline u32 afe_read_bits(u32 addr, int bits, int len)
{
	u32 u4TargetBitField = ((0x1 << len) - 1) << bits;
	u32 u4CurrValue = afe_read(addr);

	return (u4CurrValue & u4TargetBitField) >> bits;
}

void afe_enable(int en)
{
#ifdef CONFIG_MTK_LEGACY_CLOCK
	int i;

	enum cg_clk_id clks[] = {
		MT_CG_INFRA_AUDIO,
		MT_CG_AUDIO_AFE,		/*AUDIO_TOP_CON0[2]*/
		MT_CG_AUDIO_APLL,		/*AUDIO_TOP_CON0[19]*/
		MT_CG_AUDIO_A1SYS,		/*AUDIO_TOP_CON4[21]*/
		MT_CG_AUDIO_A2SYS,		/*AUDIO_TOP_CON4[22]*/
		MT_CG_AUDIO_AFE_CONN,	/*AUDIO_TOP_CON4[23]*/
	};

	for (i = 0; i < 6; ++i)
		afe_i2s_power_on_mclk(i, 0);
#endif

	if (en) {
#ifdef CONFIG_MTK_LEGACY_CLOCK
		enable_pll(AUD1PLL, "AUDIO");
		enable_pll(AUD2PLL, "AUDIO");
		enable_pll(HADDS2PLL, "AUDIO");
		for (i = 0; i < ARRAY_SIZE(clks); ++i)
			enable_clock(clks[i], "AUDIO");
#else
		/*
		 * [Programming Guide]
		 * 1. Power on apll
		 * 2. set a1sys a2sys clock div
		 * 3. a1sys a2sys clock power on
		 */
		mt_afe_a1sys_hp_ck_on();
		mt_afe_a2sys_hp_ck_on();
		mt_afe_main_clk_on();
#endif
		/*
		 * [Programming Guide]
		 * clear infra audio clock power down bit
		 */
		afe_msk_write(AUDIO_TOP_CON5, PDN_MEMIF_MOD, PDN_MEMIF_MOD);
		afe_msk_write(ASYS_TOP_CON, A1SYS_TIMING_ON, A1SYS_TIMING_ON_MASK);
		/* i2s-out1/2/3/4/5/6 don't select sine-wave gen as source */
		afe_msk_write(AFE_SGEN_CON0, (0x1F << 27), (0x1F << 27));

#ifdef AUDIO_MEM_IOREMAP
#ifdef CONFIG_MTK_LEGACY_CLOCK  /* Controlled by CCF*/
		/* 0:26M, 1:APLL1(98.304M), 2:APLL2(90.3168M), 3:HADDS, 4:EXT1, 5:EXT2 */
		topckgen_msk_write(CLK_AUDDIV_3, 0x1 << 0, AUD_MUX1_CLK_MASK);	/* 48K domain */
		topckgen_msk_write(CLK_AUDDIV_3, 0x2 << 3, AUD_MUX2_CLK_MASK);	/* 44.1K domain */
		/* topckgen_msk_write(CLK_AUDDIV_3, 0x3<<6, AUD_HADDS_CLK_MASK);//Hadds */
		topckgen_msk_write(CLK_AUDDIV_0, 0x1 << 16, AUD_A1SYS_K1_MASK);	/* APLL1 DIV Fix:APLL1/2 */
		topckgen_msk_write(CLK_AUDDIV_0, 0x1 << 24, AUD_A2SYS_K1_MASK);	/* APLL2 DIV Fix:APLL2/2 */
		/* APLL1 CK ENABLE  current apll1 & apll2 all use bit21, ECO will modify apll2 to bit22 */
		topckgen_msk_write(CLK_AUDDIV_3, AUD_A1SYS_K1_ON, AUD_A1SYS_K1_ON_MASK);
#endif
#else
		/* 0:26M, 1:APLL1(98.304M), 2:APLL2(90.3168M), 3:HADDS, 4:EXT1, 5:EXT2 */
		afe_msk_write(CLK_AUDDIV_3, 0x1 << 0, AUD_MUX1_CLK_MASK);	/* 48K domain */
		afe_msk_write(CLK_AUDDIV_3, 0x2 << 3, AUD_MUX2_CLK_MASK);	/* 44.1K domain */
		/* afe_msk_write(CLK_AUDDIV_3, 0x3<<6, AUD_HADDS_CLK_MASK); */	/* Hadds */
		afe_msk_write(CLK_AUDDIV_0, 0x1 << 16, AUD_A1SYS_K1_MASK);	/* APLL1 DIV Fix:APLL1/2 */
		afe_msk_write(CLK_AUDDIV_0, 0x1 << 24, AUD_A2SYS_K1_MASK);	/* APLL2 DIV Fix:APLL2/2 */
		/* APLL1 CK ENABLE  current apll1 & apll2 all use bit21, ECO will modify apll2 to bit22 */
		afe_msk_write(CLK_AUDDIV_3, AUD_A1SYS_K1_ON, AUD_A1SYS_K1_ON_MASK);
#endif
		afe_write(PWR1_ASM_CON1, 0x492);	/* i2s asrc clk from 208M */
		afe_write(PWR2_ASM_CON1, 0x492492);	/* i2s asrc clk from 208M */

#ifndef AUD_PINCTRL_SUPPORTING
#ifdef AUDIO_MEM_IOREMAP
		/* boost i2s0   mclk,bck,lrck's driving to 0(4mA),2(8mA),4(12mA),6(16mA) */
		topckgen_write(BOOST_DRIVING_I2S0, 0x0040);
		/* boost i2s1/2 mclk,bck,lrck's driving to 0(4mA),2(8mA),4(12mA),6(16mA) */
		topckgen_write(BOOST_DRIVING_I2S12, 0x0044);
		/* boost i2s3/4 mclk,bck,lrck's driving to 0(4mA),2(8mA),4(12mA),6(16mA) */
		topckgen_write(BOOST_DRIVING_I2S34, 0x4400);
		/* boost i2s5   mclk,bck,lrck's driving to 0(4mA),2(8mA),4(12mA),6(16mA) */
		topckgen_write(BOOST_DRIVING_I2S5, 0x0004);
		/* boost spdif out's driving to 0(4mA),2(8mA),4(12mA),6(16mA) */
		topckgen_write(BOOST_DRIVING_SPDIF, 0x0004);
#else
		/* boost i2s0   mclk,bck,lrck's driving to 0(4mA),2(8mA),4(12mA),6(16mA) */
		afe_write(BOOST_DRIVING_I2S0, 0x0040);
		/* boost i2s1/2 mclk,bck,lrck's driving to 0(4mA),2(8mA),4(12mA),6(16mA) */
		afe_write(BOOST_DRIVING_I2S12, 0x0044);
		/* boost i2s3/4 mclk,bck,lrck's driving to 0(4mA),2(8mA),4(12mA),6(16mA) */
		afe_write(BOOST_DRIVING_I2S34, 0x4400);
		/* boost i2s5   mclk,bck,lrck's driving to 0(4mA),2(8mA),4(12mA),6(16mA) */
		afe_write(BOOST_DRIVING_I2S5, 0x0004);
		/* boost            spdif out's driving to 0(4mA),2(8mA),4(12mA),6(16mA) */
		afe_write(BOOST_DRIVING_SPDIF, 0x0004);
#endif
#endif
		/* for i2s-out multi-ch */
		afe_msk_write(ASYS_I2SO1_CON, 0x0 << LAT_DATA_EN_POS, 0x1 << LAT_DATA_EN_POS);
		afe_msk_write(ASYS_I2SO2_CON, 0x0 << LAT_DATA_EN_POS, 0x1 << LAT_DATA_EN_POS);
		afe_msk_write(PWR2_TOP_CON, 0x0,
				  (0x1 << LAT_DATA_EN_I2SO3_POS)
				| (0x1 << LAT_DATA_EN_I2SO4_POS)
				| (0x1 << LAT_DATA_EN_I2SO5_POS)
				| (0x1 << LAT_DATA_EN_I2SO6_POS));
		/* for hdmi-tx */
		afe_msk_write(AUDIO_TOP_CON0, APB3_SEL, APB3_SEL | APB_R2T | APB_W2T);	/* set bus */
		/* select spdif1 as hdmi_iec's source (default) */
		afe_msk_write(AUDIO_TOP_CON3, HDMI_IEC_FROM_SEL_SPDIF, HDMI_IEC_FROM_SEL_MASK);
#if 0
		/* select spdif1 as spdif pad's source (debug only) */
		afe_msk_write(AUDIO_TOP_CON3, PAD_IEC_FROM_SEL_SPDIF, PAD_IEC_FROM_SEL_MASK);
#else
		/* select spdif2 as spdif pad's source (default) */
		afe_msk_write(AUDIO_TOP_CON3, PAD_IEC_FROM_SEL_SPDIF2, PAD_IEC_FROM_SEL_MASK);
#endif
		afe_msk_write(AFE_HDMI_CONN0, SPDIF_LRCK_SRC_SEL_SPDIF, SPDIF_LRCK_SRC_SEL_MASK);
		afe_msk_write(AFE_HDMI_CONN0, SPDIF2_LRCK_SRC_SEL_SPDIF, SPDIF2_LRCK_SRC_SEL_MASK);

		/*
		 * [Programming Guide]
		 * Turn On AFE
		 */
		afe_msk_write(AFE_DAC_CON0, AFE_ON, AFE_ON_MASK);
		/* afe_hwgain_init(AFE_HWGAIN_1); */
	} else {
		afe_msk_write(AFE_DAC_CON0, AFE_OFF, AFE_ON_MASK);
#ifdef CONFIG_MTK_LEGACY_CLOCK
		for (i = ARRAY_SIZE(clks) - 1; i >= 0; --i)
			disable_clock(clks[i], "AUDIO");
		disable_pll(HADDS2PLL, "AUDIO");
		disable_pll(AUD2PLL, "AUDIO");
		disable_pll(AUD1PLL, "AUDIO");
#else
		mt_afe_main_clk_off();
		mt_afe_a1sys_hp_ck_off();
		mt_afe_a2sys_hp_ck_off();
#endif
	}
}

void afe_power_mode(enum power_mode mode)
{
	#ifdef AUDIO_MEM_IOREMAP
		#ifdef CONFIG_MTK_LEGACY_CLOCK
		u32 val = (mode == LOW_POWER_MODE)
			  ? CLK_AUD_INTBUS_SEL_CLK26M : CLK_AUD_INTBUS_SEL_SYSPLL1D4;

		topckgen_msk_write(CLK_CFG_3, val, CLK_AUD_INTBUS_SEL_MASK);
		#else
		/* TODO */
		#endif
	#else
	u32 val = (mode == LOW_POWER_MODE)
		  ? CLK_AUD_INTBUS_SEL_CLK26M : CLK_AUD_INTBUS_SEL_SYSPLL1D4;

	afe_msk_write(CLK_CFG_3, val, CLK_AUD_INTBUS_SEL_MASK);
	#endif
}

static void trigger_cm4_soft0_irq(void)
{
	/* trigger soft0_irq_b to CM4 */
	#ifdef AUDIO_MEM_IOREMAP
	cmsys_msk_write(0x10, 0x0 << 2, 0x1 << 2);
	#else
	afe_msk_write(CMSYS_BASE + 0x10, 0x0 << 2, 0x1 << 2);
	#endif
}

static int is_cm4_soft0_irq_cleared(void)
{
	#ifdef AUDIO_MEM_IOREMAP
	return (cmsys_read(0x10) & (0x1 << 2)) >> 2;
	#else
	return (afe_read(CMSYS_BASE + 0x10) & (0x1 << 2)) >> 2;
	#endif
}

unsigned char *afe_sram_virt(void)
{
	#ifdef AUDIO_MEM_IOREMAP
	return afe_sram_address;
	#else
	return (unsigned char *)IO_PHYS_TO_VIRT(AFE_INTERNAL_SRAM_PHYS_BASE);
	#endif
}

u32 afe_sram_phys(void)
{
	return AFE_INTERNAL_SRAM_PHYS_BASE;
}

size_t afe_sram_size(void)
{
	return AFE_INTERNAL_SRAM_SIZE;
}

void afe_spdif_out2_source_sel(enum spdif_out2_source s)
{
	switch (s) {
	case SPDIF_OUT2_SOURCE_IEC2:
		afe_msk_write(AFE_SPDIFIN_CFG1, (0x0 << 11), (0x1 << 11));
		break;
	case SPDIF_OUT2_SOURCE_OPTICAL_IN:
		afe_msk_write(AFE_SPDIFIN_BR, (0x1 << 25), (0x1 << 25));
		afe_msk_write(AFE_SPDIFIN_CFG1, (0x1 << 11), (0x1 << 11));
		break;
	case SPDIF_OUT2_SOURCE_COAXIAL_IN:
		afe_msk_write(AFE_SPDIFIN_BR, (0x0 << 25), (0x1 << 25));
		afe_msk_write(AFE_SPDIFIN_CFG1, (0x1 << 11), (0x1 << 11));
		break;
	default:
		break;
	}
}

/******************** interconnection ********************/

int itrcon_connect(enum itrcon_in in, enum itrcon_out out, int connect)
{
	u32 addr;
	unsigned int bit_pos;

	connect = !!connect;
	if (out > O33) {
		pr_err("%s() error: bad output port %d\n", __func__, out);
		return -EINVAL;
	}
	if (in <= I31) {
		addr = AFE_CONN0 + out * 4;
		bit_pos = in;
	} else if (in >= I32 && in <= I36) {
		/*
		* AFE_CONN35: O04 O03 O02 O01 O00
		* AFE_CONN36: O09 O08 O07 O06 O05
		* ...
		* AFE_CONN41:     O33 O32 O31 O30
		*/
		addr = AFE_CONN35 + (out / 5) * 4;
		bit_pos = (out % 5) * 6 + (in - I32);
	} else {
		pr_err("%s() error: bad input port %d\n", __func__, in);
		return -EINVAL;
	}
	pr_debug("%s() %s I_%d -> O_%d\n", __func__, connect ? "connect" : "disconnect", in, out);
	afe_msk_write(addr, connect << bit_pos, 0x1 << bit_pos);
	return 0;
}

void itrcon_disconnectall(void)
{
	u32 addr;

	pr_debug("%s()\n", __func__);
	for (addr = AFE_CONN0; addr <= AFE_CONN33; addr += 4)
		afe_write(addr, 0x0);
	for (addr = AFE_CONN35; addr <= AFE_CONN40; addr += 4)
		/* clear BIT29 ~ BIT0 */
		afe_msk_write(addr, 0x0, 0x3fffffff);
	/* clear BIT23 ~ BIT0 */
	afe_msk_write(AFE_CONN41, 0x0, 0xffffff);
}

int itrcon_rightshift1bit(enum itrcon_out out, int shift)
{
	u32 addr;
	unsigned int bit_pos;

	shift = !!shift;
	pr_debug("%s() %s right shift 1 bit for O_%d\n", __func__, shift ? "enable" : "disable", out);
	if (out <= O31) {
		addr = AFE_CONN34;
		bit_pos = out;
	} else if (out >= O32 && out <= O33) {
		addr = AFE_CONN41;
		bit_pos = out - O32 + 24;
	} else {
		pr_err("%s() bad output port: %d\n", __func__, out);
		return -EINVAL;
	}
	afe_msk_write(addr, shift << bit_pos, 0x1 << bit_pos);
	return 0;
}

void itrcon_noshiftall(void)
{
	pr_debug("%s()\n", __func__);
	afe_write(AFE_CONN34, 0x0);
	/* clear BIT25 BIT24 */
	afe_msk_write(AFE_CONN41, 0x0, 0x3000000);
}

/*
 * [Programming Guide]
 * itcon: if not NULL, set AFE Inter-connection input to output
 * start: if 1, disable clock power down when not multi-ch in
 */
int a1sys_start(itrcon_action itrcon, int start)
{
	static DEFINE_SPINLOCK(a1sys_started_lock);
	unsigned long flags;
	static int a1sys_started;

	spin_lock_irqsave(&a1sys_started_lock, flags);
	if (start) {
		if (a1sys_started == 0) {
#ifdef CONFIG_MTK_LEGACY_CLOCK
			enable_pll(AUD1PLL, "AUDIO");
			enable_clock(MT_CG_AUDIO_A1SYS, "AUDIO");
			udelay(200);
			enable_clock(MT_CG_AUDIO_AFE, "AUDIO");
#endif
		}
		if (itrcon)
			itrcon(1);
		++a1sys_started;
	} else {
		if (itrcon)
			itrcon(0);
		if (a1sys_started > 0) {
			if (--a1sys_started == 0) {
#ifdef CONFIG_MTK_LEGACY_CLOCK
				if (itrcon)
					udelay(200);
				disable_clock(MT_CG_AUDIO_AFE, "AUDIO");
				disable_clock(MT_CG_AUDIO_A1SYS, "AUDIO");
				disable_pll(AUD1PLL, "AUDIO");
#endif
			}
		}
	}
	spin_unlock_irqrestore(&a1sys_started_lock, flags);
	return 0;
}


/******************** memory interface ********************/

int afe_memif_enable(enum afe_mem_interface memif, int en)
{
	u32 val, msk;

	en = !!en;
	val = (en << memif);
	msk = (0x1 << memif);
	if (memif == AFE_MEM_NONE || memif == AFE_MEM_RESERVED) {
		pr_err("%s() invalid memif\n", __func__);
		return -EINVAL;
	}
	/*
	 * [Programming Guide]
	 * [I2S] if multi-ch out enable DL1
	 */
	if (memif == AFE_MEM_DLMCH) {
		/* jiechao.wei said if you want to use DLMCH, DL1 should be enabled too */
		val |= (en << AFE_MEM_DL1);
		msk |= (0x1 << AFE_MEM_DL1);
	}
	/*
	 * [Programming Guide]
	 * [I2S] Enable memif
	 */
	afe_msk_write(AFE_DAC_CON0, val, msk);
	/*
	 * [Programming Guide]
	 * [SPDIF IN]Enable mphone_multi
	 */
	if (memif == AFE_MEM_ULMCH)
		afe_multilinein_enable(en);
	return 0;
}

int afe_memifs_enable(enum afe_mem_interface *memifs, size_t num, int en)
{
	/*zhifei said: for i2s multi in, enable UL2~ULx first, then enable UL1*/
	u32 val = 0, msk = 0;
	int i = 0;

	en = !!en;
	if (num <= 1)
		return 0;
	for (i = 1; i < num; ++i) {
		enum afe_mem_interface memif = memifs[i];

		if (memif == AFE_MEM_NONE || memif == AFE_MEM_RESERVED) {
			pr_err("%s() invalid memif\n", __func__);
			return -EINVAL;
		}
		val |= (en << memif);
		msk |= (0x1 << memif);
	}
	afe_msk_write(AFE_DAC_CON0, val, msk);
	return 0;
}

int afe_memif_configurate(enum afe_mem_interface memif, const struct afe_memif_config *config)
{
	u32 base, end;
	static const char *name[AFE_MEM_NUM] = {
		"AFE_MEM_NONE"	/* 0 */
		, "AFE_MEM_DL1"	/* 1 */
		, "AFE_MEM_DL2"	/* 2 */
		, "AFE_MEM_DL3"	/* 3 */
		, "AFE_MEM_DL4"	/* 4 */
		, "AFE_MEM_DL5"	/* 5 */
		, "AFE_MEM_DL6"	/* 6 */
		, "AFE_MEM_DLMCH"	/* 7 */
		, "AFE_MEM_ARB1"	/* 8 */
		, "AFE_MEM_DSDR"	/* 9 */
		, "AFE_MEM_UL1"	/* 10 */
		, "AFE_MEM_UL2"	/* 11 */
		, "AFE_MEM_UL3"	/* 12 */
		, "AFE_MEM_UL4"	/* 13 */
		, "AFE_MEM_UL5"	/* 14 */
		, "AFE_MEM_UL6"	/* 15 */
		, "AFE_MEM_ULMCH"	/* 16 */
		, "AFE_MEM_DAI"	/* 17 */
		, "AFE_MEM_MOD_PCM"	/* 18 */
		, "reserved"	/* 19 */
		, "AFE_MEM_AWB"	/* 20 */
		, "AFE_MEM_AWB2"	/* 21 */
		, "AFE_MEM_DSDW"	/* 22 */
	};

	if (config == NULL)
		return -ENOMEM;
	base = config->buffer.base;
	end = base + config->buffer.size - 1;
	if (memif > AFE_MEM_DSDW || memif == AFE_MEM_NONE || memif == AFE_MEM_RESERVED) {
		pr_err("%s() invalid memif %d\n", __func__, memif);
		return -EINVAL;
	}
	pr_debug("%s() memif=%s, base=0x%x, size=0x%x, fs=%u\n", __func__, name[memif], base,
		 config->buffer.size, (memif == AFE_MEM_DAI || memif == AFE_MEM_MOD_PCM)
		 ? (config->daimod_fs + 1) * 8000 : fs_integer(config->fs));
	if (config->buffer.size == 0 || (base & 0xF) != 0x0 || (end & 0xF) != 0xF) {
		pr_err("%s() invalid buffer: base=0x%08x, end=0x%08x\n", __func__, base, end);
		return -EINVAL;
	}
	switch (memif) {
	case AFE_MEM_DL1:
	    /*set data not from DLM*/
		afe_msk_write(AFE_MEMIF_PBUF_SIZE, DLMCH_OUT_SEL_PAIR_INTERLEAVE,
			      DLMCH_OUT_SEL_MASK);
		/*
		 * [Programming Guide]
		 * [I2S] set HD audio
		 */
		if (config->hd_audio)
			afe_msk_write(AFE_MEMIF_HD_CON0, DL1_HD_AUDIO_ON, DL1_HD_AUDIO_MASK);
		else
			afe_msk_write(AFE_MEMIF_HD_CON0, DL1_HD_AUDIO_OFF, DL1_HD_AUDIO_MASK);
		/*
		 * [Programming Guide]
		 * [I2S] set sample rate
		 */
		afe_msk_write(AFE_DAC_CON1, (config->fs << 0), DL1_MODE_MASK);
		/*
		 * [Programming Guide]
		 * [I2S] set MSB and channels
		 */
		afe_msk_write(AFE_DAC_CON3, (config->first_bit << 24) | (config->channel << 16),
			      DL1_MSB_OR_LSB_FIRST_MASK | DL1_DATA_MASK);
		/*No need*/
		afe_msk_write(AFE_DAC_CON5, (config->dsd_width << 0), DL1_DSD_WIDTH_MASK);
		/*
		 * [Programming Guide]
		 * [I2S] set base address and end address
		 */
		afe_write(AFE_DL1_BASE, base);
		afe_write(AFE_DL1_END, end);
		break;
	case AFE_MEM_DL2:
		afe_msk_write(AFE_MEMIF_PBUF_SIZE, DLMCH_OUT_SEL_PAIR_INTERLEAVE,
			      DLMCH_OUT_SEL_MASK);
		if (config->hd_audio)
			afe_msk_write(AFE_MEMIF_HD_CON0, DL2_HD_AUDIO_ON, DL2_HD_AUDIO_MASK);
		else
			afe_msk_write(AFE_MEMIF_HD_CON0, DL2_HD_AUDIO_OFF, DL2_HD_AUDIO_MASK);
		afe_msk_write(AFE_DAC_CON1, (config->fs << 5), DL2_MODE_MASK);
		afe_msk_write(AFE_DAC_CON3, (config->first_bit << 25) | (config->channel << 17),
			      DL2_MSB_OR_LSB_FIRST_MASK | DL2_DATA_MASK);
		afe_msk_write(AFE_DAC_CON5, (config->dsd_width << 2), DL2_DSD_WIDTH_MASK);
		afe_write(AFE_DL2_BASE, base);
		afe_write(AFE_DL2_END, end);
		break;
	case AFE_MEM_DL3:
		afe_msk_write(AFE_MEMIF_PBUF_SIZE, DLMCH_OUT_SEL_PAIR_INTERLEAVE,
			      DLMCH_OUT_SEL_MASK);
		if (config->hd_audio)
			afe_msk_write(AFE_MEMIF_HD_CON0, DL3_HD_AUDIO_ON, DL3_HD_AUDIO_MASK);
		else
			afe_msk_write(AFE_MEMIF_HD_CON0, DL3_HD_AUDIO_OFF, DL3_HD_AUDIO_MASK);
		afe_msk_write(AFE_DAC_CON1, (config->fs << 10), DL3_MODE_MASK);
		afe_msk_write(AFE_DAC_CON3, (config->first_bit << 26) | (config->channel << 18),
			      DL3_MSB_OR_LSB_FIRST_MASK | DL3_DATA_MASK);
		afe_msk_write(AFE_DAC_CON5, (config->dsd_width << 4), DL3_DSD_WIDTH_MASK);
		afe_write(AFE_DL3_BASE, base);
		afe_write(AFE_DL3_END, end);
		break;
	case AFE_MEM_DL4:
		afe_msk_write(AFE_MEMIF_PBUF_SIZE, DLMCH_OUT_SEL_PAIR_INTERLEAVE,
			      DLMCH_OUT_SEL_MASK);
		if (config->hd_audio)
			afe_msk_write(AFE_MEMIF_HD_CON0, DL4_HD_AUDIO_ON, DL4_HD_AUDIO_MASK);
		else
			afe_msk_write(AFE_MEMIF_HD_CON0, DL4_HD_AUDIO_OFF, DL4_HD_AUDIO_MASK);
		afe_msk_write(AFE_DAC_CON1, (config->fs << 15), DL4_MODE_MASK);
		afe_msk_write(AFE_DAC_CON3, (config->first_bit << 27) | (config->channel << 19),
			      DL4_MSB_OR_LSB_FIRST_MASK | DL4_DATA_MASK);
		afe_msk_write(AFE_DAC_CON5, (config->dsd_width << 6), DL4_DSD_WIDTH_MASK);
		afe_write(AFE_DL4_BASE, base);
		afe_write(AFE_DL4_END, end);
		break;
	case AFE_MEM_DL5:
		afe_msk_write(AFE_MEMIF_PBUF_SIZE, DLMCH_OUT_SEL_PAIR_INTERLEAVE,
			      DLMCH_OUT_SEL_MASK);
		if (config->hd_audio)
			afe_msk_write(AFE_MEMIF_HD_CON0, DL5_HD_AUDIO_ON, DL5_HD_AUDIO_MASK);
		else
			afe_msk_write(AFE_MEMIF_HD_CON0, DL5_HD_AUDIO_OFF, DL5_HD_AUDIO_MASK);
		afe_msk_write(AFE_DAC_CON1, (config->fs << 20), DL5_MODE_MASK);
		afe_msk_write(AFE_DAC_CON3, (config->first_bit << 28) | (config->channel << 20),
			      DL5_MSB_OR_LSB_FIRST_MASK | DL5_DATA_MASK);
		afe_msk_write(AFE_DAC_CON5, (config->dsd_width << 8), DL5_DSD_WIDTH_MASK);
		afe_write(AFE_DL5_BASE, base);
		afe_write(AFE_DL5_END, end);
		break;
	case AFE_MEM_DL6:
		afe_msk_write(AFE_MEMIF_PBUF_SIZE, DLMCH_OUT_SEL_PAIR_INTERLEAVE,
			      DLMCH_OUT_SEL_MASK);
		if (config->hd_audio)
			afe_msk_write(AFE_MEMIF_HD_CON0, DL6_HD_AUDIO_ON, DL6_HD_AUDIO_MASK);
		else
			afe_msk_write(AFE_MEMIF_HD_CON0, DL6_HD_AUDIO_OFF, DL6_HD_AUDIO_MASK);
		afe_msk_write(AFE_DAC_CON1, (config->fs << 25), DL6_MODE_MASK);
		afe_msk_write(AFE_DAC_CON3, (config->first_bit << 29) | (config->channel << 21),
			      DL6_MSB_OR_LSB_FIRST_MASK | DL6_DATA_MASK);
		afe_msk_write(AFE_DAC_CON5, (config->dsd_width << 10), DL6_DSD_WIDTH_MASK);
		afe_write(AFE_DL6_BASE, base);
		afe_write(AFE_DL6_END, end);
		break;
	case AFE_MEM_DLMCH:
		/*
		 * [Programming Guide]
		 * [I2S] set i2s data from DLMch
		 */
		afe_msk_write(AFE_MEMIF_PBUF_SIZE, DLMCH_OUT_SEL_FULL_INTERLEAVE,
			      DLMCH_OUT_SEL_MASK);
		/* jiechao.wei solves zero data problem */
		afe_msk_write(AFE_MEMIF_PBUF_SIZE, DLMCH_PBUF_SIZE_32BYTES, DLMCH_PBUF_SIZE_MASK);
		/*
		 * [Programming Guide]
		 * [I2S] set output data bit width
		 */
		afe_msk_write(AFE_MEMIF_PBUF_SIZE, (config->hd_audio << 28), DLMCH_BIT_WIDTH_MASK);
		/* jiechao.wei said DLMCH uses DL1's mode */
		/*
		 * [Programming Guide]
		 * [I2S] set DLMCH sample rate
		 */
		afe_msk_write(AFE_DAC_CON1, (config->fs << 0), DL1_MODE_MASK);
		/*
		 * [Programming Guide]
		 * [I2S] set DLMCH out channel number assignement
		 */
		afe_msk_write(AFE_MEMIF_PBUF_SIZE, (config->dlmch_ch_num << 24), DLMCH_CH_NUM_MASK);
		afe_write(AFE_DLMCH_BASE, base);
		afe_write(AFE_DLMCH_END, end);
		break;
	case AFE_MEM_ARB1:
		if (config->hd_audio)
			afe_msk_write(AFE_MEMIF_HD_CON0, ARB1_HD_AUDIO_ON, ARB1_HD_AUDIO_MASK);
		else
			afe_msk_write(AFE_MEMIF_HD_CON0, ARB1_HD_AUDIO_OFF, ARB1_HD_AUDIO_MASK);
		afe_msk_write(AFE_DAC_CON3, (config->fs << 10), ARB1_MODE_MASK);
		afe_msk_write(AFE_DAC_CON3, (config->channel << 22), ARB1_DATA_MASK);
		afe_msk_write(AFE_DAC_CON5, (config->dsd_width << 12), ARB1_DSD_WIDTH_MASK);
		afe_write(AFE_ARB1_BASE, base);
		afe_write(AFE_ARB1_END, end);
		break;
	case AFE_MEM_DSDR:
		if (config->hd_audio)
			afe_msk_write(AFE_MEMIF_HD_CON0, DSDR_HD_AUDIO_ON, DSDR_HD_AUDIO_MASK);
		else
			afe_msk_write(AFE_MEMIF_HD_CON0, DSDR_HD_AUDIO_OFF, DSDR_HD_AUDIO_MASK);
		afe_msk_write(AFE_DAC_CON3, (config->first_bit << 30) | (config->channel << 23),
			      DSDR_MSB_OR_LSB_FIRST_MASK | DSDR_DATA_MASK);
		afe_msk_write(AFE_DAC_CON5, (config->dsd_width << 14), DSDR_DSD_WIDTH_MASK);
		afe_write(AFE_DSDR_BASE, base);
		afe_write(AFE_DSDR_END, end);
		break;
	case AFE_MEM_UL1:
		afe_msk_write(AFE_DAC_CON2, (config->fs << 0), VUL_MODE_MASK);
		afe_msk_write(AFE_DAC_CON4, (config->first_bit << 24) | (config->channel << 0),
			      VUL_MSB_OR_LSB_FIRST_MASK | VUL_DATA_MASK);
		if (config->channel == MONO)
			afe_msk_write(AFE_DAC_CON4, (config->mono_sel << 1), VUL_R_MONO_MASK);
		afe_msk_write(AFE_MEMIF_HD_CON1, (config->hd_audio << 0), VUL_HD_AUDIO_MASK);
		afe_msk_write(AFE_DAC_CON5, (config->dsd_width << 16), VUL_DSD_WIDTH_MASK);
		afe_write(AFE_VUL_BASE, base);
		afe_write(AFE_VUL_END, end);
		break;
	case AFE_MEM_UL2:
		afe_msk_write(AFE_DAC_CON2, (config->fs << 5), UL2_MODE_MASK);
		afe_msk_write(AFE_DAC_CON4, (config->first_bit << 25) | (config->channel << 2),
			      UL2_MSB_OR_LSB_FIRST_MASK | UL2_DATA_MASK);
		if (config->channel == MONO)
			afe_msk_write(AFE_DAC_CON4, (config->mono_sel << 3), UL2_R_MONO_MASK);
		afe_msk_write(AFE_MEMIF_HD_CON1, (config->hd_audio << 2), UL2_HD_AUDIO_MASK);
		afe_msk_write(AFE_DAC_CON5, (config->dsd_width << 18), UL2_DSD_WIDTH_MASK);
		afe_write(AFE_UL2_BASE, base);
		afe_write(AFE_UL2_END, end);
		break;
	case AFE_MEM_UL3:
		afe_msk_write(AFE_DAC_CON2, (config->fs << 10), UL3_MODE_MASK);
		afe_msk_write(AFE_DAC_CON4, (config->first_bit << 26) | (config->channel << 4),
			      UL3_MSB_OR_LSB_FIRST_MASK | UL3_DATA_MASK);
		if (config->channel == MONO)
			afe_msk_write(AFE_DAC_CON4, (config->mono_sel << 5), UL3_R_MONO_MASK);
		afe_msk_write(AFE_MEMIF_HD_CON1, (config->hd_audio << 4), UL3_HD_AUDIO_MASK);
		afe_msk_write(AFE_DAC_CON5, (config->dsd_width << 20), UL3_DSD_WIDTH_MASK);
		afe_write(AFE_UL3_BASE, base);
		afe_write(AFE_UL3_END, end);
		break;
	case AFE_MEM_UL4:
		afe_msk_write(AFE_DAC_CON2, (config->fs << 15), UL4_MODE_MASK);
		afe_msk_write(AFE_DAC_CON4, (config->first_bit << 27) | (config->channel << 6),
			      UL4_MSB_OR_LSB_FIRST_MASK | UL4_DATA_MASK);
		if (config->channel == MONO)
			afe_msk_write(AFE_DAC_CON4, (config->mono_sel << 7), UL4_R_MONO_MASK);
		afe_msk_write(AFE_MEMIF_HD_CON1, (config->hd_audio << 6), UL4_HD_AUDIO_MASK);
		afe_msk_write(AFE_DAC_CON5, (config->dsd_width << 22), UL4_DSD_WIDTH_MASK);
		afe_write(AFE_UL4_BASE, base);
		afe_write(AFE_UL4_END, end);
		break;
	case AFE_MEM_UL5:
		afe_msk_write(AFE_DAC_CON2, (config->fs << 20), UL5_MODE_MASK);
		afe_msk_write(AFE_DAC_CON4, (config->first_bit << 28) | (config->channel << 8),
			      UL5_MSB_OR_LSB_FIRST_MASK | UL5_DATA_MASK);
		if (config->channel == MONO)
			afe_msk_write(AFE_DAC_CON4, (config->mono_sel << 9), UL5_R_MONO_MASK);
		afe_msk_write(AFE_MEMIF_HD_CON1, (config->hd_audio << 8), UL5_HD_AUDIO_MASK);
		afe_msk_write(AFE_DAC_CON5, (config->dsd_width << 24), UL5_DSD_WIDTH_MASK);
		afe_write(AFE_UL5_BASE, base);
		afe_write(AFE_UL5_END, end);
		break;
	case AFE_MEM_UL6:
		afe_msk_write(AFE_DAC_CON2, (config->fs << 25), UL6_MODE_MASK);
		afe_msk_write(AFE_DAC_CON4, (config->first_bit << 29) | (config->channel << 10),
			      UL6_MSB_OR_LSB_FIRST_MASK | UL6_DATA_MASK);
		if (config->channel == MONO)
			afe_msk_write(AFE_DAC_CON4, (config->mono_sel << 11), UL6_R_MONO_MASK);
		afe_msk_write(AFE_MEMIF_HD_CON1, (config->hd_audio << 10), UL6_HD_AUDIO_MASK);
		afe_msk_write(AFE_DAC_CON5, (config->dsd_width << 26), UL6_DSD_WIDTH_MASK);
		afe_write(AFE_UL6_BASE, base);
		afe_write(AFE_UL6_END, end);
		break;
	case AFE_MEM_ULMCH:
		/*
		 * [Programming Guide]
		 * [SPDIF IN]MEMIF Config
		 */
		afe_msk_write(AFE_DAC_CON4, (config->channel << 12), ULMCH_DATA_MASK);
		if (config->channel == MONO)
			afe_msk_write(AFE_DAC_CON4, (config->mono_sel << 13), ULMCH_R_MONO_MASK);
		afe_msk_write(AFE_MEMIF_HD_CON1, (config->hd_audio << 12), ULMCH_HD_AUDIO_MASK);
		afe_write(AFE_ULMCH_BASE, base);
		afe_write(AFE_ULMCH_END, end);
		break;
	case AFE_MEM_AWB:
		afe_msk_write(AFE_DAC_CON3, (config->fs << 0), AWB_MODE_MASK);
		afe_msk_write(AFE_DAC_CON4, (config->first_bit << 30) | (config->channel << 16),
			      AWB_MSB_OR_LSB_FIRST_MASK | AWB_DATA_MASK);
		if (config->channel == MONO)
			afe_msk_write(AFE_DAC_CON4, (config->mono_sel << 17), AWB_R_MONO_MASK);
		afe_msk_write(AFE_MEMIF_HD_CON1, (config->hd_audio << 14), AWB_HD_AUDIO_MASK);
		afe_msk_write(AFE_DAC_CON5, (config->dsd_width << 28), AWB_DSD_WIDTH_MASK);
		afe_write(AFE_AWB_BASE, base);
		afe_write(AFE_AWB_END, end);
		break;
	case AFE_MEM_AWB2:
		afe_msk_write(AFE_DAC_CON3, (config->fs << 5), AWB2_MODE_MASK);
		afe_msk_write(AFE_DAC_CON4, (config->first_bit << 31) | (config->channel << 18),
			      AWB2_MSB_OR_LSB_FIRST_MASK | AWB2_DATA_MASK);
		if (config->channel == MONO)
			afe_msk_write(AFE_DAC_CON4, (config->mono_sel << 19), AWB2_R_MONO_MASK);
		afe_msk_write(AFE_MEMIF_HD_CON1, (config->hd_audio << 16), AWB2_HD_AUDIO_MASK);
		afe_msk_write(AFE_DAC_CON5, (config->dsd_width << 30), AWB2_DSD_WIDTH_MASK);
		afe_write(AFE_AWB2_BASE, base);
		afe_write(AFE_AWB2_END, end);
		break;
	case AFE_MEM_DAI:
		afe_msk_write(AFE_DAC_CON2, (config->daimod_fs << 30), DAI_MODE_MASK);
		afe_msk_write(AFE_DAC_CON4, (config->dup_write << 14), DAI_DUP_WR_MASK);
		afe_write(AFE_DAI_BASE, base);
		afe_write(AFE_DAI_END, end);
		break;
	case AFE_MEM_MOD_PCM:
		afe_msk_write(AFE_DAC_CON2, (config->daimod_fs << 31), MOD_PCM_MODE_MASK);
		afe_msk_write(AFE_DAC_CON4, (config->dup_write << 15), MOD_PCM_DUP_WR_MASK);
		afe_write(AFE_MOD_PCM_BASE, base);
		afe_write(AFE_MOD_PCM_END, end);
		break;
	case AFE_MEM_DSDW:
		afe_msk_write(AFE_DAC_CON3, (config->first_bit << 31), DSDW_MSB_OR_LSB_FIRST_MASK);
		afe_msk_write(AFE_DAC_CON4, (config->channel << 20), DSDW_DATA_MASK);
		if (config->channel == MONO)
			afe_msk_write(AFE_DAC_CON4, (config->mono_sel << 21), DSDW_R_MONO_MASK);
		afe_msk_write(AFE_MEMIF_HD_CON1, (config->hd_audio << 18), DSDW_HD_AUDIO_MASK);
		afe_msk_write(AFE_DAC_CON4, (config->dsd_width << 22), DSDW_DSD_WIDTH_MASK);
		afe_write(AFE_DSDW_BASE, base);
		afe_write(AFE_DSDW_END, end);
		break;
	default:
		pr_err("%s() error: wrong memif %d\n", __func__, memif);
		return -EINVAL;
	}
	return 0;
}
/*
 * [Programming Guide]
 * playback/capture MEMIF current point read
 */
int afe_memif_pointer(enum afe_mem_interface memif, u32 *cur_ptr)
{
	static const u32 regs[] = {
		0,		/* AFE_MEM_NONE    =  0, */
		AFE_DL1_CUR,	/* AFE_MEM_DL1     =  1, */
		AFE_DL2_CUR,	/* AFE_MEM_DL2     =  2, */
		AFE_DL3_CUR,	/* AFE_MEM_DL3     =  3, */
		AFE_DL4_CUR,	/* AFE_MEM_DL4     =  4, */
		AFE_DL5_CUR,	/* AFE_MEM_DL5     =  5, */
		AFE_DL6_CUR,	/* AFE_MEM_DL6     =  6, */
		AFE_DLMCH_CUR,	/* AFE_MEM_DLMCH   =  7, */
		AFE_ARB1_CUR,	/* AFE_MEM_ARB1  =  8, */
		AFE_DSDR_CUR,	/* AFE_MEM_DSDR    =  9, */
		AFE_VUL_CUR,	/* AFE_MEM_UL1     = 10, */
		AFE_UL2_CUR,	/* AFE_MEM_UL2     = 11, */
		AFE_UL3_CUR,	/* AFE_MEM_UL3     = 12, */
		AFE_UL4_CUR,	/* AFE_MEM_UL4     = 13, */
		AFE_UL5_CUR,	/* AFE_MEM_UL5     = 14, */
		AFE_UL6_CUR,	/* AFE_MEM_UL6     = 15, */
		AFE_ULMCH_CUR,	/* AFE_MEM_ULMCH   = 16, */
		AFE_DAI_CUR,	/* AFE_MEM_DAI     = 17, */
		AFE_MOD_PCM_CUR,	/* AFE_MEM_MOD_PCM = 18, */
		0,		/* AFE_MEM_RESERVED      = 19, */
		AFE_AWB_CUR,	/* AFE_MEM_AWB     = 20, */
		AFE_AWB2_CUR,	/* AFE_MEM_AWB2    = 21, */
		AFE_DSDW_CUR,	/* AFE_MEM_DSDW    = 22 */
	};

	if (memif >= AFE_MEM_NUM || regs[memif] == 0) {
		pr_err("%s() error: invalid memif %u\n", __func__, memif);
		return -EINVAL;
	}
	if (!(afe_read(AFE_DAC_CON0) & (0x1 << memif))) /* memif has not been enabled */
		return -EPERM;
	if (cur_ptr == NULL)
		return -ENOMEM;
	*cur_ptr = afe_read(regs[memif]);
	return 0;
}

int memif_enable_clk(int memif_id, int on)
{
	int off = !on;

	if (memif_id == AFE_MEM_NONE) {
		pr_err("%s() error: memif_id %d\n", __func__, memif_id);
		return -EINVAL;
	}

	if (memif_id <= AFE_MEM_ARB1) /* AFE_MEM_DL1~AFE_MEM_ARB1 */
		afe_msk_write(AUDIO_TOP_CON5, off << (memif_id+5), 1 << (memif_id+5));
	else if (memif_id >= AFE_MEM_UL1 && memif_id <= AFE_MEM_UL6)
		afe_msk_write(AUDIO_TOP_CON5, off << (memif_id - 10), 1 << (memif_id - 10));
	else if (memif_id == AFE_MEM_DAI)
		afe_msk_write(AUDIO_TOP_CON5, off << 16, 1 << 16);
	else if (memif_id == AFE_MEM_AWB)
		afe_msk_write(AUDIO_TOP_CON5, off << 14, 1 << 14);
	else if (memif_id == AFE_MEM_AWB2)
		afe_msk_write(AUDIO_TOP_CON5, off << 15, 1 << 15);
	else
		pr_err("%s() error: memif_id %d\n", __func__, memif_id);

	if (memif_id == AFE_MEM_DLMCH) {
		if (on) {
			afe_msk_write(AUDIO_TOP_CON5, 0, PDN_MEMIF_DL1);
			afe_msk_write(AUDIO_TOP_CON5, 0, PDN_MEMIF_DL2);
			afe_msk_write(AUDIO_TOP_CON5, 0, PDN_MEMIF_DL3);
			afe_msk_write(AUDIO_TOP_CON5, 0, PDN_MEMIF_DL4);
			afe_msk_write(AUDIO_TOP_CON5, 0, PDN_MEMIF_DL5);
		} else {
			afe_msk_write(AUDIO_TOP_CON5, PDN_MEMIF_DL1, PDN_MEMIF_DL1);
			afe_msk_write(AUDIO_TOP_CON5, PDN_MEMIF_DL2, PDN_MEMIF_DL2);
			afe_msk_write(AUDIO_TOP_CON5, PDN_MEMIF_DL3, PDN_MEMIF_DL3);
			afe_msk_write(AUDIO_TOP_CON5, PDN_MEMIF_DL4, PDN_MEMIF_DL4);
			afe_msk_write(AUDIO_TOP_CON5, PDN_MEMIF_DL5, PDN_MEMIF_DL5);
		}
	}
	return 0;
}

int afe_memif_base(enum afe_mem_interface memif, u32 *base)
{
	static const u32 regs[] = {
		0,		/* AFE_MEM_NONE    =  0, */
		AFE_DL1_BASE,	/* AFE_MEM_DL1     =  1, */
		AFE_DL2_BASE,	/* AFE_MEM_DL2     =  2, */
		AFE_DL3_BASE,	/* AFE_MEM_DL3     =  3, */
		AFE_DL4_BASE,	/* AFE_MEM_DL4     =  4, */
		AFE_DL5_BASE,	/* AFE_MEM_DL5     =  5, */
		AFE_DL6_BASE,	/* AFE_MEM_DL6     =  6, */
		AFE_DLMCH_BASE,	/* AFE_MEM_DLMCH   =  7, */
		AFE_ARB1_BASE,	/* AFE_MEM_ARB1  =  8, */
		AFE_DSDR_BASE,	/* AFE_MEM_DSDR    =  9, */
		AFE_VUL_BASE,	/* AFE_MEM_UL1     = 10, */
		AFE_UL2_BASE,	/* AFE_MEM_UL2     = 11, */
		AFE_UL3_BASE,	/* AFE_MEM_UL3     = 12, */
		AFE_UL4_BASE,	/* AFE_MEM_UL4     = 13, */
		AFE_UL5_BASE,	/* AFE_MEM_UL5     = 14, */
		AFE_UL6_BASE,	/* AFE_MEM_UL6     = 15, */
		AFE_ULMCH_BASE,	/* AFE_MEM_ULMCH   = 16, */
		AFE_DAI_BASE,	/* AFE_MEM_DAI     = 17, */
		AFE_MOD_PCM_BASE,	/* AFE_MEM_MOD_PCM = 18, */
		0,		/* AFE_MEM_RESERVED      = 19, */
		AFE_AWB_BASE,	/* AFE_MEM_AWB     = 20, */
		AFE_AWB2_BASE,	/* AFE_MEM_AWB2    = 21, */
		AFE_DSDW_BASE,	/* AFE_MEM_DSDW    = 22 */
	};

	if (memif >= AFE_MEM_NUM || regs[memif] == 0) {
		pr_err("%s() error: invalid memif %u\n", __func__, memif);
		return -EINVAL;
	}
	if (base == NULL)
		return -ENOMEM;
	*base = afe_read(regs[memif]);
	return 0;
}


/******************** irq ********************/

u32 afe_irq_status(void)
{
	return afe_read(AFE_IRQ_STATUS);
}

void afe_irq_clear(u32 status)
{
	afe_msk_write(AFE_IRQ_CLR, status, IRQ_CLR_ALL);
}

u32 asys_irq_status(void)
{
	return afe_read(ASYS_IRQ_STATUS);
}

void asys_irq_clear(u32 status)
{
	afe_msk_write(ASYS_IRQ_CLR, status, ASYS_IRQ_CLR_ALL);
}

int audio_irq_configurate(enum audio_irq_id id, const struct audio_irq_config *config)
{
	u32 fs_mode;

	switch (config->mode) {
	case FS_8000HZ:
		fs_mode = 0;
		break;
	case FS_12000HZ:
		fs_mode = 1;
		break;
	case FS_16000HZ:
		fs_mode = 2;
		break;
	case FS_24000HZ:
		fs_mode = 3;
		break;
	case FS_32000HZ:
		fs_mode = 4;
		break;
	case FS_48000HZ:
		fs_mode = 5;
		break;
	case FS_96000HZ:
		fs_mode = 6;
		break;
	case FS_192000HZ:
		fs_mode = 7;
		break;
	case FS_11025HZ:
		fs_mode = 9;
		break;
	case FS_22050HZ:
		fs_mode = 0xb;
		break;
	case FS_44100HZ:
		fs_mode = 0xd;
		break;
	case FS_88200HZ:
		fs_mode = 0xe;
		break;
	case FS_176400HZ:
		fs_mode = 0xf;
		break;
	default:
		fs_mode = 5;
		break;
	}
	pr_debug("%s() id=%d, mode=%u, init_val=%u\n", __func__, id, config->mode, config->init_val);
	if (id >= IRQ_ASYS_IRQ1 && id <= IRQ_ASYS_IRQ16) {
		u32 addrCON;

		addrCON = ASYS_IRQ1_CON + (id - IRQ_ASYS_IRQ1) * 4;
		afe_msk_write(addrCON,
			(config->mode << 24) | (config->init_val << 0),
			ASYS_IRQ_MODE_MASK | ASYS_IRQ_CNT_INIT_MASK);
	}
	/* hdmi-i2s-hdmi-tx irq counter */
	else if (id == IRQ_AFE_HDMI)
		afe_msk_write(AFE_IRQ_CNT5, config->init_val << 0, 0xffffffff);
	else if (id == IRQ_AFE_IRQ1) {
		afe_msk_write(AFE_IRQ_CON, fs_mode << 4, IRQ1_FS_MASK);
		afe_msk_write(AFE_IRQ_CNT1, config->init_val, IRQ_CNT1_MASK);
	} else if (id == IRQ_AFE_IRQ2) {
		afe_msk_write(AFE_IRQ_CON, fs_mode << 8, IRQ2_FS_MASK);
		afe_msk_write(AFE_IRQ_CNT2, config->init_val, IRQ_CNT2_MASK);
	}
	return 0;
}

int audio_irq_enable(enum audio_irq_id id, int en)
{
	en = !!en;
	pr_debug("%s() %s for id=%d\n", __func__, en ? "enable" : "disable", id);
	if (id >= IRQ_ASYS_IRQ1 && id <= IRQ_ASYS_IRQ16) {
		u32 addrCON;

		addrCON = ASYS_IRQ1_CON + (id - IRQ_ASYS_IRQ1) * 4;
		afe_msk_write(addrCON, en << 31, 0x1 << 31);
	} else if (id == IRQ_AFE_SPDIF2)
		afe_msk_write(AFE_IRQ_CON, (en << 3), IRQ4_ON);
	/* hdmi-i2s-hdmi-tx */
	else if (id == IRQ_AFE_HDMI)
		afe_msk_write(AFE_IRQ_CON, (en << 12), IRQ5_ON);
	/* hdmi-iec-hdmi-tx */
	else if (id == IRQ_AFE_SPDIF)
		afe_msk_write(AFE_IRQ_CON, (en << 13), IRQ6_ON);
	else if (id == IRQ_AFE_IRQ1)
		afe_msk_write(AFE_IRQ_CON, (en << 0), IRQ1_ON);
	else if (id == IRQ_AFE_IRQ2)
		afe_msk_write(AFE_IRQ_CON, (en << 1), IRQ2_ON);
	else if (id == IRQ_AFE_MULTIIN)
		/*
		 * [Programming Guide]
		 * [SPDIF IN] IRQ3 enable
		 */
		afe_msk_write(AFE_IRQ_CON, (en << 2), IRQ_MULTI_ON);
	else {
		pr_warn("%s() %s for id=%d has not been implemented\n",
			__func__, en ? "enable" : "disable", id);
		return -EINVAL;
	}
	return 0;
}

/*
 * audio_irq_cnt_mon()
 * Return value: 0            - invalid
 *               1 ~ init_val - how many samples remained before triggering interrupt
 */
u32 audio_irq_cnt_mon(enum audio_irq_id id)
{
	u32 addr = 0, mask;

	if (id >= IRQ_ASYS_IRQ1 && id <= IRQ_ASYS_IRQ16) {
		u32 asys_irq_mon_sel = (id - IRQ_ASYS_IRQ1) << 0;

		afe_msk_write(ASYS_IRQ_CONFIG, asys_irq_mon_sel, ASYS_IRQ_MON_SEL_MASK);
		addr = ASYS_IRQ_MON2;
		mask = IRQ_CNT_STATUS_MASK;
	} else if (id >= IRQ_AFE_IRQ1 && id <= IRQ_AFE_IRQ2) {
		addr = AFE_IRQ1_MCU_CNT_MON + (id - IRQ_AFE_IRQ1) * 4;
		mask = AFE_IRQ_MCU_CNT_MON_MASK;
	}
	if (addr) {
		while (1) {
			u32 tmp1, tmp2;

			/*
			 * hailang.deng said the monitor value should be read twice,
			 * it is valid only if they are equal
			 */
			tmp1 = afe_read(addr) & mask;
			tmp2 = afe_read(addr) & mask;
			if (tmp1 == tmp2)
				return tmp1;
		}
	} else
		return 0;
}

static DEFINE_MUTEX(asys_irqs_using_lock);
#define ASYS_IRQ_NUM (16)
static int asys_irqs_using[ASYS_IRQ_NUM];

enum audio_irq_id asys_irq_acquire(void)
{
	int i;

	mutex_lock(&asys_irqs_using_lock);
	for (i = 0; i < ASYS_IRQ_NUM; ++i) {
		if (!asys_irqs_using[i]) {
			asys_irqs_using[i] = 1;
			mutex_unlock(&asys_irqs_using_lock);
			pr_debug("%s() %d is acquired\n", __func__, i);
			return IRQ_ASYS_IRQ1 + i;
		}
	}
	mutex_unlock(&asys_irqs_using_lock);
	pr_debug("%s() nothing is acquired\n", __func__);
	return IRQ_NUM;
}

void asys_irq_release(enum audio_irq_id id)
{

	if (id >= IRQ_ASYS_IRQ1 && id < IRQ_ASYS_IRQ1 + ASYS_IRQ_NUM) {
		int i = id - IRQ_ASYS_IRQ1;

		mutex_lock(&asys_irqs_using_lock);
		asys_irqs_using[i] = 0;
		mutex_unlock(&asys_irqs_using_lock);
		pr_debug("%s() %d is released\n", __func__, i);
	}
}


/******************** i2s ********************/
#ifdef CONFIG_MTK_LEGACY_CLOCK
int afe_i2s_power_on_mclk(int id, int on)
{
	u32 val, msk;

	if (id < 0 || id > 5) {
		pr_err("%s() error: i2s id %d\n", __func__, id);
		return -EINVAL;
	}
	val = (!on) << (23 + id);
	msk = 0x1 << (23 + id);
	#ifdef AUDIO_MEM_IOREMAP
	topckgen_msk_write(CLK_AUDDIV_3, val, msk);
	#else
	afe_msk_write(CLK_AUDDIV_3, val, msk);
	#endif
	return 0;
}

void afe_i2s_mclk_configurate(int id, int mclk)
{
	u32 addr, pos;
	int div;
	u32 domain_pos;
	int domain;

	switch (id) {
	case 0:
		addr = CLK_AUDDIV_1;
		pos = 0;
		domain_pos = 15;
		break;
	case 1:
		addr = CLK_AUDDIV_1;
		pos = 8;
		domain_pos = 16;
		break;
	case 2:
		addr = CLK_AUDDIV_1;
		pos = 16;
		domain_pos = 17;
		break;
	case 3:
		addr = CLK_AUDDIV_1;
		pos = 24;
		domain_pos = 18;
		break;
	case 4:
		addr = CLK_AUDDIV_2;
		pos = 0;
		domain_pos = 19;
		break;
	case 5:
		addr = CLK_AUDDIV_2;
		pos = 8;
		domain_pos = 20;
		break;
	default:
		return;
	}
	if (mclk <= 0) {
		pr_err("%s() error: i2s id %d, bad mclk %d\n", __func__, id, mclk);
		return;
	}
	if ((98304000 % mclk) == 0) {
		div = 98304000 / mclk;
		domain = 0;
	} else if ((90316800 % mclk) == 0) {
		div = 90316800 / mclk;
		domain = 1;
	} else {
		div = 1;
		domain = 0;
		pr_err("%s() error: i2s id %d, bad mclk %d\n", __func__, id, mclk);
		return;
	}
	#ifdef AUDIO_MEM_IOREMAP
	topckgen_msk_write(addr, (div - 1) << pos, 0xff << pos);
	topckgen_msk_write(CLK_AUDDIV_3, domain << domain_pos, 0x1 << domain_pos);
	#else
	afe_msk_write(addr, (div - 1) << pos, 0xff << pos);
	afe_msk_write(CLK_AUDDIV_3, domain << domain_pos, 0x1 << domain_pos);
	#endif
}
#else
void afe_i2s_mclk_configurate(int id, int mclk)
{
	int domain;

	if (mclk <= 0) {
		pr_err("%s() error: i2s id %d, bad mclk %d\n", __func__, id, mclk);
		return;
	}
	if ((98304000 % mclk) == 0) {
		domain = 0;
	} else if ((90316800 % mclk) == 0) {
		domain = 1;
	} else {
		domain = 0;
		pr_err("%s() error: i2s id %d, bad mclk %d\n", __func__, id, mclk);
		return;
	}
	mt_mclk_set(id, domain, mclk);
}
#endif

int afe_i2s_in_configurate(enum afe_i2s_in_id id, const struct afe_i2s_in_config *config)
{
	u32 addr;
	u32 val, msk;
	enum afe_sampling_rate mode;

	if (id >= AFE_I2S_IN_NUM)
		return -EINVAL;
	/*
	 * [Programming Guide]
	 * [I2S] MCLK config
	 */
	if (!config->slave)
		afe_i2s_mclk_configurate(id, config->mclk);
	/*
	* hailang.deng said if slave mode,
	* fs should be set to one of 48k domain fs
	*/
	mode = config->slave ? FS_8000HZ : config->mode;
	addr = ASYS_I2SIN1_CON + id * 4;
	val = (config->fpga_test_loop3 << 23)
	      | (config->fpga_test_loop << 21)
	      | (config->fpga_test_loop2 << 20)
	      | (config->use_asrc << 19)
	      | (config->dsd_mode << 18)
	      | (mode << 8)
	      | (config->slave << 2)
	      | (0x1 << 31)	/* enable phase-shift fix */
	      ;
	if (config->dsd_mode)
		val |= (0x0 << 1);	/* must be 32cycle */
	/* I2S,LJ,RJ are none of business */
	else {
		switch (config->fmt) {
		case FMT_32CYCLE_16BIT_I2S:
			val |= ((0x0 << 1)	/* 32cycle */
				| (0x1 << 3) /* I2S */);
			break;
		case FMT_32CYCLE_16BIT_LJ:
			val |= ((0x0 << 1)	/* 32cycle */
				| (0x0 << 3) | (0x0 << 14)	/* LJ */
				| (0x1 << 5) /* LR Invert */);
			break;
		case FMT_64CYCLE_16BIT_I2S:
		case FMT_64CYCLE_32BIT_I2S:
			val |= ((0x1 << 1)	/* 64cycle */
				| (0x1 << 3) /* I2S */);
			break;
		case FMT_64CYCLE_16BIT_LJ:
		case FMT_64CYCLE_32BIT_LJ:
			val |= ((0x1 << 1)	/* 64cycle */
				| (0x0 << 3) | (0x0 << 14)	/* LJ */
				| (0x1 << 5) /* LR Invert */);
			break;
		case FMT_64CYCLE_16BIT_RJ:
			val |= ((0x1 << 1)	/* 64cycle */
				| (0x0 << 3) | (0x1 << 14)	/* RJ */
				| (0x1 << 5)	/* LR Invert */
				| (0 << 13) /* 16bit */);
			break;
		case FMT_64CYCLE_24BIT_RJ:
			val |= ((0x1 << 1)	/* 64cycle */
				| (0x0 << 3) | (0x1 << 14)	/* RJ */
				| (0x1 << 5)	/* LR Invert */
				| (1 << 13) /* 24bit */);
			break;
		default:
			break;
		}
	}
	msk = (0x1 << 23)	/* fpga_test_loop3 */
	      | (0x1 << 21)	/* fpga_test_loop */
	      | (0x1 << 20)	/* fpga_test_loop2 */
	      | (0x1 << 19)	/* use_asrc */
	      | (0x1 << 18)	/* dsdMode */
	      | (0x1 << 13)	/* RJ 16bit/24bit */
	      | (0x1F << 8)	/* mode */
	      | (0x1 << 2)	/* slave */
	      | (0x1 << 1)	/* cycle */
	      | (0x1 << 3) | (0x1 << 14)/* fmt */
	      | (0x1 << 5)		/* LR Invert */
	      | (0x1 << 31);	/* phase-shift fix */
	/*
	 * [Programming Guide]
	 * [I2S] I2S in config
	 */
	afe_msk_write(addr, val, msk);
	return 0;
}

int afe_i2s_in_enable(enum afe_i2s_in_id id, int en)
{
	unsigned long flags;
	static DEFINE_SPINLOCK(lock);
	static int status[AFE_I2S_IN_NUM];
	u32 addr;

	if (id >= AFE_I2S_IN_NUM)
		return -EINVAL;
	en = !!en;
	addr = ASYS_I2SIN1_CON + id * 4;
	spin_lock_irqsave(&lock, flags);
	if (en) {
		if (status[id] == 0) {
			mt_turn_on_i2sin_clock(id, 1);
			afe_msk_write(addr, (0x1 << 30), (0x1 << 30));
			udelay(1);
			afe_msk_write(addr, (0x0 << 30), (0x1 << 30));
			udelay(1);
			afe_msk_write(addr, (0x1 << 0), (0x1 << 0));
		}
		++status[id];
	} else {
		if (status[id] > 0 && --status[id] == 0) {
			afe_msk_write(addr, (0x0 << 0), (0x1 << 0));
			mt_turn_on_i2sin_clock(id, 0);
		}
	}
	spin_unlock_irqrestore(&lock, flags);
	return 0;
}

int afe_i2s_out_configurate(enum afe_i2s_out_id id, const struct afe_i2s_out_config *config)
{
	u32 addr;
	u32 val, msk;
	enum afe_sampling_rate mode;
	enum afe_i2s_out_dsd_use dsd_use;

	if (id >= AFE_I2S_OUT_NUM)
		return -EINVAL;
	/*
	 * [Programming Guide]
	 * [I2S] MCLK config
	 */
	if (!config->slave)
		afe_i2s_mclk_configurate(id, config->mclk);
	/*
	* hailang.deng said if slave mode,
	* fs should be set to one of 48k domain fs
	*/
	mode = config->slave ? FS_8000HZ : config->mode;
	addr = ASYS_I2SO1_CON + id * 4;
	val = (config->fpga_test_loop << 21)
	      | (config->data_from_sine << 20)
	      | (config->use_asrc << 19)
	      | (config->dsd_mode << 18)
	      | (0 /*config->couple_mode */  << 17)
	      | (config->one_heart_mode << 16)
	      | (mode << 8)
	      | (config->slave << 2);
	if (config->dsd_mode) {
		dsd_use = config->dsd_use;
		val |= (0x0 << 1);	/* must be 32cycle */
		/* I2S,LJ,RJ are none of business */
	} else {
		dsd_use = I2S_OUT_DSD_USE_NORMAL;
		switch (config->fmt) {
		case FMT_32CYCLE_16BIT_I2S:
			val |= ((0x0 << 1)	/* 32cycle */
				| (0x1 << 3) /* I2S */);
			break;
		case FMT_32CYCLE_16BIT_LJ:
			val |= ((0x0 << 1)	/* 32cycle */
				| (0x0 << 3) | (0x0 << 14)	/* LJ */
				| (0x1 << 5) /* LR Invert */);
			break;
		case FMT_64CYCLE_16BIT_I2S:
		case FMT_64CYCLE_32BIT_I2S:
			val |= ((0x1 << 1)	/* 64cycle */
				| (0x1 << 3) /* I2S */);
			break;
		case FMT_64CYCLE_16BIT_LJ:
		case FMT_64CYCLE_32BIT_LJ:
			val |= ((0x1 << 1)	/* 64cycle */
				| (0x0 << 3) | (0x0 << 14)	/* LJ */
				| (0x1 << 5) /* LR Invert */);
			break;
		case FMT_64CYCLE_16BIT_RJ:
			val |= ((0x1 << 1)	/* 64cycle */
				| (0x0 << 3) | (0x1 << 14)	/* RJ */
				| (0x1 << 5)	/* LR Invert */
				| (0 << 13) /* 16bit */);
			break;
		case FMT_64CYCLE_24BIT_RJ:
			val |= ((0x1 << 1)	/* 64cycle */
				| (0x0 << 3) | (0x1 << 14)	/* RJ */
				| (0x1 << 5)	/* LR Invert */
				| (1 << 13) /* 24bit */);
			break;
		default:
			break;
		}
	}
	msk = (0x1 << 21)	/* fpga_test_loop */
	      | (0x1 << 20)	/* data_from_sine */
	      | (0x1 << 19)	/* use_asrc */
	      | (0x1 << 18)	/* dsd_mode */
	      | (0x1 << 17)	/* couple_mode */
	      | (0x1 << 16)	/* one_heart_mode */
	      | (0x1F << 8)	/* mode */
	      | (0x1 << 5)	/* LR Invert */
	      | (0x1 << 2)	/* slave */
	      | (0x1 << 13)	/* RJ 16bit/24bit */
	      | (0x1 << 1)	/* cycle */
	      | (0x1 << 3) | (0x1 << 14);	/* fmt */
	/*
	 * [Programming Guide]
	 * [I2S] I2S out config
	 */
	afe_msk_write(addr, val, msk);
	if (config->dsd_mode) {
		if (id == AFE_I2S_OUT_1) {
			afe_msk_write(ASYS_TOP_CON, dsd_use << 3, I2S1_DSD_USE_MASK);
			/* hailang.deng said
			 * if bypass fader, bck needs invert
			 * if use fader, bck doesn't need invert
			 */
			afe_msk_write(DSD1_FADER_CON0,
				(0x1 << 15) | (0x1 << 14) | (0x0 << 10),
				(0x1 << 15) | (0x1 << 14) | (0x1 << 10));
		} else if (id == AFE_I2S_OUT_2) {
			afe_msk_write(ASYS_TOP_CON, dsd_use << 4, I2S2_DSD_USE_MASK);
			/* hailang.deng said
			 * if bypass fader, bck needs invert
			 * if use fader, bck doesn't need invert
			 */
			afe_msk_write(DSD2_FADER_CON0,
				(0x1 << 15) | (0x1 << 14) | (0x0 << 10),
				(0x1 << 15) | (0x1 << 14) | (0x1 << 10));
		}
	} else {
		if (id == AFE_I2S_OUT_1)
			afe_msk_write(ASYS_TOP_CON, I2S1_DSD_USE_NORMAL, I2S1_DSD_USE_MASK);
		else if (id == AFE_I2S_OUT_2)
			afe_msk_write(ASYS_TOP_CON, I2S2_DSD_USE_NORMAL, I2S2_DSD_USE_MASK);
	}
	return 0;
}

int afe_i2s_out_enable(enum afe_i2s_out_id id, int en)
{
	unsigned long flags;
	static DEFINE_SPINLOCK(lock);
	static int status[AFE_I2S_OUT_NUM];
	u32 addr;
	if (id >= AFE_I2S_OUT_NUM)
		return -EINVAL;
	en = !!en;
	addr = ASYS_I2SO1_CON + id * 4;
	spin_lock_irqsave(&lock, flags);
	if (en) {
		if (status[id] == 0) {
			switch (id) {
			case AFE_I2S_OUT_6:
				afe_power_on_sample_asrc_tx(SAMPLE_ASRC_O6, 1);
				afe_msk_write(PWR2_TOP_CON,
					0x1 << LAT_DATA_EN_I2SO6_POS,
					0x1 << LAT_DATA_EN_I2SO6_POS);
				break;
			case AFE_I2S_OUT_5:
				afe_power_on_sample_asrc_tx(SAMPLE_ASRC_O5, 1);
				afe_msk_write(PWR2_TOP_CON,
					0x1 << LAT_DATA_EN_I2SO5_POS,
					0x1 << LAT_DATA_EN_I2SO5_POS);
				break;
			case AFE_I2S_OUT_4:
				afe_power_on_sample_asrc_tx(SAMPLE_ASRC_O4, 1);
				afe_msk_write(PWR2_TOP_CON,
					0x1 << LAT_DATA_EN_I2SO4_POS,
					0x1 << LAT_DATA_EN_I2SO4_POS);
				break;
			case AFE_I2S_OUT_3:
				afe_power_on_sample_asrc_tx(SAMPLE_ASRC_O3, 1);
				afe_msk_write(PWR2_TOP_CON,
					0x1 << LAT_DATA_EN_I2SO3_POS,
					0x1 << LAT_DATA_EN_I2SO3_POS);
				break;
			case AFE_I2S_OUT_2:
				afe_power_on_sample_asrc_tx(SAMPLE_ASRC_O2, 1);
				afe_msk_write(ASYS_I2SO2_CON,
					0x1 << LAT_DATA_EN_POS,
					0x1 << LAT_DATA_EN_POS);
				break;
			case AFE_I2S_OUT_1: default:
				afe_power_on_sample_asrc_tx(SAMPLE_ASRC_O1, 1);
				afe_msk_write(ASYS_I2SO1_CON,
					0x1 << LAT_DATA_EN_POS,
					0x1 << LAT_DATA_EN_POS);
				break;
			}

			mt_turn_on_i2sout_clock(id, 1);
			afe_msk_write(addr, (0x1 << 30), (0x1 << 30));
			udelay(1);
			afe_msk_write(addr, (0x0 << 30), (0x1 << 30));
			udelay(1);
			afe_msk_write(addr, (0x1 << 0), (0x1 << 0));
		}
		++status[id];
	} else {
		if (status[id] > 0 && --status[id] == 0) {
			afe_msk_write(addr, (0x0 << 0), (0x1 << 0));
			mt_turn_on_i2sout_clock(id, 0);

			switch (id) {
			case AFE_I2S_OUT_6:
				afe_msk_write(PWR2_TOP_CON,
					0x0 << LAT_DATA_EN_I2SO6_POS,
					0x1 << LAT_DATA_EN_I2SO6_POS);
				afe_power_on_sample_asrc_tx(SAMPLE_ASRC_O6, 0);
				break;
			case AFE_I2S_OUT_5:
				afe_msk_write(PWR2_TOP_CON,
					0x0 << LAT_DATA_EN_I2SO5_POS,
					0x1 << LAT_DATA_EN_I2SO5_POS);
				afe_power_on_sample_asrc_tx(SAMPLE_ASRC_O5, 0);
				break;
			case AFE_I2S_OUT_4:
				afe_msk_write(PWR2_TOP_CON,
					0x0 << LAT_DATA_EN_I2SO4_POS,
					0x1 << LAT_DATA_EN_I2SO4_POS);
				afe_power_on_sample_asrc_tx(SAMPLE_ASRC_O4, 0);
				break;
			case AFE_I2S_OUT_3:
				afe_msk_write(PWR2_TOP_CON,
					0x0 << LAT_DATA_EN_I2SO3_POS,
					0x1 << LAT_DATA_EN_I2SO3_POS);
				afe_power_on_sample_asrc_tx(SAMPLE_ASRC_O3, 0);
				break;
			case AFE_I2S_OUT_2:
				afe_msk_write(ASYS_I2SO2_CON,
					0x0 << LAT_DATA_EN_POS,
					0x1 << LAT_DATA_EN_POS);
				afe_power_on_sample_asrc_tx(SAMPLE_ASRC_O2, 0);
				break;
			case AFE_I2S_OUT_1: default:
				afe_msk_write(ASYS_I2SO1_CON,
					0x0 << LAT_DATA_EN_POS,
					0x1 << LAT_DATA_EN_POS);
				afe_power_on_sample_asrc_tx(SAMPLE_ASRC_O1, 0);
				break;
			}
		}
	}
	spin_unlock_irqrestore(&lock, flags);
	return 0;
}

int afe_i2s_out_slave_pcm_configurate(enum afe_i2s_out_id id, enum afe_i2s_format fmt, int use_asrc)
{
	/* configurate i2s-in first, and then i2s-out */
	{
		struct afe_i2s_in_config config = {
			.fpga_test_loop3 = 0,
			.fpga_test_loop = 0,
			.fpga_test_loop2 = 0,
			.use_asrc = 0,
			.dsd_mode = 0,
			.slave = 1,	/* must be slave */
			.fmt = fmt,
			.mclk = 0,
		};

		afe_i2s_in_configurate((enum afe_i2s_in_id)id, &config);
	}
	{
		struct afe_i2s_out_config config = {
			.fpga_test_loop = 0,
			.data_from_sine = 0,
			.use_asrc = use_asrc,
			.dsd_mode = 0,
			.dsd_use = I2S_OUT_DSD_USE_NORMAL,
			.one_heart_mode = 0,
			.slave = 1,	/* slave */
			.fmt = fmt,
			.mclk = 0,
		};

		afe_i2s_out_configurate(id, &config);
	}
	return 0;
}

int afe_i2s_out_slave_pcm_enable(enum afe_i2s_out_id id, int en)
{
	pr_debug("%s enable I2S %d\n", __func__, id);
	return afe_i2s_out_enable(id, en);
}

int afe_i2s_out_master_dsd_configurate(enum afe_i2s_out_id id, enum afe_sampling_rate fs, int mclk,
				       enum afe_i2s_out_dsd_use dsd_use)
{
	switch (id) {
	case AFE_I2S_OUT_1:
		afe_msk_write(ASMO_TIMING_CON1, (fs << 0), ASMO1_MODE_MASK);
		break;
	case AFE_I2S_OUT_2:
		afe_msk_write(ASMO_TIMING_CON1, (fs << 5), ASMO2_MODE_MASK);
		break;
	default:
		pr_err("%s() error: i2s-out id %d doesn't support DSD\n", __func__, id);
		return -EINVAL;
	}
	/* configurate i2s-in first, and then i2s-out */
	{
		struct afe_i2s_in_config config = {
			.fpga_test_loop3 = 0,
			.fpga_test_loop = 0,
			.fpga_test_loop2 = 0,
			.use_asrc = 0,
			.dsd_mode = 0,	/* must be none-DSD */
			.slave = 0,
			.fmt = FMT_32CYCLE_16BIT_I2S,
			.mclk = mclk,
			.mode = fs
		};

		afe_i2s_in_configurate((enum afe_i2s_in_id)id, &config);
	}
	{
		struct afe_i2s_out_config config = {
			.fpga_test_loop = 0,
			.data_from_sine = 0,
			.use_asrc = 0,
			.dsd_mode = 1,	/* DSD */
			.dsd_use = dsd_use,
			.one_heart_mode = 0,
			.slave = 0,	/* master */
			/* .fmt = ,  -> no need in DSD mode */
			.mclk = mclk,
			.mode = fs
		};

		afe_i2s_out_configurate(id, &config);
	}
	return 0;
}

int afe_i2s_out_master_dsd_enable(enum afe_i2s_out_id id, int en)
{
	pr_debug("%s enable I2S %d\n", __func__, id);
	return afe_i2s_out_enable(id, en);
}

int afe_i2s_out_slave_dsd_configurate(enum afe_i2s_out_id id)
{
	/* configurate i2s-in first, and then i2s-out */
	{
		struct afe_i2s_in_config config = {
			.fpga_test_loop3 = 0,
			.fpga_test_loop = 0,
			.fpga_test_loop2 = 0,
			.use_asrc = 0,
			.dsd_mode = 1,
			.slave = 1,
			/* .fmt = FMT_32CYCLE_16BIT_I2S, */
			/* .mclk = mclk, */
			/* .mode = fs */
		};

		afe_i2s_in_configurate((enum afe_i2s_in_id)id, &config);
	}
	{
		struct afe_i2s_out_config config = {
			.fpga_test_loop = 0,
			.data_from_sine = 0,
			.use_asrc = 0,
			.dsd_mode = 1,	/* DSD */
			.dsd_use = I2S_OUT_DSD_USE_NORMAL,
			.one_heart_mode = 0,
			.slave = 1,
			/* .fmt = ,  -> no need in DSD mode */
			/* .mclk = mclk, */
			/* .mode = fs */
		};

		afe_i2s_out_configurate(id, &config);
	}
	return 0;
}

int afe_i2s_out_slave_dsd_enable(enum afe_i2s_out_id id, int en)
{
	pr_debug("%s enable I2S %d\n", __func__, id);
	return afe_i2s_out_enable(id, en);
}

int afe_i2s_in_master_pcm_configurate(enum afe_i2s_in_id id, enum afe_i2s_format fmt, int mclk,
				      enum afe_sampling_rate fs, int use_asrc)
{
	/* configurate i2s-out first, and then i2s-in */
	{
		struct afe_i2s_out_config config = {
			.fpga_test_loop = 0,
			.data_from_sine = 0,
			.use_asrc = 0,
			.dsd_mode = 0,
			.dsd_use = I2S_OUT_DSD_USE_NORMAL,
			.one_heart_mode = 0,
			.slave = 0,
			.fmt = fmt,
			.mclk = mclk,
			.mode = fs
		};

		afe_i2s_out_configurate((enum afe_i2s_out_id)id, &config);
	}
	{
		struct afe_i2s_in_config config = {
			.fpga_test_loop3 = 0,
			.fpga_test_loop = (afe_loopback ? 1 : 0),
			.fpga_test_loop2 = 0,
			.use_asrc = use_asrc,
			.dsd_mode = 0,
			.slave = 0,
			.fmt = fmt,
			.mclk = mclk,
			.mode = fs
		};

		afe_i2s_in_configurate(id, &config);
	}
	return 0;
}

int afe_i2s_in_master_pcm_enable(enum afe_i2s_in_id id, int en)
{
	afe_i2s_in_enable(id, en);
	pr_debug("%s enable I2S %d\n", __func__, id);
	afe_i2s_out_enable((enum afe_i2s_out_id)id, en);
	return 0;
}

int afe_i2s_in_master_dsd_configurate(enum afe_i2s_in_id id, enum afe_sampling_rate fs, int mclk)
{
	switch (id) {
	case AFE_I2S_IN_1:
		afe_msk_write(ASMI_TIMING_CON1, (fs << 0), ASMI1_MODE_MASK);
		break;
	case AFE_I2S_IN_2:
		afe_msk_write(ASMI_TIMING_CON1, (fs << 5), ASMI2_MODE_MASK);
		break;
	default:
		pr_err("%s() error: i2s-in id %d doesn't support DSD\n", __func__, id);
		return -EINVAL;
	}
	/* configurate i2s-out first, and then i2s-in */
	{
		struct afe_i2s_out_config config = {
			.fpga_test_loop = 0,
			.data_from_sine = 0,
			.use_asrc = 0,
			.dsd_mode = 0,	/* must be none-DSD */
			.dsd_use = I2S_OUT_DSD_USE_NORMAL,
			.one_heart_mode = 0,
			.slave = 0,
			.fmt = FMT_32CYCLE_16BIT_I2S,
			.mclk = mclk,
			.mode = fs
		};

		afe_i2s_out_configurate((enum afe_i2s_out_id)id, &config);
	}
	{
		struct afe_i2s_in_config config = {
			.fpga_test_loop3 = 0,
			.fpga_test_loop = 0,
			.fpga_test_loop2 = 0,
			.use_asrc = 0,
			.dsd_mode = 1,	/* DSD */
			.slave = 0,	/* master */
			/* .fmt = ,  -> no need in DSD mode */
			.mclk = mclk,
			.mode = fs
		};

		afe_i2s_in_configurate(id, &config);
	}
	return 0;
}

int afe_i2s_in_master_dsd_enable(enum afe_i2s_in_id id, int en)
{
	afe_i2s_in_enable(id, en);
	pr_debug("%s enable I2S %d\n", __func__, id);
	afe_i2s_out_enable((enum afe_i2s_out_id)id, en);
	return 0;
}

int afe_i2s_in_slave_dsd_configurate(enum afe_i2s_in_id id)
{
	/* configurate i2s-out first, and then i2s-in */
	{
		struct afe_i2s_out_config config = {
			.fpga_test_loop = 0,
			.data_from_sine = 0,
			.use_asrc = 0,
			.dsd_mode = 0,	/* none-DSD */
			.dsd_use = I2S_OUT_DSD_USE_NORMAL,
			.one_heart_mode = 0,
			.slave = 1,
			/* .fmt = ,  -> no need in DSD mode */
			/* .mclk = mclk, */
			/* .mode = fs */
		};

		afe_i2s_out_configurate((enum afe_i2s_out_id)id, &config);
	}
	{
		struct afe_i2s_in_config config = {
			.fpga_test_loop3 = 0,
			.fpga_test_loop = 0,
			.fpga_test_loop2 = 0,
			.use_asrc = 0,
			.dsd_mode = 1,	/* DSD */
			.slave = 1,
			/* .fmt = FMT_32CYCLE_16BIT_I2S, */
			/* .mclk = mclk, */
			/* .mode = fs */
		};

		afe_i2s_in_configurate(id, &config);
	}
	return 0;
}

int afe_i2s_in_slave_dsd_enable(enum afe_i2s_in_id id, int en)
{
	return afe_i2s_in_enable(id, en);
}

int afe_i2s_in_slave_pcm_configurate(enum afe_i2s_in_id id, enum afe_i2s_format fmt, int use_asrc)
{
	/* configurate i2s-out first, and then i2s-in */
	{
		struct afe_i2s_out_config config = {
			.fpga_test_loop = 0,
			.data_from_sine = 0,
			.use_asrc = 0,
			.dsd_mode = 0,
			.dsd_use = I2S_OUT_DSD_USE_NORMAL,
			.one_heart_mode = 0,
			.slave = 1,
			.fmt = fmt,
			.mclk = 0,
			/* .mode = fs */
		};

		afe_i2s_out_configurate((enum afe_i2s_out_id)id, &config);
	}
	{
		struct afe_i2s_in_config config = {
			.fpga_test_loop3 = 0,
			.fpga_test_loop = 0,
			.fpga_test_loop2 = 0,
			.use_asrc = use_asrc,
			.dsd_mode = 0,
			.slave = 1,
			.fmt = fmt,
			.mclk = 0,
			/* .mode = fs */
		};

		afe_i2s_in_configurate(id, &config);
	}
	return 0;
}

int afe_i2s_in_slave_pcm_enable(enum afe_i2s_in_id id, int en)
{
	return afe_i2s_in_enable(id, en);
}

/**************i2s multi channel X********************/
int afe_i2s_inmx_pcm_configurate(int i2s_num, enum afe_i2s_format fmt,
				 int mclk, enum afe_sampling_rate fs,
				 int slave)
{
	enum afe_i2s_in_id tmp_id;
	struct afe_i2s_in_config i2s_in_config = {
		.fpga_test_loop3 = 0,
		.fpga_test_loop = 0,
		.fpga_test_loop2 = 0,
		.use_asrc = 0,
		.dsd_mode = 0,
		.slave = slave,
		.fmt = fmt,
		.mclk = mclk,
		.mode = fs
	};
	struct afe_i2s_out_config i2s_out_config = {
		.fpga_test_loop = 0,
		.data_from_sine = 0,
		.use_asrc = 0,
		.dsd_mode = 0,
		.dsd_use = I2S_OUT_DSD_USE_NORMAL,
		.couple_mode = 0,
		.one_heart_mode = 0,
		.slave = slave,
		.fmt = fmt,
		.mclk = mclk,
		.mode = fs
	};
	for (tmp_id = AFE_I2S_IN_1; tmp_id < AFE_I2S_IN_1 + i2s_num; ++tmp_id) {
		afe_i2s_in_configurate(tmp_id, &i2s_in_config);
		afe_i2s_out_configurate((enum afe_i2s_out_id)tmp_id, &i2s_out_config);
	}
	return 0;
}

int afe_i2s_inmx_pcm_enable(int i2s_num, int en)
{
	enum afe_i2s_in_id tmp_id;

	en = !!en;
	for (tmp_id = AFE_I2S_IN_1; tmp_id < AFE_I2S_IN_1 + i2s_num; ++tmp_id) {
		if (tmp_id > AFE_I2S_IN_1) {
			u32 addr = ASYS_I2SIN1_CON + tmp_id * 4;
			afe_msk_write(addr, (en << 24) | (en << 28), (0x1 << 24) | 0x1 << 28);
		}
		if (tmp_id == AFE_I2S_IN_1)
			afe_i2s_out_enable((enum afe_i2s_out_id)tmp_id, en);
		mt_i2s_power_on_mclk(tmp_id, en);
		afe_i2s_in_enable(tmp_id, en);
	}
	return 0;
}

/******************** multilinein ********************/

void afe_multilinein_configurate(const struct afe_multilinein_config *config)
{
	u32 val1, msk1;
	u32 val0, msk0;

	val1 = MULTI_SYNC_ENABLE
	       /*| MULTI_INV_BCK */
	       /*| (0x1 << 21) */   /* dsd 8 bit mode ? ? ? */
	       | (config->dsd_mode << 20)
	       | (0x0 << 19)    /* always cmpt_mode */
	       /* if 2ch, use none-hbr_mode */
	       /* if 8ch, use hbr_mode */
	       | ((config->ch_num == AFE_MULTILINEIN_8CH ? 0x1 : 0x0) << 18)
	       | (config->ch_num << 0);
	val0 = (config->intr_period << 4);
	switch (config->fmt) {
	case FMT_32CYCLE_16BIT_I2S:
		val1 |= (AINACK_CFG_LRCK_SEL_16 | AINACK_CFG_BITNUM_16 | AINACK_CFG_CLK_I2S);
		val0 |= DATA_16BIT;
		break;
	case FMT_32CYCLE_16BIT_LJ:
		val1 |= (AINACK_CFG_LRCK_SEL_16
			 | AINACK_CFG_BITNUM_16 | AINACK_CFG_CLK_LJ | AINACK_CFG_INV_LRCK);
		val0 |= DATA_16BIT;
		break;
	case FMT_64CYCLE_16BIT_I2S:
		val1 |= (AINACK_CFG_LRCK_SEL_32 | AINACK_CFG_BITNUM_16 | AINACK_CFG_CLK_I2S);
		val0 |= DATA_16BIT;
		break;
	case FMT_64CYCLE_32BIT_I2S:
		val1 |= (AINACK_CFG_LRCK_SEL_32 | AINACK_CFG_BITNUM_24 | AINACK_CFG_CLK_I2S);
		val0 |= DATA_24BIT;
		break;
	case FMT_64CYCLE_16BIT_LJ:
		val1 |= (AINACK_CFG_LRCK_SEL_32
			 | AINACK_CFG_BITNUM_16 | AINACK_CFG_CLK_LJ | AINACK_CFG_INV_LRCK);
		val0 |= DATA_16BIT;
		break;
	case FMT_64CYCLE_32BIT_LJ:
		val1 |= (AINACK_CFG_LRCK_SEL_32
			 | AINACK_CFG_BITNUM_24 | AINACK_CFG_CLK_LJ | AINACK_CFG_INV_LRCK);
		val0 |= DATA_24BIT;
		break;
	case FMT_64CYCLE_16BIT_RJ:
		val1 |= (AINACK_CFG_LRCK_SEL_32
			 | AINACK_CFG_BITNUM_16 | AINACK_CFG_CLK_RJ | AINACK_CFG_INV_LRCK);
		val0 |= DATA_16BIT;
		break;
	case FMT_64CYCLE_24BIT_RJ:
		val1 |= (AINACK_CFG_LRCK_SEL_32
			 | AINACK_CFG_BITNUM_24 | AINACK_CFG_CLK_RJ | AINACK_CFG_INV_LRCK);
		val0 |= DATA_24BIT;
		break;
	default:
		break;
	}
	if (is_hd_audio(config->fmt)) {
		/* 24bit case endian */
		if (config->endian == AFE_MULTILINEIN_LITTILE_ENDIAN) {
			val1 |= (0x1 << 22);
			val0 |= DATA_16BIT_NON_SWAP;
		} else {
			val1 |= (0x0 << 22);
			val0 |= DATA_16BIT_NON_SWAP;
		}
	} else {
		/* 16bit case endian */
		if (config->endian == AFE_MULTILINEIN_LITTILE_ENDIAN) {
			val1 |= (0x0 << 22);
			val0 |= DATA_16BIT_SWAP;
		} else {
			val1 |= (0x0 << 22);
			val0 |= DATA_16BIT_NON_SWAP;
		}
	}
	if (config->dsd_mode) {
		if (config->dsdWidth == DSD_WIDTH_32BIT) { /*24bit*/
			val1 |= (0x0 << 21); /* 24bit*/
		} else {
			val1 |= (0x1 << 21); /*8bit*/
		}
	}
	if (config->mss == AFE_MULTILINE_FROM_RX) {
		val0 |= CLK_SEL_HDMI_RX;
		val0 |= SDATA_SEL_HDMI_RX;
	} else {
		val0 |= CLK_SEL_8CH_I2S;
		val0 |= SDATA_SEL_8CH_I2S;
	}
	msk1 = AINACK_MULTI_SYNC_MASK
	       | MULTI_DSD_MODE_MASK /*dsd 8bit or 24bit*/
	       /*| (0x1 << 21) */   /* dsd 8 bit mode ? ? ? */
	       | MULTI_INV_BCK
	       | AINACK_CFG_IN_MODE
	       | MULTI_24BIT_SWAP_MASK
	       | MULTI_NONE_COMPACT_MASK
	       | AINACK_HBRMOD_MASK
	       | AINACK_CFG_LRCK_MASK
	       | AINACK_CFG_INV_LRCK
	       | AINACK_CFG_CLK_MASK
	       | AINACK_CFG_BITNUM_MASK
	       | AINACK_CFG_CH_NUM_MASK;
	msk0 = INTR_PERIOD_MASK
	       | DATA_16BIT_SWAP_MASK
	       | DATA_BIT_MASK
	       | CLK_SEL_MASK
	       | SDATA_SEL_MASK;
	afe_msk_write(AFE_MPHONE_MULTI_CON1, val1, msk1);
	afe_msk_write(AFE_MPHONE_MULTI_CON0, val0, msk0);
	afe_msk_write(AFE_MPHONE_MULTI_CON0,
		      (0x0 << 14) | (0x1 << 17) | (0x2 << 20) | (0x3 << 23) | (0x4 << 26) | (0x5 << 29),
		      (0x7 << 14) | (0x7 << 17) | (0x7 << 20) | (0x7 << 23) | (0x7 << 26) | (0x7 << 29));
}

void afe_multilinein_enable(int en)
{
	en = !!en;
	if (en) {
		#ifdef CONFIG_MTK_LEGACY_CLOCK
		enable_clock(MT_CG_AUDIO_HDMIRX, "AUDIO"); /* AUDIO_TOP_CON4[19]:pdn_multi_in */
		#else
		/*
		 * [Programming Guide]
		 * [SPDIF IN]power on MPHONE
		 */
		afe_msk_write(AUDIO_TOP_CON4, 0, PND_MULTI_IN);
		#endif
		afe_msk_write(AFE_MPHONE_MULTI_CON0, 1 << 0, HW_EN_MASK);
	} else {
		afe_msk_write(AFE_MPHONE_MULTI_CON0, 0 << 0, HW_EN_MASK);
		#ifdef CONFIG_MTK_LEGACY_CLOCK
		disable_clock(MT_CG_AUDIO_HDMIRX, "AUDIO");
		#else
		afe_msk_write(AUDIO_TOP_CON4, PND_MULTI_IN, PND_MULTI_IN);
		#endif
	}
}

int afe_asmi_timing_set(enum afe_i2s_in_id id, enum afe_sampling_rate rate)
{
	u32 addr = ASMI_TIMING_CON1;
	u32 val = (rate << (id * 5));
	u32 msk = (0x1F << (id * 5));

	afe_msk_write(addr, val, msk);
	return 0;
}

int afe_asmo_timing_set(enum afe_i2s_out_id id, enum afe_sampling_rate rate)
{
	u32 addr = ASMO_TIMING_CON1;
	u32 val = (rate << (id * 5));
	u32 msk = (0x1F << (id * 5));

	afe_msk_write(addr, val, msk);
	return 0;
}


/******************** sample-based asrc ********************/

static int power_on_sample_asrc_rx(enum afe_sample_asrc_rx_id id, int on)
{
	const int off = !on;

	switch (id) {
	case SAMPLE_ASRC_I1:
		afe_msk_write(AUDIO_TOP_CON4, off << 12, PDN_ASRCI1);
		break;
	case SAMPLE_ASRC_I2:
		afe_msk_write(AUDIO_TOP_CON4, off << 13, PDN_ASRCI2);
		break;
	case SAMPLE_ASRC_I3:
		afe_msk_write(PWR2_TOP_CON, off << PDN_ASRCI3_POS, PDN_ASRCI3_MASK);
		break;
	case SAMPLE_ASRC_I4:
		afe_msk_write(PWR2_TOP_CON, off << PDN_ASRCI4_POS, PDN_ASRCI4_MASK);
		break;
	case SAMPLE_ASRC_I5:
		afe_msk_write(PWR2_TOP_CON, off << PDN_ASRCI5_POS, PDN_ASRCI5_MASK);
		break;
	case SAMPLE_ASRC_I6:
		afe_msk_write(PWR2_TOP_CON, off << PDN_ASRCI6_POS, PDN_ASRCI6_MASK);
		break;
	case SAMPLE_ASRC_PCM_IN:
		afe_msk_write(AUDIO_TOP_CON4, off << 16, PDN_ASRC11);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

int afe_power_on_sample_asrc_rx(enum afe_sample_asrc_rx_id id, int on)
{
	static DEFINE_SPINLOCK(lock);
	unsigned long flags;
	static int status[SAMPLE_ASRC_IN_NUM];

	spin_lock_irqsave(&lock, flags);
	if (on) {
		if (status[id] == 0)
			power_on_sample_asrc_rx(id, 1);
		++status[id];
	} else {
		if (status[id] > 0 && --status[id] == 0)
			power_on_sample_asrc_rx(id, 0);
	}
	spin_unlock_irqrestore(&lock, flags);
	return 0;
}

static int power_on_sample_asrc_tx(enum afe_sample_asrc_tx_id id, int on)
{
	const int off = !on;

	switch (id) {
	case SAMPLE_ASRC_O1:
		afe_msk_write(AUDIO_TOP_CON4, off << 14, PDN_ASRCO1);
		break;
	case SAMPLE_ASRC_O2:
		afe_msk_write(AUDIO_TOP_CON4, off << 15, PDN_ASRCO2);
		break;
	case SAMPLE_ASRC_O3:
		afe_msk_write(PWR2_TOP_CON, off << PDN_ASRCO3_POS, PDN_ASRCO3_MASK);
		break;
	case SAMPLE_ASRC_O4:
		afe_msk_write(PWR2_TOP_CON, off << PDN_ASRCO4_POS, PDN_ASRCO4_MASK);
		break;
	case SAMPLE_ASRC_O5:
		afe_msk_write(PWR2_TOP_CON, off << PDN_ASRCO5_POS, PDN_ASRCO5_MASK);
		break;
	case SAMPLE_ASRC_O6:
		afe_msk_write(PWR2_TOP_CON, off << PDN_ASRCO6_POS, PDN_ASRCO6_MASK);
		break;
	case SAMPLE_ASRC_PCM_OUT:
		afe_msk_write(AUDIO_TOP_CON4, off << 17, PDN_ASRC12);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

int afe_power_on_sample_asrc_tx(enum afe_sample_asrc_tx_id id, int on)
{
	static DEFINE_SPINLOCK(lock);
	unsigned long flags;
	static int status[SAMPLE_ASRC_OUT_NUM];

	spin_lock_irqsave(&lock, flags);
	if (on) {
		if (status[id] == 0)
			power_on_sample_asrc_tx(id, 1);
		++status[id];
	} else {
		if (status[id] > 0 && --status[id] == 0)
			power_on_sample_asrc_tx(id, 0);
	}
	spin_unlock_irqrestore(&lock, flags);
	return 0;
}

static inline u32 AutoRstThLo(enum afe_sampling_rate fs)
{
	switch (fs) {
	case FS_8000HZ:
		return 0x05A000;
	case FS_12000HZ:
		return 0x03c000;
	case FS_16000HZ:
		return 0x02d000;
	case FS_24000HZ:
		return 0x01e000;
	case FS_32000HZ:
		return 0x016000;
	case FS_48000HZ:
		return 0x00f000;
	case FS_96000HZ:
		return 0x007800;
	case FS_192000HZ:
		return 0x003c00;
	case FS_384000HZ:
		return 0x001e00;
	case FS_7350HZ:
		return 0x066000;
	case FS_11025HZ:
		return 0x042000;
	case FS_14700HZ:
		return 0x033000;
	case FS_22050HZ:
		return 0x021000;
	case FS_29400HZ:
		return 0x019800;
	case FS_44100HZ:
		return 0x011000;
	case FS_88200HZ:
		return 0x008300;
	case FS_176400HZ:
		return 0x004100;
	case FS_352800HZ:
		return 0x002100;
	default:
		return 0x0;
	}
}

static inline u32 AutoRstThHi(enum afe_sampling_rate fs)
{
	switch (fs) {
	case FS_8000HZ:
		return 0x066000;
	case FS_12000HZ:
		return 0x044000;
	case FS_16000HZ:
		return 0x033000;
	case FS_24000HZ:
		return 0x022000;
	case FS_32000HZ:
		return 0x01a000;
	case FS_48000HZ:
		return 0x011000;
	case FS_96000HZ:
		return 0x008800;
	case FS_192000HZ:
		return 0x004400;
	case FS_384000HZ:
		return 0x002200;
	case FS_7350HZ:
		return 0x06F000;
	case FS_11025HZ:
		return 0x04a000;
	case FS_14700HZ:
		return 0x037800;
	case FS_22050HZ:
		return 0x025000;
	case FS_29400HZ:
		return 0x01BC00;
	case FS_44100HZ:
		return 0x012800;
	case FS_88200HZ:
		return 0x009400;
	case FS_176400HZ:
		return 0x004a00;
	case FS_352800HZ:
		return 0x002500;
	default:
		return 0x0;
	}
}

static inline u32 FrequencyPalette(enum afe_sampling_rate fs)
{
	switch (fs) {
	case FS_8000HZ:
		return 0x050000;
	case FS_12000HZ:
		return 0x078000;
	case FS_16000HZ:
		return 0x0A0000;
	case FS_24000HZ:
		return 0x0F0000;
	case FS_32000HZ:
		return 0x140000;
	case FS_48000HZ:
		return 0x1E0000;
	case FS_96000HZ:
		return 0x3C0000;
	case FS_192000HZ:
		return 0x780000;
	case FS_384000HZ:
		return 0xF00000;
	case FS_7350HZ:
		return 0x049800;
	case FS_11025HZ:
		return 0x06E400;
	case FS_14700HZ:
		return 0x093000;
	case FS_22050HZ:
		return 0x0DC800;
	case FS_29400HZ:
		return 0x126000;
	case FS_44100HZ:
		return 0x1B9000;
	case FS_88200HZ:
		return 0x372000;
	case FS_176400HZ:
		return 0x6E4000;
	case FS_352800HZ:
		return 0xDC8000;
	default:
		return 0x0;
	}
}

static u32 PeriodPalette(enum afe_sampling_rate fs)
{
	switch (fs) {
	case FS_8000HZ:
		return 0x060000;
	case FS_12000HZ:
		return 0x040000;
	case FS_16000HZ:
		return 0x030000;
	case FS_24000HZ:
		return 0x020000;
	case FS_32000HZ:
		return 0x018000;
	case FS_48000HZ:
		return 0x010000;
	case FS_96000HZ:
		return 0x008000;
	case FS_192000HZ:
		return 0x004000;
	case FS_384000HZ:
		return 0x002000;
	case FS_7350HZ:
		return 0x0687D8;
	case FS_11025HZ:
		return 0x045A90;
	case FS_14700HZ:
		return 0x0343EC;
	case FS_22050HZ:
		return 0x022D48;
	case FS_29400HZ:
		return 0x01A1F6;
	case FS_44100HZ:
		return 0x0116A4;
	case FS_88200HZ:
		return 0x008B52;
	case FS_176400HZ:
		return 0x0045A9;
	case FS_352800HZ:
		return 0x0022D4;	/* ??? */
	default:
		return 0x0;
	}
}

static const u32 *get_iir_coef(enum afe_sampling_rate input_fs, enum afe_sampling_rate output_fs, size_t *count)
{
#define RATIOVER 23
#define INV_COEF 24
#define NO_NEED 25
#define TBL_SZ_MEMASRC_IIR_COEF (48)

	static const u32 IIR_COEF_384_TO_352[TBL_SZ_MEMASRC_IIR_COEF] = {
		0x0303b6d0, 0x05cdf456, 0x0303b6d0, 0xc416d6b5, 0xe0363059, 0x00000002, 0x07cc27b7, 0x0f0ba03c,
		0x07cc27b7, 0xe25f513e, 0xf058cf70, 0x00000003, 0x07aa57c1, 0x0eda62f1, 0x07aa57c1, 0xe2f03496,
		0xf0b1a683, 0x00000003, 0x076f8c07, 0x0e7fe0ff, 0x076f8c07, 0xe3f982eb, 0xf1488afb, 0x00000003,
		0x0e01e7ef, 0x1b8868ec, 0x0e01e7ef, 0xe607f80e, 0xf26bcf29, 0x00000003, 0x0c3237da, 0x182c4952,
		0x0c3237da, 0xea8a804f, 0xf4e4c6ab, 0x00000003, 0x1082b5c7, 0x20f0495b, 0x1082b5c7, 0xe93bbad7,
		0xf4ce9040, 0x00000002, 0x00000000, 0x2b58b702, 0x2b58b702, 0xfff6882b, 0x00000000, 0x00000007
	};

	static const u32 IIR_COEF_256_TO_192[TBL_SZ_MEMASRC_IIR_COEF] = {
		0x02c5a70f, 0x03eb90a0, 0x02c5a70f, 0xdc8ebb5e, 0xe08256c1, 0x00000002, 0x0df345b2, 0x141cbadb,
		0x0df345b2, 0xde56eee1, 0xe1a284a2, 0x00000002, 0x0d03b121, 0x138292e8, 0x0d03b121, 0xe1c608f0,
		0xe3260cbe, 0x00000002, 0x0b8948fb, 0x1254a1d4, 0x0b8948fb, 0xe7c0c6a5, 0xe570d1c6, 0x00000002,
		0x12a7cba4, 0x1fe15ef0, 0x12a7cba4, 0xf1bd349d, 0xe911d52a, 0x00000002, 0x0c882b79, 0x1718bc3a,
		0x0c882b79, 0x013ec33c, 0xee982998, 0x00000002, 0x0b5bd89b, 0x163580e0, 0x0b5bd89b, 0x2873f220,
		0xea9edbcb, 0x00000001, 0x00000000, 0x2c155c70, 0x2c155c70, 0x00f204d6, 0x00000000, 0x00000006
	};

	static const u32 IIR_COEF_352_TO_256[TBL_SZ_MEMASRC_IIR_COEF] = {
		0x02b9c432, 0x038d00b6, 0x02b9c432, 0xe15c8a85, 0xe0898362, 0x00000002, 0x0da7c09d, 0x12390f60,
		0x0da7c09d, 0xe3369d52, 0xe1b8417b, 0x00000002, 0x0ca047cb, 0x11b06e35, 0x0ca047cb, 0xe6d2ee1d,
		0xe34b164a, 0x00000002, 0x0b06ae31, 0x109ad63c, 0x0b06ae31, 0xed0d2221, 0xe5a278a0, 0x00000002,
		0x116c5a39, 0x1cb58b13, 0x116c5a39, 0xf7341949, 0xe93da732, 0x00000002, 0x0b5486de, 0x147ddf59,
		0x0b5486de, 0x0657d3a0, 0xee813f4a, 0x00000002, 0x09e7f722, 0x13449867, 0x09e7f722, 0x2fcbde4b,
		0xe91f9b09, 0x00000001, 0x00000000, 0x28cd51d5, 0x28cd51d5, 0x01061d85, 0x00000000, 0x00000006
	};

	static const u32 IIR_COEF_384_TO_256[TBL_SZ_MEMASRC_IIR_COEF] = {
		0x0539819f, 0x054951e5, 0x0539819f, 0xda57122a, 0xc12ba871, 0x00000001, 0x0cf35b0b, 0x0da330e8,
		0x0cf35b0b, 0xef0f9d73, 0xe1dc9491, 0x00000002, 0x0bb8d79a, 0x0d5e3977, 0x0bb8d79a, 0xf2db2d6f,
		0xe385013c, 0x00000002, 0x09e15952, 0x0ca91405, 0x09e15952, 0xf9444c2e, 0xe5e42680, 0x00000002,
		0x0ecd2494, 0x15e07e27, 0x0ecd2494, 0x03307784, 0xe954c12e, 0x00000002, 0x08f68ac2, 0x0f542aa7,
		0x08f68ac2, 0x10cf62d9, 0xedef5cfc, 0x00000002, 0x074829ae, 0x0df259cb, 0x074829ae, 0x3e16cf8a,
		0xe566834f, 0x00000001, 0x00000000, 0x2263d8b7, 0x2263d8b7, 0x012d626d, 0x00000000, 0x00000006
	};

	static const u32 IIR_COEF_352_TO_192[TBL_SZ_MEMASRC_IIR_COEF] = {
		0x04bd6a83, 0x015bba25, 0x04bd6a83, 0x0d7cdab3, 0xc137e9aa, 0x00000001, 0x0b80613f, 0x03ed2155,
		0x0b80613f, 0x0839ad63, 0xe1ea8af7, 0x00000002, 0x09f38e84, 0x049744b0, 0x09f38e84, 0x0b793c54,
		0xe38a003b, 0x00000002, 0x0f90a19e, 0x0a5f3f69, 0x0f90a19e, 0x21996102, 0xcb6599b5, 0x00000001,
		0x0a71465a, 0x0a3901b2, 0x0a71465a, 0x185f028c, 0xe8856f0e, 0x00000002, 0x057f4d69, 0x07aa76ed,
		0x057f4d69, 0x2185beab, 0xebd12f96, 0x00000002, 0x01f3ece3, 0x0389a7a8, 0x01f3ece3, 0x29ccd8d7,
		0xeec1a5bb, 0x00000002, 0x00000000, 0x30ea5629, 0x30ea5629, 0x02d46d34, 0x00000000, 0x00000005
	};

	static const u32 IIR_COEF_384_TO_192[TBL_SZ_MEMASRC_IIR_COEF] = {
		0x04ec93a4, 0x000a1bf6, 0x04ec93a4, 0x1a59368b, 0xc11e7000, 0x00000001, 0x0b9bf8b5, 0x00b44721,
		0x0b9bf8b5, 0x0e62af3f, 0xe1c4dfac, 0x00000002, 0x09fe478b, 0x01c85505, 0x09fe478b, 0x112b48ef,
		0xe34aeed8, 0x00000002, 0x07b86f3c, 0x02fd7211, 0x07b86f3c, 0x15cb1caf, 0xe558423d, 0x00000002,
		0x0a1e67fb, 0x0754a987, 0x0a1e67fb, 0x1c63a076, 0xe80ae60d, 0x00000002, 0x0522203a, 0x0635518b,
		0x0522203a, 0x245624f0, 0xeb304910, 0x00000002, 0x01bfb83a, 0x030c2050, 0x01bfb83a, 0x2b7a0a3d,
		0xedfa64fe, 0x00000002, 0x00000000, 0x2c9308ca, 0x2c9308ca, 0x02e75633, 0x00000000, 0x00000005
	};

	static const u32 IIR_COEF_384_TO_176[TBL_SZ_MEMASRC_IIR_COEF] = {
		0x04c0f379, 0xfedb44d2, 0x04c0f379, 0x298f4134, 0xc1174e69, 0x00000001, 0x0b2dc719, 0xfde5dcb2,
		0x0b2dc719, 0x15c4fe25, 0xe1b82c15, 0x00000002, 0x097dabd2, 0xff571c5d, 0x097dabd2, 0x182d7cb1,
		0xe32d9b4e, 0x00000002, 0x072993e5, 0x01099884, 0x072993e5, 0x1c2c2780, 0xe51a57e5, 0x00000002,
		0x090f2fa1, 0x048cdc85, 0x090f2fa1, 0x21c3b5c2, 0xe7910e77, 0x00000002, 0x04619e1a, 0x0491e3db,
		0x04619e1a, 0x28513621, 0xea59a9d0, 0x00000002, 0x016b9a38, 0x0261dc20, 0x016b9a38, 0x2e08c9d1,
		0xecbe259f, 0x00000002, 0x00000000, 0x27d8fdc3, 0x27d8fdc3, 0x03060265, 0x00000000, 0x00000005
	};

	static const u32 IIR_COEF_256_TO_96[TBL_SZ_MEMASRC_IIR_COEF] = {
		0x02545a7f, 0xfe3ea711, 0x02545a7f, 0x2238ae5c, 0xe07931d4, 0x00000002, 0x0ac115ca, 0xf860f5e9,
		0x0ac115ca, 0x22bbb3c8, 0xe17e093d, 0x00000002, 0x08f408eb, 0xfa958f42, 0x08f408eb, 0x244477a8,
		0xe2c04629, 0x00000002, 0x0682ca04, 0xfd584bf8, 0x0682ca04, 0x26e0e9c4, 0xe463563b, 0x00000002,
		0x07baafe9, 0xff9d399b, 0x07baafe9, 0x2a7e74b7, 0xe66ef1dd, 0x00000002, 0x0362b43a, 0x01f07186,
		0x0362b43a, 0x2e9e4f86, 0xe8abd681, 0x00000002, 0x00f873cc, 0x016b4c05, 0x00f873cc, 0x321b967a,
		0xea8835e8, 0x00000002, 0x00000000, 0x1f82bf79, 0x1f82bf79, 0x03381d0d, 0x00000000, 0x00000005
	};

	static const u32 IIR_COEF_352_TO_128[TBL_SZ_MEMASRC_IIR_COEF] = {
		0x02449fa8, 0xfe2712a2, 0x02449fa8, 0x24311459, 0xe0787f65, 0x00000002, 0x0a84fc5d, 0xf7e60ffe,
		0x0a84fc5d, 0x24a509fa, 0xe17ae496, 0x00000002, 0x08b52aad, 0xfa3093f6, 0x08b52aad, 0x2612ff28,
		0xe2b72e39, 0x00000002, 0x064680dc, 0xfd0ab6a4, 0x064680dc, 0x2882e116, 0xe44dae31, 0x00000002,
		0x075b08c6, 0xff2a08c3, 0x075b08c6, 0x2bdcc672, 0xe6431f3f, 0x00000002, 0x032b3b9b, 0x01a2f50e,
		0x032b3b9b, 0x2fa5cc8a, 0xe860c732, 0x00000002, 0x00e4486e, 0x01476fb8, 0x00e4486e, 0x32d1c2b2,
		0xea1e3cba, 0x00000002, 0x00000000, 0x3c8f8f79, 0x3c8f8f79, 0x0682ad88, 0x00000000, 0x00000004
	};

	static const u32 IIR_COEF_384_TO_128[TBL_SZ_MEMASRC_IIR_COEF] = {
		0x02894a52, 0xfd770f7a, 0x02894a52, 0x27008598, 0xe06656ba, 0x00000002, 0x0b4d22b9, 0xf51b5d44,
		0x0b4d22b9, 0x274e7f44, 0xe1463b51, 0x00000002, 0x0976ef3e, 0xf7ac51fb, 0x0976ef3e, 0x28672e65,
		0xe26470ac, 0x00000002, 0x06e6a0fe, 0xfb12646f, 0x06e6a0fe, 0x2a5a082f, 0xe3e6aafa, 0x00000002,
		0x081c5ea5, 0xfcccf1de, 0x081c5ea5, 0x2d20d986, 0xe5d97751, 0x00000002, 0x036a7c7a, 0x00c2723e,
		0x036a7c7a, 0x305e8d68, 0xe80a0765, 0x00000002, 0x00e763b5, 0x0123b6ba, 0x00e763b5, 0x332864f5,
		0xe9e51ce6, 0x00000002, 0x00000000, 0x38c17ba1, 0x38c17ba1, 0x068927ea, 0x00000000, 0x00000004
	};

	static const u32 IIR_COEF_352_TO_96[TBL_SZ_MEMASRC_IIR_COEF] = {
		0x0217fde7, 0xfd41917c, 0x0217fde7, 0x30c91f13, 0xe060d9b0, 0x00000002, 0x09b6b0aa, 0xf39321ee,
		0x09b6b0aa, 0x30cfc632, 0xe12f3348, 0x00000002, 0x07d62437, 0xf68e9f4a, 0x07d62437, 0x3163f76d,
		0xe2263924, 0x00000002, 0x05647e38, 0xfa4bf362, 0x05647e38, 0x327c9f18, 0xe3598145, 0x00000002,
		0x05d9333f, 0xfb86a733, 0x05d9333f, 0x3400e913, 0xe4c6093b, 0x00000002, 0x02366680, 0xffa5d34a,
		0x02366680, 0x35ada3bf, 0xe63fbbf7, 0x00000002, 0x0082c4df, 0x0084bd96, 0x0082c4df, 0x370ab423,
		0xe76b048a, 0x00000002, 0x00000000, 0x2b47dcde, 0x2b47dcde, 0x06f26612, 0x00000000, 0x00000004
	};

	static const u32 IIR_COEF_384_TO_96[TBL_SZ_MEMASRC_IIR_COEF] = {
		0x023fb27b, 0xfcd2f985, 0x023fb27b, 0x326cca6b, 0xe056495c, 0x00000002, 0x0a37a3a1, 0xf1d5e48b,
		0x0a37a3a1, 0x3265763d, 0xe110322b, 0x00000002, 0x084e4d06, 0xf4ffdd82, 0x084e4d06, 0x32d2c1e7,
		0xe1f44efd, 0x00000002, 0x05c29422, 0xf91777ea, 0x05c29422, 0x33ae7ff0, 0xe3183fb3, 0x00000002,
		0x064224ff, 0xfa1849cc, 0x064224ff, 0x34e75649, 0xe47c15ec, 0x00000002, 0x0253663f, 0xff1b8654,
		0x0253663f, 0x36483578, 0xe5f577b6, 0x00000002, 0x00816bda, 0x006b4ca3, 0x00816bda, 0x376c1778,
		0xe725c430, 0x00000002, 0x00000000, 0x28b0e793, 0x28b0e793, 0x06fbe6c3, 0x00000000, 0x00000004
	};

	static const u32 IIR_COEF_384_TO_88[TBL_SZ_MEMASRC_IIR_COEF] = {
		0x02a62553, 0xfc064b8f, 0x02a62553, 0x334dac81, 0xe046a1ac, 0x00000002, 0x0b7129c3, 0xef0c6c3b,
		0x0b7129c3, 0x333f4ca6, 0xe0e333d7, 0x00000002, 0x0988a73e, 0xf251f92c, 0x0988a73e, 0x338970d0,
		0xe1afffe0, 0x00000002, 0x06d3ff07, 0xf6dd0d45, 0x06d3ff07, 0x342bbad5, 0xe2ca2e85, 0x00000002,
		0x07a6625e, 0xf756da5d, 0x07a6625e, 0x3520a02f, 0xe43bc0b9, 0x00000002, 0x02dcefe5, 0xfe24470b,
		0x02dcefe5, 0x364422e9, 0xe5ddb642, 0x00000002, 0x0095d0b1, 0x00542b8f, 0x0095d0b1, 0x374028ca,
		0xe7400a46, 0x00000002, 0x00000000, 0x27338238, 0x27338238, 0x06f4c3b3, 0x00000000, 0x00000004
	};

	static const u32 IIR_COEF_256_TO_48[TBL_SZ_MEMASRC_IIR_COEF] = {
		0x02b72fb4, 0xfb7c5152, 0x02b72fb4, 0x374ab8ef, 0xe039095c, 0x00000002, 0x05ca62de, 0xf673171b,
		0x05ca62de, 0x1b94186a, 0xf05c2de7, 0x00000003, 0x09a9656a, 0xf05ffe29, 0x09a9656a, 0x37394e81,
		0xe1611f87, 0x00000002, 0x06e86c29, 0xf54bf713, 0x06e86c29, 0x37797f41, 0xe24ce1f6, 0x00000002,
		0x07a6b7c2, 0xf5491ea7, 0x07a6b7c2, 0x37e40444, 0xe3856d91, 0x00000002, 0x02bf8a3e, 0xfd2f5fa6,
		0x02bf8a3e, 0x38673190, 0xe4ea5a4d, 0x00000002, 0x007e1bd5, 0x000e76ca, 0x007e1bd5, 0x38da5414,
		0xe61afd77, 0x00000002, 0x00000000, 0x2038247b, 0x2038247b, 0x07212644, 0x00000000, 0x00000004
	};

	static const u32 IIR_COEF_352_TO_64[TBL_SZ_MEMASRC_IIR_COEF] = {
		0x02a6b37b, 0xfb888156, 0x02a6b37b, 0x37f859ff, 0xe0386456, 0x00000002, 0x05b20926, 0xf679dff1,
		0x05b20926, 0x1be93c6e, 0xf05adf16, 0x00000003, 0x09753fb7, 0xf07a0567, 0x09753fb7, 0x37dcb603,
		0xe15a4054, 0x00000002, 0x06b68dd9, 0xf56b6f20, 0x06b68dd9, 0x3811806b, 0xe23d69ee, 0x00000002,
		0x075ca584, 0xf5743cd8, 0x075ca584, 0x386b6017, 0xe367180a, 0x00000002, 0x029bfc8d, 0xfd367105,
		0x029bfc8d, 0x38da2ce4, 0xe4b768fc, 0x00000002, 0x007521f2, 0x0006baaa, 0x007521f2, 0x393b06d9,
		0xe5d3fa99, 0x00000002, 0x00000000, 0x1ee87e92, 0x1ee87e92, 0x072c4a47, 0x00000000, 0x00000004
	};

	static const u32 IIR_COEF_384_TO_64[TBL_SZ_MEMASRC_IIR_COEF] = {
		0x029d40ba, 0xfb79d6e0, 0x029d40ba, 0x392d8096, 0xe034dd0f, 0x00000002, 0x05a2a7fa, 0xf64f6c2e,
		0x05a2a7fa, 0x1c817399, 0xf0551425, 0x00000003, 0x0953d637, 0xf036ffec, 0x0953d637, 0x38ff5268,
		0xe14354e5, 0x00000002, 0x06952665, 0xf53ce19a, 0x06952665, 0x391c1353, 0xe2158fe4, 0x00000002,
		0x072672d6, 0xf538294e, 0x072672d6, 0x39539b19, 0xe32755ee, 0x00000002, 0x027d1bb9, 0xfd10e42d,
		0x027d1bb9, 0x399a1610, 0xe45ace52, 0x00000002, 0x006a8eb3, 0xfff50a50, 0x006a8eb3, 0x39d826e9,
		0xe55db161, 0x00000002, 0x00000000, 0x1c6ede8b, 0x1c6ede8b, 0x073e2bc6, 0x00000000, 0x00000004
	};

	static const u32 IIR_COEF_352_TO_48[TBL_SZ_MEMASRC_IIR_COEF] = {
		0x02f3c075, 0xfaa0f2bb, 0x02f3c075, 0x3b1e2ce5, 0xe0266497, 0x00000002, 0x061654a7, 0xf4f6e06e,
		0x061654a7, 0x1d7a0223, 0xf03eea63, 0x00000003, 0x0525c445, 0xf6c06638, 0x0525c445, 0x1d6cc79b,
		0xf07b5ae0, 0x00000003, 0x077bc6f3, 0xf2d1482b, 0x077bc6f3, 0x3ac6a73a, 0xe1a7aca5, 0x00000002,
		0x0861aac3, 0xf1e8c6c3, 0x0861aac3, 0x3ab6aa60, 0xe29d3957, 0x00000002, 0x02f20c88, 0xfbb246f6,
		0x02f20c88, 0x3aa813c9, 0xe3c18c32, 0x00000002, 0x0072d6df, 0xffba3768, 0x0072d6df, 0x3a9ca779,
		0xe4c37362, 0x00000002, 0x00000000, 0x2ffa2764, 0x2ffa2764, 0x0ea60a6b, 0x00000000, 0x00000003
	};

	static const u32 IIR_COEF_384_TO_48[TBL_SZ_MEMASRC_IIR_COEF] = {
		0x02d975a0, 0xfabc9d25, 0x02d975a0, 0x3bece279, 0xe024cd2f, 0x00000002, 0x05f3612a, 0xf50b0a4f,
		0x05f3612a, 0x1de062d7, 0xf03c03e5, 0x00000003, 0x04ff0085, 0xf6df7cf4, 0x04ff0085, 0x1dd063ae,
		0xf074a058, 0x00000003, 0x072ec2f0, 0xf31a41fd, 0x072ec2f0, 0x3b84724d, 0xe18bfdf7, 0x00000002,
		0x07ec0168, 0xf2572ea5, 0x07ec0168, 0x3b67118c, 0xe269bd00, 0x00000002, 0x02ba0b16, 0xfbd5e151,
		0x02ba0b16, 0x3b48b8a8, 0xe36d4fda, 0x00000002, 0x00662a40, 0xffb4d0a9, 0x00662a40, 0x3b2fa355,
		0xe44f3782, 0x00000002, 0x00000000, 0x2bc64b7d, 0x2bc64b7d, 0x0ec96668, 0x00000000, 0x00000003
	};

	static const u32 IIR_COEF_384_TO_44[TBL_SZ_MEMASRC_IIR_COEF] = {
		0x03039659, 0xfa5c23d4, 0x03039659, 0x3c785635, 0xe01fb163, 0x00000002, 0x0628eac7, 0xf4805b4c,
		0x0628eac7, 0x1e27797e, 0xf03424d0, 0x00000003, 0x053a5bfc, 0xf64cf807, 0x053a5bfc, 0x1e15b5a9,
		0xf066ea58, 0x00000003, 0x07a260b2, 0xf206beb1, 0x07a260b2, 0x3c048d2d, 0xe16472aa, 0x00000002,
		0x0892f686, 0xf0ccdf29, 0x0892f686, 0x3bd501d2, 0xe23831f8, 0x00000002, 0x02fd4044, 0xfb2f4ecd,
		0x02fd4044, 0x3b9ec25d, 0xe3376e4d, 0x00000002, 0x006d4eef, 0xff9b0b4d, 0x006d4eef, 0x3b6f651a,
		0xe41af1bb, 0x00000002, 0x00000000, 0x28bbbe8a, 0x28bbbe8a, 0x0ed6fd4e, 0x00000000, 0x00000003
	};

	static const u32 IIR_COEF_352_TO_32[TBL_SZ_MEMASRC_IIR_COEF] = {
		0x0314039e, 0xfa16c22d, 0x0314039e, 0x3dc6a869, 0xe0185784, 0x00000002, 0x063c1999, 0xf40b68ae,
		0x063c1999, 0x1ed08ff8, 0xf0283846, 0x00000003, 0x0550267e, 0xf5d9545e, 0x0550267e, 0x1ebccb61,
		0xf04ff1ec, 0x00000003, 0x07cc8c28, 0xf137f60c, 0x07cc8c28, 0x3d467c42, 0xe1176705, 0x00000002,
		0x08cb7062, 0xefa566fe, 0x08cb7062, 0x3d01dd91, 0xe1c1daac, 0x00000002, 0x030c40f1, 0xfaa57571,
		0x030c40f1, 0x3cafcf6a, 0xe2923943, 0x00000002, 0x00685a9f, 0xff7ab6e7, 0x00685a9f, 0x3c665fb6,
		0xe34e3424, 0x00000002, 0x00000000, 0x2046052b, 0x2046052b, 0x0f11f8dd, 0x00000000, 0x00000003
	};

	static const u32 IIR_COEF_384_TO_32[TBL_SZ_MEMASRC_IIR_COEF] = {
		0x032f10f1, 0xf9d95961, 0x032f10f1, 0x3e1158bc, 0xe01583e6, 0x00000002, 0x065bac8d, 0xf3bb21b9,
		0x065bac8d, 0x1ef747e9, 0xf023c270, 0x00000003, 0x0574b803, 0xf580c27d, 0x0574b803, 0x1ee3c915,
		0xf047d1e6, 0x00000003, 0x08179a23, 0xf0885da5, 0x08179a23, 0x3d927e61, 0xe0fe5dc7, 0x00000002,
		0x093e2f0c, 0xee9b6c3c, 0x093e2f0c, 0x3d48a322, 0xe19f928b, 0x00000002, 0x033d2e85, 0xfa2f0f19,
		0x033d2e85, 0x3cedff76, 0xe2689466, 0x00000002, 0x006ded19, 0xff67ddb5, 0x006ded19, 0x3c9b5197,
		0xe320f682, 0x00000002, 0x00000000, 0x1e1581e2, 0x1e1581e2, 0x0f1e283c, 0x00000000, 0x00000003
	};

	static const u32 IIR_COEF_352_TO_24[TBL_SZ_MEMASRC_IIR_COEF] = {
		0x035e047f, 0xf96b61f8, 0x035e047f, 0x3ea9e77e, 0xe0103d3f, 0x00000002, 0x068ecd54, 0xf331816d,
		0x068ecd54, 0x1f468083, 0xf01b4754, 0x00000003, 0x05b1d20a, 0xf4e638ca, 0x05b1d20a, 0x1f347549,
		0xf037d0fd, 0x00000003, 0x0899975f, 0xef4dc655, 0x0899975f, 0x3e339e50, 0xe0ca7788, 0x00000002,
		0x0506416b, 0xf657152f, 0x0506416b, 0x1ef29626, 0xf0a9d1d5, 0x00000003, 0x0397fd6f, 0xf94a745b,
		0x0397fd6f, 0x3d80a443, 0xe204ec85, 0x00000002, 0x007807fd, 0xff417ee3, 0x007807fd, 0x3d21cefe,
		0xe2aca225, 0x00000002, 0x00000000, 0x1902d021, 0x1902d021, 0x0f3e4f5a, 0x00000000, 0x00000003
	};

	static const u32 IIR_COEF_384_TO_24[TBL_SZ_MEMASRC_IIR_COEF] = {
		0x038fcda4, 0xf9036a1f, 0x038fcda4, 0x3eda69b0, 0xe00d87a5, 0x00000002, 0x06bff5be, 0xf2c44847,
		0x06bff5be, 0x1f609be0, 0xf016fc9b, 0x00000003, 0x05eecea4, 0xf46276e0, 0x05eecea4, 0x1f4fdbce,
		0xf02ffbe3, 0x00000003, 0x0490e7d9, 0xf716aef0, 0x0490e7d9, 0x1f35bb3e, 0xf059477f, 0x00000003,
		0x05790864, 0xf56662d5, 0x05790864, 0x1f0d7f0d, 0xf09a0d56, 0x00000003, 0x0405a7b9, 0xf863223b,
		0x0405a7b9, 0x3dafa23c, 0xe1e1ec18, 0x00000002, 0x0087c12c, 0xff1dcc3e, 0x0087c12c, 0x3d46e81a,
		0xe28bc951, 0x00000002, 0x00000000, 0x2e64814a, 0x2e64814a, 0x1e8c999c, 0x00000000, 0x00000002
	};

	static const struct {
		const u32 *coef;
		size_t cnt;
	} iir_coef_tbl_list[23] = {
		{ IIR_COEF_384_TO_352, ARRAY_SIZE(IIR_COEF_384_TO_352) },/* 0 */
		{ IIR_COEF_256_TO_192, ARRAY_SIZE(IIR_COEF_256_TO_192) },/* 1 */
		{ IIR_COEF_352_TO_256, ARRAY_SIZE(IIR_COEF_352_TO_256) },/* 2 */
		{ IIR_COEF_384_TO_256, ARRAY_SIZE(IIR_COEF_384_TO_256) },/* 3 */
		{ IIR_COEF_352_TO_192, ARRAY_SIZE(IIR_COEF_352_TO_192) },/* 4 */
		{ IIR_COEF_384_TO_192, ARRAY_SIZE(IIR_COEF_384_TO_192) },/* 5 */
		{ IIR_COEF_384_TO_176, ARRAY_SIZE(IIR_COEF_384_TO_176) },/* 6 */
		{ IIR_COEF_256_TO_96, ARRAY_SIZE(IIR_COEF_256_TO_96) },/* 7 */
		{ IIR_COEF_352_TO_128, ARRAY_SIZE(IIR_COEF_352_TO_128) },/* 8 */
		{ IIR_COEF_384_TO_128, ARRAY_SIZE(IIR_COEF_384_TO_128) },/* 9 */
		{ IIR_COEF_352_TO_96, ARRAY_SIZE(IIR_COEF_352_TO_96) },/* 10 */
		{ IIR_COEF_384_TO_96, ARRAY_SIZE(IIR_COEF_384_TO_96) },/* 11 */
		{ IIR_COEF_384_TO_88, ARRAY_SIZE(IIR_COEF_384_TO_88) },/* 12 */
		{ IIR_COEF_256_TO_48, ARRAY_SIZE(IIR_COEF_256_TO_48) },/* 13 */
		{ IIR_COEF_352_TO_64, ARRAY_SIZE(IIR_COEF_352_TO_64) },/* 14 */
		{ IIR_COEF_384_TO_64, ARRAY_SIZE(IIR_COEF_384_TO_64) },/* 15 */
		{ IIR_COEF_352_TO_48, ARRAY_SIZE(IIR_COEF_352_TO_48) },/* 16 */
		{ IIR_COEF_384_TO_48, ARRAY_SIZE(IIR_COEF_384_TO_48) },/* 17 */
		{ IIR_COEF_384_TO_44, ARRAY_SIZE(IIR_COEF_384_TO_44) },/* 18 */
		{ IIR_COEF_352_TO_32, ARRAY_SIZE(IIR_COEF_352_TO_32) },/* 19 */
		{ IIR_COEF_384_TO_32, ARRAY_SIZE(IIR_COEF_384_TO_32) },/* 20 */
		{ IIR_COEF_352_TO_24, ARRAY_SIZE(IIR_COEF_352_TO_24) },/* 21 */
		{ IIR_COEF_384_TO_24, ARRAY_SIZE(IIR_COEF_384_TO_24) },/* 22 */
	};

	static const u32 freq_new_index[25] = {
		0, 1, 2, 3, 4, 5, 6, 7,
		8, 99, 99, 99, 99, 99, 99, 99,
		9, 10, 11, 12, 13, 14, 15, 16,
		17
	};

	static const u32 iir_coef_tbl_matrix[18][18] = {
		{
			NO_NEED, NO_NEED, NO_NEED, NO_NEED, NO_NEED, NO_NEED, NO_NEED, NO_NEED, NO_NEED, 0,
			NO_NEED, NO_NEED, NO_NEED, NO_NEED, NO_NEED, NO_NEED, NO_NEED, NO_NEED
		},
		{
			3, NO_NEED, NO_NEED, NO_NEED, NO_NEED, NO_NEED, NO_NEED, NO_NEED, NO_NEED, INV_COEF, 0,
			NO_NEED, NO_NEED, NO_NEED, NO_NEED, NO_NEED, NO_NEED, NO_NEED
		},
		{
			5, 1, NO_NEED, NO_NEED, NO_NEED, NO_NEED, NO_NEED, NO_NEED, NO_NEED, 6, INV_COEF, 0,
			NO_NEED, NO_NEED, NO_NEED, NO_NEED, NO_NEED, NO_NEED
		},
		{
			9, 5, 3, NO_NEED, NO_NEED, NO_NEED, NO_NEED, NO_NEED, NO_NEED, INV_COEF, 6, INV_COEF, 0,
			NO_NEED, NO_NEED, NO_NEED, NO_NEED, NO_NEED
		},
		{
			11, 7, 5, 1, NO_NEED, NO_NEED, NO_NEED, NO_NEED, NO_NEED, 12, INV_COEF, 6, INV_COEF, 0,
			NO_NEED, NO_NEED, NO_NEED, NO_NEED
		},
		{
			15, 11, 9, 5, 3, NO_NEED, NO_NEED, NO_NEED, NO_NEED, INV_COEF, 12, INV_COEF, 6, INV_COEF,
			0, NO_NEED, NO_NEED, NO_NEED
		},
		{
			20, 17, 15, 11, 9, 5, NO_NEED, NO_NEED, NO_NEED, INV_COEF, 18, INV_COEF, 12, INV_COEF, 6,
			0, NO_NEED, NO_NEED
		},
		{
			RATIOVER, 22, 20, 17, 15, 11, 5, NO_NEED, NO_NEED, RATIOVER, RATIOVER, INV_COEF, 18,
			INV_COEF, 12, 6, 0, NO_NEED
		},
		{
			RATIOVER, RATIOVER, RATIOVER, 22, 20, 17, 11, 5, NO_NEED, RATIOVER, RATIOVER, RATIOVER,
			RATIOVER, INV_COEF, 18, 12, 6, 0
		},
		{
			NO_NEED, NO_NEED, NO_NEED, NO_NEED, NO_NEED, NO_NEED, NO_NEED, NO_NEED, NO_NEED, NO_NEED,
			NO_NEED, NO_NEED, NO_NEED, NO_NEED, NO_NEED, NO_NEED, NO_NEED, NO_NEED
		},
		{
			2, NO_NEED, NO_NEED, NO_NEED, NO_NEED, NO_NEED, NO_NEED, NO_NEED, NO_NEED, 3, NO_NEED,
			NO_NEED, NO_NEED, NO_NEED, NO_NEED, NO_NEED, NO_NEED, NO_NEED
		},
		{
			4, INV_COEF, NO_NEED, NO_NEED, NO_NEED, NO_NEED, NO_NEED, NO_NEED, NO_NEED, 5, 1, NO_NEED,
			NO_NEED, NO_NEED, NO_NEED, NO_NEED, NO_NEED, NO_NEED
		},
		{
			8, 4, 2, NO_NEED, NO_NEED, NO_NEED, NO_NEED, NO_NEED, NO_NEED, 9, 5, 3, NO_NEED, NO_NEED,
			NO_NEED, NO_NEED, NO_NEED, NO_NEED
		},
		{
			10, INV_COEF, 4, INV_COEF, NO_NEED, NO_NEED, NO_NEED, NO_NEED, NO_NEED, 11, 7, 5, 1,
			NO_NEED, NO_NEED, NO_NEED, NO_NEED, NO_NEED
		},
		{
			14, 10, 8, 4, 2, NO_NEED, NO_NEED, NO_NEED, NO_NEED, 15, 11, 9, 5, 3, NO_NEED, NO_NEED,
			NO_NEED, NO_NEED
		},
		{
			19, 16, 14, 10, 8, 4, NO_NEED, NO_NEED, NO_NEED, 20, 17, 15, 11, 9, 5, NO_NEED, NO_NEED,
			NO_NEED
		},
		{
			RATIOVER, 21, 19, 16, 14, 10, 4, NO_NEED, NO_NEED, RATIOVER, 22, 20, 17, 15, 11, 5,
			NO_NEED, NO_NEED
		},
		{
			RATIOVER, RATIOVER, RATIOVER, 21, 19, 16, 10, 4, NO_NEED, RATIOVER, RATIOVER, RATIOVER, 22,
			20, 17, 11, 5, NO_NEED
		}
	};

	const u32 *coef = NULL;
	size_t cnt = 0;
	u32 i = freq_new_index[input_fs];
	u32 j = freq_new_index[output_fs];

	if (i >= 18 || j >= 18) {
		pr_err("%s() error: input_fs=0x%x, i=%u, output_fs=0x%x, j=%u\n",
			__func__, input_fs, i, output_fs, j);
	} else {
		u32 k = iir_coef_tbl_matrix[i][j];

		pr_debug("%s() input_fs=0x%x, output_fs=0x%x, i=%u, j=%u, k=%u\n",
			__func__, input_fs, output_fs, i, j, k);

		if (k >= NO_NEED)
			pr_debug("%s() warning: NO_NEED\n", __func__);
		else if (k == RATIOVER)
			pr_debug("%s() warning: up-sampling ratio exceeds 16\n", __func__);
		else if (k == INV_COEF)
			pr_debug("%s() warning: up-sampling ratio is rare, need gen coef\n", __func__);
		else {
			coef = iir_coef_tbl_list[k].coef;
			cnt = iir_coef_tbl_list[k].cnt;
		}
	}
	*count = cnt;
	return coef;
}

int afe_sample_asrc_tx_configurate(enum afe_sample_asrc_tx_id id,
				   const struct afe_sample_asrc_config *config)
{
	u32 addrCON0, addrCON1, addrCON4, addrCON6, addrCON7, addrCON10, addrCON11, addrCON13,
	    addrCON14;
	u32 val, msk;
	const u32 *coef;
	size_t coef_count = 0;

	switch (id) {
	case SAMPLE_ASRC_O1:
		addrCON0 = AFE_ASRCO1_NEW_CON0;
		addrCON1 = AFE_ASRCO1_NEW_CON1;
		addrCON4 = AFE_ASRCO1_NEW_CON4;
		addrCON6 = AFE_ASRCO1_NEW_CON6;
		addrCON7 = AFE_ASRCO1_NEW_CON7;
		addrCON10 = AFE_ASRCO1_NEW_CON10;
		addrCON11 = AFE_ASRCO1_NEW_CON11;
		addrCON13 = AFE_ASRCO1_NEW_CON13;
		addrCON14 = AFE_ASRCO1_NEW_CON14;
		break;
	case SAMPLE_ASRC_O2:
		addrCON0 = AFE_ASRCO2_NEW_CON0;
		addrCON1 = AFE_ASRCO2_NEW_CON1;
		addrCON4 = AFE_ASRCO2_NEW_CON4;
		addrCON6 = AFE_ASRCO2_NEW_CON6;
		addrCON7 = AFE_ASRCO2_NEW_CON7;
		addrCON10 = AFE_ASRCO2_NEW_CON10;
		addrCON11 = AFE_ASRCO2_NEW_CON11;
		addrCON13 = AFE_ASRCO2_NEW_CON13;
		addrCON14 = AFE_ASRCO2_NEW_CON14;
		break;
	case SAMPLE_ASRC_O3:
		addrCON0 = AFE_ASRCO3_NEW_CON0;
		addrCON1 = AFE_ASRCO3_NEW_CON1;
		addrCON4 = AFE_ASRCO3_NEW_CON4;
		addrCON6 = AFE_ASRCO3_NEW_CON6;
		addrCON7 = AFE_ASRCO3_NEW_CON7;
		addrCON10 = AFE_ASRCO3_NEW_CON10;
		addrCON11 = AFE_ASRCO3_NEW_CON11;
		addrCON13 = AFE_ASRCO3_NEW_CON13;
		addrCON14 = AFE_ASRCO3_NEW_CON14;
		break;
	case SAMPLE_ASRC_O4:
		addrCON0 = AFE_ASRCO4_NEW_CON0;
		addrCON1 = AFE_ASRCO4_NEW_CON1;
		addrCON4 = AFE_ASRCO4_NEW_CON4;
		addrCON6 = AFE_ASRCO4_NEW_CON6;
		addrCON7 = AFE_ASRCO4_NEW_CON7;
		addrCON10 = AFE_ASRCO4_NEW_CON10;
		addrCON11 = AFE_ASRCO4_NEW_CON11;
		addrCON13 = AFE_ASRCO4_NEW_CON13;
		addrCON14 = AFE_ASRCO4_NEW_CON14;
		break;
	case SAMPLE_ASRC_O5:
		addrCON0 = AFE_ASRCO5_NEW_CON0;
		addrCON1 = AFE_ASRCO5_NEW_CON1;
		addrCON4 = AFE_ASRCO5_NEW_CON4;
		addrCON6 = AFE_ASRCO5_NEW_CON6;
		addrCON7 = AFE_ASRCO5_NEW_CON7;
		addrCON10 = AFE_ASRCO5_NEW_CON10;
		addrCON11 = AFE_ASRCO5_NEW_CON11;
		addrCON13 = AFE_ASRCO5_NEW_CON13;
		addrCON14 = AFE_ASRCO5_NEW_CON14;
		break;
	case SAMPLE_ASRC_O6:
		addrCON0 = AFE_ASRCO6_NEW_CON0;
		addrCON1 = AFE_ASRCO6_NEW_CON1;
		addrCON4 = AFE_ASRCO6_NEW_CON4;
		addrCON6 = AFE_ASRCO6_NEW_CON6;
		addrCON7 = AFE_ASRCO6_NEW_CON7;
		addrCON10 = AFE_ASRCO6_NEW_CON10;
		addrCON11 = AFE_ASRCO6_NEW_CON11;
		addrCON13 = AFE_ASRCO6_NEW_CON13;
		addrCON14 = AFE_ASRCO6_NEW_CON14;
		break;
	case SAMPLE_ASRC_PCM_OUT:
		addrCON0 = AFE_ASRCPCMO_NEW_CON0;
		addrCON1 = AFE_ASRCPCMO_NEW_CON1;
		addrCON4 = AFE_ASRCPCMO_NEW_CON4;
		addrCON6 = AFE_ASRCPCMO_NEW_CON6;
		addrCON7 = AFE_ASRCPCMO_NEW_CON7;
		addrCON10 = AFE_ASRCPCMO_NEW_CON10;
		addrCON11 = AFE_ASRCPCMO_NEW_CON11;
		addrCON13 = AFE_ASRCPCMO_NEW_CON13;
		addrCON14 = AFE_ASRCPCMO_NEW_CON14;
		break;
	default:
		pr_err("error: invalid afe_sample_asrc_tx_id\n");
		return -EINVAL;
	}
	/* CON0 setting */
	val = (config->o16bit << 19)
	      | (config->mono << 16)
	      | (0x0 << 14)
	      | (0x3 << 12);
	msk = O16BIT_MASK | IS_MONO_MASK | (0x3 << 14)
	      | (0x3 << 12);
	afe_msk_write(addrCON0, val, msk);
	coef = get_iir_coef(config->input_fs, config->output_fs, &coef_count);
	if (coef) {
		size_t i;

		afe_msk_write(addrCON0, 0x1 << 1, 0x1 << 1);	/* CPU control IIR coeff SRAM */
		afe_write(addrCON11, 0x0);	/* set to 0, IIR coeff SRAM addr */
		for (i = 0; i < coef_count; ++i)
			afe_write(addrCON10, coef[i]);
		afe_msk_write(addrCON0, 0x0 << 1, 0x1 << 1);	/* disable IIR coeff SRAM access */
		afe_msk_write(addrCON0, CLR_IIR_HISTORY | IIR_EN | IIR_STAGE_8,
			      CLR_IIR_HISTORY_MASK | IIR_EN_MASK | IIR_STAGE_MASK);
	} else
		afe_msk_write(addrCON0, IIR_DIS, IIR_EN_MASK);
	/* CON4 setting (output period) (controlled by HW if tracking) */
	val = PeriodPalette(config->output_fs);
	afe_msk_write(addrCON4, val, 0x00FFFFFF);
	/* CON1 setting (input period) (fixed) */
	val = PeriodPalette(config->input_fs);
	afe_msk_write(addrCON1, val, 0x00FFFFFF);
	/* CON6 setting */
	if (config->tracking)
		afe_write(addrCON6, 0x003F988F);
	else {
		afe_msk_write(addrCON6, (0x0 << 3) | (0x0 << 12) | (0x1 << 1) | (0x0 << 0)
			      , (0x1 << 3) | (0x1 << 12) | (0x1 << 1) | (0x1 << 0));
	}
	/* CON7 setting */
	afe_write(addrCON7, 0x3C00);
	/* CON13 setting */
	val = AutoRstThHi(config->output_fs);
	afe_write(addrCON13, val);
	/* CON14 setting */
	val = AutoRstThLo(config->output_fs);
	afe_write(addrCON14, val);
	return 0;
}

int afe_sample_asrc_tx_enable(enum afe_sample_asrc_tx_id id, int en)
{
	u32 addrCON0;
	static u32 addrCON0s[] = {
		AFE_ASRCO1_NEW_CON0, AFE_ASRCO2_NEW_CON0, AFE_ASRCO3_NEW_CON0, AFE_ASRCO4_NEW_CON0,
		AFE_ASRCO5_NEW_CON0, AFE_ASRCO6_NEW_CON0, AFE_ASRCPCMO_NEW_CON0
	};

	if (id >= SAMPLE_ASRC_OUT_NUM)
		return -EINVAL;
	addrCON0 = addrCON0s[id];
	if (en) {
		afe_msk_write(addrCON0, (0x1 << 4), (0x1 << 4));	/* clear */
		afe_msk_write(addrCON0, (0x1 << 4) | (0x1 << 0), (0x1 << 4) | ASM_ON_MASK);	/* clear and ON */
	} else {
		afe_msk_write(addrCON0, (0x0 << 0), ASM_ON_MASK);	/* OFF */
	}
	return 0;
}

int afe_sample_asrc_rx_configurate(enum afe_sample_asrc_rx_id id,
				   const struct afe_sample_asrc_config *config)
{
	u32 addrCON0, addrCON2, addrCON3, addrCON6, addrCON7, addrCON10, addrCON11, addrCON13,
	    addrCON14;
	u32 val, msk;
	const u32 *coef;
	size_t coef_count = 0;

	switch (id) {
	case SAMPLE_ASRC_I1:
		addrCON0 = AFE_ASRC_NEW_CON0;
		addrCON2 = AFE_ASRC_NEW_CON2;
		addrCON3 = AFE_ASRC_NEW_CON3;
		addrCON6 = AFE_ASRC_NEW_CON6;
		addrCON7 = AFE_ASRC_NEW_CON7;
		addrCON10 = AFE_ASRC_NEW_CON10;
		addrCON11 = AFE_ASRC_NEW_CON11;
		addrCON13 = AFE_ASRC_NEW_CON13;
		addrCON14 = AFE_ASRC_NEW_CON14;
		break;
	case SAMPLE_ASRC_I2:
		addrCON0 = AFE_ASRCI2_NEW_CON0;
		addrCON2 = AFE_ASRCI2_NEW_CON2;
		addrCON3 = AFE_ASRCI2_NEW_CON3;
		addrCON6 = AFE_ASRCI2_NEW_CON6;
		addrCON7 = AFE_ASRCI2_NEW_CON7;
		addrCON10 = AFE_ASRCI2_NEW_CON10;
		addrCON11 = AFE_ASRCI2_NEW_CON11;
		addrCON13 = AFE_ASRCI2_NEW_CON13;
		addrCON14 = AFE_ASRCI2_NEW_CON14;
		break;
	case SAMPLE_ASRC_I3:
		addrCON0 = AFE_ASRCI3_NEW_CON0;
		addrCON2 = AFE_ASRCI3_NEW_CON2;
		addrCON3 = AFE_ASRCI3_NEW_CON3;
		addrCON6 = AFE_ASRCI3_NEW_CON6;
		addrCON7 = AFE_ASRCI3_NEW_CON7;
		addrCON10 = AFE_ASRCI3_NEW_CON10;
		addrCON11 = AFE_ASRCI3_NEW_CON11;
		addrCON13 = AFE_ASRCI3_NEW_CON13;
		addrCON14 = AFE_ASRCI3_NEW_CON14;
		break;
	case SAMPLE_ASRC_I4:
		addrCON0 = AFE_ASRCI4_NEW_CON0;
		addrCON2 = AFE_ASRCI4_NEW_CON2;
		addrCON3 = AFE_ASRCI4_NEW_CON3;
		addrCON6 = AFE_ASRCI4_NEW_CON6;
		addrCON7 = AFE_ASRCI4_NEW_CON7;
		addrCON10 = AFE_ASRCI4_NEW_CON10;
		addrCON11 = AFE_ASRCI4_NEW_CON11;
		addrCON13 = AFE_ASRCI4_NEW_CON13;
		addrCON14 = AFE_ASRCI4_NEW_CON14;
		break;
	case SAMPLE_ASRC_I5:
		addrCON0 = AFE_ASRCI5_NEW_CON0;
		addrCON2 = AFE_ASRCI5_NEW_CON2;
		addrCON3 = AFE_ASRCI5_NEW_CON3;
		addrCON6 = AFE_ASRCI5_NEW_CON6;
		addrCON7 = AFE_ASRCI5_NEW_CON7;
		addrCON10 = AFE_ASRCI5_NEW_CON10;
		addrCON11 = AFE_ASRCI5_NEW_CON11;
		addrCON13 = AFE_ASRCI5_NEW_CON13;
		addrCON14 = AFE_ASRCI5_NEW_CON14;
		break;
	case SAMPLE_ASRC_I6:
		addrCON0 = AFE_ASRCI6_NEW_CON0;
		addrCON2 = AFE_ASRCI6_NEW_CON2;
		addrCON3 = AFE_ASRCI6_NEW_CON3;
		addrCON6 = AFE_ASRCI6_NEW_CON6;
		addrCON7 = AFE_ASRCI6_NEW_CON7;
		addrCON10 = AFE_ASRCI6_NEW_CON10;
		addrCON11 = AFE_ASRCI6_NEW_CON11;
		addrCON13 = AFE_ASRCI6_NEW_CON13;
		addrCON14 = AFE_ASRCI6_NEW_CON14;
		break;
	case SAMPLE_ASRC_PCM_IN:
		addrCON0 = AFE_ASRCPCMI_NEW_CON0;
		addrCON2 = AFE_ASRCPCMI_NEW_CON2;
		addrCON3 = AFE_ASRCPCMI_NEW_CON3;
		addrCON6 = AFE_ASRCPCMI_NEW_CON6;
		addrCON7 = AFE_ASRCPCMI_NEW_CON7;
		addrCON10 = AFE_ASRCPCMI_NEW_CON10;
		addrCON11 = AFE_ASRCPCMI_NEW_CON11;
		addrCON13 = AFE_ASRCPCMI_NEW_CON13;
		addrCON14 = AFE_ASRCPCMI_NEW_CON14;
		break;
	default:
		pr_err("error: invalid SAMPLE_ASRC_RX_ID\n");
		return -EINVAL;
	}
	/* CON0 setting */
	val = (config->o16bit << 19)
	      | (config->mono << 16)
	      | (0x1 << 14)
	      | (0x2 << 12);
	msk = O16BIT_MASK | IS_MONO_MASK | (0x3 << 14)
	      | (0x3 << 12);
	afe_msk_write(addrCON0, val, msk);
	coef = get_iir_coef(config->input_fs, config->output_fs, &coef_count);
	if (coef) {
		size_t i;

		afe_msk_write(addrCON0, 0x1 << 1, 0x1 << 1);	/* CPU control IIR coeff SRAM */
		afe_write(addrCON11, 0x0);	/* set to 0, IIR coeff SRAM addr */
		for (i = 0; i < coef_count; ++i)
			afe_write(addrCON10, coef[i]);
		afe_msk_write(addrCON0, 0x0 << 1, 0x1 << 1);	/* disable IIR coeff SRAM access */
		afe_msk_write(addrCON0, CLR_IIR_HISTORY | IIR_EN | IIR_STAGE_8,
			      CLR_IIR_HISTORY_MASK | IIR_EN_MASK | IIR_STAGE_MASK);
	} else
		afe_msk_write(addrCON0, IIR_DIS, IIR_EN_MASK);
	/* CON3 setting (input fs) (controlled by HW if tracking) */
	val = FrequencyPalette(config->input_fs);
	afe_msk_write(addrCON3, val, 0x00FFFFFF);
	/* CON2 setting (output fs) (fixed) */
	val = FrequencyPalette(config->output_fs);
	afe_msk_write(addrCON2, val, 0x00FFFFFF);
	/* CON6 setting */
	if (config->tracking)
		afe_write(addrCON6, 0x003F988F);
	else {
		afe_msk_write(addrCON6, (0x0 << 3) | (0x0 << 12) | (0x1 << 1) | (0x0 << 0)
			      , (0x1 << 3) | (0x1 << 12) | (0x1 << 1) | (0x1 << 0));
	}
	/* CON7 setting */
	afe_write(addrCON7, 0x3C00);
	/* CON13 setting */
	val = AutoRstThHi(config->input_fs);
	afe_write(addrCON13, val);
	/* CON14 setting */
	val = AutoRstThLo(config->input_fs);
	afe_write(addrCON14, val);
	return 0;
}

int afe_sample_asrc_rx_enable(enum afe_sample_asrc_rx_id id, int en)
{
	u32 addrCON0;
	static u32 addrCON0s[] = {
		AFE_ASRC_NEW_CON0, AFE_ASRCI2_NEW_CON0, AFE_ASRCI3_NEW_CON0, AFE_ASRCI4_NEW_CON0,
		AFE_ASRCI5_NEW_CON0, AFE_ASRCI6_NEW_CON0, AFE_ASRCPCMI_NEW_CON0
	};

	if (id >= SAMPLE_ASRC_IN_NUM)
		return -EINVAL;
	addrCON0 = addrCON0s[id];
	if (en) {
		afe_msk_write(addrCON0, (0x1 << 4), (0x1 << 4));	/* clear */
		afe_msk_write(addrCON0, (0x1 << 4) | (0x1 << 0), (0x1 << 4) | ASM_ON_MASK);	/* clear and ON */
	} else {
		afe_msk_write(addrCON0, (0x0 << 0), ASM_ON_MASK);	/* OFF */
	}
	return 0;
}

/******************** merge interface ********************/

int afe_daibt_configurate(struct afe_daibt_config *config)
{
	u32 val, msk;

	val = (config->daibt_c << C_POS)
	      | (config->daibt_ready << DATA_RDY_POS)
	      | (config->daibt_mode << DAIBT_MODE_POS)
	      | (config->afe_daibt_input << USE_MRGIF_INPUT_POS);
	msk = C_MASK | DATA_RDY_MASK | DAIBT_MODE_MASK | USE_MRGIF_INPUT_MASK;
	afe_msk_write(AFE_DAIBT_CON0, val, msk);
	return 0;
}

int afe_daibt_set_output_fs(enum afe_daibt_output_fs outputfs)
{
	if ((outputfs != DAIBT_OUTPUT_FS_8K) && (outputfs != DAIBT_OUTPUT_FS_16K))
		return -EINVAL;
	afe_msk_write(AFE_DAIBT_CON0, outputfs << DAIBT_MODE_POS, DAIBT_MODE_MASK);
	return 0;
}

void afe_daibt_set_enable(int en)
{
	en = !!en;
	afe_msk_write(AFE_DAIBT_CON0, en << DAIBT_ON_POS, DAIBT_ON_MASK);
}

void afe_merge_set_sync_dly(unsigned int  mrgsyncdly)
{
	mrgsyncdly &= 0xF;
	afe_msk_write(AFE_MRGIF_CON, mrgsyncdly << MRG_SYNC_DLY_POS, MRG_SYNC_DLY_MASK);
}

void afe_merge_set_clk_edge_dly(unsigned int  clkedgedly)
{
	clkedgedly &= 0x3;
	afe_msk_write(AFE_MRGIF_CON, clkedgedly << MRG_CLK_EDGE_DLY_POS, MRG_CLK_EDGE_DLY_MASK);
}

void afe_merge_set_clk_dly(unsigned int  clkdly)
{
	clkdly &= 0x3;
	afe_msk_write(AFE_MRGIF_CON, clkdly << MRG_CLK_DLY_POS, MRG_CLK_DLY_MASK);
}

void afe_merge_i2s_set_mode(enum afe_mrg_i2s_mode i2smode)
{
	afe_msk_write(AFE_MRGIF_CON, i2smode << MRGIF_I2S_MODE_POS, MRGIF_I2S_MODE_MASK);
}

void afe_merge_i2s_enable(int on)
{
	on = !!on;
	afe_msk_write(AFE_MRGIF_CON, on << MRGIF_I2S_EN_POS, MRGIF_I2S_EN_MASK);
}

void afe_merge_i2s_clk_invert(int on)
{
	/* 0:invert,1:non-invert */
	on = !!on;
	afe_msk_write(AFE_MRGIF_CON, on << MRG_CLK_NO_INV_POS, MRG_CLK_NO_INV_MASK);
}

void afe_merge_set_enable(int on)
{
	on = !!on;
	afe_msk_write(AFE_MRGIF_CON, (on << MRGIF_EN_POS), MRGIF_EN_MASK);
}

void afe_i26_pcm_rx_sel_pcmrx(int on)
{
	on = !!on;
	afe_msk_write(AFE_CONN35, on << 31, I26_PCM_RX_SEL_MASK);
}

void afe_o31_pcm_tx_sel_pcmtx(int on)
{
	on = !!on;
	afe_msk_write(AFE_CONN35, on << 30, O31_PCM_TX_SEL_MASK);
}

/******************** pcm interface ********************/

int afe_power_on_btpcm(int on)
{
	const int off = !on;

	afe_msk_write(AUDIO_TOP_CON4, off << 24, PDN_PCMIF);
	return 0;
}

int afe_power_on_mrg(int on)
{
	const int off = !on;

	afe_msk_write(AUDIO_TOP_CON4, off << 25, PDN_MRGIF);
	return 0;
}

int afe_power_on_intdir(int on)
{
	const int off = !on;

	/*
	 * [Programming Guide]
	 * [SPDIF IN]power on spdif in clock
	 */
	afe_msk_write(AUDIO_TOP_CON4, off << 20, PND_INTDIR);
	return 0;
}

int afe_btpcm_configurate(const struct afe_btpcm_config *config)
{
	u32 val, msk;

	val = (config->fmt << PCM_FMT_POS)
	      | (config->mode << PCM_MODE_POS)
	      | (config->slave << PCM_SLAVE_POS)
	      | (0x0 << BYP_ASRC_POS)
	      | (0x1 << SYNC_TYPE_POS)
	      | (0x1F << SYNC_LENGTH_POS)
	      | (config->extloopback << EXT_MODEM_POS)
	      | (config->wlen << PCM_WLEN_POS);
	msk = PCM_FMT_MASK
	      | PCM_MODE_MASK
	      | PCM_SLAVE_MASK
	      | BYP_ASRC_MASK | SYNC_TYPE_MASK | SYNC_LENGTH_MASK | EXT_MODEM_MASK | PCM_WLEN_MASK;
	afe_msk_write(PCM_INTF_CON1, val, msk);
	return 0;
}

int afe_btpcm_enable(int en)
{
	en = !!en;
	afe_msk_write(PCM_INTF_CON1, en << PCM_EN_POS, PCM_EN_MASK);
	return 0;
}


/******************** dmic ********************/

int afe_power_on_dmic(enum afe_dmic_id id, int on)
{
	int off = !on;
	int pos = PDN_DMIC1_POS + id;

	if (id >= AFE_DMIC_NUM)
		return -EINVAL;
	afe_msk_write(PWR2_TOP_CON, off << pos, 0x1 << pos);
	return 0;
}

int afe_dmic_configurate(enum afe_dmic_id id, const struct afe_dmic_config *config)
{
	u32 addr;
	u32 val, msk;
	enum afe_sampling_rate mode;

	if (id >= AFE_DMIC_NUM)
		return -EINVAL;
	mode = config->voice_mode;
	addr = DMIC_TOP_CON - (id * 0x150);
	val = (1 << DMIC_TIMING_ON_POS)
	      | (config->iir_on << DMIC_IIR_ON_POS)
	      | (config->iir_mode << DMIC_IIR_MODE_POS);
	switch (mode) {
	case FS_8000HZ:
		break;
	case FS_16000HZ:
		val |= (1 << DMIC_VOICE_MODE_POS);
		break;
	case FS_32000HZ:
		val |= (2 << DMIC_VOICE_MODE_POS);
		break;
	case FS_48000HZ:
		val |= (3 << DMIC_VOICE_MODE_POS);
		break;
	case FS_44100HZ:
		val |= ((3 << DMIC_VOICE_MODE_POS) | (1 << DMIC_DMSEL_POS));
		break;
	default:
		return -EINVAL;
	}
	msk = DMIC_TIMING_ON_MASK
	      | DMIC_IIR_ON_MASK | DMIC_IIR_MODE_MASK | DMIC_VOICE_MODE_MASK | DMIC_DMSEL_MASK;
	afe_msk_write(addr, val, msk);
	return 0;
}

int afe_dmic_enable(enum afe_dmic_id id, int en)
{
	en = !!en;
	afe_msk_write(DMIC_TOP_CON - (id * 0x150), en << DMIC_CIC_ON_POS, 0x1 << DMIC_CIC_ON_POS);
	return 0;
}


/********************* memory asrc *********************/

int afe_power_on_mem_asrc_brg(int on)
{
	static DEFINE_MUTEX(lock);
	static int status;

	mutex_lock(&lock);
	if (on) {
		if (status == 0)
			afe_clear_bit(PWR2_TOP_CON, 16);
		++status;
	} else {
		if (status > 0 && --status == 0)
			afe_set_bit(PWR2_TOP_CON, 16);
	}
	mutex_unlock(&lock);
	return 0;
}

int afe_power_on_mem_asrc(enum afe_mem_asrc_id id, int on)
{
	int pos;

	if (unlikely(id >= MEM_ASRC_NUM)) {
		pr_err("%s() error: invalid asrc[%d]\n", __func__, id);
		return -EINVAL;
	}
	if (on) {
		/* asrc_ck select asm_h_ck(270M) */
		pos = id * 3;
		afe_msk_write(PWR2_ASM_CON2, 0x2 << pos, 0x3 << pos);

		pos = PDN_MEM_ASRC1_POS + id;
		afe_clear_bit(PWR2_TOP_CON, pos);

		/* force reset */
		pos = MEM_ASRC_1_RESET_POS + id;
		afe_set_bit(PWR2_ASM_CON2, pos);
		afe_clear_bit(PWR2_ASM_CON2, pos);
	} else {
		pos = PDN_MEM_ASRC1_POS + id;
		afe_set_bit(PWR2_TOP_CON, pos);
	}
	return 0;
}

static const unsigned int asrc_irq[MEM_ASRC_NUM] = {
	165, 166, 167, 200, 201
};

int afe_mem_asrc_register_irq(enum afe_mem_asrc_id id, irq_handler_t isr, const char *name, void *dev)
{
	int ret;

	if (unlikely(id >= MEM_ASRC_NUM)) {
		pr_err("%s() error: invalid id %u\n", __func__, id);
		return -EINVAL;
	}
	ret = request_irq(asrc_irq[id], isr, IRQF_TRIGGER_LOW, name, dev);
	if (ret)
		pr_err("%s() can't register ISR for mem asrc[%u] (ret=%i)\n", __func__, id, ret);
	return ret;
}

int afe_mem_asrc_unregister_irq(enum afe_mem_asrc_id id, void *dev)
{
	if (unlikely(id >= MEM_ASRC_NUM)) {
		pr_err("%s() error: invalid id %u\n", __func__, id);
		return -EINVAL;
	}
	free_irq(asrc_irq[id], dev);
	return 0;
}

u32 afe_mem_asrc_irq_status(enum afe_mem_asrc_id id)
{
	u32 addr = REG_ASRC_IFR + id * 0x100;

	return afe_read(addr);
}

void afe_mem_asrc_irq_clear(enum afe_mem_asrc_id id, u32 status)
{
	u32 addr = REG_ASRC_IFR + id * 0x100;

	afe_write(addr, status);
}

int afe_mem_asrc_irq_enable(enum afe_mem_asrc_id id, u32 interrupts, int en)
{
	u32 addr = REG_ASRC_IER + id * 0x100;
	u32 val = en ? interrupts : 0;

	afe_msk_write(addr, val, interrupts);
	return 0;
}

int afe_mem_asrc_irq_is_enabled(enum afe_mem_asrc_id id, u32 interrupt)
{
	u32 addr = REG_ASRC_IER + id * 0x100;

	return (afe_read(addr) & interrupt) ? 1 : 0;
}

static enum afe_sampling_rate freq_to_fs(u32 freq)
{
	if (freq < (0x049800 + 0x050000) / 2)
		return FS_7350HZ;	/* 0x049800 */
	else if (freq < (0x050000 + 0x06E400) / 2)
		return FS_8000HZ;	/* 0x050000 */
	else if (freq < (0x06E400 + 0x078000) / 2)
		return FS_11025HZ;	/* 0x06E400 */
	else if (freq < (0x078000 + 0x093000) / 2)
		return FS_12000HZ;	/* 0x078000 */
	else if (freq < (0x093000 + 0x0A0000) / 2)
		return FS_14700HZ;	/* 0x093000 */
	else if (freq < (0x0A0000 + 0x0DC800) / 2)
		return FS_16000HZ;	/* 0x0A0000 */
	else if (freq < (0x0DC800 + 0x0F0000) / 2)
		return FS_22050HZ;	/* 0x0DC800 */
	else if (freq < (0x0F0000 + 0x126000) / 2)
		return FS_24000HZ;	/* 0x0F0000 */
	else if (freq < (0x126000 + 0x140000) / 2)
		return FS_29400HZ;	/* 0x126000 */
	else if (freq < (0x140000 + 0x1B9000) / 2)
		return FS_32000HZ;	/* 0x140000 */
	else if (freq < (0x1B9000 + 0x1E0000) / 2)
		return FS_44100HZ;	/* 0x1B9000 */
	else if (freq < (0x1E0000 + 0x372000) / 2)
		return FS_48000HZ;	/* 0x1E0000 */
	else if (freq < (0x372000 + 0x3C0000) / 2)
		return FS_88200HZ;	/* 0x372000 */
	else if (freq < (0x3C0000 + 0x6E4000) / 2)
		return FS_96000HZ;	/* 0x3C0000 */
	else if (freq < (0x6E4000 + 0x780000) / 2)
		return FS_176400HZ;	/* 0x6E4000 */
	else if (freq < (0x780000 + 0xDC8000) / 2)
		return FS_192000HZ;	/* 0x780000 */
	else if (freq < (0xDC8000 + 0xF00000) / 2)
		return FS_352800HZ;	/* 0xDC8000 */
	else
		return FS_384000HZ;	/* 0xF00000 */
}

int afe_mem_asrc_configurate(enum afe_mem_asrc_id id, const struct afe_mem_asrc_config *config)
{
	u32 addr_offset;
	const u32 *coef;
	size_t coef_count = 0;
	enum afe_sampling_rate input_fs, output_fs;

	if (!config) {
		pr_err("%s() error: invalid config parameter\n", __func__);
		return -EINVAL;
	}
	if (unlikely(id >= MEM_ASRC_NUM)) {
		pr_err("%s() error: invalid asrc[%d]\n", __func__, id);
		return -EINVAL;
	}
	if (config->input_buffer.base & 0xF) {
		pr_err("%s() error: input_buffer.base(0x%08x) is not 16 byte align\n",
		       __func__, config->input_buffer.base);
		return -EINVAL;
	}
	if (config->input_buffer.size & 0xF) {
		pr_err("%s() error: input_buffer.size(0x%08x) is not 16 byte align\n",
		       __func__, config->input_buffer.base);
		return -EINVAL;
	}
	if (config->input_buffer.size < 64 || config->input_buffer.size > 0xffff0) {
		pr_err("%s() error: input_buffer.size(0x%08x) is too small or too large\n",
		       __func__, config->input_buffer.size);
		return -EINVAL;
	}
	if (config->output_buffer.base & 0xF) {
		pr_err("%s() error: output_buffer.base(0x%08x) is not 16 byte align\n",
		       __func__, config->output_buffer.base);
		return -EINVAL;
	}
	if (config->output_buffer.size & 0xF) {
		pr_err("%s() error: output_buffer.size(0x%08x) is not 16 byte align\n",
		       __func__, config->output_buffer.base);
		return -EINVAL;
	}
	if (config->output_buffer.size < 64 || config->output_buffer.size > 0xffff0) {
		pr_err("%s() error: output_buffer.size(0x%08x) is too small or too large\n",
		       __func__, config->output_buffer.size);
		return -EINVAL;
	}
	input_fs = freq_to_fs(config->input_buffer.freq);
	output_fs = freq_to_fs(config->output_buffer.freq);
	pr_debug("%s() config->input_buffer.freq=0x%08x(%u)\n", __func__, config->input_buffer.freq, input_fs);
	pr_debug("%s() config->output_buffer.freq=0x%08x(%u)\n", __func__, config->output_buffer.freq, output_fs);
	addr_offset = id * 0x100;
	/* check whether mem-asrc is running */
	if (afe_read_bits(REG_ASRC_GEN_CONF + addr_offset, POS_ASRC_BUSY, 1) == 1) {
		pr_err("%s() error: asrc[%d] is running\n", __func__, id);
		return -EBUSY;
	}
	/* when there is only 1 block data left in the input buffer, issue interrupt */
	/* times of 512bit. */
	afe_write_bits(REG_ASRC_IBUF_INTR_CNT0 + addr_offset, 0xFF, POS_CH01_IBUF_INTR_CNT, 8);
	/* when there is only 1 block space in the output buffer, issue interrupt */
	/* times of 512bit. 0xFF means if more than 16kB, send interrupt */
	afe_write_bits(REG_ASRC_OBUF_INTR_CNT0 + addr_offset, 0xFF, POS_CH01_OBUF_INTR_CNT, 8);
	/* enable interrupt */
	afe_mem_asrc_irq_enable(id,
		IBUF_EMPTY_INT
#if IBUF_AMOUNT_NOTIFY
		| IBUF_AMOUNT_INT
#endif
		| OBUF_OV_INT
#if OBUF_AMOUNT_NOTIFY
		| OBUF_AMOUNT_INT
#endif
		, 1);
	/* clear all interrupt flag */
	afe_mem_asrc_irq_clear(id, IBUF_EMPTY_INT | OBUF_OV_INT | IBUF_AMOUNT_INT | OBUF_AMOUNT_INT);
	/* iir coeffient setting for down-sample */
	coef = get_iir_coef(input_fs, output_fs, &coef_count);
	if (coef) {
		int i;

		/* turn on IIR coef setting path */
		afe_set_bit(REG_ASRC_GEN_CONF + addr_offset, POS_DSP_CTRL_COEFF_SRAM);
		/* Load Coef */
		afe_write_bits(REG_ASRC_IIR_CRAM_ADDR + addr_offset, 0, POS_ASRC_IIR_CRAM_ADDR, 32);
		for (i = 0; i < coef_count; ++i)
			afe_write(REG_ASRC_IIR_CRAM_DATA + addr_offset, coef[i]);
		afe_write_bits(REG_ASRC_IIR_CRAM_ADDR + addr_offset, 0, POS_ASRC_IIR_CRAM_ADDR, 32);
		/* turn off IIR coe setting path */
		afe_clear_bit(REG_ASRC_GEN_CONF + addr_offset, POS_DSP_CTRL_COEFF_SRAM);
		afe_write_bits(REG_ASRC_CH01_CNFG + addr_offset, 7, POS_IIR_STAGE, 3);	/* set IIR_stage-1 */
		afe_set_bit(REG_ASRC_CH01_CNFG + addr_offset, POS_IIR_EN);
		afe_set_bit(REG_ASRC_GEN_CONF + addr_offset, POS_CH_CLEAR);
		afe_set_bit(REG_ASRC_GEN_CONF + addr_offset, POS_CH_EN);
	} else
		afe_clear_bit(REG_ASRC_CH01_CNFG + addr_offset, POS_IIR_EN);
	pr_debug("%s() config->input_buffer.base=0x%08x\n", __func__, config->input_buffer.base);
	pr_debug("%s() config->input_buffer.size=0x%08x\n", __func__, config->input_buffer.size);
	/* set input buffer's base and size */
	afe_write(REG_ASRC_IBUF_SADR + addr_offset, config->input_buffer.base);
	afe_write_bits(REG_ASRC_IBUF_SIZE + addr_offset, config->input_buffer.size,
		       POS_CH_IBUF_SIZE, 20);
	pr_debug("%s() config->output_buffer.base=0x%08x\n", __func__, config->output_buffer.base);
	pr_debug("%s() config->output_buffer.size=0x%08x\n", __func__, config->output_buffer.size);
	/* set input buffer's rp and wp */
	afe_write(REG_ASRC_CH01_IBUF_RDPNT + addr_offset, config->input_buffer.base);
#if IBUF_AMOUNT_NOTIFY
	afe_write(REG_ASRC_CH01_IBUF_WRPNT + addr_offset, config->input_buffer.base + config->input_buffer.size - 16);
#else
	afe_write(REG_ASRC_CH01_IBUF_WRPNT + addr_offset, config->input_buffer.base);
#endif
	/* set output buffer's base and size */
	afe_write(REG_ASRC_OBUF_SADR + addr_offset, config->output_buffer.base);
	afe_write_bits(REG_ASRC_OBUF_SIZE + addr_offset, config->output_buffer.size,
		       POS_CH_OBUF_SIZE, 20);
	/* set output buffer's rp and wp */
	afe_write(REG_ASRC_CH01_OBUF_WRPNT + addr_offset, config->output_buffer.base);
	afe_write(REG_ASRC_CH01_OBUF_RDPNT + addr_offset,
		  config->output_buffer.base + config->output_buffer.size - 16);
	if (config->input_buffer.bitwidth == 16)
		afe_set_bit(REG_ASRC_CH01_CNFG + addr_offset, POS_IBIT_WIDTH);	/* 16bit */
	else
		afe_clear_bit(REG_ASRC_CH01_CNFG + addr_offset, POS_IBIT_WIDTH);	/* 32bit */
	if (config->output_buffer.bitwidth == 16)
		afe_set_bit(REG_ASRC_CH01_CNFG + addr_offset, POS_OBIT_WIDTH);	/* 16bit */
	else
		afe_clear_bit(REG_ASRC_CH01_CNFG + addr_offset, POS_OBIT_WIDTH);	/* 32bit */
	if (config->stereo)
		afe_clear_bit(REG_ASRC_CH01_CNFG + addr_offset, POS_MONO);
	else
		afe_set_bit(REG_ASRC_CH01_CNFG + addr_offset, POS_MONO);
	afe_write_bits(REG_ASRC_CH01_CNFG + addr_offset, 0x8, POS_CLAC_AMOUNT, 8);
	afe_write_bits(REG_ASRC_MAX_OUT_PER_IN0 + addr_offset, 0, POS_CH01_MAX_OUT_PER_IN0, 4);
	if (config->tracking_mode != MEM_ASRC_NO_TRACKING) {
#if 0				/* internal test only */
		u32 u4I2sTestFs =
			(MEM_ASRC_TRACKING_TX ==
			 config->tracking_mode) ? config->input_buffer.fs : config->output_buffer.fs;
		/* Check Freq Cali status */
		if (afe_read_bits(REG_ASRC_FREQ_CALI_CTRL + addr_offset, POS_FREQ_CALC_BUSY, 1) != 0)
			pr_debug("[vAudMemAsrcConfig] Warning! Freq Calibration is busy!\n");
		if (afe_read_bits(REG_ASRC_FREQ_CALI_CTRL + addr_offset, POS_CALI_EN, 1) != 0) {
			pr_debug
			("[vAudMemAsrcConfig] Warning! Freq Calibration is already enabled!\n");
		}
		pr_debug("[vAudMemAsrcConfig] Tracing mode: %s!\n",
			 (MEM_ASRC_TRACKING_TX ==
			  config->tracking_mode) ? "Tracing Tx" : "Tracing Rx");
		/* select i2sin1 slave lrck */
		afe_write_bits(MASM_TRAC_CON1, 1, (POS_MASRC1_CALC_LRCK_SEL + 3 * id), 3);
		/* freq_mode = (denominator/period_mode)*0x800000 */
		afe_write(REG_ASRC_FREQ_CALI_CYC + addr_offset, 0x3F00);
		afe_write(REG_ASRC_CALI_DENOMINATOR + addr_offset, 0x3c00);
		afe_write(REG_ASRC_FREQ_CALI_CTRL + addr_offset, 0x18D00);
		if (config->tracking_mode == MEM_ASRC_TRACKING_TX) {
			afe_clear_bit(REG_ASRC_FREQ_CALI_CTRL + addr_offset, 9);	/* Tx ->Period Mode Bit9 = 0 */
			afe_write_bits(REG_ASRC_FREQUENCY_0 + addr_offset,
				       g_u4PeriodModeVal_Dm48[config->input_buffer.fs], 0, 24);
			afe_write_bits(REG_ASRC_CH01_CNFG + addr_offset, 2, POS_IFS, 2);
			afe_write_bits(REG_ASRC_CH01_CNFG + addr_offset, 0, POS_OFS, 2);
		} else {
			afe_set_bit(REG_ASRC_FREQ_CALI_CTRL + addr_offset, 9);	/* Rx -> FreqMode   Bit9 = 1 */
			afe_write_bits(REG_ASRC_FREQUENCY_0 + addr_offset,
				       freq_mode_val[config->output_buffer.fs], 0, 24);
			afe_write_bits(REG_ASRC_CH01_CNFG + addr_offset, 2, POS_IFS, 2);
			afe_write_bits(REG_ASRC_CH01_CNFG + addr_offset, 0, POS_OFS, 2);
		}
#else
		pr_err("%s() error: don't support tracking mode\n", __func__);
		return -EINVAL;
#endif
	} else {
		afe_write_bits(REG_ASRC_FREQUENCY_0 + addr_offset, config->input_buffer.freq, 0, 24);
		afe_write_bits(REG_ASRC_FREQUENCY_1 + addr_offset, config->output_buffer.freq, 0, 24);
		afe_write_bits(REG_ASRC_CH01_CNFG + addr_offset, 0, POS_IFS, 2);
		afe_write_bits(REG_ASRC_CH01_CNFG + addr_offset, 1, POS_OFS, 2);
	}
	return 0;
}

#if 0
static void afe_mem_asrc_dump_registers(enum afe_mem_asrc_id id)
{
	u32 addr_offset = id * 0x100;

	pr_debug("PWR2_TOP_CON           [0x%08x]=0x%08x\n"
		 "PWR2_ASM_CON2          [0x%08x]=0x%08x\n"
		 "REG_ASRC_GEN_CONF      [0x%08x]=0x%08x\n"
		 "REG_ASRC_IBUF_INTR_CNT0[0x%08x]=0x%08x\n"
		 "REG_ASRC_OBUF_INTR_CNT0[0x%08x]=0x%08x\n"
		 "REG_ASRC_CH01_CNFG     [0x%08x]=0x%08x\n"
		 "REG_ASRC_IBUF_SADR     [0x%08x]=0x%08x\n"
		 "REG_ASRC_IBUF_SIZE     [0x%08x]=0x%08x\n"
		 "REG_ASRC_OBUF_SADR     [0x%08x]=0x%08x\n"
		 "REG_ASRC_OBUF_SIZE     [0x%08x]=0x%08x\n"
		 "REG_ASRC_FREQUENCY_0   [0x%08x]=0x%08x\n"
		 "REG_ASRC_FREQUENCY_1   [0x%08x]=0x%08x\n"
		 "REG_ASRC_FREQUENCY_2   [0x%08x]=0x%08x\n"
		 "REG_ASRC_FREQUENCY_3   [0x%08x]=0x%08x\n", PWR2_TOP_CON, afe_read(PWR2_TOP_CON)
		 , PWR2_ASM_CON2, afe_read(PWR2_ASM_CON2)
		 , REG_ASRC_GEN_CONF + addr_offset, afe_read(REG_ASRC_GEN_CONF + addr_offset)
		 , REG_ASRC_IBUF_INTR_CNT0 + addr_offset,
		 afe_read(REG_ASRC_IBUF_INTR_CNT0 + addr_offset)
		 , REG_ASRC_OBUF_INTR_CNT0 + addr_offset,
		 afe_read(REG_ASRC_OBUF_INTR_CNT0 + addr_offset)
		 , REG_ASRC_CH01_CNFG + addr_offset, afe_read(REG_ASRC_CH01_CNFG + addr_offset)
		 , REG_ASRC_IBUF_SADR + addr_offset, afe_read(REG_ASRC_IBUF_SADR + addr_offset)
		 , REG_ASRC_IBUF_SIZE + addr_offset, afe_read(REG_ASRC_IBUF_SIZE + addr_offset)
		 , REG_ASRC_OBUF_SADR + addr_offset, afe_read(REG_ASRC_OBUF_SADR + addr_offset)
		 , REG_ASRC_OBUF_SIZE + addr_offset, afe_read(REG_ASRC_OBUF_SIZE + addr_offset)
		 , REG_ASRC_FREQUENCY_0 + addr_offset, afe_read(REG_ASRC_FREQUENCY_0 + addr_offset)
		 , REG_ASRC_FREQUENCY_1 + addr_offset, afe_read(REG_ASRC_FREQUENCY_1 + addr_offset)
		 , REG_ASRC_FREQUENCY_2 + addr_offset, afe_read(REG_ASRC_FREQUENCY_2 + addr_offset)
		 , REG_ASRC_FREQUENCY_3 + addr_offset, afe_read(REG_ASRC_FREQUENCY_3 + addr_offset)
		);
}
#endif

int afe_mem_asrc_enable(enum afe_mem_asrc_id id, int en)
{
	u32 addr;

	if (unlikely(id >= MEM_ASRC_NUM)) {
		pr_err("%s() error: invalid id %u\n", __func__, id);
		return -EINVAL;
	}
	addr = REG_ASRC_GEN_CONF + id * 0x100;
	if (en) {
		afe_set_bit(addr, POS_CH_CLEAR);
		afe_set_bit(addr, POS_CH_EN);
		afe_set_bit(addr, POS_ASRC_EN);
#if 0
		afe_mem_asrc_dump_registers(id);
#endif
	} else {
		afe_clear_bit(addr, POS_CH_EN);
		afe_clear_bit(addr, POS_ASRC_EN);
	}
	return 0;
}

u32 afe_mem_asrc_get_ibuf_rp(enum afe_mem_asrc_id id)
{
	if (unlikely(id >= MEM_ASRC_NUM)) {
		pr_err("%s() error: invalid asrc[%d]\n", __func__, id);
		return 0;
	}
	return afe_read(REG_ASRC_CH01_IBUF_RDPNT + id * 0x100) & (~(u32) 0xF);
}

u32 afe_mem_asrc_get_ibuf_wp(enum afe_mem_asrc_id id)
{
	if (unlikely(id >= MEM_ASRC_NUM)) {
		pr_err("%s() error: invalid asrc[%d]\n", __func__, id);
		return 0;
	}
	return afe_read(REG_ASRC_CH01_IBUF_WRPNT + id * 0x100) & (~(u32) 0xF);
}

int afe_mem_asrc_set_ibuf_wp(enum afe_mem_asrc_id id, u32 p)
{
	u32 addr_offset;
	u32 base;
	u32 size;

	if (unlikely(id >= MEM_ASRC_NUM)) {
		pr_err("%s() error: invalid asrc[%d]\n", __func__, id);
		return -EINVAL;
	}
	addr_offset = id * 0x100;
	base = afe_read(REG_ASRC_IBUF_SADR + addr_offset);
	size = afe_read(REG_ASRC_IBUF_SIZE + addr_offset);
	if (unlikely(p < base || p >= (base + size))) {
		pr_err("%s() error: can't update input buffer's wp:0x%08x (base:0x%08x, size:0x%08x)\n",
			__func__, p, base, size);
		return -EINVAL;
	}
	afe_write(REG_ASRC_CH01_IBUF_WRPNT + addr_offset, p);
	return 0;
}

u32 afe_mem_asrc_get_obuf_rp(enum afe_mem_asrc_id id)
{
	if (unlikely(id >= MEM_ASRC_NUM)) {
		pr_err("%s() error: invalid asrc[%d]\n", __func__, id);
		return 0;
	}
	return afe_read(REG_ASRC_CH01_OBUF_RDPNT + id * 0x100) & (~(u32) 0xF);
}

u32 afe_mem_asrc_get_obuf_wp(enum afe_mem_asrc_id id)
{
	if (unlikely(id >= MEM_ASRC_NUM)) {
		pr_err("%s() error: invalid asrc[%d]\n", __func__, id);
		return 0;
	}
	return afe_read(REG_ASRC_CH01_OBUF_WRPNT + id * 0x100) & (~(u32) 0xF);
}

int afe_mem_asrc_set_obuf_rp(enum afe_mem_asrc_id id, u32 p)
{
	u32 addr_offset;
	u32 base;
	u32 size;

	if (unlikely(id >= MEM_ASRC_NUM)) {
		pr_err("%s() error: invalid asrc[%d]\n", __func__, id);
		return -EINVAL;
	}
	addr_offset = id * 0x100;
	base = afe_read(REG_ASRC_OBUF_SADR + addr_offset);
	size = afe_read(REG_ASRC_OBUF_SIZE + addr_offset);
	if (unlikely(p < base || p >= (base + size))) {
		pr_err("%s() error: can't update output buffer's rp:0x%08x (base:0x%08x, size:0x%08x)\n",
			__func__, p, base, size);
		return -EINVAL;
	}
	afe_write(REG_ASRC_CH01_OBUF_RDPNT + addr_offset, p);
	return 0;
}

int afe_mem_asrc_set_ibuf_freq(enum afe_mem_asrc_id id, u32 freq)
{
	u32 addr = REG_ASRC_FREQUENCY_0 + id * 0x100;

	if (unlikely(id >= MEM_ASRC_NUM)) {
		pr_err("%s() error: invalid asrc[%d]\n", __func__, id);
		return 0;
	}
	afe_write_bits(addr, freq, 0, 24);
	return 0;
}

int afe_mem_asrc_set_obuf_freq(enum afe_mem_asrc_id id, u32 freq)
{
	u32 addr = REG_ASRC_FREQUENCY_1 + id * 0x100;

	if (unlikely(id >= MEM_ASRC_NUM)) {
		pr_err("%s() error: invalid asrc[%d]\n", __func__, id);
		return 0;
	}
	afe_write_bits(addr, freq, 0, 24);
	return 0;
}


/********************* hw gain *********************/

int afe_hwgain_init(enum afe_hwgain_id id)
{
	afe_msk_write(AFE_GAIN1_CON1 + id * 0x18, 0x80000, MASK_GAIN_TARGET);	/* 0db */
	afe_msk_write(AFE_GAIN1_CON0 + id * 0x18, 0x5 << POS_GAIN_MODE, MASK_GAIN_MODE);
	afe_msk_write(AFE_GAIN1_CON0 + id * 0x18, 1 << POS_GAIN_ON, MASK_GAIN_ON);
	return 0;
}
int afe_hwgain_gainmode_set(enum afe_hwgain_id id, enum afe_sampling_rate fs)
{
	afe_msk_write(AFE_GAIN1_CON0 + id * 0x18, fs << POS_GAIN_MODE, MASK_GAIN_MODE);
	return 0;
}

void afe_hwgain_set(enum afe_hwgain_id id, struct afe_hw_gain_config *hgcfg)
{
	u32  hgupstep, hgdownstep;

	switch (hgcfg->hgstepdb) {
	case AFE_STEP_DB_1_8:	/* 0.125db */
		hgupstep = 0x81de6;
		hgdownstep = 0x7e2f3;
		break;
	case AFE_STEP_DB_1_4:	/* 0.25db */
		hgupstep = 0x83bcd;
		hgdownstep = 0x7c5e5;
		break;
	case AFE_STEP_DB_1_2:	/* 0.5db */
		hgupstep = 0x8779a;
		hgdownstep = 0x78bca;
		break;
	default:
		/* Same as AFE_STEP_DB_1_4 */
		hgupstep = 0x83bcd;
		hgdownstep = 0x7c5e5;
		break;
	}
	afe_msk_write(AFE_GAIN1_CON0 + id * 0x18,
		      hgcfg->hwgainsteplen << POS_GAIN_SAMPLE_PER_STEP, MASK_GAIN_SAMPLE_PER_STEP);
	afe_msk_write(AFE_GAIN1_CON1 + id * 0x18, hgcfg->hwtargetgain, MASK_GAIN_TARGET);
	afe_msk_write(AFE_GAIN1_CON2 + id * 0x18, hgdownstep, MASK_DOWN_STEP);
	afe_msk_write(AFE_GAIN1_CON3 + id * 0x18, hgupstep, MASK_UP_STEP);
	pr_debug("%s() AFE_GAIN1_CON0 + %d * 0x18:0x%x\n", __func__, id, afe_read(AFE_GAIN1_CON0 + id * 0x18));
	pr_debug("%s() AFE_GAIN1_CON1 + %d * 0x18:0x%x\n", __func__, id, afe_read(AFE_GAIN1_CON1 + id * 0x18));
	pr_debug("%s() AFE_GAIN1_CON2 + %d * 0x18:0x%x\n", __func__, id, afe_read(AFE_GAIN1_CON2 + id * 0x18));
	pr_debug("%s() AFE_GAIN1_CON3 + %d * 0x18:0x%x\n", __func__, id, afe_read(AFE_GAIN1_CON3 + id * 0x18));
}

/*
*    hgctl0:
*      bit0:hwgainID 0:AFE_HWGAIN_1 ,1:AFE_HWGAIN_2
*      bit1:hwgain: enable/disable
*      bit2~bit9:hwgainsample per step (0~255)
*      bit10~bit11:hwgainsetpdb:0:.125db,1:0.25db,2:0.5db
*
*     hgctl1:
*      hwgain :0x0[-inf.dB]~0x80000[0dB]
*/
int afe_hwgain_configurate(u32 hgctl0, u32 hgctl1)
{
	struct afe_hw_gain_config hwcfg;
	enum afe_hwgain_id id = hgctl0 & 0x1;	/* 0:hwgain1,1:hwgain2 */

	hwcfg.hwgainsteplen = (hgctl0 & 0x3fc) >> 2;	/* bit[9~2] */
	hwcfg.hgstepdb = (hgctl0 & 0xc00) >> 10;	/* bit[11~10] */
	hwcfg.hwtargetgain = hgctl1 & 0xfffff;	/* hwgain */
	if ((id >= AFE_HWGAIN_NUM) || (hwcfg.hgstepdb == 3)) {
		pr_err("%s() please check your hwgainid or stepdb setting\n", __func__);
		return -EINVAL;
	}
	if (hwcfg.hwtargetgain > 0x80000)
		hwcfg.hwtargetgain = 0x80000;
	afe_hwgain_set(id, &hwcfg);
	return 0;
}

int afe_hwgain_enable(enum afe_hwgain_id id, int en)
{
	en = !!en;
	afe_msk_write(AFE_GAIN1_CON0 + id * 0x18, (en << POS_GAIN_ON), MASK_GAIN_ON);
	return 0;
}

void afe_loopback_set(int en)
{
	en = !!en;

	/* For those driver which handle loopback setting by itself. Eg: I2S */
	afe_loopback = en;
}

/**************** ultra low-power ****************/

int lp_configurate(volatile struct lp_info *lp, u32 base, u32 size,
		   unsigned int rate, unsigned int channels, unsigned int bitwidth)
{
	memset((void *)lp, 0x00, sizeof(struct lp_info));
	lp->m.mode = LP_MODE_NORMAL;
	lp->buf.base = lp->buf.hw_pointer = base;
	lp->buf.size = size;
	lp->rate = rate;
	lp->channels = channels;
	lp->bitwidth = bitwidth;
	return 0;
}

u32 lp_hw_offset(volatile struct lp_info *lp)
{
	return lp->buf.hw_pointer - lp->buf.base;
}

int lp_cmd_excute(volatile struct lp_info *lp, unsigned int cmd)
{
	/* lp hardware is busy */ /* todo: add timeout */
	while (lp->cmd != LP_CMD_NONE || !is_cm4_soft0_irq_cleared())
		;
	/* send interrupt to cm4 */
	lp->cmd = cmd;
	trigger_cm4_soft0_irq();
	/* wait cmd processing done */
	while (lp->cmd != LP_CMD_NONE)
		;
	return 0;
}


/**************** spdif receiver ****************/

#define ISPDIF_FS_SUPPORT_RANGE 9

static volatile struct afe_dir_info spdifrx_state;
static bool spdifrx_inited;

static u32 spdifrx_fscnt[16][9] = {
	/*32k       44.1k        48k             64k             88.2k      96k          128k       176k         192k*/
	{6750, 4898, 4500, 3375, 2455, 2250, 1688, 1227, 1125 }, /* 1 subframe*/
	{13500, 9796, 9000, 6750, 4909, 4500, 3375, 2455, 2250 }, /* 2 subframe*/
	{27000, 19592, 18000, 13500, 9818, 9000, 6750, 4909, 4500 }, /* 4 subframe*/
	{54000, 39184, 36000, 27000, 19636, 18000, 13500, 9818, 9000 }, /* 8 subframe*/
	{108000, 78367, 72000, 54000, 39273, 36000, 27000, 19636, 18000 }, /* 16 subframe*/
	{216000, 156735, 144000, 108000, 78546, 72000, 54000, 39273, 36000 }, /* 32 subframe*/
	{432000, 313469, 288000, 216000, 157091, 144000, 108000, 78546, 72000 }, /* 64 subframe*/
	{864000, 626939, 576000, 432000, 314182, 288000, 216000, 157091, 144000 }, /* 128 subframe*/
	{1728027, 1253897, 1152018, 864014, 626949, 576008, 432000, 313469, 288000 }, /*256 subframe*/
	{3456000, 2507755, 2304000, 1728000, 1256727, 1152000, 864000, 628364, 576000 }, /* 512 subframe*/
	{6912000, 5015510, 4608000, 3456000, 2513455, 2304000, 1728000, 1256727, 1152000 }, /* 1024 subframe*/
	{13824000, 10031020, 9216000, 6912000, 5026909, 4608000, 3456000, 2513455, 2304000 }, /* 2048 subframe*/
	 /* 4096 subframe*/
	{27648000, 20062041, 18432000, 13824000, 10053818, 9216000, 6912000, 5026909, 4608000 },
	/* 8192 subframe*/
	{55296000, 40124082, 36864000, 27648000, 20107636, 18432000, 13824000, 10053818, 9216000 },
	/* 16384 subframe*/
	{110592000, 80248163, 73728000, 55296000, 40215272, 36864000, 27648000, 20107636, 18432000},
	/* 32768 subframe*/
	{221184000, 160496327, 147456000, 110592000, 80430546, 73728000, 55296000, 40215273, 36864000}
};

static u32 spdifrx_fsoft[16][9]  = {
	/*32k       44.1k        48k            64k         88.2k       96k         128k        176k        192k*/
	{78, 78, 78, 78, 78, 78, 78, 78, 78 }, /* 1 subframe*/
	{156, 156, 156, 156, 156, 156, 156, 156, 156 }, /* 2 subframe*/
	{312, 312, 312, 312, 312, 312, 312, 312, 312 }, /* 4 subframe*/
	{625, 625, 625, 625, 625, 625, 625, 625, 625 }, /* 8 subframe*/
	{1250, 1250, 1250, 1250, 1250, 1250, 1250, 1250, 1250 }, /* 16 subframe*/
	{2500, 2500, 2500, 2500, 2500, 2500, 2500, 2500, 2500 }, /*32 subframe*/
	{5000, 5000, 5000, 5000, 5000, 5000, 5000, 5000, 5000 }, /* 64 subframe*/
	{10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000 }, /* 128 subframe*/
	{200000, 45000, 45000, 27000, 20000, 18000, 14000, 10000, 9000 }, /* 256 subframe*/
	{60000, 45000, 45000, 20000, 20000, 20000, 20000, 20000, 20000 }, /* 512 subframe*/
	{80000, 80000, 80000, 80000, 80000, 80000, 80000, 80000, 80000 }, /* 1024 subframe*/
	{160000, 160000, 160000, 160000, 160000, 160000, 160000, 160000, 160000 }, /* 2048 subframe*/
	{320000, 320000, 320000, 320000, 320000, 320000, 320000, 320000, 320000 }, /* 4096 subframe*/
	{640000, 640000, 640000, 640000, 640000, 640000, 640000, 640000, 640000 }, /* 8192 subframe*/
	{1280000, 1280000, 1280000, 1280000, 1280000, 1280000, 1280000, 1280000, 1280000 }, /* 16384 subframe*/
	{2560000, 2560000, 2560000, 2560000, 2560000, 2560000, 2560000, 2560000, 2560000 }  /* 32768 subframe*/
};

u32 _u4LRCKCmp432M[9] = {
    /*32k   44.1k    48k     64k   88.2k    96k      128k   176.4k    192k*/
    /*203,   147,   135,   102,   73,      68,    51,      37,         34   432M*3%*fs/2*/
176,   98,   90,   68,   49,      46,    34,      20,         23  /* 432M*3%*fs/3 ,32k : 136+40 176.4k(25-5)*/
    /*102,   74,   68,   51,   37,      34,    26,      18,         17  // 432M*3%*fs/4 */
};

u32 _u4LRCKCmp594M[9] = {
    /*32k   44.1k    48k     64k   88.2k    96k      128k   176.4k    192k */
279,    203,    186,     140,   102,    93,      70,   51,       47
};

const volatile struct afe_dir_info *afe_spdifrx_state(void)
{
	return &spdifrx_state;
}

static void spdifrx_select_port(enum afe_spdifrx_port port)
{
	if (port == SPDIFRX_PORT_OPT)
		afe_write(AFE_SPDIFIN_INT_EXT,
			(afe_read(AFE_SPDIFIN_INT_EXT) & (~MULTI_INPUT_SEL_MASK)) | MULTI_INPUT_SEL_ARC);
	else
		afe_write(AFE_SPDIFIN_INT_EXT,
			(afe_read(AFE_SPDIFIN_INT_EXT) & (~MULTI_INPUT_SEL_MASK)) | MULTI_INPUT_SEL_OPT);
	afe_write(AFE_SPDIFIN_CFG1,
			afe_read(AFE_SPDIFIN_CFG1) & (AFE_SPDIFIN_REAL_OPTICAL) & (AFE_SPDIFIN_SWITCH_REAL_OPTICAL));
	afe_write(AFE_SPDIFIN_CFG1, afe_read(AFE_SPDIFIN_CFG1) | AFE_SPDIFIN_SEL_SPDIFIN_EN |
						AFE_SPDIFIN_SEL_SPDIFIN_CLK_EN | AFE_SPDIFIN_FIFOSTARTPOINT_5);
}

static void spdifrx_clear_vucp(void)
{
	memset((void *)spdifrx_state.c_bit, 0xff, sizeof(spdifrx_state.c_bit));
	memset((void *)spdifrx_state.u_bit, 0xff, sizeof(spdifrx_state.u_bit));
}

static u32 spdifrx_fs_interpreter(u32 fsval)
{
	u8 period, cnt;
	u32 fs = SPDIFIN_OUT_RANGE;
	u32 rangeplus, rangeminus;

	period = (afe_read(AFE_SPDIFIN_BR)&AFE_SPDIFIN_BR_SUBFRAME_MASK) >> 8;
	for (cnt = 0; cnt < ISPDIF_FS_SUPPORT_RANGE; cnt++) {
		rangeplus = (spdifrx_fscnt[period][cnt] + spdifrx_fsoft[period][cnt]);
		rangeminus = (spdifrx_fscnt[period][cnt] - spdifrx_fsoft[period][cnt]);
		rangeplus = (rangeplus * 624) / 432;
		rangeminus = (rangeminus * 624) / 432;
		if ((fsval > rangeminus) && (fsval < rangeplus)) {
			fs = cnt + SPDIFIN_32K; /*from 32k~192k*/
			break;
		}
	}
	if (cnt > ISPDIF_FS_SUPPORT_RANGE) {
		fs = SPDIFIN_OUT_RANGE;
		pr_err("%s()FS Out of Detected Range!\n", __func__);
	}
	return fs;
}

static void (*spdifrx_callback)(void);

/*
 * [Programming Guide]
 * [SPDIF IN] spdif in IRQ7 callback
 */
void afe_spdifrx_isr(void)
{
	u32 regval1, regval2, regval3, regval4, fsval, fsvalod, chsintflag;
	int i, j;
	bool err;

	regval1 = afe_read(AFE_SPDIFIN_DEBUG3);
	regval2 = afe_read(AFE_SPDIFIN_INT_EXT2);
	regval3 = afe_read(AFE_SPDIFIN_DEBUG1);
	regval4 = afe_read(AFE_SPDIFIN_CFG1);
	chsintflag = afe_read(AFE_SPDIFIN_DEBUG2);
	err = (regval1 & SPDIFIN_ALL_ERR_ERR_STS) ||
		(regval2 & SPDIFIN_LRCK_CHG_INT_STS) || (regval3 & SPDIFIN_DATA_LATCH_ERR);
	if (err) {
		if (spdifrx_state.rate > 0) {
			pr_debug("%s Spdif Rx unlock!\n", __func__);
			if (regval1 & SPDIFIN_ALL_ERR_ERR_STS)
				pr_debug("%s Error is 0x%x\n", __func__, regval1 & SPDIFIN_ALL_ERR_ERR_STS);
			if (regval2 & SPDIFIN_LRCK_CHG_INT_STS)
				pr_debug("%s LRCK Change\n", __func__);
			if (regval3 & SPDIFIN_DATA_LATCH_ERR)
				pr_debug("%s Data Latch error!\n", __func__);
			spdifrx_state.rate = 0;
			spdifrx_clear_vucp();
			if (spdifrx_callback)
				spdifrx_callback();
		}
		/*Disable SpdifRx interrupt disable*/
		afe_msk_write(AFE_SPDIFIN_CFG0, SPDIFIN_INT_DIS | SPDIFIN_DIS, SPDIFIN_INT_EN_MASK | SPDIFIN_EN_MASK);
		/*Clear interrupt bits*/
		afe_msk_write(AFE_SPDIFIN_EC, SPDIFIN_INT_CLEAR_ALL, SPDIFIN_INT_ERR_CLEAR_MASK);
		/*Disable TimeOut Interrupt*/
		afe_msk_write(AFE_SPDIFIN_CFG1, 0, SPDIFIN_TIMEOUT_INT_EN);
	} else {
		/*Enable Timeout Interrupt*/
		afe_msk_write(AFE_SPDIFIN_CFG1, SPDIFIN_TIMEOUT_INT_EN, SPDIFIN_TIMEOUT_INT_EN);
		fsval = afe_read(AFE_SPDIFIN_BR_DBG1);
		fsval = spdifrx_fs_interpreter(fsval);
		if (fsval != SPDIFIN_OUT_RANGE) {
			afe_write(AFE_SPDIFIN_INT_EXT2, (afe_read(AFE_SPDIFIN_INT_EXT2) &
				 (~SPDIFIN_LRCK_CHG_INT_EN) & (~SPDIFIN_LRC_MASK)) |
				  SPDIFIN_LRCK_CHG_INT_EN | _u4LRCKCmp594M[fsval-SPDIFIN_32K]);
			fsvalod = spdifrx_state.rate;
			spdifrx_state.rate = fsval;
			pr_debug("%s spdifrx_state.rate =0x%x.\n", __func__, spdifrx_state.rate);
			if ((spdifrx_callback) && (fsvalod != fsval))
				spdifrx_callback();

		}
		if (((chsintflag & SPDIFIN_CHANNEL_STATUS_INT_FLAG) != 0) &&
			((regval4 & SPDIFIN_CHSTS_COLLECTION_EN) != 0) && (fsval != SPDIFIN_OUT_RANGE)) {
			for (i = 0; i < 6; i++) {
				u32 temp = afe_read(AFE_SPDIFIN_CHSTS1 + i * 0x4);

				if (temp != spdifrx_state.c_bit[i]) {
					spdifrx_state.c_bit[i] =  temp;
					if (spdifrx_callback)
						spdifrx_callback();
				}
			}
			for (i = 0; i < 2; i++) {
				for (j = 0; j < 6; j++) {
					u32 temp  = afe_read(AFE_SPDIFIN_USERCODE_1 + (i * 6 + j) * 0x4);

					if (temp != spdifrx_state.u_bit[i][j]) {
						spdifrx_state.u_bit[i][j] = temp;
					if (spdifrx_callback)
						spdifrx_callback();
					}
				}
			}
		}
		/*Clear interrupt bits*/
		afe_msk_write(AFE_SPDIFIN_EC, SPDIFIN_INT_CLEAR_ALL, SPDIFIN_INT_ERR_CLEAR_MASK);
	}
	if (err) {
		/*Enable SpdifRx interrupt disable*/
		afe_msk_write(AFE_SPDIFIN_CFG0, SPDIFIN_INT_EN | SPDIFIN_EN, SPDIFIN_INT_EN_MASK | SPDIFIN_EN_MASK);
	}
}

static void spdifrx_irq_enable(int en)
{
	if (en) {
		afe_msk_write(AFE_SPDIFIN_CFG1,
			      SPDIFIN_ALL_ERR_INT_EN | SEL_BCK_SPDIFIN |
			      AFE_SPDIFIN_SEL_SPDIFIN_EN | AFE_SPDIFIN_SEL_SPDIFIN_CLK_EN,
			      SPDIFIN_INT_ERR_EN_MASK | SEL_BCK_SPDIFIN |
			      AFE_SPDIFIN_SEL_SPDIFIN_EN | AFE_SPDIFIN_SEL_SPDIFIN_CLK_EN);
		afe_msk_write(AFE_SPDIFIN_INT_EXT, SPDIFIN_DATALATCH_ERR_EN, SPDIFIN_DATALATCH_ERR_EN_MASK);
		afe_msk_write(AFE_SPDIFIN_INT_EXT2, SPDIFIN_LRCK_CHG_INT_EN, SPDIFIN_LRCK_CHG_INT_MASK);
		afe_msk_write(AFE_SPDIFIN_CFG0,
			      SPDIFIN_EN | SPDIFIN_INT_EN | SPDIFIN_FLIP_EN | 4 << 8 |
			      SPDIFIN_DE_SEL_DECNT | 0xED << 16,
			      SPDIFIN_EN_MASK | SPDIFIN_INT_EN_MASK | SPDIFIN_FLIP_EN_MASK |
			      SPDIFIN_DE_CNT_MASK | SPDIFIN_DE_SEL_MASK | MAX_LEN_NUM_MASK);
	} else {
		afe_msk_write(AFE_SPDIFIN_CFG0, SPDIFIN_DIS | SPDIFRX_INT_DIS |
			      AFE_SPDIFIN_SEL_SPDIFIN_DIS | SPDIFIN_FLIP_DIS,
			      SPDIFIN_EN | SPDIFRX_INT_EN | AFE_SPDIFIN_SEL_SPDIFIN_EN | SPDIFIN_FLIP_EN);
		afe_msk_write(AFE_SPDIFIN_CFG1,
			      SPDIFIN_ALL_ERR_INT_DIS | ~SEL_BCK_SPDIFIN |
			      AFE_SPDIFIN_SEL_SPDIFIN_DIS | AFE_SPDIFIN_SEL_SPDIFIN_CLK_DIS,
			      SPDIFIN_INT_ERR_EN_MASK | SEL_BCK_SPDIFIN |
			      AFE_SPDIFIN_SEL_SPDIFIN_EN | AFE_SPDIFIN_SEL_SPDIFIN_CLK_EN);
		afe_msk_write(AFE_SPDIFIN_INT_EXT, SPDIFIN_DATALATCH_ERR_DIS, SPDIFIN_DATALATCH_ERR_EN_MASK);
		afe_msk_write(AFE_SPDIFIN_INT_EXT2, SPDIFIN_LRCK_CHG_INT_DIS, SPDIFIN_LRCK_CHG_INT_MASK);
	}
}

static void spdifrx_init(enum afe_spdifrx_port port)
{
	if (spdifrx_inited) {
		pr_err("%s() Dir has already inited.\n", __func__);
		return;
	}
	spdifrx_clear_vucp();
	spdifrx_state.rate = 0;
	/*Set spdifin clk cfg*/
	#ifdef AUDIO_MEM_IOREMAP
	/*
	 * [Programming Guide]
	 * [SPDIF IN]Spdif in clock mux setting
	 */
	mt_afe_spdif_dir_clk_on();
	#else
	afe_msk_write(CLK_INTDIR_SEL, UNIVPLL_D2, CLK_INTDIR_SEL_MASK | DIR_PD);/*624M*/
	#endif
	/*
	 * [Programming Guide]
	 * [SPDIF IN] spdifin config
	 */
	afe_write(AFE_SPDIFIN_FREQ_INF, 0x877986);
	afe_write(AFE_SPDIFIN_FREQ_INF_2, 0x6596ED);
	afe_write(AFE_SPDIFIN_FREQ_INF_3, 0x5A4);
	/*Bitclk recovery enable and lowbound*/
	afe_msk_write(AFE_SPDIFIN_BR,
		      1 | AFE_SPDIFIN_BR_FS_256 | AFE_SPDIFIN_BR_SUBFRAME_256 | AFE_SPDIFIN_BR_TUNE_MODE1 | 0x19 << 12,
		      AFE_SPDIFIN_BRE_MASK | AFE_SPDIFIN_BR_FS_MASK | AFE_SPDIFIN_BR_SUBFRAME_MASK |
		      AFE_SPDIFIN_BR_TUNE_MODE_MASK | AFE_SPDIFIN_BR_LOWBOUND_MASK);
	afe_write(AFE_SPDIFIN_INT_EXT2, (afe_read(AFE_SPDIFIN_INT_EXT2)&(~SPDIFIN_LRCK_CHG_INT_EN)&(~SPDIFIN_LRC_MASK))|
		  SPDIFIN_LRCK_CHG_INT_EN | SPDIFIN_LRC_COMPARE_594M);
	afe_write(AFE_SPDIFIN_INT_EXT2, (afe_read(AFE_SPDIFIN_INT_EXT2)&(~SPDIFIN_MODE_CLK_MASK))|SPDIFIN_594MODE_EN);
	afe_power_on_intdir(1);
	spdifrx_select_port(port);
	spdifrx_irq_enable(1);
	spdifrx_inited = 1;
}

static void spdifrx_uninit(void)
{
	if (!spdifrx_inited) {
		pr_err("%s() Dir has already uninited.\n", __func__);
		return;
	}
	spdifrx_irq_enable(0);
	afe_power_on_intdir(0);
	mt_afe_spdif_dir_clk_off();
	spdifrx_inited = 0;
}

void afe_spdifrx_start(enum afe_spdifrx_port port, void (*callback)(void))
{
	/*
	 * [Programming Guide]
	 * [SPDIF IN]GPIO mode setting
	 */
	switch (port) {
	case SPDIFRX_PORT_OPT:
		#ifdef AUD_PINCTRL_SUPPORTING
		mt8521p_gpio_spdif_in1_select(SPDIF_MODE1_SPDIF);
		#else
		mt_set_gpio_mode(GPIO201, GPIO_MODE_01); /* spdif in1*/
		#endif
		break;
	case SPDIFRX_PORT_ARC:
		#ifdef AUD_PINCTRL_SUPPORTING
		mt8521p_gpio_spdif_in0_select(SPDIF_MODE1_SPDIF);
		#else
		mt_set_gpio_mode(GPIO202, GPIO_MODE_01); /* spdif in0*/
		#endif
		break;
	default:
		pr_err("%s() invalid port: %d\n", __func__, port);
		return;
	}
	spdifrx_callback = callback;
	spdifrx_init(port);
}

void afe_spdifrx_stop(void)
{
	spdifrx_uninit();
	spdifrx_callback = NULL;
}


/**************** iec1 & iec2 ****************/

static unsigned int get_apll_div(unsigned int div)
{
	switch (div) {
	case 0:
		return 0;
	case 1: default:
		return 1;
	case 4:
		return 2;
	case 8:
		return 3;
	case 16:
		return 4;
	case 24:
		return 5;
	}
}

static void afe_f_apll_ck_on(unsigned int mclk)
{
	unsigned int domain, div;

	if (mclk <= 0) {
		pr_err("%s() error: bad mclk %d\n", __func__, mclk);
		return;
	}
	if ((98304000 % mclk) == 0) {
		domain = 0;
		div = 98304000 / mclk;
	} else if ((90316800 % mclk) == 0) {
		domain = 1;
		div = 90316800 / mclk;
	} else {
		domain = 0;
		div = 98304000 / mclk;
		pr_err("%s() error: bad mclk %d\n", __func__, mclk);
	}

	div = get_apll_div(div);
#ifdef CONFIG_MTK_LEGACY_CLOCK
	phy_msk_write(CLK_CFG_5, div << 16, APLL_DIV_CLK_MASK);
#else
	mt_afe_f_apll_ck_on(domain, div);
#endif
}

static void afe_f_apll_ck_off(void)
{
#ifndef CONFIG_MTK_LEGACY_CLOCK
	mt_afe_f_apll_ck_off();
#endif
}

void afe_iec_clock_on(enum mt_stream_id id, unsigned int mclk)
{
	afe_f_apll_ck_on(mclk);

	if (id == MT_STREAM_IEC1) {
		afe_msk_write(AUDIO_TOP_CON0, 0x0 << 23, HDMISPDIF_POWER_UP_MASK);
		afe_msk_write(AUDIO_TOP_CON2, (1 - 1) << 0, SPDIF_CK_DIV_MASK);
		afe_msk_write(AUDIO_TOP_CON0, 0x0 << 21, SPDIF1_POWER_UP_MASK);
		afe_msk_write(AFE_SPDIF_OUT_CON0,
			SPDIF_OUT_CLOCK_ON	    | SPDIF_OUT_MEMIF_ON,
			SPDIF_OUT_CLOCK_ON_OFF_MASK | SPDIF_OUT_MEMIF_ON_OFF_MASK);
	} else if (id == MT_STREAM_IEC2) {
		afe_msk_write(AUDIO_TOP_CON0, 0x0 << 23, HDMISPDIF_POWER_UP_MASK);
		afe_msk_write(AUDIO_TOP_CON2, (1 - 1) << 16, SPDIF2_CK_DIV_MASK);
		afe_msk_write(AUDIO_TOP_CON0, 0x0 << 22, SPDIF2_POWER_UP_MASK);
		afe_msk_write(AFE_SPDIF2_OUT_CON0,
			SPDIF2_OUT_CLOCK_ON          | SPDIF2_OUT_MEMIF_ON,
			SPDIF2_OUT_CLOCK_ON_OFF_MASK | SPDIF2_OUT_MEMIF_ON_OFF_MASK);
	}
}

void afe_iec_clock_off(enum mt_stream_id id)
{
	if (id == MT_STREAM_IEC1) {
		afe_msk_write(AFE_SPDIF_OUT_CON0,
			SPDIF_OUT_CLOCK_OFF | SPDIF_OUT_MEMIF_OFF,
			SPDIF_OUT_CLOCK_ON_OFF_MASK | SPDIF_OUT_MEMIF_ON_OFF_MASK);
		afe_msk_write(AUDIO_TOP_CON0, 0x1 << 21, SPDIF1_POWER_UP_MASK);
		afe_msk_write(AUDIO_TOP_CON0, 0x1 << 23, HDMISPDIF_POWER_UP_MASK);
	} else if (id == MT_STREAM_IEC2) {
		afe_msk_write(AFE_SPDIF2_OUT_CON0,
			SPDIF2_OUT_CLOCK_OFF | SPDIF2_OUT_MEMIF_OFF,
			SPDIF2_OUT_CLOCK_ON_OFF_MASK | SPDIF2_OUT_MEMIF_ON_OFF_MASK);
		afe_msk_write(AUDIO_TOP_CON0, 0x1 << 22, SPDIF2_POWER_UP_MASK);
		afe_msk_write(AUDIO_TOP_CON0, 0x1 << 23, HDMISPDIF_POWER_UP_MASK);
	}

	afe_f_apll_ck_off();
}

void afe_iec_configurate(enum mt_stream_id id, int bitstream, struct snd_pcm_runtime *runtime)
{
	/*
	 * samplerate - index
	 *  32000     -  4
	 *  44100     - 21
	 *  48000     -  5
	 *  88200     - 22
	 *  96000     -  6
	 * 176400     - 23
	 * 192000     -  7
	 * 768000     - 25
	 */
	static const u32 default_channel_status0[26] = {
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x03001900, 0x02001900, 0x0A001900,
		0x0E001900, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00001900, 0x08001900, 0x0C001900, 0x00000000, 0x09001900
	};
	static const u32 iec_force_updata_size[26] = {
		0,  0, 0, 0, 2, 3, 5,
		0xa, 0, 0, 0, 0, 0, 0,
		0,  0, 0, 0, 0, 0, 0,
		3,  5, 9, 0, 0
	};
	u32 burst_length = frames_to_bytes(runtime, runtime->period_size) * 8;
	enum afe_sampling_rate fs = fs_enum(runtime->rate);
	u32 stat0 = default_channel_status0[fs] | ((!!bitstream) << 1);
	u32 word_length = bitstream ? 0 : ((runtime->sample_bits <= 16) ? 0x2 /*16bit*/ : 0xB /*24bit*/);
	u32 force_update_size = iec_force_updata_size[fs];
	u32 base = runtime->dma_addr, end = runtime->dma_addr + runtime->dma_bytes;

	afe_msk_write(AFE_MEMIF_PBUF_SIZE, AFE_MEMIF_IEC_PBUF_SIZE_128BYTES, AFE_MEMIF_IEC_PBUF_SIZE_MASK);

	if (id == MT_STREAM_IEC1) {
		afe_write(AFE_SPDIF_BASE, base);
		afe_write(AFE_SPDIF_END, end);
		afe_msk_write(AFE_IEC_NSNUM,
			((runtime->period_size / 2) << 16) | runtime->period_size,
			IEC_NEXT_SAM_NUM_MASK		   | IEC_INT_SAM_NUM_MASK);
		afe_msk_write(AFE_IEC_BURST_LEN, burst_length, IEC_BURST_LEN_MASK);
		afe_msk_write(AFE_IEC_BURST_INFO, 0x0000, IEC_BURST_INFO_MASK);
		if (iec1_chst_flag == 0) {
			/* chst bit31 ~ bit0 */
			afe_write(AFE_IEC_CHL_STAT0, stat0);
			afe_write(AFE_IEC_CHR_STAT0, stat0);
			/* chst bit35 ~ bit32 */
			/* chst bit47 ~ bit36 */
			afe_msk_write(AFE_IEC_CHL_STAT1,
				(0x000 << 4) | (word_length << 0),
				(0xfff << 4) | (0xf	    << 0));
			afe_msk_write(AFE_IEC_CHR_STAT1,
				(0x000 << 4) | (word_length << 0),
				(0xfff << 4) | (0xf	    << 0));
		}
		afe_msk_write(AFE_IEC_CFG,
			IEC_RAW_24BIT_SWITCH_MODE_OFF | IEC_RAW_24BIT_MODE_OFF |
			IEC_RAW_SEL_RAW | IEC_PCM_SEL_PCM | (force_update_size << 24) |
			IEC_FORCE_UPDATE | IEC_MUTE_DATA_NORMAL | IEC_VALID_DATA | IEC_SW_NORST,
			IEC_RAW_24BIT_MODE_MASK | IEC_RAW_24BIT_SWITCH_MODE_MASK |
			IEC_RAW_SEL_MASK | IEC_PCM_SEL_MASK | IEC_FORCE_UPDATE_SIZE_MASK |
			IEC_FORCE_UPDATE_MASK | IEC_MUTE_DATA_MASK | IEC_VALID_DATA_MASK | IEC_SW_RST_MASK);
		if (runtime->sample_bits == 24) {
			afe_msk_write(AFE_IEC_CFG,
				IEC_RAW_24BIT_MODE_ON | IEC_RAW_24BIT_SWITCH_MODE_ON,
				IEC_RAW_24BIT_MODE_MASK | IEC_RAW_24BIT_SWITCH_MODE_MASK);
		}
	} else if (id == MT_STREAM_IEC2) {
		afe_write(AFE_SPDIF2_BASE, base);
		afe_write(AFE_SPDIF2_END, end);
		afe_msk_write(AFE_IEC2_NSNUM,
			((runtime->period_size / 2) << 16) | runtime->period_size,
			IEC2_NEXT_SAM_NUM_MASK		   | IEC2_INT_SAM_NUM_MASK);
		afe_msk_write(AFE_IEC2_BURST_LEN, burst_length, IEC2_BURST_LEN_MASK);
		afe_msk_write(AFE_IEC2_BURST_INFO, 0x0000, IEC2_BURST_INFO_MASK);
		if (iec2_chst_flag == 0) {
			/* chst bit31 ~ bit0 */
			afe_write(AFE_IEC2_CHL_STAT0, stat0);
			afe_write(AFE_IEC2_CHR_STAT0, stat0);
			/* chst bit35 ~ bit32 */
			/* chst bit47 ~ bit36 */
			afe_msk_write(AFE_IEC2_CHL_STAT1,
				(0x000 << 4) | (word_length << 0),
				(0xfff << 4) | (0xf	    << 0));
			afe_msk_write(AFE_IEC2_CHR_STAT1,
				(0x000 << 4) | (word_length << 0),
				(0xfff << 4) | (0xf	    << 0));
		}
		afe_msk_write(AFE_IEC2_CFG,
			IEC2_RAW_24BIT_SWITCH_MODE_OFF | IEC2_RAW_24BIT_MODE_OFF |
			(force_update_size << 24) | IEC2_RAW_SEL_RAW | IEC2_REG_LOCK_EN_MASK |
			IEC2_FORCE_UPDATE | IEC2_MUTE_DATA_NORMAL | IEC2_VALID_DATA | IEC2_SW_NORST,
			IEC2_RAW_24BIT_SWITCH_MODE_MASK | IEC2_RAW_24BIT_MODE_MASK |
			IEC2_FORCE_UPDATE_SIZE_MASK | IEC2_RAW_SEL_MASK | IEC2_REG_LOCK_EN_MASK |
			IEC2_FORCE_UPDATE_MASK | IEC2_MUTE_DATA_MASK | IEC2_VALID_DATA_MASK | IEC2_SW_RST_MASK);
		if (runtime->sample_bits == 24) {
			afe_msk_write(AFE_IEC2_CFG,
				IEC2_RAW_24BIT_MODE_ON | IEC2_RAW_24BIT_SWITCH_MODE_ON,
				IEC2_RAW_24BIT_MODE_MASK | IEC2_RAW_24BIT_SWITCH_MODE_MASK);
		}
	}
	afe_iec_set_nsadr(id, base);
	afe_iec_burst_info_clear_ready(id);
	udelay(2000);	/* delay for prefetch */
}

void afe_iec_enable(enum mt_stream_id id)
{
	if (id == MT_STREAM_IEC1)
		afe_msk_write(AFE_IEC_CFG, IEC_ENABLE, IEC_EN_MASK);
	else if (id == MT_STREAM_IEC2)
		afe_msk_write(AFE_IEC2_CFG, IEC2_ENABLE, IEC2_EN_MASK);
}

void afe_iec_disable(enum mt_stream_id id)
{
	if (id == MT_STREAM_IEC1) {
		afe_msk_write(AFE_IEC_CFG,
			IEC_MUTE_DATA_MUTE | IEC_SW_RST | IEC_DISABLE,
			IEC_MUTE_DATA_MASK | IEC_SW_RST_MASK | IEC_EN_MASK);
	} else if (id == MT_STREAM_IEC2) {
		afe_msk_write(AFE_IEC2_CFG,
			IEC2_MUTE_DATA_MUTE | IEC2_SW_RST | IEC2_DISABLE,
			IEC2_MUTE_DATA_MASK | IEC2_SW_RST_MASK | IEC2_EN_MASK);
	}
}

int afe_iec_burst_info_is_ready(enum mt_stream_id id)
{
	if (id == MT_STREAM_IEC1)
		return (afe_read(AFE_IEC_BURST_INFO) & IEC_BURST_NOT_READY_MASK) == IEC_BURST_READY;
	else if (id == MT_STREAM_IEC2)
		return (afe_read(AFE_IEC2_BURST_INFO) & IEC2_BURST_NOT_READY_MASK) == IEC2_BURST_READY;
	else
		return 0;
}

void afe_iec_burst_info_clear_ready(enum mt_stream_id id)
{
	if (id == MT_STREAM_IEC1)
		afe_msk_write(AFE_IEC_BURST_INFO, IEC_BURST_NOT_READY, IEC_BURST_NOT_READY_MASK);
	else if (id == MT_STREAM_IEC2)
		afe_msk_write(AFE_IEC2_BURST_INFO, IEC2_BURST_NOT_READY, IEC2_BURST_NOT_READY_MASK);
}

u32 afe_iec_burst_len(enum mt_stream_id id)
{
	if (id == MT_STREAM_IEC1)
		return (afe_read(AFE_IEC_BURST_LEN) & 0x0007ffff) >> 3;
	else if (id == MT_STREAM_IEC2)
		return (afe_read(AFE_IEC2_BURST_LEN) & 0x0007ffff) >> 3;
	else
		return 0;
}

u32 afe_spdif_cur(enum mt_stream_id id)
{
	if (id == MT_STREAM_IEC1) {
		return ((afe_read(AFE_IEC_CFG) & IEC_EN_MASK) == IEC_ENABLE)
			? afe_read(AFE_SPDIF_CUR) : 0;
	} else if (id == MT_STREAM_IEC2) {
		return ((afe_read(AFE_IEC2_CFG) & IEC2_EN_MASK) == IEC2_ENABLE)
			? afe_read(AFE_SPDIF2_CUR) : 0;
	} else
		return 0;
}

u32 afe_spdif_base(enum mt_stream_id id)
{
	if (id == MT_STREAM_IEC1)
		return afe_read(AFE_SPDIF_BASE);
	else if (id == MT_STREAM_IEC2)
		return afe_read(AFE_SPDIF2_BASE);
	else
		return 0;
}

u32 afe_spdif_end(enum mt_stream_id id)
{
	if (id == MT_STREAM_IEC1)
		return afe_read(AFE_SPDIF_END);
	else if (id == MT_STREAM_IEC2)
		return afe_read(AFE_SPDIF2_END);
	else
		return 0;
}

static u32 iec_nsadr[2];

u32 afe_iec_nsadr(enum mt_stream_id id)
{
	if (id == MT_STREAM_IEC1)
		return iec_nsadr[0];
	else if (id == MT_STREAM_IEC2)
		return iec_nsadr[1];
	else
		return 0;
}

void afe_iec_set_nsadr(enum mt_stream_id id, u32 nsadr)
{
	if (id == MT_STREAM_IEC1) {
		iec_nsadr[0] = nsadr;
		afe_write(AFE_IEC_NSADR, nsadr);
	} else if (id == MT_STREAM_IEC2) {
		iec_nsadr[1] = nsadr;
		afe_write(AFE_IEC2_NSADR, nsadr);
	}
}

int afe_iec_power_on(enum mt_stream_id id, int on)
{
	int off = !on;

	if (id == MT_STREAM_IEC1)
		afe_msk_write(AUDIO_TOP_CON0, off << 21, PDN_SPDIF_CK);
	else if (id == MT_STREAM_IEC2)
		afe_msk_write(AUDIO_TOP_CON0, off << 22, PDN_SPDIF2_CK);
	return 0;
}

/**************** hdmi 8ch i2s ****************/

static void set_hdmi_interconnection(unsigned int input, unsigned int output, int connect)
{
	u32 val, msk, shft;
	unsigned int in, out;
	static char status[HDMI_NUM_INPUT][HDMI_NUM_OUTPUT] = {
		/* O_20 O_21 O_22 O_23 O_24 O_25 O_26 O_27 O_28 O_29 */
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, /* I_20 */
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, /* I_21 */
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, /* I_22 */
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, /* I_23 */
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, /* I_24 */
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, /* I_25 */
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, /* I_26 */
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}	/* I_27 */
	};
	/* shift bits of connection register to set for certain output */
	static const int shift[HDMI_NUM_OUTPUT] = {
		/* O_20 O_21 O_22 O_23 O_24 O_25 O_26 O_27 O_28 O_29 */
		0, 3, 6, 9, 12, 15, 18, 21, 24, 27
	};
	/* value of connection register to set for certain input */
	static const u32 value[HDMI_NUM_INPUT] = {
		/* I_20 I_21 I_22 I_23 I_24 I_25 I_26 I_27 */
		0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7
	};
	/* mask of connection register to set for certain output */
	static const u32 mask[HDMI_NUM_OUTPUT] = {
		/* O_20  O_21  O_22  O_23  O_24  O_25  O_26  O_27  O_28  O_29 */
		0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7
	};

	pr_debug("%s() input=%d, output=%d, connect=%d\n", __func__, input, output, connect);
	if (input < HDMI_IN_BASE || input > HDMI_IN_MAX
	|| output < HDMI_OUT_BASE || output > HDMI_OUT_MAX) {
		pr_err("%s() error: invalid input or output\n", __func__);
		return;
	}
	in = input - HDMI_IN_BASE;
	val = value[connect ? in : (HDMI_IN_I27 - HDMI_IN_BASE)];
	out = output - HDMI_OUT_BASE;
	msk = mask[out];
	shft = shift[out];
	afe_msk_write(AFE_HDMI_CONN0, val << shft, msk << shft);
	status[in][out] = connect;
}

void afe_hdmi_i2s_set_interconnection(unsigned int channels, int connect)
{
	static int status;

	if (status == connect)
		return;

	pr_debug("%s() %s channels=%d\n", __func__, connect ? "connect" : "disconnect", channels);
	/* O20~O27: L/R/LS/RS/C/LFE/CH7/CH8 */
	switch (channels) {
	case 8: default:
		set_hdmi_interconnection(HDMI_IN_I27, HDMI_OUT_O27, connect);
		/* fallthrough */
	case 7:
		set_hdmi_interconnection(HDMI_IN_I26, HDMI_OUT_O26, connect);
		/* fallthrough */
	case 6:
		set_hdmi_interconnection(HDMI_IN_I25, HDMI_OUT_O25, connect);
		/* fallthrough */
	case 5:
		set_hdmi_interconnection(HDMI_IN_I24, HDMI_OUT_O24, connect);
		/* fallthrough */
	case 4:
		set_hdmi_interconnection(HDMI_IN_I23, HDMI_OUT_O23, connect);
		/* fallthrough */
	case 3:
		set_hdmi_interconnection(HDMI_IN_I22, HDMI_OUT_O22, connect);
		/* fallthrough */
	case 2:
		set_hdmi_interconnection(HDMI_IN_I21, HDMI_OUT_O21, connect);
		/* fallthrough */
	case 1:
		set_hdmi_interconnection(HDMI_IN_I20, HDMI_OUT_O20, connect);
		break;
	}
	status = connect;
}

void afe_hdmi_i2s_clock_on(unsigned int rate, unsigned int mclk,
	SPDIF_PCM_BITWIDTH eBitWidth, unsigned int DSDBCK)
{
	unsigned int div;

	afe_f_apll_ck_on(mclk);
	/* bck divider */
	switch (eBitWidth) {
	case PCM_OUTPUT_8BIT:
		if (DSDBCK == 1)
			/* for dsd  = 64 * fs */
			div = ((mclk / (64 * rate)) / 2) - 1;
		else
			/* 1 */
			div = ((mclk / (16 * rate)) / 2) - 1;
		break;
	case PCM_OUTPUT_16BIT:
		if (DSDBCK == 1)
			/* for dsd  = 64 * fs */
			div = ((mclk / (64 * rate)) / 2) - 1;
		else
			/* 1 */
			div = ((mclk / (32 * rate)) / 2) - 1;
		break;
	case PCM_OUTPUT_24BIT:
	case PCM_OUTPUT_32BIT:
		/* 0 */
		div = ((mclk / (64 * rate)) / 2) - 1;
		break;
	default:
		div = 0;
		pr_err("%s() error: invalid bck divider\n", __func__);
		break;
	}
	afe_msk_write(AUDIO_TOP_CON0, 0x0 << 23, HDMISPDIF_POWER_UP_MASK);
	afe_msk_write(AUDIO_TOP_CON3, div << 8, HDMI_CK_DIV_MASK);
	afe_msk_write(AUDIO_TOP_CON0, 0x0 << 20, HDMI_POWER_UP_MASK);
}

void afe_hdmi_i2s_clock_off(void)
{
	afe_msk_write(AUDIO_TOP_CON0, 0x1 << 23, HDMISPDIF_POWER_UP_MASK);
	afe_msk_write(AUDIO_TOP_CON0, 0x1 << 20, HDMI_POWER_UP_MASK);
	afe_f_apll_ck_off();
}

void set_hdmi_out_control(unsigned int channels, unsigned int input_bit)
{
	afe_msk_write(AFE_HDMI_OUT_CON0,
		(channels << 4) | input_bit,
		HDMI_OUT_CH_NUM_MASK | HDMI_OUT_BIT_WIDTH_MASK);
}

void set_hdmi_out_dsd_control(unsigned int channels, unsigned int dsd_bit)
{
	afe_msk_write(AFE_HDMI_OUT_CON0,
		(channels << 4) | HDMI_OUT_BIT_WIDTH_32 | dsd_bit,
		HDMI_OUT_CH_NUM_MASK | HDMI_OUT_BIT_WIDTH_MASK | HDMI_OUT_DSD_WDITH_MASK);
}

void set_hdmi_out_control_enable(bool en)
{
	u32 val = en ? HDMI_OUT_ON : HDMI_OUT_OFF;

	afe_msk_write(AFE_HDMI_OUT_CON0, val, HDMI_OUT_ON_MASK);
}

void set_hdmi_i2s(void)
{
	afe_msk_write(AFE_8CH_I2S_OUT_CON,
		HDMI_8CH_I2S_CON_I2S_32BIT | HDMI_8CH_I2S_CON_I2S_DELAY |
		HDMI_8CH_I2S_CON_LRCK_INV | HDMI_8CH_I2S_CON_BCK_INV,
		HDMI_8CH_I2S_CON_I2S_WLEN_MASK | HDMI_8CH_I2S_CON_I2S_DELAY_MASK |
		HDMI_8CH_I2S_CON_LRCK_INV_MASK | HDMI_8CH_I2S_CON_BCK_INV_MASK);
}

void set_hdmi_i2s_dsd(void)
{
	afe_msk_write(AFE_8CH_I2S_OUT_CON,
		HDMI_8CH_I2S_CON_BCK_INV | HDMI_8CH_I2S_CON_LRCK_NO_INV | HDMI_8CH_I2S_CON_I2S_DELAY |
		HDMI_8CH_I2S_CON_I2S_32BIT | HDMI_8CH_I2S_CON_DSD,
		HDMI_8CH_I2S_CON_BCK_INV_MASK | HDMI_8CH_I2S_CON_LRCK_INV_MASK | HDMI_8CH_I2S_CON_I2S_DELAY_MASK |
		HDMI_8CH_I2S_CON_I2S_WLEN_MASK | HDMI_8CH_I2S_CON_DSD_MODE_MASK);
}

void set_hdmi_i2s_enable(bool en)
{
	u32 val = en ? HDMI_8CH_I2S_CON_EN : HDMI_8CH_I2S_CON_DIS;

	afe_msk_write(AFE_8CH_I2S_OUT_CON, val, HDMI_8CH_I2S_CON_EN_MASK);
}

void set_hdmi_i2s_to_i2s5(void)
{
	afe_msk_write(AUDIO_TOP_CON3,
		SPEAKER_CLOCK_AND_DATA_FROM_HDMI | (0 << 6),
		SPEAKER_CLOCK_AND_DATA_FROM_MASK | SPEAKER_OUT_HDMI_SEL_MASK);
}

void init_hdmi_dma_buffer(u32 base, u32 size)
{
	afe_write(AFE_HDMI_OUT_BASE, base);
	afe_write(AFE_HDMI_OUT_END, base + size - 1);
}
