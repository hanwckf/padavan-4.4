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

# platform driver
snd-soc-mt7622-afe-objs := mt7622-afe-pcm.o mt7622-afe-clock-ctrl.o
obj-$(CONFIG_SND_SOC_MT7622) += snd-soc-mt7622-afe.o

# machine driver
obj-$(CONFIG_SND_SOC_MT7622_DUMMY) += dummy-codec.o mt7622-machine.o
obj-$(CONFIG_SND_SOC_MT7622_WM8960) += mt7622-wm8960.o