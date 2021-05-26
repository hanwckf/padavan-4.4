/*
 * Copyright (c) 2017 MediaTek Inc.
 * Author: Ryder Lee <ryder.lee@mediatek.com>
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

#include <linux/kernel.h>
#include <linux/mfd/syscon.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/regmap.h>

#define WBSYS_IOC_PDMA			BIT(31)

#define HIFSYS_IOC_PCIE0		BIT(0)
#define HIFSYS_IOC_PCIE1_SATA		BIT(1)
#define HIFSYS_IOC_PCIE_SATA_MASK	GENMASK(1, 0)

#define HIFSYS_IOC_SSUSB_XHCI_MAS	BIT(2)
#define HIFSYS_IOC_SSUSB_XHCI_U3DATA	BIT(3)
#define HIFSYS_IOC_SSUSB_DEV		BIT(4)
#define HIFSYS_IOC_SSUSB_MASK		GENMASK(4, 2)

#define ETHSYS_IOC_PDMA			BIT(0)
#define ETHSYS_IOC_QDMA			BIT(1)
#define ETHSYS_IOC_PPE			BIT(2)
#define ETHSYS_IOC_WDMA0		BIT(3)
#define ETHSYS_IOC_HSDMA		BIT(4)
#define ETHSYS_IOC_GDMA			BIT(5)
#define ETHSYS_IOC_WDMA1		BIT(6)
#define ETHSYS_IOC_ETH_MASK		GENMASK(1, 0)

struct mtk_subsys_ioc_pdata {
	bool has_wbsys;
	bool has_hifsys;
	bool has_ethsys;

	unsigned int wb_ioc_off;
	unsigned int hif_ioc_off;
	unsigned int eth_ioc_off;
};

static int mtk_subsys_ioc_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	const struct mtk_subsys_ioc_pdata *data;
	struct regmap *wbsys, *hifsys, *ethsys;

	data = of_device_get_match_data(dev);
	if (!data)
		return -ENODEV;

	/* enable WIFI IOC if needed */
	if (data->has_wbsys && of_property_read_bool(np, "mediatek,en_wifi")) {
		wbsys = syscon_regmap_lookup_by_phandle(np, "mediatek,wbsys");
		if (IS_ERR(wbsys))
			return PTR_ERR(wbsys);

		regmap_update_bits(wbsys, data->wb_ioc_off, WBSYS_IOC_PDMA,
				   WBSYS_IOC_PDMA);
	}

	/* enable USB/PCIe/SATA IOC if needed */
	if (data->has_hifsys) {
		hifsys = syscon_regmap_lookup_by_phandle(np, "mediatek,hifsys");
		if (IS_ERR(hifsys))
			return PTR_ERR(hifsys);

		if (of_property_read_bool(np, "mediatek,en_usb"))
			regmap_update_bits(hifsys, data->hif_ioc_off,
					   HIFSYS_IOC_SSUSB_MASK,
					   HIFSYS_IOC_SSUSB_XHCI_MAS |
					   HIFSYS_IOC_SSUSB_XHCI_U3DATA |
					   HIFSYS_IOC_SSUSB_DEV);

		if (of_property_read_bool(np, "mediatek,en_pcie_sata"))
			regmap_update_bits(hifsys, data->hif_ioc_off,
					   HIFSYS_IOC_PCIE_SATA_MASK,
					   HIFSYS_IOC_PCIE0 |
					   HIFSYS_IOC_PCIE1_SATA);
	}

	/* enable ethernet IOC if needed */
	if (data->has_ethsys && of_property_read_bool(np, "mediatek,en_eth")) {
		ethsys = syscon_regmap_lookup_by_phandle(np, "mediatek,ethsys");
		if (IS_ERR(ethsys))
			return PTR_ERR(ethsys);

		/* check QoS */
		if (IS_ENABLED(CONFIG_QDMA_SUPPORT_QOS))
			regmap_update_bits(ethsys, data->eth_ioc_off,
					   ETHSYS_IOC_ETH_MASK,
					   ETHSYS_IOC_PDMA);
		else
			regmap_update_bits(ethsys, data->eth_ioc_off,
					   ETHSYS_IOC_ETH_MASK,
					   ETHSYS_IOC_PDMA |
					   ETHSYS_IOC_QDMA);
	}

	return 0;
}

static const struct mtk_subsys_ioc_pdata subsys_ioc_v2 = {
	.has_ethsys = true,
	.eth_ioc_off = 0x84,
};

static const struct mtk_subsys_ioc_pdata subsys_ioc_v1 = {
	.has_wbsys = true,
	.has_hifsys = true,
	.has_ethsys = true,
	.wb_ioc_off = 0x320,
	.hif_ioc_off = 0x08,
	.eth_ioc_off = 0x408,
};

static const struct of_device_id of_subsys_ioc_id_table[] = {
	{
		.compatible = "mediatek,mt7622-subsys-ioc",
		.data = &subsys_ioc_v1,
	}, {
		.compatible = "mediatek,leopard-subsys-ioc",
		.data = &subsys_ioc_v2,
	}, {
		/* sentinel */
	}
};

static struct platform_driver mtk_subsys_ioc_driver = {
	.probe		= mtk_subsys_ioc_probe,
	.driver		= {
		.name	= "mtk_subsys_ioc",
		.of_match_table = of_subsys_ioc_id_table,
		.suppress_bind_attrs = true,
	},
};

builtin_platform_driver(mtk_subsys_ioc_driver);
