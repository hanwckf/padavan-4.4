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

#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/hardirq.h>
#include <linux/init.h>
#include <linux/kallsyms.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/poll.h>
#include <linux/proc_fs.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/vmalloc.h>
#ifdef CONFIG_MTK_FB
#include <disp_assert_layer.h>
#endif
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>
#include <linux/workqueue.h>
#include <linux/kthread.h>
#include <linux/stacktrace.h>
#include <linux/compat.h>
#include <mt-plat/aee.h>
#include <linux/seq_file.h>
#include <linux/completion.h>
#include "aed.h"

struct aee_req_queue {
	struct list_head list;
	spinlock_t lock;
};

static struct aee_req_queue ke_queue;
static struct work_struct ke_work;
static DECLARE_COMPLETION(aed_ke_com);

static struct aee_req_queue ee_queue;
static struct work_struct ee_work;
static DECLARE_COMPLETION(aed_ee_com);
/*
 *  may be accessed from irq
*/
static spinlock_t aed_device_lock;
int aee_mode = AEE_MODE_NOT_INIT;
static int force_red_screen = AEE_FORCE_NOT_SET;
static int aee_force_exp = AEE_FORCE_EXP_NOT_SET;

static struct proc_dir_entry *aed_proc_dir;

#define MaxStackSize 8100
#define MaxMapsSize 8100

/******************************************************************************
 * DEBUG UTILITIES
 *****************************************************************************/

void msg_show(const char *prefix, struct AE_Msg *msg)
{
	const char *cmd_type = NULL;
	const char *cmd_id = NULL;

	if (msg == NULL) {
		LOGD("%s: EMPTY msg\n", prefix);
		return;
	}

	switch (msg->cmdType) {
	case AE_REQ:
		cmd_type = "REQ";
		break;
	case AE_RSP:
		cmd_type = "RESPONSE";
		break;
	case AE_IND:
		cmd_type = "IND";
		break;
	default:
		cmd_type = "UNKNOWN";
		break;
	}

	switch (msg->cmdId) {
	case AE_REQ_IDX:
		cmd_id = "IDX";
		break;
	case AE_REQ_CLASS:
		cmd_id = "CLASS";
		break;
	case AE_REQ_TYPE:
		cmd_id = "TYPE";
		break;
	case AE_REQ_MODULE:
		cmd_id = "MODULE";
		break;
	case AE_REQ_PROCESS:
		cmd_id = "PROCESS";
		break;
	case AE_REQ_DETAIL:
		cmd_id = "DETAIL";
		break;
	case AE_REQ_BACKTRACE:
		cmd_id = "BACKTRACE";
		break;
	case AE_REQ_COREDUMP:
		cmd_id = "COREDUMP";
		break;
	case AE_IND_EXP_RAISED:
		cmd_id = "EXP_RAISED";
		break;
	case AE_IND_WRN_RAISED:
		cmd_id = "WARN_RAISED";
		break;
	case AE_IND_REM_RAISED:
		cmd_id = "REMIND_RAISED";
		break;
	case AE_IND_FATAL_RAISED:
		cmd_id = "FATAL_RAISED";
		break;
	case AE_IND_LOG_CLOSE:
		cmd_id = "CLOSE";
		break;
	case AE_REQ_USERSPACEBACKTRACE:
		cmd_id = "USERBACKTRACE";
		break;
	case AE_REQ_USER_REG:
		cmd_id = "USERREG";
		break;
	default:
		cmd_id = "UNKNOWN";
		break;
	}

	LOGD("%s: cmdType=%s[%d] cmdId=%s[%d] seq=%d arg=%x len=%d\n", prefix, cmd_type,
	     msg->cmdType, cmd_id, msg->cmdId, msg->seq, msg->arg, msg->len);
}


/******************************************************************************
 * CONSTANT DEFINITIONS
 *****************************************************************************/
#define CURRENT_KE_CONSOLE "current-ke-console"
#define CURRENT_EE_COREDUMP "current-ee-coredump"

#define CURRENT_KE_ANDROID_MAIN "current-ke-android_main"
#define CURRENT_KE_ANDROID_RADIO "current-ke-android_radio"
#define CURRENT_KE_ANDROID_SYSTEM "current-ke-android_system"
#define CURRENT_KE_USERSPACE_INFO "current-ke-userspace_info"

#define CURRENT_KE_MMPROFILE "current-ke-mmprofile"

#define MAX_EE_COREDUMP 0x800000

/******************************************************************************
 * STRUCTURE DEFINITIONS
 *****************************************************************************/

struct aed_eerec {		/* external exception record */
	struct list_head list;
	char assert_type[32];
	char exp_filename[512];
	unsigned int exp_linenum;
	unsigned int fatal1;
	unsigned int fatal2;

	int *ee_log;
	int ee_log_size;
	int *ee_phy;
	int ee_phy_size;
	char *msg;
	int db_opt;
};

struct aed_kerec {		/* TODO: kernel exception record */
	char *msg;
	struct aee_oops *lastlog;
};

struct aed_dev {
	struct aed_eerec *eerec;
	wait_queue_head_t eewait;

	struct aed_kerec kerec;
	wait_queue_head_t kewait;
};


/******************************************************************************
 * FUNCTION PROTOTYPES
 *****************************************************************************/
static long aed_ioctl(struct file *file, unsigned int cmd, unsigned long arg);


/******************************************************************************
 * GLOBAL DATA
 *****************************************************************************/
static struct aed_dev aed_dev;

/******************************************************************************
 * Message Utilities
 *****************************************************************************/

inline void msg_destroy(char **ppmsg)
{
	if (*ppmsg != NULL) {
		vfree(*ppmsg);
		*ppmsg = NULL;
	}
}

inline struct AE_Msg *msg_create(char **ppmsg, int extra_size)
{
	int size;

	msg_destroy(ppmsg);
	size = sizeof(struct AE_Msg) + extra_size;

	*ppmsg = vzalloc(size);
	if (*ppmsg == NULL) {
		LOGE("%s : kzalloc() fail\n", __func__);
		return NULL;
	}

	((struct AE_Msg *) (*ppmsg))->len = extra_size;

	return (struct AE_Msg *) *ppmsg;
}

static ssize_t msg_copy_to_user(const char *prefix, char *msg, char __user *buf,
				size_t count, loff_t *f_pos)
{
	ssize_t ret = 0;
	int len;
	char *msg_tmp = NULL;

	if (msg == NULL)
		return 0;

	msg_show(prefix, (struct AE_Msg *) msg);

	msg_tmp = kzalloc(((struct AE_Msg *)msg)->len + sizeof(struct AE_Msg), GFP_KERNEL);
	if (msg_tmp != NULL) {
		memcpy(msg_tmp, msg, ((struct AE_Msg *)msg)->len + sizeof(struct AE_Msg));
	} else {
		LOGE("%s : kzalloc() fail!\n", __func__);
		msg_tmp = msg;
	}

	if (msg_tmp == NULL || ((struct AE_Msg *)msg_tmp)->cmdType < AE_REQ
		|| ((struct AE_Msg *)msg_tmp)->cmdType > AE_CMD_TYPE_END)
		goto out;

	len = ((struct AE_Msg *) msg_tmp)->len + sizeof(struct AE_Msg);

	if (*f_pos >= len) {
		ret = 0;
		goto out;
	}
	/* TODO: semaphore */
	if ((*f_pos + count) > len) {
		LOGE("read size overflow, count=%zx, *f_pos=%llx\n", count, *f_pos);
		count = len - *f_pos;
		ret = -EFAULT;
		goto out;
	}

	if (copy_to_user(buf, msg_tmp + *f_pos, count)) {
		LOGE("copy_to_user failed\n");
		ret = -EFAULT;
		goto out;
	}
	*f_pos += count;
	ret = count;
 out:
	if (msg_tmp != msg)
		kfree(msg_tmp);
	return ret;
}

/******************************************************************************
 * Kernel message handlers
 *****************************************************************************/
static void ke_gen_notavail_msg(void)
{
	struct AE_Msg *rep_msg;

	LOGD("%s\n", __func__);

	rep_msg = msg_create(&aed_dev.kerec.msg, 0);
	if (rep_msg == NULL)
		return;

	rep_msg->cmdType = AE_RSP;
	rep_msg->arg = AE_NOT_AVAILABLE;
	rep_msg->len = 0;
}

static void ke_gen_class_msg(void)
{
#define KE_CLASS_STR "Kernel (KE)"
#define KE_CLASS_SIZE 12
	struct AE_Msg *rep_msg;
	char *data;

	LOGD("%s\n", __func__);

	rep_msg = msg_create(&aed_dev.kerec.msg, KE_CLASS_SIZE);
	if (rep_msg == NULL)
		return;

	data = (char *)rep_msg + sizeof(struct AE_Msg);
	rep_msg->cmdType = AE_RSP;
	rep_msg->cmdId = AE_REQ_CLASS;
	rep_msg->len = KE_CLASS_SIZE;
	strncpy(data, KE_CLASS_STR, KE_CLASS_SIZE);
}

static void ke_gen_type_msg(void)
{
#define KE_TYPE_STR "PANIC"
#define KE_TYPE_SIZE 6
	struct AE_Msg *rep_msg;
	char *data;

	LOGD("%s\n", __func__);

	rep_msg = msg_create(&aed_dev.kerec.msg, KE_TYPE_SIZE);
	if (rep_msg == NULL)
		return;

	data = (char *)rep_msg + sizeof(struct AE_Msg);
	rep_msg->cmdType = AE_RSP;
	rep_msg->cmdId = AE_REQ_TYPE;
	rep_msg->len = KE_TYPE_SIZE;
	strncpy(data, KE_TYPE_STR, KE_TYPE_SIZE);
}

static void ke_gen_module_msg(void)
{
	struct AE_Msg *rep_msg;
	char *data;

	LOGD("%s\n", __func__);
	rep_msg = msg_create(&aed_dev.kerec.msg, strlen(aed_dev.kerec.lastlog->module) + 1);
	if (rep_msg == NULL)
		return;

	data = (char *)rep_msg + sizeof(struct AE_Msg);
	rep_msg->cmdType = AE_RSP;
	rep_msg->cmdId = AE_REQ_MODULE;
	rep_msg->len = strlen(aed_dev.kerec.lastlog->module) + 1;
	strlcpy(data, aed_dev.kerec.lastlog->module, sizeof(aed_dev.kerec.lastlog->module));
}

static void ke_gen_detail_msg(const struct AE_Msg *req_msg)
{
	struct AE_Msg *rep_msg;
	char *data;

	LOGD("ke_gen_detail_msg is called\n");
	LOGD("%s req_msg arg:%d\n", __func__, req_msg->arg);

	rep_msg = msg_create(&aed_dev.kerec.msg, aed_dev.kerec.lastlog->detail_len + 1);
	if (rep_msg == NULL)
		return;

	data = (char *)rep_msg + sizeof(struct AE_Msg);
	rep_msg->cmdType = AE_RSP;
	rep_msg->cmdId = AE_REQ_DETAIL;
	rep_msg->len = aed_dev.kerec.lastlog->detail_len + 1;
	if (aed_dev.kerec.lastlog->detail != NULL)
		strlcpy(data, aed_dev.kerec.lastlog->detail, aed_dev.kerec.lastlog->detail_len);
	data[aed_dev.kerec.lastlog->detail_len] = 0;

	LOGD("ke_gen_detail_msg is return: %s\n", data);
}

static void ke_gen_process_msg(void)
{
	struct AE_Msg *rep_msg;
	char *data;

	LOGD("%s\n", __func__);
	rep_msg = msg_create(&aed_dev.kerec.msg, AEE_PROCESS_NAME_LENGTH);
	if (rep_msg == NULL)
		return;

	data = (char *)rep_msg + sizeof(struct AE_Msg);
	rep_msg->cmdType = AE_RSP;
	rep_msg->cmdId = AE_REQ_PROCESS;

	strncpy(data, aed_dev.kerec.lastlog->process_path, AEE_PROCESS_NAME_LENGTH);
	/* Count into the NUL byte at end of string */
	rep_msg->len = strlen(data) + 1;
}

static void ke_gen_backtrace_msg(void)
{
	struct AE_Msg *rep_msg;
	char *data;

	LOGD("%s\n", __func__);
	rep_msg = msg_create(&aed_dev.kerec.msg, AEE_BACKTRACE_LENGTH);
	if (rep_msg == NULL)
		return;

	data = (char *)rep_msg + sizeof(struct AE_Msg);
	rep_msg->cmdType = AE_RSP;
	rep_msg->cmdId = AE_REQ_BACKTRACE;

	strncpy(data, aed_dev.kerec.lastlog->backtrace, AEE_BACKTRACE_LENGTH);
	/* Count into the NUL byte at end of string */
	rep_msg->len = strlen(data) + 1;
}


static void ke_gen_userbacktrace_msg(void)
{
	struct AE_Msg *rep_msg;
	char *data;
	int userinfo_len = 0;

	userinfo_len = aed_dev.kerec.lastlog->userthread_stack.StackLength + sizeof(pid_t)+sizeof(int);
	rep_msg = msg_create(&aed_dev.kerec.msg, MaxStackSize);
	if (rep_msg == NULL)
		return;

	data = (char *)rep_msg + sizeof(struct AE_Msg);
	rep_msg->cmdType = AE_RSP;
	rep_msg->cmdId = AE_REQ_USERSPACEBACKTRACE;

	rep_msg->len = userinfo_len;
	LOGD("%s rep_msg->len:%lx,\n", __func__, (long)rep_msg->len);

	memcpy(data, (char *) &(aed_dev.kerec.lastlog->userthread_stack), sizeof(pid_t) + sizeof(int));
	LOGD("len(pid+int):%lx\n", (long)(sizeof(pid_t)+sizeof(int)));
	LOGD("des :%lx\n", (long)(data + sizeof(pid_t)+sizeof(int)));
	LOGD("src addr :%lx\n", (long)((char *)(aed_dev.kerec.lastlog->userthread_stack.Userthread_Stack)));

	memcpy((data + sizeof(pid_t)+sizeof(int)),
			(char *)(aed_dev.kerec.lastlog->userthread_stack.Userthread_Stack),
			aed_dev.kerec.lastlog->userthread_stack.StackLength);

	#if 0 /* for debug */
	{
		int i = 0;

		for (i = 0; i < 64; i++)
			LOGD("%x\n ", data[i]);

	}
	#endif
	LOGD("%s  +++\n", __func__);
}

static void ke_gen_usermaps_msg(void)
{
	struct AE_Msg *rep_msg;
	char *data;
	int userinfo_len = 0;

	userinfo_len = aed_dev.kerec.lastlog->userthread_maps.Userthread_mapsLength + sizeof(pid_t)+sizeof(int);
	rep_msg = msg_create(&aed_dev.kerec.msg, MaxMapsSize);
	if (rep_msg == NULL)
		return;

	data = (char *)rep_msg + sizeof(struct AE_Msg);
	rep_msg->cmdType = AE_RSP;
	rep_msg->cmdId = AE_REQ_USER_MAPS;

	rep_msg->len = userinfo_len;
	LOGD("%s rep_msg->len:%lx,\n", __func__, (long)rep_msg->len);

	memcpy(data, (char *) &(aed_dev.kerec.lastlog->userthread_maps), sizeof(pid_t) + sizeof(int));
	LOGD("len(pid+int):%lx\n", (long)(sizeof(pid_t)+sizeof(int)));
	LOGD("des :%lx\n", (long)(data + sizeof(pid_t)+sizeof(int)));
	LOGD("src addr :%lx\n", (long)((char *)(aed_dev.kerec.lastlog->userthread_maps.Userthread_maps)));

	memcpy((data + sizeof(pid_t)+sizeof(int)),
			(char *)(aed_dev.kerec.lastlog->userthread_maps.Userthread_maps),
			aed_dev.kerec.lastlog->userthread_maps.Userthread_mapsLength);

	LOGD("%s  +++\n", __func__);
}





static void ke_gen_user_reg_msg(void)
{
	struct AE_Msg *rep_msg;
	char *data;

	rep_msg = msg_create(&aed_dev.kerec.msg, sizeof(struct aee_thread_reg));
	if (rep_msg == NULL)
		return;

	data = (char *)rep_msg + sizeof(struct AE_Msg);
	rep_msg->cmdType = AE_RSP;
	rep_msg->cmdId = AE_REQ_USER_REG;

	/* Count into the NUL byte at end of string */
	rep_msg->len = sizeof(struct aee_thread_reg);
	memcpy(data, (char *) &(aed_dev.kerec.lastlog->userthread_reg), sizeof(struct aee_thread_reg));
	#if 0 /* for debug */
	#ifdef __aarch64__ /* 64bit kernel+32 u */
	if (is_compat_task()) {	/* K64_U32 */
		LOGE(" K64+ U32 pc/lr/sp 0x%16lx/0x%16lx/0x%16lx\n",
				(long)(aed_dev.kerec.lastlog->userthread_reg.regs.user_regs.pc),
				(long)(aed_dev.kerec.lastlog->userthread_reg.regs.regs[14]),
				(long)(aed_dev.kerec.lastlog->userthread_reg.regs.regs[13]));
	}
	#endif
	#endif
	LOGD("%s +++\n", __func__);
}

static int ke_gen_ind_msg(struct aee_oops *oops)
{
	unsigned long flags = 0;

	LOGD("%s oops %p\n", __func__, oops);
	if (oops == NULL)
		return -1;

	spin_lock_irqsave(&aed_device_lock, flags);
	if (aed_dev.kerec.lastlog == NULL) {
		aed_dev.kerec.lastlog = oops;
	} else {
		/*
		 *  waaa..   Two ke api at the same time
		 *  or ke api during aed process is still busy at ke
		 *  discard the new oops!
		 *  Code should NEVER come here now!!!
		 */

		LOGW("%s: BUG!!! More than one kernel message queued, AEE does not support concurrent KE dump\n",
				__func__);
		aee_oops_free(oops);
		spin_unlock_irqrestore(&aed_device_lock, flags);

		return -1;
	}
	spin_unlock_irqrestore(&aed_device_lock, flags);

	if (aed_dev.kerec.lastlog != NULL) {
		struct AE_Msg *rep_msg;

		rep_msg = msg_create(&aed_dev.kerec.msg, 0);
		if (rep_msg == NULL)
			return 0;

		rep_msg->cmdType = AE_IND;
		switch (oops->attr) {
		case AE_DEFECT_REMINDING:
			rep_msg->cmdId = AE_IND_REM_RAISED;
			break;
		case AE_DEFECT_WARNING:
			rep_msg->cmdId = AE_IND_WRN_RAISED;
			break;
		case AE_DEFECT_EXCEPTION:
			rep_msg->cmdId = AE_IND_EXP_RAISED;
			break;
		case AE_DEFECT_FATAL:
			rep_msg->cmdId = AE_IND_FATAL_RAISED;
			break;
		default:
			/* Huh... something wrong, just go to exception */
			rep_msg->cmdId = AE_IND_EXP_RAISED;
			break;
		}

		rep_msg->arg = oops->clazz;
		rep_msg->len = 0;
		rep_msg->dbOption = oops->dump_option;

		init_completion(&aed_ke_com);
		wake_up(&aed_dev.kewait);
		/*
		 * wait until current ke work is done, then aed_dev is available,
		 * add a 60s timeout in case of debuggerd quit abnormally
		 */
		if (wait_for_completion_timeout(&aed_ke_com, msecs_to_jiffies(5 * 60 * 1000)))
			LOGE("%s: TIMEOUT, not receive close event, skip\n", __func__);
	}
	return 0;
}

static void ke_destroy_log(void)
{
	struct aee_oops *lastlog = aed_dev.kerec.lastlog;

	LOGD("%s\n", __func__);
	msg_destroy(&aed_dev.kerec.msg);

	if (aed_dev.kerec.lastlog) {
		aed_dev.kerec.lastlog = NULL;
		if (strncmp(lastlog->module, IPANIC_MODULE_TAG, strlen(IPANIC_MODULE_TAG)) == 0)
			ipanic_oops_free(lastlog, 0);
		else
			aee_oops_free(lastlog);
	}
}

static int ke_log_avail(void)
{
	if (aed_dev.kerec.lastlog != NULL) {
#ifdef __aarch64__
		if (is_compat_task() != ((aed_dev.kerec.lastlog->dump_option & DB_OPT_AARCH64) == 0))
			return 0;
#endif
		/* remove the log to reduce risk of dead loop: cpux keep moving log from buffer to
		 * console and can not process debuggerd work flow, meanwhile aed keep calling poll
		 * which produce more log into buffer and cpux stucked whith these log.
		 * LOGI("AEE api log available\n");
		 */
		return 1;
	}

	return 0;
}

static void ke_queue_request(struct aee_oops *oops)
{
	unsigned long flags = 0;
	int ret;

	spin_lock_irqsave(&ke_queue.lock, flags);
	list_add_tail(&oops->list, &ke_queue.list);
	spin_unlock_irqrestore(&ke_queue.lock, flags);
	ret = queue_work(system_wq, &ke_work);
	LOGI("%s: add new ke work, status %d\n", __func__, ret);
}

static void ke_worker(struct work_struct *work)
{
	int ret = 0;
	struct aee_oops *oops, *n;
	unsigned long flags = 0;

	list_for_each_entry_safe(oops, n, &ke_queue.list, list) {
		if (oops == NULL) {
			LOGE("%s:Invalid aee_oops struct\n", __func__);
			return;
		}

		ret = ke_gen_ind_msg(oops);
		spin_lock_irqsave(&ke_queue.lock, flags);
		if (!ret)
			list_del(&oops->list);
		spin_unlock_irqrestore(&ke_queue.lock, flags);
		ke_destroy_log();
	}
}

/******************************************************************************
 * EE message handlers
 *****************************************************************************/
static void ee_gen_notavail_msg(void)
{
	struct AE_Msg *rep_msg;

	LOGD("%s\n", __func__);

	rep_msg = msg_create(&aed_dev.eerec->msg, 0);
	if (rep_msg == NULL)
		return;

	rep_msg->cmdType = AE_RSP;
	rep_msg->arg = AE_NOT_AVAILABLE;
	rep_msg->len = 0;
}

static void ee_gen_class_msg(void)
{
#define EX_CLASS_EE_STR "External (EE)"
#define EX_CLASS_EE_SIZE 14
	struct AE_Msg *rep_msg;
	char *data;

	LOGD("%s\n", __func__);

	rep_msg = msg_create(&aed_dev.eerec->msg, EX_CLASS_EE_SIZE);
	if (rep_msg == NULL)
		return;

	data = (char *)rep_msg + sizeof(struct AE_Msg);
	rep_msg->cmdType = AE_RSP;
	rep_msg->cmdId = AE_REQ_CLASS;
	rep_msg->len = EX_CLASS_EE_SIZE;
	strncpy(data, EX_CLASS_EE_STR, EX_CLASS_EE_SIZE);
}

static void ee_gen_type_msg(void)
{
	struct AE_Msg *rep_msg;
	char *data;
	struct aed_eerec *eerec = aed_dev.eerec;

	LOGD("%s\n", __func__);

	rep_msg =
	    msg_create(&eerec->msg, strlen((char const *)&eerec->assert_type) + 1);
	if (rep_msg == NULL)
		return;

	data = (char *)rep_msg + sizeof(struct AE_Msg);
	rep_msg->cmdType = AE_RSP;
	rep_msg->cmdId = AE_REQ_TYPE;
	rep_msg->len = strlen((char const *)&eerec->assert_type) + 1;
	strncpy(data, (char const *)&eerec->assert_type,
		strlen((char const *)&eerec->assert_type));
}

static void ee_gen_process_msg(void)
{
#define PROCESS_STRLEN 512

	int n = 0;
	struct AE_Msg *rep_msg;
	char *data;
	struct aed_eerec *eerec = aed_dev.eerec;

	LOGD("%s\n", __func__);

	rep_msg = msg_create(&eerec->msg, PROCESS_STRLEN);
	if (rep_msg == NULL)
		return;

	data = (char *)rep_msg + sizeof(struct AE_Msg);

	if (eerec->exp_linenum != 0) {
		/* for old aed_md_exception1() */
		n = snprintf(data, sizeof(eerec->assert_type), "%s", eerec->assert_type);
		if (eerec->exp_filename[0] != 0) {
			n += snprintf(data + n, (PROCESS_STRLEN - n), ", filename=%s,line=%d", eerec->exp_filename,
				     eerec->exp_linenum);
		} else if (eerec->fatal1 != 0 && eerec->fatal2 != 0) {
			n += snprintf(data + n, (PROCESS_STRLEN - n), ", err1=%d,err2=%d", eerec->fatal1,
				     eerec->fatal2);
		}
	} else {
		LOGD("ee_gen_process_msg else\n");
		n = snprintf(data, PROCESS_STRLEN, "%s", eerec->exp_filename);
	}

	rep_msg->cmdType = AE_RSP;
	rep_msg->cmdId = AE_REQ_PROCESS;
	rep_msg->len = n + 1;
}

__weak int aee_dump_ccci_debug_info(int md_id, void **addr, int *size)
{
	return -1;
}

static void ee_gen_detail_msg(void)
{
	int i, n = 0, l = 0;
	struct AE_Msg *rep_msg;
	char *data;
	int *mem;
	int md_id;
	int msgsize;
	char *ccci_log = NULL;
	int ccci_log_size = 0;
	struct aed_eerec *eerec = aed_dev.eerec;

	LOGD("%s\n", __func__);

	if (strncmp(eerec->assert_type, "md32", 4) == 0) {
		msgsize = eerec->ee_log_size + 128;
		rep_msg = msg_create(&eerec->msg, msgsize);
		if (rep_msg == NULL)
			return;

		data = (char *)rep_msg + sizeof(struct AE_Msg);
		/* n += snprintf(data + n, msgsize - n, "== EXTERNAL EXCEPTION LOG ==\n"); */
		/* n += snprintf(data + n, msgsize - n, "%s\n", (char *)eerec->ee_log); */
		l = snprintf(data + n, msgsize - n, "== EXTERNAL EXCEPTION LOG ==\n%s\n", (char *)eerec->ee_log);
		if (l >= msgsize - n)
			LOGE("ee_log may overflow! %d >= %d\n", l, msgsize - n);
		n += min(l, msgsize - n);
	} else {
		if (strncmp(eerec->assert_type, "modem", 5) == 0) {
			if (sscanf(eerec->exp_filename, "md%d:", &md_id) == 1) {
				if (aee_dump_ccci_debug_info(md_id, (void **)&ccci_log, &ccci_log_size)) {
					ccci_log = NULL;
					ccci_log_size = 0;
				}
			}
		}
		msgsize = (eerec->ee_log_size + ccci_log_size) * 4 + 128;
		rep_msg = msg_create(&eerec->msg, msgsize);
		if (rep_msg == NULL)
			return;

		data = (char *)rep_msg + sizeof(struct AE_Msg);
		n += snprintf(data + n, msgsize - n, "== EXTERNAL EXCEPTION LOG ==\n");
		mem = (int *)eerec->ee_log;
		if (mem) {
			for (i = 0; i < eerec->ee_log_size / 4; i += 4) {
				n += snprintf(data + n, msgsize - n, "0x%08X 0x%08X 0x%08X 0x%08X\n",
					     mem[i], mem[i + 1], mem[i + 2], mem[i + 3]);
			}
		} else {
			n += snprintf(data + n, msgsize - n, "kmalloc fail, no log available\n");
		}
	}
	l = snprintf(data + n, msgsize - n, "== MEM DUMP(%d) ==\n", eerec->ee_phy_size);
	n += min(l, msgsize - n);
	if (ccci_log) {
		n += snprintf(data + n, msgsize - n, "== CCCI LOG ==\n");
		mem = (int *)ccci_log;
		for (i = 0; i < ccci_log_size / 4; i += 4) {
			n += snprintf(data + n, msgsize - n, "0x%08X 0x%08X 0x%08X 0x%08X\n",
				     mem[i], mem[i + 1], mem[i + 2], mem[i + 3]);
		}
		n += snprintf(data + n, msgsize - n, "== MEM DUMP(%d) ==\n", ccci_log_size);
	}

	rep_msg->cmdType = AE_RSP;
	rep_msg->cmdId = AE_REQ_DETAIL;
	rep_msg->arg = AE_PASS_BY_MEM;
	rep_msg->len = n + 1;
}

static void ee_gen_coredump_msg(void)
{
	struct AE_Msg *rep_msg;
	char *data;

	LOGD("%s\n", __func__);

	rep_msg = msg_create(&aed_dev.eerec->msg, 256);
	if (rep_msg == NULL)
		return;

	data = (char *)rep_msg + sizeof(struct AE_Msg);
	rep_msg->cmdType = AE_RSP;
	rep_msg->cmdId = AE_REQ_COREDUMP;
	rep_msg->arg = 0;
	snprintf(data, 256, "/proc/aed/%s", CURRENT_EE_COREDUMP);
	rep_msg->len = strlen(data) + 1;
}

static void ee_destroy_log(void)
{
	struct aed_eerec *eerec = aed_dev.eerec;

	LOGD("%s\n", __func__);

	if (eerec == NULL)
		return;

	aed_dev.eerec = NULL;
	msg_destroy(&eerec->msg);

	if (eerec->ee_phy != NULL) {
		vfree(eerec->ee_phy);
		eerec->ee_phy = NULL;
	}
	eerec->ee_log_size = 0;
	eerec->ee_phy_size = 0;

	if (eerec->ee_log != NULL) {
		kfree(eerec->ee_log);
		/*after this, another ee can enter */
		eerec->ee_log = NULL;
	}

	kfree(eerec);
}

static int ee_log_avail(void)
{
	return (aed_dev.eerec != NULL);
}

static char *ee_msg_avail(void)
{
	if (aed_dev.eerec)
		return aed_dev.eerec->msg;
	return NULL;
}

static void ee_gen_ind_msg(struct aed_eerec *eerec)
{
	unsigned long flags = 0;
	struct AE_Msg *rep_msg;

	LOGD("%s\n", __func__);
	if (eerec == NULL)
		return;

	/*
	 * Don't lock the whole function for the time is uncertain.
	 * we rely on the fact that ee_rec is not null if race here!
	 */
	spin_lock_irqsave(&aed_device_lock, flags);

	if (aed_dev.eerec == NULL) {
		aed_dev.eerec = eerec;
	} else {
		/* should never come here, skip*/
		spin_unlock_irqrestore(&aed_device_lock, flags);
		LOGW("%s: More than one EE message queued\n", __func__);
		return;
	}
	spin_unlock_irqrestore(&aed_device_lock, flags);

	rep_msg = msg_create(&aed_dev.eerec->msg, 0);
	if (rep_msg == NULL)
		return;

	rep_msg->cmdType = AE_IND;
	rep_msg->cmdId = AE_IND_EXP_RAISED;
	rep_msg->arg = AE_EE;
	rep_msg->len = 0;
	rep_msg->dbOption = eerec->db_opt;

	init_completion(&aed_ee_com);
	wake_up(&aed_dev.eewait);
	if (wait_for_completion_timeout(&aed_ee_com, msecs_to_jiffies(5 * 60 * 1000)))
		LOGE("%s: TIMEOUT, not receive close event, skip\n", __func__);
}

static void ee_queue_request(struct aed_eerec *eerec)
{
	int ret;
	unsigned long flags = 0;

	spin_lock_irqsave(&ee_queue.lock, flags);
	list_add_tail(&eerec->list, &ee_queue.list);
	spin_unlock_irqrestore(&ee_queue.lock, flags);
	ret = queue_work(system_wq, &ee_work);
	LOGI("%s: add new ee work, status %d\n", __func__, ret);
}

static void ee_worker(struct work_struct *work)
{
	struct aed_eerec *eerec, *tmp;
	unsigned long flags = 0;

	list_for_each_entry_safe(eerec, tmp, &ee_queue.list, list) {
		if (eerec == NULL) {
			LOGE("%s:null eerec\n", __func__);
			return;
		}

		ee_gen_ind_msg(eerec);
		spin_lock_irqsave(&ee_queue.lock, flags);
		list_del(&eerec->list);
		spin_unlock_irqrestore(&ee_queue.lock, flags);
		ee_destroy_log();
	}
}

/******************************************************************************
 * AED EE File operations
 *****************************************************************************/
static int aed_ee_open(struct inode *inode, struct file *filp)
{
	LOGD("%s:%d:%d\n", __func__, MAJOR(inode->i_rdev), MINOR(inode->i_rdev));
	return 0;
}

static int aed_ee_release(struct inode *inode, struct file *filp)
{
	LOGD("%s:%d:%d\n", __func__, MAJOR(inode->i_rdev), MINOR(inode->i_rdev));
	return 0;
}

static unsigned int aed_ee_poll(struct file *file, struct poll_table_struct *ptable)
{
	/* LOGD("%s\n", __func__); */
	if (ee_log_avail() && ee_msg_avail())
		return POLLIN | POLLRDNORM | POLLOUT | POLLWRNORM;
	poll_wait(file, &aed_dev.eewait, ptable);
	return 0;
}

static ssize_t aed_ee_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	if (aed_dev.eerec == NULL) {
		LOGE("aed_ee_read fail for invalid kerec\n");
		return 0;
	}
	return msg_copy_to_user(__func__, aed_dev.eerec->msg, buf, count, f_pos);
}

static ssize_t aed_ee_write(struct file *filp, const char __user *buf, size_t count,
			    loff_t *f_pos)
{
	struct AE_Msg msg;
	int rsize;
	struct aed_eerec *eerec = aed_dev.eerec;

	/* recevied a new request means the previous response is unavilable */
	/* 1. set position to be zero */
	/* 2. destroy the previous response message */
	*f_pos = 0;

	if (!eerec)
		return -1;

	msg_destroy(&eerec->msg);

	/* the request must be an *struct AE_Msg buffer */
	if (count != sizeof(struct AE_Msg)) {
		LOGD("%s: ERR, aed_write count=%zx\n", __func__, count);
		return -1;
	}

	rsize = copy_from_user(&msg, buf, count);
	if (rsize != 0) {
		LOGE("%s: ERR, copy_from_user rsize=%d\n", __func__, rsize);
		return -1;
	}

	/*the same reason removing "AEE api log available". msg_show(__func__, &msg);*/

	if (msg.cmdType == AE_REQ) {
		if (!ee_log_avail()) {
			ee_gen_notavail_msg();
			return count;
		}
		switch (msg.cmdId) {
		case AE_REQ_CLASS:
			ee_gen_class_msg();
			break;
		case AE_REQ_TYPE:
			ee_gen_type_msg();
			break;
		case AE_REQ_DETAIL:
			ee_gen_detail_msg();
			break;
		case AE_REQ_PROCESS:
			ee_gen_process_msg();
			break;
		case AE_REQ_BACKTRACE:
			ee_gen_notavail_msg();
			break;
		case AE_REQ_COREDUMP:
			ee_gen_coredump_msg();
			break;
		default:
			LOGD("Unknown command id %d\n", msg.cmdId);
			ee_gen_notavail_msg();
			break;
		}
	} else if (msg.cmdType == AE_IND) {
		switch (msg.cmdId) {
		case AE_IND_LOG_CLOSE:
			complete(&aed_ee_com);
			break;
		default:
			/* IGNORE */
			break;
		}
	} else if (msg.cmdType == AE_RSP) {	/* IGNORE */
	}

	return count;
}

/******************************************************************************
 * AED KE File operations
 *****************************************************************************/
static int aed_ke_open(struct inode *inode, struct file *filp)
{
	struct aee_oops *oops_open = NULL;
	int major = MAJOR(inode->i_rdev);
	int minor = MINOR(inode->i_rdev);
	unsigned char *devname = filp->f_path.dentry->d_iname;

	LOGD("%s:(%s)%d:%d\n", __func__, devname, major, minor);

	if (strstr(devname, "aed1")) {	/* aed_ke_open is also used by other device */
		oops_open = ipanic_oops_copy();
		if (oops_open == NULL)
			return 0;
		/* The panic log only occur on system startup, so check it now */
		ke_queue_request(oops_open);
	}
	return 0;
}

static int aed_ke_release(struct inode *inode, struct file *filp)
{
	LOGD("%s:%d:%d\n", __func__, MAJOR(inode->i_rdev), MINOR(inode->i_rdev));
	return 0;
}

static unsigned int aed_ke_poll(struct file *file, struct poll_table_struct *ptable)
{
	if (ke_log_avail())
		return POLLIN | POLLRDNORM | POLLOUT | POLLWRNORM;
	poll_wait(file, &aed_dev.kewait, ptable);
	return 0;
}


struct current_ke_buffer {
	void *data;
	ssize_t size;
};

static void *current_ke_start(struct seq_file *m, loff_t *pos)
{
	struct current_ke_buffer *ke_buffer;
	int index;

	ke_buffer = m->private;
	if (ke_buffer == NULL)
		return NULL;
	index = *pos * (PAGE_SIZE - 1);
	if (index < ke_buffer->size)
		return ke_buffer->data + index;
	return NULL;
}

static void *current_ke_next(struct seq_file *m, void *p, loff_t *pos)
{
	struct current_ke_buffer *ke_buffer;
	int index;

	ke_buffer = m->private;
	if (ke_buffer == NULL)
		return NULL;
	++*pos;
	index = *pos * (PAGE_SIZE - 1);
	if (index < ke_buffer->size)
		return ke_buffer->data + index;
	return NULL;
}

static void current_ke_stop(struct seq_file *m, void *p)
{
}

static int current_ke_show(struct seq_file *m, void *p)
{
	unsigned long len;
	struct current_ke_buffer *ke_buffer;

	ke_buffer = m->private;
	if (ke_buffer == NULL)
		return 0;
	if ((unsigned long)p >= (unsigned long)ke_buffer->data + ke_buffer->size)
		return 0;
	len = (unsigned long)ke_buffer->data + ke_buffer->size - (unsigned long)p;
	len = len < PAGE_SIZE ? len : (PAGE_SIZE - 1);
	if (seq_write(m, p, len)) {
		len = 0;
		return -1;
	}
	return 0;
}

static const struct seq_operations current_ke_op = {
	.start = current_ke_start,
	.next = current_ke_next,
	.stop = current_ke_stop,
	.show = current_ke_show
};

#define AED_CURRENT_KE_OPEN(ENTRY) \
static int current_ke_##ENTRY##_open(struct inode *inode, struct file *file) \
{ \
	int ret; \
	struct aee_oops *oops; \
	struct seq_file *m; \
	struct current_ke_buffer *ke_buffer; \
	ret = seq_open_private(file, &current_ke_op, sizeof(struct current_ke_buffer)); \
	if (ret == 0) { \
		oops = aed_dev.kerec.lastlog; \
		m = file->private_data; \
		if (!oops) \
			return ret; \
		ke_buffer = (struct current_ke_buffer *)m->private; \
		ke_buffer->data = oops->ENTRY; \
		ke_buffer->size = oops->ENTRY##_len;\
	} \
	return ret; \
}

#define AED_PROC_CURRENT_KE_FOPS(ENTRY) \
static const struct file_operations proc_current_ke_##ENTRY##_fops = { \
	.open		= current_ke_##ENTRY##_open, \
	.read		= seq_read, \
	.llseek		= seq_lseek, \
	.release	= seq_release_private, \
}


static ssize_t aed_ke_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	return msg_copy_to_user(__func__, aed_dev.kerec.msg, buf, count, f_pos);
}

static ssize_t aed_ke_write(struct file *filp, const char __user *buf, size_t count,
			    loff_t *f_pos)
{
	struct AE_Msg msg;
	int rsize;

	/* recevied a new request means the previous response is unavilable */
	/* 1. set position to be zero */
	/* 2. destroy the previous response message */
	*f_pos = 0;
	msg_destroy(&aed_dev.kerec.msg);

	/* the request must be an * AE_Msg buffer */
	if (count != sizeof(struct AE_Msg)) {
		LOGD("ERR: aed_write count=%zx\n", count);
		return -1;
	}

	rsize = copy_from_user(&msg, buf, count);
	if (rsize != 0) {
		LOGD("copy_from_user rsize=%d\n", rsize);
		return -1;
	}

	/*the same reason removing "AEE api log available". msg_show(__func__, &msg);*/

	if (msg.cmdType == AE_REQ) {
		if (!ke_log_avail()) {
			ke_gen_notavail_msg();

			return count;
		}

		switch (msg.cmdId) {
		case AE_REQ_CLASS:
			ke_gen_class_msg();
			break;
		case AE_REQ_TYPE:
			ke_gen_type_msg();
			break;
		case AE_REQ_MODULE:
			ke_gen_module_msg();
			break;
		case AE_REQ_DETAIL:
			ke_gen_detail_msg(&msg);
			break;
		case AE_REQ_PROCESS:
			ke_gen_process_msg();
			break;
		case AE_REQ_BACKTRACE:
			ke_gen_backtrace_msg();
			break;
		case AE_REQ_USERSPACEBACKTRACE:
			ke_gen_userbacktrace_msg();
			break;
		case AE_REQ_USER_REG:
			ke_gen_user_reg_msg();
			break;
		case AE_REQ_USER_MAPS:
			ke_gen_usermaps_msg();
			break;
		default:
			ke_gen_notavail_msg();
			break;
		}
	} else if (msg.cmdType == AE_IND) {
		switch (msg.cmdId) {
		case AE_IND_LOG_CLOSE:
			/* real release operation move to ke_worker(): ke_destroy_log(); */
			complete(&aed_ke_com);
			break;
		default:
			/* IGNORE */
			break;
		}
	} else if (msg.cmdType == AE_RSP) {	/* IGNORE */
	}

	return count;
}

static long aed_ioctl_bt(unsigned long arg)
{
	int ret = 0;
	struct aee_ioctl ioctl;
	struct aee_process_bt bt;

	if (copy_from_user(&ioctl, (struct aee_ioctl __user *)arg, sizeof(struct aee_ioctl))) {
		ret = -EFAULT;
		return ret;
	}
	bt.pid = ioctl.pid;
	ret = aed_get_process_bt(&bt);
	if (ret == 0) {
		ioctl.detail = 0xAEE00001;
		ioctl.size = bt.nr_entries;
		if (copy_to_user((struct aee_ioctl __user *)arg, &ioctl, sizeof(struct aee_ioctl))) {
			ret = -EFAULT;
			return ret;
		}
		if (!ioctl.out) {
			ret = -EFAULT;
		} else
		    if (copy_to_user
			((struct aee_bt_frame __user *)(unsigned long)ioctl.out,
			 (const void *)bt.entries, sizeof(struct aee_bt_frame) * AEE_NR_FRAME)) {
			ret = -EFAULT;
		}
	}
	return ret;
}

/*
 * aed process daemon and other command line may access me
 * concurrently
 */
DEFINE_SEMAPHORE(aed_dal_sem);
static long aed_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	if (cmd == AEEIOCTL_GET_PROCESS_BT)
		return aed_ioctl_bt(arg);


	if (down_interruptible(&aed_dal_sem) < 0)
		return -ERESTARTSYS;

	switch (cmd) {
	case AEEIOCTL_SET_AEE_MODE:
		{
			if (copy_from_user(&aee_mode, (void __user *)arg, sizeof(aee_mode))) {
				ret = -EFAULT;
				goto EXIT;
			}
			LOGD("set aee mode = %d\n", aee_mode);
			break;
		}
	case AEEIOCTL_SET_AEE_FORCE_EXP:
		{
			if (copy_from_user(&aee_force_exp, (void __user *)arg, sizeof(aee_force_exp))) {
				ret = -EFAULT;
				goto EXIT;
			}
			LOGD("set aee force_exp = %d\n", aee_force_exp);
			break;
		}
	case AEEIOCTL_DAL_SHOW:
		{
			/*It's troublesome to allocate more than 1KB size on stack */
			struct aee_dal_show *dal_show = kzalloc(sizeof(struct aee_dal_show),
								GFP_KERNEL);
			if (dal_show == NULL) {
				ret = -EFAULT;
				goto EXIT;
			}

			if (copy_from_user(dal_show, (struct aee_dal_show __user *)arg,
					   sizeof(struct aee_dal_show))) {
				ret = -EFAULT;
				goto OUT;
			}

			if (aee_mode >= AEE_MODE_CUSTOMER_ENG) {
				LOGD("DAL_SHOW not allowed (mode %d)\n", aee_mode);
				goto OUT;
			}

			/* Try to prevent overrun */
			dal_show->msg[sizeof(dal_show->msg) - 1] = 0;
#ifdef CONFIG_MTK_FB
			LOGD("AEE CALL DAL_Printf now\n");
			DAL_Printf("%s", dal_show->msg);
#endif

 OUT:
			kfree(dal_show);
			dal_show = NULL;
			goto EXIT;
		}

	case AEEIOCTL_DAL_CLEAN:
		{
			/* set default bgcolor to red, it will be used in DAL_Clean */
			struct aee_dal_setcolor dal_setcolor;

			dal_setcolor.foreground = 0x00ff00;	/*green */
			dal_setcolor.background = 0xff0000;	/*red */

#ifdef CONFIG_MTK_FB
			LOGD("AEE CALL DAL_SetColor now\n");
			DAL_SetColor(dal_setcolor.foreground, dal_setcolor.background);
			LOGD("AEE CALL DAL_Clean now\n");
			DAL_Clean();
#endif
			break;
		}

	case AEEIOCTL_SETCOLOR:
		{
			struct aee_dal_setcolor dal_setcolor;

			if (aee_mode >= AEE_MODE_CUSTOMER_ENG) {
				LOGD("SETCOLOR not allowed (mode %d)\n", aee_mode);
				goto EXIT;
			}

			if (copy_from_user(&dal_setcolor, (struct aee_dal_setcolor __user *)arg,
					   sizeof(struct aee_dal_setcolor))) {
				ret = -EFAULT;
				goto EXIT;
			}
#ifdef CONFIG_MTK_FB
			LOGD("AEE CALL DAL_SetColor now\n");
			DAL_SetColor(dal_setcolor.foreground, dal_setcolor.background);
			LOGD("AEE CALL DAL_SetScreenColor now\n");
			DAL_SetScreenColor(dal_setcolor.screencolor);
#endif
			break;
		}

	case AEEIOCTL_GET_THREAD_REG:
		{
			struct aee_thread_reg *tmp;

			LOGD("%s: get thread registers ioctl\n", __func__);

			tmp = kzalloc(sizeof(struct aee_thread_reg), GFP_KERNEL);
			if (tmp == NULL) {
				ret = -ENOMEM;
				goto EXIT;
			}

			if (copy_from_user
			    (tmp, (struct aee_thread_reg __user *)arg,
			     sizeof(struct aee_thread_reg))) {
				kfree(tmp);
				ret = -EFAULT;
				goto EXIT;
			}

			if (tmp->tid > 0) {
				struct task_struct *task;
				struct pt_regs *user_ret = NULL;

				rcu_read_lock();
				task = find_task_by_vpid(tmp->tid);
				if (task == NULL) {
					rcu_read_unlock();
					kfree(tmp);
					ret = -EINVAL;
					goto EXIT;
				}
				rcu_read_unlock();

				user_ret = task_pt_regs(task);
				memcpy(&(tmp->regs), user_ret, sizeof(struct pt_regs));
				if (copy_to_user
				    ((struct aee_thread_reg __user *)arg, tmp,
				     sizeof(struct aee_thread_reg))) {
					kfree(tmp);
					ret = -EFAULT;
					goto EXIT;
				}

			} else {
				LOGD("%s: get thread registers ioctl tid invalid\n", __func__);
				kfree(tmp);
				ret = -EINVAL;
				goto EXIT;
			}

			kfree(tmp);

			break;
		}

	case  AEEIOCTL_USER_IOCTL_TO_KERNEL_WANING: /* get current user space reg when call aee_kernel_warning_api */
			{
				LOGD("%s: AEEIOCTL_USER_IOCTL_TO_KERNEL_WANING,call kthread create ,is ok\n", __func__);
				/* kthread_create(Dstate_test, NULL, "D-state"); */

				aee_kernel_warning_api(__FILE__, __LINE__, DB_OPT_DEFAULT|DB_OPT_NATIVE_BACKTRACE,
						"AEEIOCTL_USER_IOCTL_TO_KERNEL_WANING",
						"Trigger Kernel warning");
				break;
			}

	case AEEIOCTL_CHECK_SUID_DUMPABLE:
		{
			int pid;

			LOGD("%s: check suid dumpable ioctl\n", __func__);

			if (copy_from_user(&pid, (void __user *)arg, sizeof(int))) {
				ret = -EFAULT;
				goto EXIT;
			}

			if (pid > 0) {
				struct task_struct *task;
				int dumpable = -1;

				rcu_read_lock();
				task = find_task_by_vpid(pid);
				if (task == NULL) {
					rcu_read_unlock();
					LOGD("%s: process:%d task null\n", __func__, pid);
					ret = -EINVAL;
					goto EXIT;
				}
				rcu_read_unlock();

				if (task->mm == NULL) {
					LOGD("%s: process:%d task mm null\n", __func__, pid);
					ret = -EINVAL;
					goto EXIT;
				}

				task_lock(task);
				dumpable = get_dumpable(task->mm);
				if ((dumpable == 0) && (task->mm != NULL)) {
					LOGD("%s: set process:%d dumpable\n", __func__, pid);
					set_dumpable(task->mm, 1);
				} else
					LOGD("%s: get process:%d dumpable:%d\n", __func__, pid,
					     dumpable);
				task_unlock(task);
			} else {
				LOGD("%s: check suid dumpable ioctl pid invalid\n", __func__);
				ret = -EINVAL;
			}

			break;
		}

	case AEEIOCTL_SET_FORECE_RED_SCREEN:
		{
			if (copy_from_user
			    (&force_red_screen, (void __user *)arg, sizeof(force_red_screen))) {
				ret = -EFAULT;
				goto EXIT;
			}
			LOGD("force aee red screen = %d\n", force_red_screen);
			break;
		}

	case AEEIOCTL_GET_AEE_SIGINFO:
		{
			struct aee_siginfo aee_si;

			LOGD("%s: get aee_siginfo ioctl\n", __func__);

			if (copy_from_user
			    (&aee_si, (struct aee_siginfo __user *)arg,
			     sizeof(aee_si))) {
				ret = -EFAULT;
				goto EXIT;
			}

			if (aee_si.tid > 0) {
				struct task_struct *task;
				siginfo_t *psi = NULL;

				rcu_read_lock();
				task = find_task_by_vpid(aee_si.tid);
				if (task == NULL) {
					rcu_read_unlock();
					ret = -EINVAL;
					goto EXIT;
				}
				rcu_read_unlock();

				psi = task->last_siginfo;
				if (psi) {
					aee_si.si_signo = psi->si_signo;
					aee_si.si_errno = psi->si_errno;
					if (psi->si_code >= 0)  /* debuggerd original_si_code */
						aee_si.si_code = psi->si_code & ~__SI_MASK;
					else
						aee_si.si_code = psi->si_code;
					aee_si.fault_addr = (uintptr_t)psi->si_addr;
					if (copy_to_user
						((struct aee_siginfo __user *)arg, &aee_si,
						sizeof(aee_si))) {
						ret = -EFAULT;
						goto EXIT;
					}
				}
			} else {
				LOGD("%s: get aee_siginfo ioctl tid invalid\n", __func__);
				ret = -EINVAL;
				goto EXIT;
			}

			break;
		}

	default:
		ret = -EINVAL;
	}

 EXIT:
	up(&aed_dal_sem);
	return ret;
}

static void aed_get_traces(char *msg)
{
	struct stack_trace trace;
	unsigned long stacks[32];
	int i;
	int offset;

	trace.entries = stacks;
	/*save backtraces */
	trace.nr_entries = 0;
	trace.max_entries = 32;
	trace.skip = 2;
	save_stack_trace_tsk(current, &trace);
	offset = strlen(msg);
	for (i = 0; i < trace.nr_entries; i++) {
		offset += snprintf(msg + offset, AEE_BACKTRACE_LENGTH - offset, "[<%p>] %pS\n",
				   (void *)trace.entries[i], (void *)trace.entries[i]);
	}
}

void Log2Buffer(struct aee_oops *oops, const char *fmt, ...)
{
	char buf[256];
	int len = 0;
	va_list ap;

	va_start(ap, fmt);
	len = strlen(oops->userthread_maps.Userthread_maps);

	if ((len + sizeof(buf)) < MaxMapsSize) {
		vsnprintf(&oops->userthread_maps.Userthread_maps[len], sizeof(buf), fmt, ap);
		oops->userthread_maps.Userthread_mapsLength = len + sizeof(buf);
	}
	va_end(ap);
}

int DumpThreadNativeInfo(struct aee_oops *oops)
{
	struct task_struct *current_task;
	struct pt_regs *user_ret;
	struct vm_area_struct *vma;
	unsigned long userstack_start = 0;
	unsigned long userstack_end = 0, length = 0;
	int mapcount = 0;
	struct file *file;
	int flags;
	struct mm_struct *mm;
	int ret = 0;

	current_task = get_current();
	user_ret = task_pt_regs(current_task);
	/* CurrentUserPid=current_task->pid; //Thread id */
	oops->userthread_reg.tid = current_task->tgid;
	oops->userthread_stack.tid = current_task->tgid;
	oops->userthread_maps.tid = current_task->tgid;

	memcpy(&oops->userthread_reg.regs, user_ret, sizeof(struct pt_regs));
	LOGE(" pid:%d /// tgid:%d, stack:0x%08lx\n",
			current_task->pid, current_task->tgid,
			(long)oops->userthread_stack.Userthread_Stack);
	if (!user_mode(user_ret))
		return 0;

	if (current_task->mm == NULL)
		return 0;



	#if 1
	vma = current_task->mm->mmap;
	while (vma && (mapcount < current_task->mm->map_count)) {
		file = vma->vm_file;
		flags = vma->vm_flags;
		if (file) {
			LOGE("%08lx-%08lx %c%c%c%c    %s\n", vma->vm_start, vma->vm_end,
			     flags & VM_READ ? 'r' : '-',
			     flags & VM_WRITE ? 'w' : '-',
			     flags & VM_EXEC ? 'x' : '-',
			     flags & VM_MAYSHARE ? 's' : 'p', (unsigned char *)(file->f_path.dentry->d_iname));
			Log2Buffer(oops, "%08lx-%08lx %c%c%c%c    %s\n", vma->vm_start, vma->vm_end,
					flags & VM_READ ? 'r' : '-',
					flags & VM_WRITE ? 'w' : '-',
					flags & VM_EXEC ? 'x' : '-',
					flags & VM_MAYSHARE ? 's' : 'p',
					(unsigned char *)(file->f_path.dentry->d_iname));
		} else {
			const char *name = arch_vma_name(vma);

			mm = vma->vm_mm;
			if (!name) {
				if (mm) {
					if (vma->vm_start <= mm->start_brk &&
					    vma->vm_end >= mm->brk) {
						name = "[heap]";
					} else if (vma->vm_start <= mm->start_stack &&
						   vma->vm_end >= mm->start_stack) {
						name = "[stack]";
					}
				} else {
					name = "[vdso]";
				}
			}
			/* if (name) */
			{

				LOGE("%08lx-%08lx %c%c%c%c    %s\n", vma->vm_start, vma->vm_end,
				     flags & VM_READ ? 'r' : '-',
				     flags & VM_WRITE ? 'w' : '-',
				     flags & VM_EXEC ? 'x' : '-',
				     flags & VM_MAYSHARE ? 's' : 'p', name);
				Log2Buffer(oops, "%08lx-%08lx %c%c%c%c    %s\n", vma->vm_start, vma->vm_end,
						flags & VM_READ ? 'r' : '-',
						flags & VM_WRITE ? 'w' : '-',
						flags & VM_EXEC ? 'x' : '-',
						flags & VM_MAYSHARE ? 's' : 'p', name);
			}
		}
		vma = vma->vm_next;
		mapcount++;

	}
	#endif

	LOGE("maps addr(0x%08lx), maps len:%d\n",
			(long)oops->userthread_maps.Userthread_maps,
			oops->userthread_maps.Userthread_mapsLength);

#ifndef __aarch64__ /* 32bit */
	LOGE(" pc/lr/sp 0x%08lx/0x%08lx/0x%08lx\n", user_ret->ARM_pc, user_ret->ARM_lr,
			 user_ret->ARM_sp);
		userstack_start = (unsigned long)user_ret->ARM_sp;

	vma = current_task->mm->mmap;
	while (vma != NULL) {
		if (vma->vm_start <= userstack_start && vma->vm_end >= userstack_start) {
			userstack_end = vma->vm_end;
			break;
		}
		vma = vma->vm_next;
		if (vma == current_task->mm->mmap)
			break;
	}
	if (userstack_end == 0) {
		LOGE("Dump native stack failed:\n");
		return 0;
	}
	LOGE("Dump stack range (0x%08lx:0x%08lx)\n", userstack_start, userstack_end);
	length = ((userstack_end - userstack_start) <
		     (MaxStackSize-1)) ? (userstack_end - userstack_start) : (MaxStackSize-1);
	oops->userthread_stack.StackLength = length;


	ret = copy_from_user((void *)(oops->userthread_stack.Userthread_Stack),
			(const void __user *)(userstack_start), length);
	LOGE("u+k 32 copy_from_user ret(0x%08x),len:%lx\n", ret, length);
	LOGE("end dump native stack:\n");
#else /* 64bit, First deal with K64+U64, the last time to deal with K64+U32 */

	if (is_compat_task()) {	/* K64_U32 */
		LOGE(" K64+ U32 pc/lr/sp 0x%16lx/0x%16lx/0x%16lx\n",
				(long)(user_ret->user_regs.pc),
				(long)(user_ret->user_regs.regs[14]),
				(long)(user_ret->user_regs.regs[13]));
		userstack_start = (unsigned long)user_ret->user_regs.regs[13];
		vma = current_task->mm->mmap;
		while (vma != NULL) {
			if (vma->vm_start <= userstack_start && vma->vm_end >= userstack_start) {
				userstack_end = vma->vm_end;
				break;
			}
		vma = vma->vm_next;
		if (vma == current_task->mm->mmap)
			break;
	}
	if (userstack_end == 0) {
		LOGE("Dump native stack failed:\n");
		return 0;
	}
	LOGE("Dump stack range (0x%08lx:0x%08lx)\n", userstack_start, userstack_end);
		length = ((userstack_end - userstack_start) <
		     (MaxStackSize-1)) ? (userstack_end - userstack_start) : (MaxStackSize-1);
		oops->userthread_stack.StackLength = length;
		ret = copy_from_user((void *)(oops->userthread_stack.Userthread_Stack),
				(const void __user *)(userstack_start), length);
		LOGE("copy_from_user ret(0x%16x),len:%lx\n", ret, length);
	} else {	/*K64+U64*/
		LOGE(" K64+ U64 pc/lr/sp 0x%16lx/0x%16lx/0x%16lx\n",
				(long)(user_ret->user_regs.pc),
				(long)(user_ret->user_regs.regs[30]),
				(long)(user_ret->user_regs.sp));
		userstack_start = (unsigned long)user_ret->user_regs.sp;
		vma = current_task->mm->mmap;
		while (vma != NULL) {
			if (vma->vm_start <= userstack_start && vma->vm_end >= userstack_start) {
				userstack_end = vma->vm_end;
				break;
			}
			vma = vma->vm_next;
			if (vma == current_task->mm->mmap)
				break;
		}
		if (userstack_end == 0) {
			LOGE("Dump native stack failed:\n");
			return 0;
		}

		LOGE("Dump stack range (0x%16lx:0x%16lx)\n", userstack_start, userstack_end);
		length = ((userstack_end - userstack_start) <
		     (MaxStackSize-1)) ? (userstack_end - userstack_start) : (MaxStackSize-1);
		oops->userthread_stack.StackLength = length;
		ret = copy_from_user((void *)(oops->userthread_stack.Userthread_Stack),
				(const void __user *)(userstack_start), length);
		LOGE("copy_from_user ret(0x%08x),len:%lx\n", ret, length);
	}

#endif
	return 0;
}

static void kernel_reportAPI(const AE_DEFECT_ATTR attr, const int db_opt, const char *module,
			     const char *msg)
{
	struct aee_oops *oops;
	int n = 0;

	if (aee_mode == AEE_MODE_CUSTOMER_USER || (aee_mode == AEE_MODE_CUSTOMER_ENG && attr == AE_DEFECT_WARNING))
		return;
	oops = aee_oops_create(attr, AE_KERNEL_PROBLEM_REPORT, module);
	if (oops != NULL) {
		n += snprintf(oops->backtrace, AEE_BACKTRACE_LENGTH, msg);
		snprintf(oops->backtrace + n, AEE_BACKTRACE_LENGTH - n, "\nBacktrace:\n");
		aed_get_traces(oops->backtrace);
		oops->detail = (char *)(oops->backtrace);
		oops->detail_len = strlen(oops->backtrace) + 1;
		oops->dump_option = db_opt;
#ifdef __aarch64__
		if ((db_opt & DB_OPT_NATIVE_BACKTRACE)  && !is_compat_task())
			oops->dump_option |= DB_OPT_AARCH64;
#endif
		if (db_opt & DB_OPT_NATIVE_BACKTRACE) {
			oops->userthread_stack.Userthread_Stack = vzalloc(MaxStackSize);
			if (oops->userthread_stack.Userthread_Stack == NULL) {
				LOGE("%s: oops->userthread_stack.Userthread_Stack Vmalloc fail", __func__);
				kfree(oops);
				return;
			}
			oops->userthread_maps.Userthread_maps = vzalloc(MaxMapsSize);
			if (oops->userthread_maps.Userthread_maps == NULL) {
				LOGE("%s: oops->userthread_maps.Userthread_maps Vmalloc fail", __func__);
				kfree(oops);
				return;
			}
			LOGE("%s: oops->userthread_stack.Userthread_Stack :0x%08lx,maps:0x%08lx",
					__func__,
					(long)oops->userthread_stack.Userthread_Stack,
					(long)oops->userthread_maps.Userthread_maps);
			oops->userthread_stack.StackLength = MaxStackSize;
			oops->userthread_maps.Userthread_mapsLength = MaxMapsSize;
			DumpThreadNativeInfo(oops);

		}
		LOGI("%s,%s,%s,0x%x\n", __func__, module, msg, db_opt);
		ke_queue_request(oops);
	}
}

#if 0/*disable aee_kernel_dal_api*/
void aee_kernel_dal_api(const char *file, const int line, const char *msg)
{
	LOGW("aee_kernel_dal_api : <%s:%d> %s ", file, line, msg);
	if (in_interrupt()) {
		LOGE("aee_kernel_dal_api: in interrupt context, skip");
		return;
	}

#if defined(CONFIG_MTK_AEE_AED) && defined(CONFIG_MTK_FB)
	if (down_interruptible(&aed_dal_sem) < 0) {
		LOGI("ERROR : aee_kernel_dal_api() get aed_dal_sem fail ");
		return;
	}
	if (msg != NULL) {
		struct aee_dal_setcolor dal_setcolor;
		struct aee_dal_show *dal_show = kzalloc(sizeof(struct aee_dal_show), GFP_KERNEL);

		if (dal_show == NULL) {
			LOGI("ERROR : aee_kernel_dal_api() kzalloc fail\n ");
			up(&aed_dal_sem);
			return;
		}
		if (((aee_mode == AEE_MODE_MTK_ENG) && (force_red_screen == AEE_FORCE_NOT_SET))
		    || ((aee_mode < AEE_MODE_CUSTOMER_ENG)
			&& (force_red_screen == AEE_FORCE_RED_SCREEN))) {
			dal_setcolor.foreground = 0xff00ff;	/* fg: purple */
			dal_setcolor.background = 0x00ff00;	/* bg: green */
			LOGD("AEE CALL DAL_SetColor now\n");
			DAL_SetColor(dal_setcolor.foreground, dal_setcolor.background);
			dal_setcolor.screencolor = 0xff0000;	/* screen:red */
			LOGD("AEE CALL DAL_SetScreenColor now\n");
			DAL_SetScreenColor(dal_setcolor.screencolor);
			strncpy(dal_show->msg, msg, sizeof(dal_show->msg) - 1);
			dal_show->msg[sizeof(dal_show->msg) - 1] = 0;
			LOGD("AEE CALL DAL_Printf now\n");
			DAL_Printf("%s", dal_show->msg);
		} else {
			LOGD("DAL not allowed (mode %d)\n", aee_mode);
		}
		kfree(dal_show);
	}
	up(&aed_dal_sem);
#endif
}
#else
void aee_kernel_dal_api(const char *file, const int line, const char *msg)
{
	LOGW("aee_kernel_dal_api has been phased out! caller info: <%s:%d> %s ", file, line, msg);
}
#endif
EXPORT_SYMBOL(aee_kernel_dal_api);

static void external_exception(const char *assert_type, const int *log, int log_size,
			       const int *phy, int phy_size, const char *detail, const int db_opt)
{
	int *ee_log = NULL;
	struct aed_eerec *eerec;

	LOGD("%s : [%s] log ptr %p size %d, phy ptr %p size %d\n", __func__,
	     assert_type, log, log_size, phy, phy_size);
	if ((aee_mode >= AEE_MODE_CUSTOMER_USER) && (aee_force_exp == AEE_FORCE_EXP_NOT_SET))
		return;
	eerec = kzalloc(sizeof(struct aed_eerec), GFP_ATOMIC);
	if (eerec == NULL) {
		LOGE("%s: kmalloc fail", __func__);
		return;
	}

	if ((log_size > 0) && (log != NULL)) {
		eerec->ee_log_size = log_size;
		ee_log = kmalloc(log_size, GFP_ATOMIC);
		if (ee_log != NULL) {
			eerec->ee_log = ee_log;
			memcpy(ee_log, log, log_size);
		}
	} else {
		eerec->ee_log_size = 16;
		ee_log = kzalloc(eerec->ee_log_size, GFP_ATOMIC);
		eerec->ee_log = ee_log;
	}

	if (ee_log == NULL) {
		LOGE("%s : memory alloc() fail\n", __func__);
		kfree(eerec);
		return;
	}


	memset(eerec->assert_type, 0, sizeof(eerec->assert_type));
	strncpy(eerec->assert_type, assert_type, sizeof(eerec->assert_type) - 1);
	memset(eerec->exp_filename, 0, sizeof(eerec->exp_filename));
	strncpy(eerec->exp_filename, detail, sizeof(eerec->exp_filename) - 1);
	LOGD("EE [%s]\n", eerec->assert_type);

	eerec->exp_linenum = 0;
	eerec->fatal1 = 0;
	eerec->fatal2 = 0;

	/* Check if we can dump memory */
	if (in_interrupt()) {
		/* kernel vamlloc cannot be used in interrupt context */
		LOGD("External exception occur in interrupt context, no coredump");
		phy_size = 0;
	} else if ((phy == NULL) || (phy_size > MAX_EE_COREDUMP)) {
		LOGD("EE Physical memory size(%d) too large or invalid", phy_size);
		phy_size = 0;
	}

	if (phy_size > 0) {
		eerec->ee_phy = (int *)vmalloc_user(phy_size);
		if (eerec->ee_phy != NULL) {
			memcpy(eerec->ee_phy, phy, phy_size);
			eerec->ee_phy_size = phy_size;
		} else {
			LOGD("Losing ee phy mem due to vmalloc return NULL\n");
			eerec->ee_phy_size = 0;
		}
	} else {
		eerec->ee_phy = NULL;
		eerec->ee_phy_size = 0;
	}
	eerec->db_opt = db_opt;
	ee_queue_request(eerec);
	LOGD("external_exception out\n");
}

static bool rr_reported;
module_param(rr_reported, bool, S_IRUSR | S_IWUSR);

static struct aee_kernel_api kernel_api = {
	.kernel_reportAPI = kernel_reportAPI,
	.md_exception = external_exception,
	.md32_exception = external_exception,
	.scp_exception = external_exception,
	.combo_exception = external_exception
};

AED_CURRENT_KE_OPEN(console);
AED_PROC_CURRENT_KE_FOPS(console);
AED_CURRENT_KE_OPEN(userspace_info);
AED_PROC_CURRENT_KE_FOPS(userspace_info);
AED_CURRENT_KE_OPEN(android_main);
AED_PROC_CURRENT_KE_FOPS(android_main);
AED_CURRENT_KE_OPEN(android_radio);
AED_PROC_CURRENT_KE_FOPS(android_radio);
AED_CURRENT_KE_OPEN(android_system);
AED_PROC_CURRENT_KE_FOPS(android_system);
AED_CURRENT_KE_OPEN(mmprofile);
AED_PROC_CURRENT_KE_FOPS(mmprofile);
AED_CURRENT_KE_OPEN(mini_rdump);
AED_PROC_CURRENT_KE_FOPS(mini_rdump);


static int current_ke_ee_coredump_open(struct inode *inode, struct file *file)
{
	int ret = seq_open_private(file, &current_ke_op, sizeof(struct current_ke_buffer));

	if (ret == 0) {
		struct aed_eerec *eerec = aed_dev.eerec;
		struct seq_file *m = file->private_data;
		struct current_ke_buffer *ee_buffer;

		if (!eerec)
			return ret;
		ee_buffer = (struct current_ke_buffer *)m->private;
		ee_buffer->data = eerec->ee_phy;
		ee_buffer->size = eerec->ee_phy_size;
	}
	return ret;
}

/* AED_CURRENT_KE_OPEN(ee_coredump); */
AED_PROC_CURRENT_KE_FOPS(ee_coredump);


static int aed_proc_init(void)
{
	aed_proc_dir = proc_mkdir("aed", NULL);
	if (aed_proc_dir == NULL) {
		LOGE("aed proc_mkdir failed\n");
		return -ENOMEM;
	}

	AED_PROC_ENTRY(current-ke-console, current_ke_console, S_IRUSR);
	AED_PROC_ENTRY(current-ke-userspace_info, current_ke_userspace_info, S_IRUSR);
	AED_PROC_ENTRY(current-ke-android_system, current_ke_android_system, S_IRUSR);
	AED_PROC_ENTRY(current-ke-android_radio, current_ke_android_radio, S_IRUSR);
	AED_PROC_ENTRY(current-ke-android_main, current_ke_android_main, S_IRUSR);
	AED_PROC_ENTRY(current-ke-mmprofile, current_ke_mmprofile, S_IRUSR);
	AED_PROC_ENTRY(current-ke-mini_rdump, current_ke_mini_rdump, S_IRUSR);
	AED_PROC_ENTRY(current-ee-coredump, current_ke_ee_coredump, S_IRUSR);

	aee_rr_proc_init(aed_proc_dir);

	aed_proc_debug_init(aed_proc_dir);

	dram_console_init(aed_proc_dir);

	return 0;
}

static int aed_proc_done(void)
{
	remove_proc_entry(CURRENT_KE_CONSOLE, aed_proc_dir);
	remove_proc_entry(CURRENT_EE_COREDUMP, aed_proc_dir);

	aed_proc_debug_done(aed_proc_dir);

	dram_console_done(aed_proc_dir);

	remove_proc_entry("aed", NULL);
	return 0;
}

/******************************************************************************
 * Module related
 *****************************************************************************/
static const struct file_operations aed_ee_fops = {
	.owner = THIS_MODULE,
	.open = aed_ee_open,
	.release = aed_ee_release,
	.poll = aed_ee_poll,
	.read = aed_ee_read,
	.write = aed_ee_write,
	.unlocked_ioctl = aed_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = aed_ioctl,
#endif
};

static const struct file_operations aed_ke_fops = {
	.owner = THIS_MODULE,
	.open = aed_ke_open,
	.release = aed_ke_release,
	.poll = aed_ke_poll,
	.read = aed_ke_read,
	.write = aed_ke_write,
	.unlocked_ioctl = aed_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = aed_ioctl,
#endif
};

/* QHQ RT Monitor end */
static struct miscdevice aed_ee_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "aed0",
	.fops = &aed_ee_fops,
};



static struct miscdevice aed_ke_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "aed1",
	.fops = &aed_ke_fops,
};

static int __init aed_init(void)
{
	int err = 0;

	err = aed_proc_init();
	if (err != 0)
		return err;

	err = ksysfs_bootinfo_init();
	if (err != 0)
		return err;

	spin_lock_init(&ke_queue.lock);
	spin_lock_init(&ee_queue.lock);
	INIT_LIST_HEAD(&ke_queue.list);
	INIT_LIST_HEAD(&ee_queue.list);

	init_waitqueue_head(&aed_dev.eewait);
	memset(&aed_dev.kerec, 0, sizeof(struct aed_kerec));
	init_waitqueue_head(&aed_dev.kewait);

	INIT_WORK(&ke_work, ke_worker);
	INIT_WORK(&ee_work, ee_worker);

	aee_register_api(&kernel_api);

	spin_lock_init(&aed_device_lock);
	err = misc_register(&aed_ee_dev);
	if (unlikely(err)) {
		LOGE("aee: failed to register aed0(ee) device!\n");
		return err;
	}

	err = misc_register(&aed_ke_dev);
	if (unlikely(err)) {
		LOGE("aee: failed to register aed1(ke) device!\n");
		return err;
	}

	return err;
}

static void __exit aed_exit(void)
{
	misc_deregister(&aed_ee_dev);
	misc_deregister(&aed_ke_dev);

	ee_destroy_log();
	ke_destroy_log();

	aed_proc_done();
	ksysfs_bootinfo_exit();
}
module_init(aed_init);
module_exit(aed_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MediaTek AED Driver");
MODULE_AUTHOR("MediaTek Inc.");
