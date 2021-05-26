/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 *
 * Copyright (C) 2008-2011 Gabor Juhos <juhosg@openwrt.org>
 * Copyright (C) 2008 Imre Kaloz <kaloz@openwrt.org>
 * Copyright (C) 2013 John Crispin <blogic@openwrt.org>
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/clk-provider.h>
#include <linux/cpumask.h>
#include <linux/sizes.h>

#include <asm/time.h>
#include <asm/mipsregs.h>
#include <asm/smp-ops.h>
#include <asm/mips-cm.h>
#include <asm/mips-cpc.h>
#include <asm/mach-ralink/ralink_regs.h>
#include <asm/mach-ralink/mt7621.h>
#include <asm/delay.h>
#include <asm/div64.h>
#include <asm/bootinfo.h>

#include <pinmux.h>

#include "common.h"

#define SYSC_REG_CPLL_CLKCFG0	0x2c
#define CPU_CLK_SEL_MASK	0x03
#define CPU_CLK_SEL_SHIFT	30

#define SYSC_REG_CUR_CLK_STS	0x44
#define CUR_CPU_FDIV_MASK	0x1f
#define CUR_CPU_FDIV_SHIFT	8
#define CUR_CPU_FFRAC_MASK	0x1f
#define CUR_CPU_FFRAC_SHIFT	0

#define DRAMC_REG_MPLL18	0x648
#define CPLL_PREDIV_MASK	0x03
#define CPLL_PREDIV_SHIFT	12
#define CPLL_FBDIV_MASK		0x7f
#define CPLL_FBDIV_SHIFT	4

#define MT7621_GPIO_MODE_UART1		1
#define MT7621_GPIO_MODE_I2C		2
#define MT7621_GPIO_MODE_UART3_MASK	0x3
#define MT7621_GPIO_MODE_UART3_SHIFT	3
#define MT7621_GPIO_MODE_UART3_GPIO	1
#define MT7621_GPIO_MODE_UART2_MASK	0x3
#define MT7621_GPIO_MODE_UART2_SHIFT	5
#define MT7621_GPIO_MODE_UART2_GPIO	1
#define MT7621_GPIO_MODE_JTAG		7
#define MT7621_GPIO_MODE_WDT_MASK	0x3
#define MT7621_GPIO_MODE_WDT_SHIFT	8
#define MT7621_GPIO_MODE_WDT_GPIO	1
#define MT7621_GPIO_MODE_PCIE_RST	0
#define MT7621_GPIO_MODE_PCIE_REF	2
#define MT7621_GPIO_MODE_PCIE_MASK	0x3
#define MT7621_GPIO_MODE_PCIE_SHIFT	10
#define MT7621_GPIO_MODE_PCIE_GPIO	1
#define MT7621_GPIO_MODE_MDIO_MASK	0x3
#define MT7621_GPIO_MODE_MDIO_SHIFT	12
#define MT7621_GPIO_MODE_MDIO_GPIO	1
#define MT7621_GPIO_MODE_RGMII1		14
#define MT7621_GPIO_MODE_RGMII2		15
#define MT7621_GPIO_MODE_SPI_MASK	0x3
#define MT7621_GPIO_MODE_SPI_SHIFT	16
#define MT7621_GPIO_MODE_SPI_GPIO	1
#define MT7621_GPIO_MODE_SDHCI_MASK	0x3
#define MT7621_GPIO_MODE_SDHCI_SHIFT	18
#define MT7621_GPIO_MODE_SDHCI_GPIO	1

static struct rt2880_pmx_func uart1_grp[] =  { FUNC("uart1", 0, 1, 2) };
static struct rt2880_pmx_func i2c_grp[] =  { FUNC("i2c", 0, 3, 2) };
static struct rt2880_pmx_func uart3_grp[] = {
	FUNC("uart3", 0, 5, 4),
	FUNC("i2s", 2, 5, 4),
	FUNC("spdif3", 3, 5, 4),
};
static struct rt2880_pmx_func uart2_grp[] = {
	FUNC("uart2", 0, 9, 4),
	FUNC("pcm", 2, 9, 4),
	FUNC("spdif2", 3, 9, 4),
};
static struct rt2880_pmx_func jtag_grp[] = { FUNC("jtag", 0, 13, 5) };
static struct rt2880_pmx_func wdt_grp[] = {
	FUNC("wdt rst", 0, 18, 1),
	FUNC("wdt refclk", 2, 18, 1),
};
static struct rt2880_pmx_func pcie_rst_grp[] = {
	FUNC("pcie rst", MT7621_GPIO_MODE_PCIE_RST, 19, 1),
	FUNC("pcie refclk", MT7621_GPIO_MODE_PCIE_REF, 19, 1)
};
static struct rt2880_pmx_func mdio_grp[] = { FUNC("mdio", 0, 20, 2) };
static struct rt2880_pmx_func rgmii2_grp[] = { FUNC("rgmii2", 0, 22, 12) };
static struct rt2880_pmx_func spi_grp[] = {
	FUNC("spi", 0, 34, 7),
	FUNC("nand1", 2, 34, 7),
};
static struct rt2880_pmx_func sdhci_grp[] = {
	FUNC("sdhci", 0, 41, 8),
	FUNC("nand2", 2, 41, 8),
};
static struct rt2880_pmx_func rgmii1_grp[] = { FUNC("rgmii1", 0, 49, 12) };

static struct rt2880_pmx_group mt7621_pinmux_data[] = {
	GRP("uart1", uart1_grp, 1, MT7621_GPIO_MODE_UART1),
	GRP("i2c", i2c_grp, 1, MT7621_GPIO_MODE_I2C),
	GRP_G("uart3", uart3_grp, MT7621_GPIO_MODE_UART3_MASK,
		MT7621_GPIO_MODE_UART3_GPIO, MT7621_GPIO_MODE_UART3_SHIFT),
	GRP_G("uart2", uart2_grp, MT7621_GPIO_MODE_UART2_MASK,
		MT7621_GPIO_MODE_UART2_GPIO, MT7621_GPIO_MODE_UART2_SHIFT),
	GRP("jtag", jtag_grp, 1, MT7621_GPIO_MODE_JTAG),
	GRP_G("wdt", wdt_grp, MT7621_GPIO_MODE_WDT_MASK,
		MT7621_GPIO_MODE_WDT_GPIO, MT7621_GPIO_MODE_WDT_SHIFT),
	GRP_G("pcie", pcie_rst_grp, MT7621_GPIO_MODE_PCIE_MASK,
		MT7621_GPIO_MODE_PCIE_GPIO, MT7621_GPIO_MODE_PCIE_SHIFT),
	GRP_G("mdio", mdio_grp, MT7621_GPIO_MODE_MDIO_MASK,
		MT7621_GPIO_MODE_MDIO_GPIO, MT7621_GPIO_MODE_MDIO_SHIFT),
	GRP("rgmii2", rgmii2_grp, 1, MT7621_GPIO_MODE_RGMII2),
	GRP_G("spi", spi_grp, MT7621_GPIO_MODE_SPI_MASK,
		MT7621_GPIO_MODE_SPI_GPIO, MT7621_GPIO_MODE_SPI_SHIFT),
	GRP_G("sdhci", sdhci_grp, MT7621_GPIO_MODE_SDHCI_MASK,
		MT7621_GPIO_MODE_SDHCI_GPIO, MT7621_GPIO_MODE_SDHCI_SHIFT),
	GRP("rgmii1", rgmii1_grp, 1, MT7621_GPIO_MODE_RGMII1),
	{ 0 }
};

static unsigned long cpu_rate;

static void __init mt7621_detect_memory_region(struct ralink_soc_info *soc_info);
static void *detect_magic __initdata = mt7621_detect_memory_region;

phys_addr_t mips_cpc_default_phys_base(void)
{
	panic("Cannot detect cpc address");
}

void __init ralink_clk_init(void)
{
	int cpu_fdiv, cpu_ffrac;
	int mc_fb, mc_div;
	u32 clk_sts, mc_cpll;
	u8 clk_sel, xtal_mode, mc_prediv_sel;
	u64 cpu_clk;
	u32 xtal_clk;

	xtal_mode = (rt_sysc_r32(SYSC_REG_SYSTEM_CONFIG0) >> XTAL_MODE_SEL_SHIFT)
		    & XTAL_MODE_SEL_MASK;

	if (xtal_mode <= 2)
		xtal_clk = 20 * 1000 * 1000;
	else if (xtal_mode <= 5)
		xtal_clk = 40 * 1000 * 1000;
	else
		xtal_clk = 25 * 1000 * 1000;

	clk_sel = (rt_sysc_r32(SYSC_REG_CPLL_CLKCFG0) >> CPU_CLK_SEL_SHIFT)
		  & CPU_CLK_SEL_MASK;

	switch (clk_sel) {
	case 0:
		cpu_clk = 500 * 1000 * 1000;
		break;
	case 1:
		mc_cpll = rt_memc_r32(DRAMC_REG_MPLL18);
		mc_fb = (mc_cpll >> CPLL_FBDIV_SHIFT) & CPLL_FBDIV_MASK;
		mc_prediv_sel = (mc_cpll >> CPLL_PREDIV_SHIFT)
				& CPLL_PREDIV_MASK;

		switch (mc_prediv_sel) {
		case 0:
			mc_div = 1;
			break;
		case 1:
			mc_div = 2;
			break;
		default:
			mc_div = 4;
		}

		cpu_clk = xtal_clk * (mc_fb + 1);
		do_div(cpu_clk, mc_div);
		break;
	default:
		cpu_clk = xtal_clk;
	}

	clk_sts = rt_sysc_r32(SYSC_REG_CUR_CLK_STS);
	cpu_fdiv = ((clk_sts >> CUR_CPU_FDIV_SHIFT) & CUR_CPU_FDIV_MASK);
	cpu_ffrac = ((clk_sts >> CUR_CPU_FFRAC_SHIFT) & CUR_CPU_FFRAC_MASK);

	cpu_clk *= cpu_ffrac;
	do_div(cpu_clk, cpu_fdiv);

	if (cpu_clk >> 32)
		pr_notice("Invalid CPU clock: %lluHz\n", cpu_clk);

	cpu_rate = cpu_clk & 0xffffffff;

	mips_hpt_frequency = cpu_rate / 2;
}

void __init ralink_of_remap(void)
{
	rt_sysc_membase = plat_of_remap_node("mtk,mt7621-sysc");
	rt_memc_membase = plat_of_remap_node("mtk,mt7621-memc");

	if (!rt_sysc_membase || !rt_memc_membase)
		panic("Failed to remap core resources");
}

bool plat_cpu_core_present(int core)
{
	void __iomem *sysc = (void __iomem *) KSEG1ADDR(MT7621_SYSC_BASE);
	u32 rev = __raw_readl(sysc + SYSC_REG_CHIP_REV);

	if (!core)
		return true;

	if (core > 1)
		return false;

	if ((rev >> CHIP_CPU_ID_SHIFT) & CHIP_CPU_ID_MASK)
		return true;

	return false;
}

static int udelay_fixup(void)
{
	unsigned int i;

	for (i = 1; i < num_present_cpus(); i++)
		cpu_data[i].udelay_val = cpu_data[0].udelay_val;

	return 0;
}

postcore_initcall(udelay_fixup);

static void __init mt7621_detect_memory_region(struct ralink_soc_info *soc_info)
{
	void *dm = &detect_magic;
	phys_addr_t size;

	for (size = MT7621_DRAM_SIZE_MIN * SZ_1M;
	     size < MT7621_DRAM_SIZE_MAX * SZ_1M;
	     size <<= 1) {
		if (!memcmp(dm, dm + size, sizeof(detect_magic)))
			break;
	}

	if (size < SZ_512M) {
		add_memory_region(0, size, BOOT_MEM_RAM);
	} else {
		add_memory_region(0, SZ_512M - SZ_64M, BOOT_MEM_RAM);
		add_memory_region(SZ_512M, SZ_64M, BOOT_MEM_RAM);
	}
}

void prom_soc_init(struct ralink_soc_info *soc_info)
{
	void __iomem *sysc = (void __iomem *) KSEG1ADDR(MT7621_SYSC_BASE);
	unsigned char *name = NULL;
	u32 n0;
	u32 n1;
	u32 rev;

	n0 = __raw_readl(sysc + SYSC_REG_CHIP_NAME0);
	n1 = __raw_readl(sysc + SYSC_REG_CHIP_NAME1);

	if (n0 == MT7621_CHIP_NAME0 && n1 == MT7621_CHIP_NAME1) {
		name = "MT7621";
		soc_info->compatible = "mtk,mt7621-soc";
	} else {
		panic("mt7621: unknown SoC, n0:%08x n1:%08x\n", n0, n1);
	}
	ralink_soc = MT762X_SOC_MT7621AT;
	rev = __raw_readl(sysc + SYSC_REG_CHIP_REV);

	snprintf(soc_info->sys_type, RAMIPS_SYS_TYPE_LEN,
		"MediaTek %s ver:%u eco:%u",
		name,
		(rev >> CHIP_REV_VER_SHIFT) & CHIP_REV_VER_MASK,
		(rev & CHIP_REV_ECO_MASK));

	soc_info->mem_size_min = MT7621_DRAM_SIZE_MIN;
	soc_info->mem_size_max = MT7621_DRAM_SIZE_MAX;
	soc_info->mem_base = MT7621_DRAM_BASE;
	soc_info->mem_detect = mt7621_detect_memory_region;

	rt2880_pinmux_data = mt7621_pinmux_data;

	/* Early detection of CMP support */
	mips_cm_probe();
	mips_cpc_probe();

	if (mips_cm_numiocu()) {
		/*
		 * mips_cm_probe() wipes out bootloader
		 * config for CM regions and we have to configure them
		 * again. This SoC cannot talk to pamlbus devices
		 * witout proper iocu region set up.

		 * FIXME: it would be better to do this with values
		 * from DT, but we need this very early because
		 * without this we cannot talk to pretty much anything
		 * including serial.
		 */
		write_gcr_reg0_base(MT7621_PALMBUS_BASE);
		write_gcr_reg0_mask(~MT7621_PALMBUS_SIZE | CM_GCR_REGn_MASK_CMTGT_IOCU0);
	}

	if (!register_cps_smp_ops())
		return;
	if (!register_cmp_smp_ops())
		return;
	if (!register_vsmp_smp_ops())
		return;
}

static void of_mt7621_cpu_clk_setup(struct device_node *node)
{
	struct clk *clk;
	const char *clk_name = node->name;

	of_property_read_string(node, "clock-output-names", &clk_name);

	clk = clk_register_fixed_rate_with_accuracy(NULL, clk_name, NULL,
						    CLK_IS_ROOT, cpu_rate,
						    0);
	if (!IS_ERR(clk))
		of_clk_add_provider(node, of_clk_src_simple_get, clk);
}

static void of_mt7621_sys_bus_clk_setup(struct device_node *node)
{
	struct clk *clk;
	const char *clk_name = node->name;

	of_property_read_string(node, "clock-output-names", &clk_name);

	clk = clk_register_fixed_rate_with_accuracy(NULL, clk_name, NULL,
						    CLK_IS_ROOT, cpu_rate / 4,
						    0);
	if (!IS_ERR(clk))
		of_clk_add_provider(node, of_clk_src_simple_get, clk);
}

CLK_OF_DECLARE(mt7621_cpu_clock, "mtk,mt7621-cpu-clock",
				 of_mt7621_cpu_clk_setup);

CLK_OF_DECLARE(mt7621_sys_bus_clock, "mtk,mt7621-sys-bus-clock",
				 of_mt7621_sys_bus_clk_setup);
