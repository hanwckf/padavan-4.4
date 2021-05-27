/*
 ***************************************************************************
 * Ralink Tech Inc.
 * 4F, No. 2 Technology 5th Rd.
 * Science-based Industrial Park
 * Hsin-chu, Taiwan, R.O.C.
 *
 * (c) Copyright 2002, Ralink Technology, Inc.
 *
 * All rights reserved. Ralink's source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of Ralink Tech. Any attemp
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************

    Module Name:
    fsm_sync.c

    Abstract:

    Revision History:
    Who         When            What
    --------    ----------      ----------------------------------------------
				2016-08-18		AP/APCLI/STA SYNC FSM Integration
*/


#include "rt_config.h"

#ifdef CONFIG_AP_SUPPORT
#ifdef DISABLE_HOSTAPD_PROBE_RESP
INT build_country_power_ie(RTMP_ADAPTER *pAd, struct wifi_dev *wdev, UCHAR *buf)
#else
static INT build_country_power_ie(RTMP_ADAPTER *pAd, struct wifi_dev *wdev, UCHAR *buf)
#endif
{
	ULONG len = 0;
	UCHAR Environment = 0x20;

	/* add Country IE and power-related IE */
	if (pAd->CommonCfg.bCountryFlag ||
		(wdev->channel > 14 && pAd->CommonCfg.bIEEE80211H == TRUE)
#ifdef DOT11K_RRM_SUPPORT
		|| IS_RRM_ENABLE(wdev)
#endif /* DOT11K_RRM_SUPPORT */
	   ) {

		ULONG TmpLen2 = 0;
		UCHAR TmpLen3 = 0;
		UCHAR TmpFrame[256] = {0};
		const UCHAR CountryIe = IE_COUNTRY;
#ifndef EXT_BUILD_CHANNEL_LIST
		PCH_DESC pChDesc = NULL;
		ULONG TmpLen = 0;
		UCHAR op_ht_bw = wlan_config_get_ht_bw(wdev);

		if (WMODE_CAP_2G(wdev->PhyMode)) {
			if (pAd->CommonCfg.pChDesc2G != NULL)
				pChDesc = (PCH_DESC)pAd->CommonCfg.pChDesc2G;
			else
				MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
						 ("%s: pChDesc2G is NULL !!!\n", __func__));
		} else if (WMODE_CAP_5G(wdev->PhyMode)) {
			if (pAd->CommonCfg.pChDesc5G != NULL)
				pChDesc = (PCH_DESC)pAd->CommonCfg.pChDesc5G;
			else
				MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
						 ("%s: pChDesc5G is NULL !!!\n", __func__));
		}
#endif
		/*
			Only APs that comply with 802.11h or 802.11k are required to include
			the Power Constraint element (IE=32) and
			the TPC Report element (IE=35) and
			the VHT Transmit Power Envelope element (IE=195)
			in beacon frames and probe response frames
		*/
		if ((wdev->channel > 14 && pAd->CommonCfg.bIEEE80211H == TRUE)
#ifdef DOT11K_RRM_SUPPORT
			|| IS_RRM_ENABLE(wdev)
#endif /* DOT11K_RRM_SUPPORT */
		   ) {
			/* prepare power constraint IE */
			MAKE_IE_TO_BUF(buf, PowerConstraintIE, 3, len);
			/* prepare TPC Report IE */
			InsertTpcReportIE(pAd, (UCHAR *)(buf + len), &len,
							  GetMaxTxPwr(pAd), 0);
#ifdef DOT11_HE_AX
			if (WMODE_CAP_AX(wdev->PhyMode)) {
				const UINT8 he_txpwr_env_ie = IE_VHT_TXPWR_ENV;
				UINT8 ie_len;
				HE_TXPWR_ENV_IE txpwr_env;

				ie_len = build_he_txpwr_envelope(wdev, (UCHAR *)&txpwr_env);

				MAKE_IE_TO_BUF(buf, &he_txpwr_env_ie, 1, len);
				MAKE_IE_TO_BUF(buf, &ie_len, 1, len);
				MAKE_IE_TO_BUF(buf, &txpwr_env, ie_len, len);
			}
#else
#ifdef DOT11_VHT_AC

			/* prepare VHT Transmit Power Envelope IE */
			if (WMODE_CAP_AC(wdev->PhyMode)) {
				const UINT8 vht_txpwr_env_ie = IE_VHT_TXPWR_ENV;
				UINT8 ie_len;
				VHT_TXPWR_ENV_IE txpwr_env;

				ie_len = build_vht_txpwr_envelope(pAd, wdev, (UCHAR *)&txpwr_env);
				MAKE_IE_TO_BUF(buf, &vht_txpwr_env_ie, 1, len);
				MAKE_IE_TO_BUF(buf, &ie_len, 1, len);
				MAKE_IE_TO_BUF(buf, &txpwr_env, ie_len, len);
			}

#endif /* DOT11_VHT_AC */
#endif
		}

		NdisZeroMemory(TmpFrame, sizeof(TmpFrame));
#ifdef EXT_BUILD_CHANNEL_LIST
		BuildBeaconChList(pAd, wdev, TmpFrame, &TmpLen2);
#else
		{
			UINT i = 0;
			UCHAR MaxTxPower = GetCuntryMaxTxPwr(pAd, wdev->PhyMode, wdev, op_ht_bw);

			/* force max power to be 30 dBm */
			MaxTxPower = 30;

			if (pChDesc) {
				for (i = 0; pChDesc[i].FirstChannel != 0; i++) {
					MakeOutgoingFrame(TmpFrame + TmpLen2, &TmpLen,
									  1, &pChDesc[i].FirstChannel,
									  1, &pChDesc[i].NumOfCh,
									  1, &MaxTxPower,
									  END_OF_ARGS);
					TmpLen2 += TmpLen;
				}
			}
		}
#endif /* EXT_BUILD_CHANNEL_LIST */
#ifdef DOT11K_RRM_SUPPORT

		if (IS_RRM_ENABLE(wdev)) {
			UCHAR reg_class = get_regulatory_class(pAd, wdev->channel, wdev->PhyMode, wdev);

			TmpLen2 = 0;
			NdisZeroMemory(TmpFrame, sizeof(TmpFrame));
			RguClass_BuildBcnChList(pAd, TmpFrame, &TmpLen2, wdev->PhyMode, reg_class);
		}

#endif /* DOT11K_RRM_SUPPORT */
#ifdef MBO_SUPPORT
					if (IS_MBO_ENABLE(wdev))
						Environment = MBO_AP_USE_GLOBAL_OPERATING_CLASS;
#endif /* MBO_SUPPORT */

		/* need to do the padding bit check, and concatenate it */
		MAKE_IE_TO_BUF(buf, &CountryIe, 1, len);

		if ((TmpLen2 % 2) == 0) {
			TmpLen3 = (UCHAR)(TmpLen2 + 4);
			TmpLen2++;
		} else
			TmpLen3 = (UCHAR)(TmpLen2 + 3);

		MAKE_IE_TO_BUF(buf, &TmpLen3, 1, len);
		MAKE_IE_TO_BUF(buf, &pAd->CommonCfg.CountryCode[0], 1, len);
		MAKE_IE_TO_BUF(buf, &pAd->CommonCfg.CountryCode[1], 1, len);
		MAKE_IE_TO_BUF(buf, &Environment, 1, len);
		MAKE_IE_TO_BUF(buf, TmpFrame, TmpLen2, len);
	} /* Country IE - */

	return len;
}

#ifdef DISABLE_HOSTAPD_PROBE_RESP
INT build_ch_switch_announcement_ie(RTMP_ADAPTER *pAd, struct wifi_dev *wdev, UCHAR *buf)
#else
static INT build_ch_switch_announcement_ie(RTMP_ADAPTER *pAd, struct wifi_dev *wdev, UCHAR *buf)
#endif
{
	INT len = 0;
	struct DOT11_H *pDot11h = NULL;

	pDot11h = wdev->pDot11_H;
	if (pDot11h == NULL)
		return 0;

	if ((wdev->channel > 14)
		&& (pAd->CommonCfg.bIEEE80211H == 1)
		&& (pDot11h->RDMode == RD_SWITCHING_MODE)) {
		UCHAR CSAIe = IE_CHANNEL_SWITCH_ANNOUNCEMENT;
		UCHAR CSALen = 3;
		UCHAR CSAMode = 1;

		MAKE_IE_TO_BUF(buf, &CSAIe, 1, len);
		MAKE_IE_TO_BUF(buf, &CSALen, 1, len);
		MAKE_IE_TO_BUF(buf, &CSAMode, 1, len);
		MAKE_IE_TO_BUF(buf, &wdev->channel, 1, len);
		MAKE_IE_TO_BUF(buf, &pDot11h->CSCount, 1, len);
#ifdef DOT11_N_SUPPORT

		if (pAd->CommonCfg.bExtChannelSwitchAnnouncement) {
			HT_EXT_CHANNEL_SWITCH_ANNOUNCEMENT_IE HtExtChannelSwitchIe = {0};

			build_ext_channel_switch_ie(pAd, &HtExtChannelSwitchIe, wdev->channel, wdev->PhyMode, wdev);
			MAKE_IE_TO_BUF(buf, &HtExtChannelSwitchIe, sizeof(HT_EXT_CHANNEL_SWITCH_ANNOUNCEMENT_IE), len);
		}

#endif /* DOT11_N_SUPPORT */
	}

	return len;
}

static BOOLEAN ap_join_peer_response_matched(struct _RTMP_ADAPTER *pAd,
		struct wifi_dev *wdev, BCN_IE_LIST *ie_list, MLME_QUEUE_ELEM *Elem)
{
	ASSERT(0);
	return FALSE;
}

static BOOLEAN ap_join_peer_response_updated(struct _RTMP_ADAPTER *pAd,
		struct wifi_dev *wdev, BCN_IE_LIST *ie_list, MLME_QUEUE_ELEM *Elem,
		NDIS_802_11_VARIABLE_IEs *pVIE, USHORT LenVIE)
{
	ASSERT(0);
	return FALSE;
}

static BOOLEAN ap_rx_peer_response_allowed(struct _RTMP_ADAPTER *pAd,
		struct wifi_dev *wdev, BCN_IE_LIST *ie_list, MLME_QUEUE_ELEM *Elem)
{
	PMAC_TABLE_ENTRY pEntry = NULL;
	UCHAR WorkChannel = Elem->Channel;
	struct freq_oper oper;

#ifdef AUTOMATION
	rx_peer_beacon_check(pAd, ie_list, Elem);
#endif /* AUTOMATION */

	if (VALID_UCAST_ENTRY_WCID(pAd, Elem->Wcid)) {
		pEntry = MacTableLookup(pAd, ie_list->Addr2); /* Found the pEntry from Peer Bcn Content */

		if (!pEntry || !pEntry->wdev)
			return FALSE;

		WorkChannel = pEntry->wdev->channel;
	}

	/* ignore BEACON not in this channel */
	if ((ie_list->Channel != WorkChannel)
#ifdef DOT11_N_SUPPORT
#ifdef DOT11N_DRAFT3
		&& (pAd->CommonCfg.bOverlapScanning == FALSE)
#endif /* DOT11N_DRAFT3 */
#endif /* DOT11_N_SUPPORT */
#ifdef RT_CFG80211_P2P_CONCURRENT_DEVICE
		&& (!RTMP_CFG80211_VIF_P2P_CLI_ON(pAd))
#endif /* RT_CFG80211_P2P_CONCURRENT_DEVICE */
#ifdef CFG80211_MULTI_STA
		&& (!RTMP_CFG80211_MULTI_STA_ON(pAd, pAd->cfg80211_ctrl.multi_sta_net_dev))
#endif /* CFG80211_MULTI_STA */
	   )
		return FALSE;

#ifdef DOT11_N_SUPPORT

	hc_radio_query_by_channel(pAd, WorkChannel, &oper);

	if (WorkChannel <= 14
		&& (oper.ht_bw == HT_BW_40)
#ifdef DOT11N_DRAFT3
		&& (pAd->CommonCfg.bOverlapScanning == FALSE)
#endif /* DOT11N_DRAFT3 */
	   ) {
		if (WorkChannel <= 14) {
#if defined(RT_CFG80211_P2P_CONCURRENT_DEVICE) || defined(CFG80211_MULTI_STA)

			if (OPSTATUS_TEST_FLAG(pAd, fOP_AP_STATUS_MEDIA_STATE_CONNECTED) &&
#ifdef RT_CFG80211_P2P_CONCURRENT_DEVICE
				RTMP_CFG80211_VIF_P2P_CLI_ON(pAd)
#else
				RTMP_CFG80211_MULTI_STA_ON(pAd, pAd->cfg80211_ctrl.multi_sta_net_dev)
#endif /* RT_CFG80211_P2P_CONCURRENT_DEVICE */
			   ) {
				if (ie_list->Channel != WorkChannel) {
					MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_INFO, ("Channel=%d is not equal as  band Channel = %d.\n",
							 ie_list->Channel, WorkChannel));
				}
			} else
#endif /* RT_CFG80211_P2P_CONCURRENT_DEVICE || CFG80211_MULTI_STA */
				if (((oper.cen_ch_1 + 2) != ie_list->Channel) &&
					((oper.cen_ch_1 - 2) != ie_list->Channel)) {
					/*
									MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%02x:%02x:%02x:%02x:%02x:%02x is a legacy BSS (%d)\n",
												Bssid[0], Bssid[1], Bssid[2], Bssid[3], Bssid[4], Bssid[5], Channel));
					*/
					return FALSE;
				}
		}
	}

#endif /* DOT11_N_SUPPORT */
	return TRUE;
}

static BOOLEAN ap_rx_peer_response_updated(struct _RTMP_ADAPTER *pAd,
		struct wifi_dev *wdev, BCN_IE_LIST *ie_list, MLME_QUEUE_ELEM *Elem,
		NDIS_802_11_VARIABLE_IEs *pVIE, USHORT LenVIE)
{
	UCHAR WorkChannel = Elem->Channel;
	PMAC_TABLE_ENTRY pEntry = NULL;
	CHAR RealRssi = 0;
	UCHAR Rates[MAX_LEN_OF_SUPPORTED_RATES] = {0}, *pRates = NULL, RatesLen = 0;
	BOOLEAN LegacyBssExist = FALSE;
	UCHAR MaxSupportedRate = 0;
	struct legacy_rate *legacy_rate = &ie_list->cmm_ies.rate;
#ifdef MWDS
	int BssIdx = 0;
	BSS_TABLE *ScanTab = get_scan_tab_by_wdev(pAd, wdev);
#endif
#ifdef OCE_SUPPORT
	P_OCE_CTRL	pOceCtrl = &wdev->OceCtrl;
	INT OceBOnlyPresentOldValue;
	ULONG Bssidx;
	BSS_TABLE *OceScanTab = get_scan_tab_by_wdev(pAd, wdev);
#endif /* OCE_SUPPORT */
	RSSI_SAMPLE rssi_sample;

	if (VALID_UCAST_ENTRY_WCID(pAd, Elem->Wcid)) {
		pEntry = MacTableLookup(pAd, ie_list->Addr2); /* Found the pEntry from Peer Bcn Content */
		if (pEntry)
			WorkChannel = pEntry->wdev->channel;
	}
		rssi_sample.AvgRssi[0] = Elem->rssi_info.raw_rssi[0];
		rssi_sample.AvgRssi[1] = Elem->rssi_info.raw_rssi[1];
		rssi_sample.AvgRssi[2] = Elem->rssi_info.raw_rssi[2];
		rssi_sample.AvgRssi[3] = Elem->rssi_info.raw_rssi[3];
		RealRssi = rtmp_avg_rssi(pAd, &rssi_sample);
#ifdef IDS_SUPPORT
	/* Conflict SSID detection */
	RTMPConflictSsidDetection(pAd, (PUCHAR)ie_list->Ssid, ie_list->SsidLen,
							  (CHAR)Elem->rssi_info.raw_rssi[0],
							  (CHAR)Elem->rssi_info.raw_rssi[1],
							  (CHAR)Elem->rssi_info.raw_rssi[2]);
#endif /* IDS_SUPPORT */
#ifdef DOT11_N_SUPPORT

	/* 40Mhz BSS Width Trigger events Intolerant devices */
	if ((RealRssi > OBSS_BEACON_RSSI_THRESHOLD) &&
		(ie_list->cmm_ies.ht_cap.HtCapInfo.Forty_Mhz_Intolerant)) /* || (HtCapabilityLen == 0))) */
		Handle_BSS_Width_Trigger_Events(pAd, WorkChannel);

#endif /* DOT11_N_SUPPORT */
	pRates = (PUCHAR)Rates;
	SupportRate(legacy_rate, &pRates, &RatesLen, &MaxSupportedRate);

	if ((ie_list->Erp & 0x01) || (RatesLen <= 4))
		LegacyBssExist = TRUE;
	else
		LegacyBssExist = FALSE;

#ifdef OCE_SUPPORT
	if (IS_OCE_ENABLE(wdev) && !pOceCtrl->Scan11bOceAPTimerRunning) {
		if (WMODE_CAP_2G(wdev->PhyMode) && RatesLen <= 4) {
			OceBOnlyPresentOldValue = OCE_GET_CONTROL_FIELD(pOceCtrl->OceCapIndication,
			OCE_11B_ONLY_PRESENT_MASK, OCE_11B_ONLY_PRESENT_OFFSET);
			OCE_SET_CONTROL_FIELD(pOceCtrl->OceCapIndication,
			1, OCE_11B_ONLY_PRESENT_MASK, OCE_11B_ONLY_PRESENT_OFFSET);

			if (OceBOnlyPresentOldValue != OCE_GET_CONTROL_FIELD(pOceCtrl->OceCapIndication,
				OCE_11B_ONLY_PRESENT_MASK, OCE_11B_ONLY_PRESENT_OFFSET))
				OceSendFilsDiscoveryAction(pAd, wdev);

			ie_list->is_11bonly_ap = TRUE;
		} else
			ie_list->is_11bonly_ap = FALSE;

		RTMPSetTimer(&pOceCtrl->Scan11bOceAPTimer, OCE_SCAN_11BOCEAP_PERIOD_TIME);
		pOceCtrl->Scan11bOceAPTimerRunning = TRUE;

		/* Update ScanTab */
		Bssidx = BssTableSearch(OceScanTab, ie_list->Bssid, ie_list->Channel);
		if (Bssidx == BSS_NOT_FOUND) {
			/* discover new AP of this network, create BSS entry */
			Bssidx = BssTableSetEntry(pAd, wdev, OceScanTab, ie_list, RealRssi, LenVIE, pVIE);
			if (Bssidx != BSS_NOT_FOUND) {/* check if BSS table full */

				NdisMoveMemory(OceScanTab->BssEntry[Bssidx].PTSF, &Elem->Msg[24], 4);
				NdisMoveMemory(&OceScanTab->BssEntry[Bssidx].TTSF[0], &Elem->TimeStamp.u.LowPart, 4);
				NdisMoveMemory(&OceScanTab->BssEntry[Bssidx].TTSF[4], &Elem->TimeStamp.u.LowPart, 4);
				OceScanTab->BssEntry[Bssidx].MinSNR = Elem->Signal % 10;

				if (OceScanTab->BssEntry[Bssidx].MinSNR == 0)
					OceScanTab->BssEntry[Bssidx].MinSNR = -5;

				NdisMoveMemory(OceScanTab->BssEntry[Bssidx].MacAddr, ie_list->Addr2, MAC_ADDR_LEN);
			}
		}
	}
#endif /* OCE_SUPPORT */

	if (LegacyBssExist && pAd->CommonCfg.DisableOLBCDetect == 0) {
		pAd->ApCfg.LastOLBCDetectTime = pAd->Mlme.Now32;
	}

#ifdef DOT11_N_SUPPORT

    /* note: rssi check removed for certification purpose */
	/*if (!HAS_HT_CAPS_EXIST(ie_list->cmm_ies.ie_exists) && (RealRssi > OBSS_BEACON_RSSI_THRESHOLD)) { */
	if (!HAS_HT_CAPS_EXIST(ie_list->cmm_ies.ie_exists)) {
		pAd->ApCfg.LastNoneHTOLBCDetectTime = pAd->Mlme.Now32;
	}

#endif /* DOT11_N_SUPPORT */

#ifdef P2P_SUPPORT

		if (P2P_CLI_ON(pAd) &&
			(ApCliWaitProbRsp(pAd, 0) == TRUE) &&
			(NdisEqualMemory(pAd->StaCfg[0].CfgApCliBssid, ie_list->Bssid, MAC_ADDR_LEN)))
			MlmeEnqueue(pAd, APCLI_SYNC_STATE_MACHINE, APCLI_MT2_PEER_BEACON, Elem->MsgLen, Elem->Msg, 0);
		else {
			PeerP2pBeacon(pAd, ie_list->Addr2, Elem, ie_list->TimeStamp);

			if (ie_list->MessageToMe)
				MiniportMMRequest(pAd, 0, (PUCHAR)&pAd->StaCfg[0].PsPollFrame, sizeof(PSPOLL_FRAME));
		}

#endif /* P2P_SUPPORT */

#ifdef WDS_SUPPORT

	if (pAd->WdsTab.Mode[HcGetBandByWdev(wdev)] != WDS_DISABLE_MODE) {
		if (pAd->WdsTab.flg_wds_init[HcGetBandByWdev(wdev)]) {
			MAC_TABLE_ENTRY *pEntry;
			/* check BEACON does in WDS TABLE. */
			pEntry = WdsTableLookup(pAd, ie_list->Addr2, FALSE);

			if (pEntry && WDS_IF_UP_CHECK(pAd, HcGetBandByWdev(wdev), pEntry->func_tb_idx))
				WdsPeerBeaconProc(pAd, pEntry, MaxSupportedRate, RatesLen, ie_list);
			else {
				if (!pEntry)
					MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_INFO,
						 ("%s(), WdsTab for %02x:%02x:%02x:%02x:%02x:%02x not found!\n",
						  __func__, PRINT_MAC(ie_list->Addr2)));
				else {
					MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_INFO,
						 ("%s(), WdsTab[%d] is not enabled!\n",
						  __func__, pEntry->func_tb_idx));
				}
			}
		} else {
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_INFO, ("%s(), ERROR!! Beacon comes before wds_init\n", __func__));
		}
	}

#endif /* WDS_SUPPORT */
	bss_coex_insert_effected_ch_list(pAd, WorkChannel, ie_list, wdev);

#ifdef MWDS
		BssIdx = BssTableSearch(ScanTab, ie_list->Bssid, ie_list->Channel);
		if (BssIdx != BSS_NOT_FOUND) {
			ScanTab->BssEntry[BssIdx].bSupportMWDS = FALSE;
			if (ie_list->vendor_ie.mtk_cap_found) {
				BOOLEAN bSupportMWDS = FALSE;

				if (ie_list->vendor_ie.support_mwds)
					bSupportMWDS = TRUE;

				if (ScanTab->BssEntry[BssIdx].bSupportMWDS != bSupportMWDS)
					ScanTab->BssEntry[BssIdx].bSupportMWDS = bSupportMWDS;
			}
		}
#endif /* MWDS */

	if (pAd->ApCfg.AutoChannel_Channel) {
		UCHAR BandIdx = HcGetBandByWdev(wdev);
		AUTO_CH_CTRL *pAutoChCtrl = HcGetAutoChCtrlbyBandIdx(pAd, BandIdx);

		if (pAutoChCtrl->pChannelInfo &&
			AutoChBssSearchWithSSID(pAd, ie_list->Bssid, (PUCHAR)ie_list->Ssid, ie_list->SsidLen, ie_list->Channel, wdev) == BSS_NOT_FOUND)
			pAutoChCtrl->pChannelInfo->ApCnt[pAd->ApCfg.current_channel_index]++;

		AutoChBssInsertEntry(pAd, ie_list->Bssid, ie_list->Ssid, ie_list->SsidLen, ie_list->Channel, ie_list->NewExtChannelOffset, RealRssi, wdev);
	}

	return TRUE;
}

static BOOLEAN ap_probe_response_allowed(struct _RTMP_ADAPTER *pAd,
		struct wifi_dev *wdev, struct _PEER_PROBE_REQ_PARAM *ProbeReqParam, MLME_QUEUE_ELEM *Elem)
{
	BOOLEAN isNeedRsp = TRUE;
	BSS_STRUCT *mbss;
	struct freq_oper oper;
	BOOLEAN isSsidMatched = FALSE;
#ifdef OCE_SUPPORT
		P_OCE_CTRL pOceCtrl = NULL;
#endif
#ifdef WSC_AP_SUPPORT
	UCHAR Addr3[MAC_ADDR_LEN] = {0};
	PFRAME_802_11 pFrame = (PFRAME_802_11)Elem->Msg;

	COPY_MAC_ADDR(Addr3, pFrame->Hdr.Addr3);
#endif /* WSC_AP_SUPPORT */
	mbss = wdev->func_dev;

#ifdef OCE_SUPPORT
	pOceCtrl = &wdev->OceCtrl;

	if (ProbeReqParam->IsOceCapability && ProbeReqParam->MaxChannelTime && (pOceCtrl->MaxChannelTimesUp ||
		pAd->CommonCfg.BeaconPeriod <= ProbeReqParam->MaxChannelTime)) {
		pOceCtrl->MaxChannelTimesUp = FALSE;
		pOceCtrl->MaxChannelTimerRunning = FALSE;
		return FALSE;
	}
	pOceCtrl->MaxChannelTimesUp = FALSE;
	pOceCtrl->MaxChannelTimerRunning = FALSE;

	if (ProbeReqParam->IsOceCapability && ProbeReqParam->bProbeSupp[wdev->func_idx]) {
		ProbeReqParam->bProbeSupp[wdev->func_idx] = FALSE;
		return FALSE;
	}
#endif /* OCE_SUPPORT */

	if ((wdev->if_dev == NULL) ||
		((wdev->if_dev != NULL) && !(RTMP_OS_NETDEV_STATE_RUNNING(wdev->if_dev)))) {
		/* the interface is down, so we can not send probe response */
		return FALSE;
	}

	hc_radio_query_by_channel(pAd, wdev->channel, &oper);
	if ((Elem->Channel != oper.prim_ch) && (Elem->Channel != oper.cen_ch_1))
		return FALSE;


	/* probe with SSID */
	if (ProbeReqParam->SsidLen != 0) {
		if ((ProbeReqParam->SsidLen == mbss->SsidLen) &&
			NdisEqualMemory(ProbeReqParam->Ssid, mbss->Ssid, (ULONG) ProbeReqParam->SsidLen))
			isSsidMatched = TRUE;

#ifdef DOT11V_MBSSID_SUPPORT
		/* check SSID of Non-transmitted-BSSID */
		if (!isSsidMatched && IS_MBSSID_IE_NEEDED(pAd, mbss, HcGetBandByWdev(wdev))) {
			UCHAR apidx = 0;
			BSS_STRUCT *nonTxMbss;
			UCHAR DbdcIdx;

			for (apidx = 0; apidx < pAd->ApCfg.BssidNum; apidx++) {
				nonTxMbss = &pAd->ApCfg.MBSSID[apidx];
				DbdcIdx = HcGetBandByWdev(&nonTxMbss->wdev);

				if (IS_BSSID_11V_NON_TRANS(pAd, nonTxMbss, DbdcIdx) &&
					(DbdcIdx == HcGetBandByWdev(wdev))) {
					if ((ProbeReqParam->SsidLen == nonTxMbss->SsidLen) &&
						NdisEqualMemory(ProbeReqParam->Ssid, nonTxMbss->Ssid, (ULONG)ProbeReqParam->SsidLen)) {
						isSsidMatched = TRUE;
						MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_INFO,
								("%s(): NonTx SSID found, apidx=%d\n", __func__, apidx));
						break;
					}
				}
			}
		}
#endif
	}

	if (((((ProbeReqParam->SsidLen == 0) && (!mbss->bHideSsid)) || isSsidMatched)
#ifdef CONFIG_HOTSPOT
		 && ProbeReqforHSAP(pAd, wdev->func_idx, ProbeReqParam)
#endif /* CONFIG_HOTSPOT */
		)
#ifdef WSC_AP_SUPPORT
		/* buffalo WPS testbed STA send ProbrRequest ssid length = 32 and ssid are not AP , but DA are AP. for WPS test send ProbeResponse */
		|| ((ProbeReqParam->SsidLen == 32) && MAC_ADDR_EQUAL(Addr3, wdev->bssid) && (mbss->bHideSsid == 0))
#endif /* WSC_AP_SUPPORT */
#ifdef P2P_SUPPORT
		|| (NdisEqualMemory(ProbeReqParam.Ssid, &WILDP2PSSID[0], WILDP2PSSIDLEN))
#endif /* P2P_SUPPORT */
	   )
		;
	else {
		return FALSE; /* check next BSS */
	}
#ifdef BAND_STEERING
		if (pAd->ApCfg.BandSteering
		) {
			BOOLEAN bBndStrgCheck = TRUE;

			bBndStrgCheck = BndStrg_CheckConnectionReq(pAd, wdev, ProbeReqParam->Addr2, Elem, ProbeReqParam);
			if (bBndStrgCheck == FALSE)
				return FALSE;
		}
#endif /* BAND_STEERING */

	return isNeedRsp;
}

static BOOLEAN ap_probe_response_xmit(struct _RTMP_ADAPTER *pAd,
									  struct wifi_dev *wdev, struct _PEER_PROBE_REQ_PARAM *ProbeReqParam, MLME_QUEUE_ELEM *Elem)
{
	HEADER_802_11 ProbeRspHdr;
	NDIS_STATUS NStatus;
	PUCHAR pOutBuffer = NULL;
	ULONG FrameLen = 0, TmpLen;
	struct legacy_rate *rate;
#if defined(TXBF_SUPPORT) && defined(VHT_TXBF_SUPPORT)
	UCHAR ucETxBfCap;
#endif /* TXBF_SUPPORT && VHT_TXBF_SUPPORT */
#ifdef AP_QLOAD_SUPPORT
	QLOAD_CTRL *pQloadCtrl = HcGetQloadCtrl(pAd);
#endif /* AP_QLOAD_SUPPORT */
	ADD_HT_INFO_IE *addht;
	UCHAR cfg_ht_bw;
	UCHAR op_ht_bw;
	LARGE_INTEGER FakeTimestamp;
	UCHAR DsLen = 1;
	UCHAR ErpIeLen = 1;
	USHORT PhyMode;
	BSS_STRUCT *mbss;
	struct _build_ie_info ie_info = {0};
#ifdef CUSTOMER_VENDOR_IE_SUPPORT
	struct customer_vendor_ie *ap_vendor_ie;
	CUSTOMER_PROBE_RSP_VENDOR_IE *ap_probe_rsp_vendor_ie = NULL;
	PDL_LIST ap_probe_rsp_vendor_ie_list = NULL;
	UINT32 ie_count;
	BOOLEAN found = FALSE;
#endif /* CUSTOMER_VENDOR_IE_SUPPORT */

	mbss = wdev->func_dev;
	rate = &wdev->rate.legacy_rate;
	addht = wlan_operate_get_addht(wdev);
	cfg_ht_bw = wlan_config_get_ht_bw(wdev);
	op_ht_bw = wlan_config_get_ht_bw(wdev);
	PhyMode = wdev->PhyMode;
	ie_info.frame_subtype = SUBTYPE_PROBE_RSP;
	ie_info.channel = wdev->channel;
	ie_info.phy_mode = PhyMode;
	ie_info.wdev = wdev;

#ifdef WAPP_SUPPORT
	wapp_send_cli_probe_event(pAd, RtmpOsGetNetIfIndex(wdev->if_dev), ProbeReqParam->Addr2, Elem);
#endif
#ifdef WH_EVENT_NOTIFIER
	{
		EventHdlr pEventHdlrHook = NULL;

		pEventHdlrHook = GetEventNotiferHook(WHC_DRVEVNT_STA_PROBE_REQ);

		if (pEventHdlrHook && wdev)
			pEventHdlrHook(pAd, wdev, &ProbeReqParam, Elem);
	}
#endif /* WH_EVENT_NOTIFIER */
#ifdef CUSTOMER_VENDOR_IE_SUPPORT
	if ((ProbeReqParam->report_param.vendor_ie.element_id == IE_VENDOR_SPECIFIC) &&
		(ProbeReqParam->report_param.vendor_ie.len > 0)) {
		struct probe_req_report pProbeReqReportTemp;
		memset(&pProbeReqReportTemp, 0, sizeof(struct probe_req_report));
		pProbeReqReportTemp.band = (WMODE_CAP_2G(wdev->PhyMode) && wdev->channel <= 14) ? 0 : 1;
		COPY_MAC_ADDR(pProbeReqReportTemp.sta_mac, ProbeReqParam->Addr2);
		pProbeReqReportTemp.vendor_ie.element_id = ProbeReqParam->report_param.vendor_ie.element_id;
		pProbeReqReportTemp.vendor_ie.len = ProbeReqParam->report_param.vendor_ie.len;
		NdisMoveMemory(pProbeReqReportTemp.vendor_ie.custom_ie,
				ProbeReqParam->report_param.vendor_ie.custom_ie,
				ProbeReqParam->report_param.vendor_ie.len);
		RtmpOSWrielessEventSend(wdev->if_dev, RT_WLAN_EVENT_CUSTOM, RT_PROBE_REQ_REPORT_EVENT,
					NULL, (PUCHAR)&pProbeReqReportTemp,
					MAC_ADDR_LEN + 3 + ProbeReqParam->report_param.vendor_ie.len);
	}
#endif

	/* allocate and send out ProbeRsp frame */
	NStatus = MlmeAllocateMemory(pAd, &pOutBuffer);

	if (NStatus != NDIS_STATUS_SUCCESS)
		return FALSE;

#ifdef OCE_SUPPORT
	if (IS_OCE_ENABLE(wdev)
		&& MAC_ADDR_EQUAL(ProbeReqParam->Addr1, BROADCAST_ADDR)
		&& ProbeReqParam->IsOceCapability) /* broadcast probe request && is OCE STA*/
		MgtMacHeaderInit(pAd, &ProbeRspHdr, SUBTYPE_PROBE_RSP, 0, BROADCAST_ADDR,
						 wdev->if_addr, wdev->bssid); /* broadcast probe response */
	else
#endif /* OCE_SUPPORT */
	MgtMacHeaderInit(pAd, &ProbeRspHdr, SUBTYPE_PROBE_RSP, 0, ProbeReqParam->Addr2,
						 wdev->if_addr, wdev->bssid);



	MakeOutgoingFrame(pOutBuffer,				  &FrameLen,
					  sizeof(HEADER_802_11),	  &ProbeRspHdr,
					  TIMESTAMP_LEN,			  &FakeTimestamp,
					  2,						  &pAd->CommonCfg.BeaconPeriod,
					  2,						  &mbss->CapabilityInfo,
					  1,						  &SsidIe,
					  1,						  &mbss->SsidLen,
					  mbss->SsidLen,	 mbss->Ssid,
					  END_OF_ARGS);
	FrameLen += build_support_rate_ie(wdev, rate->sup_rate, rate->sup_rate_len, pOutBuffer + FrameLen);
	MakeOutgoingFrame(pOutBuffer + FrameLen,		&TmpLen,
					  1,						  &DsIe,
					  1,						  &DsLen,
					  1,						  &wdev->channel,
					  END_OF_ARGS);
	FrameLen += TmpLen;

	MakeOutgoingFrame(pOutBuffer + FrameLen,	&TmpLen,
					  1,						&ErpIe,
					  1,						&ErpIeLen,
					  1,						&pAd->ApCfg.ErpIeContent,
					  END_OF_ARGS);
	FrameLen += TmpLen;
	FrameLen += build_support_ext_rate_ie(wdev, rate->sup_rate_len,
			rate->ext_rate, rate->ext_rate_len, pOutBuffer + FrameLen);
#ifdef CONFIG_HOTSPOT_R2
	if ((mbss->HotSpotCtrl.HotSpotEnable == 0) && (mbss->HotSpotCtrl.bASANEnable == 1) && (IS_AKM_WPA2_Entry(wdev))) {
		/* replace RSN IE with OSEN IE if it's OSEN wdev */
		UCHAR RSNIe = IE_WPA;
		extern UCHAR			OSEN_IE[];
		extern UCHAR			OSEN_IELEN;

		MakeOutgoingFrame(pOutBuffer + FrameLen,			&TmpLen,
						  1,							&RSNIe,
						  1,							&OSEN_IELEN,
						  OSEN_IELEN,					OSEN_IE,
						  END_OF_ARGS);
		FrameLen += TmpLen;
	} else
#endif /* CONFIG_HOTSPOT_R2 */
		FrameLen += build_rsn_ie(pAd, wdev, (UCHAR *)(pOutBuffer + FrameLen));

#ifdef DOT11V_MBSSID_SUPPORT
	make_multiple_bssid_ie(pAd, wdev, &FrameLen, pOutBuffer,
				pAd->ApCfg.dot11v_mbssid_bitmap[HcGetBandByWdev(wdev)], TRUE);
#endif /* DOT11V_MBSSID_SUPPORT */

#ifdef DOT11_N_SUPPORT

	if (WMODE_CAP_N(PhyMode) &&
		(wdev->DesiredHtPhyInfo.bHtEnable)) {
		ie_info.frame_buf = (UCHAR *)(pOutBuffer + FrameLen);
		FrameLen += build_ht_ies(pAd, &ie_info);
	}

#endif /* DOT11_N_SUPPORT */
	ie_info.frame_buf = (UCHAR *)(pOutBuffer + FrameLen);
	FrameLen += build_extended_cap_ie(pAd, &ie_info);
#ifdef AP_QLOAD_SUPPORT
	if (pAd->CommonCfg.dbdc_mode == 0)
		pQloadCtrl = HcGetQloadCtrl(pAd);
	else
		pQloadCtrl = (wdev->channel > 14) ? HcGetQloadCtrlByRf(pAd, RFIC_5GHZ) : HcGetQloadCtrlByRf(pAd, RFIC_24GHZ);

	if (pQloadCtrl->FlgQloadEnable != 0) {
#ifdef CONFIG_HOTSPOT_R2

		if (mbss->HotSpotCtrl.QLoadTestEnable == 1)
			FrameLen += QBSS_LoadElementAppend_HSTEST(pAd, pOutBuffer + FrameLen, wdev->func_idx);
		else if (mbss->HotSpotCtrl.QLoadTestEnable == 0)
#endif /* CONFIG_HOTSPOT_R2 */
			FrameLen += QBSS_LoadElementAppend(pAd, pOutBuffer + FrameLen, pQloadCtrl, wdev->func_idx);
	}

#endif /* AP_QLOAD_SUPPORT */
#if defined(CONFIG_HOTSPOT) || defined(FTM_SUPPORT)
	if (mbss->HotSpotCtrl.HotSpotEnable)
		MakeHotSpotIE(wdev, &FrameLen, pOutBuffer);

#endif /* defined(CONFIG_HOTSPOT) || defined(FTM_SUPPORT) */
#ifdef CONFIG_DOT11U_INTERWORKING
	if (mbss->GASCtrl.b11U_enable) {
		ULONG TmpLen;
		/* Interworking element */
		MakeOutgoingFrame(pOutBuffer + FrameLen, &TmpLen,
						  mbss->GASCtrl.InterWorkingIELen,
						  mbss->GASCtrl.InterWorkingIE, END_OF_ARGS);
		FrameLen += TmpLen;
		/* Advertisement Protocol element */
		MakeOutgoingFrame(pOutBuffer + FrameLen, &TmpLen,
						  mbss->GASCtrl.AdvertisementProtoIELen,
						  mbss->GASCtrl.AdvertisementProtoIE, END_OF_ARGS);
		FrameLen += TmpLen;
	}
#endif /* CONFIG_DOT11U_INTERWORKING */

	/* add WMM IE here */
	ie_info.frame_buf = (UCHAR *)(pOutBuffer + FrameLen);
	FrameLen += build_wmm_cap_ie(pAd, &ie_info);
#ifdef DOT11K_RRM_SUPPORT

	if (IS_RRM_ENABLE(wdev)) {
		RRM_InsertRRMEnCapIE(pAd, wdev, pOutBuffer + FrameLen, &FrameLen, wdev->func_idx);
		InsertChannelRepIE(pAd, pOutBuffer + FrameLen, &FrameLen,
					   (RTMP_STRING *)pAd->CommonCfg.CountryCode,
					   get_regulatory_class(pAd, mbss->wdev.channel, mbss->wdev.PhyMode, &mbss->wdev),
					   NULL, PhyMode, wdev->func_idx);
#ifndef APPLE_11K_IOT
		/* Insert BSS AC Access Delay IE. */
		RRM_InsertBssACDelayIE(pAd, pOutBuffer + FrameLen, &FrameLen);
		/* Insert BSS Available Access Capacity IE. */
		RRM_InsertBssAvailableACIE(pAd, pOutBuffer + FrameLen, &FrameLen);
#endif /* !APPLE_11K_IOT */
	}
#endif /* DOT11K_RRM_SUPPORT */
	FrameLen += build_country_power_ie(pAd, wdev, (UCHAR *)(pOutBuffer + FrameLen));
	/* add Channel switch announcement IE */
	FrameLen += build_ch_switch_announcement_ie(pAd, wdev, (UCHAR *)(pOutBuffer + FrameLen));
#ifdef DOT11_N_SUPPORT

	if (WMODE_CAP_N(PhyMode) &&
		(wdev->DesiredHtPhyInfo.bHtEnable)) {
		if (pAd->bBroadComHT == TRUE) {
			ie_info.is_draft_n_type = TRUE;
			ie_info.frame_buf = (UCHAR *)(pOutBuffer + FrameLen);
			FrameLen += build_ht_ies(pAd, &ie_info);
		}
#ifdef DOT11_VHT_AC
#if defined(TXBF_SUPPORT) && defined(VHT_TXBF_SUPPORT)
		ucETxBfCap = wlan_config_get_etxbf(wdev);

		if (HcIsBfCapSupport(wdev) == FALSE)
			wlan_config_set_etxbf(wdev, SUBF_OFF);
		txbf_bfee_cap_set(TRUE, ProbeReqParam->bfer_cap_su, ProbeReqParam->num_snd_dimension);

#endif /* TXBF_SUPPORT && VHT_TXBF_SUPPORT */
		ie_info.frame_buf = (UCHAR *)(pOutBuffer + FrameLen);
		FrameLen += build_vht_ies(pAd, &ie_info);
#if defined(TXBF_SUPPORT) && defined(VHT_TXBF_SUPPORT)
		wlan_config_set_etxbf(wdev, ucETxBfCap);
#endif /* TXBF_SUPPORT && VHT_TXBF_SUPPORT */
#ifdef DOT11_HE_AX
		if (WMODE_CAP_AX(wdev->PhyMode) && wdev->DesiredHtPhyInfo.bHtEnable)
			FrameLen += add_probe_rsp_he_ies(wdev, (UINT8 *)pOutBuffer, FrameLen);
#endif /*DOT11_HE_AX*/
#endif /* DOT11_VHT_AC */
	}

#endif /* DOT11_N_SUPPORT */
	ie_info.frame_buf = (UCHAR *)(pOutBuffer + FrameLen);
	FrameLen += build_wsc_ie(pAd, &ie_info);
#ifdef DOT11R_FT_SUPPORT

	/*
	   The Mobility Domain information element (MDIE) is present in Probe-
	   Request frame when dot11FastBssTransitionEnable is set to true.
	  */
	if (wdev->FtCfg.FtCapFlag.Dot11rFtEnable) {
		PFT_CFG pFtCfg = &wdev->FtCfg;
		FT_CAP_AND_POLICY FtCap;

		FtCap.field.FtOverDs = pFtCfg->FtCapFlag.FtOverDs;
		FtCap.field.RsrReqCap = pFtCfg->FtCapFlag.RsrReqCap;
		FT_InsertMdIE(pAd, pOutBuffer + FrameLen, &FrameLen,
					  pFtCfg->FtMdId, FtCap);
	}

#endif /* DOT11R_FT_SUPPORT */

#ifdef CONFIG_MAP_SUPPORT
#if defined(WAPP_SUPPORT)
	if (IS_MAP_ENABLE(pAd) && wdev->MAPCfg.vendor_ie_len) {
		ULONG MAPIeTmpLen = 0;

		MakeOutgoingFrame(pOutBuffer + FrameLen, &MAPIeTmpLen,
				wdev->MAPCfg.vendor_ie_len, wdev->MAPCfg.vendor_ie_buf,
				END_OF_ARGS);
		FrameLen += MAPIeTmpLen;
	}
#endif /*WAPP_SUPPORT*/
#endif /* CONFIG_MAP_SUPPORT */

	/*
		add Ralink-specific IE here - Byte0.b0=1 for aggregation,
		Byte0.b1=1 for piggy-back, Byte0.b3=1 for rssi-feedback
	*/
	FrameLen += build_vendor_ie(pAd, wdev, (pOutBuffer + FrameLen), VIE_PROBE_RESP
	);
#ifdef CUSTOMER_VENDOR_IE_SUPPORT
	{	
	RTMP_SPIN_LOCK(&mbss->probe_rsp_vendor_ie_lock);
	ap_probe_rsp_vendor_ie_list = &mbss->ap_probe_rsp_vendor_ie_list;
	ie_count = DlListLen(ap_probe_rsp_vendor_ie_list);
	if (ie_count) {
		DlListForEach(ap_probe_rsp_vendor_ie, ap_probe_rsp_vendor_ie_list, 
			CUSTOMER_PROBE_RSP_VENDOR_IE, List) {
			if (memcmp(ap_probe_rsp_vendor_ie->stamac, ProbeReqParam->Addr2, MAC_ADDR_LEN) == 0) {
				found = TRUE;
				break;
			}
		}

		if (found && (ap_probe_rsp_vendor_ie->pointer != NULL)) {
			ULONG TmpLen;
			MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
			("SYNC - Send Probe response to %02x:%02x:%02x:%02x:%02x:%02x...and add vendor ie2\n",
			PRINT_MAC(ProbeReqParam->Addr2)));
			hex_dump_with_lvl("send Probe rsp IE: ",ap_probe_rsp_vendor_ie->pointer ,
				ap_probe_rsp_vendor_ie->length ,DBG_LVL_INFO);
			MakeOutgoingFrame(pOutBuffer + FrameLen,
					&TmpLen,
					ap_probe_rsp_vendor_ie->length,
					ap_probe_rsp_vendor_ie->pointer,
					END_OF_ARGS);
			FrameLen += TmpLen;
		}
	}	
	RTMP_SPIN_UNLOCK(&mbss->probe_rsp_vendor_ie_lock);
	}
#endif /*CUSTOMER_VENDOR_IE_SUPPORT*/

#if defined(MBO_SUPPORT)
#if defined(OCE_SUPPORT)
	if (IS_MBO_ENABLE(wdev) || IS_OCE_ENABLE(wdev))
		MakeMboOceIE(pAd, wdev, NULL, pOutBuffer+FrameLen, &FrameLen, MBO_FRAME_TYPE_PROBE_RSP);
#endif /* OCE_SUPPORT */
#endif /* MBO_SUPPORT */
	{
		/* Question to Rorscha: bit4 in old chip is used? but currently is using for 2.4G 256QAM */
#ifdef RSSI_FEEDBACK
		UCHAR RalinkSpecificIe[9] = {IE_VENDOR_SPECIFIC, 7, 0x00, 0x0c, 0x43, 0x00, 0x00, 0x00, 0x00};
		ULONG TmpLen;

		if (ProbeReqParam->bRequestRssi == TRUE) {
			MAC_TABLE_ENTRY *pEntry = NULL;

			MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("SYNC - Send PROBE_RSP to %02x:%02x:%02x:%02x:%02x:%02x...\n",
					 PRINT_MAC(ProbeReqParam->Addr2)));
			RalinkSpecificIe[5] |= 0x8;
			pEntry = MacTableLookup(pAd, ProbeReqParam->Addr2);

			if (pEntry != NULL) {
				RalinkSpecificIe[6] = (UCHAR)pEntry->RssiSample.AvgRssi[0];
				RalinkSpecificIe[7] = (UCHAR)pEntry->RssiSample.AvgRssi[1];
				RalinkSpecificIe[8] = (UCHAR)pEntry->RssiSample.AvgRssi[2];
			}
		}

		MakeOutgoingFrame(pOutBuffer + FrameLen, &TmpLen,
						  9, RalinkSpecificIe,
						  END_OF_ARGS);
		FrameLen += TmpLen;
#endif /* RSSI_FEEDBACK */
	}

#ifdef IGMP_TVM_SUPPORT
		/* Add TV IE to this packet */
		MakeTVMIE(pAd, wdev, pOutBuffer, &FrameLen);
#endif /* IGMP_TVM_SUPPORT */



#ifdef OCE_FILS_SUPPORT
	ie_info.frame_buf = (UCHAR *)(pOutBuffer + FrameLen);
	FrameLen += oce_build_ies(pAd, &ie_info, (MAC_ADDR_EQUAL(ProbeReqParam->Addr1, BROADCAST_ADDR) && ProbeReqParam->IsOceCapability));
#endif /*OCE_FILS_SUPPORT */

	FrameLen +=  build_rsnxe_ie(&wdev->SecConfig,
				    (UCHAR *)pOutBuffer + FrameLen);

#ifdef CUSTOMER_VENDOR_IE_SUPPORT
	{
		MAC_TABLE_ENTRY *pEntry = NULL;

		pEntry = MacTableLookup(pAd, ProbeReqParam->Addr2);
		if (pEntry != NULL) {
			ap_vendor_ie = &pAd->ApCfg.MBSSID[pEntry->apidx].ap_vendor_ie;
			RTMP_SPIN_LOCK(&ap_vendor_ie->vendor_ie_lock);
			if (ap_vendor_ie->pointer != NULL) {
				ULONG TmpLen;

				MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
					("SYNC - Send Probe response to %02x:%02x:%02x:%02x:%02x:%02x...and add vendor ie\n",
					PRINT_MAC(ProbeReqParam->Addr2)));
				MakeOutgoingFrame(pOutBuffer + FrameLen,
						&TmpLen,
						ap_vendor_ie->length,
						ap_vendor_ie->pointer,
						END_OF_ARGS);
				FrameLen += TmpLen;
			}
			RTMP_SPIN_UNLOCK(&ap_vendor_ie->vendor_ie_lock);
		}
	}
#endif /* CUSTOMER_VENDOR_IE_SUPPORT */

	/* 802.11n 11.1.3.2.2 active scanning. sending probe response with MCS rate is */
	/* configure to better support Multi-Sta */
	{
		struct _RTMP_CHIP_CAP *cap = hc_get_chip_cap(pAd->hdev_ctrl);
		UINT8 idx = 0;

		for (idx = 0; idx < cap->ProbeRspTimes; idx++)
			MiniportMMRequest(pAd, 0, pOutBuffer, FrameLen);
	}
	MlmeFreeMemory(pOutBuffer);

	return TRUE;
}

struct sync_fsm_ops ap_fsm_ops = {
	/* when Peer ProbeReq come in */
	.tx_probe_response_allowed = ap_probe_response_allowed,
	.tx_probe_response_xmit = ap_probe_response_xmit,

	.rx_peer_response_allowed = ap_rx_peer_response_allowed,
	.rx_peer_response_updated = ap_rx_peer_response_updated,

	.join_peer_response_matched = ap_join_peer_response_matched,
	.join_peer_response_updated = ap_join_peer_response_updated,
};
#endif /* CONFIG_AP_SUPPORT */
