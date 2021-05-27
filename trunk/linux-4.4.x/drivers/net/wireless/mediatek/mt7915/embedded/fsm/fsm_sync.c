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


const CHAR *SYNC_FSM_STATE_STR[SYNC_FSM_MAX_STATE] = {
	"IDLE",
	"LISTEN",
	"JOIN_WAIT",
	"PENDING"
};

const CHAR *SYNC_FSM_MSG_STR[SYNC_FSM_MAX_MSG] = {
	"JOIN_REQ",
	"JOIN_TIMEOUT",
	"SCAN_REQ",
	"SCAN_TIMEOUT",
	"PEER_PROBE_REQ",
	"PEER_PROBE_RSP",
	"PEER_BEACON",
	"ADHOC_START_REQ"
};

static inline BOOLEAN sync_fsm_state_transition(struct wifi_dev *wdev, ULONG NextState)
{
	struct _RTMP_ADAPTER *pAd = NULL;
	SCAN_CTRL *ScanCtrl = NULL;
	ULONG OldState;

	/*ASSERT(wdev);
	ASSERT(wdev->sys_handle);*/
	pAd = (RTMP_ADAPTER *)wdev->sys_handle;
	ScanCtrl = get_scan_ctrl_by_wdev(pAd, wdev);
	OldState = ScanCtrl->SyncFsm.CurrState;
	ScanCtrl->SyncFsm.CurrState = NextState;

	if (ScanCtrl->ScanReqwdev)
		MTWF_LOG(DBG_CAT_CLIENT, DBG_SUBCAT_ALL, DBG_LVL_WARN,
				 ("SYNC[%s, Band:%d, TYPE:%d]: [%s] ==============================================> [%s]\n",
				  ScanCtrl->ScanReqwdev->if_dev->name, ScanCtrl->BandIdx, ScanCtrl->ScanType,
				  SYNC_FSM_STATE_STR[OldState],
				  SYNC_FSM_STATE_STR[ScanCtrl->SyncFsm.CurrState]));
	else
		MTWF_LOG(DBG_CAT_CLIENT, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("ScanCtrl->ScanReqwdev is NULL\n"));


	return TRUE;
}

static VOID sync_fsm_enqueue_req(struct wifi_dev *wdev)
{
	RTMP_ADAPTER *pAd;
	SCAN_CTRL *ScanCtrl;
	SCAN_INFO *ScanInfo = &wdev->ScanInfo;

	/* ASSERT(wdev->sys_handle); */
	pAd = (RTMP_ADAPTER *)wdev->sys_handle;
	ScanCtrl = get_scan_ctrl_by_wdev(pAd, wdev);
	MTWF_LOG(DBG_CAT_CLIENT, DBG_SUBCAT_ALL, DBG_LVL_WARN,
			 ("%s: [%s]====================>[%s]\n", __func__,
			  (ScanCtrl->ScanReqwdev == NULL) ? "NULL" : ScanCtrl->ScanReqwdev->if_dev->name,
			  wdev->if_dev->name));
	ScanCtrl->ScanReqwdev = wdev;

	if (ScanCtrl->ScanType == SCAN_IMPROVED) {
		ScanInfo->bImprovedScan = TRUE;
		ScanCtrl->ImprovedScanWdev = wdev;
	} else if (ScanCtrl->ScanType == SCAN_PARTIAL) {
		ScanCtrl->PartialScan.bScanning = TRUE;
		ScanCtrl->PartialScan.pwdev = wdev;
	}
}

static VOID sync_fsm_scan_timeout(
	PVOID SystemSpecific1,
	PVOID FunctionContext,
	PVOID SystemSpecific2,
	PVOID SystemSpecific3)
{
	PTIMER_FUNC_CONTEXT pContext = (PTIMER_FUNC_CONTEXT)FunctionContext;
	UCHAR BandIdx = pContext->BandIdx;
	RTMP_ADAPTER *pAd = (RTMP_ADAPTER *)pContext->pAd;
	struct wifi_dev *wdev = pAd->ScanCtrl[BandIdx].ScanReqwdev;

	/* ASSERT(wdev);*/
	MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("AP SYNC - Scan Timeout\n"));

	/*
	    Do nothing if the driver is starting halt state.
	    This might happen when timer already been fired before cancel timer with mlmehalt
	*/
	if (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_HALT_IN_PROGRESS))
		goto scan_cancel;

	if (MlmeEnqueueWithWdev(pAd, SYNC_FSM, SYNC_FSM_SCAN_TIMEOUT,
							0, NULL, wdev->func_idx, wdev)) {
		RTMP_MLME_HANDLER(pAd);
		return;
	}

scan_cancel:
	/*ASSERT(FALSE);*/
	pAd->ScanCtrl[BandIdx].Channel = 0;
	sync_fsm_state_transition(wdev, SYNC_FSM_IDLE);
	cntl_scan_conf(wdev, MLME_SUCCESS);

}
DECLARE_TIMER_FUNCTION(sync_fsm_scan_timeout);
BUILD_TIMER_FUNCTION(sync_fsm_scan_timeout);

static VOID sync_fsm_join_timeout(
	IN PVOID SystemSpecific1,
	IN PVOID FunctionContext,
	IN PVOID SystemSpecific2,
	IN PVOID SystemSpecific3)
{
	PTIMER_FUNC_CONTEXT pContext = (PTIMER_FUNC_CONTEXT)FunctionContext;
	struct wifi_dev *wdev = (struct wifi_dev *)pContext->wdev;
	RTMP_ADAPTER *pAd = (RTMP_ADAPTER *)pContext->pAd;

	MTWF_LOG(DBG_CAT_CLIENT, CATCLIENT_APCLI, DBG_LVL_TRACE, ("%s - Enter\n", __func__));

	/*
	    Do nothing if the driver is starting halt state.
	    This might happen when timer already been fired before cancel timer with mlmehalt
	*/
	if (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_HALT_IN_PROGRESS))
		return;

	MlmeEnqueueWithWdev(pAd, SYNC_FSM, SYNC_FSM_JOIN_TIMEOUT, 0, NULL, wdev->func_idx, pContext->wdev);
	RTMP_MLME_HANDLER(pAd);
}
DECLARE_TIMER_FUNCTION(sync_fsm_join_timeout);
BUILD_TIMER_FUNCTION(sync_fsm_join_timeout);

#ifdef CONFIG_STA_SUPPORT
/*
	==========================================================================
	Description:
	==========================================================================
 */
static BOOLEAN sta_enqueue_join_probe_request(PRTMP_ADAPTER pAd, struct wifi_dev *wdev)
{
	NDIS_STATUS     NState;
	UCHAR           *pOutBuffer;
	ULONG           FrameLen = 0;
	HEADER_802_11   Hdr80211;
	UCHAR ssidLen;
	CHAR ssid[MAX_LEN_OF_SSID];
#if defined(TXBF_SUPPORT) && defined(VHT_TXBF_SUPPORT)
	UCHAR ucETxBfCap;
#endif /* TXBF_SUPPORT && VHT_TXBF_SUPPORT */
	struct _build_ie_info ie_info = {0};
	PUCHAR pSupRate = NULL;
	UCHAR SupRateLen;
	PUCHAR pExtRate = NULL;
	UCHAR  ExtRateLen;
	UCHAR ASupRate[] = {0x8C, 0x12, 0x98, 0x24, 0xb0, 0x48, 0x60, 0x6C};
	UCHAR ASupRateLen = sizeof(ASupRate) / sizeof(UCHAR);
	MLME_AUX *MlmeAux = NULL;
	PSTA_ADMIN_CONFIG pStaCfg = GetStaCfgByWdev(pAd, wdev);
	struct legacy_rate *rate;

	if (!pStaCfg)
		return FALSE;

	MlmeAux = &pStaCfg->MlmeAux;
	rate = &MlmeAux->rate;
	/* ASSERT(MlmeAux);
	ASSERT(wdev); */
	MTWF_LOG(DBG_CAT_CLIENT, CATCLIENT_APCLI, DBG_LVL_TRACE, ("force out a JOIN ProbeRequest ...\n"));
	NState = MlmeAllocateMemory(pAd, &pOutBuffer);  /* Get an unused nonpaged memory */

	if (NState != NDIS_STATUS_SUCCESS) {
		MTWF_LOG(DBG_CAT_CLIENT, CATCLIENT_APCLI, DBG_LVL_ERROR, ("EnqueueProbeRequest() allocate memory fail\n"));
		return FALSE;
	} else {
		ie_info.frame_subtype = SUBTYPE_PROBE_REQ;
		ie_info.channel = wdev->channel;
		ie_info.phy_mode = wdev->PhyMode;
		ie_info.wdev = wdev;

		if (MlmeAux->BssType == BSS_INFRA) {
			if (MAC_ADDR_EQUAL(MlmeAux->Bssid, ZERO_MAC_ADDR)) {
				MgtMacHeaderInitExt(pAd, &Hdr80211, SUBTYPE_PROBE_REQ, 0,
									BROADCAST_ADDR, wdev->if_addr, BROADCAST_ADDR);
			} else {
#ifdef CONFIG_MAP_SUPPORT
				if (IS_MAP_ENABLE(pAd)) {
					MgtMacHeaderInitExt(pAd, &Hdr80211, SUBTYPE_PROBE_REQ, 0,
										MlmeAux->Bssid, wdev->if_addr, MlmeAux->Bssid);
				} else {
#endif
					MgtMacHeaderInitExt(pAd, &Hdr80211, SUBTYPE_PROBE_REQ, 0,
									MlmeAux->Bssid, wdev->if_addr, BROADCAST_ADDR);
#ifdef CONFIG_MAP_SUPPORT
				}
#endif
			}
		} else {
			MgtMacHeaderInitExt(pAd, &Hdr80211, SUBTYPE_PROBE_REQ, 0,
								BROADCAST_ADDR, wdev->if_addr, BROADCAST_ADDR);
		}

		ssidLen = MlmeAux->SsidLen;
		NdisZeroMemory(ssid, MAX_LEN_OF_SSID);
		NdisMoveMemory(ssid, MlmeAux->Ssid, ssidLen);

		if (MlmeAux->Channel <= 14) {
			pSupRate = rate->sup_rate;
			SupRateLen = rate->sup_rate_len;
			pExtRate = rate->ext_rate;
			ExtRateLen = rate->ext_rate_len;
		} else {
			/* Overwrite Support Rate, CCK rate are not allowed */
			pSupRate = ASupRate;
			SupRateLen = ASupRateLen;
			ExtRateLen = 0;
		}

		/* this ProbeRequest explicitly specify SSID to reduce unwanted ProbeResponse */
		MakeOutgoingFrame(pOutBuffer,		&FrameLen,
						  sizeof(HEADER_802_11),			&Hdr80211,
						  1,								&SsidIe,
						  1,								&ssidLen,
						  ssidLen,						ssid,
						  END_OF_ARGS);
		FrameLen += build_support_rate_ie(wdev, pSupRate, SupRateLen, pOutBuffer + FrameLen);

		/* Add the extended rate IE */
		if (ExtRateLen) {
			ULONG Tmp;

			MakeOutgoingFrame(pOutBuffer + FrameLen, &Tmp,
							  1,            &ExtRateIe,
							  1,            &ExtRateLen,
							  ExtRateLen,   pExtRate,
							  END_OF_ARGS);
			FrameLen += Tmp;
		}

#ifdef DOT11_N_SUPPORT

		if (WMODE_CAP_N(wdev->PhyMode)) {
			ie_info.frame_buf = (UCHAR *)(pOutBuffer + FrameLen);
			FrameLen += build_ht_ies(pAd, &ie_info);
#ifdef DOT11_VHT_AC
#ifdef MBO_SUPPORT
		ie_info.frame_buf = (UCHAR *)(pOutBuffer + FrameLen);
		FrameLen += build_extended_cap_ie(pAd, &ie_info);
#endif /* MBO_SUPPORT */
#if defined(TXBF_SUPPORT) && defined(VHT_TXBF_SUPPORT)
			ucETxBfCap = wlan_config_get_etxbf(wdev);

			if (HcIsBfCapSupport(wdev) == FALSE)
				wlan_config_set_etxbf(wdev, SUBF_OFF);

#endif /* TXBF_SUPPORT && VHT_TXBF_SUPPORT */
			ie_info.frame_buf = (UCHAR *)(pOutBuffer + FrameLen);
			FrameLen += build_vht_ies(pAd, &ie_info);
#if defined(TXBF_SUPPORT) && defined(VHT_TXBF_SUPPORT)
			wlan_config_set_etxbf(wdev, ucETxBfCap);
#endif /* TXBF_SUPPORT && VHT_TXBF_SUPPORT */
#endif /* DOT11_VHT_AC */
		}

#endif /* DOT11_N_SUPPORT */
#ifdef MBO_SUPPORT
		if (IS_MBO_ENABLE(wdev))
			MakeMboOceIE(pAd, wdev, NULL, pOutBuffer + FrameLen, &FrameLen, MBO_FRAME_TYPE_PROBE_REQ);
#endif /* MBO_SUPPORT */
#ifdef WSC_INCLUDED
		ie_info.frame_buf = (UCHAR *)(pOutBuffer + FrameLen);
		FrameLen += build_wsc_ie(pAd, &ie_info);
#endif /* WSC_INCLUDED */
		ie_info.frame_buf = (UCHAR *)(pOutBuffer + FrameLen);
		FrameLen +=  build_extra_ie(pAd, &ie_info);
#ifdef WH_EVENT_NOTIFIER
		if (wdev->custom_vie.ie_hdr.len > 0) {
			ULONG custom_vie_len;
			ULONG total_custom_vie_len = sizeof(struct Custom_IE_Header) + wdev->custom_vie.ie_hdr.len;

			MakeOutgoingFrame((pOutBuffer + FrameLen), &custom_vie_len,
				total_custom_vie_len, (UCHAR *)wdev->custom_vie, END_OF_ARGS);
			FrameLen += custom_vie_len;
		}
#endif /* WH_EVENT_NOTIFIER */
		NState = MiniportMMRequest(pAd, QID_AC_BE, pOutBuffer, FrameLen);
		MlmeFreeMemory(pOutBuffer);

		if (NState != NDIS_STATUS_SUCCESS) {
			MTWF_LOG(DBG_CAT_CLIENT, CATCLIENT_APCLI, DBG_LVL_ERROR,
					 ("%s, %u MiniportMMRequest failed STATUS %d\033[0m\n",
					  __func__, __LINE__, NState));
			return FALSE;
		}
	}

	return TRUE;
}
#endif

static BOOLEAN sync_fsm_error_handle(struct wifi_dev *wdev, MLME_QUEUE_ELEM *Elem)
{
	BOOLEAN isErrHandle = TRUE;
	USHORT Status = MLME_INVALID_FORMAT;

	/* ASSERT(wdev);*/

	switch (Elem->MsgType) {
	case SYNC_FSM_JOIN_REQ:
	case SYNC_FSM_ADHOC_START_REQ:
	case SYNC_FSM_JOIN_TIMEOUT:
		cntl_join_start_conf(wdev, Status);
		break;

	case SYNC_FSM_SCAN_REQ:
	case SYNC_FSM_SCAN_TIMEOUT:
		cntl_scan_conf(wdev, Status);
		break;

	default:
		isErrHandle = FALSE;
	}

	return isErrHandle;
}

static BOOLEAN sync_fsm_msg_checker(PRTMP_ADAPTER pAd, MLME_QUEUE_ELEM *Elem)
{
	BOOLEAN isMsgDrop = FALSE;
	BOOLEAN isErrHandle = TRUE;
	struct wifi_dev *wdev = Elem->wdev;
	SCAN_CTRL *ScanCtrl = get_scan_ctrl_by_wdev(pAd, wdev);

	if (wdev) {
		if (!wdev->DevInfo.WdevActive)
			isMsgDrop = TRUE;

#ifdef APCLI_SUPPORT

		if (IF_COMBO_HAVE_AP_STA(pAd) &&
			(wdev->wdev_type == WDEV_TYPE_STA) &&
			(isValidApCliIf(wdev->func_idx) == FALSE))
			isMsgDrop = TRUE;

#endif /* APCLI_SUPPORT */
	} else
		isMsgDrop = TRUE;

	if (isMsgDrop == TRUE) {
	/*	ASSERT(wdev); */
		isErrHandle = sync_fsm_error_handle(wdev, Elem);

		if (isErrHandle)
			MTWF_LOG(DBG_CAT_CLIENT, DBG_SUBCAT_ALL, DBG_LVL_WARN,
					 ("%s [%s]: [%s][%s] ====================> state Recovery for CNTL\n",
					  __func__, wdev->if_dev->name,
					  SYNC_FSM_STATE_STR[ScanCtrl->SyncFsm.CurrState],
					  SYNC_FSM_MSG_STR[Elem->MsgType]));
	}

	return isMsgDrop;
}

static VOID sync_fsm_msg_invalid_state(struct _RTMP_ADAPTER *pAd, MLME_QUEUE_ELEM *Elem)
{
	struct wifi_dev *wdev = Elem->wdev;
	SCAN_CTRL *ScanCtrl = get_scan_ctrl_by_wdev(pAd, wdev);
	BOOLEAN isErrHandle = TRUE;

	isErrHandle = sync_fsm_error_handle(wdev, Elem);

	if (isErrHandle == TRUE) {
		MTWF_LOG(DBG_CAT_CLIENT, DBG_SUBCAT_ALL, DBG_LVL_WARN,
				 ("%s [%s]: [%s][%s] ====================> state Recovery for CNTL\n",
				  __func__, wdev->if_dev->name,
				  SYNC_FSM_STATE_STR[ScanCtrl->SyncFsm.CurrState],
				  SYNC_FSM_MSG_STR[Elem->MsgType]));
	} else {
		MTWF_LOG(DBG_CAT_CLIENT, DBG_SUBCAT_ALL, DBG_LVL_WARN,
				 ("%s [%s]: [%s][%s] ====================> FSM MSG DROP\n",
				  __func__, wdev->if_dev->name,
				  SYNC_FSM_STATE_STR[ScanCtrl->SyncFsm.CurrState],
				  SYNC_FSM_MSG_STR[Elem->MsgType]));
	}
}

#ifdef CON_WPS
static USHORT con_wps_scan_done_handler(PRTMP_ADAPTER pAd, MLME_QUEUE_ELEM *Elem)
{
	PWSC_CTRL   pWscControl;
	PWSC_CTRL   pApCliWscControl;
	UCHAR       apidx;
	INT         IsAPConfigured;
	struct wifi_dev *wdev;
	BOOLEAN     bNeedSetPBCTimer = TRUE;
	USHORT Status = MLME_SUCCESS;
#if defined(CON_WPS)
	INT currIfaceIdx = 0;
	UCHAR ifIdx;
	UCHAR oppifIdx;
	struct wifi_dev *ConWpsdev = NULL;
	PWSC_CTRL   pTriggerApCliWscControl;
	PWSC_CTRL   pOpposApCliWscControl;
	PRTMP_ADAPTER pOpposAd;
	BOOLEAN     bTwoCardConWPS = FALSE;
	UCHAR apcli_idx;
#ifdef MULTI_INF_SUPPORT /* Index 0 for 2.4G, 1 for 5Ghz Card */
	UINT opposIfaceIdx = !multi_inf_get_idx(pAd);
#endif /* MULTI_INF_SUPPORT */
#endif /*CON_WPS*/
#if defined(CON_WPS)
	pOpposAd = NULL;
	pOpposApCliWscControl = NULL;
	pTriggerApCliWscControl = NULL;
#ifdef MULTI_INF_SUPPORT /* Index 0 for 2.4G, 1 for 5Ghz Card */
	pOpposAd = (PRTMP_ADAPTER)adapt_list[opposIfaceIdx];
#endif /* MULTI_INF_SUPPORT */
#endif /*CON_WPS*/

	/* If We catch the SR=TRUE in last scan_res, stop the AP Wsc SM */
	if (Elem) {
		ifIdx = (USHORT)(Elem->Priv);

		if (ifIdx < pAd->ApCfg.ApCliNum)
			ConWpsdev =  &(pAd->StaCfg[ifIdx].wdev);

		if (ConWpsdev == NULL)
			return Status;
	} else
		return Status;

	if (ifIdx == BSS0)
		oppifIdx = BSS1;
	else if (ifIdx == BSS1)
		oppifIdx = BSS0;
	else
		return Status;

	if (ConWpsdev) {
		pApCliWscControl = &pAd->StaCfg[ifIdx].wdev.WscControl;
		pAd->StaCfg[ifIdx].ConWpsApCliModeScanDoneStatus = CON_WPS_APCLI_SCANDONE_STATUS_FINISH;
	}

	if (pOpposAd) {
		for (apcli_idx = 0; apcli_idx < pOpposAd->ApCfg.ApCliNum; apcli_idx++) {
			if (pOpposAd->StaCfg[apcli_idx].wdev.WscControl.conWscStatus == CON_WPS_STATUS_APCLI_RUNNING) {
				pOpposApCliWscControl = &pOpposAd->StaCfg[apcli_idx].wdev.WscControl;
				bTwoCardConWPS = TRUE;
				break;
			}
		}

		if (apcli_idx == pOpposAd->ApCfg.ApCliNum) {
			pOpposApCliWscControl = NULL;
			bTwoCardConWPS = FALSE;
		}
	}

	if (bTwoCardConWPS == FALSE) {
		for (apcli_idx = 0; apcli_idx < pAd->ApCfg.ApCliNum; apcli_idx++) {
			if (apcli_idx == ifIdx)
				continue;
			else if (pAd->StaCfg[apcli_idx].wdev.WscControl.conWscStatus == CON_WPS_STATUS_APCLI_RUNNING) {
				pOpposApCliWscControl = &pAd->StaCfg[apcli_idx].wdev.WscControl;
				break;
			}
		}
	}

	if (pOpposAd && pAd->ApCfg.ConWpsApCliMode == CON_WPS_APCLI_BAND_AUTO) { /* 2.2G and 5G must trigger scan */
		if (pOpposAd && bTwoCardConWPS) {
			for (apcli_idx = 0; apcli_idx < pAd->ApCfg.ApCliNum; apcli_idx++) {
				if (pOpposAd->StaCfg[apcli_idx].ConWpsApCliModeScanDoneStatus == CON_WPS_APCLI_SCANDONE_STATUS_ONGOING) {
					pApCliWscControl->ConWscApcliScanDoneCheckTimerRunning = TRUE;
					RTMPSetTimer(&pApCliWscControl->ConWscApcliScanDoneCheckTimer, 1000);
					return MLME_UNSPECIFY_FAIL;
				}
			}
		}
	} else {
		for (apcli_idx = 0; apcli_idx < pAd->ApCfg.ApCliNum; apcli_idx++) {
			if (pAd->StaCfg[apcli_idx].ConWpsApCliModeScanDoneStatus ==
				CON_WPS_APCLI_SCANDONE_STATUS_ONGOING ||
				((pAd->StaCfg[apcli_idx].wdev.WscControl.conWscStatus != CON_WPS_STATUS_DISABLED)
				&& (pAd->StaCfg[apcli_idx].wdev.WscControl.con_wps_scan_trigger_count <
				pApCliWscControl->con_wps_scan_trigger_count))) {
				pApCliWscControl->ConWscApcliScanDoneCheckTimerRunning = TRUE;
				MTWF_LOG(DBG_CAT_CLIENT, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
					("My PBC trigger count = %d, other band count = %d\n",
					pApCliWscControl->con_wps_scan_trigger_count,
					pAd->StaCfg[apcli_idx].wdev.WscControl.con_wps_scan_trigger_count));
				MTWF_LOG(DBG_CAT_CLIENT, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
					("scan complete handler called for %d, scan is ongoing for %d\n",
					ifIdx, apcli_idx));
				MTWF_LOG(DBG_CAT_CLIENT, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
					("starting ConWscApcliScanDoneCheckTimer\n"));
				RTMPSetTimer(&pApCliWscControl->ConWscApcliScanDoneCheckTimer, 1000);
				return MLME_UNSPECIFY_FAIL;
			}
		}
	}

	if ((pOpposApCliWscControl) == NULL && pOpposAd) {
		pOpposApCliWscControl = &pOpposAd->StaCfg[BSS0].wdev.WscControl;
		bTwoCardConWPS = TRUE;
	}

	if (pOpposApCliWscControl == NULL) {
		pOpposApCliWscControl = &pAd->StaCfg[ifIdx].wdev.WscControl;
		pOpposApCliWscControl->wdev = &pAd->StaCfg[ifIdx].wdev;
		bTwoCardConWPS = FALSE;
	}

	WscPBCBssTableSort(pAd, pApCliWscControl);
#if defined(CON_WPS)
#ifdef MULTI_INF_SUPPORT /* Index 0 for 2.4G, 1 for 5Ghz Card */

	if (pOpposAd && bTwoCardConWPS) {
		if (pOpposApCliWscControl)
			WscPBCBssTableSort(pOpposAd, pOpposApCliWscControl);
	} else
#endif /* MULTI_INF_SUPPORT */
	{
		if (pOpposApCliWscControl)
			WscPBCBssTableSort(pAd, pOpposApCliWscControl);
	}

	MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("[Iface_Idx: %d] Scan_Completed!!! In APMlmeScanCompleteAction\n", currIfaceIdx));
#endif /*CON_WPS*/
#ifdef MULTI_INF_SUPPORT
	currIfaceIdx = multi_inf_get_idx(pAd);
#else
	currIfaceIdx = (pApCliWscControl->EntryIfIdx & 0x0F);
#endif /* MULTI_INF_SUPPORT */

	for (apidx = 0; apidx < pAd->ApCfg.BssidNum; apidx++) {
		wdev = &pAd->ApCfg.MBSSID[apidx].wdev;
		pWscControl = &wdev->WscControl;
		IsAPConfigured = pWscControl->WscConfStatus;

		if ((pWscControl->WscConfMode != WSC_DISABLE) &&
			(pApCliWscControl->WscPBCBssCount > 0)) {
			if (pWscControl->bWscTrigger == TRUE) {
				MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s CON_WPS[%d]: Stop the AP Wsc Machine\n", __func__, apidx));
				WscBuildBeaconIE(pAd, IsAPConfigured, FALSE, 0, 0, apidx, NULL, 0, AP_MODE);
				WscBuildProbeRespIE(pAd, WSC_MSGTYPE_AP_WLAN_MGR, IsAPConfigured, FALSE, 0, 0, apidx, NULL, 0, AP_MODE);
				UpdateBeaconHandler(pAd, wdev, BCN_UPDATE_IE_CHG);
				WscStop(pAd, FALSE, pWscControl);
			}

			WscConWpsStop(pAd, FALSE, pWscControl);
		}

		continue;
	}

	if (bTwoCardConWPS) {
		if (pApCliWscControl->WscPBCBssCount == 1 && pOpposApCliWscControl->WscPBCBssCount == 1) {
			MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("[Iface_Idx: %d] AutoPreferIface = %d\n"
					 , currIfaceIdx, pAd->ApCfg.ConWpsApcliAutoPreferIface));

			if (currIfaceIdx == 0) {
				if (pAd->ApCfg.ConWpsApcliAutoPreferIface == CON_WPS_APCLI_AUTO_PREFER_IFACE1) {
					bNeedSetPBCTimer = FALSE;
					WscStop(pAd, TRUE, pApCliWscControl);
					MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("!! STOP APCLI = %d !!\n", currIfaceIdx));
				} else {
					WscConWpsStop(pAd, TRUE, pApCliWscControl);
					MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("!! STOP APCLI = %d !!\n", !currIfaceIdx));
				}
			} else if (currIfaceIdx == 1) {
				if (pAd->ApCfg.ConWpsApcliAutoPreferIface == CON_WPS_APCLI_AUTO_PREFER_IFACE0) {
					bNeedSetPBCTimer = FALSE;
					WscStop(pAd, TRUE, pApCliWscControl);
					MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("!!STOP APCLI = %d !!\n", currIfaceIdx));
				} else {
					WscConWpsStop(pAd, TRUE, pApCliWscControl);
					MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("!! STOP APCLI = %d !!\n", !currIfaceIdx));
				}
			}
		}

		if (pApCliWscControl->WscPBCBssCount == 1 && pOpposApCliWscControl->WscPBCBssCount == 0) {
			WscConWpsStop(pAd, TRUE, pApCliWscControl);
			MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("!! (5)STOP APCLI = %d !!\n", !currIfaceIdx));
		}
	} else {
		currIfaceIdx = (pApCliWscControl->EntryIfIdx & 0x0F);
		MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("[Iface_Idx: %d] Registrar_Found,  APCLI_Auto_Mode PreferIface = %d\n",
				 currIfaceIdx, pAd->ApCfg.ConWpsApcliAutoPreferIface));
		MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("[Iface_Idx: %d] WscPBCBssCount = %d, opposWscPBCBssCount = %d\n",
				 currIfaceIdx,
				 pApCliWscControl->WscPBCBssCount,
				 pOpposApCliWscControl->WscPBCBssCount));

		if (pApCliWscControl->WscPBCBssCount == 1 && pOpposApCliWscControl->WscPBCBssCount == 1) {
			MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("[Iface_Idx: %d] AutoPreferIface = %d\n"
					 , currIfaceIdx, pAd->ApCfg.ConWpsApcliAutoPreferIface));

			if (pAd->ApCfg.ConWpsApCliMode == CON_WPS_APCLI_BAND_AUTO) {
				if (currIfaceIdx != pAd->ApCfg.ConWpsApcliAutoPreferIface) {
					bNeedSetPBCTimer = FALSE;
					WscStop(pAd, TRUE, pApCliWscControl);
					WscConWpsStop(pAd, TRUE, pOpposApCliWscControl);
					pTriggerApCliWscControl = pOpposApCliWscControl;
				} else {
					WscConWpsStop(pAd, TRUE, pApCliWscControl);
					pTriggerApCliWscControl = pApCliWscControl;
				}
			} else if (pAd->ApCfg.ConWpsApCliMode == CON_WPS_APCLI_BAND_2G) {
				WscConWpsStop(pAd, TRUE, &(pAd->StaCfg[BSS0].wdev.WscControl));
				pTriggerApCliWscControl =  &(pAd->StaCfg[BSS0].wdev.WscControl);
			} else if (pAd->ApCfg.ConWpsApCliMode == CON_WPS_APCLI_BAND_5G) {
				WscConWpsStop(pAd, TRUE, &(pAd->StaCfg[BSS1].wdev.WscControl));
				pTriggerApCliWscControl =  &(pAd->StaCfg[BSS1].wdev.WscControl);
			}
		}

		/*Only Found 1 Registrar at one interface*/
		if (pApCliWscControl->WscPBCBssCount == 1 && pOpposApCliWscControl->WscPBCBssCount == 0) {
			if (pAd->ApCfg.ConWpsApCliMode == CON_WPS_APCLI_BAND_AUTO) {
				WscConWpsStop(pAd, TRUE, pApCliWscControl);
				pTriggerApCliWscControl = pApCliWscControl;
			} else if (pAd->ApCfg.ConWpsApCliMode == CON_WPS_APCLI_BAND_2G) {
				if (currIfaceIdx == 0) {
					WscConWpsStop(pAd, TRUE, pApCliWscControl);
					pTriggerApCliWscControl = pApCliWscControl;
				}
			} else if (pAd->ApCfg.ConWpsApCliMode == CON_WPS_APCLI_BAND_5G) {
				if (currIfaceIdx == 1) {
					WscConWpsStop(pAd, TRUE, pApCliWscControl);
					pTriggerApCliWscControl = pApCliWscControl;
				}
			}

			MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("!! (5)STOP APCLI = %d !!\n", !currIfaceIdx));
		} else if (pApCliWscControl->WscPBCBssCount == 0 && pOpposApCliWscControl->WscPBCBssCount == 1) {
			if (pAd->ApCfg.ConWpsApCliMode == CON_WPS_APCLI_BAND_AUTO) {
				WscConWpsStop(pAd, TRUE, pOpposApCliWscControl);
				pTriggerApCliWscControl = pOpposApCliWscControl;
				MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("!! (6)STOP APCLI = %d !!\n", !currIfaceIdx));
			} else {
				if ((pAd->ApCfg.ConWpsApCliMode == CON_WPS_APCLI_BAND_2G) && (pOpposApCliWscControl->EntryIfIdx & 0x0F) == 0) {
					WscConWpsStop(pAd, TRUE, pOpposApCliWscControl);
					pTriggerApCliWscControl = pOpposApCliWscControl;
				} else if ((pAd->ApCfg.ConWpsApCliMode == CON_WPS_APCLI_BAND_5G) && (pOpposApCliWscControl->EntryIfIdx & 0x0F) == 1) {
					WscConWpsStop(pAd, TRUE, pOpposApCliWscControl);
					pTriggerApCliWscControl = pOpposApCliWscControl;
				}
			}
		} else if (pApCliWscControl->WscPBCBssCount > 1 || pOpposApCliWscControl->WscPBCBssCount > 1) {
			MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
				("Overlap detected on atleast one band. stop APCLI WPS on both the bands\n"));
			WscConWpsStop(pAd, TRUE, pApCliWscControl);
			WscStop(pAd, TRUE, pApCliWscControl);
			return MLME_UNSPECIFY_FAIL;
		}
	}

	if (bTwoCardConWPS) {
		if (bNeedSetPBCTimer && pApCliWscControl->WscPBCTimerRunning == FALSE) {
			if (pApCliWscControl->bWscTrigger) {
				pApCliWscControl->WscPBCTimerRunning = TRUE;
				MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("!! TwoCardConWPS Trigger %s WPS!!\n", (pApCliWscControl->IfName)));
				RTMPSetTimer(&pApCliWscControl->WscPBCTimer, 1000);
			}
		}
	} else {
		if (pTriggerApCliWscControl != NULL &&
			(pTriggerApCliWscControl->WscPBCTimerRunning == FALSE) &&
			(pTriggerApCliWscControl->bWscTrigger == TRUE)) {
			pTriggerApCliWscControl->WscPBCTimerRunning = TRUE;
			pTriggerApCliWscControl->con_wps_scan_trigger_count++;
			MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("!! One Card DBDC Trigger %s WPS!!\n", (pTriggerApCliWscControl->IfName)));
			RTMPSetTimer(&pTriggerApCliWscControl->WscPBCTimer, 1000);
		} else {
			if (pApCliWscControl && pApCliWscControl->WscPBCTimerRunning == FALSE &&
				(pApCliWscControl->bWscTrigger == TRUE)) {
				pAd->StaCfg[(pApCliWscControl->EntryIfIdx & 0x0F)].ConWpsApCliModeScanDoneStatus = CON_WPS_APCLI_SCANDONE_STATUS_ONGOING;
				pApCliWscControl->WscPBCTimerRunning = TRUE;
				pApCliWscControl->con_wps_scan_trigger_count++;
				RTMPSetTimer(&pApCliWscControl->WscPBCTimer, 1000);
			}
		}
	}
	return Status;
}
#endif /* CON_WPS*/

static VOID sync_fsm_wsc_scan_comp_check_action(
	struct _RTMP_ADAPTER *pAd,
	MLME_QUEUE_ELEM *Elem)
{
	struct wifi_dev *wdev = Elem->wdev;
	USHORT Status = MLME_SUCCESS;

#ifdef CON_WPS
	if (wdev->WscControl.conWscStatus == CON_WPS_STATUS_APCLI_RUNNING)
		Status = con_wps_scan_done_handler(pAd, Elem);
#endif
	if (Status == MLME_SUCCESS)
		cntl_scan_conf(wdev, Status);
}

static VOID sync_fsm_scan_complete_action(
	struct _RTMP_ADAPTER *pAd,
	MLME_QUEUE_ELEM *Elem,
	BOOLEAN isScanPending)
{
	struct wifi_dev *wdev = Elem->wdev;
	SCAN_INFO *ScanInfo = &wdev->ScanInfo;
	USHORT Status = MLME_SUCCESS;
#ifdef CON_WPS
	WSC_CTRL *pWpsCtrl = &wdev->WscControl;
#endif /* CON_WPS*/

	if (isScanPending == FALSE) {
		/* scan completed, init to not FastScan */
		ScanInfo->bImprovedScan = FALSE;

#ifdef CON_WPS
		if (pWpsCtrl->conWscStatus == CON_WPS_STATUS_APCLI_RUNNING)
			Status = con_wps_scan_done_handler(pAd, Elem);
#endif /* CON_WPS*/
#ifdef OCE_SUPPORT
		if (IS_OCE_RNR_ENABLE(wdev) && IS_OCE_ENABLE(wdev)) {
#ifdef MBO_SUPPORT
			if (IS_MBO_ENABLE(wdev))
				MboIndicateNeighborReportToDaemon(pAd, wdev, TRUE, PER_EVENT_LIST_MAX_NUM);
#endif /* MBO_SUPPORT */
		}
#endif /* OCE_SUPPORT */

	}
	if (Status == MLME_SUCCESS)
		cntl_scan_conf(wdev, Status);
}

static VOID sync_fsm_join_req_action(struct _RTMP_ADAPTER *pAd, MLME_QUEUE_ELEM *Elem)
{
#ifdef CONFIG_STA_SUPPORT
	BOOLEAN Cancelled;
	BOOLEAN isGoingToJoin = FALSE;
	MLME_JOIN_REQ_STRUCT *Info = (MLME_JOIN_REQ_STRUCT *)(Elem->Msg);
	struct wifi_dev *wdev = Elem->wdev;
	struct legacy_rate *rate = &wdev->rate.legacy_rate;
	struct legacy_rate *mlme_rate;
	MLME_AUX *mlmeAux = NULL;
	BSS_ENTRY *pBss = NULL;
	PSTA_ADMIN_CONFIG pStaCfg = GetStaCfgByWdev(pAd, wdev);
	MAC_TABLE_ENTRY *pEntry = GetAssociatedAPByWdev(pAd, wdev);

	/*ASSERT(pStaCfg);*/
	MTWF_LOG(DBG_CAT_CLIENT, CATCLIENT_APCLI, DBG_LVL_TRACE,
			 ("%s - (Ssid %s)\n", __func__, Info->Ssid));
	mlmeAux = &pStaCfg->MlmeAux;
	mlme_rate = &mlmeAux->rate;
	/*ASSERT(mlmeAux);*/
#ifdef COEX_SUPPORT

	if (IS_MT76x6(pAd) || IS_MT7637(pAd) || IS_MT7622(pAd)) {
		if ((pAd->BtWlanStatus & COEX_STATUS_LINK_UP) != 0)
			MT76xxMLMEHook(pAd, MT76xx_WLAN_LINK_DONE, 0);

		MT76xxMLMEHook(pAd, MT76xx_WLAN_LINK_START, 0);
	}

#endif /*COEX_SUPPORT */
	/* reset all the timers */
	RTMPCancelTimer(&mlmeAux->JoinTimer, &Cancelled);
	sync_fsm_enqueue_req(wdev);
	mlmeAux->Rssi = -128;
	mlmeAux->isRecvJoinRsp = FALSE;
	mlmeAux->BssType = pStaCfg->BssType;

	if (pEntry) {
		MTWF_LOG(DBG_CAT_CLIENT, CATCLIENT_APCLI, DBG_LVL_OFF,
					("***** STALE  Peer AP entry present--Delete it ****\n"));
		MacTableDeleteEntry(pAd, pEntry->wcid, pEntry->Addr);
	}

	if (Info->BssIdx != BSS_NOT_FOUND) {
		pBss = &mlmeAux->SsidBssTab.BssEntry[Info->BssIdx];
		/*ASSERT(pBss);*/
		/* record the desired SSID & BSSID we're waiting for */
		COPY_MAC_ADDR(mlmeAux->Bssid, pBss->Bssid);

		/* If AP's SSID is not hidden, it is OK for updating ssid to MlmeAux again. */
		if (pBss->Hidden == 0) {
			RTMPZeroMemory(mlmeAux->Ssid, MAX_LEN_OF_SSID);
			NdisMoveMemory(mlmeAux->Ssid, pBss->Ssid, pBss->SsidLen);
			mlmeAux->SsidLen = pBss->SsidLen;
		}

		mlmeAux->BssType = pBss->BssType;
		mlmeAux->Channel = pBss->Channel;
		mlmeAux->CentralChannel = pBss->CentralChannel;
#ifdef EXT_BUILD_CHANNEL_LIST

		/* Country IE of the AP will be evaluated and will be used. */
		if ((pStaCfg->IEEE80211dClientMode != Rt802_11_D_None) &&
			(pBss->bHasCountryIE == TRUE)) {
			NdisMoveMemory(&pAd->CommonCfg.CountryCode[0], &pBss->CountryString[0], 2);

			if (pBss->CountryString[2] == 'I')
				pAd->CommonCfg.Geography = IDOR;
			else if (pBss->CountryString[2] == 'O')
				pAd->CommonCfg.Geography = ODOR;
			else
				pAd->CommonCfg.Geography = BOTH;

			BuildChannelListEx(pAd, wdev);
		}

#endif /* EXT_BUILD_CHANNEL_LIST */
		HcCrossChannelCheck(pAd, wdev, mlmeAux->Channel);
		/* switch channel and waiting for beacon timer */
		wdev->channel  = pStaCfg->MlmeAux.Channel;
		isGoingToJoin = wlan_operate_scan(wdev, pStaCfg->MlmeAux.Channel);

		if (isGoingToJoin)
			goto join_ret;

#ifdef WSC_STA_SUPPORT
#ifdef WSC_LED_SUPPORT

		/* LED indication. */
		if (mlmeAux->BssType == BSS_INFRA) {
			LEDConnectionStart(pAd);
			LEDConnectionCompletion(pAd, TRUE);
		}

#endif /* WSC_LED_SUPPORT */
#endif /* WSC_STA_SUPPORT */
	} else {
#ifdef RT_CFG80211_P2P_CONCURRENT_DEVICE
		/* Get Channel from ScanTable */
		mlme_rate->sup_rate_len = pAd->cfg80211_ctrl.P2pSupRateLen;
		NdisMoveMemory(mlme_rate->sup_rate, pAd->cfg80211_ctrl.P2pSupRate, pAd->cfg80211_ctrl.P2pSupRateLen);
		mlme_rate->ext_rate_len = pAd->cfg80211_ctrl.P2pExtRateLen;
		NdisMoveMemory(mlme_rate->ext_rate, pAd->cfg80211_ctrl.P2pExtRate, pAd->cfg80211_ctrl.P2pExtRateLen);
#else
#ifdef APCLI_CONNECTION_TRIAL

		if (pStaCfg->TrialCh == 0)
			mlmeAux->Channel = wdev->channel;
		else
			mlmeAux->Channel = pStaCfg->TrialCh;

		NdisZeroMemory(mlmeAux->Ssid, MAX_LEN_OF_SSID);
		NdisCopyMemory(mlmeAux->Ssid, pStaCfg->CfgSsid, pStaCfg->CfgSsidLen);
#else
		/* TODO: Star, need to modify when Multi-STA Ready! */
		/*Assign Channel for APCLI*/
		mlmeAux->Channel = wdev->channel;
#endif /* APCLI_CONNECTION_TRIAL */
		mlme_rate->sup_rate_len = rate->sup_rate_len;
		NdisMoveMemory(mlme_rate->sup_rate, rate->sup_rate, rate->sup_rate_len);
		/* Prepare the default value for extended rate */
		mlme_rate->ext_rate_len = rate->ext_rate_len;
		NdisMoveMemory(mlme_rate->ext_rate, rate->ext_rate, rate->ext_rate_len);
#endif /* RT_CFG80211_P2P_CONCURRENT_DEVICE */
		NdisZeroMemory(mlmeAux->Bssid, MAC_ADDR_LEN);
#ifdef CONFIG_OWE_SUPPORT
		/*For 5.2.2  with OWE Test Plan v1.2, Marvel AP does not respond to probe req with OWE BSS' bssid.*/
		/*This is IOT issue with Marvel AP, so we config apcli probe based on ssid when OWE certification*/
		if ((pAd->bApCliCertTest == TRUE) && (IS_AKM_OWE(pStaCfg->wdev.SecConfig.AKMMap)))
			MTWF_LOG(DBG_CAT_CLIENT, CATCLIENT_APCLI, DBG_LVL_TRACE, ("when OWE certification, force apcli scan based on ssid \n"));
#endif
		else
			NdisCopyMemory(mlmeAux->Bssid, pStaCfg->CfgApCliBssid, MAC_ADDR_LEN);
		NdisZeroMemory(mlmeAux->Ssid, MAX_LEN_OF_SSID);
		NdisCopyMemory(mlmeAux->Ssid, Info->Ssid, Info->SsidLen);
		mlmeAux->SsidLen =  Info->SsidLen;
	}

#if defined(MT7915)
#ifdef CFG_SUPPORT_FALCON_SR
		/*Spatial Reuse - HWITS00009626*/
		/*MT7915 && APCLI*/
		if (IS_MT7915(pAd) && (wdev->wdev_type == WDEV_TYPE_STA)) {
			SrDisSrBfrConnected(pAd, wdev, FALSE);
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_INFO,
			("%s - SrDisSrBfrConnected - FALSE\n", __func__));
		}
#endif /* CFG_SUPPORT_FALCON_SR */
#endif /* defined(MT7915) */

	/* We can't send any Probe request frame to meet 802.11h. */
	if ((scan_active_probe_disallowed(pAd, mlmeAux->Channel) == TRUE) &&
		(Info->BssIdx != BSS_NOT_FOUND) && (pBss->Hidden == 0))
		isGoingToJoin = TRUE;
	else if (sta_enqueue_join_probe_request(pAd, wdev) == TRUE)
		isGoingToJoin = TRUE;

join_ret:

	if (isGoingToJoin) {
		RTMPSetTimer(&mlmeAux->JoinTimer, JOIN_TIMEOUT);
		sync_fsm_state_transition(wdev, SYNC_FSM_JOIN_WAIT);
	}

	MTWF_LOG(DBG_CAT_CLIENT, CATCLIENT_APCLI, DBG_LVL_WARN, ("ApCli SYNC - Start Probe the SSID %s on channel =%d\n",
			 mlmeAux->Ssid, mlmeAux->Channel));
#endif
}

static VOID sync_fsm_join_timeout_action(struct _RTMP_ADAPTER *pAd, MLME_QUEUE_ELEM *Elem)
{
	BOOLEAN isGoingToConnect = FALSE;
	USHORT Status = MLME_REJ_TIMEOUT;
	struct wifi_dev *wdev = Elem->wdev;
#ifdef APCLI_SUPPORT
	BOOLEAN isRecvRsp = FALSE;
	BSS_TABLE *ScanTab = get_scan_tab_by_wdev(pAd, wdev);
#ifdef APCLI_CONNECTION_TRIAL
	PULONG pCtrl_CurrState = &wdev.cntl_machine.CurrState;
#endif /* APCLI_CONNECTION_TRIAL */
	STA_ADMIN_CONFIG *pApCliEntry = GetStaCfgByWdev(pAd, wdev);

	MTWF_LOG(DBG_CAT_CLIENT, CATCLIENT_APCLI, DBG_LVL_WARN, ("%s - ProbeTimeoutAtJoinAction\n", __func__));

	if (!pApCliEntry)
		return;

	isRecvRsp = pApCliEntry->MlmeAux.isRecvJoinRsp;
#ifdef APCLI_CONNECTION_TRIAL

	if (wdev->func_idx == (pAd->ApCfg.ApCliNum - 1)) /* last interface is for connection trial */
		*pCtrl_CurrState = APCLI_CTRL_DISCONNECTED;

	isRecvRsp = (isRecvRsp && (*pCtrl_CurrState != APCLI_CTRL_DISCONNECTED));
#endif /* APCLI_CONNECTION_TRIAL */
	MTWF_LOG(DBG_CAT_CLIENT, CATCLIENT_APCLI, DBG_LVL_TRACE, ("APCLI_SYNC - MlmeAux.Bssid=%02x:%02x:%02x:%02x:%02x:%02x\n",
			 PRINT_MAC(pApCliEntry->MlmeAux.Bssid)));

	if (isRecvRsp) {
#ifdef APCLI_AUTO_CONNECT_SUPPORT
#ifdef APCLI_CFG80211_SUPPORT
		if (1)
#else
		/* follow root ap setting while ApCliAutoConnectRunning is active */
		if ((pApCliEntry->ApCliAutoConnectRunning == TRUE)
#ifdef BT_APCLI_SUPPORT
		|| (pAd->ApCfg.ApCliAutoBWBTSupport == TRUE)
#endif
		)
#endif
		{
			ULONG Bssidx = 0;

			Bssidx = BssTableSearch(ScanTab, pApCliEntry->MlmeAux.Bssid, pApCliEntry->wdev.channel);

			if (Bssidx != BSS_NOT_FOUND) {
#ifdef APCLI_AUTO_BW_TMP /* should be removed after apcli auto-bw is applied */
				UCHAR ret = ApCliAutoConnectBWAdjust(pAd, &pApCliEntry->wdev, &ScanTab->BssEntry[Bssidx]);

				MTWF_LOG(DBG_CAT_CLIENT, CATCLIENT_APCLI, DBG_LVL_TRACE, ("%s[%d]Bssidx:%lu\n", __func__, __LINE__, Bssidx));

				if (ScanTab->BssEntry[Bssidx].SsidLen)
					MTWF_LOG(DBG_CAT_CLIENT, CATCLIENT_APCLI, DBG_LVL_TRACE, ("Root AP SSID: %s\n", ScanTab->BssEntry[Bssidx].Ssid));

				if (ret != AUTO_BW_PARAM_ERROR)
					isGoingToConnect = TRUE;

				if (ret == AUTO_BW_NEED_TO_ADJUST)
#endif /* APCLI_AUTO_BW_TMP */
				{
					MTWF_LOG(DBG_CAT_CLIENT, CATCLIENT_APCLI, DBG_LVL_TRACE, ("Switch to channel :%d\n", ScanTab->BssEntry[Bssidx].Channel));
					rtmp_set_channel(pAd, &pApCliEntry->wdev, ScanTab->BssEntry[Bssidx].Channel);
					isGoingToConnect = TRUE;
				}
			} else
				MTWF_LOG(DBG_CAT_CLIENT, CATCLIENT_APCLI, DBG_LVL_ERROR, ("%s[%d]Can not find BssEntry\n", __func__, __LINE__));
		} else
#endif /* APCLI_AUTO_CONNECT_SUPPORT */
		{
#ifndef APCLI_CFG80211_SUPPORT
			isGoingToConnect = TRUE;
#endif
		}
	}

#endif /* APCLI_SUPPORT */
	sync_fsm_state_transition(wdev, SYNC_FSM_IDLE);

	if (isGoingToConnect)
		Status = MLME_SUCCESS;
	else
		Status = MLME_REJ_TIMEOUT;

	cntl_join_start_conf(Elem->wdev, Status);

}


static VOID sync_fsm_scan_timeout_action(struct _RTMP_ADAPTER *pAd, MLME_QUEUE_ELEM *Elem)
{
	struct wifi_dev *wdev = Elem->wdev;
	SCAN_CTRL *ScanCtrl = get_scan_ctrl_by_wdev(pAd, wdev);
	SCAN_ACTION_INFO scan_action_info = {0};
	BOOLEAN ap_scan = TRUE;
#ifdef OFFCHANNEL_SCAN_FEATURE
	OFFCHANNEL_SCAN_MSG Rsp;
	UCHAR bandidx = 0;
#endif
#ifdef CONFIG_STA_SUPPORT
	PSTA_ADMIN_CONFIG pStaCfg = GetStaCfgByWdev(pAd, wdev);

	if (!pStaCfg) {
#ifndef CONFIG_APSTA_MIXED_SUPPORT
		return;
#endif /* !CONFIG_APSTA_MIXED_SUPPORT */
	} else
		ap_scan = FALSE;

	/*
		To prevent data lost.
		Send an NULL data with turned PSM bit on to current associated AP when SCAN in the channel where
		associated AP located.
	*/
	if (pStaCfg && /* snowpin for ap/sta */
		(wdev->channel == ScanCtrl->Channel) &&
		(ScanCtrl->ScanType == SCAN_ACTIVE) &&
		(INFRA_ON(pStaCfg)) &&
		STA_STATUS_TEST_FLAG(pStaCfg, fSTA_STATUS_MEDIA_STATE_CONNECTED)) {
		PMAC_TABLE_ENTRY pMacEntry = GetAssociatedAPByWdev(pAd, &pStaCfg->wdev);

		RTMPSendNullFrame(pAd, pMacEntry, pAd->CommonCfg.TxRate,
						  (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_WMM_INUSED) ? TRUE : FALSE),
						  PWR_SAVE);
	}

#endif /* CONFIG_STA_SUPPORT */
#ifdef OFFCHANNEL_SCAN_FEATURE
		bandidx = HcGetBandByWdev(wdev);

		if (ScanCtrl->state ==	OFFCHANNEL_SCAN_START) {
			MTWF_LOG(DBG_CAT_CLIENT, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
					("%s	pAd->ScanCtrl.CurrentGivenChan_Index = %d\n", __func__, ScanCtrl->CurrentGivenChan_Index));
			/* Last channel to scan from list */
			if ((ScanCtrl->Num_Of_Channels	- ScanCtrl->CurrentGivenChan_Index) == 1) {
				MTWF_LOG(DBG_CAT_CLIENT, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
						("[%s][%d] Num_of_channel = %d scanning complete\n", __func__, __LINE__, ScanCtrl->Num_Of_Channels));
				ScanCtrl->Channel = 0;
				ScanCtrl->state = OFFCHANNEL_SCAN_COMPLETE;
			}
		} else
#endif
				ScanCtrl->Channel = scan_find_next_channel(pAd, ScanCtrl, ScanCtrl->Channel);
#ifdef CONFIG_AP_SUPPORT

	/* snowpin for ap/sta IF_DEV_CONFIG_OPMODE_ON_AP(pAd) */
	if (ap_scan) {
		UCHAR BandIdx = HcGetBandByWdev(wdev);
		/*
			iwpriv set auto channel selection
			update the current index of the channel
		*/
#ifndef OFFCHANNEL_SCAN_FEATURE
		if (pAd->ApCfg.bAutoChannelAtBootup[BandIdx] == TRUE) {
#endif
			CHANNEL_CTRL *pChCtrl = hc_get_channel_ctrl(pAd->hdev_ctrl, BandIdx);
			/* update current channel info */
			UpdateChannelInfo(pAd, pAd->ApCfg.current_channel_index, pAd->ApCfg.AutoChannelAlg[BandIdx], wdev);
#ifdef OFFCHANNEL_SCAN_FEATURE
			if (ScanCtrl->state == OFFCHANNEL_SCAN_START) {
				MTWF_LOG(DBG_CAT_CLIENT, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
						("[%s] channel no : %d : obss time :%d\n",
						 __func__, pAd->ChannelInfo.ChannelNo,
						 pAd->ChannelInfo.ChStats.Obss_Time));
				memcpy(Rsp.ifrn_name, ScanCtrl->if_name, IFNAMSIZ);
				Rsp.Action = OFFCHANNEL_INFO_RSP;
				Rsp.data.channel_data.channel_busy_time = pAd->ChannelInfo.chanbusytime[pAd->ApCfg.current_channel_index];
				Rsp.data.channel_data.NF = pAd->ChannelInfo.AvgNF;
				Rsp.data.channel_data.channel = pAd->ChannelInfo.ChannelNo;
				Rsp.data.channel_data.tx_time = pAd->ChannelInfo.ChStats.Tx_Time;
				Rsp.data.channel_data.rx_time = pAd->ChannelInfo.ChStats.Rx_Time;
				Rsp.data.channel_data.obss_time = pAd->ChannelInfo.ChStats.Obss_Time;
				Rsp.data.channel_data.channel_idx = pAd->ApCfg.current_channel_index;
				/* This value to be used by application to calculate  channel busy percentage */
				Rsp.data.channel_data.actual_measured_time = ScanCtrl->ScanTimeActualDiff;
#ifdef MAP_R2
				if (IS_MAP_ENABLE(pAd) && IS_MAP_R2_ENABLE(pAd)) {
					asic_update_mib_bucket(pAd);
					Update_Mib_Bucket_for_map(pAd);
					Rsp.data.channel_data.edcca = pAd->OneSecMibBucket.EDCCAtime[ScanCtrl->BandIdx];
				}
#endif

				RtmpOSWrielessEventSend(
					pAd->net_dev,
					RT_WLAN_EVENT_CUSTOM,
					OID_OFFCHANNEL_INFO,
					NULL,
					(UCHAR *) &Rsp,
					sizeof(OFFCHANNEL_SCAN_MSG));
				ScanCtrl->ScanTime[ScanCtrl->CurrentGivenChan_Index] = 0;
				/* Scan complete increment index to start the next channel */
				ScanCtrl->CurrentGivenChan_Index++;
				/* Reinitialize the Scan parameters for the next offchannel */
				ScanCtrl->ScanType = ScanCtrl->Offchan_Scan_Type[ScanCtrl->CurrentGivenChan_Index];
				ScanCtrl->Channel  = ScanCtrl->ScanGivenChannel[ScanCtrl->CurrentGivenChan_Index];
				MTWF_LOG(DBG_CAT_CLIENT, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
						("[%s][%d]:Next OFFChannel scan for : %d:Scan type =%d from given list\n",
						 __func__, __LINE__, ScanCtrl->Channel, ScanCtrl->ScanType));
				pAd->ChannelInfo.bandidx = HcGetBandByChannel(pAd, ScanCtrl->Channel);
				if (ScanCtrl->Channel) {
					pAd->ChannelInfo.ChannelNo = ScanCtrl->Channel;
				}
			}
			/* move to next channel */
			if (ScanCtrl->state == OFFCHANNEL_SCAN_START)
#endif
			pAd->ApCfg.current_channel_index++;

			if (pAd->ApCfg.current_channel_index < pChCtrl->ChListNum)
				pAd->ApCfg.AutoChannel_Channel = pChCtrl->ChList[pAd->ApCfg.current_channel_index].Channel;
#ifndef OFFCHANNEL_SCAN_FEATURE
		}
#endif
	}

#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT
#ifdef MT_MAC_BTCOEX

	if (pStaCfg &&
		pStaCfg->MlmeAux.OldChannel <= 14 &&
		pStaCfg->MlmeAux.Channel > 14) {
		if (IS_MT76x6(pAd) || IS_MT7637(pAd)) {
			MT7636MLMEHook(pAd, MT7636_WLAN_SCANDONE_2G, 0);
			MT7636MLMEHook(pAd, MT7636_WLAN_SCANREQEST_5G, 0);
		}
	}

#endif /*MT_MAC_BTCOEX*/
#endif /* CONFIG_STA_SUPPORT */

	if (scan_next_channel(pAd, wdev, &scan_action_info) == FALSE) {
		MTWF_LOG(DBG_CAT_CLIENT, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("SYNC - MlmeScanReqAction before Startup\n"));
		sync_fsm_state_transition(wdev, SYNC_FSM_IDLE);
		cntl_scan_conf(wdev, MLME_FAIL_NO_RESOURCE);
	} else {
		if (scan_action_info.isScanDone) {
			if (scan_action_info.isScanPending)
				sync_fsm_state_transition(wdev, SYNC_FSM_PENDING);
			else {
				sync_fsm_state_transition(wdev, SYNC_FSM_IDLE);
			}
#ifdef WSC_INCLUDED
			/*Change the ctrl state from Idle to wait_sync for handling scan_conf state in WPS*/
			if ((wdev->WscControl.WscConfMode != WSC_DISABLE)
					&& (wdev->WscControl.bWscTrigger == TRUE)
					&& (wdev->WscControl.WscMode == WSC_PIN_MODE)
					&& (wdev->cntl_machine.CurrState == CNTL_IDLE)) {
				cntl_fsm_state_transition(wdev, CNTL_WAIT_SYNC, __func__);
			}
#endif /* WSC_INCLUDED */
			sync_fsm_scan_complete_action(pAd, Elem, scan_action_info.isScanPending);
		}
	}
}

static VOID sync_fsm_scan_req_action(struct _RTMP_ADAPTER *pAd, MLME_QUEUE_ELEM *Elem)
{
	BOOLEAN Cancelled;
	UCHAR Ssid[MAX_LEN_OF_SSID], SsidLen, BssType;
	UCHAR ScanType = SCAN_TYPE_MAX;
	USHORT Status = MLME_SUCCESS;
	struct wifi_dev *wdev = Elem->wdev;
	SCAN_CTRL *ScanCtrl = get_scan_ctrl_by_wdev(pAd, wdev);
	SCAN_INFO *ScanInfo = &wdev->ScanInfo;
	SCAN_ACTION_INFO scan_action_info = {0};
#ifdef CONFIG_AP_SUPPORT
	UCHAR BssIdx = 0;
#endif /* CONFIG_AP_SUPPORT */
	MTWF_LOG(DBG_CAT_CLIENT, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("SYNC - %s:[%d] LAST_CH: %d, BAND: %d\n",
			 __func__, __LINE__, ScanInfo->LastScanChannel, ScanCtrl->BandIdx));
	/*ASSERT(wdev);*/

	if (!wdev)
		return;


	/*
	Check the total scan tries for one single OID command
	If this is the CCX 2.0 Case, skip that!
	*/
	if (!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_START_UP)) {
		MTWF_LOG(DBG_CAT_CLIENT, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
				 ("SYNC - MlmeScanReqAction before Startup\n"));
		Status = MLME_FAIL_NO_RESOURCE;
		goto cntl_res_err;
	}


	if (MlmeScanReqSanity(pAd, Elem->Msg, Elem->MsgLen, &BssType, (PCHAR)Ssid, &SsidLen, &ScanType)) {
		AsicDisableSync(pAd, HW_BSSID_0);
#ifdef CONFIG_AP_SUPPORT

		/* Disable beacon tx for all BSS */
		for (BssIdx = 0; BssIdx < pAd->ApCfg.BssidNum; BssIdx++) {
			wdev = &pAd->ApCfg.MBSSID[BssIdx].wdev;

			if (wdev->bAllowBeaconing)
				UpdateBeaconHandler(pAd, wdev, BCN_UPDATE_DISABLE_TX);
		}

#endif /* CONFIG_AP_SUPPORT */
		wdev = Elem->wdev;
		NdisGetSystemUpTime(&ScanInfo->LastScanTime);
		ScanInfo->ScanChannelCnt = 0;
		RTMPCancelTimer(&ScanCtrl->ScanTimer, &Cancelled);
		sync_fsm_enqueue_req(wdev);
		ScanCtrl->BssType = BssType;
		ScanCtrl->ScanType = ScanType;
		ScanCtrl->SsidLen = SsidLen;
		NdisMoveMemory(ScanCtrl->Ssid, Ssid, SsidLen);
#ifdef OFFCHANNEL_SCAN_FEATURE
		if (ScanCtrl->ScanGivenChannel[ScanCtrl->CurrentGivenChan_Index]) {
				ScanCtrl->Channel = ScanCtrl->ScanGivenChannel[ScanCtrl->CurrentGivenChan_Index];
				MTWF_LOG(DBG_CAT_CLIENT, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
						("[%s][%d] start offchannel scan on %d : channel list index = %d\n",
						 __func__, __LINE__, ScanCtrl->Channel, ScanCtrl->CurrentGivenChan_Index));
				ScanCtrl->state = OFFCHANNEL_SCAN_START;
		} else
#endif
		ScanCtrl->Channel = scan_find_next_channel(pAd, ScanCtrl, wdev->ScanInfo.LastScanChannel);
#ifdef OFFCHANNEL_SCAN_FEATURE
	{
		if (ScanCtrl->state == OFFCHANNEL_SCAN_START)
			pAd->MsMibBucket.Enabled = FALSE;
	}
#endif
#ifdef CONFIG_AP_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_AP(pAd) {
			UCHAR BandIdx = HcGetBandByWdev(wdev);
			CHANNEL_CTRL *pChCtrl = hc_get_channel_ctrl(pAd->hdev_ctrl, BandIdx);
			if (pAd->ApCfg.bAutoChannelAtBootup[BandIdx] == TRUE) { /* iwpriv set auto channel selection */
				APAutoChannelInit(pAd, wdev);
				pAd->ApCfg.AutoChannel_Channel = pChCtrl->ChList[0].Channel;
			}
		}
#endif /* CONFIG_AP_SUPPORT */


#ifdef CONFIG_STA_SUPPORT
		/* YF_THINK: Shall check all the STA wdev connected but not in its channel then send Null Pkt */
		/*
		 *	  To prevent data lost.
		 *	  Send an NULL data with turned PSM bit on to current associated AP before SCAN progress.
		 *	  And should send an NULL data with turned PSM bit off to AP, when scan progress done
		 */
		if (wdev->wdev_type == WDEV_TYPE_STA) {
			PSTA_ADMIN_CONFIG pStaCfg = GetStaCfgByWdev(pAd, wdev);

			if (STA_STATUS_TEST_FLAG(pStaCfg, fSTA_STATUS_MEDIA_STATE_CONNECTED) && (INFRA_ON(pStaCfg))) {
				MAC_TABLE_ENTRY *pEntry = GetAssociatedAPByWdev(pAd, &pStaCfg->wdev);

				if (pStaCfg->PwrMgmt.bDoze) {
					MTWF_LOG(DBG_CAT_CLIENT, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:: H/W is in DOZE, wake up H/W before scanning\n",
							 __func__));
					RTMP_FORCE_WAKEUP(pAd, pStaCfg);
				}

				RTMPSendNullFrame(pAd,
								  pEntry,
								  pAd->CommonCfg.TxRate,
								  (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_WMM_INUSED) ? TRUE : FALSE),
								  PWR_SAVE);
				MTWF_LOG(DBG_CAT_CLIENT, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
						 ("MlmeScanReqAction -- Send PSM Data frame for off channel RM, SCAN_IN_PROGRESS=%d!\n",
						  RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_BSS_SCAN_IN_PROGRESS)));
				OS_WAIT(20);
			}
		}
#endif

		RTMPSendWirelessEvent(pAd, IW_SCANNING_EVENT_FLAG, NULL, BSS0, 0);
#ifdef RT_CFG80211_P2P_CONCURRENT_DEVICE

		if (pAd->StaCfg[MAIN_MBSSID].Valid && RTMP_CFG80211_VIF_P2P_CLI_ON(pAd)) {
			MTWF_LOG(DBG_CAT_CLIENT, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("CFG80211_NULL: PWR_SAVE IN ScanStart\n"));
			RT_CFG80211_P2P_CLI_SEND_NULL_FRAME(pAd, PWR_SAVE);
		}

#endif /* RT_CFG80211_P2P_CONCURRENT_DEVICE */
#ifdef CONFIG_STA_SUPPORT
#ifdef DOT11_N_SUPPORT
#ifdef DOT11N_DRAFT3
		/* Before scan, reset trigger event table. */
		TriEventInit(pAd);
#endif /* DOT11N_DRAFT3 */
#endif /* DOT11_N_SUPPORT */
#endif /* CONFIG_STA_SUPPORT */
#ifdef COEX_SUPPORT

		if (IS_MT76x6(pAd) || IS_MT7637(pAd) || IS_MT7622(pAd)) {
			if (ScanCtrl->Channel <= 14)
				MT76xxMLMEHook(pAd, MT76xx_WLAN_SCANREQEST_2G, 0);
			else
				MT76xxMLMEHook(pAd, MT76xx_WLAN_SCANREQEST_5G, 0);
		}

#endif /* COEX_SUPPORT */
		sync_fsm_state_transition(wdev, SYNC_FSM_LISTEN);

		if (scan_next_channel(pAd, wdev, &scan_action_info) == TRUE) {
			if (scan_action_info.isScanDone) {
				sync_fsm_state_transition(wdev, SYNC_FSM_IDLE);
				Status = MLME_FAIL_NO_RESOURCE;
			}
		} else {
			sync_fsm_state_transition(wdev, SYNC_FSM_IDLE);
			Status = MLME_FAIL_NO_RESOURCE;
		}
	} else {
		MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s - MlmeScanReqAction() sanity check fail. BUG!!!\n", __func__));
		Status = MLME_INVALID_FORMAT;
	}

cntl_res_err:

	if (Status != MLME_SUCCESS)
		cntl_scan_conf(wdev, MLME_FAIL_NO_RESOURCE);

}

static VOID sync_fsm_peer_response_scan_action(struct _RTMP_ADAPTER *pAd, MLME_QUEUE_ELEM *Elem)
{
	PFRAME_802_11 pFrame;
	UCHAR *VarIE = NULL;
	USHORT LenVIE;
	NDIS_802_11_VARIABLE_IEs *pVIE = NULL;
	CHAR RealRssi = -127;
	BCN_IE_LIST *ie_list = NULL;
	/* UCHAR BandIdx = HcGetBandByChannel(pAd, Elem->Channel); */
	struct wifi_dev *wdev = Elem->wdev; /* pAd->ScanCtrl[BandIdx].ScanReqwdev; */
	UCHAR BandIdx = BAND0; /* HcGetBandByChannel(pAd, Elem->Channel); */
	SCAN_CTRL *ScanCtrl = NULL;
	PSTA_ADMIN_CONFIG pStaCfg = NULL;
	BSS_TABLE *ScanTab = NULL;
	CHANNEL_CTRL *pChCtrl;
	RSSI_SAMPLE rssi_sample;
#ifdef APCLI_SUPPORT
#ifdef CONFIG_MAP_SUPPORT
	int index_map = 0;
#endif
#endif

	if (!wdev) {
		MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Band:%d, CH:%d, STATE: %s\n",
				 BandIdx, Elem->Channel, SYNC_FSM_STATE_STR[pAd->ScanCtrl[BandIdx].SyncFsm.CurrState]));
		return;
	}

	BandIdx = HcGetBandByWdev(wdev);
	pChCtrl = hc_get_channel_ctrl(pAd->hdev_ctrl, BandIdx);
	ScanCtrl = get_scan_ctrl_by_wdev(pAd, wdev);
	ScanTab = get_scan_tab_by_wdev(pAd, wdev);
#ifdef CONFIG_STA_SUPPORT

	if (wdev->wdev_type == WDEV_TYPE_STA) {
		pStaCfg = GetStaCfgByWdev(pAd, wdev);
		/*ASSERT(pStaCfg);*/

		if (!pStaCfg)
			return;
	}

#endif /* CONFIG_STA_SUPPORT */

	if (ScanCtrl->ScanReqwdev && (wdev != ScanCtrl->ScanReqwdev)) {
		MTWF_LOG(DBG_CAT_CLIENT, DBG_SUBCAT_ALL, DBG_LVL_INFO,
				 ("[%s] <==============================================> [%s]\n",
				  ScanCtrl->ScanReqwdev->if_dev->name, wdev->if_dev->name));
	}

	os_alloc_mem(pAd, (UCHAR **)&ie_list, sizeof(BCN_IE_LIST));

	if (!ie_list) {
		MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s: Alloc memory for ie_list fail!!!\n", __func__));
		return;
	}

	NdisZeroMemory((UCHAR *)ie_list, sizeof(BCN_IE_LIST));
	/* allocate memory */
	os_alloc_mem(NULL, (UCHAR **)&VarIE, MAX_VIE_LEN);

	if (VarIE == NULL) {
		MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s: Allocate memory fail!!!\n", __func__));
		goto LabelErr;
	}

	pFrame = (PFRAME_802_11) Elem->Msg;
	/* Init Variable IE structure */
	pVIE = (PNDIS_802_11_VARIABLE_IEs) VarIE;
	pVIE->Length = 0;

	if (PeerBeaconAndProbeRspSanity(pAd, wdev,
									Elem->Msg, Elem->MsgLen, Elem->Channel,
									ie_list, &LenVIE, pVIE, FALSE, FALSE)) {
		ULONG Idx;
		MAC_TABLE_ENTRY *pEntry = NULL;
		UCHAR Channel = 0;
#if defined(WIFI_REGION32_HIDDEN_SSID_SUPPORT) || defined(CONFIG_MAP_SUPPORT)
		CHAR SsidAllZero = 0;
		CHAR k = 0;

		/* check ssid values, assume it's all zero first */
		if (ie_list->SsidLen != 0)
			SsidAllZero = 1;

		for (k = 0 ; k < ie_list->SsidLen ; k++) {
			if (ie_list->Ssid[k] != 0) {
				SsidAllZero = 0;
				break;
			}
		}

#endif /* WIFI_REGION32_HIDDEN_SSID_SUPPORT */
		Idx = BssTableSearch(ScanTab, &ie_list->Bssid[0], ie_list->Channel);
#if defined(WIFI_REGION32_HIDDEN_SSID_SUPPORT) || defined(CONFIG_MAP_SUPPORT)

		if (Idx != BSS_NOT_FOUND && ie_list->SsidLen != 0 && SsidAllZero == 0)
#else
		if (Idx != BSS_NOT_FOUND)
#endif /* WIFI_REGION32_HIDDEN_SSID_SUPPORT */
			;/* RealRssi = ScanTab->BssEntry[Idx].Rssi; this assignment is no use*/
#ifdef CONFIG_MAP_SUPPORT
		else {
			if (!wdev) {
				 MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
					("wdev is NULL return\n"));
				 return;
			}
			if ((IS_MAP_TURNKEY_ENABLE(pAd)) &&
			(((pAd->CommonCfg.bIEEE80211H == 1) &&
				RadarChannelCheck(pAd, ScanCtrl->Channel))) &&
				(wdev->MAPCfg.FireProbe_on_DFS == FALSE)) {
					wdev->MAPCfg.FireProbe_on_DFS = TRUE;
					while (index_map < MAX_BH_PROFILE_CNT) {
					if (wdev->MAPCfg.scan_bh_ssids.scan_SSID_val[index_map].SsidLen > 0) {
						scan_extra_probe_req(pAd, OPMODE_AP, SCAN_ACTIVE, wdev,
							wdev->MAPCfg.scan_bh_ssids.scan_SSID_val[index_map].ssid,
							 wdev->MAPCfg.scan_bh_ssids.scan_SSID_val[index_map].SsidLen);
					}
					index_map++;
				}
			}
		}
#endif

#ifdef WIFI_REGION32_HIDDEN_SSID_SUPPORT
		else
			do {
				UCHAR SsidLen = 0;

				MTWF_LOG(DBG_CAT_CLIENT, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s::CountryRegion %d\n", __func__,
						 pAd->CommonCfg.CountryRegion));
				MTWF_LOG(DBG_CAT_CLIENT, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s::Channel %d\n", __func__, ie_list->Channel));
				MTWF_LOG(DBG_CAT_CLIENT, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s::ssid length %d\n", __func__, ie_list->SsidLen));

				if (((pAd->CommonCfg.CountryRegion & 0x7f) == REGION_32_BG_BAND)
					&& ((ie_list->Channel == 12) || (ie_list->Channel == 13))
					&& ((ie_list->SsidLen == 0) || (SsidAllZero == 1))) {
					HEADER_802_11	Hdr80211;
					PUCHAR			pOutBuffer = NULL;
					NDIS_STATUS	NStatus;
					ULONG			FrameLen = 0;

					NStatus = MlmeAllocateMemory(pAd, &pOutBuffer);  /* Get an unused nonpaged memory */

					if (NStatus != NDIS_STATUS_SUCCESS) {
						MTWF_LOG(DBG_CAT_CLIENT, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("PeerBeaconAtScanAction() allocate memory fail\n"));
						break;
					}

					SsidLen = pStaCfg->MlmeAux.SsidLen;
#ifdef CONFIG_STA_SUPPORT
					IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
					MgtMacHeaderInit(pAd, &Hdr80211, SUBTYPE_PROBE_REQ, 0, &ie_list->Bssid[0],
									 pStaCfg->wdev.if_addr,
									 &ie_list->Bssid[0]);
#endif /* CONFIG_STA_SUPPORT // */
					MakeOutgoingFrame(pOutBuffer,				&FrameLen,
									  sizeof(HEADER_802_11),	&Hdr80211,
									  1,						&SsidIe,
									  1,						&SsidLen,
									  SsidLen,					pStaCfg->MlmeAux.Ssid,
									  END_OF_ARGS);
					FrameLen += build_support_rate_ie(&pStaCfg->wdev, pStaCfg->wdev.rate.SupRate,
									pStaCfg->wdev.rate.SupRateLen, pOutBuffer + FrameLen);

					FrameLen += build_support_ext_rate_ie(&pStaCfg->wdev, pStaCfg->wdev.rate.SupRateLen,
							pStaCfg->wdev.rate.ExtRate, pStaCfg->wdev.rate.ExtRateLen, pOutBuffer + FrameLen);

					MiniportMMRequest(pAd, 0, pOutBuffer, FrameLen);
					MlmeFreeMemory(pOutBuffer);
				}
			} while (0);

#endif  /* WIFI_REGION32_HIDDEN_SSID_SUPPORT */
		pEntry = MacTableLookup(pAd, ie_list->Addr2);/* Found the pEntry from Peer Bcn Content */
		rssi_sample.AvgRssi[0] = Elem->rssi_info.raw_rssi[0];
		rssi_sample.AvgRssi[1] = Elem->rssi_info.raw_rssi[1];
		rssi_sample.AvgRssi[2] = Elem->rssi_info.raw_rssi[2];
		rssi_sample.AvgRssi[3] = Elem->rssi_info.raw_rssi[3];
		RealRssi = rtmp_avg_rssi(pAd, &rssi_sample);

		if (ie_list->Channel > 14)
			Channel = HcGetChannelByRf(pAd, RFIC_5GHZ);
		else
			Channel = HcGetChannelByRf(pAd, RFIC_24GHZ);

		{
			/* ignore BEACON not in this channel */
			if (ie_list->Channel != ScanCtrl->Channel
#ifdef DOT11_N_SUPPORT
#ifdef DOT11N_DRAFT3
				&& (pAd->CommonCfg.bOverlapScanning == FALSE)
#endif /* DOT11N_DRAFT3 */
#endif /* DOT11_N_SUPPORT */
			   )
				goto __End_Of_APPeerBeaconAtScanAction;
		}

#ifdef DOT11_N_SUPPORT

		if ((RealRssi > OBSS_BEACON_RSSI_THRESHOLD) &&
			(ie_list->cmm_ies.ht_cap.HtCapInfo.Forty_Mhz_Intolerant)) { /* || (HtCapabilityLen == 0))) */
			if ((ScanCtrl->ScanType == SCAN_2040_BSS_COEXIST) &&
				IF_COMBO_HAVE_AP_STA(pAd) &&
				(wdev->wdev_type == WDEV_TYPE_STA)) {
				/* STA/APCLI will decide 40->20 or not in later action frame, ignore this judge in peer_beacon */
				MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s:Ignore BW 40->20\n", __func__));
			} else {
#ifdef CONFIG_AP_SUPPORT

				if (wdev->wdev_type == WDEV_TYPE_AP)
					Handle_BSS_Width_Trigger_Events(pAd, Channel);

#endif /* CONFIG_AP_SUPPORT */
			}
		}

#endif /* DOT11_N_SUPPORT */
#ifdef CONFIG_AP_SUPPORT
#ifdef IDS_SUPPORT

		/* Conflict SSID detection */
		if (ie_list->Channel == Channel)
			RTMPConflictSsidDetection(pAd, ie_list->Ssid, ie_list->SsidLen,
									  Elem->rssi_info.raw_rssi[0],
									  Elem->rssi_info.raw_rssi[1],
									  Elem->rssi_info.raw_rssi[2]);

#endif /* IDS_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */
#ifdef DOT11_N_SUPPORT

		if (HAS_HT_CAPS_EXIST(ie_list->cmm_ies.ie_exists) || HAS_PREN_CAPS_EXIST(ie_list->cmm_ies.ie_exists))
			SET_HT_CAPS_EXIST(ie_list->cmm_ies.ie_exists);

#ifdef DOT11N_DRAFT3

		/* Check if this scan channel is the effeced channel */
		if ( pStaCfg && (
#ifdef APCLI_SUPPORT
				 APCLI_IF_UP_CHECK(pAd, wdev->func_idx) ||
#endif /* APCLI_SUPPORT */
#ifdef CONFIG_STA_SUPPORT
				 INFRA_ON(pStaCfg) ||
#endif /* CONFIG_STA_SUPPORT */
				 FALSE))
			build_trigger_event_table(pAd, Elem, ie_list);

#endif /* DOT11N_DRAFT3 */
#endif /* DOT11_N_SUPPORT */
		Idx = BssTableSetEntry(pAd, wdev, ScanTab, ie_list, RealRssi, LenVIE, pVIE);
#ifdef DOT11_N_SUPPORT
#ifdef DOT11N_DRAFT3
		{
			/* Check if this scan channel is the effeced channel */
			if ((pAd->CommonCfg.bBssCoexEnable == TRUE)
				&& ((ie_list->Channel > 0) && (ie_list->Channel <= 14))) {
				int chListIdx;

				/*
				First we find the channel list idx by the channel number
				*/
				for (chListIdx = 0; chListIdx < pChCtrl->ChListNum; chListIdx++) {
					if (ie_list->Channel == pChCtrl->ChList[chListIdx].Channel)
						break;
				}

				if (chListIdx < pChCtrl->ChListNum) {

					/*
						If this channel is effected channel for the 20/40 coex operation. Check the related IEs.
					*/
					if (pChCtrl->ChList[chListIdx].bEffectedChannel == TRUE) {
						UCHAR RegClass;
						OVERLAP_BSS_SCAN_IE BssScan;
						/* Read Beacon's Reg Class IE if any. */
						PeerBeaconAndProbeRspSanity2(pAd, Elem->Msg, Elem->MsgLen, &BssScan, &RegClass);
#ifdef CONFIG_STA_SUPPORT
						/* printk("\x1b[31m TriEventTableSetEntry \x1b[m\n"); */
						TriEventTableSetEntry(pAd, &pAd->CommonCfg.TriggerEventTab,
								ie_list->Bssid, &ie_list->cmm_ies.ht_cap,
								HAS_HT_CAPS_EXIST(ie_list->cmm_ies.ie_exists), RegClass,
								ie_list->Channel);
#endif
					}
				}
			}
		}
#endif /* DOT11N_DRAFT3 */
#endif /* DOT11_N_SUPPORT */

		if (Idx != BSS_NOT_FOUND) {
			BSS_ENTRY *pBssEntry = &ScanTab->BssEntry[Idx];

			NdisMoveMemory(pBssEntry->PTSF, &Elem->Msg[24], 4);
			NdisMoveMemory(&pBssEntry->TTSF[0], &Elem->TimeStamp.u.LowPart, 4);
			NdisMoveMemory(&pBssEntry->TTSF[4], &Elem->TimeStamp.u.LowPart, 4);
			pBssEntry->MinSNR = Elem->Signal % 10;

			if (pBssEntry->MinSNR == 0)
				pBssEntry->MinSNR = -5;

			NdisMoveMemory(pBssEntry->MacAddr, &ie_list->Addr2[0], MAC_ADDR_LEN);

			if ((pFrame->Hdr.FC.SubType == SUBTYPE_PROBE_RSP) && (LenVIE != 0)) {
				pBssEntry->VarIeFromProbeRspLen = 0;

				if (pBssEntry->pVarIeFromProbRsp) {
					pBssEntry->VarIeFromProbeRspLen = LenVIE;
					RTMPZeroMemory(pBssEntry->pVarIeFromProbRsp, MAX_VIE_LEN);
					RTMPMoveMemory(pBssEntry->pVarIeFromProbRsp, pVIE, LenVIE);
				}
			}
		}

#ifdef RT_CFG80211_SUPPORT

		if (RTMPEqualMemory(ie_list->Ssid, "DIRECT-", 7))
			MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s P2P_SCANNING: %s [%lu], channel =%d\n"
					 , __func__, ie_list->Ssid, Idx, Elem->Channel));

		/* Determine primary channel by IE's DSPS rather than channel of received frame */
		if (ie_list->Channel != 0)
			Elem->Channel = ie_list->Channel;

		MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s: Update the SSID %s in Kernel Table, Elem->Channel=%u\n",
				 __func__, ie_list->Ssid, Elem->Channel));
		RT_CFG80211_SCANNING_INFORM(pAd, Idx, Elem->Channel, (UCHAR *)Elem->Msg, Elem->MsgLen, RealRssi);
#endif /* RT_CFG80211_SUPPORT */
#ifdef MWDS

		if (Idx != BSS_NOT_FOUND) {
			ScanTab->BssEntry[Idx].bSupportMWDS = FALSE;

			if (ie_list->vendor_ie.mtk_cap_found) {
				BOOLEAN bSupportMWDS = FALSE;

				if (ie_list->vendor_ie.support_mwds)
					bSupportMWDS = TRUE;

				if (ScanTab->BssEntry[Idx].bSupportMWDS != bSupportMWDS)
					ScanTab->BssEntry[Idx].bSupportMWDS = bSupportMWDS;
			}
		}

#endif /* MWDS */
#ifdef APCLI_SUPPORT
#ifdef WH_EVENT_NOTIFIER

		if (pFrame && (pFrame->Hdr.FC.SubType == SUBTYPE_PROBE_RSP)) {
			EventHdlr pEventHdlrHook = NULL;

			pEventHdlrHook = GetEventNotiferHook(WHC_DRVEVNT_AP_PROBE_RSP);

			if (pEventHdlrHook && ScanCtrl->ScanReqwdev)
				pEventHdlrHook(pAd, ScanCtrl->ScanReqwdev, ie_list, Elem);
		}

#endif /* WH_EVENT_NOTIFIER */
#endif /* APCLI_SUPPORT */
	}

	/* sanity check fail, ignored */
__End_Of_APPeerBeaconAtScanAction:
	/*scan beacon in pastive */
#ifdef CONFIG_AP_SUPPORT

	/* snowpin for ap/sta IF_DEV_CONFIG_OPMODE_ON_AP(pAd) */
	if (wdev->wdev_type == WDEV_TYPE_AP) {
		if (ie_list->Channel == pAd->ApCfg.AutoChannel_Channel) {
			AUTO_CH_CTRL *pAutoChCtrl = HcGetAutoChCtrlbyBandIdx(pAd, BandIdx);

			if (AutoChBssSearchWithSSID(pAd, ie_list->Bssid, (PUCHAR)ie_list->Ssid,
						ie_list->SsidLen,
						ie_list->Channel, wdev) == BSS_NOT_FOUND)
				pAutoChCtrl->pChannelInfo->ApCnt[pAd->ApCfg.current_channel_index]++;

			AutoChBssInsertEntry(pAd, ie_list->Bssid, (CHAR *)ie_list->Ssid,
					ie_list->SsidLen, ie_list->Channel, ie_list->NewExtChannelOffset,
					RealRssi, wdev);
		}
	}

#endif /* CONFIG_AP_SUPPORT */
LabelErr:

	if (VarIE != NULL)
		os_free_mem(VarIE);

	if (ie_list != NULL)
		os_free_mem(ie_list);
}

static VOID sync_fsm_peer_request_idle_action(struct _RTMP_ADAPTER *pAd, MLME_QUEUE_ELEM *Elem)
{
	PEER_PROBE_REQ_PARAM ProbeReqParam = {  {0} };
	struct wifi_dev *wdev = NULL;
	struct sync_fsm_ops *fsm_ops;
#ifdef CONFIG_AP_SUPPORT
	UCHAR apidx = 0;
	BSS_STRUCT *mbss;
#endif /* CONFIG_AP_SUPPORT */
#ifdef WDS_SUPPORT

	/* if in bridge mode, no need to reply probe req. */
	if (pAd->WdsTab.Mode[HcGetBandByWdev(Elem->wdev)] == WDS_BRIDGE_MODE)
		return;

#endif /* WDS_SUPPORT */
#ifdef P2P_SUPPORT

	/* When enable P2P scan, there driver alway sent probe request.
	    But when user enter main page, LG request not sent probe response. */
	if (pAd->P2pCfg.bSentProbeRSP != TRUE)
		return;

#endif /* P2P_SUPPORT */

	if (PeerProbeReqSanity(pAd, Elem->Msg, Elem->MsgLen, &ProbeReqParam) == FALSE) {
		MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s():shiang! PeerProbeReqSanity failed!\n", __func__));
		return;
	}

#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
	{
		for (apidx = 0; apidx < pAd->ApCfg.BssidNum; apidx++) {
			mbss = &pAd->ApCfg.MBSSID[apidx];
			wdev = &mbss->wdev;

			if (!wdev->DevInfo.WdevActive)
				continue;

#ifdef DOT11V_MBSSID_SUPPORT
			if (IS_BSSID_11V_NON_TRANS(pAd, mbss, HcGetBandByWdev(wdev))) {
				MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_INFO,
						 ("%s(): wdev %d is Nontransmitted Bssid, ignore!!\n",
						  __func__, wdev->wdev_idx));
				continue;
			}
#endif /* DOT11V_MBSSID_SUPPORT */
			fsm_ops = (struct sync_fsm_ops *)wdev->sync_fsm_ops;
			/*ASSERT(fsm_ops);
			ASSERT(fsm_ops->tx_probe_response_allowed);*/

			if (fsm_ops && fsm_ops->tx_probe_response_allowed &&
				fsm_ops->tx_probe_response_allowed(pAd, wdev, &ProbeReqParam, Elem) == TRUE) {
				fsm_ops->tx_probe_response_xmit(pAd, wdev, &ProbeReqParam, Elem);
			}
		}
	}
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
	/* YF_THINK: shall check the wdev_type then go inside ? */
	IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
	{
		wdev = Elem->wdev;

		fsm_ops = (struct sync_fsm_ops *)wdev->sync_fsm_ops;
		/*ASSERT(fsm_ops);
		ASSERT(fsm_ops->tx_probe_response_allowed);*/

		if (fsm_ops && fsm_ops->tx_probe_response_allowed &&
			fsm_ops->tx_probe_response_allowed(pAd, wdev, &ProbeReqParam, Elem) == TRUE)
			fsm_ops->tx_probe_response_xmit(pAd, wdev, &ProbeReqParam, Elem);
	}
#endif /* CONFIG_STA_SUPPORT */
}

static VOID sync_fsm_peer_response_idle_action(struct _RTMP_ADAPTER *pAd, MLME_QUEUE_ELEM *Elem)
{
	UCHAR *VarIE = NULL;
	USHORT LenVIE;
	NDIS_802_11_VARIABLE_IEs *pVIE = NULL;
	BCN_IE_LIST *ie_list = NULL;
	struct wifi_dev *wdev = Elem->wdev;
	struct sync_fsm_ops *fsm_ops;
#ifdef CONFIG_STA_SUPPORT
	PSTA_ADMIN_CONFIG pStaCfg = NULL;
#endif /* CONFIG_STA_SUPPORT */
	RETURN_IF_PAD_NULL(pAd);
#ifdef CONFIG_ATE

	if (ATE_ON(pAd))
		return;

#endif /* CONFIG_ATE */
#ifdef CONFIG_STA_SUPPORT

	if (wdev->wdev_type == WDEV_TYPE_STA) {
		pStaCfg = GetStaCfgByWdev(pAd, wdev);

		if (!pStaCfg)
			return;

		if (!(INFRA_ON(pStaCfg) || ADHOC_ON(pAd)
			 ))
			return;
	}

#endif /* CONFIG_STA_SUPPORT */
	/* allocate memory */
	os_alloc_mem(NULL, (UCHAR **)&ie_list, sizeof(BCN_IE_LIST));

	if (ie_list == NULL) {
		MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s: Allocate ie_list fail!!!\n", __func__));
		goto LabelErr;
	}

	NdisZeroMemory(ie_list, sizeof(BCN_IE_LIST));
	/* Init Variable IE structure */
	os_alloc_mem(NULL, (UCHAR **)&VarIE, MAX_VIE_LEN);

	if (VarIE == NULL) {
		MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s: Allocate VarIE fail!!!\n", __func__));
		goto LabelErr;
	}

	pVIE = (PNDIS_802_11_VARIABLE_IEs) VarIE;
	pVIE->Length = 0;
	/* PeerBeaconAndProbeRspSanity() may overwrite ie_list->Channel if beacon or  probe resp contain IE_DS_PARM */
	ie_list->Channel = Elem->Channel;

	if (PeerBeaconAndProbeRspSanity(pAd, wdev, Elem->Msg, Elem->MsgLen, Elem->Channel,
									ie_list, &LenVIE, pVIE, FALSE, FALSE) == FALSE)
		goto LabelErr;

#ifdef DOT11_HE_AX
	if (HAS_HE_OP_EXIST(ie_list->cmm_ies.ie_exists))
		parse_he_bss_color_info(wdev, ie_list);
#endif

	fsm_ops = (struct sync_fsm_ops *)wdev->sync_fsm_ops;
	/*ASSERT(fsm_ops);
	ASSERT(fsm_ops->rx_peer_response_allowed);*/

	if (fsm_ops->rx_peer_response_allowed(pAd, wdev, ie_list, Elem) == TRUE)
		fsm_ops->rx_peer_response_updated(pAd, wdev, ie_list, Elem, pVIE, LenVIE);

LabelErr:

	if (VarIE != NULL)
		os_free_mem(VarIE);

	if (ie_list != NULL)
		os_free_mem(ie_list);

	return;
}

static VOID sync_fsm_peer_response_join_action(struct _RTMP_ADAPTER *pAd, MLME_QUEUE_ELEM *Elem)
{
	USHORT LenVIE;
	UCHAR *VarIE = NULL;
	NDIS_802_11_VARIABLE_IEs *pVIE = NULL;
	BCN_IE_LIST *ie_list = NULL;
	CHAR RealRssi = 0;
	ULONG Bssidx = BSS_NOT_FOUND;
	struct sync_fsm_ops *fsm_ops;
	RSSI_SAMPLE rssi_sample;
	struct wifi_dev *wdev = Elem->wdev;
	BOOLEAN isRecvJoinRsp = FALSE;
	SCAN_CTRL *ScanCtrl = get_scan_ctrl_by_wdev(pAd, wdev);
	BSS_TABLE *ScanTab = get_scan_tab_by_wdev(pAd, wdev);
#if defined(CONFIG_STA_SUPPORT) || defined(DOT11_HE_AX)
	PSTA_ADMIN_CONFIG pStaCfg = GetStaCfgByWdev(pAd, wdev);
#endif /* CONFIG_STA_SUPPORT */

	if (ScanCtrl->ScanReqwdev != wdev) {
		MTWF_LOG(DBG_CAT_CLIENT, CATCLIENT_APCLI, DBG_LVL_INFO, ("%s: %s is not in JOIN state (wdev %s issued scan)!\n",
				 __func__, wdev->if_dev->name, ScanCtrl->ScanReqwdev->if_dev->name));
		return;
	}

	/* Init Variable IE structure */
	os_alloc_mem(NULL, (UCHAR **)&VarIE, MAX_VIE_LEN);

	if (VarIE == NULL) {
		MTWF_LOG(DBG_CAT_CLIENT, CATCLIENT_APCLI, DBG_LVL_ERROR, ("%s: Allocate memory fail!!!\n", __func__));
		goto LabelErr;
	}

	pVIE = (PNDIS_802_11_VARIABLE_IEs) VarIE;
	pVIE->Length = 0;
	os_alloc_mem(NULL, (UCHAR **)&ie_list, sizeof(BCN_IE_LIST));

	if (ie_list == NULL) {
		MTWF_LOG(DBG_CAT_CLIENT, CATCLIENT_APCLI, DBG_LVL_ERROR, ("%s: Allocate ie_list fail!!!\n", __func__));
		goto LabelErr;
	}

	NdisZeroMemory(ie_list, sizeof(BCN_IE_LIST));

	if (PeerBeaconAndProbeRspSanity(pAd, wdev, Elem->Msg, Elem->MsgLen, Elem->Channel,
									ie_list, &LenVIE, pVIE, TRUE, FALSE) == FALSE)
		goto LabelErr;

	/*
		BEACON from desired BSS/IBSS found. We should be able to decide most
		BSS parameters here.
		Q. But what happen if this JOIN doesn't conclude a successful ASSOCIATEION?
			Do we need to receover back all parameters belonging to previous BSS?
		A. Should be not. There's no back-door recover to previous AP. It still need
			a new JOIN-AUTH-ASSOC sequence.
	*/
	rssi_sample.AvgRssi[0] = Elem->rssi_info.raw_rssi[0];
	rssi_sample.AvgRssi[1] = Elem->rssi_info.raw_rssi[1];
	rssi_sample.AvgRssi[2] = Elem->rssi_info.raw_rssi[2];
	rssi_sample.AvgRssi[3] = Elem->rssi_info.raw_rssi[3];
	RealRssi = rtmp_avg_rssi(pAd, &rssi_sample);
	wdev->is_marvell_ap = ie_list->is_marvell_ap;
#ifdef DOT11_HE_AX
	HAS_HE_CAPS_EXIST(ie_list->cmm_ies.ie_exists) ?
		SET_HE_CAPS_EXIST(pStaCfg->MlmeAux.ie_exists) :
		CLR_HE_CAPS_EXIST(pStaCfg->MlmeAux.ie_exists);
	MTWF_LOG(DBG_CAT_CLIENT, CATCLIENT_APCLI, DBG_LVL_TRACE, ("%s:RootAP HE_CAP(%d)\n",
		__func__, HAS_HE_CAPS_EXIST(pStaCfg->MlmeAux.ie_exists) ? 1 : 0));
#endif /* DOT11_HE_AX */
	ie_field_value_decision(wdev, ie_list);

	/* Update ScanTab: BssTableSetEntry ensures that an already existing entry with the same bssid is over-written */
	{
		/* discover new AP of this network, create BSS entry */
		Bssidx = BssTableSetEntry(pAd, wdev, ScanTab, ie_list, RealRssi, LenVIE, pVIE);

		if (Bssidx == BSS_NOT_FOUND) { /* return if BSS table full */
			MTWF_LOG(DBG_CAT_CLIENT, CATCLIENT_APCLI, DBG_LVL_ERROR, ("ERROR: Driver ScanTable Full In Apcli ProbeRsp Join\n"));
			goto LabelErr;
		}

		NdisMoveMemory(ScanTab->BssEntry[Bssidx].PTSF, &Elem->Msg[24], 4);
		NdisMoveMemory(&ScanTab->BssEntry[Bssidx].TTSF[0], &Elem->TimeStamp.u.LowPart, 4);
		NdisMoveMemory(&ScanTab->BssEntry[Bssidx].TTSF[4], &Elem->TimeStamp.u.LowPart, 4);
		ScanTab->BssEntry[Bssidx].MinSNR = Elem->Signal % 10;

		if (ScanTab->BssEntry[Bssidx].MinSNR == 0)
			ScanTab->BssEntry[Bssidx].MinSNR = -5;

		NdisMoveMemory(ScanTab->BssEntry[Bssidx].MacAddr, ie_list->Addr2, MAC_ADDR_LEN);
	}

#ifdef RT_CFG80211_SUPPORT

	/* Determine primary channel by IE's DSPS rather than channel of received frame */
	if (ie_list->Channel != 0)
		Elem->Channel = ie_list->Channel;

	MTWF_LOG(DBG_CAT_CLIENT, CATCLIENT_APCLI, DBG_LVL_TRACE, ("Info: Update the SSID %s in Kernel Table\n", ie_list->Ssid));
	RT_CFG80211_SCANNING_INFORM(pAd, Bssidx, ie_list->Channel, (UCHAR *)Elem->Msg, Elem->MsgLen, RealRssi);
#endif /* RT_CFG80211_SUPPORT */
	fsm_ops = (struct sync_fsm_ops *)wdev->sync_fsm_ops;

	if (fsm_ops->join_peer_response_matched(pAd, wdev, ie_list, Elem))
		isRecvJoinRsp = fsm_ops->join_peer_response_updated(pAd, wdev, ie_list, Elem, pVIE, LenVIE);

#ifdef CONFIG_STA_SUPPORT
	if (isRecvJoinRsp) {
		BOOLEAN TimerCancelled;
		USHORT Status = MLME_SUCCESS;

		if (pStaCfg) {
			pStaCfg->MlmeAux.isRecvJoinRsp = TRUE;
			RTMPCancelTimer(&pStaCfg->MlmeAux.JoinTimer, &TimerCancelled);
		}
		sync_fsm_state_transition(wdev, SYNC_FSM_IDLE);
#ifdef APCLI_SUPPORT

/* Update the existence of peer */
#ifdef FOLLOW_HIDDEN_SSID_FEATURE
				ApCliCheckPeerExistence(pAd, ie_list->Ssid, ie_list->SsidLen, ie_list->Bssid, ie_list->Channel);
#else
				ApCliCheckPeerExistence(pAd, ie_list->Ssid, ie_list->SsidLen, ie_list->Channel);
#endif
#endif

		cntl_join_start_conf(wdev, Status);
	}
#endif
LabelErr:

	if (VarIE != NULL)
		os_free_mem(VarIE);

	if (ie_list != NULL)
		os_free_mem(ie_list);
}

#ifdef CONFIG_STA_ADHOC_SUPPORT
static VOID sync_fsm_adhoc_start_req_action(RTMP_ADAPTER *pAd, MLME_QUEUE_ELEM *Elem)
{
	UCHAR Ssid[MAX_LEN_OF_SSID], SsidLen;
	BOOLEAN TimerCancelled;
	UCHAR *VarIE = NULL;		/* New for WPA security suites */
	NDIS_802_11_VARIABLE_IEs *pVIE = NULL;
	LARGE_INTEGER TimeStamp;
	BOOLEAN Privacy;
	/*USHORT Status;*/
	PSTA_ADMIN_CONFIG pStaCfg = GetStaCfgByWdev(pAd, Elem->wdev);
	SCAN_CTRL *ScanCtrl = get_scan_ctrl_by_wdev(pAd, Elem->wdev);
	struct adhoc_info *adhocInfo = &pStaCfg->adhocInfo;

	/*ASSERT(pStaCfg);*/

	if (!pStaCfg)
		return;

	/* allocate memory */
	os_alloc_mem(NULL, (UCHAR **)&VarIE, MAX_VIE_LEN);

	if (VarIE == NULL) {
		MTWF_LOG(DBG_CAT_CLIENT, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s: Allocate memory fail!!!\n", __func__));
		return;
	}

	/* Init Variable IE structure */
	pVIE = (PNDIS_802_11_VARIABLE_IEs) VarIE;
	pVIE->Length = 0;
	TimeStamp.u.LowPart  = 0;
	TimeStamp.u.HighPart = 0;

	if ((MlmeStartReqSanity(pAd, Elem->Msg, Elem->MsgLen, (PCHAR)Ssid, &SsidLen)) &&
		(CHAN_PropertyCheck(pAd, pStaCfg->MlmeAux.Channel, CHANNEL_NO_IBSS) == FALSE)) {
		struct wifi_dev *wdev = &pStaCfg->wdev;
		/* reset all the timers */
		RTMPCancelTimer(&ScanCtrl->ScanTimer, &TimerCancelled);
		RTMPCancelTimer(&pStaCfg->MlmeAux.JoinTimer, &TimerCancelled);
		/* Start a new IBSS. All IBSS parameters are decided now */
		MTWF_LOG(DBG_CAT_CLIENT, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
				 ("MlmeStartReqAction - Start a new IBSS. All IBSS parameters are decided now....\n"));
		pStaCfg->MlmeAux.BssType = BSS_ADHOC;
		NdisMoveMemory(pStaCfg->MlmeAux.Ssid, Ssid, SsidLen);
		pStaCfg->MlmeAux.SsidLen = SsidLen;
#ifdef IWSC_SUPPORT

		if (pStaCfg->IWscInfo.bDoNotChangeBSSID)
			pStaCfg->IWscInfo.bDoNotChangeBSSID = FALSE;
		else
#endif /* IWSC_SUPPORT */
		{
			/* generate a radom number as BSSID */
			MacAddrRandomBssid(pAd, pStaCfg->MlmeAux.Bssid);
			MTWF_LOG(DBG_CAT_CLIENT, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("MlmeStartReqAction - generate a radom number as BSSID\n"));
		}

		Privacy = IS_SECURITY(&wdev->SecConfig);
		pStaCfg->MlmeAux.CapabilityInfo = CAP_GENERATE(0, 1, Privacy, (pAd->CommonCfg.TxPreamble == Rt802_11PreambleShort),
										  pAd->CommonCfg.bUseShortSlotTime, 0);
		pStaCfg->MlmeAux.BeaconPeriod = pAd->CommonCfg.BeaconPeriod;
		pStaCfg->MlmeAux.AtimWin = adhocInfo->AtimWin;
		pStaCfg->MlmeAux.Channel = wdev->channel;
		pStaCfg->MlmeAux.CentralChannel = wdev->channel;
		check_legacy_rates(&pStaCfg->wdev.rate, &pStaCfg->MlmeAux.rate, wdev);
#ifdef DOT11_N_SUPPORT

		if (WMODE_CAP_N(wdev->PhyMode) && (adhocInfo->bAdhocN == TRUE)) {
			RTMPUpdateHTIE(&wdev->DesiredHtPhyInfo.MCSSet[0], wdev, &pStaCfg->MlmeAux.HtCapability,
						   &pStaCfg->MlmeAux.AddHtInfo);
			SET_HT_CAPS_EXIST(pStaCfg->MlmeAux.ie_exists);
			/* Not turn pStaCfg->StaActive.SupportedHtPhy.bHtEnable = TRUE here. */
			MTWF_LOG(DBG_CAT_CLIENT, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("SYNC -pStaCfg->StaActive.SupportedHtPhy.bHtEnable = TRUE\n"));
#ifdef DOT11_VHT_AC

			if (WMODE_CAP_AC(wdev->PhyMode) &&
				(pStaCfg->MlmeAux.Channel > 14)) {
				build_vht_cap_ie(pAd, wdev, (UCHAR *)&pStaCfg->MlmeAux.vht_cap);
				SET_VHT_CAPS_EXIST(pStaCfg->MlmeAux.ie_exists);
			}

#endif /* DOT11_VHT_AC */
		} else
#endif /* DOT11_N_SUPPORT */
		{
			CLR_HT_CAPS_EXIST(pStaCfg->MlmeAux.ie_exists);
			pStaCfg->StaActive.SupportedPhyInfo.bHtEnable = FALSE;
			NdisZeroMemory(&pStaCfg->StaActive.SupportedPhyInfo.MCSSet[0], 16);
		}

		/* temporarily not support QOS in IBSS */
		NdisZeroMemory(&pStaCfg->MlmeAux.APEdcaParm, sizeof(EDCA_PARM));
		NdisZeroMemory(&pStaCfg->MlmeAux.APQbssLoad, sizeof(QBSS_LOAD_PARM));
		NdisZeroMemory(&pStaCfg->MlmeAux.APQosCapability, sizeof(QOS_CAPABILITY_PARM));
		wdev->channel  = pStaCfg->MlmeAux.Channel;
		wlan_operate_set_prim_ch(wdev, pStaCfg->MlmeAux.Channel);
		MTWF_LOG(DBG_CAT_CLIENT, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
				 ("SYNC - MlmeStartReqAction(ch= %d,sup rates= %d, ext rates=%d)\n",
				  pStaCfg->MlmeAux.Channel, pStaCfg->MlmeAux.SupRateLen, pStaCfg->MlmeAux.ExtRateLen));
		/* pStaCfg->SyncMachine.CurrState = SYNC_IDLE; */

		/* yf fsm TODO */
	} else {
		MTWF_LOG(DBG_CAT_CLIENT, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("SYNC - MlmeStartReqAction() sanity check fail.\n"));
		/* pStaCfg->SyncMachine.CurrState = SYNC_IDLE; */

	}

	if (VarIE != NULL)
		os_free_mem(VarIE);
}
#endif /* CONFIG_STA_ADHOC_SUPPORT */

/* --> PUBLIC Function Start */

VOID sync_fsm_reset(struct _RTMP_ADAPTER *pAd, struct wifi_dev *wdev)
{
	UCHAR BandIdx = HcGetBandByWdev(wdev);

	pAd->ScanCtrl[BandIdx].SyncFsm.CurrState = SYNC_FSM_IDLE;
}

VOID sync_fsm_init(struct _RTMP_ADAPTER *pAd, UCHAR BandIdx, STATE_MACHINE *Sm, STATE_MACHINE_FUNC Trans[])
{
	UCHAR i;
#ifdef CONFIG_STA_SUPPORT
	PSTA_ADMIN_CONFIG pStaCfg;
#endif /* CONFIG_STA_SUPPORT */
	SCAN_CTRL *ScanCtrl = &pAd->ScanCtrl[BandIdx];

	ScanCtrl->BandIdx = BandIdx;
	StateMachineInit(Sm, (STATE_MACHINE_FUNC *)Trans, SYNC_FSM_MAX_STATE, SYNC_FSM_MAX_MSG,
					 (STATE_MACHINE_FUNC)sync_fsm_msg_invalid_state, SYNC_FSM_IDLE, SYNC_FSM_BASE);
	StateMachineSetMsgChecker(Sm, (STATE_MACHINE_MSG_CHECKER)sync_fsm_msg_checker);
	StateMachineSetAction(Sm, SYNC_FSM_IDLE, SYNC_FSM_JOIN_REQ,       (STATE_MACHINE_FUNC)sync_fsm_join_req_action);
	StateMachineSetAction(Sm, SYNC_FSM_IDLE, SYNC_FSM_SCAN_REQ,       (STATE_MACHINE_FUNC)sync_fsm_scan_req_action);
	StateMachineSetAction(Sm, SYNC_FSM_IDLE, SYNC_FSM_PEER_BEACON,    (STATE_MACHINE_FUNC)sync_fsm_peer_response_idle_action);
	StateMachineSetAction(Sm, SYNC_FSM_IDLE, SYNC_FSM_PEER_PROBE_REQ, (STATE_MACHINE_FUNC)sync_fsm_peer_request_idle_action);
	StateMachineSetAction(Sm, SYNC_FSM_IDLE, SYNC_FSM_WSC_SCAN_COMP_CHECK_REQ,
		(STATE_MACHINE_FUNC)sync_fsm_wsc_scan_comp_check_action);
#ifdef CONFIG_STA_ADHOC_SUPPORT
	StateMachineSetAction(Sm, SYNC_FSM_IDLE, SYNC_FSM_ADHOC_START_REQ, (STATE_MACHINE_FUNC)sync_fsm_adhoc_start_req_action);
#endif /* CONFIG_STA_ADHOC_SUPPORT */
	StateMachineSetAction(Sm, SYNC_FSM_LISTEN, SYNC_FSM_PEER_BEACON,    (STATE_MACHINE_FUNC)sync_fsm_peer_response_scan_action);
	StateMachineSetAction(Sm, SYNC_FSM_LISTEN, SYNC_FSM_PEER_PROBE_RSP, (STATE_MACHINE_FUNC)sync_fsm_peer_response_scan_action);
	StateMachineSetAction(Sm, SYNC_FSM_LISTEN, SYNC_FSM_SCAN_TIMEOUT,   (STATE_MACHINE_FUNC)sync_fsm_scan_timeout_action);
	StateMachineSetAction(Sm, SYNC_FSM_JOIN_WAIT, SYNC_FSM_JOIN_TIMEOUT,   (STATE_MACHINE_FUNC)sync_fsm_join_timeout_action);
	StateMachineSetAction(Sm, SYNC_FSM_JOIN_WAIT, SYNC_FSM_PEER_BEACON,    (STATE_MACHINE_FUNC)sync_fsm_peer_response_join_action);
	StateMachineSetAction(Sm, SYNC_FSM_JOIN_WAIT, SYNC_FSM_PEER_PROBE_RSP, (STATE_MACHINE_FUNC)sync_fsm_peer_response_join_action);
	/* resume scanning for fast-roaming */
	StateMachineSetAction(Sm, SYNC_FSM_PENDING, SYNC_FSM_SCAN_REQ,    (STATE_MACHINE_FUNC)sync_fsm_scan_req_action);
	StateMachineSetAction(Sm, SYNC_FSM_PENDING, SYNC_FSM_PEER_BEACON, (STATE_MACHINE_FUNC)sync_fsm_peer_response_idle_action);

	/* Cancel Action */
	for (i = 0; i < SYNC_FSM_MAX_STATE; i++)
		StateMachineSetAction(Sm, i, SYNC_FSM_CANCEL_REQ, (STATE_MACHINE_FUNC)sync_fsm_cancel_req_action);

	ScanCtrl->SyncTimerFuncContex.pAd = pAd;
	ScanCtrl->SyncTimerFuncContex.BandIdx = BandIdx;
	RTMPInitTimer(pAd, &ScanCtrl->ScanTimer, GET_TIMER_FUNCTION(sync_fsm_scan_timeout), &ScanCtrl->SyncTimerFuncContex, FALSE);
#ifdef CONFIG_STA_SUPPORT

	for (i = 0; i < MAX_MULTI_STA; i++) {
		pStaCfg = &pAd->StaCfg[i];

		if (!pStaCfg->MlmeAux.JoinTimer.Valid) {
			pStaCfg->MlmeAux.JoinTimerFuncContext.pAd = pAd;
			pStaCfg->MlmeAux.JoinTimerFuncContext.wdev = &pStaCfg->wdev;
			RTMPInitTimer(pAd, &pStaCfg->MlmeAux.JoinTimer, GET_TIMER_FUNCTION(sync_fsm_join_timeout),
						  &pStaCfg->MlmeAux.JoinTimerFuncContext, FALSE);
		}
	}

#endif /* CONFIG_STA_SUPPORT */
}

BOOLEAN sync_fsm_msg_pre_checker(struct _RTMP_ADAPTER *pAd,
								 PFRAME_802_11 pFrame, INT *Machine, INT *MsgType)
{
	BOOLEAN isNeedHandle = FALSE;

	if (pFrame->Hdr.FC.Type == FC_TYPE_MGMT) {
		switch (pFrame->Hdr.FC.SubType) {
		case SUBTYPE_BEACON:
			*Machine = SYNC_FSM;
			*MsgType = SYNC_FSM_PEER_BEACON;
			isNeedHandle = TRUE;
			break;

		case SUBTYPE_PROBE_RSP:
			*Machine = SYNC_FSM;
			*MsgType = SYNC_FSM_PEER_PROBE_RSP;
			isNeedHandle = TRUE;
			break;

		case SUBTYPE_PROBE_REQ:
			*Machine = SYNC_FSM;
			*MsgType = SYNC_FSM_PEER_PROBE_REQ;
			isNeedHandle = TRUE;
			break;
		}
	}

	return isNeedHandle;
}

extern struct sync_fsm_ops ap_fsm_ops;
extern struct sync_fsm_ops sta_fsm_ops;

BOOLEAN sync_fsm_ops_init(struct wifi_dev *wdev)
{
	RTMP_ADAPTER *pAd;

	if (!wdev)
		return FALSE;

	pAd = wdev->sys_handle;

	switch (wdev->wdev_type) {
#ifdef CONFIG_AP_SUPPORT

	case WDEV_TYPE_AP:
	case WDEV_TYPE_WDS:
	case WDEV_TYPE_GO:
	case WDEV_TYPE_MESH:
		wdev->sync_fsm_ops = &ap_fsm_ops;
		break;
#endif /* CONFIG_AP_SUPPORT */

	case WDEV_TYPE_REPEATER:
	case WDEV_TYPE_STA:
#ifdef CONFIG_STA_SUPPORT
		wdev->sync_fsm_ops = &sta_fsm_ops;
#endif /* CONFIG_STA_SUPPORT */
		break;

	default:
#ifdef CONFIG_STA_SUPPORT
		wdev->sync_fsm_ops = &sta_fsm_ops;
#endif /* CONFIG_STA_SUPPORT */
		break;
	}

	return TRUE;
}

VOID sync_fsm_cancel_req_action(RTMP_ADAPTER *pAd, struct wifi_dev *wdev)
{
	BOOLEAN Cancelled;
	SCAN_ACTION_INFO scan_action_info = {0};
	SCAN_CTRL *ScanCtrl = get_scan_ctrl_by_wdev(pAd, wdev);
	USHORT Status = MLME_STATE_MACHINE_REJECT;
#ifdef CONFIG_STA_SUPPORT
	PSTA_ADMIN_CONFIG pStaCfg = GetStaCfgByWdev(pAd, wdev);
#endif /* CONFIG_STA_SUPPORT */

	if (ScanCtrl->ScanReqwdev && (ScanCtrl->ScanReqwdev == wdev)) {
		BOOLEAN isErrHandle = TRUE;

		switch (ScanCtrl->SyncFsm.CurrState) {
		case SYNC_FSM_JOIN_WAIT:
			cntl_join_start_conf(wdev, Status);
			break;

		case SYNC_FSM_LISTEN:
			cntl_scan_conf(wdev, Status);
			break;

		default:
			isErrHandle = FALSE;
		}

		MTWF_LOG(DBG_CAT_CLIENT, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
				 ("%s [%s] Band(%d): [%s] ====================> CANCEL SYNC FSM FROM OUTSIDE (%d)\n",
				  __func__, wdev->if_dev->name, ScanCtrl->BandIdx,
				  SYNC_FSM_STATE_STR[ScanCtrl->SyncFsm.CurrState],
				  isErrHandle));
	} else {
		/*ASSERT(FALSE);*/
		return;
	}

#ifdef CONFIG_STA_SUPPORT
	if (pStaCfg)
		RTMPCancelTimer(&pStaCfg->MlmeAux.JoinTimer, &Cancelled);
#endif /* CONFIG_STA_SUPPORT */
	RTMPCancelTimer(&ScanCtrl->ScanTimer, &Cancelled);
	ScanCtrl->Channel = 0;
	scan_next_channel(pAd, wdev, &scan_action_info);
	sync_fsm_state_transition(wdev, SYNC_FSM_IDLE);
}

VOID sync_cntl_fsm_to_idle_when_scan_req(RTMP_ADAPTER *pAd, struct wifi_dev *wdev)
{
	if (wdev->cntl_machine.CurrState == CNTL_WAIT_SYNC) {
		BOOLEAN TimerCancelled;
		PSTA_ADMIN_CONFIG pStaCfg = GetStaCfgByWdev(pAd, wdev);

		if (pStaCfg) {
			RTMPCancelTimer(&pStaCfg->MlmeAux.JoinTimer, &TimerCancelled);
			sync_fsm_state_transition(wdev, SYNC_FSM_IDLE);
			cntl_fsm_state_transition(wdev, CNTL_WAIT_SYNC, __func__);
			MTWF_LOG(DBG_CAT_CLIENT, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
					 ("%s: success to set scan parameters\n", __func__));
		} else {
			MTWF_LOG(DBG_CAT_CLIENT, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
					 ("%s: fail to set scan parameters\n", __func__));
		}
	}
}
