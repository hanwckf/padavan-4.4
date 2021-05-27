/******************************************************************************
 *
 * This file is provided under a dual license.  When you use or
 * distribute this software, you may choose to be licensed under
 * version 2 of the GNU General Public License ("GPLv2 License")
 * or BSD License.
 *
 * GPLv2 License
 *
 * Copyright(C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 *
 * BSD LICENSE
 *
 * Copyright(C) 2016 MediaTek Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *****************************************************************************/
/*! \file   gl_ate_agent.h
 * \brief  This file includes private ioctl support.
 */

#ifndef _GL_ATE_AGENT_H
#define _GL_ATE_AGENT_H
#if CFG_SUPPORT_QA_TOOL
/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/

extern UINT_32 u4RxStatSeqNum;

#if CFG_SUPPORT_TX_BF
extern PFMU_PROFILE_TAG1 g_rPfmuTag1;
extern PFMU_PROFILE_TAG2 g_rPfmuTag2;
extern PFMU_DATA g_rPfmuData;
#endif
extern BOOLEAN g_bCaptureDone;

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

typedef struct _STA_REC_BF_UPD_ARGUMENT {
	UINT_32 u4WlanId;
	UINT_32 u4BssId;
	UINT_32 u4PfmuId;
	UINT_32 u4SuMu;
	UINT_32 u4eTxBfCap;
	UINT_32 u4NdpaRate;
	UINT_32 u4NdpRate;
	UINT_32 u4ReptPollRate;
	UINT_32 u4TxMode;
	UINT_32 u4Nc;
	UINT_32 u4Nr;
	UINT_32 u4Bw;
	UINT_32 u4SpeIdx;
	UINT_32 u4TotalMemReq;
	UINT_32 u4MemReq20M;
	UINT_32 au4MemRow[4];
	UINT_32 au4MemCol[4];
} STA_REC_BF_UPD_ARGUMENT, *P_STA_REC_BF_UPD_ARGUMENT;

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

int Set_ResetStatCounter_Proc(struct net_device *prNetDev, UINT_8 *prInBuf);
int SetATE(struct net_device *prNetDev, UINT_8 *prInBuf);
int SetATEDa(struct net_device *prNetDev, UINT_8 *prInBuf);
int SetATESa(struct net_device *prNetDev, UINT_8 *prInBuf);
int SetATEChannel(struct net_device *prNetDev, UINT_8 *prInBuf);
int SetATETxPower0(struct net_device *prNetDev, UINT_8 *prInBuf);
int SetATETxGi(struct net_device *prNetDev, UINT_8 *prInBuf);
int SetATETxBw(struct net_device *prNetDev, UINT_8 *prInBuf);
int SetATETxMode(struct net_device *prNetDev, UINT_8 *prInBuf);
int SetATETxLength(struct net_device *prNetDev, UINT_8 *prInBuf);
int SetATETxCount(struct net_device *prNetDev, UINT_8 *prInBuf);
int SetATETxMcs(struct net_device *prNetDev, UINT_8 *prInBuf);
int SetATEIpg(struct net_device *prNetDev, UINT_8 *prInBuf);

#if CFG_SUPPORT_TX_BF
int Set_TxBfProfileTag_Help(struct net_device *prNetDev, UINT_8 *prInBuf);
int Set_TxBfProfileTag_InValid(struct net_device *prNetDev, UINT_8 *prInBuf);
int Set_TxBfProfileTag_PfmuIdx(struct net_device *prNetDev, UINT_8 *prInBuf);
int Set_TxBfProfileTag_BfType(struct net_device *prNetDev, UINT_8 *prInBuf);
int Set_TxBfProfileTag_DBW(struct net_device *prNetDev, UINT_8 *prInBuf);
int Set_TxBfProfileTag_SuMu(struct net_device *prNetDev, UINT_8 *prInBuf);
int Set_TxBfProfileTag_Mem(struct net_device *prNetDev, UINT_8 *prInBuf);
int Set_TxBfProfileTag_Matrix(struct net_device *prNetDev, UINT_8 *prInBuf);
int Set_TxBfProfileTag_SNR(struct net_device *prNetDev, UINT_8 *prInBuf);
int Set_TxBfProfileTag_SmartAnt(struct net_device *prNetDev, UINT_8 *prInBuf);
int Set_TxBfProfileTag_SeIdx(struct net_device *prNetDev, UINT_8 *prInBuf);
int Set_TxBfProfileTag_RmsdThrd(struct net_device *prNetDev, UINT_8 *prInBuf);
int Set_TxBfProfileTag_McsThrd(struct net_device *prNetDev, UINT_8 *prInBuf);
int Set_TxBfProfileTag_TimeOut(struct net_device *prNetDev, UINT_8 *prInBuf);
int Set_TxBfProfileTag_DesiredBW(struct net_device *prNetDev, UINT_8 *prInBuf);
int Set_TxBfProfileTag_DesiredNc(struct net_device *prNetDev, UINT_8 *prInBuf);
int Set_TxBfProfileTag_DesiredNr(struct net_device *prNetDev, UINT_8 *prInBuf);
int Set_TxBfProfileTagRead(struct net_device *prNetDev, UINT_8 *prInBuf);
int Set_TxBfProfileTagWrite(struct net_device *prNetDev, UINT_8 *prInBuf);
int Set_StaRecCmmUpdate(struct net_device *prNetDev, UINT_8 *prInBuf);
int Set_StaRecBfUpdate(struct net_device *prNetDev, UINT_8 *prInBuf);

int Set_DevInfoUpdate(struct net_device *prNetDev, UINT_8 *prInBuf);

int Set_BssInfoUpdate(struct net_device *prNetDev, UINT_8 *prInBuf);
int Set_TxBfProfileDataRead(struct net_device *prNetDev, UINT_8 *prInBuf);
int Set_TxBfProfileDataWrite(struct net_device *prNetDev, UINT_8 *prInBuf);
int Set_Trigger_Sounding_Proc(struct net_device *prNetDev, UINT_8 *prInBuf);
int Set_Stop_Sounding_Proc(struct net_device *prNetDev, UINT_8 *prInBuf);
int Set_TxBfTxApply(struct net_device *prNetDev, UINT_8 *prInBuf);
int Set_TxBfProfilePnRead(struct net_device *prNetDev, UINT_8 *prInBuf);
int Set_TxBfProfilePnWrite(struct net_device *prNetDev, UINT_8 *prInBuf);
int Set_TxBfManualAssoc(struct net_device *prNetDev, UINT_8 *prInBuf);
int Set_TxBfPfmuMemAlloc(struct net_device *prNetDev, UINT_8 *prInBuf);
int Set_TxBfPfmuMemRelease(struct net_device *prNetDev, UINT_8 *prInBuf);

#if CFG_SUPPORT_MU_MIMO
int Set_MUGetInitMCS(struct net_device *prNetDev, UINT_8 *prInBuf);
int Set_MUCalInitMCS(struct net_device *prNetDev, UINT_8 *prInBuf);
int Set_MUCalLQ(struct net_device *prNetDev, UINT_8 *prInBuf);
int Set_MUGetLQ(struct net_device *prNetDev, UINT_8 *prInBuf);
int Set_MUSetSNROffset(struct net_device *prNetDev, UINT_8 *prInBuf);
int Set_MUSetZeroNss(struct net_device *prNetDev, UINT_8 *prInBuf);
int Set_MUSetSpeedUpLQ(struct net_device *prNetDev, UINT_8 *prInBuf);
int Set_MUSetMUTable(struct net_device *prNetDev, UINT_8 *prInBuf);
int Set_MUSetGroup(struct net_device *prNetDev, UINT_8 *prInBuf);
int Set_MUGetQD(struct net_device *prNetDev, UINT_8 *prInBuf);
int Set_MUSetEnable(struct net_device *prNetDev, UINT_8 *prInBuf);
int Set_MUSetGID_UP(struct net_device *prNetDev, UINT_8 *prInBuf);
int Set_MUTriggerTx(struct net_device *prNetDev, UINT_8 *prInBuf);
#endif
#endif


int WriteEfuse(struct net_device *prNetDev, UINT_8 *prInBuf);


int AteCmdSetHandle(struct net_device *prNetDev, UINT_8 *prInBuf, UINT_32 u4InBufLen);
#endif /*CFG_SUPPORT_QA_TOOL */
#endif /* _GL_ATE_AGENT_H */
