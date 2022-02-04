include $(ROOTDIR)/coustom
THISDIR = $(shell pwd)
v2ray_dir="v2ray.com/core/main"
#export GO111MODULE=on
#export GOPROXY=https://goproxy.baidu.com
V2ray_VERSION := 4.44.0
V2ray_URL := https://codeload.github.com/v2fly/v2ray-core/tar.gz/v$(V2ray_VERSION)
V2RAY_SED_ARGS:=
V2RAY_SED_ARGS += \
	s/_ "github.com\/v2fly\/v2ray-core\/v4\/main\/json"/\/\/ &/; \
	/\/\/ _ "github.com\/v2fly\/v2ray-core\/v4\/main\/jsonem"/s/\/\/ //;
V2RAY_SED_ARGS += \
	s/_ "github.com\/v2fly\/v2ray-core\/v4\/main\/json"/\/\/ &/;
ifeq ($(GITHUB_ACTION),n)
all:download_v2 build_extract build_v2ray

download_v2:
	( if [ ! -f $(THISDIR)/v2ray-core-$(V2ray_VERSION).tar.gz ]; then \
	curl --create-dirs -L $(V2ray_URL) -o $(THISDIR)/v2ray-core-$(V2ray_VERSION).tar.gz ; \
	fi )

build_extract:
	mkdir -p $(THISDIR)/v2ray.com
	mkdir -p $(THISDIR)/bin
	( if [ ! -d $(THISDIR)/v2ray.com/core ]; then \
	tar zxfv $(THISDIR)/v2ray-core-$(V2ray_VERSION).tar.gz -C $(THISDIR)/v2ray.com ; \
	mv $(THISDIR)/v2ray.com/v2ray-core-$(V2ray_VERSION) $(THISDIR)/v2ray.com/core ; \
	sed -i \
			'$(V2RAY_SED_ARGS)' \
			$(THISDIR)/$(v2ray_dir)/distro/all/all.go ; \
	fi )

build_v2ray:
	( cd $(THISDIR)/$(v2ray_dir); \
	if [ $(GOPROXY_ON) = "y" ]; then \
	go env -w GOPROXY=https://goproxy.cn,direct ; \
	fi ; \
	GOOS=linux GOARCH=mipsle go build -ldflags "-w -s" -o $(THISDIR)/bin/v2ray; \
	)
else
all:
endif

clean:
	rm -rf $(THISDIR)/v2ray.com
	rm -rf $(THISDIR)/bin

romfs:
ifeq ($(GITHUB_ACTION),n)	
	$(ROMFSINST) -p +x $(THISDIR)/bin/v2ray /usr/bin/v2ray
else
	$(ROMFSINST) -p +x $(THISDIR)/v2ray /usr/bin/v2ray
endif