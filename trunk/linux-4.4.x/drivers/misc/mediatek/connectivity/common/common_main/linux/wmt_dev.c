/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

/*! \file
 * \brief brief description
 *
 * Detailed descriptions here.
 *
 */

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/
#ifdef DFT_TAG
#undef DFT_TAG
#endif
#define DFT_TAG         "[WMT-DEV]"

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/

#ifdef CONFIG_COMPAT
#include <linux/compat.h>
#endif
#if WMT_CREATE_NODE_DYNAMIC
#include <linux/device.h>
#endif
#ifdef CONFIG_EARLYSUSPEND
#include <linux/earlysuspend.h>
#else
#include <linux/fb.h>
#endif
#include <linux/proc_fs.h>
#include <mtk_wcn_cmb_stub.h>
#include "osal_typedef.h"
#include "osal.h"
#include "wmt_dev.h"
#include "wmt_core.h"
#include "wmt_exp.h"
#include "wmt_lib.h"
#include "wmt_conf.h"
#include "wmt_dbg.h"
#include "psm_core.h"
#include "stp_core.h"
#include "stp_exp.h"
#include "bgw_desense.h"
#include "wmt_idc.h"
#include "wmt_detect.h"
#include "hif_sdio.h"

#ifdef CONFIG_COMPAT
#define COMPAT_WMT_IOCTL_SET_PATCH_NAME		_IOW(WMT_IOC_MAGIC, 4, compat_uptr_t)
#define COMPAT_WMT_IOCTL_LPBK_TEST		_IOWR(WMT_IOC_MAGIC, 8, compat_uptr_t)
#define COMPAT_WMT_IOCTL_SET_PATCH_INFO		_IOW(WMT_IOC_MAGIC, 15, compat_uptr_t)
#define COMPAT_WMT_IOCTL_PORT_NAME		_IOWR(WMT_IOC_MAGIC, 20, compat_uptr_t)
#define COMPAT_WMT_IOCTL_WMT_CFG_NAME		_IOWR(WMT_IOC_MAGIC, 21, compat_uptr_t)
#define COMPAT_WMT_IOCTL_SEND_BGW_DS_CMD	_IOW(WMT_IOC_MAGIC, 25, compat_uptr_t)
#define COMPAT_WMT_IOCTL_ADIE_LPBK_TEST		_IOWR(WMT_IOC_MAGIC, 26, compat_uptr_t)
#endif

#define WMT_IOC_MAGIC        0xa0
#define WMT_IOCTL_SET_PATCH_NAME	_IOW(WMT_IOC_MAGIC, 4, char*)
#define WMT_IOCTL_SET_STP_MODE		_IOW(WMT_IOC_MAGIC, 5, int)
#define WMT_IOCTL_FUNC_ONOFF_CTRL	_IOW(WMT_IOC_MAGIC, 6, int)
#define WMT_IOCTL_LPBK_POWER_CTRL	_IOW(WMT_IOC_MAGIC, 7, int)
#define WMT_IOCTL_LPBK_TEST		_IOWR(WMT_IOC_MAGIC, 8, char*)
#define WMT_IOCTL_GET_CHIP_INFO		_IOR(WMT_IOC_MAGIC, 12, int)
#define WMT_IOCTL_SET_LAUNCHER_KILL	_IOW(WMT_IOC_MAGIC, 13, int)
#define WMT_IOCTL_SET_PATCH_NUM		_IOW(WMT_IOC_MAGIC, 14, int)
#define WMT_IOCTL_SET_PATCH_INFO	_IOW(WMT_IOC_MAGIC, 15, char*)
#define WMT_IOCTL_PORT_NAME		_IOWR(WMT_IOC_MAGIC, 20, char*)
#define WMT_IOCTL_WMT_CFG_NAME		_IOWR(WMT_IOC_MAGIC, 21, char*)
#define WMT_IOCTL_WMT_QUERY_CHIPID	_IOR(WMT_IOC_MAGIC, 22, int)
#define WMT_IOCTL_WMT_TELL_CHIPID	_IOW(WMT_IOC_MAGIC, 23, int)
#define WMT_IOCTL_WMT_COREDUMP_CTRL	_IOW(WMT_IOC_MAGIC, 24, int)
#define WMT_IOCTL_SEND_BGW_DS_CMD	_IOW(WMT_IOC_MAGIC, 25, char*)
#define WMT_IOCTL_ADIE_LPBK_TEST	_IOWR(WMT_IOC_MAGIC, 26, char*)
#define WMT_IOCTL_WMT_STP_ASSERT_CTRL	_IOW(WMT_IOC_MAGIC, 27, int)
#define WMT_IOCTL_FW_DBGLOG_CTRL	_IOR(WMT_IOC_MAGIC, 29, int)
#define WMT_IOCTL_DYNAMIC_DUMP_CTRL	_IOR(WMT_IOC_MAGIC, 30, char*)

#define MTK_WMT_VERSION  "Consys WMT Driver - v1.0"
#define MTK_WMT_DATE     "2013/01/20"
#define WMT_DEV_MAJOR 190	/* never used number */
#define WMT_DEV_NUM 1
#define WMT_DEV_INIT_TO_MS (2 * 1000)
#define DYNAMIC_DUMP_BUF 109

#define WMT_DRIVER_NAME "mtk_stp_wmt"

P_OSAL_EVENT gpRxEvent;

UINT32 u4RxFlag;
static atomic_t gRxCount = ATOMIC_INIT(0);

/* Linux UINT8 device */
static INT32 gWmtMajor = WMT_DEV_MAJOR;
static struct cdev gWmtCdev;
static atomic_t gWmtRefCnt = ATOMIC_INIT(0);
/* WMT driver information */
static UINT8 gLpbkBuf[WMT_LPBK_BUF_LEN] = { 0 };

static UINT32 gLpbkBufLog;	/* George LPBK debug */
static INT32 gWmtInitDone;
static wait_queue_head_t gWmtInitWq;
#ifdef CONFIG_MTK_COMBO_COMM_APO
UINT32 always_pwr_on_flag;
#else
UINT32 always_pwr_on_flag = 1;
#endif
P_WMT_PATCH_INFO pPatchInfo;
UINT32 pAtchNum;
UINT32 currentLpbkStatus;



#if CFG_WMT_PROC_FOR_AEE
static struct proc_dir_entry *gWmtAeeEntry;
#define WMT_AEE_PROCNAME "driver/wmt_aee"
#define WMT_PROC_AEE_SIZE 3072
static UINT32 g_buf_len;
static PUINT8 pBuf;
#endif

#if WMT_CREATE_NODE_DYNAMIC
struct class *wmt_class;
struct device *wmt_dev;
#endif


/*LCM on/off ctrl for wmt varabile*/
UINT32 hif_info;
static struct work_struct gPwrOnOffWork;
UINT32 g_es_lr_flag_for_quick_sleep = 1;	/* for ctrl quick sleep flag */
UINT32 g_es_lr_flag_for_lpbk_onoff;	/* for ctrl lpbk on off */
OSAL_SLEEPABLE_LOCK g_es_lr_lock;


#ifdef CONFIG_EARLYSUSPEND
static VOID wmt_dev_early_suspend(struct early_suspend *h)
{
	osal_lock_sleepable_lock(&g_es_lr_lock);
	g_es_lr_flag_for_quick_sleep = 1;
	g_es_lr_flag_for_lpbk_onoff = 0;
	osal_unlock_sleepable_lock(&g_es_lr_lock);

	WMT_WARN_FUNC("@@@@@@@@@@wmt enter early suspend@@@@@@@@@@@@@@\n");

	schedule_work(&gPwrOnOffWork);
}

static VOID wmt_dev_late_resume(struct early_suspend *h)
{
	osal_lock_sleepable_lock(&g_es_lr_lock);
	g_es_lr_flag_for_quick_sleep = 0;
	g_es_lr_flag_for_lpbk_onoff = 1;
	osal_unlock_sleepable_lock(&g_es_lr_lock);

	WMT_WARN_FUNC("@@@@@@@@@@wmt enter late resume@@@@@@@@@@@@@@\n");

	schedule_work(&gPwrOnOffWork);

}

struct early_suspend wmt_early_suspend_handler = {
	.suspend = wmt_dev_early_suspend,
	.resume = wmt_dev_late_resume,
};

#else

static struct notifier_block wmt_fb_notifier;
static INT32 wmt_fb_notifier_callback(struct notifier_block *self, ULONG event, PVOID data)
{
	struct fb_event *evdata = data;
	INT32 blank;

	WMT_DBG_FUNC("wmt_fb_notifier_callback\n");

	/* If we aren't interested in this event, skip it immediately ... */
	if (event != FB_EVENT_BLANK)
		return 0;

	blank = *(INT32 *)evdata->data;
	WMT_DBG_FUNC("fb_notify(blank=%d)\n", blank);

	switch (blank) {
	case FB_BLANK_UNBLANK:
		osal_lock_sleepable_lock(&g_es_lr_lock);
		g_es_lr_flag_for_quick_sleep = 0;
		g_es_lr_flag_for_lpbk_onoff = 1;
		osal_unlock_sleepable_lock(&g_es_lr_lock);
		WMT_WARN_FUNC("@@@@@@@@@@wmt enter UNBLANK @@@@@@@@@@@@@@\n");
		if (hif_info == 0)
			break;
		schedule_work(&gPwrOnOffWork);
		break;
	case FB_BLANK_POWERDOWN:
		osal_lock_sleepable_lock(&g_es_lr_lock);
		g_es_lr_flag_for_quick_sleep = 1;
		g_es_lr_flag_for_lpbk_onoff = 0;
		osal_unlock_sleepable_lock(&g_es_lr_lock);
		WMT_WARN_FUNC("@@@@@@@@@@wmt enter early POWERDOWN @@@@@@@@@@@@@@\n");
		schedule_work(&gPwrOnOffWork);
		break;
	default:
		break;
	}
	return 0;
}
#endif /* CONFIG_EARLYSUSPEND */
/*******************************************************************************
*                          F U N C T I O N S
********************************************************************************
*/

static VOID wmt_pwr_on_off_handler(struct work_struct *work)
{
	INT32 retryCounter = 5;
	UINT32 lpbk_req_onoff;

	WMT_DBG_FUNC("wmt_pwr_on_off_handler start to run\n");

	if (always_pwr_on_flag != 0) {
		osal_lock_sleepable_lock(&g_es_lr_lock);
		lpbk_req_onoff = g_es_lr_flag_for_lpbk_onoff;
		osal_unlock_sleepable_lock(&g_es_lr_lock);
		if (currentLpbkStatus != lpbk_req_onoff)
			currentLpbkStatus = wmt_lpbk_handler(lpbk_req_onoff, retryCounter);
		else
			WMT_DBG_FUNC("drop wmt start to run\n");
	}
}

UINT32 wmt_lpbk_handler(UINT32 on_off_flag, UINT32 retry)
{
	UINT32 retry_count;
	UINT32 lpbk_status;

	retry_count = retry;
	if (on_off_flag) {
		do {
			if (mtk_wcn_wmt_func_on(WMTDRV_TYPE_LPBK) == MTK_WCN_BOOL_FALSE) {
				WMT_WARN_FUNC("WMT turn on LPBK fail, retrying, retryCounter left:%d!\n",
					      retry_count);
				retry_count--;
				osal_sleep_ms(1000);
				if (retry_count == 0)
					lpbk_status = 0;
			} else {
				WMT_DBG_FUNC("WMT turn on LPBK suceed\n");
				lpbk_status = 1;
				break;
			}
		} while (retry_count > 0);
	} else {
		if (mtk_wcn_wmt_func_off(WMTDRV_TYPE_LPBK) == MTK_WCN_BOOL_FALSE) {
			WMT_WARN_FUNC("WMT turn off LPBK fail\n");
			lpbk_status = 1;
		} else {
			WMT_DBG_FUNC("WMT turn off LPBK suceed\n");
			lpbk_status = 0;
		}
	}
	return lpbk_status;
}

MTK_WCN_BOOL wmt_dev_get_early_suspend_state(VOID)
{
	MTK_WCN_BOOL bRet =
		(g_es_lr_flag_for_quick_sleep == 0) ? MTK_WCN_BOOL_FALSE : MTK_WCN_BOOL_TRUE;
	/* WMT_INFO_FUNC("bRet:%d\n", bRet); */
	return bRet;
}

#if CFG_WMT_PROC_FOR_AEE
static ssize_t wmt_dev_proc_for_aee_read(struct file *filp, char __user *buf, size_t count,
		loff_t *f_pos)
{
	INT32 retval = 0;
	UINT32 len = 0;

	WMT_INFO_FUNC("%s: count %d pos %lld\n", __func__, count, *f_pos);

	if (*f_pos == 0) {
		pBuf = wmt_lib_get_cpupcr_xml_format(&len);
		g_buf_len = len;
		WMT_INFO_FUNC("wmt_dev:wmt for aee buffer len(%d)\n", g_buf_len);
	}

	if (g_buf_len >= count) {
		retval = copy_to_user(buf, pBuf, count);
		if (retval) {
			WMT_ERR_FUNC("copy to aee buffer failed, ret:%d\n", retval);
			retval = -EFAULT;
			goto err_exit;
		}

		*f_pos += count;
		g_buf_len -= count;
		pBuf += count;
		WMT_INFO_FUNC("wmt_dev:after read,wmt for aee buffer len(%d)\n", g_buf_len);

		retval = count;
	} else if (g_buf_len != 0) {
		retval = copy_to_user(buf, pBuf, g_buf_len);
		if (retval) {
			WMT_ERR_FUNC("copy to aee buffer failed, ret:%d\n", retval);
			retval = -EFAULT;
			goto err_exit;
		}

		*f_pos += g_buf_len;
		len = g_buf_len;
		g_buf_len = 0;
		pBuf += len;
		retval = len;
		WMT_INFO_FUNC("wmt_dev:after read,wmt for aee buffer len(%d)\n", g_buf_len);
	} else {
		WMT_INFO_FUNC("wmt_dev: no data available for aee\n");
		retval = 0;
	}

err_exit:
	return retval;
}

static ssize_t wmt_dev_proc_for_aee_write(struct file *filp, const char __user *buf, size_t count,
		loff_t *f_pos)
{
	WMT_TRC_FUNC();
	return 0;
}

INT32 wmt_dev_proc_for_aee_setup(VOID)
{
	static const struct file_operations wmt_aee_fops = {
		.owner = THIS_MODULE,
		.read = wmt_dev_proc_for_aee_read,
		.write = wmt_dev_proc_for_aee_write,
	};

	gWmtAeeEntry = proc_create(WMT_AEE_PROCNAME, 0664, NULL, &wmt_aee_fops);
	if (gWmtAeeEntry == NULL) {
		WMT_ERR_FUNC("Unable to create /proc entry\n\r");
		return -1;
	}

	return 0;
}

INT32 wmt_dev_proc_for_aee_remove(VOID)
{
	if (gWmtAeeEntry != NULL)
		remove_proc_entry(WMT_AEE_PROCNAME, NULL);

	return 0;
}
#endif /* CFG_WMT_PROC_FOR_AEE */

VOID wmt_dev_rx_event_cb(VOID)
{
	u4RxFlag = 1;
	atomic_inc(&gRxCount);
	if (gpRxEvent != NULL) {
		/* u4RxFlag = 1; */
		/* atomic_inc(&gRxCount); */
		wake_up_interruptible(&gpRxEvent->waitQueue);
	} else {
		/* WMT_ERR_FUNC("null gpRxEvent, flush rx!\n"); */
		/* wmt_lib_flush_rx(); */
	}
}

INT32 wmt_dev_rx_timeout(P_OSAL_EVENT pEvent)
{
	UINT32 ms = pEvent->timeoutValue;
	LONG lRet = 0;

	gpRxEvent = pEvent;
	if (ms != 0)
		lRet = wait_event_interruptible_timeout(gpRxEvent->waitQueue, 0 != u4RxFlag,
				msecs_to_jiffies(ms));
	else
		lRet = wait_event_interruptible(gpRxEvent->waitQueue, u4RxFlag != 0);

	u4RxFlag = 0;
/* gpRxEvent = NULL; */
	if (atomic_dec_return(&gRxCount)) {
		WMT_ERR_FUNC("gRxCount != 0 (%d), reset it!\n", atomic_read(&gRxCount));
		atomic_set(&gRxCount, 0);
	}

	return lRet;
}

/* TODO: [ChangeFeature][George] refine this function name for general filesystem read operation, not patch only. */
INT32 wmt_dev_patch_get(PUINT8 pPatchName, osal_firmware **ppPatch)
{
	INT32 iRet = -1;
	osal_firmware *fw = NULL;

	if (!ppPatch) {
		WMT_ERR_FUNC("invalid ppBufptr!\n");
		return -1;
	}
	*ppPatch = NULL;
	iRet = request_firmware((const struct firmware **)&fw, pPatchName, NULL);
	if (iRet != 0) {
		WMT_ERR_FUNC("failed to open or read!(%s)\n", pPatchName);
		return -1;
	}
	WMT_INFO_FUNC("loader firmware %s  ok!!\n", pPatchName);
	iRet = 0;
	*ppPatch = fw;

	return iRet;
}

INT32 wmt_dev_patch_put(osal_firmware **ppPatch)
{
	if (*ppPatch != NULL) {
		release_firmware((const struct firmware *)*ppPatch);
		*ppPatch = NULL;
	}
	return 0;
}

VOID wmt_dev_patch_info_free(VOID)
{
	kfree(pPatchInfo);
	pPatchInfo = NULL;
}

MTK_WCN_BOOL wmt_dev_is_file_exist(PUINT8 pFileName)
{
	INT32 iRet = 0;
	osal_firmware *fw = NULL;

	if (pFileName == NULL) {
		WMT_ERR_FUNC("invalid file name pointer(%p)\n", pFileName);
		return MTK_WCN_BOOL_FALSE;
	}

	if (osal_strlen(pFileName) < osal_strlen(defaultPatchName)) {
		WMT_ERR_FUNC("invalid file name(%s)\n", pFileName);
		return MTK_WCN_BOOL_FALSE;
	}

	iRet = request_firmware((const struct firmware **)&fw, pFileName, NULL);
	if (iRet != 0) {
		WMT_ERR_FUNC("failed to open or read!(%s)\n", pFileName);
		return MTK_WCN_BOOL_FALSE;
	}
	release_firmware(fw);
	return true;
}

static ULONG count_last_access_sdio;
static ULONG count_last_access_btif;
static ULONG count_last_access_uart;
static ULONG jiffies_last_poll;

static INT32 wmt_dev_tra_sdio_update(VOID)
{
	count_last_access_sdio += 1;
	/* WMT_INFO_FUNC("jiffies_last_access_sdio: jiffies = %ul\n", jiffies); */

	return 0;
}

extern INT32 wmt_dev_tra_bitf_update(VOID)
{
	count_last_access_btif += 1;
	/* WMT_INFO_FUNC("jiffies_last_access_btif: jiffies = %ul\n", jiffies); */

	return 0;
}

extern INT32 wmt_dev_tra_uart_update(VOID)
{
	count_last_access_uart += 1;
	/* WMT_INFO_FUNC("jiffies_last_access_btif: jiffies = %ul\n", jiffies); */

	return 0;
}

static UINT32 wmt_dev_tra_poll(VOID)
{
#define TIME_THRESHOLD_TO_TEMP_QUERY 3000
#define COUNT_THRESHOLD_TO_TEMP_QUERY 200

	ULONG during_count = 0;
	ULONG poll_during_time = 0;
	ENUM_WMT_CHIP_TYPE chip_type;

	chip_type = wmt_detect_get_chip_type();
	/* if (jiffies > jiffies_last_poll) */
	if (time_after(jiffies, jiffies_last_poll))
		poll_during_time = jiffies - jiffies_last_poll;
	else
		poll_during_time = 0xffffffff;

	WMT_DBG_FUNC("**jiffies_to_mesecs(0xffffffff) = %lu\n", jiffies_to_msecs(0xffffffff));

	if (jiffies_to_msecs(poll_during_time) < TIME_THRESHOLD_TO_TEMP_QUERY) {
		WMT_DBG_FUNC("**poll_during_time = %lu < %lu, not to query\n",
			     jiffies_to_msecs(poll_during_time), TIME_THRESHOLD_TO_TEMP_QUERY);
		return -1;
	}

	switch (chip_type) {
	case WMT_CHIP_TYPE_COMBO:
		during_count = count_last_access_sdio;
		break;
	case WMT_CHIP_TYPE_SOC:
		during_count = (*mtk_wcn_wlan_bus_tx_cnt)();
		if (mtk_wcn_wlan_bus_tx_cnt == NULL) {
			WMT_ERR_FUNC("WMT-DEV:mtk_wcn_wlan_bus_tx_cnt null pointer\n");
			return -1;
		}
		if (mtk_wcn_wlan_bus_tx_cnt_clr == NULL) {
			WMT_ERR_FUNC("WMT-DEV:mtk_wcn_wlan_bus_tx_cnt_clr null pointer\n");
			return -3;
		}
		break;
	default:
		WMT_ERR_FUNC("WMT-DEV:error chip type(%d)\n", chip_type);
	}

	if (during_count < COUNT_THRESHOLD_TO_TEMP_QUERY) {
		WMT_DBG_FUNC("**during_count = %lu < %lu, not to query\n", during_count,
				COUNT_THRESHOLD_TO_TEMP_QUERY);
		return -2;
	}

	jiffies_last_poll = jiffies;
	if (chip_type == WMT_CHIP_TYPE_COMBO)
		count_last_access_sdio = 0;
	else if (chip_type == WMT_CHIP_TYPE_SOC)
		(*mtk_wcn_wlan_bus_tx_cnt_clr)();
	else
		WMT_ERR_FUNC("WMT-DEV:error chip type(%d)\n", chip_type);
	WMT_INFO_FUNC("**poll_during_time = %lu > %lu, during_count = %lu > %lu, query\n",
		      jiffies_to_msecs(poll_during_time), TIME_THRESHOLD_TO_TEMP_QUERY,
		      jiffies_to_msecs(during_count), COUNT_THRESHOLD_TO_TEMP_QUERY);

	return 0;
}

LONG wmt_dev_tm_temp_query(VOID)
{
#define HISTORY_NUM       5
#define TEMP_THRESHOLD   60
#define REFRESH_TIME    300	/* sec */

	static INT32 temp_table[HISTORY_NUM] = { 99 };	/* not query yet. */
	static INT32 idx_temp_table;
	static struct timeval query_time, now_time;
	INT8 query_cond = 0;
	INT32 current_temp = 0;
	INT32 index = 0;
	LONG return_temp = 0;

	/* Query condition 1: */
	/* If we have the high temperature records on the past, we continue to query/monitor */
	/* the real temperature until cooling */
	for (index = 0; index < HISTORY_NUM; index++) {
		if (temp_table[index] >= TEMP_THRESHOLD) {
			query_cond = 1;
			WMT_DBG_FUNC
				("temperature table is still initial value, we should query temp temperature..\n");
		}
	}

	do_gettimeofday(&now_time);
#if 1
	/* Query condition 2: */
	/* Moniter the bus activity to decide if we have the need to query temperature. */
	if (!query_cond) {
		if (wmt_dev_tra_poll() == 0) {
			query_cond = 1;
			WMT_INFO_FUNC("traffic , we must query temperature..\n");
		} else if (temp_table[idx_temp_table] >= TEMP_THRESHOLD) {
			WMT_INFO_FUNC("temperature maybe greater than 60, query temperature\n");
			query_cond = 1;
		} else
			WMT_DBG_FUNC("idle traffic ....\n");

		/* only WIFI tx power might make temperature varies largely */
#if 0
		if (!query_cond) {
			last_access_time = wmt_dev_tra_uart_poll();
			if (jiffies_to_msecs(last_access_time) < TIME_THRESHOLD_TO_TEMP_QUERY) {
				query_cond = 1;
				WMT_DBG_FUNC("uart busy traffic , we must query temperature..\n");
			} else {
				WMT_DBG_FUNC("uart still idle traffic , we don't query temp temperature..\n");
			}
		}
#endif
	}
#endif
	/* Query condition 3: */
	/* If the query time exceeds the a certain of period, refresh temp table. */
	/*  */
	if (!query_cond) {
		/* time overflow, we refresh temp table again for simplicity! */
		if ((now_time.tv_sec < query_time.tv_sec) ||
		    ((now_time.tv_sec > query_time.tv_sec) &&
			 (now_time.tv_sec - query_time.tv_sec) > REFRESH_TIME)) {
			query_cond = 1;

			WMT_INFO_FUNC
				("It is long time (> %d sec) not to query, we must query temp temperature..\n",
				 REFRESH_TIME);
			for (index = 0; index < HISTORY_NUM; index++)
				temp_table[index] = 99;

		}
	}

	if (query_cond) {
		/* update the temperature record */
		mtk_wcn_wmt_therm_ctrl(WMTTHERM_ENABLE);
		current_temp = mtk_wcn_wmt_therm_ctrl(WMTTHERM_READ);
		mtk_wcn_wmt_therm_ctrl(WMTTHERM_DISABLE);
		idx_temp_table = (idx_temp_table + 1) % HISTORY_NUM;
		temp_table[idx_temp_table] = current_temp;
		do_gettimeofday(&query_time);

		WMT_INFO_FUNC("[Thermal] current_temp = 0x%x\n", (current_temp & 0xFF));
	} else {
		current_temp = temp_table[idx_temp_table];
		idx_temp_table = (idx_temp_table + 1) % HISTORY_NUM;
		temp_table[idx_temp_table] = current_temp;
	}

	/*  */
	/* Dump information */
	/*  */
	WMT_DBG_FUNC("[Thermal] idx_temp_table = %d\n", idx_temp_table);
	WMT_DBG_FUNC("[Thermal] now.time = %d, query.time = %d, REFRESH_TIME = %d\n", now_time.tv_sec,
		     query_time.tv_sec, REFRESH_TIME);

	WMT_DBG_FUNC("[0] = %d, [1] = %d, [2] = %d, [3] = %d, [4] = %d\n----\n",
		     temp_table[0], temp_table[1], temp_table[2], temp_table[3], temp_table[4]);

	return_temp = ((current_temp & 0x80) == 0x0) ? current_temp : (-1) * (current_temp & 0x7f);

	return return_temp;
}

ssize_t WMT_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	INT32 iRet = 0;
	UINT8 wrBuf[NAME_MAX + 1] = { 0 };
	INT32 copySize = (count < NAME_MAX) ? count : NAME_MAX;

	WMT_LOUD_FUNC("count:%d copySize:%d\n", count, copySize);

	if (copySize > 0) {
		if (copy_from_user(wrBuf, buf, copySize)) {
			iRet = -EFAULT;
			goto write_done;
		}
		iRet = copySize;
		wrBuf[NAME_MAX] = '\0';

		if (!strncasecmp(wrBuf, "ok", NAME_MAX)) {
			WMT_DBG_FUNC("resp str ok\n");
			/* pWmtDevCtx->cmd_result = 0; */
			wmt_lib_trigger_cmd_signal(0);
		} else {
			WMT_WARN_FUNC("warning resp str (%s)\n", wrBuf);
			/* pWmtDevCtx->cmd_result = -1; */
			wmt_lib_trigger_cmd_signal(-1);
		}
		/* complete(&pWmtDevCtx->cmd_comp); */
	}

write_done:
	return iRet;
}

ssize_t WMT_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	INT32 iRet = 0;
	PUINT8 pCmd = NULL;
	UINT32 cmdLen = 0;

	pCmd = wmt_lib_get_cmd();

	if (pCmd != NULL) {
		cmdLen = osal_strlen(pCmd) < NAME_MAX ? osal_strlen(pCmd) : NAME_MAX;
		if (cmdLen > count)
			cmdLen = count;
		WMT_DBG_FUNC("cmd str(%s)\n", pCmd);
		if (copy_to_user(buf, pCmd, cmdLen))
			iRet = -EFAULT;
		else
			iRet = cmdLen;
	}
#if 0
	if (test_and_clear_bit(WMT_STAT_CMD, &pWmtDevCtx->state)) {
		iRet = osal_strlen(localBuf) < NAME_MAX ? osal_strlen(localBuf) : NAME_MAX;
		/* we got something from STP driver */
		WMT_DBG_FUNC("copy cmd to user by read:%s\n", localBuf);
		if (copy_to_user(buf, localBuf, iRet)) {
			iRet = -EFAULT;
			goto read_done;
		}
	}
#endif

	return iRet;
}

UINT32 WMT_poll(struct file *filp, poll_table *wait)
{
	UINT32 mask = 0;
	P_OSAL_EVENT pEvent = wmt_lib_get_cmd_event();

	poll_wait(filp, &pEvent->waitQueue, wait);
	/* empty let select sleep */
	if (wmt_lib_get_cmd_status() == MTK_WCN_BOOL_TRUE)
		mask |= POLLIN | POLLRDNORM;	/* readable */
#if 0
	if (test_bit(WMT_STAT_CMD, &pWmtDevCtx->state))
		mask |= POLLIN | POLLRDNORM;	/* readable */
#endif
	mask |= POLLOUT | POLLWRNORM;	/* writable */

	return mask;
}

/* INT32 WMT_ioctl(struct inode *inode, struct file *filp, UINT32 cmd, unsigned long arg) */
LONG WMT_unlocked_ioctl(struct file *filp, UINT32 cmd, ULONG arg)
{
	INT32 iRet = 0;
	UINT8 *pBuffer = NULL;

	WMT_DBG_FUNC("cmd (%u), arg (0x%lx)\n", cmd, arg);
	switch (cmd) {
	case WMT_IOCTL_SET_PATCH_NAME:	/* patch location */
		{
			pBuffer = kmalloc(NAME_MAX + 1, GFP_KERNEL);
			if (!pBuffer) {
				WMT_ERR_FUNC("pBuffer kmalloc memory fail\n");
				return 0;
			}
			if (copy_from_user(pBuffer, (PVOID)arg, NAME_MAX)) {
				iRet = -EFAULT;
				kfree(pBuffer);
				break;
			}
			pBuffer[NAME_MAX] = '\0';
			wmt_lib_set_patch_name(pBuffer);
			kfree(pBuffer);
		}
		break;
	case WMT_IOCTL_SET_STP_MODE:	/* stp/hif/fm mode */
		/* set hif conf */
		do {
			P_OSAL_OP pOp;
			MTK_WCN_BOOL bRet;
			P_OSAL_SIGNAL pSignal = NULL;
			P_WMT_HIF_CONF pHif = NULL;

			iRet = wmt_lib_set_hif(arg);
			if (iRet != 0) {
				WMT_INFO_FUNC("wmt_lib_set_hif fail (%lu)\n", arg);
				break;
			}

			pOp = wmt_lib_get_free_op();
			if (!pOp) {
				WMT_INFO_FUNC("get_free_lxop fail\n");
				break;
			}
			pSignal = &pOp->signal;
			pOp->op.opId = WMT_OPID_HIF_CONF;

			pHif = wmt_lib_get_hif();

			osal_memcpy(&pOp->op.au4OpData[0], pHif, sizeof(WMT_HIF_CONF));
			pOp->op.u4InfoBit = WMT_OP_HIF_BIT;
			pSignal->timeoutValue = 0;

			bRet = wmt_lib_put_act_op(pOp);
			WMT_DBG_FUNC("WMT_OPID_HIF_CONF result(%d)\n", bRet);
			iRet = (bRet == MTK_WCN_BOOL_FALSE) ? -EFAULT : 0;
			if (iRet == 0) {
				WMT_INFO_FUNC("luncher set STP mode success!\n");
				hif_info = 1;
			}
		} while (0);
		break;
	case WMT_IOCTL_FUNC_ONOFF_CTRL:	/* test turn on/off func */
		do {
			MTK_WCN_BOOL bRet = MTK_WCN_BOOL_FALSE;

			if (arg & 0x80000000) {
				bRet = mtk_wcn_wmt_func_on(arg & 0xF);
				if (bRet)
					currentLpbkStatus = 1;
			} else
				bRet = mtk_wcn_wmt_func_off(arg & 0xF);

			iRet = (bRet == MTK_WCN_BOOL_FALSE) ? -EFAULT : 0;
		} while (0);
		break;
	case WMT_IOCTL_LPBK_POWER_CTRL:
			do {
				MTK_WCN_BOOL bRet = MTK_WCN_BOOL_FALSE;

			if (arg)
				bRet = mtk_wcn_wmt_func_on(WMTDRV_TYPE_LPBK);
				else
					bRet = mtk_wcn_wmt_func_off(WMTDRV_TYPE_LPBK);
				iRet = (bRet == MTK_WCN_BOOL_FALSE) ? -EFAULT : 0;
		} while (0);
		break;
	case WMT_IOCTL_LPBK_TEST:
		do {
			P_OSAL_OP pOp;
			MTK_WCN_BOOL bRet;
			UINT32 u4Wait;
			/* UINT8 lpbk_buf[1024] = {0}; */
			UINT32 effectiveLen = 0;
			P_OSAL_SIGNAL pSignal = NULL;

			if (copy_from_user(&effectiveLen, (PVOID)arg, sizeof(effectiveLen))) {
				iRet = -EFAULT;
				WMT_ERR_FUNC("copy_from_user failed at %d\n", __LINE__);
				break;
			}
			if (effectiveLen > sizeof(gLpbkBuf)) {
				iRet = -EFAULT;
				WMT_ERR_FUNC("length is too long\n");
				break;
			}
			WMT_DBG_FUNC("len = %d\n", effectiveLen);

			pOp = wmt_lib_get_free_op();
			if (!pOp) {
				WMT_WARN_FUNC("get_free_lxop fail\n");
				iRet = -EFAULT;
				break;
			}
			u4Wait = 2000;
			if (copy_from_user(&gLpbkBuf[0], (PVOID)arg + sizeof(ULONG), effectiveLen)) {
				WMT_ERR_FUNC("copy_from_user failed at %d\n", __LINE__);
				iRet = -EFAULT;
				break;
			}
			pSignal = &pOp->signal;
			pOp->op.opId = WMT_OPID_LPBK;
			pOp->op.au4OpData[0] = effectiveLen;	/* packet length */
			pOp->op.au4OpData[1] = (SIZE_T)&gLpbkBuf[0];	/* packet buffer pointer */
			memcpy(&gLpbkBufLog, &gLpbkBuf[((effectiveLen >= 4) ? effectiveLen - 4 : 0)], 4);
			pSignal->timeoutValue = MAX_EACH_WMT_CMD;
			WMT_INFO_FUNC("OPID(%d) type(%d) start\n", pOp->op.opId, pOp->op.au4OpData[0]);
			if (DISABLE_PSM_MONITOR()) {
				WMT_ERR_FUNC("wake up failed,OPID(%d) type(%d) abort\n",
					     pOp->op.opId, pOp->op.au4OpData[0]);
				wmt_lib_put_op_to_free_queue(pOp);
				iRet = -1;
				break;
			}

			bRet = wmt_lib_put_act_op(pOp);
			ENABLE_PSM_MONITOR();
			if (bRet == MTK_WCN_BOOL_FALSE) {
				WMT_WARN_FUNC("OPID(%d) type(%d) buf tail(0x%08x) fail\n",
					      pOp->op.opId, pOp->op.au4OpData[0], gLpbkBufLog);
				iRet = -1;
				break;
			}
			WMT_INFO_FUNC("OPID(%d) length(%d) ok\n", pOp->op.opId, pOp->op.au4OpData[0]);
			iRet = pOp->op.au4OpData[0];
			if (copy_to_user((PVOID)arg + sizeof(SIZE_T) + sizeof(UINT8[2048]), gLpbkBuf, iRet)) {
				iRet = -EFAULT;
				break;
			}

		} while (0);
		break;
	case WMT_IOCTL_ADIE_LPBK_TEST:
		do {
			P_OSAL_OP pOp;
			MTK_WCN_BOOL bRet;
			P_OSAL_SIGNAL pSignal = NULL;

			pOp = wmt_lib_get_free_op();
			if (!pOp) {
				WMT_WARN_FUNC("get_free_lxop fail\n");
				iRet = -EFAULT;
				break;
			}

			pSignal = &pOp->signal;
			pOp->op.opId = WMT_OPID_ADIE_LPBK_TEST;
			pOp->op.au4OpData[0] = 0;
			pOp->op.au4OpData[1] = (SIZE_T)&gLpbkBuf[0];
			pSignal->timeoutValue = MAX_EACH_WMT_CMD;
			WMT_INFO_FUNC("OPID(%d) start\n", pOp->op.opId);
			if (DISABLE_PSM_MONITOR()) {
				WMT_ERR_FUNC("wake up failed,OPID(%d)abort\n", pOp->op.opId);
				wmt_lib_put_op_to_free_queue(pOp);
				return -1;
			}

			bRet = wmt_lib_put_act_op(pOp);
			ENABLE_PSM_MONITOR();
			if (bRet == MTK_WCN_BOOL_FALSE) {
				WMT_WARN_FUNC("OPID(%d) fail\n", pOp->op.opId);
				iRet = -1;
				break;
			}
			WMT_INFO_FUNC("OPID(%d) length(%d) ok\n", pOp->op.opId, pOp->op.au4OpData[0]);
			iRet = pOp->op.au4OpData[0];
			if (copy_to_user((PVOID)arg + sizeof(SIZE_T), gLpbkBuf, iRet)) {
				iRet = -EFAULT;
				break;
			}
		} while (0);
		break;
	case 10:
		pBuffer = kmalloc(NAME_MAX + 1, GFP_KERNEL);
		if (!pBuffer) {
			WMT_ERR_FUNC("pBuffer kmalloc memory fail\n");
			return 0;
		}
		wmt_lib_host_awake_get();
		osal_strcpy(pBuffer, "MT662x f/w coredump start-");
		if (copy_from_user(pBuffer + osal_strlen(pBuffer), (PVOID)arg,
					NAME_MAX - osal_strlen(pBuffer))) {
			/* osal_strcpy(pBuffer, "MT662x f/w assert core dump start"); */
			WMT_ERR_FUNC("copy assert string failed\n");
		}
		pBuffer[NAME_MAX] = '\0';
		osal_dbg_assert_aee(pBuffer, pBuffer);
		kfree(pBuffer);
		break;
	case 11:
		/*osal_dbg_assert_aee("MT662x f/w coredump end", "MT662x firmware coredump ends");*/
		wmt_lib_host_awake_put();
		break;
	case WMT_IOCTL_GET_CHIP_INFO:
		if (arg == 0)
			return wmt_lib_get_icinfo(WMTCHIN_CHIPID);
		else if (arg == 1)
			return wmt_lib_get_icinfo(WMTCHIN_HWVER);
		else if (arg == 2)
			return wmt_lib_get_icinfo(WMTCHIN_FWVER);
		break;
	case WMT_IOCTL_WMT_CFG_NAME:
		{
			INT8 cWmtCfgName[NAME_MAX + 1];

			if (copy_from_user(cWmtCfgName, (void *)arg, NAME_MAX)) {
				iRet = -EFAULT;
				break;
			}
			cWmtCfgName[NAME_MAX] = '\0';
			wmt_conf_set_cfg_file(cWmtCfgName);
		}
		break;
	case WMT_IOCTL_SET_LAUNCHER_KILL:
		if (arg == 1) {
			WMT_INFO_FUNC("launcher may be killed,block abnormal stp tx.\n");
			wmt_lib_set_stp_wmt_last_close(1);
		} else
			wmt_lib_set_stp_wmt_last_close(0);
		break;
	case WMT_IOCTL_SET_PATCH_NUM:
		if (arg == 0 || arg > MAX_PATCH_NUM) {
			WMT_ERR_FUNC("patch num(%d) == 0 or > %d!\n", arg, MAX_PATCH_NUM);
			iRet = -1;
			break;
		}
		pAtchNum = arg;

		pPatchInfo = kcalloc(pAtchNum, sizeof(WMT_PATCH_INFO), GFP_ATOMIC);
		if (!pPatchInfo) {
			WMT_ERR_FUNC("allocate memory fail!\n");
			iRet = -EFAULT;
			break;
		}

		WMT_DBG_FUNC(" get patch num from launcher = %d\n", pAtchNum);
		wmt_lib_set_patch_num(pAtchNum);
		break;
	case WMT_IOCTL_SET_PATCH_INFO:
		do {
			WMT_PATCH_INFO wMtPatchInfo;
			P_WMT_PATCH_INFO pTemp = NULL;
			UINT32 dWloadSeq;
			static UINT32 counter;

			if (!pPatchInfo) {
				WMT_ERR_FUNC("NULL patch info pointer\n");
				break;
			}

			if (copy_from_user(&wMtPatchInfo, (PVOID)arg, sizeof(WMT_PATCH_INFO))) {
				WMT_ERR_FUNC("copy_from_user failed at %d\n", __LINE__);
				iRet = -EFAULT;
				break;
			}
			if (wMtPatchInfo.dowloadSeq > pAtchNum) {
				WMT_ERR_FUNC("dowloadSeq num(%d) > %d!\n", wMtPatchInfo.dowloadSeq, pAtchNum);
				iRet = -EFAULT;
				counter = 0;
				break;
			}

			dWloadSeq = wMtPatchInfo.dowloadSeq;
			WMT_DBG_FUNC(
				"patch dl seq %d,name %s,address info 0x%02x,0x%02x,0x%02x,0x%02x\n",
			     dWloadSeq, wMtPatchInfo.patchName,
			     wMtPatchInfo.addRess[0],
			     wMtPatchInfo.addRess[1],
			     wMtPatchInfo.addRess[2],
			     wMtPatchInfo.addRess[3]);
			osal_memcpy(pPatchInfo + dWloadSeq - 1, &wMtPatchInfo, sizeof(WMT_PATCH_INFO));
			pTemp = pPatchInfo + dWloadSeq - 1;
			if (++counter == pAtchNum) {
				wmt_lib_set_patch_info(pPatchInfo);
				counter = 0;
			}
		} while (0);
		break;
	case WMT_IOCTL_WMT_COREDUMP_CTRL:
		mtk_wcn_stp_coredump_flag_ctrl(arg);
		break;
	case WMT_IOCTL_WMT_STP_ASSERT_CTRL:
		if (wmt_lib_btm_cb(BTM_TRIGGER_STP_ASSERT_OP) == MTK_WCN_BOOL_TRUE) {
			WMT_INFO_FUNC("trigger stp assert succeed\n");
			iRet = 0;
		} else {
			WMT_INFO_FUNC("trigger stp assert failed\n");
			iRet = -1;
		}
		break;
	case WMT_IOCTL_WMT_QUERY_CHIPID:
		iRet = mtk_wcn_wmt_chipid_query();
		WMT_WARN_FUNC("chipid = 0x%x\n", iRet);
		break;
	case WMT_IOCTL_WMT_TELL_CHIPID:
		{
#if !(DELETE_HIF_SDIO_CHRDEV)
			iRet = mtk_wcn_hif_sdio_tell_chipid(arg);
#endif
			if (0x6628 == arg || 0x6630 == arg || 0x6632 == arg)
				wmt_lib_merge_if_flag_ctrl(1);
			else
				wmt_lib_merge_if_flag_ctrl(0);
		}
		break;
	case WMT_IOCTL_SEND_BGW_DS_CMD:
		do {
			P_OSAL_OP pOp;
			MTK_WCN_BOOL bRet;
			UINT8 desense_buf[14] = { 0 };
			UINT32 effectiveLen = 14;
			P_OSAL_SIGNAL pSignal = NULL;

			pOp = wmt_lib_get_free_op();
			if (!pOp) {
				WMT_WARN_FUNC("get_free_lxop fail\n");
				iRet = -EFAULT;
				break;
			}
			if (copy_from_user(&desense_buf[0], (PVOID)arg, effectiveLen)) {
				WMT_ERR_FUNC("copy_from_user failed at %d\n", __LINE__);
				iRet = -EFAULT;
				break;
			}
			pSignal = &pOp->signal;
			pOp->op.opId = WMT_OPID_BGW_DS;
			pOp->op.au4OpData[0] = effectiveLen;	/* packet length */
			pOp->op.au4OpData[1] = (SIZE_T)&desense_buf[0];	/* packet buffer pointer */
			pSignal->timeoutValue = MAX_EACH_WMT_CMD;
			WMT_INFO_FUNC("OPID(%d) start\n", pOp->op.opId);
			if (DISABLE_PSM_MONITOR()) {
				WMT_ERR_FUNC("wake up failed,opid(%d) abort\n", pOp->op.opId);
				wmt_lib_put_op_to_free_queue(pOp);
				return -1;
			}

			bRet = wmt_lib_put_act_op(pOp);
			ENABLE_PSM_MONITOR();
			if (bRet == MTK_WCN_BOOL_FALSE) {
				WMT_WARN_FUNC("OPID(%d) fail\n", pOp->op.opId);
				iRet = -1;
				break;
			}
			WMT_INFO_FUNC("OPID(%d) length(%d) ok\n", pOp->op.opId, pOp->op.au4OpData[0]);
			iRet = pOp->op.au4OpData[0];

		} while (0);
		break;
	case WMT_IOCTL_FW_DBGLOG_CTRL:
		iRet = wmt_plat_set_dbg_mode(arg);
		if (iRet == 0)
			wmt_dbg_fwinfor_from_emi(0, 1, 0);
		break;
	case WMT_IOCTL_DYNAMIC_DUMP_CTRL:
		do {
			UINT32 i = 0, j = 0, k = 0;
			PUINT8 pBuf = NULL;
			UINT32 int_buf[10];
			INT8 Buffer[10][11];

			pBuf = kmalloc(DYNAMIC_DUMP_BUF + 1, GFP_KERNEL);
			if (!pBuf) {
				WMT_ERR_FUNC("pBuf kmalloc memory fail\n");
				return 0;
			}
			if (copy_from_user(pBuf, (PVOID)arg, DYNAMIC_DUMP_BUF)) {
				iRet = -EFAULT;
				kfree(pBuf);
				break;
			}
			pBuf[DYNAMIC_DUMP_BUF] = '\0';
			WMT_INFO_FUNC("get dynamic dump data from property(%s)\n", pBuf);
			memset(Buffer, 0, 10*11);
			for (i = 0; i < DYNAMIC_DUMP_BUF; i++) {
				if (pBuf[i] == '/') {
					k = 0;
					j++;
				} else {
					Buffer[j][k] = pBuf[i];
					k++;
				}
			}
			for (i = 0; i < (j > 10 ? 10 : j); i++) {
				iRet = kstrtou32(Buffer[i], 0, &int_buf[i]);
				if (iRet) {
					WMT_ERR_FUNC("string convert fail(%d)\n", iRet);
					break;
				}
				WMT_INFO_FUNC("dynamic dump data buf[%d]:(0x%x)\n", j, int_buf[j]);
			}
			wmt_plat_set_dynamic_dumpmem(int_buf);
			kfree(pBuf);
		} while (0);
		break;
	default:
		iRet = -EINVAL;
		WMT_WARN_FUNC("unknown cmd (%d)\n", cmd);
		break;
	}

	return iRet;
}

#ifdef CONFIG_COMPAT
LONG WMT_compat_ioctl(struct file *filp, UINT32 cmd, ULONG arg)
{
	LONG ret;

	WMT_INFO_FUNC("cmd[0x%x]\n", cmd);
		switch (cmd) {
		case COMPAT_WMT_IOCTL_SET_PATCH_NAME:
			ret = WMT_unlocked_ioctl(filp, WMT_IOCTL_SET_PATCH_NAME, (ULONG)compat_ptr(arg));
			break;
		case COMPAT_WMT_IOCTL_LPBK_TEST:
			ret = WMT_unlocked_ioctl(filp, WMT_IOCTL_LPBK_TEST, (ULONG)compat_ptr(arg));
			break;
		case COMPAT_WMT_IOCTL_SET_PATCH_INFO:
			ret = WMT_unlocked_ioctl(filp, WMT_IOCTL_SET_PATCH_INFO, (ULONG)compat_ptr(arg));
			break;
		case COMPAT_WMT_IOCTL_PORT_NAME:
			ret = WMT_unlocked_ioctl(filp, WMT_IOCTL_PORT_NAME, (ULONG)compat_ptr(arg));
			break;
		case COMPAT_WMT_IOCTL_WMT_CFG_NAME:
			ret = WMT_unlocked_ioctl(filp, WMT_IOCTL_WMT_CFG_NAME, (ULONG)compat_ptr(arg));
			break;
		case COMPAT_WMT_IOCTL_SEND_BGW_DS_CMD:
			ret = WMT_unlocked_ioctl(filp, WMT_IOCTL_SEND_BGW_DS_CMD, (ULONG)compat_ptr(arg));
			break;
		default: {
			ret = WMT_unlocked_ioctl(filp, cmd, arg);
			break;
			}
		}
	return ret;
}
#endif /* CONFIG_COMPAT */

static INT32 WMT_open(struct inode *inode, struct file *file)
{
	LONG ret;

	WMT_INFO_FUNC("major %d minor %d (pid %d)\n", imajor(inode), iminor(inode), current->pid);
	ret = wait_event_timeout(gWmtInitWq, gWmtInitDone != 0, msecs_to_jiffies(WMT_DEV_INIT_TO_MS));
	if (!ret) {
		WMT_WARN_FUNC("wait_event_timeout (%d)ms,(%d)jiffies,return -EIO\n",
			      WMT_DEV_INIT_TO_MS, msecs_to_jiffies(WMT_DEV_INIT_TO_MS));
		return -EIO;
	}

	if (atomic_inc_return(&gWmtRefCnt) == 1)
		WMT_INFO_FUNC("1st call\n");

	return 0;
}

static INT32 WMT_close(struct inode *inode, struct file *file)
{
	WMT_INFO_FUNC("major %d minor %d (pid %d)\n", imajor(inode), iminor(inode), current->pid);

	if (atomic_dec_return(&gWmtRefCnt) == 0)
		WMT_INFO_FUNC("last call\n");

	return 0;
}

const struct file_operations gWmtFops = {
	.open = WMT_open,
	.release = WMT_close,
	.read = WMT_read,
	.write = WMT_write,
/* .ioctl = WMT_ioctl, */
	.unlocked_ioctl = WMT_unlocked_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = WMT_compat_ioctl,
#endif
	.poll = WMT_poll,
};

VOID wmt_dev_bgw_desense_init(VOID)
{
	bgw_init_socket();
}

VOID wmt_dev_bgw_desense_deinit(VOID)
{
	bgw_destroy_netlink_kernel();
}

VOID wmt_dev_send_cmd_to_daemon(UINT32 cmd)
{
	send_command_to_daemon(cmd);
}

static INT32 WMT_init(VOID)
{
	dev_t devID = MKDEV(gWmtMajor, 0);
	INT32 cdevErr = -1;
	INT32 ret = -1;
	ENUM_WMT_CHIP_TYPE chip_type;

	WMT_INFO_FUNC("WMT Version= %s DATE=%s\n", MTK_WMT_VERSION, MTK_WMT_DATE);
	/* Prepare a UINT8 device */
	/*static allocate chrdev */
	gWmtInitDone = 0;
	init_waitqueue_head((wait_queue_head_t *) &gWmtInitWq);

	stp_drv_init();

	ret = register_chrdev_region(devID, WMT_DEV_NUM, WMT_DRIVER_NAME);
	if (ret) {
		WMT_ERR_FUNC("fail to register chrdev\n");
		return ret;
	}

	cdev_init(&gWmtCdev, &gWmtFops);
	gWmtCdev.owner = THIS_MODULE;

	cdevErr = cdev_add(&gWmtCdev, devID, WMT_DEV_NUM);
	if (cdevErr) {
		WMT_ERR_FUNC("cdev_add() fails (%d)\n", cdevErr);
		goto error;
	}
	WMT_INFO_FUNC("driver(major %d) installed\n", gWmtMajor);

#if WMT_CREATE_NODE_DYNAMIC
	wmt_class = class_create(THIS_MODULE, "stpwmt");
	if (IS_ERR(wmt_class))
		goto error;
	wmt_dev = device_create(wmt_class, NULL, devID, NULL, "stpwmt");
	if (IS_ERR(wmt_dev))
		goto error;
#endif

#if 0
	pWmtDevCtx = wmt_drv_create();
	if (!pWmtDevCtx) {
		WMT_ERR_FUNC("wmt_drv_create() fails\n");
		goto error;
	}

	ret = wmt_drv_init(pWmtDevCtx);
	if (ret) {
		WMT_ERR_FUNC("wmt_drv_init() fails (%d)\n", ret);
		goto error;
	}

	WMT_INFO_FUNC("stp_btmcb_reg\n");
	wmt_cdev_btmcb_reg();

	ret = wmt_drv_start(pWmtDevCtx);
	if (ret) {
		WMT_ERR_FUNC("wmt_drv_start() fails (%d)\n", ret);
		goto error;
	}
#endif
	ret = wmt_lib_init();
	if (ret) {
		WMT_ERR_FUNC("wmt_lib_init() fails (%d)\n", ret);
		goto error;
	}
#if CFG_WMT_DBG_SUPPORT
	wmt_dev_dbg_setup();
#endif

#if CFG_WMT_PROC_FOR_AEE
	wmt_dev_proc_for_aee_setup();
#endif

	chip_type = wmt_detect_get_chip_type();
	if (chip_type == WMT_CHIP_TYPE_COMBO)
		mtk_wcn_hif_sdio_update_cb_reg(wmt_dev_tra_sdio_update);

	WMT_DBG_FUNC("wmt_dev register thermal cb\n");
	wmt_lib_register_thermal_ctrl_cb(wmt_dev_tm_temp_query);

	if (chip_type == WMT_CHIP_TYPE_SOC)
		wmt_dev_bgw_desense_init();

	gWmtInitDone = 1;
	wake_up(&gWmtInitWq);

	osal_sleepable_lock_init(&g_es_lr_lock);
	INIT_WORK(&gPwrOnOffWork, wmt_pwr_on_off_handler);
#ifdef CONFIG_EARLYSUSPEND
	register_early_suspend(&wmt_early_suspend_handler);
	WMT_INFO_FUNC("register_early_suspend finished\n");
#else
	wmt_fb_notifier.notifier_call = wmt_fb_notifier_callback;
	ret = fb_register_client(&wmt_fb_notifier);
	if (ret)
		WMT_ERR_FUNC("wmt register fb_notifier failed! ret(%d)\n", ret);
	else
		WMT_INFO_FUNC("wmt register fb_notifier OK!\n");
#endif /* CONFIG_EARLYSUSPEND */
	WMT_INFO_FUNC("success\n");

	return 0;

error:
	wmt_lib_deinit();
#if CFG_WMT_DBG_SUPPORT
	wmt_dev_dbg_remove();
#endif
#if WMT_CREATE_NODE_DYNAMIC
	if (!(IS_ERR(wmt_dev)))
		device_destroy(wmt_class, devID);
	if (!(IS_ERR(wmt_class))) {
		class_destroy(wmt_class);
		wmt_class = NULL;
	}
#endif

	if (cdevErr == 0)
		cdev_del(&gWmtCdev);

	if (ret == 0) {
		unregister_chrdev_region(devID, WMT_DEV_NUM);
		gWmtMajor = -1;
	}

	WMT_ERR_FUNC("fail\n");

	return -1;
}

static VOID WMT_exit(VOID)
{
	dev_t dev = MKDEV(gWmtMajor, 0);

	osal_sleepable_lock_deinit(&g_es_lr_lock);
#ifdef CONFIG_EARLYSUSPEND
	unregister_early_suspend(&wmt_early_suspend_handler);
	WMT_INFO_FUNC("unregister_early_suspend finished\n");
#else
	fb_unregister_client(&wmt_fb_notifier);
#endif /* CONFIG_EARLYSUSPEND */

	wmt_dev_bgw_desense_deinit();

	wmt_lib_register_thermal_ctrl_cb(NULL);

	wmt_lib_deinit();

#if CFG_WMT_DBG_SUPPORT
	wmt_dev_dbg_remove();
#endif

#if CFG_WMT_PROC_FOR_AEE
	wmt_dev_proc_for_aee_remove();
#endif
#if WMT_CREATE_NODE_DYNAMIC
	if (wmt_dev) {
		device_destroy(wmt_class, dev);
		wmt_dev = NULL;
	}
	if (wmt_class) {
		class_destroy(wmt_class);
		wmt_class = NULL;
	}
#endif
	cdev_del(&gWmtCdev);
	unregister_chrdev_region(dev, WMT_DEV_NUM);
	gWmtMajor = -1;

	stp_drv_exit();

	WMT_INFO_FUNC("done\n");
}

#ifdef MTK_WCN_REMOVE_KERNEL_MODULE

INT32 mtk_wcn_common_drv_init(VOID)
{
	return WMT_init();
}
EXPORT_SYMBOL(mtk_wcn_common_drv_init);
VOID mtk_wcn_common_drv_exit(VOID)
{
	WMT_exit();
}
EXPORT_SYMBOL(mtk_wcn_common_drv_exit);

#else
module_init(WMT_init);
module_exit(WMT_exit);
#endif
MODULE_LICENSE("GPL");
MODULE_AUTHOR("MediaTek Inc WCN");
MODULE_DESCRIPTION("MTK WCN common driver for WMT function");

module_param(gWmtMajor, uint, 0);
