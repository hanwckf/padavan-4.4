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
#include <linux/irqchip.h>
#include <linux/irqchip/mips-gic.h>

int get_c0_perfcount_int(void)
{
	return gic_get_c0_perfcount_int();
}
EXPORT_SYMBOL_GPL(get_c0_perfcount_int);

void __init
arch_init_irq(void)
{
	irqchip_init();
}
