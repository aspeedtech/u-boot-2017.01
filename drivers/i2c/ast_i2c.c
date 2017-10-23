/*
 *  i2c_adap_ast.c
 *
 *  I2C adapter for the ASPEED I2C bus access.
 *
 *  Copyright (C) 2012-2020  ASPEED Technology Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  History:
 *    2012.07.26: Initial version [Ryan Chen]
 */

#include <common.h>
#include <fdtdec.h>

#include <i2c.h>
#include <asm/arch/ast-scu.h>

#include <asm/io.h>
#include <asm/arch/regs-iic.h>

//AST2400 buffer mode issue , force I2C slave write use byte mode , read use buffer mode 
/* Use platform_data instead of module parameters */
/* Fast Mode = 400 kHz, Standard = 100 kHz */
//static int clock = 100; /* Default: 100 kHz */

/***************************************************************************/
DECLARE_GLOBAL_DATA_PTR;

static unsigned int i2c_bus_num __attribute__((section(".data")));

/* Information about i2c controller */
struct ast_i2c_bus {
	u32					reg_base;			/* virtual */	
	u32 				speed;
	u32					state;				//I2C xfer mode state matchine 
	u16 addr; 			/* slave address			*/
	u16 flags;
	u16 a_len;		/* msg length				*/
	u8 *a_buf;		/* pointer to msg data		*/	
	u16 d_len;			/* msg length				*/
	u8 *d_buf;		/* pointer to msg data		*/	
};

static struct ast_i2c_bus ast_i2c[NUM_BUS] __attribute__((section(".data")));

struct ast_i2c_timing_table {
	u32 divisor;
	u32 timing;
};

static struct ast_i2c_timing_table i2c_timing_table[] = {
#if defined(AST_SOC_G5)
	/* Divisor : Base Clock : tCK High : tCK Low  */		
	/* Divisor :	  [3:0]    :   [19:16]:   [15:12] */
	{6, 		0x77700300 | (0x0) | (0x2<<16) | (0x2<<12) },
	{7, 		0x77700300 | (0x0) | (0x3<<16) | (0x2<<12) },
	{8, 		0x77700300 | (0x0) | (0x3<<16) | (0x3<<12) },	
	{9, 		0x77700300 | (0x0) | (0x4<<16) | (0x3<<12) },
	{10, 	0x77700300 | (0x0) | (0x4<<16) | (0x4<<12) },
	{11, 	0x77700300 | (0x0) | (0x5<<16) | (0x4<<12) },
	{12, 	0x77700300 | (0x0) | (0x5<<16) | (0x5<<12) },
	{13, 	0x77700300 | (0x0) | (0x6<<16) | (0x5<<12) },
	{14, 	0x77700300 | (0x0) | (0x6<<16) | (0x6<<12) },
	{15, 	0x77700300 | (0x0) | (0x7<<16) | (0x6<<12) },
	{16, 	0x77700300 | (0x0) | (0x7<<16) | (0x7<<12) },
	{17, 	0x77700300 | (0x0) | (0x8<<16) | (0x7<<12) },
	{18, 	0x77700300 | (0x0) | (0x8<<16) | (0x8<<12) },
	{19, 	0x77700300 | (0x0) | (0x9<<16) | (0x8<<12) },
	{20, 	0x77700300 | (0x0) | (0x9<<16) | (0x9<<12) },
	{21, 	0x77700300 | (0x0) | (0xa<<16) | (0x9<<12) },
	{22, 	0x77700300 | (0x0) | (0xa<<16) | (0xa<<12) },
	{23, 	0x77700300 | (0x0) | (0xb<<16) | (0xa<<12) },
	{24, 	0x77700300 | (0x0) | (0xb<<16) | (0xb<<12) },
	{25, 	0x77700300 | (0x0) | (0xc<<16) | (0xb<<12) },
	{26, 	0x77700300 | (0x0) | (0xc<<16) | (0xc<<12) },
	{27, 	0x77700300 | (0x0) | (0xd<<16) | (0xc<<12) },
	{28, 	0x77700300 | (0x0) | (0xd<<16) | (0xd<<12) },
	{29, 	0x77700300 | (0x0) | (0xe<<16) | (0xd<<12) },
	{30, 	0x77700300 | (0x0) | (0xe<<16) | (0xe<<12) },
	{31, 	0x77700300 | (0x0) | (0xf<<16) | (0xe<<12) },
	{32, 	0x77700300 | (0x0) | (0xf<<16) | (0xf<<12) },
	
	{34, 	0x77700300 | (0x1) | (0x8<<16) | (0x7<<12) },
	{36, 	0x77700300 | (0x1) | (0x8<<16) | (0x8<<12) },
	{38, 	0x77700300 | (0x1) | (0x9<<16) | (0x8<<12) },
	{40, 	0x77700300 | (0x1) | (0x9<<16) | (0x9<<12) },
	{42, 	0x77700300 | (0x1) | (0xa<<16) | (0x9<<12) },
	{44, 	0x77700300 | (0x1) | (0xa<<16) | (0xa<<12) },
	{46, 	0x77700300 | (0x1) | (0xb<<16) | (0xa<<12) },
	{48, 	0x77700300 | (0x1) | (0xb<<16) | (0xb<<12) },
	{50, 	0x77700300 | (0x1) | (0xc<<16) | (0xb<<12) },
	{52, 	0x77700300 | (0x1) | (0xc<<16) | (0xc<<12) },
	{54, 	0x77700300 | (0x1) | (0xd<<16) | (0xc<<12) },
	{56, 	0x77700300 | (0x1) | (0xd<<16) | (0xd<<12) },
	{58, 	0x77700300 | (0x1) | (0xe<<16) | (0xd<<12) },
	{60, 	0x77700300 | (0x1) | (0xe<<16) | (0xe<<12) },
	{62, 	0x77700300 | (0x1) | (0xf<<16) | (0xe<<12) },
	{64, 	0x77700300 | (0x1) | (0xf<<16) | (0xf<<12) },
	
	{68, 	0x77700300 | (0x2) | (0x8<<16) | (0x7<<12) },
	{72, 	0x77700300 | (0x2) | (0x8<<16) | (0x8<<12) },
	{76, 	0x77700300 | (0x2) | (0x9<<16) | (0x8<<12) },
	{80, 	0x77700300 | (0x2) | (0x9<<16) | (0x9<<12) },
	{84, 	0x77700300 | (0x2) | (0xa<<16) | (0x9<<12) },
	{88, 	0x77700300 | (0x2) | (0xa<<16) | (0xa<<12) },
	{92, 	0x77700300 | (0x2) | (0xb<<16) | (0xa<<12) },
	{96, 	0x77700300 | (0x2) | (0xb<<16) | (0xb<<12) },
	{100, 	0x77700300 | (0x2) | (0xc<<16) | (0xb<<12) },
	{104, 	0x77700300 | (0x2) | (0xc<<16) | (0xc<<12) },
	{108, 	0x77700300 | (0x2) | (0xd<<16) | (0xc<<12) },
	{112, 	0x77700300 | (0x2) | (0xd<<16) | (0xd<<12) },
	{116, 	0x77700300 | (0x2) | (0xe<<16) | (0xd<<12) },
	{120, 	0x77700300 | (0x2) | (0xe<<16) | (0xe<<12) },
	{124, 	0x77700300 | (0x2) | (0xf<<16) | (0xe<<12) },
	{128, 	0x77700300 | (0x2) | (0xf<<16) | (0xf<<12) },
	
	{136, 	0x77700300 | (0x3) | (0x8<<16) | (0x7<<12) },
	{144, 	0x77700300 | (0x3) | (0x8<<16) | (0x8<<12) },
	{152, 	0x77700300 | (0x3) | (0x9<<16) | (0x8<<12) },
	{160, 	0x77700300 | (0x3) | (0x9<<16) | (0x9<<12) },
	{168, 	0x77700300 | (0x3) | (0xa<<16) | (0x9<<12) },
	{176, 	0x77700300 | (0x3) | (0xa<<16) | (0xa<<12) },
	{184, 	0x77700300 | (0x3) | (0xb<<16) | (0xa<<12) },
	{192, 	0x77700300 | (0x3) | (0xb<<16) | (0xb<<12) },
	{200, 	0x77700300 | (0x3) | (0xc<<16) | (0xb<<12) },
	{208, 	0x77700300 | (0x3) | (0xc<<16) | (0xc<<12) },
	{216, 	0x77700300 | (0x3) | (0xd<<16) | (0xc<<12) },
	{224, 	0x77700300 | (0x3) | (0xd<<16) | (0xd<<12) },
	{232, 	0x77700300 | (0x3) | (0xe<<16) | (0xd<<12) },
	{240, 	0x77700300 | (0x3) | (0xe<<16) | (0xe<<12) },
	{248, 	0x77700300 | (0x3) | (0xf<<16) | (0xe<<12) },
	{256, 	0x77700300 | (0x3) | (0xf<<16) | (0xf<<12) },

	{272, 	0x77700300 | (0x4) | (0x8<<16) | (0x7<<12) },
	{288, 	0x77700300 | (0x4) | (0x8<<16) | (0x8<<12) },
	{304, 	0x77700300 | (0x4) | (0x9<<16) | (0x8<<12) },
	{320, 	0x77700300 | (0x4) | (0x9<<16) | (0x9<<12) },
	{336, 	0x77700300 | (0x4) | (0xa<<16) | (0x9<<12) },
	{352, 	0x77700300 | (0x4) | (0xa<<16) | (0xa<<12) },
	{368, 	0x77700300 | (0x4) | (0xb<<16) | (0xa<<12) },
	{384, 	0x77700300 | (0x4) | (0xb<<16) | (0xb<<12) },
	{400, 	0x77700300 | (0x4) | (0xc<<16) | (0xb<<12) },
	{416, 	0x77700300 | (0x4) | (0xc<<16) | (0xc<<12) },
	{432, 	0x77700300 | (0x4) | (0xd<<16) | (0xc<<12) },
	{448, 	0x77700300 | (0x4) | (0xd<<16) | (0xd<<12) },
	{464, 	0x77700300 | (0x4) | (0xe<<16) | (0xd<<12) },
	{480, 	0x77700300 | (0x4) | (0xe<<16) | (0xe<<12) },
	{496, 	0x77700300 | (0x4) | (0xf<<16) | (0xe<<12) },
	{512, 	0x77700300 | (0x4) | (0xf<<16) | (0xf<<12) },

	{544, 	0x77700300 | (0x5) | (0x8<<16) | (0x7<<12) },
	{576, 	0x77700300 | (0x5) | (0x8<<16) | (0x8<<12) },
	{608, 	0x77700300 | (0x5) | (0x9<<16) | (0x8<<12) },
	{640, 	0x77700300 | (0x5) | (0x9<<16) | (0x9<<12) },
	{672, 	0x77700300 | (0x5) | (0xa<<16) | (0x9<<12) },
	{704, 	0x77700300 | (0x5) | (0xa<<16) | (0xa<<12) },
	{736, 	0x77700300 | (0x5) | (0xb<<16) | (0xa<<12) },
	{768, 	0x77700300 | (0x5) | (0xb<<16) | (0xb<<12) },
	{800, 	0x77700300 | (0x5) | (0xc<<16) | (0xb<<12) },
	{832, 	0x77700300 | (0x5) | (0xc<<16) | (0xc<<12) },
	{864, 	0x77700300 | (0x5) | (0xd<<16) | (0xc<<12) },
	{896, 	0x77700300 | (0x5) | (0xd<<16) | (0xd<<12) },
	{928, 	0x77700300 | (0x5) | (0xe<<16) | (0xd<<12) },
	{960, 	0x77700300 | (0x5) | (0xe<<16) | (0xe<<12) },
	{992, 	0x77700300 | (0x5) | (0xf<<16) | (0xe<<12) },
	{1024, 	0x77700300 | (0x5) | (0xf<<16) | (0xf<<12) },

	{1088, 	0x77700300 | (0x6) | (0x8<<16) | (0x7<<12) },
	{1152, 	0x77700300 | (0x6) | (0x8<<16) | (0x8<<12) },
	{1216, 	0x77700300 | (0x6) | (0x9<<16) | (0x8<<12) },
	{1280, 	0x77700300 | (0x6) | (0x9<<16) | (0x9<<12) },
	{1344, 	0x77700300 | (0x6) | (0xa<<16) | (0x9<<12) },
	{1408, 	0x77700300 | (0x6) | (0xa<<16) | (0xa<<12) },
	{1472, 	0x77700300 | (0x6) | (0xb<<16) | (0xa<<12) },
	{1536, 	0x77700300 | (0x6) | (0xb<<16) | (0xb<<12) },
	{1600, 	0x77700300 | (0x6) | (0xc<<16) | (0xb<<12) },
	{1664, 	0x77700300 | (0x6) | (0xc<<16) | (0xc<<12) },
	{1728, 	0x77700300 | (0x6) | (0xd<<16) | (0xc<<12) },
	{1792, 	0x77700300 | (0x6) | (0xd<<16) | (0xd<<12) },
	{1856, 	0x77700300 | (0x6) | (0xe<<16) | (0xd<<12) },
	{1920, 	0x77700300 | (0x6) | (0xe<<16) | (0xe<<12) },
	{1984, 	0x77700300 | (0x6) | (0xf<<16) | (0xe<<12) },
	{2048, 	0x77700300 | (0x6) | (0xf<<16) | (0xf<<12) },

	{2176, 	0x77700300 | (0x7) | (0x8<<16) | (0x7<<12) },
	{2304, 	0x77700300 | (0x7) | (0x8<<16) | (0x8<<12) },
	{2432, 	0x77700300 | (0x7) | (0x9<<16) | (0x8<<12) },
	{2560, 	0x77700300 | (0x7) | (0x9<<16) | (0x9<<12) },
	{2688, 	0x77700300 | (0x7) | (0xa<<16) | (0x9<<12) },
	{2816, 	0x77700300 | (0x7) | (0xa<<16) | (0xa<<12) },
	{2944, 	0x77700300 | (0x7) | (0xb<<16) | (0xa<<12) },
	{3072, 	0x77700300 | (0x7) | (0xb<<16) | (0xb<<12) },
#else
/* Divisor :      [3:0]    :   [18:16]:   [13:12] */
	{6, 		0x77700300 | (0x0) | (0x2<<16) | (0x2<<12) },
	{7, 		0x77700300 | (0x0) | (0x3<<16) | (0x2<<12) },
	{8, 		0x77700300 | (0x0) | (0x3<<16) | (0x3<<12) },	
	{9, 		0x77700300 | (0x0) | (0x4<<16) | (0x3<<12) },
	{10,		0x77700300 | (0x0) | (0x4<<16) | (0x4<<12) },
	{11,		0x77700300 | (0x0) | (0x5<<16) | (0x4<<12) },
	{12,		0x77700300 | (0x0) | (0x5<<16) | (0x5<<12) },
	{13,		0x77700300 | (0x0) | (0x6<<16) | (0x5<<12) },
	{14,		0x77700300 | (0x0) | (0x6<<16) | (0x6<<12) },
	{15,		0x77700300 | (0x0) | (0x7<<16) | (0x6<<12) },
	{16,		0x77700300 | (0x0) | (0x7<<16) | (0x7<<12) },
	
	{18,		0x77700300 | (0x1) | (0x4<<16) | (0x3<<12) },
	{20,		0x77700300 | (0x1) | (0x4<<16) | (0x4<<12) },
	{22,		0x77700300 | (0x1) | (0x5<<16) | (0x4<<12) },
	{24,		0x77700300 | (0x1) | (0x5<<16) | (0x5<<12) },
	{26,		0x77700300 | (0x1) | (0x6<<16) | (0x5<<12) },
	{28,		0x77700300 | (0x1) | (0x6<<16) | (0x6<<12) },
	{30,		0x77700300 | (0x1) | (0x7<<16) | (0x6<<12) },
	{32,		0x77700300 | (0x1) | (0x7<<16) | (0x7<<12) },
	
	{36,		0x77700300 | (0x2) | (0x4<<16) | (0x3<<12) },
	{40,		0x77700300 | (0x2) | (0x4<<16) | (0x4<<12) },
	{44,		0x77700300 | (0x2) | (0x5<<16) | (0x4<<12) },
	{48,		0x77700300 | (0x2) | (0x5<<16) | (0x5<<12) },
	{52,		0x77700300 | (0x2) | (0x6<<16) | (0x5<<12) },
	{56,		0x77700300 | (0x2) | (0x6<<16) | (0x6<<12) },
	{60,		0x77700300 | (0x2) | (0x7<<16) | (0x6<<12) },
	{64,		0x77700300 | (0x2) | (0x7<<16) | (0x7<<12) },

	{72,		0x77700300 | (0x3) | (0x4<<16) | (0x3<<12) },
	{80,		0x77700300 | (0x3) | (0x4<<16) | (0x4<<12) },
	{88,		0x77700300 | (0x3) | (0x5<<16) | (0x4<<12) },
	{96,		0x77700300 | (0x3) | (0x5<<16) | (0x5<<12) },
	{104,	0x77700300 | (0x3) | (0x6<<16) | (0x5<<12) },
	{112,	0x77700300 | (0x3) | (0x6<<16) | (0x6<<12) },
	{120,	0x77700300 | (0x3) | (0x7<<16) | (0x6<<12) },
	{128,	0x77700300 | (0x3) | (0x7<<16) | (0x7<<12) },

	{144,	0x77700300 | (0x4) | (0x4<<16) | (0x3<<12) },
	{160,	0x77700300 | (0x4) | (0x4<<16) | (0x4<<12) },
	{176,	0x77700300 | (0x4) | (0x5<<16) | (0x4<<12) },
	{192,	0x77700300 | (0x4) | (0x5<<16) | (0x5<<12) },
	{208,	0x77700300 | (0x4) | (0x6<<16) | (0x5<<12) },
	{224,	0x77700300 | (0x4) | (0x6<<16) | (0x6<<12) },
	{240,	0x77700300 | (0x4) | (0x7<<16) | (0x6<<12) },
	{256,	0x77700300 | (0x4) | (0x7<<16) | (0x7<<12) },

	{288,	0x77700300 | (0x5) | (0x4<<16) | (0x3<<12) },
	{320,	0x77700300 | (0x5) | (0x4<<16) | (0x4<<12) },
	{352,	0x77700300 | (0x5) | (0x5<<16) | (0x4<<12) },
	{384,	0x77700300 | (0x5) | (0x5<<16) | (0x5<<12) },
	{416,	0x77700300 | (0x5) | (0x6<<16) | (0x5<<12) },
	{448,	0x77700300 | (0x5) | (0x6<<16) | (0x6<<12) },
	{480,	0x77700300 | (0x5) | (0x7<<16) | (0x6<<12) },
	{512,	0x77700300 | (0x5) | (0x7<<16) | (0x7<<12) },

	{576,	0x77700300 | (0x6) | (0x4<<16) | (0x3<<12) },
	{640,	0x77700300 | (0x6) | (0x4<<16) | (0x4<<12) },
	{704,	0x77700300 | (0x6) | (0x5<<16) | (0x4<<12) },
	{768,	0x77700300 | (0x6) | (0x5<<16) | (0x5<<12) },
	{832,	0x77700300 | (0x6) | (0x6<<16) | (0x5<<12) },
	{896,	0x77700300 | (0x6) | (0x6<<16) | (0x6<<12) },
	{960,	0x77700300 | (0x6) | (0x7<<16) | (0x6<<12) },
	{1024,	0x77700300 | (0x6) | (0x7<<16) | (0x7<<12) },

	{1152,	0x77700300 | (0x7) | (0x4<<16) | (0x3<<12) },
	{1280,	0x77700300 | (0x7) | (0x4<<16) | (0x4<<12) },
	{1408,	0x77700300 | (0x7) | (0x5<<16) | (0x4<<12) },
	{1536,	0x77700300 | (0x7) | (0x5<<16) | (0x5<<12) },
	{1664,	0x77700300 | (0x7) | (0x6<<16) | (0x5<<12) },
	{1792,	0x77700300 | (0x7) | (0x6<<16) | (0x6<<12) },
	{1920,	0x77700300 | (0x7) | (0x7<<16) | (0x6<<12) },
	{2048,	0x77700300 | (0x7) | (0x7<<16) | (0x7<<12) },

	{2304,	0x77700300 | (0x8) | (0x4<<16) | (0x3<<12) },
	{2560,	0x77700300 | (0x8) | (0x4<<16) | (0x4<<12) },
	{2816,	0x77700300 | (0x8) | (0x5<<16) | (0x4<<12) },
	{3072,	0x77700300 | (0x8) | (0x5<<16) | (0x5<<12) },
	{3328,	0x77700300 | (0x8) | (0x6<<16) | (0x5<<12) },
	{3584,	0x77700300 | (0x8) | (0x6<<16) | (0x6<<12) },
	{3840,	0x77700300 | (0x8) | (0x7<<16) | (0x6<<12) },
	{4096,	0x77700300 | (0x8) | (0x7<<16) | (0x7<<12) },

	{4608,	0x77700300 | (0x9) | (0x4<<16) | (0x3<<12) },
	{5120,	0x77700300 | (0x9) | (0x4<<16) | (0x4<<12) },
	{5632,	0x77700300 | (0x9) | (0x5<<16) | (0x4<<12) },
	{6144,	0x77700300 | (0x9) | (0x5<<16) | (0x5<<12) },
	{6656,	0x77700300 | (0x9) | (0x6<<16) | (0x5<<12) },
	{7168,	0x77700300 | (0x9) | (0x6<<16) | (0x6<<12) },
	{7680,	0x77700300 | (0x9) | (0x7<<16) | (0x6<<12) },
	{8192,	0x77700300 | (0x9) | (0x7<<16) | (0x7<<12) },

	{9216,	0x77700300 | (0xa) | (0x4<<16) | (0x3<<12) },
	{10240,	0x77700300 | (0xa) | (0x4<<16) | (0x4<<12) },
	{11264,	0x77700300 | (0xa) | (0x5<<16) | (0x4<<12) },
	{12288,	0x77700300 | (0xa) | (0x5<<16) | (0x5<<12) },
	{13312,	0x77700300 | (0xa) | (0x6<<16) | (0x5<<12) },
	{14336,	0x77700300 | (0xa) | (0x6<<16) | (0x6<<12) },
	{15360,	0x77700300 | (0xa) | (0x7<<16) | (0x6<<12) },
	{16384,	0x77700300 | (0xa) | (0x7<<16) | (0x7<<12) },

	{18432,	0x77700300 | (0xb) | (0x4<<16) | (0x3<<12) },
	{20480,	0x77700300 | (0xb) | (0x4<<16) | (0x4<<12) },
	{22528,	0x77700300 | (0xb) | (0x5<<16) | (0x4<<12) },
	{24576,	0x77700300 | (0xb) | (0x5<<16) | (0x5<<12) },
	{26624,	0x77700300 | (0xb) | (0x6<<16) | (0x5<<12) },
	{28672,	0x77700300 | (0xb) | (0x6<<16) | (0x6<<12) },
	{30720,	0x77700300 | (0xb) | (0x7<<16) | (0x6<<12) },
	{32768,	0x77700300 | (0xb) | (0x7<<16) | (0x7<<12) },

	{36864, 0x77700300 | (0xc) | (0x4<<16) | (0x3<<12) },
	{40960, 0x77700300 | (0xc) | (0x4<<16) | (0x4<<12) },
	{45056, 0x77700300 | (0xc) | (0x5<<16) | (0x4<<12) },
	{49152, 0x77700300 | (0xc) | (0x5<<16) | (0x5<<12) },
	{53248, 0x77700300 | (0xc) | (0x6<<16) | (0x5<<12) },
	{57344, 0x77700300 | (0xc) | (0x6<<16) | (0x6<<12) },
	{61440, 0x77700300 | (0xc) | (0x7<<16) | (0x6<<12) },
	{65536, 0x77700300 | (0xc) | (0x7<<16) | (0x7<<12) },

	{73728, 0x77700300 | (0xd) | (0x4<<16) | (0x3<<12) },
	{81920, 0x77700300 | (0xd) | (0x4<<16) | (0x4<<12) },
	{90112, 0x77700300 | (0xd) | (0x5<<16) | (0x4<<12) },
	{98304, 0x77700300 | (0xd) | (0x5<<16) | (0x5<<12) },
	{106496, 0x77700300 | (0xd) | (0x6<<16) | (0x5<<12) },
	{114688, 0x77700300 | (0xd) | (0x6<<16) | (0x6<<12) },
	{122880, 0x77700300 | (0xd) | (0x7<<16) | (0x6<<12) },
	{131072, 0x77700300 | (0xd) | (0x7<<16) | (0x7<<12) },

	{147456, 0x77700300 | (0xe) | (0x4<<16) | (0x3<<12) },
	{163840, 0x77700300 | (0xe) | (0x4<<16) | (0x4<<12) },
	{180224, 0x77700300 | (0xe) | (0x5<<16) | (0x4<<12) },
	{196608, 0x77700300 | (0xe) | (0x5<<16) | (0x5<<12) },
	{212992, 0x77700300 | (0xe) | (0x6<<16) | (0x5<<12) },
	{229376, 0x77700300 | (0xe) | (0x6<<16) | (0x6<<12) },
	{245760, 0x77700300 | (0xe) | (0x7<<16) | (0x6<<12) },
	{262144, 0x77700300 | (0xe) | (0x7<<16) | (0x7<<12) },

	{294912, 0x77700300 | (0xf) | (0x4<<16) | (0x3<<12) },
	{327680, 0x77700300 | (0xf) | (0x4<<16) | (0x4<<12) },
	{360448, 0x77700300 | (0xf) | (0x5<<16) | (0x4<<12) },
	{393216, 0x77700300 | (0xf) | (0x5<<16) | (0x5<<12) },
	{425984, 0x77700300 | (0xf) | (0x6<<16) | (0x5<<12) },
	{458752, 0x77700300 | (0xf) | (0x6<<16) | (0x6<<12) },
	{491520, 0x77700300 | (0xf) | (0x7<<16) | (0x6<<12) },
	{524288, 0x77700300 | (0xf) | (0x7<<16) | (0x7<<12) },	
#endif	
};		

static inline void
ast_i2c_write(struct ast_i2c_bus *i2c_bus, u32 val, u32 reg)
{
#if 0
	printf("%x: W : reg %x , val: %x \n",i2c_bus->reg_base, reg, val);
#endif	
	__raw_writel(val, i2c_bus->reg_base + reg);
}

static inline u32
ast_i2c_read(struct ast_i2c_bus *i2c_bus, u32 reg)
{
#if 0
	u32 val = __raw_readl(i2c_bus->reg_base + reg);
	printf("%x: %x, R : reg %x , val: %x \n",i2c_bus->reg_base, reg, val);
	return val;
#else	
	return __raw_readl(i2c_bus->reg_base + reg);
#endif
}

static u32 select_i2c_clock(unsigned int bus_clk)
{
#if 0
	unsigned int clk, inc = 0, div, divider_ratio;
	u32 SCL_Low, SCL_High, data;

	clk = ast_get_pclk();
//	debug("pclk = %d \n",clk);
	divider_ratio = clk / bus_clk;
	for (div = 0; divider_ratio >= 16; div++)
	{
		inc |= (divider_ratio & 1);
		divider_ratio >>= 1;
	}
	divider_ratio += inc;
	SCL_Low = (divider_ratio >> 1) - 1;
	SCL_High = divider_ratio - SCL_Low - 2;
	data = 0x77700300 | (SCL_High << 16) | (SCL_Low << 12) | div;
//	printk("I2CD04 for %d = %08X\n", target_speed, data);
	return data;
#else
	int i;
	unsigned int clk;
	u32 data;

	clk = ast_get_pclk();
//	debug("pclk = %d \n",clk);

	for(i = 0; i < sizeof(i2c_timing_table)/sizeof(struct ast_i2c_timing_table); i++) {
		if((clk / i2c_timing_table[i].divisor) < bus_clk) {
			break;
		}
	}
	data = i2c_timing_table[i].timing;
//	printk("divisor [%d], timing [%x] \n", i2c_timing_table[i].divisor, i2c_timing_table[i].timing);
	return data;
#endif

}

static int ast_i2c_xfer(struct ast_i2c_bus *i2c_bus)
{
	int sts, i;
	int timeout;

	//Clear Interrupt
	ast_i2c_write(i2c_bus, 0xfffffff, I2C_INTR_STS_REG);

	/*
	 * Wait for the bus to become free.
	 */
	if(ast_i2c_read(i2c_bus,I2C_CMD_REG) & AST_I2CD_BUS_BUSY_STS) {
		printf("Bus busy \n");
		return 1;
	}

	//first start 
	debug(" %sing %d byte%s %s 0x%02x\n",
					i2c_bus->flags ? "read" : "write",
					i2c_bus->d_len, i2c_bus->d_len > 1 ? "s" : "",
					i2c_bus->flags ? "from" : "to", i2c_bus->addr);


	if(i2c_bus->flags)  {
		//READ
		if(i2c_bus->a_len) {
			//send start 
			ast_i2c_write(i2c_bus, (i2c_bus->addr <<1), I2C_BYTE_BUF_REG);
			ast_i2c_write(i2c_bus, AST_I2CD_M_TX_CMD | AST_I2CD_M_START_CMD, I2C_CMD_REG);
			
			///Wait ACK
			timeout = 0;
			while(1) {
				sts = ast_i2c_read(i2c_bus,I2C_INTR_STS_REG);
				if(timeout > 150) {
					//I2CG Reset		
					ast_i2c_write(i2c_bus, 0, I2C_FUN_CTRL_REG);
					ast_i2c_write(i2c_bus, AST_I2CD_MASTER_EN, I2C_FUN_CTRL_REG);
					return 1;
				} else if(sts & AST_I2CD_INTR_STS_TX_NAK) {
					ast_i2c_write(i2c_bus, AST_I2CD_INTR_STS_TX_NAK, I2C_INTR_STS_REG);
					ast_i2c_write(i2c_bus, AST_I2CD_M_STOP_CMD, I2C_CMD_REG);
					while(!(ast_i2c_read(i2c_bus,I2C_INTR_STS_REG) & AST_I2CD_INTR_STS_NORMAL_STOP));
					return 1; 
				} else if (sts & AST_I2CD_INTR_STS_TX_ACK) {
					ast_i2c_write(i2c_bus, AST_I2CD_INTR_STS_TX_ACK, I2C_INTR_STS_REG);
					break;
				} else {
					timeout++;
				}
				udelay(1000);
			}

			//Send Offset 
			for(i=0;i< i2c_bus->a_len;i++) {
				debug("offset [%x] \n",i2c_bus->a_buf[i]);
				ast_i2c_write(i2c_bus, i2c_bus->a_buf[i], I2C_BYTE_BUF_REG);
				ast_i2c_write(i2c_bus, AST_I2CD_M_TX_CMD, I2C_CMD_REG);
				while(!(ast_i2c_read(i2c_bus,I2C_INTR_STS_REG) & AST_I2CD_INTR_STS_TX_ACK));
				ast_i2c_write(i2c_bus, AST_I2CD_INTR_STS_TX_ACK, I2C_INTR_STS_REG);
			}
		} 

		//repeat-start 	
		ast_i2c_write(i2c_bus, (i2c_bus->addr <<1) |0x1, I2C_BYTE_BUF_REG);
		ast_i2c_write(i2c_bus, AST_I2CD_M_TX_CMD | AST_I2CD_M_START_CMD, I2C_CMD_REG);
		while(!(ast_i2c_read(i2c_bus,I2C_INTR_STS_REG) & AST_I2CD_INTR_STS_TX_ACK));
		ast_i2c_write(i2c_bus, AST_I2CD_INTR_STS_TX_ACK, I2C_INTR_STS_REG);

		for(i=0;i< i2c_bus->d_len;i++) {
			if(i == (i2c_bus->d_len - 1)) {
				ast_i2c_write(i2c_bus, AST_I2CD_M_RX_CMD | AST_I2CD_M_S_RX_CMD_LAST, I2C_CMD_REG);
			} else {
				ast_i2c_write(i2c_bus, AST_I2CD_M_RX_CMD, I2C_CMD_REG);
			}
			//BUG fix : workaround 
#if 0
			while(!(ast_i2c_read(i2c_bus,I2C_INTR_STS_REG) & AST_I2CD_INTR_STS_RX_DOWN)); 
			mdelay(1);
#else 
			do {
				sts = ast_i2c_read(i2c_bus,I2C_INTR_STS_REG);
			} while (sts != AST_I2CD_INTR_STS_RX_DOWN);
#endif			
			ast_i2c_write(i2c_bus, AST_I2CD_INTR_STS_RX_DOWN, I2C_INTR_STS_REG);
			i2c_bus->d_buf[i] = (ast_i2c_read(i2c_bus, I2C_BYTE_BUF_REG) & AST_I2CD_RX_BYTE_BUFFER) >> 8;
			
		}
		ast_i2c_write(i2c_bus, AST_I2CD_M_STOP_CMD, I2C_CMD_REG);
		
	} else {
		//Write 
		//send start 
		ast_i2c_write(i2c_bus, (i2c_bus->addr <<1), I2C_BYTE_BUF_REG);
		ast_i2c_write(i2c_bus, AST_I2CD_M_TX_CMD | AST_I2CD_M_START_CMD, I2C_CMD_REG);
		
		///Wait ACK
		timeout = 0;
		while(1) {
			sts = ast_i2c_read(i2c_bus,I2C_INTR_STS_REG);
			if(timeout > 100) {
				return 1;
			} else if(sts & AST_I2CD_INTR_STS_TX_NAK) {
				ast_i2c_write(i2c_bus, AST_I2CD_INTR_STS_TX_NAK, I2C_INTR_STS_REG);
				ast_i2c_write(i2c_bus, AST_I2CD_M_STOP_CMD, I2C_CMD_REG);
				return 1; 
			} else if (sts & AST_I2CD_INTR_STS_TX_ACK) {
				ast_i2c_write(i2c_bus, AST_I2CD_INTR_STS_TX_ACK, I2C_INTR_STS_REG);
				break;
			} else {
				timeout++;
			}
			udelay(1000);
		}
		
		//Send Offset 
		for(i=0;i< i2c_bus->a_len;i++) {
			debug("offset [%x] \n",i2c_bus->a_buf[i]);
			ast_i2c_write(i2c_bus, i2c_bus->a_buf[i], I2C_BYTE_BUF_REG);
			ast_i2c_write(i2c_bus, AST_I2CD_M_TX_CMD, I2C_CMD_REG);
			while(!(ast_i2c_read(i2c_bus,I2C_INTR_STS_REG) & AST_I2CD_INTR_STS_TX_ACK));
			ast_i2c_write(i2c_bus, AST_I2CD_INTR_STS_TX_ACK, I2C_INTR_STS_REG);
		}

		//Tx data
		for(i=0;i< i2c_bus->d_len;i++) {
			debug("Tx data [%x] \n",i2c_bus->d_buf[i]);
			ast_i2c_write(i2c_bus, i2c_bus->d_buf[i], I2C_BYTE_BUF_REG);
			ast_i2c_write(i2c_bus, AST_I2CD_M_TX_CMD, I2C_CMD_REG);
			while(!(ast_i2c_read(i2c_bus,I2C_INTR_STS_REG) & AST_I2CD_INTR_STS_TX_ACK));
			ast_i2c_write(i2c_bus, AST_I2CD_INTR_STS_TX_ACK, I2C_INTR_STS_REG);
		}
		ast_i2c_write(i2c_bus, AST_I2CD_M_STOP_CMD, I2C_CMD_REG);
	}

	
	//Clear Interrupt
	
	do {
		sts = ast_i2c_read(i2c_bus,I2C_INTR_STS_REG);
	} while (sts != AST_I2CD_INTR_STS_NORMAL_STOP);

	ast_i2c_write(i2c_bus, 0xfffffff, I2C_INTR_STS_REG);
				
	return 0;

}

/*****************************************************************************/

unsigned int i2c_get_bus_speed(void)
{
	return ast_i2c[i2c_bus_num].speed;
}

int i2c_set_bus_speed(unsigned int speed)
{
	struct ast_i2c_bus *i2c_bus = &ast_i2c[i2c_bus_num];

	/* Set AC Timing */
	ast_i2c_write(i2c_bus, select_i2c_clock(speed), I2C_AC_TIMING_REG1);
	ast_i2c_write(i2c_bus, AST_NO_TIMEOUT_CTRL, I2C_AC_TIMING_REG2);

	i2c_bus->speed = speed;

	return 0;
}

unsigned int i2c_get_base(int bus_no) {
	switch(bus_no) {
		case 0:
			return AST_I2C_DEV0_BASE;
			break;
		case 1:
			return AST_I2C_DEV1_BASE;
			break;
		case 2:
			return AST_I2C_DEV2_BASE;
			break;
		case 3:
			return AST_I2C_DEV3_BASE;
			break;
		case 4:
			return AST_I2C_DEV4_BASE;
			break;
		case 5:
			return AST_I2C_DEV5_BASE;
			break;
		case 6:
			return AST_I2C_DEV6_BASE;
			break;
		case 7:
			return AST_I2C_DEV7_BASE;
			break;
		case 8:
			return AST_I2C_DEV8_BASE;
			break;
		case 9:
			return AST_I2C_DEV9_BASE;
			break;		
		case 10:
			return AST_I2C_DEV10_BASE;
			break;
		case 11:
			return AST_I2C_DEV11_BASE;
			break;
		case 12:
			return AST_I2C_DEV12_BASE;
			break;
		case 13:
			return AST_I2C_DEV13_BASE;
			break;
		default:
			printf("i2c base error \n");
			break;
	};
	return 0;
}

void i2c_init(int speed, int slaveaddr)
{
	int i=0;
	struct ast_i2c_bus *i2c_bus;

	//SCU I2C Reset 
	ast_scu_init_i2c();

	/* This will override the speed selected in the fdt for that port */
	debug("i2c_init(speed=%u, slaveaddr=0x%x)\n", speed, slaveaddr);

	for(i = 0; i < CONFIG_SYS_MAX_I2C_BUS; i++) {
		i2c_bus = &ast_i2c[i];
		i2c_bus->reg_base = i2c_get_base(i);

		i2c_bus->speed = CONFIG_SYS_I2C_SPEED;
		i2c_bus->state = 0;

		//I2C Multi-Pin
		ast_scu_multi_func_i2c(i);

		//I2CG Reset 		
		ast_i2c_write(i2c_bus, 0, I2C_FUN_CTRL_REG);

		//Enable Master Mode
		ast_i2c_write(i2c_bus, AST_I2CD_MASTER_EN, I2C_FUN_CTRL_REG);

		//SLAVE mode enable 
#if 0	
		if(slaveaddr) {
			ast_i2c_write(i2c_bus, slaveaddr, I2C_DEV_ADDR_REG);	
			ast_i2c_write(i2c_bus, ast_i2c_read(i2c_bus,I2C_FUN_CTRL_REG) | AST_I2CD_SLAVE_EN, I2C_FUN_CTRL_REG);
		}	
#endif

		/* Set AC Timing */
		i2c_set_bus_speed(speed);

		//Clear Interrupt
		ast_i2c_write(i2c_bus, 0xfffffff, I2C_INTR_STS_REG);

		/* Set interrupt generation of I2C controller */
		ast_i2c_write(i2c_bus, 0, I2C_INTR_CTRL_REG);

	}

	i2c_bus_num = 0;
	debug("end\n");

}

/* Probe to see if a chip is present. */
int i2c_probe(uchar addr)
{
	uchar a_buf[1] = {0};
	
	struct ast_i2c_bus *i2c_bus = &ast_i2c[i2c_bus_num];

	debug("i2c_probe[bus:%d]: addr=0x%x\n", i2c_bus_num, addr);

	i2c_bus->addr = addr;
	i2c_bus->flags = 1;
	i2c_bus->a_len = 1;
	i2c_bus->a_buf = (u8 *)&a_buf;
	i2c_bus->d_len = 1;
	i2c_bus->d_buf = (u8 *)&a_buf;
	
	return ast_i2c_xfer(i2c_bus);
}

/* Read bytes */
int i2c_read(uchar addr, uint offset, int alen, uchar *buffer, int len)
{
	uchar xoffset[4];	
	struct ast_i2c_bus *i2c_bus = &ast_i2c[i2c_bus_num];

	debug("i2c_read[bus:%d]: addr=0x%x, offset=0x%x, alen=0x%x len=0x%x\n",
				i2c_bus_num, addr, offset, alen, len);

	if (alen > 4) {
		debug("I2C read: addr len %d not supported\n", alen);
		return 1;
	}

	if (alen > 0) {
		xoffset[0] = (offset >> 24) & 0xFF;
		xoffset[1] = (offset >> 16) & 0xFF;
		xoffset[2] = (offset >> 8) & 0xFF;
		xoffset[3] = offset & 0xFF;
	}

	i2c_bus->addr = addr;
	i2c_bus->flags = 1;
	i2c_bus->a_len = alen;
	i2c_bus->a_buf = &xoffset[4 - alen];
	i2c_bus->d_len = len;
	i2c_bus->d_buf = buffer;
	
	return ast_i2c_xfer(i2c_bus);
}

/* Write bytes */
int i2c_write(uchar addr, uint offset, int alen, uchar *buffer, int len)
{
	uchar xoffset[4];		
	struct ast_i2c_bus *i2c_bus = &ast_i2c[i2c_bus_num];

	debug("i2c_write[bus:%d]: addr=0x%x, offset=0x%x, alen=0x%x len=0x%x\n",
				i2c_bus_num, addr, offset, alen, len);

	if (alen > 0) {
		xoffset[0] = (offset >> 24) & 0xFF;
		xoffset[1] = (offset >> 16) & 0xFF;
		xoffset[2] = (offset >> 8) & 0xFF;
		xoffset[3] = offset & 0xFF;
	}
	
	i2c_bus->addr = addr;
	i2c_bus->flags = 0;
	i2c_bus->a_len = alen;
	i2c_bus->a_buf = &xoffset[4 - alen];
	i2c_bus->d_len = len;
	i2c_bus->d_buf = buffer;
	
	return ast_i2c_xfer(i2c_bus);
}

#if defined(CONFIG_I2C_MULTI_BUS)
/*
 * Functions for multiple I2C bus handling
 */
unsigned int i2c_get_bus_num(void)
{
	return i2c_bus_num;
}

int i2c_set_bus_num(unsigned int bus)
{
	if (bus >= NUM_BUS)
		return -1;
	i2c_bus_num = bus;

	return 0;
}
#endif

