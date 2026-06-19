ccflags-y += -I$(srctree)/board/compulab/
ccflags-y += -I$(srctree)/board/compulab/plat/imx8mp/
ccflags-y += -I$(srctree)/board/freescale/

obj-y += board.o
obj-y += efi_capsule.o
# INTEGRATION TODO: cmd.c is the CompuLab board command (DDR/board-config dump).
# It depends on fork-local helpers not yet ported (lppdr4_get_ram_name,
# SUPPORTED_CONF, ...). Re-enable once the ddr/eeprom headers it needs are
# wired up. Not required to boot.
#ifndef CONFIG_SPL_BUILD
#obj-y += cmd.o
#endif
