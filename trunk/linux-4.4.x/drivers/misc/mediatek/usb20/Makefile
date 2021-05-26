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

ifeq ($(MTK_ALPS_BOX_SUPPORT),yes)
ccflags-y += -DMTK_ALPS_BOX_SUPPORT
endif

#QMU Flags
ccflags-$(CONFIG_MTK_MUSB_QMU_SUPPORT) += -DMUSB_QMU_SUPPORT
##ccflags-$(CONFIG_MTK_MUSB_QMU_SUPPORT) += -DQMU_TASKLET

#Platform
ifneq (,$(filter y,$(CONFIG_MACH_MT2701) $(CONFIG_MACH_MT8521P) $(CONFIG_MACH_MT7623)))
    MDI_IP_FOLDER:=mt2701
endif

MTK_MDI_IP_SRC:= $(MDI_IP_FOLDER)/

#IP dependent part
obj-y += \
    $(MTK_MDI_IP_SRC)

#Driver
obj-$(CONFIG_USB_MTK_HDRC) += musb_hdrc.o
musb_hdrc-$(CONFIG_USB_MTK_HDRC) := musb_core.o
musb_hdrc-$(CONFIG_USB_MTK_HDRC) += musb_gadget_ep0.o musb_gadget.o
musb_hdrc-$(CONFIG_USB_MTK_HDRC) += musb_virthub.o musb_host.o
musb_hdrc-$(CONFIG_USB_MTK_HDRC) += musbhsdma.o
musb_hdrc-$(CONFIG_USB_MTK_HDRC) += musb_debugfs.o

# QMU Realted
obj-$(CONFIG_MTK_MUSB_QMU_SUPPORT) += mtk_qmu.o musb_qmu.o
