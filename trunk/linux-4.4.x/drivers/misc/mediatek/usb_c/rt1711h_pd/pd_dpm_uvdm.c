/*
 * Copyright (C) 2016 Richtek Technology Corp.
 *
 * PD Device Policy Manager for UVDM
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

#include "inc/tcpci.h"
#include "inc/pd_policy_engine.h"
#include "inc/pd_dpm_core.h"
#include "pd_dpm_prv.h"

#ifdef CONFIG_USB_PD_RICHTEK_UVDM

bool richtek_dfp_notify_uvdm(struct pd_port_t *pd_port,
				struct svdm_svid_data_t *svid_data, bool ack)
{
	uint32_t resp_cmd = 0;

	if (ack) {
		if (pd_port->uvdm_wait_resp)
			resp_cmd = PD_UVDM_HDR_CMD(pd_port->uvdm_data[0]);

		UVDM_INFO("dfp_notify: ACK (0x%x)\r\n");
	} else
		UVDM_INFO("dfp_notify: NAK\r\n");

	return true;
}

bool richtek_ufp_notify_uvdm(struct pd_port_t *pd_port,
				struct svdm_svid_data_t *svid_data)
{
	uint32_t hdr = PD_UVDM_HDR(0x29cf, 0x1234);
	uint32_t cmd = PD_UVDM_HDR_CMD(pd_port->uvdm_data[0]);

	UVDM_INFO("ufp_notify: %d, resp: 0x%08x\r\n", cmd, hdr);

	if (cmd == 0)
		pd_reply_uvdm(pd_port, TCPC_TX_SOP, 1, &hdr);

	return true;
}

#endif	/* CONFIG_USB_PD_RICHTEK_UVDM */
