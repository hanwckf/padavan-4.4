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

subdir-ccflags-y += -I$(srctree)/drivers/misc/mediatek/freqhopping/
subdir-ccflags-$(CONFIG_MACH_MT8521P) += -I$(srctree)/drivers/misc/mediatek/freqhopping/mt8127/
subdir-ccflags-$(CONFIG_MACH_MT8521P) += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/mt8127/include

obj-y := mt_freqhopping_drv.o

obj-$(CONFIG_MACH_MT8521P)  += mt8127/

