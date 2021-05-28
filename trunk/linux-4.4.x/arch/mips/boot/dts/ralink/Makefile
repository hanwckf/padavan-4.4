builtindtb-y		:= mt7621-raeth-nor

ifneq ($(CONFIG_RALINK_BUILTIN_DTB_NAME),"")
	builtindtb-y	:= $(patsubst "%",%,$(CONFIG_RALINK_BUILTIN_DTB_NAME))
endif

dtb-y := $(builtindtb-y).dtb
obj-y += $(builtindtb-y).dtb.o

# Force kbuild to make empty built-in.o if necessary
obj-				+= dummy.o

always				:= $(dtb-y)
clean-files			:= *.dtb *.dtb.S
