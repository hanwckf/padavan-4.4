/*
 * Copyright (c) 2016 MediaTek Inc.
 * Author: Chenglin Xu <chenglin.xu@mediatek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/delay.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/regmap.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/io.h>

#define E_PWR_INVALID_DATA	33
static struct regmap *pwrap_regmap;

unsigned int pmic_read_interface(unsigned int RegNum, unsigned int *val, unsigned int MASK, unsigned int SHIFT)
{
	unsigned int return_value = 0;
	unsigned int pmic_reg = 0;

	return_value = regmap_read(pwrap_regmap, RegNum, &pmic_reg);
	if (return_value != 0) {
		pr_err("[Power/PMIC][pmic_read_interface] Reg[%x]= pmic_wrap read data fail\n", RegNum);
		return return_value;
	}

	pmic_reg &= (MASK << SHIFT);
	*val = (pmic_reg >> SHIFT);

	return return_value;
}

unsigned int pmic_config_interface(unsigned int RegNum, unsigned int val, unsigned int MASK, unsigned int SHIFT)
{
	unsigned int return_value = 0;
	unsigned int pmic_reg = 0;

	if (val > MASK) {
		pr_err("[Power/PMIC][pmic_config_interface] Invalid data, Reg[%x]: MASK = 0x%x, val = 0x%x\n",
			RegNum, MASK, val);
		return E_PWR_INVALID_DATA;
	}

	return_value = regmap_read(pwrap_regmap, RegNum, &pmic_reg);
	if (return_value != 0) {
		pr_err("[Power/PMIC][pmic_config_interface] Reg[%x]= pmic_wrap read data fail\n", RegNum);
		return return_value;
	}

	pmic_reg &= ~(MASK << SHIFT);
	pmic_reg |= (val << SHIFT);

	return_value = regmap_write(pwrap_regmap, RegNum, pmic_reg);
	if (return_value != 0) {
		pr_err("[Power/PMIC][pmic_config_interface] Reg[%x]= pmic_wrap read data fail\n", RegNum);
		return return_value;
	}

	return return_value;
}


static u32 g_reg_value;
static ssize_t show_pmic_access(struct device *dev, struct device_attribute *attr, char *buf)
{
	pr_notice("[show_pmic_access] 0x%x\n", g_reg_value);
	return sprintf(buf, "%08X\n", g_reg_value);
}

static ssize_t store_pmic_access(struct device *dev, struct device_attribute *attr, const char *buf,
				 size_t size)
{
	int ret = 0;
	char temp_buf[32];
	char *pvalue;
	unsigned int reg_value = 0;
	unsigned int reg_address = 0;

	strncpy(temp_buf, buf, sizeof(temp_buf));
	temp_buf[sizeof(temp_buf) - 1] = 0;
	pvalue = temp_buf;

	if (size != 0) {
		if (size > 5) {
			ret = kstrtouint(strsep(&pvalue, " "), 16, &reg_address);
			if (ret)
				return ret;
			ret = kstrtouint(pvalue, 16, &reg_value);
			if (ret)
				return ret;
			pr_notice("[store_pmic_access] write PMU reg 0x%x with value 0x%x !\n",
				  reg_address, reg_value);
			ret = pmic_config_interface(reg_address, reg_value, 0xFFFFFFFF, 0x0);
		} else {
			ret = kstrtouint(pvalue, 16, &reg_address);
			if (ret)
				return ret;
			ret = pmic_read_interface(reg_address, &g_reg_value, 0xFFFFFFFF, 0x0);
			pr_notice("[store_pmic_access] read PMU reg 0x%x with value 0x%x !\n",
				  reg_address, g_reg_value);
			pr_notice
			    ("[store_pmic_access] Please use \"cat pmic_access\" to get value\r\n");
		}
	}
	return size;
}

static DEVICE_ATTR(pmic_access, 0664, show_pmic_access, store_pmic_access);	/* 664 */

static int mt6380_pmic_probe(struct platform_device *dev)
{
	struct regmap *regmap = dev_get_regmap(dev->dev.parent, NULL);

	pwrap_regmap = regmap;
	device_create_file(&(dev->dev), &dev_attr_pmic_access);

	pr_debug("[Power/PMIC] ******** MT6380 pmic driver probe done!! ********\n");

	return 0;
}

static const struct platform_device_id mt6380_pmic_ids[] = {
	{"mt6380", 0},
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(platform, mt6380_pmic_ids);

static const struct of_device_id mt6380_pmic_of_match[] = {
	{ .compatible = "mediatek,mt6380", },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, mt6380_pmic_of_match);

static struct platform_driver mt6380_pmic_driver = {
	.driver = {
		.name = "mt6380",
		.of_match_table = of_match_ptr(mt6380_pmic_of_match),
	},
	.probe = mt6380_pmic_probe,
	.id_table = mt6380_pmic_ids,
};

module_platform_driver(mt6380_pmic_driver);

MODULE_AUTHOR("Chenglin Xu <chenglin.xu@mediatek.com>");
MODULE_DESCRIPTION("PMIC Misc Setting Driver for MediaTek MT6380 PMIC");
MODULE_LICENSE("GPL v2");
