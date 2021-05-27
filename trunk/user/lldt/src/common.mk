# Before including this in a build directory
# define OS_LAYER and CC, CFLAGS, etc.

.PHONY: clean all depend

# Files for the Daemon and the Test-Tool
DCFILES = $(OS_LAYER) main.c event.c util.c packetio.c band.c \
	state.c sessionmgr.c enumeration.c mapping.c seeslist.c \
	tlv.c qospktio.c

TCFILES = $(OS_LAYER) ctmain.c event.c util.c ctpacketio.c ctstate.c

DOBJFILES = $(patsubst %c,%o,$(DCFILES))
TOBJFILES = $(patsubst %c,%o,$(TCFILES))

# Note we do not define all here; that happens where included
# depending in which targets to build

clean:
	rm -f -- .depend *~ lld2d lld2test $(DOBJFILES) $(TOBJFILES)

lld2d: $(DOBJFILES)
	$(CC) $(LDFLAGS) -o $@ $(DOBJFILES)

lld2test: $(TOBJFILES)
	$(CC) $(LDFLAGS) -o $@ $(TOBJFILES)

romfs:
	$(ROMFSINST) /bin/lld2d
	$(ROMFSINST) /etc_ro/lld2d.conf
	$(ROMFSINST) /etc_ro/icon.ico
	$(ROMFSINST) -s icon.ico /etc_ro/icon.large.ico

# End
