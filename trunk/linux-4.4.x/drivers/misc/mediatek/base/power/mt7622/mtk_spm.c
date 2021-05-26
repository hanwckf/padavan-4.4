/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/smp.h>
#include <linux/delay.h>
#include <linux/atomic.h>
#include <linux/clk.h>

#include "mtk_spm_idle.h"

/* #include <mach/wd_api.h> */

#include "mtk_spm_internal.h"

#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>

int __attribute__ ((weak))
	get_dynamic_period(int first_use, int first_wakeup_time,
					int battery_capacity_level)
{
	return 0;
}

void __iomem *spm_base;

/* device tree + 32 = IRQ number */
/* 88 + 32 = 120 */
u32 spm_irq_0 = 120;
u32 spm_irq_1 = 121;
u32 spm_irq_2 = 122;
u32 spm_irq_3 = 123;
/* 25MHz crystal divided by 1024 */
u32 spm_rtc_cnt = 24414;

/*
 * Config and Parameter
 */

/*
 * Define and Declare
 */
struct spm_irq_desc {
	unsigned int irq;
	irq_handler_t handler;
};

static twam_handler_t spm_twam_handler;

/*
 * Init and IRQ Function
 */
static irqreturn_t spm_irq0_handler(int irq, void *dev_id)
{
	u32 isr;
	unsigned long flags;
	struct twam_sig twamsig;

	spin_lock_irqsave(&__spm_lock, flags);
	/* get ISR status */
	isr = spm_read(SPM_SLEEP_ISR_STATUS);
	if (isr & ISRS_TWAM) {
		twamsig.sig0 = spm_read(SPM_SLEEP_TWAM_STATUS0);
		twamsig.sig1 = spm_read(SPM_SLEEP_TWAM_STATUS1);
		twamsig.sig2 = spm_read(SPM_SLEEP_TWAM_STATUS2);
		twamsig.sig3 = spm_read(SPM_SLEEP_TWAM_STATUS3);
	}

	/* clean ISR status */
	spm_write(SPM_SLEEP_ISR_MASK, spm_read(SPM_SLEEP_ISR_MASK) |
							ISRM_ALL_EXC_TWAM);
	spm_write(SPM_SLEEP_ISR_STATUS, isr);
	if (isr & ISRS_TWAM)
		udelay(100);	/* need 3T TWAM clock (32K/26M) */
	spm_write(SPM_PCM_SW_INT_CLEAR, PCM_SW_INT0);
	spin_unlock_irqrestore(&__spm_lock, flags);

	if ((isr & ISRS_TWAM) && spm_twam_handler)
		spm_twam_handler(&twamsig);

	if (isr & (ISRS_SW_INT0 | ISRS_PCM_RETURN))
		spm_err("IRQ0 HANDLER SHOULD NOT BE EXECUTED (0x%x)\n", isr);

	return IRQ_HANDLED;
}

static irqreturn_t spm_irq_aux_handler(u32 irq_id)
{
	u32 isr;
	unsigned long flags;

	spin_lock_irqsave(&__spm_lock, flags);
	isr = spm_read(SPM_SLEEP_ISR_STATUS);
	spm_write(SPM_PCM_SW_INT_CLEAR, (1U << irq_id));
	spin_unlock_irqrestore(&__spm_lock, flags);

	spm_err("IRQ%u HANDLER SHOULD NOT BE EXECUTED (0x%x)\n", irq_id, isr);

	return IRQ_HANDLED;
}

static irqreturn_t spm_irq1_handler(int irq, void *dev_id)
{
	return spm_irq_aux_handler(1);
}

static irqreturn_t spm_irq2_handler(int irq, void *dev_id)
{
	return spm_irq_aux_handler(2);
}

static irqreturn_t spm_irq3_handler(int irq, void *dev_id)
{
	return spm_irq_aux_handler(3);
}

static int spm_irq_register(void)
{
	int i, err, r = 0;
	struct spm_irq_desc irqdesc[] = {
		{.irq = 0, .handler = spm_irq0_handler,},
		{.irq = 0, .handler = spm_irq1_handler,},
		{.irq = 0, .handler = spm_irq2_handler,},
		{.irq = 0, .handler = spm_irq3_handler,}
	};

	irqdesc[0].irq = SPM_IRQ0_ID;
	irqdesc[1].irq = SPM_IRQ1_ID;
	irqdesc[2].irq = SPM_IRQ2_ID;
	irqdesc[3].irq = SPM_IRQ3_ID;

	for (i = 0; i < ARRAY_SIZE(irqdesc); i++) {
		err = request_irq(irqdesc[i].irq, irqdesc[i].handler,
				  IRQF_TRIGGER_LOW | IRQF_NO_SUSPEND, "SPM",
								NULL);
		if (err) {
			spm_err("FAILED TO REQUEST IRQ%d (%d)\n", i, err);
			r = -EPERM;
		}
	}
#if 0
	mt_gic_set_priority(SPM_IRQ0_ID);
#endif
	return r;
}

static void spm_register_init(void)
{
	unsigned long flags;
	struct device_node *node;
	struct clk *spm_rtc_clk;

	node = of_find_compatible_node(NULL, NULL, "mediatek,mt7622-scpsys");
	if (!node)
		spm_err("find mt7622-scpsys node failed\n");
	spm_base = of_iomap(node, 0);
	if (!spm_base)
		spm_err("base spm_base failed\n");

	spm_irq_0 = irq_of_parse_and_map(node, 0);
	if (!spm_irq_0)
		spm_err("get spm_irq_0 failed\n");
	spm_irq_1 = irq_of_parse_and_map(node, 1);
	if (!spm_irq_1)
		spm_err("get spm_irq_1 failed\n");
	spm_irq_2 = irq_of_parse_and_map(node, 2);
	if (!spm_irq_2)
		spm_err("get spm_irq_2 failed\n");
	spm_irq_3 = irq_of_parse_and_map(node, 3);
	if (!spm_irq_3)
		spm_err("get spm_irq_3 failed\n");

	spm_rtc_clk = of_clk_get_by_name(node, "spm_rtc");
	if (!spm_rtc_clk)
		spm_err("get spm_rtc clk failed\n");

	spm_err("spm_base = %p\n", spm_base);
	spm_err("spm_irq_0 = %d, spm_irq_1 = %d\n", spm_irq_0, spm_irq_1);
	spm_err("spm_irq_2 = %d, spm_irq_3 = %d\n", spm_irq_2, spm_irq_3);

	if (spm_irq_0) {
		spm_err("set spm as wakeup devcie.\n");
		irq_set_irq_wake(spm_irq_0, 1);
	}

	if (spm_rtc_clk) {
		spm_rtc_cnt = clk_get_rate(spm_rtc_clk);
		spm_info("spm_rtc cnt: %d.\n", spm_rtc_cnt);
	}

	spin_lock_irqsave(&__spm_lock, flags);

	/* enable register control */
	spm_write(SPM_POWERON_CONFIG_SET, SPM_REGWR_CFG_KEY | SPM_REGWR_EN);

	/* init power control register */
	spm_write(SPM_POWER_ON_VAL0, 0);
	spm_write(SPM_POWER_ON_VAL1, POWER_ON_VAL1_DEF);
	spm_write(SPM_PCM_PWR_IO_EN, 0);

	/* reset PCM */
	spm_write(SPM_PCM_CON0, CON0_CFG_KEY | CON0_PCM_SW_RESET);
	spm_write(SPM_PCM_CON0, CON0_CFG_KEY);
	/* PCM reset failed */
	/* BUG_ON(spm_read(SPM_PCM_FSM_STA) != PCM_FSM_STA_DEF); */

	/* init PCM control register */
	spm_write(SPM_PCM_CON0, CON0_CFG_KEY | CON0_IM_SLEEP_DVS);
	spm_write(SPM_PCM_CON1, CON1_CFG_KEY | CON1_EVENT_LOCK_EN |
		  CON1_SPM_SRAM_ISO_B | CON1_SPM_SRAM_SLP_B | CON1_MIF_APBEN);
	spm_write(SPM_PCM_IM_PTR, 0);
	spm_write(SPM_PCM_IM_LEN, 0);

	spm_write(SPM_CLK_CON, spm_read(SPM_CLK_CON) | CC_SRCLKENA_MASK_0);
	/* CC_CLKSQ0_SEL is DONT-CARE in Suspend since PCM_PWR_IO_EN[0]=1
	 * in Suspend
	 */

	spm_write(SPM_PCM_SRC_REQ, 0);

	/* TODO: check if this means "Set SRCLKENI_MASK=1'b1" */
	spm_write(SPM_AP_STANBY_CON, spm_read(SPM_AP_STANBY_CON) |
						ASC_SRCCLKENI_MASK);

	/* unmask gce_busy_mask (set to 1b1); otherwise,
	 * gce (cmd-q) can not notify SPM to exit EMI self-refresh
	 */
	spm_write(SPM_PCM_MMDDR_MASK, spm_read(SPM_PCM_MMDDR_MASK) | (1U << 4));

	/* clean ISR status */
	spm_write(SPM_SLEEP_ISR_MASK, ISRM_ALL);
	spm_write(SPM_SLEEP_ISR_STATUS, ISRC_ALL);
	spm_write(SPM_PCM_SW_INT_CLEAR, PCM_SW_INT_ALL);

	spin_unlock_irqrestore(&__spm_lock, flags);
}

static int __init spm_module_init(void)
{
	int r = 0;

	spm_register_init();

	if (spm_irq_register() != 0)
		r = -EPERM;

#if defined(CONFIG_PM)
	if (spm_fs_init() != 0)
		r = -EPERM;
#endif

	return r;
}
postcore_initcall(spm_module_init);

/*
 * PLL Request API
 */
void spm_mainpll_on_request(const char *drv_name)
{
	int req;

	req = atomic_inc_return(&__spm_mainpll_req);
	spm_debug("%s request MAINPLL on (%d)\n", drv_name, req);
}
EXPORT_SYMBOL(spm_mainpll_on_request);

void spm_mainpll_on_unrequest(const char *drv_name)
{
	int req;

	req = atomic_dec_return(&__spm_mainpll_req);
	spm_debug("%s unrequest MAINPLL on (%d)\n", drv_name, req);
}
EXPORT_SYMBOL(spm_mainpll_on_unrequest);


/*
 * TWAM Control API
 */
void spm_twam_set_idle_select(unsigned int sel)
{
}
EXPORT_SYMBOL(spm_twam_set_idle_select);

void spm_twam_register_handler(twam_handler_t handler)
{
	spm_twam_handler = handler;
}
EXPORT_SYMBOL(spm_twam_register_handler);

static unsigned int window_len;
void spm_twam_set_window_length(unsigned int len)
{
	window_len = len;
}
EXPORT_SYMBOL(spm_twam_set_window_length);

static struct twam_sig mon_type;
void spm_twam_set_mon_type(struct twam_sig *mon)
{
	if (mon) {
		mon_type.sig0 = mon->sig0 & 0x3;
		mon_type.sig1 = mon->sig1 & 0x3;
		mon_type.sig2 = mon->sig2 & 0x3;
		mon_type.sig3 = mon->sig3 & 0x3;
	}
}
EXPORT_SYMBOL(spm_twam_set_mon_type);

void spm_twam_enable_monitor(const struct twam_sig *twamsig, bool speed_mode)
{
	u32 sig0 = 0, sig1 = 0, sig2 = 0, sig3 = 0;
	u32 mon0 = 0, mon1 = 0, mon2 = 0, mon3 = 0;
	unsigned int length;
	unsigned long flags;
	unsigned int spm_twam_con_val;

	if (twamsig) {
		sig0 = twamsig->sig0 & 0x1f;
		sig1 = twamsig->sig1 & 0x1f;
		sig2 = twamsig->sig2 & 0x1f;
		sig3 = twamsig->sig3 & 0x1f;
	}

	/* Window length */
	length = window_len;
	/* Monitor type */
	mon0 = mon_type.sig0 & 0x3;
	mon1 = mon_type.sig1 & 0x3;
	mon2 = mon_type.sig2 & 0x3;
	mon3 = mon_type.sig3 & 0x3;

	spin_lock_irqsave(&__spm_lock, flags);
	spm_write(SPM_SLEEP_ISR_MASK, spm_read(SPM_SLEEP_ISR_MASK) &
							~ISRM_TWAM);
	/* Monitor Control */
	spm_twam_con_val = ((sig3 << 27) |
			(sig2 << 22) |
			(sig1 << 17) |
			(sig0 << 12) |
			(mon3 << 10) |
			(mon2 << 8) |
			(mon1 << 6) |
			(mon0 << 4) | TWAM_CON_EN);
	if (speed_mode)
		spm_twam_con_val |= (0x1 << 1);
	else
		spm_twam_con_val &= ~(0x1 << 1);

	spm_write(SPM_SLEEP_TWAM_CON, spm_twam_con_val);

	/* Window Length */
	/* 0x13DDF0 for 50ms, 0x65B8 for 1ms, 0x1458 for 200us,
	 * 0xA2C for 100us
	 */
	/* in speed mode (26 MHz) */
	spm_write(SPM_SLEEP_TWAM_WINDOW_LEN, length);
	spin_unlock_irqrestore(&__spm_lock, flags);

	spm_crit("enable TWAM for signal %u, %u, %u, %u (%u)\n",
		 sig0, sig1, sig2, sig3, speed_mode);


}
EXPORT_SYMBOL(spm_twam_enable_monitor);

void spm_twam_disable_monitor(void)
{
	unsigned long flags;

	spin_lock_irqsave(&__spm_lock, flags);
	spm_write(SPM_SLEEP_TWAM_CON, spm_read(SPM_SLEEP_TWAM_CON) &
							~TWAM_CON_EN);
	spm_write(SPM_SLEEP_ISR_MASK, spm_read(SPM_SLEEP_ISR_MASK) | ISRM_TWAM);
	spm_write(SPM_SLEEP_ISR_STATUS, ISRC_TWAM);
	spin_unlock_irqrestore(&__spm_lock, flags);

	spm_debug("disable TWAM\n");
}
EXPORT_SYMBOL(spm_twam_disable_monitor);

#define SPM_WAKE_PERIOD         600	/* sec */
u32 spm_get_wake_period(int pwake_time, wake_reason_t last_wr)
{
	int period = SPM_WAKE_PERIOD;

#if 1
	if (pwake_time < 0) {
		/* use FG to get the period of 1% battery decrease */
		period = get_dynamic_period(last_wr != WR_PCM_TIMER ? 1 : 0,
							SPM_WAKE_PERIOD, 1);
		if (period <= 0) {
			spm_warn("CANNOT GET PERIOD FROM FUEL GAUGE\n");
			period = SPM_WAKE_PERIOD;
		}
	} else {
		period = pwake_time;
		spm_crit2("pwake = %d\n", pwake_time);
	}

	if (period > 36 * 3600)	/* max period is 36.4 hours */
		period = 36 * 3600;
#endif
	return period;
}

#define SPM_CPU_PWR_STA_MASK	0x3000
unsigned int spm_get_cpu_pwr_status(void)
{
	u32 val[2] = {0};
	u32 stat = 0;

	val[0] = spm_read(SPM_PWR_STATUS);
	val[1] = spm_read(SPM_PWR_STATUS_2ND);

	stat = val[0] & SPM_CPU_PWR_STA_MASK;
	stat &= val[1] & SPM_CPU_PWR_STA_MASK;

	return stat;
}
EXPORT_SYMBOL(spm_get_cpu_pwr_status);

MODULE_DESCRIPTION("SPM Driver v0.1");
