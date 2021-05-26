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

/**
 * @file mali_platform.c
 * Platform specific Mali driver functions for:
 * - Realview Versatile platforms with ARM11 Mpcore and virtex 5.
 * - Versatile Express platforms with ARM Cortex-A9 and virtex 6.
 */
#include <linux/platform_device.h>
#include <linux/version.h>
#include <linux/pm.h>
#include <linux/io.h>
#include <linux/mali/mali_utgard.h>
#include "mali_kernel_common.h"
#include <linux/dma-mapping.h>
#include <linux/moduleparam.h>

#include "arm_core_scaling.h"
#include "mali_executor.h"
#include "platform_pmm.h"
#ifdef CONFIG_PM_RUNTIME
#include <linux/pm_runtime.h>
#endif

static int mali_core_scaling_enable;

static struct mtk_mfg_device_data mtk_mfg = {
	.mali_gpu_data = {
#if defined(CONFIG_ARM64)
		.fb_start = 0x5f000000,
		.fb_size = 0x91000000,
#else
		.fb_start = 0x80000000,
		.fb_size = 0x80000000,
#endif
		.control_interval = 200, /* 200ms */
		.utilization_callback = mali_gpu_utilization_callback,
#if defined(CONFIG_MALI_DVFS)
		.get_clock_info = mali_gpu_get_clock_info,
		.get_freq = mali_gpu_get_freq,
		.set_freq = mali_gpu_set_freq,
#endif
	},
};


static int mali_pm_suspend(struct device *device)
{
	int ret = 0;

	MALI_DEBUG_PRINT(1, ("Mali PM:%s\n", __func__));

	if (NULL != device->driver && NULL != device->driver->pm
	    && NULL != device->driver->pm->suspend) {
		/* Need to notify Mali driver about this event */
		ret = device->driver->pm->suspend(device);
	}

	/* _mali_osk_pm_delete_callback_timer();*/
	mali_platform_power_mode_change(device, MALI_POWER_MODE_DEEP_SLEEP);

	return ret;
}

static int mali_pm_resume(struct device *device)
{
	int ret = 0;

	MALI_DEBUG_PRINT(1, ("Mali PM: %s\n", __func__));

	mali_platform_power_mode_change(device, MALI_POWER_MODE_ON);

	if (NULL != device->driver && NULL != device->driver->pm
	    && NULL != device->driver->pm->resume) {
		/* Need to notify Mali driver about this event */
		ret = device->driver->pm->resume(device);
	}

	return ret;
}

#ifdef CONFIG_PM_RUNTIME
static int mali_runtime_suspend(struct device *device)
{
	int ret = 0;

	MALI_DEBUG_PRINT(4, ("mali_runtime_suspend() called\n"));

	if (NULL != device->driver && NULL != device->driver->pm
	    && NULL != device->driver->pm->runtime_suspend) {
		/* Need to notify Mali driver about this event */
		ret = device->driver->pm->runtime_suspend(device);
	}

	mali_platform_power_mode_change(device, MALI_POWER_MODE_LIGHT_SLEEP);

	return ret;
}

static int mali_runtime_resume(struct device *device)
{
	int ret = 0;

	MALI_DEBUG_PRINT(4, ("mali_runtime_resume() called\n"));

	mali_platform_power_mode_change(device, MALI_POWER_MODE_ON);

	if (NULL != device->driver && NULL != device->driver->pm
	    && NULL != device->driver->pm->runtime_resume) {
		/* Need to notify Mali driver about this event */
		ret = device->driver->pm->runtime_resume(device);
	}

	return ret;
}

static int mali_runtime_idle(struct device *device)
{
	MALI_DEBUG_PRINT(4, ("mali_runtime_idle() called\n"));

	if (NULL != device->driver && NULL != device->driver->pm
	    && NULL != device->driver->pm->runtime_idle) {
		/* Need to notify Mali driver about this event */
		int ret = device->driver->pm->runtime_idle(device);

		if (ret != 0)
			return ret;
	}

	pm_runtime_suspend(device);

	return 0;
}
#endif /* CONFIG_PM_RUNTIME */

static const struct dev_pm_ops mali_gpu_device_type_pm_ops = {
	.suspend = mali_pm_suspend,
	.resume = mali_pm_resume,
	.freeze = mali_pm_suspend,
	.thaw = mali_pm_resume,
	.restore = mali_pm_resume,

#ifdef CONFIG_PM_RUNTIME
	.runtime_suspend = mali_runtime_suspend,
	.runtime_resume = mali_runtime_resume,
	.runtime_idle = mali_runtime_idle,
#endif
};

static struct device_type mali_gpu_device_device_type = {
	.pm = &mali_gpu_device_type_pm_ops,
};

int mali_platform_device_init(struct platform_device *device)
{
	int num_pp_cores = 4;
	int err = -1;

	err = mali_mtk_mfg_bind_resource(device, &mtk_mfg.priv_data);
	if (err)
		return err;

	device->dev.type = &mali_gpu_device_device_type;

	err = platform_device_add_data(device, &mtk_mfg.mali_gpu_data,
					sizeof(mtk_mfg));

	if (err == 0) {
		mali_pmm_init(device);
#ifdef CONFIG_PM_RUNTIME
			pm_runtime_set_autosuspend_delay(&(device->dev), 1000);
			pm_runtime_use_autosuspend(&(device->dev));
			pm_runtime_enable(&(device->dev));
#endif
		MALI_DEBUG_ASSERT(num_pp_cores > 0);
		mali_core_scaling_init(num_pp_cores);
	}

	return err;
}

int mali_platform_device_deinit(struct platform_device *device)
{
	MALI_DEBUG_PRINT(4, ("mali_platform_device_deinit() called\n"));

	mali_core_scaling_term();
	mali_pmm_deinit(device);
	mali_mtk_mfg_unbind_resource(device);
	return 0;
}

static int param_set_core_scaling(const char *val,
				const struct kernel_param *kp)
{
	int ret = param_set_int(val, kp);

	if (mali_core_scaling_enable == 1)
		mali_core_scaling_sync(mali_executor_get_num_cores_enabled());

	return ret;
}

static struct kernel_param_ops param_ops_core_scaling = {
	.set = param_set_core_scaling,
	.get = param_get_int,
};

module_param_cb(mali_core_scaling_enable, &param_ops_core_scaling,
		&mali_core_scaling_enable, 0644);
MODULE_PARM_DESC(mali_core_scaling_enable,
"1 means to enable core scaling policy, 0 means to disable it");

void mali_gpu_utilization_callback(struct mali_gpu_utilization_data *data)
{
	if (mali_core_scaling_enable == 1)
		mali_core_scaling_update(data);
}

#if defined(CONFIG_MALI_DVFS)
int  mali_gpu_set_freq(int setting_clock_step)
{
	return mali_mfg_set_clock_step(&mtk_mfg, setting_clock_step);
}

int  mali_gpu_get_freq(void)
{
	/* return clock_step */
	return mali_mfg_get_clock_step(&mtk_mfg);
}

/* default freq = 416 mHz, volt = 115000 uVolt */
static struct mali_gpu_clk_item clk_item[] = { {416, 1150000} };
static struct mali_gpu_clock mali_clock_info;

void mali_gpu_get_clock_info(struct mali_gpu_clock **data)
{
	mali_mfg_get_clock_info(&mtk_mfg, &mali_clock_info);
	if (!mali_clock_info.num_of_steps) {
		mali_clock_info.item = &clk_item[0];
		mali_clock_info.num_of_steps = 1;
	}

	*data = &mali_clock_info;
}
#endif /* CONFIG_MALI_DVFS */
