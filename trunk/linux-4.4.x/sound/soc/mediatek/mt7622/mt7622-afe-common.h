/*
 * mt7622-afe-common.h  --  Mediatek 7622 audio driver definitions
 *
 * Copyright (c) 2016 MediaTek Inc.
 * Author: Garlic Tseng <garlic.tseng@mediatek.com>
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

#ifndef _MT_7622_AFE_COMMON_H_
#define _MT_7622_AFE_COMMON_H_
#include <sound/soc.h>
#include <linux/clk.h>
#include <linux/regmap.h>
#include "mt7622-reg.h"
#include "../common/mtk-base-afe.h"

#define MT2701_STREAM_DIR_NUM (SNDRV_PCM_STREAM_LAST + 1)
#define MT7622_PLL_DOMAIN_0_RATE	(294912000 / 2)
#define MT7622_PLL_DOMAIN_1_RATE	(270950400 / 2)
#define MT7622_AUD_AUD_MUX1_DIV_RATE (MT7622_PLL_DOMAIN_0_RATE / 3)
#define MT7622_AUD_AUD_MUX2_DIV_RATE (MT7622_PLL_DOMAIN_1_RATE / 3)

#define AUDIO_CLOCK_135M 135475200
#define AUDIO_CLOCK_147M 147456000

/* #define EARLY_PORTING */

enum {
	MT7622_I2S_1,
	MT7622_I2S_2,
	MT7622_I2S_3,
	MT7622_I2S_4,
	MT7622_I2S_NUM,
};

enum {
	MT7622_MEMIF_DL1,
	MT7622_MEMIF_DL2,
	MT7622_MEMIF_DL3,
	MT7622_MEMIF_DL4,
	MT7622_MEMIF_DL_SINGLE_NUM,
	MT7622_MEMIF_DLM = MT7622_MEMIF_DL_SINGLE_NUM,
	MT7622_MEMIF_UL1,
	MT7622_MEMIF_UL2,
	MT7622_MEMIF_UL3,
	MT7622_MEMIF_UL4,
	MT7622_MEMIF_DLTDM,
	MT7622_MEMIF_ULTDM,
	MT7622_MEMIF_NUM,
	MT7622_IO_I2S = MT7622_MEMIF_NUM,
	MT7622_IO_2ND_I2S,
	MT7622_IO_3RD_I2S,
	MT7622_IO_4TH_I2S,
};

enum {
	MT7622_IRQ_ASYS_START,
	MT7622_IRQ_ASYS_IRQ1_DL1 = MT7622_IRQ_ASYS_START,
	MT7622_IRQ_ASYS_IRQ2_DL2,
	MT7622_IRQ_ASYS_IRQ3_DL3,
	MT7622_IRQ_ASYS_IRQ4_DL4,
	MT7622_IRQ_ASYS_IRQ5_DLM,
	MT7622_IRQ_ASYS_IRQ6_UL1,
	MT7622_IRQ_ASYS_IRQ7_UL2,
	MT7622_IRQ_ASYS_IRQ8_UL3,
	MT7622_IRQ_ASYS_IRQ9_UL4,
	MT7622_IRQ_ASYS_IRQ10_TDMOUT,
	MT7622_IRQ_ASYS_IRQ11_TDMIN,
	MT7622_IRQ_ASYS_END,
};

enum {
	MT7622_TDMO,
	MT7622_TDMI,
	MT7622_TDM_NUM,
};

enum afe_dlmch_ch_num {
	DLMCH_0CH = 0,
	DLMCH_1CH = 1,
	DLMCH_2CH = 2,
	DLMCH_3CH = 3,
	DLMCH_4CH = 4,
	DLMCH_5CH = 5,
	DLMCH_6CH = 6,
	DLMCH_7CH = 7,
	DLMCH_8CH = 8,
	DLMCH_9CH = 9,
	DLMCH_10CH = 10,
	DLMCH_11CH = 11,
	DLMCH_12CH = 12
};

/* 2701 clock def */
enum audio_system_clock_type {
	MT7622_AUD_CLK_INFRA_AUDIO_PD,
	MT7622_AUD_A1SYS_HP_SEL,
	MT7622_AUD_A2SYS_HP_SEL,
	MT7622_AUD_A1SYS_HP_DIV,
	MT7622_AUD_A2SYS_HP_DIV,
	MT7622_AUD_AUD1PLL,
	MT7622_AUD_AUD2PLL,
	MT7622_AUD_A1SYS_HP_DIV_PD,
	MT7622_AUD_A2SYS_HP_DIV_PD,
	MT7622_AUD_AUDINTBUS,
	MT7622_AUD_SYSPLL1_D4,
	MT7622_AUD_AUDINTDIR,
	MT7622_AUD_UNIVPLL_D2,
	MT7622_AUD_APLL1_SEL,
	MT7622_AUD_APLL2_SEL,
	MT7622_AUD_I2S0_MCK_SEL,
	MT7622_AUD_I2S1_MCK_SEL,
	MT7622_AUD_I2S2_MCK_SEL,
	MT7622_AUD_I2S3_MCK_SEL,
	MT7622_AUD_APLL1_DIV,
	MT7622_AUD_APLL2_DIV,
	MT7622_AUD_I2S0_MCK_DIV,
	MT7622_AUD_I2S1_MCK_DIV,
	MT7622_AUD_I2S2_MCK_DIV,
	MT7622_AUD_I2S3_MCK_DIV,
	MT7622_AUD_APLL1_DIV_PD,
	MT7622_AUD_APLL2_DIV_PD,
	MT7622_AUD_I2S0_MCK_DIV_PD,
	MT7622_AUD_I2S1_MCK_DIV_PD,
	MT7622_AUD_I2S2_MCK_DIV_PD,
	MT7622_AUD_I2S3_MCK_DIV_PD,
	MT7622_AUD_AUD1_SEL,
	MT7622_AUD_AUD2_SEL,
	MT7622_AUD_ASM_H_SEL,
	MT7622_AUD_ASM_M_SEL,
	MT7622_AUD_SYSPLL_D5,
	MT7622_AUD_UNIVPLL2_D2,
	MT7622_AUD_AUDIO_AFE,
	MT7622_AUD_AUDIO_APLL,
	MT7622_AUD_AUDIO_A1SYS,
	MT7622_AUD_AUDIO_A2SYS,
	MT7622_CLOCK_NUM
};

static const unsigned int mt7622_afe_backup_list[] = {
	AUDIO_TOP_CON0,
	AUDIO_TOP_CON4,
	AUDIO_TOP_CON5,
	ASYS_TOP_CON,
	AFE_CONN0,
	AFE_CONN1,
	AFE_CONN2,
	AFE_CONN3,
	AFE_CONN15,
	AFE_CONN16,
	AFE_CONN17,
	AFE_CONN18,
	AFE_CONN19,
	AFE_CONN20,
	AFE_CONN21,
	AFE_CONN22,
	AFE_DAC_CON0,
	AFE_MEMIF_PBUF_SIZE,
};

struct snd_pcm_substream;
struct mtk_base_irq_data;

struct mt7622_i2s_data {
	int i2s_ctrl_reg;
	int i2s_pwn_shift;
	int i2s_asrc_fs_shift;
	int i2s_asrc_fs_mask;
};

enum mt7622_i2s_dir {
	I2S_OUT,
	I2S_IN,
	I2S_DIR_NUM,
};

struct mt7622_i2s_path {
	int dai_id;
	int mclk_rate;
	int on[I2S_DIR_NUM];
	int occupied[I2S_DIR_NUM];
	const struct mt7622_i2s_data *i2s_data[2];
};

enum mt7622_tdm_channel {
	TDM_2CH = 0,
	TDM_4CH,
	TDM_8CH,
	TDM_12CH,
	TDM_16CH
};

struct mt7622_tdm_data {
	int tdm_ctrl_reg;
	int tdm_lrck_cycle_shift;
	int tdm_on_shift;
	int tdm_out_reg;
	int tdm_out_bit_width_shift;
	int tdm_out_ch_num_shift;
};

enum afe_tdm_fmt {
	TDM_I2S,
	TDM_LJ
};

struct mt2701_tdm_path {
	int dai_id;
	int on;
	int occupied;
	const struct mt7622_tdm_data *tdm_data;
};

struct mt7622_afe_private {
	struct clk *clocks[MT7622_CLOCK_NUM];
	struct mt7622_i2s_path i2s_path[MT7622_I2S_NUM];
	struct mt2701_tdm_path tdm_path[MT7622_TDM_NUM];
	bool mrg_enable[MT2701_STREAM_DIR_NUM];
};

struct afe_i2s_out_config {
	/* fpga_test_loop: clock from I2S-out (id+1)%6 */
	/* e.g. if I2S_OUT_1.fpga_test_loop=TRUE, I2S_OUT_1's clock is from I2S_OUT_2 */
	bool fpga_test_loop;
	bool data_from_sine;
	bool dsd_mode;
	bool couple_mode;
	bool one_heart_mode;	/* 0: 2 channel mode, 1: multi channel mode */
	int sampleRate;
	bool ws_invert;
	bool fmt;
	bool slave;
	bool wlen;
};

struct afe_i2s_in_config {
	bool fpga_test_loop3;	/* for DSD loopback: out1's lrck(data) -> in1's lrck(data) */
	bool fpga_test_loop;	/* sdata: out1 -> in1  (data pior to fpga_test_loop2) */
	bool fpga_test_loop2;	/* sdata,lrck,bck: out2 -> in1 */
	bool use_asrc;
	bool dsd_mode;
	int sampleRate;
	bool ws_invert;
	bool fmt;
	bool slave;
	bool wlen;

};


#endif
