/* Copyright  2016 MediaTek Inc.
 * Author: Carlos Huang <carlos.huang@mediatek.com>
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
#include <linux/version.h>
#include <linux/netdevice.h>

#include <linux/kernel.h>
#include <linux/sched.h>

#include "raether.h"

extern struct net_device *dev_raether;

#define PHY_CONTROL_0		0x0004
#define MDIO_PHY_CONTROL_0	(RALINK_ETH_MAC_BASE + PHY_CONTROL_0)
#define enable_mdio(x)

