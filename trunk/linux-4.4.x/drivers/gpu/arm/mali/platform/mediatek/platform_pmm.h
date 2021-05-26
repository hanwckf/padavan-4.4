/*
* Copyright (c) 2015 MediaTek Inc.
* Author: Chiawen Lee <chiawen.lee@mediatek.com>
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
#ifndef __PLATFORM_PMM_H__
#define __PLATFORM_PMM_H__

struct mali_gpu_utilization_data;
struct mt_gpufreq_table_info;

struct mtk_mfg_device_data {
	struct mali_gpu_device_data mali_gpu_data;
	void *priv_data;
};

struct mfg_power_table {
	unsigned int hz;
	unsigned int u_volt;
	unsigned int power;
};

struct mfg_dvfs {
	bool		   is_enabled;
	unsigned int       update_frequency;
	int		   update_step;

	unsigned int	   total_trans;
	unsigned int	   *trans_table;
	unsigned long      *time_in_state;
	unsigned long	   last_stat_updated;
};

struct mfg_base {
	void __iomem *reg_base;
	struct platform_device *smi_pdev;
	struct clk *mfg_pll;
	struct clk *mfg_sel;
	struct regulator *vdd_g3d;

	struct clk *intermediate_pll;
	struct mfg_power_table *power_table;
	int    num_of_steps;
	int    current_step;
	unsigned int current_frequency;
	struct mutex set_frequency_lock;
	struct work_struct work;
	struct mfg_dvfs    *dvfs;

	struct mali_gpu_clk_item     *mali_item;
};

enum mali_power_mode {
	MALI_POWER_MODE_ON  = 0x1,
	MALI_POWER_MODE_DEEP_SLEEP,
	MALI_POWER_MODE_LIGHT_SLEEP,
};

int mali_clk_enable(struct device *device);
int mali_clk_disable(struct device *device);

void mali_mfg_get_clock_info(struct mtk_mfg_device_data *mfg,
			     struct mali_gpu_clock *data);
int mali_mfg_get_clock_step(struct mtk_mfg_device_data *mfg);
int mali_mfg_set_clock_step(struct mtk_mfg_device_data *mfg, int stp);


int mali_mtk_mfg_bind_resource(struct platform_device *device,
			       void **priv_data);
void mali_mtk_mfg_unbind_resource(struct platform_device *device);

/** @brief Platform power management specific GPU utilization handler
 *
 * When GPU utilization handler is enabled, this function will be
 * periodically called.
 *
 * @param utilization The Mali GPU's work loading from 0 ~ 256.
 * 0 = no utilization, 256 = full utilization.
 */
void mtk_mfg_apply_dvfs_policy(struct mtk_mfg_device_data *mfg_dev,
			       struct mali_gpu_utilization_data *data);

/* file : platform_dvfs_policy.c */
void mtk_gpufreq_dvfs_apply(int utilization, unsigned long *freq, int *step);
void mtk_gpufreq_dvfs_init(struct mfg_base *mfg_base);
void mtk_gpufreq_dvfs_deinit(void);
unsigned int mtk_gpufreq_dvfs_get_freq_level_count(void);
int mtk_gpufreq_dvfs_get_opp_by_level(unsigned int num, int *khz, int *volt,
					int *power);
int mtk_get_current_frequency_id(void);

/** @brief Platform power management initialisation of MALI
 *
 * This is called from the entrypoint of the driver to initialize the platform
 *
 */
int mali_pmm_init(struct platform_device *device);

/** @brief Platform power management deinitialisation of MALI
 *
 * This is called on the exit of the driver to terminate the platform
 *
 */
void mali_pmm_deinit(struct platform_device *device);

void mali_platform_power_mode_change(struct device *device,
				     enum mali_power_mode power_mode);

void mali_gpu_utilization_callback(struct mali_gpu_utilization_data *data);
#if defined(CONFIG_MALI_DVFS)
int  mali_gpu_set_freq(int setting_clock_step);
void mali_gpu_get_clock_info(struct mali_gpu_clock **data);
int  mali_gpu_get_freq(void);
#endif

#endif

