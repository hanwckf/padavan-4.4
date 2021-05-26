obj-$(CONFIG_RA_HW_NAT) += hw_nat.o
hw_nat-objs := ra_nat.o foe_fdb.o util.o hwnat_ioctl.o api.o
ifeq ($(CONFIG_RA_HW_NAT_PPTP_L2TP),y)
hw_nat-objs += fast_path.o
hw_nat-objs += pptp_l2tp_fdb.o
endif
ccflags-y += -Idrivers/net/ethernet/raeth
ifneq ($(CONFIG_HNAT_V2),y)
#hw_nat-objs += acl_policy.o acl_ioctl.o
#hw_nat-objs += ac_policy.o ac_ioctl.o
#hw_nat-objs += mtr_policy.o mtr_ioctl.o
endif

ifeq ($(CONFIG_PPE_MCAST),y)
hw_nat-objs += mcast_tbl.o
endif

ifeq ($(CONFIG_RAETH_GMAC2),y)
	EXTRA_CFLAGS += -DCONFIG_PSEUDO_SUPPORT
endif

ifeq ($(CONFIG_MTK_EMI_7622),y)
	EXTRA_CFLAGS += -DCONFIG_ARCH_MT7622
endif
