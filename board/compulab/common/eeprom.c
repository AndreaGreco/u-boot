// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2011 CompuLab, Ltd. <www.compulab.co.il>
 *
 * Authors: Nikita Kiryanov <nikita@compulab.co.il>
 *	    Igor Grinberg <grinberg@compulab.co.il>
 */

#include <i2c.h>
#include <net.h>
#include <vsprintf.h>
#include <eeprom_layout.h>
#include <eeprom_field.h>
#include <asm/setup.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include "eeprom.h"

#define EEPROM_LAYOUT_VER_OFFSET	44
#define BOARD_SERIAL_OFFSET		20
#define BOARD_SERIAL_OFFSET_LEGACY	8
#define BOARD_REV_OFFSET		0
#define BOARD_REV_OFFSET_LEGACY		6
#define BOARD_REV_SIZE			2
#define PRODUCT_NAME_OFFSET		128
#define PRODUCT_NAME_SIZE		16
#define MAC_ADDR_OFFSET			4
#define MAC1_ADDR_OFFSET		10
#define MAC_ADDR_OFFSET_LEGACY		0

#define LAYOUT_INVALID	0
#define LAYOUT_LEGACY	0xff

static int cl_eeprom_bus;
static int cl_eeprom_layout; /* Implicitly LAYOUT_INVALID */

static int cl_eeprom_read(uint offset, uchar *buf, int len)
{
	struct udevice *eeprom;
	int res;

	res = i2c_get_chip_for_busnum(cl_eeprom_bus, CONFIG_SYS_I2C_EEPROM_ADDR,
				      CONFIG_SYS_I2C_EEPROM_ADDR_LEN, &eeprom);
	if (res)
		return res;

	return dm_i2c_read(eeprom, offset, (uint8_t *)buf, len);
}

static int cl_eeprom_setup(uint eeprom_bus)
{
	int res;

	/*
	 * We know the setup was already done when the layout is set to a valid
	 * value and we're using the same bus as before.
	 */
	if (cl_eeprom_layout != LAYOUT_INVALID && eeprom_bus == cl_eeprom_bus)
		return 0;

	cl_eeprom_bus = eeprom_bus;
	res = cl_eeprom_read(EEPROM_LAYOUT_VER_OFFSET,
			     (uchar *)&cl_eeprom_layout, 1);
	if (res) {
		cl_eeprom_layout = LAYOUT_INVALID;
		return res;
	}

	if (cl_eeprom_layout == 0 || cl_eeprom_layout >= 0x20)
		cl_eeprom_layout = LAYOUT_LEGACY;

	return 0;
}

void get_board_serial(struct tag_serialnr *serialnr)
{
	u32 serial[2];
	uint offset;

	memset(serialnr, 0, sizeof(*serialnr));

	if (cl_eeprom_setup(CONFIG_SYS_I2C_EEPROM_BUS))
		return;

	offset = (cl_eeprom_layout != LAYOUT_LEGACY) ?
		BOARD_SERIAL_OFFSET : BOARD_SERIAL_OFFSET_LEGACY;

	if (cl_eeprom_read(offset, (uchar *)serial, 8))
		return;

	if (serial[0] != 0xffffffff && serial[1] != 0xffffffff) {
		serialnr->low = serial[0];
		serialnr->high = serial[1];
	}
}

/*
 * Routine: cl_eeprom_read_mac_addr
 * Description: read mac address and store it in buf.
 */
int cl_eeprom_read_mac_addr(uchar *buf, uint eeprom_bus)
{
	uint offset;
	int err;

	err = cl_eeprom_setup(eeprom_bus);
	if (err)
		return err;

	offset = (cl_eeprom_layout != LAYOUT_LEGACY) ?
			MAC_ADDR_OFFSET : MAC_ADDR_OFFSET_LEGACY;

	return cl_eeprom_read(offset, buf, 6);
}

/*
 * Routine: cl_eeprom_read_n_mac_addr
 * Description: read the iface_number-th mac address and store it in buf.
 *
 * Ported from the CompuLab vendor common layer: boards such as the
 * iot-gate-imx8plus carry more than one ethernet interface (FEC + EQOS) and
 * store a MAC per interface in the SoM EEPROM. Falls back to a random address
 * when the EEPROM slot is empty.
 */
int cl_eeprom_read_n_mac_addr(uchar *buf, uint iface_number, uint eeprom_bus)
{
	uint offset;
	int err;

	err = cl_eeprom_setup(eeprom_bus);
	if (err)
		return err;

	offset = (iface_number == 0) ? MAC_ADDR_OFFSET : MAC1_ADDR_OFFSET;

	err = cl_eeprom_read(offset, buf, 6);

	/* Generate a random address if the SoM EEPROM slot is empty. */
	if (buf[0] == buf[2])
		net_random_ethaddr(buf);

	return err;
}

static u32 board_rev;

/*
 * Routine: cl_eeprom_get_board_rev
 * Description: read system revision from eeprom
 */
u32 cl_eeprom_get_board_rev(uint eeprom_bus)
{
	char str[5]; /* Legacy representation can contain at most 4 digits */
	uint offset = BOARD_REV_OFFSET_LEGACY;

	if (board_rev)
		return board_rev;

	if (cl_eeprom_setup(eeprom_bus))
		return 0;

	if (cl_eeprom_layout != LAYOUT_LEGACY)
		offset = BOARD_REV_OFFSET;

	if (cl_eeprom_read(offset, (uchar *)&board_rev, BOARD_REV_SIZE))
		return 0;

	/*
	 * Convert legacy syntactic representation to semantic
	 * representation. i.e. for rev 1.00: 0x100 --> 0x64
	 */
	if (cl_eeprom_layout == LAYOUT_LEGACY) {
		sprintf(str, "%x", board_rev);
		board_rev = dectoul(str, NULL);
	}

	return board_rev;
};

/*
 * Routine: cl_eeprom_get_board_rev
 * Description: read system revision from eeprom
 *
 * @buf: buffer to store the product name
 * @eeprom_bus: i2c bus num of the eeprom
 *
 * @return: 0 on success, < 0 on failure
 */
int cl_eeprom_get_product_name(uchar *buf, uint eeprom_bus)
{
	int err;

	if (buf == NULL)
		return -EINVAL;

	err = cl_eeprom_setup(eeprom_bus);
	if (err)
		return err;

	err = cl_eeprom_read(PRODUCT_NAME_OFFSET, buf, PRODUCT_NAME_SIZE);
	if (!err) /* Protect ourselves from invalid data (unterminated str) */
		buf[PRODUCT_NAME_SIZE - 1] = '\0';

	return err;
}

/*
 * --- CompuLab SoM + carrier (SB) EEPROM, dual-device accessors ---
 * Used by the runtime DTB fixup (board/compulab/common/fdt.c: fdt_set_sn) to
 * publish the SoM and baseboard name/serial/revision/options into the kernel
 * device tree. Kept self-contained (own reader and own layout cache) so the
 * single-bus cl_eeprom_* accessors above, shared with imx8mm-cl-iot-gate, are
 * left untouched.
 */
#define PRODUCT_OPTION_OFFSET	144
#define PRODUCT_OPTION_SIZE	16
#define PRODUCT_OPTION_NUM	5

#ifndef CL_SOM_EEPROM_ADDR
#define CL_SOM_EEPROM_ADDR	0x50
#endif
#ifndef CL_SB_EEPROM_BUS
#define CL_SB_EEPROM_BUS	0
#endif
#ifndef CL_SB_EEPROM_ADDR
#define CL_SB_EEPROM_ADDR	0x54
#endif

struct cl_eeprom_dev {
	int	bus;
	u8	chip;
	int	layout;
};

static struct cl_eeprom_dev eeprom_som = {
	CONFIG_SYS_I2C_EEPROM_BUS, CL_SOM_EEPROM_ADDR, LAYOUT_INVALID,
};
static struct cl_eeprom_dev eeprom_sb = {
	CL_SB_EEPROM_BUS, CL_SB_EEPROM_ADDR, LAYOUT_INVALID,
};

static int cl_dev_read(struct cl_eeprom_dev *e, uint offset, uchar *buf, int len)
{
	struct udevice *dev;
	int ret;

	ret = i2c_get_chip_for_busnum(e->bus, e->chip, 1, &dev);
	if (ret)
		return ret;

	return dm_i2c_read(dev, offset, buf, len);
}

static int cl_dev_setup(struct cl_eeprom_dev *e)
{
	if (e->layout != LAYOUT_INVALID)
		return 0;

	if (cl_dev_read(e, EEPROM_LAYOUT_VER_OFFSET, (uchar *)&e->layout, 1)) {
		e->layout = LAYOUT_INVALID;
		return -1;
	}

	if (e->layout == 0 || e->layout >= 0x20)
		e->layout = LAYOUT_LEGACY;

	return 0;
}

static void cl_dev_get_serial(struct cl_eeprom_dev *e, struct tag_serialnr *serialnr)
{
	u32 serial[2];
	uint offset;

	memset(serialnr, 0, sizeof(*serialnr));

	if (cl_dev_setup(e))
		return;

	offset = (e->layout != LAYOUT_LEGACY) ?
		BOARD_SERIAL_OFFSET : BOARD_SERIAL_OFFSET_LEGACY;

	if (cl_dev_read(e, offset, (uchar *)serial, 8))
		return;

	if (serial[0] != 0xffffffff && serial[1] != 0xffffffff) {
		serialnr->low = serial[0];
		serialnr->high = serial[1];
	}
}

void cpl_get_som_serial(struct tag_serialnr *serialnr)
{
	cl_dev_get_serial(&eeprom_som, serialnr);
}

void cpl_get_sb_serial(struct tag_serialnr *serialnr)
{
	cl_dev_get_serial(&eeprom_sb, serialnr);
}

static u32 cl_dev_get_revision(struct cl_eeprom_dev *e)
{
	u32 revision = 0;

	if (cl_dev_setup(e))
		return 0;

	if (cl_dev_read(e, BOARD_REV_OFFSET, (uchar *)&revision, BOARD_REV_SIZE))
		return 0;

	return revision;
}

u32 cl_eeprom_get_som_revision(void)
{
	return cl_dev_get_revision(&eeprom_som);
}

u32 cl_eeprom_get_sb_revision(void)
{
	return cl_dev_get_revision(&eeprom_sb);
}

static int cl_dev_read_options(struct cl_eeprom_dev *e, char *buf)
{
	uchar tmp[PRODUCT_OPTION_SIZE];
	int len = 0;
	int i;

	if (cl_dev_setup(e))
		return snprintf(buf, PRODUCT_NAME_SIZE, "unknown");

	for (i = 0; i < PRODUCT_OPTION_NUM; ++i) {
		if (!cl_dev_read(e, PRODUCT_OPTION_OFFSET + PRODUCT_OPTION_SIZE * i,
				 tmp, PRODUCT_OPTION_SIZE) && tmp[0] != 0xff)
			len += snprintf(buf + len, PRODUCT_OPTION_SIZE, "%s", (char *)tmp);
	}

	return len;
}

int cl_eeprom_read_som_options(char *buf)
{
	return cl_dev_read_options(&eeprom_som, buf);
}

int cl_eeprom_read_sb_options(char *buf)
{
	return cl_dev_read_options(&eeprom_sb, buf);
}

static int cl_dev_read_name(struct cl_eeprom_dev *e, char *buf)
{
	uchar tmp[PRODUCT_NAME_SIZE];

	if (cl_dev_setup(e))
		return snprintf(buf, PRODUCT_NAME_SIZE, "unknown");

	if (!cl_dev_read(e, PRODUCT_NAME_OFFSET, tmp, PRODUCT_NAME_SIZE) &&
	    tmp[0] != 0xff) {
		tmp[PRODUCT_NAME_SIZE - 1] = '\0';
		return snprintf(buf, PRODUCT_NAME_SIZE, "%s", (char *)tmp);
	}

	return snprintf(buf, PRODUCT_NAME_SIZE, "unknown");
}

int cl_eeprom_read_som_name(char *buf)
{
	return cl_dev_read_name(&eeprom_som, buf);
}

int cl_eeprom_read_sb_name(char *buf)
{
	return cl_dev_read_name(&eeprom_sb, buf);
}

#ifdef CONFIG_CMD_EEPROM_LAYOUT
/**
 * eeprom_field_print_bin_ver() - print a "version field" which contains binary
 *				  data
 *
 * Treat the field data as simple binary data, and print it formatted as a
 * version number (2 digits after decimal point).
 * The field size must be exactly 2 bytes.
 *
 * Sample output:
 *      Field Name      123.45
 *
 * @field:	an initialized field to print
 */
void eeprom_field_print_bin_ver(const struct eeprom_field *field)
{
	if ((field->buf[0] == 0xff) && (field->buf[1] == 0xff)) {
		field->buf[0] = 0;
		field->buf[1] = 0;
	}

	printf(PRINT_FIELD_SEGMENT, field->name);
	int major = (field->buf[1] << 8 | field->buf[0]) / 100;
	int minor = (field->buf[1] << 8 | field->buf[0]) - major * 100;
	printf("%d.%02d\n", major, minor);
}

/**
 * eeprom_field_update_bin_ver() - update a "version field" which contains
 *				   binary data
 *
 * This function takes a version string in the form of x.y (x and y are both
 * decimal values, y is limited to two digits), translates it to the binary
 * form, then writes it to the field. The field size must be exactly 2 bytes.
 *
 * This function strictly enforces the data syntax, and will not update the
 * field if there's any deviation from it. It also protects from overflow.
 *
 * @field:	an initialized field
 * @value:	a version string
 *
 * Returns 0 on success, -1 on failure.
 */
int eeprom_field_update_bin_ver(struct eeprom_field *field, char *value)
{
	char *endptr;
	char *tok = strtok(value, ".");
	if (tok == NULL)
		return -1;

	int num = simple_strtol(tok, &endptr, 0);
	if (*endptr != '\0')
		return -1;

	tok = strtok(NULL, "");
	if (tok == NULL)
		return -1;

	int remainder = simple_strtol(tok, &endptr, 0);
	if (*endptr != '\0')
		return -1;

	num = num * 100 + remainder;
	if (num >> 16)
		return -1;

	field->buf[0] = (unsigned char)num;
	field->buf[1] = num >> 8;

	return 0;
}

char *months[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

/**
 * eeprom_field_print_date() - print a field which contains date data
 *
 * Treat the field data as simple binary data, and print it formatted as a date.
 * Sample output:
 *      Field Name      07/Feb/2014
 *      Field Name      56/BAD/9999
 *
 * @field:	an initialized field to print
 */
void eeprom_field_print_date(const struct eeprom_field *field)
{
	printf(PRINT_FIELD_SEGMENT, field->name);
	printf("%02d/", field->buf[0]);
	if (field->buf[1] >= 1 && field->buf[1] <= 12)
		printf("%s", months[field->buf[1] - 1]);
	else
		printf("BAD");

	printf("/%d\n", field->buf[3] << 8 | field->buf[2]);
}

static int validate_date(unsigned char day, unsigned char month,
			unsigned int year)
{
	int days_in_february;

	switch (month) {
	case 0:
	case 2:
	case 4:
	case 6:
	case 7:
	case 9:
	case 11:
		if (day > 31)
			return -1;
		break;
	case 3:
	case 5:
	case 8:
	case 10:
		if (day > 30)
			return -1;
		break;
	case 1:
		days_in_february = 28;
		if (year % 4 == 0) {
			if (year % 100 != 0)
				days_in_february = 29;
			else if (year % 400 == 0)
				days_in_february = 29;
		}

		if (day > days_in_february)
			return -1;

		break;
	default:
		return -1;
	}

	return 0;
}

/**
 * eeprom_field_update_date() - update a date field which contains binary data
 *
 * This function takes a date string in the form of x/Mon/y (x and y are both
 * decimal values), translates it to the binary representation, then writes it
 * to the field.
 *
 * This function strictly enforces the data syntax, and will not update the
 * field if there's any deviation from it. It also protects from overflow in the
 * year value, and checks the validity of the date.
 *
 * @field:	an initialized field
 * @value:	a date string
 *
 * Returns 0 on success, -1 on failure.
 */
int eeprom_field_update_date(struct eeprom_field *field, char *value)
{
	char *endptr;
	char *tok1 = strtok(value, "/");
	char *tok2 = strtok(NULL, "/");
	char *tok3 = strtok(NULL, "/");

	if (tok1 == NULL || tok2 == NULL || tok3 == NULL) {
		printf("%s: syntax error\n", field->name);
		return -1;
	}

	unsigned char day = (unsigned char)simple_strtol(tok1, &endptr, 0);
	if (*endptr != '\0' || day == 0) {
		printf("%s: invalid day\n", field->name);
		return -1;
	}

	unsigned char month;
	for (month = 1; month <= 12; month++)
		if (!strcmp(tok2, months[month - 1]))
			break;

	unsigned int year = simple_strtol(tok3, &endptr, 0);
	if (*endptr != '\0') {
		printf("%s: invalid year\n", field->name);
		return -1;
	}

	if (validate_date(day, month - 1, year)) {
		printf("%s: invalid date\n", field->name);
		return -1;
	}

	if (year >> 16) {
		printf("%s: year overflow\n", field->name);
		return -1;
	}

	field->buf[0] = day;
	field->buf[1] = month;
	field->buf[2] = (unsigned char)year;
	field->buf[3] = (unsigned char)(year >> 8);

	return 0;
}

#define	LAYOUT_VERSION_LEGACY 1
#define	LAYOUT_VERSION_VER1 2
#define	LAYOUT_VERSION_VER2 3
#define	LAYOUT_VERSION_VER3 4

#define DEFINE_PRINT_UPDATE(x) eeprom_field_print_##x, eeprom_field_update_##x

struct eeprom_field layout_v2[15] = {
	{ "Major Revision",            2, NULL, DEFINE_PRINT_UPDATE(bin_ver) },
	{ "Minor Revision",            2, NULL, DEFINE_PRINT_UPDATE(bin_ver) },
	{ "1st MAC Address",           6, NULL, DEFINE_PRINT_UPDATE(mac) },
	{ "2nd MAC Address",           6, NULL, DEFINE_PRINT_UPDATE(mac) },
	{ "Production Date",           4, NULL, DEFINE_PRINT_UPDATE(date) },
	{ "Serial Number",            12, NULL, DEFINE_PRINT_UPDATE(bin_rev) },
	{ "3rd MAC Address (WIFI)",    6, NULL, DEFINE_PRINT_UPDATE(mac) },
	{ "4th MAC Address (Bluetooth)", 6, NULL, DEFINE_PRINT_UPDATE(mac) },
	{ "Layout Version",            1, NULL, DEFINE_PRINT_UPDATE(bin) },
	{ RESERVED_FIELDS,            83, NULL, DEFINE_PRINT_UPDATE(reserved) },
	{ "Product Name",             16, NULL, DEFINE_PRINT_UPDATE(ascii) },
	{ "Product Options #1",       16, NULL, DEFINE_PRINT_UPDATE(ascii) },
	{ "Product Options #2",       16, NULL, DEFINE_PRINT_UPDATE(ascii) },
	{ "Product Options #3",       16, NULL, DEFINE_PRINT_UPDATE(ascii) },
	{ RESERVED_FIELDS,            64, NULL, eeprom_field_print_reserved,
						eeprom_field_update_ascii },
};

struct eeprom_field layout_v3[16] = {
	{ "Major Revision",            2, NULL, DEFINE_PRINT_UPDATE(bin_ver) },
	{ "Minor Revision",            2, NULL, DEFINE_PRINT_UPDATE(bin_ver) },
	{ "1st MAC Address",           6, NULL, DEFINE_PRINT_UPDATE(mac) },
	{ "2nd MAC Address",           6, NULL, DEFINE_PRINT_UPDATE(mac) },
	{ "Production Date",           4, NULL, DEFINE_PRINT_UPDATE(date) },
	{ "Serial Number",            12, NULL, DEFINE_PRINT_UPDATE(bin_rev) },
	{ "3rd MAC Address (WIFI)",    6, NULL, DEFINE_PRINT_UPDATE(mac) },
	{ "4th MAC Address (Bluetooth)", 6, NULL, DEFINE_PRINT_UPDATE(mac) },
	{ "Layout Version",            1, NULL, DEFINE_PRINT_UPDATE(bin) },
	{ "CompuLab EEPROM ID",        3, NULL, DEFINE_PRINT_UPDATE(bin) },
	{ RESERVED_FIELDS,            80, NULL, DEFINE_PRINT_UPDATE(reserved) },
	{ "Product Name",             16, NULL, DEFINE_PRINT_UPDATE(ascii) },
	{ "Product Options #1",       16, NULL, DEFINE_PRINT_UPDATE(ascii) },
	{ "Product Options #2",       16, NULL, DEFINE_PRINT_UPDATE(ascii) },
	{ "Product Options #3",       16, NULL, DEFINE_PRINT_UPDATE(ascii) },
	{ RESERVED_FIELDS,            64, NULL, eeprom_field_print_reserved,
						eeprom_field_update_ascii },
};

void eeprom_layout_assign(struct eeprom_layout *layout, int layout_version)
{
	switch (layout->layout_version) {
	case LAYOUT_VERSION_VER2:
		layout->fields = layout_v2;
		layout->num_of_fields = ARRAY_SIZE(layout_v2);
		break;
	case LAYOUT_VERSION_VER3:
		layout->fields = layout_v3;
		layout->num_of_fields = ARRAY_SIZE(layout_v3);
		break;
	default:
		__eeprom_layout_assign(layout, layout_version);
	}
}

int eeprom_parse_layout_version(char *str)
{
	if (!strcmp(str, "legacy"))
		return LAYOUT_VERSION_LEGACY;
	else if (!strcmp(str, "v1"))
		return LAYOUT_VERSION_VER1;
	else if (!strcmp(str, "v2"))
		return LAYOUT_VERSION_VER2;
	else if (!strcmp(str, "v3"))
		return LAYOUT_VERSION_VER3;
	else
		return LAYOUT_VERSION_UNRECOGNIZED;
}

int eeprom_layout_detect(unsigned char *data)
{
	switch (data[EEPROM_LAYOUT_VER_OFFSET]) {
	case 0xff:
	case 0:
		return LAYOUT_VERSION_VER1;
	case 2:
		return LAYOUT_VERSION_VER2;
	case 3:
		return LAYOUT_VERSION_VER3;
	}

	if (data[EEPROM_LAYOUT_VER_OFFSET] >= 0x20)
		return LAYOUT_VERSION_LEGACY;

	return LAYOUT_VERSION_UNRECOGNIZED;
}
#endif
