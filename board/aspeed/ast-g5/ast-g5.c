/*
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * (C) Copyright 2002
 * David Mueller, ELSOFT AG, <d.mueller@elsoft.ch>
 *
 * (C) Copyright 2003
 * Texas Instruments, <www.ti.com>
 * Kshitij Gupta <Kshitij@ti.com>
 *
 * (C) Copyright 2004
 * ARM Ltd.
 * Philippe Robin, <philippe.robin@arm.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <netdev.h>
#include <asm/arch/ast-scu.h>
#include <asm/arch/ast-sdmc.h>
#include <asm/io.h>

DECLARE_GLOBAL_DATA_PTR;

extern int ast_sdhi_init(u32 regbase, u32 max_clk, u32 min_clk);

#if defined(CONFIG_SHOW_BOOT_PROGRESS)
void show_boot_progress(int progress)
{
    printf("Boot reached stage %d\n", progress);
}
#endif

int board_init(void)
{
	/* adress of boot parameters */
	gd->bd->bi_boot_params = CONFIG_SYS_SDRAM_BASE + 0x100;
	gd->flags = 0;
	return 0;
}

int misc_init_r (void)
{
#ifdef CONFIG_CPU1
	//uart 3/4 shar pin
	//*((volatile ulong*) 0x1e6e2080) = 0xffff0000;
	//uart 1/2 shar pin
	*((volatile ulong*) 0x1e6e2084) = 0xffff0000;
	//LPC UART1/2 reset [clear bit 4/5]
	*((volatile ulong*) 0x1e789098) = 0x00000a00;
	//mapping table
	*((volatile ulong*) 0x1e6e2104) = CONFIG_CPU1_MAP_FLASH;
	//IO table
	*((volatile ulong*) 0x1e6e211c) = 0x1e600000;
	//IO table
	*((volatile ulong*) 0x1e6e2120) = 0x1e700000;
	//Sram
	*((volatile ulong*) 0x1e6e2118) = CONFIG_CPU1_MAP_SRAM;	

	*((volatile ulong*) 0x1e6e2124) = CONFIG_CPU1_MAP_DRAM;	

	//Enable coldfire V1 clock
//	*((volatile ulong*) 0x1e6e2100) = 0x01;	

	printf("Coldfire V1 : UART1 \n");
#endif

#ifdef CONFIG_AST_WATCHDOG
	wdt_start(CONFIG_AST_WATCHDOG_TIMEOUT);
#endif

	return 0;

}

int dram_init(void)
{
	/* dram_init must store complete ramsize in gd->ram_size */
#ifdef CONFIG_DRAM_ECC
	gd->ram_size = CONFIG_DRAM_ECC_SIZE;
#else
	u32 vga = ast_scu_get_vga_memsize();
	u32 dram = ast_sdmc_get_mem_size();

	gd->ram_size = (dram - vga - CONFIG_AST_VIDEO_SIZE);
#endif
	return 0;
}

int board_eth_init(bd_t *bd)
{
#ifdef CONFIG_FTGMAC100
	return ftgmac100_initialize(bd);
#endif
}

#ifdef CONFIG_GENERIC_MMC

#define CONFIG_SYS_MMC_NUM		2
#define CONFIG_SYS_MMC_BASE		{AST_SDHCI_SLOT0_BASE, AST_SDHCI_SLOT1_BASE}

int board_mmc_init(bd_t *bis)
{
	ulong mmc_base_address[CONFIG_SYS_MMC_NUM] = CONFIG_SYS_MMC_BASE;
	u8 i;

	ast_scu_init_sdhci();
	ast_scu_multi_func_sdhc_slot(3);
	//multipin. Remind: AST2300FPGA only supports one port at a time
	for (i = 0; i < CONFIG_SYS_MMC_NUM; i++) {
		if (ast_sdhi_init(mmc_base_address[i], ast_get_sd_clock_src(), 100000))
			return 1;
	}
	return 0;
}
#endif

#if defined(CONFIG_ARCH_AST3200)
int power_init_board(void)
{
	/****************************
		ping mux configuration 
		power control : output 
		GPIOF6	  : output
		GPIOH1&H2 : output 
		GPIOH4&H5 : output 
		GPIOH6&H7 : output 
		SRST2# GPIOAC1 : output 
		// LED 
		GPIOT6,T7 : 16mA config 
		// SWITCH BUTTON 
		SW_BTN#: GPIOF5
		FAC_RST_BTN#: GPIOF3
		MAC1RGMII.O_LINK : GPIOF0
	*****************************/
	/* pin mux */
	//POWER GPIOF6
	__raw_writel( ( __raw_readl(0x1E6E2080) & 0xBFFFFFFF ), 0x1E6E2080); // Clear bit 30
	__raw_writel( ( __raw_readl(0x1E7890A0) & 0xFFFFFFFE ), 0x1E7890A0); // Clear LHCR0 bit 0
	
	//POWER GPIO H0,H1,H2 20150903
	__raw_writel( ( __raw_readl(0x1E6E2090) & 0xFFFFFF3F ), 0x1E6E2090); // Clear bit 6 and 7
	__raw_writel( ( __raw_readl(0x1E6E2094) & 0xFFFFFF9F ), 0x1E6E2094); // Clear bit 5 and 6
	
	//POWER GPIOH4,H5
	__raw_writel( ( __raw_readl(0x1E6E2090) & 0xFFFFFF3F ), 0x1E6E2090); // Clear bit 6 and 7
	__raw_writel( ( __raw_readl(0x1E6E2094) & 0xFFFFFF7F ), 0x1E6E2094); // Clear bit 7
	
	//POWER GPIOH6,H7
	__raw_writel( ( __raw_readl(0x1E6E2090) & 0xFFFFFF3F ), 0x1E6E2090); // Clear bit 6 and 7
	
	// SRST2# : OUTPUT : GPIOAC1
	__raw_writel( ( __raw_readl(0x1E6E2070) & 0xFDFFFFFF ), 0x1E6E2070); // Clear bit 25
	__raw_writel( ( __raw_readl(0x1E6E20AC) & 0xFFFFFFFD ), 0x1E6E20AC); // Clear bit 1
	
	//LED : GPIOT6,T7 - 16mA
	__raw_writel( ( __raw_readl(0x1E6E20A0) | 0x000000C0 ), 0x1E6E20A0); // Set bit 7 6
	__raw_writel( ( __raw_readl(0x1E6E2070) | 0x00000080 ), 0x1E6E2070); // Set bit 7
				  
	// 16mA I/O
	__raw_writel( ( __raw_readl(0x1E6E2090) | 0x00000C00 ), 0x1E6E2090); // Set bit 10 11
	
	// INPUT :	GPIOF0, GPIOF3, GPIOF5
	__raw_writel( ( __raw_readl(0x1E6E2080) & 0xD6FFFFFF ), 0x1E6E2080); // Clear bit 29 27 24
	__raw_writel( ( __raw_readl(0x1E6E2090) & 0xBFFFFFFF ), 0x1E6E2090); // Clear bit 30
										
	/***************************
		direction 
	*****************************/
	//POWER: OUTPUT  GPIOF6,GPIO H0, H1, H2, H4, H5, H6, H7  20150903
	__raw_writel( ( __raw_readl(0x1E780024) | 0xF7004000 ), 0x1E780024); // Set bit 14, 24, 25, 26, 28, 29, 30, 31 20150903
	
	// SRST2# : OUTPUT : GPIOAC1
	__raw_writel( ( __raw_readl(0x1E7801EC) | 0x00000002 ), 0x1E7801EC); // Set bit 1
	
	//LED : GPIOT6,T7
	__raw_writel( ( __raw_readl(0x1E780084) | 0xC0000000 ), 0x1E780084); // Set bit 31 30
	
	// INPUT :	GPIOF0, GPIOF3, GPIOF5
	__raw_writel( ( __raw_readl(0x1E780024) & 0xFFFFFFD6 ), 0x1E780024); // Clear bit 5 3 0
	
	/***************************
		power sequence form 0618
	*****************************/
	// H0 to high 20150903
	__raw_writel( ( __raw_readl(0x1E7800C4) | 0x01000000 ), 0x1E780020); // Set bit 24, GPIO H0  20150903
	
	// SRST2#
	__raw_writel( ( __raw_readl(0x1E7801E8) & 0xFFFFFFFD ), 0x1E7801E8); // Clear bit 1, GPIO AC1
	
	// F6
	__raw_writel( 0x00004000, 0x1E780020);								 // Set bit 15 14, GPIO F6
	mdelay(80);
	
	// H1, H2
	__raw_writel( ( __raw_readl(0x1E7800C4) | 0x06000000 ), 0x1E780020); // Set bit 25 26, GPIO H1 H2
	mdelay(10);
	
	// H5
	__raw_writel( ( __raw_readl(0x1E7800C4) | 0x20000000 ), 0x1E780020); // Set bit 29, GPIO H5 
	mdelay(10);
	
	// H6
	__raw_writel( ( __raw_readl(0x1E7800C4) | 0x40000000 ), 0x1E780020); // Set bit 30, GPIO H6
	mdelay(50);
	
	// H4
	__raw_writel( ( __raw_readl(0x1E7800C4) | 0x10000000 ), 0x1E780020); // Set bit 28, GPIO H4
	
	// AC1, H7
	__raw_writel( 0x00000002, 0x1E7801E8);								 // Set bit 1, GPIO AC1
	__raw_writel( ( __raw_readl(0x1E7800C4) | 0x80000000 ), 0x1E780020); // Set bit 31, GPIO H7
	mdelay(40);    
	
	// H6
	__raw_writel( ( __raw_readl(0x1E7800C4) & 0xBFFFFFFF ), 0x1E780020); // Clear bit 30, GPIO H6
	mdelay(40);    
	
	// H6
	__raw_writel( ( __raw_readl(0x1E7800C4) | 0x40000000 ), 0x1E780020); // Set bit 30, GPIO H6
	
//	printf("Power ON(A1).\n"); 20150623

	return 0;
}
#endif

