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

#ifndef _RATE_H
#define _RATE_H

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
/* Routines in rate.c                                                         */
/*----------------------------------------------------------------------------*/
VOID
rateGetRateSetFromIEs(IN P_IE_SUPPORTED_RATE_T prIeSupportedRate,
		      IN P_IE_EXT_SUPPORTED_RATE_T prIeExtSupportedRate,
		      OUT PUINT_16 pu2OperationalRateSet,
		      OUT PUINT_16 pu2BSSBasicRateSet, OUT PBOOLEAN pfgIsUnknownBSSBasicRate);

VOID
rateGetDataRatesFromRateSet(IN UINT_16 u2OperationalRateSet,
			    IN UINT_16 u2BSSBasicRateSet, OUT PUINT_8 pucDataRates, OUT PUINT_8 pucDataRatesLen);

BOOLEAN rateGetHighestRateIndexFromRateSet(IN UINT_16 u2RateSet, OUT PUINT_8 pucHighestRateIndex);

BOOLEAN rateGetLowestRateIndexFromRateSet(IN UINT_16 u2RateSet, OUT PUINT_8 pucLowestRateIndex);

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

#endif /* _RATE_H */
