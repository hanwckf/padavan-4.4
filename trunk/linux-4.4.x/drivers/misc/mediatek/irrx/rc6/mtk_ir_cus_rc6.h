/**
* Copyright (C) 2016 MediaTek Inc.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See http://www.gnu.org/licenses/gpl-2.0.html for more details.
**/
#ifndef __MTK_IR_CUS_RC6_DEFINE_H__
#define __MTK_IR_CUS_RC6_DEFINE_H__

#include "mtk_ir_core.h"

#ifdef MTK_LK_IRRX_SUPPORT	/* platform/mt8127/lk/rule.mk */
#include <platform/mtk_ir_lk_core.h>
#else
/* #include <media/rc-map.h> */
#endif

/************************************
* for BD     (8/3MHZ) * MTK_RC6_SAPERIOD
* for 8127 (1/32KHZ)*MTK_RC6_SAPERIOD
************************************/
#define MTK_RC6_CONFIG   (IRRX_CH_END_15 | IRRX_CH_IGSYN | IRRX_CH_HWIR  | IRRX_CH_ORDINV | IRRX_CH_RC5)
#define MTK_RC6_SAPERIOD    (0xe)
#define MTK_RC6_THRESHOLD   (0x1)


#define MTK_RC6_EXP_POWE_KEY1	0x00000000
#define MTK_RC6_EXP_POWE_KEY2	0x00000000


#ifdef MTK_LK_IRRX_SUPPORT

/* this table is using in lk,  for lk boot_menu select */
static struct mtk_ir_lk_msg mtk_rc6_lk_table[] = {
	{0x58, KEY_UP},
	{0x59, KEY_DOWN},
	{0x5c, KEY_ENTER},
};

#else	/*MTK_LK_IRRX_SUPPORT */

/* this table is used in factory mode, for factory_mode menu select */
static struct rc_map_table mtk_rc6_factory_table[] = {
	{0x58, KEY_VOLUMEUP},
	{0x59, KEY_VOLUMEDOWN},
	{0x5c, KEY_POWER},
};

#define MTK_IR_RC6_CUSTOMER_CODE  0x46 /* here is RC6's customer code */
#define MTK_IR_RC6_KEYPRESS_TIMEOUT 140

/**
* When MTK_IR_SUPPORT_MOUSE_INPUT set to 1, and then if IR receive key value MTK_IR_MOUSE_RC6_SWITCH_CODE,
* it will switch to Mouse input mode. If IR receive key value MTK_IR_MOUSE_RC6_SWITCH_CODE again at Mouse input
* mode, it will switch to IR input mode.
* If you don't need Mouse input mode, please set
* MTK_IR_MOUSE_MODE_DEFAULT = MTK_IR_AS_IRRX
* MTK_IR_SUPPORT_MOUSE_INPUT = 0
**/
#define MTK_IR_MOUSE_MODE_DEFAULT	MTK_IR_AS_IRRX	/* MTK_IR_AS_IRRX or MTK_IR_AS_MOUSE */
#define MTK_IR_SUPPORT_MOUSE_INPUT	0		/* Support mouse input, 0:no support 1:support */
#define MTK_IR_MOUSE_RC6_SWITCH_CODE 0x70	/* The switch code for IR input and Mouse input */
#define MOUSE_SMALL_X_STEP 10
#define MOUSE_SMALL_Y_STEP 10
#define MOUSE_LARGE_X_STEP 30
#define MOUSE_LARGE_Y_STEP 30

/* this table is used in normal mode, for normal_boot */
static struct rc_map_table mtk_rc6_table[] = {
	{0x00, KEY_0},
	{0x01, KEY_1},
	{0x02, KEY_2},
	{0x03, KEY_3},
	{0x04, KEY_4},
	{0x05, KEY_5},
	{0x06, KEY_6},
	{0x07, KEY_7},
	{0x08, KEY_8},
	{0x09, KEY_9},

	{0x5a, KEY_LEFT},
	{0x5b, KEY_RIGHT},
	{0x58, KEY_UP},
	{0x59, KEY_DOWN},
	{0x5c, KEY_ENTER},

	{0xc7, KEY_POWER},
	{0x4c, KEY_HOMEPAGE},
	{0x83, KEY_BACKSPACE},
	{0x92, KEY_BACK},
	/*#if MTK_IRRX_AS_MOUSE_INPUT*/
	/*KEY_HELP: be carefule this key is used to send to known
	*whether key was repeated or released,but no response
	*/
	{0xffff, KEY_HELP},
	/*#endif*/
};

#endif	/*MTK_LK_IRRX_SUPPORT */
#endif
