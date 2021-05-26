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
#ifndef __MTK_IR_CUS_RC5_DEFINE_H__
#define __MTK_IR_CUS_RC5_DEFINE_H__

#include "mtk_ir_core.h"

#ifdef MTK_LK_IRRX_SUPPORT	/* platform/mt8127/lk/rule.mk */
#include <platform/mtk_ir_lk_core.h>
#else
/* #include <media/rc-map.h> */
#endif

/************************************
* for BD     (8/3MHZ) * MTK_RC5_SAPERIOD
* for 8127 (1/32KHZ)*MTK_RC5_SAPERIOD
************************************/
#define MTK_RC5_CONFIG		(0x44001)
#define MTK_RC5_SAPERIOD	(0x2000)
#define MTK_RC5_THRESHOLD	(0x600)

#ifdef MTK_LK_IRRX_SUPPORT

/* this table is using in lk,  for lk boot_menu select */
static struct mtk_ir_lk_msg mtk_rc5_lk_table[] = {
	{0x6615, KEY_POWER},
	{0x6617, KEY_POWER},
};

#else	/*MTK_LK_IRRX_SUPPORT */

/* this table is used in factory mode, for factory_mode menu select */
static struct rc_map_table mtk_rc5_factory_table[] = {
	{0x6615, KEY_POWER},
	{0x6617, KEY_POWER},
};

#define MTK_IR_RC5_CUSTOMER_CODE  0x46 /* here is RC5's customer code */
#define MTK_IR_RC5_KEYPRESS_TIMEOUT 140

/**
* When MTK_IR_SUPPORT_MOUSE_INPUT set to 1, and then if IR receive key value MTK_IR_MOUSE_RC5_SWITCH_CODE,
* it will switch to Mouse input mode. If IR receive key value MTK_IR_MOUSE_RC5_SWITCH_CODE again at Mouse input
* mode, it will switch to IR input mode.
* If you don't need Mouse input mode, please set
* MTK_IR_MOUSE_MODE_DEFAULT = MTK_IR_AS_IRRX
* MTK_IR_SUPPORT_MOUSE_INPUT = 0
**/
#define MTK_IR_MOUSE_MODE_DEFAULT	MTK_IR_AS_IRRX	/* MTK_IR_AS_IRRX or MTK_IR_AS_MOUSE */
#define MTK_IR_SUPPORT_MOUSE_INPUT	0		/* Support mouse input, 0:no support 1:support */
#define MTK_IR_MOUSE_RC5_SWITCH_CODE 0x70	/* The switch code for IR input and Mouse input */
#define MOUSE_SMALL_X_STEP 10
#define MOUSE_SMALL_Y_STEP 10
#define MOUSE_LARGE_X_STEP 30
#define MOUSE_LARGE_Y_STEP 30

/* this table is used in normal mode, for normal_boot */
static struct rc_map_table mtk_rc5_table[] = {
	{0x6015, KEY_0},
	{0x6017, KEY_0},
	{0x7015, KEY_1},
	{0x7017, KEY_1},
	{0x6815, KEY_2},
	{0x6817, KEY_2},
	{0x7815, KEY_3},
	{0x7817, KEY_3},
	{0x6415, KEY_4},
	{0x6417, KEY_4},
	{0x7415, KEY_5},
	{0x7417, KEY_5},
	{0x6C15, KEY_6},
	{0x6C17, KEY_6},
	{0x7C15, KEY_7},
	{0x7C17, KEY_7},
	{0x6215, KEY_8},
	{0x6217, KEY_8},
	{0x7215, KEY_9},
	{0x7217, KEY_9},
	{0x7595, KEY_PLAY},
	{0x7597, KEY_PLAY},
	{0x6d95, KEY_STOP},
	{0x6d97, KEY_STOP},
	{0x6195, KEY_PAUSE},
	{0x6197, KEY_PAUSE},
	{0x6095, KEY_FORWARD},
	{0x6097, KEY_FORWARD},
	{0x7095, KEY_BACK},
	{0x7097, KEY_BACK},
	{0x6615, KEY_POWER},
	{0x6617, KEY_POWER},

	{0x64ad, KEY_PROGRAM},
	{0x64af, KEY_PROGRAM},
	{0x672d, KEY_SHUFFLE},
	{0x672f, KEY_SHUFFLE},
};
#endif	/*MTK_LK_IRRX_SUPPORT */
#endif
