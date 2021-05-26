/*
 * SPDX-License-Identifier: GPL-2.0
 * MTK NFI and ECC controller
 *
 * Copyright (C) 2018 MediaTek Inc.
 * Authors:	Xiangsheng Hou	<xiangsheng.hou@mediatek.com>
 *
 */

#ifndef __DRIVERS_MTD_NAND_MTK_ECC_H__
#define __DRIVERS_MTD_NAND_MTK_ECC_H__

#include <linux/types.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/mtd.h>

/* NFI controller register definition */
#define NFI_CNFG		(0x00)
#define		CNFG_AHB		BIT(0)
#define		CNFG_READ_EN		BIT(1)
#define		CNFG_DMA_BURST_EN	BIT(2)
#define		CNFG_BYTE_RW		BIT(6)
#define		CNFG_HW_ECC_EN		BIT(8)
#define		CNFG_AUTO_FMT_EN	BIT(9)
#define		CNFG_OP_READ		(1 << 12)
#define		CNFG_OP_PROG		(3 << 12)
#define		CNFG_OP_CUST		(6 << 12)
#define		CNFG_OP_MASK		(7 << 12)
#define NFI_PAGEFMT		(0x04)
#define		PAGEFMT_FDM_ECC_SHIFT	(12)
#define		PAGEFMT_FDM_SHIFT	(8)
#define		PAGEFMT_512		(0)
#define		PAGEFMT_2K		(1)
#define		PAGEFMT_4K		(2)
#define		PAGEFMT_SPARE_16	(0)
#define		PAGEFMT_SPARE_26	(1)
#define		PAGEFMT_SPARE_27	(2)
#define		PAGEFMT_SPARE_28	(3)
#define NFI_CON			(0x08)
#define		CON_FIFO_FLUSH		BIT(0)
#define		CON_NFI_RST		BIT(1)
#define		CON_BRD			BIT(8)  /* burst  read */
#define		CON_BWR			BIT(9)	/* burst  write */
#define		CON_SEC_SHIFT		(12)
#define NFI_ACCCON		(0x0C)
#define NFI_INTR_EN		(0x10)
#define		INTR_AHB_DONE_EN	BIT(6)
#define NFI_INTR_STA		(0x14)
#define NFI_CMD			(0x20)
#define NFI_ADDRNOB		(0x30)
#define		ADDR_COL_NOB_S	(0)
#define		ADDR_ROW_NOB_S	(4)
#define		ADDR_COL_NOB_M	(7)
#define		ADDR_ROW_NOB_M	(7)
#define NFI_COLADDR		(0x34)
#define NFI_ROWADDR		(0x38)
#define NFI_STRDATA		(0x40)
#define		STAR_EN			(1)
#define		STAR_DE			(0)
#define NFI_CNRNB		(0x44)
#define NFI_DATAW		(0x50)
#define NFI_DATAR		(0x54)
#define NFI_PIO_DIRDY		(0x58)
#define		PIO_DI_RDY		(0x01)
#define NFI_STA			(0x60)
#define		STA_CMD			BIT(0)
#define		STA_ADDR		BIT(1)
#define		STA_BUSY		BIT(8)
#define		STA_EMP_PAGE		BIT(12)
#define		NFI_FSM_CUSTDATA	(0xe << 16)
#define		NFI_FSM_MASK		(0xf << 16)
#define NFI_ADDRCNTR		(0x70)
#define		CNTR_MASK		GENMASK(16, 12)
#define		ADDRCNTR_SEC_SHIFT	(12)
#define		ADDRCNTR_SEC(val) \
		(((val) & CNTR_MASK) >> ADDRCNTR_SEC_SHIFT)
#define NFI_STRADDR		(0x80)
#define NFI_BYTELEN		(0x84)
#define NFI_CSEL		(0x90)
#define NFI_FDML(x)		(0xA0 + (x) * sizeof(u32) * 2)
#define NFI_FDMM(x)		(0xA4 + (x) * sizeof(u32) * 2)
#define NFI_MASTER_STA		(0x224)
#define		MASTER_STA_MASK		(0x0FFF)
#define NFI_EMPTY_THRESH	(0x23C)

/* ECC controller register definition */
#define ECC_ENCCON		(0x00)
#define ECC_ENCCNFG		(0x04)
#define ECC_ENCIDLE		(0x0C)
#define		ECC_MS_SHIFT		(16)
#define		DEC_CON_SHIFT		(12)
#define ECC_DECCON		(0x100)
#define		ECC_NFI_MODE		(1)
#define ECC_DECCNFG		(0x104)
#define		DEC_EMPTY_EN		BIT(31)
#define		DEC_CNFG_EL	(0x2 << 12)
#define ECC_DECIDLE		(0x10C)
#define ECC_DECENUM		(0x114)
#define ECC_DECDONE		(0x118)
#define ECC_DECEL(x)		(0x11C + (x) * sizeof(u32))
#define		DEC_EL_SHIFT		(16)
#define		DEC_EL_MASK		(0x1fff)
#define		DEC_EL_BIT_MASK		(0x7)
#define		DEC_EL_BYTE_SHIFT	(3)
#define ECC_FDMADDR		(0x13c)

#define ECC_IDLE_REG(op)	((op) == ECC_ENCODE ? ECC_ENCIDLE : ECC_DECIDLE)
#define ECC_CTL_REG(op)		((op) == ECC_ENCODE ? ECC_ENCCON : ECC_DECCON)

#define MTK_TIMEOUT		(500000)
#define MTK_RESET_TIMEOUT	(1000000)
#define MTK_NAND_MAX_NSELS	(2)

#define ECC_IDLE_MASK		BIT(0)
#define ECC_OP_ENABLE		(1)
#define ECC_OP_DISABLE		(0)

enum mtk_ecc_operation {ECC_ENCODE, ECC_DECODE};

struct mtk_nfc_caps {
	u8 pageformat_spare_shift;
	u8 max_sector;
	u32 sector_size;
	u32 fdm_size;
	u32 fdm_ecc_size;
};

struct mtk_nfc_nand_chip {
	struct list_head node;
	struct nand_chip nand;

	u32 spare_per_sector;
	u32 oobsize_avail;

	int nsels;
	u8 sels[0];
	/* nothing after this field */
};

struct mtk_ecc_config {
	enum mtk_ecc_operation op;
	u32 strength;
	u32 sectors;
	u32 len;
};

struct mtk_ecc_caps {
	u32 err_mask;
	const u8 *ecc_strength;
	u8 num_ecc_strength;
	u8 ecc_mode_shift;
	u32 parity_bits;
};

struct mtk_ecc {
	struct device *dev;
	const struct mtk_ecc_caps *caps;
	void __iomem *regs;

	struct completion done;
	struct mutex lock;
	u32 sectors;
};

struct mtk_nfc {
	struct nand_hw_control controller;
	struct mtk_ecc_config ecc_cfg;
	struct mtk_ecc *ecc;
	struct nand_chip *nand;

	struct device *dev;
	const struct mtk_nfc_caps *caps;
	void __iomem *regs;

	struct list_head chips;

	u8 *buffer;

	u8 **block_buffer;
	u8 *pending_page;
	u8 *pending_oob[2];
};

int mtk_ecc_encode(struct mtk_ecc *, struct mtk_ecc_config *, u8 *, u32);
int mtk_ecc_wait_decode_done(struct mtk_ecc *ecc, u32 sector_index);
int mtk_ecc_enable(struct mtk_ecc *, struct mtk_ecc_config *);
void mtk_ecc_disable(struct mtk_ecc *);
void mtk_ecc_adjust_strength(struct mtk_ecc *ecc, u32 *p);
unsigned int mtk_ecc_get_parity_bits(struct mtk_ecc *ecc);
int mtk_ecc_correct_check(struct mtd_info *mtd, struct mtk_ecc *ecc,
			  u8 *sector_buf, u8 *fdm_buf, u32 sector_index);
int mtk_ecc_init(struct mtk_nfc *nfc, struct mtk_ecc *ecc,
		 struct mtk_ecc_config *config);
struct mtk_ecc *of_mtk_ecc_get(struct device_node *);
void mtk_ecc_release(struct mtk_ecc *);

#endif
