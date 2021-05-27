/***************************************************************************
* MediaTek Inc.
* 4F, No. 2 Technology 5th Rd.
* Science-based Industrial Park
* Hsin-chu, Taiwan, R.O.C.
*
* (c) Copyright 1997-2012, MediaTek, Inc.
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
#include "wlan_config/config_internal.h"

/*
*vht phy related
*/
#ifdef DOT11_VHT_AC
VOID vht_cfg_init(struct vht_cfg *obj)
{
	obj->vht_bw = VHT_BW_80;
	obj->vht_ldpc = TRUE;
	obj->vht_stbc = STBC_USE;
	obj->vht_sgi = GI_400;
	obj->vht_bw_sig = BW_SIGNALING_DISABLE;
	obj->max_mpdu_len = MPDU_7991_OCTETS;
}

VOID vht_cfg_exit(struct vht_cfg *obj)
{
	os_zero_mem(obj, sizeof(struct vht_cfg));
}

/*
*vht phy op related
*/

/*
* exported operation function.
*/

/*
* exported configure function.
*/

/*
* Set
*/
VOID wlan_config_set_vht_bw(struct wifi_dev *wdev, UCHAR vht_bw)
{
	struct wlan_config *cfg = (struct wlan_config *)wdev->wpf_cfg;

	if (cfg)
		cfg->vht_conf.vht_bw = vht_bw;
}

VOID wlan_config_set_vht_bw_all(struct wpf_ctrl *ctrl, UCHAR vht_bw)
{
	struct wlan_config *cfg;
	unsigned int i;

	for (i = 0; i < WDEV_NUM_MAX; i++) {
		cfg = (struct wlan_config *)ctrl->pf[i].conf;

		if (cfg)
			cfg->vht_conf.vht_bw = vht_bw;
	}
}

VOID wlan_config_set_vht_stbc(struct wifi_dev *wdev, UCHAR vht_stbc)
{
	struct wlan_config *cfg = (struct wlan_config *)wdev->wpf_cfg;

	if (cfg)
		cfg->vht_conf.vht_stbc = vht_stbc;
}

VOID wlan_config_set_vht_ldpc(struct wifi_dev *wdev, UCHAR vht_ldpc)
{
	struct wlan_config *cfg = (struct wlan_config *)wdev->wpf_cfg;

	if (cfg)
		cfg->vht_conf.vht_ldpc = vht_ldpc;
}

VOID wlan_config_set_vht_sgi(struct wifi_dev *wdev, UCHAR vht_sgi)
{
	struct wlan_config *cfg = (struct wlan_config *)wdev->wpf_cfg;

	if (cfg)
		cfg->vht_conf.vht_sgi = vht_sgi;
}

VOID wlan_config_set_vht_bw_sig(struct wifi_dev *wdev, UCHAR vht_bw_sig)
{
	struct wlan_config *cfg = (struct wlan_config *)wdev->wpf_cfg;

	if (cfg)
		cfg->vht_conf.vht_bw_sig = vht_bw_sig;
}

VOID wlan_config_set_vht_ext_nss_bw(struct wifi_dev *wdev, UINT8 ext_nss_bw)
{
	struct wlan_config *cfg = (struct wlan_config *)wdev->wpf_cfg;

	if (cfg)
		cfg->vht_conf.ext_nss_bw = ext_nss_bw;
}

VOID wlan_config_set_vht_max_mpdu_len(struct wifi_dev *wdev, UINT8 max_mpdu_len)
{
	struct wlan_config *cfg = (struct wlan_config *)wdev->wpf_cfg;

	if (cfg)
		cfg->vht_conf.max_mpdu_len = max_mpdu_len;
}

/*
* Get
*/
UCHAR wlan_config_get_vht_bw(struct wifi_dev *wdev)
{
	struct wlan_config *cfg = (struct wlan_config *)wdev->wpf_cfg;

	if (cfg)
		return cfg->vht_conf.vht_bw;
	else
		return 0;
}

UCHAR wlan_config_get_vht_stbc(struct wifi_dev *wdev)
{
	struct wlan_config *cfg = (struct wlan_config *)wdev->wpf_cfg;

	if (cfg)
		return cfg->vht_conf.vht_stbc;
	else
		return 0;
}

UCHAR wlan_config_get_vht_ldpc(struct wifi_dev *wdev)
{
	struct wlan_config *cfg = (struct wlan_config *)wdev->wpf_cfg;

	if (cfg)
		return cfg->vht_conf.vht_ldpc;
	else
		return 0;
}

UCHAR wlan_config_get_vht_sgi(struct wifi_dev *wdev)
{
	struct wlan_config *cfg = (struct wlan_config *)wdev->wpf_cfg;

	if (cfg)
		return cfg->vht_conf.vht_sgi;
	else
		return 0;
}

UCHAR wlan_config_get_vht_bw_sig(struct wifi_dev *wdev)
{
	struct wlan_config *cfg = (struct wlan_config *)wdev->wpf_cfg;

	if (cfg)
		return cfg->vht_conf.vht_bw_sig;
	else
		return 0;
}

UINT8 wlan_config_get_vht_ext_nss_bw(struct wifi_dev *wdev)
{
	struct wlan_config *cfg = (struct wlan_config *)wdev->wpf_cfg;

	if (cfg)
		return cfg->vht_conf.ext_nss_bw;
	else
		return 0;
}

UINT8 wlan_config_get_vht_max_mpdu_len(struct wifi_dev *wdev)
{
	struct wlan_config *cfg = (struct wlan_config *)wdev->wpf_cfg;

	if (cfg)
		return cfg->vht_conf.max_mpdu_len;
	else
		return 0;
}
#endif /*DOT11_VHT_AC*/
