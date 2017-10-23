/*******************************************************************************
 * File Name     : arch/arm/mach-aspeed/ast-ahbc.c
 * Author         : Ryan Chen
 * Description   : AST AHB Ctrl
 *
 * Copyright (C) 2012-2020  ASPEED Technology Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 *
 *   History      :
 *    1. 2014/03/15 Ryan Chen Create
 *
 ******************************************************************************/
#include <common.h>
#include <asm/processor.h>
#include <asm/io.h>
#include <asm/arch/ast-ahbc.h>
#include <asm/arch/aspeed.h>

/***********************  Registers for AHBC ***************************/
#define AST_AHBC_PROTECT		0x00	/* Protection Key Register */
#define AST_AHBC_PRIORITY_CTRL		0x80	/* Priority Cortrol Register */
#define AST_AHBC_ADDR_REMAP		0x8C	/* Address Remapping Register */

/* AST_AHBC_PROTECT	0x00		Protection Key Register 	*/
#define AHBC_PROTECT_UNLOCK		0xAEED1A03

/* AST_AHBC_ADDR_REMAP	0x8C		Address Remapping Register */
#define AHBC_PCI_REMAP1			(1 << 5)
#define AHBC_PCI_REMAP0			(1 << 4)

#if defined(AST_SOC_G5)
#define AHBC_PCIE_MAP			(1 << 5)
#define AHBC_LPC_PLUS_MAP		(1 << 4)
#else
#define AHBC_BOOT_REMAP			1
#endif
/**************************************************************/

//#define AST_AHBC_DEBUG

#ifdef AST_AHBC_DEBUG
#define AHBCDBUG(fmt, args...) printf("%s() " fmt, __FUNCTION__, ## args)
#else
#define AHBCDBUG(fmt, args...)
#endif

static u32 ast_ahbc_base = AST_AHBC_BASE;
static inline u32 
ast_ahbc_read(u32 reg)
{
	u32 val;
	val = readl(ast_ahbc_base + reg);
	AHBCDBUG("reg = 0x%08x, val = 0x%08x\n", reg, val);
	
	return val;
}

static inline void
ast_ahbc_write(u32 val, u32 reg) 
{
	AHBCDBUG("reg = 0x%08x, val = 0x%08x\n", reg, val);

#ifdef CONFIG_AST_AHBC_LOCK
	//unlock 
	writel(AHBC_PROTECT_UNLOCK, ast_ahbc_base);
	writel(val, ast_ahbc_base + reg);
	//lock
	writel(0xaa,ast_ahbc_base);	
#else
	writel(AHBC_PROTECT_UNLOCK, ast_ahbc_base);
	writel(val, ast_ahbc_base + reg);
#endif
}
void ast_ahbc_boot_remap(void)
{
#if ! defined(AST_SOC_G5)
	ast_ahbc_write(ast_ahbc_read(AST_AHBC_ADDR_REMAP) |
		       AHBC_BOOT_REMAP, AST_AHBC_ADDR_REMAP);
#endif
}

#ifdef AST_SOC_G5
void ast_ahbc_peie_mapping(u8 enable)
{
	if (enable)
		ast_ahbc_write(ast_ahbc_read(AST_AHBC_ADDR_REMAP) |
			       AHBC_PCIE_MAP, AST_AHBC_ADDR_REMAP);
	else
		ast_ahbc_write(ast_ahbc_read(AST_AHBC_ADDR_REMAP) &
			       ~AHBC_PCIE_MAP, AST_AHBC_ADDR_REMAP);
}

void ast_ahbc_lpc_plus_mapping(u8 enable)
{
	if(enable)
		ast_ahbc_write(ast_ahbc_read(AST_AHBC_ADDR_REMAP) |
			       AHBC_LPC_PLUS_MAP, AST_AHBC_ADDR_REMAP);
	else
		ast_ahbc_write(ast_ahbc_read(AST_AHBC_ADDR_REMAP) &
			       ~AHBC_LPC_PLUS_MAP, AST_AHBC_ADDR_REMAP);
}
#endif
