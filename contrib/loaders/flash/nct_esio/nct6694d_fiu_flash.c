// SPDX-License-Identifier: GPL-2.0-or-later

#include <stdint.h>
#include <string.h>
#include "nct6694d_flash.h"

/* flashloader parameter structure */
__attribute__ ((section(".buffers.g_cfg")))
static volatile struct nct_esio_flash_params g_cfg;

/* data buffer */
__attribute__ ((section(".buffers.g_buf")))
static volatile uint8_t g_buf[0x1000];

static void delay(uint32_t i)
{
	while (i--)
		__asm__ volatile ("nop");
}

static void gdma_memcpy_u8(uint8_t* dst, uint8_t* src, uint32_t cpylen)
{
	if (cpylen == 0) return;

	GDMA_SRCB0 = (uint32_t)src;
	GDMA_DSTB0 = (uint32_t)dst;
	GDMA_TCNT0 = cpylen;
	GDMA_CTL0 = 0x10001;

	while (GDMA_CTL0 & 0x1);
	GDMA_CTL0 = 0;
}

static void gdma_memcpy_burst_u32(uint8_t* dst, uint8_t* src, uint32_t cpylen)
{
	uint32_t rlen;

	if (cpylen == 0) return;

	/* src and dst address must 16byte aligned */
	if (((uint32_t)src & 0x0F) || ((uint32_t)dst & 0xF)) {
		gdma_memcpy_u8(dst, src, cpylen);
		return;
	}

	/* aligned 64Byte length */
	rlen = cpylen & 0xFFFFFFC0;
	if (rlen) {
		FIU0->BURST_CFG = 0x0B;

		GDMA_SRCB0 = (uint32_t)src;
		GDMA_DSTB0 = (uint32_t)dst;
		GDMA_TCNT0 = rlen/16;
		GDMA_CTL0 = 0x12201;

		while(GDMA_CTL0 & 0x1);
		GDMA_CTL0 = 0;

		FIU0->BURST_CFG = 0x03;

		src += rlen;
		dst += rlen;
		cpylen -= rlen;
	}

	/* remain length */
	if(cpylen) {
		gdma_memcpy_u8(dst, src, cpylen);
	}
}

static uint8_t fiu_read(enum flash_port port, uint32_t addr, uint8_t* buff, uint32_t len, uint8_t adr_4b)
{
	FIU_Quad_Mode_Disable;
	if (adr_4b) {
	FIU_Set_4B_CMD_EN(port, FLASH_CMD_4BYTE_READ);
	} else {
	FIU_Clr_4B_CMD_EN(port);
	}
	FIU0->SPI_FL_CFG = (0 << FIU_SPI_FL_CFG_RD_MODE_Pos) & FIU_SPI_FL_CFG_RD_MODE_Msk;
	gdma_memcpy_burst_u32(buff, (uint8_t*)((addr & 0x1FFFFFFF) + BASE_FIU(port)), len);

	return 0;
}

static void FIU_CmdReadWrite(enum flash_port port, uint8_t cmd, uint8_t tx_len, uint8_t rx_len, uint8_t* tx_dat, uint8_t* rx_dat)
{
	/* tx_len and rx_len must <= 4 */
	uint8_t len;
	uint32_t dat;

	if((tx_len > 4) || (rx_len > 4)) { return; }

	FIU0->MSR_IE_CFG |= FIU_MSR_IE_CGF_UMA_BLOCK_Msk;
	FIU0->UMA_ECTS = (FIU_UMA_ECTS_SW_CS0_Msk | FIU_UMA_ECTS_SW_CS1_Msk | FIU_UMA_ECTS_SW_CS2_Msk);
	FIU0->UMA_CODE = cmd;

	/* write */
	if (tx_len) {
		len = tx_len;
		dat = 0;
		while(len--) {
			dat <<= 8;
			dat |= *tx_dat++;
		}
		FIU0->UMA_AB0_3 = dat;
		FIU0->UMA_ECTS |= (tx_len << FIU_UMA_ECTS_UMA_ADDR_SIZE_Pos);

		if (tx_len == 4) {
			FIU0->EXT_CFG |= (FIU_EXT_CFG_FOUR_BADDR_Msk);
		}
	}

	(port == FLASH_PORT_BKP) ? ({FIU0->UMA_ECTS |= FIU_UMA_ECTS_DEV_NUM_BKP_Msk;}) : ({});
	FIU0->UMA_CTS = (FIU_UMA_CTS_EXEC_DONE_Msk | \
		((port == FLASH_PORT_PVT) ? (FIU_UMA_CTS_DEV_PRI_MSK) : (FIU_UMA_CTS_DEV_SHR_MSK)) | \
		((tx_len == 4) ? (FIU_UMA_CTS_A_SIZE_Msk) : (0)) | \
		((rx_len) ? (rx_len) : (0)));

	while (FIU0->UMA_CTS & FIU_UMA_CTS_EXEC_DONE_Msk);

	/* Read data */
	if (rx_len) {
		dat = FIU0->UMA_DB0_3;
		while (rx_len--) {
			*rx_dat++ = dat & 0xFF;
			dat >>= 8;
		}
	}

	FIU0->UMA_ECTS = (FIU_UMA_ECTS_SW_CS0_Msk | FIU_UMA_ECTS_SW_CS1_Msk | FIU_UMA_ECTS_SW_CS2_Msk);
	FIU0->MSR_IE_CFG &= ~FIU_MSR_IE_CGF_UMA_BLOCK_Msk;
	FIU0->EXT_CFG &= ~(FIU_EXT_CFG_FOUR_BADDR_Msk);
}

/* timeout_ms: 0 means waiting forever until busy flag is released. */
static void fiu_flash_wait_ready(enum flash_port port, uint32_t timeout_ms)
{
    uint8_t value;
    uint32_t countdownTick = 0;

    countdownTick = timeout_ms * 20; // 1ms => 1000us / 50us = 20 tick
    while(1) {
        delay(4799); // 50us
        FIU_CmdReadWrite(port, FLASH_CMD_READ_STATUS_REG, 0, 1, NULL, &value);
        if ((value & 0x01) == 0) break;
        if (countdownTick <= 0) continue;
        if (--countdownTick == 0) break;
    }
}

static uint8_t fiu_program(enum flash_port port, uint32_t addr, uint8_t *buff, uint32_t len, uint8_t adr_4b)
{
    uint16_t page_len;
    uint8_t d_size, align_size;

    while (len) {
		if(addr & 0xFF) {
			align_size = (256 - (addr & 0xFF));
			page_len = (len > align_size) ? (align_size) : (len);
		} else {
			page_len = (len >= 256) ? (256) : (len);
		}

		len -= page_len;

		/* write enable*/
		FIU_CmdReadWrite(port, FLASH_CMD_WRITE_ENABLE, 0, 0, 0, 0);

		/* write address 3B */
		FIU0->MSR_IE_CFG |= FIU_MSR_IE_CGF_UMA_BLOCK_Msk;
		FIU0->UMA_ECTS = ((adr_4b) ? ( 4 << FIU_UMA_ECTS_UMA_ADDR_SIZE_Pos) : (3 << FIU_UMA_ECTS_UMA_ADDR_SIZE_Pos)) |
				 ((port == FLASH_PORT_PVT) ? (FIU_UMA_ECTS_SW_CS1_Msk | FIU_UMA_ECTS_SW_CS2_Msk) : \
				 ((port == FLASH_PORT_BKP) ? (FIU_UMA_ECTS_DEV_NUM_BKP_Msk | FIU_UMA_ECTS_SW_CS0_Msk | FIU_UMA_ECTS_SW_CS1_Msk) : \
				 (FIU_UMA_ECTS_SW_CS0_Msk | FIU_UMA_ECTS_SW_CS2_Msk)));
		FIU0->UMA_AB0_3 = addr;
		FIU0->UMA_CODE = (adr_4b) ? (FLASH_CMD_4BYTE_PROGRAM) : (FLASH_CMD_PAGE_PROGRAM);

		FIU0->UMA_DB0 = buff[0];
		buff += 1;
		page_len -= 1;
		addr += 1;
		FIU0->Q_P_EN = 0;
		FIU0->UMA_CTS = (FIU_UMA_CTS_EXEC_DONE_Msk | FIU_UMA_CTS_RD_WR_Msk | 1);

		while (FIU0->UMA_CTS & FIU_UMA_CTS_EXEC_DONE_Msk);

		FIU0->UMA_ECTS &= ~FIU_UMA_ECTS_UMA_ADDR_SIZE_Msk;

		/* CS keep low and adr length = 0 */
		addr += page_len;

		while (page_len) {
			if(page_len == 1) {
				len += 1;
				addr -= 1;
				break;
			}

			d_size = (page_len >= 16) ? (16) : (page_len);
			page_len -= d_size;

			gdma_memcpy_burst_u32((uint8_t *)(FIU0->UMA_EXT_DB), buff, d_size);
			buff += d_size;

			while(FIU0->UMA_CTS & FIU_UMA_CTS_EXEC_DONE_Msk);
			FIU0->EXT_DB_CFG = EXT_DB_CFG_EXT_DB_EN_Msk | (d_size << EXT_DB_CFG_D_SIZE_Pos);
			FIU0->UMA_CTS = (FIU_UMA_CTS_EXEC_DONE_Msk | FIU_UMA_CTS_RD_WR_Msk | FIU_UMA_CTS_C_SIZE_Msk);
		}

		while(FIU0->UMA_CTS & FIU_UMA_CTS_EXEC_DONE_Msk);
		FIU0->EXT_DB_CFG = 0;
		FIU0->UMA_ECTS = (FIU_UMA_ECTS_SW_CS2_Msk | FIU_UMA_ECTS_SW_CS1_Msk | FIU_UMA_ECTS_SW_CS0_Msk);

		/* wait flash busy */
		fiu_flash_wait_ready(port, 10);
	}

	FIU0->Q_P_EN = 0;
	FIU0->MSR_IE_CFG &= ~FIU_MSR_IE_CGF_UMA_BLOCK_Msk;

	return 0;
}

static uint8_t fiu_sector_erase(enum flash_port port, uint32_t addr, uint8_t adr_4b)
{
	uint8_t dat[4];

	/* write enable */
	FIU_CmdReadWrite(port, FLASH_CMD_WRITE_ENABLE, 0, 0, 0, 0);

	/* addr to byte array */
	if (adr_4b) {
		dat[3] = addr & 0xFF;
		addr >>= 8;
	}
	dat[0] = (addr >> 16) & 0xFF;
	dat[1] = (addr >> 8) & 0xFF;
	dat[2] = addr & 0xFF;

	/* erase cmd */
	FIU_CmdReadWrite(port, ((adr_4b) ? (FLASH_CMD_4BYTE_SECTOR_ERASE) : (FLASH_CMD_SECTOR_ERASE)), ((adr_4b) ? (4):(3)), 0, dat, 0);

	/* wait flash busy */
	fiu_flash_wait_ready(port, 500);

	return 0;
}

static uint32_t halspi_rw(enum flash_port port, uint8_t *wr_buf, uint16_t wr_size, uint8_t *rd_buf, uint16_t rd_size)
{
	uint32_t len;
	uint8_t* pspidat;
	uint8_t cs_auto;
	uint8_t a_size = 0;

	if(wr_size == 0) {
		return 0xFFFFFFFF;
	}

	a_size = 0;

	if((rd_size > 4) || (wr_size >= 5)) {
		/* Read or write data, CS is manual control. */
		FIU0->UMA_ECTS = (port == FLASH_PORT_PVT) ? (FIU_UMA_ECTS_SW_CS1_Msk | FIU_UMA_ECTS_SW_CS2_Msk) :
				 (port == FLASH_PORT_SHD) ? (FIU_UMA_ECTS_SW_CS0_Msk | FIU_UMA_ECTS_SW_CS2_Msk) :
				 (port == FLASH_PORT_BKP) ? (FIU_UMA_ECTS_SW_CS0_Msk | FIU_UMA_ECTS_SW_CS1_Msk) : 0;
		FIU0->MSR_IE_CFG |= FIU_MSR_IE_CGF_UMA_BLOCK_Msk;
		cs_auto = 0;
	} else {
		/* Read or write instruction, CS is auto control. */
		FIU0->UMA_ECTS = (FIU_UMA_ECTS_SW_CS0_Msk | FIU_UMA_ECTS_SW_CS1_Msk | FIU_UMA_ECTS_SW_CS2_Msk);
		FIU0->MSR_IE_CFG &= ~FIU_MSR_IE_CGF_UMA_BLOCK_Msk;
		cs_auto = 1;
	}

	/* Command code */
	FIU0->UMA_CODE = *wr_buf++;
    (port == FLASH_PORT_BKP) ? ({FIU0->UMA_ECTS |= FIU_UMA_ECTS_DEV_NUM_BKP_Msk;}) : ({});

	if(wr_size >= 4) {
		/* address */
		FIU0->UMA_AB2 = *wr_buf++;
		FIU0->UMA_AB1 = *wr_buf++;
		FIU0->UMA_AB0 = *wr_buf++;
		wr_size -= 4;

		if(cs_auto == 0) {
			/* read or write data */
			FIU0->UMA_CTS = (FIU_UMA_CTS_EXEC_DONE_Msk | \
					 ((port == FLASH_PORT_PVT) ? (FIU_UMA_CTS_DEV_PRI_MSK) : (FIU_UMA_CTS_DEV_SHR_MSK)) | \
					 FIU_UMA_CTS_A_SIZE_Msk);

			while(FIU0->UMA_CTS & FIU_UMA_CTS_EXEC_DONE_Msk);
		} else {
			/* erase command */
			if((wr_size == 0) && (rd_size == 0)) {
				FIU0->UMA_CTS = (FIU_UMA_CTS_EXEC_DONE_Msk | \
						 FIU_UMA_CTS_RD_WR_Msk | \
						 ((port == FLASH_PORT_PVT) ? (FIU_UMA_CTS_DEV_PRI_MSK) : (FIU_UMA_CTS_DEV_SHR_MSK)) | \
						 FIU_UMA_CTS_A_SIZE_Msk);

				while(FIU0->UMA_CTS & FIU_UMA_CTS_EXEC_DONE_Msk);
			}

			/* 4-byte erase command or read <= 4byte */
			a_size = FIU_UMA_CTS_A_SIZE_Msk;
		}
	} else {
		wr_size -= 1;

		if(cs_auto == 1) {
			/* Write status */
			if((wr_size == 0) && (rd_size == 0)) {
				FIU0->UMA_CTS = (FIU_UMA_CTS_EXEC_DONE_Msk | \
				FIU_UMA_CTS_RD_WR_Msk | \
				((port == FLASH_PORT_PVT) ? (FIU_UMA_CTS_DEV_PRI_MSK) : (FIU_UMA_CTS_DEV_SHR_MSK)));

				while(FIU0->UMA_CTS & FIU_UMA_CTS_EXEC_DONE_Msk);
			}
		}
	}

	while (wr_size) {
		if(cs_auto == 0) {
			len = (wr_size >= 5)?(5):(wr_size);

			FIU0->UMA_CODE = *wr_buf++;
			FIU0->UMA_DB0 = *wr_buf++;
			FIU0->UMA_DB1 = *wr_buf++;
			FIU0->UMA_DB2 = *wr_buf++;
			FIU0->UMA_DB3 = *wr_buf++;

			/* Read per 4 byte */
			FIU0->UMA_CTS = (FIU_UMA_CTS_EXEC_DONE_Msk | \
					 FIU_UMA_CTS_RD_WR_Msk | \
					 ((port == FLASH_PORT_PVT) ? (FIU_UMA_CTS_DEV_PRI_MSK) : (FIU_UMA_CTS_DEV_SHR_MSK)) | \
					 (len - 1));

			wr_size -= len;

			while(FIU0->UMA_CTS & FIU_UMA_CTS_EXEC_DONE_Msk);
		} else {
			len = wr_size;
			pspidat = (uint8_t*)(&(FIU0->UMA_DB0));

			while(len--) {
				*pspidat++ = *wr_buf++;
			}

			/* write per < 4 byte */
			FIU0->UMA_CTS = (FIU_UMA_CTS_EXEC_DONE_Msk | \
					 FIU_UMA_CTS_RD_WR_Msk | a_size | \
					 ((port == FLASH_PORT_PVT) ? (FIU_UMA_CTS_DEV_PRI_MSK) : (FIU_UMA_CTS_DEV_SHR_MSK)) | \
					 (wr_size));

			wr_size = 0;

			while(FIU0->UMA_CTS & FIU_UMA_CTS_EXEC_DONE_Msk);
		}
	}

	while (rd_size) {
		if(rd_size > 4) {
			/* Read per 4 byte */
			FIU0->UMA_CTS = (FIU_UMA_CTS_EXEC_DONE_Msk | \
					 FIU_UMA_CTS_C_SIZE_Msk | \
					 ((port == FLASH_PORT_PVT) ? (FIU_UMA_CTS_DEV_PRI_MSK) : (FIU_UMA_CTS_DEV_SHR_MSK)) | \
					 4);
			rd_size -= 4;

			while (FIU0->UMA_CTS & FIU_UMA_CTS_EXEC_DONE_Msk);
			*rd_buf++ = FIU0->UMA_DB0;
			*rd_buf++ = FIU0->UMA_DB1;
			*rd_buf++ = FIU0->UMA_DB2;
			*rd_buf++ = FIU0->UMA_DB3;
		} else {
			/* Read per 4 byte */
			if(cs_auto == 0) {
				FIU0->UMA_CTS = (FIU_UMA_CTS_EXEC_DONE_Msk | \
						 FIU_UMA_CTS_C_SIZE_Msk | \
						 ((port == FLASH_PORT_PVT) ? (FIU_UMA_CTS_DEV_PRI_MSK) : (FIU_UMA_CTS_DEV_SHR_MSK)) | \
						 (rd_size & 0x07));

			while(FIU0->UMA_CTS & FIU_UMA_CTS_EXEC_DONE_Msk);
			} else {
				/* total rd_len < 4 */
				FIU0->UMA_CTS = (FIU_UMA_CTS_EXEC_DONE_Msk | \
						 ((port == FLASH_PORT_PVT) ? (FIU_UMA_CTS_DEV_PRI_MSK) : (FIU_UMA_CTS_DEV_SHR_MSK)) | \
						 a_size | \
						 (rd_size & 0x07));

				while(FIU0->UMA_CTS & FIU_UMA_CTS_EXEC_DONE_Msk);
			}

			pspidat = (uint8_t*)(&(FIU0->UMA_DB0));
			while(rd_size--) {
				*rd_buf++ = *pspidat++;
			}

			rd_size = 0;
		}
	}

	if(cs_auto == 0) {
		// CS_HIGH
		FIU0->UMA_ECTS = (FIU_UMA_ECTS_SW_CS0_Msk | FIU_UMA_ECTS_SW_CS1_Msk | FIU_UMA_ECTS_SW_CS2_Msk);
		FIU0->MSR_IE_CFG &= ~FIU_MSR_IE_CGF_UMA_BLOCK_Msk;
	}

    return 0;
}

static uint8_t fiu_get_flash_info(enum flash_port port, uint8_t *buf)
{
	uint32_t flash_size;
	uint8_t wr_buf[5] = {0x5A, 0, 0, 0, 0};
	uint8_t rd_buf[0x10] = {0};

	/* Read JEDEC ID */
	FIU_CmdReadWrite(port, FLASH_CMD_READ_JEDEC_ID, 0, 3, 0, rd_buf);
	buf[0] = rd_buf[2];
	buf[1] = rd_buf[1];
	buf[2] = rd_buf[0];

	halspi_rw(port, wr_buf, 5, rd_buf, 16);

	/* Check the SFDP header */
	if (!(rd_buf[0] == 'S' && rd_buf[1] == 'F' && rd_buf[2] == 'D' && rd_buf[3] == 'P'))
		return 0xFF;

	/* Access SFDP parameter table */
	wr_buf[0] = 0x5A;
	wr_buf[1] = rd_buf[0xE];
	wr_buf[2] = rd_buf[0xD];
	wr_buf[3] = rd_buf[0xC];
	wr_buf[4] = 0x00;

	halspi_rw(port, wr_buf, 5, rd_buf, 8);

	/* Get the Flash memory density */
	if (rd_buf[7] & 0x80)
		return 0xFF;

	flash_size = ((rd_buf[4] | (rd_buf[5] << 8) | (rd_buf[6] << 16) | (rd_buf[7] << 24)) + 1) >> 3;
	buf[4] = flash_size & 0xFF;
	buf[5] = (flash_size >> 8) & 0xFF;
	buf[6] = (flash_size >> 16) & 0xFF;
	buf[7] = (flash_size >> 24) & 0xFF;

	/* Verify if the flash supports 4-byte addressing mode */
	buf[8] = (rd_buf[2] & 0x03) ? 1 : 0;

	return 0;
}

static void fiu_quad_mode(enum flash_port port, uint8_t enable)
{
	#define WB_GIGA_REG2_QE     0x02
	#define MXIC_XMC_REG1_QE    0x40

	uint8_t reg[2];
	uint8_t EDID[3];

	FIU_CmdReadWrite(port, FLASH_CMD_READ_JEDEC_ID, 0, 3, 0, EDID);
	FIU_CmdReadWrite(port, FLASH_CMD_READ_STATUS_REG, 0, 1, 0, reg);
	FIU_CmdReadWrite(port, FLASH_CMD_READ_STATUS2_REG, 0, 1, 0, (reg + 1));

	if(EDID[0] == FLASH_ID_WINBOND || EDID[0] == FLASH_ID_GIGADEV) {
		if (enable) {
			reg[1] |= WB_GIGA_REG2_QE;
		} else {
			reg[1] &= ~WB_GIGA_REG2_QE;
		}
	} else if (EDID[0] == FLASH_ID_MXIC || EDID[0] == FLASH_ID_MICRON) {
		if (enable) {
			reg[0] |= MXIC_XMC_REG1_QE;
		} else {
			reg[0] &= ~MXIC_XMC_REG1_QE;
		}
	}
	FIU_CmdReadWrite(port, FLASH_CMD_WRITE_STATUS_REG, 2, 0, reg, 0);
}

static void fiu_4byte_mode(enum flash_port port, uint8_t enter)
{
	FIU_CmdReadWrite(port, (enter) ? (FLASH_CMD_ENTER_4BYTE_MODE) : (FLASH_CMD_EXIT_4BYTE_MODE), 0, 0, 0, 0);
}

static void PinSelect(enum flash_port port)
{
	switch (port) {
	case FLASH_PORT_PVT:
		RegSetBit(SCFG->DEVALTC, (BIT3 | BIT2));
		RegClrBit(SCFG->DEV_CTL3, BIT1);
		RegSetBit(SCFG->DEVALT0, BIT1);
		break;
        case FLASH_PORT_SHD:
		RegClrBit(SCFG->DEVCNT, BIT6);
		RegSetBit(SCFG->DEVALTC, (BIT3 | BIT2));
		RegClrBit(SCFG->DEV_CTL3, BIT1);
		break;
        case FLASH_PORT_BKP:
		RegSetBit(SCFG->DEVALTC, (BIT3 | BIT2));
		RegClrBit(SCFG->DEV_CTL3, BIT1);
		RegSetBit(SCFG->DEVALT0, BIT2);
		break;
        default:
		break;
	}
}

static void FIU_OpenPort(enum flash_port port, uint8_t quad_mode)
{
	/* Set FIU clock to 24MHz */
	SET_FIU_CLK_DIV(0x03);

	FIU0->BURST_CFG = FIU_BURST_CFG_R_BURST_Msk;

	/* Share SPI not tri-state */
	SET_FIU_SHD_ACTIVE;

	ENABLE_FIU_SHD_SPI;
	FIU_SWITCH_TO_SHARE;

	if (quad_mode)
		ENABLE_FIU_SHD_QUAD;

	if(port == FLASH_PORT_PVT) {
		PinSelect(FLASH_PORT_PVT);
		ENABLE_FIU_PVT_CS;
	} else if(port == FLASH_PORT_BKP) {
		PinSelect(FLASH_PORT_BKP);
		ENABLE_FIU_BKP_CS;
	} else {
		PinSelect(FLASH_PORT_SHD);
	}
}

static void fiu_port_init(enum flash_port port, uint8_t quad_mode, uint8_t adr_4b)
{
	FIU_OpenPort(port, quad_mode);
	fiu_quad_mode(port, quad_mode);
	fiu_4byte_mode(port, adr_4b);
}

static void flashloader_init(void)
{
	/* Initialize the flashloader parameters and buffer */
	memset((struct nct_esio_flash_params *)&g_cfg, 0, sizeof(struct nct_esio_flash_params));
	memset((uint8_t *)g_buf, 0, sizeof(g_buf));
	g_cfg.state = FLASH_ALGO_STATE_IDLE;
}

int main(void)
{
	uint8_t status;

	flashloader_init();

	while (1) {
		while (g_cfg.state == FLASH_ALGO_STATE_IDLE);

		switch (g_cfg.cmd) {
		case FLASH_ALGO_CMD_INIT:
			FIU_OpenPort(g_cfg.type, FLASH_MODE_SINGLE);
			status = fiu_get_flash_info(g_cfg.type, (uint8_t *)g_buf);
			break;
		case FLASH_ALGO_CMD_ERASE:
			fiu_port_init(g_cfg.type, FLASH_MODE_SINGLE, g_cfg.adr_4b);
			status = fiu_sector_erase(g_cfg.type, g_cfg.addr, g_cfg.adr_4b);
			break;
		case FLASH_ALGO_CMD_PROGRAM:
			fiu_port_init(g_cfg.type, FLASH_MODE_SINGLE, g_cfg.adr_4b);
			status = fiu_program(g_cfg.type, g_cfg.addr, (uint8_t *)g_buf, g_cfg.len, g_cfg.adr_4b);
			break;
		case FLASH_ALGO_CMD_READ:
			fiu_port_init(g_cfg.type, FLASH_MODE_SINGLE, g_cfg.adr_4b);
			status = fiu_read(g_cfg.type, g_cfg.addr, (uint8_t *)g_buf, g_cfg.len, g_cfg.adr_4b);
			break;
		default:
			status = 0xff;
			break;
		}

		if (status != 0) {
			g_cfg.state = status;
			while (1);
		} else {
			g_cfg.state = FLASH_ALGO_STATE_IDLE;
		}
	}
	return 0;
}

__attribute__ ((section(".stack")))
__attribute__ ((used))
static uint32_t stack[400 / 4];
extern uint32_t _estack;
extern uint32_t _bss;
extern uint32_t _ebss;

__attribute__ ((section(".entry")))
void entry(void)
{
	/* set sp from end of stack */
	__asm(" ldr sp, =_estack - 4");

	main();

	__asm(" bkpt #0x00");
}

