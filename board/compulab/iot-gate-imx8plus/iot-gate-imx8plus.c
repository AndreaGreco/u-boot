// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright 2024 CompuLab / 4Sigma
 *
 * Thin board glue for the CompuLab IOT-GATE-IMX8PLUS.
 *
 * The bulk of the board support now lives in the shared CompuLab platform
 * code under board/compulab/plat/imx8mp/ (board_init, board_late_init,
 * board_phy_config, MAC-from-EEPROM, DDR, ...). This file only carries what
 * that layer expects the board to provide and which is not part of the
 * CompuLab vendor common lib (board/nxp/common/mmc.c, where this normally
 * lives, is not linked for a compulab-vendor board).
 */

#include <command.h>
#include <env.h>
#include <mmc.h>
#include <vsprintf.h>

static int check_mmc_autodetect(void)
{
	char *autodetect_str = env_get("mmcautodetect");

	if (autodetect_str && !strcmp(autodetect_str, "yes"))
		return 1;

	return 0;
}

__weak int mmc_map_to_kernel_blk(int dev_no)
{
	return dev_no;
}

void board_late_mmc_env_init(void)
{
	char cmd[32];
	char mmcblk[32];
	u32 dev_no = mmc_get_env_dev();

	if (!check_mmc_autodetect())
		return;

	env_set_ulong("mmcdev", dev_no);

	/* Set mmcblk env */
	sprintf(mmcblk, "/dev/mmcblk%dp2 rootwait rw", mmc_map_to_kernel_blk(dev_no));
	env_set("mmcroot", mmcblk);

	sprintf(cmd, "mmc dev %d", dev_no);
	run_command(cmd, 0);
}
