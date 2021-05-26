/*
 * Copyright (C) 2016 Richtek Technology Corp.
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

#ifndef PD_DPM_CORE_H
#define PD_DPM_CORE_H

#include "tcpci.h"
#include "pd_core.h"

/* ---- MISC ---- */
int pd_dpm_core_init(struct pd_port_t *pd_port);
int pd_dpm_enable_vconn(struct pd_port_t *pd_port, bool en);
int pd_dpm_send_sink_caps(struct pd_port_t *pd_port);
int pd_dpm_send_source_caps(struct pd_port_t *pd_port);

/* ---- SNK ---- */

bool pd_dpm_send_request(struct pd_port_t *pd_port, int mv, int ma);

void pd_dpm_snk_evaluate_caps(struct pd_port_t *pd_port, struct pd_event_t *pd_event);
void pd_dpm_snk_transition_power(struct pd_port_t *pd_port, struct pd_event_t *pd_event);
void pd_dpm_snk_hard_reset(struct pd_port_t *pd_port, struct pd_event_t *pd_event);

/* ---- SRC ---- */

void pd_dpm_src_evaluate_request(struct pd_port_t *pd_port, struct pd_event_t *pd_event);
void pd_dpm_src_transition_power(struct pd_port_t *pd_port, struct pd_event_t *pd_event);
void pd_dpm_src_hard_reset(struct pd_port_t *pd_port);
void pd_dpm_src_inform_cable_vdo(struct pd_port_t *pd_port, struct pd_event_t *pd_event);

/* ---- UFP : Evaluate VDM Request ---- */

void pd_dpm_ufp_request_id_info(struct pd_port_t *pd_port, struct pd_event_t *pd_event);
void pd_dpm_ufp_request_svid_info(struct pd_port_t *pd_port, struct pd_event_t *pd_event);
void pd_dpm_ufp_request_mode_info(struct pd_port_t *pd_port, struct pd_event_t *pd_event);
void pd_dpm_ufp_request_enter_mode(struct pd_port_t *pd_port, struct pd_event_t *pd_event);
void pd_dpm_ufp_request_exit_mode(struct pd_port_t *pd_port, struct pd_event_t *pd_event);


/* ---- UFP : Response VDM Request ---- */

int pd_dpm_ufp_response_id(struct pd_port_t *pd_port, struct pd_event_t *pd_event);
int pd_dpm_ufp_response_svids(struct pd_port_t *pd_port, struct pd_event_t *pd_event);
int pd_dpm_ufp_response_modes(struct pd_port_t *pd_port, struct pd_event_t *pd_event);

/* ---- UFP : DP Only ---- */

#ifdef CONFIG_USB_PD_ALT_MODE
int pd_dpm_ufp_request_dp_status(struct pd_port_t *pd_port, struct pd_event_t *pd_event);
int pd_dpm_ufp_request_dp_config(struct pd_port_t *pd_port, struct pd_event_t *pd_event);
void pd_dpm_ufp_send_dp_attention(struct pd_port_t *pd_port, struct pd_event_t *pd_event);
#endif

/* ---- DFP : Inform VDM Result ---- */

void pd_dpm_dfp_inform_id(struct pd_port_t *pd_port, struct pd_event_t *pd_event, bool ack);
void pd_dpm_dfp_inform_svids(
			struct pd_port_t *pd_port, struct pd_event_t *pd_event, bool ack);
void pd_dpm_dfp_inform_modes(
			struct pd_port_t *pd_port, struct pd_event_t *pd_event, bool ack);
void pd_dpm_dfp_inform_enter_mode(
	struct pd_port_t *pd_port, struct pd_event_t *pd_event, bool ack);
void pd_dpm_dfp_inform_exit_mode(struct pd_port_t *pd_port, struct pd_event_t *pd_event);
void pd_dpm_dfp_inform_attention(struct pd_port_t *pd_port, struct pd_event_t *pd_event);

void pd_dpm_dfp_inform_cable_vdo(struct pd_port_t *pd_port, struct pd_event_t *pd_event);

/* ---- DFP : DP Only  ---- */

#ifdef CONFIG_USB_PD_ALT_MODE_DFP
void pd_dpm_dfp_send_dp_status_update(struct pd_port_t *pd_port, struct pd_event_t *pd_event);
void pd_dpm_dfp_inform_dp_status_update(
			struct pd_port_t *pd_port, struct pd_event_t *pd_event, bool ack);

void pd_dpm_dfp_send_dp_configuration(struct pd_port_t *pd_port, struct pd_event_t *pd_event);
void pd_dpm_dfp_inform_dp_configuration(
		struct pd_port_t *pd_port, struct pd_event_t *pd_event, bool ack);
#endif

/* ---- UVDM  ---- */

#ifdef CONFIG_USB_PD_UVDM

void pd_dpm_ufp_recv_uvdm(struct pd_port_t *pd_port, struct pd_event_t *pd_event);
void pd_dpm_dfp_send_uvdm(struct pd_port_t *pd_port, struct pd_event_t *pd_event);
void pd_dpm_dfp_inform_uvdm(struct pd_port_t *pd_port, struct pd_event_t *pd_event, bool ack);

#endif     /* CONFIG_USB_PD_UVDM */

/* ---- DRP : Inform PowerCap ---- */

void pd_dpm_dr_inform_sink_cap(struct pd_port_t *pd_port, struct pd_event_t *pd_event);
void pd_dpm_dr_inform_source_cap(struct pd_port_t *pd_port, struct pd_event_t *pd_event);

/* ---- DRP : Data Role Swap ---- */

void pd_dpm_drs_evaluate_swap(struct pd_port_t *pd_port, uint8_t role);
void pd_dpm_drs_change_role(struct pd_port_t *pd_port, uint8_t role);

/* ---- DRP : Power Role Swap ---- */

void pd_dpm_prs_evaluate_swap(struct pd_port_t *pd_port, uint8_t role);
void pd_dpm_prs_turn_off_power_sink(struct pd_port_t *pd_port);
void pd_dpm_prs_enable_power_source(struct pd_port_t *pd_port, bool en);
void pd_dpm_prs_change_role(struct pd_port_t *pd_port, uint8_t role);

/* ---- DRP : Vconn Swap ---- */

void pd_dpm_vcs_evaluate_swap(struct pd_port_t *pd_port);
void pd_dpm_vcs_enable_vconn(struct pd_port_t *pd_port, bool en);


/* PE : Notify DPM */

int pd_dpm_notify_pe_startup(struct pd_port_t *pd_port);
int pd_dpm_notify_pe_hardreset(struct pd_port_t *pd_port);
int pd_dpm_notify_pe_ready(struct pd_port_t *pd_port, struct pd_event_t *pd_event);

#ifdef CONFIG_USB_PD_DFP_FLOW_DELAY
int pd_dpm_notify_dfp_delay_done(
	struct pd_port_t *pd_port, struct pd_event_t *pd_event);
#endif	/* CONFIG_USB_PD_DFP_FLOW_DELAY */

/* TCPCI - VBUS Control */

static inline int pd_dpm_check_vbus_valid(struct pd_port_t *pd_port)
{
	return tcpci_check_vbus_valid(pd_port->tcpc_dev);
}

static inline int pd_dpm_sink_vbus(struct pd_port_t *pd_port, bool en)
{
	int mv = en ? TCPC_VBUS_SINK_5V : TCPC_VBUS_SINK_0V;

	return tcpci_sink_vbus(pd_port->tcpc_dev,
				TCP_VBUS_CTRL_REQUEST, mv, -1);
}

static inline int pd_dpm_source_vbus(struct pd_port_t *pd_port, bool en)
{
	int mv = en ? TCPC_VBUS_SOURCE_5V : TCPC_VBUS_SOURCE_0V;

	return tcpci_source_vbus(pd_port->tcpc_dev,
				TCP_VBUS_CTRL_REQUEST, mv, -1);
}

/* Mode Operations */

#ifdef CONFIG_USB_PD_ALT_MODE
#ifdef CONFIG_USB_PD_ALT_MODE_DFP

extern bool dp_dfp_u_notify_discover_id(struct pd_port_t *pd_port,
	struct svdm_svid_data_t *svid_data, struct pd_event_t *pd_event, bool ack);

extern bool dp_dfp_u_notify_discover_svid(
	struct pd_port_t *pd_port, struct svdm_svid_data_t *svid_data, bool ack);

extern bool dp_dfp_u_notify_discover_modes(
	struct pd_port_t *pd_port, struct svdm_svid_data_t *svid_data, bool ack);


extern bool dp_dfp_u_notify_enter_mode(struct pd_port_t *pd_port,
	struct svdm_svid_data_t *svid_data, uint8_t ops, bool ack);

extern bool dp_dfp_u_notify_exit_mode(
	struct pd_port_t *pd_port, struct svdm_svid_data_t *svid_data, uint8_t ops);

extern bool dp_dfp_u_notify_attention(struct pd_port_t *pd_port,
	struct svdm_svid_data_t *svid_data, struct pd_event_t *pd_event);
#endif	/* CONFIG_USB_PD_ALT_MODE_DFP */

extern void dp_ufp_u_request_enter_mode(
	struct pd_port_t *pd_port, struct svdm_svid_data_t *svid_data, uint8_t ops);

extern void dp_ufp_u_request_exit_mode(
	struct pd_port_t *pd_port, struct svdm_svid_data_t *svid_data, uint8_t ops);

#ifdef CONFIG_USB_PD_ALT_MODE_DFP
extern bool dp_dfp_u_notify_pe_startup(
	struct pd_port_t *pd_port, struct svdm_svid_data_t *svid_data);

extern int dp_dfp_u_notify_pe_ready(struct pd_port_t *pd_port,
	struct svdm_svid_data_t *svid_data, struct pd_event_t *pd_event);
#endif	/* CONFIG_USB_PD_ALT_MODE_DFP */

extern bool dp_reset_state(
	struct pd_port_t *pd_port, struct svdm_svid_data_t *svid_data);

#endif	/* CONFIG_USB_PD_ALT_MODE */

#ifdef CONFIG_USB_PD_RICHTEK_UVDM
extern bool richtek_dfp_notify_uvdm(struct pd_port_t *pd_port,
		struct svdm_svid_data_t *svid_data, bool ack);
extern bool richtek_ufp_notify_uvdm(struct pd_port_t *pd_port,
		struct svdm_svid_data_t *svid_data);
#endif	/* CONFIG_USB_PD_RICHTEK_UVDM */


#ifdef CONFIG_USB_PD_ALT_MODE_RTDC
extern bool dc_dfp_notify_discover_id(struct pd_port_t *pd_port,
		struct svdm_svid_data_t *svid_data, struct pd_event_t *pd_event, bool ack);

extern bool dc_dfp_notify_discover_svid(
		struct pd_port_t *pd_port, struct svdm_svid_data_t *svid_data, bool ack);

extern bool dc_dfp_notify_discover_modes(
		struct pd_port_t *pd_port, struct svdm_svid_data_t *svid_data, bool ack);

extern bool dc_dfp_notify_enter_mode(struct pd_port_t *pd_port,
		struct svdm_svid_data_t *svid_data, uint8_t ops, bool ack);

extern bool dc_dfp_notify_exit_mode(
		struct pd_port_t *pd_port, struct svdm_svid_data_t *svid_data, uint8_t ops);

extern bool dc_dfp_notify_pe_startup(
		struct pd_port_t *pd_port, struct svdm_svid_data_t *svid_data);

extern int dc_dfp_notify_pe_ready(struct pd_port_t *pd_port,
		struct svdm_svid_data_t *svid_data, struct pd_event_t *pd_event);

extern bool dc_dfp_notify_uvdm(struct pd_port_t *pd_port,
			struct svdm_svid_data_t *svid_data, bool ack);
extern bool dc_ufp_notify_uvdm(struct pd_port_t *pd_port,
			struct svdm_svid_data_t *svid_data);

#endif /* CONFIG_USB_PD_ALT_MODE_RTDC */

#endif /* PD_DPM_CORE_H */
