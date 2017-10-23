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

#define NCSI_C
static const char ThisFile[] = "NCSI.c";

#include "SWFUNC.H"

#if defined(SLT_UBOOT)
  #include <common.h>
  #include <command.h>
  #include "COMMINF.H"
  #include "NCSI.H"
  #include "IO.H"
#endif
#if defined(DOS_ALONE)
  #include <stdio.h>
  #include <stdlib.h>
  #include <conio.h>
  #include <string.h>
  #include "COMMINF.H"
  #include "NCSI.H"
  #include "IO.H"
#endif
#if defined(LinuxAP)
  #include <stdio.h>
  #include <string.h>
  #include "COMMINF.H"
  #include "NCSI.H"
  #include "IO.H"
#endif
#if defined(SLT_NEW_ARCH)
  #include <string.h>
  #include "COMMINF.H"
  #include "NCSI.H"
  #include "IO.H"
#endif


//------------------------------------------------------------
int FindErr_NCSI (MAC_ENGINE *eng, int value) {
	eng->flg.NCSI_Flag = eng->flg.NCSI_Flag | value;
	eng->flg.Err_Flag  = eng->flg.Err_Flag | Err_Flag_NCSI_LinkFail;
	if ( DbgPrn_ErrFlg )
		printf("\nErr_Flag: [%08lx] NCSI_Flag: [%08lx]\n", eng->flg.Err_Flag, eng->flg.NCSI_Flag);

	return(1);
}

//------------------------------------------------------------
// PHY IC(NC-SI)
//------------------------------------------------------------
void ncsi_reqdump (MAC_ENGINE *eng, NCSI_Command_Packet *in) {
	int     i;
	PRINTF( FP_LOG, "[NCSI-Request] DA             : %02x %02x %02x %02x %02x %02x\n", in->DA[ 0 ], in->DA[ 1 ], in->DA[ 2 ], in->DA[ 3 ], in->DA[ 4 ] , in->DA[ 5 ]);
	PRINTF( FP_LOG, "[NCSI-Request] SA             : %02x %02x %02x %02x %02x %02x\n", in->SA[ 0 ], in->SA[ 1 ], in->SA[ 2 ], in->SA[ 3 ], in->SA[ 4 ] , in->SA[ 5 ]);
	PRINTF( FP_LOG, "[NCSI-Request] EtherType      : %04x\n", SWAP_2B_BEDN( in->EtherType )             );//DMTF NC-SI
	PRINTF( FP_LOG, "[NCSI-Request] MC_ID          : %02x\n", in->MC_ID                                 );//Management Controller should set this field to 0x00
	PRINTF( FP_LOG, "[NCSI-Request] Header_Revision: %02x\n", in->Header_Revision                       );//For NC-SI 1.0 spec, this field has to set 0x01
//	PRINTF( FP_LOG, "[NCSI-Request] Reserved_1     : %02x\n", in->Reserved_1                            ); //Reserved has to set to 0x00
	PRINTF( FP_LOG, "[NCSI-Request] IID            : %02x\n", in->IID                                   );//Instance ID
	PRINTF( FP_LOG, "[NCSI-Request] Command        : %02x\n", in->Command                               );
	PRINTF( FP_LOG, "[NCSI-Request] ChID           : %02x\n", in->ChID                                  );
	PRINTF( FP_LOG, "[NCSI-Request] Payload_Length : %04x\n", SWAP_2B_BEDN( in->Payload_Length )        );//Payload Length = 12 bits, 4 bits are reserved
//	PRINTF( FP_LOG, "[NCSI-Request] Reserved_2     : %04x\n", in->Reserved_2                            );
//	PRINTF( FP_LOG, "[NCSI-Request] Reserved_3     : %04x\n", in->Reserved_3                            );
//	PRINTF( FP_LOG, "[NCSI-Request] Reserved_4     : %04x\n", in->Reserved_4                            );
//	PRINTF( FP_LOG, "[NCSI-Request] Reserved_5     : %04x\n", in->Reserved_5                            );
	PRINTF( FP_LOG, "[NCSI-Request] Response_Code  : %04x\n", SWAP_2B_BEDN( in->Response_Code )         );
	PRINTF( FP_LOG, "[NCSI-Request] Reason_Code    : %04x\n", SWAP_2B_BEDN( in->Reason_Code )           );
	for ( i = 0; i < SWAP_2B_BEDN( in->Payload_Length ); i++ ) {
		switch ( i % 4 ) {
			case 0	: PRINTF( FP_LOG, "[NCSI-Request] Payload_Data   : %02x", in->Payload_Data[ i ]); break;
			case 3	: PRINTF( FP_LOG, " %02x\n", in->Payload_Data[ i ]); break;
			default	: PRINTF( FP_LOG, " %02x", in->Payload_Data[ i ]); break;
		}
	}
	if ( ( i % 4 ) != 3 )
		PRINTF( FP_LOG, "\n");
}
void ncsi_respdump (MAC_ENGINE *eng, NCSI_Response_Packet *in) {
	int     i;
//	PRINTF( FP_LOG, "[NCSI-Respond] DA             : %02x %02x %02x %02x %02x %02x\n", in->DA[ 5 ], in->DA[ 4 ], in->DA[ 3 ], in->DA[ 2 ], in->DA[ 1] , in->DA[ 0 ]);
//	PRINTF( FP_LOG, "[NCSI-Respond] SA             : %02x %02x %02x %02x %02x %02x\n", in->SA[ 5 ], in->SA[ 4 ], in->SA[ 3 ], in->SA[ 2 ], in->SA[ 1] , in->SA[ 0 ]);
	PRINTF( FP_LOG, "[NCSI-Respond] DA             : %02x %02x %02x %02x %02x %02x\n", in->DA[ 0 ], in->DA[ 1 ], in->DA[ 2 ], in->DA[ 3 ], in->DA[ 4 ] , in->DA[ 5 ]);
	PRINTF( FP_LOG, "[NCSI-Respond] SA             : %02x %02x %02x %02x %02x %02x\n", in->SA[ 0 ], in->SA[ 1 ], in->SA[ 2 ], in->SA[ 3 ], in->SA[ 4 ] , in->SA[ 5 ]);
	PRINTF( FP_LOG, "[NCSI-Respond] EtherType      : %04x\n", SWAP_2B_BEDN( in->EtherType )             );//DMTF NC-SI
	PRINTF( FP_LOG, "[NCSI-Respond] MC_ID          : %02x\n", in->MC_ID                                 );//Management Controller should set this field to 0x00
	PRINTF( FP_LOG, "[NCSI-Respond] Header_Revision: %02x\n", in->Header_Revision                       );//For NC-SI 1.0 spec, this field has to set 0x01
//	PRINTF( FP_LOG, "[NCSI-Respond] Reserved_1     : %02x\n", in->Reserved_1                            ); //Reserved has to set to 0x00
	PRINTF( FP_LOG, "[NCSI-Respond] IID            : %02x\n", in->IID                                   );//Instance ID
	PRINTF( FP_LOG, "[NCSI-Respond] Command        : %02x\n", in->Command                               );
	PRINTF( FP_LOG, "[NCSI-Respond] ChID           : %02x\n", in->ChID                                  );
	PRINTF( FP_LOG, "[NCSI-Respond] Payload_Length : %04x\n", SWAP_2B_BEDN( in->Payload_Length )        );//Payload Length = 12 bits, 4 bits are reserved
//	PRINTF( FP_LOG, "[NCSI-Respond] Reserved_2     : %04x\n", in->Reserved_2                            );
//	PRINTF( FP_LOG, "[NCSI-Respond] Reserved_3     : %04x\n", in->Reserved_3                            );
//	PRINTF( FP_LOG, "[NCSI-Respond] Reserved_4     : %04x\n", in->Reserved_4                            );
//	PRINTF( FP_LOG, "[NCSI-Respond] Reserved_5     : %04x\n", in->Reserved_5                            );
	PRINTF( FP_LOG, "[NCSI-Respond] Response_Code  : %04x\n", SWAP_2B_BEDN( in->Response_Code )         );
	PRINTF( FP_LOG, "[NCSI-Respond] Reason_Code    : %04x\n", SWAP_2B_BEDN( in->Reason_Code )           );
	for ( i = 0; i < SWAP_2B_BEDN( in->Payload_Length ); i++ ) {
		switch ( i % 4 ) {
			case 0	: PRINTF( FP_LOG, "[NCSI-Respond] Payload_Data   : %02x", in->Payload_Data[ i ]); break;
			case 3	: PRINTF( FP_LOG, " %02x\n", in->Payload_Data[ i ]); break;
			default	: PRINTF( FP_LOG, " %02x", in->Payload_Data[ i ]); break;
		}
	}
	if ( ( i % 4 ) != 3 )
		PRINTF( FP_LOG, "\n");
}

//------------------------------------------------------------
void NCSI_PrintCommandStr (MAC_ENGINE *eng, unsigned char command, unsigned iid) {
	switch ( command & 0x80 ) {
		case 0x80   : sprintf(eng->dat.NCSI_CommandStr, "IID:%3d [%02x:Respond]", iid, command); break;
		default     : sprintf(eng->dat.NCSI_CommandStr, "IID:%3d [%02x:Request]", iid, command); break;
	}
	switch ( command & 0x7f ) {
		case 0x00   : sprintf(eng->dat.NCSI_CommandStr, "%s[CLEAR_INITIAL_STATE                ]", eng->dat.NCSI_CommandStr); break;
		case 0x01   : sprintf(eng->dat.NCSI_CommandStr, "%s[SELECT_PACKAGE                     ]", eng->dat.NCSI_CommandStr); break;
		case 0x02   : sprintf(eng->dat.NCSI_CommandStr, "%s[DESELECT_PACKAGE                   ]", eng->dat.NCSI_CommandStr); break;
		case 0x03   : sprintf(eng->dat.NCSI_CommandStr, "%s[ENABLE_CHANNEL                     ]", eng->dat.NCSI_CommandStr); break;
		case 0x04   : sprintf(eng->dat.NCSI_CommandStr, "%s[DISABLE_CHANNEL                    ]", eng->dat.NCSI_CommandStr); break;
		case 0x05   : sprintf(eng->dat.NCSI_CommandStr, "%s[RESET_CHANNEL                      ]", eng->dat.NCSI_CommandStr); break;
		case 0x06   : sprintf(eng->dat.NCSI_CommandStr, "%s[ENABLE_CHANNEL_NETWORK_TX          ]", eng->dat.NCSI_CommandStr); break;
		case 0x07   : sprintf(eng->dat.NCSI_CommandStr, "%s[DISABLE_CHANNEL_NETWORK_TX         ]", eng->dat.NCSI_CommandStr); break;
		case 0x08   : sprintf(eng->dat.NCSI_CommandStr, "%s[AEN_ENABLE                         ]", eng->dat.NCSI_CommandStr); break;
		case 0x09   : sprintf(eng->dat.NCSI_CommandStr, "%s[SET_LINK                           ]", eng->dat.NCSI_CommandStr); break;
		case 0x0A   : sprintf(eng->dat.NCSI_CommandStr, "%s[GET_LINK_STATUS                    ]", eng->dat.NCSI_CommandStr); break;
		case 0x0B   : sprintf(eng->dat.NCSI_CommandStr, "%s[SET_VLAN_FILTER                    ]", eng->dat.NCSI_CommandStr); break;
		case 0x0C   : sprintf(eng->dat.NCSI_CommandStr, "%s[ENABLE_VLAN                        ]", eng->dat.NCSI_CommandStr); break;
		case 0x0D   : sprintf(eng->dat.NCSI_CommandStr, "%s[DISABLE_VLAN                       ]", eng->dat.NCSI_CommandStr); break;
		case 0x0E   : sprintf(eng->dat.NCSI_CommandStr, "%s[SET_MAC_ADDRESS                    ]", eng->dat.NCSI_CommandStr); break;
		case 0x10   : sprintf(eng->dat.NCSI_CommandStr, "%s[ENABLE_BROADCAST_FILTERING         ]", eng->dat.NCSI_CommandStr); break;
		case 0x11   : sprintf(eng->dat.NCSI_CommandStr, "%s[DISABLE_BROADCAST_FILTERING        ]", eng->dat.NCSI_CommandStr); break;
		case 0x12   : sprintf(eng->dat.NCSI_CommandStr, "%s[ENABLE_GLOBAL_MULTICAST_FILTERING  ]", eng->dat.NCSI_CommandStr); break;
		case 0x13   : sprintf(eng->dat.NCSI_CommandStr, "%s[DISABLE_GLOBAL_MULTICAST_FILTERING ]", eng->dat.NCSI_CommandStr); break;
		case 0x14   : sprintf(eng->dat.NCSI_CommandStr, "%s[SET_NCSI_FLOW_CONTROL              ]", eng->dat.NCSI_CommandStr); break;
		case 0x15   : sprintf(eng->dat.NCSI_CommandStr, "%s[GET_VERSION_ID                     ]", eng->dat.NCSI_CommandStr); break;
		case 0x16   : sprintf(eng->dat.NCSI_CommandStr, "%s[GET_CAPABILITIES                   ]", eng->dat.NCSI_CommandStr); break;
		case 0x17   : sprintf(eng->dat.NCSI_CommandStr, "%s[GET_PARAMETERS                     ]", eng->dat.NCSI_CommandStr); break;
		case 0x18   : sprintf(eng->dat.NCSI_CommandStr, "%s[GET_CONTROLLER_PACKET_STATISTICS   ]", eng->dat.NCSI_CommandStr); break;
		case 0x19   : sprintf(eng->dat.NCSI_CommandStr, "%s[GET_NCSI_STATISTICS                ]", eng->dat.NCSI_CommandStr); break;
		case 0x1A   : sprintf(eng->dat.NCSI_CommandStr, "%s[GET_NCSI_PASS_THROUGH_STATISTICS   ]", eng->dat.NCSI_CommandStr); break;
		case 0x50   : sprintf(eng->dat.NCSI_CommandStr, "%s[OEM_COMMAND                        ]", eng->dat.NCSI_CommandStr); break;
		default     : sprintf(eng->dat.NCSI_CommandStr, "%s Not Support Command", eng->dat.NCSI_CommandStr); break ;
	}
} // End void NCSI_PrintCommandStr (MAC_ENGINE *eng, unsigned char command, unsigned iid)

//------------------------------------------------------------
void NCSI_PrintCommandType (MAC_ENGINE *eng, unsigned char command, unsigned iid) {
	NCSI_PrintCommandStr( eng, command, iid );
	printf("[NCSI-commd]%s\n", eng->dat.NCSI_CommandStr);
}

//------------------------------------------------------------
void NCSI_PrintCommandType2File (MAC_ENGINE *eng, unsigned char command, unsigned iid) {
	NCSI_PrintCommandStr( eng, command, iid );
	PRINTF( FP_LOG, "[NCSI-commd]%s\n", eng->dat.NCSI_CommandStr );
}

//------------------------------------------------------------
void NCSI_Struct_Initialize_SLT (MAC_ENGINE *eng) {
	int        i;
	ULONG      NCSI_RxDatBase;

	eng->run.NCSI_RxTimeOutScale = 1;

	for (i = 0; i < 6; i++) {
		eng->ncsi_req.DA[ i ] = 0xFF;
		eng->ncsi_req.SA[ i ] = eng->inf.SA[ i ];
	}
	eng->ncsi_req.EtherType       = SWAP_2B_BEDN( 0x88F8 ); // EtherType = 0x88F8 (DMTF NC-SI) page 50, table 8, NC-SI spec. version 1.0.0

	eng->ncsi_req.MC_ID           = 0;
	eng->ncsi_req.Header_Revision = 0x01;
	eng->ncsi_req.Reserved_1      = 0;
	eng->ncsi_req.IID             = 0;
//	eng->ncsi_req.Command         = 0;
//	eng->ncsi_req.ChID            = 0;
//	eng->ncsi_req.Payload_Length  = 0;
	eng->ncsi_req.Reserved_2      = 0;
	eng->ncsi_req.Reserved_3      = 0;

	eng->dat.NCSI_TxByteBUF = (unsigned char *) &eng->dat.NCSI_TxDWBUF[0];
	eng->dat.NCSI_RxByteBUF = (unsigned char *) &eng->dat.NCSI_RxDWBUF[0];

	eng->run.NCSI_TxDesBase = eng->run.TDES_BASE;//base for read/write
	Write_Mem_Des_NCSI_DD( eng->run.NCSI_TxDesBase + 0x04, 0                        );
	Write_Mem_Des_NCSI_DD( eng->run.NCSI_TxDesBase + 0x08, 0                        );
	Write_Mem_Des_NCSI_DD( eng->run.NCSI_TxDesBase + 0x0C, AT_MEMRW_BUF( DMA_BASE ) );

	eng->run.NCSI_RxDesBase = eng->run.RDES_BASE;//base for read/write
	NCSI_RxDatBase = AT_MEMRW_BUF( NCSI_RxDMA_BASE );//base of the descriptor

	for (i = 0; i < NCSI_RxDESNum - 1; i++) {
		Write_Mem_Des_NCSI_DD( ( eng->run.NCSI_RxDesBase        ), 0x00000000     );
		Write_Mem_Des_NCSI_DD( ( eng->run.NCSI_RxDesBase + 0x04 ), 0x00000000     );
		Write_Mem_Des_NCSI_DD( ( eng->run.NCSI_RxDesBase + 0x08 ), 0x00000000     );
		Write_Mem_Des_NCSI_DD( ( eng->run.NCSI_RxDesBase + 0x0C ), NCSI_RxDatBase );
		eng->run.NCSI_RxDesBase += 16;
		NCSI_RxDatBase += NCSI_RxDMA_PakSize;
	}
	Write_Mem_Des_NCSI_DD( ( eng->run.NCSI_RxDesBase        ), EOR_IniVal     );
	Write_Mem_Des_NCSI_DD( ( eng->run.NCSI_RxDesBase + 0x04 ), 0x00000000     );
	Write_Mem_Des_NCSI_DD( ( eng->run.NCSI_RxDesBase + 0x08 ), 0x00000000     );
//	Write_Mem_Des_NCSI_DD( ( eng->run.NCSI_RxDesBase + 0x0C ), (NCSI_RxDatBase + CPU_BUS_ADDR_SDRAM_OFFSET) ); // 20130730
	Write_Mem_Des_NCSI_DD( ( eng->run.NCSI_RxDesBase + 0x0C ), NCSI_RxDatBase ); // 20130730

	eng->run.NCSI_RxDesBase = eng->run.RDES_BASE;//base for read/write
}

//------------------------------------------------------------
void Calculate_Checksum_NCSI (MAC_ENGINE *eng, unsigned char *buffer_base, int Length) {
	ULONG      CheckSum = 0;
	ULONG      Data;
	ULONG      Data1;
	int        i;

	// Calculate checksum is from byte 14 of ethernet Haeder and Control packet header
	// Page 50, NC-SI spec. ver. 1.0.0 form DMTF
	for (i = 14; i < Length; i += 2 ) {
		Data      = buffer_base[i];
		Data1     = buffer_base[i + 1];
		CheckSum += ((Data << 8) + Data1);
	}
	eng->dat.Payload_Checksum_NCSI = SWAP_4B_BEDN(~(CheckSum) + 1); //2's complement
}

//------------------------------------------------------------
// return 0: it is PASS
// return 1: it is FAIL
//------------------------------------------------------------
char NCSI_Rx_SLT (MAC_ENGINE *eng) {
	int        timeout = 0;
	int        bytesize;
	int        dwsize;
	int        i;
	int        retry   = 0;
	char       ret     = 1;

	ULONG      NCSI_RxDatBase;
	ULONG      NCSI_RxDesDat;
	ULONG      NCSI_RxData;
	ULONG      NCSI_BufData;

	do {
		Write_Reg_MAC_DD( eng, 0x1C, 0x00000000 );//Rx Poll

		timeout = 0;
		do {
			NCSI_RxDesDat = Read_Mem_Des_NCSI_DD( eng->run.NCSI_RxDesBase );
			if ( ++timeout > TIME_OUT_NCSI * eng->run.NCSI_RxTimeOutScale ) {
				PRINTF( FP_LOG, "[RxDes] DesOwn timeout     %08lx\n", NCSI_RxDesDat );
				return( FindErr( eng, Err_Flag_NCSI_Check_RxOwnTimeOut ) );
			}
		} while( HWOwnRx( NCSI_RxDesDat ) );

		if ( NCSI_RxDesDat & Check_ErrMask_ALL ) {
#ifdef CheckRxErr
			if ( NCSI_RxDesDat & Check_ErrMask_RxErr ) {
				PRINTF( FP_LOG, "[RxDes] Error RxErr        %08lx\n", NCSI_RxDesDat );
				eng->dat.NCSI_RxEr = 1;
//				FindErr_Des( eng, Des_Flag_RxErr );
			}
#endif // End CheckRxErr

#ifdef CheckCRC
			if ( NCSI_RxDesDat & Check_ErrMask_CRC ) {
				PRINTF( FP_LOG, "[RxDes] Error CRC          %08lx\n", NCSI_RxDesDat );
				FindErr_Des( eng, Des_Flag_CRC );
			}
#endif // End CheckCRC

#ifdef CheckFTL
			if ( NCSI_RxDesDat & Check_ErrMask_FTL ) {
				PRINTF( FP_LOG, "[RxDes] Error FTL          %08lx\n", NCSI_RxDesDat );
				FindErr_Des( eng, Des_Flag_FTL );
			}
#endif // End CheckFTL

#ifdef CheckRunt
			if ( NCSI_RxDesDat & Check_ErrMask_Runt ) {
				PRINTF( FP_LOG, "[RxDes] Error Runt         %08lx\n", NCSI_RxDesDat );
				FindErr_Des( eng, Des_Flag_Runt );
			}
#endif // End CheckRunt

#ifdef CheckOddNibble
			if ( NCSI_RxDesDat & Check_ErrMask_OddNibble ) {
				PRINTF( FP_LOG, "[RxDes] Odd Nibble         %08lx\n", NCSI_RxDesDat );
				FindErr_Des( eng, Des_Flag_OddNibble );
			}
#endif // End CheckOddNibble

#ifdef CheckRxFIFOFull
			if ( NCSI_RxDesDat & Check_ErrMask_RxFIFOFull ) {
				PRINTF( FP_LOG, "[RxDes] Error Rx FIFO Full %08lx\n", NCSI_RxDesDat );
				FindErr_Des( eng, Des_Flag_RxFIFOFull );
			}
#endif // End CheckRxFIFOFull
		}

		// Get point of RX DMA buffer
		NCSI_RxDatBase = AT_BUF_MEMRW( Read_Mem_Des_NCSI_DD( eng->run.NCSI_RxDesBase + 0x0C ) );//base for read/write
		NCSI_RxData    = SWAP_4B_LEDN_NCSI( SWAP_4B_LEDN( Read_Mem_Dat_NCSI_DD( NCSI_RxDatBase + 0x0C ) ) );

		// Get RX valid data in offset 00h of RXDS#0
#ifdef NCSI_Skip_RxCRCData
		bytesize  = (NCSI_RxDesDat & 0x3fff) - 4;
#else
		bytesize  = (NCSI_RxDesDat & 0x3fff);
#endif
		// Fill up to multiple of 4
		if ( ( bytesize % 4 ) != 0 )
			dwsize = ( bytesize >> 2 ) + 1;
		else
			dwsize = bytesize >> 2;

		if ( eng->arg.GEn_PrintNCSI ) {
#ifdef NCSI_Skip_RxCRCData
			PRINTF( FP_LOG ,"----->[Rx] %d bytes(%xh) [Remove CRC data]\n", bytesize, bytesize );
#else
			PRINTF( FP_LOG ,"----->[Rx] %d bytes(%xh)\n", bytesize, bytesize );
#endif
			for (i = 0; i < dwsize - 1; i++) {
				NCSI_BufData = SWAP_4B_LEDN_NCSI( Read_Mem_Dat_NCSI_DD( NCSI_RxDatBase + ( i << 2 ) ) );
				PRINTF( FP_LOG ,"      [Rx]%02d:%08lx %08lx\n", i, NCSI_BufData, SWAP_4B( NCSI_BufData ) );
			}

			i = ( dwsize - 1 );
			NCSI_BufData = SWAP_4B_LEDN_NCSI( Read_Mem_Dat_NCSI_DD( NCSI_RxDatBase + ( i << 2 ) ) );
			switch ( bytesize % 4 ) {
				case 0  : PRINTF( FP_LOG ,"      [Rx]%02d:%08lx %08lx\n",                          i, NCSI_BufData & SWAP_4B_LEDN_NCSI( 0xffffffff ), SWAP_4B( NCSI_BufData ) & SWAP_4B_BEDN_NCSI( 0xffffffff ) ); break;
				case 3  : PRINTF( FP_LOG ,"      [Rx]%02d:%08lx %08lx [%08lx %08lx][%08x %08x]\n", i, NCSI_BufData & SWAP_4B_LEDN_NCSI( 0x00ffffff ), SWAP_4B( NCSI_BufData ) & SWAP_4B_BEDN_NCSI( 0x00ffffff ), NCSI_BufData, SWAP_4B( NCSI_BufData ), SWAP_4B_LEDN_NCSI( 0x00ffffff ), SWAP_4B_BEDN_NCSI( 0x00ffffff ) ); break;
				case 2  : PRINTF( FP_LOG ,"      [Rx]%02d:%08lx %08lx [%08lx %08lx][%08x %08x]\n", i, NCSI_BufData & SWAP_4B_LEDN_NCSI( 0x0000ffff ), SWAP_4B( NCSI_BufData ) & SWAP_4B_BEDN_NCSI( 0x0000ffff ), NCSI_BufData, SWAP_4B( NCSI_BufData ), SWAP_4B_LEDN_NCSI( 0x0000ffff ), SWAP_4B_BEDN_NCSI( 0x0000ffff ) ); break;
				case 1  : PRINTF( FP_LOG ,"      [Rx]%02d:%08lx %08lx [%08lx %08lx][%08x %08x]\n", i, NCSI_BufData & SWAP_4B_LEDN_NCSI( 0x000000ff ), SWAP_4B( NCSI_BufData ) & SWAP_4B_BEDN_NCSI( 0x000000ff ), NCSI_BufData, SWAP_4B( NCSI_BufData ), SWAP_4B_LEDN_NCSI( 0x000000ff ), SWAP_4B_BEDN_NCSI( 0x000000ff ) ); break;
				default : PRINTF( FP_LOG ,"      [Rx]%02d:error", i ); break;
			}
		}

		// EtherType field of the response packet should be 0x88F8
//
		if ( ( NCSI_RxData & 0xffff ) == 0xf888 ) {
			for (i = 0; i < dwsize; i++)
				eng->dat.NCSI_RxDWBUF[i] = SWAP_4B_LEDN_NCSI( Read_Mem_Dat_NCSI_DD( NCSI_RxDatBase + ( i << 2 ) ) );

			memcpy ( &eng->ncsi_rsp, eng->dat.NCSI_RxByteBUF, bytesize );

			if ( eng->arg.GEn_PrintNCSI )
				PRINTF( FP_LOG ,"[Frm-NCSI][Rx IID:%2d]\n", eng->ncsi_rsp.IID );

			if ( ( eng->ncsi_rsp.IID == 0x0 ) && ( eng->ncsi_rsp.Command == 0xff ) ) { // AEN Packet
				if ( eng->arg.GEn_PrintNCSI )
					PRINTF( FP_LOG ,"[Frm-NCSI][AEN Packet]Type:%2d\n", SWAP_2B_BEDN( eng->ncsi_rsp.Reason_Code ) & 0xff );
			}
			else {
				ret = 0;
			}
		}
		else {
			if ( eng->arg.GEn_PrintNCSI )
				PRINTF( FP_LOG, "[Frm-Skip] Not NCSI Response: [%08lx & %08x = %08lx]!=[%08x]\n", NCSI_RxData, 0xffff, NCSI_RxData & 0xffff, 0xf888 );
		} // End if ( ( NCSI_RxData & 0xffff ) == 0xf888 )

		if ( HWEOR( NCSI_RxDesDat ) ) {
			// it is last the descriptor in the receive Ring
			Write_Mem_Des_NCSI_DD( eng->run.NCSI_RxDesBase     , EOR_IniVal    );
			eng->run.NCSI_RxDesBase = eng->run.RDES_BASE;//base for read/write
		}
		else {
			Write_Mem_Des_NCSI_DD( eng->run.NCSI_RxDesBase     , 0x00000000    );
			eng->run.NCSI_RxDesBase += 16;
		}

		if ( ret == 0 )
			break;
		retry++;
	} while ( retry < NCSI_RxDESNum );

	if ( ( ret == 0 ) && eng->arg.GEn_PrintNCSI ) {
#ifdef Print_DetailFrame
		ncsi_respdump ( eng, &eng->ncsi_rsp );
#else
		PRINTF( FP_LOG, "[NCSI-Respond] ETyp:%04x MC_ID:%02x HeadVer:%02x IID:%02x Comm:%02x ChlID:%02x PayLen:%04x ResCd:%02x ReaCd:%02x\n",
		SWAP_2B_BEDN( eng->ncsi_rsp.EtherType ),
		eng->ncsi_rsp.MC_ID,
		eng->ncsi_rsp.Header_Revision,
		eng->ncsi_rsp.IID,
		eng->ncsi_rsp.Command,
		eng->ncsi_rsp.ChID,
		SWAP_2B_BEDN( eng->ncsi_rsp.Payload_Length ),
		SWAP_2B_BEDN( eng->ncsi_rsp.Response_Code ),
		SWAP_2B_BEDN( eng->ncsi_rsp.Reason_Code ));
#endif

		NCSI_PrintCommandType2File( eng, eng->ncsi_rsp.Command, eng->ncsi_rsp.IID );
	}

	return( ret );
} // End char NCSI_Rx_SLT (MAC_ENGINE *eng)

//------------------------------------------------------------
char NCSI_Tx (MAC_ENGINE *eng, unsigned char command, unsigned char allid, unsigned short length) {
	int        bytesize;
	int        dwsize;
	int        i;
	int        timeout = 0;
	ULONG      NCSI_TxDesDat;

	eng->ncsi_req.IID++;
	eng->ncsi_req.Command        = command;
	eng->ncsi_req.ChID           = allid;
	eng->ncsi_req.Payload_Length = SWAP_2B_BEDN( length );

	memcpy (  eng->dat.NCSI_TxByteBUF               , &eng->ncsi_req                 , 30     );
	memcpy ( (eng->dat.NCSI_TxByteBUF + 30         ), &eng->dat.NCSI_Payload_Data    , length );
	Calculate_Checksum_NCSI( eng, eng->dat.NCSI_TxByteBUF, 30 + length );
	memcpy ( (eng->dat.NCSI_TxByteBUF + 30 + length), &eng->dat.Payload_Checksum_NCSI, 4      );

	// Header of NC-SI command format is 34 bytes. page 58, NC-SI spec. ver 1.0.0 from DMTF
	// The minimum size of a NC-SI package is 64 bytes.
	bytesize = 34 + length;
	if ( bytesize < 60 ) {
		memset ( eng->dat.NCSI_TxByteBUF + bytesize, 0, 60 - bytesize );
		bytesize = 60;
	}

	// Fill up to multiple of 4
	//    dwsize = (bytesize + 3) >> 2;
	if ( ( bytesize % 4 ) != 0 )
		dwsize = ( bytesize >> 2 ) + 1;
	else
		dwsize = bytesize >> 2;

	if ( eng->arg.GEn_PrintNCSI ) {
		if ( bytesize % 4 )
			memset ( eng->dat.NCSI_TxByteBUF + bytesize, 0, (dwsize << 2) - bytesize );

		PRINTF( FP_LOG ,"----->[Tx] %d bytes(%xh)\n", bytesize, bytesize );
		for ( i = 0; i < dwsize-1; i++ )
			PRINTF( FP_LOG, "      [Tx]%02d:%08lx %08lx\n", i, eng->dat.NCSI_TxDWBUF[i], SWAP_4B( eng->dat.NCSI_TxDWBUF[i] ) );

		i = dwsize - 1;
		switch ( bytesize % 4 ) {
			case 0  : PRINTF( FP_LOG ,"      [Tx]%02d:%08lx %08lx\n",                          i, eng->dat.NCSI_TxDWBUF[i] & SWAP_4B_LEDN_NCSI( 0xffffffff ), SWAP_4B( eng->dat.NCSI_TxDWBUF[i] ) & SWAP_4B_BEDN_NCSI( 0xffffffff ) ); break;
			case 3  : PRINTF( FP_LOG ,"      [Tx]%02d:%08lx %08lx [%08lx %08lx][%08x %08x]\n", i, eng->dat.NCSI_TxDWBUF[i] & SWAP_4B_LEDN_NCSI( 0x00ffffff ), SWAP_4B( eng->dat.NCSI_TxDWBUF[i] ) & SWAP_4B_BEDN_NCSI( 0x00ffffff ), eng->dat.NCSI_TxDWBUF[i], SWAP_4B( eng->dat.NCSI_TxDWBUF[i] ), SWAP_4B_LEDN_NCSI( 0x00ffffff ), SWAP_4B_BEDN_NCSI( 0x00ffffff ) ); break;
			case 2  : PRINTF( FP_LOG ,"      [Tx]%02d:%08lx %08lx [%08lx %08lx][%08x %08x]\n", i, eng->dat.NCSI_TxDWBUF[i] & SWAP_4B_LEDN_NCSI( 0x0000ffff ), SWAP_4B( eng->dat.NCSI_TxDWBUF[i] ) & SWAP_4B_BEDN_NCSI( 0x0000ffff ), eng->dat.NCSI_TxDWBUF[i], SWAP_4B( eng->dat.NCSI_TxDWBUF[i] ), SWAP_4B_LEDN_NCSI( 0x0000ffff ), SWAP_4B_BEDN_NCSI( 0x0000ffff ) ); break;
			case 1  : PRINTF( FP_LOG ,"      [Tx]%02d:%08lx %08lx [%08lx %08lx][%08x %08x]\n", i, eng->dat.NCSI_TxDWBUF[i] & SWAP_4B_LEDN_NCSI( 0x000000ff ), SWAP_4B( eng->dat.NCSI_TxDWBUF[i] ) & SWAP_4B_BEDN_NCSI( 0x000000ff ), eng->dat.NCSI_TxDWBUF[i], SWAP_4B( eng->dat.NCSI_TxDWBUF[i] ), SWAP_4B_LEDN_NCSI( 0x000000ff ), SWAP_4B_BEDN_NCSI( 0x000000ff ) ); break;
			default : PRINTF( FP_LOG ,"      [Tx]%02d:error", i ); break;
		}
		PRINTF( FP_LOG ,"[Frm-NCSI][Tx IID:%2d]\n", eng->ncsi_req.IID );
	}

	// Copy data to DMA buffer
	for ( i = 0; i < dwsize; i++ )
		Write_Mem_Dat_NCSI_DD( DMA_BASE + ( i << 2 ), SWAP_4B_LEDN_NCSI( eng->dat.NCSI_TxDWBUF[i] ) );

	// Setting one TX descriptor
//	Write_Mem_Des_NCSI_DD( eng->run.NCSI_TxDesBase + 0x04, 0                        );
//	Write_Mem_Des_NCSI_DD( eng->run.NCSI_TxDesBase + 0x08, 0                        );
//	Write_Mem_Des_NCSI_DD( eng->run.NCSI_TxDesBase + 0x0C, AT_MEMRW_BUF( DMA_BASE ) );
	Write_Mem_Des_NCSI_DD( eng->run.NCSI_TxDesBase       , 0xf0008000 + bytesize );
	// Fire
	Write_Reg_MAC_DD( eng, 0x18, 0x00000000 );//Tx Poll

	do {
		NCSI_TxDesDat = Read_Mem_Des_NCSI_DD( eng->run.NCSI_TxDesBase );
		if ( ++timeout > TIME_OUT_NCSI ) {
			PRINTF( FP_LOG, "[TxDes] DesOwn timeout     %08lX\n", NCSI_TxDesDat );
			return( FindErr( eng, Err_Flag_NCSI_Check_TxOwnTimeOut  ));
		}
	} while ( HWOwnTx( NCSI_TxDesDat ) );

	if ( eng->arg.GEn_PrintNCSI ) {
#ifdef Print_DetailFrame
		ncsi_reqdump ( eng, &eng->ncsi_req );
#else
		PRINTF( FP_LOG, "[NCSI-Request] ETyp:%04x MC_ID:%02x HeadVer:%02x IID:%02x Comm:%02x ChlID:%02x PayLen:%04x\n",
		SWAP_2B_BEDN( eng->ncsi_req.EtherType ),
		eng->ncsi_req.MC_ID,
		eng->ncsi_req.Header_Revision,
		eng->ncsi_req.IID,
		eng->ncsi_req.Command,
		eng->ncsi_req.ChID,
		SWAP_2B_BEDN( eng->ncsi_req.Payload_Length ) );
#endif

		NCSI_PrintCommandType2File( eng, eng->ncsi_req.Command, eng->ncsi_req.IID );
	}
#ifdef Print_PackageName
	NCSI_PrintCommandType( eng, eng->ncsi_req.Command, eng->ncsi_req.IID );
#endif

	return(0);
} // End char NCSI_Tx (MAC_ENGINE *eng, unsigned char command, unsigned char allid, unsigned short length)

//------------------------------------------------------------
char NCSI_ARP (MAC_ENGINE *eng) {
	int        i;
	int        timeout = 0;
	ULONG      NCSI_TxDesDat;

	if ( eng->arg.GEn_PrintNCSI )
		PRINTF( FP_LOG ,"----->[ARP] 60 bytes x%ld\n", eng->arg.GARPNumCnt );

	for (i = 0; i < 15; i++) {
		if ( eng->arg.GEn_PrintNCSI )
			PRINTF( FP_LOG, "      [Tx%02d] %08lx %08lx\n", i, eng->dat.ARP_data[i], SWAP_4B( eng->dat.ARP_data[i] ) );

		Write_Mem_Dat_NCSI_DD( DMA_BASE + ( i << 2 ), eng->dat.ARP_data[i] );
	}

//	Write_Mem_Des_NCSI_DD( eng->run.NCSI_TxDesBase + 0x04, 0                        );
//	Write_Mem_Des_NCSI_DD( eng->run.NCSI_TxDesBase + 0x08, 0                        );
//	Write_Mem_Des_NCSI_DD( eng->run.NCSI_TxDesBase + 0x0C, AT_MEMRW_BUF( DMA_BASE ) );
	for (i = 0; i < eng->arg.GARPNumCnt; i++) {
		Write_Mem_Des_NCSI_DD( eng->run.NCSI_TxDesBase      , 0xf0008000 + 60);
		Write_Reg_MAC_DD( eng, 0x18, 0x00000000 );//Tx Poll

		do {
			NCSI_TxDesDat = Read_Mem_Des_NCSI_DD( eng->run.NCSI_TxDesBase );
			if ( ++timeout > TIME_OUT_NCSI ) {
				PRINTF( FP_LOG, "[TxDes-ARP] DesOwn timeout %08lx\n", NCSI_TxDesDat );
				return( FindErr( eng, Err_Flag_NCSI_Check_ARPOwnTimeOut ) );
			}
		} while ( HWOwnTx( NCSI_TxDesDat ) );
	}
	return(0);
} // End char NCSI_ARP (MAC_ENGINE *eng)

//------------------------------------------------------------
char NCSI_SentWaitPacket (MAC_ENGINE *eng, unsigned char command, unsigned char allid, unsigned short length) {
	int        Retry = 0;

	do {
		if ( NCSI_Tx( eng, command, allid, length ) )
			return( 1 );

#ifdef NCSI_EnableDelay_EachPackage
		DELAY( Delay_EachPackage );
#endif
		if ( NCSI_Rx_SLT( eng ) )
			return( 2 );

		if (    ( eng->ncsi_rsp.IID           != eng->ncsi_req.IID                        )
		     || ( eng->ncsi_rsp.Command       != ( command | 0x80 )                       )
		     || ( eng->ncsi_rsp.Response_Code != SWAP_2B_BEDN( COMMAND_COMPLETED ) ) ) {
			if ( eng->arg.GEn_PrintNCSI ) {
				PRINTF( FP_LOG, "Retry: Command = %x, Response_Code = %x", eng->ncsi_req.Command, SWAP_2B_BEDN( eng->ncsi_rsp.Response_Code ) );
				switch ( SWAP_2B_BEDN( eng->ncsi_rsp.Response_Code ) ) {
					case COMMAND_COMPLETED  	: PRINTF( FP_LOG, "(completed  )\n" ); break;
					case COMMAND_FAILED     	: PRINTF( FP_LOG, "(failed     )\n" ); break;
					case COMMAND_UNAVAILABLE	: PRINTF( FP_LOG, "(unavailable)\n" ); break;
					case COMMAND_UNSUPPORTED	: PRINTF( FP_LOG, "(unsupported)\n" ); break;
					default                 	: PRINTF( FP_LOG, "(-----------)\n" ); break;
				}
			}
			Retry++;
		}
		else {
			return( 0 );
		}
	} while (Retry <= SENT_RETRY_COUNT);

	return( 3 );
} // End char NCSI_SentWaitPacket (unsigned char command, unsigned char id, unsigned short length)

//------------------------------------------------------------
char Clear_Initial_State_SLT (MAC_ENGINE *eng) {//Command:0x00
	char       return_value;

	eng->flg.Bak_Err_Flag  = eng->flg.Err_Flag;
	eng->flg.Bak_NCSI_Flag = eng->flg.NCSI_Flag;

	return_value = NCSI_SentWaitPacket( eng, CLEAR_INITIAL_STATE, eng->ncsi_cap.All_ID, 0 );//Internal Channel ID = 0

	eng->flg.Err_Flag  = eng->flg.Bak_Err_Flag;
	eng->flg.NCSI_Flag = eng->flg.Bak_NCSI_Flag;
	return( return_value );//Internal Channel ID = 0
}

//------------------------------------------------------------
char Select_Package_SLT (MAC_ENGINE *eng, char skipflag) {//Command:0x01
	char       return_value;

	if ( skipflag ) {
		eng->flg.Bak_Err_Flag  = eng->flg.Err_Flag;
		eng->flg.Bak_NCSI_Flag = eng->flg.NCSI_Flag;
	}

	memset ((void *)eng->dat.NCSI_Payload_Data, 0, 4);
	eng->dat.NCSI_Payload_Data[ 3 ] = 1; //Arbitration Disable
	return_value = NCSI_SentWaitPacket( eng, SELECT_PACKAGE, ( eng->ncsi_cap.Package_ID << 5 ) + 0x1F, 4 );//Internal Channel ID = 0x1F, 0x1F means all channel
	if ( return_value )
		FindErr_NCSI( eng, NCSI_Flag_Select_Package );

	if ( skipflag ) {
		eng->flg.Err_Flag  = eng->flg.Bak_Err_Flag;
		eng->flg.NCSI_Flag = eng->flg.Bak_NCSI_Flag;
	}
	return( return_value );
}

//------------------------------------------------------------
void Select_Active_Package_SLT (MAC_ENGINE *eng) {//Command:0x01
	memset ((void *)eng->dat.NCSI_Payload_Data, 0, 4);
	eng->dat.NCSI_Payload_Data[ 3 ] = 1; //Arbitration Disable

	if ( NCSI_SentWaitPacket( eng, SELECT_PACKAGE, ( eng->ncsi_cap.Package_ID << 5 ) + 0x1F, 4 ) ) //Internal Channel ID = 0x1F, 0x1F means all channel
		FindErr_NCSI( eng, NCSI_Flag_Select_Active_Package );
}

//------------------------------------------------------------
void DeSelect_Package_SLT (MAC_ENGINE *eng) {//Command:0x02
	if ( NCSI_SentWaitPacket( eng, DESELECT_PACKAGE, ( eng->ncsi_cap.Package_ID << 5 ) + 0x1F, 0 ) ) //Internal Channel ID = 0x1F, 0x1F means all channel
		FindErr_NCSI( eng, NCSI_Flag_Deselect_Package );

#ifdef NCSI_EnableDelay_DeSelectPackage
	DELAY( Delay_DeSelectPackage );
#endif
}

//------------------------------------------------------------
void Enable_Channel_SLT (MAC_ENGINE *eng) {//Command:0x03
	if ( NCSI_SentWaitPacket( eng, ENABLE_CHANNEL, eng->ncsi_cap.All_ID, 0 ) )
		FindErr_NCSI( eng, NCSI_Flag_Enable_Channel );
}

//------------------------------------------------------------
void Disable_Channel_SLT (MAC_ENGINE *eng, char skipflag) {//Command:0x04
	if ( skipflag ) {
		eng->flg.Bak_Err_Flag  = eng->flg.Err_Flag;
		eng->flg.Bak_NCSI_Flag = eng->flg.NCSI_Flag;
	}

	memset ((void *)eng->dat.NCSI_Payload_Data, 0, 4);
	eng->dat.NCSI_Payload_Data[ 3 ] = 0x1; //ALD
	if ( NCSI_SentWaitPacket( eng, DISABLE_CHANNEL, eng->ncsi_cap.All_ID, 4 ) )
		FindErr_NCSI( eng, NCSI_Flag_Disable_Channel );

	if ( skipflag ) {
		eng->flg.Err_Flag  = eng->flg.Bak_Err_Flag;
		eng->flg.NCSI_Flag = eng->flg.Bak_NCSI_Flag;
	}
}

//------------------------------------------------------------
void Enable_Network_TX_SLT (MAC_ENGINE *eng) {//Command:0x06
	if ( NCSI_SentWaitPacket( eng, ENABLE_CHANNEL_NETWORK_TX, eng->ncsi_cap.All_ID, 0 ) )
		FindErr_NCSI( eng, NCSI_Flag_Enable_Network_TX );
}

//------------------------------------------------------------
void Disable_Network_TX_SLT (MAC_ENGINE *eng) {//Command:0x07
	if ( NCSI_SentWaitPacket( eng, DISABLE_CHANNEL_NETWORK_TX, eng->ncsi_cap.All_ID, 0 ) )
		FindErr_NCSI( eng, NCSI_Flag_Disable_Network_TX );
}

//------------------------------------------------------------
void Set_Link_SLT (MAC_ENGINE *eng) {//Command:0x09
	memset ((void *)eng->dat.NCSI_Payload_Data, 0, 8);
	eng->dat.NCSI_Payload_Data[ 2 ] = 0x02; //full duplex
//	eng->dat.NCSI_Payload_Data[ 3 ] = 0x04; //100M, auto-disable
	eng->dat.NCSI_Payload_Data[ 3 ] = 0x05; //100M, auto-enable

	if ( NCSI_SentWaitPacket( eng, SET_LINK, eng->ncsi_cap.All_ID, 8 ) )
		FindErr_NCSI( eng, NCSI_Flag_Set_Link );
}

//------------------------------------------------------------
char Get_Link_Status_SLT (MAC_ENGINE *eng) {//Command:0x0a
	if ( NCSI_SentWaitPacket( eng, GET_LINK_STATUS, eng->ncsi_cap.All_ID, 0 ) )
		return(0);
	else {
		if ( eng->ncsi_rsp.Payload_Data[ 3 ] & 0x20 ) {
			if ( eng->ncsi_rsp.Payload_Data[ 3 ] & 0x40 ) {
				if ( eng->ncsi_rsp.Payload_Data[ 3 ] & 0x01 )
					return(1); //Link Up or Not
				else
					return(0);
			}
			else
				return(0); //Auto Negotiate did not finish
		}
		else {
			if ( eng->ncsi_rsp.Payload_Data[ 3 ] & 0x01 )
				return(1); //Link Up or Not
			else
				return(0);
		}
	}
} // End char Get_Link_Status_SLT (MAC_ENGINE *eng)

//------------------------------------------------------------
void Enable_Set_MAC_Address_SLT (MAC_ENGINE *eng) {//Command:0x0e
	int        i;

	for ( i = 0; i < 6; i++ )
		eng->dat.NCSI_Payload_Data[ i ] = eng->ncsi_req.SA[ i ];
	eng->dat.NCSI_Payload_Data[ 6 ] = 1; //MAC Address Num = 1 --> address filter 1, fixed in sample code

	if ( eng->ncsi_req.SA[ 0 ] & 0x1 )
		eng->dat.NCSI_Payload_Data[ 7 ] = MULTICAST + ENABLE_MAC_ADDRESS_FILTER; //AT + E
	else
		eng->dat.NCSI_Payload_Data[ 7 ] = UNICAST   + ENABLE_MAC_ADDRESS_FILTER; //AT + E

	if ( NCSI_SentWaitPacket( eng, SET_MAC_ADDRESS, eng->ncsi_cap.All_ID, 8 ) )
		FindErr_NCSI( eng, NCSI_Flag_Enable_Set_MAC_Address );
}

//------------------------------------------------------------
void Enable_Broadcast_Filter_SLT (MAC_ENGINE *eng) {//Command:0x10
	memset ((void *)eng->dat.NCSI_Payload_Data, 0, 4);
	eng->dat.NCSI_Payload_Data[ 3 ] = 0xF; //ARP, DHCP, NetBIOS

	if ( NCSI_SentWaitPacket( eng, ENABLE_BROADCAST_FILTERING, eng->ncsi_cap.All_ID, 4 ) )
		FindErr_NCSI( eng, NCSI_Flag_Enable_Broadcast_Filter );
}

//------------------------------------------------------------
void Get_Version_ID_SLT (MAC_ENGINE *eng) {//Command:0x15
	if ( NCSI_SentWaitPacket( eng, GET_VERSION_ID, eng->ncsi_cap.All_ID, 0 ) )
		FindErr_NCSI( eng, NCSI_Flag_Get_Version_ID );
	else {
#ifdef Print_Version_ID
		printf("NCSI Version        : %02x %02x %02x %02x\n", eng->ncsi_rsp.Payload_Data[  0 ], eng->ncsi_rsp.Payload_Data[  1 ], eng->ncsi_rsp.Payload_Data[  2 ], eng->ncsi_rsp.Payload_Data[  3 ]);
		printf("NCSI Version        : %02x %02x %02x %02x\n", eng->ncsi_rsp.Payload_Data[  4 ], eng->ncsi_rsp.Payload_Data[  5 ], eng->ncsi_rsp.Payload_Data[  6 ], eng->ncsi_rsp.Payload_Data[  7 ]);
		printf("Firmware Name String: %02x %02x %02x %02x\n", eng->ncsi_rsp.Payload_Data[  8 ], eng->ncsi_rsp.Payload_Data[  9 ], eng->ncsi_rsp.Payload_Data[ 10 ], eng->ncsi_rsp.Payload_Data[ 11 ]);
		printf("Firmware Name String: %02x %02x %02x %02x\n", eng->ncsi_rsp.Payload_Data[ 12 ], eng->ncsi_rsp.Payload_Data[ 13 ], eng->ncsi_rsp.Payload_Data[ 14 ], eng->ncsi_rsp.Payload_Data[ 15 ]);
		printf("Firmware Name String: %02x %02x %02x %02x\n", eng->ncsi_rsp.Payload_Data[ 16 ], eng->ncsi_rsp.Payload_Data[ 17 ], eng->ncsi_rsp.Payload_Data[ 18 ], eng->ncsi_rsp.Payload_Data[ 19 ]);
		printf("Firmware Version    : %02x %02x %02x %02x\n", eng->ncsi_rsp.Payload_Data[ 20 ], eng->ncsi_rsp.Payload_Data[ 21 ], eng->ncsi_rsp.Payload_Data[ 22 ], eng->ncsi_rsp.Payload_Data[ 23 ]);
		printf("PCI DID/VID         : %02x %02x/%02x %02x\n", eng->ncsi_rsp.Payload_Data[ 24 ], eng->ncsi_rsp.Payload_Data[ 25 ], eng->ncsi_rsp.Payload_Data[ 26 ], eng->ncsi_rsp.Payload_Data[ 27 ]);
		printf("PCI SSID/SVID       : %02x %02x/%02x %02x\n", eng->ncsi_rsp.Payload_Data[ 28 ], eng->ncsi_rsp.Payload_Data[ 29 ], eng->ncsi_rsp.Payload_Data[ 30 ], eng->ncsi_rsp.Payload_Data[ 31 ]);
		printf("Manufacturer ID     : %02x %02x %02x %02x\n", eng->ncsi_rsp.Payload_Data[ 32 ], eng->ncsi_rsp.Payload_Data[ 33 ], eng->ncsi_rsp.Payload_Data[ 34 ], eng->ncsi_rsp.Payload_Data[ 35 ]);
		printf("Checksum            : %02x %02x %02x %02x\n", eng->ncsi_rsp.Payload_Data[ 36 ], eng->ncsi_rsp.Payload_Data[ 37 ], eng->ncsi_rsp.Payload_Data[ 38 ], eng->ncsi_rsp.Payload_Data[ 39 ]);
#endif
		eng->ncsi_cap.PCI_DID_VID    = (eng->ncsi_rsp.Payload_Data[ 24 ]<<24)
		                             | (eng->ncsi_rsp.Payload_Data[ 25 ]<<16)
		                             | (eng->ncsi_rsp.Payload_Data[ 26 ]<< 8)
		                             | (eng->ncsi_rsp.Payload_Data[ 27 ]    );
		eng->ncsi_cap.ManufacturerID = (eng->ncsi_rsp.Payload_Data[ 32 ]<<24)
		                             | (eng->ncsi_rsp.Payload_Data[ 33 ]<<16)
		                             | (eng->ncsi_rsp.Payload_Data[ 34 ]<< 8)
		                             | (eng->ncsi_rsp.Payload_Data[ 35 ]    );
	}
} // End void Get_Version_ID_SLT (MAC_ENGINE *eng)

//------------------------------------------------------------
void Get_Capabilities_SLT (MAC_ENGINE *eng) {//Command:0x16
	if ( NCSI_SentWaitPacket( eng, GET_CAPABILITIES, eng->ncsi_cap.All_ID, 0 ) )
		FindErr_NCSI( eng, NCSI_Flag_Get_Capabilities );
	else {
//		eng->ncsi_cap.Capabilities_Flags                   = (eng->ncsi_rsp.Payload_Data[  0 ]<<24)
//		                                                   | (eng->ncsi_rsp.Payload_Data[  1 ]<<16)
//		                                                   | (eng->ncsi_rsp.Payload_Data[  2 ]<< 8)
//		                                                   | (eng->ncsi_rsp.Payload_Data[  3 ]    );
//		eng->ncsi_cap.Broadcast_Packet_Filter_Capabilities = (eng->ncsi_rsp.Payload_Data[  4 ]<<24)
//		                                                   | (eng->ncsi_rsp.Payload_Data[  5 ]<<16)
//		                                                   | (eng->ncsi_rsp.Payload_Data[  6 ]<< 8)
//		                                                   | (eng->ncsi_rsp.Payload_Data[  7 ]    );
//		eng->ncsi_cap.Multicast_Packet_Filter_Capabilities = (eng->ncsi_rsp.Payload_Data[  8 ]<<24)
//		                                                   | (eng->ncsi_rsp.Payload_Data[  9 ]<<16)
//		                                                   | (eng->ncsi_rsp.Payload_Data[ 10 ]<< 8)
//		                                                   | (eng->ncsi_rsp.Payload_Data[ 11 ]    );
//		eng->ncsi_cap.Buffering_Capabilities               = (eng->ncsi_rsp.Payload_Data[ 12 ]<<24)
//		                                                   | (eng->ncsi_rsp.Payload_Data[ 13 ]<<16)
//		                                                   | (eng->ncsi_rsp.Payload_Data[ 14 ]<< 8)
//		                                                   | (eng->ncsi_rsp.Payload_Data[ 15 ]    );
//		eng->ncsi_cap.AEN_Control_Support                  = (eng->ncsi_rsp.Payload_Data[ 16 ]<<24)
//		                                                   | (eng->ncsi_rsp.Payload_Data[ 17 ]<<16)
//		                                                   | (eng->ncsi_rsp.Payload_Data[ 18 ]<< 8)
//		                                                   | (eng->ncsi_rsp.Payload_Data[ 19 ]    );
//		eng->ncsi_cap.VLAN_Filter_Count                    =  eng->ncsi_rsp.Payload_Data[ 20 ];
		eng->ncsi_cap.Mixed_Filter_Count                   =  eng->ncsi_rsp.Payload_Data[ 21 ];
//		eng->ncsi_cap.Multicast_Filter_Count               =  eng->ncsi_rsp.Payload_Data[ 22 ];
		eng->ncsi_cap.Unicast_Filter_Count                 =  eng->ncsi_rsp.Payload_Data[ 23 ];
//		eng->ncsi_cap.VLAN_Mode_Support                    =  eng->ncsi_rsp.Payload_Data[ 26 ]
		eng->ncsi_cap.Channel_Count                        =  eng->ncsi_rsp.Payload_Data[ 27 ];
	}
}

//------------------------------------------------------------
void Get_Controller_Packet_Statistics_SLT (MAC_ENGINE *eng) {//Command:0x18
	if ( NCSI_SentWaitPacket( eng, GET_CONTROLLER_PACKET_STATISTICS, eng->ncsi_cap.All_ID, 0 ) )
		FindErr_NCSI( eng, NCSI_Flag_Get_Controller_Packet_Statistics );
}

//------------------------------------------------------------
char phy_ncsi (MAC_ENGINE *eng) {
	ULONG      pkg_idx;
	ULONG      chl_idx;
	ULONG      select_flag[ MAX_PACKAGE_NUM ];
	ULONG      Re_Send;
	ULONG      Link_Status;

	eng->dat.NCSI_RxEr  = 0;
	eng->dat.number_chl = 0;
	eng->dat.number_pak = 0;
	eng->ncsi_cap.Package_ID = 0;
	eng->ncsi_cap.Channel_ID = 0x1F;
	eng->ncsi_cap.All_ID     = 0x1F;
	PRINTF( FP_LOG, "\n\n======> Start:\n" );

	NCSI_Struct_Initialize_SLT( eng );

#ifdef NCSI_Skip_Phase1_DeSelectPackage
#else
	//NCSI Start
	//Disable Channel then DeSelect Package
	for (pkg_idx = 0; pkg_idx < MAX_PACKAGE_NUM; pkg_idx++) {
		eng->ncsi_cap.Package_ID = pkg_idx;
		eng->ncsi_cap.Channel_ID = 0x1F;
		eng->ncsi_cap.All_ID     = ( eng->ncsi_cap.Package_ID << 5) + eng->ncsi_cap.Channel_ID;

		select_flag[ pkg_idx ] = Select_Package_SLT ( eng, 1 ); //skipflag// Command:0x01

		if ( select_flag[ pkg_idx ] == 0 ) {
			for ( chl_idx = 0; chl_idx < MAX_CHANNEL_NUM; chl_idx++ ) {
				eng->ncsi_cap.Channel_ID = chl_idx;
				eng->ncsi_cap.All_ID     = ( eng->ncsi_cap.Package_ID << 5) + eng->ncsi_cap.Channel_ID;

				Disable_Channel_SLT( eng, 1 );//skipflag // Command: 0x04
			}
  #ifdef NCSI_Skip_DeSelectPackage
  #else
			DeSelect_Package_SLT ( eng ); // Command:0x02
  #endif
		} // End if ( select_flag[ pkg_idx ] == 0 )
	} // End for (pkg_idx = 0; pkg_idx < MAX_PACKAGE_NUM; pkg_idx++)
#endif

	//Select Package
	for ( pkg_idx = 0; pkg_idx < MAX_PACKAGE_NUM; pkg_idx++ ) {
		eng->ncsi_cap.Package_ID = pkg_idx;
		eng->ncsi_cap.Channel_ID = 0x1F;
		eng->ncsi_cap.All_ID     = ( eng->ncsi_cap.Package_ID << 5) + eng->ncsi_cap.Channel_ID;

#ifdef NCSI_Skip_Phase1_DeSelectPackage
		select_flag[ pkg_idx ] = Select_Package_SLT ( eng, 1 ); //skipflag//Command:0x01
#endif

		if ( select_flag[ pkg_idx ] == 0 ) {
			//eng->run.NCSI_RxTimeOutScale = 1000;
			eng->run.NCSI_RxTimeOutScale = 10;

#ifdef NCSI_Skip_Phase1_DeSelectPackage
#else
			Select_Package_SLT ( eng, 0 );//Command:0x01
#endif
			eng->dat.number_pak++;
			if ( !eng->run.IO_MrgChk ) {
				printf("====Find Package ID: %d\n", eng->ncsi_cap.Package_ID);
				PRINTF(FP_LOG, "====Find Package ID: %d\n", eng->ncsi_cap.Package_ID );
			}

			// Scan all channel in the package
			for ( chl_idx = 0; chl_idx < MAX_CHANNEL_NUM; chl_idx++ ) {
				eng->ncsi_cap.Channel_ID = chl_idx;
				eng->ncsi_cap.All_ID     = ( eng->ncsi_cap.Package_ID << 5) + eng->ncsi_cap.Channel_ID;

				if ( Clear_Initial_State_SLT( eng ) == 0 ) { //Command:0x00
					eng->dat.number_chl++;
					if ( !eng->run.IO_MrgChk ) {
						printf("--------Find Channel ID: %d\n", eng->ncsi_cap.Channel_ID);
						PRINTF( FP_LOG, "--------Find Channel ID: %d\n", eng->ncsi_cap.Channel_ID );
					}

					// Get Version and Capabilities
					Get_Version_ID_SLT( eng );          //Command:0x15
					Get_Capabilities_SLT( eng );        //Command:0x16
					Select_Active_Package_SLT( eng );   //Command:0x01
					Enable_Set_MAC_Address_SLT( eng );  //Command:0x0e
					Enable_Broadcast_Filter_SLT( eng ); //Command:0x10

					// Enable TX
					Enable_Network_TX_SLT( eng );       //Command:0x06

					// Enable Channel
					Enable_Channel_SLT( eng );          //Command:0x03

					// Get Link Status
					Re_Send = 0;
					do {
#ifdef NCSI_EnableDelay_GetLinkStatus
						if ( Re_Send )
							DELAY( Delay_GetLinkStatus );
#endif

						Link_Status = Get_Link_Status_SLT( eng );//Command:0x0a
						if ( Link_Status == LINK_UP ) {
							if ( eng->arg.GARPNumCnt )
								NCSI_ARP ( eng );
							break;
						} // End if ( Link_Status == LINK_UP )
					} while ( Re_Send++ <= 2 );

					if ( !eng->run.IO_MrgChk ) {
						if ( Link_Status == LINK_UP ) {
							printf("        This Channel is LINK_UP (MFC:%d, UFC:%d, CC:%d)\n", eng->ncsi_cap.Mixed_Filter_Count, eng->ncsi_cap.Unicast_Filter_Count, eng->ncsi_cap.Channel_Count);
							PRINTF( FP_LOG, "        This Channel is LINK_UP (MFC:%d, UFC:%d, CC:%d)\n", eng->ncsi_cap.Mixed_Filter_Count, eng->ncsi_cap.Unicast_Filter_Count, eng->ncsi_cap.Channel_Count);
						}
						else {
							printf("        This Channel is LINK_DOWN (MFC:%d, UFC:%d, CC:%d)\n", eng->ncsi_cap.Mixed_Filter_Count, eng->ncsi_cap.Unicast_Filter_Count, eng->ncsi_cap.Channel_Count);
							PRINTF( FP_LOG, "        This Channel is LINK_DOWN (MFC:%d, UFC:%d, CC:%d)\n", eng->ncsi_cap.Mixed_Filter_Count, eng->ncsi_cap.Unicast_Filter_Count, eng->ncsi_cap.Channel_Count);
						}
					}

#ifdef NCSI_Skip_DiSChannel
#else
					if ( eng->run.TM_NCSI_DiSChannel ) {
						// Disable TX
						Disable_Network_TX_SLT( eng ); //Command:0x07
						// Disable Channel
						Disable_Channel_SLT( eng, 0 );    //Command:0x04
					}
#endif
				} // End if ( Clear_Initial_State_SLT( eng, chl_idx ) == 0 )
			} // End for ( chl_idx = 0; chl_idx < MAX_CHANNEL_NUM; chl_idx++ )

#ifdef NCSI_Skip_DeSelectPackage
#else
			DeSelect_Package_SLT ( eng );//Command:0x02
#endif
			eng->run.NCSI_RxTimeOutScale = 1;
		}
		else {
			if ( !eng->run.IO_MrgChk ) {
				printf("====Absence of Package ID: %ld\n", pkg_idx);
				PRINTF( FP_LOG, "====Absence of Package ID: %ld\n", pkg_idx );
			}
		} // End if ( select_flag[pkg_idx] == 0 )
	} // End for ( pkg_idx = 0; pkg_idx < MAX_PACKAGE_NUM; pkg_idx++ )

	if ( eng->dat.number_pak == 0                       ) FindErr( eng, Err_Flag_NCSI_No_PHY      );
	if ( eng->dat.number_pak != eng->arg.GPackageTolNum ) FindErr( eng, Err_Flag_NCSI_Package_Num );
	if ( eng->dat.number_chl != eng->arg.GChannelTolNum ) FindErr( eng, Err_Flag_NCSI_Channel_Num );
//	if ( eng->dat.number_chl == 0                       ) FindErr( eng );

	if ( eng->flg.Err_Flag ) {
		if ( eng->dat.NCSI_RxEr )
			FindErr_Des( eng, Des_Flag_RxErr );
		return(1);
	}
	else {
		if ( eng->dat.NCSI_RxEr ) {
			eng->flg.Wrn_Flag = eng->flg.Wrn_Flag | Wrn_Flag_RxErFloatting;
			if ( eng->arg.GEn_SkipRxEr ) {
				eng->flg.AllFail = 0;
				return(0);
			}
			else {
				FindErr_Des( eng, Des_Flag_RxErr );
				return(1);
			}
		}
		else {
			eng->flg.AllFail = 0;
			return(0);
		}
	}
}
