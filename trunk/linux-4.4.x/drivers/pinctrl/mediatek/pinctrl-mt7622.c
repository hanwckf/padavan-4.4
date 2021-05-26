/*
 * Copyright (c) 2014-2015 MediaTek Inc.
 * Author: Hongzhou.Yang <hongzhou.yang@mediatek.com>
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

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/regmap.h>
#include <linux/pinctrl/pinconf-generic.h>
#include <dt-bindings/pinctrl/mt65xx.h>

#include "pinctrl-mtk-common.h"
#include "pinctrl-mtk-mt7622.h"
static int mtk_pinctrl_set_gpio_drv(struct mtk_pinctrl *pctl, unsigned int pin, unsigned char drv)
{
	unsigned int drv_e4, drv_e8;

	switch (drv) {
	case MTK_DRIVE_4mA:
		drv_e4 = 0;
		drv_e8 = 0;
		break;
	case MTK_DRIVE_8mA:
		drv_e4 = 1;
		drv_e8 = 0;
		break;
	case MTK_DRIVE_12mA:
		drv_e4 = 0;
		drv_e8 = 1;
		break;
	case MTK_DRIVE_16mA:
		drv_e4 = 1;
		drv_e8 = 1;
		break;
	default:
		break;
	}

	mtk_pinctrl_set_gpio_value(pctl, pin, drv_e4,
			pctl->devdata->n_pin_drve4, pctl->devdata->pin_drve4_grps);
	mtk_pinctrl_set_gpio_value(pctl, pin, drv_e8,
			pctl->devdata->n_pin_drve8, pctl->devdata->pin_drve8_grps);

	return 0;
}

static int mtk_pinctrl_get_gpio_drv(struct mtk_pinctrl *pctl, unsigned int pin)
{
	unsigned int drv_e4, drv_e8;

	drv_e4 = mtk_pinctrl_get_gpio_value(pctl, pin,
		pctl->devdata->n_pin_drve4, pctl->devdata->pin_drve4_grps);
	drv_e8 = mtk_pinctrl_get_gpio_value(pctl, pin,
		pctl->devdata->n_pin_drve8, pctl->devdata->pin_drve8_grps);

	return ((drv_e4)|(drv_e8<<1));
}

static int mtk_pinctrl_set_gpio_pu_pd(struct mtk_pinctrl *pctl, unsigned int pin,
	bool enable, bool isup, unsigned int arg)
{
	if (isup == 1) {
		mtk_pinctrl_set_gpio_value(pctl, pin, 0,
				pctl->devdata->n_pin_pd, pctl->devdata->pin_pd_grps);
		mtk_pinctrl_set_gpio_value(pctl, pin, 1,
				pctl->devdata->n_pin_pu, pctl->devdata->pin_pu_grps);
	}	else {
		mtk_pinctrl_set_gpio_value(pctl, pin, 0,
				pctl->devdata->n_pin_pu, pctl->devdata->pin_pu_grps);
		mtk_pinctrl_set_gpio_value(pctl, pin, 1,
				pctl->devdata->n_pin_pd, pctl->devdata->pin_pd_grps);
	}

	return 0;
}

static int mtk_pinctrl_get_gpio_pu_pd(struct mtk_pinctrl *pctl, unsigned int pin)
{
	unsigned int bit_pu = 0, bit_pd = 0;

	bit_pu = mtk_pinctrl_get_gpio_value(pctl, pin,
			pctl->devdata->n_pin_pu, pctl->devdata->pin_pu_grps);
	bit_pd = mtk_pinctrl_get_gpio_value(pctl, pin,
			pctl->devdata->n_pin_pd, pctl->devdata->pin_pd_grps);

	if ((bit_pd != -EPERM) && (bit_pu != -EPERM))
		return (bit_pd)|(bit_pu<<1);
	else
		return -EPERM;
}

static int mtk_pinctrl_get_gpio_pullsel(struct mtk_pinctrl *pctl, unsigned int pin)
{
	unsigned int pull_sel = 0;

	pull_sel = mtk_pinctrl_get_gpio_pu_pd(pctl, pin);
	/*pull_sel = [pu,pd], 10 is pull up, 01 is pull down*/
	if ((pull_sel == 0x02))
		pull_sel = GPIO_PULL_UP;
	else if ((pull_sel == 0x01))
		pull_sel = GPIO_PULL_DOWN;
	else if ((pull_sel == -EPERM))
		pull_sel = GPIO_PULL_UNSUPPORTED;
	else
		return GPIO_NO_PULL;

	return pull_sel;
}

static const unsigned int mt7622_debounce_data[] = {
	128, 256, 500, 1000, 16000,
	32000, 64000, 128000, 256000, 512000
};

static unsigned int mt7622_spec_debounce_select(unsigned debounce)
{
	return mtk_gpio_debounce_select(mt7622_debounce_data,
		ARRAY_SIZE(mt7622_debounce_data), debounce);
}

static const struct mtk_pinctrl_devdata mt7622_pinctrl_data = {
	.pins = mtk_pins_mt7622,
	.npins = ARRAY_SIZE(mtk_pins_mt7622),
	.pin_mode_grps = mtk_pin_info_mode,
	.n_pin_mode = ARRAY_SIZE(mtk_pin_info_mode),
	.pin_smt_grps = mtk_pin_info_smt,
	.n_pin_smt = ARRAY_SIZE(mtk_pin_info_smt),
	.pin_pu_grps = mtk_pin_info_pu,
	.n_pin_pu = ARRAY_SIZE(mtk_pin_info_pu),
	.pin_pd_grps = mtk_pin_info_pd,
	.n_pin_pd = ARRAY_SIZE(mtk_pin_info_pd),
	.pin_drve4_grps = mtk_pin_info_drve4,
	.n_pin_drve4 = ARRAY_SIZE(mtk_pin_info_drve4),
	.pin_drve8_grps = mtk_pin_info_drve8,
	.n_pin_drve8 = ARRAY_SIZE(mtk_pin_info_drve8),
	.pin_dout_grps = mtk_pin_info_dataout,
	.n_pin_dout = ARRAY_SIZE(mtk_pin_info_dataout),
	.pin_din_grps = mtk_pin_info_datain,
	.n_pin_din = ARRAY_SIZE(mtk_pin_info_datain),
	.pin_dir_grps = mtk_pin_info_dir,
	.n_pin_dir = ARRAY_SIZE(mtk_pin_info_dir),
	.mtk_pctl_set_pull_sel = mtk_pinctrl_set_gpio_pu_pd,
	.mtk_pctl_get_pull_sel = mtk_pinctrl_get_gpio_pullsel,
	.mtk_pctl_set_gpio_drv = mtk_pinctrl_set_gpio_drv,
	.mtk_pctl_get_gpio_drv = mtk_pinctrl_get_gpio_drv,
	.spec_debounce_select = mt7622_spec_debounce_select,
	.type1_start = 103,
	.type1_end = 103,
	.regmap_num = 0,
	.port_shf = 4,
	.port_mask = 0xf,
	.port_align = 4,
	.eint_offsets = {
		.name = "mt7622_eint",
		.stat      = 0x000,
		.ack       = 0x040,
		.mask      = 0x080,
		.mask_set  = 0x0c0,
		.mask_clr  = 0x100,
		.sens      = 0x140,
		.sens_set  = 0x180,
		.sens_clr  = 0x1c0,
		.soft      = 0x200,
		.soft_set  = 0x240,
		.soft_clr  = 0x280,
		.pol       = 0x300,
		.pol_set   = 0x340,
		.pol_clr   = 0x380,
		.dom_en    = 0x400,
		.dbnc_ctrl = 0x500,
		.dbnc_set  = 0x600,
		.dbnc_clr  = 0x700,
		.port_mask = 7,
		.ports     = 7,
	},
	.ies_offset = MTK_PINCTRL_NOT_SUPPORT,
	.ap_num = 213,
	.db_cnt = 20,
};

static int mt7622_pinctrl_probe(struct platform_device *pdev)
{
	return mtk_pctrl_init(pdev, &mt7622_pinctrl_data, NULL);
}

static const struct of_device_id mt7622_pctrl_match[] = {
	{
		.compatible = "mediatek,mt7622-pinctrl",
	},
	{ }
};
MODULE_DEVICE_TABLE(of, mt7622_pctrl_match);

static struct platform_driver mtk_pinctrl_driver = {
	.probe = mt7622_pinctrl_probe,
	.driver = {
		.name = "mediatek-mt7622-pinctrl",
		.of_match_table = mt7622_pctrl_match,
	},
};

static int __init mtk_pinctrl_init(void)
{
	return platform_driver_register(&mtk_pinctrl_driver);
}

arch_initcall(mtk_pinctrl_init);

