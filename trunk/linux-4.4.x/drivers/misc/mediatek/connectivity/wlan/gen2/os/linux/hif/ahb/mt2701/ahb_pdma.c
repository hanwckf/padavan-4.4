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

#define MODULE_AHB_DMA

#include <linux/version.h>	/* constant of kernel version */

#include <linux/kernel.h>	/* bitops.h */

#include <linux/timer.h>	/* struct timer_list */
#include <linux/jiffies.h>	/* jiffies */
#include <linux/delay.h>	/* udelay and mdelay macro */

#if CONFIG_ANDROID
#include <linux/wakelock.h>
#endif

#include <linux/irq.h>		/* IRQT_FALLING */

#include <linux/netdevice.h>	/* struct net_device, struct net_device_stats */
#include <linux/etherdevice.h>	/* for eth_type_trans() function */
#include <linux/wireless.h>	/* struct iw_statistics */
#include <linux/if_arp.h>
#include <linux/inetdevice.h>	/* struct in_device */

#include <linux/ip.h>		/* struct iphdr */

#include <linux/string.h>	/* for memcpy()/memset() function */
#include <linux/stddef.h>	/* for offsetof() macro */

#include <linux/proc_fs.h>	/* The proc filesystem constants/structures */

#include <linux/rtnetlink.h>	/* for rtnl_lock() and rtnl_unlock() */
#include <linux/kthread.h>	/* kthread_should_stop(), kthread_run() */
#include <linux/uaccess.h>	/* for copy_from_user() */
#include <linux/fs.h>		/* for firmware download */
#include <linux/vmalloc.h>

#include <linux/kfifo.h>	/* for kfifo interface */
#include <linux/cdev.h>		/* for cdev interface */

#include <linux/firmware.h>	/* for firmware download */

#include <linux/random.h>

#include <linux/io.h>		/* readw and writew */

#include <linux/module.h>

/* #include "../../../../../../platform/mt6582/kernel/core/include/mach/mt_clkmgr.h" */
#include <mach/mt_clkmgr.h>

#include "hif.h"
#include "hif_pdma.h"
#include "gl_os.h"

#include <mach/emi_mpu.h>

/* #if (CONF_MTK_AHB_DMA == 1) */

/* #define PDMA_DEBUG_SUP */

#ifdef PDMA_DEBUG_SUP
#define PDMA_DBG	pr_debug
#else
#define PDMA_DBG(_fmt, ...)
#endif /* PDMA_DEBUG_SUP */

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/

/*******************************************************************************
*                             D A T A   T Y P E S
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
static VOID HifPdmaConfig(IN void *HifInfoSrc, IN void *Conf);

static VOID HifPdmaStart(IN void *HifInfoSrc);

static VOID HifPdmaStop(IN void *HifInfoSrc);

static MTK_WCN_BOOL HifPdmaPollStart(IN void *HifInfoSrc);

static MTK_WCN_BOOL HifPdmaPollIntr(IN void *HifInfoSrc);

static VOID HifPdmaAckIntr(IN void *HifInfoSrc);

static VOID HifPdmaClockCtrl(IN UINT32 FlgIsEnabled);

static VOID HifPdmaRegDump(IN void *HifInfoSrc);

static VOID HifPdmaReset(IN void *HifInfoSrc);

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/
GL_HIF_DMA_OPS_T HifPdmaOps = {
	.DmaConfig = HifPdmaConfig,
	.DmaStart = HifPdmaStart,
	.DmaStop = HifPdmaStop,
	.DmaPollStart = HifPdmaPollStart,
	.DmaPollIntr = HifPdmaPollIntr,
	.DmaAckIntr = HifPdmaAckIntr,
	.DmaClockCtrl = HifPdmaClockCtrl,
	.DmaRegDump = HifPdmaRegDump,
	.DmaReset = HifPdmaReset
};

/*******************************************************************************
*                        P U B L I C   F U N C T I O N S
********************************************************************************
*/

/*----------------------------------------------------------------------------*/
/*!
* \brief Config PDMA TX/RX.
*
* \param[in] prGlueInfo         Pointer to the GLUE_INFO_T structure.
* \param[in] Conf               Pointer to the settings.
*
* \retval NONE
*/
/*----------------------------------------------------------------------------*/
VOID HifPdmaInit(GL_HIF_INFO_T *HifInfo)
{
	/* IO remap PDMA register memory */
	HifInfo->DmaRegBaseAddr = ioremap(AP_DMA_HIF_BASE, AP_DMA_HIF_0_LENGTH);

	/* assign PDMA operators */
	HifInfo->DmaOps = &HifPdmaOps;

	/* enable PDMA mode */
	HifInfo->fgDmaEnable = TRUE;

	/* Set EMI protection here */
#ifdef MTK_TEE_CCCI_SECURE_SHARE_MEM_SUPPORT
	DBGLOG(INIT, INFO, "WIFI set EMI MPU for TEE project\n");
	emi_mpu_set_region_protection(gConEmiPhyBase,
				      gConEmiPhyBase + SZ_1M / 2,
				      5, SET_ACCESS_PERMISSON(FORBIDDEN, NO_PROTECTION, FORBIDDEN, FORBIDDEN));
#else
	DBGLOG(INIT, INFO, "WIFI set EMI MPU for non-TEE project\n");
	emi_mpu_set_region_protection(gConEmiPhyBase,
				      gConEmiPhyBase + SZ_1M / 2,
				      4, SET_ACCESS_PERMISSON(FORBIDDEN, NO_PROTECTION, FORBIDDEN, FORBIDDEN));
#endif

	DBGLOG(INIT, INFO, "PDMA> HifPdmaInit ok!\n");
}

/*******************************************************************************
*                       P R I V A T E   F U N C T I O N S
********************************************************************************
*/

/*----------------------------------------------------------------------------*/
/*!
* \brief Config PDMA TX/RX.
*
* \param[in] HifInfo            Pointer to the GL_HIF_INFO_T structure.
* \param[in] Param              Pointer to the settings.
*
* \retval NONE
*/
/*----------------------------------------------------------------------------*/
static VOID HifPdmaConfig(IN void *HifInfoSrc, IN void *Param)
{
	GL_HIF_INFO_T *HifInfo = (GL_HIF_INFO_T *) HifInfoSrc;
	MTK_WCN_HIF_DMA_CONF *Conf = (MTK_WCN_HIF_DMA_CONF *) Param;
	UINT32 RegVal;

	/* Assign fixed value */
	Conf->Burst = HIF_PDMA_BURST_4_4;	/* vs. HIF_BURST_4DW */
	Conf->Fix_en = FALSE;

	/* AP_P_DMA_G_DMA_2_CON */
	PDMA_DBG("PDMA> Conf->Dir = %d\n", Conf->Dir);

	/* AP_DMA_HIF_0_CON */
	RegVal = HIF_DMAR_READL(HifInfo, AP_DMA_HIF_0_CON);
	RegVal &= ~(ADH_CR_BURST_LEN | ADH_CR_FIX_EN | ADH_CR_DIR);
	RegVal |= (((Conf->Burst << ADH_CR_BURST_LEN_OFFSET) & ADH_CR_BURST_LEN) |
		   (Conf->Fix_en << ADH_CR_FIX_EN_OFFSET) | (Conf->Dir));
	HIF_DMAR_WRITEL(HifInfo, AP_DMA_HIF_0_CON, RegVal);
	PDMA_DBG("PDMA> AP_DMA_HIF_0_CON = 0x%08x\n", RegVal);

	/* AP_DMA_HIF_0_SRC_ADDR */
	HIF_DMAR_WRITEL(HifInfo, AP_DMA_HIF_0_SRC_ADDR, Conf->Src);
	PDMA_DBG("PDMA> AP_DMA_HIF_0_SRC_ADDR = 0x%08lx\n", Conf->Src);

	/* AP_DMA_HIF_0_DST_ADDR */
	HIF_DMAR_WRITEL(HifInfo, AP_DMA_HIF_0_DST_ADDR, Conf->Dst);
	PDMA_DBG("PDMA> AP_DMA_HIF_0_DST_ADDR = 0x%08lx\n", Conf->Dst);

	/* AP_DMA_HIF_0_LEN */
	HIF_DMAR_WRITEL(HifInfo, AP_DMA_HIF_0_LEN, (Conf->Count & ADH_CR_LEN));
	PDMA_DBG("PDMA> AP_DMA_HIF_0_LEN = %u\n", (UINT_32)(Conf->Count & ADH_CR_LEN));

}				/* End of HifPdmaConfig */

/*----------------------------------------------------------------------------*/
/*!
* \brief Start PDMA TX/RX.
*
* \param[in] HifInfo            Pointer to the GL_HIF_INFO_T structure.
*
* \retval NONE
*/
/*----------------------------------------------------------------------------*/
static VOID HifPdmaStart(IN void *HifInfoSrc)
{
	GL_HIF_INFO_T *HifInfo = (GL_HIF_INFO_T *) HifInfoSrc;
	UINT32 RegVal;

	/* Enable interrupt */
	RegVal = HIF_DMAR_READL(HifInfo, AP_DMA_HIF_0_INT_EN);
	HIF_DMAR_WRITEL(HifInfo, AP_DMA_HIF_0_INT_EN, (RegVal | ADH_CR_INTEN_FLAG_0));

	/* Start DMA */
	RegVal = HIF_DMAR_READL(HifInfo, AP_DMA_HIF_0_EN);
	HIF_DMAR_WRITEL(HifInfo, AP_DMA_HIF_0_EN, (RegVal | ADH_CR_EN));

	PDMA_DBG("PDMA> HifPdmaStart...\n");

}				/* End of HifPdmaStart */

/*----------------------------------------------------------------------------*/
/*!
* \brief Stop PDMA TX/RX.
*
* \param[in] HifInfo            Pointer to the GL_HIF_INFO_T structure.
*
* \retval NONE
*/
/*----------------------------------------------------------------------------*/
static VOID HifPdmaStop(IN void *HifInfoSrc)
{
	GL_HIF_INFO_T *HifInfo = (GL_HIF_INFO_T *) HifInfoSrc;
	UINT32 RegVal;
/* UINT32 pollcnt; */

	/* Disable interrupt */
	RegVal = HIF_DMAR_READL(HifInfo, AP_DMA_HIF_0_INT_EN);
	HIF_DMAR_WRITEL(HifInfo, AP_DMA_HIF_0_INT_EN, (RegVal & ~(ADH_CR_INTEN_FLAG_0)));

#if 0				/* DE says we donot need to do it */
	/* Stop DMA */
	RegVal = HIF_DMAR_READL(HifInfo, AP_DMA_HIF_0_STOP);
	HIF_DMAR_WRITEL(HifInfo, AP_DMA_HIF_0_STOP, (RegVal | ADH_CR_STOP));

	/* Polling START bit turn to 0 */
	pollcnt = 0;
	do {
		RegVal = HIF_DMAR_READL(HifInfo, AP_DMA_HIF_0_EN);
		if (pollcnt++ > 100000)
			; /* TODO: warm reset PDMA */
	} while (RegVal & ADH_CR_EN);
#endif

}				/* End of HifPdmaStop */

/*----------------------------------------------------------------------------*/
/*!
* \brief Enable PDMA TX/RX.
*
* \param[in] HifInfo            Pointer to the GL_HIF_INFO_T structure.
*
* \retval NONE
*/
/*----------------------------------------------------------------------------*/
static MTK_WCN_BOOL HifPdmaPollStart(IN void *HifInfoSrc)
{
	GL_HIF_INFO_T *HifInfo = (GL_HIF_INFO_T *) HifInfoSrc;
	UINT32 RegVal;

	RegVal = HIF_DMAR_READL(HifInfo, AP_DMA_HIF_0_EN);
	return ((RegVal & ADH_CR_EN) != 0) ? TRUE : FALSE;

}				/* End of HifPdmaPollStart */

/*----------------------------------------------------------------------------*/
/*!
* \brief Poll PDMA TX/RX done.
*
* \param[in] HifInfo            Pointer to the GL_HIF_INFO_T structure.
*
* \retval NONE
*/
/*----------------------------------------------------------------------------*/
static MTK_WCN_BOOL HifPdmaPollIntr(IN void *HifInfoSrc)
{
	GL_HIF_INFO_T *HifInfo = (GL_HIF_INFO_T *) HifInfoSrc;
	UINT32 RegVal;

	RegVal = HIF_DMAR_READL(HifInfo, AP_DMA_HIF_0_INT_FLAG);
	return ((RegVal & ADH_CR_FLAG_0) != 0) ? TRUE : FALSE;

}				/* End of HifPdmaPollIntr */

/*----------------------------------------------------------------------------*/
/*!
* \brief Acknowledge PDMA TX/RX done.
*
* \param[in] HifInfo            Pointer to the GL_HIF_INFO_T structure.
*
* \retval NONE
*/
/*----------------------------------------------------------------------------*/
static VOID HifPdmaAckIntr(IN void *HifInfoSrc)
{
	GL_HIF_INFO_T *HifInfo = (GL_HIF_INFO_T *) HifInfoSrc;
	UINT32 RegVal;

	/* Write 0 to clear interrupt */
	RegVal = HIF_DMAR_READL(HifInfo, AP_DMA_HIF_0_INT_FLAG);
	HIF_DMAR_WRITEL(HifInfo, AP_DMA_HIF_0_INT_FLAG, (RegVal & ~ADH_CR_FLAG_0));

}				/* End of HifPdmaAckIntr */

/*----------------------------------------------------------------------------*/
/*!
* \brief Acknowledge PDMA TX/RX done.
*
* \param[in] FlgIsEnabled       TRUE: enable; FALSE: disable
*
* \retval NONE
*/
/*----------------------------------------------------------------------------*/
static VOID HifPdmaClockCtrl(IN UINT32 FlgIsEnabled)
{
	if (FlgIsEnabled == TRUE)
		enable_clock(MT_CG_PERI_AP_DMA, "WLAN");
	else
		disable_clock(MT_CG_PERI_AP_DMA, "WLAN");
}

/*----------------------------------------------------------------------------*/
/*!
* \brief Dump PDMA related registers.
*
* \param[in] HifInfo            Pointer to the GL_HIF_INFO_T structure.
*
* \retval NONE
*/
/*----------------------------------------------------------------------------*/
static VOID HifPdmaRegDump(IN void *HifInfoSrc)
{
	GL_HIF_INFO_T *HifInfo = (GL_HIF_INFO_T *) HifInfoSrc;
	UINT32 RegId, RegVal;
	UINT32 RegNum = 0;

	DBGLOG(INIT, INFO, "PDMA> Register content 0x%x=\n\t", AP_DMA_HIF_BASE);
	for (RegId = 0; RegId < AP_DMA_HIF_0_LENGTH; RegId += 4) {
		RegVal = HIF_DMAR_READL(HifInfo, RegId);
		DBGLOG(INIT, INFO, "0x%08x ", RegVal);

		if (RegNum++ >= 3) {
			DBGLOG(INIT, INFO, "\n");
			DBGLOG(INIT, INFO, "PDMA> Register content 0x%x=\n\t", AP_DMA_HIF_BASE + RegId + 4);
			RegNum = 0;
		}
	}

	DBGLOG(INIT, INFO, "\nPDMA> clock status = 0x%x\n\n", *(volatile unsigned int *)0xF0003018);
}

/*----------------------------------------------------------------------------*/
/*!
* \brief Reset DMA.
*
* \param[in] HifInfo            Pointer to the GL_HIF_INFO_T structure.
*
* \retval NONE
*/
/*----------------------------------------------------------------------------*/
static VOID HifPdmaReset(IN void *HifInfoSrc)
{
	GL_HIF_INFO_T *HifInfo = (GL_HIF_INFO_T *) HifInfoSrc;
	UINT32 LoopCnt;

	/* do warm reset: DMA will wait for current traction finished */
	DBGLOG(INIT, INFO, "\nDMA> do warm reset...\n");

	/* normally, we need to sure that bit0 of AP_P_DMA_G_DMA_2_EN is 1 here */

	HIF_DMAR_WRITEL(HifInfo, AP_DMA_HIF_0_RST, 0x01);

	for (LoopCnt = 0; LoopCnt < 10000; LoopCnt++) {
		if (!HifPdmaPollStart(HifInfo))
			break;	/* reset ok */
	}

	if (HifPdmaPollStart(HifInfo)) {
		/* do hard reset because warm reset fails */
		DBGLOG(INIT, INFO, "\nDMA> do hard reset...\n");
		HIF_DMAR_WRITEL(HifInfo, AP_DMA_HIF_0_RST, 0x02);
		mdelay(1);
		HIF_DMAR_WRITEL(HifInfo, AP_DMA_HIF_0_RST, 0x00);
	}
}

/* #endif */ /* CONF_MTK_AHB_DMA */

/* End of ahb_pdma.c */
