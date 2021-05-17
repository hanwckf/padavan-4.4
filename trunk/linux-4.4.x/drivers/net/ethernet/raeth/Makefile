obj-$(CONFIG_RAETH) += raeth.o
raeth-objs := raether.o raether_pdma.o ra_mac.o mii_mgr.o ra_switch.o ra_dbg_proc.o
raeth-objs += raether_qdma.o
raeth-objs += raether_rss.o

ifeq ($(CONFIG_RAETH_ETHTOOL),y)
raeth-objs += ra_ethtool.o
endif

raeth-objs += raether_hwlro.o
raeth-objs += ra_dbg_hwlro.o
raeth-objs += ra_dbg_hwioc.o

ifeq ($(CONFIG_RAETH_ESW_CONTROL),y)
raeth-objs += mtk_esw/ioctl_mt762x.o
endif

ccflags-y += -Idrivers/net/ethernet/raeth/rtl8367c/include  -D_LITTLE_ENDIAN -DMDC_MDIO_OPERATION
ccflags-y += -Idrivers/net/ethernet/raeth
ccflags-y += -Iinclude/linux/
raeth-objs += rtl8367c/acl.o
raeth-objs += rtl8367c/cpu.o
raeth-objs += rtl8367c/dot1x.o
raeth-objs += rtl8367c/eee.o
raeth-objs += rtl8367c/igmp.o
raeth-objs += rtl8367c/interrupt.o
raeth-objs += rtl8367c/l2.o
raeth-objs += rtl8367c/leaky.o
raeth-objs += rtl8367c/led.o
raeth-objs += rtl8367c/mirror.o
raeth-objs += rtl8367c/oam.o
raeth-objs += rtl8367c/port.o
raeth-objs += rtl8367c/ptp.o
raeth-objs += rtl8367c/qos.o
raeth-objs += rtl8367c/rate.o
raeth-objs += rtl8367c/rldp.o
raeth-objs += rtl8367c/rtk_switch.o
raeth-objs += rtl8367c/rtk_hal.o
raeth-objs += rtl8367c/rtl8367c_asicdrv_acl.o
raeth-objs += rtl8367c/rtl8367c_asicdrv.o
raeth-objs += rtl8367c/rtl8367c_asicdrv_cputag.o
raeth-objs += rtl8367c/rtl8367c_asicdrv_dot1x.o
raeth-objs += rtl8367c/rtl8367c_asicdrv_eav.o
raeth-objs += rtl8367c/rtl8367c_asicdrv_eee.o
raeth-objs += rtl8367c/rtl8367c_asicdrv_fc.o
raeth-objs += rtl8367c/rtl8367c_asicdrv_green.o
raeth-objs += rtl8367c/rtl8367c_asicdrv_hsb.o
raeth-objs += rtl8367c/rtl8367c_asicdrv_igmp.o
raeth-objs += rtl8367c/rtl8367c_asicdrv_inbwctrl.o
raeth-objs += rtl8367c/rtl8367c_asicdrv_interrupt.o
raeth-objs += rtl8367c/rtl8367c_asicdrv_led.o
raeth-objs += rtl8367c/rtl8367c_asicdrv_lut.o
raeth-objs += rtl8367c/rtl8367c_asicdrv_meter.o
raeth-objs += rtl8367c/rtl8367c_asicdrv_mib.o
raeth-objs += rtl8367c/rtl8367c_asicdrv_mirror.o
raeth-objs += rtl8367c/rtl8367c_asicdrv_misc.o
raeth-objs += rtl8367c/rtl8367c_asicdrv_oam.o
raeth-objs += rtl8367c/rtl8367c_asicdrv_phy.o
raeth-objs += rtl8367c/rtl8367c_asicdrv_port.o
raeth-objs += rtl8367c/rtl8367c_asicdrv_portIsolation.o
raeth-objs += rtl8367c/rtl8367c_asicdrv_qos.o
raeth-objs += rtl8367c/rtl8367c_asicdrv_rldp.o
raeth-objs += rtl8367c/rtl8367c_asicdrv_rma.o
raeth-objs += rtl8367c/rtl8367c_asicdrv_scheduling.o
raeth-objs += rtl8367c/rtl8367c_asicdrv_storm.o
raeth-objs += rtl8367c/rtl8367c_asicdrv_svlan.o
raeth-objs += rtl8367c/rtl8367c_asicdrv_trunking.o
raeth-objs += rtl8367c/rtl8367c_asicdrv_unknownMulticast.o
raeth-objs += rtl8367c/rtl8367c_asicdrv_vlan.o
raeth-objs += rtl8367c/smi.o
raeth-objs += rtl8367c/stat.o
raeth-objs += rtl8367c/storm.o
raeth-objs += rtl8367c/svlan.o
raeth-objs += rtl8367c/trap.o
raeth-objs += rtl8367c/trunk.o
raeth-objs += rtl8367c/vlan.o

ifeq ($(CONFIG_RAETH_PDMA_DVT),y)
raeth-objs += dvt/raether_pdma_dvt.o
obj-m += dvt/pkt_gen.o
obj-m += dvt/pkt_gen_udp_frag.o
obj-m += dvt/pkt_gen_tcp_frag.o
endif

ccflags-y += -Werror
