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

#ifndef _MT8521P_AFE_H_
#define _MT8521P_AFE_H_

#include <linux/interrupt.h>

void afe_enable(int en);
int mt_afe_platform_init(void *dev);
void mt_afe_platform_deinit(void *dev);
#ifdef CONFIG_OF
unsigned int mt_afe_get_afe_irq_id(void);
unsigned int mt_afe_get_asys_irq_id(void);
#endif

enum power_mode {
	NORMAL_POWER_MODE,
	LOW_POWER_MODE
};
void afe_power_mode(enum power_mode mode);

unsigned char *afe_sram_virt(void);
u32 afe_sram_phys(void);
size_t afe_sram_size(void);

enum spdif_out2_source {
	SPDIF_OUT2_SOURCE_IEC2,
	SPDIF_OUT2_SOURCE_OPTICAL_IN,
	SPDIF_OUT2_SOURCE_COAXIAL_IN,
};
void afe_spdif_out2_source_sel(enum spdif_out2_source s);
void afe_loopback_set(int en);

/******************** interconnection ********************/

enum itrcon_in {
	I00, I01, I02, I03, I04, I05, I06, I07,
	I08, I09, I10, I11, I12, I13, I14, I15,
	I16, I17, I18, I19, I20, I21, I22, I23,
	I24, I25, I26, I27, I28, I29, I30, I31,
	I32, I33, I34, I35, I36, IN_MAX
};

enum itrcon_out {
	O00, O01, O02, O03, O04, O05, O06, O07,
	O08, O09, O10, O11, O12, O13, O14, O15,
	O16, O17, O18, O19, O20, O21, O22, O23,
	O24, O25, O26, O27, O28, O29, O30, O31,
	O32, O33, OUT_MAX
};

int itrcon_connect(enum itrcon_in in, enum itrcon_out out, int connect);
void itrcon_disconnectall(void);
int itrcon_rightshift1bit(enum itrcon_out out, int shift);
void itrcon_noshiftall(void);

typedef int (*itrcon_action)(int on);
/*
 * a1sys_start:
 * start/stop for AUD1PLL, MT_CG_AUDIO_A1SYS, MT_CG_AUDIO_AFE, itrcon
 */
int a1sys_start(itrcon_action itrcon, int start);

/******************** memory interface ********************/

enum afe_mem_interface {
	AFE_MEM_NONE = 0,
	AFE_MEM_DL1 = 1,
	AFE_MEM_DL2 = 2,
	AFE_MEM_DL3 = 3,
	AFE_MEM_DL4 = 4,
	AFE_MEM_DL5 = 5,
	AFE_MEM_DL6 = 6,
	AFE_MEM_DLMCH = 7,
	AFE_MEM_ARB1 = 8,
	AFE_MEM_DSDR = 9,
	AFE_MEM_UL1 = 10,
	AFE_MEM_UL2 = 11,
	AFE_MEM_UL3 = 12,
	AFE_MEM_UL4 = 13,
	AFE_MEM_UL5 = 14,
	AFE_MEM_UL6 = 15,
	AFE_MEM_ULMCH = 16,
	AFE_MEM_DAI = 17,
	AFE_MEM_MOD_PCM = 18,
	AFE_MEM_RESERVED = 19,
	AFE_MEM_AWB = 20,
	AFE_MEM_AWB2 = 21,
	AFE_MEM_DSDW = 22,
	AFE_MEM_NUM = 23
};

enum afe_sampling_rate {
	FS_8000HZ = 0x0,
	FS_12000HZ = 0x1,
	FS_16000HZ = 0x2,
	FS_24000HZ = 0x3,
	FS_32000HZ = 0x4,
	FS_48000HZ = 0x5,
	FS_96000HZ = 0x6,
	FS_192000HZ = 0x7,
	FS_384000HZ = 0x8,
	FS_I2S1 = 0x9,
	FS_I2S2 = 0xA,
	FS_I2S3 = 0xB,
	FS_I2S4 = 0xC,
	FS_I2S5 = 0xD,
	FS_I2S6 = 0xE,
	FS_7350HZ = 0x10,
	FS_11025HZ = 0x11,
	FS_14700HZ = 0x12,
	FS_22050HZ = 0x13,
	FS_29400HZ = 0x14,
	FS_44100HZ = 0x15,
	FS_88200HZ = 0x16,
	FS_176400HZ = 0x17,
	FS_352800HZ = 0x18,
	FS_768000HZ = 0x19
};

static inline unsigned int fs_integer(enum afe_sampling_rate fs)
{
	switch (fs) {
	case FS_8000HZ:
		return 8000;
	case FS_12000HZ:
		return 12000;
	case FS_16000HZ:
		return 16000;
	case FS_24000HZ:
		return 24000;
	case FS_32000HZ:
		return 32000;
	case FS_48000HZ:
		return 48000;
	case FS_96000HZ:
		return 96000;
	case FS_192000HZ:
		return 192000;
	case FS_384000HZ:
		return 384000;
	case FS_7350HZ:
		return 7350;
	case FS_11025HZ:
		return 11025;
	case FS_14700HZ:
		return 14700;
	case FS_22050HZ:
		return 22050;
	case FS_29400HZ:
		return 29400;
	case FS_44100HZ:
		return 44100;
	case FS_88200HZ:
		return 88200;
	case FS_176400HZ:
		return 176400;
	case FS_352800HZ:
		return 352800;
	case FS_768000HZ:
		return 768000;
	default:
		return 0;
	}
}

static inline enum afe_sampling_rate fs_enum(unsigned int fs)
{
	switch (fs) {
	case 8000:
		return FS_8000HZ;
	case 12000:
		return FS_12000HZ;
	case 16000:
		return FS_16000HZ;
	case 24000:
		return FS_24000HZ;
	case 32000:
		return FS_32000HZ;
	case 48000:
		return FS_48000HZ;
	case 96000:
		return FS_96000HZ;
	case 192000:
		return FS_192000HZ;
	case 384000:
		return FS_384000HZ;
	case 7350:
		return FS_7350HZ;
	case 11025:
		return FS_11025HZ;
	case 14700:
		return FS_14700HZ;
	case 22050:
		return FS_22050HZ;
	case 29400:
		return FS_29400HZ;
	case 44100:
		return FS_44100HZ;
	case 88200:
		return FS_88200HZ;
	case 176400:
		return FS_176400HZ;
	case 352800:
		return FS_352800HZ;
	case 768000:
		return FS_768000HZ;
	default:
		return FS_48000HZ;
	}
}

enum afe_dsd_width {
	DSD_WIDTH_32BIT = 0,	/* for PCM */
	DSD_WIDTH_16BIT = 1,	/* DSDL(16bit) DSDR(16bit) */
	DSD_WIDTH_8BIT = 2	/* DSDL(8bit) DSDR(8bit) */
};

enum afe_msb_lsb_first {
	MSB_FIRST = 0,
	LSB_FIRST = 1
};

enum afe_daimod_sampling_rate {
	DAIMOD_8000HZ = 0x0,
	DAIMOD_16000HZ = 0x1
};

enum afe_channel_mode {
	STEREO = 0x0,
	MONO = 0x1
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

enum afe_mono_select {
	MONO_USE_L = 0x0,
	MONO_USE_R = 0x1
};

enum afe_dup_write {
	DUP_WR_DISABLE = 0x0,
	DUP_WR_ENABLE = 0x1
};

struct afe_buffer {
	u32 base;
	u32 size;
};

struct afe_memif_config {
	enum afe_sampling_rate fs;
	int hd_audio;		/* 0: 16bit, 1: 24bit */
	enum afe_dsd_width dsd_width;
	enum afe_msb_lsb_first first_bit;
	enum afe_daimod_sampling_rate daimod_fs;
	enum afe_channel_mode channel;
	enum afe_dlmch_ch_num dlmch_ch_num;	/* used when DLMCH */
	enum afe_mono_select mono_sel;	/* used when AWB and VUL and data is mono */
	enum afe_dup_write dup_write;	/* used when MODPCM and DAI */
	struct afe_buffer buffer;
};

int afe_memif_enable(enum afe_mem_interface memif, int en);
int afe_memifs_enable(enum afe_mem_interface *memifs, size_t num, int en);
int afe_memif_configurate(enum afe_mem_interface memif, const struct afe_memif_config *config);
int afe_memif_pointer(enum afe_mem_interface memif, u32 *cur_ptr);
int afe_memif_base(enum afe_mem_interface memif, u32 *base);
int memif_enable_clk(int memif_id, int on);

/******************** irq ********************/

#define AFE_IRQ1_IRQ       (0x1<<0)
#define AFE_IRQ2_IRQ       (0x1<<1)
#define AFE_MULTIIN_IRQ    (0x1<<2)
#define AFE_SPDIF2_IRQ     (0x1<<3)
#define AFE_HDMI_IRQ       (0x1<<4)
#define AFE_SPDIF_IRQ      (0x1<<5)
#define AFE_SPDIFIN_IRQ    (0x1<<6)
#define AFE_DMA_IRQ        (0x1<<7)
u32 afe_irq_status(void);
void afe_irq_clear(u32 status);

#define ASYS_IRQ1_IRQ  (0x1<<0)
#define ASYS_IRQ2_IRQ  (0x1<<1)
#define ASYS_IRQ3_IRQ  (0x1<<2)
#define ASYS_IRQ4_IRQ  (0x1<<3)
#define ASYS_IRQ5_IRQ  (0x1<<4)
#define ASYS_IRQ6_IRQ  (0x1<<5)
#define ASYS_IRQ7_IRQ  (0x1<<6)
#define ASYS_IRQ8_IRQ  (0x1<<7)
#define ASYS_IRQ9_IRQ  (0x1<<8)
#define ASYS_IRQ10_IRQ  (0x1<<9)
#define ASYS_IRQ11_IRQ  (0x1<<10)
#define ASYS_IRQ12_IRQ  (0x1<<11)
#define ASYS_IRQ13_IRQ  (0x1<<12)
#define ASYS_IRQ14_IRQ  (0x1<<13)
#define ASYS_IRQ15_IRQ  (0x1<<14)
#define ASYS_IRQ16_IRQ  (0x1<<15)
u32 asys_irq_status(void);
void asys_irq_clear(u32 status);

enum audio_irq_id {
	IRQ_AFE_IRQ1, IRQ_AFE_IRQ2, IRQ_AFE_MULTIIN, IRQ_AFE_SPDIF2, IRQ_AFE_HDMI, IRQ_AFE_SPDIF,
	IRQ_AFE_SPDIFIN, IRQ_AFE_DMA, IRQ_ASYS_IRQ1, IRQ_ASYS_IRQ2, IRQ_ASYS_IRQ3,
	IRQ_ASYS_IRQ4, IRQ_ASYS_IRQ5, IRQ_ASYS_IRQ6, IRQ_ASYS_IRQ7, IRQ_ASYS_IRQ8,
	IRQ_ASYS_IRQ9, IRQ_ASYS_IRQ10, IRQ_ASYS_IRQ11, IRQ_ASYS_IRQ12, IRQ_ASYS_IRQ13,
	IRQ_ASYS_IRQ14, IRQ_ASYS_IRQ15, IRQ_ASYS_IRQ16, IRQ_NUM
};

struct audio_irq_config {
	enum afe_sampling_rate mode;
	u32 init_val;
};

int audio_irq_configurate(enum audio_irq_id id, const struct audio_irq_config *config);
int audio_irq_enable(enum audio_irq_id id, int en);
u32 audio_irq_cnt_mon(enum audio_irq_id id);
enum audio_irq_id asys_irq_acquire(void);
void asys_irq_release(enum audio_irq_id id);


/******************** i2s ********************/

enum afe_i2s_in_id {
	AFE_I2S_IN_1 = 0,
	AFE_I2S_IN_2 = 1,
	AFE_I2S_IN_3 = 2,
	AFE_I2S_IN_4 = 3,
	AFE_I2S_IN_5 = 4,
	AFE_I2S_IN_6 = 5,
	AFE_I2S_IN_NUM = 6
};

enum afe_i2s_format {
	FMT_32CYCLE_16BIT_I2S,
	FMT_32CYCLE_16BIT_LJ, /* if you want FMT_32CYCLE_16BIT_RJ, please use this too, because they are same */
	FMT_64CYCLE_16BIT_I2S,
	FMT_64CYCLE_16BIT_LJ,
	FMT_64CYCLE_16BIT_RJ,
	FMT_64CYCLE_32BIT_I2S,
	FMT_64CYCLE_32BIT_LJ, /* if you want FMT_64CYCLE_32BIT_RJ, please use this too, because they are same */
	FMT_64CYCLE_24BIT_RJ
};

static inline int is_hd_audio(enum afe_i2s_format format)
{
	return (format >= FMT_64CYCLE_32BIT_I2S && format <= FMT_64CYCLE_24BIT_RJ) ? 1 : 0;
}

struct afe_i2s_in_config {
	int fpga_test_loop3;	/* for DSD loopback: out1's lrck(data) -> in1's lrck(data) */
	int fpga_test_loop;	/* sdata: out1 -> in1  (data pior to fpga_test_loop2) */
	int fpga_test_loop2;	/* sdata,lrck,bck: out2 -> in1 */
	int use_asrc;
	int dsd_mode;
	int slave;
	enum afe_i2s_format fmt;
	int mclk;
	enum afe_sampling_rate mode;
};

enum afe_i2s_out_id {
	AFE_I2S_OUT_1 = 0,
	AFE_I2S_OUT_2 = 1,
	AFE_I2S_OUT_3 = 2,
	AFE_I2S_OUT_4 = 3,
	AFE_I2S_OUT_5 = 4,
	AFE_I2S_OUT_6 = 5,
	AFE_I2S_OUT_NUM = 6
};

enum afe_i2s_out_dsd_use {
	I2S_OUT_DSD_USE_NORMAL
};

struct afe_i2s_out_config {
	/* fpga_test_loop: clock from I2S-out (id+1)%6 */
	/* e.g. if I2S_OUT_1.fpga_test_loop=TRUE, I2S_OUT_1's clock is from I2S_OUT_2 */
	int fpga_test_loop;
	int data_from_sine;
	int use_asrc;
	int dsd_mode;
	enum afe_i2s_out_dsd_use dsd_use;
	int couple_mode;
	int one_heart_mode;	/* 0: 2 channel mode, 1: multi channel mode */
	int slave;
	enum afe_i2s_format fmt;
	int mclk;
	enum afe_sampling_rate mode;
};

int afe_i2s_in_configurate(enum afe_i2s_in_id id, const struct afe_i2s_in_config *config);
int afe_i2s_in_enable(enum afe_i2s_in_id id, int en);
int afe_i2s_out_configurate(enum afe_i2s_out_id id, const struct afe_i2s_out_config *config);
int afe_i2s_out_enable(enum afe_i2s_out_id id, int en);

int afe_i2s_in_master_pcm_configurate(enum afe_i2s_in_id id, enum afe_i2s_format fmt, int mclk,
				      enum afe_sampling_rate fs, int use_asrc);
int afe_i2s_in_master_pcm_enable(enum afe_i2s_in_id id, int en);
int afe_i2s_in_master_dsd_configurate(enum afe_i2s_in_id id, enum afe_sampling_rate fs, int mclk);
int afe_i2s_in_master_dsd_enable(enum afe_i2s_in_id id, int en);
int afe_i2s_in_slave_pcm_configurate(enum afe_i2s_in_id id, enum afe_i2s_format fmt, int use_asrc);
int afe_i2s_in_slave_pcm_enable(enum afe_i2s_in_id id, int en);
int afe_i2s_in_slave_dsd_configurate(enum afe_i2s_in_id id);
int afe_i2s_in_slave_dsd_enable(enum afe_i2s_in_id id, int en);

int afe_i2s_out_master_dsd_configurate(enum afe_i2s_out_id id, enum afe_sampling_rate fs, int mclk,
				       enum afe_i2s_out_dsd_use dsd_use);
int afe_i2s_out_master_dsd_enable(enum afe_i2s_out_id id, int en);
int afe_i2s_out_slave_pcm_configurate(enum afe_i2s_out_id id, enum afe_i2s_format fmt,
				      int use_asrc);
int afe_i2s_out_slave_pcm_enable(enum afe_i2s_out_id id, int en);
int afe_i2s_out_slave_dsd_configurate(enum afe_i2s_out_id id);
int afe_i2s_out_slave_dsd_enable(enum afe_i2s_out_id id, int en);
int afe_i2s_inmx_pcm_configurate(int i2s_num,	enum afe_i2s_format fmt,
				int mclk, enum afe_sampling_rate fs,
				int slave);
int afe_i2s_inmx_pcm_enable(int i2s_num, int en);
int afe_asmi_timing_set(enum afe_i2s_in_id id, enum afe_sampling_rate rate);
int afe_asmo_timing_set(enum afe_i2s_out_id id, enum afe_sampling_rate rate);

void afe_i2s_mclk_configurate(int id, int mclk);

/******************** sample-based asrc ********************/

struct afe_sample_asrc_config {
	int o16bit;		/* 0:32-bit 1:16-bit */
	int mono;		/* 0:STEREO 1:mono */
	enum afe_sampling_rate input_fs;
	enum afe_sampling_rate output_fs;
	int tracking;
};

enum afe_sample_asrc_tx_id {
	SAMPLE_ASRC_O1 = 0,
	SAMPLE_ASRC_O2 = 1,
	SAMPLE_ASRC_O3 = 2,
	SAMPLE_ASRC_O4 = 3,
	SAMPLE_ASRC_O5 = 4,
	SAMPLE_ASRC_O6 = 5,
	SAMPLE_ASRC_PCM_OUT = 6,
	SAMPLE_ASRC_OUT_NUM = 7
};

enum afe_sample_asrc_rx_id {
	SAMPLE_ASRC_I1 = 0,
	SAMPLE_ASRC_I2 = 1,
	SAMPLE_ASRC_I3 = 2,
	SAMPLE_ASRC_I4 = 3,
	SAMPLE_ASRC_I5 = 4,
	SAMPLE_ASRC_I6 = 5,
	SAMPLE_ASRC_PCM_IN = 6,
	SAMPLE_ASRC_IN_NUM = 7
};

int afe_power_on_sample_asrc_tx(enum afe_sample_asrc_tx_id id, int on);
int afe_sample_asrc_tx_configurate(enum afe_sample_asrc_tx_id id,
				   const struct afe_sample_asrc_config *config);
int afe_sample_asrc_tx_enable(enum afe_sample_asrc_tx_id id, int en);

int afe_power_on_sample_asrc_rx(enum afe_sample_asrc_rx_id id, int on);
int afe_sample_asrc_rx_configurate(enum afe_sample_asrc_rx_id id,
				   const struct afe_sample_asrc_config *config);
int afe_sample_asrc_rx_enable(enum afe_sample_asrc_rx_id id, int en);

/******************** merge interface ********************/

enum afe_daibt_input_sel {
	DAIBT_INPUT_SEL_DAIBTRX = 0,
	DAIBT_INPUT_SEL_MERGERIF = 1
};

enum afe_daibt_output_fs {
	DAIBT_OUTPUT_FS_8K = 0,
	DAIBT_OUTPUT_FS_16K = 1
};
enum afe_mrg_i2s_mode {
	MRG_I2S_FS_32K = 0x4,
	MRG_I2S_FS_44K = 0x15,
	MRG_I2S_FS_48K = 0x5
};
struct afe_daibt_config {
	int daibt_c;		/* this is encryption . should be set 1 for merge interface */
	int daibt_sync;		/* for daibt rx/tx, not merge */
	int daibt_ready;
	int daibt_len;		/* this is for daibt rx/tx setting, for mergeif should be set 0 */
	int daibt_mode;		/* 8K set: 0,16K set: 1 */

	/* daibt output fs is 8k or 16k. 0: 8k. 1:16k */
	enum afe_daibt_output_fs afe_daibt_fs;
	/* daibt input data is from merge if or daibt rx. 0: dairx, 1: mergeif */
	enum afe_daibt_input_sel afe_daibt_input;
};

struct afe_mrg_config {
	int mrg_clk_no_inv;
	int mrg_i2s_en;
	enum afe_mrg_i2s_mode mrg_i2s_mode;
};

int afe_power_on_mrg(int on);
int afe_daibt_configurate(struct afe_daibt_config *);
int afe_daibt_set_output_fs(enum afe_daibt_output_fs);
void afe_daibt_set_enable(int);
void afe_o31_pcm_tx_sel_pcmtx(int);
void afe_i26_pcm_rx_sel_pcmrx(int);
void afe_merge_set_sync_dly(unsigned int);
void afe_merge_set_clk_edge_dly(unsigned int);
void afe_merge_set_clk_dly(unsigned int);
void afe_merge_set_enable(int);
void afe_merge_i2s_clk_invert(int);	/* 0:invert,1:non-invert */
void afe_merge_i2s_enable(int);
void afe_merge_i2s_set_mode(enum afe_mrg_i2s_mode);


/******************** pcm interface ********************/

int afe_power_on_btpcm(int on);

enum afe_pcm_format {
	PCM_FMT_I2S = 0,
	PCM_FMT_EIAJ = 1,
	PCM_FMT_MODE_A = 2,
	PCM_FMT_MODE_B = 3
};

enum afe_pcm_mode {
	PCM_MODE_8K = 0,
	PCM_MODE_16K = 1
};

static inline enum afe_sampling_rate btpcm_fs_to_afe_fs(enum afe_pcm_mode mode)
{
	switch (mode) {
	case PCM_MODE_16K:
		return FS_16000HZ;
	case PCM_MODE_8K:
	default:
		return FS_8000HZ;
	}
}

struct afe_btpcm_config {
	enum afe_pcm_mode mode;
	enum afe_pcm_format fmt;
	int slave;
	int extloopback;
	int wlen;
};

int afe_btpcm_configurate(const struct afe_btpcm_config *config);
int afe_btpcm_enable(int en);


/********************* dmic *********************/

enum afe_dmic_id {
	AFE_DMIC_1 = 0, AFE_DMIC_2 = 1, AFE_DMIC_NUM = 2
};
struct afe_dmic_config {
	int one_wire_mode;
	int iir_on;
	int iir_mode;
	enum afe_sampling_rate voice_mode;
};
int afe_power_on_dmic(enum afe_dmic_id id, int on);
int afe_dmic_configurate(enum afe_dmic_id id, const struct afe_dmic_config *config);
int afe_dmic_enable(enum afe_dmic_id id, int en);


/********************* multilinein *********************/

enum afe_multilinein_endian {
	AFE_MULTILINEIN_BIG_ENDIAN	/* high byte in low address, low byte in high address */
	, AFE_MULTILINEIN_LITTILE_ENDIAN	/* high byte in high address, low byte in low address */
};

enum afe_multilinein_chnum {
	AFE_MULTILINEIN_2CH = 0,
	AFE_MULTILINEIN_4CH = 1,
	AFE_MULTILINEIN_6CH = 2,
	AFE_MULTILINEIN_8CH = 3
};

enum afe_multilinein_inter_period {
	AFE_MULTILINEIN_INTR_PERIOD_32 = 0,
	AFE_MULTILINEIN_INTR_PERIOD_64 = 1,
	AFE_MULTILINEIN_INTR_PERIOD_128 = 2,
	AFE_MULTILINEIN_INTR_PERIOD_256 = 3
};

enum afe_multilinein_sel_source {
	AFE_MULTILINE_FROM_8CHI2S = 0,
	AFE_MULTILINE_FROM_RX = 1
};

struct afe_multilinein_config {
	int dsd_mode;
	enum afe_multilinein_endian endian;
	enum afe_i2s_format fmt;
	enum afe_multilinein_chnum ch_num;
	enum afe_multilinein_inter_period intr_period;
	enum afe_multilinein_sel_source  mss;
	enum afe_dsd_width dsdWidth;
};


/********************* hw gain *********************/

enum afe_hg_step_db {
	AFE_STEP_DB_1_8 = 0, AFE_STEP_DB_1_4 = 1, AFE_STEP_DB_1_2 = 2
};
void afe_multilinein_enable(int en);
void afe_multilinein_configurate(const struct afe_multilinein_config *config);

struct afe_hw_gain_config {
	u32 hwtargetgain;
	u8 hwgainsteplen;	/* how many samples per step (0~255) */
	enum afe_hg_step_db hgstepdb;	/* 0:.125db,1:0.25db,2:0.5db */
};

enum afe_hwgain_id {
	AFE_HWGAIN_1 = 0, AFE_HWGAIN_2 = 1, AFE_HWGAIN_NUM = 2
};

int afe_hwgain_init(enum afe_hwgain_id id);
int afe_hwgain_configurate(u32 hgctl0, u32 hgctl1);
int afe_hwgain_enable(enum afe_hwgain_id id, int en);
int afe_hwgain_gainmode_set(enum afe_hwgain_id id, enum afe_sampling_rate fs);


/********************* spdif receiver *********************/

int afe_power_on_intdir(int on);

typedef enum {
	SPDIFIN_OUT_RANGE = 0x00, /*0x00~0x06 Freq out of range*/
	SPDIFIN_32K = 0x07,
	SPDIFIN_44K = 0x08,
	SPDIFIN_48K = 0x09,
	SPDIFIN_64K = 0x0A,
	SPDIFIN_88K = 0x0B,
	SPDIFIN_96K = 0x0C,
	SPDIFIN_128K = 0x0D,
	SPDIFIN_176K = 0x0E,
	SPDIFIN_192K = 0x0F
} SPDIFIN_FS;

struct afe_dir_info {
	int rate;
	u32 u_bit[2][6];
	u32 c_bit[6];
};

#define DATA_PCM_STATUS           0x01
#define DATA_NON_PCM_STATUS       0x00
#define DATA_UNKNOWN_STATUS       0xFF

void afe_spdifrx_isr(void);
enum afe_spdifrx_port {
	SPDIFRX_PORT_NONE = 0,
	SPDIFRX_PORT_OPT = 1,
	SPDIFRX_PORT_ARC = 2
};
void afe_spdifrx_start(enum afe_spdifrx_port port, void (*callback)(void));
void afe_spdifrx_stop(void);
const volatile struct afe_dir_info *afe_spdifrx_state(void);


/********************* memory asrc *********************/

enum afe_mem_asrc_id {
	MEM_ASRC_1 = 0,
	MEM_ASRC_2 = 1,
	MEM_ASRC_3 = 2,
	MEM_ASRC_4 = 3,
	MEM_ASRC_5 = 4,
	MEM_ASRC_NUM = 5
};

enum afe_mem_asrc_tracking_mode {
	MEM_ASRC_NO_TRACKING = 0,
	MEM_ASRC_TRACKING_TX = 1,	/* internal test only */
	MEM_ASRC_TRACKING_RX = 2	/* internal test only */
};

struct afe_mem_asrc_buffer {
	u32 base;		/* physical */
	u32 size;
	u32 freq;
	u32 bitwidth;
};

struct afe_mem_asrc_config {
	struct afe_mem_asrc_buffer input_buffer;
	struct afe_mem_asrc_buffer output_buffer;
	int stereo;
	enum afe_mem_asrc_tracking_mode tracking_mode;
};

int afe_power_on_mem_asrc_brg(int on);
int afe_power_on_mem_asrc(enum afe_mem_asrc_id id, int on);
int afe_mem_asrc_configurate(enum afe_mem_asrc_id id, const struct afe_mem_asrc_config *config);
int afe_mem_asrc_enable(enum afe_mem_asrc_id id, int en);
u32 afe_mem_asrc_get_ibuf_rp(enum afe_mem_asrc_id id);
u32 afe_mem_asrc_get_ibuf_wp(enum afe_mem_asrc_id id);
int afe_mem_asrc_set_ibuf_wp(enum afe_mem_asrc_id id, u32 p);
u32 afe_mem_asrc_get_obuf_rp(enum afe_mem_asrc_id id);
u32 afe_mem_asrc_get_obuf_wp(enum afe_mem_asrc_id id);
int afe_mem_asrc_set_obuf_rp(enum afe_mem_asrc_id id, u32 p);
int afe_mem_asrc_set_ibuf_freq(enum afe_mem_asrc_id id, u32 freq);
int afe_mem_asrc_set_obuf_freq(enum afe_mem_asrc_id id, u32 freq);
int afe_mem_asrc_register_irq(enum afe_mem_asrc_id id, irq_handler_t isr, const char *name, void *dev);
int afe_mem_asrc_unregister_irq(enum afe_mem_asrc_id id, void *dev);
#define IBUF_EMPTY_INT	 (0x1UL << 20)
#define IBUF_AMOUNT_INT  (0x1UL << 16)
#define OBUF_OV_INT      (0x1UL << 12)
#define OBUF_AMOUNT_INT  (0x1UL <<  8)
int afe_mem_asrc_irq_enable(enum afe_mem_asrc_id id, u32 interrupts, int en);
int afe_mem_asrc_irq_is_enabled(enum afe_mem_asrc_id id, u32 interrupt);
u32 afe_mem_asrc_irq_status(enum afe_mem_asrc_id id);
void afe_mem_asrc_irq_clear(enum afe_mem_asrc_id id, u32 status);
#define IBUF_AMOUNT_NOTIFY  (0)
#define OBUF_AMOUNT_NOTIFY  (0)


/**************** ultra low-power ****************/

#define LP_MODE_NORMAL		(0)
#define LP_MODE_LOWPOWER	(1)

enum lp_cpu_id {
	LP_CPU_AP = 0,
	LP_CPU_CM4 = 1
};

struct lp_region {
	enum lp_cpu_id turn;
	int interested[2];
};

struct lp_mode {
	struct lp_region region;
	unsigned int mode; /* updated by AP, LP_MODE_LOWPOWER or LP_MODE_NORMAL */
};

#define LP_CMD_NONE  (0)
#define LP_CMD_PLAYBACK_START  (1)
#define LP_CMD_PLAYBACK_STOP   (2)
#define LP_CMD_CAPTURE_START   (3)
#define LP_CMD_CAPTURE_STOP    (4)

struct lp_buffer {
	u32 base; /* AP view DRAM physical address, set by AP */
	u32 size; /* set by AP */
	u32 hw_pointer; /* AP view DRAM physical address, updated by CM4 */
	u32 appl_ofs;  /* in bytes, updated by AP according to appl_ptr */
	/*
	 * when appl_ofs is 0, the flag appl_is_in_buf_end indicates
	 * whether appl is in buf_start(appl_is_in_buf_end=0) or in buf_end(appl_is_in_buf_end=1)
	 */
	u32 appl_is_in_buf_end;
	struct lp_region appl_region;
};

struct lp_info {
	unsigned int cmd; /* set by AP, cleared by CM4 */
	struct lp_mode m; /* updated by AP, LP_MODE_LOWPOWER or LP_MODE_NORMAL */
	struct lp_buffer buf;
	unsigned int rate; /* set by AP */
	unsigned int channels; /* set by AP */
	unsigned int bitwidth; /* set by AP */
	unsigned int cm4_state; /* updated by CM4 */
};

int lp_configurate(volatile struct lp_info *lp, u32 base, u32 size,
		   unsigned int rate, unsigned int channels, unsigned int bitwidth);
u32 lp_hw_offset(volatile struct lp_info *lp);
int lp_cmd_excute(volatile struct lp_info *lp, unsigned int cmd);
int lp_switch_mode(unsigned int mode);


/**************** iec1 & iec2 ****************/
enum mt_stream_id;
struct snd_pcm_runtime;
void afe_iec_configurate(enum mt_stream_id id, int bitstream, struct snd_pcm_runtime *runtime);
void afe_iec_enable(enum mt_stream_id id);
void afe_iec_disable(enum mt_stream_id id);
int afe_iec_burst_info_is_ready(enum mt_stream_id id);
void afe_iec_burst_info_clear_ready(enum mt_stream_id id);
u32 afe_iec_burst_len(enum mt_stream_id id);
u32 afe_spdif_cur(enum mt_stream_id id);
u32 afe_spdif_base(enum mt_stream_id id);
u32 afe_spdif_end(enum mt_stream_id id);
u32 afe_iec_nsadr(enum mt_stream_id id);
void afe_iec_set_nsadr(enum mt_stream_id id, u32 nsadr);
int afe_iec_power_on(enum mt_stream_id id, int on);
void afe_iec_clock_on(enum mt_stream_id id, unsigned int mclk);
void afe_iec_clock_off(enum mt_stream_id id);

/**************** hdmi 8ch i2s ****************/

typedef enum {
	PCM_OUTPUT_8BIT = 0,
	PCM_OUTPUT_16BIT = 1,
	PCM_OUTPUT_24BIT = 2,
	PCM_OUTPUT_32BIT = 3
} SPDIF_PCM_BITWIDTH;

enum {
	HDMI_IN_I20 = 20,
	HDMI_IN_I21,
	HDMI_IN_I22,
	HDMI_IN_I23,
	HDMI_IN_I24,
	HDMI_IN_I25,
	HDMI_IN_I26,
	HDMI_IN_I27,
	HDMI_IN_BASE = HDMI_IN_I20,
	HDMI_IN_MAX = HDMI_IN_I27,
	HDMI_NUM_INPUT = (HDMI_IN_MAX - HDMI_IN_BASE + 1)
};

enum {
	HDMI_OUT_O20 = 20,
	HDMI_OUT_O21,
	HDMI_OUT_O22,
	HDMI_OUT_O23,
	HDMI_OUT_O24,
	HDMI_OUT_O25,
	HDMI_OUT_O26,
	HDMI_OUT_O27,
	HDMI_OUT_O28,
	HDMI_OUT_O29,
	HDMI_OUT_BASE = HDMI_OUT_O20,
	HDMI_OUT_MAX = HDMI_OUT_O29,
	HDMI_NUM_OUTPUT = (HDMI_OUT_MAX - HDMI_OUT_BASE + 1)
};

enum {
	HDMI_I2S_NOT_DELAY = 0,
	HDMI_I2S_FIRST_BIT_1T_DELAY
};

enum {
	HDMI_I2S_LRCK_NOT_INVERSE = 0,
	HDMI_I2S_LRCK_INVERSE
};

enum {
	HDMI_I2S_BCLK_NOT_INVERSE = 0,
	HDMI_I2S_BCLK_INVERSE
};

void afe_hdmi_i2s_set_interconnection(unsigned int channels, int connect);
void set_hdmi_out_control(unsigned int channels, unsigned int input_bit);
void set_hdmi_out_dsd_control(unsigned int channels, unsigned int dsd_bit);
void set_hdmi_out_control_enable(bool enable);
void set_hdmi_i2s(void);
void set_hdmi_i2s_dsd(void);
void set_hdmi_i2s_enable(bool enable);
void set_hdmi_i2s_to_i2s5(void);
void afe_hdmi_i2s_clock_on(unsigned int rate, unsigned int mclk,
	SPDIF_PCM_BITWIDTH eBitWidth, unsigned int DSDBCK);
void afe_hdmi_i2s_clock_off(void);
void init_hdmi_dma_buffer(u32 base, u32 size);

extern int iec1_chst_flag;
extern int iec2_chst_flag;


#endif
