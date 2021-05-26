/*
 * Copyright (c) 2014-2015 MediaTek Inc.
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

#ifndef __ARCH_ARM_MACH_BOARD_H
#define __ARCH_ARM_MACH_BOARD_H

#define MSDC_CD_PIN_EN      BIT(0)  /* card detection pin is wired   */
#define MSDC_WP_PIN_EN      BIT(1)  /* write protection pin is wired */
#define MSDC_RST_PIN_EN     BIT(2)  /* emmc reset pin is wired       */
#define MSDC_REMOVABLE      BIT(5)  /* removable slot                */

#define MSDC_SMPL_RISING    (0)
#define MSDC_SMPL_FALLING   (1)

#define MSDC_CMD_PIN        (0)
#define MSDC_DAT_PIN        (1)
#define MSDC_CD_PIN         (2)
#define MSDC_WP_PIN         (3)
#define MSDC_RST_PIN        (4)

struct msdc_hw {
	unsigned char  clk_src;          /* host clock source */
	unsigned long  flags;            /* hardware capability flags */

	/* config gpio pull mode */
	void (*config_gpio_pin)(int type, int pull);
};

extern struct msdc_hw msdc0_hw;

#endif /* __ARCH_ARM_MACH_BOARD_H */
