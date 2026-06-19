/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2019 NXP
 * Copyright 2024 CompuLab / 4Sigma
 *
 * Mainline config for the CompuLab IOT-GATE-IMX8PLUS.
 */

#ifndef __IOT_GATE_IMX8PLUS_H
#define __IOT_GATE_IMX8PLUS_H

#include <linux/sizes.h>
#include <linux/stringify.h>
#include <asm/arch/imx-regs.h>

#define CFG_SYS_UBOOT_BASE	(QSPI0_AMBA_BASE + CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR * 512)

/* USDHC2 (SD) + USDHC3 (eMMC) brought up by the SPL board_mmc_init() loop. */
#define CFG_SYS_FSL_USDHC_NUM	2

#if defined(CONFIG_CMD_NET)
#define CFG_FEC_MXC_PHYADDR          1
#endif

/* Link Definitions */
#define CFG_SYS_INIT_RAM_ADDR	0x40000000
#define CFG_SYS_INIT_RAM_SIZE	0x80000

/*
 * DRAM layout. INTEGRATION TODO: the real board ships several LPDDR4 densities;
 * size is detected at runtime by the SoC ROM/board code. These are sane EVK
 * defaults so the scaffold links.
 */
#define CFG_SYS_SDRAM_BASE		0x40000000
#define PHYS_SDRAM			0x40000000
#define PHYS_SDRAM_SIZE			0xC0000000	/* 3 GB */
#define PHYS_SDRAM_2			0x100000000
#define PHYS_SDRAM_2_SIZE		0xC0000000	/* 3 GB */

#endif
