/*
 * R8A7740 processor support
 *
 * Copyright (C) 2011  Renesas Solutions Corp.
 * Copyright (C) 2011  Kuninori Morimoto <kuninori.morimoto.gx@renesas.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/irqchip.h>
#include <linux/irqchip/arm-gic.h>
#include <linux/of_platform.h>

#include <asm/mach/map.h>
#include <asm/mach/arch.h>
#include <asm/mach/time.h>
#include <asm/hardware/cache-l2x0.h>

#include "common.h"

static struct map_desc r8a7740_io_desc[] __initdata = {
	 /*
	  * for CPGA/INTC/PFC
	  * 0xe6000000-0xefffffff -> 0xe6000000-0xefffffff
	  */
	{
		.virtual	= 0xe6000000,
		.pfn		= __phys_to_pfn(0xe6000000),
		.length		= 160 << 20,
		.type		= MT_DEVICE_NONSHARED
	},
#ifdef CONFIG_CACHE_L2X0
	/*
	 * for l2x0_init()
	 * 0xf0100000-0xf0101000 -> 0xf0002000-0xf0003000
	 */
	{
		.virtual	= 0xf0002000,
		.pfn		= __phys_to_pfn(0xf0100000),
		.length		= PAGE_SIZE,
		.type		= MT_DEVICE_NONSHARED
	},
#endif
};

static void __init r8a7740_map_io(void)
{
	debug_ll_io_init();
	iotable_init(r8a7740_io_desc, ARRAY_SIZE(r8a7740_io_desc));
}

/*
 * r8a7740 chip has lasting errata on MERAM buffer.
 * this is work-around for it.
 * see
 *	"Media RAM (MERAM)" on r8a7740 documentation
 */
#define MEBUFCNTR	0xFE950098
static void __init r8a7740_meram_workaround(void)
{
	void __iomem *reg;

	reg = ioremap_nocache(MEBUFCNTR, 4);
	if (reg) {
		iowrite32(0x01600164, reg);
		iounmap(reg);
	}
}

static void __init r8a7740_init_irq_of(void)
{
	void __iomem *intc_prio_base = ioremap_nocache(0xe6900010, 0x10);
	void __iomem *intc_msk_base = ioremap_nocache(0xe6900040, 0x10);
	void __iomem *pfc_inta_ctrl = ioremap_nocache(0xe605807c, 0x4);

	irqchip_init();

	/* route signals to GIC */
	iowrite32(0x0, pfc_inta_ctrl);

	/*
	 * To mask the shared interrupt to SPI 149 we must ensure to set
	 * PRIO *and* MASK. Else we run into IRQ floods when registering
	 * the intc_irqpin devices
	 */
	iowrite32(0x0, intc_prio_base + 0x0);
	iowrite32(0x0, intc_prio_base + 0x4);
	iowrite32(0x0, intc_prio_base + 0x8);
	iowrite32(0x0, intc_prio_base + 0xc);
	iowrite8(0xff, intc_msk_base + 0x0);
	iowrite8(0xff, intc_msk_base + 0x4);
	iowrite8(0xff, intc_msk_base + 0x8);
	iowrite8(0xff, intc_msk_base + 0xc);

	iounmap(intc_prio_base);
	iounmap(intc_msk_base);
	iounmap(pfc_inta_ctrl);
}

static void __init r8a7740_generic_init(void)
{
	r8a7740_meram_workaround();

#ifdef CONFIG_CACHE_L2X0
	/* Shared attribute override enable, 32K*8way */
	l2x0_init(IOMEM(0xf0002000), 0x00400000, 0xc20f0fff);
#endif
	of_platform_populate(NULL, of_default_bus_match_table, NULL, NULL);
}

static const char *const r8a7740_boards_compat_dt[] __initconst = {
	"renesas,r8a7740",
	NULL,
};

DT_MACHINE_START(R8A7740_DT, "Generic R8A7740 (Flattened Device Tree)")
	.map_io		= r8a7740_map_io,
	.init_early	= shmobile_init_delay,
	.init_irq	= r8a7740_init_irq_of,
	.init_machine	= r8a7740_generic_init,
	.init_late	= shmobile_init_late,
	.dt_compat	= r8a7740_boards_compat_dt,
MACHINE_END
