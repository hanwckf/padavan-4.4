SRC_NAME = ZeroTierOne-1.8.4
THISDIR = $(shell pwd)
FLAGS := DEFS=""
CFLAGS := -Os -ffunction-sections -fdata-sections
LDFLAGS := -Wl,--gc-sections,--as-needed

all: extract_test
	$(MAKE) -j$(HOST_NCPU) -C $(SRC_NAME)

extract_test:
	 if [ ! -d $(SRC_NAME) ] ; then \
		tar xf $(SRC_NAME).tar.gz ; \
		patch -d $(SRC_NAME) -p1 -i ../0001-fix-makefile.patch ; \
		patch -d $(SRC_NAME) -p1 -i ../0006-gcc10.patch ; \
		patch -d $(SRC_NAME) -p1 -i ../0007-add-cerrno-header-for-str-errno.patch ; \
		patch -d $(SRC_NAME) -p1 -i ../0008-fix-compilation-for-arm_cortex-a7-neon.patch; \
	 fi ;

config_test:
clean:
	if [ -f $(SRC_NAME)/Makefile ] ; then \
		$(MAKE) -C $(SRC_NAME) distclean ; \
	fi ; \

romfs:
	$(ROMFSINST) $(THISDIR)/$(SRC_NAME)/zerotier-one /usr/bin/zerotier-one
	$(ROMFSINST) $(THISDIR)/zerotier.sh /usr/bin/zerotier.sh
	ln -sf zerotier-one $(ROMFSDIR)/usr/bin/zerotier-cli
	ln -sf zerotier-one $(ROMFSDIR)/usr/bin/zerotier-idtool
	cp -fP $(CONFIG_CROSS_COMPILER_ROOT)/mipsel-linux-uclibc/sysroot/lib/libstdc++.so* $(ROMFSDIR)/lib

