/*
 * Copyright (C) 2016 MediaTek Inc.
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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/io.h>
#include <linux/i2c.h>

#include "musb_core.h"
#include "mtk_musb.h"
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include "musbhsdma.h"
#include <linux/device.h>
#include <linux/clk.h>
#include <linux/regulator/consumer.h>
#include <linux/usb/phy.h>
#include <linux/usb/usb_phy_generic.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>

struct clk *musb_clk;
struct clk *musb_mcu_clk;
struct clk *musb_slv_clk;

/* struct clk *musb_usbpll; */
struct regulator *usb_power;

/* #endif */
#include "usb20.h"
#include <linux/delay.h>
#ifdef CONFIG_OF
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/regulator/consumer.h>
#endif

#ifdef MUSB_QMU_SUPPORT
#include "musb_qmu.h"
#endif
/* extern struct musb *mtk_musb; */
static DEFINE_SEMAPHORE(power_clock_lock);
/* static bool platform_init_first = true;   //Mark by ALPS01262215 */
/* extern bool mtk_usb_power; */
static u32 cable_mode = CABLE_MODE_NORMAL;
#ifdef CONFIG_OF
u32 usb_irq_number1 = 0;
unsigned long usb_phy_base;
unsigned long usb_mac_base;
#endif
#ifdef MTK_UART_USB_SWITCH
u32 port_mode = PORT_MODE_USB;
u32 sw_tx = 0;
u32 sw_rx = 0;
u32 sw_uart_path = 0;
#endif

/*EP Fifo Config*/
static struct musb_fifo_cfg fifo_cfg[] __initdata = {
	{.hw_ep_num = 1, .style = MUSB_FIFO_TX, .maxpacket = 512, .ep_mode = EP_BULK, .mode =
	 MUSB_BUF_DOUBLE},
	{.hw_ep_num = 1, .style = MUSB_FIFO_RX, .maxpacket = 512, .ep_mode = EP_BULK, .mode =
	 MUSB_BUF_DOUBLE},
	{.hw_ep_num = 2, .style = MUSB_FIFO_TX, .maxpacket = 512, .ep_mode = EP_BULK, .mode =
	 MUSB_BUF_DOUBLE},
	{.hw_ep_num = 2, .style = MUSB_FIFO_RX, .maxpacket = 512, .ep_mode = EP_BULK, .mode =
	 MUSB_BUF_DOUBLE},
	{.hw_ep_num = 3, .style = MUSB_FIFO_TX, .maxpacket = 512, .ep_mode = EP_BULK, .mode =
	 MUSB_BUF_DOUBLE},
	{.hw_ep_num = 3, .style = MUSB_FIFO_RX, .maxpacket = 512, .ep_mode = EP_BULK, .mode =
	 MUSB_BUF_DOUBLE},
	{.hw_ep_num = 4, .style = MUSB_FIFO_TX, .maxpacket = 512, .ep_mode = EP_BULK, .mode =
	 MUSB_BUF_DOUBLE},
	{.hw_ep_num = 4, .style = MUSB_FIFO_RX, .maxpacket = 512, .ep_mode = EP_BULK, .mode =
	 MUSB_BUF_DOUBLE},
	{.hw_ep_num = 5, .style = MUSB_FIFO_TX, .maxpacket = 512, .ep_mode = EP_INT, .mode =
	 MUSB_BUF_SINGLE},
	{.hw_ep_num = 5, .style = MUSB_FIFO_RX, .maxpacket = 512, .ep_mode = EP_INT, .mode =
	 MUSB_BUF_SINGLE},
	{.hw_ep_num = 6, .style = MUSB_FIFO_TX, .maxpacket = 512, .ep_mode = EP_INT, .mode =
	 MUSB_BUF_SINGLE},
	{.hw_ep_num = 6, .style = MUSB_FIFO_RX, .maxpacket = 512, .ep_mode = EP_INT, .mode =
	 MUSB_BUF_SINGLE},
	{.hw_ep_num = 7, .style = MUSB_FIFO_TX, .maxpacket = 512, .ep_mode = EP_BULK, .mode =
	 MUSB_BUF_SINGLE},
	{.hw_ep_num = 7, .style = MUSB_FIFO_RX, .maxpacket = 512, .ep_mode = EP_BULK, .mode =
	 MUSB_BUF_SINGLE},
	{.hw_ep_num = 8, .style = MUSB_FIFO_TX, .maxpacket = 512, .ep_mode = EP_ISO, .mode =
	 MUSB_BUF_DOUBLE},
	{.hw_ep_num = 8, .style = MUSB_FIFO_RX, .maxpacket = 512, .ep_mode = EP_ISO, .mode =
	 MUSB_BUF_DOUBLE},
};

#ifdef CONFIG_OF
static const struct of_device_id apusb_of_ids[] = {
	/* {.compatible = "mediatek,mt-2701",}, */
	{.compatible = "mediatek,USB0",},
	{},
};

#if 0
static struct platform_device mt_usb_device = {
	.name = "mt_usb",
	.id = -1,
};
#endif

#endif
MODULE_DEVICE_TABLE(of, apusb_of_ids);

static struct timer_list musb_idle_timer;

static void musb_do_idle(unsigned long _musb)
{
	struct musb *musb = (void *)_musb;
	unsigned long flags;
	u8 devctl;

	if (musb->is_active) {
		DBG(0, "%s active, igonre do_idle\n", otg_state_string(musb->xceiv->otg->state));
		return;
	}

	spin_lock_irqsave(&musb->lock, flags);

	switch (musb->xceiv->otg->state) {
	case OTG_STATE_B_PERIPHERAL:
	case OTG_STATE_A_WAIT_BCON:
		devctl = musb_readb(musb->mregs, MUSB_DEVCTL);
		if (devctl & MUSB_DEVCTL_BDEVICE) {
			musb->xceiv->otg->state = OTG_STATE_B_IDLE;
			MUSB_DEV_MODE(musb);
		} else {
			musb->xceiv->otg->state = OTG_STATE_A_IDLE;
			MUSB_HST_MODE(musb);
		}
		break;
	case OTG_STATE_A_HOST:
		devctl = musb_readb(musb->mregs, MUSB_DEVCTL);
		if (devctl & MUSB_DEVCTL_BDEVICE)
			musb->xceiv->otg->state = OTG_STATE_B_IDLE;
		else
			musb->xceiv->otg->state = OTG_STATE_A_WAIT_BCON;
		break;
	default:
		break;
	}
	spin_unlock_irqrestore(&musb->lock, flags);

	DBG(0, "otg_state %s\n", otg_state_string(musb->xceiv->otg->state));
	
}

static void mt_usb_try_idle(struct musb *musb, unsigned long timeout)
{
	unsigned long default_timeout = jiffies + msecs_to_jiffies(3);
	static unsigned long last_timer;

	if (timeout == 0)
		timeout = default_timeout;

	/* Never idle if active, or when VBUS timeout is not set as host */
	if (musb->is_active || ((musb->a_wait_bcon == 0)
				&& (musb->xceiv->otg->state == OTG_STATE_A_WAIT_BCON))) {
		DBG(2, "%s active, deleting timer\n", otg_state_string(musb->xceiv->otg->state));
		del_timer(&musb_idle_timer);
		last_timer = jiffies;
		return;
	}

	if (time_after(last_timer, timeout)) {
		if (!timer_pending(&musb_idle_timer))
			last_timer = timeout;
		else {
			DBG(2, "Longer idle timer already pending, ignoring\n");
			return;
		}
	}
	last_timer = timeout;

	DBG(2, "%s inactive, for idle timer for %lu ms\n",
	    otg_state_string(musb->xceiv->otg->state),
	    (unsigned long)jiffies_to_msecs(timeout - jiffies));
	mod_timer(&musb_idle_timer, timeout);
}

static void mt_usb_enable(struct musb *musb)
{
	unsigned long flags;

	DBG(0, "%d, %d\n", mtk_usb_power, musb->power);

	if (musb->power == true)
		return;

	flags = musb_readl(mtk_musb->mregs, USB_L1INTM);

	/* mask ID pin, so "open clock" and "set flag" won't be interrupted. ISR may call clock_disable. */
	musb_writel(mtk_musb->mregs, USB_L1INTM, (~IDDIG_INT_STATUS) & flags);

	/* Mark by ALPS01262215
	   if (platform_init_first) {
	   DBG(0,"usb init first\n\r");
	   musb->is_host = true;
	   } */

	if (!mtk_usb_power) {
		if (down_interruptible(&power_clock_lock))
			DBG(0, "USB20 %s: busy, Couldn't get power_clock_lock\n", __func__);

#ifndef FPGA_PLATFORM
		/* enable_pll(UNIVPLL, "USB_PLL"); */
		DBG(0, "enable UPLL before connect\n");
#endif
		mdelay(10);

		usb_phy_recover();

		mtk_usb_power = true;

		up(&power_clock_lock);
	}
	musb->power = true;

	musb_writel(mtk_musb->mregs, USB_L1INTM, flags);
}

static void mt_usb_disable(struct musb *musb)
{
	DBG(0, "%s, %d, %d\n", __func__, mtk_usb_power, musb->power);

	if (musb->power == false)
		return;

	/* Mark by ALPS01262215
	   if (platform_init_first) {
	   DBG(0,"usb init first\n\r");
	   musb->is_host = false;
	   platform_init_first = false;
	   } */

	if (mtk_usb_power) {
		if (down_interruptible(&power_clock_lock))
			DBG(0, "USB20 %s: busy, Couldn't get power_clock_lock\n", __func__);

		mtk_usb_power = false;

		usb_phy_savecurrent();

#ifndef FPGA_PLATFORM
		/* disable_pll(UNIVPLL,"USB_PLL"); */
		DBG(0, "disable UPLL before disconnect\n");
#endif

		up(&power_clock_lock);
	}

	musb->power = false;
}

/* ================================ */
/* connect and disconnect functions */
/* ================================ */
bool mt_usb_is_device(void)
{
	DBG(4, "called\n");

	if (!mtk_musb) {
		DBG(0, "mtk_musb is NULL\n");
		return false;	/* don't do charger detection when usb is not ready */
	}
	DBG(4, "is_host=%d\n", mtk_musb->is_host);

	return !mtk_musb->is_host;
}

void mt_usb_connect(void)
{
#ifndef CONFIG_MTK_MUSB_SW_WITCH_MODE
	DBG(0, "[MUSB] USB is ready for connect\n");
	DBG(3, "is ready %d is_host %d power %d\n", mtk_musb->is_ready, mtk_musb->is_host,
	    mtk_musb->power);
	if (!mtk_musb || !mtk_musb->is_ready || mtk_musb->is_host || mtk_musb->power)
		return;

	DBG(0, "cable_mode=%d\n", cable_mode);

	if (cable_mode != CABLE_MODE_NORMAL) {
		DBG(0, "musb_sync_with_bat, USB_CONFIGURED\n");
		musb_sync_with_bat(mtk_musb, USB_CONFIGURED);
		mtk_musb->power = true;
		return;
	}

	if (!wake_lock_active(&mtk_musb->usb_lock))
		wake_lock(&mtk_musb->usb_lock);

	musb_start(mtk_musb);
	DBG(0, "[MUSB] USB connect\n");
#endif
}

void mt_usb_disconnect(void)
{
#ifndef CONFIG_MTK_MUSB_SW_WITCH_MODE
	DBG(0, "[MUSB] USB is ready for disconnect\n");

	if (!mtk_musb || !mtk_musb->is_ready || mtk_musb->is_host || !mtk_musb->power)
		return;

	musb_stop(mtk_musb);

	if (wake_lock_active(&mtk_musb->usb_lock))
		wake_unlock(&mtk_musb->usb_lock);

	DBG(0, "cable_mode=%d\n", cable_mode);

	if (cable_mode != CABLE_MODE_NORMAL) {
		DBG(0, "musb_sync_with_bat, USB_SUSPEND\n");
		musb_sync_with_bat(mtk_musb, USB_SUSPEND);
		mtk_musb->power = false;
	}

	DBG(0, "[MUSB] USB disconnect\n");
#endif
}

#ifdef CONFIG_MTK_MUSB_SW_WITCH_MODE
static void mt_usb20_sw_connect(void)
{
	if (!wake_lock_active(&mtk_musb->usb_lock))
		wake_lock(&mtk_musb->usb_lock);

	musb_start(mtk_musb);
	DBG(0, "[MUSB] USB connect\n");
}

static void mt_usb20_sw_disconnect(void)
{
	musb_stop(mtk_musb);

	if (wake_lock_active(&mtk_musb->usb_lock))
		wake_unlock(&mtk_musb->usb_lock);

	DBG(0, "[MUSB] USB disconnect\n");
}
#endif

bool usb_cable_connected(void)
{
#ifdef MTK_USB_VBUS_DETECT_SUPPORT
	int i4_vbus_state = 0;
#endif

#ifdef CONFIG_USB_MTK_OTG
	int iddig_state = 1;

	iddig_state = gpio_get_value(iddig_pin);
	DBG(0, "iddig_state = %d\n", iddig_state);

	if (!iddig_state)
		return false;
#endif

#ifdef MTK_USB_VBUS_DETECT_SUPPORT
	i4_vbus_state = gpio_get_value(vbus_detect_pin);
	DBG(0, "i4_vbus_state = %d\n", i4_vbus_state);

	if (1 == i4_vbus_state)
		return true;
	else
		return false;
#endif

	return false;
}

void musb_platform_reset(struct musb *musb)
{
	u16 swrst = 0;
	void __iomem *mbase = musb->mregs;

	swrst = musb_readw(mbase, MUSB_SWRST);
	swrst |= (MUSB_SWRST_DISUSBRESET | MUSB_SWRST_SWRST);
	musb_writew(mbase, MUSB_SWRST, swrst);
}

static void usb_check_connect(void)
{
#ifndef FPGA_PLATFORM
	if (usb_cable_connected())
		mt_usb_connect();
#endif
}

bool is_switch_charger(void)
{
#ifdef SWITCH_CHARGER
	return true;
#else
	return false;
#endif
}

void pmic_chrdet_int_en(int is_on)
{
#ifndef FPGA_PLATFORM
	/* upmu_interrupt_chrdet_int_en(is_on); */
#endif
}

void musb_sync_with_bat(struct musb *musb, int usb_state)
{
#ifdef CONFIG_MTK_SMART_BATTERY
#ifndef FPGA_PLATFORM
	DBG(0, "BATTERY_SetUSBState, state=%d\n", usb_state);
	BATTERY_SetUSBState(usb_state);
	wake_up_bat();
#endif
#endif
}

/*-------------------------------------------------------------------------*/
static irqreturn_t generic_interrupt(int irq, void *__hci)
{
	unsigned long flags;
	irqreturn_t retval = IRQ_NONE;
	struct musb *musb = __hci;

	spin_lock_irqsave(&musb->lock, flags);


	/* musb_read_clear_generic_interrupt */
	musb->int_usb =
	    musb_readb(musb->mregs, MUSB_INTRUSB) & musb_readb(musb->mregs, MUSB_INTRUSBE);
	musb->int_tx = musb_readw(musb->mregs, MUSB_INTRTX) & musb_readw(musb->mregs, MUSB_INTRTXE);
	musb->int_rx = musb_readw(musb->mregs, MUSB_INTRRX) & musb_readw(musb->mregs, MUSB_INTRRXE);
#ifdef MUSB_QMU_SUPPORT
	musb->int_queue = musb_readl(musb->mregs, MUSB_QISAR);
#endif
	mb();
	musb_writew(musb->mregs, MUSB_INTRRX, musb->int_rx);
	musb_writew(musb->mregs, MUSB_INTRTX, musb->int_tx);
	musb_writeb(musb->mregs, MUSB_INTRUSB, musb->int_usb);
#ifdef MUSB_QMU_SUPPORT
	if (musb->int_queue) {
		musb_writel(musb->mregs, MUSB_QISAR, musb->int_queue);
		musb->int_queue &= ~(musb_readl(musb->mregs, MUSB_QIMR));
	}
#endif
	/* musb_read_clear_generic_interrupt */

#ifdef MUSB_QMU_SUPPORT
	if (musb->int_usb || musb->int_tx || musb->int_rx || musb->int_queue)
		retval = musb_interrupt(musb);
#else
	if (musb->int_usb || musb->int_tx || musb->int_rx)
		retval = musb_interrupt(musb);
#endif

	spin_unlock_irqrestore(&musb->lock, flags);

	return retval;
}

static irqreturn_t mt_usb_interrupt(int irq, void *dev_id)
{
	irqreturn_t tmp_status;
	irqreturn_t status = IRQ_NONE;
	struct musb *musb = (struct musb *)dev_id;
	u32 usb_l1_ints;

	usb_l1_ints = musb_readl(musb->mregs, USB_L1INTS) & musb_readl(mtk_musb->mregs, USB_L1INTM);
	DBG(1, "usb interrupt assert %x %x  %x %x %x\n", usb_l1_ints,
	    musb_readl(mtk_musb->mregs, USB_L1INTM), musb_readb(musb->mregs, MUSB_INTRUSBE),
	    musb_readw(musb->mregs, MUSB_INTRTX), musb_readw(musb->mregs, MUSB_INTRTXE));

	if ((usb_l1_ints & TX_INT_STATUS) || (usb_l1_ints & RX_INT_STATUS)
	    || (usb_l1_ints & USBCOM_INT_STATUS)
#ifdef MUSB_QMU_SUPPORT
	    || (usb_l1_ints & QINT_STATUS)
#endif
	    ) {
		tmp_status = generic_interrupt(irq, musb);
		if (tmp_status != IRQ_NONE)
			status = tmp_status;
	}

	if (usb_l1_ints & DMA_INT_STATUS) {
		tmp_status = dma_controller_irq(irq, musb->dma_controller);
		if (tmp_status != IRQ_NONE)
			status = tmp_status;
	}
#ifdef	CONFIG_USB_MTK_OTG
	if (usb_l1_ints & IDDIG_INT_STATUS) {
		mt_usb_iddig_int(musb);
		status = IRQ_HANDLED;
	}
#endif

	return status;

}

/*--FOR INSTANT POWER ON USAGE--------------------------------------------------*/
static ssize_t mt_usb_show_cmode(struct device *dev, struct device_attribute *attr, char *buf)
{
	if (!dev) {
		DBG(0, "dev is null!!\n");
		return 0;
	}
	return scnprintf(buf, PAGE_SIZE, "%d\n", cable_mode);
}

static ssize_t mt_usb_store_cmode(struct device *dev, struct device_attribute *attr,
				  const char *buf, size_t count)
{
	unsigned int cmode;

	if (!dev) {
		DBG(0, "dev is null!!\n");
		return count;
		/* } else if (1 == sscanf(buf, "%d", &cmode)) { */
	} else if (kstrtol(buf, 10, (long *)&cmode) == 0) {
		DBG(0, "cmode=%d, cable_mode=%d\n", cmode, cable_mode);
		if (cmode >= CABLE_MODE_MAX)
			cmode = CABLE_MODE_NORMAL;

		if (cable_mode != cmode) {
			if (mtk_musb) {
				if (down_interruptible(&mtk_musb->musb_lock))
					DBG(0, "USB20 %s: busy, Couldn't get musb_lock\n",
					    __func__);
			}
			if (cmode == CABLE_MODE_CHRG_ONLY) {	/* IPO shutdown, disable USB */
				if (mtk_musb)
					mtk_musb->in_ipo_off = true;

			} else {	/* IPO bootup, enable USB */
				if (mtk_musb)
					mtk_musb->in_ipo_off = false;

			}

			mt_usb_disconnect();
			cable_mode = cmode;
			mdelay(10);
			/* check that "if USB cable connected and than call mt_usb_connect" */
			/* Then, the Bat_Thread won't be always wakeup while no USB/chatger cable and IPO mode */
			usb_check_connect();

#ifdef CONFIG_USB_MTK_OTG
			if (cmode == CABLE_MODE_CHRG_ONLY) {
				if (mtk_musb && mtk_musb->is_host) {	/* shut down USB host for IPO */
					if (wake_lock_active(&mtk_musb->usb_lock))
						wake_unlock(&mtk_musb->usb_lock);
					musb_platform_set_vbus(mtk_musb, 0);
					msleep(50);
					musb_stop(mtk_musb);
					MUSB_DEV_MODE(mtk_musb);
					musb_writeb(mtk_musb->mregs, MUSB_DEVCTL,
						    (~MUSB_DEVCTL_SESSION) &
						    musb_readb(mtk_musb->mregs, MUSB_DEVCTL));
				}
				switch_int_to_host_and_mask(mtk_musb);
				/* mask ID pin interrupt even if A-cable is not plugged in */
			} else {
				switch_int_to_host(mtk_musb);	/* resotre ID pin interrupt */
			}
#endif
			if (mtk_musb)
				up(&mtk_musb->musb_lock);

		}
	}
	return count;
}

DEVICE_ATTR(cmode, 0664, mt_usb_show_cmode, mt_usb_store_cmode);

#ifdef CONFIG_MTK_MUSB_CARPLAY_SUPPORT
static ssize_t mt_usb_show_mode(struct device *dev, struct device_attribute *attr, char *buf)
{
	return 0;
}

static ssize_t mt_usb_store_mode(struct device *dev, struct device_attribute *attr,
				 const char *buf, size_t count)
{
	unsigned int usb_mode;
	int ret = 0;

	DBG(0, "new kstrtou32 mt_usb_store_mode\n");
	if (!dev) {
		DBG(0, "dev is null!!\n");
		return count;
	}
	if (buf != NULL && count != 0) {
		ret = kstrtou32(buf, 10, &usb_mode);
		/* ret = sscanf(buf, "%d", &usb_mode); */
		DBG(0, "usb_mode=%d\n", usb_mode);
		if (ret < 0)
			return -1;
	}
	switch (usb_mode) {
	case 1:
		if (apple) {
			send_switch_cmd();
			DBG(0, "mt_usb_store_mode send_switch_cmd\n");
			musb_id_pin_work_device();
			DBG(0, "mt_usb_store_mode to device\n");
		} else
			DBG(0, "have no apple device apple = %d\n", apple);
		break;
	case 2:
		DBG(0, "mt_usb_store_mode to host\n");
		musb_id_pin_work_host(NULL);
	default:
		break;
	}

	return count;
}

DEVICE_ATTR(mode, 0664, mt_usb_show_mode, mt_usb_store_mode);
#endif

#ifdef CONFIG_MTK_MUSB_SW_WITCH_MODE
static bool device_mode;
static bool host_mode;

static ssize_t mt_usb_store_swmode(struct device *dev, struct device_attribute *attr,
				   const char *buf, size_t count)
{
	DBG(0, "buf = %s count = %d\n", buf, count);
	if (count >= 6 && !strncmp(buf, "device", 6)) {
		if (!device_mode && !host_mode) {
			DBG(0, "switch to device mode\n");
			mt_usb20_sw_connect();
			device_mode = true;
		}
	}

	if (count >= 4 && !strncmp(buf, "host", 4)) {
		if (!device_mode && !host_mode) {
			DBG(0, "switch to host mode\n");
			musb_id_pin_sw_work(true);
			host_mode = true;
		}
	}

	if (count >= 4 && !strncmp(buf, "idle", 4)) {
		if (device_mode) {
			DBG(0, "switch to dev->idle mode\n");
			mt_usb20_sw_disconnect();
			device_mode = false;
		} else if (host_mode) {
			DBG(0, "switch to host->idle mode\n");
			musb_id_pin_sw_work(false);
			host_mode = false;
		} else
			DBG(0, "do nothing\n");
	}

	return count;
}

DEVICE_ATTR(swmode, 0220, NULL, mt_usb_store_swmode);
#endif

#ifdef MTK_UART_USB_SWITCH
static void uart_usb_switch_dump_register(void)
{
	usb_enable_clock(true);
#ifdef FPGA_PLATFORM
	DBG(0, "[MUSB]addr: 0x6B, value: %x\n", USB_PHY_Read_Register8(0x6B));
	DBG(0, "[MUSB]addr: 0x6E, value: %x\n", USB_PHY_Read_Register8(0x6E));
	DBG(0, "[MUSB]addr: 0x22, value: %x\n", USB_PHY_Read_Register8(0x22));
	DBG(0, "[MUSB]addr: 0x68, value: %x\n", USB_PHY_Read_Register8(0x68));
	DBG(0, "[MUSB]addr: 0x6A, value: %x\n", USB_PHY_Read_Register8(0x6A));
	DBG(0, "[MUSB]addr: 0x1A, value: %x\n", USB_PHY_Read_Register8(0x1A));
#else
	DBG(0, "[MUSB]addr: 0x6B, value: %x\n", USBPHY_READ8(0x6B));
	DBG(0, "[MUSB]addr: 0x6E, value: %x\n", USBPHY_READ8(0x6E));
	DBG(0, "[MUSB]addr: 0x22, value: %x\n", USBPHY_READ8(0x22));
	DBG(0, "[MUSB]addr: 0x68, value: %x\n", USBPHY_READ8(0x68));
	DBG(0, "[MUSB]addr: 0x6A, value: %x\n", USBPHY_READ8(0x6A));
	DBG(0, "[MUSB]addr: 0x1A, value: %x\n", USBPHY_READ8(0x1A));
#endif
	usb_enable_clock(false);
	DBG(0, "[MUSB]addr: 0x11002090 (UART1), value: %x\n\n", DRV_Reg8(UART1_BASE + 0x90));
}

static ssize_t mt_usb_show_portmode(struct device *dev, struct device_attribute *attr, char *buf)
{
	if (!dev) {
		DBG(0, "dev is null!!\n");
		return 0;
	}

	if (usb_phy_check_in_uart_mode())
		port_mode = PORT_MODE_UART;
	else
		port_mode = PORT_MODE_USB;

	if (port_mode == PORT_MODE_USB)
		DBG(0, "\nUSB Port mode -> USB\n");
	else if (port_mode == PORT_MODE_UART)
		DBG(0, "\nUSB Port mode -> UART\n");

	uart_usb_switch_dump_register();

	return scnprintf(buf, PAGE_SIZE, "%d\n", port_mode);
}

static ssize_t mt_usb_store_portmode(struct device *dev, struct device_attribute *attr,
				     const char *buf, size_t count)
{
	unsigned int portmode;

	if (!dev) {
		DBG(0, "dev is null!!\n");
		return count;
		/* } else if (1 == sscanf(buf, "%d", &portmode)) { */
	} else if (kstrtol(buf, 10, &portmode) == 0) {
		DBG(0, "\nUSB Port mode: current => %d (port_mode), change to => %d (portmode)\n",
		    port_mode, portmode);
		if (portmode >= PORT_MODE_MAX)
			portmode = PORT_MODE_USB;

		if (port_mode != portmode) {
			if (portmode == PORT_MODE_USB) {	/* Changing to USB Mode */
				DBG(0, "USB Port mode -> USB\n");
				usb_phy_switch_to_usb();
			} else if (portmode == PORT_MODE_UART) {	/* Changing to UART Mode */
				DBG(0, "USB Port mode -> UART\n");
				usb_phy_switch_to_uart();
			}
			uart_usb_switch_dump_register();
			port_mode = portmode;
		}
	}
	return count;
}

DEVICE_ATTR(portmode, 0664, mt_usb_show_portmode, mt_usb_store_portmode);


static ssize_t mt_usb_show_tx(struct device *dev, struct device_attribute *attr, char *buf)
{
	UINT8 var;
	UINT8 var2;

	if (!dev) {
		DBG(0, "dev is null!!\n");
		return 0;
	}
#ifdef FPGA_PLATFORM
	var = USB_PHY_Read_Register8(0x6E);
#else
	var = USBPHY_READ8(0x6E);
#endif
	var2 = (var >> 3) & ~0xFE;
	DBG(0, "[MUSB]addr: 0x6E (TX), value: %x - %x\n", var, var2);

	sw_tx = var;

	return scnprintf(buf, PAGE_SIZE, "%x\n", var2);
}

static ssize_t mt_usb_store_tx(struct device *dev, struct device_attribute *attr,
			       const char *buf, size_t count)
{
	unsigned int val;
	UINT8 var;
	UINT8 var2;

	if (!dev) {
		DBG(0, "dev is null!!\n");
		return count;
		/* } else if (1 == sscanf(buf, "%d", &val)) { */
	} else if (kstrtol(buf, 10, &val) == 0) {
		DBG(0, "\n Write TX : %d\n", val);

#ifdef FPGA_PLATFORM
		var = USB_PHY_Read_Register8(0x6E);
#else
		var = USBPHY_READ8(0x6E);
#endif

		if (val == 0)
			var2 = var & ~(1 << 3);
		else
			var2 = var | (1 << 3);


#ifdef FPGA_PLATFORM
		USB_PHY_Write_Register8(var2, 0x6E);
		var = USB_PHY_Read_Register8(0x6E);
#else
		USBPHY_WRITE8(0x6E, var2);
		var = USBPHY_READ8(0x6E);
#endif

		var2 = (var >> 3) & ~0xFE;

		DBG(0, "[MUSB]addr: 0x6E TX [AFTER WRITE], value after: %x - %x\n", var, var2);
		sw_tx = var;
	}
	return count;
}

DEVICE_ATTR(tx, 0664, mt_usb_show_tx, mt_usb_store_tx);

static ssize_t mt_usb_show_rx(struct device *dev, struct device_attribute *attr, char *buf)
{
	UINT8 var;
	UINT8 var2;

	if (!dev) {
		DBG(0, "dev is null!!\n");
		return 0;
	}
#ifdef FPGA_PLATFORM
	var = USB_PHY_Read_Register8(0x77);
#else
	var = USBPHY_READ8(0x77);
#endif
	var2 = (var >> 7) & ~0xFE;
	DBG(0, "[MUSB]addr: 0x77 (RX), value: %x - %x\n", var, var2);
	sw_rx = var;

	return scnprintf(buf, PAGE_SIZE, "%x\n", var2);
}

DEVICE_ATTR(rx, 0664, mt_usb_show_rx, NULL);

static ssize_t mt_usb_show_uart_path(struct device *dev, struct device_attribute *attr, char *buf)
{
	UINT8 var;

	if (!dev) {
		DBG(0, "dev is null!!\n");
		return 0;
	}

	var = DRV_Reg8(UART1_BASE + 0x90);
	DBG(0, "[MUSB]addr: 0x11002090 (UART1), value: %x\n\n", DRV_Reg8(UART1_BASE + 0x90));
	sw_uart_path = var;

	return scnprintf(buf, PAGE_SIZE, "%x\n", var);
}

DEVICE_ATTR(uartpath, 0664, mt_usb_show_uart_path, NULL);
#endif


bool is_saving_mode(void)
{
	return 0;
}


#ifdef FPGA_PLATFORM
static struct i2c_client *usb_i2c_client;
static const struct i2c_device_id usb_i2c_id[] = { {"mtk-usb", 0}, {} };

static struct i2c_board_info usb_i2c_dev __initdata = { I2C_BOARD_INFO("mtk-usb", 0x60) };


void USB_PHY_Write_Register8(UINT8 var, UINT8 addr)
{
	char buffer[2];

	buffer[0] = addr;
	buffer[1] = var;
	i2c_master_send(usb_i2c_client, buffer, 2);
}

UINT8 USB_PHY_Read_Register8(UINT8 addr)
{
	UINT8 var;

	i2c_master_send(usb_i2c_client, &addr, 1);
	i2c_master_recv(usb_i2c_client, &var, 1);
	return var;
}

static int usb_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
#ifdef CONFIG_OF
	unsigned long base;
	/* if i2c probe before musb prob, this would cause KE */
	/* base = (unsigned long)((unsigned long)mtk_musb->xceiv->io_priv); */
	base = usb_phy_base;
	DBG(0, "[MUSB]usb_i2c_probe, start, base:%lx\n", base);
#endif
	usb_i2c_client = client;

#ifdef CONFIG_OF
	/* disable usb mac suspend */
	DRV_WriteReg8(base + 0x86a, 0x00);
#else
	DRV_WriteReg8(USB_SIF_BASE + 0x86a, 0x00);
#endif
	USB_PHY_Write_Register8(0x00, 0xFF);
	USB_PHY_Write_Register8(0x55, 0x05);
	USB_PHY_Write_Register8(0x84, 0x18);
	USB_PHY_Write_Register8(0x10, 0xFF);
	USB_PHY_Write_Register8(0x84, 0x0A);
	USB_PHY_Write_Register8(0x40, 0xFF);
	USB_PHY_Write_Register8(0x46, 0x38);
	USB_PHY_Write_Register8(0x40, 0x42);
	USB_PHY_Write_Register8(0xAB, 0x08);
	USB_PHY_Write_Register8(0x0C, 0x09);
	USB_PHY_Write_Register8(0x71, 0x0C);
	USB_PHY_Write_Register8(0x4F, 0x0E);
	USB_PHY_Write_Register8(0xE1, 0x10);
	USB_PHY_Write_Register8(0x5F, 0x14);
	USB_PHY_Write_Register8(0x60, 0xFF);
	USB_PHY_Write_Register8(0x03, 0x14);
	USB_PHY_Write_Register8(0x00, 0xFF);
	USB_PHY_Write_Register8(0x04, 0x6A);
	USB_PHY_Write_Register8(0x08, 0x68);


	DBG(0, "[MUSB]usb_i2c_probe, end\n");
	return 0;

}

/*static int usb_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info) {
    strcpy(info->type, "mtk-usb");
    return 0;
}*/

static int usb_i2c_remove(struct i2c_client *client)
{
	return 0;
}


struct i2c_driver usb_i2c_driver = {
	.probe = usb_i2c_probe,
	.remove = usb_i2c_remove,
	/*.detect = usb_i2c_detect, */
	.driver = {
		   .name = "mtk-usb",
		   },
	.id_table = usb_i2c_id,
};

static int add_usb_i2c_driver(void)
{
	i2c_register_board_info(2, &usb_i2c_dev, 1);

	if (i2c_add_driver(&usb_i2c_driver) != 0) {
		DBG(0, "[MUSB]usb_i2c_driver initialization failed!!\n");
		return -1;
	}

	DBG(0, "[MUSB]usb_i2c_driver initialization succeed!!\n");
	return 0;
}
#endif				/* End of FPGA_PLATFORM */

static int mt_usb_init(struct musb *musb)
{
	int ret = 0;

	DBG(0, "mt_usb_init\n");

	/* phy device added by dtsi, usbphy0 */
	musb->xceiv = usb_get_phy(USB_PHY_TYPE_USB2);
	if (IS_ERR_OR_NULL(musb->xceiv)) {
		DBG(0, "[MUSB] usb_get_phy error!!\n");
		return -EPROBE_DEFER;
	}
	musb->xceiv->io_priv = (void __iomem *)usb_phy_base;
#ifdef CONFIG_OF
	/* musb->nIrq = usb_irq_number1; */
#else
	musb->nIrq = USB_MCU_IRQ_BIT1_ID;
#endif
	musb->dma_irq = (int)SHARE_IRQ;
	musb->fifo_cfg = fifo_cfg;
	musb->fifo_cfg_size = ARRAY_SIZE(fifo_cfg);
	musb->dyn_fifo = true;
	musb->power = false;
	musb->is_host = false;
	musb->fifo_size = 8 * 1024;

	wake_lock_init(&musb->usb_lock, WAKE_LOCK_SUSPEND, "USB suspend lock");

#ifndef FPGA_PLATFORM
	/* hwPowerOn(MT6323_POWER_LDO_VUSB, VOL_3300, "VUSB_LDO"); */
	/* ret = regulator_set_voltage(usb_power, 3300000, 3300000); */
	/* if (ret) { */
	/* dev_err(musb->controller, "Set regulator fail, ret:%d!\n", ret); */
	/* return ret; */
	/* } */
	ret = regulator_enable(usb_power);
	if (ret) {
		dev_err(musb->controller, "Enable usb regulator fail, ret:%d!\n", ret);
		return ret;
	}
	DBG(0, "%s, enable VBUS_LDO\n", __func__);
#endif

	/* mt_usb_enable(musb); */

	musb->isr = mt_usb_interrupt;
	musb_writel(musb->mregs, MUSB_HSDMA_INTR, 0xff | (0xff << DMA_INTR_UNMASK_SET_OFFSET));
	DBG(0, "musb platform init %x\n", musb_readl(musb->mregs, MUSB_HSDMA_INTR));
#ifdef MUSB_QMU_SUPPORT
	musb_writel(musb->mregs, USB_L1INTM,
		    TX_INT_STATUS | RX_INT_STATUS | USBCOM_INT_STATUS | DMA_INT_STATUS |
		    QINT_STATUS);
#else
	musb_writel(musb->mregs, USB_L1INTM, TX_INT_STATUS |
		    RX_INT_STATUS | USBCOM_INT_STATUS | DMA_INT_STATUS);
#endif

	setup_timer(&musb_idle_timer, musb_do_idle, (unsigned long)musb);

#ifdef CONFIG_USB_MTK_OTG
	mt_usb_otg_init(musb);
#endif

	return 0;
}

static int mt_usb_exit(struct musb *musb)
{
	del_timer_sync(&musb_idle_timer);
	return 0;
}

static const struct musb_platform_ops mt_usb_ops = {
	.init = mt_usb_init,
	.exit = mt_usb_exit,
	/*.set_mode     = mt_usb_set_mode, */
	.try_idle = mt_usb_try_idle,
	.enable = mt_usb_enable,
	.disable = mt_usb_disable,
	.set_vbus = mt_usb_set_vbus,
	.vbus_status = mt_usb_get_vbus_status
};

static u64 mt_usb_dmamask = DMA_BIT_MASK(32);

static int mt_usb_probe(struct platform_device *pdev)
{
	struct musb_hdrc_platform_data *pdata = pdev->dev.platform_data;
	struct platform_device *musb;
	struct mt_usb_glue *glue;
	int ret = -ENOMEM;
	unsigned int volt;
#ifdef CONFIG_OF
	struct musb_hdrc_config *config;
	struct device_node *np = pdev->dev.of_node;
#endif

	glue = kzalloc(sizeof(*glue), GFP_KERNEL);
	if (!glue) {
		/* dev_err(&pdev->dev, "failed to allocate glue context\n"); */
		goto err0;
	}

	musb = platform_device_alloc("musb-hdrc", PLATFORM_DEVID_AUTO);

	if (!musb) {
		dev_err(&pdev->dev, "failed to allocate musb device\n");
		goto err1;
	}
#ifdef CONFIG_OF
	/* usb_irq_number1 = irq_of_parse_and_map(pdev->dev.of_node, 0); */
	usb_phy_base = (unsigned long)of_iomap(pdev->dev.of_node, 1);
	if (!usb_phy_base) {
		dev_err(&pdev->dev, "cannot get usb phy\n");
		ret = -ENOMEM;
		goto err2;
	}
	/* usb_mac_base = (unsigned long)of_iomap(pdev->dev.of_node, 0); */
	pdata = devm_kzalloc(&pdev->dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata) {
		/* dev_err(&pdev->dev, "failed to allocate musb platform data\n"); */
		goto err2;
	}
	config = devm_kzalloc(&pdev->dev, sizeof(*config), GFP_KERNEL);
	if (!config) {
		/* dev_err(&pdev->dev, "failed to allocate musb hdrc config\n"); */
		goto err2;
	}

	musb_clk = devm_clk_get(&pdev->dev, "usb0_main");
	if (IS_ERR(musb_clk)) {
		DBG(0, KERN_WARNING "cannot get musb clock\n");
		return PTR_ERR(musb_clk);
	}
	DBG(0, KERN_WARNING "get musb musb_clk clock ok, prepare it\n");

	musb_mcu_clk = devm_clk_get(&pdev->dev, "usb0_mcu");
	if (IS_ERR(musb_mcu_clk)) {
		DBG(0, KERN_WARNING "cannot get musb_mcu_clk musb clock\n");
		return PTR_ERR(musb_mcu_clk);
	}
	DBG(0, KERN_WARNING "get musb clock ok, prepare it\n");

	musb_slv_clk = devm_clk_get(&pdev->dev, "usb_slv");
	if (IS_ERR(musb_slv_clk)) {
		DBG(0, KERN_WARNING "cannot get musb clock\n");
		return PTR_ERR(musb_slv_clk);
	}
	DBG(0, KERN_WARNING "get musb musb_slv_clk clock ok, prepare it\n");
#if 0
	retval = clk_prepare(musb_clk);
	if (retval == 0) {
		DBG(0, KERN_WARNING "prepare done\n");
	} else {
		DBG(0, KERN_WARNING "prepare fail\n");
		return retval;
	}
#endif

#if 0
	musb_usbpll = devm_clk_get(&pdev->dev, "usb_pll");
	if (IS_ERR(musb_usbpll)) {
		DBG(0, KERN_WARNING "cannot get musb clock\n");
		return PTR_ERR(musb_usbpll);
	}
	DBG(0, KERN_WARNING "get musb clock ok, prepare it\n");


	retval = clk_prepare(musb_usbpll);
	if (retval == 0) {
		DBG(0, KERN_WARNING "prepare done\n");
	} else {
		DBG(0, KERN_WARNING "prepare fail\n");
		return retval;
	}
#endif

	usb_power = devm_regulator_get(&pdev->dev, "usb-power");
	if (IS_ERR(usb_power)) {
		ret = PTR_ERR(usb_power);
		dev_err(&pdev->dev, "Cannot get usb power from the device tree!\n\n");
		return ret;
	}

	volt = regulator_get_voltage(usb_power);
	DBG(0, "usb_power = %d\n", volt);

#ifdef CONFIG_USB_MTK_OTG
	pdata->mode = MUSB_OTG;
#else
	of_property_read_u32(np, "mode", (u32 *) &pdata->mode);
#endif
	of_property_read_u32(np, "num_eps", (u32 *) &config->num_eps);
	config->multipoint = of_property_read_bool(np, "multipoint");
	pdata->config = config;

#endif
	musb->dev.parent = &pdev->dev;
	musb->dev.dma_mask = &mt_usb_dmamask;
	musb->dev.coherent_dma_mask = mt_usb_dmamask;

	glue->dev = &pdev->dev;
	glue->musb = musb;

	pdata->platform_ops = &mt_usb_ops;
	usb_phy_generic_register();

	platform_set_drvdata(pdev, glue);

	ret = platform_device_add_resources(musb, pdev->resource, pdev->num_resources);
	if (ret) {
		dev_err(&pdev->dev, "failed to add resources\n");
		goto err2;
	}

	ret = platform_device_add_data(musb, pdata, sizeof(*pdata));
	if (ret) {
		dev_err(&pdev->dev, "failed to add platform_data\n");
		goto err2;
	}

	ret = platform_device_add(musb);

	if (ret) {
		dev_err(&pdev->dev, "failed to register musb device\n");
		goto err2;
	}

	ret = device_create_file(&pdev->dev, &dev_attr_cmode);
#ifdef CONFIG_MTK_MUSB_CARPLAY_SUPPORT
	ret = device_create_file(&pdev->dev, &dev_attr_mode);
#endif
#ifdef CONFIG_MTK_MUSB_SW_WITCH_MODE
	ret = device_create_file(&pdev->dev, &dev_attr_swmode);
#endif
#ifdef MTK_UART_USB_SWITCH
	ret = device_create_file(&pdev->dev, &dev_attr_portmode);
	ret = device_create_file(&pdev->dev, &dev_attr_tx);
	ret = device_create_file(&pdev->dev, &dev_attr_rx);
	ret = device_create_file(&pdev->dev, &dev_attr_uartpath);
#endif

	if (ret) {
		dev_err(&pdev->dev, "failed to create musb device\n");
		goto err2;
	}
	return 0;

err2:
	platform_device_put(musb);

err1:
	kfree(glue);

err0:
	return ret;
}

static int mt_usb_remove(struct platform_device *pdev)
{
	struct mt_usb_glue *glue = platform_get_drvdata(pdev);

	platform_device_unregister(glue->musb);
	kfree(glue);

	return 0;
}

static struct platform_driver mt_usb_driver = {
	.remove = mt_usb_remove,
	.probe = mt_usb_probe,
	.driver = {
		   .name = "mt_usb",
#ifdef CONFIG_OF
		   .of_match_table = apusb_of_ids,
#endif
		   },
};

static int __init usb20_init(void)
{
	int i4_ret;

	DBG(0, "usb20 init\n");

#ifdef FPGA_PLATFORM
	add_usb_i2c_driver();
#endif

	i4_ret = platform_driver_register(&mt_usb_driver);
	return i4_ret;
}
module_init(usb20_init);

static void __exit usb20_exit(void)
{
	platform_driver_unregister(&mt_usb_driver);
}
module_exit(usb20_exit)
