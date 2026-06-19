// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2019 NXP
 */

#include <env.h>
#include <init.h>
#include <miiphy.h>
#include <net.h>
#include <netdev.h>
#include <linux/delay.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <asm/mach-imx/iomux-v3.h>
#include <asm-generic/gpio.h>
#include <asm/arch/imx8mp_pins.h>
#include <asm/arch/clock.h>
#include <asm/arch/sys_proto.h>
#include <asm/mach-imx/gpio.h>
#include <asm/mach-imx/mxc_i2c.h>
#include <spl.h>
#include <led.h>
#include <pwm.h>
#include <asm/mach-imx/dma.h>
#include <power/pmic.h>
#include "common/fdt.h"
#include <imx_sip.h>
#include <linux/arm-smccc.h>
#include "ddr/ddr.h"
#include "common/eeprom.h"
#include "cmd.h"

DECLARE_GLOBAL_DATA_PTR;

int board_phys_sdram_size(phys_size_t *size)
{
	size_t dramsize;
	if (!size)
		return -EINVAL;

	dramsize = lppdr4_get_ramsize();

	*size = ((1L << 20) * dramsize );

	return 0;
}


#ifdef CONFIG_OF_BOARD_SETUP
__weak int fdt_board_vendor_setup(void *blob) {
	return 0;
}

__weak void board_save_phyaddr(int phy_addr) {
	return;
}

__weak int fdt_set_soc_info(void *blob) {
	char tmp[128];
	u32 cpurev = get_cpu_rev();
	u32 max_freq = get_cpu_speed_grade_hz() / 1000000;

	int nodeoff = fdt_add_subnode(blob, 0, "soc.info");
	if(0 > nodeoff)
		return nodeoff;

	fdt_setprop(blob, nodeoff, "imx.type", tmp,
		    sprintf(tmp, "i.MX%s", get_imx_type((cpurev & 0x1FF000) >> 12)));
	fdt_setprop(blob, nodeoff, "imx.rev", tmp,
		    sprintf(tmp, "%d.%d", (cpurev & 0x000F0) >> 4, (cpurev & 0x0000F) >> 0));
	fdt_setprop(blob, nodeoff, "imx.freq", tmp,
		    sprintf(tmp, "%d",  max_freq));

	return 0;
}

extern char version_string[];
static  int fdt_set_som_info(void *blob) {
	char tmp[128];
	char dram_name[16];
	size_t dram_size = lppdr4_get_ramsize();
	unsigned int dram_id = lpddr4_get_mr();

	int nodeoff = fdt_add_subnode(blob, 0, "som.info");
	if(0 > nodeoff)
		return nodeoff;

	fdt_setprop(blob, nodeoff, "uboot.version", tmp,
			sprintf(tmp, "%s",  version_string));

	fdt_setprop(blob, nodeoff, "board.name", tmp,
			sprintf(tmp, "%s",  CONFIG_SYS_BOARD));

	fdt_setprop(blob, nodeoff, "dram.size", tmp,
		    sprintf(tmp, "%ld",  dram_size));

	fdt_setprop(blob, nodeoff, "dram.id", tmp,
		    sprintf(tmp, "0x%x",  dram_id));

	if (lppdr4_get_ram_name(dram_name) == 0) {
		fdt_setprop(blob, nodeoff, "dram.name", tmp,
			sprintf(tmp, "%s",  dram_name));
	}

	return 0;
}

int ft_board_setup(void *blob, struct bd_info *bd)
{

#if defined(CONFIG_ENV_OFFSET) && defined(CONFIG_ENV_SIZE)
	fdt_set_env_addr(blob);
#endif

	fdt_set_sn(blob);
	fdt_set_soc_info(blob);
	fdt_set_som_info(blob);
	fdt_board_vendor_setup(blob);
	return 0;
}
#endif

/*
 * Mainline: the FEC and DWC EQOS clocks and the IOMUXC GPR (RGMII TX/TXC) are
 * configured by the DM clk/eqos/fec drivers from the device tree, so the
 * fork's manual setup_fec()/setup_eqos() are dropped.
 */

#if defined(CONFIG_FEC_MXC) || defined(CONFIG_DWC_ETH_QOS)
static int mx8_rgmii_rework(struct phy_device *phydev);
int board_phy_config(struct phy_device *phydev)
{
	mx8_rgmii_rework(phydev);

	if (phydev->drv->config)
		phydev->drv->config(phydev);
	return 0;
}
#endif

/*
 * Mainline: USB on i.MX8MP is driven by the DM USB stack
 * (USB_DWC3_GENERIC + PHY_IMX8MQ_USB) from the device tree, so the fork's
 * manual dwc3_device setup, PHY init, power gating and
 * board_usb_init()/board_usb_cleanup() are dropped.
 */

#define FSL_SIP_GPC			0xC2000000
#define FSL_SIP_CONFIG_GPC_PM_DOMAIN	0x3
#define DISPMIX				13
#define MIPI				15

__weak void board_vendor_init(void) {
	return;
}

__weak void board_vendor_late_init(void) {
	return;
}

void static board_pm_init(void)
{
	struct arm_smccc_res res;
	/* enable the dispmix & mipi phy power domain */
	arm_smccc_smc(IMX_SIP_GPC, IMX_SIP_GPC_PM_DOMAIN,
		      DISPMIX, true, 0, 0, 0, 0, &res);
	arm_smccc_smc(IMX_SIP_GPC, IMX_SIP_GPC_PM_DOMAIN,
		      MIPI, true, 0, 0, 0, 0, &res);
}

#ifdef CONFIG_MXC_SPI
#define SPI_PAD_CTRL    (PAD_CTL_DSE2 | PAD_CTL_HYS)
static iomux_v3_cfg_t const ecspi2_pads[] = {
        MX8MP_PAD_ECSPI2_SCLK__ECSPI2_SCLK | MUX_PAD_CTRL(SPI_PAD_CTRL),
        MX8MP_PAD_ECSPI2_MOSI__ECSPI2_MOSI | MUX_PAD_CTRL(SPI_PAD_CTRL),
        MX8MP_PAD_ECSPI2_MISO__ECSPI2_MISO | MUX_PAD_CTRL(SPI_PAD_CTRL),
        MX8MP_PAD_ECSPI2_SS0__GPIO5_IO13  | MUX_PAD_CTRL(NO_PAD_CTRL),
};

static void setup_spi(void)
{
        imx_iomux_v3_setup_multiple_pads(ecspi2_pads, ARRAY_SIZE(ecspi2_pads));
        init_clk_ecspi(1);
}
#endif

int board_init(void)
{
	board_pm_init();

	board_vendor_init();

#ifdef CONFIG_MXC_SPI
	setup_spi();
#endif

	return 0;
}

#if defined(CONFIG_FEC_MXC) || defined(CONFIG_DWC_ETH_QOS)
/* SoM EEPROM MAC slot indices (match common/fdt.c). */
#define FEC_MAC_EPROM_IDX	0
#define EQOS_MAC_EPROM_IDX	1

/*
 * MAC addresses from the SoM EEPROM.
 *
 * mach-imx's imx_get_mac_from_fuse() is a STRONG symbol in mainline, so a
 * board override would collide at link time. Instead we follow the
 * mainline-idiomatic path (cf. dh_imx8mp) and seed the ethaddr/eth1addr env
 * vars from the EEPROM here; the ethernet driver's write_hwaddr() then
 * programs the controller from the env. DT mapping: ethernet0 = &fec (eth0,
 * EEPROM iface 0), ethernet1 = &eqos (eth1, EEPROM iface 1).
 */
static void cl_setup_mac_env(const char *env, uint iface)
{
	unsigned char enetaddr[6];

	/* Never clobber a MAC the user already provisioned in the env. */
	if (eth_env_get_enetaddr(env, enetaddr))
		return;

	if (cl_eeprom_read_n_mac_addr(enetaddr, iface, CONFIG_SYS_I2C_EEPROM_BUS))
		return;

	if (!is_valid_ethaddr(enetaddr))
		return;

	eth_env_set_enetaddr(env, enetaddr);
}

static void cl_setup_mac_addr(void)
{
	cl_setup_mac_env("ethaddr", FEC_MAC_EPROM_IDX);
	cl_setup_mac_env("eth1addr", EQOS_MAC_EPROM_IDX);
}
#else
static inline void cl_setup_mac_addr(void) { }
#endif

int board_late_init(void)
{
#ifdef CONFIG_ENV_IS_IN_MMC
	board_late_mmc_env_init();
#endif
	cl_setup_mac_addr();

#ifdef CONFIG_ENV_VARS_UBOOT_RUNTIME_CONFIG
	env_set("board_name", CONFIG_SYS_BOARD);
	env_set("board_rev", "iMX8MP");
#endif

#ifdef CONFIG_DRAM_D2
	env_set("dram_subset", "d2");
#endif
#ifdef CONFIG_DRAM_D4
	env_set("dram_subset", "d4");
#endif
#ifdef CONFIG_DRAM_D1D8
	env_set("dram_subset", "d1d8");
#endif
	board_vendor_late_init();

	/*
	 * INTEGRATION TODO: do_pbb_restore() (power-button on_time/btn_press_time
	 * restore) lives in the not-yet-built cmd.c. Re-enable together with
	 * cmd.o once that file's deps (SUPPORTED_CONF, lppdr4_get_ram_name) are
	 * wired up. Not required to boot.
	 */
	return 0;
}

#ifdef CONFIG_FSL_FASTBOOT
#ifdef CONFIG_ANDROID_RECOVERY
int is_recovery_key_pressing(void)
{
	return 0; /*TODO*/
}
#endif /*CONFIG_ANDROID_RECOVERY*/
#endif /*CONFIG_FSL_FASTBOOT*/

#ifdef CONFIG_ANDROID_SUPPORT
bool is_power_key_pressed(void) {
	return (bool)(!!(readl(SNVS_HPSR) & (0x1 << 6)));
}
#endif

#ifdef CONFIG_SPL_MMC_SUPPORT

#define UBOOT_RAW_SECTOR_OFFSET 0x40
unsigned long spl_mmc_get_uboot_raw_sector(struct mmc *mmc)
{
	u32 boot_dev = spl_boot_device();
	switch (boot_dev) {
		case BOOT_DEVICE_MMC2:
			return CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR - UBOOT_RAW_SECTOR_OFFSET;
		default:
			return CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR;
	}
}
#endif

#define PHY_VENDOR_ID_MASK (( 1<<5 ) - 1 )
#define PHY_ATEROS_ID  0x7
#define PHY_REALTEK_ID 0x11

int get_phy_id(struct mii_dev *bus, int addr, int devad, u32 *phy_id)
{
	int phy_reg;

	/*
	 * Grab the bits from PHYIR1, and put them
	 * in the upper half
	 */
	phy_reg = bus->read(bus, addr, devad, MII_PHYSID1);

	if (phy_reg < 0)
		return -EIO;

	*phy_id = (phy_reg & 0xffff) << 16;

	/* Grab the bits from PHYIR2, and put them in the lower half */
	phy_reg = bus->read(bus, addr, devad, MII_PHYSID2);

	if (phy_reg < 0)
		return -EIO;

	*phy_id |= (phy_reg & 0xffff);

#ifdef CONFIG_TARGET_SOM_IMX8M_PLUS
	/* Specical case for REALTEK */
	phy_reg = (( phy_reg >> 4 ) & PHY_VENDOR_ID_MASK);
	if ((addr ==  0) &&  (phy_reg == PHY_REALTEK_ID)) {
		return -ENODEV;
	}
#endif
	return 0;
}

static int mx8_rgmii_rework_realtek(struct phy_device *phydev)
{
#define TXDLY_MASK ((1 << 13) | (1 << 12))
#define RXDLY_MASK ((1 << 13) | (1 << 11))

	unsigned short val;

	/* introduce tx clock delay */
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1f, 0x7);
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1e, 0xa4);

	val = phy_read(phydev, MDIO_DEVAD_NONE, 0x1c);
	val |= TXDLY_MASK;
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1c, val);

	/* introduce rx clock delay */
	val = phy_read(phydev, MDIO_DEVAD_NONE, 0x1c);
	val |= RXDLY_MASK;
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1c, val);

	phy_write(phydev, MDIO_DEVAD_NONE, 0x1f, 0x0);

	/*LEDs:*/
	/* set to extension page */
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1f, 0x0007);
	/* extension Page44 */
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1e, 0x002c);
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1c, 0x0430);
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1a, 0x0010);
	/* To disable EEE LED mode (blinking .4s/2s) */
	/* extension Page5 */
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1f, 0x0005);
	phy_write(phydev, MDIO_DEVAD_NONE, 0x05, 0x8b82);
	phy_write(phydev, MDIO_DEVAD_NONE, 0x06, 0x052b);

	phy_write(phydev, MDIO_DEVAD_NONE, 0x1f, 0x00);
	return 0;
}

static int mx8_rgmii_rework(struct phy_device *phydev)
{
	unsigned short val = phy_read(phydev, MDIO_DEVAD_NONE, 0x3);

	val = (( val >> 4 ) & PHY_VENDOR_ID_MASK);

	switch (val) {
	case PHY_ATEROS_ID:
		break;
	case PHY_REALTEK_ID:
		mx8_rgmii_rework_realtek(phydev);
		break;
	default:
		break;
	}

	return 0;
}

