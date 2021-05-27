/****************************************************************************
 * Ralink Tech Inc.
 * Taiwan, R.O.C.
 *
 * (c) Copyright 2002, Ralink Technology, Inc.
 *
 * All rights reserved. Ralink's source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of Ralink Tech. Any attemp
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************/

/****************************************************************************

    Abstract:

    Support multi-BSS function.

    Note:
    1. Call RT28xx_MBSS_Init() in init function and
       call RT28xx_MBSS_Remove() in close function

    2. MAC of different BSS is initialized in APStartUp()

    3. BSS Index (0 ~ 15) of different rx packet is got in

    4. BSS Index (0 ~ 15) of different tx packet is assigned in

    5. BSS Index (0 ~ 15) of different BSS is got in tx_pkt_handle() by using

    6. BSS Index (0 ~ 15) of IOCTL command is put in pAd->OS_Cookie->ioctl_if

    7. Beacon of different BSS is enabled in APMakeAllBssBeacon() by writing 1
       to the register MAC_BSSID_DW1

    8. The number of MBSS can be 1, 2, 4, or 8

***************************************************************************/
#ifdef MBSS_SUPPORT


#include "rt_config.h"

#ifdef MULTI_PROFILE
INT	multi_profile_devname_req(struct _RTMP_ADAPTER *ad, UCHAR *final_name, UCHAR *ifidx);
#endif /*MULTI_PROFILE*/

extern struct wifi_dev_ops ap_wdev_ops;

#ifdef DOT11V_MBSSID_SUPPORT
UCHAR bssid_num_to_max_indicator(UCHAR bssid_num)
{
	UCHAR max_bssid_num = 1;
	UCHAR dot11v_max_bssid_indicator = 0;

	if (bssid_num > 0) {
		while (max_bssid_num < bssid_num) {
			dot11v_max_bssid_indicator++;
			max_bssid_num = 1 << dot11v_max_bssid_indicator;
		}
	}

	return dot11v_max_bssid_indicator;
}

VOID MBSS_init_11v_param(RTMP_ADAPTER *pAd)
{
	pAd->ApCfg.dot11v_trans_bss_idx[DBDC_BAND0] = MAIN_MBSSID;
	pAd->ApCfg.dot11v_max_bssid_indicator[DBDC_BAND0] =
		bssid_num_to_max_indicator(pAd->ApCfg.BssidNumPerBand[DBDC_BAND0]);

#ifdef DBDC_MODE
	pAd->ApCfg.dot11v_max_bssid_indicator[DBDC_BAND1] =
		bssid_num_to_max_indicator(pAd->ApCfg.BssidNumPerBand[DBDC_BAND1]);
#ifdef MULTI_PROFILE
	/* TODO: should be fixed because trans-bss of band1 may not align with band0 */
	if (pAd->FlgMbssInit == FALSE)
		pAd->ApCfg.dot11v_trans_bss_idx[DBDC_BAND1] = pAd->ApCfg.BssidNumPerBand[DBDC_BAND0];
#else
	pAd->ApCfg.dot11v_trans_bss_idx[DBDC_BAND1] = MAIN_MBSSID;
#endif /* MULTI_PROFILE */
#endif /* DBDC_MODE */
}
#endif

VOID mbss_fill_per_band_idx(RTMP_ADAPTER *pAd, BSS_STRUCT *pMbss)
{
	INT32 IdBss;
	BSS_STRUCT *pCurMbss;
	UCHAR ucDbdcIdx = HcGetBandByWdev(&pMbss->wdev);
	UCHAR channel = pMbss->wdev.channel;
	INT32 bssIdx = 0;

	for (IdBss = MAIN_MBSSID; IdBss < pAd->ApCfg.BssidNum; IdBss++) {
		pCurMbss = &pAd->ApCfg.MBSSID[IdBss];
		if (pCurMbss == pMbss)
			break;

		/* decision by channel becasue dbdcidx assigned after INF_UP */
		if (pCurMbss->wdev.channel == channel)
			bssIdx++;
	}

	/* apply per-band mbss index */
	pMbss->mbss_grp_idx = bssIdx;

	MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\tmbss%02d@Band%d(ch-%d), grp_idx = %d\n",
		pMbss->mbss_idx, ucDbdcIdx, channel, pMbss->mbss_grp_idx));
}

/*
 * create and initialize virtual network interfaces
 */
VOID mbss_create_vif(RTMP_ADAPTER *pAd, RTMP_OS_NETDEV_OP_HOOK *pNetDevOps, INT32 IdBss)
{
	PNET_DEV pDevNew;
	RTMP_OS_NETDEV_OP_HOOK netDevHook;
	struct _RTMP_CHIP_CAP *cap = hc_get_chip_cap(pAd->hdev_ctrl);
	struct wifi_dev *wdev;
	UINT32 MC_RowID = 0, IoctlIF = 0;
	char *dev_name = NULL;
	INT32 Ret;
	UCHAR ifidx = IdBss;
	UCHAR final_name[32] = "";
	BOOLEAN autoSuffix = TRUE;


	dev_name = get_dev_name_prefix(pAd, INT_MBSSID);

	if (dev_name == NULL) {
		MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			("get_dev_name_prefix error!\n"));
		return;
	}
	snprintf(final_name, sizeof(final_name), "%s", dev_name);

	if (pAd->FlgMbssInit == TRUE) {
		if (pAd->CommonCfg.wifi_cert) {
			INT32 idx;
			INT32 CurBssidNum[DBDC_BAND_NUM] = {0};
			INT32 CurBssidNumAll = 0;

			/* current bss count */
			for (idx = 0; idx < pAd->ApCfg.BssidNum; idx++) {
				wdev = &pAd->ApCfg.MBSSID[idx].wdev;
				if (wdev->if_dev != NULL) {
					/* per band bss number */
					CurBssidNum[HcGetBandByWdev(wdev)]++;
					/* total bss number */
					CurBssidNumAll++;
				}
			}

			/* ifidx needed to re-assign for devname req */
			if ((IdBss >= CurBssidNumAll) &&
				(CurBssidNum[DBDC_BAND0] < pAd->ApCfg.BssidNumPerBand[DBDC_BAND0])) {
				/* next ifidx to be assigned */
				ifidx = CurBssidNum[DBDC_BAND0];
				MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\%s, re-assign ifidx %d -> %d\n",
						 __func__, IdBss, ifidx));
			}
		}
	}

#ifdef MULTI_PROFILE
	multi_profile_devname_req(pAd, final_name, &ifidx);
	if (ifidx == 0)
		autoSuffix = FALSE;
#endif /*MULTI_PROFILE*/

	pDevNew = RtmpOSNetDevCreate(MC_RowID, &IoctlIF, INT_MBSSID, ifidx,
					 sizeof(struct mt_dev_priv), final_name, autoSuffix);

	if (pDevNew == NULL) {
		MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			("pDevNew == NULL!\n"));
		pAd->ApCfg.BssidNum = IdBss; /* re-assign new MBSS number */
		return;
	}
	MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_OFF,
			 ("Register MBSSID IF %d (%s)\n", IdBss, RTMP_OS_NETDEV_GET_DEVNAME(pDevNew)));

	wdev = &pAd->ApCfg.MBSSID[IdBss].wdev;
	Ret = wdev_init(pAd, wdev, WDEV_TYPE_AP, pDevNew, IdBss,
					(VOID *)&pAd->ApCfg.MBSSID[IdBss], (void *)pAd);

	if (!Ret) {
		MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
				 ("Assign wdev idx for %s failed, free net device!\n",
				 RTMP_OS_NETDEV_GET_DEVNAME(pDevNew)));
		RtmpOSNetDevFree(pDevNew);
		return;
	}

	Ret = wdev_ops_register(wdev, WDEV_TYPE_AP, &ap_wdev_ops,
							cap->qos.wmm_detect_method);

	if (!Ret) {
		MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
				 ("register wdev_ops %s failed, free net device!\n",
				  RTMP_OS_NETDEV_GET_DEVNAME(pDevNew)));
		RtmpOSNetDevFree(pDevNew);
		return;
	}

	RTMP_OS_NETDEV_SET_PRIV(pDevNew, pAd);
	RTMP_OS_NETDEV_SET_WDEV(pDevNew, wdev);
	/* init operation functions and flags */
	NdisCopyMemory(&netDevHook, pNetDevOps, sizeof(netDevHook));
	netDevHook.priv_flags = INT_MBSSID;
	netDevHook.needProtcted = TRUE;
	netDevHook.wdev = wdev;
	/* Init MAC address of virtual network interface */
	NdisMoveMemory(&netDevHook.devAddr[0], &wdev->bssid[0], MAC_ADDR_LEN);

#ifdef RT_CFG80211_SUPPORT
	{
		struct wireless_dev *pWdev;
		CFG80211_CB *p80211CB = pAd->pCfg80211_CB;
		UINT32 DevType = RT_CMD_80211_IFTYPE_AP;
		pWdev = kzalloc(sizeof(*pWdev), GFP_KERNEL);
		pDevNew->ieee80211_ptr = pWdev;
		pWdev->wiphy = p80211CB->pCfg80211_Wdev->wiphy;
#if (KERNEL_VERSION(4, 0, 0) <= LINUX_VERSION_CODE)
#ifdef WIFI_IAP_IW_SET_CHANNEL_FEATURE
		pWdev->wiphy->features |= NL80211_FEATURE_AP_MODE_CHAN_WIDTH_CHANGE;
#endif/*WIFI_IAP_IW_SET_CHANNEL_FEATURE*/
#endif/*KERNEL_VERSION(4, 0, 0) */
		SET_NETDEV_DEV(pDevNew, wiphy_dev(pWdev->wiphy));
		pWdev->netdev = pDevNew;
		pWdev->iftype = DevType;
	}
#endif /* RT_CFG80211_SUPPORT */
#ifdef CONFIG_MAP_SUPPORT
		if (IS_MAP_TURNKEY_ENABLE(pAd)) {
			if (wdev && wdev->wdev_type == WDEV_TYPE_AP)
				map_make_vend_ie(pAd, IdBss);
		}
#endif /* CONFIG_MAP_SUPPORT */

	/* register this device to OS */
	if (RtmpOSNetDevAttach(pAd->OpMode, pDevNew, &netDevHook) != NDIS_STATUS_SUCCESS) {
		MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_OFF,
				 ("%s, create IF %d (%s) failed!!\n",
				 __func__, IdBss, RTMP_OS_NETDEV_GET_DEVNAME(pDevNew)));
	}


}

/*
 * ========================================================================
 * Routine Description:
 *     Initialize Multi-BSS function.
 *
 * Arguments:
 *     pAd			points to our adapter
 *     pDevMain		points to the main BSS network interface
 *
 * Return Value:
 *     None
 *
 * Note:
 *   1. Only create and initialize virtual network interfaces.
 *   2. No main network interface here.
 *   3. If you down ra0 and modify the BssNum of RT2860AP.dat/RT2870AP.dat,
 *      it will not work! You must rmmod rt2860ap.ko and lsmod rt2860ap.ko again.
 * ========================================================================
 */
VOID MBSS_Init(RTMP_ADAPTER *pAd, RTMP_OS_NETDEV_OP_HOOK *pNetDevOps)
{
	INT32 IdBss, MaxNumBss;
	INT32 CurBssNum = 0;


	/* max bss number */
	MaxNumBss = pAd->ApCfg.BssidNum;
	if (!VALID_MBSS(pAd, MaxNumBss))
		MaxNumBss = MAX_MBSSID_NUM(pAd);

#ifdef DOT11V_MBSSID_SUPPORT
	MBSS_init_11v_param(pAd);
#endif

	/* sanity check to avoid redundant virtual interfaces are created */
	if (pAd->FlgMbssInit == TRUE) {
		if (pAd->CommonCfg.wifi_cert) {
			/* current bss count */
			for (IdBss = 0; IdBss < MaxNumBss; IdBss++) {
				if (pAd->ApCfg.MBSSID[IdBss].wdev.if_dev != NULL)
					CurBssNum++;
			}
			/* add new virtual network interface */
			for (IdBss = CurBssNum; IdBss < MaxNumBss; IdBss++) {
				MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_OFF,
						 ("Add MBSSID IF =%d\n", IdBss));
				mbss_create_vif(pAd, pNetDevOps, IdBss);
			}
		}
		return;
	}

	/* first IdBss must not be 0 (BSS0), must be 1 (BSS1) */
	for (IdBss = FIRST_MBSSID; IdBss < MaxNumBss; IdBss++) {
		pAd->ApCfg.MBSSID[IdBss].wdev.if_dev = NULL;
		pAd->ApCfg.MBSSID[IdBss].wdev.bcn_buf.BeaconPkt = NULL;

		/* create virtual network interface */
		mbss_create_vif(pAd, pNetDevOps, IdBss);
	}

	pAd->FlgMbssInit = TRUE;
}


/*
========================================================================
Routine Description:
    Remove Multi-BSS network interface.

Arguments:
	pAd			points to our adapter

Return Value:
    None

Note:
    FIRST_MBSSID = 1
    Main BSS is not removed here.
========================================================================
*/
VOID MBSS_Remove(RTMP_ADAPTER *pAd)
{
	struct wifi_dev *wdev;
	UINT IdBss;
	BSS_STRUCT *pMbss;
	INT32 MaxNumBss;

	if (!pAd)
		return;

	MaxNumBss = pAd->ApCfg.BssidNum;

	if (!VALID_MBSS(pAd, MaxNumBss))
		MaxNumBss = MAX_MBSSID_NUM(pAd);


	for (IdBss = FIRST_MBSSID; IdBss < MaxNumBss; IdBss++) {
		wdev = &pAd->ApCfg.MBSSID[IdBss].wdev;
		pMbss = &pAd->ApCfg.MBSSID[IdBss];

		if (pMbss)
			bcn_buf_deinit(pAd, &wdev->bcn_buf);

		if (wdev->if_dev) {
			RtmpOSNetDevProtect(1);
			RtmpOSNetDevDetach(wdev->if_dev);
			RtmpOSNetDevProtect(0);
			wdev_deinit(pAd, wdev);
			RtmpOSNetDevFree(wdev->if_dev);
			wdev->if_dev = NULL;
		}
	}
}


/*
========================================================================
Routine Description:
    Get multiple bss idx.

Arguments:
	pAd				points to our adapter
	pDev			which WLAN network interface

Return Value:
    0: close successfully
    otherwise: close fail

Note:
========================================================================
*/
INT32 RT28xx_MBSS_IdxGet(RTMP_ADAPTER *pAd, PNET_DEV pDev)
{
	INT32 BssId = -1;
	INT32 IdBss;

	if (!pAd || !pDev)
		return -1;

	for (IdBss = 0; IdBss < pAd->ApCfg.BssidNum; IdBss++) {
		if (pAd->ApCfg.MBSSID[IdBss].wdev.if_dev == pDev) {
			BssId = IdBss;
			break;
		}
	}

	return BssId;
}

#ifdef MT_MAC
INT32 ext_mbss_hw_cr_enable(PNET_DEV pDev)
{
	PRTMP_ADAPTER pAd;
	INT BssId;

	pAd = RTMP_OS_NETDEV_GET_PRIV(pDev);
	BssId = RT28xx_MBSS_IdxGet(pAd, pDev);
	MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("##### %s, BssId = %d\n", __func__, BssId));

	if (BssId < 0)
		return -1;

	if (!IS_HIF_TYPE(pAd, HIF_MT))
		return 0;

	AsicSetExtMbssEnableCR(pAd, BssId, TRUE);/* enable rmac 0_1~0_15 bit */
	AsicSetMbssHwCRSetting(pAd, BssId, TRUE);/* enable lp timing setting for 0_1~0_15 */
	return 0;
}


INT ext_mbss_hw_cr_disable(PNET_DEV pDev)
{
	PRTMP_ADAPTER pAd;
	INT BssId;

	pAd = RTMP_OS_NETDEV_GET_PRIV(pDev);
	BssId = RT28xx_MBSS_IdxGet(pAd, pDev);

	if (BssId < 0)
		return -1;

	if (!IS_HIF_TYPE(pAd, HIF_MT))
		return 0;

	AsicSetMbssHwCRSetting(pAd, BssId, FALSE);
	AsicSetExtMbssEnableCR(pAd, BssId, FALSE);
	return 0;
}
#endif /* MT_MAC */

#endif /* MBSS_SUPPORT */


