/* Copyright  2016 MediaTek Inc.
 * Author: Nelson Chang <nelson.chang@mediatek.com>
 * Author: Carlos Huang <carlos.huang@mediatek.com>
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
#ifndef RA_MAC_H
#define RA_MAC_H

void ra2880stop(struct END_DEVICE *ei_local);
void set_mac_address(unsigned char p[6]);
void set_mac2_address(unsigned char p[6]);
int str_to_ip(unsigned int *ip, const char *str);
void enable_auto_negotiate(struct END_DEVICE *ei_local);
void set_ge1_force_1000(void);
void set_ge2_force_1000(void);
void set_ge1_an(void);
void set_ge2_an(void);
void set_ge2_gmii(void);
void set_ge0_gmii(void);
void set_ge2_force_link_down(void);
#endif
