/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef __MTK_TS_CPU_H__
#define __MTK_TS_CPU_H__


extern int bts_cur_temp;
extern u32 get_devinfo_with_index(u32 index);

extern int get_immediate_temp2_wrap(void);
extern void mtkts_dump_cali_info(void);

int mtktsbattery_register_thermal(void);
void mtktsbattery_unregister_thermal(void);

int tsallts_register_thermal(void);
void tsallts_unregister_thermal(void);


extern int IMM_IsAdcInitReady(void);
extern int IMM_GetOneChannelValue(int dwChannel, int data[4], int *rawdata);


/*mtk_ts_pa_thput.c*/
extern struct proc_dir_entry *mtk_thermal_get_proc_drv_therm_dir_entry(void);
extern bool is_meta_mode(void);
extern bool is_advanced_meta_mode(void);

/* mtk_ts_pmic6323.c */
extern int PMIC_IMM_GetOneChannelValue(int dwChannel, int deCount, int trimd);
int mtktspmic_register_thermal(void);
void mtktspmic_unregister_thermal(void);

/* mtk_thermal_platform.c */
extern int mtktscpu_limited_dmips;
#if defined(CONFIG_MTK_SMART_BATTERY)
/* global variable from battery driver... */
extern kal_bool gFG_Is_Charging;
#endif
extern unsigned int mt_gpufreq_get_cur_freq(void);

#endif				/* __MTK_TS_CPU_H__ */
