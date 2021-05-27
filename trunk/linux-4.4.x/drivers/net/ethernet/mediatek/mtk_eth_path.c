/*
 *   Copyright (C) 2018 MediaTek Inc.
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; version 2 of the License
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   Copyright (C) 2018 Sean Wang <sean.wang@mediatek.com>
 */

#include <linux/phy.h>
#include <linux/regmap.h>

#include "mtk_eth_soc.h"

struct mtk_eth_muxc {
	int (*set_path)(struct mtk_eth *eth, int path);
};

static const char * const mtk_eth_mux_name[] = {
	"mux_gdm1_to_gmac1_esw", "mux_gmac2_gmac0_to_gephy",
	"mux_u3_gmac2_to_qphy", "mux_gmac1_gmac2_to_sgmii_rgmii",
	"mux_gmac12_to_gephy_sgmii",
};

static const char * const mtk_eth_path_name[] = {
	"gmac1_rgmii", "gmac1_trgmii", "gmac1_sgmii", "gmac2_rgmii",
	"gmac2_sgmii", "gmac2_gephy", "gdm1_esw",
};

static int set_mux_gdm1_to_gmac1_esw(struct mtk_eth *eth, int path)
{
	u32 val, mask, set;
	bool updated = true;

	switch (path) {
	case MTK_ETH_PATH_GMAC1_SGMII:
		mask = ~(u32)MTK_MUX_TO_ESW;
		set = 0;
		break;
	case MTK_ETH_PATH_GDM1_ESW:
		mask = ~(u32)MTK_MUX_TO_ESW;
		set = MTK_MUX_TO_ESW;
		break;
	default:
		updated = false;
		break;
	};

	if (updated) {
		val = mtk_r32(eth, MTK_MAC_MISC);
		val = (val & mask) | set;
		mtk_w32(eth, val, MTK_MAC_MISC);
	}

	dev_info(eth->dev, "path %s in %s updated = %d\n",
		 mtk_eth_path_name[path], __func__, updated);

	return 0;
}

static int set_mux_gmac2_gmac0_to_gephy(struct mtk_eth *eth, int path)
{
	unsigned int val = 0;
	bool updated = true;

	switch (path) {
	case MTK_ETH_PATH_GMAC2_GEPHY:
		val = ~(u32)GEPHY_MAC_SEL;
		break;
	default:
		updated = false;
		break;
	}

	if (updated)
		regmap_update_bits(eth->infra, INFRA_MISC2, GEPHY_MAC_SEL, val);

	dev_info(eth->dev, "path %s in %s updated = %d\n",
		 mtk_eth_path_name[path], __func__, updated);

	return 0;
}

static int set_mux_u3_gmac2_to_qphy(struct mtk_eth *eth, int path)
{
	unsigned int val = 0;
	bool updated = true;

	switch (path) {
	case MTK_ETH_PATH_GMAC2_SGMII:
		val = CO_QPHY_SEL;
		break;
	default:
		updated = false;
		break;
	}

	if (updated)
		regmap_update_bits(eth->infra, INFRA_MISC2, CO_QPHY_SEL, val);

	dev_info(eth->dev, "path %s in %s updated = %d\n",
		 mtk_eth_path_name[path], __func__, updated);

	return 0;
}

static int set_mux_gmac1_gmac2_to_sgmii_rgmii(struct mtk_eth *eth, int path)
{
	unsigned int val = 0;
	bool updated = true;

	switch (path) {
	case MTK_ETH_PATH_GMAC1_SGMII:
		val = SYSCFG0_SGMII_GMAC1;
		break;
	case MTK_ETH_PATH_GMAC2_SGMII:
		val = SYSCFG0_SGMII_GMAC2;
		break;
	case MTK_ETH_PATH_GMAC1_RGMII:
	case MTK_ETH_PATH_GMAC2_RGMII:
		regmap_read(eth->ethsys, ETHSYS_SYSCFG0, &val);
		val &= SYSCFG0_SGMII_MASK;

		if ((path == MTK_GMAC1_RGMII && val == SYSCFG0_SGMII_GMAC1) ||
		    (path == MTK_GMAC2_RGMII && val == SYSCFG0_SGMII_GMAC2))
			val = 0;
		else
			updated = false;
		break;
	default:
		updated = false;
		break;
	};

	if (updated)
		regmap_update_bits(eth->ethsys, ETHSYS_SYSCFG0,
				   SYSCFG0_SGMII_MASK, val);

	dev_info(eth->dev, "path %s in %s updated = %d\n",
		 mtk_eth_path_name[path], __func__, updated);

	return 0;
}

static int set_mux_gmac12_to_gephy_sgmii(struct mtk_eth *eth, int path)
{
	unsigned int val = 0;
	bool updated = true;

	regmap_read(eth->ethsys, ETHSYS_SYSCFG0, &val);

	switch (path) {
	case MTK_ETH_PATH_GMAC1_SGMII:
		val |= SYSCFG0_SGMII_GMAC1_V2;
		break;
	case MTK_ETH_PATH_GMAC2_GEPHY:
		val &= ~(u32)SYSCFG0_SGMII_GMAC2_V2;
		break;
	case MTK_ETH_PATH_GMAC2_SGMII:
		val |= SYSCFG0_SGMII_GMAC2_V2;
		break;
	default:
		updated = false;
	};

	if (updated)
		regmap_update_bits(eth->ethsys, ETHSYS_SYSCFG0,
				   SYSCFG0_SGMII_MASK, val);

	if (!updated)
		dev_info(eth->dev, "path %s no needs updatiion in %s\n",
			 mtk_eth_path_name[path], __func__);

	dev_info(eth->dev, "path %s in %s updated = %d\n",
		 mtk_eth_path_name[path], __func__, updated);

	return 0;
}

static const struct mtk_eth_muxc mtk_eth_muxc[] = {
	{ .set_path = set_mux_gdm1_to_gmac1_esw, },
	{ .set_path = set_mux_gmac2_gmac0_to_gephy, },
	{ .set_path = set_mux_u3_gmac2_to_qphy, },
	{ .set_path = set_mux_gmac1_gmac2_to_sgmii_rgmii, },
	{ .set_path = set_mux_gmac12_to_gephy_sgmii, }
};

static int mtk_eth_mux_setup(struct mtk_eth *eth, int path)
{
	int i, err = 0;

	if (!MTK_HAS_CAPS(eth->soc->caps, MTK_PATH_BIT(path))) {
		dev_info(eth->dev, "path %s isn't support on the SoC\n",
			 mtk_eth_path_name[path]);
		return -EINVAL;
	}

	if (!MTK_HAS_CAPS(eth->soc->caps, MTK_MUX))
		return 0;

	/* Setup MUX in path fabric */
	for (i = 0; i < MTK_ETH_MUX_MAX; i++) {
		if (MTK_HAS_CAPS(eth->soc->caps, MTK_MUX_BIT(i))) {
			err = mtk_eth_muxc[i].set_path(eth, path);
			if (err)
				goto out;
		} else {
			dev_info(eth->dev, "mux %s isn't present on the SoC\n",
				 mtk_eth_mux_name[i]);
		}
	}

out:
	return err;
}

static int mtk_gmac_sgmii_path_setup(struct mtk_eth *eth, int mac_id)
{
	unsigned int val = 0;
	int sid, err, path;

	path = (mac_id == 0) ?  MTK_ETH_PATH_GMAC1_SGMII :
				MTK_ETH_PATH_GMAC2_SGMII;

	/* Setup proper MUXes along the path */
	err = mtk_eth_mux_setup(eth, path);
	if (err)
		return err;

	/* The path GMAC to SGMII will be enabled once the SGMIISYS is being
	 * setup done.
	 */
	regmap_read(eth->ethsys, ETHSYS_SYSCFG0, &val);

	regmap_update_bits(eth->ethsys, ETHSYS_SYSCFG0,
			   SYSCFG0_SGMII_MASK, ~(u32)SYSCFG0_SGMII_MASK);

	/* Decide how GMAC and SGMIISYS be mapped */
	sid = (MTK_HAS_CAPS(eth->soc->caps, MTK_SHARED_SGMII)) ? 0 : mac_id;

	/* Setup SGMIISYS with the determined property */
	if (MTK_HAS_FLAGS(eth->sgmii->flags[sid], MTK_SGMII_PHYSPEED_AN))
		err = mtk_sgmii_setup_mode_an(eth->sgmii, sid);
	else
		err = mtk_sgmii_setup_mode_force(eth->sgmii, sid);

	if (err)
		return err;

	regmap_update_bits(eth->ethsys, ETHSYS_SYSCFG0,
			   SYSCFG0_SGMII_MASK, val);

	return 0;
}

static int mtk_gmac_gephy_path_setup(struct mtk_eth *eth, int mac_id)
{
	int err, path = 0;

	if (mac_id == 1)
		path = MTK_ETH_PATH_GMAC2_GEPHY;

	if (!path)
		return -EINVAL;

	/* Setup proper MUXes along the path */
	err = mtk_eth_mux_setup(eth, path);
	if (err)
		return err;

	return 0;
}

static int mtk_gmac_rgmii_path_setup(struct mtk_eth *eth, int mac_id)
{
	int err, path;

	path = (mac_id == 0) ?  MTK_ETH_PATH_GMAC1_RGMII :
				MTK_ETH_PATH_GMAC2_RGMII;

	/* Setup proper MUXes along the path */
	err = mtk_eth_mux_setup(eth, path);
	if (err)
		return err;

	return 0;
}

int mtk_setup_hw_path(struct mtk_eth *eth, int mac_id, int phymode)
{
	int err;

	switch (phymode) {
	case PHY_INTERFACE_MODE_TRGMII:
	case PHY_INTERFACE_MODE_RGMII_TXID:
	case PHY_INTERFACE_MODE_RGMII_RXID:
	case PHY_INTERFACE_MODE_RGMII_ID:
	case PHY_INTERFACE_MODE_RGMII:
	case PHY_INTERFACE_MODE_MII:
	case PHY_INTERFACE_MODE_REVMII:
	case PHY_INTERFACE_MODE_RMII:
		if (MTK_HAS_CAPS(eth->soc->caps, MTK_RGMII)) {
			err = mtk_gmac_rgmii_path_setup(eth, mac_id);
			if (err)
				return err;
		}
		break;
	case PHY_INTERFACE_MODE_SGMII:
		if (MTK_HAS_CAPS(eth->soc->caps, MTK_SGMII)) {
			err = mtk_gmac_sgmii_path_setup(eth, mac_id);
			if (err)
				return err;
		}
		break;
	case PHY_INTERFACE_MODE_GMII:
		if (MTK_HAS_CAPS(eth->soc->caps, MTK_GEPHY)) {
			err = mtk_gmac_gephy_path_setup(eth, mac_id);
			if (err)
				return err;
		}
		break;
	default:
		break;
	}

	return 0;
}
