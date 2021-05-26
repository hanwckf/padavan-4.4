/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include "mt_spm_mtcmos.h"
#include "mt_spm.h"
#include "mt-smp.h"

/*
 * for CPU MTCMOS
 */
/*
 * regiser bit definition
 */
/* SPM_MP0_CPU1_PWR_CON */
#define PD_SLPB_CLAMP   (1U << 7)
#define SRAM_ISOINT_B   (1U << 6)
#define SRAM_CKISO      (1U << 5)
#define PWR_CLK_DIS     (1U << 4)
#define PWR_ON_2ND      (1U << 3)
#define PWR_ON          (1U << 2)
#define PWR_ISO         (1U << 1)
#define PWR_RST_B       (1U << 0)

/* SPM_MP0_CPU1_L1_PDN */
#define MP0_CPU1_L1_PDN_ACK      (1U << 8)
#define MP0_CPU1_L1_PDN          (1U << 0)

/* SPM_PWR_STATUS */
/* SPM_PWR_STATUS_S */
#define SPM_PWR_STATUS_CPU1       (1U << 12)
#define SPM_PWR_STATUS_CPUTOP0    (1U <<  9)

/* SPM_SLEEP_TIMER_STA */
#define MP0_CPU1_STANDBYWFI    (1U << 17)

static DEFINE_SPINLOCK(spm_cpu_lock);

void __iomem *spm_base;

int spm_mtcmos_cpu_init(void)
{
	struct device_node *node;
	static int init;

	if (init)
		return 0;

	node = of_find_compatible_node(NULL, NULL, "mediatek,leopard-scpsys");
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

int spm_mtcmos_ctrl_cpu1(int state, int check_wfi_before_pdn)
{
	unsigned long flags;

	/* enable register control */
	spm_write(SPM_POWERON_CONFIG_SET, (SPM_PROJECT_CODE << 16) | (1U << 0));

	if (state == STA_POWER_DOWN) {
		if (check_wfi_before_pdn) {
			while ((spm_read(SPM_SLEEP_TIMER_STA) &
				MP0_CPU1_STANDBYWFI) == 0)
				;
		}

		spm_mtcmos_cpu_lock(&flags);

		spm_write(SPM_MP0_CPU1_PWR_CON,
			  spm_read(SPM_MP0_CPU1_PWR_CON) |
			  PWR_ISO | PD_SLPB_CLAMP);
		spm_write(SPM_MP0_CPU1_PWR_CON,
			  spm_read(SPM_MP0_CPU1_PWR_CON) | SRAM_CKISO);

		spm_write(SPM_MP0_CPU1_L1_PDN,
			  spm_read(SPM_MP0_CPU1_L1_PDN) | MP0_CPU1_L1_PDN);
		spm_write(SPM_MP0_CPU1_PWR_CON,
			  (spm_read(SPM_MP0_CPU1_PWR_CON) & ~PWR_RST_B));
		spm_write(SPM_MP0_CPU1_PWR_CON,
			  (spm_read(SPM_MP0_CPU1_PWR_CON) | PWR_CLK_DIS));

		spm_write(SPM_MP0_CPU1_PWR_CON,
			  spm_read(SPM_MP0_CPU1_PWR_CON) &
			  ~PWR_ON & ~PWR_ON_2ND);
		while (((spm_read(SPM_PWR_STATUS) &
			 SPM_PWR_STATUS_CPU1) != 0) ||
		       ((spm_read(SPM_PWR_STATUS_S) &
			 SPM_PWR_STATUS_CPU1) != 0))
			;

		spm_mtcmos_cpu_unlock(&flags);
	} else {		/* STA_POWER_ON */
		spm_mtcmos_cpu_lock(&flags);

		spm_write(SPM_MP0_CPU1_PWR_CON,
			  spm_read(SPM_MP0_CPU1_PWR_CON) & ~PWR_RST_B);
		spm_write(SPM_MP0_CPU1_PWR_CON,
			  spm_read(SPM_MP0_CPU1_PWR_CON) | PWR_CLK_DIS);

		spm_write(SPM_MP0_CPU1_PWR_CON,
			  spm_read(SPM_MP0_CPU1_PWR_CON) | PWR_ON);
		spm_write(SPM_MP0_CPU1_PWR_CON,
			  spm_read(SPM_MP0_CPU1_PWR_CON) | PWR_ON_2ND);

		while (((spm_read(SPM_PWR_STATUS) & SPM_PWR_STATUS_CPU1) !=
			SPM_PWR_STATUS_CPU1) ||
			((spm_read(SPM_PWR_STATUS_S) & SPM_PWR_STATUS_CPU1) !=
			 SPM_PWR_STATUS_CPU1))
			;

		spm_write(SPM_MP0_CPU1_PWR_CON,
			  spm_read(SPM_MP0_CPU1_PWR_CON) &
			  ~PWR_ISO & ~PD_SLPB_CLAMP);

		spm_write(SPM_MP0_CPU1_L1_PDN,
			  spm_read(SPM_MP0_CPU1_L1_PDN) & ~MP0_CPU1_L1_PDN);
		while ((spm_read(SPM_MP0_CPU1_L1_PDN) &
			MP0_CPU1_L1_PDN_ACK) != 0)
			;

		udelay(1); /* Wait 1000ns for memory power ready */

		spm_write(SPM_MP0_CPU1_PWR_CON,
			  spm_read(SPM_MP0_CPU1_PWR_CON) & ~SRAM_CKISO);
		spm_write(SPM_MP0_CPU1_PWR_CON,
			  spm_read(SPM_MP0_CPU1_PWR_CON) & ~PWR_CLK_DIS);
		spm_write(SPM_MP0_CPU1_PWR_CON,
			  spm_read(SPM_MP0_CPU1_PWR_CON) | PWR_RST_B);

		spm_mtcmos_cpu_unlock(&flags);
	}

	return 0;
}
