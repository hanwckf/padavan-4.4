/*
 *  Copyright (c) 2017 MediaTek Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/skbuff.h>
#include <linux/completion.h>
#include <linux/version.h>
#include <linux/firmware.h>
#include <linux/skbuff.h>
#include <linux/cdev.h>
#include <linux/spinlock.h>
#include <linux/kallsyms.h>
#include <linux/fs.h>
#include <linux/kthread.h>
#include <linux/uaccess.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of.h>
#include <linux/timer.h>
#include <net/bluetooth/bluetooth.h>
#include <net/bluetooth/hci_core.h>

#include "btmtk_btif_main.h"

#include <linux/platform_device.h>
#include "mtk_btif_exp.h"

/*============================================================================*/
/* Local Configuration */
/*============================================================================*/
#define VERSION "6.0.17080301"

/*============================================================================*/
/* Function Prototype */
/*============================================================================*/
#define ___________________________________________________Function_Prototype
static int btmtk_btif_BT_init(void);
static void btmtk_btif_BT_exit(void);
static void btmtk_bitf_fwlog_queue(const unsigned char *p_buf, unsigned int len);
static void btmtk_bitf_rx_cb(const unsigned char *p_buf, unsigned int len);
static void btmtk_btif_hci_snoop_print_to_log(void);
static int btmtk_btif_send_wmt_cmd(const u8 *cmd, const int cmd_len, const u8 *event,
									const int event_len, u32 delay);
static int btmtk_btif_send_hci_cmd(const u8 *cmd, int cmd_len, const u8 *event, int event_len);
/** file_operations: stpbt */
static int btmtk_btif_fops_open(struct inode *inode, struct file *file);
static int btmtk_btif_fops_close(struct inode *inode, struct file *file);
static ssize_t btmtk_btif_fops_read(struct file *file, char __user *buf, size_t count, loff_t *f_pos);
static ssize_t btmtk_btif_fops_write(struct file *file, const char __user *buf, size_t count, loff_t *f_pos);
static unsigned int btmtk_btif_fops_poll(struct file *file, poll_table *wait);
static long btmtk_btif_fops_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
static int btmtk_btif_fops_fasync(int fd, struct file *file, int on);
static int btmtk_btif_fops_openfwlog(struct inode *inode, struct file *file);
static int btmtk_btif_fops_closefwlog(struct inode *inode, struct file *file);
static ssize_t btmtk_btif_fops_readfwlog(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
static ssize_t btmtk_btif_fops_writefwlog(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos);
static unsigned int btmtk_btif_fops_pollfwlog(struct file *filp, poll_table *wait);
static long btmtk_btif_fops_unlocked_ioctlfwlog(struct file *filp, unsigned int cmd, unsigned long arg);
static int btmtk_fops_raw_open(struct inode *inode, struct file *file);
static int btmtk_fops_raw_close(struct inode *inode, struct file *file);

static void dump_hex(unsigned char *buf, int dump_len);
static bool btmtk_get_irq(void);
static int btmtk_irq_reg(void);
static irqreturn_t btmtk_irq_handler(int irq, void *data);
static int btmtk_sleep_thread(void *data);
static void btmtk_sleep_handler(unsigned long data);
static int btmtk_wakeup_thread(void *data);
static int btmtk_wakeup_consys(void);
static int btmtk_read_buffer(struct ring_buffer_struct *metabuffer, unsigned char *p_buf, int count);
static void btmtk_write_buffer(struct ring_buffer_struct *metabuffer, u8 *buf, u32 length);


/*============================================================================*/
/* Global Variable */
/*============================================================================*/
#define _____________________________________________________Global_Variables
u8 btmtk_log_lvl = BTMTK_LOG_LEVEL_DEFAULT;
/* stpbt character device for meta */
#ifdef FIXED_STPBT_MAJOR_DEV_ID
static int BT_major = FIXED_STPBT_MAJOR_DEV_ID;
static int BT_majorfwlog = FIXED_STPBT_MAJOR_DEV_ID+1;
#else
static int BT_major;
static int BT_majorfwlog;
#endif
static struct class *pBTClass;

static struct device *pBTDev;
static struct device *pBTDevfwlog;
static struct device *pBTDev_raw;

static struct cdev BT_cdev;
static struct cdev BT_cdevfwlog;
static struct cdev BT_cdev_raw;

static dev_t g_devID;
static dev_t g_devIDfwlog;
static dev_t g_devID_raw;

static wait_queue_head_t inq;
static wait_queue_head_t fw_log_inq;
static struct btmtk_btif_data *g_data;
static int probe_counter;
static int need_reset_stack;
static int btmtk_btif_state = BTMTK_BTIF_STATE_UNKNOWN;
static int btmtk_fops_state = BTMTK_FOPS_STATE_UNKNOWN;
static DECLARE_WAIT_QUEUE_HEAD(BT_wq);
static DEFINE_MUTEX(btmtk_btif_state_mutex);
static DEFINE_MUTEX(btmtk_fops_state_mutex);
#define FOPS_MUTEX_LOCK()	mutex_lock(&btmtk_fops_state_mutex)
#define FOPS_MUTEX_UNLOCK()	mutex_unlock(&btmtk_fops_state_mutex)
static struct fasync_struct *fasync;

static DECLARE_WAIT_QUEUE_HEAD(p_wait_event_q);
static char *p_event_buffer;

typedef void (*register_early_suspend) (void (*f) (void));
typedef void (*register_late_resume) (void (*f) (void));
register_early_suspend register_early_suspend_func;
register_late_resume register_late_resume_func;

/* Hci Snoop */
static u8 hci_cmd_snoop_buf[HCI_SNOOP_ENTRY_NUM][HCI_SNOOP_BUF_SIZE];
static u8 hci_cmd_snoop_len[HCI_SNOOP_ENTRY_NUM] = { 0 };

static unsigned int hci_cmd_snoop_timestamp[HCI_SNOOP_ENTRY_NUM];
static u8 hci_event_snoop_buf[HCI_SNOOP_ENTRY_NUM][HCI_SNOOP_BUF_SIZE];
static u8 hci_event_snoop_len[HCI_SNOOP_ENTRY_NUM] = { 0 };

static unsigned int hci_event_snoop_timestamp[HCI_SNOOP_ENTRY_NUM];
static u8 hci_acl_snoop_buf[HCI_SNOOP_ENTRY_NUM][HCI_SNOOP_BUF_SIZE];
static u8 hci_acl_snoop_len[HCI_SNOOP_ENTRY_NUM] = { 0 };

static unsigned int hci_acl_snoop_timestamp[HCI_SNOOP_ENTRY_NUM];
static int hci_cmd_snoop_index = HCI_SNOOP_ENTRY_NUM - 1;
static int hci_event_snoop_index = HCI_SNOOP_ENTRY_NUM - 1;
static int hci_acl_snoop_index = HCI_SNOOP_ENTRY_NUM - 1;

static struct timer_list sleep_timer;
static bool sleep_timer_enable;
static bool enter_sleep;
static bool try_wake;
static bool fw_dump_enable;
struct task_struct *sleep_tsk;
struct task_struct *wakeup_tsk;
#define SLEEP_TIME (15 * HZ)
static DECLARE_WAIT_QUEUE_HEAD(p_sleep_q);
static bool raw_mode;

const struct file_operations BT_fops = {
	.open = btmtk_btif_fops_open,
	.release = btmtk_btif_fops_close,
	.read = btmtk_btif_fops_read,
	.write = btmtk_btif_fops_write,
	.poll = btmtk_btif_fops_poll,
	.unlocked_ioctl = btmtk_btif_fops_unlocked_ioctl,
	.fasync = btmtk_btif_fops_fasync,
};

const struct file_operations BT_fopsfwlog = {
	.open = btmtk_btif_fops_openfwlog,
	.release = btmtk_btif_fops_closefwlog,
	.read = btmtk_btif_fops_readfwlog,
	.write = btmtk_btif_fops_writefwlog,
	.poll = btmtk_btif_fops_pollfwlog,
	.unlocked_ioctl = btmtk_btif_fops_unlocked_ioctlfwlog

};

const struct file_operations BT_fops_raw = {
	.open = btmtk_fops_raw_open,
	.release = btmtk_fops_raw_close,
	.read = btmtk_btif_fops_read,
	.write = btmtk_btif_fops_write,
	.poll = btmtk_btif_fops_poll,
	.unlocked_ioctl = btmtk_btif_fops_unlocked_ioctl,
	.fasync = btmtk_btif_fops_fasync,
};

static int btmtk_btif_suspend(struct platform_device *pdev, pm_message_t state);
static int btmtk_btif_resume(struct platform_device *pdev);

static struct platform_device mtk_device_wmt = {
	.name			= "btmtk_btif",
	.id				= -1,
};

static struct platform_driver mtk_wmt_dev_drv = {
	.suspend = btmtk_btif_suspend,
	.resume  = btmtk_btif_resume,
	.driver  = {
		.name    = "btmtk_btif",
		.owner   = THIS_MODULE,
	},
};

/*============================================================================*/
/* Internal Functions */
/*============================================================================*/
#define ___________________________________________________Internal_Functions
static int btmtk_btif_init_memory(void)
{
	if (g_data == NULL) {
		BTIF_ERR("%s: g_data is NULL !", __func__);
		return -1;
	}

	g_data->chip_id = 0;
	g_data->suspend_count = 0;

	if (g_data->metabuffer)
		memset(g_data->metabuffer->buffer, 0, META_BUFFER_SIZE);

	if (g_data->fwlog_metabuffer)
		memset(g_data->fwlog_metabuffer->buffer, 0, META_BUFFER_SIZE);

	if (g_data->io_buf)
		memset(g_data->io_buf, 0, BTIF_IO_BUF_SIZE);

	if (g_data->rom_patch_bin_file_name)
		memset(g_data->rom_patch_bin_file_name, 0, MAX_BIN_FILE_NAME_LEN);

	if (g_data->i_buf)
		memset(g_data->i_buf, 0, BUFFER_SIZE);

	if (g_data->o_buf)
		memset(g_data->o_buf, 0, BUFFER_SIZE);

	if (g_data->i_fwlog_buf)
		memset(g_data->i_fwlog_buf, 0, BUFFER_SIZE);

	if (g_data->o_fwlog_buf)
		memset(g_data->o_fwlog_buf, 0, BUFFER_SIZE);

	BTIF_INFO("%s: Success", __func__);
	return 1;
}

static int btmtk_btif_allocate_memory(void)
{
	if (g_data == NULL) {
		g_data = kzalloc(sizeof(*g_data), GFP_KERNEL);
		if (!g_data) {
			BTIF_ERR("%s: alloc memory fail (g_data)", __func__);
			return -1;
		}
	}

	if (g_data->metabuffer == NULL) {
		g_data->metabuffer = kzalloc(sizeof(struct ring_buffer_struct), GFP_KERNEL);
		if (!g_data->metabuffer) {
			BTIF_ERR("%s: alloc memory fail (g_data->metabuffer)",
				__func__);
			return -1;
		}
	}

	if (g_data->fwlog_metabuffer == NULL) {
		g_data->fwlog_metabuffer = kzalloc(sizeof(struct ring_buffer_struct), GFP_KERNEL);
		if (!g_data->fwlog_metabuffer) {
			BTIF_ERR("%s: alloc memory fail (g_data->fwlog_metabuffer)",
				__func__);
			return -1;
		}
	}

	if (g_data->io_buf == NULL) {
		g_data->io_buf = kzalloc(BTIF_IO_BUF_SIZE, GFP_KERNEL);
		if (!g_data->io_buf) {
			BTIF_ERR("%s: alloc memory fail (g_data->io_buf)",
				__func__);
			return -1;
		}
	}

	if (g_data->rom_patch_bin_file_name == NULL) {
		g_data->rom_patch_bin_file_name = kzalloc(MAX_BIN_FILE_NAME_LEN, GFP_KERNEL);
		if (!g_data->rom_patch_bin_file_name) {
			BTIF_ERR
				("%s: alloc memory fail (g_data->rom_patch_bin_file_name)",
				 __func__);
			return -1;
		}
	}

	if (g_data->i_buf == NULL) {
		g_data->i_buf = kzalloc(BUFFER_SIZE, GFP_KERNEL);
		if (!g_data->i_buf) {
			BTIF_ERR("%s: alloc memory fail (g_data->i_buf)", __func__);
			return -1;
		}
	}

	if (g_data->o_buf == NULL) {
		g_data->o_buf = kzalloc(BUFFER_SIZE, GFP_KERNEL);
		if (!g_data->o_buf) {
			BTIF_ERR("%s: alloc memory fail (g_data->o_buf)",
				__func__);
			return -1;
		}
	}

	if (g_data->i_fwlog_buf == NULL) {
		g_data->i_fwlog_buf = kzalloc(BUFFER_SIZE, GFP_KERNEL);
		if (!g_data->i_fwlog_buf) {
			BTIF_ERR("%s: alloc memory fail (g_data->i_fwlog_buf)", __func__);
			return -1;
		}
	}

	if (g_data->o_fwlog_buf == NULL) {
		g_data->o_fwlog_buf = kzalloc(BUFFER_SIZE, GFP_KERNEL);
		if (!g_data->o_fwlog_buf) {
			BTIF_ERR("%s: alloc memory fail (g_data->o_fwlog_buf)", __func__);
			return -1;
		}
	}

	p_event_buffer = NULL;
	BTIF_INFO("%s: Success", __func__);
	return 1;
}

static void btmtk_btif_free_memory(void)
{
	if (!g_data) {
		BTIF_ERR("%s: g_data is NULL!\n", __func__);
		return;
	}

	kfree(g_data->metabuffer);
	g_data->metabuffer = NULL;

	kfree(g_data->fwlog_metabuffer);
	g_data->fwlog_metabuffer = NULL;

	kfree(g_data->i_buf);
	g_data->i_buf = NULL;

	kfree(g_data->o_buf);
	g_data->o_buf = NULL;

	kfree(g_data->i_fwlog_buf);
	g_data->i_fwlog_buf = NULL;

	kfree(g_data->o_fwlog_buf);
	g_data->o_fwlog_buf = NULL;

	kfree(g_data->rom_patch_bin_file_name);
	g_data->rom_patch_bin_file_name = NULL;

	kfree(g_data->io_buf);
	g_data->io_buf = NULL;

	kfree(g_data->rom_patch_image);
	g_data->rom_patch_image = NULL;

	kfree(g_data);
	g_data = NULL;

	BTIF_INFO("%s: Success", __func__);
}

static int btmtk_btif_get_state(void)
{
	return BTMTK_BTIF_STATE_WORKING;/*btmtk_btif_state;*/
}

static void btmtk_btif_set_state(int new_state)
{
	static const char * const state_msg[] = {
		"UNKNOWN", "INIT", "DISCONNECT", "PROBE", "WORKING", "EARLY_SUSPEND",
		"SUSPEND", "RESUME", "LATE_RESUME", "FW_DUMP", "SUSPEND_DISCONNECT",
		"SUSPEND_PROBE", "RESUME_DISCONNECT", "RESUME_PROBE", "RESUME_FW_DUMP"
	};

	BTIF_INFO("%s: %s(%d) -> %s(%d)", __func__, state_msg[btmtk_btif_state],
			btmtk_btif_state, state_msg[new_state], new_state);
	btmtk_btif_state = new_state;
}

static int btmtk_fops_get_state(void)
{
	return btmtk_fops_state;
}

static void btmtk_fops_set_state(int new_state)
{
	static const char * const fstate_msg[] = {"UNKNOWN", "INIT", "OPENED", "CLOSING", "CLOSED"};

	BTIF_INFO("%s: FOPS_%s(%d) -> FOPS_%s(%d)", __func__, fstate_msg[btmtk_fops_state],
			btmtk_fops_state, fstate_msg[new_state], new_state);
	btmtk_fops_state = new_state;
}

static unsigned int btmtk_btif_hci_snoop_get_microseconds(void)
{
	struct timeval now;

	do_gettimeofday(&now);
	return (now.tv_sec * 1000000 + now.tv_usec);
}

static void btmtk_btif_hci_snoop_init(void)
{
	int i;

	hci_cmd_snoop_index = HCI_SNOOP_ENTRY_NUM - 1;
	hci_event_snoop_index = HCI_SNOOP_ENTRY_NUM - 1;
	hci_acl_snoop_index = HCI_SNOOP_ENTRY_NUM - 1;
	for (i = 0; i < HCI_SNOOP_ENTRY_NUM; i++) {
		hci_cmd_snoop_len[i] = 0;
		hci_event_snoop_len[i] = 0;
		hci_acl_snoop_len[i] = 0;
	}
}

static void btmtk_btif_hci_snoop_save_cmd(u32 len, u8 *buf)
{
	u32 copy_len = HCI_SNOOP_BUF_SIZE;

	if (buf) {
		if (len < HCI_SNOOP_BUF_SIZE)
			copy_len = len;
		hci_cmd_snoop_len[hci_cmd_snoop_index] = copy_len & 0xff;
		memset(hci_cmd_snoop_buf[hci_cmd_snoop_index], 0, HCI_SNOOP_BUF_SIZE);
		memcpy(hci_cmd_snoop_buf[hci_cmd_snoop_index], buf, copy_len & 0xff);
		hci_cmd_snoop_timestamp[hci_cmd_snoop_index] = btmtk_btif_hci_snoop_get_microseconds();

		hci_cmd_snoop_index--;
		if (hci_cmd_snoop_index < 0)
			hci_cmd_snoop_index = HCI_SNOOP_ENTRY_NUM - 1;
	}
}

/*
static void btmtk_btif_hci_snoop_save_event(u32 len, u8 *buf)
{
	u32 copy_len = HCI_SNOOP_BUF_SIZE;

	if (buf) {
		if (len < HCI_SNOOP_BUF_SIZE)
			copy_len = len;
		hci_event_snoop_len[hci_event_snoop_index] = copy_len;
		memset(hci_event_snoop_buf[hci_event_snoop_index], 0,
			HCI_SNOOP_BUF_SIZE);
		memcpy(hci_event_snoop_buf[hci_event_snoop_index], buf, copy_len);
		hci_event_snoop_timestamp[hci_event_snoop_index] =
			btmtk_btif_hci_snoop_get_microseconds();

		hci_event_snoop_index--;
		if (hci_event_snoop_index < 0)
			hci_event_snoop_index = HCI_SNOOP_ENTRY_NUM - 1;
	}
}
*/
static void btmtk_btif_hci_snoop_save_acl(u32 len, u8 *buf)
{
	u32 copy_len = HCI_SNOOP_BUF_SIZE;

	if (buf) {
		if (len < HCI_SNOOP_BUF_SIZE)
			copy_len = len;
		hci_acl_snoop_len[hci_acl_snoop_index] = copy_len & 0xff;
		memset(hci_acl_snoop_buf[hci_acl_snoop_index], 0,
			HCI_SNOOP_BUF_SIZE);
		memcpy(hci_acl_snoop_buf[hci_acl_snoop_index], buf, copy_len & 0xff);
		hci_acl_snoop_timestamp[hci_acl_snoop_index] =
			btmtk_btif_hci_snoop_get_microseconds();

		hci_acl_snoop_index--;
		if (hci_acl_snoop_index < 0)
			hci_acl_snoop_index = HCI_SNOOP_ENTRY_NUM - 1;
	}
}

static void btmtk_btif_hci_snoop_print_to_log(void)
{
	int counter, index, j;

	BTIF_INFO("HCI Command Dump");
	BTIF_INFO("  index(len)(timestamp:us) :HCI Command");
	index = hci_cmd_snoop_index + 1;
	if (index >= HCI_SNOOP_ENTRY_NUM)
		index = 0;
	for (counter = 0; counter < HCI_SNOOP_ENTRY_NUM; counter++) {
		if (hci_cmd_snoop_len[index] > 0) {
			pr_cont("	%d(%02d)(%u) :", counter,
				hci_cmd_snoop_len[index],
				hci_cmd_snoop_timestamp[index]);
			for (j = 0; j < hci_cmd_snoop_len[index]; j++)
				pr_cont("%02X ", hci_cmd_snoop_buf[index][j]);
			pr_cont("\n");
		}
		index++;
		if (index >= HCI_SNOOP_ENTRY_NUM)
			index = 0;
	}

	BTIF_INFO("HCI Event Dump");
	BTIF_INFO("  index(len)(timestamp:us) :HCI Event");
	index = hci_event_snoop_index + 1;
	if (index >= HCI_SNOOP_ENTRY_NUM)
		index = 0;
	for (counter = 0; counter < HCI_SNOOP_ENTRY_NUM; counter++) {
		if (hci_event_snoop_len[index] > 0) {
			pr_cont("	%d(%02d)(%u) :", counter,
				hci_event_snoop_len[index],
				hci_event_snoop_timestamp[index]);
			for (j = 0; j < hci_event_snoop_len[index]; j++)
				pr_cont("%02X ", hci_event_snoop_buf[index][j]);
			pr_cont("\n");
		}
		index++;
		if (index >= HCI_SNOOP_ENTRY_NUM)
			index = 0;
	}

	BTIF_INFO("HCI ACL Dump");
	BTIF_INFO("  index(len)(timestamp:us) :ACL");
	index = hci_acl_snoop_index + 1;
	if (index >= HCI_SNOOP_ENTRY_NUM)
		index = 0;
	for (counter = 0; counter < HCI_SNOOP_ENTRY_NUM; counter++) {
		if (hci_acl_snoop_len[index] > 0) {
			pr_cont("	%d(%02d)(%u) :", counter,
				hci_acl_snoop_len[index],
				hci_acl_snoop_timestamp[index]);
			for (j = 0; j < hci_acl_snoop_len[index]; j++)
				pr_cont("%02X ", hci_acl_snoop_buf[index][j]);
			pr_cont("\n");
		}
		index++;
		if (index >= HCI_SNOOP_ENTRY_NUM)
			index = 0;
	}
}

static void btmtk_btif_lock_unsleepable_lock(struct OSAL_UNSLEEPABLE_LOCK *pUSL)
{
	spin_lock_irqsave(&(pUSL->lock), pUSL->flag);
}

static void btmtk_btif_unlock_unsleepable_lock(struct OSAL_UNSLEEPABLE_LOCK *pUSL)
{
	spin_unlock_irqrestore(&(pUSL->lock), pUSL->flag);
}

static int btmtk_btif_send_wmt_cmd(const u8 *cmd, const int cmd_len,
		const u8 *event, const int event_len, u32 delay)
{
	int ret = -1;	/* if successful, read length */
	int i = 0;
	bool check = FALSE;

	ret = btmtk_btif_meta_send_data(cmd, cmd_len);
	if (ret < 0)
		return ret;
	if (event != NULL && event_len > 0)
		check = TRUE;
	else
		return ret;
	wait_event_timeout(p_wait_event_q, p_event_buffer != NULL, 2*HZ);
	if (p_event_buffer == NULL) {
		BTIF_ERR("%s: no event, timeout.", __func__);
		return -1;
	}
	for (i = 0; i < event_len; i++) {
#if STP_SUPPORT
		if (event[i] != p_event_buffer[i + 4])
#else
		if (event[i] != p_event_buffer[i])
#endif
		break;
	}
	if (i != event_len) {
		BTIF_ERR("%s: event not match.", __func__);
		ret = -1;
	}
	kfree(p_event_buffer);
	p_event_buffer = NULL;
	return ret;

}

static int btmtk_btif_send_hci_cmd(const u8 *cmd, const int cmd_len,
		const u8 *event, const int event_len)
{
	int ret = -1;	/* if successful, read length */

	ret = btmtk_btif_meta_send_data(cmd, cmd_len);
	return ret;
}

void dump_hex(unsigned char *buf, int dump_len)
{
	BTIF_WARN("----dump_packet");
	BTIF_DBG_RAW(buf, dump_len, "%s: len: %d, Data:", __func__, dump_len);
	BTIF_WARN("----end dump_packet\n\n");
}

#if STP_SUPPORT
static bool btmtk_add_stp_header(const u8 *in_buffer, const unsigned int length, u8 *out_buffer)
{
	out_buffer[0] = 0x80;
	out_buffer[1] = (0x0 << 4) + ((length & 0xf00) >> 8);
	out_buffer[2] = length & 0xff;
	out_buffer[3] = (out_buffer[0] + out_buffer[1] + out_buffer[2]) & 0xff;

	/*Make Payload */
	memcpy((void *)&out_buffer[4], in_buffer, length);

	/*Make CRC */
	out_buffer[length + 4] = 0x0;
	out_buffer[length + 5] = 0x0;
	BTIF_WARN("Data with STP header");
	dump_hex(out_buffer, length+6);

	return true;
}

#endif
int btmtk_btif_meta_send_data(const u8 *buffer, const unsigned int length)
{
	int ret = -1;
	u8 *buf = NULL;
	unsigned int leng;

	if (buffer[0] != 0x01) {
		BTIF_WARN("the data from meta isn't HCI command, value: 0x%X",
			buffer[0]);
	} else {
#if STP_SUPPORT
	leng = length + 6;
	buf = kcalloc(leng, sizeof(u8), GFP_KERNEL);
	btmtk_add_stp_header(buffer, length, buf);
#else
	buf = (u8 *)buffer;
	leng = length;
#endif
	ret = mtk_wcn_btif_write(g_data->stp_btif_id, buf, leng);
#if STP_SUPPORT
	kfree(buf);
#endif
	}

	if (ret < 0) {
		BTIF_ERR("%s: error1(%d)", __func__, ret);
		return ret;
	}

	return length;
}

int btmtk_btif_send_data(const u8 *buffer, const unsigned int length)
{
	int err;
	u8 *buf = NULL;
	unsigned int leng;

	int ret = -1;

	if (buffer[0] == 0x02) {

		while (g_data->meta_tx != 0)
			udelay(500);

		g_data->meta_tx = 1;

#if STP_SUPPORT
	leng = length + 6;
	buf = kcalloc(leng, sizeof(u8), GFP_KERNEL);
	btmtk_add_stp_header(buffer, length, buf);
#else
	buf = (u8 *)buffer;
	leng = length;
#endif
	ret = mtk_wcn_btif_write(g_data->stp_btif_id, buf, leng);
#if STP_SUPPORT
	kfree(buf);
#endif
	g_data->meta_tx = 0;
	if (ret < 0) {
		BTIF_ERR("%s: error1(%d)", __func__, ret);
		return ret;
	}
	} else if (buffer[0] == 0x03) {
		BTIF_WARN("%s: Iso not supported", __func__);
		err = -1;
	} else {
		BTIF_WARN("%s: unknown data", __func__);
		err = -1;
	}

	return err;

}

static int btmtk_btif_BT_init(void)
{
	dev_t devID = MKDEV(BT_major, 0);
	dev_t devIDfwlog = MKDEV(BT_major, 1);
	dev_t devID_raw = MKDEV(BT_major, 2);
	int ret = 0;
	int cdevErr = 0;
	int major = 0;
	int majorfwlog = 0;

	BTIF_INFO("%s: begin", __func__);
	BTIF_INFO("%s: devID %d, devIDfwlog %d", __func__, devID, devIDfwlog);

	mutex_lock(&btmtk_btif_state_mutex);
	btmtk_btif_set_state(BTMTK_BTIF_STATE_INIT);
	mutex_unlock(&btmtk_btif_state_mutex);

#ifdef FIXED_STPBT_MAJOR_DEV_ID
	ret = register_chrdev_region(devID, 1, "BT_chrdev");
	if (ret) {
		BTIF_ERR("fail to register_chrdev MAJOR:%d MINOR:%d, allocate chrdev again", MAJOR(devID), 0);
		ret = alloc_chrdev_region(&devID, 0, 1, "BT_chrdev");
		if (ret) {
			BTIF_ERR("fail to allocate chrdev");
			return ret;
		}
	}

	ret = register_chrdev_region(devIDfwlog, 1, "BT_chrdevfwlog");
	if (ret) {
		BTIF_ERR("fail to register_chrdev MAJOR:%d MINOR:%d, allocate chrdev again", MAJOR(devID), 1);
		ret = alloc_chrdev_region(&devIDfwlog, MAJOR(devID), 1, "BT_chrdevfwlog");
		if (ret) {
			BTIF_ERR("fail to allocate chrdev fwlog");
			return ret;
		}
	}
	ret = register_chrdev_region(devID_raw, 1, "BT_chrdev_raw");
	if (ret) {
		BTIF_ERR("fail to register_chrdev MAJOR:%d MINOR:%d, allocate chrdev again", MAJOR(devID), 2);
		ret = alloc_chrdev_region(&devIDfwlog, MAJOR(devID), 1, "BT_chrdev_raw");
		if (ret) {
			BTIF_ERR("fail to allocate chrdev raw");
			return ret;
		}
	}

#else
	ret = alloc_chrdev_region(&devID, 0, 1, "BT_chrdev");
	if (ret) {
		BTIF_ERR("fail to allocate chrdev");
		return ret;
	}

	ret = alloc_chrdev_region(&devIDfwlog, MAJOR(devID), 1, "BT_chrdevfwlog");
	if (ret) {
		BTIF_ERR("fail to allocate chrdev fwlog");
		return ret;
	}

	ret = alloc_chrdev_region(&devID_raw, MAJOR(devID), 1, "BT_chrdev_raw");
	if (ret) {
		BTIF_ERR("fail to allocate chrdev raw");
		return ret;
	}
#endif

	BT_major = major = MAJOR(devID);
	BTIF_INFO("%s: major number: %d", __func__, BT_major);
	BT_majorfwlog = majorfwlog = MAJOR(devIDfwlog);
	BTIF_INFO("%s: BT_majorfwlog number: %d", __func__, BT_majorfwlog);

	cdev_init(&BT_cdev, &BT_fops);
	BT_cdev.owner = THIS_MODULE;
	cdevErr = cdev_add(&BT_cdev, devID, 1);
	if (cdevErr)
		goto error;
		BTIF_INFO("%s: %s driver(major %d, minor %d) installed.", __func__, "BT_chrdev",
		MAJOR(devID), MINOR(devID));

	cdev_init(&BT_cdevfwlog, &BT_fopsfwlog);
	BT_cdevfwlog.owner = THIS_MODULE;
	cdevErr = cdev_add(&BT_cdevfwlog, devIDfwlog, 1);
	if (cdevErr)
		goto error;
	BTIF_INFO("%s: %s driver(major %d, minor %d) installed.", __func__, "BT_chrdevfwlog",
		MAJOR(devIDfwlog), MINOR(devIDfwlog));


	cdev_init(&BT_cdev_raw, &BT_fops_raw);
	BT_cdev_raw.owner = THIS_MODULE;
	cdevErr = cdev_add(&BT_cdev_raw, devID_raw, 1);
	if (cdevErr)
		goto error;
	BTIF_INFO("%s: %s driver(major %d, minor %d) installed.", __func__, "BT_cdev_raw",
		MAJOR(devID_raw), MINOR(devID_raw));

	pBTClass = class_create(THIS_MODULE, "BT_chrdev");
	if (IS_ERR(pBTClass)) {
		BTIF_ERR("class create fail, error code(%ld)",
			PTR_ERR(pBTClass));
		goto err1;
	}

	pBTDev = device_create(pBTClass, NULL, devID, NULL, "stpbt");
	if (IS_ERR(pBTDev)) {
		BTIF_ERR("device create fail, error code(%ld)",
			PTR_ERR(pBTDev));
		goto err2;
	}

	pBTDevfwlog = device_create(pBTClass, NULL, devIDfwlog, NULL, "stpbtfwlog");
	if (IS_ERR(pBTDevfwlog)) {
		BTIF_ERR("device(stpbtfwlog) create fail, error code(%ld)",
			PTR_ERR(pBTDevfwlog));
		goto err2;
	}

	pBTDev_raw = device_create(pBTClass, NULL, devID_raw, NULL, "stpbt_raw");
	if (IS_ERR(pBTDev_raw)) {
		BTIF_ERR("device(stpbt_raw) create fail, error code(%ld)",
			PTR_ERR(pBTDev_raw));
		goto err2;
	}

	BTIF_INFO("%s: BT_major %d, BT_majorfwlog %d", __func__, BT_major, BT_majorfwlog);
	BTIF_INFO("%s: devID %d, devIDfwlog %d", __func__, devID, devIDfwlog);
	g_devID = devID;
	g_devIDfwlog = devIDfwlog;
	g_devID_raw = devID_raw;
	FOPS_MUTEX_LOCK();
	btmtk_fops_set_state(BTMTK_FOPS_STATE_INIT);
	FOPS_MUTEX_UNLOCK();

	/* init wait queue */
	init_waitqueue_head(&(inq));
	init_waitqueue_head(&(fw_log_inq));

	/* allocate buffers. */
	if (btmtk_btif_allocate_memory() < 0) {
		BTIF_ERR("%s: allocate memory failed!", __func__);
		goto err2;
	}

	/* init buffers. */
	if (btmtk_btif_init_memory() < 0) {
		BTIF_ERR("%s: init memory failed!", __func__);
		goto err2;
	}

	mutex_lock(&btmtk_btif_state_mutex);
	btmtk_btif_set_state(BTMTK_BTIF_STATE_DISCONNECT);
	mutex_unlock(&btmtk_btif_state_mutex);

	BTIF_INFO("%s: end", __func__);
	return 0;

err2:
	if (pBTClass) {
		class_destroy(pBTClass);
		pBTClass = NULL;
	}

	btmtk_btif_free_memory();

err1:
error:
	if (cdevErr == 0)
		cdev_del(&BT_cdev);

	if (ret == 0)
		unregister_chrdev_region(devID, 1);
	BTIF_ERR("%s: error", __func__);
	return -1;
}

static void btmtk_btif_BT_exit(void)
{
	dev_t dev = g_devID;
	dev_t devIDfwlog = g_devIDfwlog;/*MKDEV(BT_majorfwlog, 0);*/
	dev_t devID_raw = g_devID_raw;

	BTIF_INFO("%s: BT_major %d, BT_majorfwlog %d", __func__, BT_major, BT_majorfwlog);
	BTIF_INFO("%s: dev %d, devIDfwlog %d", __func__, dev, devIDfwlog);

	FOPS_MUTEX_LOCK();
	btmtk_fops_set_state(BTMTK_FOPS_STATE_UNKNOWN);
	FOPS_MUTEX_UNLOCK();

	if (pBTDevfwlog) {
		device_destroy(pBTClass, devIDfwlog);
		pBTDevfwlog = NULL;
	}

	if (pBTDev) {
		device_destroy(pBTClass, dev);
		pBTDev = NULL;
	}

	if (pBTDev_raw) {
		device_destroy(pBTClass, devID_raw);
		pBTDev = NULL;
	}

	if (pBTClass) {
		class_destroy(pBTClass);
		pBTClass = NULL;
	}

	cdev_del(&BT_cdevfwlog);
	unregister_chrdev_region(devIDfwlog, 1);

	cdev_del(&BT_cdev);
	unregister_chrdev_region(dev, 1);

	cdev_del(&BT_cdev_raw);
	unregister_chrdev_region(devID_raw, 1);

	btmtk_btif_free_memory();

	mutex_lock(&btmtk_btif_state_mutex);
	btmtk_btif_set_state(BTMTK_BTIF_STATE_UNKNOWN);
	mutex_unlock(&btmtk_btif_state_mutex);

	BTIF_INFO("%s: BT_chrdev driver removed.", __func__);
}

/*============================================================================*/
/* Internal Functions : Chip Related */
/*============================================================================*/
#define ______________________________________Internal_Functions_Chip_Related
/**
 * Only for load rom patch function, tmp_str[15] is '\n'
 */
#define SHOW_FW_DETAILS(s)							\
	BTIF_INFO("%s: %s = %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c", __func__, s,	\
			tmp_str[0], tmp_str[1], tmp_str[2], tmp_str[3],		\
			tmp_str[4], tmp_str[5], tmp_str[6], tmp_str[7],		\
			tmp_str[8], tmp_str[9], tmp_str[10], tmp_str[11],	\
			tmp_str[12], tmp_str[13], tmp_str[14]/*, tmp_str[15]*/)

static int btmtk_btif_send_wmt_power_on_cmd_7622(void)
{
	/*u8 count = 0;	 retry 3 times */
	u8 cmd[] = { 0x01, 0x6F, 0xFC, 0x06, 0x01, 0x06, 0x02, 0x00, 0x00, 0x01 };
	u8 event[] = { 0x04, 0xE4, 0x05, 0x02, 0x06, 0x01, 0x00 };	/* event[6] is key */
	int ret = -1;	/* if successful, 0 */

	g_data->event_state = BTMTK_EVENT_STATE_POWERING_ON;

	ret = btmtk_btif_send_wmt_cmd(cmd, sizeof(cmd), event, sizeof(event), 20);
	if (ret < 0) {
		BTIF_ERR("%s: failed(%d)", __func__, ret);
		g_data->event_state = BTMTK_EVENT_STATE_ERROR;

	} else
		g_data->event_state = BTMTK_EVENT_STATE_POWER_ON;

	BTIF_INFO("%s: OK", __func__);

	return ret;
}

static int btmtk_btif_send_wmt_power_off_cmd_7622(void)
{
	u8 cmd[] = { 0x01, 0x6F, 0xFC, 0x06, 0x01, 0x06, 0x02, 0x00, 0x00, 0x00 };
	u8 event[] = { 0x04, 0xE4, 0x05, 0x02, 0x06, 0x01, 0x00, 0x00 };
	int ret = -1;	/* if successful, 0 */

	g_data->event_state = BTMTK_EVENT_STATE_POWERING_OFF;
	ret = btmtk_btif_send_wmt_cmd(cmd, sizeof(cmd), event, sizeof(event), 20);
	if (ret < 0) {
		BTIF_ERR("%s: failed(%d)", __func__, ret);
		g_data->event_state = BTMTK_EVENT_STATE_ERROR;
		return ret;
	}

	BTIF_INFO("%s: OK", __func__);
	g_data->event_state = BTMTK_EVENT_STATE_POWER_OFF;
	return 0;
}

/*============================================================================*/
/* Internal Functions : Send HCI/WMT */
/*============================================================================*/
#define ______________________________________Internal_Functions_Send_HCI_WMT


/*============================================================================*/
/* Callback Functions */
/*============================================================================*/
#define ___________________________________________________Callback_Functions

static void btmtk_bitf_fwlog_queue(const unsigned char *p_buf, unsigned int len)
{
	struct sk_buff *skb_fwlog = NULL;
	ulong flags = 0;
	u8 *buf = (u8 *)p_buf;

	skb_fwlog = alloc_skb(len, GFP_ATOMIC);
	if (skb_fwlog == NULL) {
		BTIF_ERR("%s: alloc_skb return 0, error", __func__);
		return;
	}

	memcpy(&skb_fwlog->data[0], buf, len);
	skb_fwlog->len = len;
	spin_lock_irqsave(&g_data->fwlog_lock, flags);
	skb_queue_tail(&g_data->fwlog_queue, skb_fwlog);
	g_data->fwlog_queue_count++;
	spin_unlock_irqrestore(&g_data->fwlog_lock, flags);
	wake_up_interruptible(&fw_log_inq);

}

static void btmtk_bitf_rx_cb(const unsigned char *p_buf, unsigned int len)
{
	u32 length;
	int copyLen = 0;

	u8 *buf = (u8 *)p_buf;
	u8 *event_buf = NULL;
	dump_hex(buf, len);
	length = len;

	btmtk_write_buffer(g_data->fwlog_metabuffer, buf, length);
	while (0 < (copyLen = btmtk_read_buffer(g_data->fwlog_metabuffer, g_data->i_fwlog_buf, BUFFER_SIZE))) {
		event_buf = (u8 *)g_data->i_fwlog_buf;
#if STP_SUPPORT
		event_buf += 4;
		copyLen -= 6;
#endif
		if (g_data->event_state == BTMTK_EVENT_STATE_POWERING_ON ||
			g_data->event_state == BTMTK_EVENT_STATE_POWERING_OFF ||
			g_data->event_state == BTMTK_EVENT_STATE_WAIT_EVENT) {
			u8 event[2] = { 0x04, 0xE4 };
			if ((event_buf[0] == event[0]) && (event_buf[1] == event[1]) ) {
				p_event_buffer = kzalloc(copyLen, GFP_KERNEL);
#if STP_SUPPORT
				event_buf -= 4;
				copyLen += 6;
#endif
				memcpy(p_event_buffer, event_buf, copyLen);
				wake_up(&p_wait_event_q);
				continue;
			}
		}

		if (event_buf[1] == 0xFF && (event_buf[3] == 0x50 || event_buf[3] == 0x51) &&
			(g_data->fwlog_queue_count < FWLOG_QUEUE_COUNT)) {
			btmtk_bitf_fwlog_queue(event_buf, copyLen);
			continue;
		} else if (event_buf[1] == 0x6f && event_buf[2] == 0xfc &&
			(g_data->fwlog_queue_count < FWLOG_ASSERT_QUEUE_COUNT)) {
			btmtk_bitf_fwlog_queue(event_buf, copyLen);
			continue;
		}
#if STP_SUPPORT
		event_buf -= 4;
		copyLen += 6;
#endif

		btmtk_btif_hci_snoop_save_acl(copyLen, event_buf);
		btmtk_write_buffer(g_data->metabuffer, event_buf, copyLen);
		if (btmtk_btif_get_state() ==
			BTMTK_BTIF_STATE_WORKING) {
			wake_up(&BT_wq);
			wake_up_interruptible(&inq);
		} else {
			BTIF_WARN
			("%s: current is in suspend/resume (%d), Don't wake-up wait queue",
			__func__, btmtk_btif_get_state());
		}
	}

}

/*============================================================================*/
/* Interface Functions : BT Stack */
/*============================================================================*/
#define ______________________________________Interface_Function_for_BT_Stack
static ssize_t btmtk_btif_fops_write(struct file *file, const char __user *buf,
					size_t count, loff_t *f_pos)
{
	int retval = 0;
	int state = BTMTK_BTIF_STATE_UNKNOWN;
	int fstate = BTMTK_FOPS_STATE_UNKNOWN;

	if (g_data == NULL) {
		BTIF_ERR("%s: ERROR, g_data is NULL!", __func__);
		return -ENODEV;
	}

	FOPS_MUTEX_LOCK();
	fstate = btmtk_fops_get_state();
	if (fstate != BTMTK_FOPS_STATE_OPENED) {
		BTIF_WARN("%s: fops is not open yet(%d)!", __func__, fstate);
		FOPS_MUTEX_UNLOCK();
		return -ENODEV;
	}
	FOPS_MUTEX_UNLOCK();

	mutex_lock(&btmtk_btif_state_mutex);
	if (enter_sleep)
		btmtk_wakeup_consys();
	state = btmtk_btif_get_state();
	if (state != BTMTK_BTIF_STATE_WORKING) {
		BTIF_DBG("%s: current is in suspend/resume (%d).", __func__, state);
		mutex_unlock(&btmtk_btif_state_mutex);
		msleep(3000);
		return -EAGAIN;
	}
	mutex_unlock(&btmtk_btif_state_mutex);

	/* semaphore mechanism, the waited process will sleep */
	down(&g_data->wr_mtx);

	if (count > 0) {
		u32 copy_size = (count < BUFFER_SIZE) ? count : BUFFER_SIZE;

		if (copy_from_user(&g_data->o_buf[0], &buf[0], copy_size)) {
			retval = -EFAULT;
			BTIF_ERR("%s: copy data from user fail", __func__);
			goto OUT;
		}
		BTIF_DBG_RAW(buf, 16, "%s: (len=%2d)", __func__, copy_size);

		/* command */
		if (g_data->o_buf[0] == 0x01) {
			/* parsing commands */
			u8 fw_assert_cmd[] = { 0x01, 0x6F, 0xFC, 0x05, 0x01, 0x02, 0x01, 0x00, 0x08 };
			u8 reset_cmd[] = { 0x01, 0x03, 0x0C, 0x00 };
			u8 read_ver_cmd[] = { 0x01, 0x01, 0x10, 0x00 };

			if (copy_size == sizeof(fw_assert_cmd) &&
					!memcmp(g_data->o_buf, fw_assert_cmd, sizeof(fw_assert_cmd))) {
				BTIF_INFO("%s: Donge FW Assert Triggered by BT Stack!", __func__);
				btmtk_btif_hci_snoop_print_to_log();

			} else if (copy_size == sizeof(reset_cmd) &&
					!memcmp(g_data->o_buf, reset_cmd, sizeof(reset_cmd))) {
				BTIF_INFO("%s: got command: 0x03 0C 00 (HCI_RESET)", __func__);

			} else if (copy_size == sizeof(read_ver_cmd) &&
					!memcmp(g_data->o_buf, read_ver_cmd, sizeof(read_ver_cmd))) {
				BTIF_INFO("%s: got command: 0x01 10 00 (READ_LOCAL_VERSION)",
						__func__);
			}

			btmtk_btif_hci_snoop_save_cmd(copy_size, &g_data->o_buf[0]);
			retval = btmtk_btif_meta_send_data(&g_data->o_buf[0], copy_size);

		/* ACL data */
		} else if (g_data->o_buf[0] == 0x02) {
			retval = btmtk_btif_send_data(&g_data->o_buf[0], copy_size);

		/* SCO data */
		} else if (g_data->o_buf[0] == 0x03) {
			retval = btmtk_btif_send_data(&g_data->o_buf[0], copy_size);

		/* Unknown */
		} else {
			BTIF_WARN("%s: this is unknown bt data:0x%02x", __func__,
					g_data->o_buf[0]);
		}
		mutex_lock(&btmtk_btif_state_mutex);
		if (!raw_mode) {
			if (sleep_timer_enable)
				mod_timer(&sleep_timer, jiffies + SLEEP_TIME);
			else {
				sleep_timer.expires = jiffies + SLEEP_TIME;
				add_timer(&sleep_timer);
				sleep_timer_enable = true;
			}
		}
		mutex_unlock(&btmtk_btif_state_mutex);
	} else {
		retval = -EFAULT;
		BTIF_ERR
			("%s: target packet length:%zu is not allowed, retval = %d",
			 __func__, count, retval);
	}

OUT:
	up(&g_data->wr_mtx);
	return retval;
}

static ssize_t btmtk_btif_fops_writefwlog(struct file *filp, const char __user *buf,
					size_t count, loff_t *f_pos)
{
	int length = 0;
	int i = 0;
	int j = 0;
	int ret = -1;	/* if successful, 0 */
	const char *val_param = NULL;

	memset(g_data->o_fwlog_buf, 0, BUFFER_SIZE);
	mutex_lock(&btmtk_btif_state_mutex);
	if (enter_sleep)
		btmtk_wakeup_consys();
	mutex_unlock(&btmtk_btif_state_mutex);
	/* Command example : echo 01 be fc 01 05 > /dev/stpbtfwlog */
	if (count > HCI_MAX_COMMAND_BUF_SIZE) {
		BTIF_ERR("%s: your command is larger than maximum length, count = %zd\n", __func__, count);
		goto exit;
	}

	for (i = 0; i < count; i++) {
		if (buf[i] != '=') {
			g_data->o_fwlog_buf[i] = buf[i];
			BTIF_DBG("%s: tag_oparam %02x\n", __func__, g_data->o_fwlog_buf[i]);
		} else {
			/* for input log_lvl, ex echo log_lvl=4 > /dev/stpbtfwlog */
			val_param = &buf[i+1];
			if (strcmp(g_data->o_fwlog_buf, "log_lvl") == 0) {
				BTIF_DBG("%s: btmtk_log_lvl = %d\n", __func__, val_param[0] - 48);
				btmtk_log_lvl = val_param[0] - 48;
			}
			goto exit;
		}
	}
	if (i == count) {
		/* hci input command format : echo 01 be fc 01 05 > /dev/stpbtfwlog */
		/* We take the data from index to end. */
		val_param = &buf[HCI_PACKET_OFFSET];
	}
	length = strlen(val_param);

	for (i = 0; i < length; i++) {
		u8 ret = 0;
		u8 temp_str[3] = { 0 };
		long res = 0;

		if (val_param[i] == ' ' || val_param[i] == '\t' || val_param[i] == '\r'	|| val_param[i] == '\n')
			continue;
		if ((val_param[i] == '0' && val_param[i + 1] == 'x')
			|| (val_param[0] == '0' && val_param[i + 1] == 'X')) {
			i++;
			continue;
		}
		if (!(val_param[i] >= '0' && val_param[i] <= '9')
			&& !(val_param[i] >= 'A' && val_param[i] <= 'F')
			&& !(val_param[i] >= 'a' && val_param[i] <= 'f')) {
			break;
		}
		temp_str[0] = *(val_param+i);
		temp_str[1] = *(val_param+i+1);
		ret = (u8) (kstrtol((char *)temp_str, 16, &res) & 0xff);
		g_data->o_fwlog_buf[j++] = res;
		i++;
	}
	length = j;

	/* Receive command from stpbtfwlog, then Sent hci command to controller */
	BTIF_DBG("%s: hci buff is %02x%02x%02x%02x%02x ", __func__, g_data->o_fwlog_buf[0], g_data->o_fwlog_buf[1],
		 g_data->o_fwlog_buf[2], g_data->o_fwlog_buf[3], g_data->o_fwlog_buf[4]);

	/* check HCI command length */
	if (length > HCI_MAX_COMMAND_SIZE) {
		BTIF_ERR("%s: your command is larger than maximum length, length = %d\n", __func__, length);
		goto exit;
	}

	/* command */
	if (g_data->o_fwlog_buf[0] == 0x01) {
		/* parsing commands */
		u8 fw_assert_cmd[] = { 0x01, 0x6F, 0xFC, 0x05, 0x01, 0x02, 0x01, 0x00, 0x08 };
		if (!memcmp(g_data->o_fwlog_buf, fw_assert_cmd, sizeof(fw_assert_cmd))) {
			BTIF_INFO("%s: Donge FW Assert Triggered by BT Stack!", __func__);
			fw_dump_enable = true;
		}
	}
	/* send HCI command */
	ret = btmtk_btif_send_hci_cmd((u8 *)g_data->o_fwlog_buf, length, NULL, -1);
	if (ret < 0) {
		BTIF_ERR("%s: command send failed(%d)", __func__, ret);
		return ret;
	}

	BTIF_INFO("%s: write end\n", __func__);
exit:
	BTIF_INFO("%s: exit write end\n", __func__);
	BTIF_INFO("%s: total length is %d\n", __func__, length);
	return count;
}

static int btmtk_get_buffer_size(struct ring_buffer_struct *metabuffer)
{
	int size;

	size = metabuffer->write_p - metabuffer->read_p;
	if (size < 0)
		size += META_BUFFER_SIZE;
	return size;
}

/* read buffer from metabuffer to p_buff */
static int btmtk_read_buffer(struct ring_buffer_struct *metabuffer, unsigned char *p_buf, int count)
{
	int copyLen = 0;
	u8 *buffer = NULL;
	u8 *hci_pointer = NULL;
	unsigned short tailLen = 0;

	buffer = p_buf;
	btmtk_btif_lock_unsleepable_lock(&(metabuffer->spin_lock));
	while (metabuffer->read_p != metabuffer->write_p) {
		if (metabuffer->write_p > metabuffer->read_p) {
			hci_pointer = metabuffer->buffer + metabuffer->read_p;
#if STP_SUPPORT
			hci_pointer += 4;
#endif
			switch (hci_pointer[0]) {
			case 0x02:
				copyLen = (hci_pointer[3] + ((hci_pointer[4] << 8) & 0xff00))+5;
				break;
			case 0x04:
				copyLen = hci_pointer[2]+3;
				break;
			default:
				BTIF_WARN("%s: Unsupport now !!", __func__);
			}
#if STP_SUPPORT
			copyLen += 6;
#endif
			if (copyLen > btmtk_get_buffer_size(metabuffer)) {
				BTIF_ERR("%s: data not ready.", __func__);
				copyLen = -EAGAIN;
				btmtk_btif_unlock_unsleepable_lock(&(metabuffer->spin_lock));
				return copyLen;
			}
#if STP_SUPPORT
			hci_pointer -= 4;
#endif
			memcpy(p_buf, hci_pointer, copyLen);
			metabuffer->read_p += copyLen;
			break;
		} else {
			tailLen = META_BUFFER_SIZE - metabuffer->read_p;
			if (tailLen > count) {	/* exclude equal case to skip wrap check */
			hci_pointer = metabuffer->buffer + metabuffer->read_p;
#if STP_SUPPORT
			hci_pointer += 4;
#endif
			switch (hci_pointer[0]) {
			case 0x02:
				copyLen = (hci_pointer[3] + ((hci_pointer[4] << 8) & 0xff00))+5;
				break;
			case 0x04:
				copyLen = hci_pointer[2]+3;
				break;
			default:
				BTIF_WARN("%s: Unsupport now !!", __func__);
			}
#if STP_SUPPORT
			copyLen += 6;
#endif
			if (copyLen > btmtk_get_buffer_size(metabuffer)) {
				BTIF_ERR("%s: data not ready.", __func__);
				copyLen = -EAGAIN;
				btmtk_btif_unlock_unsleepable_lock(&(metabuffer->spin_lock));
				return copyLen;
			}
#if STP_SUPPORT
			hci_pointer -= 4;
#endif
			memcpy(p_buf, hci_pointer, copyLen);
			metabuffer->read_p += copyLen;
			break;
			} else {
				/* part 1: copy tailLen */
				memcpy(p_buf, metabuffer->buffer + metabuffer->read_p,
						tailLen);

				buffer += tailLen;	/* update buffer offset */

				/* part 2: check if head length is enough */
				copyLen = count - tailLen;

				/* if write_p < copyLen: means we can copy all data until write_p; */
				/* else: we can only copy data for copyLen */
				copyLen = (metabuffer->write_p < copyLen) ?
					metabuffer->write_p : copyLen;

				/* if copylen not 0, copy data to buffer */
				if (copyLen)
					memcpy(buffer, metabuffer->buffer + 0, copyLen);

				hci_pointer = p_buf;
#if STP_SUPPORT
				hci_pointer += 4;
#endif
				switch (hci_pointer[0]) {
				case 0x02:
					copyLen = (hci_pointer[3] + ((hci_pointer[4] << 8) & 0xff00))+5;
					break;
				case 0x04:
					copyLen = hci_pointer[2]+3;
					break;
				default:
					BTIF_WARN("%s: Unsupport now !!", __func__);
				}
				/* Update read_p final position */
#if STP_SUPPORT
				copyLen += 6;
#endif
				if (copyLen > btmtk_get_buffer_size(metabuffer)) {
					BTIF_ERR("%s: data not ready.", __func__);
					copyLen = -EAGAIN;
					btmtk_btif_unlock_unsleepable_lock(&(metabuffer->spin_lock));
					return copyLen;
				}
				metabuffer->read_p = (metabuffer->read_p + copyLen) % META_BUFFER_SIZE;
				/* update return length: head + tail */
#if STP_SUPPORT
				hci_pointer -= 4;
#endif
				memmove(p_buf, hci_pointer, copyLen);
			}
			break;
		}
	}

	btmtk_btif_unlock_unsleepable_lock(&(metabuffer->spin_lock));

	return copyLen;
}

/* writer buffer to metabuffer */
static void btmtk_write_buffer(struct ring_buffer_struct *metabuffer, u8 *buf, u32 length)
{
	u32 roomLeft, last_len;
	btmtk_btif_lock_unsleepable_lock(&(metabuffer->spin_lock));
	/* roomleft means the usable space */
	if (metabuffer->read_p <=
		metabuffer->write_p)
		roomLeft =
			META_BUFFER_SIZE -
			metabuffer->write_p +
			metabuffer->read_p - 1;
	else
		roomLeft =
			metabuffer->read_p -
			metabuffer->write_p - 1;

	/* no enough space to store the received data */
	if (roomLeft < length)
		BTIF_WARN("%s: Queue is full !!", __func__);

	if (length + metabuffer->write_p <
		META_BUFFER_SIZE) {

		/* copy event data */
		memcpy(metabuffer->buffer + metabuffer->write_p,
				buf, length);
		metabuffer->write_p += length;
	} else {
		last_len =
			META_BUFFER_SIZE -
			metabuffer->write_p;
		if (last_len != 0) {
			/* copy event data */
			memcpy(metabuffer->buffer + metabuffer->write_p,
					buf, last_len);
			memcpy(metabuffer->buffer, buf + last_len,
					length - last_len);
			metabuffer->write_p =
				length -
				last_len;
		} else {
			metabuffer->write_p = 0;
			/* copy event data */
			memcpy(metabuffer->buffer + metabuffer->write_p,
					buf, length);
			metabuffer->write_p += length;
		}
	}
	btmtk_btif_unlock_unsleepable_lock(&(metabuffer->spin_lock));
}


static ssize_t btmtk_btif_fops_read(struct file *file, char __user *buf, size_t count, loff_t *f_pos)
{
	int retval = 0;
	int copyLen = 0;
	int state = BTMTK_BTIF_STATE_UNKNOWN;
	int fstate = BTMTK_FOPS_STATE_UNKNOWN;
	u8 *buffer = NULL;

	FOPS_MUTEX_LOCK();
	fstate = btmtk_fops_get_state();
	if (fstate != BTMTK_FOPS_STATE_OPENED) {
		BTIF_WARN("%s: fops is not open yet(%d)!", __func__, fstate);
		FOPS_MUTEX_UNLOCK();
		return -ENODEV;
	}
	FOPS_MUTEX_UNLOCK();

	if (g_data == NULL) {
		BTIF_ERR("%s: ERROR, g_data is NULL!", __func__);
		return -ENODEV;
	}

	down(&g_data->rd_mtx);

	if (count > BUFFER_SIZE) {
		count = BUFFER_SIZE;
		BTIF_WARN("read size is bigger than 1024");
	}

	mutex_lock(&btmtk_btif_state_mutex);
	state = btmtk_btif_get_state();
	if (state == BTMTK_BTIF_STATE_FW_DUMP || state == BTMTK_BTIF_STATE_RESUME_FW_DUMP) {
		/* do nothing */
	} else if (need_reset_stack == 1 && register_late_resume_func != NULL) {
		BTIF_WARN("%s: Reset BT stack", __func__);
		mutex_unlock(&btmtk_btif_state_mutex);

		/* notify vendor library by signal */
		kill_fasync(&fasync, SIGIO, POLL_IN);

		need_reset_stack = 0;
		up(&g_data->rd_mtx);
		return -ENODEV;
	} else if (state != BTMTK_BTIF_STATE_WORKING) {
		BTIF_ERR("%s: current is in suspend/resume (%d).", __func__, state);
		mutex_unlock(&btmtk_btif_state_mutex);
		return -EAGAIN;
	}
	mutex_unlock(&btmtk_btif_state_mutex);

	btmtk_btif_lock_unsleepable_lock(&(g_data->metabuffer->spin_lock));

	/* means the buffer is empty */
	while (g_data->metabuffer->read_p == g_data->metabuffer->write_p) {

		/* unlock the buffer to let other process write data to buffer */
		btmtk_btif_unlock_unsleepable_lock(&(g_data->metabuffer->spin_lock));

		/* If nonblocking mode, return directly O_NONBLOCK is specified during open() */
		if (file->f_flags & O_NONBLOCK) {
			retval = -EAGAIN;
			goto OUT;
		}
		wait_event(BT_wq, g_data->metabuffer->read_p != g_data->metabuffer->write_p);
		btmtk_btif_lock_unsleepable_lock(&(g_data->metabuffer->spin_lock));
	}
	btmtk_btif_unlock_unsleepable_lock(&(g_data->metabuffer->spin_lock));
	/*read buffer from metabuffer to i_buff*/
	copyLen = btmtk_read_buffer(g_data->metabuffer, g_data->i_buf, (int) count);
	if (copyLen <= 0) {
		goto OUT;
	}
	buffer = (u8 *)g_data->i_buf;
#if STP_SUPPORT
	buffer += 4;
	copyLen -= 6;
#endif
	BTIF_DBG_RAW(buffer, 16, "%s:  (len=%2d)", __func__, copyLen);
	if (copy_to_user(buf, buffer, copyLen)) {
		BTIF_ERR("copy to user fail");
		copyLen = -EFAULT;
		goto OUT;
	}
	mutex_lock(&btmtk_btif_state_mutex);
	if (!raw_mode) {
		if (sleep_timer_enable)
			mod_timer(&sleep_timer, jiffies + SLEEP_TIME);
		else {
			sleep_timer.expires = jiffies + SLEEP_TIME;
			add_timer(&sleep_timer);
			sleep_timer_enable = true;
		}
	}
	mutex_unlock(&btmtk_btif_state_mutex);
OUT:
	up(&g_data->rd_mtx);
	return copyLen;
}

#define BTIF_OWNER_NAME "CONSYS_STP"

static int btmtk_btif_fops_open(struct inode *inode, struct file *file)
{
	int state = BTMTK_BTIF_STATE_UNKNOWN;
	int fstate = BTMTK_FOPS_STATE_UNKNOWN;
	int iRet;

	if (g_data == NULL) {
		BTIF_ERR("%s: ERROR, g_data is NULL!", __func__);
		return -ENODEV;
	}

	mutex_lock(&btmtk_btif_state_mutex);
	state = btmtk_btif_get_state();
	if (state == BTMTK_BTIF_STATE_INIT || state == BTMTK_BTIF_STATE_DISCONNECT) {
		mutex_unlock(&btmtk_btif_state_mutex);
		return -EAGAIN;
	}
	mutex_unlock(&btmtk_btif_state_mutex);

	FOPS_MUTEX_LOCK();
	fstate = btmtk_fops_get_state();
	if (fstate == BTMTK_FOPS_STATE_OPENED) {
		BTIF_WARN("%s: fops opened!", __func__);
		FOPS_MUTEX_UNLOCK();
		return 0;
	}

	if (fstate == BTMTK_FOPS_STATE_CLOSING) {
		BTIF_WARN("%s: fops close is on-going !", __func__);
		FOPS_MUTEX_UNLOCK();
		return -ENODEV;
	}
	FOPS_MUTEX_UNLOCK();

	BTIF_INFO("%s: Mediatek Bluetooth BTIF driver ver %s", __func__, VERSION);

	BTIF_INFO("%s: major %d minor %d (pid %d), probe counter: %d",
		__func__, imajor(inode), iminor(inode), current->pid,
		probe_counter);

	if (current->pid == 1) {
		BTIF_WARN("%s: return 0", __func__);
		return 0;
	}

	mutex_lock(&btmtk_btif_state_mutex);
	state = btmtk_btif_get_state();
	if (state != BTMTK_BTIF_STATE_WORKING) {
		BTIF_WARN("%s: not in working state(%d).", __func__,
			state);
		mutex_unlock(&btmtk_btif_state_mutex);
		return -ENODEV;
	}
	mutex_unlock(&btmtk_btif_state_mutex);

	/* init meta buffer */
	spin_lock_init(&(g_data->metabuffer->spin_lock.lock));
	spin_lock_init(&(g_data->fwlog_metabuffer->spin_lock.lock));

	sema_init(&g_data->wr_mtx, 1);
	sema_init(&g_data->rd_mtx, 1);

	/* init wait queue */
	init_waitqueue_head(&(inq));

	/* Init Hci Snoop */
	btmtk_btif_hci_snoop_init();

	btmtk_btif_lock_unsleepable_lock(&(g_data->metabuffer->spin_lock));
	g_data->metabuffer->read_p = 0;
	g_data->metabuffer->write_p = 0;
	btmtk_btif_unlock_unsleepable_lock(&(g_data->metabuffer->spin_lock));

	btmtk_btif_lock_unsleepable_lock(&(g_data->fwlog_metabuffer->spin_lock));
	g_data->fwlog_metabuffer->read_p = 0;
	g_data->fwlog_metabuffer->write_p = 0;
	btmtk_btif_unlock_unsleepable_lock(&(g_data->fwlog_metabuffer->spin_lock));

	iRet = mtk_wcn_btif_open(BTIF_OWNER_NAME, &g_data->stp_btif_id);
	if (iRet) {
		BTIF_ERR("mtk_wcn_stp_open_btif fail(%d)\n", iRet);
		return -ENODEV;
	}

	/*register stp rx call back to btif*/
	iRet = mtk_wcn_btif_rx_cb_register(g_data->stp_btif_id, (MTK_WCN_BTIF_RX_CB)btmtk_bitf_rx_cb);
	if (iRet) {
		BTIF_ERR("mtk_wcn_stp_rxcb_register fail(%d)\n", iRet);
		return -ENODEV;
	}
	/*Loop back test
	*mtk_wcn_btif_loopback_ctrl(g_data->stp_btif_id, BTIF_LPBK_ENABLE);
	*/

	msleep(20);
	btmtk_btif_send_wmt_power_on_cmd_7622();

	FOPS_MUTEX_LOCK();
	btmtk_fops_set_state(BTMTK_FOPS_STATE_OPENED);
	FOPS_MUTEX_UNLOCK();
	BTIF_INFO("%s: OK", __func__);

	return 0;
}

static int btmtk_btif_fops_close(struct inode *inode, struct file *file)
{
	int state = BTMTK_BTIF_STATE_UNKNOWN;
	int fstate = BTMTK_FOPS_STATE_UNKNOWN;

	if (g_data == NULL) {
		BTIF_ERR("%s: ERROR, g_data is NULL!", __func__);
		return -ENODEV;
	}

	FOPS_MUTEX_LOCK();
	fstate = btmtk_fops_get_state();
	if (fstate != BTMTK_FOPS_STATE_OPENED) {
		BTIF_WARN("%s: fops is not allow close(%d)", __func__, fstate);
		FOPS_MUTEX_UNLOCK();
		return 0;
	}
	btmtk_fops_set_state(BTMTK_FOPS_STATE_CLOSING);
	FOPS_MUTEX_UNLOCK();
	BTIF_INFO("%s: major %d minor %d (pid %d), probe:%d", __func__,
		imajor(inode), iminor(inode), current->pid, probe_counter);

	btmtk_btif_fops_fasync(-1, file, 0);

	mutex_lock(&btmtk_btif_state_mutex);
	state = btmtk_btif_get_state();
	if (state != BTMTK_BTIF_STATE_WORKING) {
		BTIF_WARN("%s: not in working state(%d).", __func__, state);
		mutex_unlock(&btmtk_btif_state_mutex);
		FOPS_MUTEX_LOCK();
		btmtk_fops_set_state(BTMTK_FOPS_STATE_CLOSED);
		FOPS_MUTEX_UNLOCK();
		return 0;
	}
	if (enter_sleep)
		btmtk_wakeup_consys();
	mutex_unlock(&btmtk_btif_state_mutex);

	btmtk_btif_send_wmt_power_off_cmd_7622();
	msleep(20);
	mtk_wcn_btif_close(g_data->stp_btif_id);

	btmtk_btif_lock_unsleepable_lock(&(g_data->metabuffer->spin_lock));
	g_data->metabuffer->read_p = 0;
	g_data->metabuffer->write_p = 0;
	btmtk_btif_unlock_unsleepable_lock(&(g_data->metabuffer->spin_lock));

	FOPS_MUTEX_LOCK();
	btmtk_fops_set_state(BTMTK_FOPS_STATE_CLOSED);
	FOPS_MUTEX_UNLOCK();

	BTIF_INFO("%s: OK", __func__);
	return 0;
}

static long btmtk_btif_fops_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	long retval = 0;

	if (g_data == NULL) {
		BTIF_ERR("%s: ERROR, g_data is NULL!", __func__);
		return -ENODEV;
	}

	switch (cmd) {
	case IOCTL_FW_ASSERT:
		/* BT trigger fw assert for debug */
		BTIF_INFO("BT Set fw assert..., reason:%lu", arg);
		break;
	default:
		retval = -EFAULT;
		BTIF_WARN("BT_ioctl(): unknown cmd (%d)", cmd);
		break;
	}

	return retval;
}

static int btmtk_btif_fops_fasync(int fd, struct file *file, int on)
{
	BTIF_INFO("%s: fd = 0x%X, flag = 0x%X", __func__, fd, on);
	return fasync_helper(fd, file, on, &fasync);
}

static unsigned int btmtk_btif_fops_poll(struct file *file, poll_table *wait)
{
	unsigned int mask = 0;
	int state = BTMTK_BTIF_STATE_UNKNOWN;

	if (g_data == NULL) {
		BTIF_ERR("%s: ERROR, g_data is NULL!", __func__);
		return -ENODEV;
	}

	if (g_data->metabuffer->read_p == g_data->metabuffer->write_p) {
		poll_wait(file, &inq, wait);
		/* empty let select sleep */
		if ((g_data->metabuffer->read_p != g_data->metabuffer->write_p)
			|| need_reset_stack == 1) {
			mask |= POLLIN | POLLRDNORM;		/* readable */
		}
	} else {
		mask |= POLLIN | POLLRDNORM;			/* readable */
	}
	/* do we need condition? */
	mask |= POLLOUT | POLLWRNORM;				/* writable */

	mutex_lock(&btmtk_btif_state_mutex);
	state = btmtk_btif_get_state();
	if (state == BTMTK_BTIF_STATE_FW_DUMP || state == BTMTK_BTIF_STATE_RESUME_FW_DUMP)
		mask |= POLLIN | POLLRDNORM;			/* readable */
	else if (state != BTMTK_BTIF_STATE_WORKING)	/* BTMTK_BTIF_STATE_WORKING: do nothing */
		mask = 0;
	mutex_unlock(&btmtk_btif_state_mutex);
	return mask;
}
/*fops fwlog*/
static ssize_t btmtk_btif_fops_readfwlog(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	int copyLen = 0;
	ulong flags = 0;
	unsigned char *i_buf;
	struct sk_buff *skb = NULL;

	if (g_data == NULL)
		return -ENODEV;

	if (g_data->fwlog_queue_count <= 0)
		return 0;

	spin_lock_irqsave(&g_data->fwlog_lock, flags);
	skb = skb_dequeue(&g_data->fwlog_queue);
	g_data->fwlog_queue_count--;
	spin_unlock_irqrestore(&g_data->fwlog_lock, flags);
	if (skb == NULL)
		return 0;
	i_buf = skb->data;
	copyLen = skb->len;
	/* not copy 02 or 04 in picus header as 04 FF F1 50 or 02 6F FC */
	memcpy(buf, i_buf + 1, copyLen - 1);
	kfree_skb(skb);

	return copyLen -1;
}

static int btmtk_btif_fops_openfwlog(struct inode *inode, struct file *file)
{
	if (g_data == NULL) {
		BTIF_ERR("%s: ERROR, g_data is NULL!", __func__);
		return -ENODEV;
	}

	BTIF_INFO("%s: OK", __func__);
	return 0;
}

static int btmtk_btif_fops_closefwlog(struct inode *inode, struct file *file)
{
	if (g_data == NULL) {
		BTIF_ERR("%s: ERROR, g_data is NULL!", __func__);
		return -ENODEV;
	}

	BTIF_INFO("%s: OK", __func__);
	return 0;
}

static long btmtk_btif_fops_unlocked_ioctlfwlog(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int retval = 0;

	BTIF_ERR("%s: ->", __func__);
	if (g_data == NULL) {
		BTIF_ERR("%s: ERROR, g_data is NULL!", __func__);
		return -ENODEV;
	}

	return retval;
}

static unsigned int btmtk_btif_fops_pollfwlog(struct file *file, poll_table *wait)
{
	unsigned int mask = 0;

	if (g_data == NULL) {
		BTIF_ERR("%s: ERROR, g_data is NULL!", __func__);
		return -ENODEV;
	}

	if (skb_queue_len(&g_data->fwlog_queue) > 0) {
		mask |= POLLIN | POLLRDNORM;			/* readable */
	} else {
		poll_wait(file, &fw_log_inq, wait);
		if (skb_queue_len(&g_data->fwlog_queue) > 0) {
			mask |= POLLIN | POLLRDNORM;		/* readable */
		}
	}
	return mask;
}

static int btmtk_fops_raw_open(struct inode *inode, struct file *file)
{
	int state = BTMTK_BTIF_STATE_UNKNOWN;
	int fstate = BTMTK_FOPS_STATE_UNKNOWN;
	int iRet;

	if (g_data == NULL) {
		BTIF_ERR("%s: ERROR, g_data is NULL!", __func__);
		return -ENODEV;
	}

	FOPS_MUTEX_LOCK();
	fstate = btmtk_fops_get_state();
	if (fstate == BTMTK_FOPS_STATE_OPENED) {
		BTIF_WARN("%s: fops opened!", __func__);
		FOPS_MUTEX_UNLOCK();
		return 0;
	}

	if (fstate == BTMTK_FOPS_STATE_CLOSING) {
		BTIF_WARN("%s: fops close is on-going !", __func__);
		FOPS_MUTEX_UNLOCK();
		return -ENODEV;
	}
	FOPS_MUTEX_UNLOCK();

	BTIF_INFO("%s: Mediatek Bluetooth driver ver %s", __func__, VERSION);

	BTIF_INFO("%s: major %d minor %d (pid %d), probe counter: %d",
		__func__, imajor(inode), iminor(inode), current->pid,
		probe_counter);

	if (current->pid == 1) {
		BTIF_WARN("%s: return 0", __func__);
		return 0;
	}

	mutex_lock(&btmtk_btif_state_mutex);
	state = btmtk_btif_get_state();
	if (state != BTMTK_BTIF_STATE_WORKING) {
		BTIF_WARN("%s: not in working state(%d).", __func__,
			state);
		mutex_unlock(&btmtk_btif_state_mutex);
		return -ENODEV;
	}
	mutex_unlock(&btmtk_btif_state_mutex);

	/* init meta buffer */
	spin_lock_init(&(g_data->metabuffer->spin_lock.lock));
	spin_lock_init(&(g_data->fwlog_metabuffer->spin_lock.lock));

	sema_init(&g_data->wr_mtx, 1);
	sema_init(&g_data->rd_mtx, 1);

	/* init wait queue */
	init_waitqueue_head(&(inq));

	/* Init Hci Snoop */
	btmtk_btif_hci_snoop_init();

	btmtk_btif_lock_unsleepable_lock(&(g_data->metabuffer->spin_lock));
	g_data->metabuffer->read_p = 0;
	g_data->metabuffer->write_p = 0;
	btmtk_btif_unlock_unsleepable_lock(&(g_data->metabuffer->spin_lock));

	btmtk_btif_lock_unsleepable_lock(&(g_data->fwlog_metabuffer->spin_lock));
	g_data->fwlog_metabuffer->read_p = 0;
	g_data->fwlog_metabuffer->write_p = 0;
	btmtk_btif_unlock_unsleepable_lock(&(g_data->fwlog_metabuffer->spin_lock));

	BTIF_INFO("enable interrupt and bulk in urb");

	iRet = mtk_wcn_btif_open(BTIF_OWNER_NAME, &g_data->stp_btif_id);
	if (iRet) {
		BTIF_ERR("mtk_wcn_stp_open_btif fail(%d)\n", iRet);
		return -ENODEV;
	}

	/*register stp rx call back to btif*/
	iRet = mtk_wcn_btif_rx_cb_register(g_data->stp_btif_id, (MTK_WCN_BTIF_RX_CB)btmtk_bitf_rx_cb);
	if (iRet) {
		BTIF_ERR("mtk_wcn_stp_rxcb_register fail(%d)\n", iRet);
		return -ENODEV;
	}
	/*Loop back test
	*mtk_wcn_btif_loopback_ctrl(g_data->stp_btif_id, BTIF_LPBK_ENABLE);
	*/

	FOPS_MUTEX_LOCK();
	btmtk_fops_set_state(BTMTK_FOPS_STATE_OPENED);
	FOPS_MUTEX_UNLOCK();
	raw_mode = true;
	BTIF_INFO("%s: OK", __func__);

	return 0;
}

static int btmtk_fops_raw_close(struct inode *inode, struct file *file)
{
	int state = BTMTK_BTIF_STATE_UNKNOWN;
	int fstate = BTMTK_FOPS_STATE_UNKNOWN;

	if (g_data == NULL) {
		BTIF_ERR("%s: ERROR, g_data is NULL!", __func__);
		return -ENODEV;
	}

	FOPS_MUTEX_LOCK();
	fstate = btmtk_fops_get_state();
	if (fstate != BTMTK_FOPS_STATE_OPENED) {
		BTIF_WARN("%s: fops is not allow close(%d)", __func__, fstate);
		FOPS_MUTEX_UNLOCK();
		return 0;
	}
	btmtk_fops_set_state(BTMTK_FOPS_STATE_CLOSING);
	FOPS_MUTEX_UNLOCK();
	BTIF_INFO("%s: major %d minor %d (pid %d), probe:%d", __func__,
		imajor(inode), iminor(inode), current->pid, probe_counter);

	btmtk_btif_fops_fasync(-1, file, 0);

	mutex_lock(&btmtk_btif_state_mutex);
	state = btmtk_btif_get_state();
	if (state != BTMTK_BTIF_STATE_WORKING) {
		BTIF_WARN("%s: not in working state(%d).", __func__, state);
		mutex_unlock(&btmtk_btif_state_mutex);
		FOPS_MUTEX_LOCK();
		btmtk_fops_set_state(BTMTK_FOPS_STATE_CLOSED);
		FOPS_MUTEX_UNLOCK();
		return 0;
	}
	mutex_unlock(&btmtk_btif_state_mutex);

	mtk_wcn_btif_close(g_data->stp_btif_id);

	btmtk_btif_lock_unsleepable_lock(&(g_data->metabuffer->spin_lock));
	g_data->metabuffer->read_p = 0;
	g_data->metabuffer->write_p = 0;
	btmtk_btif_unlock_unsleepable_lock(&(g_data->metabuffer->spin_lock));

	FOPS_MUTEX_LOCK();
	btmtk_fops_set_state(BTMTK_FOPS_STATE_CLOSED);
	FOPS_MUTEX_UNLOCK();
	raw_mode = false;

	BTIF_INFO("%s: OK", __func__);
	return 0;
}


/*============================================================================*/
/* Interface Functions : Kernel */
/*============================================================================*/
#define ________________________________________Interface_Function_for_Kernel
static int __init btmtk_btif_init(void)
{
	int ret;

	BTIF_INFO("%s: btmtk btif driver ver %s", __func__, VERSION);

	btmtk_btif_BT_init();

	ret = platform_device_register(&mtk_device_wmt);
	BTIF_INFO("mtk_device_wmt register ret %d", ret);
	if (ret != 0)
		BTIF_ERR("WMT platform device registered failed(%d)\n", ret);

	ret = platform_driver_register(&mtk_wmt_dev_drv);

	if (ret)
		BTIF_ERR("WMT platform driver registered failed(%d)\n", ret);

	if (btmtk_get_irq()) {
		BTIF_DBG("Get BGF_INT success");
		btmtk_irq_reg();
	} else
		BTIF_ERR("Get BGF_INT failed(%d)\n", ret);

	spin_lock_init(&g_data->fwlog_lock);
	skb_queue_head_init(&g_data->fwlog_queue);

	init_timer(&sleep_timer);
	sleep_timer.function = &btmtk_sleep_handler;
	sleep_timer.data = ((unsigned long) 0);
	sleep_timer_enable = false;
	enter_sleep = false;
	try_wake = false;
	fw_dump_enable = false;
	sleep_tsk = kthread_run(btmtk_sleep_thread, NULL, "btmtk_sleep_thread");
	if (IS_ERR(sleep_tsk))
		BTIF_ERR("%s: create sleep_tsk failed!", __func__);
	else
		BTIF_INFO("%s: create sleep_tsk ok!", __func__);
	wakeup_tsk = kthread_run(btmtk_wakeup_thread, NULL, "btmtk_wakeup_thread");
	if (IS_ERR(wakeup_tsk))
		BTIF_ERR("%s: create wakeup_tsk failed!", __func__);
	else
		BTIF_INFO("%s: create wakeup_tsk ok!", __func__);
	raw_mode = false;
	return ret;
}

static void __exit btmtk_btif_exit(void)
{
	BTIF_INFO("%s: btmtk btif driver ver %s", __func__, VERSION);

	btmtk_btif_BT_exit();
	platform_driver_unregister(&mtk_wmt_dev_drv);
	if (sleep_timer_enable)
		del_timer(&sleep_timer);
	kthread_stop(sleep_tsk);
	kthread_stop(wakeup_tsk);
}

static int btmtk_btif_suspend(struct platform_device *pdev, pm_message_t state)
{
	BTIF_INFO("%s: begin", __func__);

	mutex_lock(&btmtk_btif_state_mutex);
	btmtk_btif_set_state(BTMTK_BTIF_STATE_SUSPEND);
	mutex_unlock(&btmtk_btif_state_mutex);

	if ((g_data->suspend_count++)) {
		BTIF_WARN("%s: Has suspended. suspend_count: %d", __func__, g_data->suspend_count);
		BTIF_INFO("%s: end", __func__);
		return 0;
	}
	mutex_lock(&btmtk_btif_state_mutex);
	if (sleep_timer_enable)
		del_timer(&sleep_timer);
	sleep_timer_enable = false;
	mutex_unlock(&btmtk_btif_state_mutex);

	BTIF_INFO("%s: end", __func__);
	return 0;
}

static int btmtk_btif_resume(struct platform_device *pdev)
{
	g_data->suspend_count--;
	if (g_data->suspend_count) {
		BTIF_WARN("%s: data->suspend_count %d, return 0", __func__, g_data->suspend_count);
		return 0;
	}

	BTIF_INFO("%s: begin", __func__);

	mutex_lock(&btmtk_btif_state_mutex);
	btmtk_btif_set_state(BTMTK_BTIF_STATE_RESUME);
	mutex_unlock(&btmtk_btif_state_mutex);

	BTIF_INFO("%s: end", __func__);
	return 0;
}

/*============================================================================*/
/* Interface Functions : IRQ */
/*============================================================================*/
#define ________________________________________Interface_Function_for_IRQ
static unsigned int g_irq_flags;
static unsigned int g_irq_id = -1;
static bool btmtk_get_irq(void)
{
	struct device_node *node = NULL;

	node = of_find_compatible_node(NULL, NULL, "mediatek,btif");
	if (node)
		g_irq_id = irq_of_parse_and_map(node, 1);
	else {
		BTIF_ERR("get btif device node fail\n");
		return false;
	}

	if (g_irq_id < 0) {
		BTIF_ERR("get btif irq_id fail\n");
		return false;
	}
	/* get the interrupt line behaviour */
	g_irq_flags = irq_get_trigger_type(g_irq_id);
	if ((g_irq_id & IRQ_TYPE_SENSE_MASK) < 0) {
		BTIF_ERR("get btif irq_flags fail\n");
		g_irq_id = -1;
		return false;
	}

	BTIF_ERR("%s: irq_id:%d irq_flags:%d", __func__, g_irq_id, g_irq_flags);
	return true;
}

static int btmtk_irq_reg(void)
{
	int i_ret = -1;
	unsigned int irq_id;
	unsigned int flag;

	if (g_irq_id == -1) {
		BTIF_ERR("IRQ is not supported\n");
		return 0;
	}

	irq_id = g_irq_id;
	flag = g_irq_flags;
	i_ret = request_irq(irq_id,
			    (irq_handler_t) btmtk_irq_handler,
			    flag, "btmtk_bgf_eint", (void *)g_data);
	if (i_ret)
		return i_ret;
	return 0;
}

static int btmtk_wakeup_consys(void)
{
	unsigned char dongle_state = g_data->event_state;
#if 0
	/* WMT sleep*/
	u8 cmd[] = { 0x01, 0x6F, 0xFC, 0x05, 0x01, 0x03, 0x01, 0x00, 0x01 };
	u8 event[] = { 0x04, 0xE4, 0x06, 0x02, 0x03, 0x02, 0x00, 0x00, 0x01 };
#endif
	/* WMT wakeup*/
	u8 event[] = { 0x04, 0xE4, 0x06, 0x02, 0x03, 0x02, 0x00, 0x00, 0x03 };
	int size = sizeof(event);
	int ret = -1;   /* if successful, 0 */
	int i;

	BTIF_INFO("%s: begin", __func__);
	g_data->event_state = BTMTK_EVENT_STATE_WAIT_EVENT;
	ret = mtk_wcn_btif_wakeup_consys(g_data->stp_btif_id);
	if (ret < 0) {
		BTIF_ERR("%s: failed(%d)", __func__, ret);
		g_data->event_state = BTMTK_EVENT_STATE_ERROR;
		return ret;
	}
	wait_event_timeout(p_wait_event_q, p_event_buffer != NULL, 2*HZ);
	if (p_event_buffer == NULL) {
		BTIF_ERR("%s: no event, timeout.", __func__);
		return ret;
	}
	for (i = 0; i < size; i++) {
#if STP_SUPPORT
		if (event[i] != p_event_buffer[i + 4])
#else
		if (event[i] != p_event_buffer[i])
#endif
		break;
	}
	if (i != size) {
		BTIF_ERR("%s: event not match. %X", __func__, p_event_buffer[size + 3]);
		ret = -1;
	}
	kfree(p_event_buffer);
	p_event_buffer = NULL;
	if (ret < 0) {
		BTIF_ERR("%s: failed(%d)", __func__, ret);
		g_data->event_state = BTMTK_EVENT_STATE_ERROR;
		return ret;
	}
	enter_sleep = false;
	BTIF_INFO("%s: OK", __func__);
	g_data->event_state = dongle_state;
	return 0;
}

static irqreturn_t btmtk_irq_handler(int irq, void *data)
{
	BTIF_INFO("%s: begin", __func__);
	if ((!raw_mode) && (enter_sleep == true)) {
		mutex_lock(&btmtk_btif_state_mutex);
		if (!try_wake)
			try_wake = true;
		wake_up(&p_sleep_q);
		disable_irq_nosync(g_irq_id);
		mutex_unlock(&btmtk_btif_state_mutex);
	}
	BTIF_INFO("%s: end", __func__);
	return IRQ_HANDLED;
}

static int btmtk_wakeup_thread(void *data)
{
	unsigned char dongle_state = g_data->event_state;
#if 0
	/* WMT sleep*/
	u8 cmd[] = { 0x01, 0x6F, 0xFC, 0x05, 0x01, 0x03, 0x01, 0x00, 0x01 };
	u8 event[] = { 0x04, 0xE4, 0x06, 0x02, 0x03, 0x02, 0x00, 0x00, 0x01 };
#endif
	/* WMT wakeup*/
	u8 cmd[] = { 0x01, 0x6F, 0xFC, 0x05, 0x01, 0x03, 0x01, 0x00, 0x02 };
	u8 event[] = { 0x04, 0xE4, 0x06, 0x02, 0x03, 0x02, 0x00, 0x00, 0x02 };
	int ret = -1;   /* if successful, 0 */

	while (!kthread_should_stop()) {
		wait_event(p_sleep_q, try_wake == true);
		mutex_lock(&btmtk_btif_state_mutex);
		BTIF_INFO("%s: begin", __func__);
		g_data->event_state = BTMTK_EVENT_STATE_WAIT_EVENT;
		ret = btmtk_btif_send_wmt_cmd(cmd, sizeof(cmd), event, sizeof(event), 20);
		if (ret < 0) {
			BTIF_ERR("%s: failed(%d)", __func__, ret);
			g_data->event_state = BTMTK_EVENT_STATE_ERROR;
			mutex_unlock(&btmtk_btif_state_mutex);
			return ret;
		}

		g_data->event_state = dongle_state;
		enter_sleep = false;
		try_wake = false;
		mutex_unlock(&btmtk_btif_state_mutex);
		enable_irq(g_irq_id);
		BTIF_INFO("%s: OK", __func__);
	}
	return ret;
}

static int btmtk_sleep_thread(void *data)
{
	unsigned char dongle_state = g_data->event_state;
	/* WMT sleep*/
	u8 cmd[] = { 0x01, 0x6F, 0xFC, 0x05, 0x01, 0x03, 0x01, 0x00, 0x01 };
	u8 event[] = { 0x04, 0xE4, 0x06, 0x02, 0x03, 0x02, 0x00, 0x00, 0x01 };
	int ret = -1;   /* if successful, 0 */

	while (!kthread_should_stop()) {
		wait_event(p_sleep_q, (enter_sleep == false) && (sleep_timer_enable == true));
		BTIF_INFO("%s: begin", __func__);
		mutex_lock(&btmtk_btif_state_mutex);
		g_data->event_state = BTMTK_EVENT_STATE_WAIT_EVENT;
		ret = btmtk_btif_send_wmt_cmd(cmd, sizeof(cmd), event, sizeof(event), 20);
		if (ret < 0) {
			BTIF_ERR("%s: failed(%d)", __func__, ret);
			g_data->event_state = BTMTK_EVENT_STATE_ERROR;
			return ret;
		}

		g_data->event_state = dongle_state;
		enter_sleep = true;
		mutex_unlock(&btmtk_btif_state_mutex);
		BTIF_INFO("%s: OK", __func__);
	}

	return ret;
}

static void btmtk_sleep_handler(unsigned long data)
{
	BTIF_INFO("%s: begin", __func__);
	FOPS_MUTEX_LOCK();
	if ((BTMTK_FOPS_STATE_OPENED == btmtk_fops_get_state()) &&
		(fw_dump_enable == false)) {
			wake_up(&p_sleep_q);
	}
	FOPS_MUTEX_UNLOCK();
	BTIF_INFO("%s: end", __func__);
}

module_init(btmtk_btif_init);
module_exit(btmtk_btif_exit);

/**
 * Module information
 */
MODULE_DESCRIPTION("Mediatek Bluetooth btif driver ver " VERSION);
MODULE_VERSION(VERSION);
MODULE_LICENSE("GPL");
