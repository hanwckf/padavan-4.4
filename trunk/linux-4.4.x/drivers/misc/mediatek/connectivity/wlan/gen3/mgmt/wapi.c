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
** Id: //Department/DaVinci/BRANCHES/MT6620_WIFI_DRIVER_V2_3/mgmt/wapi.c#1
*/

/*
 * ! \file   "wapi.c"
 * \brief  This file including the WAPI related function.
 *
 *  This file provided the macros and functions library support the wapi ie parsing,
 *  cipher and AKM check to help the AP seleced deciding.
 */

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/

#include "precomp.h"
#if CFG_SUPPORT_WAPI

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/

/*******************************************************************************
*                             D A T A   T Y P E S
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
*                   F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

/*----------------------------------------------------------------------------*/
/*!
*
* \brief This routine is called to generate WPA IE for
*        associate request frame.
*
* \param[in]  prCurrentBss     The Selected BSS description
*
* \retval The append WPA IE length
*
* \note
*      Called by: AIS module, Associate request
*/
/*----------------------------------------------------------------------------*/
VOID wapiGenerateWAPIIE(IN P_ADAPTER_T prAdapter, IN P_MSDU_INFO_T prMsduInfo)
{
	PUINT_8 pucBuffer;

	ASSERT(prAdapter);
	ASSERT(prMsduInfo);

	if (prMsduInfo->ucBssIndex != prAdapter->prAisBssInfo->ucBssIndex)
		return;

	pucBuffer = (PUINT_8) ((ULONG) prMsduInfo->prPacket + (ULONG) prMsduInfo->u2FrameLength);

	/* ASSOC INFO IE ID: 68 :0x44 */
	if (/* prWlanInfo->fgWapiMode && */ prAdapter->prGlueInfo->u2WapiAssocInfoIESz) {
		kalMemCopy(pucBuffer, &prAdapter->prGlueInfo->aucWapiAssocInfoIEs,
			   prAdapter->prGlueInfo->u2WapiAssocInfoIESz);
		prMsduInfo->u2FrameLength += prAdapter->prGlueInfo->u2WapiAssocInfoIESz;
	}

}

/*----------------------------------------------------------------------------*/
/*!
* \brief This routine is called to parse WAPI IE.
*
* \param[in]  prInfoElem Pointer to the RSN IE
* \param[out] prRsnInfo Pointer to the BSSDescription structure to store the
**                  WAPI information from the given WAPI IE
*
* \retval TRUE - Succeeded
* \retval FALSE - Failed
*/
/*----------------------------------------------------------------------------*/
BOOLEAN wapiParseWapiIE(IN P_WAPI_INFO_ELEM_T prInfoElem, OUT P_WAPI_INFO_T prWapiInfo)
{
	UINT_32 i;
	INT_32 u4RemainWapiIeLen;
	UINT_16 u2Version;
	UINT_16 u2Cap = 0;
	UINT_32 u4GroupSuite = WAPI_CIPHER_SUITE_WPI;
	UINT_16 u2PairSuiteCount = 0;
	UINT_16 u2AuthSuiteCount = 0;
	PUCHAR pucPairSuite = NULL;
	PUCHAR pucAuthSuite = NULL;
	PUCHAR cp;

	DEBUGFUNC("wapiParseWapiIE");

	ASSERT(prInfoElem);
	ASSERT(prWapiInfo);

	/* Verify the length of the WAPI IE. */
	if (prInfoElem->ucLength < 6) {
		DBGLOG(SEC, TRACE, "WAPI IE length too short (length=%d)\n", prInfoElem->ucLength);
		return FALSE;
	}

	/* Check WAPI version: currently, we only support version 1. */
	WLAN_GET_FIELD_16(&prInfoElem->u2Version, &u2Version);
	if (u2Version != 1) {
		DBGLOG(SEC, TRACE, "Unsupported WAPI IE version: %d\n", u2Version);
		return FALSE;
	}

	cp = (PUCHAR) &prInfoElem->u2AuthKeyMgtSuiteCount;
	u4RemainWapiIeLen = (INT_32) prInfoElem->ucLength - 2;

	do {
		if (u4RemainWapiIeLen == 0)
			break;

		/*
		 * AuthCount    : 2
		 * AuthSuite    : 4 * authSuiteCount
		 * PairwiseCount: 2
		 * PairwiseSuite: 4 * pairSuiteCount
		 * GroupSuite   : 4
		 * Cap          : 2
		 */

		/*
		 * Parse the Authentication and Key Management Cipher Suite Count
		 * field.
		 */
		if (u4RemainWapiIeLen < 2) {
			DBGLOG(SEC, TRACE,
			       "Fail to parse WAPI IE in auth & key mgt suite count (IE len: %d)\n",
				prInfoElem->ucLength);
			return FALSE;
		}

		WLAN_GET_FIELD_16(cp, &u2AuthSuiteCount);
		cp += 2;
		u4RemainWapiIeLen -= 2;

		/*
		 * Parse the Authentication and Key Management Cipher Suite List
		 * field.
		 */
		i = (UINT_32) u2AuthSuiteCount * 4;
		if (u4RemainWapiIeLen < (INT_32) i) {
			DBGLOG(SEC, TRACE,
			       "Fail to parse WAPI IE in auth & key mgt suite list (IE len: %d)\n",
				prInfoElem->ucLength);
			return FALSE;
		}

		pucAuthSuite = cp;

		cp += i;
		u4RemainWapiIeLen -= (INT_32) i;

		if (u4RemainWapiIeLen == 0)
			break;

		/* Parse the Pairwise Key Cipher Suite Count field. */
		if (u4RemainWapiIeLen < 2) {
			DBGLOG(SEC, TRACE,
			       "Fail to parse WAPI IE in pairwise cipher suite count (IE len: %d)\n",
				prInfoElem->ucLength);
			return FALSE;
		}

		WLAN_GET_FIELD_16(cp, &u2PairSuiteCount);
		cp += 2;
		u4RemainWapiIeLen -= 2;

		/* Parse the Pairwise Key Cipher Suite List field. */
		i = (UINT_32) u2PairSuiteCount * 4;
		if (u4RemainWapiIeLen < (INT_32) i) {
			DBGLOG(SEC, TRACE,
			       "Fail to parse WAPI IE in pairwise cipher suite list (IE len: %d)\n",
				prInfoElem->ucLength);
			return FALSE;
		}

		pucPairSuite = cp;

		cp += i;
		u4RemainWapiIeLen -= (INT_32) i;

		/* Parse the Group Key Cipher Suite field. */
		if (u4RemainWapiIeLen < 4) {
			DBGLOG(SEC, TRACE,
			       "Fail to parse WAPI IE in group cipher suite (IE len: %d)\n", prInfoElem->ucLength);
			return FALSE;
		}

		WLAN_GET_FIELD_32(cp, &u4GroupSuite);
		cp += 4;
		u4RemainWapiIeLen -= 4;

		/* Parse the WAPI u2Capabilities field. */
		if (u4RemainWapiIeLen < 2) {
			DBGLOG(SEC, TRACE,
			       "Fail to parse WAPI IE in WAPI capabilities (IE len: %d)\n", prInfoElem->ucLength);
			return FALSE;
		}

		WLAN_GET_FIELD_16(cp, &u2Cap);
		u4RemainWapiIeLen -= 2;

		/* Todo:: BKID support */
	} while (FALSE);

	/* Save the WAPI information for the BSS. */

	prWapiInfo->ucElemId = ELEM_ID_WAPI;

	prWapiInfo->u2Version = u2Version;

	prWapiInfo->u4GroupKeyCipherSuite = u4GroupSuite;

	DBGLOG(SEC, LOUD, "WAPI: version %d, group key cipher suite %02x-%02x-%02x-%02x\n",
			   u2Version, (UCHAR) (u4GroupSuite & 0x000000FF),
			   (UCHAR) ((u4GroupSuite >> 8) & 0x000000FF),
			   (UCHAR) ((u4GroupSuite >> 16) & 0x000000FF), (UCHAR) ((u4GroupSuite >> 24) & 0x000000FF));

	if (pucPairSuite) {
		/* The information about the pairwise key cipher suites is present. */
		if (u2PairSuiteCount > MAX_NUM_SUPPORTED_WAPI_CIPHER_SUITES)
			u2PairSuiteCount = MAX_NUM_SUPPORTED_WAPI_CIPHER_SUITES;

		prWapiInfo->u4PairwiseKeyCipherSuiteCount = (UINT_32) u2PairSuiteCount;

		for (i = 0; i < (UINT_32) u2PairSuiteCount; i++) {
			WLAN_GET_FIELD_32(pucPairSuite, &prWapiInfo->au4PairwiseKeyCipherSuite[i]);
			pucPairSuite += 4;

			DBGLOG(SEC, LOUD,
			       "WAPI: pairwise key cipher suite [%d]: %02x-%02x-%02x-%02x\n",
				(UINT_8) i,
				(UCHAR) (prWapiInfo->au4PairwiseKeyCipherSuite[i] & 0x000000FF),
				(UCHAR) ((prWapiInfo->au4PairwiseKeyCipherSuite[i] >> 8) & 0x000000FF),
				(UCHAR) ((prWapiInfo->au4PairwiseKeyCipherSuite[i] >> 16) & 0x000000FF),
				(UCHAR) ((prWapiInfo->au4PairwiseKeyCipherSuite[i] >> 24) & 0x000000FF));
		}
	} else {
		/*
		 * The information about the pairwise key cipher suites is not present.
		 * Use the default chipher suite for WAPI: WPI.
		 */
		prWapiInfo->u4PairwiseKeyCipherSuiteCount = 1;
		prWapiInfo->au4PairwiseKeyCipherSuite[0] = WAPI_CIPHER_SUITE_WPI;

		DBGLOG(SEC, LOUD,
		       "WAPI: pairwise key cipher suite: %02x-%02x-%02x-%02x (default)\n",
			(UCHAR) (prWapiInfo->au4PairwiseKeyCipherSuite[0] & 0x000000FF),
			(UCHAR) ((prWapiInfo->au4PairwiseKeyCipherSuite[0] >> 8) & 0x000000FF),
			(UCHAR) ((prWapiInfo->au4PairwiseKeyCipherSuite[0] >> 16) & 0x000000FF),
			(UCHAR) ((prWapiInfo->au4PairwiseKeyCipherSuite[0] >> 24) & 0x000000FF));
	}

	if (pucAuthSuite) {
		/*
		 * The information about the authentication and key management suites
		 * is present.
		 */
		if (u2AuthSuiteCount > MAX_NUM_SUPPORTED_WAPI_AKM_SUITES)
			u2AuthSuiteCount = MAX_NUM_SUPPORTED_WAPI_AKM_SUITES;

		prWapiInfo->u4AuthKeyMgtSuiteCount = (UINT_32) u2AuthSuiteCount;

		for (i = 0; i < (UINT_32) u2AuthSuiteCount; i++) {
			WLAN_GET_FIELD_32(pucAuthSuite, &prWapiInfo->au4AuthKeyMgtSuite[i]);
			pucAuthSuite += 4;

			DBGLOG(SEC, LOUD, "WAPI: AKM suite [%d]: %02x-%02x-%02x-%02x\n",
					   (UINT_8) i,
					   (UCHAR) (prWapiInfo->au4AuthKeyMgtSuite[i] & 0x000000FF),
					   (UCHAR) ((prWapiInfo->au4AuthKeyMgtSuite[i] >> 8) & 0x000000FF),
					   (UCHAR) ((prWapiInfo->au4AuthKeyMgtSuite[i] >> 16) & 0x000000FF),
					   (UCHAR) ((prWapiInfo->au4AuthKeyMgtSuite[i] >> 24) & 0x000000FF));
		}
	} else {
		/*
		 * The information about the authentication and key management suites
		 * is not present. Use the default AKM suite for WAPI.
		 */
		prWapiInfo->u4AuthKeyMgtSuiteCount = 1;
		prWapiInfo->au4AuthKeyMgtSuite[0] = WAPI_AKM_SUITE_802_1X;

		DBGLOG(SEC, LOUD, "WAPI: AKM suite: %02x-%02x-%02x-%02x (default)\n",
				   (UCHAR) (prWapiInfo->au4AuthKeyMgtSuite[0] & 0x000000FF),
				   (UCHAR) ((prWapiInfo->au4AuthKeyMgtSuite[0] >> 8) & 0x000000FF),
				   (UCHAR) ((prWapiInfo->au4AuthKeyMgtSuite[0] >> 16) & 0x000000FF),
				   (UCHAR) ((prWapiInfo->au4AuthKeyMgtSuite[0] >> 24) & 0x000000FF));
	}

	prWapiInfo->u2WapiCap = u2Cap;
	DBGLOG(SEC, LOUD, "WAPI: cap: 0x%04x\n", prWapiInfo->u2WapiCap);

	return TRUE;
}				/* wapiParseWapiIE */

/*----------------------------------------------------------------------------*/
/*!
* \brief This routine is called to perform WAPI policy selection for a given BSS.
*
* \param[in]  prAdapter Pointer to the adapter object data area.
* \param[in]  prBss Pointer to the BSS description
*
* \retval TRUE - The WAPI policy selection for the given BSS is
*                successful. The selected pairwise and group cipher suites
*                are returned in the BSS description.
* \retval FALSE - The WAPI policy selection for the given BSS is failed.
*                 The driver shall not attempt to join the given BSS.
*
* \note The Encrypt status matched score will save to bss for final ap select.
*/
/*----------------------------------------------------------------------------*/
BOOLEAN wapiPerformPolicySelection(IN P_ADAPTER_T prAdapter, IN P_BSS_DESC_T prBss)
{
	UINT_32 i;
	UINT_32 u4PairwiseCipher = 0;
	UINT_32 u4GroupCipher = 0;
	UINT_32 u4AkmSuite = 0;
	P_WAPI_INFO_T prBssWapiInfo;
	P_WLAN_INFO_T prWlanInfo;

	DEBUGFUNC("wapiPerformPolicySelection");

	ASSERT(prBss);

	/* Notice!!!! WAPI AP not set the privacy bit for WAI and WAI-PSK at WZC configuration mode */
	prWlanInfo = &prAdapter->rWlanInfo;

	if (prBss->fgIEWAPI) {
		prBssWapiInfo = &prBss->rIEWAPI;
	} else {
		if (prAdapter->rWifiVar.rConnSettings.fgWapiMode == FALSE) {
			DBGLOG(SEC, TRACE, "-- No Protected BSS\n");
		} else {
			DBGLOG(SEC, TRACE, "WAPI Information Element does not exist.\n");
			return FALSE;
		}
		return TRUE;
	}

	/* Select pairwise/group ciphers */
	for (i = 0; i < prBssWapiInfo->u4PairwiseKeyCipherSuiteCount; i++) {
		if (prBssWapiInfo->au4PairwiseKeyCipherSuite[i] ==
		    prAdapter->rWifiVar.rConnSettings.u4WapiSelectedPairwiseCipher) {
			u4PairwiseCipher = prBssWapiInfo->au4PairwiseKeyCipherSuite[i];
		}
	}
	if (prBssWapiInfo->u4GroupKeyCipherSuite == prAdapter->rWifiVar.rConnSettings.u4WapiSelectedGroupCipher)
		u4GroupCipher = prBssWapiInfo->u4GroupKeyCipherSuite;

	/* Exception handler */
	/*
	 * If we cannot find proper pairwise and group cipher suites to join the
	 * BSS, do not check the supported AKM suites.
	 */
	if (u4PairwiseCipher == 0 || u4GroupCipher == 0) {
		DBGLOG(SEC, TRACE, "Failed to select pairwise/group cipher (0x%08lx/0x%08lx)\n",
				    u4PairwiseCipher, u4GroupCipher);
		return FALSE;
	}

	/* Select AKM */
	/*
	 * If the driver cannot support any authentication suites advertised in
	 * the given BSS, we fail to perform RSNA policy selection.
	 */
	/* Attempt to find any overlapping supported AKM suite. */
	for (i = 0; i < prBssWapiInfo->u4AuthKeyMgtSuiteCount; i++) {
		if (prBssWapiInfo->au4AuthKeyMgtSuite[i] == prAdapter->rWifiVar.rConnSettings.u4WapiSelectedAKMSuite) {
			u4AkmSuite = prBssWapiInfo->au4AuthKeyMgtSuite[i];
			break;
		}
	}

	if (u4AkmSuite == 0) {
		DBGLOG(SEC, TRACE, "Cannot support any AKM suites\n");
		return FALSE;
	}

	DBGLOG(SEC, TRACE,
	       "Selected pairwise/group cipher: %02x-%02x-%02x-%02x/%02x-%02x-%02x-%02x\n",
		(UINT_8) (u4PairwiseCipher & 0x000000FF),
		(UINT_8) ((u4PairwiseCipher >> 8) & 0x000000FF),
		(UINT_8) ((u4PairwiseCipher >> 16) & 0x000000FF),
		(UINT_8) ((u4PairwiseCipher >> 24) & 0x000000FF),
		(UINT_8) (u4GroupCipher & 0x000000FF), (UINT_8) ((u4GroupCipher >> 8) & 0x000000FF),
		(UINT_8) ((u4GroupCipher >> 16) & 0x000000FF), (UINT_8) ((u4GroupCipher >> 24) & 0x000000FF));

	DBGLOG(SEC, TRACE, "Selected AKM suite: %02x-%02x-%02x-%02x\n",
			    (UINT_8) (u4AkmSuite & 0x000000FF),
			    (UINT_8) ((u4AkmSuite >> 8) & 0x000000FF),
			    (UINT_8) ((u4AkmSuite >> 16) & 0x000000FF), (UINT_8) ((u4AkmSuite >> 24) & 0x000000FF));

	return TRUE;
}				/* wapiPerformPolicySelection */

#if 0
/*----------------------------------------------------------------------------*/
/*!
* \brief This routine is use for wapi mode, to update the current wpi tx idx ? 0 :1 .
*
* \param[in]  prStaRec Pointer to the Sta record
* \param[out] ucWlanIdx The Rx status->wlanidx field
*
* \retval TRUE - Succeeded
* \retval FALSE - Failed
*/
/*----------------------------------------------------------------------------*/
BOOLEAN wapiUpdateTxKeyIdx(IN P_STA_RECORD_T prStaRec, IN UINT_8 ucWlanIdx)
{
	UINT_8 ucKeyId;

	if ((ucWlanIdx & BITS(0, 3)) == CIPHER_SUITE_WPI) {

		ucKeyId = ((ucWlanIdx & BITS(4, 5)) >> 4);

		if (ucKeyId != g_prWifiVar->rAisSpecificBssInfo.ucWpiActivedPWKey) {
			DBGLOG(RSN, STATE,
			       "Change wapi key index from %d->%d\n",
				g_prWifiVar->rAisSpecificBssInfo.ucWpiActivedPWKey, ucKeyId);
			g_prWifiVar->rAisSpecificBssInfo.ucWpiActivedPWKey = ucKeyId;

			prStaRec->ucWlanIndex =
			    (ucKeyId ==
			     WTBL_AIS_BSSID_WAPI_IDX_0) ? WTBL_AIS_BSSID_WAPI_IDX_0 : WTBL_AIS_BSSID_WAPI_IDX_1;
		}
	}
}
#endif
#endif
