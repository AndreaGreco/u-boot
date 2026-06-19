/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2011 CompuLab, Ltd. <www.compulab.co.il>
 *
 * Authors: Nikita Kiryanov <nikita@compulab.co.il>
 *	    Igor Grinberg <grinberg@compulab.co.il>
 */

#ifndef _EEPROM_
#define _EEPROM_
#include <errno.h>
#include <asm/setup.h>

#if !CONFIG_IS_ENABLED(TARGET_TRIMSLICE)
int cl_eeprom_read_mac_addr(uchar *buf, uint eeprom_bus);
int cl_eeprom_read_n_mac_addr(uchar *buf, uint iface_number, uint eeprom_bus);
u32 cl_eeprom_get_board_rev(uint eeprom_bus);
int cl_eeprom_get_product_name(uchar *buf, uint eeprom_bus);

/* CompuLab SoM + carrier (SB) EEPROM, for the runtime DTB fixup (fdt_set_sn). */
u32 cl_eeprom_get_som_revision(void);
u32 cl_eeprom_get_sb_revision(void);
int cl_eeprom_read_som_name(char *buf);
int cl_eeprom_read_sb_name(char *buf);
int cl_eeprom_read_som_options(char *buf);
int cl_eeprom_read_sb_options(char *buf);
void cpl_get_som_serial(struct tag_serialnr *serialnr);
void cpl_get_sb_serial(struct tag_serialnr *serialnr);
#else
static inline int cl_eeprom_read_mac_addr(uchar *buf, uint eeprom_bus)
{
	return 1;
}
static inline int cl_eeprom_read_n_mac_addr(uchar *buf, uint iface_number,
					    uint eeprom_bus)
{
	return 1;
}
static inline u32 cl_eeprom_get_board_rev(uint eeprom_bus)
{
	return 0;
}
static inline int cl_eeprom_get_product_name(uchar *buf, uint eeprom_bus)
{
	return -ENOSYS;
}
#endif

#endif
