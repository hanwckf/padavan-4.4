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

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/

/*******************************************************************************
*                   F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
static VOID cnmStaRecHandleEventPkt(P_ADAPTER_T prAdapter, P_CMD_INFO_T prCmdInfo, PUINT_8 pucEventBuf);

static VOID cnmStaSendUpdateCmd(P_ADAPTER_T prAdapter, P_STA_RECORD_T prStaRec, BOOLEAN fgNeedResp);

static VOID cnmStaSendRemoveCmd(P_ADAPTER_T prAdapter, P_STA_RECORD_T prStaRec);

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

/*----------------------------------------------------------------------------*/
/*!
* \brief
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
P_MSDU_INFO_T cnmMgtPktAlloc(P_ADAPTER_T prAdapter, UINT_32 u4Length)
{
	P_MSDU_INFO_T prMsduInfo;
	P_QUE_T prQueList;

	KAL_SPIN_LOCK_DECLARATION();

	ASSERT(prAdapter);
	prQueList = &prAdapter->rTxCtrl.rFreeMsduInfoList;

	/* Get a free MSDU_INFO_T */
	KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_MSDU_INFO_LIST);
	QUEUE_REMOVE_HEAD(prQueList, prMsduInfo, P_MSDU_INFO_T);
	KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_MSDU_INFO_LIST);

	if (prMsduInfo) {
		prMsduInfo->prPacket = cnmMemAlloc(prAdapter, RAM_TYPE_BUF, u4Length);
		prMsduInfo->eSrc = TX_PACKET_MGMT;

		if (prMsduInfo->prPacket == NULL) {
			KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_MSDU_INFO_LIST);
			QUEUE_INSERT_TAIL(prQueList, &prMsduInfo->rQueEntry);
			KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_MSDU_INFO_LIST);
			prMsduInfo = NULL;
		}
		if (prMsduInfo) {
			prMsduInfo->eCmdType = COMMAND_TYPE_NUM;
			prMsduInfo->ucCID = 0xff;
			prMsduInfo->u4InqueTime = 0;
			prMsduInfo->ucPacketType = TX_PACKET_NUM;
		}
	} else {
		P_QUE_T prTxingQue;
		P_QUE_ENTRY_T prQueueEntry = (P_QUE_ENTRY_T) NULL;
		P_MSDU_INFO_T prMsduInfo = (P_MSDU_INFO_T) NULL;
		P_TX_TCQ_STATUS_T pTc = (P_TX_TCQ_STATUS_T) NULL;

		prTxingQue = &(prAdapter->rTxCtrl.rTxMgmtTxingQueue);
		pTc = &(prAdapter->rTxCtrl.rTc);

		DBGLOG(MEM, LOUD, "++dump TxPendingMsdu=%u, Tc0=%d Tc1=%d Tc2=%d Tc3=%d, Tc4=%d Tc5=%d\n",
				   prTxingQue->u4NumElem, pTc->aucFreeBufferCount[TC0_INDEX],
				   pTc->aucFreeBufferCount[TC1_INDEX], pTc->aucFreeBufferCount[TC2_INDEX],
				   pTc->aucFreeBufferCount[TC3_INDEX], pTc->aucFreeBufferCount[TC4_INDEX],
				   pTc->aucFreeBufferCount[TC5_INDEX]);

		prQueueEntry = QUEUE_GET_HEAD(prTxingQue);

		while (prQueueEntry) {
			prMsduInfo = (P_MSDU_INFO_T) prQueueEntry;

			DBGLOG(MEM, LOUD,
			       "msdu type=%u, ucid=%u, type=%d, time=%u, seq=%u, sta=%u\n",
				prMsduInfo->ucPacketType,
				prMsduInfo->ucCID,
				prMsduInfo->eCmdType,
				prMsduInfo->u4InqueTime, prMsduInfo->ucTxSeqNum, prMsduInfo->ucStaRecIndex);
			prQueueEntry = QUEUE_GET_NEXT_ENTRY(prQueueEntry);
		}
		DBGLOG(MEM, LOUD, "--end dump\n");
	}

#if DBG
	if (prMsduInfo == NULL) {
		DBGLOG(MEM, WARN, "MgtDesc#=%u\n", prQueList->u4NumElem);

#if CFG_DBG_MGT_BUF
		DBGLOG(MEM, WARN, "rMgtBufInfo: alloc#=%u, free#=%u, null#=%u\n",
				   prAdapter->rMgtBufInfo.u4AllocCount,
				   prAdapter->rMgtBufInfo.u4FreeCount, prAdapter->rMgtBufInfo.u4AllocNullCount);
#endif

		DBGLOG(MEM, WARN, "\n");
	}
#endif

	return prMsduInfo;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID cnmMgtPktFree(P_ADAPTER_T prAdapter, P_MSDU_INFO_T prMsduInfo)
{
	P_QUE_T prQueList;

	KAL_SPIN_LOCK_DECLARATION();

	ASSERT(prAdapter);
	ASSERT(prMsduInfo);

	prQueList = &prAdapter->rTxCtrl.rFreeMsduInfoList;

	ASSERT(prMsduInfo->prPacket);
	if (prMsduInfo->prPacket) {
		cnmMemFree(prAdapter, prMsduInfo->prPacket);
		prMsduInfo->prPacket = NULL;
	}

	KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_MSDU_INFO_LIST);
	prMsduInfo->fgIsBasicRate = FALSE;
	QUEUE_INSERT_TAIL(prQueList, &prMsduInfo->rQueEntry)
	KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_MSDU_INFO_LIST);
}

/*----------------------------------------------------------------------------*/
/*!
* \brief This function is used to initial the MGMT/MSG memory pool.
*
* \param (none)
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/
VOID cnmMemInit(P_ADAPTER_T prAdapter)
{
	P_BUF_INFO_T prBufInfo;

	/* Initialize Management buffer pool */
	prBufInfo = &prAdapter->rMgtBufInfo;
	kalMemZero(prBufInfo, sizeof(prAdapter->rMgtBufInfo));
	prBufInfo->pucBuf = prAdapter->pucMgtBufCached;

	/* Setup available memory blocks. 1 indicates FREE */
	prBufInfo->rFreeBlocksBitmap = (BUF_BITMAP) BITS(0, MAX_NUM_OF_BUF_BLOCKS - 1);

	/* Initialize Message buffer pool */
	prBufInfo = &prAdapter->rMsgBufInfo;
	kalMemZero(prBufInfo, sizeof(prAdapter->rMsgBufInfo));
	prBufInfo->pucBuf = &prAdapter->aucMsgBuf[0];

	/* Setup available memory blocks. 1 indicates FREE */
	prBufInfo->rFreeBlocksBitmap = (BUF_BITMAP) BITS(0, MAX_NUM_OF_BUF_BLOCKS - 1);

	return;

}				/* end of cnmMemInit() */

/*----------------------------------------------------------------------------*/
/*!
* \brief Allocate MGMT/MSG memory pool.
*
* \param[in] eRamType       Target RAM type.
*                           TCM blk_sz= 16bytes, BUF blk_sz= 256bytes
* \param[in] u4Length       Length of the buffer to allocate.
*
* \retval !NULL    Pointer to the start address of allocated memory.
* \retval NULL     Fail to allocat memory
*/
/*----------------------------------------------------------------------------*/
UINT_32 u4MemAllocCnt = 0, u4MemFreeCnt = 0;
PVOID cnmMemAlloc(IN P_ADAPTER_T prAdapter, IN ENUM_RAM_TYPE_T eRamType, IN UINT_32 u4Length)
{
	P_BUF_INFO_T prBufInfo;
	BUF_BITMAP rRequiredBitmap;
	UINT_32 u4BlockNum;
	UINT_32 i, u4BlkSz, u4BlkSzInPower;
	PVOID pvMemory;

	KAL_SPIN_LOCK_DECLARATION();

	ASSERT(prAdapter);
	ASSERT(u4Length);

	u4MemAllocCnt++;

	if (eRamType == RAM_TYPE_MSG && u4Length <= 256) {
		prBufInfo = &prAdapter->rMsgBufInfo;
		u4BlkSzInPower = MSG_BUF_BLOCK_SIZE_IN_POWER_OF_2;
		u4BlkSz = MSG_BUF_BLOCK_SIZE;

		u4BlockNum = (u4Length + u4BlkSz - 1) >> u4BlkSzInPower;

		ASSERT(u4BlockNum <= MAX_NUM_OF_BUF_BLOCKS);
	} else {
		eRamType = RAM_TYPE_BUF;

		prBufInfo = &prAdapter->rMgtBufInfo;
		u4BlkSzInPower = MGT_BUF_BLOCK_SIZE_IN_POWER_OF_2;
		u4BlkSz = MGT_BUF_BLOCK_SIZE;

		u4BlockNum = (u4Length + u4BlkSz - 1) >> u4BlkSzInPower;

		ASSERT(u4BlockNum <= MAX_NUM_OF_BUF_BLOCKS);
	}

#if CFG_DBG_MGT_BUF
	prBufInfo->u4AllocCount++;
#endif

	KAL_ACQUIRE_SPIN_LOCK(prAdapter, eRamType == RAM_TYPE_MSG ? SPIN_LOCK_MSG_BUF : SPIN_LOCK_MGT_BUF);

	if ((u4BlockNum > 0) && (u4BlockNum <= MAX_NUM_OF_BUF_BLOCKS)) {

		/* Convert number of block into bit cluster */
		rRequiredBitmap = BITS(0, u4BlockNum - 1);

		for (i = 0; i <= (MAX_NUM_OF_BUF_BLOCKS - u4BlockNum); i++) {

			/* Have available memory blocks */
			if ((prBufInfo->rFreeBlocksBitmap & rRequiredBitmap)
			    == rRequiredBitmap) {

				/* Clear corresponding bits of allocated memory blocks */
				prBufInfo->rFreeBlocksBitmap &= ~rRequiredBitmap;

				/* Store how many blocks be allocated */
				prBufInfo->aucAllocatedBlockNum[i] = (UINT_8)u4BlockNum;

				KAL_RELEASE_SPIN_LOCK(prAdapter,
						      eRamType == RAM_TYPE_MSG ? SPIN_LOCK_MSG_BUF : SPIN_LOCK_MGT_BUF);

				kalMemZero(prBufInfo->pucBuf + (i << u4BlkSzInPower), u4BlockNum*u4BlkSz);
				/* Return the start address of allocated memory */
				return (PVOID) (prBufInfo->pucBuf + (i << u4BlkSzInPower));

			}

			rRequiredBitmap <<= 1;
		}
	}

	KAL_RELEASE_SPIN_LOCK(prAdapter, eRamType == RAM_TYPE_MSG ? SPIN_LOCK_MSG_BUF : SPIN_LOCK_MGT_BUF);

	/* cannot move the allocation between spin_lock_irqsave and spin_unlock_irqrestore */
#ifdef LINUX
	pvMemory = (PVOID) kalMemAlloc(u4Length, VIR_MEM_TYPE);
	if (pvMemory)
		kalMemZero(pvMemory, u4Length);
#else
	pvMemory = (PVOID) NULL;
#endif

	KAL_ACQUIRE_SPIN_LOCK(prAdapter, eRamType == RAM_TYPE_MSG ? SPIN_LOCK_MSG_BUF : SPIN_LOCK_MGT_BUF);

#if CFG_DBG_MGT_BUF
	prBufInfo->u4AllocNullCount++;

	if (pvMemory)
		prAdapter->u4MemAllocDynamicCount++;
#endif

	KAL_RELEASE_SPIN_LOCK(prAdapter, eRamType == RAM_TYPE_MSG ? SPIN_LOCK_MSG_BUF : SPIN_LOCK_MGT_BUF);

	return pvMemory;

}				/* end of cnmMemAlloc() */

/*----------------------------------------------------------------------------*/
/*!
* \brief Release memory to MGT/MSG memory pool.
*
* \param pucMemory  Start address of previous allocated memory
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/
VOID cnmMemFree(IN P_ADAPTER_T prAdapter, IN PVOID pvMemory)
{
	P_BUF_INFO_T prBufInfo;
	UINT_32 u4BlockIndex;
	BUF_BITMAP rAllocatedBlocksBitmap;
	ENUM_RAM_TYPE_T eRamType;

	KAL_SPIN_LOCK_DECLARATION();

	ASSERT(prAdapter);
	ASSERT(pvMemory);
	if (!pvMemory)
		return;

	u4MemFreeCnt++;

	/* Judge it belongs to which RAM type */
	if (((ULONG) pvMemory >= (ULONG)&prAdapter->aucMsgBuf[0]) &&
	    ((ULONG) pvMemory <= (ULONG)&prAdapter->aucMsgBuf[MSG_BUFFER_SIZE - 1])) {

		prBufInfo = &prAdapter->rMsgBufInfo;
		u4BlockIndex = ((ULONG) pvMemory - (ULONG) prBufInfo->pucBuf)
		    >> MSG_BUF_BLOCK_SIZE_IN_POWER_OF_2;
		ASSERT(u4BlockIndex < MAX_NUM_OF_BUF_BLOCKS);
		eRamType = RAM_TYPE_MSG;
	} else if (((ULONG) pvMemory >= (ULONG) prAdapter->pucMgtBufCached) &&
		   ((ULONG) pvMemory <= ((ULONG) prAdapter->pucMgtBufCached + MGT_BUFFER_SIZE - 1))) {
		prBufInfo = &prAdapter->rMgtBufInfo;
		u4BlockIndex = ((ULONG) pvMemory - (ULONG) prBufInfo->pucBuf)
		    >> MGT_BUF_BLOCK_SIZE_IN_POWER_OF_2;
		ASSERT(u4BlockIndex < MAX_NUM_OF_BUF_BLOCKS);
		eRamType = RAM_TYPE_BUF;
	} else {
#ifdef LINUX
		/* For Linux, it is supported because size is not needed */
		kalMemFree(pvMemory, VIR_MEM_TYPE, 0);
#else
		/* For Windows, it is not supported because of no size argument */
		ASSERT(0);
#endif

#if CFG_DBG_MGT_BUF
		prAdapter->u4MemFreeDynamicCount++;
#endif
		return;
	}

	KAL_ACQUIRE_SPIN_LOCK(prAdapter, eRamType == RAM_TYPE_MSG ? SPIN_LOCK_MSG_BUF : SPIN_LOCK_MGT_BUF);

#if CFG_DBG_MGT_BUF
	prBufInfo->u4FreeCount++;
#endif

	/* Convert number of block into bit cluster */
	ASSERT(prBufInfo->aucAllocatedBlockNum[u4BlockIndex] > 0);

	rAllocatedBlocksBitmap = BITS(0, prBufInfo->aucAllocatedBlockNum[u4BlockIndex] - 1);
	rAllocatedBlocksBitmap <<= u4BlockIndex;

	/* Clear saved block count for this memory segment */
	prBufInfo->aucAllocatedBlockNum[u4BlockIndex] = 0;

	/* Set corresponding bit of released memory block */
	prBufInfo->rFreeBlocksBitmap |= rAllocatedBlocksBitmap;

	KAL_RELEASE_SPIN_LOCK(prAdapter, eRamType == RAM_TYPE_MSG ? SPIN_LOCK_MSG_BUF : SPIN_LOCK_MGT_BUF);

	return;

}				/* end of cnmMemFree() */

/*----------------------------------------------------------------------------*/
/*!
* \brief
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID cnmStaRecInit(P_ADAPTER_T prAdapter)
{
	P_STA_RECORD_T prStaRec;
	UINT_16 i;

	for (i = 0; i < CFG_STA_REC_NUM; i++) {
		prStaRec = &prAdapter->arStaRec[i];

		prStaRec->ucIndex = (UINT_8) i;
		prStaRec->fgIsInUse = FALSE;
		prStaRec->qosMapSet = NULL;
	}
}

/*----------------------------------------------------------------------------*/
/*!
* \brief
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID cnmStaRecUninit(IN P_ADAPTER_T prAdapter)
{
	P_STA_RECORD_T prStaRec;
	UINT_16 i;

	for (i = 0; i < CFG_STA_REC_NUM; i++) {
		prStaRec = &prAdapter->arStaRec[i];

		if (prStaRec->fgIsInUse)
			cnmStaRecFree(prAdapter, prStaRec, FALSE);
	}
}

/*----------------------------------------------------------------------------*/
/*!
* \brief
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
P_STA_RECORD_T cnmStaRecAlloc(P_ADAPTER_T prAdapter, UINT_8 ucNetTypeIndex)
{
	P_STA_RECORD_T prStaRec;
	UINT_16 i, k;

	ASSERT(prAdapter);

	for (i = 0; i < CFG_STA_REC_NUM; i++) {
		prStaRec = &prAdapter->arStaRec[i];

		if (!prStaRec->fgIsInUse) {
			/*---- Initialize STA_REC_T here ----*/
			kalMemZero(prStaRec, sizeof(STA_RECORD_T));
			prStaRec->ucIndex = (UINT_8) i;
			prStaRec->ucNetTypeIndex = ucNetTypeIndex;
			prStaRec->fgIsInUse = TRUE;

			if (prStaRec->pucAssocReqIe) {
				kalMemFree(prStaRec->pucAssocReqIe, VIR_MEM_TYPE, prStaRec->u2AssocReqIeLen);
				prStaRec->pucAssocReqIe = NULL;
				prStaRec->u2AssocReqIeLen = 0;
			}

			/* Initialize the SN caches for duplicate detection */
			for (k = 0; k < TID_NUM + 1; k++)
				prStaRec->au2CachedSeqCtrl[k] = 0xFFFF;

			/* Initialize SW TX queues in STA_REC */
			for (k = 0; k < STA_WAIT_QUEUE_NUM; k++)
				LINK_INITIALIZE(&prStaRec->arStaWaitQueue[k]);

			/* Default enable TX/RX AMPDU */
			prStaRec->fgTxAmpduEn = TRUE;
			prStaRec->fgRxAmpduEn = TRUE;

#if CFG_ENABLE_PER_STA_STATISTICS && CFG_ENABLE_PKT_LIFETIME_PROFILE
			prStaRec->u4TotalTxPktsNumber = 0;
			prStaRec->u4TotalTxPktsTime = 0;
			prStaRec->u4MaxTxPktsTime = 0;
#endif

			for (k = 0; k < NUM_OF_PER_STA_TX_QUEUES; k++)
				QUEUE_INITIALIZE(&prStaRec->arTxQueue[k]);

			break;
		}
	}

	return (i < CFG_STA_REC_NUM) ? prStaRec : NULL;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID cnmStaRecFree(P_ADAPTER_T prAdapter, P_STA_RECORD_T prStaRec, BOOLEAN fgSyncToChip)
{
	ASSERT(prAdapter);
	ASSERT(prStaRec);

	/* To do: free related resources, e.g. timers, buffers, etc */
	cnmTimerStopTimer(prAdapter, &prStaRec->rTxReqDoneOrRxRespTimer);
	prStaRec->fgTransmitKeyExist = FALSE;
	prStaRec->fgSetPwrMgtBit = FALSE;

	if (prStaRec->pucAssocReqIe) {
		kalMemFree(prStaRec->pucAssocReqIe, VIR_MEM_TYPE, prStaRec->u2AssocReqIeLen);
		prStaRec->pucAssocReqIe = NULL;
		prStaRec->u2AssocReqIeLen = 0;
	}

	qmDeactivateStaRec(prAdapter, prStaRec->ucIndex);

	if (fgSyncToChip)
		cnmStaSendRemoveCmd(prAdapter, prStaRec);

	prStaRec->fgIsInUse = FALSE;

	if (prStaRec->qosMapSet) {
		QosMapSetRelease(prStaRec);
		prStaRec->qosMapSet = NULL;
	}
}

/*----------------------------------------------------------------------------*/
/*!
* \brief
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID cnmStaFreeAllStaByNetType(P_ADAPTER_T prAdapter, ENUM_NETWORK_TYPE_INDEX_T eNetTypeIndex, BOOLEAN fgSyncToChip)
{
	P_STA_RECORD_T prStaRec;
	UINT_16 i;

	for (i = 0; i < CFG_STA_REC_NUM; i++) {
		prStaRec = (P_STA_RECORD_T) &prAdapter->arStaRec[i];

		if (prStaRec->fgIsInUse && prStaRec->ucNetTypeIndex == (UINT_8) eNetTypeIndex)
			cnmStaRecFree(prAdapter, prStaRec, fgSyncToChip);
	}	/* end of for loop */
}

/*----------------------------------------------------------------------------*/
/*!
* \brief
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
P_STA_RECORD_T cnmGetStaRecByIndex(P_ADAPTER_T prAdapter, UINT_8 ucIndex)
{
	P_STA_RECORD_T prStaRec;

	ASSERT(prAdapter);

	prStaRec = (ucIndex < CFG_STA_REC_NUM) ? &prAdapter->arStaRec[ucIndex] : NULL;

	if (prStaRec && prStaRec->fgIsInUse == FALSE)
		prStaRec = NULL;

	return prStaRec;
}

/*----------------------------------------------------------------------------*/
/*!
* @brief Get STA_RECORD_T by Peer MAC Address(Usually TA).
*
* @param[in] pucPeerMacAddr      Given Peer MAC Address.
*
* @retval   Pointer to STA_RECORD_T, if found. NULL, if not found
*/
/*----------------------------------------------------------------------------*/
P_STA_RECORD_T cnmGetStaRecByAddress(P_ADAPTER_T prAdapter, UINT_8 ucNetTypeIndex, PUINT_8 pucPeerMacAddr)
{
	P_STA_RECORD_T prStaRec;
	UINT_16 i;

	ASSERT(prAdapter);
	ASSERT(pucPeerMacAddr);

	for (i = 0; i < CFG_STA_REC_NUM; i++) {
		prStaRec = &prAdapter->arStaRec[i];

		if (prStaRec->fgIsInUse &&
		    prStaRec->ucNetTypeIndex == ucNetTypeIndex &&
		    EQUAL_MAC_ADDR(prStaRec->aucMacAddr, pucPeerMacAddr)) {
			break;
		}
	}

	return (i < CFG_STA_REC_NUM) ? prStaRec : NULL;
}

/*----------------------------------------------------------------------------*/
/*!
* @brief Reset the Status and Reason Code Field to 0 of all Station Records for
*        the specified Network Type
*
* @param[in] eNetType       Specify Network Type
*
* @return   (none)
*/
/*----------------------------------------------------------------------------*/
VOID cnmStaRecResetStatus(P_ADAPTER_T prAdapter, ENUM_NETWORK_TYPE_INDEX_T eNetTypeIndex)
{
	cnmStaFreeAllStaByNetType(prAdapter, eNetTypeIndex, FALSE);

#if 0
	P_STA_RECORD_T prStaRec;
	UINT_16 i;

	ASSERT(prAdapter);

	for (i = 0; i < CFG_STA_REC_NUM; i++) {
		prStaRec = &prAdapter->arStaRec[i];

		if (prStaRec->fgIsInUse) {
			if ((eNetTypeIndex == NETWORK_TYPE_AIS_INDEX) && IS_STA_IN_AIS(prStaRec->eStaType)) {

				prStaRec->u2StatusCode = STATUS_CODE_SUCCESSFUL;
				prStaRec->u2ReasonCode = REASON_CODE_RESERVED;
				prStaRec->ucJoinFailureCount = 0;
				prStaRec->fgTransmitKeyExist = FALSE;

				prStaRec->fgSetPwrMgtBit = FALSE;
			}

			/* TODO(Kevin): For P2P and BOW */
		}
	}

	return;
#endif
}

/*----------------------------------------------------------------------------*/
/*!
* @brief This function will change the ucStaState of STA_RECORD_T and also do
*        event indication to HOST to sync the STA_RECORD_T in driver.
*
* @param[in] prStaRec       Pointer to the STA_RECORD_T
* @param[in] u4NewState     New STATE to change.
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
VOID cnmStaRecChangeState(P_ADAPTER_T prAdapter, P_STA_RECORD_T prStaRec, UINT_8 ucNewState)
{
	BOOLEAN fgNeedResp;

	ASSERT(prAdapter);
	ASSERT(prStaRec);
	ASSERT(prStaRec->fgIsInUse);

	/* Do nothing when following state transitions happen,
	 * other 6 conditions should be sync to FW, including 1-->1, 3-->3
	 */
	if ((ucNewState == STA_STATE_2 && prStaRec->ucStaState != STA_STATE_3) ||
	    (ucNewState == STA_STATE_1 && prStaRec->ucStaState == STA_STATE_2)) {
		prStaRec->ucStaState = ucNewState;
		return;
	}

	fgNeedResp = FALSE;
	if (ucNewState == STA_STATE_3) {
		secFsmEventStart(prAdapter, prStaRec);
		if (ucNewState != prStaRec->ucStaState)
			fgNeedResp = TRUE;
	} else {
		if (ucNewState != prStaRec->ucStaState && prStaRec->ucStaState == STA_STATE_3)
			qmDeactivateStaRec(prAdapter, prStaRec->ucIndex);
		fgNeedResp = FALSE;
	}
	prStaRec->ucStaState = ucNewState;

	cnmStaSendUpdateCmd(prAdapter, prStaRec, fgNeedResp);

#if CFG_ENABLE_WIFI_DIRECT
	/* To do: Confirm if it is invoked here or other location, but it should
	 *        be invoked after state sync of STA_REC
	 * Update system operation parameters for AP mode
	 */
	if (prAdapter->fgIsP2PRegistered && (IS_STA_IN_P2P(prStaRec))) {
		P_BSS_INFO_T prBssInfo;

		prBssInfo = &prAdapter->rWifiVar.arBssInfo[prStaRec->ucNetTypeIndex];

		if (prBssInfo->eCurrentOPMode == OP_MODE_ACCESS_POINT)
			rlmUpdateParamsForAP(prAdapter, prBssInfo, FALSE);
	}
#endif
}

P_STA_RECORD_T
cnmStaTheTypeGet(P_ADAPTER_T prAdapter,
		 ENUM_NETWORK_TYPE_INDEX_T eNetTypeIndex, ENUM_STA_TYPE_T eStaType, UINT32 *pu4StartIdx)
{
	P_STA_RECORD_T prStaRec = NULL;
	UINT_16 i;

	for (i = *pu4StartIdx; i < CFG_STA_REC_NUM; i++) {
		prStaRec = (P_STA_RECORD_T) &prAdapter->arStaRec[i];

		if (prStaRec->fgIsInUse &&
		    prStaRec->ucNetTypeIndex == (UINT_8) eNetTypeIndex && prStaRec->eStaType == eStaType) {
			i++;
			break;
		}

		prStaRec = NULL;	/* reset */
	}			/* end of for loop */

	*pu4StartIdx = i;
	return prStaRec;
}

/*----------------------------------------------------------------------------*/
/*!
* @brief
*
* @param[in]
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
static VOID cnmStaRecHandleEventPkt(P_ADAPTER_T prAdapter, P_CMD_INFO_T prCmdInfo, PUINT_8 pucEventBuf)
{
	P_EVENT_ACTIVATE_STA_REC_T prEventContent;
	P_STA_RECORD_T prStaRec;

	prEventContent = (P_EVENT_ACTIVATE_STA_REC_T) pucEventBuf;
	prStaRec = cnmGetStaRecByIndex(prAdapter, prEventContent->ucStaRecIdx);

	if (prStaRec && prStaRec->ucStaState == STA_STATE_3 &&
	    !kalMemCmp(&prStaRec->aucMacAddr[0], &prEventContent->aucMacAddr[0], MAC_ADDR_LEN)) {

		qmActivateStaRec(prAdapter, prStaRec);
	}

}

/*----------------------------------------------------------------------------*/
/*!
* @brief
*
* @param[in]
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
static VOID cnmStaSendUpdateCmd(P_ADAPTER_T prAdapter, P_STA_RECORD_T prStaRec, BOOLEAN fgNeedResp)
{
	P_CMD_UPDATE_STA_RECORD_T prCmdContent;
	WLAN_STATUS rStatus;

	ASSERT(prAdapter);
	ASSERT(prStaRec);
	ASSERT(prStaRec->fgIsInUse);

	/* To do: come out a mechanism to limit one STA_REC sync once for AP mode
	 *        to avoid buffer empty case when many STAs are associated
	 *        simultaneously.
	 */

	/* To do: how to avoid 2 times of allocated memory. Use Stack?
	 *        One is here, the other is in wlanSendQueryCmd()
	 */
	prCmdContent = cnmMemAlloc(prAdapter, RAM_TYPE_BUF, sizeof(CMD_UPDATE_STA_RECORD_T));
	ASSERT(prCmdContent);

	/* To do: exception handle */
	if (!prCmdContent)
		return;

	prCmdContent->ucIndex = prStaRec->ucIndex;
	prCmdContent->ucStaType = (UINT_8) prStaRec->eStaType;
	kalMemCopy(&prCmdContent->aucMacAddr[0], &prStaRec->aucMacAddr[0], MAC_ADDR_LEN);
	prCmdContent->u2AssocId = prStaRec->u2AssocId;
	prCmdContent->u2ListenInterval = prStaRec->u2ListenInterval;
	prCmdContent->ucNetTypeIndex = prStaRec->ucNetTypeIndex;

	prCmdContent->ucDesiredPhyTypeSet = prStaRec->ucDesiredPhyTypeSet;
	prCmdContent->u2DesiredNonHTRateSet = prStaRec->u2DesiredNonHTRateSet;
	prCmdContent->u2BSSBasicRateSet = prStaRec->u2BSSBasicRateSet;
	prCmdContent->ucMcsSet = prStaRec->ucMcsSet;
	prCmdContent->ucSupMcs32 = (UINT_8) prStaRec->fgSupMcs32;
	prCmdContent->u2HtCapInfo = prStaRec->u2HtCapInfo;
	prCmdContent->ucNeedResp = (UINT_8) fgNeedResp;

#if !CFG_SLT_SUPPORT
	if (prAdapter->rWifiVar.eRateSetting != FIXED_RATE_NONE) {
		/* override rate configuration */
		nicUpdateRateParams(prAdapter,
				    prAdapter->rWifiVar.eRateSetting,
				    &(prCmdContent->ucDesiredPhyTypeSet),
				    &(prCmdContent->u2DesiredNonHTRateSet),
				    &(prCmdContent->u2BSSBasicRateSet),
				    &(prCmdContent->ucMcsSet),
				    &(prCmdContent->ucSupMcs32), &(prCmdContent->u2HtCapInfo));
	}
#endif

	prCmdContent->ucIsQoS = prStaRec->fgIsQoS;
	prCmdContent->ucIsUapsdSupported = prStaRec->fgIsUapsdSupported;
	prCmdContent->ucStaState = prStaRec->ucStaState;

	prCmdContent->ucAmpduParam = prStaRec->ucAmpduParam;
	prCmdContent->u2HtExtendedCap = prStaRec->u2HtExtendedCap;
	prCmdContent->u4TxBeamformingCap = prStaRec->u4TxBeamformingCap;
	prCmdContent->ucAselCap = prStaRec->ucAselCap;
	prCmdContent->ucRCPI = prStaRec->ucRCPI;

	prCmdContent->ucUapsdAc = prStaRec->ucBmpTriggerAC | (prStaRec->ucBmpDeliveryAC << 4);
	prCmdContent->ucUapsdSp = prStaRec->ucUapsdSp;
	/* if AP's max idle time is greater than 30s, then we send keep alive packets every 30 sec */
	prCmdContent->ucKeepAliveDuration = (UINT_8)prStaRec->u2MaxIdlePeriod;
	prCmdContent->ucKeepAliveOption = prStaRec->ucIdleOption;

	if (prCmdContent->ucKeepAliveDuration > 0)
		DBGLOG(CNM, INFO, "keep-alive duration is %d\n", prCmdContent->ucKeepAliveDuration);

	rStatus = wlanSendSetQueryCmd(prAdapter,	/* prAdapter */
				      CMD_ID_UPDATE_STA_RECORD,	/* ucCID */
				      TRUE,	/* fgSetQuery */
				      fgNeedResp,	/* fgNeedResp */
				      FALSE,	/* fgIsOid */
				      fgNeedResp ? cnmStaRecHandleEventPkt : NULL, NULL, /* pfCmdTimeoutHandler */
				      sizeof(CMD_UPDATE_STA_RECORD_T),	/* u4SetQueryInfoLen */
				      (PUINT_8) prCmdContent,	/* pucInfoBuffer */
				      NULL,	/* pvSetQueryBuffer */
				      0	/* u4SetQueryBufferLen */
	    );

	ASSERT(rStatus == WLAN_STATUS_PENDING);

	cnmMemFree(prAdapter, prCmdContent);
}

/*----------------------------------------------------------------------------*/
/*!
* @brief
*
* @param[in]
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
static VOID cnmStaSendRemoveCmd(P_ADAPTER_T prAdapter, P_STA_RECORD_T prStaRec)
{
	CMD_REMOVE_STA_RECORD_T rCmdContent;
	WLAN_STATUS rStatus;

	ASSERT(prAdapter);
	ASSERT(prStaRec);

	rCmdContent.ucIndex = prStaRec->ucIndex;
	kalMemCopy(&rCmdContent.aucMacAddr[0], &prStaRec->aucMacAddr[0], MAC_ADDR_LEN);

	rStatus = wlanSendSetQueryCmd(prAdapter,	/* prAdapter */
				      CMD_ID_REMOVE_STA_RECORD,	/* ucCID */
				      TRUE,	/* fgSetQuery */
				      FALSE,	/* fgNeedResp */
				      FALSE,	/* fgIsOid */
				      NULL,	/* pfCmdDoneHandler */
				      NULL,	/* pfCmdTimeoutHandler */
				      sizeof(CMD_REMOVE_STA_RECORD_T),	/* u4SetQueryInfoLen */
				      (PUINT_8) &rCmdContent,	/* pucInfoBuffer */
				      NULL,	/* pvSetQueryBuffer */
				      0	/* u4SetQueryBufferLen */
	    );

	ASSERT(rStatus == WLAN_STATUS_PENDING);
}
