/* Copyright (c) 2011-2013, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "mt8521p-afe-debug.h"
#include "mt8521p-afe-reg.h"
#include "mt8521p-afe-clk.h"

#include <linux/debugfs.h>
#include <linux/uaccess.h>

#define DEBUG_FS_BUFFER_SIZE 4096

struct mt_soc_audio_debug_fs {
	struct dentry *audio_dentry;
	char *fs_name;
	const struct file_operations *fops;
};

static ssize_t mt_soc_debug_read(struct file *file, char __user *buf, size_t count, loff_t *pos)
{
	const int size = DEBUG_FS_BUFFER_SIZE;
	char buffer[size];
	int n = 0;
	int i;

	mt_afe_main_clk_on();

	pr_notice("%s\n", __func__);

	n = scnprintf(buffer + n, size - n, "CLK_INFRA_AUDIO = 0x%x\n", infracfg_read(0x48));
	n += scnprintf(buffer + n, size - n, "CLK_AUDDIV_0 = 0x%x\n", topckgen_read(CLK_AUDDIV_0));
	n += scnprintf(buffer + n, size - n, "CLK_AUDDIV_1 = 0x%x\n", topckgen_read(CLK_AUDDIV_1));
	n += scnprintf(buffer + n, size - n, "CLK_AUDDIV_2 = 0x%x\n", topckgen_read(CLK_AUDDIV_2));
	n += scnprintf(buffer + n, size - n, "CLK_AUDDIV_3 = 0x%x\n", topckgen_read(CLK_AUDDIV_3));
	n += scnprintf(buffer + n, size - n, "CLK_CFG_3 = 0x%x\n", topckgen_read(CLK_CFG_3));
	n += scnprintf(buffer + n, size - n, "CLK_INTDIR_SEL = 0x%x\n", topckgen_read(CLK_INTDIR_SEL));

	n += scnprintf(buffer + n, size - n, "BOOST_DRIVING_I2S0 = 0x%x\n", pctrl_read(BOOST_DRIVING_I2S0));
	n += scnprintf(buffer + n, size - n, "BOOST_DRIVING_I2S12 = 0x%x\n", pctrl_read(BOOST_DRIVING_I2S12));
	n += scnprintf(buffer + n, size - n, "BOOST_DRIVING_I2S34 = 0x%x\n", pctrl_read(BOOST_DRIVING_I2S34));
	n += scnprintf(buffer + n, size - n, "BOOST_DRIVING_I2S5 = 0x%x\n", pctrl_read(BOOST_DRIVING_I2S5));
	n += scnprintf(buffer + n, size - n, "BOOST_DRIVING_SPDIF = 0x%x\n", pctrl_read(BOOST_DRIVING_SPDIF));

	n += scnprintf(buffer + n, size - n, "AUDIO_TOP_CON0 = 0x%x\n", afe_read(AUDIO_TOP_CON0));
	n += scnprintf(buffer + n, size - n, "AUDIO_TOP_CON1 = 0x%x\n", afe_read(AUDIO_TOP_CON1));
	n += scnprintf(buffer + n, size - n, "AUDIO_TOP_CON2 = 0x%x\n", afe_read(AUDIO_TOP_CON2));
	n += scnprintf(buffer + n, size - n, "AUDIO_TOP_CON3 = 0x%x\n", afe_read(AUDIO_TOP_CON3));
	n += scnprintf(buffer + n, size - n, "AUDIO_TOP_CON4 = 0x%x\n", afe_read(AUDIO_TOP_CON4));
	n += scnprintf(buffer + n, size - n, "AUDIO_TOP_CON5 = 0x%x\n", afe_read(AUDIO_TOP_CON5));

	n += scnprintf(buffer + n, size - n, "AFE_DAC_CON0 = 0x%x\n", afe_read(AFE_DAC_CON0));
	n += scnprintf(buffer + n, size - n, "AFE_DAC_CON1 = 0x%x\n", afe_read(AFE_DAC_CON1));
	n += scnprintf(buffer + n, size - n, "AFE_DAC_CON2 = 0x%x\n", afe_read(AFE_DAC_CON2));
	n += scnprintf(buffer + n, size - n, "AFE_DAC_CON3 = 0x%x\n", afe_read(AFE_DAC_CON3));
	n += scnprintf(buffer + n, size - n, "AFE_DAC_CON4 = 0x%x\n", afe_read(AFE_DAC_CON4));
	n += scnprintf(buffer + n, size - n, "AFE_DAC_CON5 = 0x%x\n", afe_read(AFE_DAC_CON5));

	n += scnprintf(buffer + n, size - n, "AFE_DAIBT_CON0 = 0x%x\n", afe_read(AFE_DAIBT_CON0));
	n += scnprintf(buffer + n, size - n, "AFE_MRGIF_CON = 0x%x\n", afe_read(AFE_MRGIF_CON));
	n += scnprintf(buffer + n, size - n, "AFE_BT_SECURITY0 = 0x%x\n", afe_read(AFE_BT_SECURITY0));
	n += scnprintf(buffer + n, size - n, "AFE_BT_SECURITY1 = 0x%x\n", afe_read(AFE_BT_SECURITY1));


	n += scnprintf(buffer + n, size - n, "AFE_IRQ1_MCU_CNT_MON = 0x%x\n", afe_read(AFE_IRQ1_MCU_CNT_MON));
	n += scnprintf(buffer + n, size - n, "AFE_IRQ2_MCU_CNT_MON = 0x%x\n", afe_read(AFE_IRQ2_MCU_CNT_MON));
	n += scnprintf(buffer + n, size - n, "AFE_IRQ_MCU_CNT_MON_MASK = 0x%x\n", afe_read(AFE_IRQ_MCU_CNT_MON_MASK));

	for (i = 0 ; i < 41 ; i++)
		n += scnprintf(buffer + n, size - n, "AFE_CONN%d = 0x%x\n", i, afe_read(AFE_CONN0+i*4));
	n += scnprintf(buffer + n, size - n, "ASYS_TOP_CON = 0x%x\n", afe_read(ASYS_TOP_CON));
	n += scnprintf(buffer + n, size - n, "ASYS_I2SIN1_CON = 0x%x\n", afe_read(ASYS_I2SIN1_CON));
	n += scnprintf(buffer + n, size - n, "ASYS_I2SO1_CON = 0x%x\n", afe_read(ASYS_I2SO1_CON));
	n += scnprintf(buffer + n, size - n, "AFE_MEMIF_HD_CON1 = 0x%x\n", afe_read(AFE_MEMIF_HD_CON1));
	n += scnprintf(buffer + n, size - n, "AFE_MEMIF_HD_CON0 = 0x%x\n", afe_read(AFE_MEMIF_HD_CON0));
	n += scnprintf(buffer + n, size - n, "MASM_TRAC_CON1 = 0x%x\n", afe_read(MASM_TRAC_CON1));

	n += scnprintf(buffer + n, size - n, "AFE_DL1_BASE = 0x%x\n", afe_read(AFE_DL1_BASE));
	n += scnprintf(buffer + n, size - n, "AFE_DL1_CUR = 0x%x\n", afe_read(AFE_DL1_CUR));
	n += scnprintf(buffer + n, size - n, "AFE_DL1_END = 0x%x\n", afe_read(AFE_DL1_END));
	n += scnprintf(buffer + n, size - n, "AFE_DL2_BASE = 0x%x\n", afe_read(AFE_DL2_BASE));
	n += scnprintf(buffer + n, size - n, "AFE_DL2_CUR = 0x%x\n", afe_read(AFE_DL2_CUR));
	n += scnprintf(buffer + n, size - n, "AFE_DL2_END = 0x%x\n", afe_read(AFE_DL2_END));
	n += scnprintf(buffer + n, size - n, "AFE_DL3_BASE = 0x%x\n", afe_read(AFE_DL3_BASE));
	n += scnprintf(buffer + n, size - n, "AFE_DL3_CUR = 0x%x\n", afe_read(AFE_DL3_CUR));
	n += scnprintf(buffer + n, size - n, "AFE_DL3_END = 0x%x\n", afe_read(AFE_DL3_END));
	n += scnprintf(buffer + n, size - n, "AFE_DL4_BASE = 0x%x\n", afe_read(AFE_DL4_BASE));
	n += scnprintf(buffer + n, size - n, "AFE_DL4_CUR = 0x%x\n", afe_read(AFE_DL4_CUR));
	n += scnprintf(buffer + n, size - n, "AFE_DL4_END = 0x%x\n", afe_read(AFE_DL4_END));
	n += scnprintf(buffer + n, size - n, "AFE_DL5_BASE = 0x%x\n", afe_read(AFE_DL5_BASE));
	n += scnprintf(buffer + n, size - n, "AFE_DL5_CUR = 0x%x\n", afe_read(AFE_DL5_CUR));
	n += scnprintf(buffer + n, size - n, "AFE_DL5_END = 0x%x\n", afe_read(AFE_DL5_END));
	n += scnprintf(buffer + n, size - n, "AFE_DL6_BASE = 0x%x\n", afe_read(AFE_DL6_BASE));
	n += scnprintf(buffer + n, size - n, "AFE_DL6_CUR = 0x%x\n", afe_read(AFE_DL6_CUR));
	n += scnprintf(buffer + n, size - n, "AFE_DL6_END = 0x%x\n", afe_read(AFE_DL6_END));
	n += scnprintf(buffer + n, size - n, "AFE_ARB1_BASE = 0x%x\n", afe_read(AFE_ARB1_BASE));
	n += scnprintf(buffer + n, size - n, "AFE_ARB1_CUR = 0x%x\n", afe_read(AFE_ARB1_CUR));
	n += scnprintf(buffer + n, size - n, "AFE_ARB1_END = 0x%x\n", afe_read(AFE_ARB1_END));
	n += scnprintf(buffer + n, size - n, "AFE_DLMCH_BASE = 0x%x\n", afe_read(AFE_DLMCH_BASE));
	n += scnprintf(buffer + n, size - n, "AFE_DLMCH_CUR = 0x%x\n", afe_read(AFE_DLMCH_CUR));
	n += scnprintf(buffer + n, size - n, "AFE_DLMCH_END = 0x%x\n", afe_read(AFE_DLMCH_END));
	n += scnprintf(buffer + n, size - n, "AFE_DSDR_BASE = 0x%x\n", afe_read(AFE_DSDR_BASE));
	n += scnprintf(buffer + n, size - n, "AFE_DSDR_CUR = 0x%x\n", afe_read(AFE_DSDR_CUR));
	n += scnprintf(buffer + n, size - n, "AFE_DSDR_END = 0x%x\n", afe_read(AFE_DSDR_END));
	n += scnprintf(buffer + n, size - n, "AFE_AWB_BASE = 0x%x\n", afe_read(AFE_AWB_BASE));
	n += scnprintf(buffer + n, size - n, "AFE_AWB_END = 0x%x\n", afe_read(AFE_AWB_END));
	n += scnprintf(buffer + n, size - n, "AFE_AWB_CUR = 0x%x\n", afe_read(AFE_AWB_CUR));
	n += scnprintf(buffer + n, size - n, "AFE_AWB2_BASE = 0x%x\n", afe_read(AFE_AWB2_BASE));
	n += scnprintf(buffer + n, size - n, "AFE_AWB2_END = 0x%x\n", afe_read(AFE_AWB2_END));
	n += scnprintf(buffer + n, size - n, "AFE_AWB2_CUR = 0x%x\n", afe_read(AFE_AWB2_CUR));
	n += scnprintf(buffer + n, size - n, "AFE_DSDW_BASE = 0x%x\n", afe_read(AFE_DSDW_BASE));
	n += scnprintf(buffer + n, size - n, "AFE_DSDW_END = 0x%x\n", afe_read(AFE_DSDW_END));
	n += scnprintf(buffer + n, size - n, "AFE_DSDW_CUR = 0x%x\n", afe_read(AFE_DSDW_CUR));
	n += scnprintf(buffer + n, size - n, "AFE_VUL_BASE = 0x%x\n", afe_read(AFE_VUL_BASE));
	n += scnprintf(buffer + n, size - n, "AFE_VUL_END = 0x%x\n", afe_read(AFE_VUL_END));
	n += scnprintf(buffer + n, size - n, "AFE_VUL_CUR = 0x%x\n", afe_read(AFE_VUL_CUR));
	n += scnprintf(buffer + n, size - n, "AFE_UL2_BASE = 0x%x\n", afe_read(AFE_UL2_BASE));
	n += scnprintf(buffer + n, size - n, "AFE_UL2_END = 0x%x\n", afe_read(AFE_UL2_END));
	n += scnprintf(buffer + n, size - n, "AFE_UL2_CUR = 0x%x\n", afe_read(AFE_UL2_CUR));
	n += scnprintf(buffer + n, size - n, "AFE_UL3_BASE = 0x%x\n", afe_read(AFE_UL3_BASE));
	n += scnprintf(buffer + n, size - n, "AFE_UL3_END = 0x%x\n", afe_read(AFE_UL3_END));
	n += scnprintf(buffer + n, size - n, "AFE_UL3_CUR = 0x%x\n", afe_read(AFE_UL3_CUR));
	n += scnprintf(buffer + n, size - n, "AFE_UL4_BASE = 0x%x\n", afe_read(AFE_UL4_BASE));
	n += scnprintf(buffer + n, size - n, "AFE_UL4_END = 0x%x\n", afe_read(AFE_UL4_END));
	n += scnprintf(buffer + n, size - n, "AFE_UL4_CUR = 0x%x\n", afe_read(AFE_UL4_CUR));
	n += scnprintf(buffer + n, size - n, "AFE_UL5_BASE = 0x%x\n", afe_read(AFE_UL5_BASE));
	n += scnprintf(buffer + n, size - n, "AFE_UL5_END = 0x%x\n", afe_read(AFE_UL5_END));
	n += scnprintf(buffer + n, size - n, "AFE_UL5_CUR = 0x%x\n", afe_read(AFE_UL5_CUR));

	n += scnprintf(buffer + n, size - n, "AFE_UL6_BASE = 0x%x\n", afe_read(AFE_UL6_BASE));
	n += scnprintf(buffer + n, size - n, "AFE_UL6_END = 0x%x\n", afe_read(AFE_UL6_END));
	n += scnprintf(buffer + n, size - n, "AFE_UL6_CUR = 0x%x\n", afe_read(AFE_UL6_CUR));

	n += scnprintf(buffer + n, size - n, "AFE_DAI_BASE = 0x%x\n", afe_read(AFE_DAI_BASE));
	n += scnprintf(buffer + n, size - n, "AFE_DAI_END = 0x%x\n", afe_read(AFE_DAI_END));
	n += scnprintf(buffer + n, size - n, "AFE_DAI_CUR = 0x%x\n", afe_read(AFE_DAI_CUR));

	n += scnprintf(buffer + n, size - n, "AFE_MEMIF_MON0 = 0x%x\n", afe_read(AFE_MEMIF_MON0));
	n += scnprintf(buffer + n, size - n, "AFE_MEMIF_MON1 = 0x%x\n", afe_read(AFE_MEMIF_MON1));
	n += scnprintf(buffer + n, size - n, "AFE_MEMIF_MON2 = 0x%x\n", afe_read(AFE_MEMIF_MON2));
	n += scnprintf(buffer + n, size - n, "AFE_MEMIF_MON3 = 0x%x\n", afe_read(AFE_MEMIF_MON3));
	n += scnprintf(buffer + n, size - n, "AFE_MEMIF_MON4 = 0x%x\n", afe_read(AFE_MEMIF_MON4));

	n += scnprintf(buffer + n, size - n, "ASMI_TIMING_CON1 = 0x%x\n", afe_read(ASMI_TIMING_CON1));
	n += scnprintf(buffer + n, size - n, "ASMO_TIMING_CON1 = 0x%x\n", afe_read(ASMO_TIMING_CON1));
	n += scnprintf(buffer + n, size - n, "AFE_SGEN_CON0 = 0x%x\n", afe_read(AFE_SGEN_CON0));
	n += scnprintf(buffer + n, size - n, "AFE_MOD_PCM_BASE = 0x%x\n", afe_read(AFE_MOD_PCM_BASE));
	n += scnprintf(buffer + n, size - n, "AFE_MOD_PCM_END = 0x%x\n", afe_read(AFE_MOD_PCM_END));
	n += scnprintf(buffer + n, size - n, "AFE_MOD_PCM_CUR = 0x%x\n", afe_read(AFE_MOD_PCM_CUR));
	n += scnprintf(buffer + n, size - n, "AFE_SPDIF2_OUT_CON0 = 0x%x\n", afe_read(AFE_SPDIF2_OUT_CON0));
	n += scnprintf(buffer + n, size - n, "AFE_SPDIF2_BASE = 0x%x\n", afe_read(AFE_SPDIF2_BASE));
	n += scnprintf(buffer + n, size - n, "AFE_SPDIF2_CUR = 0x%x\n", afe_read(AFE_SPDIF2_CUR));
	n += scnprintf(buffer + n, size - n, "AFE_SPDIF2_END = 0x%x\n", afe_read(AFE_SPDIF2_END));
	n += scnprintf(buffer + n, size - n, "AFE_HDMI_OUT_CON0 = 0x%x\n", afe_read(AFE_HDMI_OUT_CON0));
	n += scnprintf(buffer + n, size - n, "AFE_HDMI_OUT_BASE = 0x%x\n", afe_read(AFE_HDMI_OUT_BASE));
	n += scnprintf(buffer + n, size - n, "AFE_HDMI_OUT_CUR = 0x%x\n", afe_read(AFE_HDMI_OUT_CUR));
	n += scnprintf(buffer + n, size - n, "AFE_HDMI_OUT_END = 0x%x\n", afe_read(AFE_HDMI_OUT_END));

	n += scnprintf(buffer + n, size - n, "AFE_SPDIF_OUT_CON0 = 0x%x\n", afe_read(AFE_SPDIF_OUT_CON0));
	n += scnprintf(buffer + n, size - n, "AFE_SPDIF_BASE = 0x%x\n", afe_read(AFE_SPDIF_BASE));
	n += scnprintf(buffer + n, size - n, "AFE_SPDIF_CUR = 0x%x\n", afe_read(AFE_SPDIF_CUR));
	n += scnprintf(buffer + n, size - n, "AFE_SPDIF_END = 0x%x\n", afe_read(AFE_SPDIF_END));
	n += scnprintf(buffer + n, size - n, "AFE_HDMI_CONN0 = 0x%x\n", afe_read(AFE_HDMI_CONN0));
	n += scnprintf(buffer + n, size - n, "AFE_8CH_I2S_OUT_CON = 0x%x\n", afe_read(AFE_8CH_I2S_OUT_CON));
	n += scnprintf(buffer + n, size - n, "AFE_HDMI_CONN1 = 0x%x\n", afe_read(AFE_HDMI_CONN1));

	n += scnprintf(buffer + n, size - n, "AFE_IRQ_CON = 0x%x\n", afe_read(AFE_IRQ_CON));
	n += scnprintf(buffer + n, size - n, "AFE_IRQ_STATUS = 0x%x\n", afe_read(AFE_IRQ_STATUS));
	n += scnprintf(buffer + n, size - n, "AFE_IRQ_CLR = 0x%x\n", afe_read(AFE_IRQ_CLR));
	n += scnprintf(buffer + n, size - n, "AFE_IRQ_CNT1 = 0x%x\n", afe_read(AFE_IRQ_CNT1));
	n += scnprintf(buffer + n, size - n, "AFE_IRQ_CNT2 = 0x%x\n", afe_read(AFE_IRQ_CNT2));
	n += scnprintf(buffer + n, size - n, "AFE_IRQ_MON2 = 0x%x\n", afe_read(AFE_IRQ_MON2));
	n += scnprintf(buffer + n, size - n, "AFE_IRQ_CNT5 = 0x%x\n", afe_read(AFE_IRQ_CNT5));
	n += scnprintf(buffer + n, size - n, "AFE_IRQ1_CNT_MON = 0x%x\n", afe_read(AFE_IRQ1_CNT_MON));
	n += scnprintf(buffer + n, size - n, "AFE_IRQ2_CNT_MON = 0x%x\n", afe_read(AFE_IRQ2_CNT_MON));
	n += scnprintf(buffer + n, size - n, "AFE_IRQ1_EN_CNT_MON = 0x%x\n", afe_read(AFE_IRQ1_EN_CNT_MON));
	n += scnprintf(buffer + n, size - n, "ASYS_IRQ_CONFIG = 0x%x\n", afe_read(ASYS_IRQ_CONFIG));
	n += scnprintf(buffer + n, size - n, "ASYS_IRQ1_CON = 0x%x\n", afe_read(ASYS_IRQ1_CON));
	n += scnprintf(buffer + n, size - n, "ASYS_IRQ2_CON = 0x%x\n", afe_read(ASYS_IRQ2_CON));
	n += scnprintf(buffer + n, size - n, "ASYS_IRQ3_CON = 0x%x\n", afe_read(ASYS_IRQ3_CON));
	n += scnprintf(buffer + n, size - n, "ASYS_IRQ4_CON = 0x%x\n", afe_read(ASYS_IRQ4_CON));
	n += scnprintf(buffer + n, size - n, "ASYS_IRQ5_CON = 0x%x\n", afe_read(ASYS_IRQ5_CON));
	n += scnprintf(buffer + n, size - n, "ASYS_IRQ6_CON = 0x%x\n", afe_read(ASYS_IRQ6_CON));
	n += scnprintf(buffer + n, size - n, "ASYS_IRQ7_CON = 0x%x\n", afe_read(ASYS_IRQ7_CON));
	n += scnprintf(buffer + n, size - n, "ASYS_IRQ8_CON = 0x%x\n", afe_read(ASYS_IRQ8_CON));
	n += scnprintf(buffer + n, size - n, "ASYS_IRQ9_CON = 0x%x\n", afe_read(ASYS_IRQ9_CON));
	n += scnprintf(buffer + n, size - n, "ASYS_IRQ10_CON = 0x%x\n", afe_read(ASYS_IRQ10_CON));
	n += scnprintf(buffer + n, size - n, "ASYS_IRQ11_CON = 0x%x\n", afe_read(ASYS_IRQ11_CON));
	n += scnprintf(buffer + n, size - n, "ASYS_IRQ12_CON = 0x%x\n", afe_read(ASYS_IRQ12_CON));
	n += scnprintf(buffer + n, size - n, "ASYS_IRQ13_CON = 0x%x\n", afe_read(ASYS_IRQ13_CON));
	n += scnprintf(buffer + n, size - n, "ASYS_IRQ14_CON = 0x%x\n", afe_read(ASYS_IRQ14_CON));
	n += scnprintf(buffer + n, size - n, "ASYS_IRQ15_CON = 0x%x\n", afe_read(ASYS_IRQ15_CON));
	n += scnprintf(buffer + n, size - n, "ASYS_IRQ16_CON = 0x%x\n", afe_read(ASYS_IRQ16_CON));
	n += scnprintf(buffer + n, size - n, "ASYS_IRQ_CLR = 0x%x\n", afe_read(ASYS_IRQ_CLR));
	n += scnprintf(buffer + n, size - n, "ASYS_IRQ_STATUS = 0x%x\n", afe_read(ASYS_IRQ_STATUS));
	n += scnprintf(buffer + n, size - n, "ASYS_IRQ_MON2 = 0x%x\n", afe_read(ASYS_IRQ_MON2));
	n += scnprintf(buffer + n, size - n, "PWR2_ASM_CON2 = 0x%x\n", afe_read(PWR2_ASM_CON2));

	n += scnprintf(buffer + n, size - n, "AFE_GAIN1_CON0 = 0x%x\n", afe_read(AFE_GAIN1_CON0));
	n += scnprintf(buffer + n, size - n, "AFE_GAIN1_CON1 = 0x%x\n", afe_read(AFE_GAIN1_CON1));
	n += scnprintf(buffer + n, size - n, "AFE_GAIN1_CON2 = 0x%x\n", afe_read(AFE_GAIN1_CON2));
	n += scnprintf(buffer + n, size - n, "AFE_GAIN1_CON3 = 0x%x\n", afe_read(AFE_GAIN1_CON3));
	n += scnprintf(buffer + n, size - n, "PWR2_TOP_CON = 0x%x\n", afe_read(PWR2_TOP_CON));
	n += scnprintf(buffer + n, size - n, "PWR1_ASM_CON1 = 0x%x\n", afe_read(PWR1_ASM_CON1));
	n += scnprintf(buffer + n, size - n, "PWR2_ASM_CON1 = 0x%x\n", afe_read(PWR2_ASM_CON1));

	n += scnprintf(buffer + n, size - n, "AFE_MEMIF_PBUF_SIZE = 0x%x\n", afe_read(AFE_MEMIF_PBUF_SIZE));
	n += scnprintf(buffer + n, size - n, "AFE_ULMCH_BASE = 0x%x\n", afe_read(AFE_ULMCH_BASE));
	n += scnprintf(buffer + n, size - n, "AFE_ULMCH_END = 0x%x\n", afe_read(AFE_ULMCH_END));
	n += scnprintf(buffer + n, size - n, "AFE_ULMCH_CUR = 0x%x\n", afe_read(AFE_ULMCH_CUR));
	n += scnprintf(buffer + n, size - n, "AFE_LRCK_CNT = 0x%x\n", afe_read(AFE_LRCK_CNT));

	mt_afe_main_clk_off();

	return simple_read_from_buffer(buf, count, pos, buffer, n);
}
#if 0
static ssize_t mt_soc_debug_write(struct file *file, const char __user *user_buf,
			size_t count, loff_t *pos)
{
	char buf[64];
	size_t buf_size;
	char *start = buf;
	char *reg_str;
	char *value_str;
	const char delim[] = " ,";
	unsigned long reg, value;

	buf_size = min(count, (sizeof(buf) - 1));
	if (copy_from_user(buf, user_buf, buf_size))
		return -EFAULT;

	buf[buf_size] = 0;

	reg_str = strsep(&start, delim);
	if (!reg_str || !strlen(reg_str))
		return -EINVAL;

	value_str = strsep(&start, delim);
	if (!value_str || !strlen(value_str))
		return -EINVAL;

	if (kstrtoul(reg_str, 16, &reg))
		return -EINVAL;

	if (kstrtoul(value_str, 16, &value))
		return -EINVAL;

	mt_afe_main_clk_on();
	mt_afe_set_reg(reg, value, 0xffffffff);
	mt_afe_main_clk_off();

	return buf_size;
}

#endif
static const struct file_operations mtaudio_debug_ops = {
	.open = simple_open,
	.read = mt_soc_debug_read,
	/*.write = mt_soc_debug_write,*/
	.llseek = default_llseek,
};


static struct mt_soc_audio_debug_fs audio_debug_fs[] = {
	{NULL, "mtksocaudio", &mtaudio_debug_ops},
};

void mt_afe_debug_init(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(audio_debug_fs); i++) {
		audio_debug_fs[i].audio_dentry = debugfs_create_file(audio_debug_fs[i].fs_name,
							0644, NULL, NULL,
							audio_debug_fs[i].fops);
		if (!audio_debug_fs[i].audio_dentry)
			pr_warn("%s failed to create %s debugfs file\n", __func__,
				audio_debug_fs[i].fs_name);
	}
}

void mt_afe_debug_deinit(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(audio_debug_fs); i++)
		debugfs_remove(audio_debug_fs[i].audio_dentry);
}

