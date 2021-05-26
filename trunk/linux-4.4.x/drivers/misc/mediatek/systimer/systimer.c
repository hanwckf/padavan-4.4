/*
 * Copyright (C) 2017 MediaTek Inc.
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
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/irqreturn.h>
#include <linux/clockchips.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>


#define CNTTVAL_EN      (0x1 << 0)
#define CNTIRQ_EN       (0x1 << 1)
#define CNTIRQ_STACLR   (0x1 << 4)

#define OFFSET_CNTVAL_CON_REG  (0x40)
#define OFFSET_CNTTVAL_REG     (0x44)

#define systimer_reg_read(addr)          readl((void __iomem *)addr)
#define systimer_reg_write(addr, val)    writel(val, (void __iomem *)addr)

#define MAX_NR_SYSTIMER        (0x08)
#define SYSTIMER_IN_USE        (0x01)

struct system_timer {
	unsigned short id;	/* from 0, 1, 2, 3... */
	unsigned short irq;
	unsigned int flags;
	unsigned long cntcon_reg;  /* virtual address */
	unsigned long cnttval_reg; /* virtual address */
	void (*func)(unsigned long);
	unsigned long data;
};

struct mtk_systimer_device {
	unsigned int nr_timers;
	unsigned int freq;
	void __iomem *base_addr;
	struct system_timer systimer[MAX_NR_SYSTIMER];
};

static struct mtk_systimer_device *mtk_systimer;

static DEFINE_SPINLOCK(systimer_lock);
#define systimer_spin_lock(flags)    spin_lock_irqsave(&systimer_lock, flags)
#define systimer_spin_unlock(flags)  spin_unlock_irqrestore(&systimer_lock, flags)


/* Clear systimer[id] irq status and disable enable_irq bit */
static inline void __systimer_ack_irq(void *dev_id)
{
	unsigned int ctl;
	struct system_timer *dev = dev_id;

	ctl = systimer_reg_read(dev->cntcon_reg);
	ctl &= ~(CNTIRQ_EN);
	ctl |= (CNTIRQ_STACLR);
	systimer_reg_write(dev->cntcon_reg, ctl);
}

static irqreturn_t systimer_interrupt(int irq, void *dev_id)
{
	struct system_timer *dev = dev_id;

	/* Acknowledge interrupt first */
	__systimer_ack_irq(dev_id);

	if (dev->func)
		dev->func(dev->data);

	return IRQ_HANDLED;
}

static inline unsigned int us_to_cycle(unsigned int us)
{
	unsigned int freq = mtk_systimer->freq;

	if (freq >= USEC_PER_SEC)
		return ((freq / USEC_PER_SEC) * us);
	else if (freq * us >= USEC_PER_SEC)
		return ((freq * us) / USEC_PER_SEC);
	else
		return 1;
}

static struct system_timer *__find_idle_systimer(void)
{
	int i;
	unsigned long save_flags;
	struct mtk_systimer_device *sdev;
	struct system_timer *find_dev = NULL;

	sdev = mtk_systimer;
	if (!sdev)
		return NULL;

	systimer_spin_lock(save_flags);
	for (i = 0; i < sdev->nr_timers; i++) {
		if (!(sdev->systimer[i].flags & SYSTIMER_IN_USE)) {
			find_dev = &sdev->systimer[i];
			break;
		}
	}
	systimer_spin_unlock(save_flags);

	return find_dev;
}

static void __systimer_set_handler(struct system_timer *dev,
				void (*func)(unsigned long),
				unsigned long data)
{
	if (!dev)
		return;

	dev->func = func;
	dev->data = data;
}

/**
 *	request_systimer - request one systimer
 *	@timer_id: output value, timer_id must be globally unique;
 *		a pointer to a integer where the timer id is to be stored.
 *	@func: timer interrupt handler which is defined by user
 *	@data: input parameter for interrupt func
 *
 * Used to request one systimer for timer interrupt defined by user.
 */
int request_systimer(unsigned int *timer_id,
				void (*func)(unsigned long),
				unsigned long data)
{
	unsigned long save_flags;
	struct system_timer *dev;

	if (!timer_id)
		return -EINVAL;

	dev = __find_idle_systimer();
	if (!dev) {
		pr_err("%s: all systimers are in use!\n", __func__);
		return -EBUSY;
	}

	systimer_spin_lock(save_flags);
	if (func)
		__systimer_set_handler(dev, func, data);
	dev->flags |= SYSTIMER_IN_USE;
	/*  User ID from 1, 2, 3..... */
	*timer_id = dev->id + 1;
	systimer_spin_unlock(save_flags);
#if 1
	/* Set the requested systiemr irq affinity for each cpu-core */
	if (irq_set_affinity(dev->irq, cpumask_of(dev->id))) {
		pr_warn("unable to set irq affinity (irq=%u, cpu=%u)\n",
			dev->irq, dev->id);
	}
#endif
	pr_debug("%s: systimer%d is applied for use!\n", __func__, *timer_id);

	return 0;
}
EXPORT_SYMBOL_GPL(request_systimer);

/**
 *	free_systimer - release a systimer
 *	@timer_id: systimer id to free
 *
 * Used to free one systimer device allocated with request_systimer().
 */
int free_systimer(unsigned int timer_id)
{
	unsigned long save_flags;
	struct mtk_systimer_device *sdev;
	struct system_timer *dev;
	unsigned int id;

	sdev = mtk_systimer;
	if (!sdev)
		return -ENXIO;

	id = timer_id;
	if (id < 1 || id > sdev->nr_timers) {
		pr_err("%s: systimer%d is not exist!\n", __func__, id);
		return -EINVAL;
	}

	dev = &sdev->systimer[id - 1];
	systimer_spin_lock(save_flags);
	dev->flags &= ~SYSTIMER_IN_USE;
	__systimer_set_handler(dev, NULL, 0x0);
	systimer_spin_unlock(save_flags);

	return 0;
}
EXPORT_SYMBOL_GPL(free_systimer);

/**
 *	systimer_start - start a systimer
 *	@timer_id: systimer id to use
 *	@usec: time interval in unit of microseconds
 *
 * Used to start a systimer with XXX us expired.
 * This function may be called from interrupt context.
 */
int systimer_start(unsigned int timer_id, unsigned int usec)
{
	struct mtk_systimer_device *sdev;
	struct system_timer *dev;
	unsigned int id;
	unsigned int expires;

	sdev = mtk_systimer;
	if (!sdev)
		return -ENXIO;

	id = timer_id;
	if (id < 1 || id > sdev->nr_timers) {
		pr_err("%s: systimer%d is not exist!\n", __func__, id);
		return -EINVAL;
	}

	dev = &sdev->systimer[id - 1];
	if (!dev)
		return -ENODEV;

	if (!(dev->flags & SYSTIMER_IN_USE)) {
		pr_err("%s: systimer%d is not in use!\n", __func__, id);
		return -EBUSY;
	}

	expires = us_to_cycle(usec);

	systimer_reg_write(dev->cntcon_reg, CNTTVAL_EN);
	systimer_reg_write(dev->cnttval_reg, expires);
	systimer_reg_write(dev->cntcon_reg, CNTIRQ_EN | CNTTVAL_EN);

	return 0;
}
EXPORT_SYMBOL_GPL(systimer_start);

/**
 *	systimer_stop - stop a systimer
 *	@timer_id: systimer id to stop
 *
 * Used to stop a systimer started with systimer_start().
 * This function may be called from interrupt context.
 */
int systimer_stop(unsigned int timer_id)
{
	struct mtk_systimer_device *sdev;
	struct system_timer *dev;
	unsigned int id;

	sdev = mtk_systimer;
	if (!sdev)
		return -ENXIO;

	id = timer_id;
	if (id < 1 || id > sdev->nr_timers) {
		pr_err("%s: systimer%d is not exist!\n", __func__, id);
		return -EINVAL;
	}

	dev = &sdev->systimer[id - 1];
	if (!dev)
		return -ENODEV;

	if (!(dev->flags & SYSTIMER_IN_USE)) {
		pr_err("%s: systimer%d is not in use!\n", __func__, id);
		return -EBUSY;
	}

	systimer_reg_write(dev->cntcon_reg, 0x00);

	return 0;
}
EXPORT_SYMBOL_GPL(systimer_stop);

static inline void __fill_systimer_device_info(
				struct mtk_systimer_device *dev,
				int num, unsigned int *irq)
{
	int i;

	/* Here we assume that the number of interrupts is equivalent to
	 * the number of systimer which will be used.
	 */
	dev->nr_timers = num;
	for (i = 0; i < dev->nr_timers; i++) {
		dev->systimer[i].id = i;
		dev->systimer[i].irq = irq[i];
		dev->systimer[i].flags = 0;
		dev->systimer[i].cntcon_reg = (unsigned long)dev->base_addr
						+ OFFSET_CNTVAL_CON_REG + 0x08 * i;
		dev->systimer[i].cnttval_reg = (unsigned long)dev->base_addr
						+ OFFSET_CNTTVAL_REG + 0x08 * i;
		dev->systimer[i].func = NULL;
	}
}

static void __init mtk_systimer_init(struct device_node *node)
{
	int nr_irqs, i, j;
	unsigned int irqs[MAX_NR_SYSTIMER];
	struct clk *clk;
	struct mtk_systimer_device *sdev;
	unsigned long save_flags;

	sdev = kzalloc(sizeof(*sdev), GFP_KERNEL);
	if (!sdev)
		return;

	/* Setup IO addresses */
	sdev->base_addr = of_iomap(node, 0);
	if (IS_ERR(sdev->base_addr)) {
		pr_err("Can't map base addr!\n");
		goto err_kzalloc;
	}

	/* Parse all IRQ numbers for all systimers */
	nr_irqs = of_irq_count(node);
	nr_irqs = (nr_irqs > MAX_NR_SYSTIMER) ? MAX_NR_SYSTIMER : nr_irqs;
	for (i = 0; i < nr_irqs; i++) {
		irqs[i] = irq_of_parse_and_map(node, i);
		if (irqs[i] <= 0) {
			pr_err("Can't parse IRQ\n");
			goto err_irq_and_iomap;
		}
	}

	/* Get systimer  device frequency */
	clk = of_clk_get(node, 0);
	if (IS_ERR(clk)) {
		pr_err("Can't get systimer clock\n");
		goto err_irq_and_iomap;
	}

	if (clk_prepare_enable(clk)) {
		pr_err("Can't enable systimer clock\n");
		goto err_clk_put;
	}
	sdev->freq = clk_get_rate(clk);

	/* Initialize each systimer device */
	systimer_spin_lock(save_flags);
	__fill_systimer_device_info(sdev, nr_irqs, irqs);

	for (i = 0; i < nr_irqs; i++) {
		if (request_irq(sdev->systimer[i].irq, systimer_interrupt,
				IRQF_TIMER | IRQF_IRQPOLL,
				"mtk_systimer", &sdev->systimer[i])) {
			systimer_spin_unlock(save_flags);
			pr_err("failed to setup systimer irq %d\n", i);
			goto err_clk_disable;
		}
	}
	systimer_spin_unlock(save_flags);

	mtk_systimer = sdev;

	pr_debug("%s: systimer_nums = %u\n", __func__, nr_irqs);

	return;

err_clk_disable:
	for (j = i - 1; j >= 0; j--)
		free_irq(irqs[j], &sdev->systimer[j]);
	i = nr_irqs;
	clk_disable_unprepare(clk);
err_clk_put:
	clk_put(clk);
err_irq_and_iomap:
	for (j = i - 1; j >= 0; j--)
		irq_dispose_mapping(irqs[j]);
	iounmap(sdev->base_addr);
err_kzalloc:
	kfree(sdev);
}

CLOCKSOURCE_OF_DECLARE(mtk_systimer, "mediatek,mtk-systimer", mtk_systimer_init);

MODULE_AUTHOR("Dehui Sun <dehui.sun@mediatek.com>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Mediatek system timer common driver");
