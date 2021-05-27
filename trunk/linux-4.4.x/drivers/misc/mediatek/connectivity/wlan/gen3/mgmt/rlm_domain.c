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
 * Id: //Department/DaVinci/BRANCHES/MT6620_WIFI_DRIVER_V2_3/mgmt/rlm_domain.c#2
 */

/*
 * ! \file   "rlm_domain.c"
 *  \brief
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
#include "rlm_txpwr_init.h"

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

/*
 * The following country or domain shall be set from host driver.
 * And host driver should pass specified DOMAIN_INFO_ENTRY to MT6620 as
 * the channel list of being a STA to do scanning/searching AP or being an
 * AP to choose an adequate channel if auto-channel is set.
 */

/*
 * Define mapping tables between country code and its channel set
 */
static const UINT_16 g_u2CountryGroup0[] = {
	COUNTRY_CODE_AO, COUNTRY_CODE_BZ, COUNTRY_CODE_BJ, COUNTRY_CODE_BT,
	COUNTRY_CODE_BO, COUNTRY_CODE_BI, COUNTRY_CODE_CM, COUNTRY_CODE_CF,
	COUNTRY_CODE_TD, COUNTRY_CODE_KM, COUNTRY_CODE_CD, COUNTRY_CODE_CG,
	COUNTRY_CODE_CI, COUNTRY_CODE_DJ, COUNTRY_CODE_GQ, COUNTRY_CODE_ER,
	COUNTRY_CODE_FJ, COUNTRY_CODE_GA, COUNTRY_CODE_GM, COUNTRY_CODE_GN,
	COUNTRY_CODE_GW, COUNTRY_CODE_RKS, COUNTRY_CODE_KG, COUNTRY_CODE_LY,
	COUNTRY_CODE_MG, COUNTRY_CODE_ML, COUNTRY_CODE_NR, COUNTRY_CODE_NC,
	COUNTRY_CODE_ST, COUNTRY_CODE_SC, COUNTRY_CODE_SL, COUNTRY_CODE_SB,
	COUNTRY_CODE_SO, COUNTRY_CODE_SR, COUNTRY_CODE_SZ, COUNTRY_CODE_TJ,
	COUNTRY_CODE_TG, COUNTRY_CODE_TO, COUNTRY_CODE_TM, COUNTRY_CODE_TV,
	COUNTRY_CODE_VU, COUNTRY_CODE_YE
};

static const UINT_16 g_u2CountryGroup1[] = {
	COUNTRY_CODE_AS, COUNTRY_CODE_AI, COUNTRY_CODE_BM, COUNTRY_CODE_KY,
	COUNTRY_CODE_GU, COUNTRY_CODE_FM, COUNTRY_CODE_PR, COUNTRY_CODE_US,
	COUNTRY_CODE_VI
};

static const UINT_16 g_u2CountryGroup2[] = {
	COUNTRY_CODE_AR, COUNTRY_CODE_AU, COUNTRY_CODE_AZ, COUNTRY_CODE_BW,
	COUNTRY_CODE_CX, COUNTRY_CODE_CO, COUNTRY_CODE_CR, COUNTRY_CODE_EC,
	COUNTRY_CODE_GD, COUNTRY_CODE_GT, COUNTRY_CODE_HK, COUNTRY_CODE_KH,
	COUNTRY_CODE_KI, COUNTRY_CODE_KR, COUNTRY_CODE_LB, COUNTRY_CODE_LR,
	COUNTRY_CODE_MN, COUNTRY_CODE_AN, COUNTRY_CODE_NZ, COUNTRY_CODE_NI,
	COUNTRY_CODE_PW, COUNTRY_CODE_PY, COUNTRY_CODE_PE, COUNTRY_CODE_PH,
	COUNTRY_CODE_WS, COUNTRY_CODE_SG, COUNTRY_CODE_LK, COUNTRY_CODE_TH,
	COUNTRY_CODE_TT, COUNTRY_CODE_UY, COUNTRY_CODE_VN
};

static const UINT_16 g_u2CountryGroup3[] = {
	COUNTRY_CODE_AW, COUNTRY_CODE_LA, COUNTRY_CODE_SA, COUNTRY_CODE_AE,
	COUNTRY_CODE_UG
};

static const UINT_16 g_u2CountryGroup4[] = { COUNTRY_CODE_MM };

static const UINT_16 g_u2CountryGroup5[] = {
	COUNTRY_CODE_AL, COUNTRY_CODE_DZ, COUNTRY_CODE_AD, COUNTRY_CODE_AT,
	COUNTRY_CODE_BY, COUNTRY_CODE_BE, COUNTRY_CODE_BA, COUNTRY_CODE_VG,
	COUNTRY_CODE_BG, COUNTRY_CODE_CV, COUNTRY_CODE_HR, COUNTRY_CODE_CY,
	COUNTRY_CODE_CZ, COUNTRY_CODE_DK, COUNTRY_CODE_EE, COUNTRY_CODE_ET,
	COUNTRY_CODE_FI, COUNTRY_CODE_FR, COUNTRY_CODE_GF, COUNTRY_CODE_PF,
	COUNTRY_CODE_TF, COUNTRY_CODE_GE, COUNTRY_CODE_DE, COUNTRY_CODE_GH,
	COUNTRY_CODE_GR, COUNTRY_CODE_GP, COUNTRY_CODE_HU, COUNTRY_CODE_IS,
	COUNTRY_CODE_IQ, COUNTRY_CODE_IE, COUNTRY_CODE_IT, COUNTRY_CODE_KE,
	COUNTRY_CODE_LV, COUNTRY_CODE_LS, COUNTRY_CODE_LI, COUNTRY_CODE_LT,
	COUNTRY_CODE_LU, COUNTRY_CODE_MK, COUNTRY_CODE_MT, COUNTRY_CODE_MQ,
	COUNTRY_CODE_MR, COUNTRY_CODE_MU, COUNTRY_CODE_YT, COUNTRY_CODE_MD,
	COUNTRY_CODE_MC, COUNTRY_CODE_ME, COUNTRY_CODE_MS, COUNTRY_CODE_NL,
	COUNTRY_CODE_NO, COUNTRY_CODE_OM, COUNTRY_CODE_PL, COUNTRY_CODE_PT,
	COUNTRY_CODE_RE, COUNTRY_CODE_RO, COUNTRY_CODE_MF, COUNTRY_CODE_SM,
	COUNTRY_CODE_SN, COUNTRY_CODE_RS, COUNTRY_CODE_SK, COUNTRY_CODE_SI,
	COUNTRY_CODE_ZA, COUNTRY_CODE_ES, COUNTRY_CODE_SE, COUNTRY_CODE_CH,
	COUNTRY_CODE_TR, COUNTRY_CODE_TC, COUNTRY_CODE_GB, COUNTRY_CODE_VA,
	COUNTRY_CODE_EU
};

static const UINT_16 g_u2CountryGroup6[] = { COUNTRY_CODE_JP };

static const UINT_16 g_u2CountryGroup7[] = {
	COUNTRY_CODE_AM, COUNTRY_CODE_IL, COUNTRY_CODE_KW, COUNTRY_CODE_MA,
	COUNTRY_CODE_NE, COUNTRY_CODE_TN,
};

static const UINT_16 g_u2CountryGroup8[] = { COUNTRY_CODE_NP };

static const UINT_16 g_u2CountryGroup9[] = { COUNTRY_CODE_AF };

static const UINT_16 g_u2CountryGroup10[] = {
	COUNTRY_CODE_AG, COUNTRY_CODE_BS, COUNTRY_CODE_BH, COUNTRY_CODE_BB,
	COUNTRY_CODE_BN, COUNTRY_CODE_CL, COUNTRY_CODE_CN, COUNTRY_CODE_EG,
	COUNTRY_CODE_SV, COUNTRY_CODE_IN, COUNTRY_CODE_MY, COUNTRY_CODE_MV,
	COUNTRY_CODE_PA, COUNTRY_CODE_VE, COUNTRY_CODE_ZM
};

static const UINT_16 g_u2CountryGroup11[] = { COUNTRY_CODE_JO, COUNTRY_CODE_PG };

static const UINT_16 g_u2CountryGroup12[] = {
	COUNTRY_CODE_BF, COUNTRY_CODE_GY, COUNTRY_CODE_HT, COUNTRY_CODE_HN,
	COUNTRY_CODE_JM, COUNTRY_CODE_MO, COUNTRY_CODE_MW, COUNTRY_CODE_PK,
	COUNTRY_CODE_QA, COUNTRY_CODE_RW, COUNTRY_CODE_KN, COUNTRY_CODE_TZ
};

static const UINT_16 g_u2CountryGroup13[] = { COUNTRY_CODE_ID };

static const UINT_16 g_u2CountryGroup14[] = { COUNTRY_CODE_NG };

static const UINT_16 g_u2CountryGroup15[] = {
	COUNTRY_CODE_BD, COUNTRY_CODE_BR, COUNTRY_CODE_DM, COUNTRY_CODE_DO,
	COUNTRY_CODE_FK, COUNTRY_CODE_KZ, COUNTRY_CODE_MX, COUNTRY_CODE_MZ,
	COUNTRY_CODE_NA, COUNTRY_CODE_RU, COUNTRY_CODE_LC, COUNTRY_CODE_VC,
	COUNTRY_CODE_UA, COUNTRY_CODE_UZ, COUNTRY_CODE_ZW
};

static const UINT_16 g_u2CountryGroup16[] = { COUNTRY_CODE_MP };

static const UINT_16 g_u2CountryGroup17[] = { COUNTRY_CODE_TW };

static const UINT_16 g_u2CountryGroup18[] = { COUNTRY_CODE_CA };

static const UINT_16 g_u2CountryGroup19[] = {
	COUNTRY_CODE_CK, COUNTRY_CODE_CU, COUNTRY_CODE_TL, COUNTRY_CODE_FO,
	COUNTRY_CODE_GI, COUNTRY_CODE_GG, COUNTRY_CODE_IR, COUNTRY_CODE_IM,
	COUNTRY_CODE_JE, COUNTRY_CODE_KP, COUNTRY_CODE_MH, COUNTRY_CODE_NU,
	COUNTRY_CODE_NF, COUNTRY_CODE_PS, COUNTRY_CODE_PN, COUNTRY_CODE_PM,
	COUNTRY_CODE_SS, COUNTRY_CODE_SD, COUNTRY_CODE_SY
};

static const UINT_16 g_u2CountryGroup20[] = {
	COUNTRY_CODE_DF
	/* When country code is not found and no matched NVRAM setting,
	 * the default group will be used.
	 */
};

DOMAIN_INFO_ENTRY arSupportedRegDomains[] = {
	{
	 (PUINT_16) g_u2CountryGroup0, sizeof(g_u2CountryGroup0) / 2,
	 {
	  {81, BAND_2G4, CHNL_SPAN_5, 1, 13, FALSE}
	  ,			/* CH_SET_2G4_1_13 */

	  {115, BAND_NULL, 0, 0, 0, FALSE}
	  ,			/* CH_SET_UNII_LOW_NA */
	  {118, BAND_NULL, 0, 0, 0, FALSE}
	  ,			/* CH_SET_UNII_MID_NA */
	  {121, BAND_NULL, 0, 0, 0, FALSE}
	  ,			/* CH_SET_UNII_WW_NA */
	  {125, BAND_NULL, 0, 0, 0, FALSE}
	  ,			/* CH_SET_UNII_UPPER_NA */
	  {0, BAND_NULL, 0, 0, 0, FALSE}
	 }
	}
	,
	{
	 (PUINT_16) g_u2CountryGroup1, sizeof(g_u2CountryGroup1) / 2,
	 {
	  {81, BAND_2G4, CHNL_SPAN_5, 1, 11, FALSE}
	  ,			/* CH_SET_2G4_1_11 */

	  {115, BAND_5G, CHNL_SPAN_20, 36, 4, FALSE}
	  ,			/* CH_SET_UNII_LOW_36_48 */
	  {118, BAND_5G, CHNL_SPAN_20, 52, 4, TRUE}
	  ,			/* CH_SET_UNII_MID_52_64 */
	  {121, BAND_5G, CHNL_SPAN_20, 100, 12, TRUE}
	  ,			/* CH_SET_UNII_WW_100_144 */
	  {125, BAND_5G, CHNL_SPAN_20, 149, 5, FALSE}
	  ,			/* CH_SET_UNII_UPPER_149_165 */
	  {0, BAND_NULL, 0, 0, 0, FALSE}
	 }
	}
	,
	{
	 (PUINT_16) g_u2CountryGroup2, sizeof(g_u2CountryGroup2) / 2,
	 {
	  {81, BAND_2G4, CHNL_SPAN_5, 1, 13, FALSE}
	  ,			/* CH_SET_2G4_1_13 */

	  {115, BAND_5G, CHNL_SPAN_20, 36, 4, FALSE}
	  ,			/* CH_SET_UNII_LOW_36_48 */
	  {118, BAND_5G, CHNL_SPAN_20, 52, 4, TRUE}
	  ,			/* CH_SET_UNII_MID_52_64 */
	  {121, BAND_5G, CHNL_SPAN_20, 100, 12, TRUE}
	  ,			/* CH_SET_UNII_WW_100_144 */
	  {125, BAND_5G, CHNL_SPAN_20, 149, 5, FALSE}
	  ,			/* CH_SET_UNII_UPPER_149_165 */
	  {0, BAND_NULL, 0, 0, 0, FALSE}
	 }
	}
	,
	{
	 (PUINT_16) g_u2CountryGroup3, sizeof(g_u2CountryGroup3) / 2,
	 {
	  {81, BAND_2G4, CHNL_SPAN_5, 1, 13, FALSE}
	  ,			/* CH_SET_2G4_1_13 */

	  {115, BAND_5G, CHNL_SPAN_20, 36, 4, FALSE}
	  ,			/* CH_SET_UNII_LOW_36_48 */
	  {118, BAND_5G, CHNL_SPAN_20, 52, 4, TRUE}
	  ,			/* CH_SET_UNII_MID_52_64 */
	  {121, BAND_5G, CHNL_SPAN_20, 100, 12, TRUE}
	  ,			/* CH_SET_UNII_WW_100_144 */
	  {125, BAND_5G, CHNL_SPAN_20, 149, 4, FALSE}
	  ,			/* CH_SET_UNII_UPPER_149_161 */
	  {0, BAND_NULL, 0, 0, 0, FALSE}
	 }
	}
	,
	{
	 (PUINT_16) g_u2CountryGroup4, sizeof(g_u2CountryGroup4) / 2,
	 {
	  {81, BAND_2G4, CHNL_SPAN_5, 1, 13, FALSE}
	  ,			/* CH_SET_2G4_1_13 */

	  {115, BAND_5G, CHNL_SPAN_20, 36, 4, FALSE}
	  ,			/* CH_SET_UNII_LOW_36_48 */
	  {118, BAND_5G, CHNL_SPAN_20, 52, 4, TRUE}
	  ,			/* CH_SET_UNII_MID_52_64 */
	  {121, BAND_5G, CHNL_SPAN_20, 100, 12, TRUE}
	  ,			/* CH_SET_UNII_WW_100_144 */
	  {125, BAND_NULL, 0, 0, 0, FALSE}
	  ,			/* CH_SET_UNII_UPPER_NA */
	  {0, BAND_NULL, 0, 0, 0, FALSE}
	 }
	}
	,
	{
	 (PUINT_16) g_u2CountryGroup5, sizeof(g_u2CountryGroup5) / 2,
	 {
	  {81, BAND_2G4, CHNL_SPAN_5, 1, 13, FALSE}
	  ,			/* CH_SET_2G4_1_13 */

	  {115, BAND_5G, CHNL_SPAN_20, 36, 4, FALSE}
	  ,			/* CH_SET_UNII_LOW_36_48 */
	  {118, BAND_5G, CHNL_SPAN_20, 52, 4, TRUE}
	  ,			/* CH_SET_UNII_MID_52_64 */
	  {121, BAND_5G, CHNL_SPAN_20, 100, 11, TRUE}
	  ,			/* CH_SET_UNII_WW_100_140 */
	  {125, BAND_NULL, 0, 0, 0, FALSE}
	  ,			/* CH_SET_UNII_UPPER_NA */
	  {0, BAND_NULL, 0, 0, 0, FALSE}
	 }
	}
	,
	{
	 (PUINT_16) g_u2CountryGroup6, sizeof(g_u2CountryGroup6) / 2,
	 {
	  {81, BAND_2G4, CHNL_SPAN_5, 1, 13, FALSE}
	  ,			/* CH_SET_2G4_1_13 */
	  {82, BAND_2G4, CHNL_SPAN_5, 14, 1, FALSE}
	  ,			/* CH_SET_2G4_14_14 */
	  {115, BAND_5G, CHNL_SPAN_20, 36, 4, FALSE}
	  ,			/* CH_SET_UNII_LOW_36_48 */
	  {118, BAND_5G, CHNL_SPAN_20, 52, 4, TRUE}
	  ,			/* CH_SET_UNII_MID_52_64 */
	  {121, BAND_5G, CHNL_SPAN_20, 100, 11, TRUE}
	  ,			/* CH_SET_UNII_WW_100_140 */
	  {125, BAND_NULL, 0, 0, 0, FALSE}
	  ,			/* CH_SET_UNII_UPPER_NA */
	 }
	}
	,
	{
	 (PUINT_16) g_u2CountryGroup7, sizeof(g_u2CountryGroup7) / 2,
	 {
	  {81, BAND_2G4, CHNL_SPAN_5, 1, 13, FALSE}
	  ,			/* CH_SET_2G4_1_13 */

	  {115, BAND_5G, CHNL_SPAN_20, 36, 4, FALSE}
	  ,			/* CH_SET_UNII_LOW_36_48 */
	  {118, BAND_5G, CHNL_SPAN_20, 52, 4, TRUE}
	  ,			/* CH_SET_UNII_MID_52_64 */
	  {121, BAND_NULL, 0, 0, 0, FALSE}
	  ,			/* CH_SET_UNII_WW_NA */
	  {125, BAND_NULL, 0, 0, 0, FALSE}
	  ,			/* CH_SET_UNII_UPPER_NA */
	  {0, BAND_NULL, 0, 0, 0, FALSE}
	 }
	}
	,
	{
	 (PUINT_16) g_u2CountryGroup8, sizeof(g_u2CountryGroup8) / 2,
	 {
	  {81, BAND_2G4, CHNL_SPAN_5, 1, 13, FALSE}
	  ,			/* CH_SET_2G4_1_13 */

	  {115, BAND_5G, CHNL_SPAN_20, 36, 4, FALSE}
	  ,			/* CH_SET_UNII_LOW_36_48 */
	  {118, BAND_5G, CHNL_SPAN_20, 52, 4, TRUE}
	  ,			/* CH_SET_UNII_MID_52_64 */
	  {121, BAND_NULL, 0, 0, 0, FALSE}
	  ,			/* CH_SET_UNII_WW_NA */
	  {125, BAND_5G, CHNL_SPAN_20, 149, 4, FALSE}
	  ,			/* CH_SET_UNII_UPPER_149_161 */
	  {0, BAND_NULL, 0, 0, 0, FALSE}
	 }
	}
	,
	{
	 (PUINT_16) g_u2CountryGroup9, sizeof(g_u2CountryGroup9) / 2,
	 {
	  {81, BAND_2G4, CHNL_SPAN_5, 1, 13, FALSE}
	  ,			/* CH_SET_2G4_1_13 */

	  {115, BAND_5G, CHNL_SPAN_20, 36, 4, FALSE}
	  ,			/* CH_SET_UNII_LOW_36_48 */
	  {118, BAND_NULL, 0, 0, 0, FALSE}
	  ,			/* CH_SET_UNII_MID_NA */
	  {121, BAND_NULL, 0, 0, 0, FALSE}
	  ,			/* CH_SET_UNII_WW_NA */
	  {125, BAND_NULL, 0, 0, 0, FALSE}
	  ,			/* CH_SET_UNII_UPPER_NA */
	  {0, BAND_NULL, 0, 0, 0, FALSE}
	 }
	}
	,
	{
	 (PUINT_16) g_u2CountryGroup10, sizeof(g_u2CountryGroup10) / 2,
	 {
	  {81, BAND_2G4, CHNL_SPAN_5, 1, 13, FALSE}
	  ,			/* CH_SET_2G4_1_13 */

	  {115, BAND_5G, CHNL_SPAN_20, 36, 4, FALSE}
	  ,			/* CH_SET_UNII_LOW_36_48 */
	  {118, BAND_5G, CHNL_SPAN_20, 52, 4, TRUE}
	  ,			/* CH_SET_UNII_MID_52_64 */
	  {121, BAND_NULL, 0, 0, 0, FALSE}
	  ,			/* CH_SET_UNII_WW_NA */
	  {125, BAND_5G, CHNL_SPAN_20, 149, 5, FALSE}
	  ,			/* CH_SET_UNII_UPPER_149_165 */
	  {0, BAND_NULL, 0, 0, 0, FALSE}
	 }
	}
	,
	{
	 (PUINT_16) g_u2CountryGroup11, sizeof(g_u2CountryGroup11) / 2,
	 {
	  {81, BAND_2G4, CHNL_SPAN_5, 1, 13, FALSE}
	  ,			/* CH_SET_2G4_1_13 */

	  {115, BAND_5G, CHNL_SPAN_20, 36, 4, FALSE}
	  ,			/* CH_SET_UNII_LOW_36_48 */
	  {118, BAND_NULL, 0, 0, 0, FALSE}
	  ,			/* CH_SET_UNII_MID_NA */
	  {121, BAND_NULL, 0, 0, 0, FALSE}
	  ,			/* CH_SET_UNII_WW_NA */
	  {125, BAND_5G, CHNL_SPAN_20, 149, 5, FALSE}
	  ,			/* CH_SET_UNII_UPPER_149_165 */
	  {0, BAND_NULL, 0, 0, 0, FALSE}
	 }
	}
	,
	{
	 (PUINT_16) g_u2CountryGroup12, sizeof(g_u2CountryGroup12) / 2,
	 {
	  {81, BAND_2G4, CHNL_SPAN_5, 1, 13, FALSE}
	  ,			/* CH_SET_2G4_1_13 */

	  {115, BAND_NULL, 0, 0, 0, FALSE}
	  ,			/* CH_SET_UNII_LOW_NA */
	  {118, BAND_NULL, 0, 0, 0, FALSE}
	  ,			/* CH_SET_UNII_MID_NA */
	  {121, BAND_NULL, 0, 0, 0, FALSE}
	  ,			/* CH_SET_UNII_WW_NA */
	  {125, BAND_5G, CHNL_SPAN_20, 149, 5, FALSE}
	  ,			/* CH_SET_UNII_UPPER_149_165 */
	  {0, BAND_NULL, 0, 0, 0, FALSE}
	 }
	}
	,
	{
	 (PUINT_16) g_u2CountryGroup13, sizeof(g_u2CountryGroup13) / 2,
	 {
	  {81, BAND_2G4, CHNL_SPAN_5, 1, 13, FALSE}
	  ,			/* CH_SET_2G4_1_13 */

	  {115, BAND_NULL, 0, 0, 0, FALSE}
	  ,			/* CH_SET_UNII_LOW_NA */
	  {118, BAND_NULL, 0, 0, 0, FALSE}
	  ,			/* CH_SET_UNII_MID_NA */
	  {121, BAND_NULL, 0, 0, 0, FALSE}
	  ,			/* CH_SET_UNII_WW_NA */
	  {125, BAND_5G, CHNL_SPAN_20, 149, 4, FALSE}
	  ,			/* CH_SET_UNII_UPPER_149_161 */
	  {0, BAND_NULL, 0, 0, 0, FALSE}
	 }
	}
	,
	{
	 (PUINT_16) g_u2CountryGroup14, sizeof(g_u2CountryGroup14) / 2,
	 {
	  {81, BAND_2G4, CHNL_SPAN_5, 1, 13, FALSE}
	  ,			/* CH_SET_2G4_1_13 */

	  {115, BAND_NULL, 0, 0, 0, FALSE}
	  ,			/* CH_SET_UNII_LOW_NA */
	  {118, BAND_5G, CHNL_SPAN_20, 52, 4, TRUE}
	  ,			/* CH_SET_UNII_MID_52_64 */
	  {121, BAND_5G, CHNL_SPAN_20, 100, 11, TRUE}
	  ,			/* CH_SET_UNII_WW_100_140 */
	  {125, BAND_5G, CHNL_SPAN_20, 149, 5, FALSE}
	  ,			/* CH_SET_UNII_UPPER_149_165 */
	  {0, BAND_NULL, 0, 0, 0, FALSE}
	 }
	}
	,
	{
	 (PUINT_16) g_u2CountryGroup15, sizeof(g_u2CountryGroup15) / 2,
	 {
	  {81, BAND_2G4, CHNL_SPAN_5, 1, 13, FALSE}
	  ,			/* CH_SET_2G4_1_13 */

	  {115, BAND_5G, CHNL_SPAN_20, 36, 4, TRUE}
	  ,			/* CH_SET_UNII_LOW_36_48 */ /* Indoor */
	  {118, BAND_5G, CHNL_SPAN_20, 52, 4, TRUE}
	  ,			/* CH_SET_UNII_MID_52_64 */ /* Indoor */
	  {121, BAND_5G, CHNL_SPAN_20, 100, 11, TRUE}
	  ,			/* CH_SET_UNII_WW_100_140 */
	  {125, BAND_5G, CHNL_SPAN_20, 149, 5, FALSE}
	  ,			/* CH_SET_UNII_UPPER_149_165 */
	  {0, BAND_NULL, 0, 0, 0, FALSE}
	 }
	}
	,
	{
	 (PUINT_16) g_u2CountryGroup16, sizeof(g_u2CountryGroup16) / 2,
	 {
	  {81, BAND_2G4, CHNL_SPAN_5, 1, 11, FALSE}
	  ,			/* CH_SET_2G4_1_11 */

	  {115, BAND_5G, CHNL_SPAN_20, 36, 4, FALSE}
	  ,			/* CH_SET_UNII_LOW_36_48 */
	  {118, BAND_5G, CHNL_SPAN_20, 52, 4, TRUE}
	  ,			/* CH_SET_UNII_MID_52_64 */
	  {121, BAND_5G, CHNL_SPAN_20, 100, 11, TRUE}
	  ,			/* CH_SET_UNII_WW_100_140 */
	  {125, BAND_5G, CHNL_SPAN_20, 149, 5, FALSE}
	  ,			/* CH_SET_UNII_UPPER_149_165 */
	  {0, BAND_NULL, 0, 0, 0, FALSE}
	 }
	}
	,
	{
	 (PUINT_16) g_u2CountryGroup17, sizeof(g_u2CountryGroup17) / 2,
	 {
	  {81, BAND_2G4, CHNL_SPAN_5, 1, 11, FALSE}
	  ,			/* CH_SET_2G4_1_11 */

	  {115, BAND_5G, CHNL_SPAN_20, 36, 4, FALSE}
	  ,			/* CH_SET_UNII_LOW_36_48 */
	  {118, BAND_5G, CHNL_SPAN_20, 52, 4, TRUE}
	  ,			/* CH_SET_UNII_MID_52_64 */ /* Indoor */
	  {121, BAND_5G, CHNL_SPAN_20, 100, 11, TRUE}
	  ,			/* CH_SET_UNII_WW_100_140 */
	  {125, BAND_5G, CHNL_SPAN_20, 149, 5, FALSE}
	  ,			/* CH_SET_UNII_UPPER_149_165 */
	  {0, BAND_NULL, 0, 0, 0, FALSE}
	 }
	}
	,
	{
	 (PUINT_16) g_u2CountryGroup18, sizeof(g_u2CountryGroup18) / 2,
	 {
	  {81, BAND_2G4, CHNL_SPAN_5, 1, 11, FALSE}
	  ,			/* CH_SET_2G4_1_11 */

	  {115, BAND_5G, CHNL_SPAN_20, 36, 4, FALSE}
	  ,			/* CH_SET_UNII_LOW_36_48 */
	  {118, BAND_5G, CHNL_SPAN_20, 52, 4, TRUE}
	  ,			/* CH_SET_UNII_MID_52_64 */
	  {121, BAND_5G, CHNL_SPAN_20, 100, 5, TRUE}
	  ,			/* CH_SET_UNII_WW_100_116 */
	  {121, BAND_5G, CHNL_SPAN_20, 132, 4, TRUE}
	  ,			/* CH_SET_UNII_WW_132_144 */
	  {125, BAND_5G, CHNL_SPAN_20, 149, 5, FALSE}
				/* CH_SET_UNII_UPPER_149_165 */
	 }
	}
	,
	{
	 (PUINT_16) g_u2CountryGroup19, sizeof(g_u2CountryGroup19) / 2,
	 {
	  {81, BAND_2G4, CHNL_SPAN_5, 1, 13, FALSE}
	  ,			/* CH_SET_2G4_1_13 */

	  {115, BAND_5G, CHNL_SPAN_20, 36, 4, FALSE}
	  ,			/* CH_SET_UNII_LOW_36_48 */
	  {118, BAND_5G, CHNL_SPAN_20, 52, 4, TRUE}
	  ,			/* CH_SET_UNII_MID_52_64 */
	  {121, BAND_5G, CHNL_SPAN_20, 100, 12, TRUE}
	  ,			/* CH_SET_UNII_WW_100_144 */
	  {125, BAND_5G, CHNL_SPAN_20, 149, 5, FALSE}
	  ,			/* CH_SET_UNII_UPPER_149_165 */
	  {0, BAND_NULL, 0, 0, 0, FALSE}
	 }
	}
	,
	{
	 /* Note: Default group if no matched country code */
	 (PUINT_16) g_u2CountryGroup20, sizeof(g_u2CountryGroup20) / 2,
	 {
	  {81, BAND_2G4, CHNL_SPAN_5, 1, 13, FALSE}
	  ,			/* CH_SET_2G4_1_13 */

	  {115, BAND_5G, CHNL_SPAN_20, 36, 4, FALSE}
	  ,			/* CH_SET_UNII_LOW_36_48 */
	  {118, BAND_5G, CHNL_SPAN_20, 52, 4, TRUE}
	  ,			/* CH_SET_UNII_MID_52_64 */
	  {121, BAND_5G, CHNL_SPAN_20, 100, 12, TRUE}
	  ,			/* CH_SET_UNII_WW_100_144 */
	  {125, BAND_5G, CHNL_SPAN_20, 149, 5, FALSE}
	  ,			/* CH_SET_UNII_UPPER_149_165 */
	  {0, BAND_NULL, 0, 0, 0, FALSE}
	 }
	}
};

static const UINT_16 g_u2CountryGroup0_Passive[] = {
	COUNTRY_CODE_TW
};

DOMAIN_INFO_ENTRY arSupportedRegDomains_Passive[] = {
	{
	 (PUINT_16) g_u2CountryGroup0_Passive, sizeof(g_u2CountryGroup0_Passive) / 2,
	 {
	  {81, BAND_2G4, CHNL_SPAN_5, 1, 0, FALSE}
	  ,			/* CH_SET_2G4_1_14_NA */
	  {82, BAND_2G4, CHNL_SPAN_5, 14, 0, FALSE}
	  ,

	  {115, BAND_5G, CHNL_SPAN_20, 36, 0, FALSE}
	  ,			/* CH_SET_UNII_LOW_NA */
	  {118, BAND_5G, CHNL_SPAN_20, 52, 4, TRUE}
	  ,			/* CH_SET_UNII_MID_52_64 */
	  {121, BAND_5G, CHNL_SPAN_20, 100, 11, TRUE}
	  ,			/* CH_SET_UNII_WW_100_140 */
	  {125, BAND_5G, CHNL_SPAN_20, 149, 0, FALSE}
	  ,			/* CH_SET_UNII_UPPER_NA */
	 }
	}
	,
	{
	 /* Default passive scan channel table: ch52~64, ch100~144 */
	 NULL, 0,
	 {
	  {81, BAND_2G4, CHNL_SPAN_5, 1, 0, FALSE}
	  ,			/* CH_SET_2G4_1_14_NA */
	  {82, BAND_2G4, CHNL_SPAN_5, 14, 0, FALSE}
	  ,

	  {115, BAND_5G, CHNL_SPAN_20, 36, 0, FALSE}
	  ,			/* CH_SET_UNII_LOW_NA */
	  {118, BAND_5G, CHNL_SPAN_20, 52, 4, TRUE}
	  ,			/* CH_SET_UNII_MID_52_64 */
	  {121, BAND_5G, CHNL_SPAN_20, 100, 12, TRUE}
	  ,			/* CH_SET_UNII_WW_100_144 */
	  {125, BAND_5G, CHNL_SPAN_20, 149, 0, FALSE}
	  ,			/* CH_SET_UNII_UPPER_NA */
	 }
	}
};

SUBBAND_CHANNEL_T g_rRlmSubBand[] = {

	{BAND_2G4_LOWER_BOUND, BAND_2G4_UPPER_BOUND, 1, 0}
	,			/* 2.4G */
	{UNII1_LOWER_BOUND, UNII1_UPPER_BOUND, 2, 0}
	,			/* ch36,38,40,..,48 */
	{UNII2A_LOWER_BOUND, UNII2A_UPPER_BOUND, 2, 0}
	,			/* ch52,54,56,..,64 */
	{UNII2C_LOWER_BOUND, UNII2C_UPPER_BOUND, 2, 0}
	,			/* ch100,102,104,...,144 */
	{UNII3_LOWER_BOUND, UNII3_UPPER_BOUND, 2, 0}
				/* ch149,151,153,....,173 */
};

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
* \brief
*
* \param[in/out]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
P_DOMAIN_INFO_ENTRY rlmDomainGetDomainInfo(P_ADAPTER_T prAdapter)
{
#define REG_DOMAIN_DEF_IDX             20 /* Default regulatory domain */
#define REG_DOMAIN_GROUP_NUM  \
	(sizeof(arSupportedRegDomains) / sizeof(DOMAIN_INFO_ENTRY))

	P_DOMAIN_INFO_ENTRY prDomainInfo;
	P_REG_INFO_T prRegInfo;
	UINT_16 u2TargetCountryCode;
	UINT_16 i, j;

	ASSERT(prAdapter);

	if (prAdapter->prDomainInfo)
		return prAdapter->prDomainInfo;

	prRegInfo = &prAdapter->prGlueInfo->rRegInfo;

	DBGLOG(RLM, TRACE, "eRegChannelListMap=%d, u2CountryCode=0x%04x\n",
			   prRegInfo->eRegChannelListMap,
			   prAdapter->rWifiVar.rConnSettings.u2CountryCode);

	/*
	* Domain info can be specified by given idx of arSupportedRegDomains table,
	* customized, or searched by country code,
	* only one is set among these three methods in NVRAM.
	*/
	if (prRegInfo->eRegChannelListMap == REG_CH_MAP_TBL_IDX &&
	    prRegInfo->ucRegChannelListIndex < REG_DOMAIN_GROUP_NUM) {
		/* by given table idx */
		DBGLOG(RLM, TRACE, "ucRegChannelListIndex=%d\n", prRegInfo->ucRegChannelListIndex);
		prDomainInfo = &arSupportedRegDomains[prRegInfo->ucRegChannelListIndex];
	} else if (prRegInfo->eRegChannelListMap == REG_CH_MAP_CUSTOMIZED) {
		/* by customized */
		prDomainInfo = &prRegInfo->rDomainInfo;
	} else {
		/* by country code */
		u2TargetCountryCode = prAdapter->rWifiVar.rConnSettings.u2CountryCode;

		for (i = 0; i < REG_DOMAIN_GROUP_NUM; i++) {
			prDomainInfo = &arSupportedRegDomains[i];

			if ((prDomainInfo->u4CountryNum && prDomainInfo->pu2CountryGroup) ||
			    prDomainInfo->u4CountryNum == 0) {
				for (j = 0; j < prDomainInfo->u4CountryNum; j++) {
					if (prDomainInfo->pu2CountryGroup[j] == u2TargetCountryCode)
						break;
				}
				if (j < prDomainInfo->u4CountryNum)
					break;	/* Found */
			}
		}

		/* If no matched country code, use the default regulatory domain */
		if (i >= REG_DOMAIN_GROUP_NUM) {
			DBGLOG(RLM, INFO, "No matched country code, use the default regulatory domain\n");
			prDomainInfo = &arSupportedRegDomains[REG_DOMAIN_DEF_IDX];
		}
	}

	prAdapter->prDomainInfo = prDomainInfo;
	return prDomainInfo;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief Retrieve the supported channel list of specified band
*
* \param[in/out] eSpecificBand:   BAND_2G4, BAND_5G or BAND_NULL (both 2.4G and 5G)
*                fgNoDfs:         whether to exculde DFS channels
*                ucMaxChannelNum: max array size
*                pucNumOfChannel: pointer to returned channel number
*                paucChannelList: pointer to returned channel list array
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID rlmDomainGetChnlList(P_ADAPTER_T prAdapter,
			  ENUM_BAND_T eSpecificBand, BOOLEAN fgNoDfs,
			  UINT_8 ucMaxChannelNum, PUINT_8 pucNumOfChannel, P_RF_CHANNEL_INFO_T paucChannelList)
{
	UINT_8 i, j, ucNum;
	P_DOMAIN_SUBBAND_INFO prSubband;
	P_DOMAIN_INFO_ENTRY prDomainInfo;

	ASSERT(prAdapter);
	ASSERT(paucChannelList);
	ASSERT(pucNumOfChannel);

	prDomainInfo = rlmDomainGetDomainInfo(prAdapter);
	ASSERT(prDomainInfo);

	ucNum = 0;
	for (i = 0; i < MAX_SUBBAND_NUM; i++) {
		prSubband = &prDomainInfo->rSubBand[i];

		if (prSubband->ucBand == BAND_NULL || prSubband->ucBand >= BAND_NUM ||
		    (prSubband->ucBand == BAND_5G && !prAdapter->fgEnable5GBand))
			continue;

		if (fgNoDfs == TRUE && prSubband->fgDfs == TRUE)
			continue;

		if (eSpecificBand == BAND_NULL || prSubband->ucBand == eSpecificBand) {
			for (j = 0; j < prSubband->ucNumChannels; j++) {
				if (ucNum >= ucMaxChannelNum)
					break;
				paucChannelList[ucNum].eBand = prSubband->ucBand;
				paucChannelList[ucNum].ucChannelNum =
				    prSubband->ucFirstChannelNum + j * prSubband->ucChannelSpan;
				ucNum++;
			}
		}
	}

	*pucNumOfChannel = ucNum;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief Retrieve DFS channels from 5G band
*
* \param[in/out] ucMaxChannelNum: max array size
*                pucNumOfChannel: pointer to returned channel number
*                paucChannelList: pointer to returned channel list array
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID rlmDomainGetDfsChnls(P_ADAPTER_T prAdapter,
			  UINT_8 ucMaxChannelNum, PUINT_8 pucNumOfChannel, P_RF_CHANNEL_INFO_T paucChannelList)
{
	UINT_8 i, j, ucNum;
	P_DOMAIN_SUBBAND_INFO prSubband;
	P_DOMAIN_INFO_ENTRY prDomainInfo;

	ASSERT(prAdapter);
	ASSERT(paucChannelList);
	ASSERT(pucNumOfChannel);

	prDomainInfo = rlmDomainGetDomainInfo(prAdapter);
	ASSERT(prDomainInfo);

	ucNum = 0;
	for (i = 0; i < MAX_SUBBAND_NUM; i++) {
		prSubband = &prDomainInfo->rSubBand[i];

		if (prSubband->ucBand == BAND_5G) {
			if (!prAdapter->fgEnable5GBand)
				continue;

			if (prSubband->fgDfs == TRUE) {
				for (j = 0; j < prSubband->ucNumChannels; j++) {
					if (ucNum >= ucMaxChannelNum)
						break;
					paucChannelList[ucNum].eBand = prSubband->ucBand;
					paucChannelList[ucNum].ucChannelNum =
					    prSubband->ucFirstChannelNum + j * prSubband->ucChannelSpan;
					ucNum++;
				}
			}
		}
	}

	*pucNumOfChannel = ucNum;
}

/*----------------------------------------------------------------------------*/
/*!
* @brief
*
* @param[in]
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
VOID rlmDomainSendCmd(P_ADAPTER_T prAdapter)
{
	rlmDomainSendPassiveScanInfoCmd(prAdapter);
	rlmDomainSendDomainInfoCmd(prAdapter);
#if CFG_SUPPORT_PWR_LIMIT_COUNTRY
	rlmDomainSendPwrLimitCmd(prAdapter);
#endif
}

/*----------------------------------------------------------------------------*/
/*!
* @brief
*
* @param[in]
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
VOID rlmDomainSendDomainInfoCmd(P_ADAPTER_T prAdapter)
{
	P_DOMAIN_INFO_ENTRY prDomainInfo;
	P_CMD_SET_DOMAIN_INFO_T prCmd;
	P_DOMAIN_SUBBAND_INFO prSubBand;
	UINT_8 i;

	prDomainInfo = rlmDomainGetDomainInfo(prAdapter);
	ASSERT(prDomainInfo);

	prCmd = cnmMemAlloc(prAdapter, RAM_TYPE_BUF, sizeof(CMD_SET_DOMAIN_INFO_T));
	if (!prCmd) {
		DBGLOG(RLM, ERROR, "Alloc cmd buffer failed\n");
		return;
	}
	kalMemZero(prCmd, sizeof(CMD_SET_DOMAIN_INFO_T));

	prCmd->u2CountryCode = prAdapter->rWifiVar.rConnSettings.u2CountryCode;
	prCmd->u2IsSetPassiveScan = 0;
	prCmd->uc2G4Bandwidth = prAdapter->rWifiVar.rConnSettings.uc2G4BandwidthMode;
	prCmd->uc5GBandwidth = prAdapter->rWifiVar.rConnSettings.uc5GBandwidthMode;
	prCmd->aucReserved[0] = 0;
	prCmd->aucReserved[1] = 0;

	for (i = 0; i < MAX_SUBBAND_NUM; i++) {
		prSubBand = &prDomainInfo->rSubBand[i];

		prCmd->rSubBand[i].ucRegClass = prSubBand->ucRegClass;
		prCmd->rSubBand[i].ucBand = prSubBand->ucBand;

		if (prSubBand->ucBand != BAND_NULL && prSubBand->ucBand < BAND_NUM) {
			prCmd->rSubBand[i].ucChannelSpan = prSubBand->ucChannelSpan;
			prCmd->rSubBand[i].ucFirstChannelNum = prSubBand->ucFirstChannelNum;
			prCmd->rSubBand[i].ucNumChannels = prSubBand->ucNumChannels;
		}
	}

	/* Set domain info to chip */
	wlanSendSetQueryCmd(prAdapter, /* prAdapter */
			    CMD_ID_SET_DOMAIN_INFO, /* ucCID */
			    TRUE,  /* fgSetQuery */
			    FALSE, /* fgNeedResp */
			    FALSE, /* fgIsOid */
			    NULL,  /* pfCmdDoneHandler */
			    NULL,  /* pfCmdTimeoutHandler */
			    sizeof(CMD_SET_DOMAIN_INFO_T), /* u4SetQueryInfoLen */
			    (PUINT_8) prCmd, /* pucInfoBuffer */
			    NULL,  /* pvSetQueryBuffer */
			    0      /* u4SetQueryBufferLen */
	    );

	cnmMemFree(prAdapter, prCmd);
}

/*----------------------------------------------------------------------------*/
/*!
* @brief
*
* @param[in]
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
VOID rlmDomainSendPassiveScanInfoCmd(P_ADAPTER_T prAdapter)
{
#define REG_DOMAIN_PASSIVE_DEF_IDX	1
#define REG_DOMAIN_PASSIVE_GROUP_NUM \
	(sizeof(arSupportedRegDomains_Passive) / sizeof(DOMAIN_INFO_ENTRY))

	P_DOMAIN_INFO_ENTRY prDomainInfo;
	P_CMD_SET_DOMAIN_INFO_T prCmd;
	P_DOMAIN_SUBBAND_INFO prSubBand;
	UINT_16 u2TargetCountryCode;
	UINT_8 i, j;

	prCmd = cnmMemAlloc(prAdapter, RAM_TYPE_BUF, sizeof(CMD_SET_DOMAIN_INFO_T));
	if (!prCmd) {
		DBGLOG(RLM, ERROR, "Alloc cmd buffer failed\n");
		return;
	}
	kalMemZero(prCmd, sizeof(CMD_SET_DOMAIN_INFO_T));

	prCmd->u2CountryCode = prAdapter->rWifiVar.rConnSettings.u2CountryCode;
	prCmd->u2IsSetPassiveScan = 1;
	prCmd->uc2G4Bandwidth = prAdapter->rWifiVar.rConnSettings.uc2G4BandwidthMode;
	prCmd->uc5GBandwidth = prAdapter->rWifiVar.rConnSettings.uc5GBandwidthMode;
	prCmd->aucReserved[0] = 0;
	prCmd->aucReserved[1] = 0;

	DBGLOG(RLM, TRACE, "u2CountryCode=0x%04x\n", prAdapter->rWifiVar.rConnSettings.u2CountryCode);

	u2TargetCountryCode = prAdapter->rWifiVar.rConnSettings.u2CountryCode;

	for (i = 0; i < REG_DOMAIN_PASSIVE_GROUP_NUM; i++) {
		prDomainInfo = &arSupportedRegDomains_Passive[i];

		for (j = 0; j < prDomainInfo->u4CountryNum; j++) {
			if (prDomainInfo->pu2CountryGroup[j] == u2TargetCountryCode)
				break;
		}
		if (j < prDomainInfo->u4CountryNum)
			break;	/* Found */
	}

	if (i >= REG_DOMAIN_PASSIVE_GROUP_NUM)
		prDomainInfo = &arSupportedRegDomains_Passive[REG_DOMAIN_PASSIVE_DEF_IDX];

	for (i = 0; i < MAX_SUBBAND_NUM; i++) {
		prSubBand = &prDomainInfo->rSubBand[i];

		prCmd->rSubBand[i].ucRegClass = prSubBand->ucRegClass;
		prCmd->rSubBand[i].ucBand = prSubBand->ucBand;

		if (prSubBand->ucBand != BAND_NULL && prSubBand->ucBand < BAND_NUM) {
			prCmd->rSubBand[i].ucChannelSpan = prSubBand->ucChannelSpan;
			prCmd->rSubBand[i].ucFirstChannelNum = prSubBand->ucFirstChannelNum;
			prCmd->rSubBand[i].ucNumChannels = prSubBand->ucNumChannels;
		}
	}

	/* Set passive scan channel info to chip */
	wlanSendSetQueryCmd(prAdapter, /* prAdapter */
			    CMD_ID_SET_DOMAIN_INFO, /* ucCID */
			    TRUE,  /* fgSetQuery */
			    FALSE, /* fgNeedResp */
			    FALSE, /* fgIsOid */
			    NULL,  /* pfCmdDoneHandler */
			    NULL,  /* pfCmdTimeoutHandler */
			    sizeof(CMD_SET_DOMAIN_INFO_T), /* u4SetQueryInfoLen */
			    (PUINT_8) prCmd, /* pucInfoBuffer */
			    NULL,  /* pvSetQueryBuffer */
			    0      /* u4SetQueryBufferLen */
	    );

	cnmMemFree(prAdapter, prCmd);
}

/*----------------------------------------------------------------------------*/
/*!
* \brief
*
* \param[in/out]
*
* \return TRUE  Legal channel
*         FALSE Illegal channel for current regulatory domain
*/
/*----------------------------------------------------------------------------*/
BOOLEAN rlmDomainIsLegalChannel(P_ADAPTER_T prAdapter, ENUM_BAND_T eBand, UINT_8 ucChannel)
{
	UINT_8 i, j;
	P_DOMAIN_SUBBAND_INFO prSubband;
	P_DOMAIN_INFO_ENTRY prDomainInfo;

	prDomainInfo = rlmDomainGetDomainInfo(prAdapter);
	ASSERT(prDomainInfo);

	for (i = 0; i < MAX_SUBBAND_NUM; i++) {
		prSubband = &prDomainInfo->rSubBand[i];

		if (prSubband->ucBand == BAND_5G && !prAdapter->fgEnable5GBand)
			continue;

		if (prSubband->ucBand == eBand) {
			for (j = 0; j < prSubband->ucNumChannels; j++) {
				if ((prSubband->ucFirstChannelNum + j * prSubband->ucChannelSpan)
				    == ucChannel) {
					return TRUE;
				}
			}
		}
	}

	return FALSE;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief
*
* \param[in/out]
*
* \return none
*/
/*----------------------------------------------------------------------------*/

UINT_32 rlmDomainSupOperatingClassIeFill(PUINT_8 pBuf)
{
	/*
	 * The Country element should only be included for Status Code 0 (Successful).
	 */
	UINT_32 u4IeLen;
	UINT_8 aucClass[12] = { 0x01, 0x02, 0x03, 0x05, 0x16, 0x17, 0x19, 0x1b,
		0x1c, 0x1e, 0x20, 0x21
	};

	/*
	 * The Supported Operating Classes element is used by a STA to advertise the
	 * operating classes that it is capable of operating with in this country.
	 *
	 * The Country element (see 8.4.2.10) allows a STA to configure its PHY and MAC
	 * for operation when the operating triplet of Operating Extension Identifier,
	 * Operating Class, and Coverage Class fields is present.
	 */
	SUP_OPERATING_CLASS_IE(pBuf)->ucId = ELEM_ID_SUP_OPERATING_CLASS;
	SUP_OPERATING_CLASS_IE(pBuf)->ucLength = 1 + sizeof(aucClass);
	SUP_OPERATING_CLASS_IE(pBuf)->ucCur = 0x0c;	/* 0x51 */
	kalMemCopy(SUP_OPERATING_CLASS_IE(pBuf)->ucSup, aucClass, sizeof(aucClass));
	u4IeLen = (SUP_OPERATING_CLASS_IE(pBuf)->ucLength + 2);
	pBuf += u4IeLen;

	COUNTRY_IE(pBuf)->ucId = ELEM_ID_COUNTRY_INFO;
	COUNTRY_IE(pBuf)->ucLength = 6;
	COUNTRY_IE(pBuf)->aucCountryStr[0] = 0x55;
	COUNTRY_IE(pBuf)->aucCountryStr[1] = 0x53;
	COUNTRY_IE(pBuf)->aucCountryStr[2] = 0x20;
	COUNTRY_IE(pBuf)->arCountryStr[0].ucFirstChnlNum = 1;
	COUNTRY_IE(pBuf)->arCountryStr[0].ucNumOfChnl = 11;
	COUNTRY_IE(pBuf)->arCountryStr[0].cMaxTxPwrLv = 0x1e;
	u4IeLen += (COUNTRY_IE(pBuf)->ucLength + 2);

	return u4IeLen;
}

/*----------------------------------------------------------------------------*/
/*!
* @brief
*
* @param[in]
*
* @return (fgValid) : 0 -> inValid, 1 -> Valid
*/
/*----------------------------------------------------------------------------*/
BOOLEAN rlmDomainCheckChannelEntryValid(P_ADAPTER_T prAdapter, UINT_8 ucCentralCh)
{
	BOOLEAN fgValid = FALSE;
	UINT_8 ucTemp = 0;
	UINT_8 i;
	/*Check Power limit table channel efficient or not */

	for (i = POWER_LIMIT_2G4; i < POWER_LIMIT_SUBAND_NUM; i++) {
		if ((ucCentralCh >= g_rRlmSubBand[i].ucStartCh) && (ucCentralCh <= g_rRlmSubBand[i].ucEndCh))
			ucTemp = (ucCentralCh - g_rRlmSubBand[i].ucStartCh) % g_rRlmSubBand[i].ucInterval;
	}

#if 0
	/*2.4G, ex 1, 2, 3 */
	if (ucCentralCh >= BAND_2G4_LOWER_BOUND && ucCentralCh <= BAND_2G4_UPPER_BOUND)
		ucTemp = 0;
	/*FCC- Spec : Band UNII-1, ex 36, 38, 40.... */
	else if (ucCentralCh >= UNII1_LOWER_BOUND && ucCentralCh <= UNII1_UPPER_BOUND)
		ucTemp = (ucCentralCh - UNII1_LOWER_BOUND) % 2;
	/*FCC- Spec : Band UNII-2A, ex 52, 54, 56.... */
	else if (ucCentralCh >= UNII2A_LOWER_BOUND && ucCentralCh <= UNII2A_UPPER_BOUND)
		ucTemp = (ucCentralCh - UNII2A_LOWER_BOUND) % 2;
	/*FCC- Spec : Band UNII-2C, ex 100, 102, 104.... */
	else if (ucCentralCh >= UNII2C_LOWER_BOUND && ucCentralCh <= UNII2C_UPPER_BOUND)
		ucTemp = (ucCentralCh - UNII2C_LOWER_BOUND) % 2;
	/*FCC- Spec : Band UNII-3, ex 149, 151, 153... */
	else if (ucCentralCh >= UNII3_LOWER_BOUND && ucCentralCh <= UNII3_UPPER_BOUND)
		ucTemp = (ucCentralCh - UNII3_LOWER_BOUND) % 2;
#endif
	if (ucTemp == 0)
		fgValid = TRUE;
	return fgValid;

}

/*----------------------------------------------------------------------------*/
/*!
* \brief
*
* \param[in]
*
* \return
*/
/*----------------------------------------------------------------------------*/
UINT_8 rlmDomainGetCenterChannel(ENUM_BAND_T eBand, UINT_8 ucPriChannel, ENUM_CHNL_EXT_T eExtend)
{
	UINT_8 ucCenterChannel;

	if (eExtend == CHNL_EXT_SCA)
		ucCenterChannel = ucPriChannel + 2;
	else if (eExtend == CHNL_EXT_SCB)
		ucCenterChannel = ucPriChannel - 2;
	else
		ucCenterChannel = ucPriChannel;

	return ucCenterChannel;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief
*
* \param[in]
*
* \return
*/
/*----------------------------------------------------------------------------*/
BOOLEAN
rlmDomainIsValidRfSetting(P_ADAPTER_T prAdapter,
			  ENUM_BAND_T eBand,
			  UINT_8 ucPriChannel,
			  ENUM_CHNL_EXT_T eExtend,
			  ENUM_CHANNEL_WIDTH_T eChannelWidth, UINT_8 ucChannelS1, UINT_8 ucChannelS2)
{
	UINT_8	ucCenterChannel;
	UINT_8  ucUpperChannel;
	UINT_8  ucLowerChannel;
	BOOLEAN fgValidChannel = TRUE;
	BOOLEAN fgUpperChannel = TRUE;
	BOOLEAN fgLowerChannel = TRUE;
	BOOLEAN fgValidBW = TRUE;
	BOOLEAN fgValidRfSetting = TRUE;
	UINT_32 u4PrimaryOffset = 0;

	/*DBG msg for Channel InValid */
	if (eChannelWidth == CW_20_40MHZ) {
		ucCenterChannel = rlmDomainGetCenterChannel(eBand, ucPriChannel, eExtend);

		/* Check Central Channel Valid or Not */
		fgValidChannel = rlmDomainCheckChannelEntryValid(prAdapter, ucCenterChannel);
		/* Check Upper Channel and Lower Channel*/
		switch (eExtend) {
		case CHNL_EXT_SCA:
			ucUpperChannel = ucPriChannel + 4;
			ucLowerChannel = ucPriChannel;
			break;
		case CHNL_EXT_SCB:
			ucUpperChannel = ucPriChannel;
			ucLowerChannel = ucPriChannel - 4;
			break;
		default:
			ucUpperChannel = ucPriChannel;
			ucLowerChannel = ucPriChannel;
			break;
		}

		fgUpperChannel = rlmDomainCheckChannelEntryValid(prAdapter, ucUpperChannel);
		fgLowerChannel = rlmDomainCheckChannelEntryValid(prAdapter, ucLowerChannel);
	} else if (eChannelWidth == CW_80MHZ) {
		ucCenterChannel = ucChannelS1;

		/* Check Central Channel Valid or Not */
		fgValidChannel = rlmDomainCheckChannelEntryValid(prAdapter, ucCenterChannel);
		if (fgValidChannel == FALSE)
			DBGLOG(RLM, WARN, "Rf: CentralCh=%d\n", ucCenterChannel);
	} else if (eChannelWidth == CW_160MHZ) {
		ucCenterChannel = ucChannelS2;
		/* Check Central Channel Valid or Not */
		/*TODo */
	}

	/* Check BW Setting Correct or Not */
	if (eBand == BAND_2G4) {
		if (eChannelWidth != CW_20_40MHZ)
			fgValidBW = FALSE;
	} else {
		if (eChannelWidth == CW_80MHZ) {
			u4PrimaryOffset = CAL_CH_OFFSET_80M(ucPriChannel, ucCenterChannel);
			if (u4PrimaryOffset > 4)
				fgValidBW = FALSE;
			if (ucPriChannel == 165)
				fgValidBW = FALSE;
		} else if (eChannelWidth == CW_160MHZ) {
			u4PrimaryOffset = CAL_CH_OFFSET_160M(ucPriChannel, ucCenterChannel);
			if (u4PrimaryOffset > 8)
				fgValidBW = FALSE;
		} else if (eChannelWidth > CW_80P80MHZ)
			fgValidBW = FALSE;
	}

	if (!fgValidBW || !fgValidChannel || !fgUpperChannel || !fgLowerChannel) {
		DBGLOG(RLM, WARN,
			"Rf: ValidBw=%d, ValidChnl=%d, UpChnl=%d, LowerChnl=%d, B=%d, W=%d, offset=%d\n",
			fgValidBW, fgValidChannel, fgUpperChannel,
			fgLowerChannel, eBand, eChannelWidth, u4PrimaryOffset);
		fgValidRfSetting = FALSE;
	}
	return fgValidRfSetting;

}

#if CFG_SUPPORT_PWR_LIMIT_COUNTRY

/*----------------------------------------------------------------------------*/
/*!
* @brief
*
* @param[in]
*
* @return (fgValid) : 0 -> inValid, 1 -> Valid
*/
/*----------------------------------------------------------------------------*/
BOOLEAN
rlmDomainCheckPowerLimitValid(P_ADAPTER_T prAdapter,
			      COUNTRY_POWER_LIMIT_TABLE_CONFIGURATION rPowerLimitTableConfiguration,
			      UINT_8 ucPwrLimitNum)
{
	UINT_8 i;
	BOOLEAN fgValid = TRUE;
	PINT_8 prPwrLimit;

	prPwrLimit = &rPowerLimitTableConfiguration.aucPwrLimit[0];

	for (i = 0; i < ucPwrLimitNum; i++, prPwrLimit++) {
		if (*prPwrLimit > MAX_TX_POWER || *prPwrLimit < MIN_TX_POWER) {
			fgValid = FALSE;
			break;	/*Find out Wrong Power limit */
		}
	}
	return fgValid;

}

/*----------------------------------------------------------------------------*/
/*!
* @brief
*
* @param[in]
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
VOID rlmDomainCheckCountryPowerLimitTable(P_ADAPTER_T prAdapter)
{
	UINT_8 i, j;
	UINT_16 u2CountryCodeTable, u2CountryCodeCheck;
	BOOLEAN fgChannelValid = FALSE;
	BOOLEAN fgPowerLimitValid = FALSE;
	BOOLEAN fgEntryRepetetion = FALSE;
	BOOLEAN fgTableValid = TRUE;

	/*Configuration Table Check */
	for (i = 0; i < sizeof(g_rRlmPowerLimitConfiguration) / sizeof(COUNTRY_POWER_LIMIT_TABLE_CONFIGURATION); i++) {
		/*Table Country Code */
		WLAN_GET_FIELD_BE16(&g_rRlmPowerLimitConfiguration[i].aucCountryCode[0], &u2CountryCodeTable);

		/*Repetition Entry Check */
		for (j = i + 1;
		     j < sizeof(g_rRlmPowerLimitConfiguration) / sizeof(COUNTRY_POWER_LIMIT_TABLE_CONFIGURATION);
		     j++) {

			WLAN_GET_FIELD_BE16(&g_rRlmPowerLimitConfiguration[j].aucCountryCode[0], &u2CountryCodeCheck);
			if (((g_rRlmPowerLimitConfiguration[i].ucCentralCh) ==
			     g_rRlmPowerLimitConfiguration[j].ucCentralCh)
			    && (u2CountryCodeTable == u2CountryCodeCheck)) {
				fgEntryRepetetion = TRUE;
				DBGLOG(RLM, LOUD, "Domain: Configuration Repetition CC=%c%c, Ch=%d\n",
						   g_rRlmPowerLimitConfiguration[i].aucCountryCode[0],
						   g_rRlmPowerLimitConfiguration[i].aucCountryCode[1],
						   g_rRlmPowerLimitConfiguration[i].ucCentralCh);
			}
		}

		/*Channel Number Check */
		fgChannelValid =
		    rlmDomainCheckChannelEntryValid(prAdapter, g_rRlmPowerLimitConfiguration[i].ucCentralCh);

		/*Power Limit Check */
		fgPowerLimitValid =
		    rlmDomainCheckPowerLimitValid(prAdapter, g_rRlmPowerLimitConfiguration[i], PWR_LIMIT_NUM);

		if (fgChannelValid == FALSE || fgPowerLimitValid == FALSE) {
			fgTableValid = FALSE;
			DBGLOG(RLM, LOUD, "Domain: CC=%c%c, Ch=%d, Limit: %d,%d,%d,%d,%d\n",
					   g_rRlmPowerLimitConfiguration[i].aucCountryCode[0],
					   g_rRlmPowerLimitConfiguration[i].aucCountryCode[1],
					   g_rRlmPowerLimitConfiguration[i].ucCentralCh,
					   g_rRlmPowerLimitConfiguration[i].aucPwrLimit[PWR_LIMIT_CCK],
					   g_rRlmPowerLimitConfiguration[i].aucPwrLimit[PWR_LIMIT_20M],
					   g_rRlmPowerLimitConfiguration[i].aucPwrLimit[PWR_LIMIT_40M],
					   g_rRlmPowerLimitConfiguration[i].aucPwrLimit[PWR_LIMIT_80M],
					   g_rRlmPowerLimitConfiguration[i].aucPwrLimit[PWR_LIMIT_160M]);
		}

		if (u2CountryCodeTable == COUNTRY_CODE_NULL) {
			DBGLOG(RLM, LOUD, "Domain: Full search down\n");
			break;	/*End of country table entry */
		}

	}

	if (fgEntryRepetetion == FALSE)
		DBGLOG(RLM, TRACE, "Domain: Configuration Table no Repetiton.\n");

	/*Configuration Table no error */
	if (fgTableValid == TRUE)
		prAdapter->fgIsPowerLimitTableValid = TRUE;
	else
		prAdapter->fgIsPowerLimitTableValid = FALSE;

	/*Default Table Check */
	fgEntryRepetetion = FALSE;
	for (i = 0; i < sizeof(g_rRlmPowerLimitDefault) / sizeof(COUNTRY_POWER_LIMIT_TABLE_DEFAULT); i++) {

		WLAN_GET_FIELD_BE16(&g_rRlmPowerLimitDefault[i].aucCountryCode[0], &u2CountryCodeTable);

		for (j = i + 1; j < sizeof(g_rRlmPowerLimitDefault) / sizeof(COUNTRY_POWER_LIMIT_TABLE_DEFAULT); j++) {
			WLAN_GET_FIELD_BE16(&g_rRlmPowerLimitDefault[j].aucCountryCode[0], &u2CountryCodeCheck);
			if (u2CountryCodeTable == u2CountryCodeCheck) {
				fgEntryRepetetion = TRUE;
				DBGLOG(RLM, LOUD,
				       "Domain: Default Repetition CC=%c%c\n",
					g_rRlmPowerLimitDefault[j].aucCountryCode[0],
					g_rRlmPowerLimitDefault[j].aucCountryCode[1]);
			}
		}
	}
	if (fgEntryRepetetion == FALSE)
		DBGLOG(RLM, TRACE, "Domain: Default Table no Repetiton.\n");
}

/*----------------------------------------------------------------------------*/
/*!
* @brief
*
* @param[in]
*
* @return (u2TableIndex) : if  0xFFFF -> No Table Match
*/
/*----------------------------------------------------------------------------*/
UINT_16 rlmDomainPwrLimitDefaultTableDecision(P_ADAPTER_T prAdapter, UINT_16 u2CountryCode)
{

	UINT_16 i;
	UINT_16 u2CountryCodeTable = COUNTRY_CODE_NULL;
	UINT_16 u2TableIndex = POWER_LIMIT_TABLE_NULL;	/* No Table Match */

	/*Default Table Index */
	for (i = 0; i < sizeof(g_rRlmPowerLimitDefault) / sizeof(COUNTRY_POWER_LIMIT_TABLE_DEFAULT); i++) {

		WLAN_GET_FIELD_BE16(&g_rRlmPowerLimitDefault[i].aucCountryCode[0], &u2CountryCodeTable);

		if (u2CountryCodeTable == u2CountryCode) {
			u2TableIndex = i;
			break;	/*match country code */
		} else if (u2CountryCodeTable == COUNTRY_CODE_NULL) {
			u2TableIndex = i;
			break;	/*find last one country- Default */
		}
	}

	DBGLOG(RLM, TRACE, "Domain: Default Table Index = %d\n", u2TableIndex);

	return u2TableIndex;
}

/*----------------------------------------------------------------------------*/
/*!
* @brief
*
* @param[in]
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
VOID rlmDomainBuildCmdByDefaultTable(P_CMD_SET_COUNTRY_CHANNEL_POWER_LIMIT_T prCmd, UINT_16 u2DefaultTableIndex)
{
	UINT_8 i, k;
	P_COUNTRY_POWER_LIMIT_TABLE_DEFAULT prPwrLimitSubBand;
	P_CMD_CHANNEL_POWER_LIMIT prCmdPwrLimit;

	prCmdPwrLimit = &prCmd->rChannelPowerLimit[0];
	prPwrLimitSubBand = &g_rRlmPowerLimitDefault[u2DefaultTableIndex];

	/* Build power limit cmd by default table information */

	for (i = POWER_LIMIT_2G4; i < POWER_LIMIT_SUBAND_NUM; i++) {
		if (prPwrLimitSubBand->aucPwrLimitSubBand[i] < MAX_TX_POWER) {
			for (k = g_rRlmSubBand[i].ucStartCh; k <= g_rRlmSubBand[i].ucEndCh;
			     k += g_rRlmSubBand[i].ucInterval) {
				if ((prPwrLimitSubBand->ucPwrUnit & BIT(i)) == 0) {
					prCmdPwrLimit->ucCentralCh = k;
					kalMemSet(&prCmdPwrLimit->cPwrLimitCCK,
						  prPwrLimitSubBand->aucPwrLimitSubBand[i], PWR_LIMIT_NUM);
					prCmdPwrLimit++;
					prCmd->ucNum++;

				} else {
					/*
					 * ex:    40MHz power limit(mW\MHz) = 20MHz power limit(mW\MHz) * 2
					 * ---> 40MHz power limit(dBm) = 20MHz power limit(dBm) + 6;
					 */
					prCmdPwrLimit->ucCentralCh = k;
					prCmdPwrLimit->cPwrLimitCCK = prPwrLimitSubBand->aucPwrLimitSubBand[i];
					prCmdPwrLimit->cPwrLimit20 = prPwrLimitSubBand->aucPwrLimitSubBand[i];
					prCmdPwrLimit->cPwrLimit40 = prPwrLimitSubBand->aucPwrLimitSubBand[i] + 6;
					if (prCmdPwrLimit->cPwrLimit40 > MAX_TX_POWER)
						prCmdPwrLimit->cPwrLimit40 = MAX_TX_POWER;
					prCmdPwrLimit->cPwrLimit80 = prPwrLimitSubBand->aucPwrLimitSubBand[i] + 12;
					if (prCmdPwrLimit->cPwrLimit80 > MAX_TX_POWER)
						prCmdPwrLimit->cPwrLimit80 = MAX_TX_POWER;
					prCmdPwrLimit->cPwrLimit160 = prPwrLimitSubBand->aucPwrLimitSubBand[i] + 18;
					if (prCmdPwrLimit->cPwrLimit160 > MAX_TX_POWER)
						prCmdPwrLimit->cPwrLimit160 = MAX_TX_POWER;
					prCmdPwrLimit++;
					prCmd->ucNum++;
				}
			}
		}
	}

#if 0
	if (prPwrLimitSubBand->aucPwrLimitSubBand[POWER_LIMIT_2G4] < MAX_TX_POWER) {
		for (i = BAND_2G4_LOWER_BOUND; i <= BAND_2G4_UPPER_BOUND; i++) {
			prCmdPwrLimit->ucCentralCh = i;
			kalMemSet(&prCmdPwrLimit->cPwrLimitCCK, prPwrLimitSubBand->aucPwrLimitSubBand[POWER_LIMIT_2G4],
				  PWR_LIMIT_NUM);
			prCmdPwrLimit++;
			prCmd->ucNum++;
		}
	}

	if (prPwrLimitSubBand->aucPwrLimitSubBand[POWER_LIMIT_UNII1] < MAX_TX_POWER) {
		if (prCmd->u2CountryCode != COUNTRY_CODE_KR) {
			for (i = UNII1_LOWER_BOUND; i <= UNII1_UPPER_BOUND; i += 2) {
				prCmdPwrLimit->ucCentralCh = i;
				kalMemSet(&prCmdPwrLimit->cPwrLimitCCK,
					  prPwrLimitSubBand->aucPwrLimitSubBand[POWER_LIMIT_UNII1], PWR_LIMIT_NUM);
				prCmdPwrLimit++;
				prCmd->ucNum++;
			}
		} else {
			for (i = UNII1_LOWER_BOUND; i <= UNII1_UPPER_BOUND; i += 2) {
				/*
				 * ex:    40MHz power limit(mW\MHz) = 20MHz power limit(mW\MHz) * 2
				 * ---> 40MHz power limit(dBm) = 20MHz power limit(dBm) + 6;
				 */
				prCmdPwrLimit->ucCentralCh = i;
				prCmdPwrLimit->cPwrLimitCCK =
				    g_rRlmPowerLimitDefault[u2DefaultTableIndex].cPwrLimitUnii1;
				prCmdPwrLimit->cPwrLimit20 =
				    g_rRlmPowerLimitDefault[u2DefaultTableIndex].cPwrLimitUnii1;
				prCmdPwrLimit->cPwrLimit40 =
				    g_rRlmPowerLimitDefault[u2DefaultTableIndex].cPwrLimitUnii1 + 6;
				prCmdPwrLimit->cPwrLimit80 =
				    g_rRlmPowerLimitDefault[u2DefaultTableIndex].cPwrLimitUnii1 + 12;
				prCmdPwrLimit->cPwrLimit160 =
				    g_rRlmPowerLimitDefault[u2DefaultTableIndex].cPwrLimitUnii1 + 18;
				prCmdPwrLimit++;
				prCmd->ucNum++;
			}
		}
	}

	if (prPwrLimitSubBand->aucPwrLimitSubBand[POWER_LIMIT_UNII2A] < MAX_TX_POWER) {
		for (i = UNII2A_LOWER_BOUND; i <= UNII2A_UPPER_BOUND; i += 2) {
			prCmdPwrLimit->ucCentralCh = i;
			kalMemSet(&prCmdPwrLimit->cPwrLimitCCK,
				  prPwrLimitSubBand->aucPwrLimitSubBand[POWER_LIMIT_UNII2A], PWR_LIMIT_NUM);
			prCmdPwrLimit++;
			prCmd->ucNum++;
		}
	}

	if (prPwrLimitSubBand->aucPwrLimitSubBand[POWER_LIMIT_UNII2C] < MAX_TX_POWER) {
		for (i = UNII2C_LOWER_BOUND; i <= UNII2C_UPPER_BOUND; i += 2) {
			prCmdPwrLimit->ucCentralCh = i;
			kalMemSet(&prCmdPwrLimit->cPwrLimitCCK,
				  prPwrLimitSubBand->aucPwrLimitSubBand[POWER_LIMIT_UNII2C], PWR_LIMIT_NUM);
			prCmdPwrLimit++;
			prCmd->ucNum++;
		}
	}
	if (prPwrLimitSubBand->aucPwrLimitSubBand[POWER_LIMIT_UNII3] < MAX_TX_POWER) {
		for (i = UNII3_LOWER_BOUND; i <= UNII3_UPPER_BOUND; i += 2) {
			prCmdPwrLimit->ucCentralCh = i;
			kalMemSet(&prCmdPwrLimit->cPwrLimitCCK,
				  prPwrLimitSubBand->aucPwrLimitSubBand[POWER_LIMIT_UNII3], PWR_LIMIT_NUM);
			prCmdPwrLimit++;
			prCmd->ucNum++;
		}
	}
#endif
}

/*----------------------------------------------------------------------------*/
/*!
* @brief
*
* @param[in]
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
VOID rlmDomainBuildCmdByConfigTable(P_ADAPTER_T prAdapter, P_CMD_SET_COUNTRY_CHANNEL_POWER_LIMIT_T prCmd)
{
	UINT_8 i, k;
	UINT_16 u2CountryCodeTable = COUNTRY_CODE_NULL;
	P_CMD_CHANNEL_POWER_LIMIT prCmdPwrLimit;
	BOOLEAN fgChannelValid;

	/*Build power limit cmd by configuration table information */

	for (i = 0; i < sizeof(g_rRlmPowerLimitConfiguration) / sizeof(COUNTRY_POWER_LIMIT_TABLE_CONFIGURATION); i++) {

		WLAN_GET_FIELD_BE16(&g_rRlmPowerLimitConfiguration[i].aucCountryCode[0], &u2CountryCodeTable);

		fgChannelValid =
		    rlmDomainCheckChannelEntryValid(prAdapter, g_rRlmPowerLimitConfiguration[i].ucCentralCh);

		if (u2CountryCodeTable == COUNTRY_CODE_NULL) {
			DBGLOG(RLM, TRACE, "Domain: full search configuration table done.\n");
			break;	/*end of configuration table */
		} else if ((u2CountryCodeTable == prCmd->u2CountryCode) && (fgChannelValid == TRUE)) {

			prCmdPwrLimit = &prCmd->rChannelPowerLimit[0];

			if (prCmd->ucNum != 0) {
				for (k = 0; k < prCmd->ucNum; k++) {
					if (prCmdPwrLimit->ucCentralCh ==
						g_rRlmPowerLimitConfiguration[i].ucCentralCh) {

						/*
						 * Cmd setting (Default table information) and
						 * Configuration table has repetition channel entry,
						 * ex : Default table (ex: 2.4G, limit = 20dBm) --> ch1~14 limit =20dBm,
						 * Configuration table (ex: ch1, limit = 22dBm) --> ch 1 = 22 dBm
						 * Cmd final setting -->  ch1 = 22dBm, ch12~14 = 20dBm
						 */
						kalMemCopy(&prCmdPwrLimit->cPwrLimitCCK,
							   &g_rRlmPowerLimitConfiguration[i].aucPwrLimit,
							   PWR_LIMIT_NUM);

						DBGLOG(RLM, LOUD,
						       "Domain: CC=%c%c,ConfigCh=%d,Limit=%d,%d,%d,%d,%d,Fg=%d\n",
							((prCmd->u2CountryCode & 0xff00) >> 8),
							(prCmd->u2CountryCode & 0x00ff), prCmdPwrLimit->ucCentralCh,
							prCmdPwrLimit->cPwrLimitCCK, prCmdPwrLimit->cPwrLimit20,
							prCmdPwrLimit->cPwrLimit40, prCmdPwrLimit->cPwrLimit80,
							prCmdPwrLimit->cPwrLimit160, prCmdPwrLimit->ucFlag);

						break;
					}
					prCmdPwrLimit++;
				}
				if (k == prCmd->ucNum) {

					/*
					 * Full search cmd (Default table setting) no match channey,
					 * ex : Default table (ex: 2.4G, limit = 20dBm) --> ch1~14 limit =20dBm,
					 * Configuration table (ex: ch36, limit = 22dBm) --> ch 36 = 22 dBm
					 * Cmd final setting -->  ch1~14 = 20dBm, ch36= 22dBm
					 */
					kalMemCopy(&prCmdPwrLimit->cPwrLimitCCK,
						   &g_rRlmPowerLimitConfiguration[i].aucPwrLimit, PWR_LIMIT_NUM);
					prCmd->ucNum++;

					DBGLOG(RLM, LOUD,
					       "Domain: Full CC=%c%c,ConfigCh=%d,Limit=%d,%d,%d,%d,%d,Fg=%d\n",
						((prCmd->u2CountryCode & 0xff00) >> 8), (prCmd->u2CountryCode & 0x00ff),
						prCmdPwrLimit->ucCentralCh, prCmdPwrLimit->cPwrLimitCCK,
						prCmdPwrLimit->cPwrLimit20, prCmdPwrLimit->cPwrLimit40,
						prCmdPwrLimit->cPwrLimit80, prCmdPwrLimit->cPwrLimit160,
						prCmdPwrLimit->ucFlag);

				}
			} else {

				/*
				 * Default table power limit value are 63--> cmd table no channel entry
				 * ex : Default table (ex: 2.4G, limit = 63Bm) --> no channel entry in cmd,
				 * Configuration table (ex: ch36, limit = 22dBm) --> ch 36 = 22 dBm
				 * Cmd final setting -->   ch36= 22dBm
				 */
				prCmdPwrLimit->ucCentralCh = g_rRlmPowerLimitConfiguration[i].ucCentralCh;
				kalMemCopy(&prCmdPwrLimit->cPwrLimitCCK, &g_rRlmPowerLimitConfiguration[i].aucPwrLimit,
					   PWR_LIMIT_NUM);
				prCmd->ucNum++;

				DBGLOG(RLM, LOUD, "Domain: Default table power limit value are 63.\n");
				DBGLOG(RLM, LOUD, "Domain: CC=%c%c,ConfigCh=%d,Limit=%d,%d,%d,%d,%d,Fg=%d\n",
						   ((prCmd->u2CountryCode & 0xff00) >> 8),
						   (prCmd->u2CountryCode & 0x00ff), prCmdPwrLimit->ucCentralCh,
						   prCmdPwrLimit->cPwrLimitCCK, prCmdPwrLimit->cPwrLimit20,
						   prCmdPwrLimit->cPwrLimit40, prCmdPwrLimit->cPwrLimit80,
						   prCmdPwrLimit->cPwrLimit160, prCmdPwrLimit->ucFlag);

			}
		}
	}
}

/*----------------------------------------------------------------------------*/
/*!
* @brief
*
* @param[in]
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
VOID rlmDomainSendPwrLimitCmd(P_ADAPTER_T prAdapter)
{
	P_CMD_SET_COUNTRY_CHANNEL_POWER_LIMIT_T prCmd;
	UINT_8 i;
	UINT_16 u2DefaultTableIndex;
	UINT_32 u4SetCmdTableMaxSize;
	UINT_32 u4SetQueryInfoLen;
	P_CMD_CHANNEL_POWER_LIMIT prCmdPwrLimit;	/* for print usage */

	u4SetCmdTableMaxSize =
	    sizeof(CMD_SET_COUNTRY_CHANNEL_POWER_LIMIT_T) +
	    MAX_CMD_SUPPORT_CHANNEL_NUM * sizeof(CMD_CHANNEL_POWER_LIMIT);

	prCmd = cnmMemAlloc(prAdapter, RAM_TYPE_BUF, u4SetCmdTableMaxSize);

	if (!prCmd) {
		DBGLOG(RLM, ERROR, "Domain: no buf to send cmd\n");
		return;
	}
	kalMemZero(prCmd, u4SetCmdTableMaxSize);

	u2DefaultTableIndex =
	    rlmDomainPwrLimitDefaultTableDecision(prAdapter, prAdapter->rWifiVar.rConnSettings.u2CountryCode);

	if (u2DefaultTableIndex != POWER_LIMIT_TABLE_NULL) {

		WLAN_GET_FIELD_BE16(&g_rRlmPowerLimitDefault[u2DefaultTableIndex].aucCountryCode[0],
				    &prCmd->u2CountryCode);

		prCmd->ucNum = 0;

		if (prCmd->u2CountryCode != COUNTRY_CODE_NULL) {
			/*Command - default table information */
			rlmDomainBuildCmdByDefaultTable(prCmd, u2DefaultTableIndex);

			/*Command - configuration table information */
			rlmDomainBuildCmdByConfigTable(prAdapter, prCmd);
		}
	}
#if 0
	u4SetCmdTableMaxSize =
	    sizeof(CMD_SET_COUNTRY_CHANNEL_POWER_LIMIT_T) +
	    MAX_CMD_SUPPORT_CHANNEL_NUM * sizeof(CMD_CHANNEL_POWER_LIMIT);

	prCmd = cnmMemAlloc(prAdapter, RAM_TYPE_BUF, u4SetCmdTableMaxSize);
	ASSERT(prCmd);

	/* To do: exception handle */
	if (!prCmd) {
		DBGLOG(RLM, ERROR, "Domain: no buf to send cmd\n");
		return;
	}
	kalMemZero(prCmd, u4SetCmdTableMaxSize);	/* TODO memzero */

	if (u2TableIndex != POWER_LIMIT_TABLE_NULL && u2TableIndex < MAX_DEFAULT_TABLE_COUNTRY_NUM) {

		prCmd->u2CountryCode = (((UINT_16) g_rRlmCountryPowerLimitTable[u2TableIndex].aucCountryCode[0]) << 8) |
		    (((UINT_16) g_rRlmCountryPowerLimitTable[u2TableIndex].aucCountryCode[1]) & BITS(0, 7));
		prChPwrLimit = &g_rRlmCountryPowerLimitTable[u2TableIndex].rChannelPowerLimit[0];
		prCmdPwrLimit = &prCmd->rChannelPowerLimit[0];
		prCmd->ucNum = 0;
		for (i = 0; i < MAX_CMD_SUPPORT_CHANNEL_NUM; i++) {

			if (prChPwrLimit->ucCentralCh != ENDCH) {

				/*Check Power limit table channel efficient or not */
				fgChannelValid = rlmDomainCheckChannelEntryValid(prAdapter, prChPwrLimit->ucCentralCh);

				/*Cmd set up */
				if (fgChannelValid) {
					kalMemCopy(prCmdPwrLimit, prChPwrLimit, sizeof(CMD_CHANNEL_POWER_LIMIT));
					DBGLOG(RLM, INFO,
					       "Domain: ValidCh=%d,Limit=%d,%d,%d,%d,%d,Fg=%d\n",
						prCmdPwrLimit->ucCentralCh, prCmdPwrLimit->cPwrLimitCCK,
						prCmdPwrLimit->cPwrLimit20, prCmdPwrLimit->cPwrLimit40,
						prCmdPwrLimit->cPwrLimit80, prCmdPwrLimit->cPwrLimit160,
						prCmdPwrLimit->ucFlag);
					prCmd->ucNum++;
					prCmdPwrLimit++;
				} else {
					DBGLOG(RLM, INFO,
					       "Domain: Non-Ch=%d,Limit=%d,%d,%d,%d,%d,Fg=%d\n",
						prChPwrLimit->ucCentralCh, prChPwrLimit->cPwrLimitCCK,
						prChPwrLimit->cPwrLimit20, prChPwrLimit->cPwrLimit40,
						prChPwrLimit->cPwrLimit80, prChPwrLimit->cPwrLimit160,
						prChPwrLimit->ucFlag);
				}
				prChPwrLimit++;
			} else {
				/*End of the chanel entry */
				break;
			}
		};
	}
#endif

	if (prCmd->u2CountryCode != 0) {
		DBGLOG(RLM, INFO,
		       "Domain: ValidCC =%c%c, ChNum=%d\n", ((prCmd->u2CountryCode & 0xff00) >> 8),
			(prCmd->u2CountryCode & 0x00ff), prCmd->ucNum);
	} else {
		DBGLOG(RLM, INFO, "Domain: ValidCC =0x%04x, ucNum=%d\n", prCmd->u2CountryCode, prCmd->ucNum);
	}
	prCmdPwrLimit = &prCmd->rChannelPowerLimit[0];

	for (i = 0; i < prCmd->ucNum; i++) {
		DBGLOG(RLM, TRACE, "Domain: Ch=%d,Limit=%d,%d,%d,%d,%d,Fg=%d\n", prCmdPwrLimit->ucCentralCh,
				   prCmdPwrLimit->cPwrLimitCCK, prCmdPwrLimit->cPwrLimit20, prCmdPwrLimit->cPwrLimit40,
				   prCmdPwrLimit->cPwrLimit80, prCmdPwrLimit->cPwrLimit160, prCmdPwrLimit->ucFlag);
		prCmdPwrLimit++;
	}

	u4SetQueryInfoLen =
	    (sizeof(CMD_SET_COUNTRY_CHANNEL_POWER_LIMIT_T) + (prCmd->ucNum) * sizeof(CMD_CHANNEL_POWER_LIMIT));

	/* Update domain info to chip */
	if (prCmd->ucNum <= MAX_CMD_SUPPORT_CHANNEL_NUM) {
		wlanSendSetQueryCmd(prAdapter, /* prAdapter */
			     CMD_ID_SET_COUNTRY_POWER_LIMIT, /* ucCID */
			     TRUE,  /* fgSetQuery */
			     FALSE, /* fgNeedResp */
			     FALSE, /* fgIsOid */
			     NULL,  /* pfCmdDoneHandler */
			     NULL,  /* pfCmdTimeoutHandler */
			     u4SetQueryInfoLen, /* u4SetQueryInfoLen */
			     (PUINT_8) prCmd, /* pucInfoBuffer */
			     NULL,  /* pvSetQueryBuffer */
			     0      /* u4SetQueryBufferLen */
		    );
	} else
		DBGLOG(RLM, ERROR, "Domain: illegal power limit table");

	cnmMemFree(prAdapter, prCmd);

}
#endif
