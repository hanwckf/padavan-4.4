/*
 * Copyright (C) 2016 Richtek Technology Corp.
 *
 * Power Delivery Debug Information
 *
 * Author: Sakya <jeff_chang@richtek.com>
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include "inc/pd_dbg_info.h"

#ifdef CONFIG_PD_DBG_INFO

#define PD_INFO_BUF_SIZE	(2048*256)
#define MSG_POLLING_MS		20

#define OUT_BUF_MAX (128)
static struct {
	int used;
	char buf[PD_INFO_BUF_SIZE + 1 + OUT_BUF_MAX];
} pd_dbg_buffer[2];

static struct mutex buff_lock;
static struct mutex dbg_info_lock;
static unsigned int using_buf;
static atomic_t running = ATOMIC_INIT(1);

static atomic_t busy = ATOMIC_INIT(0);

void pd_dbg_info_lock(void)
{
	atomic_inc(&busy);
}

void pd_dbg_info_unlock(void)
{
	atomic_dec_if_positive(&busy);
}

static int print_out_thread_fn(void *arg)
{
	unsigned int index, i;
	char temp;

	while (atomic_read(&running)) {
		mutex_lock(&buff_lock);
		index = using_buf;
		using_buf ^= 0x01; /* exchange buffer */
		mutex_unlock(&buff_lock);
		if (pd_dbg_buffer[index].used) {
			pd_dbg_buffer[index].
				buf[pd_dbg_buffer[index].used] = '\0';
			pr_info("///PD dbg info %ud\n",
					pd_dbg_buffer[index].used);
			for (i = 0; i < pd_dbg_buffer[index].used;
							i += OUT_BUF_MAX) {
				temp = pd_dbg_buffer[index].
							buf[OUT_BUF_MAX + i];
				pd_dbg_buffer[index].
						buf[OUT_BUF_MAX + i] = '\0';
				/* while (atomic_read(&busy)); */
				printk(pd_dbg_buffer[index].buf + i);
				pd_dbg_buffer[index].
						buf[OUT_BUF_MAX + i] = temp;
				/* msleep(2); */
			}
			pr_info("PD dbg info///\n");
		}
		pd_dbg_buffer[index].used = 0;
		msleep(MSG_POLLING_MS);
	}
	return 0;
}

int pd_dbg_info(const char *fmt, ...)
{
	unsigned int index;
	va_list args;
	int r;
	int used;
	u64 ts;
	unsigned long rem_usec;

	ts = local_clock();
	rem_usec = do_div(ts, 1000000000) / 1000 / 1000;
	va_start(args, fmt);
	mutex_lock(&buff_lock);
	index = using_buf;
	used = pd_dbg_buffer[index].used;
	r = snprintf(pd_dbg_buffer[index].buf + used,
		PD_INFO_BUF_SIZE - used, "<%5lu.%03lu>",
		(unsigned long)ts, rem_usec);
	if (r > 0)
		used += r;
	r = vsnprintf(pd_dbg_buffer[index].buf + used,
			PD_INFO_BUF_SIZE - used, fmt, args);
	if (r > 0)
		used += r;
	pd_dbg_buffer[index].used = used;
	mutex_unlock(&buff_lock);
	va_end(args);
	return r;
}
EXPORT_SYMBOL(pd_dbg_info);


static struct task_struct *print_out_tsk;

int pd_dbg_info_init(void)
{
	pr_info("%s\n", __func__);
	mutex_init(&buff_lock);
	mutex_init(&dbg_info_lock);
	print_out_tsk = kthread_create(
			print_out_thread_fn, NULL, "pd_dbg_info");
	atomic_set(&running, 1);
	wake_up_process(print_out_tsk);
	return 0;
}

void pd_dbg_info_exit(void)
{
	atomic_set(&running, 0);
	kthread_stop(print_out_tsk);
	mutex_destroy(&buff_lock);
}

subsys_initcall(pd_dbg_info_init);
module_exit(pd_dbg_info_exit);

MODULE_DESCRIPTION("PD Debug Info Module");
MODULE_AUTHOR("Patrick Chang <patrick_chang@richtek.com>");
MODULE_LICENSE("GPL");

#endif	/* CONFIG_PD_DBG_INFO */
