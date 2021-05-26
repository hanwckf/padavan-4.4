EXEC = switch

SRC = switch_fun.c switch_753x.c switch_ioctl.c

all: $(EXEC)

switch: $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRC)

romfs:
	$(ROMFSINST) /bin/switch

clean:
	-rm -f $(EXEC) *.o
