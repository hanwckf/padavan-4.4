#
# Makefile for the linux FAT12/16/32(VFAT)/64(exFAT) filesystem driver.
#

ifneq ($(KERNELRELEASE),)
# Called from inline kernel build
# DKMS_DEFINE
obj-$(CONFIG_EXFAT_FS) += exfat.o

exfat-objs	:= super.o core.o core_exfat.o blkdev.o fatent.o cache.o \
		   nls.o misc.o extent.o xattr.o
else
# Called from external kernel module build

KERNELRELEASE	?= $(shell uname -r)
KDIR	?= /lib/modules/${KERNELRELEASE}/build
MDIR	?= /lib/modules/${KERNELRELEASE}
PWD	:= $(shell pwd)

export CONFIG_EXFAT_FS := m

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

help:
	$(MAKE) -C $(KDIR) M=$(PWD) help

install: exfat.ko
	rm -f ${MDIR}/kernel/fs/exfat/exfat.ko
	install -m644 -b -D exfat.ko ${MDIR}/kernel/fs/exfat/exfat.ko
	depmod -aq

uninstall:
	rm -rf ${MDIR}/kernel/fs/exfat
	depmod -aq

endif

.PHONY : all clean install uninstall
