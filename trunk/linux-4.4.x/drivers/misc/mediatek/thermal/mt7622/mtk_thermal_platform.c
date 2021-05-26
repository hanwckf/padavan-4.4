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

#include <linux/uaccess.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/dmi.h>
#include <linux/acpi.h>
#include <linux/thermal.h>
#include <linux/platform_device.h>
#include <mt-plat/aee.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/err.h>
#include <linux/syscalls.h>
#include <linux/time.h>
#include <linux/string.h>
#include <linux/mutex.h>
#include <linux/bug.h>
#include <linux/workqueue.h>
#include <mt-plat/mtk_thermal_platform.h>
#include "mtk_thermal_typedefs.h"

#include "mach/mtk_thermal.h"
#include <mt-plat/aee.h>
#ifdef FOR_COMPILE_PASS
#include <mtk_gpu_utility.h>
#endif

#include "inc/mtk_ts_cpu.h"

/* ************************************ */
/* Definition */
/* ************************************ */

/* Number of CPU CORE */
#define NUMBER_OF_CORE (8)

#ifdef FOR_COMPILE_PASS
/* This function pointer is for GPU LKM to register a function to get GPU loading. */
unsigned long (*mtk_thermal_get_gpu_loading_fp)(void) = NULL;
/* EXPORT_SYMBOL(mtk_thermal_get_gpu_loading_fp); */
#endif


/* ************************************ */
/* Global Variable */
/* ************************************ */
static bool enable_ThermalMonitorXlog;

static DEFINE_MUTEX(MTM_SYSINFO_LOCK);

/* ************************************ */
/* Macro */
/* ************************************ */
#define THRML_LOG(fmt, args...) \
do { \
	if (enable_ThermalMonitorXlog) { \
		pr_debug("THERMAL/PLATFORM" fmt, ##args); \
	} \
} while (0)


#define THRML_ERROR_LOG(fmt, args...) \
pr_debug("THERMAL/PLATFORM" fmt, ##args)


int __attribute__ ((weak))
force_get_tbat(void)
{
	pr_err("E_WF: %s doesn't exist\n", __func__);
	return 30;
}


/* ************************************ */
/* Define */
/* ************************************ */

/* ********************************************* */
/* System Information Monitor */
/* ********************************************* */
static mm_segment_t oldfs;

/*
 *  Read Battery Information.
 *
 *  "cat /sys/devices/platform/mt6575-battery/FG_Battery_CurrentConsumption"
 *  "cat /sys/class/power_supply/battery/batt_vol"
 *  "cat /sys/class/power_supply/battery/batt_temp"
 */
static int get_sys_battery_info(char *dev)
{
	int fd;
	long nRet;
	int eCheck;
	int nReadSize;
	char buf[64];

	oldfs = get_fs();
	set_fs(KERNEL_DS);
	fd = sys_open(dev, O_RDONLY, 0);
	if (fd < 0) {
		THRML_LOG("[get_sys_battery_info] open fail dev:%s fd:%d\n", dev, fd);
		set_fs(oldfs);
		return fd;
	}

	nReadSize = sys_read(fd, buf, sizeof(buf) - 1);
	THRML_LOG("[get_sys_battery_info] nReadSize:%d\n", nReadSize);
	eCheck = kstrtol(buf, 10, &nRet);

	set_fs(oldfs);
	sys_close(fd);

	if (eCheck == 0)
		return (int)nRet;
	else
		return 0;
}

/* ********************************************* */
/* Get Wifi Tx throughput */
/* ********************************************* */
static int get_sys_wifi_throughput(char *dev, int nRetryNr)
{
	int fd;
	long nRet;
	int eCheck;
	int nReadSize;
	int nRetryCnt = 0;
	char buf[64];

	oldfs = get_fs();
	set_fs(KERNEL_DS);

	/* If sys_open fail, it will retry "nRetryNr" times. */
	do {
		fd = sys_open(dev, O_RDONLY, 0);
		if (nRetryCnt > nRetryNr) {
			THRML_LOG("[get_sys_wifi_throughput] open fail dev:%s fd:%d\n", dev, fd);
			set_fs(oldfs);
			return fd;
		}
		nRetryCnt++;
	} while (fd < 0);

	if (nRetryCnt > 1)
		THRML_LOG("[get_sys_wifi_throughput] open fail nRetryCnt:%d\n", nRetryCnt);


	nReadSize = sys_read(fd, buf, sizeof(buf) - 1);
	THRML_LOG("[get_sys_wifi_throughput] nReadSize:%d\n", nReadSize);
	eCheck = kstrtol(buf, 10, &nRet);

	set_fs(oldfs);
	sys_close(fd);

	if (eCheck == 0)
		return (int)nRet;
	else
		return 0;
}

/* ********************************************* */
/* For get_sys_cpu_usage_info_ex() */
/* ********************************************* */

#define CPU_USAGE_CURRENT_FIELD (0)
#define CPU_USAGE_SAVE_FIELD    (1)
#define CPU_USAGE_FRAME_FIELD   (2)

struct cpu_index_st {
	unsigned long u[3];
	unsigned long s[3];
	unsigned long n[3];
	unsigned long i[3];
	unsigned long w[3];
	unsigned long q[3];
	unsigned long sq[3];
	unsigned long tot_frme;
	unsigned long tz;
	int usage;
	int freq;
};

#ifdef FOR_COMPILE_PASS
struct gpu_index_st {
	int usage;
	int freq;
};
#endif

#define NO_CPU_CORES (8)
static struct cpu_index_st cpu_index_list[NO_CPU_CORES];	/* /< 4-Core is maximum */
static int cpufreqs[NO_CPU_CORES];
static int cpuloadings[NO_CPU_CORES];

#define SEEK_BUFF(x, c)	\
do { \
	while (*x != c)\
		x++; \
	x++; \
} while (0)

#define TRIMz_ex(tz, x)   ((tz = (unsigned long long)(x)) < 0 ? 0 : tz)

/* ********************************************* */
/* CPU Index */
/* ********************************************* */
#include <linux/kernel_stat.h>
#include <linux/cpumask.h>
#include <asm/cputime.h>
#include <linux/sched.h>
#include <linux/tick.h>
#include <linux/time.h>

#ifdef arch_idle_time

static cputime64_t get_idle_time(int cpu)
{
	cputime64_t idle;

	idle = kcpustat_cpu(cpu).cpustat[CPUTIME_IDLE];
	if (cpu_online(cpu) && !nr_iowait_cpu(cpu))
		idle += arch_idle_time(cpu);
	return idle;
}

static cputime64_t get_iowait_time(int cpu)
{
	cputime64_t iowait;

	iowait = kcpustat_cpu(cpu).cpustat[CPUTIME_IOWAIT];
	if (cpu_online(cpu) && nr_iowait_cpu(cpu))
		iowait += arch_idle_time(cpu);
	return iowait;
}

#else

static u64 get_idle_time(int cpu)
{
	u64 idle, idle_time = -1ULL;

	if (cpu_online(cpu))
		idle_time = get_cpu_idle_time_us(cpu, NULL);

	if (idle_time == -1ULL)
		/* !NO_HZ or cpu offline so we can rely on cpustat.idle */
		idle = kcpustat_cpu(cpu).cpustat[CPUTIME_IDLE];
	else
		idle = usecs_to_cputime64(idle_time);

	return idle;
}

static u64 get_iowait_time(int cpu)
{
	u64 iowait, iowait_time = -1ULL;

	if (cpu_online(cpu))
		iowait_time = get_cpu_iowait_time_us(cpu, NULL);

	if (iowait_time == -1ULL)
		/* !NO_HZ or cpu offline so we can rely on cpustat.iowait */
		iowait = kcpustat_cpu(cpu).cpustat[CPUTIME_IOWAIT];
	else
		iowait = usecs_to_cputime64(iowait_time);

	return iowait;
}

#endif

static int get_sys_cpu_usage_info_ex(void)
{
	int nCoreIndex = 0, i;

	for (i = 0; i < NO_CPU_CORES; i++)
		cpuloadings[i] = 0;

	for_each_online_cpu(nCoreIndex) {

		/* Get CPU Info */
		cpu_index_list[nCoreIndex].u[CPU_USAGE_CURRENT_FIELD] =
		    kcpustat_cpu(nCoreIndex).cpustat[CPUTIME_USER];
		cpu_index_list[nCoreIndex].n[CPU_USAGE_CURRENT_FIELD] =
		    kcpustat_cpu(nCoreIndex).cpustat[CPUTIME_NICE];
		cpu_index_list[nCoreIndex].s[CPU_USAGE_CURRENT_FIELD] =
		    kcpustat_cpu(nCoreIndex).cpustat[CPUTIME_SYSTEM];
		cpu_index_list[nCoreIndex].i[CPU_USAGE_CURRENT_FIELD] = get_idle_time(nCoreIndex);
		cpu_index_list[nCoreIndex].w[CPU_USAGE_CURRENT_FIELD] = get_iowait_time(nCoreIndex);
		cpu_index_list[nCoreIndex].q[CPU_USAGE_CURRENT_FIELD] =
		    kcpustat_cpu(nCoreIndex).cpustat[CPUTIME_IRQ];
		cpu_index_list[nCoreIndex].sq[CPU_USAGE_CURRENT_FIELD] =
		    kcpustat_cpu(nCoreIndex).cpustat[CPUTIME_SOFTIRQ];

		/* Frame */
		cpu_index_list[nCoreIndex].u[CPU_USAGE_FRAME_FIELD] =
		    cpu_index_list[nCoreIndex].u[CPU_USAGE_CURRENT_FIELD] -
		    cpu_index_list[nCoreIndex].u[CPU_USAGE_SAVE_FIELD];
		cpu_index_list[nCoreIndex].n[CPU_USAGE_FRAME_FIELD] =
		    cpu_index_list[nCoreIndex].n[CPU_USAGE_CURRENT_FIELD] -
		    cpu_index_list[nCoreIndex].n[CPU_USAGE_SAVE_FIELD];
		cpu_index_list[nCoreIndex].s[CPU_USAGE_FRAME_FIELD] =
		    cpu_index_list[nCoreIndex].s[CPU_USAGE_CURRENT_FIELD] -
		    cpu_index_list[nCoreIndex].s[CPU_USAGE_SAVE_FIELD];
		cpu_index_list[nCoreIndex].i[CPU_USAGE_FRAME_FIELD] =
		    TRIMz_ex(cpu_index_list[nCoreIndex].tz,
			     (cpu_index_list[nCoreIndex].i[CPU_USAGE_CURRENT_FIELD] -
			      cpu_index_list[nCoreIndex].i[CPU_USAGE_SAVE_FIELD]));
		cpu_index_list[nCoreIndex].w[CPU_USAGE_FRAME_FIELD] =
		    cpu_index_list[nCoreIndex].w[CPU_USAGE_CURRENT_FIELD] -
		    cpu_index_list[nCoreIndex].w[CPU_USAGE_SAVE_FIELD];
		cpu_index_list[nCoreIndex].q[CPU_USAGE_FRAME_FIELD] =
		    cpu_index_list[nCoreIndex].q[CPU_USAGE_CURRENT_FIELD] -
		    cpu_index_list[nCoreIndex].q[CPU_USAGE_SAVE_FIELD];
		cpu_index_list[nCoreIndex].sq[CPU_USAGE_FRAME_FIELD] =
		    cpu_index_list[nCoreIndex].sq[CPU_USAGE_CURRENT_FIELD] -
		    cpu_index_list[nCoreIndex].sq[CPU_USAGE_SAVE_FIELD];

		/* Total Frame */
		cpu_index_list[nCoreIndex].tot_frme =
		    cpu_index_list[nCoreIndex].u[CPU_USAGE_FRAME_FIELD] +
		    cpu_index_list[nCoreIndex].n[CPU_USAGE_FRAME_FIELD] +
		    cpu_index_list[nCoreIndex].s[CPU_USAGE_FRAME_FIELD] +
		    cpu_index_list[nCoreIndex].i[CPU_USAGE_FRAME_FIELD] +
		    cpu_index_list[nCoreIndex].w[CPU_USAGE_FRAME_FIELD] +
		    cpu_index_list[nCoreIndex].q[CPU_USAGE_FRAME_FIELD] +
		    cpu_index_list[nCoreIndex].sq[CPU_USAGE_FRAME_FIELD];

		/* CPU Usage */
		if (cpu_index_list[nCoreIndex].tot_frme > 0) {
			cpuloadings[nCoreIndex] =
			    (100 -
			     (((int)cpu_index_list[nCoreIndex].i[CPU_USAGE_FRAME_FIELD] * 100) /
			      (int)cpu_index_list[nCoreIndex].tot_frme));
		} else {
			/* CPU unplug case */
			cpuloadings[nCoreIndex] = 0;
		}

		cpu_index_list[nCoreIndex].u[CPU_USAGE_SAVE_FIELD] =
		    cpu_index_list[nCoreIndex].u[CPU_USAGE_CURRENT_FIELD];
		cpu_index_list[nCoreIndex].n[CPU_USAGE_SAVE_FIELD] =
		    cpu_index_list[nCoreIndex].n[CPU_USAGE_CURRENT_FIELD];
		cpu_index_list[nCoreIndex].s[CPU_USAGE_SAVE_FIELD] =
		    cpu_index_list[nCoreIndex].s[CPU_USAGE_CURRENT_FIELD];
		cpu_index_list[nCoreIndex].i[CPU_USAGE_SAVE_FIELD] =
		    cpu_index_list[nCoreIndex].i[CPU_USAGE_CURRENT_FIELD];
		cpu_index_list[nCoreIndex].w[CPU_USAGE_SAVE_FIELD] =
		    cpu_index_list[nCoreIndex].w[CPU_USAGE_CURRENT_FIELD];
		cpu_index_list[nCoreIndex].q[CPU_USAGE_SAVE_FIELD] =
		    cpu_index_list[nCoreIndex].q[CPU_USAGE_CURRENT_FIELD];
		cpu_index_list[nCoreIndex].sq[CPU_USAGE_SAVE_FIELD] =
		    cpu_index_list[nCoreIndex].sq[CPU_USAGE_CURRENT_FIELD];

		THRML_LOG("CPU%d Frame:%d USAGE:%d\n", nCoreIndex,
			  (int)cpu_index_list[nCoreIndex].tot_frme, cpuloadings[nCoreIndex]);

		for (i = 0; i < 3; i++) {
			THRML_LOG("Index %d [u:%d] [n:%d] [s:%d] [i:%d] [w:%d] [q:%d] [sq:%d]\n",
				  i,
				  (int)cpu_index_list[nCoreIndex].u[i],
				  (int)cpu_index_list[nCoreIndex].n[i],
				  (int)cpu_index_list[nCoreIndex].s[i],
				  (int)cpu_index_list[nCoreIndex].i[i],
				  (int)cpu_index_list[nCoreIndex].w[i],
				  (int)cpu_index_list[nCoreIndex].q[i],
				  (int)cpu_index_list[nCoreIndex].sq[i]);

		}
	}

	return 0;

}

static bool dmips_limit_warned;
static int check_dmips_limit;

#include <linux/cpufreq.h>
static int get_sys_all_cpu_freq_info(void)
{
	int i;
	int cpu_total_dmips = 0;

	for (i = 0; i < NO_CPU_CORES; i++) {
		cpufreqs[i] = cpufreq_quick_get(i) / 1000;	/* MHz */
		cpu_total_dmips += cpufreqs[i];
	}

	cpu_total_dmips /= 1000;
	/* TODO: think a way to easy start and stop, and start for only once */
	if (check_dmips_limit == 1) {
		if (cpu_total_dmips > mtktscpu_limited_dmips) {
			THRML_ERROR_LOG("cpu %d over limit %d\n", cpu_total_dmips,
					mtktscpu_limited_dmips);
			if (dmips_limit_warned == false) {
#ifdef FOR_COMPILE_PASS
				aee_kernel_warning("thermal", "cpu %d over limit %d\n",
						   cpu_total_dmips, mtktscpu_limited_dmips);
#endif
				dmips_limit_warned = true;
			}
		}
	}

	return 0;
}

static int mtk_thermal_validation_rd(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", check_dmips_limit);

	return 0;
}

static ssize_t mtk_thermal_validation_wr(struct file *file, const char __user *buffer,
					 size_t count, loff_t *data)
{
	char desc[32];
	int check_switch;
	int len = 0;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;
	desc[len] = '\0';

	if (kstrtoint(desc, 10, &check_switch) == 0) {
		if (check_switch == 1) {
			dmips_limit_warned = false;
			check_dmips_limit = check_switch;
		} else if (!check_switch) {
			check_dmips_limit = check_switch;
		}
		return count;
	}
	THRML_ERROR_LOG("[mtk_thermal_validation_wr] bad argument\n");
	return -EINVAL;
}

static int mtk_thermal_validation_open(struct inode *inode, struct file *file)
{
	return single_open(file, mtk_thermal_validation_rd, NULL);
}

static const struct file_operations mtk_thermal_validation_fops = {
	.owner = THIS_MODULE,
	.open = mtk_thermal_validation_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = mtk_thermal_validation_wr,
	.release = single_release,
};


/* Init */
static int __init mtk_thermal_platform_init(void)
{
	int err = 0;
	struct proc_dir_entry *entry;

	entry =
	    proc_create("driver/tm_validation", S_IRUGO | S_IWUSR, NULL,
			&mtk_thermal_validation_fops);
	if (!entry) {
		THRML_ERROR_LOG
		    ("[mtk_thermal_platform_init] Can not create /proc/driver/tm_validation\n");
	}

	return err;
}

/* Exit */
static void __exit mtk_thermal_platform_exit(void)
{

}

int mtk_thermal_get_cpu_info(int *nocores, int **cpufreq, int **cpuloading)
{
	/* ****************** */
	/* CPU Usage */
	/* ****************** */
	mutex_lock(&MTM_SYSINFO_LOCK);

	/* Read CPU Usage Information */
	get_sys_cpu_usage_info_ex();

	get_sys_all_cpu_freq_info();

	mutex_unlock(&MTM_SYSINFO_LOCK);

	if (nocores)
		*nocores = NO_CPU_CORES;

	if (cpufreq)
		*cpufreq = cpufreqs;

	if (cpuloading)
		*cpuloading = cpuloadings;

	return 0;
}
EXPORT_SYMBOL(mtk_thermal_get_cpu_info);

#ifdef FOR_COMPILE_PASS
#define NO_GPU_CORES (1)
static int gpufreqs[NO_GPU_CORES];
static int gpuloadings[NO_GPU_CORES];
#endif

int mtk_thermal_get_gpu_info(int *nocores, int **gpufreq, int **gpuloading)
{
#ifdef FOR_COMPILE_PASS
	/* ****************** */
	/* GPU Index */
	/* ****************** */
	THRML_LOG("[mtk_thermal_get_gpu_info]\n");

	if (nocores)
		*nocores = NO_GPU_CORES;

	if (gpufreq) {
#ifndef MT6752_EVB_BUILD_PASS	/* Jerry fix build error FIX_ME */
		gpufreqs[0] = mt_gpufreq_get_cur_freq() / 1000;	/* MHz */
#endif
		*gpufreq = gpufreqs;
	}

	if (gpuloading) {
		unsigned int rd_gpu_loading = 0;
#ifndef MT6752_EVB_BUILD_PASS	/* Jerry fix build error FIX_ME */
		if (mtk_get_gpu_loading(&rd_gpu_loading))
#endif
			gpuloadings[0] = (int)rd_gpu_loading;
			*gpuloading = gpuloadings;
	}

#endif
	return 0;
}
EXPORT_SYMBOL(mtk_thermal_get_gpu_info);

int mtk_thermal_get_batt_info(int *batt_voltage, int *batt_current, int *batt_temp)
{
	/* ****************** */
	/* Battery */
	/* ****************** */

	/* Read Battery Information */
	if (batt_current) {
		*batt_current =
		    get_sys_battery_info
		    ("/sys/devices/platform/battery/FG_Battery_CurrentConsumption");
		/* the return value is 0.1mA */
		if (*batt_current % 10 < 5)
			*batt_current /= 10;
		else
			*batt_current = 1 + (*batt_current / 10);


#if defined(CONFIG_MTK_SMART_BATTERY)
		if (gFG_Is_Charging == KAL_TRUE)
			*batt_current *= -1;
#endif
	}

	if (batt_voltage)
		*batt_voltage = get_sys_battery_info("/sys/class/power_supply/battery/batt_vol");

	if (batt_temp)
		*batt_temp = get_sys_battery_info("/sys/class/power_supply/battery/batt_temp");

	return 0;
}
EXPORT_SYMBOL(mtk_thermal_get_batt_info);

#define NO_EXTRA_THERMAL_ATTR (7)
static char *extra_attr_names[NO_EXTRA_THERMAL_ATTR] = { 0 };
static int extra_attr_values[NO_EXTRA_THERMAL_ATTR] = { 0 };
static char *extra_attr_units[NO_EXTRA_THERMAL_ATTR] = { 0 };

int mtk_thermal_get_extra_info(int *no_extra_attr,
			       char ***attr_names, int **attr_values, char ***attr_units)
{

	int i = 0;

	if (no_extra_attr)
		*no_extra_attr = NO_EXTRA_THERMAL_ATTR;

	/* ****************** */
	/* Wifi Index */
	/* ****************** */
	/* Get Wi-Fi Tx throughput */
	extra_attr_names[i] = "WiFi_TP";
	extra_attr_values[i] = get_sys_wifi_throughput("/proc/driver/thermal/wifi_tx_thro", 3);
	extra_attr_units[i] = "Kbps";

	if (attr_names)
		*attr_names = extra_attr_names;

	if (attr_values)
		*attr_values = extra_attr_values;

	if (attr_units)
		*attr_units = extra_attr_units;

	return 0;

}
EXPORT_SYMBOL(mtk_thermal_get_extra_info);

int mtk_thermal_force_get_batt_temp(void)
{
	int ret = 0;

	ret = force_get_tbat();

	return ret;
}
EXPORT_SYMBOL(mtk_thermal_force_get_batt_temp);

static unsigned int _thermal_scen;

unsigned int mtk_thermal_set_user_scenarios(unsigned int mask)
{
	if ((mask & MTK_THERMAL_SCEN_CALL))	{
		/* only one scen is handled now... */
		set_taklking_flag(true);	/* make mtk_ts_cpu.c aware of call scenario */
		_thermal_scen |= (unsigned int)MTK_THERMAL_SCEN_CALL;
	}
	return _thermal_scen;
}
EXPORT_SYMBOL(mtk_thermal_set_user_scenarios);

unsigned int mtk_thermal_clear_user_scenarios(unsigned int mask)
{
	if ((mask & MTK_THERMAL_SCEN_CALL))	{
		/* only one scen is handled now... */
		set_taklking_flag(false);	/* make mtk_ts_cpu.c aware of call scenario */
		_thermal_scen &= ~((unsigned int)MTK_THERMAL_SCEN_CALL);
	}
	return _thermal_scen;
}
EXPORT_SYMBOL(mtk_thermal_clear_user_scenarios);

/* ********************************************* */
/* Export Interface */
/* ********************************************* */

module_init(mtk_thermal_platform_init);
module_exit(mtk_thermal_platform_exit);
