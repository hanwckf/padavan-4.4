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

#include "precomp.h"
#include "p2p_dev_state.h"
#if CFG_ENABLE_WIFI_DIRECT

#if 1
/*lint -save -e64 Type mismatch */
static PUINT_8 apucDebugP2pDevState[P2P_DEV_STATE_NUM] = {
	(PUINT_8) DISP_STRING("P2P_DEV_STATE_IDLE"),
	(PUINT_8) DISP_STRING("P2P_DEV_STATE_SCAN"),
	(PUINT_8) DISP_STRING("P2P_DEV_STATE_REQING_CHANNEL"),
	(PUINT_8) DISP_STRING("P2P_DEV_STATE_CHNL_ON_HAND"),
	(PUINT_8) DISP_STRING("P2P_DEV_STATE_NUM")
};

/*lint -restore */
#endif /* DBG */

UINT_8 p2pDevFsmInit(IN P_ADAPTER_T prAdapter)
{
	P_P2P_DEV_FSM_INFO_T prP2pDevFsmInfo = (P_P2P_DEV_FSM_INFO_T) NULL;
	P_P2P_CHNL_REQ_INFO_T prP2pChnlReqInfo = (P_P2P_CHNL_REQ_INFO_T) NULL;
	P_P2P_MGMT_TX_REQ_INFO_T prP2pMgmtTxReqInfo = (P_P2P_MGMT_TX_REQ_INFO_T) NULL;
	P_BSS_INFO_T prP2pBssInfo = (P_BSS_INFO_T) NULL;

	do {
		ASSERT_BREAK(prAdapter != NULL);

		prP2pDevFsmInfo = prAdapter->rWifiVar.prP2pDevFsmInfo;

		ASSERT_BREAK(prP2pDevFsmInfo != NULL);

		kalMemZero(prP2pDevFsmInfo, sizeof(P2P_DEV_FSM_INFO_T));

		prP2pDevFsmInfo->eCurrentState = P2P_DEV_STATE_IDLE;

		cnmTimerInitTimer(prAdapter,
				  &(prP2pDevFsmInfo->rP2pFsmTimeoutTimer),
				  (PFN_MGMT_TIMEOUT_FUNC) p2pDevFsmRunEventTimeout, (ULONG) prP2pDevFsmInfo);

		prP2pBssInfo = cnmGetBssInfoAndInit(prAdapter, NETWORK_TYPE_P2P, TRUE);
		if (prP2pBssInfo == NULL)
			break;
		COPY_MAC_ADDR(prP2pBssInfo->aucOwnMacAddr, prAdapter->rMyMacAddr);
		prP2pBssInfo->aucOwnMacAddr[0] ^= 0x2;	/* change to local administrated address */

		prP2pDevFsmInfo->ucBssIndex = prP2pBssInfo->ucBssIndex;

		prP2pBssInfo->eCurrentOPMode = OP_MODE_P2P_DEVICE;
		prP2pBssInfo->ucConfigAdHocAPMode = AP_MODE_11G_P2P;
		prP2pBssInfo->u2HwDefaultFixedRateCode = RATE_OFDM_6M;

		prP2pBssInfo->ucPhyTypeSet = prAdapter->rWifiVar.ucAvailablePhyTypeSet & PHY_TYPE_SET_802_11GN;

		prP2pBssInfo->ucNonHTBasicPhyType = (UINT_8)
		    rNonHTApModeAttributes[prP2pBssInfo->ucConfigAdHocAPMode].ePhyTypeIndex;
		prP2pBssInfo->u2BSSBasicRateSet =
		    rNonHTApModeAttributes[prP2pBssInfo->ucConfigAdHocAPMode].u2BSSBasicRateSet;

		prP2pBssInfo->u2OperationalRateSet =
		    rNonHTPhyAttributes[prP2pBssInfo->ucNonHTBasicPhyType].u2SupportedRateSet;

		rateGetDataRatesFromRateSet(prP2pBssInfo->u2OperationalRateSet,
					    prP2pBssInfo->u2BSSBasicRateSet,
					    prP2pBssInfo->aucAllSupportedRates, &prP2pBssInfo->ucAllSupportedRatesLen);

		prP2pChnlReqInfo = &prP2pDevFsmInfo->rChnlReqInfo;
		LINK_INITIALIZE(&prP2pChnlReqInfo->rP2pChnlReqLink);

		prP2pMgmtTxReqInfo = &prP2pDevFsmInfo->rMgmtTxInfo;
		LINK_INITIALIZE(&prP2pMgmtTxReqInfo->rP2pTxReqLink);

		p2pDevFsmStateTransition(prAdapter, prP2pDevFsmInfo, P2P_DEV_STATE_IDLE);
	} while (FALSE);

	if (prP2pBssInfo)
		return prP2pBssInfo->ucBssIndex;
	else
		return P2P_DEV_BSS_INDEX + 1;

#if 0
	do {
		ASSERT_BREAK(prAdapter != NULL);

		prP2pFsmInfo = prAdapter->rWifiVar.prP2pFsmInfo;
		prP2pBssInfo = &(prAdapter->rWifiVar.arBssInfo[NETWORK_TYPE_P2P_INDEX]);

		ASSERT_BREAK(prP2pFsmInfo != NULL);

		LINK_INITIALIZE(&(prP2pFsmInfo->rMsgEventQueue));

		prP2pFsmInfo->eCurrentState = prP2pFsmInfo->ePreviousState = P2P_STATE_IDLE;
		prP2pFsmInfo->prTargetBss = NULL;

		cnmTimerInitTimer(prAdapter,
				  &(prP2pFsmInfo->rP2pFsmTimeoutTimer),
				  (PFN_MGMT_TIMEOUT_FUNC) p2pFsmRunEventFsmTimeout, (ULONG) prP2pFsmInfo);

		/* 4 <2> Initiate BSS_INFO_T - common part */
		BSS_INFO_INIT(prAdapter, NETWORK_TYPE_P2P_INDEX);

		/* 4 <2.1> Initiate BSS_INFO_T - Setup HW ID */
		prP2pBssInfo->ucConfigAdHocAPMode = AP_MODE_11G_P2P;
		prP2pBssInfo->ucHwDefaultFixedRateCode = RATE_OFDM_6M;

		prP2pBssInfo->ucNonHTBasicPhyType = (UINT_8)
		    rNonHTApModeAttributes[prP2pBssInfo->ucConfigAdHocAPMode].ePhyTypeIndex;
		prP2pBssInfo->u2BSSBasicRateSet =
		    rNonHTApModeAttributes[prP2pBssInfo->ucConfigAdHocAPMode].u2BSSBasicRateSet;

		prP2pBssInfo->u2OperationalRateSet =
		    rNonHTPhyAttributes[prP2pBssInfo->ucNonHTBasicPhyType].u2SupportedRateSet;

		rateGetDataRatesFromRateSet(prP2pBssInfo->u2OperationalRateSet,
					    prP2pBssInfo->u2BSSBasicRateSet,
					    prP2pBssInfo->aucAllSupportedRates, &prP2pBssInfo->ucAllSupportedRatesLen);

		prP2pBssInfo->prBeacon = cnmMgtPktAlloc(prAdapter,
							OFFSET_OF(WLAN_BEACON_FRAME_T, aucInfoElem[0]) + MAX_IE_LENGTH);

		if (prP2pBssInfo->prBeacon) {
			prP2pBssInfo->prBeacon->eSrc = TX_PACKET_MGMT;
			prP2pBssInfo->prBeacon->ucStaRecIndex = 0xFF;	/* NULL STA_REC */
			prP2pBssInfo->prBeacon->ucNetworkType = NETWORK_TYPE_P2P_INDEX;
		} else {
			/* Out of memory. */
			ASSERT(FALSE);
		}

		prP2pBssInfo->eCurrentOPMode = OP_MODE_NUM;

		prP2pBssInfo->rPmProfSetupInfo.ucBmpDeliveryAC = PM_UAPSD_ALL;
		prP2pBssInfo->rPmProfSetupInfo.ucBmpTriggerAC = PM_UAPSD_ALL;
		prP2pBssInfo->rPmProfSetupInfo.ucUapsdSp = WMM_MAX_SP_LENGTH_2;
		prP2pBssInfo->ucPrimaryChannel = P2P_DEFAULT_LISTEN_CHANNEL;
		prP2pBssInfo->eBand = BAND_2G4;
		prP2pBssInfo->eBssSCO = CHNL_EXT_SCN;

		if (IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucQoS))
			prP2pBssInfo->fgIsQBSS = TRUE;
		else
			prP2pBssInfo->fgIsQBSS = FALSE;

		SET_NET_PWR_STATE_IDLE(prAdapter, NETWORK_TYPE_P2P_INDEX);

		p2pFsmStateTransition(prAdapter, prP2pFsmInfo, P2P_STATE_IDLE);
	} while (FALSE);

	return;
#endif
}				/* p2pDevFsmInit */

VOID p2pDevFsmUninit(IN P_ADAPTER_T prAdapter)
{
	P_P2P_DEV_FSM_INFO_T prP2pDevFsmInfo = (P_P2P_DEV_FSM_INFO_T) NULL;
	P_BSS_INFO_T prP2pBssInfo = (P_BSS_INFO_T) NULL;

	do {
		ASSERT_BREAK(prAdapter != NULL);

		prP2pDevFsmInfo = prAdapter->rWifiVar.prP2pDevFsmInfo;

		ASSERT_BREAK(prP2pDevFsmInfo != NULL);

		prP2pBssInfo = prAdapter->aprBssInfo[prP2pDevFsmInfo->ucBssIndex];

		cnmTimerStopTimer(prAdapter, &(prP2pDevFsmInfo->rP2pFsmTimeoutTimer));

		/* Abort device FSM */
		p2pDevFsmStateTransition(prAdapter, prP2pDevFsmInfo, P2P_DEV_STATE_IDLE);
		p2pDevFsmRunEventAbort(prAdapter, prP2pDevFsmInfo);

		SET_NET_PWR_STATE_IDLE(prAdapter, prP2pBssInfo->ucBssIndex);

		/* Clear CmdQue */
		kalClearMgmtFramesByBssIdx(prAdapter->prGlueInfo, prP2pBssInfo->ucBssIndex);
		kalClearSecurityFramesByBssIdx(prAdapter->prGlueInfo, prP2pBssInfo->ucBssIndex);
		/* Clear PendingCmdQue */
		wlanReleasePendingCMDbyBssIdx(prAdapter, prP2pBssInfo->ucBssIndex);
		/* Clear PendingTxMsdu */
		nicFreePendingTxMsduInfoByBssIdx(prAdapter, prP2pBssInfo->ucBssIndex);

		/* Deactivate BSS. */
		UNSET_NET_ACTIVE(prAdapter, prP2pBssInfo->ucBssIndex);

		nicDeactivateNetwork(prAdapter, prP2pBssInfo->ucBssIndex);

		cnmFreeBssInfo(prAdapter, prP2pBssInfo);
	} while (FALSE);

#if 0
	P_P2P_FSM_INFO_T prP2pFsmInfo = (P_P2P_FSM_INFO_T) NULL;
	P_BSS_INFO_T prP2pBssInfo = (P_BSS_INFO_T) NULL;

	do {
		ASSERT_BREAK(prAdapter != NULL);

		DEBUGFUNC("p2pFsmUninit()");
		DBGLOG(P2P, INFO, "->p2pFsmUninit()\n");

		prP2pFsmInfo = prAdapter->rWifiVar.prP2pFsmInfo;
		prP2pBssInfo = &(prAdapter->rWifiVar.arBssInfo[NETWORK_TYPE_P2P_INDEX]);

		p2pFuncSwitchOPMode(prAdapter, prP2pBssInfo, OP_MODE_P2P_DEVICE, TRUE);

		p2pFsmRunEventAbort(prAdapter, prP2pFsmInfo);

		p2pStateAbort_IDLE(prAdapter, prP2pFsmInfo, P2P_STATE_NUM);

		UNSET_NET_ACTIVE(prAdapter, NETWORK_TYPE_P2P_INDEX);

		wlanAcquirePowerControl(prAdapter);

		/* Release all pending CMD queue. */
		DBGLOG(P2P, TRACE,
		       "p2pFsmUninit: wlanProcessCommandQueue, num of element:%d\n",
			prAdapter->prGlueInfo->rCmdQueue.u4NumElem);
		wlanProcessCommandQueue(prAdapter, &prAdapter->prGlueInfo->rCmdQueue);

		wlanReleasePowerControl(prAdapter);

		/* Release pending mgmt frame,
		 * mgmt frame may be pending by CMD without resource.
		 */
		kalClearMgmtFramesByBssIdx(prAdapter->prGlueInfo, NETWORK_TYPE_P2P_INDEX);

		/* Clear PendingCmdQue */
		wlanReleasePendingCMDbyBssIdx(prAdapter, NETWORK_TYPE_P2P_INDEX);

		if (prP2pBssInfo->prBeacon) {
			cnmMgtPktFree(prAdapter, prP2pBssInfo->prBeacon);
			prP2pBssInfo->prBeacon = NULL;
		}
	} while (FALSE);

	return;
#endif
}				/* p2pDevFsmUninit */

VOID
p2pDevFsmStateTransition(IN P_ADAPTER_T prAdapter,
			 IN P_P2P_DEV_FSM_INFO_T prP2pDevFsmInfo, IN ENUM_P2P_DEV_STATE_T eNextState)
{
	BOOLEAN fgIsLeaveState = (BOOLEAN) FALSE;

	ASSERT(prP2pDevFsmInfo->ucBssIndex == P2P_DEV_BSS_INDEX);

	do {
		if (!IS_BSS_ACTIVE(prAdapter->aprBssInfo[prP2pDevFsmInfo->ucBssIndex])) {
			if (!cnmP2PIsPermitted(prAdapter))
				return;

			SET_NET_ACTIVE(prAdapter, prP2pDevFsmInfo->ucBssIndex);
			nicActivateNetwork(prAdapter, prP2pDevFsmInfo->ucBssIndex);
		}

		fgIsLeaveState = fgIsLeaveState ? FALSE : TRUE;

		if (!fgIsLeaveState) {
			DBGLOG(P2P, STATE, "[P2P_DEV]TRANSITION: [%s] -> [%s]\n",
					    apucDebugP2pDevState[prP2pDevFsmInfo->eCurrentState],
					    apucDebugP2pDevState[eNextState]);

			/* Transition into current state. */
			prP2pDevFsmInfo->eCurrentState = eNextState;
		}

		switch (prP2pDevFsmInfo->eCurrentState) {
		case P2P_DEV_STATE_IDLE:
			if (!fgIsLeaveState) {
				fgIsLeaveState = p2pDevStateInit_IDLE(prAdapter,
								      &prP2pDevFsmInfo->rChnlReqInfo, &eNextState);
			} else {
				p2pDevStateAbort_IDLE(prAdapter);
			}
			break;
		case P2P_DEV_STATE_SCAN:
			if (!fgIsLeaveState) {
				p2pDevStateInit_SCAN(prAdapter,
						     prP2pDevFsmInfo->ucBssIndex, &prP2pDevFsmInfo->rScanReqInfo);
			} else {
				p2pDevStateAbort_SCAN(prAdapter, prP2pDevFsmInfo);
			}
			break;
		case P2P_DEV_STATE_REQING_CHANNEL:
			if (!fgIsLeaveState) {
				fgIsLeaveState = p2pDevStateInit_REQING_CHANNEL(prAdapter,
										prP2pDevFsmInfo->ucBssIndex,
										&(prP2pDevFsmInfo->rChnlReqInfo),
										&eNextState);
			} else {
				p2pDevStateAbort_REQING_CHANNEL(prAdapter,
								&(prP2pDevFsmInfo->rChnlReqInfo), eNextState);
			}
			break;
		case P2P_DEV_STATE_CHNL_ON_HAND:
			if (!fgIsLeaveState) {
				p2pDevStateInit_CHNL_ON_HAND(prAdapter,
							     prAdapter->aprBssInfo[prP2pDevFsmInfo->ucBssIndex],
							     prP2pDevFsmInfo, &(prP2pDevFsmInfo->rChnlReqInfo));
			} else {
				p2pDevStateAbort_CHNL_ON_HAND(prAdapter,
							      prAdapter->aprBssInfo[prP2pDevFsmInfo->ucBssIndex],
							prP2pDevFsmInfo, &(prP2pDevFsmInfo->rChnlReqInfo),
							eNextState);
			}
			break;
		case P2P_DEV_STATE_OFF_CHNL_TX:
			if (!fgIsLeaveState) {
				fgIsLeaveState = p2pDevStateInit_OFF_CHNL_TX(prAdapter,
									     prP2pDevFsmInfo,
									     &(prP2pDevFsmInfo->rChnlReqInfo),
									     &(prP2pDevFsmInfo->rMgmtTxInfo),
									     &eNextState);
			} else {
				p2pDevStateAbort_OFF_CHNL_TX(prAdapter,
							     &(prP2pDevFsmInfo->rMgmtTxInfo),
							     &(prP2pDevFsmInfo->rChnlReqInfo), eNextState);
			}
			break;
		default:
			/* Unexpected state. */
			ASSERT(FALSE);
			break;
		}
	} while (fgIsLeaveState);
}				/* p2pDevFsmStateTransition */

VOID p2pDevFsmRunEventAbort(IN P_ADAPTER_T prAdapter, IN P_P2P_DEV_FSM_INFO_T prP2pDevFsmInfo)
{
	do {
		ASSERT_BREAK((prAdapter != NULL) && (prP2pDevFsmInfo != NULL));

		if (prP2pDevFsmInfo->eCurrentState != P2P_DEV_STATE_IDLE) {
			/* Get into IDLE state. */
			p2pDevFsmStateTransition(prAdapter, prP2pDevFsmInfo, P2P_DEV_STATE_IDLE);
		}

		/* Abort IDLE. */
		p2pDevStateAbort_IDLE(prAdapter);
	} while (FALSE);

}				/* p2pDevFsmRunEventAbort */

VOID p2pDevFsmRunEventTimeout(IN P_ADAPTER_T prAdapter, IN ULONG ulParamPtr)
{
	P_P2P_DEV_FSM_INFO_T prP2pDevFsmInfo = (P_P2P_DEV_FSM_INFO_T) ulParamPtr;

	do {
		ASSERT_BREAK((prAdapter != NULL) && (prP2pDevFsmInfo != NULL));

		DBGLOG(P2P, INFO, "p2p dev fsm timeout, current state: %d:%d:%s\n",
			prP2pDevFsmInfo->eCurrentState,
			prP2pDevFsmInfo->eListenExted,
			apucDebugP2pDevState[prP2pDevFsmInfo->eCurrentState]);

		switch (prP2pDevFsmInfo->eCurrentState) {
		case P2P_DEV_STATE_IDLE:
			/* TODO: IDLE timeout for low power mode. */
			break;
		case P2P_DEV_STATE_CHNL_ON_HAND:
			switch (prP2pDevFsmInfo->eListenExted) {
			case P2P_DEV_NOT_EXT_LISTEN:
			case P2P_DEV_EXT_LISTEN_WAITFOR_TIMEOUT:
				DBGLOG(P2P, TRACE, "p2p timeout, state==P2P_DEV_STATE_CHNL_ON_HAND, eListenExted: %d\n",
					prP2pDevFsmInfo->eListenExted);
			p2pDevFsmStateTransition(prAdapter, prP2pDevFsmInfo, P2P_DEV_STATE_IDLE);
				prP2pDevFsmInfo->eListenExted = P2P_DEV_NOT_EXT_LISTEN;
				break;
			case P2P_DEV_EXT_LISTEN_ING:
				DBGLOG(P2P, TRACE, "p2p timeout, state==P2P_DEV_STATE_CHNL_ON_HAND, eListenExted: %d\n",
					prP2pDevFsmInfo->eListenExted);
				p2pDevFsmStateTransition(prAdapter, prP2pDevFsmInfo, P2P_DEV_STATE_CHNL_ON_HAND);
				prP2pDevFsmInfo->eListenExted = P2P_DEV_EXT_LISTEN_WAITFOR_TIMEOUT;
				break;
			default:
				ASSERT(FALSE);
				DBGLOG(P2P, ERROR,
					"Current P2P Dev State %d is unexpected for FSM timeout event.\n",
					prP2pDevFsmInfo->eCurrentState);
			}
			break;
		default:
			ASSERT(FALSE);
			DBGLOG(P2P, ERROR,
			       "Current P2P Dev State %d is unexpected for FSM timeout event.\n",
				prP2pDevFsmInfo->eCurrentState);
			break;
		}
	} while (FALSE);

}				/* p2pDevFsmRunEventTimeout */

/*================ Message Event =================*/
VOID p2pDevFsmRunEventScanRequest(IN P_ADAPTER_T prAdapter, IN P_MSG_HDR_T prMsgHdr)
{
	P_MSG_P2P_SCAN_REQUEST_T prP2pScanReqMsg = (P_MSG_P2P_SCAN_REQUEST_T) NULL;
	P_P2P_DEV_FSM_INFO_T prP2pDevFsmInfo = (P_P2P_DEV_FSM_INFO_T) NULL;
	P_P2P_SCAN_REQ_INFO_T prScanReqInfo = (P_P2P_SCAN_REQ_INFO_T) NULL;
	UINT_32 u4ChnlListSize = 0;
	P_P2P_SSID_STRUCT_T prP2pSsidStruct = (P_P2P_SSID_STRUCT_T) NULL;

	do {
		ASSERT_BREAK((prAdapter != NULL) && (prMsgHdr != NULL));

		prP2pDevFsmInfo = prAdapter->rWifiVar.prP2pDevFsmInfo;

		if (prP2pDevFsmInfo == NULL)
			break;

		if (prP2pDevFsmInfo->eCurrentState != P2P_DEV_STATE_IDLE)
			p2pDevFsmRunEventAbort(prAdapter, prP2pDevFsmInfo);

		prP2pScanReqMsg = (P_MSG_P2P_SCAN_REQUEST_T) prMsgHdr;
		prScanReqInfo = &(prP2pDevFsmInfo->rScanReqInfo);

		DBGLOG(P2P, TRACE, "p2pDevFsmRunEventScanRequest\n");

		/* Do we need to be in IDLE state? */
		/* p2pDevFsmRunEventAbort(prAdapter, prP2pDevFsmInfo); */

		ASSERT(prScanReqInfo->fgIsScanRequest == FALSE);

		prScanReqInfo->fgIsAbort = TRUE;
		prScanReqInfo->eScanType = prP2pScanReqMsg->eScanType;
		prScanReqInfo->u2PassiveDewellTime = 0;

		if (prP2pScanReqMsg->u4NumChannel) {
			prScanReqInfo->eChannelSet = SCAN_CHANNEL_SPECIFIED;

			/* Channel List */
			prScanReqInfo->ucNumChannelList = prP2pScanReqMsg->u4NumChannel;
			DBGLOG(P2P, TRACE, "Scan Request Channel List Number: %d\n", prScanReqInfo->ucNumChannelList);
			if (prScanReqInfo->ucNumChannelList > MAXIMUM_OPERATION_CHANNEL_LIST) {
				DBGLOG(P2P, TRACE,
				       "Channel List Number Overloaded: %d, change to: %d\n",
					prScanReqInfo->ucNumChannelList, MAXIMUM_OPERATION_CHANNEL_LIST);
				prScanReqInfo->ucNumChannelList = MAXIMUM_OPERATION_CHANNEL_LIST;
			}

			u4ChnlListSize = sizeof(RF_CHANNEL_INFO_T) * prScanReqInfo->ucNumChannelList;
			kalMemCopy(prScanReqInfo->arScanChannelList,
				   prP2pScanReqMsg->arChannelListInfo, u4ChnlListSize);
			if (prP2pScanReqMsg->u4NumChannel == 1) {
				DBGLOG(P2P, INFO, "Enlarge Dwell time to 100ms, Channel Number: %d\n",
					prP2pScanReqMsg->u4NumChannel);

				prScanReqInfo->u2PassiveDewellTime = 100;
			}
		} else {
			/* If channel number is ZERO.
			 * It means do a FULL channel scan.
			 */
			prScanReqInfo->eChannelSet = SCAN_CHANNEL_FULL;
		}

		/* SSID */
		prP2pSsidStruct = prP2pScanReqMsg->prSSID;
		for (prScanReqInfo->ucSsidNum = 0;
		     prScanReqInfo->ucSsidNum < prP2pScanReqMsg->i4SsidNum; prScanReqInfo->ucSsidNum++) {
			kalMemCopy(prScanReqInfo->arSsidStruct[prScanReqInfo->ucSsidNum].aucSsid,
				   prP2pSsidStruct->aucSsid, prP2pSsidStruct->ucSsidLen);

			prScanReqInfo->arSsidStruct[prScanReqInfo->ucSsidNum].ucSsidLen = prP2pSsidStruct->ucSsidLen;

			prP2pSsidStruct++;
		}

		/* IE Buffer */
		kalMemCopy(prScanReqInfo->aucIEBuf, prP2pScanReqMsg->pucIEBuf, prP2pScanReqMsg->u4IELen);

		prScanReqInfo->u4BufLength = prP2pScanReqMsg->u4IELen;

		p2pDevFsmStateTransition(prAdapter, prP2pDevFsmInfo, P2P_DEV_STATE_SCAN);
	} while (FALSE);

	if (prMsgHdr)
		cnmMemFree(prAdapter, prMsgHdr);

}				/* p2pDevFsmRunEventScanRequest */

VOID
p2pDevFsmRunEventScanDone(IN P_ADAPTER_T prAdapter, IN P_MSG_HDR_T prMsgHdr, IN P_P2P_DEV_FSM_INFO_T prP2pDevFsmInfo)
{
	P_MSG_SCN_SCAN_DONE prScanDoneMsg = (P_MSG_SCN_SCAN_DONE) prMsgHdr;
	P_P2P_SCAN_REQ_INFO_T prP2pScanReqInfo = (P_P2P_SCAN_REQ_INFO_T) NULL;

	do {
		ASSERT_BREAK((prAdapter != NULL) && (prMsgHdr != NULL) && (prP2pDevFsmInfo != NULL));

		if (!prP2pDevFsmInfo) {
			DBGLOG(P2P, ERROR, "prP2pDevFsmInfo is null, maybe remove p2p already\n");
			break;
		}

		prP2pScanReqInfo = &(prP2pDevFsmInfo->rScanReqInfo);

		if (prScanDoneMsg->ucSeqNum != prP2pScanReqInfo->ucSeqNumOfScnMsg) {
			DBGLOG(P2P, TRACE,
			       "P2P Scan Done SeqNum:%d  <->   P2P Dev FSM Scan SeqNum:%d",
				prScanDoneMsg->ucSeqNum, prP2pScanReqInfo->ucSeqNumOfScnMsg);
			break;
		}

		ASSERT_BREAK(prScanDoneMsg->ucBssIndex == prP2pDevFsmInfo->ucBssIndex);

		prP2pScanReqInfo->fgIsAbort = FALSE;
		prP2pScanReqInfo->fgIsScanRequest = FALSE;

		p2pDevFsmStateTransition(prAdapter, prP2pDevFsmInfo, P2P_DEV_STATE_IDLE);
	} while (FALSE);

	if (prMsgHdr)
		cnmMemFree(prAdapter, prMsgHdr);

}				/* p2pDevFsmRunEventScanDone */

VOID p2pDevFsmRunEventChannelRequest(IN P_ADAPTER_T prAdapter, IN P_MSG_HDR_T prMsgHdr)
{
	P_P2P_DEV_FSM_INFO_T prP2pDevFsmInfo = (P_P2P_DEV_FSM_INFO_T) NULL;
	P_P2P_CHNL_REQ_INFO_T prChnlReqInfo = (P_P2P_CHNL_REQ_INFO_T) NULL;
	BOOLEAN fgIsChnlFound = FALSE;

	do {
		ASSERT_BREAK((prAdapter != NULL) && (prMsgHdr != NULL));

		DBGLOG(P2P, TRACE, "p2pDevFsmRunEventChannelRequest\n");

		prP2pDevFsmInfo = prAdapter->rWifiVar.prP2pDevFsmInfo;

		if (prP2pDevFsmInfo == NULL)
			break;

		prChnlReqInfo = &(prP2pDevFsmInfo->rChnlReqInfo);

		DBGLOG(P2P, TRACE, "p2pDevFsmRunEventChannelRequest\n");

		if (!LINK_IS_EMPTY(&prChnlReqInfo->rP2pChnlReqLink)) {
			P_LINK_ENTRY_T prLinkEntry = (P_LINK_ENTRY_T) NULL;
			P_MSG_P2P_CHNL_REQUEST_T prP2pMsgChnlReq = (P_MSG_P2P_CHNL_REQUEST_T) NULL;

			LINK_FOR_EACH(prLinkEntry, &prChnlReqInfo->rP2pChnlReqLink) {
				prP2pMsgChnlReq =
				    (P_MSG_P2P_CHNL_REQUEST_T) LINK_ENTRY(prLinkEntry, MSG_HDR_T, rLinkEntry);

				if (prP2pMsgChnlReq->eChnlReqType == CH_REQ_TYPE_P2P_LISTEN) {
					LINK_REMOVE_KNOWN_ENTRY(&prChnlReqInfo->rP2pChnlReqLink, prLinkEntry);
					cnmMemFree(prAdapter, prP2pMsgChnlReq);
					/* DBGLOG(P2P, TRACE, */
					/* ("p2pDevFsmRunEventChannelAbort: Channel Abort, cookie found:%d\n", */
					/* prChnlAbortMsg->u8Cookie)); */
					fgIsChnlFound = TRUE;
					break;
				}
			}
		}

		/* Queue the channel request. */
		LINK_INSERT_TAIL(&(prChnlReqInfo->rP2pChnlReqLink), &(prMsgHdr->rLinkEntry));
		prMsgHdr = NULL;

		/* If channel is not requested, it may due to channel is released. */
		if ((!fgIsChnlFound) &&
		    (prChnlReqInfo->eChnlReqType == CH_REQ_TYPE_P2P_LISTEN) && (prChnlReqInfo->fgIsChannelRequested)) {
			ASSERT((prP2pDevFsmInfo->eCurrentState == P2P_DEV_STATE_REQING_CHANNEL) ||
			       (prP2pDevFsmInfo->eCurrentState == P2P_DEV_STATE_CHNL_ON_HAND));

			p2pDevFsmRunEventAbort(prAdapter, prP2pDevFsmInfo);

			break;
		}

		if (prP2pDevFsmInfo->eCurrentState == P2P_DEV_STATE_IDLE) {
			/* Re-enter IDLE state would trigger channel request. */
			DBGLOG(P2P, TRACE, "prepare to enter idle to trigger channel req\n");
			p2pDevFsmStateTransition(prAdapter, prP2pDevFsmInfo, P2P_DEV_STATE_IDLE);
		}
	} while (FALSE);

	if (prMsgHdr)
		cnmMemFree(prAdapter, prMsgHdr);

}				/* p2pDevFsmRunEventChannelRequest */

VOID p2pDevFsmRunEventChannelAbort(IN P_ADAPTER_T prAdapter, IN P_MSG_HDR_T prMsgHdr)
{
	P_P2P_DEV_FSM_INFO_T prP2pDevFsmInfo = (P_P2P_DEV_FSM_INFO_T) NULL;
	P_MSG_P2P_CHNL_ABORT_T prChnlAbortMsg = (P_MSG_P2P_CHNL_ABORT_T) NULL;
	P_P2P_CHNL_REQ_INFO_T prChnlReqInfo = (P_P2P_CHNL_REQ_INFO_T) NULL;

	do {
		ASSERT_BREAK((prAdapter != NULL) && (prMsgHdr != NULL));

		prChnlAbortMsg = (P_MSG_P2P_CHNL_ABORT_T) prMsgHdr;

		prP2pDevFsmInfo = prAdapter->rWifiVar.prP2pDevFsmInfo;

		if (prP2pDevFsmInfo == NULL)
			break;

		prChnlReqInfo = &(prP2pDevFsmInfo->rChnlReqInfo);

		DBGLOG(P2P, TRACE, "p2pDevFsmRunEventChannelAbort\n");

		/* If channel is not requested, it may due to channel is released. */
		if ((prChnlAbortMsg->u8Cookie == prChnlReqInfo->u8Cookie) && (prChnlReqInfo->fgIsChannelRequested)) {
			ASSERT((prP2pDevFsmInfo->eCurrentState == P2P_DEV_STATE_REQING_CHANNEL) ||
			       (prP2pDevFsmInfo->eCurrentState == P2P_DEV_STATE_CHNL_ON_HAND));

			if (prP2pDevFsmInfo->eCurrentState == P2P_DEV_STATE_REQING_CHANNEL) {

				kalP2PIndicateChannelReady(prAdapter->prGlueInfo,
							   prChnlReqInfo->u8Cookie,
							   prChnlReqInfo->ucReqChnlNum,
							   prChnlReqInfo->eBand,
							   prChnlReqInfo->eChnlSco,
							   prChnlReqInfo->u4MaxInterval);

				kalP2PIndicateChannelExpired(prAdapter->prGlueInfo,
								 prChnlReqInfo->u8Cookie,
								 prChnlReqInfo->ucReqChnlNum,
								 prChnlReqInfo->eBand,
								 prChnlReqInfo->eChnlSco);
			}

			p2pDevFsmRunEventAbort(prAdapter, prP2pDevFsmInfo);

			break;
		} else if (!LINK_IS_EMPTY(&prChnlReqInfo->rP2pChnlReqLink)) {
			P_LINK_ENTRY_T prLinkEntry = (P_LINK_ENTRY_T) NULL;
			P_MSG_P2P_CHNL_REQUEST_T prP2pMsgChnlReq = (P_MSG_P2P_CHNL_REQUEST_T) NULL;

			LINK_FOR_EACH(prLinkEntry, &prChnlReqInfo->rP2pChnlReqLink) {
				prP2pMsgChnlReq =
				    (P_MSG_P2P_CHNL_REQUEST_T) LINK_ENTRY(prLinkEntry, MSG_HDR_T, rLinkEntry);

				if (prP2pMsgChnlReq->u8Cookie == prChnlAbortMsg->u8Cookie) {
					LINK_REMOVE_KNOWN_ENTRY(&prChnlReqInfo->rP2pChnlReqLink, prLinkEntry);
					DBGLOG(P2P, INFO, "Channel Abort. Indicating event, cookie found: 0x%llx\n",
						prChnlAbortMsg->u8Cookie);

					kalP2PIndicateChannelReady(prAdapter->prGlueInfo,
								   prP2pMsgChnlReq->u8Cookie,
								   prP2pMsgChnlReq->rChannelInfo.ucChannelNum,
								   prP2pMsgChnlReq->rChannelInfo.eBand,
								   prP2pMsgChnlReq->eChnlSco,
								   prP2pMsgChnlReq->u4Duration);

					kalP2PIndicateChannelExpired(prAdapter->prGlueInfo,
									 prP2pMsgChnlReq->u8Cookie,
									 prP2pMsgChnlReq->rChannelInfo.ucChannelNum,
									 prP2pMsgChnlReq->rChannelInfo.eBand,
									 prP2pMsgChnlReq->eChnlSco);

					cnmMemFree(prAdapter, prP2pMsgChnlReq);
					break;
				}
			}
		} else {
			DBGLOG(P2P, WARN,
			       "p2pDevFsmRunEventChannelAbort: Channel Abort Fail, cookie not found:%d\n",
				prChnlAbortMsg->u8Cookie);
		}
	} while (FALSE);

	if (prMsgHdr)
		cnmMemFree(prAdapter, prMsgHdr);

}				/* p2pDevFsmRunEventChannelAbort */

VOID
p2pDevFsmRunEventChnlGrant(IN P_ADAPTER_T prAdapter, IN P_MSG_HDR_T prMsgHdr, IN P_P2P_DEV_FSM_INFO_T prP2pDevFsmInfo)
{
	P_MSG_CH_GRANT_T prMsgChGrant = (P_MSG_CH_GRANT_T) NULL;
	P_P2P_CHNL_REQ_INFO_T prChnlReqInfo = (P_P2P_CHNL_REQ_INFO_T) NULL;

	if ((prAdapter == NULL) || (prMsgHdr == NULL) || (prP2pDevFsmInfo == NULL))
		return;
	do {
		prMsgChGrant = (P_MSG_CH_GRANT_T) prMsgHdr;
		prChnlReqInfo = &(prP2pDevFsmInfo->rChnlReqInfo);

		if ((prMsgChGrant->ucTokenID != prChnlReqInfo->ucSeqNumOfChReq) ||
		    (!prChnlReqInfo->fgIsChannelRequested)) {
			break;
		}

		ASSERT(prMsgChGrant->ucPrimaryChannel == prChnlReqInfo->ucReqChnlNum);
		ASSERT(prMsgChGrant->eReqType == prChnlReqInfo->eChnlReqType);

		DBGLOG(P2P, INFO, "P2P: channel grant: u4MaxInterval: %d, Cookie: 0x%llx\n",
			prChnlReqInfo->u4MaxInterval, prChnlReqInfo->u8Cookie);

		if (prMsgChGrant->eReqType == CH_REQ_TYPE_P2P_LISTEN) {
			p2pDevFsmStateTransition(prAdapter, prP2pDevFsmInfo, P2P_DEV_STATE_CHNL_ON_HAND);
		} else {
			ASSERT(prMsgChGrant->eReqType == CH_REQ_TYPE_OFFCHNL_TX);
			p2pDevFsmStateTransition(prAdapter, prP2pDevFsmInfo, P2P_DEV_STATE_OFF_CHNL_TX);
		}
	} while (FALSE);

	if (prMsgHdr)
		cnmMemFree(prAdapter, prMsgHdr);

}				/* p2pDevFsmRunEventChnlGrant */

VOID p2pDevFsmRunEventMgmtTx(IN P_ADAPTER_T prAdapter, IN P_MSG_HDR_T prMsgHdr)
{
	P_P2P_DEV_FSM_INFO_T prP2pDevFsmInfo = (P_P2P_DEV_FSM_INFO_T) NULL;
	P_MSG_P2P_MGMT_TX_REQUEST_T prMgmtTxMsg = (P_MSG_P2P_MGMT_TX_REQUEST_T) NULL;
	P_P2P_MGMT_TX_REQ_INFO_T prP2pMgmtTxReqInfo = (P_P2P_MGMT_TX_REQ_INFO_T) NULL;

	do {
		ASSERT_BREAK((prAdapter != NULL) && (prMsgHdr != NULL));

		prMgmtTxMsg = (P_MSG_P2P_MGMT_TX_REQUEST_T) prMsgHdr;

		if ((prMgmtTxMsg->ucBssIdx != P2P_DEV_BSS_INDEX) && (IS_NET_ACTIVE(prAdapter, prMgmtTxMsg->ucBssIdx))) {
			DBGLOG(P2P, TRACE, " Role Interface\n");
			p2pFuncTxMgmtFrame(prAdapter,
					   prMgmtTxMsg->ucBssIdx,
					   prMgmtTxMsg->prMgmtMsduInfo, prMgmtTxMsg->fgNoneCckRate);
			break;
		}

		DBGLOG(P2P, TRACE, " Device Interface\n");
		DBGLOG(P2P, TRACE, "p2pDevFsmRunEventMgmtTx\n");

		prMgmtTxMsg->ucBssIdx = P2P_DEV_BSS_INDEX;

		prP2pDevFsmInfo = prAdapter->rWifiVar.prP2pDevFsmInfo;

		if (prP2pDevFsmInfo == NULL)
			break;

		prP2pMgmtTxReqInfo = &(prP2pDevFsmInfo->rMgmtTxInfo);

		if ((!prMgmtTxMsg->fgIsOffChannel) ||
		    ((prP2pDevFsmInfo->eCurrentState == P2P_DEV_STATE_OFF_CHNL_TX) &&
		     (LINK_IS_EMPTY(&prP2pMgmtTxReqInfo->rP2pTxReqLink)))) {
			DBGLOG(P2P, INFO, "send without roc\n");
			p2pFuncTxMgmtFrame(prAdapter,
					   prP2pDevFsmInfo->ucBssIndex,
					   prMgmtTxMsg->prMgmtMsduInfo, prMgmtTxMsg->fgNoneCckRate);
		} else {
			P_P2P_OFF_CHNL_TX_REQ_INFO_T prOffChnlTxReq = (P_P2P_OFF_CHNL_TX_REQ_INFO_T) NULL;

			prOffChnlTxReq = cnmMemAlloc(prAdapter, RAM_TYPE_MSG, sizeof(P2P_OFF_CHNL_TX_REQ_INFO_T));

			if (prOffChnlTxReq == NULL) {
				DBGLOG(P2P, ERROR, "Can not serve TX request due to MSG buffer not enough\n");
				ASSERT(FALSE);
				break;
			}
			DBGLOG(P2P, INFO, "send roc\n");

			prOffChnlTxReq->prMgmtTxMsdu = prMgmtTxMsg->prMgmtMsduInfo;
			prOffChnlTxReq->fgNoneCckRate = prMgmtTxMsg->fgNoneCckRate;
			kalMemCopy(&prOffChnlTxReq->rChannelInfo, &prMgmtTxMsg->rChannelInfo,
				   sizeof(RF_CHANNEL_INFO_T));
			prOffChnlTxReq->eChnlExt = prMgmtTxMsg->eChnlExt;
			prOffChnlTxReq->fgIsWaitRsp = prMgmtTxMsg->fgIsWaitRsp;

			LINK_INSERT_TAIL(&prP2pMgmtTxReqInfo->rP2pTxReqLink, &prOffChnlTxReq->rLinkEntry);

			/* Channel Request if needed. */
			if (prP2pDevFsmInfo->eCurrentState != P2P_DEV_STATE_OFF_CHNL_TX) {
				P_MSG_P2P_CHNL_REQUEST_T prP2pMsgChnlReq = (P_MSG_P2P_CHNL_REQUEST_T) NULL;

				prP2pMsgChnlReq = cnmMemAlloc(prAdapter, RAM_TYPE_MSG, sizeof(MSG_P2P_CHNL_REQUEST_T));

				if (prP2pMsgChnlReq == NULL) {
					cnmMemFree(prAdapter, prOffChnlTxReq);
					ASSERT(FALSE);
					DBGLOG(P2P, ERROR, "Not enough MSG buffer for channel request\n");
					break;
				}

				prP2pMsgChnlReq->eChnlReqType = CH_REQ_TYPE_OFFCHNL_TX;

				/* Not used in TX OFFCHNL REQ fields. */
				prP2pMsgChnlReq->rMsgHdr.eMsgId = MID_MNY_P2P_CHNL_REQ;
				prP2pMsgChnlReq->u8Cookie = 0;
				prP2pMsgChnlReq->u4Duration = P2P_OFF_CHNL_TX_DEFAULT_TIME_MS;

				kalMemCopy(&prP2pMsgChnlReq->rChannelInfo,
					   &prMgmtTxMsg->rChannelInfo, sizeof(RF_CHANNEL_INFO_T));
				prP2pMsgChnlReq->eChnlSco = prMgmtTxMsg->eChnlExt;

				p2pDevFsmRunEventChannelRequest(prAdapter, (P_MSG_HDR_T) prP2pMsgChnlReq);
			}
		}
	} while (FALSE);

	if (prMsgHdr)
		cnmMemFree(prAdapter, prMsgHdr);

}				/* p2pDevFsmRunEventMgmtTx */

WLAN_STATUS
p2pDevFsmRunEventMgmtFrameTxDone(IN P_ADAPTER_T prAdapter,
				 IN P_MSDU_INFO_T prMsduInfo, IN ENUM_TX_RESULT_CODE_T rTxDoneStatus)
{
	BOOLEAN fgIsSuccess = FALSE;
	P_P2P_DEV_FSM_INFO_T prP2pDevFsmInfo = (P_P2P_DEV_FSM_INFO_T) NULL;
	UINT_64 u8Cookie;

	do {
		ASSERT_BREAK((prAdapter != NULL) && (prMsduInfo != NULL));

		if (!prMsduInfo->prPacket) {
			DBGLOG(P2P, WARN, "Buffer Cookie freed\n");
			break;
		}

		u8Cookie = *(PUINT_64)((ULONG) prMsduInfo->prPacket + (ULONG)prMsduInfo->u2FrameLength
						+ MAC_TX_RESERVED_FIELD);

		prP2pDevFsmInfo = prAdapter->rWifiVar.prP2pDevFsmInfo;

		if (prP2pDevFsmInfo && (prP2pDevFsmInfo->eCurrentState == P2P_DEV_STATE_OFF_CHNL_TX))
			p2pDevFsmStateTransition(prAdapter, prP2pDevFsmInfo, P2P_DEV_STATE_OFF_CHNL_TX);

		if (rTxDoneStatus != TX_RESULT_SUCCESS) {
			DBGLOG(P2P, INFO, "Mgmt Frame TX Fail, Status: %d, cookie: 0x%llx SeqNO: %d.\n",
				rTxDoneStatus, u8Cookie, prMsduInfo->ucTxSeqNum);
		} else {
			fgIsSuccess = TRUE;
			DBGLOG(P2P, INFO, "Mgmt Frame TX Done, cookie: 0x%llx SeqNO: %d.\n",
				   u8Cookie, prMsduInfo->ucTxSeqNum);
		}

		kalP2PIndicateMgmtTxStatus(prAdapter->prGlueInfo, prMsduInfo, fgIsSuccess);
	} while (FALSE);

	return WLAN_STATUS_SUCCESS;
}				/* p2pDevFsmRunEventMgmtFrameTxDone */

VOID p2pDevFsmRunEventMgmtFrameRegister(IN P_ADAPTER_T prAdapter, IN P_MSG_HDR_T prMsgHdr)
{
	/* TODO: RX Filter Management. */

	if (prMsgHdr)
		cnmMemFree(prAdapter, prMsgHdr);

}				/* p2pDevFsmRunEventMgmtFrameRegister */

/* /////////////////////////////////////////// */

/*----------------------------------------------------------------------------*/
/*!
 * \brief    This function is call when channel is granted by CNM module from FW.
 *
 * \param[in] prAdapter  Pointer of ADAPTER_T
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
VOID p2pFsmRunEventChGrant(IN P_ADAPTER_T prAdapter, IN P_MSG_HDR_T prMsgHdr)
{
	P_MSG_CH_GRANT_T prMsgChGrant = (P_MSG_CH_GRANT_T) NULL;
	P_BSS_INFO_T prP2pBssInfo = (P_BSS_INFO_T) NULL;

	do {
		ASSERT_BREAK((prAdapter != NULL) && (prMsgHdr != NULL));

		prMsgChGrant = (P_MSG_CH_GRANT_T) prMsgHdr;

		prP2pBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prMsgChGrant->ucBssIndex);

		DBGLOG(P2P, TRACE, "P2P Run Event Channel Grant\n");

		switch (prP2pBssInfo->eCurrentOPMode) {
		case OP_MODE_P2P_DEVICE:
			ASSERT(prP2pBssInfo->ucBssIndex == P2P_DEV_BSS_INDEX);
			p2pDevFsmRunEventChnlGrant(prAdapter, prMsgHdr, prAdapter->rWifiVar.prP2pDevFsmInfo);
			break;
		case OP_MODE_INFRASTRUCTURE:
		case OP_MODE_ACCESS_POINT:
			ASSERT(prP2pBssInfo->ucBssIndex < P2P_DEV_BSS_INDEX);
			p2pRoleFsmRunEventChnlGrant(prAdapter, prMsgHdr,
						    P2P_ROLE_INDEX_2_ROLE_FSM_INFO(prAdapter,
										   prP2pBssInfo->u4PrivateData));
			break;
		default:
			ASSERT(FALSE);
			break;
		}
	} while (FALSE);

}				/* p2pFsmRunEventChGrant */

VOID p2pFsmRunEventScanRequest(IN P_ADAPTER_T prAdapter, IN P_MSG_HDR_T prMsgHdr)
{
	P_MSG_P2P_SCAN_REQUEST_T prP2pScanReqMsg = (P_MSG_P2P_SCAN_REQUEST_T) NULL;

	do {
		ASSERT_BREAK((prAdapter != NULL) && (prMsgHdr != NULL));

		prP2pScanReqMsg = (P_MSG_P2P_SCAN_REQUEST_T) prMsgHdr;

		if (prP2pScanReqMsg->ucBssIdx == P2P_DEV_BSS_INDEX)
			p2pDevFsmRunEventScanRequest(prAdapter, prMsgHdr);
		else
			p2pRoleFsmRunEventScanRequest(prAdapter, prMsgHdr);
	} while (FALSE);

	if (prP2pScanReqMsg == NULL)
		cnmMemFree(prAdapter, prMsgHdr);

}				/* p2pDevFsmRunEventScanRequest */

/*----------------------------------------------------------------------------*/
/*!
 * \brief    This function is used to handle scan done event during Device Discovery.
 *
 * \param[in] prAdapter  Pointer of ADAPTER_T
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
VOID p2pFsmRunEventScanDone(IN P_ADAPTER_T prAdapter, IN P_MSG_HDR_T prMsgHdr)
{
	P_MSG_SCN_SCAN_DONE prScanDoneMsg = (P_MSG_SCN_SCAN_DONE) NULL;
	P_BSS_INFO_T prP2pBssInfo = (P_BSS_INFO_T) NULL;

	do {
		ASSERT_BREAK((prAdapter != NULL) && (prMsgHdr != NULL));

		prScanDoneMsg = (P_MSG_SCN_SCAN_DONE) prMsgHdr;

		prP2pBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prScanDoneMsg->ucBssIndex);

		if (prAdapter->fgIsP2PRegistered == FALSE) {
			DBGLOG(P2P, TRACE, "P2P BSS Info is removed, break p2pFsmRunEventScanDone\n");

			if (prMsgHdr)
				cnmMemFree(prAdapter, prMsgHdr);
			break;
		}

		DBGLOG(P2P, TRACE, "P2P Scan Done Event\n");

		switch (prP2pBssInfo->eCurrentOPMode) {
		case OP_MODE_P2P_DEVICE:
			ASSERT(prP2pBssInfo->ucBssIndex == P2P_DEV_BSS_INDEX);
			p2pDevFsmRunEventScanDone(prAdapter, prMsgHdr, prAdapter->rWifiVar.prP2pDevFsmInfo);
			break;
		case OP_MODE_INFRASTRUCTURE:
		case OP_MODE_ACCESS_POINT:
			ASSERT(prP2pBssInfo->ucBssIndex < P2P_DEV_BSS_INDEX);
			p2pRoleFsmRunEventScanDone(prAdapter, prMsgHdr,
						   P2P_ROLE_INDEX_2_ROLE_FSM_INFO(prAdapter,
										  prP2pBssInfo->u4PrivateData));
			break;
		default:
			ASSERT(FALSE);
			break;
		}
	} while (FALSE);

}				/* p2pFsmRunEventScanDone */

VOID p2pFsmRunEventNetDeviceRegister(IN P_ADAPTER_T prAdapter, IN P_MSG_HDR_T prMsgHdr)
{
	P_MSG_P2P_NETDEV_REGISTER_T prNetDevRegisterMsg = (P_MSG_P2P_NETDEV_REGISTER_T) NULL;

	do {
		ASSERT_BREAK((prAdapter != NULL) && (prMsgHdr != NULL));

		DBGLOG(P2P, TRACE, "p2pFsmRunEventNetDeviceRegister\n");

		prNetDevRegisterMsg = (P_MSG_P2P_NETDEV_REGISTER_T) prMsgHdr;

		if (prNetDevRegisterMsg->fgIsEnable) {
			p2pSetMode((prNetDevRegisterMsg->ucMode == 1) ? TRUE : FALSE);
			if (p2pLaunch(prAdapter->prGlueInfo))
				ASSERT(prAdapter->fgIsP2PRegistered);
		} else {
			if (prAdapter->fgIsP2PRegistered)
				p2pRemove(prAdapter->prGlueInfo);
		}
	} while (FALSE);

	if (prMsgHdr)
		cnmMemFree(prAdapter, prMsgHdr);

}				/* p2pFsmRunEventNetDeviceRegister */

VOID p2pFsmRunEventUpdateMgmtFrame(IN P_ADAPTER_T prAdapter, IN P_MSG_HDR_T prMsgHdr)
{
	P_MSG_P2P_MGMT_FRAME_UPDATE_T prP2pMgmtFrameUpdateMsg = (P_MSG_P2P_MGMT_FRAME_UPDATE_T) NULL;

	do {
		ASSERT_BREAK((prAdapter != NULL) && (prMsgHdr != NULL));

		DBGLOG(P2P, TRACE, "p2pFsmRunEventUpdateMgmtFrame\n");

		prP2pMgmtFrameUpdateMsg = (P_MSG_P2P_MGMT_FRAME_UPDATE_T) prMsgHdr;

		switch (prP2pMgmtFrameUpdateMsg->eBufferType) {
		case ENUM_FRAME_TYPE_EXTRA_IE_BEACON:
			break;
		case ENUM_FRAME_TYPE_EXTRA_IE_ASSOC_RSP:
			break;
		case ENUM_FRAME_TYPE_EXTRA_IE_PROBE_RSP:
			break;
		case ENUM_FRAME_TYPE_PROBE_RSP_TEMPLATE:
			break;
		case ENUM_FRAME_TYPE_BEACON_TEMPLATE:
			break;
		default:
			break;
		}
	} while (FALSE);

	if (prMsgHdr)
		cnmMemFree(prAdapter, prMsgHdr);

}				/* p2pFsmRunEventUpdateMgmtFrame */

VOID p2pFsmRunEventExtendListen(IN P_ADAPTER_T prAdapter, IN P_MSG_HDR_T prMsgHdr)
{
	P_P2P_DEV_FSM_INFO_T prP2pDevFsmInfo = NULL;
	struct _MSG_P2P_EXTEND_LISTEN_INTERVAL_T *prExtListenMsg = NULL;

	ASSERT_BREAK((prAdapter != NULL) && (prMsgHdr != NULL));

	prExtListenMsg = (struct _MSG_P2P_EXTEND_LISTEN_INTERVAL_T *) prMsgHdr;

	prP2pDevFsmInfo = prAdapter->rWifiVar.prP2pDevFsmInfo;
	ASSERT_BREAK(prP2pDevFsmInfo);

	if (!prExtListenMsg->wait) {
		DBGLOG(P2P, INFO, "reset listen interval\n");
		prP2pDevFsmInfo->eListenExted = P2P_DEV_NOT_EXT_LISTEN;
		if (prMsgHdr)
			cnmMemFree(prAdapter, prMsgHdr);
		return;
	}

	if (prP2pDevFsmInfo && (prP2pDevFsmInfo->eListenExted == P2P_DEV_NOT_EXT_LISTEN)) {
		DBGLOG(P2P, TRACE, "try to ext listen, p2p state: %d\n", prP2pDevFsmInfo->eCurrentState);
		if (prP2pDevFsmInfo->eCurrentState == P2P_DEV_STATE_CHNL_ON_HAND) {
			DBGLOG(P2P, INFO, "here to ext listen interval\n");
			prP2pDevFsmInfo->eListenExted = P2P_DEV_EXT_LISTEN_ING;
		}
	}
	if (prMsgHdr)
		cnmMemFree(prAdapter, prMsgHdr);
}				/* p2pFsmRunEventUpdateMgmtFrame */

#if CFG_SUPPORT_WFD
VOID p2pFsmRunEventWfdSettingUpdate(IN P_ADAPTER_T prAdapter, IN P_MSG_HDR_T prMsgHdr)
{
	P_WFD_CFG_SETTINGS_T prWfdCfgSettings = (P_WFD_CFG_SETTINGS_T) NULL;
	P_MSG_WFD_CONFIG_SETTINGS_CHANGED_T prMsgWfdCfgSettings = (P_MSG_WFD_CONFIG_SETTINGS_CHANGED_T) NULL;

	/* WLAN_STATUS rStatus =  WLAN_STATUS_SUCCESS; */

	DBGLOG(P2P, INFO, "p2pFsmRunEventWfdSettingUpdate\n");

	do {
		ASSERT_BREAK((prAdapter != NULL));

		if (prMsgHdr != NULL) {
			prMsgWfdCfgSettings = (P_MSG_WFD_CONFIG_SETTINGS_CHANGED_T) prMsgHdr;
			prWfdCfgSettings = prMsgWfdCfgSettings->prWfdCfgSettings;
		} else {
			prWfdCfgSettings = &prAdapter->rWifiVar.rWfdConfigureSettings;
		}

		DBGLOG(P2P, INFO, "WFD Enalbe %x info %x state %x flag %x adv %x\n",
				   prWfdCfgSettings->ucWfdEnable,
				   prWfdCfgSettings->u2WfdDevInfo,
				   (UINT_32) prWfdCfgSettings->u4WfdState,
				   (UINT_32) prWfdCfgSettings->u4WfdFlag,
				   (UINT_32) prWfdCfgSettings->u4WfdAdvancedFlag);

		if (prWfdCfgSettings->ucWfdEnable == 0)
			prAdapter->prGlueInfo->prP2PInfo->u2WFDIELen = 0;
#if 0
TODO:use chip config more easy rStatus = wlanSendSetQueryCmd(prAdapter,	/* prAdapter */
									CMD_ID_SET_WFD_CTRL,	/* ucCID */
									TRUE,	/* fgSetQuery */
									FALSE,	/* fgNeedResp */
									FALSE,	/* fgIsOid */
									NULL, NULL,	/*
											 *pfCmdTimeoutHandler
											 **/
									sizeof(WFD_CFG_SETTINGS_T), /*
												     *u4SetQueryInfoLen
												     **/
									(PUINT_8) prWfdCfgSettings, /* pucInfoBuffer
												     **/
									NULL,	/*
										 *pvSetQueryBuffer
										 **/
									0	/*
										 *u4SetQueryBufferLen
										 **/
		    );
#endif
	} while (FALSE);

	if (prMsgHdr)
		cnmMemFree(prAdapter, prMsgHdr);

}

/* p2pFsmRunEventWfdSettingUpdate */

#endif /* CFG_SUPPORT_WFD */

#endif /* RunEventWfdSettingUpdate */
