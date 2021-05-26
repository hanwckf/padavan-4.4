/*
 * Copyright (c) 2015 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <mach/mt_spm_mtcmos.h>
#include "mt_spm.h"

/*
 * for CPU MTCMOS
 */
/*
 * regiser bit definition
 */
/* SPM_FC1_PWR_CON */
/* SPM_FC2_PWR_CON */
/* SPM_FC3_PWR_CON */
#define SRAM_ISOINT_B   (1U << 6)
#define SRAM_CKISO      (1U << 5)
#define PWR_CLK_DIS     (1U << 4)
#define PWR_ON_S        (1U << 3)
#define PWR_ON          (1U << 2)
#define PWR_ISO         (1U << 1)
#define PWR_RST_B       (1U << 0)

/* SPM_CPU_FC1_L1_PDN */
/* SPM_CPU_FC2_L1_PDN */
/* SPM_CPU_FC3_L1_PDN */
#define L1_PDN_ACK      (1U << 8)
#define L1_PDN          (1U << 0)

/* SPM_PWR_STATUS */
/* SPM_PWR_STATUS_S */
#define FC1             (1U << 11)
#define FC2             (1U << 10)
#define FC3             (1U <<  9)

/* SPM_SLEEP_TIMER_STA */
#define APMCU3_SLEEP    (1U << 18)
#define APMCU2_SLEEP    (1U << 17)
#define APMCU1_SLEEP    (1U << 16)

static DEFINE_SPINLOCK(spm_cpu_lock);

int spm_mtcmos_cpu_init(void)
{
	struct device_node *node;
	static int init;

	if (init)
		return 0;

	node = of_find_compatible_node(NULL, NULL, "mediatek,mt8127-scpsys");
	if (!node) {
		pr_err("find SLEEP node failed\n");
		return -EINVAL;
	}
	spm_base = of_iomap(node, 0);
	if (!spm_base) {
		pr_err("base spm_base failed\n");
		return -EINVAL;
	}
	init = 1;

	return 0;
}

void spm_mtcmos_cpu_lock(unsigned long *flags)
{
	spin_lock_irqsave(&spm_cpu_lock, *flags);
}

void spm_mtcmos_cpu_unlock(unsigned long *flags)
{
	spin_unlock_irqrestore(&spm_cpu_lock, *flags);
}

typedef int (*spm_cpu_mtcmos_ctrl_func) (int state, int chkWfiBeforePdn);
static spm_cpu_mtcmos_ctrl_func spm_cpu_mtcmos_ctrl_funcs[] = {
	spm_mtcmos_ctrl_cpu0,
	spm_mtcmos_ctrl_cpu1,
	spm_mtcmos_ctrl_cpu2,
	spm_mtcmos_ctrl_cpu3,
};

int spm_mtcmos_ctrl_cpu(unsigned int cpu, int state, int chkWfiBeforePdn)
{
	return (*spm_cpu_mtcmos_ctrl_funcs[cpu]) (state, chkWfiBeforePdn);
}

int spm_mtcmos_ctrl_cpu0(int state, int chkWfiBeforePdn)
{
	return 0;
}

int spm_mtcmos_ctrl_cpu1(int state, int chkWfiBeforePdn)
{
	unsigned long flags;

	/* enable register control */
	spm_write(SPM_POWERON_CONFIG_SET, (SPM_PROJECT_CODE << 16) | (1U << 0));

	if (state == STA_POWER_DOWN) {
		if (chkWfiBeforePdn)
			while ((spm_read(SPM_SLEEP_TIMER_STA) &	APMCU1_SLEEP) == 0)
				;

		spm_mtcmos_cpu_lock(&flags);

		spm_write(SPM_FC1_PWR_CON, spm_read(SPM_FC1_PWR_CON) | SRAM_CKISO);
		spm_write(SPM_FC1_PWR_CON, spm_read(SPM_FC1_PWR_CON) & ~SRAM_ISOINT_B);

		spm_write(SPM_CPU_FC1_L1_PDN, spm_read(SPM_CPU_FC1_L1_PDN) | L1_PDN);
		while ((spm_read(SPM_CPU_FC1_L1_PDN) & L1_PDN_ACK) != L1_PDN_ACK)
			;

		spm_write(SPM_FC1_PWR_CON, spm_read(SPM_FC1_PWR_CON) | PWR_ISO);
		spm_write(SPM_FC1_PWR_CON, (spm_read(SPM_FC1_PWR_CON) | PWR_CLK_DIS) & ~PWR_RST_B);

		spm_write(SPM_FC1_PWR_CON, spm_read(SPM_FC1_PWR_CON) & ~PWR_ON);
		spm_write(SPM_FC1_PWR_CON, spm_read(SPM_FC1_PWR_CON) & ~PWR_ON_S);
		while (((spm_read(SPM_PWR_STATUS) & FC1) != 0) | ((spm_read(SPM_PWR_STATUS_S) & FC1) != 0))
			;

		spm_mtcmos_cpu_unlock(&flags);
	} else {		/* STA_POWER_ON */

		spm_mtcmos_cpu_lock(&flags);

		spm_write(SPM_FC1_PWR_CON, spm_read(SPM_FC1_PWR_CON) | PWR_ON);
		udelay(1);
		spm_write(SPM_FC1_PWR_CON, spm_read(SPM_FC1_PWR_CON) | PWR_ON_S);
		while (((spm_read(SPM_PWR_STATUS) & FC1) != FC1) | ((spm_read(SPM_PWR_STATUS_S) & FC1) != FC1))
			;

		spm_write(SPM_FC1_PWR_CON, spm_read(SPM_FC1_PWR_CON) & ~PWR_CLK_DIS);

		spm_write(SPM_CPU_FC1_L1_PDN, spm_read(SPM_CPU_FC1_L1_PDN) & ~L1_PDN);
		while ((spm_read(SPM_CPU_FC1_L1_PDN) & L1_PDN_ACK) != 0)
			;

		spm_write(SPM_FC1_PWR_CON, spm_read(SPM_FC1_PWR_CON) | SRAM_ISOINT_B);
		spm_write(SPM_FC1_PWR_CON, spm_read(SPM_FC1_PWR_CON) & ~SRAM_CKISO);

		spm_write(SPM_FC1_PWR_CON, spm_read(SPM_FC1_PWR_CON) & ~PWR_ISO);
		spm_write(SPM_FC1_PWR_CON, spm_read(SPM_FC1_PWR_CON) | PWR_RST_B);

		spm_mtcmos_cpu_unlock(&flags);
	}

	return 0;
}

int spm_mtcmos_ctrl_cpu2(int state, int chkWfiBeforePdn)
{
	unsigned long flags;

	/* enable register control */
	spm_write(SPM_POWERON_CONFIG_SET, (SPM_PROJECT_CODE << 16) | (1U << 0));

	if (state == STA_POWER_DOWN) {
		if (chkWfiBeforePdn)
			while ((spm_read(SPM_SLEEP_TIMER_STA) & APMCU2_SLEEP) == 0)
				;

		spm_mtcmos_cpu_lock(&flags);

		spm_write(SPM_FC2_PWR_CON, spm_read(SPM_FC2_PWR_CON) | SRAM_CKISO);
		spm_write(SPM_FC2_PWR_CON, spm_read(SPM_FC2_PWR_CON) & ~SRAM_ISOINT_B);

		spm_write(SPM_CPU_FC2_L1_PDN, spm_read(SPM_CPU_FC2_L1_PDN) | L1_PDN);
		while ((spm_read(SPM_CPU_FC2_L1_PDN) & L1_PDN_ACK) != L1_PDN_ACK)
			;

		spm_write(SPM_FC2_PWR_CON, spm_read(SPM_FC2_PWR_CON) | PWR_ISO);
		spm_write(SPM_FC2_PWR_CON, (spm_read(SPM_FC2_PWR_CON) | PWR_CLK_DIS) & ~PWR_RST_B);

		spm_write(SPM_FC2_PWR_CON, spm_read(SPM_FC2_PWR_CON) & ~PWR_ON);
		spm_write(SPM_FC2_PWR_CON, spm_read(SPM_FC2_PWR_CON) & ~PWR_ON_S);
		while (((spm_read(SPM_PWR_STATUS) & FC2) != 0) | ((spm_read(SPM_PWR_STATUS_S) & FC2) != 0))
			;

		spm_mtcmos_cpu_unlock(&flags);
	} else {		/* STA_POWER_ON */

		spm_mtcmos_cpu_lock(&flags);

		spm_write(SPM_FC2_PWR_CON, spm_read(SPM_FC2_PWR_CON) | PWR_ON);
		udelay(1);
		spm_write(SPM_FC2_PWR_CON, spm_read(SPM_FC2_PWR_CON) | PWR_ON_S);
		while (((spm_read(SPM_PWR_STATUS) & FC2) != FC2) | ((spm_read(SPM_PWR_STATUS_S) & FC2) != FC2))
			;

		spm_write(SPM_FC2_PWR_CON, spm_read(SPM_FC2_PWR_CON) & ~PWR_CLK_DIS);

		spm_write(SPM_CPU_FC2_L1_PDN, spm_read(SPM_CPU_FC2_L1_PDN) & ~L1_PDN);
		while ((spm_read(SPM_CPU_FC2_L1_PDN) & L1_PDN_ACK) != 0)
			;

		spm_write(SPM_FC2_PWR_CON, spm_read(SPM_FC2_PWR_CON) | SRAM_ISOINT_B);
		spm_write(SPM_FC2_PWR_CON, spm_read(SPM_FC2_PWR_CON) & ~SRAM_CKISO);

		spm_write(SPM_FC2_PWR_CON, spm_read(SPM_FC2_PWR_CON) & ~PWR_ISO);
		spm_write(SPM_FC2_PWR_CON, spm_read(SPM_FC2_PWR_CON) | PWR_RST_B);

		spm_mtcmos_cpu_unlock(&flags);
	}

	return 0;
}

int spm_mtcmos_ctrl_cpu3(int state, int chkWfiBeforePdn)
{
	unsigned long flags;

	/* enable register control */
	spm_write(SPM_POWERON_CONFIG_SET, (SPM_PROJECT_CODE << 16) | (1U << 0));

	if (state == STA_POWER_DOWN) {
		if (chkWfiBeforePdn)
			while ((spm_read(SPM_SLEEP_TIMER_STA) &
				APMCU3_SLEEP) == 0)
				;

		spm_mtcmos_cpu_lock(&flags);

		spm_write(SPM_FC3_PWR_CON, spm_read(SPM_FC3_PWR_CON) | SRAM_CKISO);
		spm_write(SPM_FC3_PWR_CON, spm_read(SPM_FC3_PWR_CON) & ~SRAM_ISOINT_B);
		spm_write(SPM_CPU_FC3_L1_PDN, spm_read(SPM_CPU_FC3_L1_PDN) | L1_PDN);
		while ((spm_read(SPM_CPU_FC3_L1_PDN) & L1_PDN_ACK) != L1_PDN_ACK)
			;

		spm_write(SPM_FC3_PWR_CON, spm_read(SPM_FC3_PWR_CON) | PWR_ISO);
		spm_write(SPM_FC3_PWR_CON, (spm_read(SPM_FC3_PWR_CON) | PWR_CLK_DIS) & ~PWR_RST_B);

		spm_write(SPM_FC3_PWR_CON, spm_read(SPM_FC3_PWR_CON) & ~PWR_ON);
		spm_write(SPM_FC3_PWR_CON, spm_read(SPM_FC3_PWR_CON) & ~PWR_ON_S);
		while (((spm_read(SPM_PWR_STATUS) & FC3) != 0) | ((spm_read(SPM_PWR_STATUS_S) & FC3) != 0))
			;

		spm_mtcmos_cpu_unlock(&flags);
	} else {		/* STA_POWER_ON */

		spm_mtcmos_cpu_lock(&flags);

		spm_write(SPM_FC3_PWR_CON, spm_read(SPM_FC3_PWR_CON) | PWR_ON);
		udelay(1);
		spm_write(SPM_FC3_PWR_CON, spm_read(SPM_FC3_PWR_CON) | PWR_ON_S);
		while (((spm_read(SPM_PWR_STATUS) & FC3) != FC3) | ((spm_read(SPM_PWR_STATUS_S) & FC3) != FC3))
			;

		spm_write(SPM_FC3_PWR_CON, spm_read(SPM_FC3_PWR_CON) & ~PWR_CLK_DIS);

		spm_write(SPM_CPU_FC3_L1_PDN, spm_read(SPM_CPU_FC3_L1_PDN) & ~L1_PDN);
		while ((spm_read(SPM_CPU_FC3_L1_PDN) & L1_PDN_ACK) != 0)
			;

		spm_write(SPM_FC3_PWR_CON, spm_read(SPM_FC3_PWR_CON) | SRAM_ISOINT_B);
		spm_write(SPM_FC3_PWR_CON, spm_read(SPM_FC3_PWR_CON) & ~SRAM_CKISO);

		spm_write(SPM_FC3_PWR_CON, spm_read(SPM_FC3_PWR_CON) & ~PWR_ISO);
		spm_write(SPM_FC3_PWR_CON, spm_read(SPM_FC3_PWR_CON) | PWR_RST_B);

		spm_mtcmos_cpu_unlock(&flags);
	}

	return 0;
}

int spm_mtcmos_ctrl_cpu4(int state, int chkWfiBeforePdn)
{
	return 0;
}
int spm_mtcmos_ctrl_cpu5(int state, int chkWfiBeforePdn)
{
	return 0;
}
int spm_mtcmos_ctrl_cpu6(int state, int chkWfiBeforePdn)
{
	return 0;
}
int spm_mtcmos_ctrl_cpu7(int state, int chkWfiBeforePdn)
{
	return 0;
}
int spm_mtcmos_ctrl_cpusys0(int state, int chkWfiBeforePdn)
{
	return 0;
}

bool spm_cpusys0_can_power_down(void)
{
	return !(spm_read(SPM_PWR_STATUS) & (FC1 | FC2 | FC3)) &&
	!(spm_read(SPM_PWR_STATUS_S) & (FC1 | FC2 | FC3));
}
