/*
 * Copyright (C) 2016 Richtek Technology Corp.
 *
 * Power Delivery Managert Driver
 *
 * Author: TH <tsunghan_tsai@richtek.com>
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include "inc/tcpm.h"
#include "inc/pd_core.h"
#include "inc/tcpci.h"
#include "inc/pd_policy_engine.h"
#include "inc/pd_dpm_core.h"
#include "inc/tcpci_typec.h"

/* Inquire TCPC status */

int tcpm_inquire_remote_cc(struct tcpc_device *tcpc_dev,
	uint8_t *cc1, uint8_t *cc2, bool from_ic)
{
	int rv = 0;

	if (from_ic) {
		rv = tcpci_get_cc(tcpc_dev);
		if (rv < 0)
			return rv;
	}

	*cc1 = tcpc_dev->typec_remote_cc[0];
	*cc2 = tcpc_dev->typec_remote_cc[1];
	return 0;
}

int tcpm_inquire_vbus_level(
	struct tcpc_device *tcpc_dev, bool from_ic)
{
	int rv = 0;
	uint16_t power_status = 0;

	if (from_ic) {
		rv = tcpci_get_power_status(tcpc_dev, &power_status);
		if (rv < 0)
			return rv;

		tcpci_vbus_level_init(tcpc_dev, power_status);
	}

	return tcpc_dev->vbus_level;
}

bool tcpm_inquire_cc_polarity(
	struct tcpc_device *tcpc_dev)
{
	return tcpc_dev->typec_polarity;
}

uint8_t tcpm_inquire_typec_attach_state(
	struct tcpc_device *tcpc_dev)
{
	return tcpc_dev->typec_attach_new;
}

uint8_t tcpm_inquire_typec_role(
	struct tcpc_device *tcpc_dev)
{
	return tcpc_dev->typec_role;
}

uint8_t tcpm_inquire_typec_local_rp(
	struct tcpc_device *tcpc_dev)
{
	uint8_t level;

	switch (tcpc_dev->typec_local_rp_level) {
	case TYPEC_CC_RP_1_5:
		level = 1;
		break;

	case TYPEC_CC_RP_3_0:
		level = 2;
		break;

	default:
	case TYPEC_CC_RP_DFT:
		level = 0;
		break;
	}

	return level;
}

int tcpm_typec_set_rp_level(
	struct tcpc_device *tcpc_dev, uint8_t level)
{
	uint8_t res;

	if (level == 2)
		res = TYPEC_CC_RP_3_0;
	else if (level == 1)
		res = TYPEC_CC_RP_1_5;
	else
		res = TYPEC_CC_RP_DFT;

	return tcpc_typec_set_rp_level(tcpc_dev, res);
}

int tcpm_typec_change_role(
	struct tcpc_device *tcpc_dev, uint8_t typec_role)
{
	return tcpc_typec_change_role(tcpc_dev, typec_role);
}

#ifdef CONFIG_USB_POWER_DELIVERY

bool tcpm_inquire_pd_connected(
	struct tcpc_device *tcpc_dev)
{
	struct pd_port_t *pd_port = &tcpc_dev->pd_port;

	return pd_port->pd_connected;
}

bool tcpm_inquire_pd_prev_connected(
	struct tcpc_device *tcpc_dev)
{
	struct pd_port_t *pd_port = &tcpc_dev->pd_port;

	return pd_port->pd_prev_connected;
}

uint8_t tcpm_inquire_pd_data_role(
	struct tcpc_device *tcpc_dev)
{
	struct pd_port_t *pd_port = &tcpc_dev->pd_port;

	return pd_port->data_role;
}

uint8_t tcpm_inquire_pd_power_role(
	struct tcpc_device *tcpc_dev)
{
	struct pd_port_t *pd_port = &tcpc_dev->pd_port;

	return pd_port->power_role;
}

uint8_t tcpm_inquire_pd_vconn_role(
	struct tcpc_device *tcpc_dev)
{
	struct pd_port_t *pd_port = &tcpc_dev->pd_port;

	return pd_port->vconn_source;
}

#endif	/* CONFIG_USB_POWER_DELIVERY */

/* Request TCPC to send PD Request */

#ifdef CONFIG_USB_POWER_DELIVERY

int tcpm_power_role_swap(struct tcpc_device *tcpc_dev)
{
	bool ret;
	struct pd_port_t *pd_port = &tcpc_dev->pd_port;

	ret = pd_put_dpm_pd_request_event(pd_port,
				PD_DPM_PD_REQUEST_PR_SWAP);
	if (!ret)
		return TCPM_ERROR_PUT_EVENT;

	return TCPM_SUCCESS;
}
EXPORT_SYMBOL(tcpm_power_role_swap);

int tcpm_data_role_swap(struct tcpc_device *tcpc_dev)
{
	bool ret;
	struct pd_port_t *pd_port = &tcpc_dev->pd_port;

	ret = pd_put_dpm_pd_request_event(pd_port,
				PD_DPM_PD_REQUEST_DR_SWAP);
	if (!ret)
		return TCPM_ERROR_PUT_EVENT;

	return TCPM_SUCCESS;
}
EXPORT_SYMBOL(tcpm_data_role_swap);

int tcpm_vconn_swap(struct tcpc_device *tcpc_dev)
{
	bool ret;
	struct pd_port_t *pd_port = &tcpc_dev->pd_port;

	ret = pd_put_dpm_pd_request_event(pd_port,
				PD_DPM_PD_REQUEST_VCONN_SWAP);
	if (!ret)
		return TCPM_ERROR_PUT_EVENT;

	return TCPM_SUCCESS;
}
EXPORT_SYMBOL(tcpm_vconn_swap);

int tcpm_goto_min(struct tcpc_device *tcpc_dev)
{
	bool ret;
	struct pd_port_t *pd_port = &tcpc_dev->pd_port;

	ret = pd_put_dpm_pd_request_event(pd_port,
				PD_DPM_PD_REQUEST_GOTOMIN);
	if (!ret)
		return TCPM_ERROR_PUT_EVENT;

	return TCPM_SUCCESS;
}
EXPORT_SYMBOL(tcpm_goto_min);

int tcpm_soft_reset(struct tcpc_device *tcpc_dev)
{
	bool ret;
	struct pd_port_t *pd_port = &tcpc_dev->pd_port;

	ret = pd_put_dpm_pd_request_event(pd_port,
				PD_DPM_PD_REQUEST_SOFTRESET);
	if (!ret)
		return TCPM_ERROR_PUT_EVENT;

	return TCPM_SUCCESS;
}
EXPORT_SYMBOL(tcpm_soft_reset);

int tcpm_hard_reset(struct tcpc_device *tcpc_dev)
{
	bool ret;
	struct pd_port_t *pd_port = &tcpc_dev->pd_port;

	ret = pd_put_dpm_pd_request_event(pd_port,
				PD_DPM_PD_REQUEST_HARDRESET);
	if (!ret)
		return TCPM_ERROR_PUT_EVENT;

	return TCPM_SUCCESS;
}
EXPORT_SYMBOL(tcpm_hard_reset);

int tcpm_get_source_cap(
	struct tcpc_device *tcpc_dev, struct tcpm_power_cap *cap)
{
	bool ret;
	struct pd_port_t *pd_port = &tcpc_dev->pd_port;

	ret = pd_put_dpm_pd_request_event(pd_port,
				PD_DPM_PD_REQUEST_GET_SOURCE_CAP);
	if (!ret)
		return TCPM_ERROR_PUT_EVENT;

	/* TODO: Finish it later */

	return TCPM_SUCCESS;
}
EXPORT_SYMBOL(tcpm_get_source_cap);

int tcpm_get_sink_cap(
	struct tcpc_device *tcpc_dev, struct tcpm_power_cap *cap)
{
	bool ret;
	struct pd_port_t *pd_port = &tcpc_dev->pd_port;

	ret = pd_put_dpm_pd_request_event(pd_port,
				PD_DPM_PD_REQUEST_GET_SINK_CAP);
	if (!ret)
		return TCPM_ERROR_PUT_EVENT;

	/* TODO: Finish it later */

	return TCPM_SUCCESS;
}
EXPORT_SYMBOL(tcpm_get_sink_cap);

int tcpm_bist_cm2(struct tcpc_device *tcpc_dev)
{
	bool ret;
	struct pd_port_t *pd_port = &tcpc_dev->pd_port;

	ret = pd_put_dpm_pd_request_event(pd_port,
				PD_DPM_PD_REQUEST_BIST_CM2);
	if (!ret)
		return TCPM_ERROR_PUT_EVENT;

	/* TODO: Finish it later */

	return TCPM_SUCCESS;
}
EXPORT_SYMBOL(tcpm_bist_cm2);

int tcpm_request(struct tcpc_device *tcpc_dev, int mv, int ma)
{
	bool ret;
	struct pd_port_t *pd_port = &tcpc_dev->pd_port;

	mutex_lock(&pd_port->pd_lock);
	ret = pd_dpm_send_request(pd_port, mv, ma);
	mutex_unlock(&pd_port->pd_lock);

	if (!ret)
		return TCPM_ERROR_PUT_EVENT;

	return TCPM_SUCCESS;
}
EXPORT_SYMBOL(tcpm_request);

int tcpm_error_recovery(struct tcpc_device *tcpc_dev)
{
	bool ret;
	struct pd_port_t *pd_port = &tcpc_dev->pd_port;

	ret = pd_put_dpm_event(pd_port, PD_DPM_ERROR_RECOVERY);
	if (!ret)
		return TCPM_ERROR_PUT_EVENT;

	return TCPM_SUCCESS;
}

int tcpm_discover_cable(struct tcpc_device *tcpc_dev, uint32_t *vdos)
{
	bool ret;
	struct pd_port_t *pd_port = &tcpc_dev->pd_port;

	mutex_lock(&pd_port->pd_lock);
	pd_port->dpm_flags |= DPM_FLAGS_CHECK_CABLE_ID;
	ret = vdm_put_dpm_discover_cable_event(pd_port);
	mutex_unlock(&pd_port->pd_lock);

	if (!ret)
		return TCPM_ERROR_PUT_EVENT;

	return TCPM_SUCCESS;
}

int tcpm_vdm_request_id(struct tcpc_device *tcpc_dev,
				uint8_t *cnt, uint8_t *payload)
{
	bool ret;
	struct pd_port_t *pd_port = &tcpc_dev->pd_port;

	mutex_lock(&pd_port->pd_lock);
	ret = vdm_put_dpm_vdm_request_event(
		pd_port, PD_DPM_VDM_REQUEST_DISCOVER_ID);
	mutex_unlock(&pd_port->pd_lock);

	if (!ret)
		return TCPM_ERROR_PUT_EVENT;

	return TCPM_SUCCESS;
}

#ifdef CONFIG_USB_PD_ALT_MODE

int tcpm_dp_attention(
	struct tcpc_device *tcpc_dev, uint32_t dp_status)
{
	bool ret = false;
	struct pd_port_t *pd_port = &tcpc_dev->pd_port;

	mutex_lock(&pd_port->pd_lock);

	ret = vdm_put_dpm_vdm_request_event(
		pd_port, PD_DPM_VDM_REQUEST_ATTENTION);

	if (ret) {
		pd_port->dp_status = dp_status;
		pd_port->mode_svid = USB_SID_DISPLAYPORT;
	}

	mutex_unlock(&pd_port->pd_lock);

	if (!ret)
		return TCPM_ERROR_PUT_EVENT;

	return TCPM_SUCCESS;
}
EXPORT_SYMBOL(tcpm_dp_attention);

#ifdef CONFIG_USB_PD_ALT_MODE_DFP

int tcpm_dp_status_update(
	struct tcpc_device *tcpc_dev, uint32_t dp_status)
{
	bool ret = false;
	struct pd_port_t *pd_port = &tcpc_dev->pd_port;

	mutex_lock(&pd_port->pd_lock);

	ret = vdm_put_dpm_vdm_request_event(
		pd_port, PD_DPM_VDM_REQUEST_DP_STATUS_UPDATE);

	if (ret) {
		pd_port->dp_status = dp_status;
		pd_port->mode_svid = USB_SID_DISPLAYPORT;
	}

	mutex_unlock(&pd_port->pd_lock);

	if (!ret)
		return TCPM_ERROR_PUT_EVENT;

	return TCPM_SUCCESS;
}
EXPORT_SYMBOL(tcpm_dp_status_update);

int tcpm_dp_configuration(
	struct tcpc_device *tcpc_dev, uint32_t dp_config)
{
	bool ret = false;
	struct pd_port_t *pd_port = &tcpc_dev->pd_port;

	mutex_lock(&pd_port->pd_lock);

	ret = vdm_put_dpm_vdm_request_event(
		pd_port, PD_DPM_VDM_REQUEST_DP_CONFIG);

	if (ret) {
		pd_port->local_dp_config = dp_config;
		pd_port->mode_svid = USB_SID_DISPLAYPORT;
	}

	mutex_unlock(&pd_port->pd_lock);

	if (!ret)
		return TCPM_ERROR_PUT_EVENT;

	return TCPM_SUCCESS;
}
EXPORT_SYMBOL(tcpm_dp_configuration);

#endif	/* CONFIG_USB_PD_ALT_MODE_DFP */
#endif	/* CONFIG_USB_PD_ALT_MODE */

#ifdef CONFIG_USB_PD_UVDM
void tcpm_set_uvdm_handle_flag(struct tcpc_device *tcpc_dev, unsigned char en)
{
	tcpc_dev->uvdm_handle_flag = en ? 1 : 0;
}

bool tcpm_get_uvdm_handle_flag(struct tcpc_device *tcpc_dev)
{
	return tcpc_dev->uvdm_handle_flag;
}

int tcpm_send_uvdm(struct tcpc_device *tcpc_dev,
	uint8_t cnt, uint32_t *data, bool wait_resp)
{
	bool ret = false;
	struct pd_port_t *pd_port = &tcpc_dev->pd_port;

	if (tcpc_dev->typec_attach_old == TYPEC_UNATTACHED)
		return TCPM_ERROR_UNATTACHED;

	if (cnt > VDO_MAX_SIZE)
		return TCPM_ERROR_PARAMETER;

	mutex_lock(&pd_port->pd_lock);

	pd_port->uvdm_cnt = cnt;
	pd_port->uvdm_wait_resp = wait_resp;
	memcpy(pd_port->uvdm_data, data, sizeof(uint32_t) * cnt);

	ret = vdm_put_dpm_vdm_request_event(
		pd_port, PD_DPM_VDM_REQUEST_UVDM);

	mutex_unlock(&pd_port->pd_lock);

	if (!ret)
		return TCPM_ERROR_PUT_EVENT;

	return 0;
}
#endif	/* CONFIG_USB_PD_UVDM */

int tcpm_notify_vbus_stable(
	struct tcpc_device *tcpc_dev)
{
#if CONFIG_USB_PD_VBUS_STABLE_TOUT
	tcpc_disable_timer(tcpc_dev, PD_TIMER_VBUS_STABLE);
#endif

	pd_put_vbus_stable_event(tcpc_dev);
	return TCPM_SUCCESS;
}
EXPORT_SYMBOL(tcpm_notify_vbus_stable);

int tcpm_get_remote_power_cap(struct tcpc_device *tcpc_dev,
		struct tcpm_remote_power_cap *cap)
{
	int vmax, vmin, ioper;
	int i;

	cap->selected_cap_idx = tcpc_dev->pd_port.remote_selected_cap;
	cap->nr = tcpc_dev->pd_port.remote_src_cap.nr;
	for (i = 0; i < cap->nr; i++) {
		pd_extract_pdo_power(tcpc_dev->pd_port.remote_src_cap.pdos[i],
				&vmin, &vmax, &ioper);
		cap->max_mv[i] = vmax;
		cap->min_mv[i] = vmin;
		cap->ma[i] = ioper;
	}
	return TCPM_SUCCESS;
}

int tcpm_get_cable_capability(struct tcpc_device *tcpc_dev,
					unsigned char *capability)
{
	struct pd_port_t *pd_port = &tcpc_dev->pd_port;
	unsigned char limit = 0;

	if (pd_port->power_cable_present) {
		limit =  PD_VDO_CABLE_CURR(
				pd_port->cable_vdos[VDO_INDEX_CABLE])+1;
		pr_info("%s limit = %d\n", __func__, limit);
		limit = (limit > 3) ? 0 : limit;
		*capability = limit;
	} else
		pr_info("%s it's not power cable\n", __func__);

	return TCPM_SUCCESS;
}

#ifdef CONFIG_RT7207_ADAPTER
int tcpm_set_direct_charge_en(struct tcpc_device *tcpc_dev, bool en)
{
	pd_dbg_info("%s set direct charge %s\n",
			__func__, en ? "enable" : "disable");
	tcpc_dev->rt7207_direct_charge_flag = en;
	pd_dbg_info("%s rt7207_direct_charge_flag = %d\n", __func__,
			tcpc_dev->rt7207_direct_charge_flag);
	return 0;
}

void tcpm_reset_pe30_ta(struct tcpc_device *tcpc_dev)
{
	tcpc_dev->rt7207_sup_flag = 0;
}

int tcpm_get_pe30_ta_sup(struct tcpc_device *tcpc_dev)
{
	int ret = 0;

	ret = tcpc_dev->rt7207_sup_flag;
	pd_dbg_info("%s (%d)\n", __func__, ret);
	return ret;
}
#endif /* CONFIG_RT7207_ADAPTER */

bool tcpm_get_boot_check_flag(struct tcpc_device *tcpc)
{
	return tcpc->boot_check_flag ? true : false;
}

void tcpm_set_boot_check_flag(struct tcpc_device *tcpc, unsigned char en)
{
	tcpc->boot_check_flag = en ? 1 : 0;
}

bool tcpm_get_ta_hw_exist(struct tcpc_device *tcpc)
{
	uint16_t data;

	tcpci_get_power_status(tcpc, &data);
	return (data & TCPC_REG_POWER_STATUS_VBUS_PRES);
}

#endif /* CONFIG_USB_POWER_DELIVERY */
