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

#define PHYGPIO_C
static const char ThisFile[] = "PHYGPIO.c";

#include "SWFUNC.H"
#include "COMMINF.H"

#if defined(SLT_UBOOT)
  #include <common.h>
  #include <command.h>
  #include "STDUBOOT.H"
#endif
#if defined(DOS_ALONE)
  #include <stdio.h>
  #include <stdlib.h>
  #include <conio.h>
  #include <string.h>
#endif

#include "COMMINF.H"

#if defined(PHY_GPIO)
//------------------------------------------------------------
// PHY Read/Write Using GPIO
//------------------------------------------------------------
//          AST2500  AST2400  AST2300  AST1010
// MDC1  :  GPIOR6   GPIOR6   GPIOR6   GPIOK6
// MDIO1 :  GPIOR7   GPIOR7   GPIOR7   GPIOK7
// MDC2  :  GPIOA6   GPIOA6   GPIOA6   ------
// MDIO2 :  GPIOA7   GPIOA7   GPIOA7   ------
//------------------------------------------------------------

//------------------------------------------------------------
// Initial
//------------------------------------------------------------
void phy_gpio_init( MAC_ENGINE *eng )
{
#ifdef  DbgPrn_FuncHeader
	printf("phy_gpio_init\n");
	Debug_delay();
#endif
#ifndef AST1010_IOMAP
	switch ( eng->run.MAC_idx_PHY ) {
		case 0  :
			eng->GPIO.IOAdr_Dat      = 0x00000080;
			eng->GPIO.IOAdr_OE       = 0x00000084;
			eng->GPIO.MDIO_shiftbit  = 15;
			eng->GPIO.Mask_MDC       = 0x00004000;
			eng->GPIO.Mask_MDIO      = 0x00008000;
			break;
		case 1  :
			eng->GPIO.IOAdr_Dat      = 0x00000000;
			eng->GPIO.IOAdr_OE       = 0x00000004;
			eng->GPIO.MDIO_shiftbit  = 7;
			eng->GPIO.Mask_MDC       = 0x00000040;
			eng->GPIO.Mask_MDIO      = 0x00000080;
			break;
		default : break;
	}
#else
	eng->GPIO.IOAdr_Dat      = 0x00000070;
	eng->GPIO.IOAdr_OE       = 0x00000074;
	eng->GPIO.MDIO_shiftbit  = 23;
	eng->GPIO.Mask_MDC       = 0x00400000;
	eng->GPIO.Mask_MDIO      = 0x00800000;
#endif
	eng->GPIO.Mask_all  = eng->GPIO.Mask_MDC | eng->GPIO.Mask_MDIO;

	eng->GPIO.Value_Dat =  Read_Reg_GPIO_DD( eng->GPIO.IOAdr_Dat ) & (~eng->GPIO.Mask_all);
	eng->GPIO.Value_OE  = (Read_Reg_GPIO_DD( eng->GPIO.IOAdr_OE  ) & (~eng->GPIO.Mask_MDIO)) | eng->GPIO.Mask_MDC;
	Write_Reg_GPIO_DD( eng->GPIO.IOAdr_Dat, eng->GPIO.Value_Dat | eng->GPIO.Mask_all );
	Write_Reg_GPIO_DD( eng->GPIO.IOAdr_OE , eng->GPIO.Value_OE                       );
	
	eng->GPIO.Dat_RdDelay = 0;
}

//------------------------------------------------------------
// PHY R/W GPIO
//------------------------------------------------------------
void phy_gpio_wrbit ( MAC_ENGINE *eng, int value )
{
	if ( value & 0x8000 )
	{
		Write_Reg_GPIO_DD( eng->GPIO.IOAdr_Dat, eng->GPIO.Value_Dat | eng->GPIO.Mask_MDIO );                 
		PHY_GPIO_DELAY( PHY_GPIO_MDC_HalfPeriod );

		Write_Reg_GPIO_DD( eng->GPIO.IOAdr_Dat, eng->GPIO.Value_Dat | eng->GPIO.Mask_all  );
		PHY_GPIO_DELAY( PHY_GPIO_MDC_HalfPeriod );
	} else {
		Write_Reg_GPIO_DD( eng->GPIO.IOAdr_Dat, eng->GPIO.Value_Dat                       );
		PHY_GPIO_DELAY( PHY_GPIO_MDC_HalfPeriod );

		Write_Reg_GPIO_DD( eng->GPIO.IOAdr_Dat, eng->GPIO.Value_Dat | eng->GPIO.Mask_MDC  );
		PHY_GPIO_DELAY( PHY_GPIO_MDC_HalfPeriod );
	}
}

char phy_gpio_rdbit ( MAC_ENGINE *eng )
{
	static char read_bit;
	Write_Reg_GPIO_DD( eng->GPIO.IOAdr_Dat, eng->GPIO.Value_Dat | eng->GPIO.Mask_MDIO );
	PHY_GPIO_DELAY( PHY_GPIO_MDC_HalfPeriod );

	read_bit = ( Read_Reg_GPIO_DD( eng->GPIO.IOAdr_Dat ) >> eng->GPIO.MDIO_shiftbit ) & 0x1;
	Write_Reg_GPIO_DD( eng->GPIO.IOAdr_Dat, eng->GPIO.Value_Dat | eng->GPIO.Mask_all  );
	PHY_GPIO_DELAY( PHY_GPIO_MDC_HalfPeriod );

//	return ( ( Read_Reg_GPIO_DD( eng->GPIO.IOAdr_Dat ) >> eng->GPIO.MDIO_shiftbit ) & 0x1 );
	return ( read_bit );
}

void phy_gpio_write( MAC_ENGINE *eng, int regadr, int wrdata )
{
	static int phy_wr_cnt;
	static int phy_wr_wrvalue;

//#ifdef  DbgPrn_FuncHeader
//	printf("phy_gpio_write\n");
//	Debug_delay();
//#endif
#ifdef PHY_GPIO_ReadValueEveryTime
	eng->GPIO.Value_Dat = Read_Reg_GPIO_DD( eng->GPIO.IOAdr_Dat ) & (~eng->GPIO.Mask_all);
	eng->GPIO.Value_OE  = Read_Reg_GPIO_DD( eng->GPIO.IOAdr_OE  );
#endif
	Write_Reg_GPIO_DD( eng->GPIO.IOAdr_OE, eng->GPIO.Value_OE | eng->GPIO.Mask_MDIO );

	for ( phy_wr_cnt = 0; phy_wr_cnt < 36; phy_wr_cnt++ )
		phy_gpio_wrbit ( eng, 0x8000 );

	phy_wr_wrvalue = 0x5002 | (( eng->phy.Adr & 0x1f ) << 7 ) | (( regadr & 0x1f ) << 2 );
	for ( phy_wr_cnt = 0; phy_wr_cnt < 16; phy_wr_cnt++ )
	{
		phy_gpio_wrbit ( eng, phy_wr_wrvalue );
		phy_wr_wrvalue = phy_wr_wrvalue << 1;
	}

	phy_wr_wrvalue = wrdata;
	for ( phy_wr_cnt = 0; phy_wr_cnt < 16; phy_wr_cnt++ )
	{
		phy_gpio_wrbit ( eng, phy_wr_wrvalue );
		phy_wr_wrvalue = phy_wr_wrvalue << 1;
	}

	Write_Reg_GPIO_DD( eng->GPIO.IOAdr_OE, eng->GPIO.Value_OE );
	phy_gpio_wrbit ( eng, 1 );
	phy_gpio_wrbit ( eng, 1 );
}

ULONG phy_gpio_read( MAC_ENGINE *eng, int regadr )
{
	static int phy_rd_cnt;
	static int phy_rd_wrvalue;
	static int phy_rd_rdvalue;

//#ifdef  DbgPrn_FuncHeader
//	printf("phy_gpio_read\n");
//	Debug_delay();
//#endif
#ifdef PHY_GPIO_ReadValueEveryTime
	eng->GPIO.Value_Dat = Read_Reg_GPIO_DD( eng->GPIO.IOAdr_Dat ) & (~eng->GPIO.Mask_all);
	eng->GPIO.Value_OE  = Read_Reg_GPIO_DD( eng->GPIO.IOAdr_OE  );
#endif
	Write_Reg_GPIO_DD( eng->GPIO.IOAdr_OE, eng->GPIO.Value_OE | eng->GPIO.Mask_MDIO );

	for ( phy_rd_cnt = 0; phy_rd_cnt < 36; phy_rd_cnt++ )
		phy_gpio_wrbit ( eng, 0x8000 );

	phy_rd_wrvalue = 0x6000 | (( eng->phy.Adr & 0x1f ) << 7 ) | (( regadr & 0x1f ) << 2 );
	for ( phy_rd_cnt = 0; phy_rd_cnt < 14; phy_rd_cnt++ )
	{
		phy_gpio_wrbit ( eng, phy_rd_wrvalue );
		phy_rd_wrvalue = phy_rd_wrvalue << 1;
	}

	Write_Reg_GPIO_DD( eng->GPIO.IOAdr_OE, eng->GPIO.Value_OE );
	phy_gpio_rdbit( eng );
	phy_gpio_rdbit(eng );

	for (phy_rd_cnt = 0; phy_rd_cnt < eng->GPIO.Dat_RdDelay; phy_rd_cnt++)
		phy_gpio_rdbit( eng );

	phy_rd_rdvalue = phy_gpio_rdbit( eng );
	for (phy_rd_cnt = 0; phy_rd_cnt < 15; phy_rd_cnt++) {
		phy_rd_rdvalue = ( phy_rd_rdvalue << 1 ) | phy_gpio_rdbit( eng );
	}

	phy_gpio_rdbit( eng );
	phy_gpio_rdbit( eng );
	phy_gpio_rdbit( eng );
	phy_gpio_rdbit( eng );

	return ( phy_rd_rdvalue );
}
#endif // defined(PHY_GPIO)
