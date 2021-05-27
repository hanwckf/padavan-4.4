/***************************************************************************
 * MediaTek Inc.
 * 4F, No. 2 Technology 5th Rd.
 * Science-based Industrial Park
 * Hsin-chu, Taiwan, R.O.C.
 *
 * (c) Copyright 1997-2017, MediaTek, Inc.
 *
 * All rights reserved. MediaTek source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of MediaTek. Any attemp
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of MediaTek Technology, Inc. is obtained.
 ***************************************************************************

*/

#include "rt_config.h"

UCHAR tmi_rate_map_cck_lp[] = {
	TMI_TX_RATE_CCK_1M_LP,
	TMI_TX_RATE_CCK_2M_LP,
	TMI_TX_RATE_CCK_5M_LP,
	TMI_TX_RATE_CCK_11M_LP,
};

UCHAR tmi_rate_map_cck_lp_size = ARRAY_SIZE(tmi_rate_map_cck_lp);

UCHAR tmi_rate_map_cck_sp[] = {
	TMI_TX_RATE_CCK_2M_SP,
	TMI_TX_RATE_CCK_5M_SP,
	TMI_TX_RATE_CCK_11M_SP,
};

UCHAR tmi_rate_map_cck_sp_size = ARRAY_SIZE(tmi_rate_map_cck_sp);

UCHAR tmi_rate_map_ofdm[] = {
	TMI_TX_RATE_OFDM_6M,
	TMI_TX_RATE_OFDM_9M,
	TMI_TX_RATE_OFDM_12M,
	TMI_TX_RATE_OFDM_18M,
	TMI_TX_RATE_OFDM_24M,
	TMI_TX_RATE_OFDM_36M,
	TMI_TX_RATE_OFDM_48M,
	TMI_TX_RATE_OFDM_54M,
};

UCHAR tmi_rate_map_ofdm_size = ARRAY_SIZE(tmi_rate_map_ofdm);

const UCHAR altx_filter_list[] = {
	SUBTYPE_ASSOC_REQ,
	SUBTYPE_ASSOC_RSP,
	SUBTYPE_REASSOC_REQ,
	SUBTYPE_REASSOC_RSP,
	SUBTYPE_PROBE_REQ,
	SUBTYPE_PROBE_RSP,
	SUBTYPE_ATIM,
	SUBTYPE_DISASSOC,
	SUBTYPE_AUTH,
	SUBTYPE_DEAUTH,
};

char *pkt_ft_str[] = {"cut_through", "store_forward", "cmd", "PDA_FW_Download"};
char *hdr_fmt_str[] = {
	"Non-80211-Frame",
	"Command-Frame",
	"Normal-80211-Frame",
	"enhanced-80211-Frame",
};

char *rmac_info_type_str[] = {
	"TXS",
	"RXV",
	"RxNormal",
	"DupRFB",
	"TMR",
	"Unknown",
};

char *rxd_pkt_type_str(INT pkt_type)
{
	if (pkt_type <= 0x04)
		return rmac_info_type_str[pkt_type];
	else
		return rmac_info_type_str[5];
}

BOOLEAN in_altx_filter_list(HEADER_802_11 *pHeader)
{
	UCHAR i;
	UCHAR sub_type = pHeader->FC.SubType;

	for (i = 0; i < (ARRAY_SIZE(altx_filter_list)); i++) {
		if (sub_type == altx_filter_list[i])
			return TRUE;
	}
	if (sub_type == SUBTYPE_ACTION) {
		UINT8 *ptr = (UINT8 *)pHeader;

		ptr += sizeof(HEADER_802_11);
		if (*ptr == CATEGORY_PUBLIC ||
			((*ptr == CATEGORY_BA) && (*(ptr+1) == ADDBA_RESP)))
			return TRUE;
	}


	return FALSE;
}

#ifdef CUT_THROUGH
INT mt_ct_get_hw_resource_free_num(RTMP_ADAPTER *pAd, struct wifi_dev *wdev, UCHAR resource_idx, UINT32 *free_num, UINT32 *free_token)
{
	UINT free_ring_num, free_token_num;
	struct _PCI_HIF_T *hif = hc_get_hif_ctrl(pAd->hdev_ctrl);
	struct hif_pci_tx_ring *tx_ring = pci_get_tx_ring_by_ridx(hif, resource_idx);
	PKT_TOKEN_CB *cb = hif->PktTokenCb;
	UINT8 band_idx = 0;
	struct token_tx_pkt_queue *que = NULL;

	if (wdev)
		band_idx = HcGetBandByWdev(wdev);
	else
		band_idx = 0;

	que = token_tx_get_queue_by_band(cb, band_idx);

	free_ring_num = pci_get_tx_resource_free_num_nolock(pAd, resource_idx);

	if (free_ring_num < tx_ring->tx_ring_low_water_mark) {
		pci_inc_resource_full_cnt(pAd, resource_idx);
		pci_set_resource_state(pAd, resource_idx, TX_RING_LOW);
		return NDIS_STATUS_RESOURCES;
	}
	free_ring_num = free_ring_num - tx_ring->tx_ring_low_water_mark + 1;
	free_token_num = token_tx_get_free_cnt(que);

	if (free_token_num < token_tx_get_lwmark(que)) {
		token_tx_inc_full_cnt(que);
		token_tx_set_state(que, TX_TOKEN_LOW);
		return NDIS_STATUS_FAILURE;
	}

	free_token_num = free_token_num - token_tx_get_lwmark(que) + 1;

	/* return free num which can be used */
	*free_num = (free_ring_num < free_token_num) ? free_ring_num:free_token_num;
	*free_token = free_token_num;
	return NDIS_STATUS_SUCCESS;
}
#endif

INT mt_sf_hw_tx(RTMP_ADAPTER *pAd, TX_BLK *tx_blk)
{
	USHORT free_cnt;

	if (!TX_BLK_TEST_FLAG(tx_blk, fTX_MCU_OFFLOAD))
		tx_bytes_calculate(pAd, tx_blk);

	asic_write_tmac_info(pAd, &tx_blk->HeaderBuf[0], tx_blk);
	asic_write_tx_resource(pAd, tx_blk, TRUE, &free_cnt);
	return NDIS_STATUS_SUCCESS;
}

#ifdef CONFIG_FWOWN_SUPPORT
VOID FwOwn(RTMP_ADAPTER *pAd)
{
	struct _RTMP_CHIP_OP *ops = hc_get_chip_ops(pAd->hdev_ctrl);

	if (ops && ops->fw_own)
		ops->fw_own(pAd);
}

INT32 DriverOwn(RTMP_ADAPTER *pAd)
{
	INT32 Ret = NDIS_STATUS_SUCCESS;
	struct _RTMP_CHIP_OP *ops = hc_get_chip_ops(pAd->hdev_ctrl);

	if (ops && ops->driver_own)
		Ret = ops->driver_own(pAd);

	return Ret;
}

BOOLEAN FwOwnSts(RTMP_ADAPTER *pAd)
{
	BOOLEAN Ret = FALSE;
	struct _RTMP_CHIP_OP *ops = hc_get_chip_ops(pAd->hdev_ctrl);

	if (ops && ops->fw_own_sts)
		Ret = ops->fw_own_sts(pAd);

	return Ret;
}

#endif /*CONFIG_FWOWN_SUPPORT*/

/*
*
*/
INT mt_sf_check_hw_resource(RTMP_ADAPTER *pAd, struct wifi_dev *wdev, UCHAR resource_idx)
{
	INT free_cnt;

	free_cnt = hif_get_tx_resource_free_num(pAd->hdev_ctrl, resource_idx);

	return free_cnt > 0 ? NDIS_STATUS_SUCCESS : NDIS_STATUS_FAILURE;
}


/*
*
*/
INT mt_sf_mlme_hw_tx(RTMP_ADAPTER *pAd, UCHAR *tmac_info, MAC_TX_INFO *info, HTTRANSMIT_SETTING *transmit, TX_BLK *tx_blk)
{
	UINT16 free_cnt = 1;
	struct _RTMP_CHIP_CAP *cap = hc_get_chip_cap(pAd->hdev_ctrl);
	UINT8 tx_hw_hdr_len = cap->tx_hw_hdr_len;
#ifdef RT_BIG_ENDIAN
	PHEADER_802_11 pHeader_802_11;
	pHeader_802_11 = (HEADER_802_11 *)(tx_blk->pSrcBufHeader + cap->tx_hw_hdr_len);
#endif
	asic_write_tmac_info_fixed_rate(pAd, tmac_info, info, transmit);
#ifdef RT_BIG_ENDIAN
	RTMPFrameEndianChange(pAd, (PUCHAR)pHeader_802_11, DIR_WRITE, FALSE);
	MTMacInfoEndianChange(pAd, tmac_info, TYPE_TMACINFO, sizeof(TMAC_TXD_L));
#endif
	tx_blk->pSrcBufData = tx_blk->pSrcBufHeader;
	NdisCopyMemory(&tx_blk->HeaderBuf[0], tx_blk->pSrcBufData, tx_hw_hdr_len);
	tx_blk->pSrcBufData += tx_hw_hdr_len;
	tx_blk->SrcBufLen -= tx_hw_hdr_len;
	tx_blk->MpduHeaderLen = 0;
	tx_blk->wifi_hdr_len = 0;
	tx_blk->HdrPadLen = 0;
	tx_blk->hw_rsv_len = 0;
	asic_write_tx_resource(pAd, tx_blk, TRUE, &free_cnt);
	return NDIS_STATUS_SUCCESS;
}

/*
*
*/
VOID ComposePsPoll(
	IN	RTMP_ADAPTER *pAd,
	IN	PPSPOLL_FRAME pPsPollFrame,
	IN	USHORT	Aid,
	IN	UCHAR *pBssid,
	IN	UCHAR *pTa)
{
	NdisZeroMemory(pPsPollFrame, sizeof(PSPOLL_FRAME));
	pPsPollFrame->FC.Type = FC_TYPE_CNTL;
	pPsPollFrame->FC.SubType = SUBTYPE_PS_POLL;
	pPsPollFrame->Aid = Aid | 0xC000;
	COPY_MAC_ADDR(pPsPollFrame->Bssid, pBssid);
	COPY_MAC_ADDR(pPsPollFrame->Ta, pTa);
}

