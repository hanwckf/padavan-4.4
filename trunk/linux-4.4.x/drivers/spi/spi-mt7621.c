/*
 * Copyright (C) 2019 MediaTek Inc.
 *
 * SPI controller driver for the MediaTek MT7621
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/bitops.h>
#include <linux/spi/spi.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/reset.h>

#define DRIVER_NAME			"spi-mt7621"

#define SPI_TRANS_REG			0x00
#define SPI_MASTER_BUSY			BIT(16)
#define SPI_MASTER_START		BIT(8)

#define SPI_OP_ADDR_REG			0x04

#define SPI_DIDO_REG(x)			(0x08 + (x) * 4)

#define SPI_MASTER_REG			0x28
#define RS_SLAVE_SEL_M			0x7
#define RS_SLAVE_SEL_S			29
#define RS_CLK_SEL_M			0xfff
#define RS_CLK_SEL_S			16
#define CPHA				BIT(5)
#define CPOL				BIT(4)
#define LSB_FIRST			BIT(3)
#define MORE_BUF_MODE			BIT(2)

#define SPI_MOREBUF_REG			0x2c
#define CMD_BIT_CNT_M			0x3f
#define CMD_BIT_CNT_S			24
#define MISO_BIT_CNT_M			0x1ff
#define MISO_BIT_CNT_S			12
#define MOSI_BIT_CNT_M			0x1ff
#define MOSI_BIT_CNT_S			0

#define SPI_CS_POL			0x38

#define MT7621_RX_FIFO_LEN		32
#define MT7621_TX_FIFO_LEN		36

struct mt7621_spi {
	struct spi_master	*master;
	void __iomem		*base;
	u32			hclk_freq;
	struct clk		*hclk;
};

static inline struct mt7621_spi *spidev_to_mt7621_spi(struct spi_device *spi)
{
	return spi_master_get_devdata(spi->master);
}

static u32 mt7621_spi_get_clk_div(u32 hclk_freq, u32 freq)
{
	u32 div;

	div = (hclk_freq + freq - 1) / freq;

	if (div > (RS_CLK_SEL_M + 2))
		return RS_CLK_SEL_M;

	if (div < 2)
		return 2;

	return div - 2;
}

static void mt7621_spi_prepare(struct spi_device *spi)
{
	struct mt7621_spi *ms = spidev_to_mt7621_spi(spi);
	u32 master, clk_div, cs_bit;
	int cs = spi->chip_select;

	clk_div = mt7621_spi_get_clk_div(ms->hclk_freq, spi->max_speed_hz);

	master = RS_SLAVE_SEL_M << RS_SLAVE_SEL_S;
	master |= clk_div << RS_CLK_SEL_S;
	master |= MORE_BUF_MODE;

	if (spi->mode & SPI_LSB_FIRST)
		master |= LSB_FIRST;

	if (spi->mode & SPI_CPOL)
		master |= CPOL;

	if (spi->mode & SPI_CPHA)
		master |= CPHA;

	if (spi->mode & SPI_CS_HIGH)
		cs_bit = BIT(cs);
	else
		cs_bit = 0;

	writel(master, ms->base + SPI_MASTER_REG);
	writel(cs_bit, ms->base + SPI_CS_POL);
}

static void mt7621_spi_set_cs(struct spi_device *spi, int enable)
{
	struct mt7621_spi *ms = spidev_to_mt7621_spi(spi);
	int cs = spi->chip_select;

	if (spi->mode & SPI_CS_HIGH)
		enable = !enable;

	if (enable)
		writel(BIT(cs), ms->base + SPI_CS_POL);
	else
		writel(0, ms->base + SPI_CS_POL);
}

static void mt7621_spi_set_speed(struct mt7621_spi *ms, u32 speed)
{
	u32 master, clk_div;

	clk_div = mt7621_spi_get_clk_div(ms->hclk_freq, speed);

	master = readl(ms->base + SPI_MASTER_REG);
	master &= ~(RS_CLK_SEL_M << RS_CLK_SEL_S);
	master |= clk_div << RS_CLK_SEL_S;
	writel(master, ms->base + SPI_MASTER_REG);
}

static int mt7621_spi_busy_wait(struct mt7621_spi *ms)
{
	u32 val;
	int ret;

	ret = readl_poll_timeout(ms->base + SPI_TRANS_REG, val,
				 !(val & SPI_MASTER_BUSY), 0, 100000);

	return ret;
}

static int mt7621_spi_read(struct mt7621_spi *ms, u8 *buf, size_t len)
{
	size_t rx_len;
	int i, ret;
	u32 val = 0;

	while (len) {
		rx_len = min_t(size_t, len, MT7621_RX_FIFO_LEN);

		writel((rx_len * 8) << MISO_BIT_CNT_S,
		       ms->base + SPI_MOREBUF_REG);
		writel(SPI_MASTER_START, ms->base + SPI_TRANS_REG);

		ret = mt7621_spi_busy_wait(ms);
		if (ret)
			return ret;

		for (i = 0; i < rx_len; i++) {
			if ((i % 4) == 0)
				val = readl(ms->base + SPI_DIDO_REG(i / 4));
			*buf++ = val & 0xff;
			val >>= 8;
		}

		len -= rx_len;
	}

	return ret;
}

static int mt7621_spi_write(struct mt7621_spi *ms, const u8 *buf, size_t len)
{
	size_t tx_len, opcode_len, dido_len;
	int i, ret;
	u32 val;

	while (len) {
		tx_len = min_t(size_t, len, MT7621_TX_FIFO_LEN);

		opcode_len = min_t(size_t, tx_len, 4);
		dido_len = tx_len - opcode_len;

		val = 0;
		for (i = 0; i < opcode_len; i++) {
			val <<= 8;
			val |= *buf++;
		}

		writel(val, ms->base + SPI_OP_ADDR_REG);

		val = 0;
		for (i = 0; i < dido_len; i++) {
			val |= (*buf++) << ((i % 4) * 8);

			if ((i % 4 == 3) || (i == dido_len - 1)) {
				writel(val, ms->base + SPI_DIDO_REG(i / 4));
				val = 0;
			}
		}

		writel(((opcode_len * 8) << CMD_BIT_CNT_S) |
		       ((dido_len * 8) << MOSI_BIT_CNT_S),
		       ms->base + SPI_MOREBUF_REG);
		writel(SPI_MASTER_START, ms->base + SPI_TRANS_REG);

		ret = mt7621_spi_busy_wait(ms);
		if (ret)
			return ret;

		len -= tx_len;
	}

	return 0;
}

static int mt7621_spi_transfer_one_message(struct spi_master *master,
					   struct spi_message *m)
{
	struct mt7621_spi *ms = spi_master_get_devdata(master);
	struct spi_device *spi = m->spi;
	struct spi_transfer *t = NULL;
	int status;

	status = mt7621_spi_busy_wait(ms);
	if (status)
		goto out;

	mt7621_spi_prepare(spi);

	mt7621_spi_set_cs(spi, 1);

	list_for_each_entry(t, &m->transfers, transfer_list) {
		if (t->speed_hz)
			mt7621_spi_set_speed(ms, t->speed_hz);

		if (t->tx_buf)
			status = mt7621_spi_write(ms, t->tx_buf, t->len);
		else if (t->rx_buf)
			status = mt7621_spi_read(ms, t->rx_buf, t->len);

		if (status)
			goto out;

		m->actual_length += t->len;
	}

out:
	mt7621_spi_set_cs(spi, 0);

	m->status = status;
	spi_finalize_current_message(master);

	return 0;
}

static int mt7621_spi_setup(struct spi_device *spi)
{
	struct mt7621_spi *ms = spidev_to_mt7621_spi(spi);

	if ((spi->max_speed_hz == 0) ||
		(spi->max_speed_hz > (ms->hclk_freq / 2)))
		spi->max_speed_hz = (ms->hclk_freq / 2);

	if (spi->max_speed_hz < (ms->hclk_freq / (RS_CLK_SEL_M + 2))) {
		dev_info(&spi->dev, "setup: requested speed is too low %d Hz\n",
			spi->max_speed_hz);
		return -EINVAL;
	}

	dev_info(&spi->dev, "setup: max speed is %d Hz\n",
		spi->max_speed_hz);

	return 0;
}

static int mt7621_spi_probe(struct platform_device *pdev)
{
	struct spi_master *master;
	struct mt7621_spi *ms;
	void __iomem *base;
	struct resource *r;
	u32 hclk_freq;
	int status = 0;
	struct clk *hclk;

	r = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	base = devm_ioremap_resource(&pdev->dev, r);
	if (IS_ERR(base))
		return PTR_ERR(base);

	hclk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(hclk)) {
		dev_info(&pdev->dev, "unable to get hclk, err = %d\n", status);
		return PTR_ERR(hclk);
	}

	status = clk_prepare_enable(hclk);
	if (status)
		return status;

	hclk_freq = clk_get_rate(hclk);

	master = spi_alloc_master(&pdev->dev, sizeof(*ms));
	if (master == NULL)
		return -ENOMEM;

	master->mode_bits = SPI_CPOL | SPI_CPHA | SPI_LSB_FIRST | SPI_CS_HIGH;
	master->flags = SPI_MASTER_HALF_DUPLEX;

	master->setup = mt7621_spi_setup;
	master->transfer_one_message = mt7621_spi_transfer_one_message;
	master->bits_per_word_mask = SPI_BPW_MASK(8);
	master->dev.of_node = pdev->dev.of_node;
	master->num_chipselect = 2;

	master->min_speed_hz = hclk_freq / (RS_CLK_SEL_M + 2);

	dev_set_drvdata(&pdev->dev, master);

	ms = spi_master_get_devdata(master);
	ms->base = base;
	ms->hclk = hclk;
	ms->master = master;
	ms->hclk_freq = hclk_freq;

	device_reset(&pdev->dev);

	return spi_register_master(master);
}

static int mt7621_spi_remove(struct platform_device *pdev)
{
	struct spi_master *master;
	struct mt7621_spi *ms;

	master = dev_get_drvdata(&pdev->dev);
	ms = spi_master_get_devdata(master);

	clk_disable(ms->hclk);
	spi_unregister_master(master);

	return 0;
}

static const struct of_device_id mt7621_spi_match[] = {
	{ .compatible = "mediatek,mt7621-spi" },
	{ },
};
MODULE_DEVICE_TABLE(of, mt7621_spi_match);

static struct platform_driver mt7621_spi_driver = {
	.driver = {
		.name = DRIVER_NAME,
		.owner = THIS_MODULE,
		.of_match_table = mt7621_spi_match,
	},
	.probe = mt7621_spi_probe,
	.remove = mt7621_spi_remove,
};

module_platform_driver(mt7621_spi_driver);

MODULE_DESCRIPTION("MediaTek MT7621 SPI driver");
MODULE_AUTHOR("Weijie Gao <hackpascal@gmail.com>");
MODULE_ALIAS("platform:" DRIVER_NAME);
MODULE_LICENSE("GPL");
