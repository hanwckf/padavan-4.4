/*
 * Copyright (c) 2014 MediaTek Inc.
 * Author: Jie Qiu <jie.qiu@mediatek.com>
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
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>

#include "mtk_cec.h"

#define TR_CONFIG		0x00
#define BYPASS				BIT(28)
#define DEVICE_ADDR1			(0xf << 16)
#define LA1_SHIFT			16
#define DEVICE_ADDR2			(0Xf << 20)
#define LA2_SHIFT			20
#define DEVICE_ADDR3			(0xf << 24)
#define LA3_SHIFT			24
#define CLEAR_CEC_IRQ			BIT(15)
#define TX_G_EN				BIT(13)
#define TX_EN				BIT(12)
#define RX_G_EN				BIT(9)
#define RX_EN				BIT(8)
#define CEC_ID_EN			BIT(1)
#define CEC_EN				BIT(0)

#define CEC_CKGEN		0x04
#define CLK_32K_PDN			BIT(19)
#define CLK_27M_PDN			BIT(18)
#define CLK_SEL_DIV			BIT(17)
#define PDN				BIT(16)
#define DIV_SEL				(0xffff << 0)
#define DIV_SEL_100K		0x82

#define RX_T_START_R		0x08
#define RX_T_START_F		0x0c
#define RX_T_DATA		0x10
#define RX_T_ACK		0x14
#define RX_T_ERROR		0x18
#define TX_T_START		0x1c
#define TX_T_DATA_R		0x20
#define TX_T_DATA_F		0x24
#define TX_ARB			0x28

#define RX_HEADER		0x40
#define RXING_M3_DATA_MASK		(0xf << 24)
#define RXING_SRC			(0xf << 20)
#define RXING_DST			(0xf << 16)
#define RXING_H_EOM			BIT(15)
#define RXING_H_ACK			BIT(14)
#define RXING_D_EOM			BIT(13)
#define RXING_D_ACK			BIT(12)
#define RXING_M3_ID			(0x7 << 8)
#define RXING_M1_DIR			BIT(7)
#define RXING_M1_PAS			BIT(6)
#define RXING_M1_NAS			BIT(5)
#define RXING_M1_DES			BIT(4)
#define RXING_MODE			(0x3 << 0)

#define RX_DATA			0x44
#define RXED_DATA			0xffffffff

#define RX_HD_NEXT		0x48
#define RX_DATA_NEXT		0x4c

#define RX_EVENT		0x54
#define HDMI_PORD			BIT(25)
#define HDMI_HTPLG			BIT(24)
#define DATA_RDY			BIT(23)
#define HEADER_RDY			BIT(22)
#define MODE_RDY			BIT(21)
#define OV				BIT(20)
#define BR_SB_RDY			BIT(18)
#define SB_RDY				BIT(17)
#define BR_RDY				BIT(16)
#define HDMI_PORD_INT_EN		BIT(9)
#define HDMI_HTPLG_INT_EN		BIT(8)
#define I_EN_DATA			BIT(7)
#define I_EN_HEADER			BIT(6)
#define I_EN_MODE			BIT(5)
#define I_EN_OV				BIT(4)
#define I_EN_PULSE			BIT(3)
#define I_EN_BR_SB			BIT(2)
#define I_EN_SB				BIT(1)
#define I_EN_BR				BIT(0)

#define RX_GEN_WD		0x58
#define HDMI_PORD_INT_32K_STATUS	BIT(26)
#define RX_RISC_INT_32K_STATUS		BIT(25)
#define HDMI_HTPLG_INT_32K_STATUS	BIT(24)
#define HDMI_PORD_INT_32K_CLR		BIT(18)
#define RX_INT_32K_CLR			BIT(17)
#define HDMI_HTPLG_INT_32K_CLR		BIT(16)
#define HDMI_PORD_INT_32K_STA_MASK	BIT(10)
#define RX_RISC_INT_32K_STA_MASK	BIT(9)
#define HDMI_HTPLG_INT_32K_STA_MASK	BIT(8)
#define HDMI_PORD_INT_32K_EN		BIT(2)
#define RX_INT_32K_EN			BIT(1)
#define HDMI_HTPLG_INT_32K_EN		BIT(0)

#define NORMAL_INT_CTRL		0x5C
#define HDMI_HTPLG_INT_STA		BIT(0)
#define HDMI_PORD_INT_STA		BIT(1)
#define HDMI_HTPLG_INT_CLR		BIT(16)
#define HDMI_PORD_INT_CLR		BIT(17)
#define HDMI_FULL_INT_CLR		BIT(20)

#define RX_GEN_INTR		0x64
#define RX_STATUS		0x6c
#define CEC_FSM_IDLE			BIT(0)
#define CEC_FSM_INIT			BIT(1)
#define CEC_FSM_EOM			BIT(2)
#define CEC_FSM_RETRYNSMIT		BIT(3)
#define CEC_FSM_FAIL			BIT(4)
#define CEC_FSM_START			BIT(5)
#define CEC_FSM_MODE			BIT(6)
#define CEC_FSM_MODE1_HEADER		BIT(7)
#define CEC_FSM_MODE1_DATA		BIT(8)
#define CEC_FSM_MODE2_HEADER		BIT(9)
#define CEC_FSM_MODE2_CMD		BIT(10)
#define CEC_FSM_MODE3_ID		BIT(11)
#define CEC_FSM_MODE3_HEADER		BIT(12)
#define CEC_FSM_MODE3_DATA		BIT(13)
#define CEC_FSM_GENERAL			BIT(14)

#define TX_HD_NEXT		0x80
#define M3_DATA_MASK			(0xf << 24)
#define SRC				(0xf << 20)
#define SRC_SHIFT			20
#define DST				(0xf << 16)
#define DST_SHIFT			16
#define H_EOM				BIT(15)
#define D_EOM				BIT(13)
#define M3_ID				(0x7 << 8)
#define M1_DIR				BIT(7)
#define M1_PAS				BIT(6)
#define M1_NAS				BIT(5)
#define M1_DES				BIT(4)
#define MODE				(0x3 << 0)
#define MODE_3				(3)

#define TX_DATA_NEXT		0x84
#define WTX_DATA			0xffffffff

#define RX_CAP_90		0x90
#define TX_EVENT		0x94
#define UN				BIT(20)
#define FAIL				BIT(19)
#define LOWB				BIT(18)
#define BS				BIT(17)
#define RB_RDY				BIT(16)
#define I_EN_UN				BIT(4)
#define I_EN_FAIL			BIT(3)
#define I_EN_LOW			BIT(2)
#define I_EN_BS				BIT(1)
#define I_EN_RB				BIT(0)

#define TX_GEN_MASK		0x9c
#define TX_GEN_INTR		0xa4

#define TX_FAIL			0xa8
#define RETX_MAX			BIT(28)
#define DATA				BIT(24)
#define CMD				BIT(20)
#define HEADER				BIT(16)
#define SOURCE				BIT(12)
#define ID				BIT(8)
#define FMODE				BIT(4)
#define FDIR				BIT(0)

#define TX_STATUS		0xac
#define TX_BIT_COUNTER			(0xf << 28)
#define TX_TIMER			(0x7ff << 16)
#define TX_G_PTR			BIT(15)
#define TX_FSM				(0x7fff)
#define TX_FSM_FAIL			BIT(4)

#define TR_TEST			0xbc
#define	PAD_ENABLE			BIT(30)
#define DATA_FINISH_IRQ_ENABLE		BIT(14)
#define TX_COMP_TIM			(0x1f << 0)
#define COMPARE_TIMING			0x19

enum cec_tx_status {
	CEC_TX_START,
	CEC_TX_REMAIN_DATA,
	CEC_TX_DONE,
	CEC_TX_FAIL,
};

enum cec_rx_status {
	CEC_RX_IDLE,
	CEC_RX_WAIT_FRAME_COMPLETE,
	CEC_RX_COMPLETE_NEW_FRAME,
	CEC_RX_GET_NEW_HEADER,
	CEC_RX_LOST_EOM,
};


struct cec_frame {
	struct cec_msg *msg;
	unsigned char offset;
	unsigned char retry_count;
	union {
		enum cec_tx_status tx_status;
		enum cec_rx_status rx_status;
	} status;
};

struct mtk_cec {
	void __iomem *regs;
	struct clk *clk;
	int irq;
	bool hpd;
	void (*hpd_event)(bool hpd, struct device *dev);
	bool hpd_event_handled;
	struct device *hdmi_dev;
	spinlock_t lock;
	struct cec_adapter *adap;
	struct cec_frame transmitting;
	struct cec_frame received;
	struct cec_msg rx_msg;	/*need to be discussed: dynamic alloc or fixed memory?? */
};

enum cec_inner_clock {
	CLK_27M_SRC = 0,
};

enum cec_la_num {
	DEVICE_ADDR_1 = 1,
	DEVICE_ADDR_2 = 2,
	DEVICE_ADDR_3 = 3,
};

struct cec_msg_header_block {
	unsigned char destination:4;
	unsigned char initiator:4;
};

static void mtk_cec_clear_bits(struct mtk_cec *cec, unsigned int offset, unsigned int bits)
{
	void __iomem *reg = cec->regs + offset;
	u32 tmp;

	tmp = readl(reg);
	tmp &= ~bits;
	writel(tmp, reg);
}

static void mtk_cec_set_bits(struct mtk_cec *cec, unsigned int offset, unsigned int bits)
{
	void __iomem *reg = cec->regs + offset;
	u32 tmp;

	tmp = readl(reg);
	tmp |= bits;
	writel(tmp, reg);
}

static void mtk_cec_mask(struct mtk_cec *cec, unsigned int offset,
			 unsigned int val, unsigned int mask)
{
	u32 tmp = readl(cec->regs + offset) & ~mask;

	tmp |= (val & mask);
	writel(tmp, cec->regs + offset);
}

static void mtk_cec_write(struct mtk_cec *cec, unsigned int offset, unsigned int val)
{
	writel(val, cec->regs + offset);
}

int mtk_cec_get_phy_addr(struct device *dev, u8 *edid, int len)
{
	struct mtk_cec *cec = dev_get_drvdata(dev);
	int offset = 0;
	u16 phy_addr = CEC_PHYS_ADDR_INVALID;
	int err;

	phy_addr = cec_get_edid_phys_addr(edid, len, &offset);
	if (offset == 0) {
		dev_err(dev, "no cec phys addr found\n");
		return -EINVAL;
	};

	err = cec_phys_addr_validate(phy_addr, NULL, NULL);
	if (err) {
		dev_err(dev, "cec phys addr invalid\n");
		return err;
	}

	dev_err(dev, "phy_addr is 0x%x\n", phy_addr);
	cec_s_phys_addr(cec->adap, phy_addr, true);

	return 0;
}

void mtk_cec_set_hpd_event(struct device *dev,
			   void (*hpd_event)(bool hpd, struct device *dev),
			   struct device *hdmi_dev)
{
	struct mtk_cec *cec = dev_get_drvdata(dev);
	unsigned long flags;

	spin_lock_irqsave(&cec->lock, flags);
	cec->hdmi_dev = hdmi_dev;
	cec->hpd_event = hpd_event;
	spin_unlock_irqrestore(&cec->lock, flags);

	if (!cec->hpd_event_handled) {
		hpd_event(cec->hpd, hdmi_dev);
		cec->hpd_event_handled = true;
	}
}

bool mtk_cec_hpd_high(struct device *dev)
{
	struct mtk_cec *cec = dev_get_drvdata(dev);
	unsigned int status;

	status = readl(cec->regs + RX_EVENT);

	return (status & (HDMI_PORD | HDMI_HTPLG)) == (HDMI_PORD | HDMI_HTPLG);
}

static void mtk_cec_htplg_irq_init(struct mtk_cec *cec)
{
	mtk_cec_mask(cec, CEC_CKGEN, 0 | CLK_32K_PDN, PDN | CLK_32K_PDN);
	mtk_cec_set_bits(cec, RX_GEN_WD, HDMI_PORD_INT_32K_CLR |
			 RX_INT_32K_CLR | HDMI_HTPLG_INT_32K_CLR);
	mtk_cec_mask(cec, RX_GEN_WD, 0, HDMI_PORD_INT_32K_CLR | RX_INT_32K_CLR |
		     HDMI_HTPLG_INT_32K_CLR | HDMI_PORD_INT_32K_EN |
		     RX_INT_32K_EN | HDMI_HTPLG_INT_32K_EN);
}

static void mtk_cec_htplg_irq_enable(struct mtk_cec *cec)
{
	mtk_cec_set_bits(cec, RX_EVENT, HDMI_PORD_INT_EN | HDMI_HTPLG_INT_EN);
}

static void mtk_cec_htplg_irq_disable(struct mtk_cec *cec)
{
	mtk_cec_clear_bits(cec, RX_EVENT, HDMI_PORD_INT_EN | HDMI_HTPLG_INT_EN);
}

static void mtk_cec_clear_htplg_irq(struct mtk_cec *cec)
{
	mtk_cec_set_bits(cec, TR_CONFIG, CLEAR_CEC_IRQ);
	mtk_cec_set_bits(cec, NORMAL_INT_CTRL, HDMI_HTPLG_INT_CLR |
			 HDMI_PORD_INT_CLR | HDMI_FULL_INT_CLR);
	mtk_cec_set_bits(cec, RX_GEN_WD, HDMI_PORD_INT_32K_CLR |
			 RX_INT_32K_CLR | HDMI_HTPLG_INT_32K_CLR);
	usleep_range(5, 10);
	mtk_cec_clear_bits(cec, NORMAL_INT_CTRL, HDMI_HTPLG_INT_CLR |
			   HDMI_PORD_INT_CLR | HDMI_FULL_INT_CLR);
	mtk_cec_clear_bits(cec, TR_CONFIG, CLEAR_CEC_IRQ);
	mtk_cec_clear_bits(cec, RX_GEN_WD, HDMI_PORD_INT_32K_CLR |
			   RX_INT_32K_CLR | HDMI_HTPLG_INT_32K_CLR);
}

static void mtk_cec_hpd_event(struct mtk_cec *cec, bool hpd)
{
	void (*hpd_event)(bool hpd, struct device *dev);
	struct device *hdmi_dev;
	unsigned long flags;

	spin_lock_irqsave(&cec->lock, flags);
	hpd_event = cec->hpd_event;
	hdmi_dev = cec->hdmi_dev;
	spin_unlock_irqrestore(&cec->lock, flags);

	if (hpd_event) {
		hpd_event(hpd, hdmi_dev);
		cec->hpd_event_handled = true;
	}
}

static void mtk_cec_rx_config(struct mtk_cec *cec, enum cec_inner_clock clk_src)
{
	unsigned int rx_start_rising_conf;
	unsigned int rx_start_falling_conf;
	unsigned int rx_data_timing_conf;
	unsigned int rx_ack_timing_conf;
	unsigned int rx_err_timing_conf;

	if (clk_src == CLK_27M_SRC) {
		rx_start_rising_conf = 0x01980154;
		rx_start_falling_conf = 0x01e801a9;
		rx_data_timing_conf = 0x006e00c8;
		rx_ack_timing_conf = 0x00000096;
		rx_err_timing_conf = 0x01680212;
	} else {
		dev_err(cec->hdmi_dev, "Invalid cec rx clk source\n");
		return;
	}

	mtk_cec_write(cec, RX_T_START_R, rx_start_rising_conf);
	mtk_cec_write(cec, RX_T_START_F, rx_start_falling_conf);
	mtk_cec_write(cec, RX_T_DATA, rx_data_timing_conf);
	mtk_cec_write(cec, RX_T_ACK, rx_ack_timing_conf);
	mtk_cec_write(cec, RX_T_ERROR, rx_err_timing_conf);

	mtk_cec_write(cec, RX_CAP_90, 0x00000000);
	mtk_cec_write(cec, RX_GEN_WD, 0x00000000);
	mtk_cec_write(cec, NORMAL_INT_CTRL, 0x00000000);
	mtk_cec_write(cec, RX_GEN_INTR, 0x00000000);

	mtk_cec_clear_bits(cec, RX_HD_NEXT, RXING_H_ACK | RXING_D_ACK);
	mtk_cec_mask(cec, RX_EVENT, 0x00, 0xff);

	mtk_cec_set_bits(cec, TR_CONFIG, RX_EN);
	mtk_cec_set_bits(cec, RX_EVENT, I_EN_OV | I_EN_BR | I_EN_HEADER);
}

static void mtk_cec_tx_config(struct mtk_cec *cec, enum cec_inner_clock clk_src)
{
	unsigned int tx_start_timing_conf;
	unsigned int tx_data_rising_conf;
	unsigned int tx_data_falling_conf;
	unsigned int tx_arb_timing_conf;

	if (clk_src == CLK_27M_SRC) {
		tx_start_timing_conf = 0x01c20172;
		tx_data_rising_conf = 0x003c0096;
		tx_data_falling_conf = 0x00f000f0;
		tx_arb_timing_conf = 0x00000596;
	} else {
		dev_err(cec->hdmi_dev, "Invalid cec tx clk source\n");
		return;
	}

	mtk_cec_write(cec, TX_T_START, tx_start_timing_conf);
	mtk_cec_write(cec, TX_T_DATA_R, tx_data_rising_conf);
	mtk_cec_write(cec, TX_T_DATA_F, tx_data_falling_conf);
	mtk_cec_write(cec, TX_ARB, tx_arb_timing_conf);

	/* turn off interrupt of general mode */
	mtk_cec_write(cec, TX_GEN_INTR, 0x00000000);
	mtk_cec_mask(cec, TX_HD_NEXT, 0, M3_ID);
	mtk_cec_write(cec, TX_GEN_MASK, 0x00000000);
	mtk_cec_clear_bits(cec, TX_HD_NEXT, M1_DIR | M1_PAS | M1_NAS | M1_DES);

	mtk_cec_mask(cec, TX_HD_NEXT, MODE_3, MODE);
	mtk_cec_mask(cec, TX_EVENT, 0x00, 0xff);
	mtk_cec_set_bits(cec, TR_CONFIG, TX_EN);
	mtk_cec_clear_bits(cec, TX_EVENT, I_EN_UN | I_EN_LOW | I_EN_FAIL | I_EN_BS | I_EN_RB);
}

static void mtk_cec_tx_rx_config(struct mtk_cec *cec)
{
	mtk_cec_mask(cec, CEC_CKGEN, CLK_32K_PDN | CLK_SEL_DIV | DIV_SEL_100K,
						CLK_32K_PDN | CLK_SEL_DIV | DIV_SEL);
	mtk_cec_clear_bits(cec, CEC_CKGEN, CLK_27M_PDN | PDN);
	mtk_cec_set_bits(cec, TR_CONFIG, CEC_EN);

	mtk_cec_rx_config(cec, CLK_27M_SRC);
	mtk_cec_tx_config(cec, CLK_27M_SRC);
}

static void mtk_cec_set_logic_addr(struct mtk_cec *cec, u8 la, enum cec_la_num num)
{
	if (num == DEVICE_ADDR_1)
		mtk_cec_mask(cec, TR_CONFIG, la << LA1_SHIFT, DEVICE_ADDR1);
	else if (num == DEVICE_ADDR_2)
		mtk_cec_mask(cec, TR_CONFIG, la << LA2_SHIFT, DEVICE_ADDR2);
	else if (num == DEVICE_ADDR_3)
		mtk_cec_mask(cec, TR_CONFIG, la << LA3_SHIFT, DEVICE_ADDR3);
	else
		dev_err(cec->hdmi_dev, "Cannot support more than 3 logic address");
}

static void mtk_cec_logic_addr_config(struct cec_adapter *adap)
{
	__u8 unreg_la_addr = 0xff;
	struct mtk_cec *cec = adap->priv;
	struct cec_log_addrs *cec_la = &(adap->log_addrs);

	memset((void *)cec_la, 0, sizeof(struct cec_log_addrs));
	cec_la->num_log_addrs = 0;
	cec_la->log_addr[0] = CEC_LOG_ADDR_UNREGISTERED;
	cec_la->cec_version = CEC_OP_CEC_VERSION_1_4;
	cec_la->log_addr_type[0] = CEC_LOG_ADDR_TYPE_UNREGISTERED;
	cec_la->primary_device_type[0] = CEC_LOG_ADDR_TYPE_UNREGISTERED;
	cec_la->log_addr_mask = 0;

	mtk_cec_set_logic_addr(cec, unreg_la_addr, DEVICE_ADDR_1);
	mtk_cec_set_logic_addr(cec, unreg_la_addr, DEVICE_ADDR_2);
	mtk_cec_set_logic_addr(cec, unreg_la_addr, DEVICE_ADDR_3);
}

static int mtk_cec_adap_enable(struct cec_adapter *adap, bool enable)
{
	struct mtk_cec *cec = adap->priv;

	mtk_cec_logic_addr_config(adap);
	mtk_cec_tx_rx_config(cec);

	return 0;
}

static int mtk_cec_adap_log_addr(struct cec_adapter *adap, u8 logical_addr)
{
	struct mtk_cec *cec = adap->priv;

	/* to do :mtk cec can accept 3 logic address, now just accept one */
	if (logical_addr != CEC_LOG_ADDR_INVALID)
		mtk_cec_set_logic_addr(cec, logical_addr, DEVICE_ADDR_1);

	return 0;
}

static int mtk_cec_tx_hw_status(struct mtk_cec *cec)
{
	return readl(cec->regs + TX_STATUS);
}

static bool mtk_cec_tx_retry_max(struct mtk_cec *cec)
{
	u16 tx_fail_record = readl(cec->regs + TX_FAIL);

	return ((tx_fail_record & RETX_MAX) ? true : false);
}

static void mtk_cec_hw_reset(struct mtk_cec *cec)
{
	mtk_cec_clear_bits(cec, TR_CONFIG, TX_EN);
	mtk_cec_set_bits(cec, TR_CONFIG, TX_EN);
}

static void mtk_cec_set_msg_header(struct mtk_cec *cec, struct cec_msg *msg)
{
	struct cec_msg_header_block header = { };

	header.destination = (msg->msg[0]) & 0x0f;
	header.initiator = ((msg->msg[0]) & 0xf0) >> 4;
	mtk_cec_mask(cec, TX_HD_NEXT, header.initiator << SRC_SHIFT, SRC);
	mtk_cec_mask(cec, TX_HD_NEXT, header.destination << DST_SHIFT, DST);
}

static void mtk_cec_set_msg_len(struct mtk_cec *cec, unsigned int len)
{
	mtk_cec_mask(cec, TX_HD_NEXT, len << 24, M3_DATA_MASK);
}

static void mtk_cec_mark_header_eom(struct mtk_cec *cec, bool flag)
{
	mtk_cec_mask(cec, TX_HD_NEXT, flag ? H_EOM : 0, H_EOM);
}

static void mtk_cec_mark_data_eom(struct mtk_cec *cec, bool flag)
{
	mtk_cec_mask(cec, TX_HD_NEXT, flag ? D_EOM : 0, D_EOM);
}

static void mtk_cec_set_msg_data(struct mtk_cec *cec, unsigned int data)
{
	mtk_cec_mask(cec, TX_DATA_NEXT, data, WTX_DATA);
}

static void mtk_cec_trigger_tx_hw(struct mtk_cec *cec)
{
	mtk_cec_set_bits(cec, TX_EVENT, RB_RDY);
}


static void mtk_cec_print_cec_frame(struct cec_frame *frame)
{
	struct cec_msg *msg = frame->msg;
	unsigned char header = msg->msg[0];

	pr_err("cec message data offset is %d\n", frame->offset);
	pr_err("cec message initiator is %d\n", (header & 0xf0) >> 4);
	pr_err("cec message follower is %d\n", header & 0x0f);
	pr_err("cec message length is %d\n", msg->len);
	pr_err("cec message opcode is 0x%x\n", msg->msg[1]);
}

static void mtk_cec_mark_header_data_eom(struct mtk_cec *cec, unsigned char size)
{
	unsigned int msg_len;

	bool header_eom = false;
	bool data_eom = true;

	if (size > 4) {
		msg_len = 0xf;
		data_eom = false;
	} else if (size == 4)
		msg_len = 0xf;
	else if (size == 3)
		msg_len = 0x7;
	else if (size == 2)
		msg_len = 0x3;
	else if (size == 1)
		msg_len = 0x1;
	else if (size == 0) {
		/* header-only */
		msg_len = 0;
		header_eom = true;
		data_eom = false;
	};

	/* fill eom bits, set length */
	mtk_cec_set_msg_len(cec, msg_len);
	mtk_cec_mark_header_eom(cec, header_eom);
	mtk_cec_mark_data_eom(cec, data_eom);
}

static unsigned int mtk_cec_parse_msg_data(struct mtk_cec *cec, unsigned char msg_data_size)
{
	unsigned int cec_data = 0;
	unsigned int i;
	unsigned int j;
	bool eom;
	unsigned char size = msg_data_size;
	unsigned char index = cec->transmitting.offset;
	struct cec_msg *msg = cec->transmitting.msg;

	for (i = 0, j = size; i < 4; i++) {
		cec_data >>= 8;
		cec_data |= msg->msg[index] << 24;
		if (i < j) {
			index++;
			size--;
		}
	}
	cec->transmitting.offset = index;

	if (size == 0) {
		eom = false;
		mtk_cec_clear_bits(cec, TX_EVENT, I_EN_RB | I_EN_LOW | I_EN_UN);
	} else {
		eom = true;
		mtk_cec_set_bits(cec, TX_EVENT, I_EN_RB | I_EN_LOW | I_EN_UN);
	}

	mtk_cec_clear_bits(cec, TX_EVENT, BS | FAIL);
	mtk_cec_set_bits(cec, TX_EVENT, I_EN_BS | I_EN_FAIL);

	return cec_data;
}

static int mtk_cec_send_msg(struct mtk_cec *cec)
{
	unsigned int cec_data;
	unsigned char msg_data_size;
	unsigned char size;
	struct cec_frame *frame = &cec->transmitting;

	size = frame->msg->len;
	msg_data_size = size - frame->offset;

	mtk_cec_print_cec_frame(frame);

	if (frame->status.tx_status == CEC_TX_START) {
		if (!(mtk_cec_tx_hw_status(cec) & CEC_FSM_IDLE)
		    || mtk_cec_tx_retry_max(cec))
			mtk_cec_hw_reset(cec);

		mtk_cec_set_msg_header(cec, frame->msg);
	}

	/*Mark header/data eom according to the msg data size */
	mtk_cec_mark_header_data_eom(cec, msg_data_size);
	cec_data = mtk_cec_parse_msg_data(cec, msg_data_size);
	mtk_cec_set_msg_data(cec, cec_data);

	mtk_cec_trigger_tx_hw(cec);

	return 0;
}

static void mtk_cec_msg_init(struct mtk_cec *cec, struct cec_msg *msg)
{
	cec->transmitting.msg = msg;
	cec->transmitting.offset = 1;
	cec->transmitting.retry_count = 0;
	cec->transmitting.status.tx_status = CEC_TX_START;
}

static int mtk_cec_adap_transmit(struct cec_adapter *adap, u8 attempts,
				 u32 signal_free_time, struct cec_msg *msg)
{
	struct mtk_cec *cec = adap->priv;

	mtk_cec_msg_init(cec, msg);
	mtk_cec_send_msg(cec);

	return 0;
}

static int mtk_cec_received(struct cec_adapter *adap, struct cec_msg *msg)
{
	return -ENOMSG;
}

static int mtk_cec_msg_destination_validate(struct mtk_cec *cec)
{
	unsigned char msg_dst;
	struct cec_adapter *adap = cec->adap;

	msg_dst = (readl(cec->regs + RX_HD_NEXT) & RXING_DST) >> 16;
	if (((msg_dst & 0xf) == 0xf)
	    || (adap->log_addrs.log_addr_mask & (1 << msg_dst)))
		return true;
	return false;
}

static void mtk_cec_set_rx_status(struct cec_frame *frame, enum cec_rx_status status)
{
	frame->status.rx_status = status;
}

static unsigned char mtk_cec_get_rx_msg_len(struct mtk_cec *cec)
{
	__u8 reg_val;
	__u8 msg_data_size;

	reg_val = (readl(cec->regs + RX_HEADER) & RXING_M3_DATA_MASK) >> 24;
	if (reg_val == 0x3)
		msg_data_size = 2;
	else if (reg_val == 0x7)
		msg_data_size = 3;
	else if (reg_val == 0xf)
		msg_data_size = 4;
	else if (reg_val == 0x1)
		msg_data_size = 1;
	else
		msg_data_size = 0;

	return msg_data_size;
}

static void mtk_cec_trigger_rx_hw(struct mtk_cec *cec)
{
	mtk_cec_clear_bits(cec, RX_EVENT, BR_RDY);
}

static void mtk_cec_get_rx_header(struct mtk_cec *cec, struct cec_frame *rx_frame)
{
	__u8 initiator;
	__u8 dest;
	struct cec_msg *rx_msg = rx_frame->msg;

	initiator = (readl(cec->regs + RX_HEADER) & RXING_SRC) >> 20;
	dest = (readl(cec->regs + RX_HEADER) & RXING_DST) >> 16;
	*(rx_msg->msg + 0) = (initiator << 4) | dest;

	rx_frame->offset = 1;
}

static void mtk_cec_get_rx_data(struct mtk_cec *cec, struct cec_frame *rx_frame)
{
	unsigned int msg_data;
	unsigned char msg_data_size;
	bool data_eom = false;
	unsigned int i;
	struct cec_msg *rx_msg = rx_frame->msg;

	msg_data_size = mtk_cec_get_rx_msg_len(cec);
	if (msg_data_size == 0)
		return;

	msg_data = readl(cec->regs + RX_DATA);
	data_eom = readl(cec->regs + RX_HEADER) & RXING_D_EOM;
	mtk_cec_trigger_rx_hw(cec);

	for (i = 0; i < msg_data_size; i++)
		rx_msg->msg[rx_frame->offset++] = (msg_data >> (i * 8)) & 0xff;

	if (data_eom) {
		rx_frame->msg->len = rx_frame->offset;
		mtk_cec_set_rx_status(rx_frame, CEC_RX_COMPLETE_NEW_FRAME);
		/* mtk_cec_print_cec_frame(rx_frame); */
	};
}

static void mtk_cec_receive_msg(struct mtk_cec *cec)
{
	unsigned char msg_len;
	struct cec_frame *rx_msg = &cec->received;

	msg_len = mtk_cec_get_rx_msg_len(cec);
	if (msg_len == 0) {	/*Polling msg only */
		mtk_cec_trigger_rx_hw(cec);
		mtk_cec_set_rx_status(rx_msg, CEC_RX_IDLE);
		return;
	};

	switch (rx_msg->status.rx_status) {
	case CEC_RX_GET_NEW_HEADER:
		mtk_cec_get_rx_header(cec, rx_msg);
		mtk_cec_set_rx_status(rx_msg, CEC_RX_WAIT_FRAME_COMPLETE);
		mtk_cec_get_rx_data(cec, rx_msg);
		break;

	case CEC_RX_WAIT_FRAME_COMPLETE:
		mtk_cec_get_rx_data(cec, rx_msg);
		break;

	case CEC_RX_LOST_EOM:
	default:
		mtk_cec_trigger_rx_hw(cec);
		mtk_cec_set_rx_status(rx_msg, CEC_RX_IDLE);
		break;
	}

	if (rx_msg->status.rx_status == CEC_RX_COMPLETE_NEW_FRAME) {
		cec_received_msg(cec->adap, rx_msg->msg);
		mtk_cec_set_rx_status(rx_msg, CEC_RX_IDLE);
	}
}

static void mtk_cec_rx_event_handler(struct mtk_cec *cec, unsigned int rx_event)
{
	struct cec_frame *received_msg = &cec->received;

	received_msg->msg = &cec->rx_msg;

	if (rx_event & HEADER_RDY) {
		if (mtk_cec_msg_destination_validate(cec)) {
			if (received_msg->status.rx_status != CEC_RX_IDLE)
				mtk_cec_set_rx_status(received_msg, CEC_RX_LOST_EOM);
			else {
				mtk_cec_set_rx_status(received_msg, CEC_RX_GET_NEW_HEADER);
				memset(received_msg->msg, 0, sizeof(struct cec_msg));
			}
		}
	}

	if (rx_event & BR_RDY)
		mtk_cec_receive_msg(cec);
}

static void mtk_cec_tx_event_handler(struct mtk_cec *cec, unsigned int tx_event)
{
	switch (tx_event & ((0x1f << 16) | I_EN_RB)) {
	case BS:
		cec->transmitting.status.tx_status = CEC_TX_DONE;
		cec_transmit_done(cec->adap, CEC_TX_STATUS_OK, 0, 0, 0, 0);
		mtk_cec_clear_bits(cec,
				TX_EVENT, I_EN_FAIL | I_EN_RB | I_EN_LOW | I_EN_UN | I_EN_BS);
		break;

	case I_EN_RB:
		cec->transmitting.status.tx_status = CEC_TX_REMAIN_DATA;
		mtk_cec_send_msg(cec);
		break;

	case LOWB:
	case FAIL:
	case UN:
		dev_err(cec->hdmi_dev, "cec transmit msg fail\n");
		cec->transmitting.status.tx_status = CEC_TX_FAIL;
		mtk_cec_clear_bits(cec, TX_EVENT, UN | LOWB | FAIL);
		cec_transmit_done(cec->adap, CEC_TX_STATUS_NACK, 0, 0, 0, 1);
		break;

	case RB_RDY:
		dev_err(cec->hdmi_dev, "RB_RDY\n");
		break;
	default:
		break;
	}
}

static void mtk_cec_tx_rx_event_handler(struct mtk_cec *cec)
{
	unsigned int rx_event;
	unsigned int tx_event;

	rx_event = readl(cec->regs + RX_EVENT);
	tx_event = readl(cec->regs + TX_EVENT);

	mtk_cec_rx_event_handler(cec, rx_event);
	mtk_cec_tx_event_handler(cec, tx_event);
}

static irqreturn_t mtk_cec_htplg_isr_thread(int irq, void *arg)
{
	struct device *dev = arg;
	struct mtk_cec *cec = dev_get_drvdata(dev);
	bool hpd;

	mtk_cec_clear_htplg_irq(cec);
	mtk_cec_tx_rx_event_handler(cec);
	hpd = mtk_cec_hpd_high(dev);

	if (cec->hpd != hpd) {
		dev_dbg(dev, "hotplug event! cur hpd = %d, hpd = %d\n", cec->hpd, hpd);
		cec->hpd = hpd;
		mtk_cec_hpd_event(cec, hpd);
	}

	return IRQ_HANDLED;
}

static const struct cec_adap_ops mtk_hdmi_cec_adap_ops = {
	.adap_enable = mtk_cec_adap_enable,
	.adap_log_addr = mtk_cec_adap_log_addr,
	.adap_transmit = mtk_cec_adap_transmit,
	.received = mtk_cec_received,
};

static int mtk_cec_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_cec *cec;
	struct resource *res;
	int ret;

	cec = devm_kzalloc(dev, sizeof(*cec), GFP_KERNEL);
	if (!cec)
		return -ENOMEM;

	platform_set_drvdata(pdev, cec);
	spin_lock_init(&cec->lock);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	cec->regs = devm_ioremap_resource(dev, res);
	if (IS_ERR(cec->regs)) {
		ret = PTR_ERR(cec->regs);
		dev_err(dev, "Failed to ioremap cec: %d\n", ret);
		return ret;
	}

	cec->clk = devm_clk_get(dev, NULL);
	if (IS_ERR(cec->clk)) {
		ret = PTR_ERR(cec->clk);
		dev_err(dev, "Failed to get cec clock: %d\n", ret);
		return ret;
	}

	cec->irq = platform_get_irq(pdev, 0);
	if (cec->irq < 0) {
		dev_err(dev, "Failed to get cec irq: %d\n", cec->irq);
		return cec->irq;
	}

	ret = devm_request_threaded_irq(dev, cec->irq, NULL,
					mtk_cec_htplg_isr_thread,
					IRQF_SHARED | IRQF_TRIGGER_LOW |
					IRQF_ONESHOT, "hdmi hpd", dev);
	if (ret) {
		dev_err(dev, "Failed to register cec irq: %d\n", ret);
		return ret;
	}

	ret = clk_prepare_enable(cec->clk);
	if (ret) {
		dev_err(dev, "Failed to enable cec clock: %d\n", ret);
		return ret;
	}

	cec->adap = cec_allocate_adapter(&mtk_hdmi_cec_adap_ops,
					cec, "mtk-hdmi-cec",
					CEC_CAP_TRANSMIT | CEC_CAP_PASSTHROUGH |
					CEC_CAP_LOG_ADDRS, 1);

	ret = PTR_ERR_OR_ZERO(cec->adap);
	if (ret < 0) {
		dev_err(dev, "Failed to allocate cec adapter %d\n", ret);
		return ret;
	}

	ret = cec_register_adapter(cec->adap, dev);
	if (ret) {
		dev_err(dev, "Fail to register cec adapter\n");
		cec_delete_adapter(cec->adap);
		return ret;
	}

	mtk_cec_htplg_irq_init(cec);
	mtk_cec_htplg_irq_enable(cec);

	return 0;
}

static int mtk_cec_remove(struct platform_device *pdev)
{
	struct mtk_cec *cec = platform_get_drvdata(pdev);

	cec_unregister_adapter(cec->adap);

	mtk_cec_htplg_irq_disable(cec);
	clk_disable_unprepare(cec->clk);
	return 0;
}

static const struct of_device_id mtk_cec_of_ids[] = {
	{ .compatible = "mediatek,mt8173-cec", },
	{}
};

struct platform_driver mtk_cec_driver = {
	.probe = mtk_cec_probe,
	.remove = mtk_cec_remove,
	.driver = {
		.name = "mediatek-cec",
		.of_match_table = mtk_cec_of_ids,
	},
};
