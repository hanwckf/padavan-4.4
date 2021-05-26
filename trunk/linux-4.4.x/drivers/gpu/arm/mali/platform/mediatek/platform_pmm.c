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
#include <linux/atomic.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/mali/mali_utgard.h>
#include <linux/mutex.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/pm_opp.h>
#include <linux/pm_runtime.h>
#include <linux/regulator/consumer.h>
#include <soc/mediatek/smi.h>

#include "arm_core_scaling.h"
#include "mali_executor.h"
#include "mali_kernel_common.h"
#include "mali_osk.h"
#include "mali_ukk.h"
#if defined(CONFIG_MALI400_PROFILING)
#include "mali_osk_profiling.h"
#endif
#include "platform_pmm.h"

atomic_t mfg_power_off;

#define REG_MFG_G3D BIT(0)

#define REG_MFG_CG_STA 0x00
#define REG_MFG_CG_SET 0x04
#define REG_MFG_CG_CLR 0x08

static void mtk_mfg_set_clock_gating(void __iomem *reg)
{
	writel(REG_MFG_G3D, reg + REG_MFG_CG_SET);
}

static void mtk_mfg_clr_clock_gating(void __iomem *reg)
{
	writel(REG_MFG_G3D, reg + REG_MFG_CG_CLR);
}

int mtk_mfg_set_clock_rate(struct mfg_base *mfg, unsigned long freq)
{
	int ret;

	mutex_lock(&mfg->set_frequency_lock);

	clk_prepare_enable(mfg->mfg_sel);

	ret = clk_set_parent(mfg->mfg_sel, mfg->intermediate_pll);
	if (ret) {
		pr_err("failed to select intermediate_pll\n");
		goto err_clk_disable;
	}

	ret = clk_set_rate(mfg->mfg_pll, freq);
	if (ret) {
		pr_err("failed to change gpu freq %lu\n", freq);
		goto err_clk_disable;
	}

	ret = clk_set_parent(mfg->mfg_sel, mfg->mfg_pll);
	if (ret) {
		pr_err("failed to select mfg_pll\n");
		goto err_clk_disable;
	}

err_clk_disable:
	clk_disable_unprepare(mfg->mfg_sel);
	mutex_unlock(&mfg->set_frequency_lock);

	return ret;
}

static void mtk_mfg_dvfs_handler(struct work_struct *work)
{
	static unsigned long exceed_time;
	struct mfg_base *mfg = container_of(work, struct mfg_base, work);
	struct mfg_dvfs *dvfs = mfg->dvfs;
	unsigned long cur_time = jiffies_to_msecs(jiffies);
	int prev_step = mfg->current_step;

	dvfs->time_in_state[prev_step] += cur_time - dvfs->last_stat_updated;
	if (mfg->current_frequency == dvfs->update_frequency)
		goto fini;

	mali_executor_suspend();
	if (!mtk_mfg_set_clock_rate(mfg, dvfs->update_frequency)) {
		mfg->current_step = dvfs->update_step;
		mfg->current_frequency = dvfs->update_frequency;
	}
	mali_executor_resume();

	if (exceed_time < (jiffies_to_msecs(jiffies) - cur_time)) {
		exceed_time = jiffies_to_msecs(jiffies) - cur_time;
		pr_err("mtk_mfg_dvfs_handler exceed time : %lu ms\n",
		       exceed_time);
	}

	if (dvfs->update_step != prev_step) {
		int idx = (prev_step * mfg->num_of_steps) + dvfs->update_step;

		dvfs->trans_table[idx]++;
		dvfs->total_trans++;
	}

fini:
	dvfs->last_stat_updated = cur_time;
	dvfs->update_frequency = 0;
}

int mtk_mfg_init_dvfs(struct device *dev, struct mfg_base *mfg)
{
	struct mfg_dvfs *dvfs;

	dvfs = devm_kzalloc(dev, sizeof(*dvfs), GFP_KERNEL);
	if (!dvfs)
		return -ENOMEM;

	dvfs->trans_table = devm_kcalloc(dev,
					 mfg->num_of_steps * mfg->num_of_steps,
					 sizeof(*dvfs->trans_table),
					 GFP_KERNEL);
	if (!dvfs->trans_table)
		return -ENOMEM;

	dvfs->time_in_state = devm_kcalloc(dev, mfg->num_of_steps,
					   sizeof(*dvfs->time_in_state),
					   GFP_KERNEL);
	if (!dvfs->time_in_state)
		return -ENOMEM;

	clk_prepare_enable(mfg->intermediate_pll);
	INIT_WORK(&mfg->work, mtk_mfg_dvfs_handler);
	mutex_init(&mfg->set_frequency_lock);
	/* dvfs->is_enabled = true; */
	dvfs->last_stat_updated = jiffies_to_msecs(jiffies);
	mfg->dvfs = dvfs;

#ifdef MTK_GPU_DVFS_POLICY
	mtk_gpufreq_dvfs_init(mfg);
#endif
	return 0;
}

void mtk_mfg_deinit_dvfs(struct mfg_base *mfg)
{
#ifdef MTK_GPU_DVFS_POLICY
	mtk_gpufreq_dvfs_deinit();
#endif
	mfg->dvfs->is_enabled = false;
	cancel_work_sync(&mfg->work);
	clk_disable_unprepare(mfg->intermediate_pll);
}

void mali_mfg_get_clock_info(struct mtk_mfg_device_data *mfg_dev,
			     struct mali_gpu_clock *gpu_clock)
{
	int i;
	struct mfg_base *mfg = mfg_dev->priv_data;

	if (!mfg->num_of_steps || !mfg->mali_item)
		return;

	gpu_clock->num_of_steps = mfg->num_of_steps;
	gpu_clock->item = mfg->mali_item;

	for (i = 0; i < mfg->num_of_steps; i++) {
		unsigned int freq = mfg->power_table[i].hz / 1000000;
		unsigned int volt = mfg->power_table[i].u_volt;

		gpu_clock->item[i].clock = freq;
		gpu_clock->item[i].vol = volt;
	}
}

int mali_mfg_set_clock_step(struct mtk_mfg_device_data *mfg_dev, int step)
{
	struct mfg_base *mfg = mfg_dev->priv_data;
	struct mfg_dvfs *dvfs = mfg->dvfs;
	unsigned long freq = mfg->power_table[step].hz;

	if (atomic_read(&mfg_power_off) == 1) {
		pr_err("don't change gpu freq at poweroff\n");
		return 0;
	}

	if (!mfg->dvfs->is_enabled)
		return 0;

	if (!dvfs->update_frequency) {
		dvfs->update_frequency = freq;
		dvfs->update_step = step;
		schedule_work(&mfg->work);
	}

	MALI_DEBUG_PRINT(1, ("mali_mfg_set_clock_step freq = %lu\n", freq));
	return 0;
}

int mali_mfg_get_clock_step(struct mtk_mfg_device_data *mfg_dev)
{
	struct mfg_base *mfg = mfg_dev->priv_data;

	return mfg->current_step;
}

#ifdef MTK_GPU_DVFS_POLICY
static int mtk_mfg_fill_opp_table(struct device *dev, struct mfg_base *mfg)
{
	int i, num_of_steps = mtk_gpufreq_dvfs_get_freq_level_count();

	if (!num_of_steps)
		return 0;

	mfg->power_table = devm_kcalloc(dev, num_of_steps,
					sizeof(*mfg->power_table), GFP_KERNEL);
	if (!mfg->power_table)
		return -ENOMEM;

	for (i = 0; i < num_of_steps; i++) {
		int khz, volt, power;
		int ret = mtk_gpufreq_dvfs_get_opp_by_level(i, &khz, &volt,
								&power);

		if (ret < 0)
			break;
		mfg->power_table[i].hz = khz * 1000;
		mfg->power_table[i].u_volt = volt;
		mfg->power_table[i].power = power;

		dev_info(dev, "power_table %d/%d (%d, %d, %d)\n",
			 i, mfg->num_of_steps,
			 mfg->power_table[i].hz,
			 mfg->power_table[i].u_volt,
			 mfg->power_table[i].power);
	}
	if (i == num_of_steps)
		mfg->num_of_steps = num_of_steps;
	return 0;
}

#else
static int mtk_mfg_dynamic_power(unsigned long freq, unsigned long volt)
{
	#define HZ_to_KHZ(x) (x / 1000)
	struct mfg_power_table ref = {
		.hz = 416000000,
		.u_volt = 1150000,
		.power = 1087,
	};

	unsigned long weight;
	unsigned long dynamic;

	weight = (100 * volt) / ref.u_volt;
	weight = weight * weight * HZ_to_KHZ(freq);
	weight = weight / HZ_to_KHZ(ref.hz);
	dynamic = ref.power * weight;
	dynamic = dynamic / (100 * 100);

	return dynamic;
}

static int mtk_mfg_fill_opp_table(struct device *dev, struct mfg_base *mfg)
{
	const struct property *prop;
	const __be32 *val;
	int i, nr;

	prop = of_find_property(dev->of_node, "operating-points", NULL);
	if (!prop)
		return -ENODEV;
	if (!prop->value)
		return -ENODATA;

	/*
	 * Each OPP is a set of tuples consisting of frequency and
	 * voltage like <freq-kHz vol-uV>.
	 */
	nr = prop->length / sizeof(u32);
	if (nr % 2) {
		dev_err(dev, "%s: Invalid OPP list\n", __func__);
		return -EINVAL;
	}

	mfg->power_table = devm_kcalloc(dev, nr / 2,
					sizeof(*mfg->power_table), GFP_KERNEL);
	if (!mfg->power_table)
		return -ENOMEM;

	mfg->mali_item = devm_kcalloc(dev, nr / 2,
				      sizeof(*mfg->mali_item), GFP_KERNEL);
	if (!mfg->mali_item)
		return -ENOMEM;

	mfg->num_of_steps = nr / 2;

	val = prop->value;
	for (i = 0; i < mfg->num_of_steps; i++) {
		unsigned long freq = be32_to_cpup(val++) * 1000;
		unsigned long volt = be32_to_cpup(val++);

		mfg->power_table[i].hz = freq;
		mfg->power_table[i].u_volt = volt;
		mfg->power_table[i].power =
				mtk_mfg_dynamic_power(freq, volt);

		dev_info(dev, "power_table %d/%d (%lu, %lu, %d)\n",
			 i, mfg->num_of_steps, freq, volt,
			 mfg->power_table[i].power);
	}

	return 0;
}
#endif

static ssize_t show_available_frequencies(struct device *dev,
					  struct device_attribute *attr,
					  char *buf)
{
	struct mtk_mfg_device_data *mfg_dev = dev_get_platdata(dev);
	struct mfg_base *mfg = mfg_dev->priv_data;
	ssize_t ret = 0;
	u32 i;

	for (i = 0; i < mfg->num_of_steps; i++)
		ret += scnprintf(buf + ret, PAGE_SIZE - ret, "%u\n",
				 mfg->power_table[i].hz);

	return ret;
}

static ssize_t show_trans_stat(struct device *dev,
			       struct device_attribute *attr,
			       char *buf)
{
	struct mtk_mfg_device_data *mfg_dev = dev_get_platdata(dev);
	struct mfg_base *mfg = mfg_dev->priv_data;
	ssize_t len = 0;

	len += sprintf(buf + len, "clock rate = %lu\n",
			 clk_get_rate(mfg->mfg_pll));

	return len;
}

static ssize_t show_memory_usage(struct device *dev,
			    struct device_attribute *attr, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "total allocate memory %u\n",
			 _mali_ukk_report_memory_usage());
}

#ifndef MTK_GPU_DVFS_POLICY
static ssize_t show_clock(struct device *dev,
			  struct device_attribute *attr, char *buf)
{
	struct mtk_mfg_device_data *mfg_dev = dev_get_platdata(dev);
	struct mfg_base *mfg = mfg_dev->priv_data;

	return scnprintf(buf, PAGE_SIZE, "%lu\n", clk_get_rate(mfg->mfg_pll));
}

static ssize_t set_clock(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count)
{
	struct mtk_mfg_device_data *mfg_dev = dev_get_platdata(dev);
	struct mfg_base *mfg = mfg_dev->priv_data;
	unsigned long freq;
	ssize_t ret;

	ret = kstrtoul(buf, 10, &freq);
	if (ret)
		return ret;

	clk_set_rate(mfg->mfg_pll, freq);
	if (ret)
		return ret;

	return count;
}

static ssize_t show_dvfs_enable(struct device *dev,
			   struct device_attribute *attr, char *buf)
{
	struct mtk_mfg_device_data *mfg_dev = dev_get_platdata(dev);
	struct mfg_base *mfg = mfg_dev->priv_data;
	struct mfg_dvfs *dvfs = mfg->dvfs;

	return scnprintf(buf, PAGE_SIZE, "%u\n", dvfs->is_enabled);
}

static ssize_t set_dvfs_enable(struct device *dev,
			       struct device_attribute *attr, const char *buf,
			       size_t count)
{
	struct mtk_mfg_device_data *mfg_dev = dev_get_platdata(dev);
	struct mfg_base *mfg = mfg_dev->priv_data;
	struct mfg_dvfs *dvfs = mfg->dvfs;

	unsigned long enable;
	ssize_t ret;

	ret = kstrtoul(buf, 0, &enable);
	if (ret)
		return ret;

	if (enable == 1)
		dvfs->is_enabled = true;
	else if (enable == 0)
		dvfs->is_enabled = false;
	else
		return -EINVAL;

	return count;
}
#endif

static ssize_t set_power_down(struct device *dev,
			       struct device_attribute *attr, const char *buf,
			       size_t count)
{
	unsigned long enable;
	ssize_t ret;

	ret = kstrtoul(buf, 0, &enable);
	if (ret)
		return ret;

	if (enable == 1)
		pm_runtime_put_sync_autosuspend(dev);

	return count;
}

DEVICE_ATTR(available_frequencies, S_IRUGO, show_available_frequencies, NULL);
DEVICE_ATTR(trans_stat, S_IRUGO, show_trans_stat, NULL);
DEVICE_ATTR(memory_usage, S_IRUGO, show_memory_usage, NULL);
#ifndef MTK_GPU_DVFS_POLICY
DEVICE_ATTR(clock, S_IRUGO | S_IWUSR, show_clock, set_clock);
DEVICE_ATTR(dvfs_enable, S_IRUGO | S_IWUSR, show_dvfs_enable, set_dvfs_enable);
#endif
DEVICE_ATTR(power_down, S_IWUSR, NULL, set_power_down);

static struct attribute *mtk_mfg_sysfs_entries[] = {
	&dev_attr_available_frequencies.attr,
	&dev_attr_trans_stat.attr,
	&dev_attr_memory_usage.attr,
#ifndef MTK_GPU_DVFS_POLICY
	&dev_attr_clock.attr,
	&dev_attr_dvfs_enable.attr,
#endif
	&dev_attr_power_down.attr,
	NULL,
};

static const struct attribute_group mtk_mfg_attr_group = {
	.attrs	= mtk_mfg_sysfs_entries,
};

static int mtk_mfg_create_sysfs(struct device *dev)
{
	int ret;

	ret = sysfs_create_group(&dev->kobj, &mtk_mfg_attr_group);
	if (ret)
		dev_err(dev, "create sysfs group error, %d\n", ret);

	return ret;
}

void mtk_mfg_remove_sysfs(struct device *dev)
{
	sysfs_remove_group(&dev->kobj, &mtk_mfg_attr_group);
}

int mali_mfgsys_init(struct platform_device *device, struct mfg_base *mfg)
{
	int err = 0;
	struct device_node *node;

	mfg->reg_base = of_iomap(device->dev.of_node, 1);
	if (!mfg->reg_base)
		return -ENOMEM;

	node = of_parse_phandle(device->dev.of_node, "larb", 0);
	if (!node)
		return -EINVAL;

	mfg->smi_pdev = of_find_device_by_node(node);
	if (!mfg->smi_pdev || !mfg->smi_pdev->dev.driver) {
		of_node_put(node);
		return -EPROBE_DEFER;
	}

	mfg->mfg_pll = devm_clk_get(&device->dev, "mfg_pll");
	if (IS_ERR(mfg->mfg_pll)) {
		err = PTR_ERR(mfg->mfg_pll);
		dev_err(&device->dev, "devm_clk_get mfg_pll failed\n");
		goto err_iounmap_reg_base;
	}

	mfg->mfg_sel = devm_clk_get(&device->dev, "mfg_sel");
	if (IS_ERR(mfg->mfg_sel)) {
		err = PTR_ERR(mfg->mfg_sel);
		dev_err(&device->dev, "devm_clk_get mfg_sel failed\n");
		goto err_iounmap_reg_base;
	}

	mfg->vdd_g3d = devm_regulator_get(&device->dev, "vdd_g3d");
	if (IS_ERR(mfg->vdd_g3d)) {
		err = PTR_ERR(mfg->vdd_g3d);
		goto err_iounmap_reg_base;
	}

	err = regulator_enable(mfg->vdd_g3d);
	if (err != 0) {
		dev_err(&device->dev, "failed to enable regulator vdd_g3d\n");
		goto err_iounmap_reg_base;
	}

	err = mtk_mfg_fill_opp_table(&device->dev, mfg);
	if (!err) {
		mfg->intermediate_pll =
				devm_clk_get(&device->dev, "intermediate");
		if (IS_ERR(mfg->intermediate_pll)) {
			err = PTR_ERR(mfg->intermediate_pll);
			dev_err(&device->dev, "get intermediate failed\n");
			goto err_iounmap_reg_base;
		}
	}

	mtk_mfg_create_sysfs(&device->dev);

	return 0;

err_iounmap_reg_base:
	iounmap(mfg->reg_base);
	return err;
}

void mali_mfgsys_deinit(struct platform_device *device)
{
	struct mtk_mfg_device_data *mfg_dev = dev_get_platdata(&device->dev);
	struct mfg_base *mfg = mfg_dev->priv_data;

	mtk_mfg_remove_sysfs(&device->dev);
	if (mfg->intermediate_pll)
		mtk_mfg_deinit_dvfs(mfg);
	iounmap(mfg->reg_base);
	regulator_disable(mfg->vdd_g3d);
}

int mali_clk_enable(struct device *dev)
{
	struct mtk_mfg_device_data *mfg_dev = dev_get_platdata(dev);
	struct mfg_base *mfg = mfg_dev->priv_data;

	MALI_DEBUG_PRINT(1, ("mali_clk_enable\n"));

	pm_runtime_get_sync(dev);
	if (mtk_smi_larb_get(&mfg->smi_pdev->dev))
		dev_err(dev, "mtk_smi_larb_get failed\n");

	clk_prepare_enable(mfg->mfg_pll);
	clk_prepare_enable(mfg->mfg_sel);
	mtk_mfg_clr_clock_gating(mfg->reg_base);

	return 0;
}

int mali_clk_disable(struct device *dev)
{
	struct mtk_mfg_device_data *mfg_dev = dev_get_platdata(dev);
	struct mfg_base *mfg = mfg_dev->priv_data;

	MALI_DEBUG_PRINT(1, ("mali_clk_disable\n"));

	mtk_mfg_set_clock_gating(mfg->reg_base);
	clk_disable_unprepare(mfg->mfg_sel);
	clk_disable_unprepare(mfg->mfg_pll);

	mtk_smi_larb_put(&mfg->smi_pdev->dev);
	pm_runtime_put_sync(dev);

	return 0;
}

static void mali_set_initial_clock(struct device *dev)
{
	int err;
	struct mtk_mfg_device_data *mfg_dev = dev_get_platdata(dev);
	struct mfg_base *mfg = mfg_dev->priv_data;

	if (mfg->num_of_steps) {
#ifdef MTK_GPU_DVFS_POLICY
		int step = mtk_get_current_frequency_id();
#else
		int step = mfg->num_of_steps - 1;
#endif
		unsigned long freq = mfg->power_table[step].hz;

		mutex_init(&mfg->set_frequency_lock);
		err = mtk_mfg_set_clock_rate(mfg, freq);
		if (!err)
			mfg->current_step = step;
		dev_err(dev, "mali_set_initial_clock [%d] %lu\n",
			step, freq);
	} else {
		clk_prepare_enable(mfg->mfg_sel);
		err = clk_set_parent(mfg->mfg_sel, mfg->mfg_pll);
		clk_disable_unprepare(mfg->mfg_sel);
		if (err)
			dev_err(dev, "failed to set mfg select\n");
	}
}

int mali_mtk_mfg_bind_resource(struct platform_device *device,
			       void **priv_data)
{
	int err;
	struct mfg_base *mfg;

	mfg = devm_kzalloc(&device->dev, sizeof(*mfg), GFP_KERNEL);
	if (!mfg)
		return -ENOMEM;

	err = mali_mfgsys_init(device, mfg);
	if (err)
		return err;

	*priv_data = mfg;

	return 0;
}

void mali_mtk_mfg_unbind_resource(struct platform_device *device)
{
	mali_mfgsys_deinit(device);
}

int mali_pmm_init(struct platform_device *device)
{
	atomic_set(&mfg_power_off, 1);
	mali_platform_power_mode_change(&device->dev, MALI_POWER_MODE_ON);
	mali_set_initial_clock(&device->dev);

	return 0;
}

void mali_pmm_deinit(struct platform_device *device)
{
	mali_platform_power_mode_change(&device->dev,
					MALI_POWER_MODE_DEEP_SLEEP);
}

void mali_platform_power_mode_change(struct device *device,
				     enum mali_power_mode power_mode)
{
#if defined(CONFIG_MALI400_PROFILING)
	int event = MALI_PROFILING_EVENT_TYPE_SINGLE |
		    MALI_PROFILING_EVENT_CHANNEL_GPU |
		    MALI_PROFILING_EVENT_REASON_SINGLE_GPU_FREQ_VOLT_CHANGE;
#endif

	switch (power_mode) {
	case MALI_POWER_MODE_ON:
		MALI_DEBUG_PRINT(1,
				 ("Mali platform: MALI_POWER_MODE_ON, %s\n",
				  atomic_read(&mfg_power_off) ?
				  "powering on" : "already on"));
		if (atomic_read(&mfg_power_off) == 1) {
			/*Leave this to undepend ref count of clkmgr */
			if (!mali_clk_enable(device))
				atomic_set(&mfg_power_off, 0);
#if defined(CONFIG_MALI400_PROFILING)
			_mali_osk_profiling_add_event(event,
					500, 1200 / 1000, 0, 0, 0);
#endif
		}
		break;
	case MALI_POWER_MODE_LIGHT_SLEEP:
	case MALI_POWER_MODE_DEEP_SLEEP:
		MALI_DEBUG_PRINT(1,
				 ("Mali platform: Got %s event, %s\n",
				  power_mode ==
				  MALI_POWER_MODE_LIGHT_SLEEP ?
				  "MALI_POWER_MODE_LIGHT_SLEEP" :
				  "MALI_POWER_MODE_DEEP_SLEEP",
				  atomic_read(&mfg_power_off) ?
				  "already off" : "powering off"));

		if (atomic_read(&mfg_power_off) == 0) {
			if (!mali_clk_disable(device))
				atomic_set(&mfg_power_off, 1);
#if defined(CONFIG_MALI400_PROFILING)
			_mali_osk_profiling_add_event(event,
					0, 0, 0, 0, 0);
#endif
		}
		break;
	}
}
