/*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See http://www.gnu.org/licenses/gpl-2.0.html for more details.
*/

#ifndef _ASSOC_H
#define _ASSOC_H

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
*                         D A T A   T Y P E S
********************************************************************************
*/

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
/*----------------------------------------------------------------------------*/
/* Routines in assoc.c                                                        */
/*----------------------------------------------------------------------------*/
UINT_16 assocBuildCapabilityInfo(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prStaRec);

WLAN_STATUS assocSendReAssocReqFrame(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prStaRec);

WLAN_STATUS assocCheckTxReAssocReqFrame(IN P_ADAPTER_T prAdapter, IN P_MSDU_INFO_T prMsduInfo);

WLAN_STATUS assocCheckTxReAssocRespFrame(IN P_ADAPTER_T prAdapter, IN P_MSDU_INFO_T prMsduInfo);

WLAN_STATUS
assocCheckRxReAssocRspFrameStatus(IN P_ADAPTER_T prAdapter, IN P_SW_RFB_T prSwRfb, OUT PUINT_16 pu2StatusCode);

WLAN_STATUS assocSendDisAssocFrame(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prStaRec, IN UINT_16 u2ReasonCode);

WLAN_STATUS
assocProcessRxDisassocFrame(IN P_ADAPTER_T prAdapter,
			    IN P_SW_RFB_T prSwRfb, IN UINT_8 aucBSSID[], OUT PUINT_16 pu2ReasonCode);

WLAN_STATUS assocProcessRxAssocReqFrame(IN P_ADAPTER_T prAdapter, IN P_SW_RFB_T prSwRfb, OUT PUINT_16 pu2StatusCode);

WLAN_STATUS assocSendReAssocRespFrame(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prStaRec);

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

#endif /* _ASSOC_H */
