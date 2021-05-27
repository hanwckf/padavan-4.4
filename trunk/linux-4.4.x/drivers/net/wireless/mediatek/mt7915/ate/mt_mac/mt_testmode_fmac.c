/*
 ***************************************************************************
 * MediaTek Inc.
 *
 * All rights reserved. source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of MediaTek. Any attemp
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of MediaTek, Inc. is obtained.
 ***************************************************************************
	Module Name:
	mt_testmode_fmac.c

*/

#ifdef COMPOS_TESTMODE_WIN
#include "config.h"
#else
#include "rt_config.h"
#endif
#include "chip/mt7915_coda/bn0_wf_agg_top.h"
#include "chip/mt7915_coda/bn1_wf_agg_top.h"
#include "chip/mt7915_coda/bn0_wf_tmac_top.h"
#include "chip/mt7915_coda/bn1_wf_tmac_top.h"
#include "chip/mt7915_coda/bn0_wf_arb_top.h"
#include "chip/mt7915_coda/bn1_wf_arb_top.h"

INT32 mtf_ate_ipg_cr_restore(RTMP_ADAPTER *ad, UCHAR band_idx)
{
	if (band_idx == TESTMODE_BAND0) {
		MtTestModeRestoreCr(ad, BN0_WF_AGG_TOP_ATCR1_ADDR);
		MtTestModeRestoreCr(ad, BN0_WF_AGG_TOP_ATCR3_ADDR);
		MtTestModeRestoreCr(ad, BN0_WF_TMAC_TOP_TRCR0_ADDR);
		MtTestModeRestoreCr(ad, BN0_WF_TMAC_TOP_ICR0_ADDR);	/* IFS CR, for SIFS/SLOT time control */
		MtTestModeRestoreCr(ad, BN0_WF_ARB_TOP_DRNGR0_ADDR);	/* For fixing backoff random number ARB_DRNGR0 */
		MtTestModeRestoreCr(ad, BN0_WF_ARB_TOP_DRNGR1_ADDR);	/* For fixing backoff random number ARB_DRNGR1 */
	}
#ifdef DBDC_MODE
	else {
			MtTestModeRestoreCr(ad, BN1_WF_AGG_TOP_ATCR1_ADDR);
			MtTestModeRestoreCr(ad, BN1_WF_AGG_TOP_ATCR3_ADDR);
			MtTestModeRestoreCr(ad, BN1_WF_TMAC_TOP_TRCR0_ADDR);
			MtTestModeRestoreCr(ad, BN1_WF_TMAC_TOP_ICR0_ADDR);	/* IFS CR, for SIFS/SLOT time control */
			MtTestModeRestoreCr(ad, BN1_WF_ARB_TOP_DRNGR0_ADDR);	/* For fixing backoff random number ARB_DRNGR0 */
			MtTestModeRestoreCr(ad, BN1_WF_ARB_TOP_DRNGR1_ADDR);	/* For fixing backoff random number ARB_DRNGR1 */
	}
#endif /* DBDC_MODE */

	return 0;
}

INT32 mtf_ate_mac_cr_restore(RTMP_ADAPTER *ad)
{
	if (IS_AXE(ad) || IS_MT7915(ad)) {
		/* Data frame protection CR recover */
		MtTestModeRestoreCr(ad, BN0_WF_AGG_TOP_PCR0_ADDR);
		MtTestModeRestoreCr(ad, BN0_WF_AGG_TOP_PCR1_ADDR);
		/* BA related CR recover */
		MtTestModeRestoreCr(ad, BN0_WF_AGG_TOP_AWSCR0_ADDR);
		MtTestModeRestoreCr(ad, BN0_WF_AGG_TOP_AWSCR1_ADDR);
		MtTestModeRestoreCr(ad, BN0_WF_AGG_TOP_AWSCR2_ADDR);
		MtTestModeRestoreCr(ad, BN0_WF_AGG_TOP_AWSCR3_ADDR);
		/* Enable HW BAR feature */
		MtTestModeRestoreCr(ad, BN0_WF_AGG_TOP_MRCR_ADDR);
		/* TCR */
		MtTestModeRestoreCr(ad, BN0_WF_TMAC_TOP_TCR0_ADDR);
		/* TFCR0 */
		MtTestModeRestoreCr(ad, BN0_WF_TMAC_TOP_TFCR0_ADDR);
		/* IPG related CRs resotre */
		MtTestModeRestoreCr(ad, BN0_WF_AGG_TOP_ATCR1_ADDR);
		MtTestModeRestoreCr(ad, BN0_WF_AGG_TOP_ATCR3_ADDR);
		MtTestModeRestoreCr(ad, BN0_WF_TMAC_TOP_TRCR0_ADDR);
		MtTestModeRestoreCr(ad, BN0_WF_TMAC_TOP_ICR0_ADDR);	/* IFS CR, for SIFS/SLOT time control */
		MtTestModeRestoreCr(ad, BN0_WF_ARB_TOP_DRNGR0_ADDR);	/* For fixing backoff random number ARB_DRNGR0 */
		MtTestModeRestoreCr(ad, BN0_WF_ARB_TOP_DRNGR1_ADDR);	/* For fixing backoff random number ARB_DRNGR1 */
#ifdef DBDC_MODE
		if (IS_ATE_DBDC(ad)) {
			/* Data frame protection CR recover */
			MtTestModeRestoreCr(ad, BN1_WF_AGG_TOP_PCR0_ADDR);
			MtTestModeRestoreCr(ad, BN1_WF_AGG_TOP_PCR1_ADDR);
			/* BA related CR recover */
			MtTestModeRestoreCr(ad, BN1_WF_AGG_TOP_AWSCR0_ADDR);
			MtTestModeRestoreCr(ad, BN1_WF_AGG_TOP_AWSCR1_ADDR);
			MtTestModeRestoreCr(ad, BN1_WF_AGG_TOP_AWSCR2_ADDR);
			MtTestModeRestoreCr(ad, BN1_WF_AGG_TOP_AWSCR3_ADDR);
			/* Enable HW BAR feature */
			MtTestModeRestoreCr(ad, BN1_WF_AGG_TOP_MRCR_ADDR);
			/* TCR */
			MtTestModeRestoreCr(ad, BN1_WF_TMAC_TOP_TCR0_ADDR);
			/* TFCR0 */
			MtTestModeRestoreCr(ad, BN1_WF_TMAC_TOP_TFCR0_ADDR);
			/* IPG related CRs resotre */
			MtTestModeRestoreCr(ad, BN1_WF_AGG_TOP_ATCR1_ADDR);
			MtTestModeRestoreCr(ad, BN1_WF_AGG_TOP_ATCR3_ADDR);
			MtTestModeRestoreCr(ad, BN1_WF_TMAC_TOP_TRCR0_ADDR);
			MtTestModeRestoreCr(ad, BN1_WF_TMAC_TOP_ICR0_ADDR);	/* IFS CR, for SIFS/SLOT time control */
			MtTestModeRestoreCr(ad, BN1_WF_ARB_TOP_DRNGR0_ADDR);	/* For fixing backoff random number ARB_DRNGR0 */
			MtTestModeRestoreCr(ad, BN1_WF_ARB_TOP_DRNGR1_ADDR);	/* For fixing backoff random number ARB_DRNGR1 */
		}
#endif /* DBDC_MODE */
	}

	return 0;
}

INT32 mtf_ate_mac_cr_backup_and_set(RTMP_ADAPTER *ad)
{
	struct _ATE_CTRL *ate_ctrl = &ad->ATECtrl;
	NdisZeroMemory(&ate_ctrl->bk_cr, sizeof(struct _TESTMODE_BK_CR)*MAX_TEST_BKCR_NUM);

	if (IS_MT7915(ad)) {
		UINT32 val = 0;
		/* Disable data frame protection for test mode */
		MtTestModeBkCr(ad, BN0_WF_AGG_TOP_PCR0_ADDR, TEST_MAC_BKCR);
		MAC_IO_READ32(ad->hdev_ctrl, BN0_WF_AGG_TOP_PCR0_ADDR, &val);
		val &= ~(BN0_WF_AGG_TOP_PCR0_MM_PROTECTION0_MASK | BN0_WF_AGG_TOP_PCR0_GF_PROTECTION0_MASK | BN0_WF_AGG_TOP_PCR0_ERP_PROTECTION0_MASK | BN0_WF_AGG_TOP_PCR0_VHT_PROTECTION0_MASK);
		val &= ~(BN0_WF_AGG_TOP_PCR0_BW20_PROTECTION0_MASK | BN0_WF_AGG_TOP_PCR0_BW40_PROTECTION0_MASK | BN0_WF_AGG_TOP_PCR0_BW80_PROTECTION0_MASK);
		val |= BN0_WF_AGG_TOP_PCR0_PROTECTION_DIS_IN_PTA_WIN0_MASK;
		MAC_IO_WRITE32(ad->hdev_ctrl, BN0_WF_AGG_TOP_PCR0_ADDR, val);

		MtTestModeBkCr(ad, BN0_WF_AGG_TOP_PCR1_ADDR, TEST_MAC_BKCR);
		MAC_IO_READ32(ad->hdev_ctrl, BN0_WF_AGG_TOP_PCR1_ADDR, &val);
		/* Setting RTS length/numb threshold to max value to aviod send RTS in test mode */
		val = (BN0_WF_AGG_TOP_PCR1_RTS0_PKT_NUM_THRESHOLD_MASK | BN0_WF_AGG_TOP_PCR1_RTS0_PKT_LEN_THRESHOLD_MASK);
		MAC_IO_WRITE32(ad->hdev_ctrl, BN0_WF_AGG_TOP_PCR1_ADDR, val);
		/* BA related CR backup */
		MtTestModeBkCr(ad, BN0_WF_AGG_TOP_AWSCR0_ADDR, TEST_MAC_BKCR);
		MtTestModeBkCr(ad, BN0_WF_AGG_TOP_AWSCR1_ADDR, TEST_MAC_BKCR);
		MtTestModeBkCr(ad, BN0_WF_AGG_TOP_AWSCR2_ADDR, TEST_MAC_BKCR);
		MtTestModeBkCr(ad, BN0_WF_AGG_TOP_AWSCR3_ADDR, TEST_MAC_BKCR);
		/* HW BAR feature */
		MtTestModeBkCr(ad, BN0_WF_AGG_TOP_MRCR_ADDR, TEST_MAC_BKCR);
		MAC_IO_READ32(ad->hdev_ctrl, BN0_WF_AGG_TOP_MRCR_ADDR, &val);
		val &= ~(BN0_WF_AGG_TOP_MRCR_BAR_TX_CNT_LIMIT_MASK);
		val &= ~(BN0_WF_AGG_TOP_MRCR_LAST_RTS_AS_CTS_EN_MASK);
		val &= ~(BN0_WF_AGG_TOP_MRCR_RTS_FAIL_CNT_LIMIT_MASK | BN0_WF_AGG_TOP_MRCR_TXCMD_RTS_FAIL_CNT_LIMIT_MASK);
		val |= ((0x1 << BN0_WF_AGG_TOP_MRCR_RTS_FAIL_CNT_LIMIT_SHFT) | (0x1 << BN0_WF_AGG_TOP_MRCR_TXCMD_RTS_FAIL_CNT_LIMIT_SHFT));
		MAC_IO_WRITE32(ad->hdev_ctrl, BN0_WF_AGG_TOP_MRCR_ADDR, val);
		/* TFCR */
		MtTestModeBkCr(ad, BN0_WF_TMAC_TOP_TFCR0_ADDR, TEST_MAC_BKCR);
		MAC_IO_WRITE32(ad->hdev_ctrl, BN0_WF_TMAC_TOP_TFCR0_ADDR, 0);
		/* TCR */
		MtTestModeBkCr(ad, BN0_WF_TMAC_TOP_TCR0_ADDR, TEST_MAC_BKCR);
		MAC_IO_READ32(ad->hdev_ctrl, BN0_WF_TMAC_TOP_TCR0_ADDR, &val);
		val &= ~(BN0_WF_TMAC_TOP_TCR0_TBTT_TX_STOP_CONTROL_MASK);
		MAC_IO_WRITE32(ad->hdev_ctrl, BN0_WF_TMAC_TOP_TCR0_ADDR, val);

		/* IPG related CR back up */
		MtTestModeBkCr(ad, BN0_WF_AGG_TOP_ATCR1_ADDR, TEST_MAC_BKCR);
		MtTestModeBkCr(ad, BN0_WF_AGG_TOP_ATCR3_ADDR, TEST_MAC_BKCR);
		MtTestModeBkCr(ad, BN0_WF_TMAC_TOP_TRCR0_ADDR, TEST_MAC_BKCR);
		MtTestModeBkCr(ad, BN0_WF_TMAC_TOP_ICR0_ADDR, TEST_MAC_BKCR);	/* IFS CR, for SIFS/SLOT time control */
		MtTestModeBkCr(ad, BN0_WF_ARB_TOP_DRNGR0_ADDR, TEST_MAC_BKCR);	/* For fixing backoff random number ARB_DRNGR0 */
		MtTestModeBkCr(ad, BN0_WF_ARB_TOP_DRNGR1_ADDR, TEST_MAC_BKCR);	/* For fixing backoff random number ARB_DRNGR1 */
#ifdef DBDC_MODE
		/* RTS threshold disable for band1 */
		if (IS_ATE_DBDC(ad)) {
			/* Disable data frame protection for test mode */
			MtTestModeBkCr(ad, BN1_WF_AGG_TOP_PCR0_ADDR, TEST_MAC_BKCR);
			MAC_IO_READ32(ad->hdev_ctrl, BN1_WF_AGG_TOP_PCR0_ADDR, &val);
			val &= ~(BN1_WF_AGG_TOP_PCR0_MM_PROTECTION0_MASK | BN1_WF_AGG_TOP_PCR0_GF_PROTECTION0_MASK | BN1_WF_AGG_TOP_PCR0_ERP_PROTECTION0_MASK | BN1_WF_AGG_TOP_PCR0_VHT_PROTECTION0_MASK);
			val &= ~(BN1_WF_AGG_TOP_PCR0_BW20_PROTECTION0_MASK | BN1_WF_AGG_TOP_PCR0_BW40_PROTECTION0_MASK | BN1_WF_AGG_TOP_PCR0_BW80_PROTECTION0_MASK);
			val |= BN1_WF_AGG_TOP_PCR0_PROTECTION_DIS_IN_PTA_WIN0_MASK;
			MAC_IO_WRITE32(ad->hdev_ctrl, BN1_WF_AGG_TOP_PCR0_ADDR, val);

			MtTestModeBkCr(ad, BN1_WF_AGG_TOP_PCR1_ADDR, TEST_MAC_BKCR);
			/* Setting RTS length/numb threshold to max value to aviod send RTS in test mode */
			val = (BN1_WF_AGG_TOP_PCR1_RTS0_PKT_NUM_THRESHOLD_MASK | BN1_WF_AGG_TOP_PCR1_RTS0_PKT_LEN_THRESHOLD_MASK);
			MAC_IO_WRITE32(ad->hdev_ctrl, BN1_WF_AGG_TOP_PCR1_ADDR, val);
			/* BA related CR backup */
			MtTestModeBkCr(ad, BN1_WF_AGG_TOP_AWSCR0_ADDR, TEST_MAC_BKCR);
			MtTestModeBkCr(ad, BN1_WF_AGG_TOP_AWSCR1_ADDR, TEST_MAC_BKCR);
			MtTestModeBkCr(ad, BN1_WF_AGG_TOP_AWSCR2_ADDR, TEST_MAC_BKCR);
			MtTestModeBkCr(ad, BN1_WF_AGG_TOP_AWSCR3_ADDR, TEST_MAC_BKCR);
			/* HW BAR feature */
			MtTestModeBkCr(ad, BN1_WF_AGG_TOP_MRCR_ADDR, TEST_MAC_BKCR);
			MAC_IO_READ32(ad->hdev_ctrl, BN1_WF_AGG_TOP_MRCR_ADDR, &val);
			val &= ~(BN1_WF_AGG_TOP_MRCR_BAR_TX_CNT_LIMIT_MASK);
			val &= ~(BN1_WF_AGG_TOP_MRCR_LAST_RTS_AS_CTS_EN_MASK);
			val &= ~(BN1_WF_AGG_TOP_MRCR_RTS_FAIL_CNT_LIMIT_MASK | BN0_WF_AGG_TOP_MRCR_TXCMD_RTS_FAIL_CNT_LIMIT_MASK);
			val |= ((0x1 << BN1_WF_AGG_TOP_MRCR_RTS_FAIL_CNT_LIMIT_SHFT) | (0x1 << BN1_WF_AGG_TOP_MRCR_TXCMD_RTS_FAIL_CNT_LIMIT_SHFT));
			MAC_IO_WRITE32(ad->hdev_ctrl, BN1_WF_AGG_TOP_MRCR_ADDR, val);
			/* TFCR */
			MtTestModeBkCr(ad, BN1_WF_TMAC_TOP_TFCR0_ADDR, TEST_MAC_BKCR);
			MAC_IO_WRITE32(ad->hdev_ctrl, BN1_WF_TMAC_TOP_TFCR0_ADDR, 0);
			/* TCR */
			MtTestModeBkCr(ad, BN1_WF_TMAC_TOP_TCR0_ADDR, TEST_MAC_BKCR);
			MAC_IO_READ32(ad->hdev_ctrl, BN1_WF_TMAC_TOP_TCR0_ADDR, &val);
			val &= ~(BN1_WF_TMAC_TOP_TCR0_TBTT_TX_STOP_CONTROL_MASK);
			MAC_IO_WRITE32(ad->hdev_ctrl, BN1_WF_TMAC_TOP_TCR0_ADDR, val);
			/* IPG related CR back up */
			MtTestModeBkCr(ad, BN1_WF_AGG_TOP_ATCR1_ADDR, TEST_MAC_BKCR);
			MtTestModeBkCr(ad, BN1_WF_AGG_TOP_ATCR3_ADDR, TEST_MAC_BKCR);
			MtTestModeBkCr(ad, BN1_WF_TMAC_TOP_TRCR0_ADDR, TEST_MAC_BKCR);
			MtTestModeBkCr(ad, BN1_WF_TMAC_TOP_ICR0_ADDR, TEST_MAC_BKCR);	/* IFS CR, for SIFS/SLOT time control */
			MtTestModeBkCr(ad, BN1_WF_ARB_TOP_DRNGR0_ADDR, TEST_MAC_BKCR);	/* For fixing backoff random number ARB_DRNGR0 */
			MtTestModeBkCr(ad, BN1_WF_ARB_TOP_DRNGR1_ADDR, TEST_MAC_BKCR);	/* For fixing backoff random number ARB_DRNGR1 */
		}
#endif /* DBDC_MODE */



#ifdef DBDC_MODE
		if (IS_ATE_DBDC(ad)) {
			MtTestModeBkCr(ad, BN1_WF_TMAC_TOP_TRCR0_ADDR, TEST_MAC_BKCR);
			MtTestModeBkCr(ad, BN1_WF_TMAC_TOP_ICR0_ADDR, TEST_MAC_BKCR);
			/* For fixing backoff random number ARB_DRNGR0/ARB_DRNGR0 */
			MtTestModeBkCr(ad, BN1_WF_ARB_TOP_DRNGR0_ADDR, TEST_MAC_BKCR);
			MtTestModeBkCr(ad, BN1_WF_ARB_TOP_DRNGR1_ADDR, TEST_MAC_BKCR);
		}
#endif /* DBDC_MODE */
	}

	return 0;
}

UINT32 bn0_agg_cnt_array[] = {BN0_WF_AGG_TOP_AALCR0_ADDR, BN0_WF_AGG_TOP_AALCR1_ADDR, BN0_WF_AGG_TOP_AALCR2_ADDR, BN0_WF_AGG_TOP_AALCR3_ADDR};
UINT32 bn1_agg_cnt_array[] = {BN1_WF_AGG_TOP_AALCR0_ADDR, BN1_WF_AGG_TOP_AALCR1_ADDR, BN1_WF_AGG_TOP_AALCR2_ADDR, BN1_WF_AGG_TOP_AALCR3_ADDR};

INT32 mtf_ate_ampdu_ba_limit(RTMP_ADAPTER *ad, UINT8 wmm_idx, UINT8 agg_limit)
{
	UINT32 value;

	if (wmm_idx > 3) {
		MTWF_LOG(DBG_CAT_TEST, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			("%s: invalid WmmIdx=%d, set to all!\n",
			__func__, wmm_idx));
		wmm_idx = 0xFF;
	}

	MTWF_LOG(DBG_CAT_TEST, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
		("%s: WmmIdx=%d\n", __func__, wmm_idx));
	value = ((agg_limit & 0xFF) << 24)
		| ((agg_limit & 0xFF) << 16)
		| ((agg_limit & 0xFF) << 8)
		| ((agg_limit & 0xFF) << 0);

	if (wmm_idx <= 3)
		MAC_IO_WRITE32(ad->hdev_ctrl, bn0_agg_cnt_array[wmm_idx], value);
	else {
		MAC_IO_WRITE32(ad->hdev_ctrl, BN0_WF_AGG_TOP_AALCR0_ADDR, value);
		MAC_IO_WRITE32(ad->hdev_ctrl, BN0_WF_AGG_TOP_AALCR1_ADDR, value);
		MAC_IO_WRITE32(ad->hdev_ctrl, BN0_WF_AGG_TOP_AALCR2_ADDR, value);
		MAC_IO_WRITE32(ad->hdev_ctrl, BN0_WF_AGG_TOP_AALCR3_ADDR, value);
	}

	value = 0x0;
	MAC_IO_WRITE32(ad->hdev_ctrl, BN0_WF_AGG_TOP_AWSCR0_ADDR, value);
	MAC_IO_WRITE32(ad->hdev_ctrl, BN0_WF_AGG_TOP_AWSCR1_ADDR, value);
	MAC_IO_WRITE32(ad->hdev_ctrl, BN0_WF_AGG_TOP_AWSCR2_ADDR, value);
	MAC_IO_WRITE32(ad->hdev_ctrl, BN0_WF_AGG_TOP_AWSCR3_ADDR, value);

	return 0;
}

INT32 mtf_ate_set_sta_pause_cr(RTMP_ADAPTER *ad, UINT8 ac_idx)
{
	INT32 ret = 0;
	return ret;
}

INT32 mtf_ate_set_ifs_cr(RTMP_ADAPTER *pAd, UINT8 band_idx)
{
	INT32 ret = 0;
	struct _ATE_IPG_PARAM *ipg_param = NULL;
	UINT16 slot_time, sifs_time;
	UINT32 txv_time = 0, i2t_chk_time = 0, tr2t_chk_time = 0;
	UINT32 value = 0;

	ipg_param = (struct _ATE_IPG_PARAM *)TESTMODE_GET_PADDR(pAd, TESTMODE_GET_BAND_IDX(pAd), ipg_param);

	slot_time = ipg_param->slot_time;
	sifs_time = ipg_param->sifs_time;
	/* in uint of ns */
	MAC_IO_READ32(pAd->hdev_ctrl, BN0_WF_TMAC_TOP_ATCR_ADDR, &txv_time);
	txv_time &= BN0_WF_TMAC_TOP_ATCR_TXV_TOUT_MASK;
	txv_time *= NORMAL_CLOCK_TIME;

	i2t_chk_time = (UINT32)(slot_time * 1000) - txv_time - BBP_PROCESSING_TIME;
	tr2t_chk_time = (UINT32)(sifs_time * 1000) - txv_time - BBP_PROCESSING_TIME;
	MTWF_LOG(DBG_CAT_TEST, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
		("%s: txv_time=%dns, i2t_chk_time=%dns, tr2t_chk_time=%dns\n",
		__func__, txv_time, i2t_chk_time, tr2t_chk_time));

	i2t_chk_time /= NORMAL_CLOCK_TIME;
	tr2t_chk_time /= NORMAL_CLOCK_TIME;

	if (band_idx == TESTMODE_BAND0) {
		MAC_IO_READ32(pAd->hdev_ctrl, BN0_WF_TMAC_TOP_TRCR0_ADDR, &value);
		value |= ((tr2t_chk_time << BN0_WF_TMAC_TOP_TRCR0_TR2T_CHK_SHFT) & BN0_WF_TMAC_TOP_TRCR0_TR2T_CHK_MASK);
		value |= ((i2t_chk_time << BN0_WF_TMAC_TOP_TRCR0_I2T_CHK_SHFT) & BN0_WF_TMAC_TOP_TRCR0_I2T_CHK_MASK);
		MAC_IO_WRITE32(pAd->hdev_ctrl, BN0_WF_TMAC_TOP_TRCR0_ADDR, value);
	} else if (band_idx == TESTMODE_BAND1) {
		MAC_IO_READ32(pAd->hdev_ctrl, BN1_WF_TMAC_TOP_TRCR0_ADDR, &value);
		value |= ((tr2t_chk_time << BN1_WF_TMAC_TOP_TRCR0_TR2T_CHK_SHFT) & BN1_WF_TMAC_TOP_TRCR0_TR2T_CHK_MASK);
		value |= ((i2t_chk_time << BN1_WF_TMAC_TOP_TRCR0_I2T_CHK_SHFT) & BN1_WF_TMAC_TOP_TRCR0_I2T_CHK_MASK);
		MAC_IO_WRITE32(pAd->hdev_ctrl, BN1_WF_TMAC_TOP_TRCR0_ADDR, value);
	} else {
		MTWF_LOG(DBG_CAT_TEST, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			("%s: Invalid band_idx!!\n", __func__));
		return FALSE;
	}

	return ret;
}
