EXEC = hw_nat 

HWNAT_OBJS = hw_nat.o hwnat_api.o util.o

CONF_H := $(ROOTDIR)/$(CONFIG_LINUXDIR)/include/generated/autoconf.h

CFLAGS += -I$(ROOTDIR)/$(CONFIG_LINUXDIR)/include
CFLAGS += -I$(ROOTDIR)/$(CONFIG_LINUXDIR)/net/nat/hw_nat/
CFLAGS += -I$(ROOTDIR)/$(CONFIG_LINUXDIR)/arch/mips/include
CFLAGS += -Wall

all: $(EXEC)

hw_nat: $(HWNAT_OBJS) $(CONF_H)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(HWNAT_OBJS) 

romfs:
	$(ROMFSINST) /bin/hw_nat

clean:
	rm -f $(EXEC) *.o
