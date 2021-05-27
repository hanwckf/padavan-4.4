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

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/thermal.h>

#include "thermal_core.h"

/**
 * legacy_throttle
 * @tz - thermal_zone_device
 *
 * This function update the cooler state by monitoring the current temperature and trip points
 */
static int legacy_throttle(struct thermal_zone_device *tz, int trip)
{
	int trip_temp;
	struct thermal_instance *instance;

	if (trip == THERMAL_TRIPS_NONE)
		trip_temp = tz->forced_passive;
	else
		tz->ops->get_trip_temp(tz, trip, &trip_temp);

	/* mutex_lock(&tz->lock); */

	list_for_each_entry(instance, &tz->thermal_instances, tz_node) {
		if (instance->trip != trip)
			continue;

		if (tz->temperature >= trip_temp)
			instance->target = 1;
		else
			instance->target = 0;
		instance->cdev->updated = false;
		thermal_cdev_update(instance->cdev);
	}

	/* mutex_unlock(&tz->lock); */

	return 0;
}

static struct thermal_governor thermal_gov_legacy = {
	.name = "legacy",
	.throttle = legacy_throttle,
	/* .owner                = THIS_MODULE, */
};

static int __init thermal_gov_legacy_init(void)
{
	return thermal_register_governor(&thermal_gov_legacy);
}

static void __exit thermal_gov_legacy_exit(void)
{
	thermal_unregister_governor(&thermal_gov_legacy);
}

/* This should load after thermal framework */
fs_initcall(thermal_gov_legacy_init);
module_exit(thermal_gov_legacy_exit);

MODULE_AUTHOR("Weiyi Lu");
MODULE_DESCRIPTION("A legacy compatible Thermal governor");
MODULE_LICENSE("GPL");
