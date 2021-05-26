/*
 * Copyright (C) 2016 Richtek Technology Corp.
 *
 * TCPC Type-C Driver for Richtek
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

#include <linux/delay.h>
#include <linux/cpu.h>

#include "inc/tcpci.h"
#include "inc/tcpci_typec.h"
#include "inc/tcpci_timer.h"

#ifdef CONFIG_TYPEC_CAP_TRY_SOURCE
#define CONFIG_TYPEC_CAP_TRY_STATE
#endif

#ifdef CONFIG_TYPEC_CAP_TRY_SINK
#undef	CONFIG_TYPEC_CAP_TRY_STATE
#define CONFIG_TYPEC_CAP_TRY_STATE
#endif

enum TYPEC_WAIT_PS_STATE {
	TYPEC_WAIT_PS_DISABLE = 0,
	TYPEC_WAIT_PS_SNK_VSAFE5V,
	TYPEC_WAIT_PS_SRC_VSAFE0V,
	TYPEC_WAIT_PS_SRC_VSAFE5V,
};

#if TYPEC_DBG_ENABLE
static const char *const typec_wait_ps_name[] = {
	"Disable",
	"SNK_VSafe5V",
	"SRC_VSafe0V",
	"SRC_VSafe5V",
};
#endif	/* TYPEC_DBG_ENABLE */

static inline void typec_wait_ps_change(struct tcpc_device *tcpc_dev,
					enum TYPEC_WAIT_PS_STATE state)
{
#if TYPEC_DBG_ENABLE
	uint8_t old_state = tcpc_dev->typec_wait_ps_change;
	uint8_t new_state = (uint8_t) state;

	if (new_state != old_state)
		TYPEC_DBG("wait_ps=%s\r\n", typec_wait_ps_name[new_state]);
#endif	/* TYPEC_DBG_ENABLE */

#ifdef CONFIG_TYPEC_ATTACHED_SRC_VSAFE0V_TIMEOUT
	if (state == TYPEC_WAIT_PS_SRC_VSAFE0V)
		tcpc_enable_timer(tcpc_dev, TYPEC_RT_TIMER_SAFE0V_TOUT);
#endif	/* CONFIG_TYPEC_ATTACHED_SRC_VSAFE0V_TIMEOUT */

	if (tcpc_dev->typec_wait_ps_change == TYPEC_WAIT_PS_SRC_VSAFE0V
		&& state != TYPEC_WAIT_PS_SRC_VSAFE0V) {
		tcpc_disable_timer(tcpc_dev, TYPEC_RT_TIMER_SAFE0V_DELAY);

#ifdef CONFIG_TYPEC_ATTACHED_SRC_VSAFE0V_TIMEOUT
		tcpc_disable_timer(tcpc_dev, TYPEC_RT_TIMER_SAFE0V_TOUT);
#endif	/* CONFIG_TYPEC_ATTACHED_SRC_VSAFE0V_TIMEOUT */
	}

	tcpc_dev->typec_wait_ps_change = (uint8_t) state;
}

/* #define TYPEC_EXIT_ATTACHED_SRC_NO_DEBOUNCE */
#define TYPEC_EXIT_ATTACHED_SNK_VIA_VBUS

static inline int typec_enable_low_power_mode(
	struct tcpc_device *tcpc_dev, int pull);

#define typec_get_cc1()		\
	tcpc_dev->typec_remote_cc[0]
#define typec_get_cc2()		\
	tcpc_dev->typec_remote_cc[1]
#define typec_get_cc_res()	\
	(tcpc_dev->typec_polarity ? typec_get_cc2() : typec_get_cc1())

#define typec_check_cc1(cc)	\
	(typec_get_cc1() == cc)

#define typec_check_cc2(cc)	\
	(typec_get_cc2() == cc)

#define typec_check_cc(cc1, cc2)	\
	(typec_check_cc1(cc1) && typec_check_cc2(cc2))

#define typec_check_cc1_unequal(cc)	\
	(typec_get_cc1() != cc)

#define typec_check_cc2_unequal(cc)	\
	(typec_get_cc2() != cc)

#define typec_check_cc_unequal(cc1, cc2)	\
	(typec_check_cc1_unequal(cc1) && typec_check_cc2_unequal(cc2))

#define typec_is_drp_toggling() \
	(typec_get_cc1() == TYPEC_CC_DRP_TOGGLING)

#define typec_is_cc_open()	\
	typec_check_cc(TYPEC_CC_VOLT_OPEN, TYPEC_CC_VOLT_OPEN)


/* TYPEC_GET_CC_STATUS */

/*
 * [BLOCK] TYPEC Connection State Definition
 */

enum TYPEC_CONNECTION_STATE {
	typec_disabled = 0,
	typec_errorrecovery,

	typec_unattached_snk,
	typec_unattached_src,

	typec_attachwait_snk,
	typec_attachwait_src,

	typec_attached_snk,
	typec_attached_src,

#ifdef CONFIG_TYPEC_CAP_TRY_SOURCE
	/* Require : Assert Rp
	 * Exit(-> Attached.SRC) : Detect Rd (tPDDebounce).
	 * Exit(-> TryWait.SNK) : Not detect Rd after tDRPTry
	 */
	typec_try_src,

	/* Require : Assert Rd
	 * Exit(-> Attached.SNK) : Detect Rp (tCCDebounce) and Vbus present.
	 * Exit(-> Unattached.SNK) : Not detect Rp (tPDDebounce)
	 */

	typec_trywait_snk,
	typec_trywait_snk_pe,
#endif

#ifdef CONFIG_TYPEC_CAP_TRY_SINK

	/* Require : Assert Rd
	 * Wait for tDRPTry and only then begin monitoring CC.
	 * Exit (-> Attached.SNK) : Detect Rp (tPDDebounce) and Vbus present.
	 * Exit (-> TryWait.SRC) : Not detect Rp for tPDDebounce.
	 */
	typec_try_snk,

	/*
	 * Require : Assert Rp
	 * Exit (-> Attached.SRC) : Detect Rd (tCCDebounce)
	 * Exit (-> Unattached.SNK) : Not detect Rd after tDRPTry
	 */

	typec_trywait_src,
	typec_trywait_src_pe,
#endif	/* CONFIG_TYPEC_CAP_TRY_SINK */

	typec_audioaccessory,
	typec_debugaccessory,

#ifdef CONFIG_TYPEC_CAP_DBGACC_SNK
	typec_attached_dbgacc_snk,
#endif	/* CONFIG_TYPEC_CAP_DBGACC_SNK */

#ifdef CONFIG_TYPEC_CAP_CUSTOM_SRC
	typec_attached_custom_src,
#endif	/* CONFIG_TYPEC_CAP_CUSTOM_SRC */

	typec_unattachwait_pe,	/* Wait Policy Engine go to Idle */
};

static const char *const typec_state_name[] = {
	"Disabled",
	"ErrorRecovery",

	"Unattached.SNK",
	"Unattached.SRC",

	"AttachWait.SNK",
	"AttachWait.SRC",

	"Attached.SNK",
	"Attached.SRC",

#ifdef CONFIG_TYPEC_CAP_TRY_SOURCE
	"Try.SRC",
	"TryWait.SNK",
	"TryWait.SNK.PE",
#endif	/* CONFIG_TYPEC_CAP_TRY_SOURCE */

#ifdef CONFIG_TYPEC_CAP_TRY_SINK
	"Try.SNK",
	"TryWait.SRC",
	"TryWait.SRC.PE",
#endif	/* CONFIG_TYPEC_CAP_TRY_SINK */

	"AudioAccessory",
	"DebugAccessory",

#ifdef CONFIG_TYPEC_CAP_DBGACC_SNK
	"DBGACC.SNK",
#endif	/* CONFIG_TYPEC_CAP_DBGACC_SNK */

#ifdef CONFIG_TYPEC_CAP_CUSTOM_SRC
	"Custom.SRC",
#endif	/* CONFIG_TYPEC_CAP_CUSTOM_SRC */

	"UnattachWait.PE",
};

static inline void typec_transfer_state(struct tcpc_device *tcpc_dev,
					enum TYPEC_CONNECTION_STATE state)
{
	TYPEC_INFO("** %s\r\n", typec_state_name[state]);
	tcpc_dev->typec_state = (uint8_t) state;
}

#define TYPEC_NEW_STATE(state)  \
	(typec_transfer_state(tcpc_dev, state))

/*
 * [BLOCK] TypeC Alert Attach Status Changed
 */

static const char *const typec_attach_name[] = {
	"NULL",
	"SINK",
	"SOURCE",
	"AUDIO",
	"DEBUG",

#ifdef CONFIG_TYPEC_CAP_DBGACC_SNK
	"DBGACC_SNK",
#endif	/* CONFIG_TYPEC_CAP_DBGACC_SNK */

#ifdef CONFIG_TYPEC_CAP_CUSTOM_SRC
	"CUSTOM_SRC",
#endif	/* CONFIG_TYPEC_CAP_CUSTOM_SRC */
};

static int typec_alert_attach_state_change(struct tcpc_device *tcpc_dev)
{
	int ret = 0;

	if (tcpc_dev->typec_attach_old == tcpc_dev->typec_attach_new) {
		TYPEC_DBG("Attached-> %s(repeat)\r\n",
			typec_attach_name[tcpc_dev->typec_attach_new]);
		return 0;
	}

	TYPEC_INFO("Attached-> %s\r\n",
		   typec_attach_name[tcpc_dev->typec_attach_new]);

	/*Report function */
	ret = tcpci_report_usb_port_changed(tcpc_dev);

	tcpc_dev->typec_attach_old = tcpc_dev->typec_attach_new;
	return ret;
}

/*
 * [BLOCK] Unattached Entry
 */

static inline int typec_enable_low_power_mode(
	struct tcpc_device *tcpc_dev, int pull)
{
	int ret = 0;

#ifdef CONFIG_TYPEC_CHECK_LEGACY_CABLE
	if (tcpc_dev->typec_legacy_cable) {
		TYPEC_DBG("LPM_LCOnly\r\n");
		return 0;
	}
#endif	/* CONFIG_TYPEC_CHECK_LEGACY_CABLE */

	if (tcpc_dev->typec_cable_only) {
		TYPEC_DBG("LPM_RaOnly\r\n");

#ifdef CONFIG_TYPEC_CAP_LPM_WAKEUP_WATCHDOG
		if (tcpc_dev->tcpc_flags & TCPC_FLAGS_LPM_WAKEUP_WATCHDOG)
			tcpc_enable_timer(tcpc_dev, TYPEC_TIMER_WAKEUP);
#endif	/* CONFIG_TYPEC_CAP_LPM_WAKEUP_WATCHDOG */

		return 0;
	}

	if (tcpc_dev->typec_lpm != true)
		ret = tcpci_set_low_power_mode(tcpc_dev, true, pull);

	tcpc_dev->typec_lpm = true;
	return ret;
}

static inline int typec_disable_low_power_mode(
	struct tcpc_device *tcpc_dev)
{
	int ret = 0;

	if (tcpc_dev->typec_lpm != false)
		ret = tcpci_set_low_power_mode(tcpc_dev, false, TYPEC_CC_DRP);

	tcpc_dev->typec_lpm = false;
	return ret;
}

static void typec_unattached_power_entry(struct tcpc_device *tcpc_dev)
{
	typec_wait_ps_change(tcpc_dev, TYPEC_WAIT_PS_DISABLE);

	if (tcpc_dev->typec_power_ctrl) {
		tcpci_set_vconn(tcpc_dev, false);
		tcpci_disable_vbus_control(tcpc_dev);
		tcpci_report_power_control(tcpc_dev, false);
	}
}

static void typec_unattached_entry(struct tcpc_device *tcpc_dev)
{
	typec_unattached_power_entry(tcpc_dev);

	switch (tcpc_dev->typec_role) {
	case TYPEC_ROLE_SNK:
		TYPEC_NEW_STATE(typec_unattached_snk);
		tcpci_set_cc(tcpc_dev, TYPEC_CC_RD);
		typec_enable_low_power_mode(tcpc_dev, TYPEC_CC_RD);
		break;
	case TYPEC_ROLE_SRC:
		TYPEC_NEW_STATE(typec_unattached_src);
		tcpci_set_cc(tcpc_dev, TYPEC_CC_RP);
		typec_enable_low_power_mode(tcpc_dev, TYPEC_CC_RP);
		break;
	default:
		switch (tcpc_dev->typec_state) {
		case typec_attachwait_snk:
		case typec_audioaccessory:
			TYPEC_NEW_STATE(typec_unattached_src);
			tcpci_set_cc(tcpc_dev, TYPEC_CC_RP);
			tcpc_enable_timer(tcpc_dev, TYPEC_TIMER_DRP_SRC_TOGGLE);
			break;
		default:
			TYPEC_NEW_STATE(typec_unattached_snk);
			tcpci_set_cc(tcpc_dev, TYPEC_CC_DRP);
			typec_enable_low_power_mode(tcpc_dev, TYPEC_CC_DRP);
			break;
		}
		break;
	}
}

static void typec_unattach_wait_pe_idle_entry(struct tcpc_device *tcpc_dev)
{
	tcpc_dev->typec_attach_new = TYPEC_UNATTACHED;

#ifdef CONFIG_USB_POWER_DELIVERY
	if (tcpc_dev->typec_attach_old) {
		TYPEC_NEW_STATE(typec_unattachwait_pe);
		return;
	}
#endif

	typec_unattached_entry(tcpc_dev);
}

/*
 * [BLOCK] Attached Entry
 */

static inline int typec_set_polarity(struct tcpc_device *tcpc_dev,
					bool polarity)
{
	tcpc_dev->typec_polarity = polarity;
	return tcpci_set_polarity(tcpc_dev, polarity);
}

static inline int typec_set_plug_orient(struct tcpc_device *tcpc_dev,
				uint8_t res, bool polarity)
{
	int rv = typec_set_polarity(tcpc_dev, polarity);

	if (rv)
		return rv;

	return tcpci_set_cc(tcpc_dev, res);
}

static void typec_source_attached_with_vbus_entry(struct tcpc_device *tcpc_dev)
{
	tcpc_dev->typec_attach_new = TYPEC_ATTACHED_SRC;
	typec_wait_ps_change(tcpc_dev, TYPEC_WAIT_PS_DISABLE);
}

static inline void typec_source_attached_entry(struct tcpc_device *tcpc_dev)
{
	TYPEC_NEW_STATE(typec_attached_src);
	typec_wait_ps_change(tcpc_dev, TYPEC_WAIT_PS_SRC_VSAFE5V);

	tcpc_disable_timer(tcpc_dev, TYPEC_TRY_TIMER_DRP_TRY);

	typec_set_plug_orient(tcpc_dev,
		tcpc_dev->typec_local_rp_level,
		typec_check_cc2(TYPEC_CC_VOLT_RD));

	tcpci_report_power_control(tcpc_dev, true);
	tcpci_set_vconn(tcpc_dev, true);
	tcpci_source_vbus(tcpc_dev,
			TCP_VBUS_CTRL_TYPEC, TCPC_VBUS_SOURCE_5V, -1);
}

static inline void typec_sink_attached_entry(struct tcpc_device *tcpc_dev)
{
	TYPEC_NEW_STATE(typec_attached_snk);
	typec_wait_ps_change(tcpc_dev, TYPEC_WAIT_PS_DISABLE);

#ifdef CONFIG_TYPEC_CHECK_LEGACY_CABLE
	tcpc_dev->typec_legacy_cable = false;
	tcpc_dev->typec_legacy_cable_suspect = 0;
#endif	/* CONFIG_TYPEC_CHECK_LEGACY_CABLE */

	tcpc_dev->typec_attach_new = TYPEC_ATTACHED_SNK;

#ifdef CONFIG_TYPEC_CAP_TRY_STATE
	if (tcpc_dev->typec_role >= TYPEC_ROLE_DRP)
		tcpc_reset_typec_try_timer(tcpc_dev);
#endif	/* CONFIG_TYPEC_CAP_TRY_STATE */

	typec_set_plug_orient(tcpc_dev, TYPEC_CC_RD,
		typec_check_cc2_unequal(TYPEC_CC_VOLT_OPEN));
	tcpc_dev->typec_remote_rp_level = typec_get_cc_res();

	tcpci_report_power_control(tcpc_dev, true);
	tcpci_sink_vbus(tcpc_dev, TCP_VBUS_CTRL_TYPEC, TCPC_VBUS_SINK_5V, -1);
}

static inline void typec_custom_src_attached_entry(
	struct tcpc_device *tcpc_dev)
{
#ifdef CONFIG_TYPEC_CAP_CUSTOM_SRC
	int cc1 = typec_get_cc1();
	int cc2 = typec_get_cc2();

	if (cc1 == TYPEC_CC_VOLT_SNK_DFT && cc2 == TYPEC_CC_VOLT_SNK_DFT) {
		TYPEC_NEW_STATE(typec_attached_custom_src);
		tcpc_dev->typec_attach_new = TYPEC_ATTACHED_CUSTOM_SRC;
		return;
	}
#endif	/* CONFIG_TYPEC_CAP_CUSTOM_SRC */

#ifdef CONFIG_TYPEC_CAP_DBGACC_SNK
	TYPEC_DBG("[Warning] Same Rp (%d)\r\n", typec_get_cc1());
#else
	TYPEC_DBG("[Warning] CC Both Rp\r\n");
#endif
}

#ifdef CONFIG_TYPEC_CAP_DBGACC_SNK

static inline uint8_t typec_get_sink_dbg_acc_rp_level(
	int cc1, int cc2)
{
	if (cc2 == TYPEC_CC_VOLT_SNK_DFT)
		return cc1;
	else
		return TYPEC_CC_VOLT_SNK_DFT;
}

static inline void typec_sink_dbg_acc_attached_entry(
	struct tcpc_device *tcpc_dev)
{
	bool polarity;
	uint8_t rp_level;

	int cc1 = typec_get_cc1();
	int cc2 = typec_get_cc2();

	if (cc1 == cc2) {
		typec_custom_src_attached_entry(tcpc_dev);
		return;
	}

	TYPEC_NEW_STATE(typec_attached_dbgacc_snk);

	tcpc_dev->typec_attach_new = TYPEC_ATTACHED_DBGACC_SNK;

	polarity = cc2 > cc1;

	if (polarity)
		rp_level = typec_get_sink_dbg_acc_rp_level(cc2, cc1);
	else
		rp_level = typec_get_sink_dbg_acc_rp_level(cc1, cc2);

	typec_set_plug_orient(tcpc_dev, TYPEC_CC_RD, polarity);
	tcpc_dev->typec_remote_rp_level = rp_level;

	tcpci_report_power_control(tcpc_dev, true);
	tcpci_sink_vbus(tcpc_dev, TCP_VBUS_CTRL_TYPEC, TCPC_VBUS_SINK_5V, -1);
}
#else
static inline void typec_sink_dbg_acc_attached_entry(
	struct tcpc_device *tcpc_dev)
{
	typec_custom_src_attached_entry(tcpc_dev);
}
#endif	/* CONFIG_TYPEC_CAP_DBGACC_SNK */


/*
 * [BLOCK] Try.SRC / TryWait.SNK
 */

#ifdef CONFIG_TYPEC_CAP_TRY_SOURCE

static inline void typec_try_src_entry(struct tcpc_device *tcpc_dev)
{
	TYPEC_NEW_STATE(typec_try_src);
	tcpc_dev->typec_drp_try_timeout = false;

	tcpci_set_cc(tcpc_dev, TYPEC_CC_RP);
	tcpc_enable_timer(tcpc_dev, TYPEC_TRY_TIMER_DRP_TRY);
}

static inline void typec_trywait_snk_entry(struct tcpc_device *tcpc_dev)
{
	TYPEC_NEW_STATE(typec_trywait_snk);
	typec_wait_ps_change(tcpc_dev, TYPEC_WAIT_PS_DISABLE);

	tcpci_set_vconn(tcpc_dev, false);
	tcpci_set_cc(tcpc_dev, TYPEC_CC_RD);
	tcpci_source_vbus(tcpc_dev,
			TCP_VBUS_CTRL_TYPEC, TCPC_VBUS_SOURCE_0V, 0);
	tcpc_disable_timer(tcpc_dev, TYPEC_TRY_TIMER_DRP_TRY);

	tcpc_enable_timer(tcpc_dev, TYPEC_TIMER_PDDEBOUNCE);
}

static inline void typec_trywait_snk_pe_entry(struct tcpc_device *tcpc_dev)
{
	tcpc_dev->typec_attach_new = TYPEC_UNATTACHED;

#ifdef CONFIG_USB_POWER_DELIVERY
	if (tcpc_dev->typec_attach_old) {
		TYPEC_NEW_STATE(typec_trywait_snk_pe);
		return;
	}
#endif

	typec_trywait_snk_entry(tcpc_dev);
}

#endif /* #ifdef CONFIG_TYPEC_CAP_TRY_SOURCE */

/*
 * [BLOCK] Try.SNK / TryWait.SRC
 */

#ifdef CONFIG_TYPEC_CAP_TRY_SINK

static inline void typec_try_snk_entry(struct tcpc_device *tcpc_dev)
{
	TYPEC_NEW_STATE(typec_try_snk);
	tcpc_dev->typec_drp_try_timeout = false;

	tcpci_set_cc(tcpc_dev, TYPEC_CC_RD);
	tcpc_enable_timer(tcpc_dev, TYPEC_TRY_TIMER_DRP_TRY);
}

static inline void typec_trywait_src_entry(struct tcpc_device *tcpc_dev)
{
	TYPEC_NEW_STATE(typec_trywait_src);
	tcpc_dev->typec_drp_try_timeout = false;

	tcpci_set_cc(tcpc_dev, TYPEC_CC_RP);
	tcpci_sink_vbus(tcpc_dev, TCP_VBUS_CTRL_TYPEC, TCPC_VBUS_SINK_0V, 0);
	tcpc_enable_timer(tcpc_dev, TYPEC_TRY_TIMER_DRP_TRY);
}

#ifndef CONFIG_USB_POWER_DELIVERY
static inline void typec_trywait_src_pe_entry(struct tcpc_device *tcpc_dev)
{
	tcpc_dev->typec_attach_new = TYPEC_UNATTACHED;

	if (tcpc_dev->typec_attach_old) {
		TYPEC_NEW_STATE(typec_trywait_src_pe);
		return;
	}

	typec_trywait_src_entry(tcpc_dev);
}
#endif

#endif /* CONFIG_TYPEC_CAP_TRY_SINK */

/*
 * [BLOCK] Attach / Detach
 */

static inline void typec_cc_snk_detect_vsafe5v_entry(
	struct tcpc_device *tcpc_dev)
{
	typec_wait_ps_change(tcpc_dev, TYPEC_WAIT_PS_DISABLE);

	if (typec_check_cc_unequal(TYPEC_CC_VOLT_OPEN, TYPEC_CC_VOLT_OPEN)) {
		typec_sink_dbg_acc_attached_entry(tcpc_dev);
		return;
	}

#ifdef CONFIG_TYPEC_CAP_TRY_SOURCE
	if (tcpc_dev->typec_role == TYPEC_ROLE_TRY_SRC) {
		if (tcpc_dev->typec_state == typec_attachwait_snk) {
			typec_try_src_entry(tcpc_dev);
			return;
		}
	}
#endif /* CONFIG_TYPEC_CAP_TRY_SOURCE */

	typec_sink_attached_entry(tcpc_dev);
}

static inline void typec_cc_snk_detect_entry(struct tcpc_device *tcpc_dev)
{
	uint8_t cc_res;

	if (tcpc_dev->typec_attach_old == TYPEC_ATTACHED_SNK) {
		cc_res = typec_get_cc_res();
		if (cc_res != tcpc_dev->typec_remote_rp_level) {
			TYPEC_INFO("RpLvl Change\r\n");
			tcpc_dev->typec_remote_rp_level = cc_res;
			tcpci_sink_vbus(tcpc_dev,
				TCP_VBUS_CTRL_TYPEC, TCPC_VBUS_SINK_5V, -1);
		}
		return;
	}

	/* If Port Partner act as Source without VBUS, wait vSafe5V */
	if (tcpci_check_vbus_valid(tcpc_dev))
		typec_cc_snk_detect_vsafe5v_entry(tcpc_dev);
	else
		typec_wait_ps_change(tcpc_dev, TYPEC_WAIT_PS_SNK_VSAFE5V);
}

static inline void typec_cc_src_detect_vsafe0v_entry(
	struct tcpc_device *tcpc_dev)
{
	typec_wait_ps_change(tcpc_dev, TYPEC_WAIT_PS_DISABLE);

#ifdef CONFIG_TYPEC_CAP_TRY_SINK
	if (tcpc_dev->typec_role == TYPEC_ROLE_TRY_SNK) {
		if (tcpc_dev->typec_state == typec_attachwait_src) {
			typec_try_snk_entry(tcpc_dev);
			return;
		}
	}
#endif /* CONFIG_TYPEC_CAP_TRY_SINK */

	typec_source_attached_entry(tcpc_dev);
}

static inline void typec_cc_src_detect_entry(
	struct tcpc_device *tcpc_dev)
{
	/* If Port Partner act as Sink with low VBUS, wait vSafe0v */
	bool vbus_absent = tcpci_check_vsafe0v(tcpc_dev, true);

	if (vbus_absent)
		typec_cc_src_detect_vsafe0v_entry(tcpc_dev);
	else
		typec_wait_ps_change(tcpc_dev, TYPEC_WAIT_PS_SRC_VSAFE0V);
}

static inline void typec_cc_src_remove_entry(struct tcpc_device *tcpc_dev)
{
	typec_wait_ps_change(tcpc_dev, TYPEC_WAIT_PS_DISABLE);

#ifdef CONFIG_TYPEC_CAP_TRY_SOURCE
	if (tcpc_dev->typec_role == TYPEC_ROLE_TRY_SRC) {
		switch (tcpc_dev->typec_state) {
		case typec_attached_src:
			typec_trywait_snk_pe_entry(tcpc_dev);
			return;
		case typec_try_src:
			typec_trywait_snk_entry(tcpc_dev);
			return;
		}
	}
#endif	/* CONFIG_TYPEC_CAP_TRY_SOURCE */

	typec_unattach_wait_pe_idle_entry(tcpc_dev);
}

static inline void typec_cc_snk_remove_entry(struct tcpc_device *tcpc_dev)
{
	typec_wait_ps_change(tcpc_dev, TYPEC_WAIT_PS_DISABLE);

#ifdef CONFIG_TYPEC_CAP_TRY_SINK
	if (tcpc_dev->typec_state == typec_try_snk) {
		typec_trywait_src_entry(tcpc_dev);
		return;
	}
#endif	/* CONFIG_TYPEC_CAP_TRY_SINK */

	typec_unattach_wait_pe_idle_entry(tcpc_dev);
}

/*
 * [BLOCK] CC Change (after debounce)
 */

#ifdef CONFIG_TYPEC_CHECK_CC_STABLE

static inline bool typec_check_cc_stable_source(
	struct tcpc_device *tcpc_dev)
{
	int ret, cc1a, cc2a, cc1b, cc2b;
	bool check_stable = false;

	if (!(tcpc_dev->tcpc_flags & TCPC_FLAGS_CHECK_CC_STABLE))
		return true;

	cc1a = typec_get_cc1();
	cc2a = typec_get_cc2();

	if ((cc1a == TYPEC_CC_VOLT_RD) && (cc2a == TYPEC_CC_VOLT_RD))
		check_stable = true;

	if ((cc1a == TYPEC_CC_VOLT_RA) || (cc2a == TYPEC_CC_VOLT_RA))
		check_stable = true;

	if (check_stable) {
		TYPEC_INFO("CC Stable Check...\r\n");
		typec_set_polarity(tcpc_dev, !tcpc_dev->typec_polarity);
		mdelay(1);

		ret = tcpci_get_cc(tcpc_dev);
		cc1b = typec_get_cc1();
		cc2b = typec_get_cc2();

		if ((cc1b != cc1a) || (cc2b != cc2a)) {
			TYPEC_INFO("CC Unstable... %d/%d\r\n", cc1b, cc2b);

			if ((cc1b == TYPEC_CC_VOLT_RD) &&
				(cc2b != TYPEC_CC_VOLT_RD))
				return true;

			if ((cc1b != TYPEC_CC_VOLT_RD) &&
				(cc2b == TYPEC_CC_VOLT_RD))
				return true;

			typec_cc_src_remove_entry(tcpc_dev);
			return false;
		}

		typec_set_polarity(tcpc_dev, !tcpc_dev->typec_polarity);
		mdelay(1);

		ret = tcpci_get_cc(tcpc_dev);
		cc1b = typec_get_cc1();
		cc2b = typec_get_cc2();

		if ((cc1b != cc1a) || (cc2b != cc2a)) {
			TYPEC_INFO("CC Unstable1... %d/%d\r\n", cc1b, cc2b);

			if ((cc1b == TYPEC_CC_VOLT_RD) &&
						(cc2b != TYPEC_CC_VOLT_RD))
				return true;

			if ((cc1b != TYPEC_CC_VOLT_RD) &&
						(cc2b == TYPEC_CC_VOLT_RD))
				return true;

			typec_cc_src_remove_entry(tcpc_dev);
			return false;
		}
	}

	return true;
}

static inline bool typec_check_cc_stable_sink(
	struct tcpc_device *tcpc_dev)
{
	int ret, cc1a, cc2a, cc1b, cc2b;

	if (!(tcpc_dev->tcpc_flags & TCPC_FLAGS_CHECK_CC_STABLE))
		return true;

	cc1a = typec_get_cc1();
	cc2a = typec_get_cc2();

	if ((cc1a != TYPEC_CC_VOLT_OPEN) && (cc2a != TYPEC_CC_VOLT_OPEN)) {
		TYPEC_INFO("CC Stable Check...\r\n");
		typec_set_polarity(tcpc_dev, !tcpc_dev->typec_polarity);
		mdelay(1);

		ret = tcpci_get_cc(tcpc_dev);
		cc1b = typec_get_cc1();
		cc2b = typec_get_cc2();

		if ((cc1b != cc1a) || (cc2b != cc2a))
			TYPEC_INFO("CC Unstable... %d/%d\r\n", cc1b, cc2b);
	}

	return true;
}

#endif

#ifdef CONFIG_TYPEC_CHECK_LEGACY_CABLE

static inline bool typec_check_legacy_cable(
	struct tcpc_device *tcpc_dev)
{
	bool check_legacy = false;

	if (typec_check_cc(TYPEC_CC_VOLT_RD, TYPEC_CC_VOLT_OPEN) ||
		typec_check_cc(TYPEC_CC_VOLT_OPEN, TYPEC_CC_VOLT_RD))
		check_legacy = true;

	if (check_legacy &&
		tcpc_dev->typec_legacy_cable_suspect >=
					TCPC_LEGACY_CABLE_CONFIRM) {

		TYPEC_INFO("LC->Suspect\r\n");
		tcpci_set_cc(tcpc_dev, TYPEC_CC_RP_1_5);
		tcpc_dev->typec_legacy_cable_suspect = 0;
		mdelay(1);

		if (tcpci_get_cc(tcpc_dev) != 0) {
			TYPEC_INFO("LC->Confirm\r\n");
			tcpc_dev->typec_legacy_cable = true;
			return true;
		}

		tcpc_dev->typec_legacy_cable = false;
		tcpci_set_cc(tcpc_dev, TYPEC_CC_RP);
	}

	return false;
}

#endif	/* CONFIG_TYPEC_CHECK_LEGACY_CABLE */

static inline bool typec_cc_change_source_entry(struct tcpc_device *tcpc_dev)
{
	int cc1, cc2;

#ifdef CONFIG_TYPEC_CHECK_CC_STABLE
	if (!typec_check_cc_stable_source(tcpc_dev))
		return true;
#endif

	cc1 = typec_get_cc1();
	cc2 = typec_get_cc2();

	if ((cc1 == TYPEC_CC_VOLT_RD) && (cc2 == TYPEC_CC_VOLT_RD)) {
		TYPEC_NEW_STATE(typec_debugaccessory);
		TYPEC_DBG("[Debug] CC1&2 Both Rd\r\n");
		tcpc_dev->typec_attach_new = TYPEC_ATTACHED_DEBUG;
	} else if ((cc1 == TYPEC_CC_VOLT_RA) && (cc2 == TYPEC_CC_VOLT_RA)) {
		TYPEC_NEW_STATE(typec_audioaccessory);
		TYPEC_DBG("[Audio] CC1&2 Both Ra\r\n");
		tcpc_dev->typec_attach_new = TYPEC_ATTACHED_AUDIO;
	} else {
		if ((cc1 == TYPEC_CC_VOLT_RD) || (cc2 == TYPEC_CC_VOLT_RD))
			typec_cc_src_detect_entry(tcpc_dev);
		else {
			if ((cc1 == TYPEC_CC_VOLT_RA)
			    || (cc2 == TYPEC_CC_VOLT_RA))
				TYPEC_DBG("[Cable] Ra Only\r\n");
			typec_cc_src_remove_entry(tcpc_dev);
		}
	}

	return true;
}

static inline bool typec_cc_change_sink_entry(struct tcpc_device *tcpc_dev)
{
#ifdef CONFIG_TYPEC_CHECK_CC_STABLE
	typec_check_cc_stable_sink(tcpc_dev);
#endif

	if (typec_is_cc_open())
		typec_cc_snk_remove_entry(tcpc_dev);
	else
		typec_cc_snk_detect_entry(tcpc_dev);

	return true;
}

static inline bool typec_is_act_as_sink_role(
	struct tcpc_device *tcpc_dev)
{
	bool as_sink = true;
	uint8_t cc_sum;

	switch (tcpc_dev->typec_local_cc & 0x07) {
	case TYPEC_CC_RP:
		as_sink = false;
		break;
	case TYPEC_CC_RD:
		as_sink = true;
		break;
	case TYPEC_CC_DRP:
		cc_sum = typec_get_cc1() + typec_get_cc2();
		as_sink = (cc_sum >= TYPEC_CC_VOLT_SNK_DFT);
		break;
	}

	return as_sink;
}

static inline bool typec_handle_cc_changed_entry(struct tcpc_device *tcpc_dev)
{
	TYPEC_INFO("[CC_Change] %d/%d\r\n", typec_get_cc1(), typec_get_cc2());

	tcpc_dev->typec_attach_new = tcpc_dev->typec_attach_old;

	if (typec_is_act_as_sink_role(tcpc_dev))
		typec_cc_change_sink_entry(tcpc_dev);
	else
		typec_cc_change_source_entry(tcpc_dev);

	typec_alert_attach_state_change(tcpc_dev);
	return true;
}

/*
 * [BLOCK] Handle cc-change event
 */

static inline void typec_attach_wait_entry(struct tcpc_device *tcpc_dev)
{
	bool as_sink;

#ifdef CONFIG_USB_POWER_DELIVERY
	bool pd_en = tcpc_dev->pd_port.pd_prev_connected;
#else
	bool pd_en = false;
#endif	/* CONFIG_USB_POWER_DELIVERY */

	if (tcpc_dev->typec_attach_old == TYPEC_ATTACHED_SNK && !pd_en) {
		tcpc_enable_timer(tcpc_dev, TYPEC_TIMER_PDDEBOUNCE);
		TYPEC_DBG("RpLvl Alert\r\n");
		return;
	}

	if (tcpc_dev->typec_attach_old ||
		tcpc_dev->typec_state == typec_attached_src) {
		tcpc_reset_typec_debounce_timer(tcpc_dev);
		TYPEC_DBG("Attached, Ignore cc_attach\r\n");
		return;
	}

	switch (tcpc_dev->typec_state) {

#ifdef CONFIG_TYPEC_CAP_TRY_SOURCE
	case typec_try_src:
		tcpc_enable_timer(tcpc_dev, TYPEC_TIMER_PDDEBOUNCE);
		return;

	case typec_trywait_snk:
		tcpc_enable_timer(tcpc_dev, TYPEC_TIMER_CCDEBOUNCE);
		return;
#endif

#ifdef CONFIG_TYPEC_CAP_TRY_SINK
	case typec_try_snk:
		tcpc_enable_timer(tcpc_dev, TYPEC_TIMER_PDDEBOUNCE);
		return;

	case typec_trywait_src:
		tcpc_enable_timer(tcpc_dev, TYPEC_TIMER_CCDEBOUNCE);
		return;
#endif

#ifdef CONFIG_USB_POWER_DELIVERY
	case typec_unattachwait_pe:
		TYPEC_INFO("Force PE Idle\r\n");
		tcpc_dev->pd_wait_pe_idle = false;
		tcpc_disable_timer(tcpc_dev, TYPEC_RT_TIMER_PE_IDLE);
		typec_unattached_power_entry(tcpc_dev);
		break;
#endif
	}

	as_sink = typec_is_act_as_sink_role(tcpc_dev);

#ifdef CONFIG_TYPEC_CHECK_LEGACY_CABLE
	if (!as_sink && typec_check_legacy_cable(tcpc_dev))
		return;
#endif	/* CONFIG_TYPEC_CHECK_LEGACY_CABLE */

	if (as_sink)
		TYPEC_NEW_STATE(typec_attachwait_snk);
	else
		TYPEC_NEW_STATE(typec_attachwait_src);

	tcpc_enable_timer(tcpc_dev, TYPEC_TIMER_CCDEBOUNCE);
}

#ifdef TYPEC_EXIT_ATTACHED_SNK_VIA_VBUS
static inline int typec_attached_snk_cc_detach(struct tcpc_device *tcpc_dev)
{
	int vbus_valid = tcpci_check_vbus_valid(tcpc_dev);
	bool detach_by_cc = false;

	/* For Ellisys Test, Applying Low VBUS (3.67v) as Sink */
	if (vbus_valid) {
		detach_by_cc = true;
		TYPEC_DBG("Detach_CC (LowVBUS)\r\n");
	}

#ifdef CONFIG_USB_POWER_DELIVERY
	/* For Source detach during HardReset */
	if ((!vbus_valid) &&
		tcpc_dev->pd_wait_hard_reset_complete) {
		detach_by_cc = true;
		TYPEC_DBG("Detach_CC (HardReset)\r\n");
	}
#endif

	if (detach_by_cc)
		tcpc_enable_timer(tcpc_dev, TYPEC_TIMER_PDDEBOUNCE);

	return 0;
}
#endif	/* TYPEC_EXIT_ATTACHED_SNK_VIA_VBUS */

static inline void typec_detach_wait_entry(struct tcpc_device *tcpc_dev)
{
#ifdef CONFIG_TYPEC_CHECK_LEGACY_CABLE
	bool suspect_legacy = false;

	if (tcpc_dev->typec_state == typec_attachwait_src)
		suspect_legacy = true;
	else if (tcpc_dev->typec_state == typec_attached_src) {
		if (tcpc_dev->typec_attach_old != TYPEC_ATTACHED_SRC)
			suspect_legacy = true;
		else {
			tcpc_dev->typec_legacy_cable = false;
			tcpc_dev->typec_legacy_cable_suspect = 0;
		}
	}

	if (suspect_legacy) {
		tcpc_dev->typec_legacy_cable_suspect++;
		TYPEC_DBG("LC_suspect: %d\r\n",
			tcpc_dev->typec_legacy_cable_suspect);
	}
#endif	/* CONFIG_TYPEC_CHECK_LEGACY_CABLE */

	switch (tcpc_dev->typec_state) {
#ifdef TYPEC_EXIT_ATTACHED_SNK_VIA_VBUS
	case typec_attached_snk:
		typec_attached_snk_cc_detach(tcpc_dev);
		break;
#endif /* TYPEC_EXIT_ATTACHED_SNK_VIA_VBUS */

	case typec_audioaccessory:
		tcpc_enable_timer(tcpc_dev, TYPEC_TIMER_CCDEBOUNCE);
		break;

#ifdef TYPEC_EXIT_ATTACHED_SRC_NO_DEBOUNCE
	case typec_attached_src:
		TYPEC_INFO("Exit Attached.SRC immediately\r\n");
		tcpc_reset_typec_debounce_timer(tcpc_dev);

		/* force to terminate TX */
		tcpci_init(tcpc_dev, true);

		typec_cc_src_remove_entry(tcpc_dev);
		typec_alert_attach_state_change(tcpc_dev);
		break;
#endif /* TYPEC_EXIT_ATTACHED_SRC_NO_DEBOUNCE */

#ifdef CONFIG_TYPEC_CAP_TRY_SOURCE
	case typec_try_src:
		if (tcpc_dev->typec_drp_try_timeout)
			tcpc_enable_timer(tcpc_dev, TYPEC_TIMER_PDDEBOUNCE);
		else {
			tcpc_reset_typec_debounce_timer(tcpc_dev);
			TYPEC_DBG("[Try] Igrone cc_detach\r\n");
		}
		break;
#endif	/* CONFIG_TYPEC_CAP_TRY_SOURCE */

#ifdef CONFIG_TYPEC_CAP_TRY_SINK
	case typec_trywait_src:
		if (tcpc_dev->typec_drp_try_timeout)
			tcpc_enable_timer(tcpc_dev, TYPEC_TIMER_PDDEBOUNCE);
		else {
			tcpc_reset_typec_debounce_timer(tcpc_dev);
			TYPEC_DBG("[Try] Igrone cc_detach\r\n");
		}
		break;
#endif	/* CONFIG_TYPEC_CAP_TRY_SINK */
	default:
		tcpc_enable_timer(tcpc_dev, TYPEC_TIMER_PDDEBOUNCE);
		break;
	}
}

static inline bool typec_is_cc_attach(struct tcpc_device *tcpc_dev)
{
	bool cc_attach = false;
	int cc1 = typec_get_cc1();
	int cc2 = typec_get_cc2();
	int cc_res = typec_get_cc_res();

	tcpc_dev->typec_cable_only = false;

	if (tcpc_dev->typec_attach_old) {
		if ((cc_res != TYPEC_CC_VOLT_OPEN) &&
				(cc_res != TYPEC_CC_VOLT_RA))
			cc_attach = true;
	} else {
		if (cc1 != TYPEC_CC_VOLT_OPEN)
			cc_attach = true;

		if (cc2 != TYPEC_CC_VOLT_OPEN)
			cc_attach = true;

		/* Cable Only, no device */
		if ((cc1+cc2) == TYPEC_CC_VOLT_RA) {
			cc_attach = false;
			tcpc_dev->typec_cable_only = true;
		}
	}

	return cc_attach;
}

int tcpc_typec_handle_cc_change(struct tcpc_device *tcpc_dev)
{
	int ret = tcpci_get_cc(tcpc_dev);

	if (ret < 0)
		return ret;

	if (typec_is_drp_toggling()) {
		TYPEC_DBG("[Waring] DRP Toggling\r\n");
		if (tcpc_dev->typec_lpm)
			tcpci_set_low_power_mode(tcpc_dev, true, TYPEC_CC_DRP);
		return 0;
	}

	TYPEC_INFO("[CC_Alert] %d/%d\r\n", typec_get_cc1(), typec_get_cc2());

	typec_disable_low_power_mode(tcpc_dev);

#ifdef CONFIG_TYPEC_CHECK_LEGACY_CABLE
	if (tcpc_dev->typec_legacy_cable && !typec_is_cc_open()) {
		TYPEC_INFO("LC->Detached (CC)\r\n");
		tcpc_dev->typec_legacy_cable = false;
		tcpci_set_cc(tcpc_dev, TYPEC_CC_DRP);
		typec_enable_low_power_mode(tcpc_dev, TYPEC_CC_DRP);
		return 0;
	}
#endif	/* CONFIG_TYPEC_CHECK_LEGACY_CABLE */

#ifdef CONFIG_USB_POWER_DELIVERY
	if (tcpc_dev->pd_wait_pr_swap_complete) {
		TYPEC_DBG("[PR.Swap] Ignore CC_Alert\r\n");
		return 0;
	}

	if (tcpc_dev->pd_wait_error_recovery) {
		TYPEC_DBG("[Recovery] Ignore CC_Alert\r\n");
		return 0;
	}
#endif /* CONFIG_USB_POWER_DELIVERY */

#ifdef CONFIG_TYPEC_CAP_TRY_SINK
	if ((tcpc_dev->typec_state == typec_try_snk) &&
		(!tcpc_dev->typec_drp_try_timeout)) {
		TYPEC_DBG("[Try.SNK] Ignore CC_Alert\r\n");
		return 0;
	}

	if (tcpc_dev->typec_state == typec_trywait_src_pe) {
		TYPEC_DBG("[Try.PE] Ignore CC_Alert\r\n");
		return 0;
	}
#endif	/* CONFIG_TYPEC_CAP_TRY_SINK */

#ifdef CONFIG_TYPEC_CAP_TRY_SOURCE
	if (tcpc_dev->typec_state == typec_trywait_snk_pe) {
		TYPEC_DBG("[Try.PE] Ignore CC_Alert\r\n");
		return 0;
	}
#endif	/* CONFIG_TYPEC_CAP_TRY_SOURCE */

	if (tcpc_dev->typec_state == typec_attachwait_snk
		|| tcpc_dev->typec_state == typec_attachwait_src)
		typec_wait_ps_change(tcpc_dev, TYPEC_WAIT_PS_DISABLE);

	if (typec_is_cc_attach(tcpc_dev))
		typec_attach_wait_entry(tcpc_dev);
	else
		typec_detach_wait_entry(tcpc_dev);

	return 0;
}

/*
 * [BLOCK] Handle timeout event
 */

#ifdef CONFIG_TYPEC_CAP_TRY_STATE
static inline int typec_handle_drp_try_timeout(struct tcpc_device *tcpc_dev)
{
	bool src_detect = false, en_timer;

	tcpc_dev->typec_drp_try_timeout = true;
	tcpc_disable_timer(tcpc_dev, TYPEC_TRY_TIMER_DRP_TRY);

	if (typec_is_drp_toggling()) {
		TYPEC_DBG("[Waring] DRP Toggling\r\n");
		return 0;
	}

	if (typec_check_cc1(TYPEC_CC_VOLT_RD) ||
		typec_check_cc2(TYPEC_CC_VOLT_RD)) {
		src_detect = true;
	}

	switch (tcpc_dev->typec_state) {
#ifdef CONFIG_TYPEC_CAP_TRY_SOURCE
	case typec_try_src:
		en_timer = !src_detect;
		break;
#endif /* CONFIG_TYPEC_CAP_TRY_SOURCE */

#ifdef CONFIG_TYPEC_CAP_TRY_SINK
	case typec_trywait_src:
		en_timer = !src_detect;
		break;

	case typec_try_snk:
		en_timer = true;
		break;
#endif /* CONFIG_TYPEC_CAP_TRY_SINK */

	default:
		en_timer = false;
		break;
	}

	if (en_timer)
		tcpc_enable_timer(tcpc_dev, TYPEC_TIMER_PDDEBOUNCE);

	return 0;
}
#endif	/* CONFIG_TYPEC_CAP_TRY_STATE */

static inline int typec_handle_debounce_timeout(struct tcpc_device *tcpc_dev)
{
	if (typec_is_drp_toggling()) {
		TYPEC_DBG("[Waring] DRP Toggling\r\n");
		return 0;
	}

	typec_handle_cc_changed_entry(tcpc_dev);
	return 0;
}

#ifdef CONFIG_USB_POWER_DELIVERY

static inline int typec_handle_error_recovery_timeout(
						struct tcpc_device *tcpc_dev)
{
	/* TODO: Check it later */
	tcpc_dev->typec_attach_new = TYPEC_UNATTACHED;

	mutex_lock(&tcpc_dev->access_lock);
	tcpc_dev->pd_wait_error_recovery = false;
	mutex_unlock(&tcpc_dev->access_lock);

	typec_unattach_wait_pe_idle_entry(tcpc_dev);
	typec_alert_attach_state_change(tcpc_dev);

	return 0;
}

static inline int typec_handle_pe_idle(struct tcpc_device *tcpc_dev)
{
	switch (tcpc_dev->typec_state) {

#ifdef CONFIG_TYPEC_CAP_TRY_SOURCE
	case typec_trywait_snk_pe:
		typec_trywait_snk_entry(tcpc_dev);
		break;
#endif

	case typec_unattachwait_pe:
		typec_unattached_entry(tcpc_dev);
		break;

	default:
		TYPEC_DBG("Dummy pe_idle\r\n");
		break;
	}

	return 0;
}
#endif /* CONFIG_USB_POWER_DELIVERY */

static inline int typec_handle_src_reach_vsafe0v(struct tcpc_device *tcpc_dev)
{
	if (typec_is_drp_toggling()) {
		TYPEC_DBG("[Waring] DRP Toggling\r\n");
		return 0;
	}

	typec_cc_src_detect_vsafe0v_entry(tcpc_dev);
	return 0;
}

static inline int typec_handle_src_toggle_timeout(struct tcpc_device *tcpc_dev)
{
	if (tcpc_dev->typec_state == typec_unattached_src) {
		TYPEC_NEW_STATE(typec_unattached_snk);
		tcpci_set_cc(tcpc_dev, TYPEC_CC_DRP);
		typec_enable_low_power_mode(tcpc_dev, TYPEC_CC_DRP);
	}

	return 0;
}

int tcpc_typec_handle_timeout(struct tcpc_device *tcpc_dev, uint32_t timer_id)
{
	int ret = 0;

#ifdef CONFIG_TYPEC_CAP_TRY_STATE
	if (timer_id == TYPEC_TRY_TIMER_DRP_TRY)
		return typec_handle_drp_try_timeout(tcpc_dev);
#endif	/* CONFIG_TYPEC_CAP_TRY_STATE */

	if (timer_id >= TYPEC_TIMER_START_ID)
		tcpc_reset_typec_debounce_timer(tcpc_dev);
	else if (timer_id >= TYPEC_RT_TIMER_START_ID)
		tcpc_disable_timer(tcpc_dev, timer_id);

#ifdef CONFIG_USB_POWER_DELIVERY
	if (tcpc_dev->pd_wait_pr_swap_complete) {
		TYPEC_DBG("[PR.Swap] Igrone timer_evt\r\n");
		return 0;
	}

	if (tcpc_dev->pd_wait_error_recovery &&
		(timer_id != TYPEC_TIMER_ERROR_RECOVERY)) {
		TYPEC_DBG("[Recovery] Igrone timer_evt\r\n");
		return 0;
	}
#endif

	switch (timer_id) {
	case TYPEC_TIMER_CCDEBOUNCE:
	case TYPEC_TIMER_PDDEBOUNCE:
		ret = typec_handle_debounce_timeout(tcpc_dev);
		break;

#ifdef CONFIG_USB_POWER_DELIVERY
	case TYPEC_TIMER_ERROR_RECOVERY:
		ret = typec_handle_error_recovery_timeout(tcpc_dev);
		break;

	case TYPEC_RT_TIMER_PE_IDLE:
		ret = typec_handle_pe_idle(tcpc_dev);
		break;
#endif /* CONFIG_USB_POWER_DELIVERY */

	case TYPEC_RT_TIMER_SAFE0V_DELAY:
		ret = typec_handle_src_reach_vsafe0v(tcpc_dev);
		break;

	case TYPEC_TIMER_WAKEUP:
		if (tcpc_dev->typec_lpm || tcpc_dev->typec_cable_only) {
			tcpc_dev->typec_lpm = true;
			ret = tcpci_set_low_power_mode(tcpc_dev, true,
				(tcpc_dev->typec_role == TYPEC_ROLE_SRC) ?
				TYPEC_CC_RP : TYPEC_CC_DRP);
		}
		break;

#ifdef CONFIG_TYPEC_ATTACHED_SRC_VSAFE0V_TIMEOUT
	case TYPEC_RT_TIMER_SAFE0V_TOUT:
		ret = tcpc_typec_handle_vsafe0v(tcpc_dev);
		break;
#endif	/* CONFIG_TYPEC_ATTACHED_SRC_VSAFE0V_TIMEOUT */

	case TYPEC_TIMER_DRP_SRC_TOGGLE:
		ret = typec_handle_src_toggle_timeout(tcpc_dev);
		break;

	}

	return ret;
}

/*
 * [BLOCK] Handle ps-change event
 */

static inline int typec_handle_vbus_present(struct tcpc_device *tcpc_dev)
{
	switch (tcpc_dev->typec_wait_ps_change) {
	case TYPEC_WAIT_PS_SNK_VSAFE5V:
		typec_cc_snk_detect_vsafe5v_entry(tcpc_dev);
		typec_alert_attach_state_change(tcpc_dev);
		break;
	case TYPEC_WAIT_PS_SRC_VSAFE5V:
		typec_source_attached_with_vbus_entry(tcpc_dev);
		typec_alert_attach_state_change(tcpc_dev);
		break;
	}

	return 0;
}

static inline int typec_attached_snk_vbus_absent(struct tcpc_device *tcpc_dev)
{
#ifdef TYPEC_EXIT_ATTACHED_SNK_VIA_VBUS
#ifdef CONFIG_USB_POWER_DELIVERY
	if (tcpc_dev->pd_wait_hard_reset_complete ||
				tcpc_dev->pd_hard_reset_event_pending) {
		if (typec_get_cc_res() != TYPEC_CC_VOLT_OPEN) {
			TYPEC_DBG
			    ("Ignore vbus_absent(snk), HReset & CC!=0\r\n");
			return 0;
		}
	}
#endif /* CONFIG_USB_POWER_DELIVERY */

#ifdef CONFIG_RT7207_ADAPTER
	if (tcpc_dev->rt7207_direct_charge_flag) {
		if (typec_get_cc_res() != TYPEC_CC_VOLT_OPEN &&
				!tcpci_check_vsafe0v(tcpc_dev, true)) {
			TYPEC_DBG("Ignore vbus_absent(snk), Dircet Charging\n");
			return 0;
		}
	}
#endif /* CONFIG_RT7207_ADAPTER */

	typec_unattach_wait_pe_idle_entry(tcpc_dev);
	typec_alert_attach_state_change(tcpc_dev);
#endif /* TYPEC_EXIT_ATTACHED_SNK_VIA_VBUS */

	return 0;
}


static inline int typec_handle_vbus_absent(struct tcpc_device *tcpc_dev)
{
#ifdef CONFIG_USB_POWER_DELIVERY
	if (tcpc_dev->pd_wait_pr_swap_complete) {
		TYPEC_DBG("[PR.Swap] Igrone vbus_absent\r\n");
		return 0;
	}

	if (tcpc_dev->pd_wait_error_recovery) {
		TYPEC_DBG("[Recovery] Igrone vbus_absent\r\n");
		return 0;
	}
#endif

	if (tcpc_dev->typec_state == typec_attached_snk)
		typec_attached_snk_vbus_absent(tcpc_dev);

#ifndef CONFIG_TCPC_VSAFE0V_DETECT
	tcpc_typec_handle_vsafe0v(tcpc_dev);
#endif /* #ifdef CONFIG_TCPC_VSAFE0V_DETECT */

	return 0;
}

int tcpc_typec_handle_ps_change(struct tcpc_device *tcpc_dev, int vbus_level)
{
#ifdef CONFIG_TYPEC_CHECK_LEGACY_CABLE
	if (tcpc_dev->typec_legacy_cable) {
		if (vbus_level) {
			TYPEC_INFO("LC->Attached\r\n");
			tcpc_dev->typec_legacy_cable = false;
			tcpci_set_cc(tcpc_dev, TYPEC_CC_RD);
		} else {
			TYPEC_INFO("LC->Detached (PS)\r\n");
			tcpc_dev->typec_legacy_cable = false;
			tcpci_set_cc(tcpc_dev, TYPEC_CC_DRP);
			typec_enable_low_power_mode(tcpc_dev, TYPEC_CC_DRP);
		}
		return 0;
	}
#endif /* CONFIG_TYPEC_CHECK_LEGACY_CABLE */

	if (typec_is_drp_toggling()) {
		TYPEC_DBG("[Waring] DRP Toggling\r\n");
		return 0;
	}

	if (vbus_level)
		return typec_handle_vbus_present(tcpc_dev);
	else
		return typec_handle_vbus_absent(tcpc_dev);
}

/*
 * [BLOCK] Handle PE event
 */

#ifdef CONFIG_USB_POWER_DELIVERY

int tcpc_typec_advertise_explicit_contract(struct tcpc_device *tcpc_dev)
{
	if (tcpc_dev->typec_local_rp_level == TYPEC_CC_RP_DFT)
		tcpci_set_cc(tcpc_dev, TYPEC_CC_RP_1_5);

	return 0;
}

int tcpc_typec_handle_pe_pr_swap(struct tcpc_device *tcpc_dev)
{
	int ret = 0;

	mutex_lock(&tcpc_dev->typec_lock);
	switch (tcpc_dev->typec_state) {
	case typec_attached_snk:
		TYPEC_NEW_STATE(typec_attached_src);
		tcpc_dev->typec_attach_old = TYPEC_ATTACHED_SRC;
		tcpci_set_cc(tcpc_dev, tcpc_dev->typec_local_rp_level);
		break;
	case typec_attached_src:
		TYPEC_NEW_STATE(typec_attached_snk);
		tcpc_dev->typec_attach_old = TYPEC_ATTACHED_SNK;
		tcpci_set_cc(tcpc_dev, TYPEC_CC_RD);
		break;
	default:
		break;
	}
	mutex_unlock(&tcpc_dev->typec_lock);
	return ret;
}

#endif /* CONFIG_USB_POWER_DELIVERY */

/*
 * [BLOCK] Handle reach vSafe0V event
 */

int tcpc_typec_handle_vsafe0v(struct tcpc_device *tcpc_dev)
{
	if (tcpc_dev->typec_wait_ps_change == TYPEC_WAIT_PS_SRC_VSAFE0V) {
#ifdef CONFIG_TYPEC_ATTACHED_SRC_SAFE0V_DELAY
		tcpc_enable_timer(tcpc_dev, TYPEC_RT_TIMER_SAFE0V_DELAY);
#else
		typec_handle_src_reach_vsafe0v(tcpc_dev);
#endif
	}

	return 0;
}

/*
 * [BLOCK] TCPCI TypeC I/F
 */

static const char *const typec_role_name[] = {
	"UNKNOWN",
	"SNK",
	"SRC",
	"DRP",
	"TrySRC",
	"TrySNK",
};

#ifndef CONFIG_USB_POWER_DELIVERY
int tcpc_typec_swap_role(struct tcpc_device *tcpc_dev)
{
	if (tcpc_dev->typec_role < TYPEC_ROLE_DRP)
		return -1;
	TYPEC_INFO("tcpc_typec_swap_role\r\n");

	switch (tcpc_dev->typec_state) {
	case typec_attached_src:
#ifdef CONFIG_TYPEC_CAP_TRY_SOURCE
		typec_trywait_snk_pe_entry(tcpc_dev);
#else
		TYPEC_INFO("SRC->SNK (X)\r\n");
#endif /* CONFIG_TYPEC_CAP_TRY_SOURCR */
		break;
	case typec_attached_snk:
#ifdef CONFIG_TYPEC_CAP_TRY_SINK
		typec_trywait_src_pe_entry(tcpc_dev);
#else
		TYPEC_INFO("SNK->SRC (X)\r\n");
#endif /* CONFIG_TYPEC_CAP_TRY_SINK */
		break;
	}
	return typec_alert_attach_state_change(tcpc_dev);
}
#endif /* ifndef CONFIG_USB_POWER_DELIVERY */

int tcpc_typec_set_rp_level(struct tcpc_device *tcpc_dev, uint8_t res)
{
	switch (res) {
	case TYPEC_CC_RP_DFT:
	case TYPEC_CC_RP_1_5:
	case TYPEC_CC_RP_3_0:
		TYPEC_INFO("TypeC-Rp: %d\r\n", res);
		tcpc_dev->typec_local_rp_level = res;
		break;

	default:
		TYPEC_INFO("TypeC-Unknown-Rp (%d)\r\n", res);
		return -1;
	}

#ifdef CONFIG_USB_PD_DBG_ALWAYS_LOCAL_RP
	tcpci_set_cc(tcpc_dev, tcpc_dev->typec_local_rp_level);
#else
	if ((tcpc_dev->typec_attach_old != TYPEC_UNATTACHED) &&
		(tcpc_dev->typec_attach_new != TYPEC_UNATTACHED)) {
		return tcpci_set_cc(tcpc_dev, res);
	}
#endif

	return 0;
}

int tcpc_typec_change_role(
	struct tcpc_device *tcpc_dev, uint8_t typec_role)
{
	uint8_t local_cc;
	bool force_unattach = false;

	if (typec_role == TYPEC_ROLE_UNKNOWN ||
		typec_role >= TYPEC_ROLE_NR) {
		TYPEC_INFO("Wrong TypeC-Role: %d\r\n", typec_role);
		return -1;
	}

	mutex_lock(&tcpc_dev->access_lock);

	tcpc_dev->typec_role = typec_role;
	TYPEC_INFO("typec_new_role: %s\r\n", typec_role_name[typec_role]);

	local_cc = tcpc_dev->typec_local_cc & 0x07;

	if (typec_role == TYPEC_ROLE_SNK && local_cc == TYPEC_CC_RP)
		force_unattach = true;

	if (typec_role == TYPEC_ROLE_SRC && local_cc == TYPEC_CC_RD)
		force_unattach = true;

	if (tcpc_dev->typec_attach_new == TYPEC_UNATTACHED)
		force_unattach = true;

	if (force_unattach) {
		TYPEC_DBG("force_unattach\r\n");
		typec_disable_low_power_mode(tcpc_dev);
		typec_unattached_entry(tcpc_dev);
	}

	mutex_unlock(&tcpc_dev->access_lock);
	return 0;
}

#ifdef CONFIG_TYPEC_CAP_POWER_OFF_CHARGE
static int typec_init_power_off_charge(struct tcpc_device *tcpc_dev)
{
	int ret = tcpci_get_cc(tcpc_dev);

	if (ret < 0)
		return ret;

	if (tcpc_dev->typec_role == TYPEC_ROLE_SRC)
		return 0;

	if (typec_is_cc_open())
		return 0;

	if (!tcpci_check_vbus_valid(tcpc_dev))
		return 0;

	TYPEC_DBG("PowerOffCharge\r\n");

	TYPEC_NEW_STATE(typec_unattached_snk);
	typec_wait_ps_change(tcpc_dev, TYPEC_WAIT_PS_DISABLE);

	tcpci_set_cc(tcpc_dev, TYPEC_CC_OPEN);
	tcpci_set_cc(tcpc_dev, TYPEC_CC_RD);

	return 1;
}
#endif	/* CONFIG_TYPEC_CAP_POWER_OFF_CHARGE */

int tcpc_typec_init(struct tcpc_device *tcpc_dev, uint8_t typec_role)
{
	int ret;

	if (typec_role >= TYPEC_ROLE_NR) {
		TYPEC_INFO("Wrong TypeC-Role: %d\r\n", typec_role);
		return -2;
	}

	TYPEC_INFO("typec_init: %s\r\n", typec_role_name[typec_role]);

	tcpc_dev->typec_role = typec_role;
	tcpc_dev->typec_attach_new = TYPEC_UNATTACHED;
	tcpc_dev->typec_attach_old = TYPEC_UNATTACHED;

	tcpc_dev->typec_remote_cc[0] = TYPEC_CC_VOLT_OPEN;
	tcpc_dev->typec_remote_cc[1] = TYPEC_CC_VOLT_OPEN;

#ifdef CONFIG_TYPEC_CHECK_LEGACY_CABLE
	tcpc_dev->typec_legacy_cable = false;
	tcpc_dev->typec_legacy_cable_suspect = 0;
#endif	/* CONFIG_TYPEC_CHECK_LEGACY_CABLE */

#ifdef CONFIG_TYPEC_CAP_POWER_OFF_CHARGE
	ret = typec_init_power_off_charge(tcpc_dev);
	if (ret != 0)
		return ret;
#endif	/* CONFIG_TYPEC_CAP_POWER_OFF_CHARGE */

#ifdef CONFIG_TYPEC_POWER_CTRL_INIT
	tcpc_dev->typec_power_ctrl = true;
#endif	/* CONFIG_TYPEC_POWER_CTRL_INIT */

	if (typec_role >= TYPEC_ROLE_DRP) {
		tcpci_get_cc(tcpc_dev);
		if (tcpc_dev->typec_remote_cc[0] == TYPEC_CC_VOLT_OPEN &&
			tcpc_dev->typec_remote_cc[1] == TYPEC_CC_VOLT_OPEN) {
			tcpci_set_cc(tcpc_dev, TYPEC_CC_OPEN);
			mdelay(50);
		}
	}
	typec_unattached_entry(tcpc_dev);
	return 0;
}

void  tcpc_typec_deinit(struct tcpc_device *tcpc_dev)
{
}
