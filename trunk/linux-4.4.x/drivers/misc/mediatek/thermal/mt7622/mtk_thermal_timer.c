/*
 * Copyright (C) 2016 MediaTek Inc.
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

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/thermal.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/string.h>
#include "mach/mtk_thermal.h"
#include <mt-plat/aee.h>
/*
*mtk_thermal_timer.c is an interface to collect all thermal timer functions
*It exports two common functions for Suspend, SODI, Deep idle scenarios
*	mtkTTimer_cancel_timer
*	mtkTTimer_start_timer
*
*We don't have to take care those two function lists project by project, because each thermal zone
*will register his functions to mtk_thermal_timer.c when booting up
*/

/* MAX_NUM is the maximum number of pair of thermal zone functions we can hold */
#define MAX_NUM (20)

#define NAME_LEN (20)
#define mtkTTimer_debug_log (0)

#define mtkTTimer_dprintk(fmt, args...)   \
do {	\
	if (mtkTTimer_debug_log)	\
		pr_err("[Thermal timer] " fmt, ##args); \
} while (0)

static DEFINE_SPINLOCK(tTimer_lock);
static bool is_tTimer_init;

struct thermal_timer_func {
	char name[NAME_LEN];
	void (*start_timer)(void);
	void (*cancel_timer)(void);
};

struct thermal_timer_array {
	int count;
	struct thermal_timer_func tFuncs[MAX_NUM];
};

static struct thermal_timer_array tTimerArray;

static void mtkTTimer_init(void)
{
	int i;

	mtkTTimer_dprintk("[%s]\n", __func__);

	tTimerArray.count = 0;

	for (i = 0; i < MAX_NUM; i++) {
		tTimerArray.tFuncs[i].name[0] = '\0';
		tTimerArray.tFuncs[i].start_timer = NULL;
		tTimerArray.tFuncs[i].cancel_timer = NULL;
	}
}

static int mtkTTimer_getIndex(const char *name)
{
	int i, index = -1;

	spin_lock(&tTimer_lock);
	for (i = 0; i < tTimerArray.count; i++) {
		if (!strcmp(name, tTimerArray.tFuncs[i].name)) {
			index = i;
			break;
		}
	}
	spin_unlock(&tTimer_lock);

	return index;
}

int mtkTTimer_register(const char *name, void (*start_timer) (void), void (*cancel_timer) (void))
{
	int index;

	mtkTTimer_dprintk("[%s]\n", __func__);

	if (name) {
		if (strlen(name) >= NAME_LEN) {
			#ifdef CONFIG_MTK_AEE_FEATURE
			aee_kernel_warning_api(__FILE__, __LINE__, DB_OPT_DEFAULT, "mtkTTimer_register",
					"Name is too long");
			#endif
			mtkTTimer_dprintk("[%s] Name is too long\n", __func__);
			return -1;
		}
	} else {
		#ifdef CONFIG_MTK_AEE_FEATURE
		aee_kernel_warning_api(__FILE__, __LINE__, DB_OPT_DEFAULT, "mtkTTimer_register",
				"No name");
		#endif
		mtkTTimer_dprintk("[%s] No name\n", __func__);
		return -1;
	}

	if (tTimerArray.count == MAX_NUM) {
		#ifdef CONFIG_MTK_AEE_FEATURE
		aee_kernel_warning_api(__FILE__, __LINE__, DB_OPT_DEFAULT, "mtkTTimer_register",
					"Array is full");
		#endif
		mtkTTimer_dprintk("[%s] Array is full\n", __func__);
		return -1;
	}

	index = mtkTTimer_getIndex(name);
	if (index != -1) {
		#ifdef CONFIG_MTK_AEE_FEATURE
		aee_kernel_warning_api(__FILE__, __LINE__, DB_OPT_DEFAULT, "mtkTTimer_register",
					"%s registered already", name);
		#endif
		mtkTTimer_dprintk("[%s] %s registered already\n", __func__, name);
		return -1;
	}

	spin_lock(&tTimer_lock);
		if (!is_tTimer_init) {
			is_tTimer_init = true;
			mtkTTimer_init();
		}
	spin_unlock(&tTimer_lock);

	spin_lock(&tTimer_lock);
	index = tTimerArray.count;

	strlcpy(tTimerArray.tFuncs[index].name, name,
		sizeof(tTimerArray.tFuncs[index].name));

	tTimerArray.tFuncs[index].start_timer = start_timer;
	tTimerArray.tFuncs[index].cancel_timer = cancel_timer;

	tTimerArray.count++;
	spin_unlock(&tTimer_lock);
	return 0;
}

int mtkTTimer_unregister(const char *name)
{
	int index;

	if (name) {
		if (strlen(name) >= NAME_LEN) {
			#ifdef CONFIG_MTK_AEE_FEATURE
			aee_kernel_warning_api(__FILE__, __LINE__, DB_OPT_DEFAULT, "mtkTTimer_unregister",
					"Name is too long");
			#endif
			mtkTTimer_dprintk("[%s] Name is too long\n", __func__);
			return -1;
		}
	} else {
		#ifdef CONFIG_MTK_AEE_FEATURE
		aee_kernel_warning_api(__FILE__, __LINE__, DB_OPT_DEFAULT, "mtkTTimer_unregister",
				"No name");
		#endif
		mtkTTimer_dprintk("[%s] No name\n", __func__);
		return -1;
	}

	index = mtkTTimer_getIndex(name);
	if (index == -1)
		return 0;

	spin_lock(&tTimer_lock);
	for (; index < (tTimerArray.count - 1); index++) {
		strlcpy(tTimerArray.tFuncs[index].name, tTimerArray.tFuncs[index + 1].name,
			sizeof(tTimerArray.tFuncs[index].name));
		tTimerArray.tFuncs[index].start_timer = tTimerArray.tFuncs[index + 1].start_timer;
		tTimerArray.tFuncs[index].cancel_timer = tTimerArray.tFuncs[index + 1].cancel_timer;
	}

	tTimerArray.tFuncs[index].name[0] = '\0';
	tTimerArray.tFuncs[index].start_timer = NULL;
	tTimerArray.tFuncs[index].cancel_timer = NULL;

	tTimerArray.count--;
	spin_unlock(&tTimer_lock);
	return 0;
}

void mtkTTimer_cancel_timer(void)
{
	int i;

	spin_lock(&tTimer_lock);
	for (i = 0; i < tTimerArray.count; i++) {
		mtkTTimer_dprintk("[%s] %s\n", __func__, tTimerArray.tFuncs[i].name);
		tTimerArray.tFuncs[i].cancel_timer();
	}
	spin_unlock(&tTimer_lock);
}

void mtkTTimer_start_timer(void)
{
	int i;

	spin_lock(&tTimer_lock);
	for (i = 0; i < tTimerArray.count; i++) {
		mtkTTimer_dprintk("[%s] %s\n", __func__, tTimerArray.tFuncs[i].name);
		tTimerArray.tFuncs[i].start_timer();
	}
	spin_unlock(&tTimer_lock);
}
