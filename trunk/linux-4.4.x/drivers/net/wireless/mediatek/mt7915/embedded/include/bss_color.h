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


#ifndef __BSS_COLOR_H__
#define __BSS_COLOR_H__

#define BSS_COLOR_VALUE_MIN	1
#define BSS_COLOR_VALUE_MAX	63
#define BSS_COLOR_ENTRY_MAX	32

/* the duration for which an HE AP shall wait before disabling BSS color, 50 ~ 255 */
#define BSS_COLOR_COLLISION_AP_PERIOD 50
/* the interval between successive BSS color collision reports, 5 ~ 10 */
#define BSS_COLOR_COLLISION_STA_PERIOD 5
/* Countdown in BSS Color Change Announcement IE */
#define BSS_COLOR_CA_COUNT_DEFAULT 10
/* used to age out rombie BSS Color, seconds  */
#define BSS_COLOR_AGEOUT_TIME_DEFAULT 10
/* amount of Report Event should be sent */
#define BSS_COLOR_NOTIFY_TIMES_DEFAULT 3

struct bss_color_ctrl {
	BOOLEAN disabled;
	UINT8 color;
	UINT8 next_color;
	BOOLEAN collision_detected;
	ULONG collision_time;
	union {
#ifdef CONFIG_AP_SUPPORT
		struct {
			UCHAR bcc_count;
			UCHAR ap_collision_period;
			BOOLEAN trigger_timer_running;
			RALINK_TIMER_STRUCT trigger_timer;
		} ap_ctrl;
#endif
#ifdef CONFIG_STA_SUPPORT
		struct {
			UINT8 collision_notify_times;
			UCHAR sta_collision_period;
			BOOLEAN notify_timer_running;
			RALINK_TIMER_STRUCT notify_timer;
		} sta_ctrl;
#endif
	} u;
};

enum dbg_action {
	BSS_COLOR_DBG_OCCUPY = 1,
	BSS_COLOR_DBG_SETPERIOD,
	BSS_COLOR_DBG_TRIGGER,
	BSS_COLOR_DBG_CHANGE,
	BSS_COLOR_DBG_ASSIGN_MANUAL,
	BSS_COLOR_DBG_CHANGE_MANUAL,
	BSS_COLOR_DBG_MAX = BSS_COLOR_DBG_CHANGE_MANUAL
};

BOOLEAN get_bss_color_disabled(struct wifi_dev *wdev);
BOOLEAN get_bss_color_collision(struct wifi_dev *wdev);
UINT8 get_bss_color(struct wifi_dev *wdev);
void set_bss_color_info(struct wifi_dev *wdev, BOOLEAN disabled, UINT8 color);
void get_bss_color_bitmap(struct wifi_dev *wdev, UINT8 *bitmap);
void bss_color_parse_collision_report(struct wifi_dev *wdev, UINT8 *bitmap);
void bss_color_parse_inuse_report(struct wifi_dev *wdev, UINT8 *color);
void bss_color_collision_detect(struct wifi_dev *wdev, BOOLEAN disabled, UINT8 color);
void bss_color_event_handler(struct wifi_dev *wdev);
void bss_color_init(struct wifi_dev *wdev, struct _BSS_INFO_ARGUMENT_T *bssinfo);
void bss_color_timer_init(struct wifi_dev *wdev);
void bss_color_timer_release(struct wifi_dev *wdev);
void show_bss_color_info(struct _RTMP_ADAPTER *ad);
void set_bss_color_dbg(struct _RTMP_ADAPTER *ad, UINT8 wdev_idx, UINT8 action, UINT8 value);

#ifdef CONFIG_AP_SUPPORT
void trigger_timer_callback(
		IN PVOID SystemSpecific1,
		IN PVOID FunctionContext,
		IN PVOID SystemSpecific2,
		IN PVOID SystemSpecific3);
#endif

#ifdef CONFIG_STA_SUPPORT
void notify_timer_callback(
		IN PVOID SystemSpecific1,
		IN PVOID FunctionContext,
		IN PVOID SystemSpecific2,
		IN PVOID SystemSpecific3);
#endif
#endif
