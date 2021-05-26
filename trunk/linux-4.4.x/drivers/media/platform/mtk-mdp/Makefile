obj-y += \
	mtk_mdp_core.o \
	mtk_mdp_comp.o \
	mtk_mdp_m2m.o \
	mtk_mdp_regs.o \
	mtk_mdp_vpu.o \
	mtk_mdp_drm.o

ccflags-y += \
    -I$(srctree)/drivers \
    -I$(srctree)/include \
    -I$(srctree)/drivers/media/platform/mtk-vcodec \
    -I$(srctree)/drivers/media/platform/mtk-vpu

ccflags-$(CONFIG_VIDEO_MEDIATEK_VCU) += -I$(srctree)/drivers/media/platform/mtk-vcu
subdir-ccflags-y += -Werror

obj-$(CONFIG_VIDEO_MEDIATEK_MDP_FRVC) += fast-rvc/
