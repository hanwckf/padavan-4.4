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
 * Id: stats.c#1
 */

/*
 * ! \file stats.c
 *  \brief This file includes statistics support.
 */

/*******************************************************************************
 *						C O M P I L E R	 F L A G S
 ********************************************************************************
 */

/*******************************************************************************
 *						E X T E R N A L	R E F E R E N C E S
 ********************************************************************************
 */
#include "precomp.h"

#if (CFG_SUPPORT_STATISTICS == 1)

enum EVENT_TYPE {
	EVENT_RX,
	EVENT_TX,
};
/*******************************************************************************
*						C O N S T A N T S
********************************************************************************
*/

/*******************************************************************************
*						F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

/*******************************************************************************
*						P U B L I C   D A T A
********************************************************************************
*/

/*******************************************************************************
*						P R I V A T E  F U N C T I O N S
********************************************************************************
*/

UINT_32 u4TotalTx;
UINT_32 u4NoDelayTx;
UINT_32 u4TotalRx;
UINT_32 u4NoDelayRx;

static UINT_8 g_ucTxRxFlag;
static UINT_8 g_ucTxIpProto;
static UINT_16 g_u2TxUdpPort;
static UINT_32 g_u4TxDelayThreshold;
static UINT_8 g_ucRxIpProto;
static UINT_16 g_u2RxUdpPort;
static UINT_32 g_u4RxDelayThreshold;

VOID StatsResetTxRx(VOID)
{
	u4TotalRx = 0;
	u4TotalTx = 0;
	u4NoDelayRx = 0;
	u4NoDelayTx = 0;
}

UINT_64 StatsEnvTimeGet(VOID)
{
	UINT_64 u8Clk;

	u8Clk = sched_clock();	/* unit: naro seconds */

	return (UINT_64) u8Clk;	/* sched_clock *//* jiffies size = 4B */
}

VOID StatsEnvGetPktDelay(OUT PUINT_8 pucTxRxFlag, OUT PUINT_8 pucTxIpProto, OUT PUINT_16 pu2TxUdpPort,
	OUT PUINT_32 pu4TxDelayThreshold, OUT PUINT_8 pucRxIpProto,
	OUT PUINT_16 pu2RxUdpPort, OUT PUINT_32 pu4RxDelayThreshold)
{
	*pucTxRxFlag = g_ucTxRxFlag;
	*pucTxIpProto = g_ucTxIpProto;
	*pu2TxUdpPort = g_u2TxUdpPort;
	*pu4TxDelayThreshold = g_u4TxDelayThreshold;
	*pucRxIpProto = g_ucRxIpProto;
	*pu2RxUdpPort = g_u2RxUdpPort;
	*pu4RxDelayThreshold = g_u4RxDelayThreshold;
}

VOID StatsEnvSetPktDelay(IN UINT_8 ucTxOrRx, IN UINT_8 ucIpProto, IN UINT_16 u2UdpPort, UINT_32 u4DelayThreshold)
{
#define MODULE_RESET 0
#define MODULE_TX 1
#define MODULE_RX 2

	if (ucTxOrRx == MODULE_TX) {
		g_ucTxRxFlag |= BIT(0);
		g_ucTxIpProto = ucIpProto;
		g_u2TxUdpPort = u2UdpPort;
		g_u4TxDelayThreshold = u4DelayThreshold;
	} else if (ucTxOrRx == MODULE_RX) {
		g_ucTxRxFlag |= BIT(1);
		g_ucRxIpProto = ucIpProto;
		g_u2RxUdpPort = u2UdpPort;
		g_u4RxDelayThreshold = u4DelayThreshold;
	} else if (ucTxOrRx == MODULE_RESET) {
		g_ucTxRxFlag = 0;
		g_ucTxIpProto = 0;
		g_u2TxUdpPort = 0;
		g_u4TxDelayThreshold = 0;
		g_ucRxIpProto = 0;
		g_u2RxUdpPort = 0;
		g_u4RxDelayThreshold = 0;
	}
}

VOID StatsEnvRxTime2Host(IN P_ADAPTER_T prAdapter, struct sk_buff *prSkb)
{
	PUINT_8 pucEth = prSkb->data;
	UINT_16 u2EthType = 0;
	UINT_8 ucIpVersion = 0;
	UINT_8 ucIpProto = 0;
	UINT_16 u2IPID = 0;
	UINT_16 u2UdpDstPort = 0;
	UINT_16 u2UdpSrcPort = 0;
	UINT_64 u8IntTime = 0;
	UINT_64 u8RxTime = 0;
	UINT_32 u4Delay = 0;
	struct timeval tval;
	struct rtc_time tm;

	if ((g_ucTxRxFlag & BIT(1)) == 0)
		return;

	if (prSkb->len <= 24 + ETH_HLEN)
		return;
	u2EthType = (pucEth[ETH_TYPE_LEN_OFFSET] << 8) | (pucEth[ETH_TYPE_LEN_OFFSET + 1]);
	pucEth += ETH_HLEN;
	if (u2EthType != ETH_P_IPV4)
		return;
	ucIpProto = pucEth[9];
	if (g_ucRxIpProto && (ucIpProto != g_ucRxIpProto))
		return;
	ucIpVersion = (pucEth[0] & IPVH_VERSION_MASK) >> IPVH_VERSION_OFFSET;
	if (ucIpVersion != IPVERSION)
		return;
	u2IPID = pucEth[4] << 8 | pucEth[5];
	u8IntTime = GLUE_RX_GET_PKT_INT_TIME(prSkb);
	u4Delay = ((UINT_32)(sched_clock() - u8IntTime))/NSEC_PER_USEC;
	u8RxTime = GLUE_RX_GET_PKT_RX_TIME(prSkb);
	do_gettimeofday(&tval);
	rtc_time_to_tm(tval.tv_sec, &tm);

	switch (ucIpProto) {
	case IP_PRO_TCP:
	case IP_PRO_UDP:
		u2UdpSrcPort = (pucEth[20] << 8) | pucEth[21];
		u2UdpDstPort = (pucEth[22] << 8) | pucEth[23];
		if (g_u2RxUdpPort && (u2UdpSrcPort != g_u2RxUdpPort))
			break;
	case IP_PRO_ICMP:
		u4TotalRx++;
		if (g_u4RxDelayThreshold && (u4Delay <= g_u4RxDelayThreshold)) {
			u4NoDelayRx++;
			break;
		}
		DBGLOG(RX, INFO,
	"IPID 0x%04x src %d dst %d UP %d,delay %u us,int2rx %lu us,IntTime %llu,%u/%u,leave at %02d:%02d:%02d.%06ld\n",
			u2IPID, u2UdpSrcPort, u2UdpDstPort, ((pucEth[1] & IPTOS_PREC_MASK) >> IPTOS_PREC_OFFSET),
			u4Delay, ((UINT_32)(u8RxTime - u8IntTime))/NSEC_PER_USEC, u8IntTime, u4NoDelayRx, u4TotalRx,
			tm.tm_hour, tm.tm_min, tm.tm_sec, tval.tv_usec);
		break;
	default:
		break;
	}
}

VOID StatsEnvTxTime2Hif(IN P_ADAPTER_T prAdapter, IN P_MSDU_INFO_T prMsduInfo)
{
	UINT_64 u8SysTime, u8SysTimeIn;
	UINT32 u4TimeDiff;
	PUINT_8 pucAheadBuf = ((struct sk_buff *)prMsduInfo->prPacket)->data;
	UINT_32 u4PacketLen = ((struct sk_buff *)prMsduInfo->prPacket)->len;
	UINT_8 ucIpVersion = 0;
	UINT_8 ucIpProto = 0;
	PUINT_8 pucEthBody = NULL;
	UINT_16 u2EthType = 0;
	UINT_16 u2IPID = 0;
	UINT_16 u2UdpDstPort = 0;
	UINT_16 u2UdpSrcPort = 0;

	u8SysTime = StatsEnvTimeGet();
	u8SysTimeIn = GLUE_GET_PKT_XTIME(prMsduInfo->prPacket);

	if ((g_ucTxRxFlag & BIT(0)) == 0)
		return;

	if ((u8SysTimeIn == 0) || (u8SysTime <= u8SysTimeIn))
		return;

	/* units of u4TimeDiff is micro seconds (us) */
	if (u4PacketLen < 24 + ETH_HLEN)
		return;
	u2EthType = (pucAheadBuf[ETH_TYPE_LEN_OFFSET] << 8) | (pucAheadBuf[ETH_TYPE_LEN_OFFSET + 1]);
	pucEthBody = &pucAheadBuf[ETH_HLEN];
	if (u2EthType != ETH_P_IPV4)
		return;
	ucIpProto = pucEthBody[9];
	if (g_ucTxIpProto && (ucIpProto != g_ucTxIpProto))
		return;
	ucIpVersion = (pucEthBody[0] & IPVH_VERSION_MASK) >> IPVH_VERSION_OFFSET;
	if (ucIpVersion != IPVERSION)
		return;
	u2IPID = pucEthBody[4]<<8 | pucEthBody[5];
	u8SysTime = u8SysTime - u8SysTimeIn;
	u4TimeDiff = (UINT32) u8SysTime;
	u4TimeDiff = u4TimeDiff / 1000;	/* ns to us */

	switch (ucIpProto) {
	case IP_PRO_TCP:
	case IP_PRO_UDP:
		u2UdpDstPort = (pucEthBody[22] << 8) | pucEthBody[23];
		u2UdpSrcPort = (pucEthBody[20] << 8) | pucEthBody[21];
		if (g_u2TxUdpPort && (u2UdpDstPort != g_u2TxUdpPort))
			break;
	case IP_PRO_ICMP:
		u4TotalTx++;
		if (g_u4TxDelayThreshold && (u4TimeDiff <= g_u4TxDelayThreshold)) {
			u4NoDelayTx++;
			break;
		}
		DBGLOG(TX, INFO,
			"IPID 0x%04x src %d dst %d UP %d,delay %u us,u8SysTimeIn %llu, %u/%u\n",
			u2IPID, u2UdpSrcPort, u2UdpDstPort,
			((pucEthBody[1] & IPTOS_PREC_MASK) >> IPTOS_PREC_OFFSET),
			u4TimeDiff, u8SysTimeIn, u4NoDelayTx, u4TotalTx);
		break;
	default:
		break;
	}
}

static VOID
statsParsePktInfo(P_ADAPTER_T prAdapter, PUINT_8 pucPkt, struct sk_buff *skb, UINT_8 status, UINT_8 eventType)
{
	/* get ethernet protocol */
	UINT_16 u2EtherType = (pucPkt[ETH_TYPE_LEN_OFFSET] << 8) | (pucPkt[ETH_TYPE_LEN_OFFSET + 1]);
	PUINT_8 pucEthBody = &pucPkt[ETH_HLEN];

	switch (u2EtherType) {
	case ETH_P_ARP:
	{
		UINT_16 u2OpCode = (pucEthBody[6] << 8) | pucEthBody[7];

		switch (eventType) {
		case EVENT_RX:
			GLUE_SET_INDEPENDENT_PKT(skb, TRUE);
			if (u2OpCode == ARP_PRO_REQ)
				DBGLOG(RX, TRACE, "<RX> Arp Req From IP: %d.%d.%d.%d\n",
					pucEthBody[14], pucEthBody[15], pucEthBody[16], pucEthBody[17]);
			else if (u2OpCode == ARP_PRO_RSP)
				DBGLOG(RX, TRACE, "<RX> Arp Rsp from IP: %d.%d.%d.%d\n",
					pucEthBody[14], pucEthBody[15], pucEthBody[16], pucEthBody[17]);
			break;
		case EVENT_TX:
			if (u2OpCode == ARP_PRO_REQ)
				DBGLOG(TX, TRACE, "<TX> Arp Req to IP: %d.%d.%d.%d\n",
					pucEthBody[24], pucEthBody[25], pucEthBody[26], pucEthBody[27]);
			else if (u2OpCode == ARP_PRO_RSP)
				DBGLOG(TX, TRACE, "<TX> Arp Rsp to IP: %d.%d.%d.%d\n",
					pucEthBody[24], pucEthBody[25], pucEthBody[26], pucEthBody[27]);
			break;
		}
		break;
	}
	case ETH_P_IPV4:
	{
		UINT_8 ucIpProto = pucEthBody[9]; /* IP header without options */
		UINT_8 ucIpVersion = (pucEthBody[0] & IPVH_VERSION_MASK) >> IPVH_VERSION_OFFSET;
		UINT_16 u2IpId = *(UINT_16 *) &pucEthBody[4];

		if (ucIpVersion != IPVERSION)
			break;
		switch (ucIpProto) {
		case IP_PRO_ICMP:
		{
			/* the number of ICMP packets is seldom so we print log here */
			UINT_8 ucIcmpType;
			UINT_16 u2IcmpId, u2IcmpSeq;
			PUINT_8 pucIcmp = &pucEthBody[20];

			ucIcmpType = pucIcmp[0];
			if (ucIcmpType == 3) /* don't log network unreachable packet */
				break;
			u2IcmpId = *(UINT_16 *) &pucIcmp[4];
			u2IcmpSeq = *(UINT_16 *) &pucIcmp[6];
			switch (eventType) {
			case EVENT_RX:
				GLUE_SET_INDEPENDENT_PKT(skb, TRUE);
				DBGLOG(RX, TRACE, "<RX> ICMP: Type %d, Id BE 0x%04x, Seq BE 0x%04x\n",
							ucIcmpType, u2IcmpId, u2IcmpSeq);
				break;
			case EVENT_TX:
				DBGLOG(TX, TRACE, "<TX> ICMP: Type %d, Id 0x04%x, Seq BE 0x%04x\n",
								ucIcmpType, u2IcmpId, u2IcmpSeq);
				break;
			}
			break;
		}
		case IP_PRO_UDP:
		{
			/* the number of DHCP packets is seldom so we print log here */
			PUINT_8 pucUdp = &pucEthBody[20];
			PUINT_8 pucBootp = &pucUdp[8];
			UINT_16 u2UdpDstPort;
			UINT_16 u2UdpSrcPort;
			UINT_32 u4TransID;

			u2UdpDstPort = (pucUdp[2] << 8) | pucUdp[3];
			u2UdpSrcPort = (pucUdp[0] << 8) | pucUdp[1];
			if ((u2UdpDstPort == UDP_PORT_DHCPS) || (u2UdpDstPort == UDP_PORT_DHCPC)) {
			u4TransID = pucBootp[4]<<24  | pucBootp[5]<<16 | pucBootp[6]<<8  | pucBootp[7];
			switch (eventType) {
			case EVENT_RX:
					GLUE_SET_INDEPENDENT_PKT(skb, TRUE);
					DBGLOG(RX, INFO, "<RX> DHCP: IPID 0x%02x, MsgType 0x%x, TransID 0x%04x\n",
								u2IpId, pucBootp[0], u4TransID);
				break;
			case EVENT_TX:
					DBGLOG(TX, INFO, "<TX> DHCP: IPID 0x%02x, MsgType 0x%x, TransID 0x%04x\n",
								u2IpId, pucBootp[0], u4TransID);
				break;
				}
			} else if (u2UdpSrcPort == UDP_PORT_DNS) { /* tx dns */
				UINT_16 u2TransId = (pucBootp[0] << 8) | pucBootp[1];

				if (eventType == EVENT_RX) {
					DBGLOG(RX, INFO,
						"<RX> DNS: IPID 0x%02x, TransID 0x%04x\n", u2IpId, u2TransId);
				}
			}
			break;
		}
		}
		break;
	}
	case ETH_P_1X:
	{
		PUINT_8 pucEapol = pucEthBody;
		UINT_8 ucEapolType = pucEapol[1];
		UINT_8 ucAisBssIndex;

		switch (ucEapolType) {
		case 0: /* eap packet */
			switch (eventType) {
			case EVENT_RX:
				DBGLOG(RX, INFO, "<RX> EAP Packet: code %d, id %d, type %d\n",
						pucEapol[4], pucEapol[5], pucEapol[7]);
				if (pucEapol[4] != 0x4)
					break;
				ucAisBssIndex = prAdapter->prAisBssInfo->ucBssIndex;
				if (GLUE_GET_PKT_BSS_IDX(skb) == ucAisBssIndex)
					break;
				DBGLOG(RX, INFO, "<RX> P2P:WSC: EAP-FAILURE: code %d, id %d, type %d\n",
					pucEapol[4], pucEapol[5], pucEapol[7]);
				prAdapter->prP2pInfo->fgWaitEapFailure = FALSE;

				break;
			case EVENT_TX:
				DBGLOG(TX, INFO, "<TX> EAP Packet: code %d, id %d, type %d\n",
						pucEapol[4], pucEapol[5], pucEapol[7]);
				break;
			}
			break;
		case 1: /* eapol start */
			switch (eventType) {
			case EVENT_RX:
				DBGLOG(RX, INFO, "<RX> EAPOL: start\n");
				break;
			case EVENT_TX:
				DBGLOG(TX, INFO, "<TX> EAPOL: start\n");
				break;
			}
			break;
		case 3: /* key */
			switch (eventType) {
			case EVENT_RX:
				DBGLOG(RX, INFO, "<RX> EAPOL: key, KeyInfo 0x%04x\n",
						*((PUINT_16)(&pucEapol[5])));
				break;
			case EVENT_TX:
				DBGLOG(TX, INFO, "<TX> EAPOL: key, KeyInfo 0x%04x\n",
						*((PUINT_16)(&pucEapol[5])));
				break;
			}

			break;
		}
		break;
	}
	case ETH_WPI_1X:
	{
		UINT_8 ucSubType = pucEthBody[3]; /* sub type filed*/
		UINT_16 u2Length = *(PUINT_16)&pucEthBody[6];
		UINT_16 u2Seq = *(PUINT_16)&pucEthBody[8];

		switch (eventType) {
		case EVENT_RX:
			DBGLOG(RX, INFO, "<RX> WAPI: subType %d, Len %d, Seq %d\n",
					ucSubType, u2Length, u2Seq);
			break;
		case EVENT_TX:
			DBGLOG(TX, INFO, "<TX> WAPI: subType %d, Len %d, Seq %d\n",
					ucSubType, u2Length, u2Seq);
			break;
		}
		break;
	}
	case 0x890d:
		switch (eventType) {
		case EVENT_RX:
			DBGLOG(RX, INFO,
				"<RX> TDLS type %d, category %d, Action %d, Token %d\n",
				pucEthBody[0], pucEthBody[1], pucEthBody[2], pucEthBody[3]);
			break;
		case EVENT_TX:
			DBGLOG(TX, INFO,
				"<TX> TDLS type %d, category %d, Action %d, Token %d\n",
				pucEthBody[0], pucEthBody[1], pucEthBody[2], pucEthBody[3]);
			break;
		}
		break;
	}
}
/*----------------------------------------------------------------------------*/
/*! \brief  This routine is called to display rx packet information.
*
* \param[in] pPkt			Pointer to the packet
* \param[out] None
*
* \retval None
*/
/*----------------------------------------------------------------------------*/
VOID StatsRxPktInfoDisplay(P_ADAPTER_T prAdapter, P_SW_RFB_T prSwRfb)
{
	PUINT_8 pPkt = NULL;
	struct sk_buff *skb = NULL;

	if (prSwRfb->u2PacketLen <= ETHER_HEADER_LEN)
		return;

	pPkt = prSwRfb->pvHeader;
	if (!pPkt)
		return;

	skb = (struct sk_buff *)(prSwRfb->pvPacket);
	if (!skb)
		return;

	statsParsePktInfo(prAdapter, pPkt, skb, 0, EVENT_RX);
}

/*----------------------------------------------------------------------------*/
/*! \brief  This routine is called to display tx packet information.
*
* \param[in] pPkt			Pointer to the packet
* \param[out] None
*
* \retval None
*/
/*----------------------------------------------------------------------------*/
VOID StatsTxPktInfoDisplay(UINT_8 *pPkt)
{
	UINT_16 u2EtherTypeLen;

	u2EtherTypeLen = (pPkt[ETH_TYPE_LEN_OFFSET] << 8) | (pPkt[ETH_TYPE_LEN_OFFSET + 1]);
	statsParsePktInfo(NULL, pPkt, NULL, 0, EVENT_TX);
}

#endif /* CFG_SUPPORT_STATISTICS */

/* End of stats.c */
