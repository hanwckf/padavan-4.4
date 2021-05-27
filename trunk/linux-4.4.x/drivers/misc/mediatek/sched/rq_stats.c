/* Copyright (c) 2010-2013, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/hrtimer.h>
#include <linux/cpu.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/notifier.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/cpufreq.h>
#include <linux/kernel_stat.h>
#include <linux/tick.h>
#include <linux/suspend.h>
#include <linux/version.h>
#include <asm/smp_plat.h>

#include <trace/events/sched.h>
#include "rq_stats.h"

#define MAX_LONG_SIZE 24
#define DEFAULT_RQ_POLL_JIFFIES 1
#define DEFAULT_DEF_TIMER_JIFFIES 5
#ifdef CONFIG_SCHED_HMP_PRIO_FILTER
static unsigned int heavy_task_prio = NICE_TO_PRIO(CONFIG_SCHED_HMP_PRIO_FILTER_VAL);
#define task_low_priority(prio) ((prio >= heavy_task_prio)?1:0)
#endif

#ifdef CONFIG_MTK_SCHED_RQAVG_US
struct rq_data rq_info;
spinlock_t rq_lock;
#endif

static int cpufreq_variant = 1; /* set to 1 if per cpu load is cpu freq. variant */
#ifdef CONFIG_CPU_FREQ
struct notifier_block freq_transition;
#endif /* CONFIG_CPU_FREQ */
struct notifier_block cpu_hotplug;
static unsigned int heavy_task_threshold = 650; /* max=1023 */
static int htask_cpucap_ctrl = 1;

struct cpu_load_data {
	cputime64_t prev_cpu_idle;
	cputime64_t prev_cpu_wall;
	cputime64_t prev_cpu_iowait;
	unsigned int avg_load_maxfreq_rel;
	unsigned int avg_load_maxfreq_abs;
	unsigned int samples;
	unsigned int window_size;
	unsigned int cur_freq;
	unsigned int policy_max;
	cpumask_var_t related_cpus;
	spinlock_t cpu_load_lock;
};

static DEFINE_PER_CPU(struct cpu_load_data, cpuload);
#ifdef CONFIG_CPU_FREQ
static DEFINE_PER_CPU(struct cpufreq_policy, cpupolicy);
#endif
typedef enum {
	AVG_LOAD_IGNORE,
	AVG_LOAD_UPDATE
} AVG_LOAD_ID;

#ifdef CONFIG_CPU_FREQ
#include <linux/cpufreq.h>
#else  /* !CONFIG_CPU_FREQ  */
static inline u64 get_cpu_idle_time_jiffy(unsigned int cpu, u64 *wall)
{
	u64 idle_time;
	u64 cur_wall_time;
	u64 busy_time;

	cur_wall_time = jiffies64_to_cputime64(get_jiffies_64());

	busy_time = kcpustat_cpu(cpu).cpustat[CPUTIME_USER];
	busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_SYSTEM];
	busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_IRQ];
	busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_SOFTIRQ];
	busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_STEAL];
	busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_NICE];

	idle_time = cur_wall_time - busy_time;
	if (wall)
		*wall = cputime_to_usecs(cur_wall_time);

	return cputime_to_usecs(idle_time);
}

static inline cputime64_t get_cpu_idle_time_internal(unsigned int cpu, cputime64_t *wall)
{
	u64 idle_time = get_cpu_idle_time_us(cpu, NULL);

	if (idle_time == -1ULL)
		idle_time = get_cpu_idle_time_jiffy(cpu, wall);
	else
		idle_time += get_cpu_iowait_time_us(cpu, wall);

	return idle_time;
}
#endif /* CONFIG_CPU_FREQ */

static inline cputime64_t get_cpu_iowait_time(unsigned int cpu,
							cputime64_t *wall)
{
	u64 iowait_time = get_cpu_iowait_time_us(cpu, wall);

	if (iowait_time == -1ULL)
		return 0;

	return iowait_time;
}

static int update_average_load(AVG_LOAD_ID id, unsigned int freq, unsigned int cpu)
{

	struct cpu_load_data *pcpu = &per_cpu(cpuload, cpu);
	cputime64_t cur_wall_time, cur_idle_time, cur_iowait_time;
	unsigned int idle_time, wall_time, iowait_time;
	unsigned int cur_load, prev_avg_load_rel, prev_avg_load_abs;
	unsigned int load_at_max_freq_rel, load_at_max_freq_abs;
	cputime64_t prev_wall_time, prev_cpu_idle, prev_cpu_iowait;

#ifdef CONFIG_CPU_FREQ
	cur_idle_time = get_cpu_idle_time(cpu, &cur_wall_time, 0);
#else  /* !CONFIG_CPU_FREQ  */
	cur_idle_time = get_cpu_idle_time_internal(cpu, &cur_wall_time);
#endif /* CONFIG_CPU_FREQ */
	cur_iowait_time = get_cpu_iowait_time(cpu, &cur_wall_time);

	prev_wall_time = pcpu->prev_cpu_wall;
	prev_cpu_idle = pcpu->prev_cpu_idle;
	prev_cpu_iowait = pcpu->prev_cpu_iowait;

	wall_time = (unsigned int) (cur_wall_time - pcpu->prev_cpu_wall);
	pcpu->prev_cpu_wall = cur_wall_time;

	idle_time = (unsigned int) (cur_idle_time - pcpu->prev_cpu_idle);
	pcpu->prev_cpu_idle = cur_idle_time;

	iowait_time = (unsigned int) (cur_iowait_time - pcpu->prev_cpu_iowait);
	pcpu->prev_cpu_iowait = cur_iowait_time;

	if (idle_time >= iowait_time)
		idle_time -= iowait_time;

	if (unlikely(!wall_time || wall_time < idle_time))
		return 0;

	if (id == AVG_LOAD_IGNORE)
		cur_load = 0;
	else
		cur_load = 100 * (wall_time - idle_time) / wall_time;

	/* Calculate the scaled load across CPU */
	if (cpu_online(cpu)) {
		load_at_max_freq_abs = (cur_load * freq) / pcpu->policy_max;
		load_at_max_freq_rel = cur_load;
	} else {
		load_at_max_freq_abs = 0;
		load_at_max_freq_rel = 0;
	}

	prev_avg_load_rel = pcpu->avg_load_maxfreq_rel;
	prev_avg_load_abs = pcpu->avg_load_maxfreq_abs;

#if 1
	if (!pcpu->avg_load_maxfreq_rel || !pcpu->avg_load_maxfreq_abs) {
		/* This is the first sample in this window*/
		pcpu->avg_load_maxfreq_rel = load_at_max_freq_rel;
		pcpu->avg_load_maxfreq_abs = load_at_max_freq_abs;
		pcpu->window_size = wall_time;
	} else {
		/*
		 * The is already a sample available in this window.
		 * Compute weighted average with prev entry, so that we get
		 * the precise weighted load.
		 */
		pcpu->avg_load_maxfreq_rel =
		    ((pcpu->avg_load_maxfreq_rel * pcpu->window_size) +
			(load_at_max_freq_rel * wall_time)) /
			(wall_time + pcpu->window_size);
		pcpu->avg_load_maxfreq_abs =
		    ((pcpu->avg_load_maxfreq_abs * pcpu->window_size) +
			(load_at_max_freq_abs * wall_time)) /
			(wall_time + pcpu->window_size);

		pcpu->window_size += wall_time;
	}
#else /* debug only */
	pcpu->avg_load_maxfreq_rel = load_at_max_freq_rel;
	pcpu->avg_load_maxfreq_abs = load_at_max_freq_abs;
	pcpu->window_size = wall_time;
#endif
#if 0
	mt_sched_printf(sched_log, "[%s] cpu(%u) load: abs(%u/%u) rel(%u/%u) prev_load: abs(%u) rel(%u)",
					__func__, cpu, pcpu->avg_load_maxfreq_abs, load_at_max_freq_abs,
					pcpu->avg_load_maxfreq_rel, load_at_max_freq_rel,
					prev_avg_load_abs, prev_avg_load_rel);
	mt_sched_printf(sched_log, "[%s]  wdz:%u wall:%u(%llu/%llu) idle: %u(%llu/%llu) iowait: %u(%llu/%llu)",
					__func__,
					pcpu->window_size, wall_time, cur_wall_time, prev_wall_time,
					idle_time, cur_idle_time, prev_cpu_idle,
					iowait_time, cur_iowait_time, prev_cpu_iowait);
#endif
	return 0;
}

#if 0
static void report_load_at_max_freq(bool reset, unsigned int *rel_load, unsigned int *abs_load)
{
	int cpu;
	struct cpu_load_data *pcpu;
	unsigned long flags;

	*rel_load = 0;
	*abs_load = 0;
	for_each_online_cpu(cpu) {
		pcpu = &per_cpu(cpuload, cpu);
		spin_lock_irqsave(&pcpu->cpu_load_lock, flags);
		update_average_load(pcpu->cur_freq, cpu);
		*rel_load += pcpu->avg_load_maxfreq_rel;
		*abs_load += pcpu->avg_load_maxfreq_abs;
		if (reset) {
			pcpu->avg_load_maxfreq_rel = 0;
			pcpu->avg_load_maxfreq_abs = 0;
		}
		spin_unlock_irqrestore(&pcpu->cpu_load_lock, flags);
	}
}
#endif

void sched_get_percpu_load2(int cpu, bool reset, unsigned int *rel_load, unsigned int *abs_load)
{
	struct cpu_load_data *pcpu;
	unsigned long flags;

#if 0
	if (!cpu_online(cpu))
		return 0;
#endif
	if (!rel_load || !abs_load)
		return;

	*rel_load = 0;
	*abs_load = 0;
	if (rq_info.init != 1) {
		*rel_load = 90;
		*abs_load = 90;
		return;
	}

	pcpu = &per_cpu(cpuload, cpu);
	spin_lock_irqsave(&pcpu->cpu_load_lock, flags);
	update_average_load(AVG_LOAD_UPDATE, pcpu->cur_freq, cpu);
	*rel_load = pcpu->avg_load_maxfreq_rel;
	*abs_load = pcpu->avg_load_maxfreq_abs;
	if (reset) {
		pcpu->avg_load_maxfreq_rel = 0;
		pcpu->avg_load_maxfreq_abs = 0;
	}
	spin_unlock_irqrestore(&pcpu->cpu_load_lock, flags);
}
EXPORT_SYMBOL(sched_get_percpu_load2);

unsigned int sched_get_percpu_load(int cpu, bool reset, bool use_maxfreq)
{
	int rel_load, abs_load;

	sched_get_percpu_load2(cpu, reset, &rel_load, &abs_load);
	return use_maxfreq ? abs_load : rel_load;
}
EXPORT_SYMBOL(sched_get_percpu_load);

/* #define DETECT_HTASK_HEAT */

#ifdef DETECT_HTASK_HEAT
#define MAX_HTASK_TEMPERATURE 10
static unsigned int htask_temperature;
static void __heat_refined(int *count)
{
	if (!arch_is_smp()) {
		if (*count) {
			htask_temperature += (htask_temperature < MAX_HTASK_TEMPERATURE) ? 1 : 0;
		} else {
			*count = (htask_temperature > 0) ? 1 : 0;
			htask_temperature -= (htask_temperature > 0) ? 1 : 0;
		}
	}
}
#else
static inline void __heat_refined(int *count) {}
#endif

static void __trace_out(int heavy, int cpu, struct task_struct *p)
{
#ifdef CONFIG_CPU_FREQ
#define TRACEBUF_LEN 128
	char tracebuf[TRACEBUF_LEN];

		snprintf(tracebuf, TRACEBUF_LEN, " %s cpu=%d load=%4lu cpucap=%4lu/%4lu pid=%4d name=%s",
				 heavy ? "Y" : "N",
				 cpu, p->se.avg.load_avg,
				 topology_cur_cpu_capacity(cpu), topology_max_cpu_capacity(cpu),
				 p->pid, p->comm);
		trace_sched_heavy_task(tracebuf);
#endif
#if 0
		if (unlikely(heavy))
			trace_sched_task_entity_avg(5, p, &p->se.avg);
#endif
}

static int ack_by_curcap(int cpu, int cluster_id, int max_cluster_id)
{
	unsigned long cur_cap;
	unsigned long max_cap;
	int acked = 0;
	int thrshld;

	if (!htask_cpucap_ctrl) /* disable cpucap control */
		return 1;

	if (cluster_id == max_cluster_id)
		return 1; /* skip checking for the fastest cluster */
	else if (cluster_id == 0)
		thrshld = SCHED_CAPACITY_SCALE*60/100;
	else
		thrshld = SCHED_CAPACITY_SCALE*40/100;
#ifdef CONFIG_CPU_FREQ
	cur_cap = topology_cur_cpu_capacity(cpu);
#else
	cur_cap = topology_max_cpu_capacity(cpu);
	max_cap = topology_max_cpu_capacity(cpu);
#endif
	if ((cur_cap * SCHED_CAPACITY_SCALE) >= max_cap * thrshld)
		acked = 1;
	else
		acked = 0;

	return acked;
}

static unsigned int htask_statistic;
unsigned int sched_get_nr_heavy_task_by_threshold(int cluster_id, unsigned int threshold)
{
	int cpu;
	struct task_struct *p;
	unsigned long flags;
	unsigned int count = 0;
	int is_heavy = 0;
	int clusters;
	struct cpumask cls_cpus;

	if (rq_info.init != 1)
		return 0;
	clusters = arch_get_nr_clusters();
	if (cluster_id < 0 || cluster_id >= clusters) {
		pr_warn("[%s] invalid cluster id %d\n", __func__, cluster_id);
		return 0;
	}

	arch_get_cluster_cpus(&cls_cpus, cluster_id);
	for_each_cpu(cpu, &cls_cpus) {
		if (likely(!cpu_online(cpu)))
			continue;
		raw_spin_lock_irqsave(&cpu_rq(cpu)->lock, flags);
		list_for_each_entry(p, &cpu_rq(cpu)->cfs_tasks, se.group_node) {
			is_heavy = 0;
#ifdef CONFIG_SCHED_HMP_PRIO_FILTER
			if (task_low_priority(p->prio))
				continue;
#endif
			if (p->se.avg.load_avg >= threshold) {
				is_heavy = ack_by_curcap(cpu, cluster_id, clusters-1);
				count += is_heavy ? 1 : 0;
				__trace_out(is_heavy, cpu, p);
			}
		}
		raw_spin_unlock_irqrestore(&cpu_rq(cpu)->lock, flags);
	}

	__heat_refined(&count);
	if (count)
		htask_statistic++;
	return count;
}
EXPORT_SYMBOL(sched_get_nr_heavy_task_by_threshold);

unsigned int sched_get_nr_heavy_task(void)
{
	int nr_clusters = arch_get_nr_clusters();
	int id, total_htask = 0;

	for (id = 0; id < nr_clusters; id++)
		total_htask += sched_get_nr_heavy_task_by_threshold(id, heavy_task_threshold);
	return total_htask;
}
EXPORT_SYMBOL(sched_get_nr_heavy_task);

unsigned int sched_get_nr_heavy_task2(int cluster_id)
{
	return sched_get_nr_heavy_task_by_threshold(cluster_id, heavy_task_threshold);
}
EXPORT_SYMBOL(sched_get_nr_heavy_task2);

void sched_set_heavy_task_threshold(unsigned int val)
{
	heavy_task_threshold = val;
}
EXPORT_SYMBOL(sched_set_heavy_task_threshold);

#if 0
static int cpufreq_policy_handler(struct notifier_block *nb,
			unsigned long event, void *data)
{
	int cpu = 0;
	struct cpufreq_policy *policy = data;
	struct cpu_load_data *this_cpu;

	if (event == CPUFREQ_START)
		return 0;

	if (event != CPUFREQ_INCOMPATIBLE)
		return 0;

	/* Make sure that all CPUs impacted by this policy are
	 * updated since we will only get a notification when the
	 * user explicitly changes the policy on a CPU.
	 */
	for_each_cpu(cpu, policy->cpus) {
		struct cpu_load_data *pcpu = &per_cpu(cpuload, j);

		spin_lock_irqsave(&pcpu->cpu_load_lock, flags);
		pcpu->policy_max = policy->cpuinfo.max_freq;
		spin_unlock_irqrestore(&pcpu->cpu_load_lock, flags);
	}

	return 0;
}
#endif

#ifdef CONFIG_CPU_FREQ
static int cpufreq_transition_handler(struct notifier_block *nb,
			unsigned long val, void *data)
{
	struct cpufreq_freqs *freqs = data;
	struct cpu_load_data *this_cpu = &per_cpu(cpuload, freqs->cpu);
	int j;
	unsigned long flags;

	if (rq_info.init != 1)
		return 0;

	switch (val) {
	case CPUFREQ_POSTCHANGE:
		for_each_cpu(j, this_cpu->related_cpus) {
			struct cpu_load_data *pcpu = &per_cpu(cpuload, j);
			/* flush previous laod */
			if (pcpu->policy_max == 1) {
				struct cpufreq_policy cpu_policy;

				if (!cpufreq_get_policy(&cpu_policy, j)) {
					spin_lock_irqsave(&pcpu->cpu_load_lock, flags);
					pcpu->policy_max = cpu_policy.cpuinfo.max_freq;
					spin_unlock_irqrestore(&pcpu->cpu_load_lock, flags);
				}
			}

			spin_lock_irqsave(&pcpu->cpu_load_lock, flags);
				if (cpu_online(j))
					update_average_load(AVG_LOAD_UPDATE, freqs->old, freqs->cpu);
			pcpu->cur_freq = cpufreq_variant ? freqs->new : pcpu->policy_max;
			spin_unlock_irqrestore(&pcpu->cpu_load_lock, flags);
		}
		break;
	}
	return 0;
}
#endif /* CONFIG_CPU_FREQ */

static int cpu_hotplug_handler(struct notifier_block *nb,
			unsigned long val, void *data)
{
	unsigned int cpu = (unsigned long)data;
	struct cpu_load_data *this_cpu = &per_cpu(cpuload, cpu);
	unsigned long flags;
#ifdef CONFIG_CPU_FREQ
	unsigned int i;
#endif
	if (rq_info.init != 1)
		return NOTIFY_OK;

	switch (val) {
	case CPU_ONLINE:
		if (!this_cpu->cur_freq)
			this_cpu->cur_freq = cpufreq_quick_get(cpu);
#ifdef CONFIG_CPU_FREQ
		for_each_cpu(i, cpu_online_mask) {
			struct cpu_load_data *cld = &per_cpu(cpuload, i);
			struct cpufreq_policy *cpu_policy  = &per_cpu(cpupolicy, i);

			cpufreq_get_policy(cpu_policy, i);
			cpumask_copy(cld->related_cpus, cpu_policy->cpus);
		}
#endif
		/* cpu_online()=0 here, count cpu offline period as idle */
		spin_lock_irqsave(&this_cpu->cpu_load_lock, flags);
		update_average_load(AVG_LOAD_IGNORE, 0, cpu);
		spin_unlock_irqrestore(&this_cpu->cpu_load_lock, flags);
		break;
	case CPU_ONLINE_FROZEN:
	case CPU_UP_PREPARE:
		/* cpu_online()=0 here, count cpu offline period as idle */
		spin_lock_irqsave(&this_cpu->cpu_load_lock, flags);
		update_average_load(AVG_LOAD_IGNORE, 0, cpu);
		spin_unlock_irqrestore(&this_cpu->cpu_load_lock, flags);
		break;
	case CPU_DOWN_PREPARE:
		/* cpu_online()=1 here, flush previous load */
		spin_lock_irqsave(&this_cpu->cpu_load_lock, flags);
		update_average_load(AVG_LOAD_UPDATE, this_cpu->cur_freq, cpu);
		spin_unlock_irqrestore(&this_cpu->cpu_load_lock, flags);
		break;
	}
	return NOTIFY_OK;
}

static int system_suspend_handler(struct notifier_block *nb,
				unsigned long val, void *data)
{
	switch (val) {
	case PM_POST_HIBERNATION:
	case PM_POST_SUSPEND:
	case PM_POST_RESTORE:
		rq_info.hotplug_disabled = 0;
		break;
	case PM_HIBERNATION_PREPARE:
	case PM_SUSPEND_PREPARE:
		rq_info.hotplug_disabled = 1;
		break;
	default:
		return NOTIFY_DONE;
	}
	return NOTIFY_OK;
}


static ssize_t hotplug_disable_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	unsigned int val = 0;

	val = rq_info.hotplug_disabled;
	return snprintf(buf, MAX_LONG_SIZE, "%d\n", val);
}

static struct kobj_attribute hotplug_disabled_attr = __ATTR_RO(hotplug_disable);

#ifdef CONFIG_MTK_SCHED_RQAVG_US_ENABLE_WQ
static void def_work_fn(struct work_struct *work)
{
	int64_t diff;

	diff = ktime_to_ns(ktime_get()) - rq_info.def_start_time;
	do_div(diff, 1000 * 1000);
	rq_info.def_interval = (unsigned int) diff;

	/* Notify polling threads on change of value */
	sysfs_notify(rq_info.kobj, NULL, "def_timer_ms");
}
#endif

static ssize_t run_queue_avg_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	unsigned int val = 0;
	unsigned long flags = 0;

	spin_lock_irqsave(&rq_lock, flags);
	/* rq avg currently available only on one core */
	val = rq_info.rq_avg;
	rq_info.rq_avg = 0;
	spin_unlock_irqrestore(&rq_lock, flags);

	return snprintf(buf, PAGE_SIZE, "%d.%d\n", val/10, val%10);
}

static struct kobj_attribute run_queue_avg_attr = __ATTR_RO(run_queue_avg);

static ssize_t show_run_queue_poll_ms(struct kobject *kobj,
				      struct kobj_attribute *attr, char *buf)
{
	int ret = 0;
	unsigned long flags = 0;

	spin_lock_irqsave(&rq_lock, flags);
	ret = snprintf(buf, MAX_LONG_SIZE, "%u\n",
		       jiffies_to_msecs(rq_info.rq_poll_jiffies));
	spin_unlock_irqrestore(&rq_lock, flags);

	return ret;
}

static ssize_t store_run_queue_poll_ms(struct kobject *kobj,
				       struct kobj_attribute *attr,
				       const char *buf, size_t count)
{
	unsigned int val = 0;
	unsigned long flags = 0;
	static DEFINE_MUTEX(lock_poll_ms);

	mutex_lock(&lock_poll_ms);

	spin_lock_irqsave(&rq_lock, flags);
	if (sscanf(buf, "%iu", &val) != 0)
		rq_info.rq_poll_jiffies = msecs_to_jiffies(val);
	spin_unlock_irqrestore(&rq_lock, flags);

	mutex_unlock(&lock_poll_ms);

	return count;
}

static struct kobj_attribute run_queue_poll_ms_attr =
	__ATTR(run_queue_poll_ms, S_IWUSR | S_IRUSR, show_run_queue_poll_ms,
	store_run_queue_poll_ms);

static ssize_t show_def_timer_ms(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, MAX_LONG_SIZE, "%u\n", rq_info.def_interval);
}

static ssize_t store_def_timer_ms(struct kobject *kobj,
				  struct kobj_attribute *attr, const char *buf, size_t count)
{
	unsigned int val = 0;

	if (sscanf(buf, "%iu", &val) != 0)
		rq_info.def_timer_jiffies = msecs_to_jiffies(val);

	rq_info.def_start_time = ktime_to_ns(ktime_get());
	return count;
}

static struct kobj_attribute def_timer_ms_attr =
	__ATTR(def_timer_ms, S_IWUSR | S_IRUSR, show_def_timer_ms,
	store_def_timer_ms);

static ssize_t cpu_normalized_load_show(struct kobject *kobj,
					struct kobj_attribute *attr, char *buf)
{
	int cpu;
	unsigned int len = 0;
	unsigned int load_rel = 0, load_abs = 0;
	unsigned int max_len = 4096;
	int i;

	for_each_possible_cpu(cpu) {
		sched_get_percpu_load2(cpu, 0, &load_rel, &load_abs);
		len += snprintf(buf+len, max_len-len, "cpu(%d): load(rel/abs) = %u/%u\n", cpu, load_rel, load_abs);
#if 0
		unsigned int idle_time, wall_time, iowait_time;
		struct cpu_load_data *pcpu = &per_cpu(cpuload, cpu);

		idle_time = get_cpu_idle_time(cpu, &wall_time, 0);
		iowait_time = get_cpu_iowait_time(cpu, &wall_time);
		len += snprintf(buf+len, max_len-len, "curr idle=%u, io=%u, wall=%u\n",
			(unsigned int)idle_time,
			(unsigned int)iowait_time,
			(unsigned int)wall_time);
		len += snprintf(buf+len, max_len-len,
						"prev idle=%u, io=%u, wall=%u, l(rel/abs)=%u/%u, w=%u, f=%u m=%u, %s\n",
			(unsigned int)pcpu->prev_cpu_idle,
			(unsigned int)pcpu->prev_cpu_iowait,
			(unsigned int)pcpu->prev_cpu_wall,
			pcpu->avg_load_maxfreq_rel,
			pcpu->avg_load_maxfreq_abs,
			pcpu->window_size,
			pcpu->cur_freq,
			pcpu->policy_max,
			(unsigned int)(cpu_online(cpu))?"on":"off");
#endif
	}
	len += snprintf(buf+len, max_len-len, "htask_threshold=%d, total_htask#=%d\n",
		heavy_task_threshold, htask_statistic);
	for (i = 0; i < arch_get_nr_clusters(); i++)
		len += snprintf(buf+len, max_len-len, "cluster%d: current_htask#=%u\n", i, sched_get_nr_heavy_task2(i));

	return len;
}
static struct kobj_attribute cpu_normalized_load_attr = __ATTR_RO(cpu_normalized_load);

static ssize_t show_heavy_tasks(struct kobject *kobj,
					struct kobj_attribute *attr, char *buf)
{
	unsigned int len = 0;
	unsigned int max_len = 4096;
	int i;

	len += snprintf(buf+len, max_len-len, "htask_cpucap_ctrl=%d, htask_threshold=%d, total_htask#=%d\n",
					htask_cpucap_ctrl, heavy_task_threshold, htask_statistic);
	for (i = 0; i < arch_get_nr_clusters(); i++)
		len += snprintf(buf+len, max_len-len, "cluster%d: current_htask#=%u\n", i, sched_get_nr_heavy_task2(i));

	return len;
}

static ssize_t store_heavy_task_threshold(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	unsigned int val = 0;

	if (sscanf(buf, "%iu", &val) != 0)
		sched_set_heavy_task_threshold(val);

	return count;
}

static struct kobj_attribute htasks_attr =
	__ATTR(htasks, S_IWUSR | S_IRUSR, show_heavy_tasks,
			store_heavy_task_threshold);

static ssize_t htask_cpucap_ctrl_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	unsigned int val = 0;

	if (sscanf(buf, "%iu", &val) != 0)
		htask_cpucap_ctrl = (val == 0 ? 0 : 1);

	return count;
}
static struct kobj_attribute htask_cpucap_ctrl_attr = __ATTR_WO(htask_cpucap_ctrl);

static struct attribute *rq_attrs[] = {
	&cpu_normalized_load_attr.attr,
	&def_timer_ms_attr.attr,
	&run_queue_avg_attr.attr,
	&run_queue_poll_ms_attr.attr,
	&hotplug_disabled_attr.attr,
	&htasks_attr.attr,
	&htask_cpucap_ctrl_attr.attr,
	NULL,
};

static struct attribute_group rq_attr_group = {
	.attrs = rq_attrs,
};

static int init_rq_attribs(void)
{
	int err;

	rq_info.rq_avg = 0;
	rq_info.attr_group = &rq_attr_group;

	/* Create /sys/devices/system/cpu/rq-stats/... */
	rq_info.kobj = kobject_create_and_add("rq-stats", &cpu_subsys.dev_root->kobj);
	if (!rq_info.kobj)
		return -ENOMEM;

	err = sysfs_create_group(rq_info.kobj, rq_info.attr_group);
	if (err)
		kobject_put(rq_info.kobj);
	else
		kobject_uevent(rq_info.kobj, KOBJ_ADD);

	return err;
}

static int __init rq_stats_init(void)
{
	int ret = 0;
	int i;
#ifdef CONFIG_CPU_FREQ
	struct cpufreq_policy *cpu_policy;

	cpu_policy = kmalloc(sizeof(struct cpufreq_policy), GFP_KERNEL);
	if (!cpu_policy) {
		rq_info.init = 0;
		return -ENODATA;
	}
#endif
	/* Bail out if this is not an SMP Target */
#ifndef CONFIG_SMP
#ifdef CONFIG_CPU_FREQ
	kfree(cpu_policy);
#endif
	rq_info.init = 0;
	return -ENODATA;
#endif

#ifdef CONFIG_MTK_SCHED_RQAVG_US_ENABLE_WQ
	rq_wq = create_singlethread_workqueue("rq_stats");

	if (!rq_wq)
		return -ENODATA;

	INIT_WORK(&rq_info.def_timer_work, def_work_fn);
#endif

	spin_lock_init(&rq_lock);
	rq_info.rq_poll_jiffies = DEFAULT_RQ_POLL_JIFFIES;
	rq_info.def_timer_jiffies = DEFAULT_DEF_TIMER_JIFFIES;
	rq_info.rq_poll_last_jiffy = 0;
	rq_info.def_timer_last_jiffy = 0;
	rq_info.hotplug_disabled = 0;
	ret = init_rq_attribs();

	for_each_possible_cpu(i) {
		struct cpu_load_data *pcpu = &per_cpu(cpuload, i);

		spin_lock_init(&pcpu->cpu_load_lock);
		pcpu->cur_freq = pcpu->policy_max = 1;
#ifdef CONFIG_CPU_FREQ
		if (!cpufreq_get_policy(cpu_policy, i)) {
			pcpu->policy_max = cpu_policy->cpuinfo.max_freq;
			if (cpu_online(i))
				pcpu->cur_freq = cpufreq_quick_get(i);
			cpumask_copy(pcpu->related_cpus, cpu_policy->cpus);
		}
#endif /* CONFIG_CPU_FREQ */
		if (!cpufreq_variant)
			pcpu->cur_freq = pcpu->policy_max;
	}
#ifdef CONFIG_CPU_FREQ
	freq_transition.notifier_call = cpufreq_transition_handler;
	cpufreq_register_notifier(&freq_transition, CPUFREQ_TRANSITION_NOTIFIER);
#endif /* CONFIG_CPU_FREQ */
	cpu_hotplug.notifier_call = cpu_hotplug_handler;
	register_hotcpu_notifier(&cpu_hotplug);

	rq_info.init = 1;
#ifdef CONFIG_CPU_FREQ
	kfree(cpu_policy);
#endif

	return ret;
}
late_initcall(rq_stats_init);

static int __init rq_stats_early_init(void)
{

	/* Bail out if this is not an SMP Target */
#ifndef CONFIG_SMP
		rq_info.init = 0;
		return -ENODATA;
#endif

	pm_notifier(system_suspend_handler, 0);
	return 0;
}
core_initcall(rq_stats_early_init);
