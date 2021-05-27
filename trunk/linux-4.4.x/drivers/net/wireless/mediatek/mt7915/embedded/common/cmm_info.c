/****************************************************************************
 * Ralink Tech Inc.
 * 4F, No. 2 Technology 5th Rd.
 * Science-based Industrial Park
 * Hsin-chu, Taiwan, R.O.C.
 * (c) Copyright 2002, Ralink Technology, Inc.
 *
 * All rights reserved. Ralink's source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of Ralink Tech. Any attemp
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of Ralink Technology, Inc. is obtained.
 ****************************************************************************

    Module Name:
	cmm_info.c

    Abstract:

    Revision History:
    Who          When          What
    ---------    ----------    ----------------------------------------------
 */

#include "rt_config.h"
#include "hdev/hdev_basic.h"
#include "rtmp.h"
#include "rtmp_def.h"

#define MCAST_WCID_TO_REMOVE 0 /* Pat: TODO */

INT MCSMappingRateTable[] = {
	2,  4, 11, 22, 12,  18,  24,  36, 48,  72,  96, 108, 109, 110, 111, 112,/* CCK and OFDM */
	13, 26, 39, 52, 78, 104, 117, 130, 26,  52,  78, 104, 156, 208, 234, 260,
	39, 78, 117, 156, 234, 312, 351, 390, /* BW 20, 800ns GI, MCS 0~23 */
	27, 54, 81, 108, 162, 216, 243, 270, 54, 108, 162, 216, 324, 432, 486, 540,
	81, 162, 243, 324, 486, 648, 729, 810, /* BW 40, 800ns GI, MCS 0~23 */
	14, 29, 43, 57, 87, 115, 130, 144, 29, 59,   87, 115, 173, 230, 260, 288,
	43, 87, 130, 173, 260, 317, 390, 433, /* BW 20, 400ns GI, MCS 0~23 */
	30, 60, 90, 120, 180, 240, 270, 300, 60, 120, 180, 240, 360, 480, 540, 600,
	90, 180, 270, 360, 540, 720, 810, 900, /* BW 40, 400ns GI, MCS 0~23 */

	/*for 11ac:20 Mhz 800ns GI*/
	6,  13, 19, 26,  39,  52,  58,  65,  78,  90,     /*1ss mcs 0~8*/
	13, 26, 39, 52,  78,  104, 117, 130, 156, 180,     /*2ss mcs 0~8*/
	19, 39, 58, 78,  117, 156, 175, 195, 234, 260,   /*3ss mcs 0~9*/
	26, 52, 78, 104, 156, 208, 234, 260, 312, 360,     /*4ss mcs 0~8*/

	/*for 11ac:40 Mhz 800ns GI*/
	13,	27,	40,	54,	 81,  108, 121, 135, 162, 180,   /*1ss mcs 0~9*/
	27,	54,	81,	108, 162, 216, 243, 270, 324, 360,   /*2ss mcs 0~9*/
	40,	81,	121, 162, 243, 324, 364, 405, 486, 540,  /*3ss mcs 0~9*/
	54,	108, 162, 216, 324, 432, 486, 540, 648, 720, /*4ss mcs 0~9*/

	/*for 11ac:80 Mhz 800ns GI*/
	29,	58,	87,	117, 175, 234, 263, 292, 351, 390,   /*1ss mcs 0~9*/
	58,	117, 175, 243, 351, 468, 526, 585, 702, 780, /*2ss mcs 0~9*/
	87,	175, 263, 351, 526, 702, 0,	877, 1053, 1170, /*3ss mcs 0~9*/
	117, 234, 351, 468, 702, 936, 1053, 1170, 1404, 1560, /*4ss mcs 0~9*/

	/*for 11ac:160 Mhz 800ns GI*/
	58,	117, 175, 234, 351, 468, 526, 585, 702, 780, /*1ss mcs 0~9*/
	117, 234, 351, 468, 702, 936, 1053, 1170, 1404, 1560, /*2ss mcs 0~9*/
	175, 351, 526, 702, 1053, 1404, 1579, 1755, 2160, 0, /*3ss mcs 0~8*/
	234, 468, 702, 936, 1404, 1872, 2106, 2340, 2808, 3120, /*4ss mcs 0~9*/

	/*for 11ac:20 Mhz 400ns GI*/
	7,	14,	21,	28,  43,  57,   65,	 72,  86,  100,    /*1ss mcs 0~8*/
	14,	28,	43,	57,	 86,  115,  130, 144, 173, 200,    /*2ss mcs 0~8*/
	21,	43,	65,	86,	 130, 173,  195, 216, 260, 288,  /*3ss mcs 0~9*/
	28,	57,	86,	115, 173, 231,  260, 288, 346, 400,    /*4ss mcs 0~8*/

	/*for 11ac:40 Mhz 400ns GI*/
	15,	30,	45,	60,	 90,  120,  135, 150, 180, 200,  /*1ss mcs 0~9*/
	30,	60,	90,	120, 180, 240,  270, 300, 360, 400,  /*2ss mcs 0~9*/
	45,	90,	135, 180, 270, 360,  405, 450, 540, 600, /*3ss mcs 0~9*/
	60,	120, 180, 240, 360, 480,  540, 600, 720, 800, /*4ss mcs 0~9*/

	/*for 11ac:80 Mhz 400ns GI*/
	32,	65,	97,	130, 195, 260,  292, 325, 390, 433,  /*1ss mcs 0~9*/
	65,	130, 195, 260, 390, 520,  585, 650, 780, 866, /*2ss mcs 0~9*/
	97,	195, 292, 390, 585, 780,  0,	 975, 1170, 1300, /*3ss mcs 0~9*/
	130, 260, 390, 520, 780, 1040,	1170, 1300, 1560, 1733, /*4ss mcs 0~9*/

	/*for 11ac:160 Mhz 400ns GI*/
	65,	130, 195, 260, 390, 520,  585, 650, 780, 866, /*1ss mcs 0~9*/
	130, 260, 390, 520, 780, 1040,	1170, 1300, 1560, 1733, /*2ss mcs 0~9*/
	195, 390, 585, 780, 1170, 1560,	1755, 1950, 2340, 0, /*3ss mcs 0~8*/
	260, 520, 780, 1040, 1560, 2080,	2340, 2600, 3120, 3466, /*4ss mcs 0~9*/

	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
	20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37
}; /* 3*3 */

#ifdef DOT11_HE_AX
#define MAX_NUM_HE_BANDWIDTHS 4
#define MAX_NUM_HE_SPATIAL_STREAMS 4
#define MAX_NUM_HE_MCS_ENTRIES 12
UINT16 he_mcs_phyrate_mapping_table[MAX_NUM_HE_BANDWIDTHS][MAX_NUM_HE_SPATIAL_STREAMS][MAX_NUM_HE_MCS_ENTRIES] = {
	{ /*20 Mhz*/
		/* 1 SS */
		{
			/* DCM 0*/
			 8,
			 17,
			 25,
			 34,
			 51,
			 68,
			 77,
			 86,
			 103,
			 114,
			 129,
			 143
		},
		/* 2 SS */
		{
			/* DCM 0 */
			 17,
			 34,
			 51,
			 68,
			 103,
			 137,
			 154,
			 172,
			 206,
			 229,
			 258,
			 286
		},
		/* 3 SS */
		{
			/* DCM 0 */
			 25,
			 51,
			 77,
			 103,
			 154,
			 206,
			 232,
			 258,
			 309,
			 344,
			 387,
			 430
		},
		/* 4 SS */
		{
			/* DCM 0 */
			 34,
			 68,
			 103,
			 137,
			 206,
			 275,
			 309,
			 344,
			 412,
			 458,
			 516,
			 573
		}
	},
	{ /*40 Mhz*/
		/* 1 SS */
		{
			/* DCM 0*/
			 17,
			 34,
			 51,
			 68,
			 103,
			 137,
			 154,
			 172,
			 206,
			 229,
			 258,
			 286
		},
		/* 2 SS */
		{
			/* DCM 0 */
			 34,
			 68,
			 103,
			 137,
			 206,
			 275,
			 309,
			 344,
			 412,
			 458,
			 516,
			 573

		},
		/* 3 SS */
		{
			/* DCM 0 */
			 51,
			 103,
			 154,
			 206,
			 309,
			 412,
			 464,
			 516,
			 619,
			 688,
			 774,
			 860

		},
		/* 4 SS */
		{
			/* DCM 0 */
			 68,
			 137,
			 206,
			 275,
			 412,
			 550,
			 619,
			 688,
			 825,
			 917,
			 1032,
			 1147
		}
	},
	{ /*80 Mhz*/
		/* 1 SS */
		{
			/* DCM 0*/
			 36,
			 72,
			 108,
			 144,
			 216,
			 288,
			 324,
			 360,
			 432,
			 480,
			 540,
			 600
		},
		/* 2 SS */
		{
			/* DCM 0 */
			 72,
			 144,
			 216,
			 288,
			 432,
			 576,
			 648,
			 720,
			 864,
			 960,
			 1080,
			 1201
		},
		/* 3 SS */
		{
			/* DCM 0 */
			 108,
			 216,
			 324,
			 432,
			 648,
			 864,
			 972,
			 1080,
			 1297,
			 1441,
			 1621,
			 1801
		},
		/* 4 SS */
		{
			/* DCM 0 */
			 144,
			 288,
			 432,
			 576,
			 864,
			 1152,
			 1297,
			 1141,
			 1729,
			 1921,
			 2161,
			 2401
		}
	},
	{ /*160 Mhz*/
		/* 1 SS */
		{
			/* DCM 0*/
			 72,
			 144,
			 216,
			 288,
			 432,
			 576,
			 648,
			 720,
			 864,
			 960,
			 1080,
			 1201
		},
		/* 2 SS */
		{
			/* DCM 0 */
			 144,
			 288,
			 432,
			 576,
			 864,
			 1152,
			 1297,
			 1441,
			 1729,
			 1921,
			 2161,
			 2401
		},
		/* 3 SS */
		{
			/* DCM 0 */
			 216,
			 432,
			 648,
			 864,
			 1297,
			 1729,
			 1945,
			 2161,
			 2594,
			 2882,
			 3242,
			 3602
		},
		/* 4 SS */
		{
			/* DCM 0 */
			 288,
			 576,
			 864,
			 1152,
			 1729,
			 2305,
			 2594,
			 2882,
			 3458,
			 3843,
			 4323,
			 4803
		},
	}

};

#endif /* DOT11_HE_AX */


#if defined(CONFIG_WIFI_PKT_FWD) || defined(CONFIG_WIFI_PKT_FWD_MODULE)

extern struct wifi_fwd_func_table wf_drv_tbl;

#endif
VOID show_tpinfo_host(RTMP_ADAPTER *pAd, UINT32 option, UINT32 param0, UINT32 param1);
#ifdef CONFIG_DBG_QDISC
extern void os_system_tx_queue_dump(PNET_DEV dev);
#endif

static UINT32 debug_lvl = DBG_LVL_OFF;
static UINT32 debug_cat = DBG_CAT_ALL;

/*
    ==========================================================================
    Description:
	Get Driver version.

    Return:
    ==========================================================================
*/
INT show_driverinfo_proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("Driver version: %s\n", AP_DRIVER_VERSION));
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("Driver version: %s\n", STA_DRIVER_VERSION));
#endif /* CONFIG_STA_SUPPORT */

	MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_OFF,
			 ("FW ver: 0x%x, HW ver: 0x%x, CHIP ID: 0x%x\n", pAd->FWVersion, pAd->HWVersion, pAd->ChipID));

	show_patch_info(pAd);
	show_fw_info(pAd);

	return TRUE;
}

#if defined(BB_SOC) && defined(TCSUPPORT_WLAN_SW_RPS)
extern int (*ecnt_set_wifi_rps_hook)(int RxOn, int WLanCPU, int TxOn, int LanCPU);
extern int rx_detect_flag;

INT	Set_RxMaxTraffic_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	ULONG flag;

	flag = os_str_tol(arg, 0, 10);
	printk("old pAd->rxThreshold = %d Mbps\n", pAd->rxThreshold);
	pAd->rxThreshold = flag;
	printk("new pAd->rxThreshold = %d Mbps\n", pAd->rxThreshold);

	return TRUE;
}

INT	Set_rx_detect_flag_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	ULONG flag;

	flag = os_str_tol(arg, 0, 10);
	rx_detect_flag = flag;
	printk("rx_detect_flag = %d\n", rx_detect_flag);
	if (rx_detect_flag == 0) {
		if (ecnt_set_wifi_rps_hook)
			ecnt_set_wifi_rps_hook(0, 0, 0, 0);
	}

	return TRUE;
}
#endif

/*
    ==========================================================================
    Description:
	Set Country Region.
	This command will not work, if the field of CountryRegion in eeprom is programmed.
    Return:
	TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT Set_CountryRegion_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	int retval;
	UCHAR BandIdx, IfIdx;
	CHANNEL_CTRL *pChCtrl;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	struct wifi_dev *wdev = NULL;
	IfIdx = pObj->ioctl_if;

	if (pObj->ioctl_if_type == INT_MBSSID || pObj->ioctl_if_type == INT_MAIN) {
#ifdef CONFIG_AP_SUPPORT
	wdev = &pAd->ApCfg.MBSSID[IfIdx].wdev;
#endif
	}
#ifdef CONFIG_STA_SUPPORT
	else if (pObj->ioctl_if_type == INT_APCLI) {
		wdev = &pAd->StaCfg[IfIdx].wdev;
	}
	else if (pObj->ioctl_if_type == INT_MSTA) {
		wdev = &pAd->StaCfg[IfIdx].wdev;
	}
#endif
	else {
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("[Set_CountryRegion_Proc]: pObj->ioctl_if_type = %d!!\n", pObj->ioctl_if_type));
		return FALSE;
	}

	/* Check RF lock Status */
	if (chip_check_rf_lock_down(pAd)) {
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s: RF lock down!! Cannot config CountryRegion status!!\n",
				 __func__));
		return TRUE;
	}

#ifdef EXT_BUILD_CHANNEL_LIST
	return -EOPNOTSUPP;
#endif /* EXT_BUILD_CHANNEL_LIST */
	retval = RT_CfgSetCountryRegion(pAd, arg, BAND_24G);

	if (retval == FALSE)
		return FALSE;

	/* If country region is set, driver needs to be reset*/
	/* Change channel state to NONE */
	BandIdx = HcGetBandByWdev(wdev);
	pChCtrl = hc_get_channel_ctrl(pAd->hdev_ctrl, BandIdx);
	hc_set_ChCtrlChListStat(pChCtrl, CH_LIST_STATE_NONE);
	BuildChannelList(pAd, wdev);
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Set_CountryRegion_Proc::(CountryRegion=%d)\n",
			 pAd->CommonCfg.CountryRegion));
	return TRUE;
}

/*
    ==========================================================================
    Description:
	Set Country Region for A band.
	This command will not work, if the field of CountryRegion in eeprom is programmed.
    Return:
	TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT Set_CountryRegionABand_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	int retval;
	UCHAR BandIdx, IfIdx;
	CHANNEL_CTRL *pChCtrl;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	struct wifi_dev *wdev = NULL;
	IfIdx = pObj->ioctl_if;

	if (pObj->ioctl_if_type == INT_MBSSID || pObj->ioctl_if_type == INT_MAIN) {
#ifdef CONFIG_AP_SUPPORT
	wdev = &pAd->ApCfg.MBSSID[IfIdx].wdev;
#endif
	}
#ifdef CONFIG_STA_SUPPORT
	else if (pObj->ioctl_if_type == INT_APCLI) {
		wdev = &pAd->StaCfg[IfIdx].wdev;
	}
	else if (pObj->ioctl_if_type == INT_MSTA) {
		wdev = &pAd->StaCfg[IfIdx].wdev;
	}
#endif
	else {
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("[Set_CountryRegionABand_Proc]: pObj->ioctl_if_type = %d!!\n", pObj->ioctl_if_type));
		return FALSE;
	}

	/* Check RF lock Status */
	if (chip_check_rf_lock_down(pAd)) {
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s: RF lock down!! Cannot config CountryRegion status!!\n",
				 __func__));
		return TRUE;
	}

#ifdef EXT_BUILD_CHANNEL_LIST
	return -EOPNOTSUPP;
#endif /* EXT_BUILD_CHANNEL_LIST */
	retval = RT_CfgSetCountryRegion(pAd, arg, BAND_5G);

	if (retval == FALSE)
		return FALSE;

	/* If Country Region is set, channel list needs to be rebuilt*/
	/* Change channel state to NONE */
	BandIdx = HcGetBandByWdev(wdev);
	pChCtrl = hc_get_channel_ctrl(pAd->hdev_ctrl, BandIdx);
	hc_set_ChCtrlChListStat(pChCtrl, CH_LIST_STATE_NONE);
	BuildChannelList(pAd, wdev);
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Set_CountryRegionABand_Proc::(CountryRegion=%d)\n",
			 pAd->CommonCfg.CountryRegionForABand));
	return TRUE;
}

INT	Set_Cmm_WirelessMode_Proc(
	IN RTMP_ADAPTER *pAd,
	IN RTMP_STRING *arg)
{
	INT	success = TRUE;
	LONG cfg_mode = os_str_tol(arg, 0, 10);
	USHORT wmode = cfgmode_2_wmode((UCHAR)cfg_mode);
	struct wifi_dev *wdev = NULL;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
#ifdef CONFIG_AP_SUPPORT
	UINT32 i = 0;
	struct wifi_dev *TmpWdev = NULL;
#endif
	CHANNEL_CTRL *pChCtrl;
	UCHAR BandIdx;

	if (!wmode_valid_and_correct(pAd, &wmode)) {
		success = FALSE;
		goto error;
	}

#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd) {
		wdev = &pAd->ApCfg.MBSSID[pObj->ioctl_if].wdev;
		wdev->PhyMode = wmode;
#ifdef MBSS_SUPPORT
		success = RT_CfgSetMbssWirelessMode(pAd, arg);

		if (!success)
			goto error;

		for (i = 0; i < pAd->ApCfg.BssidNum; i++) {
			TmpWdev = &pAd->ApCfg.MBSSID[i].wdev;

			/*update WmmCapable*/
			if (!wmode_band_equal(TmpWdev->PhyMode, wmode))
				continue;

			TmpWdev->bWmmCapable = pAd->ApCfg.MBSSID[i].bWmmCapableOrg;
		}

		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Set_Cmm_WirelessMode_Proc::(BSS%d=%d)\n", pObj->ioctl_if,
				 wdev->PhyMode));
#else
		success = RT_CfgSetWirelessMode(pAd, arg, wdev);

		if (!success)
			goto error;

#endif /*MBSS_SUPPORT*/
		HcAcquireRadioForWdev(pAd, wdev);
		/* Change channel state to NONE */
		BandIdx = HcGetBandByWdev(wdev);
		pChCtrl = hc_get_channel_ctrl(pAd->hdev_ctrl, BandIdx);
		hc_set_ChCtrlChListStat(pChCtrl, CH_LIST_STATE_NONE);
#ifdef EXT_BUILD_CHANNEL_LIST
		BuildChannelListEx(pAd, wdev);
#else
		BuildChannelList(pAd, wdev);
#endif
		RTMPSetPhyMode(pAd, wdev, wmode);
		RTMPUpdateRateInfo(wmode, &wdev->rate);
#ifdef CONFIG_RA_PHY_RATE_SUPPORT
		rtmpeapupdaterateinfo(wmode, &wdev->rate, &wdev->eap);
#endif /* CONFIG_RA_PHY_RATE_SUPPORT */

		UpdateBeaconHandler(pAd, wdev, BCN_UPDATE_IE_CHG);
	}
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_STA(pAd) {
		UINT8 ba_en = 1;
		PSTA_ADMIN_CONFIG pStaCfg = &pAd->StaCfg[pObj->ioctl_if];
		SCAN_INFO *ScanInfo = NULL;
		BSS_TABLE *ScanTab = NULL;
		wdev = &pAd->StaCfg[pObj->ioctl_if].wdev;
		wdev->PhyMode = wmode;
		ScanInfo = &wdev->ScanInfo;
		ScanTab = get_scan_tab_by_wdev(pAd, wdev);
		success = RT_CfgSetWirelessMode(pAd, arg, wdev);

		if (!success)
			goto error;

		HcAcquireRadioForWdev(pAd, wdev);
		/* Change channel state to NONE */
		BandIdx = HcGetBandByWdev(wdev);
		pChCtrl = hc_get_channel_ctrl(pAd->hdev_ctrl, BandIdx);
		hc_set_ChCtrlChListStat(pChCtrl, CH_LIST_STATE_NONE);
		BuildChannelList(pAd, wdev);
		RTMPSetPhyMode(pAd, wdev, wmode);
		RTMPUpdateRateInfo(wmode, &wdev->rate);
		BssTableInit(ScanTab);
		ScanInfo->LastScanTime = 0;
#ifdef DOT11_N_SUPPORT
		ba_en = (WMODE_CAP_N(wmode)) ? 1 : 0;
		wlan_config_set_ba_enable(wdev, ba_en);
#endif /* DOT11_N_SUPPORT */

		/* Set AdhocMode rates*/
		if (pStaCfg->BssType == BSS_ADHOC) {
			MlmeUpdateTxRates(pAd, FALSE, 0);
			UpdateBeaconHandler(pAd, wdev, BCN_UPDATE_IF_STATE_CHG);
			AsicEnableIbssSync(
				pAd,
				pAd->CommonCfg.BeaconPeriod,
				HW_BSSID_0,
				OPMODE_ADHOC);
		}
	}
#endif /* CONFIG_STA_SUPPORT */
	return success;
error:
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("Set_WirelessMode_Proc::parameters out of range\n"));
	return success;
}

#ifdef CONFIG_AP_SUPPORT
#ifdef MBSS_SUPPORT
/*
    ==========================================================================
    Description:
	Set Wireless Mode for MBSS
    Return:
	TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_MBSS_WirelessMode_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	return Set_Cmm_WirelessMode_Proc(pAd, arg);
}
#endif /* MBSS_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */

/*
    ==========================================================================
    Description:
	Set Wireless Mode
    Return:
	TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT Set_WirelessMode_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	return Set_Cmm_WirelessMode_Proc(pAd, arg);
}

#ifdef RT_CFG80211_SUPPORT
INT Set_DisableCfg2040Scan_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	pAd->cfg80211_ctrl.FlgCfg8021Disable2040Scan = (UCHAR) os_str_tol(arg, 0, 10);
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("pAd->cfg80211_ctrl.FlgCfg8021Disable2040Scan  %d\n",
			 pAd->cfg80211_ctrl.FlgCfg8021Disable2040Scan));
	return TRUE;
}
#endif

/*
 *  ==========================================================================
 *  Description:
 *	Set Probe_Rsp's times
 *  Return:
 *	TRUE if all parameters are OK, FALSE otherwise
 *  ==========================================================================
*/

INT Set_Probe_Rsp_Times_Proc(
	RTMP_ADAPTER * pAd,
	RTMP_STRING *arg)
{

	struct _RTMP_CHIP_CAP *cap = hc_get_chip_cap(pAd->hdev_ctrl);
	UINT8 ProbeRspTimes = (UINT8) os_str_tol(arg, 0, 10);

	if ((ProbeRspTimes > 10) || (ProbeRspTimes < 1)) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
				("Set_PROBE_RSP_TIMES_Proc! INVALID, ProbeRspTimes(%d) should be <1~10>\n",
				 ProbeRspTimes));
		return FALSE;
	}

	cap->ProbeRspTimes = ProbeRspTimes;

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
				("Set_PROBE_RSP_TIMES_Proc! ProbeRspTimes = %d\n", cap->ProbeRspTimes));

	return TRUE;
}


/*
    ==========================================================================
    Description:
	Set phy channel for debugging/testing
    Return:
	TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT set_phy_channel_proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR if_idx = pObj->ioctl_if;
	struct wifi_dev *wdev = get_wdev_by_ioctl_idx_and_iftype(pAd, if_idx, pObj->ioctl_if_type);
	INT32 recv = 0;
	struct freq_oper freq;
	UCHAR band_idx = 0;
	/* ch parameters */
	UINT32 ch_band = 0;
	UINT32 ht_bw = 0;
	UINT32 vht_bw = 0;
	UINT32 bw = 0;
	UINT32 ext_cha = 0;
	UINT32 prim_ch = 0;
	UINT32 cen_ch_1 = 0;
	UINT32 cen_ch_2 = 0;
	UINT32 rx_stream = 0;
	UINT32 ap_bw = 0;
	UINT32 ap_cen_ch = 0;

	if (wdev == NULL) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s: wdev == NULL! if_type %d, if_idx = %d\n", __func__,
				 pObj->ioctl_if_type,
				 if_idx));
		return FALSE;
	}

	band_idx = HcGetBandByWdev(wdev);
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
				("%s(): band_idx = %d\n",
				__func__, band_idx));

	os_zero_mem(&freq, sizeof(freq));
	if (arg) {
		recv = sscanf(arg, "%d-%d-%d-%d-%d-%d-%d-%d-%d-%d-%d",
							&(ch_band), &(ht_bw), &(vht_bw),
							&(bw), &(ext_cha), &(prim_ch),
							&(cen_ch_1), &(cen_ch_2), &(rx_stream),
							&(ap_bw), &(ap_cen_ch));

		if (recv != 11) {
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
				("Format Error! Please enter in the following format\n"
					"ch_band-ht_bw-vht_bw-"
					"bw-ext_cha-prim_ch-"
					"cen_ch_1-cen_ch_2-rx_stream-"
					"ap_bw-ap_cen_ch\n"));
			return TRUE;
		}
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
					("%s():ch_band = %d\n ht_bw = %d\n vht_bw = %d\n"
					"bw = %d\n ext_cha = %d\n prim_ch = %d\n cen_ch_1 = %d\n"
					"cen_ch_2 = %d\n rx_stream = %d\n ap_bw = %d\n ap_cen_ch = %d\n",
					__func__, ch_band, ht_bw, vht_bw,
					bw, ext_cha, prim_ch, cen_ch_1,
					cen_ch_2, rx_stream, ap_bw, ap_cen_ch));

		freq.ch_band = ch_band;
		freq.ht_bw = ht_bw;
		freq.vht_bw = vht_bw;
		freq.bw = bw;
		freq.ext_cha = ext_cha;
		freq.prim_ch = prim_ch;
		freq.cen_ch_1 = cen_ch_1;
		freq.cen_ch_2 = cen_ch_2;
		freq.rx_stream = rx_stream;
		freq.ap_bw = ap_bw;
		freq.ap_cen_ch = ap_cen_ch;

		AsicSwitchChannel(pAd, band_idx, &freq, FALSE);
	}

	return TRUE;
}

/*
    ==========================================================================
    Description:
	Set Channel
    Return:
	TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT Set_Channel_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE	pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR if_idx = pObj->ioctl_if;
	struct wifi_dev *wdev = get_wdev_by_ioctl_idx_and_iftype(pAd, if_idx, pObj->ioctl_if_type);
	UCHAR Channel = (UCHAR) os_str_tol(arg, 0, 10);
	INT32 success = FALSE;
#ifdef TR181_SUPPORT
	UCHAR old_channel = wdev->channel;
	struct hdev_obj *hdev = (struct hdev_obj *)wdev->pHObj;
#endif /*TR181_SUPPORT*/
	INT ret = 0;

	if (wdev == NULL) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s: wdev == NULL! if_type %d, if_idx = %d\n", __func__,
				 pObj->ioctl_if_type,
				 if_idx));
		return FALSE;
	}

#if defined(MT_DFS_SUPPORT) && defined(BACKGROUND_SCAN_SUPPORT)
	DfsDedicatedExamineSetNewCh(pAd, wdev, Channel);
	DedicatedZeroWaitStop(pAd, TRUE);

#endif

#ifdef CONFIG_MAP_SUPPORT
	wdev->quick_ch_change = FALSE;
#endif
	pAd->ApCfg.iwpriv_event_flag = TRUE;
#ifdef TR181_SUPPORT
	success = rtmp_set_channel(pAd, wdev, Channel);

	if (success && (old_channel != Channel)) {
		hdev->rdev->pRadioCtrl->ManualChannelChangeCount++;
		hdev->rdev->pRadioCtrl->TotalChannelChangeCount++;
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s:success = %d	Manual:%d Total:%d\n",
						__func__, success, hdev->rdev->pRadioCtrl->ManualChannelChangeCount,
						hdev->rdev->pRadioCtrl->TotalChannelChangeCount));
	}
#else
#ifdef DFS_CAC_R2
	if (IS_MAP_ENABLE(pAd) || IS_MAP_TURNKEY_ENABLE(pAd)) {
		success = rtmp_set_channel(pAd, wdev, Channel);
		if (success == FALSE)
			wapp_send_cac_stop(pAd, RtmpOsGetNetIfIndex(wdev->if_dev), wdev->channel, FALSE);
		/*return success; after set channel finished,then return iwpriv.*/
	} else
#endif
	success = rtmp_set_channel(pAd, wdev, Channel);
#endif
	if (pAd->ApCfg.set_ch_async_flag == TRUE) {
		ret = RTMP_OS_WAIT_FOR_COMPLETION_TIMEOUT(&pAd->ApCfg.set_ch_aync_done, ((50*100*OS_HZ)/1000));/*Wait 5s.*/
		if (ret)
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s() wait channel setting success.\n", __func__));
		else {
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s() wait channel setting timeout.\n", __func__));
			pAd->ApCfg.set_ch_async_flag = FALSE;
		}
	}
	pAd->ApCfg.iwpriv_event_flag = FALSE;
	return success;
}

#ifdef CONVERTER_MODE_SWITCH_SUPPORT


/*
*    ==========================================================================
*    Description:
*	Enable/disable AP Beacons in Vendor10 Converter mode
*    Return:
*	TRUE if all parameters are OK, FALSE otherwise
*    ==========================================================================
*/
INT Set_V10ConverterMode_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UCHAR APBeaconEn  = (UCHAR) os_str_tol(arg, 0, 10);
	INT32 success = TRUE;	/*FALSE = 0*/
	UCHAR idx = 0;

	if (APBeaconEn) {
		for (idx = 0; idx < pAd->ApCfg.BssidNum; idx++) {
			BSS_STRUCT *pMbss = &pAd->ApCfg.MBSSID[idx];
			pMbss->APStartPseduState = AP_STATE_ALWAYS_START_AP_DEFAULT;
			if (WDEV_WITH_BCN_ABILITY(&pMbss->wdev)) {
					pMbss->wdev.bAllowBeaconing = TRUE;
					if (wdev_do_linkup(&pMbss->wdev, NULL) != TRUE)
							MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
							("%s: link up fail!!\n", __func__));
				}
#ifdef VOW_SUPPORT
				vow_mbss_init(pAd, &pMbss->wdev);
#endif
		}
	} else {
		for (idx = 0; idx < pAd->ApCfg.BssidNum; idx++) {
				BSS_STRUCT *pMbss = &pAd->ApCfg.MBSSID[idx];
				if (WDEV_WITH_BCN_ABILITY(&pMbss->wdev)) {
						pMbss->wdev.bAllowBeaconing = FALSE;
						if (wdev_do_linkdown(&pMbss->wdev) != TRUE)
							MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
									 ("%s: link down fail!!\n", __func__));
					}
				pMbss->APStartPseduState = AP_STATE_START_AFTER_APCLI_CONNECTION;
			}
	}
	return success;
}

#endif /*CONVERTER_MODE_SWITCH_SUPPORT*/

#if (defined(CONFIG_MAP_SUPPORT) || defined(CHANNEL_OPTIMIZATION))

/*
*	==========================================================================
*	Description:
*		This routine reset the entire MAC table. All packets pending in
*		the power-saving queues are freed here.
*	==========================================================================
*/
VOID MacTableResetNonMapWdev(RTMP_ADAPTER *pAd, struct wifi_dev *wdev)
{
	int i;
#ifdef CONFIG_AP_SUPPORT
	UCHAR *pOutBuffer = NULL;
	NDIS_STATUS NStatus;
	ULONG FrameLen = 0;
	HEADER_802_11 DeAuthHdr;
	USHORT Reason;
	struct _BSS_STRUCT *mbss;
#endif /* CONFIG_AP_SUPPORT */
	MAC_TABLE_ENTRY *pMacEntry;

	MTWF_LOG(DBG_CAT_MLME, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("MacTableResetWdev\n"));

	/* TODO:Carter, check why start from 1 */
	for (i = 1; VALID_UCAST_ENTRY_WCID(pAd, i); i++) {
		pMacEntry = &pAd->MacTab.Content[i];

		if (pMacEntry->wdev != wdev)
			continue;
#ifdef CONFIG_MAP_SUPPORT
	if ((IS_MAP_TURNKEY_ENABLE(pAd)) &&
		((pMacEntry->DevPeerRole & BIT(MAP_ROLE_BACKHAUL_STA)) &&
		(wdev->MAPCfg.DevOwnRole & BIT(MAP_ROLE_BACKHAUL_BSS))))
		continue;

#endif
		if (IS_ENTRY_CLIENT(pMacEntry)) {
#ifdef CONFIG_STA_SUPPORT
#ifdef ADHOC_WPA2PSK_SUPPORT
			IF_DEV_CONFIG_OPMODE_ON_STA(pAd) {
				BOOLEAN Cancelled;

				RTMPReleaseTimer(&pMacEntry->WPA_Authenticator.MsgRetryTimer, &Cancelled);
			}
#endif /* ADHOC_WPA2PSK_SUPPORT */
#endif /* CONFIG_STA_SUPPORT */
			pMacEntry->EnqueueEapolStartTimerRunning = EAPOL_START_DISABLE;
#ifdef CONFIG_AP_SUPPORT
			IF_DEV_CONFIG_OPMODE_ON_AP(pAd) {
				/* Before reset MacTable, send disassociation packet to client.*/
				if (pMacEntry->Sst == SST_ASSOC) {
					/*	send out a De-authentication request frame*/
					NStatus = MlmeAllocateMemory(pAd, &pOutBuffer);

					if (NStatus != NDIS_STATUS_SUCCESS) {
						MTWF_LOG(DBG_CAT_MLME, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
							(" MlmeAllocateMemory fail  ..\n"));
						/*NdisReleaseSpinLock(&pAd->MacTabLock);*/
						return;
					}

					Reason = REASON_NO_LONGER_VALID;
					MTWF_LOG(DBG_CAT_MLME, DBG_SUBCAT_ALL, DBG_LVL_WARN,
						("Send DeAuth (Reason=%d) to %02x:%02x:%02x:%02x:%02x:%02x\n",
							 Reason, PRINT_MAC(pMacEntry->Addr)));
					MgtMacHeaderInit(pAd, &DeAuthHdr, SUBTYPE_DEAUTH, 0, pMacEntry->Addr,
									 wdev->if_addr,
									 wdev->bssid);
					MakeOutgoingFrame(pOutBuffer, &FrameLen,
									  sizeof(HEADER_802_11), &DeAuthHdr,
									  2, &Reason,
									  END_OF_ARGS);
					MiniportMMRequest(pAd, 0, pOutBuffer, FrameLen);
					MlmeFreeMemory(pOutBuffer);
					RtmpusecDelay(5000);
				}
			}
#endif /* CONFIG_AP_SUPPORT */
		}

		/* Delete a entry via WCID */
		MacTableDeleteEntry(pAd, i, pMacEntry->Addr);
	}

#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd) {
		mbss = &pAd->ApCfg.MBSSID[wdev->func_idx];
#ifdef WSC_AP_SUPPORT
		{
			BOOLEAN Cancelled;

			RTMPCancelTimer(&mbss->wdev.WscControl.EapolTimer, &Cancelled);
			mbss->wdev.WscControl.EapolTimerRunning = FALSE;
			NdisZeroMemory(mbss->wdev.WscControl.EntryAddr, MAC_ADDR_LEN);
			mbss->wdev.WscControl.EapMsgRunning = FALSE;
		}
#endif /* WSC_AP_SUPPORT */
		mbss->StaCount = 0;
	}
#endif /* CONFIG_AP_SUPPORT */
}

void update_ch_by_wdev(RTMP_ADAPTER *pAd, struct wifi_dev *wdev)
{
	if (wdev->wdev_type == WDEV_TYPE_AP) {
		/* disconnect all STA in case of 2.4G, and non MAP sta in case of 5G*/
		if (wdev->channel > 14)
			MacTableResetNonMapWdev(pAd, wdev);
		else
			MacTableResetWdev(pAd, wdev);

		if (WMODE_CAP_5G(wdev->PhyMode)) {
#ifdef MT_DFS_SUPPORT /* Jelly20150217 */
			WrapDfsRadarDetectStop(pAd);
			/* Zero wait hand off recovery for CAC period + interface down case */
			DfsZeroHandOffRecovery(pAd, wdev);
#endif
		}

	}
	wlan_operate_init(wdev);
	/* wdev->quick_ch_change = FALSE; */
	UpdateBeaconHandler(pAd, wdev, BCN_UPDATE_IE_CHG);
}


void ap_phy_rrm_init_byRf(RTMP_ADAPTER *pAd, struct wifi_dev *wdev)
{
	UCHAR i = 0;
	struct wifi_dev *tdev;
	UCHAR band_idx = HcGetBandByWdev(wdev);
#ifdef CHANNEL_OPTIMIZATION
	if (pAd->QuickChSwEn[band_idx]) {
		for (i = 0; i < WDEV_NUM_MAX; i++) {

			tdev = pAd->wdev_list[i];
			if (tdev && HcIsRadioAcq(tdev) && (band_idx == HcGetBandByWdev(tdev))) {
				MTWF_LOG(DBG_CAT_MLME, DBG_SUBCAT_ALL, DBG_LVL_OFF,
					("Disconnect all STA Before Set (%s) Channel\n", (char *)tdev->if_dev->name));
				if (tdev->wdev_type == WDEV_TYPE_AP) {
				/* disconnect all STA */
					MacTableResetWdev(pAd, tdev);

					if (WMODE_CAP_5G(tdev->PhyMode)) {
#ifdef MT_DFS_SUPPORT /* Jelly20150217 */
						WrapDfsRadarDetectStop(pAd);
						/* Zero wait hand off recovery for CAC period + interface down case */
						DfsZeroHandOffRecovery(pAd, tdev);
#endif
					}
				}
			}
		}

		for (i = 0; i < WDEV_NUM_MAX; i++) {

			tdev = pAd->wdev_list[i];
			if (tdev && HcIsRadioAcq(tdev) && (band_idx == HcGetBandByWdev(tdev))) {
				MTWF_LOG(DBG_CAT_MLME, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
						("Wlan_operate_init and Update Beacon for : %s\n", (char *)tdev->if_dev->name));
					wlan_operate_init(tdev);
					/* wdev->quick_ch_change = FALSE; */
					UpdateBeaconHandler(pAd, tdev, BCN_UPDATE_IE_CHG);
				}
			}
		}else
#endif
	{
		for (i = 0; i < WDEV_NUM_MAX; i++) {

			tdev = pAd->wdev_list[i];
			if (tdev && HcIsRadioAcq(tdev) && (band_idx == HcGetBandByWdev(tdev))) {
				MTWF_LOG(DBG_CAT_MLME, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
				("Wlan_operate_init for : %s\n", (char *)tdev->if_dev->name));
			update_ch_by_wdev(pAd, tdev);
			} else if ((wdev->wdev_type == WDEV_TYPE_STA) &&
					(tdev != NULL) &&
					(tdev->wdev_type == WDEV_TYPE_AP) &&
					(tdev->if_up_down_state == 0) &&
					(((tdev->channel > 14) && (wdev->channel > 14)) ||
					((tdev->channel <= 14) && (wdev->channel <= 14)))) {
				MTWF_LOG(DBG_CAT_MLME, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
					("Wlan_operate_init--1 for : %s\n", (char *)tdev->if_dev->name));
				update_ch_by_wdev(pAd, tdev);
			}
		}
	}
	if (WMODE_CAP_2G(wdev->PhyMode)) {
		/*Do Obss scan*/
		UINT8 idx;
		BSS_STRUCT *pCurMbss = NULL;

		for (idx = 0; idx < pAd->ApCfg.BssidNum; idx++) {
			pCurMbss = &pAd->ApCfg.MBSSID[idx];

			/* check MBSS status is up */
			if (!pCurMbss->wdev.if_up_down_state)
				continue;

			if (wdev->channel <  14) {
				/* check MBSS work on the same RF(channel) */
				if (pCurMbss->wdev.channel == wdev->channel) {
					ap_over_lapping_scan(pAd, pCurMbss);
					break;
				}
			}
		}
	}
}
#endif
#ifdef CHANNEL_OPTIMIZATION
/*
*    ==========================================================================
*    Description:
*	Enable/disable quick Channel Switch feature
*    Return:
*	TRUE if all parameters are OK, FALSE otherwise
*    ==========================================================================
*/
INT Set_Quick_Channel_Switch_En_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UCHAR Bandidx = 0, QuickChannelEn = 0;
	INT32 success = TRUE;	/*FALSE = 0*/
	POS_COOKIE	pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR if_idx = pObj->ioctl_if;
	struct wifi_dev *wdev = get_wdev_by_ioctl_idx_and_iftype(pAd, if_idx, pObj->ioctl_if_type);

	if (!wdev)
			return FALSE;
	Bandidx = HcGetBandByWdev(wdev);
	QuickChannelEn = (UCHAR) os_str_tol(arg, 0, 10);

	pAd->QuickChSwEn[Bandidx] = QuickChannelEn ? TRUE : FALSE;
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
			("%s(): Bandidx(%d) Quick Channel Switch Enable = %d\n", __func__,Bandidx, pAd->QuickChSwEn[Bandidx]));
	return success;
}

#endif



#ifdef CONFIG_MAP_SUPPORT

/*
*    ==========================================================================
*    Description:
*	Enable/disable quick Channel change feature
*    Return:
*	TRUE if all parameters are OK, FALSE otherwise
*    ==========================================================================
*/
INT Set_Map_Channel_En_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UCHAR MapChannelEn = (UCHAR) os_str_tol(arg, 0, 10);
	INT32 success = TRUE;	/*FALSE = 0*/

	pAd->bMAPQuickChChangeEn = MapChannelEn;
	return success;
}


/*
*    ==========================================================================
*    Description:
*	Set Channel quickly without AP start/stop
*    Return:
*	TRUE if all parameters are OK, FALSE otherwise
*    ==========================================================================
*/
INT Set_Map_Channel_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE	pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR if_idx = pObj->ioctl_if;
	struct wifi_dev *wdev = get_wdev_by_ioctl_idx_and_iftype(pAd, if_idx, pObj->ioctl_if_type);
	INT32 success = FALSE;	/*FALSE = 0*/
#ifdef TR181_SUPPORT
	UCHAR old_channel = wdev->channel;
	struct hdev_obj *hdev = (struct hdev_obj *)wdev->pHObj;
#endif
	UCHAR Channel;
#ifdef MAP_R2
	UCHAR cac_req;
	UCHAR dev_role;
	UINT32 i;
	RTMP_STRING *token;
#endif
#ifdef MAP_R2
	token = rstrtok(arg, ":");
	if (token)
		Channel = os_str_tol(token, 0, 10);
	token = rstrtok(NULL, ":");
	if (token) {
		cac_req = os_str_tol(token, 0, 10);
#ifdef MT_DFS_SUPPORT
		if (cac_req == 0 && pAd->CommonCfg.DfsParameter.bDfsEnable) {
			for (i = 0; i < MAX_BEACON_NUM; i++) {
				if (pAd->ApCfg.MBSSID[i].wdev.channel == wdev->channel)
					pAd->ApCfg.MBSSID[i].wdev.cac_not_required = TRUE;
			}
		}
#endif
	}
	token = rstrtok(NULL, ":");
	if (token) {
		dev_role = os_str_tol(token, 0, 10);
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			("%s:dev_role %d\n", __func__, dev_role));
		for (i = 0; i < MAX_BEACON_NUM; i++) {
			if (pAd->ApCfg.MBSSID[i].wdev.channel == wdev->channel)
				pAd->ApCfg.MBSSID[i].wdev.dev_role = dev_role;
		}
	}
#else
	Channel = (UCHAR) os_str_tol(arg, 0, 10);
#endif

	if (pAd->bMAPQuickChChangeEn == FALSE) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("MAPQuickChChange feaure not enabled!!"));
		return Set_Channel_Proc(pAd, arg);
	}

	if (wdev == NULL) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			("%s: wdev == NULL! if_type %d, if_idx = %d\n", __func__,
				 pObj->ioctl_if_type,
				 if_idx));
		return FALSE;
	}

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s\n", __func__));

#if defined(MT_DFS_SUPPORT) && defined(BACKGROUND_SCAN_SUPPORT)
	DfsDedicatedExamineSetNewCh(pAd, wdev, Channel);
	DedicatedZeroWaitStop(pAd, TRUE);
#endif
	wdev->quick_ch_change = TRUE;

#ifdef TR181_SUPPORT
	success = rtmp_set_channel(pAd, wdev, Channel);

	if (success && (old_channel != Channel)) {
		hdev->rdev->pRadioCtrl->ManualChannelChangeCount++;
		hdev->rdev->pRadioCtrl->TotalChannelChangeCount++;
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s:success = %d	Manual:%d Total:%d\n",
						__func__, success, hdev->rdev->pRadioCtrl->ManualChannelChangeCount,
						hdev->rdev->pRadioCtrl->TotalChannelChangeCount));
	}
	return success;
#else
	success = rtmp_set_channel(pAd, wdev, Channel);
	if (wdev->wdev_type == WDEV_TYPE_STA)
		wdev->quick_ch_change = FALSE;

	return success;
#endif
}

#ifdef MAP_TS_TRAFFIC_SUPPORT
INT Set_MapTS_Proc(
	PRTMP_ADAPTER pAd,
	char *arg)
{
	UCHAR enable = os_str_tol(arg, 0, 10);

	if (pAd->bTSEnable == enable) {
		/* No need to do anything, current and previos values are same */
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF,
			("%s MAP TS is already %s\n", __func__, enable?"enabled":"disabled"));
		return TRUE;
	}

	if (!enable)
		pAd->bTSEnable = FALSE;
	else
		pAd->bTSEnable = TRUE;

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF,
			("%s: MAP TS is %s\n", __func__, pAd->bTSEnable?"enabled":"disabled"));

	return TRUE;
}
#endif

#ifdef MAP_R2
INT Set_Map_Bh_Primary_Vid_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UINT16 vid = (UINT16) os_str_tol(arg, 0, 10);

	POS_COOKIE	pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR if_idx = pObj->ioctl_if;
	struct wifi_dev *wdev = get_wdev_by_ioctl_idx_and_iftype(pAd, if_idx, pObj->ioctl_if_type);

	if (wdev == NULL) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			("%s: wdev == NULL! if_type %d, if_idx = %d\n", __func__,
				 pObj->ioctl_if_type,
				 if_idx));
		return FALSE;
	}

	wdev->MAPCfg.primary_vid = vid;
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			("%s: %s default vid=%d\n", __func__, wdev->if_dev->name, vid));

	return TRUE;
}

INT Set_Map_Bh_Primary_Pcp_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UCHAR pcp = (UCHAR) os_str_tol(arg, 0, 10);

	POS_COOKIE	pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR if_idx = pObj->ioctl_if;
	struct wifi_dev *wdev = get_wdev_by_ioctl_idx_and_iftype(pAd, if_idx, pObj->ioctl_if_type);

	if (wdev == NULL) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			("%s: wdev == NULL! if_type %d, if_idx = %d\n", __func__,
				 pObj->ioctl_if_type,
				 if_idx));
		return FALSE;
	}

	wdev->MAPCfg.primary_pcp = pcp;
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			("%s: %s default pcp=%d\n", __func__, wdev->if_dev->name, pcp));

	return TRUE;
}

INT Set_Map_Bh_Vid_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UINT16 vid = (UINT16) os_str_tol(arg, 0, 10);
	UINT32 index = 0, offset = 0;
	RTMP_STRING *p = NULL;

	POS_COOKIE	pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR if_idx = pObj->ioctl_if;
	struct wifi_dev *wdev = get_wdev_by_ioctl_idx_and_iftype(pAd, if_idx, pObj->ioctl_if_type);

	if (wdev == NULL) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			("%s: wdev == NULL! if_type %d, if_idx = %d\n", __func__,
				 pObj->ioctl_if_type,
				 if_idx));
		return FALSE;
	}

	RTMPZeroMemory(wdev->MAPCfg.vids, sizeof(wdev->MAPCfg.vids));
	wdev->MAPCfg.vid_num = 0;

	while (1) {
		p = strsep(&arg, ",");
		if (!p || *p == '\0')
			break;
		vid = (UINT16) os_str_tol(p, 0, 10);
		if (vid >= INVALID_VLAN_ID || vid == 0) {
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
					("%s: %s invalid vid=%d\n", __func__, wdev->if_dev->name, vid));
			continue;
		}

		index = vid / (sizeof(UINT32) * 8);
		offset = vid % (sizeof(UINT32) * 8);

		if (!(wdev->MAPCfg.vids[index] & BIT(offset))) {
			wdev->MAPCfg.vids[index] |= BIT(offset);
			wdev->MAPCfg.vid_num++;
		}
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
				("%s: %s bh vid vlan id=%d\n", __func__, wdev->if_dev->name, vid));
	}

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			("%s: %s total vid_num=%d\n", __func__, wdev->if_dev->name, wdev->MAPCfg.vid_num));

	return TRUE;
}

INT Set_Map_Fh_Vid_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UINT16 vid = (UINT16) os_str_tol(arg, 0, 10);

	POS_COOKIE	pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR if_idx = pObj->ioctl_if;
	struct wifi_dev *wdev = get_wdev_by_ioctl_idx_and_iftype(pAd, if_idx, pObj->ioctl_if_type);

	if (wdev == NULL) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			("%s: wdev == NULL! if_type %d, if_idx = %d\n", __func__,
				 pObj->ioctl_if_type,
				 if_idx));
		return FALSE;
	}

	wdev->MAPCfg.fh_vid = vid;
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			("%s: %s fh vid=%d\n", __func__, wdev->if_dev->name, vid));

	return TRUE;
}

INT Set_Map_Transparent_Vid_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UINT16 vid = 0;
	UINT32 index = 0, offset = 0;
	POS_COOKIE	pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR if_idx = pObj->ioctl_if;
	RTMP_STRING *p = NULL;
	struct wifi_dev *wdev = get_wdev_by_ioctl_idx_and_iftype(pAd, if_idx, pObj->ioctl_if_type);

	if (wdev == NULL) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			("%s: wdev == NULL! if_type %d, if_idx = %d\n", __func__,
				 pObj->ioctl_if_type,
				 if_idx));
		return FALSE;
	}

	RTMPZeroMemory(wdev->MAPCfg.bitmap_trans_vlan, sizeof(wdev->MAPCfg.bitmap_trans_vlan));

	while (1) {
		p = strsep(&arg, ",");
		if (!p || *p == '\0')
			break;
		vid = (UINT16) os_str_tol(p, 0, 10);
		if (vid >= INVALID_VLAN_ID || vid == 0) {
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
					("%s: %s invalid vid=%d\n", __func__, wdev->if_dev->name, vid));
			continue;
		}

		index = vid / (sizeof(UINT32) * 8);
		offset = vid % (sizeof(UINT32) * 8);

		wdev->MAPCfg.bitmap_trans_vlan[index] |= BIT(offset);
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
				("%s: %s transparent vlan id=%d\n", __func__, wdev->if_dev->name, vid));
	}

	return TRUE;
}
#endif
#endif


BOOLEAN mt_validate_dfs_channel_for_cac(RTMP_ADAPTER *pAdapter, struct wifi_dev *wdev)
{
	BSS_ENTRY *bss;
	UCHAR i = 0;
	PBSS_TABLE ScanTab = NULL;
	if ((wdev->wdev_type != WDEV_TYPE_STA) &&
	(wdev->wdev_type != WDEV_TYPE_REPEATER))
		return TRUE;
	ScanTab = get_scan_tab_by_wdev(pAdapter, wdev);
	if (ScanTab->BssNr == 0)
		return TRUE;
	for (i = 0; i < ScanTab->BssNr; i++) {
		bss = &ScanTab->BssEntry[i];
		if (bss->Channel == wdev->channel)
			return FALSE;
	}
	return TRUE;
}

INT	rtmp_set_channel(RTMP_ADAPTER *pAd, struct wifi_dev *wdev, UCHAR Channel)
{
	INT32 Success = TRUE;
	UCHAR OriChannel;
	UCHAR BandIdx = BAND0;
#ifdef OFFCHANNEL_SCAN_FEATURE
	OFFCHANNEL_SCAN_MSG Rsp;
	UCHAR RfIC = 0;
#endif
	USHORT PhyMode;
#ifdef CONFIG_STA_SUPPORT
	PSTA_ADMIN_CONFIG pStaCfg = GetStaCfgByWdev(pAd, wdev);
#endif
#ifdef CONFIG_AP_SUPPORT
	BSS_STRUCT *pMbss = NULL;
	PNET_DEV ndev_ap_if = NULL;
	UCHAR i = 0;
#endif
	struct DOT11_H *pDot11h = NULL;

#if (defined(CONFIG_RCSA_SUPPORT) || defined(MT_DFS_SUPPORT))
	CHANNEL_CTRL *pChCtrl = NULL;
	PDFS_PARAM pDfsParam = &pAd->CommonCfg.DfsParameter;
#endif

	pAd->ApCfg.set_ch_async_flag = FALSE;
	if (wdev == NULL) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s: wdev == NULL!\n", __func__));
		return FALSE;
	}

	if (IsHcRadioCurStatOffByWdev(wdev))
		return Success;

	BandIdx = HcGetBandByWdev(wdev);

	pDot11h = wdev->pDot11_H;
	PhyMode = wdev->PhyMode;
	OriChannel = wdev->channel;
	/*165 channel can only work at 20M*/
	if ((Channel == 165) && WMODE_CAP_5G(wdev->PhyMode)) {
		if (wlan_operate_get_ht_bw(wdev) || wlan_operate_get_vht_bw(wdev)) {
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s():Warning: 165 Channel must have to call back to HtBw/VhtBw 20M! \n", __func__));
		}
	}

	/* check if this channel is valid*/
	if (SwitchChSanityCheckByWdev(pAd, wdev, OriChannel, Channel) ||
		(SwitchChSanityCheck(pAd, OriChannel, Channel) == TRUE)) {
#ifdef CONFIG_STA_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_STA(pAd) {
			/* Save the channel on MlmeAux for CntlOidRTBssidProc used. */
			pStaCfg->MlmeAux.Channel = Channel;
			/*apply channel directly*/
			wlan_operate_set_prim_ch(wdev, Channel);
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s(): CtrlChannel(%d), CentralChannel(%d)\n",
					 __func__, Channel,
					 wlan_operate_get_cen_ch_1(wdev)));
		}
#endif /* CONFIG_STA_SUPPORT */
		Success = TRUE;
	} else {
#ifdef CONFIG_AP_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_AP(pAd) {
			Channel = FirstChannel(pAd, wdev);
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_WARN,
					 ("This channel is out of channel list, set as the first channel(%d)\n ", Channel));
		}
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
		Success = FALSE;
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_WARN, ("This channel is out of channel list, nothing to do!\n "));
#endif /* CONFIG_STA_SUPPORT */
	}
	/*used for not support MCC*/
	wdev->channel = Channel;
	/*sync to other device*/

#ifdef CONFIG_MAP_SUPPORT
	if (IS_MAP_TURNKEY_ENABLE(pAd)) {
		if (RadarChannelCheck(pAd, OriChannel)) {
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
				("[%s] send CAC stop OriChannel = %d, wdev->channel = %d \n",
					__func__, OriChannel, wdev->channel));
			wapp_send_cac_stop(pAd, RtmpOsGetNetIfIndex(wdev->if_dev), OriChannel, TRUE);
		}
	 }
#endif
	wdev_sync_prim_ch(wdev->sys_handle, wdev);

#ifdef CONFIG_AP_SUPPORT
		if (pAd->CommonCfg.bIEEE80211H == TRUE) {
			if (CheckNonOccupancyChannel(pAd, wdev, RDD_CHECK_NOP_BY_WDEV) == FALSE) {
				MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR, (
					"%s: Can not update channel(%d), restoring old channel(%d)\n",
						__func__, wdev->channel, OriChannel));
#ifdef CONFIG_MAP_SUPPORT
				if (IS_MAP_TURNKEY_ENABLE(pAd))
					wapp_send_radar_detect_notif(pAd, wdev, wdev->channel, 0);
#endif
				wdev->channel = OriChannel;
				wdev_sync_prim_ch(wdev->sys_handle, wdev);
#if (DFS_ZEROWAIT_DEFAULT_FLOW == 1)
#ifdef MT_DFS_SUPPORT
				/* if radar is detected during in-band ch switch, dev resets inband channel if new dfs ch is invalid */
				if (pAd->CommonCfg.DfsParameter.bDedicatedZeroWaitDefault == 0)
					return FALSE;
#endif
#else
				return FALSE;
#endif
			}
		}
#endif

#ifdef MT_DFS_SUPPORT
		pChCtrl = hc_get_channel_ctrl(pAd->hdev_ctrl, BandIdx);

		if ((pAd->CommonCfg.dbdc_mode == TRUE) && (RadarChannelCheck(pAd, wdev->channel)) && (pChCtrl->ChListNum == 0)) {
			pDfsParam->NeedSetNewChList[BandIdx] = DFS_SET_NEWCH_ENABLED;
			DfsBuildChannelList(pAd, wdev);
		}
#endif

	if (pDot11h == NULL)
		return FALSE;
#ifdef CONFIG_AP_SUPPORT
	if (wdev->wdev_type == WDEV_TYPE_AP) {
		pMbss = &pAd->ApCfg.MBSSID[wdev->func_idx];
	} else if (IF_COMBO_HAVE_AP_STA(pAd) && (wdev->wdev_type == WDEV_TYPE_STA)) {
		/* for APCLI, find first BSS with same channel */
		for (i = 0; i < pAd->ApCfg.BssidNum; i++) {
			if ((pAd->ApCfg.MBSSID[i].wdev.channel == wdev->channel) &&
					(pAd->ApCfg.MBSSID[i].wdev.if_up_down_state != 0)) {
				pMbss = &pAd->ApCfg.MBSSID[i];
				break;
			}
		}
	}
	if (pMbss != NULL)
		ndev_ap_if = pMbss->wdev.if_dev;

	IF_DEV_CONFIG_OPMODE_ON_AP(pAd) {
#ifdef APCLI_AUTO_CONNECT_SUPPORT

		if (pAd->ApCfg.ApCliAutoConnectChannelSwitching == FALSE)
			pAd->ApCfg.ApCliAutoConnectChannelSwitching = TRUE;

#endif /* APCLI_AUTO_CONNECT_SUPPORT */
	}
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd) {
		if (Success == TRUE) {
			if ((Channel > 14)
				&& (pAd->CommonCfg.bIEEE80211H == TRUE))
				pDot11h->org_ch = OriChannel;

			if ((Channel > 14)
				&& (pAd->CommonCfg.bIEEE80211H == TRUE)
			   ) {

					if (pMbss == NULL) {
						 /*AP Interface is not present and CLI wants to change channel*/
						MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
							("Only Change CLI Channel to %d!\n", wdev->channel));
						wlan_operate_set_prim_ch(wdev, wdev->channel);
#ifdef APCLI_AUTO_CONNECT_SUPPORT
						pAd->ApCfg.ApCliAutoConnectChannelSwitching = FALSE;
#endif
						return Success;
					}
#ifdef CONFIG_MAP_SUPPORT
#ifdef MT_DFS_SUPPORT
				if (IS_MAP_TURNKEY_ENABLE(pAd) && !(mt_validate_dfs_channel_for_cac(pAd, wdev)) &&
						pAd->CommonCfg.DfsParameter.bDfsEnable) {
					for (i = 0; i < MAX_BEACON_NUM; i++) {
						if (pAd->ApCfg.MBSSID[i].wdev.channel == wdev->channel)
							pAd->ApCfg.MBSSID[i].wdev.cac_not_required = TRUE;
					}
				}
#endif /* MT_DFS_SUPPORT */
#endif

				if ((pDot11h->RDMode == RD_SILENCE_MODE)
					|| ((ndev_ap_if != NULL) && (!RTMP_OS_NETDEV_STATE_RUNNING(ndev_ap_if)))) {
					pDot11h->RDMode = RD_SWITCHING_MODE;
#ifdef CONFIG_MAP_SUPPORT
					if (/*IS_MAP_ENABLE(pAd) &&*/ wdev->quick_ch_change == FALSE) {
#endif
					if (pMbss != NULL)
						APStop(pAd, pMbss, AP_BSS_OPER_BY_RF);
#ifdef MT_DFS_SUPPORT
						if (DfsStopWifiCheck(pAd, wdev)) {
							MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("[%s] Stop AP Startup\n", __func__));
							return FALSE;
						}
#endif
#ifdef OFFCHANNEL_SCAN_FEATURE
					if (pMbss != NULL) {
						RfIC = (WMODE_CAP_5G(wdev->PhyMode)) ? RFIC_5GHZ : RFIC_24GHZ;
						MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s 2.4G channel switch RFIC = %d\n", __func__, RfIC));
						memcpy(Rsp.ifrn_name, pAd->ScanCtrl[BandIdx].if_name, IFNAMSIZ);
						Rsp.Action = DRIVER_CHANNEL_SWITCH_SUCCESSFUL;
						Rsp.data.operating_ch_info.channel = HcGetChannelByRf(pAd, RfIC);
						MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s 2.4G channel to switch = %d\n", __func__, Rsp.data.operating_ch_info.channel));
						Rsp.data.operating_ch_info.cfg_ht_bw = wlan_config_get_ht_bw(&pAd->ApCfg.MBSSID[MAIN_MBSSID].wdev);
						Rsp.data.operating_ch_info.cfg_vht_bw = wlan_config_get_vht_bw(&pAd->ApCfg.MBSSID[MAIN_MBSSID].wdev);
						Rsp.data.operating_ch_info.RDDurRegion = pAd->CommonCfg.RDDurRegion;
						Rsp.data.operating_ch_info.region = GetCountryRegionFromCountryCode(pAd->CommonCfg.CountryCode);
						if ((pAd->Antenna.field.TxPath == 4) && (pAd->Antenna.field.RxPath == 4))
							Rsp.data.operating_ch_info.is4x4Mode = 1;
						else
							Rsp.data.operating_ch_info.is4x4Mode = 0;
						RtmpOSWrielessEventSend(
							pAd->net_dev,
							RT_WLAN_EVENT_CUSTOM,
							OID_OFFCHANNEL_INFO,
							NULL,
							(UCHAR *) &Rsp,
							sizeof(OFFCHANNEL_SCAN_MSG));
					}
#endif

					if (pMbss != NULL)
						APStartUp(pAd, pMbss, AP_BSS_OPER_BY_RF);

#ifdef CONFIG_MAP_SUPPORT
					if (IS_MAP_TURNKEY_ENABLE(pAd)) {
						if (pMbss && pMbss->wdev.cac_not_required) {
							for (i = 0; i < MAX_BEACON_NUM; i++) {
								if (pAd->ApCfg.MBSSID[i].wdev.channel == wdev->channel)
									pAd->ApCfg.MBSSID[i].wdev.cac_not_required = FALSE;
							}
							pDot11h->RDCount = pDot11h->cac_time;
						}
					}
#endif
#ifdef CONFIG_MAP_SUPPORT
					} else {
						ap_phy_rrm_init_byRf(pAd, wdev);
					}
#endif
#ifdef CONVERTER_MODE_SWITCH_SUPPORT
								/* When this Procuct is in CNV mode or */
								/* REP mode and if ApCli is not connected */
								/* then we do not send CSA, and so we should imitate the event functionality here */
					} else if ((pMbss->APStartPseduState != AP_STATE_ALWAYS_START_AP_DEFAULT)
					&& (HcIsRfSupport(pAd, RFIC_5GHZ))) {
						UINT i = 0;
						for (i = 0; i < WDEV_NUM_MAX; i++) {
							if (pAd->wdev_list[i] && (pAd->wdev_list[i]->wdev_type == WDEV_TYPE_AP)) {
									MTWF_LOG(DBG_CAT_PROTO, CATPROTO_DFS, DBG_LVL_TRACE, ("%s:	Type = %d, func_idx = %d\n",
									 __func__, pAd->wdev_list[i]->wdev_type, pAd->wdev_list[i]->func_idx));
									RTEnqueueInternalCmd(pAd, CMDTHRED_DOT11H_SWITCH_CHANNEL, &pAd->wdev_list[i]->func_idx, sizeof(UCHAR));
									break;
							}
						}
#endif /* CONVERTER_MODE_SWITCH_SUPPORT*/

				} else if (!WDEV_WITH_BCN_ABILITY(wdev)
#ifdef CONFIG_MAP_SUPPORT
				 && wdev->quick_ch_change == FALSE
#endif
				) {
					wlan_operate_set_prim_ch(wdev, wdev->channel);
					if (pMbss != NULL) {
						APStop(pAd, pMbss, AP_BSS_OPER_BY_RF);
						APStartUp(pAd, pMbss, AP_BSS_OPER_BY_RF);
					}
				} else {

					NotifyChSwAnnToPeerAPs(pAd, ZERO_MAC_ADDR, pAd->CurrentAddress, 1, Channel);
					pDot11h->RDMode = RD_SWITCHING_MODE;
					pDot11h->CSCount = 0;
					pDot11h->new_channel = Channel;
#ifdef CONFIG_RCSA_SUPPORT
					if (pDfsParam->bRCSAEn && pDfsParam->fSendRCSA) {
						MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
							("%s:Sending RCSA NewChannel:%d\n", __func__, Channel));
						notify_channel_switch_to_backhaulAP(pAd, wdev, Channel, pDfsParam->ChSwMode);
						pDfsParam->fSendRCSA = FALSE;
					}
#endif
					if (HcUpdateCsaCntByChannel(pAd, OriChannel) != 0) {
#ifdef CONFIG_MAP_SUPPORT
						if (IS_MAP_TURNKEY_ENABLE(pAd))
							wdev->map_indicate_channel_change = 1;
#endif
						return Success;
					}
				}
			} else {
					wlan_operate_set_prim_ch(wdev, wdev->channel);
#ifdef CHANNEL_OPTIMIZATION
					if (pAd->QuickChSwEn[BandIdx]) {
						MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s BandIdx (%d) Quickly Set Channel = (%d)\n", __func__, BandIdx, wdev->channel));
						ap_phy_rrm_init_byRf(pAd, wdev);
					} else
#endif
					if (pMbss != NULL) {
						APStop(pAd, pMbss, AP_BSS_OPER_BY_RF);
						APStartUp(pAd, pMbss, AP_BSS_OPER_BY_RF);
					}
			}
		}
	}
#endif /* CONFIG_AP_SUPPORT */

	if (Success == TRUE) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Set_Channel_Proc_by_Wdev::(Channel=%d)\n", Channel));
#ifdef CONFIG_MAP_SUPPORT
		if (IS_MAP_TURNKEY_ENABLE(pAd)) {
			wdev->map_indicate_channel_change = 1;
		}
#endif
	}

#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd) {
#ifdef APCLI_AUTO_CONNECT_SUPPORT
		pAd->ApCfg.ApCliAutoConnectChannelSwitching = FALSE;
#endif /* APCLI_AUTO_CONNECT_SUPPORT */
	}
#endif /* CONFIG_AP_SUPPORT */
	return Success;
}

#if defined(OFFCHANNEL_SCAN_FEATURE) || defined(TXRX_STAT_SUPPORT)
VOID ReadChannelStats(
	IN PRTMP_ADAPTER   pAd)
{
}
#endif
/*
    ==========================================================================
    Description:
	Set Short Slot Time Enable or Disable
    Return:
	TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_ShortSlot_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	int retval;

	retval = RT_CfgSetShortSlot(pAd, arg);

	if (retval == TRUE)
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Set_ShortSlot_Proc::(ShortSlot=%d)\n",
				 pAd->CommonCfg.bUseShortSlotTime));

	return retval;
}

/*
    ==========================================================================
    Description:
	Set Tx power
    Return:
	TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT Set_TxPower_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	LONG    TxPower;
	INT     status = FALSE;
	UINT8   BandIdx = 0;
	struct  wifi_dev *wdev;
#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd) {
		POS_COOKIE  pObj = (POS_COOKIE) pAd->OS_Cookie;
		UCHAR       apidx = pObj->ioctl_if;

		/* obtain Band index */
		if (apidx >= pAd->ApCfg.BssidNum)
			return FALSE;

		wdev = &pAd->ApCfg.MBSSID[apidx].wdev;
		BandIdx = HcGetBandByWdev(wdev);
	}
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_STA(pAd) {
		wdev = &pAd->StaCfg[0].wdev;
		BandIdx = HcGetBandByWdev(wdev);
	}
#endif /* CONFIG_STA_SUPPORT */
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s: BandIdx = %d\n", __func__, BandIdx));

	/* sanity check for Band index */
	if (BandIdx >= DBDC_BAND_NUM)
		return FALSE;

	TxPower = simple_strtol(arg, 0, 10);

	if (TxPower <= 100) {
#ifdef CONFIG_AP_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_AP(pAd) {
			pAd->CommonCfg.ucTxPowerPercentage[BandIdx] = TxPower;
		}
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_STA(pAd) {
			pAd->CommonCfg.ucTxPowerDefault[BandIdx] = TxPower;
			pAd->CommonCfg.ucTxPowerPercentage[BandIdx] = pAd->CommonCfg.ucTxPowerDefault[BandIdx];
		}
#endif /* CONFIG_STA_SUPPORT */
		status = TRUE;
	} else
		status = FALSE;

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Set_TxPower_Proc: BandIdx: %d, (TxPowerPercentage=%d)\n",
			 BandIdx, pAd->CommonCfg.ucTxPowerPercentage[BandIdx]));
	return status;
}

/*
    ==========================================================================
    Description:
	Set 11B/11G Protection
    Return:
	TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_BGProtection_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE	pObj = (POS_COOKIE) pAd->OS_Cookie;
	struct wifi_dev *wdev = get_wdev_by_ioctl_idx_and_iftype(pAd, pObj->ioctl_if, pObj->ioctl_if_type);

	if (!wdev)
		return FALSE;

	switch (os_str_tol(arg, 0, 10)) {
	case 0: /*AUTO*/
		pAd->CommonCfg.UseBGProtection = 0;
		break;

	case 1: /*Always On*/
		pAd->CommonCfg.UseBGProtection = 1;
		break;

	case 2: /*Always OFF*/
		pAd->CommonCfg.UseBGProtection = 2;
		break;

	default:  /*Invalid argument */
		return FALSE;
	}

#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd) {
		ApUpdateCapabilityAndErpIe(pAd, &pAd->ApCfg.MBSSID[pObj->ioctl_if]);
	}
#endif /* CONFIG_AP_SUPPORT */
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Set_BGProtection_Proc::(BGProtection=%ld)\n",
			 pAd->CommonCfg.UseBGProtection));
	return TRUE;
}

/*
 *  ==========================================================================
 *  Description:
 *	Set Set_McastBcastMcs_CCK
 *  Return:
 *	TRUE if all parameters are OK, FALSE otherwise
 *  ==========================================================================
*/
#ifdef MCAST_VENDOR10_CUSTOM_FEATURE
INT Set_McastBcastMcs_CCK(
	IN PRTMP_ADAPTER pAd,
	IN UCHAR Mcs)
{
	BSS_INFO_ARGUMENT_T bss_info_argument;
	BOOLEAN isband5g, tmp_band;
	HTTRANSMIT_SETTING *pTransmit;
	struct wifi_dev *wdev = NULL;
	INT i = 0;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR apidx = pObj->ioctl_if;

	if ((pObj->ioctl_if_type != INT_MBSSID) && (pObj->ioctl_if_type != INT_MAIN)) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
				 ("Do nothing! This device interface is NOT AP mode!\n"));
		return FALSE;
	}

	if (apidx >= pAd->ApCfg.BssidNum) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
				 ("Invalid device interface!\n"));
		return FALSE;
	}

	if (Mcs > 15) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
				 ("Mcs must be in range of 0 to 15\n"));
		return FALSE;
	}

	wdev = &pAd->ApCfg.MBSSID[apidx].wdev;
	isband5g = (wdev->channel > 14) ? TRUE : FALSE;
	pTransmit = (isband5g) ? (&pAd->CommonCfg.MCastPhyMode_5G) : (&pAd->CommonCfg.MCastPhyMode);

	if ((Mcs <= 3) || (Mcs >= 8 && Mcs <= 11)) {
		pTransmit->field.MCS = Mcs;
	} else {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
				 ("MCS must in range of 0 ~ 3 and 8 ~ 11 for CCK Mode.\n"));
		return FALSE;
	}

	for (i = 0; i < pAd->ApCfg.BssidNum; i++) {
		wdev = &pAd->ApCfg.MBSSID[i].wdev;
		tmp_band = (wdev->channel > 14) ? TRUE : FALSE;

		if (tmp_band != isband5g)
			continue;

		NdisZeroMemory(&bss_info_argument, sizeof(BSS_INFO_ARGUMENT_T));
		bss_info_argument.bss_state = BSS_ACTIVE;
		bss_info_argument.ucBssIndex = wdev->bss_info_argument.ucBssIndex;
		bss_info_argument.u4BssInfoFeature = BSS_INFO_BROADCAST_INFO_FEATURE;

		memmove(&bss_info_argument.BcTransmit, pTransmit, sizeof(HTTRANSMIT_SETTING));
		memmove(&bss_info_argument.McTransmit, pTransmit, sizeof(HTTRANSMIT_SETTING));

		if (AsicBssInfoUpdate(pAd, &bss_info_argument) != NDIS_STATUS_SUCCESS)
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
					 ("Fail to apply the bssinfo, BSSID=%d!\n", i));
	}

	return TRUE;
}
#endif

/*
    ==========================================================================
    Description:
	Set TxPreamble
    Return:
	TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_TxPreamble_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	RT_802_11_PREAMBLE	Preamble;
#ifdef MCAST_VENDOR10_CUSTOM_FEATURE
	RT_802_11_PREAMBLE	OldPreamble = OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_SHORT_PREAMBLE_INUSED);
	HTTRANSMIT_SETTING *pTransmit = NULL;
	POS_COOKIE	pObj = (POS_COOKIE) pAd->OS_Cookie;
	struct wifi_dev *wdev = get_wdev_by_ioctl_idx_and_iftype(pAd, pObj->ioctl_if, pObj->ioctl_if_type);
	BOOLEAN isband5g = FALSE;
	UCHAR MCS = 0;
#endif

	Preamble = (RT_802_11_PREAMBLE)os_str_tol(arg, 0, 10);
#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)

	if (Preamble == Rt802_11PreambleAuto)
		return FALSE;

#endif /* CONFIG_AP_SUPPORT */

	switch (Preamble) {
	case Rt802_11PreambleShort:
		pAd->CommonCfg.TxPreamble = Preamble;
#ifdef CONFIG_STA_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
		MlmeSetTxPreamble(pAd, Rt802_11PreambleShort);
#endif /* CONFIG_STA_SUPPORT */
		break;

	case Rt802_11PreambleLong:
#ifdef CONFIG_STA_SUPPORT
	case Rt802_11PreambleAuto:
		/*
			If user wants AUTO, initialize to LONG here, then change according to AP's
			capability upon association
		*/
#endif /* CONFIG_STA_SUPPORT */
		pAd->CommonCfg.TxPreamble = Preamble;
#ifdef CONFIG_STA_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
		MlmeSetTxPreamble(pAd, Rt802_11PreambleLong);
#endif /* CONFIG_STA_SUPPORT */
		break;

	default: /*Invalid argument */
		return FALSE;
	}

#ifdef MCAST_VENDOR10_CUSTOM_FEATURE
	/* MCS Switch based on new Preamble */
	if (wdev) {
		isband5g = (wdev->channel > 14) ? TRUE : FALSE;
		pTransmit = (isband5g) ? (&pAd->CommonCfg.MCastPhyMode_5G) : (&pAd->CommonCfg.MCastPhyMode);

		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("[%s] Old MCS %d Old Preamble %d New Preamble %d\n",
			__func__, MCS, OldPreamble, Preamble));

		if (pTransmit && (pTransmit->field.MODE == MODE_CCK)) {
			MCS = pTransmit->field.MCS;

		if ((MCS) && (Preamble == Rt802_11PreambleShort) && (OldPreamble == Rt802_11PreambleLong))
			/* New Preamble = Short : Old Preamble = Long */
			MCS--;
		else if (((Preamble == Rt802_11PreambleLong) && (OldPreamble == Rt802_11PreambleShort))
#ifdef CONFIG_STA_SUPPORT
			|| ((pAd->OpMode == OPMODE_STA) &&
				(Preamble == Rt802_11PreambleAuto)
				&& (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_SHORT_PREAMBLE_INUSED))))
#else
			)
#endif
				/* New Preamble = Long/Auto : Old Preamble = Short */
				MCS++;

				if ((pTransmit->field.MCS != MCS) || (Preamble != OldPreamble))
					Set_McastBcastMcs_CCK(pAd, MCS);
		}
	}
#endif /* MCAST_VENDOR10_CUSTOM_FEATURE */

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Set_TxPreamble_Proc::(TxPreamble=%ld)\n",
			 pAd->CommonCfg.TxPreamble));
	return TRUE;
}

/*
    ==========================================================================
    Description:
	Set RTS Threshold
    Return:
	TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
static VOID set_rts_len_thld(struct wifi_dev *wdev, UINT32 length)
{
	wlan_operate_set_rts_len_thld(wdev, length);
}

INT Set_RTSThreshold_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	struct wifi_dev *wdev = get_wdev_by_ioctl_idx_and_iftype(pAd, pObj->ioctl_if, pObj->ioctl_if_type);
	UINT32 length = MAX_RTS_THRESHOLD;

	if (arg == NULL) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
				 ("Usage:\niwpriv raN set RTSThreshold=[length]\n"));
		return FALSE;
	}

	if (!wdev)
		return FALSE;

	length = os_str_tol(arg, 0, 10);
	set_rts_len_thld(wdev, length);
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
			 ("%s: set wdev%d rts length threshold=%d(0x%x)\n", __func__, wdev->wdev_idx, length, length));
	return TRUE;
}

/*
    ==========================================================================
    Description:
	Set Fragment Threshold
    Return:
	TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT Set_FragThreshold_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UINT32 frag_thld;
	POS_COOKIE obj = (POS_COOKIE)pAd->OS_Cookie;
	struct wifi_dev *wdev = get_wdev_by_ioctl_idx_and_iftype(pAd, obj->ioctl_if, obj->ioctl_if_type);

	if (!arg)
		return FALSE;

	if (!wdev)
		return FALSE;

	frag_thld = os_str_tol(arg, 0, 10);

	if (frag_thld > MAX_FRAG_THRESHOLD || frag_thld < MIN_FRAG_THRESHOLD)
		frag_thld = MAX_FRAG_THRESHOLD;
	else if ((frag_thld % 2) == 1)
		frag_thld -= 1;

	wlan_operate_set_frag_thld(wdev, frag_thld);
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
			 ("%s::set wdev%d FragThreshold=%d)\n", __func__, wdev->wdev_idx, frag_thld));
	return TRUE;
}

/*
    ==========================================================================
    Description:
	Set TxBurst
    Return:
	TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_TxBurst_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	LONG TxBurst;

	TxBurst = os_str_tol(arg, 0, 10);

	if (TxBurst == 1)
		pAd->CommonCfg.bEnableTxBurst = TRUE;
	else if (TxBurst == 0)
		pAd->CommonCfg.bEnableTxBurst = FALSE;
	else
		return FALSE;  /*Invalid argument */

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Set_TxBurst_Proc::(TxBurst=%d)\n",
			 pAd->CommonCfg.bEnableTxBurst));
	return TRUE;
}

INT Set_MaxTxPwr_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UCHAR IfIdx, BandIdx;
	UCHAR MaxTxPwr = 0;
	CHANNEL_CTRL *pChCtrl;
	struct wifi_dev *wdev = NULL;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;

	IfIdx = pObj->ioctl_if;
#ifdef CONFIG_AP_SUPPORT
	if ((pObj->ioctl_if_type == INT_MBSSID) || (pObj->ioctl_if_type == INT_MAIN))
		wdev = &pAd->ApCfg.MBSSID[IfIdx].wdev;
#endif
#ifdef CONFIG_STA_SUPPORT
	if (pObj->ioctl_if_type == INT_APCLI)
		wdev = &pAd->StaCfg[IfIdx].wdev;
	else if (pObj->ioctl_if_type == INT_MSTA)
		wdev = &pAd->StaCfg[IfIdx].wdev;
#endif
	if (wdev == NULL) {
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			("[Set_MaxTxPwr_Proc]: pObj->ioctl_if_type = %d!!\n",
			pObj->ioctl_if_type));
		return FALSE;
	}

	MaxTxPwr = (UCHAR) simple_strtol(arg, 0, 10);

	if ((MaxTxPwr > 0) && (MaxTxPwr < 0xff)) {
		pAd->MaxTxPwr = MaxTxPwr;
		BandIdx = HcGetBandByWdev(wdev);
		pChCtrl = hc_get_channel_ctrl(pAd->hdev_ctrl, BandIdx);
		hc_set_ChCtrlChListStat(pChCtrl, CH_LIST_STATE_NONE);
#ifdef EXT_BUILD_CHANNEL_LIST
		BuildChannelListEx(pAd, wdev);
#else
		BuildChannelList(pAd, wdev);
#endif
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
			("Set MaxTxPwr = %d\n", MaxTxPwr));
		return TRUE;
	}

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
		("ERROR: wrong power announced(MaxTxPwr=%d)\n", MaxTxPwr));
	return FALSE;

}

#ifdef AGGREGATION_SUPPORT
/*
    ==========================================================================
    Description:
	Set TxBurst
    Return:
	TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_PktAggregate_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	LONG aggre;

	aggre = os_str_tol(arg, 0, 10);

	if (aggre == 1)
		pAd->CommonCfg.bAggregationCapable = TRUE;
	else if (aggre == 0)
		pAd->CommonCfg.bAggregationCapable = FALSE;
	else
		return FALSE;  /*Invalid argument */

#ifdef CONFIG_AP_SUPPORT
#ifdef PIGGYBACK_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd) {
		pAd->CommonCfg.bPiggyBackCapable = pAd->CommonCfg.bAggregationCapable;
		AsicSetPiggyBack(pAd, pAd->CommonCfg.bPiggyBackCapable);
	}
#endif /* PIGGYBACK_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Set_PktAggregate_Proc::(AGGRE=%d)\n",
			 pAd->CommonCfg.bAggregationCapable));
	return TRUE;
}
#endif

/*
    ==========================================================================
    Description:
	Set IEEE80211H.
	This parameter is 1 when needs radar detection, otherwise 0
    Return:
	TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_IEEE80211H_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	LONG ieee80211h;

	ieee80211h = os_str_tol(arg, 0, 10);

	if (ieee80211h == 1)
		pAd->CommonCfg.bIEEE80211H = TRUE;
	else if (ieee80211h == 0)
		pAd->CommonCfg.bIEEE80211H = FALSE;
	else
		return FALSE;  /*Invalid argument */

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Set_IEEE80211H_Proc::(IEEE80211H=%d)\n",
			 pAd->CommonCfg.bIEEE80211H));
	return TRUE;
}

#ifdef EXT_BUILD_CHANNEL_LIST
/*
    ==========================================================================
    Description:
	Set Country Code.
    Return:
	TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT Set_ExtCountryCode_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	if (RTMP_DRIVER_IOCTL_SANITY_CHECK(pAd, NULL) == NDIS_STATUS_SUCCESS) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s can only be used when interface is down.\n", __func__));
		return TRUE;
	}

	if (strlen(arg) == 2) {
		NdisMoveMemory(pAd->CommonCfg.CountryCode, arg, 2);
		pAd->CommonCfg.bCountryFlag = TRUE;
	} else {
		NdisZeroMemory(pAd->CommonCfg.CountryCode, sizeof(pAd->CommonCfg.CountryCode));
		pAd->CommonCfg.bCountryFlag = FALSE;
	}

	{
		UCHAR CountryCode[3] = {0};

		NdisMoveMemory(CountryCode, pAd->CommonCfg.CountryCode, 2);
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Set_CountryCode_Proc::(bCountryFlag=%d, CountryCode=%s)\n",
				 pAd->CommonCfg.bCountryFlag,
				 CountryCode));
	}

	return TRUE;
}
/*
    ==========================================================================
    Description:
	Set Ext DFS Type
    Return:
	TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT Set_ExtDfsType_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UCHAR	 *pDfsType = &pAd->CommonCfg.DfsType;

	if (RTMP_DRIVER_IOCTL_SANITY_CHECK(pAd, NULL) == NDIS_STATUS_SUCCESS) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s can only be used when interface is down.\n", __func__));
		return TRUE;
	}

	if (!strcmp(arg, "CE"))
		*pDfsType = CE;
	else if (!strcmp(arg, "FCC"))
		*pDfsType = FCC;
	else if (!strcmp(arg, "JAP"))
		*pDfsType = JAP;
	else
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Unsupported DFS type:%s (Legal types are: CE, FCC, JAP)\n",
				 arg));

	return TRUE;
}

/*
    ==========================================================================
    Description:
	Add new channel list
    Return:
	TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT Set_ChannelListAdd_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	CH_DESP		inChDesp;
	PCH_REGION pChRegion = NULL;

	if (RTMP_DRIVER_IOCTL_SANITY_CHECK(pAd, NULL) == NDIS_STATUS_SUCCESS) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s can only be used when interface is down.\n", __func__));
		return TRUE;
	}

	/* Get Channel Region (CountryCode)*/
	{
		INT loop = 0;

		while (strcmp((RTMP_STRING *) ChRegion[loop].CountReg, "") != 0) {
			if (strncmp((RTMP_STRING *) ChRegion[loop].CountReg, pAd->CommonCfg.CountryCode, 2) == 0) {
				pChRegion = &ChRegion[loop];
				break;
			}

			loop++;
		}

		if (pChRegion == NULL) {
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("CountryCode is not configured or not valid\n"));
			return TRUE;
		}
	}
	/* Parsing the arg, IN:arg; OUT:inChRegion */
	{
		UCHAR strBuff[64], count = 0;
		PUCHAR	pStart, pEnd, tempIdx, tempBuff[5];

		if (strlen(arg) < 64)
			NdisCopyMemory(strBuff, arg, strlen(arg));

		pStart = rtstrchr(strBuff, '[');

		if (pStart != NULL) {
			pEnd = rtstrchr(pStart++, ']');

			if (pEnd != NULL) {
				tempBuff[count++] = pStart;

				for (tempIdx = pStart; tempIdx != pEnd; tempIdx++) {
					if (*tempIdx == ',') {
						*tempIdx = '\0';
						tempBuff[count++] = ++tempIdx;
					}
				}

				*(pEnd) = '\0';

				if (count != 5) {
					MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Input Error. Too more or too less parameters.\n"));
					return TRUE;
				} else {
					inChDesp.FirstChannel = (UCHAR) os_str_tol(tempBuff[0], 0, 10);
					inChDesp.NumOfCh = (UCHAR) os_str_tol(tempBuff[1], 0, 10);
					inChDesp.MaxTxPwr = (UCHAR) os_str_tol(tempBuff[2], 0, 10);
					inChDesp.Geography = (!strcmp(tempBuff[3], "BOTH") ? BOTH : (!strcmp(tempBuff[3], "IDOR") ? IDOR : ODOR));
					inChDesp.DfsReq = (!strcmp(tempBuff[4], "TRUE") ? TRUE : FALSE);
				}
			} else {
				MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Missing End \"]\"\n"));
				return TRUE;
			}
		} else {
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s: Invalid input format.\n", __func__));
			return TRUE;
		}
	}
	/* Add entry to Channel List*/
	{
		UCHAR EntryIdx;
		PCH_DESP pChDesp = NULL;
		UCHAR CountryCode[3] = {0};

		if (pAd->CommonCfg.pChDesp == NULL) {
			os_alloc_mem(pAd,  &pAd->CommonCfg.pChDesp, MAX_PRECONFIG_DESP_ENTRY_SIZE * sizeof(CH_DESP));
			pChDesp = (PCH_DESP) pAd->CommonCfg.pChDesp;

			if (pChDesp) {
				for (EntryIdx = 0; pChRegion->pChDesp[EntryIdx].FirstChannel != 0; EntryIdx++) {
					if (EntryIdx == (MAX_PRECONFIG_DESP_ENTRY_SIZE - 2)) { /* Keep an NULL entry in the end of table*/
						MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Table is full.\n"));
						return TRUE;
					}

					NdisCopyMemory(&pChDesp[EntryIdx], &pChRegion->pChDesp[EntryIdx], sizeof(CH_DESP));
				}

				/* Copy the NULL entry*/
				NdisCopyMemory(&pChDesp[EntryIdx], &pChRegion->pChDesp[EntryIdx], sizeof(CH_DESP));
			} else {
				MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("os_alloc_mem failded.\n"));
				return FALSE;
			}
		} else {
			pChDesp = (PCH_DESP) pAd->CommonCfg.pChDesp;

			for (EntryIdx = 0; pChDesp[EntryIdx].FirstChannel != 0; EntryIdx++) {
				if (EntryIdx ==  (MAX_PRECONFIG_DESP_ENTRY_SIZE - 2)) { /* Keep an NULL entry in the end of table*/
					MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Table is full.\n"));
					return TRUE;
				}
			}
		}

		NdisMoveMemory(CountryCode, pAd->CommonCfg.CountryCode, 2);
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Add channel lists {%u, %u, %u, %s, %s} to %s.\n",
				 inChDesp.FirstChannel,
				 inChDesp.NumOfCh,
				 inChDesp.MaxTxPwr,
				 (inChDesp.Geography == BOTH) ? "BOTH" : (inChDesp.Geography == IDOR) ?  "IDOR" : "ODOR",
				 (inChDesp.DfsReq == TRUE) ? "TRUE" : "FALSE",
				 CountryCode));
		NdisCopyMemory(&pChDesp[EntryIdx], &inChDesp, sizeof(CH_DESP));
		pChDesp[++EntryIdx].FirstChannel = 0;
	}
	return TRUE;
}

INT Set_ChannelListShow_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	PCH_REGION	pChRegion = NULL;
	UCHAR		EntryIdx, CountryCode[3] = {0};
	/* Get Channel Region (CountryCode)*/
	{
		INT loop = 0;

		while (strcmp((RTMP_STRING *) ChRegion[loop].CountReg, "") != 0) {
			if (strncmp((RTMP_STRING *) ChRegion[loop].CountReg, pAd->CommonCfg.CountryCode, 2) == 0) {
				pChRegion = &ChRegion[loop];
				break;
			}

			loop++;
		}

		if (pChRegion == NULL) {
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("CountryCode is not configured or not valid\n"));
			return TRUE;
		}
	}
	NdisMoveMemory(CountryCode, pAd->CommonCfg.CountryCode, 2);

	if (pAd->CommonCfg.DfsType == MAX_RD_REGION)
		pAd->CommonCfg.DfsType = pChRegion->op_class_region;

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("=========================================\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("CountryCode:%s\n", CountryCode));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("DfsType:%s\n",
			 (pAd->CommonCfg.DfsType == JAP) ? "JAP" :
			 ((pAd->CommonCfg.DfsType == FCC) ? "FCC" : "CE")));

	if (pAd->CommonCfg.pChDesp != NULL) {
		PCH_DESP pChDesp = (PCH_DESP) pAd->CommonCfg.pChDesp;

		for (EntryIdx = 0; pChDesp[EntryIdx].FirstChannel != 0; EntryIdx++) {
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%u. {%3u, %2u, %2u, %s, %5s}.\n",
					 EntryIdx,
					 pChDesp[EntryIdx].FirstChannel,
					 pChDesp[EntryIdx].NumOfCh,
					 pChDesp[EntryIdx].MaxTxPwr,
					 (pChDesp[EntryIdx].Geography == BOTH) ? "BOTH" : (pChDesp[EntryIdx].Geography == IDOR) ?  "IDOR" : "ODOR",
					 (pChDesp[EntryIdx].DfsReq == TRUE) ? "TRUE" : "FALSE"));
		}
	} else {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Default channel list table:\n"));

		for (EntryIdx = 0; pChRegion->pChDesp[EntryIdx].FirstChannel != 0; EntryIdx++) {
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%u. {%3u, %2u, %2u, %s, %5s}.\n",
					 EntryIdx,
					 pChRegion->pChDesp[EntryIdx].FirstChannel,
					 pChRegion->pChDesp[EntryIdx].NumOfCh,
					 pChRegion->pChDesp[EntryIdx].MaxTxPwr,
					 (pChRegion->pChDesp[EntryIdx].Geography == BOTH) ? "BOTH" : (pChRegion->pChDesp[EntryIdx].Geography == IDOR) ?  "IDOR" :
					 "ODOR",
					 (pChRegion->pChDesp[EntryIdx].DfsReq == TRUE) ? "TRUE" : "FALSE"));
		}
	}

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("=========================================\n"));
	return TRUE;
}

INT Set_ChannelListDel_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UCHAR EntryIdx, TargetIdx, NumOfEntry;
	PCH_REGION	pChRegion = NULL;
	PCH_DESP pChDesp = NULL;

	TargetIdx = os_str_tol(arg, 0, 10);

	if (RTMP_DRIVER_IOCTL_SANITY_CHECK(pAd, NULL) == NDIS_STATUS_SUCCESS) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s can only be used when interface is down.\n", __func__));
		return TRUE;
	}

	/* Get Channel Region (CountryCode)*/
	{
		INT loop = 0;

		while (strcmp((RTMP_STRING *) ChRegion[loop].CountReg, "") != 0) {
			if (strncmp((RTMP_STRING *) ChRegion[loop].CountReg, pAd->CommonCfg.CountryCode, 2) == 0) {
				pChRegion = &ChRegion[loop];
				break;
			}

			loop++;
		}

		if (pChRegion == NULL) {
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("CountryCode is not configured or not valid\n"));
			return TRUE;
		}
	}

	if (pAd->CommonCfg.pChDesp == NULL) {
		os_alloc_mem(pAd,  &pAd->CommonCfg.pChDesp, MAX_PRECONFIG_DESP_ENTRY_SIZE * sizeof(CH_DESP));

		if (pAd->CommonCfg.pChDesp) {
			pChDesp = (PCH_DESP) pAd->CommonCfg.pChDesp;

			for (EntryIdx = 0; pChRegion->pChDesp[EntryIdx].FirstChannel != 0; EntryIdx++) {
				if (EntryIdx == (MAX_PRECONFIG_DESP_ENTRY_SIZE - 2)) { /* Keep an NULL entry in the end of table*/
					MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Table is full.\n"));
					return TRUE;
				}

				NdisCopyMemory(&pChDesp[EntryIdx], &pChRegion->pChDesp[EntryIdx], sizeof(CH_DESP));
			}

			/* Copy the NULL entry*/
			NdisCopyMemory(&pChDesp[EntryIdx], &pChRegion->pChDesp[EntryIdx], sizeof(CH_DESP));
		} else {
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("os_alloc_mem failded.\n"));
			return FALSE;
		}
	} else
		pChDesp = (PCH_DESP) pAd->CommonCfg.pChDesp;

	if (!strcmp(arg, "default")) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Default table used.\n"));

		if (pAd->CommonCfg.pChDesp != NULL)
			os_free_mem(pAd->CommonCfg.pChDesp);

		pAd->CommonCfg.pChDesp = NULL;
		pAd->CommonCfg.DfsType = MAX_RD_REGION;
	} else if (!strcmp(arg, "all")) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Remove all entries.\n"));

		for (EntryIdx = 0; EntryIdx < MAX_PRECONFIG_DESP_ENTRY_SIZE; EntryIdx++)
			NdisZeroMemory(&pChDesp[EntryIdx], sizeof(CH_DESP));
	} else if (TargetIdx < (MAX_PRECONFIG_DESP_ENTRY_SIZE - 1)) {
		for (EntryIdx = 0; pChDesp[EntryIdx].FirstChannel != 0; EntryIdx++) {
			if (EntryIdx ==  (MAX_PRECONFIG_DESP_ENTRY_SIZE - 2)) { /* Keep an NULL entry in the end of table */
				MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Last entry should be NULL.\n"));
				pChDesp[EntryIdx].FirstChannel = 0;
				return TRUE;
			}
		}

		NumOfEntry = EntryIdx;

		if (TargetIdx >= NumOfEntry) {
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Out of table range.\n"));
			return TRUE;
		}

		for (EntryIdx = TargetIdx; EntryIdx < NumOfEntry; EntryIdx++)
			NdisCopyMemory(&pChDesp[EntryIdx], &pChDesp[EntryIdx + 1], sizeof(CH_DESP));

		NdisZeroMemory(&pChDesp[EntryIdx], sizeof(CH_DESP)); /*NULL entry*/
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Entry %u deleted.\n", TargetIdx));
	} else
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Entry not found.\n"));

	return TRUE;
}
#endif /* EXT_BUILD_CHANNEL_LIST  */

#ifdef WSC_INCLUDED
INT	Set_WscGenPinCode_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	PWSC_CTRL   pWscControl = NULL;
	POS_COOKIE  pObj;
	UCHAR       apidx;

	if (pAd == NULL) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s: pAd == NULL!\n", __func__));
		return TRUE;
	}

	pObj = (POS_COOKIE) pAd->OS_Cookie;
	apidx = pObj->ioctl_if;
#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd) {
#ifdef APCLI_SUPPORT

		if (pObj->ioctl_if_type == INT_APCLI) {
			pWscControl = &pAd->StaCfg[apidx].wdev.WscControl;
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
					 ("IF(apcli%d) Set_WscGenPinCode_Proc:: This command is from apcli interface now.\n", apidx));
		} else
#endif /* APCLI_SUPPORT */
		{
			pWscControl = &pAd->ApCfg.MBSSID[apidx].wdev.WscControl;
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
					 ("IF(ra%d) Set_WscGenPinCode_Proc:: This command is from ra interface now.\n", apidx));
		}
	}
#endif /* CONFIG_AP_SUPPORT */
#ifdef P2P_SUPPORT

	if (pObj->ioctl_if_type == INT_P2P) {
		pWscControl = &pAd->StaCfg[apidx].wdev.WscControl;
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
				 ("IF(p2p%d) Set_WscGenPinCode_Proc:: This command is from apcli interface now.\n", apidx));
	}

#endif /* P2P_SUPPORT */
#ifdef CONFIG_STA_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_STA(pAd) {
#ifdef P2P_SUPPORT

		if (pObj->ioctl_if_type != INT_P2P)
#endif /* P2P_SUPPORT */
		{
			pWscControl = &pAd->StaCfg[pObj->ioctl_if].wdev.WscControl;
		}
	}
#endif /* CONFIG_STA_SUPPORT */

	if (pWscControl == NULL) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s: pWscControl == NULL!\n", __func__));
		return TRUE;
	}

	if (pWscControl->WscEnrollee4digitPinCode) {
		pWscControl->WscEnrolleePinCodeLen = 4;
		pWscControl->WscEnrolleePinCode = WscRandomGen4digitPinCode(pAd);
	} else {
		pWscControl->WscEnrolleePinCodeLen = 8;
#ifdef P2P_SSUPPORT
#ifdef WIDI_SUPPORT

		if (pObj->ioctl_if_type == INT_P2P)
			pWscControl->WscEnrolleePinCode = WscRandomGenerateP2PPinCode(pAd, apidx);
		else
#endif /* WIDI_SUPPORT */
#endif /* P2P_SUPPORT */
			pWscControl->WscEnrolleePinCode = WscRandomGeneratePinCode(pAd, apidx);
	}

#ifdef P2P_SUPPORT

	if (pObj->ioctl_if_type == INT_P2P) {
		PWSC_CTRL   pApWscControl = &pAd->ApCfg.MBSSID[apidx].wdev.WscControl;

		pApWscControl->WscEnrolleePinCodeLen  = pWscControl->WscEnrolleePinCodeLen;
		pApWscControl->WscEnrolleePinCode  = pWscControl->WscEnrolleePinCode;
	}

#endif /* P2P_SUPPORT */
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Set_WscGenPinCode_Proc:: Enrollee PinCode\t\t%08u\n",
			 pWscControl->WscEnrolleePinCode));
	return TRUE;
}

#ifdef BB_SOC
INT	Set_WscResetPinCode_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	PWSC_CTRL   pWscControl = NULL;
	POS_COOKIE  pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR	    apidx = pObj->ioctl_if;
#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd) {
		{
			pWscControl = &pAd->ApCfg.MBSSID[apidx].wdev.WscControl;
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
					 ("IF(ra%d) Set_WscResetPinCode_Proc:: This command is from ra interface now.\n", apidx));
		}
		pWscControl->WscEnrolleePinCode = GenerateWpsPinCode(pAd, 0, apidx);
	}
#endif /* CONFIG_AP_SUPPORT // */
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Set_WscResetPinCode_Proc:: Enrollee PinCode\t\t%08u\n",
			 pWscControl->WscEnrolleePinCode));
	return TRUE;
}
#endif

INT Set_WscVendorPinCode_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	PWSC_CTRL   pWscControl = NULL;
	POS_COOKIE  pObj = (POS_COOKIE) pAd->OS_Cookie;
#ifdef CONFIG_AP_SUPPORT
	UCHAR       apidx = pObj->ioctl_if;

	IF_DEV_CONFIG_OPMODE_ON_AP(pAd) {
#ifdef APCLI_SUPPORT

		if (pObj->ioctl_if_type == INT_APCLI) {
			pWscControl = &pAd->StaCfg[apidx].wdev.WscControl;
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Set_WscVendorPinCode_Proc() for apcli(%d)\n", apidx));
		} else
#endif /* APCLI_SUPPORT */
		{
			pWscControl = &pAd->ApCfg.MBSSID[apidx].wdev.WscControl;
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Set_WscVendorPinCode_Proc() for ra%d!\n", apidx));
		}
	}
#endif /* CONFIG_AP_SUPPORT */
#ifdef P2P_SUPPORT

	if (pObj->ioctl_if_type == INT_P2P) {
		pWscControl = &pAd->ApCfg.MBSSID[apidx].wdev.WscControl;
		RT_CfgSetWscPinCode(pAd, arg, pWscControl);
		pWscControl = &pAd->StaCfg[apidx].wdev.WscControl;
		RT_CfgSetWscPinCode(pAd, arg, pWscControl);
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Set_WscVendorPinCode_Proc() for p2p(%d)\n", apidx));
		return TRUE;
	}

#endif /* P2P_SUPPORT */
#ifdef CONFIG_STA_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_STA(pAd) {
#ifdef P2P_SUPPORT

		if (pObj->ioctl_if_type != INT_P2P)
#endif /* P2P_SUPPORT */
			pWscControl = &pAd->StaCfg[pObj->ioctl_if].wdev.WscControl;
	}
#endif /* CONFIG_STA_SUPPORT */

	if (!pWscControl)
		return FALSE;
	else
		return RT_CfgSetWscPinCode(pAd, arg, pWscControl);
}
#endif /* WSC_INCLUDED */


#ifdef DBG
INT rx_temp_dbg;

static PCHAR cat_str[32] = {
	"MISC",
	"INIT",
	"HW",
	"FW",
	"HIF",
	"FPGA",
	"TEST",
	"RA",
	"AP",
	"CLIENT",
	"TX",
	"RX",
	"CFG",
	"MLME",
	"PROTO",
	"SEC",
	"PS",
	"POWER",
	"COEX",
	"P2P",
	"TOKEN",
	"CMW",
	"BF",
};


static PCHAR sub_cat_str[32][32] = {
	{"MISC"}, /* misc */
	{"MISC"}, /* initialization/shutdown */
	{"MISC", "SA"}, /* MAC/BBP/RF/Chip */
	{"MISC"}, /* FW related command, response, CR that FW care about */
	{"MISC", "PCI", "USB", "SDIO"}, /* Host interface: usb/sdio/pcie/rbus */
	{"MISC"}, /* FPGA Chip verify, DVT */
	{"MISC"}, /* ATE, QA, UT, FPGA?, TDT, SLT, WHQL, and other TEST */
	{"MISC"}, /* Rate Adaption/Throughput related */
	{"MISC", "MBSS", "WDS", "BCN"}, /* AP, MBSS, WDS, Beacon */
	{"MISC", "ADHOC", "APCLI", "MESH"}, /* STA, ApClient, AdHoc, Mesh */
	{"MISC", "TMAC"}, /* Tx data path */
	{"MISC"}, /* Rx data path */
	{"MISC"}, /* ioctl/oid/profile/cfg80211/Registry */
	{"MISC", "WTBL"}, /* 802.11 fundamental connection flow, auth, assoc, disconnect, etc */
	{"MISC", "ACM", "BA", "TDLS", "WNM", "IGMP", "MAT", "RRM", "DFS", "FT", "SCAN", "FTM", "OCE", "TWT", "COLOR"}, /* protocol, ex. TDLS */
	{"MISC", "EY", "WPS", "WAPI", "PMF", "SAE", "SUITEB", "OWE"}, /* security/key/WPS/WAPI/PMF/11i related*/
	{"MISC", "UAPSD"}, /* power saving/UAPSD */
	{"MISC"}, /* power Setting, Single Sku, Temperature comp, etc */
	{"MISC"}, /* BT, BT WiFi Coex, LTE, TVWS*/
	{"MISC"}, /* P2P, Miracast */
	{"MISC", "INFO", "PROFILE", "TRACE"}, /* token */
	{"MISC"}, /* CMW Link Test related */
	{"MISC", "IWCMD", "ASSOC"}, /* BF: MISC: All. IWCMD: iwpriv BF commands. ASSOC: During STA association */
};


/*
    ==========================================================================
    Description:
	For Debug information
	Change DebugLevel
    Return:
	TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT Set_Debug_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UINT32 dbg_lvl = 0;
	UINT32 dbg_cat = 0;
	UINT32 dbg_sub_cat = 0;
	UINT32 i;
	UINT32 j;
	RTMP_STRING *str  = NULL;
	RTMP_STRING *str2  = NULL;
	UCHAR para = 3;
	UCHAR show_usage_para = 0;

	if (arg == NULL || strlen(arg) == 0)
		goto format_error;

	str = strsep(&arg, ":");

	if (arg == NULL) {
		para = 1;
		if (rtstrcasecmp(str, "?") == TRUE)
			show_usage_para = 1;
	} else {
		str2 = strsep(&arg, ":");
		if (arg == NULL) {
			para = 2;
			if (rtstrcasecmp(str2, "?") == TRUE)
				show_usage_para = 2;
		} else if (rtstrcasecmp(arg, "?") == TRUE)
			show_usage_para = 3;
	}

	dbg_lvl = os_str_tol(str, 0, 10);
	if (para >= 2)
		dbg_cat = os_str_tol(str2, 0, 10);
	if (para >= 3)
		dbg_sub_cat = os_str_tol(arg, 0, 10);

	if (show_usage_para == 1) {
		MTWF_PRINT("usage and current state:\n");
		for (j = 0; j < 32; j++) {
			if (cat_str[j] == NULL)
				break;
			for (i = DBG_LVL_MAX; i != 0; i--)
				if (DebugSubCategory[i][j] != 0) {
					MTWF_PRINT("%2d:%s(L%d", j, cat_str[j], i);
					if (DebugSubCategory[i][j] != DBG_SUBCAT_EN_ALL_MASK)
						MTWF_PRINT("*");
					MTWF_PRINT(")\t");
					break;
				}
			if ((j + 1) % 4 == 0)
				MTWF_PRINT("\n");
		}

		MTWF_PRINT("\n");
		return TRUE;
	} else if (show_usage_para == 2) {
		MTWF_PRINT("usage and current state for DebugLevel %d:\n", dbg_lvl);
		for (j = 0; j < 32; j++) {
			if (cat_str[j] == NULL)
				break;
			MTWF_PRINT("%2d:%s(0x%08x)\t", j, cat_str[j], DebugSubCategory[dbg_lvl][j]);
			if ((j + 1) % 4 == 0)
				MTWF_PRINT("\n");
		}

		MTWF_PRINT("\n");
		return TRUE;
	} else if (show_usage_para == 3) {
		MTWF_PRINT("usage and current state for DebugLevel %d, Category %d(%s):\n", dbg_lvl, dbg_cat, cat_str[dbg_cat]);
		for (j = 0; j < 32; j++) {
			if (sub_cat_str[dbg_cat][j] == NULL)
				break;
			MTWF_PRINT("%2d:%s(", j, sub_cat_str[dbg_cat][j]);
			if (DebugSubCategory[dbg_lvl][dbg_cat] & (0x1 << j))
				MTWF_PRINT("on)\t");
			else
				MTWF_PRINT("off)\t");
			if ((j + 1) % 4 == 0)
				MTWF_PRINT("\n");
		}

		MTWF_PRINT("\n");
		return TRUE;
	}

	if (dbg_lvl <= DBG_LVL_MAX) {
		if (para == 1) {
			DebugLevel = dbg_lvl;
			for (i = 0; i <= DBG_LVL_MAX; i++) {
				UINT32 tmp = (i <= dbg_lvl) ? DBG_SUBCAT_EN_ALL_MASK : 0;

				for (j = 0; j < 32; j++)
					DebugSubCategory[i][j] = tmp;
			}
		} else if (para == 2) {
			if (dbg_cat > DBG_CAT_MAX)
				goto format_error;
			for (i = 0; i <= DBG_LVL_MAX; i++) {
				UINT32 tmp = (i <= dbg_lvl) ? DBG_SUBCAT_EN_ALL_MASK : 0;

				MTWF_PRINT("%s(): change cat%d(level %d) from %x to ", __func__,
					dbg_cat, i, DebugSubCategory[i][dbg_cat]);
				DebugSubCategory[i][dbg_cat] = tmp;
				MTWF_PRINT("%x\n", DebugSubCategory[i][dbg_cat]);
			}
#if defined(CONFIG_WLAN_SERVICE)
			if (dbg_cat == 6) {	/* test mode */
				extern UINT32 serv_dbg_lvl;

				serv_dbg_lvl = dbg_lvl;
			}
#endif	/* CONFIG_WLAN_SERVICE */
		} else if (para == 3) {
			if (dbg_sub_cat > 31)
				goto format_error;
			for (i = 0; i <= DBG_LVL_MAX; i++) {
				MTWF_PRINT("%s(): change cat%d(level %d) from %x to ", __func__,
					dbg_cat, i, DebugSubCategory[i][dbg_cat]);
				if (i <= dbg_lvl)
					DebugSubCategory[i][dbg_cat] |= (0x1 << dbg_sub_cat);
				else
					DebugSubCategory[i][dbg_cat] &= ~(0x1 << dbg_sub_cat);
				MTWF_PRINT("%x\n", DebugSubCategory[i][dbg_cat]);
			}
		}
	} else
		goto format_error;

	MTWF_PRINT("%s(): (DebugLevel = %d)\n", __func__, DebugLevel);
	return TRUE;

format_error:
	MTWF_PRINT("Format error! correct format:\n");
	MTWF_PRINT("iwpriv ra0 set Debug=[DebugLevel]:[DebugCat]:[DebugSubCat]\n");
	MTWF_PRINT("\t[DebugLevel]:0~6 or ?\n");
	MTWF_PRINT("\t[DebugCat]:0~31 or ?, optional\n");
	MTWF_PRINT("\t[DebugSubCat]:0~31 or ?, optional\n");
	MTWF_PRINT("EX: 1.iwpriv ra0 set Debug=2\n");
	MTWF_PRINT("\t DebugSubCategory[0~2][0~31] = 0xffffffff, DebugSubCategory[3~6][0~31] = 0\n");
	MTWF_PRINT("    2.iwpriv ra0 set Debug=4:5\n");
	MTWF_PRINT("\t DebugSubCategory[0~4][5] = 0xffffffff, DebugSubCategory[5~6][5] = 0\n");
	MTWF_PRINT("    3.iwpriv ra0 set Debug=3:10:7\n");
	MTWF_PRINT("\t DebugSubCategory[0~3][10] |= (0x1 << 7), DebugSubCategory[4~6][10] &= ~(0x1 << 7)\n");
	MTWF_PRINT("    4.iwpriv ra0 set Debug=?\n");
	MTWF_PRINT("\t query category list and current debuglevel value for each category\n");
	MTWF_PRINT("    5.iwpriv ra0 set Debug=3:?\n");
	MTWF_PRINT("\t query category list and current subcategory bitmap value for each category at DebugLevel 3\n");
	MTWF_PRINT("    6.iwpriv ra0 set Debug=2:8:?\n");
	MTWF_PRINT("\t query subcategory list and current subcategory on/off state for category 8 at DebugLevel 2\n");

	return FALSE;
}
#ifdef CUSTOMER_VENDOR_IE_SUPPORT
INT Set_Max_ProbeRsp_IE_Cnt_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UINT32 ap_probe_rsp_vendor_ie_max_count;

	ap_probe_rsp_vendor_ie_max_count = simple_strtol(arg, 0, 10);

	pAd->ApCfg.ap_probe_rsp_vendor_ie_max_count = ap_probe_rsp_vendor_ie_max_count;

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, 
		("%s: ap_probe_rsp_vendor_ie_max_count: %d \n",
		__func__, ap_probe_rsp_vendor_ie_max_count));
	return TRUE;
}
#endif /*CUSTOMER_VENDOR_IE_SUPPORT*/

#ifdef RATE_PRIOR_SUPPORT
INT Set_RatePrior_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	ULONG RatePrior;
	INT idx;
	PMAC_TABLE_ENTRY pEntry = NULL;
	PBLACK_STA pBlackSta = NULL, tmp;
	UINT16 wtbl_max_num;

	RatePrior = simple_strtol(arg, 0, 10);
	if (RatePrior == 1) {
		pAd->LowRateCtrl.RatePrior = 1;
		pAd->LowRateCtrl.LowRateRatioThreshold = 2;
		pAd->LowRateCtrl.LowRateCountPeriod = 5;
		pAd->LowRateCtrl.TotalCntThreshold = 50;
		pAd->LowRateCtrl.BlackListTimeout = 30;
	} else {
		pAd->LowRateCtrl.RatePrior = 0;
		/*clear the list*/
		RTMP_SEM_LOCK(&pAd->LowRateCtrl.BlackListLock);
		DlListForEach(pBlackSta, &pAd->LowRateCtrl.BlackList, BLACK_STA, List) {
				MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
						("Remove from blklist, %02x:%02x:%02x:%02x:%02x:%02x\n", PRINT_MAC(pBlackSta->Addr)));
				tmp = pBlackSta;
				pBlackSta = DlListEntry(pBlackSta->List.Prev, BLACK_STA, List);
				DlListDel(&(tmp->List));
				os_free_mem(tmp);
		}
		RTMP_SEM_UNLOCK(&pAd->LowRateCtrl.BlackListLock);
		/*clear entry info*/
		wtbl_max_num = WTBL_MAX_NUM(pAd);
		for (idx = 1; idx < wtbl_max_num; idx++) {
			pEntry = &(pAd->MacTab.Content[idx]);
			if (pEntry != NULL) {
				pEntry->McsTotalRxCount = 0;
				pEntry->McsLowRateRxCount = 0;
			}
		}
}

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s: RatePrior: %s \n",
		__func__, RatePrior == 1 ? "Enable" : "Disable"));
	return TRUE;
}

INT Set_BlackListTimeout_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UINT BlackListTimeout;

	BlackListTimeout = simple_strtol(arg, 0, 10);

	if (BlackListTimeout <= 0) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s: ERROR! BlackListTimeout value: %d \n",
		__func__, BlackListTimeout));
		return FALSE;
	}

	pAd->LowRateCtrl.BlackListTimeout = BlackListTimeout;
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s: BlackListTimeout: %d \n",
		__func__, BlackListTimeout));
	return TRUE;
}


INT Set_LowRateRatio_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UINT LowRateRatioThreshold;

	LowRateRatioThreshold = simple_strtol(arg, 0, 10);
	if (LowRateRatioThreshold <= 0) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s: ERROR! LowRateRatioThreshold value: %d \n",
		__func__, LowRateRatioThreshold));
		return FALSE;
	}

	pAd->LowRateCtrl.LowRateRatioThreshold = LowRateRatioThreshold;
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s: LowRateRatioThreshold: %d \n",
		__func__, LowRateRatioThreshold));
	return TRUE;
}

INT Set_LowRateCountPeriod_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UINT LowRateCountPeriod;

	LowRateCountPeriod = simple_strtol(arg, 0, 10);
	if (LowRateCountPeriod <= 0) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s: ERROR! LowRateCountPeriod value: %d \n",
		__func__, LowRateCountPeriod));
		return FALSE;
	}

	pAd->LowRateCtrl.LowRateCountPeriod = LowRateCountPeriod;
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s: LowRateCountPeriod: %d \n",
		__func__, LowRateCountPeriod));
	return TRUE;
}

INT Set_TotalCntThreshold_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UINT TotalCntThreshold;

	TotalCntThreshold = simple_strtol(arg, 0, 10);
	if (TotalCntThreshold <= 0) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s: ERROR! TotalCntThreshold value: %d \n",
		__func__, TotalCntThreshold));
		return FALSE;
	}

	pAd->LowRateCtrl.TotalCntThreshold = TotalCntThreshold;
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s: TotalCntThreshold: %d \n",
		__func__, TotalCntThreshold));
	return TRUE;
}

#endif /*RATE_PRIOR_SUPPORT*/

/*
    ==========================================================================
    Description:
	Change DebugCategory
    Return:
	TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_DebugCategory_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UINT32 category = (UINT32)os_str_tol(arg, 0, 16);

	DebugCategory = category;
	MTWF_PRINT("%s(): Set DebugCategory = 0x%x\n", __func__, DebugCategory);
	return TRUE;
}

static BOOLEAN ascii2hex(RTMP_STRING *in, UINT32 *out)
{
	UINT32 hex_val, val;
	CHAR *p, asc_val;

	hex_val = 0;
	p = (char *)in;

	while ((*p) != 0) {
		val = 0;
		asc_val = *p;

		if ((asc_val >= 'a') && (asc_val <= 'f'))
			val = asc_val - 87;
		else if ((*p >= 'A') && (asc_val <= 'F'))
			val = asc_val - 55;
		else if ((asc_val >= '0') && (asc_val <= '9'))
			val = asc_val - 48;
		else
			return FALSE;

		hex_val = (hex_val << 4) + val;
		p++;
	}

	*out = hex_val;
	return TRUE;
}

#if defined(CONFIG_ARCH_MT7623) || defined(CONFIG_ARCH_MT8590) || defined(CONFIG_ARCH_MT7622)
BOOLEAN mt_mac_cr_range_mapping(RTMP_ADAPTER *pAd, UINT32 *mac_addr);
#endif

/*
    ==========================================================================
    Description:
	Read / Write MAC
    Arguments:
	pAd                    Pointer to our adapter
	wrq                         Pointer to the ioctl argument

    Return Value:
	None

    Note:
	Usage:
	       1.) iwpriv ra0 mac 0        ==> read MAC where Addr=0x0
	       2.) iwpriv ra0 mac 0=12     ==> write MAC where Addr=0x0, value=12
    ==========================================================================
*/
VOID RTMPIoctlMAC(RTMP_ADAPTER *pAd, RTMP_IOCTL_INPUT_STRUCT *wrq)
{
	RTMP_STRING *seg_str, *addr_str, *val_str, *range_str;
	RTMP_STRING *mpool, *msg;
	RTMP_STRING *arg, *ptr;
	UINT32 macVal = 0;
	BOOLEAN bFromUI, is_write, is_range;
	UINT32 IdMac, map_addr, mac_s = 0, mac_e = 0;
	BOOLEAN IsFound;

	os_alloc_mem(NULL, (UCHAR **)&mpool, sizeof(CHAR) * (4096 + 256 + 12));

	if (!mpool)
		return;

	bFromUI = ((wrq->u.data.flags & RTPRIV_IOCTL_FLAG_UI) == RTPRIV_IOCTL_FLAG_UI) ? TRUE : FALSE;
	msg = (RTMP_STRING *)((ULONG)(mpool + 3) & (ULONG)~0x03);
	arg = (RTMP_STRING *)((ULONG)(msg + 4096 + 3) & (ULONG)~0x03);
	memset(msg, 0x00, 4096);
	memset(arg, 0x00, 256);

	if (wrq->u.data.length > 1) {
#ifdef LINUX
		INT Status = NDIS_STATUS_SUCCESS;

		Status = copy_from_user(arg, wrq->u.data.pointer, (wrq->u.data.length > 255) ? 255 : wrq->u.data.length);
#else
		NdisMoveMemory(arg, wrq->u.data.pointer, (wrq->u.data.length > 255) ? 255 : wrq->u.data.length);
#endif /* LINUX */
		arg[255] = 0x00;
	}

	ptr = arg;

	if ((ptr != NULL) && (strlen(ptr) > 0)) {
		while ((*ptr != 0) && (*ptr == 0x20)) /* remove space */
			ptr++;

		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s():after trim space, ptr len=%zu, pointer(%p)=%s!\n",
				 __func__, strlen(ptr), ptr, ptr));
	}

	{
		while ((seg_str = strsep((char **)&ptr, ",")) != NULL) {
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("seg_str[%zu]=%s\n", strlen(seg_str), seg_str));
			is_write = FALSE;
			addr_str = seg_str;
			val_str = NULL;
			val_str = strchr(seg_str, '=');

			if (val_str != NULL) {
				*val_str++ = 0;
				is_write = 1;
			} else
				is_write = 0;

			if (addr_str) {
				range_str = strchr(addr_str, '-');

				if (range_str != NULL) {
					*range_str++ = 0;
					is_range = 1;
				} else
					is_range = 0;

				if ((ascii2hex(addr_str, &mac_s) == FALSE)) {
					MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("Invalid MAC CR Addr, str=%s\n", addr_str));
					break;
				}

				if (is_range) {
					if (ascii2hex(range_str, &mac_e) == FALSE) {
						MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("Invalid Range End MAC CR Addr[0x%x], str=%s\n",
								 mac_e, range_str));
						break;
					}

					if (mac_e < mac_s) {
						MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("Invalid Range MAC Addr[%s - %s] => [0x%x - 0x%x]\n",
								 addr_str, range_str, mac_s, mac_e));
						break;
					}
				} else
					mac_e = mac_s;
			}

			if (val_str) {
				if ((strlen(val_str) == 0) || ascii2hex(val_str, &macVal) == FALSE) {
					MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("Invalid MAC value[0x%s]\n", val_str));
					break;
				}
			}

			if (is_write) {
				RTMP_IO_WRITE32(pAd->hdev_ctrl, mac_s, macVal);

				/* call mt_mac_cr_range_mapping here is only for debugging purpose */
				map_addr = mac_s;
				IsFound = mt_mac_cr_range_mapping(pAd, &map_addr);
				sprintf(msg + strlen(msg), "[0x%04x]:%08x  ", map_addr, macVal);
				MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("MacAddr=0x%x, MacValue=0x%x, IsRemap=%d\n", map_addr, macVal, !IsFound));
			} else {
				for (IdMac = mac_s; IdMac <= mac_e; IdMac += 4) {
					RTMP_IO_READ32(pAd->hdev_ctrl, IdMac, &macVal);

					/* call mt_mac_cr_range_mapping here is only for debugging purpose */
					map_addr = IdMac;
					IsFound = mt_mac_cr_range_mapping(pAd, &map_addr);
					sprintf(msg + strlen(msg), "[0x%04x]:%08x  ", map_addr, macVal);
					MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("MacAddr=0x%x, MacValue=0x%x, IsRemap=%d\n", map_addr, macVal, !IsFound));
				}
			}

			if (ptr)
				MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("NextRound: ptr[%zu]=%s\n", strlen(ptr), ptr));
		}
	}

	if (strlen(msg) == 1)
		sprintf(msg + strlen(msg), "===>Error command format!");

#ifdef LINUX
	/* Copy the information into the user buffer */
	wrq->u.data.length = strlen(msg);

	if (copy_to_user(wrq->u.data.pointer, msg, wrq->u.data.length))
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s: copy_to_user() fail\n", __func__));

#endif /* LINUX */

	os_free_mem(mpool);
}

#endif /* DBG */

#ifdef DOT11_HE_AX
INT set_color_dbg(RTMP_ADAPTER *ad, RTMP_STRING *arg)
{
	UINT32 wdev_idx, action, value;

	if (arg == NULL || strlen(arg) == 0)
		goto format_error;

	if (sscanf(arg, "%d:%d:%d", &wdev_idx, &action, &value) != 3)
		goto format_error;

	if (wdev_idx < 0 || wdev_idx >= WDEV_NUM_MAX) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("wdev_index is out of range\n"));
		return FALSE;
	}
	if (action < 0 || action > BSS_COLOR_DBG_MAX) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("action is out of range\n"));
		return FALSE;
	}

	set_bss_color_dbg(ad, (UINT8)wdev_idx, (UINT8)action, (UINT8)value);
	return TRUE;

format_error:
	MTWF_PRINT("iwpriv ra0 set color_dbg=[wdev_idx]:[action]:[value]\n");
	MTWF_PRINT("[action] 1:occupy, 2:setperiod, 3:trigger, 4: change\n");
	MTWF_PRINT("         5:assign manually 6: change manually\n");
	return FALSE;
}


/*
 *   ==========================================================================
 *   Description:
 *   Set TXOP Duration based RTS Threshold
 *   Return:
 *   TRUE if all parameters are OK, FALSE otherwise
 *   ==========================================================================
 */
static VOID set_txop_dur_rts_thld(struct wifi_dev *wdev, UINT16 txop_dur_thld)
{
	struct _RTMP_ADAPTER *ad = (struct _RTMP_ADAPTER *)wdev->sys_handle;

	wlan_operate_set_he_txop_dur_rts_thld(wdev, txop_dur_thld);
	HW_SET_PROTECT(ad, wdev, PROT_TXOP_DUR_BASE, 0, 0);
}

INT set_txop_duration_prot_threshold(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	struct wifi_dev *wdev = get_wdev_by_ioctl_idx_and_iftype(pAd, pObj->ioctl_if, pObj->ioctl_if_type);
	UINT32 txop_dur_thld = MAX_TXOP_DURATION_RTS_THRESHOLD;

	if (arg == NULL) {
		MTWF_LOG(DBG_CAT_CFG,
			 DBG_SUBCAT_ALL,
			 DBG_LVL_ERROR,
			 ("Usage:\niwpriv raN set txop_rts_thld=[txop_duration_threshold]\n"));
		return FALSE;
	}

	if (!wdev)
		return FALSE;

	txop_dur_thld = os_str_tol(arg, 0, 10);
	if (txop_dur_thld > MAX_TXOP_DURATION_RTS_THRESHOLD) {
		MTWF_LOG(DBG_CAT_CFG,
			 DBG_SUBCAT_ALL,
			 DBG_LVL_ERROR,
			 ("%s: incorrect value:%d\n", __func__, txop_dur_thld));
		return FALSE;
	}

	set_txop_dur_rts_thld(wdev, txop_dur_thld);
	MTWF_LOG(DBG_CAT_CFG,
		 DBG_SUBCAT_ALL,
		 DBG_LVL_OFF,
		 ("%s: set wdev%d txop_duration rts threshold=%d (0x%x)\n",
			__func__,
			wdev->wdev_idx,
			txop_dur_thld,
			txop_dur_thld));
	return TRUE;
}

#endif

#ifdef RANDOM_PKT_GEN
INT Set_TxCtrl_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	struct _RTMP_CHIP_DBG *chip_dbg = hc_get_chip_dbg(pAd->hdev_ctrl);

	if (chip_dbg->set_txctrl_proc)
		return chip_dbg->set_txctrl_proc(pAd->hdev_ctrl, arg);
	else
		return FALSE;
}

VOID regular_pause_umac(RTMP_ADAPTER *pAd)
{
	struct _RTMP_CHIP_DBG *chip_dbg = hc_get_chip_dbg(pAd->hdev_ctrl);

	if (chip_dbg->regular_pause_umac)
		chip_dbg->regular_pause_umac(pAd->hdev_ctrl);
}
#endif
#ifdef CSO_TEST_SUPPORT
INT32 CsCtrl;
INT Set_CsCtrl_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	if (arg == NULL)
		return FALSE;

	CsCtrl = os_str_tol(arg, 0, 10);

	if (CsCtrl & BIT0)
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("IPV4 checksum overwrite enable!\n"));

	if (CsCtrl & BIT1)
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("TCP checksum overwrite enable!\n"));

	if (CsCtrl & BIT2)
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("UDP checksum overwrite enable!\n"));

	if (CsCtrl & BIT3)
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("Tx Debug log enable!\n"));

	if (CsCtrl & BIT4)
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("Rx Debug log enable!\n"));

	return TRUE;
}
#endif

INT	Show_WifiSysInfo_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	wifi_sys_dump(pAd);
	return TRUE;
}

INT Show_DescInfo_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
#ifdef RTMP_MAC_PCI
	INT32 i, resource_idx;
	RXD_STRUC *pRxD;
	TXD_STRUC *pTxD;
#ifdef RT_BIG_ENDIAN
	RXD_STRUC *pDestRxD, RxD;
	TXD_STRUC *pDestTxD, TxD;
#endif /* RT_BIG_ENDIAN */
	PCI_HIF_T *hif = hc_get_hif_ctrl(pAd->hdev_ctrl);
	struct hif_pci_tx_ring *tx_ring;
	struct hif_pci_rx_ring *rx_ring;
	PUCHAR pDMAHeaderBufVA;
	RTMP_STRING *dir;

	if (arg != NULL && strlen(arg)) {
		dir = strsep(&arg, ":");
	} else {
		goto err;
	}

	if (arg != NULL && strlen(arg)) {
		resource_idx = os_str_tol(arg, 0, 10);
	} else {
		goto err;
	}

	if (rtstrcasecmp(dir, "tx")) {
		if (resource_idx < hif->tx_res_num) {
			UINT16 tx_ring_size;
			tx_ring = pci_get_tx_ring_by_ridx(hif, resource_idx);
			tx_ring_size = tx_ring->ring_size;
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("Tx Ring %d ---------------------------------\n", resource_idx));

			for (i = 0; i < tx_ring->ring_size; i++) {
				pDMAHeaderBufVA = (UCHAR *)tx_ring->Cell[i].DmaBuf.AllocVa;
#ifdef RT_BIG_ENDIAN
				pDestTxD = (TXD_STRUC *)tx_ring->Cell[i].AllocVa;
				TxD = *pDestTxD;
				pTxD = &TxD;
				RTMPDescriptorEndianChange((PUCHAR)pTxD, TYPE_TXD);

				if (pDMAHeaderBufVA)
					MTMacInfoEndianChange(pAd, (PUCHAR)(pDMAHeaderBufVA), TYPE_TMACINFO, 32);
				else
					MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("pkt is null\n"));

#else
				pTxD = (TXD_STRUC *)tx_ring->Cell[i].AllocVa;
#endif /* RT_BIG_ENDIAN */
				MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("Desc #%d\n", i));

				if (pTxD)
					dump_txd(pAd, pTxD);
				else
					MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("TXD null!!\n"));

				if (pDMAHeaderBufVA) {
					asic_dump_tmac_info(pAd, pDMAHeaderBufVA);
					MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("pkt physical address = %x\n",
							 (UINT32)tx_ring->Cell[i].PacketPa));
#ifdef RT_BIG_ENDIAN
					MTMacInfoEndianChange(pAd, (PUCHAR)(pDMAHeaderBufVA), TYPE_TMACINFO, 32);
#endif
				} else
					MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("pkt is null\n"));
			}
		} else {
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("Tx resource_idx %d out of range\n", resource_idx));
		}
	} else if (rtstrcasecmp(dir, "rx")) {
		if (resource_idx < hif->rx_res_num) {
			UINT16 RxRingSize;
			rx_ring = pci_get_rx_ring_by_ridx(hif, resource_idx);
			RxRingSize = rx_ring->ring_size;
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("Rx Ring %d ---------------------------------\n", resource_idx));

			for (i = 0; i < RxRingSize; i++) {
#ifdef RT_BIG_ENDIAN
				pDestRxD = (RXD_STRUC *)rx_ring->Cell[i].AllocVa;
				RxD = *pDestRxD;
				pRxD = &RxD;
				RTMPDescriptorEndianChange((PUCHAR)pRxD, TYPE_RXD);
#else
				pRxD = (RXD_STRUC *)rx_ring->Cell[i].AllocVa;
#endif /* RT_BIG_ENDIAN */
				MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("Desc #%d\n", i));

				if (pRxD) {
					dump_rxd(pAd, pRxD);
					MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("pRxD->DDONE = %x\n", pRxD->DDONE));
				} else
					MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("RXD null!!\n"));
			}
		} else {
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("Rx resource_idx %d out of range\n", resource_idx));
		}
	} else {
		goto err;
	}
	return TRUE;

err:
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("Usage:   iwpriv $(inf_name) show descinfo=$(tx/rx):$(resource_idx)\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("Example: iwpriv ra0 show descinfo=tx:0\n"));
#endif /* RTMP_MAC_PCI */
	return FALSE;
}

/*
    ==========================================================================
    Description:
	Reset statistics counter

    Arguments:
	pAd            Pointer to our adapter
	arg

    Return:
	TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_ResetStatCounter_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	struct wifi_dev *wdev = get_wdev_by_ioctl_idx_and_iftype(pAd, pObj->ioctl_if, pObj->ioctl_if_type);
	UINT8 ucBand = BAND0;

	if (wdev != NULL)
		ucBand = HcGetBandByWdev(wdev);

	if (ucBand >= DBDC_BAND_NUM)
		return FALSE;

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("==>Set_ResetStatCounter_Proc\n"));
	/* add the most up-to-date h/w raw counters into software counters*/
	NICUpdateRawCountersNew(pAd);

	NdisZeroMemory(&pAd->WlanCounters[ucBand], sizeof(COUNTER_802_11));
	NdisZeroMemory(&pAd->Counters8023, sizeof(COUNTER_802_3));
	NdisZeroMemory(&pAd->RalinkCounters, sizeof(COUNTER_RALINK));
	pAd->mcli_ctl[ucBand].last_tx_cnt = 0;
	pAd->mcli_ctl[ucBand].last_tx_fail_cnt = 0;
#ifdef RACTRL_FW_OFFLOAD_SUPPORT
	{
		/* clear TX success/fail count in MCU */
		EXT_EVENT_TX_STATISTIC_RESULT_T rTxStatResult;

		MtCmdGetTxStatistic(pAd, GET_TX_STAT_TOTAL_TX_CNT, ucBand, 0, &rTxStatResult);
	}
#endif /* RACTRL_FW_OFFLOAD_SUPPORT */
#ifdef CONFIG_ATE
	/* Clear TX success count in ATE mode */
	if (ATE_ON(pAd)) {
#if defined(CONFIG_WLAN_SERVICE)
		struct service *serv = &pAd->serv;
		struct service_test *serv_test = (struct service_test *)serv->serv_handle;

		os_zero_mem(&serv_test->test_rx_statistic[TESTMODE_GET_BAND_IDX(pAd)], sizeof(struct test_rx_stat));
#else
		struct _ATE_CTRL *ATECtrl = &pAd->ATECtrl;

		NdisZeroMemory(&ATECtrl->rx_stat, sizeof(struct _ATE_RX_STATISTIC));
#endif
		TESTMODE_SET_PARAM(pAd, TESTMODE_GET_BAND_IDX(pAd), ATE_TXDONE_CNT, 0);
	}
#ifdef CONFIG_QA
	MT_ATEUpdateRxStatistic(pAd, 2, NULL);
#endif /* CONFIG_QA */
#endif /* CONFIG_ATE */
#ifdef CONFIG_AP_SUPPORT
#endif /* CONFIG_AP_SUPPORT */
#ifdef TXBF_SUPPORT
{
	struct _RTMP_CHIP_CAP *cap = hc_get_chip_cap(pAd->hdev_ctrl);

	if (cap->FlgHwTxBfCap) {
		int i;

		for (i = 0; VALID_UCAST_ENTRY_WCID(pAd, i); i++)
			NdisZeroMemory(&pAd->MacTab.Content[i].TxBFCounters, sizeof(pAd->MacTab.Content[i].TxBFCounters));
	}
}
#endif /* TXBF_SUPPORT */
	return TRUE;
}

BOOLEAN RTMPCheckStrPrintAble(
	IN  CHAR *pInPutStr,
	IN  UCHAR strLen)
{
	UCHAR i = 0;

	for (i = 0; i < strLen; i++) {
		if ((pInPutStr[i] < 0x20) || (pInPutStr[i] > 0x7E))
			return FALSE;
	}

	return TRUE;
}

/*
	========================================================================

	Routine Description:
		Remove WPA Key process

	Arguments:
		pAd					Pointer to our adapter
		pBuf							Pointer to the where the key stored

	Return Value:
		NDIS_SUCCESS					Add key successfully

	IRQL = DISPATCH_LEVEL

	Note:

	========================================================================
*/
#ifdef CONFIG_STA_SUPPORT
VOID RTMPSetDesiredRates(RTMP_ADAPTER *pAd, struct wifi_dev *wdev, LONG Rates)
{
	NDIS_802_11_RATES aryRates;

	memset(&aryRates, 0x00, sizeof(NDIS_802_11_RATES));

	switch (wdev->PhyMode) {
	case (UCHAR)(WMODE_A): /* A only*/
		switch (Rates) {
		case 6000000: /*6M*/
			aryRates[0] = 0x0c; /* 6M*/
			wdev->DesiredTransmitSetting.field.MCS = MCS_0;
			break;

		case 9000000: /*9M*/
			aryRates[0] = 0x12; /* 9M*/
			wdev->DesiredTransmitSetting.field.MCS = MCS_1;
			break;

		case 12000000: /*12M*/
			aryRates[0] = 0x18; /* 12M*/
			wdev->DesiredTransmitSetting.field.MCS = MCS_2;
			break;

		case 18000000: /*18M*/
			aryRates[0] = 0x24; /* 18M*/
			wdev->DesiredTransmitSetting.field.MCS = MCS_3;
			break;

		case 24000000: /*24M*/
			aryRates[0] = 0x30; /* 24M*/
			wdev->DesiredTransmitSetting.field.MCS = MCS_4;
			break;

		case 36000000: /*36M*/
			aryRates[0] = 0x48; /* 36M*/
			wdev->DesiredTransmitSetting.field.MCS = MCS_5;
			break;

		case 48000000: /*48M*/
			aryRates[0] = 0x60; /* 48M*/
			wdev->DesiredTransmitSetting.field.MCS = MCS_6;
			break;

		case 54000000: /*54M*/
			aryRates[0] = 0x6c; /* 54M*/
			wdev->DesiredTransmitSetting.field.MCS = MCS_7;
			break;

		case -1: /*Auto*/
		default:
			aryRates[0] = 0x6c; /* 54Mbps*/
			aryRates[1] = 0x60; /* 48Mbps*/
			aryRates[2] = 0x48; /* 36Mbps*/
			aryRates[3] = 0x30; /* 24Mbps*/
			aryRates[4] = 0x24; /* 18M*/
			aryRates[5] = 0x18; /* 12M*/
			aryRates[6] = 0x12; /* 9M*/
			aryRates[7] = 0x0c; /* 6M*/
			wdev->DesiredTransmitSetting.field.MCS = MCS_AUTO;
			break;
		}

		break;

	case (UCHAR)(WMODE_B | WMODE_G): /* B/G Mixed*/
	case (UCHAR)(WMODE_B): /* B only*/
	case (UCHAR)(WMODE_A | WMODE_B | WMODE_G): /* A/B/G Mixed*/
	default:
		switch (Rates) {
		case 1000000: /*1M*/
			aryRates[0] = 0x02;
			wdev->DesiredTransmitSetting.field.MCS = MCS_0;
			break;

		case 2000000: /*2M*/
			aryRates[0] = 0x04;
			wdev->DesiredTransmitSetting.field.MCS = MCS_1;
			break;

		case 5000000: /*5.5M*/
			aryRates[0] = 0x0b; /* 5.5M*/
			wdev->DesiredTransmitSetting.field.MCS = MCS_2;
			break;

		case 11000000: /*11M*/
			aryRates[0] = 0x16; /* 11M*/
			wdev->DesiredTransmitSetting.field.MCS = MCS_3;
			break;

		case 6000000: /*6M*/
			aryRates[0] = 0x0c; /* 6M*/
			wdev->DesiredTransmitSetting.field.MCS = MCS_0;
			break;

		case 9000000: /*9M*/
			aryRates[0] = 0x12; /* 9M*/
			wdev->DesiredTransmitSetting.field.MCS = MCS_1;
			break;

		case 12000000: /*12M*/
			aryRates[0] = 0x18; /* 12M*/
			wdev->DesiredTransmitSetting.field.MCS = MCS_2;
			break;

		case 18000000: /*18M*/
			aryRates[0] = 0x24; /* 18M*/
			wdev->DesiredTransmitSetting.field.MCS = MCS_3;
			break;

		case 24000000: /*24M*/
			aryRates[0] = 0x30; /* 24M*/
			wdev->DesiredTransmitSetting.field.MCS = MCS_4;
			break;

		case 36000000: /*36M*/
			aryRates[0] = 0x48; /* 36M*/
			wdev->DesiredTransmitSetting.field.MCS = MCS_5;
			break;

		case 48000000: /*48M*/
			aryRates[0] = 0x60; /* 48M*/
			wdev->DesiredTransmitSetting.field.MCS = MCS_6;
			break;

		case 54000000: /*54M*/
			aryRates[0] = 0x6c; /* 54M*/
			wdev->DesiredTransmitSetting.field.MCS = MCS_7;
			break;

		case -1: /*Auto*/
		default:
			if (wdev->PhyMode == WMODE_B) {
				/*B Only*/
				aryRates[0] = 0x16; /* 11Mbps*/
				aryRates[1] = 0x0b; /* 5.5Mbps*/
				aryRates[2] = 0x04; /* 2Mbps*/
				aryRates[3] = 0x02; /* 1Mbps*/
			} else {
				/*(B/G) Mixed or (A/B/G) Mixed*/
				aryRates[0] = 0x6c; /* 54Mbps*/
				aryRates[1] = 0x60; /* 48Mbps*/
				aryRates[2] = 0x48; /* 36Mbps*/
				aryRates[3] = 0x30; /* 24Mbps*/
				aryRates[4] = 0x16; /* 11Mbps*/
				aryRates[5] = 0x0b; /* 5.5Mbps*/
				aryRates[6] = 0x04; /* 2Mbps*/
				aryRates[7] = 0x02; /* 1Mbps*/
			}

			wdev->DesiredTransmitSetting.field.MCS = MCS_AUTO;
			break;
		}

		break;
	}

	NdisZeroMemory(wdev->rate.DesireRate, MAX_LEN_OF_SUPPORTED_RATES);
	NdisMoveMemory(wdev->rate.DesireRate, &aryRates, sizeof(NDIS_802_11_RATES));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
			 (" RTMPSetDesiredRates (%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x)\n",
			  wdev->rate.DesireRate[0], wdev->rate.DesireRate[1],
			  wdev->rate.DesireRate[2], wdev->rate.DesireRate[3],
			  wdev->rate.DesireRate[4], wdev->rate.DesireRate[5],
			  wdev->rate.DesireRate[6], wdev->rate.DesireRate[7]));
	/* Changing DesiredRate may affect the MAX TX rate we used to TX frames out*/
	MlmeUpdateTxRates(pAd, FALSE, 0);
}
#endif /* CONFIG_STA_SUPPORT */

#if defined(CONFIG_STA_SUPPORT) || defined(WPA_SUPPLICANT_SUPPORT)
NDIS_STATUS RTMPWPARemoveKeyProc(
	IN	PRTMP_ADAPTER	pAd,
	IN	PVOID			pBuf)
{
	PNDIS_802_11_REMOVE_KEY pKey;
	ULONG					KeyIdx;
	NDIS_STATUS			Status = NDIS_STATUS_FAILURE;
	BOOLEAN	bTxKey;		/* Set the key as transmit key*/
	BOOLEAN	bPairwise;		/* Indicate the key is pairwise key*/
	BOOLEAN	bKeyRSC;		/* indicate the receive  SC set by KeyRSC value.*/
	/* Otherwise, it will set by the NIC.*/
	BOOLEAN	bAuthenticator; /* indicate key is set by authenticator.*/
	INT		i;
#ifdef APCLI_SUPPORT
#ifdef WPA_SUPPLICANT_SUPPORT
	UCHAR ifIndex;
	INT BssIdx;
	POS_COOKIE	pObj = (POS_COOKIE) pAd->OS_Cookie;
#endif/*WPA_SUPPLICANT_SUPPORT*/
#endif/*APCLI_SUPPORT*/
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("---> RTMPWPARemoveKeyProc\n"));
	pKey = (PNDIS_802_11_REMOVE_KEY) pBuf;
	KeyIdx = pKey->KeyIndex & 0xff;
	/* Bit 31 of Add-key, Tx Key*/
	bTxKey = (pKey->KeyIndex & 0x80000000) ? TRUE : FALSE;
	/* Bit 30 of Add-key PairwiseKey*/
	bPairwise = (pKey->KeyIndex & 0x40000000) ? TRUE : FALSE;
	/* Bit 29 of Add-key KeyRSC*/
	bKeyRSC = (pKey->KeyIndex & 0x20000000) ? TRUE : FALSE;
	/* Bit 28 of Add-key Authenticator*/
	bAuthenticator = (pKey->KeyIndex & 0x10000000) ? TRUE : FALSE;

	/* 1. If bTx is TRUE, return failure information*/
	if (bTxKey == TRUE)
		return NDIS_STATUS_INVALID_DATA;

	/* 2. Check Pairwise Key*/
	if (bPairwise) {
		/* a. If BSSID is broadcast, remove all pairwise keys.*/
		/* b. If not broadcast, remove the pairwise specified by BSSID*/
		for (i = 0; i < SHARE_KEY_NUM; i++) {
#ifdef APCLI_SUPPORT
#ifdef WPA_SUPPLICANT_SUPPORT

			if (pObj->ioctl_if_type == INT_APCLI) {
				/*if (MAC_ADDR_EQUAL(pAd->StaCfg[ifIndex].SharedKey[i].BssId, pKey->BSSID)) */
				{
					ifIndex = pObj->ioctl_if;
					BssIdx = pAd->ApCfg.BssidNum + MAX_MESH_NUM + ifIndex;
					MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("APCLI RTMPWPARemoveKeyProc(KeyIdx=%d)\n", i));
					pAd->StaCfg[ifIndex].SharedKey[i].KeyLen = 0;
					pAd->StaCfg[ifIndex].SharedKey[i].CipherAlg = CIPHER_NONE;
					AsicRemoveSharedKeyEntry(pAd, BssIdx, (UCHAR)i);
					Status = NDIS_STATUS_SUCCESS;
					break;
				}
			} else
#endif/*WPA_SUPPLICANT_SUPPORT*/
#endif/*APCLI_SUPPORT*/
			{
#ifdef CONFIG_STA_SUPPORT

				if (MAC_ADDR_EQUAL(pAd->SharedKey[BSS0][i].BssId, pKey->BSSID)) {
					MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("RTMPWPARemoveKeyProc(KeyIdx=%d)\n", i));
					pAd->SharedKey[BSS0][i].KeyLen = 0;
					pAd->SharedKey[BSS0][i].CipherAlg = CIPHER_NONE;
					AsicRemoveSharedKeyEntry(pAd, BSS0, (UCHAR)i);
					Status = NDIS_STATUS_SUCCESS;
					break;
				}

#endif/*CONFIG_STA_SUPPORT*/
			}
		}
	}
	/* 3. Group Key*/
	else {
		/* a. If BSSID is broadcast, remove all group keys indexed*/
		/* b. If BSSID matched, delete the group key indexed.*/
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("RTMPWPARemoveKeyProc(KeyIdx=%ld)\n", KeyIdx));
		pAd->SharedKey[BSS0][KeyIdx].KeyLen = 0;
		pAd->SharedKey[BSS0][KeyIdx].CipherAlg = CIPHER_NONE;
		AsicRemoveSharedKeyEntry(pAd, BSS0, (UCHAR)KeyIdx);
		Status = NDIS_STATUS_SUCCESS;
	}

	return Status;
}
#endif /* defined(CONFIG_STA_SUPPORT) || defined(WPA_SUPPLICANT_SUPPORT) */

#ifdef CONFIG_STA_SUPPORT
/*
	========================================================================

	Routine Description:
		Remove All WPA Keys

	Arguments:
		pAd					Pointer to our adapter

	Return Value:
		None

	IRQL = DISPATCH_LEVEL

	Note:

	========================================================================
*/
VOID RTMPWPARemoveAllKeys(RTMP_ADAPTER *pAd, struct wifi_dev *wdev)
{
	ASIC_SEC_INFO Info = {0};
	MAC_TABLE_ENTRY *pEntry = NULL;

	pEntry = GetAssociatedAPByWdev(pAd, wdev);

	if (!pEntry)
		return;

	/* Set key material to Asic */
	os_zero_mem(&Info, sizeof(ASIC_SEC_INFO));
	Info.Operation = SEC_ASIC_REMOVE_PAIRWISE_KEY;
	Info.Wcid = pEntry->wcid;
	HW_ADDREMOVE_KEYTABLE(pAd, &Info);
}
#endif /* CONFIG_STA_SUPPORT */

/*
	========================================================================
	Routine Description:
		Change NIC PHY mode. Re-association may be necessary

	Arguments:
		pAd - Pointer to our adapter
		phymode  -

	IRQL = PASSIVE_LEVEL
	IRQL = DISPATCH_LEVEL

	========================================================================
*/
VOID RTMPSetPhyMode(RTMP_ADAPTER *pAd, struct wifi_dev *wdev, USHORT phymode)
{
	INT i;
	UCHAR RfIC = wmode_2_rfic(phymode);
	UCHAR Channel = (wdev->channel) ? wdev->channel : HcGetChannelByRf(pAd, RfIC);
	UCHAR BandIdx = HcGetBandByWdev(wdev);
	CHANNEL_CTRL *pChCtrl = hc_get_channel_ctrl(pAd->hdev_ctrl, BandIdx);


	/* sanity check user setting*/
	for (i = 0; i < pChCtrl->ChListNum; i++) {
		if (Channel == pChCtrl->ChList[i].Channel)
			break;
	}

	if (i == pChCtrl->ChListNum) {
#ifdef CONFIG_AP_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_AP(pAd)

		if (Channel != 0)
			Channel = FirstChannel(pAd, wdev);

#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
		Channel = FirstChannel(pAd, wdev);
#endif /* CONFIG_STA_SUPPORT */
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s(): channel out of range, use first ch=%d\n",
				 __func__, Channel));
		wdev->channel = Channel;
		wlan_operate_set_prim_ch(wdev, wdev->channel);
	}

	MlmeUpdateTxRatesWdev(pAd, FALSE, wdev);
	/* CFG_TODO */
#ifdef RT_CFG80211_P2P_SUPPORT
	NdisZeroMemory(pAd->cfg80211_ctrl.P2pSupRate, MAX_LEN_OF_SUPPORTED_RATES);
	NdisZeroMemory(pAd->cfg80211_ctrl.P2pExtRate, MAX_LEN_OF_SUPPORTED_RATES);
	pAd->cfg80211_ctrl.P2pSupRate[0]  = 0x8C;        /* 6 mbps, in units of 0.5 Mbps, basic rate*/
	pAd->cfg80211_ctrl.P2pSupRate[1]  = 0x12;        /* 9 mbps, in units of 0.5 Mbps*/
	pAd->cfg80211_ctrl.P2pSupRate[2]  = 0x98;        /* 12 mbps, in units of 0.5 Mbps, basic rate*/
	pAd->cfg80211_ctrl.P2pSupRate[3]  = 0x24;        /* 18 mbps, in units of 0.5 Mbps*/
	pAd->cfg80211_ctrl.P2pSupRate[4]  = 0xb0;        /* 24 mbps, in units of 0.5 Mbps, basic rate*/
	pAd->cfg80211_ctrl.P2pSupRate[5]  = 0x48;        /* 36 mbps, in units of 0.5 Mbps*/
	pAd->cfg80211_ctrl.P2pSupRate[6]  = 0x60;        /* 48 mbps, in units of 0.5 Mbps*/
	pAd->cfg80211_ctrl.P2pSupRate[7]  = 0x6c;        /* 54 mbps, in units of 0.5 Mbps*/
	pAd->cfg80211_ctrl.P2pSupRateLen  = 8;
	pAd->cfg80211_ctrl.P2pExtRateLen  = 0;
#ifdef RT_CFG80211_P2P_CONCURRENT_DEVICE
	printk("%s: Update for AP\n", __func__);
	MlmeUpdateTxRates(pAd, FALSE, MAIN_MBSSID + MIN_NET_DEVICE_FOR_CFG80211_VIF_P2P_GO);
	printk("%s: Update for APCLI\n", __func__);
	MlmeUpdateTxRates(pAd, FALSE, MAIN_MBSSID + MIN_NET_DEVICE_FOR_APCLI);
#endif /* RT_CFG80211_P2P_CONCURRENT_DEVICE */
#endif /* RT_CFG80211_P2P_SUPPORT */
#ifdef DOT11_N_SUPPORT
	SetCommonHtVht(pAd, wdev);
#endif /* DOT11_N_SUPPORT */
}

VOID RTMPUpdateRateInfo(
	USHORT phymode,
	struct dev_rate_info *rate
)
{
	struct legacy_rate *legacy_rate = &rate->legacy_rate;

	NdisZeroMemory(legacy_rate->sup_rate, MAX_LEN_OF_SUPPORTED_RATES);
	NdisZeroMemory(legacy_rate->ext_rate, MAX_LEN_OF_SUPPORTED_RATES);
	NdisZeroMemory(rate->DesireRate, MAX_LEN_OF_SUPPORTED_RATES);

	switch (phymode) {
	case (WMODE_B):
		legacy_rate->sup_rate[0]  = 0x82;	  /* 1 mbps, in units of 0.5 Mbps, basic rate */
		legacy_rate->sup_rate[1]  = 0x84;	  /* 2 mbps, in units of 0.5 Mbps, basic rate */
		legacy_rate->sup_rate[2]  = 0x8B;	  /* 5.5 mbps, in units of 0.5 Mbps, basic rate */
		legacy_rate->sup_rate[3]  = 0x96;	  /* 11 mbps, in units of 0.5 Mbps, basic rate */
		legacy_rate->sup_rate_len = 4;
		legacy_rate->ext_rate_len = 0;
		rate->DesireRate[0]  = 2;	   /* 1 mbps, in units of 0.5 Mbps*/
		rate->DesireRate[1]  = 4;	   /* 2 mbps, in units of 0.5 Mbps*/
		rate->DesireRate[2]  = 11;    /* 5.5 mbps, in units of 0.5 Mbps*/
		rate->DesireRate[3]  = 22;    /* 11 mbps, in units of 0.5 Mbps*/
		/*pAd->CommonCfg.HTPhyMode.field.MODE = MODE_CCK;  This MODE is only FYI. not use*/
		/*update MlmeTransmit rate*/
		rate->MlmeTransmit.field.MCS = MCS_0;
		rate->MlmeTransmit.field.BW = BW_20;
		rate->MlmeTransmit.field.MODE = MODE_CCK;
		break;

	/*
		In current design, we will put supported/extended rate element in
		beacon even we are 11n-only mode.
		Or some 11n stations will not connect to us if we do not put
		supported/extended rate element in beacon.
	*/
	case (WMODE_G):
	case (WMODE_B | WMODE_G):
	case (WMODE_A | WMODE_B | WMODE_G):
#ifdef DOT11_N_SUPPORT
	case (WMODE_GN):
	case (WMODE_A | WMODE_B | WMODE_G | WMODE_GN | WMODE_AN):
	case (WMODE_B | WMODE_G | WMODE_GN | WMODE_A | WMODE_AN | WMODE_AC):
	case (WMODE_B | WMODE_G | WMODE_GN):
	case (WMODE_G | WMODE_GN):
#endif /* DOT11_N_SUPPORT */
		legacy_rate->sup_rate[0]  = 0x82;	  /* 1 mbps, in units of 0.5 Mbps, basic rate*/
		legacy_rate->sup_rate[1]  = 0x84;	  /* 2 mbps, in units of 0.5 Mbps, basic rate*/
		legacy_rate->sup_rate[2]  = 0x8B;	  /* 5.5 mbps, in units of 0.5 Mbps, basic rate*/
		legacy_rate->sup_rate[3]  = 0x96;	  /* 11 mbps, in units of 0.5 Mbps, basic rate*/
		legacy_rate->sup_rate[4]  = 0x12;	  /* 9 mbps, in units of 0.5 Mbps*/
		legacy_rate->sup_rate[5]  = 0x24;	  /* 18 mbps, in units of 0.5 Mbps*/
		legacy_rate->sup_rate[6]  = 0x48;	  /* 36 mbps, in units of 0.5 Mbps*/
		legacy_rate->sup_rate[7]  = 0x6c;	  /* 54 mbps, in units of 0.5 Mbps*/
		legacy_rate->sup_rate_len = 8;
		legacy_rate->ext_rate[0]  = 0x0C;	  /* 6 mbps, in units of 0.5 Mbps*/
		legacy_rate->ext_rate[1]  = 0x18;	  /* 12 mbps, in units of 0.5 Mbps*/
		legacy_rate->ext_rate[2]  = 0x30;	  /* 24 mbps, in units of 0.5 Mbps*/
		legacy_rate->ext_rate[3]  = 0x60;	  /* 48 mbps, in units of 0.5 Mbps*/
		legacy_rate->ext_rate_len = 4;
		rate->DesireRate[0]  = 2;	   /* 1 mbps, in units of 0.5 Mbps*/
		rate->DesireRate[1]  = 4;	   /* 2 mbps, in units of 0.5 Mbps*/
		rate->DesireRate[2]  = 11;    /* 5.5 mbps, in units of 0.5 Mbps*/
		rate->DesireRate[3]  = 22;    /* 11 mbps, in units of 0.5 Mbps*/
		rate->DesireRate[4]  = 12;    /* 6 mbps, in units of 0.5 Mbps*/
		rate->DesireRate[5]  = 18;    /* 9 mbps, in units of 0.5 Mbps*/
		rate->DesireRate[6]  = 24;    /* 12 mbps, in units of 0.5 Mbps*/
		rate->DesireRate[7]  = 36;    /* 18 mbps, in units of 0.5 Mbps*/
		rate->DesireRate[8]  = 48;    /* 24 mbps, in units of 0.5 Mbps*/
		rate->DesireRate[9]  = 72;    /* 36 mbps, in units of 0.5 Mbps*/
		rate->DesireRate[10] = 96;    /* 48 mbps, in units of 0.5 Mbps*/
		rate->DesireRate[11] = 108;   /* 54 mbps, in units of 0.5 Mbps*/
		/*update MlmeTransmit rate*/
		rate->MlmeTransmit.field.MCS = MCS_0;
		rate->MlmeTransmit.field.BW = BW_20;
		rate->MlmeTransmit.field.MODE = MODE_CCK;
		break;

	case (WMODE_A):
#ifdef DOT11_N_SUPPORT
	case (WMODE_A | WMODE_AN):
	case (WMODE_A | WMODE_G | WMODE_GN | WMODE_AN):
	case (WMODE_AN):
#endif /* DOT11_N_SUPPORT */
#ifdef DOT11_VHT_AC
	case (WMODE_A | WMODE_AN | WMODE_AC):
	case (WMODE_AN | WMODE_AC):
#endif /* DOT11_VHT_AC */
#ifdef DOT11_HE_AX
	case (WMODE_A | WMODE_AN | WMODE_AC | WMODE_AX_5G):
#endif
		legacy_rate->sup_rate[0]  = 0x8C;	  /* 6 mbps, in units of 0.5 Mbps, basic rate*/
		legacy_rate->sup_rate[1]  = 0x12;	  /* 9 mbps, in units of 0.5 Mbps*/
		legacy_rate->sup_rate[2]  = 0x98;	  /* 12 mbps, in units of 0.5 Mbps, basic rate*/
		legacy_rate->sup_rate[3]  = 0x24;	  /* 18 mbps, in units of 0.5 Mbps*/
		legacy_rate->sup_rate[4]  = 0xb0;	  /* 24 mbps, in units of 0.5 Mbps, basic rate*/
		legacy_rate->sup_rate[5]  = 0x48;	  /* 36 mbps, in units of 0.5 Mbps*/
		legacy_rate->sup_rate[6]  = 0x60;	  /* 48 mbps, in units of 0.5 Mbps*/
		legacy_rate->sup_rate[7]  = 0x6c;	  /* 54 mbps, in units of 0.5 Mbps*/
		legacy_rate->sup_rate_len = 8;
		legacy_rate->ext_rate_len = 0;
		rate->DesireRate[0]  = 12;    /* 6 mbps, in units of 0.5 Mbps*/
		rate->DesireRate[1]  = 18;    /* 9 mbps, in units of 0.5 Mbps*/
		rate->DesireRate[2]  = 24;    /* 12 mbps, in units of 0.5 Mbps*/
		rate->DesireRate[3]  = 36;    /* 18 mbps, in units of 0.5 Mbps*/
		rate->DesireRate[4]  = 48;    /* 24 mbps, in units of 0.5 Mbps*/
		rate->DesireRate[5]  = 72;    /* 36 mbps, in units of 0.5 Mbps*/
		rate->DesireRate[6]  = 96;    /* 48 mbps, in units of 0.5 Mbps*/
		rate->DesireRate[7]  = 108;   /* 54 mbps, in units of 0.5 Mbps*/
		/*pAd->CommonCfg.HTPhyMode.field.MODE = MODE_OFDM;  This MODE is only FYI. not use*/
		/*update MlmeTransmit rate*/
		rate->MlmeTransmit.field.MCS = MCS_RATE_6;
		rate->MlmeTransmit.field.BW = BW_20;
		rate->MlmeTransmit.field.MODE = MODE_OFDM;
		break;
#ifdef DOT11_HE_AX
	case (WMODE_B | WMODE_G | WMODE_GN | WMODE_AX_24G):
		legacy_rate->sup_rate[0]  = 0x82;	  /* 1 mbps, in units of 0.5 Mbps, basic rate*/
		legacy_rate->sup_rate[1]  = 0x84;	  /* 2 mbps, in units of 0.5 Mbps, basic rate*/
		legacy_rate->sup_rate[2]  = 0x8B;	  /* 5.5 mbps, in units of 0.5 Mbps, basic rate*/
		legacy_rate->sup_rate[3]  = 0x96;	  /* 11 mbps, in units of 0.5 Mbps, basic rate*/
		legacy_rate->sup_rate[4]  = 0x12;	  /* 9 mbps, in units of 0.5 Mbps*/
		legacy_rate->sup_rate[5]  = 0x24;	  /* 18 mbps, in units of 0.5 Mbps*/
		legacy_rate->sup_rate[6]  = 0x48;	  /* 36 mbps, in units of 0.5 Mbps*/
		legacy_rate->sup_rate[7]  = 0x6c;	  /* 54 mbps, in units of 0.5 Mbps*/
		legacy_rate->sup_rate_len = 8;
		legacy_rate->ext_rate[0]  = 0x0C;	  /* 6 mbps, in units of 0.5 Mbps*/
		legacy_rate->ext_rate[1]  = 0x18;	  /* 12 mbps, in units of 0.5 Mbps*/
		legacy_rate->ext_rate[2]  = 0x30;	  /* 24 mbps, in units of 0.5 Mbps*/
		legacy_rate->ext_rate[3]  = 0x60;	  /* 48 mbps, in units of 0.5 Mbps*/
		legacy_rate->ext_rate_len = 4;
		rate->DesireRate[0]  = 2;	   /* 1 mbps, in units of 0.5 Mbps*/
		rate->DesireRate[1]  = 4;	   /* 2 mbps, in units of 0.5 Mbps*/
		rate->DesireRate[2]  = 11;    /* 5.5 mbps, in units of 0.5 Mbps*/
		rate->DesireRate[3]  = 22;    /* 11 mbps, in units of 0.5 Mbps*/
		rate->DesireRate[4]  = 12;    /* 6 mbps, in units of 0.5 Mbps*/
		rate->DesireRate[5]  = 18;    /* 9 mbps, in units of 0.5 Mbps*/
		rate->DesireRate[6]  = 24;    /* 12 mbps, in units of 0.5 Mbps*/
		rate->DesireRate[7]  = 36;    /* 18 mbps, in units of 0.5 Mbps*/
		rate->DesireRate[8]  = 48;    /* 24 mbps, in units of 0.5 Mbps*/
		rate->DesireRate[9]  = 72;    /* 36 mbps, in units of 0.5 Mbps*/
		rate->DesireRate[10] = 96;    /* 48 mbps, in units of 0.5 Mbps*/
		rate->DesireRate[11] = 108;   /* 54 mbps, in units of 0.5 Mbps*/
		/*update MlmeTransmit rate*/
		rate->MlmeTransmit.field.MCS = MCS_0;
		rate->MlmeTransmit.field.BW = BW_20;
		rate->MlmeTransmit.field.MODE = MODE_CCK;
		break;
#endif

	default:
		break;
	}
}
#ifdef CONFIG_RA_PHY_RATE_SUPPORT
VOID rtmpeapupdaterateinfo(
	USHORT phymode,
	struct dev_rate_info *rate,
	struct dev_eap_info *eap
)
{
	INT i;
	struct legacy_rate *eap_legacy_rate = &eap->eap_legacy_rate;
	struct legacy_rate *legacy_rate = &rate->legacy_rate;

	if (eap->eap_suprate_en != TRUE)
		return;

	NdisZeroMemory(legacy_rate->sup_rate, MAX_LEN_OF_SUPPORTED_RATES);
	NdisZeroMemory(legacy_rate->ext_rate, MAX_LEN_OF_SUPPORTED_RATES);

	switch (phymode) {
	case (WMODE_B):
		for (i = 0; i < eap_legacy_rate->sup_rate_len; i++)
			legacy_rate->sup_rate[i] = eap_legacy_rate->sup_rate[i] | 0x80;

		legacy_rate->sup_rate_len = eap_legacy_rate->sup_rate_len;
		break;
	case (WMODE_G):
	case (WMODE_B | WMODE_G):
	case (WMODE_A | WMODE_B | WMODE_G):
#ifdef DOT11_N_SUPPORT
	case (WMODE_GN):
	case (WMODE_A | WMODE_B | WMODE_G | WMODE_GN | WMODE_AN):
	case (WMODE_B | WMODE_G | WMODE_GN | WMODE_A | WMODE_AN | WMODE_AC):
	case (WMODE_B | WMODE_G | WMODE_GN):
	case (WMODE_G | WMODE_GN):
#endif /* DOT11_N_SUPPORT */
		for (i = 0; i < eap_legacy_rate->sup_rate_len; i++)
			legacy_rate->sup_rate[i] = eap_legacy_rate->sup_rate[i];

		legacy_rate->sup_rate_len = eap_legacy_rate->sup_rate_len;

		for (i = 0; i < eap_legacy_rate->ext_rate_len; i++)
			legacy_rate->ext_rate[i] = eap_legacy_rate->ext_rate[i];

		legacy_rate->ext_rate_len = eap_legacy_rate->ext_rate_len;

		break;

	case (WMODE_A):
#ifdef DOT11_N_SUPPORT
	case (WMODE_A | WMODE_AN):
	case (WMODE_A | WMODE_G | WMODE_GN | WMODE_AN):
	case (WMODE_AN):
#endif /* DOT11_N_SUPPORT */
#ifdef DOT11_VHT_AC
	case (WMODE_A | WMODE_AN | WMODE_AC):
	case (WMODE_AN | WMODE_AC):
#endif /* DOT11_VHT_AC */
#ifdef DOT11_HE_AX
	case (WMODE_A | WMODE_AN | WMODE_AC | WMODE_AX_5G):
#endif
		for (i = 0; i < eap_legacy_rate->sup_rate_len; i++)
			legacy_rate->sup_rate[i] = eap_legacy_rate->sup_rate[i];

		legacy_rate->sup_rate_len = eap_legacy_rate->sup_rate_len;
		legacy_rate->ext_rate_len = 0;
		break;
#ifdef DOT11_HE_AX
	case (WMODE_B | WMODE_G | WMODE_GN | WMODE_AX_24G):
		for (i = 0; i < eap_legacy_rate->sup_rate_len; i++)
			legacy_rate->sup_rate[i] = eap_legacy_rate->sup_rate[i];

		legacy_rate->sup_rate_len = eap_legacy_rate->sup_rate_len;

		for (i = 0; i < eap_legacy_rate->ext_rate_len; i++)
			legacy_rate->ext_rate[i] = eap_legacy_rate->ext_rate[i];

		legacy_rate->ext_rate_len = eap_legacy_rate->ext_rate_len;

		break;
#endif

	default:
		break;
	}

}
#endif /* CONFIG_RA_PHY_RATE_SUPPORT */


#ifdef GN_MIXMODE_SUPPORT
VOID RTMPUpdateGNRateInfo(
	USHORT phymode,
	struct dev_rate_info *rate
	)
{
	struct legacy_rate *legacy_rate = &rate->legacy_rate;

	NdisZeroMemory(legacy_rate->sup_rate, MAX_LEN_OF_SUPPORTED_RATES);
	NdisZeroMemory(legacy_rate->ext_rate, MAX_LEN_OF_SUPPORTED_RATES);
	NdisZeroMemory(rate->DesireRate, MAX_LEN_OF_SUPPORTED_RATES);

	legacy_rate->sup_rate[0]  = 0x8C;	  /* 6 mbps, in units of 0.5 Mbps, basic rate*/
	legacy_rate->sup_rate[1]  = 0x12;	  /* 9 mbps, in units of 0.5 Mbps*/
	legacy_rate->sup_rate[2]  = 0x98;	  /* 12 mbps, in units of 0.5 Mbps, basic rate*/
	legacy_rate->sup_rate[3]  = 0x24;	  /* 18 mbps, in units of 0.5 Mbps*/
	legacy_rate->sup_rate[4]  = 0xb0;	  /* 24 mbps, in units of 0.5 Mbps, basic rate*/
	legacy_rate->sup_rate[5]  = 0x48;	  /* 36 mbps, in units of 0.5 Mbps*/
	legacy_rate->sup_rate[6]  = 0x60;	  /* 48 mbps, in units of 0.5 Mbps*/
	legacy_rate->sup_rate[7]  = 0x6c;	  /* 54 mbps, in units of 0.5 Mbps*/
	legacy_rate->sup_rate_len = 8;
	legacy_rate->ext_rate_len = 0;

	rate->DesireRate[0]  = 12;    /* 6 mbps, in units of 0.5 Mbps*/
	rate->DesireRate[1]  = 18;    /* 9 mbps, in units of 0.5 Mbps*/
	rate->DesireRate[2]  = 24;    /* 12 mbps, in units of 0.5 Mbps*/
	rate->DesireRate[3]  = 36;    /* 18 mbps, in units of 0.5 Mbps*/
	rate->DesireRate[4]  = 48;    /* 24 mbps, in units of 0.5 Mbps*/
	rate->DesireRate[5]  = 72;    /* 36 mbps, in units of 0.5 Mbps*/
	rate->DesireRate[6]  = 96;    /* 48 mbps, in units of 0.5 Mbps*/
	rate->DesireRate[7]  = 108;   /* 54 mbps, in units of 0.5 Mbps*/

	/*update MlmeTransmit rate*/
	rate->MlmeTransmit.field.MCS = MCS_RATE_6;
	rate->MlmeTransmit.field.BW = BW_20;
	rate->MlmeTransmit.field.MODE = MODE_OFDM;

}
#endif /* GN_MIXMODE_SUPPORT */

/*
	========================================================================
	Description:
		Add Client security information into ASIC WCID table and IVEIV table.
    Return:
	========================================================================
*/
VOID RTMPAddWcidAttributeEntry(
	IN RTMP_ADAPTER *pAd,
	IN UCHAR BssIdx,
	IN UCHAR KeyIdx,
	IN UCHAR CipherAlg,
	IN MAC_TABLE_ENTRY *pEntry)
{
	UINT32		WCIDAttri = 0;
	USHORT		offset;
	UCHAR		IVEIV = 0;
	USHORT		Wcid = 0;
#ifdef CONFIG_AP_SUPPORT
	BOOLEAN		IEEE8021X = FALSE;
	struct wifi_dev *wdev = NULL;
#endif /* CONFIG_AP_SUPPORT */

	/* TODO: shiang-7603!! fix me */
	if (IS_MT7603(pAd) || IS_MT7628(pAd) || IS_MT76x6(pAd) || IS_MT7637(pAd)) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s(): MT7603 Not support yet!\n", __func__));
		return;
	}

	{
#ifdef CONFIG_AP_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_AP(pAd) {
#ifdef APCLI_SUPPORT

			if (BssIdx >= MIN_NET_DEVICE_FOR_APCLI) {
				if (pEntry)
					BssIdx -= MIN_NET_DEVICE_FOR_APCLI;
				else {
					MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_WARN,
							 ("RTMPAddWcidAttributeEntry: AP-Client link doesn't need to set Group WCID Attribute.\n"));
					return;
				}
			} else
#endif /* APCLI_SUPPORT */
#ifdef WDS_SUPPORT
				if (BssIdx >= MIN_NET_DEVICE_FOR_WDS) {
					if (pEntry)
						BssIdx = BSS0;
					else {
						MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_WARN,
								 ("RTMPAddWcidAttributeEntry: WDS link doesn't need to set Group WCID Attribute.\n"));
						return;
					}
				} else
#endif /* WDS_SUPPORT */
				{
					if (BssIdx >= pAd->ApCfg.BssidNum) {
						MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
								 ("RTMPAddWcidAttributeEntry: The BSS-index(%d) is out of range for MBSSID link.\n", BssIdx));
						return;
					}
				}

			/* choose wcid number*/
			if (pEntry)
				Wcid = pEntry->wcid;
			else {
				wdev = &pAd->ApCfg.MBSSID[BssIdx].wdev;
				GET_GroupKey_WCID(wdev, Wcid);
			}

#ifdef DOT1X_SUPPORT

			if ((BssIdx < pAd->ApCfg.BssidNum) && VALID_MBSS(pAd, BssIdx)) {
				if (!pEntry) {
					MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("RTMPAddWcidAttributeEntry: pEntry is Null\n"));
					return;
				}

				IEEE8021X = pEntry->SecConfig.IEEE8021X;
			}

#endif /* DOT1X_SUPPORT */
		}
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_STA(pAd) {
			if (BssIdx > BSS0) {
				MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
						 ("RTMPAddWcidAttributeEntry: The BSS-index(%d) is out of range for Infra link.\n", BssIdx));
				return;
			}

			/*
				1.	In ADHOC mode, the AID is wcid number. And NO mesh link exists.
				2.	In Infra mode, the AID:1 MUST be wcid of infra STA.
									the AID:2~ assign to mesh link entry.
			*/

			if (pEntry)
				Wcid = pEntry->wcid;
			else
				Wcid = MCAST_WCID_TO_REMOVE;
		}
#endif /* CONFIG_STA_SUPPORT */
	}

	/* Update WCID attribute table*/
	{
		UINT32 wcid_attr_base = 0, wcid_attr_size = 0;

		offset = wcid_attr_base + (Wcid * wcid_attr_size);
#ifdef CONFIG_AP_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_AP(pAd) {
			/*
				1.	Wds-links and Mesh-links always use Pair-wise key table.
				2.	When the CipherAlg is TKIP, AES or the dynamic WEP is enabled,
					it needs to set key into Pair-wise Key Table.
				3.	The pair-wise key security mode is set NONE, it means as no security.
			*/
			if (pEntry && (IS_ENTRY_WDS(pEntry) || IS_ENTRY_MESH(pEntry)))
				WCIDAttri = (BssIdx << 4) | (CipherAlg << 1) | PAIRWISEKEYTABLE;
			else if ((pEntry) &&
					 ((CipherAlg == CIPHER_TKIP) ||
					  (CipherAlg == CIPHER_AES) ||
					  (CipherAlg == CIPHER_NONE) ||
					  (IEEE8021X == TRUE)))
				WCIDAttri = (BssIdx << 4) | (CipherAlg << 1) | PAIRWISEKEYTABLE;
			else
				WCIDAttri = (BssIdx << 4) | (CipherAlg << 1) | SHAREDKEYTABLE;
		}
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_STA(pAd) {
			if (pEntry && IS_ENTRY_MESH(pEntry))
				WCIDAttri = (CipherAlg << 1) | PAIRWISEKEYTABLE;

#if defined(DOT11Z_TDLS_SUPPORT) || defined(CFG_TDLS_SUPPORT)
			else if (((pEntry) || IS_ENTRY_TDLS(pEntry)) &&
					 ((CipherAlg == CIPHER_TKIP) ||
					  (CipherAlg == CIPHER_AES) ||
					  (CipherAlg == CIPHER_NONE)))
				WCIDAttri = (CipherAlg << 1) | PAIRWISEKEYTABLE;

#endif /* defined(DOT11Z_TDLS_SUPPORT) */
			else
				WCIDAttri = (CipherAlg << 1) | SHAREDKEYTABLE;
		}
#endif /* CONFIG_STA_SUPPORT */
		RTMP_IO_WRITE32(pAd->hdev_ctrl, offset, WCIDAttri);
	}
	/* Update IV/EIV table*/
	{
		UINT32 iveiv_tb_base = 0, iveiv_tb_size = 0;

		offset = iveiv_tb_base + (Wcid * iveiv_tb_size);

		/* WPA mode*/
		if ((CipherAlg == CIPHER_TKIP) || (CipherAlg == CIPHER_AES)) {
			/* Eiv bit on. keyid always is 0 for pairwise key */
			IVEIV = (KeyIdx << 6) | 0x20;
		} else {
			/* WEP KeyIdx is default tx key. */
			IVEIV = (KeyIdx << 6);
		}

	}
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("RTMPAddWcidAttributeEntry: WCID #%d, KeyIndex #%d, Alg=%s\n",
			 Wcid, KeyIdx, CipherName[CipherAlg]));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("	WCIDAttri = 0x%x\n",  WCIDAttri));
}

/* WTBL Test */
INT set_assign_wcid_proc(PRTMP_ADAPTER pAd, RTMP_STRING *arg)
{
	UINT16 assignWcid;

	assignWcid = os_str_tol(arg, 0, 10);
	pAd->assignWcid = assignWcid;

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s: assignWcid = %d\n", __func__, assignWcid));
	return TRUE;
}

/*
    ==========================================================================
    Description:
	Parse encryption type
Arguments:
    pAdapter                    Pointer to our adapter
    wrq                         Pointer to the ioctl argument

    Return Value:
	None

    Note:
    ==========================================================================
*/
RTMP_STRING *GetEncryptType(CHAR enc)
{
	if (enc == Ndis802_11WEPDisabled)
		return "NONE";

	if (enc == Ndis802_11WEPEnabled)
		return "WEP";

	if (enc == Ndis802_11TKIPEnable)
		return "TKIP";

	if (enc == Ndis802_11AESEnable)
		return "AES";

	if (enc == Ndis802_11TKIPAESMix)
		return "TKIPAES";

	else
		return "UNKNOW";
}

RTMP_STRING *GetAuthMode(CHAR auth)
{
	if (auth == Ndis802_11AuthModeOpen)
		return "OPEN";

	if (auth == Ndis802_11AuthModeShared)
		return "SHARED";

	if (auth == Ndis802_11AuthModeAutoSwitch)
		return "AUTOWEP";

	if (auth == Ndis802_11AuthModeWPA)
		return "WPA";

	if (auth == Ndis802_11AuthModeWPAPSK)
		return "WPAPSK";

	if (auth == Ndis802_11AuthModeWPANone)
		return "WPANONE";

	if (auth == Ndis802_11AuthModeWPA2)
		return "WPA2";

	if (auth == Ndis802_11AuthModeWPA2PSK)
		return "WPA2PSK";

	if (auth == Ndis802_11AuthModeWPA1WPA2)
		return "WPA1WPA2";

	if (auth == Ndis802_11AuthModeWPA1PSKWPA2PSK)
		return "WPA1PSKWPA2PSK";

	return "UNKNOW";
}


/*
    ==========================================================================
    Description:
	Get site survey results
	Arguments:
	    pAdapter                    Pointer to our adapter
	    wrq                         Pointer to the ioctl argument

    Return Value:
	None

    Note:
	Usage:
			1.) UI needs to wait 4 seconds after issue a site survey command
			2.) iwpriv ra0 get_site_survey
			3.) UI needs to prepare at least 4096bytes to get the results
    ==========================================================================
*/
#define	LINE_LEN	(4+33+20+23+9+12+7+3)	/* Channel+SSID+Bssid+Security+Signal+WiressMode+ExtCh+NetworkType*/

#ifdef CONFIG_STA_SUPPORT
#ifdef WSC_STA_SUPPORT
#define	WPS_LINE_LEN	(4+5)	/* WPS+DPID*/
#endif /* WSC_STA_SUPPORT */
#ifdef DOT11R_FT_SUPPORT
#define DOT11R_LINE_LEN	(5+9+10)	/* MDId+FToverDS+RsrReqCap*/
#endif /* DOT11R_FT_SUPPORT */
#endif /* CONFIG_STA_SUPPORT */
VOID RTMPCommSiteSurveyData(
	IN  RTMP_STRING *msg,
	IN  BSS_ENTRY * pBss,
	IN  UINT32 MsgLen)
{
	INT         Rssi = 0;
	UINT        Rssi_Quality = 0;
	NDIS_802_11_NETWORK_TYPE    wireless_mode;
	CHAR		Ssid[MAX_LEN_OF_SSID + 1];
	RTMP_STRING SecurityStr[32] = {0};
	/*Channel*/
	sprintf(msg + strlen(msg), "%-4d", pBss->Channel);
	/*SSID*/
	NdisZeroMemory(Ssid, (MAX_LEN_OF_SSID + 1));

	if (RTMPCheckStrPrintAble((PCHAR)pBss->Ssid, pBss->SsidLen))
		NdisMoveMemory(Ssid, pBss->Ssid, pBss->SsidLen);
	else {
		INT idx = 0;

		sprintf(Ssid, "0x");

		for (idx = 0; (idx < 15) && (idx < pBss->SsidLen); idx++)
			sprintf(Ssid + 2 + (idx * 2), "%02X", (UCHAR)pBss->Ssid[idx]);
	}

	sprintf(msg + strlen(msg), "%-33s", Ssid);
	/*BSSID*/
	sprintf(msg + strlen(msg), "%02x:%02x:%02x:%02x:%02x:%02x   ",
			pBss->Bssid[0],
			pBss->Bssid[1],
			pBss->Bssid[2],
			pBss->Bssid[3],
			pBss->Bssid[4],
			pBss->Bssid[5]);
	/*Security*/
	RTMPZeroMemory(SecurityStr, 32);
	sprintf(SecurityStr, "%s/%s", GetAuthModeStr(pBss->AKMMap), GetEncryModeStr(pBss->PairwiseCipher));
	sprintf(msg + strlen(msg), "%-23s", SecurityStr);
	/* Rssi*/
	Rssi = (INT)pBss->Rssi;

	if (Rssi >= -50)
		Rssi_Quality = 100;
	else if (Rssi >= -80)    /* between -50 ~ -80dbm*/
		Rssi_Quality = (UINT)(24 + ((Rssi + 80) * 26) / 10);
	else if (Rssi >= -90)   /* between -80 ~ -90dbm*/
		Rssi_Quality = (UINT)(((Rssi + 90) * 26) / 10);
	else    /* < -84 dbm*/
		Rssi_Quality = 0;

	sprintf(msg + strlen(msg), "%-9d", Rssi_Quality);
	/* Wireless Mode*/
	wireless_mode = NetworkTypeInUseSanity(pBss);

	if (wireless_mode == Ndis802_11FH ||
		wireless_mode == Ndis802_11DS)
		sprintf(msg + strlen(msg), "%-12s", "11b");
	else if (wireless_mode == Ndis802_11OFDM5)
		sprintf(msg + strlen(msg), "%-12s", "11a");
	else if (wireless_mode == Ndis802_11OFDM5_N)
		sprintf(msg + strlen(msg), "%-12s", "11a/n");
	else if (wireless_mode == Ndis802_11OFDM5_AC)
		sprintf(msg + strlen(msg), "%-12s", "11a/n/ac");
	else if (wireless_mode == Ndis802_11OFDM24)
		sprintf(msg + strlen(msg), "%-12s", "11b/g");
	else if (wireless_mode == Ndis802_11OFDM24_N)
		sprintf(msg + strlen(msg), "%-12s", "11b/g/n");
	else if (wireless_mode == Ndis802_11OFDM24_HE)
		sprintf(msg + strlen(msg), "%-12s", "11b/g/n/ax");
	else if (wireless_mode == Ndis802_11OFDM5_HE)
		sprintf(msg + strlen(msg), "%-12s", "11a/n/ac/ax");
	else
		sprintf(msg + strlen(msg), "%-12s", "unknow");

	/* Ext Channel*/
	if (HAS_HT_OP_EXIST(pBss->ie_exists)) {
		if (pBss->AddHtInfo.AddHtInfo.ExtChanOffset == EXTCHA_ABOVE)
			sprintf(msg + strlen(msg), "%-7s", " ABOVE");
		else if (pBss->AddHtInfo.AddHtInfo.ExtChanOffset == EXTCHA_BELOW)
			sprintf(msg + strlen(msg), "%-7s", " BELOW");
		else
			sprintf(msg + strlen(msg), "%-7s", " NONE");
	} else
		sprintf(msg + strlen(msg), "%-7s", " NONE");

	/*Network Type		*/
	if (pBss->BssType == BSS_ADHOC)
		sprintf(msg + strlen(msg), "%-3s", " Ad");
	else
		sprintf(msg + strlen(msg), "%-3s", " In");

	/* SSID Length */
	//sprintf(msg + strlen(msg), " %-8d", pBss->SsidLen);

	sprintf(msg + strlen(msg), "\n");
	return;
}

static
BOOLEAN ascii2int(RTMP_STRING *in, UINT32 *out)
{
	UINT32 decimal_val, val;
	CHAR *p, asc_val;

	decimal_val = 0;
	p = (char *)in;

	while ((*p) != 0) {
		val = 0;
		asc_val = *p;

		if ((asc_val >= '0') && (asc_val <= '9'))
			val = asc_val - 48;
		else
			return FALSE;

		decimal_val = (decimal_val * 10) + val;
		p++;
	}

	*out = decimal_val;
	return TRUE;
}

#if defined(AP_SCAN_SUPPORT) || defined(CONFIG_STA_SUPPORT)
VOID RTMPIoctlGetSiteSurvey(
	IN	PRTMP_ADAPTER	pAdapter,
	IN	RTMP_IOCTL_INPUT_STRUCT	 *wrq)
{
	RTMP_STRING *msg;
	INT		i = 0;
	INT			WaitCnt;
	INT		Status = 0;
	INT         max_len = LINE_LEN;
	RTMP_STRING *this_char;
	UINT32		bss_start_idx;
	BSS_ENTRY *pBss;
	UINT32 TotalLen, BufLen = IW_SCAN_MAX_DATA;
#ifdef CONFIG_STA_SUPPORT
	STA_ADMIN_CONFIG *pApCliEntry = NULL;
#endif
	POS_COOKIE pObj = (POS_COOKIE)pAdapter->OS_Cookie;
	struct wifi_dev *wdev = get_wdev_by_ioctl_idx_and_iftype(pAdapter, pObj->ioctl_if, pObj->ioctl_if_type);
	BSS_TABLE *ScanTab = get_scan_tab_by_wdev(pAdapter, wdev);
#ifdef CONFIG_STA_SUPPORT
	if (wdev->wdev_type == WDEV_TYPE_STA) {
		pApCliEntry = &pAdapter->StaCfg[wdev->func_idx];
		pApCliEntry->apcliNeedEnable = FALSE;
	}
#endif /*CONFIG_STA_SUPPORT */
	os_alloc_mem(NULL, (UCHAR **)&this_char, wrq->u.data.length + 1);
	if (!this_char) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s: Allocate memory fail!!!\n", __func__));
		return;
	}

	if (copy_from_user(this_char, wrq->u.data.pointer, wrq->u.data.length)) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s: copy_from_user() fail!!!\n", __func__));
		os_free_mem(this_char);
		return;
	}
	this_char[wrq->u.data.length] = 0;

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s(): Before check, this_char = %s\n"
			 , __func__, this_char));

	if (ascii2int(this_char, &bss_start_idx) == FALSE)
		bss_start_idx = 0;

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s(): After check, this_char = %s, out = %d\n"
			 , __func__, this_char, bss_start_idx));
	TotalLen = sizeof(CHAR) * ((MAX_LEN_OF_BSS_TABLE) * max_len) + 100;
	BufLen = IW_SCAN_MAX_DATA;
	os_alloc_mem(NULL, (PUCHAR *)&msg, TotalLen);

	if (msg == NULL) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("RTMPIoctlGetSiteSurvey - msg memory alloc fail.\n"));
		os_free_mem(this_char);
		return;
	}

	memset(msg, 0, TotalLen);

	if (ScanTab->BssNr == 0) {
		sprintf(msg, "No BssInfo\n");
		wrq->u.data.length = strlen(msg);
		Status = copy_to_user(wrq->u.data.pointer, msg, wrq->u.data.length);
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("RTMPIoctlGetSiteSurvey - wrq->u.data.length = %d\n",
				 wrq->u.data.length));
		os_free_mem(this_char);
		os_free_mem((PUCHAR)msg);
		return;
	}

	if (bss_start_idx > (ScanTab->BssNr - 1)) {
		sprintf(msg, "BssInfo Idx(%d) is out of range(0~%d)\n",
				bss_start_idx, (ScanTab->BssNr - 1));
		wrq->u.data.length = strlen(msg);
		Status = copy_to_user(wrq->u.data.pointer, msg, wrq->u.data.length);
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("RTMPIoctlGetSiteSurvey - wrq->u.data.length = %d\n",
				 wrq->u.data.length));
		os_free_mem((PUCHAR)msg);
		os_free_mem(this_char);
		return;
	}

	sprintf(msg, "%s", "\n");

	sprintf(msg + strlen(msg), "%-4s%-33s%-20s%-23s%-9s%-12s%-7s%-3s\n",
			"Ch", "SSID", "BSSID", "Security", "Siganl(%)", "W-Mode", " ExtCH", " NT");

	WaitCnt = 0;
#ifdef CONFIG_STA_SUPPORT
	if (wdev->wdev_type == WDEV_TYPE_STA)
	pAdapter->StaCfg[pObj->ioctl_if].bSkipAutoScanConn = TRUE;
#endif /* CONFIG_STA_SUPPORT */

	while ((scan_in_run_state(pAdapter, wdev) == TRUE) && (WaitCnt++ < 200)) {
#if defined(APCLI_SUPPORT) && defined(CONFIG_STA_SUPPORT)
		if (pApCliEntry && (pApCliEntry->ApcliInfStat.Enable == TRUE)) {
			Set_ApCli_Enable_Proc(pAdapter, "0");
			pApCliEntry->apcliNeedEnable = TRUE;
		}
#endif /* defined(APCLI_SUPPORT) && defined(CONFIG_STA_SUPPORT) */
			OS_WAIT(500);
	}

	BssTableSortByRssi(ScanTab,FALSE);

	for (i = bss_start_idx; i < ScanTab->BssNr; i++) {
		pBss = &ScanTab->BssEntry[i];

		if (pBss->Channel == 0)
			break;

		if ((strlen(msg) + 100) >= BufLen)
			break;

		RTMPCommSiteSurveyData(msg, pBss, TotalLen);

	}

#ifdef CONFIG_STA_SUPPORT
	if (wdev->wdev_type == WDEV_TYPE_STA)
		pAdapter->StaCfg[pObj->ioctl_if].bSkipAutoScanConn = FALSE;
#endif /* CONFIG_STA_SUPPORT */
	wrq->u.data.length = strlen(msg);
#if defined(APCLI_SUPPORT) && defined(CONFIG_STA_SUPPORT)
	if (pApCliEntry && (pApCliEntry->apcliNeedEnable == TRUE)) {
		Set_ApCli_Enable_Proc(pAdapter, "1");
		pApCliEntry->apcliNeedEnable = FALSE;
	}
#endif /*defined(APCLI_SUPPORT) && defined(CONFIG_STA_SUPPORT)*/
	Status = copy_to_user(wrq->u.data.pointer, msg, wrq->u.data.length);
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("RTMPIoctlGetSiteSurvey - wrq->u.data.length = %d\n",
			 wrq->u.data.length));
	os_free_mem((PUCHAR)msg);
	os_free_mem(this_char);
}
#endif

USHORT RTMPGetLastTxRate(PRTMP_ADAPTER pAd, MAC_TABLE_ENTRY *pEntry)
{
	HTTRANSMIT_SETTING lastTxRate;
#ifdef RACTRL_FW_OFFLOAD_SUPPORT
	EXT_EVENT_TX_STATISTIC_RESULT_T rTxStatResult;

	MtCmdGetTxStatistic(pAd, GET_TX_STAT_ENTRY_TX_RATE, 0/*Don't Care*/, pEntry->wcid, &rTxStatResult);
	lastTxRate.field.MODE = rTxStatResult.rEntryTxRate.MODE;
	lastTxRate.field.BW = rTxStatResult.rEntryTxRate.BW;
	lastTxRate.field.ldpc = rTxStatResult.rEntryTxRate.ldpc ? 1 : 0;
	lastTxRate.field.ShortGI = rTxStatResult.rEntryTxRate.ShortGI ? 1 : 0;
	lastTxRate.field.STBC = rTxStatResult.rEntryTxRate.STBC;

	if (lastTxRate.field.MODE >= MODE_VHT)
		lastTxRate.field.MCS = (((rTxStatResult.rEntryTxRate.VhtNss - 1) & 0x3) << 4) + rTxStatResult.rEntryTxRate.MCS;
	else if (lastTxRate.field.MODE == MODE_OFDM)
		lastTxRate.field.MCS = getLegacyOFDMMCSIndex(rTxStatResult.rEntryTxRate.MCS) & 0x0000003F;
	else
		lastTxRate.field.MCS = rTxStatResult.rEntryTxRate.MCS;

#else
	lastTxRate.word = pEntry->HTPhyMode.word;
#endif /* RACTRL_FW_OFFLOAD_SUPPORT */
	return lastTxRate.word;
}

typedef struct _RT_802_11_MAC_ENTRY_PDV {
	UCHAR ApIdx;
	UCHAR Addr[MAC_ADDR_LEN];
	UCHAR Aid;
	UCHAR Psm;
	UCHAR MimoPs;
	CHAR AvgRssi0;
	CHAR AvgRssi1;
	CHAR AvgRssi2;
	UINT32 ConnectedTime;
	HTTRANSMIT_SETTING TxRate;
	UINT32 LastRxRate;
} RT_802_11_MAC_ENTRY_PDV, *PRT_802_11_MAC_ENTRY_PDV;

typedef struct _RT_802_11_MAC_TABLE_PDV {
	ULONG Num;
	RT_802_11_MAC_ENTRY_PDV Entry[MAX_NUMBER_OF_MAC];
} RT_802_11_MAC_TABLE_PDV, *PRT_802_11_MAC_TABLE_PDV;

static VOID
copy_mac_table_entry(RT_802_11_MAC_ENTRY_PDV *pDst, MAC_TABLE_ENTRY *pEntry)
{
	pDst->ApIdx = (UCHAR)pEntry->func_tb_idx;
	COPY_MAC_ADDR(pDst->Addr, &pEntry->Addr);
	pDst->Aid = (UCHAR)pEntry->Aid;
	pDst->Psm = pEntry->PsMode;

#ifdef DOT11_N_SUPPORT
	pDst->MimoPs = pEntry->MmpsMode;
#endif /* DOT11_N_SUPPORT */

	/* Fill in RSSI per entry*/
	pDst->AvgRssi0 = pEntry->RssiSample.AvgRssi[0];
	pDst->AvgRssi1 = pEntry->RssiSample.AvgRssi[1];
	pDst->AvgRssi2 = pEntry->RssiSample.AvgRssi[2];

	/* the connected time per entry*/
	pDst->ConnectedTime = pEntry->StaConnectTime;

	pDst->TxRate.word = pEntry->HTPhyMode.word;
	pDst->LastRxRate = pEntry->LastRxRate;
}

VOID RTMPIoctlGetMacTableStaInfo(
	IN PRTMP_ADAPTER pAd,
	IN RTMP_IOCTL_INPUT_STRUCT *wrq)
{
	INT i;
	RT_802_11_MAC_TABLE_PDV *pMacTab = NULL;
	PRT_802_11_MAC_ENTRY_PDV pDst;
	MAC_TABLE_ENTRY *pEntry;
	UINT16 wrq_len = wrq->u.data.length;
	POS_COOKIE pObj = (POS_COOKIE)pAd->OS_Cookie;
	struct tx_rx_ctl *tr_ctl = &pAd->tr_ctl;
	STA_TR_ENTRY *tr_entry;

	wrq->u.data.length = 0;

#ifdef APCLI_SUPPORT
	if (pObj->ioctl_if_type == INT_APCLI)
	{
		if (!tr_ctl)
			return;
		if (wrq_len < sizeof(RT_802_11_MAC_ENTRY_PDV))
			return;
		if (pObj->ioctl_if >= MAX_APCLI_NUM)
			return;
		if (pAd->StaCfg[pObj->ioctl_if].ApcliInfStat.Valid != TRUE)
			return;

		tr_entry = &tr_ctl->tr_entry[pAd->StaCfg[pObj->ioctl_if].MacTabWCID];
		pEntry = &pAd->MacTab.Content[pAd->StaCfg[pObj->ioctl_if].MacTabWCID];

		if ((pEntry->Sst == SST_ASSOC) && (tr_entry->PortSecured == WPA_802_1X_PORT_SECURED))
		{
			RT_802_11_MAC_ENTRY_PDV MacEntry;
			
			pDst = &MacEntry;
			copy_mac_table_entry(pDst, pEntry);
			
			wrq->u.data.length = sizeof(RT_802_11_MAC_ENTRY_PDV);
			copy_to_user(wrq->u.data.pointer, pDst, wrq->u.data.length);
		}
		
		return;
	}
#endif

	/* allocate memory */
	os_alloc_mem(NULL, (UCHAR **)&pMacTab, sizeof(RT_802_11_MAC_TABLE_PDV));

	if (pMacTab == NULL) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s: Allocate memory fail!!!\n", __func__));
		return;
	}

	NdisZeroMemory(pMacTab, sizeof(RT_802_11_MAC_TABLE_PDV));

	for (i = 0; VALID_UCAST_ENTRY_WCID(pAd, i); i++) {
		pEntry = &(pAd->MacTab.Content[i]);

		if (IS_ENTRY_CLIENT(pEntry) && (pEntry->Sst == SST_ASSOC)) {
			pDst = &pMacTab->Entry[pMacTab->Num];
			pDst->ApIdx = pEntry->func_tb_idx;
			COPY_MAC_ADDR(pDst->Addr, &pEntry->Addr);
			pDst->Aid = (UCHAR)pEntry->Aid;
			pDst->Psm = pEntry->PsMode;
#ifdef DOT11_N_SUPPORT
			pDst->MimoPs = pEntry->MmpsMode;
#endif /* DOT11_N_SUPPORT */
			/* Fill in RSSI per entry*/
			pDst->AvgRssi0 = pEntry->RssiSample.AvgRssi[0];
			pDst->AvgRssi1 = pEntry->RssiSample.AvgRssi[1];
			pDst->AvgRssi2 = pEntry->RssiSample.AvgRssi[2];
			/* the connected time per entry*/
			pDst->ConnectedTime = pEntry->StaConnectTime;
			pDst->TxRate.word = RTMPGetLastTxRate(pAd, pEntry);
			pDst->LastRxRate = pEntry->LastRxRate;
			pMacTab->Num += 1;
		}
	}

	wrq->u.data.length = sizeof(RT_802_11_MAC_TABLE_PDV);

	if (copy_to_user(wrq->u.data.pointer, pMacTab, wrq->u.data.length))
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s: copy_to_user() fail\n", __func__));

	if (pMacTab != NULL)
		os_free_mem(pMacTab);
}

VOID RTMPIoctlGetDriverInfo(
	IN PRTMP_ADAPTER pAd,
	IN RTMP_IOCTL_INPUT_STRUCT * wrq)
{
	RTMP_STRING *msg;
	UINT32 TotalLen = 4096;

	os_alloc_mem(NULL, (PUCHAR *)&msg, TotalLen);
	if (msg == NULL) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
				("RTMPIoctlGetDriverInfo - msg memory alloc fail.\n"));
		return;
	}

	NdisZeroMemory(msg, TotalLen);
#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
		sprintf(msg, "Driver version: %s \n", AP_DRIVER_VERSION);
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
		sprintf(msg+strlen(msg), "Driver version: %s \n", STA_DRIVER_VERSION);
#endif /* CONFIG_STA_SUPPORT */

	sprintf(msg+strlen(msg), "FW ver: 0x%x, HW ver: 0x%x, CHIP ID: 0x%x\n",
			pAd->FWVersion, pAd->HWVersion, pAd->ChipID);

	wrq->u.data.length = strlen(msg);
	if (copy_to_user(wrq->u.data.pointer, msg, wrq->u.data.length))
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s: copy_to_user() fail\n", __func__));

	os_free_mem(msg);
}


#define	MAC_LINE_LEN	(1+14+4+4+4+4+10+10+10+6+6)	/* "\n"+Addr+aid+psm+datatime+rxbyte+txbyte+current tx rate+last tx rate+"\n" */
VOID RTMPIoctlGetMacTable(
	IN PRTMP_ADAPTER pAd,
	IN RTMP_IOCTL_INPUT_STRUCT *wrq)
{
	INT i;
	/*	RT_802_11_MAC_TABLE MacTab;*/
	RT_802_11_MAC_TABLE *pMacTab = NULL;
	RT_802_11_MAC_ENTRY *pDst;
	MAC_TABLE_ENTRY *pEntry;
	struct tx_rx_ctl *tr_ctl = &pAd->tr_ctl;
	STA_TR_ENTRY *tr_entry;
	char *msg;
	/* allocate memory */
	os_alloc_mem(NULL, (UCHAR **)&pMacTab, sizeof(RT_802_11_MAC_TABLE));

	if (pMacTab == NULL) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s: Allocate memory fail!!!\n", __func__));
		return;
	}

	NdisZeroMemory(pMacTab, sizeof(RT_802_11_MAC_TABLE));
	pMacTab->Num = 0;

	for (i = 0; VALID_UCAST_ENTRY_WCID(pAd, i); i++) {
		pEntry = &(pAd->MacTab.Content[i]);
		tr_entry = &(tr_ctl->tr_entry[i]);
		if (IS_ENTRY_CLIENT(pEntry) && (pEntry->Sst == SST_ASSOC))
		{
			pDst = &pMacTab->Entry[pMacTab->Num];
			pDst->ApIdx = (UCHAR)pEntry->func_tb_idx;
			COPY_MAC_ADDR(pDst->Addr, &pEntry->Addr);
			pDst->Aid = (UCHAR)pEntry->Aid;
			pDst->Psm = pEntry->PsMode;
#ifdef DOT11_N_SUPPORT
			pDst->MimoPs = pEntry->MmpsMode;
#endif /* DOT11_N_SUPPORT */
			/* Fill in RSSI per entry*/
			pDst->AvgRssi0 = pEntry->RssiSample.AvgRssi[0];
			pDst->AvgRssi1 = pEntry->RssiSample.AvgRssi[1];
			pDst->AvgRssi2 = pEntry->RssiSample.AvgRssi[2];
			/* the connected time per entry*/
			pDst->ConnectedTime = pEntry->StaConnectTime;
			pDst->TxRate.word = RTMPGetLastTxRate(pAd, pEntry);
#ifdef RTMP_RBUS_SUPPORT

			if (IS_RBUS_INF(pAd))
				pDst->LastRxRate = pEntry->LastRxRate;

#endif /* RTMP_RBUS_SUPPORT */
			pMacTab->Num += 1;
		}
	}

	wrq->u.data.length = sizeof(RT_802_11_MAC_TABLE);

	if (copy_to_user(wrq->u.data.pointer, pMacTab, wrq->u.data.length))
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s: copy_to_user() fail\n", __func__));

	os_alloc_mem(NULL, (UCHAR **)&msg, sizeof(CHAR) * (GET_MAX_UCAST_NUM(pAd)*MAC_LINE_LEN));

	if (msg == NULL) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s():Alloc memory failed\n", __func__));
		goto LabelOK;
	}

	memset(msg, 0, GET_MAX_UCAST_NUM(pAd)*MAC_LINE_LEN);
	sprintf(msg, "%s", "\n");
	sprintf(msg + strlen(msg), "%-14s%-4s%-4s%-4s%-4s%-6s%-6s%-10s%-10s%-10s\n",
			"MAC", "AP",  "AID", "PSM", "AUTH", "CTxR", "LTxR", "LDT", "RxB", "TxB");

	for (i = 0; VALID_UCAST_ENTRY_WCID(pAd, i); i++) {
		MAC_TABLE_ENTRY *pEntry = &pAd->MacTab.Content[i];
		if (IS_ENTRY_CLIENT(pEntry) && (pEntry->Sst == SST_ASSOC))
		{
			if ((strlen(msg) + MAC_LINE_LEN) >= (GET_MAX_UCAST_NUM(pAd)*MAC_LINE_LEN))
				break;

			sprintf(msg + strlen(msg), "%02x%02x%02x%02x%02x%02x  ", PRINT_MAC(pEntry->Addr));
			sprintf(msg + strlen(msg), "%-4d", (int)pEntry->func_tb_idx);
			sprintf(msg + strlen(msg), "%-4d", (int)pEntry->Aid);
			sprintf(msg + strlen(msg), "%-4d", (int)pEntry->PsMode);
			sprintf(msg + strlen(msg), "%-4d", (int)pEntry->AuthState);
			sprintf(msg + strlen(msg), "%-6d", RateIdToMbps[pAd->MacTab.Content[i].CurrTxRate]);
			sprintf(msg + strlen(msg), "%-6d", 0/*RateIdToMbps[pAd->MacTab.Content[i].HTPhyMode.word]*/); /* ToDo*/
			sprintf(msg + strlen(msg), "%-10d", 0/*pAd->MacTab.Content[i].HSCounter.LastDataPacketTime*/); /* ToDo*/
			sprintf(msg + strlen(msg), "%-10d", 0/*pAd->MacTab.Content[i].HSCounter.TotalRxByteCount*/); /* ToDo*/
			sprintf(msg + strlen(msg), "%-10d\n", 0/*pAd->MacTab.Content[i].HSCounter.TotalTxByteCount*/); /* ToDo*/
		}
	}

	/* for compatible with old API just do the printk to console*/
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s", msg));
	os_free_mem(msg);
LabelOK:

	if (pMacTab != NULL)
		os_free_mem(pMacTab);
}

#if defined(INF_AR9) || defined(BB_SOC)
#if defined(AR9_MAPI_SUPPORT) || defined(BB_SOC)
#ifdef CONFIG_AP_SUPPORT
VOID RTMPAR9IoctlGetMacTable(
	IN PRTMP_ADAPTER pAd,
	IN RTMP_IOCTL_INPUT_STRUCT *wrq)
{
	INT i;
	char *msg;

	os_alloc_mem(NULL, (UCHAR **)&msg, sizeof(CHAR) * (GET_MAX_UCAST_NUM(pAd)*MAC_LINE_LEN));

	if (msg == NULL) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s():Alloc memory failed\n", __func__));
		return;
	}

	memset(msg, 0, GET_MAX_UCAST_NUM(pAd) * MAC_LINE_LEN);
	sprintf(msg, "%s", "\n");
	sprintf(msg + strlen(msg), "%-14s%-4s%-4s%-4s%-4s%-6s%-6s%-10s%-10s%-10s\n",
			"MAC", "AP",  "AID", "PSM", "AUTH", "CTxR", "LTxR", "LDT", "RxB", "TxB");

	for (i = 0; VALID_UCAST_ENTRY_WCID(pAd, i); i++) {
		PMAC_TABLE_ENTRY pEntry = &pAd->MacTab.Content[i];

		if (IS_ENTRY_CLIENT(pEntry) && (pEntry->Sst == SST_ASSOC)) {
			if ((strlen(msg) + MAC_LINE_LEN) >= (GET_MAX_UCAST_NUM(pAd) * MAC_LINE_LEN))
				break;

			sprintf(msg + strlen(msg), "%02x%02x%02x%02x%02x%02x  ",
					pEntry->Addr[0], pEntry->Addr[1], pEntry->Addr[2],
					pEntry->Addr[3], pEntry->Addr[4], pEntry->Addr[5]);
			sprintf(msg + strlen(msg), "%-4d", (int)pEntry->func_tb_idx);
			sprintf(msg + strlen(msg), "%-4d", (int)pEntry->Aid);
			sprintf(msg + strlen(msg), "%-4d", (int)pEntry->PsMode);
			sprintf(msg + strlen(msg), "%-4d", (int)pEntry->AuthState);
			sprintf(msg + strlen(msg), "%-6d", RateIdToMbps[pAd->MacTab.Content[i].CurrTxRate]);
			sprintf(msg + strlen(msg), "%-6d", 0/*RateIdToMbps[pAd->MacTab.Content[i].HTPhyMode.word]*/); /* ToDo*/
			sprintf(msg + strlen(msg), "%-10d", 0/*pAd->MacTab.Content[i].HSCounter.LastDataPacketTime*/); /* ToDo*/
			sprintf(msg + strlen(msg), "%-10d", 0/*pAd->MacTab.Content[i].HSCounter.TotalRxByteCount*/); /* ToDo*/
			sprintf(msg + strlen(msg), "%-10d\n", 0/*pAd->MacTab.Content[i].HSCounter.TotalTxByteCount*/); /* ToDo*/
		}
	}

	/* for compatible with old API just do the printk to console*/
	wrq->u.data.length = strlen(msg);

	if (copy_to_user(wrq->u.data.pointer, msg, wrq->u.data.length))
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s", msg));

	os_free_mem(msg);
}

VOID RTMPIoctlGetSTAT2(
	IN PRTMP_ADAPTER pAd,
	IN RTMP_IOCTL_INPUT_STRUCT *wrq)
{
	char *msg;
	BSS_STRUCT *pMbss;
	INT apidx;

	os_alloc_mem(NULL, (UCHAR **)&msg, sizeof(CHAR) * (pAd->ApCfg.BssidNum * (14 * 128)));

	if (msg == NULL) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s():Alloc memory failed\n", __func__));
		return;
	}

	memset(msg, 0, pAd->ApCfg.BssidNum * (14 * 128));
	sprintf(msg, "%s", "\n");

	for (apidx = 0; apidx < pAd->ApCfg.BssidNum; apidx++) {
		pMbss = &pAd->ApCfg.MBSSID[apidx];
		sprintf(msg + strlen(msg), "ra%d\n", apidx);
		sprintf(msg + strlen(msg), "bytesTx = %ld\n", (pMbss->TransmittedByteCount));
		sprintf(msg + strlen(msg), "bytesRx = %ld\n", (pMbss->ReceivedByteCount));
		sprintf(msg + strlen(msg), "pktsTx = %ld\n", pMbss->TxCount);
		sprintf(msg + strlen(msg), "pktsRx = %ld\n", pMbss->RxCount);
		sprintf(msg + strlen(msg), "errorsTx = %ld\n", pMbss->TxErrorCount);
		sprintf(msg + strlen(msg), "errorsRx = %ld\n", pMbss->RxErrorCount);
		sprintf(msg + strlen(msg), "discardPktsTx = %ld\n", pMbss->TxDropCount);
		sprintf(msg + strlen(msg), "discardPktsRx = %ld\n", pMbss->RxDropCount);
		sprintf(msg + strlen(msg), "ucPktsTx = %ld\n", pMbss->ucPktsTx);
		sprintf(msg + strlen(msg), "ucPktsRx = %ld\n", pMbss->ucPktsRx);
		sprintf(msg + strlen(msg), "mcPktsTx = %ld\n", pMbss->mcPktsTx);
		sprintf(msg + strlen(msg), "mcPktsRx = %ld\n", pMbss->mcPktsRx);
		sprintf(msg + strlen(msg), "bcPktsTx = %ld\n", pMbss->bcPktsTx);
		sprintf(msg + strlen(msg), "bcPktsRx = %ld\n", pMbss->bcPktsRx);
	}

	wrq->u.data.length = strlen(msg);

	if (copy_to_user(wrq->u.data.pointer, msg, wrq->u.data.length))
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s", msg));

	os_free_mem(msg);
}

VOID RTMPIoctlGetRadioDynInfo(
	IN PRTMP_ADAPTER pAd,
	IN RTMP_IOCTL_INPUT_STRUCT *wrq)
{
	char *msg;
	BSS_STRUCT *pMbss;
	INT status, bandwidth;
	struct wifi_dev *wdev;
	UCHAR op_ht_bw;
	UCHAR ht_gi;

	os_alloc_mem(NULL, (UCHAR **)&msg, sizeof(CHAR) * (4096));

	if (msg == NULL) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s():Alloc memory failed\n", __func__));
		return;
	}

	memset(msg, 0, 4096);
	sprintf(msg, "%s", "\n");
	pMbss = &pAd->ApCfg.MBSSID[0];
	wdev = &pMbss->wdev;
	op_ht_bw = wlan_operate_get_ht_bw(wdev);

	if (IsHcRadioCurStatOffByWdev(wdev))
		status = 0;
	else
		status = 1;

	if (op_ht_bw  == BW_40)
		bandwidth = 1;
	else
		bandwidth = 0;

	ht_gi = wlan_config_get_ht_gi(wdev);
	sprintf(msg + strlen(msg), "status = %d\n", status);
	sprintf(msg + strlen(msg), "channelsInUse = %d\n", pAd->ChannelListNum);
	sprintf(msg + strlen(msg), "channel = %d\n", wdev->channel);
	sprintf(msg + strlen(msg), "chanWidth = %d\n", bandwidth);
	sprintf(msg + strlen(msg), "guardIntvl = %d\n", ht_gi);
	sprintf(msg + strlen(msg), "MCS = %d\n", wdev->DesiredTransmitSetting.field.MCS);
	wrq->u.data.length = strlen(msg);

	if (copy_to_user(wrq->u.data.pointer, msg, wrq->u.data.length))
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s", msg));

	os_free_mem(msg);
}
#endif/*CONFIG_AP_SUPPORT*/
#endif/*AR9_MAPI_SUPPORT*/
#endif/* INF_AR9 */

#ifdef DOT11_N_SUPPORT
INT	Set_BASetup_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UCHAR mac[6], tid;
	RTMP_STRING *token;
	RTMP_STRING sepValue[] = ":", DASH = '-';
	INT i;
	MAC_TABLE_ENTRY *pEntry = NULL;

	/*
		The BASetup inupt string format should be xx:xx:xx:xx:xx:xx-d,
			=>The six 2 digit hex-decimal number previous are the Mac address,
			=>The seventh decimal number is the tid value.
	*/
	/*MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE,("\n%s\n", arg));*/

	if (strlen(arg) <
		19) /*Mac address acceptable format 01:02:03:04:05:06 length 17 plus the "-" and tid value in decimal format.*/
		return FALSE;

	token = strchr(arg, DASH);

	if ((token != NULL) && (strlen(token) > 1)) {
		tid = (UCHAR) os_str_tol((token + 1), 0, 10);

		if (tid > (NUM_OF_TID - 1))
			return FALSE;

		*token = '\0';

		for (i = 0, token = rstrtok(arg, &sepValue[0]); token; token = rstrtok(NULL, &sepValue[0]), i++) {
			if ((strlen(token) != 2) || (!isxdigit(*token)) || (!isxdigit(*(token + 1))))
				return FALSE;

			AtoH(token, (&mac[i]), 1);
		}

		if (i != 6)
			return FALSE;

		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\n%02x:%02x:%02x:%02x:%02x:%02x-%02x\n",
				 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], tid));
#ifdef CONFIG_AP_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
		pEntry = MacTableLookup(pAd, (PUCHAR) mac);
#endif
#ifdef CONFIG_STA_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
		pEntry = MacTableLookup2(pAd, (PUCHAR) mac, NULL);
#endif

		if (pEntry) {
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\nSetup BA Session: Tid = %d\n", tid));
			ba_ori_session_setup(pAd, pEntry->wcid, tid, 0);
		}

		return TRUE;
	}

	return FALSE;
}

INT	Set_BADecline_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	struct wifi_dev *wdev = get_wdev_by_ioctl_idx_and_iftype(pAd, pObj->ioctl_if, pObj->ioctl_if_type);
	UINT8 ba_decline;

	ba_decline = os_str_tol(arg, 0, 10);
	wlan_config_set_ba_decline(wdev, ba_decline);

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Set_BADecline_Proc::(BADecline=%d)\n",
			 wlan_config_get_ba_decline(wdev)));
	return TRUE;
}

INT	Set_BAOriTearDown_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UCHAR mac[6], tid;
	UINT16 wcid;
	RTMP_STRING *token;
	RTMP_STRING sepValue[] = ":", DASH = '-';
	INT i;
	MAC_TABLE_ENTRY *pEntry = NULL;

	/*MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE,("\n%s\n", arg));*/
	/*
		The BAOriTearDown inupt string format should be xx:xx:xx:xx:xx:xx-d,
			=>The six 2 digit hex-decimal number previous are the Mac address,
			=>The seventh decimal number is the tid value.
	*/
	if (strlen(arg) <
		19) { /*Mac address acceptable format 01:02:03:04:05:06 length 17 plus the "-" and tid value in decimal format.*/
		/* another acceptable format wcid-tid */
		token = strchr(arg, DASH);

		if ((token != NULL) && (strlen(token) > 1)) {
			tid = os_str_tol((token + 1), 0, 10);

			if (tid > (NUM_OF_TID - 1)) {
				MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("tid=%d is wrong\n\r", tid));
				return FALSE;
			}

			*token = '\0';
			wcid = os_str_tol(arg, 0, 10);

			if (wcid >= 128) {
				MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("wcid=%d is wrong\n\r", wcid));
				return FALSE;
			}

			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("tear down ori ba,wcid=%d,tid=%d\n\r", wcid, tid));
			ba_ori_session_tear_down(pAd, wcid, tid, FALSE);
			return TRUE;
		}

		return FALSE;
	}

	token = strchr(arg, DASH);

	if ((token != NULL) && (strlen(token) > 1)) {
		tid = os_str_tol((token + 1), 0, 10);

		if (tid > (NUM_OF_TID - 1))
			return FALSE;

		*token = '\0';

		for (i = 0, token = rstrtok(arg, &sepValue[0]); token; token = rstrtok(NULL, &sepValue[0]), i++) {
			if ((strlen(token) != 2) || (!isxdigit(*token)) || (!isxdigit(*(token + 1))))
				return FALSE;

			AtoH(token, (&mac[i]), 1);
		}

		if (i != 6)
			return FALSE;

		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\n%02x:%02x:%02x:%02x:%02x:%02x-%02x",
				 PRINT_MAC(mac), tid));
#ifdef CONFIG_AP_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
		pEntry = MacTableLookup(pAd, (PUCHAR) mac);
#endif
#ifdef CONFIG_STA_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
		pEntry = MacTableLookup2(pAd, (PUCHAR) mac, NULL);
#endif

		if (pEntry) {
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\nTear down Ori BA Session: Tid = %d\n", tid));
			ba_ori_session_tear_down(pAd, pEntry->wcid, tid, FALSE);
		}

		return TRUE;
	}

	return FALSE;
}

INT	Set_BARecTearDown_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UCHAR mac[6], tid;
	UINT16 wcid;
	RTMP_STRING *token;
	RTMP_STRING sepValue[] = ":", DASH = '-';
	INT i;
	MAC_TABLE_ENTRY *pEntry = NULL;

	/*MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE,("\n%s\n", arg));*/
	/*
		The BARecTearDown inupt string format should be xx:xx:xx:xx:xx:xx-d,
			=>The six 2 digit hex-decimal number previous are the Mac address,
			=>The seventh decimal number is the tid value.
	*/
	if (strlen(arg) <
		19) { /*Mac address acceptable format 01:02:03:04:05:06 length 17 plus the "-" and tid value in decimal format.*/
		/* another acceptable format wcid-tid */
		token = strchr(arg, DASH);

		if ((token != NULL) && (strlen(token) > 1)) {
			tid = os_str_tol((token + 1), 0, 10);

			if (tid > (NUM_OF_TID - 1)) {
				MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("tid=%d is wrong\n\r", tid));
				return FALSE;
			}

			*token = '\0';
			wcid = os_str_tol(arg, 0, 10);

			if (wcid >= 128) {
				MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("wcid=%d is wrong\n\r", wcid));
				return FALSE;
			}

			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("tear down rec ba,wcid=%d,tid=%d\n\r", wcid, tid));
			ba_rec_session_tear_down(pAd, wcid, tid, FALSE);
			return TRUE;
		}

		return FALSE;
	}

	token = strchr(arg, DASH);

	if ((token != NULL) && (strlen(token) > 1)) {
		tid = os_str_tol((token + 1), 0, 10);

		if (tid > (NUM_OF_TID - 1))
			return FALSE;

		*token = '\0';

		for (i = 0, token = rstrtok(arg, &sepValue[0]); token; token = rstrtok(NULL, &sepValue[0]), i++) {
			if ((strlen(token) != 2) || (!isxdigit(*token)) || (!isxdigit(*(token + 1))))
				return FALSE;

			AtoH(token, (&mac[i]), 1);
		}

		if (i != 6)
			return FALSE;

		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\n%02x:%02x:%02x:%02x:%02x:%02x-%02x",
				 PRINT_MAC(mac), tid));
#ifdef CONFIG_AP_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
		pEntry = MacTableLookup(pAd, (PUCHAR) mac);
#endif
#ifdef CONFIG_STA_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
		pEntry = MacTableLookup2(pAd, (PUCHAR) mac, NULL);
#endif

		if (pEntry) {
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\nTear down Rec BA Session: Tid = %d\n", tid));
			ba_rec_session_tear_down(pAd, pEntry->wcid, tid, FALSE);
		}

		return TRUE;
	}

	return FALSE;
}

INT	Set_HtBw_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	struct wifi_dev *tdev;
	UCHAR Bandidx = 0;
	UCHAR i = 0;
	ULONG HtBw;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	struct wifi_dev *wdev = get_wdev_by_ioctl_idx_and_iftype(pAd, pObj->ioctl_if, pObj->ioctl_if_type);

	if (!wdev)
		return FALSE;

	Bandidx = HcGetBandByWdev(wdev);
	HtBw = os_str_tol(arg, 0, 10);

	if ((HtBw != BW_40) && (HtBw != BW_20))
			return FALSE;  /*Invalid argument */

	for (i = 0; i < WDEV_NUM_MAX; i++) {
		tdev = pAd->wdev_list[i];
		if (tdev && (Bandidx == HcGetBandByWdev(tdev))) {
			if (HtBw == BW_40) {
				wlan_config_set_ht_bw(tdev, BW_40);
				wlan_operate_set_ht_bw(tdev, HT_BW_40, wlan_operate_get_ext_cha(tdev));
			} else {
				wlan_config_set_ht_bw(tdev, BW_20);
				wlan_operate_set_ht_bw(tdev, HT_BW_20, EXTCHA_NONE);
			}
			SetCommonHtVht(pAd, tdev);
		}
	}

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Set_HtBw_Proc::(HtBw=%d)\n", wlan_config_get_ht_bw(wdev)));
	return TRUE;
}

INT	Set_HtMcs_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
#ifdef CONFIG_STA_SUPPORT
	BOOLEAN bAutoRate = FALSE;
#endif /* CONFIG_STA_SUPPORT */
	UCHAR HtMcs = MCS_AUTO, Mcs_tmp, ValidMcs = 15;
#ifdef DOT11_VHT_AC
	RTMP_STRING *mcs_str, *ss_str;
	UCHAR ss = 0, mcs = 0;
#endif /* DOT11_VHT_AC */
	struct wifi_dev *wdev = get_wdev_by_ioctl_idx_and_iftype(pAd, pObj->ioctl_if, pObj->ioctl_if_type);

#ifdef DOT11_VHT_AC
	if (!wdev)
		return FALSE;

	ss_str = arg;
	mcs_str = rtstrchr(arg, ':');

	if (mcs_str != NULL) {
		*mcs_str = 0;
		mcs_str++;
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s(): ss_str=%s, mcs_str=%s\n",
				 __func__, ss_str, mcs_str));

		if (strlen(ss_str) && strlen(mcs_str)) {
			mcs = os_str_tol(mcs_str, 0, 10);
			ss = os_str_tol(ss_str, 0, 10);

			if ((ss <= wlan_operate_get_tx_stream(wdev)) && (mcs <= 7))
				HtMcs = ((ss - 1) << 4) | mcs;
			else {
				HtMcs = MCS_AUTO;
				ss = 0;
			}

			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s(): %dSS-MCS%d, Auto=%s\n",
					 __func__, ss, mcs,
					 (HtMcs == MCS_AUTO && ss == 0) ? "TRUE" : "FALSE"));
			Set_FixedTxMode_Proc(pAd, "VHT");
		}
	} else
#endif /* DOT11_VHT_AC */
	{
		Mcs_tmp = os_str_tol(arg, 0, 10);

		if (Mcs_tmp <= ValidMcs || Mcs_tmp == 32)
			HtMcs = Mcs_tmp;
		else
			HtMcs = MCS_AUTO;
	}

#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd) {
		wdev = &pAd->ApCfg.MBSSID[pObj->ioctl_if].wdev;
		wdev->DesiredTransmitSetting.field.MCS = HtMcs;
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Set_HtMcs_Proc::(HtMcs=%d) for ra%d\n",
				 wdev->DesiredTransmitSetting.field.MCS, pObj->ioctl_if));
	}
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_STA(pAd) {
		MAC_TABLE_ENTRY *pEntry = NULL;

		wdev = &pAd->StaCfg[pObj->ioctl_if].wdev;
		pEntry = GetAssociatedAPByWdev(pAd, wdev);
		ASSERT(pEntry);
		wdev->DesiredTransmitSetting.field.MCS = HtMcs;
		wdev->bAutoTxRateSwitch = (HtMcs == MCS_AUTO) ? TRUE : FALSE;
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Set_HtMcs_Proc::(HtMcs=%d, bAutoTxRateSwitch = %d)\n",
				 wdev->DesiredTransmitSetting.field.MCS, wdev->bAutoTxRateSwitch));

		if ((!WMODE_CAP_N(wdev->PhyMode)) ||
			(pEntry->HTPhyMode.field.MODE < MODE_HTMIX)) {
			if ((wdev->DesiredTransmitSetting.field.MCS != MCS_AUTO) &&
				(HtMcs <= 3) &&
				(wdev->DesiredTransmitSetting.field.FixedTxMode == FIXED_TXMODE_CCK))
				RTMPSetDesiredRates(pAd, wdev, (LONG) (RateIdToMbps[HtMcs] * 1000000));
			else if ((wdev->DesiredTransmitSetting.field.MCS != MCS_AUTO) &&
					 (HtMcs <= 7) &&
					 (wdev->DesiredTransmitSetting.field.FixedTxMode == FIXED_TXMODE_OFDM))
				RTMPSetDesiredRates(pAd, wdev, (LONG) (RateIdToMbps[HtMcs + 4] * 1000000));
			else
				bAutoRate = TRUE;

			if (bAutoRate) {
				wdev->DesiredTransmitSetting.field.MCS = MCS_AUTO;
				RTMPSetDesiredRates(pAd, wdev, -1);
			}

			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Set_HtMcs_Proc::(FixedTxMode=%d)\n",
					 wdev->DesiredTransmitSetting.field.FixedTxMode));
		}

		if (ADHOC_ON(pAd))
			return TRUE;
	}
#endif /* CONFIG_STA_SUPPORT */
	SetCommonHtVht(pAd, wdev);

	return TRUE;
}

INT	Set_HtGi_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	ULONG HtGi;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	struct wifi_dev *wdev = get_wdev_by_ioctl_idx_and_iftype(pAd, pObj->ioctl_if, pObj->ioctl_if_type);

	if (!wdev)
		return FALSE;

	HtGi = os_str_tol(arg, 0, 10);

	if ((HtGi != GI_400) && (HtGi != GI_800))
		return FALSE;

	wlan_config_set_ht_gi(wdev, HtGi);
	SetCommonHtVht(pAd, wdev);
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
			 ("Set_HtGi_Proc::(ShortGI=%d)\n", wlan_config_get_ht_gi(wdev)));
	return TRUE;
}

INT	Set_HtTxBASize_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UCHAR Size;

	Size = os_str_tol(arg, 0, 10);

	if (Size <= 0 || Size >= 64)
		Size = 8;

	pAd->CommonCfg.TxBASize = Size - 1;
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("Set_HtTxBASize ::(TxBASize= %d)\n", Size));
	return TRUE;
}

INT	Set_HtDisallowTKIP_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	ULONG Value;

	Value = os_str_tol(arg, 0, 10);

	if (Value == 1)
		pAd->CommonCfg.HT_DisallowTKIP = TRUE;
	else
		pAd->CommonCfg.HT_DisallowTKIP = FALSE;

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Set_HtDisallowTKIP_Proc ::%s\n",
			 (pAd->CommonCfg.HT_DisallowTKIP == TRUE) ? "enabled" : "disabled"));
	return TRUE;
}

INT	Set_HtOpMode_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	ULONG Value;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	struct wifi_dev *wdev = get_wdev_by_ioctl_idx_and_iftype(pAd, pObj->ioctl_if, pObj->ioctl_if_type);

	if (!wdev)
		return FALSE;

	Value = os_str_tol(arg, 0, 10);

	if (Value == HTMODE_GF)
		pAd->CommonCfg.RegTransmitSetting.field.HTMODE  = HTMODE_GF;
	else if (Value == HTMODE_MM)
		pAd->CommonCfg.RegTransmitSetting.field.HTMODE  = HTMODE_MM;
	else
		return FALSE; /*Invalid argument */

	SetCommonHtVht(pAd, wdev);
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Set_HtOpMode_Proc::(HtOpMode=%d)\n",
			 pAd->CommonCfg.RegTransmitSetting.field.HTMODE));
	return TRUE;
}

INT	Set_HtLdpc_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	ULONG Value;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	struct wifi_dev *wdev = get_wdev_by_ioctl_idx_and_iftype(pAd, pObj->ioctl_if, pObj->ioctl_if_type);

	if (!wdev)
		return FALSE;

	Value = os_str_tol(arg, 0, 10);

	if (Value > 1 || Value < 0) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s:Invalid arguments!\n", __func__));
		return FALSE;
	}

	wlan_config_set_ht_ldpc(wdev, (UCHAR)Value);
	wlan_operate_set_ht_ldpc(wdev, (UCHAR)Value);

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(HtLdpc=%d)\n",
		__func__, wlan_config_get_ht_ldpc(wdev)));
	return TRUE;
}

INT	Set_HtStbc_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	ULONG Value;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	struct wifi_dev *wdev = get_wdev_by_ioctl_idx_and_iftype(pAd, pObj->ioctl_if, pObj->ioctl_if_type);

	if (!wdev)
		return FALSE;

	Value = os_str_tol(arg, 0, 10);

	if (Value == STBC_USE)
		wlan_config_set_ht_stbc(wdev, STBC_USE);
	else if (Value == STBC_NONE)
		wlan_config_set_ht_stbc(wdev, STBC_NONE);
	else
		return FALSE; /*Invalid argument */

	SetCommonHtVht(pAd, wdev);
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Set_Stbc_Proc::(HtStbc=%d)\n", wlan_config_get_ht_stbc(wdev)));
	return TRUE;
}

/*configure useage*/
INT	set_extcha_for_wdev(RTMP_ADAPTER *pAd, struct wifi_dev *wdev, UCHAR value)
{
	value = value ? EXTCHA_ABOVE : EXTCHA_BELOW;
	wlan_config_set_ext_cha(wdev, value);
	SetCommonHtVht(pAd, wdev);
	return TRUE;
}

INT	Set_HtExtcha_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	struct wifi_dev *tdev;
	UCHAR Bandidx = 0;
	UCHAR i = 0;
	ULONG Value;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	struct wifi_dev *wdev = get_wdev_by_ioctl_idx_and_iftype(pAd, pObj->ioctl_if, pObj->ioctl_if_type);
	UCHAR ext_cha;

	if (!wdev)
		return FALSE;

	Bandidx = HcGetBandByWdev(wdev);
	Value = os_str_tol(arg, 0, 10);

	if (Value != 0 && Value != 1)
		return FALSE;

	for (i = 0; i < WDEV_NUM_MAX; i++) {
		tdev = pAd->wdev_list[i];
		if (tdev && (Bandidx == HcGetBandByWdev(tdev)))
			set_extcha_for_wdev(pAd, tdev, Value);
	}
	ext_cha = wlan_config_get_ext_cha(wdev);
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Set_HtExtcha_Proc::(HtExtcha=%d)\n", ext_cha));
	return TRUE;
}

INT	Set_HtMpduDensity_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	ULONG Value;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	struct wifi_dev *wdev = get_wdev_by_ioctl_idx_and_iftype(pAd, pObj->ioctl_if, pObj->ioctl_if_type);

	Value = os_str_tol(arg, 0, 10);

	if (!wdev)
		return FALSE;

	if (Value <= 7)
		wlan_config_set_min_mpdu_start_space(wdev, Value);
	else
		wlan_config_set_min_mpdu_start_space(wdev, INTERVAL_NO_RESTRICTION);

	SetCommonHtVht(pAd, NULL);

	Value = wlan_config_get_min_mpdu_start_space(wdev);
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
			("Set_HtMpduDensity_Proc::(HtMpduDensity=%d)\n", (UCHAR)Value));
	return TRUE;
}

INT	Set_HtBaWinSize_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	struct wifi_dev *wdev = get_wdev_by_ioctl_idx_and_iftype(pAd, pObj->ioctl_if, pObj->ioctl_if_type);
	UINT16 val = BA_WIN_SZ_64;

	if (!wdev)
		return FALSE;

	val = os_str_tol(arg, 0, 10);
	if (val == 0 || val > BA_WIN_SZ_256)
		return FALSE;

	wlan_config_set_ba_txrx_wsize(wdev, val, val);
	SetCommonHtVht(pAd, wdev);

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
			("Set_HtBaWinSize_Proc::(HtBaWinSize=%d)\n", val));

	return TRUE;
}

INT	Set_HtRdg_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	ULONG Value;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	struct wifi_dev *wdev = get_wdev_by_ioctl_idx_and_iftype(pAd, pObj->ioctl_if, pObj->ioctl_if_type);

	if (!wdev)
		return FALSE;

	Value = os_str_tol(arg, 0, 10);

	if (Value != 0 && IS_ASIC_CAP(pAd, fASIC_CAP_RDG))
		pAd->CommonCfg.bRdg = TRUE;
	else
		pAd->CommonCfg.bRdg = FALSE;

	SetCommonHtVht(pAd, wdev);
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
			 ("Set_HtRdg_Proc::(HtRdg=%d)\n", pAd->CommonCfg.bRdg));
	return TRUE;
}

INT	Set_HtLinkAdapt_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	ULONG Value;

	Value = os_str_tol(arg, 0, 10);

	if (Value == 0)
		pAd->bLinkAdapt = FALSE;
	else if (Value == 1)
		pAd->bLinkAdapt = TRUE;
	else
		return FALSE; /*Invalid argument*/

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Set_HtLinkAdapt_Proc::(HtLinkAdapt=%d)\n", pAd->bLinkAdapt));
	return TRUE;
}

INT	Set_HtAmsdu_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	ULONG Value;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	struct wifi_dev *wdev = get_wdev_by_ioctl_idx_and_iftype(pAd, pObj->ioctl_if, pObj->ioctl_if_type);

	if (!wdev)
		return FALSE;

	Value = os_str_tol(arg, 0, 10);
	wlan_config_set_amsdu_en(wdev, Value);
	SetCommonHtVht(pAd, wdev);

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Set_HtAmsdu_Proc::(HtAmsdu=%d)\n",
			 wlan_config_get_amsdu_en(wdev)));
	return TRUE;
}

INT	Set_HtAutoBa_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	ULONG Value;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	struct wifi_dev *wdev = get_wdev_by_ioctl_idx_and_iftype(pAd, pObj->ioctl_if, pObj->ioctl_if_type);
	UINT8 ba_en = 1;

	if (!wdev)
		return FALSE;

	Value = os_str_tol(arg, 0, 10);
	ba_en = Value;
	wlan_config_set_ba_enable(wdev, ba_en);
	SetCommonHtVht(pAd, wdev);
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Set_HtAutoBa_Proc::(HtAutoBa=%d)\n",
			 ba_en));
	return TRUE;
}

INT	Set_HtProtect_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UCHAR Value;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	struct wifi_dev *wdev = get_wdev_by_ioctl_idx_and_iftype(pAd, pObj->ioctl_if, pObj->ioctl_if_type);

	if (!wdev)
		return FALSE;

	Value = os_str_tol(arg, 0, 10);
	wlan_config_set_ht_protect_en(wdev, Value);
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Set_HtProtect_Proc::(HtProtect=%d)\n", Value));
	return TRUE;
}

INT	Set_SendSMPSAction_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UCHAR mac[6], mode;
	RTMP_STRING *token;
	RTMP_STRING sepValue[] = ":", DASH = '-';
	INT i;
	MAC_TABLE_ENTRY *pEntry = NULL;

	/*MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE,("\n%s\n", arg));*/
	/*
		The BARecTearDown inupt string format should be xx:xx:xx:xx:xx:xx-d,
			=>The six 2 digit hex-decimal number previous are the Mac address,
			=>The seventh decimal number is the mode value.
	*/
	if (strlen(arg) <
		19) /*Mac address acceptable format 01:02:03:04:05:06 length 17 plus the "-" and mode value in decimal format.*/
		return FALSE;

	token = strchr(arg, DASH);

	if ((token != NULL) && (strlen(token) > 1)) {
		mode = os_str_tol((token + 1), 0, 10);

		if (mode > MMPS_DISABLE)
			return FALSE;

		*token = '\0';

		for (i = 0, token = rstrtok(arg, &sepValue[0]); token; token = rstrtok(NULL, &sepValue[0]), i++) {
			if ((strlen(token) != 2) || (!isxdigit(*token)) || (!isxdigit(*(token + 1))))
				return FALSE;

			AtoH(token, (&mac[i]), 1);
		}

		if (i != 6)
			return FALSE;

		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\n%02x:%02x:%02x:%02x:%02x:%02x-%02x",
				 PRINT_MAC(mac), mode));
#ifdef CONFIG_AP_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
		pEntry = MacTableLookup(pAd, mac);
#endif
#ifdef CONFIG_STA_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
		pEntry = MacTableLookup2(pAd, mac, NULL);
#endif

		if (pEntry) {
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\nSendSMPSAction SMPS mode = %d\n", mode));
			SendSMPSAction(pAd, pEntry->wcid, mode);
		}

		return TRUE;
	}

	return FALSE;
}

INT	Set_HtMIMOPSmode_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	ULONG Value;
	UCHAR mmps = 0;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	struct wifi_dev *wdev = get_wdev_by_ioctl_idx_and_iftype(pAd, pObj->ioctl_if, pObj->ioctl_if_type);

	if (!wdev)
		return FALSE;

	Value = os_str_tol(arg, 0, 10);

	if (Value > 3)
		Value = 3;
	wlan_config_set_mmps(wdev, Value);
	SetCommonHtVht(pAd, wdev);
	mmps = wlan_config_get_mmps(wdev);

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Set_HtMIMOPSmode_Proc::(MIMOPS mode=%d)\n",
			 mmps));
	return TRUE;
}

#ifdef CONFIG_AP_SUPPORT
/*
    ==========================================================================
    Description:
	Set Tx Stream number
    Return:
	TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_HtTxStream_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	ULONG	Value;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	BSS_STRUCT *pMbss = &pAd->ApCfg.MBSSID[pObj->ioctl_if];
	struct wifi_dev *wdev = get_wdev_by_ioctl_idx_and_iftype(pAd, pObj->ioctl_if, pObj->ioctl_if_type);
	UINT8 ucTxPath = pAd->Antenna.field.TxPath;

	if (!wdev)
		return FALSE;

#ifdef DBDC_MODE
	if (pAd->CommonCfg.dbdc_mode) {
		UINT8 band_idx = HcGetBandByWdev(wdev);

		if (band_idx == DBDC_BAND0)
			ucTxPath = pAd->dbdc_band0_tx_path;
		else
			ucTxPath = pAd->dbdc_band1_tx_path;
	}
#endif

#ifdef ANTENNA_CONTROL_SUPPORT
	{
		UINT8 BandIdx = HcGetBandByWdev(wdev);
		if (pAd->bAntennaSetAPEnable[BandIdx])
			ucTxPath = pAd->TxStream[BandIdx];
	}
#endif /* ANTENNA_CONTROL_SUPPORT */

	Value = os_str_tol(arg, 0, 10);

	if ((Value >= 1) && (Value <= ucTxPath)) {
		wlan_config_set_tx_stream(wdev, Value);
		wlan_operate_set_tx_stream(wdev, Value);
	} else {
		wlan_config_set_tx_stream(wdev, ucTxPath);
		wlan_operate_set_tx_stream(wdev, ucTxPath);
	}

	SetCommonHtVht(pAd, wdev);
#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd) {
		APStop(pAd, pMbss, AP_BSS_OPER_BY_RF);
		APStartUp(pAd, pMbss, AP_BSS_OPER_BY_RF);
	}
#endif
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Set_HtTxStream_Proc::(Tx Stream=%d)\n",
			 wlan_operate_get_tx_stream(wdev)));
	return TRUE;
}

/*
    ==========================================================================
    Description:
	Set Rx Stream number
    Return:
	TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_HtRxStream_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	ULONG	Value;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	BSS_STRUCT *pMbss = &pAd->ApCfg.MBSSID[pObj->ioctl_if];
	struct wifi_dev *wdev = get_wdev_by_ioctl_idx_and_iftype(pAd, pObj->ioctl_if, pObj->ioctl_if_type);
	UINT8 ucRxPath = pAd->Antenna.field.RxPath;

	if (!wdev)
		return FALSE;

#ifdef DBDC_MODE
	if (pAd->CommonCfg.dbdc_mode) {
		UINT8 band_idx = HcGetBandByWdev(wdev);

		if (band_idx == DBDC_BAND0)
			ucRxPath = pAd->dbdc_band0_rx_path;
		else
			ucRxPath = pAd->dbdc_band1_rx_path;
	}
#endif

	Value = os_str_tol(arg, 0, 10);

	if ((Value >= 1) && (Value <= ucRxPath)) {
		wlan_config_set_rx_stream(wdev, Value);
		wlan_operate_set_rx_stream(wdev, Value);
	} else {
		wlan_config_set_rx_stream(wdev, ucRxPath);
		wlan_operate_set_rx_stream(wdev, ucRxPath);
	}

	SetCommonHtVht(pAd, wdev);
#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd) {
		APStop(pAd, pMbss, AP_BSS_OPER_BY_RF);
		APStartUp(pAd, pMbss, AP_BSS_OPER_BY_RF);
	}
#endif
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Set_HtRxStream_Proc::(Rx Stream=%d)\n",
			 wlan_operate_get_rx_stream(wdev)));
	return TRUE;
}

#ifdef DOT11_N_SUPPORT
#ifdef GREENAP_SUPPORT
INT	Set_GreenAP_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	ULONG Value;

	Value = os_str_tol(arg, 0, 10);

	if (Value == 0)
		greenap_proc(pAd, FALSE);
	else if (Value == 1)
		greenap_proc(pAd, TRUE);
	else
		return FALSE; /*Invalid argument*/

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Set_GreenAP_Proc::(greenap_cap=%d)\n",
			 greenap_get_capability(pAd)));
	return TRUE;
}
#endif /* GREENAP_SUPPORT */
#endif /* DOT11_N_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */

#ifdef PCIE_ASPM_DYM_CTRL_SUPPORT
INT set_pcie_aspm_dym_ctrl_cap_proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	ULONG Value;

	Value = os_str_tol(arg, 0, 10);

	if (Value == 0) {
		mt_asic_pcie_aspm_dym_ctrl(pAd, DBDC_BAND0, FALSE, FALSE);
		if (pAd->CommonCfg.dbdc_mode)
			mt_asic_pcie_aspm_dym_ctrl(pAd, DBDC_BAND1, FALSE, FALSE);
		set_pcie_aspm_dym_ctrl_cap(pAd, FALSE);
	} else if (Value == 1) {
		set_pcie_aspm_dym_ctrl_cap(pAd, TRUE);
	} else {
		return FALSE; /*Invalid argument*/
	}

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s=%d\n",
		__func__,
		get_pcie_aspm_dym_ctrl_cap(pAd)));

	return TRUE;
}
#endif /* PCIE_ASPM_DYM_CTRL_SUPPORT */

#ifdef DOT11_HE_AX
#ifdef WIFI_TWT_SUPPORT
INT set_twt_support_proc(struct _RTMP_ADAPTER *ad, RTMP_STRING *arg)
{
	POS_COOKIE obj = (POS_COOKIE) ad->OS_Cookie;
	struct wifi_dev *wdev = get_wdev_by_ioctl_idx_and_iftype(ad, obj->ioctl_if, obj->ioctl_if_type);
	UINT8 twt_support =  os_str_tol(arg, 0, 10);

	if (!wdev)
		return FALSE;

	if (twt_support < TWT_SUPPORT_TYPE_NUM) {
		wlan_config_set_he_twt_support(wdev, twt_support);
	}

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
		("%s(), if=%d, if_type=%d, twt_support on wf_cfg_twt=%d\n",
		__func__,
		obj->ioctl_if,
		obj->ioctl_if_type,
		wlan_config_get_he_twt_support(wdev)));

	return TRUE;
}

INT set_twt_proc(struct _RTMP_ADAPTER *ad, RTMP_STRING *arg)
{
	static struct twt_agrt_para twt_agrt_para_local = {0};

	POS_COOKIE	obj = (POS_COOKIE) ad->OS_Cookie;
	UCHAR if_idx = obj->ioctl_if;
	struct wifi_dev *wdev = NULL;

	UINT32 para_id = 0;
	UINT32 value[8] = {0};
	INT para_num = 0;

	UINT32 current_tsf[2];

	wdev = get_wdev_by_ioctl_idx_and_iftype(ad, if_idx, obj->ioctl_if_type);

	if (!wdev) {
		MTWF_LOG(DBG_CAT_PROTO, CATPROTO_TWT, DBG_LVL_ERROR,
			("%s: wdev=NULL, please check\n", __func__));
		return FALSE;
	}

	if (arg == NULL || strlen(arg) == 0)
		goto format_error;

	para_num = sscanf(arg, "%d:%d:%d:%d:%d:%d:%d:%d:%d", &para_id, &value[0],
		&value[1], &value[2], &value[3], &value[4], &value[5], &value[6], &value[7]);

	MTWF_LOG(DBG_CAT_PROTO, CATPROTO_TWT, DBG_LVL_TRACE,
		("para_num=%d, para_id=%d\n",
		para_num, para_id));

	/* para_id=0(dump), para_id=1(write) */
	if (((para_id >= 0) && (para_id <= 5)) && (para_num != 1))
		goto format_error;

	/* para_id=6~23 case */
	if (((para_id >= 6) && (para_id < 24)) && (para_num != 2))
		goto format_error;

	/* para_id=24 case */
	if ((para_id == 24) && (para_num != 9))
		goto format_error;

	/* handle parameters assignment */
	switch (para_id) {
	case 0: /* dump twt parameters */
		MTWF_LOG(DBG_CAT_PROTO, CATPROTO_TWT, DBG_LVL_TRACE, ("agrt_tbl_idx=%d\n", twt_agrt_para_local.agrt_tbl_idx));
		MTWF_LOG(DBG_CAT_PROTO, CATPROTO_TWT, DBG_LVL_TRACE, ("agrt_ctrl_flag=%d\n", twt_agrt_para_local.agrt_ctrl_flag));
		MTWF_LOG(DBG_CAT_PROTO, CATPROTO_TWT, DBG_LVL_TRACE, ("own_mac_idx=%d\n", twt_agrt_para_local.own_mac_idx));
		MTWF_LOG(DBG_CAT_PROTO, CATPROTO_TWT, DBG_LVL_TRACE, ("flow_id=%d\n", twt_agrt_para_local.flow_id));
		MTWF_LOG(DBG_CAT_PROTO, CATPROTO_TWT, DBG_LVL_TRACE, ("peer_id_grp_id=%d\n", twt_agrt_para_local.peer_id_grp_id));
		MTWF_LOG(DBG_CAT_PROTO, CATPROTO_TWT, DBG_LVL_TRACE, ("agrt_sp_duration=%d\n", twt_agrt_para_local.agrt_sp_duration));
		MTWF_LOG(DBG_CAT_PROTO, CATPROTO_TWT, DBG_LVL_TRACE, ("bss_idx=%d\n", twt_agrt_para_local.bss_idx));
		MTWF_LOG(DBG_CAT_PROTO, CATPROTO_TWT, DBG_LVL_TRACE, ("agrt_sp_start_tsf_low=%d\n", twt_agrt_para_local.agrt_sp_start_tsf_low));
		MTWF_LOG(DBG_CAT_PROTO, CATPROTO_TWT, DBG_LVL_TRACE, ("agrt_sp_start_tsf_high=%d\n", twt_agrt_para_local.agrt_sp_start_tsf_high));
		MTWF_LOG(DBG_CAT_PROTO, CATPROTO_TWT, DBG_LVL_TRACE, ("agrt_sp_wake_intvl_mantissa=%d\n", twt_agrt_para_local.agrt_sp_wake_intvl_mantissa));
		MTWF_LOG(DBG_CAT_PROTO, CATPROTO_TWT, DBG_LVL_TRACE, ("agrt_sp_wake_intvl_exponent=%d\n", twt_agrt_para_local.agrt_sp_wake_intvl_exponent));
		MTWF_LOG(DBG_CAT_PROTO, CATPROTO_TWT, DBG_LVL_TRACE, ("is_role_ap=%d\n", twt_agrt_para_local.is_role_ap));
		MTWF_LOG(DBG_CAT_PROTO, CATPROTO_TWT, DBG_LVL_TRACE, ("agrt_para_bitmap=%d\n", twt_agrt_para_local.agrt_para_bitmap));
		MTWF_LOG(DBG_CAT_PROTO, CATPROTO_TWT, DBG_LVL_TRACE, ("reserved_a=%d\n", twt_agrt_para_local.reserved_a));
		MTWF_LOG(DBG_CAT_PROTO, CATPROTO_TWT, DBG_LVL_TRACE, ("reserved_b=%d\n", twt_agrt_para_local.reserved_b));
		MTWF_LOG(DBG_CAT_PROTO, CATPROTO_TWT, DBG_LVL_TRACE, ("grp_member_cnt=%d\n", twt_agrt_para_local.grp_member_cnt));
		MTWF_LOG(DBG_CAT_PROTO, CATPROTO_TWT, DBG_LVL_TRACE, ("reserved_d=%d\n", twt_agrt_para_local.reserved_d));
		MTWF_LOG(DBG_CAT_PROTO, CATPROTO_TWT, DBG_LVL_TRACE, ("sta_list[%d,%d,%d,%d,%d,%d,%d,%d]\n",
			twt_agrt_para_local.sta_list[0],
			twt_agrt_para_local.sta_list[1],
			twt_agrt_para_local.sta_list[2],
			twt_agrt_para_local.sta_list[3],
			twt_agrt_para_local.sta_list[4],
			twt_agrt_para_local.sta_list[5],
			twt_agrt_para_local.sta_list[6],
			twt_agrt_para_local.sta_list[7]));
		break;

	case 1:
		mt_asic_twt_agrt_update(wdev, twt_agrt_para_local);
		break;

	case 2:
		twt_resource_dump(wdev);
		break;

	case 3:
		twt_get_current_tsf(wdev, current_tsf);
		MTWF_LOG(DBG_CAT_PROTO, CATPROTO_TWT, DBG_LVL_TRACE, ("current_tsf(0x%.8x, 0x%.8x)\n",
			current_tsf[1], current_tsf[0]));
		break;

	case 4:
		break;

	case 5:
		break;

	case 6:
		twt_agrt_para_local.agrt_tbl_idx = value[0];
		break;

	case 7:
		twt_agrt_para_local.agrt_ctrl_flag = value[0];
		break;

	case 8:
		twt_agrt_para_local.own_mac_idx = value[0];
		break;

	case 9:
		twt_agrt_para_local.flow_id = value[0];
		break;

	case 10:
		twt_agrt_para_local.peer_id_grp_id = value[0];
		break;

	case 11:
		twt_agrt_para_local.agrt_sp_duration = value[0];
		break;

	case 12:
		twt_agrt_para_local.bss_idx = value[0];
		break;

	case 13:
		twt_agrt_para_local.agrt_sp_start_tsf_low = value[0];
		break;

	case 14:
		twt_agrt_para_local.agrt_sp_start_tsf_high = value[0];
		break;

	case 15:
		twt_agrt_para_local.agrt_sp_wake_intvl_mantissa = value[0];
		break;

	case 16:
		twt_agrt_para_local.agrt_sp_wake_intvl_exponent = value[0];
		break;

	case 17:
		twt_agrt_para_local.is_role_ap = value[0];
		break;

	case 18:
		twt_agrt_para_local.agrt_para_bitmap = value[0];
		break;

	case 19:
		twt_agrt_para_local.reserved_a = value[0];
		break;

	case 20:
		twt_agrt_para_local.reserved_b = value[0];
		break;

	case 21:
		twt_agrt_para_local.grp_member_cnt = value[0];
		break;

	case 22:
		twt_agrt_para_local.reserved_c = value[0];
		break;

	case 23:
		twt_agrt_para_local.reserved_d = value[0];
		break;

	case 24:
		os_move_mem(&twt_agrt_para_local.sta_list, &value, sizeof(value));
		break;

	default:
		break;
	}

	return TRUE;

format_error:
	MTWF_LOG(DBG_CAT_PROTO, CATPROTO_TWT, DBG_LVL_TRACE, ("Format error! correct format:\n"));
	MTWF_LOG(DBG_CAT_PROTO, CATPROTO_TWT, DBG_LVL_TRACE, ("iwpriv ra0 set twt=[para_id]:[value]\n"));
	MTWF_LOG(DBG_CAT_PROTO, CATPROTO_TWT, DBG_LVL_TRACE, ("\t 1. Dump twt parameters, para_id=0\n"));
	MTWF_LOG(DBG_CAT_PROTO, CATPROTO_TWT, DBG_LVL_TRACE, ("\t 2. Write twt parameters to FW, para_id=1\n"));
	MTWF_LOG(DBG_CAT_PROTO, CATPROTO_TWT, DBG_LVL_TRACE, ("\t 2. Update twt parameters, para_id=6~24:value\n"));

	return FALSE;
}
#endif /* WIFI_TWT_SUPPORT */
#endif /* DOT11_HE_AX */

INT	Set_ForceShortGI_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	ULONG Value;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	struct wifi_dev *wdev = get_wdev_by_ioctl_idx_and_iftype(pAd, pObj->ioctl_if, pObj->ioctl_if_type);

	if (!wdev)
		return FALSE;

	Value = os_str_tol(arg, 0, 10);

	if (Value == 0)
		pAd->WIFItestbed.bShortGI = FALSE;
	else if (Value == 1)
		pAd->WIFItestbed.bShortGI = TRUE;
	else
		return FALSE; /*Invalid argument*/

	SetCommonHtVht(pAd, wdev);
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Set_ForceShortGI_Proc::(ForceShortGI=%d)\n",
			 pAd->WIFItestbed.bShortGI));
	return TRUE;
}

INT	Set_ForceGF_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	ULONG Value;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	struct wifi_dev *wdev = get_wdev_by_ioctl_idx_and_iftype(pAd, pObj->ioctl_if, pObj->ioctl_if_type);

	if (!wdev)
		return FALSE;

	Value = os_str_tol(arg, 0, 10);

	if (Value == 0)
		pAd->WIFItestbed.bGreenField = FALSE;
	else if (Value == 1)
		pAd->WIFItestbed.bGreenField = TRUE;
	else
		return FALSE; /*Invalid argument*/

	SetCommonHtVht(pAd, wdev);
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Set_ForceGF_Proc::(ForceGF=%d)\n",
			 pAd->WIFItestbed.bGreenField));
	return TRUE;
}

INT	Set_HtMimoPs_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	ULONG Value;

	Value = os_str_tol(arg, 0, 10);

	if (Value == 0)
		pAd->CommonCfg.bMIMOPSEnable = FALSE;
	else if (Value == 1)
		pAd->CommonCfg.bMIMOPSEnable = TRUE;
	else
		return FALSE; /*Invalid argument*/

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Set_HtMimoPs_Proc::(HtMimoPs=%d)\n",
			 pAd->CommonCfg.bMIMOPSEnable));
	return TRUE;
}

#ifdef DOT11N_DRAFT3
INT Set_HT_BssCoex_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *pParam)
{
	UCHAR bBssCoexEnable = os_str_tol(pParam, 0, 10);
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	struct wifi_dev *wdev = get_wdev_by_ioctl_idx_and_iftype(pAd, pObj->ioctl_if, pObj->ioctl_if_type);

	if (!wdev)
		return FALSE;

	pAd->CommonCfg.bBssCoexEnable = ((bBssCoexEnable == 1) ? TRUE : FALSE);
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Set bBssCoexEnable=%d!\n", pAd->CommonCfg.bBssCoexEnable));

	if ((pAd->CommonCfg.bBssCoexEnable == FALSE)
		&& pAd->CommonCfg.bRcvBSSWidthTriggerEvents) {
		/* switch back 20/40 */
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Set bBssCoexEnable:  Switch back 20/40.\n"));
		pAd->CommonCfg.bRcvBSSWidthTriggerEvents = FALSE;

		if ((HcIsRfSupport(pAd, RFIC_24GHZ)) && (wlan_config_get_ht_bw(wdev) == BW_40))
			wlan_operate_set_ht_bw(wdev, HT_BW_40, wlan_config_get_ext_cha(wdev));

	}

	return TRUE;
}

INT Set_HT_BssCoexApCntThr_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *pParam)
{
	pAd->CommonCfg.BssCoexApCntThr = os_str_tol(pParam, 0, 10);
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Set BssCoexApCntThr=%d!\n", pAd->CommonCfg.BssCoexApCntThr));
	return TRUE;
}
#endif /* DOT11N_DRAFT3 */

#endif /* DOT11_N_SUPPORT */

#ifdef DOT11_VHT_AC
INT	Set_VhtBw_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	struct wifi_dev *tdev;
	UCHAR Bandidx = 0;
	UCHAR i = 0;
	ULONG vht_cw;
	UCHAR vht_bw = VHT_BW_80;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	struct wifi_dev *wdev = get_wdev_by_ioctl_idx_and_iftype(pAd, pObj->ioctl_if, pObj->ioctl_if_type);

	if (!wdev)
		return FALSE;

	Bandidx = HcGetBandByWdev(wdev);
	vht_cw = os_str_tol(arg, 0, 10);

	if (wdev->channel <= 14)
		goto direct_done;

	if (vht_cw <= VHT_BW_8080)
		vht_bw = vht_cw;
	else
		vht_bw = VHT_BW_2040;

	for (i = 0; i < WDEV_NUM_MAX; i++) {
		tdev = pAd->wdev_list[i];
		if (tdev && (Bandidx == HcGetBandByWdev(tdev))) {
				wlan_config_set_vht_bw(tdev, vht_bw);
				if (!WMODE_CAP_AC(tdev->PhyMode))
					goto direct_done;
				wlan_operate_set_vht_bw(tdev, vht_bw);
		}
	}
direct_done:
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Set_VhtBw_Proc::(VHT_BW=%d)\n", vht_bw));
	return TRUE;
}

INT set_VhtBwSignal_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UCHAR *bwsig_str[] = {"NONE", "STATIC", "DYNAMIC"};
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	struct wifi_dev *wdev = get_wdev_by_ioctl_idx_and_iftype(pAd, pObj->ioctl_if, pObj->ioctl_if_type);
	ULONG bw_signal = os_str_tol(arg, 0, 10);

	if (bw_signal > BW_SIGNALING_DYNAMIC)
		bw_signal = BW_SIGNALING_DISABLE;
	wlan_config_set_vht_bw_sig(wdev, bw_signal);

	AsicSetRtsSignalTA(pAd, bw_signal);

	if (bw_signal > BW_SIGNALING_DISABLE) {
		UINT32 value = 0;

		MAC_IO_READ32(pAd->hdev_ctrl, TMAC_TCR, &value);
		value |= DCH_DET_DIS;
		MAC_IO_WRITE32(pAd->hdev_ctrl, TMAC_TCR, value);

		if (IS_MT7615(pAd)) {
			if (bw_signal == BW_SIGNALING_DYNAMIC)
				MAC_IO_WRITE32(pAd->hdev_ctrl, AGG_AALCR0, 0x02020202);
		}
		/*Set RTS Threshold to a lower Value */
		/* Otherwise RTS Packets are not send - TGAC 5.2.67A*/
		set_rts_len_thld(wdev, 500);
	} else {
		UINT32 value = 0;

		MAC_IO_READ32(pAd->hdev_ctrl, TMAC_TCR, &value);
		value &= (~DCH_DET_DIS);
		MAC_IO_WRITE32(pAd->hdev_ctrl, TMAC_TCR, value);

		if (IS_MT7615(pAd)) {
			if (bw_signal == BW_SIGNALING_DYNAMIC)
				MAC_IO_WRITE32(pAd->hdev_ctrl, AGG_AALCR0, 0x00000000);
		}
		/*Set RTS Threshold to a lower Value */
		/* Otherwise RTS Packets are not send - TGAC 5.2.67A*/
		set_rts_len_thld(wdev, 500);
	}

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
			("vht_bw_signal = %s\n", bwsig_str[bw_signal]));

	return TRUE;
}

INT	Set_VhtLdpc_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	ULONG Value;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	struct wifi_dev *wdev = get_wdev_by_ioctl_idx_and_iftype(pAd, pObj->ioctl_if, pObj->ioctl_if_type);

	if (!wdev)
		return FALSE;

	Value = os_str_tol(arg, 0, 10);

	if (Value > 1 || Value < 0) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s:Invalid arguments!\n", __func__));
		return FALSE;
	}

	wlan_config_set_vht_ldpc(wdev, (UCHAR)Value);
	wlan_operate_set_vht_ldpc(wdev, (UCHAR)Value);

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(VhtLdpc=%d)\n",
		__func__, wlan_config_get_vht_ldpc(wdev)));
	return TRUE;
}

INT	Set_VhtStbc_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	ULONG Value;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	struct wifi_dev *wdev = get_wdev_by_ioctl_idx_and_iftype(pAd, pObj->ioctl_if, pObj->ioctl_if_type);

	if (!wdev)
		return FALSE;

	Value = os_str_tol(arg, 0, 10);

	if (Value == STBC_USE)
		wlan_config_set_vht_stbc(wdev, STBC_USE);
	else if (Value == STBC_NONE)
		wlan_config_set_vht_stbc(wdev, STBC_NONE);
	else
		return FALSE; /*Invalid argument */

	SetCommonHtVht(pAd, wdev);
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Set_VhtStbc_Proc::(VhtStbc=%d)\n",
			 wlan_config_get_vht_stbc(wdev)));
	return TRUE;
}

INT	Set_VhtDisallowNonVHT_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	ULONG Value;

	Value = os_str_tol(arg, 0, 10);

	if (Value == 0)
		pAd->CommonCfg.bNonVhtDisallow = FALSE;
	else
		pAd->CommonCfg.bNonVhtDisallow = TRUE;

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Set_VhtDisallowNonVHT_Proc::(bNonVhtDisallow=%d)\n",
			 pAd->CommonCfg.bNonVhtDisallow));
	return TRUE;
}
#endif /* DOT11_VHT_AC */

#ifdef ETH_CONVERT_SUPPORT
INT Set_EthConvertMode_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	/*
		Dongle mode: it means use our default MAC address to connect to AP, and
					support multiple internal PCs connect to Internet via this default MAC
		Clone mode : it means use one specific MAC address to connect to remote AP, and
					just the node who owns the MAC address can connect to Internet.
		Hybrid mode: it means use some specific MAC address to connecto to remote AP, and
					support mulitple internal PCs connect to Internet via this specified MAC address.
	*/
	if (rtstrcasecmp(arg, "dongle") == TRUE) {
		pAd->EthConvert.ECMode = ETH_CONVERT_MODE_DONGLE;
		NdisMoveMemory(&pAd->EthConvert.EthCloneMac[0], &pAd->CurrentAddress[0], MAC_ADDR_LEN);
		pAd->EthConvert.CloneMacVaild = TRUE;
	} else if (rtstrcasecmp(arg, "clone") == TRUE) {
		pAd->EthConvert.ECMode = ETH_CONVERT_MODE_CLONE;
		pAd->EthConvert.CloneMacVaild = FALSE;
	} else if (rtstrcasecmp(arg, "hybrid") == TRUE) {
		pAd->EthConvert.ECMode = ETH_CONVERT_MODE_HYBRID;
		pAd->EthConvert.CloneMacVaild = FALSE;
	} else {
		pAd->EthConvert.ECMode = ETH_CONVERT_MODE_DISABLE;
		pAd->EthConvert.CloneMacVaild = FALSE;
	}

	pAd->EthConvert.macAutoLearn = FALSE;
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Set_EthConvertMode_Proc(): EthConvertMode=%d!\n",
			 pAd->EthConvert.ECMode));
	return TRUE;
}

INT Set_EthCloneMac_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	extern UCHAR ZERO_MAC_ADDR[MAC_ADDR_LEN];
	extern UCHAR BROADCAST_ADDR[MAC_ADDR_LEN];
	/*
		If the input is the zero mac address, it means use our default(from EEPROM) MAC address as out-going
		   MAC address.
		If the input is the broadcast MAC address, it means use the source MAC of first packet forwarded by
		   our device as the out-going MAC address.
		If the input is any other specific valid MAC address, use it as the out-going MAC address.
	*/
	pAd->EthConvert.macAutoLearn = FALSE;

	if (strlen(arg) == 0) {
		NdisZeroMemory(&pAd->EthConvert.EthCloneMac[0], MAC_ADDR_LEN);
		goto done;
	}

	if (rtstrmactohex(arg, (RTMP_STRING *) &pAd->EthConvert.EthCloneMac[0]) == FALSE)
		goto fail;

done:
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
			 ("Set_EthCloneMac_Proc(): CloneMac = %02x:%02x:%02x:%02x:%02x:%02x\n",
			  pAd->EthConvert.EthCloneMac[0], pAd->EthConvert.EthCloneMac[1], pAd->EthConvert.EthCloneMac[2],
			  pAd->EthConvert.EthCloneMac[3], pAd->EthConvert.EthCloneMac[4], pAd->EthConvert.EthCloneMac[5]));

	if (NdisEqualMemory(&pAd->EthConvert.EthCloneMac[0], &ZERO_MAC_ADDR[0], MAC_ADDR_LEN)) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Use our default Mac address for cloned MAC!\n"));
		NdisMoveMemory(&pAd->EthConvert.EthCloneMac[0], &pAd->CurrentAddress[0], MAC_ADDR_LEN);
		pAd->EthConvert.CloneMacVaild = TRUE;
	} else if (NdisEqualMemory(&pAd->EthConvert.EthCloneMac[0], &BROADCAST_ADDR[0], MAC_ADDR_LEN)) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Use first frowarded Packet's source Mac for cloned MAC!\n"));
		NdisMoveMemory(&pAd->EthConvert.EthCloneMac[0], &pAd->CurrentAddress[0], MAC_ADDR_LEN);
		pAd->EthConvert.CloneMacVaild = FALSE;
		pAd->EthConvert.macAutoLearn = TRUE;
	} else {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Use user assigned spcific Mac address for cloned MAC!\n"));
		pAd->EthConvert.CloneMacVaild = TRUE;
	}

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
			 ("Set_EthCloneMac_Proc(): After ajust, CloneMac = %02x:%02x:%02x:%02x:%02x:%02x\n",
			  pAd->EthConvert.EthCloneMac[0], pAd->EthConvert.EthCloneMac[1], pAd->EthConvert.EthCloneMac[2],
			  pAd->EthConvert.EthCloneMac[3], pAd->EthConvert.EthCloneMac[4], pAd->EthConvert.EthCloneMac[5]));
	return TRUE;
fail:
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("Set_EthCloneMac_Proc: wrong Mac Address format or length!\n"));
	NdisMoveMemory(&pAd->EthConvert.EthCloneMac[0], &pAd->CurrentAddress[0], MAC_ADDR_LEN);
	return FALSE;
}
#endif /* ETH_CONVERT_SUPPORT */

INT	Set_FixedTxMode_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	struct wifi_dev *wdev = NULL;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	INT	fix_tx_mode = RT_CfgSetFixedTxPhyMode(arg);
#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
	wdev = &pAd->ApCfg.MBSSID[pObj->ioctl_if].wdev;
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
	wdev = &pAd->StaCfg[pObj->ioctl_if].wdev;
#endif /* CONFIG_STA_SUPPORT */

	if (wdev)
		wdev->DesiredTransmitSetting.field.FixedTxMode = fix_tx_mode;

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s():(FixedTxMode=%d)\n",
			 __func__, fix_tx_mode));
	return TRUE;
}

#ifdef CONFIG_APSTA_MIXED_SUPPORT
INT	Set_OpMode_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	ULONG Value;

	Value = os_str_tol(arg, 0, 10);
#ifdef RTMP_MAC_PCI

	if (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_INTERRUPT_REGISTER_TO_OS))
#endif /* RTMP_MAC_PCI */
		{
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("Can not switch operate mode on interface up !!\n"));
			return FALSE;
		}

	if (Value == 0)
		pAd->OpMode = OPMODE_STA;
	else if (Value == 1)
		pAd->OpMode = OPMODE_AP;
	else
		return FALSE; /*Invalid argument*/

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Set_OpMode_Proc::(OpMode=%s)\n",
			 pAd->OpMode == 1 ? "AP Mode" : "STA Mode"));
	return TRUE;
}
#endif /* CONFIG_APSTA_MIXED_SUPPORT */

#ifdef STREAM_MODE_SUPPORT
/*
	========================================================================
	Routine Description:
		Set the enable/disable the stream mode

	Arguments:
		1:	enable for 1SS
		2:	enable for 2SS
		3:	enable for 1SS and 2SS
		0:	disable

	Notes:
		Currently only support 1SS
	========================================================================
*/
INT Set_StreamMode_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UINT32 streamWord, reg, regAddr;
	struct _RTMP_CHIP_CAP *cap = hc_get_chip_cap(pAd->hdev_ctrl);

	/* TODO: shiang-7603 */
	if (IS_HIF_TYPE(pAd, HIF_MT)) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s(): Not support for HIF_MT yet!\n",
				 __func__));
		return FALSE;
	}

	if (cap->FlgHwStreamMode == FALSE) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("chip not supported feature\n"));
		return FALSE;
	}

	pAd->CommonCfg.StreamMode = (os_str_tol(arg, 0, 10) & 0x3);
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s():StreamMode=%d\n", __func__, pAd->CommonCfg.StreamMode));
	streamWord = StreamModeRegVal(pAd);

	for (regAddr = TX_CHAIN_ADDR0_H; regAddr <= TX_CHAIN_ADDR3_H; regAddr += 8) {
		RTMP_IO_READ32(pAd->hdev_ctrl, regAddr, &reg);
		reg &= (~0x000F0000);
		RTMP_IO_WRITE32(pAd->hdev_ctrl, regAddr, streamWord | reg);
	}

	return TRUE;
}

INT Set_StreamModeMac_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	return FALSE;
}

INT Set_StreamModeMCS_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	pAd->CommonCfg.StreamModeMCS = os_str_tol(arg, 0, 16);
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s():StreamModeMCS=%02X\n",
			 __func__, pAd->CommonCfg.StreamModeMCS));
	return TRUE;
}
#endif /* STREAM_MODE_SUPPORT */

INT Set_LongRetryLimit_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UCHAR LongRetryLimit = (UCHAR)os_str_tol(arg, 0, 10);

	AsicSetRetryLimit(pAd, TX_RTY_CFG_RTY_LIMIT_LONG, LongRetryLimit);
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("IF Set_LongRetryLimit_Proc::(LongRetryLimit=0x%x)\n",
			 LongRetryLimit));
	return TRUE;
}

INT Set_ShortRetryLimit_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UCHAR ShortRetryLimit = (UCHAR)os_str_tol(arg, 0, 10);

	AsicSetRetryLimit(pAd, TX_RTY_CFG_RTY_LIMIT_SHORT, ShortRetryLimit);
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("IF Set_ShortRetryLimit_Proc::(ShortRetryLimit=0x%x)\n",
			 ShortRetryLimit));
	return TRUE;
}

INT Set_AutoFallBack_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	return RT_CfgSetAutoFallBack(pAd, arg);
}

#ifdef MEM_ALLOC_INFO_SUPPORT
INT Show_MemInfo_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UINT show = 0, option = 0;
	CHAR *param;

	if (arg == NULL || strlen(arg) == 0)
		goto err;

	param = rstrtok(arg, "-");

	if (param) {
		show = os_str_tol(param, 0, 10);
		param = rstrtok(NULL, "-");

		if (param)
			option = os_str_tol(param, 0, 16);
	}
	ShowMemAllocInfo(show, option);
	return TRUE;
err:
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
		("\tiwpriv $(inf_name) show meminfo=[type]-[calleridx]\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
		("\t[type]:\n\t%s\t%s\t%s\t%s\t%s", "0: Show alloc num of caller\n",
		"1: Show all the alloc info\n", "2: Show all the alloc info of caller\n",
		"3: Dump alloc data of caller\n", "4: Release all the memory of caller\n"));
	return TRUE;
}

INT Show_PktInfo_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UINT show = 0, option = 0;
	CHAR *param;

	if (arg == NULL || strlen(arg) == 0)
		goto err;

	param = rstrtok(arg, "-");

	if (param) {
		show = os_str_tol(param, 0, 10);
		param = rstrtok(NULL, "-");

		if (param)
			option = os_str_tol(param, 0, 16);
	}
	ShowPktAllocInfo(show, option);
	return TRUE;
err:
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
		("\tiwpriv $(inf_name) show pktmeminfo=[type]-[calleridx]\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
		("\t[type]:\n\t%s\t%s\t%s\t%s\t%s", "0: Show alloc num of caller\n",
		"1: Show all the alloc info\n", "2: Show all the alloc info of caller\n",
		"3: Dump alloc data of caller\n", "4: Release all the memory of caller\n"));
	return TRUE;
}
#endif /* MEM_ALLOC_INFO_SUPPORT */

INT	Show_SSID_Proc(
	IN	PRTMP_ADAPTER	pAd,
	OUT	RTMP_STRING *pBuf,
	IN	ULONG			BufLen)
{
	UCHAR	ssid_str[33];
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;

	NdisZeroMemory(&ssid_str[0], 33);
#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd) {
		NdisMoveMemory(&ssid_str[0],
					   pAd->ApCfg.MBSSID[pObj->ioctl_if].Ssid,
					   pAd->ApCfg.MBSSID[pObj->ioctl_if].SsidLen);
	}
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_STA(pAd) {
		NdisMoveMemory(&ssid_str[0],
					   pAd->StaCfg[pObj->ioctl_if].Ssid,
					   pAd->StaCfg[pObj->ioctl_if].SsidLen);
	}
#endif /* CONFIG_STA_SUPPORT */
	snprintf(pBuf, BufLen, "\t%s", ssid_str);
	return 0;
}

static VOID GetWirelessMode(USHORT PhyMode, UCHAR *pBuf, UCHAR BufLen)
{
	switch (PhyMode) {
	case (WMODE_B | WMODE_G):
		snprintf(pBuf, BufLen, "\t11B/G");
		break;

	case (WMODE_B):
		snprintf(pBuf, BufLen, "\t11B");
		break;

	case (WMODE_A):
		snprintf(pBuf, BufLen, "\t11A");
		break;

	case (WMODE_A | WMODE_B | WMODE_G):
		snprintf(pBuf, BufLen, "\t11A/B/G");
		break;

	case (WMODE_G):
		snprintf(pBuf, BufLen, "\t11G");
		break;
#ifdef DOT11_N_SUPPORT

	case (WMODE_A | WMODE_B | WMODE_G | WMODE_GN | WMODE_AN):
		snprintf(pBuf, BufLen, "\t11A/B/G/N");
		break;

	case (WMODE_GN):
		snprintf(pBuf, BufLen, "\t11N only with 2.4G");
		break;

	case (WMODE_G | WMODE_GN):
		snprintf(pBuf, BufLen, "\t11G/N");
		break;

	case (WMODE_A | WMODE_AN):
		snprintf(pBuf, BufLen, "\t11A/N");
		break;

	case (WMODE_B | WMODE_G | WMODE_GN):
		snprintf(pBuf, BufLen, "\t11B/G/N");
		break;

	case (WMODE_A | WMODE_G | WMODE_GN | WMODE_AN):
		snprintf(pBuf, BufLen, "\t11A/G/N");
		break;

	case (WMODE_AN):
		snprintf(pBuf, BufLen, "\t11N only with 5G");
		break;
#endif /* DOT11_N_SUPPORT */

	default:
		snprintf(pBuf, BufLen, "\tUnknow Value(%d)", PhyMode);
		break;
	}
}

INT	Show_WirelessMode_Proc(
	IN	PRTMP_ADAPTER	pAd,
	OUT	RTMP_STRING *pBuf,
	IN	ULONG			BufLen)
{
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	struct wifi_dev *wdev = get_wdev_by_ioctl_idx_and_iftype(pAd, pObj->ioctl_if, pObj->ioctl_if_type);

	GetWirelessMode(wdev->PhyMode, pBuf, BufLen);
	return 0;
}

INT	Show_TxBurst_Proc(
	IN	PRTMP_ADAPTER	pAd,
	OUT	RTMP_STRING *pBuf,
	IN	ULONG			BufLen)
{
	snprintf(pBuf, BufLen, "\t%s", pAd->CommonCfg.bEnableTxBurst ? "TRUE" : "FALSE");
	return 0;
}

INT	Show_TxPreamble_Proc(
	IN	PRTMP_ADAPTER	pAd,
	OUT	RTMP_STRING *pBuf,
	IN	ULONG			BufLen)
{
	switch (pAd->CommonCfg.TxPreamble) {
	case Rt802_11PreambleShort:
		snprintf(pBuf, BufLen, "\tShort");
		break;

	case Rt802_11PreambleLong:
		snprintf(pBuf, BufLen, "\tLong");
		break;

	case Rt802_11PreambleAuto:
		snprintf(pBuf, BufLen, "\tAuto");
		break;

	default:
		snprintf(pBuf, BufLen, "\tUnknown Value(%lu)", pAd->CommonCfg.TxPreamble);
		break;
	}

	return 0;
}

INT	Show_TxPower_Proc(
	IN	PRTMP_ADAPTER	pAd,
	OUT	RTMP_STRING *pBuf,
	IN	ULONG			BufLen)
{
	UINT8   BandIdx = 0;
	struct  wifi_dev *wdev;
#ifdef CONFIG_AP_SUPPORT
	POS_COOKIE  pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR       apidx = pObj->ioctl_if;
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_AP_SUPPORT

	/* obtain Band index */
	if (apidx >= pAd->ApCfg.BssidNum)
		return FALSE;

	wdev = &pAd->ApCfg.MBSSID[apidx].wdev;
	BandIdx = HcGetBandByWdev(wdev);
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT
	wdev = &pAd->StaCfg[0].wdev;
	BandIdx = HcGetBandByWdev(wdev);
#endif /* CONFIG_STA_SUPPORT */
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s: BandIdx = %d\n", __func__, BandIdx));

	/* sanity check for Band index */
	if (BandIdx >= DBDC_BAND_NUM)
		return 1;

	snprintf(pBuf, BufLen, "\t%u", pAd->CommonCfg.ucTxPowerPercentage[BandIdx]);
	return 0;
}

INT	Show_Channel_Proc(
	IN	PRTMP_ADAPTER	pAd,
	OUT	RTMP_STRING *pBuf,
	IN	ULONG			BufLen)
{
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	struct wifi_dev *wdev = get_wdev_by_ioctl_idx_and_iftype(pAd, pObj->ioctl_if, pObj->ioctl_if_type);

	if (wdev->channel > 14)
		snprintf(pBuf, BufLen, "\t5G Band: %d\n", wdev->channel);
	else
		snprintf(pBuf, BufLen, "\t2.4G Band: %d\n", wdev->channel);

	return 0;
}

INT	Show_BGProtection_Proc(
	IN	PRTMP_ADAPTER	pAd,
	OUT	RTMP_STRING *pBuf,
	IN	ULONG			BufLen)
{
	switch (pAd->CommonCfg.UseBGProtection) {
	case 1: /*Always On*/
		snprintf(pBuf, BufLen, "\tON");
		break;

	case 2: /*Always OFF*/
		snprintf(pBuf, BufLen, "\tOFF");
		break;

	case 0: /*AUTO*/
		snprintf(pBuf, BufLen, "\tAuto");
		break;

	default:
		snprintf(pBuf, BufLen, "\tUnknow Value(%lu)", pAd->CommonCfg.UseBGProtection);
		break;
	}

	return 0;
}

INT	Show_RTSThreshold_Proc(
	IN	PRTMP_ADAPTER	pAd,
	OUT	RTMP_STRING *pBuf,
	IN	ULONG			BufLen)
{
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	struct wifi_dev *wdev = get_wdev_by_ioctl_idx_and_iftype(pAd, pObj->ioctl_if, pObj->ioctl_if_type);
	UINT32 oper_len_thld;
	UINT32 conf_len_thld;

	if (!wdev)
		return 0;

	conf_len_thld = wlan_config_get_rts_len_thld(wdev);
	oper_len_thld = wlan_operate_get_rts_len_thld(wdev);
	snprintf(pBuf, BufLen, "\tRTSThreshold:: conf=%d, oper=%d", conf_len_thld, oper_len_thld);
	return 0;
}

INT	Show_FragThreshold_Proc(
	IN	PRTMP_ADAPTER	pAd,
	OUT	RTMP_STRING *pBuf,
	IN	ULONG			BufLen)
{
	UINT32 conf_frag_thld;
	UINT32 oper_frag_thld;
	POS_COOKIE pobj = (POS_COOKIE)pAd->OS_Cookie;
	struct wifi_dev *wdev = get_wdev_by_ioctl_idx_and_iftype(pAd, pobj->ioctl_if, pobj->ioctl_if_type);

	if (!wdev)
		return 0;

	conf_frag_thld = wlan_config_get_frag_thld(wdev);
	oper_frag_thld = wlan_operate_get_frag_thld(wdev);
	snprintf(pBuf, BufLen, "\tFrag thld:: conf=%u, oper=%u", conf_frag_thld, oper_frag_thld);
	return 0;
}

#ifdef DOT11_N_SUPPORT
INT	Show_HtBw_Proc(
	IN	PRTMP_ADAPTER	pAd,
	OUT	RTMP_STRING *pBuf,
	IN	ULONG			BufLen)
{
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	struct wifi_dev *wdev = get_wdev_by_ioctl_idx_and_iftype(pAd, pObj->ioctl_if, pObj->ioctl_if_type);

	if (wlan_config_get_ht_bw(wdev) == BW_40)
		snprintf(pBuf, BufLen, "\t40 MHz");
	else
		snprintf(pBuf, BufLen, "\t20 MHz");

	return 0;
}

INT	Show_HtMcs_Proc(
	IN	PRTMP_ADAPTER	pAd,
	OUT	RTMP_STRING *pBuf,
	IN	ULONG			BufLen)
{
	struct wifi_dev *wdev = NULL;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
	wdev = &pAd->ApCfg.MBSSID[pObj->ioctl_if].wdev;
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
	wdev = &pAd->StaCfg[pObj->ioctl_if].wdev;
#endif /* CONFIG_STA_SUPPORT */

	if (wdev)
		snprintf(pBuf, BufLen, "\t%u", wdev->DesiredTransmitSetting.field.MCS);

	return 0;
}

INT	Show_HtGi_Proc(
	IN	PRTMP_ADAPTER	pAd,
	OUT	RTMP_STRING *pBuf,
	IN	ULONG			BufLen)
{
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	struct wifi_dev *wdev = get_wdev_by_ioctl_idx_and_iftype(pAd, pObj->ioctl_if, pObj->ioctl_if_type);
	UCHAR ht_gi = GI_400;
	UCHAR *msg[3] = {"GI_800", "GI_400", "GI_Unknown"};

	if (!wdev)
		return 0;

	ht_gi = wlan_config_get_ht_gi(wdev);

	if (ht_gi > GI_400)
		ht_gi = 2; /*Unknown GI*/

	snprintf(pBuf, BufLen, "\ti%s", msg[ht_gi]);
	return 0;
}

INT	Show_HtOpMode_Proc(
	IN	PRTMP_ADAPTER	pAd,
	OUT	RTMP_STRING *pBuf,
	IN	ULONG			BufLen)
{
	switch (pAd->CommonCfg.RegTransmitSetting.field.HTMODE) {
	case HTMODE_GF:
		snprintf(pBuf, BufLen, "\tGF");
		break;

	case HTMODE_MM:
		snprintf(pBuf, BufLen, "\tMM");
		break;

	default:
		snprintf(pBuf, BufLen, "\tUnknow Value(%u)", pAd->CommonCfg.RegTransmitSetting.field.HTMODE);
		break;
	}

	return 0;
}

INT	Show_HtExtcha_Proc(
	IN	PRTMP_ADAPTER	pAd,
	OUT	RTMP_STRING *pBuf,
	IN	ULONG			BufLen)
{
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	struct wifi_dev *wdev = get_wdev_by_ioctl_idx_and_iftype(pAd, pObj->ioctl_if, pObj->ioctl_if_type);
	UCHAR ext_cha;

	ext_cha = wlan_config_get_ext_cha(wdev);

	switch (ext_cha) {
	case EXTCHA_BELOW:
		snprintf(pBuf, BufLen, "\tBelow");
		break;

	case EXTCHA_ABOVE:
		snprintf(pBuf, BufLen, "\tAbove");
		break;

	default:
		snprintf(pBuf, BufLen, "\tUnknow Value(%u)", ext_cha);
		break;
	}

	return 0;
}

INT	Show_HtMpduDensity_Proc(
	IN	PRTMP_ADAPTER	pAd,
	OUT	RTMP_STRING *pBuf,
	IN	ULONG			BufLen)
{
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	struct wifi_dev *wdev = get_wdev_by_ioctl_idx_and_iftype(pAd, pObj->ioctl_if, pObj->ioctl_if_type);
	UCHAR mpdu_density = 0;

	if (wdev)
		mpdu_density = wlan_config_get_min_mpdu_start_space(wdev);
	snprintf(pBuf, BufLen, "\t%u", mpdu_density);
	return 0;
}

INT	Show_HtBaWinSize_Proc(
	IN	PRTMP_ADAPTER	pAd,
	OUT	RTMP_STRING *pBuf,
	IN	ULONG			BufLen)
{
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	struct wifi_dev *wdev = get_wdev_by_ioctl_idx_and_iftype(pAd, pObj->ioctl_if, pObj->ioctl_if_type);
	UINT16 ba_tx_wsize = 0, ba_rx_wsize = 0;

	if (!wdev)
		return 0;
	ba_tx_wsize = wlan_config_get_ba_tx_wsize(wdev);
	ba_rx_wsize = wlan_config_get_ba_rx_wsize(wdev);
	snprintf(pBuf, BufLen, "\t%u %u", ba_tx_wsize, ba_rx_wsize);

	return 0;
}

INT	Show_HtRdg_Proc(
	IN	PRTMP_ADAPTER	pAd,
	OUT	RTMP_STRING *pBuf,
	IN	ULONG			BufLen)
{
	snprintf(pBuf, BufLen, "\t%s", pAd->CommonCfg.bRdg ? "TRUE" : "FALSE");
	return 0;
}

INT	Show_HtAmsdu_Proc(
	IN	PRTMP_ADAPTER	pAd,
	OUT	RTMP_STRING *pBuf,
	IN	ULONG			BufLen)
{
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	struct wifi_dev *wdev = get_wdev_by_ioctl_idx_and_iftype(pAd, pObj->ioctl_if, pObj->ioctl_if_type);
	UCHAR amsdu_en = 0;

	if (wdev)
		amsdu_en = wlan_config_get_amsdu_en(wdev);
	snprintf(pBuf, BufLen, "\t%s", (amsdu_en) ? "TRUE" : "FALSE");
	return 0;
}

INT	Show_HtAutoBa_Proc(
	IN	PRTMP_ADAPTER	pAd,
	OUT	RTMP_STRING *pBuf,
	IN	ULONG			BufLen)
{
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	struct wifi_dev *wdev = get_wdev_by_ioctl_idx_and_iftype(pAd, pObj->ioctl_if, pObj->ioctl_if_type);
	UINT8 ba_en = 1;

	if (wdev)
		ba_en = wlan_config_get_ba_enable(wdev);

	snprintf(pBuf, BufLen, "\t%s", (ba_en) ? "TRUE" : "FALSE");
	return 0;
}
#endif /* DOT11_N_SUPPORT */

INT	Show_CountryRegion_Proc(
	IN	PRTMP_ADAPTER	pAd,
	OUT	RTMP_STRING *pBuf,
	IN	ULONG			BufLen)
{
	snprintf(pBuf, BufLen, "\t%d", pAd->CommonCfg.CountryRegion);
	return 0;
}

INT	Show_CountryRegionABand_Proc(
	IN	PRTMP_ADAPTER	pAd,
	OUT	RTMP_STRING *pBuf,
	IN	ULONG			BufLen)
{
	snprintf(pBuf, BufLen, "\t%d", pAd->CommonCfg.CountryRegionForABand);
	return 0;
}

INT	Show_CountryCode_Proc(
	IN	PRTMP_ADAPTER	pAd,
	OUT	RTMP_STRING *pBuf,
	IN	ULONG			BufLen)
{
	snprintf(pBuf, BufLen, "\t%s", pAd->CommonCfg.CountryCode);
	return 0;
}

#ifdef AGGREGATION_SUPPORT
INT	Show_PktAggregate_Proc(
	IN	PRTMP_ADAPTER	pAd,
	OUT	RTMP_STRING *pBuf,
	IN	ULONG			BufLen)
{
	snprintf(pBuf, BufLen, "\t%s", pAd->CommonCfg.bAggregationCapable ? "TRUE" : "FALSE");
	return 0;
}
#endif /* AGGREGATION_SUPPORT */

INT	Show_WmmCapable_Proc(
	IN	PRTMP_ADAPTER	pAd,
	OUT	RTMP_STRING *pBuf,
	IN	ULONG			BufLen)
{
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
	snprintf(pBuf, BufLen, "\t%s", pAd->ApCfg.MBSSID[pObj->ioctl_if].wdev.bWmmCapable ? "TRUE" : "FALSE");
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
	snprintf(pBuf, BufLen, "\t%s", pAd->StaCfg[pObj->ioctl_if].wdev.bWmmCapable ? "TRUE" : "FALSE");
#endif /* CONFIG_STA_SUPPORT */
	return 0;
}

INT	Show_IEEE80211H_Proc(
	IN	PRTMP_ADAPTER	pAd,
	OUT	RTMP_STRING *pBuf,
	IN	ULONG			BufLen)
{
	snprintf(pBuf, BufLen, "\t%s", pAd->CommonCfg.bIEEE80211H ? "TRUE" : "FALSE");
	return 0;
}

#ifdef CONFIG_STA_SUPPORT
INT	Show_NetworkType_Proc(
	IN	PRTMP_ADAPTER	pAd,
	OUT	RTMP_STRING *pBuf,
	IN	ULONG			BufLen)
{
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	PSTA_ADMIN_CONFIG pStaCfg = &pAd->StaCfg[pObj->ioctl_if];

	switch (pStaCfg->BssType) {
	case BSS_ADHOC:
		snprintf(pBuf, BufLen, "\tAdhoc");
		break;

	case BSS_INFRA:
		snprintf(pBuf, BufLen, "\tInfra");
		break;

	case BSS_ANY:
		snprintf(pBuf, BufLen, "\tAny");
		break;

	case BSS_MONITOR:
		snprintf(pBuf, BufLen, "\tMonitor");
		break;

	default:
		sprintf(pBuf, "\tUnknow Value(%d)", pStaCfg->BssType);
		break;
	}

	return 0;
}

#ifdef WSC_STA_SUPPORT
INT	Show_WpsPbcBand_Proc(
	IN	PRTMP_ADAPTER	pAd,
	OUT	RTMP_STRING *pBuf,
	IN	ULONG			BufLen)
{
	POS_COOKIE pObj = (POS_COOKIE)pAd->OS_Cookie;
	PSTA_ADMIN_CONFIG pStaCfg = &pAd->StaCfg[pObj->ioctl_if];

	switch (pStaCfg->wdev.WscControl.WpsApBand) {
	case PREFERRED_WPS_AP_PHY_TYPE_2DOT4_G_FIRST:
		snprintf(pBuf, BufLen, "\t2.4G");
		break;

	case PREFERRED_WPS_AP_PHY_TYPE_5_G_FIRST:
		snprintf(pBuf, BufLen, "\t5G");
		break;

	case PREFERRED_WPS_AP_PHY_TYPE_AUTO_SELECTION:
		snprintf(pBuf, BufLen, "\tAuto");
		break;

	default:
		snprintf(pBuf, BufLen, "\tUnknow Value(%d)", pStaCfg->wdev.WscControl.WpsApBand);
		break;
	}

	return 0;
}
#endif /* WSC_STA_SUPPORT */

INT	Show_WPAPSK_Proc(
	IN	PRTMP_ADAPTER	pAd,
	OUT	RTMP_STRING *pBuf,
	IN	ULONG			BufLen)
{
	POS_COOKIE pObj = (POS_COOKIE)pAd->OS_Cookie;
	PSTA_ADMIN_CONFIG pStaCfg = &pAd->StaCfg[pObj->ioctl_if];

	if ((pStaCfg->WpaPassPhraseLen >= 8) &&
		(pStaCfg->WpaPassPhraseLen < 64))
		snprintf(pBuf, BufLen, "\tWPAPSK = %s", pStaCfg->WpaPassPhrase);
	else {
		INT idx;

		snprintf(pBuf, BufLen, "\tWPAPSK = ");

		for (idx = 0; idx < 32; idx++)
			snprintf(pBuf + strlen(pBuf), BufLen - strlen(pBuf), "%02X", pStaCfg->WpaPassPhrase[idx]);
	}

	return 0;
}

INT	Show_AutoReconnect_Proc(
	IN	PRTMP_ADAPTER	pAd,
	OUT	RTMP_STRING *pBuf,
	IN	ULONG			BufLen)
{
	POS_COOKIE pObj = (POS_COOKIE)pAd->OS_Cookie;
	PSTA_ADMIN_CONFIG pStaCfg = &pAd->StaCfg[pObj->ioctl_if];

	snprintf(pBuf, BufLen, "\tAutoReconnect = %d", pStaCfg->bAutoReconnect);
	return 0;
}

#endif /* CONFIG_STA_SUPPORT */

INT	Show_STA_RAInfo_Proc(
	IN	PRTMP_ADAPTER	pAd,
	OUT	RTMP_STRING *pBuf,
	IN	ULONG			BufLen)
{
	sprintf(pBuf, "\n");
#ifdef NEW_RATE_ADAPT_SUPPORT
	sprintf(pBuf + strlen(pBuf), "LowTrafficThrd: %d\n", pAd->CommonCfg.lowTrafficThrd);
	sprintf(pBuf + strlen(pBuf), "TrainUpRule: %d\n", pAd->CommonCfg.TrainUpRule);
	sprintf(pBuf + strlen(pBuf), "TrainUpRuleRSSI: %d\n", pAd->CommonCfg.TrainUpRuleRSSI);
	sprintf(pBuf + strlen(pBuf), "TrainUpLowThrd: %d\n", pAd->CommonCfg.TrainUpLowThrd);
	sprintf(pBuf + strlen(pBuf), "TrainUpHighThrd: %d\n", pAd->CommonCfg.TrainUpHighThrd);
#endif /* NEW_RATE_ADAPT_SUPPORT // */
#ifdef STREAM_MODE_SUPPORT
	sprintf(pBuf + strlen(pBuf), "StreamMode: %d\n", pAd->CommonCfg.StreamMode);
	sprintf(pBuf + strlen(pBuf), "StreamModeMCS: 0x%04x\n", pAd->CommonCfg.StreamModeMCS);
#endif /* STREAM_MODE_SUPPORT // */
#ifdef TXBF_SUPPORT
	sprintf(pBuf + strlen(pBuf), "ITxBfEn: %d\n", pAd->CommonCfg.RegTransmitSetting.field.ITxBfEn);
	sprintf(pBuf + strlen(pBuf), "ITxBfTimeout: %ld\n", pAd->CommonCfg.ITxBfTimeout);
	sprintf(pBuf + strlen(pBuf), "ETxBfTimeout: %ld\n", pAd->CommonCfg.ETxBfTimeout);
	sprintf(pBuf + strlen(pBuf), "CommonCfg.ETxBfEnCond: %ld\n", pAd->CommonCfg.ETxBfEnCond);
	sprintf(pBuf + strlen(pBuf), "ETxBfNoncompress: %d\n", pAd->CommonCfg.ETxBfNoncompress);
	sprintf(pBuf + strlen(pBuf), "ETxBfIncapable: %d\n", pAd->CommonCfg.ETxBfIncapable);
#endif /* TXBF_SUPPORT // */
	return 0;
}

static INT dump_mac_table(RTMP_ADAPTER *pAd, UINT32 ent_type, BOOLEAN bReptCli)
{
	INT i, j;
	ULONG DataRate = 0;
	ULONG DataRate_r = 0;
	ULONG max_DataRate = 0;
	INT sta_cnt = 0;
	INT apcli_cnt = 0;
	INT rept_cnt = 0;
	UCHAR	tmp_str[30];
	INT		temp_str_len = sizeof(tmp_str);
	ADD_HT_INFO_IE *addht;
	CHAR rssi[4] = {-127, -127, -127, -127};
#ifdef RACTRL_FW_OFFLOAD_SUPPORT
	struct _RTMP_CHIP_CAP *cap = hc_get_chip_cap(pAd->hdev_ctrl);
#endif

	struct entry_type_str_map {
		UINT32 type;
		UCHAR str[6];
	} type_str_map[9] = {
		{ENTRY_INFRA,	"AP"},
		{ENTRY_GC,	"GC"},
		{ENTRY_ADHOC,	"ADHOC"},
		{ENTRY_APCLI,	"APCLI"},
		{ENTRY_DLS,	"DLS"},
		{ENTRY_TDLS,	"TDLS"},
		{ENTRY_CLIENT,	"STA"},
		{ENTRY_REPEATER, "REPT"},
		{0,		""}
	};

	printk("\n");
#ifdef CONFIG_HOTSPOT_R2
	MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_OFF,
		("\n%-19s%-6s%-5s%-7s%-4s%-4s%-4s%-7s%-20s%-12s%-9s%-12s%-9s%-10s%-7s%-10s%-7s\n",
		   "MAC", "MODE", "AID", "WCID", "BSS", "PSM",
		   "WMM", "MIMOPS", "RSSI0/1/2/3", "PhMd", "BW", "MCS", "SGI",
		   "STBC",      "Idle", "Rate",     "QosMap"));
#else
	MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_OFF,
		("\n%-19s%-6s%-5s%-7s%-4s%-4s%-4s%-7s%-20s%-12s%-9s%-12s%-9s%-10s%-7s%-10s\n",
		   "MAC", "MODE", "AID", "WCID", "BSS", "PSM",
		   "WMM", "MIMOPS", "RSSI0/1/2/3", "PhMd(T/R)", "BW(T/R)", "MCS(T/R)", "SGI(T/R)",
		   "STBC(T/R)", "Idle", "Rate(T/R)"));
#endif /* CONFIG_HOTSPOT_R2 */
#ifdef MWDS
	printk("%-8s", "MWDSCap");
#endif /* MWDS */

	for (i = 0; VALID_UCAST_ENTRY_WCID(pAd, i); i++) {
		PMAC_TABLE_ENTRY pEntry = &pAd->MacTab.Content[i];

		if ((ent_type == ENTRY_NONE)) {
			/* dump all MacTable entries */
			if (pEntry->EntryType == ENTRY_NONE)
				continue;
		} else {
			/* dump MacTable entries which match the EntryType */
			if (pEntry->EntryType != ent_type)
				continue;

			if ((IS_ENTRY_CLIENT(pEntry) || IS_ENTRY_PEER_AP(pEntry) || IS_ENTRY_REPEATER(pEntry))
				&& (pEntry->Sst != SST_ASSOC))
				continue;

#ifdef MAC_REPEATER_SUPPORT

			if (bReptCli == FALSE) {
				/* only dump the apcli entry which not a RepeaterCli */
				if (IS_REPT_LINK_UP(pEntry->pReptCli))
					continue;
			}

#endif /* MAC_REPEATER_SUPPORT */
		}

		if (IS_ENTRY_CLIENT(pEntry))
			sta_cnt++;

		if (IS_ENTRY_PEER_AP(pEntry))
			apcli_cnt++;

		if (IS_ENTRY_REPEATER(pEntry))
			rept_cnt++;

		addht = wlan_operate_get_addht(pEntry->wdev);
#ifdef DOT11_N_SUPPORT
		printk("HT Operating Mode : %d\n", addht->AddHtInfo2.OperaionMode);
		printk("\n");
#endif /* DOT11_N_SUPPORT */
		DataRate = 0;
		getRate(pEntry->HTPhyMode, &DataRate);
		printk("%02X:%02X:%02X:%02X:%02X:%02X  ", PRINT_MAC(pEntry->Addr));

		for (j = 0; type_str_map[j].type != 0; j++) {
			if (type_str_map[j].type == pEntry->EntryType) {
				MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF,
					("%-6s", type_str_map[j].str));
				break;
			}
		}
		printk("%-5d", (int)pEntry->Aid);
		printk("%-7d", (int)pEntry->wcid);
		printk("%-4d", (int)pEntry->func_tb_idx);
		printk("%-4d", (int)pEntry->PsMode);
		printk("%-4d", (int)CLIENT_STATUS_TEST_FLAG(pEntry, fCLIENT_STATUS_WMM_CAPABLE));
#ifdef DOT11_N_SUPPORT
		printk("%-7d", (int)pEntry->MmpsMode);
#endif /* DOT11_N_SUPPORT */
		rtmp_get_rssi(pAd, pEntry->wcid, rssi, 4);
		snprintf(tmp_str, temp_str_len, "%d/%d/%d/%d",
			rssi[0], rssi[1], rssi[2], rssi[3]);
		printk("%-20s", tmp_str);
#ifdef RACTRL_FW_OFFLOAD_SUPPORT

		if (cap->fgRateAdaptFWOffload == TRUE/*&& (pEntry->bAutoTxRateSwitch == TRUE)*/) {
			UCHAR phy_mode, rate, bw, sgi, stbc;
			UCHAR phy_mode_r, rate_r, bw_r, sgi_r, stbc_r;
			UCHAR nss;
			UCHAR nss_r;
			UINT32 RawData;
			UINT32 lastTxRate = pEntry->LastTxRate;
			UINT32 lastRxRate = pEntry->LastRxRate;
			UCHAR ucBand = HcGetBandByWdev(pEntry->wdev);

			EXT_EVENT_TX_STATISTIC_RESULT_T rTxStatResult;
			EXT_EVENT_PHY_STATE_RX_RATE rRxStatResult;
			HTTRANSMIT_SETTING LastTxRate;
			HTTRANSMIT_SETTING LastRxRate;

			MtCmdGetTxStatistic(pAd, GET_TX_STAT_ENTRY_TX_RATE, 0/*Don't Care*/, pEntry->wcid, &rTxStatResult);
			LastTxRate.field.MODE = rTxStatResult.rEntryTxRate.MODE;
			LastTxRate.field.BW = rTxStatResult.rEntryTxRate.BW;
			LastTxRate.field.ldpc = rTxStatResult.rEntryTxRate.ldpc ? 1 : 0;
			LastTxRate.field.ShortGI = rTxStatResult.rEntryTxRate.ShortGI ? 1 : 0;
			LastTxRate.field.STBC = rTxStatResult.rEntryTxRate.STBC;

			if (LastTxRate.field.MODE >= MODE_VHT)
				LastTxRate.field.MCS = (((rTxStatResult.rEntryTxRate.VhtNss - 1) & 0x3) << 4) + rTxStatResult.rEntryTxRate.MCS;
			else if (LastTxRate.field.MODE == MODE_OFDM)
				LastTxRate.field.MCS = getLegacyOFDMMCSIndex(rTxStatResult.rEntryTxRate.MCS) & 0x0000003F;
			else
				LastTxRate.field.MCS = rTxStatResult.rEntryTxRate.MCS;

			lastTxRate = (UINT32)(LastTxRate.word);
			LastRxRate.word = (USHORT)lastRxRate;
			RawData = lastTxRate;
			phy_mode = (RawData >> 13) & 0x7;
			rate = RawData & 0x3F;
			bw = (RawData >> 7) & 0x3;
			sgi = rTxStatResult.rEntryTxRate.ShortGI;
			stbc = ((RawData >> 10) & 0x1);
			nss = rTxStatResult.rEntryTxRate.VhtNss;

			MtCmdPhyGetRxRate(pAd, CMD_PHY_STATE_CONTENTION_RX_PHYRATE, ucBand, pEntry->wcid, &rRxStatResult);
			LastRxRate.field.MODE = rRxStatResult.u1RxMode;
			LastRxRate.field.BW = rRxStatResult.u1BW;
			LastRxRate.field.ldpc = rRxStatResult.u1Coding;
			LastRxRate.field.ShortGI = rRxStatResult.u1Gi ? 1 : 0;
			LastRxRate.field.STBC = rRxStatResult.u1Stbc;

			if (LastRxRate.field.MODE >= MODE_VHT)
				LastRxRate.field.MCS = ((rRxStatResult.u1RxNsts & 0x3) << 4) + rRxStatResult.u1RxRate;
			else if (LastRxRate.field.MODE == MODE_OFDM)
				LastRxRate.field.MCS = getLegacyOFDMMCSIndex(rRxStatResult.u1RxRate & 0xF);
			else
				LastRxRate.field.MCS = rRxStatResult.u1RxRate;

			phy_mode_r = rRxStatResult.u1RxMode;
			rate_r = rRxStatResult.u1RxRate & 0x3F;
			bw_r = rRxStatResult.u1BW;
			sgi_r = rRxStatResult.u1Gi;
			stbc_r = rRxStatResult.u1Stbc;

			snprintf(tmp_str, temp_str_len, "%s/%s", get_phymode_str(phy_mode), get_phymode_str(phy_mode_r));
			printk("%-12s", tmp_str);
			snprintf(tmp_str, temp_str_len, "%s/%s", get_bw_str(bw), get_bw_str(bw_r));
			printk("%-9s", tmp_str);
#ifdef DOT11_VHT_AC

			if (phy_mode >= MODE_VHT) {
				rate = rate & 0xF;
				snprintf(tmp_str, temp_str_len, "%dS-M%d/", nss, rate);
			} else
#endif /* DOT11_VHT_AC */
				snprintf(tmp_str, temp_str_len, "%d/", rate);

#ifdef DOT11_VHT_AC

			if (phy_mode_r >= MODE_VHT) {
				nss_r = (rRxStatResult.u1RxNsts + 1) / (rRxStatResult.u1Stbc + 1);
				rate_r = rate_r & 0xF;
				snprintf(tmp_str + strlen(tmp_str), temp_str_len - strlen(tmp_str), "%dS-M%d", nss_r, rate_r);
			} else
#endif /* DOT11_VHT_AC */
#if DOT11_N_SUPPORT
				if (phy_mode_r >= MODE_HTMIX)
					snprintf(tmp_str + strlen(tmp_str), temp_str_len - strlen(tmp_str), "%d", rate_r);
				else
#endif
					if (phy_mode_r == MODE_OFDM) {
						rate_r = rate_r & 0xF;
						if (rate_r == TMI_TX_RATE_OFDM_6M)
							LastRxRate.field.MCS = 0;
						else if (rate_r == TMI_TX_RATE_OFDM_9M)
							LastRxRate.field.MCS = 1;
						else if (rate_r == TMI_TX_RATE_OFDM_12M)
							LastRxRate.field.MCS = 2;
						else if (rate_r == TMI_TX_RATE_OFDM_18M)
							LastRxRate.field.MCS = 3;
						else if (rate_r == TMI_TX_RATE_OFDM_24M)
							LastRxRate.field.MCS = 4;
						else if (rate_r == TMI_TX_RATE_OFDM_36M)
							LastRxRate.field.MCS = 5;
						else if (rate_r == TMI_TX_RATE_OFDM_48M)
							LastRxRate.field.MCS = 6;
						else if (rate_r == TMI_TX_RATE_OFDM_54M)
							LastRxRate.field.MCS = 7;
						else
							LastRxRate.field.MCS = 0;

						snprintf(tmp_str + strlen(tmp_str), temp_str_len - strlen(tmp_str), "%d", LastRxRate.field.MCS);
					} else if (phy_mode_r == MODE_CCK) {
						rate_r = rate_r & 0x7;
						if (rate_r == TMI_TX_RATE_CCK_1M_LP)
							LastRxRate.field.MCS = 0;
						else if (rate_r == TMI_TX_RATE_CCK_2M_LP)
							LastRxRate.field.MCS = 1;
						else if (rate_r == TMI_TX_RATE_CCK_5M_LP)
							LastRxRate.field.MCS = 2;
						else if (rate_r == TMI_TX_RATE_CCK_11M_LP)
							LastRxRate.field.MCS = 3;
						else if (rate_r == TMI_TX_RATE_CCK_2M_SP)
							LastRxRate.field.MCS = 1;
						else if (rate_r == TMI_TX_RATE_CCK_5M_SP)
							LastRxRate.field.MCS = 2;
						else if (rate_r == TMI_TX_RATE_CCK_11M_SP)
							LastRxRate.field.MCS = 3;
						else
							LastRxRate.field.MCS = 0;

						snprintf(tmp_str + strlen(tmp_str), temp_str_len - strlen(tmp_str), "%d", LastRxRate.field.MCS);
					}

			printk("%-14s", tmp_str);
			snprintf(tmp_str, temp_str_len, "%d/%d", sgi, sgi_r);
			printk("%-7s", tmp_str);
			snprintf(tmp_str, temp_str_len, "%d/%d",  stbc, stbc_r);
			printk("%-10s", tmp_str);

			if (phy_mode >= MODE_HE) {
				get_rate_he((rate & 0xf), bw, nss, 0, &DataRate);
				if (sgi == 1)
					DataRate = (DataRate * 967) >> 10;
				if (sgi == 2)
					DataRate = (DataRate * 870) >> 10;
			} else {
				getRate(LastTxRate, &DataRate);
			}

			if (phy_mode_r >= MODE_HE) {
				get_rate_he((rate_r & 0xf), bw_r, nss_r, 0, &DataRate_r);
				if (sgi_r == 1)
					DataRate_r = (DataRate_r * 967) >> 10;
				if (sgi_r == 2)
					DataRate_r = (DataRate_r * 870) >> 10;
			} else {
				getRate(LastRxRate, &DataRate_r);
			}
		} else
#endif /* RACTRL_FW_OFFLOAD_SUPPORT */
		{
			printk("%-12s", get_phymode_str(pEntry->HTPhyMode.field.MODE));
			printk("%-9s", get_bw_str(pEntry->HTPhyMode.field.BW));
#ifdef DOT11_VHT_AC

			if (pEntry->HTPhyMode.field.MODE >= MODE_VHT)
				snprintf(tmp_str, temp_str_len, "%dS-M%d", ((pEntry->HTPhyMode.field.MCS >> 4) + 1),
						 (pEntry->HTPhyMode.field.MCS & 0xf));
			else
#endif /* DOT11_VHT_AC */
				snprintf(tmp_str, temp_str_len, "%d", pEntry->HTPhyMode.field.MCS);

			printk("%-12s", tmp_str);
			printk("%-9d", pEntry->HTPhyMode.field.ShortGI);
			printk("%-10d", pEntry->HTPhyMode.field.STBC);
		}

		printk("%-7d", (int)(pEntry->StaIdleTimeout - pEntry->NoDataIdleCount));
		snprintf(tmp_str, temp_str_len, "%d/%d", (int)DataRate, (int)DataRate_r);
		printk("%-10s", tmp_str);
#ifdef CONFIG_HOTSPOT_R2
		printk("%-7d", (int)pEntry->QosMapSupport);
#endif
		printk("%-10d, %d, %d%%\n", pEntry->DebugFIFOCount, pEntry->DebugTxCount,
			   (pEntry->DebugTxCount) ? ((pEntry->DebugTxCount - pEntry->DebugFIFOCount) * 100 / pEntry->DebugTxCount) : 0);
#ifdef CONFIG_HOTSPOT_R2

		if (pEntry->QosMapSupport) {
			int k = 0;

			printk("DSCP Exception:\n");

			for (k = 0; k < pEntry->DscpExceptionCount / 2; k++)
				printk("[Value: %4d] [UP: %4d]\n", pEntry->DscpException[k] & 0xff, (pEntry->DscpException[k] >> 8) & 0xff);

			printk("DSCP Range:\n");

			for (k = 0; k < 8; k++)
				printk("[UP :%3d][Low Value: %4d] [High Value: %4d]\n", k, pEntry->DscpRange[k] & 0xff,
					   (pEntry->DscpRange[k] >> 8) & 0xff);
		}

#endif
#ifdef MWDS

		if (IS_ENTRY_PEER_AP(pEntry)) {
			if (pEntry->func_tb_idx < MAX_APCLI_NUM) {
				if (pAd->StaCfg[pEntry->func_tb_idx].MlmeAux.bSupportMWDS)
					printk("%-8s", "YES");
				else
					printk("%-8s", "NO");
			}
		} else {
			if (pEntry->bSupportMWDS)
				printk("%-8s", "YES");
			else
				printk("%-8s", "NO");
		}

#endif /* MWDS */
		/* +++Add by shiang for debug */
		printk("%69s%-12s", "MaxCap:", get_phymode_str(pEntry->MaxHTPhyMode.field.MODE));
		printk("%-9s", get_bw_str(pEntry->MaxHTPhyMode.field.BW));
#ifdef DOT11_VHT_AC

		if (pEntry->MaxHTPhyMode.field.MODE >= MODE_VHT)
			snprintf(tmp_str, temp_str_len, "%dS-M%d", ((pEntry->MaxHTPhyMode.field.MCS >> 4) + 1),
					 (pEntry->MaxHTPhyMode.field.MCS & 0xf));
		else
#endif /* DOT11_VHT_AC */
			snprintf(tmp_str, temp_str_len, "%d", pEntry->MaxHTPhyMode.field.MCS);

		printk("%-12s", tmp_str);
		printk("%-9d", pEntry->MaxHTPhyMode.field.ShortGI);
		printk("%-10d", pEntry->MaxHTPhyMode.field.STBC);
		if (pEntry->MaxHTPhyMode.field.MODE >= MODE_HE)
			get_rate_he((pEntry->MaxHTPhyMode.field.MCS & 0xf), pEntry->MaxHTPhyMode.field.BW,
						((pEntry->MaxHTPhyMode.field.MCS >> 4) & 0x3) + 1, 0, &max_DataRate);
		else
			getRate(pEntry->MaxHTPhyMode, &max_DataRate);
		printk("%-7s", "-");
		printk("%-10d", (int)max_DataRate);
#ifdef HTC_DECRYPT_IOT
		printk("%20s%-10d", "HTC_ICVErr:", pEntry->HTC_ICVErrCnt);
		printk("%20s%-10s", "HTC_AAD_OM_Force:", pEntry->HTC_AAD_OM_Force ? "YES" : "NO");
#endif /* HTC_DECRYPT_IOT */
		printk("  wdev%d\n", (int)pEntry->wdev->wdev_idx);
		/* ---Add by shiang for debug */
		printk("\n");
	}

	printk("sta_cnt=%d\n\r", sta_cnt);
	printk("apcli_cnt=%d\n\r", apcli_cnt);
	printk("rept_cnt=%d\n\r", rept_cnt);
#ifdef OUI_CHECK_SUPPORT
	printk("oui_mgroup=%d\n\r", pAd->MacTab.oui_mgroup_cnt);
	printk("repeater_wcid_error_cnt=%d\n\r", pAd->MacTab.repeater_wcid_error_cnt);
	printk("repeater_bm_wcid_error_cnt=%d\n\r", pAd->MacTab.repeater_bm_wcid_error_cnt);
#endif /*OUI_CHECK_SUPPORT*/
#ifdef HTC_DECRYPT_IOT
	printk("HTC_ICV_Err_TH=%d\n\r", pAd->HTC_ICV_Err_TH);
#endif /* HTC_DECRYPT_IOT */
	return TRUE;
}

INT Show_MacTable_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UINT32 ent_type = ENTRY_CLIENT;
	INT ret = TRUE;
	RTMP_STRING *rate_str = NULL;
	RTMP_STRING *psm_str = NULL;
	RTMP_STRING *adv_str = NULL;
	RTMP_STRING *bss_str = NULL;
	CHAR *pch = NULL;
	UINT32 check_aid = 0;
	UINT32 check_bssidx = 0;

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s(): arg=%s\n", __func__, (arg == NULL ? "" : arg)));

	if (arg && strlen(arg)) {
		if (rtstrcasecmp(arg, "sta") == TRUE)
			ent_type = ENTRY_CLIENT;
		else if (rtstrcasecmp(arg, "ap") == TRUE)
			ent_type = ENTRY_AP;
		else
			ent_type = ENTRY_NONE;

		rate_str = strstr(arg, "rate:");
		psm_str = strstr(arg, "psm:");
		adv_str = strstr(arg, "adv:");
		bss_str = strstr(arg, "bss:");
		if (rate_str) {
			pch = strchr(rate_str, ':');
			check_aid = (UINT32)os_str_tol(pch + 1, 0, 10);

			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
				("%s check_aid:%d RATE\n", __func__, check_aid));
			if (check_aid)
				ret = entrytb_traversal(pAd, traversal_func_dump_entry_rate_by_aid, (void *)&check_aid);

			return ret;
		}

		if (psm_str) {
			pch = strchr(psm_str, ':');
			check_aid = (UINT32)os_str_tol(pch + 1, 0, 10);

			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
				("%s check_aid:%d PSM\n", __func__, check_aid));
			if (check_aid)
				ret = entrytb_traversal(pAd, traversal_func_dump_entry_psm_by_aid, (void *)&check_aid);

			return ret;
		}

		if (bss_str) {
			entrytb_bss_idx_search_t bss_search;

			os_zero_mem(&bss_search, sizeof(entrytb_bss_idx_search_t));
			pch = strchr(bss_str, ':');

			if (pch)
				check_bssidx = (UINT32)os_str_tol(pch + 1, 0, 10);
			else {
				MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
					("%s bss_str parse fail\n", __func__));
				return ret;
			}

			bss_search.bss_idx = check_bssidx;
			bss_search.need_print_field_name = 1;

			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
				("%s check_bssidx:%d associated entries\n", __func__, check_bssidx));
			ret = entrytb_traversal(pAd, traversal_func_dump_entry_associated_to_bss, (void *)&bss_search);

			return ret;
		}
	}

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("Dump MacTable entries info, EntType=0x%x\n", ent_type));
	return dump_mac_table(pAd, ent_type, FALSE);
}

INT Show_Mib_Info_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd) {
		struct wifi_dev *wdev = NULL;
		POS_COOKIE pObj = NULL;
		UCHAR BandIdx = 0;

		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
			("show mib info statistic:\n"));
		pObj = (POS_COOKIE) pAd->OS_Cookie;

		if (pObj == NULL) {
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
				("pObj is NULL\n"));
			return FALSE;
		}

		if (arg != NULL && strlen(arg)) {
			pAd->partial_mib_show_en = os_str_tol(arg, 0, 10);
		} else {
			goto err;
		}

		if (pAd->partial_mib_show_en == 1) {
			wdev = &pAd->ApCfg.MBSSID[pObj->ioctl_if].wdev;
			BandIdx = HcGetBandByWdev(wdev);

			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
				("%s RX FCS Error Count         = %d\n", __func__,
				pAd->WlanCounters[BandIdx].RxFcsErrorCount.u.LowPart));
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
				("%s RX FIFO Overflow Count     = %d\n", __func__,
				pAd->WlanCounters[BandIdx].RxFifoFullCount.u.LowPart));
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
				("%s RX MPDU Count              = %ld\n", __func__,
				(ULONG)pAd->WlanCounters[BandIdx].RxMpduCount.QuadPart));
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
				("%s Channel Idle Count         = %ld\n", __func__,
				(ULONG)pAd->WlanCounters[BandIdx].ChannelIdleCount.QuadPart));
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
				("%s CCA NAV TX Time            = %ld\n", __func__,
				(ULONG)pAd->WlanCounters[BandIdx].CcaNavTxTime.QuadPart));
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
				("%s RX MDRDY Count             = %ld\n", __func__,
				(ULONG)pAd->WlanCounters[BandIdx].RxMdrdyCount.QuadPart));
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
				("%s S CCA Time                 = %ld\n", __func__,
				(ULONG)pAd->WlanCounters[BandIdx].SCcaTime.QuadPart));
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
				("%s P ED Time                  = %ld\n", __func__,
				(ULONG)pAd->WlanCounters[BandIdx].PEdTime.QuadPart));
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
				("%s RX Total Byte Count        = %ld\n", __func__,
				(ULONG)pAd->WlanCounters[BandIdx].RxTotByteCount.QuadPart));
		}
	}
#endif /* CONFIG_AP_SUPPORT */

	return TRUE;

#ifdef CONFIG_AP_SUPPORT
err:
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF,
		("invalid input, should be enable of disable show mib\n"));
	return TRUE;
#endif /* CONFIG_AP_SUPPORT */
}

#ifdef ACL_BLK_COUNT_SUPPORT
INT Show_ACLRejectCount_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
	{
		POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
		UCHAR apidx = pObj->ioctl_if;

		if (arg && strlen(arg)) {
			if (rtstrcasecmp(arg, "1") == TRUE) {
				int count;

				if (pAd->ApCfg.MBSSID[apidx].AccessControlList.Policy == 2) {
					MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
							("ACL: Policy=%lu(0:Dis,1:White,2:Black),ACL: Num=%lu\n",
							pAd->ApCfg.MBSSID[apidx].AccessControlList.Policy,
							pAd->ApCfg.MBSSID[apidx].AccessControlList.Num));
					for (count = 0; count < pAd->ApCfg.MBSSID[apidx].AccessControlList.Num; count++) {
						MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
						("MAC:%02x:%02x:%02x:%02x:%02x:%02x , Reject_Count: %lu\n",
						PRINT_MAC(pAd->ApCfg.MBSSID[apidx].AccessControlList.Entry[count].Addr),
						pAd->ApCfg.MBSSID[apidx].AccessControlList.Entry[count].Reject_Count));
					}
				} else {
					MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
						("ERROR:Now Policy=%lu(0:Disable,1:White List,2:Black List)\n",
						pAd->ApCfg.MBSSID[apidx].AccessControlList.Policy));
				}
			}
		}
		return TRUE;
	}
#endif/*ACL_BLK_COUNT_SUPPORT*/

INT Show_PSTable_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UINT32 ent_type = ENTRY_CLIENT;
	struct _RTMP_CHIP_DBG *chip_dbg = hc_get_chip_dbg(pAd->hdev_ctrl);

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s(): arg=%s\n", __func__, (arg == NULL ? "" : arg)));

	if (arg && strlen(arg)) {
		if (rtstrcasecmp(arg, "sta") == TRUE)
			ent_type = ENTRY_CLIENT;
		else if (rtstrcasecmp(arg, "ap") == TRUE)
			ent_type = ENTRY_AP;
		else
			ent_type = ENTRY_NONE;
	}

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("Dump MacTable entries info, EntType=0x%x\n", ent_type));
	if (chip_dbg->dump_ps_table)
		return chip_dbg->dump_ps_table(pAd->hdev_ctrl, ent_type, FALSE);
	else
		return FALSE;
}

INT Show_BaTable_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	CHAR *tok;
	ULONG first_index, second_index;

	tok = rstrtok(arg, ":");

	if (tok) {
		first_index = os_str_toul(tok, NULL, 10);

		switch (first_index) {
		case 0:
			while (tok) {
				tok = rstrtok(NULL, ":");
				if (tok) {
					second_index = os_str_toul(tok, NULL, 10);
					ba_resource_dump_all(pAd, second_index);
				} else {
					ba_resource_dump_all(pAd, 0);
				}
			}
			break;
		case 1:
			ba_reordering_resource_dump_all(pAd);
			break;
		case 2:
			while (tok) {
				tok = rstrtok(NULL, ":");
				if (tok) {
					second_index = os_str_toul(tok, NULL, 10);
					ba_reodering_resource_dump(pAd, second_index);
				}
			}
			break;
		}
	} else {
		ba_resource_dump_all(pAd, 0);
	}

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("Dump BaTable info arg = %s\n", arg));
	return TRUE;
}

#ifdef DFS_VENDOR10_CUSTOM_FEATURE
INT show_client_idle_time(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UCHAR i = 0;

	if (IS_SUPPORT_V10_DFS(pAd) == FALSE)
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s(): Feature Not Supported\n", __func__));
	else
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s(): Client Idle Time\n", __func__));

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("==============================================\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("WCID Client MAC Address   Idle Period(seconds)\n"));

	for (i = 1; VALID_UCAST_ENTRY_WCID(pAd, i); i++) {
		PMAC_TABLE_ENTRY pEntry = &pAd->MacTab.Content[i];

		if (IS_ENTRY_CLIENT(pEntry) || IS_ENTRY_APCLI(pEntry) || IS_ENTRY_REPEATER(pEntry)) {
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%d    ", pEntry->wcid));
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
				("%02X:%02X:%02X:%02X:%02X:%02X ", PRINT_MAC(pEntry->Addr)));
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("      %ld\n", pEntry->LastRxTimeCount));
		}
	}
	return TRUE;
}
#endif

#ifdef MT_MAC
INT show_wtbl_proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	INT i, start, end, idx = -1;
	/* WTBL_ENTRY wtbl_entry; */

	asic_dump_wtbl_base_info(pAd);

	if (arg == NULL)
		return TRUE;

	MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s(): arg=%s\n", __func__, (arg == NULL ? "" : arg)));

	if (strlen(arg)) {
		idx = os_str_toul(arg, NULL, 10);
		start = end = idx;
	} else {
		start = 0;
		end = pAd->mac_ctrl.wtbl_entry_cnt[0] - 1;
	}

	MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("Dump WTBL entries info, start=%d, end=%d, idx=%d\n",
			 start, end, idx));

	for (i = start; i <= end; i++) {
		asic_dump_wtbl_info(pAd, i);
	}

	return TRUE;
}

INT show_wtbltlv_proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	CHAR *Param;
	UINT16 u2Wcid = 0;
	UCHAR ucCmdId = 0;
	UCHAR ucAction = 0;
	union _wtbl_debug_u debug_u;

	NdisZeroMemory(&debug_u, sizeof(union _wtbl_debug_u));

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
			 ("%s::param=%s\n", __func__, arg));

	if (arg == NULL)
		goto error;

	Param = rstrtok(arg, ":");

	if (Param != NULL)
		u2Wcid = os_str_tol(Param, 0, 10);
	else
		goto error;

	Param = rstrtok(NULL, ":");

	if (Param != NULL)
		ucCmdId = os_str_tol(Param, 0, 10);
	else
		goto error;

	Param = rstrtok(NULL, ":");

	if (Param != NULL)
		ucAction = os_str_tol(Param, 0, 10);
	else
		goto error;

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
			 ("%s():Wcid(%d), CmdId(%d), Action(%d)\n",  __func__, u2Wcid, ucCmdId, ucAction));
	mt_wtbltlv_debug(pAd, u2Wcid, ucCmdId, ucAction, &debug_u);
	return TRUE;
error:
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
			 ("%s: param = %s not correct\n", __func__, arg));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
			 ("%s: iwpriv ra0 show wtbltlv=Wcid,CmdId,Action\n", __func__));
	return 0;
}

INT show_amsdu_proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	struct _RTMP_CHIP_DBG *chip_dbg = hc_get_chip_dbg(pAd->hdev_ctrl);
	struct tx_rx_ctl *tr_ctl = &pAd->tr_ctl;
#ifdef DBG_AMSDU
	UINT8 slot_index;
	UINT32 i;
	STA_TR_ENTRY *tr_entry = NULL;
	MAC_TABLE_ENTRY *mac_table_entry = NULL;
#endif

	if (tr_ctl->amsdu_type == TX_SW_AMSDU) {
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("TX AMSDU Usage\n"));
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\tTimeSlot \tamsdu_1 \tamsdu_2 \tamsdu_3 \tamsdu_4\t\
									amsdu_5\t amsdu_6\t amsdu_7\t amsdu_8\n"));
#ifdef DBG_AMSDU
		for (i = 0; IS_TR_WCID_VALID(pAd, i); i++) {
			tr_entry = &tr_ctl->tr_entry[i];
			mac_table_entry = &pAd->MacTab.Content[i];
			if (!IS_ENTRY_NONE(tr_entry)) {
				MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\ttr_entry index = %d, amsdu_limit_len_adjust = %d\n", i, mac_table_entry->amsdu_limit_len_adjust));
				for (slot_index = 0; slot_index < TIME_SLOT_NUMS; slot_index++) {
					MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t%d \t\t%d \t\t%d \t\t%d \t\t%d \t\t%d \t\t%d\
												\t\t%d \t\t%d\n", slot_index,
							tr_entry->amsdu_1_rec[slot_index],
							 tr_entry->amsdu_2_rec[slot_index],
							 tr_entry->amsdu_3_rec[slot_index],
							 tr_entry->amsdu_4_rec[slot_index],
							 tr_entry->amsdu_5_rec[slot_index],
							 tr_entry->amsdu_6_rec[slot_index],
							 tr_entry->amsdu_7_rec[slot_index],
							 tr_entry->amsdu_8_rec[slot_index]));
				}
			}
		}
#endif
	} else if (tr_ctl->amsdu_type == TX_HW_AMSDU) {
		if (chip_dbg->dump_ple_amsdu_count_info)
			return chip_dbg->dump_ple_amsdu_count_info(pAd->hdev_ctrl);
		else if (IS_ASIC_CAP(pAd, fASIC_CAP_HW_TX_AMSDU))
			asic_dump_dmac_amsdu_info(pAd);
	}

	return TRUE;
}

INT show_mib_proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	struct _RTMP_CHIP_DBG *chip_dbg = hc_get_chip_dbg(pAd->hdev_ctrl);

	if (chip_dbg->dump_mib_info)
		return chip_dbg->dump_mib_info(pAd->hdev_ctrl, arg);
	else
		return FALSE;
}

#ifdef DBDC_MODE
INT32 ShowDbdcProc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	HcShowBandInfo(pAd);
	return TRUE;
}
#endif

INT32 ShowChCtrl(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	HcShowChCtrlInfo(pAd);
	return TRUE;
}
#ifdef GREENAP_SUPPORT
INT32 ShowGreenAPProc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	HcShowGreenAPInfo(pAd);
	return TRUE;
}
#endif /* GREENAP_SUPPORT */

#ifdef PCIE_ASPM_DYM_CTRL_SUPPORT
INT32 show_pcie_aspm_dym_ctrl_cap_proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_OFF,
		("\tflag_pcie_aspm_dym_ctrl_cap=%d\n",
		get_pcie_aspm_dym_ctrl_cap(pAd)));

	return TRUE;
}
#endif /* PCIE_ASPM_DYM_CTRL_SUPPORT */

#ifdef DOT11_HE_AX
#ifdef WIFI_TWT_SUPPORT
INT32 show_twt_support_cap_proc(struct _RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UINT8 i = 0;
	struct wifi_dev *wdev = NULL;

#ifdef CONFIG_STA_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_STA(pAd) {
		for (i = 0; i < MAX_MULTI_STA; i++) {
			wdev = &pAd->StaCfg[i].wdev;
			if (wdev) {
				MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_OFF,
					("\t STA_%d, twt_support on wf_cfg=%d\n",
					i,
					wlan_config_get_he_twt_support(wdev)));
			}
		}
	}
#endif /* CONFIG_STA_SUPPORT */

#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd) {
		for (i = 0; i < pAd->ApCfg.BssidNum; i++) {
			wdev = &pAd->ApCfg.MBSSID[i].wdev;
			if (wdev) {
				MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_OFF,
					("\t AP_%d, twt_support on wf_cfg=%d\n",
					i,
					wlan_config_get_he_twt_support(wdev)));
			}
		}
	}
#endif /* CONFIG_AP_SUPPORT */

	return TRUE;
}
#endif /* WIFI_TWT_SUPPORT */
#endif /* DOT11_HE_AX */

static UINT16 txop_to_ms(UINT16 *txop_level)
{
	UINT16 ms = (*txop_level) >> 5;

	ms += ((*txop_level) & (1 << 4)) ? 1 : 0;
	return ms;
}

static void dump_txop_level(UINT16 *txop_level, UINT32 len)
{
	UINT32 prio;

	for (prio = 0; prio < len; prio++) {
		UINT16 ms = txop_to_ms(txop_level + prio);

		MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_OFF,
				 (" {%x:0x%x(%ums)} ", prio, *(txop_level + prio), ms));
	}

	MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\n"));
}

static void dump_tx_burst_info(struct _RTMP_ADAPTER *pAd)
{
	struct wifi_dev **wdev = pAd->wdev_list;
	EDCA_PARM *edca_param = NULL;
	UINT32 idx = 0;
	UCHAR wmm_idx = 0;
	UCHAR bss_idx = 0xff;

	MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_OFF,
			 ("[%s]\n", __func__));

	do {
		if (wdev[idx] == NULL)
			break;

		if (bss_idx != wdev[idx]->bss_info_argument.ucBssIndex) {
			edca_param = HcGetEdca(pAd, wdev[idx]);

			if (edca_param == NULL)
				break;

			wmm_idx = HcGetWmmIdx(pAd, wdev[idx]);
			MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_OFF,
					 ("<bss_%x>\n", wdev[idx]->bss_info_argument.ucBssIndex));
			MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_OFF,
					 (" |-[wmm_idx]: %x\n", wmm_idx));
			MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_OFF,
					 (" |-[bitmap]: %08x\n", wdev[idx]->bss_info_argument.prio_bitmap));
			MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_OFF,
					 (" |-[prio:level]:"));
			dump_txop_level(wdev[idx]->bss_info_argument.txop_level, MAX_PRIO_NUM);
			bss_idx = wdev[idx]->bss_info_argument.ucBssIndex;
		}

		MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_OFF, (" |---<wdev_%x>\n", idx));
		MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_OFF,
				 ("      |-[bitmap]: %08x\n", wdev[idx]->prio_bitmap));
		MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_OFF,
				 ("      |-[prio:level]:"));
		dump_txop_level(wdev[idx]->txop_level, MAX_PRIO_NUM);
		idx++;
	} while (idx < WDEV_NUM_MAX);
}

INT32 show_tx_burst_info(struct _RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	dump_tx_burst_info(pAd);
	return TRUE;
}

INT32 show_wifi_sys(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	wifi_sys_dump(pAd);
	return TRUE;
}

INT32 show_wmm_info(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	hc_show_edca_info(pAd->hdev_ctrl);
	return TRUE;
}

INT32 ShowTmacInfo(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	struct _RTMP_CHIP_DBG *chip_dbg = hc_get_chip_dbg(pAd->hdev_ctrl);

	if (chip_dbg->show_tmac_info)
		return chip_dbg->show_tmac_info(pAd->hdev_ctrl, arg);
	else
		return FALSE;
}

INT32 ShowAggInfo(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	struct _RTMP_CHIP_DBG *chip_dbg = hc_get_chip_dbg(pAd->hdev_ctrl);

	if (chip_dbg->show_agg_info)
		return chip_dbg->show_agg_info(pAd->hdev_ctrl, arg);
	else
		return FALSE;
}

INT32 ShowArbInfo(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	struct _RTMP_CHIP_DBG *chip_dbg = hc_get_chip_dbg(pAd->hdev_ctrl);

	if (chip_dbg->show_arb_info)
		return chip_dbg->show_arb_info(pAd->hdev_ctrl, arg);
	else
		return FALSE;
}

INT ShowManualTxOP(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UINT32 txop;

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("CURRENT: ManualTxOP = %d\n", pAd->CommonCfg.ManualTxop));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("       : bEnableTxBurst = %d\n", pAd->CommonCfg.bEnableTxBurst));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("       : MacTab.Size = %d\n", pAd->MacTab.Size));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("       : RDG_ACTIVE = %d\n", RTMP_TEST_FLAG(pAd,
			 fRTMP_ADAPTER_RDG_ACTIVE)));
	RTMP_IO_READ32(pAd->hdev_ctrl, TMAC_ACTXOPLR1, &txop);
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("       : AC0 TxOP = 0x%x\n", GET_AC0LIMIT(txop)));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("       : AC1 TxOP = 0x%x\n", GET_AC1LIMIT(txop)));
	return TRUE;
}

INT show_dmasch_proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	struct _RTMP_CHIP_DBG *chip_dbg = hc_get_chip_dbg(pAd->hdev_ctrl);

	if (chip_dbg->show_dmasch_info)
		return chip_dbg->show_dmasch_info(pAd->hdev_ctrl, arg);
	else
		return FALSE;
}

INT32 ShowPseInfo(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	struct _RTMP_CHIP_DBG *chip_dbg = hc_get_chip_dbg(pAd->hdev_ctrl);

	if (chip_dbg->show_pse_info)
		return chip_dbg->show_pse_info(pAd->hdev_ctrl, arg);
	else
		return FALSE;
}

INT32 ShowPseData(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	CHAR *Param;
	UINT8 StartFID, FrameNums;
	struct _RTMP_CHIP_DBG *chip_dbg = hc_get_chip_dbg(pAd->hdev_ctrl);

	Param = rstrtok(arg, ",");

	if (Param != NULL)
		StartFID = os_str_tol(Param, 0, 10);
	else
		goto error;

	Param = rstrtok(NULL, ",");

	if (Param != NULL)
		FrameNums = os_str_tol(Param, 0, 10);
	else
		goto error;

	if (chip_dbg->show_pse_data)
		return chip_dbg->show_pse_data(pAd->hdev_ctrl, StartFID, FrameNums);
	else
		return FALSE;

error:
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
			 ("%s: param = %s not correct\n", __func__, arg));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
			 ("%s: iwpriv ra0 show psedata=startfid,framenums\n", __func__));
	return 0;
}

INT ShowPLEInfo(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	struct _RTMP_CHIP_DBG *chip_dbg = hc_get_chip_dbg(pAd->hdev_ctrl);

	if (chip_dbg->show_ple_info)
		return chip_dbg->show_ple_info(pAd->hdev_ctrl, arg);
	else
		return FALSE;
}

INT show_drr_proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	struct _RTMP_CHIP_DBG *chip_dbg = hc_get_chip_dbg(pAd->hdev_ctrl);

	if (chip_dbg->show_drr_info)
		return chip_dbg->show_drr_info(pAd->hdev_ctrl, arg);
	else
		return FALSE;
}

INT show_TXD_proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UINT fid;

	if (arg == NULL)
		return FALSE;

	if (strlen(arg) == 0)
		return FALSE;

	fid = simple_strtol(arg, 0, 16);
	return ShowTXDInfo(pAd, fid);
}
#define UMAC_FID_FAULT	0xFFF
#define DUMP_MEM_SIZE 64
INT ShowTXDInfo(RTMP_ADAPTER *pAd, UINT fid)
{
	INT i = 0;
	UINT8 data[DUMP_MEM_SIZE];
	UINT32 Addr = 0;

	if (fid >= UMAC_FID_FAULT)
		return FALSE;

	os_zero_mem(data, DUMP_MEM_SIZE);
	Addr = 0xa << 28 | fid << 16; /* TXD addr: 0x{a}{fid}{0000}*/
	MtCmdMemDump(pAd, Addr, &data[0]);

	for (i = 0; i < DUMP_MEM_SIZE; i = i + 4)
		MTWF_LOG(DBG_CAT_FW, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("DW%02d: 0x%02x%02x%02x%02x\n", i / 4, data[i + 3], data[i + 2],
				 data[i + 1], data[i]));

	asic_dump_tmac_info(pAd, &data[0]);
	return TRUE;
}
INT show_mem_proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UINT addr = os_str_tol(arg, 0, 16);
	UINT8 data[DUMP_MEM_SIZE];
	INT i = 0;

	os_zero_mem(data, DUMP_MEM_SIZE);
	MtCmdMemDump(pAd, addr, &data[0]);

	for (i = 0; i < DUMP_MEM_SIZE; i = i + 4)
		MTWF_LOG(DBG_CAT_FW, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("addr 0x%08x: 0x%02x%02x%02x%02x\n", addr + i, data[i + 3],
				 data[i + 2], data[i + 1], data[i]));

	return TRUE;
}
INT show_protect_info(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	struct _RTMP_CHIP_DBG *chip_dbg = hc_get_chip_dbg(pAd->hdev_ctrl);

	if (chip_dbg->show_protect_info)
		return chip_dbg->show_protect_info(pAd->hdev_ctrl, arg);
	else
		return FALSE;
}

INT show_cca_info(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	struct _RTMP_CHIP_DBG *chip_dbg = hc_get_chip_dbg(pAd->hdev_ctrl);

	if (chip_dbg->show_cca_info)
		return chip_dbg->show_cca_info(pAd->hdev_ctrl, arg);
	else
		return FALSE;

}

#endif /*MT_MAC*/


INT Show_sta_tr_proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	INT idx;
	STA_TR_ENTRY *tr_entry;
	struct tx_rx_ctl *tr_ctl = &pAd->tr_ctl;

	for (idx = 0; IS_TR_WCID_VALID(pAd, idx); idx++) {
		tr_entry = &tr_ctl->tr_entry[idx];

		if (IS_VALID_ENTRY(tr_entry))
			TRTableEntryDump(pAd, idx, __func__, __LINE__);
	}

	return TRUE;
}

INT show_stainfo_proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	INT i;
	ULONG DataRate = 0, irqflags;
	UCHAR mac_addr[MAC_ADDR_LEN];
	RTMP_STRING *token;
	CHAR sep[1] = {':'};
	MAC_TABLE_ENTRY *pEntry = NULL;
	struct tx_rx_ctl *tr_ctl = &pAd->tr_ctl;
	STA_TR_ENTRY *tr_entry;

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s(): Input string=%s\n",
			 __func__, arg));

	for (i = 0, token = rstrtok(arg, &sep[0]); token; token = rstrtok(NULL, &sep[0]), i++) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s(): token(len=%zu) =%s\n",
				 __func__, strlen(token), token));

		if ((strlen(token) != 2) || (!isxdigit(*token)) || (!isxdigit(*(token + 1))))
			return FALSE;

		AtoH(token, (&mac_addr[i]), 1);
	}

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s(): i= %d\n", __func__, i));

	if (i != 6)
		return FALSE;

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\nAddr %02x:%02x:%02x:%02x:%02x:%02x\n",
			 PRINT_MAC(mac_addr)));
#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
	pEntry = MacTableLookup(pAd, (UCHAR *)mac_addr);
#endif
#ifdef CONFIG_STA_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
	pEntry = MacTableLookup2(pAd, (UCHAR *)mac_addr, NULL);
#endif

	if (!pEntry)
		return FALSE;

	if (IS_ENTRY_NONE(pEntry)) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("Invalid MAC address!\n"));
		return FALSE;
	}

	printk("\n");
	printk("EntryType : %d\n", pEntry->EntryType);
	printk("Entry Capability:\n");
	printk("\tPhyMode:%-10s\n", get_phymode_str(pEntry->MaxHTPhyMode.field.MODE));
	printk("\tBW:%-6s\n", get_bw_str(pEntry->MaxHTPhyMode.field.BW));
	printk("\tDataRate:\n");
#ifdef DOT11_VHT_AC

	if (pEntry->MaxHTPhyMode.field.MODE >= MODE_VHT)
		printk("%dS-M%d", ((pEntry->MaxHTPhyMode.field.MCS >> 4) + 1), (pEntry->MaxHTPhyMode.field.MCS & 0xf));
	else
#endif /* DOT11_VHT_AC */
		printk(" %-6d", pEntry->MaxHTPhyMode.field.MCS);

	printk(" %-6d", pEntry->MaxHTPhyMode.field.ShortGI);
	printk(" %-6d\n", pEntry->MaxHTPhyMode.field.STBC);
	printk("Entry Operation Features\n");
	printk("\t%-4s%-4s%-4s%-4s%-8s%-7s%-7s%-7s%-10s%-6s%-6s%-6s%-6s%-7s%-7s\n",
		   "AID", "BSS", "PSM", "WMM", "MIMOPS", "RSSI0", "RSSI1",
		   "RSSI2", "PhMd", "BW", "MCS", "SGI", "STBC", "Idle", "Rate");
	DataRate = 0;
	getRate(pEntry->HTPhyMode, &DataRate);
	printk("\t%-4d", (int)pEntry->Aid);
	printk("%-4d", (int)pEntry->func_tb_idx);
	printk("%-4d", (int)pEntry->PsMode);
	printk("%-4d", (int)CLIENT_STATUS_TEST_FLAG(pEntry, fCLIENT_STATUS_WMM_CAPABLE));
#ifdef DOT11_N_SUPPORT
	printk("%-8d", (int)pEntry->MmpsMode);
#endif /* DOT11_N_SUPPORT */
	printk("%-7d", pEntry->RssiSample.AvgRssi[0]);
	printk("%-7d", pEntry->RssiSample.AvgRssi[1]);
	printk("%-7d", pEntry->RssiSample.AvgRssi[2]);
	printk("%-10s", get_phymode_str(pEntry->HTPhyMode.field.MODE));
	printk("%-6s", get_bw_str(pEntry->HTPhyMode.field.BW));
#ifdef DOT11_VHT_AC

	if (pEntry->HTPhyMode.field.MODE >= MODE_VHT)
		printk("%dS-M%d", ((pEntry->HTPhyMode.field.MCS >> 4) + 1), (pEntry->HTPhyMode.field.MCS & 0xf));
	else
#endif /* DOT11_VHT_AC */
		printk("%-6d", pEntry->HTPhyMode.field.MCS);

	printk("%-6d", pEntry->HTPhyMode.field.ShortGI);
	printk("%-6d", pEntry->HTPhyMode.field.STBC);
	printk("%-7d", (int)(pEntry->StaIdleTimeout - pEntry->NoDataIdleCount));
	printk("%-7d", (int)DataRate);
	printk("%-10d, %d, %d%%\n", pEntry->DebugFIFOCount, pEntry->DebugTxCount,
		   (pEntry->DebugTxCount) ? ((pEntry->DebugTxCount - pEntry->DebugFIFOCount) * 100 / pEntry->DebugTxCount) : 0);
	printk("\n");
	ASSERT(pEntry->wcid <= GET_MAX_UCAST_NUM(pAd));
	tr_entry = &tr_ctl->tr_entry[pEntry->wcid];
	printk("Entry TxRx Info\n");
	printk("\tEntryType : %d\n", tr_entry->EntryType);
	printk("\tHookingWdev : %p\n", tr_entry->wdev);
	printk("\tIndexing : FuncTd=%d, WCID=%d\n", tr_entry->func_tb_idx, tr_entry->wcid);
	printk("Entry TxRx Features\n");
	printk("\tIsCached, PortSecured, PsMode, LockTx, VndAth\n");
	printk("\t%d\t%d\t%d\t%d\t%d\n", tr_entry->isCached, tr_entry->PortSecured,
		   tr_entry->PsMode, tr_entry->LockEntryTx,
		   tr_entry->bIAmBadAtheros);
	printk("\t%-6s%-6s%-6s%-6s%-6s%-6s%-6s\n", "TxQId", "PktNum", "QHead", "QTail", "EnQCap", "DeQCap", "PktSeq");

	for (i = 0; i < WMM_QUE_NUM;  i++) {
		RTMP_IRQ_LOCK(&tr_entry->txq_lock[i], irqflags);
		printk("\t%d %6d  %p  %6p %d %d %d\n",
			   i,
			   tr_entry->tx_queue[i].Number,
			   tr_entry->tx_queue[i].Head,
			   tr_entry->tx_queue[i].Tail,
			   tr_entry->enq_cap, tr_entry->deq_cap,
			   tr_entry->TxSeq[i]);
		RTMP_IRQ_UNLOCK(&tr_entry->txq_lock[i], irqflags);
	}

	RTMP_IRQ_LOCK(&tr_entry->ps_queue_lock, irqflags);
	printk("\tpsQ %6d  %p  %p %d %d  NoQ:%d\n",
		   tr_entry->ps_queue.Number,
		   tr_entry->ps_queue.Head,
		   tr_entry->ps_queue.Tail,
		   tr_entry->enq_cap, tr_entry->deq_cap,
		   tr_entry->NonQosDataSeq);
	RTMP_IRQ_UNLOCK(&tr_entry->ps_queue_lock, irqflags);
	printk("\n");
	return TRUE;
}

extern INT show_radio_info_proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT show_devinfo_proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UCHAR *pstr;

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("Device MAC\n"));

	if (pAd->OpMode == OPMODE_AP)
		pstr = "AP";
	else if (pAd->OpMode == OPMODE_STA)
		pstr = "STA";
	else
		pstr = "Unknown";

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("Operation Mode: %s\n", pstr));
	show_radio_info_proc(pAd, arg);
	return TRUE;
}

INT show_wdev_info(RTMP_ADAPTER *pAd, struct wifi_dev *wdev)
{
	INT idx;

	for (idx = 0; idx < WDEV_NUM_MAX; idx++) {
		if (pAd->wdev_list[idx] == wdev)
			break;
	}

	if (idx >= WDEV_NUM_MAX) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("ERR! Cannot found required wdev(%p)!\n", wdev));
		return FALSE;
	}

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("WDEV Instance(%d) Info:\n", idx));
	return TRUE;
}


CHAR *wdev_type_str[] = {"AP", "STA", "ADHOC", "WDS", "MESH", "GO", "GC", "APCLI", "REPEATER", "P2P_DEVICE", "SERVICE", "ATE", "Unknown"};

RTMP_STRING *wdev_type2str(int type)
{
	switch (type) {
	case WDEV_TYPE_AP:
		return wdev_type_str[0];

	case WDEV_TYPE_STA:
		return wdev_type_str[1];

	case WDEV_TYPE_ADHOC:
		return wdev_type_str[2];

	case WDEV_TYPE_WDS:
		return wdev_type_str[3];

	case WDEV_TYPE_MESH:
		return wdev_type_str[4];

	case WDEV_TYPE_GO:
		return wdev_type_str[5];

	case WDEV_TYPE_GC:
		return wdev_type_str[6];

	/*case WDEV_TYPE_APCLI:
		return wdev_type_str[7];*/

	case WDEV_TYPE_REPEATER:
		return wdev_type_str[8];

	case WDEV_TYPE_P2P_DEVICE:
		return wdev_type_str[9];

	case WDEV_TYPE_SERVICE_TXC:
	case WDEV_TYPE_SERVICE_TXD:
		return wdev_type_str[10];

	case WDEV_TYPE_ATE_AP:
	case WDEV_TYPE_ATE_STA:
		return wdev_type_str[11];

	default:
		return wdev_type_str[12];
	}
}


INT show_sysinfo_proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	INT idx;
	UINT32 total_size = 0, cntr_size;
	struct wifi_dev *wdev;
	UCHAR ext_cha;
	struct tx_rx_ctl *tr_ctl = &pAd->tr_ctl;
#ifdef CONFIG_STA_SUPPORT
	POS_COOKIE pObj = (POS_COOKIE)pAd->OS_Cookie;
	PSTA_ADMIN_CONFIG pStaCfg = &pAd->StaCfg[pObj->ioctl_if];
	BSS_TABLE *ScanTab = NULL;
	wdev = &pStaCfg->wdev;
	ScanTab = get_scan_tab_by_wdev(pAd, wdev);
#endif

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("Device Instance\n"));

	for (idx = 0; idx < WDEV_NUM_MAX; idx++) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\tWDEV %02d:", idx));

		if (pAd->wdev_list[idx]) {
			UCHAR *str = NULL;

			wdev = pAd->wdev_list[idx];
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\n\t\tName/Type:%s/%s\n",
					 RTMP_OS_NETDEV_GET_DEVNAME(wdev->if_dev),
					 wdev_type2str(wdev->wdev_type)));
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t\tWdev(list) Idx:%d\n", wdev->wdev_idx));
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t\tMacAddr:%02x:%02x:%02x:%02x:%02x:%02x\n",
					 PRINT_MAC(wdev->if_addr)));
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t\tBSSID:%02x:%02x:%02x:%02x:%02x:%02x\n",
					 PRINT_MAC(wdev->bssid)));
			str = wmode_2_str(wdev->PhyMode);

			if (str) {
				MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t\tPhyMode:%s\n", str));
				os_free_mem(str);
			}

			ext_cha = wlan_config_get_ext_cha(wdev);
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t\tChannel:%d,ExtCha:%d\n", wdev->channel, ext_cha));
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t\tPortSecured/ForbidTx: %d(%sSecured)/%lx\n",
					 wdev->PortSecured,
					 (wdev->PortSecured == WPA_802_1X_PORT_SECURED ? "" : "Not"),
					 wdev->forbid_data_tx));
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t\tEdcaIdx:%d\n", wdev->EdcaIdx));
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t\tif_dev:0x%p\tfunc_dev:[%d]0x%p\tsys_handle:0x%p\n",
					 wdev->if_dev, wdev->func_idx, wdev->func_dev, wdev->sys_handle));
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t\tIgmpSnoopEnable:%d\n", wdev->IgmpSnoopEnable));
#ifdef LINUX

			if (wdev->if_dev) {
				UINT idx, q_num;
				UCHAR *mac_str = RTMP_OS_NETDEV_GET_PHYADDR(wdev->if_dev);

				MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
						 ("\t\tOS NetDev status(%s[%d]-%02x:%02x:%02x:%02x:%02x:%02x):\n",
						  RtmpOsGetNetDevName(wdev->if_dev),
						  RtmpOsGetNetIfIndex(wdev->if_dev),
						  PRINT_MAC(mac_str)));
				MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t\t\tdev->state: 0x%lx\n", RtmpOSGetNetDevState(wdev->if_dev)));
				MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t\t\tdev->flag: 0x%x\n", RtmpOSGetNetDevFlag(wdev->if_dev)));
				q_num = RtmpOSGetNetDevQNum(wdev->if_dev);

				for (idx = 0; idx < q_num; idx++) {
					MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
							 ("\t\t\tdev->queue[%d].state: 0x%lx\n", idx,
							  RtmpOSGetNetDevQState(wdev->if_dev, idx)));
				}
			}

#endif /* LINUX */
		} else
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\n"));
	}

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("Memory Statistics:\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\tsize>\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t\tpAd = \t\t%zu bytes\n\n", sizeof(*pAd)));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t\t\tCommonCfg = \t%zu bytes\n", sizeof(pAd->CommonCfg)));
	total_size += sizeof(pAd->CommonCfg);
#ifdef CONFIG_AP_SUPPORT
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t\t\tApCfg = \t%zu bytes\n", sizeof(pAd->ApCfg)));
	total_size += sizeof(pAd->ApCfg);
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t\t\t\tMBSSID = \t%zu B (PerMBSS =%zu B, Total MBSS Num= %d)\n",
			 sizeof(pAd->ApCfg.MBSSID), sizeof(struct _BSS_STRUCT), MAX_BEACON_NUM));
#ifdef APCLI_SUPPORT
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
			 ("\t\t\t\t\tAPCLI = \t%zu bytes (PerAPCLI =%zu bytes, Total APCLI Num= %d)\n",
			  sizeof(pAd->StaCfg), sizeof(struct _STA_ADMIN_CONFIG), MAX_APCLI_NUM));
#endif /* APCLI_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */
#ifdef RTMP_MAC_PCI
{
	struct _PCI_HIF_T *hif = hc_get_hif_ctrl(pAd->hdev_ctrl);
	INT idx2;

	cntr_size = 0;
	for (idx = 0; idx < hif->tx_res_num; idx++) {
		struct hif_pci_tx_ring *tx_ring = pci_get_tx_ring_by_ridx(hif, idx);

		cntr_size += tx_ring->desc_ring.AllocSize;
		cntr_size += tx_ring->buf_space.AllocSize;
	}
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t\t\tTxRing = \t%d bytes\n", cntr_size));
	total_size += cntr_size;
	cntr_size = 0;
	for (idx = 0; idx < hif->rx_res_num; idx++) {
		struct hif_pci_rx_ring *rx_ring = pci_get_rx_ring_by_ridx(hif, idx);
		cntr_size += rx_ring->desc_ring.AllocSize;
		for (idx2 = 0; idx2 < rx_ring->ring_size; idx2++)
			cntr_size += rx_ring->Cell[idx2].DmaBuf.AllocSize;
	}
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t\t\tRxRing = \t%d bytes\n", cntr_size));
	total_size += cntr_size;
}
#endif /* RTMP_MAC_PCI */
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t\t\tMlme = \t%zu bytes\n", sizeof(pAd->Mlme)));
	total_size += sizeof(pAd->Mlme);
#ifdef CONFIG_STA_SUPPORT
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t\t\tMlmeAux = \t%zu bytes\n", sizeof(pStaCfg->MlmeAux)));
	total_size += sizeof(pStaCfg->MlmeAux);
#endif /* CONFIG_STA_SUPPORT */
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t\t\tMacTab = \t%zu bytes\n", sizeof(pAd->MacTab)));
	total_size += sizeof(pAd->MacTab);
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t\t\tBA Control = \t%zu bytes\n", sizeof(tr_ctl->ba_ctl)));
	total_size += sizeof(tr_ctl->ba_ctl);
	cntr_size = sizeof(pAd->Counters8023) + sizeof(pAd->WlanCounters) +
				sizeof(pAd->RalinkCounters) + /* sizeof(pAd->DrsCounters) */ +
				sizeof(pAd->PrivateInfo);
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t\t\tCounter** = \t%d bytes\n", cntr_size));
	total_size += cntr_size;
#ifdef CONFIG_STA_SUPPORT
#if defined(AP_SCAN_SUPPORT) || defined(CONFIG_STA_SUPPORT)
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t\t\tScanTab = \t%zu bytes\n", sizeof(ScanTab)));
	total_size += sizeof(ScanTab);
#endif
#endif
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\tsize> Total = \t\t%d bytes, Others = %zu bytes\n\n",
			 total_size, sizeof(*pAd) - total_size));
	return TRUE;
}

void wifi_dump_info(void)
{
	RTMP_ADAPTER *pAd = NULL;
	struct net_device *ndev = NULL;
	CHAR * dev_list[] = { "ra0", "rax0", "rai0", "rae0"};
	UCHAR idx = 0;
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s--------------------\n", __func__));
	for (idx = 0 ; idx < 4; idx++) {
		ndev = dev_get_by_name(&init_net, dev_list[idx]);

		if (ndev) {
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("--------%s--------\n", dev_list[idx]));
			pAd = ((struct mt_dev_priv *)netdev_priv(ndev))->sys_handle;
			show_tpinfo_host(pAd, WFDMA_INFO, 0, 0);
			show_tpinfo_host(pAd, COUNTER_INFO, FALSE, 0);
			show_trinfo_proc(pAd, "");
			ShowPLEInfo(pAd, "");
			ShowPseInfo(pAd, "");
			Show_PSTable_Proc(pAd, "");
			show_swqinfo(pAd, "");
#ifdef ERR_RECOVERY
			ShowSerProc2(pAd, "");
#endif
#ifdef FQ_SCH_SUPPORT
			show_fq_info(pAd, "");
#endif
		}
	}
}
EXPORT_SYMBOL(wifi_dump_info);

#ifdef CONFIG_TP_DBG
INT Set_TPDbg_Level(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UINT dbg;
	struct tp_debug *tp_dbg = &pAd->tr_ctl.tp_dbg;

	dbg = simple_strtol(arg, 0, 10);
	tp_dbg->debug_flag = dbg;
	if (!(dbg & TP_DEBUG_TIMING)) {
		memset(tp_dbg->TRDoneTimesRec, 0x0, sizeof(tp_dbg->TRDoneTimesRec));
		memset(tp_dbg->TRDoneInterval, 0x0, sizeof(tp_dbg->TRDoneInterval));
	}
	MTWF_PRINT("%s(): (TPDebugLevel = %d)\n", __func__, dbg);

	return TRUE;
}

INT show_TPDbg_info_proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UINT8 tp_dbg_slot_idx;
	UCHAR dump = 0;
	UCHAR dbg_detail_lvl = DBG_LVL_TRACE;
	UCHAR time_slot_num = TP_DBG_TIME_SLOT_NUMS;
	struct tp_debug *tp_dbg = &pAd->tr_ctl.tp_dbg;

	if (arg != NULL)
		dump = os_str_toul(arg, 0, 16);

	if (dump == 1)
		dbg_detail_lvl = DBG_LVL_OFF;
	else if (dump == 2)
		dbg_detail_lvl = DBG_LVL_TRACE;
	else if (dump == 3) {
		/* show less information for MSP debugging */
		dbg_detail_lvl = DBG_LVL_OFF;
		time_slot_num = TP_DBG_TIME_SLOT_NUMS/2;
	}

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, dbg_detail_lvl,
			 ("\n\tTimeSlot \tTxIsr \tRxIsr/Rx1Isr/RxDlyIsr \t\tTxIoRead/TxIoWrite \tRxIoRead/RxIoWrite \tRx1IoRead/Rx1IoWrite\n"));

	for (tp_dbg_slot_idx = 0; tp_dbg_slot_idx < time_slot_num; tp_dbg_slot_idx++) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, dbg_detail_lvl,
				 ("\t%d \t\t%d \t%5d/%6d/%8d \t\t%8d/%9d \t%8d/%9d \t%9d/%10d\n",
				tp_dbg_slot_idx, tp_dbg->IsrTxCntRec[tp_dbg_slot_idx],
				tp_dbg->IsrRxCntRec[tp_dbg_slot_idx],
				tp_dbg->IsrRx1CntRec[tp_dbg_slot_idx],
				tp_dbg->IsrRxDlyCntRec[tp_dbg_slot_idx],
				tp_dbg->IoReadTxRec[tp_dbg_slot_idx],
				tp_dbg->IoWriteTxRec[tp_dbg_slot_idx],
				tp_dbg->IoReadRxRec[tp_dbg_slot_idx],
				tp_dbg->IoWriteRxRec[tp_dbg_slot_idx],
				tp_dbg->IoReadRx1Rec[tp_dbg_slot_idx],
				tp_dbg->IoWriteRx1Rec[tp_dbg_slot_idx]));
	}

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, dbg_detail_lvl,
			("\n\tTimeSlot \tRx0Cnt_A/Cnt_B/Cnt_C/Cnt_D \t\tRx1Cnt_A/Cnt_B/Cnt_C/Cnt_D \t\tTRdone \t\tTRdoneInterval\n"));

	for (tp_dbg_slot_idx = 0; tp_dbg_slot_idx < time_slot_num; tp_dbg_slot_idx++) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, dbg_detail_lvl, ("\t%d \t\t%8d/%5d/%5d/%5d \t\t%8d/%5d/%5d/%5d \t\t%d \t\t%d\n",
				 tp_dbg_slot_idx, tp_dbg->MaxProcessCntRxRecA[tp_dbg_slot_idx],
				tp_dbg->MaxProcessCntRxRecB[tp_dbg_slot_idx],
				tp_dbg->MaxProcessCntRxRecC[tp_dbg_slot_idx],
				tp_dbg->MaxProcessCntRxRecD[tp_dbg_slot_idx],
				tp_dbg->MaxProcessCntRx1RecA[tp_dbg_slot_idx],
				tp_dbg->MaxProcessCntRx1RecB[tp_dbg_slot_idx],
				tp_dbg->MaxProcessCntRx1RecC[tp_dbg_slot_idx],
				tp_dbg->MaxProcessCntRx1RecD[tp_dbg_slot_idx],
				tp_dbg->TRDoneTimesRec[tp_dbg_slot_idx],
				tp_dbg->TRDoneInterval[tp_dbg_slot_idx]));
	}
	return TRUE;
}
#endif /* CONFIG_TP_DBG */

#ifdef CUT_THROUGH
static VOID show_tx_free_notify_host_info(RTMP_ADAPTER *pAd)
{
	UINT8 i, j;
	PKT_TOKEN_CB *cb = hc_get_ct_cb(pAd->hdev_ctrl);
	struct token_tx_pkt_queue *que = NULL;

	for (i = 0; i < cb->que_nums; i++) {
		que = &cb->que[i];
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, debug_lvl,
			("\tTX Token Que Index = %d\n", i));
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, debug_lvl,
			("\tTX Token Full Count = %d\n", que->token_full_cnt));
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, debug_lvl,
			("\tTX FreeToken Number = %d\n", atomic_read(&que->free_token_cnt)));
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, debug_lvl,
			("\tTX FreeToken LowMark = %d\n", que->low_water_mark));
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, debug_lvl,
			("\tTX FreeToken HighMark = %d\n", que->high_water_mark));
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, debug_lvl,
			("\tTX Token Total Enq Number = %d\n", que->total_enq_cnt));
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, debug_lvl,
			("\tTX Token Total Deq Number = %d\n", que->total_deq_cnt));
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, debug_lvl,
			("\tTX Token Total Back Number = %d\n", que->total_back_cnt));

		for (j = 0; j < TX_FREE_NOTIFY_DEEP_STAT_SIZE; j++) {
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, debug_lvl,
				("\tTX Free Notify deep_boundary(%d) = %d\n",
				que->deep_stat[j].boundary, que->deep_stat[j].cnt));
		}
	}
}
#endif

#ifdef RTMP_MAC_PCI
static VOID show_wfdma_info(RTMP_ADAPTER *pAd)
{
	UINT8 i;
	PCI_HIF_T *hif = hc_get_hif_ctrl(pAd->hdev_ctrl);
	UINT8 num_of_tx_ring = hif_get_tx_res_num(pAd->hdev_ctrl);
	struct tx_rx_ctl *tr_ctl = &pAd->tr_ctl;
	struct tr_flow_control *tr_flow_ctl = &tr_ctl->tr_flow_ctl;

	for (i = 0; i < num_of_tx_ring; i++) {
		struct hif_pci_tx_ring *tx_ring = pci_get_tx_ring_by_ridx(hif, i);

		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, debug_lvl,
			("\tRing %d TxRing LowMark = %d\n",
			i, tx_ring->tx_ring_low_water_mark));
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, debug_lvl,
			("\tRing %d TxRing HighMark = %d\n",
			i, tx_ring->tx_ring_high_water_mark));
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, debug_lvl,
			("\tRing %d TxRing State = %lu\n",
			i, tx_ring->tx_ring_state));
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, debug_lvl,
			("\tRing %d TxRingFull Count = %d\n",
			i, tx_ring->tx_ring_full_cnt));
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, debug_lvl,
			("\tTxFlowBlockState = %d\n",
			tr_flow_ctl->TxFlowBlockState[i]));
	}

}
#endif

static VOID show_counter_info(RTMP_ADAPTER *pAd, UINT32 en_rx_profiling)
{
	struct tx_rx_ctl *tr_ctl = &pAd->tr_ctl;
	struct tr_counter *tr_cnt = &tr_ctl->tr_cnt;
	UINT8 i, j;
	struct _RTMP_CHIP_OP *ops = hc_get_chip_ops(pAd->hdev_ctrl);

	/* TX */
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\nTX Counter\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\tamsdu type = %s\n", tr_ctl->amsdu_type ? "TX_HW_AMSDU" : "TX_SW_AMSDU"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\tnet_if_stop_cnt = %d\n", tr_cnt->net_if_stop_cnt));
	if (tr_cnt->net_if_stop_cnt != 0) {
		UCHAR band_idx = DBDC_BAND0;
		struct wifi_dev *wdev_block = NULL;

		for (band_idx = DBDC_BAND0; band_idx < DBDC_BAND_NUM; band_idx++) {
			DlListForEach(wdev_block, &pAd->fp_tx_flow_ctl.TxBlockDevList[band_idx], struct wifi_dev, tx_block_list) {
				if (wdev_block) {
					MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t\t(%s is stopped)\n", wdev_block->if_dev->name));
				}
			}
		}
	}
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\ttx_sw_dataq_drop = %d\n", tr_cnt->tx_sw_dataq_drop));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\ttx_sw_mgmtq_drop = %d\n", tr_cnt->tx_sw_mgmtq_drop));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\ttx_wcid_invalid = %d\n", tr_cnt->tx_wcid_invalid));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\twlan_state_non_valid_drop = %d\n", tr_cnt->wlan_state_non_valid_drop));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\tmgmt_max_drop = %d\n", tr_cnt->mgmt_max_drop));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\ttx_not_allowed_drop = %d\n", tr_cnt->tx_not_allowed_drop));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\tsys_not_ready_drop = %d\n", tr_cnt->sys_not_ready_drop));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\terr_recovery_drop = %d\n", tr_cnt->err_recovery_drop));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\ttx_forbid_drop = %d\n", tr_cnt->tx_forbid_drop));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\tigmp_clone_fail_drop = %d\n", tr_cnt->igmp_clone_fail_drop));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\tps_max_drop = %d\n", tr_cnt->ps_max_drop));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\tfill_tx_blk_fail_drop = %d\n", tr_cnt->fill_tx_blk_fail_drop));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\twdev_null_drop = %d\n", tr_cnt->wdev_null_drop));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\tcarrier_detect_drop = %d\n", tr_cnt->carrier_detect_drop));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\ttx_unknow_type_drop = %d\n", tr_cnt->tx_unknow_type_drop));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\tpkt_len_invalid = %d\n", tr_cnt->pkt_len_invalid));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\tpkt_invalid_wcid = %d\n", tr_cnt->pkt_invalid_wcid));
	for (i = 0; i < NUM_OF_TID; i++) {
		if (tr_cnt->me[i] > 0) {
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\tme = %d\n", tr_cnt->me[i]));
		}
		if (tr_cnt->re[i] > 0) {
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\tre = %d\n", tr_cnt->re[i]));
		}
		if (tr_cnt->le[i] > 0) {
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\tle = %d\n", tr_cnt->le[i]));
		}
		if (tr_cnt->be[i] > 0) {
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\tbe = %d\n", tr_cnt->be[i]));
		}
		if (tr_cnt->txop_limit_error[i] > 0) {
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\ttxop_limit_error = %d\n",
				tr_cnt->txop_limit_error[i]));
		}
		if (tr_cnt->baf[i] > 0) {
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\tbaf = %d\n", tr_cnt->baf[i]));
		}
	}
	for (i = 0; i < sizeof(tr_cnt->queue_deep_cnt) / sizeof(UINT32); i++)
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\tqueue_deep_cnt[%d] = %d\n", i, tr_cnt->queue_deep_cnt[i]));

#if defined(CTXD_SCATTER_AND_GATHER) || defined(CTXD_MEM_CPY)
	for (i = 0; i < sizeof(tr_cnt->ctxd_num) / sizeof(UINT32); i++)
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\tctxd_cnt[%d] = %d\n", i, tr_cnt->ctxd_num[i]));
#endif

	for (i = 0; i < 2; i++) {
		for (j = 0; j < 4; j++)
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\tband(%d) TX Enq CPU(%d) Stat = %d\n", i, j, tr_cnt->tx_enq_cpu_stat[i][j]));
	}

	for (i = 0; i < 2; i++) {
		for (j = 0; j < 4; j++)
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\tband(%d) TX Deq CPU(%d) Stat = %d\n", i, j, tr_cnt->tx_deq_cpu_stat[i][j]));
	}

	/* RX */
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\nRX Counter\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\tdamsdu type = %s\n", tr_ctl->amsdu_type ? "RX_HW_AMSDU" : "RX_SW_AMSDU"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\trx_invalid_wdev = %d\n", tr_cnt->rx_invalid_wdev));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\trx_pn_mismatch = %d\n", tr_cnt->rx_pn_mismatch));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\trx_invalid_pkt_len = %d\n", tr_cnt->rx_invalid_pkt_len));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\trx_to_os_drop = %d\n", tr_cnt->rx_to_os_drop));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\tba_err_wcid_invalid = %d\n", tr_cnt->ba_err_wcid_invalid));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\tba_drop_unknown = %d\n", tr_cnt->ba_drop_unknown));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\tba_err_old = %d\n", tr_cnt->ba_err_old));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\tba_err_dup1 = %d\n", tr_cnt->ba_err_dup1));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\tba_err_dup2 = %d\n", tr_cnt->ba_err_dup2));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\tba_err_tear_down = %d\n", tr_cnt->ba_err_tear_down));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\tba_flush_one = %d\n", tr_cnt->ba_flush_one));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\tba_flush_all = %d\n", tr_cnt->ba_flush_all));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\tba_sn_large_win_end = %d\n", tr_cnt->ba_sn_large_win_end));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\tbar_cnt = %d\n", tr_cnt->bar_cnt));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\tbar_large_win_start = %d\n", tr_cnt->bar_large_win_start));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\trx_icv_err_cnt = %d\n", tr_cnt->rx_icv_err_cnt));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\tba_amsdu_miss = %d\n", tr_cnt->ba_amsdu_miss));
#ifdef RX_RPS_SUPPORT
	{
		UINT32 cpu;
		for (cpu = 0; cpu < NR_CPUS; cpu++)
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\tsw_rx_queue[%u] cnt = %d\n",
				cpu, pAd->rx_que[i].Number));
	}
#else
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\tsw_rx_queue cnt = %d\n", pAd->rx_que.Number));
#endif
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\trx_sw_q_drop = %d\n", tr_cnt->rx_sw_q_drop));
	if (IS_MT7915(pAd)) {
		for (i = 0; i < DBDC_BAND_NUM; i++) {
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\tB%u:RMAC_ppdu_drop = %u\n", i,
				pAd->mcli_ctl[i].RxPPDUDropCnt));
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\tB%u:RMAC_RXFIFO_NOT_ENOUGH = %u\n", i,
				pAd->mcli_ctl[i].RxFIFONotEnoughCnt));
			pAd->mcli_ctl[i].RxPPDUDropCnt = pAd->mcli_ctl[i].RxFIFONotEnoughCnt = 0;
		}
	}

	if (pAd->tr_ctl.en_rx_profiling != en_rx_profiling && ops->ctrl_rxv_group) {
		if (en_rx_profiling > 0) {
			pAd->tr_ctl.en_rx_profiling = TRUE;
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
				 ("\t===== Enable RX rate statistics report(%d) =====\n", en_rx_profiling));
			ops->ctrl_rxv_group(pAd, BAND0, 0x2, TRUE);
			ops->ctrl_rxv_group(pAd, BAND1, 0x2, TRUE);
			ops->ctrl_rxv_group(pAd, BAND0, 0x3, TRUE);
			ops->ctrl_rxv_group(pAd, BAND1, 0x3, TRUE);
		} else {
			pAd->tr_ctl.en_rx_profiling = FALSE;
			os_zero_mem(pAd->tr_ctl.tr_cnt.rx_rate_rc, sizeof(struct _rx_profiling)*2);
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
				("\t===== Disable RX rate statistics report(%d) =====\n", en_rx_profiling));
			ops->ctrl_rxv_group(pAd, BAND0, 0x2, FALSE);
			ops->ctrl_rxv_group(pAd, BAND1, 0x2, FALSE);
			ops->ctrl_rxv_group(pAd, BAND0, 0x3, FALSE);
			ops->ctrl_rxv_group(pAd, BAND1, 0x3, FALSE);
		}
	} else if (pAd->tr_ctl.en_rx_profiling) {
		UINT8 band_idx = 0, bw_idx = 0;
		BOOLEAN rx_rate_data = FALSE;

		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t===== RX rate statistics =====\n"));
		for (band_idx = 0 ; band_idx < BAND_NUM ; band_idx++) {
			struct _rx_profiling *rx_rate_rc = &pAd->tr_ctl.tr_cnt.rx_rate_rc[band_idx];

			if (rx_rate_rc->total_mpdu_cnt != 0) {
				rx_rate_data = TRUE;
				MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
					("\t[Band %d] Total MPDU:%u retry:%u(%d.%d%%)\n", band_idx,
					rx_rate_rc->total_mpdu_cnt, rx_rate_rc->total_retry_cnt,
					((rx_rate_rc->total_retry_cnt*1000)/rx_rate_rc->total_mpdu_cnt)/10,
					((rx_rate_rc->total_retry_cnt*1000)/rx_rate_rc->total_mpdu_cnt)%10));
			}
			if (rx_rate_data == FALSE)
				MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t[Band %d]\tN/A\n", band_idx));
			else {
				struct _rx_mod_cnt *mpdu_cnt = NULL;
				struct _rx_mod_cnt *retry_cnt = NULL;

				rx_rate_data = FALSE;
				for (bw_idx = 0 ; bw_idx < BW_160 ; bw_idx++) {
					mpdu_cnt = &pAd->tr_ctl.tr_cnt.rx_rate_rc[band_idx].mpdu_cnt[bw_idx];
					retry_cnt = &pAd->tr_ctl.tr_cnt.rx_rate_rc[band_idx].retry_cnt[bw_idx];

					if (bw_idx < 1) {
						/* CCK/OFDM only 20MHz bandwidth */
						for (i = 0; i < ARRAY_SIZE(mpdu_cnt->cck); i++) {
							if (mpdu_cnt->cck[i] != 0 && rx_rate_rc->total_mpdu_cnt != 0) {
								rx_rate_data = TRUE;
								MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
									 ("\t\t[CCK]\tMCS%d\t%u(%d.%d%%), retry:%u(%d.%d%%)\n",
									 i, mpdu_cnt->cck[i],
									 ((mpdu_cnt->cck[i]*1000)/rx_rate_rc->total_mpdu_cnt)/10,
									 ((mpdu_cnt->cck[i]*1000)/rx_rate_rc->total_mpdu_cnt)%10,
									 retry_cnt->cck[i],
									 ((retry_cnt->cck[i]*1000)/mpdu_cnt->cck[i])/10,
									 ((retry_cnt->cck[i]*1000)/mpdu_cnt->cck[i])%10));
							}
						}
						if (rx_rate_data == FALSE)
							MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_INFO, ("\t\t[CCK]\tN/A\n"));
						else
							rx_rate_data = FALSE;

						for (i = 0; i < ARRAY_SIZE(mpdu_cnt->ofdm); i++) {
							if (mpdu_cnt->ofdm[i] != 0 && rx_rate_rc->total_mpdu_cnt != 0) {
								rx_rate_data = TRUE;
								MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
									 ("\t\t[OFDM]\tMCS%d\t%u(%d.%d%%), retry:%u(%d.%d%%)\n",
									 i, mpdu_cnt->ofdm[i],
									 ((mpdu_cnt->ofdm[i]*1000)/rx_rate_rc->total_mpdu_cnt)/10,
									 ((mpdu_cnt->ofdm[i]*1000)/rx_rate_rc->total_mpdu_cnt)%10,
									 retry_cnt->ofdm[i],
									 ((retry_cnt->ofdm[i]*1000)/mpdu_cnt->ofdm[i])/10,
									 ((retry_cnt->ofdm[i]*1000)/mpdu_cnt->ofdm[i])%10));
							}
						}
						if (rx_rate_data == FALSE)
							MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_INFO, ("\t\t[OFDM]\tN/A\n"));
						else
							rx_rate_data = FALSE;
					}
					if (bw_idx < 2) {
						/* HT rates only 20/40Mhz bandwidth */
						for (i = 0; i < ARRAY_SIZE(mpdu_cnt->ht[0]); i++) {
							if (mpdu_cnt->ht[0][i] != 0 && rx_rate_rc->total_mpdu_cnt != 0) {
								rx_rate_data = TRUE;
								MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
									 ("\t\t[HT][%dMHz][LGI]\t", (INT32)(BIT(bw_idx)*20)));
								MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
									 ("MCS%d\t%u(%d.%d%%), retry:%u(%d.%d%%)\n",
									 i, mpdu_cnt->ht[0][i],
									 ((mpdu_cnt->ht[0][i]*1000)/rx_rate_rc->total_mpdu_cnt)/10,
									 ((mpdu_cnt->ht[0][i]*1000)/rx_rate_rc->total_mpdu_cnt)%10,
									 retry_cnt->ht[0][i],
									 ((retry_cnt->ht[0][i]*1000)/mpdu_cnt->ht[0][i])/10,
									 ((retry_cnt->ht[0][i]*1000)/mpdu_cnt->ht[0][i])%10));
							}
						}
						if (rx_rate_data == FALSE)
							MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_INFO,
								("\t\t[HT][%dMHz][LGI]\tN/A\n", (INT32)(BIT(bw_idx)*20)));
						else
							rx_rate_data = FALSE;

						for (i = 0; i < ARRAY_SIZE(mpdu_cnt->ht[1]); i++) {
							if (mpdu_cnt->ht[1][i] != 0 && rx_rate_rc->total_mpdu_cnt != 0) {
								rx_rate_data = TRUE;
								MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
									 ("\t\t[HT][%dMHz][SGI]\t", (INT32)(BIT(bw_idx)*20)));
								MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
									 ("MCS%d\t%u(%d.%d%%), retry:%u(%d.%d%%)\n",
									 i, mpdu_cnt->ht[1][i],
									 ((mpdu_cnt->ht[1][i]*1000)/rx_rate_rc->total_mpdu_cnt)/10,
									 ((mpdu_cnt->ht[1][i]*1000)/rx_rate_rc->total_mpdu_cnt)%10,
									 retry_cnt->ht[1][i],
									 ((retry_cnt->ht[1][i]*1000)/mpdu_cnt->ht[1][i])/10,
									 ((retry_cnt->ht[1][i]*1000)/mpdu_cnt->ht[1][i])%10));
							}
						}
						if (rx_rate_data == FALSE)
							MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_INFO,
								("\t\t[HT][%dMHz][SGI]\tN/A\n", (INT32)(BIT(bw_idx)*20)));
						else
							rx_rate_data = FALSE;
					}
					for (i = 0; i < 4; i++) {
						UINT32 *rc = &mpdu_cnt->vht[0][i][0];
						UINT32 *retry_rc = &retry_cnt->vht[0][i][0];

						for (j = 0; j < ARRAY_SIZE(mpdu_cnt->vht[0][i]); j++) {
							if (rc[j] != 0 && rx_rate_rc->total_mpdu_cnt != 0) {
								rx_rate_data = TRUE;
								MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
									 ("\t\t[VHT][%dMHz][LGI]\t[NSS=%d]\t",
									  (INT32)(BIT(bw_idx)*20), i+1));
								MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
									 ("MCS%d\t%u(%d.%d%%), retry:%u (%d.%d%%)\n",
									  j, rc[j],
									  ((rc[j]*1000)/rx_rate_rc->total_mpdu_cnt)/10,
									  ((rc[j]*1000)/rx_rate_rc->total_mpdu_cnt)%10,
									  retry_rc[j],
									  ((retry_rc[j]*1000)/rc[j])/10,
									  ((retry_rc[j]*1000)/rc[j])%10));
							}
						}
						if (rx_rate_data == FALSE)
							MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_INFO,
								 ("\t\t[VHT][%dMHz][LGI]\t[NSS=%d]\tN/A\n",
								  (INT32)(BIT(bw_idx)*20), i+1));
						else
							rx_rate_data = FALSE;
					}
					for (i = 0; i < 4; i++) {
						UINT32 *rc = &mpdu_cnt->vht[1][i][0];
						UINT32 *retry_rc = &retry_cnt->vht[1][i][0];

						for (j = 0; j < ARRAY_SIZE(mpdu_cnt->vht[1][i]); j++) {
							if (rc[j] != 0 && rx_rate_rc->total_mpdu_cnt != 0) {
								rx_rate_data = TRUE;
								MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
									 ("\t\t[VHT][%dMHz][SGI]\t[NSS=%d]\t",
									  (INT32)(BIT(bw_idx)*20), i+1));
								MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
									 ("MCS%d\t%u(%d.%d%%), retry:%u(%d.%d%%)\n",
									  j, rc[j],
									  ((rc[j]*1000)/rx_rate_rc->total_mpdu_cnt)/10,
									  ((rc[j]*1000)/rx_rate_rc->total_mpdu_cnt)%10,
									  retry_rc[j],
									  ((retry_rc[j]*1000)/rc[j])/10,
									  ((retry_rc[j]*1000)/rc[j])%10));
							}
						}
						if (rx_rate_data == FALSE)
							MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_INFO,
								 ("\t\t[VHT][%dMHz][SGI]\t[NSS=%d]\tN/A\n",
								  (INT32)(BIT(bw_idx)*20), i+1));
						else
							rx_rate_data = FALSE;
					}
					for (i = 0; i < 4; i++) {
						UINT32 *rc = &mpdu_cnt->he[0][i][0];
						UINT32 *retry_rc = &retry_cnt->he[0][i][0];

						for (j = 0; j < ARRAY_SIZE(mpdu_cnt->he[0][i]); j++) {
							if (rc[j] != 0 && rx_rate_rc->total_mpdu_cnt != 0) {
								rx_rate_data = TRUE;
								MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
									 ("\n\t\t[HE][%dMHz][0.8us gi]\t[NSS=%d]\t",
									  (INT32)(BIT(bw_idx)*20), i+1));
								MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
									 ("MCS%d\t%u(%d.%d%%), retry:%u(%d.%d%%)\n",
									  j, rc[j],
									  ((rc[j]*1000)/rx_rate_rc->total_mpdu_cnt)/10,
									  ((rc[j]*1000)/rx_rate_rc->total_mpdu_cnt)%10,
									  retry_rc[j],
									  ((retry_rc[j]*1000)/rc[j])/10,
									  ((retry_rc[j]*1000)/rc[j])%10));
							}
						}
						if (rx_rate_data == FALSE)
							MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_INFO,
								 ("\t\t[HE][%dMHz][0.8us gi]\t[NSS=%d]\tN/A\n",
								  (INT32)(BIT(bw_idx)*20), i+1));
						else
							rx_rate_data = FALSE;
					}
					for (i = 0; i < 4; i++) {
						UINT32 *rc = &mpdu_cnt->he[1][i][0];
						UINT32 *retry_rc = &retry_cnt->he[1][i][0];

						for (j = 0; j < ARRAY_SIZE(mpdu_cnt->he[1][i]); j++) {
							if (rc[j] != 0 && rx_rate_rc->total_mpdu_cnt != 0) {
								rx_rate_data = TRUE;
								MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
									 ("\t\t[HE][%dMHz][1.6us gi]\t[NSS=%d]\t",
									  (INT32)(BIT(bw_idx)*20), i+1));
								MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
									 ("MCS%d\t%u(%d.%d%%), retry:%u(%d.%d%%)\n",
									  j, rc[j],
									  ((rc[j]*1000)/rx_rate_rc->total_mpdu_cnt)/10,
									  ((rc[j]*1000)/rx_rate_rc->total_mpdu_cnt)%10,
									  retry_rc[j],
									  ((retry_rc[j]*1000)/rc[j])/10,
									  ((retry_rc[j]*1000)/rc[j])%10));
							}
						}
						if (rx_rate_data == FALSE)
							MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_INFO,
								 ("\t\t[HE][%dMHz][1.6us gi]\t[NSS=%d]\tN/A\n",
								  (INT32)(BIT(bw_idx)*20), i+1));
						else
							rx_rate_data = FALSE;
					}
					for (i = 0; i < 4; i++) {
						UINT32 *rc = &mpdu_cnt->he[2][i][0];
						UINT32 *retry_rc = &retry_cnt->he[2][i][0];

						for (j = 0; j < ARRAY_SIZE(mpdu_cnt->he[2][i]); j++) {
							if (rc[j] != 0 && rx_rate_rc->total_mpdu_cnt != 0) {
								rx_rate_data = TRUE;
								MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
									 ("\t\t[HE][%dMHz][3.2us gi]\t[NSS=%d]\t",
									  (INT32)(BIT(bw_idx)*20), i+1));
								MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
									 ("MCS%d\t%u(%d.%d%%), retry:%u(%d.%d%%)\n",
									  j, rc[j],
									  ((rc[j]*1000)/rx_rate_rc->total_mpdu_cnt)/10,
									  ((rc[j]*1000)/rx_rate_rc->total_mpdu_cnt)%10,
									  retry_rc[j],
									  ((retry_rc[j]*1000)/rc[j])/10,
									  ((retry_rc[j]*1000)/rc[j])%10));
							}
						}
						if (rx_rate_data == FALSE)
							MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_INFO,
								 ("\t\t[HE][%dMHz][3.2us gi]\t[NSS=%d]\tN/A\n",
								  (INT32)(BIT(bw_idx)*20), i+1));
						else
							rx_rate_data = FALSE;
					}
				}
				os_zero_mem(rx_rate_rc, sizeof(*rx_rate_rc));
			}
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\n"));
		}
	}
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\n\t==============================\n"));
}

static VOID show_debug_info(RTMP_ADAPTER *pAd, UINT32 param0, UINT32 param1)
{
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("Current:\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\tDebug level: %d\n", debug_lvl));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\tDebug category: 0x%08x\n", debug_cat));
	debug_lvl = param0;
	debug_cat = param1;
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("After:\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\tDebug level: %d\n", debug_lvl));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\tDebug category: 0x%08x\n", debug_cat));
}

static VOID show_tpinfo_host_usage(RTMP_ADAPTER *pAd)
{
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("Host option usage:\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t0: help\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t1: debug info\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t2: tx free notify host info\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t3: wfdma info\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t4: counter info\n"));

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("Host debug info usage:\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t0: DBG_LVL_OFF\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t1: DBG_LVL_ERROR\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t2: DBG_LVL_WARN\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t3: DBG_LVL_TRACE\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t4: DBG_LVL_INFO\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t5: DBG_LVL_LOUD\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t6: DBG_LVL_NOISY\n"));
}

VOID show_tpinfo_host(RTMP_ADAPTER *pAd, UINT32 option, UINT32 param0, UINT32 param1)
{
	switch (option) {
	case HOST_HELP:
		show_tpinfo_host_usage(pAd);
		break;
	case HOST_DBG_INFO:
		if (param0 == 0xFFFFFFFF && param1 == 0xFFFFFFFF) {
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_WARN,
				("%s: param0/param1 shall be set.\n", __func__));
			break;
		}
		show_debug_info(pAd, param0, param1);
		break;
#ifdef CUT_THROUGH
	case TX_FREE_NOTIFY_HOST_INFO:
		show_tx_free_notify_host_info(pAd);
		break;
#endif
#ifdef RTMP_MAC_PCI
	case WFDMA_INFO:
		show_wfdma_info(pAd);
		break;
#endif
	case COUNTER_INFO:
		show_counter_info(pAd, param0);
		break;
	}
}

static VOID show_tpinfo_wacpu_usage(VOID)
{
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("Wacpu option usage:\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t0: help\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t1: debug info\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t2: msdu drop info\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t3: ac tail drop info\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t4: bss table info\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t5: sta record info\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t6: tx free notify info\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t7: ctxd info\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t8: igmp info\n"));
}

static VOID show_tpinfo_wacpu(RTMP_ADAPTER *pAd, UINT32 option)
{
	MtCmdFwLog2Host(pAd, 1, 2);

	switch (option) {
	case WACPU_HELP:
		show_tpinfo_wacpu_usage();
		break;
	case WACPU_DBG_INFO:
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_WARN,
			("%s: not support option = %d\n", __func__, option));
		break;
	case MSDU_DROP_INFO:
		MtCmdCr4Query(pAd, 0x16, 0, 0);
		break;
	case AC_TAIL_DROP_INFO:
		MtCmdCr4Query(pAd, 0x17, 0, 0);
		break;
	case BSS_TABLE_INFO:
		MtCmdCr4Query(pAd, 0x20, 0, 0);
		break;
	case STAREC_INFO:
		MtCmdCr4Query(pAd, 0x21, 0, 0);
		break;
	case TX_FREE_NOTIFY_WACPU_INFO:
		MtCmdCr4Query(pAd, 0x19, 0, 0);
		break;
	case CTXD_INFO:
		MtCmdCr4Query(pAd, 0x18, 0, 0);
		break;
	case IGMP_INFO:
		MtCmdCr4Query(pAd, 0x1a, 0, 0);
		break;
	case IGMP_WHITE_LIST_INFO:
		MtCmdCr4Query(pAd, 0x1b, 0, 0);
		break;
	default:
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_WARN,
			("%s: unknown option = %d\n", __func__, option));
		break;
	}

	MtCmdFwLog2Host(pAd, 1, 0);
}

static VOID show_tpinfo_wocpu_usage(VOID)
{
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("Wocpu option usage:\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t0: help\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t1: debug info\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t2: dev info\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t3: bss info\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t4: sta rec info\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t5: ba info\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t6: fbcmd ring info\n"));

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("Wocpu debug level usage:\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t0: WO_DBG_ALERT\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t1: WO_DBG_ERR\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t2: WO_DBG_WARN\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t3: WO_DBG_INFO\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t4: WO_DBG_DEBUG\n"));

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("Wocpu debug category usage:\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t0: clear all category\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t1: WO_DBGM_HAL\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t2: WO_DBGM_QM\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t4: WO_DBGM_RRO\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t8: WO_DBGM_RXM\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t16: WO_DBGM_INTR\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t32: WO_DBGM_RING\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t64: WO_DBGM_FB_CMD\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t128: WO_DBGM_FW_CMD\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t256: WO_DBGM_MIOD\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t512: WO_DBGM_STA_INFO\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t1024: WO_DBGM_BSS\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t2048: WO_DBGM_DEV\n"));
}

static VOID show_tpinfo_wocpu(RTMP_ADAPTER *pAd, UINT32 option, UINT32 param0, UINT32 param1)
{
	/* For wo level and category */
	debug_lvl = 0;
	debug_cat = 0xFFFFFFFF;
	switch (option) {
	case WOCPU_HELP:
		show_tpinfo_wocpu_usage();
		break;
	case WOCPU_DBG_INFO:
		if (param0 == 0xFFFFFFFF && param1 == 0xFFFFFFFF) {
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_WARN,
				("%s: param0/param1 shall be set.\n", __func__));
			break;
		}
		show_debug_info(pAd, param0, param1);
		mt_cmd_wo_query(pAd, WO_CMD_DBG_INFO, debug_lvl, debug_cat);
		break;
	case WOCPU_DEV_INFO:
		mt_cmd_wo_query(pAd, WO_CMD_DEV_INFO_DUMP, param0, 0xFFFFFFFF);
		break;
	case WOCPU_BSS_INFO:
		mt_cmd_wo_query(pAd, WO_CMD_BSS_INFO_DUMP, param0, 0xFFFFFFFF);
		break;
	case WOCPU_STA_REC:
		mt_cmd_wo_query(pAd, WO_CMD_STA_REC_DUMP, param0, 0xFFFFFFFF);
		break;
	case WOCPU_BA_INFO:
		mt_cmd_wo_query(pAd, WO_CMD_BA_INFO_DUMP, param0, 0xFFFFFFFF);
		break;
	case WOCPU_FBCMD_Q_INFO:
		mt_cmd_wo_query(pAd, WO_CMD_FBCMD_Q_DUMP, param0, 0xFFFFFFFF);
		break;
	default:
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_WARN,
			("%s: unknown option = %d\n", __func__, option));
		break;
	}
}

INT show_tpinfo_proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UINT32 cpu = 0, option = 0;
	UINT32 param0 = 0xFFFFFFFF, param1 = 0xFFFFFFFF;
	CHAR *param;

	if (arg == NULL || strlen(arg) == 0)
		goto err;

	param = rstrtok(arg, "-");

	if (param) {
		cpu = os_str_tol(param, 0, 10);
		param = rstrtok(NULL, "-");

		if (param) {
			option = os_str_toul(param, 0, 10);
			param = rstrtok(NULL, "-");

			if (param) {
				param0 = os_str_tol(param, 0, 10);
				param = rstrtok(NULL, "-");
				if (param)
					param1 = os_str_tol(param, 0, 10);
			}
		}
	}

	if (cpu == 0)
		show_tpinfo_host(pAd, option, param0, param1);
	else if (cpu == 1)
		show_tpinfo_wacpu(pAd, option);
	else if (cpu == 2)
		show_tpinfo_wocpu(pAd, option, param0, param1);
	else
		goto err;

	return TRUE;
err:
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
		("\tiwpriv $(inf_name) show tpinfo=[cpu]-[option]-[param0]-[param1]\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
		("\t[cpu] 0: host, 1: wacpu, 2: wocpu\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
		("\t[param0] may be debug info or wlan_idx/bss_idx/dev_idx\n"
		 "\t\t- option1(debug info): debug level\n"
		 "\t\t- cpu2(wocpu): wlan_idx/bss_idx/dev_idx\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
		("\t[param1]\n"
		 "\t\t- option1(debug info): debug category\n"));

	return TRUE;
}

INT show_trinfo_proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	struct _RTMP_CHIP_DBG *chip_dbg = hc_get_chip_dbg(pAd->hdev_ctrl);
#if defined(RTMP_PCI_SUPPORT) || defined(RTMP_RBUS_SUPPORT)
	UINT8 num_of_tx_ring = hif_get_tx_res_num(pAd->hdev_ctrl);
	UINT8 num_of_rx_ring = hif_get_rx_res_num(pAd->hdev_ctrl);
	PCI_HIF_T *hif = hc_get_hif_ctrl(pAd->hdev_ctrl);
	struct hif_pci_rx_ring *rx_ring = NULL;
	struct hif_pci_tx_ring *tx_ring = NULL;

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
			("=================================================\n"));

	if (IS_RBUS_INF(pAd) || IS_PCI_INF(pAd)) {
		UINT32 tbase[num_of_tx_ring], tcnt[num_of_tx_ring];
		UINT32 tcidx[num_of_tx_ring], tdidx[num_of_tx_ring];
		UINT32 rbase[num_of_rx_ring], rcnt[num_of_rx_ring];
		UINT32 rcidx[num_of_rx_ring], rdidx[num_of_rx_ring];
		INT idx;
		INT TxHwRingNum = num_of_tx_ring;
		INT RxHwRingNum = num_of_rx_ring;
#ifdef ERR_RECOVERY

		if (IsStopingPdma(&pAd->ErrRecoveryCtl))
			return TRUE;

#endif /* ERR_RECOVERY */

		for (idx = 0; idx < TxHwRingNum; idx++) {
			tx_ring = pci_get_tx_ring_by_ridx(hif, idx);
			HIF_IO_READ32(pAd->hdev_ctrl, tx_ring->hw_desc_base, &tbase[idx]);
			HIF_IO_READ32(pAd->hdev_ctrl, tx_ring->hw_cnt_addr, &tcnt[idx]);
			HIF_IO_READ32(pAd->hdev_ctrl, tx_ring->hw_cidx_addr, &tcidx[idx]);
			HIF_IO_READ32(pAd->hdev_ctrl, tx_ring->hw_didx_addr, &tdidx[idx]);
		}

		for (idx = 0; idx < RxHwRingNum; idx++) {
			rx_ring = pci_get_rx_ring_by_ridx(hif, idx);
			HIF_IO_READ32(pAd->hdev_ctrl, rx_ring->hw_desc_base, &rbase[idx]);
			HIF_IO_READ32(pAd->hdev_ctrl, rx_ring->hw_cnt_addr, &rcnt[idx]);
			HIF_IO_READ32(pAd->hdev_ctrl, rx_ring->hw_cidx_addr, &rcidx[idx]);
			HIF_IO_READ32(pAd->hdev_ctrl, rx_ring->hw_didx_addr, &rdidx[idx]);
		}

		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("TxRing Configuration\n"));
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%4s %8s %8s %10s %6s %6s %6s %6s %6s\n",
				"Idx", "Attr", "Reg", "Base", "Cnt", "CIDX", "DIDX", "QCnt", "FreeCnt"));

		for (idx = 0; idx < TxHwRingNum; idx++) {
			UINT32 queue_cnt, free_cnt;

			tx_ring = pci_get_tx_ring_by_ridx(hif, idx);
			queue_cnt = (tcidx[idx] >= tdidx[idx]) ? (tcidx[idx] - tdidx[idx]) : (tcidx[idx] - tdidx[idx] + tcnt[idx]);
			free_cnt = hif_get_tx_resource_free_num(pAd->hdev_ctrl, idx);

			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
				("%4d %8s %8x %10x %6x %6x %6x %6x %6x\n",
				idx,
				(tx_ring->ring_attr == HIF_TX_DATA) ? "DATA" :
				(tx_ring->ring_attr == HIF_TX_CMD) ? "CMD" :
				(tx_ring->ring_attr == HIF_TX_CMD_WM) ? "CMD_WM" :
				(tx_ring->ring_attr == HIF_TX_FWDL) ? "FWDL" : "UN",
				tx_ring->hw_desc_base, tbase[idx],
				tcnt[idx], tcidx[idx], tdidx[idx], queue_cnt, free_cnt));
		}

		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("RxRing Configuration\n"));
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%4s %8s %8s %10s %6s %6s %6s %6s\n",
				"Idx", "Attr", "Reg", "Base", "Cnt", "CIDX", "DIDX", "QCnt"));

		for (idx = 0; idx < RxHwRingNum; idx++) {
			UINT32 queue_cnt;

			rx_ring = pci_get_rx_ring_by_ridx(hif, idx);
			queue_cnt = (rdidx[idx] > rcidx[idx]) ? (rdidx[idx] - rcidx[idx] - 1) : (rdidx[idx] - rcidx[idx] + rcnt[idx] - 1);

			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
				("%4d %8s %8x %10x %6x %6x %6x %6x\n",
				idx,
				(rx_ring->ring_attr == HIF_RX_DATA) ? "DATA" :
				(rx_ring->ring_attr == HIF_RX_EVENT) ? "EVENT" : "UN",
				rx_ring->hw_desc_base, rbase[idx],
				rcnt[idx], rcidx[idx], rdidx[idx], queue_cnt));
		}
	}

#endif /* defined(RTMP_PCI_SUPPORT) || defined(RTMP_RBUS_SUPPORT) */

	if (chip_dbg->show_dma_info)
		chip_dbg->show_dma_info(pAd->hdev_ctrl);

	return TRUE;
}

INT show_swqinfo(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	struct qm_ops *ops = pAd->qm_ops;

	if (ops->dump_all_sw_queue) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
			 ("%s: show_swqinfo\n", __func__));

		ops->dump_all_sw_queue(pAd);
	}

	return TRUE;
}

INT show_txqinfo_proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	struct qm_ops *ops = pAd->qm_ops;
	CHAR *param;
	UINT16 wcid = 0;
	UCHAR q_idx = 0;
	enum PACKET_TYPE pkt_type = 0;

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
			 ("%s::param = %s\n", __func__, arg));

	if (arg == NULL)
		goto error;

	param = rstrtok(arg, ":");

	if (param != NULL)
		wcid = os_str_tol(param, 0, 10);
	else
		goto error;

	param = rstrtok(NULL, ":");

	if (param != NULL)
		pkt_type = os_str_tol(param, 0, 10);
	else
		goto error;

	param = rstrtok(NULL, ":");

	if (param != NULL)
		q_idx = os_str_tol(param, 0, 10);
	else
		goto error;

	if (ops->sta_dump_queue) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
			 ("%s::wcid = %d, pkt_type = %d, q_idx = %d\n",
			  __func__, wcid, pkt_type, q_idx));

		ops->sta_dump_queue(pAd, wcid, pkt_type, q_idx);
	}

	return TRUE;

error:

	return 0;
}

#ifdef DOT11_HE_AX
INT show_bsscolor_proc(RTMP_ADAPTER *ad, RTMP_STRING *arg)
{
	show_bss_color_info(ad);
	return TRUE;
}
#endif

#ifdef WSC_STA_SUPPORT
INT	Show_WpsManufacturer_Proc(
	IN	PRTMP_ADAPTER	pAd,
	OUT	RTMP_STRING *pBuf,
	IN ULONG			BufLen)
{
	POS_COOKIE pObj = (POS_COOKIE)pAd->OS_Cookie;
	PSTA_ADMIN_CONFIG pStaCfg = &pAd->StaCfg[pObj->ioctl_if];

	sprintf(pBuf, "\tManufacturer = %s", pStaCfg->wdev.WscControl.RegData.SelfInfo.Manufacturer);
	return 0;
}

INT	Show_WpsModelName_Proc(
	IN	PRTMP_ADAPTER	pAd,
	OUT	RTMP_STRING *pBuf,
	IN ULONG			BufLen)
{
	POS_COOKIE pObj = (POS_COOKIE)pAd->OS_Cookie;
	PSTA_ADMIN_CONFIG pStaCfg = &pAd->StaCfg[pObj->ioctl_if];

	sprintf(pBuf, "\tModelName = %s", pStaCfg->wdev.WscControl.RegData.SelfInfo.ModelName);
	return 0;
}

INT	Show_WpsDeviceName_Proc(
	IN	PRTMP_ADAPTER	pAd,
	OUT	RTMP_STRING *pBuf,
	IN ULONG			BufLen)
{
	POS_COOKIE pObj = (POS_COOKIE)pAd->OS_Cookie;
	PSTA_ADMIN_CONFIG pStaCfg = &pAd->StaCfg[pObj->ioctl_if];

	sprintf(pBuf, "\tDeviceName = %s", pStaCfg->wdev.WscControl.RegData.SelfInfo.DeviceName);
	return 0;
}

INT	Show_WpsModelNumber_Proc(
	IN	PRTMP_ADAPTER	pAd,
	OUT	RTMP_STRING *pBuf,
	IN ULONG			BufLen)
{
	POS_COOKIE pObj = (POS_COOKIE)pAd->OS_Cookie;
	PSTA_ADMIN_CONFIG pStaCfg = &pAd->StaCfg[pObj->ioctl_if];

	sprintf(pBuf, "\tModelNumber = %s", pStaCfg->wdev.WscControl.RegData.SelfInfo.ModelNumber);
	return 0;
}

INT	Show_WpsSerialNumber_Proc(
	IN	PRTMP_ADAPTER	pAd,
	OUT	RTMP_STRING *pBuf,
	IN ULONG			BufLen)
{
	POS_COOKIE pObj = (POS_COOKIE)pAd->OS_Cookie;
	PSTA_ADMIN_CONFIG pStaCfg = &pAd->StaCfg[pObj->ioctl_if];

	sprintf(pBuf, "\tSerialNumber = %s", pStaCfg->wdev.WscControl.RegData.SelfInfo.SerialNumber);
	return 0;
}
#endif /* WSC_STA_SUPPORT */

#ifdef SINGLE_SKU
INT	Show_ModuleTxpower_Proc(
	IN	PRTMP_ADAPTER	pAd,
	OUT	RTMP_STRING *pBuf,
	IN	ULONG			BufLen)
{
	snprintf(pBuf, BufLen, "\tModuleTxpower = %d", pAd->CommonCfg.ModuleTxpower);
	return 0;
}
#endif /* SINGLE_SKU */

#ifdef APCLI_SUPPORT
INT RTMPIoctlConnStatus(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	INT i = 0;
	POS_COOKIE pObj;
	UCHAR ifIndex;
	BOOLEAN bConnect = FALSE;
	struct wifi_dev *wdev = NULL;
#ifdef MAC_REPEATER_SUPPORT
	MBSS_TO_CLI_LINK_MAP_T *pMbssToCliLinkMap = NULL;
	INT	MbssIdx;
#endif
	struct tx_rx_ctl *tr_ctl = &pAd->tr_ctl;

	pObj = (POS_COOKIE) pAd->OS_Cookie;
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("==>RTMPIoctlConnStatus\n"));

	if (pObj->ioctl_if_type != INT_APCLI)
		return FALSE;

	ifIndex = pObj->ioctl_if;
	wdev = &pAd->StaCfg[ifIndex].wdev;
	if (!wdev)
		return FALSE;
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("=============================================================\n"));

	if (((GetAssociatedAPByWdev(pAd, wdev)) != NULL) && (pAd->StaCfg[ifIndex].SsidLen != 0)) {
		for (i = 0; VALID_UCAST_ENTRY_WCID(pAd, i); i++) {
			PMAC_TABLE_ENTRY pEntry = &pAd->MacTab.Content[i];
			STA_TR_ENTRY *tr_entry = &tr_ctl->tr_entry[i];

			if (IS_ENTRY_PEER_AP(pEntry)
				&& (pEntry->Sst == SST_ASSOC)
				&& (tr_entry->PortSecured == WPA_802_1X_PORT_SECURED)) {
				if (pEntry->wdev == &pAd->StaCfg[ifIndex].wdev) {
					MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
							 ("ApCli%d         Connected AP : %02X:%02X:%02X:%02X:%02X:%02X   SSID:%s\n",
							  ifIndex, PRINT_MAC(pEntry->Addr), pAd->StaCfg[ifIndex].Ssid));
					bConnect = TRUE;
#ifdef MWDS

					if (pAd->StaCfg[ifIndex].MlmeAux.bSupportMWDS)
						MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("MWDSCap : YES\n"));
					else
						MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("MWDSCap : NO\n"));

#endif /* MWDS */
				}
			}

#ifdef MAC_REPEATER_SUPPORT
			else if (IS_ENTRY_REPEATER(pEntry)
					 && (pEntry->Sst == SST_ASSOC)
					 && (tr_entry->PortSecured == WPA_802_1X_PORT_SECURED)) {
				if (pEntry->wdev == &pAd->StaCfg[ifIndex].wdev) {
					MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
							 ("Rept[wcid=%-3d] Connected AP : %02X:%02X:%02X:%02X:%02X:%02X   SSID:%s\n",
							  i, PRINT_MAC(pEntry->Addr), pAd->StaCfg[ifIndex].Ssid));
					bConnect = TRUE;
				}
			}

#endif
		}

		if (!bConnect)
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("ApCli%d Connected AP : Disconnect\n", ifIndex));
	} else
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("ApCli%d Connected AP : Disconnect\n", ifIndex));

#ifdef MAC_REPEATER_SUPPORT
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("ApCli%d CliLinkMap ra:", ifIndex));

	for (MbssIdx = 0; MbssIdx < pAd->ApCfg.BssidNum; MbssIdx++) {
		pMbssToCliLinkMap = &pAd->ApCfg.MbssToCliLinkMap[MbssIdx];

		if (pMbssToCliLinkMap->cli_link_wdev == &pAd->StaCfg[ifIndex].wdev)
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%d ", MbssIdx));
	}

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\n\r"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("Ignore repeater MAC address\n\r"));

	for (i = 0; i < MAX_IGNORE_AS_REPEATER_ENTRY_NUM; i++) {
		INVAILD_TRIGGER_MAC_ENTRY *pEntry = NULL;

		pEntry = &pAd->ApCfg.ReptControl.IgnoreAsRepeaterEntry[i];

		if (pEntry->bInsert)
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("[%d]%02X:%02X:%02X:%02X:%02X:%02X\n\r", i,
					 PRINT_MAC(pEntry->MacAddr)));
	}

#endif
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\n\r"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("=============================================================\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("<==RTMPIoctlConnStatus\n"));
	return TRUE;
}
#endif/*APCLI_SUPPORT*/

INT32 getLegacyOFDMMCSIndex(UINT8 MCS)
{
	INT32 mcs_index = MCS;

	if (MCS == 0xb)
		mcs_index = 0;
	else if (MCS == 0xf)
		mcs_index = 1;
	else if (MCS == 0xa)
		mcs_index = 2;
	else if (MCS == 0xe)
		mcs_index = 3;
	else if (MCS == 0x9)
		mcs_index = 4;
	else if (MCS == 0xd)
		mcs_index = 5;
	else if (MCS == 0x8)
		mcs_index = 6;
	else if (MCS == 0xc)
		mcs_index = 7;

	return mcs_index;
}

void  getRate(HTTRANSMIT_SETTING HTSetting, ULONG *fLastTxRxRate)

{
	UINT8					Antenna = 0;
	UINT8					MCS = HTSetting.field.MCS;
	int rate_count = sizeof(MCSMappingRateTable) / sizeof(int);
	int rate_index = 0;
	int value = 0;

#ifdef DOT11_VHT_AC
	if (HTSetting.field.MODE >= MODE_VHT) {
		MCS = HTSetting.field.MCS & 0xf;
		Antenna = (HTSetting.field.MCS >> 4) + 1;

		if (HTSetting.field.BW == BW_20) {
			rate_index = 112 + ((Antenna - 1) * 10) +
						 ((UCHAR)HTSetting.field.ShortGI * 160) +
						 ((UCHAR)MCS);
		} else if (HTSetting.field.BW == BW_40) {
			rate_index = 152 + ((Antenna - 1) * 10) +
						 ((UCHAR)HTSetting.field.ShortGI * 160) +
						 ((UCHAR)MCS);
		} else if (HTSetting.field.BW == BW_80) {
			rate_index = 192 + ((Antenna - 1) * 10) +
						 ((UCHAR)HTSetting.field.ShortGI * 160) +
						 ((UCHAR)MCS);
		} else if (HTSetting.field.BW == BW_160) {
			rate_index = 232 + ((Antenna - 1) * 10) +
						 ((UCHAR)HTSetting.field.ShortGI * 160) +
						 ((UCHAR)MCS);
		}
	} else {
#endif /* DOT11_VHT_AC */
		if (HTSetting.field.MODE >= MODE_HTMIX) {
			MCS = HTSetting.field.MCS;

			if ((HTSetting.field.MODE == MODE_HTMIX)
				|| (HTSetting.field.MODE == MODE_HTGREENFIELD))
				Antenna = (MCS >> 3) + 1;

			/* map back to 1SS MCS , multiply by antenna numbers later */
			if (MCS > 7)
				MCS %= 8;

			rate_index = 16 + ((UCHAR)HTSetting.field.BW * 24) + ((UCHAR)HTSetting.field.ShortGI * 48) + ((UCHAR)MCS);
		} else {
			if (HTSetting.field.MODE == MODE_OFDM)
				rate_index = getLegacyOFDMMCSIndex(HTSetting.field.MCS) + 4;
			else if (HTSetting.field.MODE == MODE_CCK)
				rate_index = (UCHAR)(HTSetting.field.MCS);
		}
	}

	if (rate_index < 0)
		rate_index = 0;

	if (rate_index >= rate_count)
		rate_index = rate_count - 1;

	if (HTSetting.field.MODE < MODE_VHT)
		value = (MCSMappingRateTable[rate_index] * 5) / 10;
	else
		value =  MCSMappingRateTable[rate_index];

	if (HTSetting.field.MODE >= MODE_HTMIX && HTSetting.field.MODE < MODE_VHT)
		value *= Antenna;

	*fLastTxRxRate = (ULONG)value;
	return;
}

#ifdef DOT11_HE_AX
void  get_rate_he(UINT8 mcs, UINT8 bw, UINT8 nss, UINT8 dcm, ULONG *last_tx_rate)

{
	ULONG value = 0;

	if (mcs > MAX_NUM_HE_MCS_ENTRIES)
		mcs = MAX_NUM_HE_MCS_ENTRIES - 1;

	if (nss > MAX_NUM_HE_SPATIAL_STREAMS)
		nss = MAX_NUM_HE_SPATIAL_STREAMS;

	if (bw > MAX_NUM_HE_BANDWIDTHS)
		bw = MAX_NUM_HE_BANDWIDTHS - 1;

	nss--;

	value = he_mcs_phyrate_mapping_table[bw][nss][mcs];
	/*In spec data rate when DCM =1 is half of the data rate when DCM = 0*/
	if (dcm && value)
		value = value / 2 ;

	*last_tx_rate = (ULONG)value;

	return;
}

#endif

#ifdef MT_MAC
INT show_txvinfo(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	struct _RTMP_CHIP_DBG *chip_dbg = hc_get_chip_dbg(pAd->hdev_ctrl);

	if (chip_dbg->show_txv_info)
		return chip_dbg->show_txv_info(pAd->hdev_ctrl, arg);
	else
		return FALSE;
}
#endif

#ifdef TXBF_SUPPORT

/*
	Set_InvTxBfTag_Proc - Invalidate BF Profile Tags
		usage: "iwpriv ra0 set InvTxBfTag=n"
		Reset Valid bit and zero out MAC address of each profile. The next profile will be stored in profile 0
*/
INT	Set_InvTxBfTag_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	return TRUE;
}

#ifdef MT_MAC
/*
	Set_ETxBfCodebook_Proc - Set ETxBf Codebook
	usage: iwpriv ra0 set ETxBfCodebook=0 to 3
*/
INT Set_ETxBfCodebook_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	ULONG t = os_str_tol(arg, 0, 10);

	/* TODO: shiang-7603 */
	if (IS_HIF_TYPE(pAd, HIF_MT)) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s(): Not support for HIF_MT yet!\n",
				 __func__));
		return FALSE;
	}

	if (t > 3) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("Set_ETxBfCodebook_Proc: value > 3!\n"));
		return FALSE;
	}

	return TRUE;
}

/*
	Set_ETxBfCoefficient_Proc - Set ETxBf Coefficient
		usage: iwpriv ra0 set ETxBfCoefficient=0 to 3
*/
INT Set_ETxBfCoefficient_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	ULONG t = os_str_tol(arg, 0, 10);

	/* TODO: shiang-7603 */
	if (IS_HIF_TYPE(pAd, HIF_MT)) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s(): Not support for HIF_MT yet!\n",
				 __func__));
		return FALSE;
	}

	if (t > 3) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("Set_ETxBfCoefficient_Proc: value > 3!\n"));
		return FALSE;
	}

	return TRUE;
}

/*
	Set_ETxBfGrouping_Proc - Set ETxBf Grouping
		usage: iwpriv ra0 set ETxBfGrouping=0 to 2
*/
INT Set_ETxBfGrouping_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	ULONG t = os_str_tol(arg, 0, 10);

	/* TODO: shiang-7603 */
	if (IS_HIF_TYPE(pAd, HIF_MT)) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s(): Not support for HIF_MT yet!\n",
				 __func__));
		return FALSE;
	}

	if (t > 2) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("Set_ETxBfGrouping_Proc: value > 2!\n"));
		return FALSE;
	}

	return TRUE;
}
#endif /* MT_MAC */

/*
	Set_ETxBfNoncompress_Proc - Set ETxBf Noncompress option
		usage: iwpriv ra0 set ETxBfNoncompress=0 or 1
*/
INT Set_ETxBfNoncompress_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	ULONG t = os_str_tol(arg, 0, 10);

	if (t > 1) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("Set_ETxBfNoncompress_Proc: value > 1!\n"));
		return FALSE;
	}

	pAd->CommonCfg.ETxBfNoncompress = t;
	return TRUE;
}

/*
	Set_ETxBfIncapable_Proc - Set ETxBf Incapable option
		usage: iwpriv ra0 set ETxBfIncapable=0 or 1
*/
INT Set_ETxBfIncapable_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	ULONG t = os_str_tol(arg, 0, 10);
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	struct wifi_dev *wdev = get_wdev_by_ioctl_idx_and_iftype(pAd, pObj->ioctl_if, pObj->ioctl_if_type);

	if (!wdev)
		return FALSE;

	if (t > 1)
		return FALSE;

	pAd->CommonCfg.ETxBfIncapable = t;
#ifdef MT_MAC
	mt_WrapSetETxBFCap(pAd, wdev, &pAd->CommonCfg.HtCapability.TxBFCap);
#endif /* MT_MAC */
	return TRUE;
}

/*
	Set_ITxBfDivCal_Proc - Calculate ITxBf Divider Calibration parameters
	usage: iwpriv ra0 set ITxBfDivCal=dd
			0=>display calibration parameters
			1=>update EEPROM values
			2=>update BBP R176
			10=>display calibration parameters and dump capture data
			11=>Skip divider calibration, just capture and dump capture data
*/
INT	Set_ITxBfDivCal_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	int calFunction;
	UINT32 value, value1, restore_value, loop = 0;
	struct _RTMP_CHIP_OP *ops = hc_get_chip_ops(pAd->hdev_ctrl);
	/* backup mac 1004 value */
	RTMP_IO_READ32(pAd->hdev_ctrl, 0x1004, &restore_value);
	/* Backup the original RTS retry count and then set to 0 */
	/* RTMP_IO_READ32(pAd->hdev_ctrl, 0x1344, &pAd->rts_tx_retry_num); */
	/* disable mac tx/rx */
	value = restore_value;
	value &= ~0xC;
	RTMP_IO_WRITE32(pAd->hdev_ctrl, 0x1004, value);
	/* set RTS retry count = 0 */
	RTMP_IO_WRITE32(pAd->hdev_ctrl, 0x1344, 0x00092B00);

	/* wait mac 0x1200, bbp 0x2130 idle */
	do {
		RTMP_IO_READ32(pAd->hdev_ctrl, 0x1200, &value);
		value &= 0x1;
		RTMP_IO_READ32(pAd->hdev_ctrl, 0x2130, &value1);
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_INFO, ("%s:: Wait until MAC 0x1200 bit0 and BBP 0x2130 become 0\n",
				 __func__));
		RtmpusecDelay(1);
		loop++;
	} while (((value != 0) || (value1 != 0)) && (loop < 300));

	if (loop >= 300) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
				 ("%s:: Wait until MAC 0x1200 bit0 and BBP 0x2130 become 0 > 300 times\n", __func__));
		return FALSE;
	}

	calFunction = os_str_tol(arg, 0, 10);
	ops->fITxBfDividerCalibration(pAd, calFunction, 0, NULL);
	/* enable TX/RX */
	RTMP_IO_WRITE32(pAd->hdev_ctrl, 0x1004, restore_value);
	/* Restore RTS retry count */
	/* RTMP_IO_WRITE32(pAd->hdev_ctrl, 0x1344, pAd->rts_tx_retry_num); */
	return TRUE;
}


#ifdef MT_MAC
/*
	Set_ETxBfEnCond_Proc - enable/disable ETxBF
	usage: iwpriv ra0 set ETxBfEnCond=dd
		0=>disable, 1=>enable
	Note: After use this command, need to re-run apStartup()/LinkUp() operations to sync all status.
		  If ETxBfIncapable!=0 then we don't need to reassociate.
*/
INT	Set_ETxBfEnCond_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UINT16 i;
	UCHAR ucETxBfEn;
	UCHAR ucStatus = FALSE;
	MAC_TABLE_ENTRY		*pEntry;
	TXBF_STATUS_INFO    TxBfInfo;
	struct wifi_dev *wdev = NULL;

	ucETxBfEn = os_str_tol(arg, 0, 10);

	if (ucETxBfEn > 1)
		return FALSE;

	for (i = 0; VALID_UCAST_ENTRY_WCID(pAd, i); i++) {
		pEntry = &pAd->MacTab.Content[i];

		if (!IS_ENTRY_NONE(pEntry)) {
			wdev = pEntry->wdev;
			TxBfInfo.ucTxPathNum = pAd->Antenna.field.TxPath;
			TxBfInfo.ucRxPathNum = pAd->Antenna.field.RxPath;
#ifdef DBDC_MODE
			if (pAd->CommonCfg.dbdc_mode) {
				UINT8 band_idx = HcGetBandByWdev(wdev);

				if (band_idx == DBDC_BAND0) {
					TxBfInfo.ucTxPathNum = pAd->dbdc_band0_tx_path;
					TxBfInfo.ucRxPathNum = pAd->dbdc_band0_rx_path;
				} else {
					TxBfInfo.ucTxPathNum = pAd->dbdc_band1_tx_path;
					TxBfInfo.ucRxPathNum = pAd->dbdc_band1_rx_path;
				}
			}
#endif

#ifdef ANTENNA_CONTROL_SUPPORT
			{
				UINT8 BandIdx = HcGetBandByWdev(wdev);
				if (pAd->bAntennaSetAPEnable[BandIdx]) {
					TxBfInfo.ucTxPathNum = pAd->TxStream[BandIdx];
					TxBfInfo.ucRxPathNum = pAd->RxStream[BandIdx];
				}
			}
#endif /* ANTENNA_CONTROL_SUPPORT */

			TxBfInfo.ucPhyMode   = wdev->PhyMode;
			TxBfInfo.u2Channel   = wdev->channel;
			TxBfInfo.pHtTxBFCap  = &pAd->CommonCfg.HtCapability.TxBFCap;
			TxBfInfo.cmmCfgETxBfIncapable = pAd->CommonCfg.ETxBfIncapable;
			TxBfInfo.cmmCfgETxBfNoncompress = pAd->CommonCfg.ETxBfNoncompress;
#ifdef VHT_TXBF_SUPPORT
			TxBfInfo.pVhtTxBFCap = &pAd->CommonCfg.vht_cap_ie.vht_cap;
#endif
			TxBfInfo.u4WTBL1 = pAd->mac_ctrl.wtbl_base_addr[0] +  i * pAd->mac_ctrl.wtbl_entry_size[0];
			TxBfInfo.u4WTBL2 = pAd->mac_ctrl.wtbl_base_addr[1] +  i * pAd->mac_ctrl.wtbl_entry_size[1];
			TxBfInfo.ucETxBfTxEn = ucETxBfEn;
			TxBfInfo.ucITxBfTxEn  = FALSE;
			TxBfInfo.u2Wcid = i;
			TxBfInfo.ucBW = pEntry->HTPhyMode.field.BW;
			TxBfInfo.ucNDPARate = 2; /* MCS2 */
			ucStatus = AsicTxBfEnCondProc(pAd, &TxBfInfo);
		}
	}

	return ucStatus;
}

INT set_txbf_stop_report_poll_proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UINT8	para[6];

	para[0] = os_str_tol(arg, 0, 10);

	if (para[0] != 0 && para[0] != 1) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
			("Wrong format!\niwpirv ra0 set TxBfStopReportPoll=N\nN=1: Stop Rpt Poll\nN=0: Re-enable Rpt Poll\n"));
		return FALSE;
	}

	para[1] = '\0';

	return txbf_config(pAd, BF_CONFIG_TYPE_STOP_REPORT_POLL, &para[0]);
}

INT Set_TxBfTxApply(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	CHAR	 *value;
	UCHAR	Input[5];
	INT		i;
	UINT16  u2WlanIdx;
	BOOLEAN fgETxBf, fgITxBf, fgMuTxBf, fgPhaseCali;
	BOOLEAN fgStatus = TRUE;

	if (strlen(arg) != 14)
		return FALSE;

	for (i = 0, value = rstrtok(arg, ":"); value; value = rstrtok(NULL, ":")) {
		if ((strlen(value) != 2) || (!isxdigit(*value)) || (!isxdigit(*(value + 1))))
			return FALSE;  /*Invalid*/

		AtoH(value, &Input[i++], 1);
	}

	u2WlanIdx   = Input[0];
	fgETxBf     = Input[1];
	fgITxBf     = Input[2];
	fgMuTxBf    = Input[3];
	fgPhaseCali = Input[4];
	CmdTxBfTxApplyCtrl(pAd,
					   u2WlanIdx,
					   fgETxBf,
					   fgITxBf,
					   fgMuTxBf,
					   fgPhaseCali);
	return fgStatus;
}

#ifdef TXBF_DYNAMIC_DISABLE
INT Set_TxBfDynamicDisable_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	BOOLEAN fgStatus = TRUE;
	BOOLEAN fgDisable;

	fgDisable = simple_strtol(arg, 0, 10);

	DynamicTxBfDisable(pAd, fgDisable);

	return fgStatus;
}
#endif /* TXBF_DYNAMIC_DISABLE */

INT set_txbf_dynamic_mechanism_proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	BOOLEAN fgStatus = TRUE;

	if (strlen(arg) == 0) {
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF,
		("bfdm Usage:\niwpriv ra0 set bfdm=On_Off_Bit_Map\nBit0: Dynamic BFee Adaption\n"));
	} else {
		pAd->bfdm.bfdm_bitmap = simple_strtol(arg, 0, 10);
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("bfdm_bitmap=%d\n", pAd->bfdm.bfdm_bitmap));
	}

	return fgStatus;
}

INT	Set_Trigger_Sounding_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UCHAR			Input[7];
	CHAR			*value;
	INT				i;
	BOOLEAN         fgStatus = FALSE;
	UCHAR           ucSu_Mu, ucMuNum, ucWlanId[4];
	UINT32          u4SndInterval;

#ifdef CONFIG_ATE
	UCHAR			control_band_idx;
#ifdef CONFIG_WLAN_SERVICE
	struct service_test *serv_test;
#else
	struct _ATE_CTRL *ATECtrl;
#endif /*  CONFIG_WLAN_SERVICE */
#endif /*CONFIG_ATE*/

	if (strlen(arg) != 20)
		return FALSE;

	for (i = 0, value = rstrtok(arg, ":"); value; value = rstrtok(NULL, ":")) {
		if ((strlen(value) != 2) || (!isxdigit(*value)) || (!isxdigit(*(value + 1))))
			return FALSE;  /*Invalid*/

		AtoH(value, &Input[i++], 1);
	}

	ucSu_Mu       = Input[0];
	ucMuNum       = Input[1];
	u4SndInterval = (UINT32) Input[2];
	u4SndInterval = u4SndInterval << 2;
	ucWlanId[0]   = Input[3];
	ucWlanId[1]   = Input[4];
	ucWlanId[2]   = Input[5];
	ucWlanId[3]   = Input[6];

	if (pAd->Antenna.field.TxPath <= 1)
		return FALSE;

#ifdef ANTENNA_CONTROL_SUPPORT
	{
		POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
		struct wifi_dev *wdev = get_wdev_by_ioctl_idx_and_iftype(pAd, pObj->ioctl_if, pObj->ioctl_if_type);
		UINT8 BandIdx = HcGetBandByWdev(wdev);

		if (pAd->bAntennaSetAPEnable[BandIdx] &&
			(pAd->Antenna.field.TxPath <= 1)) {
			return FALSE;
		}
	}
#endif /* ANTENNA_CONTROL_SUPPORT */

#ifdef CONFIG_ATE
#ifdef CONFIG_WLAN_SERVICE
	serv_test = (struct service_test *)(pAd->serv.serv_handle);
	control_band_idx = serv_test->ctrl_band_idx;
#else
	ATECtrl = &(pAd->ATECtrl);
	control_band_idx = ATECtrl->control_band_idx;
#endif /*  CONFIG_WLAN_SERVICE */
	if (ATE_ON(pAd)) {
		/* Enable Tx MAC HW before trigger sounding */
		MtATESetMacTxRx(pAd, ASIC_MAC_TX, TRUE, control_band_idx);
	}
#endif /*CONFIG_ATE*/

	if (mt_Trigger_Sounding_Packet(pAd,
								   TRUE,
								   u4SndInterval,
								   ucSu_Mu,
								   ucMuNum,
								   ucWlanId) == STATUS_TRUE)
		fgStatus = TRUE;

	return fgStatus;
}

INT	Set_Stop_Sounding_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	BOOLEAN         fgStatus = FALSE;

	if (mt_Trigger_Sounding_Packet(pAd,
								   FALSE,
								   0,
								   0,
								   0,
								   NULL) == STATUS_TRUE)
		fgStatus = TRUE;

	return fgStatus;
}

INT	Set_TxBfPfmuMemAlloc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UCHAR			Input[2];
	CHAR			*value;
	INT				i;
	BOOLEAN         fgStatus = FALSE;
	UCHAR           ucSu_Mu;
	UINT16          u2WlanIdx;


	if (strlen(arg) != 5)
		return FALSE;

	for (i = 0, value = rstrtok(arg, ":"); value; value = rstrtok(NULL, ":")) {
		if ((strlen(value) != 2) || (!isxdigit(*value)) || (!isxdigit(*(value + 1))))
			return FALSE;  /*Invalid*/

		AtoH(value, &Input[i++], 1);
	}

	ucSu_Mu   = Input[0];
	u2WlanIdx = Input[1];

	if (CmdPfmuMemAlloc(pAd,
						ucSu_Mu,
						u2WlanIdx) == STATUS_TRUE)
		fgStatus = TRUE;

	return fgStatus;
}

INT	Set_TxBfPfmuMemRelease(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	BOOLEAN         fgStatus = FALSE;
	UINT16          u2WlanIdx;

	u2WlanIdx  = os_str_tol(arg, 0, 10);

	if (CmdPfmuMemRelease(pAd, u2WlanIdx) == STATUS_TRUE)
		fgStatus = TRUE;

	return fgStatus;
}

INT	Set_TxBfPfmuMemAllocMapRead(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	BOOLEAN         fgStatus = FALSE;

	if (CmdPfmuMemAllocMapRead(pAd) == STATUS_TRUE)
		fgStatus = TRUE;

	return fgStatus;
}


INT Set_StaRecBfUpdate(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	CHAR			 *value;
	UCHAR			 Input[23];
	INT				 i;
	CHAR             BssIdx, WlanIdx;
	PMAC_TABLE_ENTRY pEntry;
	BOOLEAN          fgStatus = FALSE;
	struct txbf_starec_conf *man_bf_sta_rec = &pAd->manual_bf_sta_rec;

	if (strlen(arg) != 68)
		return FALSE;

	for (i = 0, value = rstrtok(arg, ":"); value; value = rstrtok(NULL, ":")) {
		if ((strlen(value) != 2) || (!isxdigit(*value)) || (!isxdigit(*(value + 1))))
			return FALSE;  /*Invalid*/

		AtoH(value, &Input[i++], 1);
	}

	WlanIdx                          = Input[0];
	BssIdx                           = Input[1];
	pEntry                           = &pAd->MacTab.Content[WlanIdx];

	if (pEntry == NULL)
		return FALSE;

	pEntry->rStaRecBf.u2PfmuId       = Input[2];
	pEntry->rStaRecBf.fgSU_MU        = Input[3];
	pEntry->rStaRecBf.u1TxBfCap      = Input[4];
	pEntry->rStaRecBf.ucNdpaRate     = Input[5];
	pEntry->rStaRecBf.ucNdpRate      = Input[6];
	pEntry->rStaRecBf.ucReptPollRate = Input[7];
	pEntry->rStaRecBf.ucTxMode       = Input[8];
	pEntry->rStaRecBf.ucNc           = Input[9];
	pEntry->rStaRecBf.ucNr           = Input[10];
	pEntry->rStaRecBf.ucCBW          = Input[11];
	pEntry->rStaRecBf.ucSEIdx        = Input[12];
	pEntry->rStaRecBf.ucTotMemRequire = Input[13];
	pEntry->rStaRecBf.ucMemRequire20M = Input[14];
	pEntry->rStaRecBf.ucMemRow0      = Input[15];
	pEntry->rStaRecBf.ucMemCol0      = Input[16];
	pEntry->rStaRecBf.ucMemRow1      = Input[17];
	pEntry->rStaRecBf.ucMemCol1      = Input[18];
	pEntry->rStaRecBf.ucMemRow2      = Input[19];
	pEntry->rStaRecBf.ucMemCol2      = Input[20];
	pEntry->rStaRecBf.ucMemRow3      = Input[21];
	pEntry->rStaRecBf.ucMemCol3      = Input[22];
	/* Default setting */
	pEntry->rStaRecBf.u2SmartAnt     = 0;
	pEntry->rStaRecBf.ucSoundingPhy  = 1;
	pEntry->rStaRecBf.uciBfTimeOut   = 0xFF;
	pEntry->rStaRecBf.uciBfDBW       = 0;
	pEntry->rStaRecBf.uciBfNcol      = 0;
	pEntry->rStaRecBf.uciBfNrow      = 0;
	pEntry->rStaRecBf.nr_bw160       = 0;
	pEntry->rStaRecBf.nc_bw160       = 0;
	pEntry->rStaRecBf.ru_start_idx   = 0;
	pEntry->rStaRecBf.ru_end_idx     = 0;
	pEntry->rStaRecBf.trigger_su     = 0;
	pEntry->rStaRecBf.trigger_mu     = 0;
	pEntry->rStaRecBf.ng16_su        = 0;
	pEntry->rStaRecBf.ng16_mu        = 0;
	pEntry->rStaRecBf.codebook42_su  = 0;
	pEntry->rStaRecBf.codebook75_mu  = 0;
	pEntry->rStaRecBf.he_ltf         = 0;

	if (man_bf_sta_rec->conf & BIT(MANUAL_HE_SU_MU))
		pEntry->rStaRecBf.fgSU_MU = man_bf_sta_rec->conf_su_mu;

	if (man_bf_sta_rec->conf & BIT(MANUAL_HE_RU_RANGE)) {
		pEntry->rStaRecBf.ru_start_idx = man_bf_sta_rec->conf_ru_start_idx;
		pEntry->rStaRecBf.ru_end_idx = man_bf_sta_rec->conf_ru_end_idx;
	}

	if (man_bf_sta_rec->conf & BIT(MANUAL_HE_TRIGGER)) {
		pEntry->rStaRecBf.trigger_su = man_bf_sta_rec->conf_trigger_su;
		pEntry->rStaRecBf.trigger_mu = man_bf_sta_rec->conf_trigger_mu;
	}

	if (man_bf_sta_rec->conf & BIT(MANUAL_HE_NG16)) {
		pEntry->rStaRecBf.ng16_su = man_bf_sta_rec->conf_ng16_su;
		pEntry->rStaRecBf.ng16_mu = man_bf_sta_rec->conf_ng16_mu;
	}

	if (man_bf_sta_rec->conf & BIT(MANUAL_HE_CODEBOOK)) {
		pEntry->rStaRecBf.codebook42_su = man_bf_sta_rec->conf_codebook42_su;
		pEntry->rStaRecBf.codebook75_mu = man_bf_sta_rec->conf_codebook75_mu;
	}

	if (man_bf_sta_rec->conf & BIT(MANUAL_HE_LTF))
		pEntry->rStaRecBf.he_ltf = man_bf_sta_rec->conf_he_ltf;

	if (man_bf_sta_rec->conf & BIT(MANUAL_HE_IBF)) {
		pEntry->rStaRecBf.uciBfNcol = man_bf_sta_rec->conf_ibf_ncol;
		pEntry->rStaRecBf.uciBfNrow = man_bf_sta_rec->conf_ibf_nrow;
	}

	if (man_bf_sta_rec->conf & BIT(NANUAL_HE_BW160)) {
		pEntry->rStaRecBf.nr_bw160 = man_bf_sta_rec->conf_nr_bw160;
		pEntry->rStaRecBf.nc_bw160 = man_bf_sta_rec->conf_nc_bw160;
	}

	{
		STA_REC_CFG_T StaCfg;

		os_zero_mem(&StaCfg, sizeof(STA_REC_CFG_T));
		StaCfg.MuarIdx = 0;
		StaCfg.ConnectionState = TRUE;
		StaCfg.ConnectionType = 0;
		StaCfg.u4EnableFeature = (1 << STA_REC_BF);
		StaCfg.ucBssIndex = BssIdx;
		StaCfg.u2WlanIdx = WlanIdx;
		StaCfg.pEntry = pEntry;

		if (CmdExtStaRecUpdate(pAd, StaCfg) == STATUS_TRUE)
			fgStatus = TRUE;
	}
	os_zero_mem(man_bf_sta_rec, sizeof(struct txbf_starec_conf));
	return fgStatus;
}

INT set_txbf_he_bf_starec(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	struct txbf_starec_conf *man_bf_sta_rec = &pAd->manual_bf_sta_rec;
	UINT32 input[15];
	CHAR *value;
	INT i;

	os_zero_mem(man_bf_sta_rec, sizeof(struct txbf_starec_conf));

	for (i = 0, value = rstrtok(arg, ":"); value; value = rstrtok(NULL, ":"), i++)
		input[i] = os_str_toul(value, 0, 16);

	man_bf_sta_rec->conf = input[0];
	man_bf_sta_rec->conf_su_mu = input[1];
	man_bf_sta_rec->conf_ru_start_idx = input[2];
	man_bf_sta_rec->conf_ru_end_idx = input[3];
	man_bf_sta_rec->conf_trigger_su	= input[4];
	man_bf_sta_rec->conf_trigger_mu = input[5];
	man_bf_sta_rec->conf_ng16_su = input[6];
	man_bf_sta_rec->conf_ng16_mu = input[7];
	man_bf_sta_rec->conf_codebook42_su = input[8];
	man_bf_sta_rec->conf_codebook75_mu = input[9];
	man_bf_sta_rec->conf_he_ltf = input[10];
	man_bf_sta_rec->conf_ibf_ncol = input[11];
	man_bf_sta_rec->conf_ibf_nrow = input[12];
	man_bf_sta_rec->conf_nr_bw160 = input[13];
	man_bf_sta_rec->conf_nc_bw160 = input[14];

	MTWF_LOG(DBG_CAT_BF, CATBF_IWCMD, DBG_LVL_TRACE, ("conf=%d\n", man_bf_sta_rec->conf));
	MTWF_LOG(DBG_CAT_BF, CATBF_IWCMD, DBG_LVL_TRACE, ("conf_su_mu=%d\n", man_bf_sta_rec->conf_su_mu));
	MTWF_LOG(DBG_CAT_BF, CATBF_IWCMD, DBG_LVL_TRACE, ("conf_ru_start_idx=%d\n", man_bf_sta_rec->conf_ru_start_idx));
	MTWF_LOG(DBG_CAT_BF, CATBF_IWCMD, DBG_LVL_TRACE, ("conf_ru_end_idx=%d\n", man_bf_sta_rec->conf_ru_end_idx));
	MTWF_LOG(DBG_CAT_BF, CATBF_IWCMD, DBG_LVL_TRACE, ("conf_trigger_su=%d\n", man_bf_sta_rec->conf_trigger_su));
	MTWF_LOG(DBG_CAT_BF, CATBF_IWCMD, DBG_LVL_TRACE, ("conf_trigger_mu=%d\n", man_bf_sta_rec->conf_trigger_mu));
	MTWF_LOG(DBG_CAT_BF, CATBF_IWCMD, DBG_LVL_TRACE, ("conf_ng16_su=%d\n", man_bf_sta_rec->conf_ng16_su));
	MTWF_LOG(DBG_CAT_BF, CATBF_IWCMD, DBG_LVL_TRACE, ("conf_ng16_mu=%d\n", man_bf_sta_rec->conf_ng16_mu));
	MTWF_LOG(DBG_CAT_BF, CATBF_IWCMD, DBG_LVL_TRACE, ("conf_codebook42_su=%d\n", man_bf_sta_rec->conf_codebook42_su));
	MTWF_LOG(DBG_CAT_BF, CATBF_IWCMD, DBG_LVL_TRACE, ("conf_codebook75_mu=%d\n", man_bf_sta_rec->conf_codebook75_mu));
	MTWF_LOG(DBG_CAT_BF, CATBF_IWCMD, DBG_LVL_TRACE, ("conf_he_ltf=%d\n", man_bf_sta_rec->conf_he_ltf));
	MTWF_LOG(DBG_CAT_BF, CATBF_IWCMD, DBG_LVL_TRACE, ("conf_ibf_ncol=%d\n", man_bf_sta_rec->conf_ibf_ncol));
	MTWF_LOG(DBG_CAT_BF, CATBF_IWCMD, DBG_LVL_TRACE, ("conf_ibf_nrow=%d\n", man_bf_sta_rec->conf_ibf_nrow));
	MTWF_LOG(DBG_CAT_BF, CATBF_IWCMD, DBG_LVL_TRACE, ("conf_nr_bw160=0x%2X\n", man_bf_sta_rec->conf_nr_bw160));
	MTWF_LOG(DBG_CAT_BF, CATBF_IWCMD, DBG_LVL_TRACE, ("conf_nc_bw160=0x%2X\n", man_bf_sta_rec->conf_nc_bw160));

	return TRUE;
}

INT Set_StaRecBfRead(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UINT16           u2WlanIdx;
	BOOLEAN          fgStatus = FALSE;

	u2WlanIdx = os_str_tol(arg, 0, 10);

	if (CmdETxBfStaRecRead(pAd,
						   u2WlanIdx) == STATUS_TRUE)
		fgStatus = TRUE;

	return fgStatus;
}


INT Set_TxBfAwareCtrl(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	BOOLEAN fgBfAware, fgStatus = FALSE;

	fgBfAware = os_str_tol(arg, 0, 10);

	if (CmdTxBfAwareCtrl(pAd, fgBfAware) == STATUS_TRUE)
		fgStatus = TRUE;

	return fgStatus;
}

INT set_dynsnd_en_intr(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	BOOLEAN intr_en;
	INT status = 0;

	intr_en = os_str_tol(arg, 0, 10);

	if (cmd_txbf_en_dynsnd_intr(pAd, intr_en) == STATUS_TRUE)
		status = 1;

	return status;
}

INT Set_HostReportTxLatency(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	BOOLEAN	fgStatus = FALSE;
	UCHAR	ucEnable;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	struct wifi_dev *wdev = get_wdev_by_ioctl_idx_and_iftype(pAd, pObj->ioctl_if, pObj->ioctl_if_type);

	if (wdev == NULL) {
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			    ("%s: wdev is null.\n", __func__));
		return FALSE;
	}

	ucEnable = os_str_tol(arg, 0, 10);

	/* Set Host Report Tx Latency, Don't need input OwnMac Info */
	if (CmdExtCmdCfgUpdate(
			pAd,
			wdev,
			CFGINFO_HOSTREPORT_TXLATENCY_FEATURE,
			&ucEnable) == TRUE)
		fgStatus = TRUE;

	return fgStatus;
}

INT Set_RxFilterDropCtrlFrame(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	INT8 *value = NULL;
	UINT16 input[3] = {0};
	UINT8 i, argCnt = 0, ucAction = 0;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	struct wifi_dev *wdev = get_wdev_by_ioctl_idx_and_iftype(pAd, pObj->ioctl_if, pObj->ioctl_if_type);

	for (i = 0, value = rstrtok(arg, "-"); value; value = rstrtok(NULL, "-"), i++) {
		input[i] = os_str_toul(value, 0, 10);
		argCnt++;
	}

	/* Check Item Count */
	if ((argCnt == 0) || (argCnt != 3))
		goto error;

	if (wdev == NULL) {
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			    ("%s: wdev is null.\n", __func__));
		return FALSE;
	}

	/* Drop RTS */
	if (input[0] == 1)
		ucAction |= CFGINFO_DROP_RTS_CTRL_FRAME;

	/* Drop CTS */
	if (input[1] == 1)
		ucAction |= CFGINFO_DROP_CTS_CTRL_FRAME;

	/* Drop unwanted Ctrl Frame */
	if (input[2] == 1)
		ucAction |= CFGINFO_DROP_UNWANTED_CTRL_FRAME;

	if (CmdExtCmdCfgUpdate(
			pAd,
			wdev,
			CFGINFO_RX_FILTER_DROP_CTRL_FRAME_FEATURE,
			&ucAction) != STATUS_TRUE)
		goto error;

	return TRUE;

error:
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("Wrong Cmd Format. Plz input:\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("iwpriv ra0 set rx_filter_ctrl=[0]-[1]-[2]\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("  [0]=0: Don't Drop RTS CTRL Frame\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("	   1: Drop RTS CTRL Frame)\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("  [1]=0: Don't Drop CTS CTRL Frame\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("	   1: Drop CTS CTRL Frame)\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("  [2]=0: Don't Drop Unwanted CTRL Frame\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("	   1: Drop Unwanted CTRL Frame)\n"));

	return FALSE;
}

INT Set_CertCfg(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	BOOLEAN	fgStatus = FALSE;
	UCHAR	ucEnable;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	struct wifi_dev *wdev = get_wdev_by_ioctl_idx_and_iftype(pAd, pObj->ioctl_if, pObj->ioctl_if_type);

	if (wdev == NULL) {
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			    ("%s: wdev is null.\n", __func__));
		return FALSE;
	}

	ucEnable = os_str_tol(arg, 0, 10);

	/* Set Host Report Tx Latency, Don't need input OwnMac Info */
	if (CmdExtCmdCfgUpdate(
			pAd,
			wdev,
			CFGINFO_CERT_CFG_FEATURE,
			&ucEnable) == TRUE)
		fgStatus = TRUE;

	return fgStatus;
}

#ifdef CFG_SUPPORT_MU_MIMO
INT set_dynsnd_cfg_dmcs(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	CHAR *tmp = NULL;
	INT status = 0;
	UINT8 mcs_index, mcs_th;

	tmp = strsep(&arg, ":");

	if (tmp != NULL)
		mcs_index = os_str_tol(tmp, 0, 10);
	else
		goto error;

	tmp = strsep(&arg, "");

	if (tmp != NULL)
		mcs_th = os_str_tol(tmp, 0, 10);
	else
		goto error;

	if (cmd_txbf_cfg_dynsnd_dmcsth(pAd, mcs_index, mcs_th) == STATUS_TRUE)
		status = 1;

error:
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			 ("%s:(status = %d\n", __func__, status));
	return status;
}

INT set_dynsnd_en_mu_intr(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	CHAR *tmp = NULL;
	INT status = 0;
	BOOLEAN mu_intr_en;
	UINT8 pfid;

	tmp = strsep(&arg, ":");

	if (tmp != NULL)
		mu_intr_en = os_str_tol(tmp, 0, 10);
	else
		goto error;

	tmp = strsep(&arg, "");

	if (tmp != NULL)
		pfid = os_str_tol(tmp, 0, 10);
	else
		goto error;

	if (cmd_txbf_en_dynsnd_pfid_intr(pAd, mu_intr_en, pfid) == STATUS_TRUE)
		status = 1;

error:
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			 ("%s:(status = %d\n", __func__, status));
	return status;
}
#endif /* CFG_SUPPORT_MU_MIMO */

#ifdef CONFIG_ATE
INT Set_StaRecCmmUpdate(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	PMAC_TABLE_ENTRY pEntry;
	CHAR			 *value;
	UCHAR			 Input[9];
	INT				 i;
	CHAR             BssIdx, WlanIdx;
	BOOLEAN          fgStatus = FALSE;

	if (strlen(arg) != 26)
		return FALSE;

	for (i = 0, value = rstrtok(arg, ":"); value; value = rstrtok(NULL, ":")) {
		if ((strlen(value) != 2) || (!isxdigit(*value)) || (!isxdigit(*(value + 1))))
			return FALSE;  /*Invalid*/

		AtoH(value, &Input[i++], 1);
	}

	WlanIdx       = Input[0];
	BssIdx        = Input[1];
	pEntry = &pAd->MacTab.Content[WlanIdx];

	if (pEntry == NULL)
		return FALSE;

	pEntry->Aid     = Input[2];
	pEntry->Addr[0] = Input[3];
	pEntry->Addr[1] = Input[4];
	pEntry->Addr[2] = Input[5];
	pEntry->Addr[3] = Input[6];
	pEntry->Addr[4] = Input[7];
	pEntry->Addr[5] = Input[8];
	{
		STA_REC_CFG_T StaCfg;

		os_zero_mem(&StaCfg, sizeof(STA_REC_CFG_T));
		StaCfg.MuarIdx = 0;
		StaCfg.ConnectionState = TRUE;
		StaCfg.ConnectionType = CONNECTION_INFRA_AP;
		StaCfg.u4EnableFeature = (1 << STA_REC_BASIC_STA_RECORD);
		StaCfg.ucBssIndex = BssIdx;
		StaCfg.u2WlanIdx = WlanIdx;
		StaCfg.pEntry = pEntry;
		StaCfg.IsNewSTARec = TRUE;

		if (CmdExtStaRecUpdate(pAd, StaCfg) == STATUS_TRUE)
			fgStatus = TRUE;
	}
	return fgStatus;
}

INT Set_BssInfoUpdate(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	CHAR			 *value;
	UCHAR			 Input[8];
	UCHAR            Bssid[MAC_ADDR_LEN];
	INT				 i;
	CHAR             OwnMacIdx, BssIdx;
	BOOLEAN          fgStatus = FALSE;
	BSS_INFO_ARGUMENT_T bss_info_argument;
	UCHAR control_band_idx;
#ifdef CONFIG_WLAN_SERVICE
	struct service_test *serv_test;
	serv_test = (struct service_test *)(pAd->serv.serv_handle);
	control_band_idx = serv_test->ctrl_band_idx;
#else
	struct _ATE_CTRL *ATECtrl = &(pAd->ATECtrl);
	control_band_idx = ATECtrl->control_band_idx;
#endif /*  CONFIG_WLAN_SERVICE */

	if (strlen(arg) != 23)
		return FALSE;

	for (i = 0, value = rstrtok(arg, ":"); value; value = rstrtok(NULL, ":")) {
		if ((strlen(value) != 2) || (!isxdigit(*value)) || (!isxdigit(*(value + 1))))
			return FALSE;  /*Invalid*/

		AtoH(value, &Input[i++], 1);
	}

	OwnMacIdx = Input[0];
	BssIdx    = Input[1];
	Bssid[0]  = Input[2];
	Bssid[1]  = Input[3];
	Bssid[2]  = Input[4];
	Bssid[3]  = Input[5];
	Bssid[4]  = Input[6];
	Bssid[5]  = Input[7];
	NdisZeroMemory(&bss_info_argument, sizeof(BSS_INFO_ARGUMENT_T));

	bss_info_argument.OwnMacIdx = OwnMacIdx;
	bss_info_argument.ucBssIndex = BssIdx;
	os_move_mem(bss_info_argument.Bssid, Bssid, MAC_ADDR_LEN);
	bss_info_argument.bmc_wlan_idx = 1; /* MCAST_WCID which needs to be modified by Patrick; */
	bss_info_argument.NetworkType = NETWORK_INFRA;
	bss_info_argument.u4ConnectionType = CONNECTION_INFRA_AP;
	bss_info_argument.CipherSuit = CIPHER_SUIT_NONE;
	bss_info_argument.bss_state = BSS_ACTIVE;
	bss_info_argument.ucBandIdx = control_band_idx;
	bss_info_argument.u4BssInfoFeature = BSS_INFO_OWN_MAC_FEATURE | BSS_INFO_BASIC_FEATURE;

	if (AsicBssInfoUpdate(pAd, &bss_info_argument) == STATUS_TRUE)
		fgStatus = TRUE;

	return fgStatus;
}

INT Set_DevInfoUpdate(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	CHAR			 *value;
	UCHAR			 Input[8];
	UCHAR            OwnMacAddr[MAC_ADDR_LEN];
	INT				 i;
	CHAR             OwnMacIdx;
	BOOLEAN          fgStatus = FALSE;
	UINT8		     BandIdx = 0;

	if (strlen(arg) != 23)
		return FALSE;

	for (i = 0, value = rstrtok(arg, ":"); value; value = rstrtok(NULL, ":")) {
		if ((strlen(value) != 2) || (!isxdigit(*value)) || (!isxdigit(*(value + 1))))
			return FALSE;  /*Invalid*/

		AtoH(value, &Input[i++], 1);
	}
	OwnMacIdx     = Input[0];
	OwnMacAddr[0] = Input[1];
	OwnMacAddr[1] = Input[2];
	OwnMacAddr[2] = Input[3];
	OwnMacAddr[3] = Input[4];
	OwnMacAddr[4] = Input[5];
	OwnMacAddr[5] = Input[6];
	BandIdx = Input[7];

	if (AsicDevInfoUpdate(
			pAd,
			OwnMacIdx,
			OwnMacAddr,
			BandIdx,
			TRUE,
			DEVINFO_ACTIVE_FEATURE) == STATUS_TRUE)
		fgStatus = TRUE;

	return fgStatus;
}
#endif /*CONFIG_ATE*/

#endif /* MT_MAC */

#if defined(MT_MAC)
INT	Set_ITxBfEn_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UINT16 i;
	UCHAR ucITxBfEn;
	INT   u4Status = FALSE;
	MAC_TABLE_ENTRY		*pEntry;
	TXBF_STATUS_INFO    TxBfInfo;
	struct wifi_dev *wdev = NULL;

	ucITxBfEn = os_str_tol(arg, 0, 10);

	if (ucITxBfEn > 1)
		return FALSE;

	for (i = 0; VALID_UCAST_ENTRY_WCID(pAd, i); i++) {
		pEntry = &pAd->MacTab.Content[i];

		if (!IS_ENTRY_NONE(pEntry)) {
			wdev = pEntry->wdev;
			TxBfInfo.ucTxPathNum = pAd->Antenna.field.TxPath;
			TxBfInfo.ucRxPathNum = pAd->Antenna.field.RxPath;
#ifdef DBDC_MODE
			if (pAd->CommonCfg.dbdc_mode) {
				UINT8 band_idx = HcGetBandByWdev(wdev);

				if (band_idx == DBDC_BAND0) {
					TxBfInfo.ucTxPathNum = pAd->dbdc_band0_tx_path;
					TxBfInfo.ucRxPathNum = pAd->dbdc_band0_rx_path;
				} else {
					TxBfInfo.ucTxPathNum = pAd->dbdc_band1_tx_path;
					TxBfInfo.ucRxPathNum = pAd->dbdc_band1_rx_path;
				}
			}
#endif

#ifdef ANTENNA_CONTROL_SUPPORT
			{
				UINT8 BandIdx = HcGetBandByWdev(wdev);
				if (pAd->bAntennaSetAPEnable[BandIdx]) {
					TxBfInfo.ucTxPathNum = pAd->TxStream[BandIdx];
					TxBfInfo.ucRxPathNum = pAd->RxStream[BandIdx];
				}
			}
#endif /* ANTENNA_CONTROL_SUPPORT */

			TxBfInfo.ucPhyMode   = wdev->PhyMode;
			TxBfInfo.u2Channel   = wdev->channel;
			TxBfInfo.pHtTxBFCap  = &pAd->CommonCfg.HtCapability.TxBFCap;
			TxBfInfo.cmmCfgETxBfIncapable  = pAd->CommonCfg.ETxBfIncapable;
			TxBfInfo.cmmCfgETxBfNoncompress = pAd->CommonCfg.ETxBfNoncompress;
#ifdef VHT_TXBF_SUPPORT
			TxBfInfo.pVhtTxBFCap = &pAd->CommonCfg.vht_cap_ie.vht_cap;
#endif
			TxBfInfo.u4WTBL1 = pAd->mac_ctrl.wtbl_base_addr[0] + i * pAd->mac_ctrl.wtbl_entry_size[0];
			TxBfInfo.u4WTBL2 = pAd->mac_ctrl.wtbl_base_addr[1] + i * pAd->mac_ctrl.wtbl_entry_size[1];
			TxBfInfo.ucETxBfTxEn = FALSE;
			TxBfInfo.ucITxBfTxEn = ucITxBfEn;
			TxBfInfo.u2Wcid      = i;
			TxBfInfo.ucBW        = pEntry->HTPhyMode.field.BW;
			TxBfInfo.ucNDPARate  = 2; /* MCS2 */
			u4Status = AsicTxBfEnCondProc(pAd, &TxBfInfo);
		}
	}

	return u4Status;
}

#endif /* MT_MAC */
#endif /* TXBF_SUPPORT */

#ifdef VHT_TXBF_SUPPORT
/*
	The VhtNDPA sounding inupt string format should be xx:xx:xx:xx:xx:xx-d,
		=>The six 2 digit hex-decimal number previous are the Mac address,
		=>The seventh decimal number is the MCS value.
*/
INT Set_VhtNDPA_Sounding_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UCHAR mac[6];
	UINT mcs;
	RTMP_STRING *token;
	RTMP_STRING sepValue[] = ":", DASH = '-';
	INT i;
	MAC_TABLE_ENTRY *pEntry = NULL;

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("\n%s\n", arg));

	/*Mac address acceptable format 01:02:03:04:05:06 length 17 plus the "-" and MCS value in decimal format.*/
	if (strlen(arg) < 19)
		return FALSE;

	token = strchr(arg, DASH);

	if ((token != NULL) && (strlen(token) > 1)) {
		mcs = (UINT)os_str_tol((token + 1), 0, 10);
		*token = '\0';

		for (i = 0, token = rstrtok(arg, &sepValue[0]); token; token = rstrtok(NULL, &sepValue[0]), i++) {
			if ((strlen(token) != 2) || (!isxdigit(*token)) || (!isxdigit(*(token + 1))))
				return FALSE;

			AtoH(token, (&mac[i]), 1);
		}

		if (i != 6)
			return FALSE;

		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\n%02x:%02x:%02x:%02x:%02x:%02x-%02x\n",
				 PRINT_MAC(mac), mcs));
		pEntry = MacTableLookup(pAd, (PUCHAR) mac);

		if (pEntry) {
#ifdef SOFT_SOUNDING
			pEntry->snd_rate.field.MODE = MODE_VHT;
			pEntry->snd_rate.field.BW = (mcs / 100) > BW_80 ? BW_80 : (mcs / 100);
			mcs %= 100;
			pEntry->snd_rate.field.MCS = ((mcs / 10) << 4 | (mcs % 10));
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
					 ("%s():Trigger VHT NDPA Sounding=%02x:%02x:%02x:%02x:%02x:%02x, snding rate=VHT-%sHz, %dSS-MCS%d\n",
					  __func__, PRINT_MAC(mac),
					  get_bw_str(pEntry->snd_rate.field.BW),
					  (pEntry->snd_rate.field.MCS >> 4) + 1,
					  pEntry->snd_rate.field.MCS & 0xf));
#endif
			trigger_vht_ndpa(pAd, pEntry);
		}

		return TRUE;
	}

	return FALSE;
}
#endif /* VHT_TXBF_SUPPORT */

#if defined(MT_MAC)
#ifdef TXBF_SUPPORT

INT Set_TxBfProfileTag_Help(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, (
				 "========================================================================================================================\n"
				 "TxBfProfile Tag1 setting example :\n"
				 "iwpriv ra0 set TxBfProfileTagPfmuIdx  =xx\n"
				 "iwpriv ra0 set TxBfProfileTagBfType   =xx (0: iBF; 1: eBF)\n"
				 "iwpriv ra0 set TxBfProfileTagBw       =xx (0/1/2/3 : BW20/40/80/160NC)\n"
				 "iwpriv ra0 set TxBfProfileTagSuMu     =xx (0:SU, 1:MU)\n"
				 "iwpriv ra0 set TxBfProfileTagInvalid  =xx (0: valid, 1: invalid)\n"
				 "iwpriv ra0 set TxBfProfileTagMemAlloc =xx:xx:xx:xx:xx:xx:xx:xx (mem_row, mem_col), ..\n"
				 "iwpriv ra0 set TxBfProfileTagMatrix   =nrow:nol:ng:LM\n"
				 "iwpriv ra0 set TxBfProfileTagSnr      =SNR_STS0:SNR_STS1:SNR_STS2:SNR_STS3\n"
				 "\n\n"
				 "TxBfProfile Tag2 setting example :\n"
				 "iwpriv ra0 set TxBfProfileTagSmtAnt   =xx (11:0)\n"
				 "iwpriv ra0 set TxBfProfileTagSeIdx    =xx\n"
				 "iwpriv ra0 set TxBfProfileTagRmsdThrd =xx\n"
				 "iwpriv ra0 set TxBfProfileTagMcsThrd  =xx:xx:xx:xx:xx:xx (MCS TH L1SS:S1SS:L2SS:....)\n"
				 "iwpriv ra0 set TxBfProfileTagTimeOut  =xx\n"
				 "iwpriv ra0 set TxBfProfileTagDesiredBw=xx (0/1/2/3 : BW20/40/80/160NC)\n"
				 "iwpriv ra0 set TxBfProfileTagDesiredNc=xx\n"
				 "iwpriv ra0 set TxBfProfileTagDesiredNr=xx\n"
				 "\n\n"
				 "Read TxBf profile Tag :\n"
				 "iwpriv ra0 set TxBfProfileTagRead     =xx (PFMU ID)\n"
				 "\n"
				 "Write TxBf profile Tag :\n"
				 "iwpriv ra0 set TxBfProfileTagWrite    =xx (PFMU ID)\n"
				 "When you use one of relative CMD to update one of tag parameters, you should call TxBfProfileTagWrite to update Tag\n"
				 "\n\n"
				 "Read TxBf profile Data	:\n"
				 "iwpriv ra0 set TxBfProfileDataRead    =xx (PFMU ID)\n"
				 "\n"
				 "Write TxBf profile Data :\n"
				 "iwpriv ra0 set TxBfProfileDataWrite   =BW :subcarrier:phi11:psi2l:Phi21:Psi31:Phi31:Psi41:Phi22:Psi32:Phi32:Psi42:Phi33:Psi43\n"
				 "iwpriv ra0 set TxBfProfileDataWriteAll=Profile ID : BW (BW       : 0x00 (20M) , 0x01 (40M), 0x02 (80M), 0x3 (160M)\n"
				 "When you use CMD TxBfProfileDataWrite to update profile data per subcarrier, you should call TxBfProfileDataWriteAll to update all of\n"
				 "subcarrier's profile data.\n\n"
				 "Read TxBf profile PN	:\n"
				 "iwpriv ra0 set TxBfProfilePnRead      =xx (PFMU ID)\n"
				 "\n"
				 "Write TxBf profile PN :\n"
				 "iwpriv ra0 set TxBfProfilePnWrite     =Profile ID:BW:1STS_Tx0:1STS_Tx1:1STS_Tx2:1STS_Tx3:2STS_Tx0:2STS_Tx1:2STS_Tx2:2STS_Tx3:3STS_Tx1:3STS_Tx2:3STS_Tx3\n"
				 "========================================================================================================================\n"));
	return TRUE;
}

INT Set_TxBfProfileTag_PfmuIdx(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	struct _RTMP_CHIP_OP *ops = hc_get_chip_ops(pAd->hdev_ctrl);
	UINT16 profileIdx;

	profileIdx = (UINT16)os_str_tol(arg, 0, 10);
	if (ops->set_txbf_pfmu_tag)
		return ops->set_txbf_pfmu_tag(pAd->hdev_ctrl, TAG1_PFMU_ID, profileIdx);
	else
		return FALSE;
}

INT Set_TxBfProfileTag_BfType(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	struct _RTMP_CHIP_OP *ops = hc_get_chip_ops(pAd->hdev_ctrl);
	UINT8 ucBfType;

	ucBfType = (UINT8)os_str_tol(arg, 0, 10);
	if (ops->set_txbf_pfmu_tag)
		return ops->set_txbf_pfmu_tag(pAd->hdev_ctrl, TAG1_IEBF, ucBfType);
	else
		return FALSE;
}

INT Set_TxBfProfileTag_DBW(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	struct _RTMP_CHIP_OP *ops = hc_get_chip_ops(pAd->hdev_ctrl);
	UINT8 ucBw;

	ucBw = (UINT8)os_str_tol(arg, 0, 10);
	if (ops->set_txbf_pfmu_tag)
		return ops->set_txbf_pfmu_tag(pAd->hdev_ctrl, TAG1_DBW, ucBw);
	else
		return FALSE;
}

INT Set_TxBfProfileTag_SuMu(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	struct _RTMP_CHIP_OP *ops = hc_get_chip_ops(pAd->hdev_ctrl);
	UINT8 ucSuMu;

	ucSuMu = (UINT8)os_str_tol(arg, 0, 10);
	if (ops->set_txbf_pfmu_tag)
		return ops->set_txbf_pfmu_tag(pAd->hdev_ctrl, TAG1_SU_MU, ucSuMu);
	else
		return FALSE;
}

INT Set_TxBfProfileTag_InValid(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	struct _RTMP_CHIP_OP *ops = hc_get_chip_ops(pAd->hdev_ctrl);
	UINT8 InValid;

	InValid = (UINT8)os_str_tol(arg, 0, 10);
	if (ops->set_txbf_pfmu_tag)
		return ops->set_txbf_pfmu_tag(pAd->hdev_ctrl, TAG1_INVALID, InValid);
	else
		return FALSE;
}

INT Set_TxBfProfileTag_Mem(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	struct _RTMP_CHIP_OP *ops = hc_get_chip_ops(pAd->hdev_ctrl);
	UCHAR   Input[8];
	CHAR	 *value;
	INT	i;
	UINT8 aMemAddrColIdx[4], aMemAddrRowIdx[4];

	/* mem col0:row0:col1:row1:col2:row2:col3:row3 */
	if (strlen(arg) != 23)
		return FALSE;

	for (i = 0, value = rstrtok(arg, ":"); value; value = rstrtok(NULL, ":")) {
		if ((!isxdigit(*value)) || (!isxdigit(*(value + 1))))
			return FALSE;  /*Invalid*/

		AtoH(value, &Input[i++], 1);
	}

	aMemAddrColIdx[0] = Input[0];
	aMemAddrRowIdx[0] = Input[1];
	aMemAddrColIdx[1] = Input[2];
	aMemAddrRowIdx[1] = Input[3];
	aMemAddrColIdx[2] = Input[4];
	aMemAddrRowIdx[2] = Input[5];
	aMemAddrColIdx[3] = Input[6];
	aMemAddrRowIdx[3] = Input[7];

	if (ops->set_txbf_pfmu_tag) {
		ops->set_txbf_pfmu_tag(pAd->hdev_ctrl, TAG1_MEM_ROW0, aMemAddrRowIdx[0]);
		ops->set_txbf_pfmu_tag(pAd->hdev_ctrl, TAG1_MEM_ROW1, aMemAddrRowIdx[1]);
		ops->set_txbf_pfmu_tag(pAd->hdev_ctrl, TAG1_MEM_ROW2, aMemAddrRowIdx[2]);
		ops->set_txbf_pfmu_tag(pAd->hdev_ctrl, TAG1_MEM_ROW3, aMemAddrRowIdx[3]);
		ops->set_txbf_pfmu_tag(pAd->hdev_ctrl, TAG1_MEM_COL0, aMemAddrColIdx[0]);
		ops->set_txbf_pfmu_tag(pAd->hdev_ctrl, TAG1_MEM_COL1, aMemAddrColIdx[1]);
		ops->set_txbf_pfmu_tag(pAd->hdev_ctrl, TAG1_MEM_COL2, aMemAddrColIdx[2]);
		ops->set_txbf_pfmu_tag(pAd->hdev_ctrl, TAG1_MEM_COL3, aMemAddrColIdx[3]);

		return TRUE;
	} else
		return FALSE;
}

INT Set_TxBfProfileTag_Matrix(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	struct _RTMP_CHIP_OP *ops = hc_get_chip_ops(pAd->hdev_ctrl);
	UCHAR   Input[6];
	CHAR	 *value;
	INT	i;
	UINT8   ucNrow, ucNcol, ucNgroup, ucLM, ucCodeBook, ucHtcExist;

	/* nrow:nol:ng:LM:CodeBook:HtcExist */
	if (strlen(arg) != 17)
		return FALSE;

	for (i = 0, value = rstrtok(arg, ":"); value; value = rstrtok(NULL, ":")) {
		if ((!isxdigit(*value)) || (!isxdigit(*(value + 1))))
			return FALSE;  /*Invalid*/

		AtoH(value, &Input[i++], 1);
	}

	ucNrow     = Input[0];
	ucNcol     = Input[1];
	ucNgroup   = Input[2];
	ucLM       = Input[3];
	ucCodeBook = Input[4];
	ucHtcExist = Input[5];

	if (ops->set_txbf_pfmu_tag) {
		ops->set_txbf_pfmu_tag(pAd->hdev_ctrl, TAG1_NR, ucNrow);
		ops->set_txbf_pfmu_tag(pAd->hdev_ctrl, TAG1_NC, ucNcol);
		ops->set_txbf_pfmu_tag(pAd->hdev_ctrl, TAG1_NG, ucNgroup);
		ops->set_txbf_pfmu_tag(pAd->hdev_ctrl, TAG1_LM, ucLM);
		ops->set_txbf_pfmu_tag(pAd->hdev_ctrl, TAG1_CODEBOOK, ucCodeBook);
		ops->set_txbf_pfmu_tag(pAd->hdev_ctrl, TAG1_HTC, ucHtcExist);

		return TRUE;
	} else
		return FALSE;

}

INT set_txbf_prof_tag_ru_range(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	struct _RTMP_CHIP_OP *ops = hc_get_chip_ops(pAd->hdev_ctrl);
	UINT8 input[2];
	UINT8 ru_start, ru_end;
	CHAR *value;
	INT i;

	for (i = 0, value = rstrtok(arg, ":"); value; value = rstrtok(NULL, ":"), i++)
		input[i] = (UINT8)os_str_toul(value, 0, 10);

	if (i != 2) {
		MTWF_LOG(DBG_CAT_FW, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("Error: Un-expected format!\n"));
		return FALSE;
	}

	ru_start = input[0];
	ru_end = input[1];

	if (ops->set_txbf_pfmu_tag) {
		ops->set_txbf_pfmu_tag(pAd->hdev_ctrl, TAG1_RU_START, ru_start);
		ops->set_txbf_pfmu_tag(pAd->hdev_ctrl, TAG1_RU_END, ru_end);

		return TRUE;
	} else
		return FALSE;
}

INT set_txbf_prof_tag_mob_cal_en(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	struct _RTMP_CHIP_OP *ops = hc_get_chip_ops(pAd->hdev_ctrl);
	UINT8 mob_cal_en;

	mob_cal_en = (UINT8)os_str_tol(arg, 0, 10);
	if (ops->set_txbf_pfmu_tag)
		return ops->set_txbf_pfmu_tag(pAd->hdev_ctrl, TAG1_MOB_CAL_EN, mob_cal_en);
	else
		return FALSE;
}

INT Set_TxBfProfileTag_SNR(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	struct _RTMP_CHIP_OP *ops = hc_get_chip_ops(pAd->hdev_ctrl);
	UCHAR   Input[8];
	CHAR	 *value;
	INT	i;
	UINT8 ucSNR_STS0, ucSNR_STS1, ucSNR_STS2, ucSNR_STS3;
	UINT8 ucSNR_STS4, ucSNR_STS5, ucSNR_STS6, ucSNR_STS7;

	if ((strlen(arg) != 11) && (strlen(arg) != 23))
		return FALSE;

	for (i = 0, value = rstrtok(arg, ":"); value; value = rstrtok(NULL, ":")) {
		if ((!isxdigit(*value)) || (!isxdigit(*(value + 1))))
			return FALSE;  /*Invalid*/

		AtoH(value, &Input[i++], 1);
	}

	ucSNR_STS0 = Input[0];
	ucSNR_STS1 = Input[1];
	ucSNR_STS2 = Input[2];
	ucSNR_STS3 = Input[3];
	ucSNR_STS4 = Input[4];
	ucSNR_STS5 = Input[5];
	ucSNR_STS6 = Input[6];
	ucSNR_STS7 = Input[7];

	if (ops->set_txbf_pfmu_tag) {
		ops->set_txbf_pfmu_tag(pAd->hdev_ctrl, TAG1_SNR_STS0, ucSNR_STS0);
		ops->set_txbf_pfmu_tag(pAd->hdev_ctrl, TAG1_SNR_STS1, ucSNR_STS1);
		ops->set_txbf_pfmu_tag(pAd->hdev_ctrl, TAG1_SNR_STS2, ucSNR_STS2);
		ops->set_txbf_pfmu_tag(pAd->hdev_ctrl, TAG1_SNR_STS3, ucSNR_STS3);
		ops->set_txbf_pfmu_tag(pAd->hdev_ctrl, TAG1_SNR_STS4, ucSNR_STS4);
		ops->set_txbf_pfmu_tag(pAd->hdev_ctrl, TAG1_SNR_STS5, ucSNR_STS5);
		ops->set_txbf_pfmu_tag(pAd->hdev_ctrl, TAG1_SNR_STS6, ucSNR_STS6);
		ops->set_txbf_pfmu_tag(pAd->hdev_ctrl, TAG1_SNR_STS7, ucSNR_STS7);

		return TRUE;
	} else
		return FALSE;
}

INT Set_TxBfProfileTag_SmartAnt(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	struct _RTMP_CHIP_OP *ops = hc_get_chip_ops(pAd->hdev_ctrl);
	UINT32 SmartAnt;

	SmartAnt = (UINT32)os_str_tol(arg, 0, 10);
	if (ops->set_txbf_pfmu_tag)
		return ops->set_txbf_pfmu_tag(pAd->hdev_ctrl, TAG2_SMART_ANT, SmartAnt);
	else
		return FALSE;
}

INT Set_TxBfProfileTag_SeIdx(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	struct _RTMP_CHIP_OP *ops = hc_get_chip_ops(pAd->hdev_ctrl);
	UINT8 ucSeIdx;

	ucSeIdx = (UINT8)os_str_tol(arg, 0, 10);
	if (ops->set_txbf_pfmu_tag)
		return ops->set_txbf_pfmu_tag(pAd->hdev_ctrl, TAG2_SE_ID, ucSeIdx);
	else
		return FALSE;
}

INT Set_TxBfProfileTag_RmsdThrd(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	struct _RTMP_CHIP_OP *ops = hc_get_chip_ops(pAd->hdev_ctrl);
	UINT8 ucRmsdThrd;

	ucRmsdThrd = (UINT8)os_str_tol(arg, 0, 10);
	if (ops->set_txbf_pfmu_tag)
		return ops->set_txbf_pfmu_tag(pAd->hdev_ctrl, TAG2_RMSD_THRESHOLD, ucRmsdThrd);
	else
		return FALSE;
}

INT Set_TxBfProfileTag_McsThrd(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UCHAR   Input[6];
	CHAR	 *value;
	INT	i;
	UCHAR   ucMcsLss[3], ucMcsSss[3];

	if (strlen(arg) != 17)
		return FALSE;

	for (i = 0, value = rstrtok(arg, ":"); value; value = rstrtok(NULL, ":")) {
		if ((!isxdigit(*value)) || (!isxdigit(*(value + 1))))
			return FALSE;  /*Invalid*/

		AtoH(value, &Input[i++], 1);
	}

	ucMcsLss[0] = Input[0];
	ucMcsSss[0] = Input[1];
	ucMcsLss[1] = Input[2];
	ucMcsSss[1] = Input[3];
	ucMcsLss[2] = Input[4];
	ucMcsSss[2] = Input[5];
#ifndef DOT11_HE_AX
	TxBfProfileTag_McsThd(&pAd->rPfmuTag2,
						  ucMcsLss,
						  ucMcsSss);
#endif
	return TRUE;
}

INT Set_TxBfProfileTag_TimeOut(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	struct _RTMP_CHIP_OP *ops = hc_get_chip_ops(pAd->hdev_ctrl);
	UINT8 ucTimeOut;

	ucTimeOut = (UINT8)os_str_tol(arg, 0, 10);
	if (ops->set_txbf_pfmu_tag)
		return ops->set_txbf_pfmu_tag(pAd->hdev_ctrl, TAG2_IBF_TIMEOUT, ucTimeOut);
	else
		return FALSE;
}

INT Set_TxBfProfileTag_DesiredBW(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	struct _RTMP_CHIP_OP *ops = hc_get_chip_ops(pAd->hdev_ctrl);
	UINT8 ucDesiredBW;

	ucDesiredBW = (UINT8)os_str_tol(arg, 0, 10);
	if (ops->set_txbf_pfmu_tag)
		return ops->set_txbf_pfmu_tag(pAd->hdev_ctrl, TAG2_IBF_DBW, ucDesiredBW);
	else
		return FALSE;
}

INT Set_TxBfProfileTag_DesiredNc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	struct _RTMP_CHIP_OP *ops = hc_get_chip_ops(pAd->hdev_ctrl);
	UINT8 ucDesiredNc;

	ucDesiredNc = (UINT8)os_str_tol(arg, 0, 10);
	if (ops->set_txbf_pfmu_tag)
		return ops->set_txbf_pfmu_tag(pAd->hdev_ctrl, TAG2_IBF_NCOL, ucDesiredNc);
	else
		return FALSE;
}

INT Set_TxBfProfileTag_DesiredNr(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	struct _RTMP_CHIP_OP *ops = hc_get_chip_ops(pAd->hdev_ctrl);
	UINT8 ucDesiredNr;

	ucDesiredNr = (UINT8)os_str_tol(arg, 0, 10);
	if (ops->set_txbf_pfmu_tag)
		return ops->set_txbf_pfmu_tag(pAd->hdev_ctrl, TAG2_IBF_NROW, ucDesiredNr);
	else
		return FALSE;
}

INT set_txbf_prof_tag_ru_alloc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	struct _RTMP_CHIP_OP *ops = hc_get_chip_ops(pAd->hdev_ctrl);
	UINT8 ru_alloc;

	ru_alloc = (UINT8)os_str_tol(arg, 0, 10);
	if (ops->set_txbf_pfmu_tag)
		return ops->set_txbf_pfmu_tag(pAd->hdev_ctrl, TAG2_IBF_RU_ALLOC, ru_alloc);
	else
		return FALSE;
}

INT Set_TxBfProfileTagRead(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UCHAR   profileIdx;
	BOOLEAN fgBFer;
	UCHAR   Input[2];
	CHAR	 *value;
	INT	i;

	if (strlen(arg) != 5)
		return FALSE;

	for (i = 0, value = rstrtok(arg, ":"); value; value = rstrtok(NULL, ":")) {
		if ((!isxdigit(*value)) || (!isxdigit(*(value + 1))))
			return FALSE;  /*Invalid*/

		AtoH(value, &Input[i++], 1);
	}

	profileIdx = Input[0];
	fgBFer     = Input[1];

	return TxBfProfileTagRead(pAd, profileIdx, fgBFer);
}


INT Set_TxBfProfileTagWrite(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	struct _RTMP_CHIP_OP *ops = hc_get_chip_ops(pAd->hdev_ctrl);
	UCHAR profileIdx;

	profileIdx = (UCHAR) os_str_tol(arg, 0, 10);

	if (ops->write_txbf_pfmu_tag)
		return ops->write_txbf_pfmu_tag(pAd->hdev_ctrl, profileIdx);
	else
		return FALSE;
}

INT Set_TxBfProfileDataRead(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	CHAR	 *value;
	UCHAR   Input[4];
	INT	i;
	UCHAR   profileIdx, subcarrIdx_H, subcarrIdx_L;
	BOOLEAN fgBFer;
	USHORT  subcarrIdx;

	/* Profile Select : Subcarrier Select */
	if (strlen(arg) != 11)
		return FALSE;

	for (i = 0, value = rstrtok(arg, ":"); value; value = rstrtok(NULL, ":")) {
		if ((!isxdigit(*value)) || (!isxdigit(*(value + 1))))
			return FALSE;  /*Invalid*/

		AtoH(value, &Input[i++], 1);
	}

	profileIdx   = Input[0];
	fgBFer       = Input[1];
	subcarrIdx_H = Input[2];
	subcarrIdx_L = Input[3];
	subcarrIdx = ((USHORT)(subcarrIdx_H << 8) | (USHORT)subcarrIdx_L);

	return TxBfProfileDataRead(pAd, profileIdx, fgBFer, subcarrIdx);
}

INT Set_TxBfProfileDataWrite(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	USHORT Input[18];
	CHAR *value, value_T[12], onebyte;
	UCHAR strLen;
	INT i;
	struct _RTMP_CHIP_OP *ops = hc_get_chip_ops(pAd->hdev_ctrl);

	os_zero_mem(Input, 36);

	/* Profile Select : Subcarrier Select */
	if (strlen(arg) != 60)
		return FALSE;

	for (i = 0, value = rstrtok(arg, ":"); value; value = rstrtok(NULL, ":"), i++) {
		if ((!isxdigit(*value)) || (!isxdigit(*(value + 1))))
			return FALSE;  /*Invalid*/

		strLen = strlen(value);

		if (strLen & 1) {
			strcpy(value_T, "0");
			strncat(value_T, value, strLen);
			AtoH(value_T, (PCHAR)(&Input[i]), 2);
			Input[i] = be2cpu16(Input[i]);
		} else if (strLen == 2) {
			AtoH(value, (PCHAR)(&onebyte), 1);
			Input[i] = ((USHORT)onebyte) & ((USHORT)0x00FF);
		} else
			MTWF_LOG(DBG_CAT_FW, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			("%s:Error: Un-expected string len!!!!!\n", __func__));
	}

	if (ops->write_txbf_profile_data)
		return ops->write_txbf_profile_data(pAd, Input);
	else
		return FALSE;
}

INT set_txbf_angle_write(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	struct _RTMP_CHIP_OP *ops = hc_get_chip_ops(pAd->hdev_ctrl);
	UINT32 bfer, nc;
	UINT32 angle[14]; /* max angel pair - phi and psi */
	CHAR *tok;
	INT i;

	os_zero_mem(angle, sizeof(angle));

	tok = rstrtok(arg, ":");
	if (tok) {
		bfer = os_str_toul(tok, 0, 16);
	} else {
		MTWF_LOG(DBG_CAT_FW, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("Error: Un-expected format!\n"));
		return FALSE;
	}

	tok = rstrtok(NULL, ":");
	if (tok) {
		nc = os_str_toul(tok, 0, 16);
	} else {
		MTWF_LOG(DBG_CAT_FW, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("Error: Un-expected format!\n"));
		return FALSE;
	}

	tok = rstrtok(NULL, ":");
	for (i = 0; tok; i++) {
		angle[i] = os_str_toul(tok, 0, 16);
		tok = rstrtok(NULL, ":");
	}

	if (ops->set_txbf_angle)
		return ops->set_txbf_angle(pAd->hdev_ctrl, bfer, nc, angle);
	else
		return FALSE;
}

INT set_txbf_dsnr_write(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	struct _RTMP_CHIP_OP *ops = hc_get_chip_ops(pAd->hdev_ctrl);
	UINT32 bfer;
	UINT32 dsnr[8]; /* max dsnr quantity */
	CHAR *tok;
	INT i;

	os_zero_mem(dsnr, sizeof(dsnr));

	tok = rstrtok(arg, ":");
	if (tok) {
		bfer = os_str_toul(tok, 0, 16);
	} else {
		MTWF_LOG(DBG_CAT_FW, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("Error: Un-expected format!\n"));
		return FALSE;
	}

	tok = rstrtok(NULL, ":");
	for (i = 0; tok; i++) {
		dsnr[i] = os_str_toul(tok, 0, 16);
		tok = rstrtok(NULL, ":");
	}

	if (ops->set_txbf_dsnr)
		return ops->set_txbf_dsnr(pAd->hdev_ctrl, bfer, dsnr);
	else
		return FALSE;
}

INT set_txbf_pfmu_data_write(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	struct _RTMP_CHIP_OP *ops = hc_get_chip_ops(pAd->hdev_ctrl);
	UINT32 input[3];
	UINT16 subc;
	UINT8 pfmu_id;
	BOOLEAN bfer;
	CHAR *value;
	INT i;

	for (i = 0, value = rstrtok(arg, ":"); value; value = rstrtok(NULL, ":"), i++)
		input[i] = os_str_toul(value, 0, 16);

	if (i != 3) {
		MTWF_LOG(DBG_CAT_FW, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("Error: Un-expected format!\n"));
		return FALSE;
	}

	pfmu_id = input[0];
	subc = input[1];
	bfer = input[2];

	if (ops->write_txbf_pfmu_data)
		return ops->write_txbf_pfmu_data(pAd->hdev_ctrl, pfmu_id, subc, bfer);
	else
		return FALSE;
}

#ifdef CONFIG_ATE
INT Set_TxBfProfileData20MAllWrite(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	/* Input Cmd Argument Parsing */
	USHORT	*input;     /* Input[] array stores absolute data */
	CHAR	*value, value_t[12], one_byte; /* *value stores every input argument
between : */
	UCHAR	str_len;
	INT i;
	struct  _ATE_CTRL *ATECtrl = &pAd->ATECtrl;
	UCHAR	control_band_idx;
	UINT8   tx_path = pAd->Antenna.field.TxPath;
	CHAR    sub_num;
	INT     sub_num_idx, arg_len;
	UCHAR	profile_idx;
	USHORT	sub_carr_id;
	UINT16  angle_ph11, angle_ph21, angle_ph31, angle_ph41;
	INT16   phi11,     phi21,     phi31;
	BOOLEAN fg_status = FALSE, fg_final_raw_data = FALSE;

	/* Init Array 720 bytes */
	os_alloc_mem(pAd, (UCHAR **)&input, sizeof(USHORT) * 360);
	if (!input)
		goto end;
	os_zero_mem(input, sizeof(USHORT) * 360);

	arg_len = strlen(arg);

	/* Absolute Phi Value Processing */
	if ((strlen(arg) != 183) && (strlen(arg) != 5)) {
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
				("%s: False: Command inputs not meet the Command format Length!\n",
__func__));

		fg_status = FALSE;
		goto end;
	}

	for (i = 0, value = rstrtok(arg, ":"); value; value = rstrtok(NULL, ":"), i++
) {
		if ((!isxdigit(*value)) || (!isxdigit(*(value + 1)))) {
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
					("%s: False: Command input arguments aren't Hex format!\n", __func__));
			fg_status = FALSE;
			goto end;
		}

		str_len = strlen(value);

		if (str_len & 1) {
			strcpy(value_t, "0");
			strncat(value_t, value, str_len);
			AtoH(value_t, (PCHAR)(&input[i]), 2);
			input[i] = be2cpu16(input[i]);
		} else if (str_len == 2) {
			AtoH(value, (PCHAR)(&one_byte), 1);
			input[i] = ((USHORT)one_byte) & ((USHORT)0x00FF);
		} else if (str_len == 4) {
			AtoH(value, (PCHAR)(&input[i]), 2);
			input[i] = be2cpu16(input[i]);
		} else
			MTWF_LOG(DBG_CAT_FW, DBG_SUBCAT_ALL,  DBG_LVL_ERROR,
					("%s: Error: Un-expected Argument Length!\n", __func__));
	}

	/* Check if the input is the last raw data or not */
	fg_final_raw_data = FALSE;
	if (arg_len == 5) {
		if (input[1] == 0x00F0) {
			pAd->profile_data_cnt = 0;
			fg_status = TRUE;
			MTWF_LOG(DBG_CAT_FW, DBG_SUBCAT_ALL,  DBG_LVL_TRACE,
					("%s: Status: Start to Input Profile Data!\n", __func__));
			goto end;
		}

		if (input[1] == 0x00FF) {
			fg_final_raw_data = TRUE;
			profile_idx = input[0];
			MTWF_LOG(DBG_CAT_FW, DBG_SUBCAT_ALL,  DBG_LVL_TRACE,
					("%s: Status: End to Input Profile Data!\n", __func__));
		}
	}

	/* Relative Phi Value Processing */
	control_band_idx = ATECtrl->control_band_idx;
#ifdef DBDC_MODE
	if (pAd->CommonCfg.dbdc_mode) {
		if (control_band_idx == DBDC_BAND0)
			tx_path = pAd->dbdc_band0_tx_path;
		else
			tx_path = pAd->dbdc_band1_tx_path;
	}

	MTWF_LOG(DBG_CAT_FW, DBG_SUBCAT_ALL,  DBG_LVL_TRACE,
			("%s: Status: Config_DBDC_MODE=%d, control_band_idx=%d, \t"
			"dbdc_band0_tx_path=%d, dbdc_band1_tx_path=%d,  \t"
			"Tx Path = %d!\n", __func__, pAd->CommonCfg.dbdc_mode, control_band_idx,
pAd->dbdc_band0_tx_path, pAd->dbdc_band1_tx_path, tx_path));
#endif

	if (fg_final_raw_data == FALSE) {
		/* Input Array Structure Assignment */
		for (sub_num = 0 ; sub_num < 8 ; sub_num++) {
			sub_num_idx = sub_num * 5;
			sub_carr_id = input[sub_num_idx];

			if (sub_carr_id < 32)
				sub_carr_id += 224;
			else
				sub_carr_id -= 32;

			angle_ph11  = input[sub_num_idx+1];
			angle_ph21  = input[sub_num_idx+2];
			angle_ph31  = input[sub_num_idx+3];
			angle_ph41  = input[sub_num_idx+4];

			switch (tx_path) {
			case NSTS_2:
				phi11    = (INT16)(angle_ph21 - angle_ph11);
				phi21    = 0;
				phi31    = 0;

				MTWF_LOG(DBG_CAT_TEST, DBG_SUBCAT_ALL, DBG_LVL_OFF,
						("%s:: SubCarrier ID=%d, angle_ph21 = %x, angle_ph11 = %x, phi11 = %x\n",
						__func__, sub_carr_id, angle_ph21, angle_ph11, phi11));
				break;

			case NSTS_3:
				phi11    = (INT16)(angle_ph31 - angle_ph11);
				phi21    = (INT16)(angle_ph31 - angle_ph21);
				phi31    = 0;

				MTWF_LOG(DBG_CAT_TEST, DBG_SUBCAT_ALL, DBG_LVL_OFF,
						("%s:: SubCarrier ID=%d, angle_ph31 = %x, angle_ph21 = %x, angle_ph11 = %x, phi11 = %x, phi21 = %x\n",
						__func__, sub_carr_id, angle_ph31, angle_ph21, angle_ph11, phi11, phi21));
				break;

			case NSTS_4:
			default:
#ifdef DBDC_MODE
				if (pAd->CommonCfg.dbdc_mode) {
					phi11    = (INT16)(angle_ph21 - angle_ph11);
					phi21    = 0;
					phi31    = 0;
				} else
#endif
				{
					phi11    = (INT16)(angle_ph41 - angle_ph11);
					phi21    = (INT16)(angle_ph41 - angle_ph21);
					phi31    = (INT16)(angle_ph41 - angle_ph31);
				}

				break;
			}
			pAd->profile_data[pAd->profile_data_cnt + sub_num].u2SubCarrIdx =
sub_carr_id;
			pAd->profile_data[pAd->profile_data_cnt + sub_num].i2Phi11 = phi11;
			pAd->profile_data[pAd->profile_data_cnt + sub_num].i2Phi21 = phi21;
			pAd->profile_data[pAd->profile_data_cnt + sub_num].i2Phi31 = phi31;
		}

		pAd->profile_data_cnt += sub_num;
		fg_status = TRUE;

	} else {
		if (CmdETxBfPfmuProfileDataWrite20MAll(pAd,
												profile_idx,
												(PUCHAR)&pAd->profile_data[0]) == STATUS_TRUE)
		fg_status = TRUE;
		MTWF_LOG(DBG_CAT_FW, DBG_SUBCAT_ALL,  DBG_LVL_ERROR,
				("%s: Status: Cmd Send to FW!\n", __func__));
	}

end:
	if (input)
		os_free_mem(input);
	return fg_status;

}
#endif

INT Set_TxBfProfilePnRead(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UCHAR   profileIdx;

	profileIdx = os_str_tol(arg, 0, 10);
	return TxBfProfilePnRead(pAd, profileIdx);
}

INT Set_TxBfProfilePnWrite(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UCHAR   profileIdx;
	UCHAR   ucBw;
	CHAR    *value, value_T[12], onebyte;
	UCHAR   strLen;
	SHORT   Input[14] = {0};
	INT	status, i;
	PFMU_PN rPfmuPn;
	PFMU_PN_DBW80_80M rPfmuPn160M;
	PUCHAR  pPfmuPn = NULL;

	/* Profile Select : Subcarrier Select */
	if (strlen(arg) != 55)
		return FALSE;

	for (i = 0, value = rstrtok(arg, ":"); value; value = rstrtok(NULL, ":"), i++) {
		if ((!isxdigit(*value)) || (!isxdigit(*(value + 1))))
			return FALSE;  /*Invalid*/

		strLen = strlen(value);

		if (strLen & 1) {
			strcpy(value_T, "0");
			strncat(value_T, value, strLen);
			AtoH(value_T, (PCHAR)(&Input[i]), 2);
			Input[i] = be2cpu16(Input[i]);
		} else if (strLen == 2) {
			AtoH(value, (PCHAR)(&onebyte), 1);
			Input[i] = ((USHORT)onebyte) & ((USHORT)0x00FF);
		} else
			MTWF_LOG(DBG_CAT_FW, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
				("%s:Error: Un-expected string len!!!!!\n", __func__));
	}

	profileIdx    = Input[0];
	ucBw          = Input[1];

	if (ucBw != P_DBW160M) {
		os_zero_mem(&rPfmuPn, sizeof(rPfmuPn));
		rPfmuPn.rField.u2CMM_1STS_Tx0    = Input[2];
		rPfmuPn.rField.u2CMM_1STS_Tx1    = Input[3];
		rPfmuPn.rField.u2CMM_1STS_Tx2    = Input[4] & 0x3FF;
		rPfmuPn.rField.u2CMM_1STS_Tx2Msb = Input[4] >> 11;
		rPfmuPn.rField.u2CMM_1STS_Tx3    = Input[5];
		rPfmuPn.rField.u2CMM_2STS_Tx0    = Input[6];
		rPfmuPn.rField.u2CMM_2STS_Tx1    = Input[7] & 0x1FF;
		rPfmuPn.rField.u2CMM_2STS_Tx1Msb = Input[7] >> 10;
		rPfmuPn.rField.u2CMM_2STS_Tx2    = Input[8];
		rPfmuPn.rField.u2CMM_2STS_Tx3    = Input[9];
		rPfmuPn.rField.u2CMM_3STS_Tx0    = Input[10] & 0x0FF;
		rPfmuPn.rField.u2CMM_3STS_Tx0Msb = Input[10] >> 9;
		rPfmuPn.rField.u2CMM_3STS_Tx1    = Input[11];
		rPfmuPn.rField.u2CMM_3STS_Tx2    = Input[12];
		rPfmuPn.rField.u2CMM_3STS_Tx3    = Input[13] & 0x07F;
		rPfmuPn.rField.u2CMM_3STS_Tx3Msb = Input[13] >> 8;
		pPfmuPn = (PUCHAR) (&rPfmuPn);
		status = TxBfProfilePnWrite(pAd, profileIdx, ucBw, pPfmuPn);
	} else {
		os_zero_mem(&rPfmuPn160M, sizeof(rPfmuPn160M));
		rPfmuPn160M.rField.u2DBW160_1STS_Tx0    = Input[2];
		rPfmuPn160M.rField.u2DBW160_1STS_Tx1    = Input[3];
		rPfmuPn160M.rField.u2DBW160_2STS_Tx0    = Input[4] & 0x3FF;
		rPfmuPn160M.rField.u2DBW160_2STS_Tx0Msb = Input[4] >> 11;
		rPfmuPn160M.rField.u2DBW160_2STS_Tx1    = Input[5];
		pPfmuPn = (PUCHAR) (&rPfmuPn160M);
		status = TxBfProfilePnWrite(pAd, profileIdx, ucBw, pPfmuPn);
	}

	return status;
}

INT Set_TxBfQdRead(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	INT8	subcarrIdx;

	subcarrIdx = (INT8)simple_strtol(arg, 0, 10);
	TxBfQdRead(pAd, subcarrIdx);
	return TRUE;
}

/*
    ==========================================================================
    Description:
	RF test switch mode.

	iwpriv ra0 set TxBfFbRptDbgInfo = ucAction : Input[1]

	ucAction
	0: BF_READ_AND_CLEAR_FBK_STAT_INFO
	1: BF_READ_FBK_STAT_INFO
	2: BF_SET_POLL_PFMU_INTR_STAT_TIMEOUT
	   Also set Input[1] as PollPFMUIntrStatTimeOut
	3: BF_SET_PFMU_DEQ_INTERVAL
	   Also set Input[1] as FbRptDeQInterval

	Return:
    ==========================================================================
*/
INT Set_TxBfFbRptDbgInfo(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	INT8 *value = NULL;
	UINT8 i, Input[3] = {0};
	EXT_CMD_TXBF_FBRPT_DBG_INFO_T ETxBfFbRptData;

	for (i = 0, value = rstrtok(arg, ":"); value; value = rstrtok(NULL, ":"), i++) {
		Input[i] = (UINT8)simple_strtol(value, 0, 10);
	}

	os_zero_mem(&ETxBfFbRptData, sizeof(EXT_CMD_TXBF_FBRPT_DBG_INFO_T));

	ETxBfFbRptData.u1Action = Input[0];

	switch (ETxBfFbRptData.u1Action) {
	case BF_SET_POLL_PFMU_INTR_STAT_TIMEOUT:
		ETxBfFbRptData.u1PollPFMUIntrStatTimeOut = Input[1];
		break;
	case BF_SET_PFMU_DEQ_INTERVAL:
		ETxBfFbRptData.u1FbRptDeQInterval = Input[1];
		break;
	case BF_DYNAMIC_PFMU_UPDATE:
		ETxBfFbRptData.u2WlanIdx = Input[1];
		ETxBfFbRptData.u1PFMUUpdateEn = Input[2];
		break;
	default:
		break;
	}

	TxBfFbRptDbgInfo(pAd, (PUINT8)&ETxBfFbRptData);

	return TRUE;
}

INT Set_TxBfTxSndInfo(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	INT8 *value = NULL;
	UINT16 input[4] = {0};
	UINT8 i;
	EXT_CMD_TXBF_SND_CMD_T txbfSndCmd;

	for (i = 0, value = rstrtok(arg, ":"); value; value = rstrtok(NULL, ":"), i++)
		input[i] = os_str_toul(value, 0, 16);

	os_zero_mem(&txbfSndCmd, sizeof(EXT_CMD_TXBF_SND_CMD_T));

	txbfSndCmd.ucAction = input[0];

	switch (txbfSndCmd.ucAction) {
	case BF_SND_READ_INFO:
		txbfSndCmd.ucReadClr = input[1];
		break;
	case BF_SND_CFG_OPT:
		txbfSndCmd.ucVhtOpt = input[1];
		txbfSndCmd.ucHeOpt = input[2];
		txbfSndCmd.ucGloOpt = input[3];
		break;
	case BF_SND_CFG_INTV:
		txbfSndCmd.u2WlanIdx = input[1];
		txbfSndCmd.ucSndIntv = input[2];
		break;
	case BF_SND_STA_STOP:
		txbfSndCmd.u2WlanIdx = input[1];
		txbfSndCmd.ucSndStop = input[2];
		break;
	case BF_SND_CFG_MAX_STA:
		txbfSndCmd.ucMaxSndStas = input[1];
		break;
	case BF_SND_CFG_BFRP:
		txbfSndCmd.ucTxTime = input[1];
		txbfSndCmd.ucMcs = input[2];
		txbfSndCmd.fgLDPC = input[3];
		break;
	case BF_SND_CFG_INF:
		txbfSndCmd.ucInf = input[1];
		break;
	default:
		break;
	}

	TxBfTxSndInfo(pAd, (PUINT8)&txbfSndCmd);

	return TRUE;
}

INT Set_TxBfPlyInfo(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	INT8 *value = NULL;
	UINT16 input[4] = {0};
	UINT8 i;
	EXT_CMD_TXBF_PLY_CMD_T txbfPlyCmd;

	for (i = 0, value = rstrtok(arg, ":"); value; value = rstrtok(NULL, ":"), i++)
		input[i] = os_str_toul(value, 0, 16);

	os_zero_mem(&txbfPlyCmd, sizeof(EXT_CMD_TXBF_PLY_CMD_T));

	txbfPlyCmd.ucAction = input[0];

	switch (txbfPlyCmd.ucAction) {
	case BF_PLY_READ_INFO:
		break;
	case BF_PLY_CFG_OPT:
		txbfPlyCmd.ucGloOpt = input[1];
		txbfPlyCmd.ucGrpIBfOpt = input[2];
		txbfPlyCmd.ucGrpEBfOpt = input[3];
		break;
	case BF_PLY_CFG_STA_PLY:
		txbfPlyCmd.u2WlanIdx = input[1];
		txbfPlyCmd.ucNss = input[2];
		txbfPlyCmd.ucSSPly = input[3];
		break;
	default:
		break;
	}

	TxBfPlyInfo(pAd, (PUINT8)&txbfPlyCmd);

	return TRUE;
}

INT Set_TxBfTxCmd(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	INT8 *value = NULL;
	UINT16 input[4] = {0};
	UINT8 i;
	EXT_CMD_TXBF_TXCMD_CMD_T txbfTxCmdCmd;

	for (i = 0, value = rstrtok(arg, ":"); value; value = rstrtok(NULL, ":"), i++)
		input[i] = os_str_toul(value, 0, 16);

	os_zero_mem(&txbfTxCmdCmd, sizeof(EXT_CMD_TXBF_TXCMD_CMD_T));

	txbfTxCmdCmd.ucAction = input[0];

	switch (txbfTxCmdCmd.ucAction) {
	case BF_TXCMD_READ_INFO:
		break;
	case BF_TXCMD_BF_CFG:
		txbfTxCmdCmd.fgTxCmdBfManual = input[1];
		txbfTxCmdCmd.ucTxCmdBfBit = input[2];
		break;
	default:
		break;
	}

	TxBfTxCmd(pAd, (PUINT8)&txbfTxCmdCmd);

	return TRUE;
}


INT Set_TxBfSndCnt(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	INT8 *value = NULL;
	UINT16 input[4] = {0};
	UINT8 i, u1ArgCnt = 0;
	EXT_CMD_TXBF_SND_CNT_CMD_T rSndCntCmd;

	for (i = 0, value = rstrtok(arg, "-"); value; value = rstrtok(NULL, "-"), i++) {
		input[i] = os_str_toul(value, 0, 10);
		u1ArgCnt++;
	}

	if (u1ArgCnt == 0)
		goto error;

	os_zero_mem(&rSndCntCmd, sizeof(EXT_CMD_TXBF_SND_CNT_CMD_T));

	rSndCntCmd.u1Action = input[0];

	switch (rSndCntCmd.u1Action) {
	case BF_SND_CNT_READ:
		break;

	case BF_SND_CNT_SET_LMT_MAN:
		if (u1ArgCnt < 2)
			goto error;
		rSndCntCmd.u2SndCntLmtMan= input[1];
		break;

	default:
		break;
	}

	TxBfSndCnt(pAd, (PUINT8)&rSndCntCmd);

	return TRUE;

error:
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("Read Info:\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("  iwpriv ra0 set TxBfSndCnt=0\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("Set Snd Cnt Limit Manully:\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("  iwpriv ra0 set TxBfSndCnt=1-[1]\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("                              [1]: Cnt Limit (0: reset default, 1-65535: Cnt Limit)\n"));

	return TRUE;
}

INT Set_HeRaMuMetricInfo(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	INT8 *value = NULL;
	UINT16 input[19] = {0};
	UINT8 i;
	HERA_MU_METRIC_CMD_T MuMetricCmd;

	for (i = 0, value = rstrtok(arg, ":"); value; value = rstrtok(NULL, ":"), i++)
		input[i] = os_str_toul(value, 0, 16);

	os_zero_mem(&MuMetricCmd, sizeof(HERA_MU_METRIC_CMD_T));

	MuMetricCmd.u1Action = input[0];

	switch (MuMetricCmd.u1Action) {
	case HERA_METRIC_READ_INFO:
		MuMetricCmd.u1ReadClr = input[1];
		break;
	case HERA_METRIC_START_CALC:
		MuMetricCmd.u1Band = input[1];
		MuMetricCmd.u1NUser = input[2];
		MuMetricCmd.u1DBW = input[3];
		MuMetricCmd.u1NTxer = input[4];
		MuMetricCmd.u1PFD = input[5];
		MuMetricCmd.u1RuSize = input[6];
		MuMetricCmd.u1RuIdx = input[7];
		MuMetricCmd.u1SpeIdx = input[8];
		MuMetricCmd.u1SpeedUp = input[9];
		MuMetricCmd.u1LDPC = input[10];
		MuMetricCmd.u1NStsUser[0] = input[11];
		MuMetricCmd.u1NStsUser[1] = input[12];
		MuMetricCmd.u1NStsUser[2] = input[13];
		MuMetricCmd.u1NStsUser[3] = input[14];
		MuMetricCmd.u2PfidUser[0] = input[15];
		MuMetricCmd.u2PfidUser[1] = input[16];
		MuMetricCmd.u2PfidUser[2] = input[17];
		MuMetricCmd.u2PfidUser[3] = input[18];
		break;
	case HERA_METRIC_CHANGE_POLLING_TIME:
		MuMetricCmd.u1PollingTime = input[1];
	default:
		break;
	}

	HeRaMuMetricInfo(pAd, (PUINT8)&MuMetricCmd);

	return TRUE;
}

INT Set_TxBfProfileSwTagWrite(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	INT    status = FALSE, rv;
	UINT32 lm, nc, nr, bw, codebook, group;

	if (arg) {
		rv = sscanf(arg, "%d-%d-%d-%d-%d-%d", &lm, &nr, &nc, &bw, &codebook, &group);
		if ((lm > 0) && (rv > 1) && (group < 3) && (nr < 4) && (nc < 4) && (codebook < 4)) {
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
				("%s: Lm=%d Nr=%d Nc=%d BW=%d CodeBook=%d Group=%d\n", __func__, lm, nr, nc, bw, codebook, group));

			status = TxBfPseudoTagUpdate(pAd, lm, nr, nc, bw, codebook, group);

			if (!status) {
				MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s: set command failed.\n", __func__));
				return FALSE;
			}
		} else {
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
				("%s: cmd params are invalid!!\n", __func__));

			return FALSE;
		}
	}

	return TRUE;
}

INT Set_TxBfAidUpdate(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	INT32 Ret = TRUE;
	UINT16 u2Aid = 0;

	if (arg != NULL)
		u2Aid = (UINT16)os_str_toul(arg, 0, 10);
	else {
		MTWF_LOG(DBG_CAT_BF, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			("%s: Argument is NULL\n", __func__));
		Ret = FALSE;
		goto error;
	}

	MTWF_LOG(DBG_CAT_BF, DBG_SUBCAT_ALL, DBG_LVL_OFF,
		("%s: Aid:%u\n", __func__, u2Aid));
	if (CmdETxBfAidSetting(pAd, u2Aid))
		Ret = FALSE;
error:
	MTWF_LOG(DBG_CAT_BF, DBG_SUBCAT_ALL, DBG_LVL_OFF,
		("%s: CMD %s\n", __func__, Ret ? "Success":"Fail"));
	return Ret;
}

VOID ate_set_manual_assoc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	struct _RTMP_CHIP_OP *ops = hc_get_chip_ops(pAd->hdev_ctrl);

	if (ops->set_manual_assoc)
		ops->set_manual_assoc(pAd, arg);
}

VOID ate_set_cmm_starec(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	struct _RTMP_CHIP_OP *ops = hc_get_chip_ops(pAd->hdev_ctrl);

	if (ops->set_cmm_starec)
		ops->set_cmm_starec(pAd, arg);
}

#endif  /* TXBF_SUPPORT */
#endif  /* MT_MAC */

INT set_mec_ctrl(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	INT8 *value = NULL;
	UINT16 input[5] = {0};
	UINT8 i, argCnt = 0;
	CMD_MEC_CTRL_CMD_T mecCtrlCmd;

	for (i = 0, value = rstrtok(arg, "-"); value; value = rstrtok(NULL, "-"), i++) {
		input[i] = os_str_toul(value, 0, 10);
		argCnt++;
	}

	if (argCnt == 0)
		goto error;

	if (input[0] >= MEC_CTRL_ACTION_MAX)
		goto error;

	os_zero_mem(&mecCtrlCmd, sizeof(CMD_MEC_CTRL_CMD_T));

	mecCtrlCmd.u2Action = input[0];
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s: u2Action=%u\n",
			__func__, mecCtrlCmd.u2Action));

	switch (mecCtrlCmd.u2Action) {
	case MEC_CTRL_ACTION_READ_INFO:
		mecCtrlCmd.mecCmdPara.mec_read_info_t.u2ReadType = input[1];
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s: u2ReadType=%u\n",
				__func__, mecCtrlCmd.mecCmdPara.mec_read_info_t.u2ReadType));

		break;

	case MEC_CTRL_ACTION_AMSDU_ALGO_EN_STA:
		if (argCnt != 3)
			goto error;
		mecCtrlCmd.mecCmdPara.mec_algo_en_sta_t.u2WlanIdx = input[1];
		mecCtrlCmd.mecCmdPara.mec_algo_en_sta_t.u1AmsduAlgoEn = input[2];
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s: u2WlanIdx=%u, u1AmsduAlgoEn=%u\n",
				__func__,
				mecCtrlCmd.mecCmdPara.mec_algo_en_sta_t.u2WlanIdx,
				mecCtrlCmd.mecCmdPara.mec_algo_en_sta_t.u1AmsduAlgoEn));
		break;

	case MEC_CTRL_ACTION_AMSDU_PARA_STA:
		if (argCnt != 5)
			goto error;

		mecCtrlCmd.mecCmdPara.mec_amsdu_para_sta_t.u2WlanIdx = input[1];
		mecCtrlCmd.mecCmdPara.mec_amsdu_para_sta_t.u1AmsduEn = input[2];
		mecCtrlCmd.mecCmdPara.mec_amsdu_para_sta_t.u1AmsduNum = input[3];
		mecCtrlCmd.mecCmdPara.mec_amsdu_para_sta_t.u2AmsduLen = input[4];
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
			    ("%s: u2WlanIdx=%u, u1AmsduEn=%u, u1AmsduNum=%u, u2AmsduLen=%u\n",
				__func__,
				mecCtrlCmd.mecCmdPara.mec_amsdu_para_sta_t.u2WlanIdx,
				mecCtrlCmd.mecCmdPara.mec_amsdu_para_sta_t.u1AmsduEn,
				mecCtrlCmd.mecCmdPara.mec_amsdu_para_sta_t.u1AmsduNum,
				mecCtrlCmd.mecCmdPara.mec_amsdu_para_sta_t.u2AmsduLen));
		break;

	case MEC_CTRL_ACTION_AMSDU_ALGO_THRESHOLD:
		if (argCnt != 4)
			goto error;

		mecCtrlCmd.mecCmdPara.mec_amsdu_algo_thr.u1BaNum = input[1];
		mecCtrlCmd.mecCmdPara.mec_amsdu_algo_thr.u1AmsduNum = input[2];
		mecCtrlCmd.mecCmdPara.mec_amsdu_algo_thr.u2AmsduRateThr = input[3];
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s: u1BaNum=%u, u1AmsduNum=%u, u2AmsduRateThr=%u\n",
				__func__,
				mecCtrlCmd.mecCmdPara.mec_amsdu_algo_thr.u1BaNum,
				mecCtrlCmd.mecCmdPara.mec_amsdu_algo_thr.u1AmsduNum,
				mecCtrlCmd.mecCmdPara.mec_amsdu_algo_thr.u2AmsduRateThr));
		break;

	default:
		goto error;
		break;
	}

	CmdMecCtrl(pAd, (PUINT8)&mecCtrlCmd);
	return TRUE;

error:
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("Wrong Cmd Format. Plz input:\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("iwpriv ra0 set mec_ctrl=[0]-[1]-[2]-[3]-[4]\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("  [0]=0: Read Info: =0-[1]\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("                 , [1]: Read Type (Optional, 0 for all, BIT(1) for Algo En, BIT(2) for Algo Thr)\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("  [0]=1: Set AMSDU Algo Enable: =1-[1]-[2] \n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("                 , [1]: Wlan Idx (65535 for all)\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("                 , [2]: Enable (0, 1 for Disable, Enable)\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("  [0]=2: Set WTBL AMSDU Parameters: =2-[1]-[2]-[3]-[4]\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("                 , [1]: Wlan Idx (65535 for all)\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("                 , [2]: Enable (0, 1 for Disable, Enable)\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("                 , [3]: Number (0-7 for AMSDU Num 1-8)\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("                 , [4]: Length (1-31 for 255-7935 Bytes, unit 256 Bytes)\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("  [0]=3: Set PHY Rate Threshold of AMSDU Length: =3-[1]-[2]-[3]\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("                 , [1]: BA Numer (0, 1 for BA 64, 256)\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("                 , [2]: Amsdu Number (1-3 for AMSDU Num 2-4)\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("                 , [3]: Threshold of PHY Rate (1-65535, unit: Mbps, 0 for reset default)\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("                 e.g. =3-1-300 means if current PHY rate > 300Mbps, then set AMSDU Len of 2 num\n"));
	return FALSE;
}

#if defined(CONFIG_WIFI_PKT_FWD) || defined(CONFIG_WIFI_PKT_FWD_MODULE)
INT Set_WifiFwd_Proc(
	IN PRTMP_ADAPTER pAd,
	IN RTMP_STRING *arg)
{
	int active = os_str_tol(arg, 0, 10);

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s::active=%d\n", __func__, active));

	if (active == 0) {
		if (wf_drv_tbl.wf_fwd_pro_halt_hook)
			wf_drv_tbl.wf_fwd_pro_halt_hook();
	} else  {
		if (wf_drv_tbl.wf_fwd_pro_active_hook)
			wf_drv_tbl.wf_fwd_pro_active_hook();
	}

	return TRUE;
}

INT WifiFwdSet(
	IN int disabled)
{
	if (disabled != 0) {
		if (wf_drv_tbl.wf_fwd_pro_disabled_hook)
			wf_drv_tbl.wf_fwd_pro_disabled_hook();
	}

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s::disabled=%d\n", __func__, disabled));
	return TRUE;
}

INT Set_WifiFwd_Down(
	IN PRTMP_ADAPTER pAd,
	IN RTMP_STRING *arg)
{
	int disable = os_str_tol(arg, 0, 10);

	WifiFwdSet(disable);
	return TRUE;
}

INT Set_WifiFwdAccessSchedule_Proc(
	IN PRTMP_ADAPTER pAd,
	IN RTMP_STRING *arg)
{
	int active = os_str_tol(arg, 0, 10);

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s::active=%d\n", __func__, active));

	if (active == 0) {
		if (wf_drv_tbl.wf_fwd_access_schedule_halt_hook)
			wf_drv_tbl.wf_fwd_access_schedule_halt_hook();
	} else  {
		if (wf_drv_tbl.wf_fwd_access_schedule_active_hook)
			wf_drv_tbl.wf_fwd_access_schedule_active_hook();
	}

	return TRUE;
}

INT Set_WifiFwdHijack_Proc(
	IN PRTMP_ADAPTER pAd,
	IN RTMP_STRING *arg)
{
	int active = os_str_tol(arg, 0, 10);

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s::active=%d\n", __func__, active));

	if (active == 0) {
		if (wf_drv_tbl.wf_fwd_hijack_halt_hook)
			wf_drv_tbl.wf_fwd_hijack_halt_hook();
	} else  {
		if (wf_drv_tbl.wf_fwd_hijack_active_hook)
			wf_drv_tbl.wf_fwd_hijack_active_hook();
	}

	return TRUE;
}

INT Set_WifiFwdBpdu_Proc(
	IN PRTMP_ADAPTER pAd,
	IN RTMP_STRING *arg)
{
	int active = os_str_tol(arg, 0, 10);

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s::active=%d\n", __func__, active));

	if (active == 0) {
		if (wf_drv_tbl.wf_fwd_bpdu_halt_hook)
			wf_drv_tbl.wf_fwd_bpdu_halt_hook();
	} else {
		if (wf_drv_tbl.wf_fwd_bpdu_active_hook)
			wf_drv_tbl.wf_fwd_bpdu_active_hook();
	}

	return TRUE;
}

INT Set_WifiFwdRepDevice(
	IN PRTMP_ADAPTER pAd,
	IN RTMP_STRING *arg)
{
	int rep = os_str_tol(arg, 0, 10);

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s::rep=%d\n", __func__, rep));

	if (wf_drv_tbl.wf_fwd_get_rep_hook)
		wf_drv_tbl.wf_fwd_get_rep_hook(rep);

	return TRUE;
}

INT Set_WifiFwdShowEntry(
	IN PRTMP_ADAPTER pAd,
	IN RTMP_STRING *arg)
{
	if (wf_drv_tbl.wf_fwd_show_entry_hook)
		wf_drv_tbl.wf_fwd_show_entry_hook();

	return TRUE;
}

INT Set_WifiFwdDeleteEntry(
	IN PRTMP_ADAPTER pAd,
	IN RTMP_STRING *arg)
{
	int idx = os_str_tol(arg, 0, 10);

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s::idx=%d\n", __func__, idx));

	if (wf_drv_tbl.wf_fwd_delete_entry_hook)
		wf_drv_tbl.wf_fwd_delete_entry_hook(idx);

	return TRUE;
}

INT Set_PacketSourceShowEntry(
	IN PRTMP_ADAPTER pAd,
	IN RTMP_STRING *arg)
{
	if (wf_drv_tbl.packet_source_show_entry_hook)
		wf_drv_tbl.packet_source_show_entry_hook();

	return TRUE;
}

INT Set_PacketSourceDeleteEntry(
	IN PRTMP_ADAPTER pAd,
	IN RTMP_STRING *arg)
{
	int idx = os_str_tol(arg, 0, 10);

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s::idx=%d\n", __func__, idx));

	if (wf_drv_tbl.packet_source_delete_entry_hook)
		wf_drv_tbl.packet_source_delete_entry_hook(idx);

	return TRUE;
}

#define BRIDGE_INTF_NAME_MAX_SIZE 10

INT Set_WifiFwdBridge_Proc(
	IN PRTMP_ADAPTER pAd,
	IN RTMP_STRING * arg)
{

	UINT8 Length = 0;

	Length = strlen(arg);

	if (Length > BRIDGE_INTF_NAME_MAX_SIZE) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			("%s:Bridge Intf Name too large =%s,size:%d\n", __func__, arg, Length));
		return FALSE;

	}

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
		("%s:Bridge Intf Name =%s,size:%d\n", __func__, arg, Length));

	if (wf_drv_tbl.wf_fwd_set_bridge_hook)
		wf_drv_tbl.wf_fwd_set_bridge_hook(arg, Length);

	return TRUE;
}

#endif /* CONFIG_WIFI_PKT_FWD */

#ifdef DOT11_N_SUPPORT
void assoc_ht_info_debugshow(
	IN PRTMP_ADAPTER pAd,
	IN MAC_TABLE_ENTRY *pEntry,
	IN HT_CAPABILITY_IE * pHTCapability)
{
	HT_CAP_INFO			*pHTCap;
	HT_CAP_PARM		*pHTCapParm;
	EXT_HT_CAP_INFO		*pExtHT;
#ifdef TXBF_SUPPORT
	HT_BF_CAP			*pBFCap;
#endif /* TXBF_SUPPORT */

	if (pHTCapability) {
		pHTCap = &pHTCapability->HtCapInfo;
		pHTCapParm = &pHTCapability->HtCapParm;
		pExtHT = &pHTCapability->ExtHtCapInfo;
#ifdef TXBF_SUPPORT
		pBFCap = &pHTCapability->TxBFCap;
#endif /* TXBF_SUPPORT */
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Peer - 11n HT Info\n"));
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("\tHT Cap Info:\n"));
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
				 ("\t\t HT_RX_LDPC(%d), BW(%d), MIMOPS(%d), GF(%d), ShortGI_20(%d), ShortGI_40(%d)\n",
				  pHTCap->ht_rx_ldpc, pHTCap->ChannelWidth, pHTCap->MimoPs, pHTCap->GF,
				  pHTCap->ShortGIfor20, pHTCap->ShortGIfor40));
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
				 ("\t\t TxSTBC(%d), RxSTBC(%d), DelayedBA(%d), A-MSDU(%d), CCK_40(%d)\n",
				  pHTCap->TxSTBC, pHTCap->RxSTBC, pHTCap->DelayedBA, pHTCap->AMsduSize, pHTCap->CCKmodein40));
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("\t\t PSMP(%d), Forty_Mhz_Intolerant(%d), L-SIG(%d)\n",
				 pHTCap->PSMP, pHTCap->Forty_Mhz_Intolerant, pHTCap->LSIGTxopProSup));
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("\tHT Parm Info:\n"));
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("\t\t MaxRx A-MPDU Factor(%d), MPDU Density(%d)\n",
				 pHTCapParm->MaxRAmpduFactor, pHTCapParm->MpduDensity));
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("\tHT MCS set:\n"));
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
				 ("\t\t RxMCS(%02x %02x %02x %02x %02x) MaxRxMbps(%d) TxMCSSetDef(%02x)\n",
				  pHTCapability->MCSSet[0], pHTCapability->MCSSet[1], pHTCapability->MCSSet[2],
				  pHTCapability->MCSSet[3], pHTCapability->MCSSet[4],
				  (pHTCapability->MCSSet[11] << 8) + pHTCapability->MCSSet[10],
				  pHTCapability->MCSSet[12]));
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("\tExt HT Cap Info:\n"));
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
				 ("\t\t PCO(%d), TransTime(%d), MCSFeedback(%d), +HTC(%d), RDG(%d)\n",
				  pExtHT->Pco, pExtHT->TranTime, pExtHT->MCSFeedback, pExtHT->PlusHTC, pExtHT->RDGSupport));
#ifdef TXBF_SUPPORT
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("\tTX BF Cap:\n"));
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
				 ("\t\t ImpRxCap(%d), RXStagSnd(%d), TXStagSnd(%d), RxNDP(%d), TxNDP(%d) ImpTxCap(%d)\n",
				  pBFCap->TxBFRecCapable, pBFCap->RxSoundCapable, pBFCap->TxSoundCapable,
				  pBFCap->RxNDPCapable, pBFCap->TxNDPCapable, pBFCap->ImpTxBFCapable));
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
				 ("\t\t Calibration(%d), ExpCSICapable(%d), ExpComSteerCapable(%d), ExpCSIFbk(%d), ExpNoComBF(%d) ExpComBF(%d)\n",
				  pBFCap->Calibration, pBFCap->ExpCSICapable, pBFCap->ExpComSteerCapable,
				  pBFCap->ExpCSIFbk, pBFCap->ExpNoComBF, pBFCap->ExpComBF));
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
				 ("\t\t MinGrouping(%d), CSIBFAntSup(%d), NoComSteerBFAntSup(%d), ComSteerBFAntSup(%d), CSIRowBFSup(%d) ChanEstimation(%d)\n",
				  pBFCap->MinGrouping, pBFCap->CSIBFAntSup, pBFCap->NoComSteerBFAntSup,
				  pBFCap->ComSteerBFAntSup, pBFCap->CSIRowBFSup, pBFCap->ChanEstimation));
#endif /* TXBF_SUPPORT */
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
				 ("\nPeer - MODE=%d, BW=%d, MCS=%d, ShortGI=%d, MaxRxFactor=%d, MpduDensity=%d, MIMOPS=%d, AMSDU=%d\n",
				  pEntry->HTPhyMode.field.MODE, pEntry->HTPhyMode.field.BW,
				  pEntry->HTPhyMode.field.MCS, pEntry->HTPhyMode.field.ShortGI,
				  pEntry->MaxRAmpduFactor, pEntry->MpduDensity,
				  pEntry->MmpsMode, pEntry->AMsduSize));
#ifdef DOT11N_DRAFT3
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("\tExt Cap Info:\n"));
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("\t\tBss2040CoexistMgmt=%d\n",
				 pEntry->BSS2040CoexistenceMgmtSupport));
#endif /* DOT11N_DRAFT3 */
	}
}

INT	Set_BurstMode_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	ULONG Value;

	Value = os_str_tol(arg, 0, 10);
	pAd->CommonCfg.bRalinkBurstMode = ((Value == 1) ? TRUE : FALSE);
	AsicSetRalinkBurstMode(pAd, pAd->CommonCfg.bRalinkBurstMode);
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Set_BurstMode_Proc ::%s\n",
			 (pAd->CommonCfg.bRalinkBurstMode == TRUE) ? "enabled" : "disabled"));
	return TRUE;
}
#endif /* DOT11_N_SUPPORT */

#ifdef DOT11_VHT_AC
VOID assoc_vht_info_debugshow(
	IN RTMP_ADAPTER *pAd,
	IN MAC_TABLE_ENTRY *pEntry,
	IN VHT_CAP_IE * vht_cap,
	IN VHT_OP_IE * vht_op)
{
	VHT_CAP_INFO *cap_info;
	VHT_MCS_SET *mcs_set;
	struct vht_opinfo *op_info;
	VHT_MCS_MAP *mcs_map;
	struct wifi_dev *wdev = pEntry->wdev;
	USHORT PhyMode = wdev->PhyMode;

	if (!WMODE_CAP_AC(PhyMode))
		return;

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Peer - 11AC VHT Info\n"));

	if (vht_cap) {
		cap_info = &vht_cap->vht_cap;
		mcs_set = &vht_cap->mcs_set;
		hex_dump("peer vht_cap raw data", (UCHAR *)cap_info, sizeof(VHT_CAP_INFO));
		hex_dump("peer vht_mcs raw data", (UCHAR *)mcs_set, sizeof(VHT_MCS_SET));
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("\tVHT Cap Info:\n"));
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
				 ("\t\tMaxMpduLen(%d), BW(%d), SGI_80M(%d), RxLDPC(%d), TxSTBC(%d), RxSTBC(%d), +HTC-VHT(%d)\n",
				  cap_info->max_mpdu_len, cap_info->ch_width, cap_info->sgi_80M, cap_info->rx_ldpc, cap_info->tx_stbc,
				  cap_info->rx_stbc, cap_info->htc_vht_cap));
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
				 ("\t\tMaxAmpduExp(%d), VhtLinkAdapt(%d), RxAntConsist(%d), TxAntConsist(%d)\n",
				  cap_info->max_ampdu_exp, cap_info->vht_link_adapt, cap_info->rx_ant_consistency, cap_info->tx_ant_consistency));
		mcs_map = &mcs_set->rx_mcs_map;
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("\t\tRxMcsSet: HighRate(%d), RxMCSMap(%d,%d,%d,%d,%d,%d,%d)\n",
				 mcs_set->rx_high_rate, mcs_map->mcs_ss1, mcs_map->mcs_ss2, mcs_map->mcs_ss3,
				 mcs_map->mcs_ss4, mcs_map->mcs_ss5, mcs_map->mcs_ss6, mcs_map->mcs_ss7));
		mcs_map = &mcs_set->tx_mcs_map;
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("\t\tTxMcsSet: HighRate(%d), TxMcsMap(%d,%d,%d,%d,%d,%d,%d)\n",
				 mcs_set->tx_high_rate, mcs_map->mcs_ss1, mcs_map->mcs_ss2, mcs_map->mcs_ss3,
				 mcs_map->mcs_ss4, mcs_map->mcs_ss5, mcs_map->mcs_ss6, mcs_map->mcs_ss7));
#ifdef VHT_TXBF_SUPPORT
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("\t\tETxBfCap: Bfer(%d), Bfee(%d), SndDim(%d)\n",
				 cap_info->bfer_cap_su, cap_info->bfee_cap_su, cap_info->num_snd_dimension));
#endif
	}

	if (vht_op) {
		op_info = &vht_op->vht_op_info;
		mcs_map = &vht_op->basic_mcs_set;
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("\tVHT OP Info:\n"));
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("\t\tChannel Width(%d), CenteralFreq1(%d), CenteralFreq2(%d)\n",
				 op_info->ch_width, op_info->ccfs_0, op_info->ccfs_1));
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
				 ("\t\tBasicMCSSet(SS1:%d, SS2:%d, SS3:%d, SS4:%d, SS5:%d, SS6:%d, SS7:%d)\n",
				  mcs_map->mcs_ss1, mcs_map->mcs_ss2, mcs_map->mcs_ss3,
				  mcs_map->mcs_ss4, mcs_map->mcs_ss5, mcs_map->mcs_ss6,
				  mcs_map->mcs_ss7));
	}

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("\n"));
}
#endif /* DOT11_VHT_AC */

INT Set_RateAdaptInterval(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UINT32 ra_time, ra_qtime;
	RTMP_STRING *token;
	char sep = ':';
	ULONG irqFlags;
	/*
		The ra_interval inupt string format should be d:d, in units of ms.
			=>The first decimal number indicates the rate adaptation checking period,
			=>The second decimal number indicates the rate adaptation quick response checking period.
	*/
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s():%s\n", __func__, arg));
	token = strchr(arg, sep);

	if (token != NULL) {
		*token = '\0';

		if (strlen(arg) && strlen(token + 1)) {
			ra_time = os_str_tol(arg, 0, 10);
			ra_qtime = os_str_tol(token + 1, 0, 10);
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s():Set RateAdaptation TimeInterval as(%d:%d) ms\n",
					 __func__, ra_time, ra_qtime));
			RTMP_IRQ_LOCK(&pAd->irq_lock, irqFlags);
			pAd->ra_interval = ra_time;
			pAd->ra_fast_interval = ra_qtime;
#ifdef CONFIG_AP_SUPPORT

			if (pAd->ApCfg.ApQuickResponeForRateUpTimerRunning == TRUE) {
				BOOLEAN Cancelled;

				RTMPCancelTimer(&pAd->ApCfg.ApQuickResponeForRateUpTimer, &Cancelled);
				pAd->ApCfg.ApQuickResponeForRateUpTimerRunning = FALSE;
			}

#endif /* CONFIG_AP_SUPPORT  */
			RTMP_IRQ_UNLOCK(&pAd->irq_lock, irqFlags);
			return TRUE;
		}
	}

	return FALSE;
}

#ifdef SNIFFER_SUPPORT
INT Set_MonitorMode_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
#ifdef CONFIG_HW_HAL_OFFLOAD
	struct _EXT_CMD_SNIFFER_MODE_T SnifferFWCmd;
#endif /* CONFIG_HW_HAL_OFFLOAD */
	POS_COOKIE pObj;
	struct wifi_dev *wdev;

	pAd->monitor_ctrl.CurrentMonitorMode = os_str_tol(arg, 0, 10);
	pObj = (POS_COOKIE)pAd->OS_Cookie;
	wdev = &pAd->ApCfg.MBSSID[pObj->ioctl_if].wdev;
	SnifferFWCmd.ucDbdcIdx = 0;

	if (pAd->monitor_ctrl.CurrentMonitorMode > MONITOR_MODE_FULL || pAd->monitor_ctrl.CurrentMonitorMode < MONITOR_MODE_OFF)
		pAd->monitor_ctrl.CurrentMonitorMode = MONITOR_MODE_OFF;

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			 ("set Current Monitor Mode = %d , range(%d ~ %d)\n"
			  , pAd->monitor_ctrl.CurrentMonitorMode, MONITOR_MODE_OFF, MONITOR_MODE_FULL));

	switch (pAd->monitor_ctrl.CurrentMonitorMode) {
	case MONITOR_MODE_OFF:			/* reset to normal */
		pAd->monitor_ctrl.bMonitorOn = FALSE;
#ifdef CONFIG_HW_HAL_OFFLOAD
		SnifferFWCmd.ucSnifferEn = 0;
		MtCmdSetSnifferMode(pAd, &SnifferFWCmd);
#else
		AsicSetRxFilter(pAd);
#endif /* CONFIG_HW_HAL_OFFLOAD */
		break;

	case MONITOR_MODE_REGULAR_RX:			/* report probe_request only , normal rx filter */
		pAd->monitor_ctrl.bMonitorOn = TRUE;
#ifdef CONFIG_HW_HAL_OFFLOAD
		SnifferFWCmd.ucSnifferEn = 1;
		MtCmdSetSnifferMode(pAd, &SnifferFWCmd);
#else
		AsicSetRxFilter(pAd);
#endif /* CONFIG_HW_HAL_OFFLOAD */
		break;

	case MONITOR_MODE_FULL:			/* fully report, Enable Rx with promiscuous reception */
		pAd->monitor_ctrl.bMonitorOn = TRUE;
#ifdef CONFIG_HW_HAL_OFFLOAD
		SnifferFWCmd.ucSnifferEn = 1;
		MtCmdSetSnifferMode(pAd, &SnifferFWCmd);
#else
		AsicSetRxFilter(pAd);
#endif /* CONFIG_HW_HAL_OFFLOAD */
		break;
	}

	return TRUE;
}

INT Set_MonitorFilterSize_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	pAd->monitor_ctrl.FilterSize = os_str_tol(arg, 0, 10);
	if (pAd->monitor_ctrl.FilterSize < sizeof(struct mtk_radiotap_header))
		pAd->monitor_ctrl.FilterSize = RX_BUFFER_SIZE_MIN + sizeof(struct mtk_radiotap_header);
	return TRUE;
}

INT Set_MonitorFrameType_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	pAd->monitor_ctrl.FrameType = os_str_tol(arg, 0, 10);
	if (pAd->monitor_ctrl.FrameType > FC_TYPE_DATA)
		pAd->monitor_ctrl.FrameType = FC_TYPE_RSVED;
	return TRUE;
}

INT Set_MonitorMacFilter_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	INT ret = TRUE;
	RTMP_STRING *this_char = NULL;
	RTMP_STRING *value = NULL;
	INT idx = 0;

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("--> %s()\n", __func__));

	while ((this_char = strsep((char **)&arg, ";")) != NULL) {
		if (*this_char == '\0') {
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_WARN, ("An unnecessary delimiter entered!\n"));
			continue;
		}
		/* the acceptable format of MAC address is like 01:02:03:04:05:06 with length 17 */
		if (strlen(this_char) != 17) {
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
				("illegal MAC address length! (acceptable format 01:02:03:04:05:06 length 17)\n"));
			continue;
		}

		for (idx = 0, value = rstrtok(this_char, ":"); value; value = rstrtok(NULL, ":")) {
			if ((strlen(value) != 2) || (!isxdigit(*value)) || (!isxdigit(*(value + 1)))) {
				MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
					("illegal MAC address format or octet!\n"));
				break;
			}

			AtoH(value, &pAd->monitor_ctrl.MacFilterAddr[idx++], 1);
		}

		if (idx != MAC_ADDR_LEN)
			continue;
	}

	for (idx = 0; idx < MAC_ADDR_LEN; idx++)
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%02X ", pAd->monitor_ctrl.MacFilterAddr[idx]));

	pAd->monitor_ctrl.MacFilterOn = TRUE;
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\n"));
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("<-- %s()\n", __func__));
	return ret;
}

INT Set_MonitorMacFilterOff_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	pAd->monitor_ctrl.MacFilterOn = FALSE;
	return TRUE;
}

#endif /* SNIFFER_SUPPORT */

#ifdef SINGLE_SKU
INT Set_ModuleTxpower_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UINT16 Value;

	if (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_INTERRUPT_REGISTER_TO_OS)) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("Do NOT accept this command after interface is up.\n"));
		return FALSE;
	}

	Value = (UINT16)os_str_tol(arg, 0, 10);
	pAd->CommonCfg.ModuleTxpower = Value;
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("IF Set_ModuleTxpower_Proc::(ModuleTxpower=%d)\n",
			 pAd->CommonCfg.ModuleTxpower));
	return TRUE;
}
#endif /* SINGLE_SKU */


INT set_no_bcn(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	ULONG no_bcn;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	struct wifi_dev *wdev = get_wdev_by_ioctl_idx_and_iftype(pAd, pObj->ioctl_if, pObj->ioctl_if_type);

	no_bcn = os_str_tol(arg, 0, 10);
	if (wdev) {
		if (no_bcn)
			UpdateBeaconHandler(pAd, wdev, BCN_UPDATE_DISABLE_TX);
		else
			UpdateBeaconHandler(pAd, wdev, BCN_UPDATE_ENABLE_TX);
	}

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s(): Set no beacon as:%d\n",
			 __func__, (no_bcn ? 1 : 0)));
	return TRUE;
}

#ifdef DOT11_N_SUPPORT

#define MAX_AGG_CNT	8

/* DisplayTxAgg - display Aggregation statistics from MAC */
void DisplayTxAgg(RTMP_ADAPTER *pAd)
{
	ULONG totalCount;
	ULONG aggCnt[MAX_AGG_CNT + 2];
	int i;

	AsicReadAggCnt(pAd, aggCnt, sizeof(aggCnt) / sizeof(ULONG));
	totalCount = aggCnt[0] + aggCnt[1];

	if (totalCount > 0)
		for (i = 0; i < MAX_AGG_CNT; i++)
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t%d MPDU=%ld (%ld%%)\n", i + 1, aggCnt[i + 2],
					 aggCnt[i + 2] * 100 / totalCount));

	printk("====================\n");
}
#endif /* DOT11_N_SUPPORT */

#ifdef REDUCE_TCP_ACK_SUPPORT
INT Set_ReduceAckEnable_Proc(
	IN  PRTMP_ADAPTER   pAdapter,
	IN  RTMP_STRING     *pParam)
{
	if (pParam == NULL)
		return FALSE;

	ReduceAckSetEnable(pAdapter, os_str_tol(pParam, 0, 10));
	return TRUE;
}

INT Show_ReduceAckInfo_Proc(
	IN  PRTMP_ADAPTER   pAdapter,
	IN  RTMP_STRING     *pParam)
{
	ReduceAckShow(pAdapter);
	return TRUE;
}

INT Set_ReduceAckProb_Proc(
	IN  PRTMP_ADAPTER   pAdapter,
	IN  RTMP_STRING     *pParam)
{
	if (pParam == NULL)
		return FALSE;

	ReduceAckSetProbability(pAdapter, os_str_tol(pParam, 0, 10));
	return TRUE;
}
#endif

#ifdef RTMP_RBUS_SUPPORT
#ifdef LED_CONTROL_SUPPORT
INT Set_WlanLed_Proc(
	IN PRTMP_ADAPTER	pAd,
	IN RTMP_STRING *arg)
{
#if defined(RTMP_PCI_SUPPORT) && defined(RTMP_RBUS_SUPPORT)

	if (!IS_RBUS_INF(pAd)) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s : Support RBUS interface only\n", __func__));
		return TRUE;
	}

#endif /* defined(RTMP_PCI_SUPPORT) && defined(RTMP_RBUS_SUPPORT) */
	BOOLEAN bWlanLed;

	bWlanLed = (BOOLEAN) os_str_tol(arg, 0, 10);
	{
		if (bWlanLed)
			RTMPStartLEDMode(pAd);
		else
			RTMPExitLEDMode(pAd);
	};
	return TRUE;
}
#endif /* LED_CONTROL_SUPPORT */
#endif /* RTMP_RBUS_SUPPORT */

#ifdef MT_MAC
static INT32 SetMTRF(RTMP_ADAPTER *pAd, RTMP_STRING *Arg)
{
	UCHAR *param_ptr = NULL;
	UCHAR param_idx = 0, ret = 0;
	UINT32 rf_param[3] = {0}; /* rfidx, offset, value */

	if (Arg) {
		for (param_idx = 0, param_ptr = rstrtok(Arg, "-"); param_ptr; param_ptr = rstrtok(NULL, "-"), param_idx++) {
			ret = sscanf(param_ptr, "%8x", &rf_param[param_idx]);

			if (ret == 0) {
				MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s: invalid format(%s), ignored!\n", __func__, param_ptr));
				goto err_out;
			}
		}
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("RfIdx = %d, Offset = 0x%08x, Value = 0x%08x\n", rf_param[0], rf_param[1], rf_param[2]));
	}

	if (param_idx < 4) {
		if (param_idx == 2) {
			MtCmdRFRegAccessRead(pAd, rf_param[0], rf_param[1], &rf_param[2]);
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s:%d read[0x%08x]=0x%08x\n", __func__, rf_param[0], rf_param[1], rf_param[2]));
		} else if (param_idx == 3) {
			MtCmdRFRegAccessWrite(pAd, rf_param[0], rf_param[1], rf_param[2]);
			rf_param[2] = 0;
			MtCmdRFRegAccessRead(pAd, rf_param[0], rf_param[1], &rf_param[2]);
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s:%d write[0x%08x]=0x%08x\n", __func__, rf_param[0], rf_param[2], rf_param[2]));
		}
	} else
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s(): incorrect format(%s)\n", __func__, Arg));

err_out:
	return 0;
}
#endif

INT32 SetRF(RTMP_ADAPTER *pAd, RTMP_STRING *Arg)
{
	INT32 Ret = 0;
#ifdef MT_MAC
	struct _RTMP_CHIP_CAP *cap = hc_get_chip_cap(pAd->hdev_ctrl);

	if (cap->rf_type == RF_MT)
		Ret = SetMTRF(pAd, Arg);

#endif
	return Ret;
}

static struct {
	RTMP_STRING *name;
	INT (*show_proc)(RTMP_ADAPTER *pAd, RTMP_STRING *arg, ULONG BufLen);
} *PRTMP_PRIVATE_STA_SHOW_CFG_VALUE_PROC, RTMP_PRIVATE_STA_SHOW_CFG_VALUE_PROC[] = {
#ifdef DBG
	{"SSID",					Show_SSID_Proc},
	{"WirelessMode",			Show_WirelessMode_Proc},
	{"TxBurst",					Show_TxBurst_Proc},
	{"TxPreamble",				Show_TxPreamble_Proc},
	{"TxPower",					Show_TxPower_Proc},
	{"Channel",					Show_Channel_Proc},
	{"BGProtection",			Show_BGProtection_Proc},
	{"RTSThreshold",			Show_RTSThreshold_Proc},
	{"FragThreshold",			Show_FragThreshold_Proc},
#ifdef DOT11_N_SUPPORT
	{"HtBw",					Show_HtBw_Proc},
	{"HtMcs",					Show_HtMcs_Proc},
	{"HtGi",					Show_HtGi_Proc},
	{"HtOpMode",				Show_HtOpMode_Proc},
	{"HtExtcha",				Show_HtExtcha_Proc},
	{"HtMpduDensity",			Show_HtMpduDensity_Proc},
	{"HtBaWinSize",		        Show_HtBaWinSize_Proc},
	{"HtRdg",			Show_HtRdg_Proc},
	{"HtAmsdu",			Show_HtAmsdu_Proc},
	{"HtAutoBa",		        Show_HtAutoBa_Proc},
#endif /* DOT11_N_SUPPORT */
	{"CountryRegion",			Show_CountryRegion_Proc},
	{"CountryRegionABand",		Show_CountryRegionABand_Proc},
	{"CountryCode",				Show_CountryCode_Proc},
#ifdef AGGREGATION_SUPPORT
	{"PktAggregate",			Show_PktAggregate_Proc},
#endif

	{"WmmCapable",				Show_WmmCapable_Proc},

	{"IEEE80211H",				Show_IEEE80211H_Proc},
#ifdef CONFIG_STA_SUPPORT
	{"NetworkType",				Show_NetworkType_Proc},
#ifdef WSC_STA_SUPPORT
	{"WpsApBand",				Show_WpsPbcBand_Proc},
	{"Manufacturer",			Show_WpsManufacturer_Proc},
	{"ModelName",				Show_WpsModelName_Proc},
	{"DeviceName",				Show_WpsDeviceName_Proc},
	{"ModelNumber",				Show_WpsModelNumber_Proc},
	{"SerialNumber",			Show_WpsSerialNumber_Proc},
#endif /* WSC_STA_SUPPORT */
	{"WPAPSK",					Show_WPAPSK_Proc},
	{"AutoReconnect",			Show_AutoReconnect_Proc},
	{"secinfo",				Show_STASecurityInfo_Proc},
#endif /* CONFIG_STA_SUPPORT */
#ifdef SINGLE_SKU
	{"ModuleTxpower",			Show_ModuleTxpower_Proc},
#endif /* SINGLE_SKU */
#endif /* DBG */
	{"rainfo",					Show_STA_RAInfo_Proc},
	{NULL, NULL}
};

INT RTMPShowCfgValue(
	IN	PRTMP_ADAPTER	pAd,
	IN	RTMP_STRING *pName,
	IN	RTMP_STRING *pBuf,
	IN	UINT32			MaxLen)
{
	INT	Status = 0;

	for (PRTMP_PRIVATE_STA_SHOW_CFG_VALUE_PROC = RTMP_PRIVATE_STA_SHOW_CFG_VALUE_PROC;
		 PRTMP_PRIVATE_STA_SHOW_CFG_VALUE_PROC->name; PRTMP_PRIVATE_STA_SHOW_CFG_VALUE_PROC++) {
		if (!strcmp(pName, PRTMP_PRIVATE_STA_SHOW_CFG_VALUE_PROC->name)) {
			if (PRTMP_PRIVATE_STA_SHOW_CFG_VALUE_PROC->show_proc(pAd, pBuf, MaxLen))
				Status = -EINVAL;

			break;  /*Exit for loop.*/
		}
	}

	if (PRTMP_PRIVATE_STA_SHOW_CFG_VALUE_PROC->name == NULL) {
		snprintf(pBuf, MaxLen, "\n");

		for (PRTMP_PRIVATE_STA_SHOW_CFG_VALUE_PROC = RTMP_PRIVATE_STA_SHOW_CFG_VALUE_PROC;
			 PRTMP_PRIVATE_STA_SHOW_CFG_VALUE_PROC->name; PRTMP_PRIVATE_STA_SHOW_CFG_VALUE_PROC++) {
			if ((strlen(pBuf) + strlen(PRTMP_PRIVATE_STA_SHOW_CFG_VALUE_PROC->name)) >= MaxLen)
				break;

			sprintf(pBuf, "%s%s\n", pBuf, PRTMP_PRIVATE_STA_SHOW_CFG_VALUE_PROC->name);
		}
	}

	return Status;
}

INT32 ShowBBPInfo(RTMP_ADAPTER *pAd, RTMP_STRING *Arg)
{
	ShowAllBBP(pAd);
	return TRUE;
}

INT32 ShowRFInfo(RTMP_ADAPTER *pAd, RTMP_STRING *Arg)
{
	ShowAllRF(pAd);
	return 0;
}

#define WIFI_INTERRUPT_NUM_MAX  1

INT32 ShowWifiInterruptCntProc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	const UCHAR WifiIntMaxNum = WIFI_INTERRUPT_NUM_MAX;
	const CHAR WifiIntDesc[WIFI_INTERRUPT_NUM_MAX][32] = {"Wifi Abnormal counter"};
	UINT32 WifiIntCnt[WIFI_INTERRUPT_NUM_MAX];
	UINT32 WifiIntMask = 0xF;
	UCHAR BandIdx;
	UINT32 WifiIntIdx;

	os_zero_mem(WifiIntCnt, sizeof(WifiIntCnt));

	for (BandIdx = 0; BandIdx < DBDC_BAND_NUM; BandIdx++) {
		MtCmdGetWifiInterruptCnt(pAd, BandIdx, WifiIntMaxNum, WifiIntMask, WifiIntCnt);

		for (WifiIntIdx = 0; WifiIntIdx < WifiIntMaxNum; WifiIntIdx++)
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("Band %u:%s = %u\n", BandIdx, WifiIntDesc[WifiIntIdx],
					 WifiIntCnt[WifiIntIdx]));
	}

	return TRUE;
}

#ifdef BACKGROUND_SCAN_SUPPORT
INT set_background_scan(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR IfIdx;
	struct wifi_dev *wdev = NULL;
	UCHAR band_idx = BAND0;
	UINT8 BgndscanType = os_str_tol(arg, 0, 10);
	UINT8 bgnd_band_scan_info = 0;

	if ((pObj->ioctl_if_type == INT_MBSSID) || (pObj->ioctl_if_type == INT_MAIN)) {
		IfIdx = pObj->ioctl_if;
		wdev = &pAd->ApCfg.MBSSID[IfIdx].wdev;
	} else
		return FALSE;

	if (!wdev)
		return FALSE;

	band_idx = HcGetBandByWdev(wdev);

	/* Bit[7:4]: band_idx, Bit[3:0]: BgndscanType*/
	bgnd_band_scan_info |= BgndscanType;
	bgnd_band_scan_info |= (band_idx << BGND_BAND_IDX_SHFT);

	BackgroundScanStart(pAd, wdev, bgnd_band_scan_info);
	return TRUE;
}

#if (RDD_2_SUPPORTED == 1)
INT set_background_scan_cfg(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	INT ipith = 0;
	INT32 Recv = 0;

	Recv = sscanf(arg, "%d", &(ipith));

	if (Recv != 1) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("Format Error!\n"));
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
				 ("iwpriv ra0 set bgndscancfg=[IPI_TH]\n"));
	} else {
		pAd->BgndScanCtrl.ipi_th = ipith;
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("ipith = %d\n", ipith));
	}

	return TRUE;
}

#else
INT set_background_scan_cfg(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UINT32	bgndscanduration = 0; /* ms */
	UINT32	bgndscaninterval = 0; /* second */
	UINT32	bgndscannoisyth = 0;
	UINT32	bgndscanchbusyth = 0;
	UINT32	DriverTrigger = 0;
	UINT32	bgndscansupport = 0;
	UINT32	ipith = 0;
	INT32	Recv = 0;

	Recv = sscanf(arg, "%d-%d-%d-%d-%d-%d-%d", &(bgndscanduration), &(bgndscaninterval), &(bgndscannoisyth),
				  &(bgndscanchbusyth), &(ipith), &(DriverTrigger), &(bgndscansupport));

	if (Recv != 7) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("Format Error!\n"));
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
				 ("iwpriv ra0 set bgndscancfg=[Scan_Duration]-[Partial_Scan_Interval]-[Noisy_TH]-[BusyTime_TH]-[IPI_TH]-[Driver_trigger_Support]-[BGND_Support]\n"));
	} else {
		pAd->BgndScanCtrl.ScanDuration = bgndscanduration;
		pAd->BgndScanCtrl.PartialScanInterval = bgndscaninterval;
		pAd->BgndScanCtrl.NoisyTH = bgndscannoisyth;
		pAd->BgndScanCtrl.ChBusyTimeTH = bgndscanchbusyth;
		pAd->BgndScanCtrl.DriverTrigger = (BOOL)DriverTrigger;
		pAd->BgndScanCtrl.BgndScanSupport = (BOOL)bgndscansupport;
		pAd->BgndScanCtrl.IPIIdleTimeTH = (BOOL)ipith;
	}

	return TRUE;
}
#endif

INT set_background_scan_test(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	INT	i;
	CHAR *value = 0;
	MT_BGND_SCAN_CFG BgndScanCfg;

	os_zero_mem(&BgndScanCfg, sizeof(MT_BGND_SCAN_CFG));

	for (i = 0, value = rstrtok(arg, ":"); value; value = rstrtok(NULL, ":"), i++) {
		switch (i) {
		case 0: /* ControlChannel */
			BgndScanCfg.ControlChannel = os_str_tol(value, 0, 10);
			break;

		case 1: /*  CentralChannel */
			BgndScanCfg.CentralChannel = os_str_tol(value, 0, 10);
			break;

		case 2: /* BW */
			BgndScanCfg.Bw = os_str_tol(value, 0, 10);
			break;

		case 3: /* TxStream */
			BgndScanCfg.TxStream = os_str_tol(value, 0, 10);
			break;

		case 4: /* RxPath */
			BgndScanCfg.RxPath = os_str_tol(value, 0, 16);
			break;

		case 5: /* Reason */
			BgndScanCfg.Reason = os_str_tol(value, 0, 10);
			break;

		case 6: /* BandIdx */
			BgndScanCfg.BandIdx = os_str_tol(value, 0, 10);
			break;

		default:
			break;
		}
	}

	MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
			 ("%s  Bandidx=%d, BW=%d, CtrlCh=%d, CenCh=%d, Reason=%d, RxPath=%d\n",
			  __func__, BgndScanCfg.BandIdx, BgndScanCfg.Bw, BgndScanCfg.ControlChannel,
			  BgndScanCfg.CentralChannel, BgndScanCfg.Reason, BgndScanCfg.RxPath));
	BackgroundScanTest(pAd, BgndScanCfg);
	return TRUE;
}
INT set_background_scan_notify(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	CHAR *value = 0;
	MT_BGND_SCAN_NOTIFY BgScNotify;
	int i;

	os_zero_mem(&BgScNotify, sizeof(MT_BGND_SCAN_NOTIFY));

	for (i = 0, value = rstrtok(arg, ":"); value; value = rstrtok(NULL, ":"), i++) {
		switch (i) {
		case 0: /* Notify function */
			BgScNotify.NotifyFunc = os_str_tol(value, 0, 10);
			break;

		case 1: /*  Status */
			BgScNotify.BgndScanStatus = os_str_tol(value, 0, 10);
			break;

		default:
			break;
		}
	}

	MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s  NotifyFunc=%d, BgndScanStatus=%d\n",
			 __func__, BgScNotify.NotifyFunc, BgScNotify.BgndScanStatus));
	MtCmdBgndScanNotify(pAd, BgScNotify);
	return TRUE;
}

INT show_background_scan_info(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	MTWF_LOG(DBG_CAT_RX, DBG_SUBCAT_ALL, DBG_LVL_OFF,
			 (" Background scan support = %d\n", pAd->BgndScanCtrl.BgndScanSupport));

	if (pAd->BgndScanCtrl.BgndScanSupport == TRUE) {
		MTWF_LOG(DBG_CAT_RX, DBG_SUBCAT_ALL, DBG_LVL_OFF,
				 ("===== Configuration =====\n"));
		MTWF_LOG(DBG_CAT_RX, DBG_SUBCAT_ALL, DBG_LVL_OFF,
				 (" Channel busy time Threshold = %d\n", pAd->BgndScanCtrl.ChBusyTimeTH));
		MTWF_LOG(DBG_CAT_RX, DBG_SUBCAT_ALL, DBG_LVL_OFF,
				 (" Noisy Threshold = %d\n", pAd->BgndScanCtrl.NoisyTH));
		MTWF_LOG(DBG_CAT_RX, DBG_SUBCAT_ALL, DBG_LVL_OFF,
				 (" IPI Idle Threshold (*8us) = %d\n", pAd->BgndScanCtrl.IPIIdleTimeTH));
		MTWF_LOG(DBG_CAT_RX, DBG_SUBCAT_ALL, DBG_LVL_OFF,
				 (" Scan Duration = %d ms\n", pAd->BgndScanCtrl.ScanDuration));
		MTWF_LOG(DBG_CAT_RX, DBG_SUBCAT_ALL, DBG_LVL_OFF,
				 (" Partial Scan Interval = %d second\n", pAd->BgndScanCtrl.PartialScanInterval));
		MTWF_LOG(DBG_CAT_RX, DBG_SUBCAT_ALL, DBG_LVL_OFF,
				 (" DriverTrigger support= %d\n", pAd->BgndScanCtrl.DriverTrigger));
		MTWF_LOG(DBG_CAT_RX, DBG_SUBCAT_ALL, DBG_LVL_OFF,
				 ("===== Status / Statistic =====\n"));
		MTWF_LOG(DBG_CAT_RX, DBG_SUBCAT_ALL, DBG_LVL_OFF,
				 (" One sec channel busy time = %d\n", pAd->OneSecMibBucket.ChannelBusyTimeCcaNavTx[0]));
		MTWF_LOG(DBG_CAT_RX, DBG_SUBCAT_ALL, DBG_LVL_OFF,
				 (" One sec primary channel busy time = %d\n", pAd->OneSecMibBucket.ChannelBusyTime[0]));
		MTWF_LOG(DBG_CAT_RX, DBG_SUBCAT_ALL, DBG_LVL_OFF,
				 (" One sec My Tx Airtime = %d\n", pAd->OneSecMibBucket.MyTxAirtime[0]));
		MTWF_LOG(DBG_CAT_RX, DBG_SUBCAT_ALL, DBG_LVL_OFF,
				 (" One sec My Rx Airtime = %d\n", pAd->OneSecMibBucket.MyRxAirtime[0]));
		MTWF_LOG(DBG_CAT_RX, DBG_SUBCAT_ALL, DBG_LVL_OFF,
				 (" IPI Idle time = %d\n", pAd->BgndScanCtrl.IPIIdleTime));
		MTWF_LOG(DBG_CAT_RX, DBG_SUBCAT_ALL, DBG_LVL_OFF,
				 (" Noisy = %d\n", pAd->BgndScanCtrl.Noisy));
		MTWF_LOG(DBG_CAT_RX, DBG_SUBCAT_ALL, DBG_LVL_OFF,
				 (" Current state = %ld\n", pAd->BgndScanCtrl.BgndScanStatMachine.CurrState));
		MTWF_LOG(DBG_CAT_RX, DBG_SUBCAT_ALL, DBG_LVL_OFF,
				 (" Scan type = %d\n", pAd->BgndScanCtrl.ScanType));
		/* MTWF_LOG(DBG_CAT_RX, DBG_SUBCAT_ALL, DBG_LVL_OFF, */
		/* (" Interval count = %d\n", pAd->BgndScanCtrl.BgndScanIntervalCount)); */
		/* MTWF_LOG(DBG_CAT_RX, DBG_SUBCAT_ALL, DBG_LVL_OFF, */
		/* (" Interval = %d\n", pAd->BgndScanCtrl.BgndScanInterval)); */
	}

	return TRUE;
}
#endif /* BACKGROUND_SCAN_SUPPORT */

#if defined(INTERNAL_CAPTURE_SUPPORT) || defined(WIFI_SPECTRUM_SUPPORT)
/*
    ==========================================================================
    Description:
	RF test switch mode.

	iwpriv ra0 set RBIST_SwitchMode = ModeEnable

	ModeEnable
	0: OPERATION_NORMAL_MODE
	1: OPERATION_RFTEST_MODE
	2: OPERATION_ICAP_MODE
	4: OPERATION_WIFI_SPECTRUM

    Return:
    ==========================================================================
*/
INT32 Set_RBIST_Switch_Mode(
	IN RTMP_ADAPTER *pAd,
	IN RTMP_STRING *arg)
{
	UINT8 ModeEnable = 0;
#ifdef CONFIG_ATE
	UINT32 op_mode = TESTMODE_GET_PARAM(pAd, TESTMODE_BAND0, op_mode);
#endif/* CONFIG_ATE */

	ModeEnable = simple_strtol(arg, 0, 10);
	if (ModeEnable == OPERATION_NORMAL_MODE) {
#ifdef CONFIG_ATE
		TESTMODE_SET_PARAM(pAd, TESTMODE_BAND0, op_mode, op_mode &= ~fATE_IN_RFTEST);
#endif/* CONFIG_ATE */
		MtCmdRfTestSwitchMode(pAd, OPERATION_NORMAL_MODE, 0,
							  RF_TEST_DEFAULT_RESP_LEN);
	} else if (ModeEnable == OPERATION_RFTEST_MODE) {
#ifdef CONFIG_ATE
		TESTMODE_SET_PARAM(pAd, TESTMODE_BAND0, op_mode, op_mode |= fATE_IN_RFTEST);
#endif/* CONFIG_ATE */
		MtCmdRfTestSwitchMode(pAd, OPERATION_RFTEST_MODE, 0,
							  RF_TEST_DEFAULT_RESP_LEN);
	} else if (ModeEnable == OPERATION_ICAP_MODE) {
#ifdef CONFIG_ATE
		TESTMODE_SET_PARAM(pAd, TESTMODE_BAND0, op_mode, op_mode |= fATE_IN_RFTEST);
#endif/* CONFIG_ATE */
		MtCmdRfTestSwitchMode(pAd, OPERATION_ICAP_MODE, 0,
							  RF_TEST_DEFAULT_RESP_LEN);
	} else if (ModeEnable == OPERATION_WIFI_SPECTRUM) {
#ifdef CONFIG_ATE
		TESTMODE_SET_PARAM(pAd, TESTMODE_BAND0, op_mode, op_mode &= ~fATE_IN_RFTEST);
#endif/* CONFIG_ATE */
		MtCmdRfTestSwitchMode(pAd, OPERATION_WIFI_SPECTRUM, 0,
							  RF_TEST_DEFAULT_RESP_LEN);
	} else {
#ifdef CONFIG_ATE
		TESTMODE_SET_PARAM(pAd, TESTMODE_BAND0, op_mode, op_mode &= ~fATE_IN_RFTEST);
#endif/* CONFIG_ATE */
		MtCmdRfTestSwitchMode(pAd, OPERATION_NORMAL_MODE, 0,
							  RF_TEST_DEFAULT_RESP_LEN);
	}

	return TRUE;
}

/*
    ==========================================================================
    Description:
	Set parameters when ICap/Wifi-spectrum is started or stopped.

	iwpriv ra0 set RBIST_CaptureStart
	= Mode : Trigger : RingCapEn : TriggerEvent : CaptureNode : CaptureLen :
	  CapStopCycle : BW : MACTriggerEvent : SourceAddr. : Band : PhyIdx : CapSrc

    Return:
    ==========================================================================
*/
INT32 Set_RBIST_Capture_Start(
	IN RTMP_ADAPTER *pAd,
	IN RTMP_STRING *arg)
{
	INT32 i, j, retval;
	RTMP_STRING Temp1[2] = {0};
	INT8 *pTemp1 = Temp1;
	INT8 *value = NULL;
	UINT32 Temp2[6] = {0};
	UINT32 Mode = 0, Trig = 0, RingCapEn = 0, BBPTrigEvent = 0, CapNode = 0;
	UINT32 CapLen = 0, CapStopCycle = 0, MACTrigEvent = 0, PhyIdx = 0;
	UINT32 SrcAddrLSB = 0, SrcAddrMSB = 0, BandIdx = 0, BW = 0, CapSrc = 0;
	RBIST_CAP_START_T *prRBISTInfo = NULL;

	/* Dynamic allocate memory for prRBISTInfo */
	retval = os_alloc_mem(pAd, (UCHAR **)&prRBISTInfo, sizeof(RBIST_CAP_START_T));
	if (retval != NDIS_STATUS_SUCCESS) {
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
				("%s : Not enough memory for dynamic allocating !!\n", __func__));
		goto error;
	}
	os_zero_mem(prRBISTInfo, sizeof(RBIST_CAP_START_T));

	for (i = 0, value = rstrtok(arg, ":"); value; value = rstrtok(NULL, ":"), i++) {
		switch (i) {
		case CAP_MODE:
			Mode = simple_strtol(value, 0, 16);
			break;

		case CAP_TRIGGER:
			Trig = simple_strtol(value, 0, 16);
			prRBISTInfo->fgTrigger = Trig;
			break;

		case CAP_RING_MODE:
			RingCapEn = simple_strtol(value, 0, 16);
			prRBISTInfo->fgRingCapEn = RingCapEn;
			break;

		case CAP_BBP_EVENT:
			BBPTrigEvent = simple_strtol(value, 0, 16);
			prRBISTInfo->u4TriggerEvent = BBPTrigEvent;
			break;

		case CAP_NODE:
			CapNode = simple_strtol(value, 0, 16);
			prRBISTInfo->u4CaptureNode = CapNode;
			break;

		case CAP_LENGTH:
			CapLen = simple_strtol(value, 0, 16);
			prRBISTInfo->u4CaptureLen = CapLen;
			break;

		case CAP_STOP_CYCLE:
			CapStopCycle = simple_strtol(value, 0, 16);
			prRBISTInfo->u4CapStopCycle = CapStopCycle;
			break;

		case CAP_BW:
			BW = simple_strtol(value, 0, 16);
			prRBISTInfo->u4BW = BW;
			break;

		case CAP_MAC_EVENT:
			MACTrigEvent = simple_strtol(value, 0, 16);
			prRBISTInfo->u4MACTriggerEvent = MACTrigEvent;
			break;

		case CAP_SOURCE_ADDR:
			for (j = 0; j < 6; j++) {
				RTMPMoveMemory(pTemp1, value, 2);
				Temp2[j] = simple_strtol(pTemp1, 0, 16);
				value += 2;
			}

			SrcAddrLSB = (Temp2[0] | (Temp2[1] << 8) |
						  (Temp2[2] << 16) | (Temp2[3] << 24));
			SrcAddrMSB = (Temp2[4] | (Temp2[5] << 8) | (0x1 << 16));
			prRBISTInfo->u4SourceAddressLSB = SrcAddrLSB;
			prRBISTInfo->u4SourceAddressMSB = SrcAddrMSB;
			break;

		case CAP_BAND:
			BandIdx = simple_strtol(value, 0, 16);
			prRBISTInfo->u4BandIdx = BandIdx;
			break;

		case CAP_PHY:
			PhyIdx = simple_strtol(value, 0, 16);
			prRBISTInfo->u4PhyIdx = PhyIdx;
			break;

		case CAP_SOURCE:
			CapSrc = simple_strtol(value, 0, 16);
			prRBISTInfo->u4CapSource = CapSrc;
			break;

		default:
			break;
		}
	}

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s :\n Mode = 0x%08x\n"
			" Trigger = 0x%08x\n RingCapEn = 0x%08x\n TriggerEvent = 0x%08x\n CaptureNode = 0x%08x\n"
			" CaptureLen = 0x%08x\n CapStopCycle = 0x%08x\n BW = 0x%08x\n MACTriggerEvent = 0x%08x\n"
			" SourceAddrLSB = 0x%08x\n SourceAddrMSB = 0x%08x\n Band = 0x%08x\n PhyIdx = 0x%08x\n"
			" CapSrc = 0x%08x\n", __func__, Mode, Trig, RingCapEn, BBPTrigEvent, CapNode, CapLen,
			CapStopCycle, BW, MACTrigEvent, SrcAddrLSB, SrcAddrMSB, BandIdx, PhyIdx, CapSrc));

	if (Mode == ICAP_MODE) {
#ifdef INTERNAL_CAPTURE_SUPPORT
		struct _RTMP_CHIP_OP *ops = hc_get_chip_ops(pAd->hdev_ctrl);

		if (ops->ICapStart != NULL)
			ops->ICapStart(pAd, (UINT8 *)prRBISTInfo);
		else {
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
					("%s : The function is not hooked !!\n", __func__));
		}
#endif /* INTERNAL_CAPTURE_SUPPORT */
	} else if (Mode == WIFI_SPECTRUM_MODE) {
#ifdef WIFI_SPECTRUM_SUPPORT
		struct _RTMP_CHIP_OP *ops = hc_get_chip_ops(pAd->hdev_ctrl);

		if (ops->SpectrumStart != NULL)
			ops->SpectrumStart(pAd, (UINT8 *)prRBISTInfo);
		else {
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
					("%s : The function is not hooked !!\n", __func__));
		}
#endif /* WIFI_SPECTRUM_SUPPORT */
	}

error:
	if (prRBISTInfo != NULL)
		os_free_mem(prRBISTInfo);

	return TRUE;
}

/*
    ==========================================================================
    Description:
	Query ICap/Wifi-spectrum status.

	iwpriv ra0 set RBIST_CaptureStatus = Choice

	 Choice
	 0: ICAP_MODE
	 1: WIFI_SPECTRUM_MODE

    Return:
    ==========================================================================
*/
INT32 Get_RBIST_Capture_Status(
	IN RTMP_ADAPTER *pAd,
	IN RTMP_STRING *arg)
{
	INT32 Choice;

	Choice = simple_strtol(arg, 0, 10);
	switch (Choice) {
	case ICAP_MODE:
#ifdef INTERNAL_CAPTURE_SUPPORT
	{
		struct _RTMP_CHIP_OP *ops = hc_get_chip_ops(pAd->hdev_ctrl);

		if (ops->ICapStatus != NULL)
			ops->ICapStatus(pAd);
		else {
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
					("%s : The function is not hooked !!\n", __func__));
		}
	}
#endif /* INTERNAL_CAPTURE_SUPPORT */
		break;

	case WIFI_SPECTRUM_MODE:
#ifdef WIFI_SPECTRUM_SUPPORT
	{
		struct _RTMP_CHIP_OP *ops = hc_get_chip_ops(pAd->hdev_ctrl);

		if (ops->SpectrumStatus != NULL)
			ops->SpectrumStatus(pAd);
		else {
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
					("%s : The function is not hooked !!\n", __func__));
		}
	}
#endif /* WIFI_SPECTRUM_SUPPORT */
		break;

	default:
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
				("%s: Not support for %d this selection !!\n", __func__, Choice));
		break;
	}

	return TRUE;
}

/*
    ==========================================================================
    Description:
	 Get ICap/Wifi-spectrum RBIST sysram raw data .

	 iwpriv ra0 set RBIST_RawDataProc = Choice

	 Choice
	 0: ICAP_MODE
	    a. Get ICap RBIST sysram raw data by unsolicited event.(on-the-fly)
	    b. Re-arrange ICap sysram buffer by wrapper.
	    c. Parsing ICap I/Q data.
	    d. Dump L32bit/M32bit/H32bit to file.
	 1: WIFI_SPECTRUM_MODE
	    a. Get Wifi-spectrum RBIST sysram raw data by unsolicited event.(on-the-fly)
	    b. Parsing Wifi-spectrum I/Q data.
	    c. Dump I/Q/LNA/LPF data to file.
    Return:
    ==========================================================================
*/
INT32 Get_RBIST_Raw_Data_Proc(
	IN RTMP_ADAPTER *pAd,
	IN RTMP_STRING *arg)
{
	UINT32 Choice;
	INT32 Status = CAP_FAIL;

	Choice = simple_strtol(arg, 0, 10);
	switch (Choice) {
	case ICAP_MODE:
#ifdef INTERNAL_CAPTURE_SUPPORT
	{
		struct _RTMP_CHIP_OP *ops = hc_get_chip_ops(pAd->hdev_ctrl);

		if (ops->ICapCmdUnSolicitRawDataProc != NULL)
			Status = ops->ICapCmdUnSolicitRawDataProc(pAd);
		else {
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
					("%s : The function is not hooked !!\n", __func__));
		}
	}
#endif /* INTERNAL_CAPTURE_SUPPORT */
		break;
	case WIFI_SPECTRUM_MODE:
#ifdef WIFI_SPECTRUM_SUPPORT
	{
		struct _RTMP_CHIP_OP *ops = hc_get_chip_ops(pAd->hdev_ctrl);

		if (ops->SpectrumCmdRawDataProc != NULL)
			Status = ops->SpectrumCmdRawDataProc(pAd);
		else {
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
					("%s : The function is not hooked !!\n", __func__));
		}
	}
#endif /* WIFI_SPECTRUM_SUPPORT */
		break;

	default:
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
				("%s: Not support for %d this selection !!\n", __func__, Choice));
		break;
	}

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF,
			("%s:(Status = %d)\n", __func__, Status));

	return TRUE;
}

/*
    ==========================================================================
    Description:
	 Get ICap I/Q data which is stored in IQ_Array captured by
	 WF0/WF1/WF2/WF3.

    Return:
    ==========================================================================
*/
INT32 Get_RBIST_IQ_Data(
	IN RTMP_ADAPTER *pAd,
	IN PINT32 pData,
	IN PINT32 pDataLen,
	IN UINT32 IQ_Type,
	IN UINT32 WF_Num)
{
	UINT32 i, CapNode, TotalCnt, Len, CapSrc;
	RTMP_CHIP_CAP *pChipCap = hc_get_chip_cap(pAd->hdev_ctrl);
	P_RBIST_IQ_DATA_T pIQ_Array = pAd->pIQ_Array;

	/* Initialization of pData and pDataLen buffer */
	Len = ICAP_EVENT_DATA_SAMPLE * sizeof(INT32);
	os_zero_mem(pData, Len);
	os_zero_mem(pDataLen, sizeof(INT32));

	/* Query current capture node */
	CapNode = pAd->ICapCapNode;

	/* Query current capture source */
	CapSrc = pAd->ICapCapSrc;

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
			("%s : CapNode = 0x%08x, CapSrc = 0x%08x\n", __func__, CapNode, CapSrc));

	/* Update total count of I or Q sample of each WF */
	if (IS_MT7626(pAd)) {
		if ((CapNode == pChipCap->ICapWF01PackedADC)
			|| (CapNode == pChipCap->ICapWF02PackedADC)
			|| (CapNode == pChipCap->ICapWF12PackedADC))
			TotalCnt = pChipCap->ICapADCIQCnt;
		else
			TotalCnt = pChipCap->ICapIQCIQCnt;
	} else {
		if (CapNode == pChipCap->ICapPackedADC)
			TotalCnt = pChipCap->ICapADCIQCnt;
		else
			TotalCnt = pChipCap->ICapIQCIQCnt;
	}

	/* Update initial value of ICapDataCnt if user want to display short length of data */
	if ((TotalCnt > pAd->ICapCapLen) && (pAd->ICapDataCnt == 0))
		pAd->ICapDataCnt = TotalCnt - pAd->ICapCapLen;

	/* Store I or Q data(1KBytes) to data buffer */
	for (i = 0; i < ICAP_EVENT_DATA_SAMPLE; i++) {
		UINT32 idx = pAd->ICapDataCnt;

		/* If it is the last one of I or Q data, just stop querying */
		if (pAd->ICapDataCnt == TotalCnt)
			break;

		/* Store I/Q data to data buffer */
		pData[i] = pIQ_Array[idx].IQ_Array[WF_Num][IQ_Type];
		/* Update data counter */
		pAd->ICapDataCnt++;
	}

	/* Update data length */
	*pDataLen = i;

	/* Reset data counter */
	if (*pDataLen == 0)
		pAd->ICapDataCnt = 0;

	return TRUE;
}

/*
    ==========================================================================
    Description:
	 Get ICap I/Q data which is captured by WF0 or WF1 or WF2 or WF3.

	 iwpriv ra0 set RBIST_IQDataProc = IQ_Type : WF_Num : ICap_Len

	 IQ_Type
	 0: I_TYPE/1: Q_TYPE
	 WF_Num
	 0: WF0/1: WF1/2: WF2/3: WF3
	 ICap_Len(Unit: I or Q sample cnt)

	 a. Store I/Q data which is captured by WF0/WF1/WF2/WF3 to data buffer.
	 b. Dump I/Q data to file.

    Return:
    ==========================================================================
*/
#define CAP_IQ_Type			0
#define CAP_WF_Num			1
#define CAP_Len				2
INT32 Get_RBIST_IQ_Data_Proc(
	IN RTMP_ADAPTER *pAd,
	IN RTMP_STRING *arg)
{
	INT i, retval;
	UINT32 IQ_Type = 0, WF_Num = 0, Len;
	PINT32 pData = NULL, pDataLen = NULL;
	RTMP_STRING *value = NULL;
	RTMP_STRING *pSrc_IQ = NULL;

	for (i = 0, value = rstrtok(arg, ":"); value; value = rstrtok(NULL, ":"), i++) {
		switch (i) {
		case CAP_IQ_Type:
			IQ_Type = simple_strtol(value, 0, 10);
			break;

		case CAP_WF_Num:
			WF_Num = simple_strtol(value, 0, 10);
			break;
		case CAP_Len:
			pAd->ICapCapLen = simple_strtol(value, 0, 10);
			break;
		default:
			break;
		}
	}

	/* Dynamic allocate memory for pSrc_IQ */
	retval = os_alloc_mem(pAd, (UCHAR **)&pSrc_IQ, sizeof(RTMP_STRING));
	if (retval != NDIS_STATUS_SUCCESS) {
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
				("%s : Not enough memory for dynamic allocating !!\n", __func__));
		goto error;
	}
	os_zero_mem(pSrc_IQ, sizeof(RTMP_STRING));

	/* Dynamic allocate memory for 1KByte data buffer */
	Len = ICAP_EVENT_DATA_SAMPLE * sizeof(INT32);
	retval = os_alloc_mem(pAd, (UCHAR **)&pData, Len);
	if (retval != NDIS_STATUS_SUCCESS) {
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
				("%s : Not enough memory for dynamic allocating !!\n", __func__));
		goto error;
	}
	os_zero_mem(pData, Len);

	/* Dynamic allocate memory for data length */
	retval = os_alloc_mem(pAd, (UCHAR **)&pDataLen, sizeof(INT32));
	if (retval != NDIS_STATUS_SUCCESS) {
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
				("%s : Not enough memory for dynamic allocating !!\n", __func__));
		goto error;
	}
	os_zero_mem(pDataLen, sizeof(INT32));

	/* Fill in title for console log */
	if (IQ_Type == CAP_I_TYPE)
		sprintf(pSrc_IQ, "Icap_%s%d", "I", WF_Num);
	else if (IQ_Type == CAP_Q_TYPE)
		sprintf(pSrc_IQ, "Icap_%s%d", "Q", WF_Num);
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s\n", pSrc_IQ));

	/* Initialization of ICapDataCnt */
	pAd->ICapDataCnt = 0;

	while (1) {
#ifdef INTERNAL_CAPTURE_SUPPORT
		struct _RTMP_CHIP_OP *ops = hc_get_chip_ops(pAd->hdev_ctrl);

		/* Query I/Q data from buffer */
		if (ops->ICapGetIQData != NULL)
			ops->ICapGetIQData(pAd, pData, pDataLen, IQ_Type, WF_Num);
		else if (ops->ICapCmdSolicitRawDataProc != NULL)
			ops->ICapCmdSolicitRawDataProc(pAd, pData, pDataLen, IQ_Type, WF_Num);
		else {
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
					("%s : The function is not hooked !!\n", __func__));
		}
#endif /* INTERNAL_CAPTURE_SUPPORT */

		/* If data length is zero, it means the end of data querying */
		if (*pDataLen == 0)
			break;

		/* Print data log to console */
		for (i = 0; i < *pDataLen; i++)
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%d\n", pData[i]));
	}

error:
	if (pData != NULL)
		os_free_mem(pData);

	if (pDataLen != NULL)
		os_free_mem(pDataLen);

	if (pSrc_IQ != NULL)
		os_free_mem(pSrc_IQ);

	return TRUE;
}

/*
    ==========================================================================
    Description:
	 Get Icap/Wifi-spectrum capture node information.

    Return: Value of capture node.
    ==========================================================================
*/
UINT32 Get_System_CapNode_Info(
	IN RTMP_ADAPTER *pAd)
{
	UINT32 CapNode = 0;

	CapNode = pAd->ICapCapNode;
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
			("%s : CaptureNode = 0x%08x\n", __func__, CapNode));

	return CapNode;
}

/*
    ==========================================================================
    Description:
	 Get current band central frequency information.

    Return: Value of central frequency(MHz).
    ==========================================================================
*/
UINT32 Get_System_CenFreq_Info(
	IN RTMP_ADAPTER *pAd,
	IN UINT32 CapNode)
{
	UINT32 ChIdx, CenFreq = 0;
	UINT8 CenCh = 0;
	struct freq_oper oper;
	UCHAR rfic = RFIC_24GHZ;
	RTMP_CHIP_CAP *pChipCap = hc_get_chip_cap(pAd->hdev_ctrl);

	if (pAd->CommonCfg.dbdc_mode) { /* Dual Band */
#if defined(INTERNAL_CAPTURE_SUPPORT) || defined(WIFI_SPECTRUM_SUPPORT)
		if ((CapNode == pChipCap->SpectrumWF0ADC)  || (CapNode == pChipCap->SpectrumWF1ADC)
			|| (CapNode == pChipCap->SpectrumWF0FIIQ) || (CapNode == pChipCap->SpectrumWF1FIIQ)
			|| (CapNode == pChipCap->SpectrumWF0FDIQ) || (CapNode == pChipCap->SpectrumWF1FDIQ))
			rfic = RFIC_24GHZ;
		else if ((CapNode == pChipCap->SpectrumWF2ADC)  || (CapNode == pChipCap->SpectrumWF3ADC)
				 || (CapNode == pChipCap->SpectrumWF2FIIQ) || (CapNode == pChipCap->SpectrumWF3FIIQ)
				 || (CapNode == pChipCap->SpectrumWF2FDIQ) || (CapNode == pChipCap->SpectrumWF3FDIQ))
			rfic =  RFIC_5GHZ;
#endif
	} else { /* Single Band */
		if (HcGetRadioChannel(pAd) <= 14)
			rfic = RFIC_24GHZ;
		else
			rfic = RFIC_5GHZ;
	}

	if (hc_radio_query_by_rf(pAd, rfic, &oper) != HC_STATUS_OK) {
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF,
				("%s : can't find radio for RFIC:%d\n", __func__, rfic));
	}

	CenCh = oper.cen_ch_1;
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF,
			("%s : CentralCh = %d\n", __func__, CenCh));

	for (ChIdx = 0; ChIdx < CH_HZ_ID_MAP_NUM; ChIdx++) {
		if (CenCh == CH_HZ_ID_MAP[ChIdx].channel) {
			CenFreq = CH_HZ_ID_MAP[ChIdx].freqKHz;
			break;
		}
	}

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF,
			("%s : CentralFreq = %d\n", __func__, CenFreq));

	return CenFreq;
}

/*
    ==========================================================================
    Description:
	 Get current band bandwidth information.

    Return: Value of capture BW.
	    CAP_BW_20                            0
	    CAP_BW_40                            1
	    CAP_BW_80                            2
    ==========================================================================
*/
UINT8 Get_System_Bw_Info(
	IN RTMP_ADAPTER *pAd,
	IN UINT32 CapNode)
{
	INT8 Bw = 0, CapBw = 0;
	struct freq_oper oper;
	UCHAR rfic = RFIC_24GHZ;
	RTMP_CHIP_CAP *pChipCap = hc_get_chip_cap(pAd->hdev_ctrl);

	if (pAd->CommonCfg.dbdc_mode) { /* Dual Band */
		if ((CapNode == pChipCap->SpectrumWF0ADC)  || (CapNode == pChipCap->SpectrumWF1ADC)
			|| (CapNode == pChipCap->SpectrumWF0FIIQ) || (CapNode == pChipCap->SpectrumWF1FIIQ)
			|| (CapNode == pChipCap->SpectrumWF0FDIQ) || (CapNode == pChipCap->SpectrumWF1FDIQ))
			rfic = RFIC_24GHZ;
		else if ((CapNode == pChipCap->SpectrumWF2ADC)  || (CapNode == pChipCap->SpectrumWF3ADC)
				 || (CapNode == pChipCap->SpectrumWF2FIIQ) || (CapNode == pChipCap->SpectrumWF3FIIQ)
				 || (CapNode == pChipCap->SpectrumWF2FDIQ) || (CapNode == pChipCap->SpectrumWF3FDIQ))
			rfic = RFIC_5GHZ;
	} else { /* Single Band */
		if (HcGetRadioChannel(pAd) <= 14)
			rfic = RFIC_24GHZ;
		else
			rfic = RFIC_5GHZ;
	}

	if (hc_radio_query_by_rf(pAd, rfic, &oper) != HC_STATUS_OK) {
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
				("%s : can't find radio for RFIC:%d\n", __func__, rfic));

		return -1;
	}

		Bw = oper.bw;
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF,
			("%s : Bw = %d\n", __func__, Bw));

	switch (Bw) {
	case CMD_BW_20:
		CapBw = CAP_BW_20;
		break;

	case CMD_BW_40:
		CapBw = CAP_BW_40;
		break;

	case CMD_BW_80:
		CapBw = CAP_BW_80;
		break;

	case CMD_BW_160:
		CapBw = CAP_BW_80;
		break;

	case CMD_BW_8080:
		CapBw = CAP_BW_80;
		break;

	default:
		CapBw = CAP_BW_20;
		break;
	}

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF,
			("%s : CaptureBw = %d\n", __func__, CapBw));

	return CapBw;
}

/*
    ==========================================================================
    Description:
	 Used for getting current band wireless information.

	 iwpriv ra0 set WirelessInfo = Choice

	 Choice
	 0: CentralFreq
	 1: Bw

    Return:
    ==========================================================================
*/
#define CEN_FREQ			0
#define SYS_BW				1
INT32 Get_System_Wireless_Info(
	IN RTMP_ADAPTER *pAd,
	IN RTMP_STRING *arg)
{
	UINT32 CapNode = 0;
	UINT16 CenFreq = 0;
	UINT8 Bw = 0;
	INT32 Choice;

	Choice = simple_strtol(arg, 0, 10);
	switch (Choice) {
	case CEN_FREQ:
		CapNode = Get_System_CapNode_Info(pAd);
		CenFreq = Get_System_CenFreq_Info(pAd, CapNode);
		break;

	case SYS_BW:
		CapNode = Get_System_CapNode_Info(pAd);
		Bw = Get_System_Bw_Info(pAd, CapNode);
		break;

	default:
		break;
	}

	return TRUE;
}
#endif /* defined(INTERNAL_CAPTURE_SUPPORT) || defined(WIFI_SPECTRUM_SUPPORT) */

/*
    ==========================================================================
    Description:
	 Set IRR ADC parameters.

    Return:
    ==========================================================================
*/
INT Set_IRR_ADC(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	INT i;
	CHAR    *value = 0;
	UINT32  ChannelFreq = 0;
	UINT8   AntIndex = 0;
	UINT8   BW = 0;
	UINT8   SX = 0;
	UINT8   DbdcIdx = 0;
	UINT8   RunType = 0;
	UINT8   FType = 0;

	for (i = 0, value = rstrtok(arg, ":"); value; value = rstrtok(NULL, ":"), i++) {
		switch (i) {
		case 0:
			AntIndex = os_str_tol(value, 0, 10);

			switch (AntIndex) {
			case QA_IRR_WF0:
				AntIndex = WF0;
				break;

			case QA_IRR_WF1:
				AntIndex = WF1;
				break;

			case QA_IRR_WF2:
				AntIndex = WF2;
				break;

			case QA_IRR_WF3:
				AntIndex = WF3;
				break;
			}

			break;

		case 1:
			ChannelFreq = os_str_tol(value, 0, 10);
			break;

		case 2:
			BW = os_str_tol(value, 0, 10);
			break;

		case 3:
			SX = os_str_tol(value, 0, 10);
			break;

		case 4:
			DbdcIdx = os_str_tol(value, 0, 10);
			break;

		case 5:
			RunType = os_str_tol(value, 0, 10);
			break;

		case 6:
			FType = os_str_tol(value, 0, 10);
			break;

		default:
			break;
		}
	}

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s: <SetADC> Input Checking Log\n\
					--------------------------------------------------------------\n\
					ChannelFreq = %d \n\
					AntIndex = %d \n\
					BW = %d \n\
					SX= %d \n\
					DbdcIdx = %d \n\
					RunType = %d \n\
					FType = %d \n\n", __func__, \
			 ChannelFreq, \
			 AntIndex, \
			 BW, \
			 SX, \
			 DbdcIdx, \
			 RunType, \
			 FType));
	MtCmdRfTestSetADC(pAd, ChannelFreq, AntIndex, BW, SX, DbdcIdx, RunType, FType);
	return TRUE;
}

/*
    ==========================================================================
    Description:
	 Set IRR Rx Gain parameters.

    Return:
    ==========================================================================
*/
INT Set_IRR_RxGain(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	INT i;
	CHAR    *value = 0;
	UINT8   LPFG = 0;
	UINT8   LNA = 0;
	UINT8   DbdcIdx = 0;
	UINT8   AntIndex = 0;

	for (i = 0, value = rstrtok(arg, ":"); value; value = rstrtok(NULL, ":"), i++) {
		switch (i) {
		case 0:
			LPFG = os_str_tol(value, 0, 10);
			break;

		case 1:
			LNA = os_str_tol(value, 0, 10);
			break;

		case 2:
			DbdcIdx = os_str_tol(value, 0, 10);
			break;

		case 3:
			AntIndex = os_str_tol(value, 0, 10);

			switch (AntIndex) {
			case QA_IRR_WF0:
				AntIndex = WF0;
				break;

			case QA_IRR_WF1:
				AntIndex = WF1;
				break;

			case QA_IRR_WF2:
				AntIndex = WF2;
				break;

			case QA_IRR_WF3:
				AntIndex = WF3;
				break;
			}

			break;

		default:
			break;
		}
	}

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s: <SetRxGain> Input Checking Log\n\
					--------------------------------------------------------------\n\
					LPFG = %d \n\
					LNA = %d \n\
					DbdcIdx = %d \n\
					AntIndex= %d \n\n", __func__, \
			 LPFG, \
			 LNA, \
			 DbdcIdx, \
			 AntIndex));
	MtCmdRfTestSetRxGain(pAd, LPFG, LNA, DbdcIdx, AntIndex);
	return TRUE;
}

/*
    ==========================================================================
    Description:
	 Set IRR TTG parameters.

    Return:
    ==========================================================================
*/
INT Set_IRR_TTG(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	INT i;
	CHAR    *value = 0;
	UINT32  ChannelFreq = 0;
	UINT32  ToneFreq = 0;
	UINT8   TTGPwrIdx = 0;
	UINT8	XtalFreq = 0;
	UINT8   DbdcIdx = 0;

	for (i = 0, value = rstrtok(arg, ":"); value; value = rstrtok(NULL, ":"), i++) {
		switch (i) {
		case 0:
			TTGPwrIdx = os_str_tol(value, 0, 10);
			break;

		case 1:
			ToneFreq = os_str_tol(value, 0, 10);
			break;

		case 2:
			ChannelFreq = os_str_tol(value, 0, 10);
			break;

		case 3:
			XtalFreq = os_str_tol(value, 0, 10);
			break;

		case 4:
			DbdcIdx = os_str_tol(value, 0, 10);
			break;

		default:
			break;
		}
	}

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s: <SetTTG> Input Checking Log\n\
					--------------------------------------------------------------\n\
					ChannelFreq = %d \n\
					ToneFreq = %d \n\
					TTGPwrIdx = %d \n\
					DbdcIdx= %d \n\n", __func__, \
			 ChannelFreq, \
			 ToneFreq, \
			 TTGPwrIdx, \
			 DbdcIdx));
	MtCmdRfTestSetTTG(pAd, ChannelFreq, ToneFreq, TTGPwrIdx, XtalFreq, DbdcIdx);
	return TRUE;
}

/*
    ==========================================================================
    Description:
	 Set IRR TTGOnOff parameters.

    Return:
    ==========================================================================
*/
INT Set_IRR_TTGOnOff(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	INT i;
	CHAR    *value = 0;
	UINT8   TTGEnable = 0;
	UINT8   DbdcIdx = 0;
	UINT8   AntIndex = 0;

	for (i = 0, value = rstrtok(arg, ":"); value; value = rstrtok(NULL, ":"), i++) {
		switch (i) {
		case 0:
			TTGEnable = os_str_tol(value, 0, 10);
			break;

		case 1:
			DbdcIdx = os_str_tol(value, 0, 10);
			break;

		case 2:
			AntIndex = os_str_tol(value, 0, 10);

			switch (AntIndex) {
			case QA_IRR_WF0:
				AntIndex = WF0;
				break;

			case QA_IRR_WF1:
				AntIndex = WF1;
				break;

			case QA_IRR_WF2:
				AntIndex = WF2;
				break;

			case QA_IRR_WF3:
				AntIndex = WF3;
				break;
			}

			break;

		default:
			break;
		}
	}

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s: <SetTTGOnOff> Input Checking Log\n\
					--------------------------------------------------------------\n\
					TTGEnable = %d \n\
					DbdcIdx = %d \n\
					AntIndex = %d \n\n", __func__, \
			 TTGEnable, \
			 DbdcIdx, \
			 AntIndex));
	MtCmdRfTestSetTTGOnOff(pAd, TTGEnable, DbdcIdx, AntIndex);
	return TRUE;
}

INT set_manual_protect(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	CHAR *token;
	UINT32 wdev_idx = 0, mode = 0;
	struct wifi_dev *wdev;

	if (arg == NULL)
		goto err1;

	if (arg != NULL) {
		token = strsep(&arg, "-");
		wdev_idx = os_str_tol(token, 0, 10);

		if (pAd->wdev_list[wdev_idx] == NULL)
			goto err2;
	}

	wdev = pAd->wdev_list[wdev_idx];

	while (arg != NULL) {
		token = strsep(&arg, "+");

		if (!strcmp(token, "erp"))
			mode |= SET_PROTECT(ERP);
		else if (!strcmp(token, "no"))
			mode |= SET_PROTECT(NO_PROTECTION);
		else if (!strcmp(token, "non_member"))
			mode |= SET_PROTECT(NON_MEMBER_PROTECT);
		else if (!strcmp(token, "ht20"))
			mode |= SET_PROTECT(HT20_PROTECT);
		else if (!strcmp(token, "non_ht_mixmode"))
			mode |= SET_PROTECT(NON_HT_MIXMODE_PROTECT);
		else if (!strcmp(token, "longnav"))
			mode |= SET_PROTECT(LONG_NAV_PROTECT);
		else if (!strcmp(token, "gf"))
			mode |= SET_PROTECT(GREEN_FIELD_PROTECT);
		else if (!strcmp(token, "rifs"))
			mode |= SET_PROTECT(RIFS_PROTECT);
		else if (!strcmp(token, "rdg"))
			mode |= SET_PROTECT(RDG_PROTECT);
		else if (!strcmp(token, "force_rts"))
			mode |= SET_PROTECT(FORCE_RTS_PROTECT);
		else
			goto err3;
	}

	pAd->wdev_list[wdev_idx]->protection = mode;
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
			 (" <<< manual trigger >>>\n HWFLAG_ID_UPDATE_PROTECT\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF,
			 ("   -- wdev_%d->protection: 0x%08x\n",
			  wdev_idx, pAd->wdev_list[wdev_idx]->protection));
	HW_SET_PROTECT(pAd, wdev, PROT_PROTOCOL, 0, 0);
	goto end;
err3:
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			 (" -no mode [ERROR 3]\n"));
	goto err1;
err2:
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			 (" -no wdev_idx: 0x%x [ERROR 2]\n", wdev_idx));
err1:
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			 ("Usage:\niwpriv ra0 set protect=[wdev_idx]-[mode]+...\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			 ("       mode: [erp|no|non_member|ht20|non_ht_mixmode|longnav|gf|rifs|rdg|force_rts]\n"));
end:
	return TRUE;
}

INT set_cca_en(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	struct _RTMP_CHIP_DBG *chip_dbg = hc_get_chip_dbg(pAd->hdev_ctrl);

	if (chip_dbg->set_cca_en)
		return chip_dbg->set_cca_en(pAd->hdev_ctrl, arg);
	else
		return FALSE;
}

INT show_timer_list(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	RTMPShowTimerList(pAd);
	return TRUE;
}

INT show_wtbl_state(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	HcWtblRecDump(pAd);
	return TRUE;
}

/*
*
*/
UINT VIRTUAL_IF_INC(RTMP_ADAPTER *pAd)
{
	UINT cnt;
	ULONG flags = 0;

	OS_SPIN_LOCK_IRQSAVE(&pAd->VirtualIfLock, &flags);
	cnt = pAd->VirtualIfCnt++;
	OS_SPIN_UNLOCK_IRQRESTORE(&pAd->VirtualIfLock, &flags);
	return cnt;
}

/*
*
*/
UINT VIRTUAL_IF_DEC(RTMP_ADAPTER *pAd)
{
	UINT cnt;
	ULONG flags = 0;

	OS_SPIN_LOCK_IRQSAVE(&pAd->VirtualIfLock, &flags);
	cnt = pAd->VirtualIfCnt--;
	OS_SPIN_UNLOCK_IRQRESTORE(&pAd->VirtualIfLock, &flags);
	return cnt;
}

/*
*
*/
UINT VIRTUAL_IF_NUM(RTMP_ADAPTER *pAd)
{
	UINT cnt;

	cnt = pAd->VirtualIfCnt;
	return cnt;
}

INT Set_Rx_Vector_Control(
	IN RTMP_ADAPTER *pAd,
	IN RTMP_STRING *arg,
	IN RTMP_IOCTL_INPUT_STRUCT *wrq)
{
	BOOLEAN Enable = 1;
	UCHAR ucBandIdx = 0;
	UCHAR concurrent_bands = HcGetAmountOfBand(pAd);
	UINT8 i;
	/* obtain Band index */
#ifdef CONFIG_AP_SUPPORT
	POS_COOKIE  pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR       apidx = pObj->ioctl_if;
	struct wifi_dev *wdev;

	if (apidx >= pAd->ApCfg.BssidNum)
		return FALSE;

	wdev = &pAd->ApCfg.MBSSID[apidx].wdev;
	ucBandIdx = HcGetBandByWdev(wdev);
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s: BandIdx = %d\n", __func__, ucBandIdx));
#endif /* CONFIG_AP_SUPPORT */

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s----------------->\n", __func__));

	if (arg)
		Enable = os_str_tol(arg, 0, 10);

	Enable = (Enable == 0 ? 0 : 1);

	/* Turn off MibBucket */
	for (i = 0; i < concurrent_bands; i++)
		pAd->OneSecMibBucket.Enabled[i] = !Enable;
	pAd->MsMibBucket.Enabled = !Enable;

	/* Mac Enable*/
	AsicSetMacTxRx(pAd, ASIC_MAC_TXRX_RXV, Enable);

	/* Rxv Enable*/
	AsicSetRxvFilter(pAd, Enable, ucBandIdx);

	if (Enable)
		pAd->parse_rxv_stat_enable = 1;
	else
		pAd->parse_rxv_stat_enable = 0;

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s<-----------------\n", __func__));
	return TRUE;
}

static VOID Parse_Rx_Rssi_CR(PRTMP_ADAPTER pAd, struct _RX_STATISTIC_CR *RxStat, INT type, UINT32 value)
{
	UINT32 IBRssi0 = 0, IBRssi1 = 0, WBRssi0 = 0, WBRssi1 = 0;

	MTWF_LOG(DBG_CAT_TEST, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
			 ("%s: Value : %02x\n", __func__, value));

	if (IS_MT7615(pAd)) {
		IBRssi0 = (value & 0xFF000000) >> 24;
		if (IBRssi0 >= 128)
			IBRssi0 -= 256;
		WBRssi0 = (value & 0x00FF0000) >> 16;
		if (WBRssi0 >= 128)
			WBRssi0 -= 256;
		IBRssi1 = (value & 0x0000FF00) >> 8;
		if (IBRssi1 >= 128)
			IBRssi1 -= 256;
		WBRssi1 = (value & 0x000000FF);
		if (WBRssi1 >= 128)
			WBRssi1 -= 256;
	} else {
		IBRssi1 = (value & 0xFF000000) >> 24;
		if (IBRssi1 >= 128)
		    IBRssi1 -= 256;
		WBRssi1 = (value & 0x00FF0000) >> 16;
		if (WBRssi1 >= 128)
			WBRssi1 -= 256;
		IBRssi0 = (value & 0x0000FF00) >> 8;
		if (IBRssi0 >= 128)
			IBRssi0 -= 256;
		WBRssi0 = (value & 0x000000FF);
		if (WBRssi0 >= 128)
			WBRssi0 -= 256;
	}

	if (type == HQA_RX_STAT_RSSI || type == HQA_RX_STAT_RSSI_BAND1) {
		RxStat->Inst_IB_RSSSI[0] =  IBRssi0;
		RxStat->Inst_WB_RSSSI[0] =  WBRssi0;
		RxStat->Inst_IB_RSSSI[1] =  IBRssi1;
		RxStat->Inst_WB_RSSSI[1] =  WBRssi1;
	} else {
		RxStat->Inst_IB_RSSSI[2] =  IBRssi0;
		RxStat->Inst_WB_RSSSI[2] =  WBRssi0;
		RxStat->Inst_IB_RSSSI[3] =  IBRssi1;
		RxStat->Inst_WB_RSSSI[3] =  WBRssi1;
	}
}

INT Show_Rx_Statistic(
	IN RTMP_ADAPTER *pAd,
	IN RTMP_STRING *arg,
	IN RTMP_IOCTL_INPUT_STRUCT *wrq)
{
#define MSG_LEN 2048
#define ENABLE 1
#define DISABLE 0
#define BAND0 0
#define BAND1 1
	RX_STATISTIC_RXV *rx_stat_rxv = pAd->rx_stat_rxv + BAND0;
	RX_STATISTIC_CR rx_stat_cr;
	UINT32 value = 0, i = 0, set = 1;
	UINT32 Status;
	UINT32 CurrBand0FCSErr, CurrBand0MDRDY;
	static UINT32 PreBand0FCSErr, PreBand0MDRDY;
#ifdef DBDC_MODE
	UINT32 CurrBand1FCSErr, CurrBand1MDRDY;
	static UINT32 PreBand1FCSErr, PreBand1MDRDY;
#endif/*DBDC_MODE*/
	RTMP_STRING *msg;
	UCHAR ucBandIdx = 0;

#ifdef CONFIG_AP_SUPPORT
	POS_COOKIE  pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR       apidx = pObj->ioctl_if;
	struct wifi_dev *wdev;

	if (apidx >= pAd->ApCfg.BssidNum)
		return FALSE;

	wdev = &pAd->ApCfg.MBSSID[apidx].wdev;
	ucBandIdx = HcGetBandByWdev(wdev);
#endif /* CONFIG_AP_SUPPORT */
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s----------------->\n", __func__));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s: BandIdx = %d\n", __func__, ucBandIdx));

	if (arg)
		set = os_str_tol(arg, 0, 10);

	set = (set == 0 ? 0 : 1);
	os_alloc_mem(pAd, (UCHAR **)&msg, sizeof(CHAR)*MSG_LEN);
	memset(msg, 0x00, MSG_LEN);
	sprintf(msg, "\n");

	switch (set) {
	case RESET_COUNTER:
		sprintf(msg + strlen(msg), "Reset counter !!\n");
#ifdef CONFIG_HW_HAL_OFFLOAD
		/*Disable PHY Counter*/
		MtCmdSetPhyCounter(pAd, DISABLE, BAND0);
#ifdef DBDC_MODE

		if (pAd->CommonCfg.dbdc_mode == TRUE)
			MtCmdSetPhyCounter(pAd, DISABLE, BAND1);

#endif /*DBDC_MODE*/
#endif/*CONFIG_HW_HAL_OFFLOAD*/
		PreBand0FCSErr = 0;
		PreBand0MDRDY = 0;
		pAd->AccuOneSecRxBand0FcsErrCnt = 0;
		pAd->AccuOneSecRxBand0MdrdyCnt = 0;
		pAd->AccuOneSecRxBand1FcsErrCnt = 0;
		pAd->AccuOneSecRxBand1MdrdyCnt = 0;
		break;

	case SHOW_RX_STATISTIC:
#ifdef CONFIG_HW_HAL_OFFLOAD
		/*Enable PHY Counter*/
		MtCmdSetPhyCounter(pAd, ENABLE, BAND0);
#ifdef DBDC_MODE

		if (pAd->CommonCfg.dbdc_mode == TRUE)
			MtCmdSetPhyCounter(pAd, ENABLE, BAND1);

#endif /*DBDC_MODE*/
#endif/*CONFIG_HW_HAL_OFFLOAD*/

		/*Band0 PHY Counter */
		value = AsicGetRxStat(pAd, HQA_RX_STAT_PHY_FCSERRCNT);
		rx_stat_cr.FCSErr_OFDM = (value >> 16);
		rx_stat_cr.FCSErr_CCK = (value & 0xFFFF);
		value = AsicGetRxStat(pAd, HQA_RX_STAT_PD);
		rx_stat_cr.OFDM_PD = (value >> 16);
		rx_stat_cr.CCK_PD = (value & 0xFFFF);
		value = AsicGetRxStat(pAd, HQA_RX_STAT_CCK_SIG_SFD);
		rx_stat_cr.CCK_SIG_Err = (value >> 16);
		rx_stat_cr.CCK_SFD_Err = (value & 0xFFFF);
		value = AsicGetRxStat(pAd, HQA_RX_STAT_OFDM_SIG_TAG);
		rx_stat_cr.OFDM_SIG_Err = (value >> 16);
		rx_stat_cr.OFDM_TAG_Err = (value & 0xFFFF);

		{
			/*IBRSSI0 WBRSSI0 IBRSSI1 WBRSSI1*/
			value = AsicGetRxStat(pAd, HQA_RX_STAT_RSSI);
			Parse_Rx_Rssi_CR(pAd, &rx_stat_cr, HQA_RX_STAT_RSSI, value);

			/*IBRSSI2 WBRSSI2 IBRSSI3 WBRSSI3*/
			value = AsicGetRxStat(pAd, HQA_RX_STAT_RSSI_RX23);
			Parse_Rx_Rssi_CR(pAd, &rx_stat_cr, HQA_RX_STAT_RSSI_RX23, value);

			value = AsicGetRxStat(pAd, HQA_RX_STAT_ACI_HITL);
			rx_stat_cr.ACIHitLow = ((value >> 18) & 0x1);
			value = AsicGetRxStat(pAd, HQA_RX_STAT_ACI_HITH);
			rx_stat_cr.ACIHitHigh = ((value >> 18) & 0x1);
		}

		value = AsicGetRxStat(pAd, HQA_RX_STAT_PHY_MDRDYCNT);
		rx_stat_cr.PhyMdrdyOFDM = (value >> 16);
		if (IS_MT7622(pAd)) {
			/* HW issue and SW workaround */
			rx_stat_cr.PhyMdrdyCCK =  ((value & 0xFFFF)/2);
		} else
			rx_stat_cr.PhyMdrdyCCK =  (value & 0xFFFF);

		/*Band0 MAC Counter*/
		CurrBand0FCSErr = pAd->AccuOneSecRxBand0FcsErrCnt;
		rx_stat_cr.RxMacFCSErrCount = CurrBand0FCSErr - PreBand0FCSErr;
		PreBand0FCSErr = CurrBand0FCSErr;
		CurrBand0MDRDY = pAd->AccuOneSecRxBand0MdrdyCnt;
		rx_stat_cr.RxMacMdrdyCount = CurrBand0MDRDY - PreBand0MDRDY;
		PreBand0MDRDY = CurrBand0MDRDY;
		rx_stat_cr.RxMacFCSOKCount = rx_stat_cr.RxMacMdrdyCount - rx_stat_cr.RxMacFCSErrCount;

		sprintf(msg + strlen(msg), "\x1b[41m%s : \x1b[m\n", __func__);
		sprintf(msg + strlen(msg), "FreqOffsetFromRx   = %d\n", rx_stat_rxv->FreqOffsetFromRx[0]);

		for (i = 0; i < 4; i++)
			sprintf(msg + strlen(msg), "RCPI_%d             = %d\n", i, rx_stat_rxv->RCPI[i]);

		for (i = 0; i < 4; i++)
			sprintf(msg + strlen(msg), "FAGC_RSSI_IB_%d     = %d\n",  i, rx_stat_rxv->FAGC_RSSI_IB[i]);

		for (i = 0; i < 4; i++)
			sprintf(msg + strlen(msg), "FAGC_RSSI_WB_%d     = %d\n",  i, rx_stat_rxv->FAGC_RSSI_WB[i]);

		for (i = 0; i < 4; i++)
			sprintf(msg + strlen(msg), "Inst_IB_RSSI_%d     = %d\n",  i, rx_stat_cr.Inst_IB_RSSSI[i]);

		for (i = 0; i < 4; i++)
			sprintf(msg + strlen(msg), "Inst_WB_RSSI_%d     = %d\n",  i, rx_stat_cr.Inst_WB_RSSSI[i]);

		sprintf(msg + strlen(msg), "SNR                = %d\n",  rx_stat_rxv->SNR[0]);
		sprintf(msg + strlen(msg), "ACIHitHigh         = %u\n",  rx_stat_cr.ACIHitHigh);
		sprintf(msg + strlen(msg), "ACIHitLow          = %u\n",  rx_stat_cr.ACIHitLow);
		sprintf(msg + strlen(msg), "\x1b[41mFor Band0Index : \x1b[m\n");
		sprintf(msg + strlen(msg), "MacMdrdyCount      = %u\n",  rx_stat_cr.RxMacMdrdyCount);
		sprintf(msg + strlen(msg), "MacFCSErrCount     = %u\n",  rx_stat_cr.RxMacFCSErrCount);
		sprintf(msg + strlen(msg), "MacFCSOKCount      = %u\n",  rx_stat_cr.RxMacFCSOKCount);
		sprintf(msg + strlen(msg), "CCK_PD             = %u\n",  rx_stat_cr.CCK_PD);
		sprintf(msg + strlen(msg), "CCK_SFD_Err        = %u\n",  rx_stat_cr.CCK_SFD_Err);
		sprintf(msg + strlen(msg), "CCK_SIG_Err        = %u\n",  rx_stat_cr.CCK_SIG_Err);
		sprintf(msg + strlen(msg), "CCK_FCS_Err        = %u\n",  rx_stat_cr.FCSErr_CCK);
		sprintf(msg + strlen(msg), "OFDM_PD            = %u\n",  rx_stat_cr.OFDM_PD);
		sprintf(msg + strlen(msg), "OFDM_SIG_Err       = %u\n",  rx_stat_cr.OFDM_SIG_Err);
		sprintf(msg + strlen(msg), "OFDM_FCS_Err       = %u\n",  rx_stat_cr.FCSErr_OFDM);
#ifdef DBDC_MODE

		if (pAd->CommonCfg.dbdc_mode == TRUE) {
			/*Band1 MAC Counter*/
			CurrBand1FCSErr = pAd->AccuOneSecRxBand1FcsErrCnt;
			rx_stat_cr.RxMacFCSErrCount_band1 = CurrBand1FCSErr - PreBand1FCSErr;
			PreBand1FCSErr = CurrBand1FCSErr;
			CurrBand1MDRDY = pAd->AccuOneSecRxBand1MdrdyCnt;
			rx_stat_cr.RxMacMdrdyCount_band1 = CurrBand1MDRDY - PreBand1MDRDY;
			PreBand1MDRDY = CurrBand1MDRDY;
			rx_stat_cr.RxMacFCSOKCount_band1 = rx_stat_cr.RxMacMdrdyCount_band1 - rx_stat_cr.RxMacFCSErrCount_band1;
			/*Band1 PHY Counter*/
			value = AsicGetRxStat(pAd, HQA_RX_STAT_PHY_MDRDYCNT_BAND1);
			rx_stat_cr.PhyMdrdyOFDM_band1 = (value >> 16);
			rx_stat_cr.PhyMdrdyCCK_band1 = (value & 0xFFFF);
			value = AsicGetRxStat(pAd, HQA_RX_STAT_PD_BAND1);
			rx_stat_cr.OFDM_PD_band1 = (value >> 16);
			rx_stat_cr.CCK_PD_band1 = (value & 0xFFFF);
			value = AsicGetRxStat(pAd, HQA_RX_STAT_CCK_SIG_SFD_BAND1);
			rx_stat_cr.CCK_SIG_Err_band1 = (value >> 16);
			rx_stat_cr.CCK_SFD_Err_band1 = (value & 0xFFFF);
			value = AsicGetRxStat(pAd, HQA_RX_STAT_OFDM_SIG_TAG_BAND1);
			rx_stat_cr.OFDM_SIG_Err_band1 = (value >> 16);
			rx_stat_cr.OFDM_TAG_Err_band1 = (value & 0xFFFF);
			sprintf(msg + strlen(msg), "\x1b[41mFor Band1Index : \x1b[m\n");
			sprintf(msg + strlen(msg), "MacMdrdyCount      = %u\n",  rx_stat_cr.RxMacMdrdyCount_band1);
			sprintf(msg + strlen(msg), "MacFCSErrCount     = %u\n",  rx_stat_cr.RxMacFCSErrCount_band1);
			sprintf(msg + strlen(msg), "MacFCSOKCount      = %u\n",  rx_stat_cr.RxMacFCSOKCount_band1);
			sprintf(msg + strlen(msg), "CCK_PD             = %u\n",  rx_stat_cr.CCK_PD_band1);
			sprintf(msg + strlen(msg), "CCK_SFD_Err        = %u\n",  rx_stat_cr.CCK_SFD_Err_band1);
			sprintf(msg + strlen(msg), "CCK_SIG_Err        = %u\n",  rx_stat_cr.CCK_SIG_Err_band1);
			sprintf(msg + strlen(msg), "OFDM_PD            = %u\n",  rx_stat_cr.OFDM_PD_band1);
			sprintf(msg + strlen(msg), "OFDM_SIG_Err       = %u\n",  rx_stat_cr.OFDM_SIG_Err_band1);
		}

#endif/*DBDC_MODE*/
		break;
	}

	wrq->u.data.length = strlen(msg);
	Status = copy_to_user(wrq->u.data.pointer, msg, wrq->u.data.length);
	os_free_mem(msg);
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s<-----------------\n", __func__));
	return TRUE;
}

#ifdef SMART_CARRIER_SENSE_SUPPORT
INT Show_SCSinfo_proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	INT i = 0;
	UCHAR       concurrent_bands = HcGetAmountOfBand(pAd);

	if (IS_MT7615(pAd) || IS_MT7622(pAd) || IS_MT7663(pAd) || IS_MT7626(pAd)) {
		for (i = 0; i < concurrent_bands; i++) {
			MTWF_LOG(DBG_CAT_RX, DBG_SUBCAT_ALL, DBG_LVL_OFF,
					 ("************** Bnad%d  Information*************\n", i));
			MTWF_LOG(DBG_CAT_RX, DBG_SUBCAT_ALL, DBG_LVL_OFF,
					 (" Bnad%d SCSEnable = %d\n", i, pAd->SCSCtrl.SCSEnable[i]));
			MTWF_LOG(DBG_CAT_RX, DBG_SUBCAT_ALL, DBG_LVL_OFF,
					 (" Bnad%d SCSStatus = %d\n", i, pAd->SCSCtrl.SCSStatus[i]));
			MTWF_LOG(DBG_CAT_RX, DBG_SUBCAT_ALL, DBG_LVL_OFF,
					 (" Bnad%d SCSMinRssi = %d\n", i, pAd->SCSCtrl.SCSMinRssi[i]));
			MTWF_LOG(DBG_CAT_RX, DBG_SUBCAT_ALL, DBG_LVL_OFF,
					 (" Bnad%d CckPdBlkTh = %d (%ddBm)\n", i, pAd->SCSCtrl.CckPdBlkTh[i], (pAd->SCSCtrl.CckPdBlkTh[i] - 256)));
			MTWF_LOG(DBG_CAT_RX, DBG_SUBCAT_ALL, DBG_LVL_OFF,
					 (" Bnad%d OfdmPdBlkTh = %d(%ddBm)\n", i, pAd->SCSCtrl.OfdmPdBlkTh[i], (pAd->SCSCtrl.OfdmPdBlkTh[i] - 512) / 2));
			MTWF_LOG(DBG_CAT_RX, DBG_SUBCAT_ALL, DBG_LVL_OFF,
					 (" Bnad%d Traffic TH = %d\n", i, pAd->SCSCtrl.SCSTrafficThreshold[i]));
			MTWF_LOG(DBG_CAT_RX, DBG_SUBCAT_ALL, DBG_LVL_OFF,
					 (" Bnad%d MinRssiTolerance = %d\n", i, pAd->SCSCtrl.SCSMinRssiTolerance[i]));
			MTWF_LOG(DBG_CAT_RX, DBG_SUBCAT_ALL, DBG_LVL_OFF,
					 (" Bnad%d SCSThTolerance = %d\n", i, pAd->SCSCtrl.SCSThTolerance[i]));
			/* MTWF_LOG(DBG_CAT_RX, DBG_SUBCAT_ALL, DBG_LVL_OFF, */
			/* (" Bnad%d OFDM Support = %d\n", i, pAd->SCSCtrl.OfdmPdSupport[i])); */
			MTWF_LOG(DBG_CAT_RX, DBG_SUBCAT_ALL, DBG_LVL_OFF,
					 (" Bnad%d One sec TxByte = %d\n", i, pAd->SCSCtrl.OneSecTxByteCount[i]));
			MTWF_LOG(DBG_CAT_RX, DBG_SUBCAT_ALL, DBG_LVL_OFF,
					 (" Bnad%d One sec RxByte = %d\n", i, pAd->SCSCtrl.OneSecRxByteCount[i]));
			MTWF_LOG(DBG_CAT_RX, DBG_SUBCAT_ALL, DBG_LVL_OFF,
					 (" Bnad%d RTS count = %d\n", i, pAd->SCSCtrl.RtsCount[i]));
			MTWF_LOG(DBG_CAT_RX, DBG_SUBCAT_ALL, DBG_LVL_OFF,
					 (" Bnad%d RTS retry count = %d\n", i, pAd->SCSCtrl.RtsRtyCount[i]));

			/* SCS_Gen3 support RTS Drop Count at Band0 */
			if (pAd->SCSCtrl.SCSGeneration == SCS_Gen3 && i == 0) {
				MTWF_LOG(DBG_CAT_RX, DBG_SUBCAT_ALL, DBG_LVL_OFF,
						 (" Bnad0 RTS   MPDU drop count = %d\n", pAd->SCSCtrl.RTS_MPDU_DROP_CNT));
				MTWF_LOG(DBG_CAT_RX, DBG_SUBCAT_ALL, DBG_LVL_OFF,
						 (" Bnad0 Retry MPDU drop count = %d\n", pAd->SCSCtrl.Retry_MPDU_DROP_CNT));
				MTWF_LOG(DBG_CAT_RX, DBG_SUBCAT_ALL, DBG_LVL_OFF,
						 (" Bnad0 LTO   MPDU drop count = %d\n", pAd->SCSCtrl.LTO_MPDU_DROP_CNT));
			}

			MTWF_LOG(DBG_CAT_RX, DBG_SUBCAT_ALL, DBG_LVL_OFF,
					 ("=========CCK=============\n"));
			MTWF_LOG(DBG_CAT_RX, DBG_SUBCAT_ALL, DBG_LVL_OFF,
					 (" Bnad%d CCK false-CCA= %d\n", i, pAd->SCSCtrl.CckFalseCcaCount[i]));
			MTWF_LOG(DBG_CAT_RX, DBG_SUBCAT_ALL, DBG_LVL_OFF,
					 (" Bnad%d CCK false-CCA up bond= %d\n", i, pAd->SCSCtrl.CckFalseCcaUpBond[i]));
			MTWF_LOG(DBG_CAT_RX, DBG_SUBCAT_ALL, DBG_LVL_OFF,
					 (" Bnad%d CCK false-CCA low bond= %d\n", i, pAd->SCSCtrl.CckFalseCcaLowBond[i]));
			MTWF_LOG(DBG_CAT_RX, DBG_SUBCAT_ALL, DBG_LVL_OFF,
					 (" Bnad%d CCK fixed RSSI boundary= %d (%ddBm)\n", i, pAd->SCSCtrl.CckFixedRssiBond[i],
					  (pAd->SCSCtrl.CckFixedRssiBond[i] - 256)));
			MTWF_LOG(DBG_CAT_RX, DBG_SUBCAT_ALL, DBG_LVL_OFF,
					 ("=========OFDM=============\n"));
			MTWF_LOG(DBG_CAT_RX, DBG_SUBCAT_ALL, DBG_LVL_OFF,
					 (" Bnad%d OFDM false-CCA= %d\n", i, pAd->SCSCtrl.OfdmFalseCcaCount[i]));
			MTWF_LOG(DBG_CAT_RX, DBG_SUBCAT_ALL, DBG_LVL_OFF,
					 (" Bnad%d OFDM false-CCA up bond= %d\n", i, pAd->SCSCtrl.OfdmFalseCcaUpBond[i]));
			MTWF_LOG(DBG_CAT_RX, DBG_SUBCAT_ALL, DBG_LVL_OFF,
					 (" Bnad%d OFDM false-CCA low bond= %d\n", i, pAd->SCSCtrl.OfdmFalseCcaLowBond[i]));
			MTWF_LOG(DBG_CAT_RX, DBG_SUBCAT_ALL, DBG_LVL_OFF,
					 (" Bnad%d OFDM fixed RSSI boundary= %d(%ddBm)\n", i, pAd->SCSCtrl.OfdmFixedRssiBond[i],
					  (pAd->SCSCtrl.OfdmFixedRssiBond[i] - 512) / 2));
		}
	}

	return TRUE;
}
#endif /* SMART_CARRIER_SENSE_SUPPORT */

#ifdef LED_CONTROL_SUPPORT
INT	Set_Led_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	PCHAR thisChar;
	long led_param[8];
	INT i = 0, j = 0;

	printk("\n %s ==> arg = %s\n", __func__, arg);
	memset(led_param, 0, sizeof(long) * 8);

	while ((thisChar = strsep((char **)&arg, "-")) != NULL) {
		led_param[i] = os_str_tol(thisChar, 0, 10);
		i++;

		if (i >= 8)
			break;
	}

	if (IS_MT7615(pAd) || IS_MT7663(pAd) || IS_MT7626(pAd) || IS_MT7915(pAd)) {
		rtmp_control_led_cmd(pAd, led_param[0], led_param[1], led_param[2], led_param[3],
			led_param[4], led_param[5], led_param[6], led_param[7]);
	}

	return TRUE;
}
#endif

INT TpcManCtrl(
	IN PRTMP_ADAPTER pAd,
	IN BOOLEAN	fgTpcManual
)
{
	BOOLEAN  fgStatus = FALSE;

	if (MtCmdTpcManCtrl(pAd, fgTpcManual) == RETURN_STATUS_TRUE)
		fgStatus = TRUE;

	return fgStatus;
}

INT TpcEnableCfg(
	IN PRTMP_ADAPTER pAd,
	IN BOOLEAN	fgTpcEnable
)
{
	BOOLEAN  fgStatus = FALSE;

	if (MtCmdTpcEnableCfg(pAd, fgTpcEnable) == RETURN_STATUS_TRUE)
		fgStatus = TRUE;

	return fgStatus;
}

INT TpcWlanIdCtrl(
	IN PRTMP_ADAPTER pAd,
	IN BOOLEAN	fgUplink,
	IN UINT8	u1EntryIdx,
	IN UINT16	u2WlanId,
	IN UINT8    u1DlTxType
)
{
	BOOLEAN  fgStatus = FALSE;

	if (MtCmdTpcWlanIdCtrl(pAd, fgUplink, u1EntryIdx, u2WlanId, u1DlTxType) == RETURN_STATUS_TRUE)
		fgStatus = TRUE;

	return fgStatus;
}

INT TpcUlAlgoCtrl(
	IN PRTMP_ADAPTER pAd,
	IN UINT8	u1TpcCmd,
	IN UINT8	u1ApTxPwr,
	IN UINT8	u1EntryIdx,
	IN UINT8	u1TargetRssi,
	IN UINT8	u1UPH,
	IN BOOLEAN	fgMinPwrFlag
)
{
	BOOLEAN  fgStatus = FALSE;

	if (MtCmdTpcUlAlgoCtrl(pAd, u1TpcCmd, u1ApTxPwr, u1EntryIdx, u1TargetRssi, u1UPH, fgMinPwrFlag)
		== RETURN_STATUS_TRUE)
		fgStatus = TRUE;

	return fgStatus;
}


INT TpcUlUtVarCfg(
	PRTMP_ADAPTER pAd,
	UINT8 u1EntryIdx,
	UINT8 u1VarType,
	INT16 i2Value
)
{
	BOOLEAN  fgStatus = FALSE;

	if (MtCmdTpcUlUtVarCfg(pAd, u1EntryIdx, u1VarType, i2Value)
		== RETURN_STATUS_TRUE)
		fgStatus = TRUE;

	return fgStatus;
}

INT TpcAlgoUtGo(
	IN PRTMP_ADAPTER pAd,
	IN BOOLEAN fgTpcUtGo
)
{
	BOOLEAN  fgStatus = FALSE;

	if (MtCmdTpcUlUtGo(pAd, fgTpcUtGo) == RETURN_STATUS_TRUE)
		fgStatus = TRUE;

	return fgStatus;
}


INT TpcDlAlgoCtrl(
	IN PRTMP_ADAPTER pAd,
	IN UINT8	u1TpcCmd,
	IN BOOLEAN	fgCmdCtrl,
	IN UINT8	u1DlTxType,
	IN CHAR		DlTxPwr,
	IN UINT8	u1EntryIdx,
	IN INT16	DlTxpwrAlpha
)
{
	BOOLEAN  fgStatus = FALSE;

	if (MtCmdTpcDlAlgoCtrl(pAd, u1TpcCmd, fgCmdCtrl, u1DlTxType, DlTxPwr, u1EntryIdx, DlTxpwrAlpha)
		== RETURN_STATUS_TRUE)
		fgStatus = TRUE;

	return fgStatus;
}

INT TpcManTblInfo(
	IN PRTMP_ADAPTER pAd,
	IN BOOLEAN	fgUplink
)
{
	BOOLEAN  fgStatus = FALSE;

	if (MtCmdTpcManTblInfo(pAd, fgUplink) == RETURN_STATUS_TRUE)
		fgStatus = TRUE;

	return fgStatus;
}

INT support_rate_table_ctrl(
	IN PRTMP_ADAPTER pAd,
	IN UINT8 tx_mode,
	IN UINT8 tx_nss,
	IN UINT8 tx_bw,
	IN UINT16 *mcs_cap
)
{
	BOOLEAN status = FALSE;

	if (mt_cmd_support_rate_table_ctrl(pAd, tx_mode, tx_nss, tx_bw, mcs_cap, TRUE) == RETURN_STATUS_TRUE)
		status = TRUE;

	return status;
}

INT support_rate_table_info(
	IN PRTMP_ADAPTER pAd,
	IN UINT8 tx_mode,
	IN UINT8 tx_nss,
	IN UINT8 tx_bw,
	IN UINT16 *mcs_cap
)
{
	BOOLEAN status = FALSE;

	if (mt_cmd_support_rate_table_ctrl(pAd, tx_mode, tx_nss, tx_bw, mcs_cap, FALSE) == RETURN_STATUS_TRUE)
		status = TRUE;

	return status;
}

INT ra_dbg_ctrl(
	IN PRTMP_ADAPTER pAd,
	IN UINT8 param_num,
	IN UINT32 *param
)
{
	BOOLEAN status = FALSE;

	if (mt_cmd_ra_dbg_ctrl(pAd, param_num, param) == RETURN_STATUS_TRUE)
		status = TRUE;

	return status;
}

#ifdef SINGLE_SKU_V2
INT TxPowerSKUCtrl(
	IN PRTMP_ADAPTER pAd,
	IN BOOLEAN tx_pwr_sku_en,
	IN UCHAR ucBandIdx
)
{
	BOOLEAN  fgStatus = FALSE;

	if (MtCmdTxPowerSKUCtrl(pAd, tx_pwr_sku_en, ucBandIdx) == RETURN_STATUS_TRUE)
		fgStatus = TRUE;

	return fgStatus;
}

INT TxPowerBfBackoffCtrl(
	IN PRTMP_ADAPTER pAd,
	IN BOOLEAN fgTxBFBackoffEn,
	IN UCHAR ucBandIdx
)
{
	BOOLEAN  fgStatus = FALSE;

	if (MtCmdTxBfBackoffCtrl(pAd, fgTxBFBackoffEn, ucBandIdx) == RETURN_STATUS_TRUE)
		fgStatus = TRUE;

	return fgStatus;
}
#if defined(MT7615) || defined(MT7622)
INT TxPowerBfBackoffParaCtrl(
	IN PRTMP_ADAPTER pAd,
	IN UCHAR ChannelBand,
	IN UCHAR ControlChannel,
	IN UCHAR ucBandIdx
)
{
	UINT8   ucBFBackoffTblIdx;
	UINT8   ucBFBackoffCtrlIdx;
	BOOLEAN fg5Gband;
	CHAR	cBfBoundTable[3];
	CHAR	acTxPwrFccBfOnCase[10];
	CHAR	acTxPwrFccBfOffCase[10];
	CHAR	cBFBackOffset[3][BF_BACKOFF_CONTROL_REGISTER_SIZE] = {
																	{  6,  0,  6, -4,  2,  6, -6,  0,  3,  6},	/* BF_BACKOFF_2T_MODE */
																	{  9,  3,  9,  0,  6,  9, -3,  3,  7,  9},	/* BF_BACKOFF_3T_MODE */
																	{ 12,  6, 12,  2,  8, 12,  0,  6,  9, 12}	/* BF_BACKOFF_4T_MODE */
																 };

	if (ChannelBand == 0) { /* Not 802.11j */
		if (ControlChannel <= 14)
			fg5Gband = 0;
		else
			fg5Gband = 1;
	} else
		fg5Gband = 1;

	/* obtain BF Backoff Table */
	MtFillBackoffParam(pAd, ControlChannel, fg5Gband, cBfBoundTable);

	for (ucBFBackoffTblIdx = 0; ucBFBackoffTblIdx < 3; ucBFBackoffTblIdx++)
		MTWF_LOG(DBG_CAT_FW, DBG_SUBCAT_ALL, DBG_LVL_INFO, ("%s: BfBoundTable[%d]: 0x%x\n", __func__, ucBFBackoffTblIdx, cBfBoundTable[ucBFBackoffTblIdx]));

#ifdef DBDC_MODE
	if (pAd->CommonCfg.dbdc_mode)
		pAd->ucBFBackOffMode = BF_BACKOFF_2T_MODE;
	else
		pAd->ucBFBackOffMode = pAd->Antenna.field.TxPath;
#else
	pAd->ucBFBackOffMode = pAd->Antenna.field.TxPath;
#endif /* DBDC_MODE */

	/* Fill BF Backoff ON Table with default value 0x3F */
	os_fill_mem(acTxPwrFccBfOnCase, BF_BACKOFF_CONTROL_REGISTER_SIZE, 0x3F);

	/* Fill BF Backoff OFF Table with default value 0x3F */
	os_fill_mem(acTxPwrFccBfOffCase, BF_BACKOFF_CONTROL_REGISTER_SIZE, 0x3F);

	/* Update BF Backoff value (BF Backoff ON Table) for different Tx Stream case */
	if (pAd->ucBFBackOffMode != BF_BACKOFF_1T_MODE) {
		for (ucBFBackoffCtrlIdx = 0; ucBFBackoffCtrlIdx < BF_BACKOFF_CONTROL_REGISTER_SIZE; ucBFBackoffCtrlIdx++)
			acTxPwrFccBfOnCase[ucBFBackoffCtrlIdx] = cBfBoundTable[BF_BACKOFF_4T_MODE - pAd->ucBFBackOffMode] + cBFBackOffset[pAd->ucBFBackOffMode - 1][ucBFBackoffCtrlIdx];
	}

	for (ucBFBackoffCtrlIdx = 0; ucBFBackoffCtrlIdx < BF_BACKOFF_CONTROL_REGISTER_SIZE; ucBFBackoffCtrlIdx++)
		MTWF_LOG(DBG_CAT_FW, DBG_SUBCAT_ALL, DBG_LVL_INFO, ("acTxPwrFccBfOnCase[%d]: 0x%x\n", ucBFBackoffCtrlIdx, acTxPwrFccBfOnCase[ucBFBackoffCtrlIdx]));

	CmdTxBfTxPwrBackOff(pAd, ucBandIdx, acTxPwrFccBfOnCase, acTxPwrFccBfOffCase);
	return TRUE;
}
#else
#endif /* defined(MT7615) || defined(MT7622) */
#endif /* SINGLE_SKU_V2 */

INT TxPowerManualCtrl(
	IN PRTMP_ADAPTER	pAd,
	IN UCHAR		ucBandIdx,
	IN INT_8		cTxPower,
	IN UINT8		ucPhyMode,
	IN UINT8		ucTxRate,
	IN UINT8		ucBW
)
{

	BOOLEAN  fgStatus = FALSE;
#ifdef CONFIG_ATE
	if (MtCmdSetForceTxPowerCtrl(pAd, ucBandIdx, cTxPower, ucPhyMode, ucTxRate, ucBW) == RETURN_STATUS_TRUE)
		fgStatus = TRUE;
#endif
	return fgStatus;
}

INT TxPowerPercentCtrl(
	IN PRTMP_ADAPTER pAd,
	IN BOOLEAN fgTxPowerPercentEn,
	IN UCHAR ucBandIdx
)
{
	BOOLEAN  fgStatus = FALSE;
#ifdef CONFIG_ATE
	if (MtCmdTxPowerPercentCtrl(pAd, fgTxPowerPercentEn, ucBandIdx) == RETURN_STATUS_TRUE)
		fgStatus = TRUE;
#endif
	return fgStatus;
}

INT TxPowerDropCtrl(
	IN PRTMP_ADAPTER pAd,
	IN UINT8 ucPowerDrop,
	IN UCHAR ucBandIdx
)
{
	BOOLEAN  fgStatus = FALSE;
	INT8  cPowerDropLevel = 0;

	/* config Tx Power Drop value */
	if ((ucPowerDrop > 90) && (ucPowerDrop < 100))
		cPowerDropLevel = 0;
	else if ((ucPowerDrop > 60) && (ucPowerDrop <= 90))  /* reduce Pwr for 1 dB. */
		cPowerDropLevel = 2;
	else if ((ucPowerDrop > 30) && (ucPowerDrop <= 60))  /* reduce Pwr for 3 dB. */
		cPowerDropLevel = 6;
	else if ((ucPowerDrop > 15) && (ucPowerDrop <= 30))  /* reduce Pwr for 6 dB. */
		cPowerDropLevel = 12;
	else if ((ucPowerDrop > 9) && (ucPowerDrop <= 15))   /* reduce Pwr for 9 dB. */
		cPowerDropLevel = 18;
	else if ((ucPowerDrop > 0) && (ucPowerDrop <= 9))   /* reduce Pwr for 12 dB. */
		cPowerDropLevel = 24;

	if (MtCmdTxPowerDropCtrl(pAd, cPowerDropLevel, ucBandIdx) == RETURN_STATUS_TRUE)
		fgStatus = TRUE;

	return fgStatus;
}

INT TxCCKStreamCtrl(
	IN PRTMP_ADAPTER pAd,
	IN UINT8 u1CCKTxStream,
	IN UCHAR ucBandIdx
)
{
	BOOLEAN  fgStatus = FALSE;

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("u1CCKTxStream = %d \n", u1CCKTxStream));

	/* Work around - profile setting*/
	if (!u1CCKTxStream)
		u1CCKTxStream = 1;

	/* sanity check for input parameter range */
	if (u1CCKTxStream >= WF_NUM) {
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s: set wrong parameters\n", __func__));
		return FALSE;
	}

	/* Only for 7622 */
	if (IS_MT7622(pAd)) {
		if (MtCmdTxCCKStream(pAd, u1CCKTxStream, ucBandIdx) == RETURN_STATUS_TRUE)
			fgStatus = TRUE;
	}

	return fgStatus;
}

INT ThermoCompCtrl(
	IN PRTMP_ADAPTER pAd,
	IN BOOLEAN fgThermoCompEn,
	IN UCHAR ucBandIdx
)
{
	BOOLEAN  fgStatus = FALSE;

	if (MtCmdThermoCompCtrl(pAd, fgThermoCompEn, ucBandIdx) == RETURN_STATUS_TRUE)
		fgStatus = TRUE;

	return fgStatus;
}

INT TxPowerRfTxAnt(
	IN PRTMP_ADAPTER pAd,
	IN UINT8 ucTxAntIdx
)
{
	BOOLEAN  fgStatus = FALSE;

	if (MtCmdTxPwrRfTxAntCtrl(pAd, ucTxAntIdx) == RETURN_STATUS_TRUE)
		fgStatus = TRUE;

	return fgStatus;
}

INT TxPowerShowInfo(
	IN PRTMP_ADAPTER	pAd,
	IN UCHAR            ucTxPowerInfoCatg,
	IN UINT8            ucBandIdx
)
{
	BOOLEAN  fgStatus = FALSE;

	if (MtCmdTxPwrShowInfo(pAd, ucTxPowerInfoCatg, ucBandIdx) == RETURN_STATUS_TRUE)
		fgStatus = TRUE;

	return fgStatus;
}


NDIS_STATUS edcca_thres_set(
	IN PRTMP_ADAPTER	pAd,
	IN struct wifi_dev *wdev
)
{
	UCHAR band_idx = 0;
	EXT_CMD_ED_CCA_CTRL cmd_set_edcca;
	os_zero_mem(&cmd_set_edcca, sizeof(EXT_CMD_ED_CCA_CTRL));

	if (wdev != NULL)
		band_idx = HcGetBandByWdev(wdev);
	else
		return NDIS_STATUS_FAILURE;

	if (pAd->CommonCfg.edcca_mode[band_idx] == FALSE) {
		MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
			("%s(): BandIdx = %d, cfg_mode = %d\n",
			__func__, band_idx, pAd->CommonCfg.edcca_mode[band_idx]));
		return NDIS_STATUS_SUCCESS;
	}

	cmd_set_edcca.u1CmdIdx = EDCCA_CTRL_THRES;
	cmd_set_edcca.u1BandIdx = band_idx;
	/* ED threshold: -49 dBm */
	cmd_set_edcca.u1SetVal = 0xcf;

	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
		("%s(): BandIdx = %d, set ed_cca threshold\n",
		__func__, band_idx));
	mt_cmd_set_ed_cca(pAd, &cmd_set_edcca);

	return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS edcca_init(
	IN PRTMP_ADAPTER	pAd,
	IN struct wifi_dev *wdev
)
{
	/* edcca threshold */
	edcca_thres_set(pAd, wdev);

	return NDIS_STATUS_SUCCESS;
}

#ifdef WIFI_GPIO_CTRL
INT SetGpioCtrl(PRTMP_ADAPTER pAd, UINT8 GpioIdx, BOOLEAN GpioEn)
{
	INT status = 0;

	if (MtCmdSetGpioCtrl(pAd, GpioIdx, GpioEn) == RETURN_STATUS_TRUE)
		status = 1;

	return status;
}

INT SetGpioValue(PRTMP_ADAPTER pAd, UINT8 GpioIdx, UINT8 GpioVal)
{
	INT status = 0;

	if (MtCmdSetGpioVal(pAd, GpioIdx, GpioVal) == RETURN_STATUS_TRUE)
		status = 1;

	return status;
}
#endif /* WIFI_GPIO_CTRL */

INT TOAECtrlCmd(
	IN PRTMP_ADAPTER	pAd,
	IN UCHAR                TOAECtrl
)
{
	BOOLEAN  fgStatus = FALSE;

	if (MtCmdTOAECalCtrl(pAd, TOAECtrl) == RETURN_STATUS_TRUE)
		fgStatus = TRUE;

	return fgStatus;
}

INT MuPwrCtrlCmd(
	IN PRTMP_ADAPTER pAd,
	IN BOOLEAN fgMuTxPwrManEn,
	IN CHAR cMuTxPwr,
	IN UINT8 u1BandIdx
)
{
	BOOLEAN  fgStatus = FALSE;

	if (MtCmdMuPwrCtrl(pAd, fgMuTxPwrManEn, cMuTxPwr, u1BandIdx) == RETURN_STATUS_TRUE)
		fgStatus = TRUE;

	return fgStatus;
}

INT BFNDPATxDCtrlCmd(
	IN PRTMP_ADAPTER        pAd,
	IN BOOLEAN              fgNDPA_ManualMode,
	IN UINT8                ucNDPA_TxMode,
	IN UINT8                ucNDPA_Rate,
	IN UINT8                ucNDPA_BW,
	IN UINT8                ucNDPA_PowerOffset
)
{
	BOOLEAN  fgStatus = FALSE;

	if (MtCmdBFNDPATxDCtrl(pAd, fgNDPA_ManualMode, ucNDPA_TxMode, ucNDPA_Rate, ucNDPA_BW,
						   ucNDPA_PowerOffset) == RETURN_STATUS_TRUE)
		fgStatus = TRUE;

	return fgStatus;
}

INT RxvEnCtrl(
	IN PRTMP_ADAPTER pAd,
	IN BOOLEAN fgRxvEnCtrl
)
{
	BOOLEAN fgStatus = FALSE;

	if (mt_cmd_set_rxv_ctrl(pAd, fgRxvEnCtrl) == RETURN_STATUS_TRUE)
		fgStatus = TRUE;

	return fgStatus;
}

INT RxvRuCtrl(
	IN PRTMP_ADAPTER pAd,
	IN UINT8 u1RxvRuCtrl
)
{
	BOOLEAN fgStatus = FALSE;

	if (mt_cmd_set_rxv_ru_ctrl(pAd, u1RxvRuCtrl) == RETURN_STATUS_TRUE)
		fgStatus = TRUE;

	return fgStatus;
}

INT ThermalManCtrl(
	IN PRTMP_ADAPTER pAd,
	IN UINT8 u1BandIdx,
	IN BOOLEAN fgManualMode,
	IN UINT8 u1ThermalAdc
)
{
	INT i4Status = FALSE;

	if (NDIS_STATUS_SUCCESS != MtCmdThermalManCtrl(pAd, u1BandIdx, fgManualMode, u1ThermalAdc))
		i4Status = TRUE;

	return i4Status;
}

INT ThermalTaskCtrl(
	IN PRTMP_ADAPTER pAd,
	IN UINT8 u1BandIdx,
	IN BOOLEAN fgTrigEn,
	IN UINT8 u1Thres
)
{
#if defined(MT7615) || defined(MT7622)
    return TRUE;
#else
	INT i4Status = FALSE;
	UINT32 u4FuncPtr = 0;

	/* update function pointer */
	u4FuncPtr = *(UINT32 *)ThermalTaskAction;

	if (NDIS_STATUS_SUCCESS != MtCmdThermalTaskCtrl(pAd, u1BandIdx, fgTrigEn, u1Thres, u4FuncPtr))
		i4Status = TRUE;

	return i4Status;
#endif /* defined(MT7615) || defined(MT7622) */
}

INT ThermalTaskAction(
	IN PRTMP_ADAPTER pAd,
	IN UINT8 u4PhyIdx,
	IN UINT32 u4ThermalTaskProp,
	IN UINT8 u1ThermalAdc
)
{
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s(): u4PhyIdx: %d, u4ThermalTaskProp: %d, u1ThermalAdc: %d\n", __func__, u4PhyIdx, u4ThermalTaskProp, u1ThermalAdc));

	return TRUE;
}

#if defined(MT7615) || defined(MT7622)
#else
INT ThermalBasicInfo(
	IN PRTMP_ADAPTER pAd,
	IN UINT8 u1BandIdx
)
{
	INT8 i4Status = FALSE;

	if (NDIS_STATUS_SUCCESS != MtCmdThermalBasicInfo(pAd, u1BandIdx))
		i4Status = TRUE;

	return i4Status;
}
#endif /* defined(MT7615) || defined(MT7622) */

#ifdef TX_POWER_CONTROL_SUPPORT
INT TxPwrUpCtrl(
	IN PRTMP_ADAPTER   pAd,
	IN UINT8           ucBandIdx,
	IN CHAR            cPwrUpCat,
	IN CHAR            cPwrUpValue[POWER_UP_CATEGORY_RATE_NUM]
)
{
	BOOLEAN  fgStatus = FALSE;

	if (MtCmdTxPwrUpCtrl(pAd, ucBandIdx, cPwrUpCat, cPwrUpValue) ==
			RETURN_STATUS_TRUE)
		fgStatus = TRUE;

	return fgStatus;
}
#endif /* TX_POWER_CONTROL_SUPPORT */

/* [channel_band] 0: 2.4G, 1: 5G*/
UINT8 TxPowerGetChBand(
	IN UINT8				ucBandIdx,
	IN UINT8				CentralCh
)
{
	UINT8	ChannelBand = 0;

	if (CentralCh >= 14)
		ChannelBand = 1;
	else {
		if (ucBandIdx == 0)
			ChannelBand = 0;
		else
			ChannelBand = 1;
	}

	return ChannelBand;
}

#ifdef TPC_SUPPORT
INT TxPowerTpcFeatureCtrl(
	IN PRTMP_ADAPTER	pAd,
	IN struct wifi_dev		*wdev,
	IN INT8					TpcPowerValue
)
{
	BOOLEAN  fgStatus = FALSE;
	return fgStatus;
}

INT TxPowerTpcFeatureForceCtrl(
	IN PRTMP_ADAPTER	pAd,
	IN INT8					TpcPowerValue,
	IN UINT8				ucBandIdx,
	IN UINT8				CentralChannel
)
{
	BOOLEAN  fgStatus = FALSE;
	return fgStatus;
}
#endif /* TPC_SUPPORT */

VOID cp_support_is_enabled(PRTMP_ADAPTER pAd)
{
	if ((pAd->cp_support >= 1) && (pAd->cp_support <= 3)) {
		if (pAd->cp_have_cr4 == TRUE) {
			MtCmdSetCPSEnable(pAd, HOST2CR4, pAd->cp_support);
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
					 ("%s: set CR4 CP_SUPPORT to Mode %d.\n", __func__, pAd->cp_support));
		} else {
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
					 ("%s: set Driver CP_SUPPORT to Mode %d.\n", __func__, pAd->cp_support));
		}
	} else {
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
				 ("The CP Mode is invaild. Mode should be 1~3.\n"));
	}
}

INT set_cp_support_en(
	IN  PRTMP_ADAPTER pAd,
	IN  RTMP_STRING *arg)
{
	UINT32 rv, Mode;

	if (arg) {
		rv = sscanf(arg, "%d", &Mode);

		if ((rv > 0) && (Mode >= 1) && (Mode <= 3)) {
			pAd->cp_support = Mode;

			if (pAd->cp_have_cr4 == TRUE) {
				MtCmdSetCPSEnable(pAd, HOST2CR4, Mode);
				MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
						 ("%s: set CR4 CP_SUPPORT to Mode %d.\n", __func__, Mode));
			} else {
				MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
						 ("%s: set Driver CP_SUPPORT to Mode %d.\n", __func__, Mode));
			}
		} else {
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
					 ("The Mode is invaild. Mode should be 1~3.\n"));
			return FALSE;
		}
	} else
		return FALSE;

	return TRUE;
}

INT Show_MibBucket_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
#if defined(MT7615) || defined(MT7622) || defined(MT7663) || defined(MT7626) || defined(MT7915)
	INT i = 0;
	UCHAR concurrent_bands = HcGetAmountOfBand(pAd);

	if (IS_MT7615(pAd) || IS_MT7622(pAd) || IS_MT7663(pAd) || IS_MT7626(pAd) || IS_MT7915(pAd)) {
		for (i = 0; i < concurrent_bands; i++) {
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
					 ("====Bnad%d Enable = %d====\n", i, pAd->OneSecMibBucket.Enabled[i]));
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
					 ("Channel Busy Time = %d\n", pAd->OneSecMibBucket.ChannelBusyTimeCcaNavTx[i]));
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
					 ("Primary Channel Busy Time = %d\n", pAd->OneSecMibBucket.ChannelBusyTime[i]));
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
					 ("OBSS Air Time = %d\n", pAd->OneSecMibBucket.OBSSAirtime[i]));
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
					 ("My Tx Air Time = %d\n", pAd->OneSecMibBucket.MyTxAirtime[i]));
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
					 ("My Rx Air Time = %d\n", pAd->OneSecMibBucket.MyRxAirtime[i]));
			MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
					 ("EDCCA Time = %d\n", pAd->OneSecMibBucket.EDCCAtime[i]));
			if (!IS_MT7915(pAd)) {
				MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
						 ("PD count = %x\n", pAd->OneSecMibBucket.PdCount[i]));
				MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
						 ("MDRDY Count = %x\n", pAd->OneSecMibBucket.MdrdyCount[i]));
			}
		}
	}

#endif
	return TRUE;
}

#ifdef GN_MIXMODE_SUPPORT
VOID gn_mixmode_is_enable(PRTMP_ADAPTER pAd)
{
	if (pAd->CommonCfg.GNMixMode) {
		MtCmdSetGNMixModeEnable(pAd, HOST2CR4, pAd->CommonCfg.GNMixMode);
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
		("%s: set CR4/N9 GN MIXMODE Enable to %d.\n", __func__, pAd->CommonCfg.GNMixMode));
	}
}
#endif /* GN_MIXMODE_SUPPORT */

#ifdef DHCP_UC_SUPPORT
static UINT32  checksum(PUCHAR buf, INT32  nbytes, UINT32 sum)
{
	uint i;

	/* Checksum all the pairs of bytes first... */
	for (i = 0; i < (nbytes & ~1U); i += 2) {
		sum += (UINT16)ntohs(*((u_int16_t *)(buf + i)));

		if (sum > 0xFFFF)
			sum -= 0xFFFF;
	}

	/*
	 * If there's a single byte left over, checksum it, too.
	 * Network byte order is big-endian, so the remaining byte is
	 * the high byte.
	 */
	if (i < nbytes) {
		sum += buf[i] << 8;

		if (sum > 0xFFFF)
			sum -= 0xFFFF;
	}

	return sum;
}

static UINT32 wrapsum(UINT32 sum)
{
	UINT32 ret;

	sum = ~sum & 0xFFFF;
	ret = htons(sum);
	return ret;
}
UINT16 RTMP_UDP_Checksum(IN PNDIS_PACKET pSkb)
{
	PUCHAR pPktHdr, pLayerHdr;
	PUCHAR pPseudo_Hdr;
	PUCHAR pPayload_Hdr;
	PUCHAR pUdpHdr;
	UINT16 udp_chksum;
	UINT16 udp_len;
	UINT16 payload_len;

	pPktHdr = GET_OS_PKT_DATAPTR(pSkb);

	if (IS_VLAN_PACKET(GET_OS_PKT_DATAPTR(pSkb)))
		pLayerHdr = (pPktHdr + MAT_VLAN_ETH_HDR_LEN);
	else
		pLayerHdr = (pPktHdr + MAT_ETHER_HDR_LEN);

	pUdpHdr = pLayerHdr + 20;
	pPseudo_Hdr = pUdpHdr - 8;
	pPayload_Hdr = pUdpHdr + 8;
	udp_chksum = (*((UINT16 *) (pUdpHdr + 6)));
	udp_len = ntohs(*((UINT16 *) (pUdpHdr + 4)));
	payload_len = udp_len - 8;
	udp_chksum = wrapsum(
					 checksum(
						 pUdpHdr,
						 8,
						 checksum(
							 pPayload_Hdr,
							 payload_len,
							 checksum(
								 (unsigned char *)pPseudo_Hdr,
								 2 * 4,
								 17 + udp_len)))
				 );
	return udp_chksum;
}
#endif /* DHCP_UC_SUPPORT */

#ifdef ERR_RECOVERY
INT32 ShowSerProc(struct _RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s,::E R , stage=%d\n",
			 __func__, ErrRecoveryCurStage(&pAd->ErrRecoveryCtl)));
	/* Dump SER related CRs */
	chip_dump_ser_stat(pAd, TRUE);
	/* print out ser log timing */
	SerTimeLogDump(pAd);
	return TRUE;
}

INT32 ShowSerProc2(struct _RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{

	ShowSerProc(pAd, arg);

#ifdef DUMMY_N9_HEART_BEAT
	if (1) {
		UINT32 reg_tmp_val;

		MAC_IO_READ32(pAd->hdev_ctrl, DUMMY_N9_HEART_BEAT, &reg_tmp_val);

		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("HeartBeat 0x%x = %d\n",
			DUMMY_N9_HEART_BEAT, reg_tmp_val));
	}
#endif

	/* We will get more info from FW */
	CmdExtSER(pAd, SER_ACTION_QUERY, 0, 0);

	return TRUE;
}

#endif


INT32 ShowBcnProc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
#ifdef CONFIG_AP_SUPPORT
	UINT32 band_idx = 0;
	struct _RTMP_CHIP_DBG *chip_dbg = hc_get_chip_dbg(pAd->hdev_ctrl);

	for (band_idx = 0; band_idx < DBDC_BAND_NUM; band_idx++) {
		if (arg != NULL && band_idx != os_str_toul(arg, 0, 10))
			continue;

		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s, Band %d\n", __func__, band_idx));
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("===============================\n"));
		if (chip_dbg->show_bcn_info)
			chip_dbg->show_bcn_info(pAd->hdev_ctrl, band_idx);
	}

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("===============================\n"));
#endif
	return TRUE;
}

#if (defined(CFG_SUPPORT_FALCON_MURU) || defined(CFG_SUPPORT_MU_MIMO))
INT ShowMuMimoGroupTblEntry(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	struct _RTMP_CHIP_OP *ops = hc_get_chip_ops(pAd->hdev_ctrl);

	if (ops->show_mumimo_group_entry_tbl)
		ops->show_mumimo_group_entry_tbl(pAd, arg);
	else
		return FALSE;

	return TRUE;
}
#endif /*CFG_SUPPORT_FALCON_MURU || CFG_SUPPORT_MU_MIMO*/

#if (defined(CFG_SUPPORT_MU_MIMO_RA) || defined(CFG_SUPPORT_FALCON_MURU))
INT ShowMuMimoAlgorithmMonitor(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	struct _RTMP_CHIP_OP *ops = hc_get_chip_ops(pAd->hdev_ctrl);

	if (ops->show_mumimo_algorithm_monitor)
		ops->show_mumimo_algorithm_monitor(pAd, arg);
	else
		return FALSE;

	return TRUE;
}

INT32 SetMuMimoFixedRate(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	struct _RTMP_CHIP_OP *ops = hc_get_chip_ops(pAd->hdev_ctrl);

	if (ops->set_mumimo_fixed_rate)
		ops->set_mumimo_fixed_rate(pAd, arg);
	else
		return FALSE;

	return TRUE;
}

INT32 SetMuMiMoFixedGroupRateProc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	struct _RTMP_CHIP_OP *ops = hc_get_chip_ops(pAd->hdev_ctrl);

	if (ops->set_mumimo_fixed_group_rate)
		ops->set_mumimo_fixed_group_rate(pAd, arg);
	else
		return FALSE;

	return TRUE;
}

INT SetMuMimoForceMu(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	INT status = 0;
	BOOLEAN fgForceMU;

	if (arg != NULL)
		fgForceMU = os_str_tol(arg, 0, 10);
	else
		goto error;

	if (fgForceMU)
		fgForceMU = MUMIMO_FORCE_ENABLE_MU;

	if (SetMuMimoForceMUEnable(pAd, fgForceMU) == TRUE)
		status = 1;

error:
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_INFO,
			("%s:(status = %d\n", __func__, status));
	return status;
}

INT32 SetMuMimoForceMUEnable(RTMP_ADAPTER *pAd, BOOLEAN fgForceMu)
{
	struct _RTMP_CHIP_OP *ops = hc_get_chip_ops(pAd->hdev_ctrl);

	if (ops->set_mumimo_force_mu_enable)
		ops->set_mumimo_force_mu_enable(pAd, fgForceMu);
	else
		return FALSE;

	return TRUE;
}

#endif /*CFG_SUPPORT_MU_MIMO_RA || CFG_SUPPORT_FALCON_MURU*/

#ifdef TX_POWER_CONTROL_SUPPORT
INT32 SetTxPowerBoostInfo(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UINT_8 u1BandIdx = 0;
	CHAR	*value = 0;
	UINT8	u1Bw, u1PhyMode, u1PwrUpCat = 0;
	struct _RTMP_ARCH_OP *arch_ops = hc_get_arch_ops(pAd->hdev_ctrl);

	/* sanity check for input parameter format */
	if (!arg) {
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			 ("%s: No parameters!!\n", __func__));
		return FALSE;
	}

	value = strsep(&arg, ":");
	if (value == NULL) {
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s: set wrong parameters: Please enter band index\n", __func__));
		return -1;
	}
	u1BandIdx = simple_strtol(value, 0, 10);

	/* sanity check for Band index */
	if (u1BandIdx >= DBDC_BAND_NUM) {
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
				("Invalid Band Index!!!\n"));
		return FALSE;
	}

	value = strsep(&arg, ":");
	if (value == NULL) {
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s: set wrong parameters: Please enter PhyMode\n", __func__));
		return -1;
	}
	u1PhyMode = simple_strtol(value, 0, 10);

	value = strsep(&arg, "");
	if (value == NULL) {
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s: set wrong parameters: Please enter bandwidth\n", __func__));
		return -1;
	}
	u1Bw = simple_strtol(value, 0, 10);

	if (arch_ops && arch_ops->arch_txpower_boost_power_cat_type) {
		if (!arch_ops->arch_txpower_boost_power_cat_type(pAd, u1PhyMode, u1Bw, &u1PwrUpCat))
			return FALSE;
	}

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF,
		("=======================================================\n"));
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF,
			("Power Up Table (Band%d)\n", u1BandIdx));
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF,
		("=======================================================\n"));

	if (arch_ops && arch_ops->arch_txpower_boost_rate_type) {
		if (!arch_ops->arch_txpower_boost_rate_type(pAd, u1BandIdx, u1PwrUpCat))
			return FALSE;
	}

	return TRUE;
}
#endif /* TX_POWER_CONTROL_SUPPORT */

#ifdef ETSI_RX_BLOCKER_SUPPORT
INT32 ShowRssiThInfo(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("--------------------------------------------------------------\n"));
	MTWF_LOG(DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s: c1RWbRssiHTh: %d, c1RWbRssiLTh: %d, c1RIbRssiLTh: %d,c1WBRssiTh4R: %d\n", __FUNCTION__,
															pAd->c1RWbRssiHTh, pAd->c1RWbRssiLTh, pAd->c1RIbRssiLTh, pAd->c1WBRssiTh4R));
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("--------------------------------------------------------------\n"));
	return TRUE;
}
#endif /* end of ETSI_RX_BLOCKER_SUPPORT */

#ifdef EEPROM_RETRIEVE_SUPPORT
static UINT8 g_dump_content[MAX_EEPROM_BUFFER_SIZE];
INT32 show_e2p_proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	int status = 0;
	PCHAR pch = NULL;
	UINT16 dump_offset = 0x0;
	UINT16 dump_size = 0x20;
	UINT8 *dump_content = &g_dump_content[0];
	int i;
	if (arg != NULL) {
		pch = strsep(&arg, "-");
		if (pch != NULL)
			dump_offset = os_str_tol(pch, 0, 10);

		pch = strsep(&arg, "-");
		if (pch != NULL)
			dump_size = os_str_tol(pch, 0, 10);
	}

	MTWF_LOG(DBG_CAT_FW, DBG_SUBCAT_ALL, DBG_LVL_OFF,
			("\x1b[1;33m %s: eeprom_type=%d, offset=%d, size=%d\x1b[m \n",
			 __func__, pAd->eeprom_type, dump_offset, dump_size));

	MtCmdEfusBufferModeGet(pAd, EEPROM_EFUSE, dump_offset, dump_size, dump_content);

	MTWF_LOG(DBG_CAT_FW, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("Dump\n\r"));
	for (i = 0; i < dump_size; i++) {
		if ((i%32) == 0)
			MTWF_LOG(DBG_CAT_FW, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\n\r"));
		MTWF_LOG(DBG_CAT_FW, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%02x ", dump_content[i]));
	}
	MTWF_LOG(DBG_CAT_FW, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\n\r"));
	MTWF_LOG(DBG_CAT_FW, DBG_SUBCAT_ALL, DBG_LVL_OFF,
			("\x1b[1;33m %s: End \x1b[m \n", __func__));
	return TRUE;
}
#endif /* EEPROM_RETRIEVE_SUPPORT */

INT32 show_hwcfg_proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	struct _RTMP_CHIP_OP *ops = hc_get_chip_ops(pAd->hdev_ctrl);

	if (ops->show_hwcfg_info)
		ops->show_hwcfg_info(pAd);
	else
		return FALSE;

	return TRUE;
}

#ifdef ANDLINK_FEATURE_SUPPORT
INT get_sta_rate_info(
	RTMP_ADAPTER *pAd,
	MAC_TABLE_ENTRY *pEntry,
	mtk_rate_info_t *tx_rate_info,
	mtk_rate_info_t *rx_rate_info) {

	u16 wcid = 0;
	struct wifi_dev *wdev = NULL;
	STA_ADMIN_CONFIG *pstacfg = NULL;
	ADD_HT_INFO_IE *addht= NULL;
	ULONG datarate_tx = 0;
	ULONG datarate_rx = 0;
#ifdef RACTRL_FW_OFFLOAD_SUPPORT
	struct _RTMP_CHIP_CAP *cap = hc_get_chip_cap(pAd->hdev_ctrl);
#endif

	if (!pEntry || !tx_rate_info || !rx_rate_info) {
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
		("[%s](%d): ERROR!! pEntry=%p, tx_rate_info=%p, rx_rate_info=%p\n",
		__func__, __LINE__, pEntry, tx_rate_info, rx_rate_info));
		return FALSE;
	}

	wcid = pEntry->wcid;
	wdev = pEntry->wdev;
#ifdef CONFIG_STA_SUPPORT
	pstacfg = (STA_ADMIN_CONFIG *) GetStaCfgByWdev(pAd, wdev);
#endif /*CONFIG_STA_SUPPORT*/
	if (pEntry->Sst != SST_ASSOC) {
		MTWF_LOG(DBG_CAT_SEC, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			("[%s](%d):idx: %d, mac: %02X:%02X:%02X:%02X:%02X:%02X is disassociated\n",
			__func__, __LINE__, pEntry->wcid, PRINT_MAC(pEntry->Addr)));
		return FALSE;
	}

	addht = wlan_operate_get_addht(pEntry->wdev);
	getRate(pEntry->HTPhyMode, &datarate_tx);
#ifdef RACTRL_FW_OFFLOAD_SUPPORT
		if (cap->fgRateAdaptFWOffload == TRUE) {
			UCHAR phy_mode, rate, bw, sgi, stbc;
			UCHAR phy_mode_r, rate_r, bw_r, sgi_r, stbc_r;
			UCHAR nss;
			UCHAR nss_r;
			UINT32 RawData;
			UINT32 lastTxRate = pEntry->LastTxRate;
			UINT32 lastRxRate = pEntry->LastRxRate;
#ifdef MT7915
			UCHAR ucBand = HcGetBandByWdev(pEntry->wdev);
#endif

			EXT_EVENT_TX_STATISTIC_RESULT_T rTxStatResult;
			EXT_EVENT_PHY_STATE_RX_RATE rRxStatResult;
			HTTRANSMIT_SETTING LastTxRate;
			HTTRANSMIT_SETTING LastRxRate;

			MtCmdGetTxStatistic(pAd, GET_TX_STAT_ENTRY_TX_RATE, 0/*Don't Care*/, pEntry->wcid, &rTxStatResult);
			LastTxRate.field.MODE = rTxStatResult.rEntryTxRate.MODE;
			LastTxRate.field.BW = rTxStatResult.rEntryTxRate.BW;
			LastTxRate.field.ldpc = rTxStatResult.rEntryTxRate.ldpc ? 1 : 0;
			LastTxRate.field.ShortGI = rTxStatResult.rEntryTxRate.ShortGI ? 1 : 0;
			LastTxRate.field.STBC = rTxStatResult.rEntryTxRate.STBC;

			if (LastTxRate.field.MODE >= MODE_VHT)
				LastTxRate.field.MCS = (((rTxStatResult.rEntryTxRate.VhtNss - 1) & 0x3) << 4) + rTxStatResult.rEntryTxRate.MCS;
			else if (LastTxRate.field.MODE == MODE_OFDM)
				LastTxRate.field.MCS = getLegacyOFDMMCSIndex(rTxStatResult.rEntryTxRate.MCS) & 0x0000003F;
			else
				LastTxRate.field.MCS = rTxStatResult.rEntryTxRate.MCS;

			lastTxRate = (UINT32)(LastTxRate.word);
			LastRxRate.word = (USHORT)lastRxRate;
			RawData = lastTxRate;
			phy_mode = (RawData >> 13) & 0x7;
			rate = RawData & 0x3F;
			bw = (RawData >> 7) & 0x3;
			sgi = rTxStatResult.rEntryTxRate.ShortGI;
			stbc = ((RawData >> 10) & 0x1);
			nss = rTxStatResult.rEntryTxRate.VhtNss;

			MtCmdPhyGetRxRate(pAd, CMD_PHY_STATE_CONTENTION_RX_PHYRATE, ucBand, pEntry->wcid, &rRxStatResult);
			LastRxRate.field.MODE = rRxStatResult.u1RxMode;
			LastRxRate.field.BW = rRxStatResult.u1BW;
			LastRxRate.field.ldpc = rRxStatResult.u1Coding;
			LastRxRate.field.ShortGI = rRxStatResult.u1Gi ? 1 : 0;
			LastRxRate.field.STBC = rRxStatResult.u1Stbc;

			if (LastRxRate.field.MODE >= MODE_VHT)
				LastRxRate.field.MCS = ((rRxStatResult.u1RxNsts & 0x3) << 4) + rRxStatResult.u1RxRate;
			else if (LastRxRate.field.MODE == MODE_OFDM)
				LastRxRate.field.MCS = getLegacyOFDMMCSIndex(rRxStatResult.u1RxRate) & 0x0000003F;
			else
				LastRxRate.field.MCS = rRxStatResult.u1RxRate;

			phy_mode_r = rRxStatResult.u1RxMode;
			rate_r = rRxStatResult.u1RxRate & 0x3F;
			bw_r = rRxStatResult.u1BW;
			sgi_r = rRxStatResult.u1Gi;
			stbc_r = rRxStatResult.u1Stbc;
			nss_r = rRxStatResult.u1RxNsts + 1;
	/*TX MCS*/
#ifdef DOT11_VHT_AC
			if (phy_mode >= MODE_VHT) {
				rate = rate & 0xF;
			}
#endif /* DOT11_VHT_AC */


	/*RX MCS*/
#ifdef DOT11_VHT_AC
			if (phy_mode_r >= MODE_VHT) {
				rate_r = rate_r & 0xF;
			} else
#endif /* DOT11_VHT_AC */
#if DOT11_N_SUPPORT
			if (phy_mode_r >= MODE_HTMIX) {
				MTWF_LOG(DBG_CAT_SEC, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
					("[%s](%d):MODE_HTMIX(%d >= %d): MCS=%d\n",
				__func__, __LINE__, phy_mode_r, MODE_HTMIX, rate_r));
			} else
#endif
			if (phy_mode_r == MODE_OFDM) {
				if (rate_r == TMI_TX_RATE_OFDM_6M)
					LastRxRate.field.MCS = 0;
				else if (rate_r == TMI_TX_RATE_OFDM_9M)
					LastRxRate.field.MCS = 1;
				else if (rate_r == TMI_TX_RATE_OFDM_12M)
					LastRxRate.field.MCS = 2;
				else if (rate_r == TMI_TX_RATE_OFDM_18M)
					LastRxRate.field.MCS = 3;
				else if (rate_r == TMI_TX_RATE_OFDM_24M)
					LastRxRate.field.MCS = 4;
				else if (rate_r == TMI_TX_RATE_OFDM_36M)
					LastRxRate.field.MCS = 5;
				else if (rate_r == TMI_TX_RATE_OFDM_48M)
					LastRxRate.field.MCS = 6;
				else if (rate_r == TMI_TX_RATE_OFDM_54M)
					LastRxRate.field.MCS = 7;
				else
					LastRxRate.field.MCS = 0;

				rate_r = LastRxRate.field.MCS;
				rx_rate_info->mcs = rate_r;/*rx mcs: ofdm*/
			} else if (phy_mode_r == MODE_CCK) {
				if (rate_r == TMI_TX_RATE_CCK_1M_LP)
					LastRxRate.field.MCS = 0;
				else if (rate_r == TMI_TX_RATE_CCK_2M_LP)
					LastRxRate.field.MCS = 1;
				else if (rate_r == TMI_TX_RATE_CCK_5M_LP)
					LastRxRate.field.MCS = 2;
				else if (rate_r == TMI_TX_RATE_CCK_11M_LP)
					LastRxRate.field.MCS = 3;
				else if (rate_r == TMI_TX_RATE_CCK_2M_SP)
					LastRxRate.field.MCS = 1;
				else if (rate_r == TMI_TX_RATE_CCK_5M_SP)
					LastRxRate.field.MCS = 2;
				else if (rate_r == TMI_TX_RATE_CCK_11M_SP)
					LastRxRate.field.MCS = 3;
				else
					LastRxRate.field.MCS = 0;

				rate_r = LastRxRate.field.MCS;
				rx_rate_info->mcs = LastRxRate.field.MCS;/*rx_mcs:cck*/
			}

			if (phy_mode >= MODE_HE) {
				/*ax tx */
				get_rate_he((rate & 0xf), bw, nss, 0, &datarate_tx);
				if (sgi == 1)
					datarate_tx = (datarate_tx * 967) >> 10;
				if (sgi == 2)
					datarate_tx = (datarate_tx * 870) >> 10;

				get_rate_he((rate_r & 0xf), bw_r, nss_r, 0, &datarate_rx);
				if (sgi_r == 1)
					datarate_rx = (datarate_rx * 967) >> 10;
				if (sgi_r == 2)
					datarate_rx = (datarate_rx * 870) >> 10;
				/*tx rate infos*/
				tx_rate_info->flags |= MTK_RATE_INFO_FLAGS_HE_MCS;
				tx_rate_info->mcs = rate;
				tx_rate_info->legacy = (UINT16)datarate_tx;
				tx_rate_info->nss = nss;
				tx_rate_info->bw = bw;

				/*rx rate infos*/
				rx_rate_info->flags |= MTK_RATE_INFO_FLAGS_HE_MCS;
				rx_rate_info->mcs = rate_r;
				rx_rate_info->legacy = (UINT16)datarate_rx;
				rx_rate_info->nss = nss_r;
				rx_rate_info->bw = bw_r;

			} else {
				tx_rate_info->gi = sgi;/*tx_gi*/
				rx_rate_info->gi = sgi_r;/*tx_gi*/

				getRate(LastTxRate, &datarate_tx);
				getRate(LastRxRate, &datarate_rx);
				/*tx rate infos*/
				if (phy_mode >= MODE_VHT) {
					tx_rate_info->flags |= MTK_RATE_INFO_FLAGS_VHT_MCS;
				} else if (phy_mode >= MODE_HTMIX) {
					tx_rate_info->flags |= MTK_RATE_INFO_FLAGS_MCS;
				} else {
					tx_rate_info->flags = 0;/*other as legacy*/
				}
				tx_rate_info->mcs = rate;
				tx_rate_info->legacy = (UINT16)datarate_tx;
				tx_rate_info->nss = nss;
				tx_rate_info->bw = bw;

				/*rx rate infos*/
				if (phy_mode_r >= MODE_VHT) {
					rx_rate_info->flags |= MTK_RATE_INFO_FLAGS_VHT_MCS;
				} else if (phy_mode_r >= MODE_HTMIX) {
					rx_rate_info->flags |= MTK_RATE_INFO_FLAGS_VHT_MCS;
				} else {
					/*other as legacy*/
					rx_rate_info->flags = 0;
				}
				rx_rate_info->mcs = rate_r;
				rx_rate_info->legacy = (UINT16)datarate_rx;
				rx_rate_info->nss = nss_r;
				rx_rate_info->bw = bw_r;
			}
			/*tx rate infos*/
			MTWF_LOG(DBG_CAT_SEC, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
				("\n[%s](%d):tx_rate: rate=%u, mcs=%u, bw=%u, nss=%u\n",
				__func__, __LINE__, tx_rate_info->legacy,
				tx_rate_info->mcs, tx_rate_info->bw, tx_rate_info->nss));
			/*rx rate infos*/
			MTWF_LOG(DBG_CAT_SEC, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
				("\n[%s](%d):rx_rate: rate=%u, mcs=%u, bw=%u, nss=%u\n",
				__func__, __LINE__, rx_rate_info->legacy,
				rx_rate_info->mcs, rx_rate_info->bw, rx_rate_info->nss));

		}
		return TRUE;
#endif /* RACTRL_FW_OFFLOAD_SUPPORT */
	return FALSE;
}


#ifdef ANDLINK_HOSTNAME_IP

NDIS_STATUS update_sta_ip(IN PRTMP_ADAPTER	pAd,
	                   IN PNDIS_PACKET  pPkt) {
    NET_PRO_ARP_HDR * arpHdr = NULL;
    BOOLEAN isUcastMac, isGoodIP;
    PUCHAR	pSMac = NULL;
    PUCHAR pSIP = NULL;
    PUCHAR  pLayerHdr = NULL;
    PUCHAR  pPktHdr = NULL;
    UINT16  protoType;

    if (pAd == NULL && pPkt == NULL) {
        MTWF_LOG(DBG_CAT_RX, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("pAd or pPkt is NULL!\n"));
        return NDIS_STATUS_FAILURE;
    }

    pPktHdr = GET_OS_PKT_DATAPTR(pPkt);
	if (!pPktHdr)
		return NDIS_STATUS_FAILURE;

    /* Get the upper layer protocol type of this 802.3 pkt and dispatch to specific handler */
    protoType = OS_NTOHS(get_unaligned((PUINT16)(pPktHdr + 12)));
    /*ARP request check */
    if (ETH_P_ARP == protoType) {
#ifdef MAC_REPEATER_SUPPORT
        REPEATER_CLIENT_ENTRY *pRepEntry = NULL;
#endif

        /*arp ops check*/
        pLayerHdr = (pPktHdr + MAT_ETHER_HDR_LEN);
        arpHdr = (NET_PRO_ARP_HDR *)pLayerHdr;
		/*
		*Check the arp header.
		*We just handle ether type hardware address and IPv4 internet
		*address type and opcode is  ARP reuqest/response.
		*/
		if ((arpHdr->ar_hrd != OS_HTONS(ARPHRD_ETHER)) || (arpHdr->ar_pro != OS_HTONS(ETH_P_IP)) ||
			(arpHdr->ar_op != OS_HTONS(ARPOP_REPLY) && arpHdr->ar_op != OS_HTONS(ARPOP_REQUEST))){
			MTWF_LOG(DBG_CAT_RX, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("is not ARP packet!\n"));
			return NDIS_STATUS_FAILURE;
        }

        /*sender(Mac/IP address) fields*/
        pSMac = (pLayerHdr + 8);/*sender mac*/
        pSIP = (PUCHAR)(pSMac + MAC_ADDR_LEN);/*sender ip*/
        isUcastMac = IS_UCAST_MAC(pSMac);
        isGoodIP = IS_GOOD_IP(get_unaligned32((PUINT) pSIP));

#ifdef MAC_REPEATER_SUPPORT
        pRepEntry = RTMPLookupRepeaterCliEntry(pAd, TRUE, pSMac, TRUE);
        if (pAd->MatCfg.bMACRepeaterEn && pRepEntry != NULL && isUcastMac && isGoodIP) {
            if(pRepEntry != NULL && isUcastMac && isGoodIP) {
                pRepEntry->ipaddr = ((UINT)pSIP[0]<<24) + (((UINT)pSIP[1]<<16)) + (((UINT)pSIP[2]<<8)) + (UINT)pSIP[3];
                MTWF_LOG(DBG_CAT_RX, DBG_SUBCAT_ALL, DBG_LVL_WARN, ("%s(): Got the Mac(%02x:%02x:%02x:%02x:%02x:%02x) of IP(%d.%d.%d.%d)\n",
						__func__, pRepEntry->OriginalAddress[0],pRepEntry->OriginalAddress[1],pRepEntry->OriginalAddress[2], pRepEntry->OriginalAddress[3],pRepEntry->OriginalAddress[4],
						pRepEntry->OriginalAddress[5], (pRepEntry->ipaddr>>24) & 0xff, (pRepEntry->ipaddr>>16) & 0xff, (pRepEntry->ipaddr>>8) & 0xff, pRepEntry->ipaddr & 0xff));
                return NDIS_STATUS_SUCCESS;
            }
        }else
#endif
        {
            MAC_TABLE_ENTRY *pEntry = NULL;
            pEntry = MacTableLookup(pAd, pSMac);
            if (pEntry != NULL && isUcastMac && isGoodIP) {
                pEntry->ipaddr = ((UINT)pSIP[0]<<24) + (((UINT)pSIP[1]<<16)) + (((UINT)pSIP[2]<<8)) + (UINT)pSIP[3];

                MTWF_LOG(DBG_CAT_RX, DBG_SUBCAT_ALL, DBG_LVL_WARN, ("%s(): Got the Mac(%02x:%02x:%02x:%02x:%02x:%02x) of IP(%d.%d.%d.%d)\n",
						__func__, pEntry->Addr[0],pEntry->Addr[1],pEntry->Addr[2], pEntry->Addr[3],pEntry->Addr[4],
						pEntry->Addr[5], (pEntry->ipaddr>>24) & 0xff, (pEntry->ipaddr>>16) & 0xff, (pEntry->ipaddr>>8) & 0xff, pEntry->ipaddr & 0xff));
                return NDIS_STATUS_SUCCESS;
            }else {
                MTWF_LOG(DBG_CAT_RX, DBG_SUBCAT_ALL, DBG_LVL_WARN,
                         ("[%s](%d) Can't find STA!\n",__func__, __LINE__));;
            }
        }
    }

    return NDIS_STATUS_FAILURE;
}

/*
*	========================================================================
*	Routine	Description:
*		andlink Sepc get each sta hostname in repeater mode.
*
*	Arguments:
*		pAd		=>Pointer to our adapter
*		pPkt 	=>pointer to the 802.3 header of outgoing packet
*
*	Return Value:
*		Success	=>
*			TRUE
*			find sta hostname in DHCP packet.
*		Error	=>
*			FALSE.
*
*	Note:
*		1.the pPktHdr must be a 802.3 packet.
*		2.We check every packet handle DHCP packet to get hostname.
*	========================================================================
 */
NDIS_STATUS update_sta_hostname(IN PRTMP_ADAPTER	pAd,
	                         IN PNDIS_PACKET  pPkt){
    PNDIS_PACKET newSkb = NULL;
    PUCHAR pSrcMac = NULL;
	PUCHAR pSrcIP = NULL;
    PUCHAR  pLayerHdr = NULL;
    PUCHAR  pPktHdr = NULL;
    UINT16  protoType;
    UCHAR pCliMacAdr[MAC_ADDR_LEN]={0};
	UCHAR  DHCP_MAGIC[] =  {0x63, 0x82, 0x53, 0x63};

    if (pAd == NULL && pPkt == NULL) {
        MTWF_LOG(DBG_CAT_RX, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("pAd or pPkt is NULL!\n"));
        return NDIS_STATUS_FAILURE;
    }

    pPktHdr = GET_OS_PKT_DATAPTR(pPkt);
	if (!pPktHdr)
		return NDIS_STATUS_FAILURE;

    /* Get the upper layer protocol type of this 802.3 pkt and dispatch to specific handler */
    protoType = OS_NTOHS(get_unaligned((PUINT16)(pPktHdr + 12)));
    /*DHCP Discover check*/
    if (ETH_P_IP == protoType) {
        pLayerHdr = (pPktHdr + MAT_ETHER_HDR_LEN);
        pSrcMac = pPktHdr + 6;/*src mac*/
        pSrcIP = pLayerHdr + 12;/*src ip*/
        /*For UDP packet, we need to check about the DHCP packet, to modify the flag of DHCP discovey/request as broadcast. */
        if (*(pLayerHdr + 9) == 0x11) {
            PUCHAR udpHdr;
            UINT16 srcPort, dstPort;
            udpHdr = pLayerHdr + 20;
            srcPort = OS_NTOHS(get_unaligned((PUINT16)(udpHdr)));
            dstPort = OS_NTOHS(get_unaligned((PUINT16)(udpHdr + 2)));
            /*It's a DHCP packet */
            if (srcPort == 68 && dstPort == 67) {
                PUCHAR bootpHdr;
                UINT16 bootpFlag;
                PUCHAR dhcpHdr;
                PNDIS_PACKET pSkb;

                pSkb = RTPKT_TO_OSPKT(pPkt);
                if (OS_PKT_CLONED(pSkb)) {
				    OS_PKT_COPY(pSkb, newSkb);

                    if (newSkb) {
                        /* reassign packet header pointer for new skb*/
                        if (IS_VLAN_PACKET(GET_OS_PKT_DATAPTR(newSkb))) {
                            pPktHdr = GET_OS_PKT_DATAPTR(newSkb);
                            pLayerHdr = (pPktHdr + MAT_VLAN_ETH_HDR_LEN);
                            udpHdr = pLayerHdr + 20;
                        } else {
                            pPktHdr = GET_OS_PKT_DATAPTR(newSkb);
                            pLayerHdr = (pPktHdr + MAT_ETHER_HDR_LEN);
                            udpHdr = pLayerHdr + 20;
                        }
                    }
                }
                bootpHdr = udpHdr + 8;
                bootpFlag = OS_NTOHS(get_unaligned((PUINT16)(bootpHdr + 10)));
                dhcpHdr = bootpHdr + 236;
                /*client hw address*/
                NdisMoveMemory(pCliMacAdr, (bootpHdr + 28), MAC_ADDR_LEN);
                /*dhcp magic check*/
                if (NdisEqualMemory(dhcpHdr, DHCP_MAGIC, 4)){
                    PUCHAR pOptCode, pOptLen;
                    UINT16 udpLen;
#ifdef MAC_REPEATER_SUPPORT
                    PREPEATER_CLIENT_ENTRY pRepEntry = NULL;
#endif


                    udpLen = OS_NTOHS(get_unaligned((PUINT16)(udpHdr + 4)));
                    pOptCode = (dhcpHdr + 4);

                    do {
                        pOptLen = pOptCode + 1;

                        if (*pOptCode == 12) {  /*hostname*/
                            /* update hostname */
#ifdef MAC_REPEATER_SUPPORT
                            pRepEntry = RTMPLookupRepeaterCliEntry(pAd, TRUE, pCliMacAdr, TRUE);

                            if (pAd->MatCfg.bMACRepeaterEn && pRepEntry) {
                                if(pRepEntry != NULL) {
                                    NdisZeroMemory(pRepEntry->hostname, HOSTNAME_LEN);
                                    NdisMoveMemory(pRepEntry->hostname, pOptCode+2, *pOptLen > HOSTNAME_LEN ? HOSTNAME_LEN : *pOptLen);
                                    MTWF_LOG(DBG_CAT_RX, DBG_SUBCAT_ALL, DBG_LVL_WARN,
                                         ("HostName: %s\n",
                                          pRepEntry->hostname));
                                    return NDIS_STATUS_SUCCESS;
                                }
                            }else
#endif/*MAC_REPEATER_SUPPORT*/
                            {
                                MAC_TABLE_ENTRY *pEntry = NULL;
                                pEntry = MacTableLookup(pAd, pCliMacAdr);
                                if (NULL != pEntry) {
                                    NdisZeroMemory(pEntry->hostname, HOSTNAME_LEN);
                                    NdisMoveMemory(pEntry->hostname, pOptCode+2, *pOptLen);
                                    MTWF_LOG(DBG_CAT_RX, DBG_SUBCAT_ALL, DBG_LVL_WARN,
                                         ("HostName: %s\n", pEntry->hostname));
                                    return NDIS_STATUS_SUCCESS;
                                }else {
                                    MTWF_LOG(DBG_CAT_RX, DBG_SUBCAT_ALL, DBG_LVL_WARN,
                                     ("[%s](%d)Can't find STA!\n",__func__, __LINE__));
                                }
                            }

                        }

                        pOptCode += (2 + *pOptLen);
                    } while ((*pOptCode != 0xFF) && ((pOptCode - udpHdr) <= udpLen));

                }
            }
        }
    }
    return NDIS_STATUS_FAILURE;
}
#endif/*ANDLINK_HOSTNAME_IP*/
#endif/*ANDLINK_FEATURE_SUPPORT*/


#ifdef ACK_CTS_TIMEOUT_SUPPORT
static UINT32 get_ack_timeout_bycr(RTMP_ADAPTER *pAd, UINT32 reg_addr, UINT32 *ptimeout)
{

	if (NULL == pAd || NULL == ptimeout || NULL == pAd->hdev_ctrl) {
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			("%s: invalid null input: pAd=%p, pAd->hdev_ctrl=%p, ptimeout=%p!!\n",
			__FUNCTION__, pAd, pAd->hdev_ctrl, ptimeout));
		return FALSE;
	}

	MAC_IO_READ32(pAd->hdev_ctrl, reg_addr, ptimeout);
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
		("reg_addr(%x) = %x!!\n", reg_addr, *ptimeout));
	return TRUE;
}

static INT32 get_ack_timeout_mode_byband(
	RTMP_ADAPTER *pAd,
	UINT32 *ptimeout,
	UINT32 bandidx,
	ACK_TIMEOUT_MODE_T ackmode)
{
	UINT32 tmp_val = 0;

	if ((*ptimeout > MAX_ACK_TIMEOUT)) {
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			("CTS/ACK Timeout Range should between [0xFFFF:0]!!\n"));
		return FALSE;
	}

	if (pAd->CommonCfg.ack_cts_enable[bandidx] == FALSE) {
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			("[%s](%d):ERROR! BAND%u, ack_cts_enable=%u, CTS/ACK FEATURE is not enable!!\n",
			__func__, __LINE__, bandidx, pAd->CommonCfg.ack_cts_enable[bandidx]));
		return FALSE;
	}

	switch (bandidx) {
		case BAND0: {
			/*set BAND0 CCK OFDM OFDMA TIMEOUT*/
			switch (ackmode) {
				case CCK_TIME_OUT: {
					get_ack_timeout_bycr(pAd, BN0_WF_TMAC_TOP_CDTR_ADDR, ptimeout);
					break;
				}
				case OFDM_TIME_OUT: {
					get_ack_timeout_bycr(pAd, BN0_WF_TMAC_TOP_ODTR_ADDR, ptimeout);
					break;
				}
				case OFDMA_TIME_OUT: {
					get_ack_timeout_bycr(pAd, BN0_WF_TMAC_TOP_OMDTR_ADDR, ptimeout);
					break;
				}
				case ACK_ALL_TIME_OUT: {
					get_ack_timeout_bycr(pAd, BN0_WF_TMAC_TOP_CDTR_ADDR, &tmp_val);
					*ptimeout = (tmp_val & MAX_ACK_TIMEOUT);
					get_ack_timeout_bycr(pAd, BN0_WF_TMAC_TOP_ODTR_ADDR, &tmp_val);
					*ptimeout = max(*ptimeout, (tmp_val & MAX_ACK_TIMEOUT));
					get_ack_timeout_bycr(pAd, BN0_WF_TMAC_TOP_OMDTR_ADDR, &tmp_val);
					*ptimeout = max(*ptimeout, (tmp_val & MAX_ACK_TIMEOUT));
					MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
					("[%s](%d): *ptimeout = %d us!!\n",
					__func__, __LINE__, (*ptimeout) & MAX_ACK_TIMEOUT));
					break;
				}
				default: {
					MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
					("%s: invalid ackmode ,  ackmode=%d!!\n",
					__FUNCTION__, ackmode));
					return FALSE;
				}
			}
			break;
		}
		case BAND1: {
			/*set BAND1 CCK OFDM OFDMA TIMEOUT*/
			switch (ackmode) {
				case CCK_TIME_OUT: {
					get_ack_timeout_bycr(pAd, BN1_WF_TMAC_TOP_CDTR_ADDR, ptimeout);
					break;
				}
				case OFDM_TIME_OUT: {
					get_ack_timeout_bycr(pAd, BN1_WF_TMAC_TOP_ODTR_ADDR, ptimeout);
					break;
				}
				case OFDMA_TIME_OUT: {
					get_ack_timeout_bycr(pAd, BN1_WF_TMAC_TOP_OMDTR_ADDR, ptimeout);
					break;
				}
				case ACK_ALL_TIME_OUT: {
					get_ack_timeout_bycr(pAd, BN1_WF_TMAC_TOP_CDTR_ADDR, &tmp_val);
					*ptimeout = (tmp_val & MAX_ACK_TIMEOUT);
					get_ack_timeout_bycr(pAd, BN1_WF_TMAC_TOP_ODTR_ADDR, &tmp_val);
					*ptimeout = max(*ptimeout, (tmp_val & MAX_ACK_TIMEOUT));
					get_ack_timeout_bycr(pAd, BN1_WF_TMAC_TOP_OMDTR_ADDR, &tmp_val);
					*ptimeout = max(*ptimeout, (tmp_val & MAX_ACK_TIMEOUT));
					MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
					("[%s](%d): *ptimeout = %d us!!\n",
					__func__, __LINE__, (*ptimeout) & MAX_ACK_TIMEOUT));
					break;
				}
				default: {
					MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
					("%s: invalid ackmode,  ackmode=%d!!\n",
					__FUNCTION__, ackmode));
					return FALSE;
				}
			}
			break;
		}
		default: {
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			("%s: invalid BAND idx,  band_id=%d!!\n",
			__FUNCTION__, bandidx));
			return FALSE;
		}
	}
	return TRUE;
}


static INT32 get_cck_ofdm_ofdma_tout (RTMP_ADAPTER *pAd, UINT32 *ptimeout, ACK_TIMEOUT_MODE_T ack_mode)
{
	struct wifi_dev *wdev = NULL;
	POS_COOKIE pObj = (POS_COOKIE)pAd->OS_Cookie;
	UCHAR band_idx = 0;

	if (NULL == pAd || NULL == pObj) {
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			("%s: invalid null input: pAd=%p; pObj=%p!!\n",
			__func__, pAd, pObj));
		return FALSE;
	}

	if (((pObj->ioctl_if_type == INT_MBSSID) || (pObj->ioctl_if_type == INT_MAIN))
		&& (pObj->ioctl_if < MAX_BEACON_NUM)) {
			wdev = &pAd->ApCfg.MBSSID[pObj->ioctl_if].wdev;
	} else {
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			("%s: invalid interafce or interface idx: %d!!\n",
			__func__, pObj->ioctl_if));
		return FALSE;
	}

	/*GET BANDINDX*/
	band_idx = HcGetBandByWdev(wdev);


	/*get from chip*/
	if (FALSE == get_ack_timeout_mode_byband(pAd, ptimeout, band_idx, ack_mode)) {
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			("[%s](%d):SET CTS/ACK Timeout Fail!!\n", __func__, __LINE__));
		return FALSE;
	}

	return TRUE;
}

INT show_distance_proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UINT32 mac_val = 0;
	UINT32 distance = 0;

	/* Read CCK ACK Time CR*/
	if (TRUE == get_cck_ofdm_ofdma_tout(pAd, &mac_val, ACK_ALL_TIME_OUT)) {
		distance = ((mac_val & MAX_ACK_TIMEOUT)/2)*LIGHT_SPEED;
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF,
		("Distance = %d m\n", distance));
		return TRUE;
	} else {
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
		("[%s](%d): SHOW CCK/OFDM/OFDMA CTS/ACK Timeout FAIL!\n",
		__func__, __LINE__));
		return FALSE;
	}
}

INT show_cck_ack_timeout_porc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UINT32 mac_val = 0;

	/* Read CCK ACK Time CR*/
	if (TRUE == get_cck_ofdm_ofdma_tout(pAd, &mac_val, CCK_TIME_OUT)) {
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF,
		("CCK CTS/ACK Timeout = %d us\n",
		(mac_val & MAX_ACK_TIMEOUT)));
		return TRUE;
	} else {
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
		("[%s](%d): SHOW OFDM CTS/ACK Timeout FAIL!\n",
		__func__, __LINE__));
		return FALSE;
	}
}

INT show_ofdm_ack_timeout_proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UINT32 mac_val = 0;

	/* Read OFDM ACK Time CR*/
	if (TRUE == get_cck_ofdm_ofdma_tout(pAd, &mac_val, OFDM_TIME_OUT)) {
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF,
		("OFDM CTS/ACK Timeout = %d us\n",
		(mac_val & MAX_ACK_TIMEOUT)));
		return TRUE;
	} else {
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
		("[%s](%d): SHOW OFDM CTS/ACK Timeout FAIL!\n",
		__func__, __LINE__));
		return FALSE;
	}
}

INT show_ofdma_ack_timeout_proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UINT32 mac_val = 0;

	/* Read OFDMA ACK Time CR*/
	if (TRUE == get_cck_ofdm_ofdma_tout(pAd, &mac_val, OFDMA_TIME_OUT)) {
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF,
		("OFDMA CTS/ACK Timeout = %d us\n",
		(mac_val & MAX_ACK_TIMEOUT)));
		return TRUE;
	} else {
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
		("[%s](%d): SHOW OFDMA CTS/ACK Timeout FAIL!\n",
		__func__, __LINE__));
		return FALSE;
	}
}

#endif/*ACK_CTS_TIMEOUT_SUPPORT*/

