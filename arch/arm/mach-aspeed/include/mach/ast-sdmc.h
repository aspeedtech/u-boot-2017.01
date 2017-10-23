/*******************************************************************************
 * File Name     : arch/arm/mach-aspeed/include/plat/ast-sdmc.h
 * Author        : Ryan Chen
 * Description   : AST SDMC Header
 *
 * Copyright (C) 2012-2020  ASPEED Technology Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 *   History      :
 *      1. 2012/08/03 Ryan Chen create this file
 *
 ******************************************************************************/

#ifndef __AST_SDMC_H_INCLUDED
#define __AST_SDMC_H_INCLUDED

extern u32 ast_sdmc_get_mem_size(void);
extern u8 ast_sdmc_get_ecc_recover_count(void);
extern u8 ast_sdmc_get_ecc_unrecover_count(void);
extern u8 ast_sdmc_get_ecc(void);
extern u32 ast_sdmc_get_ecc_size(void);
extern u8 ast_sdmc_get_cache(void);
extern void ast_sdmc_disable_mem_protection(u8 req);
#endif
