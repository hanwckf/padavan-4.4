# use host target
CC        = gcc
GCC       = gcc
CXX       = g++
AS        = as
AR        = ar
LD        = ld
NM        = nm
OBJCOPY   = objcopy
RANLIB    = ranlib
STRIP     = strip
STRIPTOOL = $(STRIP)
SSTRIP_NAME = ELFkickers-3.1

CFLAGS    = -Wall -O2
LDFLAGS   =

THISDIR = $(shell pwd)
#N_PATH := $(THISDIR)/go/go/bin:$(PATH)
#export PATH=$(N_PATH)
include $(ROOTDIR)/coustom
-include $(LINUX_CONFIG)

all:go_build
	make -C mkimage
	mkdir -p sstrip
	make -C $(SSTRIP_NAME)
	make -C lzma
	make -C mksquashfs_xz

go_build:
ifeq ($(GITHUB_ACTION),n)
	mkdir -p $(THISDIR)/go
	( if [ ! -f $(THISDIR)/go/go1.17.6.linux-amd64.tar.gz ]; then \
	curl --create-dirs -L https://gomirrors.org/dl/go/go1.17.6.linux-amd64.tar.gz -o $(THISDIR)/go/go1.17.6.linux-amd64.tar.gz ; \
	fi )
	( if [ ! -d $(THISDIR)/go/go ]; then \
	tar zxfv $(THISDIR)/go/go1.17.6.linux-amd64.tar.gz -C go ; \
	fi )
endif
clean:
	if [ -f $(SSTRIP_NAME)/Makefile ] ; then \
		make -C $(SSTRIP_NAME) clean ; \
	fi ;
	rm -rf sstrip
	make -C mkimage clean
	make -C lzma clean
	make -C mksquashfs_xz clean
