/*
 * Copyright (C) 2016 Richtek Technology Corp.
 *
 * Power Delivery Policy Engine Driver
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

#include "inc/pd_core.h"
#include "inc/pd_dpm_core.h"
#include "inc/tcpci.h"
#include "inc/pd_process_evt.h"
#include "inc/pd_policy_engine.h"


/* ---- Policy Engine State ---- */

#if PE_STATE_FULL_NAME

static const char *const pe_state_name[] = {

	"PE_SRC_STARTUP",
	"PE_SRC_DISCOVERY",
	"PE_SRC_SEND_CAPABILITIES",
	"PE_SRC_NEGOTIATE_CAPABILITIES",
	"PE_SRC_TRANSITION_SUPPLY",
	"PE_SRC_TRANSITION_SUPPLY2",
	"PE_SRC_READY",
	"PE_SRC_DISABLED",
	"PE_SRC_CAPABILITY_RESPONSE",
	"PE_SRC_HARD_RESET",
	"PE_SRC_HARD_RESET_RECEIVED",
	"PE_SRC_TRANSITION_TO_DEFAULT",
	"PE_SRC_GIVE_SOURCE_CAP",
	"PE_SRC_GET_SINK_CAP",
	"PE_SRC_WAIT_NEW_CAPABILITIES",

	"PE_SRC_SEND_SOFT_RESET",
	"PE_SRC_SOFT_RESET",
	"PE_SRC_PING",

#ifdef CONFIG_USB_PD_SRC_STARTUP_DISCOVER_ID
	"PE_SRC_VDM_IDENTITY_REQUEST",
	"PE_SRC_VDM_IDENTITY_ACKED",
	"PE_SRC_VDM_IDENTITY_NAKED",
#endif	/* CONFIG_USB_PD_SRC_STARTUP_DISCOVER_ID */

	"PE_SNK_STARTUP",
	"PE_SNK_DISCOVERY",
	"PE_SNK_WAIT_FOR_CAPABILITIES",
	"PE_SNK_EVALUATE_CAPABILITY",
	"PE_SNK_SELECT_CAPABILITY",
	"PE_SNK_TRANSITION_SINK",
	"PE_SNK_READY",
	"PE_SNK_HARD_RESET",
	"PE_SNK_TRANSITION_TO_DEFAULT",
	"PE_SNK_GIVE_SINK_CAP",
	"PE_SNK_GET_SOURCE_CAP",

	"PE_SNK_SEND_SOFT_RESET",
	"PE_SNK_SOFT_RESET",


	"PE_DRS_DFP_UFP_EVALUATE_DR_SWAP",
	"PE_DRS_DFP_UFP_ACCEPT_DR_SWAP",
	"PE_DRS_DFP_UFP_CHANGE_TO_UFP",
	"PE_DRS_DFP_UFP_SEND_DR_SWAP",
	"PE_DRS_DFP_UFP_REJECT_DR_SWAP",

	"PE_DRS_UFP_DFP_EVALUATE_DR_SWAP",
	"PE_DRS_UFP_DFP_ACCEPT_DR_SWAP",
	"PE_DRS_UFP_DFP_CHANGE_TO_DFP",
	"PE_DRS_UFP_DFP_SEND_SWAP",
	"PE_DRS_UFP_DFP_REJECT_DR_SWAP",

	"PE_PRS_SRC_SNK_EVALUATE_PR_SWAP",
	"PE_PRS_SRC_SNK_ACCEPT_PR_SWAP",
	"PE_PRS_SRC_SNK_TRANSITION_TO_OFF",
	"PE_PRS_SRC_SNK_ASSERT_RD",
	"PE_PRS_SRC_SNK_WAIT_SOURCE_ON",
	"PE_PRS_SRC_SNK_SEND_SWAP",
	"PE_PRS_SRC_SNK_REJECT_PR_SWAP",

	"PE_PRS_SNK_SRC_EVALUATE_PR_SWAP",
	"PE_PRS_SNK_SRC_ACCEPT_PR_SWAP",
	"PE_PRS_SNK_SRC_TRANSITION_TO_OFF",
	"PE_PRS_SNK_SRC_ASSERT_RP",
	"PE_PRS_SNK_SRC_SOURCE_ON",
	"PE_PRS_SNK_SRC_SEND_PR_SWAP",
	"PE_PRS_SNK_SRC_REJECT_SWAP",

	"PE_DR_SRC_GET_SOURCE_CAP",

	"PE_DR_SRC_GIVE_SINK_CAP",

	"PE_DR_SNK_GET_SINK_CAP",

	"PE_DR_SNK_GIVE_SOURCE_CAP",

	"PE_VCS_SEND_SWAP",
	"PE_VCS_EVALUATE_SWAP",
	"PE_VCS_ACCEPT_SWAP",
	"PE_VCS_REJECT_SWAP",
	"PE_VCS_WAIT_FOR_VCONN",
	"PE_VCS_TURN_OFF_VCONN",
	"PE_VCS_TURN_ON_VCONN",
	"PE_VCS_SEND_PS_RDY",

	"PE_UFP_VDM_GET_IDENTITY",
	"PE_UFP_VDM_SEND_IDENTITY",
	"PE_UFP_VDM_GET_IDENTITY_NAK",

	"PE_UFP_VDM_GET_SVIDS",
	"PE_UFP_VDM_SEND_SVIDS",
	"PE_UFP_VDM_GET_SVIDS_NAK",

	"PE_UFP_VDM_GET_MODES",
	"PE_UFP_VDM_SEND_MODES",
	"PE_UFP_VDM_GET_MODES_NAK",

	"PE_UFP_VDM_EVALUATE_MODE_ENTRY",
	"PE_UFP_VDM_MODE_ENTRY_ACK",
	"PE_UFP_VDM_MODE_ENTRY_NAK",

	"PE_UFP_VDM_MODE_EXIT",
	"PE_UFP_VDM_MODE_EXIT_ACK",
	"PE_UFP_VDM_MODE_EXIT_NAK",

	"PE_UFP_VDM_ATTENTION_REQUEST",

#ifdef CONFIG_USB_PD_ALT_MODE
	"PE_UFP_VDM_DP_STATUS_UPDATE",
	"PE_UFP_VDM_DP_CONFIGURE",
#endif	/* CONFIG_USB_PD_ALT_MODE */

	"PE_DFP_UFP_VDM_IDENTITY_REQUEST",
	"PE_DFP_UFP_VDM_IDENTITY_ACKED",
	"PE_DFP_UFP_VDM_IDENTITY_NAKED",

	"PE_DFP_CBL_VDM_IDENTITY_REQUEST",
	"PE_DFP_CBL_VDM_IDENTITY_ACKED",
	"PE_DFP_CBL_VDM_IDENTITY_NAKED",

	"PE_DFP_VDM_SVIDS_REQUEST",
	"PE_DFP_VDM_SVIDS_ACKED",
	"PE_DFP_VDM_SVIDS_NAKED",

	"PE_DFP_VDM_MODES_REQUEST",
	"PE_DFP_VDM_MODES_ACKED",
	"PE_DFP_VDM_MODES_NAKED",

	"PE_DFP_VDM_MODE_ENTRY_REQUEST",
	"PE_DFP_VDM_MODE_ENTRY_ACKED",
	"PE_DFP_VDM_MODE_ENTRY_NAKED",

	"PE_DFP_VDM_MODE_EXIT_REQUEST",
	"PE_DFP_VDM_MODE_EXIT_ACKED",

	"PE_DFP_VDM_ATTENTION_REQUEST",

#ifdef CONFIG_USB_PD_ALT_MODE_DFP
	"PE_DFP_VDM_DP_STATUS_UPDATE_REQUEST",
	"PE_DFP_VDM_DP_STATUS_UPDATE_ACKED",
	"PE_DFP_VDM_DP_STATUS_UPDATE_NAKED",

	"PE_DFP_VDM_DP_CONFIGURATION_REQUEST",
	"PE_DFP_VDM_DP_CONFIGURATION_ACKED",
	"PE_DFP_VDM_DP_CONFIGURATION_NAKED",
#endif	/* CONFIG_USB_PD_ALT_MODE_DFP */

#ifdef CONFIG_USB_PD_UVDM
	"PE_UFP_UVDM_RECV",
	"PE_DFP_UVDM_SEND",
	"PE_DFP_UVDM_ACKED",
	"PE_DFP_UVDM_NAKED",
#endif	/* CONFIG_USB_PD_UVDM */

#ifdef CONFIG_USB_PD_CUSTOM_DBGACC
	"PE_DBG_READY",
#endif	/* CONFIG_USB_PD_CUSTOM_DBGACC */

#ifdef CONFIG_USB_PD_RECV_HRESET_COUNTER
	"PE_OVER_RECV_HRESET_LIMIT",
#endif	/* CONFIG_USB_PD_RECV_HRESET_COUNTER */

	"PE_ERROR_RECOVERY",

	"PE_BIST_TEST_DATA",
	"PE_BIST_CARRIER_MODE_2",

	"PE_IDLE1",
	"PE_IDLE2",

	"PE_VIRT_HARD_RESET",
	"PE_VIRT_READY",
};
#else

static const char *const pe_state_name[] = {

	"SRC_START",
	"SRC_DISCOVERY",
	"SRC_SEND_CAP",
	"SRC_NEG_CAP",
	"SRC_TRANS_SUPPLY",
	"SRC_TRANS_SUPPLY2",
	"SRC_READY",
	"SRC_DISABLED",
	"SRC_CAP_RESP",
	"SRC_HRESET",
	"SRC_HRESET_RECV",
	"SRC_TRANS_DFT",
	"SRC_GIVE_CAP",
	"SRC_GET_CAP",
	"SRC_WAIT_CAP",

	"SRC_SEND_SRESET",
	"SRC_SRESET",
	"SRC_PING",

#ifdef CONFIG_USB_PD_SRC_STARTUP_DISCOVER_ID
	"SRC_VDM_ID_REQ",
	"SRC_VDM_ID_ACK",
	"SRC_VDM_ID_NAK",
#endif	/* CONFIG_USB_PD_SRC_STARTUP_DISCOVER_ID */

	"SNK_START",
	"SNK_DISCOVERY",
	"SNK_WAIT_CAP",
	"SNK_EVA_CAP",
	"SNK_SEL_CAP",
	"SNK_TRANS_SINK",
	"SNK_READY",
	"SNK_HRESET",
	"SNK_TRANS_DFT",
	"SNK_GIVE_CAP",
	"SNK_GET_CAP",

	"SNK_SEND_SRESET",
	"SNK_SRESET",

	"D_DFP_EVALUATE",
	"D_DFP_ACCEPT",
	"D_DFP_CHANGE",
	"D_DFP_SEND",
	"D_DFP_REJECT",

	"D_UFP_EVALUATE",
	"D_UFP_ACCEPT",
	"D_UFP_CHANGE",
	"D_UFP_SEND",
	"D_UFP_REJECT",

	"P_SRC_EVALUATE",
	"P_SRC_ACCEPT",
	"P_SRC_TRANS_OFF",
	"P_SRC_ASSERT",
	"P_SRC_WAIT_ON",
	"P_SRC_SEND",
	"P_SRC_REJECT",

	"P_SNK_EVALUATE",
	"P_SNK_ACCEPT",
	"P_SNK_TRANS_OFF",
	"P_SNK_ASSERT",
	"P_SNK_SOURCE_ON",
	"P_SNK_SEND",
	"P_SNK_REJECT",

	"DR_SRC_GET_CAP",	/* get source cap */
	"DR_SRC_GIVE_CAP",	/* give sink cap */
	"DR_SNK_GET_CAP",	/* get sink cap */
	"DR_SNK_GIVE_CAP", /* give source cap */

	"V_SEND",
	"V_EVALUATE",
	"V_ACCEPT",
	"V_REJECT",
	"V_WAIT_VCONN",
	"V_TURN_OFF",
	"V_TURN_ON",
	"V_PS_RDY",

	"U_GET_ID",
	"U_SEND_ID",
	"U_GET_ID_N",

	"U_GET_SVID",
	"U_SEND_SVID",
	"U_GET_SVID_N",

	"U_GET_MODE",
	"U_SEND_MODE",
	"U_GET_MODE_N",

	"U_EVA_MODE",
	"U_MODE_EN_A",
	"U_MODE_EN_N",

	"U_MODE_EX",
	"U_MODE_EX_A",
	"U_MODE_EX_N",

	"U_ATTENTION",

#ifdef CONFIG_USB_PD_ALT_MODE
	"U_D_STATUS",
	"U_D_CONFIG",
#endif	/* CONFIG_USB_PD_ALT_MODE */

	"D_UID_REQ",
	"D_UID_A",
	"D_UID_N",

	"D_CID_REQ",
	"D_CID_ACK",
	"D_CID_NAK",

	"D_SVID_REQ",
	"D_SVID_ACK",
	"D_SVID_NAK",

	"D_MODE_REQ",
	"D_MODE_ACK",
	"D_MODE_NAK",

	"D_MODE_EN_REQ",
	"D_MODE_EN_ACK",
	"D_MODE_EN_NAK",

	"D_MODE_EX_REQ",
	"D_MODE_EX_ACK",

	"D_ATTENTION",

#ifdef CONFIG_USB_PD_ALT_MODE_DFP
	"D_DP_STATUS_REQ",
	"D_DP_STATUS_ACK",
	"D_DP_STATUS_NAK",

	"D_DP_CONFIG_REQ",
	"D_DP_CONFIG_ACK",
	"D_DP_CONFIG_NAK",
#endif	/* CONFIG_USB_PD_ALT_MODE_DFP */

#ifdef CONFIG_USB_PD_UVDM
	"U_UVDM_RECV",
	"D_UVDM_SEND",
	"D_UVDM_ACKED",
	"D_UVDM_NAKED",
#endif	/* CONFIG_USB_PD_UVDM */

#ifdef CONFIG_USB_PD_CUSTOM_DBGACC
	"DBG_READY",
#endif	/* CONFIG_USB_PD_CUSTOM_DBGACC */

#ifdef CONFIG_USB_PD_RECV_HRESET_COUNTER
	"OVER_HRESET_LIMIT",
#endif	/* CONFIG_USB_PD_RECV_HRESET_COUNTER */

	"ERR_RECOVERY",

	"BIST_TD",
	"BIST_C2",

	"IDLE1",
	"IDLE2",

	"VIRT_HARD_RESET",
	"VIRT_READY",
};
#endif

struct pe_state_actions_t {
	void (*entry_action)
	(struct pd_port_t *pd_port, struct pd_event_t *pd_event);
	/* const pd_pe_state_action_fcn_t exit_action; */
};

/*
 * Policy Engine General State Activity
 * extern int rt1711_set_bist_carrier_mode
 * (struct tcpc_device *tcpc_dev, uint8_t pattern);
 */
static void pe_idle_reset_data(struct pd_port_t *pd_port)
{
	pd_reset_pe_timer(pd_port);
	pd_reset_svid_data(pd_port);

	pd_port->pd_prev_connected = false;
	pd_port->state_machine = PE_STATE_MACHINE_IDLE;

	switch (pd_port->pe_state_curr) {
	case PE_BIST_TEST_DATA:
		pd_enable_bist_test_mode(pd_port, false);
		break;

	case PE_BIST_CARRIER_MODE_2:
		pd_disable_bist_mode2(pd_port);
		break;
	}

	pd_unlock_msg_output(pd_port);
}

static void pe_idle1_entry(struct pd_port_t *pd_port, struct pd_event_t *pd_event)
{
	pe_idle_reset_data(pd_port);

	pd_try_put_pe_idle_event(pd_port);
}

static void pe_idle2_entry(struct pd_port_t *pd_port, struct pd_event_t *pd_event)
{
	pd_set_rx_enable(pd_port, PD_RX_CAP_PE_IDLE);
	pd_notify_pe_idle(pd_port);
}

void pe_error_recovery_entry(struct pd_port_t *pd_port, struct pd_event_t *pd_event)
{
	pe_idle_reset_data(pd_port);

	pd_set_rx_enable(pd_port, PD_RX_CAP_PE_IDLE);
	pd_notify_pe_error_recovery(pd_port);
	pd_free_pd_event(pd_port, pd_event);
}

#ifdef CONFIG_USB_PD_RECV_HRESET_COUNTER
void pe_over_recv_hreset_limit_entry(struct pd_port_t *pd_port, struct pd_event_t *pd_event)
{
	PE_INFO("OverHResetLimit++\r\n");
	pe_idle_reset_data(pd_port);
	pd_notify_pe_over_recv_hreset(pd_port);
	PE_INFO("OverHResetLimit--\r\n");
}
#endif	/* CONFIG_USB_PD_RECV_HRESET_COUNTER */

void pe_bist_test_data_entry(struct pd_port_t *pd_port, struct pd_event_t *pd_event)
{
	pd_enable_bist_test_mode(pd_port, true);
	pd_free_pd_event(pd_port, pd_event);
}

void pe_bist_test_data_exit(struct pd_port_t *pd_port, struct pd_event_t *pd_event)
{
	pd_enable_bist_test_mode(pd_port, false);
}

void pe_bist_carrier_mode_2_entry(struct pd_port_t *pd_port, struct pd_event_t *pd_event)
{
	pd_send_bist_mode2(pd_port);
	pd_enable_timer(pd_port, PD_TIMER_BIST_CONT_MODE);
	pd_free_pd_event(pd_port, pd_event);
}

void pe_bist_carrier_mode_2_exit(struct pd_port_t *pd_port, struct pd_event_t *pd_event)
{
	pd_disable_timer(pd_port, PD_TIMER_BIST_CONT_MODE);
	pd_disable_bist_mode2(pd_port);
}

/*
 * Policy Engine Share State Activity
 */

void pe_power_ready_entry(struct pd_port_t *pd_port, struct pd_event_t *pd_event)
{
	pd_port->during_swap = false;
	pd_port->explicit_contract = true;

	if (pd_port->data_role == PD_ROLE_UFP)
		pd_set_rx_enable(pd_port, PD_RX_CAP_PE_READY_UFP);
	else
		pd_set_rx_enable(pd_port, PD_RX_CAP_PE_READY_DFP);

	pd_dpm_notify_pe_ready(pd_port, pd_event);
	pd_free_pd_event(pd_port, pd_event);
}

static const struct pe_state_actions_t pe_state_actions[] = {

	/* src activity */
	{.entry_action = pe_src_startup_entry},
	{.entry_action = pe_src_discovery_entry},
	{.entry_action = pe_src_send_capabilities_entry},
	{.entry_action = pe_src_negotiate_capabilities_entry},
	{.entry_action = pe_src_transition_supply_entry},
	{.entry_action = pe_src_transition_supply2_entry},
	{.entry_action = pe_src_ready_entry},
	{.entry_action = pe_src_disabled_entry},
	{.entry_action = pe_src_capability_response_entry},
	{.entry_action = pe_src_hard_reset_entry},
	{.entry_action = pe_src_hard_reset_received_entry},
	{.entry_action = pe_src_transition_to_default_entry},
	{.entry_action = pe_src_give_source_cap_entry},
	{.entry_action = pe_src_get_sink_cap_entry},
	{.entry_action = pe_src_wait_new_capabilities_entry},

	{.entry_action = pe_src_send_soft_reset_entry},
	{.entry_action = pe_src_soft_reset_entry},
	{.entry_action = pe_src_ping_entry},

#ifdef CONFIG_USB_PD_SRC_STARTUP_DISCOVER_ID
	{.entry_action = pe_src_vdm_identity_request_entry},
	{.entry_action = pe_src_vdm_identity_acked_entry},
	{.entry_action = pe_src_vdm_identity_naked_entry},
#endif

	/* snk activity */
	{.entry_action = pe_snk_startup_entry},
	{.entry_action = pe_snk_discovery_entry},
	{.entry_action = pe_snk_wait_for_capabilities_entry},
	{.entry_action = pe_snk_evaluate_capability_entry},
	{.entry_action = pe_snk_select_capability_entry},
	{.entry_action = pe_snk_transition_sink_entry},
	{.entry_action = pe_snk_ready_entry},
	{.entry_action = pe_snk_hard_reset_entry},
	{.entry_action = pe_snk_transition_to_default_entry},
	{.entry_action = pe_snk_give_sink_cap_entry},
	{.entry_action = pe_snk_get_source_cap_entry},

	{.entry_action = pe_snk_send_soft_reset_entry},
	{.entry_action = pe_snk_soft_reset_entry},



	/* drs dfp activity */
	{.entry_action = pe_drs_dfp_ufp_evaluate_dr_swap_entry},
	{.entry_action = pe_drs_dfp_ufp_accept_dr_swap_entry},
	{.entry_action = pe_drs_dfp_ufp_change_to_ufp_entry},
	{.entry_action = pe_drs_dfp_ufp_send_dr_swap_entry},
	{.entry_action = pe_drs_dfp_ufp_reject_dr_swap_entry},

	/* drs ufp activity */
	{.entry_action = pe_drs_ufp_dfp_evaluate_dr_swap_entry},
	{.entry_action = pe_drs_ufp_dfp_accept_dr_swap_entry},
	{.entry_action = pe_drs_ufp_dfp_change_to_dfp_entry},
	{.entry_action = pe_drs_ufp_dfp_send_dr_swap_entry},
	{.entry_action = pe_drs_ufp_dfp_reject_dr_swap_entry},

	/* prs src activity */
	{.entry_action = pe_prs_src_snk_evaluate_pr_swap_entry},
	{.entry_action = pe_prs_src_snk_accept_pr_swap_entry},
	{.entry_action = pe_prs_src_snk_transition_to_off_entry},
	{.entry_action = pe_prs_src_snk_assert_rd_entry},
	{.entry_action = pe_prs_src_snk_wait_source_on_entry},
	{.entry_action = pe_prs_src_snk_send_swap_entry},
	{.entry_action = pe_prs_src_snk_reject_pr_swap_entry},

	/* prs snk activity */
	{.entry_action = pe_prs_snk_src_evaluate_pr_swap_entry},
	{.entry_action = pe_prs_snk_src_accept_pr_swap_entry},
	{.entry_action = pe_prs_snk_src_transition_to_off_entry},
	{.entry_action = pe_prs_snk_src_assert_rp_entry},
	{.entry_action = pe_prs_snk_src_source_on_entry},
	{.entry_action = pe_prs_snk_src_send_swap_entry},
	{.entry_action = pe_prs_snk_src_reject_swap_entry},

	/* dr src activity */
	{.entry_action = pe_dr_src_get_source_cap_entry},
	{.entry_action = pe_dr_src_give_sink_cap_entry},

	/* dr snk activity */
	{.entry_action = pe_dr_snk_get_sink_cap_entry},
	{.entry_action = pe_dr_snk_give_source_cap_entry},

	/* vcs activity */
	{.entry_action = pe_vcs_send_swap_entry},
	{.entry_action = pe_vcs_evaluate_swap_entry},
	{.entry_action = pe_vcs_accept_swap_entry},
	{.entry_action = pe_vcs_reject_vconn_swap_entry},
	{.entry_action = pe_vcs_wait_for_vconn_entry},
	{.entry_action = pe_vcs_turn_off_vconn_entry},
	{.entry_action = pe_vcs_turn_on_vconn_entry},
	{.entry_action = pe_vcs_send_ps_rdy_entry},

	/* ufp structured vdm activity */
	{.entry_action = pe_ufp_vdm_get_identity_entry},
	{.entry_action = pe_ufp_vdm_send_identity_entry},
	{.entry_action = pe_ufp_vdm_get_identity_nak_entry},

	{.entry_action = pe_ufp_vdm_get_svids_entry},
	{.entry_action = pe_ufp_vdm_send_svids_entry},
	{.entry_action = pe_ufp_vdm_get_svids_nak_entry},

	{.entry_action = pe_ufp_vdm_get_modes_entry},
	{.entry_action = pe_ufp_vdm_send_modes_entry},
	{.entry_action = pe_ufp_vdm_get_modes_nak_entry},

	{.entry_action = pe_ufp_vdm_evaluate_mode_entry_entry},
	{.entry_action = pe_ufp_vdm_mode_entry_ack_entry},
	{.entry_action = pe_ufp_vdm_mode_entry_nak_entry},

	{.entry_action = pe_ufp_vdm_mode_exit_entry},
	{.entry_action = pe_ufp_vdm_mode_exit_ack_entry},
	{.entry_action = pe_ufp_vdm_mode_exit_nak_entry},

	{.entry_action = pe_ufp_vdm_attention_request_entry},

#ifdef CONFIG_USB_PD_ALT_MODE
	{.entry_action = pe_ufp_vdm_dp_status_update_entry},
	{.entry_action = pe_ufp_vdm_dp_configure_entry},
#endif	/* CONFIG_USB_PD_ALT_MODE */

	/* dfp structured vdm */
	{.entry_action = pe_dfp_ufp_vdm_identity_request_entry},
	{.entry_action = pe_dfp_ufp_vdm_identity_acked_entry},
	{.entry_action = pe_dfp_ufp_vdm_identity_naked_entry},

	{.entry_action = pe_dfp_cbl_vdm_identity_request_entry},
	{.entry_action = pe_dfp_cbl_vdm_identity_acked_entry},
	{.entry_action = pe_dfp_cbl_vdm_identity_naked_entry},

	{.entry_action = pe_dfp_vdm_svids_request_entry},
	{.entry_action = pe_dfp_vdm_svids_acked_entry},
	{.entry_action = pe_dfp_vdm_svids_naked_entry},

	{.entry_action = pe_dfp_vdm_modes_request_entry},
	{.entry_action = pe_dfp_vdm_modes_acked_entry},
	{.entry_action = pe_dfp_vdm_modes_naked_entry},

	{.entry_action = pe_dfp_vdm_mode_entry_request_entry},
	{.entry_action = pe_dfp_vdm_mode_entry_acked_entry},
	{.entry_action = pe_dfp_vdm_mode_entry_naked_entry},

	{.entry_action = pe_dfp_vdm_mode_exit_request_entry},
	{.entry_action = pe_dfp_vdm_mode_exit_acked_entry},

	{.entry_action = pe_dfp_vdm_attention_request_entry},

#ifdef CONFIG_USB_PD_ALT_MODE_DFP
	{.entry_action = pe_dfp_vdm_dp_status_update_request_entry},
	{.entry_action = pe_dfp_vdm_dp_status_update_acked_entry},
	{.entry_action = pe_dfp_vdm_dp_status_update_naked_entry},

	{.entry_action = pe_dfp_vdm_dp_configuration_request_entry},
	{.entry_action = pe_dfp_vdm_dp_configuration_acked_entry},
	{.entry_action = pe_dfp_vdm_dp_configuration_naked_entry},
#endif	/* CONFIG_USB_PD_ALT_MODE_DFP */

#ifdef CONFIG_USB_PD_UVDM
	{.entry_action = pe_ufp_uvdm_recv_entry},
	{.entry_action = pe_dfp_uvdm_send_entry},
	{.entry_action = pe_dfp_uvdm_acked_entry},
	{.entry_action = pe_dfp_uvdm_naked_entry},
#endif	/* CONFIG_USB_PD_UVDM */

	/* general activity */
#ifdef CONFIG_USB_PD_CUSTOM_DBGACC
	{.entry_action = pe_dbg_ready_entry},
#endif	/* CONFIG_USB_PD_CUSTOM_DBGACC */

#ifdef CONFIG_USB_PD_RECV_HRESET_COUNTER
	{.entry_action = pe_over_recv_hreset_limit_entry},
#endif	/* CONFIG_USB_PD_RECV_HRESET_COUNTER */

	{.entry_action = pe_error_recovery_entry},

	{.entry_action = pe_bist_test_data_entry},
	{.entry_action = pe_bist_carrier_mode_2_entry},

	{.entry_action = pe_idle1_entry},
	{.entry_action = pe_idle2_entry},
};

static void pe_exit_action_disable_sender_response(
		struct pd_port_t *pd_port, struct pd_event_t *pd_event)
{
	pd_disable_timer(pd_port, PD_TIMER_SENDER_RESPONSE);
}

void pe_get_exit_action(uint8_t pe_state, void (*pe_state_exit_action)
	(struct pd_port_t *pd_port, struct pd_event_t *pd_event))
{

	switch (pe_state) {

	/* Source */
	case PE_SRC_SEND_CAPABILITIES:
		pe_state_exit_action = pe_src_send_capabilities_exit;
		break;
	case PE_SRC_TRANSITION_SUPPLY:
		pe_state_exit_action = pe_src_transition_supply_exit;
		break;
	case PE_SRC_TRANSITION_TO_DEFAULT:
		pe_state_exit_action = pe_src_transition_to_default_exit;
		break;
	case PE_SRC_GET_SINK_CAP:
		pe_state_exit_action = pe_src_get_sink_cap_exit;
		break;

	/* Sink */
	case PE_SNK_WAIT_FOR_CAPABILITIES:
		pe_state_exit_action = pe_snk_wait_for_capabilities_exit;
		break;
	case PE_SNK_SELECT_CAPABILITY:
		pe_state_exit_action = pe_snk_select_capability_exit;
		break;
	case PE_SNK_TRANSITION_SINK:
		pe_state_exit_action = pe_snk_transition_sink_exit;
		break;
	case PE_SNK_TRANSITION_TO_DEFAULT:
		pe_state_exit_action = pe_snk_transition_to_default_exit;
		break;

	case PE_DR_SRC_GET_SOURCE_CAP:
		pe_state_exit_action = pe_dr_src_get_source_cap_exit;
		break;
	case PE_DR_SNK_GET_SINK_CAP:
		pe_state_exit_action = pe_dr_snk_get_sink_cap_exit;
		break;

	case PE_BIST_TEST_DATA:
		pe_state_exit_action = pe_bist_test_data_exit;
		break;

	case PE_BIST_CARRIER_MODE_2:
		pe_state_exit_action = pe_bist_carrier_mode_2_exit;
		break;

	case PE_VCS_SEND_SWAP:
	case PE_PRS_SRC_SNK_SEND_SWAP:
	case PE_PRS_SNK_SRC_SEND_SWAP:
	case PE_DRS_DFP_UFP_SEND_DR_SWAP:
	case PE_DRS_UFP_DFP_SEND_DR_SWAP:
		pe_state_exit_action = pe_exit_action_disable_sender_response;
		break;

	case PE_PRS_SRC_SNK_WAIT_SOURCE_ON:
		pe_state_exit_action = pe_prs_src_snk_wait_source_on_exit;
		break;

	case PE_PRS_SNK_SRC_SOURCE_ON:
		pe_state_exit_action = pe_prs_snk_src_source_on_exit;
		break;

	case PE_PRS_SNK_SRC_TRANSITION_TO_OFF:
		pe_state_exit_action = pe_prs_snk_src_transition_to_off_exit;
		break;

	case PE_VCS_WAIT_FOR_VCONN:
		pe_state_exit_action = pe_vcs_wait_for_vconn_exit;
		break;
	}
}

static inline void print_state(
	struct pd_port_t *pd_port, bool vdm_evt, uint8_t state)
{
	/*
	 * Source (P, Provider), Sink (C, Consumer)
	 * DFP (D), UFP (U)
	 * Vconn Source (Y/N)
	 */

#if PE_DBG_ENABLE
	PE_DBG("%s -> %s (%c%c%c)\r\n",
		vdm_evt ? "VDM" : "PD", pe_state_name[state],
		pd_port->power_role ? 'P' : 'C',
		pd_port->data_role ? 'D' : 'U',
		pd_port->vconn_source ? 'Y' : 'N');
#else
	PE_STATE_INFO("%s-> %s\r\n",
		vdm_evt ? "VDM" : "PD", pe_state_name[state]);
#endif	/* PE_DBG_ENABLE */
}

static void pd_pe_state_change(
	struct pd_port_t *pd_port, struct pd_event_t *pd_event, bool vdm_evt)
{
	void (*prev_exit_action)
		(struct pd_port_t *pd_port, struct pd_event_t *pd_event);
	void (*next_entry_action)
		(struct pd_port_t *pd_port, struct pd_event_t *pd_event);

	uint8_t old_state = pd_port->pe_state_curr;
	uint8_t new_state = pd_port->pe_state_next;

	WARN_ON(old_state >= PD_NR_PE_STATES);
	WARN_ON(new_state >= PD_NR_PE_STATES);

	if ((new_state == PE_IDLE1) || (new_state == PE_IDLE2))
		prev_exit_action = NULL;
	else
		pe_get_exit_action(old_state, prev_exit_action);

	next_entry_action = pe_state_actions[new_state].entry_action;

#if PE_STATE_INFO_VDM_DIS
	if (!vdm_evt)
#endif	/* PE_STATE_INFO_VDM_DIS */
		print_state(pd_port, vdm_evt, new_state);

	if (prev_exit_action)
		prev_exit_action(pd_port, pd_event);

	if (next_entry_action)
		next_entry_action(pd_port, pd_event);

	if (vdm_evt)
		pd_port->pe_vdm_state = new_state;
	else
		pd_port->pe_pd_state = new_state;
}

static int pd_handle_event(
	struct pd_port_t *pd_port, struct pd_event_t *pd_event, bool vdm_evt)
{
	if (vdm_evt) {
		if (pd_port->reset_vdm_state) {
			pd_port->reset_vdm_state = false;
			pd_port->pe_vdm_state = pd_port->pe_pd_state;
		}

		pd_port->pe_state_curr = pd_port->pe_vdm_state;
	} else {
		pd_port->pe_state_curr = pd_port->pe_pd_state;
	}

	if (pd_process_event(pd_port, pd_event, vdm_evt))
		pd_pe_state_change(pd_port, pd_event, vdm_evt);
	else
		pd_free_pd_event(pd_port, pd_event);

	return 1;
}

static inline int pd_put_dpm_ack_immediately(
	struct pd_port_t *pd_port, bool vdm_evt)
{
	struct pd_event_t pd_event = {
		.event_type = PD_EVT_DPM_MSG,
		.msg = PD_DPM_ACK,
		.pd_msg = NULL,
	};

	pd_handle_event(pd_port, &pd_event, vdm_evt);

	PE_DBG("ACK_Immediately\r\n");
	pd_port->dpm_ack_immediately = false;
	return 1;
}

int pd_policy_engine_run(struct tcpc_device *tcpc_dev)
{
	bool vdm_evt = false;
	struct pd_event_t pd_event;
	struct pd_port_t *pd_port = &tcpc_dev->pd_port;

	if (!pd_get_event(tcpc_dev, &pd_event)) {

		switch (pd_port->pe_pd_state) {
		case PE_SNK_READY:
			vdm_evt = pd_get_vdm_event(tcpc_dev, &pd_event);
			break;
		case PE_SRC_READY:
			vdm_evt = pd_get_vdm_event(tcpc_dev, &pd_event);
			break;
		case PE_SRC_STARTUP:
			vdm_evt = pd_get_vdm_event(tcpc_dev, &pd_event);
			break;
		case PE_SRC_DISCOVERY:
			vdm_evt = pd_get_vdm_event(tcpc_dev, &pd_event);
			break;
#ifdef CONFIG_USB_PD_CUSTOM_DBGACC
		case PE_DBG_READY:
#endif	/* CONFIG_USB_PD_CUSTOM_DBGACC */
			vdm_evt = pd_get_vdm_event(tcpc_dev, &pd_event);
			break;
		}

		if (!vdm_evt)
			return 0;
	}

#ifdef CONFIG_TCPC_IDLE_MODE
	tcpci_idle_poll_ctrl(tcpc_dev, true, 1);
#endif
	mutex_lock(&pd_port->pd_lock);

	pd_handle_event(pd_port, &pd_event, vdm_evt);

	if (pd_port->dpm_ack_immediately)
		pd_put_dpm_ack_immediately(pd_port, vdm_evt);

	mutex_unlock(&pd_port->pd_lock);

#ifdef CONFIG_TCPC_IDLE_MODE
	tcpci_idle_poll_ctrl(tcpc_dev, false, 1);
#endif
	return 1;
}
