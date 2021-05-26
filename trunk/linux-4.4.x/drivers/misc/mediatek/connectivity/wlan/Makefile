ifeq ($(CONFIG_MTK_COMBO_WIFI), y)
	subdir-ccflags-y += -D MTK_WCN_BUILT_IN_DRIVER
	hif=sdio
	export CONFIG_MTK_COMBO_WIFI_HIF=$(hif)
	export CONFIG_MTK_WIFI_ONLY=n
endif

subdir-ccflags-y += \
	-I$(src)/../common/common_main/include \
	-I$(src)/../common/common_main/linux/include
#########################################################
# Wi-Fi character device driver

ccflags-y += -D CREATE_NODE_DYNAMIC=1

ifeq ($(CONFIG_MTK_TC1_FEATURE), y)
    ccflags-y += -DCFG_USE_AOSP_TETHERING_NAME
endif

ifeq ($(CONFIG_MTK_TC10_FEATURE), y)
    ccflags-y += -DCFG_USE_AOSP_TETHERING_NAME
endif

obj-$(CONFIG_MTK_COMBO_WIFI) += wmt_chrdev_wifi.o

#########################################################
# WLAN driver

ifneq ($(filter "MT6630",$(CONFIG_MTK_COMBO_CHIP)),)
	obj-y   +=  gen3/
endif

ifneq ($(filter "CONSYS_6797" "CONSYS_6759",$(CONFIG_MTK_COMBO_CHIP)),)
	obj-y   +=  gen3/
else ifneq ($(filter "CONSYS_%",$(CONFIG_MTK_COMBO_CHIP)),)
	obj-y   +=  gen2/
endif

ifneq ($(filter "MT6632",$(CONFIG_MTK_COMBO_CHIP)),)
	obj-y   +=  gen4/
endif
