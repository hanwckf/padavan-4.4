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
#include "gl_os.h"
#include "gl_wext.h"
#include "precomp.h"
#if CFG_TC1_FEATURE
#include <tc1_partition.h>
#endif
#if CFG_SUPPORT_AGPS_ASSIST
#include <net/netlink.h>
#endif
#if CFG_SUPPORT_WAKEUP_REASON_DEBUG
#include <mt_sleep.h>
#endif

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/

#define OPEN_FIRMWARE_BY_REQUEST		1

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/
#if DBG
int allocatedMemSize;
#endif

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/
/* #define MTK_DMA_BUF_MEMCPY_SUP */
static PVOID pvIoBuffer;

#ifdef MTK_DMA_BUF_MEMCPY_SUP
static PVOID pvIoPhyBuf;
static PVOID pvDmaBuffer;
static PVOID pvDmaPhyBuf;
#endif /* MTK_DMA_BUF_MEMCPY_SUP */

static UINT_32 pvIoBufferSize;
static UINT_32 pvIoBufferUsage;
static struct KAL_HALT_CTRL_T rHaltCtrl = {
	.lock = __SEMAPHORE_INITIALIZER(rHaltCtrl.lock, 1),
	.owner = NULL,
	.fgHalt = TRUE,
	.fgHeldByKalIoctl = FALSE,
	.u4HoldStart = 0,
};

/* framebuffer callback related variable and status flag */
static struct notifier_block wlan_fb_notifier;
void *wlan_fb_notifier_priv_data;
BOOLEAN wlan_fb_power_down = FALSE;
/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
#if defined(MT6620) && CFG_MULTI_ECOVER_SUPPORT
typedef enum _ENUM_WMTHWVER_TYPE_T {
	WMTHWVER_MT6620_E1 = 0x0,
	WMTHWVER_MT6620_E2 = 0x1,
	WMTHWVER_MT6620_E3 = 0x2,
	WMTHWVER_MT6620_E4 = 0x3,
	WMTHWVER_MT6620_E5 = 0x4,
	WMTHWVER_MT6620_E6 = 0x5,
	WMTHWVER_MT6620_MAX,
	WMTHWVER_INVALID = 0xff
} ENUM_WMTHWVER_TYPE_T, *P_ENUM_WMTHWVER_TYPE_T;
#endif

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/
VOID kalHifAhbKalWakeLockTimeout(IN P_GLUE_INFO_T prGlueInfo)
{
	KAL_WAKE_LOCK_TIMEOUT(prGlueInfo->prAdapter, &(prGlueInfo->rAhbIsrWakeLock), (HZ / 10));	/* 100ms */
}

#if CFG_ENABLE_FW_DOWNLOAD

#if OPEN_FIRMWARE_BY_REQUEST
/*----------------------------------------------------------------------------*/
/*!
* \brief This routine is used to load firmware image
*
* \param pvGlueInfo     Pointer of GLUE Data Structure
* \param ppvMapFileBuf  Pointer of pointer to memory-mapped firmware image
* \param pu4FileLength  File length and memory mapped length as well

* \retval Map File Handle, used for unammping
*/
/*----------------------------------------------------------------------------*/
PVOID kalFirmwareImageMapping(IN P_GLUE_INFO_T prGlueInfo, OUT PPVOID ppvMapFileBuf, OUT PUINT_32 pu4FileLength)
{
	GL_HIF_INFO_T *prHifInfo = &prGlueInfo->rHifInfo;
	INT_32 i4Ret = 0;
	UINT_8 aucFilePath[32];

	DEBUGFUNC("kalFirmwareImageMapping");

	ASSERT(prGlueInfo);
	ASSERT(ppvMapFileBuf);
	ASSERT(pu4FileLength);

	prGlueInfo->prFw = NULL;
	kalMemZero(aucFilePath, sizeof(aucFilePath));

#if defined(MT6620) & CFG_MULTI_ECOVER_SUPPORT
	switch (mtk_wcn_wmt_hwver_get()) {
	case WMTHWVER_MT6620_E1:
	case WMTHWVER_MT6620_E2:
	case WMTHWVER_MT6620_E3:
	case WMTHWVER_MT6620_E4:
	case WMTHWVER_MT6620_E5:
		kalMemCopy(aucFilePath, CFG_FW_FILENAME,
			strlen(CFG_FW_FILENAME));
		break;
	case WMTHWVER_MT6620_E6:
	default:
		kalMemCopy(aucFilePath, CFG_FW_FILENAME "_E6",
			strlen(CFG_FW_FILENAME "_E6"));
		break;
	}
#elif defined(MT6628)
#if 0				/* new wifi ram code mechanism, waiting firmware ready, then we can enable these code */
	kalMemCopy(aucFilePath, CFG_FW_FILENAME "_AD",
		strlen(CFG_FW_FILENAME "_AD"));
#endif
	kalMemCopy(aucFilePath, CFG_FW_FILENAME "_",
		strlen(CFG_FW_FILENAME "_"));
	glGetChipInfo(prGlueInfo, &aucFilePath[strlen(CFG_FW_FILENAME "_")]);
#else
	kalMemCopy(aucFilePath, CFG_FW_FILENAME,
		strlen(CFG_FW_FILENAME));
#endif

	/* <1> Open firmware */
	do {
		i4Ret = request_firmware(&prGlueInfo->prFw, aucFilePath, prHifInfo->Dev);
	} while (i4Ret == -EAGAIN); /* By programming guide */

	if (i4Ret == WLAN_STATUS_SUCCESS) {
		DBGLOG(INIT, INFO, "FW %s: request done [%d]\n", aucFilePath, i4Ret);
	} else {
		DBGLOG(INIT, ERROR, "FW %s: request failed [%d]\n", aucFilePath, i4Ret);
		return NULL;
	}

	*pu4FileLength = prGlueInfo->prFw->size;
	*ppvMapFileBuf = ((u8 *) prGlueInfo->prFw->data);

	return ((u8 *) prGlueInfo->prFw->data);
}

/*----------------------------------------------------------------------------*/
/*!
* \brief This routine is used to unload firmware image mapped memory
*
* \param pvGlueInfo     Pointer of GLUE Data Structure
* \param pvFwHandle     Pointer to mapping handle
* \param pvMapFileBuf   Pointer to memory-mapped firmware image
*
* \retval none
*/
/*----------------------------------------------------------------------------*/

VOID kalFirmwareImageUnmapping(IN P_GLUE_INFO_T prGlueInfo, IN PVOID prFwHandle, IN PVOID pvMapFileBuf)
{
	DEBUGFUNC("kalFirmwareImageUnmapping");

	ASSERT(prGlueInfo);
	ASSERT(pvMapFileBuf);

	release_firmware(prGlueInfo->prFw);

}

#else

static struct file *filp;
static uid_t orgfsuid;
static gid_t orgfsgid;
static mm_segment_t orgfs;

/*----------------------------------------------------------------------------*/
/*!
* \brief This function is provided by GLUE Layer for internal driver stack to
*        open firmware image in kernel space
*
* \param[in] prGlueInfo     Pointer of GLUE Data Structure
*
* \retval WLAN_STATUS_SUCCESS.
* \retval WLAN_STATUS_FAILURE.
*
*/
/*----------------------------------------------------------------------------*/
WLAN_STATUS kalFirmwareOpen(IN P_GLUE_INFO_T prGlueInfo)
{
	UINT_8 aucFilePath[50];

	/* FIX ME: since we don't have hotplug script in the filesystem,
	 * so the request_firmware() KAPI can not work properly
	 */

	/*
	 * save uid and gid used for filesystem access.
	 * set user and group to 0(root)
	 */
	struct cred *cred = (struct cred *)get_current_cred();

	orgfsuid = cred->fsuid.val;
	orgfsgid = cred->fsgid.val;
	cred->fsuid.val = cred->fsgid.val = 0;

	ASSERT(prGlueInfo);

	orgfs = get_fs();
	set_fs(get_ds());

	/* open the fw file */
#if defined(MT6620) & CFG_MULTI_ECOVER_SUPPORT
	switch (mtk_wcn_wmt_hwver_get()) {
	case WMTHWVER_MT6620_E1:
	case WMTHWVER_MT6620_E2:
	case WMTHWVER_MT6620_E3:
	case WMTHWVER_MT6620_E4:
	case WMTHWVER_MT6620_E5:
		filp = filp_open("/vendor/firmware/" CFG_FW_FILENAME, O_RDONLY, 0);
		break;

	case WMTHWVER_MT6620_E6:
	default:
		filp = filp_open("/vendor/firmware/" CFG_FW_FILENAME "_E6", O_RDONLY, 0);
		break;
	}
#elif defined(MT6628)
/* filp = filp_open("/vendor/firmware/"CFG_FW_FILENAME"_MT6628", O_RDONLY, 0); */
/* filp = filp_open("/vendor/firmware/"CFG_FW_FILENAME"_MT6582", O_RDONLY, 0); */
#if 0				/* new wifi ram code mechanism, waiting firmware ready, then we can enable these code */
	kalMemZero(aucFilePath, sizeof(aucFilePath));
	kalMemCopy(aucFilePath, "/vendor/firmware/" CFG_FW_FILENAME "_AD",
		sizeof("/vendor/firmware/" CFG_FW_FILENAME "_AD"));
	filp = filp_open(aucFilePath, O_RDONLY, 0);
	if (!IS_ERR(filp))
		goto open_success;
#endif
	kalMemZero(aucFilePath, sizeof(aucFilePath));
	kalMemCopy(aucFilePath, "/vendor/firmware/" CFG_FW_FILENAME "_",
		strlen("/vendor/firmware/" CFG_FW_FILENAME "_"));
	glGetChipInfo(prGlueInfo, &aucFilePath[strlen("/vendor/firmware/" CFG_FW_FILENAME "_")]);

	DBGLOG(INIT, INFO, "open file: %s\n", aucFilePath);

	filp = filp_open(aucFilePath, O_RDONLY, 0);
#else
	filp = filp_open("/vendor/firmware/" CFG_FW_FILENAME, O_RDONLY, 0);
#endif
	if (IS_ERR(filp)) {
		DBGLOG(INIT, ERROR, "Open FW image: %s failed\n", CFG_FW_FILENAME);
		goto error_open;
	}
#if 0
open_success:
#endif
	DBGLOG(INIT, TRACE, "Open FW image: %s done\n", CFG_FW_FILENAME);
	return WLAN_STATUS_SUCCESS;

error_open:
	/* restore */
	set_fs(orgfs);
	cred->fsuid.val = orgfsuid;
	cred->fsgid.val = orgfsgid;
	put_cred(cred);
	return WLAN_STATUS_FAILURE;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief This function is provided by GLUE Layer for internal driver stack to
*        release firmware image in kernel space
*
* \param[in] prGlueInfo     Pointer of GLUE Data Structure
*
* \retval WLAN_STATUS_SUCCESS.
* \retval WLAN_STATUS_FAILURE.
*
*/
/*----------------------------------------------------------------------------*/
WLAN_STATUS kalFirmwareClose(IN P_GLUE_INFO_T prGlueInfo)
{
	ASSERT(prGlueInfo);

	if ((filp != NULL) && !IS_ERR(filp)) {
		/* close firmware file */
		filp_close(filp, NULL);

		/* restore */
		set_fs(orgfs);
		{
			struct cred *cred = (struct cred *)get_current_cred();

			cred->fsuid.val = orgfsuid;
			cred->fsgid.val = orgfsgid;
			put_cred(cred);
		}
		filp = NULL;
	}

	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief This function is provided by GLUE Layer for internal driver stack to
*        load firmware image in kernel space
*
* \param[in] prGlueInfo     Pointer of GLUE Data Structure
*
* \retval WLAN_STATUS_SUCCESS.
* \retval WLAN_STATUS_FAILURE.
*
*/
/*----------------------------------------------------------------------------*/
WLAN_STATUS kalFirmwareLoad(IN P_GLUE_INFO_T prGlueInfo, OUT PVOID prBuf, IN UINT_32 u4Offset, OUT PUINT_32 pu4Size)
{
	ASSERT(prGlueInfo);
	ASSERT(pu4Size);
	ASSERT(prBuf);

	/* l = filp->f_path.dentry->d_inode->i_size; */

	/* the object must have a read method */
	if ((filp == NULL) || IS_ERR(filp) || (filp->f_op == NULL)) {
		goto error_read;
	} else {
		filp->f_pos = u4Offset;
		DBGLOG(INIT, INFO, "kalFirmwareLoad read start!\n");
		*pu4Size = __vfs_read(filp, (__force void __user *)prBuf, *pu4Size, &filp->f_pos);
	}

	return WLAN_STATUS_SUCCESS;

error_read:
	return WLAN_STATUS_FAILURE;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief This function is provided by GLUE Layer for internal driver stack to
*        query firmware image size in kernel space
*
* \param[in] prGlueInfo     Pointer of GLUE Data Structure
*
* \retval WLAN_STATUS_SUCCESS.
* \retval WLAN_STATUS_FAILURE.
*
*/
/*----------------------------------------------------------------------------*/

WLAN_STATUS kalFirmwareSize(IN P_GLUE_INFO_T prGlueInfo, OUT PUINT_32 pu4Size)
{
	ASSERT(prGlueInfo);
	ASSERT(pu4Size);

	*pu4Size = filp->f_path.dentry->d_inode->i_size;

	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief This routine is used to load firmware image
*
* \param pvGlueInfo     Pointer of GLUE Data Structure
* \param ppvMapFileBuf  Pointer of pointer to memory-mapped firmware image
* \param pu4FileLength  File length and memory mapped length as well

* \retval Map File Handle, used for unammping
*/
/*----------------------------------------------------------------------------*/

PVOID kalFirmwareImageMapping(IN P_GLUE_INFO_T prGlueInfo, OUT PPVOID ppvMapFileBuf, OUT PUINT_32 pu4FileLength)
{
	UINT_32 u4FwSize = 0;
	PVOID prFwBuffer = NULL;

	DEBUGFUNC("kalFirmwareImageMapping");

	ASSERT(prGlueInfo);
	ASSERT(ppvMapFileBuf);
	ASSERT(pu4FileLength);

	do {
		/* <1> Open firmware */
		if (kalFirmwareOpen(prGlueInfo) != WLAN_STATUS_SUCCESS) {
			DBGLOG(INIT, TRACE, "kalFirmwareOpen fail!\n");
			break;
		}

		/* <2> Query firmare size */
		kalFirmwareSize(prGlueInfo, &u4FwSize);
		/* <3> Use vmalloc for allocating large memory trunk */
		prFwBuffer = vmalloc(ALIGN_4(u4FwSize));
		/* <4> Load image binary into buffer */
		if (kalFirmwareLoad(prGlueInfo, prFwBuffer, 0, &u4FwSize) != WLAN_STATUS_SUCCESS) {
			vfree(prFwBuffer);
			kalFirmwareClose(prGlueInfo);
			DBGLOG(INIT, TRACE, "kalFirmwareLoad fail!\n");
			break;
		}
		/* <5> write back info */
		*pu4FileLength = u4FwSize;
		*ppvMapFileBuf = prFwBuffer;

		return prFwBuffer;

	} while (FALSE);

	return NULL;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief This routine is used to unload firmware image mapped memory
*
* \param pvGlueInfo     Pointer of GLUE Data Structure
* \param pvFwHandle     Pointer to mapping handle
* \param pvMapFileBuf   Pointer to memory-mapped firmware image
*
* \retval none
*/
/*----------------------------------------------------------------------------*/

VOID kalFirmwareImageUnmapping(IN P_GLUE_INFO_T prGlueInfo, IN PVOID prFwHandle, IN PVOID pvMapFileBuf)
{
	DEBUGFUNC("kalFirmwareImageUnmapping");

	ASSERT(prGlueInfo);

	/* pvMapFileBuf might be NULL when file doesn't exist */
	if (pvMapFileBuf)
		vfree(pvMapFileBuf);

	kalFirmwareClose(prGlueInfo);
}

#endif /* OPEN_FIRMWARE_BY_REQUEST */

#endif /* CFG_ENABLE_FW_DOWNLOAD */

/*----------------------------------------------------------------------------*/
/*!
* \brief This function is provided by GLUE Layer for internal driver stack to acquire
*        OS SPIN_LOCK.
*
* \param[in] prGlueInfo     Pointer of GLUE Data Structure
* \param[in] rLockCategory  Specify which SPIN_LOCK
* \param[out] pu4Flags      Pointer of a variable for saving IRQ flags
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/
VOID
kalAcquireSpinLock(IN P_GLUE_INFO_T prGlueInfo, IN ENUM_SPIN_LOCK_CATEGORY_E rLockCategory, OUT unsigned long *pu4Flags)
{
	unsigned long u4Flags = 0;

	ASSERT(prGlueInfo);
	ASSERT(pu4Flags);

	if (rLockCategory < SPIN_LOCK_NUM) {

#if CFG_USE_SPIN_LOCK_BOTTOM_HALF
		spin_lock_bh(&prGlueInfo->rSpinLock[rLockCategory]);
#else /* !CFG_USE_SPIN_LOCK_BOTTOM_HALF */
		spin_lock_irqsave(&prGlueInfo->rSpinLock[rLockCategory], u4Flags);
#endif /* !CFG_USE_SPIN_LOCK_BOTTOM_HALF */

		*pu4Flags = u4Flags;
/* DBGLOG(INIT, TRACE, ("A+%d\n", rLockCategory)); */
	}

}				/* end of kalAcquireSpinLock() */

/*----------------------------------------------------------------------------*/
/*!
* \brief This function is provided by GLUE Layer for internal driver stack to release
*        OS SPIN_LOCK.
*
* \param[in] prGlueInfo     Pointer of GLUE Data Structure
* \param[in] rLockCategory  Specify which SPIN_LOCK
* \param[in] u4Flags        Saved IRQ flags
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/
VOID kalReleaseSpinLock(IN P_GLUE_INFO_T prGlueInfo, IN ENUM_SPIN_LOCK_CATEGORY_E rLockCategory, IN UINT_32 u4Flags)
{
	ASSERT(prGlueInfo);

	if (rLockCategory < SPIN_LOCK_NUM) {
		/* DBGLOG(INIT, TRACE, ("A-%d %d %d\n", rLockCategory, u4MemAllocCnt, u4MemFreeCnt)); */
#if CFG_USE_SPIN_LOCK_BOTTOM_HALF
		spin_unlock_bh(&prGlueInfo->rSpinLock[rLockCategory]);
#else /* !CFG_USE_SPIN_LOCK_BOTTOM_HALF */
		spin_unlock_irqrestore(&prGlueInfo->rSpinLock[rLockCategory], u4Flags);
#endif /* !CFG_USE_SPIN_LOCK_BOTTOM_HALF */

	}

}				/* end of kalReleaseSpinLock() */

/*----------------------------------------------------------------------------*/
/*!
* \brief This function is provided by GLUE Layer for internal driver stack to update
*        current MAC address.
*
* \param[in] prGlueInfo     Pointer of GLUE Data Structure
* \param[in] pucMacAddr     Pointer of current MAC address
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/
VOID kalUpdateMACAddress(IN P_GLUE_INFO_T prGlueInfo, IN PUINT_8 pucMacAddr)
{
	ASSERT(prGlueInfo);
	ASSERT(pucMacAddr);

	if (UNEQUAL_MAC_ADDR(prGlueInfo->prDevHandler->dev_addr, pucMacAddr))
		memcpy(prGlueInfo->prDevHandler->dev_addr, pucMacAddr, PARAM_MAC_ADDR_LEN);

}

#if CFG_TCP_IP_CHKSUM_OFFLOAD
/*----------------------------------------------------------------------------*/
/*!
* \brief To query the packet information for offload related parameters.
*
* \param[in] pvPacket Pointer to the packet descriptor.
* \param[in] pucFlag  Points to the offload related parameter.
*
* \return (none)
*
*/
/*----------------------------------------------------------------------------*/
VOID kalQueryTxChksumOffloadParam(IN PVOID pvPacket, OUT PUINT_8 pucFlag)
{
	struct sk_buff *skb = (struct sk_buff *)pvPacket;
	UINT_8 ucFlag = 0;

	ASSERT(pvPacket);
	ASSERT(pucFlag);


	if (skb->ip_summed == CHECKSUM_PARTIAL) {
#if DBG
		/* Kevin: do double check, we can remove this part in Normal Driver.
		 * Because we register NIC feature with NETIF_F_IP_CSUM for MT5912B MAC, so
		 * we'll process IP packet only.
		 */
		if (skb->protocol != htons(ETH_P_IP)) {
			/* printk("Wrong skb->protocol( = %08x) for TX Checksum Offload.\n", skb->protocol); */
		} else
#endif
		ucFlag |= (TX_CS_IP_GEN | TX_CS_TCP_UDP_GEN);
	}

	*pucFlag = ucFlag;

}				/* kalQueryChksumOffloadParam */

/* 4 2007/10/8, mikewu, this is rewritten by Mike */
/*----------------------------------------------------------------------------*/
/*!
* \brief To update the checksum offload status to the packet to be indicated to OS.
*
* \param[in] pvPacket Pointer to the packet descriptor.
* \param[in] pucFlag  Points to the offload related parameter.
*
* \return (none)
*
*/
/*----------------------------------------------------------------------------*/
VOID kalUpdateRxCSUMOffloadParam(IN PVOID pvPacket, IN ENUM_CSUM_RESULT_T aeCSUM[])
{
	struct sk_buff *skb = (struct sk_buff *)pvPacket;

	ASSERT(pvPacket);

	if ((aeCSUM[CSUM_TYPE_IPV4] == CSUM_RES_SUCCESS || aeCSUM[CSUM_TYPE_IPV6] == CSUM_RES_SUCCESS) &&
	    ((aeCSUM[CSUM_TYPE_TCP] == CSUM_RES_SUCCESS) || (aeCSUM[CSUM_TYPE_UDP] == CSUM_RES_SUCCESS))) {
		skb->ip_summed = CHECKSUM_UNNECESSARY;
	} else {
		skb->ip_summed = CHECKSUM_NONE;
#if DBG
		if (aeCSUM[CSUM_TYPE_IPV4] == CSUM_RES_NONE && aeCSUM[CSUM_TYPE_IPV6] == CSUM_RES_NONE)
			DBGLOG(RX, TRACE, "RX: \"non-IPv4/IPv6\" Packet\n");
		else if (aeCSUM[CSUM_TYPE_IPV4] == CSUM_RES_FAILED)
			DBGLOG(RX, TRACE, "RX: \"bad IP Checksum\" Packet\n");
		else if (aeCSUM[CSUM_TYPE_TCP] == CSUM_RES_FAILED)
			DBGLOG(RX, TRACE, "RX: \"bad TCP Checksum\" Packet\n");
		else if (aeCSUM[CSUM_TYPE_UDP] == CSUM_RES_FAILED)
			DBGLOG(RX, TRACE, "RX: \"bad UDP Checksum\" Packet\n");
		else
			/* Do nothing */
#endif
	}

}				/* kalUpdateRxCSUMOffloadParam */
#endif /* CFG_TCP_IP_CHKSUM_OFFLOAD */

/*----------------------------------------------------------------------------*/
/*!
* \brief This function is called to free packet allocated from kalPacketAlloc.
*
* \param[in] prGlueInfo     Pointer of GLUE Data Structure
* \param[in] pvPacket       Pointer of the packet descriptor
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/
VOID kalPacketFree(IN P_GLUE_INFO_T prGlueInfo, IN PVOID pvPacket)
{
	dev_kfree_skb((struct sk_buff *)pvPacket);
	if (prGlueInfo)
		prGlueInfo->u8SkbFreed++;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief Only handles driver own creating packet (coalescing buffer).
*
* \param prGlueInfo   Pointer of GLUE Data Structure
* \param u4Size       Pointer of Packet Handle
* \param ppucData     Status Code for OS upper layer
*
* \return NULL: Failed to allocate skb, Not NULL get skb
*/
/*----------------------------------------------------------------------------*/
PVOID kalPacketAlloc(IN P_GLUE_INFO_T prGlueInfo, IN UINT_32 u4Size, OUT PUINT_8 *ppucData)
{
	struct sk_buff *prSkb = dev_alloc_skb(u4Size);

	if (prSkb)
		*ppucData = (PUINT_8) (prSkb->data);
#if DBG
	{
		PUINT_32 pu4Head = (PUINT_32) &prSkb->cb[0];
		*pu4Head = (UINT_32) prSkb->head;
		DBGLOG(RX, TRACE, "prSkb->head = %#x, prSkb->cb = %#x\n", (UINT_32) prSkb->head, *pu4Head);
	}
#endif
	return (PVOID) prSkb;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief Process the received packet for indicating to OS.
*
* \param[in] prGlueInfo     Pointer to the Adapter structure.
* \param[in] pvPacket       Pointer of the packet descriptor
* \param[in] pucPacketStart The starting address of the buffer of Rx packet.
* \param[in] u4PacketLen    The packet length.
* \param[in] pfgIsRetain    Is the packet to be retained.
* \param[in] aerCSUM        The result of TCP/ IP checksum offload.
*
* \retval WLAN_STATUS_SUCCESS.
* \retval WLAN_STATUS_FAILURE.
*
*/
/*----------------------------------------------------------------------------*/
WLAN_STATUS
kalProcessRxPacket(IN P_GLUE_INFO_T prGlueInfo, IN PVOID pvPacket, IN PUINT_8 pucPacketStart, IN UINT_32 u4PacketLen,
		   /* IN PBOOLEAN           pfgIsRetain, */
		   IN BOOLEAN fgIsRetain, IN ENUM_CSUM_RESULT_T aerCSUM[])
{
	WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
	struct sk_buff *skb = (struct sk_buff *)pvPacket;

	skb->data = pucPacketStart;
	skb_reset_tail_pointer(skb);	/* reset tail pointer first, for 64bit kernel,we should call linux kernel API */
	skb_trim(skb, 0);	/* only if skb->len > len, then skb_trim has effect */
	skb_put(skb, u4PacketLen);	/* shift tail and skb->len to correct value */

#if CFG_TCP_IP_CHKSUM_OFFLOAD
	kalUpdateRxCSUMOffloadParam(skb, aerCSUM);
#endif

	return rStatus;
}

#if (CONF_HIF_LOOPBACK_AUTO == 1)
/*----------------------------------------------------------------------------*/
/*!
* \brief Do HIF loopback test.
*
* \param[in] GlueInfo   Pointer to the GLUE_INFO_T structure.
*
* \retval None
*/
/*----------------------------------------------------------------------------*/
unsigned int testmode;
unsigned int testlen = 64;

void kalDevLoopbkAuto(IN GLUE_INFO_T *GlueInfo)
{
#define HIF_LOOPBK_AUTO_TEST_LEN    1600
/* GL_HIF_INFO_T *HifInfo; */
	static unsigned int txcnt;
	struct sk_buff *MsduInfo;
	UINT_8 *Pkt;
	UINT_32 RegVal;
	UINT_32 PktLen = 16;

	/* Init */
	if (testmode != 0) {
		PktLen = kalRandomNumber() % 1520;
		if (PktLen < 64)
			PktLen = 64;
	} else {
		PktLen = testlen++;
		if (PktLen > 1520) {
			testmode = 1;
			PktLen = 64;
		}
	}

/* PktLen = 100; */
	DBGLOG(INIT, INFO, "kalDevLoopbkAuto> Send a packet to HIF (len = %d) (total = %d)...\n", PktLen, ++txcnt);
/* HifInfo = &GlueInfo->rHifInfo; */

	/* Allocate a MSDU_INFO_T */
	MsduInfo = kalPacketAlloc(GlueInfo, HIF_LOOPBK_AUTO_TEST_LEN, &Pkt);
	if (MsduInfo == NULL) {
		DBGLOG(INIT, WARN, "No PKT_INFO_T for sending loopback packet!\n");
		return;
	}

	/* Init the packet */
	MsduInfo->dev = GlueInfo->prDevHandler;
	if (MsduInfo->dev == NULL) {
		DBGLOG(INIT, WARN, "MsduInfo->dev == NULL!!\n");
		kalPacketFree(GlueInfo, MsduInfo);
		return;
	}

	MsduInfo->len = PktLen;
	kalMemSet(MsduInfo->data, 0xff, 6);
	kalMemSet(MsduInfo->data + 6, 0x5a, PktLen - 6);

	/* Simulate OS to send the packet */
	wlanHardStartXmit(MsduInfo, MsduInfo->dev);

#if 0
	PktLen += 4;
	if (PktLen >= 1600)
		PktLen = 16;
#endif

	/* Note: in FPGA, clock is not accuracy so 3000 here, not 10000 */
/* HifInfo->HifTmrLoopbkFn.expires = jiffies + MSEC_TO_SYSTIME(1000); */
/* add_timer(&(HifInfo->HifTmrLoopbkFn)); */
}

int kalDevLoopbkThread(IN void *data)
{
	struct net_device *dev = data;
	P_GLUE_INFO_T GlueInfo = *((P_GLUE_INFO_T *) netdev_priv(dev));
	GL_HIF_INFO_T *HifInfo = &GlueInfo->rHifInfo;
	int ret;
	static int test;

	while (TRUE) {
		ret = wait_event_interruptible(HifInfo->HifWaitq, (HifInfo->HifLoopbkFlg != 0));

		if (HifInfo->HifLoopbkFlg == 0xFFFFFFFF)
			break;

		while (TRUE) {
			/* if ((HifInfo->HifLoopbkFlg & 0x01) == 0) */
			if (GlueInfo->i4TxPendingFrameNum < 64) {
				DBGLOG(INIT, INFO, "GlueInfo->i4TxPendingFrameNum = %d\n",
					GlueInfo->i4TxPendingFrameNum);
				kalDevLoopbkAuto(GlueInfo);

				if (testmode == 0)
					kalMsleep(3000);
			} else
				kalMsleep(1);
		}
	}
}

void kalDevLoopbkRxHandle(IN P_ADAPTER_T prAdapter, IN OUT P_SW_RFB_T prSwRfb)
{
	static unsigned int rxcnt;
	UINT_32 i;
	UINT_8 *Buf = prSwRfb->pucRecvBuff + sizeof(HIF_TX_HEADER_T);
	P_HIF_RX_HEADER_T prHifRxHdr = prSwRfb->prHifRxHdr;
	UINT_32 len = prHifRxHdr->u2PacketLen - sizeof(HIF_TX_HEADER_T);

	if (len > 1600) {
		while (1)
			DBGLOG(INIT, ERROR, "HIF> Loopback len > 1600!!! error!!!\n");
	}

	for (i = 0; i < 6; i++) {
		if (Buf[i] != 0xff) {
			while (1) {
				DBGLOG(INIT, ERROR, "HIF> Loopbk dst addr error (len = %d)!\n", len);
				dumpMemory8(prSwRfb->pucRecvBuff, prHifRxHdr->u2PacketLen);
			}
		}
	}

	for (i = 6; i < len; i++) {
		if (Buf[i] != 0x5a) {
			while (1) {
				DBGLOG(INIT, ERROR, "HIF> Loopbk error (len = %d)!\n", len);
				dumpMemory8(prSwRfb->pucRecvBuff, prHifRxHdr->u2PacketLen);
			}
		}
	}

	DBGLOG(INIT, INFO, "HIF> Loopbk OK (len = %d) (total = %d)!\n", len, ++rxcnt);
}
#endif /* CONF_HIF_LOOPBACK_AUTO */

/*----------------------------------------------------------------------------*/
/*!
* \brief To indicate an array of received packets is available for higher
*        level protocol uses.
*
* \param[in] prGlueInfo Pointer to the Adapter structure.
* \param[in] apvPkts The packet array to be indicated
* \param[in] ucPktNum The number of packets to be indicated
*
* \retval TRUE Success.
*
*/
/*----------------------------------------------------------------------------*/
WLAN_STATUS kalRxIndicatePkts(IN P_GLUE_INFO_T prGlueInfo, IN PVOID apvPkts[], IN UINT_8 ucPktNum)
{
	UINT_8 ucIdx = 0;
	struct net_device *prNetDev = prGlueInfo->prDevHandler;
	struct sk_buff *prSkb = NULL;

	ASSERT(prGlueInfo);
	ASSERT(apvPkts);

#if CFG_BOW_TEST
	UINT_32 i;
#endif

	for (ucIdx = 0; ucIdx < ucPktNum; ucIdx++) {
		prSkb = apvPkts[ucIdx];
#if DBG
		do {
			PUINT_8 pu4Head = (PUINT_8) &prSkb->cb[0];
			UINT_32 u4HeadValue = 0;

			kalMemCopy(&u4HeadValue, pu4Head, sizeof(u4HeadValue));
			DBGLOG(RX, TRACE, "prSkb->head = %p, prSkb->cb = 0x%x\n", pu4Head, u4HeadValue);
		} while (0);
#endif

		if (GLUE_GET_PKT_IS_P2P(prSkb)) {
			/* P2P */
#if CFG_ENABLE_WIFI_DIRECT
			if (prGlueInfo->prAdapter->fgIsP2PRegistered)
				prNetDev = kalP2PGetDevHdlr(prGlueInfo);
			/* prNetDev->stats.rx_bytes += prSkb->len; */
			/* prNetDev->stats.rx_packets++; */
			prGlueInfo->prP2PInfo->rNetDevStats.rx_bytes += prSkb->len;
			prGlueInfo->prP2PInfo->rNetDevStats.rx_packets++;

#else
			prNetDev = prGlueInfo->prDevHandler;
#endif
		} else if (GLUE_GET_PKT_IS_PAL(prSkb)) {
			/* BOW */
#if CFG_ENABLE_BT_OVER_WIFI && CFG_BOW_SEPARATE_DATA_PATH
			if (prGlueInfo->rBowInfo.fgIsNetRegistered)
				prNetDev = prGlueInfo->rBowInfo.prDevHandler;
#else
			prNetDev = prGlueInfo->prDevHandler;
#endif
		} else {
			/* AIS */
			prNetDev = prGlueInfo->prDevHandler;
			prGlueInfo->rNetDevStats.rx_bytes += prSkb->len;
			prGlueInfo->rNetDevStats.rx_packets++;

		}

		/* check if the "unicast" packet is from us */
		if (kalMemCmp(prSkb->data, prSkb->data + 6, 6) == 0) {
			/* we will filter broadcast/multicast packet sent from us in hardware */
			/* source address = destination address ? */
			DBGLOG(RX, EVENT,
			       "kalRxIndicatePkts got from us!!! Drop it! ([ %pM ] len %d)\n",
				prSkb->data, prSkb->len);
			wlanReturnPacket(prGlueInfo->prAdapter, prSkb);
			continue;
		}
#if (CFG_SUPPORT_TDLS == 1)
		if (TdlsexRxFrameDrop(prGlueInfo, prSkb->data) == TRUE) {
			/* drop the received TDLS action frame */
			DBGLOG(TDLS, WARN,
			       "<tdls_fme> %s: drop a received packet from %pM %u\n",
				__func__, prSkb->data,
				(UINT32) ((P_ADAPTER_T) (prGlueInfo->prAdapter))->rRxCtrl.rFreeSwRfbList.u4NumElem);
			wlanReturnPacket(prGlueInfo->prAdapter, prSkb);
			continue;
		}

		/*
		 * get a TDLS request/response/confirm, we need to parse the HT IE
		 * because older supplicant does not pass HT IE to us
		 */
		TdlsexRxFrameHandle(prGlueInfo, prSkb->data, prSkb->len);
#endif /* CFG_SUPPORT_TDLS */

		STATS_RX_PKT_INFO_DISPLAY(prSkb->data);

		prNetDev->last_rx = jiffies;
		prSkb->protocol = eth_type_trans(prSkb, prNetDev);
		prSkb->dev = prNetDev;
		/* DBGLOG_MEM32(RX, TRACE, (PUINT_32)prSkb->data, prSkb->len); */
		DBGLOG(RX, TRACE, "kalRxIndicatePkts len = %d\n", prSkb->len);

#if CFG_BOW_TEST
		DBGLOG(BOW, TRACE, "Rx sk_buff->len: %d\n", prSkb->len);
		DBGLOG(BOW, TRACE, "Rx sk_buff->data_len: %d\n", prSkb->data_len);
		DBGLOG(BOW, TRACE, "Rx sk_buff->data:\n");

		for (i = 0; i < prSkb->len; i++) {
			DBGLOG(BOW, TRACE, "%4x", prSkb->data[i]);

			if ((i + 1) % 16 == 0)
				DBGLOG(BOW, TRACE, "\n");
		}

		DBGLOG(BOW, TRACE, "\n");
#endif

		if (!in_interrupt())
			netif_rx_ni(prSkb);	/* only in non-interrupt context */
		else
			netif_rx(prSkb);

		wlanReturnPacket(prGlueInfo->prAdapter, NULL);
	}

	if (netif_carrier_ok(prNetDev))
		kalPerMonStart(prGlueInfo);

	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief Called by driver to indicate event to upper layer, for example, the wpa
*        supplicant or wireless tools.
*
* \param[in] pvAdapter Pointer to the adapter descriptor.
* \param[in] eStatus Indicated status.
* \param[in] pvBuf Indicated message buffer.
* \param[in] u4BufLen Indicated message buffer size.
*
* \return (none)
*
*/
/*----------------------------------------------------------------------------*/
UINT_32 ScanCnt = 0, ScanDoneFailCnt = 0;
VOID
kalIndicateStatusAndComplete(IN P_GLUE_INFO_T prGlueInfo, IN WLAN_STATUS eStatus, IN PVOID pvBuf, IN UINT_32 u4BufLen)
{
	UINT_32 bufLen;
	P_PARAM_STATUS_INDICATION_T pStatus = (P_PARAM_STATUS_INDICATION_T) pvBuf;
	P_PARAM_AUTH_EVENT_T pAuth = (P_PARAM_AUTH_EVENT_T) pStatus;
	P_PARAM_PMKID_CANDIDATE_LIST_T pPmkid = (P_PARAM_PMKID_CANDIDATE_LIST_T) (pStatus + 1);
	PARAM_MAC_ADDRESS arBssid;
	struct cfg80211_scan_request *prScanRequest = NULL;
	PARAM_SSID_T ssid;
	struct ieee80211_channel *prChannel = NULL;
	struct cfg80211_bss *bss;
	UINT_8 ucChannelNum;
	P_BSS_DESC_T prBssDesc = NULL;

	GLUE_SPIN_LOCK_DECLARATION();

	kalMemZero(arBssid, MAC_ADDR_LEN);

	ASSERT(prGlueInfo);

	switch (eStatus) {
	case WLAN_STATUS_ROAM_OUT_FIND_BEST:
	case WLAN_STATUS_MEDIA_CONNECT:

		prGlueInfo->eParamMediaStateIndicated = PARAM_MEDIA_STATE_CONNECTED;

		/* indicate assoc event */
		wlanQueryInformation(prGlueInfo->prAdapter, wlanoidQueryBssid, &arBssid[0], sizeof(arBssid), &bufLen);
		wext_indicate_wext_event(prGlueInfo, SIOCGIWAP, arBssid, bufLen);

		/* switch netif on */
		netif_carrier_on(prGlueInfo->prDevHandler);

		do {
			/* print message on console */
			wlanQueryInformation(prGlueInfo->prAdapter, wlanoidQuerySsid, &ssid, sizeof(ssid), &bufLen);

			ssid.aucSsid[(ssid.u4SsidLen >= PARAM_MAX_LEN_SSID) ?
				     (PARAM_MAX_LEN_SSID - 1) : ssid.u4SsidLen] = '\0';
			DBGLOG(AIS, INFO, " %s netif_carrier_on [ssid:%s %pM ]\n",
					    prGlueInfo->prDevHandler->name, ssid.aucSsid, arBssid);
		} while (0);

		if (prGlueInfo->fgIsRegistered == TRUE) {
			struct cfg80211_bss *bss_others = NULL;
			UINT_8 ucLoopCnt = 15; /* only loop 15 times to avoid dead loop */

			/* retrieve channel */
			ucChannelNum = wlanGetChannelNumberByNetwork(prGlueInfo->prAdapter, NETWORK_TYPE_AIS_INDEX);
			if (ucChannelNum <= 14) {
				prChannel =
				    ieee80211_get_channel(priv_to_wiphy(prGlueInfo),
							  ieee80211_channel_to_frequency(ucChannelNum,
											 IEEE80211_BAND_2GHZ));
			} else {
				prChannel =
				    ieee80211_get_channel(priv_to_wiphy(prGlueInfo),
							  ieee80211_channel_to_frequency(ucChannelNum,
											 IEEE80211_BAND_5GHZ));
			}

			if (!prChannel)
				DBGLOG(SCN, ERROR, "prChannel is NULL and ucChannelNum is %d\n", ucChannelNum);

			/* ensure BSS exists */
			bss = cfg80211_get_bss(priv_to_wiphy(prGlueInfo), prChannel, arBssid,
					       ssid.aucSsid, ssid.u4SsidLen, WLAN_CAPABILITY_ESS, WLAN_CAPABILITY_ESS);

			if (bss == NULL) {
				/* create BSS on-the-fly */
				prBssDesc =
				    wlanGetTargetBssDescByNetwork(prGlueInfo->prAdapter, NETWORK_TYPE_AIS_INDEX);

				if ((prBssDesc != NULL) && (prChannel != NULL)) {
					bss = cfg80211_inform_bss(priv_to_wiphy(prGlueInfo), prChannel,
								CFG80211_BSS_FTYPE_PRESP,
								arBssid, 0,	/* TSF */
								WLAN_CAPABILITY_ESS,
								prBssDesc->u2BeaconInterval,	/* beacon interval */
								prBssDesc->aucIEBuf,	/* IE */
								prBssDesc->u2IELength,	/* IE Length */
								RCPI_TO_dBm(prBssDesc->ucRCPI) * 100,	/* MBM */
								GFP_KERNEL);
				}
			}
			/*
			 * remove all bsses that before and only channel different with the current connected one
			 * if without this patch, UI will show channel A is connected even if AP has change channel
			 * from A to B
			 */
			while (ucLoopCnt--) {
				bss_others = cfg80211_get_bss(priv_to_wiphy(prGlueInfo), NULL, arBssid,
						ssid.aucSsid, ssid.u4SsidLen, WLAN_CAPABILITY_ESS, WLAN_CAPABILITY_ESS);
				if (bss && bss_others && bss_others != bss) {
					DBGLOG(SCN, INFO, "remove BSSes that only channel different\n");
					cfg80211_unlink_bss(priv_to_wiphy(prGlueInfo), bss_others);
				} else
					break;
			}

			/* CFG80211 Indication */
			if (eStatus == WLAN_STATUS_ROAM_OUT_FIND_BEST) {
				cfg80211_roamed_bss(prGlueInfo->prDevHandler,
						    bss,
						    prGlueInfo->aucReqIe,
						    prGlueInfo->u4ReqIeLength,
						    prGlueInfo->aucRspIe, prGlueInfo->u4RspIeLength, GFP_KERNEL);
			} else {
				/*
				 * to support user space roaming, cfg80211 will change the sme_state to connecting
				 * before reassociate
				 */
				cfg80211_connect_result(prGlueInfo->prDevHandler,
							arBssid,
							prGlueInfo->aucReqIe,
							prGlueInfo->u4ReqIeLength,
							prGlueInfo->aucRspIe,
							prGlueInfo->u4RspIeLength, WLAN_STATUS_SUCCESS, GFP_KERNEL);
			}
		}

		break;

	case WLAN_STATUS_MEDIA_DISCONNECT:
	case WLAN_STATUS_MEDIA_DISCONNECT_LOCALLY:
		/* indicate disassoc event */
		wext_indicate_wext_event(prGlueInfo, SIOCGIWAP, NULL, 0);
		/*
		 * For CR 90 and CR99, While supplicant do reassociate, driver will do netif_carrier_off first,
		 * after associated success, at joinComplete(), do netif_carier_on,
		 * but for unknown reason, the supplicant 1x pkt will not called the driver
		 * hardStartXmit, for template workaround these bugs, add this compiling flag
		 */
		/* switch netif off */

		DBGLOG(AIS, INFO, "[wifi] %s netif_carrier_off\n",
				    prGlueInfo->prDevHandler->name);

		netif_carrier_off(prGlueInfo->prDevHandler);

		/*Full2Partial*/
		/*at here, should init u4LastFullScanTime, ucTrScanType, ucChannelListNum, ucChannelNum*/
		DBGLOG(INIT, TRACE, "Full2Partial disconenct reset value\n");
		prGlueInfo->u4LastFullScanTime = 0;
		prGlueInfo->ucTrScanType = 0;
		kalMemSet(prGlueInfo->ucChannelNum, 0, FULL_SCAN_MAX_CHANNEL_NUM);
		if (prGlueInfo->puFullScan2PartialChannel != NULL) {
			kalMemFree(prGlueInfo->puFullScan2PartialChannel,
			VIR_MEM_TYPE, sizeof(PARTIAL_SCAN_INFO));
			prGlueInfo->puFullScan2PartialChannel = NULL;
		}

		if (prGlueInfo->fgIsRegistered == TRUE) {
			P_WIFI_VAR_T prWifiVar = &prGlueInfo->prAdapter->rWifiVar;
			UINT_16 u2DeauthReason = prWifiVar->arBssInfo[NETWORK_TYPE_AIS_INDEX].u2DeauthReason;
			/* CFG80211 Indication */
			DBGLOG(AIS, INFO, "[wifi] %s cfg80211_disconnected\n", prGlueInfo->prDevHandler->name);
			cfg80211_disconnected(prGlueInfo->prDevHandler, u2DeauthReason, NULL, 0,
				eStatus == WLAN_STATUS_MEDIA_DISCONNECT_LOCALLY ? true : false, GFP_KERNEL);
		}

		prGlueInfo->eParamMediaStateIndicated = PARAM_MEDIA_STATE_DISCONNECTED;

		break;

	case WLAN_STATUS_SCAN_COMPLETE:
		/* indicate scan complete event */
		wext_indicate_wext_event(prGlueInfo, SIOCGIWSCAN, NULL, 0);

		/* 1. reset first for newly incoming request */
		GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
		if (prGlueInfo->prScanRequest != NULL) {
			prScanRequest = prGlueInfo->prScanRequest;
			prGlueInfo->prScanRequest = NULL;
		}
		GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);

		/* 2. then CFG80211 Indication */
		DBGLOG(SCN, TRACE, "[ais] scan complete %p %d %d\n", prScanRequest, ScanCnt, ScanDoneFailCnt);

		if (prScanRequest != NULL)
			cfg80211_scan_done(prScanRequest, FALSE);
		break;
	case WLAN_STATUS_CONNECT_INDICATION:
		prBssDesc = prGlueInfo->prAdapter->rWifiVar.rAisFsmInfo.prTargetBssDesc;

		if (prBssDesc)
			COPY_MAC_ADDR(arBssid, prBssDesc->aucBSSID);
		else
			COPY_MAC_ADDR(arBssid, prGlueInfo->prAdapter->rWifiVar.rConnSettings.aucBSSID);

		/* indicate AIS Jion fail  event
		*if (prGlueInfo->prDevHandler->ieee80211_ptr->sme_state == CFG80211_SME_CONNECTING)
		*/
		cfg80211_connect_result(prGlueInfo->prDevHandler,
					arBssid,
					prGlueInfo->aucReqIe,
					prGlueInfo->u4ReqIeLength,
					prGlueInfo->aucRspIe,
					prGlueInfo->u4RspIeLength, WLAN_STATUS_AUTH_TIMEOUT, GFP_KERNEL);
		break;

#if 0
	case WLAN_STATUS_MSDU_OK:
		if (netif_running(prGlueInfo->prDevHandler))
			netif_wake_queue(prGlueInfo->prDevHandler);
		break;
#endif

	case WLAN_STATUS_MEDIA_SPECIFIC_INDICATION:
		if (pStatus) {
			switch (pStatus->eStatusType) {
			case ENUM_STATUS_TYPE_AUTHENTICATION:
				/*
				 * printk(KERN_NOTICE "ENUM_STATUS_TYPE_AUTHENTICATION: L(%ld) [ %pM ] F:%lx\n",
				 * pAuth->Request[0].Length,
				 * pAuth->Request[0].Bssid,
				 * pAuth->Request[0].Flags);
				 */
				/* indicate (UC/GC) MIC ERROR event only */
				if ((pAuth->arRequest[0].u4Flags ==
				     PARAM_AUTH_REQUEST_PAIRWISE_ERROR) ||
				    (pAuth->arRequest[0].u4Flags == PARAM_AUTH_REQUEST_GROUP_ERROR)) {
					cfg80211_michael_mic_failure(prGlueInfo->prDevHandler, NULL,
								     (pAuth->arRequest[0].u4Flags ==
								      PARAM_AUTH_REQUEST_PAIRWISE_ERROR) ?
								     NL80211_KEYTYPE_PAIRWISE : NL80211_KEYTYPE_GROUP,
								     0, NULL, GFP_KERNEL);
					wext_indicate_wext_event(prGlueInfo, IWEVMICHAELMICFAILURE,
								 (unsigned char *)&pAuth->arRequest[0],
								 pAuth->arRequest[0].u4Length);
				}
				break;

			case ENUM_STATUS_TYPE_CANDIDATE_LIST:
				/*
				 * printk(KERN_NOTICE "Param_StatusType_PMKID_CandidateList: Ver(%ld) Num(%ld)\n",
				 * pPmkid->u2Version,
				 * pPmkid->u4NumCandidates);
				 * if (pPmkid->u4NumCandidates > 0) {
				 * printk(KERN_NOTICE "candidate[ %pM ] preAuth Flag:%lx\n",
				 * pPmkid->arCandidateList[0].rBSSID,
				 * pPmkid->arCandidateList[0].fgFlags);
				 * }
				 */
				{
					UINT_32 i = 0;
					/*struct net_device *prDev = prGlueInfo->prDevHandler; */
					P_PARAM_PMKID_CANDIDATE_T prCand = NULL;
					/*
					 * indicate pmk candidate via cfg80211 to supplicant,
					 * the second parameter is 1000 for
					 * cfg80211_pmksa_candidate_notify, because wpa_supplicant defined it.
					 */
					for (i = 0; i < pPmkid->u4NumCandidates; i++) {
						prCand = &pPmkid->arCandidateList[i];
						cfg80211_pmksa_candidate_notify(prGlueInfo->prDevHandler, 1000,
										prCand->arBSSID, prCand->u4Flags,
										GFP_KERNEL);

						wext_indicate_wext_event(prGlueInfo,
									 IWEVPMKIDCAND,
									 (unsigned char *)prCand,
									 pPmkid->u4NumCandidates);
					}
				}
				break;

			default:
				/* case ENUM_STATUS_TYPE_MEDIA_STREAM_MODE */
				/*
				 * printk(KERN_NOTICE "unknown media specific indication type:%x\n",
				 * pStatus->StatusType);
				 */
				break;
			}
		} else {
			/*
			 * printk(KERN_WARNING "media specific indication buffer NULL\n");
			 */
		}
		break;

#if CFG_SUPPORT_BCM && CFG_SUPPORT_BCM_BWCS
	case WLAN_STATUS_BWCS_UPDATE:
		{
			wext_indicate_wext_event(prGlueInfo, IWEVCUSTOM, pvBuf, sizeof(PTA_IPC_T));
		}

		break;

#endif

	default:
		/*
		 * printk(KERN_WARNING "unknown indication:%lx\n", eStatus);
		 */
		break;
	}
}				/* kalIndicateStatusAndComplete */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to update the (re)association request
 *        information to the structure used to query and set
 *        OID_802_11_ASSOCIATION_INFORMATION.
 *
 * \param[in] prGlueInfo Pointer to the Glue structure.
 * \param[in] pucFrameBody Pointer to the frame body of the last (Re)Association
 *                         Request frame from the AP.
 * \param[in] u4FrameBodyLen The length of the frame body of the last
 *                           (Re)Association Request frame.
 * \param[in] fgReassocRequest TRUE, if it is a Reassociation Request frame.
 *
 * \return (none)
 *
 */
/*----------------------------------------------------------------------------*/
VOID
kalUpdateReAssocReqInfo(IN P_GLUE_INFO_T prGlueInfo,
			IN PUINT_8 pucFrameBody, IN UINT_32 u4FrameBodyLen, IN BOOLEAN fgReassocRequest)
{
	PUINT_8 cp;

	ASSERT(prGlueInfo);

	/* reset */
	prGlueInfo->u4ReqIeLength = 0;

	if (fgReassocRequest) {
		if (u4FrameBodyLen < 15) {
			/*
			 * printk(KERN_WARNING "frameBodyLen too short:%ld\n", frameBodyLen);
			 */
			return;
		}
	} else {
		if (u4FrameBodyLen < 9) {
			/*
			 * printk(KERN_WARNING "frameBodyLen too short:%ld\n", frameBodyLen);
			 */
			return;
		}
	}

	cp = pucFrameBody;

	if (fgReassocRequest) {
		/* Capability information field 2 */
		/* Listen interval field 2 */
		/* Current AP address 6 */
		cp += 10;
		u4FrameBodyLen -= 10;
	} else {
		/* Capability information field 2 */
		/* Listen interval field 2 */
		cp += 4;
		u4FrameBodyLen -= 4;
	}

	wext_indicate_wext_event(prGlueInfo, IWEVASSOCREQIE, cp, u4FrameBodyLen);

	if (u4FrameBodyLen <= CFG_CFG80211_IE_BUF_LEN) {
		prGlueInfo->u4ReqIeLength = u4FrameBodyLen;
		kalMemCopy(prGlueInfo->aucReqIe, cp, u4FrameBodyLen);
	}

}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is called to update the (re)association
 *        response information to the structure used to reply with
 *        cfg80211_connect_result
 *
 * @param prGlueInfo      Pointer to adapter descriptor
 * @param pucFrameBody    Pointer to the frame body of the last (Re)Association
 *                         Response frame from the AP
 * @param u4FrameBodyLen  The length of the frame body of the last
 *                          (Re)Association Response frame
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
VOID kalUpdateReAssocRspInfo(IN P_GLUE_INFO_T prGlueInfo, IN PUINT_8 pucFrameBody, IN UINT_32 u4FrameBodyLen)
{
	UINT_32 u4IEOffset = 6;	/* cap_info, status_code & assoc_id */
	UINT_32 u4IELength = u4FrameBodyLen - u4IEOffset;

	ASSERT(prGlueInfo);

	/* reset */
	prGlueInfo->u4RspIeLength = 0;

	if (u4IELength <= CFG_CFG80211_IE_BUF_LEN) {
		prGlueInfo->u4RspIeLength = u4IELength;
		kalMemCopy(prGlueInfo->aucRspIe, pucFrameBody + u4IEOffset, u4IELength);
	}

}				/* kalUpdateReAssocRspInfo */

/*----------------------------------------------------------------------------*/
/*!
 * \brief Notify OS with SendComplete event of the specific packet. Linux should
 *        free packets here.
 *
 * \param[in] prGlueInfo     Pointer of GLUE Data Structure
 * \param[in] pvPacket       Pointer of Packet Handle
 * \param[in] status         Status Code for OS upper layer
 *
 * \return -
 */
/*----------------------------------------------------------------------------*/
VOID kalSendCompleteAndAwakeQueue(IN P_GLUE_INFO_T prGlueInfo, IN PVOID pvPacket)
{

	struct net_device *prDev = NULL;
	struct sk_buff *prSkb = NULL;
	UINT_16 u2QueueIdx = 0;
	UINT_8 ucNetworkType = 0;
	BOOLEAN fgIsValidDevice = TRUE;

	ASSERT(pvPacket);
	ASSERT(prGlueInfo->i4TxPendingFrameNum);

	prSkb = (struct sk_buff *)pvPacket;
	u2QueueIdx = skb_get_queue_mapping(prSkb);
	ASSERT(u2QueueIdx < CFG_MAX_TXQ_NUM);

	if (GLUE_GET_PKT_IS_P2P(prSkb)) {
		ucNetworkType = NETWORK_TYPE_P2P_INDEX;
#if CFG_ENABLE_WIFI_DIRECT
		/* in case packet was sent after P2P device is unregistered */
		if (prGlueInfo->prAdapter->fgIsP2PRegistered == FALSE)
			fgIsValidDevice = FALSE;
#endif
	} else if (GLUE_GET_PKT_IS_PAL(prSkb)) {
		ucNetworkType = NETWORK_TYPE_BOW_INDEX;
	} else {
		ucNetworkType = NETWORK_TYPE_AIS_INDEX;
	}

	GLUE_DEC_REF_CNT(prGlueInfo->i4TxPendingFrameNum);
	if (u2QueueIdx < CFG_MAX_TXQ_NUM)
		GLUE_DEC_REF_CNT(prGlueInfo->ai4TxPendingFrameNumPerQueue[ucNetworkType][u2QueueIdx]);
	prDev = prSkb->dev;

	ASSERT(prDev);

	if ((fgIsValidDevice == TRUE) && (u2QueueIdx < CFG_MAX_TXQ_NUM)) {
		if (netif_subqueue_stopped(prDev, prSkb) &&
		    prGlueInfo->ai4TxPendingFrameNumPerQueue[ucNetworkType][u2QueueIdx] <=
		    CFG_TX_START_NETIF_PER_QUEUE_THRESHOLD) {
			DBGLOG(TX, INFO, "netif_wake_subqueue for bss: %d. Queue len: %d\n",
				ucNetworkType,
				prGlueInfo->ai4TxPendingFrameNumPerQueue[ucNetworkType][u2QueueIdx]);
			netif_wake_subqueue(prDev, u2QueueIdx);

#if (CONF_HIF_LOOPBACK_AUTO == 1)
			prGlueInfo->rHifInfo.HifLoopbkFlg &= ~0x01;
#endif /* CONF_HIF_LOOPBACK_AUTO */
		}
	}

	dev_kfree_skb((struct sk_buff *)pvPacket);
	prGlueInfo->u8SkbFreed++;

	DBGLOG(TX, EVENT, "----- pending frame %d -----\n", prGlueInfo->i4TxPendingFrameNum);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Copy Mac Address setting from registry. It's All Zeros in Linux.
 *
 * \param[in] prAdapter Pointer to the Adapter structure
 *
 * \param[out] paucMacAddr Pointer to the Mac Address buffer
 *
 * \retval WLAN_STATUS_SUCCESS
 *
 * \note
 */
/*----------------------------------------------------------------------------*/
VOID kalQueryRegistryMacAddr(IN P_GLUE_INFO_T prGlueInfo, OUT PUINT_8 paucMacAddr)
{
	UINT_8 aucZeroMac[MAC_ADDR_LEN] = { 0, 0, 0, 0, 0, 0 }

	DEBUGFUNC("kalQueryRegistryMacAddr");

	ASSERT(prGlueInfo);
	ASSERT(paucMacAddr);

	kalMemCopy((PVOID) paucMacAddr, (PVOID) aucZeroMac, MAC_ADDR_LEN);

}				/* end of kalQueryRegistryMacAddr() */

#if CFG_SUPPORT_EXT_CONFIG
/*----------------------------------------------------------------------------*/
/*!
 * \brief Read external configuration, ex. NVRAM or file
 *
 * \param[in] prGlueInfo     Pointer of GLUE Data Structure
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
UINT_32 kalReadExtCfg(IN P_GLUE_INFO_T prGlueInfo)
{
	ASSERT(prGlueInfo);

	/*
	 * External data is given from user space by ioctl or /proc, not read by
	 * driver.
	 */
	if (prGlueInfo->u4ExtCfgLength != 0)
		DBGLOG(INIT, TRACE, "Read external configuration data -- OK\n");
	else
		DBGLOG(INIT, TRACE, "Read external configuration data -- fail\n");

	return prGlueInfo->u4ExtCfgLength;
}
#endif
/*----------------------------------------------------------------------------*/
/*!
 * @brief This inline function is to extract some packet information, including
 *        user priority, packet length, destination address, 802.1x and BT over Wi-Fi
 *        or not.
 *
 * @param prGlueInfo         Pointer to the glue structure
 * @param prNdisPacket       Packet descriptor
 * @param pucPriorityParam   User priority
 * @param pu4PacketLen       Packet length
 * @param pucEthDestAddr Destination address
 * @param pfgIs1X            802.1x packet or not
 * @param pfgIsPAL           BT over Wi-Fi packet or not
 * @prGenUse		    General used param
 *
 * @retval TRUE      Success to extract information
 * @retval FALSE     Fail to extract correct information
 */
/*----------------------------------------------------------------------------*/

BOOLEAN
kalQoSFrameClassifierAndPacketInfo(IN P_GLUE_INFO_T prGlueInfo,
				   IN P_NATIVE_PACKET prPacket,
				   OUT PUINT_8 pucPriorityParam,
				   OUT PUINT_32 pu4PacketLen,
				   OUT PUINT_8 pucEthDestAddr,
				   OUT PBOOLEAN pfgIs1X,
				   OUT PBOOLEAN pfgIsPAL, OUT PUINT_8 pucNetworkType,
				   OUT PVOID prGenUse)
{

	UINT_32 u4PacketLen;

	UINT_8 ucUserPriority = USER_PRIORITY_DEFAULT;	/* Default */
	UINT_8 ucEtherType;
	UINT_16 u2EtherTypeLen;
	struct sk_buff *prSkb = (struct sk_buff *)prPacket;
	PUINT_8 aucLookAheadBuf = NULL;

	DEBUGFUNC("kalQoSFrameClassifierAndPacketInfo");

	u4PacketLen = prSkb->len;

	if (u4PacketLen < ETH_HLEN) {
		DBGLOG(TX, WARN, "Invalid Ether packet length: %u\n", (UINT_32) u4PacketLen);
		return FALSE;
	}

	aucLookAheadBuf = prSkb->data;

	*pfgIs1X = FALSE;
	*pfgIsPAL = FALSE;

	/* 4 <3> Obtain the User Priority for WMM */
	ucEtherType = kalGetPktEtherType(aucLookAheadBuf);
	u2EtherTypeLen = (aucLookAheadBuf[ETH_TYPE_LEN_OFFSET] << 8) | (aucLookAheadBuf[ETH_TYPE_LEN_OFFSET + 1]);

	/* <4> Network type */
#if CFG_ENABLE_BT_OVER_WIFI
	if (*pfgIsPAL == TRUE) {
		*pucNetworkType = NETWORK_TYPE_BOW_INDEX;
	} else
#endif
	{
#if CFG_ENABLE_WIFI_DIRECT
		if (prGlueInfo->prAdapter->fgIsP2PRegistered && GLUE_GET_PKT_IS_P2P(prPacket)) {
			*pucNetworkType = NETWORK_TYPE_P2P_INDEX;
		} else
#endif
		{
			*pucNetworkType = NETWORK_TYPE_AIS_INDEX;
		}
	}

	if ((u2EtherTypeLen == ETH_P_IP) && (u4PacketLen >= LOOK_AHEAD_LEN)) {
		PUINT_8 pucIpHdr = &aucLookAheadBuf[ETH_HLEN];
		UINT_8 ucIpVersion;
		PUINT_8 pucIpPayload = pucIpHdr + 20;

		ucIpVersion = (pucIpHdr[0] & IPVH_VERSION_MASK) >> IPVH_VERSION_OFFSET;
		if (ucIpVersion == IPVERSION) {
			UINT_8 ucIpTos;

			ucIpTos = pucIpHdr[1];
			/* Get the DSCP value from the header of IP packet. */
			ucUserPriority = getUpFromDscp(prGlueInfo, *pucNetworkType, ucIpTos & 0x3F);



#if (1 || defined(PPR2_TEST))
		DBGLOG(TX, TRACE, "setUP ucIpTos: %d, ucUP: %d\n", ucIpTos, ucUserPriority);
		if (pucIpHdr[9] == IP_PRO_ICMP && pucIpPayload[0] == 0x08) {
			DBGLOG(TX, INFO, "PING ipid: %d ucIpTos: %d, ucUP: %d\n",
				(pucIpHdr[5] << 8 | pucIpHdr[4]),
				ucIpTos, ucUserPriority);
		}
#endif
		if (ucUserPriority == 0xFF)
			ucUserPriority = ((ucIpTos & IPTOS_PREC_MASK) >> IPTOS_PREC_OFFSET);


		}

		/* TODO(Kevin): Add TSPEC classifier here */
	}  else if (u2EtherTypeLen == ETH_P_1X || u2EtherTypeLen == ETH_P_PRE_1X) {	/* For Port Control */
		PUINT_8 pucEapol = &aucLookAheadBuf[ETH_HLEN];
		UINT_8 ucEapolType = pucEapol[1];
		UINT_16 u2KeyInfo = pucEapol[5]<<8 | pucEapol[6];
		/*
		 * generate a seq number used to trace security frame TX
		 */
		if (prGenUse)
			*(UINT_8 *)prGenUse = nicIncreaseCmdSeqNum(prGlueInfo->prAdapter);

		switch (ucEapolType) {
		case 0: /* eap packet */
			DBGLOG(TX, INFO, "<TX> EAP Packet: code %d, id %d, type %d, seqNo %d\n",
					pucEapol[4], pucEapol[5], pucEapol[7],
					prGenUse ? *(UINT_8 *)prGenUse : 0);
			break;
		case 1: /* eapol start */
			DBGLOG(TX, INFO, "<TX> EAPOL: start, seqNo %d\n",
					prGenUse ? *(UINT_8 *)prGenUse : 0);
			break;
		case 3: /* key */
			DBGLOG(TX, INFO,
				"<TX> EAPOL: key, KeyInfo 0x%04x, Nonce %02x%02x%02x%02x%02x%02x%02x%02x... seqNo %d\n",
				u2KeyInfo, pucEapol[17], pucEapol[18], pucEapol[19], pucEapol[20],
				pucEapol[21], pucEapol[22], pucEapol[23], pucEapol[24],
				prGenUse ? *(UINT_8 *)prGenUse : 0);
			break;
		}
		*pfgIs1X = TRUE;
	}
#if CFG_SUPPORT_WAPI
	else if (u2EtherTypeLen == ETH_WPI_1X) {
		PUINT_8 pucEthBody = &aucLookAheadBuf[ETH_HLEN];
		UINT_8 ucSubType = pucEthBody[3]; /* sub type filed*/
		UINT_16 u2Length = *(PUINT_16)&pucEthBody[6];
		UINT_16 u2Seq = *(PUINT_16)&pucEthBody[8];

		DBGLOG(TX, INFO, "<TX> WAPI: subType %d, Len %d, Seq %d\n",
				ucSubType, u2Length, u2Seq);
		*pfgIs1X = TRUE;
	}
#endif
#if (CFG_SUPPORT_TDLS == 1)
	else if (u2EtherTypeLen == TDLS_FRM_PROT_TYPE) {
		/* TDLS case */
		TDLSEX_UP_ASSIGN(ucUserPriority);
	}
#endif /* CFG_SUPPORT_TDLS */
	else if (u2EtherTypeLen <= 1500) {	/* 802.3 Frame */
		UINT_8 ucDSAP, ucSSAP, ucControl;
		UINT_8 aucOUI[3];

		ucDSAP = *(PUINT_8) &aucLookAheadBuf[ETH_LLC_OFFSET];
		ucSSAP = *(PUINT_8) &aucLookAheadBuf[ETH_LLC_OFFSET + 1];
		ucControl = *(PUINT_8) &aucLookAheadBuf[ETH_LLC_OFFSET + 2];

		aucOUI[0] = *(PUINT_8) &aucLookAheadBuf[ETH_SNAP_OFFSET];
		aucOUI[1] = *(PUINT_8) &aucLookAheadBuf[ETH_SNAP_OFFSET + 1];
		aucOUI[2] = *(PUINT_8) &aucLookAheadBuf[ETH_SNAP_OFFSET + 2];

		if (ucDSAP == ETH_LLC_DSAP_SNAP &&
		    ucSSAP == ETH_LLC_SSAP_SNAP &&
		    ucControl == ETH_LLC_CONTROL_UNNUMBERED_INFORMATION &&
		    aucOUI[0] == ETH_SNAP_BT_SIG_OUI_0 &&
		    aucOUI[1] == ETH_SNAP_BT_SIG_OUI_1 && aucOUI[2] == ETH_SNAP_BT_SIG_OUI_2) {

			UINT_16 tmp =
			    ((aucLookAheadBuf[ETH_SNAP_OFFSET + 3] << 8) | aucLookAheadBuf[ETH_SNAP_OFFSET + 4]);

			*pfgIsPAL = TRUE;
			ucUserPriority = (UINT_8) prSkb->priority;

			if (tmp == BOW_PROTOCOL_ID_SECURITY_FRAME) {
				PUINT_8 pucEapol = &aucLookAheadBuf[ETH_SNAP_OFFSET + 5];
				UINT_8 ucEapolType = pucEapol[1];
				UINT_16 u2KeyInfo = pucEapol[5]<<8 | pucEapol[6];

				if (prGenUse)
					*(UINT_8 *)prGenUse = nicIncreaseCmdSeqNum(prGlueInfo->prAdapter);

				switch (ucEapolType) {
				case 0: /* eap packet */
					DBGLOG(TX, INFO, "<TX> EAP Packet: code %d, id %d, type %d, seqNo %d\n",
							pucEapol[4], pucEapol[5], pucEapol[7],
							prGenUse ? *(UINT_8 *)prGenUse : 0);
					break;
				case 1: /* eapol start */
					DBGLOG(TX, INFO, "<TX> EAPOL: start, seqNo %d\n",
							prGenUse ? *(UINT_8 *)prGenUse : 0);
					break;
				case 3: /* key */
					DBGLOG(TX, INFO,
						"<TX> EAPOL: key, KeyInfo 0x%04x, Nonce %02x%02x%02x%02x%02x%02x%02x%02x seqNo %d\n",
						u2KeyInfo, pucEapol[17], pucEapol[18], pucEapol[19], pucEapol[20],
						pucEapol[21], pucEapol[22], pucEapol[23], pucEapol[24],
						prGenUse ? *(UINT_8 *)prGenUse : 0);
					break;
				}
				*pfgIs1X = TRUE;
			}
		}
	}
	/* 4 <5> Check ethernet type and return the value of Priority Parameter. */
	if (ucEtherType == ENUM_PKT_DHCP || ucEtherType == ENUM_PKT_ARP) {
		ucUserPriority = 6; /* use VO priority */
		DBGLOG(TX, TRACE, "change UserPriority = %d\n", ucUserPriority);
	}
	*pucPriorityParam = ucUserPriority;

	/* 4 <6> Retrieve Packet Information - DA */
	/* Packet Length/ Destination Address */
	*pu4PacketLen = u4PacketLen;

	kalMemCopy(pucEthDestAddr, aucLookAheadBuf, PARAM_MAC_ADDR_LEN);

	return TRUE;
}				/* end of kalQoSFrameClassifier() */

VOID
kalOidComplete(IN P_GLUE_INFO_T prGlueInfo,
	       IN BOOLEAN fgSetQuery, IN UINT_32 u4SetQueryInfoLen, IN WLAN_STATUS rOidStatus)
{

	ASSERT(prGlueInfo);
	/* remove timeout check timer */
	wlanoidClearTimeoutCheck(prGlueInfo->prAdapter);

	/* if (prGlueInfo->u4TimeoutFlag != 1) { */
	prGlueInfo->rPendStatus = rOidStatus;
	DBGLOG(OID, TEMP, "kalOidComplete, caller: %p\n", __builtin_return_address(0));

	/* complete ONLY if there are waiters */
	if (!completion_done(&prGlueInfo->rPendComp))
		complete(&prGlueInfo->rPendComp);
	else
		DBGLOG(INIT, WARN, "SKIP multiple OID complete!\n");

	prGlueInfo->u4OidCompleteFlag = 1;
	/* } */
	/* else let it timeout on kalIoctl entry */
}

VOID kalOidClearance(IN P_GLUE_INFO_T prGlueInfo)
{
	/* if (prGlueInfo->u4TimeoutFlag != 1) { */
	/* clear_bit(GLUE_FLAG_OID_BIT, &prGlueInfo->u4Flag); */
	if (prGlueInfo->u4OidCompleteFlag != 1) {
		DBGLOG(OID, TEMP, "kalOidClearance, caller: %p\n", __builtin_return_address(0));
		/* complete ONLY if there are waiters */
		if (!completion_done(&prGlueInfo->rPendComp))
			complete(&prGlueInfo->rPendComp);
		else
			DBGLOG(INIT, WARN, "SKIP multiple OID complete!\n");

	}
	/* } */
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is used to transfer linux ioctl to OID, and  we
 * need to specify the behavior of the OID by ourself
 *
 * @param prGlueInfo         Pointer to the glue structure
 * @param pvInfoBuf          Data buffer
 * @param u4InfoBufLen       Data buffer length
 * @param fgRead             Is this a read OID
 * @param fgWaitResp         does this OID need to wait for values
 * @param fgCmd              does this OID compose command packet
 * @param pu4QryInfoLen      The data length of the return values
 *
 * @retval TRUE      Success to extract information
 * @retval FALSE     Fail to extract correct information
 */
/*----------------------------------------------------------------------------*/

/* todo: enqueue the i/o requests for multiple processes access */
/*  */
/* currently, return -1 */
/*  */

/* static GL_IO_REQ_T OidEntry; */

WLAN_STATUS
kalIoctl(IN P_GLUE_INFO_T prGlueInfo,
	 IN PFN_OID_HANDLER_FUNC pfnOidHandler,
	 IN PVOID pvInfoBuf,
	 IN UINT_32 u4InfoBufLen,
	 IN BOOLEAN fgRead, IN BOOLEAN fgWaitResp, IN BOOLEAN fgCmd, IN BOOLEAN fgIsP2pOid, OUT PUINT_32 pu4QryInfoLen)
{
	P_GL_IO_REQ_T prIoReq = NULL;
	WLAN_STATUS ret = WLAN_STATUS_SUCCESS;

	if (fgIsResetting == TRUE)
		return WLAN_STATUS_SUCCESS;

	/* GLUE_SPIN_LOCK_DECLARATION(); */
	ASSERT(prGlueInfo);

	/* <1> Check if driver is halt */
	/* if (prGlueInfo->u4Flag & GLUE_FLAG_HALT) { */
	/* return WLAN_STATUS_ADAPTER_NOT_READY; */
	/* } */

	/*
	 * if wait longer than double OID timeout timer, then will show backtrace who held halt lock.
	 * at this case, we will return kalIoctl failure because tx_thread may be hung
	 */
	if (kalHaltLock(2 * WLAN_OID_TIMEOUT_THRESHOLD))
		return WLAN_STATUS_FAILURE;

	if (kalIsHalted()) {
		kalHaltUnlock();
		return WLAN_STATUS_ADAPTER_NOT_READY;
	}

	if (down_interruptible(&prGlueInfo->ioctl_sem)) {
		kalHaltUnlock();
		return WLAN_STATUS_FAILURE;
	}

	/* <2> TODO: thread-safe */

	/* <3> point to the OidEntry of Glue layer */

	prIoReq = &(prGlueInfo->OidEntry);

	ASSERT(prIoReq);

	/* <4> Compose the I/O request */
	prIoReq->prAdapter = prGlueInfo->prAdapter;
	prIoReq->pfnOidHandler = pfnOidHandler;
	prIoReq->pvInfoBuf = pvInfoBuf;
	prIoReq->u4InfoBufLen = u4InfoBufLen;
	prIoReq->pu4QryInfoLen = pu4QryInfoLen;
	prIoReq->fgRead = fgRead;
	prIoReq->fgWaitResp = fgWaitResp;
	prIoReq->rStatus = WLAN_STATUS_FAILURE;
#if CFG_ENABLE_WIFI_DIRECT
	prIoReq->fgIsP2pOid = fgIsP2pOid;
#endif

	/* <5> Reset the status of pending OID */
	prGlueInfo->rPendStatus = WLAN_STATUS_FAILURE;
	/* prGlueInfo->u4TimeoutFlag = 0; */
	/* prGlueInfo->u4OidCompleteFlag = 0; */

	/* <6> Check if we use the command queue */
	prIoReq->u4Flag = fgCmd;

	/* <7> schedule the OID bit */
	set_bit(GLUE_FLAG_OID_BIT, &prGlueInfo->ulFlag);

	/* <8> Wake up tx thread to handle kick start the I/O request */
	wake_up_interruptible(&prGlueInfo->waitq);

	/* <9> Block and wait for event or timeout, current the timeout is 5 secs */
	/* if (wait_for_completion_interruptible_timeout(&prGlueInfo->rPendComp, 5 * KAL_HZ)) { */
	/* if (!wait_for_completion_interruptible(&prGlueInfo->rPendComp)) { */
	DBGLOG(OID, TEMP, "kalIoctl: before wait, caller: %p\n", __builtin_return_address(0));
	wait_for_completion(&prGlueInfo->rPendComp); {
		/* Case 1: No timeout. */
		/* if return WLAN_STATUS_PENDING, the status of cmd is stored in prGlueInfo  */
		if (prIoReq->rStatus == WLAN_STATUS_PENDING)
			ret = prGlueInfo->rPendStatus;
		else
			ret = prIoReq->rStatus;
	}
#if 0
	else {
		/* Case 2: timeout */
		/* clear pending OID's cmd in CMD queue */
		if (fgCmd) {
			prGlueInfo->u4TimeoutFlag = 1;
			wlanReleasePendingOid(prGlueInfo->prAdapter, 0);
		}
		ret = WLAN_STATUS_FAILURE;
	}
#endif
	DBGLOG(OID, TEMP, "kalIoctl: done\n");
	up(&prGlueInfo->ioctl_sem);
	kalHaltUnlock();

	return ret;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to clear all pending security frames
 *
 * \param prGlueInfo     Pointer of GLUE Data Structure
 *
 * \retval none
 */
/*----------------------------------------------------------------------------*/
VOID kalClearSecurityFrames(IN P_GLUE_INFO_T prGlueInfo)
{
	P_QUE_T prCmdQue;
	QUE_T rTempCmdQue;
	P_QUE_T prTempCmdQue = &rTempCmdQue;
	P_QUE_ENTRY_T prQueueEntry = (P_QUE_ENTRY_T) NULL;
	P_CMD_INFO_T prCmdInfo = (P_CMD_INFO_T) NULL;

	GLUE_SPIN_LOCK_DECLARATION();

	ASSERT(prGlueInfo);

	/* Clear pending security frames in prGlueInfo->rCmdQueue */
	prCmdQue = &prGlueInfo->rCmdQueue;

	GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);
	QUEUE_MOVE_ALL(prTempCmdQue, prCmdQue);

	QUEUE_REMOVE_HEAD(prTempCmdQue, prQueueEntry, P_QUE_ENTRY_T);
	while (prQueueEntry) {
		prCmdInfo = (P_CMD_INFO_T) prQueueEntry;

		if (prCmdInfo->eCmdType == COMMAND_TYPE_SECURITY_FRAME) {
			prCmdInfo->pfCmdTimeoutHandler(prGlueInfo->prAdapter, prCmdInfo);
			cmdBufFreeCmdInfo(prGlueInfo->prAdapter, prCmdInfo);
		} else {
			QUEUE_INSERT_TAIL(prCmdQue, prQueueEntry);
		}

		QUEUE_REMOVE_HEAD(prTempCmdQue, prQueueEntry, P_QUE_ENTRY_T);
	}

	QUEUE_CONCATENATE_QUEUES(prCmdQue, prTempCmdQue);
	GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to clear pending security frames
 *        belongs to dedicated network type
 *
 * \param prGlueInfo         Pointer of GLUE Data Structure
 * \param eNetworkTypeIdx    Network Type Index
 *
 * \retval none
 */
/*----------------------------------------------------------------------------*/
VOID kalClearSecurityFramesByNetType(IN P_GLUE_INFO_T prGlueInfo, IN ENUM_NETWORK_TYPE_INDEX_T eNetworkTypeIdx)
{
	P_QUE_T prCmdQue;
	QUE_T rTempCmdQue;
	P_QUE_T prTempCmdQue = &rTempCmdQue;
	P_QUE_ENTRY_T prQueueEntry = (P_QUE_ENTRY_T) NULL;

	P_CMD_INFO_T prCmdInfo = (P_CMD_INFO_T) NULL;

	GLUE_SPIN_LOCK_DECLARATION();

	ASSERT(prGlueInfo);

	/* Clear pending security frames in prGlueInfo->rCmdQueue */
	prCmdQue = &prGlueInfo->rCmdQueue;

	GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);
	QUEUE_MOVE_ALL(prTempCmdQue, prCmdQue);

	QUEUE_REMOVE_HEAD(prTempCmdQue, prQueueEntry, P_QUE_ENTRY_T);
	while (prQueueEntry) {
		prCmdInfo = (P_CMD_INFO_T) prQueueEntry;

		if (prCmdInfo->eCmdType == COMMAND_TYPE_SECURITY_FRAME && prCmdInfo->eNetworkType == eNetworkTypeIdx) {
			prCmdInfo->pfCmdTimeoutHandler(prGlueInfo->prAdapter, prCmdInfo);
			cmdBufFreeCmdInfo(prGlueInfo->prAdapter, prCmdInfo);
		} else {
			QUEUE_INSERT_TAIL(prCmdQue, prQueueEntry);
		}

		QUEUE_REMOVE_HEAD(prTempCmdQue, prQueueEntry, P_QUE_ENTRY_T);
	}

	QUEUE_CONCATENATE_QUEUES(prCmdQue, prTempCmdQue);
	GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to clear all pending management frames
 *
 * \param prGlueInfo     Pointer of GLUE Data Structure
 *
 * \retval none
 */
/*----------------------------------------------------------------------------*/
VOID kalClearMgmtFrames(IN P_GLUE_INFO_T prGlueInfo)
{
	P_QUE_T prCmdQue;
	QUE_T rTempCmdQue;
	P_QUE_T prTempCmdQue = &rTempCmdQue;
	P_QUE_ENTRY_T prQueueEntry = (P_QUE_ENTRY_T) NULL;
	P_CMD_INFO_T prCmdInfo = (P_CMD_INFO_T) NULL;

	GLUE_SPIN_LOCK_DECLARATION();

	ASSERT(prGlueInfo);

	/* Clear pending management frames in prGlueInfo->rCmdQueue */
	prCmdQue = &prGlueInfo->rCmdQueue;

	GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);
	QUEUE_MOVE_ALL(prTempCmdQue, prCmdQue);

	QUEUE_REMOVE_HEAD(prTempCmdQue, prQueueEntry, P_QUE_ENTRY_T);
	while (prQueueEntry) {
		prCmdInfo = (P_CMD_INFO_T) prQueueEntry;

		if (prCmdInfo->eCmdType == COMMAND_TYPE_MANAGEMENT_FRAME) {
			wlanReleaseCommand(prGlueInfo->prAdapter, prCmdInfo);
			cmdBufFreeCmdInfo(prGlueInfo->prAdapter, prCmdInfo);
		} else {
			QUEUE_INSERT_TAIL(prCmdQue, prQueueEntry);
		}

		QUEUE_REMOVE_HEAD(prTempCmdQue, prQueueEntry, P_QUE_ENTRY_T);
	}

	QUEUE_CONCATENATE_QUEUES(prCmdQue, prTempCmdQue);
	GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to clear all pending management frames
 *           belongs to dedicated network type
 * \param prGlueInfo     Pointer of GLUE Data Structure
 *
 * \retval none
 */
/*----------------------------------------------------------------------------*/
VOID kalClearMgmtFramesByNetType(IN P_GLUE_INFO_T prGlueInfo, IN ENUM_NETWORK_TYPE_INDEX_T eNetworkTypeIdx)
{
	P_QUE_T prCmdQue;
	QUE_T rTempCmdQue;
	P_QUE_T prTempCmdQue = &rTempCmdQue;
	P_QUE_ENTRY_T prQueueEntry = (P_QUE_ENTRY_T) NULL;
	P_CMD_INFO_T prCmdInfo = (P_CMD_INFO_T) NULL;

	GLUE_SPIN_LOCK_DECLARATION();

	ASSERT(prGlueInfo);

	/* Clear pending management frames in prGlueInfo->rCmdQueue */
	prCmdQue = &prGlueInfo->rCmdQueue;

	GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);
	QUEUE_MOVE_ALL(prTempCmdQue, prCmdQue);

	QUEUE_REMOVE_HEAD(prTempCmdQue, prQueueEntry, P_QUE_ENTRY_T);
	while (prQueueEntry) {
		prCmdInfo = (P_CMD_INFO_T) prQueueEntry;

		if (prCmdInfo->eCmdType == COMMAND_TYPE_MANAGEMENT_FRAME &&
			prCmdInfo->eNetworkType == eNetworkTypeIdx) {
			wlanReleaseCommand(prGlueInfo->prAdapter, prCmdInfo);
			cmdBufFreeCmdInfo(prGlueInfo->prAdapter, prCmdInfo);
		} else {
			QUEUE_INSERT_TAIL(prCmdQue, prQueueEntry);
		}

		QUEUE_REMOVE_HEAD(prTempCmdQue, prQueueEntry, P_QUE_ENTRY_T);
	}

	QUEUE_CONCATENATE_QUEUES(prCmdQue, prTempCmdQue);
	GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);
}				/* kalClearMgmtFramesByNetType */

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is a kernel thread function for handling command packets
 * Tx requests and interrupt events
 *
 * @param data       data pointer to private data of tx_thread
 *
 * @retval           If the function succeeds, the return value is 0.
 * Otherwise, an error code is returned.
 *
 */
/*----------------------------------------------------------------------------*/

int tx_thread(void *data)
{
	struct net_device *dev = data;
	P_GLUE_INFO_T prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(dev));

	P_QUE_ENTRY_T prQueueEntry = NULL;
	P_GL_IO_REQ_T prIoReq = NULL;
	P_QUE_T prTxQueue = NULL;
	P_QUE_T prCmdQue = NULL;

	int ret = 0;

	BOOLEAN fgNeedHwAccess = FALSE;

	struct sk_buff *prSkb = NULL;

	/* for spin lock acquire and release */
	GLUE_SPIN_LOCK_DECLARATION();

	prTxQueue = &prGlueInfo->rTxQueue;
	prCmdQue = &prGlueInfo->rCmdQueue;

	current->flags |= PF_NOFREEZE;

	DBGLOG(INIT, INFO, "tx_thread starts running...\n");

	while (TRUE) {

#if CFG_ENABLE_WIFI_DIRECT
		/*run p2p multicast list work. */
		if (test_and_clear_bit(GLUE_FLAG_SUB_MOD_MULTICAST_BIT, &prGlueInfo->ulFlag))
			p2pSetMulticastListWorkQueueWrapper(prGlueInfo);
#endif

		if (test_and_clear_bit(GLUE_FLAG_FRAME_FILTER_AIS_BIT, &prGlueInfo->ulFlag)) {
			P_AIS_FSM_INFO_T prAisFsmInfo = (P_AIS_FSM_INFO_T) NULL;
			/* printk("prGlueInfo->u4OsMgmtFrameFilter = %x", prGlueInfo->u4OsMgmtFrameFilter); */
			prAisFsmInfo = &(prGlueInfo->prAdapter->rWifiVar.rAisFsmInfo);
			prAisFsmInfo->u4AisPacketFilter = prGlueInfo->u4OsMgmtFrameFilter;
		}

		if (prGlueInfo->ulFlag & GLUE_FLAG_HALT) {
			KAL_WAKE_UNLOCK(prGlueInfo->prAdapter, &(prGlueInfo->prAdapter)->rTxThreadWakeLock);
			DBGLOG(INIT, INFO, "tx_thread should stop now...\n");
			break;
		}

		/*
		 * sleep on waitqueue if no events occurred. Event contain (1) GLUE_FLAG_INT
		 * (2) GLUE_FLAG_OID (3) GLUE_FLAG_TXREQ (4) GLUE_FLAG_HALT
		 *
		 */
		KAL_WAKE_UNLOCK(prGlueInfo->prAdapter, &(prGlueInfo->prAdapter)->rTxThreadWakeLock);

		ret = wait_event_interruptible(prGlueInfo->waitq, (prGlueInfo->ulFlag != 0));

		KAL_WAKE_LOCK(prGlueInfo->prAdapter, &(prGlueInfo->prAdapter)->rTxThreadWakeLock);

/* #if (CONF_HIF_LOOPBACK_AUTO == 1) */
/* if (test_and_clear_bit(GLUE_FLAG_HIF_LOOPBK_AUTO_BIT, &prGlueInfo->u4Flag)) { */
/* kalDevLoopbkAuto(prGlueInfo); */
/* } */
/* #endif */ /* CONF_HIF_LOOPBACK_AUTO */

#if CFG_DBG_GPIO_PINS
		/* TX thread Wake up */
		mtk_wcn_stp_debug_gpio_assert(IDX_TX_THREAD, DBG_TIE_LOW);
#endif
#if CFG_ENABLE_WIFI_DIRECT
		/*run p2p multicast list work. */
		if (test_and_clear_bit(GLUE_FLAG_SUB_MOD_MULTICAST_BIT, &prGlueInfo->ulFlag))
			p2pSetMulticastListWorkQueueWrapper(prGlueInfo);

		if (test_and_clear_bit(GLUE_FLAG_FRAME_FILTER_BIT, &prGlueInfo->ulFlag)) {
			p2pFuncUpdateMgmtFrameRegister(prGlueInfo->prAdapter,
						       prGlueInfo->prP2PInfo->u4OsMgmtFrameFilter);
		}
#endif
		if (test_and_clear_bit(GLUE_FLAG_FRAME_FILTER_AIS_BIT, &prGlueInfo->ulFlag)) {
			P_AIS_FSM_INFO_T prAisFsmInfo = (P_AIS_FSM_INFO_T) NULL;
			/* printk("prGlueInfo->u4OsMgmtFrameFilter = %x", prGlueInfo->u4OsMgmtFrameFilter); */
			prAisFsmInfo = &(prGlueInfo->prAdapter->rWifiVar.rAisFsmInfo);
			prAisFsmInfo->u4AisPacketFilter = prGlueInfo->u4OsMgmtFrameFilter;
		}

		if (prGlueInfo->ulFlag & GLUE_FLAG_HALT) {
			KAL_WAKE_UNLOCK(prGlueInfo->prAdapter, &(prGlueInfo->prAdapter)->rTxThreadWakeLock);
			DBGLOG(INIT, INFO, "<1>tx_thread should stop now...\n");
			break;
		}

		fgNeedHwAccess = FALSE;

		/* Handle Interrupt */
		if (test_and_clear_bit(GLUE_FLAG_INT_BIT, &prGlueInfo->ulFlag)) {
			if (fgNeedHwAccess == FALSE) {
				fgNeedHwAccess = TRUE;

				wlanAcquirePowerControl(prGlueInfo->prAdapter);
			}

			/*
			 * the Wi-Fi interrupt is already disabled in mmc thread,
			 * so we set the flag only to enable the interrupt later
			 */
			prGlueInfo->prAdapter->fgIsIntEnable = FALSE;
			/* wlanISR(prGlueInfo->prAdapter, TRUE); */

			if (prGlueInfo->ulFlag & GLUE_FLAG_HALT) {
				/* Should stop now... skip pending interrupt */
				DBGLOG(INIT, INFO, "ignore pending interrupt\n");
			} else {
				prGlueInfo->TaskIsrCnt++;
				wlanIST(prGlueInfo->prAdapter);
			}
		}

		/* transfer ioctl to OID request */
#if 0
		if (prGlueInfo->u4Flag & GLUE_FLAG_HALT) {
			DBGLOG(INIT, INFO, "<2>tx_thread should stop now...\n");
			break;
		}
#endif

		do {
			if (test_and_clear_bit(GLUE_FLAG_OID_BIT, &prGlueInfo->ulFlag)) {
				/* get current prIoReq */
				prGlueInfo->u4OidCompleteFlag = 0;

				prIoReq = &(prGlueInfo->OidEntry);
#if CFG_ENABLE_WIFI_DIRECT
				if (prGlueInfo->prAdapter->fgIsP2PRegistered == FALSE && prIoReq->fgIsP2pOid == TRUE) {
					/*
					 * if this Oid belongs to p2p and p2p module is removed
					 * do nothing,
					 */
				} else
#endif
				{
					if (prIoReq->fgRead == FALSE) {
						prIoReq->rStatus = wlanSetInformation(prIoReq->prAdapter,
										      prIoReq->pfnOidHandler,
										      prIoReq->pvInfoBuf,
										      prIoReq->u4InfoBufLen,
										      prIoReq->pu4QryInfoLen);
					} else {
						prIoReq->rStatus = wlanQueryInformation(prIoReq->prAdapter,
											prIoReq->pfnOidHandler,
											prIoReq->pvInfoBuf,
											prIoReq->u4InfoBufLen,
											prIoReq->pu4QryInfoLen);
					}

					if (prIoReq->rStatus != WLAN_STATUS_PENDING
						&& (!completion_done(&prGlueInfo->rPendComp))) {

						DBGLOG(OID, TEMP, "tx_thread, complete\n");
						complete(&prGlueInfo->rPendComp);

					} else {
						wlanoidTimeoutCheck(prGlueInfo->prAdapter, prIoReq->pfnOidHandler);
					}
				}
			}

		} while (FALSE);

		/*
		 *
		 * if TX request, clear the TXREQ flag. TXREQ set by kalSetEvent/GlueSetEvent
		 * indicates the following requests occur
		 *
		 */
#if 0
		if (prGlueInfo->u4Flag & GLUE_FLAG_HALT) {
			DBGLOG(INIT, INFO, "<3>tx_thread should stop now...\n");
			break;
		}
#endif

		if (test_and_clear_bit(GLUE_FLAG_TXREQ_BIT, &prGlueInfo->ulFlag)) {
			/* Process Mailbox Messages */
			wlanProcessMboxMessage(prGlueInfo->prAdapter);

			/* Process CMD request */
			do {
				if (prCmdQue->u4NumElem > 0) {
					if (fgNeedHwAccess == FALSE) {
						fgNeedHwAccess = TRUE;

						wlanAcquirePowerControl(prGlueInfo->prAdapter);
					}
					wlanProcessCommandQueue(prGlueInfo->prAdapter, prCmdQue);
				}
			} while (FALSE);

			/* Handle Packet Tx */
			{
				while (QUEUE_IS_NOT_EMPTY(prTxQueue)) {
					GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_TX_QUE);
					QUEUE_REMOVE_HEAD(prTxQueue, prQueueEntry, P_QUE_ENTRY_T);
					GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_TX_QUE);

					ASSERT(prQueueEntry);
					if (prQueueEntry == NULL)
						break;

					prSkb = (struct sk_buff *)GLUE_GET_PKT_DESCRIPTOR(prQueueEntry);
					ASSERT(prSkb);
					if (prSkb == NULL) {
						DBGLOG(INIT, ERROR, "prSkb == NULL!\n");
						continue;
					}
#if (CFG_SUPPORT_TDLS_DBG == 1)
					UINT8 *pkt = prSkb->data;
					UINT16 u2Identifier;

					if ((*(pkt + 12) == 0x08) && (*(pkt + 13) == 0x00)) {
						/* ip */
						u2Identifier = ((*(pkt + 18)) << 8) | (*(pkt + 19));
						DBGLOG(INIT, LOUD, "<TDLS> %d\n", u2Identifier);
					}
#endif
					if (wlanEnqueueTxPacket(prGlueInfo->prAdapter,
								(P_NATIVE_PACKET) prSkb) == WLAN_STATUS_RESOURCES) {
						/* no available entry in rFreeMsduInfoList */
						GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_TX_QUE);
						QUEUE_INSERT_HEAD(prTxQueue, prQueueEntry);
						GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_TX_QUE);

						break;
					}
				}

				if (wlanGetTxPendingFrameCount(prGlueInfo->prAdapter) > 0) {
					/* send packets to HIF here */
					wlanTxPendingPackets(prGlueInfo->prAdapter, &fgNeedHwAccess);
				}
			}

		}

		/* Process RX, In linux, we don't need to free sk_buff by ourself */

		/* In linux, we don't need to free sk_buff by ourself */

		/* In linux, we don't do reset */
		if (fgNeedHwAccess == TRUE)
			wlanReleasePowerControl(prGlueInfo->prAdapter);

		/* handle cnmTimer time out */
		if (test_and_clear_bit(GLUE_FLAG_TIMEOUT_BIT, &prGlueInfo->ulFlag))
			wlanTimerTimeoutCheck(prGlueInfo->prAdapter);
#if CFG_DBG_GPIO_PINS
		/* TX thread go to sleep */
		if (!prGlueInfo->ulFlag)
			mtk_wcn_stp_debug_gpio_assert(IDX_TX_THREAD, DBG_TIE_HIGH);
#endif
	}

#if 0
	if (fgNeedHwAccess == TRUE)
		wlanReleasePowerControl(prGlueInfo->prAdapter);
#endif

	/* exit while loop, tx thread is closed so we flush all pending packets */
	/* flush the pending TX packets */
	if (prGlueInfo->i4TxPendingFrameNum > 0)
		kalFlushPendingTxPackets(prGlueInfo);

	/* flush pending security frames */
	if (prGlueInfo->i4TxPendingSecurityFrameNum > 0)
		kalClearSecurityFrames(prGlueInfo);

	/* remove pending oid */
	wlanReleasePendingOid(prGlueInfo->prAdapter, 0);

	/* In linux, we don't need to free sk_buff by ourself */

	DBGLOG(INIT, INFO, "mtk_sdiod stops\n");
	complete(&prGlueInfo->rHaltComp);

	return 0;

}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to check if card is removed
 *
 * \param pvGlueInfo     Pointer of GLUE Data Structure
 *
 * \retval TRUE:     card is removed
 *         FALSE:    card is still attached
 */
/*----------------------------------------------------------------------------*/
BOOLEAN kalIsCardRemoved(IN P_GLUE_INFO_T prGlueInfo)
{
	ASSERT(prGlueInfo);

	return FALSE;
	/* Linux MMC doesn't have removal notification yet */
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to send command to firmware for overriding netweork address
 *
 * \param pvGlueInfo Pointer of GLUE Data Structure

 * \retval TRUE
 *         FALSE
 */
/*----------------------------------------------------------------------------*/
BOOLEAN kalRetrieveNetworkAddress(IN P_GLUE_INFO_T prGlueInfo, IN OUT PARAM_MAC_ADDRESS *prMacAddr)
{
	ASSERT(prGlueInfo);

	if (prGlueInfo->fgIsMacAddrOverride == FALSE) {
#if !defined(CONFIG_X86)
#if !CFG_TC1_FEATURE
		UINT_32 i;
#endif
		BOOLEAN fgIsReadError = FALSE;

#if !CFG_TC1_FEATURE
		for (i = 0; i < MAC_ADDR_LEN; i += 2) {
			if (kalCfgDataRead16(prGlueInfo,
					     OFFSET_OF(WIFI_CFG_PARAM_STRUCT, aucMacAddress) + i,
					     (PUINT_16) (((PUINT_8) prMacAddr) + i)) == FALSE) {
				fgIsReadError = TRUE;
				break;
			}
		}
#else
		TC1_FAC_NAME(FacReadWifiMacAddr) ((unsigned char *)prMacAddr);
#endif

		if (fgIsReadError == TRUE)
			return FALSE;
		else
			return TRUE;
#else
		/* x86 Linux doesn't need to override network address so far */
		return FALSE;
#endif
	} else {
		COPY_MAC_ADDR(prMacAddr, prGlueInfo->rMacAddrOverride);

		return TRUE;
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to flush pending TX packets in glue layer
 *
 * \param pvGlueInfo     Pointer of GLUE Data Structure
 *
 * \retval none
 */
/*----------------------------------------------------------------------------*/
VOID kalFlushPendingTxPackets(IN P_GLUE_INFO_T prGlueInfo)
{
	P_QUE_T prTxQue;
	P_QUE_ENTRY_T prQueueEntry;
	PVOID prPacket;

	GLUE_SPIN_LOCK_DECLARATION();

	ASSERT(prGlueInfo);

	prTxQue = &(prGlueInfo->rTxQueue);

	if (prGlueInfo->i4TxPendingFrameNum) {
		while (TRUE) {
			GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_TX_QUE);
			QUEUE_REMOVE_HEAD(prTxQue, prQueueEntry, P_QUE_ENTRY_T);
			GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_TX_QUE);

			if (prQueueEntry == NULL)
				break;

			prPacket = GLUE_GET_PKT_DESCRIPTOR(prQueueEntry);
			kalSendComplete(prGlueInfo, prPacket, WLAN_STATUS_NOT_ACCEPTED);
		}
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is get indicated media state
 *
 * \param pvGlueInfo     Pointer of GLUE Data Structure
 *
 * \retval
 */
/*----------------------------------------------------------------------------*/
ENUM_PARAM_MEDIA_STATE_T kalGetMediaStateIndicated(IN P_GLUE_INFO_T prGlueInfo)
{
	ASSERT(prGlueInfo);

	return prGlueInfo->eParamMediaStateIndicated;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to set indicated media state
 *
 * \param pvGlueInfo     Pointer of GLUE Data Structure
 *
 * \retval none
 */
/*----------------------------------------------------------------------------*/
VOID kalSetMediaStateIndicated(IN P_GLUE_INFO_T prGlueInfo, IN ENUM_PARAM_MEDIA_STATE_T eParamMediaStateIndicate)
{
	ASSERT(prGlueInfo);

	prGlueInfo->eParamMediaStateIndicated = eParamMediaStateIndicate;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to clear pending OID staying in command queue
 *
 * \param prGlueInfo     Pointer of GLUE Data Structure
 *
 * \retval none
 */
/*----------------------------------------------------------------------------*/
VOID kalOidCmdClearance(IN P_GLUE_INFO_T prGlueInfo)
{
	P_QUE_T prCmdQue;
	QUE_T rTempCmdQue;
	P_QUE_T prTempCmdQue = &rTempCmdQue;
	P_QUE_ENTRY_T prQueueEntry = (P_QUE_ENTRY_T) NULL;
	P_CMD_INFO_T prCmdInfo = (P_CMD_INFO_T) NULL;

	GLUE_SPIN_LOCK_DECLARATION();

	ASSERT(prGlueInfo);

	prCmdQue = &prGlueInfo->rCmdQueue;

	GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);
	QUEUE_MOVE_ALL(prTempCmdQue, prCmdQue);

	QUEUE_REMOVE_HEAD(prTempCmdQue, prQueueEntry, P_QUE_ENTRY_T);
	while (prQueueEntry) {

		if (((P_CMD_INFO_T) prQueueEntry)->fgIsOid) {
			prCmdInfo = (P_CMD_INFO_T) prQueueEntry;
			break;
		}
		QUEUE_INSERT_TAIL(prCmdQue, prQueueEntry);

		QUEUE_REMOVE_HEAD(prTempCmdQue, prQueueEntry, P_QUE_ENTRY_T);
	}

	QUEUE_CONCATENATE_QUEUES(prCmdQue, prTempCmdQue);
	GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);

	if (prCmdInfo) {
		if (prCmdInfo->pfCmdTimeoutHandler)
			prCmdInfo->pfCmdTimeoutHandler(prGlueInfo->prAdapter, prCmdInfo);
		else
			kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, 0, WLAN_STATUS_NOT_ACCEPTED);

		prGlueInfo->u4OidCompleteFlag = 1;
		cmdBufFreeCmdInfo(prGlueInfo->prAdapter, prCmdInfo);
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to insert command into prCmdQueue
 *
 * \param prGlueInfo     Pointer of GLUE Data Structure
 *        prQueueEntry   Pointer of queue entry to be inserted
 *
 * \retval none
 */
/*----------------------------------------------------------------------------*/
VOID kalEnqueueCommand(IN P_GLUE_INFO_T prGlueInfo, IN P_QUE_ENTRY_T prQueueEntry)
{
	P_QUE_T prCmdQue;
	P_CMD_INFO_T prCmdInfo;
	P_MSDU_INFO_T prMsduInfo;

	GLUE_SPIN_LOCK_DECLARATION();

	ASSERT(prGlueInfo);
	ASSERT(prQueueEntry);

	prCmdQue = &prGlueInfo->rCmdQueue;

	prCmdInfo = (P_CMD_INFO_T) prQueueEntry;
	if (prCmdInfo->prPacket && prCmdInfo->eCmdType == COMMAND_TYPE_MANAGEMENT_FRAME) {
		prMsduInfo = (P_MSDU_INFO_T) (prCmdInfo->prPacket);
		prMsduInfo->eCmdType = prCmdInfo->eCmdType;
		prMsduInfo->ucCID = prCmdInfo->ucCID;
		prMsduInfo->u4InqueTime = kalGetTimeTick();
	}

	GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);
	QUEUE_INSERT_TAIL(prCmdQue, prQueueEntry);
	GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Handle EVENT_ID_ASSOC_INFO event packet by indicating to OS with
 *        proper information
 *
 * @param pvGlueInfo     Pointer of GLUE Data Structure
 * @param prAssocInfo    Pointer of EVENT_ID_ASSOC_INFO Packet
 *
 * @return none
 */
/*----------------------------------------------------------------------------*/
VOID kalHandleAssocInfo(IN P_GLUE_INFO_T prGlueInfo, IN P_EVENT_ASSOC_INFO prAssocInfo)
{
	/* to do */
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to get firmware load address from registry
 *
 * \param prGlueInfo     Pointer of GLUE Data Structure
 *
 * \retval
 */
/*----------------------------------------------------------------------------*/
UINT_32 kalGetFwLoadAddress(IN P_GLUE_INFO_T prGlueInfo)
{
	ASSERT(prGlueInfo);

	return prGlueInfo->rRegInfo.u4LoadAddress;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to get firmware start address from registry
 *
 * \param prGlueInfo     Pointer of GLUE Data Structure
 *
 * \retval
 */
/*----------------------------------------------------------------------------*/
UINT_32 kalGetFwStartAddress(IN P_GLUE_INFO_T prGlueInfo)
{
	ASSERT(prGlueInfo);

	return prGlueInfo->rRegInfo.u4StartAddress;
}

/*----------------------------------------------------------------------------*/
/*!
  * @brief Notify OS with SendComplete event of the specific packet. Linux should
  *        free packets here.
  *
  * @param pvGlueInfo     Pointer of GLUE Data Structure
  * @param pvPacket       Pointer of Packet Handle
  * @param status         Status Code for OS upper layer
  *
  * @return none
  */
/*----------------------------------------------------------------------------*/

/* / Todo */
VOID kalSecurityFrameSendComplete(IN P_GLUE_INFO_T prGlueInfo, IN PVOID pvPacket, IN WLAN_STATUS rStatus)
{
	ASSERT(pvPacket);

	dev_kfree_skb((struct sk_buff *)pvPacket);
	if (prGlueInfo) {
		prGlueInfo->u8SkbFreed++;
		GLUE_DEC_REF_CNT(prGlueInfo->i4TxPendingSecurityFrameNum);
	}
}

UINT_32 kalGetTxPendingFrameCount(IN P_GLUE_INFO_T prGlueInfo)
{
	ASSERT(prGlueInfo);

	return (UINT_32) (prGlueInfo->i4TxPendingFrameNum);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to retrieve the number of pending commands
 *        (including MMPDU, 802.1X and command packets)
 *
 * \param prGlueInfo     Pointer of GLUE Data Structure
 *
 * \retval
 */
/*----------------------------------------------------------------------------*/
UINT_32 kalGetTxPendingCmdCount(IN P_GLUE_INFO_T prGlueInfo)
{
	P_QUE_T prCmdQue;

	ASSERT(prGlueInfo);
	prCmdQue = &prGlueInfo->rCmdQueue;

	return prCmdQue->u4NumElem;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Timer Initialization Procedure
 *
 * \param[in] prGlueInfo     Pointer to GLUE Data Structure
 * \param[in] prTimerHandler Pointer to timer handling function, whose only
 *                           argument is "prAdapter"
 *
 * \retval none
 *
 */
/*----------------------------------------------------------------------------*/

/* static struct timer_list tickfn; */

VOID kalOsTimerInitialize(IN P_GLUE_INFO_T prGlueInfo, IN PVOID prTimerHandler)
{

	ASSERT(prGlueInfo);

	init_timer(&(prGlueInfo->tickfn));
	prGlueInfo->tickfn.function = prTimerHandler;
	prGlueInfo->tickfn.data = (ULONG) prGlueInfo;
}

/* Todo */
/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to set the time to do the time out check.
 *
 * \param[in] prGlueInfo Pointer to GLUE Data Structure
 * \param[in] rInterval  Time out interval from current time.
 *
 * \retval TRUE Success.
 */
/*----------------------------------------------------------------------------*/
BOOLEAN kalSetTimer(IN P_GLUE_INFO_T prGlueInfo, IN UINT_32 u4Interval)
{
	ASSERT(prGlueInfo);
	del_timer_sync(&(prGlueInfo->tickfn));

	prGlueInfo->tickfn.expires = jiffies + u4Interval * HZ / MSEC_PER_SEC;
	add_timer(&(prGlueInfo->tickfn));

	return TRUE;		/* success */
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to cancel
 *
 * \param[in] prGlueInfo Pointer to GLUE Data Structure
 *
 * \retval TRUE  :   Timer has been canceled
 *         FALAE :   Timer doens't exist
 */
/*----------------------------------------------------------------------------*/
BOOLEAN kalCancelTimer(IN P_GLUE_INFO_T prGlueInfo)
{
	ASSERT(prGlueInfo);

	clear_bit(GLUE_FLAG_TIMEOUT_BIT, &prGlueInfo->ulFlag);

	if (del_timer_sync(&(prGlueInfo->tickfn)) >= 0)
		return TRUE;
	else
		return FALSE;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is a callback function for scanning done
 *
 * \param[in] prGlueInfo Pointer to GLUE Data Structure
 *
 * \retval none
 *
 */
/*----------------------------------------------------------------------------*/
VOID kalScanDone(IN P_GLUE_INFO_T prGlueInfo, IN ENUM_KAL_NETWORK_TYPE_INDEX_T eNetTypeIdx, IN WLAN_STATUS status)
{
	P_AIS_FSM_INFO_T prAisFsmInfo;

	ASSERT(prGlueInfo);

	prAisFsmInfo = &(prGlueInfo->prAdapter->rWifiVar.rAisFsmInfo);
	/* report all queued beacon/probe response frames  to upper layer */
	scanReportBss2Cfg80211(prGlueInfo->prAdapter, BSS_TYPE_INFRASTRUCTURE, NULL);
	cnmTimerStopTimer(prGlueInfo->prAdapter, &prAisFsmInfo->rScanDoneTimer);

	/* check for system configuration for generating error message on scan list */
	wlanCheckSystemConfiguration(prGlueInfo->prAdapter);

	kalIndicateStatusAndComplete(prGlueInfo, WLAN_STATUS_SCAN_COMPLETE, NULL, 0);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to generate a random number
 *
 * \param none
 *
 * \retval UINT_32
 */
/*----------------------------------------------------------------------------*/
UINT_32 kalRandomNumber(VOID)
{
	UINT_32 number = 0;

	get_random_bytes(&number, 4);

	return number;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief command timeout call-back function
 *
 * \param[in] prGlueInfo Pointer to the GLUE data structure.
 *
 * \retval (none)
 */
/*----------------------------------------------------------------------------*/
VOID kalTimeoutHandler(ULONG arg)
{

	P_GLUE_INFO_T prGlueInfo = (P_GLUE_INFO_T) arg;

	ASSERT(prGlueInfo);

	/* Notify tx thread  for timeout event */
	set_bit(GLUE_FLAG_TIMEOUT_BIT, &prGlueInfo->ulFlag);
	wake_up_interruptible(&prGlueInfo->waitq);

}

VOID kalSetEvent(P_GLUE_INFO_T pr)
{
	set_bit(GLUE_FLAG_TXREQ_BIT, &pr->ulFlag);
	wake_up_interruptible(&pr->waitq);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief to check if configuration file (NVRAM/Registry) exists
 *
 * \param[in]
 *           prGlueInfo
 *
 * \return
 *           TRUE
 *           FALSE
 */
/*----------------------------------------------------------------------------*/
BOOLEAN kalIsConfigurationExist(IN P_GLUE_INFO_T prGlueInfo)
{
#if !defined(CONFIG_X86)
	ASSERT(prGlueInfo);

	return prGlueInfo->fgNvramAvailable;
#else
	/* there is no configuration data for x86-linux */
	return FALSE;
#endif
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief to retrieve Registry information
 *
 * \param[in]
 *           prGlueInfo
 *
 * \return
 *           Pointer of REG_INFO_T
 */
/*----------------------------------------------------------------------------*/
P_REG_INFO_T kalGetConfiguration(IN P_GLUE_INFO_T prGlueInfo)
{
	ASSERT(prGlueInfo);

	return &(prGlueInfo->rRegInfo);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief to retrieve version information of corresponding configuration file
 *
 * \param[in]
 *           prGlueInfo
 *
 * \param[out]
 *           pu2Part1CfgOwnVersion
 *           pu2Part1CfgPeerVersion
 *           pu2Part2CfgOwnVersion
 *           pu2Part2CfgPeerVersion
 *
 * \return
 *           NONE
 */
/*----------------------------------------------------------------------------*/
VOID
kalGetConfigurationVersion(IN P_GLUE_INFO_T prGlueInfo,
			   OUT PUINT_16 pu2Part1CfgOwnVersion,
			   OUT PUINT_16 pu2Part1CfgPeerVersion,
			   OUT PUINT_16 pu2Part2CfgOwnVersion, OUT PUINT_16 pu2Part2CfgPeerVersion)
{
	ASSERT(prGlueInfo);

	ASSERT(pu2Part1CfgOwnVersion);
	ASSERT(pu2Part1CfgPeerVersion);
	ASSERT(pu2Part2CfgOwnVersion);
	ASSERT(pu2Part2CfgPeerVersion);

	kalCfgDataRead16(prGlueInfo, OFFSET_OF(WIFI_CFG_PARAM_STRUCT, u2Part1OwnVersion), pu2Part1CfgOwnVersion);

	kalCfgDataRead16(prGlueInfo, OFFSET_OF(WIFI_CFG_PARAM_STRUCT, u2Part1PeerVersion), pu2Part1CfgPeerVersion);

	kalCfgDataRead16(prGlueInfo, OFFSET_OF(WIFI_CFG_PARAM_STRUCT, u2Part2OwnVersion), pu2Part2CfgOwnVersion);

	kalCfgDataRead16(prGlueInfo, OFFSET_OF(WIFI_CFG_PARAM_STRUCT, u2Part2PeerVersion), pu2Part2CfgPeerVersion);

}

/*----------------------------------------------------------------------------*/
/*!
 * \brief to check if the WPS is active or not
 *
 * \param[in]
 *           prGlueInfo
 *
 * \return
 *           TRUE
 *           FALSE
 */
/*----------------------------------------------------------------------------*/
BOOLEAN kalWSCGetActiveState(IN P_GLUE_INFO_T prGlueInfo)
{
	ASSERT(prGlueInfo);

	return prGlueInfo->fgWpsActive;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief update RSSI and LinkQuality to GLUE layer
 *
 * \param[in]
 *           prGlueInfo
 *           eNetTypeIdx
 *           cRssi
 *           cLinkQuality
 *
 * \return
 *           None
 */
/*----------------------------------------------------------------------------*/
VOID
kalUpdateRSSI(IN P_GLUE_INFO_T prGlueInfo,
	      IN ENUM_KAL_NETWORK_TYPE_INDEX_T eNetTypeIdx, IN INT_8 cRssi, IN INT_8 cLinkQuality)
{
	struct iw_statistics *pStats = (struct iw_statistics *)NULL;

	ASSERT(prGlueInfo);

	switch (eNetTypeIdx) {
	case KAL_NETWORK_TYPE_AIS_INDEX:
		pStats = (struct iw_statistics *)(&(prGlueInfo->rIwStats));
		break;
#if CFG_ENABLE_WIFI_DIRECT
#if CFG_SUPPORT_P2P_RSSI_QUERY
	case KAL_NETWORK_TYPE_P2P_INDEX:
		pStats = (struct iw_statistics *)(&(prGlueInfo->rP2pIwStats));
		break;
#endif
#endif
	default:
		break;

	}

	if (pStats) {
		pStats->qual.qual = cLinkQuality;
		pStats->qual.noise = 0;
		pStats->qual.updated = IW_QUAL_QUAL_UPDATED | IW_QUAL_NOISE_UPDATED | IW_QUAL_DBM;
		pStats->qual.level = 0x100 + cRssi;
		pStats->qual.updated |= IW_QUAL_LEVEL_UPDATED;
	}

}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Pre-allocate I/O buffer
 *
 * \param[in]
 *           none
 *
 * \return
 *           TRUE
 *           FALSE
 */
/*----------------------------------------------------------------------------*/
BOOLEAN kalInitIOBuffer(VOID)
{
	UINT_32 u4Size;

	if (CFG_COALESCING_BUFFER_SIZE >= CFG_RX_COALESCING_BUFFER_SIZE)
		u4Size = CFG_COALESCING_BUFFER_SIZE + sizeof(ENHANCE_MODE_DATA_STRUCT_T);
	else
		u4Size = CFG_RX_COALESCING_BUFFER_SIZE + sizeof(ENHANCE_MODE_DATA_STRUCT_T);

#ifdef MTK_DMA_BUF_MEMCPY_SUP
	pvDmaBuffer = dma_alloc_coherent(NULL, CFG_RX_MAX_PKT_SIZE, &pvDmaPhyBuf, GFP_KERNEL);
	if (pvDmaBuffer == NULL)
		return FALSE;
#endif /* MTK_DMA_BUF_MEMCPY_SUP */

	pvIoBuffer = kmalloc(u4Size, GFP_KERNEL);
	/* pvIoBuffer = dma_alloc_coherent(NULL, u4Size, &pvIoPhyBuf, GFP_KERNEL); */
	if (pvIoBuffer) {
		pvIoBufferSize = u4Size;
		pvIoBufferUsage = 0;

		return TRUE;
	}

	return FALSE;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Free pre-allocated I/O buffer
 *
 * \param[in]
 *           none
 *
 * \return
 *           none
 */
/*----------------------------------------------------------------------------*/
VOID kalUninitIOBuffer(VOID)
{
	kfree(pvIoBuffer);
#ifdef MTK_DMA_BUF_MEMCPY_SUP
	dma_free_coherent(NULL, CFG_RX_MAX_PKT_SIZE, pvDmaBuffer, pvDmaPhyBuf);
#endif /* MTK_DMA_BUF_MEMCPY_SUP */
	/* dma_free_coherent(NULL, pvIoBufferSize, pvIoBuffer, pvIoPhyBuf); */

	pvIoBuffer = (PVOID) NULL;
	pvIoBufferSize = 0;
	pvIoBufferUsage = 0;

}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Dispatch pre-allocated I/O buffer
 *
 * \param[in]
 *           u4AllocSize
 *
 * \return
 *           PVOID for pointer of pre-allocated I/O buffer
 */
/*----------------------------------------------------------------------------*/
PVOID kalAllocateIOBuffer(IN UINT_32 u4AllocSize)
{
	PVOID ret = (PVOID) NULL;

	if (pvIoBuffer) {
		if (u4AllocSize <= (pvIoBufferSize - pvIoBufferUsage)) {
			ret = (PVOID) &(((PUINT_8) (pvIoBuffer))[pvIoBufferUsage]);
			pvIoBufferUsage += u4AllocSize;
		}
	} else {
		/* fault tolerance */
		ret = (PVOID) kalMemAlloc(u4AllocSize, PHY_MEM_TYPE);
	}

	return ret;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Release all dispatched I/O buffer
 *
 * \param[in]
 *           none
 *
 * \return
 *           none
 */
/*----------------------------------------------------------------------------*/
VOID kalReleaseIOBuffer(IN PVOID pvAddr, IN UINT_32 u4Size)
{
	if (pvIoBuffer) {
		pvIoBufferUsage -= u4Size;
	} else {
		/* fault tolerance */
		kalMemFree(pvAddr, PHY_MEM_TYPE, u4Size);
	}
}
/*----------------------------------------------------------------------------*/
/*!
* \brief decode ethernet type from package head
*
* \param[in]
*           none
*
* \return
*           none
*/
/*----------------------------------------------------------------------------*/

UINT_8 kalGetPktEtherType(IN PUINT_8 pucPkt)
{
	UINT_16 u2EtherType;
	PUINT_8 pucEthBody;
	UINT_8 ucResult = ENUM_PKT_FLAG_NUM;

	if (pucPkt == NULL) {
		DBGLOG(INIT, WARN, "kalGetPktEtherType pucPkt is null!\n");
		return ucResult;
	}

	u2EtherType = (pucPkt[ETH_TYPE_LEN_OFFSET] << 8) | (pucPkt[ETH_TYPE_LEN_OFFSET + 1]);
	pucEthBody = &pucPkt[ETH_HLEN];

	switch (u2EtherType) {
	case ETH_P_ARP:
	{
		DBGLOG(INIT, LOUD, "kalGetPktEtherType : ARP\n");
		ucResult = ENUM_PKT_ARP;
		break;
	}
	case ETH_P_IP:
	{
		UINT_8 ucIpProto = pucEthBody[9]; /* IP header without options */

		switch (ucIpProto) {
		case IP_PRO_ICMP:
		{
			DBGLOG(INIT, LOUD, "kalGetPktEtherType : ICMP\n");
			ucResult = ENUM_PKT_ICMP;
			break;
		}
		case IP_PRO_UDP:
		{
			PUINT_8 pucUdp = &pucEthBody[20];
			UINT_16 u2UdpSrcPort;
			UINT_16 u2UdpDstPort;

			u2UdpDstPort = (pucUdp[2] << 8) | pucUdp[3];
			u2UdpSrcPort = (pucUdp[0] << 8) | pucUdp[1];

			if ((u2UdpDstPort == UDP_PORT_DHCPS) || (u2UdpDstPort == UDP_PORT_DHCPC)) {
				DBGLOG(INIT, LOUD, "kalGetPktEtherType : DHCP\n");
				ucResult = ENUM_PKT_DHCP;
				break;
			} else if (u2UdpSrcPort == UDP_PORT_DNS) {
				DBGLOG(INIT, LOUD, "kalGetPktEtherType : DNS\n");
				ucResult = ENUM_PKT_DNS;
				break;
			}
		}
		}
			break;
	}
	case ETH_P_PRE_1X:
	{
		ucResult = ENUM_PKT_PROTECTED_1X;
		break;
	}
	case ETH_P_1X:
	{
		ucResult = ENUM_PKT_1X;
		break;
	}
	case TDLS_FRM_PROT_TYPE:
	{
		ucResult = ENUM_PKT_TDLS;
		break;
	}
	case ETH_WPI_1X:
	{
		ucResult = ENUM_PKT_WPI_1X;
		break;

	}
	default:
		DBGLOG(INIT, LOUD, "unSupport pkt type:u2EtherType:0x%x\n"
			, u2EtherType);
		break;
	}

	return ucResult;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in] prAdapter  Pointer of ADAPTER_T
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
VOID
kalGetChannelList(IN P_GLUE_INFO_T prGlueInfo,
		  IN ENUM_BAND_T eSpecificBand,
		  IN UINT_8 ucMaxChannelNum, IN PUINT_8 pucNumOfChannel, IN P_RF_CHANNEL_INFO_T paucChannelList)
{
	rlmDomainGetChnlList(prGlueInfo->prAdapter, eSpecificBand, FALSE, ucMaxChannelNum,
			     pucNumOfChannel, paucChannelList);
}
/*----------------------------------------------------------------------------*/
/*!
* \brief : get APMCU Mem buffer
*
* \param[in] type
* \param[in] index
* \param[in] pucBuffer
* \param[in] u4BufferLen

*
* \return none
*/
/*----------------------------------------------------------------------------*/

VOID kalGetAPMCUMen(IN P_GLUE_INFO_T prGlueInfo, IN UINT32 u4StartAddr
		, IN UINT32 u4Offset, IN UINT32 index, OUT PUINT_8 pucBuffer, IN UINT_32 u4BufferLen)
{

	PUINT8 descBaseAddr;

	DBGLOG(INIT, TRACE, "TC start addr:0x%x, index:%d, offset:0x%x len:%d"
		, u4StartAddr, index, u4Offset, u4BufferLen);

	descBaseAddr = (prGlueInfo->rHifInfo.APMcuRegBaseAddr + u4StartAddr
	+(u4Offset * index));

	if ((descBaseAddr != NULL) && (pucBuffer != NULL))
		kalMemCopy(pucBuffer, descBaseAddr, u4BufferLen);

}


/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in] prAdapter  Pointer of ADAPTER_T
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
BOOLEAN kalIsAPmode(IN P_GLUE_INFO_T prGlueInfo)
{
#if CFG_ENABLE_WIFI_DIRECT
	if (IS_NET_ACTIVE(prGlueInfo->prAdapter, NETWORK_TYPE_P2P_INDEX) &&
	    p2pFuncIsAPMode(prGlueInfo->prAdapter->rWifiVar.prP2pFsmInfo))
		return TRUE;
#endif

	return FALSE;
}

#ifdef MTK_DMA_BUF_MEMCPY_SUP
/*----------------------------------------------------------------------------*/
/*!
 * \brief This function gets the physical address for Pre-allocate I/O buffer.
 *
 * \param[in] prGlueInfo     Pointer of GLUE Data Structure
 * \param[in] rLockCategory  Specify which SPIN_LOCK
 * \param[out] pu4Flags      Pointer of a variable for saving IRQ flags
 *
 * \return physical addr
 */
/*----------------------------------------------------------------------------*/
ULONG kalIOPhyAddrGet(IN ULONG VirtAddr)
{
	ULONG PhyAddr;

	if ((VirtAddr >= (ULONG) pvIoBuffer) && (VirtAddr <= ((ULONG) (pvIoBuffer) + pvIoBufferSize))) {
		PhyAddr = (ULONG) pvIoPhyBuf;
		PhyAddr += (VirtAddr - (ULONG) (pvIoBuffer));
		return PhyAddr;
	}

	return 0;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function gets the physical address for Pre-allocate I/O buffer.
 *
 * \param[in] prGlueInfo     Pointer of GLUE Data Structure
 * \param[in] rLockCategory  Specify which SPIN_LOCK
 * \param[out] pu4Flags      Pointer of a variable for saving IRQ flags
 *
 * \return physical addr
 */
/*----------------------------------------------------------------------------*/
VOID kalDmaBufGet(OUT VOID **VirtAddr, OUT VOID **PhyAddr)
{
	*VirtAddr = pvDmaBuffer;
	*PhyAddr = pvDmaPhyBuf;
}
#endif /* MTK_DMA_BUF_MEMCPY_SUP */

#if CFG_SUPPORT_802_11W
/*----------------------------------------------------------------------------*/
/*!
 * \brief to check if the MFP is active or not
 *
 * \param[in]
 *           prGlueInfo
 *
 * \return
 *           TRUE
 *           FALSE
 */
/*----------------------------------------------------------------------------*/
UINT_32 kalGetMfpSetting(IN P_GLUE_INFO_T prGlueInfo)
{
	ASSERT(prGlueInfo);

	return prGlueInfo->rWpaInfo.u4Mfp;
}
#endif

struct file *kalFileOpen(const char *path, int flags, int rights)
{
	struct file *filp = NULL;
	mm_segment_t oldfs;
	int err = 0;

	oldfs = get_fs();
	set_fs(get_ds());
	filp = filp_open(path, flags, rights);
	set_fs(oldfs);
	if (IS_ERR(filp)) {
		err = PTR_ERR(filp);
		return NULL;
	}
	return filp;
}

VOID kalFileClose(struct file *file)
{
	filp_close(file, NULL);
}

UINT_32 kalFileRead(struct file *file, UINT_64 offset, UINT_8 *data, UINT_32 size)
{
	mm_segment_t oldfs;
	INT_32 ret;

	oldfs = get_fs();
	set_fs(get_ds());

	ret = vfs_read(file, data, size, &offset);

	set_fs(oldfs);
	return ret;
}

UINT_32 kalFileWrite(struct file *file, UINT_64 offset, UINT_8 *data, UINT_32 size)
{
	mm_segment_t oldfs;
	INT_32 ret;

	oldfs = get_fs();
	set_fs(get_ds());

	ret = vfs_write(file, data, size, &offset);

	set_fs(oldfs);
	return ret;
}

UINT_32 kalWriteToFile(const PUINT_8 pucPath, BOOLEAN fgDoAppend, PUINT_8 pucData, UINT_32 u4Size)
{
	struct file *file = NULL;
	UINT_32 ret = -1;
	UINT_32 u4Flags = 0;

	if (fgDoAppend)
		u4Flags = O_APPEND;

	file = kalFileOpen(pucPath, O_WRONLY | O_CREAT | u4Flags, S_IRWXU);
	if (file) {
		ret = kalFileWrite(file, 0, pucData, u4Size);
		kalFileClose(file);
	}

	return ret;
}

INT_32 kalReadToFile(const PUINT_8 pucPath, PUINT_8 pucData, UINT_32 u4Size, PUINT_32 pu4ReadSize)
{
	struct file *file = NULL;
	INT_32 ret = -1;
	UINT_32 u4ReadSize = 0;

	DBGLOG(INIT, LOUD, "kalReadToFile() path %s\n", pucPath);

	file = kalFileOpen(pucPath, O_RDONLY, 0);

	if ((file != NULL) && !IS_ERR(file)) {
		u4ReadSize = kalFileRead(file, 0, pucData, u4Size);
		kalFileClose(file);
		if (pu4ReadSize)
			*pu4ReadSize = u4ReadSize;
		ret = 0;
	}
	return ret;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief    To indicate BSS-INFO to NL80211 as scanning result
 *
 * \param[in]
 *           prGlueInfo
 *           pucBeaconProbeResp
 *           u4FrameLen
 *
 *
 *
 * \return
 *           none
 */
/*----------------------------------------------------------------------------*/
VOID
kalIndicateBssInfo(IN P_GLUE_INFO_T prGlueInfo,
		   IN PUINT_8 pucBeaconProbeResp,
		   IN UINT_32 u4FrameLen, IN UINT_8 ucChannelNum, IN INT_32 i4SignalStrength)
{
	struct wiphy *wiphy;
	struct ieee80211_channel *prChannel = NULL;

	ASSERT(prGlueInfo);
	wiphy = priv_to_wiphy(prGlueInfo);

	/* search through channel entries */
	if (ucChannelNum <= 14) {
		prChannel =
		    ieee80211_get_channel(wiphy, ieee80211_channel_to_frequency(ucChannelNum, IEEE80211_BAND_2GHZ));
	} else {
		prChannel =
		    ieee80211_get_channel(wiphy, ieee80211_channel_to_frequency(ucChannelNum, IEEE80211_BAND_5GHZ));
	}

	if (prChannel != NULL && (prGlueInfo->prScanRequest != NULL || prGlueInfo->prSchedScanRequest != NULL)) {
		struct cfg80211_bss *bss;
#if CFG_SUPPORT_TSF_USING_BOOTTIME
		struct ieee80211_mgmt *prMgmtFrame = (struct ieee80211_mgmt *)pucBeaconProbeResp;

		prMgmtFrame->u.beacon.timestamp = kalGetBootTime();
#endif
		ScanCnt++;

		/* indicate to NL80211 subsystem */
		bss = cfg80211_inform_bss_frame(wiphy,
						prChannel,
						(struct ieee80211_mgmt *)pucBeaconProbeResp,
						u4FrameLen, i4SignalStrength * 100, GFP_KERNEL);

		if (!bss) {
			ScanDoneFailCnt++;
			DBGLOG(SCN, WARN, "inform bss to cfg80211 failed, bss channel %d, rcpi %d\n",
					ucChannelNum, i4SignalStrength);
		} else {
			cfg80211_put_bss(wiphy, bss);
			DBGLOG(SCN, TRACE, "inform bss to cfg80211, bss channel %d, rcpi %d\n",
					ucChannelNum, i4SignalStrength);
		}
	}

}

/*----------------------------------------------------------------------------*/
/*!
 * \brief    To indicate channel ready
 *
 * \param[in]
 *           prGlueInfo
 *
 * \return
 *           none
 */
/*----------------------------------------------------------------------------*/
VOID
kalReadyOnChannel(IN P_GLUE_INFO_T prGlueInfo,
		  IN UINT_64 u8Cookie,
		  IN ENUM_BAND_T eBand, IN ENUM_CHNL_EXT_T eSco, IN UINT_8 ucChannelNum, IN UINT_32 u4DurationMs)
{
	struct ieee80211_channel *prChannel = NULL;
	enum nl80211_channel_type rChannelType;

	/* ucChannelNum = wlanGetChannelNumberByNetwork(prGlueInfo->prAdapter, NETWORK_TYPE_AIS_INDEX); */

	if (prGlueInfo->fgIsRegistered == TRUE) {
		if (ucChannelNum <= 14) {
			prChannel =
			    ieee80211_get_channel(priv_to_wiphy(prGlueInfo),
						  ieee80211_channel_to_frequency(ucChannelNum, IEEE80211_BAND_2GHZ));
		} else {
			prChannel =
			    ieee80211_get_channel(priv_to_wiphy(prGlueInfo),
						  ieee80211_channel_to_frequency(ucChannelNum, IEEE80211_BAND_5GHZ));
		}

		switch (eSco) {
		case CHNL_EXT_SCN:
			rChannelType = NL80211_CHAN_NO_HT;
			break;

		case CHNL_EXT_SCA:
			rChannelType = NL80211_CHAN_HT40MINUS;
			break;

		case CHNL_EXT_SCB:
			rChannelType = NL80211_CHAN_HT40PLUS;
			break;

		case CHNL_EXT_RES:
		default:
			rChannelType = NL80211_CHAN_HT20;
			break;
		}

		cfg80211_ready_on_channel(prGlueInfo->prDevHandler->ieee80211_ptr, u8Cookie, prChannel, u4DurationMs,
					  GFP_KERNEL);
	}

}

/*----------------------------------------------------------------------------*/
/*!
 * \brief    To indicate channel expiration
 *
 * \param[in]
 *           prGlueInfo
 *
 * \return
 *           none
 */
/*----------------------------------------------------------------------------*/
VOID
kalRemainOnChannelExpired(IN P_GLUE_INFO_T prGlueInfo,
			  IN UINT_64 u8Cookie, IN ENUM_BAND_T eBand, IN ENUM_CHNL_EXT_T eSco, IN UINT_8 ucChannelNum)
{
	struct ieee80211_channel *prChannel = NULL;
	enum nl80211_channel_type rChannelType;

	ucChannelNum = wlanGetChannelNumberByNetwork(prGlueInfo->prAdapter, NETWORK_TYPE_AIS_INDEX);

	if (prGlueInfo->fgIsRegistered == TRUE) {
		if (ucChannelNum <= 14) {
			prChannel =
			    ieee80211_get_channel(priv_to_wiphy(prGlueInfo),
						  ieee80211_channel_to_frequency(ucChannelNum, IEEE80211_BAND_2GHZ));
		} else {
			prChannel =
			    ieee80211_get_channel(priv_to_wiphy(prGlueInfo),
						  ieee80211_channel_to_frequency(ucChannelNum, IEEE80211_BAND_5GHZ));
		}

		switch (eSco) {
		case CHNL_EXT_SCN:
			rChannelType = NL80211_CHAN_NO_HT;
			break;

		case CHNL_EXT_SCA:
			rChannelType = NL80211_CHAN_HT40MINUS;
			break;

		case CHNL_EXT_SCB:
			rChannelType = NL80211_CHAN_HT40PLUS;
			break;

		case CHNL_EXT_RES:
		default:
			rChannelType = NL80211_CHAN_HT20;
			break;
		}

		cfg80211_remain_on_channel_expired(prGlueInfo->prDevHandler->ieee80211_ptr, u8Cookie, prChannel,
						   GFP_KERNEL);
	}

}

/*----------------------------------------------------------------------------*/
/*!
 * \brief    To indicate Mgmt tx status
 *
 * \param[in]
 *           prGlueInfo
 *
 * \return
 *           none
 */
/*----------------------------------------------------------------------------*/
VOID
kalIndicateMgmtTxStatus(IN P_GLUE_INFO_T prGlueInfo,
			IN UINT_64 u8Cookie, IN BOOLEAN fgIsAck, IN PUINT_8 pucFrameBuf, IN UINT_32 u4FrameLen)
{

	do {
		if ((prGlueInfo == NULL) || (pucFrameBuf == NULL) || (u4FrameLen == 0)) {
			DBGLOG(AIS, TRACE, "Unexpected pointer PARAM. %p, %p, %u.",
					    prGlueInfo, pucFrameBuf, u4FrameLen);
			ASSERT(FALSE);
			break;
		}

		cfg80211_mgmt_tx_status(prGlueInfo->prDevHandler->ieee80211_ptr,
					u8Cookie, pucFrameBuf, u4FrameLen, fgIsAck, GFP_KERNEL);
	} while (FALSE);

}				/* kalIndicateMgmtTxStatus */

VOID kalIndicateRxMgmtFrame(IN P_GLUE_INFO_T prGlueInfo, IN P_SW_RFB_T prSwRfb)
{
#define DBG_MGMT_FRAME_INDICATION 1
	INT_32 i4Freq = 0;
	UINT_8 ucChnlNum = 0;
#if DBG_MGMT_FRAME_INDICATION
	P_WLAN_MAC_HEADER_T prWlanHeader = (P_WLAN_MAC_HEADER_T) NULL;
#endif

	do {
		if ((prGlueInfo == NULL) || (prSwRfb == NULL)) {
			ASSERT(FALSE);
			break;
		}

		ucChnlNum = prSwRfb->prHifRxHdr->ucHwChannelNum;

#if DBG_MGMT_FRAME_INDICATION
		prWlanHeader = (P_WLAN_MAC_HEADER_T) prSwRfb->pvHeader;

		switch (prWlanHeader->u2FrameCtrl) {
		case MAC_FRAME_PROBE_REQ:
			DBGLOG(AIS, TRACE, "RX Probe Req at channel %d ", ucChnlNum);
			break;
		case MAC_FRAME_PROBE_RSP:
			DBGLOG(AIS, TRACE, "RX Probe Rsp at channel %d ", ucChnlNum);
			break;
		case MAC_FRAME_ACTION:
			DBGLOG(AIS, TRACE, "RX Action frame at channel %d ", ucChnlNum);
			break;
		default:
			DBGLOG(AIS, INFO, "RX Packet:%d at channel %d ", prWlanHeader->u2FrameCtrl, ucChnlNum);
			break;
		}

#endif
		i4Freq = nicChannelNum2Freq(ucChnlNum) / 1000;

		cfg80211_rx_mgmt(prGlueInfo->prDevHandler->ieee80211_ptr,	/* struct net_device * dev, */
				 i4Freq,
				 RCPI_TO_dBm(prSwRfb->prHifRxHdr->ucRcpi),
				 prSwRfb->pvHeader, prSwRfb->u2PacketLen, GFP_KERNEL);
	} while (FALSE);

}				/* kalIndicateRxMgmtFrame */

#if CFG_SUPPORT_AGPS_ASSIST
BOOLEAN kalIndicateAgpsNotify(P_ADAPTER_T prAdapter, UINT_8 cmd, PUINT_8 data, UINT_16 dataLen)
{
	P_GLUE_INFO_T prGlueInfo = prAdapter->prGlueInfo;

	struct sk_buff *skb = cfg80211_testmode_alloc_event_skb(priv_to_wiphy(prGlueInfo),
								dataLen, GFP_KERNEL);
	if (!skb) {
		DBGLOG(AIS, ERROR, "kalIndicateAgpsNotify: alloc skb failed\n");
		return FALSE;
	}

	/* DBGLOG(CCX, INFO, ("WLAN_STATUS_AGPS_NOTIFY, cmd=%d\n", cmd)); */
	if (unlikely(nla_put(skb, MTK_ATTR_AGPS_CMD, sizeof(cmd), &cmd) < 0))
		goto nla_put_failure;
	if (dataLen > 0 && data && unlikely(nla_put(skb, MTK_ATTR_AGPS_DATA, dataLen, data) < 0))
		goto nla_put_failure;
	if (unlikely(nla_put(skb, MTK_ATTR_AGPS_IFINDEX, sizeof(UINT_32), &prGlueInfo->prDevHandler->ifindex) < 0))
		goto nla_put_failure;
	/* currently, the ifname maybe wlan0, p2p0, so the maximum name length will be 5 bytes */
	if (unlikely(nla_put(skb, MTK_ATTR_AGPS_IFNAME, 5, prGlueInfo->prDevHandler->name) < 0))
		goto nla_put_failure;
	cfg80211_testmode_event(skb, GFP_KERNEL);
	return TRUE;

nla_put_failure:
	kfree_skb(skb);
	return FALSE;
}
#endif

#if (CFG_SUPPORT_MET_PROFILING == 1)
#define PROC_MET_PROF_CTRL                 "met_ctrl"
#define PROC_MET_PROF_PORT                 "met_port"

struct proc_dir_entry *pMetProcDir;
void *pMetGlobalData;
static unsigned long __read_mostly tracing_mark_write_addr;

static inline void __mt_update_tracing_mark_write_addr(void)
{
	if (unlikely(tracing_mark_write_addr == 0))
		tracing_mark_write_addr = kallsyms_lookup_name("tracing_mark_write");
}

VOID kalMetProfilingStart(IN P_GLUE_INFO_T prGlueInfo, IN struct sk_buff *prSkb)
{
	UINT_8 ucIpVersion;
	UINT_16 u2UdpSrcPort;
	UINT_16 u2RtpSn;
	PUINT_8 pucEthHdr = prSkb->data;
	PUINT_8 pucIpHdr, pucUdpHdr, pucRtpHdr;

	/* | Ethernet(14) | IP(20) | UDP(8)| RTP(12) | */
	/* UDP==> |SRC_PORT(2)|DST_PORT(2)|LEN(2)|CHKSUM(2)| */
	/* RTP==> |CTRL(2)|SEQ(2)|TimeStamp(4)|... */
	/* printk("MET_PROF: MET enable=%d(HardXmit)\n", prGlueInfo->u8MetProfEnable); */
	if (prGlueInfo->u8MetProfEnable == 1) {
		u2UdpSrcPort = prGlueInfo->u16MetUdpPort;
		if ((*(pucEthHdr + 12) == 0x08) && (*(pucEthHdr + 13) == 0x00)) {
			/* IP */
			pucIpHdr = pucEthHdr + ETH_HLEN;
			ucIpVersion = (*pucIpHdr & IPVH_VERSION_MASK) >> IPVH_VERSION_OFFSET;
			if ((ucIpVersion == IPVERSION) && (pucIpHdr[IPV4_HDR_IP_PROTOCOL_OFFSET] == IP_PROTOCOL_UDP)) {
				/* UDP */
				pucUdpHdr = pucIpHdr + IP_HEADER_LEN;
				/* check UDP port number */
				if (((UINT_16) pucUdpHdr[0] << 8 | (UINT_16) pucUdpHdr[1]) == u2UdpSrcPort) {
					/* RTP */
					pucRtpHdr = pucUdpHdr + 8;
					u2RtpSn = (UINT_16) pucRtpHdr[2] << 8 | pucRtpHdr[3];
					/*
					 * trace_printk("S|%d|%s|%d\n", current->tgid, "WIFI-CHIP", u2RtpSn);
					 * //frm_sequence);
					 */
#ifdef CONFIG_TRACING
					__mt_update_tracing_mark_write_addr();
					if (tracing_mark_write_addr != 0) {
						event_trace_printk(tracing_mark_write_addr, "S|%d|%s|%d\n",
								   current->tgid, "WIFI-CHIP", u2RtpSn);
					}
#endif
				}
			}
		}
	}
}

VOID kalMetProfilingFinish(IN P_ADAPTER_T prAdapter, IN P_MSDU_INFO_T prMsduInfo)
{
	UINT_8 ucIpVersion;
	UINT_16 u2UdpSrcPort;
	UINT_16 u2RtpSn;
	struct sk_buff *prSkb = (struct sk_buff *)prMsduInfo->prPacket;
	PUINT_8 pucEthHdr = prSkb->data;
	PUINT_8 pucIpHdr, pucUdpHdr, pucRtpHdr;
	P_GLUE_INFO_T prGlueInfo = prAdapter->prGlueInfo;

	/* | Ethernet(14) | IP(20) | UDP(8)| RTP(12) | */
	/* UDP==> |SRC_PORT(2)|DST_PORT(2)|LEN(2)|CHKSUM(2)| */
	/* RTP==> |CTRL(2)|SEQ(2)|TimeStamp(4)|... */
	/* printk("MET_PROF: MET enable=%d(TxMsdu)\n", prGlueInfo->u8MetProfEnable); */
	if (prGlueInfo->u8MetProfEnable == 1) {
		u2UdpSrcPort = prGlueInfo->u16MetUdpPort;
		if ((*(pucEthHdr + 12) == 0x08) && (*(pucEthHdr + 13) == 0x00)) {
			/* IP */
			pucIpHdr = pucEthHdr + ETH_HLEN;
			ucIpVersion = (*pucIpHdr & IPVH_VERSION_MASK) >> IPVH_VERSION_OFFSET;
			if ((ucIpVersion == IPVERSION) && (pucIpHdr[IPV4_HDR_IP_PROTOCOL_OFFSET] == IP_PROTOCOL_UDP)) {
				/* UDP */
				pucUdpHdr = pucIpHdr + IP_HEADER_LEN;
				/* check UDP port number */
				if (((UINT_16) pucUdpHdr[0] << 8 | (UINT_16) pucUdpHdr[1]) == u2UdpSrcPort) {
					/* RTP */
					pucRtpHdr = pucUdpHdr + 8;
					u2RtpSn = (UINT_16) pucRtpHdr[2] << 8 | pucRtpHdr[3];
					/*
					 * trace_printk("F|%d|%s|%d\n", current->tgid, "WIFI-CHIP", u2RtpSn);
					 * //frm_sequence);
					 */
#ifdef CONFIG_TRACING
					__mt_update_tracing_mark_write_addr();
					if (tracing_mark_write_addr != 0) {
						event_trace_printk(tracing_mark_write_addr, "F|%d|%s|%d\n",
								   current->tgid, "WIFI-CHIP", u2RtpSn);
					}
#endif
				}
			}
		}
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief The PROC function for adjusting Debug Level to turn on/off debugging message.
 *
 * \param[in] file   pointer to file.
 * \param[in] buffer Buffer from user space.
 * \param[in] count  Number of characters to write
 * \param[in] data   Pointer to the private data structure.
 *
 * \return number of characters write from User Space.
 */
/*----------------------------------------------------------------------------*/
static ssize_t kalMetCtrlWriteProcfs(struct file *file, const char __user *buffer, size_t count, loff_t *off)
{
	char acBuf[128 + 1];	/* + 1 for "\0" */
	UINT_32 u4CopySize;
	int u8MetProfEnable;

	IN P_GLUE_INFO_T prGlueInfo;

	u4CopySize = (count < (sizeof(acBuf) - 1)) ? count : (sizeof(acBuf) - 1);
	if (copy_from_user(acBuf, buffer, u4CopySize))
		return -1;
	acBuf[u4CopySize] = '\0';

	if (sscanf(acBuf, " %d", &u8MetProfEnable) == 1)
		DBGLOG(INIT, INFO, "MET_PROF: Write MET PROC Enable=%d\n", u8MetProfEnable);
	if (pMetGlobalData != NULL) {
		prGlueInfo = (P_GLUE_INFO_T) pMetGlobalData;
		prGlueInfo->u8MetProfEnable = (UINT_8) u8MetProfEnable;
	}
	return count;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief The PROC function for adjusting Debug Level to turn on/off debugging message.
 *
 * \param[in] file   pointer to file.
 * \param[in] buffer Buffer from user space.
 * \param[in] count  Number of characters to write
 * \param[in] data   Pointer to the private data structure.
 *
 * \return number of characters write from User Space.
 */
/*----------------------------------------------------------------------------*/
static ssize_t kalMetPortWriteProcfs(struct file *file, const char __user *buffer, size_t count, loff_t *off)
{
	char acBuf[128 + 1];	/* + 1 for "\0" */
	UINT_32 u4CopySize;
	int u16MetUdpPort;

	IN P_GLUE_INFO_T prGlueInfo;

	u4CopySize = (count < (sizeof(acBuf) - 1)) ? count : (sizeof(acBuf) - 1);
	if (copy_from_user(acBuf, buffer, u4CopySize))
		return -1;
	acBuf[u4CopySize] = '\0';

	if (sscanf(acBuf, " %d", &u16MetUdpPort) == 1)
		DBGLOG(INIT, INFO, "MET_PROF: Write MET PROC UDP_PORT=%d\n", u16MetUdpPort);
	if (pMetGlobalData != NULL) {
		prGlueInfo = (P_GLUE_INFO_T) pMetGlobalData;
		prGlueInfo->u16MetUdpPort = (UINT_16) u16MetUdpPort;
	}
	return count;
}

const struct file_operations rMetProcCtrlFops = {
	.write = kalMetCtrlWriteProcfs
};

const struct file_operations rMetProcPortFops = {
	.write = kalMetPortWriteProcfs
};

int kalMetInitProcfs(IN P_GLUE_INFO_T prGlueInfo)
{
	/* struct proc_dir_entry *pMetProcDir; */
	if (init_net.proc_net == (struct proc_dir_entry *)NULL) {
		DBGLOG(INIT, INFO, "init proc fs fail: proc_net == NULL\n");
		return -ENOENT;
	}
	/*
	 * Directory: Root (/proc/net/wlan0)
	 */
	pMetProcDir = proc_mkdir("wlan0", init_net.proc_net);
	if (pMetProcDir == NULL)
		return -ENOENT;
	/*
	 * /proc/net/wlan0
	 * |-- met_ctrl         (PROC_MET_PROF_CTRL)
	 * |-- met_port         (PROC_MET_PROF_PORT)
	 */
	/* proc_create(PROC_MET_PROF_CTRL, 0x0644, pMetProcDir, &rMetProcFops); */
	proc_create(PROC_MET_PROF_CTRL, 0, pMetProcDir, &rMetProcCtrlFops);
	proc_create(PROC_MET_PROF_PORT, 0, pMetProcDir, &rMetProcPortFops);

	pMetGlobalData = (void *)prGlueInfo;

	return 0;
}

int kalMetRemoveProcfs(void)
{

	if (init_net.proc_net == (struct proc_dir_entry *)NULL) {
		DBGLOG(INIT, WARN, "remove proc fs fail: proc_net == NULL\n");
		return -ENOENT;
	}
	remove_proc_entry(PROC_MET_PROF_CTRL, pMetProcDir);
	remove_proc_entry(PROC_MET_PROF_PORT, pMetProcDir);
	/* remove root directory (proc/net/wlan0) */
	remove_proc_entry("wlan0", init_net.proc_net);
	/* clear MetGlobalData */
	pMetGlobalData = NULL;

	return 0;
}
#endif
UINT_64 kalGetBootTime(void)
{
	struct timespec ts;
	UINT_64 bootTime = 0;

	get_monotonic_boottime(&ts);
	/*
	 * we assign ts.tv_sec to bootTime first, then multiply USEC_PER_SEC
	 * this will prevent multiply result turn to a negative value on 32bit system
	 */
	bootTime = ts.tv_sec;
	bootTime *= USEC_PER_SEC;
	bootTime += ts.tv_nsec / NSEC_PER_USEC;
	return bootTime;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief    To indicate scheduled scan results are avilable
 *
 * \param[in]
 *           prGlueInfo
 *
 * \return
 *           None
 */
/*----------------------------------------------------------------------------*/
VOID kalSchedScanResults(IN P_GLUE_INFO_T prGlueInfo)
{
	ASSERT(prGlueInfo);

	cfg80211_sched_scan_results(priv_to_wiphy(prGlueInfo));

}

/*----------------------------------------------------------------------------*/
/*!
* \brief    To indicate scheduled scan has been stopped
*
* \param[in]
*           prGlueInfo
*
* \return
*           None
*/
/*----------------------------------------------------------------------------*/
VOID kalSchedScanStopped(IN P_GLUE_INFO_T prGlueInfo)
{
	GLUE_SPIN_LOCK_DECLARATION();

	ASSERT(prGlueInfo);

	/* 1. reset first for newly incoming request */
	GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
	if (prGlueInfo->prSchedScanRequest != NULL)
		prGlueInfo->prSchedScanRequest = NULL;
	GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
	DBGLOG(SCN, INFO, "cfg80211_sched_scan_stopped send event\n");

	/* 2. indication to cfg80211 */
	/*
	 *  20150205 change cfg80211_sched_scan_stopped to work queue to use K thread to send event instead of Tx thread
	 * due to sched_scan_mtx dead lock issue by Tx thread serves oid cmds and send event in the same time
	 */
	DBGLOG(SCN, TRACE, "start work queue to send event\n");
	schedule_delayed_work(&sched_workq, 0);
	DBGLOG(SCN, TRACE, "tx_thread return from kalSchedScanStoppped\n");

}

#if CFG_SUPPORT_WAKEUP_REASON_DEBUG
/* if SPM is not implement this function, we will use this default one */
wake_reason_t __weak slp_get_wake_reason(VOID)
{
	return WR_NONE;
}
/* if SPM is not implement this function, we will use this default one */
UINT_32 __weak spm_get_last_wakeup_src(VOID)
{
	return 0;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief    To check if device if wake up by wlan
 *
 * \param[in]
 *           prAdapter
 *
 * \return
 *           TRUE: wake up by wlan; otherwise, FALSE
 */
/*----------------------------------------------------------------------------*/
BOOLEAN kalIsWakeupByWlan(P_ADAPTER_T  prAdapter)
{
	/*
	 * SUSPEND_FLAG_FOR_WAKEUP_REASON is set means system has suspended, but may be failed
	 * duo to some driver suspend failed. so we need help of function slp_get_wake_reason
	 */
	if (test_and_clear_bit(SUSPEND_FLAG_FOR_WAKEUP_REASON, &prAdapter->ulSuspendFlag) == 0)
		return FALSE;
	/*
	 * if slp_get_wake_reason or spm_get_last_wakeup_src is NULL, it means SPM module didn't implement
	 * it. then we should return FALSE always. otherwise,  if slp_get_wake_reason returns WR_WAKE_SRC,
	 * then it means the host is suspend successfully.
	 */
	if (slp_get_wake_reason() != WR_WAKE_SRC)
		return FALSE;
	/*
	 * spm_get_last_wakeup_src will returns the last wakeup source,
	 * WAKE_SRC_CONN2AP is connsys
	 */
	return !!(spm_get_last_wakeup_src() & WAKE_SRC_CONN2AP);
}
#endif

INT_32 kalHaltLock(UINT_32 waitMs)
{
	INT_32 i4Ret = 0;

	if (waitMs) {
		i4Ret = down_timeout(&rHaltCtrl.lock, MSEC_TO_JIFFIES(waitMs));
		if (!i4Ret)
			goto success;
		if (i4Ret != -ETIME)
			return i4Ret;
		if (rHaltCtrl.fgHeldByKalIoctl) {
			P_GLUE_INFO_T prGlueInfo = NULL;

			wlanExportGlueInfo(&prGlueInfo);

			DBGLOG(INIT, ERROR,
				"kalIoctl was executed longer than %u ms, show backtrace of tx_thread!\n",
				kalGetTimeTick() - rHaltCtrl.u4HoldStart);
			if (prGlueInfo)
				show_stack(prGlueInfo->main_thread, NULL);
		} else {
			DBGLOG(INIT, ERROR, "halt lock held by %s pid %d longer than %u ms!\n",
				rHaltCtrl.owner->comm, rHaltCtrl.owner->pid,
				kalGetTimeTick() - rHaltCtrl.u4HoldStart);
			show_stack(rHaltCtrl.owner, NULL);
		}
		return i4Ret;
	}
	down(&rHaltCtrl.lock);
success:
	rHaltCtrl.owner = current;
	rHaltCtrl.u4HoldStart = kalGetTimeTick();
	return 0;
}

INT_32 kalHaltTryLock(VOID)
{
	INT_32 i4Ret = 0;

	i4Ret = down_trylock(&rHaltCtrl.lock);
	if (i4Ret)
		return i4Ret;
	rHaltCtrl.owner = current;
	rHaltCtrl.u4HoldStart = kalGetTimeTick();
	return 0;
}

VOID kalHaltUnlock(VOID)
{
	if (kalGetTimeTick() - rHaltCtrl.u4HoldStart > WLAN_OID_TIMEOUT_THRESHOLD * 2 &&
		rHaltCtrl.owner)
		DBGLOG(INIT, ERROR, "process %s pid %d hold halt lock longer than 4s!\n",
			rHaltCtrl.owner->comm, rHaltCtrl.owner->pid);
	rHaltCtrl.owner = NULL;
	up(&rHaltCtrl.lock);
}

VOID kalSetHalted(BOOLEAN fgHalt)
{
	rHaltCtrl.fgHalt = fgHalt;
}

BOOLEAN kalIsHalted(VOID)
{
	return rHaltCtrl.fgHalt;
}

VOID kalPerMonDump(IN P_GLUE_INFO_T prGlueInfo)
{
	struct PERF_MONITOR_T *prPerMonitor;

	prPerMonitor = &prGlueInfo->prAdapter->rPerMonitor;
	DBGLOG(SW4, WARN, "ulPerfMonFlag:0x%lx\n", prPerMonitor->ulPerfMonFlag);
	DBGLOG(SW4, WARN, "ulLastTxBytes:%ld\n", prPerMonitor->ulLastTxBytes);
	DBGLOG(SW4, WARN, "ulLastRxBytes:%ld\n", prPerMonitor->ulLastRxBytes);
	DBGLOG(SW4, WARN, "ulP2PLastTxBytes:%ld\n", prPerMonitor->ulP2PLastTxBytes);
	DBGLOG(SW4, WARN, "ulP2PLastRxBytes:%ld\n", prPerMonitor->ulP2PLastRxBytes);
	DBGLOG(SW4, WARN, "ulThroughput:%ld\n", prPerMonitor->ulThroughput);
	DBGLOG(SW4, WARN, "u4UpdatePeriod:%d\n", prPerMonitor->u4UpdatePeriod);
	DBGLOG(SW4, WARN, "u4TarPerfLevel:%d\n", prPerMonitor->u4TarPerfLevel);
	DBGLOG(SW4, WARN, "u4CurrPerfLevel:%d\n", prPerMonitor->u4CurrPerfLevel);
	DBGLOG(SW4, WARN, "netStats tx_bytes:%ld\n", prGlueInfo->rNetDevStats.tx_bytes);
	DBGLOG(SW4, WARN, "netStats tx_bytes:%ld\n", prGlueInfo->rNetDevStats.rx_bytes);
	DBGLOG(SW4, WARN, "p2p netStats tx_bytes:%ld\n", prGlueInfo->prP2PInfo->rNetDevStats.tx_bytes);
	DBGLOG(SW4, WARN, "p2p netStats tx_bytes:%ld\n", prGlueInfo->prP2PInfo->rNetDevStats.rx_bytes);
}

inline INT_32 kalPerMonInit(IN P_GLUE_INFO_T prGlueInfo)
{
	struct PERF_MONITOR_T *prPerMonitor;

	prPerMonitor = &prGlueInfo->prAdapter->rPerMonitor;
	DBGLOG(SW4, INFO, "enter %s\n", __func__);
	if (KAL_TEST_BIT(PERF_MON_RUNNING_BIT, prPerMonitor->ulPerfMonFlag))
		DBGLOG(SW4, WARN, "abnormal, perf monitory already running\n");
	KAL_CLR_BIT(PERF_MON_RUNNING_BIT, prPerMonitor->ulPerfMonFlag);
	KAL_CLR_BIT(PERF_MON_DISABLE_BIT, prPerMonitor->ulPerfMonFlag);
	KAL_SET_BIT(PERF_MON_STOP_BIT, prPerMonitor->ulPerfMonFlag);
	prPerMonitor->u4UpdatePeriod = 1000;
	cnmTimerInitTimer(prGlueInfo->prAdapter,
		&prPerMonitor->rPerfMonTimer,
		(PFN_MGMT_TIMEOUT_FUNC) kalPerMonHandler, (ULONG) NULL);
	DBGLOG(SW4, INFO, "exit %s\n", __func__);
	return 0;
}

inline INT_32 kalPerMonDisable(IN P_GLUE_INFO_T prGlueInfo)
{
	struct PERF_MONITOR_T *prPerMonitor;

	prPerMonitor = &prGlueInfo->prAdapter->rPerMonitor;

	DBGLOG(SW4, INFO, "enter %s\n", __func__);
	if (KAL_TEST_BIT(PERF_MON_RUNNING_BIT, prPerMonitor->ulPerfMonFlag)) {
		DBGLOG(SW4, TRACE, "need to stop before disable\n");
		kalPerMonStop(prGlueInfo);
	}
	KAL_SET_BIT(PERF_MON_DISABLE_BIT, prPerMonitor->ulPerfMonFlag);
	DBGLOG(SW4, TRACE, "exit %s\n", __func__);
	return 0;
}

inline INT_32 kalPerMonEnable(IN P_GLUE_INFO_T prGlueInfo)
{
	struct PERF_MONITOR_T *prPerMonitor;

	prPerMonitor = &prGlueInfo->prAdapter->rPerMonitor;

	DBGLOG(SW4, INFO, "enter %s\n", __func__);
	KAL_CLR_BIT(PERF_MON_DISABLE_BIT, prPerMonitor->ulPerfMonFlag);
	DBGLOG(SW4, TRACE, "exit %s\n", __func__);
	return 0;
}

inline INT_32 kalPerMonStart(IN P_GLUE_INFO_T prGlueInfo)
{
	struct PERF_MONITOR_T *prPerMonitor;

	prPerMonitor = &prGlueInfo->prAdapter->rPerMonitor;
	DBGLOG(SW4, TRACE, "enter %s\n", __func__);

	if ((wlan_fb_power_down || fgIsUnderSuspend) &&
		!KAL_TEST_BIT(PERF_MON_DISABLE_BIT, prPerMonitor->ulPerfMonFlag)) {
		/*
		 * Remove this to prevent KE, kalPerMonStart might be called in soft irq
		 * kalBoostCpu might call flush_work which will use wait_for_completion
		 * then KE will happen in this case
		 * Simply don't start performance monitor here
		 */
		/*kalPerMonDisable(prGlueInfo);*/
		return 0;
	}
	if (KAL_TEST_BIT(PERF_MON_DISABLE_BIT, prPerMonitor->ulPerfMonFlag) ||
		KAL_TEST_BIT(PERF_MON_RUNNING_BIT, prPerMonitor->ulPerfMonFlag))
		return 0;

	prPerMonitor->ulLastRxBytes = 0;
	prPerMonitor->ulLastTxBytes = 0;
	prPerMonitor->ulP2PLastRxBytes = 0;
	prPerMonitor->ulP2PLastTxBytes = 0;
	prPerMonitor->ulThroughput = 0;
	prPerMonitor->u4CurrPerfLevel = 0;
	prPerMonitor->u4TarPerfLevel = 0;
	prPerMonitor->u1ShutdownCoreCount = 0;

	cnmTimerStartTimer(prGlueInfo->prAdapter, &prPerMonitor->rPerfMonTimer, prPerMonitor->u4UpdatePeriod);
	KAL_SET_BIT(PERF_MON_RUNNING_BIT, prPerMonitor->ulPerfMonFlag);
	KAL_CLR_BIT(PERF_MON_STOP_BIT, prPerMonitor->ulPerfMonFlag);
	DBGLOG(SW4, INFO, "perf monitor started\n");
	return 0;
}

inline INT_32 kalPerMonStop(IN P_GLUE_INFO_T prGlueInfo)
{
	struct PERF_MONITOR_T *prPerMonitor;

	prPerMonitor = &prGlueInfo->prAdapter->rPerMonitor;
	DBGLOG(SW4, TRACE, "enter %s\n", __func__);

	if (KAL_TEST_BIT(PERF_MON_DISABLE_BIT, prPerMonitor->ulPerfMonFlag)) {
		DBGLOG(SW4, TRACE, "perf monitory disabled\n");
		return 0;
	}

	if (KAL_TEST_BIT(PERF_MON_STOP_BIT, prPerMonitor->ulPerfMonFlag)) {
		DBGLOG(SW4, TRACE, "perf monitory already stopped\n");
		return 0;
	}

	KAL_SET_BIT(PERF_MON_STOP_BIT, prPerMonitor->ulPerfMonFlag);
	if (KAL_TEST_BIT(PERF_MON_RUNNING_BIT, prPerMonitor->ulPerfMonFlag)) {
		cnmTimerStopTimer(prGlueInfo->prAdapter, &prPerMonitor->rPerfMonTimer);
		KAL_CLR_BIT(PERF_MON_RUNNING_BIT, prPerMonitor->ulPerfMonFlag);
		prPerMonitor->ulLastRxBytes = 0;
		prPerMonitor->ulLastTxBytes = 0;
		prPerMonitor->ulP2PLastRxBytes = 0;
		prPerMonitor->ulP2PLastTxBytes = 0;
		prPerMonitor->ulThroughput = 0;
		prPerMonitor->u4CurrPerfLevel = 0;
		prPerMonitor->u4TarPerfLevel = 0;
		prPerMonitor->u1ShutdownCoreCount = 0;

		/*Cancel CPU performance mode request*/
		kalBoostCpu(0);
	}
	DBGLOG(SW4, TRACE, "exit %s\n", __func__);
	return 0;
}

inline INT_32 kalPerMonDestroy(IN P_GLUE_INFO_T prGlueInfo)
{
	kalPerMonDisable(prGlueInfo);
	return 0;
}

VOID kalPerMonHandler(IN P_ADAPTER_T prAdapter, ULONG ulParam)
{
	/* Calculate current throughput */
	struct PERF_MONITOR_T *prPerMonitor;
	struct net_device *prNetDev = NULL;
	P_BSS_INFO_T prP2pBssInfo = (P_BSS_INFO_T) NULL;
	LONG latestTxBytes, latestRxBytes, txDiffBytes, rxDiffBytes;
	LONG p2pLatestTxBytes, p2pLatestRxBytes, p2pTxDiffBytes, p2pRxDiffBytes;
	P_GLUE_INFO_T prGlueInfo = prAdapter->prGlueInfo;
	BOOLEAN needBoostCpu = TRUE;

	if ((prGlueInfo->ulFlag & GLUE_FLAG_HALT) || (!prAdapter->fgIsP2PRegistered))
		return;

	prNetDev = prGlueInfo->prDevHandler;
	prP2pBssInfo = &prGlueInfo->prAdapter->rWifiVar.arBssInfo[NETWORK_TYPE_P2P_INDEX];
	prPerMonitor = &prAdapter->rPerMonitor;
	DBGLOG(SW4, TRACE, "enter kalPerMonHandler\n");

	latestTxBytes = prGlueInfo->rNetDevStats.tx_bytes;
	latestRxBytes = prGlueInfo->rNetDevStats.rx_bytes;
	p2pLatestTxBytes = prGlueInfo->prP2PInfo->rNetDevStats.tx_bytes;
	p2pLatestRxBytes = prGlueInfo->prP2PInfo->rNetDevStats.rx_bytes;
	if (prPerMonitor->ulLastRxBytes == 0 &&
		prPerMonitor->ulLastTxBytes == 0 &&
		prPerMonitor->ulP2PLastRxBytes == 0 &&
		prPerMonitor->ulP2PLastTxBytes == 0) {
		prPerMonitor->ulThroughput = 0;
	} else {
		txDiffBytes = latestTxBytes - prPerMonitor->ulLastTxBytes;
		rxDiffBytes = latestRxBytes - prPerMonitor->ulLastRxBytes;
		if (txDiffBytes < 0)
			txDiffBytes = -(txDiffBytes);
		if (rxDiffBytes < 0)
			rxDiffBytes = -(rxDiffBytes);
		p2pTxDiffBytes = p2pLatestTxBytes - prPerMonitor->ulP2PLastTxBytes;
		p2pRxDiffBytes = p2pLatestRxBytes - prPerMonitor->ulP2PLastRxBytes;
		if (p2pTxDiffBytes < 0)
			p2pTxDiffBytes = -(p2pTxDiffBytes);
		if (p2pRxDiffBytes < 0)
			p2pRxDiffBytes = -(p2pRxDiffBytes);

		prPerMonitor->ulThroughput = txDiffBytes + rxDiffBytes + p2pTxDiffBytes + p2pRxDiffBytes;
		prPerMonitor->ulThroughput *= (prPerMonitor->u4UpdatePeriod/1000);
		prPerMonitor->ulThroughput <<= 3;

		/* AIS: [Bytes TX][Bytes RX] [Packet TX][Packet RX], P2P: [Bytes TX][Bytes RX] [Packet TX][Packet RX] */
		DBGLOG(SW4, INFO,
			"kalPerMonHandler > Tput: %ld > AIS:[%ld][%ld] [%ld][%ld], P2P:[%ld][%ld] [%ld][%ld]\n",
			prPerMonitor->ulThroughput,
			txDiffBytes, rxDiffBytes,
			prGlueInfo->rNetDevStats.tx_packets, prGlueInfo->rNetDevStats.rx_packets,
			p2pTxDiffBytes, p2pRxDiffBytes,
			prGlueInfo->prP2PInfo->rNetDevStats.tx_packets, prGlueInfo->prP2PInfo->rNetDevStats.rx_packets);
	}
	prPerMonitor->ulLastTxBytes = latestTxBytes;
	prPerMonitor->ulLastRxBytes = latestRxBytes;
	prPerMonitor->ulP2PLastTxBytes = p2pLatestTxBytes;
	prPerMonitor->ulP2PLastRxBytes = p2pLatestRxBytes;

	if (prPerMonitor->ulThroughput < THROUGHPUT_L1_THRESHOLD) {
		if (prPerMonitor->u4CurrPerfLevel != 0 &&
			prPerMonitor->u1ShutdownCoreCount < THROUGHPUT_SHUTDOWN_CORE_COUNT) {
			prPerMonitor->u1ShutdownCoreCount++;
			DBGLOG(SW4, INFO, "kalPerMonHandler > u1ShutdownCoreCount: %d\n",
				prPerMonitor->u1ShutdownCoreCount);
				needBoostCpu = FALSE;
		}
		prPerMonitor->u4TarPerfLevel = 0;
	} else if (prPerMonitor->ulThroughput < THROUGHPUT_L2_THRESHOLD) {
		prPerMonitor->u4TarPerfLevel = 1;
	} else if (prPerMonitor->ulThroughput < THROUGHPUT_L3_THRESHOLD) {
		prPerMonitor->u4TarPerfLevel = 2;
	} else {
		prPerMonitor->u4TarPerfLevel = 3;
	}

	if (prPerMonitor->u4TarPerfLevel != 0)
		prPerMonitor->u1ShutdownCoreCount = 0;

	if (fgIsUnderSuspend ||
		wlan_fb_power_down ||
		!(netif_carrier_ok(prNetDev) ||
		(prP2pBssInfo->eConnectionState == PARAM_MEDIA_STATE_CONNECTED) ||
		(prP2pBssInfo->rStaRecOfClientList.u4NumElem > 0)))
		kalPerMonStop(prGlueInfo);
	else {
		DBGLOG(SW4, TRACE, "throughput:%ld bps\n", prPerMonitor->ulThroughput);
		if (needBoostCpu && (prPerMonitor->u4TarPerfLevel != prPerMonitor->u4CurrPerfLevel)) {
			/* if tar level = 0; core_number=prPerMonitor->u4TarPerfLevel+1*/
			if (prPerMonitor->u4TarPerfLevel)
				kalBoostCpu(prPerMonitor->u4TarPerfLevel+1);
			else
				kalBoostCpu(0);

			prPerMonitor->u4CurrPerfLevel = prPerMonitor->u4TarPerfLevel;
		}
		cnmTimerStartTimer(prGlueInfo->prAdapter, &prPerMonitor->rPerfMonTimer, prPerMonitor->u4UpdatePeriod);
	}

	DBGLOG(SW4, TRACE, "exit kalPerMonHandler\n");
}

INT32 __weak kalBoostCpu(UINT_32 core_num)
{
	DBGLOG(SW4, WARN, "enter weak kalBoostCpu, core_num:%d\n", core_num);
	return 0;
}

static int wlan_fb_notifier_callback(struct notifier_block *self, unsigned long event, void *data)
{
	struct fb_event *evdata = data;
	INT_32 blank;
	P_GLUE_INFO_T prGlueInfo = NULL;


	/* If we aren't interested in this event, skip it immediately ... */
	if (event != FB_EVENT_BLANK)
		return 0;

	if (kalHaltTryLock())
		return 0;

	prGlueInfo = (P_GLUE_INFO_T)wlan_fb_notifier_priv_data;
	blank = *(INT_32 *)evdata->data;

	switch (blank) {
	case FB_BLANK_UNBLANK:
		if (!kalIsHalted())
			kalPerMonEnable(prGlueInfo);
		wlan_fb_power_down = FALSE;
		break;
	case FB_BLANK_POWERDOWN:
		wlan_fb_power_down = TRUE;
		if (!kalIsHalted())
			kalPerMonDisable(prGlueInfo);
		break;
	default:
		break;
	}

	kalHaltUnlock();
	return 0;
}

INT_32 kalFbNotifierReg(IN P_GLUE_INFO_T prGlueInfo)
{
	INT_32 i4Ret;

	wlan_fb_notifier_priv_data = prGlueInfo;
	wlan_fb_notifier.notifier_call = wlan_fb_notifier_callback;
	i4Ret = fb_register_client(&wlan_fb_notifier);
	if (i4Ret)
		DBGLOG(SW4, WARN, "Register wlan_fb_notifier failed:%d\n", i4Ret);
	else
		DBGLOG(SW4, TRACE, "Register wlan_fb_notifier succeed\n");

	return i4Ret;
}

VOID kalFbNotifierUnReg(VOID)
{
	fb_unregister_client(&wlan_fb_notifier);
	wlan_fb_notifier_priv_data = NULL;
}
