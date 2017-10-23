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

#ifndef PHYSPECIAL_H
#define PHYSPECIAL_H

#define PHY_BCM5396_Debug_RWValue
#define PHY_BCM5389_Debug_RWValue
#define PHY_BCM5389_Debug_Counter


#define PHY_BCMIMP_TIME_OUT_RW                  100       //10
//#define PHY_BCMIMP_TIME_OUT_RW                  10       //10

#define PHY_BCMIMP_Pkt_Byte_Len                 1524      //1524/1514
#define PHY_BCMIMP_Pkt_DW_Len                    381
//#define PHY_BCMIMP_Pkt_Byte_Len                 120      //120/110   
//#define PHY_BCMIMP_Pkt_DW_Len                    30

typedef struct {
	ULONG                pkt_tx_len                              ;//__attribute__ ((aligned (4)));
	ULONG                pkt_rx[PHY_BCMIMP_Pkt_DW_Len]           ;//__attribute__ ((aligned (4)));
	ULONG                pkt_rx_len                              ;//__attribute__ ((aligned (4)));
	ULONG               *pkt_tx                                  ;//__attribute__ ((aligned (4)));
	char                 wait_init                               ;//__attribute__ ((aligned (4)));
	ULONG                rddata[2];

	char                 PHY_sel                                 ;//__attribute__ ((aligned (4)));
} PHY_BCMIMP;



#endif // PHYSPECIAL_H
