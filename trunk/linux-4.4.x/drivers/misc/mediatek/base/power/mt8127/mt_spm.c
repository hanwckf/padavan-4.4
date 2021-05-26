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
#include <linux/interrupt.h>
#include <linux/delay.h>

#if 0
#include <mach/wd_api.h>
#endif

#include "mt_spm.h"

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#endif

#ifdef CONFIG_OF
void __iomem *spm_base;
u32 spm_irq_0 = 117;
u32 spm_irq_1 = 118;
#endif

/**********************************************************
 * PCM code for normal (v2 @ 2013-04-26)
 **********************************************************/
static const u32 __pcm_normal[] = {
	0x1840001f, 0x00000001, 0x1b00001f, 0x00202000, 0x1b80001f, 0x80001000,
	0x8880000c, 0x00200000, 0xd80001e2, 0x17c07c1f, 0xe8208000, 0x100063e0,
	0x00000002, 0x1b80001f, 0x00001000, 0x809c840d, 0xd8200042, 0x17c07c1f,
	0xa1d78407, 0x1890001f, 0x10006014, 0x18c0001f, 0x10006014, 0xa0978402,
	0xe0c00002, 0x1b80001f, 0x00001000, 0xf0000000
};

static const pcm_desc_t pcm_normal = {
	.base = __pcm_normal,
	.size = 28,
	.sess = 1,
};


/**************************************
 * SW code for general and normal
 **************************************/
#define WAKE_SRC_FOR_NORMAL     (WAKE_SRC_THERM)

DEFINE_SPINLOCK(spm_lock);

static twam_handler_t spm_twam_handler;

void spm_go_to_normal(void)
{
	unsigned long flags;

	spin_lock_irqsave(&spm_lock, flags);
	/* reset PCM */
	spm_write(SPM_PCM_CON0, CON0_CFG_KEY | CON0_PCM_SW_RESET);
	spm_write(SPM_PCM_CON0, CON0_CFG_KEY);

	/* init PCM_CON1 (disable non-replace mode) */
	spm_write(SPM_PCM_CON1, CON1_CFG_KEY | CON1_SPM_SRAM_ISO_B |
		  CON1_SPM_SRAM_SLP_B | CON1_MIF_APBEN);

	/* tell IM where is PCM code */
	spm_write(SPM_PCM_IM_PTR, spm_get_base_phys(pcm_normal.base));
	spm_write(SPM_PCM_IM_LEN, pcm_normal.size - 1);

	/* unmask wakeup source */
	spm_write(SPM_SLEEP_WAKEUP_EVENT_MASK, ~WAKE_SRC_FOR_NORMAL);

	/* kick IM and PCM to run */
	spm_write(SPM_PCM_CON0, CON0_CFG_KEY | CON0_IM_KICK | CON0_PCM_KICK);
	spm_write(SPM_PCM_CON0, CON0_CFG_KEY);
	spin_unlock_irqrestore(&spm_lock, flags);
}

static irqreturn_t spm_irq_handler(int irq, void *dev_id)
{
	u32 isr;
	twam_sig_t twamsig;

	spin_lock(&spm_lock);
	/* get ISR status */
	isr = spm_read(SPM_SLEEP_ISR_STATUS);
	if (isr & ISR_TWAM) {
		twamsig.sig0 = spm_read(SPM_SLEEP_TWAM_STATUS0);
		twamsig.sig1 = spm_read(SPM_SLEEP_TWAM_STATUS1);
		twamsig.sig2 = spm_read(SPM_SLEEP_TWAM_STATUS2);
		twamsig.sig3 = spm_read(SPM_SLEEP_TWAM_STATUS3);
	}

	/* clean ISR status */
	spm_write(SPM_SLEEP_ISR_MASK, spm_read(SPM_SLEEP_ISR_MASK) | ISRM_ALL_EXC_TWAM);
	spm_write(SPM_SLEEP_ISR_STATUS, isr);
	if (isr & ISR_TWAM)
		udelay(100);	/* need 3T TWAM clock */
	spm_write(SPM_PCM_SW_INT_CLEAR, PCM_SW_INT0);
	spin_unlock(&spm_lock);

	if (isr == ISR_TWAM && spm_twam_handler)
		spm_twam_handler(&twamsig);
	else
		spm_err("ISR SHOULD NOT BE EXECUTED (0x%x)\n", isr);

	return IRQ_HANDLED;
}

static int __init spm_module_init(void)
{
	int r;
	unsigned long flags;
#if 0
	struct wd_api *wd_api;
#endif
#ifdef CONFIG_OF
	struct device_node *node;

	node = of_find_compatible_node(NULL, NULL, "mediatek,mt2701-scpsys");
	if (!node)
		spm_err("find mt2701-scpsys node failed\n");
	spm_base = of_iomap(node, 0);
	if (!spm_base)
		spm_err("base spm_base failed\n");

	spm_irq_0 = irq_of_parse_and_map(node, 0);
	if (!spm_irq_0)
		spm_err("get spm_irq_0 failed\n");
	spm_irq_1 = irq_of_parse_and_map(node, 1);
	if (!spm_irq_1)
		spm_err("get spm_irq_1 failed\n");

	spm_err("spm_base = %p, spm_irq_0 = %d, spm_irq_1 = %d\n", spm_base, spm_irq_0, spm_irq_1);
#endif

	/* set irq unmask in the progress of
	 * suspend flow.
	 */
	if (spm_irq_0)
		irq_set_irq_wake(spm_irq_0, 1);

	spin_lock_irqsave(&spm_lock, flags);
	/* enable register control */
	spm_write(SPM_POWERON_CONFIG_SET, (SPM_PROJECT_CODE << 16) | (1U << 0));

	/* init power control register (select PCM clock to 26M) */
	spm_write(SPM_POWER_ON_VAL0, 0);
	spm_write(SPM_POWER_ON_VAL1, 0x00015820);
	spm_write(SPM_PCM_PWR_IO_EN, 0);

	/* reset PCM */
	spm_write(SPM_PCM_CON0, CON0_CFG_KEY | CON0_PCM_SW_RESET);
	spm_write(SPM_PCM_CON0, CON0_CFG_KEY);

	/* init PCM control register */
	spm_write(SPM_PCM_CON0, CON0_CFG_KEY | CON0_IM_SLEEP_DVS);
	spm_write(SPM_PCM_CON1, CON1_CFG_KEY | CON1_SPM_SRAM_ISO_B |
		  CON1_SPM_SRAM_SLP_B | CON1_IM_NONRP_EN | CON1_MIF_APBEN);
	spm_write(SPM_PCM_IM_PTR, 0);
	spm_write(SPM_PCM_IM_LEN, 0);

	/* SRCLKENA: POWER_ON_VAL1 (PWR_IO_EN[7]=0) or POWER_ON_VAL1|r7 (PWR_IO_EN[7]=1) */
	/* CLKSQ: POWER_ON_VAL0 (PWR_IO_EN[0]=0) or r0 (PWR_IO_EN[0]=1) */
	/* SRCLKENAI will trigger 26M-wake/sleep event */
	spm_write(SPM_CLK_CON, CC_CXO32K_RM_EN_MD);
	spm_write(SPM_PCM_SRC_REQ, (1U << 1));

	/* clean wakeup event raw status */
	spm_write(SPM_SLEEP_WAKEUP_EVENT_MASK, 0xffffffff);

	/* clean ISR status */
	spm_write(SPM_SLEEP_ISR_MASK, ISRM_ALL);
	spm_write(SPM_SLEEP_ISR_STATUS, ISRC_ALL);
	spm_write(SPM_PCM_SW_INT_CLEAR, PCM_SW_INT_ALL);
	spin_unlock_irqrestore(&spm_lock, flags);

	r = request_irq(SPM_IRQ0_ID, spm_irq_handler, IRQF_TRIGGER_LOW | IRQF_NO_SUSPEND,
			"mt-spm", NULL);
	if (r) {
		spm_err("FAILED TO REQUEST SPM IRQ (%d)\n", r);
		WARN_ON(1);
	}

#if 0
	get_wd_api(&wd_api);
	if (wd_api->wd_spmwdt_mode_config && wd_api->wd_thermal_mode_config) {
		wd_api->wd_spmwdt_mode_config(WD_REQ_EN, WD_REQ_RST_MODE);
		wd_api->wd_thermal_mode_config(WD_REQ_EN, WD_REQ_RST_MODE);
	} else {
		spm_err("FAILED TO GET WD API\n");
		WARN_ON(1);
	}
#endif
	spm_go_to_normal();	/* let PCM help to do thermal protection */

	return 0;
}
postcore_initcall(spm_module_init);

/**************************************
 * for TWAM to integrate with MET
 **************************************/
void spm_twam_register_handler(twam_handler_t handler)
{
	spm_twam_handler = handler;
}
EXPORT_SYMBOL(spm_twam_register_handler);

void spm_twam_enable_monitor(twam_sig_t *twamsig, bool speed_mode)
{
	u32 con = TWAM_CON_EN;
	unsigned long flags;

	if (speed_mode)
		con |= TWAM_CON_SPEED_EN;

	if (twamsig) {
		con |= (twamsig->sig0 & 0xf) << 16;
		con |= (twamsig->sig1 & 0xf) << 20;
		con |= (twamsig->sig2 & 0xf) << 24;
		con |= (twamsig->sig3 & 0xf) << 28;
	}

	spin_lock_irqsave(&spm_lock, flags);
	spm_write(SPM_SLEEP_ISR_MASK, spm_read(SPM_SLEEP_ISR_MASK) & ~ISR_TWAM);
	spm_write(SPM_SLEEP_TWAM_CON, con);
	spin_unlock_irqrestore(&spm_lock, flags);
}
EXPORT_SYMBOL(spm_twam_enable_monitor);

void spm_twam_disable_monitor(void)
{
	unsigned long flags;

	spin_lock_irqsave(&spm_lock, flags);
	spm_write(SPM_SLEEP_TWAM_CON, spm_read(SPM_SLEEP_TWAM_CON) & ~TWAM_CON_EN);
	spm_write(SPM_SLEEP_ISR_MASK, spm_read(SPM_SLEEP_ISR_MASK) | ISR_TWAM);
	spm_write(SPM_SLEEP_ISR_STATUS, ISR_TWAM);
	spin_unlock_irqrestore(&spm_lock, flags);
}
EXPORT_SYMBOL(spm_twam_disable_monitor);


/**************************************
 * for PCM WDT to replace RGU WDT
 **************************************/
#if 0
int spm_wdt_register_fiq(fiq_isr_handler rgu_wdt_handler)
{
	int r;

#ifdef CONFIG_FIQ_GLUE
	r = request_fiq(MT_SPM1_IRQ_ID, rgu_wdt_handler, IRQF_TRIGGER_FALLING, NULL);
#else
	r = -EPERM;
#endif

	return r;
}

int spm_wdt_register_irq(irq_handler_t rgu_wdt_handler)
{
	int r;

	r = request_irq(SPM_IRQ1_ID, rgu_wdt_handler, IRQF_TRIGGER_FALLING, "mt-spm1", NULL);

	return r;
}
#endif

void spm_wdt_set_timeout(u32 sec)
{
	unsigned long flags;

	spin_lock_irqsave(&spm_lock, flags);
	WARN_ON(sec > 36 * 3600);
	spm_write(SPM_PCM_WDT_TIMER_VAL, sec * 32768);
	spin_unlock_irqrestore(&spm_lock, flags);
}

void spm_wdt_enable_timer(void)
{
	unsigned long flags;

	spin_lock_irqsave(&spm_lock, flags);
	spm_write(SPM_PCM_CON1, spm_read(SPM_PCM_CON1) | CON1_CFG_KEY |
		  CON1_PCM_WDT_WAKE_MODE | CON1_PCM_WDT_EN);
	spin_unlock_irqrestore(&spm_lock, flags);
}

void spm_wdt_restart_timer(void)
{
	unsigned long flags;

	spin_lock_irqsave(&spm_lock, flags);
	spm_write(SPM_POWER_ON_VAL1, spm_read(SPM_POWER_ON_VAL1) | R7_WDT_KICK_P);
	spm_write(SPM_POWER_ON_VAL1, spm_read(SPM_POWER_ON_VAL1) & ~R7_WDT_KICK_P);
	spin_unlock_irqrestore(&spm_lock, flags);
}

void spm_wdt_restart_timer_nolock(void)
{
	spm_write(SPM_POWER_ON_VAL1, spm_read(SPM_POWER_ON_VAL1) | R7_WDT_KICK_P);
	spm_write(SPM_POWER_ON_VAL1, spm_read(SPM_POWER_ON_VAL1) & ~R7_WDT_KICK_P);
}

void spm_wdt_disable_timer(void)
{
	unsigned long flags;

	spin_lock_irqsave(&spm_lock, flags);
	spm_write(SPM_POWER_ON_VAL1, spm_read(SPM_POWER_ON_VAL1) | R7_WDT_KICK_P);
	spm_write(SPM_POWER_ON_VAL1, spm_read(SPM_POWER_ON_VAL1) & ~R7_WDT_KICK_P);

	spm_write(SPM_PCM_CON1, CON1_CFG_KEY | (spm_read(SPM_PCM_CON1) & ~CON1_PCM_WDT_EN));
	spm_write(SPM_PCM_SW_INT_CLEAR, PCM_SW_INT1);
	spin_unlock_irqrestore(&spm_lock, flags);
}


/**************************************
 * for CPU DVFS
 **************************************/
#define MAX_RETRY_COUNT (100)

int spm_dvfs_ctrl_volt(u32 value)
{
	u32 ap_dvfs_con;
	int retry = 0;

#if !defined(CONFIG_MTK_PMIC_WRAP) && !defined(CONFIG_MTK_PMIC_WRAP_COMMON)
	return 0;
#endif
	ap_dvfs_con = spm_read(SPM_AP_DVFS_CON_SET);
	spm_write(SPM_AP_DVFS_CON_SET, (ap_dvfs_con & ~(0x7)) | value);
	udelay(5);

	while ((spm_read(SPM_AP_DVFS_CON_SET) & (0x1 << 31)) == 0) {
		if (retry >= MAX_RETRY_COUNT) {
			spm_crit("FAIL: no response from PMIC wrapper\n");
			return -1;
		}

		retry++;
		spm_crit("wait for ACK signal from PMIC wrapper, retry = %d\n", retry);

		udelay(5);
	}

	return 0;
}

MODULE_DESCRIPTION("SPM Driver v0.6");
