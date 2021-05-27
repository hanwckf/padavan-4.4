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

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/
#include "precomp.h"

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
static VOID rlmFillHtCapIE(P_ADAPTER_T prAdapter, P_BSS_INFO_T prBssInfo, P_MSDU_INFO_T prMsduInfo);

static VOID rlmFillExtCapIE(P_ADAPTER_T prAdapter, P_BSS_INFO_T prBssInfo, P_MSDU_INFO_T prMsduInfo);

static VOID rlmFillHtOpIE(P_ADAPTER_T prAdapter, P_BSS_INFO_T prBssInfo, P_MSDU_INFO_T prMsduInfo);

static UINT_8 rlmRecIeInfoForClient(P_ADAPTER_T prAdapter, P_BSS_INFO_T prBssInfo, PUINT_8 pucIE, UINT_16 u2IELength);

static BOOLEAN
rlmRecBcnFromNeighborForClient(P_ADAPTER_T prAdapter,
			       P_BSS_INFO_T prBssInfo, P_SW_RFB_T prSwRfb, PUINT_8 pucIE, UINT_16 u2IELength);

static BOOLEAN
rlmRecBcnInfoForClient(P_ADAPTER_T prAdapter,
		       P_BSS_INFO_T prBssInfo, P_SW_RFB_T prSwRfb, PUINT_8 pucIE, UINT_16 u2IELength);

static VOID rlmBssReset(P_ADAPTER_T prAdapter, P_BSS_INFO_T prBssInfo);

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

/*----------------------------------------------------------------------------*/
/*!
* \brief
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID rlmFsmEventInit(P_ADAPTER_T prAdapter)
{
	ASSERT(prAdapter);

	/* Note: assume TIMER_T structures are reset to zero or stopped
	 * before invoking this function.
	 */

	/* Initialize OBSS FSM */
	rlmObssInit(prAdapter);
#if CFG_SUPPORT_PWR_LIMIT_COUNTRY
	rlmDomainCheckCountryPowerLimitTable(prAdapter);
#endif
}

/*----------------------------------------------------------------------------*/
/*!
* \brief
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID rlmFsmEventUninit(P_ADAPTER_T prAdapter)
{
	P_BSS_INFO_T prBssInfo;
	UINT_8 ucNetIdx;

	ASSERT(prAdapter);

	RLM_NET_FOR_EACH(ucNetIdx) {
		prBssInfo = &prAdapter->rWifiVar.arBssInfo[ucNetIdx];
		ASSERT(prBssInfo);

		/* Note: all RLM timers will also be stopped.
		 *       Now only one OBSS scan timer.
		 */
		rlmBssReset(prAdapter, prBssInfo);
	}
}

/*----------------------------------------------------------------------------*/
/*!
* \brief For probe request, association request
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID rlmReqGenerateHtCapIE(P_ADAPTER_T prAdapter, P_MSDU_INFO_T prMsduInfo)
{
	P_BSS_INFO_T prBssInfo;
	P_STA_RECORD_T prStaRec;

	ASSERT(prAdapter);
	ASSERT(prMsduInfo);

	prBssInfo = &prAdapter->rWifiVar.arBssInfo[prMsduInfo->ucNetworkType];
	ASSERT(prBssInfo);

	prStaRec = cnmGetStaRecByIndex(prAdapter, prMsduInfo->ucStaRecIndex);

	if ((prAdapter->rWifiVar.ucAvailablePhyTypeSet & PHY_TYPE_SET_802_11N) &&
	    (!prStaRec || (prStaRec->ucPhyTypeSet & PHY_TYPE_SET_802_11N)))
		rlmFillHtCapIE(prAdapter, prBssInfo, prMsduInfo);
}

/*----------------------------------------------------------------------------*/
/*!
* \brief For probe request, association request
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID rlmReqGenerateExtCapIE(P_ADAPTER_T prAdapter, P_MSDU_INFO_T prMsduInfo)
{
	P_BSS_INFO_T prBssInfo;
	P_STA_RECORD_T prStaRec;

	ASSERT(prAdapter);
	ASSERT(prMsduInfo);

	prBssInfo = &prAdapter->rWifiVar.arBssInfo[prMsduInfo->ucNetworkType];
	ASSERT(prBssInfo);

	prStaRec = cnmGetStaRecByIndex(prAdapter, prMsduInfo->ucStaRecIndex);

	if ((prAdapter->rWifiVar.ucAvailablePhyTypeSet & PHY_TYPE_SET_802_11N) &&
	    (!prStaRec || (prStaRec->ucPhyTypeSet & PHY_TYPE_SET_802_11N)))
		rlmFillExtCapIE(prAdapter, prBssInfo, prMsduInfo);
#if CFG_SUPPORT_HOTSPOT_2_0
	else if (prAdapter->prGlueInfo->fgConnectHS20AP == TRUE)
		hs20FillExtCapIE(prAdapter, prBssInfo, prMsduInfo);
#endif
}

/*----------------------------------------------------------------------------*/
/*!
* \brief For probe response (GO, IBSS) and association response
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID rlmRspGenerateHtCapIE(P_ADAPTER_T prAdapter, P_MSDU_INFO_T prMsduInfo)
{
	P_BSS_INFO_T prBssInfo;
	P_STA_RECORD_T prStaRec;

	ASSERT(prAdapter);
	ASSERT(prMsduInfo);
	ASSERT(IS_NET_ACTIVE(prAdapter, prMsduInfo->ucNetworkType));

	prBssInfo = &prAdapter->rWifiVar.arBssInfo[prMsduInfo->ucNetworkType];
	ASSERT(prBssInfo);

	prStaRec = cnmGetStaRecByIndex(prAdapter, prMsduInfo->ucStaRecIndex);

	if (RLM_NET_IS_11N(prBssInfo) && (!prStaRec || (prStaRec->ucPhyTypeSet & PHY_TYPE_SET_802_11N)))
		rlmFillHtCapIE(prAdapter, prBssInfo, prMsduInfo);
}

/*----------------------------------------------------------------------------*/
/*!
* \brief For probe response (GO, IBSS) and association response
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID rlmRspGenerateExtCapIE(P_ADAPTER_T prAdapter, P_MSDU_INFO_T prMsduInfo)
{
	P_BSS_INFO_T prBssInfo;
	P_STA_RECORD_T prStaRec;

	ASSERT(prAdapter);
	ASSERT(prMsduInfo);
	ASSERT(IS_NET_ACTIVE(prAdapter, prMsduInfo->ucNetworkType));

	prBssInfo = &prAdapter->rWifiVar.arBssInfo[prMsduInfo->ucNetworkType];
	ASSERT(prBssInfo);

	prStaRec = cnmGetStaRecByIndex(prAdapter, prMsduInfo->ucStaRecIndex);

	if (RLM_NET_IS_11N(prBssInfo) && (!prStaRec || (prStaRec->ucPhyTypeSet & PHY_TYPE_SET_802_11N)))
		rlmFillExtCapIE(prAdapter, prBssInfo, prMsduInfo);
}

/*----------------------------------------------------------------------------*/
/*!
* \brief For probe response (GO, IBSS) and association response
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID rlmRspGenerateHtOpIE(P_ADAPTER_T prAdapter, P_MSDU_INFO_T prMsduInfo)
{
	P_BSS_INFO_T prBssInfo;
	P_STA_RECORD_T prStaRec;

	ASSERT(prAdapter);
	ASSERT(prMsduInfo);
	ASSERT(IS_NET_ACTIVE(prAdapter, prMsduInfo->ucNetworkType));

	prStaRec = cnmGetStaRecByIndex(prAdapter, prMsduInfo->ucStaRecIndex);

	prBssInfo = &prAdapter->rWifiVar.arBssInfo[prMsduInfo->ucNetworkType];
	ASSERT(prBssInfo);

	if (RLM_NET_IS_11N(prBssInfo) && (!prStaRec || (prStaRec->ucPhyTypeSet & PHY_TYPE_SET_802_11N)))
		rlmFillHtOpIE(prAdapter, prBssInfo, prMsduInfo);
}

/*----------------------------------------------------------------------------*/
/*!
* \brief For probe response (GO, IBSS) and association response
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID rlmRspGenerateErpIE(P_ADAPTER_T prAdapter, P_MSDU_INFO_T prMsduInfo)
{
	P_BSS_INFO_T prBssInfo;
	P_STA_RECORD_T prStaRec;
	P_IE_ERP_T prErpIe;

	ASSERT(prAdapter);
	ASSERT(prMsduInfo);
	ASSERT(IS_NET_ACTIVE(prAdapter, prMsduInfo->ucNetworkType));

	prStaRec = cnmGetStaRecByIndex(prAdapter, prMsduInfo->ucStaRecIndex);

	prBssInfo = &prAdapter->rWifiVar.arBssInfo[prMsduInfo->ucNetworkType];
	ASSERT(prBssInfo);

	if (RLM_NET_IS_11GN(prBssInfo) && prBssInfo->eBand == BAND_2G4 &&
	    (!prStaRec || (prStaRec->ucPhyTypeSet & PHY_TYPE_SET_802_11GN))) {
		prErpIe = (P_IE_ERP_T)
		    (((PUINT_8) prMsduInfo->prPacket) + prMsduInfo->u2FrameLength);

		/* Add ERP IE */
		prErpIe->ucId = ELEM_ID_ERP_INFO;
		prErpIe->ucLength = 1;

		prErpIe->ucERP = prBssInfo->fgObssErpProtectMode ? ERP_INFO_USE_PROTECTION : 0;

		if (prBssInfo->fgErpProtectMode)
			prErpIe->ucERP |= (ERP_INFO_NON_ERP_PRESENT | ERP_INFO_USE_PROTECTION);

		/* Handle barker preamble */
		if (!prBssInfo->fgUseShortPreamble)
			prErpIe->ucERP |= ERP_INFO_BARKER_PREAMBLE_MODE;

		ASSERT(IE_SIZE(prErpIe) <= (ELEM_HDR_LEN + ELEM_MAX_LEN_ERP));

		prMsduInfo->u2FrameLength += IE_SIZE(prErpIe);
	}
}

/*----------------------------------------------------------------------------*/
/*!
* \brief
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
UINT32
rlmFillHtCapIEByParams(BOOLEAN fg40mAllowed,
		       BOOLEAN fgShortGIDisabled,
		       UINT_8 u8SupportRxSgi20,
		       UINT_8 u8SupportRxSgi40,
		       UINT_8 u8SupportRxGf, UINT_8 u8SupportRxSTBC, ENUM_OP_MODE_T eCurrentOPMode, UINT_8 *pOutBuf)
{
	P_IE_HT_CAP_T prHtCap;
	P_SUP_MCS_SET_FIELD prSupMcsSet;

	ASSERT(pOutBuf);

	prHtCap = (P_IE_HT_CAP_T) pOutBuf;

	/* Add HT capabilities IE */
	prHtCap->ucId = ELEM_ID_HT_CAP;
	prHtCap->ucLength = sizeof(IE_HT_CAP_T) - ELEM_HDR_LEN;

	prHtCap->u2HtCapInfo = HT_CAP_INFO_DEFAULT_VAL;
	if (!fg40mAllowed) {
		prHtCap->u2HtCapInfo &= ~(HT_CAP_INFO_SUP_CHNL_WIDTH |
					  HT_CAP_INFO_SHORT_GI_40M | HT_CAP_INFO_DSSS_CCK_IN_40M);
	}
	if (fgShortGIDisabled)
		prHtCap->u2HtCapInfo &= ~(HT_CAP_INFO_SHORT_GI_20M | HT_CAP_INFO_SHORT_GI_40M);

	if (u8SupportRxSgi20 == 2)
		prHtCap->u2HtCapInfo &= ~(HT_CAP_INFO_SHORT_GI_20M);
	if (u8SupportRxSgi40 == 2)
		prHtCap->u2HtCapInfo &= ~(HT_CAP_INFO_SHORT_GI_40M);
	if (u8SupportRxGf == 2)
		prHtCap->u2HtCapInfo &= ~(HT_CAP_INFO_HT_GF);
	if (u8SupportRxSTBC == 2)
		prHtCap->u2HtCapInfo &= ~(HT_CAP_INFO_RX_STBC_1_SS);
	prHtCap->ucAmpduParam = AMPDU_PARAM_DEFAULT_VAL;

	prSupMcsSet = &prHtCap->rSupMcsSet;
	kalMemZero((PVOID)&prSupMcsSet->aucRxMcsBitmask[0], SUP_MCS_RX_BITMASK_OCTET_NUM);

	prSupMcsSet->aucRxMcsBitmask[0] = BITS(0, 7);

	if (fg40mAllowed)
		prSupMcsSet->aucRxMcsBitmask[32 / 8] = BIT(0);	/* MCS32 */
	prSupMcsSet->u2RxHighestSupportedRate = SUP_MCS_RX_DEFAULT_HIGHEST_RATE;
	prSupMcsSet->u4TxRateInfo = SUP_MCS_TX_DEFAULT_VAL;

	prHtCap->u2HtExtendedCap = HT_EXT_CAP_DEFAULT_VAL;
	if (!fg40mAllowed || eCurrentOPMode != OP_MODE_INFRASTRUCTURE)
		prHtCap->u2HtExtendedCap &= ~(HT_EXT_CAP_PCO | HT_EXT_CAP_PCO_TRANS_TIME_NONE);

	prHtCap->u4TxBeamformingCap = TX_BEAMFORMING_CAP_DEFAULT_VAL;

	prHtCap->ucAselCap = ASEL_CAP_DEFAULT_VAL;

	ASSERT(IE_SIZE(prHtCap) <= (ELEM_HDR_LEN + ELEM_MAX_LEN_HT_CAP));

	return IE_SIZE(prHtCap);
}

/*----------------------------------------------------------------------------*/
/*!
* \brief
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
static VOID rlmFillHtCapIE(P_ADAPTER_T prAdapter, P_BSS_INFO_T prBssInfo, P_MSDU_INFO_T prMsduInfo)
{
	P_IE_HT_CAP_T prHtCap;
/* P_SUP_MCS_SET_FIELD     prSupMcsSet; */
	BOOLEAN fg40mAllowed;

	ASSERT(prAdapter);
	ASSERT(prBssInfo);
	ASSERT(prMsduInfo);

	fg40mAllowed = prBssInfo->fgAssoc40mBwAllowed;

	prHtCap = (P_IE_HT_CAP_T)
	    (((PUINT_8) prMsduInfo->prPacket) + prMsduInfo->u2FrameLength);

#if 0
	/* Add HT capabilities IE */
	prHtCap->ucId = ELEM_ID_HT_CAP;
	prHtCap->ucLength = sizeof(IE_HT_CAP_T) - ELEM_HDR_LEN;

	prHtCap->u2HtCapInfo = HT_CAP_INFO_DEFAULT_VAL;
	if (!fg40mAllowed)
		prHtCap->u2HtCapInfo &= ~(HT_CAP_INFO_SUP_CHNL_WIDTH |
					  HT_CAP_INFO_SHORT_GI_40M | HT_CAP_INFO_DSSS_CCK_IN_40M);
	if (prAdapter->rWifiVar.rConnSettings.fgRxShortGIDisabled)
		prHtCap->u2HtCapInfo &= ~(HT_CAP_INFO_SHORT_GI_20M | HT_CAP_INFO_SHORT_GI_40M);

	if (prAdapter->rWifiVar.u8SupportRxSgi20 == 2)
		prHtCap->u2HtCapInfo &= ~(HT_CAP_INFO_SHORT_GI_20M);
	if (prAdapter->rWifiVar.u8SupportRxSgi40 == 2)
		prHtCap->u2HtCapInfo &= ~(HT_CAP_INFO_SHORT_GI_40M);
	if (prAdapter->rWifiVar.u8SupportRxGf == 2)
		prHtCap->u2HtCapInfo &= ~(HT_CAP_INFO_HT_GF);

	prHtCap->ucAmpduParam = AMPDU_PARAM_DEFAULT_VAL;

	prSupMcsSet = &prHtCap->rSupMcsSet;
	kalMemZero((PVOID)&prSupMcsSet->aucRxMcsBitmask[0], SUP_MCS_RX_BITMASK_OCTET_NUM);

	prSupMcsSet->aucRxMcsBitmask[0] = BITS(0, 7);

	if (fg40mAllowed)
		prSupMcsSet->aucRxMcsBitmask[32 / 8] = BIT(0);	/* MCS32 */
	prSupMcsSet->u2RxHighestSupportedRate = SUP_MCS_RX_DEFAULT_HIGHEST_RATE;
	prSupMcsSet->u4TxRateInfo = SUP_MCS_TX_DEFAULT_VAL;

	prHtCap->u2HtExtendedCap = HT_EXT_CAP_DEFAULT_VAL;
	if (!fg40mAllowed || prBssInfo->eCurrentOPMode != OP_MODE_INFRASTRUCTURE)
		prHtCap->u2HtExtendedCap &= ~(HT_EXT_CAP_PCO | HT_EXT_CAP_PCO_TRANS_TIME_NONE);

	prHtCap->u4TxBeamformingCap = TX_BEAMFORMING_CAP_DEFAULT_VAL;

	prHtCap->ucAselCap = ASEL_CAP_DEFAULT_VAL;

	ASSERT(IE_SIZE(prHtCap) <= (ELEM_HDR_LEN + ELEM_MAX_LEN_HT_CAP));

	prMsduInfo->u2FrameLength += IE_SIZE(prHtCap);
#else

	prMsduInfo->u2FrameLength += rlmFillHtCapIEByParams(fg40mAllowed,
							    prAdapter->rWifiVar.rConnSettings.fgRxShortGIDisabled,
							    prAdapter->rWifiVar.u8SupportRxSgi20,
							    prAdapter->rWifiVar.u8SupportRxSgi40,
							    prAdapter->rWifiVar.u8SupportRxGf,
							    prAdapter->rWifiVar.u8SupportRxSTBC,
							    prBssInfo->eCurrentOPMode, (UINT_8 *) prHtCap);
#endif
}

/*----------------------------------------------------------------------------*/
/*!
* \brief
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
static VOID rlmFillExtCapIE(P_ADAPTER_T prAdapter, P_BSS_INFO_T prBssInfo, P_MSDU_INFO_T prMsduInfo)
{
#if CFG_SUPPORT_HOTSPOT_2_0
	P_HS20_EXT_CAP_T prHsExtCap;
#else
	P_EXT_CAP_T prExtCap;
#endif
	BOOLEAN fg40mAllowed;

	ASSERT(prAdapter);
	ASSERT(prMsduInfo);

	fg40mAllowed = prBssInfo->fgAssoc40mBwAllowed;

#if CFG_SUPPORT_HOTSPOT_2_0
	prHsExtCap = (P_HS20_EXT_CAP_T)
	    (((PUINT_8) prMsduInfo->prPacket) + prMsduInfo->u2FrameLength);
	prHsExtCap->ucId = ELEM_ID_EXTENDED_CAP;

	if (prAdapter->prGlueInfo->fgConnectHS20AP == TRUE)
		prHsExtCap->ucLength = ELEM_MAX_LEN_EXT_CAP;
	else
		prHsExtCap->ucLength = 3 - ELEM_HDR_LEN;

	kalMemZero(prHsExtCap->aucCapabilities, sizeof(prHsExtCap->aucCapabilities));

	prHsExtCap->aucCapabilities[0] = ELEM_EXT_CAP_DEFAULT_VAL;

	if (!fg40mAllowed)
		prHsExtCap->aucCapabilities[0] &= ~ELEM_EXT_CAP_20_40_COEXIST_SUPPORT;

	if (prBssInfo->eCurrentOPMode != OP_MODE_INFRASTRUCTURE)
		prHsExtCap->aucCapabilities[0] &= ~ELEM_EXT_CAP_PSMP_CAP;

	if (prAdapter->prGlueInfo->fgConnectHS20AP == TRUE) {
		SET_EXT_CAP(prHsExtCap->aucCapabilities, ELEM_MAX_LEN_EXT_CAP, ELEM_EXT_CAP_INTERWORKING_BIT);
		SET_EXT_CAP(prHsExtCap->aucCapabilities, ELEM_MAX_LEN_EXT_CAP, ELEM_EXT_CAP_QOSMAPSET_BIT);

		/* For R2 WNM-Notification */
		SET_EXT_CAP(prHsExtCap->aucCapabilities, ELEM_MAX_LEN_EXT_CAP, ELEM_EXT_CAP_WNM_NOTIFICATION_BIT);
	}

	ASSERT(IE_SIZE(prHsExtCap) <= (ELEM_HDR_LEN + ELEM_MAX_LEN_EXT_CAP));

	prMsduInfo->u2FrameLength += IE_SIZE(prHsExtCap);

#else
	/* Add Extended Capabilities IE */
	prExtCap = (P_EXT_CAP_T)
	    (((PUINT_8) prMsduInfo->prPacket) + prMsduInfo->u2FrameLength);

	prExtCap->ucId = ELEM_ID_EXTENDED_CAP;

	prExtCap->ucLength = 3 - ELEM_HDR_LEN;
	kalMemZero(prExtCap->aucCapabilities, sizeof(prExtCap->aucCapabilities));

	prExtCap->aucCapabilities[0] = ELEM_EXT_CAP_DEFAULT_VAL;

	if (!fg40mAllowed)
		prExtCap->aucCapabilities[0] &= ~ELEM_EXT_CAP_20_40_COEXIST_SUPPORT;

	if (prBssInfo->eCurrentOPMode != OP_MODE_INFRASTRUCTURE)
		prExtCap->aucCapabilities[0] &= ~ELEM_EXT_CAP_PSMP_CAP;

	ASSERT(IE_SIZE(prExtCap) <= (ELEM_HDR_LEN + ELEM_MAX_LEN_EXT_CAP));

	prMsduInfo->u2FrameLength += IE_SIZE(prExtCap);
#endif
}

/*----------------------------------------------------------------------------*/
/*!
* \brief
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
UINT32 rlmFillHtOpIeBody(P_BSS_INFO_T prBssInfo, UINT_8 *pFme)
{
	P_IE_HT_OP_T prHtOp;
	UINT_16 i;

	prHtOp = (P_IE_HT_OP_T) pFme;

	/* Add HT operation IE */
	prHtOp->ucId = ELEM_ID_HT_OP;
	prHtOp->ucLength = sizeof(IE_HT_OP_T) - ELEM_HDR_LEN;

	/* RIFS and 20/40 bandwidth operations are included */
	prHtOp->ucPrimaryChannel = prBssInfo->ucPrimaryChannel;
	prHtOp->ucInfo1 = prBssInfo->ucHtOpInfo1;

	/* Decide HT protection mode field */
	if (prBssInfo->eHtProtectMode == HT_PROTECT_MODE_NON_HT)
		prHtOp->u2Info2 = (UINT_8) HT_PROTECT_MODE_NON_HT;
	else if (prBssInfo->eObssHtProtectMode == HT_PROTECT_MODE_NON_MEMBER)
		prHtOp->u2Info2 = (UINT_8) HT_PROTECT_MODE_NON_MEMBER;
	else {
		/* It may be SYS_PROTECT_MODE_NONE or SYS_PROTECT_MODE_20M */
		prHtOp->u2Info2 = (UINT_8) prBssInfo->eHtProtectMode;
	}

	if (prBssInfo->eGfOperationMode != GF_MODE_NORMAL) {
		/* It may be GF_MODE_PROTECT or GF_MODE_DISALLOWED
		 * Note: it will also be set in ad-hoc network
		 */
		prHtOp->u2Info2 |= HT_OP_INFO2_NON_GF_HT_STA_PRESENT;
	}

	if (0 /* Regulatory class 16 */  &&
	    prBssInfo->eObssHtProtectMode == HT_PROTECT_MODE_NON_MEMBER) {
		/* (TBD) It is HT_PROTECT_MODE_NON_MEMBER, so require protection
		 * although it is possible to have no protection by spec.
		 */
		prHtOp->u2Info2 |= HT_OP_INFO2_OBSS_NON_HT_STA_PRESENT;
	}

	prHtOp->u2Info3 = prBssInfo->u2HtOpInfo3;	/* To do: handle L-SIG TXOP */

	/* No basic MCSx are needed temporarily */
	for (i = 0; i < 16; i++)
		prHtOp->aucBasicMcsSet[i] = 0;

	return sizeof(IE_HT_OP_T);
}

static VOID rlmFillHtOpIE(P_ADAPTER_T prAdapter, P_BSS_INFO_T prBssInfo, P_MSDU_INFO_T prMsduInfo)
{
/* P_IE_HT_OP_T        prHtOp; */
/* UINT_16             i; */

	ASSERT(prAdapter);
	ASSERT(prBssInfo);
	ASSERT(prMsduInfo);

	prMsduInfo->u2FrameLength += rlmFillHtOpIeBody(prBssInfo,
						       (((PUINT_8) prMsduInfo->prPacket) + prMsduInfo->u2FrameLength));
#if 0
	prHtOp = (P_IE_HT_OP_T)
	    (((PUINT_8) prMsduInfo->prPacket) + prMsduInfo->u2FrameLength);

	/* Add HT operation IE */
	prHtOp->ucId = ELEM_ID_HT_OP;
	prHtOp->ucLength = sizeof(IE_HT_OP_T) - ELEM_HDR_LEN;

	/* RIFS and 20/40 bandwidth operations are included */
	prHtOp->ucPrimaryChannel = prBssInfo->ucPrimaryChannel;
	prHtOp->ucInfo1 = prBssInfo->ucHtOpInfo1;

	/* Decide HT protection mode field */
	if (prBssInfo->eHtProtectMode == HT_PROTECT_MODE_NON_HT)
		prHtOp->u2Info2 = (UINT_8) HT_PROTECT_MODE_NON_HT;
	else if (prBssInfo->eObssHtProtectMode == HT_PROTECT_MODE_NON_MEMBER)
		prHtOp->u2Info2 = (UINT_8) HT_PROTECT_MODE_NON_MEMBER;
	else {
		/* It may be SYS_PROTECT_MODE_NONE or SYS_PROTECT_MODE_20M */
		prHtOp->u2Info2 = (UINT_8) prBssInfo->eHtProtectMode;
	}

	if (prBssInfo->eGfOperationMode != GF_MODE_NORMAL) {
		/* It may be GF_MODE_PROTECT or GF_MODE_DISALLOWED
		 * Note: it will also be set in ad-hoc network
		 */
		prHtOp->u2Info2 |= HT_OP_INFO2_NON_GF_HT_STA_PRESENT;
	}

	if (0 /* Regulatory class 16 */  &&
	    prBssInfo->eObssHtProtectMode == HT_PROTECT_MODE_NON_MEMBER) {
		/* (TBD) It is HT_PROTECT_MODE_NON_MEMBER, so require protection
		 * although it is possible to have no protection by spec.
		 */
		prHtOp->u2Info2 |= HT_OP_INFO2_OBSS_NON_HT_STA_PRESENT;
	}

	prHtOp->u2Info3 = prBssInfo->u2HtOpInfo3;	/* To do: handle L-SIG TXOP */

	/* No basic MCSx are needed temporarily */
	for (i = 0; i < 16; i++)
		prHtOp->aucBasicMcsSet[i] = 0;

	ASSERT(IE_SIZE(prHtOp) <= (ELEM_HDR_LEN + ELEM_MAX_LEN_HT_OP));

	prMsduInfo->u2FrameLength += IE_SIZE(prHtOp);
#endif
}

/*----------------------------------------------------------------------------*/
/*!
* \brief This function should be invoked to update parameters of associated AP.
*        (Association response and Beacon)
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
static UINT_8 rlmRecIeInfoForClient(P_ADAPTER_T prAdapter, P_BSS_INFO_T prBssInfo, PUINT_8 pucIE, UINT_16 u2IELength)
{
	UINT_16 u2Offset;
	P_STA_RECORD_T prStaRec;
	P_IE_HT_CAP_T prHtCap;
	P_IE_HT_OP_T prHtOp;
	P_IE_OBSS_SCAN_PARAM_T prObssScnParam;
	UINT_8 ucERP, ucPrimaryChannel;
#if CFG_SUPPORT_QUIET && 0
	BOOLEAN fgHasQuietIE = FALSE;
#endif

	ASSERT(prAdapter);
	ASSERT(prBssInfo);
	ASSERT(pucIE);

	prStaRec = prBssInfo->prStaRecOfAP;
	ASSERT(prStaRec);
	if (!prStaRec)
		return 0;

	prBssInfo->fgUseShortPreamble = prBssInfo->fgIsShortPreambleAllowed;
	ucPrimaryChannel = 0;
	prObssScnParam = NULL;

	/* Note: HT-related members in staRec may not be zero before, so
	 *       if following IE does not exist, they are still not zero.
	 *       These HT-related parameters are valid only when the corresponding
	 *       BssInfo supports 802.11n, i.e., RLM_NET_IS_11N()
	 */
	IE_FOR_EACH(pucIE, u2IELength, u2Offset) {
		switch (IE_ID(pucIE)) {
		case ELEM_ID_HT_CAP:
			if (!RLM_NET_IS_11N(prBssInfo) || IE_LEN(pucIE) != (sizeof(IE_HT_CAP_T) - 2))
				break;
			prHtCap = (P_IE_HT_CAP_T) pucIE;
			prStaRec->ucMcsSet = prHtCap->rSupMcsSet.aucRxMcsBitmask[0];
			prStaRec->fgSupMcs32 = (prHtCap->rSupMcsSet.aucRxMcsBitmask[32 / 8] & BIT(0)) ? TRUE : FALSE;

			prStaRec->u2HtCapInfo = prHtCap->u2HtCapInfo;
			prStaRec->ucAmpduParam = prHtCap->ucAmpduParam;
			prStaRec->u2HtExtendedCap = prHtCap->u2HtExtendedCap;
			prStaRec->u4TxBeamformingCap = prHtCap->u4TxBeamformingCap;
			prStaRec->ucAselCap = prHtCap->ucAselCap;
			break;

		case ELEM_ID_HT_OP:
			if (!RLM_NET_IS_11N(prBssInfo) || IE_LEN(pucIE) != (sizeof(IE_HT_OP_T) - 2))
				break;
			prHtOp = (P_IE_HT_OP_T) pucIE;
			/* Workaround that some APs fill primary channel field by its
			 * secondary channel, but its DS IE is correct 20110610
			 */
			if (ucPrimaryChannel == 0)
				ucPrimaryChannel = prHtOp->ucPrimaryChannel;
			prBssInfo->ucHtOpInfo1 = prHtOp->ucInfo1;
			prBssInfo->u2HtOpInfo2 = prHtOp->u2Info2;
			prBssInfo->u2HtOpInfo3 = prHtOp->u2Info3;

			if (!prBssInfo->fg40mBwAllowed)
				prBssInfo->ucHtOpInfo1 &= ~(HT_OP_INFO1_SCO | HT_OP_INFO1_STA_CHNL_WIDTH);

			if ((prBssInfo->ucHtOpInfo1 & HT_OP_INFO1_SCO) != CHNL_EXT_RES)
				prBssInfo->eBssSCO = (ENUM_CHNL_EXT_T)(prBssInfo->ucHtOpInfo1 & HT_OP_INFO1_SCO);

			prBssInfo->eHtProtectMode = (ENUM_HT_PROTECT_MODE_T)
			    (prBssInfo->u2HtOpInfo2 & HT_OP_INFO2_HT_PROTECTION);

			/* To do: process regulatory class 16 */
			if ((prBssInfo->u2HtOpInfo2 & HT_OP_INFO2_OBSS_NON_HT_STA_PRESENT)
			    && 0 /* && regulatory class is 16 */)
				prBssInfo->eGfOperationMode = GF_MODE_DISALLOWED;
			else if (prBssInfo->u2HtOpInfo2 & HT_OP_INFO2_NON_GF_HT_STA_PRESENT)
				prBssInfo->eGfOperationMode = GF_MODE_PROTECT;
			else
				prBssInfo->eGfOperationMode = GF_MODE_NORMAL;

			prBssInfo->eRifsOperationMode =
			    (prBssInfo->ucHtOpInfo1 & HT_OP_INFO1_RIFS_MODE) ? RIFS_MODE_NORMAL : RIFS_MODE_DISALLOWED;

			break;

		case ELEM_ID_20_40_BSS_COEXISTENCE:
			if (!RLM_NET_IS_11N(prBssInfo))
				break;
			/* To do: store if scanning exemption grant to BssInfo */
			break;

		case ELEM_ID_OBSS_SCAN_PARAMS:
			if (!RLM_NET_IS_11N(prBssInfo) || IE_LEN(pucIE) != (sizeof(IE_OBSS_SCAN_PARAM_T) - 2))
				break;
			/* Store OBSS parameters to BssInfo */
			prObssScnParam = (P_IE_OBSS_SCAN_PARAM_T) pucIE;
			break;

		case ELEM_ID_EXTENDED_CAP:
			if (!RLM_NET_IS_11N(prBssInfo))
				break;
			/* To do: store extended capability (PSMP, coexist) to BssInfo */
			break;

		case ELEM_ID_ERP_INFO:
			if (IE_LEN(pucIE) != (sizeof(IE_ERP_T) - 2) || prBssInfo->eBand != BAND_2G4)
				break;
			ucERP = ERP_INFO_IE(pucIE)->ucERP;
			prBssInfo->fgErpProtectMode = (ucERP & ERP_INFO_USE_PROTECTION) ? TRUE : FALSE;

			if (ucERP & ERP_INFO_BARKER_PREAMBLE_MODE)
				prBssInfo->fgUseShortPreamble = FALSE;
			break;

		case ELEM_ID_DS_PARAM_SET:
			if (IE_LEN(pucIE) == ELEM_MAX_LEN_DS_PARAMETER_SET)
				ucPrimaryChannel = DS_PARAM_IE(pucIE)->ucCurrChnl;
			break;

#if CFG_SUPPORT_DFS		/* Add by Enlai */
		case ELEM_ID_CH_SW_ANNOUNCEMENT:
			{
				rlmProcessChannelSwitchIE(prAdapter, (P_IE_CHANNEL_SWITCH_T) pucIE);
			}
			break;

#if CFG_SUPPORT_QUIET && 0
			/* Note: RRM code should be moved to independent RRM function by
			 *       component design rule. But we attach it to RLM temporarily
			 */
		case ELEM_ID_QUIET:
			rrmQuietHandleQuietIE(prBssInfo, (P_IE_QUIET_T) pucIE);
			fgHasQuietIE = TRUE;
			break;
#endif
#endif

		default:
			break;
		}		/* end of switch */
	}			/* end of IE_FOR_EACH */

	/* Some AP will have wrong channel number (255) when running time.
	 * Check if correct channel number information. 20110501
	 */
	if ((prBssInfo->eBand == BAND_2G4 && ucPrimaryChannel > 14) ||
	    (prBssInfo->eBand != BAND_2G4 && (ucPrimaryChannel >= 200 || ucPrimaryChannel <= 14)))
		ucPrimaryChannel = 0;
#if CFG_SUPPORT_QUIET && 0
	if (!fgHasQuietIE)
		rrmQuietIeNotExist(prAdapter, prBssInfo);
#endif

	/* Check if OBSS scan process will launch */
	if (!prAdapter->fgEnOnlineScan || !prObssScnParam ||
	    !(prStaRec->u2HtCapInfo & HT_CAP_INFO_SUP_CHNL_WIDTH) ||
	    prBssInfo->eBand != BAND_2G4 || !prBssInfo->fg40mBwAllowed) {

		/* Note: it is ok not to stop rObssScanTimer() here */
		prBssInfo->u2ObssScanInterval = 0;
	} else {
		if (prObssScnParam->u2TriggerScanInterval < OBSS_SCAN_MIN_INTERVAL)
			prObssScnParam->u2TriggerScanInterval = OBSS_SCAN_MIN_INTERVAL;
		if (prBssInfo->u2ObssScanInterval != prObssScnParam->u2TriggerScanInterval) {

			prBssInfo->u2ObssScanInterval = prObssScnParam->u2TriggerScanInterval;

			/* Start timer to trigger OBSS scanning */
			cnmTimerStartTimer(prAdapter, &prBssInfo->rObssScanTimer,
					   prBssInfo->u2ObssScanInterval * MSEC_PER_SEC);
		}
	}

	return ucPrimaryChannel;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief AIS or P2P GC.
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
static BOOLEAN
rlmRecBcnFromNeighborForClient(P_ADAPTER_T prAdapter,
			       P_BSS_INFO_T prBssInfo, P_SW_RFB_T prSwRfb, PUINT_8 pucIE, UINT_16 u2IELength)
{
	UINT_16 u2Offset, i;
	UINT_8 ucPriChannel, ucSecChannel;
	ENUM_CHNL_EXT_T eSCO;
	BOOLEAN fgHtBss, fg20mReq;

	if ((prAdapter == NULL)
		|| (pucIE == NULL)
		|| (prBssInfo == NULL)
		|| (prSwRfb == NULL)) {
		ASSERT(FALSE);
		return FALSE;
	}

	/* Record it to channel list to change 20/40 bandwidth */
	ucPriChannel = 0;
	eSCO = CHNL_EXT_SCN;

	fgHtBss = FALSE;
	fg20mReq = FALSE;

	IE_FOR_EACH(pucIE, u2IELength, u2Offset) {
		switch (IE_ID(pucIE)) {
		case ELEM_ID_HT_CAP:
			{
				P_IE_HT_CAP_T prHtCap;

				if (IE_LEN(pucIE) != (sizeof(IE_HT_CAP_T) - 2))
					break;

				prHtCap = (P_IE_HT_CAP_T) pucIE;
				if (prHtCap->u2HtCapInfo & HT_CAP_INFO_40M_INTOLERANT)
					fg20mReq = TRUE;
				fgHtBss = TRUE;
				break;
			}
		case ELEM_ID_HT_OP:
			{
				P_IE_HT_OP_T prHtOp;

				if (IE_LEN(pucIE) != (sizeof(IE_HT_OP_T) - 2))
					break;

				prHtOp = (P_IE_HT_OP_T) pucIE;
				/* Workaround that some APs fill primary channel field by its
				 * secondary channel, but its DS IE is correct 20110610
				 */
				if (ucPriChannel == 0)
					ucPriChannel = prHtOp->ucPrimaryChannel;

				if ((prHtOp->ucInfo1 & HT_OP_INFO1_SCO) != CHNL_EXT_RES)
					eSCO = (ENUM_CHNL_EXT_T) (prHtOp->ucInfo1 & HT_OP_INFO1_SCO);
				break;
			}
		case ELEM_ID_20_40_BSS_COEXISTENCE:
			{
				P_IE_20_40_COEXIST_T prCoexist;

				if (IE_LEN(pucIE) != (sizeof(IE_20_40_COEXIST_T) - 2))
					break;

				prCoexist = (P_IE_20_40_COEXIST_T) pucIE;
				if (prCoexist->ucData & BSS_COEXIST_40M_INTOLERANT)
					fg20mReq = TRUE;
				break;
			}
		case ELEM_ID_DS_PARAM_SET:
			if (IE_LEN(pucIE) != (sizeof(IE_DS_PARAM_SET_T) - 2))
				break;
			ucPriChannel = DS_PARAM_IE(pucIE)->ucCurrChnl;
			break;

		default:
			break;
		}
	}

	/* To do: Update channel list and 5G band. All channel lists have the same
	 * update procedure. We should give it the entry pointer of desired
	 * channel list.
	 */
	if (HIF_RX_HDR_GET_RF_BAND(prSwRfb->prHifRxHdr) != BAND_2G4)
		return FALSE;

	if (ucPriChannel == 0 || ucPriChannel > 14)
		ucPriChannel = HIF_RX_HDR_GET_CHNL_NUM(prSwRfb->prHifRxHdr);

	if (fgHtBss) {
		ASSERT(prBssInfo->auc2G_PriChnlList[0] <= CHNL_LIST_SZ_2G);
		for (i = 1; i <= prBssInfo->auc2G_PriChnlList[0] && i <= CHNL_LIST_SZ_2G; i++) {
			if (prBssInfo->auc2G_PriChnlList[i] == ucPriChannel)
				break;
		}
		if ((i > prBssInfo->auc2G_PriChnlList[0]) && (i <= CHNL_LIST_SZ_2G)) {
			prBssInfo->auc2G_PriChnlList[i] = ucPriChannel;
			prBssInfo->auc2G_PriChnlList[0]++;
		}

		/* Update secondary channel */
		if (eSCO != CHNL_EXT_SCN) {
			ucSecChannel = (eSCO == CHNL_EXT_SCA) ? (ucPriChannel + 4) : (ucPriChannel - 4);

			ASSERT(prBssInfo->auc2G_SecChnlList[0] <= CHNL_LIST_SZ_2G);
			for (i = 1; i <= prBssInfo->auc2G_SecChnlList[0] && i <= CHNL_LIST_SZ_2G; i++) {
				if (prBssInfo->auc2G_SecChnlList[i] == ucSecChannel)
					break;
			}
			if ((i > prBssInfo->auc2G_SecChnlList[0]) && (i <= CHNL_LIST_SZ_2G)) {
				prBssInfo->auc2G_SecChnlList[i] = ucSecChannel;
				prBssInfo->auc2G_SecChnlList[0]++;
			}
		}

		/* Update 20M bandwidth request channels */
		if (fg20mReq) {
			ASSERT(prBssInfo->auc2G_20mReqChnlList[0] <= CHNL_LIST_SZ_2G);
			for (i = 1; i <= prBssInfo->auc2G_20mReqChnlList[0] && i <= CHNL_LIST_SZ_2G; i++) {
				if (prBssInfo->auc2G_20mReqChnlList[i] == ucPriChannel)
					break;
			}
			if ((i > prBssInfo->auc2G_20mReqChnlList[0]) && (i <= CHNL_LIST_SZ_2G)) {
				prBssInfo->auc2G_20mReqChnlList[i] = ucPriChannel;
				prBssInfo->auc2G_20mReqChnlList[0]++;
			}
		}
	} else {
		/* Update non-HT channel list */
		ASSERT(prBssInfo->auc2G_NonHtChnlList[0] <= CHNL_LIST_SZ_2G);
		for (i = 1; i <= prBssInfo->auc2G_NonHtChnlList[0] && i <= CHNL_LIST_SZ_2G; i++) {
			if (prBssInfo->auc2G_NonHtChnlList[i] == ucPriChannel)
				break;
		}
		if ((i > prBssInfo->auc2G_NonHtChnlList[0]) && (i <= CHNL_LIST_SZ_2G)) {
			prBssInfo->auc2G_NonHtChnlList[i] = ucPriChannel;
			prBssInfo->auc2G_NonHtChnlList[0]++;
		}

	}

	return FALSE;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief AIS or P2P GC.
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
static BOOLEAN
rlmRecBcnInfoForClient(P_ADAPTER_T prAdapter,
		       P_BSS_INFO_T prBssInfo, P_SW_RFB_T prSwRfb, PUINT_8 pucIE, UINT_16 u2IELength)
{
	if ((prAdapter == NULL)
		|| (pucIE == NULL)
		|| (prBssInfo == NULL)
		|| (prSwRfb == NULL)) {
		ASSERT(FALSE);
		return FALSE;
	}

#if 0				/* SW migration 2010/8/20 */
	/* Note: we shall not update parameters when scanning, otherwise
	 *       channel and bandwidth will not be correct or asserted failure
	 *       during scanning.
	 * Note: remove channel checking. All received Beacons should be processed
	 *       if measurement or other actions are executed in adjacent channels
	 *       and Beacon content checking mechanism is not disabled.
	 */
	if (IS_SCAN_ACTIVE()
	    /* || prBssInfo->ucPrimaryChannel != CHNL_NUM_BY_SWRFB(prSwRfb) */
	    ) {
		return FALSE;
	}
#endif

	/* Handle change of slot time */
	prBssInfo->u2CapInfo = ((P_WLAN_BEACON_FRAME_T) (prSwRfb->pvHeader))->u2CapInfo;
	prBssInfo->fgUseShortSlotTime = (prBssInfo->u2CapInfo & CAP_INFO_SHORT_SLOT_TIME) ? TRUE : FALSE;

	rlmRecIeInfoForClient(prAdapter, prBssInfo, pucIE, u2IELength);

	return TRUE;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID rlmProcessBcn(P_ADAPTER_T prAdapter, P_SW_RFB_T prSwRfb, PUINT_8 pucIE, UINT_16 u2IELength)
{
	P_BSS_INFO_T prBssInfo;
	BOOLEAN fgNewParameter;
	UINT_8 ucNetIdx;

	ASSERT(prAdapter);
	ASSERT(prSwRfb);
	ASSERT(pucIE);

	fgNewParameter = FALSE;

	/* When concurrent networks exist, GO shall have the same handle as
	 * the other BSS, so the Beacon shall be processed for bandwidth and
	 * protection mechanism.
	 * Note1: we do not have 2 AP (GO) cases simultaneously now.
	 * Note2: If we are GO, concurrent AIS AP should detect it and reflect
	 *        action in its Beacon, so AIS STA just follows Beacon from AP.
	 */
	RLM_NET_FOR_EACH_NO_BOW(ucNetIdx) {
		prBssInfo = &prAdapter->rWifiVar.arBssInfo[ucNetIdx];
		ASSERT(prBssInfo);

		if (IS_BSS_ACTIVE(prBssInfo)) {
			if (prBssInfo->eCurrentOPMode == OP_MODE_INFRASTRUCTURE &&
			    prBssInfo->eConnectionState == PARAM_MEDIA_STATE_CONNECTED) {
				/* P2P client or AIS infra STA */
				if (EQUAL_MAC_ADDR(prBssInfo->aucBSSID, ((P_WLAN_MAC_MGMT_HEADER_T)
									 (prSwRfb->pvHeader))->aucBSSID)) {

					fgNewParameter = rlmRecBcnInfoForClient(prAdapter,
										prBssInfo, prSwRfb, pucIE, u2IELength);
				} else {
					fgNewParameter = rlmRecBcnFromNeighborForClient(prAdapter,
											prBssInfo, prSwRfb, pucIE,
											u2IELength);
				}
			}
#if CFG_ENABLE_WIFI_DIRECT
			else if (prAdapter->fgIsP2PRegistered &&
				 (prBssInfo->eCurrentOPMode == OP_MODE_ACCESS_POINT ||
				  prBssInfo->eCurrentOPMode == OP_MODE_P2P_DEVICE)) {
				/* AP scan to check if 20/40M bandwidth is permitted */
				rlmRecBcnFromNeighborForClient(prAdapter, prBssInfo, prSwRfb, pucIE, u2IELength);
			}
#endif
			else if (prBssInfo->eCurrentOPMode == OP_MODE_IBSS) {
				/* Do nothing */
				/* To do: Ad-hoc */
			}

			/* Appy new parameters if necessary */
			if (fgNewParameter) {
				DBGLOG(RLM, TRACE, "rlmProcessBcn\n");
				rlmSyncOperationParams(prAdapter, prBssInfo);
				fgNewParameter = FALSE;
			}
		}		/* end of IS_BSS_ACTIVE() */
	}			/* end of RLM_NET_FOR_EACH_NO_BOW */
}

/*----------------------------------------------------------------------------*/
/*!
* \brief This function should be invoked after judging successful association.
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID rlmProcessAssocRsp(P_ADAPTER_T prAdapter, P_SW_RFB_T prSwRfb, PUINT_8 pucIE, UINT_16 u2IELength)
{
	P_BSS_INFO_T prBssInfo;
	P_STA_RECORD_T prStaRec;
	UINT_8 ucPriChannel;
	UINT_16 u2Offset;

	ASSERT(prAdapter);
	ASSERT(prSwRfb);
	ASSERT(pucIE);

	prStaRec = cnmGetStaRecByIndex(prAdapter, prSwRfb->ucStaRecIdx);
	ASSERT(prStaRec);
	if (!prStaRec)
		return;
	ASSERT(prStaRec->ucNetTypeIndex < NETWORK_TYPE_INDEX_NUM);

	prBssInfo = &prAdapter->rWifiVar.arBssInfo[prStaRec->ucNetTypeIndex];
	ASSERT(prStaRec == prBssInfo->prStaRecOfAP);

	/* To do: the invoked function is used to clear all members. It may be
	 *        done by center mechanism in invoker.
	 */
	rlmBssReset(prAdapter, prBssInfo);

	prBssInfo->fgUseShortSlotTime = (prBssInfo->u2CapInfo & CAP_INFO_SHORT_SLOT_TIME) ? TRUE : FALSE;

	ucPriChannel = rlmRecIeInfoForClient(prAdapter, prBssInfo, pucIE, u2IELength);
	if (ucPriChannel > 0)
		prBssInfo->ucPrimaryChannel = ucPriChannel;

	if (!RLM_NET_IS_11N(prBssInfo) || !(prStaRec->u2HtCapInfo & HT_CAP_INFO_SUP_CHNL_WIDTH))
		prBssInfo->fg40mBwAllowed = FALSE;

	/* Note: Update its capabilities to WTBL by cnmStaRecChangeState(), which
	 *       shall be invoked afterwards.
	 *       Update channel, bandwidth and protection mode by nicUpdateBss()
	 */
#if 1
	if (prStaRec->ucNetTypeIndex == NETWORK_TYPE_P2P_INDEX) {

		DBGLOG(P2P, WARN, "Force P2P BW to 20\n");
		prBssInfo->fgAssoc40mBwAllowed = FALSE;
	}
#endif
	IE_FOR_EACH(pucIE, u2IELength, u2Offset) {
		switch (IE_ID(pucIE)) {
		case ELEM_ID_BSS_MAX_IDLE_PERIOD:
		{
			struct IE_BSS_MAX_IDLE_PERIOD *prBssMaxIdle = (struct IE_BSS_MAX_IDLE_PERIOD *)pucIE;

			prStaRec->u2MaxIdlePeriod = prBssMaxIdle->u2MaxIdlePeriod;
			prStaRec->ucIdleOption = prBssMaxIdle->ucIdleOption;
			break;
		}
		default:
			break;
		}		/* end of switch */
	}			/* end of IE_FOR_EACH */

}

/*----------------------------------------------------------------------------*/
/*!
* \brief This function should be invoked after judging successful association.
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID rlmFillSyncCmdParam(P_CMD_SET_BSS_RLM_PARAM_T prCmdBody, P_BSS_INFO_T prBssInfo)
{
	ASSERT(prCmdBody && prBssInfo);
	if (!prCmdBody || !prBssInfo)
		return;

	prCmdBody->ucNetTypeIndex = prBssInfo->ucNetTypeIndex;
	prCmdBody->ucRfBand = (UINT_8) prBssInfo->eBand;
	prCmdBody->ucPrimaryChannel = prBssInfo->ucPrimaryChannel;
	prCmdBody->ucRfSco = (UINT_8) prBssInfo->eBssSCO;
	prCmdBody->ucErpProtectMode = (UINT_8) prBssInfo->fgErpProtectMode;
	prCmdBody->ucHtProtectMode = (UINT_8) prBssInfo->eHtProtectMode;
	prCmdBody->ucGfOperationMode = (UINT_8) prBssInfo->eGfOperationMode;
	prCmdBody->ucTxRifsMode = (UINT_8) prBssInfo->eRifsOperationMode;
	prCmdBody->u2HtOpInfo3 = prBssInfo->u2HtOpInfo3;
	prCmdBody->u2HtOpInfo2 = prBssInfo->u2HtOpInfo2;
	prCmdBody->ucHtOpInfo1 = prBssInfo->ucHtOpInfo1;
	prCmdBody->ucUseShortPreamble = prBssInfo->fgUseShortPreamble;
	prCmdBody->ucUseShortSlotTime = prBssInfo->fgUseShortSlotTime;
	prCmdBody->ucCheckId = 0x72;

	if (RLM_NET_PARAM_VALID(prBssInfo)) {
		DBGLOG(RLM, INFO, "N=%d b=%d c=%d s=%d e=%d h=%d I=0x%02x l=%d p=%d\n",
				   prCmdBody->ucNetTypeIndex, prCmdBody->ucRfBand,
				   prCmdBody->ucPrimaryChannel, prCmdBody->ucRfSco,
				   prCmdBody->ucErpProtectMode, prCmdBody->ucHtProtectMode,
				   prCmdBody->ucHtOpInfo1, prCmdBody->ucUseShortSlotTime,
				   prCmdBody->ucUseShortPreamble);
	} else {
		DBGLOG(RLM, TRACE, "N=%d closed\n", prCmdBody->ucNetTypeIndex);
	}
}

/*----------------------------------------------------------------------------*/
/*!
* \brief This function will operation parameters based on situations of
*        concurrent networks. Channel, bandwidth, protection mode, supported
*        rate will be modified.
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID rlmSyncOperationParams(P_ADAPTER_T prAdapter, P_BSS_INFO_T prBssInfo)
{
	P_CMD_SET_BSS_RLM_PARAM_T prCmdBody;
	WLAN_STATUS rStatus;

	ASSERT(prAdapter);
	ASSERT(prBssInfo);

	prCmdBody = (P_CMD_SET_BSS_RLM_PARAM_T)
	    cnmMemAlloc(prAdapter, RAM_TYPE_BUF, sizeof(CMD_SET_BSS_RLM_PARAM_T));
	ASSERT(prCmdBody);

	/* To do: exception handle */
	if (!prCmdBody) {
		DBGLOG(RLM, WARN, "No buf for sync RLM params (Net=%d)\n", prBssInfo->ucNetTypeIndex);
		return;
	}

	rlmFillSyncCmdParam(prCmdBody, prBssInfo);

	rStatus = wlanSendSetQueryCmd(prAdapter,	/* prAdapter */
				      CMD_ID_SET_BSS_RLM_PARAM,	/* ucCID */
				      TRUE,	/* fgSetQuery */
				      FALSE,	/* fgNeedResp */
				      FALSE,	/* fgIsOid */
				      NULL,	/* pfCmdDoneHandler */
				      NULL,	/* pfCmdTimeoutHandler */
				      sizeof(CMD_SET_BSS_RLM_PARAM_T),	/* u4SetQueryInfoLen */
				      (PUINT_8) prCmdBody,	/* pucInfoBuffer */
				      NULL,	/* pvSetQueryBuffer */
				      0	/* u4SetQueryBufferLen */
	    );

	ASSERT(rStatus == WLAN_STATUS_PENDING);

	cnmMemFree(prAdapter, prCmdBody);
}

#if CFG_SUPPORT_AAA
/*----------------------------------------------------------------------------*/
/*!
* \brief This function should be invoked after judging successful association.
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID rlmProcessAssocReq(P_ADAPTER_T prAdapter, P_SW_RFB_T prSwRfb, PUINT_8 pucIE, UINT_16 u2IELength)
{
	P_BSS_INFO_T prBssInfo;
	P_STA_RECORD_T prStaRec;
	UINT_16 u2Offset;
	P_IE_HT_CAP_T prHtCap;

	ASSERT(prAdapter);
	ASSERT(prSwRfb);
	ASSERT(pucIE);

	prStaRec = cnmGetStaRecByIndex(prAdapter, prSwRfb->ucStaRecIdx);
	ASSERT(prStaRec);
	if (!prStaRec)
		return;
	ASSERT(prStaRec->ucNetTypeIndex < NETWORK_TYPE_INDEX_NUM);

	prBssInfo = &prAdapter->rWifiVar.arBssInfo[prStaRec->ucNetTypeIndex];

	IE_FOR_EACH(pucIE, u2IELength, u2Offset) {
		switch (IE_ID(pucIE)) {
		case ELEM_ID_HT_CAP:
			if (!RLM_NET_IS_11N(prBssInfo) || IE_LEN(pucIE) != (sizeof(IE_HT_CAP_T) - 2))
				break;
			prHtCap = (P_IE_HT_CAP_T) pucIE;
			prStaRec->ucMcsSet = prHtCap->rSupMcsSet.aucRxMcsBitmask[0];
			prStaRec->fgSupMcs32 = (prHtCap->rSupMcsSet.aucRxMcsBitmask[32 / 8] & BIT(0)) ? TRUE : FALSE;

			prStaRec->u2HtCapInfo = prHtCap->u2HtCapInfo;
			prStaRec->ucAmpduParam = prHtCap->ucAmpduParam;
			prStaRec->u2HtExtendedCap = prHtCap->u2HtExtendedCap;
			prStaRec->u4TxBeamformingCap = prHtCap->u4TxBeamformingCap;
			prStaRec->ucAselCap = prHtCap->ucAselCap;
			break;

		default:
			break;
		}		/* end of switch */
	}			/* end of IE_FOR_EACH */
}
#endif /* CFG_SUPPORT_AAA */

/*----------------------------------------------------------------------------*/
/*!
* \brief It is for both STA and AP modes
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID rlmBssInitForAPandIbss(P_ADAPTER_T prAdapter, P_BSS_INFO_T prBssInfo)
{
	ASSERT(prAdapter);
	ASSERT(prBssInfo);

#if CFG_ENABLE_WIFI_DIRECT
	if (prAdapter->fgIsP2PRegistered && prBssInfo->eCurrentOPMode == OP_MODE_ACCESS_POINT)
		rlmBssInitForAP(prAdapter, prBssInfo);
#endif
}

/*----------------------------------------------------------------------------*/
/*!
* \brief It is for both STA and AP modes
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID rlmBssAborted(P_ADAPTER_T prAdapter, P_BSS_INFO_T prBssInfo)
{
	ASSERT(prAdapter);
	ASSERT(prBssInfo);

	rlmBssReset(prAdapter, prBssInfo);

	prBssInfo->fg40mBwAllowed = FALSE;
	prBssInfo->fgAssoc40mBwAllowed = FALSE;

	/* Assume FW state is updated by CMD_ID_SET_BSS_INFO, so
	 * the sync CMD is not needed here.
	 */
}

/*----------------------------------------------------------------------------*/
/*!
* \brief All RLM timers will also be stopped.
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
static VOID rlmBssReset(P_ADAPTER_T prAdapter, P_BSS_INFO_T prBssInfo)
{
	ASSERT(prAdapter);
	ASSERT(prBssInfo);

	/* HT related parameters */
	prBssInfo->ucHtOpInfo1 = 0;	/* RIFS disabled. 20MHz */
	prBssInfo->u2HtOpInfo2 = 0;
	prBssInfo->u2HtOpInfo3 = 0;

	prBssInfo->eBssSCO = 0;
	prBssInfo->fgErpProtectMode = 0;
	prBssInfo->eHtProtectMode = 0;
	prBssInfo->eGfOperationMode = 0;
	prBssInfo->eRifsOperationMode = 0;

	/* OBSS related parameters */
	prBssInfo->auc2G_20mReqChnlList[0] = 0;
	prBssInfo->auc2G_NonHtChnlList[0] = 0;
	prBssInfo->auc2G_PriChnlList[0] = 0;
	prBssInfo->auc2G_SecChnlList[0] = 0;
	prBssInfo->auc5G_20mReqChnlList[0] = 0;
	prBssInfo->auc5G_NonHtChnlList[0] = 0;
	prBssInfo->auc5G_PriChnlList[0] = 0;
	prBssInfo->auc5G_SecChnlList[0] = 0;

	/* All RLM timers will also be stopped */
	cnmTimerStopTimer(prAdapter, &prBssInfo->rObssScanTimer);
	prBssInfo->u2ObssScanInterval = 0;

	prBssInfo->fgObssErpProtectMode = 0;	/* GO only */
	prBssInfo->eObssHtProtectMode = 0;	/* GO only */
	prBssInfo->eObssGfOperationMode = 0;	/* GO only */
	prBssInfo->fgObssRifsOperationMode = 0;	/* GO only */
	prBssInfo->fgObssActionForcedTo20M = 0;	/* GO only */
	prBssInfo->fgObssBeaconForcedTo20M = 0;	/* GO only */
}

#if CFG_SUPPORT_DFS		/* Add by Enlai */
/*----------------------------------------------------------------------------*/
/*!
* \brief This function handle spectrum management action frame
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID rlmProcessSpecMgtAction(P_ADAPTER_T prAdapter, P_SW_RFB_T prSwRfb)
{
	P_ACTION_CHANNEL_SWITCH_FRAME prRxFrame;

	ASSERT(prAdapter);
	ASSERT(prSwRfb);

	DBGLOG(RLM, INFO, "[5G DFS]rlmProcessSpecMgtAction \r\n");

	prRxFrame = (P_ACTION_CHANNEL_SWITCH_FRAME) prSwRfb->pvHeader;
	DBGLOG(RLM, INFO, "[5G DFS]prRxFrame->ucAction[%d] \r\n", prRxFrame->ucAction);
	if (prRxFrame->ucAction == ACTION_CHNL_SWITCH)
		rlmProcessChannelSwitchIE(prAdapter, (P_IE_CHANNEL_SWITCH_T) prRxFrame->aucInfoElem);

}

/*----------------------------------------------------------------------------*/
/*!
* \brief This function process Channel Switch IE
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID rlmProcessChannelSwitchIE(P_ADAPTER_T prAdapter, P_IE_CHANNEL_SWITCH_T prChannelSwitchIE)
{
	P_BSS_INFO_T prAisBssInfo;

	ASSERT(prAdapter);
	ASSERT(prChannelSwitchIE);

	DBGLOG(RLM, INFO, "[5G DFS] rlmProcessChannelSwitchIE \r\n");
	DBGLOG(RLM, INFO, "[5G DFS] ucChannelSwitchMode[%d], ucChannelSwitchCount[%d], ucNewChannelNum[%d] \r\n",
	       prChannelSwitchIE->ucChannelSwitchMode,
	       prChannelSwitchIE->ucChannelSwitchCount, prChannelSwitchIE->ucNewChannelNum);
	if (prChannelSwitchIE->ucChannelSwitchMode == 1) {
		prAisBssInfo = &(prAdapter->rWifiVar.arBssInfo[NETWORK_TYPE_AIS_INDEX]);
		DBGLOG(RLM, INFO, "[5G DFS] switch channel [%d]->[%d] \r\n", prAisBssInfo->ucPrimaryChannel,
		       prChannelSwitchIE->ucNewChannelNum);
		prAisBssInfo->ucPrimaryChannel = prChannelSwitchIE->ucNewChannelNum;
		nicUpdateBss(prAdapter, prAisBssInfo->ucNetTypeIndex);
	}

}

#endif

#if (CFG_SUPPORT_TXR_ENC == 1)
VOID
rlmTxRateEnhanceConfig(
	P_ADAPTER_T         prAdapter
	)
{
	P_GLUE_INFO_T prGlueInfo;
	WLAN_STATUS rStatus = WLAN_STATUS_FAILURE;
	CMD_RLM_INFO_T rTxRInfo;
	UINT_32 u4SetInfoLen = 0;


	/* init */
	prGlueInfo = prAdapter->prGlueInfo;

	/* suggestion from Tsaiyuan.Hsu */
	kalMemZero(&rTxRInfo, sizeof(CMD_RLM_INFO_T));
	rTxRInfo.fgIsErrRatioEnhanceApplied = TRUE;
	rTxRInfo.ucErrRatio2LimitMinRate = 3;
	rTxRInfo.ucMinLegacyRateIdx = 2;
	rTxRInfo.cMinRssiThreshold = -60;
	rTxRInfo.fgIsRtsApplied = TRUE;
	rTxRInfo.ucRecoverTime = 60;

	DBGLOG(RLM, INFO, "Enable tx rate enhance function\n");

	rStatus = kalIoctl(prGlueInfo,
			wlanoidSetTxRateInfo,
			&rTxRInfo,
			sizeof(rTxRInfo),
			TRUE,
			TRUE,
			TRUE,
			FALSE,
			&u4SetInfoLen);

	if (rStatus != WLAN_STATUS_SUCCESS)
		DBGLOG(RLM, WARN, "set tx rate advance info fail 0x%lx\n", rStatus);
}


/*----------------------------------------------------------------------------*/
/*! \brief  This routine is called to send a command to TX Auto Rate module.
*
* \param[in] prGlueInfo		Pointer to the Adapter structure
* \param[in] prInBuf		A pointer to the command string buffer
* \param[in] u4InBufLen	The length of the buffer
* \param[out] None
*
* \retval None
*/
/*----------------------------------------------------------------------------*/
VOID
rlmCmd(P_GLUE_INFO_T prGlueInfo, UINT_8	*prInBuf, UINT_32 u4InBufLen)
{
	UINT_32 u4Subcmd;


	/* parse TAR sub-command */
	u4Subcmd = CmdStringDecParse(prInBuf, &prInBuf, &u4InBufLen);
	DBGLOG(RLM, INFO, "<tar_cmd> sub command = %u\n", (UINT32)u4Subcmd);

	/* handle different sub-command */
	switch (u4Subcmd) {
	case 0x00: /* configure */
		/* iwpriv wlan0 set_str_cmd 1_0_0_1_3_2_60_1_60 */
		WLAN_STATUS rStatus = WLAN_STATUS_FAILURE;
		CMD_RLM_INFO_T rTxRInfo;
		UINT_32 u4SetInfoLen = 0;

		kalMemZero(&rTxRInfo, sizeof(CMD_RLM_INFO_T));
		rTxRInfo.u4Version = CmdStringDecParse(prInBuf, &prInBuf, &u4InBufLen);
		rTxRInfo.fgIsErrRatioEnhanceApplied = CmdStringDecParse(prInBuf, &prInBuf, &u4InBufLen);
		rTxRInfo.ucErrRatio2LimitMinRate = CmdStringDecParse(prInBuf, &prInBuf, &u4InBufLen);
		rTxRInfo.ucMinLegacyRateIdx = CmdStringDecParse(prInBuf, &prInBuf, &u4InBufLen);
		rTxRInfo.cMinRssiThreshold = 0 - CmdStringDecParse(prInBuf, &prInBuf, &u4InBufLen);
		rTxRInfo.fgIsRtsApplied = CmdStringDecParse(prInBuf, &prInBuf, &u4InBufLen);
		rTxRInfo.ucRecoverTime = CmdStringDecParse(prInBuf, &prInBuf, &u4InBufLen);

		DBGLOG(RLM, INFO, "<tar_cmd> rlmCmd = %u %u %u %u %d %u %u\n",
			rTxRInfo.u4Version,
			rTxRInfo.fgIsErrRatioEnhanceApplied,
			rTxRInfo.ucErrRatio2LimitMinRate,
			rTxRInfo.ucMinLegacyRateIdx,
			rTxRInfo.cMinRssiThreshold,
			rTxRInfo.fgIsRtsApplied,
			rTxRInfo.ucRecoverTime));

		rStatus = kalIoctl(prGlueInfo,
				wlanoidSetTxRateInfo,
				&rTxRInfo,
				sizeof(rTxRInfo),
				TRUE,
				TRUE,
				TRUE,
				FALSE,
				&u4SetInfoLen);
		break;

	default:
		break;
	}
}
#endif /* CFG_SUPPORT_TXR_ENC */

#if CFG_SUPPORT_802_11K
VOID rlmProcessNeighborReportResonse(
	P_ADAPTER_T prAdapter, P_WLAN_ACTION_FRAME prAction, UINT_16 u2PacketLen)
{
	struct ACTION_NEIGHBOR_REPORT_FRAME *prNeighborResponse =
			(struct ACTION_NEIGHBOR_REPORT_FRAME *)prAction;

	ASSERT(prAdapter);
	ASSERT(prNeighborResponse);
	DBGLOG(RLM, INFO, "Neighbor Resp From %pM, DialogToken %d\n",
			prNeighborResponse->aucSrcAddr, prNeighborResponse->ucDialogToken);
	aisCollectNeighborAPChannel(prAdapter,
		(struct IE_NEIGHBOR_REPORT_T *)&prNeighborResponse->aucInfoElem[0],
		u2PacketLen - OFFSET_OF(struct ACTION_NEIGHBOR_REPORT_FRAME, aucInfoElem));
}

VOID rlmTxNeighborReportRequest(P_ADAPTER_T prAdapter, P_STA_RECORD_T prStaRec,
		struct SUB_ELEMENT_LIST *prSubIEs)
{
	static UINT_8 ucDialogToken = 1;
	P_MSDU_INFO_T prMsduInfo = NULL;
	P_BSS_INFO_T prBssInfo = NULL;
	PUINT_8 pucPayload = NULL;
	struct ACTION_NEIGHBOR_REPORT_FRAME *prTxFrame = NULL;
	UINT_16 u2TxFrameLen = 500;
	UINT_16 u2FrameLen = 0;

	prBssInfo = &prAdapter->rWifiVar.arBssInfo[prStaRec->ucNetTypeIndex];
	ASSERT(prBssInfo);
	/* 1 Allocate MSDU Info */
	prMsduInfo = (P_MSDU_INFO_T) cnmMgtPktAlloc(prAdapter, MAC_TX_RESERVED_FIELD + u2TxFrameLen);
	if (!prMsduInfo)
		return;
	prTxFrame = (struct ACTION_NEIGHBOR_REPORT_FRAME *)
	    ((ULONG) (prMsduInfo->prPacket) + MAC_TX_RESERVED_FIELD);

	/* 2 Compose The Mac Header. */
	prTxFrame->u2FrameCtrl = MAC_FRAME_ACTION;
	COPY_MAC_ADDR(prTxFrame->aucDestAddr, prStaRec->aucMacAddr);
	COPY_MAC_ADDR(prTxFrame->aucSrcAddr, prBssInfo->aucOwnMacAddr);
	COPY_MAC_ADDR(prTxFrame->aucBSSID, prBssInfo->aucBSSID);
	prTxFrame->ucCategory = CATEGORY_RM_ACTION;
	prTxFrame->ucAction = RM_ACTION_NEIGHBOR_REQUEST;
	u2FrameLen = OFFSET_OF(struct ACTION_NEIGHBOR_REPORT_FRAME, aucInfoElem);
	/* 3 Compose the frame body's frame. */
	prTxFrame->ucDialogToken = ucDialogToken++;
	u2TxFrameLen -= sizeof(*prTxFrame) - 1;
	pucPayload = &prTxFrame->aucInfoElem[0];
	while (prSubIEs && u2TxFrameLen >= (prSubIEs->rSubIE.ucLength + 2)) {
		kalMemCopy(pucPayload, &prSubIEs->rSubIE, prSubIEs->rSubIE.ucLength + 2);
		pucPayload += prSubIEs->rSubIE.ucLength + 2;
		u2FrameLen += prSubIEs->rSubIE.ucLength + 2;
		prSubIEs = prSubIEs->prNext;
	}

	/* 4 Update information of MSDU_INFO_T */
	prMsduInfo->ucPacketType = HIF_TX_PACKET_TYPE_MGMT;	/* Management frame */
	prMsduInfo->ucStaRecIndex = prStaRec->ucIndex;
	prMsduInfo->ucNetworkType = prStaRec->ucNetTypeIndex;
	prMsduInfo->ucMacHeaderLength = WLAN_MAC_MGMT_HEADER_LEN;
	prMsduInfo->fgIs802_1x = FALSE;
	prMsduInfo->fgIs802_11 = TRUE;
	prMsduInfo->u2FrameLength = u2FrameLen;
	prMsduInfo->ucTxSeqNum = nicIncreaseTxSeqNum(prAdapter);
	prMsduInfo->pfTxDoneHandler = NULL;
	prMsduInfo->fgIsBasicRate = FALSE;

	/* 5 Enqueue the frame to send this action frame. */
	nicTxEnqueueMsdu(prAdapter, prMsduInfo);
}
#endif

