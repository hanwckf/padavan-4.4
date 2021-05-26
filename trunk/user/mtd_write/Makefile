EXEC = mtd_write

OBJS = mtd.o crc32.o md5.o jffs2.o

CFLAGS += -D__BYTE_ORDER=__LITTLE_ENDIAN
CFLAGS += -ffunction-sections -fdata-sections

LDFLAGS += -Wl,--gc-sections

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

romfs:
	$(ROMFSINST) /bin/$(EXEC)

clean:
	rm -f $(EXEC) *.o
