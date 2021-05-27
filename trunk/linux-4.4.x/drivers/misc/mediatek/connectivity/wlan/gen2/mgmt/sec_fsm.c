/*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See http://www.gnu.org/licenses/gpl-2.0.html for more details.
*/

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/
#include "precomp.h"

#if CFG_RSN_MIGRATION

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/
#if DBG
/*lint -save -e64 Type mismatch */
static PUINT_8 apucDebugSecState[SEC_STATE_NUM] = {
	(PUINT_8) DISP_STRING("SEC_STATE_INIT"),
	(PUINT_8) DISP_STRING("SEC_STATE_INITIATOR_PORT_BLOCKED"),
	(PUINT_8) DISP_STRING("SEC_STATE_RESPONDER_PORT_BLOCKED"),
	(PUINT_8) DISP_STRING("SEC_STATE_CHECK_OK"),
	(PUINT_8) DISP_STRING("SEC_STATE_SEND_EAPOL"),
	(PUINT_8) DISP_STRING("SEC_STATE_SEND_DEAUTH"),
	(PUINT_8) DISP_STRING("SEC_STATE_COUNTERMEASURE"),
};

/*lint -restore */
#endif /* DBG */

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

/*----------------------------------------------------------------------------*/
/*!
* \brief This function will do initialization of Security FSM and all variables in
*        SEC_INFO_T.
*
* \param[in] prSta            Pointer to the STA record
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID secFsmInit(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prSta)
{
	P_SEC_INFO_T prSecInfo;
	P_AIS_SPECIFIC_BSS_INFO_T prAisSpecBssInfo;

	ASSERT(prSta);

	prSecInfo = &prSta->rSecInfo;

#if 1				/* MT6620 */
	/* At MT5921, is ok, but at MT6620, firmware base ASIC, the firmware */
	/* will lost these data, thus, driver have to keep the wep material and */
	/* setting to firmware while awake from D3. */
#endif

	prSecInfo->eCurrentState = SEC_STATE_INIT;

	prSecInfo->fg2nd1xSend = FALSE;
	prSecInfo->fgKeyStored = FALSE;

	if (IS_STA_IN_AIS(prSta)) {
		prAisSpecBssInfo = &prAdapter->rWifiVar.rAisSpecificBssInfo;

		prAisSpecBssInfo->u4RsnaLastMICFailTime = 0;
		prAisSpecBssInfo->fgCheckEAPoLTxDone = FALSE;

		cnmTimerInitTimer(prAdapter,
				  &prAdapter->rWifiVar.rAisSpecificBssInfo.rRsnaEAPoLReportTimeoutTimer,
				  (PFN_MGMT_TIMEOUT_FUNC) secFsmEventEapolTxTimeout, (ULONG) prSta);

		cnmTimerInitTimer(prAdapter,
				  &prAdapter->rWifiVar.rAisSpecificBssInfo.rRsnaBlockTrafficTimer,
				  (PFN_MGMT_TIMEOUT_FUNC) secFsmEventEndOfCounterMeasure, (ULONG) prSta);

	}
}

/*----------------------------------------------------------------------------*/
/*!
* \brief This function will do uninitialization of Security FSM and all variables in
*        SEC_INFO_T.
*
* \param[in] prSta            Pointer to the STA record
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID				/* whsu:Todo: */
secFsmUnInit(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prSta)
{
	P_SEC_INFO_T prSecInfo;

	ASSERT(prSta);

	prSecInfo = &prSta->rSecInfo;

	prSecInfo->fg2nd1xSend = FALSE;
	prSecInfo->fgKeyStored = FALSE;

	/* nicPrivacyRemoveWlanTable(prSta->ucWTEntry); */

	if (IS_STA_IN_AIS(prSta)) {
		cnmTimerStopTimer(prAdapter, &prAdapter->rWifiVar.rAisSpecificBssInfo.rRsnaEAPoLReportTimeoutTimer);
		cnmTimerStopTimer(prAdapter, &prAdapter->rWifiVar.rAisSpecificBssInfo.rRsnaBlockTrafficTimer);
	}

}

/*----------------------------------------------------------------------------*/
/*!
* \brief This function will do action part while in STATE transition of
*        STANDBY to CHECK_OK.
*
* \param[in] prSta            Pointer to the Sta record
*
* \return - none
*/
/*----------------------------------------------------------------------------*/
static inline VOID secFsmTrans_INIT_to_CHECK_OK(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prSta)
{
	secSetPortBlocked(prAdapter, prSta, FALSE);

}

/*----------------------------------------------------------------------------*/
/*!
* \brief This function will do action part while in STATE transition of
*        INIT to INITIATOR_PORT_BLOCKED.
*
* \param[in] prSta            Pointer to the Sta record
*
* \return - none
*/
/*----------------------------------------------------------------------------*/
static inline VOID secFsmTrans_INIT_to_INITIATOR_PORT_BLOCKED(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prSta)
{

}

/*----------------------------------------------------------------------------*/
/*!
* \brief This function will do action part while in STATE transition of
*        INIT to RESPONDER_PORT_BLOCKED.
*
* \param[in] prSta            Pointer to the Sta record
*
* \return - none
*/
/*----------------------------------------------------------------------------*/
static inline VOID secFsmTrans_INIT_to_RESPONDER_PORT_BLOCKED(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prSta)
{

}

/*----------------------------------------------------------------------------*/
/*!
* \brief This function will do action part while in STATE transition of
*        INITIATOR_PORT_BLOCKED to CHECK_OK.
*
* \param[in] prSta            Pointer to the Sta record
*
* \return - none
*/
/*----------------------------------------------------------------------------*/
static inline VOID secFsmTrans_INITIATOR_PORT_BLOCKED_to_CHECK_OK(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prSta)
{
	secSetPortBlocked(prAdapter, prSta, FALSE);
}

/*----------------------------------------------------------------------------*/
/*!
* \brief This function will do action part while in STATE transition of
*        RESPONDER_PORT_BLOCKED to CHECK_OK.
*
* \param[in] prSta            Pointer to the Sta record
*
* \return - none
*/
/*----------------------------------------------------------------------------*/
static inline VOID secFsmTrans_RESPONDER_PORT_BLOCKED_to_CHECK_OK(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prSta)
{
	secSetPortBlocked(prAdapter, prSta, FALSE);
}

/*----------------------------------------------------------------------------*/
/*!
* \brief This function will do action part while in STATE transition of
*        CHECK_OK to SEND_EAPOL
*
* \param[in] prSta            Pointer to the Sta record
*
* \return -
*/
/*----------------------------------------------------------------------------*/
static inline VOID secFsmTrans_CHECK_OK_to_SEND_EAPOL(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prSta)
{

	P_AIS_SPECIFIC_BSS_INFO_T prAisBssInfo;

	ASSERT(prAdapter);

	ASSERT(prSta);

	prAisBssInfo = &prAdapter->rWifiVar.rAisSpecificBssInfo;

	ASSERT(prAisBssInfo);

	if (!IS_STA_IN_AIS(prSta)) {
		DBGLOG(RSN, INFO, "Counter Measure should occur at AIS network!!\n");
		/* ASSERT(0); */
		return;
	}

	prAisBssInfo->fgCheckEAPoLTxDone = TRUE;

	/* cnmTimerStartTimer(prAdapter, */
	/* &prAisBssInfo->rRsnaEAPoLReportTimeoutTimer, */
	/* SEC_TO_MSEC(EAPOL_REPORT_SEND_TIMEOUT_INTERVAL_SEC)); */

}

/*----------------------------------------------------------------------------*/
/*!
* \brief This function will do action part while in STATE transition of
*        SEND_EAPOL to SEND_DEAUTH.
*
* \param[in] prSta            Pointer to the Sta record
*
* \return - none
*/
/*----------------------------------------------------------------------------*/
static inline VOID secFsmTrans_SEND_EAPOL_to_SEND_DEAUTH(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prSta)
{

	if (!IS_STA_IN_AIS(prSta)) {
		DBGLOG(RSN, INFO, "Counter Measure should occur at AIS network!!\n");
		/* ASSERT(0); */
		return;
	}

	/* Compose deauth frame to AP, a call back function for tx done */
	if (authSendDeauthFrame(prAdapter,
				prSta,
				(P_SW_RFB_T) NULL,
				REASON_CODE_MIC_FAILURE,
				(PFN_TX_DONE_HANDLER) secFsmEventDeauthTxDone) != WLAN_STATUS_SUCCESS) {
		ASSERT(FALSE);
	}
}

/*----------------------------------------------------------------------------*/
/*!
* \brief This function will do action part while in STATE transition of
*        SEND_DEAUTH to COUNTERMEASURE.
*
* \param[in] prSta            Pointer to the Sta record
*
* \return -
*/
/*----------------------------------------------------------------------------*/
static inline VOID secFsmTrans_SEND_DEAUTH_to_COUNTERMEASURE(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prSta)
{
	ASSERT(prAdapter);
	ASSERT(prSta);

	if (!IS_STA_IN_AIS(prSta)) {
		DBGLOG(RSN, INFO, "Counter Measure should occur at AIS network!!\n");
		/* ASSERT(0); */
		return;
	}
	/* Start the 60 sec timer */
	cnmTimerStartTimer(prAdapter,
			   &prAdapter->rWifiVar.rAisSpecificBssInfo.rRsnaBlockTrafficTimer,
			   SEC_TO_MSEC(COUNTER_MEASURE_TIMEOUT_INTERVAL_SEC));
}

/*----------------------------------------------------------------------------*/
/*!
* \brief This function will do action part while in STATE transition of
*        SEND_DEAUTH to COUNTERMEASURE.
*
* \param[in] prSta            Pointer to the Sta record
*
* \return -
*/
/*----------------------------------------------------------------------------*/
static inline VOID secFsmTrans_COUNTERMEASURE_to_INIT(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prSta)
{

	/* Clear the counter measure flag */
}

/*----------------------------------------------------------------------------*/
/*!
* \brief The Core FSM engine of security module.
*
* \param[in] prSta            Pointer to the Sta record
* \param[in] eNextState    Enum value of next sec STATE
*
* \return -
*/
/*----------------------------------------------------------------------------*/
VOID secFsmSteps(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prSta, IN ENUM_SEC_STATE_T eNextState)
{
	P_SEC_INFO_T prSecInfo;
	BOOLEAN fgIsTransition = (BOOLEAN) FALSE;

	ASSERT(prSta);

	prSecInfo = &prSta->rSecInfo;
	ASSERT(prSecInfo);

	DEBUGFUNC("secFsmSteps");
	do {
		/* Do entering Next State */
		prSecInfo->ePreviousState = prSecInfo->eCurrentState;

		/* Do entering Next State */
#if DBG
		DBGLOG(RSN, STATE, "\n %pM TRANSITION: [%s] -> [%s]\n\n",
				    prSta->aucMacAddr,
				    apucDebugSecState[prSecInfo->eCurrentState], apucDebugSecState[eNextState]);
#else
		DBGLOG(RSN, STATE, "\n %pM [%d] TRANSITION: [%d] -> [%d]\n\n",
				    prSta->aucMacAddr, DBG_RSN_IDX, prSecInfo->eCurrentState, eNextState);
#endif
		prSecInfo->eCurrentState = eNextState;

		fgIsTransition = (BOOLEAN) FALSE;
#if 0
		/* Do tasks of the State that we just entered */
		switch (prSecInfo->eCurrentState) {
		case SEC_STATE_INIT:
			break;
		case SEC_STATE_INITIATOR_PORT_BLOCKED:
			break;
		case SEC_STATE_RESPONDER_PORT_BLOCKED:
			break;
		case SEC_STATE_CHECK_OK:
			break;
		case SEC_STATE_SEND_EAPOL:
			break;
		case SEC_STATE_SEND_DEAUTH:
			break;
		case SEC_STATE_COUNTERMEASURE:
			break;
		default:
			ASSERT(0);	/* Make sure we have handle all STATEs */
			break;
		}
#endif
	} while (fgIsTransition);

	return;

}

/*----------------------------------------------------------------------------*/
/*!
* \brief This function will do initialization of Security FSM and all variables in
*        SEC_INFO_T.
*
* \param[in] prSta            Pointer to the Sta record
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID secFsmEventStart(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prSta)
{
	P_SEC_INFO_T prSecInfo;
	BOOLEAN fgIsTransition = (BOOLEAN) FALSE;
	ENUM_SEC_STATE_T eNextState;

	DBGLOG(RSN, TRACE, "secFsmRunEventStart\n");

	ASSERT(prSta);

	if (!prSta)
		return;

	if (!IS_STA_IN_AIS(prSta))
		return;

	DBGLOG(RSN, TRACE, "secFsmRunEventStart for sta %pM network %d\n",
			    prSta->aucMacAddr, prSta->ucNetTypeIndex);

	prSecInfo = (P_SEC_INFO_T) &prSta->rSecInfo;

	eNextState = prSecInfo->eCurrentState;

	secSetPortBlocked(prAdapter, prSta, TRUE);

	/* prSta->fgTransmitKeyExist = FALSE; */
	/* whsu:: nicPrivacySetStaDefaultWTIdx(prSta); */

#if 1				/* Since the 1x and key can set to firmware in order, always enter the check ok state */
	SEC_STATE_TRANSITION(prAdapter, prSta, INIT, CHECK_OK);
#else
	if (IS_STA_IN_AIS(prSta->eStaType)) {
		if (secRsnKeyHandshakeEnabled(prAdapter) == TRUE
#if CFG_SUPPORT_WAPI
		    || (prAdapter->rWifiVar.rConnSettings.fgWapiMode)
#endif
		    ) {
			prSta->fgTransmitKeyExist = FALSE;
			/* nicPrivacyInitialize(prSta->ucNetTypeIndex); */
			SEC_STATE_TRANSITION(prAdapter, prSta, INIT, INITIATOR_PORT_BLOCKED);
		} else {
			SEC_STATE_TRANSITION(prAdapter, prSta, INIT, CHECK_OK);
		}
	}
#if CFG_ENABLE_WIFI_DIRECT || CFG_ENABLE_BT_OVER_WIFI
#if CFG_ENABLE_WIFI_DIRECT && CFG_ENABLE_BT_OVER_WIFI
	else if ((prSta->eStaType == STA_TYPE_BOW_CLIENT) || (prSta->eStaType == STA_TYPE_P2P_GC)) {
#elif CFG_ENABLE_WIFI_DIRECT
	else if (prSta->eStaType == STA_TYPE_P2P_GC) {
#elif CFG_ENABLE_BT_OVER_WIFI
	else if (prSta->eStaType == STA_TYPE_BOW_CLIENT) {
#endif
		SEC_STATE_TRANSITION(prAdapter, prSta, INIT, RESPONDER_PORT_BLOCKED);
	}
#endif
	else
		SEC_STATE_TRANSITION(prAdapter, prSta, INIT, INITIATOR_PORT_BLOCKED);
#endif
	if (prSecInfo->eCurrentState != eNextState)
		secFsmSteps(prAdapter, prSta, eNextState);

}				/* secFsmRunEventStart */

/*----------------------------------------------------------------------------*/
/*!
* \brief This function called by reset procedure to force the sec fsm enter
*        idle state
*
* \param[in] ucNetTypeIdx  The Specific Network type index
* \param[in] prSta         Pointer to the Sta record
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID secFsmEventAbort(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prSta)
{
	P_SEC_INFO_T prSecInfo;

	DBGLOG(RSN, TRACE, "secFsmEventAbort for sta %pM network %d\n",
			    prSta->aucMacAddr, prSta->ucNetTypeIndex);

	ASSERT(prSta);

	if (!prSta)
		return;

	if (!IS_STA_IN_AIS(prSta))
		return;

	prSecInfo = (P_SEC_INFO_T) &prSta->rSecInfo;

	prSta->fgTransmitKeyExist = FALSE;

	secSetPortBlocked(prAdapter, prSta, TRUE);

	if (prSecInfo == NULL)
		return;

	if (IS_STA_IN_AIS(prSta)) {

		prAdapter->rWifiVar.rAisSpecificBssInfo.fgTransmitKeyExist = FALSE;

		if (prSecInfo->eCurrentState == SEC_STATE_SEND_EAPOL) {
			if (prAdapter->rWifiVar.rAisSpecificBssInfo.fgCheckEAPoLTxDone == FALSE) {
				DBGLOG(RSN, TRACE, "EAPOL STATE not match the flag\n");
				/*
				 * cnmTimerStopTimer(prAdapter, &prAdapter->rWifiVar.
				 * rAisSpecificBssInfo.rRsnaEAPoLReportTimeoutTimer);
				 */
			}
		}
	}
	prSecInfo->eCurrentState = SEC_STATE_INIT;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief This function will indicate an Event of "2nd EAPoL Tx is sending" to Sec FSM.
*
* \param[in] prSta            Pointer to the Sta record
*
* \return -
*/
/*----------------------------------------------------------------------------*/
VOID secFsmEvent2ndEapolTx(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prSta)
{
	P_SEC_INFO_T prSecInfo;
	ENUM_SEC_STATE_T eNextState;
	/* BOOLEAN                 fgIsTransition = (BOOLEAN)FALSE; */

	DEBUGFUNC("secFsmRunEvent2ndEapolTx");

	ASSERT(prSta);

	prSecInfo = &prSta->rSecInfo;
	eNextState = prSecInfo->eCurrentState;

#if DBG
	DBGLOG(RSN, TRACE, "%pM Sec state %s\n", prSta->aucMacAddr,
			    apucDebugSecState[prSecInfo->eCurrentState]);
#else
	DBGLOG(RSN, TRACE, "%pM Sec state [%d]\n", prSta->aucMacAddr, prSecInfo->eCurrentState);
#endif

	switch (prSecInfo->eCurrentState) {
	case SEC_STATE_INITIATOR_PORT_BLOCKED:
	case SEC_STATE_CHECK_OK:
		prSecInfo->fg2nd1xSend = TRUE;
		break;
	default:
#if DBG
		DBGLOG(RSN, WARN, "Rcv 2nd EAPoL at %s\n", apucDebugSecState[prSecInfo->eCurrentState]);
#else
		DBGLOG(RSN, WARN, "Rcv 2nd EAPoL at [%d]\n", prSecInfo->eCurrentState);
#endif
		break;
	}

	if (prSecInfo->eCurrentState != eNextState)
		secFsmSteps(prAdapter, prSta, eNextState);

	return;

}				/* secFsmRunEvent2ndEapolTx */

/*----------------------------------------------------------------------------*/
/*!
* \brief This function will indicate an Event of "4th EAPoL Tx is Tx done" to Sec FSM.
*
* \param[in] prSta            Pointer to the Sta record
*
* \return -
*/
/*----------------------------------------------------------------------------*/
VOID secFsmEvent4ndEapolTxDone(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prSta)
{
	P_SEC_INFO_T prSecInfo;
	ENUM_SEC_STATE_T eNextState;
	BOOLEAN fgIsTransition = (BOOLEAN) FALSE;
	P_CMD_802_11_KEY prStoredKey;

	DEBUGFUNC("secFsmRunEvent4ndEapolTx");

	ASSERT(prSta);

	prSecInfo = &prSta->rSecInfo;
	eNextState = prSecInfo->eCurrentState;

#if DBG
	DBGLOG(RSN, TRACE, "%pM Sec state %s\n", prSta->aucMacAddr,
			    apucDebugSecState[prSecInfo->eCurrentState]);
#else
	DBGLOG(RSN, TRACE, "%pM Sec state [%d]\n", prSta->aucMacAddr, prSecInfo->eCurrentState);
#endif

	switch (prSecInfo->eCurrentState) {
	case SEC_STATE_INITIATOR_PORT_BLOCKED:
	case SEC_STATE_CHECK_OK:
		prSecInfo->fg2nd1xSend = FALSE;
		if (prSecInfo->fgKeyStored) {
			prStoredKey = (P_CMD_802_11_KEY) prSecInfo->aucStoredKey;

			/* prSta = rxmLookupStaRecIndexFromTA(prStoredKey->aucPeerAddr); */
			/* if (nicPrivacySetKeyEntry(prStoredKey, prSta->ucWTEntry) == FALSE) */
			/* DBGLOG(RSN, WARN, ("nicPrivacySetKeyEntry() fail,..\n")); */

			/* key update */
			prSecInfo->fgKeyStored = FALSE;
			prSta->fgTransmitKeyExist = TRUE;
		}
		if (prSecInfo->eCurrentState == SEC_STATE_INITIATOR_PORT_BLOCKED)
			SEC_STATE_TRANSITION(prAdapter, prSta, INITIATOR_PORT_BLOCKED, CHECK_OK);
		break;
	default:

#if DBG
		DBGLOG(RSN, WARN, "Rcv thh EAPoL Tx done at %s\n", apucDebugSecState[prSecInfo->eCurrentState]);
#else
		DBGLOG(RSN, WARN, "Rcv thh EAPoL Tx done at [%d]\n", prSecInfo->eCurrentState);
#endif
		break;
	}

	if (prSecInfo->eCurrentState != eNextState)
		secFsmSteps(prAdapter, prSta, eNextState);

	return;

}				/* secFsmRunEvent4ndEapolTx */

/*----------------------------------------------------------------------------*/
/*!
* \brief This function will indicate an Event of "Pairwise key installed" to SEC FSM.
*
* \param[in] prSta            Pointer to the Sta record
*
* \retval TRUE The key can be installed to HW
* \retval FALSE The kay conflict with the current key, abort it
*/
/*----------------------------------------------------------------------------*/
BOOLEAN secFsmEventPTKInstalled(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prSta)
{
	P_SEC_INFO_T prSecInfo;
	ENUM_SEC_STATE_T eNextState;
	BOOLEAN fgStatus = TRUE;
	BOOLEAN fgIsTransition = (BOOLEAN) FALSE;

	ASSERT(prSta);

	prSecInfo = &prSta->rSecInfo;
	if (prSecInfo == NULL)
		return TRUE;	/* Not PTK */

#if DBG
	DBGLOG(RSN, TRACE, "%pM Sec state %s\n", prSta->aucMacAdd),
			    apucDebugSecState[prSecInfo->eCurrentState]);
#else
	DBGLOG(RSN, TRACE, "%pM Sec state [%d]\n", prSta->aucMacAddr, prSecInfo->eCurrentState);
#endif

	eNextState = prSecInfo->eCurrentState;

	switch (prSecInfo->eCurrentState) {
	case SEC_STATE_INIT:
		/* Legacy wep, wpa-none */
		break;

	case SEC_STATE_INITIATOR_PORT_BLOCKED:
		if (prSecInfo->fg2nd1xSend)
			;
		else
			SEC_STATE_TRANSITION(prAdapter, prSta, INITIATOR_PORT_BLOCKED, CHECK_OK);
		break;

	case SEC_STATE_RESPONDER_PORT_BLOCKED:
		SEC_STATE_TRANSITION(prAdapter, prSta, RESPONDER_PORT_BLOCKED, CHECK_OK);
		break;

	case SEC_STATE_CHECK_OK:
		break;

	default:
		fgStatus = FALSE;
		break;
	}

	if (prSecInfo->eCurrentState != eNextState)
		secFsmSteps(prAdapter, prSta, eNextState);

	return fgStatus;

}				/* end of secFsmRunEventPTKInstalled() */

/*----------------------------------------------------------------------------*/
/*!
* \brief This function will indicate an Event of "Counter Measure" to SEC FSM.
*
* \param[in] prSta            Pointer to the Sta record
*
* \return -
*/
/*----------------------------------------------------------------------------*/
VOID secFsmEventStartCounterMeasure(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prSta)
{
	P_SEC_INFO_T prSecInfo;
	ENUM_SEC_STATE_T eNextState;
	BOOLEAN fgIsTransition = (BOOLEAN) FALSE;

	DEBUGFUNC("secFsmRunEventStartCounterMeasure");

	ASSERT(prSta);

	if (!IS_STA_IN_AIS(prSta)) {
		DBGLOG(RSN, INFO, "Counter Measure should occur at AIS network!!\n");
		/* ASSERT(0); */
		return;
	}

	prSecInfo = &prSta->rSecInfo;

	eNextState = prSecInfo->eCurrentState;

#if DBG
	DBGLOG(RSN, TRACE, "%pM Sec state %s\n", prSta->aucMacAddr,
			    apucDebugSecState[prSecInfo->eCurrentState]);
#else
	DBGLOG(RSN, TRACE, "%pM Sec state [%d]\n", prSta->aucMacAddr, prSecInfo->eCurrentState);
#endif

	prAdapter->rWifiVar.rAisSpecificBssInfo.u4RsnaLastMICFailTime = 0;

	switch (prSecInfo->eCurrentState) {
	case SEC_STATE_CHECK_OK:
		{
			prAdapter->rWifiVar.rAisSpecificBssInfo.fgCounterMeasure = TRUE;

			/* <Todo> dls port control */
			SEC_STATE_TRANSITION(prAdapter, prSta, CHECK_OK, SEND_EAPOL);
		}
		break;

	default:
		break;
	}

	/* Call arbFsmSteps() when we are going to change ARB STATE */
	if (prSecInfo->eCurrentState != eNextState)
		secFsmSteps(prAdapter, prSta, eNextState);

	return;

}				/* secFsmRunEventStartCounterMeasure */

/*----------------------------------------------------------------------------*/
/*!
* \brief This function will indicate an Event of "802.1x EAPoL Tx Done" to Sec FSM.
*
* \param[in] prSta            Pointer to the Sta record
*
* \return -
*/
/*----------------------------------------------------------------------------*/
VOID
secFsmEventEapolTxDone(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prStaRec, IN ENUM_TX_RESULT_CODE_T rTxDoneStatus)
{
	P_SEC_INFO_T prSecInfo;
	ENUM_SEC_STATE_T eNextState;
	BOOLEAN fgIsTransition = (BOOLEAN) FALSE;
	P_AIS_SPECIFIC_BSS_INFO_T prAisBssInfo;

	DEBUGFUNC("secFsmRunEventEapolTxDone");

	ASSERT(prStaRec);

	if (rTxDoneStatus != TX_RESULT_SUCCESS) {
		DBGLOG(RSN, INFO, "Error EAPoL fram fail to send!!\n");
		/* ASSERT(0); */
		return;
	}

	if (!IS_STA_IN_AIS(prStaRec)) {
		DBGLOG(RSN, INFO, "Counter Measure should occur at AIS network!!\n");
		/* ASSERT(0); */
		return;
	}

	prAisBssInfo = &prAdapter->rWifiVar.rAisSpecificBssInfo;

	ASSERT(prAisBssInfo);

	prSecInfo = &prStaRec->rSecInfo;
	eNextState = prSecInfo->eCurrentState;

#if DBG
	DBGLOG(RSN, TRACE, "%pM Sec state %s\n", prStaRec->aucMacAddr,
			    apucDebugSecState[prSecInfo->eCurrentState]);
#else
	DBGLOG(RSN, TRACE, "%pM Sec state [%d]\n", prStaRec->aucMacAddr, prSecInfo->eCurrentState);
#endif

	switch (prSecInfo->eCurrentState) {
	case SEC_STATE_SEND_EAPOL:
		if (prAisBssInfo->fgCheckEAPoLTxDone == FALSE)
			ASSERT(0);

		prAisBssInfo->fgCheckEAPoLTxDone = FALSE;
		/* cnmTimerStopTimer(prAdapter, &prAisBssInfo->rRsnaEAPoLReportTimeoutTimer); */

		SEC_STATE_TRANSITION(prAdapter, prStaRec, SEND_EAPOL, SEND_DEAUTH);
		break;
	default:
		break;
	}

	if (prSecInfo->eCurrentState != eNextState)
		secFsmSteps(prAdapter, prStaRec, eNextState);

	return;

}				/* secFsmRunEventEapolTxDone */

/*----------------------------------------------------------------------------*/
/*!
* \brief This function will indicate an Event of "Deauth frame Tx Done" to Sec FSM.
*
* \param[in] pMsduInfo            Pointer to the Msdu Info
* \param[in] rStatus              The Tx done status
*
* \return -
*
* \note after receive deauth frame, callback function call this
*/
/*----------------------------------------------------------------------------*/
VOID
secFsmEventDeauthTxDone(IN P_ADAPTER_T prAdapter, IN P_MSDU_INFO_T prMsduInfo, IN ENUM_TX_RESULT_CODE_T rTxDoneStatus)
{
	P_STA_RECORD_T prStaRec;
	P_SEC_INFO_T prSecInfo;
	ENUM_SEC_STATE_T eNextState;
	BOOLEAN fgIsTransition = (BOOLEAN) FALSE;

	DEBUGFUNC("secFsmRunEventDeauthTxDone");

	ASSERT(prMsduInfo);

	prStaRec = cnmGetStaRecByIndex(prAdapter, prMsduInfo->ucStaRecIndex);

	ASSERT(prStaRec);

	if (!prStaRec)
		return;

	if (!IS_STA_IN_AIS(prStaRec)) {
		DBGLOG(RSN, INFO, "Counter Measure should occur at AIS network!!\n");
		/* ASSERT(0); */
		return;
	}

	prSecInfo = (P_SEC_INFO_T) &prStaRec->rSecInfo;

#if DBG
	DBGLOG(RSN, TRACE, "%pM Sec state %s\n", prStaRec->aucMacAddr,
			    apucDebugSecState[prSecInfo->eCurrentState]);
#else
	DBGLOG(RSN, TRACE, "%pM Sec state [%d]\n", prStaRec->aucMacAddr, prSecInfo->eCurrentState);
#endif

	switch (prSecInfo->eCurrentState) {
	case SEC_STATE_SEND_DEAUTH:

		DBGLOG(RSN, TRACE, "Set timer %d\n", COUNTER_MEASURE_TIMEOUT_INTERVAL_SEC);

		SEC_STATE_TRANSITION(prAdapter, prStaRec, SEND_DEAUTH, COUNTERMEASURE);

		break;

	default:
		ASSERT(0);
		break;
	}

}				/* secFsmRunEventDeauthTxDone */

/*----------------------------------------------------------------------------*/
/*!
* \brief This function will check the eapol error frame fail to send issue.
*
* \param[in] prSta            Pointer to the Sta record
*
* \return -
*/
/*----------------------------------------------------------------------------*/
VOID secFsmEventEapolTxTimeout(IN P_ADAPTER_T prAdapter, IN ULONG ulParm)
{
	P_STA_RECORD_T prStaRec;

	DEBUGFUNC("secFsmRunEventEapolTxTimeout");

	prStaRec = (P_STA_RECORD_T) ulParm;

	ASSERT(prStaRec);

	/* Todo:: How to handle the Eapol Error fail to send case? */
	ASSERT(0);

	return;

}				/* secFsmEventEapolTxTimeout */

/*----------------------------------------------------------------------------*/
/*!
* \brief This function will stop the counterMeasure duration.
*
* \param[in] prSta            Pointer to the Sta record
*
* \return -
*/
/*----------------------------------------------------------------------------*/
VOID secFsmEventEndOfCounterMeasure(IN P_ADAPTER_T prAdapter, ULONG ulParm)
{
	P_STA_RECORD_T prSta;
	P_SEC_INFO_T prSecInfo;
	ENUM_SEC_STATE_T eNextState;
	BOOLEAN fgIsTransition = (BOOLEAN) FALSE;

	DEBUGFUNC("secFsmRunEventEndOfCounterMeasure");

	prSta = (P_STA_RECORD_T) ulParm;

	ASSERT(prSta);

	if (!IS_STA_IN_AIS(prSta)) {
		DBGLOG(RSN, INFO, "Counter Measure should occur at AIS network!!\n");
		/* ASSERT(0); */
		return;
	}

	prSecInfo = &prSta->rSecInfo;
	eNextState = prSecInfo->eCurrentState;

#if DBG
	DBGLOG(RSN, TRACE, "%pM Sec state %s\n", prSta->aucMacAddr,
			    apucDebugSecState[prSecInfo->eCurrentState]);
#else
	DBGLOG(RSN, TRACE, "%pM Sec state [%d]\n", prSta->aucMacAddr, prSecInfo->eCurrentState);
#endif

	switch (prSecInfo->eCurrentState) {
	case SEC_STATE_SEND_DEAUTH:
		{
			prAdapter->rWifiVar.rAisSpecificBssInfo.fgCounterMeasure = FALSE;

			SEC_STATE_TRANSITION(prAdapter, prSta, COUNTERMEASURE, INIT);
		}
		break;

	default:
		ASSERT(0);
	}

	/* Call arbFsmSteps() when we are going to change ARB STATE */
	if (prSecInfo->eCurrentState != eNextState)
		secFsmSteps(prAdapter, prSta, eNextState);

}				/* end of secFsmRunEventEndOfCounterMeasure */
#endif
