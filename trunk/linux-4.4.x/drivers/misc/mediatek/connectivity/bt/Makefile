#########################################################
# Bluetooth character device driver

ccflags-y += -D CREATE_NODE_DYNAMIC=1

ccflags-y += \
	-I$(src)/../common/common_main/include \
	-I$(src)/../common/common_main/linux/include

obj-$(CONFIG_MTK_COMBO_BT) += stp_chrdev_bt.o
