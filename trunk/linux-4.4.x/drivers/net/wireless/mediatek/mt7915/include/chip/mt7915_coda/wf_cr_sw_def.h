/*
** $Id: @(#) wf_cr_sw_def.h $
*/

/*! \file   "wf_cr_sw_def.h"
    \brief
*/

/*******************************************************************************
* Copyright (c) 2009 MediaTek Inc.
*
* All rights reserved. Copying, compilation, modification, distribution
* or any other use whatsoever of this material is strictly prohibited
* except in accordance with a Software License Agreement with
* MediaTek Inc.
********************************************************************************
*/

/*******************************************************************************
* LEGAL DISCLAIMER
*
* BY OPENING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND
* AGREES THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK
* SOFTWARE") RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE
* PROVIDED TO BUYER ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY
* DISCLAIMS ANY AND ALL WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT
* LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
* PARTICULAR PURPOSE OR NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE
* ANY WARRANTY WHATSOEVER WITH RESPECT TO THE SOFTWARE OF ANY THIRD PARTY
* WHICH MAY BE USED BY, INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK
* SOFTWARE, AND BUYER AGREES TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY
* WARRANTY CLAIM RELATING THERETO. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE
* FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO BUYER'S SPECIFICATION OR TO
* CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
*
* BUYER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE
* LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL
* BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT
* ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY
* BUYER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
*
* THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE
* WITH THE LAWS OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT
* OF LAWS PRINCIPLES.  ANY DISPUTES, CONTROVERSIES OR CLAIMS ARISING
* THEREOF AND RELATED THERETO SHALL BE SETTLED BY ARBITRATION IN SAN
* FRANCISCO, CA, UNDER THE RULES OF THE INTERNATIONAL CHAMBER OF COMMERCE
* (ICC).
********************************************************************************
*/

#ifndef _WF_CR_SW_DEF_H
#define _WF_CR_SW_DEF_H

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/


//****************************************************************************
//
//                     MCU_SYSRAM SW CR Definitions
//
//****************************************************************************
#define WF_SW_DEF_CR_BASE                0x0041F200

#define WF_SW_DEF_CR_WACPU_STAT_ADDR            (WF_SW_DEF_CR_BASE + 0x000) /* F200 */
#define WF_SW_DEF_CR_WACPU_SLEEP_STAT_ADDR      (WF_SW_DEF_CR_BASE + 0x004) /* F204 */
#define WF_SW_DEF_CR_WM2WA_ACTION_ADDR          (WF_SW_DEF_CR_BASE + 0x008) /* F208 */
#define WF_SW_DEF_CR_WA2WM_ACTION_ADDR          (WF_SW_DEF_CR_BASE + 0x00C) /* F20C */
#define WF_SW_DEF_CR_LP_DBG0_ADDR               (WF_SW_DEF_CR_BASE + 0x010) /* F210 */
#define WF_SW_DEF_CR_LP_DBG1_ADDR               (WF_SW_DEF_CR_BASE + 0x014) /* F214 */
#define WF_SW_DEF_CR_SER_STATUS_ADDR            (WF_SW_DEF_CR_BASE + 0x040) /* F240 */
#define WF_SW_DEF_CR_PLE_STATUS_ADDR            (WF_SW_DEF_CR_BASE + 0x044) /* F244 */
#define WF_SW_DEF_CR_PLE1_STATUS_ADDR           (WF_SW_DEF_CR_BASE + 0x048) /* F248 */
#define WF_SW_DEF_CR_PLE_AMSDU_STATUS_ADDR      (WF_SW_DEF_CR_BASE + 0x04C) /* F24C */
#define WF_SW_DEF_CR_PSE_STATUS_ADDR            (WF_SW_DEF_CR_BASE + 0x050) /* F250 */
#define WF_SW_DEF_CR_PSE1_STATUS_ADDR           (WF_SW_DEF_CR_BASE + 0x054) /* F254 */
#define WF_SW_DEF_CR_LAMC_WISR6_BN0_STATUS_ADDR (WF_SW_DEF_CR_BASE + 0x058) /* F258 */
#define WF_SW_DEF_CR_LAMC_WISR6_BN1_STATUS_ADDR (WF_SW_DEF_CR_BASE + 0x05C) /* F25C */
#define WF_SW_DEF_CR_LAMC_WISR7_BN0_STATUS_ADDR (WF_SW_DEF_CR_BASE + 0x060) /* F260 */
#define WF_SW_DEF_CR_LAMC_WISR7_BN1_STATUS_ADDR (WF_SW_DEF_CR_BASE + 0x064) /* F264 */
#define WF_SW_DEF_CR_USB_MCU_EVENT_ADD          (WF_SW_DEF_CR_BASE + 0x070) /* F270 */
#define WF_SW_DEF_CR_USB_HOST_ACK_ADDR          (WF_SW_DEF_CR_BASE + 0x074) /* F274 */

#define WF_SW_DEF_CR_RSVD_DBG_0                 (WF_SW_DEF_CR_BASE + 0x0A0) /* F2A0 */
#define WF_SW_DEF_CR_RSVD_DBG_1                 (WF_SW_DEF_CR_BASE + 0x0A4) /* F2A4 */
#define WF_SW_DEF_CR_RSVD_DBG_2                 (WF_SW_DEF_CR_BASE + 0x0A8) /* F2A8 */
#define WF_SW_DEF_CR_RSVD_DBG_3                 (WF_SW_DEF_CR_BASE + 0x0AC) /* F2AC */
#define WF_SW_DEF_CR_RSVD_DBG_4                 (WF_SW_DEF_CR_BASE + 0x0B0) /* F2B0 */
#define WF_SW_DEF_CR_RSVD_DBG_5                 (WF_SW_DEF_CR_BASE + 0x0B4) /* F2B4 */
#define WF_SW_DEF_CR_RSVD_DBG_6                 (WF_SW_DEF_CR_BASE + 0x0B8) /* F2B8 */
#define WF_SW_DEF_CR_RSVD_DBG_7                 (WF_SW_DEF_CR_BASE + 0x0BC) /* F2BC */
#define WF_SW_DEF_CR_RSVD_DBG_8                 (WF_SW_DEF_CR_BASE + 0x0C0) /* F2C0 */
#define WF_SW_DEF_CR_RSVD_DBG_9                 (WF_SW_DEF_CR_BASE + 0x0C4) /* F2C4 */


/* =====================================================================================

  ---WF_SW_DEF_CR_WACPU_SLEEP_STAT_ADDR (0x0041F200 + 0x004)---

    SLEEP_STATUS[0]          - (RW) 0: Awake, 1: sleep
    GATING_STATUS[1]           - (RW) 0:Idle, 1: Gating
    RESERVED5[31..2]             - (RO) Reserved bits

 =====================================================================================*/
#define WF_SW_DEF_CR_WACPU_SLEEP_STAT_SLEEP_ADDR    WF_SW_DEF_CR_WACPU_SLEEP_STAT_ADDR
#define WF_SW_DEF_CR_WACPU_SLEEP_STAT_SLEEP_MASK    0x00000001
#define WF_SW_DEF_CR_WACPU_SLEEP_STAT_SLEEP_SHFT    0
#define WF_SW_DEF_CR_WACPU_SLEEP_STAT_GATING_ADDR   WF_SW_DEF_CR_WACPU_SLEEP_STAT_ADDR
#define WF_SW_DEF_CR_WACPU_SLEEP_STAT_GATING_MASK   0x00000002
#define WF_SW_DEF_CR_WACPU_SLEEP_STAT_GATING_SHFT   1

#endif /* _WF_CR_SW_DEF_H */


