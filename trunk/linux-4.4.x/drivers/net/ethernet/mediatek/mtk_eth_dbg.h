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
 *   Copyright (C) 2009-2016 John Crispin <blogic@openwrt.org>
 *   Copyright (C) 2009-2016 Felix Fietkau <nbd@openwrt.org>
 *   Copyright (C) 2013-2016 Michael Lee <igvtee@gmail.com>
 */

#ifndef MTK_ETH_DBG_H
#define MTK_ETH_DBG_H

#ifdef CONFIG_NET_MEDIATEK_DBG
#define MTKETH_MII_READ                  0x89F3
#define MTKETH_MII_WRITE                 0x89F4
#define MTKETH_ESW_REG_READ              0x89F1
#define MTKETH_ESW_REG_WRITE             0x89F2
#define MTKETH_MII_READ_CL45             0x89FC
#define MTKETH_MII_WRITE_CL45            0x89FD
#define REG_ESW_MAX                     0xFC

struct mtk_esw_reg {
	unsigned int off;
	unsigned int val;
};

struct mtk_mii_ioctl_data {
	unsigned int phy_id;
	unsigned int reg_num;
	unsigned int val_in;
	unsigned int val_out;
	unsigned int port_num;
	unsigned int dev_addr;
	unsigned int reg_addr;
};

#if defined(CONFIG_NET_DSA_MT7530) || defined(CONFIG_MT753X_GSW)
static inline bool mt7530_exist(struct mtk_eth *eth)
{
	return true;
}
#else
static inline bool mt7530_exist(struct mtk_eth *eth)
{
	return false;
}
#endif

u32 _mtk_mdio_read(struct mtk_eth *eth, int phy_addr, int phy_reg);
u32 _mtk_mdio_write(struct mtk_eth *eth, u32 phy_addr,
		    u32 phy_register, u32 write_data);

u32 mtk_cl45_ind_read(struct mtk_eth *eth, u32 port, u32 devad, u32 reg, u32 *data);
u32 mtk_cl45_ind_write(struct mtk_eth *eth, u32 port, u32 devad, u32 reg, u32 data);

int debug_proc_init(struct mtk_eth *eth);
void debug_proc_exit(void);

int mtketh_debugfs_init(struct mtk_eth *eth);
void mtketh_debugfs_exit(struct mtk_eth *eth);
int mtk_do_priv_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd);
#else
int mtk_do_priv_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd)
{
	return 0;
}

int mtketh_debugfs_init(struct mtk_eth *eth)
{
	return 0;
}

void mtketh_debugfs_exit(struct mtk_eth *eth)
{
}

int debug_proc_init(struct mtk_eth *eth)
{
	return 0;
}

void debug_proc_exit(void)
{
}
#endif

#endif /* MTK_ETH_DBG_H */
