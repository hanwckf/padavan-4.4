THISDIR = $(shell pwd)
all:

clean:

romfs:
	$(ROMFSINST)  $(THISDIR)/sqm.conf /etc_ro/sqm.conf
	chmod -R +x scripts/
	mkdir -p $(INSTALLDIR)/usr/lib/sqm
	$(ROMFSINST) $(THISDIR)/scripts/ /usr/lib/sqm