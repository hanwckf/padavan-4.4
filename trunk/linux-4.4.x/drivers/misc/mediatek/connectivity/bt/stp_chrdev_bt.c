/*
* Copyright (C) 2016 MediaTek Inc.
*
* This program is free software: you can redistribute it and/or modify it under the terms of the
* GNU General Public License version 2 as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along with this program.
* If not, see <http://www.gnu.org/licenses/>.
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <asm/current.h>
#include <linux/uaccess.h>
#include <linux/fcntl.h>
#include <linux/poll.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/printk.h>

#include "wmt_exp.h"
#include "stp_exp.h"

MODULE_LICENSE("Dual BSD/GPL");

#define BT_DRIVER_NAME "mtk_stp_bt_chrdev"
#define BT_DEV_MAJOR 192

#define PFX                         "[MTK-BT] "
#define BT_LOG_DBG                  3
#define BT_LOG_INFO                 2
#define BT_LOG_WARN                 1
#define BT_LOG_ERR                  0

static UINT32 gDbgLevel = BT_LOG_INFO;

#define BT_DBG_FUNC(fmt, arg...)	\
	do { if (gDbgLevel >= BT_LOG_DBG) pr_debug(PFX "%s: " fmt, __func__, ##arg); } while (0)
#define BT_INFO_FUNC(fmt, arg...)	\
	do { if (gDbgLevel >= BT_LOG_INFO) pr_debug(PFX "%s: " fmt, __func__, ##arg); } while (0)
#define BT_WARN_FUNC(fmt, arg...)	\
	do { if (gDbgLevel >= BT_LOG_WARN) pr_warn(PFX "%s: " fmt, __func__, ##arg); } while (0)
#define BT_ERR_FUNC(fmt, arg...)	\
	do { if (gDbgLevel >= BT_LOG_ERR) pr_err(PFX "%s: " fmt, __func__, ##arg); } while (0)

#define VERSION "2.0"

#define COMBO_IOC_MAGIC             0xb0
#define COMBO_IOCTL_FW_ASSERT       _IOWR(COMBO_IOC_MAGIC, 0, int)
#define COMBO_IOCTL_BT_SET_PSM      _IOWR(COMBO_IOC_MAGIC, 1, bool)
#define COMBO_IOCTL_BT_IC_HW_VER    _IOR(COMBO_IOC_MAGIC, 2, void*)
#define COMBO_IOCTL_BT_IC_FW_VER    _IOR(COMBO_IOC_MAGIC, 3, void*)

static INT32 BT_devs = 1;
static INT32 BT_major = BT_DEV_MAJOR;
module_param(BT_major, uint, 0);
static struct cdev BT_cdev;
#if CREATE_NODE_DYNAMIC
static struct class *stpbt_class;
static struct device *stpbt_dev;
#endif

#define BT_BUFFER_SIZE              2048
static UINT8 i_buf[BT_BUFFER_SIZE]; /* Input buffer for read */
static UINT8 o_buf[BT_BUFFER_SIZE]; /* Output buffer for write */

static struct semaphore wr_mtx, rd_mtx;
/* Wait queue for poll and read */
static wait_queue_head_t inq;
static DECLARE_WAIT_QUEUE_HEAD(BT_wq);
static INT32 flag;
/* Reset flag for whole chip reset senario */
static volatile INT32 rstflag;

/*******************************************************************
 * WHOLE CHIP RESET message handler
 *******************************************************************
*/
static VOID bt_cdev_rst_cb(ENUM_WMTDRV_TYPE_T src,
			   ENUM_WMTDRV_TYPE_T dst, ENUM_WMTMSG_TYPE_T type, PVOID buf, UINT32 sz)
{
	ENUM_WMTRSTMSG_TYPE_T rst_msg;

	if (sz <= sizeof(ENUM_WMTRSTMSG_TYPE_T)) {
		memcpy((PINT8)&rst_msg, (PINT8)buf, sz);
		BT_DBG_FUNC("src = %d, dst = %d, type = %d, buf = 0x%x sz = %d, max = %d\n", src,
			     dst, type, rst_msg, sz, WMTRSTMSG_RESET_MAX);
		if ((src == WMTDRV_TYPE_WMT) && (dst == WMTDRV_TYPE_BT)
		    && (type == WMTMSG_TYPE_RESET)) {
			if (rst_msg == WMTRSTMSG_RESET_START) {
				BT_INFO_FUNC("BT reset start!\n");
				rstflag = 1;
				wake_up_interruptible(&inq);

			} else if (rst_msg == WMTRSTMSG_RESET_END) {
				BT_INFO_FUNC("BT reset end!\n");
				rstflag = 2;
				wake_up_interruptible(&inq);
			}
		}
	} else
		BT_WARN_FUNC("Invalid message format!\n");
}

VOID BT_event_cb(VOID)
{
	BT_DBG_FUNC("BT_event_cb\n");

	flag = 1;

	/*
	* Finally, wake up any reader blocked in poll or read
	*/
	wake_up_interruptible(&inq);
	wake_up(&BT_wq);
}

unsigned int BT_poll(struct file *filp, poll_table *wait)
{
	UINT32 mask = 0;

/* down(&wr_mtx); */
	/*
	 * The buffer is circular; it is considered full
	 * if "wp" is right behind "rp". "left" is 0 if the
	 * buffer is empty, and it is "1" if it is completely full.
	 */
	if (mtk_wcn_stp_is_rxqueue_empty(BT_TASK_INDX)) {
		poll_wait(filp, &inq, wait);

		if (!mtk_wcn_stp_is_rxqueue_empty(BT_TASK_INDX) || rstflag)
			/* BT Rx queue has valid data, or whole chip reset occurs */
			mask |= POLLIN | POLLRDNORM;	/* Readable */
	} else {
		mask |= POLLIN | POLLRDNORM;	/* Readable */
	}

	/* Do we need condition here? */
	mask |= POLLOUT | POLLWRNORM;	/* Writable */
/* up(&wr_mtx); */
	return mask;
}

ssize_t BT_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	INT32 retval = 0;

	down(&wr_mtx);

	BT_DBG_FUNC("count %zd pos %lld\n", count, *f_pos);

	if (rstflag) {
		if (rstflag == 1) {	/* Reset start */
			retval = -88;
			BT_INFO_FUNC("detect whole chip reset start\n");
		} else if (rstflag == 2) {	/* Reset end */
			retval = -99;
			BT_INFO_FUNC("detect whole chip reset end\n");
		}
		goto OUT;
	}

	if (count > 0) {
		if (count > BT_BUFFER_SIZE) {
			count = BT_BUFFER_SIZE;
			BT_WARN_FUNC("Shorten count %zd to BT_BUFFER_SIZE\n", count);
		}

		if (copy_from_user(o_buf, buf, count)) {
			retval = -EFAULT;
			goto OUT;
		}

		retval = mtk_wcn_stp_send_data(o_buf, count, BT_TASK_INDX);
		if (retval < 0)
			BT_ERR_FUNC("mtk_wcn_stp_send_data fail, retval %d\n", retval);
		else if (retval == 0) {
			/* Device cannot process data in time, BT queue is full and no space is available for write,
			 * native program should not call BT_write with no delay.
			 */
			BT_ERR_FUNC("Packet length %zd, sent bytes %d, no space is available!\n", count, retval);
			retval = -EAGAIN;
		} else
			BT_DBG_FUNC("Packet length %zd, sent bytes %d\n", count, retval);

	} else {
		BT_ERR_FUNC("Packet length %zd is not allowed\n", count);
		retval = -EINVAL;
	}

OUT:
	up(&wr_mtx);
	return retval;
}

ssize_t BT_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	static int chip_reset_count;
	INT32 retval = 0;

	down(&rd_mtx);

	BT_DBG_FUNC("count %zd pos %lld\n", count, *f_pos);

	if (rstflag) {
		if (rstflag == 1) {	/* Reset start */
			retval = -88;
			if ((chip_reset_count%500) == 0)
				BT_INFO_FUNC("detect whole chip reset start, %d\n", chip_reset_count);
			chip_reset_count++;
		} else if (rstflag == 2) {	/* Reset end */
			retval = -99;
			BT_INFO_FUNC("detect whole chip reset end\n");
			chip_reset_count = 0;
		}
		goto OUT;
	}

	if (count > BT_BUFFER_SIZE) {
		count = BT_BUFFER_SIZE;
		BT_WARN_FUNC("Shorten count %zd to BT_BUFFER_SIZE\n", count);
	}

	retval = mtk_wcn_stp_receive_data(i_buf, count, BT_TASK_INDX);
	if (retval < 0) {
		BT_ERR_FUNC("mtk_wcn_stp_receive_data fail, retval %d\n", retval);
		goto OUT;
	}

	while (retval == 0) {	/* Got nothing, wait for STP's signal */
		/*
		* If nonblocking mode, return directly.
		* O_NONBLOCK is specified during open()
		*/
		if (filp->f_flags & O_NONBLOCK) {
			BT_DBG_FUNC("Non-blocking read, return directly\n");
			retval = -EAGAIN;
			goto OUT;
		}

		wait_event(BT_wq, flag != 0);
		flag = 0;

		retval = mtk_wcn_stp_receive_data(i_buf, count, BT_TASK_INDX);
		if (retval < 0) {
			BT_ERR_FUNC("mtk_wcn_stp_receive_data fail, retval %d\n", retval);
			goto OUT;
		}
	}

	/* Got something from STP driver */
	BT_DBG_FUNC("Read bytes %d\n", retval);
	if (copy_to_user(buf, i_buf, retval)) {
		retval = -EFAULT;
		goto OUT;
	}

OUT:
	up(&rd_mtx);
	return retval;
}

/* int BT_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg) */
long BT_unlocked_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	INT32 retval = 0;
	MTK_WCN_BOOL bRet = MTK_WCN_BOOL_TRUE;
	UINT32 ver = 0;

	BT_DBG_FUNC("cmd: 0x%08x\n", cmd);

	switch (cmd) {
	case COMBO_IOCTL_FW_ASSERT:
		/* Trigger FW assert for debug */
		BT_INFO_FUNC("Host trigger FW assert......, reason: %lu\n", arg);
		bRet = mtk_wcn_wmt_assert(WMTDRV_TYPE_BT, (UINT32)arg);
		if (bRet == MTK_WCN_BOOL_TRUE)
			BT_INFO_FUNC("Host trigger FW assert succeed\n");
		else {
			BT_ERR_FUNC("Host trigger FW assert failed\n");
			retval = -EIO;
		}
		break;
	case COMBO_IOCTL_BT_SET_PSM:
		/* BT stack may need to dynamically enable/disable Power Saving Mode
		 * in some scenarios for performance, e.g. A2DP chopping.
		 */
		BT_INFO_FUNC("BT stack change PSM setting: %lu\n", arg);
		retval = mtk_wcn_wmt_psm_ctrl((MTK_WCN_BOOL)arg);
		break;
	case COMBO_IOCTL_BT_IC_HW_VER:
		ver = mtk_wcn_wmt_ic_info_get(WMTCHIN_HWVER);
		BT_INFO_FUNC("HW ver: 0x%x\n", ver);
		if (copy_to_user((UINT32 __user *)arg, &ver, sizeof(ver)))
			retval = -EFAULT;
		break;
	case COMBO_IOCTL_BT_IC_FW_VER:
		ver = mtk_wcn_wmt_ic_info_get(WMTCHIN_FWVER);
		BT_INFO_FUNC("FW ver: 0x%x\n", ver);
		if (copy_to_user((UINT32 __user *)arg, &ver, sizeof(ver)))
			retval = -EFAULT;
		break;
	default:
		BT_ERR_FUNC("Unknown cmd: 0x%08x\n", cmd);
		retval = -EINVAL;
		break;
	}

	return retval;
}

long BT_compat_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	return BT_unlocked_ioctl(filp, cmd, arg);
}

static int BT_open(struct inode *inode, struct file *file)
{
	BT_INFO_FUNC("major %d minor %d (pid %d)\n", imajor(inode), iminor(inode), current->pid);

	/* Turn on BT */
	if (mtk_wcn_wmt_func_on(WMTDRV_TYPE_BT) == MTK_WCN_BOOL_FALSE) {
		BT_WARN_FUNC("WMT turn on BT fail!\n");
		return -EIO;
	}

	BT_INFO_FUNC("WMT turn on BT OK!\n");
	rstflag = 0;

	if (mtk_wcn_stp_is_ready()) {

		mtk_wcn_stp_set_bluez(0);

		BT_INFO_FUNC("Now it's in MTK Bluetooth Mode\n");
		BT_INFO_FUNC("STP is ready!\n");

		BT_DBG_FUNC("Register BT event callback!\n");
		mtk_wcn_stp_register_event_cb(BT_TASK_INDX, BT_event_cb);
	} else {
		BT_ERR_FUNC("STP is not ready!\n");
		mtk_wcn_wmt_func_off(WMTDRV_TYPE_BT);
		return -EIO;
	}

	BT_DBG_FUNC("Register BT reset callback!\n");
	mtk_wcn_wmt_msgcb_reg(WMTDRV_TYPE_BT, bt_cdev_rst_cb);

	sema_init(&wr_mtx, 1);
	sema_init(&rd_mtx, 1);

	return 0;
}

static int BT_close(struct inode *inode, struct file *file)
{
	BT_INFO_FUNC("major %d minor %d (pid %d)\n", imajor(inode), iminor(inode), current->pid);
	rstflag = 0;
	mtk_wcn_wmt_msgcb_unreg(WMTDRV_TYPE_BT);
	mtk_wcn_stp_register_event_cb(BT_TASK_INDX, NULL);

	if (mtk_wcn_wmt_func_off(WMTDRV_TYPE_BT) == MTK_WCN_BOOL_FALSE) {
		BT_ERR_FUNC("WMT turn off BT fail!\n");
		return -EIO;	/* Mostly, native program will not check this return value. */
	}

	BT_INFO_FUNC("WMT turn off BT OK!\n");
	return 0;
}

const struct file_operations BT_fops = {
	.open = BT_open,
	.release = BT_close,
	.read = BT_read,
	.write = BT_write,
	/* .ioctl = BT_ioctl, */
	.unlocked_ioctl = BT_unlocked_ioctl,
	.compat_ioctl = BT_compat_ioctl,
	.poll = BT_poll
};

static int BT_init(void)
{
	dev_t dev = MKDEV(BT_major, 0);
	INT32 alloc_ret = 0;
	INT32 cdev_err = 0;

	/* Allocate char device */
	alloc_ret = register_chrdev_region(dev, BT_devs, BT_DRIVER_NAME);
	if (alloc_ret) {
		BT_ERR_FUNC("Failed to register device numbers\n");
		return alloc_ret;
	}

	cdev_init(&BT_cdev, &BT_fops);
	BT_cdev.owner = THIS_MODULE;

	cdev_err = cdev_add(&BT_cdev, dev, BT_devs);
	if (cdev_err)
		goto error;

#if CREATE_NODE_DYNAMIC /* mknod replace */
	stpbt_class = class_create(THIS_MODULE, "stpbt");
	if (IS_ERR(stpbt_class))
		goto error;
	stpbt_dev = device_create(stpbt_class, NULL, dev, NULL, "stpbt");
	if (IS_ERR(stpbt_dev))
		goto error;
#endif

	BT_INFO_FUNC("%s driver(major %d) installed\n", BT_DRIVER_NAME, BT_major);

	/* Initialize wait queue */
	init_waitqueue_head(&(inq));

	return 0;

error:
#if CREATE_NODE_DYNAMIC
	if (stpbt_dev && !IS_ERR(stpbt_dev)) {
		device_destroy(stpbt_class, dev);
		stpbt_dev = NULL;
	}
	if (stpbt_class && !IS_ERR(stpbt_class)) {
		class_destroy(stpbt_class);
		stpbt_class = NULL;
	}
#endif
	if (cdev_err == 0)
		cdev_del(&BT_cdev);

	if (alloc_ret == 0)
		unregister_chrdev_region(dev, BT_devs);

	return -1;
}

static void BT_exit(void)
{
	dev_t dev = MKDEV(BT_major, 0);

#if CREATE_NODE_DYNAMIC
	if (stpbt_dev && !IS_ERR(stpbt_dev)) {
		device_destroy(stpbt_class, dev);
		stpbt_dev = NULL;
	}
	if (stpbt_class && !IS_ERR(stpbt_class)) {
		class_destroy(stpbt_class);
		stpbt_class = NULL;
	}
#endif

	cdev_del(&BT_cdev);
	unregister_chrdev_region(dev, BT_devs);

	BT_INFO_FUNC("%s driver removed\n", BT_DRIVER_NAME);
}

#ifdef MTK_WCN_REMOVE_KERNEL_MODULE

int mtk_wcn_stpbt_drv_init(void)
{
	return BT_init();
}
EXPORT_SYMBOL(mtk_wcn_stpbt_drv_init);

void mtk_wcn_stpbt_drv_exit(void)
{
	return BT_exit();
}
EXPORT_SYMBOL(mtk_wcn_stpbt_drv_exit);

#else

module_init(BT_init);
module_exit(BT_exit);

#endif
