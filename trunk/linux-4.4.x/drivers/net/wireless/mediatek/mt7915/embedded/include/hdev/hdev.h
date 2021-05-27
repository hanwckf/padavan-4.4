/*
 ***************************************************************************
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

#ifndef __HDEV_H
#define __HDEV_H

#include "hdev/hdev_basic.h"

/*Radio Control*/
VOID rc_radio_init(struct hdev_ctrl *ctrl, UCHAR rfic, UCHAR dbdc_mode);
VOID rc_radio_exit(struct hdev_ctrl *ctrl, UCHAR dbdc_mode);
struct radio_dev *rc_init(struct hdev_ctrl *ctrl);
VOID RcRadioShow(HD_RESOURCE_CFG *pHwResourceCfg);

struct radio_dev *RcAcquiredBandForObj(
	struct hdev_ctrl *ctrl,
	struct hdev_obj *obj,
	UCHAR ObjIdx,
	USHORT PhyMode,
	UCHAR Channel,
	USHORT ObjType);

VOID RcReleaseBandForObj(struct hdev_ctrl *ctrl, struct hdev_obj *obj);

INT32 RcUpdateChannel(struct radio_dev *rdev, UCHAR Channel, BOOLEAN scan_state);
INT32 RcUpdatePhyMode(struct radio_dev *rdev, UCHAR Wmode);
struct radio_dev *RcGetHdevByChannel(struct hdev_ctrl *ctrl, UCHAR Channel);
UCHAR RcGetBandIdxByRf(struct hdev_ctrl *ctrl, UCHAR RfIC);


INT32 RcUpdateBandCtrl(struct hdev_ctrl *ctrl);
INT32 RcUpdateWmmEntry(struct radio_dev *rdev, struct hdev_obj *obj, UINT32 WmmIdx);
INT32 RcUpdateRepeaterEntry(struct radio_dev *rdev, UINT32 ReptIdx);
UCHAR RcUpdateBw(struct radio_dev *rdev, UCHAR Bw);
INT32 RcUpdateRadio(struct radio_dev *rdev, UCHAR bw, UCHAR central_ch1, UCHAR control_ch2, UCHAR ext_cha, UCHAR rx_stream);
INT32 RcUpdateExtCha(struct radio_dev *rdev, UCHAR ExtCha);
UCHAR RcGetExtCha(struct radio_dev *rdev);
UINT32 RcGetMgmtQueueIdx(struct hdev_obj *obj, enum PACKET_TYPE pkt_type);
UINT32 RcGetBcnQueueIdx(struct hdev_obj *obj);
UINT32 RcGetWmmIdx(struct hdev_obj *obj);
UINT32 RcGetBandIdxByChannel(struct hdev_ctrl *ctrl, UCHAR Channel);
USHORT RcGetPhyMode(struct radio_dev *rdev);
UCHAR RcGetChannel(struct radio_dev *rdev);
UCHAR RcGetCentralCh(struct radio_dev *rdev);
UCHAR RcGetBandIdx(struct radio_dev *rdev);
PHY_STATUS RcGetRadioCurStat(struct radio_dev *rdev);
VOID RcSetRadioCurStat(struct radio_dev *rdev, PHY_STATUS CurStat);
UCHAR RcGetBw(struct radio_dev *rdev);
struct radio_dev *RcGetBandIdxByBf(struct hdev_ctrl *ctrl);
BOOLEAN RcIsBfCapSupport(struct hdev_obj *obj);
BOOLEAN rc_radio_equal(struct radio_dev *rdev, struct freq_oper *oper);
BOOLEAN rc_radio_res_acquire(struct radio_dev *rdev, struct radio_res *res);


/*WMM Control*/
VOID wmm_ctrl_release_entry(struct hdev_obj *obj);
struct wmm_entry  *wmm_ctrl_acquire_entry(struct hdev_obj *obj, struct _EDCA_PARM *pEdcaParm);
INT32 wmm_ctrl_init(struct hdev_ctrl *ctrl, struct wmm_ctrl *wctrl);
INT32 wmm_ctrl_exit(struct wmm_ctrl *ctrl);
VOID wmm_ctrl_show_entry(struct wmm_ctrl *ctrl);
struct wmm_entry *wmm_ctrl_get_entry_by_idx(struct hdev_ctrl *ctrl, UINT32 Idx);
VOID wmm_ctrl_set_edca(struct hdev_obj *obj);

/*Omac Control*/
INT32 GetOmacIdx(struct hdev_ctrl *ctrl, UINT32 OmacType, struct radio_dev *rdev, INT8 Idx);
VOID ReleaseOmacIdx(struct hdev_ctrl *ctrl, UINT32 OmacType, struct radio_dev *rdev, UINT32 Idx);
VOID OcDelRepeaterEntry(struct hdev_obj *obj, UCHAR ReptIdx);
INT32 OcAddRepeaterEntry(struct hdev_obj *obj, UCHAR ReptIdx);
HD_REPT_ENRTY *OcGetRepeaterEntry(struct hdev_obj *obj, UCHAR ReptIdx);


/*Wctl Control*/
VOID WtcInit(struct hdev_ctrl *ctrl);
VOID WtcExit(struct hdev_ctrl *ctrl);
UINT16 WtcSetMaxStaNum(struct hdev_ctrl *ctrl, UCHAR BssidNum, UCHAR MSTANum);
UINT16 WtcGetMaxStaNum(struct hdev_ctrl *ctrl);
UINT16 WtcAcquireGroupKeyWcid(struct hdev_ctrl *ctrl, struct hdev_obj *obj);
UINT16 WtcReleaseGroupKeyWcid(struct hdev_ctrl *ctrl, struct hdev_obj *obj, UINT16 wcid);
UCHAR WtcGetWcidLinkType(struct hdev_ctrl *ctrl, UINT16 wcid);
UINT16 WtcAcquireUcastWcid(struct hdev_ctrl *ctrl, struct hdev_obj *obj, UINT16 FirstWcid);
UINT16 WtcReleaseUcastWcid(struct hdev_ctrl *ctrl, struct hdev_obj *obj, UINT16 wcid);
VOID WtcRecDump(struct hdev_ctrl *ctrl);
UINT16 WtcHwAcquireWcid(struct hdev_ctrl *ctrl, UINT16 wcid);
UINT16 WtcHwReleaseWcid(struct hdev_ctrl *ctrl, UINT16 wcid);
#ifdef DOT11_HE_AX
#ifdef WIFI_TWT_SUPPORT
VOID twt_ctrl_init(struct hdev_ctrl *ctrl);
VOID twt_ctrl_exit(struct hdev_ctrl *ctrl);
struct twt_link_node *twt_ctrl_acquire_twt_node(struct hdev_ctrl *ctrl, struct hdev_obj *obj, BOOLEAN type);
BOOLEAN twt_ctrl_release_twt_node(struct hdev_ctrl *ctrl, struct hdev_obj *obj, struct twt_link_node *twt_node);
UINT8 twt_ctrl_get_max_twt_node_num(struct hdev_ctrl *ctrl);
struct twt_link_node *twt_ctrl_get_twt_node_by_index(struct hdev_ctrl *ctrl, UINT8 agrt_tbl_idx);
VOID twt_ctrl_get_free_twt_node_num(struct hdev_ctrl *ctrl, UINT8 *individual_num, UINT8 *group_num);
UINT8 twt_ctrl_acquire_link_entry(struct hdev_ctrl *ctrl, struct hdev_obj *obj);
BOOLEAN twt_ctrl_release_link_entry(struct hdev_ctrl *ctrl, struct hdev_obj *obj, UINT8 idx);
VOID twt_ctrl_resource_status_dump(struct hdev_ctrl *ctrl);
#endif /* WIFI_TWT_SUPPORT */
#endif /* DOT11_HE_AX */

#ifdef DOT11_HE_AX
/* BSS color table contrl */
void bss_color_table_init(struct hdev_ctrl *ctrl);
void bss_color_table_deinit(struct hdev_ctrl *ctrl);
UINT8 bcolor_acquire_entry(struct hdev_ctrl *ctrl, struct hdev_obj *obj);
void bcolor_release_entry(struct hdev_ctrl *ctrl, struct hdev_obj *obj, UINT8 color);
void bcolor_occupy_entry(struct hdev_ctrl *ctrl, struct hdev_obj *obj, UINT8 color);
BOOLEAN bcolor_entry_is_occupied(struct hdev_ctrl *ctrl, struct hdev_obj *obj, UINT8 color);
void bcolor_entry_ageout(struct hdev_ctrl *ctrl, struct hdev_obj *obj, UINT8 sec);
void bcolor_get_bitmap(struct hdev_ctrl *ctrl, struct hdev_obj *obj, UINT8 *bitmap);
void bcolor_update_by_bitmap(struct hdev_ctrl *ctrl, struct hdev_obj *obj, UINT8 *bitmap);
struct pe_control *rc_get_pe_ctrl(struct radio_dev *r_dev);
#endif

#endif /*__HDEV_H*/
