ccflags-y += -I$(srctree)/board/compulab/
ccflags-y += -I$(srctree)/board/compulab/plat/imx8mp/
ccflags-y += -I$(srctree)/board/freescale/

obj-y += board.o
obj-y += efi_capsule.o
# CompuLab board command (ldo4 / ddr / pbb): U-Boot proper only.
ifndef CONFIG_SPL_BUILD
obj-y += cmd.o
endif
