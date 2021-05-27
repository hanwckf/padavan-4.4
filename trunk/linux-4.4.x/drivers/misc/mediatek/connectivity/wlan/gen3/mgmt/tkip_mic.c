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
 * Id: //Department/DaVinci/TRUNK/MT6620_WiFi_Firmware/mcu/wifi/mgmt/tkip_mic.c#7
 */

/*
 * ! \file tkip_sw.c
 * \brief This file include the tkip encrypted / decrypted mic function.
 */

/*******************************************************************************
*                     C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/
#include "precomp.h"

/*******************************************************************************
*                          C O N S T A N T S
********************************************************************************
*/

/*******************************************************************************
*                         D A T A   T Y P E S
********************************************************************************
*/

/*******************************************************************************
*                        P U B L I C   D A T A
********************************************************************************
*/

/*******************************************************************************
*                       P R I V A T E   D A T A
********************************************************************************
*/

/*******************************************************************************
*                             M A C R O S
********************************************************************************
*/

#define WLAN_MAC_MIC_LEN                8	/* length of TKIP and CCMP MIC field */

#define MK16_TKIP(x, y)       (((UINT_16) (x) << 8) | (UINT_16) (y))

#define LO_8BITS(x)     ((x) & 0x00ff)	/* obtain low 8-bit from 16-bit value, OK */
#define HI_8BITS(x)     ((x) >> 8)	/* obtain high 8-bit from 16-bit value, OK */

#define ROTR32(x, y)     (((x) >> (y)) | ((x) << (32 - (y))))
#define ROTL32(x, y)     (((x) << (y)) | ((x) >> (32 - (y))))
#define ROTR16(x, y)     (((x) >> (y)) | ((x) << (16 - (y))))
#define ROTL16(x, y)     (((x) << (y)) | ((x) >> (16 - (y))))

#define XSWAP32(x)      ((((x) & 0xFF00FF00) >> 8) | (((x) & 0x00FF00FF) << 8))

/* obtain 16-bit entries SBOX form two 8-bit entries SBOX1 and SBOX2 */
#define SBOX(x)         (tkipSBOX1[LO_8BITS(x)] ^ tkipSBOX2[HI_8BITS(x)])

/*******************************************************************************
*                         D A T A   T Y P E S
********************************************************************************
*/

/*******************************************************************************
*                        P U B L I C   D A T A
********************************************************************************
*/
const UINT_16 tkipSBOX1[256] = {
	0xC6A5, 0xF884, 0xEE99, 0xF68D, 0xFF0D, 0xD6BD, 0xDEB1, 0x9154,
	0x6050, 0x0203, 0xCEA9, 0x567D, 0xE719, 0xB562, 0x4DE6, 0xEC9A,
	0x8F45, 0x1F9D, 0x8940, 0xFA87, 0xEF15, 0xB2EB, 0x8EC9, 0xFB0B,
	0x41EC, 0xB367, 0x5FFD, 0x45EA, 0x23BF, 0x53F7, 0xE496, 0x9B5B,
	0x75C2, 0xE11C, 0x3DAE, 0x4C6A, 0x6C5A, 0x7E41, 0xF502, 0x834F,
	0x685C, 0x51F4, 0xD134, 0xF908, 0xE293, 0xAB73, 0x6253, 0x2A3F,
	0x080C, 0x9552, 0x4665, 0x9D5E, 0x3028, 0x37A1, 0x0A0F, 0x2FB5,
	0x0E09, 0x2436, 0x1B9B, 0xDF3D, 0xCD26, 0x4E69, 0x7FCD, 0xEA9F,
	0x121B, 0x1D9E, 0x5874, 0x342E, 0x362D, 0xDCB2, 0xB4EE, 0x5BFB,
	0xA4F6, 0x764D, 0xB761, 0x7DCE, 0x527B, 0xDD3E, 0x5E71, 0x1397,
	0xA6F5, 0xB968, 0x0000, 0xC12C, 0x4060, 0xE31F, 0x79C8, 0xB6ED,
	0xD4BE, 0x8D46, 0x67D9, 0x724B, 0x94DE, 0x98D4, 0xB0E8, 0x854A,
	0xBB6B, 0xC52A, 0x4FE5, 0xED16, 0x86C5, 0x9AD7, 0x6655, 0x1194,
	0x8ACF, 0xE910, 0x0406, 0xFE81, 0xA0F0, 0x7844, 0x25BA, 0x4BE3,
	0xA2F3, 0x5DFE, 0x80C0, 0x058A, 0x3FAD, 0x21BC, 0x7048, 0xF104,
	0x63DF, 0x77C1, 0xAF75, 0x4263, 0x2030, 0xE51A, 0xFD0E, 0xBF6D,
	0x814C, 0x1814, 0x2635, 0xC32F, 0xBEE1, 0x35A2, 0x88CC, 0x2E39,
	0x9357, 0x55F2, 0xFC82, 0x7A47, 0xC8AC, 0xBAE7, 0x322B, 0xE695,
	0xC0A0, 0x1998, 0x9ED1, 0xA37F, 0x4466, 0x547E, 0x3BAB, 0x0B83,
	0x8CCA, 0xC729, 0x6BD3, 0x283C, 0xA779, 0xBCE2, 0x161D, 0xAD76,
	0xDB3B, 0x6456, 0x744E, 0x141E, 0x92DB, 0x0C0A, 0x486C, 0xB8E4,
	0x9F5D, 0xBD6E, 0x43EF, 0xC4A6, 0x39A8, 0x31A4, 0xD337, 0xF28B,
	0xD532, 0x8B43, 0x6E59, 0xDAB7, 0x018C, 0xB164, 0x9CD2, 0x49E0,
	0xD8B4, 0xACFA, 0xF307, 0xCF25, 0xCAAF, 0xF48E, 0x47E9, 0x1018,
	0x6FD5, 0xF088, 0x4A6F, 0x5C72, 0x3824, 0x57F1, 0x73C7, 0x9751,
	0xCB23, 0xA17C, 0xE89C, 0x3E21, 0x96DD, 0x61DC, 0x0D86, 0x0F85,
	0xE090, 0x7C42, 0x71C4, 0xCCAA, 0x90D8, 0x0605, 0xF701, 0x1C12,
	0xC2A3, 0x6A5F, 0xAEF9, 0x69D0, 0x1791, 0x9958, 0x3A27, 0x27B9,
	0xD938, 0xEB13, 0x2BB3, 0x2233, 0xD2BB, 0xA970, 0x0789, 0x33A7,
	0x2DB6, 0x3C22, 0x1592, 0xC920, 0x8749, 0xAAFF, 0x5078, 0xA57A,
	0x038F, 0x59F8, 0x0980, 0x1A17, 0x65DA, 0xD731, 0x84C6, 0xD0B8,
	0x82C3, 0x29B0, 0x5A77, 0x1E11, 0x7BCB, 0xA8FC, 0x6DD6, 0x2C3A
};

const UINT_16 tkipSBOX2[256] = {
	0xA5C6, 0x84F8, 0x99EE, 0x8DF6, 0x0DFF, 0xBDD6, 0xB1DE, 0x5491,
	0x5060, 0x0302, 0xA9CE, 0x7D56, 0x19E7, 0x62B5, 0xE64D, 0x9AEC,
	0x458F, 0x9D1F, 0x4089, 0x87FA, 0x15EF, 0xEBB2, 0xC98E, 0x0BFB,
	0xEC41, 0x67B3, 0xFD5F, 0xEA45, 0xBF23, 0xF753, 0x96E4, 0x5B9B,
	0xC275, 0x1CE1, 0xAE3D, 0x6A4C, 0x5A6C, 0x417E, 0x02F5, 0x4F83,
	0x5C68, 0xF451, 0x34D1, 0x08F9, 0x93E2, 0x73AB, 0x5362, 0x3F2A,
	0x0C08, 0x5295, 0x6546, 0x5E9D, 0x2830, 0xA137, 0x0F0A, 0xB52F,
	0x090E, 0x3624, 0x9B1B, 0x3DDF, 0x26CD, 0x694E, 0xCD7F, 0x9FEA,
	0x1B12, 0x9E1D, 0x7458, 0x2E34, 0x2D36, 0xB2DC, 0xEEB4, 0xFB5B,
	0xF6A4, 0x4D76, 0x61B7, 0xCE7D, 0x7B52, 0x3EDD, 0x715E, 0x9713,
	0xF5A6, 0x68B9, 0x0000, 0x2CC1, 0x6040, 0x1FE3, 0xC879, 0xEDB6,
	0xBED4, 0x468D, 0xD967, 0x4B72, 0xDE94, 0xD498, 0xE8B0, 0x4A85,
	0x6BBB, 0x2AC5, 0xE54F, 0x16ED, 0xC586, 0xD79A, 0x5566, 0x9411,
	0xCF8A, 0x10E9, 0x0604, 0x81FE, 0xF0A0, 0x4478, 0xBA25, 0xE34B,
	0xF3A2, 0xFE5D, 0xC080, 0x8A05, 0xAD3F, 0xBC21, 0x4870, 0x04F1,
	0xDF63, 0xC177, 0x75AF, 0x6342, 0x3020, 0x1AE5, 0x0EFD, 0x6DBF,
	0x4C81, 0x1418, 0x3526, 0x2FC3, 0xE1BE, 0xA235, 0xCC88, 0x392E,
	0x5793, 0xF255, 0x82FC, 0x477A, 0xACC8, 0xE7BA, 0x2B32, 0x95E6,
	0xA0C0, 0x9819, 0xD19E, 0x7FA3, 0x6644, 0x7E54, 0xAB3B, 0x830B,
	0xCA8C, 0x29C7, 0xD36B, 0x3C28, 0x79A7, 0xE2BC, 0x1D16, 0x76AD,
	0x3BDB, 0x5664, 0x4E74, 0x1E14, 0xDB92, 0x0A0C, 0x6C48, 0xE4B8,
	0x5D9F, 0x6EBD, 0xEF43, 0xA6C4, 0xA839, 0xA431, 0x37D3, 0x8BF2,
	0x32D5, 0x438B, 0x596E, 0xB7DA, 0x8C01, 0x64B1, 0xD29C, 0xE049,
	0xB4D8, 0xFAAC, 0x07F3, 0x25CF, 0xAFCA, 0x8EF4, 0xE947, 0x1810,
	0xD56F, 0x88F0, 0x6F4A, 0x725C, 0x2438, 0xF157, 0xC773, 0x5197,
	0x23CB, 0x7CA1, 0x9CE8, 0x213E, 0xDD96, 0xDC61, 0x860D, 0x850F,
	0x90E0, 0x427C, 0xC471, 0xAACC, 0xD890, 0x0506, 0x01F7, 0x121C,
	0xA3C2, 0x5F6A, 0xF9AE, 0xD069, 0x9117, 0x5899, 0x273A, 0xB927,
	0x38D9, 0x13EB, 0xB32B, 0x3322, 0xBBD2, 0x70A9, 0x8907, 0xA733,
	0xB62D, 0x223C, 0x9215, 0x20C9, 0x4987, 0xFFAA, 0x7850, 0x7AA5,
	0x8F03, 0xF859, 0x8009, 0x171A, 0xDA65, 0x31D7, 0xC684, 0xB8D0,
	0xC382, 0xB029, 0x775A, 0x111E, 0xCB7B, 0xFCA8, 0xD66D, 0x3A2C
};

/*******************************************************************************
*                       P R I V A T E   D A T A
********************************************************************************
*/

/*******************************************************************************
*              F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

/*******************************************************************************
*                          F U N C T I O N S
********************************************************************************
*/

/*----------------------------------------------------------------------------*/
/*!
* \brief TKIP Michael block function
*
* \param[in][out] pu4L - pointer to left value
* \param[in][out] pu4PR - pointer to right value
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/
VOID tkipMicB(IN OUT PUINT_32 pu4L, IN OUT PUINT_32 pu4R)
{
	ASSERT(pu4L);
	ASSERT(pu4R);

	*pu4R = *pu4R ^ ROTL32(*pu4L, 17);	/* r <- r ^ (l<<<17)    */
	*pu4L = (*pu4L + *pu4R);	/* l <- (l+r) mod 2^32  */
	*pu4R = *pu4R ^ XSWAP32(*pu4L);	/* r <- r ^ XSWAP(l)    */
	*pu4L = (*pu4L + *pu4R);	/* l <- (l+r) mod 2^32  */
	*pu4R = *pu4R ^ ROTL32(*pu4L, 3);	/* r <- r ^ (l<<<3)     */
	*pu4L = (*pu4L + *pu4R);	/* l <- (l+r) mod 2^32  */
	*pu4R = *pu4R ^ ROTR32(*pu4L, 2);	/* r <- r ^ (l>>>2)     */
	*pu4L = (*pu4L + *pu4R);	/* l <- (l+r) mod 2^32  */
}				/* tkipMicB */

/*----------------------------------------------------------------------------*/
/*!
* \brief TKIP Michael generation function
*
* \param[in]  pucMickey Pointer to MIC key
* \param[in]  pucData Pointer to message
* \param[in]  u4DataLen Message length, in byte(s)
* \param[in]  pucSa Pointer to source address SA
* \param[in]  pucDa Pointer to destination address DA
* \param[in]  ucPriority Priority of IEEE 802.11 traffic class
* \param[out] pucMic Pointer to 64-bit MIC
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/
VOID
tkipMicGen(IN PUCHAR pucMickey,
	   IN PUCHAR pucData,
	   IN UINT_32 u4DataLen, IN PUCHAR pucSa, IN PUCHAR pucDa, IN UCHAR ucPriority, OUT PUCHAR pucMic)
{

	UINT_32 i;
	UINT_32 l, r;
	UINT_32 au4Msg[3];

	ASSERT(pucMickey);
	ASSERT(pucData);
	ASSERT(pucSa);
	ASSERT(pucDa);
	ASSERT(pucMic);

	WLAN_GET_FIELD_32(pucMickey, &l);
	WLAN_GET_FIELD_32(pucMickey + 4, &r);

	/* Michael message processing for DA and SA. */
	WLAN_GET_FIELD_32(pucDa, &au4Msg[0]);
	au4Msg[1] = ((UINT_32) pucDa[4]) | ((UINT_32) pucDa[5] << 8) |
	    ((UINT_32) pucSa[0] << 16) | ((UINT_32) pucSa[1] << 24);
	WLAN_GET_FIELD_32(pucSa + 2, &au4Msg[2]);

	for (i = 0; i < 3; i++) {
		l = l ^ au4Msg[i];
		tkipMicB(&l, &r);
	}

	/* Michael message processing for priority. */
	au4Msg[0] = (UINT_32) ucPriority;

	l = l ^ au4Msg[0];
	tkipMicB(&l, &r);

	/*
	 * Michael message processing for MSDU data playload except the last octets
	 * which cannot be partitioned into a 32-bit word.
	 */
	for (i = 0; i < (UINT_32) u4DataLen / 4; i++) {
		WLAN_GET_FIELD_32(pucData + i * 4, &au4Msg[0]);
		l = l ^ au4Msg[0];
		tkipMicB(&l, &r);
	}

	/*
	 * Michael message processing for the last uncomplete octets, if present,
	 * and the padding.
	 */
	switch (u4DataLen & 3) {
	case 1:
		au4Msg[0] = ((UINT_32) pucData[u4DataLen - 1]) | 0x00005A00;
		break;

	case 2:
		au4Msg[0] = ((UINT_32) pucData[u4DataLen - 2]) | ((UINT_32) pucData[u4DataLen - 1] << 8) | 0x005A0000;
		break;

	case 3:
		au4Msg[0] = ((UINT_32) pucData[u4DataLen - 3]) |
		    ((UINT_32) pucData[u4DataLen - 2] << 8) | ((UINT_32) pucData[u4DataLen - 1] << 16) | 0x5A000000;
		break;

	default:
		au4Msg[0] = 0x0000005A;
	}
	au4Msg[1] = 0;
	for (i = 0; i < 2; i++) {
		l = l ^ au4Msg[i];
		tkipMicB(&l, &r);
	}

	/* return ( l, r ), i.e. MIC */
	WLAN_SET_FIELD_32(pucMic, l);
	WLAN_SET_FIELD_32(pucMic + 4, r);

}				/* tkipMicGen */

/*----------------------------------------------------------------------------*/
/*!
* \brief this function decapsulate MSDU frame body( with MIC ) according
*        to IEEE 802.11i TKIP sepcification.
*
* \param[in] prAdapter Pointer to the adapter object data area.
* \param[in]  pucDa Pointer to destination address DA
* \param[in]  pucSa Pointer to source address SA
* \param[in]  ucPriority Priority of IEEE 802.11 traffic class
* \param[in]  pucPayload Pointer to message
* \param[in]  u2PayloadLen Message length, in byte(s)
* \param[out] pucMic Pointer to 64-bit MIC
*
* \retval NONE
*/
/*----------------------------------------------------------------------------*/
VOID
tkipMicEncapsulate(IN PUINT_8 pucDa,
		   IN PUINT_8 pucSa,
		   IN UINT_8 ucPriority,
		   IN UINT_16 u2PayloadLen, IN PUINT_8 pucPayload, IN PUINT_8 pucMic, IN PUINT_8 pucMicKey)
{
	UCHAR aucMic[8];	/* MIC' */

	DEBUGFUNC("tkipSwMsduEncapsulate");

	ASSERT(pucDa);
	ASSERT(pucSa);
	ASSERT(pucPayload);
	ASSERT(pucMic);
	ASSERT(pucMicKey);

	DBGLOG(RSN, LOUD, "MIC key %02x-%02x-%02x-%02x %02x-%02x-%02x-%02x\n",
			   pucMicKey[0], pucMicKey[1], pucMicKey[2], pucMicKey[3],
			   pucMicKey[4], pucMicKey[5], pucMicKey[6], pucMicKey[7]);

	tkipMicGen(pucMicKey, (PUINT_8) pucPayload, u2PayloadLen, pucSa, pucDa, ucPriority, aucMic);

	kalMemCopy((PUINT_8) pucMic, &aucMic[0], WLAN_MAC_MIC_LEN);

	DBGLOG(RSN, LOUD, "Mic %02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x\n",
			   pucMic[0], pucMic[1], pucMic[2], pucMic[3], pucMic[4], pucMic[5], pucMic[6], pucMic[7]);

}				/* tkipSwMsduEncapsulate */

/*----------------------------------------------------------------------------*/
/*!
* \brief This function decapsulate MSDU frame body (with MIC) according
*        to IEEE 802.11i TKIP sepcification.
*
* \param[in]  prAdapter Pointer to the adapter object data area.
* \param[in]  prMacHeader Pointer to frame MAC header
* \param[in]  pucFrameBody Pointer to frame body
* \param[in]  u4FrameBodyLen Length of frame body (in bytes), include
*                            length of ICV and MIC
* \param[in]  pucMickey Pointer to MIC key
* \param[out] pu4ResultFrameBodyLen Pointer to put the result frame body length.
*
* \retval FALSE(TKIP_MIC_ERR), if this MSDU is not decapsulatable, i.e. MIC
*         verification is failure.
*         TRUE(TKIP_DECAPSULATE_SUCCESS), if this TKIP MSDU is decapsulated
*         successfully, i.e. MIC verification is successful.
*
* \note 1  If return TRUE, result frame body length
*          is only equal to data payload legth, and the result frame
*          body's format is MSDU
*       2. If return FALSE, result frame body length is equal
*          to data payload legth plus MIC and MIC', and the result
*          frame body's format is: MSDU + MIC
*/
/*----------------------------------------------------------------------------*/
BOOLEAN tkipMicDecapsulate(IN P_SW_RFB_T prSwRfb, IN PUINT_8 pucMicKey)
{
	PUCHAR pucMic1;		/* MIC  */
	UCHAR aucMic2[8];	/* MIC' */
	UCHAR ucPriority;
	BOOLEAN fgStatus;
	PUCHAR pucSa, pucDa;
	/* PUCHAR              pucMickey; */
	PUCHAR pucFrameBody;
	UINT_16 u2FrameBodyLen;
	P_WLAN_MAC_HEADER_T prMacHeader;

	DEBUGFUNC("tkipMicDecapsulate");

	ASSERT(prSwRfb);
	ASSERT(pucMicKey);

	/* prRxStatus = prSwRfb->prRxStatus; */
	pucFrameBody = prSwRfb->pucPayload;
	if (!pucFrameBody) {
		DBGLOG(RSN, INFO, "pucPayload is NULL, drop this packet");
		return FALSE;
	}
	u2FrameBodyLen = prSwRfb->u2PayloadLength;

	/* if ((prRxStatus->ucKIdxSecMode & BITS(0,3)) != CIPHER_SUITE_TKIP_WO_MIC){ */
	/* return TRUE; */
	/* } */

	DBGLOG(RSN, LOUD, "Before TKIP MSDU Decapsulate:\n");
	DBGLOG(RSN, LOUD, "MIC key:\n");
	/* DBGLOG_MEM8(RSN, LOUD, pucMicKey, 8); */

	prMacHeader = (P_WLAN_MAC_HEADER_T) prSwRfb->pvHeader;
	ASSERT(prMacHeader);

	pucDa = prMacHeader->aucAddr1;
	pucSa = prMacHeader->aucAddr3;

	switch (prMacHeader->u2FrameCtrl & MASK_TO_DS_FROM_DS) {
	case 0:
		pucDa = prMacHeader->aucAddr1;
		pucSa = prMacHeader->aucAddr2;
		break;
	case MASK_FC_FROM_DS:
		pucDa = prMacHeader->aucAddr1;
		pucSa = prMacHeader->aucAddr3;
		break;
	default:
		ASSERT((prMacHeader->u2FrameCtrl & MASK_FC_TO_DS) == 0);
		return TRUE;
	}

	if (RXM_IS_QOS_DATA_FRAME(prSwRfb->u2FrameCtrl))
		ucPriority = (UCHAR) ((((P_WLAN_MAC_HEADER_QOS_T) prSwRfb->pvHeader)->u2QosCtrl) & MASK_QC_TID);
	else
		ucPriority = 0;

	/* generate MIC' */
	tkipMicGen(pucMicKey, pucFrameBody, u2FrameBodyLen - WLAN_MAC_MIC_LEN, pucSa, pucDa, ucPriority, aucMic2);

	/* verify MIC and MIC' */
	pucMic1 = &pucFrameBody[u2FrameBodyLen - WLAN_MAC_MIC_LEN];
	if (pucMic1[0] == aucMic2[0] && pucMic1[1] == aucMic2[1] &&
	    pucMic1[2] == aucMic2[2] && pucMic1[3] == aucMic2[3] &&
	    pucMic1[4] == aucMic2[4] && pucMic1[5] == aucMic2[5] &&
	    pucMic1[6] == aucMic2[6] && pucMic1[7] == aucMic2[7]) {
		u2FrameBodyLen -= WLAN_MAC_MIC_LEN;
		fgStatus = TRUE;
	} else {
		fgStatus = FALSE;
	}

	/* DBGLOG(RSN, LOUD, ("TKIP MIC:\n")); */
	/* DBGLOG_MEM8(RSN, LOUD, pucMic1, 8); */
	/* DBGLOG(RSN, LOUD, ("TKIP MIC':\n")); */
	/* DBGLOG_MEM8(RSN, LOUD, aucMic2, 8); */

	prSwRfb->u2PayloadLength = u2FrameBodyLen;

	DBGLOG(RSN, LOUD, "After TKIP MSDU Decapsulate:\n");
	DBGLOG(RSN, LOUD, "Frame body: (length = %u)\n", u2FrameBodyLen);
	/* DBGLOG_MEM8(RSN, LOUD, pucFrameBody, u2FrameBodyLen); */

	return fgStatus;

}				/* tkipMicDecapsulate */
