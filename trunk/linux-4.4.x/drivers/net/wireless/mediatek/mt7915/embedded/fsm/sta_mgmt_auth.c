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


struct _auth_api_ops sta_auth_api;

#ifdef MAC_REPEATER_SUPPORT
#define IS_APCLI_RPT_IFINDEX_INVALID(ad, wdev, idx) (\
		IS_OPMODE_AP(ad)\
		&& (wdev)->wdev_type == WDEV_TYPE_STA\
		&& (idx) >= (hc_get_chip_cap((ad)->hdev_ctrl)->MaxRepeaterNum))
#else
#define IS_APCLI_RPT_IFINDEX_INVALID(ad, wdev, idx) (\
		IS_OPMODE_AP(ad)\
		&& (wdev)->wdev_type == WDEV_TYPE_STA\
		&& (idx) >= MAX_APCLI_NUM)

#endif /* MAC_REPEATER_SUPPORT */

BOOLEAN sta_send_auth_req(
	IN PRTMP_ADAPTER pAd,
	IN PMLME_QUEUE_ELEM Elem,
	IN PRALINK_TIMER_STRUCT pAuthTimer,
	IN RTMP_STRING *pSMName,
	IN USHORT SeqNo,
	IN PUCHAR pNewElement,
	IN ULONG ElementLen)
{
	USHORT Alg, Seq, Status;
	UCHAR Addr[6];
	ULONG Timeout;
	HEADER_802_11 AuthHdr;
	BOOLEAN TimerCancelled;
	NDIS_STATUS NStatus;
	PUCHAR pOutBuffer = NULL;
	ULONG FrameLen = 0, tmp = 0;
	struct wifi_dev *wdev = Elem->wdev;
	PSTA_ADMIN_CONFIG pStaCfg = GetStaCfgByWdev(pAd, Elem->wdev);
	USHORT ifIndex = wdev->func_idx;
	ASSERT(pStaCfg);
	ASSERT(wdev);
	MTWF_LOG(DBG_CAT_CLIENT, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s\n", __func__));

	if (!pStaCfg)
		return FALSE;

	if (IS_APCLI_RPT_IFINDEX_INVALID(pAd, wdev, ifIndex))
		return FALSE;

	/* Block all authentication request durning WPA block period */
	if (pStaCfg->bBlockAssoc == TRUE) {
		MTWF_LOG(DBG_CAT_CLIENT, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
				 ("%s - Block Auth request durning WPA block period!\n",
				  pSMName));
		auth_fsm_state_transition(wdev, AUTH_FSM_IDLE, __func__);
		Status = MLME_STATE_MACHINE_REJECT;
		cntl_auth_assoc_conf(Elem->wdev, CNTL_MLME_AUTH_CONF, Status);
		return FALSE;
	} else if (MlmeAuthReqSanity(pAd, Elem->wdev, Elem->Msg, Elem->MsgLen, Addr, &Timeout, &Alg)) {
		/* reset timer */
		RTMPCancelTimer(pAuthTimer, &TimerCancelled);
		COPY_MAC_ADDR(pStaCfg->MlmeAux.Bssid, Addr);
		pStaCfg->MlmeAux.Alg = Alg;
		Seq = SeqNo;
		Status = MLME_SUCCESS;
		NStatus = MlmeAllocateMemory(pAd, &pOutBuffer);	/*Get an unused nonpaged memory */

		if (NStatus != NDIS_STATUS_SUCCESS) {
			MTWF_LOG(DBG_CAT_CLIENT, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
					 ("%s - MlmeAuthReqAction(Alg:%d) allocate memory failed\n",
					  pSMName, Alg));
			auth_fsm_state_transition(wdev, AUTH_FSM_IDLE, __func__);
			Status = MLME_FAIL_NO_RESOURCE;
			cntl_auth_assoc_conf(Elem->wdev, CNTL_MLME_AUTH_CONF, Status);
			return FALSE;
		}


		MTWF_LOG(DBG_CAT_CLIENT, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
				 ("%s - Send AUTH request seq#1 (Alg=%d)...\n",
				  pSMName, Alg));
		MgtMacHeaderInitExt(pAd, &AuthHdr, SUBTYPE_AUTH, 0, Addr, wdev->if_addr,
							pStaCfg->MlmeAux.Bssid);
		MakeOutgoingFrame(pOutBuffer, &FrameLen, sizeof(HEADER_802_11),
						  &AuthHdr, 2, &Alg, 2, &Seq, 2, &Status,
						  END_OF_ARGS);

		if (pNewElement && ElementLen) {
			MakeOutgoingFrame(pOutBuffer + FrameLen, &tmp,
							  ElementLen, pNewElement, END_OF_ARGS);
			FrameLen += tmp;
		}


		MiniportMMRequest(pAd, 0, pOutBuffer, FrameLen);
		MlmeFreeMemory(pOutBuffer);
		MTWF_LOG(DBG_CAT_CLIENT, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:Set auth Timeout(%ld)ms\n", __func__, Timeout));
		RTMPSetTimer(pAuthTimer, Timeout);
		return TRUE;
	} else {
		MTWF_LOG(DBG_CAT_CLIENT, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s(): %s sanity check fail\n", __func__, pSMName));
		return FALSE;
	}

	return TRUE;
}

VOID sta_mlme_deauth_req_action(
	PRTMP_ADAPTER pAd,
	MLME_QUEUE_ELEM *Elem)
{
	MLME_DISCONNECT_STRUCT *pInfo; /* snowpin for cntl mgmt */
	HEADER_802_11 DeauthHdr;
	PUCHAR pOutBuffer = NULL;
	NDIS_STATUS NStatus;
	ULONG FrameLen = 0;
	USHORT Status;
	PSTA_ADMIN_CONFIG pStaCfg = GetStaCfgByWdev(pAd, Elem->wdev);
	struct wifi_dev *wdev = Elem->wdev;
	USHORT ifIndex = wdev->func_idx;

	MTWF_LOG(DBG_CAT_CLIENT, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s\n", __func__));

	ASSERT(pStaCfg);
	ASSERT(wdev);

	if (!pStaCfg)
		return;

	if (IS_APCLI_RPT_IFINDEX_INVALID(pAd, wdev, ifIndex))
		return;

	pInfo = (MLME_DISCONNECT_STRUCT *) Elem->Msg; /* snowpin for cntl mgmt */
	NStatus = MlmeAllocateMemory(pAd, &pOutBuffer); /*Get an unused nonpaged memory */

	if (NStatus != NDIS_STATUS_SUCCESS) {
		MTWF_LOG(DBG_CAT_CLIENT, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
				 ("AUTH - MlmeDeauthReqAction() allocate memory fail\n"));
		auth_fsm_state_transition(wdev, AUTH_FSM_IDLE, __func__);
		Status = MLME_FAIL_NO_RESOURCE;
		cntl_auth_assoc_conf(Elem->wdev, CNTL_MLME_DEAUTH_CONF, Status);
		return;
	}

	MTWF_LOG(DBG_CAT_CLIENT, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
			 ("AUTH - Send DE-AUTH request (Reason=%d)...\n",
			  pInfo->reason));
	MgtMacHeaderInitExt(pAd, &DeauthHdr, SUBTYPE_DEAUTH, 0, pInfo->addr,
						wdev->if_addr,
						pStaCfg->MlmeAux.Bssid);
	MakeOutgoingFrame(pOutBuffer, &FrameLen, sizeof(HEADER_802_11),
					  &DeauthHdr, 2, &pInfo->reason, END_OF_ARGS);
	MiniportMMRequest(pAd, 0, pOutBuffer, FrameLen);
	MlmeFreeMemory(pOutBuffer);
	pStaCfg->DeauthReason = pInfo->reason;
	COPY_MAC_ADDR(pStaCfg->DeauthSta, pInfo->addr);
	auth_fsm_state_transition(wdev, AUTH_FSM_IDLE, __func__);
	Status = MLME_SUCCESS;
	cntl_auth_assoc_conf(Elem->wdev, CNTL_MLME_DEAUTH_CONF, Status);
	/* send wireless event - for deauthentication */
	RTMPSendWirelessEvent(pAd, IW_DEAUTH_EVENT_FLAG, NULL, BSS0, 0);
	return;
}



VOID sta_mlme_auth_req_action(
	PRTMP_ADAPTER pAd,
	MLME_QUEUE_ELEM *Elem)
{
	PSTA_ADMIN_CONFIG pStaCfg = GetStaCfgByWdev(pAd, Elem->wdev);
	struct wifi_dev *wdev = Elem->wdev;
	USHORT ifIndex = wdev->func_idx;
	PRALINK_TIMER_STRUCT pAuthTimer = NULL;
	MTWF_LOG(DBG_CAT_CLIENT, DBG_SUBCAT_ALL, DBG_LVL_OFF,
		("%s:wdev(type=%d,wdev_idx=%d,wdev_func_idx=%d\n",
		__func__, wdev->wdev_type, wdev->wdev_idx, wdev->func_idx));
	ASSERT(pStaCfg);
	ASSERT(wdev);

	if (!pStaCfg)
		return;

	if (IS_APCLI_RPT_IFINDEX_INVALID(pAd, wdev, ifIndex))
		return;

#ifdef MAC_REPEATER_SUPPORT
	if (wdev->wdev_type == WDEV_TYPE_REPEATER) {
		PREPEATER_CLIENT_ENTRY rept = (PREPEATER_CLIENT_ENTRY) wdev->func_dev;
		pAuthTimer = &rept->ApCliAuthTimer;
	} else
#endif /* MAC_REPEATER_SUPPORT */
		pAuthTimer = &pStaCfg->MlmeAux.AuthTimer;

	if (sta_send_auth_req(pAd, Elem, pAuthTimer, "AUTH", 1, NULL, 0))
		auth_fsm_state_transition(wdev, AUTH_FSM_WAIT_SEQ2, __func__);
	else {
		USHORT Status;

		auth_fsm_state_transition(wdev, AUTH_FSM_IDLE, __func__);
		Status = MLME_INVALID_FORMAT;
		cntl_auth_assoc_conf(Elem->wdev, CNTL_MLME_AUTH_CONF, Status);
	}

	return;
}

VOID sta_class2_error_action(struct wifi_dev *wdev, UCHAR *pAddr)
{
	HEADER_802_11 DeauthHdr;
	PUCHAR pOutBuffer = NULL;
	NDIS_STATUS NStatus;
	ULONG FrameLen = 0;
	USHORT Reason = REASON_CLS2ERR;
	RTMP_ADAPTER *pAd = (RTMP_ADAPTER *)wdev->sys_handle;
	PSTA_ADMIN_CONFIG pStaCfg = GetStaCfgByWdev(pAd, wdev);

	NStatus = MlmeAllocateMemory(pAd, &pOutBuffer);	/*Get an unused nonpaged memory */

	if (NStatus != NDIS_STATUS_SUCCESS)
		return;

	MTWF_LOG(DBG_CAT_CLIENT, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
			 ("AUTH - Class 2 error, Send DEAUTH frame...\n"));
	MgtMacHeaderInitExt(pAd, &DeauthHdr, SUBTYPE_DEAUTH, 0, pAddr,
						pStaCfg->wdev.if_addr,
						pStaCfg->MlmeAux.Bssid);
	MakeOutgoingFrame(pOutBuffer, &FrameLen, sizeof(HEADER_802_11),
					  &DeauthHdr, 2, &Reason, END_OF_ARGS);
	MiniportMMRequest(pAd, 0, pOutBuffer, FrameLen);
	MlmeFreeMemory(pOutBuffer);
	pStaCfg->DeauthReason = Reason;
	COPY_MAC_ADDR(pStaCfg->DeauthSta, pAddr);
}

static VOID sta_auth_timeout_action(RTMP_ADAPTER *pAd, MLME_QUEUE_ELEM *Elem)
{
	USHORT Status;
	struct wifi_dev *wdev = Elem->wdev;
	USHORT ifIndex = wdev->func_idx;

	MTWF_LOG(DBG_CAT_CLIENT, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s\n", __func__));

	ASSERT(wdev);

	if (!wdev)
		return;

	if (IS_APCLI_RPT_IFINDEX_INVALID(pAd, wdev, ifIndex))
		return;

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("AUTH - AuthTimeoutAction\n"));
	auth_fsm_state_transition(wdev, AUTH_FSM_IDLE, __func__);
	Status = MLME_REJ_TIMEOUT;
	cntl_auth_assoc_conf(Elem->wdev, CNTL_MLME_AUTH_CONF, Status);
}

VOID sta_auth_timeout(
	IN PVOID SystemSpecific1,
	IN PVOID FunctionContext,
	IN PVOID SystemSpecific2,
	IN PVOID SystemSpecific3)
{
	PTIMER_FUNC_CONTEXT pContext = (PTIMER_FUNC_CONTEXT)FunctionContext;
	RTMP_ADAPTER *pAd = pContext->pAd;

	MTWF_LOG(DBG_CAT_CLIENT, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s():AuthTimeout\n", __func__));
	MlmeEnqueueWithWdev(pAd, AUTH_FSM, AUTH_FSM_AUTH_TIMEOUT, 0, NULL, 0, pContext->wdev);
	RTMP_MLME_HANDLER(pAd);
}

DECLARE_TIMER_FUNCTION(sta_auth_timeout);
BUILD_TIMER_FUNCTION(sta_auth_timeout);

VOID sta_peer_deauth_action(
	IN PRTMP_ADAPTER pAd,
	IN PMLME_QUEUE_ELEM Elem)
{
	UCHAR Addr1[MAC_ADDR_LEN];
	UCHAR Addr2[MAC_ADDR_LEN];
	UCHAR Addr3[MAC_ADDR_LEN];
	USHORT Reason;
	PSTA_ADMIN_CONFIG pStaCfg = GetStaCfgByWdev(pAd, Elem->wdev);
	struct wifi_dev *wdev = Elem->wdev;
	USHORT ifIndex = wdev->func_idx;
	UINT link_down_type = 0;

	MTWF_LOG(DBG_CAT_CLIENT, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s\n", __func__));

	ASSERT(pStaCfg);
	ASSERT(wdev);

	if (!pStaCfg)
		return;
#ifdef WSC_INCLUDED
	{
		struct wifi_dev *wdev = &pStaCfg->wdev;
		WSC_CTRL *wsc_ctrl = &wdev->WscControl;

		if (wsc_ctrl->WscState == WSC_STATE_WAIT_EAPFAIL) {
			RTMP_OS_WAIT_FOR_COMPLETION_TIMEOUT(&wsc_ctrl->WscEAPHandshakeCompleted,
								DISASSOC_WAIT_EAP_SUCCESS);
		}
	}
#endif
	if (IS_APCLI_RPT_IFINDEX_INVALID(pAd, wdev, ifIndex))
		return;

	if (PeerDeauthSanity(pAd, Elem->Msg, Elem->MsgLen, Addr1, Addr2, Addr3, &Reason)) {
		auth_fsm_state_transition(wdev, AUTH_FSM_IDLE, __func__);

		   if (INFRA_ON(pStaCfg)
			&& MAC_ADDR_EQUAL(pStaCfg->Bssid, Addr2)) {
			/* struct wifi_dev *wdev = &pStaCfg->wdev; */
			MTWF_LOG(DBG_CAT_CLIENT, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
					 ("AUTH_RSP - receive DE-AUTH from our AP (Reason=%d)\n",
					  Reason));

			if (Reason == REASON_4_WAY_TIMEOUT)
				RTMPSendWirelessEvent(pAd,
									  IW_PAIRWISE_HS_TIMEOUT_EVENT_FLAG,
									  NULL, 0, 0);

			if (Reason == REASON_GROUP_KEY_HS_TIMEOUT)
				RTMPSendWirelessEvent(pAd,
									  IW_GROUP_HS_TIMEOUT_EVENT_FLAG,
									  NULL, 0, 0);

#ifdef NATIVE_WPA_SUPPLICANT_SUPPORT
			RtmpOSWrielessEventSend(pAd->net_dev,
									RT_WLAN_EVENT_CGIWAP, -1, NULL,
									NULL, 0);
#endif /* NATIVE_WPA_SUPPLICANT_SUPPORT */
			/* send wireless event - for deauthentication */
			RTMPSendWirelessEvent(pAd, IW_DEAUTH_EVENT_FLAG, NULL,
								  BSS0, 0);
#ifdef WIDI_SUPPORT
			WidiUpdateStateToDaemon(pAd, MIN_NET_DEVICE_FOR_MBSSID, WIDI_MSG_TYPE_ASSOC_STATUS,
									pAd->CommonCfg.Bssid, NULL, 0, WIDI_DEAUTHENTICATED);
#endif /* WIDI_SUPPORT */
#ifdef WPA_SUPPLICANT_SUPPORT

			if ((pStaCfg->wpa_supplicant_info.WpaSupplicantUP != WPA_SUPPLICANT_DISABLE)
				&& IS_AKM_WPA2(wdev->SecConfig.AKMMap)
				&& (wdev->PortSecured == WPA_802_1X_PORT_SECURED))
				pStaCfg->wpa_supplicant_info.bLostAp = TRUE;

#endif /* WPA_SUPPLICANT_SUPPORT */

#ifdef CONFIG_OWE_SUPPORT
			if (Elem->wdev->wdev_type == WDEV_TYPE_STA)
				sta_reset_owe_parameters(pAd, ifIndex);
#endif
#if defined(DOT11_SAE_SUPPORT) || defined(CONFIG_OWE_SUPPORT)
			do {
				UCHAR if_addr[6];
				INT CachedIdx;
				SAE_INSTANCE *pSaeIns = NULL;

#ifdef MAC_REPEATER_SUPPORT
				if (wdev->wdev_type == WDEV_TYPE_REPEATER) {
					REPEATER_CLIENT_ENTRY *rept_entry = (REPEATER_CLIENT_ENTRY *) wdev->func_dev;
					COPY_MAC_ADDR(if_addr, rept_entry->CurrentAddress);
				} else
#endif /* MAC_REPEATER_SUPPORT */
					NdisCopyMemory(if_addr, pStaCfg->wdev.if_addr, MAC_ADDR_LEN);

				CachedIdx = sta_search_pmkid_cache(pAd, Addr2, ifIndex, wdev);
				if (CachedIdx != INVALID_PMKID_IDX) {
					MTWF_LOG(DBG_CAT_SEC, CATSEC_SAE, DBG_LVL_OFF,
									("%s: Delete pmkid on de-auth\n", __func__));
					sta_delete_pmkid_cache(pAd, Addr2, ifIndex, wdev);
				}
#ifdef DOT11_SAE_SUPPORT
				pSaeIns = search_sae_instance(&pAd->SaeCfg, if_addr, Addr2);
				if (pSaeIns != NULL) {
					MTWF_LOG(DBG_CAT_SEC, CATSEC_SAE, DBG_LVL_OFF,
							("%s: Delete Existing sae instance on de-auth\n", __func__));
					delete_sae_instance(pSaeIns);
				}
#endif
			} while (0);
#endif /* defined(DOT11_SAE_SUPPORT) || defined(CONFIG_OWE_SUPPORT) */
			cntl_fsm_state_transition(wdev, CNTL_WAIT_DEAUTH, __func__);

			if (cntl_auth_assoc_conf(wdev, CNTL_MLME_DEAUTH_CONF, Reason)
				== FALSE) {
			link_down_type |= LINK_REQ_FROM_AP;
			LinkDown(pAd, link_down_type, wdev, Elem);
			}
		}

#ifdef ADHOC_WPA2PSK_SUPPORT
		else if (ADHOC_ON(pAd)
				 && (MAC_ADDR_EQUAL(Addr1, Elem->wdev->if_addr)
					 || MAC_ADDR_EQUAL(Addr1, BROADCAST_ADDR))) {
			MAC_TABLE_ENTRY *pEntry;

			pEntry = MacTableLookup2(pAd, Addr2, Elem->wdev);

			if (pEntry && IS_ENTRY_CLIENT(pEntry))
				MacTableDeleteEntry(pAd, pEntry->wcid, pEntry->Addr);

			MTWF_LOG(DBG_CAT_CLIENT, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
					 ("AUTH_RSP - receive DE-AUTH from %02x:%02x:%02x:%02x:%02x:%02x\n",
					  PRINT_MAC(Addr2)));
		}

#endif /* ADHOC_WPA2PSK_SUPPORT */
	} else {
		MTWF_LOG(DBG_CAT_CLIENT, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
				 ("AUTH_RSP - PeerDeauthAction() sanity check fail\n"));
	}
}


VOID sta_peer_auth_rsp_at_seq2_action(RTMP_ADAPTER *pAd, MLME_QUEUE_ELEM *Elem)
{
	UCHAR Addr2[MAC_ADDR_LEN];
	USHORT Seq, Status, RemoteStatus, Alg;
	UCHAR iv_hdr[4];
	UCHAR *ChlgText = NULL;
	UCHAR *CyperChlgText = NULL;
	ULONG c_len = 0;
	HEADER_802_11 AuthHdr;
	BOOLEAN TimerCancelled;
	PUCHAR pOutBuffer = NULL;
	NDIS_STATUS NStatus;
	ULONG FrameLen = 0;
	USHORT Status2;
	UCHAR ChallengeIe = IE_CHALLENGE_TEXT;
	UCHAR len_challengeText = CIPHER_TEXT_LEN;
	MAC_TABLE_ENTRY *pEntry = NULL;
	PSTA_ADMIN_CONFIG pStaCfg = GetStaCfgByWdev(pAd, Elem->wdev);
	struct wifi_dev *wdev = Elem->wdev;
	USHORT ifIndex = wdev->func_idx;
	RALINK_TIMER_STRUCT *auth_timer = NULL;

	ASSERT(pStaCfg);
	ASSERT(wdev);
	MTWF_LOG(DBG_CAT_CLIENT, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s\n", __func__));

	if (!pStaCfg)
		return;

	if (IS_APCLI_RPT_IFINDEX_INVALID(pAd, wdev, ifIndex))
		return;

#ifdef MAC_REPEATER_SUPPORT
	if (wdev->wdev_type == WDEV_TYPE_REPEATER) {
		REPEATER_CLIENT_ENTRY *rept = (REPEATER_CLIENT_ENTRY *) wdev->func_dev;
		auth_timer = &rept->ApCliAuthTimer;
	} else
#endif /* MAC_REPEATER_SUPPORT */
		auth_timer = &pStaCfg->MlmeAux.AuthTimer;

	os_alloc_mem(NULL, (UCHAR **) &ChlgText, CIPHER_TEXT_LEN);

	if (ChlgText == NULL) {
		MTWF_LOG(DBG_CAT_CLIENT, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s: alloc mem fail\n", __func__));
		return;
	}

	os_alloc_mem(NULL, (UCHAR **) &CyperChlgText, CIPHER_TEXT_LEN + 8 + 8);

	if (CyperChlgText == NULL) {
		MTWF_LOG(DBG_CAT_CLIENT, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
				 ("%s: CyperChlgText Allocate memory fail!!!\n",
				  __func__));
		os_free_mem(ChlgText);
		return;
	}

	if (PeerAuthSanity(pAd, Elem->Msg, Elem->MsgLen, Addr2, &Alg, &Seq, &Status, (PCHAR)ChlgText)) {
		if (MAC_ADDR_EQUAL(pStaCfg->MlmeAux.Bssid, Addr2) && Seq == 2) {
				MTWF_LOG(DBG_CAT_CLIENT, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
					 ("AUTH - Receive AUTH_RSP seq#2 to me (Alg=%d, Status=%d, wdev_type=%u)\n",
					  Alg, Status, wdev->wdev_type));
			RTMPCancelTimer(auth_timer, &TimerCancelled);

			if (Status == MLME_SUCCESS) {
				/* Authentication Mode "LEAP" has allow for CCX 1.X */
				if (pStaCfg->MlmeAux.Alg == Ndis802_11AuthModeOpen) {
#ifdef DOT11_SAE_SUPPORT
					if (IS_AKM_SAE(pStaCfg->AKMMap)) {
						UCHAR if_addr[MAC_ADDR_LEN];
						UCHAR pmkid[LEN_PMKID];
						UCHAR pmk[LEN_PMK];

						NdisCopyMemory(if_addr, pStaCfg->wdev.if_addr, MAC_ADDR_LEN);
						if (sae_get_pmk_cache(&pAd->SaeCfg, if_addr, pStaCfg->MlmeAux.Bssid, pmkid, pmk))
							sta_add_pmkid_cache(pAd, pStaCfg->MlmeAux.Bssid, pmkid, pmk, LEN_PMK,  pStaCfg->wdev.func_idx, wdev);
					}
#endif
					pEntry = MacTableLookup2(pAd, Addr2, wdev);
					if (pEntry) {
						pEntry->SecConfig.rsnxe_len = pStaCfg->MlmeAux.rsnxe_len;
						NdisMoveMemory(pEntry->SecConfig.rsnxe_content, pStaCfg->MlmeAux.rsnxe_content, pStaCfg->MlmeAux.rsnxe_len);
					}
					auth_fsm_state_transition(wdev, AUTH_FSM_IDLE, __func__);
					cntl_auth_assoc_conf(Elem->wdev, CNTL_MLME_AUTH_CONF, Status);
				} else {
					PSEC_KEY_INFO  pKey;
					UINT default_key = wdev->SecConfig.PairwiseKeyId;

					pKey = &wdev->SecConfig.WepKey[default_key];
					/* 2. shared key, need to be challenged */
					Seq++;
					RemoteStatus = MLME_SUCCESS;
					/* Get an unused nonpaged memory */
					NStatus =
						MlmeAllocateMemory(pAd,
										   &pOutBuffer);

					if (NStatus != NDIS_STATUS_SUCCESS) {
						MTWF_LOG(DBG_CAT_CLIENT, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
								 ("AUTH - PeerAuthRspAtSeq2Action() allocate memory fail\n"));
						auth_fsm_state_transition(wdev, AUTH_FSM_IDLE, __func__);
						Status2 = MLME_FAIL_NO_RESOURCE;
						cntl_auth_assoc_conf(wdev, CNTL_MLME_AUTH_CONF, Status2);
						goto LabelOK;
					}

					MTWF_LOG(DBG_CAT_CLIENT, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
							 ("AUTH - Send AUTH request seq#3...\n"));
					MgtMacHeaderInitExt(pAd, &AuthHdr,
										SUBTYPE_AUTH, 0, Addr2,
										wdev->if_addr,
										pStaCfg->MlmeAux.Bssid);
					AuthHdr.FC.Wep = 1;
#ifdef MAC_REPEATER_SUPPORT
					if (wdev->wdev_type == WDEV_TYPE_REPEATER) {
						REPEATER_CLIENT_ENTRY *rept_entry = (REPEATER_CLIENT_ENTRY *) wdev->func_dev;
						COPY_MAC_ADDR(AuthHdr.Addr2, rept_entry->CurrentAddress);
					}
#endif
					/* TSC increment */
					INC_TX_TSC(pKey->TxTsc, LEN_WEP_TSC);
					/* Construct the 4-bytes WEP IV header */
					RTMPConstructWEPIVHdr(default_key, pKey->TxTsc, iv_hdr);
					Alg = cpu2le16(*(USHORT *) &Alg);
					Seq = cpu2le16(*(USHORT *) &Seq);
					RemoteStatus = cpu2le16(*(USHORT *) &RemoteStatus);
					/* Construct message text */
					MakeOutgoingFrame(CyperChlgText, &c_len,
									  2, &Alg,
									  2, &Seq,
									  2, &RemoteStatus,
									  1, &ChallengeIe,
									  1, &len_challengeText,
									  len_challengeText,
									  ChlgText,
									  END_OF_ARGS);

					if (RTMPSoftEncryptWEP(iv_hdr,
										   pKey,
										   CyperChlgText, c_len) == FALSE) {
						MlmeFreeMemory(pOutBuffer);
						auth_fsm_state_transition(wdev, AUTH_FSM_IDLE, __func__);
						Status2 = MLME_FAIL_NO_RESOURCE;
						cntl_auth_assoc_conf(Elem->wdev, CNTL_MLME_AUTH_CONF, Status);
						goto LabelOK;
					}

					/* Update the total length for 4-bytes ICV */
					c_len += LEN_ICV;
					MakeOutgoingFrame(pOutBuffer, &FrameLen,
									  sizeof
									  (HEADER_802_11),
									  &AuthHdr,
									  LEN_WEP_IV_HDR,
									  iv_hdr, c_len,
									  CyperChlgText,
									  END_OF_ARGS);
					MiniportMMRequest(pAd, 0, pOutBuffer, FrameLen);
					MlmeFreeMemory(pOutBuffer);
					RTMPSetTimer(auth_timer, AUTH_TIMEOUT);

					auth_fsm_state_transition(wdev, AUTH_FSM_WAIT_SEQ4, __func__);
				}
			} else {
				pStaCfg->AuthFailReason = Status;
				COPY_MAC_ADDR(pStaCfg->AuthFailSta, Addr2);
				auth_fsm_state_transition(wdev, AUTH_FSM_IDLE, __func__);
				cntl_auth_assoc_conf(Elem->wdev, CNTL_MLME_AUTH_CONF, Status);
			}
		}
	} else {
		MTWF_LOG(DBG_CAT_CLIENT, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
				 ("AUTH - PeerAuthSanity() sanity check fail\n"));
	}

LabelOK:

	if (ChlgText != NULL)
		os_free_mem(ChlgText);

	if (CyperChlgText != NULL)
		os_free_mem(CyperChlgText);

	return;
}


VOID sta_peer_auth_rsp_at_seq4_action(RTMP_ADAPTER *pAd, MLME_QUEUE_ELEM *Elem)
{
	UCHAR Addr2[MAC_ADDR_LEN];
	USHORT Alg, Seq, Status;
	/*    CHAR          ChlgText[CIPHER_TEXT_LEN]; */
	CHAR *ChlgText = NULL;
	BOOLEAN TimerCancelled;
	PSTA_ADMIN_CONFIG pStaCfg = GetStaCfgByWdev(pAd, Elem->wdev);
	struct wifi_dev *wdev = Elem->wdev;
	USHORT ifIndex = wdev->func_idx;
	RALINK_TIMER_STRUCT *auth_timer = NULL;

	MTWF_LOG(DBG_CAT_CLIENT, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s\n", __func__));

	ASSERT(pStaCfg);
	ASSERT(wdev);

	if (!pStaCfg)
		return;

	if (IS_APCLI_RPT_IFINDEX_INVALID(pAd, wdev, ifIndex))
		return;

#ifdef MAC_REPEATER_SUPPORT
	if (wdev->wdev_type == WDEV_TYPE_REPEATER) {
		REPEATER_CLIENT_ENTRY *rept = (REPEATER_CLIENT_ENTRY *) wdev->func_dev;
		auth_timer = &rept->ApCliAuthTimer;
	} else
#endif /* MAC_REPEATER_SUPPORT */
		auth_timer = &pStaCfg->MlmeAux.AuthTimer;

	/* allocate memory */
	os_alloc_mem(NULL, (UCHAR **) &ChlgText, CIPHER_TEXT_LEN);

	if (ChlgText == NULL) {
		MTWF_LOG(DBG_CAT_CLIENT, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
				 ("%s: ChlgText Allocate memory fail!!!\n",
				  __func__));
		return;
	}

	if (PeerAuthSanity
		(pAd, Elem->Msg, Elem->MsgLen, Addr2, &Alg, &Seq, &Status,
		 ChlgText)) {
		if (MAC_ADDR_EQUAL(pStaCfg->MlmeAux.Bssid, Addr2) && Seq == 4) {

				MTWF_LOG(DBG_CAT_CLIENT, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
						 ("AUTH - Receive AUTH_RSP seq#4 to me\n"));
			RTMPCancelTimer(auth_timer, &TimerCancelled);

			if (Status != MLME_SUCCESS) {
				pStaCfg->AuthFailReason = Status;
				COPY_MAC_ADDR(pStaCfg->AuthFailSta, Addr2);
				RTMPSendWirelessEvent(pAd, IW_SHARED_WEP_FAIL,
									  NULL, BSS0, 0);
			}

			auth_fsm_state_transition(wdev, AUTH_FSM_IDLE, __func__);
			cntl_auth_assoc_conf(Elem->wdev, CNTL_MLME_AUTH_CONF, Status);
		}
	} else {
		MTWF_LOG(DBG_CAT_CLIENT, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
				 ("AUTH - PeerAuthRspAtSeq4Action() sanity check fail\n"));
	}

	if (ChlgText != NULL)
		os_free_mem(ChlgText);
}

#ifdef DOT11_SAE_SUPPORT
/*
    ==========================================================================
    Description:

	IRQL = DISPATCH_LEVEL

    ==========================================================================
 */
VOID sta_sae_auth_req_action(RTMP_ADAPTER *pAd, MLME_QUEUE_ELEM *Elem)
{
	STA_ADMIN_CONFIG *pStaCfg = GetStaCfgByWdev(pAd, Elem->wdev);
	/* SAE_MLME_AUTH_REQ_STRUCT *AuthReq = (SAE_MLME_AUTH_REQ_STRUCT *)Elem->Msg; */
	MLME_AUTH_REQ_STRUCT *AuthReq = (MLME_AUTH_REQ_STRUCT *)Elem->Msg;
	struct wifi_dev *wdev = Elem->wdev;
	USHORT ifIndex = wdev->func_idx;
	UCHAR if_addr[ETH_ALEN];
	UCHAR *pSae_cfg_group = NULL;
	struct _SECURITY_CONFIG *sec_cfg;

	MTWF_LOG(DBG_CAT_SEC, CATSEC_SAE, DBG_LVL_OFF, ("==>%s()\n", __func__));
	ASSERT(pStaCfg);
	ASSERT(wdev);

	if (!pStaCfg)
		return;

	if (IS_APCLI_RPT_IFINDEX_INVALID(pAd, wdev, ifIndex))
		return;

	COPY_MAC_ADDR(pStaCfg->MlmeAux.Bssid, AuthReq->Addr);

#ifdef MAC_REPEATER_SUPPORT
	if (wdev->wdev_type == WDEV_TYPE_REPEATER) {
		REPEATER_CLIENT_ENTRY *pReptEntry = (REPEATER_CLIENT_ENTRY *) wdev->func_dev;

		pSae_cfg_group = &pReptEntry->sae_cfg_group;
		COPY_MAC_ADDR(if_addr, pReptEntry->CurrentAddress);
	} else
#endif /* MAC_REPEATER_SUPPORT */
	{
		pSae_cfg_group = &pStaCfg->sae_cfg_group;
		COPY_MAC_ADDR(if_addr, pStaCfg->wdev.if_addr);
	}

	sec_cfg = &wdev->SecConfig;

#ifdef MAC_REPEATER_SUPPORT
	if (wdev->wdev_type == WDEV_TYPE_REPEATER)
		; /* do not need to redrive pt */
	else
#endif
	if (pStaCfg->MlmeAux.use_h2e_connect)
		sae_derive_pt(&pAd->SaeCfg, sec_cfg->PSK, pStaCfg->MlmeAux.Ssid,
					pStaCfg->MlmeAux.SsidLen, NULL, &sec_cfg->pt_list);
	else
		sae_pt_list_deinit(&sec_cfg->pt_list);

	if (sae_auth_init(pAd, &pAd->SaeCfg, wdev->if_addr, AuthReq->Addr,
		pStaCfg->MlmeAux.Bssid, sec_cfg->PSK, sec_cfg->pt_list, *pSae_cfg_group))
		auth_fsm_state_transition(wdev, AUTH_FSM_WAIT_SAE, __func__);
	else {
		USHORT Status;
		auth_fsm_state_transition(wdev, AUTH_FSM_IDLE, __func__);
		Status = MLME_INVALID_FORMAT;
		cntl_auth_assoc_conf(wdev, CNTL_MLME_AUTH_CONF, Status);
	}
}


/*
    ==========================================================================
    Description:

	IRQL = DISPATCH_LEVEL

    ==========================================================================
 */
VOID sta_sae_auth_rsp_action(RTMP_ADAPTER *pAd, MLME_QUEUE_ELEM *Elem)
{
	STA_ADMIN_CONFIG *pStaCfg = GetStaCfgByWdev(pAd, Elem->wdev);
	FRAME_802_11 *Fr = (FRAME_802_11 *)Elem->Msg;
	struct wifi_dev *wdev = Elem->wdev;
	USHORT ifIndex = wdev->func_idx;
#ifdef MAC_REPEATER_SUPPORT
	REPEATER_CLIENT_ENTRY *pReptEntry = NULL;
#endif
	USHORT seq;
	USHORT status;
	USHORT mlme_status;
	UCHAR *pmk;
	UCHAR is_h2e_connect;
	ASSERT(pStaCfg);
	ASSERT(wdev);

	if (!pStaCfg)
		return;

	if (IS_APCLI_RPT_IFINDEX_INVALID(pAd, wdev, ifIndex))
		return;

	NdisMoveMemory(&seq,    &Fr->Octet[2], 2);
	NdisMoveMemory(&status, &Fr->Octet[4], 2);
	if (FALSE == sae_handle_auth(pAd, &pAd->SaeCfg, Elem->Msg, Elem->MsgLen,
						  wdev->SecConfig.PSK,
						  wdev->SecConfig.pt_list,
						  &wdev->SecConfig.sae_cap,
						  &wdev->SecConfig.pwd_id_list_head,
						  seq, status, &pmk, &is_h2e_connect)) {
		mlme_status = MLME_UNSPECIFY_FAIL;
		auth_fsm_state_transition(wdev, AUTH_FSM_IDLE, __func__);
		cntl_auth_assoc_conf(wdev, CNTL_MLME_AUTH_CONF, mlme_status);
	} else if (pmk != NULL) {
		MAC_TABLE_ENTRY *pEntry = NULL;
		hex_dump_with_lvl("pmk:", (char *)pmk, LEN_PMK, DBG_LVL_TRACE);
#ifdef MAC_REPEATER_SUPPORT
		if (wdev->wdev_type == WDEV_TYPE_REPEATER) {
			pReptEntry = (REPEATER_CLIENT_ENTRY *) wdev->func_dev;
			if (pReptEntry) {
				MTWF_LOG(DBG_CAT_SEC, CATSEC_SAE, DBG_LVL_TRACE, ("%s Move pmk to rept_PMK\n", __func__));
				NdisMoveMemory(pReptEntry->rept_PMK, pmk, LEN_PMK);
				mlme_status = MLME_SUCCESS;
			} else
				mlme_status = MLME_UNSPECIFY_FAIL;
		} else
#endif /*MAC_REPEATER_SUPPORT*/
		{
			pEntry = MacTableLookup(pAd, Fr->Hdr.Addr2);

			if (pEntry) {
				NdisMoveMemory(pEntry->SecConfig.PMK, pmk, LEN_PMK);
				pEntry->SecConfig.is_h2e_connect = is_h2e_connect;
				pEntry->SecConfig.sae_cap.gen_pwe_method = wdev->SecConfig.sae_cap.gen_pwe_method;
				pEntry->SecConfig.rsnxe_len = pStaCfg->MlmeAux.rsnxe_len;
				NdisMoveMemory(pEntry->SecConfig.rsnxe_content, pStaCfg->MlmeAux.rsnxe_content, pStaCfg->MlmeAux.rsnxe_len);
				mlme_status = MLME_SUCCESS;
				MTWF_LOG(DBG_CAT_SEC, CATSEC_SAE, DBG_LVL_OFF, ("%s(): Security AKM = 0x%x, PairwiseCipher = 0x%x, GroupCipher = 0x%x\n",
						 __func__, pEntry->SecConfig.AKMMap, pEntry->SecConfig.PairwiseCipher, pEntry->SecConfig.GroupCipher));
			} else
				mlme_status = MLME_UNSPECIFY_FAIL;
		}

		auth_fsm_state_transition(wdev, AUTH_FSM_IDLE, __func__);
		cntl_auth_assoc_conf(wdev, CNTL_MLME_AUTH_CONF, mlme_status);
	} else {
		MTWF_LOG(DBG_CAT_SEC, CATSEC_SAE, DBG_LVL_TRACE, ("**************Unhandled ************\n"));
	}
}
#endif /* DOT11_SAE_SUPPORT */


VOID sta_auth_init(struct wifi_dev *wdev)
{
	RTMP_ADAPTER *pAd = (RTMP_ADAPTER *)wdev->sys_handle;
	PSTA_ADMIN_CONFIG pStaCfg = GetStaCfgByWdev(pAd, wdev);

	ASSERT(pStaCfg);

	if (!pStaCfg)
		return;

	sta_auth_api.mlme_deauth_req_action		= sta_mlme_deauth_req_action;
	sta_auth_api.mlme_auth_req_action			= sta_mlme_auth_req_action;
	sta_auth_api.auth_timeout_action			= sta_auth_timeout_action;
	sta_auth_api.peer_deauth_action				= sta_peer_deauth_action;
	sta_auth_api.peer_auth_rsp_at_seq2_action	= sta_peer_auth_rsp_at_seq2_action;
	sta_auth_api.peer_auth_rsp_at_seq4_action	= sta_peer_auth_rsp_at_seq4_action;
#ifdef DOT11_SAE_SUPPORT
	sta_auth_api.sae_auth_req_action		= sta_sae_auth_req_action;
	sta_auth_api.sae_auth_rsp_action		= sta_sae_auth_rsp_action;
#endif
	wdev->auth_api = &sta_auth_api;
	wdev->auth_machine.CurrState = AUTH_FSM_IDLE;

	/* if Timer is not init yet, init it */
	if (!pStaCfg->MlmeAux.AuthTimer.Valid) {
		pStaCfg->MlmeAux.AuthTimerFuncContext.pAd = pAd;
		pStaCfg->MlmeAux.AuthTimerFuncContext.wdev = wdev;
		RTMPInitTimer(pAd, &pStaCfg->MlmeAux.AuthTimer,
					  GET_TIMER_FUNCTION(sta_auth_timeout), &pStaCfg->MlmeAux.AuthTimerFuncContext, FALSE);
	}
}


