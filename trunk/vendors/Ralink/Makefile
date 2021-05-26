KERNELZ		= $(IMAGEDIR)/zImage
RAMDISK		= $(IMAGEDIR)/ramdisk
IMAGE		= $(IMAGEDIR)/$(CONFIG_FIRMWARE_PRODUCT_ID)_$(FIRMWARE_KERNEL_VER).$(FIRMWARE_ROOTFS_VER)-$(FIRMWARE_BUILDS_VER).trx
BOARD_DIR	= $(ROOTDIR)/configs/boards/$(CONFIG_FIRMWARE_PRODUCT_ID)
ROMFS_DIR	= $(ROOTDIR)/romfs
MKIMAGE_DIR	:= $(ROOTDIR)/tools/mkimage
CUR_DIR		= $(shell pwd)
ifeq ($(CONFIG_KERNEL_NO_COMPRESS),y)
COMP		= none
else
COMP		= lzma
endif

ifneq ($(filter y,$(CONFIG_MTD_NAND_MTK) $(CONFIG_MTD_NAND_RALINK)),)
PART_ALIGN	= 131072
else
PART_ALIGN	= 16
endif

ifeq ($(CONFIG_PRODUCT),MT7621)
LOAD_ADDR = 81001000
else
LOAD_ADDR = 80000000
endif

ifeq ($(CONFIG_WITHOUT_KERNEL),y)
ENTRY_CMD = $(MKIMAGE_DIR)/mkimage -l $(UBOOT_HEADER) 2>/dev/null |grep "Entry" |awk '{print $$3}'
else
ENTRY_CMD = readelf -h $(ROOTDIR)/$(LINUXDIR)/vmlinux | grep "Entry" | awk '{print $$4}'
endif

ROMFS_DIRS	= \
	bin \
	dev dev/pts \
	home \
	etc \
	etc_ro \
	etc_ro/ppp/peers etc_ro/ppp/plugins etc_ro/ppp/3g \
	etc_ro/Wireless \
	lib lib/modules \
	mnt \
	media \
	opt \
	proc \
	sbin \
	sys \
	usr usr/bin usr/sbin usr/lib/pppd usr/codepages \
	tmp \
	var

clean:
	rm -f $(RAMDISK)

romfs:
	[ -d $(ROMFSDIR)/$$i ] || mkdir -p $(ROMFSDIR)
	for i in $(ROMFS_DIRS); do \
		[ -d $(ROMFSDIR)/$$i ] || mkdir -p $(ROMFSDIR)/$$i; \
	done
	if [ -f $(BOARD_DIR)/l1profile.dat ]; then \
		cp -f $(BOARD_DIR)/l1profile.dat $(ROMFSDIR)/etc_ro/Wireless; \
	fi
	for i in `ls $(BOARD_DIR)/SingleSKU*.dat 2>/dev/null` ; do \
		[ -f $$i ] && cp -f $$i $(ROMFSDIR)/etc_ro/Wireless; \
	done

romfs.post::
	[ -d $(IMAGEDIR) ] || mkdir -p $(IMAGEDIR)
	rm -f $(RAMDISK)
	rm -f $(KERNELZ).*
	$(ROOTDIR)/tools/mksquashfs_xz/mksquashfs $(ROMFSDIR) $(RAMDISK) -all-root -no-exports -noappend -nopad -noI -no-xattrs

image:
	##############CREATE IMAGE#################
ifeq ($(CONFIG_WITHOUT_KERNEL),y)
	cd $(IMAGEDIR) ; rm -f $(KERNELZ).*; \
	cp -f $(MERGE_DIR)/$(CONFIG_FIRMWARE_PRODUCT_ID)/kernel.bin $(KERNELZ).$(COMP)
else
	$(CROSS_COMPILE)objcopy -O binary -R .note -R .comment -S $(ROOTDIR)/$(LINUXDIR)/vmlinux $(KERNELZ)
ifeq ($(CONFIG_KERNEL_NO_COMPRESS),y)
	cd $(IMAGEDIR); rm -f $(KERNELZ).*; mv -f $(KERNELZ) $(KERNELZ).$(COMP)
else
	cd $(IMAGEDIR); rm -f $(KERNELZ).*; $(ROOTDIR)/tools/lzma/lzma e -a1 -d25 $(KERNELZ) $(KERNELZ).$(COMP)
endif
endif
	@SIZE=`wc -c $(KERNELZ).$(COMP) | awk '{ print $$1 }'` ; \
	SIZE=`expr $$SIZE + 64` ; \
	DIV=`expr $$SIZE % $(PART_ALIGN)` ; \
	if [ $$DIV -gt 0 ] ; then \
		PAD=`expr $(PART_ALIGN) - $$DIV` ; \
		dd if=/dev/zero count=1 bs=$$PAD conv=sync 2> /dev/null | tr \\000 \\377 >> $(KERNELZ).$(COMP) ; \
	fi
	# Padded Kernel Image + Compressed Rootfs Size
	@cat $(RAMDISK) >> $(KERNELZ).$(COMP)
	@wc -c $(KERNELZ).$(COMP)
	#===========================================
	# Pack final image and write headers
	# For No padded, need write kernel size in image header 
	# to correct mount partition in mtd drivers address
	@cd $(IMAGEDIR) ; \
	ENTRY=`LANG=C LC_ALL=C $(ENTRY_CMD)` ; \
	ISIZE=`wc -c $(KERNELZ).$(COMP) | awk '{print $$1}'` ; \
	RSIZE=`wc -c $(RAMDISK) | awk '{print $$1}'` ; \
	KRN_SIZE=`expr $$ISIZE - $$RSIZE + 64` ; \
	$(MKIMAGE_DIR)/mkimage -A mips -O linux -T kernel -C $(COMP) -a $(LOAD_ADDR) -e $$ENTRY -k $$KRN_SIZE \
	-n $(CONFIG_FIRMWARE_PRODUCT_ID) -V $(FIRMWARE_KERNEL_VER) $(FIRMWARE_ROOTFS_VER) -d $(KERNELZ).$(COMP) $(IMAGE)
