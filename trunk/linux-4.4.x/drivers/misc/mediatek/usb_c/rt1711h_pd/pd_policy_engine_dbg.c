/*
 * Copyright (C) 2016 Richtek Technology Corp.
 *
 * Power Delivery Policy Engine for DBGACC
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
#include "inc/pd_policy_engine.h"

#ifdef CONFIG_USB_PD_CUSTOM_DBGACC

void pe_dbg_ready_entry(struct pd_port_t *pd_port, struct pd_event_t *pd_event)
{
	uint8_t state;

	if (pd_port->pe_ready)
		return;

	pd_port->pe_ready = true;
	pd_port->state_machine = PE_STATE_MACHINE_DBGACC;

	if (pd_port->data_role == PD_ROLE_UFP) {
		PE_INFO("Custom_DBGACC : UFP\r\n");
		state = PD_CONNECT_PE_READY_DBGACC_UFP;
		pd_set_rx_enable(pd_port, PD_RX_CAP_PE_READY_UFP);
	} else {
		PE_INFO("Custom_DBGACC : DFP\r\n");
		state = PD_CONNECT_PE_READY_DBGACC_DFP;
		pd_set_rx_enable(pd_port, PD_RX_CAP_PE_READY_DFP);
	}

	pd_reset_protocol_layer(pd_port);
	pd_update_connect_state(pd_port, state);
}

#endif /* CONFIG_USB_PD_CUSTOM_DBGACC */
