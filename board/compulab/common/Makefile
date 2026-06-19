# SPDX-License-Identifier: GPL-2.0+
#
# (C) Copyright 2011 - 2013 CompuLab, Ltd. <www.compulab.co.il>
#
# Author: Igor Grinberg <grinberg@compulab.co.il>

CL_EEPROM=y

ifdef CONFIG_TARGET_TRIMSLICE
CL_EEPROM=
endif

obj-y				+= common.o
obj-$(CL_EEPROM)		+= eeprom.o
# Runtime DTB fixup (SoM/baseboard SN, HW revision, options, per-iface MACs).
# Only iot-gate-imx8plus consumes it (via ft_board_setup); keep it off the other
# compulab boards so they are not disturbed.
obj-$(CONFIG_TARGET_IOT_GATE_IMX8PLUS)	+= fdt.o
obj-$(CONFIG_SMC911X)		+= omap3_smc911x.o
