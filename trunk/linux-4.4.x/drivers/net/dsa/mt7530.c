/*
 * Mediatek MT7530 DSA Switch driver
 * Copyright (C) 2017 Sean Wang <sean.wang@mediatek.com>
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
#include <linux/etherdevice.h>
#include <linux/if_bridge.h>
#include <linux/iopoll.h>
#include <linux/mdio.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/of_gpio.h>
#include <linux/of_mdio.h>
#include <linux/of_net.h>
#include <linux/of_platform.h>
#include <linux/phy.h>
#include <linux/regmap.h>
#include <linux/regulator/consumer.h>
#include <linux/reset.h>
#include <linux/gpio/consumer.h>
#include <net/dsa.h>
#include <net/switchdev.h>

#include "mt7530.h"

static struct mt7530_priv *mt7530_data;

/* String, offset, and register size in bytes if different from 4 bytes */
static const struct mt7530_mib_desc mt7530_mib[] = {
	MIB_DESC(1, 0x00, "TxDrop"),
	MIB_DESC(1, 0x04, "TxCrcErr"),
	MIB_DESC(1, 0x08, "TxUnicast"),
	MIB_DESC(1, 0x0c, "TxMulticast"),
	MIB_DESC(1, 0x10, "TxBroadcast"),
	MIB_DESC(1, 0x14, "TxCollision"),
	MIB_DESC(1, 0x18, "TxSingleCollision"),
	MIB_DESC(1, 0x1c, "TxMultipleCollision"),
	MIB_DESC(1, 0x20, "TxDeferred"),
	MIB_DESC(1, 0x24, "TxLateCollision"),
	MIB_DESC(1, 0x28, "TxExcessiveCollistion"),
	MIB_DESC(1, 0x2c, "TxPause"),
	MIB_DESC(1, 0x30, "TxPktSz64"),
	MIB_DESC(1, 0x34, "TxPktSz65To127"),
	MIB_DESC(1, 0x38, "TxPktSz128To255"),
	MIB_DESC(1, 0x3c, "TxPktSz256To511"),
	MIB_DESC(1, 0x40, "TxPktSz512To1023"),
	MIB_DESC(1, 0x44, "Tx1024ToMax"),
	MIB_DESC(2, 0x48, "TxBytes"),
	MIB_DESC(1, 0x60, "RxDrop"),
	MIB_DESC(1, 0x64, "RxFiltering"),
	MIB_DESC(1, 0x6c, "RxMulticast"),
	MIB_DESC(1, 0x70, "RxBroadcast"),
	MIB_DESC(1, 0x74, "RxAlignErr"),
	MIB_DESC(1, 0x78, "RxCrcErr"),
	MIB_DESC(1, 0x7c, "RxUnderSizeErr"),
	MIB_DESC(1, 0x80, "RxFragErr"),
	MIB_DESC(1, 0x84, "RxOverSzErr"),
	MIB_DESC(1, 0x88, "RxJabberErr"),
	MIB_DESC(1, 0x8c, "RxPause"),
	MIB_DESC(1, 0x90, "RxPktSz64"),
	MIB_DESC(1, 0x94, "RxPktSz65To127"),
	MIB_DESC(1, 0x98, "RxPktSz128To255"),
	MIB_DESC(1, 0x9c, "RxPktSz256To511"),
	MIB_DESC(1, 0xa0, "RxPktSz512To1023"),
	MIB_DESC(1, 0xa4, "RxPktSz1024ToMax"),
	MIB_DESC(2, 0xa8, "RxBytes"),
	MIB_DESC(1, 0xb0, "RxCtrlDrop"),
	MIB_DESC(1, 0xb4, "RxIngressDrop"),
	MIB_DESC(1, 0xb8, "RxArlDrop"),
};

static int
mt7623_trgmii_write(struct mt7530_priv *priv,  u32 reg, u32 val)
{
	int ret;

	ret =  regmap_write(priv->ethernet, TRGMII_BASE(reg), val);
	if (ret < 0)
		dev_err(priv->dev,
			"failed to priv write register\n");
	return ret;
}

static u32
mt7623_trgmii_read(struct mt7530_priv *priv, u32 reg)
{
	int ret;
	u32 val;

	ret = regmap_read(priv->ethernet, TRGMII_BASE(reg), &val);
	if (ret < 0) {
		dev_err(priv->dev,
			"failed to priv read register\n");
		return ret;
	}

	return val;
}

static void
mt7623_trgmii_rmw(struct mt7530_priv *priv, u32 reg,
		  u32 mask, u32 set)
{
	u32 val;

	val = mt7623_trgmii_read(priv, reg);
	val &= ~mask;
	val |= set;
	mt7623_trgmii_write(priv, reg, val);
}

static void
mt7623_trgmii_set(struct mt7530_priv *priv, u32 reg, u32 val)
{
	mt7623_trgmii_rmw(priv, reg, 0, val);
}

static void
mt7623_trgmii_clear(struct mt7530_priv *priv, u32 reg, u32 val)
{
	mt7623_trgmii_rmw(priv, reg, val, 0);
}

static int
core_read_mmd_indirect(struct mt7530_priv *priv, int prtad, int devad)
{
	struct mii_bus *bus = priv->bus;
	int value, ret;

	/* Write the desired MMD Devad */
	ret = bus->write(bus, 0, MII_MMD_CTRL, devad);
	if (ret < 0)
		goto err;

	/* Write the desired MMD register address */
	ret = bus->write(bus, 0, MII_MMD_DATA, prtad);
	if (ret < 0)
		goto err;

	/* Select the Function : DATA with no post increment */
	ret = bus->write(bus, 0, MII_MMD_CTRL, (devad | MII_MMD_CTRL_NOINCR));
	if (ret < 0)
		goto err;

	/* Read the content of the MMD's selected register */
	value = bus->read(bus, 0, MII_MMD_DATA);

	return value;
err:
	dev_err(&bus->dev,  "failed to read mmd register\n");

	return ret;
}

static int
core_write_mmd_indirect(struct mt7530_priv *priv, int prtad,
			int devad, u32 data)
{
	struct mii_bus *bus = priv->bus;
	int ret;

	/* Write the desired MMD Devad */
	ret = bus->write(bus, 0, MII_MMD_CTRL, devad);
	if (ret < 0)
		goto err;

	/* Write the desired MMD register address */
	ret = bus->write(bus, 0, MII_MMD_DATA, prtad);
	if (ret < 0)
		goto err;

	/* Select the Function : DATA with no post increment */
	ret = bus->write(bus, 0, MII_MMD_CTRL, (devad | MII_MMD_CTRL_NOINCR));
	if (ret < 0)
		goto err;

	/* Write the data into MMD's selected register */
	ret = bus->write(bus, 0, MII_MMD_DATA, data);
err:
	if (ret < 0)
		dev_err(&bus->dev,
			"failed to write mmd register\n");
	return ret;
}

static void
core_write(struct mt7530_priv *priv, u32 reg, u32 val)
{
	struct mii_bus *bus = priv->bus;

	mutex_lock_nested(&bus->mdio_lock, MDIO_MUTEX_NESTED);

	core_write_mmd_indirect(priv, reg, MDIO_MMD_VEND2, val);

	mutex_unlock(&bus->mdio_lock);
}

static void
core_rmw(struct mt7530_priv *priv, u32 reg, u32 mask, u32 set)
{
	struct mii_bus *bus = priv->bus;
	u32 val;

	mutex_lock_nested(&bus->mdio_lock, MDIO_MUTEX_NESTED);

	val = core_read_mmd_indirect(priv, reg, MDIO_MMD_VEND2);
	val &= ~mask;
	val |= set;
	core_write_mmd_indirect(priv, reg, MDIO_MMD_VEND2, val);

	mutex_unlock(&bus->mdio_lock);
}

static void
core_set(struct mt7530_priv *priv, u32 reg, u32 val)
{
	core_rmw(priv, reg, 0, val);
}

static void
core_clear(struct mt7530_priv *priv, u32 reg, u32 val)
{
	core_rmw(priv, reg, val, 0);
}

static int
mt7530_mii_write(struct mt7530_priv *priv, u32 reg, u32 val)
{
	struct mii_bus *bus = priv->bus;
	u16 page, r, lo, hi;
	int ret;

	page = (reg >> 6) & 0x3ff;
	r  = (reg >> 2) & 0xf;
	lo = val & 0xffff;
	hi = val >> 16;

	/* MT7530 uses 31 as the pseudo port */
	ret = bus->write(bus, 0x1f, 0x1f, page);
	if (ret < 0)
		goto err;

	ret = bus->write(bus, 0x1f, r,  lo);
	if (ret < 0)
		goto err;

	ret = bus->write(bus, 0x1f, 0x10, hi);
err:
	if (ret < 0)
		dev_err(&bus->dev,
			"failed to write mt7530 register\n");
	return ret;
}

static u32
mt7530_mii_read(struct mt7530_priv *priv, u32 reg)
{
	struct mii_bus *bus = priv->bus;
	u16 page, r, lo, hi;
	int ret;

	page = (reg >> 6) & 0x3ff;
	r = (reg >> 2) & 0xf;

	/* MT7530 uses 31 as the pseudo port */
	ret = bus->write(bus, 0x1f, 0x1f, page);
	if (ret < 0) {
		dev_err(&bus->dev,
			"failed to read mt7530 register\n");
		return ret;
	}

	lo = bus->read(bus, 0x1f, r);
	hi = bus->read(bus, 0x1f, 0x10);

	return (hi << 16) | (lo & 0xffff);
}

static void
mt7530_write(struct mt7530_priv *priv, u32 reg, u32 val)
{
	struct mii_bus *bus = priv->bus;

	mutex_lock_nested(&bus->mdio_lock, MDIO_MUTEX_NESTED);

	mt7530_mii_write(priv, reg, val);

	mutex_unlock(&bus->mdio_lock);
}

static u32
_mt7530_read(struct mt7530_dummy_poll *p)
{
	struct mii_bus		*bus = p->priv->bus;
	u32 val;

	mutex_lock_nested(&bus->mdio_lock, MDIO_MUTEX_NESTED);

	val = mt7530_mii_read(p->priv, p->reg);

	mutex_unlock(&bus->mdio_lock);

	return val;
}

static u32
mt7530_read(struct mt7530_priv *priv, u32 reg)
{
	struct mt7530_dummy_poll p;

	INIT_MT7530_DUMMY_POLL(&p, priv, reg);
	return _mt7530_read(&p);
}

static void
mt7530_rmw(struct mt7530_priv *priv, u32 reg,
	   u32 mask, u32 set)
{
	struct mii_bus *bus = priv->bus;
	u32 val;

	mutex_lock_nested(&bus->mdio_lock, MDIO_MUTEX_NESTED);

	val = mt7530_mii_read(priv, reg);
	val &= ~mask;
	val |= set;
	mt7530_mii_write(priv, reg, val);

	mutex_unlock(&bus->mdio_lock);
}

static void
mt7530_set(struct mt7530_priv *priv, u32 reg, u32 val)
{
	mt7530_rmw(priv, reg, 0, val);
}

static void
mt7530_clear(struct mt7530_priv *priv, u32 reg, u32 val)
{
	mt7530_rmw(priv, reg, val, 0);
}

static int
mt7530_fdb_cmd(struct mt7530_priv *priv, enum mt7530_fdb_cmd cmd, u32 *rsp)
{
	u32 val;
	int ret;
	struct mt7530_dummy_poll p;

	/* Set the command operating upon the MAC address entries */
	val = ATC_BUSY | ATC_MAT(0) | cmd;
	mt7530_write(priv, MT7530_ATC, val);

	INIT_MT7530_DUMMY_POLL(&p, priv, MT7530_ATC);
	ret = readx_poll_timeout(_mt7530_read, &p, val,
				 !(val & ATC_BUSY), 20, 20000);
	if (ret < 0) {
		dev_err(priv->dev, "reset timeout\n");
		return ret;
	}

	/* Additional sanity for read command if the specified
	 * entry is invalid
	 */
	val = mt7530_read(priv, MT7530_ATC);
	if ((cmd == MT7530_FDB_READ) && (val & ATC_INVALID))
		return -EINVAL;

	if (rsp)
		*rsp = val;

	return 0;
}

static void
mt7530_fdb_read(struct mt7530_priv *priv, struct mt7530_fdb *fdb)
{
	u32 reg[3];
	int i;

	/* Read from ARL table into an array */
	for (i = 0; i < 3; i++) {
		reg[i] = mt7530_read(priv, MT7530_TSRA1 + (i * 4));

		dev_dbg(priv->dev, "%s(%d) reg[%d]=0x%x\n",
			__func__, __LINE__, i, reg[i]);
	}

	fdb->vid = (reg[1] >> CVID) & CVID_MASK;
	fdb->aging = (reg[2] >> AGE_TIMER) & AGE_TIMER_MASK;
	fdb->port_mask = (reg[2] >> PORT_MAP) & PORT_MAP_MASK;
	fdb->mac[0] = (reg[0] >> MAC_BYTE_0) & MAC_BYTE_MASK;
	fdb->mac[1] = (reg[0] >> MAC_BYTE_1) & MAC_BYTE_MASK;
	fdb->mac[2] = (reg[0] >> MAC_BYTE_2) & MAC_BYTE_MASK;
	fdb->mac[3] = (reg[0] >> MAC_BYTE_3) & MAC_BYTE_MASK;
	fdb->mac[4] = (reg[1] >> MAC_BYTE_4) & MAC_BYTE_MASK;
	fdb->mac[5] = (reg[1] >> MAC_BYTE_5) & MAC_BYTE_MASK;
	fdb->noarp = ((reg[2] >> ENT_STATUS) & ENT_STATUS_MASK) == STATIC_ENT;
}

static void
mt7530_fdb_write(struct mt7530_priv *priv, u16 vid,
		 u8 port_mask, const u8 *mac,
		 u8 aging, u8 type)
{
	u32 reg[3] = { 0 };
	int i;

	reg[1] |= vid & CVID_MASK;
	reg[2] |= (aging & AGE_TIMER_MASK) << AGE_TIMER;
	reg[2] |= (port_mask & PORT_MAP_MASK) << PORT_MAP;
	/* STATIC_ENT indicate that entry is static wouldn't
	 * be aged out and STATIC_EMP specified as erasing an
	 * entry
	 */
	reg[2] |= (type & ENT_STATUS_MASK) << ENT_STATUS;
	reg[1] |= mac[5] << MAC_BYTE_5;
	reg[1] |= mac[4] << MAC_BYTE_4;
	reg[0] |= mac[3] << MAC_BYTE_3;
	reg[0] |= mac[2] << MAC_BYTE_2;
	reg[0] |= mac[1] << MAC_BYTE_1;
	reg[0] |= mac[0] << MAC_BYTE_0;

	/* Write array into the ARL table */
	for (i = 0; i < 3; i++)
		mt7530_write(priv, MT7530_ATA1 + (i * 4), reg[i]);
}

static int
mt7530_pad_clk_setup(struct dsa_switch *ds, int mode)
{
	struct mt7530_priv *priv = ds->priv;
	u32 ncpo1, ssc_delta, trgint, i;

	switch (mode) {
	case PHY_INTERFACE_MODE_RGMII:
		trgint = 0;
		ncpo1 = 0x0c80;
		ssc_delta = 0x87;
		break;
	case PHY_INTERFACE_MODE_TRGMII:
		trgint = 1;
		ncpo1 = 0x1400;
		ssc_delta = 0x57;
		break;
	default:
		dev_err(priv->dev, "xMII mode %d not supported\n", mode);
		return -EINVAL;
	}

	mt7530_rmw(priv, MT7530_P6ECR, P6_INTF_MODE_MASK,
		   P6_INTF_MODE(trgint));

	/* Lower Tx Driving for TRGMII path */
	for (i = 0 ; i < NUM_TRGMII_CTRL ; i++)
		mt7530_write(priv, MT7530_TRGMII_TD_ODT(i),
			     TD_DM_DRVP(8) | TD_DM_DRVN(8));

	/* Setup core clock for MT7530 */
	if (!trgint) {
		/* Disable MT7530 core clock */
		core_clear(priv, CORE_TRGMII_GSW_CLK_CG, REG_GSWCK_EN);

		/* Disable PLL, since phy_device has not yet been created
		 * provided for phy_[read,write]_mmd_indirect is called, we
		 * provide our own core_write_mmd_indirect to complete this
		 * function.
		 */
		core_write_mmd_indirect(priv,
					CORE_GSWPLL_GRP1,
					MDIO_MMD_VEND2,
					0);

		/* Set core clock into 500Mhz */
		core_write(priv, CORE_GSWPLL_GRP2,
			   RG_GSWPLL_POSDIV_500M(1) |
			   RG_GSWPLL_FBKDIV_500M(25));

		/* Enable PLL */
		core_write(priv, CORE_GSWPLL_GRP1,
			   RG_GSWPLL_EN_PRE |
			   RG_GSWPLL_POSDIV_200M(2) |
			   RG_GSWPLL_FBKDIV_200M(32));

		/* Enable MT7530 core clock */
		core_set(priv, CORE_TRGMII_GSW_CLK_CG, REG_GSWCK_EN);
	}

	/* Setup the MT7530 TRGMII Tx Clock */
	core_set(priv, CORE_TRGMII_GSW_CLK_CG, REG_GSWCK_EN);
	core_write(priv, CORE_PLL_GROUP5, RG_LCDDS_PCW_NCPO1(ncpo1));
	core_write(priv, CORE_PLL_GROUP6, RG_LCDDS_PCW_NCPO0(0));
	core_write(priv, CORE_PLL_GROUP10, RG_LCDDS_SSC_DELTA(ssc_delta));
	core_write(priv, CORE_PLL_GROUP11, RG_LCDDS_SSC_DELTA1(ssc_delta));
	core_write(priv, CORE_PLL_GROUP4,
		   RG_SYSPLL_DDSFBK_EN | RG_SYSPLL_BIAS_EN |
		   RG_SYSPLL_BIAS_LPF_EN);
	core_write(priv, CORE_PLL_GROUP2,
		   RG_SYSPLL_EN_NORMAL | RG_SYSPLL_VODEN |
		   RG_SYSPLL_POSDIV(1));
	core_write(priv, CORE_PLL_GROUP7,
		   RG_LCDDS_PCW_NCPO_CHG | RG_LCCDS_C(3) |
		   RG_LCDDS_PWDB | RG_LCDDS_ISO_EN);
	core_set(priv, CORE_TRGMII_GSW_CLK_CG,
		 REG_GSWCK_EN | REG_TRGMIICK_EN);

	if (!trgint)
		for (i = 0 ; i < NUM_TRGMII_CTRL; i++)
			mt7530_rmw(priv, MT7530_TRGMII_RD(i),
				   RD_TAP_MASK, RD_TAP(16));
	else
		mt7623_trgmii_set(priv, GSW_INTF_MODE, INTF_MODE_TRGMII);

	return 0;
}

static int
mt7623_pad_clk_setup(struct dsa_switch *ds)
{
	struct mt7530_priv *priv = ds->priv;
	int i;

	for (i = 0 ; i < NUM_TRGMII_CTRL; i++)
		mt7623_trgmii_write(priv, GSW_TRGMII_TD_ODT(i),
				    TD_DM_DRVP(8) | TD_DM_DRVN(8));

	mt7623_trgmii_set(priv, GSW_TRGMII_RCK_CTRL, RX_RST | RXC_DQSISEL);
	mt7623_trgmii_clear(priv, GSW_TRGMII_RCK_CTRL, RX_RST);

	return 0;
}

static void
mt7530_mib_reset(struct dsa_switch *ds)
{
	struct mt7530_priv *priv = ds->priv;

	mt7530_write(priv, MT7530_MIB_CCR, CCR_MIB_FLUSH);
	mt7530_write(priv, MT7530_MIB_CCR, CCR_MIB_ACTIVATE);
}

static void
mt7530_port_set_status(struct mt7530_priv *priv, int port, int enable)
{
	u32 mask = PMCR_TX_EN | PMCR_RX_EN;

	if (enable)
		mt7530_set(priv, MT7530_PMCR_P(port), mask);
	else
		mt7530_clear(priv, MT7530_PMCR_P(port), mask);
}

static int mt7530_phy_read(struct dsa_switch *ds, int port, int regnum)
{
	struct mt7530_priv *priv = ds->priv;

	return mdiobus_read_nested(priv->bus, port, regnum);
}

static int mt7530_phy_write(struct dsa_switch *ds, int port, int regnum,
			    u16 val)
{
	struct mt7530_priv *priv = ds->priv;

	return mdiobus_write_nested(priv->bus, port, regnum, val);
}

static void
mt7530_get_strings(struct dsa_switch *ds, int port, uint8_t *data)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(mt7530_mib); i++)
		strncpy(data + i * ETH_GSTRING_LEN, mt7530_mib[i].name,
			ETH_GSTRING_LEN);
}

static void
mt7530_get_ethtool_stats(struct dsa_switch *ds, int port,
			 uint64_t *data)
{
	struct mt7530_priv *priv = ds->priv;
	const struct mt7530_mib_desc *mib;
	u32 reg, i;
	u64 hi;

	for (i = 0; i < ARRAY_SIZE(mt7530_mib); i++) {
		mib = &mt7530_mib[i];
		reg = MT7530_PORT_MIB_COUNTER(port) + mib->offset;

		data[i] = mt7530_read(priv, reg);
		if (mib->size == 2) {
			hi = mt7530_read(priv, reg + 4);
			data[i] |= hi << 32;
		}
	}
}

static int
mt7530_get_sset_count(struct dsa_switch *ds)
{
	return ARRAY_SIZE(mt7530_mib);
}

static void mt7530_adjust_link(struct dsa_switch *ds, int port,
			       struct phy_device *phydev)
{
	struct mt7530_priv *priv = ds->priv;

	if (phy_is_pseudo_fixed_link(phydev)) {
		dev_dbg(priv->dev, "phy-mode for master device = %x\n",
			phydev->interface);

		/* Setup TX circuit incluing relevant PAD and driving */
		mt7530_pad_clk_setup(ds, phydev->interface);

		/* Setup RX circuit, relevant PAD and driving on the host
		 * which must be placed after the setup on the device side is
		 * all finished.
		 */
		mt7623_pad_clk_setup(ds);
	} else {
		u16 lcl_adv = 0, rmt_adv = 0;
		u8 flowctrl;
		u32 mcr = PMCR_USERP_LINK | PMCR_FORCE_MODE;

		switch (phydev->speed) {
		case SPEED_1000:
			mcr |= PMCR_FORCE_SPEED_1000;
			break;
		case SPEED_100:
			mcr |= PMCR_FORCE_SPEED_100;
			break;
		};

		if (phydev->link)
			mcr |= PMCR_FORCE_LNK;

		if (phydev->duplex) {
			mcr |= PMCR_FORCE_FDX;

			if (phydev->pause)
				rmt_adv = LPA_PAUSE_CAP;
			if (phydev->asym_pause)
				rmt_adv |= LPA_PAUSE_ASYM;

			if (phydev->advertising & ADVERTISED_Pause)
				lcl_adv |= ADVERTISE_PAUSE_CAP;
			if (phydev->advertising & ADVERTISED_Asym_Pause)
				lcl_adv |= ADVERTISE_PAUSE_ASYM;

			flowctrl = mii_resolve_flowctrl_fdx(lcl_adv, rmt_adv);

			if (flowctrl & FLOW_CTRL_TX)
				mcr |= PMCR_TX_FC_EN;
			if (flowctrl & FLOW_CTRL_RX)
				mcr |= PMCR_RX_FC_EN;
		}
		mt7530_write(priv, MT7530_PMCR_P(port), mcr);
	}
}

static inline bool dsa_is_user_port(struct dsa_switch *ds, int p)
{
	return ds->phys_port_mask & (1 << p) &&
		!dsa_is_cpu_port(ds, p) && !dsa_is_dsa_port(ds, p);
}

static inline u32 dsa_user_ports(struct dsa_switch *ds)
{
	u32 mask = 0;
	int p;

	for (p = 0; p < DSA_MAX_PORTS; p++)
		if (dsa_is_user_port(ds, p))
			mask |= BIT(p);

	return mask;
}

static int
mt7530_cpu_port_enable(struct mt7530_priv *priv,
		       int port)
{
	/* Enable Mediatek header mode on the cpu port */
	mt7530_write(priv, MT7530_PVC_P(port),
		     PORT_SPEC_TAG);

	/* Setup the MAC by default for the cpu port */
	mt7530_write(priv, MT7530_PMCR_P(port), PMCR_CPUP_LINK);

	/* Disable auto learning on the cpu port */
	mt7530_set(priv, MT7530_PSC_P(port), SA_DIS);

	/* Unknown unicast frame fordwarding to the cpu port */
	mt7530_set(priv, MT7530_MFC, UNU_FFP(BIT(port)));

	/* CPU port gets connected to all user ports of
	 * the switch
	 */
	mt7530_write(priv, MT7530_PCR_P(port),
		     PCR_MATRIX(dsa_user_ports(priv->ds)));

	return 0;
}

static int
mt7530_port_enable(struct dsa_switch *ds, int port,
		   struct phy_device *phy)
{
	struct mt7530_priv *priv = ds->priv;

	mutex_lock(&priv->reg_mutex);

	/* Setup the MAC for the user port */
	mt7530_write(priv, MT7530_PMCR_P(port), PMCR_USERP_LINK);

	/* Allow the user port gets connected to the cpu port and also
	 * restore the port matrix if the port is the member of a certain
	 * bridge.
	 */
	priv->ports[port].pm |= PCR_MATRIX(BIT(MT7530_CPU_PORT));
	priv->ports[port].enable = true;
	mt7530_rmw(priv, MT7530_PCR_P(port), PCR_MATRIX_MASK,
		   priv->ports[port].pm);
	mt7530_port_set_status(priv, port, 1);

	mutex_unlock(&priv->reg_mutex);

	return 0;
}

static void
mt7530_port_disable(struct dsa_switch *ds, int port,
		    struct phy_device *phy)
{
	struct mt7530_priv *priv = ds->priv;

	mutex_lock(&priv->reg_mutex);

	/* Clear up all port matrix which could be restored in the next
	 * enablement for the port.
	 */
	priv->ports[port].enable = false;
	mt7530_rmw(priv, MT7530_PCR_P(port), PCR_MATRIX_MASK,
		   PCR_MATRIX_CLR);
	mt7530_port_set_status(priv, port, 0);

	mutex_unlock(&priv->reg_mutex);
}

static int
mt7530_stp_state_set(struct dsa_switch *ds, int port, u8 state)
{
	struct mt7530_priv *priv = ds->priv;
	u32 stp_state;

	switch (state) {
	case BR_STATE_DISABLED:
		stp_state = MT7530_STP_DISABLED;
		break;
	case BR_STATE_BLOCKING:
		stp_state = MT7530_STP_BLOCKING;
		break;
	case BR_STATE_LISTENING:
		stp_state = MT7530_STP_LISTENING;
		break;
	case BR_STATE_LEARNING:
		stp_state = MT7530_STP_LEARNING;
		break;
	case BR_STATE_FORWARDING:
	default:
		stp_state = MT7530_STP_FORWARDING;
		break;
	}

	mt7530_rmw(priv, MT7530_SSP_P(port), FID_PST_MASK, stp_state);

	return 0;
}

static int
mt7530_port_bridge_join(struct dsa_switch *ds, int port,
			u32 br_port_mask)
{
	struct mt7530_priv *priv = ds->priv;
	u32 port_bitmap = BIT(MT7530_CPU_PORT);
	int i;

	mutex_lock(&priv->reg_mutex);

	for (i = 0; i < MT7530_NUM_PORTS; i++) {
		/* Add this port to the port matrix of the other ports in the
		 * same bridge. If the port is disabled, port matrix is kept
		 * and not being setup until the port becomes enabled.
		 */
		if (dsa_is_user_port(ds, i) && i != port) {
			if (!(br_port_mask & BIT(i)))
				continue;
			if (priv->ports[i].enable)
				mt7530_set(priv, MT7530_PCR_P(i),
					   PCR_MATRIX(BIT(port)));
			priv->ports[i].pm |= PCR_MATRIX(BIT(port));

			port_bitmap |= BIT(i);
		}
	}

	/* Add the all other ports to this port matrix. */
	if (priv->ports[port].enable)
		mt7530_rmw(priv, MT7530_PCR_P(port),
			   PCR_MATRIX_MASK, PCR_MATRIX(port_bitmap));
	priv->ports[port].pm |= PCR_MATRIX(port_bitmap);

	mutex_unlock(&priv->reg_mutex);

	return 0;
}

static int
mt7530_port_bridge_leave(struct dsa_switch *ds, int port,
			 u32 br_port_mask)
{
	struct mt7530_priv *priv = ds->priv;
	int i;

	mutex_lock(&priv->reg_mutex);

	for (i = 0; i < MT7530_NUM_PORTS; i++) {
		/* Remove this port from the port matrix of the other ports
		 * in the same bridge. If the port is disabled, port matrix
		 * is kept and not being setup until the port becomes enabled.
		 */
		if (dsa_is_user_port(ds, i) && i != port) {
			if (!(br_port_mask & BIT(i)))
				continue;
			if (priv->ports[i].enable)
				mt7530_clear(priv, MT7530_PCR_P(i),
					     PCR_MATRIX(BIT(port)));
			priv->ports[i].pm &= ~PCR_MATRIX(BIT(port));
		}
	}

	/* Set the cpu port to be the only one in the port matrix of
	 * this port.
	 */
	if (priv->ports[port].enable)
		mt7530_rmw(priv, MT7530_PCR_P(port), PCR_MATRIX_MASK,
			   PCR_MATRIX(BIT(MT7530_CPU_PORT)));
	priv->ports[port].pm = PCR_MATRIX(BIT(MT7530_CPU_PORT));

	mutex_unlock(&priv->reg_mutex);

	return 0;
}

static int
mt7530_port_fdb_prepare(struct dsa_switch *ds, int port,
			const struct switchdev_obj_port_fdb *fdb,
			struct switchdev_trans *trans)
{
	/* We do not need to do anything specific here yet */
	return 0;
}

static int
mt7530_port_fdb_add(struct dsa_switch *ds, int port,
		    const struct switchdev_obj_port_fdb *fdb,
		    struct switchdev_trans *trans)
{
	struct mt7530_priv *priv = ds->priv;
	int ret;
	u8 port_mask = BIT(port);

	mutex_lock(&priv->reg_mutex);
	mt7530_fdb_write(priv, fdb->vid, port_mask, fdb->addr, -1, STATIC_ENT);
	ret = mt7530_fdb_cmd(priv, MT7530_FDB_WRITE, 0);
	mutex_unlock(&priv->reg_mutex);

	return ret;
}

static int
mt7530_port_fdb_del(struct dsa_switch *ds, int port,
		    const struct switchdev_obj_port_fdb *fdb)
{
	struct mt7530_priv *priv = ds->priv;
	int ret;
	u8 port_mask = BIT(port);

	mutex_lock(&priv->reg_mutex);
	mt7530_fdb_write(priv, fdb->vid, port_mask, fdb->addr, -1, STATIC_EMP);
	ret = mt7530_fdb_cmd(priv, MT7530_FDB_WRITE, 0);
	mutex_unlock(&priv->reg_mutex);

	return ret;
}

static int
mt7530_port_fdb_copy(struct mt7530_fdb *mt_fdb,
		     struct switchdev_obj_port_fdb *fdb,
		     int (*cb)(struct switchdev_obj *obj))
{
	ether_addr_copy(fdb->addr, mt_fdb->mac);
	fdb->vid = mt_fdb->vid;
	fdb->ndm_state = mt_fdb->noarp ? NUD_NOARP : NUD_REACHABLE;

	return cb(&fdb->obj);
}

static int
mt7530_port_fdb_dump(struct dsa_switch *ds, int port,
		     struct switchdev_obj_port_fdb *fdb,
		     int (*cb)(struct switchdev_obj *obj))
{
	struct mt7530_priv *priv = ds->priv;
	struct mt7530_fdb _fdb = { 0 };
	int cnt = MT7530_NUM_FDB_RECORDS;
	int ret = 0;
	u32 rsp = 0;

	mutex_lock(&priv->reg_mutex);

	ret = mt7530_fdb_cmd(priv, MT7530_FDB_START, &rsp);
	if (ret < 0)
		goto err;

	do {
		if (rsp & ATC_SRCH_HIT) {
			mt7530_fdb_read(priv, &_fdb);
			if (_fdb.port_mask & BIT(port)) {
				ret = mt7530_port_fdb_copy(&_fdb, fdb, cb);
				if (ret < 0)
					break;
			}
		}
	} while (--cnt &&
		 !(rsp & ATC_SRCH_END) &&
		 !mt7530_fdb_cmd(priv, MT7530_FDB_NEXT, &rsp));
err:
	mutex_unlock(&priv->reg_mutex);

	return 0;
}

static int
mt7530_setup(struct dsa_switch *ds)
{
	struct mt7530_priv *priv = ds->priv;
	int ret, i;
	u32 id, val;
	struct device_node *dn;
	struct mt7530_dummy_poll p;

	priv->ds = ds;

	/* The parent node of master netdev which holds the common system
	 * controller also is the container for two GMACs nodes representing
	 * as two netdev instances.
	 */
	dn = of_get_parent(ds->master_dev->of_node);
	priv->ethernet = syscon_node_to_regmap(dn);
	if (IS_ERR(priv->ethernet))
		return PTR_ERR(priv->ethernet);

	regulator_set_voltage(priv->core_pwr, 1000000, 1000000);
	ret = regulator_enable(priv->core_pwr);
	if (ret < 0) {
		dev_err(priv->dev,
			"Failed to enable core power: %d\n", ret);
		return ret;
	}

	regulator_set_voltage(priv->io_pwr, 3300000, 3300000);
	ret = regulator_enable(priv->io_pwr);
	if (ret < 0) {
		dev_err(priv->dev, "Failed to enable io pwr: %d\n",
			ret);
		return ret;
	}

	/* Reset whole chip through gpio pin or memory-mapped registers for
	 * different type of hardware
	 */
	if (priv->mcm) {
		reset_control_assert(priv->rstc);
		usleep_range(1000, 1100);
		reset_control_deassert(priv->rstc);
	} else {
		gpiod_set_value_cansleep(priv->reset, 0);
		usleep_range(1000, 1100);
		gpiod_set_value_cansleep(priv->reset, 1);
	}

	/* Waiting for MT7530 got to stable */
	INIT_MT7530_DUMMY_POLL(&p, priv, MT7530_HWTRAP);
	ret = readx_poll_timeout(_mt7530_read, &p, val, val != 0,
				 20, 1000000);
	if (ret < 0) {
		dev_err(priv->dev, "reset timeout\n");
		return ret;
	}

	id = mt7530_read(priv, MT7530_CREV);
	id >>= CHIP_NAME_SHIFT;
	if (id != MT7530_ID) {
		dev_err(priv->dev, "chip %x can't be supported\n", id);
		return -ENODEV;
	}

	/* Reset the switch through internal reset */
	mt7530_write(priv, MT7530_SYS_CTRL,
		     SYS_CTRL_PHY_RST | SYS_CTRL_SW_RST |
		     SYS_CTRL_REG_RST);

	/* Enable Port 6 only; P5 as GMAC5 which currently is not supported */
	val = mt7530_read(priv, MT7530_MHWTRAP);
	val &= ~MHWTRAP_P6_DIS & ~MHWTRAP_PHY_ACCESS;
	val |= MHWTRAP_MANUAL;
	mt7530_write(priv, MT7530_MHWTRAP, val);

	/* Enable and reset MIB counters */
	mt7530_mib_reset(ds);

	mt7530_clear(priv, MT7530_MFC, UNU_FFP_MASK);

	for (i = 0; i < MT7530_NUM_PORTS; i++) {
		/* Disable forwarding by default on all ports */
		mt7530_rmw(priv, MT7530_PCR_P(i), PCR_MATRIX_MASK,
			   PCR_MATRIX_CLR);

		if (dsa_is_cpu_port(ds, i))
			mt7530_cpu_port_enable(priv, i);
		else
			mt7530_port_disable(ds, i, NULL);
	}

	/* Flush the FDB table */
	ret = mt7530_fdb_cmd(priv, MT7530_FDB_FLUSH, 0);
	if (ret < 0)
		return ret;

	return 0;
}

static int
mt7530_set_addr(struct dsa_switch *ds, u8 *addr)
{
	return 0;
}

static char*
mt7530_probe(struct device *dsa_dev, struct device *host_dev,
	     int sw_addr, void **_priv)
{
	struct mt7530_priv *priv;
	struct device_node *dn;
	struct mii_bus *bus = dsa_host_dev_to_mii_bus(host_dev);

	dn = dsa_dev->of_node;

	priv = devm_kzalloc(dsa_dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return NULL;

	/* Use medatek,mcm property to distinguish hardware type that would
	 * casues a little bit differences on power-on sequence.
	 */
	priv->mcm = of_property_read_bool(dn, "mediatek,mcm");
	if (priv->mcm) {
		dev_info(dsa_dev, "MT7530 adapts as multi-chip module\n");

		priv->rstc = devm_reset_control_get(dsa_dev, "mcm");
		if (IS_ERR(priv->rstc)) {
			dev_err(dsa_dev, "Couldn't get our reset line\n");
			return NULL;
		}
	}

	priv->core_pwr = devm_regulator_get(dsa_dev, "core");
	if (IS_ERR(priv->core_pwr))
		return NULL;

	priv->io_pwr = devm_regulator_get(dsa_dev, "io");
	if (IS_ERR(priv->io_pwr))
		return NULL;

	/* Not MCM that indicates switch works as the remote standalone
	 * integrated circuit so the GPIO pin would be used to complete
	 * the reset, otherwise memory-mapped register accessing used
	 * through syscon provides in the case of MCM.
	 */
	if (!priv->mcm) {
		priv->reset = devm_gpiod_get_optional(dsa_dev, "reset",
						      GPIOD_OUT_LOW);
		if (IS_ERR(priv->reset)) {
			dev_err(dsa_dev, "Couldn't get our reset line\n");
			return NULL;
		}
	}

	priv->bus = bus;
	priv->dev = dsa_dev;

	mutex_init(&priv->reg_mutex);

	*_priv = priv;
	mt7530_data = priv;

	return "MT7530 switch";
}

static void
mt7530_remove(struct mt7530_priv *priv)
{
	int ret = 0;

	ret = regulator_disable(priv->core_pwr);
	if (ret < 0)
		dev_err(priv->dev,
			"Failed to disable core power: %d\n", ret);

	ret = regulator_disable(priv->io_pwr);
	if (ret < 0)
		dev_err(priv->dev, "Failed to disable io pwr: %d\n",
			ret);

	mutex_destroy(&priv->reg_mutex);
}

static struct dsa_switch_driver mt7530_switch_driver = {
	.tag_protocol		= DSA_TAG_PROTO_MTK,
	.probe			= mt7530_probe,
	.setup			= mt7530_setup,
	.set_addr		= mt7530_set_addr,
	.get_strings		= mt7530_get_strings,
	.phy_read		= mt7530_phy_read,
	.phy_write		= mt7530_phy_write,
	.get_ethtool_stats	= mt7530_get_ethtool_stats,
	.get_sset_count		= mt7530_get_sset_count,
	.adjust_link		= mt7530_adjust_link,
	.port_enable		= mt7530_port_enable,
	.port_disable		= mt7530_port_disable,
	.port_stp_update	= mt7530_stp_state_set,
	.port_join_bridge	= mt7530_port_bridge_join,
	.port_leave_bridge	= mt7530_port_bridge_leave,
	.port_fdb_prepare	= mt7530_port_fdb_prepare,
	.port_fdb_add		= mt7530_port_fdb_add,
	.port_fdb_del		= mt7530_port_fdb_del,
	.port_fdb_dump		= mt7530_port_fdb_dump,
};

static int __init mt7530_init(void)
{
	register_switch_driver(&mt7530_switch_driver);

	return 0;
}
module_init(mt7530_init);

static void __exit mt7530_exit(void)
{
	unregister_switch_driver(&mt7530_switch_driver);

	mt7530_remove(mt7530_data);
}
module_exit(mt7530_exit);

MODULE_AUTHOR("Sean Wang <sean.wang@mediatek.com>");
MODULE_DESCRIPTION("Driver for Mediatek MT7530 Switch");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:mediatek-mt7530");
