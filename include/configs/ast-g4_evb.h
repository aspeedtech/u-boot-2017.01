/*
 * Copyright (C) 2012-2020  ASPEED Technology Inc.
 * Ryan Chen <ryan_chen@aspeedtech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef __CONFIG_H
#define __CONFIG_H

/*#define DEBUG 1*/

#define CONFIG_MACH_TYPE		MACH_TYPE_ASPEED

#include <asm/arch/platform.h>

/* Misc CPU related */
#define CONFIG_CMDLINE_TAG		/* enable passing of ATAGs */
#define CONFIG_SETUP_MEMORY_TAGS
#define CONFIG_INITRD_TAG

#define CONFIG_CMDLINE_EDITING		1	/* command line history */

/* ------------------------------------------------------------------------- */
/* additions for new relocation code, must added to all boards */
#define CONFIG_SYS_SDRAM_BASE		(AST_DRAM_BASE)
#define CONFIG_SYS_INIT_RAM_ADDR	CONFIG_SYS_SDRAM_BASE
#define CONFIG_SYS_INIT_RAM_SIZE	(32*1024)
#define CONFIG_SYS_INIT_SP_ADDR	(CONFIG_SYS_SDRAM_BASE + 0x1000 - GENERATED_GBL_DATA_SIZE)

#define CONFIG_NR_DRAM_BANKS		1

#define CONFIG_SYS_MEMTEST_START		CONFIG_SYS_SDRAM_BASE + 0x300000
#define CONFIG_SYS_MEMTEST_END			(CONFIG_SYS_MEMTEST_START + (80*1024*1024))
/*-----------------------------------------------------------------------*/

#define CONFIG_SYS_TEXT_BASE            0
#define CONFIG_SYS_UBOOT_BASE		CONFIG_SYS_TEXT_BASE

/* Memory Info  */
#define CONFIG_SYS_MALLOC_LEN   	(0x1000 + 4*1024*1024) /* malloc() len */

/* NS16550 Configuration */
#define CONFIG_SYS_NS16550_SERIAL
#define CONFIG_SYS_NS16550_REG_SIZE		(-4)
#define CONFIG_SYS_NS16550_CLK			24000000
#define CONFIG_SYS_NS16550_COM1			AST_UART0_BASE
#define CONFIG_SYS_LOADS_BAUD_CHANGE
#define CONFIG_SERIAL1					1
#define CONFIG_CONS_INDEX				1
#define CONFIG_BAUDRATE					115200

/* BOOTP options  */
#define CONFIG_BOOTP_BOOTFILESIZE
#define CONFIG_BOOTP_BOOTPATH
#define CONFIG_BOOTP_GATEWAY
#define CONFIG_BOOTP_HOSTNAME
#define CONFIG_BOOTP_SUBNETMASK

/*
 * Environment Config
 */

#define CONFIG_BOOTFILE		"all.bin"

/*
 * Miscellaneous configurable options
 */
#define CONFIG_SYS_LONGHELP	/* undef to save memory */
#define CONFIG_SYS_CBSIZE	256		/* Console I/O Buffer Size */

/* Print Buffer Size */
#define CONFIG_SYS_PBSIZE	(CONFIG_SYS_CBSIZE + sizeof(CONFIG_SYS_PROMPT) + 16)
#define CONFIG_SYS_MAXARGS	16		/* max number of command args */
#define CONFIG_SYS_BARGSIZE	CONFIG_SYS_CBSIZE /* Boot Argument Buffer Size */

#define CONFIG_SYS_LOAD_ADDR	0x43000000	/* default load address */

#define CONFIG_BOOTARGS		"console=ttyS0,115200n8 root=/dev/ram rw init=/linuxrc"

#define CONFIG_EXTRA_ENV_SETTINGS \
	"verify=yes\0"\
	"update=tftp 40800000 ast2400.scr; so 40800000\0"\
	"ramfs=set bootargs console=ttyS0,115200n8 root=/dev/ram rw init=/linuxrc\0" \
	"squashfs=set bootargs console=ttyS0,115200n8 root=/dev/mtdblock4 rootfs=squashfs init=/linuxrc\0"\
	""

/* ------------------------------------------------------------------------- */
/* I2C definition */
#ifdef CONFIG_CMD_I2C
#define CONFIG_HARD_I2C		1		/* To enable I2C support	*/
/* #define CONFIG_CMD_EEPROM */
/* #define CONFIG_ENV_EEPROM_IS_ON_I2C */
#define CONFIG_SYS_I2C_EEPROM_ADDR_LEN		2

#define CONFIG_AST_I2C		1
#define CONFIG_I2C_MULTI_BUS	1
#define CONFIG_SYS_MAX_I2C_BUS	8	/* i2c-9 i2c-10 is for SD */

#define CONFIG_SYS_I2C_SPEED	100000 /* 100 kHz */
#define CONFIG_SYS_I2C_INIT_BOARD
#define CONFIG_SYS_I2C_SLAVE	0xfe
#define CONFIG_I2C_EDID

#define CONFIG_SYS_I2C_EEPROM_ADDR		0x50
/*#define CONFIG_SYS_I2C_MAC_OFFSET		0x800 */
#endif

/*-----------------------------------------------------------------------
 * File System Configuration
 */
/* FAT FS */
#define CONFIG_DOS_PARTITION
#define CONFIG_PARTITION_UUIDS
#define CONFIG_SUPPORT_VFAT
#define CONFIG_FS_FAT
#define CONFIG_FAT_WRITE
#define CONFIG_CMD_PART

/* ------------------------------------------------------------------------- */
/* Serial Flash */
#ifdef CONFIG_CMD_SF
#define CONFIG_SF_DEFAULT_BUS		0
#define CONFIG_SF_DEFAULT_CS		0
#define CONFIG_SF_DEFAULT_SPEED	50000000
#define CONFIG_SF_DEFAULT_MODE		SPI_MODE_3
#endif

#define CONFIG_SYS_NO_FLASH
#define CONFIG_ENV_SPI_BUS		CONFIG_SF_DEFAULT_BUS
#define CONFIG_ENV_SPI_CS		CONFIG_SF_DEFAULT_CS
#define CONFIG_ENV_SPI_MAX_HZ	CONFIG_SF_DEFAULT_SPEED //50MHz
#define CONFIG_ENV_SPI_MODE		CONFIG_SF_DEFAULT_MODE
#define CONFIG_SYS_MAX_FLASH_BANKS		(0)
#define CONFIG_ENV_IS_IN_SPI_FLASH		1
#define CONFIG_ENV_SECT_SIZE		0x10000		//4K sector
#define CONFIG_ENV_OFFSET			0x60000	/* environment starts here  */
#define CONFIG_ENV_SIZE				0x10000	/* Total Size of Environment Sector */
/* ------------------------------------------------------------------------- */
/* mtdparts command line support */
#define CONFIG_CMD_MTDPARTS
#define CONFIG_MTD_DEVICE
#define MTDIDS_DEFAULT          "nor0=spi-flash.0"
#define MTDPARTS_DEFAULT   \
						"mtdparts=spi-flash.0:0x60000@0x0(u-boot)," \
						"0x10000@0x60000(u-boot-env)," \
						"0x10000@0x70000(dtb)," \
						"0x380000@0x80000(uImage)," \
						"0x400000@0x400000(rootfs)," \
						"-(pd_rootfs)"
/* ------------------------------------------------------------------------- */
#define CONFIG_BOOTCOMMAND	"bootm 20080000 20400000 20070000"
#define CONFIG_ENV_OVERWRITE

/* ------------------------------------------------------------------------- */
#define CONFIG_GATEWAYIP		192.168.0.1
#define CONFIG_NETMASK			255.255.255.0
#define CONFIG_IPADDR			192.168.0.45
#define CONFIG_SERVERIP			192.168.0.81


/* -------------------------------------------------------------------------
 *  2. UART5 message output
 * #define	 CONFIG_DRAM_UART_38400
 * 3. DRAM Type
 * #define CONFIG_DDR3_8GSTACK     DDR3 8Gbit Stack die
 */
#endif	/* __CONFIG_H */
