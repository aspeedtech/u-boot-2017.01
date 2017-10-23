/*
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#include <common.h>
#include <command.h>
/*
#if defined(CONFIG_ARCH_AST2500) || defined(CONFIG_ARCH_AST2400) || defined(CONFIG_ARCH_AST2300) || defined(CONFIG_ARCH_AST3200)
  #include "../arch/arm/cpu/ast-common/SWFUNC.H"
  #include "../arch/arm/cpu/ast-common/COMMINF.H"
#elif defined(CONFIG_AST1010)
  #include "../board/aspeed/SWFUNC.H"
  #include "../board/aspeed/COMMINF.H"
#else
  #include "SWFUNC.H"
  #include "COMMINF.H"
#endif
*/

#if defined(CONFIG_AST1010)
  #include "../board/aspeed/SWFUNC.H"
  #include "../board/aspeed/COMMINF.H"
#else
  #include "SWFUNC.H"
  #include "COMMINF.H"
#endif

#ifdef SLT_UBOOT
extern int main_function(int argc, char * const argv[], char mode);
//extern unsigned long int strtoul(char *string, char **endPtr, int base);

int do_mactest (cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
{
	display_lantest_log_msg = 0;
	return main_function( argc, argv, MODE_DEDICATED );
}

int do_ncsitest (cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
{
	display_lantest_log_msg = 0;
	return main_function( argc, argv, MODE_NSCI );
}

U_BOOT_CMD(
	mactest,    NETESTCMD_MAX_ARGS, 0,  do_mactest,
	"Dedicated LAN test program",
	NULL
);
U_BOOT_CMD(
	ncsitest,    NETESTCMD_MAX_ARGS, 0,  do_ncsitest,
	"Share LAN (NC-SI) test program",
	NULL
);

// ------------------------------------------------------------------------------
int do_mactestd (cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
{
	display_lantest_log_msg = 1;
	return main_function( argc, argv, MODE_DEDICATED );
}

int do_ncsitestd (cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
{
	display_lantest_log_msg = 1;
	return main_function( argc, argv, MODE_NSCI );
}

U_BOOT_CMD(
	mactestd,    NETESTCMD_MAX_ARGS, 0,  do_mactestd,
	"Dedicated LAN test program and display more information",
	""
);
U_BOOT_CMD(
	ncsitestd,    NETESTCMD_MAX_ARGS, 0,  do_ncsitestd,
	"Share LAN (NC-SI) test program and display more information",
	NULL
);

// ------------------------------------------------------------------------------
void multi_pin_2_mdcmdio_init( MAC_ENGINE *eng )
{
  #if defined(CONFIG_AST1010)
	Write_Reg_SCU_DD( 0x088, ((Read_Reg_SCU_DD( 0x088 ) & 0xff3fffff ) | 0x00C00000) );//Multi-function Pin Control //[22]MDC, [23]MDIO
  #elif defined(CONFIG_ARCH_AST2500) || defined(CONFIG_ARCH_AST2400) || defined(CONFIG_ARCH_AST2300) || defined(CONFIG_ARCH_AST3200)
	switch ( eng->run.MAC_idx_PHY ) {
		case 0  : Write_Reg_SCU_DD( 0x088, (Read_Reg_SCU_DD( 0x088 ) | 0xC0000000)              ); break;//[31]MAC1 MDIO, [30]MAC1 MDC
		case 1  : Write_Reg_SCU_DD( 0x090, (Read_Reg_SCU_DD( 0x090 ) | 0x00000004)              ); break;//[2 ]MAC2 MDC/MDIO
		default : break;
	}
  #endif
} 

int do_phyread (cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
{
	MAC_ENGINE	MACENG;
	MAC_ENGINE	*eng;
	int     MACnum;
	int     PHYreg;
	ULONG   result_data;
	int     ret = 0;
	int     PHYaddr;
	int     timeout = 0;
	ULONG   MAC_040;

	eng = &MACENG;
	do {
		if ( argc != 4 ) {
			printf(" Wrong parameter number.\n");
			printf(" phyr mac addr reg\n");
			printf("   mac     : 0 or 1.   [hex]\n");
			printf("   PHY addr: 0 to 0x1F.[hex]\n");
			printf("   register: 0 to 0xFF.[hex]\n");
			printf(" example: phyr 0 0 1\n");
			ret = -1;
			break;
		}

		MACnum  = simple_strtoul(argv[1], NULL, 16);
		PHYaddr = simple_strtoul(argv[2], NULL, 16);
		PHYreg  = simple_strtoul(argv[3], NULL, 16);

		if ( MACnum == 0 ) {
			// Set MAC 0
			eng->run.MAC_BASE = MAC_BASE1;
			eng->run.MAC_idx_PHY  = 0;
		}
		else if ( MACnum == 1 ) {
			// Set MAC 1
			eng->run.MAC_BASE = MAC_BASE2;
			eng->run.MAC_idx_PHY  = 1;
		}
		else {
			printf("wrong parameter (mac number)\n");
			ret = -1;
			break;
		}

		if ( ( PHYaddr < 0 ) || ( PHYaddr > 31 ) ) {
			printf("wrong parameter (PHY address)\n");
			ret = -1;
			break;
		}

		multi_pin_2_mdcmdio_init( eng );
		MAC_040 = Read_Reg_MAC_DD( eng, 0x40 );
		eng->inf.NewMDIO = (MAC_040 & 0x80000000) ? 1 : 0;

		if ( eng->inf.NewMDIO ) {
			Write_Reg_MAC_DD( eng, 0x60, MAC_PHYRd_New | (PHYaddr << 5) | ( PHYreg & 0x1f ) );
			while ( Read_Reg_MAC_DD( eng, 0x60 ) & MAC_PHYBusy_New ) {
				if ( ++timeout > TIME_OUT_PHY_RW ) {
					ret = -1;
					break;
				}
			}
#ifdef Delay_PHYRd
			DELAY( Delay_PHYRd );
#endif
			result_data = Read_Reg_MAC_DD( eng, 0x64 ) & 0xffff;
		}
		else {
			Write_Reg_MAC_DD( eng, 0x60, MDC_Thres | MAC_PHYRd | (PHYaddr << 16) | ((PHYreg & 0x1f) << 21) );
			while ( Read_Reg_MAC_DD( eng, 0x60 ) & MAC_PHYRd ) {
				if ( ++timeout > TIME_OUT_PHY_RW ) {
					ret = -1;
					break;
				}
			}
#ifdef Delay_PHYRd
			DELAY( Delay_PHYRd );
#endif
			result_data = Read_Reg_MAC_DD( eng, 0x64 ) >> 16;
		}
		printf(" PHY[%d] reg[0x%02X] = %04lx\n", PHYaddr, PHYreg, result_data );
	} while ( 0 );

	return ret;
}

int do_phywrite (cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
{
	MAC_ENGINE	MACENG;
	MAC_ENGINE	*eng;
	int     MACnum;
	int     PHYreg;
	int     PHYaddr;
	ULONG   reg_data;
	int     ret     = 0;
	int     timeout = 0;
	ULONG   MAC_040;

	eng = &MACENG;

	do {
		if ( argc != 5 )
		{
			printf(" Wrong parameter number.\n");
			printf(" phyw mac addr reg data\n");
			printf("   mac     : 0 or 1.     [hex]\n");
			printf("   PHY addr: 0 to 0x1F.  [hex]\n");
			printf("   register: 0 to 0xFF.  [hex]\n");
			printf("   data    : 0 to 0xFFFF.[hex]\n");
			printf(" example: phyw 0 0 0 610\n");
			ret = -1;
			break;
		}

		MACnum   = simple_strtoul(argv[1], NULL, 16);
		PHYaddr  = simple_strtoul(argv[2], NULL, 16);
		PHYreg   = simple_strtoul(argv[3], NULL, 16);
		reg_data = simple_strtoul(argv[4], NULL, 16);

		if ( MACnum == 0 ) {
			// Set MAC 0
			eng->run.MAC_BASE  = MAC_BASE1;
			eng->run.MAC_idx_PHY  = 0;
		}
		else if ( MACnum == 1 ) {
			// Set MAC 1
			eng->run.MAC_BASE  = MAC_BASE2;
			eng->run.MAC_idx_PHY  = 1;
		}
		else {
			printf("wrong parameter (mac number)\n");
			ret = -1;
			break;
		}

		if ( ( PHYaddr < 0 ) || ( PHYaddr > 31 ) ) {
			printf("wrong parameter (PHY address)\n");
			ret = -1;
			break;
		}

		multi_pin_2_mdcmdio_init( eng );
		MAC_040 = Read_Reg_MAC_DD( eng, 0x40 );
		eng->inf.NewMDIO = (MAC_040 & 0x80000000) ? 1 : 0;

		if ( eng->inf.NewMDIO ) {
			Write_Reg_MAC_DD( eng, 0x60, ( reg_data << 16 ) | MAC_PHYWr_New | (PHYaddr<<5) | (PHYreg & 0x1f) );

			while ( Read_Reg_MAC_DD( eng, 0x60 ) & MAC_PHYBusy_New ) {
				if ( ++timeout > TIME_OUT_PHY_RW ) {
					ret = -1;
					break;
				}
			}
		}
		else {
			Write_Reg_MAC_DD( eng, 0x64, reg_data );
			Write_Reg_MAC_DD( eng, 0x60, MDC_Thres | MAC_PHYWr | (PHYaddr<<16) | ((PHYreg & 0x1f) << 21) );

			while ( Read_Reg_MAC_DD( eng, 0x60 ) & MAC_PHYWr ) {
				if ( ++timeout > TIME_OUT_PHY_RW ) {
					ret = -1;
					break;
				}
			}
		} // End if ( eng->inf.NewMDIO )

		printf("Write: PHY[%d] reg[0x%02X] = %04lx\n", PHYaddr, PHYreg, reg_data );
	} while ( 0 );

	return ret;
}

int do_phydump (cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
{
	MAC_ENGINE	MACENG;
	MAC_ENGINE	*eng;
	int     MACnum;
	int     PHYreg;
	ULONG   result_data;
	int     ret = 0;
	int     PHYaddr;
	int     timeout = 0;
	ULONG   MAC_040;

	eng = &MACENG;
	do {
		if ( argc != 3 ) {
			printf(" Wrong parameter number.\n");
			printf(" phyd mac addr\n");
			printf("   mac     : 0 or 1.   [hex]\n");
			printf("   PHY addr: 0 to 0x1F.[hex]\n");
			printf(" example: phyd 0 0\n");
			ret = -1;
			break;
		}

		MACnum  = simple_strtoul(argv[1], NULL, 16);
		PHYaddr = simple_strtoul(argv[2], NULL, 16);

		if ( MACnum == 0 ) {
			// Set MAC 0
			eng->run.MAC_BASE = MAC_BASE1;
			eng->run.MAC_idx_PHY  = 0;
		}
		else if ( MACnum == 1 ) {
			// Set MAC 1
			eng->run.MAC_BASE = MAC_BASE2;
			eng->run.MAC_idx_PHY  = 1;            
		}
		else {
			printf("wrong parameter (mac number)\n");
			ret = -1;
			break;
		}

		if ( ( PHYaddr < 0 ) || ( PHYaddr > 31 ) ) {
			printf("wrong parameter (PHY address)\n");
			ret = -1;
			break;
		}

		multi_pin_2_mdcmdio_init( eng );
		MAC_040 = Read_Reg_MAC_DD( eng, 0x40 );
		eng->inf.NewMDIO = (MAC_040 & 0x80000000) ? 1 : 0;

		if ( eng->inf.NewMDIO ) {
			for ( PHYreg = 0; PHYreg < 32; PHYreg++ ) {
				
				Write_Reg_MAC_DD( eng, 0x60, MAC_PHYRd_New | (PHYaddr << 5) | ( PHYreg & 0x1f ) );
				while ( Read_Reg_MAC_DD( eng, 0x60 ) & MAC_PHYBusy_New ) {
					if ( ++timeout > TIME_OUT_PHY_RW ) {
						ret = -1;
						break;
					}
				}
#ifdef Delay_PHYRd
				DELAY( Delay_PHYRd );
#endif
				result_data = Read_Reg_MAC_DD( eng, 0x64 ) & 0xffff;
				switch ( PHYreg % 4 ) {
					case 0	: printf("%02d| %04lx ", PHYreg, result_data ); break;
					case 3	: printf("%04lx\n", result_data ); break;
					default	: printf("%04lx ", result_data ); break;
				}
			}
		}
		else {
			for ( PHYreg = 0; PHYreg < 32; PHYreg++ ) {
				Write_Reg_MAC_DD( eng, 0x60, MDC_Thres | MAC_PHYRd | (PHYaddr << 16) | ((PHYreg & 0x1f) << 21) );
				while ( Read_Reg_MAC_DD( eng, 0x60 ) & MAC_PHYRd ) {
					if ( ++timeout > TIME_OUT_PHY_RW ) {
						ret = -1;
						break;
					}
				}
#ifdef Delay_PHYRd
				DELAY( Delay_PHYRd );
#endif
				result_data = Read_Reg_MAC_DD( eng, 0x64 ) >> 16;
				switch ( PHYreg % 4 ) {
					case 0	: printf("%02d| %04lx ", PHYreg, result_data ); break;
					case 3	: printf("%04lx\n", result_data ); break;
					default	: printf("%04lx ", result_data ); break;
				}
			}
		}
	} while ( 0 );

	return ret;
}

U_BOOT_CMD(
	phyr,    NETESTCMD_MAX_ARGS, 0,  do_phyread,
	"Read PHY register.  (phyr mac addr reg)",
	NULL
);

U_BOOT_CMD(
	phyw,    NETESTCMD_MAX_ARGS, 0,  do_phywrite,
	"Write PHY register. (phyw mac addr reg data)",
	NULL
);

U_BOOT_CMD(
	phyd,    NETESTCMD_MAX_ARGS, 0,  do_phydump,
	"Dump PHY register. (phyd mac addr)",
	NULL
);

// ------------------------------------------------------------------------------
int do_macgpio (cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
{
	Write_Reg_GPIO_DD( 0x78 , Read_Reg_GPIO_DD( 0x78 ) & 0xf7bfffff );
	Write_Reg_GPIO_DD( 0x7c , Read_Reg_GPIO_DD( 0x7c ) | 0x08400000 );
	DELAY( 100 );
	Write_Reg_GPIO_DD( 0x78 , Read_Reg_GPIO_DD( 0x78 ) | 0x08400000 );

	return 0;
}

U_BOOT_CMD(macgpio,    NETESTCMD_MAX_ARGS, 0,  do_macgpio,
	"Setting GPIO to trun on the system for the MACTEST/NCSITEST (OEM)",
	NULL	
);

/*
int do_clkduty (cmd_tbl_t * cmdtp, int flag, int argc, char *argv[])
{
	int temp;
	int i;
	char *re[10];

	temp   = strtoul(argv[1], NULL, 16);
	for ( i = 1; i < argc; i++ )
	{
		re[i] = argv[i + 1];
		printf("arg[%d]= %s\n", i , re[i]);
	}
	argc--;

	Write_Reg_SCU_DD( 0x1DC, 0 );
	main_function( argc, re, MODE_DEDICATED );
	printf("SCU1DC= %lx\n", Read_Reg_SCU_DD(0x1DC) );

	for ( i = 0; i < 64; i += temp )
	{
		Write_Reg_SCU_DD( 0x1DC, ( ((ULONG)(i + 0x40) << 16) | ((ULONG)(i + 0x40) << 8) ) );
		printf("SCU1DC= %lx [%lx]\n", Read_Reg_SCU_DD(0x1DC) , (ULONG)temp );
		main_function( argc, re, MODE_DEDICATED );
	}

	return 0;
}

U_BOOT_CMD(
	clkduty,    NETESTCMD_MAX_ARGS, 0,  do_clkduty,
	"clkduty",
	NULL
);
*/

#if defined(PHY_GPIO)
// ------------------------------------------------------------------------------
void multi_pin_2_gpio_init( MAC_ENGINE *eng )
{
  #if defined(CONFIG_AST1010)
	Write_Reg_SCU_DD( 0x088, ((Read_Reg_SCU_DD( 0x088 ) & 0xff3fffff ) | 0x00000000) );//Multi-function Pin Control //[22]MDC, [23]MDIO
  #elif defined(CONFIG_ARCH_AST2500) || defined(CONFIG_ARCH_AST2400) || defined(CONFIG_ARCH_AST2300) || defined(CONFIG_ARCH_AST3200)
	switch ( eng->run.MAC_idx_PHY ) {
		case 0  : Write_Reg_SCU_DD( 0x088, (Read_Reg_SCU_DD( 0x088 ) & 0x3fffffff)              ); break;//[31]MAC1 MDIO, [30]MAC1 MDC
		case 1  : Write_Reg_SCU_DD( 0x090, (Read_Reg_SCU_DD( 0x090 ) & 0xfffffffb)              ); break;//[2 ]MAC2 MDC/MDIO
		default : break;
	}
  #endif
} 

int do_phygread (cmd_tbl_t * cmdtp, int flag, int argc, char *argv[])
{
	MAC_ENGINE	MACENG;
	MAC_ENGINE	*eng;
	int     MACnum;
	int     PHYreg;
	ULONG   result_data;
	int     ret = 0;
	int     PHYaddr;
	int     GPIO_RdDelay = 0;

	eng = &MACENG;
	do {
		if ( argc == 5 ) {
			GPIO_RdDelay = strtoul(argv[4], NULL, 16);
		} else

		if ( argc != 4 ) {
			printf(" Wrong parameter number.\n");
			printf(" phyr mac addr reg\n");
			printf("   mac     : 0 or 1.   [hex]\n");
			printf("   PHY addr: 0 to 0x1F.[hex]\n");
			printf("   register: 0 to 0xFF.[hex]\n");
			printf(" example: phyr 0 0 1\n");
			ret = -1;
			break;
		}

		MACnum  = strtoul(argv[1], NULL, 16);
		PHYaddr = strtoul(argv[2], NULL, 16);
		PHYreg  = strtoul(argv[3], NULL, 16);

		if ( MACnum == 0 ) {
			// Set MAC 0
			eng->run.MAC_BASE = MAC_BASE1;
			eng->run.MAC_idx_PHY  = 0;
		}
		else if ( MACnum == 1 ) {
			// Set MAC 1
			eng->run.MAC_BASE = MAC_BASE2;
			eng->run.MAC_idx_PHY  = 1;
		}
		else {
			printf("wrong parameter (mac number)\n");
			ret = -1;
			break;
		}

		if ( ( PHYaddr < 0 ) || ( PHYaddr > 31 ) ) {
			printf("wrong parameter (PHY address)\n");
			ret = -1;
			break;
		}

		multi_pin_2_gpio_init( eng );
		phy_gpio_init( eng );
		eng->GPIO.Dat_RdDelay = GPIO_RdDelay;
		eng->phy.Adr = PHYaddr;
		result_data = phy_gpio_read( eng, PHYreg );
		printf("[GPIO][%d] PHY[%d] reg[0x%02X] = %04lx\n", eng->GPIO.Dat_RdDelay, PHYaddr, PHYreg, result_data );
	} while ( 0 );

	return ret;
}

int do_phygwrite (cmd_tbl_t * cmdtp, int flag, int argc, char *argv[])
{
	MAC_ENGINE	MACENG;
	MAC_ENGINE	*eng;
	int     MACnum;
	int     PHYreg;
	int     PHYaddr;
	ULONG   reg_data;
	int     ret     = 0;

	eng = &MACENG;

	do {
		if ( argc != 5 )
		{
			printf(" Wrong parameter number.\n");
			printf(" phyw mac addr reg data\n");
			printf("   mac     : 0 or 1.     [hex]\n");
			printf("   PHY addr: 0 to 0x1F.  [hex]\n");
			printf("   register: 0 to 0xFF.  [hex]\n");
			printf("   data    : 0 to 0xFFFF.[hex]\n");
			printf(" example: phyw 0 0 0 610\n");
			ret = -1;
			break;
		}

		MACnum   = strtoul(argv[1], NULL, 16);
		PHYaddr  = strtoul(argv[2], NULL, 16);
		PHYreg   = strtoul(argv[3], NULL, 16);
		reg_data = strtoul(argv[4], NULL, 16);

		if ( MACnum == 0 ) {
			// Set MAC 0
			eng->run.MAC_BASE  = MAC_BASE1;
			eng->run.MAC_idx_PHY  = 0;
		}
		else if ( MACnum == 1 ) {
			// Set MAC 1
			eng->run.MAC_BASE  = MAC_BASE2;
			eng->run.MAC_idx_PHY  = 1;
		}
		else {
			printf("wrong parameter (mac number)\n");
			ret = -1;
			break;
		}

		if ( ( PHYaddr < 0 ) || ( PHYaddr > 31 ) ) {
			printf("wrong parameter (PHY address)\n");
			ret = -1;
			break;
		}

		multi_pin_2_gpio_init( eng );
		phy_gpio_init( eng );
		eng->phy.Adr = PHYaddr;
		phy_gpio_write( eng, PHYreg, reg_data );
		printf("[GPIO]Write: PHY[%d] reg[0x%02X] = %04lx\n", PHYaddr, PHYreg, reg_data );
	} while ( 0 );

	return ret;
}

int do_phygdump (cmd_tbl_t * cmdtp, int flag, int argc, char *argv[])
{
	MAC_ENGINE	MACENG;
	MAC_ENGINE	*eng;
	int     MACnum;
	int     PHYreg;
	ULONG   result_data;
	int     ret = 0;
	int     PHYaddr;
	int     GPIO_RdDelay = 0;

	eng = &MACENG;
	do {
		if ( argc == 4 ) {
			GPIO_RdDelay = strtoul(argv[3], NULL, 16);
		} else if ( argc != 3 ) {
			printf(" Wrong parameter number.\n");
			printf(" phyd mac addr\n");
			printf("   mac     : 0 or 1.   [hex]\n");
			printf("   PHY addr: 0 to 0x1F.[hex]\n");
			printf(" example: phyd 0 0\n");
			ret = -1;
			break;
		}

		MACnum  = strtoul(argv[1], NULL, 16);
		PHYaddr = strtoul(argv[2], NULL, 16);

		if ( MACnum == 0 ) {
			// Set MAC 0
			eng->run.MAC_BASE = MAC_BASE1;
			eng->run.MAC_idx_PHY  = 0;
		}
		else if ( MACnum == 1 ) {
			// Set MAC 1
			eng->run.MAC_BASE = MAC_BASE2;
			eng->run.MAC_idx_PHY  = 1;            
		}
		else {
			printf("wrong parameter (mac number)\n");
			ret = -1;
			break;
		}

		if ( ( PHYaddr < 0 ) || ( PHYaddr > 31 ) ) {
			printf("wrong parameter (PHY address)\n");
			ret = -1;
			break;
		}

		multi_pin_2_gpio_init( eng );
		phy_gpio_init( eng );
		eng->GPIO.Dat_RdDelay = GPIO_RdDelay;
		eng->phy.Adr = PHYaddr;
		for ( PHYreg = 0; PHYreg < 32; PHYreg++ ) {
			result_data = phy_gpio_read( eng, PHYreg );
			switch ( PHYreg % 4 ) {
				case 0	: printf("%02d| %04lx ", PHYreg, result_data ); break;
				case 3	: printf("%04lx\n", result_data ); break;
 				default	: printf("%04lx ", result_data ); break;
			}
		}
	} while ( 0 );

	return ret;
}

int do_impread (cmd_tbl_t * cmdtp, int flag, int argc, char *argv[])
{
	MAC_ENGINE	MACENG;
	MAC_ENGINE	*eng;
	int     MACnum;
	ULONG   result_data;
	int     ret = 0;
	int     page;
	int     offset;
	int     GPIO_RdDelay = 0;

	eng = &MACENG;
	do {
		if ( argc == 5 ) {
			GPIO_RdDelay = strtoul(argv[4], NULL, 16);
		} else

		if ( argc != 4 ) {
			printf(" Wrong parameter number.\n");
			printf(" impr mac page offset\n");
			printf("   mac     : 0 or 1.   [hex]\n");
			printf("   page    : 0 to 0xFF.[hex]\n");
			printf("   offset  : 0 to 0xFF.[hex]\n");
			ret = -1;
			break;
		}

		MACnum  = strtoul(argv[1], NULL, 16);
		page    = strtoul(argv[2], NULL, 16) & 0xff;
		offset  = strtoul(argv[3], NULL, 16) & 0xff;

		if ( MACnum == 0 ) {
			// Set MAC 0
			eng->run.MAC_BASE = MAC_BASE1;
			eng->run.MAC_idx_PHY  = 0;
		}
		else if ( MACnum == 1 ) {
			// Set MAC 1
			eng->run.MAC_BASE = MAC_BASE2;
			eng->run.MAC_idx_PHY  = 1;
		}
		else {
			printf("wrong parameter (mac number)\n");
			ret = -1;
			break;
		}

		multi_pin_2_gpio_init( eng );
		phy_gpio_init( eng );
		eng->GPIO.Dat_RdDelay = GPIO_RdDelay;
		eng->phy.Adr = 30;

		BCMIMP_set_page ( eng, page );
		BCMIMP_reg_read ( eng, offset );
		printf("[IMP Rd][page %d:0x%02X] = %08lx %08lx\n", page, offset, eng->BCMIMP.rddata[1], eng->BCMIMP.rddata[0] );
	} while ( 0 );

	return ret;
}
int impwrite (MAC_ENGINE *eng, int MACnum, int page, int offset, int reg_data1, int reg_data0)
{
	BCMIMP_set_page ( eng, page );
	BCMIMP_reg_read ( eng, offset );
	printf("[IMP Wr][page %d:0x%02X] = %08lx %08lx", page, offset, eng->BCMIMP.rddata[1], eng->BCMIMP.rddata[0] );
	BCMIMP_reg_write( eng, offset, reg_data1, reg_data0 );
	BCMIMP_reg_read ( eng, offset );
	printf(" --> %08lx %08lx (%08lx %08lx)\n", eng->BCMIMP.rddata[1], eng->BCMIMP.rddata[0], reg_data1, reg_data0 );
}

int do_impwrite (cmd_tbl_t * cmdtp, int flag, int argc, char *argv[])
{
	MAC_ENGINE	MACENG;
	MAC_ENGINE	*eng;
	int     MACnum;
	ULONG   result_data;
	int     ret = 0;
	int     page;
	int     offset;
	ULONG   reg_data0;
	ULONG   reg_data1;
	int     GPIO_RdDelay = 0;

	eng = &MACENG;
	do {
		if ( argc == 7 ) {
			GPIO_RdDelay = strtoul(argv[6], NULL, 16);
		} else

		if ( argc != 6 ) {
			printf(" Wrong parameter number.\n");
			printf(" impw mac page offset dw1 dw0\n");
			printf("   mac     : 0 or 1.   [hex]\n");
			printf("   page    : 0 to 0xFF.[hex]\n");
			printf("   offset  : 0 to 0xFF.[hex]\n");
			printf("   dw1     : 0 to 0xFFFFFFFF.[hex]\n");
			printf("   dw0     : 0 to 0xFFFFFFFF.[hex]\n");
			ret = -1;
			break;
		}

		MACnum    = strtoul(argv[1], NULL, 16);
		page      = strtoul(argv[2], NULL, 16) & 0xff;
		offset    = strtoul(argv[3], NULL, 16) & 0xff;
		reg_data1 = strtoul(argv[4], NULL, 16);
		reg_data0 = strtoul(argv[5], NULL, 16);

		if ( MACnum == 0 ) {
			// Set MAC 0
			eng->run.MAC_BASE = MAC_BASE1;
			eng->run.MAC_idx_PHY  = 0;
		}
		else if ( MACnum == 1 ) {
			// Set MAC 1
			eng->run.MAC_BASE = MAC_BASE2;
			eng->run.MAC_idx_PHY  = 1;
		}
		else {
			printf("wrong parameter (mac number)\n");
			ret = -1;
			break;
		}

		multi_pin_2_gpio_init( eng );
		phy_gpio_init( eng );
		eng->GPIO.Dat_RdDelay = GPIO_RdDelay;
		eng->phy.Adr = 30;

		impwrite(eng, MACnum, page, offset, reg_data1, reg_data0);
	} while ( 0 );

	return ret;
}

int do_setbcm5396 (cmd_tbl_t * cmdtp, int flag, int argc, char *argv[])
{
	MAC_ENGINE	MACENG;
	MAC_ENGINE	*eng;
	int     MACnum;
	int     GPIO_RdDelay = 0;

	eng = &MACENG;

	MACnum = 1;

	if ( MACnum == 0 ) {
		// Set MAC 0
		eng->run.MAC_BASE = MAC_BASE1;
		eng->run.MAC_idx_PHY  = 0;
	}
	else {
		// Set MAC 1
		eng->run.MAC_BASE = MAC_BASE2;
		eng->run.MAC_idx_PHY  = 1;
	}

	multi_pin_2_gpio_init( eng );
	phy_gpio_init( eng );
	eng->GPIO.Dat_RdDelay = GPIO_RdDelay;
	eng->phy.Adr = 30;

	impwrite(eng, 1, 0x02, 0x00, 0x00000000, 0x00000082);
	impwrite(eng, 1, 0x00, 0x86, 0x00000000, 0x00000020);
	impwrite(eng, 1, 0x00, 0x70, 0x00000000, 0x0000008B);
	impwrite(eng, 1, 0x00, 0x10, 0x00000000, 0x0000001C);
	impwrite(eng, 1, 0x34, 0x08, 0x00000000, 0x00000002);
	impwrite(eng, 1, 0x10, 0x20, 0x00000000, 0x000001C1);
	impwrite(eng, 1, 0x11, 0x20, 0x00000000, 0x000001C1);
	impwrite(eng, 1, 0x12, 0x20, 0x00000000, 0x000001C1);
	impwrite(eng, 1, 0x13, 0x20, 0x00000000, 0x000001C1);
	impwrite(eng, 1, 0x14, 0x20, 0x00000000, 0x000001C1);
	impwrite(eng, 1, 0x15, 0x20, 0x00000000, 0x000001C1);
	impwrite(eng, 1, 0x16, 0x20, 0x00000000, 0x000001C1);
	impwrite(eng, 1, 0x17, 0x20, 0x00000000, 0x000001C1);
	impwrite(eng, 1, 0x18, 0x20, 0x00000000, 0x000001C1);
	impwrite(eng, 1, 0x19, 0x20, 0x00000000, 0x000001C1);
	impwrite(eng, 1, 0x1A, 0x20, 0x00000000, 0x000001C1);
	impwrite(eng, 1, 0x1B, 0x20, 0x00000000, 0x000001C1);
	impwrite(eng, 1, 0x1C, 0x20, 0x00000000, 0x000001C1);
	impwrite(eng, 1, 0x00, 0x60, 0x00000000, 0x0000008B);
	impwrite(eng, 1, 0x00, 0x61, 0x00000000, 0x0000008B);
	impwrite(eng, 1, 0x00, 0x62, 0x00000000, 0x0000008B);
	impwrite(eng, 1, 0x00, 0x63, 0x00000000, 0x0000008B);
	impwrite(eng, 1, 0x00, 0x64, 0x00000000, 0x0000008B);
	impwrite(eng, 1, 0x00, 0x65, 0x00000000, 0x0000008B);
	impwrite(eng, 1, 0x00, 0x66, 0x00000000, 0x0000008B);
	impwrite(eng, 1, 0x00, 0x67, 0x00000000, 0x0000008B);
	impwrite(eng, 1, 0x00, 0x68, 0x00000000, 0x0000008B);
	impwrite(eng, 1, 0x00, 0x69, 0x00000000, 0x0000008B);
	impwrite(eng, 1, 0x00, 0x6A, 0x00000000, 0x0000008B);
	impwrite(eng, 1, 0x00, 0x6B, 0x00000000, 0x0000008B);
	impwrite(eng, 1, 0x00, 0x6C, 0x00000000, 0x0000008B);
	impwrite(eng, 1, 0x00, 0x00, 0x00000000, 0x00000000);
	impwrite(eng, 1, 0x00, 0x01, 0x00000000, 0x00000000);
	impwrite(eng, 1, 0x00, 0x02, 0x00000000, 0x00000000);
	impwrite(eng, 1, 0x00, 0x03, 0x00000000, 0x00000000);
	impwrite(eng, 1, 0x00, 0x04, 0x00000000, 0x00000000);
	impwrite(eng, 1, 0x00, 0x05, 0x00000000, 0x00000000);
	impwrite(eng, 1, 0x00, 0x06, 0x00000000, 0x00000000);
	impwrite(eng, 1, 0x00, 0x07, 0x00000000, 0x00000000);
	impwrite(eng, 1, 0x00, 0x08, 0x00000000, 0x00000000);
	impwrite(eng, 1, 0x00, 0x09, 0x00000000, 0x00000000);
	impwrite(eng, 1, 0x00, 0x0A, 0x00000000, 0x00000000);
	impwrite(eng, 1, 0x00, 0x0B, 0x00000000, 0x00000000);
	impwrite(eng, 1, 0x00, 0x0C, 0x00000000, 0x00000000);
	impwrite(eng, 1, 0x00, 0x0D, 0x00000000, 0x00000000);
	impwrite(eng, 1, 0x00, 0x0E, 0x00000000, 0x00000000);
	impwrite(eng, 1, 0x00, 0x0F, 0x00000000, 0x00000000);
	impwrite(eng, 1, 0x00, 0x20, 0x00000000, 0x00000007);
}

U_BOOT_CMD(
	phygr,    NETESTCMD_MAX_ARGS, 0,  do_phygread,
	"Read PHY register.  (phyr mac addr reg)",
	NULL
);

U_BOOT_CMD(
	phygw,    NETESTCMD_MAX_ARGS, 0,  do_phygwrite,
	"Write PHY register. (phyw mac addr reg data)",
	NULL
);

U_BOOT_CMD(
	phygd,    NETESTCMD_MAX_ARGS, 0,  do_phygdump,
	"Dump PHY register. (phyd mac addr)",
	NULL
);

U_BOOT_CMD(
	impr,    NETESTCMD_MAX_ARGS, 0,  do_impread,
	"Read IMP Port register.  (impr mac page offset)",
	NULL
);
U_BOOT_CMD(
	impw,    NETESTCMD_MAX_ARGS, 0,  do_impwrite,
	"Write IMP Port register.  (impw mac page offset dw1 dw0)",
	NULL
);
U_BOOT_CMD(
	setbcm5396,    NETESTCMD_MAX_ARGS, 0,  do_setbcm5396,
	"Setting BCM5396 register.  (setbcm5396)",
	NULL
);
#endif // defined(PHY_GPIO)

#endif // End SLT_UBOOT

