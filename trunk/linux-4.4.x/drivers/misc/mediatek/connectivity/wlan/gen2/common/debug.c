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

#include "precomp.h"
#include "gl_kal.h"

struct COMMAND {
	UINT_8 ucCID;
	BOOLEAN fgSetQuery;
	BOOLEAN fgNeedResp;
	UINT_8 ucCmdSeqNum;
};

struct SECURITY_FRAME {
	UINT_16 u2EthType;
	UINT_16 u2Reserved;
};

struct MGMT_FRAME {
	UINT_16 u2FrameCtl;
	UINT_16 u2DurationID;
};

typedef struct _TC_RES_RELEASE_ENTRY {
	UINT_64 u8RelaseTime;
	UINT_32 u4RelCID;
	UINT_8	ucTc4RelCnt;
	UINT_8	ucAvailableTc4;
} TC_RES_RELEASE_ENTRY, *P_TC_RES_RELEASE_ENTRY;

typedef struct _CMD_TRACE_ENTRY {
	UINT_64 u8TxTime;
	COMMAND_TYPE eCmdType;
	UINT_32 u4RelCID;
	union {
		struct COMMAND rCmd;
		struct SECURITY_FRAME rSecFrame;
		struct MGMT_FRAME rMgmtFrame;
	} u;
} CMD_TRACE_ENTRY, *P_CMD_TRACE_ENTRY;

#define READ_CPUPCR_MAX_NUM 3
typedef struct _COMMAND_ENTRY {
	UINT_64 u8TxTime;
	UINT_64 u8ReadFwTime;
	UINT_32 u4ReadFwValue;
	UINT_32 arCpupcrValue[READ_CPUPCR_MAX_NUM];
	UINT_32 u4RelCID;
	UINT_16 u2Counter;
	struct COMMAND rCmd;
} COMMAND_ENTRY, *P_COMMAND_ENTRY;

typedef struct _PKT_INFO_ENTRY {
	UINT_64 u8Timestamp;
	UINT_8 status;
	UINT_16 u2EtherType;
	UINT_8 ucIpProto;
	UINT_16 u2IpId;
	UINT_16 u2ArpOpCode;

} PKT_INFO_ENTRY, *P_PKT_INFO_ENTRY;

typedef struct _PKT_TRACE_RECORD {
	P_PKT_INFO_ENTRY pTxPkt;
	P_PKT_INFO_ENTRY pRxPkt;
	UINT_32 u4TxIndex;
	UINT_32 u4RxIndex;
} PKT_TRACE_RECORD, *P_PKT_TRACE_RECORD;

typedef struct _SCAN_HIF_DESC_RECORD {
	P_HIF_TX_DESC_T pTxDescScanWriteBefore;
	P_HIF_TX_DESC_T pTxDescScanWriteDone;
	UINT_64 u8ScanWriteBeforeTime;
	UINT_64 u8ScanWriteDoneTime;
	UINT_32 aucFreeBufCntScanWriteBefore;
	UINT_32 aucFreeBufCntScanWriteDone;
} SCAN_HIF_DESC_RECORD, *P_SCAN_HIF_DESC_RECORD;

typedef struct _FWDL_DEBUG_T {
	UINT_32	u4TxStartTime;
	UINT_32	u4TxDoneTime;
	UINT_32	u4RxStartTime;
	UINT_32	u4RxDoneTime;
	UINT_32	u4Section;
	UINT_32	u4DownloadSize;
	UINT_32	u4ResponseTime;
} FWDL_DEBUG_T, *P_FWDL_DEBUG_T;

typedef struct _BSS_TRACE_RECORD {
	UINT_8 aucBSSID[MAC_ADDR_LEN];
	UINT_8 ucRCPI;
} BSS_TRACE_RECORD, *P_BSS_TRACE_RECORD;

typedef struct _SCAN_TARGET_BSS_LIST {
	P_BSS_TRACE_RECORD prBssTraceRecord;
	UINT_32 u4BSSIDCount;
} SCAN_TARGET_BSS_LIST, *P_SCAN_TARGET_BSS_LIST;

#define PKT_INFO_BUF_MAX_NUM 50
#define PKT_INFO_MSG_LENGTH 200
#define PKT_INFO_MSG_GROUP_RANGE 3
#define TC_RELEASE_TRACE_BUF_MAX_NUM 100
#define TXED_CMD_TRACE_BUF_MAX_NUM 100
#define TXED_COMMAND_BUF_MAX_NUM 10
#define MAX_FW_IMAGE_PACKET_COUNT	500
#define SCAN_TARGET_BSS_MAX_NUM 20
#define SCAN_MSG_MAX_LEN 256



static P_TC_RES_RELEASE_ENTRY gprTcReleaseTraceBuffer;
static P_CMD_TRACE_ENTRY gprCmdTraceEntry;
static P_COMMAND_ENTRY gprCommandEntry;
static PKT_TRACE_RECORD grPktRec;
static SCAN_HIF_DESC_RECORD grScanHifDescRecord;
P_FWDL_DEBUG_T gprFWDLDebug;



UINT_32 u4FWDL_packet_count;
static SCAN_TARGET_BSS_LIST grScanTargetBssList;



VOID wlanPktDebugTraceInfoARP(UINT_8 status, UINT_8 eventType, UINT_16 u2ArpOpCode)
{
	if (eventType == PKT_TX)
		status = 0xFF;

	wlanPktDebugTraceInfo(status, eventType, ETH_P_ARP, 0, 0, u2ArpOpCode);

}
VOID wlanPktDebugTraceInfoIP(UINT_8 status, UINT_8 eventType, UINT_8 ucIpProto, UINT_16 u2IpId)
{
	if (eventType == PKT_TX)
		status = 0xFF;

	wlanPktDebugTraceInfo(status, eventType, ETH_P_IP, ucIpProto, u2IpId, 0);

}

VOID wlanPktDebugTraceInfo(UINT_8 status, UINT_8 eventType
	, UINT_16 u2EtherType, UINT_8 ucIpProto, UINT_16 u2IpId, UINT_16 u2ArpOpCode)
{

	P_PKT_INFO_ENTRY prPkt = NULL;
	UINT_32 index;

	DBGLOG(TX, LOUD, "PKT id = 0x%02x, status =%d, Proto = %d, type =%d\n"
		, u2IpId, status, ucIpProto, eventType);
	do {

		if (grPktRec.pTxPkt == NULL || grPktRec.pRxPkt == NULL) {
			DBGLOG(TX, ERROR, "pTxPkt is null point !");
			break;
		}

		/* debug for Package info begin */
		if (eventType == PKT_TX) {
			prPkt = &grPktRec.pTxPkt[grPktRec.u4TxIndex];
			grPktRec.u4TxIndex++;
			if (grPktRec.u4TxIndex == PKT_INFO_BUF_MAX_NUM)
				grPktRec.u4TxIndex = 0;
		} else if (eventType == PKT_RX) {
			prPkt = &grPktRec.pRxPkt[grPktRec.u4RxIndex];
			grPktRec.u4RxIndex++;
			if (grPktRec.u4RxIndex == PKT_INFO_BUF_MAX_NUM)
				grPktRec.u4RxIndex = 0;
		}

		if (prPkt) {
			prPkt->u8Timestamp = sched_clock();
			prPkt->status = status;
			prPkt->u2EtherType = u2EtherType;
			prPkt->ucIpProto = ucIpProto;
			prPkt->u2IpId = u2IpId;
			prPkt->u2ArpOpCode = u2ArpOpCode;
		}


		/* Update tx status */
		if (eventType == PKT_TX_DONE) {
			/* Support Ethernet type = IP*/
			if (u2EtherType == ETH_P_IP) {
				for (index = 0; index < PKT_INFO_BUF_MAX_NUM; index++) {
					if (grPktRec.pTxPkt[index].u2IpId == u2IpId) {
						grPktRec.pTxPkt[index].status = status;
						DBGLOG(TX, LOUD, "PKT_TX_DONE match\n");
						break;
					}
				}
			}
		}
	} while (FALSE);
}

VOID wlanPktDebugDumpInfo(P_ADAPTER_T prAdapter)
{

	UINT_32 i;
	UINT_32 index;
	UINT_32 offsetMsg;
	UINT_32 pktIndex;
	P_PKT_INFO_ENTRY prPktInfo;
	UINT_8 pucMsg[PKT_INFO_MSG_LENGTH];

	do {

		if (grPktRec.pTxPkt == NULL || grPktRec.pRxPkt == NULL)
			break;

		if (grPktRec.u4TxIndex == 0 && grPktRec.u4RxIndex == 0)
			break;

		offsetMsg = 0;
		/* start dump pkt info of tx/rx by decrease timestap */

		for (i = 0 ; i < 2 ; i++) {
			for (index = 0; index < PKT_INFO_BUF_MAX_NUM ; index++) {
				if (i == 0) {
					/* TX */
					pktIndex = (PKT_INFO_BUF_MAX_NUM + (grPktRec.u4TxIndex - 1) - index)
						% PKT_INFO_BUF_MAX_NUM;
					prPktInfo = &grPktRec.pTxPkt[pktIndex];
				} else if (i == 1) {
					/* RX */
					pktIndex = (PKT_INFO_BUF_MAX_NUM + (grPktRec.u4RxIndex - 1) - index)
						% PKT_INFO_BUF_MAX_NUM;
					prPktInfo = &grPktRec.pRxPkt[pktIndex];
				}
				/*ucIpProto = 0x01 ICMP */
				/*ucIpProto = 0x11 UPD */
				/*ucIpProto = 0x06 TCP */
				offsetMsg += kalSnprintf(pucMsg + offsetMsg
				, PKT_INFO_MSG_LENGTH - offsetMsg
				, "(%2d)t=%llu s=%d e=0x%02x,p=0x%2x id=0x%4x,op=%d  "
				, index, prPktInfo->u8Timestamp
				, prPktInfo->status
				, prPktInfo->u2EtherType
				, prPktInfo->ucIpProto
				, prPktInfo->u2IpId
				, prPktInfo->u2ArpOpCode);

				if ((index == PKT_INFO_BUF_MAX_NUM - 1) ||
					(index % PKT_INFO_MSG_GROUP_RANGE == (PKT_INFO_MSG_GROUP_RANGE - 1))) {
					if (i == 0)
						DBGLOG(TX, INFO, "%s\n", pucMsg);
					else if (i == 1)
						DBGLOG(RX, INFO, "%s\n", pucMsg);

					offsetMsg = 0;
					kalMemSet(pucMsg, '\0', PKT_INFO_MSG_LENGTH);
				}
			}
		}

	} while (FALSE);

}
VOID wlanDebugInit(VOID)
{
	/* debug for command/tc4 resource begin */
	gprTcReleaseTraceBuffer =
		kalMemAlloc(TC_RELEASE_TRACE_BUF_MAX_NUM * sizeof(TC_RES_RELEASE_ENTRY), PHY_MEM_TYPE);
	kalMemZero(gprTcReleaseTraceBuffer, TC_RELEASE_TRACE_BUF_MAX_NUM * sizeof(TC_RES_RELEASE_ENTRY));
	gprCmdTraceEntry = kalMemAlloc(TXED_CMD_TRACE_BUF_MAX_NUM * sizeof(CMD_TRACE_ENTRY), PHY_MEM_TYPE);
	kalMemZero(gprCmdTraceEntry, TXED_CMD_TRACE_BUF_MAX_NUM * sizeof(CMD_TRACE_ENTRY));

	gprCommandEntry = kalMemAlloc(TXED_COMMAND_BUF_MAX_NUM * sizeof(COMMAND_ENTRY), PHY_MEM_TYPE);
	kalMemZero(gprCommandEntry, TXED_COMMAND_BUF_MAX_NUM * sizeof(COMMAND_ENTRY));
	/* debug for command/tc4 resource end */

	/* debug for package info begin */
	grPktRec.pTxPkt = kalMemAlloc(PKT_INFO_BUF_MAX_NUM * sizeof(PKT_INFO_ENTRY), VIR_MEM_TYPE);
	kalMemZero(grPktRec.pTxPkt, PKT_INFO_BUF_MAX_NUM * sizeof(PKT_INFO_ENTRY));
	grPktRec.u4TxIndex = 0;
	grPktRec.pRxPkt = kalMemAlloc(PKT_INFO_BUF_MAX_NUM * sizeof(PKT_INFO_ENTRY), VIR_MEM_TYPE);
	kalMemZero(grPktRec.pRxPkt, PKT_INFO_BUF_MAX_NUM * sizeof(PKT_INFO_ENTRY));
	grPktRec.u4RxIndex = 0;
	/* debug for package info end */


	/*debug for scan request tx_description begin*/
	grScanHifDescRecord.pTxDescScanWriteBefore = kalMemAlloc(NIC_TX_BUFF_COUNT_TC4 * sizeof(HIF_TX_DESC_T)
		, VIR_MEM_TYPE);
	grScanHifDescRecord.aucFreeBufCntScanWriteBefore = 0;
	grScanHifDescRecord.pTxDescScanWriteDone = kalMemAlloc(NIC_TX_BUFF_COUNT_TC4 * sizeof(HIF_TX_DESC_T)
	, VIR_MEM_TYPE);
	grScanHifDescRecord.aucFreeBufCntScanWriteDone = 0;
	/*debug for scan request tx_description end*/

	/*debug for scan target bss begin*/
	grScanTargetBssList.prBssTraceRecord = kalMemAlloc(SCAN_TARGET_BSS_MAX_NUM * sizeof(BSS_TRACE_RECORD)
		, VIR_MEM_TYPE);
	kalMemZero(grScanTargetBssList.prBssTraceRecord, SCAN_TARGET_BSS_MAX_NUM * sizeof(BSS_TRACE_RECORD));
	grScanTargetBssList.u4BSSIDCount = 0;
	/*debug for scan target bss end*/

}

VOID wlanDebugUninit(VOID)
{
	/* debug for command/tc4 resource begin */
	kalMemFree(gprTcReleaseTraceBuffer, PHY_MEM_TYPE,
			TC_RELEASE_TRACE_BUF_MAX_NUM * sizeof(TC_RES_RELEASE_ENTRY));
	kalMemFree(gprCmdTraceEntry, PHY_MEM_TYPE, TXED_CMD_TRACE_BUF_MAX_NUM * sizeof(CMD_TRACE_ENTRY));
	kalMemFree(gprCommandEntry, PHY_MEM_TYPE, TXED_COMMAND_BUF_MAX_NUM * sizeof(COMMAND_ENTRY));
	/* debug for command/tc4 resource end */

	/* debug for package info begin */
	kalMemFree(grPktRec.pTxPkt, VIR_MEM_TYPE, PKT_INFO_BUF_MAX_NUM * sizeof(PKT_INFO_ENTRY));
	grPktRec.u4TxIndex = 0;
	kalMemFree(grPktRec.pRxPkt, VIR_MEM_TYPE, PKT_INFO_BUF_MAX_NUM * sizeof(PKT_INFO_ENTRY));
	grPktRec.u4RxIndex = 0;
	/* debug for package info end */


	/*debug for scan request tx_description begin*/
	kalMemFree(grScanHifDescRecord.pTxDescScanWriteBefore
	, VIR_MEM_TYPE, NIC_TX_BUFF_COUNT_TC4 * sizeof(HIF_TX_DESC_T));
	grScanHifDescRecord.aucFreeBufCntScanWriteBefore = 0;
	kalMemFree(grScanHifDescRecord.pTxDescScanWriteDone
	, VIR_MEM_TYPE, NIC_TX_BUFF_COUNT_TC4 * sizeof(HIF_TX_DESC_T));
	grScanHifDescRecord.aucFreeBufCntScanWriteDone = 0;
	/*debug for scan request tx_description end*/

	/*debug for scan target bss begin*/
	kalMemFree(grScanTargetBssList.prBssTraceRecord
	, VIR_MEM_TYPE, SCAN_TARGET_BSS_MAX_NUM * sizeof(BSS_TRACE_RECORD));
	grScanTargetBssList.u4BSSIDCount = 0;
	/*debug for scan target bss end*/



}
VOID wlanDebugScanTargetBSSRecord(P_ADAPTER_T prAdapter, P_BSS_DESC_T prBssDesc)
{
	P_CONNECTION_SETTINGS_T prConnSettings;
	P_SCAN_INFO_T prScanInfo;
	P_BSS_TRACE_RECORD prBssTraceRecord;
	UINT_32 i;

	ASSERT(prAdapter);

	prConnSettings = &(prAdapter->rWifiVar.rConnSettings);
	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);


	if (prBssDesc == NULL) {
		DBGLOG(SCN, LOUD, "Scan Desc bss is null !\n");
		return;
	}
	if (prConnSettings->ucSSIDLen == 0) {
		DBGLOG(SCN, LOUD, "Target BSS length is 0, ignore it!\n");
		return;
	}
	if (prScanInfo->eCurrentState == SCAN_STATE_IDLE) {
		DBGLOG(SCN, LOUD, "Ignore beacon/probeRsp, during SCAN Idle!\n");
		return;
	}


	/*dump beacon and probeRsp by connect setting SSID and ignore null bss*/
	if (EQUAL_SSID(prBssDesc->aucSSID,
		prBssDesc->ucSSIDLen,
		prConnSettings->aucSSID,
		prConnSettings->ucSSIDLen) && (prConnSettings->ucSSIDLen > 0)) {
		/*Insert BssDesc and ignore repeats*/

		if (grScanTargetBssList.u4BSSIDCount > SCAN_TARGET_BSS_MAX_NUM) {
			DBGLOG(SCN, LOUD, "u4BSSIDCount out of bound!\n");
			return;
		}

		for (i = 0 ; i < grScanTargetBssList.u4BSSIDCount ; i++) {
			prBssTraceRecord = &(grScanTargetBssList.prBssTraceRecord[i]);
			if (EQUAL_MAC_ADDR(prBssTraceRecord->aucBSSID, prBssDesc->aucBSSID)) {
				/*if exist ,update it*/
				prBssTraceRecord->ucRCPI = prBssDesc->ucRCPI;
				break;
			}
		}
		if ((i == grScanTargetBssList.u4BSSIDCount) &&
			(grScanTargetBssList.u4BSSIDCount < SCAN_TARGET_BSS_MAX_NUM)) {
			prBssTraceRecord = &(grScanTargetBssList.prBssTraceRecord[i]);
			/*add the new bssDesc recored*/
			COPY_MAC_ADDR(prBssTraceRecord->aucBSSID, prBssDesc->aucBSSID);
			prBssTraceRecord->ucRCPI = prBssDesc->ucRCPI;
			grScanTargetBssList.u4BSSIDCount++;
		}

	}
}

VOID wlanDebugScanTargetBSSDump(P_ADAPTER_T prAdapter)
{

	P_CONNECTION_SETTINGS_T prConnSettings;
	P_BSS_TRACE_RECORD prBssTraceRecord;
	UINT_32 i;
	UINT_8 aucMsg[SCAN_MSG_MAX_LEN];
	UINT_8 offset = 0;

	ASSERT(prAdapter);

	prConnSettings = &(prAdapter->rWifiVar.rConnSettings);

	if (prConnSettings->ucSSIDLen == 0) {
		DBGLOG(SCN, LOUD, "Target BSS length is 0, ignore it!\n");
		return;
	}

	if (grScanTargetBssList.u4BSSIDCount > SCAN_TARGET_BSS_MAX_NUM) {
		DBGLOG(SCN, WARN, "u4BSSIDCount out of bound :%d\n", grScanTargetBssList.u4BSSIDCount);
		return;

	}

	offset += kalSnprintf(aucMsg + offset, SCAN_MSG_MAX_LEN - offset
		, "[%s: BSSIDNum:%d]:"
		, prConnSettings->aucSSID
		, grScanTargetBssList.u4BSSIDCount);

	for (i = 0 ; i < grScanTargetBssList.u4BSSIDCount ; i++) {
		prBssTraceRecord = &(grScanTargetBssList.prBssTraceRecord[i]);

		DBGLOG(SCN, LOUD, "dump:[%pM],Rssi=%d\n"
			, prBssTraceRecord->aucBSSID, RCPI_TO_dBm(prBssTraceRecord->ucRCPI));

		if (i == (SCAN_TARGET_BSS_MAX_NUM/2) ||
			(i == grScanTargetBssList.u4BSSIDCount-1)) {
			DBGLOG(SCN, INFO, "%s\n", aucMsg);
			offset = 0;
			kalMemZero(aucMsg, sizeof(aucMsg));
		}

		offset += kalSnprintf(aucMsg + offset, SCAN_MSG_MAX_LEN - offset
			, "%pM/%d,"
			, prBssTraceRecord->aucBSSID
			, RCPI_TO_dBm(prBssTraceRecord->ucRCPI));
	}

	grScanTargetBssList.u4BSSIDCount = 0;
	kalMemZero(grScanTargetBssList.prBssTraceRecord, sizeof(BSS_TRACE_RECORD) * SCAN_TARGET_BSS_MAX_NUM);

}

VOID wlanDebugHifDescriptorRecord(P_ADAPTER_T prAdapter, ENUM_AMPDU_TYPE type
	, ENUM_DEBUG_TRAFFIC_CLASS_INDEX_T tcIndex, PUINT_8 pucBuffer)
{
	UINT_32 i;
	UINT_32 u4Offset;
	UINT_32 u4StartAddr;
	P_HIF_TX_DESC_T prTxDesc;
	P_HIF_RX_DESC_T prRxDesc;
	UINT_32 u4TcCount;

	if (pucBuffer == NULL) {
		DBGLOG(TX, ERROR, "wlanDebugHifDescriptorRecord pucBuffer is Null !");
		return;
	}


	if (type == MTK_AMPDU_TX_DESC) {

		if (tcIndex == DEBUG_TC0_INDEX) {
			u4TcCount = NIC_TX_INIT_BUFF_COUNT_TC0;
			u4StartAddr = AP_MCU_TX_DESC_ADDR;
			u4Offset = AP_MCU_BANK_OFFSET;
		} else if (tcIndex == DEBUG_TC4_INDEX) {
			u4TcCount = NIC_TX_BUFF_COUNT_TC4;
			u4StartAddr = AP_MCU_TC_INDEX_4_ADDR;
			u4Offset = AP_MCU_TC_INDEX_4_OFFSET;
		} else {
			DBGLOG(TX, ERROR, "Type :%d TC_INDEX :%d don't support !", type, tcIndex);
			return;
		}

		prTxDesc = (P_HIF_TX_DESC_T)pucBuffer;
		for (i = 0; i < u4TcCount ; i++)
			HAL_GET_APMCU_MEM(prAdapter, u4StartAddr, u4Offset, i, (PUINT_8) &prTxDesc[i]
				, sizeof(HIF_TX_DESC_T));


	} else if (type == MTK_AMPDU_RX_DESC) {

		if (tcIndex == DEBUG_TC0_INDEX) {
			u4TcCount = NIC_TX_INIT_BUFF_COUNT_TC0;
			u4StartAddr = AP_MCU_RX_DESC_ADDR;
			u4Offset = AP_MCU_BANK_OFFSET;
		} else {
			DBGLOG(RX, ERROR, "Type :%d TC_INDEX :%d don't support !", type, tcIndex);
			return;
		}

		prRxDesc = (P_HIF_RX_DESC_T)pucBuffer;
		for (i = 0; i < u4TcCount ; i++)
			HAL_GET_APMCU_MEM(prAdapter, u4StartAddr, u4Offset, i, (PUINT_8) &prRxDesc[i]
				, sizeof(HIF_RX_DESC_T));
	}

}

VOID wlanDebugHifDescriptorPrint(P_ADAPTER_T prAdapter, ENUM_AMPDU_TYPE type
	, ENUM_DEBUG_TRAFFIC_CLASS_INDEX_T tcIndex, PUINT_8 pucBuffer)
{
	UINT_32 i;
	UINT_32 u4TcCount;
	P_HIF_TX_DESC_T prTxDesc;
	P_HIF_RX_DESC_T prRxDesc;

	if (pucBuffer == NULL) {
		DBGLOG(TX, ERROR, "wlanDebugHifDescriptorDump pucBuffer is Null !");
		return;
	}

	if (type == MTK_AMPDU_TX_DESC) {
		if (tcIndex == DEBUG_TC0_INDEX)
			u4TcCount = NIC_TX_INIT_BUFF_COUNT_TC0;
		else if (tcIndex == DEBUG_TC4_INDEX)
			u4TcCount = NIC_TX_BUFF_COUNT_TC4;
		else {
			DBGLOG(TX, ERROR, "Type :%d TC_INDEX :%d don't support !", type, tcIndex);
			return;
		}

		prTxDesc = (P_HIF_TX_DESC_T)pucBuffer;
		DBGLOG(TX, INFO, "Start dump Tx_desc from APMCU\n");
		for (i = 0; i < u4TcCount ; i++) {
			DBGLOG(TX, INFO
				, "TC%d[%d]uOwn:%2x,CS:%2x,R1:%2x,ND:0x%08x,SA: 0x%08x,R2:%x\n"
				, tcIndex, i, prTxDesc[i].ucOwn, prTxDesc[i].ucDescChksum
				, prTxDesc[i].u2Rsrv1, prTxDesc[i].u4NextDesc
				, prTxDesc[i].u4BufStartAddr, prTxDesc[i].u4Rsrv2);
		}

	} else if (type == MTK_AMPDU_RX_DESC) {

		if (tcIndex == DEBUG_TC0_INDEX)
			u4TcCount = NIC_TX_INIT_BUFF_COUNT_TC0;
		else {
			DBGLOG(RX, ERROR, "Type :%d TC_INDEX :%d don't support !", type, tcIndex);
			return;
		}

		prRxDesc = (P_HIF_RX_DESC_T)pucBuffer;
		DBGLOG(RX, INFO, "Start dump rx_desc from APMCU\n");
		for (i = 0; i < u4TcCount ; i++) {
			DBGLOG(RX, INFO
				, "RX%d[%d]uOwn:%2x,CS:%2x,TO:%x,CSI:%x,ND:0x%08x,SA:0x%08x,len:%x,R1:%x\n"
				, tcIndex, i, prRxDesc[i].ucOwn, prRxDesc[i].ucDescChksum
				, prRxDesc[i].ucEtherTypeOffset, prRxDesc[i].ucChkSumInfo
				, prRxDesc[i].u4NextDesc, prRxDesc[i].u4BufStartAddr
				, prRxDesc[i].u2RxBufLen, prRxDesc[i].u2Rsrv1);

		}
	}

}

VOID wlanDebugHifDescriptorDump(P_ADAPTER_T prAdapter, ENUM_AMPDU_TYPE type
	, ENUM_DEBUG_TRAFFIC_CLASS_INDEX_T tcIndex)
{
	UINT_32 size = NIC_TX_BUFF_SUM;
	P_HIF_TX_DESC_T prTxDesc;
	P_HIF_RX_DESC_T prRxDesc;


	if (type == MTK_AMPDU_TX_DESC) {

		prTxDesc = (P_HIF_TX_DESC_T) kalMemAlloc(sizeof(HIF_TX_DESC_T) * size, VIR_MEM_TYPE);
		if (prTxDesc == NULL) {
			DBGLOG(TX, WARN, "wlanDebugHifDescriptorDump prTxDesc alloc fail!\n");
			return;
		}
		kalMemZero(prTxDesc, sizeof(HIF_TX_DESC_T) * size);
		wlanDebugHifDescriptorRecord(prAdapter, type, tcIndex, (PUINT_8)prTxDesc);
		wlanDebugHifDescriptorPrint(prAdapter, type, tcIndex, (PUINT_8)prTxDesc);
		kalMemFree(prTxDesc, VIR_MEM_TYPE, sizeof(HIF_TX_DESC_T));

	} else if (type == MTK_AMPDU_RX_DESC) {

		prRxDesc = (P_HIF_RX_DESC_T) kalMemAlloc(sizeof(HIF_RX_DESC_T) * size, VIR_MEM_TYPE);
		if (prRxDesc == NULL) {
			DBGLOG(RX, WARN, "wlanDebugHifDescriptorDump prRxDesc alloc fail!\n");
			return;
		}
		kalMemZero(prRxDesc, sizeof(HIF_RX_DESC_T) * size);
		wlanDebugHifDescriptorRecord(prAdapter, type, tcIndex, (PUINT_8)prRxDesc);
		wlanDebugHifDescriptorPrint(prAdapter, type, tcIndex, (PUINT_8)prRxDesc);
		kalMemFree(prRxDesc, VIR_MEM_TYPE, sizeof(P_HIF_RX_DESC_T));
	}
}
VOID wlanDebugScanRecord(P_ADAPTER_T prAdapter, ENUM_DBG_SCAN_T recordType)
{

	UINT_32 tcIndex = DEBUG_TC4_INDEX;
	UINT_32 type = MTK_AMPDU_TX_DESC;
	P_TX_CTRL_T pTxCtrl = &prAdapter->rTxCtrl;

	if (recordType == DBG_SCAN_WRITE_BEFORE) {
		wlanDebugHifDescriptorRecord(prAdapter, type, tcIndex
		, (PUINT_8)grScanHifDescRecord.pTxDescScanWriteBefore);
		grScanHifDescRecord.aucFreeBufCntScanWriteBefore = pTxCtrl->rTc.aucFreeBufferCount[TC4_INDEX];
		grScanHifDescRecord.u8ScanWriteBeforeTime = sched_clock();
	} else if (recordType == DBG_SCAN_WRITE_DONE) {
		wlanDebugHifDescriptorRecord(prAdapter, type, tcIndex
		, (PUINT_8)grScanHifDescRecord.pTxDescScanWriteDone);
		grScanHifDescRecord.aucFreeBufCntScanWriteDone = pTxCtrl->rTc.aucFreeBufferCount[TC4_INDEX];
		grScanHifDescRecord.u8ScanWriteDoneTime = sched_clock();
	}

}
VOID wlanDebugScanDump(P_ADAPTER_T prAdapter)
{
	UINT_32 tcIndex = DEBUG_TC4_INDEX;
	UINT_32 type = MTK_AMPDU_TX_DESC;

	DBGLOG(TX, INFO, "ScanReq hal write before:Time=%llu ,freeCnt=%d,dump tc4[0]~[3] desc!\n"
		, grScanHifDescRecord.u8ScanWriteBeforeTime
		, grScanHifDescRecord.aucFreeBufCntScanWriteBefore);
	wlanDebugHifDescriptorPrint(prAdapter, type, tcIndex
		, (PUINT_8)grScanHifDescRecord.pTxDescScanWriteBefore);

	DBGLOG(TX, INFO, "ScanReq hal write done:Time=%llu ,freeCnt=%d,dump tc4[0]~[3] desc!\n"
		, grScanHifDescRecord.u8ScanWriteDoneTime
		, grScanHifDescRecord.aucFreeBufCntScanWriteDone);
	wlanDebugHifDescriptorPrint(prAdapter, type, tcIndex
		, (PUINT_8)grScanHifDescRecord.pTxDescScanWriteDone);
}


VOID wlanReadFwStatus(P_ADAPTER_T prAdapter)
{
	static UINT_16 u2CurEntryCmd;
	P_COMMAND_ENTRY prCurCommand = &gprCommandEntry[u2CurEntryCmd];
	UINT_8 i = 0;
	GL_HIF_INFO_T *prHifInfo = &prAdapter->prGlueInfo->rHifInfo;

	prCurCommand->u8ReadFwTime = sched_clock();
	HAL_MCR_RD(prAdapter, MCR_D2HRM2R, &prCurCommand->u4ReadFwValue);
	for (i = 0; i < READ_CPUPCR_MAX_NUM; i++)
		prCurCommand->arCpupcrValue[i] = MCU_REG_READL(prHifInfo, CONN_MCU_CPUPCR);
	u2CurEntryCmd++;
	if (u2CurEntryCmd == TXED_COMMAND_BUF_MAX_NUM)
		u2CurEntryCmd = 0;
}


VOID wlanTraceTxCmd(P_ADAPTER_T prAdapter, P_CMD_INFO_T prCmd)
{
	static UINT_16 u2CurEntry;
	static UINT_16 u2CurEntryCmd;
	P_CMD_TRACE_ENTRY prCurCmd = &gprCmdTraceEntry[u2CurEntry];
	P_COMMAND_ENTRY prCurCommand = &gprCommandEntry[u2CurEntryCmd];

	prCurCmd->u8TxTime = sched_clock();
	prCurCommand->u8TxTime = prCurCmd->u8TxTime;
	prCurCmd->eCmdType = prCmd->eCmdType;
	if (prCmd->eCmdType == COMMAND_TYPE_MANAGEMENT_FRAME) {
		P_WLAN_MAC_MGMT_HEADER_T prMgmt = (P_WLAN_MAC_MGMT_HEADER_T)((P_MSDU_INFO_T)prCmd->prPacket)->prPacket;

		prCurCmd->u.rMgmtFrame.u2FrameCtl = prMgmt->u2FrameCtrl;
		prCurCmd->u.rMgmtFrame.u2DurationID = prMgmt->u2Duration;
	} else if (prCmd->eCmdType == COMMAND_TYPE_SECURITY_FRAME) {
		PUINT_8 pucPkt = (PUINT_8)((struct sk_buff *)prCmd->prPacket)->data;

		prCurCmd->u.rSecFrame.u2EthType =
				(pucPkt[ETH_TYPE_LEN_OFFSET] << 8) | (pucPkt[ETH_TYPE_LEN_OFFSET + 1]);
	} else {
		prCurCmd->u.rCmd.ucCID = prCmd->ucCID;
		prCurCmd->u.rCmd.ucCmdSeqNum = prCmd->ucCmdSeqNum;
		prCurCmd->u.rCmd.fgNeedResp = prCmd->fgNeedResp;
		prCurCmd->u.rCmd.fgSetQuery = prCmd->fgSetQuery;

		prCurCommand->rCmd.ucCID = prCmd->ucCID;
		prCurCommand->rCmd.ucCmdSeqNum = prCmd->ucCmdSeqNum;
		prCurCommand->rCmd.fgNeedResp = prCmd->fgNeedResp;
		prCurCommand->rCmd.fgSetQuery = prCmd->fgSetQuery;

		prCurCommand->u2Counter = u2CurEntryCmd;
		u2CurEntryCmd++;
		if (u2CurEntryCmd == TXED_COMMAND_BUF_MAX_NUM)
			u2CurEntryCmd = 0;
		HAL_MCR_RD(prAdapter, MCR_D2HRM2R, &prCurCommand->u4RelCID);
	}
	/*for every cmd record FW mailbox*/
	HAL_MCR_RD(prAdapter, MCR_D2HRM2R, &prCurCmd->u4RelCID);

	u2CurEntry++;
	if (u2CurEntry == TC_RELEASE_TRACE_BUF_MAX_NUM)
		u2CurEntry = 0;
}

VOID wlanTraceReleaseTcRes(P_ADAPTER_T prAdapter, PUINT_8 aucTxRlsCnt, UINT_8 ucAvailable)
{
	static UINT_16 u2CurEntry;
	P_TC_RES_RELEASE_ENTRY prCurBuf = &gprTcReleaseTraceBuffer[u2CurEntry];

	HAL_MCR_RD(prAdapter, MCR_D2HRM2R, &prCurBuf->u4RelCID);
	prCurBuf->u8RelaseTime = sched_clock();
	prCurBuf->ucTc4RelCnt = aucTxRlsCnt[TC4_INDEX];
	prCurBuf->ucAvailableTc4 = ucAvailable;
	u2CurEntry++;
	if (u2CurEntry == TXED_CMD_TRACE_BUF_MAX_NUM)
		u2CurEntry = 0;
}
VOID wlanDumpTxReleaseCount(P_ADAPTER_T prAdapter)
{
	UINT_32 au4WTSR[2];

	HAL_READ_TX_RELEASED_COUNT(prAdapter, au4WTSR);
	DBGLOG(TX, INFO, "WTSR[1]=%d, WTSR[0]=%d\n", au4WTSR[1], au4WTSR[0]);
}

VOID wlanDumpTcResAndTxedCmd(PUINT_8 pucBuf, UINT_32 maxLen)
{
	UINT_16 i = 0;
	P_CMD_TRACE_ENTRY prCmd = gprCmdTraceEntry;
	P_TC_RES_RELEASE_ENTRY prTcRel = gprTcReleaseTraceBuffer;

	if (pucBuf) {
		int bufLen = 0;

		for (; i < TXED_CMD_TRACE_BUF_MAX_NUM/2; i++) {
			bufLen = snprintf(pucBuf, maxLen,
				"%2d: Time %llu, Type %d, Content %08x, RelCID:%08x; %d: Time %llu, Type %d, Content %08x, RelCID:%08x\n",
				i*2, prCmd[i*2].u8TxTime, prCmd[i*2].eCmdType, *(PUINT_32)(&prCmd[i*2].u.rCmd.ucCID),
				prCmd[i*2].u4RelCID,
				i*2+1, prCmd[i*2+1].u8TxTime, prCmd[i*2+1].eCmdType,
				*(PUINT_32)(&prCmd[i*2+1].u.rCmd.ucCID),
				prCmd[i*2+1].u4RelCID);
			if (bufLen <= 0 || (UINT_32)bufLen >= maxLen)
				break;
			pucBuf += bufLen;
			maxLen -= bufLen;
		}
		for (i = 0; i < TC_RELEASE_TRACE_BUF_MAX_NUM/2; i++) {
			bufLen = snprintf(pucBuf, maxLen,
				"%2d: Time %llu, Tc4Cnt %d, Free %d, CID %08x; %2d: Time %llu, Tc4Cnt %d, Free %d CID %08x\n",
				i*2, prTcRel[i*2].u8RelaseTime, prTcRel[i*2].ucTc4RelCnt, prTcRel[i*2].ucAvailableTc4,
				prTcRel[i*2].u4RelCID,
				i*2+1, prTcRel[i*2+1].u8RelaseTime, prTcRel[i*2+1].ucTc4RelCnt,
				prTcRel[i*2+1].ucAvailableTc4, prTcRel[i*2+1].u4RelCID);
			if (bufLen <= 0 || (UINT_32)bufLen >= maxLen)
				break;
			pucBuf += bufLen;
			maxLen -= bufLen;
		}
		return;
	}
	for (; i < TXED_CMD_TRACE_BUF_MAX_NUM/4; i++) {
		DBGLOG(TX, INFO,
			"%2d: Time %llu, Type %d, Content %08x, RelCID:%08x; %2d: Time %llu, Type %d, Content %08x, RelCID:%08x\n",
			i*2, prCmd[i*2].u8TxTime, prCmd[i*2].eCmdType, *(PUINT_32)(&prCmd[i*2].u.rCmd.ucCID),
			prCmd[i*2].u4RelCID,
			i*2+1, prCmd[i*2+1].u8TxTime, prCmd[i*2+1].eCmdType,
			*(PUINT_32)(&prCmd[i*2+1].u.rCmd.ucCID),
			prCmd[i*2+1].u4RelCID);
		DBGLOG(TX, INFO,
			"%2d: Time %llu, Type %d, Content %08x, RelCID:%08x; %2d: Time %llu, Type %d, Content %08x, RelCID:%08x\n",
			i*4+2, prCmd[i*4+2].u8TxTime, prCmd[i*4+2].eCmdType,
			*(PUINT_32)(&prCmd[i*4+2].u.rCmd.ucCID), prCmd[i*4+2].u4RelCID,
			i*4+3, prCmd[i*4+3].u8TxTime, prCmd[i*4+3].eCmdType,
			*(PUINT_32)(&prCmd[i*4+3].u.rCmd.ucCID),
			prCmd[i*4+3].u4RelCID);
	}
	for (i = 0; i < TC_RELEASE_TRACE_BUF_MAX_NUM/4; i++) {
		DBGLOG(TX, INFO,
			"%2d: Time %llu, Tc4Cnt %d, Free %d, CID %08x; %2d: Time %llu, Tc4Cnt %d, Free %d, CID %08x;",
			i*4, prTcRel[i*4].u8RelaseTime, prTcRel[i*4].ucTc4RelCnt,
			prTcRel[i*4].ucAvailableTc4, prTcRel[i*4].u4RelCID,
			i*4+1, prTcRel[i*4+1].u8RelaseTime, prTcRel[i*4+1].ucTc4RelCnt,
			prTcRel[i*4+1].ucAvailableTc4, prTcRel[i*4+1].u4RelCID);
		DBGLOG(TX, INFO,
			"%2d: Time %llu, Tc4Cnt %d, Free %d, CID %08x; %2d: Time %llu, Tc4Cnt %d, Free %d, CID %08x\n",
			i*4+2, prTcRel[i*4+2].u8RelaseTime, prTcRel[i*4+2].ucTc4RelCnt,
			prTcRel[i*4+2].ucAvailableTc4, prTcRel[i*4+2].u4RelCID,
			i*4+3, prTcRel[i*4+3].u8RelaseTime, prTcRel[i*4+3].ucTc4RelCnt,
			prTcRel[i*4+3].ucAvailableTc4, prTcRel[i*4+3].u4RelCID);
	}
}
VOID wlanDumpCommandFwStatus(VOID)
{
	UINT_16 i = 0;
	P_COMMAND_ENTRY prCmd = gprCommandEntry;

	LOG_FUNC("Start\n");
	for (; i < TXED_COMMAND_BUF_MAX_NUM; i++) {
		LOG_FUNC(
		"%d: Time %llu,Content %08x,Count %x,RelCID %08x,FwValue %08x,Time %llu,CPUPCR 0x%08x 0x%08x 0x%08x\n",
			i, prCmd[i].u8TxTime, *(PUINT_32)(&prCmd[i].rCmd.ucCID),
			prCmd[i].u2Counter, prCmd[i].u4RelCID,
			prCmd[i].u4ReadFwValue, prCmd[i].u8ReadFwTime,
			prCmd[i].arCpupcrValue[0], prCmd[i].arCpupcrValue[1], prCmd[i].arCpupcrValue[2]);
	}
}

VOID wlanFWDLDebugInit(VOID)
{
	u4FWDL_packet_count = -1;
	gprFWDLDebug = (P_FWDL_DEBUG_T) kalMemAlloc(sizeof(FWDL_DEBUG_T)*MAX_FW_IMAGE_PACKET_COUNT,
			VIR_MEM_TYPE);

	if (gprFWDLDebug)
		kalMemZero(gprFWDLDebug, sizeof(FWDL_DEBUG_T)*MAX_FW_IMAGE_PACKET_COUNT);
	else
		DBGLOG(INIT, ERROR, "wlanFWDLDebugInit alloc memory error\n");
}

VOID wlanFWDLDebugAddTxStartTime(UINT_32 u4TxStartTime)
{
	if ((gprFWDLDebug != NULL) && (u4FWDL_packet_count < MAX_FW_IMAGE_PACKET_COUNT))
		(*(gprFWDLDebug+u4FWDL_packet_count)).u4TxStartTime = u4TxStartTime;
}

VOID wlanFWDLDebugAddTxDoneTime(UINT_32 u4TxDoneTime)
{
	if ((gprFWDLDebug != NULL) && (u4FWDL_packet_count < MAX_FW_IMAGE_PACKET_COUNT))
		(*(gprFWDLDebug+u4FWDL_packet_count)).u4TxDoneTime = u4TxDoneTime;
}

VOID wlanFWDLDebugAddRxStartTime(UINT_32 u4RxStartTime)
{
	if ((gprFWDLDebug != NULL) && (u4FWDL_packet_count < MAX_FW_IMAGE_PACKET_COUNT))
		(*(gprFWDLDebug+u4FWDL_packet_count)).u4RxStartTime = u4RxStartTime;
}

VOID wlanFWDLDebugAddRxDoneTime(UINT_32 u4RxDoneTime)
{
	if ((gprFWDLDebug != NULL) && (u4FWDL_packet_count < MAX_FW_IMAGE_PACKET_COUNT))
		(*(gprFWDLDebug+u4FWDL_packet_count)).u4RxDoneTime = u4RxDoneTime;
}

VOID wlanFWDLDebugStartSectionPacketInfo(UINT_32 u4Section, UINT_32 u4DownloadSize,
	UINT_32 u4ResponseTime)
{
	u4FWDL_packet_count++;
	if ((gprFWDLDebug != NULL) && (u4FWDL_packet_count < MAX_FW_IMAGE_PACKET_COUNT)) {
		(*(gprFWDLDebug+u4FWDL_packet_count)).u4Section = u4Section;
		(*(gprFWDLDebug+u4FWDL_packet_count)).u4DownloadSize = u4DownloadSize;
		(*(gprFWDLDebug+u4FWDL_packet_count)).u4ResponseTime = u4ResponseTime;
	}
}

VOID wlanFWDLDebugDumpInfo(VOID)
{
	UINT_32 i;

	for (i = 0; i < u4FWDL_packet_count; i++) {
		/* Tx:[TxStartTime][TxDoneTime]
		*	Pkt:[DL Pkt Section][DL Pkt Size][DL Pkt Resp Time]
		*/
		DBGLOG(INIT, WARN, "wlanFWDLDumpLog > Tx:[%u][%u] Rx:[%u][%u] Pkt:[%d][%d][%u]\n"
		, (*(gprFWDLDebug+i)).u4TxStartTime, (*(gprFWDLDebug+i)).u4TxDoneTime
		, (*(gprFWDLDebug+i)).u4RxStartTime, (*(gprFWDLDebug+i)).u4RxDoneTime
		, (*(gprFWDLDebug+i)).u4Section, (*(gprFWDLDebug+i)).u4DownloadSize
		, (*(gprFWDLDebug+i)).u4ResponseTime);
	}
}

VOID wlanFWDLDebugUninit(VOID)
{
	kalMemFree(gprFWDLDebug, VIR_MEM_TYPE, sizeof(FWDL_DEBUG_T)*MAX_FW_IMAGE_PACKET_COUNT);
	gprFWDLDebug = NULL;
	u4FWDL_packet_count = -1;
}

