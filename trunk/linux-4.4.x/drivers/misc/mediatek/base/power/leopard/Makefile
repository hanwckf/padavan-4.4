#
# Copyright (C) 2016 MediaTek Inc.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#

ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat

obj-y += mt_spm_mtcmos.o
obj-$(CONFIG_SMP) += mt-smp.o
obj-$(CONFIG_SMP) += hotplug.o
obj-$(CONFIG_SMP) += mt-headsmp.o
obj-$(CONFIG_SMP) += mt_hotplug.o
obj-y += mt_cache_v7.o
obj-y += mtk_dcm.o
