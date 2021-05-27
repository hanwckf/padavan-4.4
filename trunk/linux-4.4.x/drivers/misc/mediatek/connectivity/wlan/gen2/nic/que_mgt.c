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
OS_SYSTIME g_arMissTimeout[CFG_STA_REC_NUM][CFG_RX_MAX_BA_TID_NUM];

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/
#if ARP_MONITER_ENABLE
static UINT_16 arpMoniter;
static UINT_8 apIp[4];
#endif
/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/

/*******************************************************************************
*                   F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
static inline VOID qmDetermineStaRecIndex(IN P_ADAPTER_T prAdapter, IN P_MSDU_INFO_T prMsduInfo);

static inline VOID
qmDequeueTxPacketsFromPerStaQueues(IN P_ADAPTER_T prAdapter,
				   OUT P_QUE_T prQue,
				   IN UINT_8 ucTC, IN UINT_8 ucCurrentAvailableQuota, IN UINT_8 ucTotalQuota);

static inline VOID
qmDequeueTxPacketsFromPerTypeQueues(IN P_ADAPTER_T prAdapter, OUT P_QUE_T prQue, IN UINT_8 ucTC, IN UINT_8 ucMaxNum);

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

/*----------------------------------------------------------------------------*/
/*!
* \brief Init Queue Management for TX
*
* \param[in] (none)
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/
VOID qmInit(IN P_ADAPTER_T prAdapter)
{
	UINT_32 u4QueArrayIdx;
	UINT_32 i;

	P_QUE_MGT_T prQM = &prAdapter->rQM;

	/* DbgPrint("QM: Enter qmInit()\n"); */
#if CFG_SUPPORT_QOS
	prAdapter->rWifiVar.fgSupportQoS = TRUE;
#else
	prAdapter->rWifiVar.fgSupportQoS = FALSE;
#endif

#if CFG_SUPPORT_AMPDU_RX
	prAdapter->rWifiVar.fgSupportAmpduRx = TRUE;
#else
	prAdapter->rWifiVar.fgSupportAmpduRx = FALSE;
#endif

#if CFG_SUPPORT_AMPDU_TX
	prAdapter->rWifiVar.fgSupportAmpduTx = TRUE;
#else
	prAdapter->rWifiVar.fgSupportAmpduTx = FALSE;
#endif

#if CFG_SUPPORT_TSPEC
	prAdapter->rWifiVar.fgSupportTspec = TRUE;
#else
	prAdapter->rWifiVar.fgSupportTspec = FALSE;
#endif

#if CFG_SUPPORT_UAPSD
	prAdapter->rWifiVar.fgSupportUAPSD = TRUE;
#else
	prAdapter->rWifiVar.fgSupportUAPSD = FALSE;
#endif

#if CFG_SUPPORT_UL_PSMP
	prAdapter->rWifiVar.fgSupportULPSMP = TRUE;
#else
	prAdapter->rWifiVar.fgSupportULPSMP = FALSE;
#endif

#if CFG_SUPPORT_RX_SGI
	prAdapter->rWifiVar.u8SupportRxSgi20 = 0;
	prAdapter->rWifiVar.u8SupportRxSgi40 = 0;
#else
	prAdapter->rWifiVar.u8SupportRxSgi20 = 2;
	prAdapter->rWifiVar.u8SupportRxSgi40 = 2;
#endif

#if CFG_SUPPORT_RX_HT_GF
	prAdapter->rWifiVar.u8SupportRxGf = 0;
#else
	prAdapter->rWifiVar.u8SupportRxGf = 2;
#endif

	/* 4 <2> Initialize other TX queues (queues not in STA_RECs) */
	for (u4QueArrayIdx = 0; u4QueArrayIdx < NUM_OF_PER_TYPE_TX_QUEUES; u4QueArrayIdx++)
		QUEUE_INITIALIZE(&(prQM->arTxQueue[u4QueArrayIdx]));

	/* 4 <3> Initialize the RX BA table and RX queues */
	/* Initialize the RX Reordering Parameters and Queues */
	for (u4QueArrayIdx = 0; u4QueArrayIdx < CFG_NUM_OF_RX_BA_AGREEMENTS; u4QueArrayIdx++) {
		prQM->arRxBaTable[u4QueArrayIdx].fgIsValid = FALSE;
		QUEUE_INITIALIZE(&(prQM->arRxBaTable[u4QueArrayIdx].rReOrderQue));
#if CFG_RX_BA_REORDERING_ENHANCEMENT
		QUEUE_INITIALIZE(&(prQM->arRxBaTable[u4QueArrayIdx].rNoNeedWaitQue));
#endif
		prQM->arRxBaTable[u4QueArrayIdx].u2WinStart = 0xFFFF;
		prQM->arRxBaTable[u4QueArrayIdx].u2WinEnd = 0xFFFF;

		prQM->arRxBaTable[u4QueArrayIdx].fgIsWaitingForPktWithSsn = FALSE;
		prQM->arRxBaTable[u4QueArrayIdx].fgHasBubble = FALSE;
		cnmTimerInitTimer(prAdapter,
				&(prQM->arRxBaTable[u4QueArrayIdx].rReorderBubbleTimer),
				(PFN_MGMT_TIMEOUT_FUNC) qmHandleReorderBubbleTimeout,
				(ULONG) (&prQM->arRxBaTable[u4QueArrayIdx]));

	}
	prQM->ucRxBaCount = 0;

	kalMemSet(&g_arMissTimeout, 0, sizeof(g_arMissTimeout));

#if QM_ADAPTIVE_TC_RESOURCE_CTRL
	/* 4 <4> Initialize TC resource control variables */
	for (i = 0; i < TC_NUM; i++)
		prQM->au4AverageQueLen[i] = 0;
	prQM->u4TimeToAdjustTcResource = QM_INIT_TIME_TO_ADJUST_TC_RSC;
	prQM->u4TimeToUpdateQueLen = QM_INIT_TIME_TO_UPDATE_QUE_LEN;
	prQM->u4TxNumOfVi = 0;
	prQM->u4TxNumOfVo = 0;

/* ASSERT(prQM->u4TimeToAdjust && prQM->u4TimeToUpdateQueLen); */

	/* 1 20 1 1 4 1 */
	prQM->au4CurrentTcResource[TC0_INDEX] = NIC_TX_BUFF_COUNT_TC0;
	prQM->au4CurrentTcResource[TC1_INDEX] = NIC_TX_BUFF_COUNT_TC1;
	prQM->au4CurrentTcResource[TC2_INDEX] = NIC_TX_BUFF_COUNT_TC2;
	prQM->au4CurrentTcResource[TC3_INDEX] = NIC_TX_BUFF_COUNT_TC3;
	prQM->au4CurrentTcResource[TC4_INDEX] = NIC_TX_BUFF_COUNT_TC4;	/* Not adjustable (TX port 1) */
	prQM->au4CurrentTcResource[TC5_INDEX] = NIC_TX_BUFF_COUNT_TC5;
	DBGLOG(QM, TRACE, "QM: NIC_TX_BUFF_COUNT_TC0 = %d\n", NIC_TX_BUFF_COUNT_TC0);
	DBGLOG(QM, TRACE, "QM: NIC_TX_BUFF_COUNT_TC1 = %d\n", NIC_TX_BUFF_COUNT_TC1);
	DBGLOG(QM, TRACE, "QM: NIC_TX_BUFF_COUNT_TC2 = %d\n", NIC_TX_BUFF_COUNT_TC2);
	DBGLOG(QM, TRACE, "QM: NIC_TX_BUFF_COUNT_TC3 = %d\n", NIC_TX_BUFF_COUNT_TC3);
	DBGLOG(QM, TRACE, "QM: NIC_TX_BUFF_COUNT_TC4 = %d\n", NIC_TX_BUFF_COUNT_TC4);
	DBGLOG(QM, TRACE, "QM: NIC_TX_BUFF_COUNT_TC5 = %d\n", NIC_TX_BUFF_COUNT_TC5);

	/* 1 1 1 1 2 1 */
	prQM->au4MinReservedTcResource[TC0_INDEX] = QM_MIN_RESERVED_TC0_RESOURCE;
	prQM->au4MinReservedTcResource[TC1_INDEX] = QM_MIN_RESERVED_TC1_RESOURCE;
	prQM->au4MinReservedTcResource[TC2_INDEX] = QM_MIN_RESERVED_TC2_RESOURCE;
	prQM->au4MinReservedTcResource[TC3_INDEX] = QM_MIN_RESERVED_TC3_RESOURCE;
	prQM->au4MinReservedTcResource[TC4_INDEX] = QM_MIN_RESERVED_TC4_RESOURCE;	/* Not adjustable (TX port 1) */
	prQM->au4MinReservedTcResource[TC5_INDEX] = QM_MIN_RESERVED_TC5_RESOURCE;

	/* 4 4 6 6 2 4 */
	prQM->au4GuaranteedTcResource[TC0_INDEX] = QM_GUARANTEED_TC0_RESOURCE;
	prQM->au4GuaranteedTcResource[TC1_INDEX] = QM_GUARANTEED_TC1_RESOURCE;
	prQM->au4GuaranteedTcResource[TC2_INDEX] = QM_GUARANTEED_TC2_RESOURCE;
	prQM->au4GuaranteedTcResource[TC3_INDEX] = QM_GUARANTEED_TC3_RESOURCE;
	prQM->au4GuaranteedTcResource[TC4_INDEX] = QM_GUARANTEED_TC4_RESOURCE;
	prQM->au4GuaranteedTcResource[TC5_INDEX] = QM_GUARANTEED_TC5_RESOURCE;

	prQM->fgTcResourcePostAnnealing = FALSE;

	ASSERT(QM_INITIAL_RESIDUAL_TC_RESOURCE < 64);
#endif

#if QM_TEST_MODE
	prQM->u4PktCount = 0;

#if QM_TEST_FAIR_FORWARDING

	prQM->u4CurrentStaRecIndexToEnqueue = 0;
	{
		UINT_8 aucMacAddr[MAC_ADDR_LEN];
		P_STA_RECORD_T prStaRec;

		/* Irrelevant in case this STA is an AIS AP (see qmDetermineStaRecIndex()) */
		aucMacAddr[0] = 0x11;
		aucMacAddr[1] = 0x22;
		aucMacAddr[2] = 0xAA;
		aucMacAddr[3] = 0xBB;
		aucMacAddr[4] = 0xCC;
		aucMacAddr[5] = 0xDD;

		prStaRec = &prAdapter->arStaRec[1];
		ASSERT(prStaRec);

		prStaRec->fgIsValid = TRUE;
		prStaRec->fgIsQoS = TRUE;
		prStaRec->fgIsInPS = FALSE;
		prStaRec->ucPsSessionID = 0xFF;
		prStaRec->ucNetTypeIndex = NETWORK_TYPE_AIS_INDEX;
		prStaRec->fgIsAp = TRUE;
		COPY_MAC_ADDR((prStaRec)->aucMacAddr, aucMacAddr);

	}

#endif

#endif

#if QM_FORWARDING_FAIRNESS
	{
		UINT_32 i;

		for (i = 0; i < NUM_OF_PER_STA_TX_QUEUES; i++) {
			prQM->au4ForwardCount[i] = 0;
			prQM->au4HeadStaRecIndex[i] = 0;
		}
	}
#endif

#if QM_TC_RESOURCE_EMPTY_COUNTER
	kalMemZero(prQM->au4QmTcResourceEmptyCounter, sizeof(prQM->au4QmTcResourceEmptyCounter));
#endif

}

#if QM_TEST_MODE
VOID qmTestCases(IN P_ADAPTER_T prAdapter)
{
	P_QUE_MGT_T prQM = &prAdapter->rQM;

	DbgPrint("QM: ** TEST MODE **\n");

	if (QM_TEST_STA_REC_DETERMINATION) {
		if (prAdapter->arStaRec[0].fgIsValid) {
			prAdapter->arStaRec[0].fgIsValid = FALSE;
			DbgPrint("QM: (Test) Deactivate STA_REC[0]\n");
		} else {
			prAdapter->arStaRec[0].fgIsValid = TRUE;
			DbgPrint("QM: (Test) Activate STA_REC[0]\n");
		}
	}

	if (QM_TEST_STA_REC_DEACTIVATION) {
		/* Note that QM_STA_REC_HARD_CODING shall be set to 1 for this test */

		if (prAdapter->arStaRec[0].fgIsValid) {

			DbgPrint("QM: (Test) Deactivate STA_REC[0]\n");
			qmDeactivateStaRec(prAdapter, 0);
		} else {

			UINT_8 aucMacAddr[MAC_ADDR_LEN];

			/* Irrelevant in case this STA is an AIS AP (see qmDetermineStaRecIndex()) */
			aucMacAddr[0] = 0x11;
			aucMacAddr[1] = 0x22;
			aucMacAddr[2] = 0xAA;
			aucMacAddr[3] = 0xBB;
			aucMacAddr[4] = 0xCC;
			aucMacAddr[5] = 0xDD;

			DbgPrint("QM: (Test) Activate STA_REC[0]\n");
			qmActivateStaRec(prAdapter,	/* Adapter pointer */
					 0,	/* STA_REC index from FW */
					 TRUE,	/* fgIsQoS */
					 NETWORK_TYPE_AIS_INDEX,	/* Network type */
					 TRUE,	/* fgIsAp */
					 aucMacAddr	/* MAC address */
			    );
		}
	}

	if (QM_TEST_FAIR_FORWARDING) {
		if (prAdapter->arStaRec[1].fgIsValid) {
			prQM->u4CurrentStaRecIndexToEnqueue++;
			prQM->u4CurrentStaRecIndexToEnqueue %= 2;
			DbgPrint("QM: (Test) Switch to STA_REC[%u]\n", prQM->u4CurrentStaRecIndexToEnqueue);
		}
	}

}
#endif

/*----------------------------------------------------------------------------*/
/*!
* \brief Activate a STA_REC
*
* \param[in] prAdapter  Pointer to the Adapter instance
* \param[in] u4StaRecIdx The index of the STA_REC
* \param[in] fgIsQoS    Set to TRUE if this is a QoS STA
* \param[in] pucMacAddr The MAC address of the STA
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/
VOID qmActivateStaRec(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prStaRec)
{

	/* 4 <1> Deactivate first */
	ASSERT(prStaRec);

	if (prStaRec->fgIsValid) {	/* The STA_REC has been activated */
		DBGLOG(QM, WARN, "QM: (WARNING) Activating a STA_REC which has been activated\n");
		DBGLOG(QM, WARN, "QM: (WARNING) Deactivating a STA_REC before re-activating\n");
		/* To flush TX/RX queues and del RX BA agreements */
		qmDeactivateStaRec(prAdapter, prStaRec->ucIndex);
	}
	/* 4 <2> Activate the STA_REC */
	/* Init the STA_REC */
	prStaRec->fgIsValid = TRUE;
	prStaRec->fgIsInPS = FALSE;
	prStaRec->ucPsSessionID = 0xFF;
	prStaRec->fgIsAp = (IS_AP_STA(prStaRec)) ? TRUE : FALSE;

	/* Done in qmInit() or qmDeactivateStaRec() */
#if 0
	/* At the beginning, no RX BA agreements have been established */
	for (i = 0; i < CFG_RX_MAX_BA_TID_NUM; i++)
		(prStaRec->aprRxReorderParamRefTbl)[i] = NULL;
#endif

	DBGLOG(QM, TRACE, "QM: +STA[%u]\n", (UINT_32) prStaRec->ucIndex);
}

/*----------------------------------------------------------------------------*/
/*!
* \brief Deactivate a STA_REC
*
* \param[in] prAdapter  Pointer to the Adapter instance
* \param[in] u4StaRecIdx The index of the STA_REC
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/
VOID qmDeactivateStaRec(IN P_ADAPTER_T prAdapter, IN UINT_32 u4StaRecIdx)
{
	P_STA_RECORD_T prStaRec;
	UINT_32 i;
	P_MSDU_INFO_T prFlushedTxPacketList = NULL;

	ASSERT(u4StaRecIdx < CFG_NUM_OF_STA_RECORD);

	prStaRec = &prAdapter->arStaRec[u4StaRecIdx];
	ASSERT(prStaRec);

	/* 4<1> Flush TX queues */
	prFlushedTxPacketList = qmFlushStaTxQueues(prAdapter, u4StaRecIdx);

	if (prFlushedTxPacketList)
		wlanProcessQueuedMsduInfo(prAdapter, prFlushedTxPacketList);
	/* 4 <2> Flush RX queues and delete RX BA agreements */
	for (i = 0; i < CFG_RX_MAX_BA_TID_NUM; i++) {
		/* Delete the RX BA entry with TID = i */
		qmDelRxBaEntry(prAdapter, (UINT_8) u4StaRecIdx, (UINT_8) i, FALSE);
	}

	/* 4 <3> Deactivate the STA_REC */
	prStaRec->fgIsValid = FALSE;
	prStaRec->fgIsInPS = FALSE;

	/* To reduce printk for IOT sta to connect all the time, */
	/* DBGLOG(QM, INFO, ("QM: -STA[%ld]\n", u4StaRecIdx)); */
}

/*----------------------------------------------------------------------------*/
/*!
* \brief Deactivate a STA_REC
*
* \param[in] prAdapter  Pointer to the Adapter instance
* \param[in] u4StaRecIdx The index of the network
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/

VOID qmFreeAllByNetType(IN P_ADAPTER_T prAdapter, IN ENUM_NETWORK_TYPE_INDEX_T eNetworkTypeIdx)
{

	P_QUE_MGT_T prQM;
	P_QUE_T prQue;
	QUE_T rNeedToFreeQue;
	QUE_T rTempQue;
	P_QUE_T prNeedToFreeQue;
	P_QUE_T prTempQue;
	P_MSDU_INFO_T prMsduInfo;

	prQM = &prAdapter->rQM;
	prQue = &prQM->arTxQueue[TX_QUEUE_INDEX_BMCAST];

	QUEUE_INITIALIZE(&rNeedToFreeQue);
	QUEUE_INITIALIZE(&rTempQue);

	prNeedToFreeQue = &rNeedToFreeQue;
	prTempQue = &rTempQue;

	QUEUE_MOVE_ALL(prTempQue, prQue);

	QUEUE_REMOVE_HEAD(prTempQue, prMsduInfo, P_MSDU_INFO_T);
	while (prMsduInfo) {

		if (prMsduInfo->ucNetworkType == eNetworkTypeIdx) {
			/* QUEUE_INSERT_TAIL */
			QUEUE_INSERT_TAIL(prNeedToFreeQue, (P_QUE_ENTRY_T) prMsduInfo);
		} else {
			/* QUEUE_INSERT_TAIL */
			QUEUE_INSERT_TAIL(prQue, (P_QUE_ENTRY_T) prMsduInfo);
		}

		QUEUE_REMOVE_HEAD(prTempQue, prMsduInfo, P_MSDU_INFO_T);
	}
	if (QUEUE_IS_NOT_EMPTY(prNeedToFreeQue))
		wlanProcessQueuedMsduInfo(prAdapter, (P_MSDU_INFO_T) QUEUE_GET_HEAD(prNeedToFreeQue));

}

/*----------------------------------------------------------------------------*/
/*!
* \brief Flush all TX queues
*
* \param[in] (none)
*
* \return The flushed packets (in a list of MSDU_INFOs)
*/
/*----------------------------------------------------------------------------*/
P_MSDU_INFO_T qmFlushTxQueues(IN P_ADAPTER_T prAdapter)
{
	UINT_8 ucStaArrayIdx;
	UINT_8 ucQueArrayIdx;

	P_MSDU_INFO_T prMsduInfoListHead;
	P_MSDU_INFO_T prMsduInfoListTail;

	P_QUE_MGT_T prQM = &prAdapter->rQM;

	DBGLOG(QM, TRACE, "QM: Enter qmFlushTxQueues()\n");

	prMsduInfoListHead = NULL;
	prMsduInfoListTail = NULL;

	/* Concatenate all MSDU_INFOs in per-STA queues */
	for (ucStaArrayIdx = 0; ucStaArrayIdx < CFG_NUM_OF_STA_RECORD; ucStaArrayIdx++) {

		/* Always check each STA_REC when flushing packets no matter it is inactive or active */
#if 0
		if (!prAdapter->arStaRec[ucStaArrayIdx].fgIsValid)
			continue;	/* Continue to check the next STA_REC */
#endif

		for (ucQueArrayIdx = 0; ucQueArrayIdx < NUM_OF_PER_STA_TX_QUEUES; ucQueArrayIdx++) {
			if (QUEUE_IS_EMPTY(&(prAdapter->arStaRec[ucStaArrayIdx].arTxQueue[ucQueArrayIdx])))
				continue;	/* Continue to check the next TX queue of the same STA */

			if (!prMsduInfoListHead) {

				/* The first MSDU_INFO is found */
				prMsduInfoListHead = (P_MSDU_INFO_T)
				    QUEUE_GET_HEAD(&prAdapter->arStaRec[ucStaArrayIdx].arTxQueue[ucQueArrayIdx]);
				prMsduInfoListTail = (P_MSDU_INFO_T)
				    QUEUE_GET_TAIL(&prAdapter->arStaRec[ucStaArrayIdx].arTxQueue[ucQueArrayIdx]);
			} else {
				/* Concatenate the MSDU_INFO list with the existing list */
				QM_TX_SET_NEXT_MSDU_INFO(prMsduInfoListTail,
							 QUEUE_GET_HEAD(&prAdapter->
									arStaRec[ucStaArrayIdx].arTxQueue
									[ucQueArrayIdx]));

				prMsduInfoListTail = (P_MSDU_INFO_T)
				    QUEUE_GET_TAIL(&prAdapter->arStaRec[ucStaArrayIdx].arTxQueue[ucQueArrayIdx]);
			}

			QUEUE_INITIALIZE(&prAdapter->arStaRec[ucStaArrayIdx].arTxQueue[ucQueArrayIdx]);
		}
	}

	/* Flush per-Type queues */
	for (ucQueArrayIdx = 0; ucQueArrayIdx < NUM_OF_PER_TYPE_TX_QUEUES; ucQueArrayIdx++) {

		if (QUEUE_IS_EMPTY(&(prQM->arTxQueue[ucQueArrayIdx])))
			continue;	/* Continue to check the next TX queue of the same STA */

		if (!prMsduInfoListHead) {

			/* The first MSDU_INFO is found */
			prMsduInfoListHead = (P_MSDU_INFO_T)
			    QUEUE_GET_HEAD(&prQM->arTxQueue[ucQueArrayIdx]);
			prMsduInfoListTail = (P_MSDU_INFO_T)
			    QUEUE_GET_TAIL(&prQM->arTxQueue[ucQueArrayIdx]);
		} else {
			/* Concatenate the MSDU_INFO list with the existing list */
			QM_TX_SET_NEXT_MSDU_INFO(prMsduInfoListTail, QUEUE_GET_HEAD(&prQM->arTxQueue[ucQueArrayIdx]));

			prMsduInfoListTail = (P_MSDU_INFO_T)
			    QUEUE_GET_TAIL(&prQM->arTxQueue[ucQueArrayIdx]);
		}

		QUEUE_INITIALIZE(&prQM->arTxQueue[ucQueArrayIdx]);

	}

	if (prMsduInfoListTail) {
		/* Terminate the MSDU_INFO list with a NULL pointer */
		QM_TX_SET_NEXT_MSDU_INFO(prMsduInfoListTail, NULL);
	}

	return prMsduInfoListHead;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief Flush TX packets for a particular STA
*
* \param[in] u4StaRecIdx STA_REC index
*
* \return The flushed packets (in a list of MSDU_INFOs)
*/
/*----------------------------------------------------------------------------*/
P_MSDU_INFO_T qmFlushStaTxQueues(IN P_ADAPTER_T prAdapter, IN UINT_32 u4StaRecIdx)
{
	UINT_8 ucQueArrayIdx;
	P_MSDU_INFO_T prMsduInfoListHead;
	P_MSDU_INFO_T prMsduInfoListTail;
	P_STA_RECORD_T prStaRec;

	/* To reduce printk for IOT sta to connect all the time, */
	/* DBGLOG(QM, TRACE, ("QM: Enter qmFlushStaTxQueues(%ld)\n", u4StaRecIdx)); */

	ASSERT(u4StaRecIdx < CFG_NUM_OF_STA_RECORD);

	prMsduInfoListHead = NULL;
	prMsduInfoListTail = NULL;

	prStaRec = &prAdapter->arStaRec[u4StaRecIdx];
	ASSERT(prStaRec);

	/* No matter whether this is an activated STA_REC, do flush */
#if 0
	if (!prStaRec->fgIsValid)
		return NULL;
#endif

	/* Concatenate all MSDU_INFOs in TX queues of this STA_REC */
	for (ucQueArrayIdx = 0; ucQueArrayIdx < NUM_OF_PER_STA_TX_QUEUES; ucQueArrayIdx++) {
		if (QUEUE_IS_EMPTY(&(prStaRec->arTxQueue[ucQueArrayIdx])))
			continue;

		if (!prMsduInfoListHead) {
			/* The first MSDU_INFO is found */
			prMsduInfoListHead = (P_MSDU_INFO_T)
			    QUEUE_GET_HEAD(&prStaRec->arTxQueue[ucQueArrayIdx]);
			prMsduInfoListTail = (P_MSDU_INFO_T)
			    QUEUE_GET_TAIL(&prStaRec->arTxQueue[ucQueArrayIdx]);
		} else {
			/* Concatenate the MSDU_INFO list with the existing list */
			QM_TX_SET_NEXT_MSDU_INFO(prMsduInfoListTail,
						 QUEUE_GET_HEAD(&prStaRec->arTxQueue[ucQueArrayIdx]));

			prMsduInfoListTail = (P_MSDU_INFO_T) QUEUE_GET_TAIL(&prStaRec->arTxQueue[ucQueArrayIdx]);
		}

		QUEUE_INITIALIZE(&prStaRec->arTxQueue[ucQueArrayIdx]);

	}

#if 0
	if (prMsduInfoListTail) {
		/* Terminate the MSDU_INFO list with a NULL pointer */
		QM_TX_SET_NEXT_MSDU_INFO(prMsduInfoListTail, nicGetPendingStaMMPDU(prAdapter, (UINT_8) u4StaRecIdx));
	} else {
		prMsduInfoListHead = nicGetPendingStaMMPDU(prAdapter, (UINT_8) u4StaRecIdx);
	}
#endif

	return prMsduInfoListHead;

}

/*----------------------------------------------------------------------------*/
/*!
* \brief Flush RX packets
*
* \param[in] (none)
*
* \return The flushed packets (in a list of SW_RFBs)
*/
/*----------------------------------------------------------------------------*/
P_SW_RFB_T qmFlushRxQueues(IN P_ADAPTER_T prAdapter)
{
	UINT_32 i;
	P_SW_RFB_T prSwRfbListHead;
	P_SW_RFB_T prSwRfbListTail;
	P_QUE_MGT_T prQM = &prAdapter->rQM;

	prSwRfbListHead = prSwRfbListTail = NULL;

	DBGLOG(QM, TRACE, "QM: Enter qmFlushRxQueues()\n");

	for (i = 0; i < CFG_NUM_OF_RX_BA_AGREEMENTS; i++) {
		if (QUEUE_IS_NOT_EMPTY(&(prQM->arRxBaTable[i].rReOrderQue))) {
			if (!prSwRfbListHead) {

				/* The first MSDU_INFO is found */
				prSwRfbListHead = (P_SW_RFB_T)
				    QUEUE_GET_HEAD(&(prQM->arRxBaTable[i].rReOrderQue));
				prSwRfbListTail = (P_SW_RFB_T)
				    QUEUE_GET_TAIL(&(prQM->arRxBaTable[i].rReOrderQue));
			} else {
				/* Concatenate the MSDU_INFO list with the existing list */
				QM_TX_SET_NEXT_MSDU_INFO(prSwRfbListTail,
							 QUEUE_GET_HEAD(&(prQM->arRxBaTable[i].rReOrderQue)));

				prSwRfbListTail = (P_SW_RFB_T)
				    QUEUE_GET_TAIL(&(prQM->arRxBaTable[i].rReOrderQue));
			}

			QUEUE_INITIALIZE(&(prQM->arRxBaTable[i].rReOrderQue));

		} else {
			continue;
		}
	}

	if (prSwRfbListTail) {
		/* Terminate the MSDU_INFO list with a NULL pointer */
		QM_TX_SET_NEXT_SW_RFB(prSwRfbListTail, NULL);
	}
	return prSwRfbListHead;

}

/*----------------------------------------------------------------------------*/
/*!
* \brief Flush RX packets with respect to a particular STA
*
* \param[in] u4StaRecIdx STA_REC index
* \param[in] u4Tid TID
*
* \return The flushed packets (in a list of SW_RFBs)
*/
/*----------------------------------------------------------------------------*/
P_SW_RFB_T qmFlushStaRxQueue(IN P_ADAPTER_T prAdapter, IN UINT_32 u4StaRecIdx, IN UINT_32 u4Tid)
{
	/* UINT_32 i; */
	P_SW_RFB_T prSwRfbListHead;
	P_SW_RFB_T prSwRfbListTail;
	P_RX_BA_ENTRY_T prReorderQueParm;
	P_STA_RECORD_T prStaRec;

	DBGLOG(QM, TRACE, "QM: Enter qmFlushStaRxQueues(%u)\n", u4StaRecIdx);

	prSwRfbListHead = prSwRfbListTail = NULL;

	prStaRec = &prAdapter->arStaRec[u4StaRecIdx];
	ASSERT(prStaRec);

	/* No matter whether this is an activated STA_REC, do flush */
#if 0
	if (!prStaRec->fgIsValid)
		return NULL;
#endif

	/* Obtain the RX BA Entry pointer */
	prReorderQueParm = ((prStaRec->aprRxReorderParamRefTbl)[u4Tid]);

	/* Note: For each queued packet, prCurrSwRfb->eDst equals RX_PKT_DESTINATION_HOST */
	if (prReorderQueParm) {

		if (QUEUE_IS_NOT_EMPTY(&(prReorderQueParm->rReOrderQue))) {

			prSwRfbListHead = (P_SW_RFB_T)
			    QUEUE_GET_HEAD(&(prReorderQueParm->rReOrderQue));
			prSwRfbListTail = (P_SW_RFB_T)
			    QUEUE_GET_TAIL(&(prReorderQueParm->rReOrderQue));

			QUEUE_INITIALIZE(&(prReorderQueParm->rReOrderQue));

		}
	}

	if (prSwRfbListTail) {
		/* Terminate the MSDU_INFO list with a NULL pointer */
		QM_TX_SET_NEXT_SW_RFB(prSwRfbListTail, NULL);
	}
	return prSwRfbListHead;

}

/*----------------------------------------------------------------------------*/
/*!
* \brief Enqueue TX packets
*
* \param[in] prMsduInfoListHead Pointer to the list of TX packets
*
* \return The freed packets, which are not enqueued
*/
/*----------------------------------------------------------------------------*/
P_MSDU_INFO_T qmEnqueueTxPackets(IN P_ADAPTER_T prAdapter, IN P_MSDU_INFO_T prMsduInfoListHead)
{
	P_MSDU_INFO_T prMsduInfoReleaseList;
	P_MSDU_INFO_T prCurrentMsduInfo;
	P_MSDU_INFO_T prNextMsduInfo;

	P_STA_RECORD_T prStaRec;
	QUE_T rNotEnqueuedQue;
	P_QUE_T prTxQue = &rNotEnqueuedQue;

	UINT_8 ucPacketType;
	UINT_8 ucTC;
	P_QUE_MGT_T prQM = &prAdapter->rQM;
	UINT_8 aucNextUP[WMM_AC_INDEX_NUM] = { 1 /* BEtoBK */, 1 /*na */, 0 /*VItoBE */, 4 /*VOtoVI */};
#if QM_TC_RESOURCE_EMPTY_COUNTER
	P_TX_CTRL_T prTxCtrl = NULL;
#endif
	DBGLOG(QM, LOUD, "Enter qmEnqueueTxPackets\n");

	ASSERT(prMsduInfoListHead);

#if QM_ADAPTIVE_TC_RESOURCE_CTRL
	/* UINT_32         i; */
	/* 4 <0> Update TC resource control related variables */
	/* Keep track of the queue length */
	if (--prQM->u4TimeToUpdateQueLen == 0) {	/* -- only here */
		prQM->u4TimeToUpdateQueLen = QM_INIT_TIME_TO_UPDATE_QUE_LEN;
		qmUpdateAverageTxQueLen(prAdapter);
	}
#endif

	/* Push TX packets into STA_REC (for UNICAST) or prAdapter->rQM (for BMCAST) */
	prStaRec = NULL;
	prMsduInfoReleaseList = NULL;
	prCurrentMsduInfo = NULL;
	QUEUE_INITIALIZE(&rNotEnqueuedQue);
	prNextMsduInfo = prMsduInfoListHead;

	do {
		P_BSS_INFO_T prBssInfo;
		BOOLEAN fgCheckACMAgain;
		ENUM_WMM_ACI_T eAci = WMM_AC_BE_INDEX;

		prCurrentMsduInfo = prNextMsduInfo;
		prNextMsduInfo = QM_TX_GET_NEXT_MSDU_INFO(prCurrentMsduInfo);
		ucTC = TC1_INDEX;

		/* 4 <1> Lookup the STA_REC index */
		/* The ucStaRecIndex will be set in this function */
		qmDetermineStaRecIndex(prAdapter, prCurrentMsduInfo);
		ucPacketType = HIF_TX_PACKET_TYPE_DATA;

		STATS_ENV_REPORT_DETECT(prAdapter, prCurrentMsduInfo->ucStaRecIndex);

		DBGLOG(QM, LOUD, "***** ucStaRecIndex = %d *****\n", prCurrentMsduInfo->ucStaRecIndex);

		prBssInfo = &(prAdapter->rWifiVar.arBssInfo[prCurrentMsduInfo->ucNetworkType]);

#if (CONF_HIF_LOOPBACK_AUTO == 0)
		if (IS_NET_ACTIVE(prAdapter, prCurrentMsduInfo->ucNetworkType))
#else
		/* force to send the loopback test packet */
		SET_NET_ACTIVE(prAdapter, prCurrentMsduInfo->ucNetworkType);
		prCurrentMsduInfo->ucStaRecIndex = STA_REC_INDEX_BMCAST;
		ucPacketType = HIF_TX_PKT_TYPE_HIF_LOOPBACK;
#endif /* End of CONF_HIF_LOOPBACK_AUTO */
		{
			switch (prCurrentMsduInfo->ucStaRecIndex) {
			case STA_REC_INDEX_BMCAST:
				prTxQue = &prQM->arTxQueue[TX_QUEUE_INDEX_BMCAST];
				ucTC = TC5_INDEX;
#if 0
				if (prCurrentMsduInfo->ucNetworkType == NETWORK_TYPE_P2P_INDEX
				    && prCurrentMsduInfo->eSrc != TX_PACKET_MGMT) {
					if (LINK_IS_EMPTY
					    (&prAdapter->rWifiVar.
					     arBssInfo[NETWORK_TYPE_P2P_INDEX].rStaRecOfClientList)) {
						prTxQue = &rNotEnqueuedQue;
						TX_INC_CNT(&prAdapter->rTxCtrl, TX_AP_BORADCAST_DROP);
					}
				}
#endif

				QM_DBG_CNT_INC(prQM, QM_DBG_CNT_23);
				break;

			case STA_REC_INDEX_NOT_FOUND:
				ucTC = TC5_INDEX;

				if (prCurrentMsduInfo->eSrc == TX_PACKET_FORWARDING) {

					/* if the packet is the forward type. the packet should be freed */
					DBGLOG(QM, TRACE, "Forwarding packet but Sta is STA_REC_INDEX_NOT_FOUND\n");
					/* prTxQue = &rNotEnqueuedQue; */
				}
				prTxQue = &prQM->arTxQueue[TX_QUEUE_INDEX_NO_STA_REC];
				QM_DBG_CNT_INC(prQM, QM_DBG_CNT_24);

				break;

			default:
				prStaRec = QM_GET_STA_REC_PTR_FROM_INDEX(prAdapter, prCurrentMsduInfo->ucStaRecIndex);

				if (!prStaRec) {
					DBGLOG(QM, ERROR, "prStaRec is NULL\n");
					break;
				}
				ASSERT(prStaRec->fgIsValid);

				if (prCurrentMsduInfo->ucUserPriority < 8) {
					QM_DBG_CNT_INC(prQM, prCurrentMsduInfo->ucUserPriority + 15);
					/* QM_DBG_CNT_15 *//* QM_DBG_CNT_16 *//* QM_DBG_CNT_17 *//* QM_DBG_CNT_18 */
					/* QM_DBG_CNT_19 *//* QM_DBG_CNT_20 *//* QM_DBG_CNT_21 *//* QM_DBG_CNT_22 */
				}

				eAci = WMM_AC_BE_INDEX;
				do {
					fgCheckACMAgain = FALSE;
					if (!prStaRec->fgIsQoS) {
						prTxQue = &prStaRec->arTxQueue[TX_QUEUE_INDEX_AC1];
						ucTC = TC1_INDEX;
						break;
					}

					switch (prCurrentMsduInfo->ucUserPriority) {
					case 1:
					case 2:
						prTxQue = &prStaRec->arTxQueue[TX_QUEUE_INDEX_AC0];
						ucTC = TC0_INDEX;
						eAci = WMM_AC_BK_INDEX;
						break;
					case 0:
					case 3:
						prTxQue = &prStaRec->arTxQueue[TX_QUEUE_INDEX_AC1];
						ucTC = TC1_INDEX;
						eAci = WMM_AC_BE_INDEX;
						break;
					case 4:
					case 5:
						prTxQue = &prStaRec->arTxQueue[TX_QUEUE_INDEX_AC2];
						ucTC = TC2_INDEX;
						eAci = WMM_AC_VI_INDEX;
#if QM_ADAPTIVE_TC_RESOURCE_CTRL
						prQM->u4TxNumOfVi++;
#endif
						break;
					case 6:
					case 7:
						prTxQue = &prStaRec->arTxQueue[TX_QUEUE_INDEX_AC3];
						ucTC = TC3_INDEX;
						eAci = WMM_AC_VO_INDEX;
#if QM_ADAPTIVE_TC_RESOURCE_CTRL
						prQM->u4TxNumOfVo++;
#endif
						break;
					default:
						prTxQue = &prStaRec->arTxQueue[TX_QUEUE_INDEX_AC1];
						ucTC = TC1_INDEX;
						eAci = WMM_AC_BE_INDEX;
						ASSERT(0);
						break;
					}
					if (prBssInfo->arACQueParms[eAci].fgIsACMSet && eAci
						!= WMM_AC_BK_INDEX) {
						prCurrentMsduInfo->ucUserPriority = aucNextUP[eAci];
						fgCheckACMAgain = TRUE;
					}
				} while (fgCheckACMAgain);

				/* LOG_FUNC ("QoS %u UP %u TC %u", */
				/*		prStaRec->fgIsQoS,prCurrentMsduInfo->ucUserPriority, ucTC); */

#if QM_ADAPTIVE_TC_RESOURCE_CTRL
				/*
				 * In TDLS or AP mode, peer maybe enter "sleep mode".
				 *
				 * If QM_INIT_TIME_TO_UPDATE_QUE_LEN = 60 when peer is in sleep mode,
				 * we need to wait 60 * u4TimeToAdjustTcResource = 180 packets
				 * u4TimeToAdjustTcResource = 3,
				 * then we will adjust TC resouce for VI or VO.
				 *
				 * But in TDLS test case, the throughput is very low, only 0.8Mbps in 5.7,
				 * we will to wait about 12 seconds to collect 180 packets.
				 * but the test time is only 20 seconds.
				 */
				if ((prQM->u4TxNumOfVi == 10) || (prQM->u4TxNumOfVo == 10)) {
					/* force to do TC resouce update */
					prQM->u4TimeToUpdateQueLen = QM_INIT_TIME_TO_UPDATE_QUE_LEN_MIN;
					prQM->u4TimeToAdjustTcResource = 1;
				}
#endif
#if ARP_MONITER_ENABLE
				if (IS_STA_IN_AIS(prStaRec) && prCurrentMsduInfo->eSrc == TX_PACKET_OS)
					qmDetectArpNoResponse(prAdapter, prCurrentMsduInfo);
#endif

				break;	/*default */
			}	/* switch (prCurrentMsduInfo->ucStaRecIndex) */

			if (prCurrentMsduInfo->eSrc == TX_PACKET_FORWARDING) {
				if (prTxQue->u4NumElem > 32) {
					DBGLOG(QM, WARN,
					       "Drop the Packet for full Tx queue (forwarding) Bss %u\n",
						prCurrentMsduInfo->ucNetworkType);
					prTxQue = &rNotEnqueuedQue;
					TX_INC_CNT(&prAdapter->rTxCtrl, TX_FORWARD_OVERFLOW_DROP);
				}
			}

		} else {

			DBGLOG(QM, WARN, "Drop the Packet for inactive Bss %u\n", prCurrentMsduInfo->ucNetworkType);
			QM_DBG_CNT_INC(prQM, QM_DBG_CNT_31);
			prTxQue = &rNotEnqueuedQue;
			TX_INC_CNT(&prAdapter->rTxCtrl, TX_INACTIVE_BSS_DROP);
		}

		/* 4 <3> Fill the MSDU_INFO for constructing HIF TX header */

		/* TODO: Fill MSDU_INFO according to the network type,
		 *  EtherType, and STA status (for PS forwarding control).
		 */

		/* Note that the Network Type Index and STA_REC index are determined in
		 *  qmDetermineStaRecIndex(prCurrentMsduInfo).
		 */
		QM_TX_SET_MSDU_INFO_FOR_DATA_PACKET(prCurrentMsduInfo,	/* MSDU_INFO ptr */
						    ucTC,	/* TC tag */
						    ucPacketType,	/* Packet Type */
						    0,	/* Format ID */
						    prCurrentMsduInfo->fgIs802_1x,	/* Flag 802.1x */
						    prCurrentMsduInfo->fgIs802_11,	/* Flag 802.11 */
						    0,	/* PAL LLH */
						    0,	/* ACL SN */
						    PS_FORWARDING_TYPE_NON_PS,	/* PS Forwarding Type */
						    0	/* PS Session ID */
		    );

		/* 4 <4> Enqueue the packet to different AC queue (max 5 AC queues) */
		QUEUE_INSERT_TAIL(prTxQue, (P_QUE_ENTRY_T) prCurrentMsduInfo);

		if (prTxQue != &rNotEnqueuedQue) {
			prQM->u4EnqeueuCounter++;
			prQM->au4ResourceWantedCounter[ucTC]++;
		}
		if (prStaRec)
			prStaRec->u4EnqeueuCounter++;

#if QM_TC_RESOURCE_EMPTY_COUNTER
		prTxCtrl = &prAdapter->rTxCtrl;

		if (prTxCtrl->rTc.aucFreeBufferCount[ucTC] == 0) {
			prQM->au4QmTcResourceEmptyCounter[prCurrentMsduInfo->ucNetworkType][ucTC]++;
			/*
			 * DBGLOG(QM, TRACE, ("TC%d Q Empty Count: [%d]%ld\n",
			 * ucTC,
			 * prCurrentMsduInfo->ucNetworkType,
			 * prQM->au4QmTcResourceEmptyCounter[prCurrentMsduInfo->ucNetworkType][ucTC]));
			 */
		}
#endif

#if QM_TEST_MODE
		if (++prQM->u4PktCount == QM_TEST_TRIGGER_TX_COUNT) {
			prQM->u4PktCount = 0;
			qmTestCases(prAdapter);
		}
#endif

		DBGLOG(QM, LOUD, "Current queue length = %u\n", prTxQue->u4NumElem);
	} while (prNextMsduInfo);

	if (QUEUE_IS_NOT_EMPTY(&rNotEnqueuedQue)) {
		QM_TX_SET_NEXT_MSDU_INFO((P_MSDU_INFO_T) QUEUE_GET_TAIL(&rNotEnqueuedQue), NULL);
		prMsduInfoReleaseList = (P_MSDU_INFO_T) QUEUE_GET_HEAD(&rNotEnqueuedQue);
	}

	return prMsduInfoReleaseList;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief Determine the STA_REC index for a packet
*
* \param[in] prMsduInfo Pointer to the packet
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/
static VOID qmDetermineStaRecIndex(IN P_ADAPTER_T prAdapter, IN P_MSDU_INFO_T prMsduInfo)
{
	UINT_32 i;

	P_STA_RECORD_T prTempStaRec;
	/* P_QUE_MGT_T prQM = &prAdapter->rQM; */

	prTempStaRec = NULL;

	ASSERT(prMsduInfo);

	/* 4 <1> DA = BMCAST */
	if (IS_BMCAST_MAC_ADDR(prMsduInfo->aucEthDestAddr)) {
		/*
		 * For intrastructure mode and P2P (playing as a GC), BMCAST frames shall be sent to the AP.
		 * FW shall take care of this. The host driver is not able to distinguish these cases.
		 */
		prMsduInfo->ucStaRecIndex = STA_REC_INDEX_BMCAST;
		DBGLOG(QM, LOUD, "TX with DA = BMCAST\n");
		return;
	}
#if (CFG_SUPPORT_TDLS == 1)
	/* Check if the peer is TDLS one */
	if (TdlsexStaRecIdxGet(prAdapter, prMsduInfo) == TDLS_STATUS_SUCCESS)
		return;		/* find a TDLS record */
#endif /* CFG_SUPPORT_TDLS */

	/* 4 <2> Check if an AP STA is present */
	for (i = 0; i < CFG_NUM_OF_STA_RECORD; i++) {
		prTempStaRec = &(prAdapter->arStaRec[i]);

		if ((prTempStaRec->ucNetTypeIndex == prMsduInfo->ucNetworkType)
		    && (prTempStaRec->fgIsAp)
		    && (prTempStaRec->fgIsValid)) {
			prMsduInfo->ucStaRecIndex = prTempStaRec->ucIndex;
			return;
		}
	}

	/* 4 <3> Not BMCAST, No AP --> Compare DA (i.e., to see whether this is a unicast frame to a client) */
	for (i = 0; i < CFG_NUM_OF_STA_RECORD; i++) {
		prTempStaRec = &(prAdapter->arStaRec[i]);
		if (prTempStaRec->fgIsValid) {
			if (EQUAL_MAC_ADDR(prTempStaRec->aucMacAddr, prMsduInfo->aucEthDestAddr)) {
				prMsduInfo->ucStaRecIndex = prTempStaRec->ucIndex;
				return;
			}
		}
	}

	/* 4 <4> No STA found, Not BMCAST --> Indicate NOT_FOUND to FW */
	prMsduInfo->ucStaRecIndex = STA_REC_INDEX_NOT_FOUND;
	DBGLOG(QM, LOUD, "QM: TX with STA_REC_INDEX_NOT_FOUND\n");

#if (QM_TEST_MODE && QM_TEST_FAIR_FORWARDING)
	prMsduInfo->ucStaRecIndex = (UINT_8) prQM->u4CurrentStaRecIndexToEnqueue;
#endif
}

/*----------------------------------------------------------------------------*/
/*!
* \brief Dequeue TX packets from a STA_REC for a particular TC
*
* \param[out] prQue The queue to put the dequeued packets
* \param[in] ucTC The TC index (TC0_INDEX to TC5_INDEX)
* \param[in] ucMaxNum The maximum amount of dequeued packets
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/
static VOID
qmDequeueTxPacketsFromPerStaQueues(IN P_ADAPTER_T prAdapter,
				   OUT P_QUE_T prQue, IN UINT_8 ucTC, IN UINT_8 ucCurrentQuota, IN UINT_8 ucTotalQuota)
{

#if QM_FORWARDING_FAIRNESS
	UINT_32 i;		/* Loop for */

	PUINT_32 pu4HeadStaRecIndex;	/* The Head STA index */
	PUINT_32 pu4HeadStaRecForwardCount;	/* The total forwarded packets for the head STA */

	P_STA_RECORD_T prStaRec;	/* The current focused STA */
	P_BSS_INFO_T prBssInfo;	/* The Bss for current focused STA */
	P_QUE_T prCurrQueue;	/* The current TX queue to dequeue */
	P_MSDU_INFO_T prDequeuedPkt;	/* The dequeued packet */

	UINT_32 u4ForwardCount;	/* To remember the total forwarded packets for a STA */
	UINT_32 u4MaxForwardCount;	/* The maximum number of packets a STA can forward */
	UINT_32 u4Resource;	/* The TX resource amount */

	BOOLEAN fgChangeHeadSta;	/* Whether a new head STA shall be determined at the end of the function */
	P_QUE_MGT_T prQM = &prAdapter->rQM;

	PUINT_8 pucFreeQuota = NULL;
#if CFG_ENABLE_WIFI_DIRECT
	P_P2P_CHNL_REQ_INFO_T prChnlReqInfo = &prAdapter->rWifiVar.prP2pFsmInfo->rChnlReqInfo;
	/*NFC Beam + Indication */
#endif
	DBGLOG(QM, LOUD, "Enter qmDequeueTxPacketsFromPerStaQueues (TC = %u)\n", ucTC);

	ASSERT(ucTC == TC0_INDEX || ucTC == TC1_INDEX || ucTC == TC2_INDEX || ucTC == TC3_INDEX || ucTC == TC4_INDEX);

	if (!ucCurrentQuota) {
		prQM->au4DequeueNoTcResourceCounter[ucTC]++;
		DBGLOG(TX, LOUD, "@@@@@ TC = %u ucCurrentQuota = %u @@@@@\n", ucTC, ucCurrentQuota);
		return;
	}

	u4Resource = ucCurrentQuota;

	/* 4 <1> Determine the head STA */
	/* The head STA shall be an active STA */

	pu4HeadStaRecIndex = &(prQM->au4HeadStaRecIndex[ucTC]);
	pu4HeadStaRecForwardCount = &(prQM->au4ForwardCount[ucTC]);

	DBGLOG(QM, LOUD, "(Fairness) TID = %u Init Head STA = %u Resource = %u\n",
			  ucTC, *pu4HeadStaRecIndex, u4Resource);

	/* From STA[x] to STA[x+1] to STA[x+2] to ... to STA[x] */
	for (i = 0; i < CFG_NUM_OF_STA_RECORD + 1; i++) {
		prStaRec = &prAdapter->arStaRec[(*pu4HeadStaRecIndex)];
		ASSERT(prStaRec);

		/* Only Data frame (1x was not included) will be queued in */
		if (prStaRec->fgIsValid) {

			prBssInfo = &(prAdapter->rWifiVar.arBssInfo[prStaRec->ucNetTypeIndex]);

			ASSERT(prBssInfo->ucNetTypeIndex == prStaRec->ucNetTypeIndex);

			/* Determine how many packets the head STA is allowed to send in a round */

			QM_DBG_CNT_INC(prQM, QM_DBG_CNT_25);
			u4MaxForwardCount = ucTotalQuota;
#if CFG_ENABLE_WIFI_DIRECT

			pucFreeQuota = NULL;
			if (prStaRec->fgIsInPS && (ucTC != TC4_INDEX)) {
				/* TODO: Change the threshold in coorperation with the PS forwarding mechanism */
				/* u4MaxForwardCount = ucTotalQuota; */
				/* Per STA flow control when STA in PS mode */
				/* The PHASE 1: only update from ucFreeQuota (now) */
				/* XXX The PHASE 2: Decide by ucFreeQuota and ucBmpDeliveryAC (per queue ) */
				/* aucFreeQuotaPerQueue[] */
				/* NOTE: other method to set u4Resource */

				if (prStaRec->fgIsQoS && prStaRec->fgIsUapsdSupported
				    /*
				     * && prAdapter->rWifiVar.fgSupportQoS
				     * && prAdapter->rWifiVar.fgSupportUAPSD
				     */) {

					if (prStaRec->ucBmpTriggerAC & BIT(ucTC)) {
						u4MaxForwardCount = prStaRec->ucFreeQuotaForDelivery;
						pucFreeQuota = &prStaRec->ucFreeQuotaForDelivery;
					} else {
						u4MaxForwardCount = prStaRec->ucFreeQuotaForNonDelivery;
						pucFreeQuota = &prStaRec->ucFreeQuotaForNonDelivery;
					}

				} else {
					ASSERT(prStaRec->ucFreeQuotaForDelivery == 0);
					u4MaxForwardCount = prStaRec->ucFreeQuotaForNonDelivery;
					pucFreeQuota = &prStaRec->ucFreeQuotaForNonDelivery;
				}

			}	/* fgIsInPS */
#endif /* CFG_ENABLE_WIFI_DIRECT */

#if CFG_ENABLE_WIFI_DIRECT

			/*NFC Beam + Indication */

			if (prBssInfo->fgIsNetAbsent && (ucTC != TC4_INDEX)) {
				if (prBssInfo->eCurrentOPMode == OP_MODE_ACCESS_POINT) {
					if ((prChnlReqInfo->NFC_BEAM != 1) &&
						(u4MaxForwardCount > prBssInfo->ucBssFreeQuota))
						u4MaxForwardCount = prBssInfo->ucBssFreeQuota;
				} else {
					if (u4MaxForwardCount > prBssInfo->ucBssFreeQuota)
						u4MaxForwardCount = prBssInfo->ucBssFreeQuota;
				}
			}
#endif /* CFG_ENABLE_WIFI_DIRECT */

			/* Determine whether the head STA can continue to forward packets in this round */
			if ((*pu4HeadStaRecForwardCount) < u4MaxForwardCount)
				break;

		} /* prStaRec->fgIsValid */
		else {
			/* The current Head STA has been deactivated, so search for a new head STA */
			prStaRec = NULL;
			prBssInfo = NULL;
			(*pu4HeadStaRecIndex)++;
			(*pu4HeadStaRecIndex) %= CFG_NUM_OF_STA_RECORD;

			/* Reset the forwarding count before searching (since this is for a new selected STA) */
			(*pu4HeadStaRecForwardCount) = 0;
		}
	}			/* i < CFG_NUM_OF_STA_RECORD + 1 */

	/* All STA_RECs are inactive, so exit */
	if (!prStaRec) {
		/* Under concurrent, it is possible that there is no candidcated STA. */
		/* DBGLOG(TX, EVENT, ("All STA_RECs are inactive\n")); */
		return;
	}

	DBGLOG(QM, LOUD, "(Fairness) TID = %u Round Head STA = %u\n", ucTC, *pu4HeadStaRecIndex);

	/* 4 <2> Dequeue packets from the head STA */

	prCurrQueue = &prStaRec->arTxQueue[ucTC];
	prDequeuedPkt = NULL;
	fgChangeHeadSta = FALSE;

#if (CFG_SUPPORT_TDLS == 1)
	if (pucFreeQuota != NULL)
		TdlsexTxQuotaCheck(prAdapter->prGlueInfo, prStaRec, *pucFreeQuota);
#endif /* CFG_SUPPORT_TDLS */

	while (prCurrQueue) {

#if QM_DEBUG_COUNTER

		if (ucTC <= TC4_INDEX) {
			if (QUEUE_IS_EMPTY(prCurrQueue)) {
				QM_DBG_CNT_INC(prQM, ucTC);
				/* QM_DBG_CNT_00 *//* QM_DBG_CNT_01 *//* QM_DBG_CNT_02 */
				/* QM_DBG_CNT_03 *//* QM_DBG_CNT_04 */
			}
			if (u4Resource == 0) {
				QM_DBG_CNT_INC(prQM, ucTC + 5);
				/* QM_DBG_CNT_05 *//* QM_DBG_CNT_06 *//* QM_DBG_CNT_07 */
				/* QM_DBG_CNT_08 *//* QM_DBG_CNT_09 */
			}
			if (((*pu4HeadStaRecForwardCount) >= u4MaxForwardCount)) {
				QM_DBG_CNT_INC(prQM, ucTC + 10);
				/* QM_DBG_CNT_10 *//* QM_DBG_CNT_11 *//* QM_DBG_CNT_12 */
				/* QM_DBG_CNT_13 *//* QM_DBG_CNT_14 */
			}
		}
#endif

		/* Three cases to break: (1) No resource (2) No packets (3) Fairness */
		if (QUEUE_IS_EMPTY(prCurrQueue) || ((*pu4HeadStaRecForwardCount) >= u4MaxForwardCount)) {
			fgChangeHeadSta = TRUE;
			break;
		} else if (u4Resource == 0) {
#if (CFG_SUPPORT_STATISTICS == 1)
			prStaRec->u4NumOfNoTxQuota++;
#endif /* CFG_SUPPORT_STATISTICS */
			break;
		}

		QUEUE_REMOVE_HEAD(prCurrQueue, prDequeuedPkt, P_MSDU_INFO_T);
		prStaRec->u4DeqeueuCounter++;
		prQM->u4DequeueCounter++;

#if (CFG_SUPPORT_TDLS_DBG == 1)
		if (prDequeuedPkt != NULL) {
			struct sk_buff *prSkb = (struct sk_buff *)prDequeuedPkt->prPacket;
			UINT8 *pkt = prSkb->data;
			UINT16 u2Identifier;

			if ((*(pkt + 12) == 0x08) && (*(pkt + 13) == 0x00)) {
				/* ip */
				u2Identifier = ((*(pkt + 18)) << 8) | (*(pkt + 19));
				DBGLOG(QM, LOUD, "<d> %d\n", u2Identifier);
			}
		}
#endif
#if DBG && 0
		LOG_FUNC("Deq0 TC %d queued %u net %u mac len %u len %u Type %u 1x %u 11 %u\n",
			 prDequeuedPkt->ucTC,
			 prCurrQueue->u4NumElem,
			 prDequeuedPkt->ucNetworkType,
			 prDequeuedPkt->ucMacHeaderLength,
			 prDequeuedPkt->u2FrameLength,
			 prDequeuedPkt->ucPacketType, prDequeuedPkt->fgIs802_1x, prDequeuedPkt->fgIs802_11);

		LOG_FUNC("Dest Mac: %pM\n", prDequeuedPkt->aucEthDestAddr);

#if LINUX
		{
			struct sk_buff *prSkb = (struct sk_buff *)prDequeuedPkt->prPacket;

			dumpMemory8((PUINT_8) prSkb->data, prSkb->len);
		}
#endif

#endif

		ASSERT(prDequeuedPkt->ucTC == ucTC);

		if (!QUEUE_IS_EMPTY(prCurrQueue)) {
			/* XXX: check all queues for STA */
			prDequeuedPkt->ucPsForwardingType = PS_FORWARDING_MORE_DATA_ENABLED;
		}

		QUEUE_INSERT_TAIL(prQue, (P_QUE_ENTRY_T) prDequeuedPkt);
		u4Resource--;
		(*pu4HeadStaRecForwardCount)++;

#if CFG_ENABLE_WIFI_DIRECT
		/* XXX The PHASE 2: decrease from  aucFreeQuotaPerQueue[] */
		if (prStaRec->fgIsInPS && (ucTC != TC4_INDEX)) {
			if ((pucFreeQuota) && (*pucFreeQuota > 0))
				*pucFreeQuota = *pucFreeQuota - 1;
		}
#endif /* CFG_ENABLE_WIFI_DIRECT */

#if CFG_ENABLE_WIFI_DIRECT
		if (prBssInfo->fgIsNetAbsent && (ucTC != TC4_INDEX)) {
			if (prBssInfo->ucBssFreeQuota > 0)
				prBssInfo->ucBssFreeQuota--;
		}
#endif /* CFG_ENABLE_WIFI_DIRECT */

	}

	if (*pu4HeadStaRecForwardCount) {
		DBGLOG(QM, LOUD,
		       "TC = %u Round Head STA = %u, u4HeadStaRecForwardCount = %u\n", ucTC, *pu4HeadStaRecIndex,
			(*pu4HeadStaRecForwardCount));
	}
#if QM_BURST_END_INFO_ENABLED
	/* Let FW know which packet is the last one dequeued from the STA */
	if (prDequeuedPkt)
		prDequeuedPkt->fgIsBurstEnd = TRUE;
#endif

	/* 4 <3> Dequeue from the other STAs if there is residual TX resource */

	/* Check all of the STAs to continue forwarding packets (including the head STA) */
	for (i = 0; i < CFG_NUM_OF_STA_RECORD; i++) {
		/* Break in case no reasource is available */
		if (u4Resource == 0) {
			prQM->au4DequeueNoTcResourceCounter[ucTC]++;
			break;
		}

		/* The current head STA will be examined when i = CFG_NUM_OF_STA_RECORD-1 */
		prStaRec = &prAdapter->arStaRec[((*pu4HeadStaRecIndex) + i + 1) % CFG_NUM_OF_STA_RECORD];
		if (prStaRec == NULL)   /* for coverity issue */
			break;

		if (prStaRec->fgIsValid) {

			prBssInfo = &(prAdapter->rWifiVar.arBssInfo[prStaRec->ucNetTypeIndex]);
			ASSERT(prBssInfo->ucNetTypeIndex == prStaRec->ucNetTypeIndex);

			DBGLOG(QM, LOUD, "(Fairness) TID = %u Sharing STA = %u Resource = %u\n",
					  ucTC, prStaRec->ucIndex, u4Resource);

			prCurrQueue = &prStaRec->arTxQueue[ucTC];
			u4ForwardCount = 0;
			u4MaxForwardCount = ucTotalQuota;

#if CFG_ENABLE_WIFI_DIRECT
			pucFreeQuota = NULL;
			if (prStaRec->fgIsInPS && (ucTC != TC4_INDEX)) {
				/* TODO: Change the threshold in coorperation with the PS forwarding mechanism */
				/* u4MaxForwardCount = ucTotalQuota; */
				/* Per STA flow control when STA in PS mode */
				/* The PHASE 1: only update from ucFreeQuota (now) */
				/* XXX The PHASE 2: Decide by ucFreeQuota and ucBmpDeliveryAC (per queue ) */
				/* aucFreeQuotaPerQueue[] */
				/* NOTE: other method to set u4Resource */
				if (prStaRec->fgIsQoS && prStaRec->fgIsUapsdSupported
				    /*
				     * && prAdapter->rWifiVar.fgSupportQoS
				     * && prAdapter->rWifiVar.fgSupportUAPSD
				     */) {

					if (prStaRec->ucBmpTriggerAC & BIT(ucTC)) {
						u4MaxForwardCount = prStaRec->ucFreeQuotaForDelivery;
						pucFreeQuota = &prStaRec->ucFreeQuotaForDelivery;
					} else {
						u4MaxForwardCount = prStaRec->ucFreeQuotaForNonDelivery;
						pucFreeQuota = &prStaRec->ucFreeQuotaForNonDelivery;
					}

				} else {
					ASSERT(prStaRec->ucFreeQuotaForDelivery == 0);
					u4MaxForwardCount = prStaRec->ucFreeQuotaForNonDelivery;
					pucFreeQuota = &prStaRec->ucFreeQuotaForNonDelivery;
				}

			}
#endif /* CFG_ENABLE_WIFI_DIRECT */
#if CFG_ENABLE_WIFI_DIRECT
			if (prBssInfo->fgIsNetAbsent && (ucTC != TC4_INDEX)) {
				if (u4MaxForwardCount > prBssInfo->ucBssFreeQuota)
					u4MaxForwardCount = prBssInfo->ucBssFreeQuota;
			}
#endif /* CFG_ENABLE_WIFI_DIRECT */
		} /* prStaRec->fgIsValid */
		else {
			prBssInfo = NULL;
			/* Invalid STA, so check the next STA */
			continue;
		}

		while (prCurrQueue) {
			/* Three cases to break: (1) No resource (2) No packets (3) Fairness */
			if ((u4Resource == 0) || QUEUE_IS_EMPTY(prCurrQueue) || (u4ForwardCount >= u4MaxForwardCount))
				break;


			QUEUE_REMOVE_HEAD(prCurrQueue, prDequeuedPkt, P_MSDU_INFO_T);

#if DBG && 0
			DBGLOG(QM, LOUD, "Deq0 TC %d queued %u net %u mac len %u len %u Type %u 1x %u 11 %u\n",
					  prDequeuedPkt->ucTC,
					  prCurrQueue->u4NumElem,
					  prDequeuedPkt->ucNetworkType,
					  prDequeuedPkt->ucMacHeaderLength,
					  prDequeuedPkt->u2FrameLength,
					  prDequeuedPkt->ucPacketType,
					  prDequeuedPkt->fgIs802_1x, prDequeuedPkt->fgIs802_11));

			DBGLOG(QM, LOUD, "Dest Mac: %pM\n", prDequeuedPkt->aucEthDestAddr);

#if LINUX
			{
				struct sk_buff *prSkb = (struct sk_buff *)prDequeuedPkt->prPacket;

				dumpMemory8((PUINT_8) prSkb->data, prSkb->len);
			}
#endif

#endif

			ASSERT(prDequeuedPkt->ucTC == ucTC);

			if (!QUEUE_IS_EMPTY(prCurrQueue))
				/* more data field ? */
				prDequeuedPkt->ucPsForwardingType = PS_FORWARDING_MORE_DATA_ENABLED;

			QUEUE_INSERT_TAIL(prQue, (P_QUE_ENTRY_T) prDequeuedPkt);
			if (prStaRec)
				prStaRec->u4DeqeueuCounter++;
			prQM->u4DequeueCounter++;
			u4Resource--;
			u4ForwardCount++;

#if CFG_ENABLE_WIFI_DIRECT
			/* XXX The PHASE 2: decrease from  aucFreeQuotaPerQueue[] */
			if (prStaRec->fgIsInPS && (ucTC != TC4_INDEX)) {
				ASSERT(pucFreeQuota);
				ASSERT(*pucFreeQuota > 0);
				if (*pucFreeQuota > 0)
					*pucFreeQuota = *pucFreeQuota - 1;
			}
#endif /* CFG_ENABLE_WIFI_DIRECT */

#if CFG_ENABLE_WIFI_DIRECT
			ASSERT(prBssInfo->ucNetTypeIndex == prStaRec->ucNetTypeIndex);
			if (prBssInfo->fgIsNetAbsent && (ucTC != TC4_INDEX)) {
				if (prBssInfo->ucBssFreeQuota > 0)
					prBssInfo->ucBssFreeQuota--;
			}
#endif /* CFG_ENABLE_WIFI_DIRECT */

		}

#if QM_BURST_END_INFO_ENABLED
		/* Let FW know which packet is the last one dequeued from the STA */
		if (u4ForwardCount)
			prDequeuedPkt->fgIsBurstEnd = TRUE;
#endif
	}

	if (fgChangeHeadSta) {
		(*pu4HeadStaRecIndex)++;
		(*pu4HeadStaRecIndex) %= CFG_NUM_OF_STA_RECORD;
		(*pu4HeadStaRecForwardCount) = 0;
		DBGLOG(QM, LOUD, "(Fairness) TID = %u Scheduled Head STA = %u Left Resource = %u\n",
				  ucTC, (*pu4HeadStaRecIndex), u4Resource);
	}

/***************************************************************************************/
#else
	UINT_8 ucStaRecIndex;
	P_STA_RECORD_T prStaRec;
	P_QUE_T prCurrQueue;
	UINT_8 ucPktCount;
	P_MSDU_INFO_T prDequeuedPkt;

	DBGLOG(QM, LOUD, "Enter qmDequeueTxPacketsFromPerStaQueues (TC = %u)\n", ucTC);

	if (ucCurrentQuota == 0)
		return;
	/* 4 <1> Determine the queue index and the head STA */

	/* The head STA */
	ucStaRecIndex = 0;	/* TODO: Get the current head STA */
	prStaRec = QM_GET_STA_REC_PTR_FROM_INDEX(prAdapter, ucStaRecIndex);
	ASSERT(prStaRec);

	if (prStaRec == NULL)
		return;

	/* The queue to pull out packets */
	ASSERT(ucTC == TC0_INDEX || ucTC == TC1_INDEX || ucTC == TC2_INDEX || ucTC == TC3_INDEX || ucTC == TC4_INDEX);
	prCurrQueue = &prStaRec->arTxQueue[ucTC];

	ucPktCount = ucCurrentQuota;
	prDequeuedPkt = NULL;

	/* 4 <2> Dequeue packets for the head STA */
	while (TRUE) {
		if (!(prStaRec->fgIsValid) || ucPktCount == 0 || QUEUE_IS_EMPTY(prCurrQueue)) {
			break;

		} else {

			QUEUE_REMOVE_HEAD(prCurrQueue, prDequeuedPkt, P_MSDU_INFO_T);
			/* DbgPrint("QM: Remove Queue Head, TC= %d\n", prDequeuedPkt->ucTC); */
			ASSERT(prDequeuedPkt->ucTC == ucTC);

			QUEUE_INSERT_TAIL(prQue, (P_QUE_ENTRY_T) prDequeuedPkt);
			ucPktCount--;
		}
	}

	/* DbgPrint("QM: Remaining number of queued packets = %d\n", prCurrQueue->u4NumElem); */

#if QM_BURST_END_INFO_ENABLED
	if (prDequeuedPkt)
		prDequeuedPkt->fgIsBurstEnd = TRUE;
#endif

	/* 4 <3> Update scheduling info */
	/* TODO */

	/* 4 <4> Utilize the remainaing TX opportunities for non-head STAs */
	/* TODO */
#endif
}

/*----------------------------------------------------------------------------*/
/*!
* \brief Dequeue TX packets from a per-Type-based Queue for a particular TC
*
* \param[out] prQue The queue to put the dequeued packets
* \param[in] ucTC The TC index (Shall always be TC5_INDEX)
* \param[in] ucMaxNum The maximum amount of dequeued packets
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/
static VOID
qmDequeueTxPacketsFromPerTypeQueues(IN P_ADAPTER_T prAdapter, OUT P_QUE_T prQue, IN UINT_8 ucTC, IN UINT_8 ucMaxNum)
{
	/* UINT_8          ucQueIndex; */
	/* UINT_8          ucStaRecIndex; */
	P_BSS_INFO_T prBssInfo;
	P_BSS_INFO_T parBssInfo;
	P_QUE_T prCurrQueue;
	UINT_8 ucPktCount;
	P_MSDU_INFO_T prDequeuedPkt;
	P_MSDU_INFO_T prBurstEndPkt;
	QUE_T rMergeQue;
	P_QUE_T prMergeQue;
	P_QUE_MGT_T prQM;

	DBGLOG(QM, LOUD, "Enter qmDequeueTxPacketsFromPerTypeQueues (TC = %d, Max = %d)\n", ucTC, ucMaxNum);

	/* TC5: Broadcast/Multicast data packets */
	ASSERT(ucTC == TC5_INDEX);

	if (ucMaxNum == 0)
		return;

	prQM = &prAdapter->rQM;
	/* 4 <1> Determine the queue */

	prCurrQueue = &prQM->arTxQueue[TX_QUEUE_INDEX_BMCAST];
	ucPktCount = ucMaxNum;
	prDequeuedPkt = NULL;
	prBurstEndPkt = NULL;

	parBssInfo = prAdapter->rWifiVar.arBssInfo;

	QUEUE_INITIALIZE(&rMergeQue);
	prMergeQue = &rMergeQue;

	/* 4 <2> Dequeue packets */
	while (TRUE) {
		if (ucPktCount == 0 || QUEUE_IS_EMPTY(prCurrQueue))
			break;

		QUEUE_REMOVE_HEAD(prCurrQueue, prDequeuedPkt, P_MSDU_INFO_T);
		ASSERT(prDequeuedPkt->ucTC == ucTC);

		ASSERT(prDequeuedPkt->ucNetworkType < NETWORK_TYPE_INDEX_NUM);

		prBssInfo = &parBssInfo[prDequeuedPkt->ucNetworkType];

		if (IS_BSS_ACTIVE(prBssInfo)) {
			if (!prBssInfo->fgIsNetAbsent) {
				QUEUE_INSERT_TAIL(prQue, (P_QUE_ENTRY_T) prDequeuedPkt);
				prQM->u4DequeueCounter++;
				prBurstEndPkt = prDequeuedPkt;
				ucPktCount--;
				QM_DBG_CNT_INC(prQM, QM_DBG_CNT_26);
#if DBG && 0
				LOG_FUNC
				    ("DeqType TC %d queued %u net %u mac len %u len %u Type %u 1x %u 11 %u\n",
				     prDequeuedPkt->ucTC, prCurrQueue->u4NumElem, prDequeuedPkt->ucNetworkType,
				     prDequeuedPkt->ucMacHeaderLength, prDequeuedPkt->u2FrameLength,
				     prDequeuedPkt->ucPacketType, prDequeuedPkt->fgIs802_1x,
				     prDequeuedPkt->fgIs802_11);

				LOG_FUNC("Dest Mac: %pM\n", prDequeuedPkt->aucEthDestAddr);

#if LINUX
				{
					struct sk_buff *prSkb = (struct sk_buff *)prDequeuedPkt->prPacket;

					dumpMemory8((PUINT_8) prSkb->data, prSkb->len);
				}
#endif

#endif
			} else {
				QUEUE_INSERT_TAIL(prMergeQue, (P_QUE_ENTRY_T) prDequeuedPkt);
			}
		} else {
			QM_TX_SET_NEXT_MSDU_INFO(prDequeuedPkt, NULL);
			wlanProcessQueuedMsduInfo(prAdapter, prDequeuedPkt);
		}
	}

	if (QUEUE_IS_NOT_EMPTY(prMergeQue)) {
		QUEUE_CONCATENATE_QUEUES(prMergeQue, prCurrQueue);
		QUEUE_MOVE_ALL(prCurrQueue, prMergeQue);
		if (QUEUE_GET_TAIL(prCurrQueue))
			QM_TX_SET_NEXT_MSDU_INFO((P_MSDU_INFO_T) QUEUE_GET_TAIL(prCurrQueue), NULL);
	}
#if QM_BURST_END_INFO_ENABLED
	if (prBurstEndPkt)
		prBurstEndPkt->fgIsBurstEnd = TRUE;
#endif
}				/* qmDequeueTxPacketsFromPerTypeQueues */

/*----------------------------------------------------------------------------*/
/*!
* \brief Dequeue TX packets to send to HIF TX
*
* \param[in] prTcqStatus Info about the maximum amount of dequeued packets
*
* \return The list of dequeued TX packets
*/
/*----------------------------------------------------------------------------*/
P_MSDU_INFO_T qmDequeueTxPackets(IN P_ADAPTER_T prAdapter, IN P_TX_TCQ_STATUS_T prTcqStatus)
{

	INT32 i;
	P_MSDU_INFO_T prReturnedPacketListHead;
	QUE_T rReturnedQue;

	DBGLOG(QM, LOUD, "Enter qmDequeueTxPackets\n");

	QUEUE_INITIALIZE(&rReturnedQue);

	prReturnedPacketListHead = NULL;

	/* dequeue packets from different AC queue based on available aucFreeBufferCount */
	/* TC0 to TC4: AC0~AC3, 802.1x (commands packets are not handled by QM) */
	for (i = TC4_INDEX; i >= TC0_INDEX; i--) {
		DBGLOG(QM, LOUD, "Dequeue packets from Per-STA queue[%d]\n", i);

		/*
		 * in the function, we will re-calculate the ucFreeQuota.
		 * If any packet with any priority for the station will be sent, ucFreeQuota --
		 *
		 * Note1: ucFreeQuota will be decrease only when station is in power save mode.
		 * In active mode, we will sent the packet to the air directly.
		 *
		 * if(prStaRec->fgIsInPS && (ucTC!=TC4_INDEX)) {
		 * ASSERT(pucFreeQuota);
		 * ASSERT(*pucFreeQuota>0);
		 * if ((pucFreeQuota) && (*pucFreeQuota>0)) {
		 * *pucFreeQuota = *pucFreeQuota - 1;
		 * }
		 * }
		 *
		 * Note2: maximum queued number for a station is 10, TXM_MAX_BUFFER_PER_STA_DEF in fw
		 * i.e. default prStaRec->ucFreeQuota = 10
		 *
		 * Note3: In qmUpdateFreeQuota(), we will adjust
		 * ucFreeQuotaForNonDelivery = ucFreeQuota>>1;
		 * ucFreeQuotaForDelivery =  ucFreeQuota - ucFreeQuotaForNonDelivery;
		 */
		qmDequeueTxPacketsFromPerStaQueues(prAdapter,
			&rReturnedQue,
			(UINT_8) i,
			prTcqStatus->aucFreeBufferCount[i],	/* maximum dequeue number */
			prTcqStatus->aucMaxNumOfBuffer[i]);

		/* The aggregate number of dequeued packets */
		DBGLOG(QM, LOUD, "DQA)[%u](%u)\n", i, rReturnedQue.u4NumElem);
	}

	/* TC5 (BMCAST or STA-NOT-FOUND packets) */
	qmDequeueTxPacketsFromPerTypeQueues(prAdapter,
					    &rReturnedQue, TC5_INDEX, prTcqStatus->aucFreeBufferCount[TC5_INDEX]
	    );

	DBGLOG(QM, LOUD, "Current total number of dequeued packets = %u\n", rReturnedQue.u4NumElem);

	if (QUEUE_IS_NOT_EMPTY(&rReturnedQue)) {
		prReturnedPacketListHead = (P_MSDU_INFO_T) QUEUE_GET_HEAD(&rReturnedQue);
		QM_TX_SET_NEXT_MSDU_INFO((P_MSDU_INFO_T) QUEUE_GET_TAIL(&rReturnedQue), NULL);
	}

	return prReturnedPacketListHead;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief Adjust the TC quotas according to traffic demands
*
* \param[out] prTcqAdjust The resulting adjustment
* \param[in] prTcqStatus Info about the current TC quotas and counters
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/
VOID qmAdjustTcQuotas(IN P_ADAPTER_T prAdapter, OUT P_TX_TCQ_ADJUST_T prTcqAdjust, IN P_TX_TCQ_STATUS_T prTcqStatus)
{
#if QM_ADAPTIVE_TC_RESOURCE_CTRL
	UINT_32 i;
	P_QUE_MGT_T prQM = &prAdapter->rQM;

	/* Must reset */
	for (i = 0; i < TC_NUM; i++)
		prTcqAdjust->acVariation[i] = 0;

	/* 4 <1> If TC resource is not just adjusted, exit directly */
	if (!prQM->fgTcResourcePostAnnealing)
		return;
	/* 4 <2> Adjust TcqStatus according to the updated prQM->au4CurrentTcResource */
	else {
		INT_32 i4TotalExtraQuota = 0;
		INT_32 ai4ExtraQuota[TC_NUM];
		BOOLEAN fgResourceRedistributed = TRUE;

		/* Obtain the free-to-distribute resource */
		for (i = 0; i < TC_NUM; i++) {
			ai4ExtraQuota[i] =
			    (INT_32) prTcqStatus->aucMaxNumOfBuffer[i] - (INT_32) prQM->au4CurrentTcResource[i];

			if (ai4ExtraQuota[i] > 0) {	/* The resource shall be reallocated to other TCs */

				if (ai4ExtraQuota[i] > prTcqStatus->aucFreeBufferCount[i]) {
					/*
					 * we have residunt TC resources for the TC:
					 * EX: aucMaxNumOfBuffer[] = 20, au4CurrentTcResource[] = 5
					 * ai4ExtraQuota[] = 15, aucFreeBufferCount[] = 10
					 *
					 * so ai4ExtraQuota[] = aucFreeBufferCount[] = 10
					 * because we available TC resources actually is 10, not 20
					 */
					ai4ExtraQuota[i] = prTcqStatus->aucFreeBufferCount[i];

					/*
					 * FALSE means we can re-do TC resource adjustment in tx done
					 * at next time, maybe more tx done is finished
					 */
					fgResourceRedistributed = FALSE;
				}

				/* accumulate current all available TC resources */
				i4TotalExtraQuota += ai4ExtraQuota[i];

				/* deduce unused TC resources for the TC */
				prTcqAdjust->acVariation[i] = (INT_8) (-ai4ExtraQuota[i]);
			}
		}

		/* Distribute quotas to TCs which need extra resource according to prQM->au4CurrentTcResource */
		for (i = 0; i < TC_NUM; i++) {
			if (ai4ExtraQuota[i] < 0) {

				/* The TC needs extra resources */
				if ((-ai4ExtraQuota[i]) > i4TotalExtraQuota) {
					/* the number of needed extra resources is larger than total available */
					ai4ExtraQuota[i] = (-i4TotalExtraQuota);

					/* wait for next tx done to do adjustment */
					fgResourceRedistributed = FALSE;
				}

				/* decrease the total available */
				i4TotalExtraQuota += ai4ExtraQuota[i];

				/* mark to increase TC resources for the TC */
				prTcqAdjust->acVariation[i] = (INT_8) (-ai4ExtraQuota[i]);
			}
		}

		/* In case some TC is waiting for TX Done, continue to adjust TC quotas upon TX Done */

		/*
		 * if fgResourceRedistributed == TRUE, it means we will adjust at this time so
		 * we need to re-adjust TC resources (fgTcResourcePostAnnealing = FALSE).
		 */
		prQM->fgTcResourcePostAnnealing = (!fgResourceRedistributed);

#if QM_PRINT_TC_RESOURCE_CTRL
		DBGLOG(QM, LOUD, "QM: Curr Quota [0]=%u [1]=%u [2]=%u [3]=%u [4]=%u [5]=%u\n",
				  prTcqStatus->aucFreeBufferCount[0],
				  prTcqStatus->aucFreeBufferCount[1],
				  prTcqStatus->aucFreeBufferCount[2],
				  prTcqStatus->aucFreeBufferCount[3],
				  prTcqStatus->aucFreeBufferCount[4], prTcqStatus->aucFreeBufferCount[5]
		       ));
#endif
	}

#else
	UINT_32 i;

	for (i = 0; i < TC_NUM; i++)
		prTcqAdjust->acVariation[i] = 0;

#endif
}

#if QM_ADAPTIVE_TC_RESOURCE_CTRL
/*----------------------------------------------------------------------------*/
/*!
* \brief Update the average TX queue length for the TC resource control mechanism
*
* \param (none)
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/
VOID qmUpdateAverageTxQueLen(IN P_ADAPTER_T prAdapter)
{
	INT_32 u4CurrQueLen, i, k;
	P_STA_RECORD_T prStaRec;
	P_QUE_MGT_T prQM = &prAdapter->rQM;

	/* 4 <1> Update the queue lengths for TC0 to TC3 (skip TC4) and TC5 */
	/* use moving average algorithm to calculate au4AverageQueLen for every TC queue */
	for (i = 0; i < NUM_OF_PER_STA_TX_QUEUES - 1; i++) {
		u4CurrQueLen = 0;

		for (k = 0; k < CFG_NUM_OF_STA_RECORD; k++) {
			prStaRec = &prAdapter->arStaRec[k];
			ASSERT(prStaRec);

			/* If the STA is activated, get the queue length */
			if (prStaRec->fgIsValid &&
			    (!prAdapter->rWifiVar.arBssInfo[prStaRec->ucNetTypeIndex].fgIsNetAbsent)
			    ) {

				u4CurrQueLen += (prStaRec->arTxQueue[i].u4NumElem);
			}
		}

		if (prQM->au4AverageQueLen[i] == 0) {
			prQM->au4AverageQueLen[i] = (u4CurrQueLen << QM_QUE_LEN_MOVING_AVE_FACTOR);	/* *8 */
		} else {
			/* len => len - len/8 = 7/8 * len + new len */
			prQM->au4AverageQueLen[i] -= (prQM->au4AverageQueLen[i] >> QM_QUE_LEN_MOVING_AVE_FACTOR);
			prQM->au4AverageQueLen[i] += (u4CurrQueLen);
		}

	}

	/* Update the queue length for TC5 (BMCAST) */
	u4CurrQueLen = prQM->arTxQueue[TX_QUEUE_INDEX_BMCAST].u4NumElem;

	if (prQM->au4AverageQueLen[TC_NUM - 1] == 0) {
		prQM->au4AverageQueLen[TC_NUM - 1] = (u4CurrQueLen << QM_QUE_LEN_MOVING_AVE_FACTOR);
	} else {
		prQM->au4AverageQueLen[TC_NUM - 1] -=
		    (prQM->au4AverageQueLen[TC_NUM - 1] >> QM_QUE_LEN_MOVING_AVE_FACTOR);
		prQM->au4AverageQueLen[TC_NUM - 1] += (u4CurrQueLen);
	}

	/* 4 <2> Adjust TC resource assignment every 3 times */
	/* Check whether it is time to adjust the TC resource assignment */
	if (--prQM->u4TimeToAdjustTcResource == 0) {	/* u4TimeToAdjustTcResource = 3 */

		/* The last assignment has not been completely applied */
		if (prQM->fgTcResourcePostAnnealing) {
			/* Upon the next qmUpdateAverageTxQueLen function call, do this check again */

			/* wait for next time to do qmReassignTcResource */
			prQM->u4TimeToAdjustTcResource = 1;
		} else {	/* The last assignment has been applied */
			prQM->u4TimeToAdjustTcResource = QM_INIT_TIME_TO_ADJUST_TC_RSC;
			qmReassignTcResource(prAdapter);
		}
	}

	/* Debug */
#if QM_PRINT_TC_RESOURCE_CTRL
	for (i = 0; i < TC_NUM; i++) {
		if (QM_GET_TX_QUEUE_LEN(prAdapter, i) >= 100) {
			DBGLOG(QM, LOUD, "QM: QueLen [%u %u %u %u %u %u]\n",
					  QM_GET_TX_QUEUE_LEN(prAdapter, 0),
					  QM_GET_TX_QUEUE_LEN(prAdapter, 1),
					  QM_GET_TX_QUEUE_LEN(prAdapter, 2),
					  QM_GET_TX_QUEUE_LEN(prAdapter, 3),
					  QM_GET_TX_QUEUE_LEN(prAdapter, 4), QM_GET_TX_QUEUE_LEN(prAdapter, 5)
			       ));
			break;
		}
	}
#endif

}

/*----------------------------------------------------------------------------*/
/*!
* \brief Assign TX resource for each TC according to TX queue length and current assignment
*
* \param (none)
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/
VOID qmReassignTcResource(IN P_ADAPTER_T prAdapter)
{
	INT_32 i4TotalResourceDemand = 0;
	UINT_32 u4ResidualResource = 0;
	UINT_32 i;
	INT_32 ai4PerTcResourceDemand[TC_NUM];
	UINT_32 u4ShareCount = 0;
	UINT_32 u4Share = 0;
	P_QUE_MGT_T prQM = &prAdapter->rQM;

	/* Note: After the new assignment is obtained, set prQM->fgTcResourcePostAnnealing to TRUE to
	 *  start the TC-quota adjusting procedure, which will be invoked upon every TX Done
	 */
	/*
	 * tx done -> nicProcessTxInterrupt() -> nicTxAdjustTcq()
	 * -> qmAdjustTcQuotas() -> check fgTcResourcePostAnnealing
	 */

	/* 4 <1> Determine the demands */
	/* Determine the amount of extra resource to fulfill all of the demands */
	for (i = 0; i < TC_NUM; i++) {
		/* Skip TC4, which is not adjustable */
		if (i == TC4_INDEX)
			continue;

		/*
		 * Define: extra_demand = average que_length (includes all station records) +
		 * min_reserved_quota -
		 * current available TC resources
		 *
		 * extra_demand means we need extra TC resources to transmit; other TCs can
		 * borrow their resources to us?
		 */
		ai4PerTcResourceDemand[i] =
		    ((UINT_32) (QM_GET_TX_QUEUE_LEN(prAdapter, i)) +
		     prQM->au4MinReservedTcResource[i] - prQM->au4CurrentTcResource[i]);

		/* If there are queued packets, allocate extra resource for the TC (for TCP consideration) */
		if (QM_GET_TX_QUEUE_LEN(prAdapter, i))
			ai4PerTcResourceDemand[i] += QM_EXTRA_RESERVED_RESOURCE_WHEN_BUSY;	/* 0 */

		/*
		 * accumulate all needed extra TC resources
		 * maybe someone need + resource, maybe someone need - resource
		 */
		i4TotalResourceDemand += ai4PerTcResourceDemand[i];
	}

	/* 4 <2> Case 1: Demand <= Total Resource */
	if (i4TotalResourceDemand <= 0) {
		/* 4 <2.1> Satisfy every TC */
		/* total TC resources are enough, no extra TC resources is needed */

		/* adjust used TC resources to average TC resources + min reserve TC resources */
		for (i = 0; i < TC_NUM; i++) {
			/* Skip TC4 (not adjustable) */
			if (i == TC4_INDEX)
				continue;

			/*
			 * the number of resources that one TC releases can be used for
			 * other TCs
			 *
			 * EX:  TC0 au4CurrentTcResource[0] = 10 ai4PerTcResourceDemand[0] = -5
			 * TC1 au4CurrentTcResource[1] = 5 ai4PerTcResourceDemand[0] = +5
			 * =>  TC0 au4CurrentTcResource[0] = 10 + (-5) = 5
			 * TC1 au4CurrentTcResource[1] = 5 + (+5) = 10
			 */
			prQM->au4CurrentTcResource[i] += ai4PerTcResourceDemand[i];
		}

		/* 4 <2.2> Share the residual resource evenly */
		u4ShareCount = (TC_NUM - 1);	/* 5, excluding TC4 */

		/*
		 * EX: i4TotalResourceDemand = -10
		 * means we have 10 available resources can be used.
		 */
		u4ResidualResource = (UINT_32) (-i4TotalResourceDemand);
		u4Share = (u4ResidualResource / u4ShareCount);

		/* share available TC resources to all TCs averagely */
		for (i = 0; i < TC_NUM; i++) {
			/* Skip TC4 (not adjustable) */
			if (i == TC4_INDEX)
				continue;

			/* allocate residual average resources to the TC */
			prQM->au4CurrentTcResource[i] += u4Share;

			/* Every TC is fully satisfied so no need extra resources */
			ai4PerTcResourceDemand[i] = 0;

			/* decrease the allocated resources */
			u4ResidualResource -= u4Share;
		}

		/* if still have available resources, we decide to give them to VO (TC3) queue */
		/* 4 <2.3> Allocate the left resource to TC3 (VO) */
		prQM->au4CurrentTcResource[TC3_INDEX] += (u4ResidualResource);

	}
	/* 4 <3> Case 2: Demand > Total Resource --> Guarantee a minimum amount of resource for each TC */
	else {
		/*
		 * u4ResidualResource means we at least need to keep
		 * QM_INITIAL_RESIDUAL_TC_RESOURCE available TC resources
		 *
		 * in 6628, u4ResidualResource = 26, max 28
		 */
		u4ResidualResource = QM_INITIAL_RESIDUAL_TC_RESOURCE;

		/* 4 <3.1> Allocated resource amount  = minimum of (guaranteed, total demand) */
		for (i = 0; i < TC_NUM; i++) {

			if (i == TC4_INDEX)
				continue;	/* Skip TC4 (not adjustable) */

			/* The demand can be fulfilled with the guaranteed resource amount 4 4 6 6 2 4 */

			/*
			 * ai4PerTcResourceDemand[i] =
			 * ((UINT_32)(QM_GET_TX_QUEUE_LEN(prAdapter, i)) +
			 * prQM->au4MinReservedTcResource[i] -
			 * prQM->au4CurrentTcResource[i]);
			 *
			 * so au4CurrentTcResource + ai4PerTcResourceDemand =
			 *
			 * ((UINT_32)(QM_GET_TX_QUEUE_LEN(prAdapter, i)) +
			 * prQM->au4MinReservedTcResource[i] =
			 *
			 * current average queue len + min TC resources
			 */
			if (prQM->au4CurrentTcResource[i] + ai4PerTcResourceDemand[i] <
			    prQM->au4GuaranteedTcResource[i]) {

				/* avg queue len + min reserve still smaller than guarantee so enough */
				prQM->au4CurrentTcResource[i] += ai4PerTcResourceDemand[i];

				/* accumulate available TC resources from the TC */
				u4ResidualResource +=
				    (prQM->au4GuaranteedTcResource[i] - prQM->au4CurrentTcResource[i]);
				ai4PerTcResourceDemand[i] = 0;
			}

			/* The demand can not be fulfilled with the guaranteed resource amount */
			else {

				/* means even we use all guarantee resources for the TC is still not enough */

				/*
				 * guarantee number is always for the TC so extra resource number cannot
				 * include the guarantee number.
				 *
				 * EX: au4GuaranteedTcResource = 10, au4CurrentTcResource = 5
				 * ai4PerTcResourceDemand = 6
				 *
				 * ai4PerTcResourceDemand -= (10 - 5) ==> 1
				 * only need extra 1 TC resouce is enough.
				 */
				ai4PerTcResourceDemand[i] -=
				    (prQM->au4GuaranteedTcResource[i] - prQM->au4CurrentTcResource[i]);

				/* update current avg TC resource to guarantee number */
				prQM->au4CurrentTcResource[i] = prQM->au4GuaranteedTcResource[i];

				/* count how many TC queues need to get extra resources */
				u4ShareCount++;
			}
		}

		/* 4 <3.2> Allocate the residual resource */
		do {
			/* If there is no resource left, exit directly */
			if (u4ResidualResource == 0)
				break;

			/* This shall not happen */
			if (u4ShareCount == 0) {
				prQM->au4CurrentTcResource[TC1_INDEX] += u4ResidualResource;
				DBGLOG(QM, ERROR, "QM: (Error) u4ShareCount = 0\n");
				break;
			}

			/* Share the residual resource evenly */
			u4Share = (u4ResidualResource / u4ShareCount);

			if (u4Share) {
				for (i = 0; i < TC_NUM; i++) {
					/* Skip TC4 (not adjustable) */
					if (i == TC4_INDEX)
						continue;

					if (ai4PerTcResourceDemand[i] == 0)
						continue;

					if (ai4PerTcResourceDemand[i] - u4Share) {
						/* still not enough but we just can give it u4Share resources */
						prQM->au4CurrentTcResource[i] += u4Share;
						u4ResidualResource -= u4Share;
						ai4PerTcResourceDemand[i] -= u4Share;
					} else {
						/* enough */
						prQM->au4CurrentTcResource[i] += ai4PerTcResourceDemand[i];
						u4ResidualResource -= ai4PerTcResourceDemand[i];
						ai4PerTcResourceDemand[i] = 0;
					}
				}
			}

			if (u4ResidualResource == 0)
				break;
			/* By priority, allocate the left resource that is not divisible by u4Share */

			if (ai4PerTcResourceDemand[TC3_INDEX]) {	/* VO */
				prQM->au4CurrentTcResource[TC3_INDEX]++;
				if (--u4ResidualResource == 0)
					break;
			}

			if (ai4PerTcResourceDemand[TC2_INDEX]) {	/* VI */
				prQM->au4CurrentTcResource[TC2_INDEX]++;
				if (--u4ResidualResource == 0)
					break;
			}

			if (ai4PerTcResourceDemand[TC5_INDEX]) {	/* BMCAST */
				prQM->au4CurrentTcResource[TC5_INDEX]++;
				if (--u4ResidualResource == 0)
					break;
			}

			if (ai4PerTcResourceDemand[TC1_INDEX]) {	/* BE */
				prQM->au4CurrentTcResource[TC1_INDEX]++;
				if (--u4ResidualResource == 0)
					break;
			}

			if (ai4PerTcResourceDemand[TC0_INDEX]) {	/* BK */
				prQM->au4CurrentTcResource[TC0_INDEX]++;
				if (--u4ResidualResource == 0)
					break;
			}

			/* Allocate the left resource */
			prQM->au4CurrentTcResource[TC3_INDEX] += u4ResidualResource;

		} while (FALSE);
	}

	/* mark the flag that we can start to do TC resource adjustment after TX done handle */
	prQM->fgTcResourcePostAnnealing = TRUE;

#if QM_PRINT_TC_RESOURCE_CTRL
	/* Debug print */
	DBGLOG(QM, LOUD, "QM: TC Rsc %u %u %u %u %u %u\n",
		prQM->au4CurrentTcResource[0],
		prQM->au4CurrentTcResource[1],
		prQM->au4CurrentTcResource[2],
		prQM->au4CurrentTcResource[3],
		prQM->au4CurrentTcResource[4],
		prQM->au4CurrentTcResource[5]);
#endif
}
#endif

/*----------------------------------------------------------------------------*/
/* RX-Related Queue Management                                                */
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*!
* \brief Init Queue Management for RX
*
* \param[in] (none)
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/
VOID qmInitRxQueues(IN P_ADAPTER_T prAdapter)
{
	/* DbgPrint("QM: Enter qmInitRxQueues()\n"); */
	/* TODO */
}

/*----------------------------------------------------------------------------*/
/*!
* \brief Handle RX packets (buffer reordering)
*
* \param[in] prSwRfbListHead The list of RX packets
*
* \return The list of packets which are not buffered for reordering
*/
/*----------------------------------------------------------------------------*/
P_SW_RFB_T qmHandleRxPackets(IN P_ADAPTER_T prAdapter, IN P_SW_RFB_T prSwRfbListHead)
{

#if CFG_RX_REORDERING_ENABLED
	/* UINT_32 i; */
	P_SW_RFB_T prCurrSwRfb;
	P_SW_RFB_T prNextSwRfb;
	P_HIF_RX_HEADER_T prHifRxHdr;
	QUE_T rReturnedQue;
	PUINT_8 pucEthDestAddr;
	BOOLEAN fgIsBMC;
#if CFG_RX_BA_REORDERING_ENHANCEMENT
	BOOLEAN fgIsIndependentPkt;
#endif


	/* DbgPrint("QM: Enter qmHandleRxPackets()\n"); */

	DEBUGFUNC("qmHandleRxPackets");

	ASSERT(prSwRfbListHead);

	QUEUE_INITIALIZE(&rReturnedQue);
	prNextSwRfb = prSwRfbListHead;

	do {
		prCurrSwRfb = prNextSwRfb;
		prNextSwRfb = QM_RX_GET_NEXT_SW_RFB(prCurrSwRfb);

#if CFG_RX_BA_REORDERING_ENHANCEMENT
		/* If it is indenpendant pkt, skip to reordering */
		if (prAdapter->rWifiVar.fgEnableReportIndependentPkt)
			fgIsIndependentPkt = qmIsIndependentPkt(prCurrSwRfb);
		else
			fgIsIndependentPkt = FALSE;
#endif
		prHifRxHdr = prCurrSwRfb->prHifRxHdr;	/* TODO: (Tehuang) Use macro to obtain the pointer */

		/* TODO: (Tehuang) Check if relaying */
		prCurrSwRfb->eDst = RX_PKT_DESTINATION_HOST;

		/* Decide the Destination */
#if CFG_RX_PKTS_DUMP
		if (prAdapter->rRxCtrl.u4RxPktsDumpTypeMask & BIT(HIF_RX_PKT_TYPE_DATA)) {
			DBGLOG(SW4, INFO, "QM RX DATA: net %u sta idx %u wlan idx %u ssn %u tid %u ptype %u 11 %u\n",
				(UINT_32) HIF_RX_HDR_GET_NETWORK_IDX(prHifRxHdr),
				prHifRxHdr->ucStaRecIdx, prCurrSwRfb->ucWlanIdx,
				(UINT_32) HIF_RX_HDR_GET_SN(prHifRxHdr),	/* The new SN of the frame */
				(UINT_32) HIF_RX_HDR_GET_TID(prHifRxHdr),
				prCurrSwRfb->ucPacketType,
				(UINT_32) HIF_RX_HDR_GET_80211_FLAG(prHifRxHdr));

			DBGLOG_MEM8(SW4, TRACE, (PUINT_8) prCurrSwRfb->pvHeader, prCurrSwRfb->u2PacketLen);
		}
#endif

		fgIsBMC = FALSE;
		if (!HIF_RX_HDR_GET_80211_FLAG(prHifRxHdr)) {

			UINT_8 ucNetTypeIdx;
			P_BSS_INFO_T prBssInfo;

			pucEthDestAddr = prCurrSwRfb->pvHeader;
			ucNetTypeIdx = HIF_RX_HDR_GET_NETWORK_IDX(prHifRxHdr);

			prBssInfo = &(prAdapter->rWifiVar.arBssInfo[ucNetTypeIdx]);
			/* DBGLOG_MEM8(QM, TRACE,prCurrSwRfb->pvHeader, 16); */
			/*  */

			if (IS_BMCAST_MAC_ADDR(pucEthDestAddr) && (prBssInfo->eCurrentOPMode != OP_MODE_ACCESS_POINT))
				fgIsBMC = TRUE;

			if (prAdapter->rRxCtrl.rFreeSwRfbList.u4NumElem >
				(CFG_RX_MAX_PKT_NUM - CFG_NUM_OF_QM_RX_PKT_NUM)) {

				if (!IS_BSS_ACTIVE(prBssInfo)) {
					DBGLOG(QM, WARN, "Mark NULL the Packet for inactive Bss %u\n", ucNetTypeIdx);
					prCurrSwRfb->eDst = RX_PKT_DESTINATION_NULL;
					QUEUE_INSERT_TAIL(&rReturnedQue, (P_QUE_ENTRY_T) prCurrSwRfb);
					continue;
				}

				if (prBssInfo->eCurrentOPMode == OP_MODE_ACCESS_POINT) {
					if (IS_BMCAST_MAC_ADDR(pucEthDestAddr))
						prCurrSwRfb->eDst = RX_PKT_DESTINATION_HOST_WITH_FORWARD;
					else if (UNEQUAL_MAC_ADDR(prBssInfo->aucOwnMacAddr, pucEthDestAddr) &&
							bssGetClientByAddress(prBssInfo, pucEthDestAddr))
						prCurrSwRfb->eDst = RX_PKT_DESTINATION_FORWARD;
						/* TODO : need to check the dst mac is valid */
						/* If src mac is invalid, the packet will be freed in fw */
				}	/* OP_MODE_ACCESS_POINT */
#if CFG_SUPPORT_HOTSPOT_2_0
				else if (hs20IsFrameFilterEnabled(prAdapter, prBssInfo) &&
					 hs20IsUnsecuredFrame(prAdapter, prBssInfo, prCurrSwRfb)) {
					DBGLOG(QM, WARN,
					       "Mark NULL the Packet for Dropped Packet %u\n", ucNetTypeIdx);
					prCurrSwRfb->eDst = RX_PKT_DESTINATION_NULL;
					QUEUE_INSERT_TAIL(&rReturnedQue, (P_QUE_ENTRY_T) prCurrSwRfb);
					continue;
				}
#endif
			} else {
				/* Dont not occupy other SW RFB */
				DBGLOG(QM, WARN, "Mark NULL the Packet for less Free Sw Rfb\n");
				prCurrSwRfb->eDst = RX_PKT_DESTINATION_NULL;
				QUEUE_INSERT_TAIL(&rReturnedQue, (P_QUE_ENTRY_T) prCurrSwRfb);
				continue;
			}

		}
#if CFG_SUPPORT_WAPI
		if (prCurrSwRfb->u2PacketLen > ETHER_HEADER_LEN) {
			PUINT_8 pc = (PUINT_8) prCurrSwRfb->pvHeader;
			UINT_16 u2Etype = 0;

			u2Etype = (pc[ETH_TYPE_LEN_OFFSET] << 8) | (pc[ETH_TYPE_LEN_OFFSET + 1]);

			/*
			 * for wapi integrity test. WPI_1x packet should be always in non-encrypted mode.
			 * if we received any WPI(0x88b4) packet that is encrypted, drop here.
			 */
			if (u2Etype == ETH_WPI_1X && HIF_RX_HDR_GET_SEC_MODE(prHifRxHdr) != 0) {
				DBGLOG(QM, INFO, "drop wpi packet with sec mode\n");
				prCurrSwRfb->eDst = RX_PKT_DESTINATION_NULL;
				QUEUE_INSERT_TAIL(&rReturnedQue, (P_QUE_ENTRY_T) prCurrSwRfb);
				continue;
			}
		}
#endif
		/* BAR frame */
		if (HIF_RX_HDR_GET_BAR_FLAG(prHifRxHdr)) {
			prCurrSwRfb->eDst = RX_PKT_DESTINATION_NULL;
			qmProcessBarFrame(prAdapter, prCurrSwRfb, &rReturnedQue);
		}
		/* Reordering is not required for this packet, return it without buffering */
		else if (!HIF_RX_HDR_GET_REORDER_FLAG(prHifRxHdr) || fgIsBMC) {
#if 0
			if (!HIF_RX_HDR_GET_80211_FLAG(prHifRxHdr)) {
				UINT_8 ucNetTypeIdx;
				P_BSS_INFO_T prBssInfo;

				pucEthDestAddr = prCurrSwRfb->pvHeader;
				ucNetTypeIdx = HIF_RX_HDR_GET_NETWORK_IDX(prHifRxHdr);

				prBssInfo = &(prAdapter->rWifiVar.arBssInfo[ucNetTypeIdx]);

				if (IS_BMCAST_MAC_ADDR(pucEthDestAddr) &&
					(prBssInfo->eCurrentOPMode == OP_MODE_ACCESS_POINT)) {
					prCurrSwRfb->eDst = RX_PKT_DESTINATION_HOST_WITH_FORWARD;
				}
			}
#endif

			QUEUE_INSERT_TAIL(&rReturnedQue, (P_QUE_ENTRY_T) prCurrSwRfb);
		}
		/* Reordering is required for this packet */
		else {
			/* If this packet should dropped or indicated to the host immediately,
			 *  it should be enqueued into the rReturnedQue with specific flags. If
			 *  this packet should be buffered for reordering, it should be enqueued
			 *  into the reordering queue in the STA_REC rather than into the
			 *  rReturnedQue.
			 */
#if CFG_RX_BA_REORDERING_ENHANCEMENT
			if (fgIsIndependentPkt) {
				DBGLOG(QM, TRACE, "Insert independentPkt to returnedQue directly\n");
				QUEUE_INSERT_TAIL(&rReturnedQue, (P_QUE_ENTRY_T) prCurrSwRfb);
				qmProcessIndepentReorderQueue(prAdapter, prCurrSwRfb);
			} else
				qmProcessPktWithReordering(prAdapter, prCurrSwRfb, &rReturnedQue);
#else
			qmProcessPktWithReordering(prAdapter, prCurrSwRfb, &rReturnedQue);
#endif
		}
	} while (prNextSwRfb);

	/* RX_PKT_DESTINATION_HOST_WITH_FORWARD or RX_PKT_DESTINATION_FORWARD */
	/* The returned list of SW_RFBs must end with a NULL pointer */
	if (QUEUE_IS_NOT_EMPTY(&rReturnedQue))
		QM_TX_SET_NEXT_MSDU_INFO((P_SW_RFB_T) QUEUE_GET_TAIL(&rReturnedQue), NULL);

	return (P_SW_RFB_T) QUEUE_GET_HEAD(&rReturnedQue);

#else

	/* DbgPrint("QM: Enter qmHandleRxPackets()\n"); */
	return prSwRfbListHead;

#endif

}

/*----------------------------------------------------------------------------*/
/*!
* \brief Reorder the received packet
*
* \param[in] prSwRfb The RX packet to process
* \param[out] prReturnedQue The queue for indicating packets
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/
VOID qmProcessPktWithReordering(IN P_ADAPTER_T prAdapter, IN P_SW_RFB_T prSwRfb, OUT P_QUE_T prReturnedQue)
{

	P_STA_RECORD_T prStaRec;
	P_HIF_RX_HEADER_T prHifRxHdr;
	P_RX_BA_ENTRY_T prReorderQueParm;

	UINT_32 u4SeqNo;
	UINT_32 u4WinStart;
	UINT_32 u4WinEnd;
	P_QUE_T prReorderQue;
	/* P_SW_RFB_T prReorderedSwRfb; */
	BOOLEAN fgIsBaTimeout;

	DEBUGFUNC("qmProcessPktWithReordering");

	if ((prSwRfb == NULL) || (prReturnedQue == NULL) || (prSwRfb->prHifRxHdr == NULL)) {
		ASSERT(FALSE);
		return;
	}

	prHifRxHdr = prSwRfb->prHifRxHdr;
	prSwRfb->ucStaRecIdx = prHifRxHdr->ucStaRecIdx;
	prSwRfb->u2SSN = HIF_RX_HDR_GET_SN(prHifRxHdr);	/* The new SN of the frame */
	prSwRfb->ucTid = (UINT_8) (HIF_RX_HDR_GET_TID(prHifRxHdr));
	/* prSwRfb->eDst = RX_PKT_DESTINATION_HOST; */

	/* Incorrect STA_REC index */
	if (prSwRfb->ucStaRecIdx >= CFG_NUM_OF_STA_RECORD) {
		prSwRfb->eDst = RX_PKT_DESTINATION_NULL;
		QUEUE_INSERT_TAIL(prReturnedQue, (P_QUE_ENTRY_T) prSwRfb);
		DBGLOG(QM, WARN, "Reordering for a NULL STA_REC, ucStaRecIdx = %d\n", prSwRfb->ucStaRecIdx);
		/* ASSERT(0); */
		return;
	}

	/* Check whether the STA_REC is activated */
	prStaRec = &(prAdapter->arStaRec[prSwRfb->ucStaRecIdx]);
	ASSERT(prStaRec);

#if 0
	if (!(prStaRec->fgIsValid)) {
		/* TODO: (Tehuang) Handle the Host-FW sync issue. */
		prSwRfb->eDst = RX_PKT_DESTINATION_NULL;
		QUEUE_INSERT_TAIL(prReturnedQue, (P_QUE_ENTRY_T) prSwRfb);
		DBGLOG(QM, WARN, "Reordering for an invalid STA_REC\n");
		/* ASSERT(0); */
		return;
	}
#endif

	/* Check whether the BA agreement exists */
	prReorderQueParm = ((prStaRec->aprRxReorderParamRefTbl)[prSwRfb->ucTid]);
	if (!prReorderQueParm) {
		/* TODO: (Tehuang) Handle the Host-FW sync issue. */
		prSwRfb->eDst = RX_PKT_DESTINATION_NULL;
		QUEUE_INSERT_TAIL(prReturnedQue, (P_QUE_ENTRY_T) prSwRfb);
		DBGLOG(QM, WARN, "Reordering for a NULL ReorderQueParm\n");
		/* ASSERT(0); */
		return;
	}

	/* Start to reorder packets */
	u4SeqNo = (UINT_32) (prSwRfb->u2SSN);
	prReorderQue = &(prReorderQueParm->rReOrderQue);
	u4WinStart = (UINT_32) (prReorderQueParm->u2WinStart);
	u4WinEnd = (UINT_32) (prReorderQueParm->u2WinEnd);

	/* Debug */
	/* DbgPrint("QM:(R)[%d](%ld){%ld,%ld}\n", prSwRfb->ucTid, u4SeqNo, u4WinStart, u4WinEnd); */

	/* Case 1: Fall within */
	if			/* 0 - start - sn - end - 4095 */
	    (((u4WinStart <= u4SeqNo) && (u4SeqNo <= u4WinEnd))
	     /* 0 - end - start - sn - 4095 */
	     || ((u4WinEnd < u4WinStart) && (u4WinStart <= u4SeqNo))
	     /* 0 - sn - end - start - 4095 */
	     || ((u4SeqNo <= u4WinEnd) && (u4WinEnd < u4WinStart))) {

		qmInsertFallWithinReorderPkt(prSwRfb, prReorderQueParm, prReturnedQue);

#if QM_RX_WIN_SSN_AUTO_ADVANCING
		if (prReorderQueParm->fgIsWaitingForPktWithSsn) {
			/* Let the first received packet pass the reorder check */
			DBGLOG(QM, LOUD, "QM:(A)[%d](%u){%u,%u}\n", prSwRfb->ucTid, u4SeqNo, u4WinStart, u4WinEnd);

			prReorderQueParm->u2WinStart = (UINT_16) u4SeqNo;
			prReorderQueParm->u2WinEnd =
			    ((prReorderQueParm->u2WinStart) + (prReorderQueParm->u2WinSize) - 1) % MAX_SEQ_NO_COUNT;
			prReorderQueParm->fgIsWaitingForPktWithSsn = FALSE;
		}
#endif

		if (qmPopOutDueToFallWithin(prAdapter, prReorderQueParm, prReturnedQue, &fgIsBaTimeout) == FALSE)
			STATS_RX_REORDER_HOLE_INC(prStaRec);	/* record hole count */
		STATS_RX_REORDER_HOLE_TIMEOUT_INC(prStaRec, fgIsBaTimeout);
	}
	/* Case 2: Fall ahead */
	else if
	    /* 0 - start - end - sn - (start+2048) - 4095 */
	    (((u4WinStart < u4WinEnd)
	      && (u4WinEnd < u4SeqNo)
	      && (u4SeqNo < (u4WinStart + HALF_SEQ_NO_COUNT)))
	     /* 0 - sn - (start+2048) - start - end - 4095 */
	     || ((u4SeqNo < u4WinStart)
		 && (u4WinStart < u4WinEnd)
		 && ((u4SeqNo + MAX_SEQ_NO_COUNT) < (u4WinStart + HALF_SEQ_NO_COUNT)))
	     /* 0 - end - sn - (start+2048) - start - 4095 */
	     || ((u4WinEnd < u4SeqNo)
		 && (u4SeqNo < u4WinStart)
		 && ((u4SeqNo + MAX_SEQ_NO_COUNT) < (u4WinStart + HALF_SEQ_NO_COUNT)))) {

#if QM_RX_WIN_SSN_AUTO_ADVANCING
		if (prReorderQueParm->fgIsWaitingForPktWithSsn)
			prReorderQueParm->fgIsWaitingForPktWithSsn = FALSE;
#endif

		qmInsertFallAheadReorderPkt(prSwRfb, prReorderQueParm, prReturnedQue);

		/* Advance the window after inserting a new tail */
		prReorderQueParm->u2WinEnd = (UINT_16) u4SeqNo;
		prReorderQueParm->u2WinStart =
		    (((prReorderQueParm->u2WinEnd) - (prReorderQueParm->u2WinSize) + MAX_SEQ_NO_COUNT + 1)
		     % MAX_SEQ_NO_COUNT);

		qmPopOutDueToFallAhead(prAdapter, prReorderQueParm, prReturnedQue);

		STATS_RX_REORDER_FALL_AHEAD_INC(prStaRec);

	}
	/* Case 3: Fall behind */
	else {

#if QM_RX_WIN_SSN_AUTO_ADVANCING
#if QM_RX_INIT_FALL_BEHIND_PASS
		if (prReorderQueParm->fgIsWaitingForPktWithSsn) {
			/* ?? prSwRfb->eDst = RX_PKT_DESTINATION_HOST; */
			QUEUE_INSERT_TAIL(prReturnedQue, (P_QUE_ENTRY_T) prSwRfb);
			/* DbgPrint("QM:(P)[%d](%ld){%ld,%ld}\n", prSwRfb->ucTid, u4SeqNo, u4WinStart, u4WinEnd); */
			return;
		}
#endif
#endif

		STATS_RX_REORDER_FALL_BEHIND_INC(prStaRec);
		/* An erroneous packet */
		prSwRfb->eDst = RX_PKT_DESTINATION_NULL;
		QUEUE_INSERT_TAIL(prReturnedQue, (P_QUE_ENTRY_T) prSwRfb);
		/* DbgPrint("QM:(D)[%d](%ld){%ld,%ld}\n", prSwRfb->ucTid, u4SeqNo, u4WinStart, u4WinEnd); */
		return;
	}

	return;

}
VOID qmProcessBarFrame(IN P_ADAPTER_T prAdapter, IN P_SW_RFB_T prSwRfb, OUT P_QUE_T prReturnedQue)
{

	P_STA_RECORD_T prStaRec;
	P_HIF_RX_HEADER_T prHifRxHdr;
	P_RX_BA_ENTRY_T prReorderQueParm;

	UINT_32 u4SSN;
	UINT_32 u4WinStart;
	UINT_32 u4WinEnd;
	P_QUE_T prReorderQue;
	/* P_SW_RFB_T prReorderedSwRfb; */

	if ((prSwRfb == NULL) || (prReturnedQue == NULL) || (prSwRfb->prHifRxHdr == NULL)) {
		ASSERT(FALSE);
		return;
	}

	prHifRxHdr = prSwRfb->prHifRxHdr;
	prSwRfb->ucStaRecIdx = prHifRxHdr->ucStaRecIdx;
	prSwRfb->u2SSN = HIF_RX_HDR_GET_SN(prHifRxHdr);	/* The new SSN */
	prSwRfb->ucTid = (UINT_8) (HIF_RX_HDR_GET_TID(prHifRxHdr));

	prSwRfb->eDst = RX_PKT_DESTINATION_NULL;
	QUEUE_INSERT_TAIL(prReturnedQue, (P_QUE_ENTRY_T) prSwRfb);

	/* Incorrect STA_REC index */
	if (prSwRfb->ucStaRecIdx >= CFG_NUM_OF_STA_RECORD) {
		DBGLOG(QM, WARN, "QM: (Warning) BAR for a NULL STA_REC, ucStaRecIdx = %d\n", prSwRfb->ucStaRecIdx);
		/* ASSERT(0); */
		return;
	}

	/* Check whether the STA_REC is activated */
	prStaRec = &(prAdapter->arStaRec[prSwRfb->ucStaRecIdx]);
	ASSERT(prStaRec);

#if 0
	if (!(prStaRec->fgIsValid)) {
		/* TODO: (Tehuang) Handle the Host-FW sync issue. */
		DbgPrint("QM: (Warning) BAR for an invalid STA_REC\n");
		/* ASSERT(0); */
		return;
	}
#endif

	/* Check whether the BA agreement exists */
	prReorderQueParm = ((prStaRec->aprRxReorderParamRefTbl)[prSwRfb->ucTid]);
	if (!prReorderQueParm) {
		/* TODO: (Tehuang) Handle the Host-FW sync issue. */
		DBGLOG(QM, WARN, "QM: (Warning) BAR for a NULL ReorderQueParm\n");
		/* ASSERT(0); */
		return;
	}

	u4SSN = (UINT_32) (prSwRfb->u2SSN);
	prReorderQue = &(prReorderQueParm->rReOrderQue);
	u4WinStart = (UINT_32) (prReorderQueParm->u2WinStart);
	u4WinEnd = (UINT_32) (prReorderQueParm->u2WinEnd);

	if (qmCompareSnIsLessThan(u4WinStart, u4SSN)) {
		prReorderQueParm->u2WinStart = (UINT_16) u4SSN;
		prReorderQueParm->u2WinEnd =
		    ((prReorderQueParm->u2WinStart) + (prReorderQueParm->u2WinSize) - 1) % MAX_SEQ_NO_COUNT;
		DBGLOG(QM, TRACE,
		       "QM:(BAR)[%d](%u){%d,%d}\n", prSwRfb->ucTid, u4SSN, prReorderQueParm->u2WinStart,
			prReorderQueParm->u2WinEnd);
		qmPopOutDueToFallAhead(prAdapter, prReorderQueParm, prReturnedQue);
	} else {
		DBGLOG(QM, TRACE, "QM:(BAR)(%d)(%u){%u,%u}\n", prSwRfb->ucTid, u4SSN, u4WinStart, u4WinEnd);
	}
}

VOID qmInsertFallWithinReorderPkt(IN P_SW_RFB_T prSwRfb, IN P_RX_BA_ENTRY_T prReorderQueParm, OUT P_QUE_T prReturnedQue)
{
	P_SW_RFB_T prExaminedQueuedSwRfb;
	P_QUE_T prReorderQue;

	ASSERT(prSwRfb);
	ASSERT(prReorderQueParm);
	ASSERT(prReturnedQue);

	prReorderQue = &(prReorderQueParm->rReOrderQue);
	prExaminedQueuedSwRfb = (P_SW_RFB_T) QUEUE_GET_HEAD(prReorderQue);

	/* There are no packets queued in the Reorder Queue */
	if (prExaminedQueuedSwRfb == NULL) {
		((P_QUE_ENTRY_T) prSwRfb)->prPrev = NULL;
		((P_QUE_ENTRY_T) prSwRfb)->prNext = NULL;
		prReorderQue->prHead = (P_QUE_ENTRY_T) prSwRfb;
		prReorderQue->prTail = (P_QUE_ENTRY_T) prSwRfb;
		prReorderQue->u4NumElem++;
	}

	/* Determine the insert position */
	else {
		do {
			/* Case 1: Terminate. A duplicate packet */
			if (((prExaminedQueuedSwRfb->u2SSN) == (prSwRfb->u2SSN))) {
				prSwRfb->eDst = RX_PKT_DESTINATION_NULL;
				QUEUE_INSERT_TAIL(prReturnedQue, (P_QUE_ENTRY_T) prSwRfb);
				return;
			}

			/* Case 2: Terminate. The insert point is found */
			else if (qmCompareSnIsLessThan((prSwRfb->u2SSN), (prExaminedQueuedSwRfb->u2SSN)))
				break;

			/* Case 3: Insert point not found. Check the next SW_RFB in the Reorder Queue */
			else
				prExaminedQueuedSwRfb = (P_SW_RFB_T) (((P_QUE_ENTRY_T) prExaminedQueuedSwRfb)->prNext);
		} while (prExaminedQueuedSwRfb);

		/* Update the Reorder Queue Parameters according to the found insert position */
		if (prExaminedQueuedSwRfb == NULL) {
			/* The received packet shall be placed at the tail */
			((P_QUE_ENTRY_T) prSwRfb)->prPrev = prReorderQue->prTail;
			((P_QUE_ENTRY_T) prSwRfb)->prNext = NULL;
			(prReorderQue->prTail)->prNext = (P_QUE_ENTRY_T) (prSwRfb);
			prReorderQue->prTail = (P_QUE_ENTRY_T) (prSwRfb);
		} else {
			((P_QUE_ENTRY_T) prSwRfb)->prPrev = ((P_QUE_ENTRY_T) prExaminedQueuedSwRfb)->prPrev;
			((P_QUE_ENTRY_T) prSwRfb)->prNext = (P_QUE_ENTRY_T) prExaminedQueuedSwRfb;
			if (((P_QUE_ENTRY_T) prExaminedQueuedSwRfb) == (prReorderQue->prHead)) {
				/* The received packet will become the head */
				prReorderQue->prHead = (P_QUE_ENTRY_T) prSwRfb;
			} else {
				(((P_QUE_ENTRY_T) prExaminedQueuedSwRfb)->prPrev)->prNext = (P_QUE_ENTRY_T) prSwRfb;
			}
			((P_QUE_ENTRY_T) prExaminedQueuedSwRfb)->prPrev = (P_QUE_ENTRY_T) prSwRfb;
		}

		prReorderQue->u4NumElem++;

	}
	DBGLOG(QM, TRACE, "qmInsertFallWithinReorderPkt SSN: %u\n", prSwRfb->u2SSN);
}

VOID qmInsertFallAheadReorderPkt(IN P_SW_RFB_T prSwRfb, IN P_RX_BA_ENTRY_T prReorderQueParm, OUT P_QUE_T prReturnedQue)
{
	P_QUE_T prReorderQue;

	ASSERT(prSwRfb);
	ASSERT(prReorderQueParm);
	ASSERT(prReturnedQue);

	prReorderQue = &(prReorderQueParm->rReOrderQue);

	/* There are no packets queued in the Reorder Queue */
	if (QUEUE_IS_EMPTY(prReorderQue)) {
		((P_QUE_ENTRY_T) prSwRfb)->prPrev = NULL;
		((P_QUE_ENTRY_T) prSwRfb)->prNext = NULL;
		prReorderQue->prHead = (P_QUE_ENTRY_T) prSwRfb;
	} else {
		((P_QUE_ENTRY_T) prSwRfb)->prPrev = prReorderQue->prTail;
		((P_QUE_ENTRY_T) prSwRfb)->prNext = NULL;
		(prReorderQue->prTail)->prNext = (P_QUE_ENTRY_T) (prSwRfb);
	}
	prReorderQue->prTail = (P_QUE_ENTRY_T) prSwRfb;
	prReorderQue->u4NumElem++;
	DBGLOG(QM, TRACE, "qmInsertFallAheadReorderPkt SSN: %u\n", prSwRfb->u2SSN);
}

BOOLEAN
qmPopOutDueToFallWithin(P_ADAPTER_T prAdapter, IN P_RX_BA_ENTRY_T prReorderQueParm,
		OUT P_QUE_T prReturnedQue, OUT BOOLEAN *fgIsTimeout)
{
	P_SW_RFB_T prReorderedSwRfb;
	P_QUE_T prReorderQue;
	BOOLEAN fgDequeuHead, fgMissing;
	OS_SYSTIME rCurrentTime, *prMissTimeout;

	prReorderQue = &(prReorderQueParm->rReOrderQue);

	*fgIsTimeout = FALSE;
	fgMissing = FALSE;
	rCurrentTime = 0;
	prMissTimeout = &(g_arMissTimeout[prReorderQueParm->ucStaRecIdx][prReorderQueParm->ucTid]);
	if ((*prMissTimeout)) {
		fgMissing = TRUE;
		GET_CURRENT_SYSTIME(&rCurrentTime);
	}

	/* Check whether any packet can be indicated to the higher layer */
	while (TRUE) {
		if (QUEUE_IS_EMPTY(prReorderQue))
			break;

		/* Always examine the head packet */
		prReorderedSwRfb = (P_SW_RFB_T) QUEUE_GET_HEAD(prReorderQue);
		fgDequeuHead = FALSE;
#if CFG_RX_BA_REORDERING_ENHANCEMENT
		qmHandleNoNeedWaitPktList(prReorderQueParm);
#endif
		DBGLOG(QM, TRACE, "qmPopOutDueToFallWithin SSN: %u, WS: %u, WE: %u\n",
			prReorderedSwRfb->u2SSN, prReorderQueParm->u2WinStart, prReorderQueParm->u2WinEnd);


		/* SN == WinStart, so the head packet shall be indicated (advance the window) */
		if ((prReorderedSwRfb->u2SSN) == (prReorderQueParm->u2WinStart)) {

			fgDequeuHead = TRUE;
			prReorderQueParm->u2WinStart = (((prReorderedSwRfb->u2SSN) + 1) % MAX_SEQ_NO_COUNT);
		}
		/* SN > WinStart, break to update WinEnd */
		else {
			/* Start bubble timer */
			if (!prReorderQueParm->fgHasBubble) {
				cnmTimerStartTimer(prAdapter,
						   &(prReorderQueParm->rReorderBubbleTimer),
						   QM_RX_BA_ENTRY_MISS_TIMEOUT_MS);
				prReorderQueParm->fgHasBubble = TRUE;
				prReorderQueParm->u2FirstBubbleSn = prReorderQueParm->u2WinStart;

				DBGLOG(QM, TRACE,
				       "QM:Bub Timer STA[%u] TID[%u] BubSN[%u] Win{%d, %d}\n",
					prReorderQueParm->ucStaRecIdx, prReorderedSwRfb->ucTid,
					prReorderQueParm->u2FirstBubbleSn,
					prReorderQueParm->u2WinStart, prReorderQueParm->u2WinEnd);
			}
			if ((fgMissing == TRUE) &&
			    CHECK_FOR_TIMEOUT(rCurrentTime, (*prMissTimeout),
					      MSEC_TO_SYSTIME(QM_RX_BA_ENTRY_MISS_TIMEOUT_MS))) {
				DBGLOG(QM, TRACE,
				       "QM:RX BA Timout Next Tid %d SSN %d\n", prReorderQueParm->ucTid,
					prReorderedSwRfb->u2SSN);
				fgDequeuHead = TRUE;
				prReorderQueParm->u2WinStart = (((prReorderedSwRfb->u2SSN) + 1) % MAX_SEQ_NO_COUNT);

				fgMissing = FALSE;
				*fgIsTimeout = TRUE;
			} else
				break;
		}

		/* Dequeue the head packet */
		if (fgDequeuHead) {

			if (((P_QUE_ENTRY_T) prReorderedSwRfb)->prNext == NULL) {
				prReorderQue->prHead = NULL;
				prReorderQue->prTail = NULL;
			} else {
				prReorderQue->prHead = ((P_QUE_ENTRY_T) prReorderedSwRfb)->prNext;
				(((P_QUE_ENTRY_T) prReorderedSwRfb)->prNext)->prPrev = NULL;
			}
			prReorderQue->u4NumElem--;
			/*
			 * DbgPrint("QM: [%d] %d (%d)\n",
			 * prReorderQueParm->ucTid,
			 * prReorderedSwRfb->u2PacketLen,
			 * prReorderedSwRfb->u2SSN);
			 */
			QUEUE_INSERT_TAIL(prReturnedQue, (P_QUE_ENTRY_T) prReorderedSwRfb);
		}
	}

	if (QUEUE_IS_EMPTY(prReorderQue))
		*prMissTimeout = 0;
	else {
		if (fgMissing == FALSE)
			GET_CURRENT_SYSTIME(prMissTimeout);
	}

	/* After WinStart has been determined, update the WinEnd */
	prReorderQueParm->u2WinEnd =
	    (((prReorderQueParm->u2WinStart) + (prReorderQueParm->u2WinSize) - 1) % MAX_SEQ_NO_COUNT);
	return QUEUE_IS_EMPTY(prReorderQue);
}

VOID qmPopOutDueToFallAhead(P_ADAPTER_T prAdapter, IN P_RX_BA_ENTRY_T prReorderQueParm, OUT P_QUE_T prReturnedQue)
{
	P_SW_RFB_T prReorderedSwRfb;
	P_QUE_T prReorderQue;
	BOOLEAN fgDequeuHead;

	prReorderQue = &(prReorderQueParm->rReOrderQue);

	/* Check whether any packet can be indicated to the higher layer */
	while (TRUE) {
		if (QUEUE_IS_EMPTY(prReorderQue))
			break;

		/* Always examine the head packet */
		prReorderedSwRfb = (P_SW_RFB_T) QUEUE_GET_HEAD(prReorderQue);
		fgDequeuHead = FALSE;
#if CFG_RX_BA_REORDERING_ENHANCEMENT
		qmHandleNoNeedWaitPktList(prReorderQueParm);
#endif
		DBGLOG(QM, TRACE, "qmPopOutDueToFallAhead SSN: %u, WS: %u, WE: %u\n",
			prReorderedSwRfb->u2SSN, prReorderQueParm->u2WinStart, prReorderQueParm->u2WinEnd);

		/* SN == WinStart, so the head packet shall be indicated (advance the window) */
		if ((prReorderedSwRfb->u2SSN) == (prReorderQueParm->u2WinStart)) {

			fgDequeuHead = TRUE;
			prReorderQueParm->u2WinStart = (((prReorderedSwRfb->u2SSN) + 1) % MAX_SEQ_NO_COUNT);
		}

		/* SN < WinStart, so the head packet shall be indicated (do not advance the window) */
		else if (qmCompareSnIsLessThan((UINT_32) (prReorderedSwRfb->u2SSN),
					       (UINT_32) (prReorderQueParm->u2WinStart)))
			fgDequeuHead = TRUE;

		/* SN > WinStart, break to update WinEnd */
		else {
			prReorderQueParm->fgHasBubbleInQue = TRUE;
			/* Start bubble timer */
			if (!prReorderQueParm->fgHasBubble) {
				cnmTimerStartTimer(prAdapter,
						   &(prReorderQueParm->rReorderBubbleTimer),
						   QM_RX_BA_ENTRY_MISS_TIMEOUT_MS);
				prReorderQueParm->fgHasBubble = TRUE;
				prReorderQueParm->u2FirstBubbleSn = prReorderQueParm->u2WinStart;

				DBGLOG(QM, TRACE,
				       "QM:(Bub Timer) STA[%u] TID[%u] BubSN[%u] Win{%d, %d}\n",
					prReorderQueParm->ucStaRecIdx, prReorderedSwRfb->ucTid,
					prReorderQueParm->u2FirstBubbleSn,
					prReorderQueParm->u2WinStart, prReorderQueParm->u2WinEnd);
			}
			break;
		}

		/* Dequeue the head packet */
		if (fgDequeuHead) {

			if (((P_QUE_ENTRY_T) prReorderedSwRfb)->prNext == NULL) {
				prReorderQue->prHead = NULL;
				prReorderQue->prTail = NULL;
			} else {
				prReorderQue->prHead = ((P_QUE_ENTRY_T) prReorderedSwRfb)->prNext;
				(((P_QUE_ENTRY_T) prReorderedSwRfb)->prNext)->prPrev = NULL;
			}
			prReorderQue->u4NumElem--;
			/* DbgPrint("QM: [%d] %d (%d)\n", */
			/* prReorderQueParm->ucTid, prReorderedSwRfb->u2PacketLen, prReorderedSwRfb->u2SSN); */
			QUEUE_INSERT_TAIL(prReturnedQue, (P_QUE_ENTRY_T) prReorderedSwRfb);
		}
	}

	/* After WinStart has been determined, update the WinEnd */
	prReorderQueParm->u2WinEnd =
	    (((prReorderQueParm->u2WinStart) + (prReorderQueParm->u2WinSize) - 1) % MAX_SEQ_NO_COUNT);

}

BOOLEAN qmCompareSnIsLessThan(IN UINT_32 u4SnLess, IN UINT_32 u4SnGreater)
{
	/* 0 <--->  SnLess   <--(gap>2048)--> SnGreater : SnLess > SnGreater */
	if ((u4SnLess + HALF_SEQ_NO_COUNT) <= u4SnGreater)	/* Shall be <= */
		return FALSE;

	/* 0 <---> SnGreater <--(gap>2048)--> SnLess    : SnLess < SnGreater */
	else if ((u4SnGreater + HALF_SEQ_NO_COUNT) < u4SnLess)
		return TRUE;

	/* 0 <---> SnGreater <--(gap<2048)--> SnLess    : SnLess > SnGreater */
	/* 0 <--->  SnLess   <--(gap<2048)--> SnGreater : SnLess < SnGreater */
	else if (u4SnLess < u4SnGreater)
		return TRUE;
	else
		return FALSE;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief Handle Mailbox RX messages
*
* \param[in] prMailboxRxMsg The received Mailbox message from the FW
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/
VOID qmHandleMailboxRxMessage(IN MAILBOX_MSG_T prMailboxRxMsg)
{
	/* DbgPrint("QM: Enter qmHandleMailboxRxMessage()\n"); */
	/* TODO */
}

/*----------------------------------------------------------------------------*/
/*!
* \brief Handle ADD RX BA Event from the FW
*
* \param[in] prAdapter Adapter pointer
* \param[in] prEvent The event packet from the FW
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/
VOID qmHandleEventRxAddBa(IN P_ADAPTER_T prAdapter, IN P_WIFI_EVENT_T prEvent)
{
	P_EVENT_RX_ADDBA_T prEventRxAddBa;
	P_STA_RECORD_T prStaRec;
	UINT_32 u4Tid;
	UINT_32 u4WinSize;

	DBGLOG(QM, INFO, "QM:Event +RxBa\n");

	prEventRxAddBa = (P_EVENT_RX_ADDBA_T) prEvent;
	prStaRec = QM_GET_STA_REC_PTR_FROM_INDEX(prAdapter, prEventRxAddBa->ucStaRecIdx);

	if (!prStaRec) {
		/* Invalid STA_REC index, discard the event packet */
		/* ASSERT(0); */
		DBGLOG(QM, WARN, "QM: (Warning) RX ADDBA Event for a NULL STA_REC\n");
		return;
	}
#if 0
	if (!(prStaRec->fgIsValid)) {
		/* TODO: (Tehuang) Handle the Host-FW synchronization issue */
		DBGLOG(QM, WARN, "QM: (Warning) RX ADDBA Event for an invalid STA_REC\n");
		/* ASSERT(0); */
		/* return; */
	}
#endif

	u4Tid = (((prEventRxAddBa->u2BAParameterSet) & BA_PARAM_SET_TID_MASK)
		 >> BA_PARAM_SET_TID_MASK_OFFSET);

	u4WinSize = (((prEventRxAddBa->u2BAParameterSet) & BA_PARAM_SET_BUFFER_SIZE_MASK)
		     >> BA_PARAM_SET_BUFFER_SIZE_MASK_OFFSET);

	if (!qmAddRxBaEntry(prAdapter,
			    prStaRec->ucIndex,
			    (UINT_8) u4Tid,
			    (prEventRxAddBa->u2BAStartSeqCtrl >> OFFSET_BAR_SSC_SN), (UINT_16) u4WinSize)) {

		/* FW shall ensure the availabiilty of the free-to-use BA entry */
		DBGLOG(QM, ERROR, "QM: (Error) qmAddRxBaEntry() failure\n");
		ASSERT(0);
	}

}

/*----------------------------------------------------------------------------*/
/*!
* \brief Handle DEL RX BA Event from the FW
*
* \param[in] prAdapter Adapter pointer
* \param[in] prEvent The event packet from the FW
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/
VOID qmHandleEventRxDelBa(IN P_ADAPTER_T prAdapter, IN P_WIFI_EVENT_T prEvent)
{
	P_EVENT_RX_DELBA_T prEventRxDelBa;
	P_STA_RECORD_T prStaRec;

	/* DbgPrint("QM:Event -RxBa\n"); */

	prEventRxDelBa = (P_EVENT_RX_DELBA_T) prEvent;
	prStaRec = QM_GET_STA_REC_PTR_FROM_INDEX(prAdapter, prEventRxDelBa->ucStaRecIdx);

	if (!prStaRec)
		/* Invalid STA_REC index, discard the event packet */
		/* ASSERT(0); */
		return;
#if 0
	if (!(prStaRec->fgIsValid))
		/* TODO: (Tehuang) Handle the Host-FW synchronization issue */
		/* ASSERT(0); */
		return;
#endif

	qmDelRxBaEntry(prAdapter, prStaRec->ucIndex, prEventRxDelBa->ucTid, TRUE);

}

P_RX_BA_ENTRY_T qmLookupRxBaEntry(IN P_ADAPTER_T prAdapter, UINT_8 ucStaRecIdx, UINT_8 ucTid)
{
	int i;
	P_QUE_MGT_T prQM = &prAdapter->rQM;

	/* DbgPrint("QM: Enter qmLookupRxBaEntry()\n"); */

	for (i = 0; i < CFG_NUM_OF_RX_BA_AGREEMENTS; i++) {
		if (prQM->arRxBaTable[i].fgIsValid) {
			if ((prQM->arRxBaTable[i].ucStaRecIdx == ucStaRecIdx) && (prQM->arRxBaTable[i].ucTid == ucTid))
				return &prQM->arRxBaTable[i];
		}
	}
	return NULL;
}

BOOLEAN
qmAddRxBaEntry(IN P_ADAPTER_T prAdapter,
	       IN UINT_8 ucStaRecIdx, IN UINT_8 ucTid, IN UINT_16 u2WinStart, IN UINT_16 u2WinSize)
{
	int i;
	P_RX_BA_ENTRY_T prRxBaEntry = NULL;
	P_STA_RECORD_T prStaRec;
	P_QUE_MGT_T prQM = &prAdapter->rQM;

	ASSERT(ucStaRecIdx < CFG_NUM_OF_STA_RECORD);

	if (ucStaRecIdx >= CFG_NUM_OF_STA_RECORD) {
		/* Invalid STA_REC index, discard the event packet */
		DBGLOG(QM, WARN, "QM: (WARNING) RX ADDBA Event for a invalid ucStaRecIdx = %d\n", ucStaRecIdx);
		return FALSE;
	}

	prStaRec = &prAdapter->arStaRec[ucStaRecIdx];
	ASSERT(prStaRec);

	/* if(!(prStaRec->fgIsValid)){ */
	/* DbgPrint("QM: (WARNING) Invalid STA when adding an RX BA\n"); */
	/* return FALSE; */
	/* } */

	/* 4 <1> Delete before adding */
	/* Remove the BA entry for the same (STA, TID) tuple if it exists */
	if (qmLookupRxBaEntry(prAdapter, ucStaRecIdx, ucTid))
		qmDelRxBaEntry(prAdapter, ucStaRecIdx, ucTid, TRUE);	/* prQM->ucRxBaCount-- */
	/* 4 <2> Add a new BA entry */
	/* No available entry to store the BA agreement info. Retrun FALSE. */
	if (prQM->ucRxBaCount >= CFG_NUM_OF_RX_BA_AGREEMENTS) {
		DBGLOG(QM, ERROR, "QM: **failure** (limited resource, ucRxBaCount=%d)\n", prQM->ucRxBaCount);
		return FALSE;
	}
	/* Find the free-to-use BA entry */
	for (i = 0; i < CFG_NUM_OF_RX_BA_AGREEMENTS; i++) {
		if (!prQM->arRxBaTable[i].fgIsValid) {
			prRxBaEntry = &(prQM->arRxBaTable[i]);
			prQM->ucRxBaCount++;
			DBGLOG(QM, LOUD, "QM: ucRxBaCount=%d\n", prQM->ucRxBaCount);
			break;
		}
	}
	/* If a free-to-use entry is found, configure it and associate it with the STA_REC */
	u2WinSize += CFG_RX_BA_INC_SIZE;
	if (prRxBaEntry) {
		prRxBaEntry->ucStaRecIdx = ucStaRecIdx;
		prRxBaEntry->ucTid = ucTid;
		prRxBaEntry->u2WinStart = u2WinStart;
		prRxBaEntry->u2WinSize = u2WinSize;
		prRxBaEntry->u2WinEnd = ((u2WinStart + u2WinSize - 1) % MAX_SEQ_NO_COUNT);
		prRxBaEntry->fgIsValid = TRUE;
		prRxBaEntry->fgIsWaitingForPktWithSsn = TRUE;
		prRxBaEntry->fgHasBubble = FALSE;

		g_arMissTimeout[ucStaRecIdx][ucTid] = 0;

		DBGLOG(QM, INFO, "QM: +RxBA(STA=%d TID=%d WinStart=%d WinEnd=%d WinSize=%d)\n",
				  ucStaRecIdx, ucTid,
				  prRxBaEntry->u2WinStart, prRxBaEntry->u2WinEnd, prRxBaEntry->u2WinSize);

		/* Update the BA entry reference table for per-packet lookup */
		prStaRec->aprRxReorderParamRefTbl[ucTid] = prRxBaEntry;
	} else {
		/* This shall not happen because FW should keep track of the usage of RX BA entries */
		DBGLOG(QM, ERROR, "QM: **AddBA Error** (ucRxBaCount=%d)\n", prQM->ucRxBaCount);
		return FALSE;
	}
	return TRUE;
}

VOID qmDelRxBaEntry(IN P_ADAPTER_T prAdapter, IN UINT_8 ucStaRecIdx, IN UINT_8 ucTid, IN BOOLEAN fgFlushToHost)
{
	P_RX_BA_ENTRY_T prRxBaEntry;
	P_STA_RECORD_T prStaRec;
	P_SW_RFB_T prFlushedPacketList = NULL;
	P_QUE_MGT_T prQM = &prAdapter->rQM;

	ASSERT(ucStaRecIdx < CFG_NUM_OF_STA_RECORD);

	prStaRec = &prAdapter->arStaRec[ucStaRecIdx];
	ASSERT(prStaRec);

#if 0
	if (!(prStaRec->fgIsValid)) {
		DbgPrint("QM: (WARNING) Invalid STA when deleting an RX BA\n");
		return;
	}
#endif

	/* Remove the BA entry for the same (STA, TID) tuple if it exists */
	prRxBaEntry = prStaRec->aprRxReorderParamRefTbl[ucTid];

	if (prRxBaEntry) {

		prFlushedPacketList = qmFlushStaRxQueue(prAdapter, ucStaRecIdx, ucTid);

		if (prFlushedPacketList) {

			if (fgFlushToHost) {
				wlanProcessQueuedSwRfb(prAdapter, prFlushedPacketList);
			} else {

				P_SW_RFB_T prSwRfb;
				P_SW_RFB_T prNextSwRfb;

				prSwRfb = prFlushedPacketList;

				do {
					prNextSwRfb = (P_SW_RFB_T) QUEUE_GET_NEXT_ENTRY((P_QUE_ENTRY_T) prSwRfb);
					nicRxReturnRFB(prAdapter, prSwRfb);
					prSwRfb = prNextSwRfb;
				} while (prSwRfb);

			}

		}
		if (prRxBaEntry->fgHasBubble) {
			DBGLOG(QM, TRACE, "QM:(Bub Check Cancel) STA[%u] TID[%u], DELBA\n",
					   prRxBaEntry->ucStaRecIdx, prRxBaEntry->ucTid);

			cnmTimerStopTimer(prAdapter, &prRxBaEntry->rReorderBubbleTimer);
			prRxBaEntry->fgHasBubble = FALSE;
		}
#if ((QM_TEST_MODE == 0) && (QM_TEST_STA_REC_DEACTIVATION == 0))
		/* Update RX BA entry state. Note that RX queue flush is not done here */
		prRxBaEntry->fgIsValid = FALSE;
		prQM->ucRxBaCount--;

		/* Debug */
#if 0
		DbgPrint("QM: ucRxBaCount=%d\n", prQM->ucRxBaCount);
#endif

		/* Update STA RX BA table */
		prStaRec->aprRxReorderParamRefTbl[ucTid] = NULL;
#endif

		DBGLOG(QM, INFO, "QM: -RxBA(STA=%d,TID=%d)\n", ucStaRecIdx, ucTid);

	}

	/* Debug */
#if CFG_HIF_RX_STARVATION_WARNING
	{
		P_RX_CTRL_T prRxCtrl;

		prRxCtrl = &prAdapter->rRxCtrl;
		DBGLOG(QM, TRACE,
		       "QM: (RX DEBUG) Enqueued: %d / Dequeued: %d\n", prRxCtrl->u4QueuedCnt,
			prRxCtrl->u4DequeuedCnt);
	}
#endif
}

#if CFG_RX_BA_REORDERING_ENHANCEMENT
VOID qmInsertNoNeedWaitPkt(IN P_ADAPTER_T prAdapter,
		IN P_SW_RFB_T prSwRfb, IN ENUM_NO_NEED_WATIT_DROP_REASON_T eDropReason)
{
	P_STA_RECORD_T prStaRec;
	P_RX_BA_ENTRY_T prRxBaEntry;
	P_NO_NEED_WAIT_PKT_T prNoNeedWaitPkt;

	/* Check whether the STA_REC is activated */
	prStaRec = &(prAdapter->arStaRec[prSwRfb->ucStaRecIdx]);
	ASSERT(prStaRec);

	prRxBaEntry = ((prStaRec->aprRxReorderParamRefTbl)[prSwRfb->ucTid]);

	if (!(prRxBaEntry) || !(prRxBaEntry->fgIsValid)) {
		DBGLOG(QM, WARN, "qmInsertNoNeedWaitPkt for a NULL ReorderQueParm, SSN:[%u], DropReason:(%d)\n",
			prSwRfb->u2SSN, eDropReason);
		return;
	}

	prNoNeedWaitPkt = (P_NO_NEED_WAIT_PKT_T) kalMemAlloc(sizeof(NO_NEED_WAIT_PKT_T), VIR_MEM_TYPE);

	if (prNoNeedWaitPkt == NULL) {
		DBGLOG(QM, ERROR, "qmInsertNoNeedWaitPkt alloc error SSN:[%u], DropReason:(%d)\n",
			prSwRfb->u2SSN, eDropReason);
		return;
	}

	prNoNeedWaitPkt->u2SSN = prSwRfb->u2SSN;
	prNoNeedWaitPkt->eDropReason = eDropReason;
	DBGLOG(QM, INFO, "qmInsertNoNeedWaitPkt SSN:[%u], DropReason:(%d)\n", prSwRfb->u2SSN, eDropReason);
	QUEUE_INSERT_TAIL(&(prRxBaEntry->rNoNeedWaitQue), (P_QUE_ENTRY_T) prNoNeedWaitPkt);
}

VOID qmHandleEventDropByFW(IN P_ADAPTER_T prAdapter, IN P_WIFI_EVENT_T prEvent)
{
	P_EVENT_PACKET_DROP_BY_FW_T prDropSSNEvt = (P_EVENT_PACKET_DROP_BY_FW_T) prEvent;
	P_RX_BA_ENTRY_T prRxBaEntry;
	UINT_16 u2StartSSN;
	UINT_8 u1BitmapSSN;
	UINT_8 u1SetCount, u1Count;
	UINT_16 u2OfCount;
	SW_RFB_T rSwRfb;

	u2StartSSN = QM_GET_DROP_BY_FW_SSN(prDropSSNEvt->u2StartSSN);

	/* Get target Rx BA entry */
	prRxBaEntry = qmLookupRxBaEntry(prAdapter, prDropSSNEvt->ucStaRecIdx, prDropSSNEvt->ucTid);
	rSwRfb.ucTid = prDropSSNEvt->ucTid;
	rSwRfb.ucStaRecIdx = prDropSSNEvt->ucStaRecIdx;

	/* Sanity Check */
	if (!prRxBaEntry) {
		DBGLOG(QM, ERROR, "qmHandleEventDropByFW STA[%u] TID[%u], No Rx BA entry\n",
				   prDropSSNEvt->ucStaRecIdx, prDropSSNEvt->ucTid);
		return;
	}

	u2OfCount = 0;

	for (u1SetCount = 0; u1SetCount < QM_RX_MAX_FW_DROP_SSN_SIZE; u1SetCount++) {
		u1BitmapSSN = prDropSSNEvt->au1BitmapSSN[u1SetCount];

		for (u1Count = 0; u1Count < 8; u1Count++) {
			if ((u1BitmapSSN & BIT(0)) == 1) {
				rSwRfb.u2SSN = u2StartSSN + u2OfCount;
				qmInsertNoNeedWaitPkt(prAdapter, &rSwRfb, PACKET_DROP_BY_FW);
			}
			u1BitmapSSN >>= 1;
			u2OfCount++;
		}
	}
}

VOID qmHandleNoNeedWaitPktList(IN P_RX_BA_ENTRY_T prReorderQueParm)
{
	P_QUE_T prNoNeedWaitQue;
	P_NO_NEED_WAIT_PKT_T prNoNeedWaitPkt;
	P_NO_NEED_WAIT_PKT_T prNoNeedWaitNextPkt;
	UINT_16 u2SSN, u2WinStart, u2WinEnd, u2AheadPoint;

	prNoNeedWaitQue = &(prReorderQueParm->rNoNeedWaitQue);

	if (QUEUE_IS_NOT_EMPTY(prNoNeedWaitQue)) {
		prNoNeedWaitPkt = (P_NO_NEED_WAIT_PKT_T) QUEUE_GET_HEAD(prNoNeedWaitQue);

		u2SSN = prNoNeedWaitPkt->u2SSN;
		u2WinStart = prReorderQueParm->u2WinStart;
		u2WinEnd = prReorderQueParm->u2WinEnd;

		/* Remove all packets that SSN is less than WinStart */
		do {
			u2SSN = prNoNeedWaitPkt->u2SSN;
			prNoNeedWaitNextPkt =
				(P_NO_NEED_WAIT_PKT_T) QUEUE_GET_NEXT_ENTRY((P_QUE_ENTRY_T) prNoNeedWaitPkt);

			u2AheadPoint = u2WinStart + HALF_SEQ_NO_COUNT;
			if (u2AheadPoint >= MAX_SEQ_NO_COUNT)
				u2AheadPoint -= MAX_SEQ_NO_COUNT;

			if	/*0: End - AheadPoint - SSN - Start :4095*/
				(((u2SSN < u2WinStart)
					&& (u2AheadPoint < u2SSN)
					&& (u2WinEnd < u2AheadPoint))
				/*0: Start - End - AheadPoint - SSN :4095*/
				|| ((u2AheadPoint < u2SSN)
					&& (u2WinEnd < u2AheadPoint)
					&& (u2WinStart < u2WinEnd))
				/*0: SSN - Start - End - AheadPhoint :4095*/
				|| ((u2WinEnd < u2AheadPoint)
					&& (u2WinStart < u2WinEnd)
					&& (u2SSN < u2WinStart))
				/*0: AheadPoint - SSN - Start - End :4095*/
				|| ((u2WinStart < u2WinEnd)
					&& (u2SSN < u2WinStart)
					&& (u2AheadPoint < u2SSN))) {

				QUEUE_REMOVE_HEAD(prNoNeedWaitQue, prNoNeedWaitPkt, P_NO_NEED_WAIT_PKT_T);
				kalMemFree(prNoNeedWaitPkt, VIR_MEM_TYPE, sizeof(NO_NEED_WAIT_PKT_T));

				DBGLOG(QM, TRACE, "qmHandleNoNeedWaitPktList Remove SSN:[%u], WS:%u, WE:%u\n",
					u2SSN, u2WinStart, u2WinEnd);
			}


			prNoNeedWaitPkt = prNoNeedWaitNextPkt;
		} while (prNoNeedWaitPkt);

		/* Adjust WinStart if current WinStart is contain in NoNeedWaitQue */
		while ((prNoNeedWaitPkt =
				qmSearchNoNeedWaitPktBySSN(prReorderQueParm, prReorderQueParm->u2WinStart)) != NULL) {
			prReorderQueParm->u2WinStart = (((prNoNeedWaitPkt->u2SSN) + 1) % MAX_SEQ_NO_COUNT);
			prReorderQueParm->u2WinEnd =
			(((prReorderQueParm->u2WinStart) + (prReorderQueParm->u2WinSize) - 1) % MAX_SEQ_NO_COUNT);
			QUEUE_REMOVE_HEAD(prNoNeedWaitQue, prNoNeedWaitPkt, P_NO_NEED_WAIT_PKT_T);
			kalMemFree(prNoNeedWaitPkt, VIR_MEM_TYPE, sizeof(NO_NEED_WAIT_PKT_T));
		}
	}
}

VOID qmHandleNoNeedWaitStopBubTimer(IN P_ADAPTER_T prAdapter, IN P_RX_BA_ENTRY_T prReorderQueParm)
{
	QUE_T rReturnedQue;
	P_QUE_T prReturnedQue = &rReturnedQue;
	P_SW_RFB_T prSwRfb;

	QUEUE_INITIALIZE(prReturnedQue);
	prReorderQueParm->fgHasBubbleInQue = FALSE;

	qmPopOutDueToFallAhead(prAdapter, prReorderQueParm, prReturnedQue);

	DBGLOG(QM, INFO, "qmHandleNoNeedWaitStopBubTimer BubInQue[%d] STA[%u] TID[%u] BubSN[%u] Win{%d, %d}\n",
			   prReorderQueParm->fgHasBubbleInQue,
			   prReorderQueParm->ucStaRecIdx,
			   prReorderQueParm->ucTid,
			   prReorderQueParm->u2FirstBubbleSn,
			   prReorderQueParm->u2WinStart, prReorderQueParm->u2WinEnd);

	if (prReorderQueParm->fgHasBubbleInQue == FALSE) {
		/* Stop bubble timer if there are no bubbles in reorder queue. */
		cnmTimerStopTimer(prAdapter, &(prReorderQueParm->rReorderBubbleTimer));

		prReorderQueParm->fgHasBubble = FALSE;

		if (QUEUE_IS_NOT_EMPTY(prReturnedQue)) {
			QM_TX_SET_NEXT_MSDU_INFO((P_SW_RFB_T) QUEUE_GET_TAIL(prReturnedQue), NULL);

			prSwRfb = (P_SW_RFB_T) QUEUE_GET_HEAD(prReturnedQue);
			while (prSwRfb) {
				DBGLOG(QM, TRACE,
					   "qmHandleNoNeedWaitStopBubTimer Flush STA[%u] TID[%u] Pop Out SN[%u]\n",
					prReorderQueParm->ucStaRecIdx, prReorderQueParm->ucTid, prSwRfb->u2SSN);

				prSwRfb = (P_SW_RFB_T) QUEUE_GET_NEXT_ENTRY((P_QUE_ENTRY_T) prSwRfb);
			}

			wlanProcessQueuedSwRfb(prAdapter, (P_SW_RFB_T) QUEUE_GET_HEAD(prReturnedQue));
		} else {
			DBGLOG(QM, TRACE, "qmHandleNoNeedWaitStopBubTimer Flush STA[%u] TID[%u] Pop Out 0 packet\n",
					   prReorderQueParm->ucStaRecIdx, prReorderQueParm->ucTid);
		}
	}

	qmHandleMissTimeout(prReorderQueParm);
}

P_NO_NEED_WAIT_PKT_T qmSearchNoNeedWaitPktBySSN(IN P_RX_BA_ENTRY_T prReorderQueParm, IN UINT_32 u2SSN)
{
	P_QUE_T prNoNeedWaitQue = NULL;
	P_NO_NEED_WAIT_PKT_T prNoNeedWaitPkt = NULL;

	prNoNeedWaitQue = &(prReorderQueParm->rNoNeedWaitQue);

	if (QUEUE_IS_NOT_EMPTY(prNoNeedWaitQue)) {
		prNoNeedWaitPkt = (P_NO_NEED_WAIT_PKT_T) QUEUE_GET_HEAD(prNoNeedWaitQue);

		do {
			if (prNoNeedWaitPkt->u2SSN == u2SSN)
				return prNoNeedWaitPkt;

			prNoNeedWaitPkt = (P_NO_NEED_WAIT_PKT_T) QUEUE_GET_NEXT_ENTRY((P_QUE_ENTRY_T) prNoNeedWaitPkt);
		} while (prNoNeedWaitPkt);
	}

	return NULL;
}

VOID qmRemoveAllNoNeedWaitPkt(IN P_RX_BA_ENTRY_T prReorderQueParm)
{
	P_QUE_T prNoNeedWaitQue;
	P_NO_NEED_WAIT_PKT_T prNoNeedWaitPkt;
	P_NO_NEED_WAIT_PKT_T prNoNeedWaitNextPkt;

	prNoNeedWaitQue = &(prReorderQueParm->rNoNeedWaitQue);

	if (QUEUE_IS_NOT_EMPTY(prNoNeedWaitQue)) {
		prNoNeedWaitPkt = (P_NO_NEED_WAIT_PKT_T) QUEUE_GET_HEAD(prNoNeedWaitQue);

		do {
			prNoNeedWaitNextPkt =
				(P_NO_NEED_WAIT_PKT_T) QUEUE_GET_NEXT_ENTRY((P_QUE_ENTRY_T) prNoNeedWaitPkt);
			QUEUE_REMOVE_HEAD(prNoNeedWaitQue, prNoNeedWaitPkt, P_NO_NEED_WAIT_PKT_T);
			kalMemFree(prNoNeedWaitPkt, VIR_MEM_TYPE, sizeof(NO_NEED_WAIT_PKT_T));
			prNoNeedWaitPkt = prNoNeedWaitNextPkt;
		} while (prNoNeedWaitNextPkt);
	}
}

VOID qmDumpNoNeedWaitPkt(IN P_RX_BA_ENTRY_T prReorderQueParm)
{
	P_QUE_T prNoNeedWaitQue;
	P_NO_NEED_WAIT_PKT_T prNoNeedWaitPkt;

	prNoNeedWaitQue = &(prReorderQueParm->rNoNeedWaitQue);

	if (QUEUE_IS_NOT_EMPTY(prNoNeedWaitQue)) {
		prNoNeedWaitPkt = (P_NO_NEED_WAIT_PKT_T) QUEUE_GET_HEAD(prNoNeedWaitQue);

		do {
			DBGLOG(QM, INFO, "qmDumpNoNeedWaitPkt > SSN:[%u] DropReason:(%d)\n",
				prNoNeedWaitPkt->u2SSN, prNoNeedWaitPkt->eDropReason);
			prNoNeedWaitPkt = (P_NO_NEED_WAIT_PKT_T) QUEUE_GET_NEXT_ENTRY((P_QUE_ENTRY_T) prNoNeedWaitPkt);
		} while (prNoNeedWaitPkt);
	} else
		DBGLOG(QM, INFO, "qmDumpNoNeedWaitPkt > QUEUE EMPTY\n");
}

BOOLEAN qmIsIndependentPkt(IN P_SW_RFB_T prSwRfb)
{
	PUINT_8 pucPkt = prSwRfb->pvHeader;
	UINT_16 u2EtherType = kalGetPktEtherType(pucPkt);
	BOOLEAN fgIsIndependent = FALSE;

	switch (u2EtherType) {
	case ENUM_PKT_ARP:
	case ENUM_PKT_ICMP:
	case ENUM_PKT_DHCP:
	case ENUM_PKT_DNS:
		fgIsIndependent = TRUE;
		break;
	default:
		break;
	}

	DBGLOG(QM, TRACE, "qmIsIndependentPkt type:%d,fgIsIndependent %d\n", u2EtherType, fgIsIndependent);

	return fgIsIndependent;

}
VOID qmProcessIndepentReorderQueue(IN P_ADAPTER_T prAdapter, IN P_SW_RFB_T prSwRfb)
{
	P_RX_BA_ENTRY_T prRxBaEntry;

	prSwRfb->u2SSN = HIF_RX_HDR_GET_SN(prSwRfb->prHifRxHdr);
	prSwRfb->ucTid = (UINT_8) (HIF_RX_HDR_GET_TID(prSwRfb->prHifRxHdr));

	/* Insert to NoNeedWaitPkt queue */
	qmInsertNoNeedWaitPkt(prAdapter, prSwRfb, PACKET_DROP_BY_INDEPENDENT_PKT);

	/* handle NoNeedWaitPkt queue*/
	prRxBaEntry = qmLookupRxBaEntry(prAdapter, prSwRfb->ucStaRecIdx, prSwRfb->ucTid);
	qmHandleNoNeedWaitPktList(prRxBaEntry);

}

#endif


/*----------------------------------------------------------------------------*/
/*!
* \brief To process WMM related IEs in ASSOC_RSP
*
* \param[in] prAdapter Adapter pointer
* \param[in] prSwRfb            The received frame
* \param[in] pucIE              The pointer to the first IE in the frame
* \param[in] u2IELength         The total length of IEs in the frame
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID mqmProcessAssocReq(IN P_ADAPTER_T prAdapter, IN P_SW_RFB_T prSwRfb, IN PUINT_8 pucIE, IN UINT_16 u2IELength)
{
	P_STA_RECORD_T prStaRec;
	UINT_16 u2Offset;
	PUINT_8 pucIEStart;
	UINT_8 aucWfaOui[] = VENDOR_OUI_WFA;
	P_IE_WMM_INFO_T prIeWmmInfo;
	UINT_8 ucQosInfo;
	UINT_8 ucQosInfoAC;
	UINT_8 ucBmpAC;

	DEBUGFUNC("mqmProcessAssocReq");

	ASSERT(prSwRfb);
	ASSERT(pucIE);

	prStaRec = cnmGetStaRecByIndex(prAdapter, prSwRfb->ucStaRecIdx);
	ASSERT(prStaRec);

	if (prStaRec == NULL)
		return;

	prStaRec->fgIsQoS = FALSE;
	prStaRec->fgIsWmmSupported = prStaRec->fgIsUapsdSupported = FALSE;

	pucIEStart = pucIE;

	/* If the device does not support QoS or if WMM is not supported by the peer, exit. */
	if (!prAdapter->rWifiVar.fgSupportQoS)
		return;

	/* Determine whether QoS is enabled with the association */
	IE_FOR_EACH(pucIE, u2IELength, u2Offset) {
		switch (IE_ID(pucIE)) {
		case ELEM_ID_WMM:

			if ((WMM_IE_OUI_TYPE(pucIE) == VENDOR_OUI_TYPE_WMM) &&
				(!kalMemCmp(WMM_IE_OUI(pucIE), aucWfaOui, 3))) {
				switch (WMM_IE_OUI_SUBTYPE(pucIE)) {
				case VENDOR_OUI_SUBTYPE_WMM_INFO:
					if (IE_LEN(pucIE) != 7)
						break;	/* WMM Info IE with a wrong length */
					prStaRec->fgIsQoS = TRUE;
					prStaRec->fgIsWmmSupported = TRUE;

					prIeWmmInfo = (P_IE_WMM_INFO_T) pucIE;
					ucQosInfo = prIeWmmInfo->ucQosInfo;
					ucQosInfoAC = ucQosInfo & BITS(0, 3);

					prStaRec->fgIsUapsdSupported = ((ucQosInfoAC) ? TRUE : FALSE) &
					    prAdapter->rWifiVar.fgSupportUAPSD;

					ucBmpAC = 0;

					if (ucQosInfoAC & WMM_QOS_INFO_VO_UAPSD)
						ucBmpAC |= BIT(ACI_VO);
					if (ucQosInfoAC & WMM_QOS_INFO_VI_UAPSD)
						ucBmpAC |= BIT(ACI_VI);
					if (ucQosInfoAC & WMM_QOS_INFO_BE_UAPSD)
						ucBmpAC |= BIT(ACI_BE);
					if (ucQosInfoAC & WMM_QOS_INFO_BK_UAPSD)
						ucBmpAC |= BIT(ACI_BK);

					prStaRec->ucBmpTriggerAC = prStaRec->ucBmpDeliveryAC = ucBmpAC;

					prStaRec->ucUapsdSp =
					    (ucQosInfo & WMM_QOS_INFO_MAX_SP_LEN_MASK) >> 5;
					break;
				default:
					/* Other WMM QoS IEs. Ignore any */
					break;
				}
			}
			/* else: VENDOR_OUI_TYPE_WPA, VENDOR_OUI_TYPE_WPS */

			break;

		case ELEM_ID_HT_CAP:
			/* Some client won't put the WMM IE if client is 802.11n */
			if (IE_LEN(pucIE) == (sizeof(IE_HT_CAP_T) - 2))
				prStaRec->fgIsQoS = TRUE;
			break;
		default:
			break;
		}
	}

	DBGLOG(QM, TRACE, "MQM: Assoc_Req Parsing (QoS Enabled=%d)\n", prStaRec->fgIsQoS);
}

/*----------------------------------------------------------------------------*/
/*!
* \brief To process WMM related IEs in ASSOC_RSP
*
* \param[in] prAdapter Adapter pointer
* \param[in] prSwRfb            The received frame
* \param[in] pucIE              The pointer to the first IE in the frame
* \param[in] u2IELength         The total length of IEs in the frame
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID mqmProcessAssocRsp(IN P_ADAPTER_T prAdapter, IN P_SW_RFB_T prSwRfb, IN PUINT_8 pucIE, IN UINT_16 u2IELength)
{
	P_STA_RECORD_T prStaRec;
	UINT_16 u2Offset;
	PUINT_8 pucIEStart;
	UINT_8 aucWfaOui[] = VENDOR_OUI_WFA;
	BOOLEAN hasnoQosMapSetIE = TRUE;

	DEBUGFUNC("mqmProcessAssocRsp");

	ASSERT(prSwRfb);
	ASSERT(pucIE);

	prStaRec = cnmGetStaRecByIndex(prAdapter, prSwRfb->ucStaRecIdx);
	ASSERT(prStaRec);

	if (prStaRec == NULL)
		return;

	prStaRec->fgIsQoS = FALSE;

	pucIEStart = pucIE;

	DBGLOG(QM, TRACE, "QM: (fgIsWmmSupported=%d, fgSupportQoS=%d)\n",
			   prStaRec->fgIsWmmSupported, prAdapter->rWifiVar.fgSupportQoS);

	/* If the device does not support QoS or if WMM is not supported by the peer, exit. */
	/* if((!prAdapter->rWifiVar.fgSupportQoS) || (!prStaRec->fgIsWmmSupported)) */
	if ((!prAdapter->rWifiVar.fgSupportQoS))
		return;

	/* Determine whether QoS is enabled with the association */
	IE_FOR_EACH(pucIE, u2IELength, u2Offset) {
		switch (IE_ID(pucIE)) {
		case ELEM_ID_WMM:
			if ((WMM_IE_OUI_TYPE(pucIE) == VENDOR_OUI_TYPE_WMM) &&
			    (!kalMemCmp(WMM_IE_OUI(pucIE), aucWfaOui, 3))) {

				switch (WMM_IE_OUI_SUBTYPE(pucIE)) {
				case VENDOR_OUI_SUBTYPE_WMM_PARAM:
					if (IE_LEN(pucIE) != 24)
						break;	/* WMM Info IE with a wrong length */
					prStaRec->fgIsQoS = TRUE;
					break;

				case VENDOR_OUI_SUBTYPE_WMM_INFO:
					if (IE_LEN(pucIE) != 7)
						break;	/* WMM Info IE with a wrong length */
					prStaRec->fgIsQoS = TRUE;
					break;

				default:
					/* Other WMM QoS IEs. Ignore any */
					break;
				}
			}
			/* else: VENDOR_OUI_TYPE_WPA, VENDOR_OUI_TYPE_WPS */
			break;

		case ELEM_ID_HT_CAP:
			/* Some AP won't put the WMM IE if client is 802.11n */
			if (IE_LEN(pucIE) == (sizeof(IE_HT_CAP_T) - 2))
				prStaRec->fgIsQoS = TRUE;
			break;
		case ELEM_ID_QOS_MAP_SET:
			DBGLOG(QM, WARN, "QM: received assoc resp qosmapset ie\n");
			prStaRec->qosMapSet = qosParseQosMapSet(prAdapter, pucIE);
			hasnoQosMapSetIE = FALSE;
			break;
		default:
			break;
		}
	}

	if (hasnoQosMapSetIE) {
		DBGLOG(QM, WARN, "QM: remove assoc resp qosmapset ie\n");
		QosMapSetRelease(prStaRec);
		prStaRec->qosMapSet = NULL;
	}

	/* Parse AC parameters and write to HW CRs */
	if ((prStaRec->fgIsQoS) && (prStaRec->eStaType == STA_TYPE_LEGACY_AP)) {
		mqmParseEdcaParameters(prAdapter, prSwRfb, pucIEStart, u2IELength, TRUE);
#if ARP_MONITER_ENABLE
		qmResetArpDetect();
#endif
	}

	DBGLOG(QM, TRACE, "MQM: Assoc_Rsp Parsing (QoS Enabled=%d)\n", prStaRec->fgIsQoS);
	if (prStaRec->fgIsWmmSupported)
		nicQmUpdateWmmParms(prAdapter, prStaRec->ucNetTypeIndex);
}

/*----------------------------------------------------------------------------*/
/*!
* \brief To parse WMM Parameter IE (in BCN or Assoc_Rsp)
*
* \param[in] prAdapter          Adapter pointer
* \param[in] prSwRfb            The received frame
* \param[in] pucIE              The pointer to the first IE in the frame
* \param[in] u2IELength         The total length of IEs in the frame
* \param[in] fgForceOverride    TRUE: If EDCA parameters are found, always set to HW CRs.
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID
mqmParseEdcaParameters(IN P_ADAPTER_T prAdapter,
		       IN P_SW_RFB_T prSwRfb, IN PUINT_8 pucIE, IN UINT_16 u2IELength, IN BOOLEAN fgForceOverride)
{
	P_STA_RECORD_T prStaRec;
	UINT_16 u2Offset;
	UINT_8 aucWfaOui[] = VENDOR_OUI_WFA;
	P_BSS_INFO_T prBssInfo;
	P_AC_QUE_PARMS_T prAcQueParams;
	P_IE_WMM_PARAM_T prIeWmmParam;
	ENUM_WMM_ACI_T eAci;
	PUINT_8 pucWmmParamSetCount;

	DEBUGFUNC("mqmParseEdcaParameters");

	ASSERT(prSwRfb);
	ASSERT(pucIE);

	prStaRec = cnmGetStaRecByIndex(prAdapter, prSwRfb->ucStaRecIdx);
	ASSERT(prStaRec);
	if (prStaRec == NULL)
		return;

	DBGLOG(QM, TRACE, "QM: (fgIsWmmSupported=%d, fgIsQoS=%d)\n", prStaRec->fgIsWmmSupported, prStaRec->fgIsQoS);

	if ((!prAdapter->rWifiVar.fgSupportQoS) || (!prStaRec->fgIsWmmSupported) || (!prStaRec->fgIsQoS))
		return;

	prBssInfo = &(prAdapter->rWifiVar.arBssInfo[prStaRec->ucNetTypeIndex]);

	/* Goal: Obtain the EDCA parameters */
	IE_FOR_EACH(pucIE, u2IELength, u2Offset) {
		switch (IE_ID(pucIE)) {
		case ELEM_ID_WMM:
			if ((WMM_IE_OUI_TYPE(pucIE) != VENDOR_OUI_TYPE_WMM) ||
				(kalMemCmp(WMM_IE_OUI(pucIE), aucWfaOui, 3)))
				break;

			switch (WMM_IE_OUI_SUBTYPE(pucIE)) {
			case VENDOR_OUI_SUBTYPE_WMM_PARAM:
				if (IE_LEN(pucIE) != 24)
					break;	/* WMM Param IE with a wrong length */

				pucWmmParamSetCount = &(prBssInfo->ucWmmParamSetCount);
				prIeWmmParam = (P_IE_WMM_PARAM_T) pucIE;

				/* Check the Parameter Set Count to determine whether EDCA parameters */
				/* have been changed */
				if (!fgForceOverride && (*pucWmmParamSetCount
					== (prIeWmmParam->ucQosInfo & WMM_QOS_INFO_PARAM_SET_CNT)))
					break;	/* Ignore the IE without updating HW CRs */

				/* Update Parameter Set Count */
				*pucWmmParamSetCount =
				    (prIeWmmParam->ucQosInfo & WMM_QOS_INFO_PARAM_SET_CNT);

				/* Update EDCA parameters */
				for (eAci = 0; eAci < WMM_AC_INDEX_NUM; eAci++) {

					prAcQueParams = &prBssInfo->arACQueParms[eAci];
					mqmFillAcQueParam(prIeWmmParam, eAci, prAcQueParams);

					prAcQueParams->fgIsACMSet =
					    (prAcQueParams->u2Aifsn & WMM_ACIAIFSN_ACM) ? TRUE : FALSE;
					prAcQueParams->u2Aifsn &= WMM_ACIAIFSN_AIFSN;

					DBGLOG(QM, LOUD,
					"eAci:%d, ACM:%d, Aifsn:%d, CWmin:%d, CWmax:%d, TxopLmt:%d\n",
					eAci, prAcQueParams->fgIsACMSet, prAcQueParams->u2Aifsn,
					prAcQueParams->u2CWmin, prAcQueParams->u2CWmax,
					prAcQueParams->u2TxopLimit);
				}
				break;
			default:
				/* Other WMM QoS IEs. Ignore */
				break;
			}

			/* else: VENDOR_OUI_TYPE_WPA, VENDOR_OUI_TYPE_WPS, ... (not cared) */
			break;
		default:
			break;
		}
	}
}

/*----------------------------------------------------------------------------*/
/*!
* \brief This function is used for parsing EDCA parameters specified in the WMM Parameter IE
*
* \param[in] prAdapter           Adapter pointer
* \param[in] prIeWmmParam        The pointer to the WMM Parameter IE
* \param[in] u4AcOffset          The offset specifying the AC queue for parsing
* \param[in] prHwAcParams        The parameter structure used to configure the HW CRs
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID mqmFillAcQueParam(IN P_IE_WMM_PARAM_T prIeWmmParam, IN UINT_32 u4AcOffset, OUT P_AC_QUE_PARMS_T prAcQueParams)
{
	prAcQueParams->u2Aifsn = *((PUINT_8) (&(prIeWmmParam->ucAciAifsn_BE)) + (u4AcOffset * 4));

	prAcQueParams->u2CWmax = BIT(((*((PUINT_8) (&(prIeWmmParam->ucEcw_BE)) + (u4AcOffset * 4))) & WMM_ECW_WMAX_MASK)
				     >> WMM_ECW_WMAX_OFFSET) - 1;

	prAcQueParams->u2CWmin =
	    BIT((*((PUINT_8) (&(prIeWmmParam->ucEcw_BE)) + (u4AcOffset * 4))) & WMM_ECW_WMIN_MASK) - 1;

	WLAN_GET_FIELD_16(((PUINT_8) (&(prIeWmmParam->aucTxopLimit_BE)) + (u4AcOffset * 4)),
			  &(prAcQueParams->u2TxopLimit));

	prAcQueParams->ucGuradTime = TXM_DEFAULT_FLUSH_QUEUE_GUARD_TIME;

}

/*----------------------------------------------------------------------------*/
/*!
* \brief To parse WMM/11n related IEs in scan results (only for AP peers)
*
* \param[in] prAdapter       Adapter pointer
* \param[in]  prScanResult   The scan result which shall be parsed to obtain needed info
* \param[out] prStaRec       The obtained info is stored in the STA_REC
*
* \return none
*/
/*----------------------------------------------------------------------------*/
#if (CFG_SUPPORT_TDLS == 1)	/* for test purpose */
BOOLEAN flgTdlsTestExtCapElm = FALSE;
UINT8 aucTdlsTestExtCapElm[7];
#endif /* CFG_SUPPORT_TDLS */
VOID mqmProcessScanResult(IN P_ADAPTER_T prAdapter, IN P_BSS_DESC_T prScanResult, OUT P_STA_RECORD_T prStaRec)
{
	PUINT_8 pucIE;
	UINT_16 u2IELength;
	UINT_16 u2Offset;
	UINT_8 aucWfaOui[] = VENDOR_OUI_WFA;

	DEBUGFUNC("mqmProcessScanResult");

	ASSERT(prScanResult);
	ASSERT(prStaRec);

	/* Reset the flag before parsing */
	prStaRec->fgIsWmmSupported = prStaRec->fgIsUapsdSupported = FALSE;

	if (!prAdapter->rWifiVar.fgSupportQoS)
		return;

	u2IELength = prScanResult->u2IELength;
	pucIE = prScanResult->aucIEBuf;

#if (CFG_SUPPORT_TDLS == 1)
	/* TDLS test purpose */
	if (flgTdlsTestExtCapElm == TRUE)
		TdlsexBssExtCapParse(prStaRec, aucTdlsTestExtCapElm);
#endif /* CFG_SUPPORT_TDLS */

	/* Goal: Determine whether the peer supports WMM/QoS and UAPSDU */
	IE_FOR_EACH(pucIE, u2IELength, u2Offset) {
		switch (IE_ID(pucIE)) {
		case ELEM_ID_EXTENDED_CAP:
#if (CFG_SUPPORT_TDLS == 1)
			TdlsexBssExtCapParse(prStaRec, pucIE);
#endif /* CFG_SUPPORT_TDLS */
			break;

		case ELEM_ID_WMM:
			if ((WMM_IE_OUI_TYPE(pucIE) == VENDOR_OUI_TYPE_WMM) &&
			    (!kalMemCmp(WMM_IE_OUI(pucIE), aucWfaOui, 3))) {

				switch (WMM_IE_OUI_SUBTYPE(pucIE)) {
				case VENDOR_OUI_SUBTYPE_WMM_PARAM:
					if (IE_LEN(pucIE) != 24)
						break;	/* WMM Param IE with a wrong length */

					prStaRec->fgIsWmmSupported = TRUE;
					prStaRec->fgIsUapsdSupported =
					    (((((P_IE_WMM_PARAM_T) pucIE)->ucQosInfo) & WMM_QOS_INFO_UAPSD) ?
					     TRUE : FALSE);
					break;

				case VENDOR_OUI_SUBTYPE_WMM_INFO:
					if (IE_LEN(pucIE) != 7)
						break;	/* WMM Info IE with a wrong length */

					prStaRec->fgIsWmmSupported = TRUE;
					prStaRec->fgIsUapsdSupported =
					    (((((P_IE_WMM_INFO_T) pucIE)->ucQosInfo) & WMM_QOS_INFO_UAPSD) ?
					     TRUE : FALSE);
					break;

				default:
					/* A WMM QoS IE that doesn't matter. Ignore it. */
					break;
				}
			}
			/* else: VENDOR_OUI_TYPE_WPA, VENDOR_OUI_TYPE_WPS, ... (not cared) */

			break;

		default:
			/* A WMM IE that doesn't matter. Ignore it. */
			break;
		}
	}
	DBGLOG(QM, LOUD, "MQM: Scan Result Parsing (WMM=%d, UAPSD=%d)\n",
			  prStaRec->fgIsWmmSupported, prStaRec->fgIsUapsdSupported);

}

UINT_8 qmGetStaRecIdx(IN P_ADAPTER_T prAdapter, IN PUINT_8 pucEthDestAddr, IN ENUM_NETWORK_TYPE_INDEX_T eNetworkType)
{
	UINT_32 i;
	P_STA_RECORD_T prTempStaRec;

	prTempStaRec = NULL;

	ASSERT(prAdapter);

	/* 4 <1> DA = BMCAST */
	if (IS_BMCAST_MAC_ADDR(pucEthDestAddr))
		return STA_REC_INDEX_BMCAST;
	/* 4 <2> Check if an AP STA is present */
	for (i = 0; i < CFG_NUM_OF_STA_RECORD; i++) {
		prTempStaRec = &(prAdapter->arStaRec[i]);
		if ((prTempStaRec->ucNetTypeIndex == eNetworkType)
		    && (prTempStaRec->fgIsAp)
		    && (prTempStaRec->fgIsValid)) {
			return prTempStaRec->ucIndex;
		}
	}

	/* 4 <3> Not BMCAST, No AP --> Compare DA (i.e., to see whether this is a unicast frame to a client) */
	for (i = 0; i < CFG_NUM_OF_STA_RECORD; i++) {
		prTempStaRec = &(prAdapter->arStaRec[i]);
		if (prTempStaRec->fgIsValid) {
			if (EQUAL_MAC_ADDR(prTempStaRec->aucMacAddr, pucEthDestAddr))
				return prTempStaRec->ucIndex;
		}
	}

	/* 4 <4> No STA found, Not BMCAST --> Indicate NOT_FOUND to FW */
	return STA_REC_INDEX_NOT_FOUND;
}

UINT_32
mqmGenerateWmmInfoIEByParam(BOOLEAN fgSupportUAPSD,
			    UINT_8 ucBmpDeliveryAC, UINT_8 ucBmpTriggerAC, UINT_8 ucUapsdSp, UINT_8 *pOutBuf)
{
	P_IE_WMM_INFO_T prIeWmmInfo;
	UINT_32 ucUapsd[] = {
		WMM_QOS_INFO_BE_UAPSD,
		WMM_QOS_INFO_BK_UAPSD,
		WMM_QOS_INFO_VI_UAPSD,
		WMM_QOS_INFO_VO_UAPSD
	};
	UINT_8 aucWfaOui[] = VENDOR_OUI_WFA;

	ASSERT(pOutBuf);

	prIeWmmInfo = (P_IE_WMM_INFO_T) pOutBuf;

	prIeWmmInfo->ucId = ELEM_ID_WMM;
	prIeWmmInfo->ucLength = ELEM_MAX_LEN_WMM_INFO;

	/* WMM-2.2.1 WMM Information Element Field Values */
	prIeWmmInfo->aucOui[0] = aucWfaOui[0];
	prIeWmmInfo->aucOui[1] = aucWfaOui[1];
	prIeWmmInfo->aucOui[2] = aucWfaOui[2];
	prIeWmmInfo->ucOuiType = VENDOR_OUI_TYPE_WMM;
	prIeWmmInfo->ucOuiSubtype = VENDOR_OUI_SUBTYPE_WMM_INFO;

	prIeWmmInfo->ucVersion = VERSION_WMM;
	prIeWmmInfo->ucQosInfo = 0;

	/* UAPSD initial queue configurations (delivery and trigger enabled) */
	if (fgSupportUAPSD) {

		UINT_8 ucQosInfo = 0;
		UINT_8 i;

		/* Static U-APSD setting */
		for (i = ACI_BE; i <= ACI_VO; i++) {
			if (ucBmpDeliveryAC & ucBmpTriggerAC & BIT(i))
				ucQosInfo |= (UINT_8) ucUapsd[i];
		}

		if (ucBmpDeliveryAC & ucBmpTriggerAC) {
			switch (ucUapsdSp) {
			case WMM_MAX_SP_LENGTH_ALL:
				ucQosInfo |= WMM_QOS_INFO_MAX_SP_ALL;
				break;

			case WMM_MAX_SP_LENGTH_2:
				ucQosInfo |= WMM_QOS_INFO_MAX_SP_2;
				break;

			case WMM_MAX_SP_LENGTH_4:
				ucQosInfo |= WMM_QOS_INFO_MAX_SP_4;
				break;

			case WMM_MAX_SP_LENGTH_6:
				ucQosInfo |= WMM_QOS_INFO_MAX_SP_6;
				break;

			default:
				DBGLOG(QM, WARN, "MQM: Incorrect SP length\n");
				ucQosInfo |= WMM_QOS_INFO_MAX_SP_2;
				break;
			}
		}
		prIeWmmInfo->ucQosInfo = ucQosInfo;

	}

	/* Increment the total IE length for the Element ID and Length fields. */
	return IE_SIZE(prIeWmmInfo);
}

/*----------------------------------------------------------------------------*/
/*!
* @brief Generate the WMM Info IE
*
* \param[in] prAdapter  Adapter pointer
* @param prMsduInfo The TX MMPDU
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
VOID mqmGenerateWmmInfoIE(IN P_ADAPTER_T prAdapter, IN P_MSDU_INFO_T prMsduInfo)
{
	P_IE_WMM_INFO_T prIeWmmInfo;
	P_PM_PROFILE_SETUP_INFO_T prPmProfSetupInfo;
	P_BSS_INFO_T prBssInfo;
	P_STA_RECORD_T prStaRec;

	DEBUGFUNC("mqmGenerateWmmInfoIE");

	ASSERT(prMsduInfo);

	/* In case QoS is not turned off, exit directly */
	if (!prAdapter->rWifiVar.fgSupportQoS)
		return;

	prStaRec = cnmGetStaRecByIndex(prAdapter, prMsduInfo->ucStaRecIndex);
	ASSERT(prStaRec);

	if (prStaRec == NULL)
		return;

	if (!prStaRec->fgIsWmmSupported)
		return;

	prBssInfo = &(prAdapter->rWifiVar.arBssInfo[prStaRec->ucNetTypeIndex]);

	prPmProfSetupInfo = &prBssInfo->rPmProfSetupInfo;

	prIeWmmInfo = (P_IE_WMM_INFO_T)
	    ((PUINT_8) prMsduInfo->prPacket + prMsduInfo->u2FrameLength);

#if 0
	prIeWmmInfo->ucId = ELEM_ID_WMM;
	prIeWmmInfo->ucLength = ELEM_MAX_LEN_WMM_INFO;

	/* WMM-2.2.1 WMM Information Element Field Values */
	prIeWmmInfo->aucOui[0] = aucWfaOui[0];
	prIeWmmInfo->aucOui[1] = aucWfaOui[1];
	prIeWmmInfo->aucOui[2] = aucWfaOui[2];
	prIeWmmInfo->ucOuiType = VENDOR_OUI_TYPE_WMM;
	prIeWmmInfo->ucOuiSubtype = VENDOR_OUI_SUBTYPE_WMM_INFO;

	prIeWmmInfo->ucVersion = VERSION_WMM;
	prIeWmmInfo->ucQosInfo = 0;

	/* UAPSD initial queue configurations (delivery and trigger enabled) */
	/* if (prAdapter->rWifiVar.fgSupportUAPSD) { */
	if (prAdapter->rWifiVar.fgSupportUAPSD && prStaRec->fgIsUapsdSupported) {

		UINT_8 ucQosInfo = 0;
		UINT_8 i;

		/* Static U-APSD setting */
		for (i = ACI_BE; i <= ACI_VO; i++) {
			if (prPmProfSetupInfo->ucBmpDeliveryAC & prPmProfSetupInfo->ucBmpTriggerAC & BIT(i))
				ucQosInfo |= (UINT_8) ucUapsd[i];
		}

		if (prPmProfSetupInfo->ucBmpDeliveryAC & prPmProfSetupInfo->ucBmpTriggerAC) {
			switch (prPmProfSetupInfo->ucUapsdSp) {
			case WMM_MAX_SP_LENGTH_ALL:
				ucQosInfo |= WMM_QOS_INFO_MAX_SP_ALL;
				break;

			case WMM_MAX_SP_LENGTH_2:
				ucQosInfo |= WMM_QOS_INFO_MAX_SP_2;
				break;

			case WMM_MAX_SP_LENGTH_4:
				ucQosInfo |= WMM_QOS_INFO_MAX_SP_4;
				break;

			case WMM_MAX_SP_LENGTH_6:
				ucQosInfo |= WMM_QOS_INFO_MAX_SP_6;
				break;

			default:
				DBGLOG(QM, INFO, "MQM: Incorrect SP length\n");
				ucQosInfo |= WMM_QOS_INFO_MAX_SP_2;
				break;
			}
		}
		prIeWmmInfo->ucQosInfo = ucQosInfo;

	}

	/* Increment the total IE length for the Element ID and Length fields. */
	prMsduInfo->u2FrameLength += IE_SIZE(prIeWmmInfo);
#else

	prMsduInfo->u2FrameLength += mqmGenerateWmmInfoIEByParam((prAdapter->rWifiVar.fgSupportUAPSD
								  && prStaRec->fgIsUapsdSupported),
								 prPmProfSetupInfo->ucBmpDeliveryAC,
								 prPmProfSetupInfo->ucBmpTriggerAC,
								 prPmProfSetupInfo->ucUapsdSp, (UINT_8 *) prIeWmmInfo);
#endif
}

#if 0
/*----------------------------------------------------------------------------*/
/*!
* @brief log2 calculation for CW
*
* @param[in] val value
*
* @return log2(val)
*/
/*----------------------------------------------------------------------------*/

UINT_32 cwlog2(UINT_32 val)
{

	UINT_32 n;

	n = 0;

	while (val >= 512) {
		n += 9;
		val = val >> 9;
	}
	while (val >= 16) {
		n += 4;
		val >>= 4;
	}
	while (val >= 2) {
		n += 1;
		val >>= 1;
	}
	return n;
}
#endif

UINT_32 mqmGenerateWmmParamIEByParam(P_ADAPTER_T prAdapter,
			P_BSS_INFO_T prBssInfo, UINT_8 *pOutBuf, ENUM_OP_MODE_T ucOpMode)
{
	P_IE_WMM_PARAM_T prIeWmmParam;
	UINT_8 aucWfaOui[] = VENDOR_OUI_WFA;
	UINT_8 aucACI[] = {
		WMM_ACI_AC_BE,
		WMM_ACI_AC_BK,
		WMM_ACI_AC_VI,
		WMM_ACI_AC_VO
	};
	ENUM_WMM_ACI_T eAci;
	UCHAR *pucAciAifsn, *pucEcw, *pucTxopLimit;

	ASSERT(pOutBuf);

	prIeWmmParam = (P_IE_WMM_PARAM_T) pOutBuf;

	prIeWmmParam->ucId = ELEM_ID_WMM;
	prIeWmmParam->ucLength = ELEM_MAX_LEN_WMM_PARAM;

	/* WMM-2.2.1 WMM Information Element Field Values */
	prIeWmmParam->aucOui[0] = aucWfaOui[0];
	prIeWmmParam->aucOui[1] = aucWfaOui[1];
	prIeWmmParam->aucOui[2] = aucWfaOui[2];
	prIeWmmParam->ucOuiType = VENDOR_OUI_TYPE_WMM;
	prIeWmmParam->ucOuiSubtype = VENDOR_OUI_SUBTYPE_WMM_PARAM;

	prIeWmmParam->ucVersion = VERSION_WMM;
	prIeWmmParam->ucQosInfo = (prBssInfo->ucWmmParamSetCount & WMM_QOS_INFO_PARAM_SET_CNT);

	/* UAPSD initial queue configurations (delivery and trigger enabled) */
	if (prAdapter->rWifiVar.fgSupportUAPSD) {
		if (ucOpMode == OP_MODE_INFRASTRUCTURE)
			prIeWmmParam->ucQosInfo = 0xf;
		else
			prIeWmmParam->ucQosInfo |= WMM_QOS_INFO_UAPSD;
	}

	/* EDCA parameter */

	for (eAci = 0; eAci < WMM_AC_INDEX_NUM; eAci++) {

		/* DBGLOG(QM, LOUD, */
		/* ("MQM: eAci = %d, ACM = %d, Aifsn = %d, CWmin = %d, CWmax = %d, TxopLimit = %d\n", */
		/* eAci,prBssInfo->arACQueParmsForBcast[eAci].fgIsACMSet , */
		/* prBssInfo->arACQueParmsForBcast[eAci].u2Aifsn, */
		/* prBssInfo->arACQueParmsForBcast[eAci].u2CWmin, */
		/* prBssInfo->arACQueParmsForBcast[eAci].u2CWmax, */
		/* prBssInfo->arACQueParmsForBcast[eAci].u2TxopLimit)); */

#if 0
		*(((PUINT_8) (&prIeWmmParam->ucAciAifsn_BE)) +
			(eAci << 2)) = (UINT_8) (aucACI[eAci] |
			(prBssInfo->arACQueParmsForBcast[eAci].fgIsACMSet ?
			WMM_ACIAIFSN_ACM : 0) |
			(prBssInfo->arACQueParmsForBcast[eAci].u2Aifsn &
			(WMM_ACIAIFSN_AIFSN)));
#else
		/* avoid compile warnings in Klockwork tool */
		if (eAci == WMM_AC_BE_INDEX) {
			pucAciAifsn = &prIeWmmParam->ucAciAifsn_BE;
			pucEcw = &prIeWmmParam->ucEcw_BE;
			pucTxopLimit = prIeWmmParam->aucTxopLimit_BE;
		} else if (eAci == WMM_AC_BK_INDEX) {
			pucAciAifsn = &prIeWmmParam->ucAciAifsn_BG;
			pucEcw = &prIeWmmParam->ucEcw_BG;
			pucTxopLimit = prIeWmmParam->aucTxopLimit_BG;
		} else if (eAci == WMM_AC_VI_INDEX) {
			pucAciAifsn = &prIeWmmParam->ucAciAifsn_VI;
			pucEcw = &prIeWmmParam->ucEcw_VI;
			pucTxopLimit = prIeWmmParam->aucTxopLimit_VI;
		} else if (eAci == WMM_AC_VO_INDEX) {
			pucAciAifsn = &prIeWmmParam->ucAciAifsn_VO;
			pucEcw = &prIeWmmParam->ucEcw_VO;
			pucTxopLimit = prIeWmmParam->aucTxopLimit_VO;
		}

		*pucAciAifsn = (UINT_8) (aucACI[eAci]
					 | (prBssInfo->arACQueParmsForBcast[eAci].fgIsACMSet ? WMM_ACIAIFSN_ACM : 0)
					 | (prBssInfo->arACQueParmsForBcast[eAci].u2Aifsn & (WMM_ACIAIFSN_AIFSN)));
#endif

#if 1
/* *( ((PUINT_8)(&prIeWmmParam->ucEcw_BE)) + (eAci <<2) ) = (UINT_8) (0 */
		*pucEcw = (UINT_8) (0 | (((prBssInfo->aucCWminLog2ForBcast[eAci])) & WMM_ECW_WMIN_MASK)
				    | ((((prBssInfo->aucCWmaxLog2ForBcast[eAci])) << WMM_ECW_WMAX_OFFSET) &
				       WMM_ECW_WMAX_MASK)
		    );
#else
		*(((PUINT_8) (&prIeWmmParam->ucEcw_BE)) +
			(eAci << 2)) = (UINT_8) (0 |
			(cwlog2((prBssInfo->arACQueParmsForBcast[eAci].u2CWmin +
			1)) & WMM_ECW_WMIN_MASK) |
			((cwlog2
			((prBssInfo->arACQueParmsForBcast[eAci].u2CWmax +
			1)) << WMM_ECW_WMAX_OFFSET) &
			WMM_ECW_WMAX_MASK)
		    );
#endif

#if 0
		WLAN_SET_FIELD_16(((PUINT_8) (prIeWmmParam->aucTxopLimit_BE)) + (eAci << 2)
				  , prBssInfo->arACQueParmsForBcast[eAci].u2TxopLimit);
#else
		WLAN_SET_FIELD_16(pucTxopLimit, prBssInfo->arACQueParmsForBcast[eAci].u2TxopLimit);
#endif
	}

	/* Increment the total IE length for the Element ID and Length fields. */
	return IE_SIZE(prIeWmmParam);
}

/*----------------------------------------------------------------------------*/
/*!
* @brief Generate the WMM Param IE
*
* \param[in] prAdapter  Adapter pointer
* @param prMsduInfo The TX MMPDU
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
VOID mqmGenerateWmmParamIE(IN P_ADAPTER_T prAdapter, IN P_MSDU_INFO_T prMsduInfo)
{
	P_IE_WMM_PARAM_T prIeWmmParam;

#if 0
	UINT_8 aucWfaOui[] = VENDOR_OUI_WFA;

	UINT_8 aucACI[] = {
		WMM_ACI_AC_BE,
		WMM_ACI_AC_BK,
		WMM_ACI_AC_VI,
		WMM_ACI_AC_VO
	};
	ENUM_WMM_ACI_T eAci;
#endif

	P_BSS_INFO_T prBssInfo;
	P_STA_RECORD_T prStaRec;

	DEBUGFUNC("mqmGenerateWmmParamIE");
	DBGLOG(QM, LOUD, "\n");

	ASSERT(prMsduInfo);

	/* In case QoS is not turned off, exit directly */
	if (!prAdapter->rWifiVar.fgSupportQoS)
		return;

	prStaRec = cnmGetStaRecByIndex(prAdapter, prMsduInfo->ucStaRecIndex);

	if (prStaRec) {
		if (!prStaRec->fgIsQoS)
			return;
	}

	prBssInfo = &(prAdapter->rWifiVar.arBssInfo[prMsduInfo->ucNetworkType]);

	if (!prBssInfo->fgIsQBSS)
		return;
/* 20120220 frog: update beacon content & change OP mode is a separate event for P2P network. */
#if 0
	if (prBssInfo->eCurrentOPMode != OP_MODE_ACCESS_POINT && prBssInfo->eCurrentOPMode != OP_MODE_BOW)
		return;
#endif

	prIeWmmParam = (P_IE_WMM_PARAM_T)
	    ((PUINT_8) prMsduInfo->prPacket + prMsduInfo->u2FrameLength);

#if 0
	prIeWmmParam->ucId = ELEM_ID_WMM;
	prIeWmmParam->ucLength = ELEM_MAX_LEN_WMM_PARAM;

	/* WMM-2.2.1 WMM Information Element Field Values */
	prIeWmmParam->aucOui[0] = aucWfaOui[0];
	prIeWmmParam->aucOui[1] = aucWfaOui[1];
	prIeWmmParam->aucOui[2] = aucWfaOui[2];
	prIeWmmParam->ucOuiType = VENDOR_OUI_TYPE_WMM;
	prIeWmmParam->ucOuiSubtype = VENDOR_OUI_SUBTYPE_WMM_PARAM;

	prIeWmmParam->ucVersion = VERSION_WMM;
	prIeWmmParam->ucQosInfo = (prBssInfo->ucWmmParamSetCount & WMM_QOS_INFO_PARAM_SET_CNT);

	/* UAPSD initial queue configurations (delivery and trigger enabled) */
	if (prAdapter->rWifiVar.fgSupportUAPSD)
		prIeWmmParam->ucQosInfo |= WMM_QOS_INFO_UAPSD;

	/* EDCA parameter */

	for (eAci = 0; eAci < WMM_AC_INDEX_NUM; eAci++) {

		/* DBGLOG(QM, LOUD, */
		/* ("MQM: eAci = %d, ACM = %d, Aifsn = %d, CWmin = %d, CWmax = %d, TxopLimit = %d\n", */
		/* eAci,prBssInfo->arACQueParmsForBcast[eAci].fgIsACMSet , */
		/* prBssInfo->arACQueParmsForBcast[eAci].u2Aifsn, */
		/* prBssInfo->arACQueParmsForBcast[eAci].u2CWmin, */
		/* prBssInfo->arACQueParmsForBcast[eAci].u2CWmax, */
		/* prBssInfo->arACQueParmsForBcast[eAci].u2TxopLimit)); */

		*(((PUINT_8) (&prIeWmmParam->ucAciAifsn_BE)) +
			(eAci << 2)) = (UINT_8) (aucACI[eAci] |
			(prBssInfo->arACQueParmsForBcast[eAci].fgIsACMSet ?
			WMM_ACIAIFSN_ACM : 0) |
			(prBssInfo->arACQueParmsForBcast[eAci].u2Aifsn &
			(WMM_ACIAIFSN_AIFSN)));
#if 1
		*(((PUINT_8) (&prIeWmmParam->ucEcw_BE)) +
			(eAci << 2)) = (UINT_8) (0 |
			(((prBssInfo->aucCWminLog2ForBcast[eAci])) & WMM_ECW_WMIN_MASK) |
			((((prBssInfo->aucCWmaxLog2ForBcast[eAci])) << WMM_ECW_WMAX_OFFSET) &
			WMM_ECW_WMAX_MASK));
#else
		*(((PUINT_8) (&prIeWmmParam->ucEcw_BE)) +
			(eAci << 2)) = (UINT_8) (0 |
			(cwlog2((prBssInfo->arACQueParmsForBcast[eAci].u2CWmin +
			1)) & WMM_ECW_WMIN_MASK) |
			((cwlog2((prBssInfo->arACQueParmsForBcast[eAci].u2CWmax + 1))
				<< WMM_ECW_WMAX_OFFSET) & WMM_ECW_WMAX_MASK));
#endif

		WLAN_SET_FIELD_16(((PUINT_8) (prIeWmmParam->aucTxopLimit_BE)) + (eAci << 2)
				  , prBssInfo->arACQueParmsForBcast[eAci].u2TxopLimit);

	}

	/* Increment the total IE length for the Element ID and Length fields. */
	prMsduInfo->u2FrameLength += IE_SIZE(prIeWmmParam);
#else

	prMsduInfo->u2FrameLength += mqmGenerateWmmParamIEByParam(prAdapter,
				prBssInfo, (UINT_8 *) prIeWmmParam, OP_MODE_ACCESS_POINT);
#endif
}

ENUM_FRAME_ACTION_T
qmGetFrameAction(IN P_ADAPTER_T prAdapter,
		 IN ENUM_NETWORK_TYPE_INDEX_T eNetworkType,
		 IN UINT_8 ucStaRecIdx, IN P_MSDU_INFO_T prMsduInfo, IN ENUM_FRAME_TYPE_IN_CMD_Q_T eFrameType)
{
	P_BSS_INFO_T prBssInfo;
	P_STA_RECORD_T prStaRec;
	P_WLAN_MAC_HEADER_T prWlanFrame;
	UINT_16 u2TxFrameCtrl;

	DEBUGFUNC("qmGetFrameAction");

#if (NIC_TX_BUFF_COUNT_TC4 > 2)
#define QM_MGMT_QUUEUD_THRESHOLD 2
#else
#define QM_MGMT_QUUEUD_THRESHOLD 1
#endif

	DATA_STRUCT_INSPECTING_ASSERT(QM_MGMT_QUUEUD_THRESHOLD <= (NIC_TX_BUFF_COUNT_TC4));
	DATA_STRUCT_INSPECTING_ASSERT(QM_MGMT_QUUEUD_THRESHOLD > 0);

	prBssInfo = &(prAdapter->rWifiVar.arBssInfo[eNetworkType]);
	prStaRec = QM_GET_STA_REC_PTR_FROM_INDEX(prAdapter, ucStaRecIdx);

	/* XXX Check BOW P2P AIS time ot set active */
	if (!IS_BSS_ACTIVE(prBssInfo)) {
		if (eFrameType == FRAME_TYPE_MMPDU) {
			prWlanFrame = (P_WLAN_MAC_HEADER_T) prMsduInfo->prPacket;
			u2TxFrameCtrl = (prWlanFrame->u2FrameCtrl) & MASK_FRAME_TYPE;	/* Optimized for ARM */
			if (((u2TxFrameCtrl == MAC_FRAME_DEAUTH)
				&& (prMsduInfo->pfTxDoneHandler == NULL))
				|| (u2TxFrameCtrl == MAC_FRAME_ACTION))		/* whsu */
				return FRAME_ACTION_TX_PKT;
		}

		DBGLOG(QM, WARN, "Drop packets Action, eFrameType: %d (Bss Index %u).\n",
				eFrameType, prBssInfo->ucNetTypeIndex);
		TX_INC_CNT(&prAdapter->rTxCtrl, TX_INACTIVE_BSS_DROP);
		return FRAME_ACTION_DROP_PKT;
	}

	/* TODO Handle disconnect issue */

	/* P2P probe Request frame */
	do {
		if (eFrameType == FRAME_TYPE_MMPDU) {
			prWlanFrame = (P_WLAN_MAC_HEADER_T) prMsduInfo->prPacket;
			u2TxFrameCtrl = (prWlanFrame->u2FrameCtrl) & MASK_FRAME_TYPE;	/* Optimized for ARM */

			if (u2TxFrameCtrl == MAC_FRAME_BEACON) {
				if (prBssInfo->fgIsNetAbsent)
					return FRAME_ACTION_DROP_PKT;
			} else if (u2TxFrameCtrl == MAC_FRAME_PROBE_RSP) {
				if (prBssInfo->fgIsNetAbsent)
					return FRAME_ACTION_DROP_PKT;
			} else if (u2TxFrameCtrl == MAC_FRAME_DEAUTH) {
				if (prBssInfo->fgIsNetAbsent)
					break;
				DBGLOG(P2P, LOUD, "Sending DEAUTH Frame\n");
				return FRAME_ACTION_TX_PKT;
			}
			/* MMPDU with prStaRec && fgIsInUse not check fgIsNetActive */
			else if (u2TxFrameCtrl == MAC_FRAME_ASSOC_REQ
				 || u2TxFrameCtrl == MAC_FRAME_AUTH
				 || u2TxFrameCtrl == MAC_FRAME_REASSOC_REQ
				 || u2TxFrameCtrl == MAC_FRAME_PROBE_REQ || u2TxFrameCtrl == MAC_FRAME_ACTION) {

				if ((prStaRec) && (prStaRec->fgIsInPS)) {
					if (nicTxGetResource(prAdapter, TC4_INDEX) >= QM_MGMT_QUUEUD_THRESHOLD)
						return FRAME_ACTION_TX_PKT;
					else
						return FRAME_ACTION_QUEUE_PKT;
				}
				return FRAME_ACTION_TX_PKT;
			}

			if (!prStaRec)
				return FRAME_ACTION_TX_PKT;

			if (!prStaRec->fgIsInUse)
				return FRAME_ACTION_DROP_PKT;

		/* FRAME_TYPE_MMPDU */
		} else if (eFrameType == FRAME_TYPE_802_1X) {

			if (!prStaRec)
				return FRAME_ACTION_TX_PKT;

			if (!prStaRec->fgIsInUse)
				return FRAME_ACTION_DROP_PKT;
			if (prStaRec->fgIsInPS) {
				if (nicTxGetResource(prAdapter, TC4_INDEX) >= QM_MGMT_QUUEUD_THRESHOLD)
					return FRAME_ACTION_TX_PKT;
				else
					return FRAME_ACTION_QUEUE_PKT;
			}

		/* FRAME_TYPE_802_1X */
		} else if ((!IS_BSS_ACTIVE(prBssInfo))
			 || (!prStaRec)
			 || (!prStaRec->fgIsInUse)) {
			return FRAME_ACTION_DROP_PKT;
		}
	} while (0);

	if (prBssInfo->fgIsNetAbsent) {
		DBGLOG(QM, LOUD, "Queue packets (Absent %u).\n", prBssInfo->ucNetTypeIndex);
		return FRAME_ACTION_QUEUE_PKT;
	}

	if (prStaRec && prStaRec->fgIsInPS) {
		DBGLOG(QM, LOUD, "Queue packets (PS %u).\n", prStaRec->fgIsInPS);
		return FRAME_ACTION_QUEUE_PKT;
	}
	switch (eFrameType) {
	case FRAME_TYPE_802_1X:
		if (!prStaRec->fgIsValid)
			return FRAME_ACTION_QUEUE_PKT;
		break;

	case FRAME_TYPE_MMPDU:
		break;

	default:
		ASSERT(0);
	}

	return FRAME_ACTION_TX_PKT;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief Handle BSS change operation Event from the FW
*
* \param[in] prAdapter Adapter pointer
* \param[in] prEvent The event packet from the FW
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/
VOID qmHandleEventBssAbsencePresence(IN P_ADAPTER_T prAdapter, IN P_WIFI_EVENT_T prEvent)
{
	P_EVENT_BSS_ABSENCE_PRESENCE_T prEventBssStatus;
	P_BSS_INFO_T prBssInfo;
	BOOLEAN fgIsNetAbsentOld;

	prEventBssStatus = (P_EVENT_BSS_ABSENCE_PRESENCE_T) prEvent;
	prBssInfo = &(prAdapter->rWifiVar.arBssInfo[prEventBssStatus->ucNetTypeIdx]);
	fgIsNetAbsentOld = prBssInfo->fgIsNetAbsent;
	prBssInfo->fgIsNetAbsent = prEventBssStatus->fgIsAbsent;
	prBssInfo->ucBssFreeQuota = prEventBssStatus->ucBssFreeQuota;

	/* DBGLOG(QM, TRACE, ("qmHandleEventBssAbsencePresence (ucNetTypeIdx=%d, fgIsAbsent=%d, FreeQuota=%d)\n", */
	/* prEventBssStatus->ucNetTypeIdx, prBssInfo->fgIsNetAbsent, prBssInfo->ucBssFreeQuota)); */

	DBGLOG(QM, INFO, "NAF=%d,%d,%d\n",
			   prEventBssStatus->ucNetTypeIdx, prBssInfo->fgIsNetAbsent, prBssInfo->ucBssFreeQuota);

	if (!prBssInfo->fgIsNetAbsent) {
		/* QM_DBG_CNT_27 */
		QM_DBG_CNT_INC(&(prAdapter->rQM), QM_DBG_CNT_27);
	} else {
		/* QM_DBG_CNT_28 */
		QM_DBG_CNT_INC(&(prAdapter->rQM), QM_DBG_CNT_28);
	}
	/* From Absent to Present */
	if ((fgIsNetAbsentOld) && (!prBssInfo->fgIsNetAbsent))
		kalSetEvent(prAdapter->prGlueInfo);
}

/*----------------------------------------------------------------------------*/
/*!
* \brief Handle STA change PS mode Event from the FW
*
* \param[in] prAdapter Adapter pointer
* \param[in] prEvent The event packet from the FW
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/
VOID qmHandleEventStaChangePsMode(IN P_ADAPTER_T prAdapter, IN P_WIFI_EVENT_T prEvent)
{
	P_EVENT_STA_CHANGE_PS_MODE_T prEventStaChangePsMode;
	P_STA_RECORD_T prStaRec;
	BOOLEAN fgIsInPSOld;

	/* DbgPrint("QM:Event -RxBa\n"); */

	prEventStaChangePsMode = (P_EVENT_STA_CHANGE_PS_MODE_T) prEvent;
	prStaRec = QM_GET_STA_REC_PTR_FROM_INDEX(prAdapter, prEventStaChangePsMode->ucStaRecIdx);
	ASSERT(prStaRec);

	if (prStaRec) {

		fgIsInPSOld = prStaRec->fgIsInPS;
		prStaRec->fgIsInPS = prEventStaChangePsMode->fgIsInPs;

		qmUpdateFreeQuota(prAdapter,
				  prStaRec,
				  prEventStaChangePsMode->ucUpdateMode, prEventStaChangePsMode->ucFreeQuota, 0);

		/* DBGLOG(QM, TRACE, ("qmHandleEventStaChangePsMode (ucStaRecIdx=%d, fgIsInPs=%d)\n", */
		/* prEventStaChangePsMode->ucStaRecIdx, prStaRec->fgIsInPS)); */

		DBGLOG(QM, TRACE, "PS=%d,%d\n", prEventStaChangePsMode->ucStaRecIdx, prStaRec->fgIsInPS);

		/* From PS to Awake */
		if ((fgIsInPSOld) && (!prStaRec->fgIsInPS))
			kalSetEvent(prAdapter->prGlueInfo);
	}
}

/*----------------------------------------------------------------------------*/
/*!
* \brief Update STA free quota Event from FW
*
* \param[in] prAdapter Adapter pointer
* \param[in] prEvent The event packet from the FW
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/
VOID qmHandleEventStaUpdateFreeQuota(IN P_ADAPTER_T prAdapter, IN P_WIFI_EVENT_T prEvent)
{
	P_EVENT_STA_UPDATE_FREE_QUOTA_T prEventStaUpdateFreeQuota;
	P_STA_RECORD_T prStaRec;

	prEventStaUpdateFreeQuota = (P_EVENT_STA_UPDATE_FREE_QUOTA_T) prEvent;
	prStaRec = QM_GET_STA_REC_PTR_FROM_INDEX(prAdapter, prEventStaUpdateFreeQuota->ucStaRecIdx);
	ASSERT(prStaRec);

	if (prStaRec) {
		if (prStaRec->fgIsInPS) {
			qmUpdateFreeQuota(prAdapter,
					  prStaRec,
					  prEventStaUpdateFreeQuota->ucUpdateMode,
					  prEventStaUpdateFreeQuota->ucFreeQuota,
					  prEventStaUpdateFreeQuota->aucReserved[0]);

			kalSetEvent(prAdapter->prGlueInfo);
		}
#if 0
		DBGLOG(QM, TRACE,
		       "qmHandleEventStaUpdateFreeQuota (ucStaRecIdx=%d, ucUpdateMode=%d, ucFreeQuota=%d)\n",
			prEventStaUpdateFreeQuota->ucStaRecIdx, prEventStaUpdateFreeQuota->ucUpdateMode,
			prEventStaUpdateFreeQuota->ucFreeQuota);
#endif

		DBGLOG(QM, TRACE, "UFQ=%d,%d,%d\n",
				   prEventStaUpdateFreeQuota->ucStaRecIdx,
				   prEventStaUpdateFreeQuota->ucUpdateMode, prEventStaUpdateFreeQuota->ucFreeQuota);

	}

}

/*----------------------------------------------------------------------------*/
/*!
* \brief Update STA free quota
*
* \param[in] prStaRec the STA
* \param[in] ucUpdateMode the method to update free quota
* \param[in] ucFreeQuota  the value for update
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/
VOID
qmUpdateFreeQuota(IN P_ADAPTER_T prAdapter,
		  IN P_STA_RECORD_T prStaRec, IN UINT_8 ucUpdateMode, IN UINT_8 ucFreeQuota, IN UINT_8 ucNumOfTxDone)
{

	UINT_8 ucFreeQuotaForNonDelivery;
	UINT_8 ucFreeQuotaForDelivery;
	BOOLEAN flgIsUpdateForcedToDelivery;

	ASSERT(prStaRec);
	DBGLOG(QM, LOUD, "qmUpdateFreeQuota orig ucFreeQuota=%d Mode %u New %u\n",
			  prStaRec->ucFreeQuota, ucUpdateMode, ucFreeQuota);

	if (!prStaRec->fgIsInPS)
		return;

	flgIsUpdateForcedToDelivery = FALSE;

	if (ucNumOfTxDone > 0) {
		/*
		 * update free quota by
		 * num of tx done + resident free quota (delivery + non-delivery)
		 */
		UINT_8 ucAvailQuota;

		ucAvailQuota = ucNumOfTxDone + prStaRec->ucFreeQuotaForDelivery + prStaRec->ucFreeQuotaForNonDelivery;
		if (ucAvailQuota > ucFreeQuota)	/* sanity check */
			ucAvailQuota = ucFreeQuota;

		/* update current free quota */
		ucFreeQuota = ucAvailQuota;

		/* check if the update is from last packet */
		if (ucFreeQuota == (prStaRec->ucFreeQuota + 1)) {
			/* just add the extra quota to delivery queue */

			/*
			 * EX:
			 * 1. TDLS peer enters power save
			 * 2. When the last 2 VI packets are tx done, we will receive 2 update events
			 * 3. 1st update event: ucFreeQuota = 9
			 * 4. We will correct new quota for delivey and non-delivery to 7:2
			 * 5. 2rd update event: ucFreeQuota = 10
			 * 6. We will re-correct new quota for delivery and non-delivery to 5:5
			 *
			 * But non-delivery queue is not busy.
			 * So in the case, we will have wrong decision, i.e. higher queue always quota 5
			 *
			 * Solution: skip the 2rd update event and just add the extra quota to delivery.
			 */

			flgIsUpdateForcedToDelivery = TRUE;
		}
	}

	switch (ucUpdateMode) {
	case FREE_QUOTA_UPDATE_MODE_INIT:
	case FREE_QUOTA_UPDATE_MODE_OVERWRITE:
		prStaRec->ucFreeQuota = ucFreeQuota;
		break;
	case FREE_QUOTA_UPDATE_MODE_INCREASE:
		prStaRec->ucFreeQuota += ucFreeQuota;
		break;
	case FREE_QUOTA_UPDATE_MODE_DECREASE:
		prStaRec->ucFreeQuota -= ucFreeQuota;
		break;
	default:
		ASSERT(0);
	}

	DBGLOG(QM, LOUD, "qmUpdateFreeQuota new ucFreeQuota=%d)\n", prStaRec->ucFreeQuota);

	ucFreeQuota = prStaRec->ucFreeQuota;

	ucFreeQuotaForNonDelivery = 0;
	ucFreeQuotaForDelivery = 0;

	if (ucFreeQuota > 0) {
		if (prStaRec->fgIsQoS && prStaRec->fgIsUapsdSupported
		    /*
		     * && prAdapter->rWifiVar.fgSupportQoS
		     * && prAdapter->rWifiVar.fgSupportUAPSD
		     */) {
			/* XXX We should assign quota to aucFreeQuotaPerQueue[NUM_OF_PER_STA_TX_QUEUES]  */

			if (flgIsUpdateForcedToDelivery == FALSE) {
				if (prStaRec->ucFreeQuotaForNonDelivery > 0 && prStaRec->ucFreeQuotaForDelivery > 0) {
					ucFreeQuotaForNonDelivery = ucFreeQuota >> 1;
					ucFreeQuotaForDelivery = ucFreeQuota - ucFreeQuotaForNonDelivery;
				} else if (prStaRec->ucFreeQuotaForNonDelivery == 0
					   && prStaRec->ucFreeQuotaForDelivery == 0) {
					ucFreeQuotaForNonDelivery = ucFreeQuota >> 1;
					ucFreeQuotaForDelivery = ucFreeQuota - ucFreeQuotaForNonDelivery;
				} else if (prStaRec->ucFreeQuotaForNonDelivery > 0) {
					/* NonDelivery is not busy */
					if (ucFreeQuota >= 3) {
						ucFreeQuotaForNonDelivery = 2;
						ucFreeQuotaForDelivery = ucFreeQuota - ucFreeQuotaForNonDelivery;
					} else {
						ucFreeQuotaForDelivery = ucFreeQuota;
						ucFreeQuotaForNonDelivery = 0;
					}
				} else if (prStaRec->ucFreeQuotaForDelivery > 0) {
					/* Delivery is not busy */
					if (ucFreeQuota >= 3) {
						ucFreeQuotaForDelivery = 2;
						ucFreeQuotaForNonDelivery = ucFreeQuota - ucFreeQuotaForDelivery;
					} else {
						ucFreeQuotaForNonDelivery = ucFreeQuota;
						ucFreeQuotaForDelivery = 0;
					}
				}
			} else {
				ucFreeQuotaForNonDelivery = 2;
				ucFreeQuotaForDelivery = ucFreeQuota - ucFreeQuotaForNonDelivery;
			}
		} else {
			/* no use ? */
			/* !prStaRec->fgIsUapsdSupported */
			ucFreeQuotaForNonDelivery = ucFreeQuota;
			ucFreeQuotaForDelivery = 0;
		}
	}
	/* ucFreeQuota > 0 */
	prStaRec->ucFreeQuotaForDelivery = ucFreeQuotaForDelivery;
	prStaRec->ucFreeQuotaForNonDelivery = ucFreeQuotaForNonDelivery;

#if (CFG_SUPPORT_TDLS_DBG == 1)
	if (IS_TDLS_STA(prStaRec))
		DBGLOG(QM, LOUD, "<tx> quota %d %d %d\n",
				ucFreeQuota, ucFreeQuotaForDelivery, ucFreeQuotaForNonDelivery);
#endif

	DBGLOG(QM, LOUD, "new QuotaForDelivery = %d  QuotaForNonDelivery = %d\n",
			  prStaRec->ucFreeQuotaForDelivery, prStaRec->ucFreeQuotaForNonDelivery);

}

/*----------------------------------------------------------------------------*/
/*!
* \brief Return the reorder queued RX packets
*
* \param[in] (none)
*
* \return The number of queued RX packets
*/
/*----------------------------------------------------------------------------*/
UINT_32 qmGetRxReorderQueuedBufferCount(IN P_ADAPTER_T prAdapter)
{
	UINT_32 i, u4Total;
	P_QUE_MGT_T prQM = &prAdapter->rQM;

	u4Total = 0;
	/* XXX The summation may impact the performance */
	for (i = 0; i < CFG_NUM_OF_RX_BA_AGREEMENTS; i++) {
		u4Total += prQM->arRxBaTable[i].rReOrderQue.u4NumElem;
#if DBG && 0
		if (QUEUE_IS_EMPTY(&(prQM->arRxBaTable[i].rReOrderQue)))
			ASSERT(prQM->arRxBaTable[i].rReOrderQue == 0);
#endif
	}
	ASSERT(u4Total <= (CFG_NUM_OF_QM_RX_PKT_NUM * 2));
	return u4Total;
}

#if ARP_MONITER_ENABLE
VOID qmDetectArpNoResponse(P_ADAPTER_T prAdapter, P_MSDU_INFO_T prMsduInfo)
{
	struct sk_buff *prSkb = NULL;
	PUINT_8 pucData = NULL;
	UINT_16 u2EtherType = 0;
	int arpOpCode = 0;

	prSkb = (struct sk_buff *)prMsduInfo->prPacket;

	if (!prSkb || (prSkb->len <= ETHER_HEADER_LEN))
		return;

	pucData = prSkb->data;
	if (!pucData)
		return;
	u2EtherType = (pucData[ETH_TYPE_LEN_OFFSET] << 8) | (pucData[ETH_TYPE_LEN_OFFSET + 1]);

	if (u2EtherType != ETH_P_ARP || (apIp[0] | apIp[1] | apIp[2] | apIp[3]) == 0)
		return;

	if (kalMemCmp(apIp, &pucData[ETH_TYPE_LEN_OFFSET + 26], sizeof(apIp))) /* dest ip address */
		return;

	arpOpCode = (pucData[ETH_TYPE_LEN_OFFSET + 8] << 8) | (pucData[ETH_TYPE_LEN_OFFSET + 8 + 1]);
	if (arpOpCode == ARP_PRO_REQ) {
		arpMoniter++;
		if (arpMoniter > 20) {
			DBGLOG(INIT, WARN, "IOT Critical issue, arp no resp, check AP!\n");
			aisBssBeaconTimeout(prAdapter);
			arpMoniter = 0;
			kalMemZero(apIp, sizeof(apIp));
		}
	}
}

VOID qmHandleRxArpPackets(P_ADAPTER_T prAdapter, P_SW_RFB_T prSwRfb)
{
	PUINT_8 pucData = NULL;
	UINT_16 u2EtherType = 0;
	int arpOpCode = 0;
	P_BSS_INFO_T prBssInfo = NULL;

	if (prSwRfb->u2PacketLen <= ETHER_HEADER_LEN)
		return;

	pucData = (PUINT_8)prSwRfb->pvHeader;
	if (!pucData)
		return;
	u2EtherType = (pucData[ETH_TYPE_LEN_OFFSET] << 8) | (pucData[ETH_TYPE_LEN_OFFSET + 1]);

	if (u2EtherType != ETH_P_ARP)
		return;

	arpOpCode = (pucData[ETH_TYPE_LEN_OFFSET + 8] << 8) | (pucData[ETH_TYPE_LEN_OFFSET + 8 + 1]);
	prBssInfo = &(prAdapter->rWifiVar.arBssInfo[NETWORK_TYPE_AIS_INDEX]);
	if (arpOpCode == ARP_PRO_RSP) {
		arpMoniter = 0;
		if (prBssInfo && prBssInfo->prStaRecOfAP && prBssInfo->prStaRecOfAP->aucMacAddr) {
			if (EQUAL_MAC_ADDR(&(pucData[ETH_TYPE_LEN_OFFSET + 10]), /* source hardware address */
					prBssInfo->prStaRecOfAP->aucMacAddr)) {
				kalMemCopy(apIp, &(pucData[ETH_TYPE_LEN_OFFSET + 16]), sizeof(apIp));
				DBGLOG(INIT, TRACE, "get arp response from AP %d.%d.%d.%d\n",
					apIp[0], apIp[1], apIp[2], apIp[3]);
			}
		}
	}
}

VOID qmResetArpDetect(VOID)
{
	arpMoniter = 0;
	kalMemZero(apIp, sizeof(apIp));
}
#endif


VOID qmHandleReorderBubbleTimeout(IN P_ADAPTER_T prAdapter, IN ULONG ulParamPtr)
{
	P_RX_BA_ENTRY_T prReorderQueParm = (P_RX_BA_ENTRY_T) ulParamPtr;
	P_SW_RFB_T prSwRfb = (P_SW_RFB_T) NULL;
	P_EVENT_CHECK_REORDER_BUBBLE_T prCheckReorderEvent;

	if (!prReorderQueParm->fgIsValid) {
		DBGLOG(QM, TRACE, "QM:Bub Check Cancel STA[%u] TID[%u], No Rx BA entry\n",
				   prReorderQueParm->ucStaRecIdx, prReorderQueParm->ucTid);
		return;
	}

	if (!prReorderQueParm->fgHasBubble) {
		DBGLOG(QM, TRACE,
		       "QM:Bub Check Cancel STA[%u] TID[%u], Bubble has been filled\n",
			prReorderQueParm->ucStaRecIdx, prReorderQueParm->ucTid);
		return;
	}

	DBGLOG(QM, TRACE, "QM:Bub Timeout STA[%u] TID[%u] BubSN[%u]\n",
			   prReorderQueParm->ucStaRecIdx, prReorderQueParm->ucTid, prReorderQueParm->u2FirstBubbleSn);

	/* Generate a self-inited event to Rx path */
	QUEUE_REMOVE_HEAD(&prAdapter->rRxCtrl.rFreeSwRfbList, prSwRfb, P_SW_RFB_T);

	if (prSwRfb) {
		prCheckReorderEvent = (P_EVENT_CHECK_REORDER_BUBBLE_T) prSwRfb->pucRecvBuff;

		prSwRfb->ucPacketType = HIF_RX_PKT_TYPE_EVENT;

		prCheckReorderEvent->ucEID = EVENT_ID_CHECK_REORDER_BUBBLE;
		prCheckReorderEvent->ucSeqNum = 0;

		prCheckReorderEvent->ucStaRecIdx = prReorderQueParm->ucStaRecIdx;
		prCheckReorderEvent->ucTid = prReorderQueParm->ucTid;
		prCheckReorderEvent->u2Length = sizeof(EVENT_CHECK_REORDER_BUBBLE_T);

		QUEUE_INSERT_TAIL(&prAdapter->rRxCtrl.rReceivedRfbList, &prSwRfb->rQueEntry);
		RX_INC_CNT(&prAdapter->rRxCtrl, RX_MPDU_TOTAL_COUNT);

		DBGLOG(QM, LOUD, "QM:Bub Check Event Sent STA[%u] TID[%u]\n",
				  prReorderQueParm->ucStaRecIdx, prReorderQueParm->ucTid);

		nicRxProcessRFBs(prAdapter);

		DBGLOG(QM, LOUD, "QM:Bub Check Event Handled STA[%u] TID[%u]\n",
				  prReorderQueParm->ucStaRecIdx, prReorderQueParm->ucTid);
	} else {
		DBGLOG(QM, TRACE,
		       "QM:Bub Check Cancel STA[%u] TID[%u], Bub check event alloc failed\n",
			prReorderQueParm->ucStaRecIdx, prReorderQueParm->ucTid);

		cnmTimerStartTimer(prAdapter, &(prReorderQueParm->rReorderBubbleTimer), QM_RX_BA_ENTRY_MISS_TIMEOUT_MS);

		DBGLOG(QM, TRACE, "QM:Bub Timer Restart STA[%u] TID[%u] BubSN[%u] Win{%d, %d}\n",
				   prReorderQueParm->ucStaRecIdx,
				   prReorderQueParm->ucTid,
				   prReorderQueParm->u2FirstBubbleSn,
				   prReorderQueParm->u2WinStart, prReorderQueParm->u2WinEnd);
	}
}

VOID qmHandleEventCheckReorderBubble(IN P_ADAPTER_T prAdapter, IN P_WIFI_EVENT_T prEvent)
{
	P_EVENT_CHECK_REORDER_BUBBLE_T prCheckReorderEvent = (P_EVENT_CHECK_REORDER_BUBBLE_T) prEvent;
	P_RX_BA_ENTRY_T prReorderQueParm;
	P_QUE_T prReorderQue;
	QUE_T rReturnedQue;
	P_QUE_T prReturnedQue = &rReturnedQue;
	P_SW_RFB_T prReorderedSwRfb, prSwRfb;

	QUEUE_INITIALIZE(prReturnedQue);

	/* Get target Rx BA entry */
	prReorderQueParm = qmLookupRxBaEntry(prAdapter, prCheckReorderEvent->ucStaRecIdx, prCheckReorderEvent->ucTid);

	/* Sanity Check */
	if (!prReorderQueParm || !prReorderQueParm->fgIsValid || !prReorderQueParm->fgHasBubble) {
		if (prReorderQueParm) {
			DBGLOG(QM, WARN, "QM:Bub Check Cancel STA[%u] TID[%u]. QueParm %p valid %d has bubble %d\n",
				   prCheckReorderEvent->ucStaRecIdx, prCheckReorderEvent->ucTid, prReorderQueParm,
				   prReorderQueParm->fgIsValid, prReorderQueParm->fgHasBubble);
		} else {
			DBGLOG(QM, WARN, "QM:Bub Check Cancel STA[%u] TID[%u].\n",
				   prCheckReorderEvent->ucStaRecIdx, prCheckReorderEvent->ucTid);
		}
		return;
	}

	prReorderQue = &(prReorderQueParm->rReOrderQue);

	if (QUEUE_IS_EMPTY(prReorderQue)) {
		prReorderQueParm->fgHasBubble = FALSE;

		DBGLOG(QM, WARN,
		       "QM:Bub Check Cancel STA[%u] TID[%u], Bubble has been filled\n",
			prReorderQueParm->ucStaRecIdx, prReorderQueParm->ucTid);

		return;
	}

	DBGLOG(QM, TRACE, "QM:Bub Check Event Got STA[%u] TID[%u]\n",
			   prReorderQueParm->ucStaRecIdx, prReorderQueParm->ucTid);

	/* Expected bubble timeout => pop out packets before win_end */
	if (prReorderQueParm->u2FirstBubbleSn == prReorderQueParm->u2WinStart) {

		prReorderedSwRfb = (P_SW_RFB_T) QUEUE_GET_TAIL(prReorderQue);

		prReorderQueParm->u2WinStart = prReorderedSwRfb->u2SSN + 1;
		prReorderQueParm->u2WinEnd =
		    ((prReorderQueParm->u2WinStart) + (prReorderQueParm->u2WinSize) - 1) % MAX_SEQ_NO_COUNT;

		qmPopOutDueToFallAhead(prAdapter, prReorderQueParm, prReturnedQue);

		DBGLOG(QM, TRACE, "QM:Bub Flush) STA[%u] TID[%u] BubSN[%u] Win{%d, %d}\n",
				   prReorderQueParm->ucStaRecIdx,
				   prReorderQueParm->ucTid,
				   prReorderQueParm->u2FirstBubbleSn,
				   prReorderQueParm->u2WinStart, prReorderQueParm->u2WinEnd);

		if (QUEUE_IS_NOT_EMPTY(prReturnedQue)) {
			QM_TX_SET_NEXT_MSDU_INFO((P_SW_RFB_T) QUEUE_GET_TAIL(prReturnedQue), NULL);

			prSwRfb = (P_SW_RFB_T) QUEUE_GET_HEAD(prReturnedQue);
			while (prSwRfb) {
				DBGLOG(QM, TRACE,
				       "QM:Bub Flush STA[%u] TID[%u] Pop Out SN[%u]\n",
					prReorderQueParm->ucStaRecIdx, prReorderQueParm->ucTid, prSwRfb->u2SSN);

				prSwRfb = (P_SW_RFB_T) QUEUE_GET_NEXT_ENTRY((P_QUE_ENTRY_T) prSwRfb);
			}

			wlanProcessQueuedSwRfb(prAdapter, (P_SW_RFB_T) QUEUE_GET_HEAD(prReturnedQue));
		} else {
			DBGLOG(QM, TRACE, "QM:Bub Flush STA[%u] TID[%u] Pop Out 0 packet\n",
					   prReorderQueParm->ucStaRecIdx, prReorderQueParm->ucTid);
		}

		prReorderQueParm->fgHasBubble = FALSE;
	}
	/* First bubble has been filled but others exist */
	else {
		prReorderQueParm->u2FirstBubbleSn = prReorderQueParm->u2WinStart;
		cnmTimerStartTimer(prAdapter, &(prReorderQueParm->rReorderBubbleTimer), QM_RX_BA_ENTRY_MISS_TIMEOUT_MS);

		DBGLOG(QM, TRACE, "QM:Bub Timer STA[%u] TID[%u] BubSN[%u] Win{%d, %d}\n",
				   prReorderQueParm->ucStaRecIdx,
				   prReorderQueParm->ucTid,
				   prReorderQueParm->u2FirstBubbleSn,
				   prReorderQueParm->u2WinStart, prReorderQueParm->u2WinEnd);
	}

	qmHandleMissTimeout(prReorderQueParm);

}

VOID qmHandleMissTimeout(IN P_RX_BA_ENTRY_T prReorderQueParm)
{
	P_QUE_T prReorderQue;
	OS_SYSTIME *prMissTimeout;

	prReorderQue = &(prReorderQueParm->rReOrderQue);

	prMissTimeout = &g_arMissTimeout[prReorderQueParm->ucStaRecIdx][prReorderQueParm->ucTid];
	if (QUEUE_IS_EMPTY(prReorderQue)) {
		DBGLOG(QM, TRACE, "QM:(Bub Check) Reset prMissTimeout to zero\n");
		*prMissTimeout = 0;
	} else {
		DBGLOG(QM, TRACE, "QM:(Bub Check) Reset prMissTimeout to current time\n");
		GET_CURRENT_SYSTIME(prMissTimeout);
	}
}

