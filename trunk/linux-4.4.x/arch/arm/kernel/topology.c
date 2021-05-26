/*
 * arch/arm/kernel/topology.c
 *
 * Copyright (C) 2011 Linaro Limited.
 * Written by: Vincent Guittot
 *
 * based on arch/sh/kernel/topology.c
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 */

#include <linux/cpu.h>
#include <linux/cpumask.h>
#include <linux/export.h>
#include <linux/init.h>
#include <linux/percpu.h>
#include <linux/node.h>
#include <linux/nodemask.h>
#include <linux/of.h>
#include <linux/sched.h>
#include <linux/slab.h>

#include <asm/cputype.h>
#include <asm/topology.h>

/*
 * cpu capacity scale management
 */

/*
 * cpu capacity table
 * This per cpu data structure describes the relative capacity of each core.
 * On a heteregenous system, cores don't have the same computation capacity
 * and we reflect that difference in the cpu_capacity field so the scheduler
 * can take this difference into account during load balance. A per cpu
 * structure is preferred because each CPU updates its own cpu_capacity field
 * during the load balance except for idle cores. One idle core is selected
 * to run the rebalance_domains for all idle cores and the cpu_capacity can be
 * updated during this sequence.
 */
static DEFINE_PER_CPU(unsigned long, cpu_scale);

unsigned long scale_cpu_capacity(struct sched_domain *sd, int cpu)
{
#ifdef CONFIG_CPU_FREQ
	unsigned long max_freq_scale = cpufreq_scale_max_freq_capacity(cpu);

	return per_cpu(cpu_scale, cpu) * max_freq_scale >> SCHED_CAPACITY_SHIFT;
#else
	return per_cpu(cpu_scale, cpu);
#endif
}

static void set_capacity_scale(unsigned int cpu, unsigned long capacity)
{
	per_cpu(cpu_scale, cpu) = capacity;
}

#ifdef CONFIG_OF
struct cpu_efficiency {
	const char *compatible;
	unsigned long efficiency;
};

#ifdef CONFIG_MTK_CPU_TOPOLOGY
static int __init get_cpu_for_node(struct device_node *node)
{
	struct device_node *cpu_node;
	int cpu;

	cpu_node = of_parse_phandle(node, "cpu", 0);
	if (!cpu_node)
		return -1;

	for_each_possible_cpu(cpu) {
		if (of_get_cpu_node(cpu, NULL) == cpu_node) {
			of_node_put(cpu_node);
			return cpu;
		}
	}

	pr_crit("Unable to find CPU node for %s\n", cpu_node->full_name);

	of_node_put(cpu_node);
	return -1;
}

static int __init parse_core(struct device_node *core, int cluster_id,
			     int core_id)
{
	char name[10];
	bool leaf = true;
	int i = 0;
	int cpu;
	struct device_node *t;

	do {
		snprintf(name, sizeof(name), "thread%d", i);
		t = of_get_child_by_name(core, name);
		if (t) {
			leaf = false;
			cpu = get_cpu_for_node(t);
			if (cpu >= 0) {
				cpu_topology[cpu].socket_id = cluster_id;
				cpu_topology[cpu].core_id = core_id;
				cpu_topology[cpu].thread_id = i;
			} else {
				pr_err("%s: Can't get CPU for thread\n",
				       t->full_name);
				of_node_put(t);
				return -EINVAL;
			}
			of_node_put(t);
		}
		i++;
	} while (t);

	cpu = get_cpu_for_node(core);
	if (cpu >= 0) {
		if (!leaf) {
			pr_err("%s: Core has both threads and CPU\n",
			       core->full_name);
			return -EINVAL;
		}

		cpu_topology[cpu].socket_id = cluster_id;
		cpu_topology[cpu].core_id = core_id;
	} else if (leaf) {
		pr_err("%s: Can't get CPU for leaf core\n", core->full_name);
		return -EINVAL;
	}

	return 0;
}

static int __init parse_cluster(struct device_node *cluster, int depth)
{
	char name[10];
	bool leaf = true;
	bool has_cores = false;
	int core_id = 0;

	static int cluster_id __initdata;
	struct device_node *c;
	int i, ret;

	/*
	 * First check for child clusters; we currently ignore any
	 * information about the nesting of clusters and present the
	 * scheduler with a flat list of them.
	 */
	i = 0;
	do {
		snprintf(name, sizeof(name), "cluster%d", i);
		c = of_get_child_by_name(cluster, name);
		if (c) {
			leaf = false;
			ret = parse_cluster(c, depth + 1);
			of_node_put(c);
			if (ret != 0)
				return ret;
		}
		i++;
	} while (c);

	/* Now check for cores */
	i = 0;
	do {
		snprintf(name, sizeof(name), "core%d", i);
		c = of_get_child_by_name(cluster, name);
		if (c) {
			has_cores = true;

			if (depth == 0) {
				pr_err("%s: cpu-map children should be clusters\n",
				       c->full_name);
				of_node_put(c);
				return -EINVAL;
			}

			if (leaf) {
				ret = parse_core(c, cluster_id, core_id++);
			} else {
				pr_err("%s: Non-leaf cluster with core %s\n",
				       cluster->full_name, name);
				ret = -EINVAL;
			}

			of_node_put(c);
			if (ret != 0)
				return ret;
		}
		i++;
	} while (c);

	if (leaf && !has_cores)
		pr_warn("%s: empty cluster\n", cluster->full_name);

	if (leaf)
		cluster_id++;

	return 0;
}
#endif

/*
 * Table of relative efficiency of each processors
 * The efficiency value must fit in 20bit and the final
 * cpu_scale value must be in the range
 *   0 < cpu_scale < SCHED_CAPACITY_SCALE.
 * Processors that are not defined in the table,
 * use the default SCHED_CAPACITY_SCALE value for cpu_scale.
 */
static const struct cpu_efficiency table_efficiency[] = {
	{"arm,cortex-a15", 3891},
	{"arm,cortex-a17", 3276},
	{"arm,cortex-a12", 3276},
	{"arm,cortex-a53", 2520},
	{"arm,cortex-a7",  2048},
	{NULL, },
};

static unsigned long *__cpu_capacity;
#define cpu_capacity(cpu)	__cpu_capacity[cpu]

static unsigned long max_cpu_perf, min_cpu_perf;

#ifdef CONFIG_MTK_CPU_TOPOLOGY
static int __init parse_dt_topology(void)
{
	struct device_node *cn, *map;
	int ret = 0;
	int cpu;

	cn = of_find_node_by_path("/cpus");
	if (!cn) {
		pr_err("No CPU information found in DT\n");
		return 0;
	}

	/*
	 * When topology is provided cpu-map is essentially a root
	 * cluster with restricted subnodes.
	 */
	map = of_get_child_by_name(cn, "cpu-map");
	if (!map)
		goto out;

	ret = parse_cluster(map, 0);
	if (ret != 0)
		goto out_map;

	/*
	 * Check that all cores are in the topology; the SMP code will
	 * only mark cores described in the DT as possible.
	 */
	for_each_possible_cpu(cpu)
		if (cpu_topology[cpu].socket_id == -1)
			ret = -EINVAL;

out_map:
	of_node_put(map);
out:
	of_node_put(cn);
	return ret;
}

/*
 * Iterate all CPUs' descriptor in DT and compute the efficiency
 * (as per table_efficiency). Calculate the max cpu performance too.
 */

static void parse_dt_cpu_capacity(void)
{
	const struct cpu_efficiency *cpu_eff;
	struct device_node *cn = NULL;
	int cpu = 0, i = 0;

	__cpu_capacity = kcalloc(nr_cpu_ids, sizeof(*__cpu_capacity),
				 GFP_NOWAIT);

	min_cpu_perf = ULONG_MAX;
	max_cpu_perf = 0;

	for_each_possible_cpu(cpu) {
		const u32 *rate;
		int len;
		unsigned long cpu_perf;

		/* too early to use cpu->of_node */
		cn = of_get_cpu_node(cpu, NULL);
		if (!cn) {
			pr_err("missing device node for CPU %d\n", cpu);
			continue;
		}

		for (cpu_eff = table_efficiency; cpu_eff->compatible; cpu_eff++)
			if (of_device_is_compatible(cn, cpu_eff->compatible))
				break;

		if (cpu_eff->compatible == NULL)
			continue;

		rate = of_get_property(cn, "clock-frequency", &len);
		if (!rate || len != 4) {
			pr_err("%s missing clock-frequency property\n",
				cn->full_name);
			continue;
		}

		cpu_perf = ((be32_to_cpup(rate)) >> 20) * cpu_eff->efficiency;
		cpu_capacity(cpu) = cpu_perf;
		max_cpu_perf = max(max_cpu_perf, cpu_perf);
		min_cpu_perf = min(min_cpu_perf, cpu_perf);
		i++;
	}

	if (i < num_possible_cpus()) {
		max_cpu_perf = 0;
		min_cpu_perf = 0;
	}
}

static const struct sched_group_energy * const cpu_core_energy(int cpu);

/*
 * Look for a customed capacity of a CPU in the cpu_capacity table during the
 * boot. The update of all CPUs is in O(n^2) for heteregeneous system but the
 * function returns directly for SMP systems or if there is no complete set
 * of cpu efficiency, clock frequency data for each cpu.
 */
static void update_cpu_capacity(unsigned int cpu)
{
	unsigned long capacity = cpu_capacity(cpu);

	if (!capacity || !max_cpu_perf) {
		cpu_capacity(cpu) = 0;
		return;
	}

	capacity *= SCHED_CAPACITY_SCALE;
	capacity /= max_cpu_perf;

	set_capacity_scale(cpu, capacity);

	pr_info("CPU%u: update cpu_capacity %lu\n",
		cpu, arch_scale_cpu_capacity(NULL, cpu));
}
#else /* CONFIG_MTK_CPU_TOPOLOGY */
static unsigned long middle_capacity = 1;
/*
 * Iterate all CPUs' descriptor in DT and compute the efficiency
 * (as per table_efficiency). Also calculate a middle efficiency
 * as close as possible to  (max{eff_i} - min{eff_i}) / 2
 * This is later used to scale the cpu_capacity field such that an
 * 'average' CPU is of middle capacity. Also see the comments near
 * table_efficiency[] and update_cpu_capacity().
 */
static void __init parse_dt_topology(void)
{
	const struct cpu_efficiency *cpu_eff;
	struct device_node *cn = NULL;
	unsigned long min_capacity = ULONG_MAX;
	unsigned long max_capacity = 0;
	unsigned long capacity = 0;
	int cpu = 0;

	__cpu_capacity = kcalloc(nr_cpu_ids, sizeof(*__cpu_capacity),
				 GFP_NOWAIT);

	for_each_possible_cpu(cpu) {
		const u32 *rate;
		int len;

		/* too early to use cpu->of_node */
		cn = of_get_cpu_node(cpu, NULL);
		if (!cn) {
			pr_err("missing device node for CPU %d\n", cpu);
			continue;
		}

		for (cpu_eff = table_efficiency; cpu_eff->compatible; cpu_eff++)
			if (of_device_is_compatible(cn, cpu_eff->compatible))
				break;

		if (cpu_eff->compatible == NULL)
			continue;

		rate = of_get_property(cn, "clock-frequency", &len);
		if (!rate || len != 4) {
			pr_err("%s missing clock-frequency property\n",
				cn->full_name);
			continue;
		}

		capacity = ((be32_to_cpup(rate)) >> 20) * cpu_eff->efficiency;

		/* Save min capacity of the system */
		if (capacity < min_capacity)
			min_capacity = capacity;

		/* Save max capacity of the system */
		if (capacity > max_capacity)
			max_capacity = capacity;

		cpu_capacity(cpu) = capacity;
	}

	/* If min and max capacities are equals, we bypass the update of the
	 * cpu_scale because all CPUs have the same capacity. Otherwise, we
	 * compute a middle_capacity factor that will ensure that the capacity
	 * of an 'average' CPU of the system will be as close as possible to
	 * SCHED_CAPACITY_SCALE, which is the default value, but with the
	 * constraint explained near table_efficiency[].
	 */
	if (4*max_capacity < (3*(max_capacity + min_capacity)))
		middle_capacity = (min_capacity + max_capacity)
				>> (SCHED_CAPACITY_SHIFT+1);
	else
		middle_capacity = ((max_capacity / 3)
				>> (SCHED_CAPACITY_SHIFT-1)) + 1;

}

static const struct sched_group_energy * const cpu_core_energy(int cpu);
static void update_cpu_capacity(unsigned int cpu)
{
	unsigned long capacity = SCHED_CAPACITY_SCALE;

	if (cpu_core_energy(cpu)) {
		int max_cap_idx = cpu_core_energy(cpu)->nr_cap_states - 1;

		capacity = cpu_core_energy(cpu)->cap_states[max_cap_idx].cap;
	}

	set_capacity_scale(cpu, capacity);

	pr_info("CPU%u: update cpu_capacity %lu\n",
		cpu, arch_scale_cpu_capacity(NULL, cpu));
}

#endif /* CONFIG_MTK_CPU_TOPOLOGY */
#else
static inline void parse_dt_topology(void) {}
static inline void update_cpu_capacity(unsigned int cpuid) {}
#endif

#if 0
/*
 * Scheduler load-tracking scale-invariance
 *
 * Provides the scheduler with a scale-invariance correction factor that
 * compensates for frequency scaling.
 */

static DEFINE_PER_CPU(atomic_long_t, cpu_freq_capacity);
static DEFINE_PER_CPU(atomic_long_t, cpu_max_freq);

/* cpufreq callback function setting current cpu frequency */
void arch_scale_set_curr_freq(int cpu, unsigned long freq)
{
	unsigned long max = atomic_long_read(&per_cpu(cpu_max_freq, cpu));
	unsigned long curr;

	if (!max)
		return;

	curr = (freq * SCHED_CAPACITY_SCALE) / max;

	atomic_long_set(&per_cpu(cpu_freq_capacity, cpu), curr);
}

/* cpufreq callback function setting max cpu frequency */
void arch_scale_set_max_freq(int cpu, unsigned long freq)
{
	atomic_long_set(&per_cpu(cpu_max_freq, cpu), freq);
}

unsigned long arch_scale_freq_capacity(struct sched_domain *sd, int cpu)
{
	unsigned long curr = atomic_long_read(&per_cpu(cpu_freq_capacity, cpu));

	if (!curr)
		return SCHED_CAPACITY_SCALE;

	return curr;
}
#endif
unsigned long arch_get_max_cpu_capacity(int cpu)
{
	return per_cpu(cpu_scale, cpu);
}
#ifdef CONFIG_CPU_FREQ
unsigned long arch_get_cur_cpu_capacity(int cpu)
{
	unsigned long scale_freq =  arch_scale_freq_capacity(NULL, cpu);

	if (!scale_freq)
		scale_freq = SCHED_CAPACITY_SCALE;

	return (per_cpu(cpu_scale, cpu) * scale_freq / SCHED_CAPACITY_SCALE);
}
#endif
/*
 * cpu topology table
 */
struct cputopo_arm cpu_topology[NR_CPUS];
EXPORT_SYMBOL_GPL(cpu_topology);

const struct cpumask *cpu_coregroup_mask(int cpu)
{
	return &cpu_topology[cpu].core_sibling;
}

/*
 * The current assumption is that we can power gate each core independently.
 * This will be superseded by DT binding once available.
 */
const struct cpumask *cpu_corepower_mask(int cpu)
{
	return &cpu_topology[cpu].thread_sibling;
}

static void update_siblings_masks(unsigned int cpuid)
{
	struct cputopo_arm *cpu_topo, *cpuid_topo = &cpu_topology[cpuid];
	int cpu;

	/* update core and thread sibling masks */
	for_each_possible_cpu(cpu) {
		cpu_topo = &cpu_topology[cpu];

		if (cpuid_topo->socket_id != cpu_topo->socket_id)
			continue;

		cpumask_set_cpu(cpuid, &cpu_topo->core_sibling);
		if (cpu != cpuid)
			cpumask_set_cpu(cpu, &cpuid_topo->core_sibling);

		if (cpuid_topo->core_id != cpu_topo->core_id)
			continue;

		cpumask_set_cpu(cpuid, &cpu_topo->thread_sibling);
		if (cpu != cpuid)
			cpumask_set_cpu(cpu, &cpuid_topo->thread_sibling);
	}
	smp_wmb();
}

/*
 * store_cpu_topology is called at boot when only one cpu is running
 * and with the mutex cpu_hotplug.lock locked, when several cpus have booted,
 * which prevents simultaneous write access to cpu_topology array
 */
void store_cpu_topology(unsigned int cpuid)
{
	struct cputopo_arm *cpuid_topo = &cpu_topology[cpuid];
	unsigned int mpidr;

	mpidr = read_cpuid_mpidr();

	/* If the cpu topology has been already set, just return */
	if (cpuid_topo->socket_id != -1)
#ifdef CONFIG_MTK_CPU_TOPOLOGY
		goto topology_populated;
#else
		return;
#endif
	/* create cpu topology mapping */
	if ((mpidr & MPIDR_SMP_BITMASK) == MPIDR_SMP_VALUE) {
		/*
		 * This is a multiprocessor system
		 * multiprocessor format & multiprocessor mode field are set
		 */

		if (mpidr & MPIDR_MT_BITMASK) {
			/* core performance interdependency */
			cpuid_topo->thread_id = MPIDR_AFFINITY_LEVEL(mpidr, 0);
			cpuid_topo->core_id = MPIDR_AFFINITY_LEVEL(mpidr, 1);
			cpuid_topo->socket_id = MPIDR_AFFINITY_LEVEL(mpidr, 2);
		} else {
			/* largely independent cores */
			cpuid_topo->thread_id = -1;
			cpuid_topo->core_id = MPIDR_AFFINITY_LEVEL(mpidr, 0);
			cpuid_topo->socket_id = MPIDR_AFFINITY_LEVEL(mpidr, 1);
		}
	} else {
		/*
		 * This is an uniprocessor system
		 * we are in multiprocessor format but uniprocessor system
		 * or in the old uniprocessor format
		 */
		cpuid_topo->thread_id = -1;
		cpuid_topo->core_id = 0;
		cpuid_topo->socket_id = -1;
	}

	cpuid_topo->partno = read_cpuid_part();
#ifdef CONFIG_MTK_CPU_TOPOLOGY
topology_populated:
#endif
	update_siblings_masks(cpuid);

	update_cpu_capacity(cpuid);

	pr_info("CPU%u: thread %d, cpu %d, socket %d, mpidr %x\n",
		cpuid, cpu_topology[cpuid].thread_id,
		cpu_topology[cpuid].core_id,
		cpu_topology[cpuid].socket_id, mpidr);
}

/*
 * ARM TC2 specific energy cost model data. There are no unit requirements for
 * the data. Data can be normalized to any reference point, but the
 * normalization must be consistent. That is, one bogo-joule/watt must be the
 * same quantity for all data, but we don't care what it is.
 */
static struct idle_state idle_states_cluster_a7[] = {
	 { .power = 25 }, /* arch_cpu_idle() (active idle) = WFI */
	 { .power = 25 }, /* WFI */
	 { .power = 10 }, /* cluster-sleep-l */
	};

static struct idle_state idle_states_cluster_a15[] = {
	 { .power = 70 }, /* arch_cpu_idle() (active idle) = WFI */
	 { .power = 70 }, /* WFI */
	 { .power = 25 }, /* cluster-sleep-b */
	};

static struct capacity_state cap_states_cluster_a7[] = {
	/* Cluster only power */
	 { .cap =  150, .power = 2967, }, /*  350 MHz */
	 { .cap =  172, .power = 2792, }, /*  400 MHz */
	 { .cap =  215, .power = 2810, }, /*  500 MHz */
	 { .cap =  258, .power = 2815, }, /*  600 MHz */
	 { .cap =  301, .power = 2919, }, /*  700 MHz */
	 { .cap =  344, .power = 2847, }, /*  800 MHz */
	 { .cap =  387, .power = 3917, }, /*  900 MHz */
	 { .cap =  430, .power = 4905, }, /* 1000 MHz */
	};

static struct capacity_state cap_states_cluster_a15[] = {
	/* Cluster only power */
	 { .cap =  426, .power =  7920, }, /*  500 MHz */
	 { .cap =  512, .power =  8165, }, /*  600 MHz */
	 { .cap =  597, .power =  8172, }, /*  700 MHz */
	 { .cap =  682, .power =  8195, }, /*  800 MHz */
	 { .cap =  768, .power =  8265, }, /*  900 MHz */
	 { .cap =  853, .power =  8446, }, /* 1000 MHz */
	 { .cap =  938, .power = 11426, }, /* 1100 MHz */
	 { .cap = 1024, .power = 15200, }, /* 1200 MHz */
	};

static struct sched_group_energy energy_cluster_a7 = {
	  .nr_idle_states = ARRAY_SIZE(idle_states_cluster_a7),
	  .idle_states    = idle_states_cluster_a7,
	  .nr_cap_states  = ARRAY_SIZE(cap_states_cluster_a7),
	  .cap_states     = cap_states_cluster_a7,
};

static struct sched_group_energy energy_cluster_a15 = {
	  .nr_idle_states = ARRAY_SIZE(idle_states_cluster_a15),
	  .idle_states    = idle_states_cluster_a15,
	  .nr_cap_states  = ARRAY_SIZE(cap_states_cluster_a15),
	  .cap_states     = cap_states_cluster_a15,
};

static struct idle_state idle_states_core_a7[] = {
	 { .power = 0 }, /* arch_cpu_idle (active idle) = WFI */
	 { .power = 0 }, /* WFI */
	 { .power = 0 }, /* cluster-sleep-l */
	};

static struct idle_state idle_states_core_a15[] = {
	 { .power = 0 }, /* arch_cpu_idle (active idle) = WFI */
	 { .power = 0 }, /* WFI */
	 { .power = 0 }, /* cluster-sleep-b */
	};

static struct capacity_state cap_states_core_a7[] = {
	/* Power per cpu */
	 { .cap =  150, .power =  187, }, /*  350 MHz */
	 { .cap =  172, .power =  275, }, /*  400 MHz */
	 { .cap =  215, .power =  334, }, /*  500 MHz */
	 { .cap =  258, .power =  407, }, /*  600 MHz */
	 { .cap =  301, .power =  447, }, /*  700 MHz */
	 { .cap =  344, .power =  549, }, /*  800 MHz */
	 { .cap =  387, .power =  761, }, /*  900 MHz */
	 { .cap =  430, .power = 1024, }, /* 1000 MHz */
	};

static struct capacity_state cap_states_core_a15[] = {
	/* Power per cpu */
	 { .cap =  426, .power = 2021, }, /*  500 MHz */
	 { .cap =  512, .power = 2312, }, /*  600 MHz */
	 { .cap =  597, .power = 2756, }, /*  700 MHz */
	 { .cap =  682, .power = 3125, }, /*  800 MHz */
	 { .cap =  768, .power = 3524, }, /*  900 MHz */
	 { .cap =  853, .power = 3846, }, /* 1000 MHz */
	 { .cap =  938, .power = 5177, }, /* 1100 MHz */
	 { .cap = 1024, .power = 6997, }, /* 1200 MHz */
	};

static struct sched_group_energy energy_core_a7 = {
	  .nr_idle_states = ARRAY_SIZE(idle_states_core_a7),
	  .idle_states    = idle_states_core_a7,
	  .nr_cap_states  = ARRAY_SIZE(cap_states_core_a7),
	  .cap_states     = cap_states_core_a7,
};

static struct sched_group_energy energy_core_a15 = {
	  .nr_idle_states = ARRAY_SIZE(idle_states_core_a15),
	  .idle_states    = idle_states_core_a15,
	  .nr_cap_states  = ARRAY_SIZE(cap_states_core_a15),
	  .cap_states     = cap_states_core_a15,
};

/* sd energy functions */
static inline
const struct sched_group_energy * const cpu_cluster_energy(int cpu)
{
	return cpu_topology[cpu].socket_id ? &energy_cluster_a7 :
			&energy_cluster_a15;
}

static inline
const struct sched_group_energy * const cpu_core_energy(int cpu)
{
	return cpu_topology[cpu].socket_id ? &energy_core_a7 :
			&energy_core_a15;
}

static inline int cpu_corepower_flags(void)
{
	return SD_SHARE_PKG_RESOURCES  | SD_SHARE_POWERDOMAIN | \
	       SD_SHARE_CAP_STATES;
}

static struct sched_domain_topology_level arm_topology[] = {
#ifdef CONFIG_SCHED_MC
	{ cpu_coregroup_mask, cpu_corepower_flags, cpu_core_energy, SD_INIT_NAME(MC) },
#endif
	{ cpu_cpu_mask, NULL, cpu_cluster_energy, SD_INIT_NAME(DIE) },
	{ NULL, },
};

#ifdef CONFIG_MTK_CPU_TOPOLOGY
static void __init reset_cpu_topology(void)
{
	unsigned int cpu;

	/* init core mask and capacity */
	for_each_possible_cpu(cpu) {
		struct cputopo_arm *cpu_topo = &(cpu_topology[cpu]);

		cpu_topo->thread_id = -1;
		cpu_topo->core_id =  -1;
		cpu_topo->socket_id = -1;
		cpumask_clear(&cpu_topo->core_sibling);
		cpumask_set_cpu(cpu, &cpu_topo->core_sibling);
		cpumask_clear(&cpu_topo->thread_sibling);
		cpumask_set_cpu(cpu, &cpu_topo->thread_sibling);

		set_capacity_scale(cpu, SCHED_CAPACITY_SCALE);
	}
	smp_wmb();
}

static int cpu_topology_init;
/*
 * init_cpu_topology is called at boot when only one cpu is running
 * which prevent simultaneous write access to cpu_topology array
 */
void __init init_cpu_topology(void)
{
	if (cpu_topology_init)
		return;
	reset_cpu_topology();

	/*
	 * Discard anything that was parsed if we hit an error so we
	 * don't use partial information.
	 */
	if (parse_dt_topology())
		reset_cpu_topology();

	parse_dt_cpu_capacity();

	/* Set scheduler topology descriptor */
	set_sched_topology(arm_topology);
}

void __init arch_build_cpu_topology_domain(void)
{
	init_cpu_topology();
	cpu_topology_init = 1;
}
#else
void __init init_cpu_topology(void)
{
	unsigned int cpu;

	/* init core mask and capacity */
	for_each_possible_cpu(cpu) {
		struct cputopo_arm *cpu_topo = &(cpu_topology[cpu]);

		cpu_topo->thread_id = -1;
		cpu_topo->core_id =  -1;
		cpu_topo->socket_id = -1;
		cpumask_clear(&cpu_topo->core_sibling);
		cpumask_clear(&cpu_topo->thread_sibling);

		set_capacity_scale(cpu, SCHED_CAPACITY_SCALE);
	}
	smp_wmb();

	parse_dt_topology();

	/* Set scheduler topology descriptor */
	set_sched_topology(arm_topology);
}
#endif

/*
 * Extras of CPU & Cluster functions
 */
int arch_cpu_is_big(unsigned int cpu)
{
	struct cputopo_arm *arm_cputopo = &cpu_topology[cpu];

	switch (arm_cputopo->partno) {
	case ARM_CPU_PART_CORTEX_A12:
	case ARM_CPU_PART_CORTEX_A17:
	case ARM_CPU_PART_CORTEX_A15:
		return 1;
	default:
		return 0;
	}
}

int arch_cpu_is_little(unsigned int cpu)
{
	return !arch_cpu_is_big(cpu);
}

int arch_is_smp(void)
{
	static int __arch_smp = -1;

	if (__arch_smp != -1)
		return __arch_smp;

	__arch_smp = (max_cpu_perf != min_cpu_perf) ? 0 : 1;

	return __arch_smp;
}

int arch_get_nr_clusters(void)
{
	static int __arch_nr_clusters = -1;
	int max_id = 0;
	unsigned int cpu;

	if (__arch_nr_clusters != -1)
		return __arch_nr_clusters;

	/* assume socket id is monotonic increasing without gap. */
	for_each_possible_cpu(cpu) {
		struct cputopo_arm *arm_cputopo = &cpu_topology[cpu];

		if (arm_cputopo->socket_id > max_id)
			max_id = arm_cputopo->socket_id;
	}
	__arch_nr_clusters = max_id + 1;
	return __arch_nr_clusters;
}

int arch_is_multi_cluster(void)
{
	return arch_get_nr_clusters() > 1 ? 1 : 0;
}

int arch_get_cluster_id(unsigned int cpu)
{
	struct cputopo_arm *arm_cputopo = &cpu_topology[cpu];

	return arm_cputopo->socket_id < 0 ? 0 : arm_cputopo->socket_id;
}

void arch_get_cluster_cpus(struct cpumask *cpus, int cluster_id)
{
	unsigned int cpu;

	cpumask_clear(cpus);
	for_each_possible_cpu(cpu) {
		struct cputopo_arm *arm_cputopo = &cpu_topology[cpu];

		if (arm_cputopo->socket_id == cluster_id)
			cpumask_set_cpu(cpu, cpus);
	}
}

int arch_better_capacity(unsigned int cpu)
{
	return cpu_capacity(cpu) > min_cpu_perf;
}

#ifdef CONFIG_SCHED_HMP
void __init arch_get_hmp_domains(struct list_head *hmp_domains_list)
{
	struct hmp_domain *domain;
	struct cpumask cpu_mask;
	int id, maxid;

	cpumask_clear(&cpu_mask);
	maxid = arch_get_nr_clusters();

	/*
	 * Initialize hmp_domains
	 * Must be ordered with respect to compute capacity.
	 * Fastest domain at head of list.
	 */
	for (id = 0; id < maxid; id++) {
		arch_get_cluster_cpus(&cpu_mask, id);
		domain = (struct hmp_domain *)
			kmalloc(sizeof(struct hmp_domain), GFP_KERNEL);
		cpumask_copy(&domain->possible_cpus, &cpu_mask);
		cpumask_and(&domain->cpus, cpu_online_mask, &domain->possible_cpus);
		list_add(&domain->hmp_domains, hmp_domains_list);
	}
}
#else
void __init arch_get_hmp_domains(struct list_head *hmp_domains_list) {}
#endif /* CONFIG_SCHED_HMP */

