/*
 * MediaTek leopard Pinctrl Driver
 *
 * Copyright (C) 2018 Zhiyong Tao <zhiyong.tao@mediatek.com>
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

#include <linux/gpio.h>
#include <linux/gpio/driver.h>
#include <linux/io.h>
#include <linux/init.h>
#include <linux/mfd/syscon.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinmux.h>
#include <linux/pinctrl/pinconf.h>
#include <linux/pinctrl/pinconf-generic.h>
#include <linux/regmap.h>

#include "../core.h"
#include "../pinconf.h"
#include "../pinmux.h"
#include "mtk-eint.h"

#define PINCTRL_PINCTRL_DEV		KBUILD_MODNAME
#define MTK_RANGE(_a)		{ .range = (_a), .nranges = ARRAY_SIZE(_a), }
#define PINCTRL_PIN_GROUP(name, id)			\
	{						\
		name,					\
		id##_pins,				\
		ARRAY_SIZE(id##_pins),			\
		id##_funcs,				\
	}

#define MT_GPIO_TO_EINT(gpio, eint)	\
	{				\
		.gpionum = gpio,	\
		.eintnum = eint,	\
	}

#define MTK_GPIO_MODE	0
#define MTK_INPUT	0
#define MTK_OUTPUT	1
#define MTK_DISABLE	0
#define MTK_ENABLE	1

/* Custom pinconf parameters */
#define MTK_PIN_CONFIG_TDSEL	(PIN_CONFIG_END + 1)
#define MTK_PIN_CONFIG_RDSEL	(PIN_CONFIG_END + 2)

/* List these attributes which could be modified for the pin */
enum {
	PINCTRL_PIN_REG_MODE,
	PINCTRL_PIN_REG_DIR,
	PINCTRL_PIN_REG_DI,
	PINCTRL_PIN_REG_DO,
	PINCTRL_PIN_REG_SR,
	PINCTRL_PIN_REG_IES,
	PINCTRL_PIN_REG_SMT,
	PINCTRL_PIN_REG_PULLEN,
	PINCTRL_PIN_REG_PULLSEL,
	PINCTRL_PIN_REG_DRV,
	PINCTRL_PIN_REG_TDSEL,
	PINCTRL_PIN_REG_RDSEL,
	PINCTRL_PIN_REG_MAX,
};

struct mtk_gpio_to_eint {
	unsigned int gpionum;
	unsigned int eintnum;
};

/* struct mtk_pin_field - the structure that holds the information of the field
 *			  used to describe the attribute for the pin
 * @offset:		the register offset relative to the base address
 * @mask:		the mask used to filter out the field from the register
 * @bitpos:		the start bit relative to the register
 * @next:		the indication that the field would be extended to the
			next register
 */
struct mtk_pin_field {
	u32 offset;
	u32 mask;
	u8  bitpos;
	u8  next;
};

/* struct mtk_pin_field_calc - the structure that holds the range providing
 *			       the guide used to look up the relevant field
 * @s_pin:		the start pin within the range
 * @e_pin:		the end pin within the range
 * @s_addr:		the start address for the range
 * @x_addrs:		the address distance between two consecutive registers
 *			within the range
 * @s_bit:		the start bit for the first register within the range
 * @x_bits:		the bit distance between two consecutive pins within
 *			the range
 */
struct mtk_pin_field_calc {
	u16 s_pin;
	u16 e_pin;
	u32 s_addr;
	u8  x_addrs;
	u8  s_bit;
	u8  x_bits;
};

/* struct mtk_pin_reg_calc - the structure that holds all ranges used to
 *			     determine which register the pin would make use of
 *			     for certain pin attribute.
 * @range:		     the start address for the range
 * @nranges:		     the number of items in the range
 */
struct mtk_pin_reg_calc {
	const struct mtk_pin_field_calc *range;
	unsigned int nranges;
};

/* struct mtk_pin_soc - the structure that holds SoC-specific data */
struct mtk_pin_soc {
	const struct mtk_pin_reg_calc	*reg_cal;
	const struct pinctrl_pin_desc	*pins;
	unsigned int			npins;
	const struct group_desc		*grps;
	unsigned int			ngrps;
	const struct function_desc	*funcs;
	unsigned int			nfuncs;
	const struct mtk_eint_regs	*eint_regs;
	const struct mtk_eint_hw	*eint_hw;
};

struct mtk_pinctrl {
	struct pinctrl_dev		*pctrl;
	void __iomem			*base;
	struct device			*dev;
	struct gpio_chip		chip;
	const struct mtk_pin_soc	*soc;
	struct mtk_eint			*eint;
};

static const struct mtk_pin_field_calc leopard_pin_mode_range[] = {
	{0, 78, 0x300, 0x10, 0, 4},
};

static const struct mtk_pin_field_calc leopard_pin_dir_range[] = {
	{0, 78, 0x0, 0x10, 0, 1},
};

static const struct mtk_pin_field_calc leopard_pin_di_range[] = {
	{0, 78, 0x200, 0x10, 0, 1},
};

static const struct mtk_pin_field_calc leopard_pin_do_range[] = {
	{0, 78, 0x100, 0x10, 0, 1},
};

static const struct mtk_pin_field_calc leopard_pin_sr_range[] = {
	{0, 10, 0x1600, 0x10, 3, 4},
	{11, 18, 0x2600, 0x10, 3, 4},
	{19, 32, 0x3600, 0x10, 3, 4},
	{33, 48, 0x4600, 0x10, 3, 4},
	{49, 50, 0x5600, 0x10, 3, 4},
	{51, 69, 0x6600, 0x10, 3, 4},
	{70, 78, 0x7600, 0x10, 3, 4},
};

static const struct mtk_pin_field_calc leopard_pin_ies_range[] = {
	{0, 10, 0x1000, 0x10, 0, 1},
	{11, 18, 0x2000, 0x10, 0, 1},
	{19, 32, 0x3000, 0x10, 0, 1},
	{33, 48, 0x4000, 0x10, 0, 1},
	{49, 50, 0x5000, 0x10, 0, 1},
	{51, 69, 0x6000, 0x10, 0, 1},
	{70, 78, 0x7000, 0x10, 0, 1},
};

static const struct mtk_pin_field_calc leopard_pin_smt_range[] = {
	{0, 10, 0x1100, 0x10, 0, 1},
	{11, 18, 0x2100, 0x10, 0, 1},
	{19, 32, 0x3100, 0x10, 0, 1},
	{33, 48, 0x4100, 0x10, 0, 1},
	{49, 50, 0x5100, 0x10, 0, 1},
	{51, 69, 0x6100, 0x10, 0, 1},
	{70, 78, 0x7100, 0x10, 0, 1},
};

static const struct mtk_pin_field_calc leopard_pin_pullen_range[] = {
	{0, 10, 0x1400, 0x10, 0, 1},
	{11, 18, 0x2400, 0x10, 0, 1},
	{19, 32, 0x3400, 0x10, 0, 1},
	{33, 48, 0x4400, 0x10, 0, 1},
	{49, 50, 0x5400, 0x10, 0, 1},
	{51, 69, 0x6400, 0x10, 0, 1},
	{70, 78, 0x7400, 0x10, 0, 1},
};

static const struct mtk_pin_field_calc leopard_pin_pullsel_range[] = {
	{0, 10, 0x1500, 0x10, 0, 1},
	{11, 18, 0x2500, 0x10, 0, 1},
	{19, 32, 0x3500, 0x10, 0, 1},
	{33, 48, 0x4500, 0x10, 0, 1},
	{49, 50, 0x5500, 0x10, 0, 1},
	{51, 69, 0x6500, 0x10, 0, 1},
	{70, 78, 0x7500, 0x10, 0, 1},
};

static const struct mtk_pin_field_calc leopard_pin_drv_range[] = {
	{0, 10, 0x1600, 0x10, 0, 4},
	{11, 18, 0x2600, 0x10, 0, 4},
	{19, 32, 0x3600, 0x10, 0, 4},
	{33, 48, 0x4600, 0x10, 0, 4},
	{49, 50, 0x5600, 0x10, 0, 4},
	{51, 69, 0x6600, 0x10, 0, 4},
	{70, 78, 0x7600, 0x10, 0, 4},
};

static const struct mtk_pin_field_calc leopard_pin_tdsel_range[] = {
	{0, 10, 0x1200, 0x10, 0, 4},
	{11, 18, 0x2200, 0x10, 0, 4},
	{19, 32, 0x3200, 0x10, 0, 4},
	{33, 48, 0x4200, 0x10, 0, 4},
	{49, 50, 0x5200, 0x10, 0, 4},
	{51, 69, 0x6200, 0x10, 0, 4},
	{70, 78, 0x7200, 0x10, 0, 4},
};

static const struct mtk_pin_field_calc leopard_pin_rdsel_range[] = {
	{0, 10, 0x1300, 0x10, 0, 4},
	{11, 18, 0x2300, 0x10, 0, 4},
	{19, 32, 0x3300, 0x10, 0, 4},
	{33, 48, 0x4300, 0x10, 0, 4},
	{49, 50, 0x5300, 0x10, 0, 4},
	{51, 69, 0x6300, 0x10, 0, 4},
	{70, 78, 0x7300, 0x10, 0, 4},
};

static const struct mtk_pin_reg_calc leopard_reg_cals[PINCTRL_PIN_REG_MAX] = {
	[PINCTRL_PIN_REG_MODE] = MTK_RANGE(leopard_pin_mode_range),
	[PINCTRL_PIN_REG_DIR] = MTK_RANGE(leopard_pin_dir_range),
	[PINCTRL_PIN_REG_DI] = MTK_RANGE(leopard_pin_di_range),
	[PINCTRL_PIN_REG_DO] = MTK_RANGE(leopard_pin_do_range),
	[PINCTRL_PIN_REG_SR] = MTK_RANGE(leopard_pin_sr_range),
	[PINCTRL_PIN_REG_IES] = MTK_RANGE(leopard_pin_ies_range),
	[PINCTRL_PIN_REG_SMT] = MTK_RANGE(leopard_pin_smt_range),
	[PINCTRL_PIN_REG_PULLEN] = MTK_RANGE(leopard_pin_pullen_range),
	[PINCTRL_PIN_REG_PULLSEL] = MTK_RANGE(leopard_pin_pullsel_range),
	[PINCTRL_PIN_REG_DRV] = MTK_RANGE(leopard_pin_drv_range),
	[PINCTRL_PIN_REG_TDSEL] = MTK_RANGE(leopard_pin_tdsel_range),
	[PINCTRL_PIN_REG_RDSEL] = MTK_RANGE(leopard_pin_rdsel_range),
};

static const struct pinctrl_pin_desc leopard_pins[] = {
	PINCTRL_PIN(0, "TOP_5G_CLK"),
	PINCTRL_PIN(1, "TOP_5G_DATA"),
	PINCTRL_PIN(2, "WF0_5G_HB0"),
	PINCTRL_PIN(3, "WF0_5G_HB1"),
	PINCTRL_PIN(4, "WF0_5G_HB2"),
	PINCTRL_PIN(5, "WF0_5G_HB3"),
	PINCTRL_PIN(6, "WF0_5G_HB4"),
	PINCTRL_PIN(7, "WF0_5G_HB5"),
	PINCTRL_PIN(8, "WF0_5G_HB6"),
	PINCTRL_PIN(9, "XO_REQ"),
	PINCTRL_PIN(10, "TOP_RST_N"),
	PINCTRL_PIN(11, "SYS_WATCHDOG"),
	PINCTRL_PIN(12, "EPHY_LED0_N_JTDO"),
	PINCTRL_PIN(13, "EPHY_LED1_N_JTDI"),
	PINCTRL_PIN(14, "EPHY_LED2_N_JTMS"),
	PINCTRL_PIN(15, "EPHY_LED3_N_JTCLK"),
	PINCTRL_PIN(16, "EPHY_LED4_N_JTRST_N"),
	PINCTRL_PIN(17, "WF2G_LED_N"),
	PINCTRL_PIN(18, "WF5G_LED_N"),
	PINCTRL_PIN(19, "I2C_SDA"),
	PINCTRL_PIN(20, "I2C_SCL"),
	PINCTRL_PIN(21, "GPIO_9"),
	PINCTRL_PIN(22, "GPIO_10"),
	PINCTRL_PIN(23, "GPIO_11"),
	PINCTRL_PIN(24, "GPIO_12"),
	PINCTRL_PIN(25, "UART1_TXD"),
	PINCTRL_PIN(26, "UART1_RXD"),
	PINCTRL_PIN(27, "UART1_CTS"),
	PINCTRL_PIN(28, "UART1_RTS"),
	PINCTRL_PIN(29, "UART2_TXD"),
	PINCTRL_PIN(30, "UART2_RXD"),
	PINCTRL_PIN(31, "UART2_CTS"),
	PINCTRL_PIN(32, "UART2_RTS"),
	PINCTRL_PIN(33, "MDI_TP_P1"),
	PINCTRL_PIN(34, "MDI_TN_P1"),
	PINCTRL_PIN(35, "MDI_RP_P1"),
	PINCTRL_PIN(36, "MDI_RN_P1"),
	PINCTRL_PIN(37, "MDI_RP_P2"),
	PINCTRL_PIN(38, "MDI_RN_P2"),
	PINCTRL_PIN(39, "MDI_TP_P2"),
	PINCTRL_PIN(40, "MDI_TN_P2"),
	PINCTRL_PIN(41, "MDI_TP_P3"),
	PINCTRL_PIN(42, "MDI_TN_P3"),
	PINCTRL_PIN(43, "MDI_RP_P3"),
	PINCTRL_PIN(44, "MDI_RN_P3"),
	PINCTRL_PIN(45, "MDI_RP_P4"),
	PINCTRL_PIN(46, "MDI_RN_P4"),
	PINCTRL_PIN(47, "MDI_TP_P4"),
	PINCTRL_PIN(48, "MDI_TN_P4"),
	PINCTRL_PIN(49, "SMI_MDC"),
	PINCTRL_PIN(50, "SMI_MDIO"),
	PINCTRL_PIN(51, "PCIE_PERESET_N"),
	PINCTRL_PIN(52, "PWM_0"),
	PINCTRL_PIN(53, "GPIO_0"),
	PINCTRL_PIN(54, "GPIO_1"),
	PINCTRL_PIN(55, "GPIO_2"),
	PINCTRL_PIN(56, "GPIO_3"),
	PINCTRL_PIN(57, "GPIO_4"),
	PINCTRL_PIN(58, "GPIO_5"),
	PINCTRL_PIN(59, "GPIO_6"),
	PINCTRL_PIN(60, "GPIO_7"),
	PINCTRL_PIN(61, "GPIO_8"),
	PINCTRL_PIN(62, "SPI_CLK"),
	PINCTRL_PIN(63, "SPI_CS"),
	PINCTRL_PIN(64, "SPI_MOSI"),
	PINCTRL_PIN(65, "SPI_MISO"),
	PINCTRL_PIN(66, "SPI_WP"),
	PINCTRL_PIN(67, "SPI_HOLD"),
	PINCTRL_PIN(68, "UART0_TXD"),
	PINCTRL_PIN(69, "UART0_RXD"),
	PINCTRL_PIN(70, "TOP_2G_CLK"),
	PINCTRL_PIN(71, "TOP_2G_DATA"),
	PINCTRL_PIN(72, "WF0_2G_HB0"),
	PINCTRL_PIN(73, "WF0_2G_HB1"),
	PINCTRL_PIN(74, "WF0_2G_HB2"),
	PINCTRL_PIN(75, "WF0_2G_HB3"),
	PINCTRL_PIN(76, "WF0_2G_HB4"),
	PINCTRL_PIN(77, "WF0_2G_HB5"),
	PINCTRL_PIN(78, "WF0_2G_HB6"),
};

static const struct mtk_gpio_to_eint leopard_gpio_to_eint[] = {
	MT_GPIO_TO_EINT(0, 53),
	MT_GPIO_TO_EINT(1, 54),
	MT_GPIO_TO_EINT(2, 55),
	MT_GPIO_TO_EINT(3, 56),
	MT_GPIO_TO_EINT(4, 57),
	MT_GPIO_TO_EINT(5, 58),
	MT_GPIO_TO_EINT(6, 59),
	MT_GPIO_TO_EINT(7, 60),
	MT_GPIO_TO_EINT(8, 61),
	MT_GPIO_TO_EINT(9, 9),
	MT_GPIO_TO_EINT(10, 10),
	MT_GPIO_TO_EINT(11, 11),
	MT_GPIO_TO_EINT(12, 12),
	MT_GPIO_TO_EINT(13, 13),
	MT_GPIO_TO_EINT(14, 14),
	MT_GPIO_TO_EINT(15, 15),
	MT_GPIO_TO_EINT(16, 16),
	MT_GPIO_TO_EINT(17, 17),
	MT_GPIO_TO_EINT(18, 18),
	MT_GPIO_TO_EINT(19, 19),
	MT_GPIO_TO_EINT(20, 20),
	MT_GPIO_TO_EINT(21, 21),
	MT_GPIO_TO_EINT(22, 22),
	MT_GPIO_TO_EINT(23, 23),
	MT_GPIO_TO_EINT(24, 24),
	MT_GPIO_TO_EINT(25, 25),
	MT_GPIO_TO_EINT(26, 26),
	MT_GPIO_TO_EINT(27, 27),
	MT_GPIO_TO_EINT(28, 28),
	MT_GPIO_TO_EINT(29, 29),
	MT_GPIO_TO_EINT(30, 30),
	MT_GPIO_TO_EINT(31, 31),
	MT_GPIO_TO_EINT(32, 32),
	MT_GPIO_TO_EINT(33, 33),
	MT_GPIO_TO_EINT(34, 34),
	MT_GPIO_TO_EINT(35, 35),
	MT_GPIO_TO_EINT(36, 36),
	MT_GPIO_TO_EINT(37, 37),
	MT_GPIO_TO_EINT(38, 38),
	MT_GPIO_TO_EINT(39, 39),
	MT_GPIO_TO_EINT(40, 40),
	MT_GPIO_TO_EINT(41, 41),
	MT_GPIO_TO_EINT(42, 42),
	MT_GPIO_TO_EINT(43, 43),
	MT_GPIO_TO_EINT(44, 44),
	MT_GPIO_TO_EINT(45, 45),
	MT_GPIO_TO_EINT(46, 46),
	MT_GPIO_TO_EINT(47, 47),
	MT_GPIO_TO_EINT(48, 48),
	MT_GPIO_TO_EINT(49, 49),
	MT_GPIO_TO_EINT(50, 50),
	MT_GPIO_TO_EINT(51, 51),
	MT_GPIO_TO_EINT(52, 52),
	MT_GPIO_TO_EINT(53, 0),
	MT_GPIO_TO_EINT(54, 1),
	MT_GPIO_TO_EINT(55, 2),
	MT_GPIO_TO_EINT(56, 3),
	MT_GPIO_TO_EINT(57, 4),
	MT_GPIO_TO_EINT(58, 5),
	MT_GPIO_TO_EINT(59, 6),
	MT_GPIO_TO_EINT(60, 7),
	MT_GPIO_TO_EINT(61, 8),
	MT_GPIO_TO_EINT(62, 62),
	MT_GPIO_TO_EINT(63, 63),
	MT_GPIO_TO_EINT(64, 64),
	MT_GPIO_TO_EINT(65, 65),
	MT_GPIO_TO_EINT(66, 66),
	MT_GPIO_TO_EINT(67, 67),
	MT_GPIO_TO_EINT(68, 68),
	MT_GPIO_TO_EINT(69, 69),
	MT_GPIO_TO_EINT(70, 70),
	MT_GPIO_TO_EINT(71, 71),
	MT_GPIO_TO_EINT(72, 72),
	MT_GPIO_TO_EINT(73, 73),
	MT_GPIO_TO_EINT(74, 74),
	MT_GPIO_TO_EINT(75, 75),
	MT_GPIO_TO_EINT(76, 76),
	MT_GPIO_TO_EINT(77, 77),
	MT_GPIO_TO_EINT(78, 78)
};

/* List all groups consisting of these pins dedicated to the enablement of
 * certain hardware block and the corresponding mode for all of the pins. The
 * hardware probably has multiple combinations of these pinouts.
 */

/* WF 5G */
static int leopard_wf0_5g_pins[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, };
static int leopard_wf0_5g_funcs[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, };

/* LED for EPHY */
static int leopard_ephy_leds_pins[] = { 12, 13, 14, 15, 16, 17, 18, };
static int leopard_ephy_leds_funcs[] = { 1, 1, 1, 1, 1, 1, 1, };
static int leopard_ephy_led0_pins[] = { 12, };
static int leopard_ephy_led0_funcs[] = { 1, };
static int leopard_ephy_led1_pins[] = { 13, };
static int leopard_ephy_led1_funcs[] = { 1, };
static int leopard_ephy_led2_pins[] = { 14, };
static int leopard_ephy_led2_funcs[] = { 1, };
static int leopard_ephy_led3_pins[] = { 15, };
static int leopard_ephy_led3_funcs[] = { 1, };
static int leopard_ephy_led4_pins[] = { 16, };
static int leopard_ephy_led4_funcs[] = { 1, };
static int leopard_wf2g_led_pins[] = { 17, };
static int leopard_wf2g_led_funcs[] = { 1, };
static int leopard_wf5g_led_pins[] = { 18, };
static int leopard_wf5g_led_funcs[] = { 1, };

/* Watchdog */
static int leopard_watchdog_pins[] = { 11, };
static int leopard_watchdog_funcs[] = { 1, };

/* LED for GPHY */
static int leopard_gphy_leds_0_pins[] = { 21, 22, 23, };
static int leopard_gphy_leds_0_funcs[] = { 2, 2, 2, };
static int leopard_gphy_led1_0_pins[] = { 21, };
static int leopard_gphy_led1_0_funcs[] = { 2, };
static int leopard_gphy_led2_0_pins[] = { 22, };
static int leopard_gphy_led2_0_funcs[] = { 2, };
static int leopard_gphy_led3_0_pins[] = { 23, };
static int leopard_gphy_led3_0_funcs[] = { 2, };
static int leopard_gphy_leds_1_pins[] = { 57, 58, 59, };
static int leopard_gphy_leds_1_funcs[] = { 1, 1, 1, };
static int leopard_gphy_led1_1_pins[] = { 57, };
static int leopard_gphy_led1_1_funcs[] = { 1, };
static int leopard_gphy_led2_1_pins[] = { 58, };
static int leopard_gphy_led2_1_funcs[] = { 1, };
static int leopard_gphy_led3_1_pins[] = { 59, };
static int leopard_gphy_led3_1_funcs[] = { 1, };

/* I2C */
static int leopard_i2c_0_pins[] = { 19, 20, };
static int leopard_i2c_0_funcs[] = { 1, 1, };
static int leopard_i2c_1_pins[] = { 53, 54, };
static int leopard_i2c_1_funcs[] = { 1, 1, };

/* SPI */
static int leopard_spi_0_pins[] = { 21, 22, 23, 24, };
static int leopard_spi_0_funcs[] = { 1, 1, 1, 1, };
static int leopard_spi_1_pins[] = { 62, 63, 64, 65, };
static int leopard_spi_1_funcs[] = { 1, 1, 1, 1, };
static int leopard_spi_wp_pins[] = { 66, };
static int leopard_spi_wp_funcs[] = { 1, };
static int leopard_spi_hold_pins[] = { 67, };
static int leopard_spi_hold_funcs[] = { 1, };

/* UART */
static int leopard_uart1_0_txd_rxd_pins[] = { 25, 26, };
static int leopard_uart1_0_txd_rxd_funcs[] = { 1, 1, };
static int leopard_uart1_1_txd_rxd_pins[] = { 53, 54, };
static int leopard_uart1_1_txd_rxd_funcs[] = { 2, 2, };
static int leopard_uart2_0_txd_rxd_pins[] = { 29, 30, };
static int leopard_uart2_0_txd_rxd_funcs[] = { 1, 1, };
static int leopard_uart2_1_txd_rxd_pins[] = { 57, 58, };
static int leopard_uart2_1_txd_rxd_funcs[] = { 2, 2, };
static int leopard_uart1_0_cts_rts_pins[] = { 27, 28, };
static int leopard_uart1_0_cts_rts_funcs[] = { 1, 1, };
static int leopard_uart1_1_cts_rts_pins[] = { 55, 56, };
static int leopard_uart1_1_cts_rts_funcs[] = { 2, 2, };
static int leopard_uart2_0_cts_rts_pins[] = { 31, 32, };
static int leopard_uart2_0_cts_rts_funcs[] = { 1, 1, };
static int leopard_uart2_1_cts_rts_pins[] = { 59, 60, };
static int leopard_uart2_1_cts_rts_funcs[] = { 2, 2, };
static int leopard_uart0_txd_rxd_pins[] = { 68, 69, };
static int leopard_uart0_txd_rxd_funcs[] = { 1, 1, };

/* MDC/MDIO */
static int leopard_mdc_mdio_pins[] = { 49, 50, };
static int leopard_mdc_mdio_funcs[] = { 1, 1, };

/* PCIE */
static int leopard_pcie_pereset_pins[] = { 51, };
static int leopard_pcie_pereset_funcs[] = { 1, };
static int leopard_pcie_wake_pins[] = { 55, };
static int leopard_pcie_wake_funcs[] = { 1, };
static int leopard_pcie_clkreq_pins[] = { 56, };
static int leopard_pcie_clkreq_funcs[] = { 1, };

/* PWM */
static int leopard_pwm_0_pins[] = { 52, };
static int leopard_pwm_0_funcs[] = { 1, };
static int leopard_pwm_1_pins[] = { 61, };
static int leopard_pwm_1_funcs[] = { 2, };

/* WF 2G */
static int leopard_wf0_2g_pins[] = { 70, 71, 72, 73, 74, 75, 76, 77, 78, };
static int leopard_wf0_2g_funcs[] = { 1, 1, 1, 1, 1, 1, 1, 1, };

/* SNFI */
static int leopard_snfi_pins[] = { 62, 63, 64, 65, };
static int leopard_snfi_funcs[] = { 2, 2, 2, 2, };
static int leopard_snfi_wp_pins[] = { 66, };
static int leopard_snfi_wp_funcs[] = { 2, };
static int leopard_snfi_hold_pins[] = { 67, };
static int leopard_snfi_hold_funcs[] = { 2, };

/* spi nor flash */
static int leopard_nor_flash_io_pins[] = { 62, 63, 64, 65, };
static int leopard_nor_flash_io_funcs[] = { 1, 1, 1, 1, };
static int leopard_nor_flash_wp_pins[] = { 66, };
static int leopard_nor_flash_wp_funcs[] = { 1, };
static int leopard_nor_flash_hold_pins[] = { 67, };
static int leopard_nor_flash_hold_funcs[] = { 1, };

/* CONN_EXT_PRI */
static int leopard_conn_ext_pri_0_pins[] = { 21, };
static int leopard_conn_ext_pri_0_funcs[] = { 3, };
static int leopard_conn_ext_pri_1_pins[] = { 57, };
static int leopard_conn_ext_pri_1_funcs[] = { 3, };

/* CONN_EXT_ACT */
static int leopard_conn_ext_act_0_pins[] = { 22, };
static int leopard_conn_ext_act_0_funcs[] = { 3, };
static int leopard_conn_ext_act_1_pins[] = { 58, };
static int leopard_conn_ext_act_1_funcs[] = { 3, };

/* CONN_WLAN_ACT */
static int leopard_conn_wlan_act_0_pins[] = { 23, };
static int leopard_conn_wlan_act_0_funcs[] = { 3, };
static int leopard_conn_wlan_act_1_pins[] = { 59, };
static int leopard_conn_wlan_act_1_funcs[] = { 3, };

/* EXT2BT_ACTIVE */
static int leopard_ext2bt_active_0_pins[] = { 30, };
static int leopard_ext2bt_active_0_funcs[] = { 3, };
static int leopard_ext2bt_active_1_pins[] = { 52, };
static int leopard_ext2bt_active_1_funcs[] = { 3, };

/* EXT2WF_TX_ACTIVE */
static int leopard_ext2wf_tx_active_0_pins[] = { 31, };
static int leopard_ext2wf_tx_active_0_funcs[] = { 3, };
static int leopard_ext2wf_tx_active_1_pins[] = { 60, };
static int leopard_ext2wf_tx_active_1_funcs[] = { 3, };

/* EXT2EXT_TX_ACTIVE */
static int leopard_ext2ext_tx_active_pins[] = { 32, };
static int leopard_ext2ext_tx_active_funcs[] = { 3, };

/* WF2EXT_TX_ACTIVE */
static int leopard_wf2ext_tx_active_pins[] = { 61, };
static int leopard_wf2ext_tx_active_funcs[] = { 3, };

/* DFD */
static int leopard_dfd_pins[] = { 12, 13, 14, 15, 16, };
static int leopard_dfd_funcs[] = { 4, 4, 4, 4, 4, };

/* JTAG DEBUG */
static int leopard_jtag_pins[] = { 12, 13, 14, 15, };
static int leopard_jtag_funcs[] = { 7, 7, 7, 7, };

static const struct group_desc leopard_groups[] = {
	PINCTRL_PIN_GROUP("wf0_5g", leopard_wf0_5g),
	PINCTRL_PIN_GROUP("ephy_leds", leopard_ephy_leds),
	PINCTRL_PIN_GROUP("ephy_led0", leopard_ephy_led0),
	PINCTRL_PIN_GROUP("ephy_led1", leopard_ephy_led1),
	PINCTRL_PIN_GROUP("ephy_led2", leopard_ephy_led2),
	PINCTRL_PIN_GROUP("ephy_led3", leopard_ephy_led3),
	PINCTRL_PIN_GROUP("ephy_led4", leopard_ephy_led4),
	PINCTRL_PIN_GROUP("wf2g_led", leopard_wf2g_led),
	PINCTRL_PIN_GROUP("wf5g_led", leopard_wf5g_led),
	PINCTRL_PIN_GROUP("watchdog", leopard_watchdog),
	PINCTRL_PIN_GROUP("gphy_leds_0", leopard_gphy_leds_0),
	PINCTRL_PIN_GROUP("gphy_led1_0", leopard_gphy_led1_0),
	PINCTRL_PIN_GROUP("gphy_led2_0", leopard_gphy_led2_0),
	PINCTRL_PIN_GROUP("gphy_led3_0", leopard_gphy_led3_0),
	PINCTRL_PIN_GROUP("gphy_leds_1", leopard_gphy_leds_1),
	PINCTRL_PIN_GROUP("gphy_led1_1", leopard_gphy_led1_1),
	PINCTRL_PIN_GROUP("gphy_led2_1", leopard_gphy_led2_1),
	PINCTRL_PIN_GROUP("gphy_led3_1", leopard_gphy_led3_1),
	PINCTRL_PIN_GROUP("i2c_0", leopard_i2c_0),
	PINCTRL_PIN_GROUP("i2c_1", leopard_i2c_1),
	PINCTRL_PIN_GROUP("spi_0", leopard_spi_0),
	PINCTRL_PIN_GROUP("spi_1", leopard_spi_1),
	PINCTRL_PIN_GROUP("spi_wp", leopard_spi_wp),
	PINCTRL_PIN_GROUP("spi_hold", leopard_spi_hold),
	PINCTRL_PIN_GROUP("uart1_0_txd_rxd", leopard_uart1_0_txd_rxd),
	PINCTRL_PIN_GROUP("uart1_1_txd_rxd", leopard_uart1_1_txd_rxd),
	PINCTRL_PIN_GROUP("uart2_0_txd_rxd", leopard_uart2_0_txd_rxd),
	PINCTRL_PIN_GROUP("uart2_1_txd_rxd", leopard_uart2_1_txd_rxd),
	PINCTRL_PIN_GROUP("uart1_0_cts_rts", leopard_uart1_0_cts_rts),
	PINCTRL_PIN_GROUP("uart1_1_cts_rts", leopard_uart1_1_cts_rts),
	PINCTRL_PIN_GROUP("uart2_0_cts_rts", leopard_uart2_0_cts_rts),
	PINCTRL_PIN_GROUP("uart2_1_cts_rts", leopard_uart2_1_cts_rts),
	PINCTRL_PIN_GROUP("uart0_txd_rxd", leopard_uart0_txd_rxd),
	PINCTRL_PIN_GROUP("mdc_mdio", leopard_mdc_mdio),
	PINCTRL_PIN_GROUP("pcie_pereset", leopard_pcie_pereset),
	PINCTRL_PIN_GROUP("pcie_wake", leopard_pcie_wake),
	PINCTRL_PIN_GROUP("pcie_clkreq", leopard_pcie_clkreq),
	PINCTRL_PIN_GROUP("pwm_0", leopard_pwm_0),
	PINCTRL_PIN_GROUP("pwm_1", leopard_pwm_1),
	PINCTRL_PIN_GROUP("wf0_2g", leopard_wf0_2g),
	PINCTRL_PIN_GROUP("snfi", leopard_snfi),
	PINCTRL_PIN_GROUP("snfi_wp", leopard_snfi_wp),
	PINCTRL_PIN_GROUP("snfi_hold", leopard_snfi_hold),
	PINCTRL_PIN_GROUP("nor_flash_io", leopard_nor_flash_io),
	PINCTRL_PIN_GROUP("nor_flash_wp", leopard_nor_flash_wp),
	PINCTRL_PIN_GROUP("nor_flash_hold", leopard_nor_flash_hold),
	PINCTRL_PIN_GROUP("conn_ext_pri_0", leopard_conn_ext_pri_0),
	PINCTRL_PIN_GROUP("conn_ext_pri_1", leopard_conn_ext_pri_1),
	PINCTRL_PIN_GROUP("conn_ext_act_0", leopard_conn_ext_act_0),
	PINCTRL_PIN_GROUP("conn_ext_act_1", leopard_conn_ext_act_1),
	PINCTRL_PIN_GROUP("conn_wlan_act_0", leopard_conn_wlan_act_0),
	PINCTRL_PIN_GROUP("conn_wlan_act_1", leopard_conn_wlan_act_1),
	PINCTRL_PIN_GROUP("ext2bt_active_0", leopard_ext2bt_active_0),
	PINCTRL_PIN_GROUP("ext2bt_active_1", leopard_ext2bt_active_1),
	PINCTRL_PIN_GROUP("ext2wf_tx_active_0", leopard_ext2wf_tx_active_0),
	PINCTRL_PIN_GROUP("ext2wf_tx_active_1", leopard_ext2wf_tx_active_1),
	PINCTRL_PIN_GROUP("ext2ext_tx_active", leopard_ext2ext_tx_active),
	PINCTRL_PIN_GROUP("wf2ext_tx_active", leopard_wf2ext_tx_active),
	PINCTRL_PIN_GROUP("dfd", leopard_dfd),
	PINCTRL_PIN_GROUP("jtag", leopard_jtag),
};

/* Joint those groups owning the same capability in user point of view which
 * allows that people tend to use through the device tree.
 */
static const char *const leopard_ethernet_groups[] = { "mdc_mdio", };
static const char *const leopard_wifi_groups[] = { "wf0_5g", "wf0_2g", };
static const char *const leopard_i2c_groups[] = { "i2c_0", "i2c_1", };
static const char *const leopard_led_groups[] = { "ephy_leds", "ephy_led0",
						  "ephy_led1", "ephy_led2",
						  "ephy_led3", "ephy_led4",
						  "wf2g_led", "wf5g_led",
						  "gphy_leds_0", "gphy_led1_0",
						  "gphy_led2_0", "gphy_led3_0",
						  "gphy_leds_1", "gphy_led1_1",
						  "gphy_led2_1", "gphy_led3_1",
						};
static const char *const leopard_flash_groups[] = { };
static const char *const leopard_pcie_groups[] = { "pcie_pereset", "pcie_wake",
						   "pcie_clkreq", };
static const char *const leopard_pwm_groups[] = { "pwm_0", "pwm_1", };
static const char *const leopard_spi_groups[] = { "spi_0", "spi_1", "spi_wp",
						  "spi_hold", };
static const char *const leopard_dfd_groups[] = { "dfd", };
static const char *const leopard_uart_groups[] = { "uart1_0_txd_rxd",
						   "uart1_1_txd_rxd",
						   "uart2_0_txd_rxd",
						   "uart2_1_txd_rxd",
						   "uart1_0_cts_rts",
						   "uart1_1_cts_rts",
						   "uart2_0_cts_rts",
						   "uart2_1_cts_rts",
						   "uart0_txd_rxd", };
static const char *const leopard_wdt_groups[] = { "watchdog", };
static const char *const leopard_snfi_groups[] = { "snfi", "snfi_wp",
						   "snfi_hold", };
static const char *const leopard_nor_flash_groups[] = { "nor_flash_io", "nor_flash_wp",
						   "nor_flash_hold", };
static const char *const leopard_ext_groups[] = { "conn_ext_pri_0",
						  "conn_ext_pri_1",
						  "conn_ext_act_0",
						  "conn_ext_act_1",
						  "conn_wlan_act_0",
						  "conn_wlan_act_1",
						  "ext2bt_active_0",
						  "ext2bt_active_1",
						  "ext2wf_tx_active_0",
						  "ext2wf_tx_active_1",
						  "ext2ext_tx_active",
						  "wf2ext_tx_active", };
static const char *const leopard_jtag_groups[] = { "jtag", };

static const struct function_desc leopard_functions[] = {
	{"eth",	leopard_ethernet_groups, ARRAY_SIZE(leopard_ethernet_groups)},
	{"wifi", leopard_wifi_groups, ARRAY_SIZE(leopard_wifi_groups)},
	{"i2c", leopard_i2c_groups, ARRAY_SIZE(leopard_i2c_groups)},
	{"led",	leopard_led_groups, ARRAY_SIZE(leopard_led_groups)},
	{"flash", leopard_flash_groups, ARRAY_SIZE(leopard_flash_groups)},
	{"pcie", leopard_pcie_groups, ARRAY_SIZE(leopard_pcie_groups)},
	{"pwm",	leopard_pwm_groups, ARRAY_SIZE(leopard_pwm_groups)},
	{"spi",	leopard_spi_groups, ARRAY_SIZE(leopard_spi_groups)},
	{"dfd",	leopard_dfd_groups, ARRAY_SIZE(leopard_dfd_groups)},
	{"uart", leopard_uart_groups, ARRAY_SIZE(leopard_uart_groups)},
	{"watchdog", leopard_wdt_groups, ARRAY_SIZE(leopard_wdt_groups)},
	{"snfi", leopard_snfi_groups, ARRAY_SIZE(leopard_snfi_groups)},
	{"nor_flash", leopard_nor_flash_groups, ARRAY_SIZE(leopard_nor_flash_groups)},
	{"ext", leopard_ext_groups, ARRAY_SIZE(leopard_ext_groups)},
	{"jtag", leopard_jtag_groups, ARRAY_SIZE(leopard_jtag_groups)},
};

static const struct pinconf_generic_params mtk_custom_bindings[] = {
	{"mediatek,tdsel",	MTK_PIN_CONFIG_TDSEL,		0},
	{"mediatek,rdsel",	MTK_PIN_CONFIG_RDSEL,		0},
};

#ifdef CONFIG_DEBUG_FS
static const struct pin_config_item mtk_conf_items[] = {
	PCONFDUMP(MTK_PIN_CONFIG_TDSEL, "tdsel", NULL, true),
	PCONFDUMP(MTK_PIN_CONFIG_RDSEL, "rdsel", NULL, true),
};
#endif

static const struct mtk_eint_hw leopard_eint_hw = {
	.port_mask = 7,
	.ports     = 7,
	.ap_num    = ARRAY_SIZE(leopard_pins),
	.db_cnt    = 16,
};

static const struct mtk_pin_soc leopard_data = {
	.reg_cal = leopard_reg_cals,
	.pins = leopard_pins,
	.npins = ARRAY_SIZE(leopard_pins),
	.grps = leopard_groups,
	.ngrps = ARRAY_SIZE(leopard_groups),
	.funcs = leopard_functions,
	.nfuncs = ARRAY_SIZE(leopard_functions),
	.eint_hw = &leopard_eint_hw,
};

static void mtk_w32(struct mtk_pinctrl *pctl, u32 reg, u32 val)
{
	writel_relaxed(val, pctl->base + reg);
}

static u32 mtk_r32(struct mtk_pinctrl *pctl, u32 reg)
{
	return readl_relaxed(pctl->base + reg);
}

static void mtk_rmw(struct mtk_pinctrl *pctl, u32 reg, u32 mask, u32 set)
{
	u32 val;

	val = mtk_r32(pctl, reg);
	val &= ~mask;
	val |= set;
	mtk_w32(pctl, reg, val);
}

static int mtk_hw_pin_field_lookup(struct mtk_pinctrl *hw, int pin,
				   const struct mtk_pin_reg_calc *rc,
				   struct mtk_pin_field *pfd)
{
	const struct mtk_pin_field_calc *c, *e;
	u32 bits;

	c = rc->range;
	e = c + rc->nranges;

	while (c < e) {
		if (pin >= c->s_pin && pin <= c->e_pin)
			break;
		c++;
	}

	if (c >= e) {
		dev_err(hw->dev, "Out of range for pin = %d\n", pin);
		return -EINVAL;
	}

	/* Caculated bits as the overall offset the pin is located at */
	bits = c->s_bit + (pin - c->s_pin) * (c->x_bits);

	/* Fill pfd from bits and 32-bit register applied is assumed */
	pfd->offset = c->s_addr + c->x_addrs * (bits / 32);
	pfd->bitpos = bits % 32;
	pfd->mask = (1 << c->x_bits) - 1;

	/* pfd->next is used for indicating that bit wrapping-around happens
	 * which requires the manipulation for bit 0 starting in the next
	 * register to form the complete field read/write.
	 */
	pfd->next = pfd->bitpos + c->x_bits - 1 > 31 ? c->x_addrs : 0;

	return 0;
}

static int mtk_hw_pin_field_get(struct mtk_pinctrl *hw, int pin,
				int field, struct mtk_pin_field *pfd)
{
	const struct mtk_pin_reg_calc *rc;

	if (field < 0 || field >= PINCTRL_PIN_REG_MAX) {
		dev_err(hw->dev, "Invalid Field %d\n", field);
		return -EINVAL;
	}

	if (hw->soc->reg_cal && hw->soc->reg_cal[field].range) {
		rc = &hw->soc->reg_cal[field];
	} else {
		dev_err(hw->dev, "Undefined range for field %d\n", field);
		return -EINVAL;
	}

	return mtk_hw_pin_field_lookup(hw, pin, rc, pfd);
}

static void mtk_hw_bits_part(struct mtk_pin_field *pf, int *h, int *l)
{
	*l = 32 - pf->bitpos;
	*h = get_count_order(pf->mask) - *l;
}

static void mtk_hw_write_cross_field(struct mtk_pinctrl *hw,
				     struct mtk_pin_field *pf, int value)
{
	int nbits_l, nbits_h;

	mtk_hw_bits_part(pf, &nbits_h, &nbits_l);

	mtk_rmw(hw, pf->offset, pf->mask << pf->bitpos,
		(value & pf->mask) << pf->bitpos);

	mtk_rmw(hw, pf->offset + pf->next, BIT(nbits_h) - 1,
		(value & pf->mask) >> nbits_l);
}

static void mtk_hw_read_cross_field(struct mtk_pinctrl *hw,
				    struct mtk_pin_field *pf, int *value)
{
	int nbits_l, nbits_h, h, l;

	mtk_hw_bits_part(pf, &nbits_h, &nbits_l);

	l  = (mtk_r32(hw, pf->offset) >> pf->bitpos) & (BIT(nbits_l) - 1);
	h  = (mtk_r32(hw, pf->offset + pf->next)) & (BIT(nbits_h) - 1);

	*value = (h << nbits_l) | l;
}

static int mtk_hw_set_value(struct mtk_pinctrl *hw, int pin, int field,
			    int value)
{
	struct mtk_pin_field pf;
	int err;

	err = mtk_hw_pin_field_get(hw, pin, field, &pf);
	if (err)
		return err;

	if (!pf.next)
		mtk_rmw(hw, pf.offset, pf.mask << pf.bitpos,
			(value & pf.mask) << pf.bitpos);
	else
		mtk_hw_write_cross_field(hw, &pf, value);

	return 0;
}

static int mtk_hw_get_value(struct mtk_pinctrl *hw, int pin, int field,
			    int *value)
{
	struct mtk_pin_field pf;
	int err;

	err = mtk_hw_pin_field_get(hw, pin, field, &pf);
	if (err)
		return err;

	if (!pf.next)
		*value = (mtk_r32(hw, pf.offset) >> pf.bitpos) & pf.mask;
	else
		mtk_hw_read_cross_field(hw, &pf, value);

	return 0;
}

static int mtk_pinmux_set_mux(struct pinctrl_dev *pctldev,
			      unsigned int selector, unsigned int group)
{
	struct mtk_pinctrl *hw = pinctrl_dev_get_drvdata(pctldev);
	struct function_desc *func;
	struct group_desc *grp;
	int i;

	func = pinmux_generic_get_function(pctldev, selector);
	if (!func)
		return -EINVAL;

	grp = pinctrl_generic_get_group(pctldev, group);
	if (!grp)
		return -EINVAL;

	dev_dbg(pctldev->dev, "enable function %s group %s\n",
		func->name, grp->name);

	for (i = 0; i < grp->num_pins; i++) {
		int *pin_modes = grp->data;

		mtk_hw_set_value(hw, grp->pins[i], PINCTRL_PIN_REG_MODE,
				 pin_modes[i]);
	}

	return 0;
}

static int mtk_pinmux_gpio_request_enable(struct pinctrl_dev *pctldev,
					  struct pinctrl_gpio_range *range,
					  unsigned int pin)
{
	struct mtk_pinctrl *hw = pinctrl_dev_get_drvdata(pctldev);

	return mtk_hw_set_value(hw, pin, PINCTRL_PIN_REG_MODE, MTK_GPIO_MODE);
}

static int mtk_pinmux_gpio_set_direction(struct pinctrl_dev *pctldev,
					 struct pinctrl_gpio_range *range,
					 unsigned int pin, bool input)
{
	struct mtk_pinctrl *hw = pinctrl_dev_get_drvdata(pctldev);

	/* hardware would take 0 as input direction */
	return mtk_hw_set_value(hw, pin, PINCTRL_PIN_REG_DIR, !input);
}

static int mtk_pconf_set_pull_select(struct mtk_pinctrl *hw,
				     unsigned int pin,
				     bool enable, bool isup, unsigned int arg)
{
	int err;

	/* For generic pull config, default arg value should be 0 or 1. */
	if (arg != 0 && arg != 1) {
		dev_err(hw->dev, "invalid pull-up argument %d on pin %d .\n",
			arg, pin);
		return -EINVAL;
	}

	if (enable)
		err = mtk_hw_set_value(hw, pin, PINCTRL_PIN_REG_PULLEN, 1);
	else
		err = mtk_hw_set_value(hw, pin, PINCTRL_PIN_REG_PULLEN, 0);
	if (err)
		goto err;

	if (isup)
		err = mtk_hw_set_value(hw, pin, PINCTRL_PIN_REG_PULLSEL, 1);
	else
		err = mtk_hw_set_value(hw, pin, PINCTRL_PIN_REG_PULLSEL, 0);
	if (err)
		goto err;

	return 0;

err:
	return err;
}

static int mtk_pinconf_get(struct pinctrl_dev *pctldev,
			   unsigned int pin, unsigned long *config)
{
	struct mtk_pinctrl *hw = pinctrl_dev_get_drvdata(pctldev);
	u32 param = pinconf_to_config_param(*config);
	int val, val2, err, reg, ret = 1;

	switch (param) {
	case PIN_CONFIG_BIAS_DISABLE:
		err = mtk_hw_get_value(hw, pin, PINCTRL_PIN_REG_PULLEN, &val);
		if (err)
			return err;

		if (val)
			return -EINVAL;

		break;
	case PIN_CONFIG_BIAS_PULL_UP:
	case PIN_CONFIG_BIAS_PULL_DOWN:
	case PIN_CONFIG_SLEW_RATE:
		reg = (param == PIN_CONFIG_BIAS_PULL_UP) ?
		      PINCTRL_PIN_REG_PULLSEL :
		      (param == PIN_CONFIG_BIAS_PULL_DOWN) ?
		      PINCTRL_PIN_REG_PULLSEL : PINCTRL_PIN_REG_SR;

		err = mtk_hw_get_value(hw, pin, reg, &val);
		if (err)
			return err;

		if (!val)
			return -EINVAL;

		break;
	case PIN_CONFIG_INPUT_ENABLE:
	case PIN_CONFIG_OUTPUT_ENABLE:
		err = mtk_hw_get_value(hw, pin, PINCTRL_PIN_REG_DIR, &val);
		if (err)
			return err;

		/* HW takes input mode as zero; output mode as non-zero */
		if ((val && param == PIN_CONFIG_INPUT_ENABLE) ||
		    (!val && param == PIN_CONFIG_OUTPUT_ENABLE))
			return -EINVAL;

		break;
	case PIN_CONFIG_INPUT_SCHMITT_ENABLE:
		err = mtk_hw_get_value(hw, pin, PINCTRL_PIN_REG_DIR, &val);
		if (err)
			return err;

		err = mtk_hw_get_value(hw, pin, PINCTRL_PIN_REG_SMT, &val2);
		if (err)
			return err;

		if (val || !val2)
			return -EINVAL;

		break;
	case PIN_CONFIG_DRIVE_STRENGTH:
		err = mtk_hw_get_value(hw, pin, PINCTRL_PIN_REG_DRV, &val);
		if (err)
			return err;

		/* 4mA when [2:0] = 000; 8mA when [2:0] = 010;
		 * 12mA when [2:0] = 100; 16mA when [2:0] = 110;
		 */

		ret = (((val & 0x7) >> 1) + 1) * 4;

		break;
	case MTK_PIN_CONFIG_TDSEL:
	case MTK_PIN_CONFIG_RDSEL:
		reg = (param == MTK_PIN_CONFIG_TDSEL) ?
		       PINCTRL_PIN_REG_TDSEL : PINCTRL_PIN_REG_RDSEL;

		err = mtk_hw_get_value(hw, pin, reg, &val);
		if (err)
			return err;

		ret = val;

		break;
	default:
		return -ENOTSUPP;
	}

	*config = pinconf_to_config_packed(param, ret);

	return 0;
}

static int mtk_pinconf_set(struct pinctrl_dev *pctldev, unsigned int pin,
			   unsigned long *configs, unsigned int num_configs)
{
	struct mtk_pinctrl *hw = pinctrl_dev_get_drvdata(pctldev);
	u32 reg, param, arg;
	int cfg, err = 0;

	for (cfg = 0; cfg < num_configs; cfg++) {
		param = pinconf_to_config_param(configs[cfg]);
		arg = pinconf_to_config_argument(configs[cfg]);

		switch (param) {
		case PIN_CONFIG_BIAS_DISABLE:
			err = mtk_pconf_set_pull_select(hw, pin,
							false, false, arg);
			if (err)
				goto err;
			break;
		case PIN_CONFIG_BIAS_PULL_UP:
			err = mtk_pconf_set_pull_select(hw, pin,
							true, true, arg);
			if (err)
				goto err;
			break;
		case PIN_CONFIG_BIAS_PULL_DOWN:
			err = mtk_pconf_set_pull_select(hw, pin,
							true, false, arg);
			if (err)
				goto err;
			break;
		case PIN_CONFIG_OUTPUT_ENABLE:
			err = mtk_hw_set_value(hw, pin, PINCTRL_PIN_REG_SMT,
					       MTK_DISABLE);
			if (err)
				goto err;
			err = mtk_hw_set_value(hw, pin, PINCTRL_PIN_REG_DIR,
					       MTK_OUTPUT);
			if (err)
				goto err;
			break;
		case PIN_CONFIG_INPUT_ENABLE:
			err = mtk_hw_set_value(hw, pin, PINCTRL_PIN_REG_IES,
					       MTK_ENABLE);
			if (err)
				goto err;
			err = mtk_hw_set_value(hw, pin, PINCTRL_PIN_REG_DIR,
					       MTK_INPUT);
			if (err)
				goto err;
			break;
		case PIN_CONFIG_SLEW_RATE:
			err = mtk_hw_set_value(hw, pin, PINCTRL_PIN_REG_SR,
					       arg);
			if (err)
				goto err;

			break;
		case PIN_CONFIG_OUTPUT:
			err = mtk_hw_set_value(hw, pin, PINCTRL_PIN_REG_DIR,
					       MTK_OUTPUT);
			if (err)
				goto err;

			err = mtk_hw_set_value(hw, pin, PINCTRL_PIN_REG_DO,
					       arg);
			if (err)
				goto err;
			break;
		case PIN_CONFIG_INPUT_SCHMITT_ENABLE:
			/* arg = 1: Input mode & SMT enable ;
			 * arg = 0: Output mode & SMT disable
			 */
			arg = arg ? 2 : 1;
			err = mtk_hw_set_value(hw, pin, PINCTRL_PIN_REG_DIR,
					       arg & 1);
			if (err)
				goto err;

			err = mtk_hw_set_value(hw, pin, PINCTRL_PIN_REG_SMT,
					       !!(arg & 2));
			if (err)
				goto err;
			break;
		case PIN_CONFIG_DRIVE_STRENGTH:
			/* 4mA when [2:0] = 000;
			 * 8mA when [2:0] = 010;
			 * 12mA when [2:0] = 100;
			 * 16mA when [2:0] = 110;
			 */
			if (!(arg % 4) && (arg >= 4 && arg <= 16)) {
				err = mtk_hw_set_value(hw, pin,
					       PINCTRL_PIN_REG_DRV,
					       (arg/4 - 1) * 2);
				if (err)
					goto err;
			} else {
				err = -ENOTSUPP;
			}
			break;
		case MTK_PIN_CONFIG_TDSEL:
		case MTK_PIN_CONFIG_RDSEL:
			reg = (param == MTK_PIN_CONFIG_TDSEL) ?
			       PINCTRL_PIN_REG_TDSEL : PINCTRL_PIN_REG_RDSEL;

			err = mtk_hw_set_value(hw, pin, reg, arg);
			if (err)
				goto err;
			break;
		default:
			err = -ENOTSUPP;
		}
	}
err:
	return err;
}

static int mtk_pinconf_group_get(struct pinctrl_dev *pctldev,
				 unsigned int group, unsigned long *config)
{
	const unsigned int *pins;
	unsigned int i, npins, old = 0;
	int ret;

	ret = pinctrl_generic_get_group_pins(pctldev, group, &pins, &npins);
	if (ret)
		return ret;

	for (i = 0; i < npins; i++) {
		if (mtk_pinconf_get(pctldev, pins[i], config))
			return -ENOTSUPP;

		/* configs do not match between two pins */
		if (i && old != *config)
			return -ENOTSUPP;

		old = *config;
	}

	return 0;
}

static int mtk_pinconf_group_set(struct pinctrl_dev *pctldev,
				 unsigned int group, unsigned long *configs,
				 unsigned int num_configs)
{
	const unsigned int *pins;
	unsigned int i, npins;
	int ret;

	ret = pinctrl_generic_get_group_pins(pctldev, group, &pins, &npins);
	if (ret)
		return ret;

	for (i = 0; i < npins; i++) {
		ret = mtk_pinconf_set(pctldev, pins[i], configs, num_configs);
		if (ret)
			return ret;
	}

	return 0;
}

static const struct pinctrl_ops mtk_pctlops = {
	.get_groups_count = pinctrl_generic_get_group_count,
	.get_group_name = pinctrl_generic_get_group_name,
	.get_group_pins = pinctrl_generic_get_group_pins,
	.dt_node_to_map = pinconf_generic_dt_node_to_map_all,
	.dt_free_map = pinconf_generic_dt_free_map,
};

static const struct pinmux_ops mtk_pmxops = {
	.get_functions_count = pinmux_generic_get_function_count,
	.get_function_name = pinmux_generic_get_function_name,
	.get_function_groups = pinmux_generic_get_function_groups,
	.set_mux = mtk_pinmux_set_mux,
	.gpio_request_enable = mtk_pinmux_gpio_request_enable,
	.gpio_set_direction = mtk_pinmux_gpio_set_direction,
	.strict = true,
};

static const struct pinconf_ops mtk_confops = {
	.is_generic = true,
	.pin_config_get = mtk_pinconf_get,
	.pin_config_set = mtk_pinconf_set,
	.pin_config_group_get = mtk_pinconf_group_get,
	.pin_config_group_set = mtk_pinconf_group_set,
	.pin_config_config_dbg_show = pinconf_generic_dump_config,
};

static struct pinctrl_desc mtk_desc = {
	.name = PINCTRL_PINCTRL_DEV,
	.pctlops = &mtk_pctlops,
	.pmxops = &mtk_pmxops,
	.confops = &mtk_confops,
	.owner = THIS_MODULE,
};

static int mtk_gpio_get(struct gpio_chip *chip, unsigned int gpio)
{
	struct mtk_pinctrl *hw = container_of(chip, struct mtk_pinctrl, chip);
	int value, err;

	err = mtk_hw_get_value(hw, gpio, PINCTRL_PIN_REG_DI, &value);
	if (err)
		return err;

	return !!value;
}

static void mtk_gpio_set(struct gpio_chip *chip, unsigned int gpio, int value)
{
	struct mtk_pinctrl *hw = container_of(chip, struct mtk_pinctrl, chip);

	mtk_hw_set_value(hw, gpio, PINCTRL_PIN_REG_DO, !!value);
}

static int mtk_gpio_direction_input(struct gpio_chip *chip, unsigned int gpio)
{
	return pinctrl_gpio_direction_input(chip->base + gpio);
}

static int mtk_gpio_direction_output(struct gpio_chip *chip, unsigned int gpio,
				     int value)
{
	mtk_gpio_set(chip, gpio, value);

	return pinctrl_gpio_direction_output(chip->base + gpio);
}

static int mtk_get_gpio_by_eint(unsigned int eint)
{
	int i, gpio_n;

	int size = ARRAY_SIZE(leopard_gpio_to_eint);

	for (i = 0; i < size; i++) {
		if (leopard_gpio_to_eint[i].eintnum == eint)
			gpio_n = leopard_gpio_to_eint[i].gpionum;
	}

	return gpio_n;
}

static int mtk_get_eint_by_gpio(unsigned int gpio)
{
	unsigned int i;
	unsigned int eint_n;

	int size = ARRAY_SIZE(leopard_gpio_to_eint);

	for (i = 0; i < size; i++) {
		if (leopard_gpio_to_eint[i].gpionum == gpio)
			eint_n = leopard_gpio_to_eint[i].eintnum;
	}

	return eint_n;
}

static int mtk_gpio_to_irq(struct gpio_chip *chip, unsigned int offset)
{
	struct mtk_pinctrl *hw = container_of(chip, struct mtk_pinctrl, chip);
	unsigned long eint_n;

	eint_n = mtk_get_eint_by_gpio(offset);

	return mtk_eint_find_irq(hw->eint, eint_n);
}

static int mtk_gpio_set_debounce(struct gpio_chip *chip, unsigned int offset,
			       unsigned int debounce)
{
	struct mtk_pinctrl *hw = container_of(chip, struct mtk_pinctrl, chip);
	unsigned long eint_n;

	eint_n = mtk_get_eint_by_gpio(offset);

	return mtk_eint_set_debounce(hw->eint, eint_n, debounce);
}

static int mtk_build_gpiochip(struct mtk_pinctrl *hw, struct device_node *np)
{
	struct gpio_chip *chip = &hw->chip;
	int ret;

	chip->label		= PINCTRL_PINCTRL_DEV;
	chip->request		= gpiochip_generic_request;
	chip->free		= gpiochip_generic_free;
	chip->direction_input	= mtk_gpio_direction_input;
	chip->direction_output	= mtk_gpio_direction_output;
	chip->get		= mtk_gpio_get;
	chip->set		= mtk_gpio_set;
	chip->to_irq		= mtk_gpio_to_irq;
	chip->set_debounce	= mtk_gpio_set_debounce;
	chip->base		= -1;
	chip->ngpio		= hw->soc->npins;
	chip->of_node		= np;
	chip->of_gpio_n_cells	= 2;

	ret = gpiochip_add(chip);
	if (ret < 0)
		return ret;

	ret = gpiochip_add_pin_range(chip, dev_name(hw->dev), 0, 0,
				     chip->ngpio);
	if (ret < 0) {
		gpiochip_remove(chip);
		return ret;
	}

	return 0;
}

static int mtk_build_groups(struct mtk_pinctrl *hw)
{
	int err, i;

	for (i = 0; i < hw->soc->ngrps; i++) {
		const struct group_desc *group = hw->soc->grps + i;

		err = pinctrl_generic_add_group(hw->pctrl, group->name,
						group->pins, group->num_pins,
						group->data);
		if (err) {
			dev_err(hw->dev, "Failed to register group %s\n",
				group->name);
			return err;
		}
	}

	return 0;
}

static int mtk_build_functions(struct mtk_pinctrl *hw)
{
	int i, err;

	for (i = 0; i < hw->soc->nfuncs ; i++) {
		const struct function_desc *func = hw->soc->funcs + i;

		err = pinmux_generic_add_function(hw->pctrl, func->name,
						  func->group_names,
						  func->num_group_names,
						  func->data);
		if (err) {
			dev_err(hw->dev, "Failed to register function %s\n",
				func->name);
			return err;
		}
	}

	return 0;
}

static int mtk_xt_get_gpio_n(void *data, unsigned long eint_n,
			     unsigned int *gpio_n,
			     struct gpio_chip **gpio_chip)
{
	struct mtk_pinctrl *hw = (struct mtk_pinctrl *)data;

	*gpio_chip = &hw->chip;
	*gpio_n = mtk_get_gpio_by_eint(eint_n);

	return 0;
}

static int mtk_xt_get_gpio_state(void *data, unsigned long eint_n)
{
	struct mtk_pinctrl *hw = (struct mtk_pinctrl *)data;
	struct gpio_chip *gpio_chip;
	unsigned int gpio_n;
	int err;

	err = mtk_xt_get_gpio_n(hw, eint_n, &gpio_n, &gpio_chip);
	if (err)
		return err;

	return mtk_gpio_get(gpio_chip, gpio_n);
}

static int mtk_xt_set_gpio_as_eint(void *data, unsigned long eint_n)
{
	struct mtk_pinctrl *hw = (struct mtk_pinctrl *)data;
	struct gpio_chip *gpio_chip;
	unsigned int gpio_n;
	int err;

	err = mtk_xt_get_gpio_n(hw, eint_n, &gpio_n, &gpio_chip);
	if (err)
		return err;

	err = mtk_hw_set_value(hw, gpio_n, PINCTRL_PIN_REG_MODE,
			       MTK_GPIO_MODE);
	if (err)
		return err;

	err = mtk_hw_set_value(hw, gpio_n, PINCTRL_PIN_REG_DIR, MTK_INPUT);
	if (err)
		return err;

	err = mtk_hw_set_value(hw, gpio_n, PINCTRL_PIN_REG_IES, MTK_ENABLE);
	if (err)
		return err;

	return 0;
}

static const struct mtk_eint_xt mtk_eint_xt = {
	.get_gpio_n = mtk_xt_get_gpio_n,
	.get_gpio_state = mtk_xt_get_gpio_state,
	.set_gpio_as_eint = mtk_xt_set_gpio_as_eint,
};

static int mtk_eint_suspend(struct device *device)
{
	struct mtk_pinctrl *pctl = dev_get_drvdata(device);

	return mtk_eint_do_suspend(pctl->eint);
}

static int mtk_eint_resume(struct device *device)
{
	struct mtk_pinctrl *pctl = dev_get_drvdata(device);

	return mtk_eint_do_resume(pctl->eint);
}

static const struct dev_pm_ops mtk_eint_pm_ops = {
	.suspend_noirq = mtk_eint_suspend,
	.resume_noirq = mtk_eint_resume,
};

static int
mtk_build_eint(struct mtk_pinctrl *hw, struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct resource *res;

	if (!IS_ENABLED(CONFIG_EINT_MTK))
		return 0;

	if (!of_property_read_bool(np, "interrupt-controller"))
		return -ENODEV;

	hw->eint = devm_kzalloc(hw->dev, sizeof(*hw->eint), GFP_KERNEL);
	if (!hw->eint)
		return -ENOMEM;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "eint");
	if (!res) {
		pr_err("Unable to get eint resource\n");
		return -ENODEV;
	}

	hw->eint->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(hw->eint->base))
		return PTR_ERR(hw->eint->base);

	hw->eint->irq = irq_of_parse_and_map(np, 0);
	if (!hw->eint->irq)
		return -EINVAL;

	hw->eint->dev = &pdev->dev;
	hw->eint->hw = hw->soc->eint_hw;
	hw->eint->pctl = hw;
	hw->eint->gpio_xlate = &mtk_eint_xt;

	return mtk_eint_do_init(hw->eint);
}

static const struct of_device_id mtk_pinctrl_of_match[] = {
	{ .compatible = "mediatek,leopard-pinctrl", .data = &leopard_data},
	{ }
};

static int mtk_pinctrl_probe(struct platform_device *pdev)
{
	struct resource *res;
	struct mtk_pinctrl *hw;
	const struct of_device_id *of_id =
		of_match_device(mtk_pinctrl_of_match, &pdev->dev);
	int err;

	hw = devm_kzalloc(&pdev->dev, sizeof(*hw), GFP_KERNEL);
	if (!hw)
		return -ENOMEM;

	hw->soc = of_id->data;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "missing IO resource\n");
		return -ENXIO;
	}

	hw->dev = &pdev->dev;
	hw->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(hw->base))
		return PTR_ERR(hw->base);

	/* Setup pins descriptions per SoC types */
	mtk_desc.pins = hw->soc->pins;
	mtk_desc.npins = hw->soc->npins;
	mtk_desc.num_custom_params = ARRAY_SIZE(mtk_custom_bindings);
	mtk_desc.custom_params = mtk_custom_bindings;
#ifdef CONFIG_DEBUG_FS
	mtk_desc.custom_conf_items = mtk_conf_items;
#endif

	hw->pctrl = pinctrl_register(&mtk_desc, &pdev->dev, hw);
	if (IS_ERR(hw->pctrl))
		return PTR_ERR(hw->pctrl);

	/* Setup groups descriptions per SoC types */
	err = mtk_build_groups(hw);
	if (err) {
		pinctrl_unregister(hw->pctrl);
		dev_err(&pdev->dev, "Failed to build groups\n");
		return 0;
	}

	/* Setup functions descriptions per SoC types */
	err = mtk_build_functions(hw);
	if (err) {
		pinctrl_unregister(hw->pctrl);
		dev_err(&pdev->dev, "Failed to build functions\n");
		return err;
	}

	err = mtk_build_gpiochip(hw, pdev->dev.of_node);
	if (err) {
		pinctrl_unregister(hw->pctrl);
		dev_err(&pdev->dev, "Failed to add gpio_chip\n");
		return err;
	}

	err = mtk_build_eint(hw, pdev);
	if (err)
		pr_err("Failed to add EINT, but pinctrl still can work\n");

	platform_set_drvdata(pdev, hw);

	return 0;
}

static struct platform_driver mtk_pinctrl_driver = {
	.driver = {
		.name = "mtk-pinctrl",
		.of_match_table = mtk_pinctrl_of_match,
		.pm = &mtk_eint_pm_ops,
	},
	.probe = mtk_pinctrl_probe,
};

static int __init mtk_pinctrl_init(void)
{
	return platform_driver_register(&mtk_pinctrl_driver);
}
arch_initcall(mtk_pinctrl_init);
