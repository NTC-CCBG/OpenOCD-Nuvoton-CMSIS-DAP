// SPDX-License-Identifier: GPL-2.0-or-later

#include <stdint.h>
#include <string.h>
#include "nct6692d_flash.h"

/* flashloader parameter structure */
__attribute__ ((section(".buffers.g_cfg")))
static volatile struct nct_esio_flash_params g_cfg;

/* data buffer */
__attribute__ ((section(".buffers.g_buf")))
static volatile uint8_t g_buf[0x1000];

static inline uint32_t __REV(uint32_t value) {
    return ((value & 0xFF000000) >> 24) |
           ((value & 0x00FF0000) >> 8)  |
           ((value & 0x0000FF00) << 8)  |
           ((value & 0x000000FF) << 24);
}

static void delay(uint32_t i)
{
	while (i--)
		__asm__ volatile ("nop");
}

static void UMA_ExeCmd(FIU_T *fiu, enum flash_command cmd, uint8_t sts)
{
	fiu->UMA_CODE = cmd;
	fiu->UMA_CTS = sts;
	while (fiu->UMA_CTS & FIU_UMA_CTS_EXEC_DONE_Msk);
}

/* timeout_ms: 0 means waiting forever until busy flag is released. */
static int UMA_WaitFlashReady(FIU_T *fiu, uint8_t flash_sel, uint32_t timeout_ms)
{
	uint32_t countdownTick = timeout_ms * 20; // 1ms => 1000us / 50us = 20 tick

	while(1) {
		delay(2399); // 50us @ 48MHz
		UMA_ExeCmd(fiu, FLASH_CMD_READ_STATUS_REG, (FIU_UMA_CTS_EXEC_DONE_Msk | flash_sel | 1));
		if ((fiu->UMA_DB0 & 0x01) == 0) break;
		if (countdownTick == 0) continue;
		if (--countdownTick == 0) return -1;
	}

	return 0;
}

void gdma_memcpy_u8(uint8_t* dst, uint8_t* src, uint32_t cpylen)
{
	if (cpylen == 0) return;

	GDMA_SRCB0 = (uint32_t)src;
	GDMA_DSTB0 = (uint32_t)dst;
	GDMA_TCNT0 = cpylen;
	GDMA_CTL0 = 0x10001;

	while (GDMA_CTL0 & 0x1);
	GDMA_CTL0 = 0;
}

void gdma_memcpy_burst_u32(uint8_t* dst, uint8_t* src, uint32_t cpylen)
{
	uint32_t rlen;

	if(cpylen == 0) return;

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

		while (GDMA_CTL0 & 0x1);
		GDMA_CTL0 = 0;

		FIU0->BURST_CFG = 0x03;

		src += rlen;
		dst += rlen;
		cpylen -= rlen;
	}

	/* remain length */
	if (cpylen) {
		gdma_memcpy_u8(dst, src, cpylen);
	}
}

static int fiu_dual_io_read(enum flash_port port, uint32_t addr, uint8_t* buff, uint32_t len, uint8_t adr_4b)
{
	if (port == FLASH_PORT_SHD) {
		if (adr_4b) {
			FIU0->EXT_CFG |= (FIU_EXT_CFG_SET_CMD_EN_Msk | FIU_EXT_CFG_FOUR_BADDR_Msk);
			FIU0->RD_CMD = FLASH_CMD_4BYTE_DUAL_IO_READ;
		}

		gdma_memcpy_burst_u32(buff, (uint8_t*)(addr + 0x64000000), len);

		if (adr_4b) {
			FIU0->EXT_CFG &= ~(FIU_EXT_CFG_SET_CMD_EN_Msk | FIU_EXT_CFG_FOUR_BADDR_Msk);
			FIU0->RD_CMD = 0;
		}
	} else {
		gdma_memcpy_burst_u32(buff, (uint8_t*)(addr + 0x60000000), len);
	}

	return 0;
}

static int FIU_PageProgram(enum flash_port port, uint32_t addr, uint8_t *data, uint32_t len, uint8_t FourByteMode)
{
	uint16_t dataCnt = 0;
	uint16_t maxBytes = 256 - (addr % 256);
	uint16_t n;
	uint32_t d_size = 0;
	int ret = 0;

	if (FourByteMode) {
		FIU0->EXT_CFG |= FIU_EXT_CFG_FOUR_BADDR_Msk;
	}

	FIU_UMA_BLOCK(FIU0);
#define UMA_TEST 2
#if (UMA_TEST == 1)
	while (len > 0) {
		n = (len <= maxBytes) ? len : maxBytes;

		FIU0->UMA_ECTS = (BIT0 | BIT1);
		UMA_ExeCmd(FIU0, FLASH_CMD_WRITE_ENABLE, (FIU_UMA_CTS_EXEC_DONE_Msk | \
							  ((port == FLASH_PORT_PVT) ? 0 : (FIU_UMA_CTS_DEV_NUM_Msk))));

		FIU0->UMA_ECTS = ((port == FLASH_PORT_PVT) ? BIT1 : BIT0);  // keep CS low
		FIU0->UMA_AB0_3 = addr;
		FIU0->UMA_CODE = (FourByteMode) ? FLASH_CMD_4BYTE_PROGRAM : FLASH_CMD_PAGE_PROGRAM;
		FIU0->UMA_DB0 = *data++;
		FIU0->UMA_CTS = (FIU_UMA_CTS_EXEC_DONE_Msk | FIU_UMA_CTS_RD_WR_Msk | FIU_UMA_CTS_A_SIZE_Msk | \
				 ((port == FLASH_PORT_PVT) ? 0 : (FIU_UMA_CTS_DEV_NUM_Msk)) | 1);
		while (FIU0->UMA_CTS & FIU_UMA_CTS_EXEC_DONE_Msk);

		/* write data */
		dataCnt = 1;
		while (dataCnt < n) {
			d_size = ((n - dataCnt) < 5) ? (n - dataCnt) : 5 ;

			FIU0->UMA_CODE = data[0];
			FIU0->UMA_DB0 = data[1];
			FIU0->UMA_DB1 = data[2];
			FIU0->UMA_DB2 = data[3];
			FIU0->UMA_DB3 = data[4];

			data += d_size;
			dataCnt += d_size;

			FIU0->UMA_CTS = (FIU_UMA_CTS_EXEC_DONE_Msk | FIU_UMA_CTS_RD_WR_Msk | \
					 ((port == FLASH_PORT_PVT) ? 0 : (FIU_UMA_CTS_DEV_NUM_Msk)) | (d_size - 1));
			while (FIU0->UMA_CTS & FIU_UMA_CTS_EXEC_DONE_Msk);
		}
		FIU0->UMA_ECTS = (BIT0 | BIT1); // release CS

		/* Wait flash is ready */
		if ((ret = UMA_WaitFlashReady(FIU0, ((port == FLASH_PORT_PVT) ? 0 : (FIU_UMA_CTS_DEV_NUM_Msk)), 50)) < 0) break;

		addr += n;      // adjust the addresses
		len -= n;
		maxBytes = 256;     // now we can do up to 256 bytes per loop
	}
#elif (UMA_TEST == 2)
	while (len > 0) {
		n = (len <= maxBytes) ? len : maxBytes;

		FIU0->UMA_ECTS = (BIT0 | BIT1);
		UMA_ExeCmd(FIU0, FLASH_CMD_WRITE_ENABLE, (FIU_UMA_CTS_EXEC_DONE_Msk | \
							  ((port == FLASH_PORT_PVT) ? 0 : (FIU_UMA_CTS_DEV_NUM_Msk))));

		if (!FourByteMode) FIU0->EXT_CFG &= ~(FIU_EXT_CFG_FOUR_BADDR_Msk);
		FIU0->UMA_ECTS = ((port == FLASH_PORT_PVT) ? BIT1 : BIT0);  // keep CS low
		FIU0->UMA_AB0_3 = addr;
		FIU0->UMA_CODE = (FourByteMode) ? FLASH_CMD_4BYTE_PROGRAM : FLASH_CMD_PAGE_PROGRAM;
		FIU0->UMA_CTS = (FIU_UMA_CTS_EXEC_DONE_Msk | FIU_UMA_CTS_RD_WR_Msk | FIU_UMA_CTS_A_SIZE_Msk | \
				 ((port == FLASH_PORT_PVT) ? 0 : (FIU_UMA_CTS_DEV_NUM_Msk)) | 0);

		while (FIU0->UMA_CTS & FIU_UMA_CTS_EXEC_DONE_Msk);

		FIU0->EXT_CFG |= FIU_EXT_CFG_FOUR_BADDR_Msk;
		/* write data */
		dataCnt = 0;

		while (dataCnt < n) {
			if ((n-dataCnt) >= 9) {
				d_size = 9;
			} else if ((n-dataCnt) < 5) {
				d_size = (n-dataCnt);
			} else {
				d_size = 5;
			}

			while (FIU0->UMA_CTS & FIU_UMA_CTS_EXEC_DONE_Msk);

			if (d_size == 9) {
				FIU0->UMA_CODE = data[0];
				FIU0->UMA_AB0_3 = __REV(*(uint32_t*)&data[1]);
				FIU0->UMA_DB0 = data[5];
				FIU0->UMA_DB1 = data[6];
				FIU0->UMA_DB2 = data[7];
				FIU0->UMA_DB3 = data[8];
				FIU0->UMA_CTS = (FIU_UMA_CTS_EXEC_DONE_Msk | FIU_UMA_CTS_RD_WR_Msk | FIU_UMA_CTS_A_SIZE_Msk | \
						 ((port == FLASH_PORT_PVT) ? 0 : (FIU_UMA_CTS_DEV_NUM_Msk)) | 4);
			}
			else {
				FIU0->UMA_CODE = data[0];
				FIU0->UMA_DB0 = data[1];
				FIU0->UMA_DB1 = data[2];
				FIU0->UMA_DB2 = data[3];
				FIU0->UMA_DB3 = data[4];
				FIU0->UMA_CTS = (FIU_UMA_CTS_EXEC_DONE_Msk | FIU_UMA_CTS_RD_WR_Msk | \
						 ((port == FLASH_PORT_PVT) ? 0 : (FIU_UMA_CTS_DEV_NUM_Msk)) | (d_size - 1));
			}

			data += d_size;
			dataCnt += d_size;
		}

		while (FIU0->UMA_CTS & FIU_UMA_CTS_EXEC_DONE_Msk);
		FIU0->UMA_ECTS = (BIT0 | BIT1); // release CS

		/* Wait flash is ready */
		if ((ret = UMA_WaitFlashReady(FIU0, ((port == FLASH_PORT_PVT) ? 0 : (FIU_UMA_CTS_DEV_NUM_Msk)), 50)) < 0) break;

		addr += n;      // adjust the addresses
		len -= n;
		maxBytes = 256;     // now we can do up to 256 bytes per loop
	}
#endif
	FIU_UMA_UNBLOCK(FIU0);

	FIU0->EXT_CFG &= ~(FIU_EXT_CFG_FOUR_BADDR_Msk);
	return ret;
}

static int FIU_SectorErase(enum flash_port port, uint32_t addr, uint8_t FourByteMode)
{
	int ret;

	if (FourByteMode) {
		FIU0->EXT_CFG |= FIU_EXT_CFG_FOUR_BADDR_Msk;
	}

	FIU_UMA_BLOCK(FIU0);
	// Write enable
	UMA_ExeCmd(FIU0, FLASH_CMD_WRITE_ENABLE, (FIU_UMA_CTS_EXEC_DONE_Msk | ((port == FLASH_PORT_PVT) ? 0 : (FIU_UMA_CTS_DEV_NUM_Msk))));
	// Set address
	FIU0->UMA_AB0_3 = addr;
	// Erase
	UMA_ExeCmd(FIU0, FLASH_CMD_SECTOR_ERASE, (FIU_UMA_CTS_EXEC_DONE_Msk | FIU_UMA_CTS_A_SIZE_Msk | \
						  ((port == FLASH_PORT_PVT) ? 0 : (FIU_UMA_CTS_DEV_NUM_Msk))));
	// Wait flash is ready
	ret = UMA_WaitFlashReady(FIU0, ((port == FLASH_PORT_PVT) ? 0 : (FIU_UMA_CTS_DEV_NUM_Msk)), 500);
	FIU_UMA_UNBLOCK(FIU0);

	if (FourByteMode) {
		FIU0->EXT_CFG &= ~(FIU_EXT_CFG_FOUR_BADDR_Msk);
	}

	return ret;
}

static uint32_t halspi_rw(enum flash_port port, uint8_t *wr_buf, uint16_t wr_size, uint8_t *rd_buf, uint16_t rd_size)
{
	uint32_t len;
	uint8_t* pspidat;
	uint8_t cs_auto;
	uint8_t a_size = 0;

	if (wr_size == 0) {
		return 0xFFFFFFFF;
	}

	a_size = 0;

	if ((rd_size > 4) || (wr_size >= 5)) {
		/* Read or write data, CS is manual control. */
		FIU0->UMA_ECTS = (port == FLASH_PORT_PVT) ? FIU_UMA_ECTS_SW_CS1_Msk : FIU_UMA_ECTS_SW_CS0_Msk;
		FIU0->MSR_IE_CFG |= FIU_MSR_IE_CGF_UMA_BLOCK_Msk;
		cs_auto = 0;
	} else {
		/* Read or write instruction, CS is auto control. */
		FIU0->UMA_ECTS = (FIU_UMA_ECTS_SW_CS1_Msk | FIU_UMA_ECTS_SW_CS0_Msk);
		FIU0->MSR_IE_CFG &= ~FIU_MSR_IE_CGF_UMA_BLOCK_Msk;
		cs_auto = 1;
	}

	/* Command code */
	FIU0->UMA_CODE = *wr_buf++;

	if (wr_size >= 4) {
		/* address */
		FIU0->UMA_AB2 = *wr_buf++;
		FIU0->UMA_AB1 = *wr_buf++;
		FIU0->UMA_AB0 = *wr_buf++;
		wr_size -= 4;

		if(cs_auto == 0) {
			/* read or write data */
			FIU0->UMA_CTS = (FIU_UMA_CTS_EXEC_DONE_Msk | \
					 ((port == FLASH_PORT_PVT) ? 0 : (FIU_UMA_CTS_DEV_NUM_Msk)) | \
					 FIU_UMA_CTS_A_SIZE_Msk);
			while (FIU0->UMA_CTS & FIU_UMA_CTS_EXEC_DONE_Msk);
		} else {
			/* erase command */
			if ((wr_size == 0) && (rd_size == 0)) {
				FIU0->UMA_CTS = (FIU_UMA_CTS_EXEC_DONE_Msk | \
						 FIU_UMA_CTS_RD_WR_Msk | \
						 ((port == FLASH_PORT_PVT) ? 0 : (FIU_UMA_CTS_DEV_NUM_Msk)) | \
						 FIU_UMA_CTS_A_SIZE_Msk);
				while (FIU0->UMA_CTS & FIU_UMA_CTS_EXEC_DONE_Msk);
			}

			/* 4-byte erase command or read <= 4byte */
			a_size = FIU_UMA_CTS_A_SIZE_Msk;
		}
	} else {
		wr_size -= 1;

		if(cs_auto == 0) {
			//FIU0->UMA_CTS = (FIU_UMA_CTS_EXEC_DONE_Msk |
			//                 FIU_UMA_CTS_DEV_NUM_Msk);
			//while(FIU0->UMA_CTS & FIU_UMA_CTS_EXEC_DONE_Msk);
		}
		else {
			/* Write status */
			if((wr_size == 0) && (rd_size == 0)) {
				FIU0->UMA_CTS = (FIU_UMA_CTS_EXEC_DONE_Msk | \
						 FIU_UMA_CTS_RD_WR_Msk | \
						 ((port == FLASH_PORT_PVT) ? 0 : (FIU_UMA_CTS_DEV_NUM_Msk)));
				while (FIU0->UMA_CTS & FIU_UMA_CTS_EXEC_DONE_Msk);
			}
		}
	}

	while (wr_size) {
		if (cs_auto == 0) {
			len = (wr_size >= 5)?(5):(wr_size);

			FIU0->UMA_CODE = *wr_buf++;
			FIU0->UMA_DB0 = *wr_buf++;
			FIU0->UMA_DB1 = *wr_buf++;
			FIU0->UMA_DB2 = *wr_buf++;
			FIU0->UMA_DB3 = *wr_buf++;

			/* Read per 4 byte */
			FIU0->UMA_CTS = (FIU_UMA_CTS_EXEC_DONE_Msk | \
					 FIU_UMA_CTS_RD_WR_Msk | \
					 ((port == FLASH_PORT_PVT) ? 0 : (FIU_UMA_CTS_DEV_NUM_Msk)) | \
					 (len - 1));

			wr_size -= len;
			while (FIU0->UMA_CTS & FIU_UMA_CTS_EXEC_DONE_Msk);
		} else {
			len = wr_size;
			pspidat = (uint8_t*)(&(FIU0->UMA_DB0));

			while (len--) {
				*pspidat++ = *wr_buf++;
			}

			/* write per < 4 byte */
			FIU0->UMA_CTS = (FIU_UMA_CTS_EXEC_DONE_Msk | \
					 FIU_UMA_CTS_RD_WR_Msk | a_size | \
					 ((port == FLASH_PORT_PVT) ? 0 : (FIU_UMA_CTS_DEV_NUM_Msk)) | \
					 (wr_size));

			wr_size = 0;
			while (FIU0->UMA_CTS & FIU_UMA_CTS_EXEC_DONE_Msk);
		}
	}

	while (rd_size) {
		if(rd_size > 4) {
			/* Read per 4 byte */
			FIU0->UMA_CTS = (FIU_UMA_CTS_EXEC_DONE_Msk | \
					 FIU_UMA_CTS_C_SIZE_Msk | \
					 ((port == FLASH_PORT_PVT) ? 0 : (FIU_UMA_CTS_DEV_NUM_Msk)) | \
					 4);
			rd_size -= 4;
			while (FIU0->UMA_CTS & FIU_UMA_CTS_EXEC_DONE_Msk);
			*rd_buf++ = FIU0->UMA_DB0;
			*rd_buf++ = FIU0->UMA_DB1;
			*rd_buf++ = FIU0->UMA_DB2;
			*rd_buf++ = FIU0->UMA_DB3;
		} else {
			/* Read per 4 byte */
			if (cs_auto == 0) {
				FIU0->UMA_CTS = (FIU_UMA_CTS_EXEC_DONE_Msk | \
						 FIU_UMA_CTS_C_SIZE_Msk | \
						 ((port == FLASH_PORT_PVT) ? 0 : (FIU_UMA_CTS_DEV_NUM_Msk)) | \
						 (rd_size & 0x07));

				while (FIU0->UMA_CTS & FIU_UMA_CTS_EXEC_DONE_Msk);
			} else {
				/* total rd_len < 4 */
				FIU0->UMA_CTS = (FIU_UMA_CTS_EXEC_DONE_Msk | \
						 ((port == FLASH_PORT_PVT) ? 0 : (FIU_UMA_CTS_DEV_NUM_Msk)) | \
						 a_size | \
						 (rd_size & 0x07));

				while (FIU0->UMA_CTS & FIU_UMA_CTS_EXEC_DONE_Msk);
			}

			pspidat = (uint8_t*)(&(FIU0->UMA_DB0));
			while (rd_size--) {
				*rd_buf++ = *pspidat++;
			}

			rd_size = 0;
		}
	}

	if (cs_auto == 0) {
		// CS_HIGH
		FIU0->UMA_ECTS = (FIU_UMA_ECTS_SW_CS1_Msk | FIU_UMA_ECTS_SW_CS0_Msk);
		FIU0->MSR_IE_CFG &= ~FIU_MSR_IE_CGF_UMA_BLOCK_Msk;
	}

	return 0;
}

static uint8_t fiu_get_flash_info(enum flash_port port, uint8_t *buf)
{
	uint32_t flash_size;
	uint8_t wr_buf[5] = {0x5A, 0, 0, 0, 0};
	uint8_t rd_buf[0x10] = {0};

	FIU_UMA_BLOCK(FIU0);
	FIU0->UMA_ECTS = (BIT0 | BIT1);

	/* Read JEDEC ID */
	FIU0->UMA_CODE = FLASH_CMD_READ_JEDEC_ID;
	FIU0->UMA_DB0 = 0;
	FIU0->UMA_DB1 = 0;
	FIU0->UMA_DB2 = 0;

	FIU0->UMA_CTS = (FIU_UMA_CTS_EXEC_DONE_Msk | ((port == FLASH_PORT_PVT) ? 0 : (FIU_UMA_CTS_DEV_NUM_Msk)) | 3);
	while(FIU0->UMA_CTS & FIU_UMA_CTS_EXEC_DONE_Msk);

	buf[0] = FIU0->UMA_DB2; // Manufacturer ID
	buf[1] = FIU0->UMA_DB1; // Memory Type
	buf[2] = FIU0->UMA_DB0; // Capacity

	/* Read SFDP header */
	// TODO: Implement SFDP reading using halspi_rw or direct UMA commands
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

	FIU0->UMA_ECTS = (BIT0 | BIT1); // release CS
	FIU_UMA_UNBLOCK(FIU0);

	return 0;
}

static uint32_t FIU_GetID(enum flash_port port)
{
	FIU_UMA_BLOCK(FIU0);

	FIU0->UMA_ECTS = (BIT0 | BIT1);

	FIU0->UMA_CODE = FLASH_CMD_READ_JEDEC_ID;
	FIU0->UMA_DB0 = 0;
	FIU0->UMA_DB1 = 0;
	FIU0->UMA_DB2 = 0;

	FIU0->UMA_CTS = (FIU_UMA_CTS_EXEC_DONE_Msk | ((port == FLASH_PORT_PVT) ? 0 : (FIU_UMA_CTS_DEV_NUM_Msk)) | 3);
	while(FIU0->UMA_CTS & FIU_UMA_CTS_EXEC_DONE_Msk);

	FIU0->UMA_ECTS = (BIT0 | BIT1); // release CS

	FIU_UMA_UNBLOCK(FIU0);

	return (((FIU0->UMA_DB0) << 16) | ((FIU0->UMA_DB1) << 8) | (FIU0->UMA_DB2));
}

static void PinSelect(enum flash_port port)
{
	switch (port) {
	case FLASH_PORT_PVT:
		SCFG->DEVCNT = 0x40;
		SCFG->DEVALTC = 0x10;
		break;
        case FLASH_PORT_SHD:
		SCFG->DEVCNT = 0x00;
		SCFG->DEVALTC = 0x28;
		break;
        default:
		break;
	}
}

void FIU_4ByteMode(enum flash_port port, uint8_t enter)
{
	FIU_UMA_BLOCK(FIU0);
	if (enter == 1)
	{
		// Enter 4-byte Address mode
		UMA_ExeCmd(FIU0, FLASH_CMD_ENTER_4BYTE_MODE, (FIU_UMA_CTS_EXEC_DONE_Msk | \
							      ((port == FLASH_PORT_PVT) ? 0 : (FIU_UMA_CTS_DEV_NUM_Msk))));
		// Switch FIU to 4byte mode
		//FIU_ADR_4BYTE_ENABLE(FIU0, FASTREAD_DUAL_4B);
	}
	else
	{
		// Exit 4-byte Address mode
		UMA_ExeCmd(FIU0, FLASH_CMD_EXIT_4BYTE_MODE, (FIU_UMA_CTS_EXEC_DONE_Msk | \
							     ((port == FLASH_PORT_PVT) ? 0 : (FIU_UMA_CTS_DEV_NUM_Msk))));
		// Switch FIU to 3byte mode
		//FIU_ADR_4BYTE_DISABLE(FIU0);
	}
	FIU_UMA_UNBLOCK(FIU0);
}

static void fiu_port_init(enum flash_port port, uint8_t adr_4b)
{
	if (port != FLASH_PORT_SHD) return;

	PinSelect(port);
        FIU0->BURST_CFG = 3;
        FIU_RD_MODE_FAST_RD_DUAL_IO(FIU0);
	FIU_4ByteMode(port, adr_4b);
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
	uint32_t id;

	flashloader_init();

	while (1) {
		enum flash_port port;

		while (g_cfg.state == FLASH_ALGO_STATE_IDLE);
		port = g_cfg.type;
		if (port != FLASH_PORT_PVT && port != FLASH_PORT_SHD) {
			g_cfg.state = 0xFF;
			while (1);
		}

		switch (g_cfg.cmd) {
		case FLASH_ALGO_CMD_INIT:
			PinSelect(port);
			if (port == FLASH_PORT_PVT) {
				id = FIU_GetID(port);
				if (id == 0 || id == 0xFFFFFF) {
					status = 0xFF;
				} else {
					g_buf[0] = id & 0xff;
					g_buf[1] = (id >> 8) & 0xff;
					g_buf[2] = (id >> 16) & 0xff;
					g_buf[3] = 0x00;
					g_buf[8] = 0;
					status = 0;
				}
			} else if (port == FLASH_PORT_SHD) {
				status = fiu_get_flash_info(port, (uint8_t *)g_buf);
			} else {
				status = 0xFF;
			}

			break;
		case FLASH_ALGO_CMD_ERASE:
			fiu_port_init(port, g_cfg.adr_4b);
			status = FIU_SectorErase(port, g_cfg.addr, g_cfg.adr_4b);
			break;
		case FLASH_ALGO_CMD_PROGRAM:
			fiu_port_init(port, g_cfg.adr_4b);
			status = FIU_PageProgram(port, g_cfg.addr, (uint8_t *)g_buf, g_cfg.len, g_cfg.adr_4b);
			status = 0;
			break;
		case FLASH_ALGO_CMD_READ:
			fiu_port_init(port, g_cfg.adr_4b);
			status = fiu_dual_io_read(port, g_cfg.addr, (uint8_t *)g_buf, g_cfg.len, g_cfg.adr_4b);
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

