subdir-ccflags-y += -Werror

obj-$(CONFIG_COMMON_CLK_MEDIATEK) += clk-mtk.o clk-pll.o clk-gate.o clk-apmixed.o clk-cpumux.o
obj-$(CONFIG_RESET_CONTROLLER) += reset.o
obj-$(CONFIG_COMMON_CLK_MT2701) += clk-mt2701.o
obj-$(CONFIG_COMMON_CLK_MT2712) += clk-mt2712.o clkchk-mt2712.o
obj-$(CONFIG_COMMON_CLK_MT2712_BDPSYS) += clk-mt2712-bdp.o
obj-$(CONFIG_COMMON_CLK_MT2712_IMGSYS) += clk-mt2712-img.o
obj-$(CONFIG_COMMON_CLK_MT2712_JPGDECSYS) += clk-mt2712-jpgdec.o
obj-$(CONFIG_COMMON_CLK_MT2712_MFGCFG) += clk-mt2712-mfg.o
obj-$(CONFIG_COMMON_CLK_MT2712_MMSYS) += clk-mt2712-mm.o
obj-$(CONFIG_COMMON_CLK_MT2712_VDECSYS) += clk-mt2712-vdec.o
obj-$(CONFIG_COMMON_CLK_MT2712_VENCSYS) += clk-mt2712-venc.o
obj-$(CONFIG_COMMON_CLK_MT7622) += clk-mt7622.o clkchk-mt7622.o
obj-$(CONFIG_COMMON_CLK_LEOPARD) += clk-leopard.o clkchk-leopard.o \
				 clk-leopard-hif.o clk-leopard-eth.o
obj-$(CONFIG_COMMON_CLK_MT8135) += clk-mt8135.o
obj-$(CONFIG_COMMON_CLK_MT8173) += clk-mt8173.o
obj-y += clk-bringup.o

ifeq ($(CONFIG_DEBUG_FS), y)
obj-$(CONFIG_COMMON_CLK_MEDIATEK) += clkdbg.o
obj-$(CONFIG_COMMON_CLK_MT2712) += clkdbg-mt2712.o
obj-$(CONFIG_COMMON_CLK_MT7622) += clkdbg-mt7622.o
obj-$(CONFIG_COMMON_CLK_LEOPARD) += clkdbg-leopard.o
endif