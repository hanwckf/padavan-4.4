#
# Copyright (C) 2015 MediaTek Inc.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#

#
# for USB OTG silicon based on Mentor Graphics INVENTRA designs
#
ccflags-$(CONFIG_USB_MTK_HDRC) += -I$(srctree)/drivers/misc/mediatek/usb20


ccflags-y += -DID_PIN_USE_EX_EINT
//ccflags-y += -DMTK_USB_NO_BATTERY

ccflags-y += -DMUSB_QMU_SUPPORT
#ccflags-y += -DMUSB_QMU_SUPPORT_HOST

ccflags-y += -DMTK_USB_VBUS_DETECT_SUPPORT
ccflags-y += -Werror

obj-$(CONFIG_USB_MTK_HDRC)	:= usb20.o usb20_phy.o
obj-$(CONFIG_USB_MTK_HDRC)  += usb20_host.o
obj-$(CONFIG_USBIF_COMPLIANCE)   += usb20_otg_if.o
obj-$(CONFIG_USB_MTK_HDRC)	+= usb20_phy_debugfs.o
obj-$(CONFIG_MTK_MUSB_CARPLAY_SUPPORT)  += otg.o
