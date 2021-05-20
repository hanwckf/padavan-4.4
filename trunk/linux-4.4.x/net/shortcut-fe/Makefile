#
# Makefile for Shortcut FE.
#

ccflags-y += -DSFE_SUPPORT_IPV6

obj-m += shortcut-fe.o
obj-m += shortcut-fe-ipv6.o
obj-m += shortcut-fe-cm.o
obj-m += fast-classifier.o

shortcut-fe-objs := \
	sfe_ipv4.o

shortcut-fe-ipv6-objs := \
	sfe_ipv6.o

shortcut-fe-cm-objs := \
	sfe_cm.o
