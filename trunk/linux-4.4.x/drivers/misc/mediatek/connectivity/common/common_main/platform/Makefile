ifeq ($(CONFIG_MTK_COMBO),y)

ccflags-y += \
	-I$(src)/include \
	-I$(src)/../linux/include \
	-I$(src)/../core/include \
	-I$(src)/../include \
	-I$(src)/../../common_detect

ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/$(MTK_PLATFORM)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/include
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include/mach

subdir-ccflags-y += -D MTK_WCN_WMT_STP_EXP_SYMBOL_ABSTRACT

ifeq ($(CONFIG_MTK_CONN_LTE_IDC_SUPPORT),y)
	subdir-ccflags-y += -DWMT_IDC_SUPPORT=1
else
	subdir-ccflags-y += -DWMT_IDC_SUPPORT=0
endif

PLAT := alps
ifneq ($(filter "CONSYS_%",$(CONFIG_MTK_COMBO_CHIP)),)
obj-y	+= $(MTK_PLATFORM).o
endif
obj-y	+= wmt_plat_$(PLAT).o
#obj-y	+= wmt_plat_stub.o
obj-y	+= mtk_wcn_cmb_hw.o
obj-y	+= mtk_wcn_consys_hw.o

endif
