/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 *
 * Copyright (C) 2009 Gabor Juhos <juhosg@openwrt.org>
 * Copyright (C) 2013 John Crispin <blogic@openwrt.org>
 */

#include <linux/init.h>

#include <linux/of.h>
#include <linux/clk-provider.h>
#include <linux/clocksource.h>
#include <asm/time.h>

#include "common.h"

void __init plat_time_init(void)
{
	ralink_of_remap();

	ralink_clk_init();

	of_clk_init(NULL);
	clocksource_probe();
}
