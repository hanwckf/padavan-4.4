/* Copyright (c) 2011-2013, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/ioctl.h>
#include <linux/signal.h>
#include <linux/kthread.h>
#include "mt8521p-afe.h"

#define AUDRTC_CMD_ACC_TRIGGER         _IOW(0, 0, int)
#define AUDRTC_CMD_SET_TARGET_ACC      _IOW(0, 1, int)
#define AUDRTC_CMD_GET_PACKET          _IOR(0, 2, int)

static DEFINE_MUTEX(packet_lock);
struct audrtc_packet {
	u32 acc;
	u32 tsf;
};

struct audrtc_private {
	u32 acc_int;
	struct audrtc_packet packet;
	struct fasync_struct *fasync_queue;
};

#define AUDRTC_MSG_QUIT    (0)
#define AUDRTC_MSG_NOTIFY  (1)

static int audrtc_thread(void *data)
{
	struct audrtc_private *priv = data;

	for (;;) {
		int msg = AUDRTC_MSG_QUIT;

		if (msg == AUDRTC_MSG_QUIT) {
			break;
		} else if (msg == AUDRTC_MSG_NOTIFY) {
			mutex_lock(&packet_lock);
			priv->packet.acc = priv->acc_int;
			priv->packet.tsf = 0;
			mutex_unlock(&packet_lock);
			if (priv->fasync_queue)
				kill_fasync(&priv->fasync_queue, SIGIO,
					    POLL_IN);
		}
	}
	return 0;
}

static int audrtc_fasync(int fd, struct file *filp, int on)
{
	struct audrtc_private *priv = filp->private_data;

	if (!priv)
		return -EFAULT;
	pr_debug("%s()\n", __func__);
	return fasync_helper(fd, filp, on, &priv->fasync_queue);
}

static int audrtc_open(struct inode *node, struct file *filp)
{
	int pid;
	struct audrtc_private *priv;

	pr_debug("%s()\n", __func__);
	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	if (!priv) {
		pr_err("%s() error: kzalloc audrtc_private failed\n", __func__);
		goto alloc_priv_error;
	}
	pid = kernel_thread(audrtc_thread, priv, CLONE_FS | CLONE_FILES |
			    CLONE_SIGHAND | SIGCHLD);
	if (pid < 0) {
		pr_err("%s() error: kernel_thread return %d\n", __func__, pid);
		goto create_thread_error;
	}
	filp->private_data = priv;
	return 0;

create_thread_error:
	kzfree(priv);
alloc_priv_error:
	return -EPERM;
}

static int audrtc_release(struct inode *node, struct file *filp)
{
	struct audrtc_private *priv = filp->private_data;

	pr_debug("%s()\n", __func__);
	if (!priv)
		return -ENOMEM;
	audrtc_fasync(-1, filp, 0);
	kzfree(priv);
	filp->private_data = NULL;
	return 0;
}

static long audrtc_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct audrtc_private *priv = filp->private_data;

	pr_debug("%s() cmd=0x%08x, arg=0x%lx\n", __func__, cmd, arg);
	if (!priv)
		return -ENOMEM;
	switch (cmd) {
	case AUDRTC_CMD_ACC_TRIGGER:
		break;
	case AUDRTC_CMD_GET_PACKET:
		if (!arg)
			return -EFAULT;
		mutex_lock(&packet_lock);
		if (copy_to_user((void __user *)(arg), (void *)(&priv->packet),
				 sizeof(struct audrtc_packet)) != 0) {
			mutex_unlock(&packet_lock);
			return -EFAULT;
		}
		mutex_unlock(&packet_lock);
		break;
	default:
		pr_err("%s() error: unknown audrtc cmd 0x%08x\n",
		       __func__, cmd);
		return -EINVAL;
	}
	return 0;
}

static const struct file_operations audrtc_fops = {
	.owner = THIS_MODULE,
	.open = audrtc_open,
	.release = audrtc_release,
	.unlocked_ioctl = audrtc_ioctl,
	.write = NULL,
	.read = NULL,
	.flush = NULL,
	.fasync = audrtc_fasync,
	.mmap = NULL
};

static struct miscdevice audrtc_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "audrtc",
	.fops = &audrtc_fops,
};

static int audrtc_mod_init(void)
{
	int ret;

	pr_debug("%s()\n", __func__);
	ret = misc_register(&audrtc_device);
	if (ret)
		pr_err("%s() error: misc_register audrtc failed %d\n",
		       __func__, ret);
	return 0;
}

static void audrtc_mod_exit(void)
{
	pr_debug("%s()\n", __func__);
	misc_deregister(&audrtc_device);
}
module_init(audrtc_mod_init);
module_exit(audrtc_mod_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("audrtc driver");
