THISDIR = $(shell pwd)
all:

clean:

romfs:
	$(ROMFSINST) -p +x $(THISDIR)/adguardhome.sh /usr/bin/adguardhome.sh
	$(ROMFSINST) -p +x $(THISDIR)/AdGuardHome /usr/bin/AdGuardHome
	$(ROMFSINST) -p +x $(THISDIR)/cacert.pem /usr/bin/cacert.pem
