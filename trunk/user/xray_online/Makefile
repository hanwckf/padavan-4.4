THISDIR = $(shell pwd)
#export GO111MODULE=on
#export GOPROXY=https://goproxy.bj.bcebos.com/
Xray_VERSION := 1.5.3
Xray_URL := https://codeload.github.com/XTLS/Xray-core/tar.gz/v$(Xray_VERSION)
Xray_dir = xray-core/Xray-core-$(Xray_VERSION)/main

all:download_v2 build_extract build_Xray

download_v2:
	( if [ ! -f $(THISDIR)/Xray-core-$(Xray_VERSION).tar.gz ]; then \
	curl --create-dirs -L $(Xray_URL) -o $(THISDIR)/Xray-core-$(Xray_VERSION).tar.gz ; \
	fi )

build_extract:
	mkdir -p $(THISDIR)/xray-core
	mkdir -p $(THISDIR)/bin
	( if [ ! -d $(THISDIR)/xray-core/Xray-core-$(Xray_VERSION) ]; then \
	rm -rf $(THISDIR)/xray-core/* ; \
	tar zxfv $(THISDIR)/Xray-core-$(Xray_VERSION).tar.gz -C $(THISDIR)/xray-core ; \
	fi )

build_Xray:
	( cd $(THISDIR)/$(Xray_dir); \
	go env -w GOPROXY=https://goproxy.cn,direct ; \
	GOOS=linux GOARCH=mipsle go build -ldflags "-w -s" -o $(THISDIR)/bin/xray; \
	)

clean:
	rm -rf $(THISDIR)/xray-core
	rm -rf $(THISDIR)/bin

romfs:
	$(ROMFSINST) -p +x $(THISDIR)/bin/xray /usr/bin/xray