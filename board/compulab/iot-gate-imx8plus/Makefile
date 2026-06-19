# SPDX-License-Identifier: GPL-2.0+
# Copyright 2018 CompuLab
# Andrea Greco <a.greco@4sigma.it>

ccflags-y += -I$(srctree)/board/compulab/plat/imx8mp/
ccflags-y += -I$(srctree)/board/compulab/plat/imx8/

# Board glue is U-Boot-proper only (board_late_mmc_env_init helper). The real
# board/SPL/DDR support comes from the shared CompuLab platform code below.
ifndef CONFIG_SPL_BUILD
obj-y += iot-gate-imx8plus.o
endif

obj-y += ../plat/imx8mp/
obj-y += ../plat/imx8/
