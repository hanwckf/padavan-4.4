/*
 ***************************************************************************
 * Ralink Tech Inc.
 * 4F, No. 2 Technology	5th	Rd.
 * Science-based Industrial	Park
 * Hsin-chu, Taiwan, R.O.C.
 *
 * (c) Copyright 2002-2004, Ralink Technology, Inc.
 *
 * All rights reserved.	Ralink's source	code is	an unpublished work	and	the
 * use of a	copyright notice does not imply	otherwise. This	source code
 * contains	confidential trade secret material of Ralink Tech. Any attemp
 * or participation	in deciphering,	decoding, reverse engineering or in	any
 * way altering	the	source code	is stricitly prohibited, unless	the	prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************

	Module Name:
	cmm_asic_mt.c

	Abstract:
	Functions used to communicate with ASIC

	Revision History:
	Who			When			What
	--------	----------		----------------------------------------------
*/

#include "rt_config.h"
#include "hdev/hdev.h"

/* DEV Info */
INT32 MtAsicSetDevMacByFw(
	RTMP_ADAPTER * pAd,
	UINT8 OwnMacIdx,
	UINT8 *OwnMacAddr,
	UINT8 BandIdx,
	UINT8 Active,
	UINT32 EnableFeature)
{
	return CmdExtDevInfoUpdate(pAd,
							   OwnMacIdx,
							   OwnMacAddr,
							   BandIdx,
							   Active,
							   EnableFeature);
}


/* BSS Info */
INT32 MtAsicSetBssidByFw(
	struct _RTMP_ADAPTER *pAd,
	struct _BSS_INFO_ARGUMENT_T *bss_info_argument)
{
	return CmdExtBssInfoUpdate(pAd, bss_info_argument);
}

/* STARec Info */
INT32 MtAsicSetStaRecByFw(
	RTMP_ADAPTER * pAd,
	STA_REC_CFG_T StaCfg)
{
	return CmdExtStaRecUpdate(pAd, StaCfg);
}

INT32 MtAsicUpdateStaRecBaByFw(
	struct _RTMP_ADAPTER *pAd,
	STA_REC_BA_CFG_T StaRecBaCfg)
{
	return CmdExtStaRecBaUpdate(pAd, StaRecBaCfg);
}


VOID MtSetTmrCRByFw(struct _RTMP_ADAPTER *pAd, UCHAR enable, UCHAR BandIdx)
{
	CmdExtSetTmrCR(pAd, enable, BandIdx);
}


VOID AsicAutoBATrigger(struct _RTMP_ADAPTER *pAd, BOOLEAN Enable, UINT32 Timeout)
{
	CmdAutoBATrigger(pAd, Enable, Timeout);
}


VOID MtAsicDelWcidTabByFw(
	IN PRTMP_ADAPTER pAd,
	IN UINT16 wcid_idx)
{
	MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s, --->\n", __func__));
	MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s, wcid_idx(%d)\n", __func__, wcid_idx));

	if (wcid_idx == WCID_ALL)
		CmdExtWtblUpdate(pAd, 0, RESET_ALL_WTBL, NULL, 0);
	else
		CmdExtWtblUpdate(pAd, wcid_idx, RESET_WTBL_AND_SET, NULL, 0);

	MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s, <---\n", __func__));
}


#ifdef HTC_DECRYPT_IOT
/*
	Old Chip PATH (ex: MT7615 / MT7622 ) :
*/
VOID MtAsicSetWcidAAD_OMByFw(
	IN PRTMP_ADAPTER pAd,
	IN UINT16 wcid_idx,
	IN UCHAR value)
{
	UINT32 mask = 0xfffffff7;
	UINT32 val;

	MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s, wcid_idx(%d), value(%d)\n", __func__, wcid_idx, value));

	if (value) {
		val = 0x8;
		WtblDwSet(pAd, wcid_idx, 1, 2, mask, val);
	} else {
		val = 0x0;
		WtblDwSet(pAd, wcid_idx, 1, 2, mask, val);
	}

	MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s, <---\n", __func__));
}

/*
	CONNAC F/W CMD PATH:
*/
INT32 MtAsicUpdateStaRecAadOmByFw(
	IN PRTMP_ADAPTER pAd,
	IN UINT16 Wcid,
	IN UINT8 AadOm)
{
	return CmdExtStaRecAADOmUpdate(pAd, Wcid, AadOm);
}

#endif /* HTC_DECRYPT_IOT */

#if defined(MBSS_AS_WDS_AP_SUPPORT) || defined(APCLI_AS_WDS_STA_SUPPORT)
VOID MtAsicSetWcid4Addr_HdrTransByFw(
	IN PRTMP_ADAPTER pAd,
	IN UINT16 wcid_idx,
	IN UCHAR IsEnable,
	IN UCHAR IsApcliEntry)
{

	CMD_WTBL_HDR_TRANS_T	rWtblHdrTrans = {0};

	rWtblHdrTrans.u2Tag = WTBL_HDR_TRANS;
	rWtblHdrTrans.u2Length = sizeof(CMD_WTBL_HDR_TRANS_T);

	MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
						("%s: WCID %u ISenable %u\n",
						__FUNCTION__, wcid_idx, IsEnable));
	/*Set to 1 */
	if (IsEnable) {
		rWtblHdrTrans.ucTd = 1;
		rWtblHdrTrans.ucFd = 1;
	} else if (IsApcliEntry) {
		rWtblHdrTrans.ucTd = 1;
		rWtblHdrTrans.ucFd = 0;
	} else {
		rWtblHdrTrans.ucTd = 0;
		rWtblHdrTrans.ucFd = 1;
	}
	rWtblHdrTrans.ucDisRhtr = 0;
	CmdExtWtblUpdate(pAd, wcid_idx, SET_WTBL, &rWtblHdrTrans, sizeof(CMD_WTBL_HDR_TRANS_T));

}
#endif
VOID MtAsicUpdateRxWCIDTableByFw(
	IN PRTMP_ADAPTER pAd,
	IN MT_WCID_TABLE_INFO_T WtblInfo)
{
	NDIS_STATUS					Status = NDIS_STATUS_SUCCESS;
	UCHAR						*pTlvBuffer = NULL;
	UCHAR						*pTempBuffer = NULL;
	UINT32						u4TotalTlvLen = 0;
	UCHAR						ucTotalTlvNumber = 0;
	/* Tag = 0, Generic */
	CMD_WTBL_GENERIC_T		rWtblGeneric = {0};
	/* Tage = 1, Rx */
	CMD_WTBL_RX_T				rWtblRx = {0};
#ifdef DOT11_N_SUPPORT
	/* Tag = 2, HT */
	CMD_WTBL_HT_T				rWtblHt = {0};
#ifdef DOT11_VHT_AC
	/* Tag = 3, VHT */
	CMD_WTBL_VHT_T			rWtblVht = {0};
#endif /* DOT11_VHT_AC */
#endif /* DOT11_N_SUPPORT */
	/* Tag = 5, TxPs */
	CMD_WTBL_TX_PS_T			rWtblTxPs = {0};
#if defined(HDR_TRANS_TX_SUPPORT) || defined(HDR_TRANS_RX_SUPPORT)
	/* Tag = 6, Hdr Trans */
	CMD_WTBL_HDR_TRANS_T	rWtblHdrTrans = {0};
#endif /* HDR_TRANS_TX_SUPPORT */
	/* Tag = 7, Security Key */
	CMD_WTBL_SECURITY_KEY_T	rWtblSecurityKey = {0};
	/* Tag = 9, Rdg */
	CMD_WTBL_RDG_T			rWtblRdg = {0};
#ifdef TXBF_SUPPORT
	/* Tag = 12, BF */
	CMD_WTBL_BF_T           rWtblBf = {0};
#endif /* TXBF_SUPPORT */
	/* Tag = 13, SMPS */
	CMD_WTBL_SMPS_T			rWtblSmPs = {0};
	/* Tag = 16, SPE */
	CMD_WTBL_SPE_T          rWtblSpe = {0};
	/* Allocate TLV msg */
	Status = os_alloc_mem(pAd, (UCHAR **)&pTlvBuffer, MAX_BUF_SIZE_OF_WTBL_INFO);
	MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s(): %d,%d,%d,%d,%d,%d,%d,%d,%d,(%x:%x:%x:%x:%x:%x),%d,%d,%d,%d,%d,%d)\n", __func__,
			 WtblInfo.Wcid,
			 WtblInfo.Aid,
			 WtblInfo.BssidIdx,
			 WtblInfo.MacAddrIdx,
			 WtblInfo.SmpsMode,
			 WtblInfo.MaxRAmpduFactor,
			 WtblInfo.MpduDensity,
			 WtblInfo.WcidType,
			 WtblInfo.aad_om,
			 PRINT_MAC(WtblInfo.Addr),
			 WtblInfo.CipherSuit,
			 WtblInfo.PfmuId,
			 WtblInfo.SupportHT,
			 WtblInfo.SupportVHT,
			 WtblInfo.SupportRDG,
			 WtblInfo.SupportQoS));

	if ((Status != NDIS_STATUS_SUCCESS) || (pTlvBuffer == NULL))
		goto error;

	pTempBuffer = pTlvBuffer;
	rWtblRx.ucRv   = WtblInfo.rv;
	rWtblRx.ucRca2 = WtblInfo.rca2;

	if (WtblInfo.WcidType == MT_WCID_TYPE_APCLI_MCAST) {
		/* prevent BMC ICV message dumped during GTK rekey */
		if (IF_COMBO_HAVE_AP_STA(pAd) && HcGetWcidLinkType(pAd, WtblInfo.Wcid) == WDEV_TYPE_STA)
			rWtblRx.ucRcid = 1;
	}

	/* Manipulate TLV msg */
	if (WtblInfo.WcidType == MT_WCID_TYPE_BMCAST) {
		/* Tag = 0 */
		rWtblGeneric.ucMUARIndex = 0x0e;
		/* Tag = 1 */
		rWtblRx.ucRv = 1;
		rWtblRx.ucRca1 = 1;
		/* if (pAd->OpMode == OPMODE_AP) */
		{
			rWtblRx.ucRca2 = 1;
		}
		/* Tag = 7 */
		rWtblSecurityKey.ucAlgorithmId = WTBL_CIPHER_NONE;
		/* Tag = 6 */
#ifdef HDR_TRANS_TX_SUPPORT

		if (pAd->OpMode == OPMODE_AP) {
			rWtblHdrTrans.ucFd = 1;
			rWtblHdrTrans.ucTd = 0;
		}

#endif
	} else {
		/* Tag = 0 */
		rWtblGeneric.ucMUARIndex = WtblInfo.MacAddrIdx;
		rWtblGeneric.ucQos = (WtblInfo.SupportQoS) ? 1 : 0;
		rWtblGeneric.u2PartialAID = WtblInfo.Aid;
		rWtblGeneric.ucAadOm = WtblInfo.aad_om;

		/* Tag = 1 */
		if ((WtblInfo.WcidType == MT_WCID_TYPE_APCLI) ||
			(WtblInfo.WcidType == MT_WCID_TYPE_REPEATER) ||
			(WtblInfo.WcidType == MT_WCID_TYPE_AP) ||
			(WtblInfo.WcidType == MT_WCID_TYPE_APCLI_MCAST))
			rWtblRx.ucRca1 = 1;

		rWtblRx.ucRv = 1;
		rWtblRx.ucRca2 = 1;
		/* Tag = 7 */
		rWtblSecurityKey.ucAlgorithmId = WtblInfo.CipherSuit;
		rWtblSecurityKey.ucRkv = (WtblInfo.CipherSuit != WTBL_CIPHER_NONE) ? 1 : 0;
		/* Tag = 6 */
#ifdef HDR_TRANS_TX_SUPPORT

		switch (WtblInfo.WcidType) {
		case MT_WCID_TYPE_AP:
			rWtblHdrTrans.ucFd = 0;
			rWtblHdrTrans.ucTd = 1;
			break;

		case MT_WCID_TYPE_CLI:
			rWtblHdrTrans.ucFd = 1;
			rWtblHdrTrans.ucTd = 0;
#if defined(A4_CONN) || defined(MBSS_AS_WDS_AP_SUPPORT)
			if (WtblInfo.a4_enable) {
				rWtblHdrTrans.ucFd = 1;
				rWtblHdrTrans.ucTd = 1;
				/* MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_TRACE,("MtAsicUpdateRxWCIDTableByFw MT_WCID_TYPE_CLI: do FdTd settings in rWtblHdrTrans\n")); */
			}
#endif /* A4_CONN */
			break;

		case MT_WCID_TYPE_APCLI:
		case MT_WCID_TYPE_REPEATER:
			rWtblHdrTrans.ucFd = 0;
			rWtblHdrTrans.ucTd = 1;
#if defined(A4_CONN) || defined(APCLI_AS_WDS_STA_SUPPORT)
			if (WtblInfo.a4_enable) {
				rWtblHdrTrans.ucFd = 1;
				rWtblHdrTrans.ucTd = 1;
				/* MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_TRACE,("MtAsicUpdateRxWCIDTableByFw MT_WCID_TYPE_APCLI/MT_WCID_TYPE_REPEATER do FdTd settings in rWtblHdrTrans\n")); */
			}
#endif /* A4_CONN */
			break;

		case MT_WCID_TYPE_WDS:
			rWtblHdrTrans.ucFd = 1;
			rWtblHdrTrans.ucTd = 1;
			break;

		default:
			MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
					 ("%s: Unknown entry type(%d) do not support header translation\n",
					  __func__, WtblInfo.WcidType));
			break;
		}

#endif /* HDR_TRANS_TX_SUPPORT */
#ifdef HDR_TRANS_RX_SUPPORT

		if (WtblInfo.DisRHTR)
			rWtblHdrTrans.ucDisRhtr = 1;
		else
			rWtblHdrTrans.ucDisRhtr = 0;

#endif /* HDR_TRANS_RX_SUPPORT */
#ifdef DOT11_N_SUPPORT

		if (WtblInfo.SupportHT) {
			/* Tag = 0 */
			rWtblGeneric.ucQos = 1;
			rWtblGeneric.ucBafEn = 0;
			/* Tag = 2 */
			rWtblHt.ucHt = 1;
			rWtblHt.ucMm = WtblInfo.MpduDensity;
			rWtblHt.ucAf = WtblInfo.MaxRAmpduFactor;

			/* Tga = 9 */
			if (WtblInfo.SupportRDG) {
				rWtblRdg.ucR = 1;
				rWtblRdg.ucRdgBa = 1;
			}

			/* Tag = 13*/
			if (WtblInfo.SmpsMode == MMPS_DYNAMIC)
				rWtblSmPs.ucSmPs = 1;
			else
				rWtblSmPs.ucSmPs = 0;

#ifdef DOT11_VHT_AC

			/* Tag = 3 */
			if (WtblInfo.SupportVHT) {
				rWtblVht.ucVht = 1;
				/* ucDynBw for WTBL: 0 - not DynBW / 1 -DynBW */
				rWtblVht.ucDynBw = (WtblInfo.dyn_bw == BW_SIGNALING_DYNAMIC)?1:0;
			}

#endif /* DOT11_VHT_AC */
		}

#endif /* DOT11_N_SUPPORT */
	}

	/* Tag = 0 */
	os_move_mem(rWtblGeneric.aucPeerAddress, WtblInfo.Addr, MAC_ADDR_LEN);
	/* Tag = 5 */
	rWtblTxPs.ucTxPs = 0;
#ifdef TXBF_SUPPORT
	/* Tag = 0xc */
	rWtblBf.ucGid     = WtblInfo.gid;
	rWtblBf.ucPFMUIdx = WtblInfo.PfmuId;
	rWtblBf.ucTiBf    = WtblInfo.fgTiBf;
	rWtblBf.ucTeBf    = WtblInfo.fgTeBf;
	rWtblBf.ucTibfVht = WtblInfo.fgTibfVht;
	rWtblBf.ucTebfVht = WtblInfo.fgTebfVht;
#endif /* TXBF_SUPPORT */
	/* Tag = 0x10 */
	rWtblSpe.ucSpeIdx = WtblInfo.spe_idx;
	/* Append TLV msg */
	pTempBuffer = pTlvAppend(
					  pTlvBuffer,
					  (WTBL_GENERIC),
					  (sizeof(CMD_WTBL_GENERIC_T)),
					  &rWtblGeneric,
					  &u4TotalTlvLen,
					  &ucTotalTlvNumber);
	pTempBuffer = pTlvAppend(
					  pTempBuffer,
					  (WTBL_RX),
					  (sizeof(CMD_WTBL_RX_T)),
					  &rWtblRx,
					  &u4TotalTlvLen,
					  &ucTotalTlvNumber);
#ifdef DOT11_N_SUPPORT
	pTempBuffer = pTlvAppend(
					  pTempBuffer,
					  (WTBL_HT),
					  (sizeof(CMD_WTBL_HT_T)),
					  &rWtblHt,
					  &u4TotalTlvLen,
					  &ucTotalTlvNumber);
	pTempBuffer = pTlvAppend(
					  pTempBuffer,
					  (WTBL_RDG),
					  (sizeof(CMD_WTBL_RDG_T)),
					  &rWtblRdg,
					  &u4TotalTlvLen,
					  &ucTotalTlvNumber);
	pTempBuffer = pTlvAppend(
					  pTempBuffer,
					  (WTBL_SMPS),
					  (sizeof(CMD_WTBL_SMPS_T)),
					  &rWtblSmPs,
					  &u4TotalTlvLen,
					  &ucTotalTlvNumber);
#ifdef DOT11_VHT_AC
	pTempBuffer = pTlvAppend(
					  pTempBuffer,
					  (WTBL_VHT),
					  (sizeof(CMD_WTBL_VHT_T)),
					  &rWtblVht,
					  &u4TotalTlvLen,
					  &ucTotalTlvNumber);
#endif /* DOT11_VHT_AC */
#endif /* DOT11_N_SUPPORT */
	pTempBuffer = pTlvAppend(
					  pTempBuffer,
					  (WTBL_TX_PS),
					  (sizeof(CMD_WTBL_TX_PS_T)),
					  &rWtblTxPs,
					  &u4TotalTlvLen,
					  &ucTotalTlvNumber);
#if defined(HDR_TRANS_RX_SUPPORT) || defined(HDR_TRANS_TX_SUPPORT)
	pTempBuffer = pTlvAppend(
					  pTempBuffer,
					  (WTBL_HDR_TRANS),
					  (sizeof(CMD_WTBL_HDR_TRANS_T)),
					  &rWtblHdrTrans,
					  &u4TotalTlvLen,
					  &ucTotalTlvNumber);
#endif /* HDR_TRANS_RX_SUPPORT || HDR_TRANS_TX_SUPPORT */
	if (WtblInfo.SkipClearPrevSecKey == FALSE)
	pTempBuffer = pTlvAppend(
					  pTempBuffer,
					  (WTBL_SECURITY_KEY),
					  (sizeof(CMD_WTBL_SECURITY_KEY_T)),
					  &rWtblSecurityKey,
					  &u4TotalTlvLen,
					  &ucTotalTlvNumber);
#ifdef TXBF_SUPPORT

	if (pAd->rStaRecBf.u2PfmuId != 0xFFFF) {
		pTempBuffer = pTlvAppend(
						  pTempBuffer,
						  (WTBL_BF),
						  (sizeof(CMD_WTBL_BF_T)),
						  &rWtblBf,
						  &u4TotalTlvLen,
						  &ucTotalTlvNumber);
	}

#endif /* TXBF_SUPPORT */
	pTempBuffer = pTlvAppend(
					  pTempBuffer,
					  (WTBL_SPE),
					  (sizeof(CMD_WTBL_SPE_T)),
					  &rWtblSpe,
					  &u4TotalTlvLen,
					  &ucTotalTlvNumber);
	/* Send TLV msg*/
	if (WtblInfo.SkipClearPrevSecKey == FALSE || WtblInfo.IsReset == TRUE)
		CmdExtWtblUpdate(pAd, WtblInfo.Wcid, RESET_WTBL_AND_SET, pTlvBuffer, u4TotalTlvLen);
	else
		CmdExtWtblUpdate(pAd, WtblInfo.Wcid, SET_WTBL, pTlvBuffer, u4TotalTlvLen);
	/* Free TLV msg */
	if (pTlvBuffer)
		os_free_mem(pTlvBuffer);

error:
	MTWF_LOG(DBG_CAT_FW, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(Ret = %d)\n", __func__, Status));
}



VOID MtAsicUpdateBASessionByWtblTlv(RTMP_ADAPTER *pAd, MT_BA_CTRL_T BaCtrl)
{
	CMD_WTBL_BA_T		rWtblBa = {0};
	struct _RTMP_CHIP_CAP *cap = hc_get_chip_cap(pAd->hdev_ctrl);
	UINT16 *ba_range = cap->ppdu.ba_range;

	if (BaCtrl.Tid > 7) {
		MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s: unknown tid(%d)\n", __func__, BaCtrl.Tid));
		return;
	}

	rWtblBa.u2Tag = WTBL_BA;
	rWtblBa.u2Length = sizeof(CMD_WTBL_BA_T);
	rWtblBa.ucTid = BaCtrl.Tid;
	rWtblBa.ucBaSessionType = BaCtrl.BaSessionType;

	if (BaCtrl.BaSessionType == BA_SESSION_RECP) {
		/* Reset BA SSN & Score Board Bitmap, for BA Receiptor */
		if (BaCtrl.isAdd) {
			os_move_mem(rWtblBa.aucPeerAddress,  BaCtrl.PeerAddr, MAC_ADDR_LEN);
			rWtblBa.ucRstBaTid = BaCtrl.Tid;
			rWtblBa.ucRstBaSel = RST_BA_MAC_TID_MATCH;
			rWtblBa.ucStartRstBaSb = 1;
			CmdExtWtblUpdate(pAd, BaCtrl.Wcid, SET_WTBL, &rWtblBa, sizeof(rWtblBa));
		}
	} else {
		if (BaCtrl.isAdd) {
			INT idx = 0;
			/* Clear WTBL2. SN: Direct Updating */
			rWtblBa.u2Sn = BaCtrl.Sn;

			/*get ba win size from range */
			while (ba_range[idx] < BaCtrl.BaWinSize) {
				if (idx == 7)
					break;

				idx++;
			};

			if (ba_range[idx] > BaCtrl.BaWinSize)
				idx--;

			/* Clear BA_WIN_SIZE and set new value to it */
			rWtblBa.ucBaWinSizeIdx = idx;
			/* Enable BA_EN */
			rWtblBa.ucBaEn = 1;
		} else {
			/* Clear BA_WIN_SIZE and set new value to it */
			rWtblBa.ucBaWinSizeIdx = 0;
			/* Enable BA_EN */
			rWtblBa.ucBaEn = 0;
		}

		CmdExtWtblUpdate(pAd, BaCtrl.Wcid, SET_WTBL, &rWtblBa, sizeof(rWtblBa));
	}
}


INT32 MtAsicUpdateBASessionByFw(
	IN PRTMP_ADAPTER pAd,
	IN MT_BA_CTRL_T BaCtrl)
{
	INT32				Status = NDIS_STATUS_FAILURE;
	CMD_WTBL_BA_T		rWtblBa = {0};
	struct _RTMP_CHIP_CAP *cap = hc_get_chip_cap(pAd->hdev_ctrl);
	UINT16 *ba_range = cap->ppdu.ba_range;

	if (BaCtrl.Tid > 7) {
		MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s: unknown tid(%d)\n", __func__, BaCtrl.Tid));
		return Status;
	}

	rWtblBa.u2Tag = WTBL_BA;
	rWtblBa.u2Length = sizeof(CMD_WTBL_BA_T);
	rWtblBa.ucTid = BaCtrl.Tid;
	rWtblBa.ucBaSessionType = BaCtrl.BaSessionType;

	if (BaCtrl.BaSessionType == BA_SESSION_RECP) {
		/* Reset BA SSN & Score Board Bitmap, for BA Receiptor */
		if (BaCtrl.isAdd) {
			rWtblBa.ucBandIdx = BaCtrl.band_idx;
			os_move_mem(rWtblBa.aucPeerAddress,  BaCtrl.PeerAddr, MAC_ADDR_LEN);
			rWtblBa.ucRstBaTid = BaCtrl.Tid;
			rWtblBa.ucRstBaSel = RST_BA_MAC_TID_MATCH;
			rWtblBa.ucStartRstBaSb = 1;
			Status = CmdExtWtblUpdate(pAd, BaCtrl.Wcid, SET_WTBL, &rWtblBa, sizeof(rWtblBa));
		}

		/* TODO: Hanmin 7615, need rWtblBa.ucBaEn=0 for delete? */
	} else {
		if (BaCtrl.isAdd) {
			INT idx = 0;
			/* Clear WTBL2. SN: Direct Updating */
			rWtblBa.u2Sn = BaCtrl.Sn;

			/* Get ba win size from range */
			while (BaCtrl.BaWinSize > ba_range[idx]) {
				if (idx == (MT_DMAC_BA_AGG_RANGE - 1))
					break;

				idx++;
			};

			if ((idx > 0) && (ba_range[idx] > BaCtrl.BaWinSize))
				idx--;

			/* Clear BA_WIN_SIZE and set new value to it */
			rWtblBa.ucBaWinSizeIdx = idx;
			/* Enable BA_EN */
			rWtblBa.ucBaEn = 1;
		} else {
			/* Clear BA_WIN_SIZE and set new value to it */
			rWtblBa.ucBaWinSizeIdx = 0;
			/* Enable BA_EN */
			rWtblBa.ucBaEn = 0;
		}

		Status = CmdExtWtblUpdate(pAd, BaCtrl.Wcid, SET_WTBL, &rWtblBa, sizeof(rWtblBa));
	}

	return Status;
}

/* offload BA winsize index calculation to FW */
INT32 MtAsicUpdateBASessionOffloadByFw(
	IN PRTMP_ADAPTER pAd,
	IN MT_BA_CTRL_T BaCtrl)
{
	INT32				Status = NDIS_STATUS_FAILURE;
	CMD_WTBL_BA_T		rWtblBa = {0};

	if (BaCtrl.Tid > 7) {
		MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s: unknown tid(%d)\n", __func__, BaCtrl.Tid));
		return Status;
	}

	rWtblBa.u2Tag = WTBL_BA;
	rWtblBa.u2Length = sizeof(CMD_WTBL_BA_T);
	rWtblBa.ucTid = BaCtrl.Tid;
	rWtblBa.ucBaSessionType = BaCtrl.BaSessionType;

	if (BaCtrl.BaSessionType == BA_SESSION_RECP) {
		/* Reset BA SSN & Score Board Bitmap, for BA Receiptor */
		if (BaCtrl.isAdd) {
			rWtblBa.ucBandIdx = BaCtrl.band_idx;
			os_move_mem(rWtblBa.aucPeerAddress,  BaCtrl.PeerAddr, MAC_ADDR_LEN);
			rWtblBa.ucRstBaTid = BaCtrl.Tid;
			rWtblBa.ucRstBaSel = RST_BA_MAC_TID_MATCH;
			rWtblBa.ucStartRstBaSb = 1;
			Status = CmdExtWtblUpdate(pAd, BaCtrl.Wcid, SET_WTBL, &rWtblBa, sizeof(rWtblBa));
		}

		/* TODO: Hanmin 7615, need rWtblBa.ucBaEn=0 for delete? */
	} else {
		if (BaCtrl.isAdd) {
			/* Clear WTBL2. SN: Direct Updating */
			rWtblBa.u2Sn = BaCtrl.Sn;
			/* Clear BA_WIN_SIZE and set new value to it */
			rWtblBa.u2BaWinSize = BaCtrl.BaWinSize;
			/* Enable BA_EN */
			rWtblBa.ucBaEn = 1;
		} else {
			/* Clear BA_WIN_SIZE and set new value to it */
			rWtblBa.u2BaWinSize = 0;
			/* Enable BA_EN */
			rWtblBa.ucBaEn = 0;
		}

		Status = CmdExtWtblUpdate(pAd, BaCtrl.Wcid, SET_WTBL, &rWtblBa, sizeof(rWtblBa));
	}

	return Status;
}

VOID MtAsicAddRemoveKeyTabByFw(
	IN struct _RTMP_ADAPTER *pAd,
	IN struct _ASIC_SEC_INFO *pInfo)
{
	CMD_WTBL_SECURITY_KEY_T *wtbl_security_key = NULL;
	UINT32 cmd_len = 0;

	MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:, wcid=%d, Operation=%d, Direction=%d\n",
			 __func__, pInfo->Wcid, pInfo->Operation, pInfo->Direction));

	if (chip_fill_key_install_cmd(pAd->hdev_ctrl, pInfo, WTBL_SEC_KEY_METHOD, (VOID **)&wtbl_security_key, &cmd_len) != NDIS_STATUS_SUCCESS)
		return;

	CmdExtWtblUpdate(pAd, pInfo->Wcid, SET_WTBL, (PUCHAR)wtbl_security_key, cmd_len);

	os_free_mem(wtbl_security_key);
}


VOID MtAsicSetSMPSByFw(
	IN struct _RTMP_ADAPTER *pAd,
	IN UINT16 Wcid,
	IN UCHAR Smps)
{
	CMD_WTBL_SMPS_T	CmdWtblSmPs = {0};

	CmdWtblSmPs.u2Tag = WTBL_SMPS;
	CmdWtblSmPs.u2Length = sizeof(CMD_WTBL_SMPS_T);
	CmdWtblSmPs.ucSmPs = Smps;
	CmdExtWtblUpdate(pAd, Wcid, SET_WTBL, (PUCHAR)&CmdWtblSmPs, sizeof(CMD_WTBL_SMPS_T));
}

VOID MtAsicGetTxTscByFw(
	IN struct _RTMP_ADAPTER *pAd,
	IN struct wifi_dev *wdev,
	OUT UCHAR * pTxTsc)
{
	CMD_WTBL_PN_T cmdWtblPn = {0};
	USHORT wcid = 0;

	GET_GroupKey_WCID(wdev, wcid);
	cmdWtblPn.u2Tag = WTBL_PN;
	cmdWtblPn.u2Length = sizeof(CMD_WTBL_PN_T);
	CmdExtWtblUpdate(pAd, wcid, QUERY_WTBL, (PUCHAR)&cmdWtblPn, sizeof(CMD_WTBL_PN_T));

	os_move_mem(pTxTsc, cmdWtblPn.aucPn, 6);
}

VOID mt_wtbltlv_debug(RTMP_ADAPTER *pAd, UINT16 u2Wcid, UCHAR ucCmdId, UCHAR ucAtion, union _wtbl_debug_u *debug_u)
{
	/* tag 0 */
	if (ucCmdId == WTBL_GENERIC) {
		debug_u->wtbl_generic_t.u2Tag = WTBL_GENERIC;
		debug_u->wtbl_generic_t.u2Length = sizeof(CMD_WTBL_GENERIC_T);

		if (ucAtion == 0) {
			/* Set to 0 */
			UCHAR TestMac[MAC_ADDR_LEN] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

			os_move_mem(debug_u->wtbl_generic_t.aucPeerAddress, TestMac, MAC_ADDR_LEN);
			debug_u->wtbl_generic_t.ucMUARIndex = 0x0;
			debug_u->wtbl_generic_t.ucSkipTx = 0;
			debug_u->wtbl_generic_t.ucCfAck = 0;
			debug_u->wtbl_generic_t.ucQos = 0;
			debug_u->wtbl_generic_t.ucMesh = 0;
			debug_u->wtbl_generic_t.ucAdm = 0;
			debug_u->wtbl_generic_t.u2PartialAID = 0;
			debug_u->wtbl_generic_t.ucBafEn = 0;
			CmdExtWtblUpdate(pAd, u2Wcid, SET_WTBL, &debug_u->wtbl_generic_t, sizeof(CMD_WTBL_GENERIC_T));
		} else if (ucAtion == 1) {
			/* Set to 1 */
			UCHAR TestMac[MAC_ADDR_LEN] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

			os_move_mem(debug_u->wtbl_generic_t.aucPeerAddress, TestMac, MAC_ADDR_LEN);
			debug_u->wtbl_generic_t.ucMUARIndex = 0x0e;
			debug_u->wtbl_generic_t.ucSkipTx = 1;
			debug_u->wtbl_generic_t.ucCfAck = 1;
			debug_u->wtbl_generic_t.ucQos = 1;
			debug_u->wtbl_generic_t.ucMesh = 1;
			debug_u->wtbl_generic_t.ucAdm = 1;
			debug_u->wtbl_generic_t.u2PartialAID = 32;
			debug_u->wtbl_generic_t.ucBafEn = 1;
			CmdExtWtblUpdate(pAd, u2Wcid, SET_WTBL, &debug_u->wtbl_generic_t, sizeof(CMD_WTBL_GENERIC_T));
		} else if (ucAtion == 2) {
			/* query */
			CmdExtWtblUpdate(pAd, u2Wcid, QUERY_WTBL, &debug_u->wtbl_generic_t, sizeof(CMD_WTBL_GENERIC_T));
		} else
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s::Cmd Error\n", __func__));
	}

	/* tag 1 */
	if (ucCmdId == WTBL_RX) {
		debug_u->wtbl_rx_t.u2Tag = WTBL_RX;
		debug_u->wtbl_rx_t.u2Length = sizeof(CMD_WTBL_RX_T);

		if (ucAtion == 0) {
			/* Set to 0 */
			debug_u->wtbl_rx_t.ucRcid = 0;
			debug_u->wtbl_rx_t.ucRca1 = 0;
			debug_u->wtbl_rx_t.ucRca2 = 0;
			debug_u->wtbl_rx_t.ucRv = 0;
			CmdExtWtblUpdate(pAd, u2Wcid, SET_WTBL, &debug_u->wtbl_rx_t, sizeof(CMD_WTBL_RX_T));
		} else if (ucAtion == 1) {
			/* Set to 1 */
			debug_u->wtbl_rx_t.ucRcid = 1;
			debug_u->wtbl_rx_t.ucRca1 = 1;
			debug_u->wtbl_rx_t.ucRca2 = 1;
			debug_u->wtbl_rx_t.ucRv = 1;
			CmdExtWtblUpdate(pAd, u2Wcid, SET_WTBL, &debug_u->wtbl_rx_t, sizeof(CMD_WTBL_RX_T));
		} else if (ucAtion == 2) {
			/* query */
			CmdExtWtblUpdate(pAd, u2Wcid, QUERY_WTBL, &debug_u->wtbl_rx_t, sizeof(CMD_WTBL_RX_T));
		} else
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s::Cmd Error\n", __func__));
	}

	/* tag 2 */
	if (ucCmdId == WTBL_HT) {
		debug_u->wtbl_ht_t.u2Tag = WTBL_HT;
		debug_u->wtbl_ht_t.u2Length = sizeof(CMD_WTBL_HT_T);

		if (ucAtion == 0) {
			/* Set to 0 */
			debug_u->wtbl_ht_t.ucHt = 0;
			debug_u->wtbl_ht_t.ucLdpc = 0;
			debug_u->wtbl_ht_t.ucAf = 0;
			debug_u->wtbl_ht_t.ucMm = 0;
			CmdExtWtblUpdate(pAd, u2Wcid, SET_WTBL, &debug_u->wtbl_ht_t, sizeof(CMD_WTBL_HT_T));
		} else if (ucAtion == 1) {
			/* Set to 1 */
			debug_u->wtbl_ht_t.ucHt = 1;
			debug_u->wtbl_ht_t.ucLdpc = 1;
			debug_u->wtbl_ht_t.ucAf = 1;
			debug_u->wtbl_ht_t.ucMm = 1;
			CmdExtWtblUpdate(pAd, u2Wcid, SET_WTBL, &debug_u->wtbl_ht_t, sizeof(CMD_WTBL_HT_T));
		} else if (ucAtion == 2) {
			/* query */
			CmdExtWtblUpdate(pAd, u2Wcid, QUERY_WTBL, &debug_u->wtbl_ht_t, sizeof(CMD_WTBL_HT_T));
		} else
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s::Cmd Error\n", __func__));
	}

	/* tag 3 */
	if (ucCmdId == WTBL_VHT) {
		debug_u->wtbl_vht_t.u2Tag = WTBL_VHT;
		debug_u->wtbl_vht_t.u2Length = sizeof(CMD_WTBL_VHT_T);

		if (ucAtion == 0) {
			/* Set to 0 */
			debug_u->wtbl_vht_t.ucLdpcVht = 0;
			debug_u->wtbl_vht_t.ucDynBw = 0;
			debug_u->wtbl_vht_t.ucVht = 0;
			debug_u->wtbl_vht_t.ucTxopPsCap = 0;
			CmdExtWtblUpdate(pAd, u2Wcid, SET_WTBL, &debug_u->wtbl_vht_t, sizeof(CMD_WTBL_VHT_T));
		} else if (ucAtion == 1) {
			/* Set to 1 */
			debug_u->wtbl_vht_t.ucLdpcVht = 1;
			debug_u->wtbl_vht_t.ucDynBw = 1;
			debug_u->wtbl_vht_t.ucVht = 1;
			debug_u->wtbl_vht_t.ucTxopPsCap = 1;
			CmdExtWtblUpdate(pAd, u2Wcid, SET_WTBL, &debug_u->wtbl_vht_t, sizeof(CMD_WTBL_VHT_T));
		} else if (ucAtion == 2) {
			/* query */
			CmdExtWtblUpdate(pAd, u2Wcid, QUERY_WTBL, &debug_u->wtbl_vht_t, sizeof(CMD_WTBL_VHT_T));
		} else
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s::Cmd Error\n", __func__));
	}

	/* tag 4 */
	if (ucCmdId == WTBL_PEER_PS) {
		debug_u->wtbl_peer_ps_t.u2Tag = WTBL_PEER_PS;
		debug_u->wtbl_peer_ps_t.u2Length = sizeof(CMD_WTBL_PEER_PS_T);

		if (ucAtion == 0) {
			/* Set to 0 */
			debug_u->wtbl_peer_ps_t.ucDuIPsm = 0;
			debug_u->wtbl_peer_ps_t.ucIPsm = 0;
			CmdExtWtblUpdate(pAd, u2Wcid, SET_WTBL, &debug_u->wtbl_peer_ps_t, sizeof(CMD_WTBL_PEER_PS_T));
		} else if (ucAtion == 1) {
			/* Set to 1 */
			debug_u->wtbl_peer_ps_t.ucDuIPsm = 1;
			debug_u->wtbl_peer_ps_t.ucIPsm = 1;
			CmdExtWtblUpdate(pAd, u2Wcid, SET_WTBL, &debug_u->wtbl_peer_ps_t, sizeof(CMD_WTBL_PEER_PS_T));
		} else if (ucAtion == 2) {
			/* query */
			CmdExtWtblUpdate(pAd, u2Wcid, QUERY_WTBL, &debug_u->wtbl_peer_ps_t, sizeof(CMD_WTBL_PEER_PS_T));
		} else
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s::Cmd Error\n", __func__));
	}

	/* tag 5 */
	if (ucCmdId == WTBL_TX_PS) {
		debug_u->wtbl_tx_ps_t.u2Tag = WTBL_TX_PS;
		debug_u->wtbl_tx_ps_t.u2Length = sizeof(CMD_WTBL_TX_PS_T);

		if (ucAtion == 0) {
			/* Set to 0 */
			debug_u->wtbl_tx_ps_t.ucTxPs = 0;
			CmdExtWtblUpdate(pAd, u2Wcid, SET_WTBL, &debug_u->wtbl_tx_ps_t, sizeof(CMD_WTBL_TX_PS_T));
		} else if (ucAtion == 1) {
			/* Set to 1 */
			debug_u->wtbl_tx_ps_t.ucTxPs = 1;
			CmdExtWtblUpdate(pAd, u2Wcid, SET_WTBL, &debug_u->wtbl_tx_ps_t, sizeof(CMD_WTBL_TX_PS_T));
		} else if (ucAtion == 2) {
			/* query */
			CmdExtWtblUpdate(pAd, u2Wcid, QUERY_WTBL, &debug_u->wtbl_tx_ps_t, sizeof(CMD_WTBL_TX_PS_T));
		} else
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s::Cmd Error\n", __func__));
	}

	/* tag 6 */
	if (ucCmdId == WTBL_HDR_TRANS) {
		debug_u->wtbl_hdr_trans_t.u2Tag = WTBL_HDR_TRANS;
		debug_u->wtbl_hdr_trans_t.u2Length = sizeof(CMD_WTBL_HDR_TRANS_T);

		if (ucAtion == 0) {
			/* Set to 0 */
			debug_u->wtbl_hdr_trans_t.ucTd = 0;
			debug_u->wtbl_hdr_trans_t.ucFd = 0;
			debug_u->wtbl_hdr_trans_t.ucDisRhtr = 0;
			CmdExtWtblUpdate(pAd,
					 u2Wcid,
					 SET_WTBL,
					 &debug_u->wtbl_hdr_trans_t,
					 sizeof(CMD_WTBL_HDR_TRANS_T));
		} else if (ucAtion == 1) {
			/* Set to 1 */
			debug_u->wtbl_hdr_trans_t.ucTd = 1;
			debug_u->wtbl_hdr_trans_t.ucFd = 1;
			debug_u->wtbl_hdr_trans_t.ucDisRhtr = 1;
			CmdExtWtblUpdate(pAd,
					 u2Wcid,
					 SET_WTBL,
					 &debug_u->wtbl_hdr_trans_t,
					 sizeof(CMD_WTBL_HDR_TRANS_T));
		} else if (ucAtion == 2) {
			/* query */
			CmdExtWtblUpdate(pAd,
					 u2Wcid,
					 QUERY_WTBL,
					 &debug_u->wtbl_hdr_trans_t,
					 sizeof(CMD_WTBL_HDR_TRANS_T));
		} else
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s::Cmd Error\n", __func__));
	}

	/* tag 7 security */
	/* tag 8 BA */

	/* tag 9 */
	if (ucCmdId == WTBL_RDG) {
		debug_u->wtbl_rdg_t.u2Tag = WTBL_RDG;
		debug_u->wtbl_rdg_t.u2Length = sizeof(CMD_WTBL_RDG_T);

		if (ucAtion == 0) {
			/* Set to 0 */
			debug_u->wtbl_rdg_t.ucRdgBa = 0;
			debug_u->wtbl_rdg_t.ucR = 0;
			CmdExtWtblUpdate(pAd, u2Wcid, SET_WTBL, &debug_u->wtbl_rdg_t, sizeof(CMD_WTBL_RDG_T));
		} else if (ucAtion == 1) {
			/* Set to 1 */
			debug_u->wtbl_rdg_t.ucRdgBa = 1;
			debug_u->wtbl_rdg_t.ucR = 1;
			CmdExtWtblUpdate(pAd, u2Wcid, SET_WTBL, &debug_u->wtbl_rdg_t, sizeof(CMD_WTBL_RDG_T));
		} else if (ucAtion == 2) {
			/* query */
			CmdExtWtblUpdate(pAd, u2Wcid, QUERY_WTBL, &debug_u->wtbl_rdg_t, sizeof(CMD_WTBL_RDG_T));
		} else
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s::Cmd Error\n", __func__));
	}

	/* tag 10 */
	if (ucCmdId == WTBL_PROTECTION) {
		debug_u->wtbl_prot_t.u2Tag = WTBL_PROTECTION;
		debug_u->wtbl_prot_t.u2Length = sizeof(CMD_WTBL_PROTECTION_T);

		if (ucAtion == 0) {
			/* Set to 0 */
			debug_u->wtbl_prot_t.ucRts = 0;
			CmdExtWtblUpdate(pAd, u2Wcid, SET_WTBL, &debug_u->wtbl_prot_t, sizeof(CMD_WTBL_PROTECTION_T));
		} else if (ucAtion == 1) {
			/* Set to 1 */
			debug_u->wtbl_prot_t.ucRts = 1;
			CmdExtWtblUpdate(pAd, u2Wcid, SET_WTBL, &debug_u->wtbl_prot_t, sizeof(CMD_WTBL_PROTECTION_T));
		} else if (ucAtion == 2) {
			/* query */
			CmdExtWtblUpdate(pAd, u2Wcid, QUERY_WTBL, &debug_u->wtbl_prot_t, sizeof(CMD_WTBL_PROTECTION_T));
		} else
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s::Cmd Error\n", __func__));
	}

	/* tag 11 */
	if (ucCmdId == WTBL_CLEAR) {
		debug_u->wtbl_clear_t.u2Tag = WTBL_CLEAR;
		debug_u->wtbl_clear_t.u2Length = sizeof(CMD_WTBL_CLEAR_T);

		if (ucAtion == 0) {
			/* Set to 0 */
		} else if (ucAtion == 1) {
			/* Set to 1 */
			debug_u->wtbl_clear_t.ucClear = ((0 << 1) |
							 (1 << 1) |
							 (1 << 2) |
							 (1 << 3) |
							 (1 << 4) |
							 (1 << 5));
			CmdExtWtblUpdate(pAd, u2Wcid, SET_WTBL, &debug_u->wtbl_clear_t, sizeof(CMD_WTBL_CLEAR_T));
		} else if (ucAtion == 2) {
			/* query */
		} else
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s::Cmd Error\n", __func__));
	}

	/* tag 12 */
	if (ucCmdId == WTBL_BF) {
		debug_u->wtbl_bf_t.u2Tag = WTBL_BF;
		debug_u->wtbl_bf_t.u2Length = sizeof(CMD_WTBL_BF_T);

		if (ucAtion == 0) {
			/* Set to 0 */
			debug_u->wtbl_bf_t.ucTiBf = 0;
			debug_u->wtbl_bf_t.ucTeBf = 0;
			debug_u->wtbl_bf_t.ucTibfVht = 0;
			debug_u->wtbl_bf_t.ucTebfVht = 0;
			debug_u->wtbl_bf_t.ucGid = 0;
			CmdExtWtblUpdate(pAd, u2Wcid, SET_WTBL, &debug_u->wtbl_bf_t, sizeof(CMD_WTBL_BF_T));
		} else if (ucAtion == 1) {
			/* Set to 1 */
			debug_u->wtbl_bf_t.ucTiBf = 1;
			debug_u->wtbl_bf_t.ucTeBf = 1;
			debug_u->wtbl_bf_t.ucTibfVht = 1;
			debug_u->wtbl_bf_t.ucTebfVht = 1;
			debug_u->wtbl_bf_t.ucGid = 1;
			CmdExtWtblUpdate(pAd, u2Wcid, SET_WTBL, &debug_u->wtbl_bf_t, sizeof(CMD_WTBL_BF_T));
		} else if (ucAtion == 2) {
			/* query */
			CmdExtWtblUpdate(pAd, u2Wcid, QUERY_WTBL, &debug_u->wtbl_bf_t, sizeof(CMD_WTBL_BF_T));
		} else
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s::Cmd Error\n", __func__));
	}

	/* tag 13 */
	if (ucCmdId == WTBL_SMPS) {
		debug_u->wtbl_smps_t.u2Tag = WTBL_SMPS;
		debug_u->wtbl_smps_t.u2Length = sizeof(CMD_WTBL_SMPS_T);

		if (ucAtion == 0) {
			/* Set to 0 */
			debug_u->wtbl_smps_t.ucSmPs = 0;
			CmdExtWtblUpdate(pAd, u2Wcid, SET_WTBL, &debug_u->wtbl_smps_t, sizeof(CMD_WTBL_SMPS_T));
		} else if (ucAtion == 1) {
			/* Set to 1 */
			debug_u->wtbl_smps_t.ucSmPs = 1;
			CmdExtWtblUpdate(pAd, u2Wcid, SET_WTBL, &debug_u->wtbl_smps_t, sizeof(CMD_WTBL_SMPS_T));
		} else if (ucAtion == 2) {
			/* query */
			CmdExtWtblUpdate(pAd, u2Wcid, QUERY_WTBL, &debug_u->wtbl_smps_t, sizeof(CMD_WTBL_SMPS_T));
		} else
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s::Cmd Error\n", __func__));
	}

	/* tag 14 */
	if (ucCmdId == WTBL_RAW_DATA_RW) {
		debug_u->wtbl_raw_data_rw_t.u2Tag = WTBL_RAW_DATA_RW;
		debug_u->wtbl_raw_data_rw_t.u2Length = sizeof(CMD_WTBL_RAW_DATA_RW_T);

		if (ucAtion == 0) {
			/* Set to 0 */
			debug_u->wtbl_raw_data_rw_t.ucWtblIdx = 1;
			debug_u->wtbl_raw_data_rw_t.ucWhichDW = 0;
			debug_u->wtbl_raw_data_rw_t.u4DwMask = 0xffff00ff;
			debug_u->wtbl_raw_data_rw_t.u4DwValue = 0x12340078;
			CmdExtWtblUpdate(pAd,
					 u2Wcid,
					 SET_WTBL,
					 &debug_u->wtbl_raw_data_rw_t,
					 sizeof(CMD_WTBL_RAW_DATA_RW_T));
		} else if (ucAtion == 1) {
			/* Set to 1 */
			debug_u->wtbl_raw_data_rw_t.ucWtblIdx = 1;
			debug_u->wtbl_raw_data_rw_t.ucWhichDW = 0;
			debug_u->wtbl_raw_data_rw_t.u4DwMask = 0xffff00ff;
			debug_u->wtbl_raw_data_rw_t.u4DwValue = 0x12345678;
			CmdExtWtblUpdate(pAd,
					 u2Wcid,
					 SET_WTBL,
					 &debug_u->wtbl_raw_data_rw_t,
					 sizeof(CMD_WTBL_RAW_DATA_RW_T));
		} else if (ucAtion == 2) {
			/* query */
			debug_u->wtbl_raw_data_rw_t.ucWtblIdx = 1;
			CmdExtWtblUpdate(pAd,
					 u2Wcid,
					 QUERY_WTBL,
					 &debug_u->wtbl_raw_data_rw_t,
					 sizeof(CMD_WTBL_RAW_DATA_RW_T));
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF,
				("%s::rWtblRawDataRw.u4DwValue(%x)\n",
					__func__,
					debug_u->wtbl_raw_data_rw_t.u4DwValue));
		} else
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s::Cmd Error\n", __func__));
	}

	/* tag 15 */
	if (ucCmdId == WTBL_PN) {
		debug_u->wtbl_pn_t.u2Tag = WTBL_PN;
		debug_u->wtbl_pn_t.u2Length = sizeof(CMD_WTBL_PN_T);

		if (ucAtion == 0) {
			/* Set to 0 */
		} else if (ucAtion == 1) {
			/* Set to 1 */
			os_fill_mem(debug_u->wtbl_pn_t.aucPn, 6, 0xf);
			CmdExtWtblUpdate(pAd, u2Wcid, SET_WTBL, &debug_u->wtbl_pn_t, sizeof(CMD_WTBL_PN_T));
		} else if (ucAtion == 2) {
			/* query */
			CmdExtWtblUpdate(pAd, u2Wcid, QUERY_WTBL, &debug_u->wtbl_pn_t, sizeof(CMD_WTBL_PN_T));
			hex_dump("WTBL_PN", debug_u->wtbl_pn_t.aucPn, 6);
		}
	}
}


VOID MtAsicUpdateProtectByFw(struct _RTMP_ADAPTER *ad, VOID *cookie)
{
	struct _EXT_CMD_UPDATE_PROTECT_T fw_protect;
	MT_PROTECT_CTRL_T *protect = (MT_PROTECT_CTRL_T *)cookie;

	os_zero_mem(&fw_protect, sizeof(fw_protect));
	fw_protect.ucProtectIdx = UPDATE_PROTECTION_CTRL;
	fw_protect.ucDbdcIdx = protect->band_idx;
	fw_protect.Data.rUpdateProtect.ucLongNav = protect->long_nav;
	fw_protect.Data.rUpdateProtect.ucMMProtect = protect->mix_mode;
	fw_protect.Data.rUpdateProtect.ucGFProtect = protect->gf;
	fw_protect.Data.rUpdateProtect.ucBW40Protect = protect->bw40;
	fw_protect.Data.rUpdateProtect.ucRifsProtect = protect->rifs;
	fw_protect.Data.rUpdateProtect.ucBW80Protect = protect->bw80;
	fw_protect.Data.rUpdateProtect.ucBW160Protect = protect->bw160;
	fw_protect.Data.rUpdateProtect.ucERProtectMask = protect->erp_mask;
	MtCmdUpdateProtect(ad, &fw_protect);
}


VOID MtAsicUpdateRtsThldByFw(
	struct _RTMP_ADAPTER *pAd, struct wifi_dev *wdev, UCHAR pkt_num, UINT32 length)
{
	MT_RTS_THRESHOLD_T rts_thld = {0};

	rts_thld.band_idx = HcGetBandByWdev(wdev);
	rts_thld.pkt_num_thld = pkt_num;
	rts_thld.pkt_len_thld = length;
	if (MTK_REV_GTE(pAd, MT7615, MT7615E1) && MTK_REV_LT(pAd, MT7615, MT7615E3) && pAd->CommonCfg.dbdc_mode) {
		;/* DBDC does not support RTS setting */
	} else {
		struct _EXT_CMD_UPDATE_PROTECT_T fw_rts;

		os_zero_mem(&fw_rts, sizeof(fw_rts));
		fw_rts.ucProtectIdx = UPDATE_RTS_THRESHOLD;
		fw_rts.ucDbdcIdx = rts_thld.band_idx;
		fw_rts.Data.rUpdateRtsThld.u4RtsPktLenThreshold = cpu2le32(rts_thld.pkt_len_thld);
		fw_rts.Data.rUpdateRtsThld.u4RtsPktNumThreshold = cpu2le32(rts_thld.pkt_num_thld);
		MtCmdUpdateProtect(pAd, &fw_rts);
	}
}


INT MtAsicSetRDGByFw(RTMP_ADAPTER *pAd, MT_RDG_CTRL_T *Rdg)
{
	struct _EXT_CMD_RDG_CTRL_T fw_rdg;

	fw_rdg.u4TxOP = Rdg->Txop;
	fw_rdg.ucLongNav = Rdg->LongNav;
	fw_rdg.ucInit = Rdg->Init;
	fw_rdg.ucResp = Rdg->Resp;
	WCID_SET_H_L(fw_rdg.ucWlanIdxHnVer, fw_rdg.ucWlanIdxL, Rdg->WlanIdx);
	fw_rdg.ucBand = Rdg->BandIdx;
	MtCmdSetRdg(pAd, &fw_rdg);
	return TRUE;
}


#ifdef DBDC_MODE
INT32  MtAsicGetDbdcCtrlByFw(RTMP_ADAPTER *pAd, BCTRL_INFO_T *pbInfo)
{
	UINT32 ret;
	UINT32 i = 0, j = 0;
	/*DBDC enable will not need BctrlEntries so minus 1*/
	pbInfo->TotalNum = 0;
	/*PTA*/
	pbInfo->BctrlEntries[i].Type = DBDC_TYPE_PTA;
	pbInfo->BctrlEntries[i].Index = 0;
	i++;
	/*MU*/
	pbInfo->BctrlEntries[i].Type = DBDC_TYPE_MU;
	pbInfo->BctrlEntries[i].Index = 0;
	i++;

	/*BF*/
	for (j = 0; j < 3; j++) {
		pbInfo->BctrlEntries[i].Type = DBDC_TYPE_BF;
		pbInfo->BctrlEntries[i].Index = j;
		i++;
	}

	/*WMM*/
	for (j = 0; j < 4; j++) {
		pbInfo->BctrlEntries[i].Type = DBDC_TYPE_WMM;
		pbInfo->BctrlEntries[i].Index = j;
		i++;
	}

	/*MGMT*/
	for (j = 0; j < 2; j++) {
		pbInfo->BctrlEntries[i].Type = DBDC_TYPE_MGMT;
		pbInfo->BctrlEntries[i].Index = j;
		i++;
	}

	/*MBSS*/
	for (j = 0; j < 15; j++) {
		pbInfo->BctrlEntries[i].Type = DBDC_TYPE_MBSS;
		pbInfo->BctrlEntries[i].Index = j;
		i++;
	}

	/*BSS*/
	for (j = 0; j < 5; j++) {
		pbInfo->BctrlEntries[i].Type = DBDC_TYPE_BSS;
		pbInfo->BctrlEntries[i].Index = j;
		i++;
	}

	/*Repeater*/
	for (j = 0; j < 32; j++) {
		pbInfo->BctrlEntries[i].Type = DBDC_TYPE_REPEATER;
		pbInfo->BctrlEntries[i].Index = j;
		i++;
	}

	pbInfo->TotalNum = i;
	ret = MtCmdGetDbdcCtrl(pAd, pbInfo);
	return ret;
}


INT32 MtAsicSetDbdcCtrlByFw(RTMP_ADAPTER *pAd, BCTRL_INFO_T *pbInfo)
{
	UINT32 ret = 0;

	ret = MtCmdSetDbdcCtrl(pAd, pbInfo);
	return ret;
}

#endif /*DBDC_MODE*/

UINT32 MtAsicGetWmmParamByFw(RTMP_ADAPTER *pAd, UINT32 AcNum, UINT32 EdcaType)
{
	UINT32 ret, Value = 0;
	MT_EDCA_CTRL_T EdcaCtrl;

	os_zero_mem(&EdcaCtrl, sizeof(MT_EDCA_CTRL_T));
	EdcaCtrl.ucTotalNum = 1;
	EdcaCtrl.ucAction = EDCA_ACT_GET;
	EdcaCtrl.rAcParam[0].ucAcNum = AcNum;
	ret = MtCmdGetEdca(pAd, &EdcaCtrl);

	switch (EdcaType) {
	case WMM_PARAM_TXOP:
		Value = EdcaCtrl.rAcParam[0].u2Txop;
		break;

	case WMM_PARAM_AIFSN:
		Value = EdcaCtrl.rAcParam[0].ucAifs;
		break;

	case WMM_PARAM_CWMIN:
		Value = EdcaCtrl.rAcParam[0].ucWinMin;
		break;

	case WMM_PARAM_CWMAX:
		Value = EdcaCtrl.rAcParam[0].u2WinMax;
		break;

	default:
		Value = 0xdeadbeef;
		break;
	}

	return Value;
}

INT MtAsicSetWmmParam(RTMP_ADAPTER *pAd, UCHAR idx, UINT32 AcNum, UINT32 EdcaType, UINT32 EdcaValue)
{
	MT_EDCA_CTRL_T EdcaParam;
	P_TX_AC_PARAM_T pAcParam;
	UCHAR index = 0;

	/* Could write any queue by FW */
	if ((AcNum < 4) && (idx < 4))
		index = (idx * 4) + AcNum;
	else {
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
				 ("%s(): Non-WMM Queue, WmmIdx/QueIdx=%d/%d!\n",
				  __func__, idx, AcNum));
		index = AcNum;
	}

	os_zero_mem(&EdcaParam, sizeof(MT_EDCA_CTRL_T));
	EdcaParam.ucTotalNum = 1;
	EdcaParam.ucAction = EDCA_ACT_SET;
	pAcParam = &EdcaParam.rAcParam[0];
	pAcParam->ucAcNum = (UINT8)index;

	switch (EdcaType) {
	case WMM_PARAM_TXOP:
		pAcParam->ucVaildBit = CMD_EDCA_TXOP_BIT;
		pAcParam->u2Txop = (UINT16)EdcaValue;
		break;

	case WMM_PARAM_AIFSN:
		pAcParam->ucVaildBit = CMD_EDCA_AIFS_BIT;
		pAcParam->ucAifs = (UINT8)EdcaValue;
		break;

	case WMM_PARAM_CWMIN:
		pAcParam->ucVaildBit = CMD_EDCA_WIN_MIN_BIT;
		pAcParam->ucWinMin = (UINT8)EdcaValue;
		break;

	case WMM_PARAM_CWMAX:
		pAcParam->ucVaildBit = CMD_EDCA_WIN_MAX_BIT;
		pAcParam->u2WinMax = (UINT16)EdcaValue;
		break;

	default:
		MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s(%d): Error type=%d\n", __func__, __LINE__, EdcaType));
		break;
	}
	MtCmdEdcaParameterSet(pAd, EdcaParam);
	return NDIS_STATUS_SUCCESS;
}

VOID MtAsicSetEdcaParm(RTMP_ADAPTER *pAd, UCHAR idx, UCHAR tx_mode, PEDCA_PARM pEdcaParm)
{
	MT_EDCA_CTRL_T EdcaParam;
	P_TX_AC_PARAM_T pAcParam;
	UINT32 ac = 0, index = 0;

	os_zero_mem(&EdcaParam, sizeof(MT_EDCA_CTRL_T));

	if ((pEdcaParm != NULL) && (pEdcaParm->bValid != FALSE)) {
		EdcaParam.ucTotalNum = CMD_EDCA_AC_MAX;
		EdcaParam.ucTxModeValid = TRUE;
		EdcaParam.ucTxMode = tx_mode;

		for (ac = 0; ac < CMD_EDCA_AC_MAX;  ac++) {
			index = idx*4+ac;
			pAcParam = &EdcaParam.rAcParam[ac];
			pAcParam->ucVaildBit = CMD_EDCA_ALL_BITS;
			pAcParam->ucAcNum =  asic_get_hwq_from_ac(pAd, idx, ac);
			pAcParam->ucAifs = pEdcaParm->Aifsn[ac];
			pAcParam->ucWinMin = pEdcaParm->Cwmin[ac];
			pAcParam->u2WinMax = pEdcaParm->Cwmax[ac];
			pAcParam->u2Txop = pEdcaParm->Txop[ac];
		}
	}
	MtCmdEdcaParameterSet(pAd, EdcaParam);
}

INT MtAsicGetTsfTimeByFirmware(
	RTMP_ADAPTER *pAd,
	UINT32 *high_part,
	UINT32 *low_part,
	UCHAR HwBssidIdx)
{
	TSF_RESULT_T TsfResult;

	MtCmdGetTsfTime(pAd, HwBssidIdx, &TsfResult);
	*high_part = TsfResult.u4TsfBit63_32;
	*low_part = TsfResult.u4TsfBit0_31;
	return TRUE;
}

VOID MtAsicSetSlotTime(RTMP_ADAPTER *pAd, UINT32 SlotTime, UINT32 SifsTime, UCHAR BandIdx)
{
	UINT32 RifsTime = RIFS_TIME;
	UINT32 EifsTime = EIFS_TIME;

	MtCmdSlotTimeSet(pAd, (UINT8)SlotTime, (UINT8)SifsTime, (UINT8)RifsTime, (UINT16)EifsTime, BandIdx);
}

UINT32 MtAsicGetChBusyCntByFw(RTMP_ADAPTER *pAd, UCHAR ch_idx)
{
	UINT32 msdr16, ret;

	ret = MtCmdGetChBusyCnt(pAd, ch_idx, &msdr16);
	return msdr16;
}


INT32 MtAsicSetMacTxRxByFw(RTMP_ADAPTER *pAd, INT32 TxRx, BOOLEAN Enable, UCHAR BandIdx)
{
	UINT32 ret;

	ret = MtCmdSetMacTxRx(pAd, BandIdx, Enable);
	return ret;
}

INT32 MtAsicSetRxvFilter(RTMP_ADAPTER *pAd, BOOLEAN Enable, UCHAR BandIdx)
{
	UINT32 ret;

	ret = MtCmdSetRxvFilter(pAd, BandIdx, Enable);
	return ret;
}

VOID MtAsicDisableSyncByFw(struct _RTMP_ADAPTER *pAd, UCHAR HWBssidIdx)
{
	struct wifi_dev *wdev = NULL;
	UCHAR i;

	for (i = 0; i < WDEV_NUM_MAX; i++) {
		wdev = pAd->wdev_list[i];

		if (wdev != NULL) {
			if (wdev->OmacIdx == HWBssidIdx)
				break;
		} else
			continue;
	}

	/* ASSERT(wdev != NULL); */

	if (wdev == NULL)
		return;

	if (WDEV_BSS_STATE(wdev) == BSS_INIT) {
		MTWF_LOG(DBG_CAT_TX, DBG_SUBCAT_ALL, DBG_LVL_WARN, ("%s: BssInfo idx (%d) is INIT currently!!!\n",
				 __func__, wdev->bss_info_argument.ucBssIndex));
		return;
	}

	WDEV_BSS_STATE(wdev) = BSS_INITED;
	CmdSetSyncModeByBssInfoUpdate(pAd, &wdev->bss_info_argument);
}

VOID MtAsicEnableBssSyncByFw(
	struct _RTMP_ADAPTER *pAd,
	USHORT BeaconPeriod,
	UCHAR HWBssidIdx,
	UCHAR OPMode)
{
	struct wifi_dev *wdev = NULL;
	UCHAR i;

	for (i = 0; i < WDEV_NUM_MAX; i++) {
		wdev = pAd->wdev_list[i];

		if (wdev != NULL) {
			if (wdev->OmacIdx == HWBssidIdx)
				break;
		} else
			continue;
	}

	/* ASSERT(wdev != NULL); */

	if (wdev == NULL)
		return;

	if (WDEV_BSS_STATE(wdev) == BSS_INIT) {
		MTWF_LOG(DBG_CAT_TX, DBG_SUBCAT_ALL, DBG_LVL_WARN, ("%s: BssInfo idx (%d) is INIT currently!!!\n",
				 __func__, wdev->bss_info_argument.ucBssIndex));
		return;
	}

	WDEV_BSS_STATE(wdev) = BSS_ACTIVE;
	CmdSetSyncModeByBssInfoUpdate(pAd, &wdev->bss_info_argument);
}

#if defined(MT_MAC) && defined(TXBF_SUPPORT)
/* STARec Info */
INT32 MtAsicSetAid(
	RTMP_ADAPTER *pAd,
	UINT16 Aid,
	UINT8 OmacIdx)
{
	return CmdETxBfAidSetting(pAd,
							  Aid);
}
#endif

#ifdef APCLI_SUPPORT
#ifdef MAC_REPEATER_SUPPORT
/* TODO: Carter/Star for Repeater can support DBDC, after define STA/APCLI/Repeater */
INT MtAsicSetReptFuncEnableByFw(RTMP_ADAPTER *pAd, BOOLEAN bEnable, UCHAR band_idx)
{
	EXT_CMD_MUAR_T config_muar;

	NdisZeroMemory(&config_muar, sizeof(EXT_CMD_MUAR_T));

	if (bEnable == TRUE)
		config_muar.ucMuarModeSel = MUAR_REPEATER;
	else
		config_muar.ucMuarModeSel = MUAR_NORMAL;
#if defined(MT7915)
	config_muar.ucBand = band_idx;
#endif
	MtCmdMuarConfigSet(pAd, (UCHAR *)&config_muar);
	return TRUE;
}

VOID MtAsicInsertRepeaterEntryByFw(
	IN PRTMP_ADAPTER pAd,
	IN UCHAR CliIdx,
	IN PUCHAR pAddr)
{
	UCHAR *pdata = NULL;
	EXT_CMD_MUAR_T config_muar;
	EXT_CMD_MUAR_MULTI_ENTRY_T muar_entry;
	REPEATER_CLIENT_ENTRY *pReptEntry = NULL;

	NdisZeroMemory(&config_muar, sizeof(EXT_CMD_MUAR_T));
	NdisZeroMemory(&muar_entry, sizeof(EXT_CMD_MUAR_MULTI_ENTRY_T));
	os_alloc_mem(pAd,
				 (UCHAR **)&pdata,
				 sizeof(EXT_CMD_MUAR_T) + sizeof(EXT_CMD_MUAR_MULTI_ENTRY_T));
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			 ("\n%s %02x:%02x:%02x:%02x:%02x:%02x-CliIdx(%d)\n",
			  __func__,
			  pAddr[0],
			  pAddr[1],
			  pAddr[2],
			  pAddr[3],
			  pAddr[4],
			  pAddr[5],
			  CliIdx));

	pReptEntry = &pAd->ApCfg.pRepeaterCliPool[CliIdx];
	config_muar.ucMuarModeSel = MUAR_REPEATER;
	config_muar.ucEntryCnt = 1;
	config_muar.ucAccessMode = MUAR_WRITE;
#if defined(MT7915)
	config_muar.ucBand = HcGetBandByWdev(&pReptEntry->wdev);
#endif
	muar_entry.ucMuarIdx = ReptGetMuarIdxByCliIdx(pAd, CliIdx);
	COPY_MAC_ADDR(muar_entry.aucMacAddr, pAddr);
	NdisMoveMemory(pdata, &config_muar, sizeof(EXT_CMD_MUAR_T));
	NdisMoveMemory(pdata + sizeof(EXT_CMD_MUAR_T),
				   &muar_entry,
				   sizeof(EXT_CMD_MUAR_MULTI_ENTRY_T));
	MtCmdMuarConfigSet(pAd, (UCHAR *)pdata);
	os_free_mem(pdata);
}


VOID MtAsicRemoveRepeaterEntryByFw(RTMP_ADAPTER *pAd, UCHAR CliIdx)
{
	UCHAR *pdata = NULL;
	UCHAR *ptr = NULL;
	UCHAR i = 0;
	UCHAR zeroMac[MAC_ADDR_LEN] = {0};
	EXT_CMD_MUAR_T config_muar;
	EXT_CMD_MUAR_MULTI_ENTRY_T muar_entry;
	REPEATER_CLIENT_ENTRY *pReptEntry = NULL;

	NdisZeroMemory(&config_muar, sizeof(EXT_CMD_MUAR_T));
	NdisZeroMemory(&muar_entry, sizeof(EXT_CMD_MUAR_MULTI_ENTRY_T));
	pReptEntry = &pAd->ApCfg.pRepeaterCliPool[CliIdx];
	config_muar.ucMuarModeSel = MUAR_REPEATER;
	config_muar.ucEntryCnt = 2;
	config_muar.ucAccessMode = MUAR_WRITE;
#if defined(MT7915)
	config_muar.ucBand = HcGetBandByWdev(&pReptEntry->wdev);
#endif
	os_alloc_mem(pAd,
				 (UCHAR **)&pdata,
				 sizeof(EXT_CMD_MUAR_T) +
				 (config_muar.ucEntryCnt * sizeof(EXT_CMD_MUAR_MULTI_ENTRY_T)));
	ptr = pdata;
	NdisMoveMemory(pdata, &config_muar, sizeof(EXT_CMD_MUAR_T));
	ptr = pdata + sizeof(EXT_CMD_MUAR_T);

	for (i = 0; i < config_muar.ucEntryCnt; i++) {
		muar_entry.ucMuarIdx = ReptGetMuarIdxByCliIdx(pAd, CliIdx) + i;
		COPY_MAC_ADDR(muar_entry.aucMacAddr, zeroMac);
		NdisMoveMemory(ptr,
					   &muar_entry,
					   sizeof(EXT_CMD_MUAR_MULTI_ENTRY_T));
		ptr = ptr + sizeof(EXT_CMD_MUAR_MULTI_ENTRY_T);
	}

	MtCmdMuarConfigSet(pAd, (UCHAR *)pdata);
	os_free_mem(pdata);
}

VOID MtAsicInsertRepeaterRootEntryByFw(
	IN PRTMP_ADAPTER pAd,
	IN UINT16 Wcid,
	IN UCHAR *pAddr,
	IN UCHAR ReptCliIdx)
{
	UCHAR *pdata = NULL;
	EXT_CMD_MUAR_T config_muar;
	EXT_CMD_MUAR_MULTI_ENTRY_T muar_entry;
	REPEATER_CLIENT_ENTRY *pReptEntry = NULL;

	NdisZeroMemory(&config_muar, sizeof(EXT_CMD_MUAR_T));
	NdisZeroMemory(&muar_entry, sizeof(EXT_CMD_MUAR_MULTI_ENTRY_T));
	os_alloc_mem(pAd,
				 (UCHAR **)&pdata,
				 sizeof(EXT_CMD_MUAR_T) + sizeof(EXT_CMD_MUAR_MULTI_ENTRY_T));
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			 ("\n%s %02x:%02x:%02x:%02x:%02x:%02x-CliIdx(%d)\n",
			  __func__,
			  pAddr[0],
			  pAddr[1],
			  pAddr[2],
			  pAddr[3],
			  pAddr[4],
			  pAddr[5],
			  ReptCliIdx));

	pReptEntry = &pAd->ApCfg.pRepeaterCliPool[ReptCliIdx];
	config_muar.ucMuarModeSel = MUAR_REPEATER;
	config_muar.ucEntryCnt = 1;
	config_muar.ucAccessMode = MUAR_WRITE;
#if defined(MT7915)
	config_muar.ucBand = HcGetBandByWdev(&pReptEntry->wdev);
#endif
	muar_entry.ucMuarIdx = ReptGetMuarIdxByCliIdx(pAd, ReptCliIdx) + 1;
	COPY_MAC_ADDR(muar_entry.aucMacAddr, pAddr);
	NdisMoveMemory(pdata, &config_muar, sizeof(EXT_CMD_MUAR_T));
	NdisMoveMemory(pdata + sizeof(EXT_CMD_MUAR_T),
				   &muar_entry,
				   sizeof(EXT_CMD_MUAR_MULTI_ENTRY_T));
	MtCmdMuarConfigSet(pAd, (UCHAR *)pdata);
	os_free_mem(pdata);
}

#endif /* MAC_REPEATER_SUPPORT */
#endif /* APCLI_SUPPORT */

INT32 MtAsicRxHeaderTransCtl(RTMP_ADAPTER *pAd, BOOLEAN En, BOOLEAN ChkBssid, BOOLEAN InSVlan, BOOLEAN RmVlan, BOOLEAN SwPcP)
{
	return CmdRxHdrTransUpdate(pAd, En, ChkBssid, InSVlan, RmVlan, SwPcP);
}

INT32 MtAsicRxHeaderTaranBLCtl(RTMP_ADAPTER *pAd, UINT32 Index, BOOLEAN En, UINT32 EthType)
{
	return CmdRxHdrTransBLUpdate(pAd, Index, En, EthType);
}

#ifdef VLAN_SUPPORT
#define WF_MDP_BASE (0x820CD000)
#define MDP_VTR0                      (WF_MDP_BASE + 0x010)
#define MDP_VTR2                       (WF_MDP_BASE + 0x018)
#define MDP_VTR_GET_ADDR(_omac) \
			((_omac < 0x10) ? (MDP_VTR0 + ((_omac >> 1) << 2)) : (MDP_VTR2 + (((_omac - 0x10) >> 1) << 2)))
#define MDP_VTR_SET_VID(_omac, _tci, _vid) \
			((_omac % 2) ? ((_tci & 0xF000FFFF) | (_vid << 16)) : ((_tci & 0xFFFFF000) | _vid))
#define MDP_VTR_SET_PCP(_omac, _tci, _pcp) \
			((_omac % 2) ? ((_tci & 0x1FFFFFFF) | (_pcp << 29)) : ((_tci & 0xFFFF1FFF) | (_pcp << 13)))

INT32 mt_asic_update_vlan_id_by_fw(struct _RTMP_ADAPTER *ad, UCHAR band_idx, UINT8 omac_idx, UINT16 vid)
{
	UINT32 addr, vlantag;

	addr = MDP_VTR_GET_ADDR(omac_idx);
	MAC_IO_READ32(ad->hdev_ctrl, addr, &vlantag);
	vlantag = MDP_VTR_SET_VID(omac_idx, vlantag, vid);
	MAC_IO_WRITE32(ad->hdev_ctrl, addr, vlantag);

	return 0;
	/*return cmd_update_vlan_id(ad, band_idx, omac_idx, vid);*/
}

INT32 mt_asic_update_vlan_priority_by_fw(struct _RTMP_ADAPTER *ad, UCHAR band_idx, UINT8 omac_idx, UINT8 priority)
{
	UINT32 addr, vlantag;

	addr = MDP_VTR_GET_ADDR(omac_idx);
	MAC_IO_READ32(ad->hdev_ctrl, addr, &vlantag);
	vlantag = MDP_VTR_SET_PCP(omac_idx, vlantag, priority);
	MAC_IO_WRITE32(ad->hdev_ctrl, addr, vlantag);

	return 0;
	/*return cmd_update_vlan_priority(ad, band_idx, omac_idx, priority);*/
}
#endif
