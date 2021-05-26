SRC_NAME=pptpd-1.4.0
THISDIR=$(shell pwd)

CFLAGS  += -O2
CFLAGS  += -ffunction-sections -fdata-sections
LDFLAGS += -Wl,--gc-sections

all: config_test
	$(MAKE) -C $(SRC_NAME)

config_test:
	( if [ -f ./config_done ]; then \
		echo "the same configuration"; \
	else \
		make configure && touch config_done; \
	fi )

configure:
	( cd $(SRC_NAME) ; \
	./reconf ; \
	export ac_cv_header_libintl_h=no ; \
	./configure \
		--prefix=/usr \
		--enable-bcrelay \
		--host=$(HOST_TARGET) \
		--build=$(HOST_BUILD) ; \
	)

clean:
	if [ -f $(SRC_NAME)/Makefile ] ; then \
		$(MAKE) -C $(SRC_NAME) distclean ; \
	fi ; \
	rm -f config_done

romfs:
	$(ROMFSINST) -p +x $(THISDIR)/$(SRC_NAME)/pptpd /usr/sbin/pptpd
	$(ROMFSINST) -p +x $(THISDIR)/$(SRC_NAME)/pptpctrl /usr/sbin/pptpctrl
	$(ROMFSINST) -p +x $(THISDIR)/$(SRC_NAME)/bcrelay /usr/sbin/bcrelay
