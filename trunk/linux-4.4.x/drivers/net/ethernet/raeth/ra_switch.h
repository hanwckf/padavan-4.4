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
#ifndef RA_SWITCH_H
#define RA_SWITCH_H

#include  "./rtl8367c/include/rtk_switch.h"
#include  "./rtl8367c/include/rtk_hal.h"
#include  "./rtl8367c/include/port.h"
#include  "./rtl8367c/include/vlan.h"
#include  "./rtl8367c/include/rtl8367c_asicdrv_port.h"

extern struct net_device *dev_raether;
#define ANACAL_INIT		0x01
#define ANACAL_ERROR		0xFD
#define ANACAL_SATURATION	0xFE
#define	ANACAL_FINISH		0xFF
#define ANACAL_PAIR_A		0
#define ANACAL_PAIR_B		1
#define ANACAL_PAIR_C		2
#define ANACAL_PAIR_D		3
#define DAC_IN_0V		0x000
#define DAC_IN_2V		0x0f0
#define TX_AMP_OFFSET_0MV	0x20
#define TX_AMP_OFFSET_VALID_BITS	6
#define FE_CAL_P0			0
#define FE_CAL_P1			1
#if defined(CONFIG_MACH_LEOPARD)
#define FE_CAL_COMMON			1
#else
#define FE_CAL_COMMON			0
#endif

void fe_sw_init(void);
void fe_sw_preinit(struct END_DEVICE *ei_local);
void fe_sw_deinit(struct END_DEVICE *ei_local);
void sw_ioctl(struct ra_switch_ioctl_data *ioctl_data);
irqreturn_t esw_interrupt(int irq, void *resv);
irqreturn_t gsw_interrupt(int irq, void *resv);

/* struct mtk_gsw -	the structure that holds the SoC specific data
 * @dev:		The Device struct
 * @base:		The base address
 * @piac_offset:	The PIAC base may change depending on SoC
 * @irq:		The IRQ we are using
 * @port4:		The port4 mode on MT7620
 * @autopoll:		Is MDIO autopolling enabled
 * @ethsys:		The ethsys register map
 * @pctl:		The pin control register map
 * @clk_trgpll:		The trgmii pll clock
 */
struct mtk_gsw {
	struct mtk_eth		*eth;
	struct device		*dev;
	void __iomem		*base;
	u32			piac_offset;
	int			irq;
	int			port4;
	unsigned long int	autopoll;

	struct regmap		*ethsys;
	struct regmap		*pctl;

	int			trgmii_force;
	bool			wllll;
	bool			mcm;
	struct pinctrl *pins;
	struct pinctrl_state *ps_default;
	struct pinctrl_state *ps_reset;
	int reset_pin;
	struct regulator *supply;
	struct regulator *b3v;
};

extern u8 fe_cal_flag;
extern u8 fe_cal_flag_mdix;
extern u8 fe_cal_tx_offset_flag;
extern u8 fe_cal_tx_offset_flag_mdix;
extern u8 fe_cal_r50_flag;
extern u8 fe_cal_vbg_flag;
void fe_cal_r50(u8 port_num, u32 delay);
void fe_cal_tx_amp(u8 port_num, u32 delay);
void fe_cal_tx_amp_mdix(u8 port_num, u32 delay);
void fe_cal_tx_offset(u8 port_num, u32 delay);
void fe_cal_tx_offset_mdix(u8 port_num, u32 delay);
void fe_cal_vbg(u8 port_num, u32 delay);
/*giga port calibration*/
void ge_cal_r50(u8 port_num, u32 delay);
void ge_cal_tx_amp(u8 port_num, u32 delay);
void ge_cal_tx_offset(u8 port_num, u32 delay);
void do_ge_phy_all_analog_cal(u8 phyaddr);
#endif
