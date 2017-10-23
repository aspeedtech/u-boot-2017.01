/*
 * Configuation settings for the ASPEED AST Chip.
 *
 * Copyright (C) 2012-2020 ASPEED Tech. Inc.
 * Ryan Chen (ryan_chen@aspeedtech.com)
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <spi.h>
#include <malloc.h>
#include <asm/io.h>
#include <spi_flash.h>

#include <asm/arch/ast-scu.h>
#include <asm/arch/aspeed.h>
#include "../mtd/spi/sf_internal.h"

/* Register definitions for the AST SPI Controller  AST-G4*/
#define AST_SPI_CONFIG			0x00
#define AST_SPI_CTRL				0x04
#define AST_SPI_DMA_STS			0x08
#define AST_SPI_MISC			0x10
#define AST_SPI_TIMING			0x14

/* AST_SPI_CONFIG 0x00 : SPI00 CE Type Setting Register */
#ifdef AST_FMC_SPI
#define SPI_CONF_CE1_WEN		(0x1 << 17)
#define SPI_CONF_CE0_WEN		(0x1 << 16)
#else
#define SPI_CONF_CE0_WEN		(0x1)
#endif

/* Register offsets */
#define FMC_SPI_CONFIG			0x00
#define FMC_SPI_CTRL			0x04
#define FMC_SPI_DMA_STS		0x08

#define FMC_SPI_CE0_CTRL		0x10
#define FMC_SPI_CE1_CTRL		0x14

#define AST_SPI_DMA_CTRL		0x80
#define AST_SPI_DMA_FLASH_BASE	0x84
#define AST_SPI_DMA_DRAM_BASE	0x88
#define AST_SPI_DMA_LENGTH	0x8c

/* AST_FMC_CONFIG 0x00 : FMC00 CE Type Setting Register */
#define FMC_CONF_LAGACY_DIS	(0x1 << 31)
#define FMC_CONF_CE1_WEN		(0x1 << 17)
#define FMC_CONF_CE0_WEN		(0x1 << 16)
#define FMC_CONF_CE1_SPI		(0x2 << 2)
#define FMC_CONF_CE0_SPI		(0x2)

/* FMC_SPI_CTRL	: 0x04 : FMC04 CE Control Register */
#define FMC_CTRL_CE1_4BYTE_MODE	(0x1 << 1)
#define FMC_CTRL_CE0_4BYTE_MODE	(0x1)

/* FMC_SPI_DMA_STS	: 0x08 : FMC08 Interrupt Control and Status Register */
#define FMC_STS_DMA_READY		0x0800
#define FMC_STS_DMA_CLEAR		0x0800

/* FMC_CE0_CTRL	for SPI 0x10, 0x14, 0x18, 0x1c, 0x20 */
#define SPI_IO_MODE_MASK		(3 << 28)
#define SPI_SINGLE_BIT			(0 << 28)
#define SPI_DUAL_MODE			(0x2 << 28)
#define SPI_DUAL_IO_MODE		(0x3 << 28)
#define SPI_QUAD_MODE			(0x4 << 28)
#define SPI_QUAD_IO_MODE		(0x5 << 28)

#define SPI_CE_WIDTH(x)			(x << 24)
#define SPI_CMD_DATA_MASK		(0xff << 16)
#define SPI_CMD_DATA(x)			(x << 16)
#define SPI_DUMMY_CMD			(1 << 15)
#define SPI_DUMMY_HIGH			(1 << 14)
//#define SPI_CLK_DIV				(1 << 13)		?? TODO ask....
//#define SPI_ADDR_CYCLE			(1 << 13)		?? TODO ask....
#define SPI_CMD_MERGE_DIS		(1 << 12)
#define SPI_CLK_DIV(x)			(x << 8)
#define SPI_CLK_DIV_MASK		(0xf << 8)

#define SPI_DUMMY_LOW_MASK	(0x3 << 6)
#define SPI_DUMMY_LOW(x)		((x) << 6)
#define SPI_LSB_FIRST_CTRL		(1 << 5)
#define SPI_CPOL_1				(1 << 4)
#define SPI_DUAL_DATA			(1 << 3)
#define SPI_CE_INACTIVE			(1 << 2)
#define SPI_CMD_MODE_MASK		(0x3)
#define SPI_CMD_NORMAL_READ_MODE		0
#define SPI_CMD_READ_CMD_MODE		1
#define SPI_CMD_WRITE_CMD_MODE		2
#define SPI_CMD_USER_MODE			3

/* AST_SPI_DMA_CTRL				0x80 */
#define FMC_DMA_ENABLE		(0x1)

struct ast_spi_host {
	struct spi_slave slave;
	void		*base;
	void		*ctrl_regs;
	void 		*buff;
	u32 	(*get_clk)(void);
};

//#define AST_SPI_DEBUG
//#define AST_SPI_DATA_DEBUG

#ifdef AST_SPI_DEBUG
#define SPIBUG(fmt, args...) printf(fmt, ## args)
#else
#define SPIBUG(fmt, args...)
#endif

#ifdef AST_SPI_DATA_DEBUG
#define SPIDBUG(fmt, args...) printf(fmt, ## args)
#else
#define SPIDBUG(fmt, args...)
#endif

DECLARE_GLOBAL_DATA_PTR;

static inline struct ast_spi_host *to_ast_spi(struct spi_slave *slave)
{
	return container_of(slave, struct ast_spi_host, slave);
}

void spi_init(void)
{
	SPIBUG("spi_init iiii\n");
}

static u32 ast_spi_calculate_divisor(u32 max_speed_hz)
{
	// [0] ->15 : HCLK , HCLK/16
	u8 SPI_DIV[16] = {16, 7, 14, 6, 13, 5, 12, 4, 11, 3, 10, 2, 9, 1, 8, 0};
	u32 i, hclk, spi_cdvr=0;

	hclk = ast_get_ahbclk();
	for(i=1;i<17;i++) {
		if(max_speed_hz >= (hclk/i)) {
			spi_cdvr = SPI_DIV[i-1];
//			printf("hclk = %d , spi_cdvr = %d \n",hclk, spi_cdvr);
			break;
		}
	}

//	printf("hclk is %d, divisor is %d, target :%d , cal speed %d\n", hclk, spi_cdvr, max_speed_hz, hclk/SPI_DIV[i]);
	return spi_cdvr;
}

struct spi_slave *spi_setup_slave(unsigned int bus, unsigned int cs,
				  unsigned int max_hz, unsigned int mode)
{
	struct ast_spi_host	*ast_spi;
	u32 			fmc_config = FMC_CONF_LAGACY_DIS;
	u32 			spi_config = 0;
	u32			spi_ctrl;
	u32			div;

	SPIBUG("spi_setup_slave bus %d, cs %d, max_hz %d, mode %d\n", bus, cs, max_hz, mode);

	ast_spi = spi_alloc_slave(struct ast_spi_host, bus, cs);
	if (!ast_spi)
		return NULL;
	
	ast_spi->slave.bus = bus;
	ast_spi->slave.cs = cs;

#ifdef CONFIG_AST_SPI_QUAD
	//only support in G4 platform
	ast_spi->slave.mode = SPI_MODE_3 | SPI_RX_DUAL | SPI_RX_QUAD | SPI_TX_QUAD;
#else
	ast_spi->slave.mode = SPI_MODE_3 | SPI_RX_DUAL;
#endif

	switch(bus) {
		case 0:			
			ast_spi->base = (void *)AST_FMC_BASE;
			switch (cs) {
				case 0:
					fmc_config |= FMC_CONF_CE0_WEN | FMC_CONF_CE0_SPI;
					ast_spi->slave.memory_map = (void *)AST_FMC_CS0_BASE;
					ast_spi->ctrl_regs = (void *)AST_FMC_BASE + FMC_SPI_CE0_CTRL;
					ast_spi->buff = (void *)AST_FMC_CS0_BASE;
					break;
				case 1:
					*((volatile ulong*) 0x1e6e2088) |= (0x1 << 24); /* FWSPICS1# pin*/
					fmc_config |= FMC_CONF_CE1_WEN | FMC_CONF_CE1_SPI;
					ast_spi->slave.memory_map = (void *)AST_FMC_CS1_BASE;
					ast_spi->ctrl_regs = (void *)AST_FMC_BASE + FMC_SPI_CE1_CTRL;
					ast_spi->buff = (void *)AST_FMC_CS1_BASE;
					break;
				default:
					return 0;
					break;
			}
			/* enable Flash Write and select spi flash type*/
//			printf("read config %x , and set config %x \n", readl(ast_spi->base), fmc_config);
//			printf("read config 0x04 %x \n", readl(ast_spi->base + 0x04));
			writel(fmc_config, ast_spi->base);
			break;
		case 1:
			//AST-G5 use FMC, AST-G4 use SPI register 
			//SCU70  SPI master strap */
			ast_scu_spi_master(1);
#ifdef AST_FMC_SPI
			ast_spi->base = (void *)AST_FMC_SPI0_BASE;
#else
			ast_spi->base = (void *)AST_SPI0_BASE;
#endif
			switch (cs) {
				case 0:
					spi_config |= SPI_CONF_CE0_WEN;
#ifdef AST_FMC_SPI
					ast_spi->ctrl_regs = (void *)AST_FMC_SPI0_BASE + FMC_SPI_CE0_CTRL;
					ast_spi->slave.memory_map = (void *)AST_SPI0_CS0_BASE;
					ast_spi->buff = (void *)AST_SPI0_CS0_BASE;
#else
					ast_spi->ctrl_regs = (void *)AST_SPI0_BASE + FMC_SPI_CTRL;
					ast_spi->slave.memory_map = (void *)AST_SPI0_MEM;
					ast_spi->buff = (void *)AST_SPI0_MEM;
#endif
					break;
/* AST-G4 no CS1 */
#ifdef AST_SPI0_CS1_BASE
				case 1:
					spi_config |= SPI_CONF_CE1_WEN;
					ast_spi->ctrl_regs = (void *)AST_FMC_SPI0_BASE + FMC_SPI_CE1_CTRL;
					ast_spi->slave.memory_map = (void *)AST_SPI0_CS1_BASE;
					ast_spi->buff = (void *)AST_SPI0_CS1_BASE;
					break;
#endif
				default:
					return 0;
					break;
				
			}
			/* Flash Controller enable Flash Write */ 
			writel(spi_config, ast_spi->base);
			break;
#ifdef AST_FMC_SPI1_BASE			
		case 2:
			*((volatile ulong*) 0x1e6e2088) |= (0x7 << 27); /* SPI pin*/
			ast_spi->base = (void *)AST_FMC_SPI1_BASE;
			switch (cs) {
#ifdef AST_SPI1_CS0_BASE				
				case 0:
					*((volatile ulong*) 0x1e6e2088) |= (0x1 << 26); /* SPI pin*/
					spi_config |= SPI_CONF_CE0_WEN;
					ast_spi->ctrl_regs = (void *)AST_FMC_SPI1_BASE + FMC_SPI_CE0_CTRL;
					ast_spi->buff = (void *)AST_SPI1_CS0_BASE;
					break;
#endif					
#ifdef AST_SPI1_CS1_BASE
				case 1:
					*((volatile ulong*) 0x1e6e208c) |= (0x1); /* SPI pin*/
					spi_config |= SPI_CONF_CE0_WEN;
					ast_spi->ctrl_regs = (void *)AST_FMC_SPI1_BASE + FMC_SPI_CE1_CTRL;
					ast_spi->buff = (void *)AST_SPI1_CS1_BASE;
					break;
#endif
				default:
					return 0;
					break;
			}
			/* Flash Controller enable Flash Write */ 
			writel(spi_config, ast_spi->base);			
			break;
#endif			
	}

	/* AST2300 limit Max SPI CLK to 50MHz (Datasheet v1.2) */
	spi_ctrl = 0;//readl(ast_spi->ctrl_regs);

	//TODO MASK first
	spi_ctrl &= ~SPI_IO_MODE_MASK;
	
	spi_ctrl &= ~SPI_CLK_DIV_MASK;
//	spi_ctrl |= SPI_CLK_DIV(0x7);

	
	div = ast_spi_calculate_divisor(max_hz);
//	printf("max_hz %d, div: %x \n",max_hz, div);
	spi_ctrl |= SPI_CLK_DIV(div);	

//	if (SPI_CPOL & mode) 
//		spi_ctrl |= SPI_CPOL_1;
//	else
//		spi_ctrl &= ~SPI_CPOL_1;

	//ISSUE : ast spi ctrl couldn't use mode 3, so fix mode 0
	spi_ctrl &= ~SPI_CPOL_1;
	
	SPIBUG("ctrl reg %x, : %x \n", (u32)ast_spi->ctrl_regs, spi_ctrl);
	writel(spi_ctrl, ast_spi->ctrl_regs);


	return &ast_spi->slave;
}

void fmc_spi_read_config(struct ast_spi_host *ast_spi)
{
	struct spi_flash *flash = ast_spi->slave.flash;
	/* Look for read commands */
	SPIBUG("fmc_spi_read_config %x \n", flash->read_cmd);	
	switch(flash->read_cmd) {
		case CMD_READ_ARRAY_FAST:
			writel(readl(ast_spi->ctrl_regs) | SPI_CMD_READ_CMD_MODE, ast_spi->ctrl_regs);
			break;
		case CMD_READ_ARRAY_SLOW:
			//keep to nomal read [0x03]
			break;
		case CMD_READ_DUAL_OUTPUT_FAST:
		case CMD_READ_DUAL_OUTPUT_FAST_4B:
			writel(readl(ast_spi->ctrl_regs) | SPI_DUAL_MODE | SPI_CMD_READ_CMD_MODE, ast_spi->ctrl_regs);
			break;
		case CMD_READ_DUAL_IO_FAST:
		case CMD_READ_DUAL_IO_FAST_4B:
			writel(readl(ast_spi->ctrl_regs) | SPI_DUAL_IO_MODE | SPI_CMD_READ_CMD_MODE, ast_spi->ctrl_regs);
			break;					
		case CMD_READ_QUAD_IO_FAST:
		case CMD_READ_QUAD_IO_FAST_4B:
			writel(readl(ast_spi->ctrl_regs) | SPI_QUAD_IO_MODE | SPI_CMD_READ_CMD_MODE, ast_spi->ctrl_regs);
			break;
		case CMD_READ_QUAD_OUTPUT_FAST:
		case CMD_READ_QUAD_OUTPUT_FAST_4B:	
			writel(readl(ast_spi->ctrl_regs) | SPI_QUAD_MODE | SPI_CMD_READ_CMD_MODE, ast_spi->ctrl_regs);
			break;
		default:
			printf("unknow read cmd %x \n", flash->read_cmd);
			break;
	}
	SPIBUG("END yes flash %x \n", readl(ast_spi->ctrl_regs));	
}

int spi_xfer(struct spi_slave *slave, unsigned int bitlen, const void *dout,
	     void *din, unsigned long flags)
{
	struct ast_spi_host *ast_spi = to_ast_spi(slave);
	/* assume spi core configured to do 8 bit transfers */
	uint bytes = bitlen / 8;
	const uchar *txp = dout;
	uchar cmd = txp[0];
	uchar *rxp = din;
	struct spi_flash *flash = slave->flash;

	SPIBUG("%s: ctrl %x, bus:%i cs:%i bitlen:%i bytes:%i flags:%lx \n", __func__, readl(ast_spi->ctrl_regs),
		slave->bus, slave->cs, bitlen, bytes, flags);

	if (flags & SPI_XFER_MMAP) {
//		printf("SPI_XFER_MMAP \n: flash->write_cmd %x , flash->read_cmd %x \n", flash->write_cmd, flash->read_cmd);
//		printf("spi_xfer flags %x read cmd %x TODO ~~\n", flags, *txp);
		return 0;
	}

	if(flags & SPI_XFER_MMAP_END){
		SPIBUG("SPI_XFER_MMAP_END \n");
//		ast_spi_write(spi, ast_spi_read(spi, 0x00) & ~SPI_CE_HIGH, 0x00);
		return 0;
	}

	if (bitlen == 0)
		return -1;

	if (bitlen % 8) {
		debug("spi_xfer: Non byte aligned SPI transfer\n");
		return -1;
	}

	if (flags & SPI_XFER_BEGIN) {
		SPIDBUG("\n ----------Xfer BEGIN -------\n");		
		if(flash->name) {
			writel(readl(ast_spi->ctrl_regs) & ~SPI_IO_MODE_MASK, ast_spi->ctrl_regs);
		}
		writel((readl(ast_spi->ctrl_regs) | SPI_CMD_USER_MODE), ast_spi->ctrl_regs);
		writel(readl(ast_spi->ctrl_regs) & ~SPI_CE_INACTIVE, ast_spi->ctrl_regs);
#if 0
		if(cmd == CMD_QUAD_IO_PAGE_PROGRAM) {
			printf("CMD_QUAD_IO_PAGE_PROGRAM \n");
			//for cmd is nornal write 
			__raw_writeb(txp[0], ast_spi->buff);
			writel((readl(ast_spi->ctrl_regs) | SPI_QUAD_IO_MODE), ast_spi->ctrl_regs);
			bytes--;
			*txp++;
			//for address is quad io mode
		}
#endif
	}		
   	
	while (bytes--) {
		uchar d;
		if(txp) {
			d = txp ? *txp++ : 0xff;
			//SPIDBUG("%s: tx:%x ", __func__, d);
			__raw_writeb(d, ast_spi->buff);
		}
		//SPIDBUG("\n");
		if (rxp) {
			d = __raw_readb(ast_spi->buff);
			*rxp++ = d;
			//SPIDBUG("rx:%x ", d);
		}
	}
	//SPIDBUG("\n");
#if 0
	if ((flags & SPI_XFER_BEGIN) && (cmd == CMD_QUAD_PAGE_PROGRAM)) {
		//next xfer will be quad mode write
		writel(readl(ast_spi->ctrl_regs) | SPI_QUAD_MODE, ast_spi->ctrl_regs);
		printf("next is quad write data cmd %x write %x\n", cmd, flash->write_cmd);		
	}
#endif	
	if (flags & SPI_XFER_END) {
		SPIDBUG("\n ----------Xfer END -------\n");
		writel(readl(ast_spi->ctrl_regs) | SPI_CE_INACTIVE, ast_spi->ctrl_regs);
		writel(readl(ast_spi->ctrl_regs) & ~(SPI_CMD_USER_MODE), ast_spi->ctrl_regs);
		if(flash->name) {
			fmc_spi_read_config(ast_spi);
		}
	}

	return 0;
}

void spi_free_slave(struct spi_slave *slave)
{
	struct ast_spi_host *ast_spi = to_ast_spi(slave);
	SPIBUG("spi_free_slave slave->bus %d , slave->cs %d \n", slave->bus, slave->cs);
	free(ast_spi);
}

int spi_claim_bus(struct spi_slave *slave)
{
	//init controller
	struct ast_spi_host *ast_spi = to_ast_spi(slave);
	struct spi_flash *flash = slave->flash;
	u32 ctrl = readl(ast_spi->ctrl_regs) & ~(SPI_CMD_DATA_MASK | SPI_DUMMY_LOW_MASK | SPI_DUMMY_HIGH | SPI_IO_MODE_MASK | SPI_CMD_MODE_MASK);
	SPIBUG("spi_claim_bus ctrl %x, flash->write_cmd %x , flash->read_cmd %x flash->dummy_byte %d, 4 byte mode %d \n", readl(ast_spi->ctrl_regs), flash->write_cmd, flash->read_cmd, flash->dummy_byte, flash->bytemode);
	switch(slave->bus) {
		case 0:
			switch(slave->cs) {
				case 0:
					if(flash->bytemode)
						writel(readl(ast_spi->base + FMC_SPI_CTRL) | FMC_CTRL_CE0_4BYTE_MODE, ast_spi->base + FMC_SPI_CTRL);
					else
						writel(readl(ast_spi->base + FMC_SPI_CTRL) & ~FMC_CTRL_CE0_4BYTE_MODE, ast_spi->base + FMC_SPI_CTRL);					
					break;
				case 1:
					if(flash->bytemode)
						writel(readl(ast_spi->base + FMC_SPI_CTRL) | FMC_CTRL_CE1_4BYTE_MODE, ast_spi->base + FMC_SPI_CTRL);
					else
						writel(readl(ast_spi->base + FMC_SPI_CTRL) & ~FMC_CTRL_CE1_4BYTE_MODE, ast_spi->base + FMC_SPI_CTRL);					
					break;
			}
			break;
		case 1:
			switch(slave->cs) {
				case 0:
					if(flash->bytemode)
						writel(readl(ast_spi->base + FMC_SPI_CTRL) | FMC_CTRL_CE0_4BYTE_MODE, ast_spi->base + FMC_SPI_CTRL);
					else
						writel(readl(ast_spi->base + FMC_SPI_CTRL) & ~FMC_CTRL_CE0_4BYTE_MODE, ast_spi->base + FMC_SPI_CTRL);					
					break;
				case 1:
					if(flash->bytemode)
						writel(readl(ast_spi->base + FMC_SPI_CTRL) | FMC_CTRL_CE1_4BYTE_MODE, ast_spi->base + FMC_SPI_CTRL);
					else
						writel(readl(ast_spi->base + FMC_SPI_CTRL) & ~FMC_CTRL_CE1_4BYTE_MODE, ast_spi->base + FMC_SPI_CTRL);					
					break;
			}
			break;
	}
//	printf("read ctrl and mask %x \n", ctrl);
	if(!flash->name) {
		SPIBUG("Flash not yet \n");
		writel(ctrl, ast_spi->ctrl_regs);
	} else {
		SPIBUG("Flash is ok %s \n",flash->name);
		//for high bit dummy byte
		if(flash->dummy_byte & 0x4) 
			ctrl |= SPI_CMD_DATA(flash->read_cmd) | SPI_DUMMY_LOW(flash->dummy_byte & 0x3) | SPI_DUMMY_HIGH;
		else
			ctrl |= SPI_CMD_DATA(flash->read_cmd) | SPI_DUMMY_LOW(flash->dummy_byte & 0x3);		

		writel(ctrl, ast_spi->ctrl_regs);
		fmc_spi_read_config(ast_spi);
	}
	return 0;
}

void spi_release_bus(struct spi_slave *slave)
{
	SPIBUG("spi_release_bus\n");
	/* TODO: Shut the controller down */
}

/* TODO: control from sf layer to here through dm-spi */
void spi_flash_copy_mmap(void *data, void *offset, size_t len)
{	
	ulong tmp;

	SPIBUG("spi_flash_copy_mmap , data %x , offset %x, len %x[hex], ctrl reg %x\n", (u32)data, (u32)offset, len, readl(AST_FMC_BASE + 0x10));
	
	if(len < 4) printf("TODO Fix \n");
	if(len > 0x2000000) printf("TODO add dma bigger\n");
	/* 4-bytes align 0 : 4 byte */
	if(len % 4)
		len = len/4;
	else
		len = len/4 - 1;
	
      /* force end of burst read */
	*(ulong *) (AST_FMC_BASE + AST_SPI_DMA_CTRL) = (ulong) (~FMC_DMA_ENABLE);
	*(ulong *) (AST_FMC_BASE + AST_SPI_DMA_FLASH_BASE) = (ulong) (offset);
	*(ulong *) (AST_FMC_BASE + AST_SPI_DMA_DRAM_BASE) = (ulong) (data);
	*(ulong *) (AST_FMC_BASE + AST_SPI_DMA_LENGTH) = (ulong) (len);
	*(ulong *) (AST_FMC_BASE + AST_SPI_DMA_CTRL) = (ulong) (FMC_DMA_ENABLE);

	/* wait poll */
	do {
		udelay(100);
		tmp = *(ulong *) (AST_FMC_BASE + FMC_SPI_DMA_STS);
	} while (!(tmp & FMC_STS_DMA_READY));

	*(ulong *) (AST_FMC_BASE + AST_SPI_DMA_CTRL) &= ~(FMC_DMA_ENABLE);
	/* clear status */
	*(ulong *) (AST_FMC_BASE + FMC_SPI_DMA_STS) |= FMC_STS_DMA_CLEAR;

}
