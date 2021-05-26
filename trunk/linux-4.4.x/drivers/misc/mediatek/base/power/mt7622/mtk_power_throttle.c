/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#include <linux/kernel.h> /* ARRAY_SIZE */
#include <linux/slab.h> /* kzalloc */
#include "mtk_static_power.h"	/* static power */
#include <linux/cpufreq.h>
#include "mtk_power_throttle.h"
#include "mt_hotplug_strategy.h"
#include "mtk_svs.h"
/*#include "inc/mtk_thermal.h"*/

static unsigned long clipped_freq;
static unsigned int limited_max_ncpu;
static unsigned int limited_max_freq;

#define NR_MAX_OPP_TBL  8
#define NR_MAX_CPU      2

struct mt_cpu_power_info {
	unsigned int cpufreq_khz;
	unsigned int cpufreq_ncpu;
	unsigned int cpufreq_power;
};

struct mt_cpu_dvfs {
	const char *name;
	unsigned int cpu_id;	                /* for cpufreq */
	unsigned int cpu_level;
	struct mt_cpu_dvfs_ops *ops;

	/* opp (freq) table */
	struct mt_cpu_freq_info *opp_tbl;       /* OPP table */
	int nr_opp_tbl;                         /* size for OPP table */
	int idx_opp_tbl;                        /* current OPP idx */
	int idx_normal_max_opp;                 /* idx for normal max OPP */
	int idx_opp_tbl_for_late_resume;	/* keep the setting for late resume */

	struct cpufreq_frequency_table *freq_tbl_for_cpufreq; /* freq table for cpufreq */

	/* power table */
	struct mt_cpu_power_info *power_tbl;
	unsigned int nr_power_tbl;

	/* enable/disable DVFS function */
	int dvfs_disable_count;
	bool dvfs_disable_by_ptpod;
	bool dvfs_disable_by_suspend;
	bool dvfs_disable_by_early_suspend;
	bool dvfs_disable_by_procfs;

	/* limit for thermal */
	unsigned int limited_max_ncpu;
	unsigned int limited_max_freq;
	unsigned int idx_opp_tbl_for_thermal_thro;
	unsigned int thermal_protect_limited_power;

	/* limit for HEVC (via. sysfs) */
	unsigned int limited_freq_by_hevc;

	/* limit max freq from user */
	unsigned int limited_max_freq_by_user;

	/* for ramp down */
	int ramp_down_count;
	int ramp_down_count_const;

#ifdef CONFIG_CPU_DVFS_DOWNGRADE_FREQ
	/* param for micro throttling */
	bool downgrade_freq_for_ptpod;
#endif

	int over_max_cpu;
	int ptpod_temperature_limit_1;
	int ptpod_temperature_limit_2;
	int ptpod_temperature_time_1;
	int ptpod_temperature_time_2;

	int pre_online_cpu;
	unsigned int pre_freq;
	unsigned int downgrade_freq;

	unsigned int downgrade_freq_counter;
	unsigned int downgrade_freq_counter_return;

	unsigned int downgrade_freq_counter_limit;
	unsigned int downgrade_freq_counter_return_limit;

	/* turbo mode */
	unsigned int turbo_mode;

	/* power throttling */
#ifdef CONFIG_CPU_DVFS_POWER_THROTTLING
	int idx_opp_tbl_for_pwr_thro;           /* keep the setting for power throttling */
	int idx_pwr_thro_max_opp;               /* idx for power throttle max OPP */
	unsigned int pwr_thro_mode;
#endif
};

struct mt_cpu_dvfs cpu_dvfs;

struct mt_cpu_freq_info opp_tbl_default[] = {
	OP(0, 0),
	OP(0, 0),
	OP(0, 0),
	OP(0, 0),
	OP(0, 0),
	OP(0, 0),
	OP(0, 0),
	OP(0, 0),
};

static unsigned int cpu_power_tbl[] = {
	995, 811, 736, 624, 492, 376, 295, 225,  /* 2 core */
	517, 377, 333, 258, 204, 166, 140, 119}; /* 1 core */

static void _power_calculation(struct mt_cpu_dvfs *p, int oppidx, int ncpu)
{
	int possible_cpu = NR_MAX_CPU;

	p->power_tbl[NR_MAX_OPP_TBL * (possible_cpu - 1 - ncpu) + oppidx].cpufreq_ncpu
		= ncpu + 1;
	p->power_tbl[NR_MAX_OPP_TBL * (possible_cpu - 1 - ncpu) + oppidx].cpufreq_khz
		= p->opp_tbl[oppidx].cpufreq_khz;
	p->power_tbl[NR_MAX_OPP_TBL * (possible_cpu - 1 - ncpu) + oppidx].cpufreq_power
		= cpu_power_tbl[NR_MAX_OPP_TBL * (possible_cpu - ncpu) - 1 - oppidx];
}

void init_mt_cpu_dvfs(struct mt_cpu_dvfs *p)
{
	p->nr_opp_tbl = ARRAY_SIZE(opp_tbl_default);
	p->opp_tbl = opp_tbl_default;
}

void dump_power_table(void)
{
	int	i;

	/* dump power table */
	for (i = 0; i < cpu_dvfs.nr_opp_tbl * NR_MAX_CPU; i++) {
		pr_info("%s[%d] = { .cpufreq_khz = %d,\t.cpufreq_ncpu = %d,\t.cpufreq_power = %d }\n",
		__func__,
		i,
		cpu_dvfs.power_tbl[i].cpufreq_khz,
		cpu_dvfs.power_tbl[i].cpufreq_ncpu,
		cpu_dvfs.power_tbl[i].cpufreq_power
		);
	}
}

int setup_power_table(struct mt_cpu_dvfs *p)
{
	unsigned int pwr_eff_tbl[NR_MAX_OPP_TBL][NR_MAX_CPU];
	int possible_cpu = NR_MAX_CPU;
	int i, j;
	int ret = 0;

	WARN_ON(p == NULL);

	if (p->power_tbl)
		goto out;

	/* allocate power table */
	memset((void *)pwr_eff_tbl, 0, sizeof(pwr_eff_tbl));
	p->power_tbl = kzalloc(p->nr_opp_tbl * possible_cpu * sizeof(struct mt_cpu_power_info), GFP_KERNEL);

	if (p->power_tbl == NULL) {
		ret = -ENOMEM;
		goto out;
	}

	p->nr_power_tbl = p->nr_opp_tbl * possible_cpu;

	/* calc power and fill in power table */
	for (i = 0; i < p->nr_opp_tbl; i++) {
		for (j = 0; j < possible_cpu; j++) {
			if (pwr_eff_tbl[i][j] == 0)
				_power_calculation(p, i, j);
		}
	}

	/* sort power table */
	for (i = p->nr_opp_tbl * possible_cpu; i > 0; i--) {
		for (j = 1; j < i; j++) {
			if (p->power_tbl[j - 1].cpufreq_power < p->power_tbl[j].cpufreq_power) {
				struct mt_cpu_power_info tmp;

				tmp.cpufreq_khz					= p->power_tbl[j - 1].cpufreq_khz;
				tmp.cpufreq_ncpu				= p->power_tbl[j - 1].cpufreq_ncpu;
				tmp.cpufreq_power				= p->power_tbl[j - 1].cpufreq_power;

				p->power_tbl[j - 1].cpufreq_khz   = p->power_tbl[j].cpufreq_khz;
				p->power_tbl[j - 1].cpufreq_ncpu  = p->power_tbl[j].cpufreq_ncpu;
				p->power_tbl[j - 1].cpufreq_power = p->power_tbl[j].cpufreq_power;

				p->power_tbl[j].cpufreq_khz		= tmp.cpufreq_khz;
				p->power_tbl[j].cpufreq_ncpu	= tmp.cpufreq_ncpu;
				p->power_tbl[j].cpufreq_power	= tmp.cpufreq_power;
			}
		}
	}

	/* dump power table */
	dump_power_table();

out:
	return ret;
}

int __attribute__ ((weak)) is_svs_initialized_done(void)
{
	pr_warn("%s() is not implemented\n", __func__);
	return 0;
}

/**
 * cpufreq_thermal_notifier - notifier callback for cpufreq policy change.
 * @nb:	struct notifier_block * with callback info.
 * @event: value showing cpufreq event for which this function invoked.
 * @data: callback-specific data
 *
 * Callback to hijack the notification on cpufreq policy transition.
 * Every time there is a change in policy, we will intercept and
 * update the cpufreq policy with thermal constraints.
 *
 * Return: 0 (success)
 */

static int mtk_cpufreq_thermal_notifier(struct notifier_block *nb,
				    unsigned long event, void *data)
{
	struct cpufreq_policy *policy = data;
	int ret;

	pr_debug("%s %ld\n", __func__, event);

	if (event != CPUFREQ_ADJUST)
		return NOTIFY_DONE;

	/*
	 * policy->max is the maximum allowed frequency defined by user
	 * and clipped_freq is the maximum that thermal constraints
	 * allow.
	 *
	 * If clipped_freq is lower than policy->max, then we need to
	 * readjust policy->max.
	 *
	 * But, if clipped_freq is greater than policy->max, we don't
	 * need to do anything.
	 */

	if (/*mtktscpu_debug_log  & 0x1*/0)
		pr_err("%s clipped_freq = %ld, policy->max=%d, policy->min=%d\n",
				__func__, clipped_freq, policy->max, policy->min);

	/* Since thermal throttling could hogplug CPU, less cpufreq might cause
	 * a performance issue.
	 * Only DVFS TLP feature enable, we can keep the max freq by CPUFREQ
	 * GOVERNOR or Pref service.
	 */
	if ((policy->max != clipped_freq) && (clipped_freq >= policy->min)) {
		ret = is_svs_initialized_done();
		if (ret) {
			pr_err("SVS is initializing. Cannot do thermal throttling, ret = %d\n", ret);
			return NOTIFY_DONE;
		}

		cpufreq_verify_within_limits(policy, 0, clipped_freq);
	}

	return NOTIFY_OK;
}

/* Notifier for cpufreq policy change */
static struct notifier_block thermal_cpufreq_notifier_block = {
	.notifier_call = mtk_cpufreq_thermal_notifier,
};

static int power_table_ready;
int setup_power_table_tk(void)
{
	int	ret;

	init_mt_cpu_dvfs(&cpu_dvfs);

	if (cpu_dvfs.opp_tbl[0].cpufreq_khz == 0)
		return 0;

	cpufreq_register_notifier(&thermal_cpufreq_notifier_block,
					  CPUFREQ_POLICY_NOTIFIER);

	ret = setup_power_table(&cpu_dvfs);
	power_table_ready = 1;
	return ret;
}

void mt_cpufreq_thermal_protect(unsigned int limited_power)
{
	struct mt_cpu_dvfs *p;
	int possible_cpu;
	int ncpu;
	int found = 0;
	int i;

	if (power_table_ready == 0) {
		pr_err("%s power_table_ready is not ready\n", __func__);
		return;
	}

	p = &cpu_dvfs;

	WARN_ON(p == NULL);

	possible_cpu = NR_MAX_CPU;

	/* no limited */
	if (limited_power == 0) {
		limited_max_ncpu = possible_cpu;
		limited_max_freq = cpu_dvfs.power_tbl[0].cpufreq_khz;
	} else {
		for (ncpu = possible_cpu; ncpu > 0; ncpu--) {
			for (i = 0; i < p->nr_opp_tbl * possible_cpu; i++) {
				/* p->power_tbl[i].cpufreq_ncpu == ncpu && */
				if (p->power_tbl[i].cpufreq_power <= limited_power) {
					limited_max_ncpu = p->power_tbl[i].cpufreq_ncpu;
					limited_max_freq = p->power_tbl[i].cpufreq_khz;
					found = 1;
					ncpu = 0; /* for break outer loop */
					break;
				}
			}
		}

		/* not found and use lowest power limit */
		if (!found) {
			limited_max_ncpu = p->power_tbl[p->nr_power_tbl - 1].cpufreq_ncpu;
			limited_max_freq = p->power_tbl[p->nr_power_tbl - 1].cpufreq_khz;
		}
	}

	clipped_freq = limited_max_freq;

	if (/*mtktscpu_debug_log & 0x1*/0) {
		pr_err("%s found = %d, limited_power = %u\n",
			__func__, found, limited_power);
		pr_err("%s possible_cpu = %d, cpu_dvfs.power_tbl[0].cpufreq_khz =%u\n",
			__func__, possible_cpu, cpu_dvfs.power_tbl[0].cpufreq_khz);
		pr_err("%s limited_max_ncpu = %u, limited_max_freq = %u\n",
			__func__, limited_max_ncpu, limited_max_freq);
	}

	/*hps_set_cpu_num_limit(LIMIT_THERMAL, limited_max_ncpu, 0);*/
	/* update cpufreq policy */
#ifdef CONFIG_CPU_FREQ
	cpufreq_update_policy(0);
#endif
}

