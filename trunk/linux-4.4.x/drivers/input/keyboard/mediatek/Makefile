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

subdir-ccflags-y += -I$(srctree)/drivers/input/keyboard/mediatek/$(CONFIG_KEYBOARD_MTK_PLAT)/
subdir-ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/
obj-$(CONFIG_KEYBOARD_MTK) := kpd.o

obj-y += $(subst ",,$(CONFIG_KEYBOARD_MTK_PLAT))/

