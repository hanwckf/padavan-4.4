/* Copyright (c) 2011-2013, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __MT8521P_AFE_CLK_H__
#define __MT8521P_AFE_CLK_H__
extern int mt_i2s_power_on_mclk(int id, int on);
extern void mt_afe_a1sys_hp_ck_on(void);
extern void mt_afe_a2sys_hp_ck_on(void);
extern void mt_afe_f_apll_ck_on(unsigned int mux_select, unsigned int divider);
extern void mt_afe_a1sys_hp_ck_off(void);
extern void mt_afe_a2sys_hp_ck_off(void);
extern void mt_afe_f_apll_ck_off(void);
extern void mt_afe_unipll_clk_on(void);
extern void mt_afe_unipll_clk_off(void);
extern int mt_afe_init_clock(void *dev);
extern int mt_afe_deinit_clock(void *dev);
extern void mt_afe_main_clk_on(void);
extern void mt_afe_main_clk_off(void);
extern void mt_turn_on_i2sout_clock(int id, int on);
extern void mt_turn_on_i2sin_clock(int id, int on);
extern void mt_mclk_set(int id, int domain, int mclk);
extern void mt_afe_spdif_dir_clk_on(void);
extern void mt_afe_spdif_dir_clk_off(void);

#endif
