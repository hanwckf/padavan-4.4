 /*
  * Copyright (C) 2016 MediaTek Inc.
  *
  * MTK Direct Charge Vdm Driver
  * Author: Sakya <jeff_chang@richtek.com>
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

#include <linux/err.h>
#include <linux/delay.h>
#include <linux/wakelock.h>
#ifdef CONFIG_DEBUG_FS
#include <linux/debugfs.h>
#endif /* CONFIG_DEBUG_FS */
#include <linux/usb/class-dual-role.h>

#include "inc/mtk_direct_charge_vdm.h"
#include "inc/tcpm.h"

#define MTK_VDM_COUNT	(20)
#define MTK_VDM_DELAY	(10)

#define RT7207_VDM_HDR	(0x29CF << 16)

static struct tcpc_device *tcpc;
static struct dual_role_phy_instance *dr_usb;
static atomic_t vdm_event_flag;
static struct notifier_block vdm_nb;
static struct mutex vdm_event_lock;
static struct mutex vdm_par_lock;
static struct wake_lock vdm_event_wake_lock;
static bool vdm_inited;
static uint32_t vdm_payload[7];
static bool vdm_success;

static int vdm_tcp_notifier_call(struct notifier_block *nb,
					unsigned long event, void *data)
{
	struct tcp_notify *noti = data;
	int i;

	switch (event) {
	case TCP_NOTIFY_UVDM:
		if (atomic_read(&vdm_event_flag)) {
			mutex_lock(&vdm_par_lock);
			if (noti->uvdm_msg.ack) {
				vdm_success = true;
				for (i = 0; i < 7; i++) {
					vdm_payload[i] =
						noti->uvdm_msg.uvdm_data[i];
				}
			} else
				vdm_success = false;
			mutex_unlock(&vdm_par_lock);
			atomic_dec(&vdm_event_flag); /* set flag = 0 */
		}
		break;
	default:
		break;
	}
	return NOTIFY_OK;
}

static void mtk_vdm_lock(void)
{
	wake_lock(&vdm_event_wake_lock);
	mutex_lock(&vdm_event_lock);
}

static void mtk_vdm_unlock(void)
{
	mutex_unlock(&vdm_event_lock);
	wake_unlock(&vdm_event_wake_lock);
}

int mtk_get_ta_id(struct tcpc_device *tcpc)
{
	int count = MTK_VDM_COUNT;
	uint32_t data[7] = {0};
	int id = 0;

	if (!vdm_inited) {
		pr_err("%s vdm not inited\n", __func__);
		return -EINVAL;
	}

	if (!mtk_is_pep30_en_unlock()) {
		pr_err("%s pep30 is not ready\n", __func__);
		return -EINVAL;
	}

	data[0] = RT7207_VDM_HDR | 0x1012;
	mtk_vdm_lock();
	atomic_inc(&vdm_event_flag); /* set flag = 1 */
	tcpm_send_uvdm(tcpc, 1, data, true);
	while (count) {
		if (atomic_read(&vdm_event_flag) == 0) {
			mutex_lock(&vdm_par_lock);
			if (vdm_success)
				id = vdm_payload[1];
			else
				id = -1;
			pr_err("%s id = 0x%x\n", __func__, id);
			mutex_unlock(&vdm_par_lock);
			mtk_vdm_unlock();
			return id;
		}
		count--;
		mdelay(MTK_VDM_DELAY);
	}
	atomic_dec(&vdm_event_flag);
	mtk_vdm_unlock();
	if (!tcpm_get_uvdm_handle_flag(tcpc)) {
		pr_err("%s Sw Busy Time out\n", __func__);
		return MTK_VDM_SW_BUSY;
	}
	pr_err("%s Time out\n", __func__);
	return MTK_VDM_FAIL;
}

int mtk_get_ta_charger_status(struct tcpc_device *tcpc, struct pd_ta_stat *ta)
{
	int count = MTK_VDM_COUNT;
	uint32_t data[7] = {0};
	int status = MTK_VDM_FAIL;

	if (!vdm_inited) {
		pr_err("%s vdm not inited\n", __func__);
		return -EINVAL;
	}
	if (!mtk_is_pep30_en_unlock()) {
		pr_err("%s pep30 is not ready\n", __func__);
		return -EINVAL;
	}

	data[0] = RT7207_VDM_HDR | 0x101a;

	mtk_vdm_lock();
	atomic_inc(&vdm_event_flag); /* set flag = 1 */
	tcpm_send_uvdm(tcpc, 1, data, true);
	while (count) {
		if (atomic_read(&vdm_event_flag) == 0) {
			mutex_lock(&vdm_par_lock);
			if (vdm_success) {
				ta->chg_mode = (vdm_payload[1]&0x80000000) ?
					RT7207_CC_MODE : RT7207_CV_MODE;
				ta->dc_en =
					(vdm_payload[1]&0x40000000) ? 1 : 0;
				ta->dpc_en =
					(vdm_payload[1]&0x20000000) ? 1 : 0;
				ta->pc_en =
					(vdm_payload[1]&0x10000000) ? 1 : 0;
				ta->ovp =
					(vdm_payload[1]&0x00000001) ? 1 : 0;
				ta->otp =
					(vdm_payload[1]&0x00000002) ? 1 : 0;
				ta->uvp =
					(vdm_payload[1]&0x00000004) ? 1 : 0;
				ta->rvs_cur =
					(vdm_payload[1]&0x00000008) ? 1 : 0;
				ta->ping_chk_fail =
					(vdm_payload[1]&0x00000010) ? 1 : 0;
				pr_err("%s %s), dc(%d), pdc(%d), pc(%d) ovp(%d), otp(%d) uvp(%d) rc(%d), pf(%d)\n",
					__func__, ta->chg_mode > 0 ? "CC Mode" : "CV Mode",
					ta->dc_en, ta->dpc_en, ta->pc_en, ta->ovp, ta->otp,
					ta->uvp, ta->rvs_cur, ta->ping_chk_fail);
				status = MTK_VDM_SUCCESS;
			} else {
				pr_err("%s Failed\n", __func__);
				status = MTK_VDM_FAIL;
			}
			mutex_unlock(&vdm_par_lock);
			mtk_vdm_unlock();
			return status;
		}
		count--;
		mdelay(MTK_VDM_DELAY);
	}
	atomic_dec(&vdm_event_flag);
	mtk_vdm_unlock();
	if (!tcpm_get_uvdm_handle_flag(tcpc)) {
		pr_err("%s Sw Busy Time out\n", __func__);
		return MTK_VDM_SW_BUSY;
	}
	pr_err("%s Time out\n", __func__);
	return MTK_VDM_FAIL;
}

int mtk_get_ta_current_cap(struct tcpc_device *tcpc,
					struct mtk_vdm_ta_cap *cap)
{
	int count = MTK_VDM_COUNT;
	int status = MTK_VDM_FAIL;
	uint32_t data[7] = {0};

	if (!vdm_inited) {
		pr_err("%s vdm not inited\n", __func__);
		return -EINVAL;
	}
	if (!mtk_is_pep30_en_unlock()) {
		pr_err("%s pep30 is not ready\n", __func__);
		return -EINVAL;
	}

	data[0] = RT7207_VDM_HDR | 0x101b;
	mtk_vdm_lock();
	atomic_inc(&vdm_event_flag); /* set flag = 1 */
	tcpm_send_uvdm(tcpc, 1, data, true);
	while (count) {
		if (atomic_read(&vdm_event_flag) == 0) {
			mutex_lock(&vdm_par_lock);
			if (vdm_success) {
				cap->vol = vdm_payload[1]&0x0000ffff;
				cap->cur = (vdm_payload[1]&0xffff0000)>>16;
				status = MTK_VDM_SUCCESS;
				pr_err("%s mv = %dmv,ma = %dma\n",
					__func__, cap->vol, cap->cur);
			} else {
				pr_err("%s Failed\n", __func__);
				status = MTK_VDM_FAIL;
			}
			mutex_unlock(&vdm_par_lock);
			mtk_vdm_unlock();
			return status;
		}
		count--;
		mdelay(MTK_VDM_DELAY);
	}
	atomic_dec(&vdm_event_flag);
	mtk_vdm_unlock();
	if (!tcpm_get_uvdm_handle_flag(tcpc)) {
		pr_err("%s Sw Busy Time out\n", __func__);
		return MTK_VDM_SW_BUSY;
	}
	pr_err("%s Time out\n", __func__);
	return MTK_VDM_FAIL;
}

int mtk_monitor_ta_inform(struct tcpc_device *tcpc, struct mtk_vdm_ta_cap *cap)
{
	int count = MTK_VDM_COUNT;
	int status = MTK_VDM_FAIL;
	uint32_t data[7] = {0};

	if (!vdm_inited) {
		pr_err("%s vdm not inited\n", __func__);
		return -EINVAL;
	}
	if (!mtk_is_pep30_en_unlock()) {
		pr_err("%s pep30 is not ready\n", __func__);
		return -EINVAL;
	}

	data[0] = RT7207_VDM_HDR | 0x101f;
	mtk_vdm_lock();
	atomic_inc(&vdm_event_flag); /* set flag = 1 */
	tcpm_send_uvdm(tcpc, 1, data, true);
	while (count) {
		if (atomic_read(&vdm_event_flag) == 0) {
			mutex_lock(&vdm_par_lock);
			if (vdm_success) {
				status = MTK_VDM_SUCCESS;
				cap->vol = (vdm_payload[1]&0x0000ffff)*5*2700/1023;
				cap->cur = (vdm_payload[1]&0xffff0000)>>16;
				cap->temp = vdm_payload[2];
				pr_err("%s VOL(%d),CUR(%d),TEMP(%d)\n",
					__func__, cap->vol, cap->cur, cap->temp);
			} else {
				pr_err("%s Failed\n", __func__);
				status = MTK_VDM_FAIL;
			}
			mutex_unlock(&vdm_par_lock);
			mtk_vdm_unlock();
			return status;
		}
		count--;
		mdelay(MTK_VDM_DELAY);
	}
	atomic_dec(&vdm_event_flag);
	mtk_vdm_unlock();
	if (!tcpm_get_uvdm_handle_flag(tcpc)) {
		pr_err("%s Sw Busy Time out\n", __func__);
		return MTK_VDM_SW_BUSY;
	}
	pr_err("%s Time out\n", __func__);
	return MTK_VDM_FAIL;
}

int mtk_enable_direct_charge(struct tcpc_device *tcpc, bool en)
{
	struct pd_ta_stat stat;
	int count = MTK_VDM_COUNT, ret;
	int status = MTK_VDM_FAIL;
	uint32_t data[7] = {0};

	if (!vdm_inited) {
		pr_err("%s vdm not inited\n", __func__);
		return -EINVAL;
	}
	if (!mtk_is_pep30_en_unlock()) {
		pr_err("%s pep30 is not ready\n", __func__);
		return -EINVAL;
	}

	ret = mtk_get_ta_charger_status(tcpc, &stat);
	if (ret < 0) {
		pr_err("%s get ta charger status fail\n", __func__);
		return -EINVAL;
	}

	if (en) {
		if (stat.dc_en) {
			pr_err("%s DC already enable\n", __func__);
			return MTK_VDM_SUCCESS;
		}
	} else {
		if (!stat.dc_en) {
			pr_err("%s DC already disable\n", __func__);
			return MTK_VDM_SUCCESS;
		}
	}

	data[0] = RT7207_VDM_HDR | 0x2020;
	data[1] = en ? 0x29CF : 0;

	mtk_vdm_lock();
	atomic_inc(&vdm_event_flag); /* set flag = 1 */
	tcpm_send_uvdm(tcpc, 2, data, true);
	while (count) {
		if (atomic_read(&vdm_event_flag) == 0) {
			mutex_lock(&vdm_par_lock);
			if (vdm_success) {
				pr_info("%s (%d)Success\n", __func__, en);
				status = MTK_VDM_SUCCESS;
			} else {
				pr_err("%s (%d)Failed\n", __func__, en);
				status = MTK_VDM_FAIL;
			}
			mutex_unlock(&vdm_par_lock);
			mtk_vdm_unlock();
			return status;
		}
		count--;
		mdelay(MTK_VDM_DELAY);
	}
	atomic_dec(&vdm_event_flag);
	mtk_vdm_unlock();
	if (!tcpm_get_uvdm_handle_flag(tcpc)) {
		pr_err("%s (%d)Sw Busy Time out\n", __func__, en);
		return MTK_VDM_SW_BUSY;
	}
	pr_err("%s (%d)Time out\n", __func__, en);
	return MTK_VDM_FAIL;
}

int mtk_enable_ta_dplus_dect(struct tcpc_device *tcpc, bool en, int time)
{
	struct pd_ta_stat stat;
	int count = MTK_VDM_COUNT;
	int status = MTK_VDM_FAIL;
	int ret;
	uint32_t data[7] = {0};

	if (!vdm_inited) {
		pr_err("%s vdm not inited\n", __func__);
		return -EINVAL;
	}
	if (!mtk_is_pep30_en_unlock()) {
		pr_err("%s pep30 is not ready\n", __func__);
		return -EINVAL;
	}

	ret = mtk_get_ta_charger_status(tcpc, &stat);
	if (ret < 0) {
		pr_err("%s get ta charger status fail\n", __func__);
		return -EINVAL;
	}

	data[0] = RT7207_VDM_HDR | 0x2020;
	if (en) {
		if (stat.dc_en) {
			data[1] = 0x29CF | (0x3c << 16) | (0x5c << 24);
			data[2] = time;
		} else {
			pr_err("%s DC should enabled\n", __func__);
			return -EINVAL;
		}
	} else {
		if (stat.dc_en)
			data[1] = 0x29CF;
		else {
			pr_err("%s DC already disabled\n", __func__);
			return MTK_VDM_SUCCESS;
		}
	}

	mtk_vdm_lock();
	atomic_inc(&vdm_event_flag); /* set flag = 1 */
	if (en)
		tcpm_send_uvdm(tcpc, 3, data, true);
	else
		tcpm_send_uvdm(tcpc, 2, data, true);

	while (count) {
		if (atomic_read(&vdm_event_flag) == 0) {
			mutex_lock(&vdm_par_lock);
			if (vdm_success) {
				pr_info("%s Success\n", __func__);
				status = MTK_VDM_SUCCESS;
			} else {
				pr_err("%s Failed\n", __func__);
				status = MTK_VDM_FAIL;
			}
			mutex_unlock(&vdm_par_lock);
			mtk_vdm_unlock();
			return status;
		}
		count--;
		mdelay(MTK_VDM_DELAY);
	}
	atomic_dec(&vdm_event_flag);
	mtk_vdm_unlock();
	if (!tcpm_get_uvdm_handle_flag(tcpc)) {
		pr_err("%s Sw Busy Time out\n", __func__);
		return MTK_VDM_SW_BUSY;
	}
	pr_err("%s Time out\n", __func__);
	return MTK_VDM_FAIL;
}

int mtk_get_ta_setting_dac(struct tcpc_device *tcpc,
					struct mtk_vdm_ta_cap *cap)
{
	int count = MTK_VDM_COUNT;
	int status = MTK_VDM_FAIL;
	uint32_t data[7] = {0};

	if (!vdm_inited) {
		pr_err("%s vdm not inited\n", __func__);
		return -EINVAL;
	}
	if (!mtk_is_pep30_en_unlock()) {
		pr_err("%s pep30 is not ready\n", __func__);
		return -EINVAL;
	}

	data[0] = RT7207_VDM_HDR | 0x101c;
	mtk_vdm_lock();
	atomic_inc(&vdm_event_flag); /* set flag = 1 */
	tcpm_send_uvdm(tcpc, 1, data, true);
	while (count) {
		if (atomic_read(&vdm_event_flag) == 0) {
			mutex_lock(&vdm_par_lock);
			if (vdm_success) {
				cap->vol = (vdm_payload[1]&0x0000ffff) * 26;
				cap->cur =
					((vdm_payload[1]&0xffff0000)>>16) * 26;
				status = MTK_VDM_SUCCESS;
				pr_err("%s mv = %dmv,ma = %dma\n",
					__func__, cap->vol, cap->cur);
			} else {
				pr_err("%s Failed\n", __func__);
				status = MTK_VDM_FAIL;
			}
			mutex_unlock(&vdm_par_lock);
			mtk_vdm_unlock();
			return status;
		}
		count--;
		mdelay(MTK_VDM_DELAY);
	}
	atomic_dec(&vdm_event_flag);
	mtk_vdm_unlock();
	if (!tcpm_get_uvdm_handle_flag(tcpc)) {
		pr_err("%s Sw Busy Time out\n", __func__);
		return MTK_VDM_SW_BUSY;
	}
	pr_err("%s Time out\n", __func__);
	return MTK_VDM_FAIL;
}

int mtk_set_ta_output_switch(struct tcpc_device *tcpc, bool on_off)
{
	int count = MTK_VDM_COUNT;
	int status = MTK_VDM_FAIL;
	uint32_t data[7] = {0};

	if (!vdm_inited) {
		pr_err("%s vdm not inited\n", __func__);
		return -EINVAL;
	}
	if (!mtk_is_pep30_en_unlock()) {
		pr_err("%s pep30 is not ready\n", __func__);
		return -EINVAL;
	}

	data[0] = RT7207_VDM_HDR | 0x101d;
	data[1] = on_off ? 1 : 0;
	mtk_vdm_lock();
	atomic_inc(&vdm_event_flag); /* set flag = 1 */
	tcpm_send_uvdm(tcpc, 2, data, true);
	while (count) {
		if (atomic_read(&vdm_event_flag) == 0) {
			mutex_lock(&vdm_par_lock);
			if (vdm_success) {
				status = MTK_VDM_SUCCESS;
				pr_err("%s %s Success\n", __func__,
							on_off ? "on" : "off");
			} else {
				pr_err("%s %s Failed\n", __func__,
							on_off ? "on" : "off");
				status = MTK_VDM_FAIL;
			}
			mutex_unlock(&vdm_par_lock);
			mtk_vdm_unlock();
			return status;
		}
		count--;
		mdelay(MTK_VDM_DELAY);
	}
	atomic_dec(&vdm_event_flag);
	mtk_vdm_unlock();
	if (!tcpm_get_uvdm_handle_flag(tcpc)) {
		pr_err("%s Sw Busy Time out\n", __func__);
		return MTK_VDM_SW_BUSY;
	}
	pr_err("%s Time out\n", __func__);
	return MTK_VDM_FAIL;
}

int mtk_get_ta_temperature(struct tcpc_device *tcpc, int *temp)
{
	int count = MTK_VDM_COUNT;
	int status = MTK_VDM_FAIL;
	uint32_t data[7] = {0};

	if (!vdm_inited) {
		pr_err("%s vdm not inited\n", __func__);
		return -EINVAL;
	}
	if (!mtk_is_pep30_en_unlock()) {
		pr_err("%s pep30 is not ready\n", __func__);
		return -EINVAL;
	}

	data[0] = RT7207_VDM_HDR | 0x101d;
	mtk_vdm_lock();
	atomic_inc(&vdm_event_flag); /* set flag = 1 */
	tcpm_send_uvdm(tcpc, 1, data, true);
	while (count) {
		if (atomic_read(&vdm_event_flag) == 0) {
			mutex_lock(&vdm_par_lock);
			if (vdm_success) {
				*temp = vdm_payload[1]&0x0000ffff;
				status = MTK_VDM_SUCCESS;
				pr_err("%s temperatue = %d\n", __func__, *temp);
			} else {
				pr_err("%s Failed\n", __func__);
				status = MTK_VDM_FAIL;
			}
			mutex_unlock(&vdm_par_lock);
			mtk_vdm_unlock();
			return status;
		}
		count--;
		mdelay(MTK_VDM_DELAY);
	}
	atomic_dec(&vdm_event_flag);
	mtk_vdm_unlock();
	if (!tcpm_get_uvdm_handle_flag(tcpc)) {
		pr_err("%s Sw Busy Time out\n", __func__);
		return MTK_VDM_SW_BUSY;
	}
	pr_err("%s Time out\n", __func__);
	return MTK_VDM_FAIL;
}

int mtk_set_ta_boundary_cap(struct tcpc_device *tcpc,
					struct mtk_vdm_ta_cap *cap)
{
	int count = MTK_VDM_COUNT;
	int status = MTK_VDM_FAIL;
	uint32_t data[7] = {0};

	if (!vdm_inited) {
		pr_err("%s vdm not inited\n", __func__);
		return -EINVAL;
	}
	if (!mtk_is_pep30_en_unlock()) {
		pr_err("%s pep30 is not ready\n", __func__);
		return -EINVAL;
	}

	data[0] = RT7207_VDM_HDR | 0x2021;
	data[1] = (cap->cur<<16)|cap->vol;
	pr_err("%s set mv = %dmv,ma = %dma\n",
		__func__, cap->vol, cap->cur);

	mtk_vdm_lock();
	atomic_inc(&vdm_event_flag); /* set flag = 1 */
	tcpm_send_uvdm(tcpc, 2, data, true);
	while (count) {
		if (atomic_read(&vdm_event_flag) == 0) {
			mutex_lock(&vdm_par_lock);
			if (vdm_success) {
				cap->vol = vdm_payload[1]&0x0000ffff;
				cap->cur = (vdm_payload[1]&0xffff0000)>>16;
				status = MTK_VDM_SUCCESS;
				pr_err("%s mv = %dmv,ma = %dma\n",
					__func__, cap->vol, cap->cur);
			} else {
				pr_err("%s Failed\n", __func__);
				status = MTK_VDM_FAIL;
			}
			mutex_unlock(&vdm_par_lock);
			mtk_vdm_unlock();
			return status;
		}
		count--;
		mdelay(MTK_VDM_DELAY);
	}
	atomic_dec(&vdm_event_flag);
	mtk_vdm_unlock();
	if (!tcpm_get_uvdm_handle_flag(tcpc)) {
		pr_err("%s Sw Busy Time out\n", __func__);
		return MTK_VDM_SW_BUSY;
	}
	pr_err("%s Time out\n", __func__);
	return MTK_VDM_FAIL;
}

int mtk_get_ta_boundary_cap(struct tcpc_device *tcpc,
					struct mtk_vdm_ta_cap *cap)
{
	int count = MTK_VDM_COUNT;
	int status = MTK_VDM_FAIL;
	uint32_t data[7] = {0};

	if (!vdm_inited) {
		pr_err("%s vdm not inited\n", __func__);
		return -EINVAL;
	}
	if (!mtk_is_pep30_en_unlock()) {
		pr_err("%s pep30 is not ready\n", __func__);
		return -EINVAL;
	}

	data[0] = RT7207_VDM_HDR | 0x1021;
	mtk_vdm_lock();
	atomic_inc(&vdm_event_flag); /* set flag = 1 */
	tcpm_send_uvdm(tcpc, 1, data, true);
	while (count) {
		if (atomic_read(&vdm_event_flag) == 0) {
			mutex_lock(&vdm_par_lock);
			if (vdm_success) {
				cap->vol = vdm_payload[1]&0x0000ffff;
				cap->cur = (vdm_payload[1]&0xffff0000)>>16;
				status = MTK_VDM_SUCCESS;
				pr_err("%s mv = %dmv,ma = %dma\n",
					__func__, cap->vol, cap->cur);
			} else {
				pr_err("%s Failed\n", __func__);
				status = MTK_VDM_FAIL;
			}
			mutex_unlock(&vdm_par_lock);
			mtk_vdm_unlock();
			return status;
		}
		count--;
		mdelay(MTK_VDM_DELAY);
	}
	atomic_dec(&vdm_event_flag);
	mtk_vdm_unlock();
	if (!tcpm_get_uvdm_handle_flag(tcpc)) {
		pr_err("%s Sw Busy Time out\n", __func__);
		return MTK_VDM_SW_BUSY;
	}
	pr_err("%s Time out\n", __func__);
	return MTK_VDM_FAIL;
}

int mtk_set_ta_cap(struct tcpc_device *tcpc, struct mtk_vdm_ta_cap *cap)
{
	int count = MTK_VDM_COUNT;
	int status = MTK_VDM_FAIL;
	uint32_t data[7] = {0};

	if (!vdm_inited) {
		pr_err("%s vdm not inited\n", __func__);
		return -EINVAL;
	}
	if (!mtk_is_pep30_en_unlock()) {
		pr_err("%s pep30 is not ready\n", __func__);
		return -EINVAL;
	}

	data[0] = RT7207_VDM_HDR | 0x2022;
	data[1] = (cap->cur<<16)|cap->vol;
	mtk_vdm_lock();
	atomic_inc(&vdm_event_flag); /* set flag = 1 */
	tcpm_send_uvdm(tcpc, 2, data, true);
	while (count) {
		if (atomic_read(&vdm_event_flag) == 0) {
			mutex_lock(&vdm_par_lock);
			if (vdm_success) {
				cap->vol = vdm_payload[1]&0x0000ffff;
				cap->cur = (vdm_payload[1]&0xffff0000)>>16;
				status = MTK_VDM_SUCCESS;
				pr_err("%s mv = %dmv,ma = %dma\n",
					__func__, cap->vol, cap->cur);
			} else {
				pr_err("%s Failed\n", __func__);
				status = MTK_VDM_FAIL;
			}
			mutex_unlock(&vdm_par_lock);
			mtk_vdm_unlock();
			return status;
		}
		count--;
		mdelay(MTK_VDM_DELAY);
	}
	atomic_dec(&vdm_event_flag);
	mtk_vdm_unlock();
	if (!tcpm_get_uvdm_handle_flag(tcpc)) {
		pr_err("%s Sw Busy Time out\n", __func__);
		return MTK_VDM_SW_BUSY;
	}
	pr_err("%s Time out\n", __func__);
	return MTK_VDM_FAIL;
}

int mtk_get_ta_cap(struct tcpc_device *tcpc,
					struct mtk_vdm_ta_cap *cap)
{
	int count = MTK_VDM_COUNT;
	int status = MTK_VDM_FAIL;
	uint32_t data[7] = {0};

	if (!vdm_inited) {
		pr_err("%s vdm not inited\n", __func__);
		return -EINVAL;
	}
	if (!mtk_is_pep30_en_unlock()) {
		pr_err("%s pep30 is not ready\n", __func__);
		return -EINVAL;
	}

	data[0] = RT7207_VDM_HDR | 0x1022;

	mtk_vdm_lock();
	atomic_inc(&vdm_event_flag); /* set flag = 1 */
	tcpm_send_uvdm(tcpc, 1, data, true);
	while (count) {
		if (atomic_read(&vdm_event_flag) == 0) {
			mutex_lock(&vdm_par_lock);
			if (vdm_success) {
				cap->vol = vdm_payload[1]&0x0000ffff;
				cap->cur = (vdm_payload[1]&0xffff0000)>>16;
				status = MTK_VDM_SUCCESS;
				pr_err("%s mv = %dmv,ma = %dma\n",
					__func__, cap->vol, cap->cur);
			} else {
				pr_err("%s Failed\n", __func__);
				status = MTK_VDM_FAIL;
			}
			mutex_unlock(&vdm_par_lock);
			mtk_vdm_unlock();
			return status;
		}
		count--;
		mdelay(MTK_VDM_DELAY);
	}
	atomic_dec(&vdm_event_flag);
	mtk_vdm_unlock();
	if (!tcpm_get_uvdm_handle_flag(tcpc)) {
		pr_err("%s Sw Busy Time out\n", __func__);
		return MTK_VDM_SW_BUSY;
	}
	pr_err("%s Time out\n", __func__);
	return MTK_VDM_FAIL;
}

int mtk_set_ta_uvlo(struct tcpc_device *tcpc, int mv)
{
	int count = MTK_VDM_COUNT;
	int status = MTK_VDM_FAIL;
	uint32_t data[7] = {0};

	if (!vdm_inited) {
		pr_err("%s vdm not inited\n", __func__);
		return -EINVAL;
	}
	if (!mtk_is_pep30_en_unlock()) {
		pr_err("%s pep30 is not ready\n", __func__);
		return -EINVAL;
	}

	data[0] = RT7207_VDM_HDR | 0x2023;
	data[1] = (mv&0x0000ffff);
	mtk_vdm_lock();
	atomic_inc(&vdm_event_flag); /* set flag = 1 */
	tcpm_send_uvdm(tcpc, 2, data, true);
	while (count) {
		if (atomic_read(&vdm_event_flag) == 0) {
			mutex_lock(&vdm_par_lock);
			if (vdm_success)
				status = MTK_VDM_SUCCESS;
			else {
				pr_err("%s Failed\n", __func__);
				status = MTK_VDM_FAIL;
			}
			pr_err("%s uvlo = %dmv\n", __func__,
				(vdm_payload[1]&0x0000ffff));
			mutex_unlock(&vdm_par_lock);
			mtk_vdm_unlock();
			return status;
		}
		count--;
		mdelay(MTK_VDM_DELAY);
	}
	atomic_dec(&vdm_event_flag);
	mtk_vdm_unlock();
	if (!tcpm_get_uvdm_handle_flag(tcpc)) {
		pr_err("%s Sw Busy Time out\n", __func__);
		return MTK_VDM_SW_BUSY;
	}
	pr_err("%s Time out\n", __func__);
	return MTK_VDM_FAIL;
}

#ifdef CONFIG_DEBUG_FS
struct rt_debug_st {
	int id;
};

static struct dentry *debugfs_vdm_dir;
static struct dentry *debugfs_vdm_file[3];
static struct rt_debug_st vdm_dbg_data[3];

enum {
	RT7207_VDM_TEST,
};

static int de_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}

static ssize_t de_read(struct file *file,
		char __user *user_buffer, size_t count, loff_t *position)
{
	struct rt_debug_st *db = file->private_data;
	char tmp[200] = {0};
#if 0
	unsigned char val;
	int ret;
	struct mtk_vdm_ta_cap cap;
	struct pd_ta_stat stat;
#endif

	switch (db->id) {
	case RT7207_VDM_TEST:
		sprintf(tmp, "RT7207 VDM API Test\n");
#if 0
		tcpm_get_cable_capability(tcpc, &val);
		pr_info("cable capability = %d\n", val);
		pr_info("mtk_get is pd chg ready (%d), is pe30 ready (%d)\n",
			mtk_is_pd_chg_ready(), mtk_is_pep30_en_unlock());
		mtk_get_ta_id(tcpc);
		mtk_enable_direct_charge(tcpc, true);
		mtk_get_ta_charger_status(tcpc, &stat);
		mtk_get_ta_current_cap(tcpc, &cap);
		mtk_get_ta_temperature(tcpc, &ret);
		mtk_get_ta_cap(tcpc, &cap);
		cap.vol = 5000;
		cap.cur = 3000;
		mtk_set_ta_cap(tcpc, &cap);
		mtk_monitor_ta_inform(tcpc, &cap);
#endif
		break;
	default:
		break;
	}
	return simple_read_from_buffer(user_buffer, count,
					position, tmp, strlen(tmp));
}

static ssize_t de_write(struct file *file,
		const char __user *user_buffer, size_t count, loff_t *position)
{
	struct rt_debug_st *db = file->private_data;
	char buf[200] = {0};
	unsigned long yo;
	struct mtk_vdm_ta_cap cap;
	struct pd_ta_stat stat;
	int ret;

	simple_write_to_buffer(buf, sizeof(buf), position, user_buffer, count);
	ret = kstrtoul(buf, 16, &yo);

	switch (db->id) {
	case RT7207_VDM_TEST:
		if (yo == 1)
			mtk_get_ta_id(tcpc);
		else if (yo == 2)
			mtk_get_ta_charger_status(tcpc, &stat);
		else if (yo == 3)
			mtk_get_ta_current_cap(tcpc, &cap);
		else if (yo == 4)
			mtk_get_ta_temperature(tcpc, &ret);
		else if (yo == 5)
			tcpm_set_direct_charge_en(tcpc, true);
		else if (yo == 6)
			tcpm_set_direct_charge_en(tcpc, false);
		else if (yo == 7) {
			cap.cur = 3000;
			cap.vol = 5000;
			mtk_set_ta_boundary_cap(tcpc, &cap);
		} else if (yo == 8) {
			cap.cur = 1200;
			cap.vol = 3800;
			mtk_set_ta_cap(tcpc, &cap);
		} else if (yo == 9)
			mtk_set_ta_uvlo(tcpc, 3000);
		break;
	default:
		break;
	}
	return count;
}

static const struct file_operations debugfs_fops = {
	.open = de_open,
	.read = de_read,
	.write = de_write,
};
#endif /* CONFIG_DEBUG_FS */

int mtk_direct_charge_vdm_init(void)
{
	int ret = 0;

	if (!vdm_inited) {
		tcpc = tcpc_dev_get_by_name("type_c_port0");
		if (!tcpc) {
			pr_err("%s get tcpc device type_c_port0 fail\n",
								__func__);
			return -ENODEV;
		}

		dr_usb = dual_role_phy_instance_get_byname(
						"dual-role-type_c_port0");
		if (!dr_usb) {
			pr_err("%s get dual role instance fail\n", __func__);
			return -EINVAL;
		}

		mutex_init(&vdm_event_lock);
		mutex_init(&vdm_par_lock);
		wake_lock_init(&vdm_event_wake_lock,
			WAKE_LOCK_SUSPEND, "RT7208 VDM Wakelock");
		atomic_set(&vdm_event_flag, 0);

		vdm_nb.notifier_call = vdm_tcp_notifier_call;
		ret = register_tcp_dev_notifier(tcpc, &vdm_nb);
		if (ret < 0) {
			pr_err("%s: register tcpc notifier fail\n", __func__);
			return -EINVAL;
		}

		vdm_inited = true;

#ifdef CONFIG_DEBUG_FS
		debugfs_vdm_dir = debugfs_create_dir("rt7207_vdm_dbg", 0);
		if (!IS_ERR(debugfs_vdm_dir)) {
			vdm_dbg_data[0].id = RT7207_VDM_TEST;
			debugfs_vdm_file[0] = debugfs_create_file("test", 0644,
				debugfs_vdm_dir, (void *)&vdm_dbg_data[0],
				&debugfs_fops);
		}
#endif /* CONFIG_DEBUG_FS */
		pr_info("%s init OK!\n", __func__);
	}
	return 0;
}
EXPORT_SYMBOL(mtk_direct_charge_vdm_init);

int mtk_direct_charge_vdm_deinit(void)
{
	if (vdm_inited) {
		mutex_destroy(&vdm_event_lock);
		vdm_inited = false;
		if (!IS_ERR(debugfs_vdm_dir))
			debugfs_remove_recursive(debugfs_vdm_dir);
	}
	return 0;
}
EXPORT_SYMBOL(mtk_direct_charge_vdm_deinit);
