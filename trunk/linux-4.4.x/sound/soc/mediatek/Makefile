subdir-ccflags-y += -Werror

obj-$(CONFIG_SND_SOC_MEDIATEK) += common/
obj-$(CONFIG_SND_SOC_MT7622) += mt7622/
obj-$(CONFIG_SND_SOC_MT2701) += mt2701/
obj-$(CONFIG_SND_SOC_MT8173) += mt8173/
obj-$(CONFIG_SND_SOC_MT7621) += mt7621/

ifneq ($(CONFIG_MT_SND_SOC_8521P),)
obj-$(CONFIG_MT_SND_SOC_8521P) += mt8521p/
endif
