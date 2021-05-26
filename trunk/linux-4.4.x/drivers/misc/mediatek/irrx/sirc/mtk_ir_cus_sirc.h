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
#ifndef __MTK_IR_CUS_SIRC_DEFINE_H__
#define __MTK_IR_CUS_SIRC_DEFINE_H__

#include "mtk_ir_core.h"

#ifdef MTK_LK_IRRX_SUPPORT	/* platform/mt8127/lk/rule.mk */
#include <platform/mtk_ir_lk_core.h>
#else
/* #include <media/rc-map.h> */
#endif

/************************************
* for BD     (8/3MHZ) * MTK_SIRC_SAPERIOD
* for 8127 (1/32KHZ)*MTK_SIRC_SAPERIOD
************************************/

#define MTK_SIRC_CONFIG      (IRRX_CH_END_15 + IRRX_CH_HWIR + IRRX_CH_ORDINV)
#define MTK_SIRC_SAPERIOD    (0x006) /*0x00F limit IR pulse decode*/
#define MTK_SIRC_THRESHOLD   (0x602) /*bit[12:8]for  deglitch  200/31.25=6*/

#define MTK_SIRC_EXP_POWE_KEY1  0x00000000
#define MTK_SIRC_EXP_POWE_KEY2  0x00000000

#define SIRC_LENGTH_12 (0x0c)/*12*/
#define SIRC_LENGTH_15 (0x0f)/*15*/
#define SIRC_LENGTH_20 (0x14)/*20*/

/*define of customer code(also called category code)*/
#define SIRC_CUSTOMER_12BIT  (0x10)
#define SIRC_CUSTOMER_15BIT  (0x90)
#define SIRC_CUSTOMER_20BIT  (0x73a)
#define SIRC_CUSTOMER_20BIT_DUAL  (0x410)
#define SIRC_CUSTOMER_20BIT_TRIBLE  (0xc10)
#define SIRC_KEY_CODE(LENGTH, CUSTOM, DATA) (((LENGTH<<24)&0xff000000) | ((CUSTOM<<8)&0xffff00) | ((DATA)&0xff))

#ifdef MTK_LK_IRRX_SUPPORT

/* this table is using in lk,  for lk boot_menu select */
static struct mtk_ir_lk_msg mtk_sirc_lk_table[] = {
	{0x78, KEY_UP},
	{0x79, KEY_DOWN},
	{0x7c, KEY_ENTER},
};

#else	/*MTK_LK_IRRX_SUPPORT */

/* this table is used in factory mode, for factory_mode menu select */
static struct rc_map_table mtk_sirc_factory_table[] = {
	{SIRC_KEY_CODE(SIRC_LENGTH_20, SIRC_CUSTOMER_20BIT, 0x78), KEY_VOLUMEUP},
	{SIRC_KEY_CODE(SIRC_LENGTH_20, SIRC_CUSTOMER_20BIT, 0x79), KEY_VOLUMEDOWN},
	{SIRC_KEY_CODE(SIRC_LENGTH_20, SIRC_CUSTOMER_20BIT, 0x7c), KEY_POWER},

};

/**
* When MTK_IR_SUPPORT_MOUSE_INPUT set to 1, and then if IR receive key value MTK_IR_MOUSE_SIRC_SWITCH_CODE,
* it will switch to Mouse input mode. If IR receive key value MTK_IR_MOUSE_SIRC_SWITCH_CODE again at Mouse input
* mode, it will switch to IR input mode.
* If you don't need Mouse input mode, please set
* MTK_IR_MOUSE_MODE_DEFAULT = MTK_IR_AS_IRRX
* MTK_IR_SUPPORT_MOUSE_INPUT = 0
**/

#define MTK_IR_SIRC_CUSTOMER_CODE SIRC_CUSTOMER_20BIT	/* here is sirc's customer code */
#define MTK_IR_SIRC_KEYPRESS_TIMEOUT	140
#define MTK_IR_MOUSE_MODE_DEFAULT	MTK_IR_AS_IRRX	/* MTK_IR_AS_IRRX or MTK_IR_AS_MOUSE */
#define MTK_IR_SUPPORT_MOUSE_INPUT	0		/* Support mouse input, 0:no support 1:support */
#define MTK_IR_MOUSE_SIRC_SWITCH_CODE 0x68	/* The switch code for IR input and Mouse input */
#define MOUSE_SMALL_X_STEP 10
#define MOUSE_SMALL_Y_STEP 10
#define MOUSE_LARGE_X_STEP 30
#define MOUSE_LARGE_Y_STEP 30

/* this table is used in normal mode, for normal_boot */
static struct rc_map_table mtk_sirc_table[] = {
	{SIRC_KEY_CODE(SIRC_LENGTH_12, SIRC_CUSTOMER_12BIT, 0x12), KEY_VOLUMEUP},
	{SIRC_KEY_CODE(SIRC_LENGTH_12, SIRC_CUSTOMER_12BIT, 0x13), KEY_VOLUMEDOWN},
	{SIRC_KEY_CODE(SIRC_LENGTH_12, SIRC_CUSTOMER_12BIT, 0x15), KEY_POWER},
	{SIRC_KEY_CODE(SIRC_LENGTH_15, SIRC_CUSTOMER_15BIT, 0x69), KEY_1},
	{SIRC_KEY_CODE(SIRC_LENGTH_15, SIRC_CUSTOMER_15BIT, 0x6a), KEY_1},
	{SIRC_KEY_CODE(SIRC_LENGTH_20, SIRC_CUSTOMER_20BIT, 0x00), KEY_1},
	{SIRC_KEY_CODE(SIRC_LENGTH_20, SIRC_CUSTOMER_20BIT, 0x01), KEY_2},
	{SIRC_KEY_CODE(SIRC_LENGTH_20, SIRC_CUSTOMER_20BIT, 0x02), KEY_3},
	{SIRC_KEY_CODE(SIRC_LENGTH_20, SIRC_CUSTOMER_20BIT, 0x03), KEY_4},
	{SIRC_KEY_CODE(SIRC_LENGTH_20, SIRC_CUSTOMER_20BIT, 0x04), KEY_5},
	{SIRC_KEY_CODE(SIRC_LENGTH_20, SIRC_CUSTOMER_20BIT, 0x05), KEY_6},
	{SIRC_KEY_CODE(SIRC_LENGTH_20, SIRC_CUSTOMER_20BIT, 0x06), KEY_7},
	{SIRC_KEY_CODE(SIRC_LENGTH_20, SIRC_CUSTOMER_20BIT, 0x07), KEY_8},
	{SIRC_KEY_CODE(SIRC_LENGTH_20, SIRC_CUSTOMER_20BIT, 0x08), KEY_9},
	{SIRC_KEY_CODE(SIRC_LENGTH_20, SIRC_CUSTOMER_20BIT, 0x09), KEY_0},
	{SIRC_KEY_CODE(SIRC_LENGTH_20, SIRC_CUSTOMER_20BIT, 0x32), KEY_PLAYCD},
	{SIRC_KEY_CODE(SIRC_LENGTH_20, SIRC_CUSTOMER_20BIT, 0x39), KEY_PAUSECD},
	{SIRC_KEY_CODE(SIRC_LENGTH_20, SIRC_CUSTOMER_20BIT, 0x38), KEY_STOPCD},
	{SIRC_KEY_CODE(SIRC_LENGTH_20, SIRC_CUSTOMER_20BIT, 0x7a), KEY_LEFT},
	{SIRC_KEY_CODE(SIRC_LENGTH_20, SIRC_CUSTOMER_20BIT, 0x7b), KEY_RIGHT},
	{SIRC_KEY_CODE(SIRC_LENGTH_20, SIRC_CUSTOMER_20BIT, 0x7c), KEY_ENTER},
	{SIRC_KEY_CODE(SIRC_LENGTH_20, SIRC_CUSTOMER_20BIT, 0x19), KEY_HOMEPAGE},
	{SIRC_KEY_CODE(SIRC_LENGTH_20, SIRC_CUSTOMER_20BIT, 0x7d), KEY_BACK},
	{SIRC_KEY_CODE(SIRC_LENGTH_20, SIRC_CUSTOMER_20BIT_DUAL, 0x29), KEY_SEARCH},
	{SIRC_KEY_CODE(SIRC_LENGTH_20, SIRC_CUSTOMER_20BIT_TRIBLE, 0x7d), KEY_SPORT},
	/*#if MTK_IRRX_AS_MOUSE_INPUT*/
		{0xffff, KEY_HELP},/*be carefule this key is used to send,but no response*/
	/*#endif*/
};

#endif	/*MTK_LK_IRRX_SUPPORT */
#endif
