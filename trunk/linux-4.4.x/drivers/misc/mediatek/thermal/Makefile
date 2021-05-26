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

ccflags-y += -I$(srctree)

ifeq ($(CONFIG_MACH_MT8521P), y)
# MT2701 & MT8521P use the same thermal driver.
THERMAL_CHIP_DRIVER_DIR := $(srctree)/drivers/misc/mediatek/thermal/mt8127
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/mt8127/include
else ifeq ($(CONFIG_MTK_BASE_POWER_MT7622), y)
THERMAL_CHIP_DRIVER_DIR := $(srctree)/drivers/misc/mediatek/thermal/mt7622
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/mt7622/include
endif

ccflags-y  += -I$(THERMAL_CHIP_DRIVER_DIR)/inc
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/thermal/fakeHeader/
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/base/power/$(MTK_PLATFORM)
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/include/

obj-$(CONFIG_THERMAL) += mtk_thermal_monitor.o
obj-$(CONFIG_THERMAL) += mtk_cooler_shutdown.o
ifeq ($(CONFIG_MTK_LEDS), y)
    # setMaxbrightness() defined in mediatek/kernel/drivers/leds/leds_drv.c only built when CONFIG_MTK_LEDS is set
    obj-$(CONFIG_THERMAL) += mtk_cooler_backlight.o
endif
obj-$(CONFIG_THERMAL) += mtk_cooler_kshutdown.o
#obj-$(CONFIG_THERMAL) += mtk_cooler_spkrvol.o
#obj-$(CONFIG_THERMAL) += mtk_cooler_dropcall.o
obj-$(CONFIG_THERMAL) += mtk_cooler_vrt.o

ifeq ($(CONFIG_MACH_MT8521P), y)
# MT2701 & MT7623 use the same thermal driver.
obj-y += mt8127/
else ifeq ($(CONFIG_MTK_BASE_POWER_MT7622), y)
obj-y += mt7622/
else
obj-y += $(subst ",,$(CONFIG_MTK_PLATFORM))/
endif
