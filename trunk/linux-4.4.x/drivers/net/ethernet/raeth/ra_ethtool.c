/* Copyright  2016 MediaTek Inc.
 * Author: Harry Huang <harry.huang@mediatek.com>
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
#include "raether.h"
#include "ra_ethtool.h"

#define RAETHER_DRIVER_NAME		"raether"
#define RA_NUM_STATS			4

unsigned char get_current_phy_address(void)
{
	struct net_device *cur_dev_p;
	struct END_DEVICE *ei_local;

	cur_dev_p = dev_get_by_name(&init_net, DEV_NAME);
	if (!cur_dev_p)
		return 0;
	ei_local = netdev_priv(cur_dev_p);
	return ei_local->mii_info.phy_id;
}

#define MII_CR_ADDR			0x00
#define MII_CR_MR_AUTONEG_ENABLE	BIT(12)
#define MII_CR_MR_RESTART_NEGOTIATION	BIT(9)

#define AUTO_NEGOTIATION_ADVERTISEMENT	0x04
#define AN_PAUSE			BIT(10)

u32 et_get_link(struct net_device *dev)
{
	struct END_DEVICE *ei_local = netdev_priv(dev);

	return mii_link_ok(&ei_local->mii_info);
}

int et_get_settings(struct net_device *dev, struct ethtool_cmd *cmd)
{
	struct END_DEVICE *ei_local = netdev_priv(dev);

	mii_ethtool_gset(&ei_local->mii_info, cmd);
	return 0;
}

/* mii_mgr_read wrapper for mii.o ethtool */
int mdio_read(struct net_device *dev, int phy_id, int location)
{
	unsigned int result;
	struct END_DEVICE *ei_local = netdev_priv(dev);

	mii_mgr_read((unsigned int)ei_local->mii_info.phy_id,
		     (unsigned int)location, &result);
/* printk("\n%s mii.o query= phy_id:%d\n",dev->name, phy_id);*/
/*printk("address:%d retval:%x\n", location, result); */
	return (int)result;
}

/* mii_mgr_write wrapper for mii.o ethtool */
void mdio_write(struct net_device *dev, int phy_id, int location, int value)
{
	struct END_DEVICE *ei_local = netdev_priv(dev);

	mii_mgr_write((unsigned int)ei_local->mii_info.phy_id,
		      (unsigned int)location, (unsigned int)value);
/* printk("mii.o write= phy_id:%d\n", phy_id);*/
/*printk("address:%d value:%x\n", location, value); */
}

/* #ifdef CONFIG_PSEUDO_SUPPORT */
/*We unable to re-use the Raether functions because it is hard to tell
 * where the calling from is. From eth2 or eth3?
 *
 * These code size is around 950 bytes.
 */

u32 et_virt_get_link(struct net_device *dev)
{
	struct PSEUDO_ADAPTER *pseudo = netdev_priv(dev);
	struct END_DEVICE *ei_local = netdev_priv(dev_raether);

	if (ei_local->features & FE_GE2_SUPPORT)
		return mii_link_ok(&pseudo->mii_info);
	else
		return 0;
}

int et_virt_get_settings(struct net_device *dev, struct ethtool_cmd *cmd)
{
	struct PSEUDO_ADAPTER *pseudo = netdev_priv(dev);
	struct END_DEVICE *ei_local = netdev_priv(dev_raether);

	if (ei_local->features & FE_GE2_SUPPORT)
		mii_ethtool_gset(&pseudo->mii_info, cmd);
	return 0;
}

int mdio_virt_read(struct net_device *dev, int phy_id, int location)
{
	unsigned int result;
	struct PSEUDO_ADAPTER *pseudo = netdev_priv(dev);
	struct END_DEVICE *ei_local = netdev_priv(dev_raether);

	if (ei_local->features & FE_GE2_SUPPORT) {
		mii_mgr_read((unsigned int)pseudo->mii_info.phy_id,
			     (unsigned int)location, &result);
/* printk("%s mii.o query= phy_id:%d,\n", dev->name, phy_id); */
/*printk("address:%d retval:%d\n", location, result);*/
		return (int)result;
	} else {
		return 0;
	}
}

void mdio_virt_write(struct net_device *dev, int phy_id, int location,
		     int value)
{
	struct PSEUDO_ADAPTER *pseudo = netdev_priv(dev);
	struct END_DEVICE *ei_local = netdev_priv(dev_raether);

	if (ei_local->features & FE_GE2_SUPPORT) {
		mii_mgr_write((unsigned int)pseudo->mii_info.phy_id,
			      (unsigned int)location, (unsigned int)value);
	}

/* printk("mii.o write= phy_id:%d\n", phy_id);*/
/*printk("address:%d value:%d\n)", location, value); */
}

void ethtool_init(struct net_device *dev)
{
	struct END_DEVICE *ei_local = netdev_priv(dev);

	/* init mii structure */
	ei_local->mii_info.dev = dev;
	ei_local->mii_info.mdio_read = mdio_read;
	ei_local->mii_info.mdio_write = mdio_write;
	ei_local->mii_info.phy_id_mask = 0x1f;
	ei_local->mii_info.reg_num_mask = 0x1f;
	ei_local->mii_info.supports_gmii =
	    mii_check_gmii_support(&ei_local->mii_info);

	/* TODO:   phy_id: 0~4 */
	ei_local->mii_info.phy_id = 1;
}

void ethtool_virt_init(struct net_device *dev)
{
	struct PSEUDO_ADAPTER *p_pseudo_ad = netdev_priv(dev);

	/* init mii structure */
	p_pseudo_ad->mii_info.dev = dev;
	p_pseudo_ad->mii_info.mdio_read = mdio_virt_read;
	p_pseudo_ad->mii_info.mdio_write = mdio_virt_write;
	p_pseudo_ad->mii_info.phy_id_mask = 0x1f;
	p_pseudo_ad->mii_info.reg_num_mask = 0x1f;
	p_pseudo_ad->mii_info.phy_id = 0x1e;
	p_pseudo_ad->mii_info.supports_gmii =
	    mii_check_gmii_support(&p_pseudo_ad->mii_info);
}

