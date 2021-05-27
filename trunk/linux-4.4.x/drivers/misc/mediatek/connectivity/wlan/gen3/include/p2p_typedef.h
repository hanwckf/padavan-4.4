/*
* Copyright (C) 2016 MediaTek Inc.
*
* This program is free software: you can redistribute it and/or modify it under the terms of the
* GNU General Public License version 2 as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along with this program.
* If not, see <http://www.gnu.org/licenses/>.
*/

/*
** Id: //Department/DaVinci/BRANCHES/MT6620_WIFI_DRIVER_V2_3/include/p2p_typedef.h#1
*/

/*
 * ! \file   p2p_typedef.h
 *  \brief  Declaration of data type and return values of internal protocol stack.
 *
 *   In this file we declare the data type and return values which will be exported
 *   to all MGMT Protocol Stack.
 */

#ifndef _P2P_TYPEDEF_H
#define _P2P_TYPEDEF_H

#if CFG_ENABLE_WIFI_DIRECT

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

/*
* type definition of pointer to p2p structure
*/
/* typedef struct _GL_P2P_INFO_T   GL_P2P_INFO_T, *P_GL_P2P_INFO_T; */
typedef struct _P2P_INFO_T P2P_INFO_T, *P_P2P_INFO_T;

typedef struct _P2P_FSM_INFO_T P2P_FSM_INFO_T, *P_P2P_FSM_INFO_T;

typedef struct _P2P_DEV_FSM_INFO_T P2P_DEV_FSM_INFO_T, *P_P2P_DEV_FSM_INFO_T;

typedef struct _P2P_ROLE_FSM_INFO_T P2P_ROLE_FSM_INFO_T, *P_P2P_ROLE_FSM_INFO_T;

typedef struct _P2P_CONNECTION_SETTINGS_T P2P_CONNECTION_SETTINGS_T, *P_P2P_CONNECTION_SETTINGS_T;

/* Type definition for function pointer to p2p function*/
typedef BOOLEAN(*P2P_LAUNCH) (P_GLUE_INFO_T prGlueInfo);

typedef BOOLEAN(*P2P_REMOVE) (P_GLUE_INFO_T prGlueInfo, BOOLEAN fgIsWlanLaunched);

typedef BOOLEAN(*KAL_P2P_GET_CIPHER) (IN P_GLUE_INFO_T prGlueInfo);

typedef BOOLEAN(*KAL_P2P_GET_TKIP_CIPHER) (IN P_GLUE_INFO_T prGlueInfo);

typedef BOOLEAN(*KAL_P2P_GET_CCMP_CIPHER) (IN P_GLUE_INFO_T prGlueInfo);

typedef BOOLEAN(*KAL_P2P_GET_WSC_MODE) (IN P_GLUE_INFO_T prGlueInfo);

typedef struct net_device *(*KAL_P2P_GET_DEV_HDLR) (P_GLUE_INFO_T prGlueInfo);

typedef VOID(*KAL_P2P_SET_MULTICAST_WORK_ITEM) (P_GLUE_INFO_T prGlueInfo);

typedef VOID(*P2P_NET_REGISTER) (P_GLUE_INFO_T prGlueInfo);

typedef VOID(*P2P_NET_UNREGISTER) (P_GLUE_INFO_T prGlueInfo);

typedef VOID(*KAL_P2P_UPDATE_ASSOC_INFO) (IN P_GLUE_INFO_T prGlueInfo,
					  IN PUINT_8 pucFrameBody,
					  IN UINT_32 u4FrameBodyLen, IN BOOLEAN fgReassocRequest);

typedef BOOLEAN(*P2P_VALIDATE_AUTH) (IN P_ADAPTER_T prAdapter,
				     IN P_SW_RFB_T prSwRfb, IN PP_STA_RECORD_T pprStaRec, OUT PUINT_16 pu2StatusCode);

typedef BOOLEAN(*P2P_VALIDATE_ASSOC_REQ) (IN P_ADAPTER_T prAdapter,
					  IN P_SW_RFB_T prSwRfb, OUT PUINT_16 pu4ControlFlags);

typedef VOID(*P2P_RUN_EVENT_AAA_TX_FAIL) (IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prStaRec);

typedef BOOLEAN(*P2P_PARSE_CHECK_FOR_P2P_INFO_ELEM) (IN P_ADAPTER_T prAdapter,
						     IN PUINT_8 pucBuf, OUT PUINT_8 pucOuiType);

typedef WLAN_STATUS(*P2P_RUN_EVENT_AAA_COMPLETE) (IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prStaRec);

typedef VOID(*P2P_PROCESS_EVENT_UPDATE_NOA_PARAM) (IN P_ADAPTER_T prAdapter,
						   UINT_8 ucNetTypeIndex,
						   P_EVENT_UPDATE_NOA_PARAMS_T prEventUpdateNoaParam);

typedef VOID(*SCAN_P2P_PROCESS_BEACON_AND_PROBE_RESP) (IN P_ADAPTER_T prAdapter,
						       IN P_SW_RFB_T prSwRfb,
						       IN P_WLAN_STATUS prStatus,
						       IN P_BSS_DESC_T prBssDesc,
						       IN P_WLAN_BEACON_FRAME_T prWlanBeaconFrame);

typedef VOID(*P2P_RX_PUBLIC_ACTION_FRAME) (P_ADAPTER_T prAdapter, IN P_SW_RFB_T prSwRfb);

typedef VOID(*RLM_RSP_GENERATE_OBSS_SCAN_IE) (P_ADAPTER_T prAdapter, P_MSDU_INFO_T prMsduInfo);

typedef VOID(*RLM_UPDATE_BW_BY_CH_LIST_FOR_AP) (P_ADAPTER_T prAdapter, P_BSS_INFO_T prBssInfo);

typedef VOID(*RLM_PROCESS_PUBLIC_ACTION) (P_ADAPTER_T prAdapter, P_SW_RFB_T prSwRfb);

typedef VOID(*RLM_PROCESS_HT_ACTION) (P_ADAPTER_T prAdapter, P_SW_RFB_T prSwRfb);

typedef VOID(*RLM_UPDATE_PARAMS_FOR_AP) (P_ADAPTER_T prAdapter, P_BSS_INFO_T prBssInfo, BOOLEAN fgUpdateBeacon);

typedef VOID(*RLM_HANDLE_OBSS_STATUS_EVENT_PKT) (P_ADAPTER_T prAdapter, P_EVENT_AP_OBSS_STATUS_T prObssStatus);

typedef BOOLEAN(*P2P_FUNC_VALIDATE_PROBE_REQ) (IN P_ADAPTER_T prAdapter,
					       IN P_SW_RFB_T prSwRfb, OUT PUINT_32 pu4ControlFlags);

typedef VOID(*RLM_BSS_INIT_FOR_AP) (P_ADAPTER_T prAdapter, P_BSS_INFO_T prBssInfo);

typedef UINT_32(*P2P_GET_PROB_RSP_IE_TABLE_SIZE) (VOID);

typedef PUINT_8(*P2P_BUILD_REASSOC_REQ_FRAME_COMMON_IES) (IN P_ADAPTER_T prAdapter,
							  IN P_MSDU_INFO_T prMsduInfo, IN PUINT_8 pucBuffer);

typedef VOID(*P2P_FUNC_DISCONNECT) (IN P_ADAPTER_T prAdapter,
				    IN P_STA_RECORD_T prStaRec, IN BOOLEAN fgSendDeauth, IN UINT_16 u2ReasonCode);

typedef VOID(*P2P_FSM_RUN_EVENT_RX_DEAUTH) (IN P_ADAPTER_T prAdapter,
					    IN P_STA_RECORD_T prStaRec, IN P_SW_RFB_T prSwRfb);

typedef VOID(*P2P_FSM_RUN_EVENT_RX_DISASSOC) (IN P_ADAPTER_T prAdapter,
					      IN P_STA_RECORD_T prStaRec, IN P_SW_RFB_T prSwRfb);

typedef BOOLEAN(*P2P_FUN_IS_AP_MODE) (IN P_P2P_FSM_INFO_T prP2pFsmInfo);

typedef VOID(*P2P_FSM_RUN_EVENT_BEACON_TIMEOUT) (IN P_ADAPTER_T prAdapter);

typedef VOID(*P2P_FUNC_STORE_ASSOC_RSP_IE_BUFFER) (IN P_ADAPTER_T prAdapter, IN P_SW_RFB_T prSwRfb);

typedef VOID(*P2P_GENERATE_P2P_IE) (IN P_ADAPTER_T prAdapter, IN P_MSDU_INFO_T prMsduInfo);

typedef UINT_32(*P2P_CALCULATE_P2P_IE_LEN) (IN P_ADAPTER_T prAdapter, IN UINT_8 ucBssIndex, IN P_STA_RECORD_T prStaRec);

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/
#endif /*CFG_ENABLE_WIFI_DIRECT */

#endif /* _P2P_TYPEDEF_H */
