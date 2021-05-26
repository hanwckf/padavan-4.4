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

/* #ifdef CONFIG_USB_MTK_OTG */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/io.h>
/* #include <linux/xlog.h> */
#ifndef CONFIG_OF
#include <mach/irqs.h>
#endif
/* #include <mach/eint.h> */
#if defined(CONFIG_MTK_LEGACY)
#include <mt-plat/mt_gpio.h>
#include <cust_gpio_usage.h>
#endif
#include "musb_core.h"
#include <linux/platform_device.h>
#include "musbhsdma.h"
//#include <linux/switch.h> //modiyf by wyl
#include "usb20.h"
#ifdef CONFIG_OF
#include <linux/of_irq.h>
#include <linux/of_address.h>
#endif

/* #include <mach/mt_boot_common.h> */

#ifdef CONFIG_OF
struct device_node *usb_node;
unsigned int iddig_pin;
/* static unsigned int iddig_pin_mode; */
/* static unsigned int iddig_if_config = 1; */
static unsigned int drvvbus_pin;
/* static unsigned int drvvbus_pin_mode; */
/* static unsigned int drvvbus_if_config = 1; */

struct pinctrl *pinctrl;
struct pinctrl_state *pinctrl_iddig;
struct pinctrl_state *pinctrl_drvvbus;
struct pinctrl_state *pinctrl_drvvbus_low;
struct pinctrl_state *pinctrl_drvvbus_high;
#ifdef ID_PIN_USE_EX_EINT
static int usb_iddig_number;
struct device_node *iddig_node;
#endif
#endif

#ifdef MTK_USB_VBUS_DETECT_SUPPORT
unsigned int vbus_detect_pin;
int vbus_detect_number;
struct device_node *vbus_detect_node;
#endif

/* #ifdef CONFIG_USB_MTK_OTG */
/* extern struct musb *mtk_musb; */

static struct musb_fifo_cfg fifo_cfg_host[] = {
	{.hw_ep_num = 1, .style = MUSB_FIFO_TX, .maxpacket = 512, .mode = MUSB_BUF_SINGLE},
	{.hw_ep_num = 1, .style = MUSB_FIFO_RX, .maxpacket = 512, .mode = MUSB_BUF_SINGLE},
	{.hw_ep_num = 2, .style = MUSB_FIFO_TX, .maxpacket = 512, .mode = MUSB_BUF_SINGLE},
	{.hw_ep_num = 2, .style = MUSB_FIFO_RX, .maxpacket = 512, .mode = MUSB_BUF_SINGLE},
	{.hw_ep_num = 3, .style = MUSB_FIFO_TX, .maxpacket = 512, .mode = MUSB_BUF_SINGLE},
	{.hw_ep_num = 3, .style = MUSB_FIFO_RX, .maxpacket = 512, .mode = MUSB_BUF_SINGLE},
	{.hw_ep_num = 4, .style = MUSB_FIFO_TX, .maxpacket = 512, .mode = MUSB_BUF_SINGLE},
	{.hw_ep_num = 4, .style = MUSB_FIFO_RX, .maxpacket = 512, .mode = MUSB_BUF_SINGLE},
	{.hw_ep_num = 5, .style = MUSB_FIFO_TX, .maxpacket = 512, .mode = MUSB_BUF_SINGLE},
	{.hw_ep_num = 5, .style = MUSB_FIFO_RX, .maxpacket = 512, .mode = MUSB_BUF_SINGLE},
	{.hw_ep_num = 6, .style = MUSB_FIFO_TX, .maxpacket = 512, .mode = MUSB_BUF_SINGLE},
	{.hw_ep_num = 6, .style = MUSB_FIFO_RX, .maxpacket = 512, .mode = MUSB_BUF_SINGLE},
	{.hw_ep_num = 7, .style = MUSB_FIFO_TX, .maxpacket = 512, .mode = MUSB_BUF_SINGLE},
	{.hw_ep_num = 7, .style = MUSB_FIFO_RX, .maxpacket = 512, .mode = MUSB_BUF_SINGLE},
	{.hw_ep_num = 8, .style = MUSB_FIFO_TX, .maxpacket = 512, .mode = MUSB_BUF_SINGLE},
	{.hw_ep_num = 8, .style = MUSB_FIFO_RX, .maxpacket = 64, .mode = MUSB_BUF_SINGLE},
};

u32 delay_time = 15;
module_param(delay_time, int, 0644);
u32 delay_time1 = 55;
module_param(delay_time1, int, 0644);

void mt_usb_set_vbus(struct musb *musb, int is_on)
{
	DBG(0, "mt65xx_usb20_vbus++,is_on=%d\r\n", is_on);
	if (is_on) {

#ifdef MTK_USB_VBUS_DETECT_SUPPORT
		/* disable VBUS detect pin interrupt */
		disable_irq(vbus_detect_number);
		/* msleep(100); */
		msleep(150);
#endif
		pinctrl_select_state(pinctrl, pinctrl_drvvbus_high);
		DBG(0, "mt65xx_usb20_vbus++,disable VBUS detect is_on=%d\r\n", is_on);
	} else {
		pinctrl_select_state(pinctrl, pinctrl_drvvbus_low);

#ifdef MTK_USB_VBUS_DETECT_SUPPORT
		/* msleep(100); */
		msleep(150);
		/* enable VBUS detect pin interrupt */
		enable_irq(vbus_detect_number);
#endif
		DBG(0, "mt65xx_usb20_vbus++,enable VBUS detect is_on=%d\r\n", is_on);
	}
}

int mt_usb_get_vbus_status(struct musb *musb)
{
#if 1
	return true;
#else
	int ret = 0;

	if ((musb_readb(musb->mregs, MUSB_DEVCTL) & MUSB_DEVCTL_VBUS) != MUSB_DEVCTL_VBUS) {
		ret = 1;
	} else {
		DBG(0, "VBUS error, devctl=%x, power=%d\n", musb_readb(musb->mregs, MUSB_DEVCTL),
		    musb->power);
	}
	DBG(0, "vbus ready = %d\n", ret);
	return ret;
#endif
}

#ifdef MTK_USB_VBUS_DETECT_SUPPORT
static void switch_vbus_int_to_idle(void)
{
	irq_set_irq_type(vbus_detect_number, IRQF_TRIGGER_LOW);
	DBG(0, "vbus switch interrupt for idle detect is done\n");
}

static void switch_vbus_int_to_device(void)
{
	irq_set_irq_type(vbus_detect_number, IRQF_TRIGGER_HIGH);
	DBG(0, "vbus switch interrupt for device detect is done\n");
}

static irqreturn_t musb_vbus_pin_detect_work(int irq, void *dev_id)
{
	int i4_vbus_state = 0;

	i4_vbus_state = gpio_get_value(vbus_detect_pin);
	DBG(0, "i4_vbus_state = %d\n", i4_vbus_state);

	if (1 == i4_vbus_state) {
		mt_usb_connect();
		switch_vbus_int_to_idle();
	} else {
		mt_usb_disconnect();
		switch_vbus_int_to_device();
	}
	return IRQ_HANDLED;
}
#endif

void mt_usb_init_drvvbus(void)
{
	int ret = 0;

	pr_debug("****%s:%d before Init Drive VBUS KS!!!!!\n", __func__, __LINE__);

	pinctrl_drvvbus = pinctrl_lookup_state(pinctrl, "drvvbus_init");
	if (IS_ERR(pinctrl_drvvbus)) {
		ret = PTR_ERR(pinctrl_drvvbus);
		dev_err(mtk_musb->controller, "Cannot find usb pinctrl drvvbus\n");
	}

	pinctrl_drvvbus_low = pinctrl_lookup_state(pinctrl, "drvvbus_low");
	if (IS_ERR(pinctrl_drvvbus_low)) {
		ret = PTR_ERR(pinctrl_drvvbus_low);
		dev_err(mtk_musb->controller, "Cannot find usb pinctrl drvvbus_low\n");
	}

	pinctrl_drvvbus_high = pinctrl_lookup_state(pinctrl, "drvvbus_high");
	if (IS_ERR(pinctrl_drvvbus_high)) {
		ret = PTR_ERR(pinctrl_drvvbus_high);
		dev_err(mtk_musb->controller, "Cannot find usb pinctrl drvvbus_high\n");
	}

	pinctrl_select_state(pinctrl, pinctrl_drvvbus);

#ifdef MTK_USB_VBUS_DETECT_SUPPORT
	vbus_detect_number = gpio_to_irq(vbus_detect_pin);
	ret = request_irq(vbus_detect_number, musb_vbus_pin_detect_work,
			  IRQF_TRIGGER_HIGH, "USB_VBUS_DETECT", NULL);
	if (ret > 0)
		pr_err("USB_VBUS_DETECT IRQ LINE not available!! vbus_detect_number = %d\n",
		       vbus_detect_number);
	else
		pr_debug("USB_VBUS_DETECT IRQ LINE not available!! vbus_detect_number = %d\n",
			 vbus_detect_number);
	DBG(0, "enable_irq vbus_detect_number:%d\n", vbus_detect_number);
#endif
	pr_debug("****%s:%d end Init Drive VBUS KS!!!!!\n", __func__, __LINE__);

}

u32 sw_deboun_time = 400;
module_param(sw_deboun_time, int, 0644);
/* extern int ep_config_from_table_for_host(struct musb *musb); */
static bool musb_is_host(void)
{
	u8 devctl = 0;
	int iddig_state = 1;
	bool usb_is_host = 0;

	DBG(0, "will mask PMIC charger detection\n");
#ifndef FPGA_PLATFORM
	pmic_chrdet_int_en(0);
#endif

	musb_platform_enable(mtk_musb);

#ifdef ID_PIN_USE_EX_EINT
	/* iddig_state = mt_get_gpio_in(GPIO28); */
	iddig_state = gpio_get_value(iddig_pin);
	DBG(0, "iddig_state = %d\n", iddig_state);
#else
	iddig_state = 0;
	devctl = musb_readb(mtk_musb->mregs, MUSB_DEVCTL);
	DBG(0, "devctl = %x before end session\n", devctl);
	devctl &= ~MUSB_DEVCTL_SESSION;	/* this will cause A-device change back to B-device after A-cable plug out */
	musb_writeb(mtk_musb->mregs, MUSB_DEVCTL, devctl);
	msleep(delay_time);

	devctl = musb_readb(mtk_musb->mregs, MUSB_DEVCTL);
	DBG(0, "devctl = %x before set session\n", devctl);

	devctl |= MUSB_DEVCTL_SESSION;
	musb_writeb(mtk_musb->mregs, MUSB_DEVCTL, devctl);
	msleep(delay_time1);
	devctl = musb_readb(mtk_musb->mregs, MUSB_DEVCTL);
	DBG(0, "devclt = %x\n", devctl);
#endif
	if (devctl & MUSB_DEVCTL_BDEVICE || iddig_state) {
		DBG(0, "will unmask PMIC charger detection\n");
#ifndef FPGA_PLATFORM
		pmic_chrdet_int_en(1);
#endif
		usb_is_host = false;
	} else {
		usb_is_host = true;
	}
	DBG(0, "usb_is_host = %d\n", usb_is_host);
	return usb_is_host;
}

void musb_session_restart(struct musb *musb)
{
	void __iomem *mbase = musb->mregs;

	musb_writeb(mbase, MUSB_DEVCTL, (musb_readb(mbase, MUSB_DEVCTL) & (~MUSB_DEVCTL_SESSION)));
	DBG(0, "[MUSB] stopped session for VBUSERROR interrupt\n");
	USBPHY_SET8(0x6d, 0x3c);
	USBPHY_SET8(0x6c, 0x10);
	USBPHY_CLR8(0x6c, 0x2c);
	DBG(0, "[MUSB] force PHY to idle, 0x6d=%x, 0x6c=%x\n", USBPHY_READ8(0x6d),
	    USBPHY_READ8(0x6c));
	mdelay(5);
	USBPHY_CLR8(0x6d, 0x3c);
	USBPHY_CLR8(0x6c, 0x3c);
	DBG(0, "[MUSB] let PHY resample VBUS, 0x6d=%x, 0x6c=%x\n", USBPHY_READ8(0x6d),
	    USBPHY_READ8(0x6c));
	musb_writeb(mbase, MUSB_DEVCTL, (musb_readb(mbase, MUSB_DEVCTL) | MUSB_DEVCTL_SESSION));
	DBG(0, "[MUSB] restart session\n");
}

void switch_int_to_device(struct musb *musb)
{
#ifdef ID_PIN_USE_EX_EINT
	irq_set_irq_type(usb_iddig_number, IRQF_TRIGGER_HIGH);
	enable_irq(usb_iddig_number);
#else
	musb_writel(musb->mregs, USB_L1INTP, 0);
	musb_writel(musb->mregs, USB_L1INTM,
		    IDDIG_INT_STATUS | musb_readl(musb->mregs, USB_L1INTM));
#endif
	DBG(0, "switch_int_to_device is done\n");
}

void switch_int_to_host(struct musb *musb)
{
#ifdef ID_PIN_USE_EX_EINT
	irq_set_irq_type(usb_iddig_number, IRQF_TRIGGER_LOW);
	enable_irq(usb_iddig_number);
#else
	musb_writel(musb->mregs, USB_L1INTP, IDDIG_INT_STATUS);
	musb_writel(musb->mregs, USB_L1INTM,
		    IDDIG_INT_STATUS | musb_readl(musb->mregs, USB_L1INTM));
#endif
	DBG(0, "switch_int_to_host is done\n");

}

void switch_int_to_host_and_mask(struct musb *musb)
{

#ifdef ID_PIN_USE_EX_EINT
	irq_set_irq_type(usb_iddig_number, IRQF_TRIGGER_LOW);
	disable_irq(usb_iddig_number);
#else
	musb_writel(musb->mregs, USB_L1INTM,
		    (~IDDIG_INT_STATUS) & musb_readl(musb->mregs, USB_L1INTM));
	mb();
	musb_writel(musb->mregs, USB_L1INTP, IDDIG_INT_STATUS);
#endif
	DBG(0, "swtich_int_to_host_and_mask is done\n");

}

#ifdef CONFIG_MTK_MUSB_CARPLAY_SUPPORT
void musb_id_pin_work_host(struct work_struct *data)
{
#ifdef ID_PIN_USE_EX_EINT
	u8 devctl = 0;
#endif

	unsigned long flags;

	spin_lock_irqsave(&mtk_musb->lock, flags);
	musb_generic_disable(mtk_musb);
	spin_unlock_irqrestore(&mtk_musb->lock, flags);

	down(&mtk_musb->musb_lock);
	DBG(0, "work start, is_host=%d\n", mtk_musb->is_host);
	if (mtk_musb->in_ipo_off) {
		DBG(0, "do nothing due to in_ipo_off\n");
		goto out;
	}
	mtk_musb->is_host = true;
	DBG(0, "musb is as %s\n", mtk_musb->is_host ? "host" : "device");

	if (mtk_musb->is_host) {

		/* setup fifo for host mode */
		ep_config_from_table_for_host(mtk_musb);
		wake_lock(&mtk_musb->usb_lock);
		msleep(100);

#ifdef ID_PIN_USE_EX_EINT
		/* clear session */
		devctl = musb_readb(mtk_musb->mregs, MUSB_DEVCTL);
		musb_writeb(mtk_musb->mregs, MUSB_DEVCTL, (devctl & (~MUSB_DEVCTL_SESSION)));
		/* USB MAC OFF */
		/* VBUSVALID=0, AVALID=0, BVALID=0, SESSEND=1, IDDIG=X */
		USBPHY_SET8(0x6c, 0x10);
		USBPHY_CLR8(0x6c, 0x2e);
		USBPHY_SET8(0x6d, 0x3e);
		DBG(0, "force PHY to idle, 0x6d=%x, 0x6c=%x\n", USBPHY_READ8(0x6d),
		    USBPHY_READ8(0x6c));
		/* wait */
		mdelay(5);
		/* restart session */
		devctl = musb_readb(mtk_musb->mregs, MUSB_DEVCTL);
		musb_writeb(mtk_musb->mregs, MUSB_DEVCTL, (devctl | MUSB_DEVCTL_SESSION));
		/* USB MAC ONand Host Mode */
		/* VBUSVALID=1, AVALID=1, BVALID=1, SESSEND=0, IDDIG=0 */
		USBPHY_CLR8(0x6c, 0x10);
		USBPHY_SET8(0x6c, 0x2c);
		USBPHY_SET8(0x6d, 0x3e);
		DBG(0, "force PHY to host mode, 0x6d=%x, 0x6c=%x\n", USBPHY_READ8(0x6d),
		    USBPHY_READ8(0x6c));
#endif
		musb_start(mtk_musb);
		MUSB_HST_MODE(mtk_musb);
#ifdef CONFIG_PM
		mtk_musb->is_active = 0;
		DBG(0, "set active to 0 in Pm runtime issue\n");
#endif
	}
out:
	DBG(0, "work end, is_host=%d\n", mtk_musb->is_host);
	up(&mtk_musb->musb_lock);


}

void musb_id_pin_work_device(void)
{
	unsigned long flags;

	spin_lock_irqsave(&mtk_musb->lock, flags);
	musb_generic_disable(mtk_musb);
	spin_unlock_irqrestore(&mtk_musb->lock, flags);

	down(&mtk_musb->musb_lock);
	DBG(0, "work start, is_host=%d\n", mtk_musb->is_host);
	if (mtk_musb->in_ipo_off) {
		DBG(0, "do nothing due to in_ipo_off\n");
		goto out;
	}
	mtk_musb->is_host = false;
	DBG(0, "musb is as %s\n", mtk_musb->is_host ? "host" : "device");

	DBG(0, "devctl is %x\n", musb_readb(mtk_musb->mregs, MUSB_DEVCTL));
	musb_writeb(mtk_musb->mregs, MUSB_DEVCTL, 0);
	if (wake_lock_active(&mtk_musb->usb_lock))
		wake_unlock(&mtk_musb->usb_lock);
#ifdef ID_PIN_USE_EX_EINT
	/* USB MAC OFF */
	/* VBUSVALID=0, AVALID=0, BVALID=0, SESSEND=1, IDDIG=X */
	USBPHY_SET8(0x6c, 0x10);
	USBPHY_CLR8(0x6c, 0x2e);
	USBPHY_SET8(0x6d, 0x3e);
	DBG(0, "force PHY to idle, 0x6d=%x, 0x6c=%x\n", USBPHY_READ8(0x6d), USBPHY_READ8(0x6c));
#endif
	musb_stop(mtk_musb);
	/* ALPS00849138 */
	mtk_musb->xceiv->otg->state = OTG_STATE_B_IDLE;
	MUSB_DEV_MODE(mtk_musb);
	musb_start(mtk_musb);
out:
	DBG(0, "work end, is_host=%d\n", mtk_musb->is_host);
	up(&mtk_musb->musb_lock);
}
#endif

#ifdef CONFIG_MTK_MUSB_SW_WITCH_MODE
void musb_id_pin_sw_work(bool host_mode)
{
#ifdef ID_PIN_USE_EX_EINT
	u8 devctl;
#endif

	unsigned long flags;

	spin_lock_irqsave(&mtk_musb->lock, flags);
	musb_generic_disable(mtk_musb);
	spin_unlock_irqrestore(&mtk_musb->lock, flags);

	down(&mtk_musb->musb_lock);
	DBG(0, "work start, is_host=%d\n", mtk_musb->is_host);
	if (mtk_musb->in_ipo_off) {
		DBG(0, "do nothing due to in_ipo_off\n");
		goto out;
	}
	mtk_musb->is_host = host_mode;
	musb_platform_enable(mtk_musb);
	DBG(0, "musb is as %s\n", mtk_musb->is_host ? "host" : "device");

	if (mtk_musb->is_host) {
		/* setup fifo for host mode */
		ep_config_from_table_for_host(mtk_musb);
		wake_lock(&mtk_musb->usb_lock);

		musb_platform_set_vbus(mtk_musb, 1);
		DBG(0, "not P2_PROJECT\n");

		/* for no VBUS sensing IP */
		/* wait VBUS ready */
		msleep(100);
#ifdef ID_PIN_USE_EX_EINT
		/* clear session */
		devctl = musb_readb(mtk_musb->mregs, MUSB_DEVCTL);
		musb_writeb(mtk_musb->mregs, MUSB_DEVCTL, (devctl & (~MUSB_DEVCTL_SESSION)));
		/* USB MAC OFF */
		/* VBUSVALID=0, AVALID=0, BVALID=0, SESSEND=1, IDDIG=X */
		USBPHY_SET8(0x6c, 0x10);
		USBPHY_CLR8(0x6c, 0x2e);
		USBPHY_SET8(0x6d, 0x3e);
		DBG(0, "force PHY to idle, 0x6d=%x, 0x6c=%x\n", USBPHY_READ8(0x6d),
		    USBPHY_READ8(0x6c));
		/* wait */
		mdelay(5);
		/* restart session */
		devctl = musb_readb(mtk_musb->mregs, MUSB_DEVCTL);
		musb_writeb(mtk_musb->mregs, MUSB_DEVCTL, (devctl | MUSB_DEVCTL_SESSION));
		/* USB MAC ONand Host Mode */
		/* VBUSVALID=1, AVALID=1, BVALID=1, SESSEND=0, IDDIG=0 */
		USBPHY_CLR8(0x6c, 0x10);
		USBPHY_SET8(0x6c, 0x2c);
		USBPHY_SET8(0x6d, 0x3e);
		DBG(0, "force PHY to host mode, 0x6d=%x, 0x6c=%x\n", USBPHY_READ8(0x6d),
		    USBPHY_READ8(0x6c));
#endif
		musb_start(mtk_musb);
		MUSB_HST_MODE(mtk_musb);
		switch_int_to_device(mtk_musb);
#ifdef CONFIG_PM
		mtk_musb->is_active = 0;
		DBG(0, "set active to 0 in Pm runtime issue\n");
#endif
	} else {
		DBG(0, "devctl is %x\n", musb_readb(mtk_musb->mregs, MUSB_DEVCTL));
		musb_writeb(mtk_musb->mregs, MUSB_DEVCTL, 0);
		if (wake_lock_active(&mtk_musb->usb_lock))
			wake_unlock(&mtk_musb->usb_lock);
		musb_platform_set_vbus(mtk_musb, 0);
		/* for no VBUS sensing IP */
#ifdef ID_PIN_USE_EX_EINT
		/* USB MAC OFF */
		/* VBUSVALID=0, AVALID=0, BVALID=0, SESSEND=1, IDDIG=X */
		USBPHY_SET8(0x6c, 0x10);
		USBPHY_CLR8(0x6c, 0x2e);
		USBPHY_SET8(0x6d, 0x3e);
		DBG(0, "force PHY to idle, 0x6d=%x, 0x6c=%x\n", USBPHY_READ8(0x6d),
		    USBPHY_READ8(0x6c));
#endif
		musb_stop(mtk_musb);
		/* ALPS00849138 */
		mtk_musb->xceiv->otg->state = OTG_STATE_B_IDLE;
		MUSB_DEV_MODE(mtk_musb);
		switch_int_to_host(mtk_musb);
	}
out:
	DBG(0, "work end, is_host=%d\n", mtk_musb->is_host);
	up(&mtk_musb->musb_lock);
}
#endif

static void musb_id_pin_work(struct work_struct *data)
{
#ifdef ID_PIN_USE_EX_EINT
	u8 devctl = 0;
#endif

	unsigned long flags;

#ifdef P2_PROJECT
	musb_platform_set_vbus(mtk_musb, 1);
	msleep(100);
	DBG(0, "P2_PROJECT\n");
#endif

	spin_lock_irqsave(&mtk_musb->lock, flags);
	musb_generic_disable(mtk_musb);
	spin_unlock_irqrestore(&mtk_musb->lock, flags);

	down(&mtk_musb->musb_lock);
	DBG(0, "work start, is_host=%d\n", mtk_musb->is_host);
	if (mtk_musb->in_ipo_off) {
		DBG(0, "do nothing due to in_ipo_off\n");
		goto out;
	}
	mtk_musb->is_host = musb_is_host();
	DBG(0, "musb is as %s\n", mtk_musb->is_host ? "host" : "device");

	if (mtk_musb->is_host) {

		/* setup fifo for host mode */
		ep_config_from_table_for_host(mtk_musb);
		wake_lock(&mtk_musb->usb_lock);

#ifndef P2_PROJECT
		musb_platform_set_vbus(mtk_musb, 1);
		DBG(0, "not P2_PROJECT\n");
#endif

		/* for no VBUS sensing IP */
		/* wait VBUS ready */
		msleep(100);
#ifdef ID_PIN_USE_EX_EINT
		/* clear session */
		devctl = musb_readb(mtk_musb->mregs, MUSB_DEVCTL);
		musb_writeb(mtk_musb->mregs, MUSB_DEVCTL, (devctl & (~MUSB_DEVCTL_SESSION)));
		/* USB MAC OFF */
		/* VBUSVALID=0, AVALID=0, BVALID=0, SESSEND=1, IDDIG=X */
		USBPHY_SET8(0x6c, 0x10);
		USBPHY_CLR8(0x6c, 0x2e);
		USBPHY_SET8(0x6d, 0x3e);
		DBG(0, "force PHY to idle, 0x6d=%x, 0x6c=%x\n", USBPHY_READ8(0x6d),
		    USBPHY_READ8(0x6c));
		DBG(0, "mtk_musb->mregs=0x%x, mtk_musb->xceiv->io_priv=0x%x\n", 
			(unsigned int)mtk_musb->mregs, (unsigned int)mtk_musb->xceiv->io_priv); 
		/* wait */
		mdelay(5);
		/* restart session */
		devctl = musb_readb(mtk_musb->mregs, MUSB_DEVCTL);
		musb_writeb(mtk_musb->mregs, MUSB_DEVCTL, (devctl | MUSB_DEVCTL_SESSION));
		/* USB MAC ONand Host Mode */
		/* VBUSVALID=1, AVALID=1, BVALID=1, SESSEND=0, IDDIG=0 */
		USBPHY_CLR8(0x6c, 0x10);
		USBPHY_SET8(0x6c, 0x2c);
		USBPHY_SET8(0x6d, 0x3e);
		DBG(0, "force PHY to host mode, 0x6d=%x, 0x6c=%x\n", USBPHY_READ8(0x6d),
		    USBPHY_READ8(0x6c));
#endif
		musb_start(mtk_musb);
		MUSB_HST_MODE(mtk_musb);
		switch_int_to_device(mtk_musb);
#ifdef CONFIG_PM
		mtk_musb->is_active = 0;
		DBG(0, "set active to 0 in Pm runtime issue\n");
#endif
	} else {
		DBG(0, "devctl is %x\n", musb_readb(mtk_musb->mregs, MUSB_DEVCTL));
		musb_writeb(mtk_musb->mregs, MUSB_DEVCTL, 0);
		if (wake_lock_active(&mtk_musb->usb_lock))
			wake_unlock(&mtk_musb->usb_lock);
		musb_platform_set_vbus(mtk_musb, 0);
		/* for no VBUS sensing IP */
#ifdef ID_PIN_USE_EX_EINT
		/* USB MAC OFF */
		/* VBUSVALID=0, AVALID=0, BVALID=0, SESSEND=1, IDDIG=X */
		USBPHY_SET8(0x6c, 0x10);
		USBPHY_CLR8(0x6c, 0x2e);
		USBPHY_SET8(0x6d, 0x3e);
		DBG(0, "force PHY to idle, 0x6d=%x, 0x6c=%x\n", USBPHY_READ8(0x6d),
		    USBPHY_READ8(0x6c));
#endif
		musb_stop(mtk_musb);
		/* ALPS00849138 */
		mtk_musb->xceiv->otg->state = OTG_STATE_B_IDLE;
		MUSB_DEV_MODE(mtk_musb);
		switch_int_to_host(mtk_musb);
	}
out:
	DBG(0, "work end, is_host=%d\n", mtk_musb->is_host);
	up(&mtk_musb->musb_lock);


}

#ifdef ID_PIN_USE_EX_EINT
static irqreturn_t mt_usb_ext_iddig_int(int irq, void *dev_id)
{
#ifndef CONFIG_MTK_MUSB_SW_WITCH_MODE
	DBG(0, "id pin interrupt assert, mtk_musb->is_ready:%d\n", mtk_musb->is_ready);
	if (!mtk_musb->is_ready) {
		/* dealy 10 sec if usb function is not ready */
		schedule_delayed_work(&mtk_musb->id_pin_work, 5000 * HZ / 1000);
	} else {
		schedule_delayed_work(&mtk_musb->id_pin_work, sw_deboun_time * HZ / 1000);
	}
	DBG(0, "id pin interrupt assert, mtk_musb->is_ready:%d\n", mtk_musb->is_ready);
#endif
	disable_irq_nosync(usb_iddig_number);
	return IRQ_HANDLED;
}
#endif

void mt_usb_iddig_int(struct musb *musb)
{
	u32 usb_l1_ploy = musb_readl(musb->mregs, USB_L1INTP);

	DBG(0, "id pin interrupt assert,polarity=0x%x\n", usb_l1_ploy);
	if (usb_l1_ploy & IDDIG_INT_STATUS)
		usb_l1_ploy &= (~IDDIG_INT_STATUS);
	else
		usb_l1_ploy |= IDDIG_INT_STATUS;

	musb_writel(musb->mregs, USB_L1INTP, usb_l1_ploy);
	musb_writel(musb->mregs, USB_L1INTM,
		    (~IDDIG_INT_STATUS) & musb_readl(musb->mregs, USB_L1INTM));

	if (!mtk_musb->is_ready) {
		/* dealy 5 sec if usb function is not ready */
		schedule_delayed_work(&mtk_musb->id_pin_work, 5000 * HZ / 1000);

	} else {
		schedule_delayed_work(&mtk_musb->id_pin_work, sw_deboun_time * HZ / 1000);
	}
}

static void otg_int_init(void)
{

#ifdef CONFIG_OF
	int ret = 0;
#ifndef ID_PIN_USE_EX_EINT
	u32 phy_id_pull = 0;
#endif

	pr_debug("****%s:%d before Init IDDIG KS!!!!!\n", __func__, __LINE__);
	pinctrl_iddig = pinctrl_lookup_state(pinctrl, "iddig_irq_init");
	if (IS_ERR(pinctrl_iddig)) {
		ret = PTR_ERR(pinctrl_iddig);
		dev_err(mtk_musb->controller, "Cannot find usb pinctrl iddig_irq_init\n");
	}
	pinctrl_select_state(pinctrl, pinctrl_iddig);
	pr_debug("****%s:%d end Init IDDIG KS!!!!!\n", __func__, __LINE__);

#ifdef ID_PIN_USE_EX_EINT
	/* gpio_request(iddig_pin, "USB_IDDIG"); */
	/* gpio_set_debounce(iddig_pin, 64); */
	/* usb_iddig_number = irq_of_parse_and_map(iddig_node, 0); */
	usb_iddig_number = gpio_to_irq(iddig_pin);
	/* usb_iddig_number = irq_of_parse_and_map(iddig_node,0); */
	ret = request_irq(usb_iddig_number, mt_usb_ext_iddig_int,
			  IRQF_TRIGGER_LOW, "USB_IDDIG", NULL);
	if (ret > 0)
		pr_err("USB IDDIG IRQ LINE not available!! usb_iddig_number = %d\n",
		       usb_iddig_number);
	else
		pr_debug("USB IDDIG IRQ LINE available!! usb_iddig_number= %d\n", usb_iddig_number);
#else
	/* u32 phy_id_pull = 0; */

	phy_id_pull = __raw_readl((void __iomem *)U2PHYDTM1);
	phy_id_pull |= ID_PULL_UP;
	__raw_writel(phy_id_pull, (void __iomem *)U2PHYDTM1);

	musb_writel(mtk_musb->mregs, USB_L1INTM,
		    IDDIG_INT_STATUS | musb_readl(mtk_musb->mregs, USB_L1INTM));
#endif
#endif


}


void mt_usb_otg_init(struct musb *musb)
{
#ifdef CONFIG_OF
	int ret = 0;

	usb_node = of_find_compatible_node(NULL, NULL, "mediatek,USB0");
	if (usb_node == NULL)
		pr_err("USB OTG - get USB0 node failed\n");

#ifdef ID_PIN_USE_EX_EINT
	iddig_node = of_get_child_by_name(usb_node, "otg-iddig");
	if (iddig_node == NULL)
		pr_err("USB IDDIG EINT - get IDDIG EINT node failed\n");

	iddig_pin = of_get_named_gpio(iddig_node, "iddig_gpio", 0);
#else
	iddig_pin = of_get_named_gpio(usb_node, "iddig_gpio", 0);

#endif
#ifdef MTK_USB_VBUS_DETECT_SUPPORT
	vbus_detect_node = of_get_child_by_name(usb_node, "vbus-detect");
	if (vbus_detect_node == NULL)
		pr_err("USB IDDIG EINT - get VBUS EINT node failed\n");

	vbus_detect_pin = of_get_named_gpio(vbus_detect_node, "vbus_gpio", 0);

	ret = gpio_request(vbus_detect_pin, "vbus_gpio");
	if (ret != 0)
		pr_err("gpio_request vbus_gpio fail\n");
#endif

	drvvbus_pin = of_get_named_gpio(usb_node, "drvvbus_gpio", 0);

	ret = gpio_request(iddig_pin, "iddig_gpio");
	if (ret != 0)
		pr_err("gpio_request iddig_gpio fail\n");

	ret = gpio_request(drvvbus_pin, "drvvbus_gpio");
	if (ret != 0)
		pr_err("gpio_request drvvbus_gpio fail\n");

	/* gpio_direction_input(iddig_pin); */

	gpio_direction_output(drvvbus_pin, 0);


	pinctrl = devm_pinctrl_get(mtk_musb->controller);
	if (IS_ERR(pinctrl)) {
		ret = PTR_ERR(pinctrl);
		dev_err(mtk_musb->controller, "Cannot find usb pinctrl!\n");
	}
#endif

	/*init drrvbus */
	mt_usb_init_drvvbus();
	DBG(0, "mt_usb_otg_init\n");

	/* EP table */
	musb->fifo_cfg_host = fifo_cfg_host;
	musb->fifo_cfg_host_size = ARRAY_SIZE(fifo_cfg_host);

	INIT_DELAYED_WORK(&musb->id_pin_work, musb_id_pin_work);
#ifdef CONFIG_MTK_MUSB_CARPLAY_SUPPORT
	INIT_DELAYED_WORK(&musb->carplay_work, musb_id_pin_work_host);
#endif

	/* init idpin interrupt */
	otg_int_init();
}


#if 0
/* #else */

/* for not define CONFIG_USB_MTK_OTG */
void mt_usb_otg_init(struct musb *musb)
{
}

void mt_usb_init_drvvbus(void)
{
}

void mt_usb_set_vbus(struct musb *musb, int is_on)
{
}

int mt_usb_get_vbus_status(struct musb *musb)
{
	return 1;
}

void mt_usb_iddig_int(struct musb *musb)
{
}

void switch_int_to_device(struct musb *musb)
{
}

void switch_int_to_host(struct musb *musb)
{
}

void switch_int_to_host_and_mask(struct musb *musb)
{
}

void musb_session_restart(struct musb *musb)
{
}
#endif
/* #endif */
/* #endif */
