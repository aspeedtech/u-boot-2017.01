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
#include <console.h>
#include <bootretry.h>
#include <cli.h>
#include <command.h>
#include <console.h>

#include <inttypes.h>
#include <mapmem.h>
#include <asm/io.h>
#include <linux/compiler.h>

DECLARE_GLOBAL_DATA_PTR;

#define DRAM_MapAdr	81000000
#define TIMEOUT_DRAM	5000000

/* ------------------------------------------------------------------------- */
int MMCTestBurst(unsigned int datagen)
{
	unsigned int data;
	unsigned int timeout = 0;

	writel(0x00000000, 0x1E6E0070);
	writel((0x000000C1 | (datagen << 3)), 0x1E6E0070);
  
	do {
		data = readl(0x1E6E0070) & 0x3000;

		if( data & 0x2000 )
			return(0);

		if( ++timeout > TIMEOUT_DRAM ) {
			printf("Timeout!!\n");
			writel(0x00000000, 0x1E6E0070);
			return(0);
		} 
	} while (!data);

	writel(0x00000000, 0x1E6E0070);

	return(1);
}

/* ------------------------------------------------------------------------- */
int MMCTestSingle(unsigned int datagen)
{
	unsigned int data;
	unsigned int timeout = 0;

	writel(0x00000000, 0x1E6E0070);
	writel((0x00000085 | (datagen << 3)), 0x1E6E0070);

	do {
		data = readl(0x1E6E0070) & 0x3000;

		if( data & 0x2000 )
			return(0);

		if( ++timeout > TIMEOUT_DRAM ){
			printf("Timeout!!\n");
			writel(0x00000000, 0x1E6E0070);

			return(0);
		}
	} while ( !data );
	
	writel(0x00000000, 0x1E6E0070);

	return(1);
}

/* ------------------------------------------------------------------------- */
int MMCTest(void)
{
	unsigned int pattern;

	pattern = readl( 0x1E6E2078 );
	printf("Pattern = %08X : ",pattern);

	writel((DRAM_MapAdr | 0x7fffff), 0x1E6E0074);
	writel(pattern, 0x1E6E007C);
  
	if(!MMCTestBurst(0))    return(0);
	if(!MMCTestBurst(1))    return(0);
	if(!MMCTestBurst(2))    return(0);
	if(!MMCTestBurst(3))    return(0);
	if(!MMCTestBurst(4))    return(0);
	if(!MMCTestBurst(5))    return(0);
	if(!MMCTestBurst(6))    return(0);
	if(!MMCTestBurst(7))    return(0);
	if(!MMCTestSingle(0))   return(0);
	if(!MMCTestSingle(1))   return(0);
	if(!MMCTestSingle(2))   return(0);
	if(!MMCTestSingle(3))   return(0);
	if(!MMCTestSingle(4))   return(0);
	if(!MMCTestSingle(5))   return(0);
	if(!MMCTestSingle(6))   return(0);
	if(!MMCTestSingle(7))   return(0);

	return(1);
}

/* ------------------------------------------------------------------------- */
static int do_ast_dramtest(cmd_tbl_t *cmdtp, int flag, int argc,
			char * const argv[])
{
	unsigned int PassCnt     = 0;
	ulong Testcounter = 0;
	int ret = 1;

	printf("**************************************************** \n");       
	printf("*** ASPEED Stress DRAM                           *** \n");
	printf("***                          20131107 for u-boot *** \n");
	printf("**************************************************** \n"); 
	printf("\n"); 

	if ( argc != 2 ){
		ret = 0;
		return ( ret );
	} else {
		if (strict_strtoul(argv[1], 10, &Testcounter) < 0)
			return CMD_RET_USAGE;
	}

	writel(0xFC600309, 0x1E6E0000);

	while( ( Testcounter > PassCnt ) || ( Testcounter == 0 ) ){
		if( !MMCTest() ) {
			printf("FAIL...%d/%ld\n", PassCnt, Testcounter);
			ret = 0;

			break;
		} else {
			PassCnt++;
			printf("Pass %d/%ld\n", PassCnt, Testcounter);
		}
	} // End while()

	return( ret );
}


U_BOOT_CMD(
	dramtest,   5, 0,  do_ast_dramtest,
	"ASPEED dramtest- Stress DRAM",
	""
);
