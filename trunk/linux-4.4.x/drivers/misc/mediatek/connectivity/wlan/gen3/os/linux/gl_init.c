/*
* Copyright (C) 2016 MediaTek Inc.
*
* This program is free software: you can redistribute it and/or modify it under the terms of the
* GNU General Public License version 2 as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along with this program.
* If not, see <http://www.gnu.org/licenses/>.
*/

/*
** Id: //Department/DaVinci/BRANCHES/MT6620_WIFI_DRIVER_V2_3/os/linux/gl_init.c#11
*/

/*
 * ! \file   gl_init.c
 * \brief  Main routines of Linux driver
 *
 *  This file contains the main routines of Linux driver for MediaTek Inc. 802.11
 *  Wireless LAN Adapters.
 */

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/
#include "gl_os.h"
#include "wlan_lib.h"
#include "gl_wext.h"
#include "gl_cfg80211.h"
#include "precomp.h"
#if CFG_SUPPORT_AGPS_ASSIST
#include "gl_kal.h"
#endif
#include "gl_vendor.h"

#ifdef FW_CFG_SUPPORT
#include "fwcfg.h"
#endif
/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/
/* #define MAX_IOREQ_NUM   10 */
static struct wireless_dev *gprWdev;
/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/
/*
 * Tasklet mechanism is like buttom-half in Linux. We just want to
 * send a signal to OS for interrupt defer processing. All resources
 * are NOT allowed reentry, so txPacket, ISR-DPC and ioctl must avoid preempty.
 */
typedef struct _WLANDEV_INFO_T {
	struct net_device *prDev;
} WLANDEV_INFO_T, *P_WLANDEV_INFO_T;

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

#define CHAN2G(_channel, _freq, _flags)         \
{                                           \
	.band               = IEEE80211_BAND_2GHZ,  \
	.center_freq        = (_freq),              \
	.hw_value           = (_channel),           \
	.flags              = (_flags),             \
	.max_antenna_gain   = 0,                    \
	.max_power          = 30,                   \
}

static struct ieee80211_channel mtk_2ghz_channels[] = {
	CHAN2G(1, 2412, 0),
	CHAN2G(2, 2417, 0),
	CHAN2G(3, 2422, 0),
	CHAN2G(4, 2427, 0),
	CHAN2G(5, 2432, 0),
	CHAN2G(6, 2437, 0),
	CHAN2G(7, 2442, 0),
	CHAN2G(8, 2447, 0),
	CHAN2G(9, 2452, 0),
	CHAN2G(10, 2457, 0),
	CHAN2G(11, 2462, 0),
	CHAN2G(12, 2467, 0),
	CHAN2G(13, 2472, 0),
	CHAN2G(14, 2484, 0),
};

#define CHAN5G(_channel, _flags)                    \
{                                               \
	.band               = IEEE80211_BAND_5GHZ,      \
	.center_freq        = 5000 + (5 * (_channel)),  \
	.hw_value           = (_channel),               \
	.flags              = (_flags),                 \
	.max_antenna_gain   = 0,                        \
	.max_power          = 30,                       \
}

static struct ieee80211_channel mtk_5ghz_channels[] = {
	CHAN5G(34, 0), CHAN5G(36, 0),
	CHAN5G(38, 0), CHAN5G(40, 0),
	CHAN5G(42, 0), CHAN5G(44, 0),
	CHAN5G(46, 0), CHAN5G(48, 0),
	CHAN5G(52, 0), CHAN5G(56, 0),
	CHAN5G(60, 0), CHAN5G(64, 0),
	CHAN5G(100, 0), CHAN5G(104, 0),
	CHAN5G(108, 0), CHAN5G(112, 0),
	CHAN5G(116, 0), CHAN5G(120, 0),
	CHAN5G(124, 0), CHAN5G(128, 0),
	CHAN5G(132, 0), CHAN5G(136, 0),
	CHAN5G(140, 0), CHAN5G(149, 0),
	CHAN5G(153, 0), CHAN5G(157, 0),
	CHAN5G(161, 0), CHAN5G(165, 0),
	CHAN5G(169, 0), CHAN5G(173, 0),
	CHAN5G(184, 0), CHAN5G(188, 0),
	CHAN5G(192, 0), CHAN5G(196, 0),
	CHAN5G(200, 0), CHAN5G(204, 0),
	CHAN5G(208, 0), CHAN5G(212, 0),
	CHAN5G(216, 0),
};

#define RATETAB_ENT(_rate, _rateid, _flags) \
{                                       \
	.bitrate    = (_rate),              \
	.hw_value   = (_rateid),            \
	.flags      = (_flags),             \
}

/* for cfg80211 - rate table */
static struct ieee80211_rate mtk_rates[] = {
	RATETAB_ENT(10, 0x1000, 0),
	RATETAB_ENT(20, 0x1001, 0),
	RATETAB_ENT(55, 0x1002, 0),
	RATETAB_ENT(110, 0x1003, 0),	/* 802.11b */
	RATETAB_ENT(60, 0x2000, 0),
	RATETAB_ENT(90, 0x2001, 0),
	RATETAB_ENT(120, 0x2002, 0),
	RATETAB_ENT(180, 0x2003, 0),
	RATETAB_ENT(240, 0x2004, 0),
	RATETAB_ENT(360, 0x2005, 0),
	RATETAB_ENT(480, 0x2006, 0),
	RATETAB_ENT(540, 0x2007, 0),	/* 802.11a/g */
};

#define mtk_a_rates         (mtk_rates + 4)
#define mtk_a_rates_size    (ARRAY_SIZE(mtk_rates) - 4)
#define mtk_g_rates         (mtk_rates + 0)
#define mtk_g_rates_size    (ARRAY_SIZE(mtk_rates) - 0)

#define WLAN_MCS_INFO                                 \
{                                                       \
	.rx_mask        = {0xff, 0, 0, 0, 0, 0, 0, 0, 0, 0},\
	.rx_highest     = 0,                                \
	.tx_params      = IEEE80211_HT_MCS_TX_DEFINED,      \
}

#define WLAN_HT_CAP                                   \
{                                                       \
	.ht_supported   = true,                             \
	.cap            = IEEE80211_HT_CAP_SUP_WIDTH_20_40  \
			| IEEE80211_HT_CAP_SM_PS            \
			| IEEE80211_HT_CAP_GRN_FLD          \
			| IEEE80211_HT_CAP_SGI_20           \
			| IEEE80211_HT_CAP_SGI_40,          \
	.ampdu_factor   = IEEE80211_HT_MAX_AMPDU_64K,       \
	.ampdu_density  = IEEE80211_HT_MPDU_DENSITY_NONE,   \
	.mcs            = WLAN_MCS_INFO,                  \
}

/**********************************************************
* Public for both legacy Wi-Fi and P2P to access
**********************************************************/
struct ieee80211_supported_band mtk_band_2ghz = {
	.band = IEEE80211_BAND_2GHZ,
	.channels = mtk_2ghz_channels,
	.n_channels = ARRAY_SIZE(mtk_2ghz_channels),
	.bitrates = mtk_g_rates,
	.n_bitrates = mtk_g_rates_size,
	.ht_cap = WLAN_HT_CAP,
};

/* public for both Legacy Wi-Fi / P2P access */
struct ieee80211_supported_band mtk_band_5ghz = {
	.band = IEEE80211_BAND_5GHZ,
	.channels = mtk_5ghz_channels,
	.n_channels = ARRAY_SIZE(mtk_5ghz_channels),
	.bitrates = mtk_a_rates,
	.n_bitrates = mtk_a_rates_size,
	.ht_cap = WLAN_HT_CAP,
};

const UINT_32 mtk_cipher_suites[5] = {
	/* keep WEP first, it may be removed below */
	WLAN_CIPHER_SUITE_WEP40,
	WLAN_CIPHER_SUITE_WEP104,
	WLAN_CIPHER_SUITE_TKIP,
	WLAN_CIPHER_SUITE_CCMP,

	/* keep last -- depends on hw flags! */
	WLAN_CIPHER_SUITE_AES_CMAC
};

/*********************************************************/

/* NIC interface name */
#define NIC_INF_NAME    "wlan%d"

#if CFG_SUPPORT_SNIFFER
#define NIC_MONITOR_INF_NAME	"radiotap%d"
#endif

UINT_8 aucDebugModule[DBG_MODULE_NUM];

/* 4 2007/06/26, mikewu, now we don't use this, we just fix the number of wlan device to 1 */
static WLANDEV_INFO_T arWlanDevInfo[CFG_MAX_WLAN_DEVICES] = { {0} };

static UINT_32 u4WlanDevNum;	/* How many NICs coexist now */

/**20150205 added work queue for sched_scan to avoid cfg80211 stop schedule scan dead loack**/
struct delayed_work sched_workq;

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

static struct cfg80211_ops mtk_wlan_ops = {
	.suspend = mtk_cfg80211_suspend,
	.resume	= mtk_cfg80211_resume,
	.change_virtual_intf = mtk_cfg80211_change_iface,
	.add_key = mtk_cfg80211_add_key,
	.get_key = mtk_cfg80211_get_key,
	.del_key = mtk_cfg80211_del_key,
	.set_default_key = mtk_cfg80211_set_default_key,
	.get_station = mtk_cfg80211_get_station,
#if CFG_SUPPORT_TDLS
	.change_station = mtk_cfg80211_change_station,
	.add_station = mtk_cfg80211_add_station,
	.del_station = mtk_cfg80211_del_station,
#endif
	.scan = mtk_cfg80211_scan,
	.connect = mtk_cfg80211_connect,
	.disconnect = mtk_cfg80211_disconnect,
	.join_ibss = mtk_cfg80211_join_ibss,
	.leave_ibss = mtk_cfg80211_leave_ibss,
	.set_power_mgmt = mtk_cfg80211_set_power_mgmt,
	.set_pmksa = mtk_cfg80211_set_pmksa,
	.del_pmksa = mtk_cfg80211_del_pmksa,
	.flush_pmksa = mtk_cfg80211_flush_pmksa,
#ifdef CONFIG_SUPPORT_GTK_REKEY
	.set_rekey_data = mtk_cfg80211_set_rekey_data,
#endif
	.assoc = mtk_cfg80211_assoc,
	/* Action Frame TX/RX */
	.remain_on_channel = mtk_cfg80211_remain_on_channel,
	.cancel_remain_on_channel = mtk_cfg80211_cancel_remain_on_channel,
	.mgmt_tx = mtk_cfg80211_mgmt_tx,
	/* .mgmt_tx_cancel_wait        = mtk_cfg80211_mgmt_tx_cancel_wait, */
	.mgmt_frame_register = mtk_cfg80211_mgmt_frame_register,
#ifdef CONFIG_NL80211_TESTMODE
	.testmode_cmd = mtk_cfg80211_testmode_cmd,
#endif
	.sched_scan_start = mtk_cfg80211_sched_scan_start,
	.sched_scan_stop = mtk_cfg80211_sched_scan_stop,
#if CFG_SUPPORT_TDLS
	.tdls_oper = mtk_cfg80211_tdls_oper,
	.tdls_mgmt = mtk_cfg80211_tdls_mgmt,
#endif
};

static const struct wiphy_vendor_command mtk_wlan_vendor_ops[] = {
	{
		{
			.vendor_id = GOOGLE_OUI,
			.subcmd = WIFI_SUBCMD_GET_CHANNEL_LIST
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = mtk_cfg80211_vendor_get_channel_list
	},
	{
		{
			.vendor_id = GOOGLE_OUI,
			.subcmd = WIFI_SUBCMD_SET_COUNTRY_CODE
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = mtk_cfg80211_vendor_set_country_code
	},
	/* GSCAN */
#if CFG_SUPPORT_GSCN
	{
		{
			.vendor_id = GOOGLE_OUI,
			.subcmd = GSCAN_SUBCMD_GET_CAPABILITIES
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = mtk_cfg80211_vendor_get_gscan_capabilities
	},
	{
		{
			.vendor_id = GOOGLE_OUI,
			.subcmd = GSCAN_SUBCMD_SET_CONFIG
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = mtk_cfg80211_vendor_set_config
	},
	{
		{
			.vendor_id = GOOGLE_OUI,
			.subcmd = GSCAN_SUBCMD_SET_SCAN_CONFIG
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = mtk_cfg80211_vendor_set_scan_config
	},
	{
		{
			.vendor_id = GOOGLE_OUI,
			.subcmd = GSCAN_SUBCMD_ENABLE_GSCAN
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = mtk_cfg80211_vendor_enable_scan
	},
	{
		{
			.vendor_id = GOOGLE_OUI,
			.subcmd = GSCAN_SUBCMD_ENABLE_FULL_SCAN_RESULTS
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = mtk_cfg80211_vendor_enable_full_scan_results
	},
	{
		{
			.vendor_id = GOOGLE_OUI,
			.subcmd = GSCAN_SUBCMD_GET_SCAN_RESULTS
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = mtk_cfg80211_vendor_get_gscan_result
	},
#endif
	{
		{
			.vendor_id = GOOGLE_OUI,
			.subcmd = GSCAN_SUBCMD_SET_SIGNIFICANT_CHANGE_CONFIG
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = mtk_cfg80211_vendor_set_significant_change
	},
	{
		{
			.vendor_id = GOOGLE_OUI,
			.subcmd = GSCAN_SUBCMD_SET_HOTLIST
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = mtk_cfg80211_vendor_set_hotlist
	},
	/* RTT */
	{
		{
			.vendor_id = GOOGLE_OUI,
			.subcmd = RTT_SUBCMD_GETCAPABILITY
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = mtk_cfg80211_vendor_get_rtt_capabilities
	},
	/* Link Layer Statistics */
	{
		{
			.vendor_id = GOOGLE_OUI,
			.subcmd = LSTATS_SUBCMD_GET_INFO
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = mtk_cfg80211_vendor_llstats_get_info
	},
	/* RSSI Monitoring */
	{
		{
			.vendor_id = GOOGLE_OUI,
			.subcmd = WIFI_SUBCMD_SET_RSSI_MONITOR
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = mtk_cfg80211_vendor_set_rssi_monitoring
	},
	/* Packet Keep Alive */
	{
		{
			.vendor_id = GOOGLE_OUI,
			.subcmd = WIFI_OFFLOAD_START_MKEEP_ALIVE
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = mtk_cfg80211_vendor_packet_keep_alive_start
	},
	{
		{
			.vendor_id = GOOGLE_OUI,
			.subcmd = WIFI_OFFLOAD_STOP_MKEEP_ALIVE
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = mtk_cfg80211_vendor_packet_keep_alive_stop
	},
	{
		{
			.vendor_id = OUI_QCA,
			.subcmd = WIFI_SUBCMD_SET_ROAMING
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = mtk_cfg80211_vendor_set_roaming_policy
	},
};

static const struct nl80211_vendor_cmd_info mtk_wlan_vendor_events[] = {
	{
		.vendor_id = GOOGLE_OUI,
		.subcmd = GSCAN_EVENT_SIGNIFICANT_CHANGE_RESULTS
	},
	{
		.vendor_id = GOOGLE_OUI,
		.subcmd = GSCAN_EVENT_HOTLIST_RESULTS_FOUND
	},
	{
		.vendor_id = GOOGLE_OUI,
		.subcmd = GSCAN_EVENT_SCAN_RESULTS_AVAILABLE
	},
	{
		.vendor_id = GOOGLE_OUI,
		.subcmd = GSCAN_EVENT_FULL_SCAN_RESULTS
	},
	{
		.vendor_id = GOOGLE_OUI,
		.subcmd = RTT_EVENT_COMPLETE
	},
	{
		.vendor_id = GOOGLE_OUI,
		.subcmd = GSCAN_EVENT_COMPLETE_SCAN
	},
	{
		.vendor_id = GOOGLE_OUI,
		.subcmd = GSCAN_EVENT_HOTLIST_RESULTS_LOST
	},
	{
		.vendor_id = GOOGLE_OUI,
		.subcmd = WIFI_EVENT_RSSI_MONITOR
	},
};

/* There isn't a lot of sense in it, but you can transmit anything you like */
static const struct ieee80211_txrx_stypes
	mtk_cfg80211_ais_default_mgmt_stypes[NUM_NL80211_IFTYPES] = {
	[NL80211_IFTYPE_ADHOC] = {
		.tx = 0xffff,
		.rx = BIT(IEEE80211_STYPE_ACTION >> 4)
	},
	[NL80211_IFTYPE_STATION] = {
		.tx = 0xffff,
		.rx = BIT(IEEE80211_STYPE_ACTION >> 4) | BIT(IEEE80211_STYPE_PROBE_REQ >> 4)
	},
	[NL80211_IFTYPE_AP] = {
		.tx = 0xffff,
		.rx = BIT(IEEE80211_STYPE_PROBE_REQ >> 4) | BIT(IEEE80211_STYPE_ACTION >> 4)
	},
	[NL80211_IFTYPE_AP_VLAN] = {
		/* copy AP */
		.tx = 0xffff,
		.rx = BIT(IEEE80211_STYPE_ASSOC_REQ >> 4) |
		      BIT(IEEE80211_STYPE_REASSOC_REQ >> 4) |
		      BIT(IEEE80211_STYPE_PROBE_REQ >> 4) |
		      BIT(IEEE80211_STYPE_DISASSOC >> 4) |
		      BIT(IEEE80211_STYPE_AUTH >> 4) |
		      BIT(IEEE80211_STYPE_DEAUTH >> 4) | BIT(IEEE80211_STYPE_ACTION >> 4)
	},
	[NL80211_IFTYPE_P2P_CLIENT] = {
		.tx = 0xffff,
		.rx = BIT(IEEE80211_STYPE_ACTION >> 4) | BIT(IEEE80211_STYPE_PROBE_REQ >> 4)
	},
	[NL80211_IFTYPE_P2P_GO] = {
		.tx = 0xffff,
		.rx = BIT(IEEE80211_STYPE_PROBE_REQ >> 4) | BIT(IEEE80211_STYPE_ACTION >> 4)
	}
};

#ifdef CONFIG_PM
static const struct wiphy_wowlan_support mtk_wlan_wowlan_support = {
	.flags = WIPHY_WOWLAN_DISCONNECT | WIPHY_WOWLAN_ANY,
};
#endif

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/

/*******************************************************************************
*                   F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

#if 0
/*----------------------------------------------------------------------------*/
/*!
* \brief Override the implementation of select queue
*
* \param[in] dev Pointer to struct net_device
* \param[in] skb Pointer to struct skb_buff
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/
unsigned int _cfg80211_classify8021d(struct sk_buff *skb)
{
	unsigned int dscp = 0;

	/* skb->priority values from 256->263 are magic values
	 * directly indicate a specific 802.1d priority.  This is
	 * to allow 802.1d priority to be passed directly in from
	 * tags
	 */

	if (skb->priority >= 256 && skb->priority <= 263)
		return skb->priority - 256;
	switch (skb->protocol) {
	case htons(ETH_P_IP):
		dscp = ip_hdr(skb)->tos & 0xfc;
		break;
	}
	return dscp >> 5;
}
#endif

UINT_16 wlanSelectQueue(struct net_device *dev, struct sk_buff *skb,
			void *accel_priv, select_queue_fallback_t fallback)
{
	UINT_16 au16Wlan1dToQueueIdx[8] = { 1, 0, 0, 1, 2, 2, 3, 3 };

	/* Use Linux wireless utility function */
	skb->priority = cfg80211_classify8021d(skb, NULL);

	return au16Wlan1dToQueueIdx[skb->priority];
}

/*----------------------------------------------------------------------------*/
/*!
* \brief Load NVRAM data and translate it into REG_INFO_T
*
* \param[in]  prGlueInfo Pointer to struct GLUE_INFO_T
* \param[out] prRegInfo  Pointer to struct REG_INFO_T
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/
static void glLoadNvram(IN P_GLUE_INFO_T prGlueInfo, OUT P_REG_INFO_T prRegInfo)
{
	UINT_32 i;
	PUINT_8 pucDest;
	struct device_node *node;

	ASSERT(prGlueInfo);
	ASSERT(prRegInfo);

	if ((!prGlueInfo) || (!prRegInfo))
		return;


	node = of_find_compatible_node(NULL, NULL, "mediatek,connectivity-combo");

	if (node) {
		const UINT_8 *prop;
		prGlueInfo->fgNvramAvailable = TRUE;

		/* load MAC Address */
		prop = of_get_property(node, "aucMacAddress", NULL);
		if (prop != NULL) {
			COPY_MAC_ADDR(prRegInfo->aucMacAddr, prop);
			DBGLOG(INIT, INFO, "DTB MAC address: " MACSTR, prRegInfo->aucMacAddr);
		}

		/* load country code */
		prop = of_get_property(node, "aucCountryCode", NULL);
		if (prop != NULL) {
		/* cast to wide characters */
			memcpy(prRegInfo->au2CountryCode, prop, sizeof(UINT_8));
			memcpy(&prRegInfo->au2CountryCode[1], &prop[1], sizeof(UINT_8));
			DBGLOG(INIT, INFO, "DTB CountryCode:%02x:%02x\n",
					prRegInfo->au2CountryCode[0], prRegInfo->au2CountryCode[1]);
		}


		/* load default normal TX power */
		prop = of_get_property(node, "cTxPwr2G4Cck", NULL);
		if (prop != NULL) {
			prRegInfo->rTxPwr.cTxPwr2G4Cck = prop[0];
			DBGLOG(INIT, INFO, "DTB cTxPwr2G4Cck:%02x\n", prRegInfo->rTxPwr.cTxPwr2G4Cck);
		}

		prop = of_get_property(node, "cTxPwr2G4Dsss", NULL);
		if (prop != NULL) {
			prRegInfo->rTxPwr.cTxPwr2G4Dsss = prop[0];
			DBGLOG(INIT, INFO, "DTB cTxPwr2G4Dsss:%02x\n", prRegInfo->rTxPwr.cTxPwr2G4Dsss);
		}

		prop = of_get_property(node, "acReserved", NULL);
		if (prop != NULL) {
			memcpy(prRegInfo->rTxPwr.acReserved, prop, 2*sizeof(INT_8));
			DBGLOG(INIT, INFO, "DTB acReserved:%02x:%02x\n",
					prRegInfo->rTxPwr.acReserved[0], prRegInfo->rTxPwr.acReserved[1]);
		}

		prop = of_get_property(node, "cTxPwr2G4OFDM_BPSK", NULL);
		if (prop != NULL) {
			prRegInfo->rTxPwr.cTxPwr2G4OFDM_BPSK = prop[0];
			DBGLOG(INIT, INFO, "DTB cTxPwr2G4OFDM_BPSK:%02x\n", prRegInfo->rTxPwr.cTxPwr2G4OFDM_BPSK);
		}

		prop = of_get_property(node, "cTxPwr2G4OFDM_QPSK", NULL);
		if (prop != NULL) {
			prRegInfo->rTxPwr.cTxPwr2G4OFDM_QPSK = prop[0];
			DBGLOG(INIT, INFO, "DTB cTxPwr2G4OFDM_QPSK:%02x\n", prRegInfo->rTxPwr.cTxPwr2G4OFDM_QPSK);
		}

		prop = of_get_property(node, "cTxPwr2G4OFDM_16QAM", NULL);
		if (prop != NULL) {
			prRegInfo->rTxPwr.cTxPwr2G4OFDM_16QAM = prop[0];
			DBGLOG(INIT, INFO, "DTB cTxPwr2G4OFDM_16QAM:%02x\n", prRegInfo->rTxPwr.cTxPwr2G4OFDM_16QAM);
		}

		prop = of_get_property(node, "cTxPwr2G4OFDM_Reserved", NULL);
		if (prop != NULL) {
			prRegInfo->rTxPwr.cTxPwr2G4OFDM_Reserved = prop[0];
			DBGLOG(INIT, INFO, "DTB cTxPwr2G4OFDM_Reserved:%02x\n",
					prRegInfo->rTxPwr.cTxPwr2G4OFDM_Reserved);
		}

		prop = of_get_property(node, "cTxPwr2G4OFDM_48Mbps", NULL);
		if (prop != NULL) {
			prRegInfo->rTxPwr.cTxPwr2G4OFDM_48Mbps = prop[0];
			DBGLOG(INIT, INFO, "DTB cTxPwr2G4OFDM_48Mbps:%02x\n", prRegInfo->rTxPwr.cTxPwr2G4OFDM_48Mbps);
		}

		prop = of_get_property(node, "cTxPwr2G4OFDM_54Mbps", NULL);
		if (prop != NULL) {
			prRegInfo->rTxPwr.cTxPwr2G4OFDM_54Mbps = prop[0];
			DBGLOG(INIT, INFO, "DTB cTxPwr2G4OFDM_54Mbps:%02x\n", prRegInfo->rTxPwr.cTxPwr2G4OFDM_54Mbps);
		}

		prop = of_get_property(node, "cTxPwr2G4HT20_BPSK", NULL);
		if (prop != NULL) {
			prRegInfo->rTxPwr.cTxPwr2G4HT20_BPSK = prop[0];
			DBGLOG(INIT, INFO, "DTB cTxPwr2G4HT20_BPSK:%02x\n", prRegInfo->rTxPwr.cTxPwr2G4HT20_BPSK);
		}

		prop = of_get_property(node, "cTxPwr2G4HT20_QPSK", NULL);
		if (prop != NULL) {
			prRegInfo->rTxPwr.cTxPwr2G4HT20_QPSK = prop[0];
			DBGLOG(INIT, INFO, "DTB cTxPwr2G4HT20_QPSK:%02x\n", prRegInfo->rTxPwr.cTxPwr2G4HT20_QPSK);
		}

		prop = of_get_property(node, "cTxPwr2G4HT20_16QAM", NULL);
		if (prop != NULL) {
			prRegInfo->rTxPwr.cTxPwr2G4HT20_16QAM = prop[0];
			DBGLOG(INIT, INFO, "DTB cTxPwr2G4HT20_16QAM:%02x\n", prRegInfo->rTxPwr.cTxPwr2G4HT20_16QAM);
		}

		prop = of_get_property(node, "cTxPwr2G4HT20_MCS5", NULL);
		if (prop != NULL) {
			prRegInfo->rTxPwr.cTxPwr2G4HT20_MCS5 = prop[0];
			DBGLOG(INIT, INFO, "DTB cTxPwr2G4HT20_MCS5:%02x\n", prRegInfo->rTxPwr.cTxPwr2G4HT20_MCS5);
		}

		prop = of_get_property(node, "cTxPwr2G4HT20_MCS6", NULL);
		if (prop != NULL) {
			prRegInfo->rTxPwr.cTxPwr2G4HT20_MCS6 = prop[0];
			DBGLOG(INIT, INFO, "DTB cTxPwr2G4HT20_MCS6:%02x\n", prRegInfo->rTxPwr.cTxPwr2G4HT20_MCS6);
		}

		prop = of_get_property(node, "cTxPwr2G4HT20_MCS7", NULL);
		if (prop != NULL) {
			prRegInfo->rTxPwr.cTxPwr2G4HT20_MCS7 = prop[0];
			DBGLOG(INIT, INFO, "DTB cTxPwr2G4HT20_MCS7:%02x\n", prRegInfo->rTxPwr.cTxPwr2G4HT20_MCS7);
		}

		prop = of_get_property(node, "cTxPwr2G4HT40_BPSK", NULL);
		if (prop != NULL) {
			prRegInfo->rTxPwr.cTxPwr2G4HT40_BPSK = prop[0];
			DBGLOG(INIT, INFO, "DTB cTxPwr2G4HT40_BPSK:%02x\n", prRegInfo->rTxPwr.cTxPwr2G4HT40_BPSK);
		}


		prop = of_get_property(node, "cTxPwr2G4HT40_QPSK", NULL);
		if (prop != NULL) {
			prRegInfo->rTxPwr.cTxPwr2G4HT40_QPSK = prop[0];
			DBGLOG(INIT, INFO, "DTB cTxPwr2G4HT40_QPSK:%02x\n", prRegInfo->rTxPwr.cTxPwr2G4HT40_QPSK);
		}

		prop = of_get_property(node, "cTxPwr2G4HT40_16QAM", NULL);
		if (prop != NULL) {
			prRegInfo->rTxPwr.cTxPwr2G4HT40_16QAM = prop[0];
			DBGLOG(INIT, INFO, "DTB cTxPwr2G4HT40_16QAM:%02x\n", prRegInfo->rTxPwr.cTxPwr2G4HT40_16QAM);
		}

		prop = of_get_property(node, "cTxPwr2G4HT40_MCS5", NULL);
		if (prop != NULL) {
			prRegInfo->rTxPwr.cTxPwr2G4HT40_MCS5 = prop[0];
			DBGLOG(INIT, INFO, "DTB cTxPwr2G4HT40_MCS5:%02x\n", prRegInfo->rTxPwr.cTxPwr2G4HT40_MCS5);
		}

		prop = of_get_property(node, "cTxPwr2G4HT40_MCS6", NULL);
		if (prop != NULL) {
			prRegInfo->rTxPwr.cTxPwr2G4HT40_MCS6 = prop[0];
			DBGLOG(INIT, INFO, "DTB cTxPwr2G4HT40_MCS6:%02x\n", prRegInfo->rTxPwr.cTxPwr2G4HT40_MCS6);
		}

		prop = of_get_property(node, "cTxPwr2G4HT40_MCS7", NULL);
		if (prop != NULL) {
			prRegInfo->rTxPwr.cTxPwr2G4HT40_MCS7 = prop[0];
			DBGLOG(INIT, INFO, "DTB cTxPwr2G4HT40_MCS7:%02x\n", prRegInfo->rTxPwr.cTxPwr2G4HT40_MCS7);
		}

		prop = of_get_property(node, "cTxPwr5GOFDM_BPSK", NULL);
		if (prop != NULL) {
			prRegInfo->rTxPwr.cTxPwr5GOFDM_BPSK = prop[0];
			DBGLOG(INIT, INFO, "DTB cTxPwr5GOFDM_BPSK:%02x\n", prRegInfo->rTxPwr.cTxPwr5GOFDM_BPSK);
		}

		prop = of_get_property(node, "cTxPwr5GOFDM_QPSK", NULL);
		if (prop != NULL) {
			prRegInfo->rTxPwr.cTxPwr5GOFDM_QPSK = prop[0];
			DBGLOG(INIT, INFO, "DTB cTxPwr5GOFDM_QPSK:%02x\n", prRegInfo->rTxPwr.cTxPwr5GOFDM_QPSK);
		}

		prop = of_get_property(node, "cTxPwr5GOFDM_16QAM", NULL);
		if (prop != NULL) {
			prRegInfo->rTxPwr.cTxPwr5GOFDM_16QAM = prop[0];
			DBGLOG(INIT, INFO, "DTB cTxPwr5GOFDM_16QAM:%02x\n", prRegInfo->rTxPwr.cTxPwr5GOFDM_16QAM);
		}

		prop = of_get_property(node, "cTxPwr5GOFDM_Reserved", NULL);
		if (prop != NULL) {
			prRegInfo->rTxPwr.cTxPwr5GOFDM_Reserved = prop[0];
			DBGLOG(INIT, INFO, "DTB cTxPwr5GOFDM_Reserved:%02x\n", prRegInfo->rTxPwr.cTxPwr5GOFDM_Reserved);
		}

		prop = of_get_property(node, "cTxPwr5GOFDM_48Mbps", NULL);
		if (prop != NULL) {
			prRegInfo->rTxPwr.cTxPwr5GOFDM_48Mbps = prop[0];
			DBGLOG(INIT, INFO, "DTB cTxPwr5GOFDM_48Mbps:%02x\n", prRegInfo->rTxPwr.cTxPwr5GOFDM_48Mbps);
		}

		prop = of_get_property(node, "cTxPwr5GOFDM_54Mbps", NULL);
		if (prop != NULL) {
			prRegInfo->rTxPwr.cTxPwr5GOFDM_54Mbps = prop[0];
			DBGLOG(INIT, INFO, "DTB cTxPwr5GOFDM_54Mbps:%02x\n", prRegInfo->rTxPwr.cTxPwr5GOFDM_54Mbps);
		}

		prop = of_get_property(node, "cTxPwr5GHT20_BPSK", NULL);
		if (prop != NULL) {
			prRegInfo->rTxPwr.cTxPwr5GHT20_BPSK = prop[0];
			DBGLOG(INIT, INFO, "DTB cTxPwr5GHT20_BPSK:%02x\n", prRegInfo->rTxPwr.cTxPwr5GHT20_BPSK);
		}

		prop = of_get_property(node, "cTxPwr5GHT20_QPSK", NULL);
		if (prop != NULL) {
			prRegInfo->rTxPwr.cTxPwr5GHT20_QPSK = prop[0];
			DBGLOG(INIT, INFO, "DTB cTxPwr5GHT20_QPSK:%02x\n", prRegInfo->rTxPwr.cTxPwr5GHT20_QPSK);
		}

		prop = of_get_property(node, "cTxPwr5GHT20_16QAM", NULL);
		if (prop != NULL) {
			prRegInfo->rTxPwr.cTxPwr5GHT20_16QAM = prop[0];
			DBGLOG(INIT, INFO, "DTB cTxPwr5GHT20_16QAM:%02x\n", prRegInfo->rTxPwr.cTxPwr5GHT20_16QAM);
		}

		prop = of_get_property(node, "cTxPwr5GHT20_MCS5", NULL);
		if (prop != NULL) {
			prRegInfo->rTxPwr.cTxPwr5GHT20_MCS5 = prop[0];
			DBGLOG(INIT, INFO, "DTB cTxPwr5GHT20_MCS5:%02x\n", prRegInfo->rTxPwr.cTxPwr5GHT20_MCS5);
		}

		prop = of_get_property(node, "cTxPwr5GHT20_MCS6", NULL);
		if (prop != NULL) {
			prRegInfo->rTxPwr.cTxPwr5GHT20_MCS6 = prop[0];
			DBGLOG(INIT, INFO, "DTB cTxPwr5GHT20_MCS6:%02x\n", prRegInfo->rTxPwr.cTxPwr5GHT20_MCS6);
		}

		prop = of_get_property(node, "cTxPwr5GHT20_MCS7", NULL);
		if (prop != NULL) {
			prRegInfo->rTxPwr.cTxPwr5GHT20_MCS7 = prop[0];
			DBGLOG(INIT, INFO, "DTB cTxPwr5GHT20_MCS7:%02x\n", prRegInfo->rTxPwr.cTxPwr5GHT20_MCS7);
		}

		prop = of_get_property(node, "cTxPwr5GHT40_BPSK", NULL);
		if (prop != NULL) {
			prRegInfo->rTxPwr.cTxPwr5GHT40_BPSK = prop[0];
			DBGLOG(INIT, INFO, "DTB cTxPwr5GHT40_BPSK:%02x\n", prRegInfo->rTxPwr.cTxPwr5GHT40_BPSK);
		}

		prop = of_get_property(node, "cTxPwr5GHT40_QPSK", NULL);
		if (prop != NULL) {
			prRegInfo->rTxPwr.cTxPwr5GHT40_QPSK = prop[0];
			DBGLOG(INIT, INFO, "DTB cTxPwr5GHT40_QPSK:%02x\n", prRegInfo->rTxPwr.cTxPwr5GHT40_QPSK);
		}

		prop = of_get_property(node, "cTxPwr5GHT40_16QAM", NULL);
		if (prop != NULL) {
			prRegInfo->rTxPwr.cTxPwr5GHT40_16QAM = prop[0];
			DBGLOG(INIT, INFO, "DTB cTxPwr5GHT40_16QAM:%02x\n", prRegInfo->rTxPwr.cTxPwr5GHT40_16QAM);
		}

		prop = of_get_property(node, "cTxPwr5GHT40_MCS5", NULL);
		if (prop != NULL) {
			prRegInfo->rTxPwr.cTxPwr5GHT40_MCS5 = prop[0];
			DBGLOG(INIT, INFO, "DTB cTxPwr5GHT40_MCS5:%02x\n", prRegInfo->rTxPwr.cTxPwr5GHT40_MCS5);
		}

		prop = of_get_property(node, "cTxPwr5GHT40_MCS6", NULL);
		if (prop != NULL) {
			prRegInfo->rTxPwr.cTxPwr5GHT40_MCS6 = prop[0];
			DBGLOG(INIT, INFO, "DTB cTxPwr5GHT40_MCS6:%02x\n", prRegInfo->rTxPwr.cTxPwr5GHT40_MCS6);
		}

		prop = of_get_property(node, "cTxPwr5GHT40_MCS7", NULL);
		if (prop != NULL) {
			prRegInfo->rTxPwr.cTxPwr5GHT40_MCS7 = prop[0];
			DBGLOG(INIT, INFO, "DTB cTxPwr5GHT40_MCS7:%02x\n", prRegInfo->rTxPwr.cTxPwr5GHT40_MCS7);
		}

		/* load feature flags */
		prop = of_get_property(node, "ucTxPwrValid", NULL);
		if (prop != NULL) {
			prRegInfo->ucTxPwrValid = prop[0];
			DBGLOG(INIT, INFO, "DTB ucTxPwrValid:%02x\n", prRegInfo->ucTxPwrValid);
		}

		prop = of_get_property(node, "ucSupport5GBand", NULL);
		if (prop != NULL) {
			prRegInfo->ucSupport5GBand = prop[0];
			DBGLOG(INIT, INFO, "DTB ucSupport5GBand:%02x\n", prRegInfo->ucSupport5GBand);
		}

		prop = of_get_property(node, "uc2G4BwFixed20M", NULL);
		if (prop != NULL) {
			prRegInfo->uc2G4BwFixed20M = prop[0];
			DBGLOG(INIT, INFO, "DTB uc2G4BwFixed20M:%02x\n", prRegInfo->uc2G4BwFixed20M);
		}

		prop = of_get_property(node, "uc5GBwFixed20M", NULL);
		if (prop != NULL) {
			prRegInfo->uc5GBwFixed20M = prop[0];
			DBGLOG(INIT, INFO, "DTB uc5GBwFixed20M:%02x\n", prRegInfo->uc5GBwFixed20M);
		}

		prop = of_get_property(node, "ucEnable5GBand", NULL);
		if (prop != NULL) {
			prRegInfo->ucEnable5GBand = prop[0];
			DBGLOG(INIT, INFO, "DTB ucEnable5GBand:%02x\n", prRegInfo->ucEnable5GBand);
		}

		prop = of_get_property(node, "ucRxDiversity", NULL);
		if (prop != NULL) {
			prRegInfo->ucRxDiversity = prop[0];
			DBGLOG(INIT, INFO, "DTB ucRxDiversity:%02x\n", prRegInfo->ucRxDiversity);
		}

		prop = of_get_property(node, "ucRssiPathCompasationUsed", NULL);
		if (prop != NULL) {
			prRegInfo->ucRssiPathCompasationUsed = prop[0];
			DBGLOG(INIT, INFO, "DTB ucRssiPathCompasationUsed:%02x\n",
					prRegInfo->ucRssiPathCompasationUsed);
		}

		prop = of_get_property(node, "ucGpsDesense", NULL);
		if (prop != NULL) {
			prRegInfo->ucGpsDesense = prop[0];
			DBGLOG(INIT, INFO, "DTB ucGpsDesense:%02x\n", prRegInfo->ucGpsDesense);
		}

#if CFG_SUPPORT_NVRAM_5G
		/* load EFUSE overriding part */
		prop = of_get_property(node, "aucEFUSE144", NULL);
		if (prop != NULL) {
			memcpy(prRegInfo->aucEFUSE, prop, sizeof(prRegInfo->aucEFUSE));
			for (i = 0; i < sizeof(prRegInfo->aucEFUSE); i += sizeof(UINT_8))
				DBGLOG(INIT, INFO, "DTB aucEFUSE[%d]:%02x\n", i, prRegInfo->aucEFUSE[i]);
		}
		prRegInfo->prOldEfuseMapping = (P_NEW_EFUSE_MAPPING2NVRAM_T)&prRegInfo->aucEFUSE;
#else
		/* load EFUSE overriding part */
		prop = of_get_property(node, "aucEFUSE144", NULL);
		if (prop != NULL) {
			memcpy(prRegInfo->aucEFUSE, prop, sizeof(prRegInfo->aucEFUSE));
			for (i = 0; i < sizeof(prRegInfo->aucEFUSE); i += sizeof(UINT_8))
				DBGLOG(INIT, INFO, "DTB aucEFUSE[%d]:%02x\n", i, prRegInfo->aucEFUSE[i]);
		}
#endif

		/* load band edge tx power control */
		prop = of_get_property(node, "fg2G4BandEdgePwrUsed", NULL);
		if (prop != NULL) {
			prRegInfo->fg2G4BandEdgePwrUsed = (BOOLEAN)prop[0];
			DBGLOG(INIT, INFO, "DTB fg2G4BandEdgePwrUsed:%02x\n", prRegInfo->fg2G4BandEdgePwrUsed);
		}
		if (prRegInfo->fg2G4BandEdgePwrUsed) {
			prop = of_get_property(node, "cBandEdgeMaxPwrCCK", NULL);
			if (prop != NULL) {
				prRegInfo->cBandEdgeMaxPwrCCK = prop[0];
				DBGLOG(INIT, INFO, "DTB cBandEdgeMaxPwrCCK:%02x\n", prRegInfo->cBandEdgeMaxPwrCCK);
			}

			prop = of_get_property(node, "cBandEdgeMaxPwrOFDM20", NULL);
			if (prop != NULL) {
				prRegInfo->cBandEdgeMaxPwrOFDM20 = prop[0];
				DBGLOG(INIT, INFO, "DTB cBandEdgeMaxPwrOFDM20:%02x\n",
				prRegInfo->cBandEdgeMaxPwrOFDM20);
			}

			prop = of_get_property(node, "cBandEdgeMaxPwrOFDM40", NULL);
			if (prop != NULL) {
				prRegInfo->cBandEdgeMaxPwrOFDM40 = prop[0];
				DBGLOG(INIT, INFO, "DTB cBandEdgeMaxPwrOFDM40:%02x\n",
				prRegInfo->cBandEdgeMaxPwrOFDM40);
			}
		}

		/* load regulation subbands */
		prop = of_get_property(node, "ucRegChannelListMap", NULL);
		if (prop != NULL) {
			prRegInfo->eRegChannelListMap = (ENUM_REG_CH_MAP_T)prop[0];
			DBGLOG(INIT, INFO, "DTB ucRegChannelListMap:%02x\n", prRegInfo->eRegChannelListMap);
		}

		prop = of_get_property(node, "ucRegChannelListIndex", NULL);
		if (prop != NULL) {
			prRegInfo->ucRegChannelListIndex = prop[0];
			DBGLOG(INIT, INFO, "DTB ucRegChannelListIndex:%02x\n", prRegInfo->ucRegChannelListIndex);
		}

		if (prRegInfo->eRegChannelListMap == REG_CH_MAP_CUSTOMIZED) {
			prop = of_get_property(node, "aucRegSubbandInfo36", NULL);
			if (prop != NULL) {
				for (i = 0; i < MAX_SUBBAND_NUM; i++) {
					pucDest = (PUINT_8) &prRegInfo->rDomainInfo.rSubBand[i];
					memcpy(pucDest, &prop[i*sizeof(DOMAIN_SUBBAND_INFO)],
							sizeof(DOMAIN_SUBBAND_INFO));
					DBGLOG(INIT, INFO, "DTB aucRegSubbandInfo36[%d]:" MACSTR, i, pucDest);
				}
			}
		}
		/* load rssiPathCompensation */
		prop = of_get_property(node, "rRssiPathCompensation", NULL);
		if (prop != NULL) {
			prRegInfo->rRssiPathCompasation.c2GRssiCompensation = prop[0];
			prRegInfo->rRssiPathCompasation.c5GRssiCompensation = prop[1];
			DBGLOG(INIT, INFO, "DTB rRssiPathCompensation:%02x %02x\n",
					prRegInfo->rRssiPathCompasation.c2GRssiCompensation,
					prRegInfo->rRssiPathCompasation.c5GRssiCompensation);
		}
#if 1
		/* load full NVRAM */
		prRegInfo->prNvramSettings = (P_WIFI_CFG_PARAM_STRUCT)&prRegInfo->aucNvram;

		prop = of_get_property(node, "u2Part1OwnVersion", NULL);
		if (prop != NULL) {
			memcpy(&prRegInfo->prNvramSettings->u2Part1OwnVersion, prop, sizeof(UINT_16));
			DBGLOG(INIT, INFO, "DTB u2Part1OwnVersion: %04x\n",
					prRegInfo->prNvramSettings->u2Part1OwnVersion);
		}

		prop = of_get_property(node, "u2Part1PeerVersion", NULL);
		if (prop != NULL) {
			memcpy(&prRegInfo->prNvramSettings->u2Part1PeerVersion, prop, sizeof(UINT_16));
			DBGLOG(INIT, INFO, "DTB u2Part1PeerVersion: %04x\n",
					prRegInfo->prNvramSettings->u2Part1PeerVersion);
		}

		COPY_MAC_ADDR(prRegInfo->prNvramSettings->aucMacAddress, prRegInfo->aucMacAddr);
		prop = of_get_property(node, "aucCountryCode", NULL);
		if (prop != NULL) {
			memcpy(prRegInfo->prNvramSettings->aucCountryCode, prop, 2*sizeof(UINT_8));
			DBGLOG(INIT, INFO, "DTB aucCountryCode:%02x:%02x\n",
				prRegInfo->prNvramSettings->aucCountryCode[0],
				prRegInfo->prNvramSettings->aucCountryCode[1]);
		}

		memcpy(&prRegInfo->prNvramSettings->rTxPwr,
					&prRegInfo->rTxPwr, sizeof(TX_PWR_PARAM_T));

#if CFG_SUPPORT_NVRAM_5G
		memcpy(prRegInfo->prNvramSettings->EfuseMapping.aucEFUSE,
				prRegInfo->aucEFUSE, sizeof(prRegInfo->aucEFUSE));
#else
		memcpy(prRegInfo->prNvramSettings->aucEFUSE,
				prRegInfo->aucEFUSE, sizeof(prRegInfo->aucEFUSE));
#endif
		prRegInfo->prNvramSettings->ucTxPwrValid = prRegInfo->ucTxPwrValid;
		prRegInfo->prNvramSettings->ucSupport5GBand = prRegInfo->ucSupport5GBand;
		prRegInfo->prNvramSettings->fg2G4BandEdgePwrUsed = prRegInfo->fg2G4BandEdgePwrUsed;

		prop = of_get_property(node, "cBandEdgeMaxPwrCCK", NULL);
		if (prop != NULL)
			prRegInfo->prNvramSettings->cBandEdgeMaxPwrCCK = prop[0];

		prop = of_get_property(node, "cBandEdgeMaxPwrOFDM20", NULL);
		if (prop != NULL)
			prRegInfo->prNvramSettings->cBandEdgeMaxPwrOFDM20 = prop[0];

		prop = of_get_property(node, "cBandEdgeMaxPwrOFDM40", NULL);
		if (prop != NULL)
			prRegInfo->prNvramSettings->cBandEdgeMaxPwrOFDM40 = prop[0];

		prRegInfo->prNvramSettings->ucRegChannelListMap = (UINT_8)(prRegInfo->eRegChannelListMap);
		prRegInfo->prNvramSettings->ucRegChannelListIndex = prRegInfo->ucRegChannelListIndex;

		prop = of_get_property(node, "aucRegSubbandInfo36", NULL);
		if (prop != NULL)
			memcpy(prRegInfo->prNvramSettings->aucRegSubbandInfo, prop,
				sizeof(prRegInfo->prNvramSettings->aucRegSubbandInfo));

		prop = of_get_property(node, "aucReserved16", NULL);
		if (prop != NULL)
			memcpy(prRegInfo->prNvramSettings->aucReserved2, prop,
					sizeof(prRegInfo->prNvramSettings->aucReserved2));

		prop = of_get_property(node, "u2Part2OwnVersion", NULL);
		if (prop != NULL) {
			memcpy(&prRegInfo->prNvramSettings->u2Part2OwnVersion, prop, sizeof(UINT_16));
			DBGLOG(INIT, INFO, "DTB u2Part2OwnVersion: %04x\n",
					prRegInfo->prNvramSettings->u2Part2OwnVersion);
		}

		prop = of_get_property(node, "u2Part2PeerVersion", NULL);
		if (prop != NULL) {
			memcpy(&prRegInfo->prNvramSettings->u2Part2PeerVersion, prop, sizeof(UINT_16));
			DBGLOG(INIT, INFO, "DTB u2Part2PeerVersion: %04x\n",
					prRegInfo->prNvramSettings->u2Part2PeerVersion);
		}

		prRegInfo->prNvramSettings->uc2G4BwFixed20M = prRegInfo->uc2G4BwFixed20M;
		prRegInfo->prNvramSettings->uc5GBwFixed20M = prRegInfo->uc5GBwFixed20M;
		prRegInfo->prNvramSettings->ucEnable5GBand = prRegInfo->ucEnable5GBand;
		prRegInfo->prNvramSettings->ucRxDiversity = prRegInfo->ucRxDiversity;
		memcpy(&prRegInfo->prNvramSettings->rRssiPathCompensation,
				&prRegInfo->rRssiPathCompasation, sizeof(RSSI_PATH_COMPASATION_T));

		prRegInfo->prNvramSettings->fgRssiCompensationVaildbit = prRegInfo->ucRssiPathCompasationUsed;
		prRegInfo->prNvramSettings->ucGpsDesense = prRegInfo->ucGpsDesense;

		prop = of_get_property(node, "u2FeatureReserved", NULL);
		if (prop != NULL) {
			memcpy(&prRegInfo->prNvramSettings->u2FeatureReserved, prop, sizeof(UINT_16));
			DBGLOG(INIT, INFO, "DTB u2FeatureReserved: %04x\n",
					prRegInfo->prNvramSettings->u2FeatureReserved);
		}

		prop = of_get_property(node, "aucPreTailReserved", NULL);
		if (prop != NULL) {
			prRegInfo->prNvramSettings->aucPreTailReserved = prop[0];
			DBGLOG(INIT, INFO, "DTB aucPreTailReserved: %02x\n",
					prRegInfo->prNvramSettings->aucPreTailReserved);
		}

		prop = of_get_property(node, "aucTailReserved241", NULL);
		if (prop != NULL)
			memcpy(prRegInfo->prNvramSettings->aucTailReserved, prop,
					sizeof(prRegInfo->prNvramSettings->aucTailReserved));
#endif
	} else {
		DBGLOG(INIT, INFO, "glLoadNvram fail\n");
		prGlueInfo->fgNvramAvailable = FALSE;
	}

}

/*----------------------------------------------------------------------------*/
/*!
* \brief Release prDev from wlandev_array and free tasklet object related to it.
*
* \param[in] prDev  Pointer to struct net_device
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/
static void wlanClearDevIdx(struct net_device *prDev)
{
	int i;

	ASSERT(prDev);

	for (i = 0; i < CFG_MAX_WLAN_DEVICES; i++) {
		if (arWlanDevInfo[i].prDev == prDev) {
			arWlanDevInfo[i].prDev = NULL;
			u4WlanDevNum--;
		}
	}

}				/* end of wlanClearDevIdx() */

/*----------------------------------------------------------------------------*/
/*!
* \brief Allocate an unique interface index, net_device::ifindex member for this
*        wlan device. Store the net_device in wlandev_array, and initialize
*        tasklet object related to it.
*
* \param[in] prDev  Pointer to struct net_device
*
* \retval >= 0      The device number.
* \retval -1        Fail to get index.
*/
/*----------------------------------------------------------------------------*/
static int wlanGetDevIdx(struct net_device *prDev)
{
	int i;

	ASSERT(prDev);

	for (i = 0; i < CFG_MAX_WLAN_DEVICES; i++) {
		if (arWlanDevInfo[i].prDev == (struct net_device *)NULL) {
			/* Reserve 2 bytes space to store one digit of
			 * device number and NULL terminator.
			 */
			arWlanDevInfo[i].prDev = prDev;
			u4WlanDevNum++;
			return i;
		}
	}

	return -1;
}				/* end of wlanGetDevIdx() */

/*----------------------------------------------------------------------------*/
/*!
* \brief A method of struct net_device, a primary SOCKET interface to configure
*        the interface lively. Handle an ioctl call on one of our devices.
*        Everything Linux ioctl specific is done here. Then we pass the contents
*        of the ifr->data to the request message handler.
*
* \param[in] prDev      Linux kernel netdevice
*
* \param[in] prIfReq    Our private ioctl request structure, typed for the generic
*                       struct ifreq so we can use ptr to function
*
* \param[in] cmd        Command ID
*
* \retval 0  The IOCTL command is executed successfully.
* \retval <0 The execution of IOCTL command is failed.
*/
/*----------------------------------------------------------------------------*/
int wlanDoIOCTL(struct net_device *prDev, struct ifreq *prIfReq, int i4Cmd)
{
	P_GLUE_INFO_T prGlueInfo = NULL;
	int ret = 0;

	/* Verify input parameters for the following functions */
	ASSERT(prDev && prIfReq);
	if (!prDev || !prIfReq) {
		DBGLOG(INIT, ERROR, "Invalid input data\n");
		return -EINVAL;
	}

	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prDev));
	if (!prGlueInfo) {
		DBGLOG(INIT, ERROR, "prGlueInfo is NULL\n");
		return -EFAULT;
	}

	if (prGlueInfo->u4ReadyFlag == 0) {
		DBGLOG(INIT, ERROR, "Adapter is not ready\n");
		return -EINVAL;
	}

	if ((i4Cmd >= SIOCIWFIRST) && (i4Cmd < SIOCIWFIRSTPRIV)) {
		/* 0x8B00 ~ 0x8BDF, wireless extension region */
		ret = wext_support_ioctl(prDev, prIfReq, i4Cmd);
	} else if ((i4Cmd >= SIOCIWFIRSTPRIV) && (i4Cmd < SIOCIWLASTPRIV)) {
		/* 0x8BE0 ~ 0x8BFF, private ioctl region */
		ret = priv_support_ioctl(prDev, prIfReq, i4Cmd);
	} else if (i4Cmd == SIOCDEVPRIVATE + 1) {
		ret = priv_support_driver_cmd(prDev, prIfReq, i4Cmd);
	} else {
		DBGLOG(INIT, WARN, "Unexpected ioctl command: 0x%04x\n", i4Cmd);
		ret = -EOPNOTSUPP;
	}

	return ret;
}				/* end of wlanDoIOCTL() */

/*----------------------------------------------------------------------------*/
/*!
* \brief Export wlan GLUE_INFO_T pointer to p2p module
*
* \param[in]  prGlueInfo Pointer to struct GLUE_INFO_T
*
* \return TRUE: get GlueInfo pointer successfully
*            FALSE: wlan is not started yet
*/
/*---------------------------------------------------------------------------*/
P_GLUE_INFO_T wlanGetGlueInfo(VOID)
{
	if (!gprWdev)
		return NULL;
	return (P_GLUE_INFO_T) wiphy_priv(gprWdev->wiphy);
}

/*----------------------------------------------------------------------------*/
/*!
* \brief This function is to set multicast list and set rx mode.
*
* \param[in] prDev  Pointer to struct net_device
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/

static struct delayed_work workq;
static struct net_device *gPrDev;

static void wlanSetMulticastList(struct net_device *prDev)
{
	gPrDev = prDev;
	schedule_delayed_work(&workq, 0);
}

/*
 * FIXME: Since we cannot sleep in the wlanSetMulticastList, we arrange
 * another workqueue for sleeping. We don't want to block
 * tx_thread, so we can't let tx_thread to do this
 */

static void wlanSetMulticastListWorkQueue(struct work_struct *work)
{

	P_GLUE_INFO_T prGlueInfo = NULL;
	UINT_32 u4PacketFilter = 0;
	UINT_32 u4SetInfoLen;
	struct net_device *prDev = gPrDev;

	if (kalHaltLock(KAL_HALT_LOCK_TIMEOUT_NORMAL_CASE))
		return;
	if (kalIsHalted()) {
		kalHaltUnlock();
		return;
	}

	prGlueInfo = (prDev != NULL) ? *((P_GLUE_INFO_T *) netdev_priv(prDev)) : NULL;
	ASSERT(prDev);
	ASSERT(prGlueInfo);
	if (!prDev || !prGlueInfo) {
		DBGLOG(INIT, WARN, "abnormal dev or skb: prDev(0x%p), prGlueInfo(0x%p)\n", prDev, prGlueInfo);
		kalHaltUnlock();
		return;
	}

	if (prDev->flags & IFF_PROMISC)
		u4PacketFilter |= PARAM_PACKET_FILTER_PROMISCUOUS;

	if (prDev->flags & IFF_BROADCAST)
		u4PacketFilter |= PARAM_PACKET_FILTER_BROADCAST;

	if (prDev->flags & IFF_MULTICAST) {
		if ((prDev->flags & IFF_ALLMULTI) || (netdev_mc_count(prDev) > MAX_NUM_GROUP_ADDR))
			u4PacketFilter |= PARAM_PACKET_FILTER_ALL_MULTICAST;
		else
			u4PacketFilter |= PARAM_PACKET_FILTER_MULTICAST;
	}

	kalHaltUnlock();

	if (kalIoctl(prGlueInfo,
		     wlanoidSetCurrentPacketFilter,
		     &u4PacketFilter,
		     sizeof(u4PacketFilter), FALSE, FALSE, TRUE, &u4SetInfoLen) != WLAN_STATUS_SUCCESS) {
		return;
	}

	if (u4PacketFilter & PARAM_PACKET_FILTER_MULTICAST) {
		/* Prepare multicast address list */
		struct netdev_hw_addr *ha;
		PUINT_8 prMCAddrList = NULL;
		UINT_32 i = 0;

		if (kalHaltLock(KAL_HALT_LOCK_TIMEOUT_NORMAL_CASE))
			return;

		if (kalIsHalted()) {
			kalHaltUnlock();
			return;
		}

		prMCAddrList = kalMemAlloc(MAX_NUM_GROUP_ADDR * ETH_ALEN, VIR_MEM_TYPE);

		netdev_for_each_mc_addr(ha, prDev) {
			if (i < MAX_NUM_GROUP_ADDR) {
				memcpy((prMCAddrList + i * ETH_ALEN), ha->addr, ETH_ALEN);
				i++;
			}
		}

		kalHaltUnlock();

		kalIoctl(prGlueInfo,
			 wlanoidSetMulticastList, prMCAddrList, (i * ETH_ALEN), FALSE, FALSE, TRUE, &u4SetInfoLen);

		kalMemFree(prMCAddrList, VIR_MEM_TYPE, MAX_NUM_GROUP_ADDR * ETH_ALEN);
	}

}				/* end of wlanSetMulticastList() */

/*----------------------------------------------------------------------------*/
/*!
* \brief    To indicate scheduled scan has been stopped
*
* \param[in]
*           prGlueInfo
*
* \return
*           None
*/
/*----------------------------------------------------------------------------*/
VOID wlanSchedScanStoppedWorkQueue(struct work_struct *work)
{
	P_GLUE_INFO_T prGlueInfo = NULL;
	struct net_device *prDev = gPrDev;

	DBGLOG(SCN, INFO, "wlanSchedScanStoppedWorkQueue\n");
	prGlueInfo = (prDev != NULL) ? *((P_GLUE_INFO_T *) netdev_priv(prDev)) : NULL;
	if (!prGlueInfo) {
		DBGLOG(SCN, INFO, "prGlueInfo == NULL unexpected\n");
		return;
	}

	/* 2. indication to cfg80211 */
	/* 20150205 change cfg80211_sched_scan_stopped to work queue due to sched_scan_mtx dead lock issue */
	cfg80211_sched_scan_stopped(priv_to_wiphy(prGlueInfo));
	DBGLOG(SCN, INFO,
	       "cfg80211_sched_scan_stopped event send done WorkQueue thread return from wlanSchedScanStoppedWorkQueue\n");
	return;

}

/*----------------------------------------------------------------------------*/
/*
* \brief This function is TX entry point of NET DEVICE.
*
* \param[in] prSkb  Pointer of the sk_buff to be sent
* \param[in] prDev  Pointer to struct net_device
*
* \retval NETDEV_TX_OK - on success.
* \retval NETDEV_TX_BUSY - on failure, packet will be discarded by upper layer.
*/
/*----------------------------------------------------------------------------*/
int wlanHardStartXmit(struct sk_buff *prSkb, struct net_device *prDev)
{
	P_NETDEV_PRIVATE_GLUE_INFO prNetDevPrivate = (P_NETDEV_PRIVATE_GLUE_INFO) NULL;
	P_GLUE_INFO_T prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prDev));
	UINT_8 ucBssIndex;

	ASSERT(prSkb);
	ASSERT(prDev);
	ASSERT(prGlueInfo);

	prNetDevPrivate = (P_NETDEV_PRIVATE_GLUE_INFO) netdev_priv(prDev);
	ASSERT(prNetDevPrivate->prGlueInfo == prGlueInfo);
	ucBssIndex = prNetDevPrivate->ucBssIdx;
	prGlueInfo->u8SkbToDriver++;

#if CFG_SUPPORT_PASSPOINT
	if (prGlueInfo->fgIsDad) {
		/* kalPrint("[Passpoint R2] Due to ipv4_dad...TX is forbidden\n"); */
		dev_kfree_skb(prSkb);
		prGlueInfo->u8SkbFreed++;
		return NETDEV_TX_OK;
	}
	if (prGlueInfo->fgIs6Dad) {
		/* kalPrint("[Passpoint R2] Due to ipv6_dad...TX is forbidden\n"); */
		dev_kfree_skb(prSkb);
		prGlueInfo->u8SkbFreed++;

		return NETDEV_TX_OK;
	}
#endif /* CFG_SUPPORT_PASSPOINT */

	kalResetPacket(prGlueInfo, (P_NATIVE_PACKET) prSkb);

	STATS_TX_TIME_ARRIVE(prSkb);

	if (kalHardStartXmit(prSkb, prDev, prGlueInfo, ucBssIndex) == WLAN_STATUS_SUCCESS) {
		/* Successfully enqueue to Tx queue */
		/* Successfully enqueue to Tx queue */
		if (netif_carrier_ok(prDev))
			kalPerMonStart(prGlueInfo);
	}

	/* For Linux, we'll always return OK FLAG, because we'll free this skb by ourself */
	return NETDEV_TX_OK;
}				/* end of wlanHardStartXmit() */

/*----------------------------------------------------------------------------*/
/*!
* \brief A method of struct net_device, to get the network interface statistical
*        information.
*
* Whenever an application needs to get statistics for the interface, this method
* is called. This happens, for example, when ifconfig or netstat -i is run.
*
* \param[in] prDev      Pointer to struct net_device.
*
* \return net_device_stats buffer pointer.
*/
/*----------------------------------------------------------------------------*/
struct net_device_stats *wlanGetStats(IN struct net_device *prDev)
{
	return (struct net_device_stats *)kalGetStats(prDev);
}				/* end of wlanGetStats() */

VOID wlanDebugInit(VOID)
{
	UINT_8 i;

	/* Set the initial debug level of each module */
#if DBG
	for (i = 0; i < DBG_MODULE_NUM; i++)
		aucDebugModule[i] = DBG_CLASS_MASK;	/* enable all */
#else
	for (i = 0; i < DBG_MODULE_NUM; i++)
		aucDebugModule[i] = DBG_CLASS_ERROR | DBG_CLASS_WARN | DBG_CLASS_STATE | DBG_CLASS_INFO;

	aucDebugModule[DBG_INTR_IDX] = DBG_CLASS_ERROR;
#endif /* DBG */

	LOG_FUNC("Reset ALL DBG module log level to DEFAULT!");

}

WLAN_STATUS wlanSetDebugLevel(IN UINT_32 u4DbgIdx, IN UINT_32 u4DbgMask)
{
	UINT_32 u4Idx;
	WLAN_STATUS fgStatus = WLAN_STATUS_SUCCESS;

	if (u4DbgIdx == DBG_ALL_MODULE_IDX) {
		for (u4Idx = 0; u4Idx < DBG_MODULE_NUM; u4Idx++)
			aucDebugModule[u4Idx] = (UINT_8) u4DbgMask;
		LOG_FUNC("Set ALL DBG module log level to [0x%02x]\n", u4DbgMask);
	} else if (u4DbgIdx < DBG_MODULE_NUM) {
		aucDebugModule[u4DbgIdx] = (UINT_8) u4DbgMask;
		LOG_FUNC("Set DBG module[%u] log level to [0x%02x]\n", u4DbgIdx, u4DbgMask);
	} else {
		fgStatus = WLAN_STATUS_FAILURE;
	}

	return fgStatus;
}

WLAN_STATUS wlanGetDebugLevel(IN UINT_32 u4DbgIdx, OUT PUINT_32 pu4DbgMask)
{
	if (u4DbgIdx < DBG_MODULE_NUM) {
		*pu4DbgMask = aucDebugModule[u4DbgIdx];
		return WLAN_STATUS_SUCCESS;
	}

	return WLAN_STATUS_FAILURE;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief A function for prDev->init
*
* \param[in] prDev      Pointer to struct net_device.
*
* \retval 0         The execution of wlanInit succeeds.
* \retval -ENXIO    No such device.
*/
/*----------------------------------------------------------------------------*/
static int wlanInit(struct net_device *prDev)
{
	P_GLUE_INFO_T prGlueInfo = NULL;

	if (!prDev)
		return -ENXIO;

	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prDev));
	INIT_DELAYED_WORK(&workq, wlanSetMulticastListWorkQueue);

/* 20150205 work queue for sched_scan */
	INIT_DELAYED_WORK(&sched_workq, wlanSchedScanStoppedWorkQueue);

	return 0;		/* success */
}				/* end of wlanInit() */

/*----------------------------------------------------------------------------*/
/*!
* \brief A function for prDev->uninit
*
* \param[in] prDev      Pointer to struct net_device.
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/
static void wlanUninit(struct net_device *prDev)
{
}				/* end of wlanUninit() */

/*----------------------------------------------------------------------------*/
/*!
* \brief A function for prDev->open
*
* \param[in] prDev      Pointer to struct net_device.
*
* \retval 0     The execution of wlanOpen succeeds.
* \retval < 0   The execution of wlanOpen failed.
*/
/*----------------------------------------------------------------------------*/
static int wlanOpen(struct net_device *prDev)
{
	ASSERT(prDev);

	netif_tx_start_all_queues(prDev);

	return 0;		/* success */
}				/* end of wlanOpen() */

/*----------------------------------------------------------------------------*/
/*!
* \brief A function for prDev->stop
*
* \param[in] prDev      Pointer to struct net_device.
*
* \retval 0     The execution of wlanStop succeeds.
* \retval < 0   The execution of wlanStop failed.
*/
/*----------------------------------------------------------------------------*/
static int wlanStop(struct net_device *prDev)
{
	P_GLUE_INFO_T prGlueInfo = NULL;
	struct cfg80211_scan_request *prScanRequest = NULL;

	GLUE_SPIN_LOCK_DECLARATION();

	ASSERT(prDev);

	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prDev));

	/* CFG80211 down */
	GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
	if (prGlueInfo->prScanRequest != NULL) {
		prScanRequest = prGlueInfo->prScanRequest;
		prGlueInfo->prScanRequest = NULL;
	}
	GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);

	if (prScanRequest)
		cfg80211_scan_done(prScanRequest, TRUE);
	netif_tx_stop_all_queues(prDev);

	return 0;		/* success */
}				/* end of wlanStop() */

#if CFG_SUPPORT_SNIFFER
static int wlanMonOpen(struct net_device *prDev)
{
	ASSERT(prDev);

	netif_tx_start_all_queues(prDev);

	return 0;		/* success */
}

static int wlanMonStop(struct net_device *prDev)
{
	ASSERT(prDev);

	netif_tx_stop_all_queues(prDev);

	return 0;		/* success */
}

static const struct net_device_ops wlan_mon_netdev_ops = {
	.ndo_open = wlanMonOpen,
	.ndo_stop = wlanMonStop,
};

void wlanMonWorkHandler(struct work_struct *work)
{
	P_GLUE_INFO_T prGlueInfo;

	prGlueInfo = container_of(work, GLUE_INFO_T, monWork);

	if (prGlueInfo->fgIsEnableMon) {
		if (prGlueInfo->prMonDevHandler)
			return;

		prGlueInfo->prMonDevHandler =
		    alloc_netdev_mq(sizeof(NETDEV_PRIVATE_GLUE_INFO), NIC_MONITOR_INF_NAME,
					NET_NAME_PREDICTABLE, ether_setup, CFG_MAX_TXQ_NUM);

		if (prGlueInfo->prMonDevHandler == NULL) {
			DBGLOG(INIT, ERROR, "wlanMonWorkHandler: Allocated prMonDevHandler context FAIL.\n");
			return;
		}

		((P_NETDEV_PRIVATE_GLUE_INFO) netdev_priv(prGlueInfo->prMonDevHandler))->prGlueInfo = prGlueInfo;
		prGlueInfo->prMonDevHandler->type = ARPHRD_IEEE80211_RADIOTAP;
		prGlueInfo->prMonDevHandler->netdev_ops = &wlan_mon_netdev_ops;
		netif_carrier_off(prGlueInfo->prMonDevHandler);
		netif_tx_stop_all_queues(prGlueInfo->prMonDevHandler);
		kalResetStats(prGlueInfo->prMonDevHandler);

		if (register_netdev(prGlueInfo->prMonDevHandler) < 0) {
			DBGLOG(INIT, ERROR, "wlanMonWorkHandler: Registered prMonDevHandler context FAIL.\n");
			free_netdev(prGlueInfo->prMonDevHandler);
			prGlueInfo->prMonDevHandler = NULL;
		}
		DBGLOG(INIT, INFO, "wlanMonWorkHandler: Registered prMonDevHandler context DONE.\n");
	} else {
		if (prGlueInfo->prMonDevHandler) {
			unregister_netdev(prGlueInfo->prMonDevHandler);
			prGlueInfo->prMonDevHandler = NULL;
			DBGLOG(INIT, INFO, "wlanMonWorkHandler: unRegistered prMonDevHandler context DONE.\n");
		}
	}
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * \brief Update channel table for cfg80211 based on current country domain
 *
 * \param[in] prGlueInfo      Pointer to glue info
 *
 * \return   none
 */
/*----------------------------------------------------------------------------*/
VOID wlanUpdateChannelTable(P_GLUE_INFO_T prGlueInfo)
{
	UINT_8 i, j;
	UINT_8 ucNumOfChannel;
	RF_CHANNEL_INFO_T aucChannelList[ARRAY_SIZE(mtk_2ghz_channels) + ARRAY_SIZE(mtk_5ghz_channels)];

	/* 1. Disable all channels */
	for (i = 0; i < ARRAY_SIZE(mtk_2ghz_channels); i++) {
		mtk_2ghz_channels[i].flags |= IEEE80211_CHAN_DISABLED;
		mtk_2ghz_channels[i].orig_flags |= IEEE80211_CHAN_DISABLED;
	}

	for (i = 0; i < ARRAY_SIZE(mtk_5ghz_channels); i++) {
		mtk_5ghz_channels[i].flags |= IEEE80211_CHAN_DISABLED;
		mtk_5ghz_channels[i].orig_flags |= IEEE80211_CHAN_DISABLED;
	}

	/* 2. Get current domain channel list */
	rlmDomainGetChnlList(prGlueInfo->prAdapter,
			     BAND_NULL, FALSE,
			     ARRAY_SIZE(mtk_2ghz_channels) + ARRAY_SIZE(mtk_5ghz_channels),
			     &ucNumOfChannel, aucChannelList);

	/* 3. Enable specific channel based on domain channel list */
	for (i = 0; i < ucNumOfChannel; i++) {
		switch (aucChannelList[i].eBand) {
		case BAND_2G4:
			for (j = 0; j < ARRAY_SIZE(mtk_2ghz_channels); j++) {
				if (mtk_2ghz_channels[j].hw_value == aucChannelList[i].ucChannelNum) {
					mtk_2ghz_channels[j].flags &= ~IEEE80211_CHAN_DISABLED;
					mtk_2ghz_channels[j].orig_flags &= ~IEEE80211_CHAN_DISABLED;
					break;
				}
			}
			break;

		case BAND_5G:
			for (j = 0; j < ARRAY_SIZE(mtk_5ghz_channels); j++) {
				if (mtk_5ghz_channels[j].hw_value == aucChannelList[i].ucChannelNum) {
					mtk_5ghz_channels[j].flags &= ~IEEE80211_CHAN_DISABLED;
					mtk_5ghz_channels[j].orig_flags &= ~IEEE80211_CHAN_DISABLED;
					break;
				}
			}
			break;

		default:
			DBGLOG(INIT, WARN, "Unknown band %d\n", aucChannelList[i].eBand);
			break;
		}
	}
}

/*----------------------------------------------------------------------------*/
/*!
* \brief Register the device to the kernel and return the index.
*
* \param[in] prDev      Pointer to struct net_device.
*
* \retval 0     The execution of wlanNetRegister succeeds.
* \retval < 0   The execution of wlanNetRegister failed.
*/
/*----------------------------------------------------------------------------*/
static INT_32 wlanNetRegister(struct wireless_dev *prWdev)
{
	P_GLUE_INFO_T prGlueInfo;
	INT_32 i4DevIdx = -1;
	P_NETDEV_PRIVATE_GLUE_INFO prNetDevPrivate = (P_NETDEV_PRIVATE_GLUE_INFO) NULL;

	ASSERT(prWdev);

	do {
		if (!prWdev)
			break;

		prGlueInfo = (P_GLUE_INFO_T) wiphy_priv(prWdev->wiphy);
		i4DevIdx = wlanGetDevIdx(prWdev->netdev);
		if (i4DevIdx < 0) {
			DBGLOG(INIT, ERROR, "net_device number exceeds!\n");
			break;
		}

		if (register_netdev(prWdev->netdev) < 0) {
			DBGLOG(INIT, ERROR, "Register net_device failed\n");
			wlanClearDevIdx(prWdev->netdev);
			i4DevIdx = -1;
		}
#if 1
		prNetDevPrivate = (P_NETDEV_PRIVATE_GLUE_INFO) netdev_priv(prGlueInfo->prDevHandler);
		ASSERT(prNetDevPrivate->prGlueInfo == prGlueInfo);
		prNetDevPrivate->ucBssIdx = prGlueInfo->prAdapter->prAisBssInfo->ucBssIndex;
		wlanBindBssIdxToNetInterface(prGlueInfo,
					     prGlueInfo->prAdapter->prAisBssInfo->ucBssIndex, (PVOID) prWdev->netdev);
#else
		wlanBindBssIdxToNetInterface(prGlueInfo,
					     prGlueInfo->prAdapter->prAisBssInfo->ucBssIndex, (PVOID) prWdev->netdev);
		/* wlanBindNetInterface(prGlueInfo, NET_DEV_WLAN_IDX, (PVOID)prWdev->netdev); */
#endif
		if (i4DevIdx != -1)
			prGlueInfo->fgIsRegistered = TRUE;

	} while (FALSE);

	return i4DevIdx;	/* success */
}				/* end of wlanNetRegister() */

/*----------------------------------------------------------------------------*/
/*!
* \brief Unregister the device from the kernel
*
* \param[in] prWdev      Pointer to struct net_device.
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/
static VOID wlanNetUnregister(struct wireless_dev *prWdev)
{
	P_GLUE_INFO_T prGlueInfo;

	if (!prWdev) {
		DBGLOG(INIT, ERROR, "wlanNetUnregister: The device context is NULL\n");
		return;
	}

	prGlueInfo = (P_GLUE_INFO_T) wiphy_priv(prWdev->wiphy);

	wlanClearDevIdx(prWdev->netdev);
	unregister_netdev(prWdev->netdev);

	prGlueInfo->fgIsRegistered = FALSE;

#if CFG_SUPPORT_SNIFFER
	if (prGlueInfo->prMonDevHandler) {
		unregister_netdev(prGlueInfo->prMonDevHandler);
		prGlueInfo->prMonDevHandler = NULL;
	}
	prGlueInfo->fgIsEnableMon = FALSE;
#endif

}				/* end of wlanNetUnregister() */

static const struct net_device_ops wlan_netdev_ops = {
	.ndo_open = wlanOpen,
	.ndo_stop = wlanStop,
	.ndo_set_rx_mode = wlanSetMulticastList,
	.ndo_get_stats = wlanGetStats,
	.ndo_do_ioctl = wlanDoIOCTL,
	.ndo_start_xmit = wlanHardStartXmit,
	.ndo_init = wlanInit,
	.ndo_uninit = wlanUninit,
	.ndo_select_queue = wlanSelectQueue,
};

static void createWirelessDevice(void)
{
	struct wiphy *prWiphy = NULL;
	struct wireless_dev *prWdev = NULL;

	/* 4 <1.1> Create wireless_dev */
	prWdev = kzalloc(sizeof(struct wireless_dev), GFP_KERNEL);
	if (!prWdev) {
		DBGLOG(INIT, ERROR, "Allocating memory to wireless_dev context failed\n");
		return;
	}
	/* 4 <1.2> Create wiphy */
	prWiphy = wiphy_new(&mtk_wlan_ops, sizeof(GLUE_INFO_T));
	if (!prWiphy) {
		DBGLOG(INIT, ERROR, "Allocating memory to wiphy device failed\n");
		goto free_wdev;
	}
	/* 4 <1.3> configure wireless_dev & wiphy */
	prWdev->iftype = NL80211_IFTYPE_STATION;
	prWiphy->max_scan_ssids = SCN_SSID_MAX_NUM + 1; /* include one wildcard ssid */
	prWiphy->max_scan_ie_len = 512;
#if CFG_SUPPORT_SCHED_SCN_SSID_SETS
	prWiphy->max_sched_scan_ssids     = CFG_SCAN_HIDDEN_SSID_MAX_NUM;
#else
	prWiphy->max_sched_scan_ssids     = CFG_SCAN_SSID_MAX_NUM;
#endif
	prWiphy->max_match_sets           = CFG_SCAN_SSID_MATCH_MAX_NUM;
	prWiphy->max_sched_scan_ie_len    = CFG_CFG80211_IE_BUF_LEN;
	prWiphy->interface_modes = BIT(NL80211_IFTYPE_STATION) | BIT(NL80211_IFTYPE_ADHOC);
	prWiphy->bands[IEEE80211_BAND_2GHZ] = &mtk_band_2ghz;
	/*
	 * always assign 5Ghz bands here, if the chip is not support 5Ghz,
	 * bands[IEEE80211_BAND_5GHZ] will be assign to NULL
	 */
	prWiphy->bands[IEEE80211_BAND_5GHZ] = &mtk_band_5ghz;
	prWiphy->signal_type = CFG80211_SIGNAL_TYPE_MBM;
	prWiphy->cipher_suites = mtk_cipher_suites;
	prWiphy->n_cipher_suites = ARRAY_SIZE(mtk_cipher_suites);
	prWiphy->flags = WIPHY_FLAG_SUPPORTS_FW_ROAM | WIPHY_FLAG_HAS_REMAIN_ON_CHANNEL |
		WIPHY_FLAG_SUPPORTS_SCHED_SCAN;
	prWiphy->regulatory_flags = REGULATORY_CUSTOM_REG;
#if CFG_SUPPORT_TDLS
	TDLSEX_WIPHY_FLAGS_INIT(prWiphy->flags);
	prWiphy->flags |= WIPHY_FLAG_SUPPORTS_FW_ROAM |
			WIPHY_FLAG_TDLS_EXTERNAL_SETUP | WIPHY_FLAG_SUPPORTS_TDLS;
#endif /* CFG_SUPPORT_TDLS */
	prWiphy->max_remain_on_channel_duration = 5000;
	prWiphy->mgmt_stypes = mtk_cfg80211_ais_default_mgmt_stypes;
	prWiphy->vendor_commands = mtk_wlan_vendor_ops;
	prWiphy->n_vendor_commands = sizeof(mtk_wlan_vendor_ops) / sizeof(struct wiphy_vendor_command);
	prWiphy->vendor_events = mtk_wlan_vendor_events;
	prWiphy->n_vendor_events = ARRAY_SIZE(mtk_wlan_vendor_events);

	/* 4 <1.4> wowlan support */
#ifdef CONFIG_PM
	prWiphy->wowlan = &mtk_wlan_wowlan_support;
#endif
#ifdef CONFIG_CFG80211_WEXT
	/* 4 <1.5> Use wireless extension to replace IOCTL */
	prWiphy->wext = &wext_handler_def;
#endif

	if (wiphy_register(prWiphy) < 0) {
		DBGLOG(INIT, ERROR, "wiphy_register error\n");
		goto free_wiphy;
	}
	prWdev->wiphy = prWiphy;
	gprWdev = prWdev;
	DBGLOG(INIT, INFO, "Create wireless device success\n");
	return;

free_wiphy:
	wiphy_free(prWiphy);
free_wdev:
	kfree(prWdev);
}

static void destroyWirelessDevice(void)
{
	wiphy_unregister(gprWdev->wiphy);
	wiphy_free(gprWdev->wiphy);
	kfree(gprWdev);
	gprWdev = NULL;
}

VOID wlanWakeLockInit(P_GLUE_INFO_T prGlueInfo)
{
	KAL_WAKE_LOCK_INIT(NULL, &prGlueInfo->rIntrWakeLock, "WLAN interrupt");
	KAL_WAKE_LOCK_INIT(NULL, &prGlueInfo->rTimeoutWakeLock, "WLAN timeout");
}

VOID wlanWakeLockUninit(P_GLUE_INFO_T prGlueInfo)
{
	if (KAL_WAKE_LOCK_ACTIVE(NULL, &prGlueInfo->rIntrWakeLock))
		KAL_WAKE_UNLOCK(NULL, &prGlueInfo->rIntrWakeLock);
	KAL_WAKE_LOCK_DESTROY(NULL, &prGlueInfo->rIntrWakeLock);

	if (KAL_WAKE_LOCK_ACTIVE(NULL, &prGlueInfo->rTimeoutWakeLock))
		KAL_WAKE_UNLOCK(NULL, &prGlueInfo->rTimeoutWakeLock);
	KAL_WAKE_LOCK_DESTROY(NULL, &prGlueInfo->rTimeoutWakeLock);
}

/*----------------------------------------------------------------------------*/
/*!
* \brief A method for creating Linux NET4 struct net_device object and the
*        private data(prGlueInfo and prAdapter). Setup the IO address to the HIF.
*        Assign the function pointer to the net_device object
*
* \param[in] pvData     Memory address for the device
*
* \retval Not null      The wireless_dev object.
* \retval NULL          Fail to create wireless_dev object
*/
/*----------------------------------------------------------------------------*/
static struct lock_class_key rSpinKey[SPIN_LOCK_NUM];
static struct wireless_dev *wlanNetCreate(PVOID pvData)
{
	struct wireless_dev *prWdev = gprWdev;
	P_GLUE_INFO_T prGlueInfo = NULL;
	P_ADAPTER_T prAdapter = NULL;
	UINT_32 i;
	struct device *prDev;
	P_NETDEV_PRIVATE_GLUE_INFO prNetDevPrivate = (P_NETDEV_PRIVATE_GLUE_INFO) NULL;

	if (prWdev == NULL) {
		DBGLOG(INIT, ERROR, "No wireless dev exist, abort power on\n");
		return NULL;
	}

	/* 4 <1.3> co-relate wiphy & prDev */
#if (MTK_WCN_HIF_SDIO == 1)
	mtk_wcn_hif_sdio_get_dev(*((MTK_WCN_HIF_SDIO_CLTCTX *) pvData), &prDev);
#else
	prDev = pvData;
#endif
	if (!prDev)
		DBGLOG(INIT, ERROR, "unable to get struct dev for wlan\n");
	/*
	 * don't set prDev as parent of wiphy->dev, because we have done device_add
	 * in driver init. if we set parent here, parent will be not able to know this child,
	 * and may occurs a KE in device_shutdown, to free wiphy->dev, because his parent
	 * has been freed.
	 */
	/*set_wiphy_dev(prWdev->wiphy, prDev); */

	/* 4 <2> Create Glue structure */
	prGlueInfo = (P_GLUE_INFO_T) wiphy_priv(prWdev->wiphy);
	kalMemZero(prGlueInfo, sizeof(GLUE_INFO_T));

	/* 4 <3> Initialize Glue structure */
	/* 4 <3.1> Create net device */
	prGlueInfo->prDevHandler = alloc_netdev_mq(sizeof(NETDEV_PRIVATE_GLUE_INFO), NIC_INF_NAME,
						   NET_NAME_PREDICTABLE, ether_setup, CFG_MAX_TXQ_NUM);
	if (!prGlueInfo->prDevHandler) {
		DBGLOG(INIT, ERROR, "Allocating memory to net_device context failed\n");
		goto netcreate_err;
	}
	DBGLOG(INIT, INFO, "net_device prDev(0x%p) allocated\n", prGlueInfo->prDevHandler);

	/* 4 <3.1.1> Initialize net device varaiables */
#if 1
	prNetDevPrivate = (P_NETDEV_PRIVATE_GLUE_INFO) netdev_priv(prGlueInfo->prDevHandler);
	prNetDevPrivate->prGlueInfo = prGlueInfo;
#else
	*((P_GLUE_INFO_T *) netdev_priv(prGlueInfo->prDevHandler)) = prGlueInfo;
#endif
	prGlueInfo->prDevHandler->netdev_ops = &wlan_netdev_ops;
#ifdef CONFIG_WIRELESS_EXT
	prGlueInfo->prDevHandler->wireless_handlers = &wext_handler_def;
#endif
	netif_carrier_off(prGlueInfo->prDevHandler);
	netif_tx_stop_all_queues(prGlueInfo->prDevHandler);
	kalResetStats(prGlueInfo->prDevHandler);

#if CFG_SUPPORT_SNIFFER
	INIT_WORK(&(prGlueInfo->monWork), wlanMonWorkHandler);
#endif

	/* 4 <3.1.2> co-relate with wiphy bi-directionally */
	prGlueInfo->prDevHandler->ieee80211_ptr = prWdev;
#if CFG_TCP_IP_CHKSUM_OFFLOAD
	prGlueInfo->prDevHandler->features = NETIF_F_HW_CSUM;
#endif
	prWdev->netdev = prGlueInfo->prDevHandler;

	/* 4 <3.1.3> co-relate net device & prDev */
	SET_NETDEV_DEV(prGlueInfo->prDevHandler, prDev);

	/* 4 <3.2> Initialize Glue variables */
	prGlueInfo->eParamMediaStateIndicated = PARAM_MEDIA_STATE_DISCONNECTED;
	prGlueInfo->ePowerState = ParamDeviceStateD0;
	prGlueInfo->fgIsMacAddrOverride = FALSE;
	prGlueInfo->fgIsRegistered = FALSE;
	prGlueInfo->prScanRequest = NULL;
	prGlueInfo->prSchedScanRequest = NULL;
	prGlueInfo->puScanChannel = NULL;

#if CFG_SUPPORT_PASSPOINT
	/* Init DAD */
	prGlueInfo->fgIsDad = FALSE;
	prGlueInfo->fgIs6Dad = FALSE;
	kalMemZero(prGlueInfo->aucDADipv4, 4);
	kalMemZero(prGlueInfo->aucDADipv6, 16);
#endif /* CFG_SUPPORT_PASSPOINT */

	init_completion(&prGlueInfo->rScanComp);
	init_completion(&prGlueInfo->rHaltComp);
	init_completion(&prGlueInfo->rPendComp);

#if CFG_SUPPORT_MULTITHREAD
	init_completion(&prGlueInfo->rHifHaltComp);
	init_completion(&prGlueInfo->rRxHaltComp);
	init_completion(&prGlueInfo->rHalRDMCRComp);
	init_completion(&prGlueInfo->rHalWRMCRComp);
#endif

	/* initialize timer for OID timeout checker */
	kalOsTimerInitialize(prGlueInfo, kalTimeoutHandler);

	for (i = 0; i < SPIN_LOCK_NUM; i++) {
		spin_lock_init(&prGlueInfo->rSpinLock[i]);
		lockdep_set_class(&prGlueInfo->rSpinLock[i], &rSpinKey[i]);
	}

	for (i = 0; i < MUTEX_NUM; i++)
		mutex_init(&prGlueInfo->arMutex[i]);

	/* initialize semaphore for ioctl */
	sema_init(&prGlueInfo->ioctl_sem, 1);

#if CFG_SUPPORT_SDIO_READ_WRITE_PATTERN
	/* initialize SDIO read-write pattern control */
	prGlueInfo->fgEnSdioTestPattern = FALSE;
	prGlueInfo->fgIsSdioTestInitialized = FALSE;
#endif

	/* 4 <8> Init Queues */
	init_waitqueue_head(&prGlueInfo->waitq);
	QUEUE_INITIALIZE(&prGlueInfo->rCmdQueue);
	QUEUE_INITIALIZE(&prGlueInfo->rTxQueue);
	glSetHifInfo(prGlueInfo, (ULONG) pvData);

	/* main thread is created in this function */
#if CFG_SUPPORT_MULTITHREAD
	init_waitqueue_head(&prGlueInfo->waitq_rx);
	init_waitqueue_head(&prGlueInfo->waitq_hif);

	prGlueInfo->u4TxThreadPid = 0xffffffff;
	prGlueInfo->u4RxThreadPid = 0xffffffff;
	prGlueInfo->u4HifThreadPid = 0xffffffff;
#endif

	/* 4 <4> Create Adapter structure */
	prAdapter = (P_ADAPTER_T) wlanAdapterCreate(prGlueInfo);

	if (!prAdapter) {
		DBGLOG(INIT, ERROR, "Allocating memory to adapter failed\n");
		glClearHifInfo(prGlueInfo);
		goto netcreate_err;
	}

	prGlueInfo->prAdapter = prAdapter;

	goto netcreate_done;

netcreate_err:
	if (prAdapter != NULL) {
		wlanAdapterDestroy(prAdapter);
		prAdapter = NULL;
	}

	if (prGlueInfo->prDevHandler != NULL) {
		free_netdev(prGlueInfo->prDevHandler);
		prGlueInfo->prDevHandler = NULL;
	}

netcreate_done:

	return prWdev;
}				/* end of wlanNetCreate() */

/*----------------------------------------------------------------------------*/
/*!
* \brief Destroying the struct net_device object and the private data.
*
* \param[in] prWdev      Pointer to struct wireless_dev.
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/
static VOID wlanNetDestroy(struct wireless_dev *prWdev)
{
	P_GLUE_INFO_T prGlueInfo = NULL;

	ASSERT(prWdev);

	if (!prWdev) {
		DBGLOG(INIT, ERROR, "wlanNetDestroy: The device context is NULL\n");
		return;
	}

	/* prGlueInfo is allocated with net_device */
	prGlueInfo = (P_GLUE_INFO_T) wiphy_priv(prWdev->wiphy);
	ASSERT(prGlueInfo);

	/* destroy kal OS timer */
	kalCancelTimer(prGlueInfo);

	glClearHifInfo(prGlueInfo);

	wlanAdapterDestroy(prGlueInfo->prAdapter);
	prGlueInfo->prAdapter = NULL;

	/* Free net_device and private data, which are allocated by alloc_netdev().
	 */
	free_netdev(prWdev->netdev);

}				/* end of wlanNetDestroy() */

VOID wlanSetSuspendMode(P_GLUE_INFO_T prGlueInfo, BOOLEAN fgEnable)
{
	struct net_device *prDev = NULL;

	if (!prGlueInfo)
		return;

#if CFG_ROAMING_CTRL_BY_SUSPEND
	{
		UINT_32 u4SetInfoLen = 0;

		kalIoctl(prGlueInfo, wlanoidSetRoamingCtrl, &fgEnable, sizeof(fgEnable),
					FALSE, FALSE, TRUE, &u4SetInfoLen);
	}
#endif
	prDev = prGlueInfo->prDevHandler;
	if (!prDev)
		return;

	kalSetNetAddressFromInterface(prGlueInfo, prDev, fgEnable);
}

#if CFG_ENABLE_EARLY_SUSPEND
static struct early_suspend wlan_early_suspend_desc = {
	.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN,
};

static void wlan_early_suspend(struct early_suspend *h)
{
	struct net_device *prDev = NULL;
	P_GLUE_INFO_T prGlueInfo = NULL;

	/* 4 <1> Sanity Check */
	if ((u4WlanDevNum == 0) && (u4WlanDevNum > CFG_MAX_WLAN_DEVICES)) {
		DBGLOG(INIT, ERROR, "wlanLateResume u4WlanDevNum==0 invalid!!\n");
		return;
	}

	prDev = arWlanDevInfo[u4WlanDevNum - 1].prDev;
	if (!prDev)
		return;

	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prDev));
	if (!prGlueInfo)
		return;

	DBGLOG(INIT, INFO, "********<%s>********\n", __func__);

	if (prGlueInfo->fgIsInSuspendMode == TRUE) {
		DBGLOG(INIT, INFO, "%s: Already in suspend mode, SKIP!\n", __func__);
		return;
	}

	prGlueInfo->fgIsInSuspendMode = TRUE;

	wlanSetSuspendMode(prGlueInfo, TRUE);
	p2pSetSuspendMode(prGlueInfo, TRUE);
}

static void wlan_late_resume(struct early_suspend *h)
{
	struct net_device *prDev = NULL;
	P_GLUE_INFO_T prGlueInfo = NULL;

	/* 4 <1> Sanity Check */
	if ((u4WlanDevNum == 0) && (u4WlanDevNum > CFG_MAX_WLAN_DEVICES)) {
		DBGLOG(INIT, ERROR, "wlanLateResume u4WlanDevNum==0 invalid!!\n");
		return;
	}

	prDev = arWlanDevInfo[u4WlanDevNum - 1].prDev;
	if (!prDev)
		return;

	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prDev));
	if (!prGlueInfo)
		return;

	DBGLOG(INIT, INFO, "********<%s>********\n", __func__);

	if (prGlueInfo->fgIsInSuspendMode == FALSE) {
		DBGLOG(INIT, INFO, "%s: Not in suspend mode, SKIP!\n", __func__);
		return;
	}

	prGlueInfo->fgIsInSuspendMode = FALSE;

	/* 4 <2> Set suspend mode for each network */
	wlanSetSuspendMode(prGlueInfo, FALSE);
	p2pSetSuspendMode(prGlueInfo, FALSE);
}
#endif

VOID nicConfigProcSetCamCfgWrite(BOOLEAN enabled)
{
	struct net_device *prDev = NULL;
	P_GLUE_INFO_T prGlueInfo = NULL;
	P_ADAPTER_T prAdapter = NULL;
	PARAM_POWER_MODE ePowerMode;
	UINT_8 ucBssIndex;
	CMD_PS_PROFILE_T rPowerSaveMode;

	/* 4 <1> Sanity Check */
	if ((u4WlanDevNum == 0) && (u4WlanDevNum > CFG_MAX_WLAN_DEVICES)) {
		DBGLOG(INIT, ERROR, "wlanLateResume u4WlanDevNum==0 invalid!!\n");
		return;
	}

	prDev = arWlanDevInfo[u4WlanDevNum - 1].prDev;
	if (!prDev)
		return;

	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prDev));
	if (!prGlueInfo)
		return;

	prAdapter = prGlueInfo->prAdapter;
	if ((!prAdapter) || (!prAdapter->prAisBssInfo))
		return;

	ucBssIndex = prAdapter->prAisBssInfo->ucBssIndex;
	if (ucBssIndex >= BSS_INFO_NUM)
		return;
	rPowerSaveMode.ucBssIndex = ucBssIndex;

	if (enabled) {
		prAdapter->rWlanInfo.fgEnSpecPwrMgt = TRUE;
		ePowerMode = Param_PowerModeCAM;
		rPowerSaveMode.ucPsProfile = (UINT_8) ePowerMode;
		DBGLOG(INIT, INFO, "Enable CAM BssIndex:%d, PowerMode:%d\n",
		       ucBssIndex, rPowerSaveMode.ucPsProfile);
	} else {
		prAdapter->rWlanInfo.fgEnSpecPwrMgt = FALSE;
		rPowerSaveMode.ucPsProfile =
				prAdapter->rWlanInfo.arPowerSaveMode[ucBssIndex].ucPsProfile;
		DBGLOG(INIT, INFO, "Disable CAM BssIndex:%d, PowerMode:%d\n",
		       ucBssIndex, rPowerSaveMode.ucPsProfile);
	}

	wlanSendSetQueryCmd(prAdapter,
			    CMD_ID_POWER_SAVE_MODE,
			    TRUE,
			    FALSE,
			    FALSE,
			    NULL,
			    NULL,
			    sizeof(CMD_PS_PROFILE_T),
			    (PUINT_8) &rPowerSaveMode,
			    NULL, 0);
}

int set_p2p_mode_handler(struct net_device *netdev, PARAM_CUSTOM_P2P_SET_STRUCT_T p2pmode)
{
	P_GLUE_INFO_T prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(netdev));
	PARAM_CUSTOM_P2P_SET_STRUCT_T rSetP2P;
	WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
	UINT_32 u4BufLen = 0;

	rSetP2P.u4Enable = p2pmode.u4Enable;
	rSetP2P.u4Mode = p2pmode.u4Mode;

	if ((!rSetP2P.u4Enable) && (fgIsResetting == FALSE))
		p2pNetUnregister(prGlueInfo, FALSE);

	rStatus = kalIoctl(prGlueInfo,
			   wlanoidSetP2pMode,
			   &rSetP2P, sizeof(PARAM_CUSTOM_P2P_SET_STRUCT_T), FALSE, FALSE, TRUE, &u4BufLen);
	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(INIT, ERROR, "kalIoctl failed: 0x%08lx\n", (UINT_32) rStatus);
		return -1;
	}

	if ((rSetP2P.u4Enable) && (fgIsResetting == FALSE))
		p2pNetRegister(prGlueInfo, FALSE);

	return 0;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief Wlan probe function. This function probes and initializes the device.
*
* \param[in] pvData     data passed by bus driver init function
*                           _HIF_EHPI: NULL
*                           _HIF_SDIO: sdio bus driver handle
*
* \retval 0 Success
* \retval negative value Failed
*/
/*----------------------------------------------------------------------------*/
static INT_32 wlanProbe(PVOID pvData)
{
	struct wireless_dev *prWdev = NULL;
	enum ENUM_PROBE_FAIL_REASON {
		BUS_INIT_FAIL,
		NET_CREATE_FAIL,
		BUS_SET_IRQ_FAIL,
		ADAPTER_START_FAIL,
		NET_REGISTER_FAIL,
		PROC_INIT_FAIL,
		FAIL_MET_INIT_PROCFS,
		FAIL_REASON_NUM
	} eFailReason;
	P_WLANDEV_INFO_T prWlandevInfo = NULL;
	INT_32 i4DevIdx = 0;
	P_GLUE_INFO_T prGlueInfo = NULL;
	P_ADAPTER_T prAdapter = NULL;
	INT_32 i4Status = 0;
	BOOL bRet = FALSE;

	eFailReason = FAIL_REASON_NUM;
	do {
		/* 4 <1> Initialize the IO port of the interface */
		/*  GeorgeKuo: pData has different meaning for _HIF_XXX:
		 * _HIF_EHPI: pointer to memory base variable, which will be
		 *      initialized by glBusInit().
		 * _HIF_SDIO: bus driver handle
		 */
		bRet = glBusInit(pvData);
#if (CFG_SUPPORT_TRACE_TC4 == 1)
		wlanDebugTC4Init();
#endif
		/* Cannot get IO address from interface */
		if (bRet == FALSE) {
			DBGLOG(INIT, ERROR, "wlanProbe: glBusInit() fail\n");
			i4Status = -EIO;
			eFailReason = BUS_INIT_FAIL;
			break;
		}
		/* 4 <2> Create network device, Adapter, KalInfo, prDevHandler(netdev) */
		prWdev = wlanNetCreate(pvData);
		if (prWdev == NULL) {
			DBGLOG(INIT, ERROR, "wlanProbe: No memory for dev and its private\n");
			i4Status = -ENOMEM;
			eFailReason = NET_CREATE_FAIL;
			break;
		}
		/* 4 <2.5> Set the ioaddr to HIF Info */
		prGlueInfo = (P_GLUE_INFO_T) wiphy_priv(prWdev->wiphy);
		gPrDev = prGlueInfo->prDevHandler;

		/* prGlueInfo->main_thread = kthread_run(tx_thread, prGlueInfo->prDevHandler, "tx_thread"); */

		/* 4 <4> Setup IRQ */
		prWlandevInfo = &arWlanDevInfo[i4DevIdx];

		/* Init wakelock */
		wlanWakeLockInit(prGlueInfo);

		i4Status = glBusSetIrq(prWdev->netdev, NULL, prGlueInfo);

		if (i4Status != WLAN_STATUS_SUCCESS) {
			DBGLOG(INIT, ERROR, "wlanProbe: Set IRQ error\n");
			eFailReason = BUS_SET_IRQ_FAIL;
			break;
		}

		prGlueInfo->i4DevIdx = i4DevIdx;

		prAdapter = prGlueInfo->prAdapter;

		prGlueInfo->u4ReadyFlag = 0;

#if CFG_TCP_IP_CHKSUM_OFFLOAD
		prAdapter->u4CSUMFlags = (CSUM_OFFLOAD_EN_TX_TCP | CSUM_OFFLOAD_EN_TX_UDP | CSUM_OFFLOAD_EN_TX_IP);
#endif

		prAdapter->fgIsReadRevID = FALSE;

#if CFG_SUPPORT_CFG_FILE
		wlanCfgInit(prAdapter, NULL, 0, 0);
#ifdef ENABLED_IN_ENGUSERDEBUG
		{
			PUINT_8 pucConfigBuf;
			UINT_32 u4ConfigReadLen;

			pucConfigBuf = (PUINT_8) kalMemAlloc(WLAN_CFG_FILE_BUF_SIZE, VIR_MEM_TYPE);
			u4ConfigReadLen = 0;
			kalMemZero(pucConfigBuf, WLAN_CFG_FILE_BUF_SIZE);
			if (pucConfigBuf) {
				if (kalReadToFile("/storage/sdcard0/wifi.cfg", pucConfigBuf,
						  WLAN_CFG_FILE_BUF_SIZE, &u4ConfigReadLen) == 0)
					;
				else
					kalReadToFile("/data/misc/wifi/wifi.cfg", pucConfigBuf,
							 WLAN_CFG_FILE_BUF_SIZE, &u4ConfigReadLen);

				if (pucConfigBuf[0] != '\0' && u4ConfigReadLen > 0)
					wlanCfgInit(prAdapter, pucConfigBuf, u4ConfigReadLen, 0);
				kalMemFree(pucConfigBuf, VIR_MEM_TYPE, WLAN_CFG_FILE_BUF_SIZE);
			}	/* pucConfigBuf */
		}
#endif
#endif

		/* 4 <5> Start Device */
		/*  */
#if CFG_ENABLE_FW_DOWNLOAD
		/* before start adapter, we need to open and load firmware */
		{
			PVOID prFwBuffer = NULL;
			UINT_32 u4FwSize = 0;
			UINT_32 u4FwLoadAddr = CFG_FW_LOAD_ADDRESS;
			UINT_32 u4FwStartAddr = CFG_FW_START_ADDRESS;
			P_REG_INFO_T prRegInfo = &prGlueInfo->rRegInfo;

			kalMemSet(prRegInfo, 0, sizeof(REG_INFO_T));

#if defined(MT6631)
#ifdef CONFIG_OF
			if (prGlueInfo->rHifInfo.Dev) {
				if (of_property_read_u32_index(prGlueInfo->rHifInfo.Dev->of_node,
							       "hardware-values", 1, &u4FwLoadAddr) ||
				    of_property_read_u32_index(prGlueInfo->rHifInfo.Dev->of_node,
							       "hardware-values", 2, &u4FwStartAddr))
					DBGLOG(INIT, ERROR, "Failed to get hardware-values from DT!\n");
			}
#endif
#endif

			prRegInfo->u4LoadAddress = u4FwLoadAddr;
			prRegInfo->u4StartAddress = u4FwStartAddr;

			/* Trigger the action of switching power state to DRV_OWN */
			nicpmCheckAndTriggerDriverOwn(prAdapter);

			/* Load NVRAM content to REG_INFO_T */
			glLoadNvram(prGlueInfo, prRegInfo);

			/* kalMemCopy(&prGlueInfo->rRegInfo, prRegInfo, sizeof(REG_INFO_T)); */

			prRegInfo->u4PowerMode = CFG_INIT_POWER_SAVE_PROF;
			prRegInfo->fgEnArpFilter = TRUE;

			if (kalFirmwareImageMapping(prGlueInfo, &prFwBuffer, &u4FwSize) == NULL) {
				i4Status = -EIO;
				goto bailout;
			} else {
				if (wlanAdapterStart(prAdapter, prRegInfo, prFwBuffer, u4FwSize) != WLAN_STATUS_SUCCESS)
					i4Status = -EIO;
			}

			kalFirmwareImageUnmapping(prGlueInfo, NULL, prFwBuffer);

bailout:
			/* kfree(prRegInfo); */

			if (i4Status < 0) {
				eFailReason = ADAPTER_START_FAIL;
				break;
			}
		}
#else
		/* P_REG_INFO_T prRegInfo = (P_REG_INFO_T) kmalloc(sizeof(REG_INFO_T), GFP_KERNEL); */
		kalMemSet(&prGlueInfo->rRegInfo, 0, sizeof(REG_INFO_T));
		P_REG_INFO_T prRegInfo = &prGlueInfo->rRegInfo;

		/* Load NVRAM content to REG_INFO_T */
		glLoadNvram(prGlueInfo, prRegInfo);

		prRegInfo->u4PowerMode = CFG_INIT_POWER_SAVE_PROF;

		if (wlanAdapterStart(prAdapter, prRegInfo, NULL, 0) != WLAN_STATUS_SUCCESS) {
			i4Status = -EIO;
			eFailReason = ADAPTER_START_FAIL;
			break;
		}
#endif

		prGlueInfo->main_thread = kthread_run(tx_thread, prGlueInfo->prDevHandler, "tx_thread");
#if CFG_SUPPORT_MULTITHREAD
		prGlueInfo->hif_thread = kthread_run(hif_thread, prGlueInfo->prDevHandler, "hif_thread");
		prGlueInfo->rx_thread = kthread_run(rx_thread, prGlueInfo->prDevHandler, "rx_thread");
#endif

		/* TODO the change schedule API shall be provided by OS glue layer */
		/* Switch the Wi-Fi task priority to higher priority and change the scheduling method */
		if (prGlueInfo->prAdapter->rWifiVar.ucThreadPriority > 0) {
			struct sched_param param = {.sched_priority = prGlueInfo->prAdapter->rWifiVar.ucThreadPriority
			};
			sched_setscheduler(prGlueInfo->main_thread,
					   prGlueInfo->prAdapter->rWifiVar.ucThreadScheduling, &param);
#if CFG_SUPPORT_MULTITHREAD
			sched_setscheduler(prGlueInfo->hif_thread,
					   prGlueInfo->prAdapter->rWifiVar.ucThreadScheduling, &param);
			sched_setscheduler(prGlueInfo->rx_thread,
					   prGlueInfo->prAdapter->rWifiVar.ucThreadScheduling, &param);
#endif
			DBGLOG(INIT, INFO,
			       "Set pri = %d, sched = %d\n",
			       prGlueInfo->prAdapter->rWifiVar.ucThreadPriority,
			       prGlueInfo->prAdapter->rWifiVar.ucThreadScheduling);
		}

		if (prAdapter->fgEnable5GBand == FALSE)
			prWdev->wiphy->bands[IEEE80211_BAND_5GHZ] = NULL;
		else
			prWdev->wiphy->bands[IEEE80211_BAND_5GHZ] = &mtk_band_5ghz;

		kalSetHalted(FALSE);
		/* set MAC address */
		{
			WLAN_STATUS rStatus = WLAN_STATUS_FAILURE;
			struct sockaddr MacAddr;
			UINT_32 u4SetInfoLen = 0;

			rStatus = kalIoctl(prGlueInfo,
					   wlanoidQueryCurrentAddr,
					   &MacAddr.sa_data, PARAM_MAC_ADDR_LEN, TRUE, TRUE, TRUE, &u4SetInfoLen);

			if (rStatus != WLAN_STATUS_SUCCESS) {
				DBGLOG(INIT, WARN, "set MAC addr fail 0x%lx\n", rStatus);
				prGlueInfo->u4ReadyFlag = 0;
			} else {
				ether_addr_copy(prGlueInfo->prDevHandler->dev_addr, MacAddr.sa_data);
				ether_addr_copy(prGlueInfo->prDevHandler->perm_addr,
				       prGlueInfo->prDevHandler->dev_addr);

				/* card is ready */
				prGlueInfo->u4ReadyFlag = 1;
#if CFG_SHOW_MACADDR_SOURCE
				DBGLOG(INIT, INFO, "MAC address: " MACSTR, MAC2STR(MacAddr.sa_data));
#endif
			}
		}

#ifdef FW_CFG_SUPPORT
		{
			if (wlanFwArrayCfg(prAdapter) != WLAN_STATUS_FAILURE)
				DBGLOG(INIT, TRACE, "FW Array Cfg done!");
		}
#ifdef ENABLED_IN_ENGUSERDEBUG
		{
			if (wlanFwFileCfg(prAdapter) != WLAN_STATUS_FAILURE)
				DBGLOG(INIT, TRACE, "FW File Cfg done!");
		}
#endif
#endif
#if CFG_TCP_IP_CHKSUM_OFFLOAD
		/* set HW checksum offload */
		{
			WLAN_STATUS rStatus = WLAN_STATUS_FAILURE;
			UINT_32 u4CSUMFlags = CSUM_OFFLOAD_EN_ALL;
			UINT_32 u4SetInfoLen = 0;

			rStatus = kalIoctl(prGlueInfo,
					   wlanoidSetCSUMOffload,
					   (PVOID) &u4CSUMFlags, sizeof(UINT_32), FALSE, FALSE, TRUE, &u4SetInfoLen);

			if (rStatus != WLAN_STATUS_SUCCESS)
				DBGLOG(INIT, WARN, "set HW checksum offload fail 0x%lx\n", rStatus);
		}
#endif

		/* 4 <3> Register the card */
		i4DevIdx = wlanNetRegister(prWdev);
		if (i4DevIdx < 0) {
			i4Status = -ENXIO;
			DBGLOG(INIT, ERROR, "wlanProbe: Cannot register the net_device context to the kernel\n");
			eFailReason = NET_REGISTER_FAIL;
			break;
		}
		/* 4 <4> Register early suspend callback */
#if CFG_ENABLE_EARLY_SUSPEND
		glRegisterEarlySuspend(&wlan_early_suspend_desc, wlan_early_suspend, wlan_late_resume);
#endif

		/* 4 <5> Register Notifier callback */
		wlanRegisterNotifier();

		/* 4 <6> Initialize /proc filesystem */
#ifdef WLAN_INCLUDE_PROC
		i4Status = procCreateFsEntry(prGlueInfo);
		if (i4Status < 0) {
			DBGLOG(INIT, ERROR, "wlanProbe: init procfs failed\n");
			eFailReason = PROC_INIT_FAIL;
			break;
		}
#endif /* WLAN_INCLUDE_PROC */

#ifdef FW_CFG_SUPPORT
		i4Status = cfgCreateProcEntry(prGlueInfo);
		if (i4Status < 0) {
			DBGLOG(INIT, ERROR, "fw cfg proc failed\n");
			break;
		}
#endif /* WLAN_INCLUDE_PROC */

#if CFG_MET_PACKET_TRACE_SUPPORT
		kalMetInit(prGlueInfo);
#endif

#if CFG_ENABLE_BT_OVER_WIFI
		prGlueInfo->rBowInfo.fgIsNetRegistered = FALSE;
		prGlueInfo->rBowInfo.fgIsRegistered = FALSE;
		glRegisterAmpc(prGlueInfo);
#endif

#if (CFG_ENABLE_WIFI_DIRECT)
		register_set_p2p_mode_handler(set_p2p_mode_handler);
#endif

#if (CFG_MET_PACKET_TRACE_SUPPORT == 1)
		DBGLOG(INIT, TRACE, "init MET procfs...\n");
		i4Status = kalMetInitProcfs(prGlueInfo);
		if (i4Status < 0) {
			DBGLOG(INIT, ERROR, "wlanProbe: init MET procfs failed\n");
			eFailReason = FAIL_MET_INIT_PROCFS;
			break;
		}
#endif
	} while (FALSE);

	if (i4Status == WLAN_STATUS_SUCCESS) {
		wlanCfgSetSwCtrl(prGlueInfo->prAdapter);
		wlanCfgSetChip(prGlueInfo->prAdapter);
		wlanGetFwInfo(prGlueInfo->prAdapter);
		wlanCfgSetCountryCode(prGlueInfo->prAdapter);
		/* Init performance monitor structure */
		kalPerMonInit(prGlueInfo);
#if CFG_SUPPORT_AGPS_ASSIST
		kalIndicateAgpsNotify(prAdapter, AGPS_EVENT_WLAN_ON, NULL, 0);
#endif
		DBGLOG(INIT, TRACE, "wlanProbe success\n");
	} else {
		switch (eFailReason) {
		case FAIL_MET_INIT_PROCFS:
			kalMetRemoveProcfs();
			/* Fall through */
		case PROC_INIT_FAIL:
			wlanNetUnregister(prWdev);
			/* Fall through */
		case NET_REGISTER_FAIL:
			set_bit(GLUE_FLAG_HALT_BIT, &prGlueInfo->ulFlag);
			/* wake up main thread */
			wake_up_interruptible(&prGlueInfo->waitq);
			/* wait main thread stops */
			wait_for_completion_interruptible(&prGlueInfo->rHaltComp);
			wlanAdapterStop(prAdapter);
			/* Fall through */
		case ADAPTER_START_FAIL:
			glBusFreeIrq(prWdev->netdev, *((P_GLUE_INFO_T *) netdev_priv(prWdev->netdev)));
			/* Fall through */
		case BUS_SET_IRQ_FAIL:
			wlanWakeLockUninit(prGlueInfo);
			wlanNetDestroy(prWdev);
			break;
		case NET_CREATE_FAIL:
			/* Fall through */
		case BUS_INIT_FAIL:
			/* Fall through */
		default:
			break;
		}
		/* Set the power off flag to FALSE in WMT to prevent chip power off after
		 * wlanProbe return failure, because we need to do core dump afterward.
		 */
		if (g_IsNeedDoChipReset)
			mtk_wcn_set_connsys_power_off_flag(FALSE);

		DBGLOG(INIT, ERROR, "wlanProbe failed\n");
	}
	return i4Status;
}				/* end of wlanProbe() */

/*----------------------------------------------------------------------------*/
/*!
* \brief A method to stop driver operation and release all resources. Following
*        this call, no frame should go up or down through this interface.
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/
static VOID wlanRemove(VOID)
{
#define KAL_WLAN_REMOVE_TIMEOUT_MSEC			3000
	struct net_device *prDev = NULL;
	P_WLANDEV_INFO_T prWlandevInfo = NULL;
	P_GLUE_INFO_T prGlueInfo = NULL;
	P_ADAPTER_T prAdapter = NULL;

	DBGLOG(INIT, INFO, "Remove wlan!\n");

	/* 4 <0> Sanity check */
	ASSERT(u4WlanDevNum <= CFG_MAX_WLAN_DEVICES);
	if (u4WlanDevNum == 0) {
		DBGLOG(INIT, ERROR, "0 == u4WlanDevNum\n");
		return;
	}
#if (CFG_ENABLE_WIFI_DIRECT)
	register_set_p2p_mode_handler(NULL);
#endif

	prDev = arWlanDevInfo[u4WlanDevNum - 1].prDev;
	prWlandevInfo = &arWlanDevInfo[u4WlanDevNum - 1];

	ASSERT(prDev);
	if (prDev == NULL) {
		DBGLOG(INIT, ERROR, "NULL == prDev\n");
		return;
	}

	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prDev));
	ASSERT(prGlueInfo);
	if (prGlueInfo == NULL) {
		DBGLOG(INIT, ERROR, "NULL == prGlueInfo\n");
		free_netdev(prDev);
		return;
	}
#ifdef FW_CFG_SUPPORT
	cfgRemoveProcEntry();
#endif
#ifdef WLAN_INCLUDE_PROC
	procRemoveProcfs();
#endif /* WLAN_INCLUDE_PROC */

	kalPerMonDestroy(prGlueInfo);

	/* complete possible pending oid, which may block wlanRemove some time and then whole chip reset may failed */
	if (kalIsResetting())
		wlanReleasePendingOid(prGlueInfo->prAdapter, 1);

#if CFG_ENABLE_BT_OVER_WIFI
	if (prGlueInfo->rBowInfo.fgIsNetRegistered) {
		bowNotifyAllLinkDisconnected(prGlueInfo->prAdapter);
		/* wait 300ms for BoW module to send deauth */
		kalMsleep(300);
	}
#endif

	flush_delayed_work(&workq);

/* 20150205 work queue for sched_scan */

	flush_delayed_work(&sched_workq);

	if (kalHaltLock(KAL_WLAN_REMOVE_TIMEOUT_MSEC) == -ETIME) {
		DBGLOG(INIT, ERROR, "Halt Lock, need OidComplete.\n");
		kalOidComplete(prGlueInfo, FALSE, 0, WLAN_STATUS_NOT_ACCEPTED);
	}
	kalSetHalted(TRUE);

	/* 4 <2> Mark HALT, notify main thread to stop, and clean up queued requests */
	set_bit(GLUE_FLAG_HALT_BIT, &prGlueInfo->ulFlag);

#if CFG_SUPPORT_MULTITHREAD
	wake_up_interruptible(&prGlueInfo->waitq_hif);
	if (!wait_for_completion_timeout(&prGlueInfo->rHifHaltComp, MSEC_TO_JIFFIES(KAL_WLAN_REMOVE_TIMEOUT_MSEC))) {
		DBGLOG(INIT, ERROR, "wait hif_thread exit timeout, longer than 3s, show backtrace of hif_thread\n");
		show_stack(prGlueInfo->hif_thread, NULL);
	}
	wake_up_interruptible(&prGlueInfo->waitq_rx);
	if (!wait_for_completion_timeout(&prGlueInfo->rRxHaltComp, MSEC_TO_JIFFIES(KAL_WLAN_REMOVE_TIMEOUT_MSEC))) {
		DBGLOG(INIT, ERROR, "wait rx_thread exit timeout, longer than 3s, show backtrace of rx_thread\n");
		show_stack(prGlueInfo->rx_thread, NULL);
	}
#endif

	/* wake up main thread */
	wake_up_interruptible(&prGlueInfo->waitq);
	/* wait main thread stops */
	if (!wait_for_completion_timeout(&prGlueInfo->rHaltComp, MSEC_TO_JIFFIES(KAL_WLAN_REMOVE_TIMEOUT_MSEC))) {
		DBGLOG(INIT, ERROR, "wait tx_thread exit timeout, longer than 3s, show backtrace of tx_thread\n");
		show_stack(prGlueInfo->main_thread, NULL);
	}

	DBGLOG(INIT, TRACE, "mtk_sdiod stopped\n");

	/* prGlueInfo->rHifInfo.main_thread = NULL; */
	prGlueInfo->main_thread = NULL;
#if CFG_SUPPORT_MULTITHREAD
	prGlueInfo->hif_thread = NULL;
	prGlueInfo->rx_thread = NULL;

	prGlueInfo->u4TxThreadPid = 0xffffffff;
	prGlueInfo->u4HifThreadPid = 0xffffffff;
#endif

	/* Destroy wakelock */
	wlanWakeLockUninit(prGlueInfo);

	kalMemSet(&(prGlueInfo->prAdapter->rWlanInfo), 0, sizeof(WLAN_INFO_T));

#if CFG_ENABLE_WIFI_DIRECT
	if (prGlueInfo->prAdapter->fgIsP2PRegistered) {
		DBGLOG(INIT, TRACE, "p2pNetUnregister...\n");
		p2pNetUnregister(prGlueInfo, FALSE);
		DBGLOG(INIT, TRACE, "p2pRemove...\n");
		/*p2pRemove must before wlanAdapterStop */
		p2pRemove(prGlueInfo);
	}
#endif

#if CFG_ENABLE_BT_OVER_WIFI
	if (prGlueInfo->rBowInfo.fgIsRegistered)
		glUnregisterAmpc(prGlueInfo);
#endif

#if (CFG_MET_PACKET_TRACE_SUPPORT == 1)
	kalMetRemoveProcfs();
#endif
	/* 4 <4> wlanAdapterStop */
	prAdapter = prGlueInfo->prAdapter;
#if CFG_SUPPORT_AGPS_ASSIST
	kalIndicateAgpsNotify(prAdapter, AGPS_EVENT_WLAN_OFF, NULL, 0);
#endif

	wlanAdapterStop(prAdapter);
	DBGLOG(INIT, TRACE, "Number of Stalled Packets = %d\n", GLUE_GET_REF_CNT(prGlueInfo->i4TxPendingFrameNum));

	/* 4 <x> Stopping handling interrupt and free IRQ */
	glBusFreeIrq(prDev, prGlueInfo);

	/* 4 <5> Release the Bus */
	glBusRelease(prDev);

	kalHaltUnlock();
#if (CFG_SUPPORT_TRACE_TC4 == 1)
	wlanDebugTC4Uninit();
#endif
	/* 4 <6> Unregister the card */
	wlanNetUnregister(prDev->ieee80211_ptr);

	/* 4 <7> Destroy the device */
	wlanNetDestroy(prDev->ieee80211_ptr);
	prDev = NULL;

	/* 4 <8> Unregister early suspend callback */
#if CFG_ENABLE_EARLY_SUSPEND
	glUnregisterEarlySuspend(&wlan_early_suspend_desc);
#endif
	gprWdev->netdev = NULL;

	/* 4 <9> Unregister notifier callback */
	wlanUnregisterNotifier();

}				/* end of wlanRemove() */

/*----------------------------------------------------------------------------*/
/*!
* \brief Driver entry point when the driver is configured as a Linux Module, and
*        is called once at module load time, by the user-level modutils
*        application: insmod or modprobe.
*
* \retval 0     Success
*/
/*----------------------------------------------------------------------------*/
/* 1 Module Entry Point */
static int initWlan(void)
{
	int ret = 0;

	DBGLOG(INIT, INFO, "initWlan\n");

	wlanDebugInit();

	/* memory pre-allocation */
	kalInitIOBuffer();
	procInitFs();

	createWirelessDevice();
	if (gprWdev)
		glP2pCreateWirelessDevice((P_GLUE_INFO_T) wiphy_priv(gprWdev->wiphy));

	ret = ((glRegisterBus(wlanProbe, wlanRemove) == WLAN_STATUS_SUCCESS) ? 0 : -EIO);

	if (ret == -EIO) {
		kalUninitIOBuffer();
		return ret;
	}
#if (CFG_CHIP_RESET_SUPPORT)
	glResetInit();
#endif

	kalFbNotifierReg((P_GLUE_INFO_T) wiphy_priv(gprWdev->wiphy));
	return ret;
}				/* end of initWlan() */

/*----------------------------------------------------------------------------*/
/*!
* \brief Driver exit point when the driver as a Linux Module is removed. Called
*        at module unload time, by the user level modutils application: rmmod.
*        This is our last chance to clean up after ourselves.
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/
/* 1 Module Leave Point */
static VOID exitWlan(void)
{

	kalFbNotifierUnReg();
#if CFG_CHIP_RESET_SUPPORT
	glResetUninit();
#endif

	glUnregisterBus(wlanRemove);

	/* free pre-allocated memory */
	kalUninitIOBuffer();
	destroyWirelessDevice();
	glP2pDestroyWirelessDevice();
	procUninitProcFs();

	DBGLOG(INIT, INFO, "exitWlan\n");

}				/* end of exitWlan() */

#ifdef MTK_WCN_BUILT_IN_DRIVER

int mtk_wcn_wlan_gen3_init(void)
{
	return initWlan();
}
EXPORT_SYMBOL(mtk_wcn_wlan_gen3_init);

void mtk_wcn_wlan_gen3_exit(void)
{
	return exitWlan();
}
EXPORT_SYMBOL(mtk_wcn_wlan_gen3_exit);

#else

module_init(initWlan);
module_exit(exitWlan);

#endif

MODULE_AUTHOR(NIC_AUTHOR);
MODULE_DESCRIPTION(NIC_DESC);
MODULE_SUPPORTED_DEVICE(NIC_NAME);
MODULE_LICENSE("GPL");
