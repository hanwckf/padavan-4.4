##################################################################
# Define Ralink/Mediatek boards
##################################################################

BOARD_NUM_USB_PORTS=0

include $(ROOTDIR)/user/shared/board.mk

##################################################################

ifneq ($(CONFIG_BOARD_RAM_SIZE),)
CFLAGS += -DBOARD_RAM_SIZE=$(CONFIG_BOARD_RAM_SIZE)
else
CFLAGS += -DBOARD_RAM_SIZE=128
endif

BOARD_2G_IN_SOC=0
BOARD_5G_IN_SOC=0
BOARD_HAS_2G_RADIO=0
BOARD_HAS_5G_RADIO=0

ifndef CONFIG_USB_SUPPORT
BOARD_NUM_USB_PORTS=0
endif

ifndef CONFIG_FIRST_IF_NONE
BOARD_HAS_2G_RADIO=1
BOARD_HAS_5G_RADIO=1
endif

CFLAGS += -DBOARD_2G_IN_SOC=$(BOARD_2G_IN_SOC)
CFLAGS += -DBOARD_5G_IN_SOC=$(BOARD_5G_IN_SOC)
CFLAGS += -DBOARD_HAS_2G_RADIO=$(BOARD_HAS_2G_RADIO)
CFLAGS += -DBOARD_HAS_5G_RADIO=$(BOARD_HAS_5G_RADIO)
CFLAGS += -DBOARD_NUM_USB_PORTS=$(BOARD_NUM_USB_PORTS)

export BOARD_2G_IN_SOC
export BOARD_5G_IN_SOC
export BOARD_HAS_2G_RADIO
export BOARD_HAS_5G_RADIO
export BOARD_NUM_USB_PORTS
