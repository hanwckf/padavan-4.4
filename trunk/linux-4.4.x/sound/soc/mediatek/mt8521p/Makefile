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

ccflags-y += -DDEBUG

# include path
subdir-ccflags-y += -I$(srctree)/sound/soc/codecs

# platform driver
# platform
obj-$(CONFIG_MT_SND_SOC_8521P) += mt8521p-lp-audio.o
# platform
obj-$(CONFIG_MT_SND_SOC_8521P) += mt8521p-audio.o
# cpu dai
obj-$(CONFIG_MT_SND_SOC_8521P) += mt8521p-dai.o
# hardware
obj-$(CONFIG_MT_SND_SOC_8521P) += mt8521p-afe.o mt8521p-afe-clk.o mt8521p-aud-gpio.o

# codec driver
obj-$(CONFIG_MT_SND_SOC_8521P) += dummy-codec.o pcm1795.o pcm4202.o pcm1861.o pcm5102a.o

# others
obj-$(CONFIG_MT_SND_SOC_8521P) += mt8521p-asrc.o mt8521p-audrtc.o mt8521p-register-debug.o mt8521p-afe-debug.o

# machine driver
obj-$(CONFIG_SND_SOC_MT8521P_EVB) += mt8521p-machine.o
obj-$(CONFIG_SND_SOC_MT8521P_RT5640) += mt8521p-rt5640.o
obj-$(CONFIG_SND_SOC_MT8521P_CS42448) += mt8521p-cs42448.o
obj-$(CONFIG_SND_SOC_MT7623_WM8960) += mt7623-wm8960.o
