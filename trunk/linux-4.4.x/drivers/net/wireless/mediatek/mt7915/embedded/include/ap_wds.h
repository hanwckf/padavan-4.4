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
    ap_wds.h

    Abstract:
    Support WDS function.

    Revision History:
    Who       When            What
    ------    ----------      ----------------------------------------------
    Fonchi    02-13-2007      created
*/


#ifndef _AP_WDS_H_
#define _AP_WDS_H_

#define WDS_ENTRY_RETRY_INTERVAL	(100 * OS_HZ / 1000)

/* flags description
 * ASSIGNED: WDS entry config is fed from config file
 * VALID   : WDS entry has finished peer establishment
 */
enum wds_flags {
	ASSIGNED = 1,
	VALID    = (1 << 1),
};

#define WDS_ENTRY_IS_ASSIGNED(flag)     ((flag) & ASSIGNED)
#define WDS_ENTRY_IS_VALID(flag)        ((flag) & VALID)
#define WDS_ENTRY_SET_ASSIGNED(flag)    ((flag) |= ASSIGNED)
#define WDS_ENTRY_SET_VALID(flag)       ((flag) |= VALID)

static inline BOOLEAN WDS_IF_UP_CHECK(
	IN  PRTMP_ADAPTER   pAd,
	IN  UCHAR band_idx,
	IN  ULONG ifidx)
{
	if ((pAd->WdsTab.flg_wds_init[band_idx] != TRUE) ||
		(ifidx >= MAX_WDS_ENTRY)) {
		MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s[band%d]WDS forbidden!\n", __func__, band_idx));
		return FALSE;
	}

	/*	if(RTMP_OS_NETDEV_STATE_RUNNING(pAd->WdsTab.WdsEntry[ifidx].dev)) */
	/* Patch for wds ,when dirver call apmlmeperiod => APMlmeDynamicTxRateSwitching check if wds device ready */
	if ((pAd->WdsTab.WdsEntry[ifidx].wdev.if_dev != NULL) && (RTMP_OS_NETDEV_STATE_RUNNING(pAd->WdsTab.WdsEntry[ifidx].wdev.if_dev))) {
		return TRUE;
	}

	return FALSE;
}

INT WdsEntryAlloc(
	IN PRTMP_ADAPTER pAd,
	IN UCHAR band_idx,
	IN PUCHAR pAddr);

VOID WdsEntryDel(
	IN PRTMP_ADAPTER pAd,
	IN PUCHAR pAddr);

MAC_TABLE_ENTRY *MacTableInsertWDSEntry(
	IN  PRTMP_ADAPTER   pAd,
	IN  PUCHAR pAddr,
	UINT WdsTabIdx);

MAC_TABLE_ENTRY *WdsTableLookupByWcid(
	IN  PRTMP_ADAPTER   pAd,
	IN UINT16 wcid,
	IN PUCHAR pAddr,
	IN BOOLEAN bResetIdelCount);

MAC_TABLE_ENTRY *WdsTableLookup(
	IN  PRTMP_ADAPTER   pAd,
	IN  PUCHAR          pAddr,
	IN BOOLEAN bResetIdelCount);

MAC_TABLE_ENTRY *FindWdsEntry(
	IN PRTMP_ADAPTER	pAd,
	IN struct _RX_BLK *pRxBlk);

VOID WdsTableMaintenance(
	IN PRTMP_ADAPTER    pAd,
	IN UCHAR band_idx);

VOID AsicUpdateWdsEncryption(
	IN PRTMP_ADAPTER pAd,
	IN UINT16 wcid);

VOID WdsPeerBeaconProc(
	IN PRTMP_ADAPTER pAd,
	IN PMAC_TABLE_ENTRY pEntry,
	IN UCHAR MaxSupportedRateIn500Kbps,
	IN UCHAR MaxSupportedRateLen,
	IN BCN_IE_LIST * ie_list);

VOID APWdsInitialize(RTMP_ADAPTER *pAd);

INT	Show_WdsTable_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT profile_wds_reg(
	IN PRTMP_ADAPTER ad,
	IN UCHAR band_idx,
	IN RTMP_STRING *buffer);

VOID rtmp_read_wds_from_file(
	IN  PRTMP_ADAPTER pAd,
	RTMP_STRING *tmpbuf,
	RTMP_STRING *buffer);

VOID WDS_Init(RTMP_ADAPTER *pAd, UCHAR band_idx, RTMP_OS_NETDEV_OP_HOOK *pNetDevOps);
VOID WDS_Remove(RTMP_ADAPTER *pAd);
BOOLEAN WDS_StatsGet(RTMP_ADAPTER *pAd, RT_CMD_STATS *pStats);
VOID AP_WDS_KeyNameMakeUp(RTMP_STRING *pKey, UINT32 KeyMaxSize, INT KeyId);

INT wds_tx_pkt_allowed(
	RTMP_ADAPTER *pAd,
	struct wifi_dev *wdev,
	PNDIS_PACKET pkt);

INT wds_rx_foward_handle(RTMP_ADAPTER *pAd, struct wifi_dev *wdev, PNDIS_PACKET pPacket);

/*
	==========================================================================
	Description:
		Check the WDS Entry is valid or not.
	==========================================================================
 */

BOOLEAN wds_entry_is_valid(
	struct _RTMP_ADAPTER *ad,
	UCHAR wds_index);

INT wds_inf_open(struct wifi_dev *wdev);
INT wds_inf_close(struct wifi_dev *wdev);

BOOLEAN WdsMsgTypeSubst(
	IN PRTMP_ADAPTER  pAd,
	IN PFRAME_802_11 pFrame,
	OUT PINT Machine,
	OUT PINT MsgType);

VOID ap_wds_rcv_uc_data_action(
	IN struct _RTMP_ADAPTER *pAd,
	IN struct _MLME_QUEUE_ELEM *Elem);

VOID ap_wds_wdev_linkdown(
	IN struct _RTMP_ADAPTER *pAd,
	IN struct wifi_dev *wdev,
	IN USHORT wcid);

VOID ap_wds_bss_linkdown(
	IN struct _RTMP_ADAPTER *pAd,
	IN struct _MLME_QUEUE_ELEM *Elem);

/* ap_wds.c */
VOID WdsStateMachineInit(
	IN PRTMP_ADAPTER pAd,
	IN STATE_MACHINE * Sm,
	OUT STATE_MACHINE_FUNC Trans[]);

#endif /* _AP_WDS_H_ */

