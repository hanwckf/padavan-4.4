/*
 * Copyright (c) 2017 MediaTek Inc.
 * Author: Bayi Cheng <bayi.cheng@mediatek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/slab.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/dma-mapping.h>
#include <linux/of_device.h>
#include <linux/clk.h>
#include <linux/pm_runtime.h>
#include "bmt.h"
#include "mtk_ecc.h"
#include "mtk_snand.h"
#include "mtk_partition.h"

#define MODULE_NAME  "# MTK SNAND #"
#define __INTERNAL_USE_AHB_MODE__   (1)

/* delay latency */
#define SNAND_DEV_RESET_LATENCY_US	(3 * 10)

#define SNAND_MAX_ID		3
#define SNAND_TEST		0

/* NFI_DEBUG_CON1 */
#define DEBUG_CON1_BYPASS_MASTER_EN	(0x8000)

/* NFI_SPIDMA_REG32 */
#define SPIDMA_SEC_EN		(0x00010000)
#define SPIDMA_SEC_SIZE_MASK	(0x0000FFFF)

/* NFI_MASTERSTA */
#define MASTERSTA_MASK		(0x0FFF)

/* NFI_CNFG */
#define CNFG_AHB		(0x0001)
#define CNFG_READ_EN		(0x0002)
#define CNFG_DMA_BURST_EN	(0x0004)
#define CNFG_BYTE_RW		(0x0040)
#define CNFG_HW_ECC_EN		(0x0100)
#define CNFG_AUTO_FMT_EN	(0x0200)
#define CNFG_OP_IDLE		(0x0000)
#define CNFG_OP_READ		(0x1000)
#define CNFG_OP_SRD		(0x2000)
#define CNFG_OP_PRGM		(0x3000)
#define CNFG_OP_ERASE		(0x4000)
#define CNFG_OP_RESET		(0x5000)
#define CNFG_OP_CUST		(0x6000)
#define CNFG_OP_MODE_MASK	(0x7000)
#define CNFG_OP_MODE_SHIFT	(12)

/* NFI_PAGEFMT */
#define PAGEFMT_512		(0x0000)
#define PAGEFMT_2K		(0x0001)
#define PAGEFMT_4K		(0x0002)

#define PAGEFMT_PAGE_MASK	(0x0003)

#define PAGEFMT_SEC_SEL_512	(0x0004)

#define PAGEFMT_DBYTE_EN	(0x0008)

#define PAGEFMT_SPARE_16	(0x0000)
#define PAGEFMT_SPARE_26	(0x0001)
#define PAGEFMT_SPARE_27	(0x0002)
#define PAGEFMT_SPARE_28	(0x0003)
#define PAGEFMT_SPARE_32	(0x0004)
#define PAGEFMT_SPARE_MASK	(0x00F0)
#define PAGEFMT_SPARE_SHIFT	(4)

#define PAGEFMT_FDM_MASK	(0x0F00)
#define PAGEFMT_FDM_SHIFT	(8)

#define PAGEFMT_FDM_ECC_MASK	(0xF000)
#define PAGEFMT_FDM_ECC_SHIFT	(12)

/* NFI_CON */
#define CON_FIFO_FLUSH		(0x0001)
#define CON_NFI_RST		(0x0002)
#define CON_NFI_SRD		(0x0010)

#define CON_NFI_NOB_MASK	(0x00E0)
#define CON_NFI_NOB_SHIFT	(5)

#define CON_NFI_BRD		(0x0100)
#define CON_NFI_BWR		(0x0200)

#define CON_NFI_SEC_MASK	(0x1F000)
#define CON_NFI_SEC_SHIFT	(12)

/* NFI_ACCCON */
#define ACCCON_SETTING		()

/* NFI_INTR_EN */
#define INTR_RD_DONE_EN		(0x0001)
#define INTR_WR_DONE_EN		(0x0002)
#define INTR_RST_DONE_EN	(0x0004)
#define INTR_ERASE_DONE_EN	(0x0008)
#define INTR_BSY_RTN_EN		(0x0010)
#define INTR_ACC_LOCK_EN	(0x0020)
#define INTR_AHB_DONE_EN	(0x0040)
#define INTR_ALL_INTR_DE	(0x0000)
#define INTR_ALL_INTR_EN	(0x007F)
#define INTR_CUSTOM_PROG_DONE_INTR_EN	(0x00000080)
#define INTR_AUTO_PROG_DONE_INTR_EN	(0x00000200)
#define INTR_AUTO_BLKER_INTR_EN	(0x00000800)

/* NFI_INTR */
#define INTR_RD_DONE		(0x0001)
#define INTR_WR_DONE		(0x0002)
#define INTR_RST_DONE		(0x0004)
#define INTR_ERASE_DONE		(0x0008)
#define INTR_BSY_RTN		(0x0010)
#define INTR_ACC_LOCK		(0x0020)
#define INTR_AHB_DONE		(0x0040)

/* NFI_ADDRNOB */
#define ADDR_COL_NOB_MASK	(0x0007)
#define ADDR_COL_NOB_SHIFT	(0)
#define ADDR_ROW_NOB_MASK	(0x0070)
#define ADDR_ROW_NOB_SHIFT	(4)

/* NFI_STA */
#define STA_READ_EMPTY		(0x00001000)
#define STA_ACC_LOCK		(0x00000010)
#define STA_CMD_STATE		(0x00000001)
#define STA_ADDR_STATE		(0x00000002)
#define STA_DATAR_STATE		(0x00000004)
#define STA_DATAW_STATE		(0x00000008)

#define STA_NAND_FSM_MASK	(0x3F800000)
#define STA_NAND_BUSY		(0x00000100)
#define STA_NAND_BUSY_RETURN	(0x00000200)
#define STA_NFI_FSM_MASK	(0x000F0000)
#define STA_NFI_OP_MASK		(0x0000000F)

/* NFI_FIFOSTA */
#define FIFO_RD_EMPTY		(0x0040)
#define FIFO_RD_FULL		(0x0080)
#define FIFO_WR_FULL		(0x8000)
#define FIFO_WR_EMPTY		(0x4000)
#define FIFO_RD_REMAIN(x)	(0x1F&(x))
#define FIFO_WR_REMAIN(x)	((0x1F00&(x))>>8)

/* NFI_ADDRCNTR */
#define ADDRCNTR_CNTR(x)	((0x1F000&(x))>>12)
#define ADDRCNTR_OFFSET(x)	(0x0FFF&(x))

/* NFI_LOCK */
#define NFI_LOCK_ON		(0x0001)

/* NFI_LOCKANOB */
#define PROG_RADD_NOB_MASK	(0x7000)
#define PROG_RADD_NOB_SHIFT	(12)
#define PROG_CADD_NOB_MASK	(0x0300)
#define PROG_CADD_NOB_SHIFT	(8)
#define ERASE_RADD_NOB_MASK	(0x0070)
#define ERASE_RADD_NOB_SHIFT	(4)
#define ERASE_CADD_NOB_MASK	(0x0007)
#define ERASE_CADD_NOB_SHIFT	(0)

/* NFI_DEBUG_CON1 */
#define HWDCM_SWCON_ON		(1<<1)
#define WBUF_EN			(1<<2)
#define NFI_BYPASS		0x8000
#define ECC_BYPASS		0x1

/* SPI-NAND dummy command for NFI */
#define NAND_CMD_DUMMYREAD	0x00
#define NAND_CMD_DUMMYPROG	0x80

/* ECC_BYPASS_REG32 */

/* ECC_ENCON */
#define ECC_PARITY_BIT		(14)

#define ENC_EN			(0x0001)
#define ENC_DE			(0x0000)

/* ECC_ENCCNFG */
#define ECC_CNFG_ECC4		(0x0000)
#define ECC_CNFG_ECC6		(0x0001)
#define ECC_CNFG_ECC8		(0x0002)
#define ECC_CNFG_ECC10		(0x0003)
#define ECC_CNFG_ECC12		(0x0004)
#define ECC_CNFG_ECC_MASK	(0x0000001F)

#define ENC_CNFG_NFI		(0x0010)
#define ENC_CNFG_MODE_MASK	(0x0060)

#define ENC_CNFG_META6		(0x10300000)
#define ENC_CNFG_META8		(0x10400000)

#define ENC_CNFG_MSG_MASK	(0x3FFF0000)
#define ENC_CNFG_MSG_SHIFT	(0x10)

/* ECC_ENCIDLE */
#define ENC_IDLE		(0x0001)

/* ECC_ENCSTA */
#define STA_FSM			(0x0007)
#define STA_COUNT_PS		(0xFF10)
#define STA_COUNT_MS		(0x3FFF0000)

/* ECC_ENCIRQEN */
#define ENC_IRQEN		(0x0001)

/* ECC_ENCIRQSTA */
#define ENC_IRQSTA		(0x0001)

/* ECC_DECCON */
#define DEC_EN			(0x0001)
#define DEC_DE			(0x0000)

/* ECC_ENCCNFG */
#define DEC_CNFG_ECC4          (0x0000)

#define DEC_CNFG_DEC_MODE_MASK (0x0060)
#define DEC_CNFG_AHB           (0x0000)
#define DEC_CNFG_NFI           (0x0010)  /* changed by bayi */

#define DEC_CNFG_FER           (0x01000)
#define DEC_CNFG_EL            (0x02000)
#define DEC_CNFG_CORRECT       (0x03000)
#define DEC_CNFG_TYPE_MASK     (0x03000)

#define DEC_CNFG_EMPTY_EN      (0x80000000)
#define DEC_CNFG_DEC_BURST_EN  (0x00000100)

#define DEC_CNFG_CODE_MASK     (0x3FFF0000)
#define DEC_CNFG_CODE_SHIFT    (0x10)

/* ECC_DECIDLE */
#define DEC_IDLE		(0x0001)

/* ECC_DECFER */
#define DEC_FER0               (0x0001)
#define DEC_FER1               (0x0002)
#define DEC_FER2               (0x0004)
#define DEC_FER3               (0x0008)
#define DEC_FER4               (0x0010)
#define DEC_FER5               (0x0020)
#define DEC_FER6               (0x0040)
#define DEC_FER7               (0x0080)

/* ECC_DECENUM */
#define ERR_NUM0               (0x0000003F)
#define ERR_NUM1               (0x00003F00)
#define ERR_NUM2               (0x003F0000)
#define ERR_NUM3               (0x3F000000)
#define ERR_NUM4               (0x0000003F)
#define ERR_NUM5               (0x00003F00)
#define ERR_NUM6               (0x003F0000)
#define ERR_NUM7               (0x3F000000)

/* ECC_DECDONE */
#define DEC_DONE0               (0x0001)
#define DEC_DONE1               (0x0002)
#define DEC_DONE2               (0x0004)
#define DEC_DONE3               (0x0008)
#define DEC_DONE4               (0x0010)
#define DEC_DONE5               (0x0020)
#define DEC_DONE6               (0x0040)
#define DEC_DONE7               (0x0080)

/* Standard Commands for SPI NAND */
#define SNAND_CMD_DIE_SELECT                (0xC2)
#define SNAND_CMD_BLOCK_ERASE               (0xD8)
#define SNAND_CMD_GET_FEATURES              (0x0F)
#define SNAND_CMD_FEATURES_BLOCK_LOCK       (0xA0)
#define SNAND_CMD_FEATURES_OTP              (0xB0)
#define SNAND_CMD_FEATURES_STATUS           (0xC0)
#define SNAND_CMD_PAGE_READ                 (0x13)
#define SNAND_CMD_PROGRAM_EXECUTE           (0x10)
#define SNAND_CMD_PROGRAM_LOAD              (0x02)
#define SNAND_CMD_PROGRAM_LOAD_X4           (0x32)
#define SNAND_CMD_READ_ID                   (0x9F)
#define SNAND_CMD_RANDOM_READ               (0x03)
#define SNAND_CMD_RANDOM_READ_SPIQ          (0x6B)
#define SNAND_CMD_SET_FEATURES              (0x1F)

#define SNAND_CMD_SW_RESET                  (0xFF)
#define SNAND_CMD_WRITE_ENABLE              (0x06)

/* Status register */
#define SNAND_STATUS_OIP                    (0x01)
#define SNAND_STATUS_ERASE_FAIL             (0x04)
#define SNAND_STATUS_PROGRAM_FAIL           (0x08)

/* OTP register */
#define SNAND_OTP_ECC_ENABLE                (0x10)
#define SNAND_OTP_QE                        (0x01)

/* Block lock register */
#define SNAND_BLOCK_LOCK_BITS               (0x7E)

#define SNF_BASE                            (NFI_BASE)

/* RW_SNAND_DLY_CTL2 */
#define SNAND_SFIO0_IN_DLY_MASK       (0x0000003F)
#define SNAND_SFIO1_IN_DLY_MASK       (0x00003F00)
#define SNAND_SFIO2_IN_DLY_MASK       (0x003F0000)
#define SNAND_SFIO3_IN_DLY_MASK       (0x3F000000)

/* RW_SNAND_DLY_CTL3 */
#define SNAND_SFCK_OUT_DLY_MASK       (0x00003F00)
#define SNAND_SFCK_OUT_DLY_OFFSET     (8)
#define SNAND_SFCK_SAM_DLY_MASK       (0x0000003F)
#define SNAND_SFCK_SAM_DLY_OFFSET     (0)
#define SNAND_SFIFO_WR_EN_DLY_SEL_MASK (0x3F000000)

/* RW_SNAND_RD_CTL1 */
#define SNAND_PAGE_READ_CMD_OFFSET    (24)
#define SNAND_PAGE_READ_ADDRESS_MASK  (0x00FFFFFF)

/* RW_SNAND_RD_CTL2 */
#define SNAND_DATA_READ_DUMMY_OFFSET  (8)
#define SNAND_DATA_READ_CMD_MASK      (0x000000FF)

/* RW_SNAND_RD_CTL3 */
#define SNAND_DATA_READ_ADDRESS_MASK  (0x0000FFFF)

/* RW_SNAND_MISC_CTL */
#define SNAND_DATA_READ_MODE_X1       (0x0)
#define SNAND_DATA_READ_MODE_X4       (0x2)
#define SNAND_CLK_INVERSE             (0x20)
#define SNAND_SAMPLE_CLK_INVERSE      (1 << 22)
#define SNAND_4FIFO_EN                (1 << 24)
#define SNAND_DATA_READ_MODE_OFFSET   (16)
#define SNAND_DATA_READ_MODE_MASK     (0x00070000)
#define SNAND_FIFO_RD_LTC_MASK        (0x06000000)
#define SNAND_FIFO_RD_LTC_OFFSET      (25)
#define SNAND_FIFO_RD_LTC_0           (0)
#define SNAND_FIFO_RD_LTC_2           (2)
#define SNAND_PG_LOAD_X4_EN           (1 << 20)
#define SNAND_DATARD_CUSTOM_EN        (0x00000040)
#define SNAND_PG_LOAD_CUSTOM_EN       (0x00000080)
#define SNAND_SW_RST                  (0x10000000)
#define SNAND_LATCH_LAT_MASK          (0x00000300)
#define SNAND_LATCH_LAT_OFFSET        (8)

/* RW_SNAND_MISC_CTL2 */
#define SNAND_PROGRAM_LOAD_BYTE_LEN_OFFSET    (16)
#define SNAND_READ_DATA_BYTE_LEN_OFFSET       (0)

/* RW_SNAND_PG_CTL1 */
#define SNAND_PG_EXE_CMD_OFFSET               (16)
#define SNAND_PG_LOAD_CMD_OFFSET              (8)
#define SNAND_PG_WRITE_EN_CMD_OFFSET          (0)

/* RW_SNAND_PG_CTL2 */
#define SNAND_PG_LOAD_CMD_DUMMY_OUT_OFFSET    (12)
#define SNAND_PG_LOAD_ADDR_MASK               (0x0000FFFF)

/* RW_SNAND_GF_CTL1 */
#define SNAND_GF_STATUS_MASK          (0x000000FF)

/* RW_SNAND_GF_CTL3 */
#define SNAND_GF_LOOP_LIMIT_MASK      (0x000F0000)
#define SNAND_GF_POLLING_CYCLE_MASK   (0x0000FFFF)
#define SNAND_GF_LOOP_LIMIT_OFFSET    (16)

/* RW_SNAND_STA_CTL1 */
#define SNAND_AUTO_BLKER              (0x01000000)
#define SNAND_AUTO_READ               (0x02000000)
#define SNAND_AUTO_PROGRAM            (0x04000000)
#define SNAND_CUSTOM_READ             (0x08000000)
#define SNAND_CUSTOM_PROGRAM          (0x10000000)

/* RW_SNAND_STA_CTL2 */
#define SNAND_DATARD_BYTE_CNT_OFFSET  (16)
#define SNAND_DATARD_BYTE_CNT_MASK    (0x1FFF0000)

/* RW_SNAND_MAC_CTL */
#define SNAND_WIP                     (0x00000001)  /* b0 */
#define SNAND_WIP_READY               (0x00000002)  /* b1 */
#define SNAND_TRIG                    (0x00000004)  /* b2 */
#define SNAND_MAC_EN                  (0x00000008)  /* b3 */
#define SNAND_MAC_SIO_SEL             (0x00000010)  /* b4 */

/* RW_SNAND_DIRECT_CTL */
#define SNAND_QPI_EN                  (0x00000001)  /* b0 */
#define SNAND_CMD1_EXTADDR_EN         (0x00000002)  /* b1 */
#define SNAND_CMD2_EN                 (0x00000004)  /* b2 */
#define SNAND_CMD2_EXTADDR_EN         (0x00000008)  /* b3 */
#define SNAND_DR_MODE_MASK            (0x00000070)  /* b4~b6 */
#define SNAND_NO_RELOAD               (0x00000080)  /* b7 */
#define SNAND_DR_CMD2_DUMMY_CYC_MASK  (0x00000F00)  /* b8~b11 */
#define SNAND_DR_CMD1_DUMMY_CYC_MASK  (0x0000F000)  /* b12~b15 */
#define SNAND_DR_CMD2_DUMMY_CYC_OFFSET         (8)
#define SNAND_DR_CMD1_DUMMY_CYC_OFFSET        (12)
#define SNAND_DR_CMD2_MASK            (0x00FF0000)  /* b16~b23 */
#define SNAND_DR_CMD1_MASK            (0xFF000000)  /* b24~b31 */
#define SNAND_DR_CMD2_OFFSET                  (16)
#define SNAND_DR_CMD1_OFFSET                  (24)

/* RW_SNAND_ER_CTL */
#define SNAND_ER_CMD_OFFSET                    (8)
#define SNAND_ER_CMD_MASK             (0x0000FF00)
#define SNAND_AUTO_ERASE_TRIGGER      (0x00000001)

/* RW_SNAND_GF_CTL3 */
#define SNAND_LOOP_LIMIT_OFFSET               (16)
#define SNAND_LOOP_LIMIT_MASK         (0x000F0000)
#define SNAND_LOOP_LIMIT_NO_LIMIT            (0xF)
#define SNAND_POLLING_CYCLE_MASK      (0x0000FFFF)

#define SNAND_NFI_FDM_SIZE            (8)
#define SNAND_NFI_FDM_ECC_SIZE        (8)

#define SNAND_ADV_READ_SPLIT                (0x00000001)
#define SNAND_ADV_VENDOR_RESERVED_BLOCKS    (0x00000002)
#define SNAND_ADV_TWO_DIE                   (0x00000004)

typedef enum {
	SNAND_RB_DEFAULT    = 0,
	SNAND_RB_READ_ID    = 1,
	SNAND_RB_CMD_STATUS = 2,
	SNAND_RB_PIO        = 3
} SNAND_Read_Byte_Mode;

typedef enum {
	SNAND_IDLE	= 0,
	SNAND_DEV_ERASE_DONE,
	SNAND_DEV_PROG_DONE,
	SNAND_BUSY,
	SNAND_NFI_CUST_READING,
	SNAND_NFI_AUTO_ERASING,
	SNAND_DEV_PROG_EXECUTING,
} SNAND_Status;

static SNAND_Read_Byte_Mode    g_snand_read_byte_mode  = SNAND_RB_DEFAULT;
static SNAND_Status            g_snand_dev_status      = SNAND_IDLE;
static int                     g_snand_cmd_status      = NAND_STATUS_READY;
static int                     g_snand_erase_cmds;
static int                     g_snand_erase_addr;

static u8 g_snand_id_data[8];
static u8 g_snand_id_data_idx;

/* Read Split related definitions and variables */
#define SNAND_RS_BOUNDARY_KB			(1024)

#define SNAND_RS_SPARE_PER_SECTOR_PART0_VAL	(16)
#define SNAND_RS_SPARE_PER_SECTOR_PART0_NFI	(PAGEFMT_SPARE_16 \
						<< PAGEFMT_SPARE_SHIFT)
#define SNAND_RS_ECC_BIT_PART0                      (0)

static u32 g_snand_rs_ecc_bit_second_part;
static u32 g_snand_rs_spare_per_sector_second_part_nfi;
static u32 g_snand_rs_num_page;
static u32 g_snand_rs_cur_part;
static u32 g_snand_rs_ecc_bit;

static u8 g_snand_k_spare_per_sec;
#if __INTERNAL_USE_AHB_MODE__
static dma_addr_t dma_addr;
#endif

/* Supported SPI protocols */
typedef enum {
	SF_UNDEF = 0,
	SPI      = 1,
	SPIQ     = 2,
	QPI      = 3
} SNAND_Mode;

typedef struct {
	u8 id[SNAND_MAX_ID];
	u8 id_length;
	u16 totalsize;
	u16 blocksize;
	u16 pagesize;
	u16 sparesize;
	u32 SNF_DLY_CTL1;
	u32 SNF_DLY_CTL2;
	u32 SNF_DLY_CTL3;
	u32 SNF_DLY_CTL4;
	u32 SNF_MISC_CTL;
	u32 SNF_DRIVING_E4;
	u32 SNF_DRIVING_E8;
	u8 devicename[30];
	u32 advancedmode;
} snand_flashdev_info;

struct mtk_snfc_nand_chip {
	struct list_head node;
	struct nand_chip nand;
	u32 spare_per_sector;

	int nsels;
	u8 sels[0];
	/* nothing after this field */
};

struct mtk_snfc_clk {
	struct clk *nfi_clk;
	struct clk *pad_clk;
	struct clk *nfiecc_clk;
};

enum mtk_snfc_type {
	MTK_NAND_MT2701,
	MTK_NAND_MT7622,
	MTK_NAND_LEOPARD,
};

struct mtk_snand_type {
	int *nfi_regs;
	enum mtk_snfc_type type;
	int fdm_ecc_size;
	u32 no_bm_swap;
	u32 use_bmt;
};

struct mtk_snfc {
	struct nand_hw_control controller;
	struct mtk_snfc_clk clk;

	struct device *dev;
	void __iomem *regs;
	struct mtk_snand_type *chip_data;

	struct completion done;
	struct list_head chips;

	u8 *buffer;
};

struct NAND_CMD {
	u32	u4ColAddr;
	u32	u4RowAddr;
	u32	u4OOBRowAddr;
	u8	au1OOB[128];
	u8	*pDataBuf;
};

struct mtk_snand_host_hw mtk_nand_hw = {
	.nfi_bus_width          = 8,
	.nfi_access_timing      = 0x30c77fff,
	.nfi_cs_num             = 1,
	.nand_sec_size          = 512,
	.nand_sec_shift         = 9,
	.nand_ecc_size          = 2048,
	.nand_ecc_bytes         = 32,
	.nand_ecc_mode          = NAND_ECC_HW,
	.nand_fdm_size          = 8,
};

#define SNAND_MAX_PAGE_SIZE             (4096)
#define NAND_SECTOR_SIZE                (512)
#define OOB_PER_SECTOR                  (28)
#define OOB_AVAI_PER_SECTOR             (8)
#define _SNAND_CACHE_LINE_SIZE		(64)

static unsigned char __aligned(64) g_snand_k_spare[256];

static u8 *g_snand_k_temp;

#if defined(MTK_COMBO_NAND_SUPPORT)
	/* BMT_POOL_SIZE is not used anymore */
#else
	#ifndef PART_SIZE_BMTPOOL
	#define BMT_POOL_SIZE (80)
	#else
	#define BMT_POOL_SIZE (PART_SIZE_BMTPOOL)
	#endif
#endif

#define PMT_POOL_SIZE	(2)
struct nand_perf_log {
	unsigned int ReadPageCount;
	suseconds_t  ReadPageTotalTime;
	unsigned int ReadBusyCount;
	suseconds_t  ReadBusyTotalTime;
	unsigned int ReadDMACount;
	suseconds_t  ReadDMATotalTime;

	unsigned int WritePageCount;
	suseconds_t  WritePageTotalTime;
	unsigned int WriteBusyCount;
	suseconds_t  WriteBusyTotalTime;
	unsigned int WriteDMACount;
	suseconds_t  WriteDMATotalTime;

	unsigned int EraseBlockCount;
	suseconds_t  EraseBlockTotalTime;

	unsigned int ReadSectorCount;
	suseconds_t  ReadSectorTotalTime;
};

static struct nand_perf_log g_NandPerfLog = {0};

#define PFM_BEGIN(time)
#define PFM_END_R(time, n)
#define PFM_END_W(time, n)

static struct completion g_comp_AHB_Done;
static struct NAND_CMD g_kCMD;
static bool g_bInitDone;
static bool g_bHwEcc = 1;
static int g_i4Interrupt;

struct mtk_snand_host *host;
static u8 g_running_dma;

#define NAND_MAX_OOBSIZE 256 /* [Note: Bayi]fix me */
static u8 local_oob_buf[NAND_MAX_OOBSIZE];
static snand_flashdev_info devinfo;

#define SNFC_REG(snfc, x)  (snfc->regs + snfc->chip_data->nfi_regs[x])

enum nfi_regs {
	NFI_CNFG,
	NFI_PAGEFMT,
	NFI_CON,
	NFI_ACCCON,
	NFI_INTR_EN,
	NFI_INTR_STA,
	NFI_CMD,
	NFI_ADDRNOB,
	NFI_COLADDR,
	NFI_ROWADDR,
	NFI_STRDATA,
	NFI_CNRNB,
	NFI_DATAW,
	NFI_DATAR,
	NFI_PIO_DIRDY,
	NFI_STA,
	NFI_FIFOSTA,
	NFI_ADDRCNTR,
	NFI_STRADDR,
	NFI_BYTELEN,
	NFI_CSEL,
	NFI_FDM0L,
	NFI_FDM0M,
	NFI_FDM1L,
	NFI_FDM1M,
	NFI_FDM2L,
	NFI_FDM2M,
	NFI_FDM3L,
	NFI_FDM3M,
	NFI_FDM4L,
	NFI_FDM4M,
	NFI_FDM5L,
	NFI_FDM5M,
	NFI_FDM6L,
	NFI_FDM6M,
	NFI_FDM7L,
	NFI_FDM7M,
	NFI_FIFO_DATA0,
	NFI_FIFO_DATA1,
	NFI_FIFO_DATA2,
	NFI_FIFO_DATA3,
	NFI_DEBUG_CON1,
	NFI_MASTER_STA,
	NFI_SPIDMA,
	NFI_SPIADDRCNTR,
	NFI_EMPTY_THRESH,
	NFI_SNAND_MAC_CTL,
	NFI_SNAND_MAC_OUTL,
	NFI_SNAND_MAC_INL,
	NFI_SNAND_RD_CTL1,
	NFI_SNAND_RD_CTL2,
	NFI_SNAND_RD_CTL3,
	NFI_SNAND_GF_CTL1,
	NFI_SNAND_GF_CTL3,
	NFI_SNAND_PG_CTL1,
	NFI_SNAND_PG_CTL2,
	NFI_SNAND_PG_CTL3,
	NFI_SNAND_ER_CTL,
	NFI_SNAND_ER_CTL2,
	NFI_SNAND_MISC_CTL,
	NFI_SNAND_MISC_CTL2,
	NFI_SNAND_DLY_CTL1,
	NFI_SNAND_DLY_CTL2,
	NFI_SNAND_DLY_CTL3,
	NFI_SNAND_DLY_CTL4,
	NFI_SNAND_STA_CTL1,
	NFI_SNAND_STA_CTL2,
	NFI_SNAND_STA_CTL3,
	NFI_SNAND_CNFG,
	NFI_SNAND_GPRAM_DATA,
	NFI_SNAND_GPRAM_DATA1,

	ECC_ENCCON,
	ECC_ENCCNFG,
	ECC_ENCDIADDR,
	ECC_ENCIDLE,
	ECC_ENCPAR0,
	ECC_ENCPAR1,
	ECC_ENCPAR2,
	ECC_ENCPAR3,
	ECC_ENCPAR4,
	ECC_ENCPAR5,
	ECC_ENCPAR6,
	ECC_ENCSTA,
	ECC_ENCIRQEN,
	ECC_ENCIRQSTA,

	ECC_DECCON,
	ECC_DECCNFG,
	ECC_DECDIADDR,
	ECC_DECIDLE,
	ECC_DECFER,
	ECC_DECENUM0,
	ECC_DECENUM1,
	ECC_DECDONE,
	ECC_DECEL0,
	ECC_DECEL1,
	ECC_DECEL2,
	ECC_DECEL3,
	ECC_DECEL4,
	ECC_DECEL5,
	ECC_DECEL6,
	ECC_DECEL7,
	ECC_DECIRQEN,
	ECC_DECIRQSTA,
	ECC_FDMADDR,
	ECC_DECFSM,
	ECC_SYNSTA,
	ECC_DECNFIDI,
	ECC_SYN0,
	ECC_BYPASS_Reg,
};

static int mt7622_snfi_regs[] = {
	[NFI_CNFG]		= 0x0,
	[NFI_PAGEFMT]		= 0x4,
	[NFI_CON]		= 0x8,
	[NFI_ACCCON]		= 0xc,
	[NFI_INTR_EN]		= 0x10,
	[NFI_INTR_STA]		= 0x14,
	[NFI_CMD]		= 0x20,
	[NFI_ADDRNOB]		= 0x30,
	[NFI_COLADDR]		= 0x34,
	[NFI_ROWADDR]		= 0x38,
	[NFI_STRDATA]		= 0x40,
	[NFI_CNRNB]		= 0x44,
	[NFI_DATAW]		= 0x50,
	[NFI_DATAR]		= 0x54,
	[NFI_PIO_DIRDY]		= 0x58,
	[NFI_STA]		= 0x60,
	[NFI_FIFOSTA]		= 0x64,
	[NFI_ADDRCNTR]		= 0x70,
	[NFI_STRADDR]		= 0x80,
	[NFI_BYTELEN]		= 0x84,
	[NFI_CSEL]		= 0x90,
	[NFI_FDM0L]		= 0xa0,
	[NFI_FDM0M]		= 0xa4,
	[NFI_FDM1L]		= 0xa8,
	[NFI_FDM1M]		= 0xac,
	[NFI_FDM2L]		= 0xb0,
	[NFI_FDM2M]		= 0xb4,
	[NFI_FDM3L]		= 0xb8,
	[NFI_FDM3M]		= 0xbc,
	[NFI_FDM4L]		= 0xc0,
	[NFI_FDM4M]		= 0xc4,
	[NFI_FDM5L]		= 0xc8,
	[NFI_FDM5M]		= 0xcc,
	[NFI_FDM6L]		= 0xd0,
	[NFI_FDM6M]		= 0xd4,
	[NFI_FDM7L]		= 0xd8,
	[NFI_FDM7M]		= 0xdc,
	[NFI_FIFO_DATA0]	= 0x190,
	[NFI_FIFO_DATA1]	= 0x194,
	[NFI_FIFO_DATA2]	= 0x198,
	[NFI_FIFO_DATA3]	= 0x19c,
	[NFI_DEBUG_CON1]	= 0x220,
	[NFI_MASTER_STA]	= 0x224,
	[NFI_SPIDMA]		= 0x22C,
	[NFI_SPIADDRCNTR]	= 0x230,
	[NFI_EMPTY_THRESH]	= 0x23c,

	[NFI_SNAND_MAC_CTL]	= 0x500,
	[NFI_SNAND_MAC_OUTL]	= 0x504,
	[NFI_SNAND_MAC_INL]	= 0x508,

	[NFI_SNAND_RD_CTL1]	= 0x50C,
	[NFI_SNAND_RD_CTL2]	= 0x510,
	[NFI_SNAND_RD_CTL3]	= 0x514,

	[NFI_SNAND_GF_CTL1]	= 0x518,
	[NFI_SNAND_GF_CTL3]	= 0x520,

	[NFI_SNAND_PG_CTL1]	= 0x524,
	[NFI_SNAND_PG_CTL2]	= 0x528,
	[NFI_SNAND_PG_CTL3]	= 0x52C,

	[NFI_SNAND_ER_CTL]	= 0x530,
	[NFI_SNAND_ER_CTL2]	= 0x534,

	[NFI_SNAND_MISC_CTL]	= 0x538,
	[NFI_SNAND_MISC_CTL2]	= 0x53C,

	[NFI_SNAND_DLY_CTL1]	= 0x540,
	[NFI_SNAND_DLY_CTL2]	= 0x544,
	[NFI_SNAND_DLY_CTL3]	= 0x548,
	[NFI_SNAND_DLY_CTL4]	= 0x54C,

	[NFI_SNAND_STA_CTL1]	= 0x550,
	[NFI_SNAND_STA_CTL2]	= 0x554,
	[NFI_SNAND_STA_CTL3]	= 0x558,
	[NFI_SNAND_CNFG]	= 0x55C,

	[NFI_SNAND_GPRAM_DATA]	= 0x800,
	[NFI_SNAND_GPRAM_DATA1]	= 0x804,

	[ECC_ENCCON]	= 0x1000,
	[ECC_ENCCNFG]	= 0x1004,
	[ECC_ENCDIADDR]	= 0x1008,
	[ECC_ENCIDLE]	= 0x100C,
	[ECC_ENCPAR0]	= 0x1010,
	[ECC_ENCPAR1]	= 0x1014,
	[ECC_ENCPAR2]	= 0x1018,
	[ECC_ENCPAR3]	= 0x101C,
	[ECC_ENCPAR4]	= 0x1020,
	[ECC_ENCPAR5]	= 0x1024,
	[ECC_ENCPAR6]	= 0x1028,
	[ECC_ENCSTA]	= 0x102C,
	[ECC_ENCIRQEN]	= 0x1030,
	[ECC_ENCIRQSTA]	= 0x1034,

	[ECC_DECCON]	= 0x1100,
	[ECC_DECCNFG]	= 0x1104,
	[ECC_DECDIADDR]	= 0x1108,
	[ECC_DECIDLE]	= 0x110C,
	[ECC_DECFER]	= 0x1110,
	[ECC_DECENUM0]	= 0x1114,
	[ECC_DECENUM1]	= 0x1118,
	[ECC_DECDONE]	= 0x111C,
	[ECC_DECEL0]	= 0x1120,
	[ECC_DECEL1]	= 0x1124,
	[ECC_DECEL2]	= 0x1128,
	[ECC_DECEL3]	= 0x112C,
	[ECC_DECEL4]	= 0x1130,
	[ECC_DECEL5]	= 0x1134,
	[ECC_DECEL6]	= 0x1138,
	[ECC_DECEL7]	= 0x113C,
	[ECC_DECIRQEN]	= 0x1140,
	[ECC_DECIRQSTA]	= 0x1144,
	[ECC_FDMADDR]	= 0x1148,
	[ECC_DECFSM]	= 0x114C,
	[ECC_SYNSTA]	= 0x1150,
	[ECC_DECNFIDI]	= 0x1154,
	[ECC_SYN0]	= 0x1158,
	[ECC_BYPASS_Reg] = 0x120C,
};

static bool mtk_snand_reset_con(struct mtk_snfc *snfc);
static int mtk_nand_erase(struct mtd_info *mtd, int page);

static const snand_flashdev_info gen_snand_FlashTable[] = {
	{{0xEF, 0xAA, 0x20}, 3, 64, 128, 2048, 64, 0x00000000, 0x00000000, 0x00000028,
		0x00000000, 0x0552000A, 0x0000, 0x3F00, "Winbond 512Mb", 0x00000000},
	{{0xEF, 0xAA, 0x21}, 3, 128, 128, 2048, 64, 0x00000000, 0x00000000, 0x00000028,
		0x00000000, 0x0552000A, 0x0000, 0x3F00, "Winbond 1Gb", 0x00000000},
	{{0xEF, 0xAB, 0x21}, 3, 256, 128, 2048, 64, 0x00000000, 0x00000000, 0x00000028,
		0x00000000, 0x0552000A, 0x0000, 0x3F00, "Winbond 2Gb", SNAND_ADV_TWO_DIE},
	{{0xC8, 0xD4}, 2, 512, 256, 4096, 256, 0x00000000, 0x00000000, 0x00000014,
		0x00000000, 0x0552000A, 0x0000, 0x3F00, "GD5F4GQ4UBYIG", 0x00000000},
	{{0xC8, 0xF4}, 2, 512, 128, 2048, 64, 0x00000000, 0x00000000, 0x00000014,
		0x00000000, 0x0552000A, 0x0000, 0x3F00, "GD5F4GQ4UAYIG", 0x00000000},
	{{0xC8, 0xD1}, 2, 128, 128, 2048, 128, 0x00000000, 0x00000000, 0x00000014,
		0x00000000, 0x0552000A, 0x0000, 0x3F00, "GD5F1GQ4UX", 0x00000000},
	{{0xC8, 0xD9}, 2, 128, 128, 2048, 64, 0x00000000, 0x00000000, 0x00000014,
		0x00000000, 0x0552000A, 0x0000, 0x3F00, "GD5F1GQ4UE", 0x00000000},
	{{0xC8, 0xD2}, 2, 256, 128, 2048, 128, 0x00000000, 0x00000000, 0x00000014,
		0x00000000, 0x0552000A, 0x0000, 0x3F00, "GD5F2GQ4UX", 0x00000000},
	{{0xC8, 0x32}, 2, 256, 128, 2048, 64, 0x00000000, 0x00000000, 0x00000014,
		0x00000000, 0x0552000A, 0x0000, 0x3F00, "GD5F2GQ4UE", 0x00000000},
	{{0xC2, 0x22}, 2, 256, 128, 2048, 64, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x0552000A, 0x3F00, 0x0000, "MX35LF2GE4AB", 0x00000000},
	{{0xC2, 0x20}, 2, 256, 128, 2048, 64, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x0552000A, 0x3F00, 0x0000, "MX35LF2G14AC", 0x00000000},
	{{0xC2, 0x12}, 2, 128, 128, 2048, 64, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x0552000A, 0x3F00, 0x0000, "MX35LF1GE4AB", 0x00000000},
	{{0xC8, 0x21}, 2, 128, 128, 2048, 64, 0x00000000, 0x00000000, 0x00000028,
		0x00000000, 0x0552000A, 0x3F00, 0x0000, "F50L1G41A", 0x00000000},
	{{0xC8, 0x01}, 2, 128, 128, 2048, 64, 0x00000000, 0x00000000, 0x00000028,
		0x00000000, 0x0552000A, 0x3F00, 0x0000, "F50L1G41LB", 0x00000000},
	{{0xC8, 0x0a}, 2, 256, 128, 2048, 64, 0x00000000, 0x00000000, 0x00000028,
		0x00000000, 0x0552000A, 0x3F00, 0x0000, "F50L2G41A", SNAND_ADV_TWO_DIE},
	{{0x2C, 0x14}, 2, 128, 128, 2048, 128, 0x00000000, 0x00000000, 0x00000014,
		0x00000000, 0x0552000A, 0x3F00, 0x0000, "MT29F1G01ABAGD", 0x00000000},
	{{0x2C, 0x24}, 2, 256, 128, 2048, 128, 0x00000000, 0x00000000, 0x00000014,
		0x00000000, 0x0552000A, 0x3F00, 0x0000, "MT29F2G01ABAGD", 0x00000000},
	{{0x2C, 0x36}, 2, 512, 128, 2048, 128, 0x00000000, 0x00000000, 0x00000014,
		0x00000000, 0x0552000A, 0x3F00, 0x0000, "MT29F4G01ADAGD", 0x00000000},
	{{0x2C, 0x34}, 2, 512, 256, 4096, 256, 0x00000000, 0x00000000, 0x1A00001A,
		0x00000000, 0x0552000A, 0x00, 0x0000, "MT29F4G01ABAFD", 0x00000000},
	{{0x98, 0xC2}, 2, 128, 128, 2048, 128, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x0552000A, 0x3F00, 0x0000, "TC58CVG0S3HRAIG", 0x00000000},
	{{0x98, 0xCB}, 2, 256, 128, 2048, 128, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x0552000A, 0x3F00, 0x0000, "TC58CVG1S3HRAIG", 0x00000000},
	{{0x98, 0xCD}, 2, 512, 256, 4096, 256, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x0552000A, 0x3F00, 0x0000, "TC58CVG2S3HRAIG", 0x00000000},
	{{0x98, 0xE2}, 2, 128, 128, 2048, 128, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x0552000A, 0x3F00, 0x0000, "TC58CVG0S3HRAIJ", 0x00000000},
	{{0x98, 0xEB}, 2, 256, 128, 2048, 128, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x0552000A, 0x3F00, 0x0000, "TC58CVG1S3HRAIJ", 0x00000000},
	{{0x98, 0xED}, 2, 512, 256, 4096, 256, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x0552000A, 0x3F00, 0x0000, "TC58CVG2S0HRAIJ", 0x00000000},
	{{0x98, 0xE4}, 2, 1024, 256, 4096, 256, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x0552000A, 0x3F00, 0x0000, "TH58CVG3S0HRAIJ", 0x00000000},
	{{0xD5, 0x11}, 2, 128, 128, 2048, 120, 0x00000000, 0x00000000, 0x00000014,
		0x00000000, 0x0552000A, 0x3F00, 0x0000, "EM73C01G44SNB", 0x00000000},
	{{0xD5, 0x12}, 2, 256, 128, 2048, 128, 0x00000000, 0x00000000, 0x00000014,
		0x00000000, 0x0552000A, 0x3F00, 0x0000, "EM73D02G44SNA", 0x00000000},
	{{0xD5, 0x03}, 2, 512, 256, 4096, 256, 0x00000000, 0x00000000, 0x00000014,
		0x00000000, 0x0552000A, 0x3F00, 0x0000, "EM73E04G44SNA", 0x00000000},
	{{0xD5, 0x1D}, 2, 128, 128, 2048, 64, 0x00000000, 0x00000000, 0x00000014,
		0x00000000, 0x0552000A, 0x3F00, 0x0000, "EM73F044SND", 0x00000000},
	{{0xD5, 0x1C}, 2, 128, 128, 2048, 64, 0x00000000, 0x00000000, 0x00000014,
		0x00000000, 0x0552000A, 0x3F00, 0x0000, "EM73F044VCD", 0x00000000},
	{{0xD5, 0x10}, 2, 256, 128, 2048, 128, 0x00000000, 0x00000000, 0x00000014,
		0x00000000, 0x0552000A, 0x3F00, 0x0000, "EM73F044SNF", 0x00000000},
	{{0xD5, 0x1F}, 2, 256, 128, 2048, 64, 0x00000000, 0x00000000, 0x00000014,
		0x00000000, 0x0552000A, 0x3F00, 0x0000, "EM73D044VCG", 0x00000000},
	{{0xD5, 0x1B}, 2, 256, 128, 2048, 64, 0x00000000, 0x00000000, 0x00000014,
		0x00000000, 0x0552000A, 0x3F00, 0x0000, "EM73F044VCH", 0x00000000},
	{{0xD5, 0x01}, 2, 64, 128, 2048, 64, 0x00000000, 0x00000000, 0x00000014,
		0x00000000, 0x0552000A, 0x3F00, 0x0000, "EM73B044VCA", 0x00000000},
	{{0xD5, 0x24}, 2, 1024, 256, 4096, 256, 0x00000000, 0x00000000, 0x00000028,
		0x00000000, 0x0552000A, 0x3F00, 0x0000, "EM73F044SNA", 0x00000000},
	{{0xD5, 0x2D}, 2, 1024, 256, 4096, 256, 0x00000000, 0x00000000, 0x00000028,
		0x00000000, 0x0552000A, 0x3F00, 0x0000, "EM73F044VCA", 0x00000000},
	{{0x6B, 0x01}, 2, 256, 128, 2048, 128, 0x00000000, 0x00000000, 0x1A00001A,
		0x00000000, 0x0552000A, 0x0, 0x0, "CS11G1T0A0AA", 0x00000000},
	{{0x6B, 0x02}, 2, 512, 128, 2048, 128, 0x00000000, 0x00000000, 0x1A00001A,
		0x00000000, 0x0552000A, 0x01, 0x0, "CS11G2T0A0AA", 0x00000000},
	{{0x6B, 0x00}, 2, 128, 128, 2048, 128, 0x00000000, 0x00000000, 0x1A00001A,
		0x00000000, 0x0552000A, 0x01, 0x0, "CS11G0T0A0AA", 0x00000000},
	{{0x6B, 0x10}, 2, 128, 128, 2048, 128, 0x00000000, 0x00000000, 0x1A00001A,
		0x00000000, 0x0552000A, 0x01, 0x0, "CS11G0G0A0AA", 0x00000000},
	{{0x6B, 0x20}, 2, 128, 128, 2048, 64, 0x00000000, 0x00000000, 0x1A00001A,
		0x00000000, 0x0552000A, 0x01, 0x0, "CS11G0S0A0AA", 0x00000000},
	{{0x6B, 0x21}, 2, 256, 128, 2048, 64, 0x00000000, 0x00000000, 0x1A00001A,
		0x00000000, 0x0552000A, 0x01, 0x0, "CS11G1S0A0AA", 0x00000000},
	{{0x6B, 0x22}, 2, 512, 128, 2048, 64, 0x00000000, 0x00000000, 0x1A00001A,
		0x00000000, 0x0552000A, 0x01, 0x0, "CS11G2G0A0AA", 0x00000000},
};

void  bm_swap(struct mtd_info *mtd, u8 *pFDMBuf, u8 *buf)
{
	struct nand_chip *chip = mtd_to_nand(mtd);
	struct mtk_snfc *snfc = nand_get_controller_data(chip);
	u8 *pSw1, *pSw2, tmp;
	int sec_num = mtd->writesize / 512;

	if (snfc->chip_data->no_bm_swap)
		return;

	/* the data */
	pSw1 = buf + mtd->writesize - ((mtd->oobsize * (sec_num - 1) / sec_num));
	/* the first FDM byte of the last sector */
	pSw2 = pFDMBuf + (8 * (sec_num - 1));

	tmp = *pSw1;
	*pSw1 = *pSw2;
	*pSw2 = tmp;
}

static inline void snfi_writel(struct mtk_snfc *snfc, u32 val,
			       enum nfi_regs reg)
{
	writel(val, SNFC_REG(snfc, reg));
}

static inline void snfi_writew(struct mtk_snfc *snfc, u16 val,
			       enum nfi_regs reg)
{
	writew(val, SNFC_REG(snfc, reg));
}

static inline void snfi_writeb(struct mtk_snfc *snfc, u8 val, enum nfi_regs reg)
{
	writeb(val, SNFC_REG(snfc, reg));
}

static inline u32 snfi_readl(struct mtk_snfc *snfc, enum nfi_regs reg)
{
	return readl_relaxed(SNFC_REG(snfc, reg));
}

static inline u16 snfi_readw(struct mtk_snfc *snfc, enum nfi_regs reg)
{
	return readw_relaxed(SNFC_REG(snfc, reg));
}

static inline u8 snfi_readb(struct mtk_snfc *snfc, enum nfi_regs reg)
{
	return readb_relaxed(SNFC_REG(snfc, reg));
}

static void mtk_snand_dump_reg(struct mtk_snfc *snfc)
{
	pr_warn("~~~~Dump NFI/SNF/GPIO Register in Kernel~~~~\n");
	pr_warn("CNFG_REG16: 0x%x\n", snfi_readw(snfc, NFI_CNFG));
	pr_warn("PAGEFMT_REG16: 0x%x\n", snfi_readw(snfc, NFI_PAGEFMT));
	pr_warn("CON_REG16: 0x%x\n", snfi_readw(snfc, NFI_CON));
	pr_warn("ACCCON: 0x%x\n", snfi_readl(snfc, NFI_ACCCON));
	pr_warn("INTR_EN_REG16: 0x%x\n", snfi_readw(snfc, NFI_INTR_EN));
	pr_warn("INTR_REG16: 0x%x\n", snfi_readw(snfc, NFI_INTR_STA));
	pr_warn("CMD_REG16: 0x%x\n", snfi_readw(snfc, NFI_CMD));
	pr_warn("ADDRNOB_REG32: 0x%x\n", snfi_readl(snfc, NFI_ADDRNOB));
	pr_warn("ROWADDR_REG32: 0x%x\n", snfi_readl(snfc, NFI_ROWADDR));
	pr_warn("STRDATA_REG16: 0x%x\n", snfi_readw(snfc, NFI_STRDATA));
	pr_warn("CNRNB_REG16: 0x%x\n", snfi_readl(snfc, NFI_CNRNB));
	pr_warn("DATAW_REG32: 0x%x\n", snfi_readl(snfc, NFI_DATAW));
	pr_warn("DATAR_REG32: 0x%x\n", snfi_readl(snfc, NFI_DATAR));
	pr_warn("PIO_DIRDY_REG16: 0x%x\n", snfi_readw(snfc, NFI_PIO_DIRDY));
	pr_warn("STA_REG32: 0x%x\n", snfi_readl(snfc, NFI_STA));
	pr_warn("FIFOSTA_REG16: 0x%x\n", snfi_readw(snfc, NFI_FIFOSTA));
	pr_warn("ADDRCNTR_REG16: 0x%x\n", snfi_readw(snfc, NFI_ADDRCNTR));
	pr_warn("STRADDR_REG32: 0x%x\n", snfi_readl(snfc, NFI_STRADDR));
	pr_warn("BYTELEN_REG16: 0x%x\n", snfi_readw(snfc, NFI_BYTELEN));
	pr_warn("CSEL_REG16: 0x%x\n",  snfi_readw(snfc, NFI_CSEL));
	pr_warn("FDM0L_REG32: 0x%x\n",  snfi_readl(snfc, NFI_FDM0L));
	pr_warn("FDM0M_REG32: 0x%x\n",  snfi_readl(snfc, NFI_FDM0M));
	pr_warn("MASTERSTA_REG16: 0x%x\n",  snfi_readw(snfc, NFI_MASTER_STA));

	pr_warn("DEBUG_CON1_REG16: 0x%x\n", snfi_readl(snfc, NFI_DEBUG_CON1));
	pr_warn("SPIADDRCNTR_REG32 0x%x\n", snfi_readl(snfc, NFI_SPIADDRCNTR));

	pr_warn("DECCNFG_REG32: 0x%x\n",  snfi_readl(snfc, ECC_DECCNFG));
	pr_warn("DECENUM0_REG32: 0x%x\n",  snfi_readl(snfc, ECC_DECENUM0));
	pr_warn("DECENUM1_REG32: 0x%x\n",  snfi_readl(snfc, ECC_DECENUM1));
	pr_warn("DECDONE_REG16: 0x%x\n",  snfi_readw(snfc, ECC_DECDONE));
	pr_warn("ENCCNFG_REG32: 0x%x\n", snfi_readl(snfc, ECC_ENCCNFG));

	pr_warn("MAC_CTL: 0x%x\n",  snfi_readl(snfc, NFI_SNAND_MAC_CTL));
	pr_warn("MAC_OUTL: 0x%x\n", snfi_readl(snfc, NFI_SNAND_MAC_OUTL));
	pr_warn("MAC_INL: 0x%x\n", snfi_readl(snfc, NFI_SNAND_MAC_INL));

	pr_warn("RD_CTL1: 0x%x\n", snfi_readl(snfc, NFI_SNAND_RD_CTL1));
	pr_warn("RD_CTL2: 0x%x\n", snfi_readl(snfc, NFI_SNAND_RD_CTL2));
	pr_warn("RD_CTL3: 0x%x\n", snfi_readl(snfc, NFI_SNAND_RD_CTL3));

	pr_warn("GF_CTL1: 0x%x\n", snfi_readl(snfc, NFI_SNAND_GF_CTL1));
	pr_warn("GF_CTL3: 0x%x\n", snfi_readl(snfc, NFI_SNAND_GF_CTL3));

	pr_warn("PG_CTL1: 0x%x\n", snfi_readl(snfc, NFI_SNAND_PG_CTL1));
	pr_warn("PG_CTL2: 0x%x\n", snfi_readl(snfc, NFI_SNAND_PG_CTL2));
	pr_warn("PG_CTL3: 0x%x\n", snfi_readl(snfc, NFI_SNAND_PG_CTL3));

	pr_warn("ER_CTL: 0x%x\n",  snfi_readl(snfc, NFI_SNAND_ER_CTL));
	pr_warn("ER_CTL2: 0x%x\n",  snfi_readl(snfc, NFI_SNAND_ER_CTL2));

	pr_warn("MISC_CTL: 0x%x\n",  snfi_readl(snfc, NFI_SNAND_MISC_CTL));
	pr_warn("MISC_CTL2: 0x%x\n", snfi_readl(snfc, NFI_SNAND_MISC_CTL2));

	pr_warn("DLY_CTL1: 0x%x\n", snfi_readl(snfc, NFI_SNAND_DLY_CTL1));
	pr_warn("DLY_CTL2: 0x%x\n", snfi_readl(snfc, NFI_SNAND_DLY_CTL2));
	pr_warn("DLY_CTL3: 0x%x\n", snfi_readl(snfc, NFI_SNAND_DLY_CTL3));
	pr_warn("DLY_CTL4: 0x%x\n", snfi_readl(snfc, NFI_SNAND_DLY_CTL4));
	pr_warn("STA_CTL1: 0x%x\n", snfi_readl(snfc, NFI_SNAND_STA_CTL1));
	pr_warn("CNFG: 0x%x\n", snfi_readl(snfc, NFI_SNAND_CNFG));
}

static void mtk_snand_dump_mem(u32 *buf, u32 size)
{
	u32 i;

	for (i = 0; i < (size / sizeof(u32)); i++) {
		pr_debug("%08X ", buf[i]);

		if ((i % 8) == 7)
			pr_debug("\n");
	}
}

static void mtk_snand_transfer_oob(struct nand_chip *chip, uint8_t *buf)
{
	int i, j = 0;
	int sec_num = 1 << (chip->page_shift - 9);

	for (i = 0; i < (sec_num * OOB_AVAI_PER_SECTOR); i++) {
		if (i % 8 == 0) {
			buf[i] = 0xFF;
		} else {
			buf[i] = chip->oob_poi[j];
			j++;
		}
	}
}

static void mtk_snand_fill_oob(struct nand_chip *chip, uint8_t *buf)
{
	int i, j = 0;
	int sec_num = 1 << (chip->page_shift - 9);

	for (i = 0; i < (sec_num * OOB_AVAI_PER_SECTOR); i++) {
		if (i % 8 == 0) {
			continue;
		} else {
			chip->oob_poi[j] = buf[i];
			j++;
		}
	}
}

static suseconds_t Cal_timediff(struct timeval *end_time,
				struct timeval *start_time)
{
	struct timeval difference;

	difference.tv_sec = end_time->tv_sec - start_time->tv_sec;
	difference.tv_usec = end_time->tv_usec - start_time->tv_usec;

	while (difference.tv_usec < 0) {
		difference.tv_usec += 1000000;
		difference.tv_sec -= 1;
	}

	return 1000000LL*difference.tv_sec + difference.tv_usec;

}

static bool mtk_snand_get_device_info(u8 *id, snand_flashdev_info *devinfo)
{
	u32 i, m, n, mismatch, ret;
	u32 target = -1;
	u8 target_id_len = 0;

	for (i = 0; i < ARRAY_SIZE(gen_snand_FlashTable); i++) {
		mismatch = 0;
		for (m = 0; m < gen_snand_FlashTable[i].id_length; m++) {
			if (id[m] != gen_snand_FlashTable[i].id[m]) {
				mismatch = 1;
				break;
			}
		}
		if (mismatch == 0 && gen_snand_FlashTable[i].id_length
		    > target_id_len) {
			target = i;
			target_id_len = gen_snand_FlashTable[i].id_length;
		}
	}

	if (target != -1) {
		pr_warn("Recognize NAND: ID [");

		for (n = 0; n < gen_snand_FlashTable[target].id_length; n++) {
			devinfo->id[n] = gen_snand_FlashTable[target].id[n];
			pr_warn("%x ", devinfo->id[n]);
		}

		pr_warn("], [%s], Page[%d]B, Spare [%d]B Total [%d]MB\n",
			gen_snand_FlashTable[target].devicename,
			gen_snand_FlashTable[target].pagesize,
			gen_snand_FlashTable[target].sparesize,
			gen_snand_FlashTable[target].totalsize);
		devinfo->id_length = gen_snand_FlashTable[target].id_length;
		devinfo->blocksize = gen_snand_FlashTable[target].blocksize;
		devinfo->advancedmode =
			gen_snand_FlashTable[target].advancedmode;
		devinfo->pagesize = gen_snand_FlashTable[target].pagesize;
		devinfo->sparesize = gen_snand_FlashTable[target].sparesize;
		devinfo->totalsize = gen_snand_FlashTable[target].totalsize;

		memcpy(devinfo->devicename,
		       gen_snand_FlashTable[target].devicename,
		       sizeof(devinfo->devicename));

		devinfo->SNF_DLY_CTL1
			= gen_snand_FlashTable[target].SNF_DLY_CTL1;
		devinfo->SNF_DLY_CTL2
			= gen_snand_FlashTable[target].SNF_DLY_CTL2;
		devinfo->SNF_DLY_CTL3
			= gen_snand_FlashTable[target].SNF_DLY_CTL3;
		devinfo->SNF_DLY_CTL4
			= gen_snand_FlashTable[target].SNF_DLY_CTL4;
		devinfo->SNF_MISC_CTL
			= gen_snand_FlashTable[target].SNF_MISC_CTL;
		devinfo->SNF_DRIVING_E4
			= gen_snand_FlashTable[target].SNF_DRIVING_E4;
		devinfo->SNF_DRIVING_E8
			= gen_snand_FlashTable[target].SNF_DRIVING_E8;

		/* init read split boundary */
		g_snand_rs_num_page = SNAND_RS_BOUNDARY_KB * 1024
				      / devinfo->pagesize;
		g_snand_k_spare_per_sec = devinfo->sparesize
					  / (devinfo->pagesize
					  / NAND_SECTOR_SIZE);

		ret = 1;
	} else {
		pr_info("Not Support SPI-Nand device:[0x%x 0x%x]\n", id[0], id[1]);
		ret = 0;
	}

	return ret;
}

bool mtk_snand_is_vendor_reserved_blocks(u32 row_addr)
{
	u32 page_per_block = (devinfo.blocksize << 10) / devinfo.pagesize;
	u32 target_block = row_addr / page_per_block;

	if (devinfo.advancedmode & SNAND_ADV_VENDOR_RESERVED_BLOCKS) {
		if (target_block >= 2045 && target_block <= 2048)
			return 1;
	}

	return 0;
}

static void mtk_snand_dev_mac_enable(struct mtk_snfc *snfc, SNAND_Mode mode)
{
	u32 mac;

	mac = snfi_readl(snfc, NFI_SNAND_MAC_CTL);
	/* SPI */
	if (mode == SPI) {
		mac &= ~SNAND_MAC_SIO_SEL;
		mac |= SNAND_MAC_EN;
	} else {
		mac |= (SNAND_MAC_SIO_SEL | SNAND_MAC_EN);
	}

	snfi_writel(snfc, mac, NFI_SNAND_MAC_CTL);
}

static void mtk_snand_dev_mac_trigger(struct mtk_snfc *snfc)
{
	u32 mac;

	mac = snfi_readl(snfc, NFI_SNAND_MAC_CTL);

	/* Trigger SFI: Set TRIG and enable Macro mode */
	mac |= (SNAND_TRIG | SNAND_MAC_EN);
	snfi_writel(snfc, mac, NFI_SNAND_MAC_CTL);

	while (!(snfi_readl(snfc, NFI_SNAND_MAC_CTL) & SNAND_WIP_READY))
	;

	while ((snfi_readl(snfc, NFI_SNAND_MAC_CTL) & SNAND_WIP))
	;
}

static void mtk_snand_dev_mac_leave(struct mtk_snfc *snfc)
{
	u32 mac;
	/* clear SF_TRIG and leave mac mode */
	mac = snfi_readl(snfc, NFI_SNAND_MAC_CTL);

	/*
	* Clear following bits
	* SF_TRIG: Confirm the macro command sequence is completed
	* SNAND_MAC_EN: Leaves macro mode, and enters direct read mode
	* SNAND_MAC_SIO_SEL: Always reset quad macro control bit at the end
	*/
	mac &= ~(SNAND_TRIG | SNAND_MAC_EN | SNAND_MAC_SIO_SEL);
	snfi_writel(snfc, mac, NFI_SNAND_MAC_CTL);
}

static void mtk_snand_dev_mac_op(struct mtk_snfc *snfc, SNAND_Mode mode)
{
	mtk_snand_dev_mac_enable(snfc, mode);
	mtk_snand_dev_mac_trigger(snfc);
	mtk_snand_dev_mac_leave(snfc);
}

static void mtk_snand_dev_command_ext(struct mtk_snfc *snfc,
				      SNAND_Mode mode, const u8 cmd[],
				      u8 data[], const u32 outl,
				      const u32 inl)
{
	u32 tmp;
	u32 i, j, reg;
	u8  *p_tmp = (u8 *)(&tmp);

	/* Moving commands into SFI GPRAM */
	for (i = 0; i < outl; ) {
		for (j = 0, tmp = 0; i < outl && j < 4; i++, j++)
			p_tmp[j] = cmd[i];

		snfi_writel(snfc, tmp, NFI_SNAND_GPRAM_DATA);
	}

	snfi_writel(snfc, outl, NFI_SNAND_MAC_OUTL);
	snfi_writel(snfc, inl, NFI_SNAND_MAC_INL);
	mtk_snand_dev_mac_op(snfc, mode);

	/* for NULL data, this loop will be skipped */
	if (inl > 0)
		for (i = 0; i < 3; ++i, ++data) {
			reg = snfi_readl(snfc, NFI_SNAND_GPRAM_DATA);
			*data  = (reg >> ((i + outl) * 8)) & 0xFF;
		}

	/* For read id, When outl + inl grater than 3 */
	if ((outl + inl) > 3)
		for (i = 0; i < (outl + inl - 3); ++i, ++data) {
			reg = snfi_readl(snfc, NFI_SNAND_GPRAM_DATA1);
			*data  = reg >> ((i * 8)) & 0xFF;
		}

}

static void mtk_snand_dev_command(struct mtk_snfc *snfc, const u32 cmd,
				  u8 outlen)
{
	snfi_writel(snfc, cmd, NFI_SNAND_GPRAM_DATA);
	snfi_writel(snfc, outlen, NFI_SNAND_MAC_OUTL);
	snfi_writel(snfc, 0, NFI_SNAND_MAC_INL);
	mtk_snand_dev_mac_op(snfc, SPI);
}

static void mtk_snand_reset_dev(struct mtk_snfc *snfc)
{
	u8 cmd = SNAND_CMD_SW_RESET;
	u32 reg;

	mtk_snand_reset_con(snfc);
	/* issue SW RESET command to device */
	mtk_snand_dev_command_ext(snfc, SPI, &cmd, NULL, 1, 0);

	/* wait for awhile, then polling status register (required by spec) */
	udelay(SNAND_DEV_RESET_LATENCY_US);

	snfi_writel(snfc, (SNAND_CMD_GET_FEATURES
			   | (SNAND_CMD_FEATURES_STATUS << 8)),
			   NFI_SNAND_GPRAM_DATA);
	snfi_writel(snfc, 2, NFI_SNAND_MAC_OUTL);
	snfi_writel(snfc, 1, NFI_SNAND_MAC_INL);

	/* polling status register */
	for (;;) {
		mtk_snand_dev_mac_op(snfc, SPI);

		reg = snfi_readl(snfc, NFI_SNAND_GPRAM_DATA);
		cmd  = (reg>>16)&0xFF;

		if (0 == (cmd & SNAND_STATUS_OIP))
			break;
	}
}

static u32 mtk_snand_reverse_byte_order(u32 num)
{
	u32 ret = 0;

	ret |= ((num >> 24) & 0x000000FF);
	ret |= ((num >> 8)  & 0x0000FF00);
	ret |= ((num << 8)  & 0x00FF0000);
	ret |= ((num << 24) & 0xFF000000);

	return ret;
}

static u32 mtk_snand_gen_c1a3(const u32 cmd, const u32 address)
{
	return ((mtk_snand_reverse_byte_order(address) & 0xFFFFFF00)
		| (cmd & 0xFF));
}
static void mtk_snand_dev_die_select_op(struct mtk_snfc *snfc, u8 die_id)
{
	u32  cmd;

	cmd = SNAND_CMD_DIE_SELECT | (die_id << 8);
	snfi_writel(snfc, cmd, NFI_SNAND_GPRAM_DATA);
	snfi_writel(snfc, 2, NFI_SNAND_MAC_OUTL);
	snfi_writel(snfc, 0, NFI_SNAND_MAC_INL);

	mtk_snand_dev_mac_op(snfc, SPI);
}

static u32 mtk_snand_dev_die_select(struct mtd_info *mtd, u32 page)
{
	struct nand_chip *chip = mtd_to_nand(mtd);
	struct mtk_snfc *snfc = nand_get_controller_data(chip);
	u32 total_blocks = devinfo.totalsize << (20 - chip->phys_erase_shift);
	u16 page_per_block = 1 << (chip->phys_erase_shift - chip->page_shift);
	u16 page_in_block = page % page_per_block;
	u32 block = page / page_per_block;

	/* Die Select operation */
	if (devinfo.advancedmode & SNAND_ADV_TWO_DIE) {
		if (block >= total_blocks >> 1) {
			mtk_snand_dev_die_select_op(snfc, 1);
			block -= total_blocks >> 1;
		} else {
			mtk_snand_dev_die_select_op(snfc, 0);
		}
	}

	return (block * page_per_block + page_in_block);
}

static void mtk_snand_dev_enable_spiq(struct mtk_snfc *snfc, bool enable)
{
	u8   regval;
	u32  cmd, reg;
	u32  ret = 0;

	/* read QE in status register */
	cmd = SNAND_CMD_GET_FEATURES | (SNAND_CMD_FEATURES_OTP << 8);

	snfi_writel(snfc, cmd, NFI_SNAND_GPRAM_DATA);
	snfi_writel(snfc, 2, NFI_SNAND_MAC_OUTL);
	snfi_writel(snfc, 1, NFI_SNAND_MAC_INL);

	mtk_snand_dev_mac_op(snfc, SPI);

	reg = snfi_readl(snfc, NFI_SNAND_GPRAM_DATA);
	regval  = (reg>>16)&0xFF;

	if (enable == 0) {
		if ((regval & SNAND_OTP_QE) == 0)
			ret = 1;
		else
			regval = regval & ~SNAND_OTP_QE;
	} else {
		if ((regval & SNAND_OTP_QE) == 1)
			ret = 1;
		else
			regval = regval | SNAND_OTP_QE;
	}

	if (ret == 1)
		return;
	/* if goes here, it means QE needs to be set as new different value
	 * write status register
	 */

	mtk_snand_dev_command(snfc, SNAND_CMD_WRITE_ENABLE, 1);

	cmd = SNAND_CMD_SET_FEATURES | (SNAND_CMD_FEATURES_OTP << 8)
				     | (regval << 16);
	snfi_writel(snfc, cmd, NFI_SNAND_GPRAM_DATA);
	snfi_writel(snfc, 3, NFI_SNAND_MAC_OUTL);
	snfi_writel(snfc, 0, NFI_SNAND_MAC_INL);

	mtk_snand_dev_mac_op(snfc, SPI);
}

/* Read Split related APIs */
static bool mtk_snand_rs_if_require_split(void)
{
	u32 ret;

	if (devinfo.advancedmode & SNAND_ADV_READ_SPLIT) {
		if (g_snand_rs_cur_part != 0)
			ret = 1;
		else
			ret = 0;
	} else {
		ret = 0;
	}

	return ret;
}

/* NOTE(Bayi): added for erase ok */
void mtk_snand_dev_unlock_all_blocks(struct mtk_snfc *snfc)
{
	u32 cmd;
	u8  lock;
	u8  lock_new;

	/* read original block lock settings */
	cmd = SNAND_CMD_GET_FEATURES | (SNAND_CMD_FEATURES_BLOCK_LOCK << 8);
	snfi_writel(snfc, cmd, NFI_SNAND_GPRAM_DATA);
	snfi_writel(snfc, 2, NFI_SNAND_MAC_OUTL);
	snfi_writel(snfc, 1, NFI_SNAND_MAC_INL);

	mtk_snand_dev_mac_enable(snfc, SPI);
	mtk_snand_dev_mac_trigger(snfc);
	mtk_snand_dev_mac_leave(snfc);

	cmd = snfi_readl(snfc, NFI_SNAND_GPRAM_DATA);
	lock  = (cmd >> 16) & 0xFF;
	pr_debug("[SNF] Lock register(before):0x%x\n\r", lock);

	lock_new = lock & ~SNAND_BLOCK_LOCK_BITS;

	if (lock != lock_new) {
		/* write enable */
		mtk_snand_dev_command(snfc, SNAND_CMD_WRITE_ENABLE, 1);

		/* set features */
		cmd = SNAND_CMD_SET_FEATURES
		      | (SNAND_CMD_FEATURES_BLOCK_LOCK << 8)
		      | (lock_new << 16);
		mtk_snand_dev_command(snfc, cmd, 3);
	}

	/* cosnfrm if unlock is successful */
	cmd = SNAND_CMD_GET_FEATURES | (SNAND_CMD_FEATURES_BLOCK_LOCK << 8);
	snfi_writel(snfc, cmd, NFI_SNAND_GPRAM_DATA);
	snfi_writel(snfc, 2, NFI_SNAND_MAC_OUTL);
	snfi_writel(snfc, 1, NFI_SNAND_MAC_INL);

	mtk_snand_dev_mac_enable(snfc, SPI);
	mtk_snand_dev_mac_trigger(snfc);
	mtk_snand_dev_mac_leave(snfc);

	cmd = snfi_readl(snfc, NFI_SNAND_GPRAM_DATA);
	lock  = (cmd>>16)&0xFF;

	if (lock & SNAND_BLOCK_LOCK_BITS)
		pr_warn("[SNF] Unlock all blocks failed!\n\r");
}

void mtk_snand_dev_ecc_control(struct mtk_snfc *snfc)
{
	u32 cmd, reg;
	u8  otp;
	u8  otp_new;

	/* read original otp settings */
	cmd = SNAND_CMD_GET_FEATURES | (SNAND_CMD_FEATURES_OTP << 8);
	snfi_writel(snfc, cmd, NFI_SNAND_GPRAM_DATA);
	snfi_writel(snfc, 2, NFI_SNAND_MAC_OUTL);
	snfi_writel(snfc, 1, NFI_SNAND_MAC_INL);
	mtk_snand_dev_mac_op(snfc, SPI);

	reg = snfi_readl(snfc, NFI_SNAND_GPRAM_DATA);
	otp  = (reg>>16)&0xFF;
	/* disable device ecc */
	otp_new = otp & ~SNAND_OTP_ECC_ENABLE;

	if (otp != otp_new) {
		/* write enable */
		mtk_snand_dev_command(snfc, SNAND_CMD_WRITE_ENABLE, 1);
		/* set features */
		cmd = SNAND_CMD_SET_FEATURES | (SNAND_CMD_FEATURES_OTP << 8)
					     | (otp_new << 16);
		mtk_snand_dev_command(snfc, cmd, 3);

		/* read confirm , read original otp settings */
		cmd = SNAND_CMD_GET_FEATURES | (SNAND_CMD_FEATURES_OTP << 8);
		snfi_writel(snfc, cmd, NFI_SNAND_GPRAM_DATA);
		snfi_writel(snfc, 2, NFI_SNAND_MAC_OUTL);
		snfi_writel(snfc, 1, NFI_SNAND_MAC_INL);
		mtk_snand_dev_mac_op(snfc, SPI);

		reg = snfi_readl(snfc, NFI_SNAND_GPRAM_DATA);
		otp  = (reg>>16)&0xFF;
		if (otp != otp_new)
			pr_warn("spi nand ecc disable fail");
		else
			pr_warn("spi nand ecc disable success");
	}
}

static void mtk_snand_rs_reconfig_nfiecc(u32 row_addr)
{
	/* 1. only decode part should be re-configured
	 * only re-configure essential part
	 * (fixed register will not be re-configured)
	 */
	u16 reg16;
	u32 ecc_bit_cfg = 0;
	u32 u4DECODESize;
	struct mtd_info *mtd = &host->mtd;
	struct nand_chip *nand = mtd_to_nand(mtd);
	struct mtk_snfc *snfc = nand_get_controller_data(nand);

	if ((devinfo.advancedmode & SNAND_ADV_READ_SPLIT) == 0)
		return;
	pr_debug("row_addr:0x%x, g_snand_rs_num_page:0x%x\n", row_addr,
		g_snand_rs_num_page);

	if (row_addr < g_snand_rs_num_page) {
		pr_debug("mtk_snand_rs_reconfig_nfiecc , test01\n");
		if (g_snand_rs_cur_part != 0) {
			pr_debug("mtk_snand_rs_reconfig_nfiecc , test02\n");
			g_snand_rs_ecc_bit = SNAND_RS_ECC_BIT_PART0;
			reg16 = snfi_readw(snfc, NFI_PAGEFMT);
			reg16 &= ~PAGEFMT_SPARE_MASK;
			reg16 |= SNAND_RS_SPARE_PER_SECTOR_PART0_NFI;
			snfi_writew(snfc, reg16, NFI_PAGEFMT);
			g_snand_k_spare_per_sec
				= SNAND_RS_SPARE_PER_SECTOR_PART0_VAL;
			g_snand_rs_cur_part = 0;
		} else {
			return;
		}
	} else {
		pr_debug("mtk_snand_rs_reconfig_nfiecc , test03\n");
		if (g_snand_rs_cur_part != 1) {
			pr_debug("mtk_snand_rs_reconfig_nfiecc , test04\n");
			g_snand_rs_ecc_bit = g_snand_rs_ecc_bit_second_part;

			reg16 = snfi_readw(snfc, NFI_PAGEFMT);
			reg16 &= ~PAGEFMT_SPARE_MASK;
			reg16 |= g_snand_rs_spare_per_sector_second_part_nfi;
			snfi_writew(snfc, reg16, NFI_PAGEFMT);

			g_snand_k_spare_per_sec = mtd->oobsize
						  / (mtd->writesize
						  / NAND_SECTOR_SIZE);

			g_snand_rs_cur_part = 1;
		} else {
			return;
		}
	}
	mtk_snand_dev_ecc_control(snfc);   /* disable device ECC */

	switch (g_snand_rs_ecc_bit) {
	case 4:
		ecc_bit_cfg = ECC_CNFG_ECC4;
		break;
	case 8:
		ecc_bit_cfg = ECC_CNFG_ECC8;
		break;
	case 10:
		ecc_bit_cfg = ECC_CNFG_ECC10;
		break;
	case 12:
		ecc_bit_cfg = ECC_CNFG_ECC12;
		break;
	default:
		break;
	}
	snfi_writew(snfc, DEC_DE, ECC_DECCON);

	do {
	;
	} while (!snfi_readw(snfc, ECC_DECIDLE));
	u4DECODESize = ((NAND_SECTOR_SIZE + 1) << 3) + g_snand_rs_ecc_bit * 13;
	/* configure ECC decoder && encoder */
	snfi_writel(snfc, DEC_CNFG_CORRECT | ecc_bit_cfg
			  | DEC_CNFG_NFI | DEC_CNFG_EMPTY_EN
			  | (u4DECODESize << DEC_CNFG_CODE_SHIFT),
			  ECC_DECCNFG);
}

static void mtk_snand_ecc_config(struct mtk_snfc *snfc,
				 struct mtk_snand_host_hw *hw,
				 u32 ecc_bit)
{
	u32 u4ENCODESize;
	u32 u4DECODESize;
	u32 ecc_bit_cfg = ECC_CNFG_ECC4;
	u32 reg;

	switch (ecc_bit) {
	case 4:
		ecc_bit_cfg = ECC_CNFG_ECC4;
		break;
	case 8:
		ecc_bit_cfg = ECC_CNFG_ECC8;
		break;
	case 10:
		ecc_bit_cfg = ECC_CNFG_ECC10;
		break;
	case 12:
		ecc_bit_cfg = ECC_CNFG_ECC12;
		break;
	default:
		break;
	}
	snfi_writew(snfc, DEC_DE, ECC_DECCON);
	do {
	;
	} while (!snfi_readw(snfc, ECC_DECIDLE));

	snfi_writew(snfc, ENC_DE, ECC_ENCCON);
	do {
	;
	} while (!snfi_readl(snfc, ECC_ENCIDLE));

	/* setup FDM register base */
	/* Sector + FDM , ecc paritry num is 13, use 1 for upstream  fmd ecc*/
	u4ENCODESize = (hw->nand_sec_size + 1) << 3;
	/* Sector + FDM + YAFFS2 meta data bits */
	u4DECODESize = ((hw->nand_sec_size + 1) << 3) + ecc_bit * 13;
	/* configure ECC decoder && encoder */
	snfi_writel(snfc, ecc_bit_cfg | DEC_CNFG_NFI | DEC_CNFG_EMPTY_EN
		    | (u4DECODESize << DEC_CNFG_CODE_SHIFT), ECC_DECCNFG);
	snfi_writel(snfc, ecc_bit_cfg | ENC_CNFG_NFI | (u4ENCODESize
		    << ENC_CNFG_MSG_SHIFT), ECC_ENCCNFG);
#ifndef MANUAL_CORRECT
	reg = snfi_readl(snfc, ECC_DECCNFG) | DEC_CNFG_CORRECT;
	snfi_writel(snfc, reg, ECC_DECCNFG);
#else
	reg = snfi_readl(snfc, ECC_DECCNFG) | DEC_CNFG_EL;
	snfi_writel(snfc, reg, ECC_DECCNFG);
#endif
}

static void mtk_snand_ecc_decode_start(struct mtk_snfc *snfc)
{
	/* wait for device returning idle */
	while (!(snfi_readw(snfc, ECC_DECIDLE) & DEC_IDLE))
	;
	snfi_writeb(snfc, DEC_EN, ECC_DECCON);
}

static void mtk_snand_ecc_decode_end(struct mtk_snfc *snfc)
{
	u32 timeout = 0xFFFF;

	/* wait for device returning idle */
	while (!(snfi_readw(snfc, ECC_DECIDLE) & DEC_IDLE)) {
		timeout--;
		if (timeout == 0)
			break;
	}
	snfi_writew(snfc, DEC_DE, ECC_DECCON);
}

static void mtk_snand_ecc_encode_start(struct mtk_snfc *snfc)
{
	/* wait for device returning idle */
	while (!(snfi_readl(snfc, ECC_ENCIDLE) & ENC_IDLE))
	;
	snfi_writew(snfc, ENC_EN, ECC_ENCCON);
}

static void mtk_snand_ecc_encode_end(struct mtk_snfc *snfc)
{
	/* wait for device returning idle */
	while (!(snfi_readl(snfc, ECC_ENCIDLE) & ENC_IDLE))
	;
	snfi_writew(snfc, ENC_DE, ECC_ENCCON);
}
static bool is_empty_page(u8 *spare_buf, u32 sec_num)
{
	u32 i = 0;
	bool is_empty = 1;

	for (i = 0; i < OOB_INDEX_SIZE; i++) {
		if (spare_buf[OOB_INDEX_OFFSET+i] != 0xFF) {
			is_empty = 0;
			break;
		}
	}
	pr_debug("This page is %s!\n", is_empty ? "empty" : "not empty");
	return is_empty;
}

static bool return_fake_buf(u8 *data_buf, u32 page_size, u32 sec_num,
			    u32 u4PageAddr)
{
	u32 i = 0, j = 0;
	u32 sec_zero_count = 0;
	u8 t = 0;
	u8 *p = data_buf;
	bool ret = 1;

	for (j = 0; j < sec_num; j++) {
		p = data_buf+j * 512;
		sec_zero_count = 0;
		for (i = 0; i < 512; i++) {
			t = p[i];
			t = ~t;
			t = ((t & 0xaa) >> 1) + (t & 0x55);
			t = ((t & 0xcc) >> 2) + (t & 0x33);
			t = ((t & 0xf0f0) >> 4) + (t & 0x0f0f);
			sec_zero_count += t;
			if (t > 0)
				pr_debug("%d bit filp at sec(%d)\n", t, j);
		}
		if (sec_zero_count > 2)
			ret = 0;
	}
	return ret;
}

void mtk_snand_nfi_enable_bypass(struct mtk_snfc *snfc, u8 enable)
{
	u32 reg;

	if (enable == 1) {
		reg = snfi_readw(snfc, NFI_DEBUG_CON1)
				 | DEBUG_CON1_BYPASS_MASTER_EN;
		snfi_writew(snfc, reg, NFI_DEBUG_CON1);

		reg = snfi_readl(snfc, ECC_BYPASS_Reg) | ECC_BYPASS;
		snfi_writel(snfc, reg, ECC_BYPASS_Reg);
	} else {
		reg = snfi_readw(snfc, NFI_DEBUG_CON1);
		reg &= ~DEBUG_CON1_BYPASS_MASTER_EN;
		snfi_writew(snfc, reg, NFI_DEBUG_CON1);

		reg = snfi_readl(snfc, ECC_BYPASS_Reg);
		reg &= ~ECC_BYPASS;
		snfi_writel(snfc, reg, ECC_BYPASS_Reg);
	}
}

u32 mtk_snand_nfi_if_empty_page(struct mtk_snfc *snfc)
{
	u32 reg_val = 0;

	reg_val = snfi_readl(snfc, NFI_STA);
	if (0 != (reg_val & STA_READ_EMPTY)) /* empty page */
		reg_val = 1;
	else
		reg_val = 0;

	mtk_snand_reset_con(snfc);
	return reg_val;
}

static bool mtk_snand_check_bch_error(struct mtd_info *mtd, u8 *pDataBuf,
				      u8 *spareBuf, u32 u4SecIndex,
				      u32 u4PageAddr)
{
	bool ret = 1;
	u16 u2SectorDoneMask = 1 << u4SecIndex;
	u32 u4ErrorNumDebug0, u4ErrorNumDebug1, i, u4ErrNum;
	u32 timeout = 0xFFFF;
	u32 correct_count = 0;
	u32 page_size = (u4SecIndex+1) * 512;
	u32 sec_num = u4SecIndex + 1;
	u32 failed_sec = 0;
	struct nand_chip *nand = mtd_to_nand(mtd);
	struct mtk_snfc *snfc = nand_get_controller_data(nand);

	while ((u2SectorDoneMask & snfi_readw(snfc, ECC_DECDONE)) == 0) {
		timeout--;
		if (timeout == 0)
			return 0;
	}
	u4ErrorNumDebug0 = snfi_readl(snfc, ECC_DECENUM0);
	u4ErrorNumDebug1 = snfi_readl(snfc, ECC_DECENUM1);

	if ((u4ErrorNumDebug0 & 0x000FFFFF) != 0
	    || (u4ErrorNumDebug1 & 0x000FFFFF) != 0) {
		failed_sec = 0;
		for (i = 0; i <= u4SecIndex; ++i) {
			if (i < 4)
				u4ErrNum = snfi_readl(snfc, ECC_DECENUM0)
						      >> (i * 5);
			else
				u4ErrNum = snfi_readl(snfc, ECC_DECENUM1)
						      >> ((i - 4) * 5);

			u4ErrNum &= 0x1F;
			if (u4ErrNum == 0x1F) {
				failed_sec++;
			} else {
				if (u4ErrNum) {
					correct_count += u4ErrNum;
				}
			}
		}

		if (is_empty_page(spareBuf, sec_num)) {
			failed_sec = 0;
			ret = 1;
		}

		if (failed_sec != 0) {
			if ((failed_sec == (u4SecIndex + 1)) &&
				(mtk_snand_nfi_if_empty_page(snfc) == 1)) {
				failed_sec = 0;
			} else {
				ret = 0;
				pr_notice("ECC-U, PA=%d\n", u4PageAddr);
			}
		}

		if (ret == 0)
			if (return_fake_buf(pDataBuf, page_size, sec_num,
			    u4PageAddr)) {
				ret = 1;
				pr_debug("empty page have few filped bit(s)\n");
				memset(pDataBuf, 0xff, page_size);
				memset(spareBuf, 0xff, sec_num*8);
				failed_sec = 0;
			}

		mtd->ecc_stats.failed += failed_sec;
		if (correct_count > 2 && ret)
			mtd->ecc_stats.corrected++;
	}
	return ret;
}


static bool mtk_snand_RFIFOValidSize(struct mtk_snfc *snfc, u16 u2Size)
{
	u32 timeout = 0xFFFF;

	while (FIFO_RD_REMAIN(snfi_readw(snfc, NFI_FIFOSTA)) < u2Size) {
		timeout--;
		if (timeout == 0)
			return 0;
	}
	return 1;
}

static bool mtk_snand_WFIFOValidSize(struct mtk_snfc *snfc, u16 u2Size)
{
	u32 timeout = 0xFFFF;

	while (FIFO_WR_REMAIN(snfi_readw(snfc, NFI_FIFOSTA)) > u2Size) {
		timeout--;
		if (timeout == 0)
			return 0;
	}
	return 1;
}


static bool mtk_snand_status_ready(struct mtk_snfc *snfc, u32 u4Status)
{
	u32 timeout = 0xFFFF;

	u4Status &= ~STA_NAND_BUSY;
	while ((snfi_readl(snfc, NFI_STA) & u4Status) != 0) {
		timeout--;
		if (timeout == 0)
			return 0;
	}
	return 1;
}

static bool mtk_snand_reset_con(struct mtk_snfc *snfc)
{
	u32 reg32;

	int timeout = 0xFFFF;

	/* part 1. SNF */
	reg32 = snfi_readl(snfc, NFI_SNAND_MISC_CTL);
	reg32 |= SNAND_SW_RST;
	snfi_writel(snfc, reg32, NFI_SNAND_MISC_CTL);
	reg32 &= ~SNAND_SW_RST;
	snfi_writel(snfc, reg32, NFI_SNAND_MISC_CTL);
	/* part 2. NFI */

	for (;;) {
		reg32 = snfi_readw(snfc, NFI_MASTER_STA);
		if ((reg32 & MASTERSTA_MASK) == 0)
			break;
	}

	snfi_writew(snfc, CON_FIFO_FLUSH | CON_NFI_RST, NFI_CON);
	for (;;) {
		if ((snfi_readl(snfc, NFI_STA) & (STA_NFI_FSM_MASK
			| STA_NAND_FSM_MASK)) == 0)
			break;

		timeout--;
		if (!timeout)
			pr_warn("NFI_MASTERSTA_REG16 timeout!!\n");
	}
	/* issue reset operation */
	return mtk_snand_status_ready(snfc, STA_NFI_FSM_MASK | STA_NAND_BUSY)
				      && mtk_snand_RFIFOValidSize(snfc, 0)
				      && mtk_snand_WFIFOValidSize(snfc, 0);
}

static void mtk_snand_set_mode(struct mtk_snfc *snfc, u16 u2OpMode)
{
	u16 u2Mode = snfi_readw(snfc, NFI_CNFG);

	u2Mode &= ~CNFG_OP_MODE_MASK;
	u2Mode |= u2OpMode;
	snfi_writel(snfc, u2Mode, NFI_CNFG);
}

static void mtk_snand_set_autoformat(struct mtk_snfc *snfc, bool bEnable)
{
	u32 reg;

	if (bEnable) {
		reg = snfi_readw(snfc, NFI_CNFG) | CNFG_AUTO_FMT_EN;
		snfi_writew(snfc, reg, NFI_CNFG);
	} else {
		reg = snfi_readw(snfc, NFI_CNFG);
		reg &= ~CNFG_AUTO_FMT_EN;
		snfi_writew(snfc, reg, NFI_CNFG);
	}
}

static void mtk_snand_configure_fdm(struct mtk_snfc *snfc, u16 u2FDMSize,
				    u16 u2FDMECCSize)
{
	u32 reg;

	reg = snfi_readw(snfc, NFI_PAGEFMT);
	reg &= ~(PAGEFMT_FDM_MASK | PAGEFMT_FDM_ECC_MASK);
	snfi_writew(snfc, reg, NFI_PAGEFMT);

	reg = snfi_readw(snfc, NFI_PAGEFMT) | (u2FDMSize << PAGEFMT_FDM_SHIFT);
	snfi_writew(snfc, reg, NFI_PAGEFMT);

	reg = snfi_readw(snfc, NFI_PAGEFMT)
			 | (u2FDMECCSize << PAGEFMT_FDM_ECC_SHIFT);
	snfi_writew(snfc, reg, NFI_PAGEFMT);
}

static bool mtk_snand_check_RW_count(struct mtk_snfc *snfc, u16 u2WriteSize)
{
	u32 timeout = 0xFFFF;
	u16 u2SecNum = u2WriteSize >> 9;

	while (ADDRCNTR_CNTR(snfi_readw(snfc, NFI_ADDRCNTR)) < u2SecNum) {
		timeout--;
		if (timeout == 0) {
			pr_warn("[%s] timeout\n", __func__);
			return 0;
		}
	}
	return 1;
}

static bool mtk_snand_ready_for_read_custom(struct mtk_snfc *snfc,
					    struct nand_chip *nand,
					    u32 u4RowAddr, u32 u4ColAddr,
					    u32 u4SecNum, u8 *buf, u8 mtk_ecc,
					    u8 auto_fmt, u8 ahb_mode)
{
	u8  ret = 1;
	u32 cmd, reg, rc;
	u32 col_addr = u4ColAddr;
	SNAND_Mode mode = SPIQ;
	struct timeval stimer, etimer;

#if __INTERNAL_USE_AHB_MODE__
	dma_addr = dma_map_single(snfc->dev, (void *)buf, u4SecNum *
				  (NAND_SECTOR_SIZE + g_snand_k_spare_per_sec),
				  DMA_FROM_DEVICE);
	rc = dma_mapping_error(snfc->dev, dma_addr);
	if (rc) {
		dev_err(snfc->dev, "dma mapping error\n");
		return -EINVAL;
	}
#endif

	do_gettimeofday(&stimer);
	if (!mtk_snand_reset_con(snfc)) {
		ret = 0;
		goto cleanup;
	}

	/* 1. Read page to cache, PAGE_READ command + 3-byte address */
	cmd = mtk_snand_gen_c1a3(SNAND_CMD_PAGE_READ, u4RowAddr);

	snfi_writel(snfc, cmd, NFI_SNAND_GPRAM_DATA);
	snfi_writel(snfc, 1 + 3, NFI_SNAND_MAC_OUTL);
	snfi_writel(snfc, 0, NFI_SNAND_MAC_INL);
	mtk_snand_dev_mac_op(snfc, SPI);

	/* 2. Get features (status polling) */
	cmd = SNAND_CMD_GET_FEATURES | (SNAND_CMD_FEATURES_STATUS << 8);
	snfi_writel(snfc, cmd, NFI_SNAND_GPRAM_DATA);
	snfi_writel(snfc, 2, NFI_SNAND_MAC_OUTL);
	snfi_writel(snfc, 1, NFI_SNAND_MAC_INL);

	for (;;) {
		mtk_snand_dev_mac_op(snfc, SPI);
		reg = snfi_readl(snfc, NFI_SNAND_GPRAM_DATA);
		cmd  = (reg>>16)&0xFF;

		if ((cmd & SNAND_STATUS_OIP) == 0)
			break;
	}
	/* ------ SNF Part ------ */
	/* set PAGE READ command & address */
	reg = (SNAND_CMD_PAGE_READ << SNAND_PAGE_READ_CMD_OFFSET)
	       | (u4RowAddr & SNAND_PAGE_READ_ADDRESS_MASK);
	snfi_writel(snfc, reg, NFI_SNAND_RD_CTL1);

	/* set DATA READ dummy cycle and command (use default value, ignored) */
	if (mode == SPI) {
		reg = snfi_readl(snfc, NFI_SNAND_RD_CTL2);
		reg &= ~SNAND_DATA_READ_CMD_MASK;
		reg |= SNAND_CMD_RANDOM_READ & SNAND_DATA_READ_CMD_MASK;
		snfi_writel(snfc, reg, NFI_SNAND_RD_CTL2);

	} else if (mode == SPIQ) {
		mtk_snand_dev_enable_spiq(snfc, 1);

		reg = snfi_readl(snfc, NFI_SNAND_RD_CTL2);
		reg &= ~SNAND_DATA_READ_CMD_MASK;
		reg |= SNAND_CMD_RANDOM_READ_SPIQ & SNAND_DATA_READ_CMD_MASK;
		snfi_writel(snfc, reg, NFI_SNAND_RD_CTL2);
	}

	/* set DATA READ address */
	snfi_writel(snfc, (col_addr & SNAND_DATA_READ_ADDRESS_MASK),
		    NFI_SNAND_RD_CTL3);

	/* set SNF timing */
	reg = snfi_readl(snfc, NFI_SNAND_MISC_CTL);
	reg |= SNAND_DATARD_CUSTOM_EN;

	if (mode == SPI) {
		reg &= ~SNAND_DATA_READ_MODE_MASK;
	} else if (mode == SPIQ) {
		reg &= ~SNAND_DATA_READ_MODE_MASK;
		reg |= ((SNAND_DATA_READ_MODE_X4 << SNAND_DATA_READ_MODE_OFFSET)
			& SNAND_DATA_READ_MODE_MASK);
	}

	snfi_writel(snfc, reg, NFI_SNAND_MISC_CTL);
	/* set SNF data length */
	reg = u4SecNum * (NAND_SECTOR_SIZE + g_snand_k_spare_per_sec);
	snfi_writel(snfc, (reg | (reg << SNAND_PROGRAM_LOAD_BYTE_LEN_OFFSET)),
		    NFI_SNAND_MISC_CTL2);
	/* ------ NFI Part ------ */
	mtk_snand_set_mode(snfc, CNFG_OP_CUST);
	reg = snfi_readw(snfc, NFI_CNFG) | CNFG_READ_EN;
	snfi_writew(snfc, reg, NFI_CNFG);
	snfi_writew(snfc, u4SecNum << CON_NFI_SEC_SHIFT, NFI_CON);

	if (ahb_mode) {
		reg = snfi_readw(snfc, NFI_CNFG) | CNFG_AHB;
		snfi_writew(snfc, reg, NFI_CNFG);
		snfi_writel(snfc, (u32)dma_addr, NFI_STRADDR);
	} else {
		reg = snfi_readw(snfc, NFI_CNFG);
		reg &= ~CNFG_AHB;
		snfi_writew(snfc, reg, NFI_CNFG);
	}

	if (g_bHwEcc && mtk_ecc) {
		reg = snfi_readw(snfc, NFI_CNFG) | CNFG_HW_ECC_EN;
		snfi_writew(snfc, reg, NFI_CNFG);
		mtk_snand_ecc_decode_start(snfc);
	} else {
		reg = snfi_readw(snfc, NFI_CNFG);
		reg &= ~CNFG_HW_ECC_EN;
		snfi_writew(snfc, reg, NFI_CNFG);
	}
	mtk_snand_set_autoformat(snfc, auto_fmt);

cleanup:

	do_gettimeofday(&etimer);
	g_NandPerfLog.ReadBusyTotalTime += Cal_timediff(&etimer, &stimer);
	g_NandPerfLog.ReadBusyCount++;

	return ret;
}

static bool mtk_snand_ready_for_write(struct nand_chip *nand, u32 u4RowAddr,
				      u32 col_addr, u8 *buf, u8 mtk_ecc,
				      u8 auto_fmt, u8 ahb_mode)
{
	struct mtk_snfc *snfc = nand_get_controller_data(nand);
	u32 sec_num = 1 << (nand->page_shift - 9);
	u32 reg, rc;
	SNAND_Mode mode = SPIQ;

#if __INTERNAL_USE_AHB_MODE__
	dma_addr = dma_map_single(snfc->dev, (void *)buf, sec_num*
				  (NAND_SECTOR_SIZE + g_snand_k_spare_per_sec),
				  DMA_TO_DEVICE);
	rc = dma_mapping_error(snfc->dev, dma_addr);
	if (rc) {
		dev_err(snfc->dev, "dma mapping error\n");
		return -EINVAL;
	}
#endif

	/* For Toshiba spi nand 1st generation just support SPI mode*/
	if ((devinfo.id[0] == 0x98) && ((devinfo.id[1] & 0xf0) != 0xe0))
		mode = SPI;

	if (!mtk_snand_reset_con(snfc))
		return 0;

	/* 1. Write Enable */
	mtk_snand_dev_command(snfc, SNAND_CMD_WRITE_ENABLE, 1);
	/*------ SNF Part ------ */
	if (mode == SPI) {
		reg = SNAND_CMD_WRITE_ENABLE | (SNAND_CMD_PROGRAM_LOAD <<
		      SNAND_PG_LOAD_CMD_OFFSET) | (SNAND_CMD_PROGRAM_EXECUTE <<
		      SNAND_PG_EXE_CMD_OFFSET);
		snfi_writel(snfc, reg, NFI_SNAND_PG_CTL1);
	} else if (mode == SPIQ) {
		reg = SNAND_CMD_WRITE_ENABLE | (SNAND_CMD_PROGRAM_LOAD_X4 <<
		      SNAND_PG_LOAD_CMD_OFFSET) | (SNAND_CMD_PROGRAM_EXECUTE <<
		      SNAND_PG_EXE_CMD_OFFSET);
		snfi_writel(snfc, reg, NFI_SNAND_PG_CTL1);
		mtk_snand_dev_enable_spiq(snfc, 1);
	}
	mtk_snand_nfi_enable_bypass(snfc, 1);
	/* set program load address */
	snfi_writel(snfc, col_addr & SNAND_PG_LOAD_ADDR_MASK,
		    NFI_SNAND_PG_CTL2);
	/* set program execution address */
	snfi_writel(snfc, u4RowAddr, NFI_SNAND_PG_CTL3);

	/* set SNF timing */
	reg = snfi_readl(snfc, NFI_SNAND_MISC_CTL);
	reg |= SNAND_PG_LOAD_CUSTOM_EN;    /* use custom program */

	if (mode == SPI) {
		reg &= ~SNAND_DATA_READ_MODE_MASK;
		reg |= ((SNAND_DATA_READ_MODE_X1 << SNAND_DATA_READ_MODE_OFFSET)
			& SNAND_DATA_READ_MODE_MASK);
		reg &= ~SNAND_PG_LOAD_X4_EN;
	} else if (mode == SPIQ) {
		reg &= ~SNAND_DATA_READ_MODE_MASK;
		reg |= ((SNAND_DATA_READ_MODE_X4 << SNAND_DATA_READ_MODE_OFFSET)
			& SNAND_DATA_READ_MODE_MASK);
		reg |= SNAND_PG_LOAD_X4_EN;
	}

	snfi_writel(snfc, reg, NFI_SNAND_MISC_CTL);
	/* set SNF data length */
	reg = sec_num * (NAND_SECTOR_SIZE + g_snand_k_spare_per_sec);
	snfi_writel(snfc, reg | (reg << SNAND_PROGRAM_LOAD_BYTE_LEN_OFFSET),
		    NFI_SNAND_MISC_CTL2);
	/* NFI Part  */
	mtk_snand_set_mode(snfc, CNFG_OP_PRGM);

	reg = snfi_readw(snfc, NFI_CNFG);
	reg &= ~CNFG_READ_EN;
	snfi_writew(snfc, reg, NFI_CNFG);
	snfi_writew(snfc, sec_num << CON_NFI_SEC_SHIFT, NFI_CON);

	if (ahb_mode) {
		reg = snfi_readw(snfc, NFI_CNFG) | CNFG_AHB;
		snfi_writew(snfc, reg, NFI_CNFG);
		snfi_writel(snfc, (u32)dma_addr, NFI_STRADDR);
	} else {
		reg = snfi_readw(snfc, NFI_CNFG);
		reg &= ~CNFG_AHB;
		snfi_writew(snfc, reg, NFI_CNFG);
	}

	mtk_snand_set_autoformat(snfc, auto_fmt);
	if (mtk_ecc) {
		reg = snfi_readw(snfc, NFI_CNFG) | CNFG_HW_ECC_EN;
		snfi_writew(snfc, reg, NFI_CNFG);
		mtk_snand_ecc_encode_start(snfc);
	} else {
		reg = snfi_readw(snfc, NFI_CNFG);
		reg &= ~CNFG_HW_ECC_EN;
		snfi_writew(snfc, reg, NFI_CNFG);
	}

	return 1;
}

static bool mtk_snand_check_dececc_done(struct mtk_snfc *snfc, u32 u4SecNum)
{
	u32 timeout, dec_mask;

	timeout = 0xffff;
	dec_mask = (1 << u4SecNum) - 1;

	while ((dec_mask != snfi_readw(snfc, ECC_DECDONE)) && timeout > 0) {
		timeout--;
		if (timeout == 0)
			return 0;
	}
	return 1;
}

static bool mtk_snand_dma_read_data(struct mtd_info *mtd, u8 *buf, u32 num_sec)
{
	struct nand_chip *chip = mtd_to_nand(mtd);
	struct mtk_snfc *snfc = nand_get_controller_data(chip);
	int interrupt_en = g_i4Interrupt;
	int timeout = 0xfffff;
	u32 reg;
	struct timeval stimer, etimer;

	do_gettimeofday(&stimer);

	reg = snfi_readw(snfc, NFI_CNFG);
	reg &= ~CNFG_BYTE_RW;
	snfi_writew(snfc, reg, NFI_CNFG);

	reg = snfi_readw(snfc, NFI_CNFG) | CNFG_DMA_BURST_EN;
	snfi_writew(snfc, reg, NFI_CNFG);

	/* set dummy command to trigger NFI enter custom mode */
	snfi_writew(snfc, NAND_CMD_DUMMYREAD, NFI_CMD);
	snfi_readw(snfc, NFI_INTR_STA);
	snfi_writew(snfc, INTR_AHB_DONE_EN, NFI_INTR_EN);

	if (interrupt_en)
		init_completion(&g_comp_AHB_Done);

	reg = snfi_readw(snfc, NFI_CON) | CON_NFI_BRD;
	snfi_writew(snfc, reg, NFI_CON);

	g_snand_dev_status = SNAND_NFI_CUST_READING;
	g_snand_cmd_status = 0; /* busy */
	g_running_dma = 1;

	if (interrupt_en) {
		if (!wait_for_completion_timeout(&g_comp_AHB_Done, 20)) {
			pr_warn("wait for dma completion timeout\n");
			mtk_snand_dump_reg(snfc);
			g_snand_dev_status = SNAND_IDLE;
			g_running_dma = 0;

			goto mtk_snand_dma_read_data_failed;
		}
		g_snand_dev_status = SNAND_IDLE;
		g_running_dma = 0;
		while (num_sec > ((snfi_readw(snfc, NFI_BYTELEN) & 0xf000)
			>> 12)) {
			timeout--;
			if (timeout == 0) {
				pr_warn("AHB mode poll BYTELEN error\n");
				g_snand_dev_status = SNAND_IDLE;
				g_running_dma = 0;

				goto mtk_snand_dma_read_data_failed;
			}
		}
	} else {
		while (!snfi_readw(snfc, NFI_INTR_STA)) {
			timeout--;
			if (timeout == 0) {
				pr_warn("[%s] poll nfi_intr error\n", __func__);
				mtk_snand_dump_reg(snfc);
				g_snand_dev_status = SNAND_IDLE;
				g_running_dma = 0;

				goto mtk_snand_dma_read_data_failed;
			}
		}

		while (num_sec > ((snfi_readw(snfc, NFI_BYTELEN) & 0xf000)
		       >> 12)) {
			timeout--;
			if (timeout == 0) {
				pr_warn("PIO mode poll BYTELEN error\n");
				mtk_snand_dump_reg(snfc);
				g_snand_dev_status = SNAND_IDLE;
				g_running_dma = 0;

				goto mtk_snand_dma_read_data_failed;
			}
		}

		g_snand_dev_status = SNAND_IDLE;
		g_running_dma = 0;
	}

	do_gettimeofday(&etimer);
	g_NandPerfLog.ReadDMATotalTime += Cal_timediff(&etimer, &stimer);
	g_NandPerfLog.ReadDMACount++;
	mtk_snand_nfi_enable_bypass(snfc, 0);

	return 1;

mtk_snand_dma_read_data_failed:

	mtk_snand_nfi_enable_bypass(snfc, 0);
	return 0;
}


static bool mtk_snand_read_page_data(struct mtd_info *mtd, u8 *pDataBuf,
				     u32 u4SecNum, u8 full)
{
	return mtk_snand_dma_read_data(mtd, pDataBuf, u4SecNum);
}

static bool mtk_snand_dma_write_data(struct mtd_info *mtd, u8 *pDataBuf,
				     u32 u4Size, u8 full)
{
	struct nand_chip *chip = mtd_to_nand(mtd);
	struct mtk_snfc *snfc = nand_get_controller_data(chip);
	int i4Interrupt = 0;
	u32 snf_len, reg;
	struct timeval stimer, etimer;

	do_gettimeofday(&stimer);
	/* set SNF data length */
	if (full)
		snf_len = u4Size + mtd->oobsize;
	else
		snf_len = u4Size;

	snfi_writel(snfc, ((snf_len) | (snf_len
		    << SNAND_PROGRAM_LOAD_BYTE_LEN_OFFSET)),
		    NFI_SNAND_MISC_CTL2);

	reg = snfi_readw(snfc, NFI_CNFG);
	reg &= ~CNFG_BYTE_RW;
	snfi_writew(snfc, reg, NFI_CNFG);

	/* set dummy command to trigger NFI enter custom mode */
	snfi_writew(snfc, NAND_CMD_DUMMYPROG, NFI_CMD);

	snfi_readw(snfc, NFI_INTR_STA);
	snfi_writew(snfc, INTR_CUSTOM_PROG_DONE_INTR_EN, NFI_INTR_EN);

	reg = snfi_readw(snfc, NFI_CNFG) | CNFG_DMA_BURST_EN;
	snfi_writew(snfc, reg, NFI_CNFG);

	if (i4Interrupt) {
		init_completion(&g_comp_AHB_Done);
		snfi_readw(snfc, NFI_INTR_STA);
		snfi_writel(snfc, INTR_CUSTOM_PROG_DONE_INTR_EN, NFI_INTR_EN);
	}
	mtk_snand_nfi_enable_bypass(snfc, 0);
	reg = snfi_readw(snfc, NFI_CON) | CON_NFI_BWR;
	snfi_writew(snfc, reg, NFI_CON);
	g_running_dma = 3;

	if (i4Interrupt) {
		if (!wait_for_completion_timeout(&g_comp_AHB_Done, 20)) {
			pr_warn("wait for completion timeout @[%s]: %d\n",
				__func__, __LINE__);
			mtk_snand_dump_reg(snfc);
			g_running_dma = 0;
			return 0;
		}
		g_running_dma = 0;
	} else {
		while (!(snfi_readl(snfc, NFI_SNAND_STA_CTL1)
				    & SNAND_CUSTOM_PROGRAM)){
		}
		g_running_dma = 0;
	}

	g_NandPerfLog.WriteDMATotalTime += Cal_timediff(&etimer, &stimer);
	g_NandPerfLog.WriteDMACount++;

	return 1;
}


static bool mtk_snand_write_page_data(struct mtd_info *mtd, u8 *buf,
				      u32 size, u8 full)
{
	return mtk_snand_dma_write_data(mtd, buf, size, full);
}

static void mtk_snand_read_fdm_data(struct mtk_snfc *snfc, u8 *pDataBuf,
				    u32 u4SecNum)
{
	u32 i;
	u32 *pBuf32 = (u32 *) pDataBuf;

	if (pBuf32) {
		for (i = 0; i < u4SecNum; ++i) {
			*pBuf32++ = snfi_readl(snfc, NFI_FDM0L + i * 2);
			*pBuf32++ = snfi_readl(snfc, NFI_FDM0M + i * 2);
		}
	}
}

static void mtk_snand_write_fdm_data(struct nand_chip *chip, u8 *pDataBuf,
				     u32 u4SecNum)
{
	struct mtk_snfc *snfc = nand_get_controller_data(chip);
	u32 vall, valm;
	u32 i, j;

	for (i = 0; i < u4SecNum; ++i) {
		vall = 0;
		valm = 0;
		for (j = 0; j < 8; j++) {
			if (j < 4)
				vall |= (*pDataBuf++) << (j * 8);
			else
				valm |= (*pDataBuf++) << ((j - 4) * 8);
		}
		snfi_writel(snfc, vall, NFI_FDM0L + (i << 1));
		snfi_writel(snfc, valm, NFI_FDM0M + (i << 1));
	}
}

static void mtk_snand_stop_read_custom(struct mtk_snfc *snfc, u8 mtk_ecc)
{
	u32 reg;
	/* NFI Part */
	reg = snfi_readw(snfc, NFI_CON);
	reg &= ~CON_NFI_BRD;
	snfi_writew(snfc, reg, NFI_CON);
	/* SNF Part, set 1 then set 0 to clear done flag */
	snfi_writel(snfc, SNAND_CUSTOM_READ, NFI_SNAND_STA_CTL1);
	snfi_writel(snfc, 0, NFI_SNAND_STA_CTL1);
	/* clear essential SNF setting */
	reg = snfi_readl(snfc, NFI_SNAND_MISC_CTL);
	reg &= ~SNAND_PG_LOAD_CUSTOM_EN;
	snfi_writel(snfc, reg, NFI_SNAND_MISC_CTL);

	if (mtk_ecc)
		mtk_snand_ecc_decode_end(snfc);

	snfi_writel(snfc, 0, NFI_INTR_EN);
	mtk_snand_nfi_enable_bypass(snfc, 0);
	mtk_snand_reset_con(snfc);
}

static void mtk_snand_stop_write(struct mtk_snfc *snfc, u8 mtk_ecc)
{
	u32 reg;
	/* NFI Part */
	reg = snfi_readw(snfc, NFI_CON);
	reg &= ~CON_NFI_BWR;
	snfi_writew(snfc, reg, NFI_CON);
	/* SNF Part, set 1 then set 0 to clear done flag */
	snfi_writel(snfc, SNAND_CUSTOM_PROGRAM, NFI_SNAND_STA_CTL1);
	snfi_writel(snfc, 0, NFI_SNAND_STA_CTL1);
	/* clear essential SNF setting */
	reg = snfi_readl(snfc, NFI_SNAND_MISC_CTL);
	reg &= ~SNAND_PG_LOAD_CUSTOM_EN;
	snfi_writel(snfc, reg, NFI_SNAND_MISC_CTL);

	mtk_snand_nfi_enable_bypass(snfc, 0);
	mtk_snand_dev_enable_spiq(snfc, 0);

	if (mtk_ecc)
		mtk_snand_ecc_encode_end(snfc);

	snfi_writew(snfc, 0, NFI_INTR_EN);
}

static bool mtk_snand_read_page_part2(struct mtd_info *mtd, u32 row_addr,
				      u32 num_sec, u8 *buf)
{
	struct nand_chip *nand = mtd_to_nand(mtd);
	struct mtk_snfc *snfc = nand_get_controller_data(nand);
	bool    bRet = 1;
	u32     reg;
	u32     col_part2, i, len;
	u32     spare_per_sector;
	u8      *buf_part2;
	u32     timeout = 0xFFFF;
	u32     old_dec_mode = 0;
	u32     rc;

#if __INTERNAL_USE_AHB_MODE__
	dma_addr = dma_map_single(snfc->dev, (void *)buf, NAND_SECTOR_SIZE
				  + OOB_PER_SECTOR, DMA_FROM_DEVICE);
	rc = dma_mapping_error(snfc->dev, dma_addr);
	if (rc) {
		dev_err(snfc->dev, "dma mapping error\n");
		return -EINVAL;
	}
#endif

	spare_per_sector = mtd->oobsize / (mtd->writesize / NAND_SECTOR_SIZE);

	for (i = 0; i < 2 ; i++) {
		mtk_snand_reset_con(snfc);
		mtk_snand_nfi_enable_bypass(snfc, 1);

		if (i == 0) {
			col_part2 = (NAND_SECTOR_SIZE + spare_per_sector)
				     * (num_sec - 1);
			buf_part2 = buf;
			len = 2112 - col_part2;
		} else {
			col_part2 = 2112;
			buf_part2 += len;   /* append to first round */
			len = (num_sec * (NAND_SECTOR_SIZE + spare_per_sector))
			       - 2112;
		}

		/* SNF Part , set DATA READ address */
		snfi_writel(snfc, (col_part2 & SNAND_DATA_READ_ADDRESS_MASK),
			    NFI_SNAND_RD_CTL3);
		if (i == 0) {
			/* set RW_SNAND_MISC_CTL */
			reg = snfi_readl(snfc, NFI_SNAND_MISC_CTL);
			reg |= SNAND_DATARD_CUSTOM_EN;
			reg &= ~SNAND_DATA_READ_MODE_MASK;
			reg |= ((SNAND_DATA_READ_MODE_X4
				<< SNAND_DATA_READ_MODE_OFFSET)
				& SNAND_DATA_READ_MODE_MASK);

			snfi_writel(snfc, reg, NFI_SNAND_MISC_CTL);
		}

		/* set SNF data length */
		reg = len | (len << SNAND_PROGRAM_LOAD_BYTE_LEN_OFFSET);
		snfi_writel(snfc, reg, NFI_SNAND_MISC_CTL2);
		/*------ NFI Part ------ */
		if (i == 0) {
			mtk_snand_set_mode(snfc, CNFG_OP_CUST);
			reg = snfi_readw(snfc, NFI_CNFG) | CNFG_READ_EN;
			snfi_writew(snfc, reg, NFI_CNFG);
			reg = snfi_readw(snfc, NFI_CNFG) | CNFG_AHB;
			snfi_writew(snfc, reg, NFI_CNFG);

			reg = snfi_readw(snfc, NFI_CNFG);
			reg &= ~CNFG_HW_ECC_EN;
			snfi_writew(snfc, reg, NFI_CNFG);
			mtk_snand_set_autoformat(snfc, 0);
		}
		snfi_writel(snfc, (u32)dma_addr, NFI_STRADDR);
		snfi_writel(snfc, SPIDMA_SEC_EN | (len & SPIDMA_SEC_SIZE_MASK),
			    NFI_SPIDMA);

		reg = snfi_readw(snfc, NFI_CNFG);
		reg &= ~CNFG_BYTE_RW;
		snfi_writew(snfc, reg, NFI_CNFG);
		/* set dummy command to trigger NFI enter custom mode */
		snfi_writew(snfc, NAND_CMD_DUMMYREAD, NFI_CMD);
		snfi_readw(snfc, NFI_INTR_STA);
		snfi_writew(snfc, INTR_AHB_DONE_EN, NFI_INTR_EN);
		reg = snfi_readw(snfc, NFI_CON) | ((1 << CON_NFI_SEC_SHIFT)
				 | CON_NFI_BRD);
		snfi_writew(snfc, reg, NFI_CON);
		timeout = 0xFFFF;
		while (!(snfi_readw(snfc, NFI_INTR_STA) & INTR_AHB_DONE)) {
			timeout--;
			if (timeout == 0) {
				bRet = 0;
				goto unmap_and_cleanup;
			}
		}
		timeout = 0xFFFF;
		while (((snfi_readw(snfc, NFI_BYTELEN) & 0x1f000) >> 12) != 1) {
			timeout--;
			if (timeout == 0) {
				bRet = 0;
				goto unmap_and_cleanup;
			}
		}

		/* NFI Part */
		reg = snfi_readw(snfc, NFI_CON);
		reg &= ~CON_NFI_BRD;
		snfi_writew(snfc, reg, NFI_CON);

		/* SNF Part, set 1 then set 0 to clear done flag */
		snfi_writel(snfc, SNAND_CUSTOM_READ, NFI_SNAND_STA_CTL1);
		snfi_writel(snfc, 0, NFI_SNAND_STA_CTL1);

		/* clear essential SNF setting */
		if (i == 1) {
			reg = snfi_readw(snfc, NFI_SNAND_MISC_CTL);
			reg &= ~SNAND_DATARD_CUSTOM_EN;
			snfi_writew(snfc, reg, NFI_SNAND_MISC_CTL);
		}

		mtk_snand_nfi_enable_bypass(snfc, 0);
	}
	dma_unmap_single(snfc->dev, dma_addr, NAND_SECTOR_SIZE
			 + OOB_PER_SECTOR, DMA_FROM_DEVICE);

	if (g_bHwEcc) {
		dma_addr = dma_map_single(snfc->dev, (void *)buf, NAND_SECTOR_SIZE
					  + OOB_PER_SECTOR, DMA_BIDIRECTIONAL);

		mtk_snand_nfi_enable_bypass(snfc, 1);

		/* configure ECC decoder && encoder */
		reg = snfi_readl(snfc, ECC_DECCNFG);
		old_dec_mode = reg & DEC_CNFG_DEC_MODE_MASK;
		reg &= ~DEC_CNFG_DEC_MODE_MASK;
		reg |= DEC_CNFG_AHB;
		reg |= DEC_CNFG_DEC_BURST_EN;
		snfi_writel(snfc, reg, ECC_DECCNFG);

		snfi_writel(snfc, (u32)dma_addr, ECC_DECDIADDR);
		snfi_writel(snfc, DEC_DE, ECC_DECCON);
		snfi_writel(snfc, DEC_EN, ECC_DECCON);

		while (!(snfi_readw(snfc, ECC_DECDONE) & (1 << 0)))
		;

		mtk_snand_nfi_enable_bypass(snfc, 0);
		dma_unmap_single(snfc->dev, dma_addr, NAND_SECTOR_SIZE
				 + OOB_PER_SECTOR, DMA_BIDIRECTIONAL);

		reg = snfi_readl(snfc, ECC_DECENUM0);

		if (reg != 0) {
			reg &= 0x1F;

			if (reg == 0x1F) {
				bRet = 0;
				pr_debug("ECC-U(2), PA=%d, S=%d\n",
					row_addr, (num_sec - 1));
			} else {
				if (reg)
					pr_debug("ECC-C(2), #err:%d, PA=%d, S=%d\n",
						reg, row_addr, num_sec - 1);
			}
		}

		/* restore essential NFI and ECC registers */
		snfi_writel(snfc, 0, NFI_SPIDMA);
		reg = snfi_readl(snfc, ECC_DECCNFG);
		reg &= ~DEC_CNFG_DEC_MODE_MASK;
		reg |= old_dec_mode;
		reg &= ~DEC_CNFG_DEC_BURST_EN;
		snfi_writel(snfc, reg, ECC_DECCNFG);
		snfi_writel(snfc, DEC_DE, ECC_DECCON);
		snfi_writel(snfc, 0, ECC_DECDIADDR);
	}

unmap_and_cleanup:

	mtk_snand_nfi_enable_bypass(snfc, 0);

	return bRet;
}

bool mtk_nand_exec_read_page(struct mtd_info *mtd, u32 u4RowAddr,
			     u32 u4PageSize, u8 *pPageBuf, u8 *pFDMBuf)
{
	u8 *buf;
	u8 *p_buf_local;
	bool bRet = 1;
	u8 retry;
	u32 u4SecNum = u4PageSize >> 9;
	struct nand_chip *nand = mtd_to_nand(mtd);
	struct mtk_snfc *snfc = nand_get_controller_data(nand);

	PFM_BEGIN(pfm_time_read);

	buf = pPageBuf;

	if (!virt_addr_valid(pPageBuf))
		buf = g_snand_k_temp;

	u4RowAddr = mtk_snand_dev_die_select(mtd, u4RowAddr);
	mtk_snand_rs_reconfig_nfiecc(u4RowAddr);
	if (mtk_snand_rs_if_require_split())
		u4SecNum--;

	retry = 0;

mtk_nand_exec_read_page_retry:

	bRet = 1;
	if (g_snand_rs_ecc_bit != 0) {
		if (mtk_snand_ready_for_read_custom(snfc, nand, u4RowAddr, 0,
						    u4SecNum, buf, 1, 1, 1)) {
			if (!mtk_snand_read_page_data(mtd, buf, u4SecNum, 1))
				bRet = 0;

			if (g_bHwEcc)
				if (!mtk_snand_check_dececc_done(snfc,
								 u4SecNum))
					bRet = 0;

			mtk_snand_read_fdm_data(snfc, pFDMBuf, u4SecNum);
			if (g_bHwEcc)
				if (!mtk_snand_check_bch_error(mtd, buf,
							       pFDMBuf,
							       u4SecNum - 1,
							       u4RowAddr))
					bRet = 0;
			bm_swap(mtd, pFDMBuf, buf);
			mtk_snand_stop_read_custom(snfc, 1);
		} else {
			bRet = 0;
			bm_swap(mtd, pFDMBuf, buf);
			mtk_snand_stop_read_custom(snfc, 0);
		}
		dma_unmap_single(snfc->dev, dma_addr, u4SecNum * (NAND_SECTOR_SIZE
				 + g_snand_k_spare_per_sec),
				 DMA_FROM_DEVICE);

		if (buf != pPageBuf)
			memcpy(pPageBuf, buf, u4PageSize);
	}   /* use device ECC */
    /* no need retry for SLC nand */
	if (bRet == 0) {
		if (retry < 0) {
			retry++;
			pr_warn("[%s] read retrying (the %d time)\n",
				__func__, retry);
			mtk_snand_reset_dev(snfc);

			goto mtk_nand_exec_read_page_retry;
		}
	}

	if (mtk_snand_rs_if_require_split()) {
		/* u4SecNum++; */
		u4SecNum = u4PageSize >> 9;
		/* note: use local temp buffer to read part 2 */
		if (!mtk_snand_read_page_part2(mtd, u4RowAddr, u4SecNum,
					       g_snand_k_temp))
			bRet = 0;
		p_buf_local = buf + NAND_SECTOR_SIZE * (u4SecNum - 1);
		memcpy(p_buf_local, g_snand_k_temp, NAND_SECTOR_SIZE);
		p_buf_local = pFDMBuf + OOB_AVAI_PER_SECTOR * (u4SecNum - 1);

		memcpy(p_buf_local, (g_snand_k_temp + NAND_SECTOR_SIZE),
		       OOB_AVAI_PER_SECTOR);

	}
	mtk_snand_dev_enable_spiq(snfc, 0);

	PFM_END_R(pfm_time_read, u4PageSize + 32);

	return bRet;
}

static bool mtk_snand_dev_program_execute(struct mtk_snfc *snfc, u32 page)
{
	u32 cmd;
	bool bRet = 1;

	/* 3. Program Execute */
	g_snand_dev_status = SNAND_DEV_PROG_EXECUTING;
	g_snand_cmd_status = 0; /* busy */

	cmd = mtk_snand_gen_c1a3(SNAND_CMD_PROGRAM_EXECUTE, page);
	mtk_snand_dev_command(snfc, cmd, 4);

	return bRet;
}

int mtk_nand_exec_write_page(struct mtd_info *mtd, u32 u4RowAddr,
			     u32 u4PageSize, u8 *pPageBuf,
			     u8 *pFDMBuf, u8 oobraw)
{
	struct nand_chip *nand = mtd_to_nand(mtd);
	struct mtk_snfc *snfc = nand_get_controller_data(nand);
	struct timeval stimer, etimer;
	u32 u4SecNum = u4PageSize >> 9;
	u8 *buf;
	u8 status = 0;
	u8 wait_status = 0;
	u8 mtk_ecc;

	PFM_BEGIN(pfm_time_write);

	buf = pPageBuf;

	if (!virt_addr_valid(pPageBuf)) {
		buf = g_snand_k_temp;
		memcpy(buf, pPageBuf, u4PageSize);
	}

	/*For jffs2, close HW ECC when only write oob*/
	mtk_ecc = !oobraw;

	u4RowAddr = mtk_snand_dev_die_select(mtd, u4RowAddr);
	mtk_snand_rs_reconfig_nfiecc(u4RowAddr);
	bm_swap(mtd, pFDMBuf, buf);
	if (g_snand_rs_ecc_bit != 0) {
		if (mtk_snand_ready_for_write(nand, u4RowAddr, 0, buf, mtk_ecc, 1,
						 1)) {
			mtk_snand_write_fdm_data(nand, pFDMBuf, u4SecNum);
			if (!mtk_snand_write_page_data(mtd, buf, u4PageSize, 1))
				status |= NAND_STATUS_FAIL;

			if (!mtk_snand_check_RW_count(snfc, u4PageSize))
				status |= NAND_STATUS_FAIL;

			mtk_snand_stop_write(snfc, 1);
			do_gettimeofday(&stimer);
			mtk_snand_dev_program_execute(snfc, u4RowAddr);

			do_gettimeofday(&etimer);
			g_NandPerfLog.WriteBusyTotalTime +=
				Cal_timediff(&etimer, &stimer);
			g_NandPerfLog.WriteBusyCount++;
		} else {
			mtk_snand_stop_write(snfc, 0);
			status |= NAND_STATUS_FAIL;
		}
		dma_unmap_single(snfc->dev, dma_addr, u4SecNum * (NAND_SECTOR_SIZE +
				 g_snand_k_spare_per_sec), DMA_TO_DEVICE);
	}
	/* Swap back after write operation */
	bm_swap(mtd, pFDMBuf, buf);
	PFM_END_W(pfm_time_write, u4PageSize + 32);
	wait_status = nand->waitfunc(mtd, nand);

	if ((status & NAND_STATUS_FAIL) || (wait_status & NAND_STATUS_FAIL))
		return -EIO;
	else
		return 0;
}

static int mtk_snand_write_page(struct mtd_info *mtd, struct nand_chip *chip,
				uint32_t offset, int data_len, const u8 *buf,
				int oob_required, int page, int cached, int raw)
{
	int page_per_block = 1 << (chip->phys_erase_shift - chip->page_shift);
	struct mtk_snfc *snfc = nand_get_controller_data(chip);
	int block = page / page_per_block;
	u16 page_in_block = page % page_per_block;
	int mapped_block;
	struct timeval stimer, etimer;

	if (!snfc->chip_data->use_bmt)
		mapped_block = block;
	else
		mapped_block = get_mapping_block_index(block);

	do_gettimeofday(&stimer);
	if (mtk_snand_is_vendor_reserved_blocks(page_in_block + mapped_block
						* page_per_block) == 1)
		return 1;

	/* write bad index into oob */
	if (mapped_block != block)
		set_bad_index_to_oob(chip->oob_poi, block);
	else
		set_bad_index_to_oob(chip->oob_poi, FAKE_INDEX);

	memset(local_oob_buf, 0xFF, mtd->oobsize);
	mtk_snand_transfer_oob(chip, local_oob_buf);
	if (mtk_nand_exec_write_page(mtd, page_in_block + mapped_block
				     * page_per_block, mtd->writesize,
				     (u8 *) buf, local_oob_buf, 0)) {
		pr_warn("write fail at: 0x%x, page: 0x%x\n",
				     mapped_block, page_in_block);
		if (snfc->chip_data->use_bmt) {
			if (update_bmt((page_in_block + mapped_block * page_per_block)
					<< chip->page_shift, UPDATE_WRITE_FAIL,
					(u8 *) buf, local_oob_buf))
				return 0;
			else
				return -EIO;
		} else {
			return -EIO;
		}
	}

	do_gettimeofday(&etimer);
	g_NandPerfLog.WritePageTotalTime += Cal_timediff(&etimer, &stimer);
	g_NandPerfLog.WritePageCount++;

	return 0;
}


static void mtk_snand_dev_read_id(struct mtk_snfc *snfc, u8 id[])
{
	u8 cmd = SNAND_CMD_READ_ID;

	mtk_snand_dev_command_ext(snfc, SPI, &cmd, id, 1, SNAND_MAX_ID + 1);
}

static void mtk_snand_command_bp(struct mtd_info *mtd, unsigned int command,
				 int column, int page_addr)
{
	struct nand_chip *nand = mtd_to_nand(mtd);
	struct mtk_snfc *snfc = nand_get_controller_data(nand);

	switch (command) {
	case NAND_CMD_SEQIN:
		memset(g_kCMD.au1OOB, 0xFF, sizeof(g_kCMD.au1OOB));
		g_kCMD.pDataBuf = NULL;
		g_kCMD.u4RowAddr = page_addr;
		g_kCMD.u4ColAddr = column;
		break;

	case NAND_CMD_PAGEPROG:
		if (g_kCMD.pDataBuf || (g_kCMD.au1OOB[0] != 0xFF)) {
			u8 *pDataBuf = g_kCMD.pDataBuf ? g_kCMD.pDataBuf
				       : nand->buffers->databuf;
			mtk_nand_exec_write_page(mtd, g_kCMD.u4RowAddr,
						 mtd->writesize, pDataBuf,
						 g_kCMD.au1OOB, 0);
			g_kCMD.u4RowAddr = (u32) -1;
			g_kCMD.u4OOBRowAddr = (u32) -1;
		}
		break;

	case NAND_CMD_READOOB:
		g_kCMD.u4RowAddr = page_addr;
		g_kCMD.u4ColAddr = column + mtd->writesize;
		break;

	case NAND_CMD_READ0:
		g_kCMD.u4RowAddr = page_addr;
		g_kCMD.u4ColAddr = column;
		break;

	case NAND_CMD_ERASE1:
		g_snand_erase_cmds = 1;
		g_snand_erase_addr = page_addr;
		break;

	case NAND_CMD_ERASE2:
		if (g_snand_erase_cmds) {
			g_snand_erase_cmds = 0;
			pr_debug("erase page:0x%x\n", g_snand_erase_addr);
			mtk_nand_erase(mtd, g_snand_erase_addr);
		}
		break;

	case NAND_CMD_STATUS:
		if ((g_snand_dev_status == SNAND_DEV_ERASE_DONE) ||
		    (g_snand_dev_status == SNAND_DEV_PROG_DONE))
			g_snand_dev_status = SNAND_IDLE;

		g_snand_read_byte_mode = SNAND_RB_CMD_STATUS;
		break;

	case NAND_CMD_RESET:
		(void)mtk_snand_reset_con(snfc);
		g_snand_read_byte_mode = SNAND_RB_DEFAULT;
		break;

	case NAND_CMD_READID:
		mtk_snand_reset_dev(snfc);
		mtk_snand_reset_con(snfc);
		mtk_snand_dev_read_id(snfc, g_snand_id_data);
		g_snand_id_data_idx = 1;
		g_snand_read_byte_mode = SNAND_RB_READ_ID;

		break;

	default:
		break;
	}
}

static void mtk_snand_select_chip(struct mtd_info *mtd, int chip)
{
	u32 reg;
	struct nand_chip *nand = mtd_to_nand(mtd);
	struct mtk_snfc  *snfc = nand_get_controller_data(nand);

	if (!snfc) {
		pr_warn("failed to allocate device structure.\n");
		return;
	}

	if (chip == -1 && g_bInitDone == 0) {
		u32 spare_per_sector = mtd->oobsize/(mtd->writesize/512);
		u32 ecc_bit = 4;
		u32 spare_bit = PAGEFMT_SPARE_16;

		if (spare_per_sector >= 28) {
			spare_bit = PAGEFMT_SPARE_28;
			ecc_bit = 12;
			spare_per_sector = 28;
		} else if (spare_per_sector >= 27) {
			spare_bit = PAGEFMT_SPARE_27;
			ecc_bit = 8;
			spare_per_sector = 27;
		} else if (spare_per_sector >= 26) {
			spare_bit = PAGEFMT_SPARE_26;
			ecc_bit = 8;
			spare_per_sector = 26;
		} else if (spare_per_sector >= 16) {
			spare_bit = PAGEFMT_SPARE_16;
			ecc_bit = 4;
			spare_per_sector = 16;
		} else {
			pr_warn("[NAND]: NFI not support oobsize: %x\n",
				spare_per_sector);
		}

		mtd->oobsize = spare_per_sector*(mtd->writesize/512);
		nand->ecc.strength = ecc_bit;
		pr_warn("[NAND]select ecc bit:%d, sparesize :%d\n",
			ecc_bit, mtd->oobsize);

		/* Setup PageFormat */
		if (mtd->writesize  == 4096) {
			reg = snfi_readl(snfc, NFI_PAGEFMT)
					 | ((spare_bit << PAGEFMT_SPARE_SHIFT)
					 | PAGEFMT_4K | PAGEFMT_SEC_SEL_512);
			snfi_writel(snfc, reg, NFI_PAGEFMT);
			nand->cmdfunc = mtk_snand_command_bp;
		} else if (mtd->writesize == 2048) {
			reg = snfi_readl(snfc, NFI_PAGEFMT) |
					 ((spare_bit << PAGEFMT_SPARE_SHIFT)
					 | PAGEFMT_2K | PAGEFMT_SEC_SEL_512);
			snfi_writel(snfc, reg, NFI_PAGEFMT);
			nand->cmdfunc = mtk_snand_command_bp;
		}
		mtk_snand_ecc_config(snfc, host->hw, ecc_bit);
		g_snand_rs_ecc_bit_second_part = ecc_bit;
		/*[NOTE]:add by bayi, otherwise  it will skip read*/
		g_snand_rs_ecc_bit = ecc_bit;
		g_snand_rs_spare_per_sector_second_part_nfi
			= (spare_bit << PAGEFMT_SPARE_SHIFT);
		g_snand_k_spare_per_sec = spare_per_sector;

		g_bInitDone = 1;
	}
	switch (chip) {
	case -1:
		break;
	case 0:
	case 1:
		snfi_writel(snfc, chip, NFI_CSEL);
		break;
	}
}

static uint8_t mtk_snand_read_byte(struct mtd_info *mtd)
{
	struct nand_chip *nand = mtd_to_nand(mtd);
	struct mtk_snfc *snfc = nand_get_controller_data(nand);
	u8      reg8;
	u32     reg32;

	if (g_snand_read_byte_mode == SNAND_RB_READ_ID) {
		if (g_snand_id_data_idx > SNAND_MAX_ID)
			return 0;
		else
			return g_snand_id_data[g_snand_id_data_idx++];
	} else if (g_snand_read_byte_mode == SNAND_RB_CMD_STATUS) {
		if ((g_snand_dev_status == SNAND_DEV_ERASE_DONE) ||
		    (g_snand_dev_status == SNAND_DEV_PROG_DONE))
			return g_snand_cmd_status | NAND_STATUS_WP;

		else if (g_snand_dev_status == SNAND_DEV_PROG_EXECUTING) {
			snfi_writel(snfc, (SNAND_CMD_GET_FEATURES
				    | (SNAND_CMD_FEATURES_STATUS << 8)),
				    NFI_SNAND_GPRAM_DATA);

			snfi_writel(snfc, 2, NFI_SNAND_MAC_OUTL);
			snfi_writel(snfc, 1, NFI_SNAND_MAC_INL);
			mtk_snand_dev_mac_op(snfc, SPI);

			reg32 = snfi_readl(snfc, NFI_SNAND_GPRAM_DATA);
			reg8  = (reg32>>16)&0xFF;

			if ((reg8 & SNAND_STATUS_OIP) == 0) {
				/* ready but having fail report from device */
				g_snand_dev_status = SNAND_DEV_PROG_DONE;
				if ((reg8 & SNAND_STATUS_PROGRAM_FAIL) != 0) {
					pr_warn("[snand] Prog failed!\n");
					g_snand_cmd_status = NAND_STATUS_READY
							     | NAND_STATUS_FAIL
							     | NAND_STATUS_WP;
				} else {
					g_snand_cmd_status = NAND_STATUS_READY
							     | NAND_STATUS_WP;
				}
			} else {
				return NAND_STATUS_WP;	/* busy */
			}
		} else if (g_snand_dev_status == SNAND_NFI_AUTO_ERASING) {
			reg32 = snfi_readl(snfc, NFI_SNAND_STA_CTL1);

			if ((reg32 & SNAND_AUTO_BLKER) == 0) {
				g_snand_cmd_status = NAND_STATUS_WP;
			} else {
				g_snand_dev_status = SNAND_DEV_ERASE_DONE;
				reg8 = (u8)(snfi_readl(snfc, NFI_SNAND_GF_CTL1)
					& SNAND_GF_STATUS_MASK);

				if (0 != (reg8 & SNAND_STATUS_ERASE_FAIL)) {
					pr_warn("[snand] Erase failed!\n");
					g_snand_cmd_status = NAND_STATUS_READY
							     | NAND_STATUS_FAIL
							     | NAND_STATUS_WP;
				} else {
					g_snand_cmd_status = NAND_STATUS_READY
							     | NAND_STATUS_WP;
				}
			}
		} else if (g_snand_dev_status == SNAND_NFI_CUST_READING) {
			g_snand_cmd_status = NAND_STATUS_WP;
		} else {
			g_snand_dev_status = SNAND_IDLE;
			g_snand_cmd_status = (NAND_STATUS_READY
					      | NAND_STATUS_WP);
			/* indicate SPI-NAND does NOT WP */
		}
	} else {
		return 0;
	}

	return g_snand_cmd_status;
}

static void mtk_snand_read_buf(struct mtd_info *mtd, uint8_t *buf, int len)
{
	struct nand_chip *nand = (struct nand_chip *)mtd->priv;
	struct NAND_CMD *pkCMD = &g_kCMD;
	u32 u4ColAddr = pkCMD->u4ColAddr;
	u32 u4PageSize = mtd->writesize;
	u32 u4Size, u4Offset;

	if (u4ColAddr < u4PageSize) {
		if ((u4ColAddr == 0) && (len >= u4PageSize)) {
			mtk_nand_exec_read_page(mtd, pkCMD->u4RowAddr,
						u4PageSize, buf,
						pkCMD->au1OOB);
			if (len > u4PageSize) {
				u4Size = min((size_t)(len - u4PageSize),
					     sizeof(pkCMD->au1OOB));

				memcpy(buf + u4PageSize, pkCMD->au1OOB,
				       u4Size);
			}
		} else {
			mtk_nand_exec_read_page(mtd, pkCMD->u4RowAddr, u4PageSize,
						nand->buffers->databuf, pkCMD->au1OOB);
			memcpy(buf, nand->buffers->databuf + u4ColAddr, len);
		}
		pkCMD->u4OOBRowAddr = pkCMD->u4RowAddr;
	} else {
		u4Offset = u4ColAddr - u4PageSize;
		u4Size = min((size_t)(len - u4Offset), sizeof(pkCMD->au1OOB));

		if (pkCMD->u4OOBRowAddr != pkCMD->u4RowAddr) {
			mtk_nand_exec_read_page(mtd, pkCMD->u4RowAddr,
						u4PageSize,
						nand->buffers->databuf,
						pkCMD->au1OOB);
			pkCMD->u4OOBRowAddr = pkCMD->u4RowAddr;
		}
		memcpy(buf, pkCMD->au1OOB + u4Offset, u4Size);
	}
	pkCMD->u4ColAddr += len;
}

static void mtk_snand_write_buf(struct mtd_info *mtd, const uint8_t *buf,
				int len)
{
	struct NAND_CMD *pkCMD = &g_kCMD;
	u32 u4ColAddr = pkCMD->u4ColAddr;
	u32 u4PageSize = mtd->writesize;
	u32 i4Size, i, u4Offset;
	u8 *pOOB;

	if (u4ColAddr >= u4PageSize) {
		u4Offset = u4ColAddr - u4PageSize;
		pOOB = pkCMD->au1OOB + u4Offset;

		i4Size = min(len, (int)(sizeof(pkCMD->au1OOB) - u4Offset));

		for (i = 0; i < i4Size; i++)
			pOOB[i] &= buf[i];
	} else {
		pkCMD->pDataBuf = (u8 *)buf;
	}

	pkCMD->u4ColAddr += len;
}

int mtk_snand_write_page_hwecc(struct mtd_info *mtd, struct nand_chip *chip,
			       const uint8_t *buf, int oob_required, int page)
{
	mtk_snand_write_buf(mtd, buf, mtd->writesize);
	mtk_snand_write_buf(mtd, chip->oob_poi, mtd->oobsize);

	return 0;
}

static int mtk_snand_read_page_hwecc(struct mtd_info *mtd,
				     struct nand_chip *chip, uint8_t *buf,
				     int oob_required, int page)
{
	struct mtk_snfc *snfc = nand_get_controller_data(chip);
	struct NAND_CMD *pkCMD = &g_kCMD;
	u32 u4ColAddr = pkCMD->u4ColAddr;
	u32 u4PageSize = mtd->writesize;
	int page_per_block = 1 << (chip->phys_erase_shift - chip->page_shift);
	int block = pkCMD->u4RowAddr / page_per_block;
	u16 page_in_block = pkCMD->u4RowAddr % page_per_block;
	int mapped_block;

	if (!snfc->chip_data->use_bmt)
		mapped_block = block;
	else
		mapped_block = get_mapping_block_index(block);

	memset(local_oob_buf, 0xFF, mtd->oobsize);
	if (u4ColAddr == 0) {
		mtk_nand_exec_read_page(mtd, page_in_block + page_per_block * mapped_block, u4PageSize, buf,
					local_oob_buf);

		/*
		 * Initialise oob buf to all 0xFF,beacause of driver skip first byte
		 * of spare per sector to compatible with Jffs2.
		 */
		memset(chip->oob_poi, 0xff, mtd->oobsize);
		mtk_snand_fill_oob(chip, local_oob_buf);
		pkCMD->u4ColAddr += u4PageSize + mtd->oobsize;
	}
	return 0;
}

static void mtk_snand_dev_stop_erase(struct mtk_snfc *snfc)
{
	u32 reg;

	/* set 1 then set 0 to clear done flag */
	reg = snfi_readl(snfc, NFI_SNAND_STA_CTL1);

	snfi_writel(snfc, reg, NFI_SNAND_STA_CTL1);
	reg = reg & ~SNAND_AUTO_BLKER;
	snfi_writel(snfc, reg, NFI_SNAND_STA_CTL1);

	/* clear trigger bit */
	reg = snfi_readl(snfc, NFI_SNAND_ER_CTL);
	reg &= ~SNAND_AUTO_ERASE_TRIGGER;

	snfi_writel(snfc, reg, NFI_SNAND_ER_CTL);

	g_snand_dev_status = SNAND_IDLE;
}

static void mtk_snand_dev_erase(struct mtk_snfc *snfc, u32 row_addr)
{
	struct nand_chip *chip = &host->nand_chip;
	struct mtd_info *mtd = nand_to_mtd(chip);
	u32 reg;

	mtk_snand_reset_con(snfc);

	row_addr = mtk_snand_dev_die_select(mtd, row_addr);

	/* write enable */
	mtk_snand_dev_command(snfc, SNAND_CMD_WRITE_ENABLE, 1);
	mtk_snand_reset_dev(snfc);

	/* erase address */
	snfi_writel(snfc, row_addr, NFI_SNAND_ER_CTL2);

	/* set loop limit and polling cycles */
	reg = (SNAND_LOOP_LIMIT_NO_LIMIT << SNAND_LOOP_LIMIT_OFFSET) | 0x20;
	snfi_writel(snfc, reg, NFI_SNAND_GF_CTL3);

	/* set latch latency & CS de-select latency (ignored) */
	/* set erase command */
	reg = SNAND_CMD_BLOCK_ERASE << SNAND_ER_CMD_OFFSET;
	snfi_writel(snfc, reg, NFI_SNAND_ER_CTL);

	/* trigger interrupt waiting */
	reg = snfi_readw(snfc, NFI_INTR_EN);
	reg = INTR_AUTO_BLKER_INTR_EN;
	snfi_writew(snfc, reg, NFI_INTR_EN);

	/* trigger auto erase */
	reg = snfi_readl(snfc, NFI_SNAND_ER_CTL);
	reg |= SNAND_AUTO_ERASE_TRIGGER;
	snfi_writel(snfc, reg, NFI_SNAND_ER_CTL);

	g_snand_dev_status = SNAND_NFI_AUTO_ERASING;
	g_snand_cmd_status = 0; /* busy */
}

int mtk_nand_erase_hw(struct mtd_info *mtd, int page)
{
	struct nand_chip *chip = mtd_to_nand(mtd);
	struct mtk_snfc *snfc = nand_get_controller_data(chip);
	int page_per_block = 1 << (chip->phys_erase_shift - chip->page_shift);
	int block = page / page_per_block;
	int ret;

	if (mtk_snand_is_vendor_reserved_blocks(page) == 1)
		return NAND_STATUS_FAIL;

	mtk_snand_dev_erase(snfc, block * page_per_block);
	ret = chip->waitfunc(mtd, chip);

	/* FIXME: debug */
	if (ret & NAND_STATUS_FAIL)
		pr_warn("[K-SNAND][%s] Erase blk %d failed!\n", __func__,
			page / 64);

	mtk_snand_dev_stop_erase(snfc);

	return ret;
}

static int mtk_nand_erase(struct mtd_info *mtd, int page)
{
	struct nand_chip *chip = mtd->priv;
	struct mtk_snfc *snfc = nand_get_controller_data(chip);
	int page_per_block = 1 << (chip->phys_erase_shift - chip->page_shift);
	int page_in_block = page % page_per_block;
	int block = page / page_per_block;
	int mapped_block;
	int status;

	if (!snfc->chip_data->use_bmt)
		mapped_block = block;
	else
		mapped_block = get_mapping_block_index(block);

	status = mtk_nand_erase_hw(mtd, page_in_block + page_per_block
				   * mapped_block);

	if (status & NAND_STATUS_FAIL) {
		if (snfc->chip_data->use_bmt) {
			if (update_bmt((page_in_block + mapped_block * page_per_block)
				<< chip->page_shift, UPDATE_ERASE_FAIL, NULL, NULL))
				return 0;
			else
				return -EIO;
		} else {
			return -EIO;
		}
	}
	g_NandPerfLog.EraseBlockCount++;

	return 0;
}

static int mtk_snand_read_oob_raw(struct mtd_info *mtd, uint8_t *buf,
				  int page_addr, int len)
{
	int bRet = 1;
	int num_sec, num_sec_original;
	u32 i;
	struct nand_chip *nand = mtd_to_nand(mtd);
	struct mtk_snfc *snfc = nand_get_controller_data(nand);

	if (len > NAND_MAX_OOBSIZE || len % OOB_AVAI_PER_SECTOR || !buf) {
		pr_warn("[%s] invalid parameter, len: %d, buf: %p\n",
			__func__, len, buf);
		return -EINVAL;
	}

	num_sec_original = num_sec = len / OOB_AVAI_PER_SECTOR;
	page_addr = mtk_snand_dev_die_select(mtd, page_addr);
	mtk_snand_rs_reconfig_nfiecc(page_addr);

	if (((num_sec_original * NAND_SECTOR_SIZE) == mtd->writesize)
	      && mtk_snand_rs_if_require_split())
		num_sec--;

	/* read the 1st sector, with MTK ECC enabled */
	if (g_snand_rs_ecc_bit != 0) {
		if (mtk_snand_ready_for_read_custom(snfc, nand, page_addr, 0,
						    num_sec, g_snand_k_temp,
						    1, 1, 1)) {
			if (!mtk_snand_read_page_data(mtd, g_snand_k_temp,
						      num_sec, 1))
				bRet = 0;

			if (!mtk_snand_check_dececc_done(snfc, num_sec))
				bRet = 0;

			mtk_snand_read_fdm_data(snfc, g_snand_k_spare,
						num_sec);
			bm_swap(mtd, g_snand_k_spare, g_snand_k_temp);
			mtk_snand_stop_read_custom(snfc, 1);
		} else {
			bm_swap(mtd, g_snand_k_spare, g_snand_k_temp);
			mtk_snand_stop_read_custom(snfc, 0);
			bRet = 0;
		}
		dma_unmap_single(snfc->dev, dma_addr, num_sec * (NAND_SECTOR_SIZE
				 + g_snand_k_spare_per_sec),
				 DMA_FROM_DEVICE);
	}

	if (((num_sec_original * NAND_SECTOR_SIZE) == mtd->writesize)
	      && mtk_snand_rs_if_require_split()) {
		/* read part II */
		num_sec++;
		/* note: use local temp buffer to read part 2 */
		mtk_snand_read_page_part2(mtd, page_addr, num_sec,
					  g_snand_k_temp +
					 ((num_sec - 1) * NAND_SECTOR_SIZE));
		/* copy spare data */
		for (i = 0; i < OOB_AVAI_PER_SECTOR; i++)
			g_snand_k_spare[(num_sec - 1) * OOB_AVAI_PER_SECTOR + i]
			= g_snand_k_temp[((num_sec - 1) * NAND_SECTOR_SIZE)
			+ NAND_SECTOR_SIZE + i];
	}

	mtk_snand_dev_enable_spiq(snfc, 0);
	num_sec = num_sec * OOB_AVAI_PER_SECTOR;
	for (i = 0; i < num_sec; i++)
		buf[i] = g_snand_k_spare[i];

	return bRet;
}

static int mtk_snand_write_oob_raw(struct mtd_info *mtd, const uint8_t *buf,
				   int page_addr, int len)
{
	int status = 0;
	u32 i;

	if (len >  NAND_MAX_OOBSIZE || len % OOB_AVAI_PER_SECTOR || !buf) {
		pr_warn("[%s] invalid parameter, len: %d, buf: %p\n",
			__func__, len, buf);
		return -EINVAL;
	}

	memset((void *)g_snand_k_temp, 0xFF, mtd->writesize);
	memset((void *)g_snand_k_spare, 0xFF, mtd->oobsize);

	for (i = 0; i < (u32)len; i++)
		((u8 *)g_snand_k_spare)[i] = buf[i];

	pr_debug("[K-SNAND] g_snand_k_spare:\n");
	mtk_snand_dump_mem((u32 *)g_snand_k_spare, mtd->oobsize);
	status = mtk_nand_exec_write_page(mtd, page_addr, mtd->writesize,
					 (u8 *)g_snand_k_temp,
					 (u8 *)g_snand_k_spare, 1);

	pr_debug("[K-SNAND][%s] page_addr=%d (blk=%d), status=%d\n",
		__func__, page_addr, page_addr / 64, status);

	return status;
}

static int mtk_snand_write_oob_hw(struct mtd_info *mtd, struct nand_chip *chip,
				  int page)
{
	int sec_num = 1<<(chip->page_shift-9);

	memset(local_oob_buf, 0xFF, mtd->oobsize);
	mtk_snand_transfer_oob(chip, local_oob_buf);

	return mtk_snand_write_oob_raw(mtd, local_oob_buf, page,  (sec_num * OOB_AVAI_PER_SECTOR));
}

static int mtk_snand_write_oob(struct mtd_info *mtd, struct nand_chip *chip,
			       int page)
{
	struct mtk_snfc *snfc = nand_get_controller_data(chip);
	int page_per_block = 1 << (chip->phys_erase_shift - chip->page_shift);
	int block = page / page_per_block;
	u16 page_in_block = page % page_per_block;
	int mapped_block;

	if (!snfc->chip_data->use_bmt)
		mapped_block = block;
	else
		mapped_block = get_mapping_block_index(block);

	/* write bad index into oob */
	if (mapped_block != block)
		set_bad_index_to_oob(chip->oob_poi, block);
	else
		set_bad_index_to_oob(chip->oob_poi, FAKE_INDEX);

	if (mtk_snand_write_oob_hw(mtd, chip, page_in_block + mapped_block
				   * page_per_block)) {
		memset(local_oob_buf, 0xFF, mtd->oobsize);
		mtk_snand_transfer_oob(chip, local_oob_buf);
		if (snfc->chip_data->use_bmt) {
			if (update_bmt((page_in_block + mapped_block * page_per_block)
					<< chip->page_shift, UPDATE_WRITE_FAIL, NULL,
					local_oob_buf))
				return 0;
			else
				return -EIO;
		} else {
			return -EIO;
		}
	}

	return 0;
}

int mtk_nand_block_markbad_hw(struct mtd_info *mtd, loff_t offset)
{
	struct nand_chip *chip = mtd->priv;
	int block = (int)offset >> chip->phys_erase_shift;
	int page = block * (1 << (chip->phys_erase_shift - chip->page_shift));
	int ret;
	u32 buf[8]; /* 8 bytes */

	mtk_nand_erase_hw(mtd, page);  /* erase before marking bad */

	memset((u8 *)buf, 0xFF, 8);
	((u8 *)buf)[0] = 0;
	ret = mtk_snand_write_oob_raw(mtd, (u8 *)buf, page, 8);

	return ret;
}

static int mtk_snand_block_markbad(struct mtd_info *mtd, loff_t offset)
{
	struct nand_chip *chip = mtd->priv;
	struct mtk_snfc *snfc = nand_get_controller_data(chip);
	int block = (int)offset >> chip->phys_erase_shift;
	int mapped_block;

	if (!snfc->chip_data->use_bmt)
		mapped_block = block;
	else
		mapped_block = get_mapping_block_index(block);

	return  mtk_nand_block_markbad_hw(mtd, mapped_block
					  << chip->phys_erase_shift);
}

int mtk_snand_read_oob_hw(struct mtd_info *mtd, struct nand_chip *chip,
			  int page)
{
	int sec_num = 1 << (chip->page_shift - 9);

	memset(local_oob_buf, 0xFF, mtd->oobsize);
	if (mtk_snand_read_oob_raw(mtd, local_oob_buf, page, (sec_num * OOB_AVAI_PER_SECTOR))
				   == 0) {
		pr_warn("[%s]read oob raw failed\n", __func__);
		return -EIO;
	}

	/*
	 * Initialise oob buf to all 0xFF,beacause of driver skip first byte
	 * of spare per sector to compatible with Jffs2.
	 */
	memset(chip->oob_poi, 0xff, mtd->oobsize);
	mtk_snand_fill_oob(chip, local_oob_buf);

	return 0;
}

static int mtk_snand_read_oob(struct mtd_info *mtd, struct nand_chip *chip,
			      int page)
{
	struct mtk_snfc *snfc = nand_get_controller_data(chip);
	int page_per_block = 1 << (chip->phys_erase_shift - chip->page_shift);
	int block = page / page_per_block;
	u16 page_in_block = page % page_per_block;
	int mapped_block;

	if (!snfc->chip_data->use_bmt)
		mapped_block = block;
	else
		mapped_block = get_mapping_block_index(block);

	return  mtk_snand_read_oob_hw(mtd, chip, page_in_block + mapped_block
				      * page_per_block);
}

int mtk_nand_block_bad_hw(struct mtd_info *mtd, loff_t ofs)
{
	struct nand_chip *chip = mtd_to_nand(mtd);
	struct mtk_snfc *snfc = nand_get_controller_data(chip);
	int page_addr = (int)(ofs >> chip->page_shift);
	unsigned int page_per_block = 1 << (chip->phys_erase_shift
					- chip->page_shift);
	unsigned char oob_buf[64];
	unsigned int u4PageSize = 1 << chip->page_shift;
	u32 u4SecNum = u4PageSize >> 9;
	unsigned int offset = 0;

	if (snfc->chip_data->type == MTK_NAND_LEOPARD)
		offset = (u4SecNum - 1) * OOB_AVAI_PER_SECTOR;

	page_addr &= ~(page_per_block - 1);
	/* return bad block if it is reserved block */
	if (mtk_snand_is_vendor_reserved_blocks(page_addr) == 1)
		return 1;

	if (mtk_snand_read_oob_raw(mtd, oob_buf, page_addr, u4SecNum * OOB_AVAI_PER_SECTOR)
				   == 0) {
		pr_warn("mtk_snand_read_oob_raw return error\n");
		return 1;
	}

	if (oob_buf[offset] != 0xff) {
		pr_debug("Bad block at 0x%x (blk:%d), Badmark is is 0x%x\n",
			page_addr, page_addr / page_per_block, oob_buf[offset]);
		return 1;
	}
	/* everything is OK, good block */
	return 0;
}

static int mtk_snand_block_bad(struct mtd_info *mtd, loff_t ofs)
{
	struct nand_chip *chip = (struct nand_chip *)mtd->priv;
	struct mtk_snfc *snfc = nand_get_controller_data(chip);
	int block = (int)ofs >> chip->phys_erase_shift;
	int mapped_block;
	int ret;

	if (!snfc->chip_data->use_bmt)
		mapped_block = block;
	else
		mapped_block = get_mapping_block_index(block);

	ret = mtk_nand_block_bad_hw(mtd, mapped_block
				    << chip->phys_erase_shift);

	if (ret) {
		if (snfc->chip_data->use_bmt) {
			pr_debug("Unmapped bad block: 0x%x\n", mapped_block);
			if (update_bmt(mapped_block << chip->phys_erase_shift,
				       UPDATE_UNMAPPED_BLOCK, NULL, NULL)) {
				ret = 0;
			} else {
				pr_debug("Update BMT fail\n");
				ret = 1;
			}
		} else {
			ret = 1;
		}
	}

	return ret;
}

static void mtk_snand_init_hw(struct mtk_snfc *snfc,
			      struct mtk_snand_host *host)
{
	struct mtk_snand_host_hw *hw = host->hw;
	u32 reg;

	g_bInitDone = 0;
	g_kCMD.u4OOBRowAddr = (u32) -1;

	/* Set default NFI access timing control */
	snfi_writel(snfc, /*0x30c77fff*/ 0x10804211, NFI_ACCCON);
	snfi_writew(snfc, 0, NFI_CNFG);

	/* Reset the state machine and data FIFO, because flushing FIFO */
	(void)mtk_snand_reset_con(snfc);

	/* Set the ECC engine */
	if (hw->nand_ecc_mode == NAND_ECC_HW) {
		if (g_bHwEcc) {
			reg = snfi_readw(snfc, NFI_CNFG) | CNFG_HW_ECC_EN;
			snfi_writew(snfc, reg, NFI_CNFG);
		}
		mtk_snand_ecc_config(snfc, hw, 4);
		mtk_snand_configure_fdm(snfc, 8, 1);
	}

	/* Initialize interrupt. Clear interrupt, read clear. */
	snfi_readw(snfc, NFI_INTR_STA);

	snfi_writew(snfc, 0, NFI_INTR_EN);
	snfi_writew(snfc, WBUF_EN, NFI_DEBUG_CON1);

	snfi_writel(snfc, 1, NFI_SNAND_CNFG);

	/* NOTE(Bayi): reset device and set ecc control */
	mdelay(10);
	mtk_snand_reset_dev(snfc);
	/* NOTE(Bayi): disable internal ecc, and use mtk ecc */
	mtk_snand_dev_ecc_control(snfc);
	mtk_snand_dev_unlock_all_blocks(snfc);
}

static int mtk_snand_dev_ready(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd_to_nand(mtd);
	struct mtk_snfc *snfc = nand_get_controller_data(chip);
	u8  reg8;
	u32 reg32;

	if (g_snand_dev_status == SNAND_DEV_PROG_EXECUTING) {
		snfi_writel(snfc, (SNAND_CMD_GET_FEATURES
			    | (SNAND_CMD_FEATURES_STATUS << 8)),
			    NFI_SNAND_GPRAM_DATA);

		snfi_writel(snfc, 2, NFI_SNAND_MAC_OUTL);
		snfi_writel(snfc, 1, NFI_SNAND_MAC_INL);

		mtk_snand_dev_mac_op(snfc, SPI);

		reg32 = snfi_readl(snfc, NFI_SNAND_GPRAM_DATA);
		reg8  = (reg32>>16)&0xFF;

		if ((reg8 & SNAND_STATUS_OIP) == 0) {
			g_snand_dev_status = SNAND_DEV_PROG_DONE;
			/* ready but having fail report from device */
			if (0 != (reg8 & SNAND_STATUS_PROGRAM_FAIL))
				g_snand_cmd_status = NAND_STATUS_READY
						     | NAND_STATUS_FAIL;
			else
				g_snand_cmd_status = NAND_STATUS_READY;
		} else {
			g_snand_cmd_status = 0;
		}
	} else if (g_snand_dev_status == SNAND_NFI_AUTO_ERASING) {
		/* wait for auto erase finish */
		reg32 = snfi_readl(snfc, NFI_SNAND_STA_CTL1);

		if ((reg32 & SNAND_AUTO_BLKER) == 0) {
			g_snand_cmd_status = 0; /* busy */
		} else {
			g_snand_dev_status = SNAND_DEV_ERASE_DONE;

			reg8 = (u8)(snfi_readl(snfc, NFI_SNAND_GF_CTL1)
					       & SNAND_GF_STATUS_MASK);
			/* ready but having fail report from device */
			if (0 != (reg8 & SNAND_STATUS_ERASE_FAIL))
				g_snand_cmd_status = NAND_STATUS_READY
						     | NAND_STATUS_FAIL;
			else
				g_snand_cmd_status = NAND_STATUS_READY;
		}
	} else if (g_snand_dev_status == SNAND_NFI_CUST_READING) {
		g_snand_cmd_status = 0; /* busy */
	} else {
		g_snand_cmd_status = NAND_STATUS_READY; /* idle */
	}

	return g_snand_cmd_status;
}

#ifdef SNAND_TEST
#define PAGE_IDX  0x8002
#define PAGE_SIZE_TEST 2048
void mtk_nand_test_kernel(struct nand_chip *nand_chip, struct mtd_info *mtd)
{
	char *pDateBuf_w, *pDateBuf_r, pFdmBuf[64];

	pDateBuf_w = kzalloc(PAGE_SIZE_TEST + 128, GFP_KERNEL);
	pDateBuf_r = kzalloc(PAGE_SIZE_TEST + 128, GFP_KERNEL);

	mtk_nand_erase(mtd, PAGE_IDX);
	memset(pDateBuf_r, 0xFE, PAGE_SIZE_TEST);
	mtk_nand_exec_read_page(mtd, PAGE_IDX, PAGE_SIZE_TEST, pDateBuf_r,
				pFdmBuf);
	pr_warn("read data after erase: 0x%x\n", pDateBuf_r[0]);

	memset(pDateBuf_w, 0xBC, PAGE_SIZE_TEST);

	if (nand_chip->write_page(mtd, nand_chip, 0, PAGE_SIZE_TEST,
				  pDateBuf_w, 0, PAGE_IDX, 0, 0))
		pr_warn("[Test]Write page fail!\n");
	else
		pr_warn("[Test]Write page Success!\n");

	memset(pDateBuf_r, 0xFE, PAGE_SIZE_TEST);
	mtk_nand_exec_read_page(mtd, PAGE_IDX, PAGE_SIZE_TEST, pDateBuf_r,
				pFdmBuf);

	pr_warn("read data after write: 0x%x\n", pDateBuf_r[0]);

	if (memcmp(pDateBuf_w, pDateBuf_r, PAGE_SIZE_TEST))
		pr_warn("[Test]Compare data fail!\n");
	else
		pr_warn("[Test]Compare data success!\n");
}
#endif


static int mtk_snfc_enable_clk(struct device *dev, struct mtk_snfc_clk *clk)
{
	int ret;

	ret = clk_prepare_enable(clk->nfi_clk);
	if (ret) {
		dev_err(dev, "failed to enable nfi clk\n");
		return ret;
	}

	ret = clk_prepare_enable(clk->pad_clk);
	if (ret) {
		dev_err(dev, "failed to enable pad clk\n");
		clk_disable_unprepare(clk->nfi_clk);
		return ret;
	}

	ret = clk_prepare_enable(clk->nfiecc_clk);
	if (ret) {
		dev_err(dev, "failed to enable ecc clk\n");
		clk_disable_unprepare(clk->nfi_clk);
		clk_disable_unprepare(clk->pad_clk);
		return ret;
	}

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static void mtk_snfc_disable_clk(struct mtk_snfc_clk *clk)
{
	clk_disable_unprepare(clk->nfi_clk);
	clk_disable_unprepare(clk->pad_clk);
	clk_disable_unprepare(clk->nfiecc_clk);
}
#endif

static const struct mtk_snand_type snfc_mt7622 = {
	.nfi_regs = mt7622_snfi_regs,
	.type = MTK_NAND_MT7622,
	.fdm_ecc_size = 1,
	.no_bm_swap = 1,
	.use_bmt = 1,
};

static const struct mtk_snand_type snfc_leopard = {
	.nfi_regs = mt7622_snfi_regs,
	.type = MTK_NAND_LEOPARD,
	.fdm_ecc_size = 1,
	.no_bm_swap = 0,
	.use_bmt = 0,
};

static const struct of_device_id mtk_snfc_id_table[] = {
	{
		.compatible = "mediatek,mt7622-snand",
		.data = &snfc_mt7622,
	}, {
		.compatible = "mediatek,leopard-snand",
		.data = &snfc_leopard,
	}, {
	}
};
MODULE_DEVICE_TABLE(of, mtk_snfc_id_table);

static const char * const probes[] = {"ofpart", NULL};
static int mtk_snand_probe(struct platform_device *pdev)
{
	struct mtk_snand_host_hw *hw;
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct mtd_info *mtd;
	struct mtk_snfc *snfc;
	struct mtk_snfc_nand_chip *chip;
	struct nand_chip *nand_chip;
	struct resource *res = pdev->resource;
	const struct of_device_id *of_nand_id = NULL;
	struct mtd_part_parser_data ppdata;
	int err = 0;
	u8 id[SNAND_MAX_ID];
	int i, nsels, tmp, ret;

	of_nand_id = of_match_device(mtk_snfc_id_table, &pdev->dev);
	if (!of_nand_id)
		return -EINVAL;

	if (!np) {
		dev_err(&pdev->dev, "No DT found\n");
		return -EINVAL;
	}

	snfc = devm_kzalloc(dev, sizeof(*snfc), GFP_KERNEL);
	if (!snfc)
		return -ENOMEM;

	spin_lock_init(&snfc->controller.lock);
	init_waitqueue_head(&snfc->controller.wq);
	INIT_LIST_HEAD(&snfc->chips);

	snfc->dev = dev;
	snfc->chip_data = (struct mtk_snand_type *)of_nand_id->data;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	snfc->regs = devm_ioremap_resource(dev, res);
	if (IS_ERR(snfc->regs)) {
		err = PTR_ERR(snfc->regs);
		dev_err(dev, "no snfi base\n");
		goto out;
	}
	g_snand_k_temp = kzalloc(4096+128, GFP_KERNEL);

	snfc->clk.nfi_clk = devm_clk_get(dev, "nfi_clk");
	if (IS_ERR(snfc->clk.nfi_clk)) {
		dev_err(dev, "no hclk\n");
		err = PTR_ERR(snfc->clk.nfi_clk);
		goto out;
	}

	snfc->clk.pad_clk = devm_clk_get(dev, "pad_clk");
	if (IS_ERR(snfc->clk.pad_clk)) {
		dev_err(dev, "no pad clk\n");
		err = PTR_ERR(snfc->clk.pad_clk);
		goto out;
	}

	snfc->clk.nfiecc_clk = devm_clk_get(dev, "nfiecc_clk");
	if (IS_ERR(snfc->clk.nfiecc_clk)) {
		dev_err(dev, "no pad clk\n");
		err = PTR_ERR(snfc->clk.nfiecc_clk);
		goto out;
	}

	err = mtk_snfc_enable_clk(dev, &snfc->clk);
	if (err)
		goto out;

	ret = dma_set_mask(dev, DMA_BIT_MASK(32));
	if (ret) {
		dev_err(dev, "failed to set dma mask\n");
		goto out;
	}

	pm_runtime_enable(dev);
	pm_runtime_get_sync(dev);
	platform_set_drvdata(pdev, snfc);

	if (!of_get_property(np, "reg", &nsels))
		return -ENODEV;

	chip = devm_kzalloc(dev, sizeof(*chip) + nsels * sizeof(u8),
			    GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	nsels = 1;
	chip->nsels = nsels;
	for (i = 0; i < nsels; i++) {
		ret = of_property_read_u32_index(np, "reg", i, &tmp);
		if (ret) {
			dev_err(dev, "reg property failure : %d\n", ret);
			return ret;
		}
		chip->sels[i] = tmp;
	}

	hw  = &mtk_nand_hw;
	/* Allocate memory for the device structure (and zero it) */
	host = kzalloc(sizeof(struct mtk_snand_host), GFP_KERNEL);
	if (!host)
		return -ENOMEM;

	host->hw = hw;
	/* init mtd data structure */

	nand_chip = &host->nand_chip;
	nand_chip->controller = &snfc->controller;
	nand_set_flash_node(nand_chip, np);
	nand_set_controller_data(nand_chip, snfc);
	nand_chip->ecc.mode = NAND_ECC_HW; /* enable ECC */
	nand_chip->read_byte = mtk_snand_read_byte;
	nand_chip->read_buf = mtk_snand_read_buf;
	nand_chip->write_buf = mtk_snand_write_buf;
	nand_chip->select_chip = mtk_snand_select_chip;
	nand_chip->dev_ready = mtk_snand_dev_ready;
	nand_chip->cmdfunc = mtk_snand_command_bp;
	nand_chip->ecc.read_page = mtk_snand_read_page_hwecc;
	nand_chip->ecc.write_page = mtk_snand_write_page_hwecc;

	nand_chip->options = NAND_SKIP_BBTSCAN;

	/* For BMT, we need to revise driver architecture */
	nand_chip->write_page = mtk_snand_write_page;

	nand_chip->ecc.write_oob = mtk_snand_write_oob;
	nand_chip->ecc.read_oob = mtk_snand_read_oob;
	nand_chip->block_markbad = mtk_snand_block_markbad;
	nand_chip->erase = mtk_nand_erase;
	nand_chip->block_bad = mtk_snand_block_bad;

	mtd = nand_to_mtd(nand_chip);
	mtd->priv = nand_chip;
	mtd->owner = THIS_MODULE;
	mtd->dev.parent = dev;
	mtd->name = "MTK-SNAND";

	mtk_snand_init_hw(snfc, host);
	/* Select the device */
	nand_chip->select_chip(mtd, 0);
	nand_chip->cmdfunc(mtd, NAND_CMD_RESET, -1, -1);
	nand_chip->cmdfunc(mtd, NAND_CMD_READID, 0x00, -1);

	for (i = 0; i < SNAND_MAX_ID; i++)
		id[i] = nand_chip->read_byte(mtd);

	if (!mtk_snand_get_device_info(id, &devinfo)) {
		err = -ENXIO;
		goto out;
	}

	if (devinfo.advancedmode & SNAND_ADV_TWO_DIE) {
		mtk_snand_dev_die_select_op(snfc, 1);
		/* disable internal ecc, and use mtk ecc */
		mtk_snand_dev_ecc_control(snfc);
		mtk_snand_dev_unlock_all_blocks(snfc);
	}

	if (devinfo.pagesize == 4096)
		hw->nand_ecc_size = 4096;
	else if (devinfo.pagesize == 2048)
		hw->nand_ecc_size = 2048;
	else if (devinfo.pagesize == 512)
		hw->nand_ecc_size = 512;

	/* Modify to fit device character */
	nand_chip->ecc.size = hw->nand_ecc_size;
	nand_chip->ecc.bytes = hw->nand_ecc_bytes;

	if (err != 0)
		goto out;
	mtd->writesize = devinfo.pagesize;
	mtd->oobsize = devinfo.sparesize;
	mtd->erasesize = devinfo.blocksize<<10;

	/* Scan to find existence of the device */
	if (nand_scan(mtd, nsels)) {
		dev_warn(dev, "%s : nand_scan fail.\n", MODULE_NAME);
		err = -ENXIO;
		goto out;
	}

	#if defined(MTK_COMBO_NAND_SUPPORT)
	nand_chip->chipsize -= (PART_SIZE_BMTPOOL);
	#else
	nand_chip->chipsize -= (BMT_POOL_SIZE) << nand_chip->phys_erase_shift;
	#endif
	mtd->size = nand_chip->chipsize;

	/* config read empty threshold for MTK ECC */
	snfi_writel(snfc, 1, NFI_EMPTY_THRESH);

	if (snfc->chip_data->use_bmt)
		if (init_bmt(host,
			1 << (nand_chip->chip_shift - nand_chip->phys_erase_shift),
			(nand_chip->chipsize >> nand_chip->phys_erase_shift) - 2) != 0) {
			dev_info(dev, "Error: init bmt failed\n");
			return 0;
		}

	nand_chip->chipsize -= (PMT_POOL_SIZE) << nand_chip->phys_erase_shift;
	mtd->size = nand_chip->chipsize;

	np = of_get_next_available_child(pdev->dev.of_node, NULL);
	if (!np) {
		dev_err(dev, "no snand device to configure\n");
		return -ENODEV;
	}

	ppdata.of_node = np;
	err = mtd_device_parse_register(mtd, probes, &ppdata, NULL, 0);
	if (err) {
		dev_warn(dev, "[mtk_snand] mtd parse partition error!\n");
		nand_release(mtd);
		return err;
	}

	/* Successfully!! */
	if (!err) {
		dev_warn(dev, "[mtk_snand] probe successfully!\n");

		#if SNAND_TEST
		mtk_nand_test_kernel(nand_chip, mtd);
		#endif

		return err;
	}

 /* Fail!! */
 out:
	nand_release(mtd);
	platform_set_drvdata(pdev, NULL);
	kfree(host);

	return err;
}

static int mtk_snand_remove(struct platform_device *pdev)
{
	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int mtk_snand_suspend(struct device *dev)
{
	struct mtk_snfc *snfc = dev_get_drvdata(dev);

	mtk_snfc_disable_clk(&snfc->clk);

	return 0;
}

static int mtk_snand_resume(struct device *dev)
{
	struct mtk_snfc *snfc = dev_get_drvdata(dev);
	int ret;

	udelay(200);

	ret = mtk_snfc_enable_clk(dev, &snfc->clk);
	if (ret)
		return ret;

	mtk_snand_init_hw(snfc, host);

	return 0;
}

static const struct dev_pm_ops mtk_snand_dev_pm_ops = {
	.suspend = mtk_snand_suspend,
	.resume = mtk_snand_resume,
};

#define MTK_SNAND_DEV_PM_OPS	(&mtk_snand_dev_pm_ops)
#else
#define MTK_SNAND_DEV_PM_OPS	NULL
#endif

static struct platform_driver mtk_snfc_driver = {
	.probe = mtk_snand_probe,
	.remove = mtk_snand_remove,
	.driver = {
		.name = "mtk-snand",
		.of_match_table = mtk_snfc_id_table,
		.pm = MTK_SNAND_DEV_PM_OPS,
	},
};

module_platform_driver(mtk_snfc_driver)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Bayi Cheng <bayi.cheng@mediatek.com>");
MODULE_DESCRIPTION("MTK Spi Nand Flash Controller Driver");

