/*
 ***************************************************************************
 * MediaTek Inc.
 *
 * All rights reserved. source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of MediaTek. Any attemp
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of MediaTek, Inc. is obtained.
 ***************************************************************************

    Module Name:
    cmm_info_element.c

    Abstract:

    Revision History:
    Who         When            What
    --------    ----------      --------------------------------------------
				2016-08-18		AP/APCLI/STA SYNC FSM Integration
*/

#include "rt_config.h"

VOID SupportRate(
	IN struct legacy_rate *rate,
	OUT PUCHAR *ppRates,
	OUT PUCHAR RatesLen,
	OUT PUCHAR pMaxSupportRate)
{
	INT i;
	UCHAR sup_rate_len, ext_rate_len, *sup_rate, *ext_rate;

	sup_rate_len = rate->sup_rate_len;
	ext_rate_len = rate->ext_rate_len;
	sup_rate = rate->sup_rate;
	ext_rate = rate->ext_rate;

	*pMaxSupportRate = 0;
	if ((sup_rate_len <= MAX_LEN_OF_SUPPORTED_RATES) && (sup_rate_len > 0)) {
		NdisMoveMemory(*ppRates, sup_rate, sup_rate_len);
		*RatesLen = sup_rate_len;
	} else {
		/* HT rate not ready yet. return true temporarily. rt2860c */
		/*MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("PeerAssocReqSanity - wrong IE_SUPP_RATES\n")); */
		*RatesLen = 8;
		*(*ppRates + 0) = 0x82;
		*(*ppRates + 1) = 0x84;
		*(*ppRates + 2) = 0x8b;
		*(*ppRates + 3) = 0x96;
		*(*ppRates + 4) = 0x12;
		*(*ppRates + 5) = 0x24;
		*(*ppRates + 6) = 0x48;
		*(*ppRates + 7) = 0x6c;
		MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_INFO, ("SUPP_RATES., Len=%d\n", sup_rate_len));
	}

	if (ext_rate_len + *RatesLen <= MAX_LEN_OF_SUPPORTED_RATES) {
		NdisMoveMemory((*ppRates + (ULONG)*RatesLen), ext_rate, ext_rate_len);
		*RatesLen = (*RatesLen) + ext_rate_len;
	} else {
		NdisMoveMemory((*ppRates + (ULONG)*RatesLen), ext_rate, MAX_LEN_OF_SUPPORTED_RATES - (*RatesLen));
		*RatesLen = MAX_LEN_OF_SUPPORTED_RATES;
	}


	for (i = 0; i < *RatesLen; i++) {
		if (*pMaxSupportRate < (*(*ppRates + i) & 0x7f))
			*pMaxSupportRate = (*(*ppRates + i) & 0x7f);
	}
}

#ifdef DOT11_N_SUPPORT
void build_ext_channel_switch_ie(
	IN PRTMP_ADAPTER pAd,
	IN HT_EXT_CHANNEL_SWITCH_ANNOUNCEMENT_IE * pIE,
	IN UCHAR Channel,
	IN USHORT PhyMode,
	IN struct wifi_dev *wdev)
{
	struct DOT11_H *pDot11h = NULL;

	if (wdev == NULL)
		return;

	pDot11h = wdev->pDot11_H;
	if (pDot11h == NULL)
		return;
	pIE->ID = IE_EXT_CHANNEL_SWITCH_ANNOUNCEMENT;
	pIE->Length = 4;
	pIE->ChannelSwitchMode = 1;	/*no further frames */
	pIE->NewRegClass = get_regulatory_class(pAd, Channel, PhyMode, wdev);
	pIE->NewChannelNum = Channel;
	pIE->ChannelSwitchCount = (pDot11h->CSPeriod - pDot11h->CSCount - 1);
}
#endif /* DOT11_N_SUPPORT */

static INT build_wsc_probe_req_ie(RTMP_ADAPTER *pAd, struct wifi_dev *wdev, UCHAR *buf)
{
	INT len = 0;
#ifdef WSC_INCLUDED
	BOOLEAN bHasWscIe = FALSE;
#ifdef CONFIG_MAP_SUPPORT
	UCHAR ChBandIdx = HcGetBandByWdev(wdev);

/* To prevent WSC overlap in dbdc case ,disable wsc IE in probe request during all channel scan */
	if (IS_MAP_TURNKEY_ENABLE(pAd)) {
		if ((pAd->ScanCtrl[ChBandIdx].Channel != 0) &&
			(pAd->ScanCtrl[ChBandIdx].ScanReqwdev) &&
			(pAd->ScanCtrl[ChBandIdx].ScanReqwdev->wdev_type == WDEV_TYPE_STA))
			return len;
	}

#endif

	if (IF_COMBO_HAVE_AP_STA(pAd) && (wdev->wdev_type == WDEV_TYPE_STA)) {
#ifdef APCLI_SUPPORT
			/*
				Append WSC information in probe request if WSC state is running
			*/
			if ((wdev->WscControl.WscConfMode != WSC_DISABLE) &&
				(wdev->WscControl.bWscTrigger))
				bHasWscIe = TRUE;

#if defined(WSC_V2_SUPPORT) && !defined(CONFIG_MAP_SUPPORT) && !defined(CON_WPS)
			/* need to check if !defined(CONFIG_MAP_SUPPORT) is necessary */
			else if (wdev->WscControl.WscV2Info.bEnableWpsV2)
				bHasWscIe = TRUE;
#endif /* WSC_V2_SUPPORT */
#endif /* APCLI_SUPPORT */
#ifdef CON_WPS
		if (bHasWscIe) {
			PWSC_CTRL pWscControl = NULL;

			/* Do not include wsc ie in case concurrent WPS is running */
			bHasWscIe = FALSE;
			pWscControl = &wdev->WscControl;

			if ((pWscControl->conWscStatus == CON_WPS_STATUS_DISABLED) ||
			    (pAd->ApCfg.ConWpsApCliMode != CON_WPS_APCLI_BAND_AUTO))
				bHasWscIe = TRUE;

			MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_WARN,
				 ("[scan_active: %d] ConWpsApCliMode=%d conWscStatus=%d bHasWscIe=%d\n",
				  __LINE__, pAd->ApCfg.ConWpsApCliMode,
				  pWscControl->conWscStatus, bHasWscIe));

		}
#endif /*CON_WPS*/
	} else if (wdev->wdev_type == WDEV_TYPE_STA) {
#ifdef CONFIG_STA_SUPPORT
		PSTA_ADMIN_CONFIG pStaCfg = GetStaCfgByWdev(pAd, wdev);

		ASSERT(pStaCfg);
#ifdef WSC_STA_SUPPORT

		/*
			Append WSC information in probe request if WSC state is running
		*/
		if ((wdev->WscControl.WscEnProbeReqIE) &&
			(wdev->WscControl.WscConfMode != WSC_DISABLE) &&
			(wdev->WscControl.bWscTrigger == TRUE))
			bHasWscIe = TRUE;

#ifdef WSC_V2_SUPPORT
		else if ((wdev->WscControl.WscEnProbeReqIE) &&
				 (wdev->WscControl.WscV2Info.bEnableWpsV2))
			bHasWscIe = TRUE;

#endif /* WSC_V2_SUPPORT */

#endif /* WSC_STA_SUPPORT */
#endif /*CONFIG_STA_SUPPORT*/
	}

	if (bHasWscIe) {
		UCHAR *pWscBuf = NULL, WscIeLen = 0;

		os_alloc_mem(NULL, (UCHAR **)&pWscBuf, 512);

		if (pWscBuf != NULL) {
			NdisZeroMemory(pWscBuf, 512);
			WscBuildProbeReqIE(pAd, wdev, pWscBuf, &WscIeLen);
			MAKE_IE_TO_BUF(buf, pWscBuf, WscIeLen, len);
			os_free_mem(pWscBuf);
		} else
			MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_WARN, ("%s:: WscBuf Allocate failed!\n", __func__));
	}

#endif /* WSC_INCLUDED */
	return len;
}

static INT build_wsc_probe_rsp_ie(RTMP_ADAPTER *pAd, struct wifi_dev *wdev, UCHAR *buf)
{
	INT len = 0;

	switch (wdev->wdev_type) {
#ifdef WSC_AP_SUPPORT

	case WDEV_TYPE_AP: {
		/* for windows 7 logo test */
		if ((wdev->WscControl.WscConfMode != WSC_DISABLE) &&
#ifdef DOT1X_SUPPORT
			(!IS_IEEE8021X_Entry(wdev)) &&
#endif /* DOT1X_SUPPORT */
			(IS_CIPHER_WEP(wdev->SecConfig.PairwiseCipher))) {
			/*
				Non-WPS Windows XP and Vista PCs are unable to determine if a WEP enalbed network is static key based
				or 802.1X based. If the legacy station gets an EAP-Rquest/Identity from the AP, it assume the WEP
				network is 802.1X enabled & will prompt the user for 802.1X credentials. If the legacy station doesn't
				receive anything after sending an EAPOL-Start, it will assume the WEP network is static key based and
				prompt user for the WEP key. <<from "WPS and Static Key WEP Networks">>
				A WPS enabled AP should include this IE in the beacon when the AP is hosting a static WEP key network.
				The IE would be 7 bytes long with the Extended Capability field set to 0 (all bits zero)
				http://msdn.microsoft.com/library/default.asp?url=/library/en-us/randz/protocol/securing_public_wi-fi_hotspots.asp
			*/
			const UCHAR PROVISION_SERVICE_IE[7] = {0xDD, 0x05, 0x00, 0x50, 0xF2, 0x05, 0x00};

			MAKE_IE_TO_BUF(buf, PROVISION_SERVICE_IE, 7, len);
		}

		/* add Simple Config Information Element */
		if ((wdev->WscControl.WscConfMode > WSC_DISABLE) && (wdev->WscIEProbeResp.ValueLen))
			MAKE_IE_TO_BUF(buf, wdev->WscIEProbeResp.Value, wdev->WscIEProbeResp.ValueLen, len);

		break;
	}

#endif /* WSC_AP_SUPPORT */
#ifdef WSC_STA_SUPPORT

	case WDEV_TYPE_ADHOC: {
		/* PSTA_ADMIN_CONFIG pStaCfg = GetStaCfgByWdev(pAd,wdev); */
		/* add Simple Config Information Element */
		if (wdev->WscIEProbeResp.ValueLen != 0)
			MAKE_IE_TO_BUF(buf, wdev->WscIEProbeResp.Value, wdev->WscIEProbeResp.ValueLen, len);

		break;
	}

#endif /* WSC_STA_SUPPORT */
	}

	return len;
}

static INT build_wsc_assoc_req_ie(RTMP_ADAPTER *pAd, struct wifi_dev *wdev, UCHAR *buf)
{
	INT len = 0;
#ifdef WSC_STA_SUPPORT

	/* Add WSC IE if we are connecting to WSC AP */
	if ((wdev->WscControl.WscEnAssociateIE) &&
		(wdev->WscControl.WscConfMode != WSC_DISABLE) &&
		(wdev->WscControl.bWscTrigger)) {
		UCHAR *pWscBuf = NULL, WscIeLen = 0;

		os_alloc_mem(pAd, (UCHAR **) &pWscBuf, 512);

		if (pWscBuf != NULL) {
			NdisZeroMemory(pWscBuf, 512);
			WscBuildAssocReqIE(&wdev->WscControl, pWscBuf, &WscIeLen);
			MAKE_IE_TO_BUF(buf, pWscBuf, WscIeLen, len);
			os_free_mem(pWscBuf);
		} else
			MTWF_LOG(DBG_CAT_CLIENT, DBG_SUBCAT_ALL, DBG_LVL_WARN,
					 ("%s:: WscBuf Allocate failed!\n",
					  __func__));
	}

#endif /* WSC_STA_SUPPORT */
	return len;
}

INT build_wsc_ie(RTMP_ADAPTER *pAd, struct _build_ie_info *info)
{
	INT len = 0;

	if (info->frame_subtype == SUBTYPE_PROBE_REQ)
		len += build_wsc_probe_req_ie(pAd, info->wdev, (UCHAR *)(info->frame_buf + len));
	else if (info->frame_subtype == SUBTYPE_PROBE_RSP)
		len += build_wsc_probe_rsp_ie(pAd, info->wdev, (UCHAR *)(info->frame_buf + len));
	else if (info->frame_subtype == SUBTYPE_ASSOC_REQ)
		len += build_wsc_assoc_req_ie(pAd, info->wdev, (UCHAR *)(info->frame_buf + len));

	return len;
}

/* rsp */
INT build_rsn_ie(RTMP_ADAPTER *pAd, struct wifi_dev *wdev, UCHAR *buf)
{
	INT len = 0;
	struct _SECURITY_CONFIG *pSecConfig = NULL;

	pSecConfig = &wdev->SecConfig;

	switch (wdev->wdev_type) {
#ifdef CONFIG_AP_SUPPORT

	case WDEV_TYPE_AP: {
		BSS_STRUCT *mbss;
		CHAR rsne_idx = 0;

		mbss = wdev->func_dev;
#ifdef CONFIG_HOTSPOT_R2

		if ((mbss->HotSpotCtrl.HotSpotEnable == 0) &&
			(mbss->HotSpotCtrl.bASANEnable == 1) &&
			(IS_AKM_WPA2_Entry(wdev))) {
			/* replace RSN IE with OSEN IE if it's OSEN wdev */
			extern UCHAR OSEN_IE[];
			extern UCHAR OSEN_IELEN;
			UCHAR RSNIe = IE_WPA;

			MAKE_IE_TO_BUF(buf, &RSNIe, 1, len);
			MAKE_IE_TO_BUF(buf, &OSEN_IELEN,  1, len);
			MAKE_IE_TO_BUF(buf, OSEN_IE, OSEN_IELEN, len);
		} else
#endif /* CONFIG_HOTSPOT_R2 */
		{
			for (rsne_idx = 0; rsne_idx < SEC_RSNIE_NUM; rsne_idx++) {
				if (pSecConfig->RSNE_Type[rsne_idx] == SEC_RSNIE_NONE)
					continue;

				MAKE_IE_TO_BUF(buf, &pSecConfig->RSNE_EID[rsne_idx][0], 1, len);
				MAKE_IE_TO_BUF(buf, &pSecConfig->RSNE_Len[rsne_idx], 1, len);
				MAKE_IE_TO_BUF(buf, &pSecConfig->RSNE_Content[rsne_idx][0],
							   pSecConfig->RSNE_Len[rsne_idx], len);
			}
		}

		break;
	}

#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT

	case WDEV_TYPE_ADHOC: {
		PSTA_ADMIN_CONFIG pStaCfg = GetStaCfgByWdev(pAd, wdev);
		UCHAR RSNIe = IE_WPA;

		/* Modify by Eddy, support WPA2PSK in Adhoc mode */
		if (IS_AKM_WPANONE(wdev->SecConfig.AKMMap)
#ifdef ADHOC_WPA2PSK_SUPPORT
			|| IS_AKM_WPA2PSK(wdev->SecConfig.AKMMap)
#endif /* ADHOC_WPA2PSK_SUPPORT */
		   ) {
#ifdef ADHOC_WPA2PSK_SUPPORT
			RTMPMakeRSNIE(pAd, wdev->AuthMode, wdev->WepStatus, BSS0);

			if (IS_AKM_WPA2PSK(wdev->SecConfig.AKMMap))
				RSNIe = IE_RSN;

#endif /* ADHOC_WPA2PSK_SUPPORT */
			MAKE_IE_TO_BUF(buf, &RSNIe, 1, len);
			MAKE_IE_TO_BUF(buf, &pStaCfg->RSNIE_Len, 1, len);
			MAKE_IE_TO_BUF(buf, pStaCfg->RSN_IE, pStaCfg->RSNIE_Len, len);
		}

		break;
	}

#endif /* CONFIG_STA_SUPPORT */
	}

	return len;
}


static INT build_extra_probe_req_ie(RTMP_ADAPTER *pAd, struct wifi_dev *wdev, UCHAR *buf)
{
	INT len = 0;
	SCAN_INFO *ScanInfo = &wdev->ScanInfo;
#ifdef CONFIG_STA_SUPPORT
#ifdef RT_CFG80211_SUPPORT
	PSTA_ADMIN_CONFIG pStaCfg = GetStaCfgByWdev(pAd, wdev);

	if (wdev->wdev_type == WDEV_TYPE_STA)
		ASSERT(pStaCfg);
#ifdef WPA_SUPPLICANT_SUPPORT

	if ((wdev->wdev_type == WDEV_TYPE_STA) &&
		(pStaCfg->wpa_supplicant_info.WpaSupplicantUP != WPA_SUPPLICANT_DISABLE) &&
		(pStaCfg->wpa_supplicant_info.WpsProbeReqIeLen != 0)) {
		MAKE_IE_TO_BUF(buf, pStaCfg->wpa_supplicant_info.pWpsProbeReqIe,
					   pStaCfg->wpa_supplicant_info.WpsProbeReqIeLen, len);
	}

#endif /* WPA_SUPPLICANT_SUPPORT */

#if defined(WPA_SUPPLICANT_SUPPORT) || defined(APCLI_CFG80211_SUPPORT)
	if (pStaCfg &&
		(pStaCfg->wpa_supplicant_info.WpaSupplicantUP != WPA_SUPPLICANT_DISABLE) &&
		(pAd->cfg80211_ctrl.ExtraIeLen > 0)) {
		MAKE_IE_TO_BUF(buf, pAd->cfg80211_ctrl.pExtraIe,
					   pAd->cfg80211_ctrl.ExtraIeLen, len);
	}
#endif

#endif /* RT_CFG80211_SUPPORT */
#endif /*CONFIG_STA_SUPPORT*/

	if (ScanInfo->ExtraIeLen && ScanInfo->ExtraIe) {
		MAKE_IE_TO_BUF(buf, ScanInfo->ExtraIe,
					   ScanInfo->ExtraIeLen, len);
	}

	return len;
}

static INT build_extra_assoc_req_ie(RTMP_ADAPTER *pAd, struct wifi_dev *wdev, UCHAR *buf)
{
	INT len = 0;
#ifdef WPA_SUPPLICANT_SUPPORT
	PSTA_ADMIN_CONFIG pStaCfg = GetStaCfgByWdev(pAd, wdev);

	/*
		Can not use SIOCSIWGENIE definition, it is used in wireless.h
		We will not see the definition in MODULE.
		The definition can be saw in UTIL and NETIF.
	*/
	if ((pStaCfg->wpa_supplicant_info.WpaSupplicantUP & WPA_SUPPLICANT_ENABLE) &&
		(pStaCfg->wpa_supplicant_info.bRSN_IE_FromWpaSupplicant == TRUE)) {
		USHORT VarIesOffset = 0;

		MAKE_IE_TO_BUF(buf, pStaCfg->wpa_supplicant_info.pWpaAssocIe,
					   pStaCfg->wpa_supplicant_info.WpaAssocIeLen, len);
		VarIesOffset = pStaCfg->ReqVarIELen;
		NdisMoveMemory(pStaCfg->ReqVarIEs + VarIesOffset,
					   pStaCfg->wpa_supplicant_info.pWpaAssocIe,
					   pStaCfg->wpa_supplicant_info.WpaAssocIeLen);
		VarIesOffset += pStaCfg->wpa_supplicant_info.WpaAssocIeLen;
		/* Set Variable IEs Length */
		pStaCfg->ReqVarIELen = VarIesOffset;
	}

#endif /* WPA_SUPPLICANT_SUPPORT */
	return len;
}

INT build_extra_ie(RTMP_ADAPTER *pAd, struct _build_ie_info *info)
{
	INT len = 0;

	if (info->frame_subtype == SUBTYPE_PROBE_REQ)
		len += build_extra_probe_req_ie(pAd, info->wdev, (UCHAR *)(info->frame_buf + len));
	else if (info->frame_subtype == SUBTYPE_ASSOC_REQ)
		len += build_extra_assoc_req_ie(pAd, info->wdev, (UCHAR *)(info->frame_buf + len));

	return len;
}

#ifdef CONFIG_AP_SUPPORT
/* Extended Capabilities IE */
INT build_ap_extended_cap_ie(RTMP_ADAPTER *pAd, struct wifi_dev *wdev, UCHAR *buf)
{
	INT len = 0;
	ULONG infoPos;
	PUCHAR pInfo;
	BOOLEAN bNeedAppendExtIE = FALSE;
	UCHAR extInfoLen = sizeof(EXT_CAP_INFO_ELEMENT);
	BSS_STRUCT *mbss;
	EXT_CAP_INFO_ELEMENT extCapInfo = { 0 };

	/* NdisZeroMemory(&extCapInfo, extInfoLen); */
	mbss = wdev->func_dev;
#ifdef DOT11_N_SUPPORT
#ifdef DOT11N_DRAFT3

	/* P802.11n_D1.10, HT Information Exchange Support */
	if ((pAd->CommonCfg.bBssCoexEnable == TRUE) &&
		WMODE_CAP_N(wdev->PhyMode) && (wdev->channel <= 14) &&
		(wdev->DesiredHtPhyInfo.bHtEnable))
		extCapInfo.BssCoexistMgmtSupport = 1;

#endif /* DOT11N_DRAFT3 */
#endif /* DOT11_N_SUPPORT */
#ifdef CONFIG_DOT11V_WNM

	if (mbss->WNMCtrl.ProxyARPEnable)
		extCapInfo.proxy_arp = 1;

	if (mbss->WNMCtrl.WNMBTMEnable)
		extCapInfo.BssTransitionManmt = 1;

#ifdef CONFIG_HOTSPOT_R2

	if (mbss->WNMCtrl.WNMNotifyEnable)
		extCapInfo.wnm_notification = 1;

	if (mbss->HotSpotCtrl.QosMapEnable && mbss->HotSpotCtrl.HotSpotEnable)
		extCapInfo.qosmap = 1;

#endif /* CONFIG_HOTSPOT_R2 */
#endif /* CONFIG_DOT11V_WNM */

#if defined(WAPP_SUPPORT) || defined(FTM_SUPPORT) || defined(CONFIG_DOT11U_INTERWORKING)

	if (mbss->GASCtrl.b11U_enable)
		extCapInfo.interworking = 1;

#endif
#ifdef DOT11V_WNM_SUPPORT

	if (IS_BSS_TRANSIT_MANMT_SUPPORT(pAd, wdev->func_idx))
		extCapInfo.BssTransitionManmt = 1;

	if (IS_WNMDMS_SUPPORT(pAd, wdev->func_idx))
		extCapInfo.DMSSupport = 1;

#endif /* DOT11V_WNM_SUPPORT */
#ifdef DOT11_VHT_AC

	if (WMODE_CAP_AC(wdev->PhyMode) &&
		(wdev->channel > 14))
		extCapInfo.operating_mode_notification = 1;

#endif /* DOT11_VHT_AC */

#ifdef OCE_FILS_SUPPORT
	if (IS_AKM_FILS(wdev->SecConfig.AKMMap))
		extCapInfo.FILSCap = 1;
#endif /* OCE_FILS_SUPPORT */


#ifdef DOT11V_MBSSID_SUPPORT
	if (IS_MBSSID_IE_NEEDED(pAd, mbss, HcGetBandByWdev(wdev)))
		extCapInfo.mbssid = 1;
	else
		extCapInfo.mbssid = 0;

	if (IS_BSSID_11V_ENABLED(pAd, HcGetBandByWdev(wdev)))
		extCapInfo.cmpl_non_txbssid = 1;
	else
		extCapInfo.cmpl_non_txbssid = 0;
#endif


#ifdef DOT11_HE_AX
	if (WMODE_CAP_AX(wdev->PhyMode)) {
#ifdef WIFI_TWT_SUPPORT
		if (wdev->wdev_type == WDEV_TYPE_AP) {
			if (wlan_config_get_asic_twt_caps(wdev) &&
				(wlan_config_get_he_twt_support(wdev) >= TWT_SUPPORT_ENABLE))
				extCapInfo.twt_responder_support = 1;
		} else if (wdev->wdev_type == WDEV_TYPE_STA) {
			if (wlan_config_get_asic_twt_caps(wdev) &&
				(wlan_config_get_he_twt_support(wdev) >= TWT_SUPPORT_ENABLE))
				extCapInfo.twt_requester_support = 1;
		}
#endif /* WIFI_TWT_SUPPORT */
    }
#endif /* DOT11_HE_AX */

#ifdef DOT11_SAE_SUPPORT
	{
		struct _SECURITY_CONFIG *sec_cfg = &wdev->SecConfig;

		if (IS_AKM_SAE(sec_cfg->AKMMap) && sec_cfg->pwd_id_cnt != 0)
			extCapInfo.sae_pwd_id_in_use = 1;
		else
			extCapInfo.sae_pwd_id_in_use = 0;

		if (IS_AKM_SAE(sec_cfg->AKMMap) && sec_cfg->sae_cap.pwd_id_only)
			extCapInfo.sae_pwd_id_used_exclusively = 1;
		else
			extCapInfo.sae_pwd_id_used_exclusively = 0;
	}
#endif


#ifdef RT_BIG_ENDIAN
	RTMPEndianChange((UCHAR *)&extCapInfo, 8);
#endif

	pInfo = (PUCHAR)(&extCapInfo);

	for (infoPos = 0; infoPos < extInfoLen; infoPos++) {
		if (pInfo[infoPos] != 0) {
			bNeedAppendExtIE = TRUE;
			break;
		}
	}

	if (bNeedAppendExtIE == TRUE) {
		for (infoPos = (extInfoLen - 1); infoPos >= EXT_CAP_MIN_SAFE_LENGTH; infoPos--) {
			if (pInfo[infoPos] == 0)
				extInfoLen--;
			else
				break;
		}

		MAKE_IE_TO_BUF(buf, &ExtCapIe,   1, len);
		MAKE_IE_TO_BUF(buf, &extInfoLen, 1, len);
		MAKE_IE_TO_BUF(buf, &extCapInfo, extInfoLen, len);
	}

	return len;
}
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT
INT build_sta_extended_cap_ie(RTMP_ADAPTER *pAd, struct wifi_dev *wdev, UCHAR *buf)
{
	INT len = 0;
#if defined(DOT11N_DRAFT3) || defined(DOT11Z_TDLS_SUPPORT) || defined(CFG_TDLS_SUPPORT)
	{
		EXT_CAP_INFO_ELEMENT extCapInfo;
		UCHAR extInfoLen;
		ULONG infoPos;
		PUCHAR pInfo;
		BOOLEAN bNeedAppendExtIE = FALSE;

		extInfoLen = sizeof(EXT_CAP_INFO_ELEMENT);
		NdisZeroMemory(&extCapInfo, extInfoLen);
#ifdef DOT11N_DRAFT3

		if ((pAd->CommonCfg.bBssCoexEnable == TRUE) &&
			WMODE_CAP_N(wdev->PhyMode)
			&& (wdev->channel <= 14)
		   )
			extCapInfo.BssCoexistMgmtSupport = 1;

#endif /* DOT11N_DRAFT3 */
#ifdef MBO_SUPPORT
		extCapInfo.BssTransitionManmt = 1;
#endif /* MBO_SUPPORT */
#if defined(DOT11Z_TDLS_SUPPORT) || defined(CFG_TDLS_SUPPORT)

		if (IS_TDLS_SUPPORT(pAd)) {
			extCapInfo.UAPSDBufSTASupport = 1;
#ifdef CFG_TDLS_SUPPORT

			if (pStaCfg->wpa_supplicant_info.CFG_Tdls_info.TdlsChSwitchSupp)
#else
			if (pStaCfg->TdlsInfo.TdlsChSwitchSupp)
#endif
			{
				extCapInfo.TDLSChSwitchSupport = 1;
			} else
				extCapInfo.TDLSChSwitchSupport = 0;

			extCapInfo.TDLSSupport = 1;
		}

#endif /* defined(DOT11Z_TDLS_SUPPORT) || defined(CFG_TDLS_SUPPORT) */
#ifdef DOT11_VHT_AC

		if (WMODE_CAP_AC(wdev->PhyMode) &&
			(wdev->channel > 14))
			extCapInfo.operating_mode_notification = 1;

#endif /* DOT11_VHT_AC */

#ifdef DOT11_HE_AX
	if (WMODE_CAP_AX(wdev->PhyMode)) {
#ifdef DOT11V_MBSSID_SUPPORT
		extCapInfo.mbssid = 1;
#endif /* DOT11V_MBSSID_SUPPORT */
#ifdef WIFI_TWT_SUPPORT
		if (wdev->wdev_type == WDEV_TYPE_AP) {
			if (wlan_config_get_asic_twt_caps(wdev) &&
				(wlan_config_get_he_twt_support(wdev) >= TWT_SUPPORT_ENABLE))
				extCapInfo.twt_responder_support = 1;
		} else if (wdev->wdev_type == WDEV_TYPE_STA) {
			if (wlan_config_get_asic_twt_caps(wdev) &&
				(wlan_config_get_he_twt_support(wdev) >= TWT_SUPPORT_ENABLE))
				extCapInfo.twt_requester_support = 1;
		}
#endif /* WIFI_TWT_SUPPORT */
    }
#endif /* DOT11_HE_AX */

#ifdef DOT11_SAE_SUPPORT
		{
			struct _SECURITY_CONFIG *sec_cfg = &wdev->SecConfig;

			if (IS_AKM_SAE(sec_cfg->AKMMap) && sec_cfg->pwd_id_cnt != 0)
				extCapInfo.sae_pwd_id_in_use = 1;
			else
				extCapInfo.sae_pwd_id_in_use = 0;

			if (IS_AKM_SAE(sec_cfg->AKMMap) && sec_cfg->sae_cap.pwd_id_only)
				extCapInfo.sae_pwd_id_used_exclusively = 1;
			else
				extCapInfo.sae_pwd_id_used_exclusively = 0;
		}
#endif


		pInfo = (PUCHAR)(&extCapInfo);

		for (infoPos = 0; infoPos < extInfoLen; infoPos++) {
			if (pInfo[infoPos] != 0) {
				bNeedAppendExtIE = TRUE;
				break;
			}
		}

		if (bNeedAppendExtIE == TRUE) {
			for (infoPos = (extInfoLen - 1); infoPos >= EXT_CAP_MIN_SAFE_LENGTH; infoPos--) {
				if (pInfo[infoPos] == 0)
					extInfoLen--;
				else
					break;
			}
#ifdef RT_BIG_ENDIAN
			RTMPEndianChange((UCHAR *)&extCapInfo, 8);
#endif

			MAKE_IE_TO_BUF(buf, &ExtCapIe,   1, len);
			MAKE_IE_TO_BUF(buf, &extInfoLen, 1, len);
			MAKE_IE_TO_BUF(buf, &extCapInfo, extInfoLen, len);
		}
	}
#endif /* defined(DOT11N_DRAFT3) || defined(DOT11Z_TDLS_SUPPORT) */
	return len;
}
#endif /* CONFIG_STA_SUPPORT */

INT build_extended_cap_ie(RTMP_ADAPTER *pAd, struct _build_ie_info *info)
{
	INT len = 0;
	struct wifi_dev *wdev = info->wdev;

	switch (wdev->wdev_type) {
#ifdef CONFIG_STA_SUPPORT
	case WDEV_TYPE_STA:
	case WDEV_TYPE_ADHOC:
	case WDEV_TYPE_MESH:
		len += build_sta_extended_cap_ie(pAd, info->wdev, (UCHAR *)(info->frame_buf + len));
		break;
#endif /* CONFIG_STA_SUPPORT */
	default:
#ifdef CONFIG_AP_SUPPORT
		len += build_ap_extended_cap_ie(pAd, info->wdev, (UCHAR *)(info->frame_buf + len));
#endif /* CONFIG_AP_SUPPORT */
		break;
	}

	return len;
}

#ifdef CONFIG_AP_SUPPORT
static INT build_ap_wmm_cap_ie(RTMP_ADAPTER *pAd, struct wifi_dev *wdev, UCHAR *buf)
{
	INT len = 0;
	UCHAR i;
	UCHAR WmeParmIe[26] = {IE_VENDOR_SPECIFIC, 24, 0x00, 0x50, 0xf2, 0x02, 0x01, 0x01, 0, 0};
	UINT8 AIFSN[4];
	struct _EDCA_PARM *pBssEdca = wlan_config_get_ht_edca(wdev);

	if (pBssEdca) {
		WmeParmIe[8] = pBssEdca->EdcaUpdateCount & 0x0f;
#ifdef UAPSD_SUPPORT
		UAPSD_MR_IE_FILL(WmeParmIe[8], &wdev->UapsdInfo);
#endif /* UAPSD_SUPPORT */
		NdisMoveMemory(AIFSN, pBssEdca->Aifsn, sizeof(AIFSN));

		for (i = QID_AC_BK; i <= QID_AC_VO; i++) {
			WmeParmIe[10 + (i * 4)] = (i << 5)							  +	/* b5-6 is ACI */
									  ((UCHAR)pBssEdca->bACM[i] << 4)	  +	/* b4 is ACM */
									  (AIFSN[i] & 0x0f);						/* b0-3 is AIFSN */
			WmeParmIe[11 + (i * 4)] = (pBssEdca->Cwmax[i] << 4)		  +	/* b5-8 is CWMAX */
									  (pBssEdca->Cwmin[i] & 0x0f);				/* b0-3 is CWMIN */
			WmeParmIe[12 + (i * 4)] = (UCHAR)(pBssEdca->Txop[i] & 0xff);		/* low byte of TXOP */
			WmeParmIe[13 + (i * 4)] = (UCHAR)(pBssEdca->Txop[i] >> 8);			/* high byte of TXOP */
		}

		MAKE_IE_TO_BUF(buf, WmeParmIe, 26, len);
	}

	return len;
}
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
static INT build_sta_wmm_cap_ie(RTMP_ADAPTER *pAd, struct wifi_dev *wdev, UCHAR *buf)
{
	INT len = 0;
	UCHAR WmeIe[9] = {IE_VENDOR_SPECIFIC, 0x07, 0x00, 0x50, 0xf2, 0x02, 0x00, 0x01, 0x00};
	PSTA_ADMIN_CONFIG pStaCfg = GetStaCfgByWdev(pAd, wdev);

	if (pStaCfg->MlmeAux.APEdcaParm.bValid) {
		if (pStaCfg->wdev.UapsdInfo.bAPSDCapable
			&& pStaCfg->MlmeAux.APEdcaParm.bAPSDCapable) {
			QBSS_STA_INFO_PARM QosInfo;

			NdisZeroMemory(&QosInfo, sizeof(QBSS_STA_INFO_PARM));
			QosInfo.UAPSD_AC_BE = pAd->CommonCfg.bAPSDAC_BE;
			QosInfo.UAPSD_AC_BK = pAd->CommonCfg.bAPSDAC_BK;
			QosInfo.UAPSD_AC_VI = pAd->CommonCfg.bAPSDAC_VI;
			QosInfo.UAPSD_AC_VO = pAd->CommonCfg.bAPSDAC_VO;
			QosInfo.MaxSPLength = pAd->CommonCfg.MaxSPLength;
			WmeIe[8] |= *(PUCHAR)&QosInfo;
		} else {
			/* The Parameter Set Count is set to 0 in the association request frames */
			/* WmeIe[8] |= (pStaCfg->MlmeAux.APEdcaParm.EdcaUpdateCount & 0x0f); */
		}

		MAKE_IE_TO_BUF(buf, WmeIe, 9, len);
	}

	return len;
}
#endif /* CONFIG_STA_SUPPORT */

INT build_wmm_cap_ie(RTMP_ADAPTER *pAd, struct _build_ie_info *info)
{
	INT len = 0;
	struct wifi_dev *wdev = info->wdev;

	if (!wdev->bWmmCapable)
		return len;

	switch (wdev->wdev_type) {
#ifdef CONFIG_STA_SUPPORT
	case WDEV_TYPE_STA:
	case WDEV_TYPE_ADHOC:
	case WDEV_TYPE_MESH:
		len += build_sta_wmm_cap_ie(pAd, info->wdev, (UCHAR *)(info->frame_buf + len));
		break;
#endif /* CONFIG_STA_SUPPORT */

	default:
#ifdef CONFIG_AP_SUPPORT
		len += build_ap_wmm_cap_ie(pAd, info->wdev, (UCHAR *)(info->frame_buf + len));
#endif /* CONFIG_AP_SUPPORT */
		break;
	}

	return len;
}


#ifdef MBO_SUPPORT
INT build_supp_op_class_ie(RTMP_ADAPTER *pAd, struct wifi_dev *wdev, UCHAR *buf)
{
	ULONG TmpLen;
	UCHAR OperatingIe = IE_SUPP_REG_CLASS;
	UCHAR OperClassList = 0;
	UCHAR SuppClasslist[50];
	INT len = 0;
	UCHAR RegulatoryClass = get_regulatory_class(pAd, wdev->channel, wdev->PhyMode, wdev);

	OperClassList = get_operating_class_list(pAd, wdev->channel, wdev->PhyMode, wdev, SuppClasslist, &len);
	SuppClasslist[len] = 130;
	len += 2; /* to include Regulatory Class and Delimiter 0x82 */
	if (OperClassList == TRUE) {
		MakeOutgoingFrame(buf,    &TmpLen,
				1, &OperatingIe,
				1, &len,
				1, &RegulatoryClass,
				len-1, &SuppClasslist,
				END_OF_ARGS);
	}
	return TmpLen;
}
#endif /* MBO_SUPPORT */


ULONG build_support_rate_ie(struct wifi_dev *wdev, UCHAR *sup_rate, UCHAR sup_rate_len, UCHAR *buf)
{
	ULONG frame_len;
	ULONG bss_mem_selector_len = 0;
	UCHAR bss_mem_selector_code = BSS_MEMBERSHIP_SELECTOR_VALID | BSS_MEMBERSHIP_SELECTOR_SAE_H2E_ONLY;
	USHORT PhyMode = wdev->PhyMode;
	UCHAR real_sup_rate_len = sup_rate_len;
	UCHAR total_len;

	if (PhyMode == WMODE_B)
		real_sup_rate_len = 4;

#ifdef DOT11_SAE_SUPPORT
	if (wdev->SecConfig.sae_cap.gen_pwe_method == PWE_HASH_ONLY && real_sup_rate_len < 8)
		bss_mem_selector_len++;
#endif
	total_len = real_sup_rate_len + bss_mem_selector_len;

	MakeOutgoingFrame(buf, &frame_len, 1, &SupRateIe,
				1, &total_len,
				real_sup_rate_len, sup_rate,
				bss_mem_selector_len, &bss_mem_selector_code,
				END_OF_ARGS);

	return frame_len;
}


ULONG build_support_ext_rate_ie(struct wifi_dev *wdev, UCHAR sup_rate_len,
	UCHAR *ext_sup_rate, UCHAR ext_sup_rate_len, UCHAR *buf)
{
	ULONG frame_len = 0;
	ULONG bss_mem_selector_len = 0;
	UCHAR bss_mem_selector_code = BSS_MEMBERSHIP_SELECTOR_VALID | BSS_MEMBERSHIP_SELECTOR_SAE_H2E_ONLY;
	USHORT PhyMode = wdev->PhyMode;
	UCHAR total_len;

	if (PhyMode == WMODE_B)
		return frame_len;

#ifdef DOT11_SAE_SUPPORT
	if (sup_rate_len >= 8 && wdev->SecConfig.sae_cap.gen_pwe_method == PWE_HASH_ONLY)
		bss_mem_selector_len++;
#endif
	if (ext_sup_rate_len == 0 && bss_mem_selector_len == 0)
		return frame_len;

	total_len = ext_sup_rate_len + bss_mem_selector_len;

	MakeOutgoingFrame(buf, &frame_len, 1, &ExtRateIe,
				1, &total_len,
				ext_sup_rate_len, ext_sup_rate,
				bss_mem_selector_len, &bss_mem_selector_code,
				END_OF_ARGS);

	return frame_len;
}

VOID parse_support_rate_ie(struct legacy_rate *rate, EID_STRUCT *eid_ptr)
{
	UCHAR i = 0;

	if ((eid_ptr->Len <= MAX_LEN_OF_SUPPORTED_RATES) && (eid_ptr->Len > 0)) {
		rate->sup_rate_len = 0;
		for (i = 0; i < eid_ptr->Len; i++)
			if (eid_ptr->Octet[i] != (BSS_MEMBERSHIP_SELECTOR_VALID | BSS_MEMBERSHIP_SELECTOR_HT_PHY) &&
			    eid_ptr->Octet[i] != (BSS_MEMBERSHIP_SELECTOR_VALID | BSS_MEMBERSHIP_SELECTOR_VHT_PHY) &&
			    eid_ptr->Octet[i] != (BSS_MEMBERSHIP_SELECTOR_VALID | BSS_MEMBERSHIP_SELECTOR_SAE_H2E_ONLY) &&
			    eid_ptr->Octet[i] != (BSS_MEMBERSHIP_SELECTOR_VALID | BSS_MEMBERSHIP_SELECTOR_HE_PHY))
				rate->sup_rate[rate->sup_rate_len++] = eid_ptr->Octet[i];
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_INFO,
				 ("%s - IE_SUPP_RATES., Len=%d. Rates[0]=%x\n",
				 __func__, eid_ptr->Len, rate->sup_rate[0]));
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_INFO,
				 ("Rates[1]=%x %x %x %x %x %x %x\n",
				  rate->sup_rate[1], rate->sup_rate[2],
				  rate->sup_rate[3], rate->sup_rate[4],
				  rate->sup_rate[5], rate->sup_rate[6],
				  rate->sup_rate[7]));
	} else {
		UCHAR RateDefault[8] = { 0x82, 0x84, 0x8b, 0x96, 0x12, 0x24, 0x48, 0x6c };
		/* HT rate not ready yet. return true temporarily. rt2860c */
		/*MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("PeerAssocReqSanity - wrong IE_SUPP_RATES\n")); */
		NdisMoveMemory(rate->sup_rate, RateDefault, 8);
		rate->sup_rate_len = 8;
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
				 ("%s - wrong IE_SUPP_RATES., Len=%d\n",
				 __func__, eid_ptr->Len));
	}
}

VOID parse_support_ext_rate_ie(struct legacy_rate *rate, EID_STRUCT *eid_ptr)
{
	UINT16 i = 0;

	if (eid_ptr->Len > MAX_LEN_OF_SUPPORTED_RATES) {
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			("%s:ext support rate ie size(%d) is large than MAX_LEN_OF_SUPPORTED_RATE(%d))\n",
			__func__, eid_ptr->Len, MAX_LEN_OF_SUPPORTED_RATES));
		return;
	}

	rate->ext_rate_len = 0;
	for (i = 0; i < eid_ptr->Len; i++)
		if (eid_ptr->Octet[i] != (BSS_MEMBERSHIP_SELECTOR_VALID | BSS_MEMBERSHIP_SELECTOR_HT_PHY) &&
			eid_ptr->Octet[i] != (BSS_MEMBERSHIP_SELECTOR_VALID | BSS_MEMBERSHIP_SELECTOR_VHT_PHY) &&
			eid_ptr->Octet[i] != (BSS_MEMBERSHIP_SELECTOR_VALID | BSS_MEMBERSHIP_SELECTOR_SAE_H2E_ONLY) &&
			eid_ptr->Octet[i] != (BSS_MEMBERSHIP_SELECTOR_VALID | BSS_MEMBERSHIP_SELECTOR_HE_PHY))
			rate->ext_rate[rate->ext_rate_len++] = eid_ptr->Octet[i];
}
