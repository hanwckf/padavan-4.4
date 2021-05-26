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

ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/hps_v1/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/mt7622/include

obj-y += mt7622-cpufreq.o
obj-y += mtk_static_power.o
obj-y += mtk_power_throttle.o
obj-y += mtk_dcm.o
obj-y += mtk_static_power.o
obj-y += mtk_power_throttle.o
obj-y += mtk_spm.o
obj-y += mtk_spm_internal.o
obj-y += mtk_sleep.o
obj-y += mtk_spm_pcm.o
obj-y += mtk_spm_sleep.o
obj-y += mtk_spm_dpidle.o
obj-y += mtk_spm_fs.o
obj-y += mtk_cpuidle.o
obj-$(CONFIG_ARM) += cpu_dormant-v7.o
obj-$(CONFIG_ARM64) += cpu_dormant.o
obj-y += pmic_mt6380.o
obj-y += mtk_svs.o
