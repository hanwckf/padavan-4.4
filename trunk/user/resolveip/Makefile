THISDIR = $(shell pwd)
EXEC = resolveip
OBJS = resolveip.o 

# If you want to debug daemon, add following line
CFLAGS += -I./ -DDBG -DSYSLOG -ffunction-sections -fdata-sections -fvisibility=hidden
LDFLAGS += -Wl,--gc-sections
#-DSYSLOG
$(EXEC): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

romfs:
	$(ROMFSINST) /sbin/$(EXEC)

clean:
	rm -f core *.o resolveip
