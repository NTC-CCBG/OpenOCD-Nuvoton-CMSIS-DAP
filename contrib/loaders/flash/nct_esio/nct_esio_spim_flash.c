// SPDX-License-Identifier: GPL-2.0-or-later

#include <stdint.h>
#include <string.h>
#include "nct_esio_flash.h"

/* flashloader parameter structure */
__attribute__ ((section(".buffers.g_cfg")))
static volatile struct nct_esio_flash_params g_cfg;

/* data buffer */
__attribute__ ((section(".buffers.g_buf")))
static volatile uint8_t g_buf[0x1000];

static void SPIM_CmdReadWrite(uint8_t cmd, uint8_t tx_len, uint8_t rx_len,
						      uint8_t* tx_dat, uint8_t* rx_dat)
{
	unsigned long len;
	unsigned long spim_ctl_copy = SPIM->CTL0;

	/* bit width:8, busrtlen = tx_len */
	SPIM->CTL0 = SPIM_CTL0_NORMAL(SPIM_BITMODE_SINGLE, 1, 8, 1);

	SPIM->TX[0] = cmd;

	/* CS active & start*/
	SPIM->CTL1 &= ~SPIM_CTL1_SS_Msk;
	SPIM->CTL1 |= SPIM_CTL1_SPIMEN_Msk;

	/* wait ready */
	while (SPIM->CTL1 & SPIM_CTL1_SPIMEN_Msk);

	while (tx_len) {
		len = (tx_len > 4) ? 4 : tx_len;
		tx_len -= len;

		/* bit width:8, busrtlen = tx_len */
		SPIM->CTL0 = SPIM_CTL0_NORMAL(SPIM_BITMODE_SINGLE, 1, 8, len);

		/* Tx data */
		while(len) {
			SPIM->TX[--len] = *tx_dat++;
		}

		/* start */
		SPIM->CTL1 |= SPIM_CTL1_SPIMEN_Msk;

		while(SPIM->CTL1 & SPIM_CTL1_SPIMEN_Msk);
	}

	while (rx_len) {
		len = (tx_len > 4) ? 4 : rx_len;
		rx_len -= len;

		/* bit width:8, busrt len = tx_len */
		SPIM->CTL0 = SPIM_CTL0_NORMAL(SPIM_BITMODE_SINGLE, 0, 8, len);

		SPIM->CTL1 |= SPIM_CTL1_SPIMEN_Msk;

		while (SPIM->CTL1 & SPIM_CTL1_SPIMEN_Msk);

        /* Rx data */
        while (len)
		*rx_dat++ = SPIM->RX[--len];
	}

	/* restore CTL0 setting */
	SPIM->CTL0 = spim_ctl_copy;

	/* clear cache, set invalid, CS inactive */
	SPIM->CTL1 |= (SPIM_CTL1_SS_Msk | SPIM_CTL1_CDINVAL_Msk);
}

static void spim_wait_flash_ready(void)
{
	unsigned char dat;
    
	do {
		SPIM_CmdReadWrite(FLASH_CMD_READ_STATUS_REG, 0, 1, NULL, &dat);
	} while (dat & 0x01);
}

static uint8_t spim_program(uint32_t addr, uint8_t *buf, uint32_t len)
{
	unsigned long spim_ctl_copy = SPIM->CTL0;

	while(1) {
		/* write enable automatically send in DMA mode */
		SPIM_CmdReadWrite(FLASH_CMD_WRITE_ENABLE, 0, 0, NULL, NULL);

		/* DMA write */
		SPIM->CTL0 = (FLASH_CMD_PAGE_PROGRAM << SPIM_CTL0_CMDCODE_Pos) |
				     (SPIM_MODE_DMA_WRITE << SPIM_CTL0_OPMODE_Pos) |
				     SPIM_CTL0_CIPHOFF_Msk | 0x02;

		/* DMA */
		SPIM->SRAMADDR = (unsigned long)buf;
		SPIM->DMACNT = (len > 256) ? 256 : len;
		SPIM->FADDR = addr;

		/* start*/
		SPIM->CTL1 |= (SPIM_CTL1_SPIMEN_Msk);
		while (SPIM->CTL1 & SPIM_CTL1_SPIMEN_Msk);

		/* wait flash busy */
		spim_wait_flash_ready();

		/* check length */
		if(len > 256) {
			addr += 256;
			buf += 256;
			len -= 256;
		} else {
			break;
		}
	}
    
	/* restore CTL0 setting */
	SPIM->CTL0 = spim_ctl_copy;

	/* clear cache, set invalid */
	SPIM->CTL1 |= SPIM_CTL1_CDINVAL_Msk;

	return 0;
}

static uint8_t spim_erase_sector(uint32_t addr)
{
	uint8_t dat[3];

	/* write enable */
	SPIM_CmdReadWrite(FLASH_CMD_WRITE_ENABLE, 0, 0, NULL, NULL);

	/* addr to byte array */
	dat[0] = (addr >> 16) & 0xFF;
	dat[1] = (addr >> 8) & 0xFF;
	dat[2] = addr & 0xFF;

	/* erase cmd */
	SPIM_CmdReadWrite(FLASH_CMD_SECTOR_ERASE, 3, 0, dat, NULL);

	/* wait flash busy */
	spim_wait_flash_ready();

	/* clear cache, set invalid */
	SPIM->CTL1 |= SPIM_CTL1_CDINVAL_Msk;

	return 0;
}

static uint8_t spim_get_id(uint32_t *id)
{
	uint8_t jedec_id[3];

	SPIM_CmdReadWrite(FLASH_CMD_READ_JEDEC_ID, 0, 3, NULL, jedec_id);

	*id = (jedec_id[0] << 16) | (jedec_id[1] << 8) | jedec_id[2];

	return 0;
}

static void spim_clk_div(uint32_t div)
{
    SPIM->CTL1 = (SPIM->CTL1 & ~SPIM_CTL1_DIVIDER_Msk) | (div << SPIM_CTL1_DIVIDER_Pos);
}

static void flashloader_init(void)
{
	memset((struct nct_esio_flash_params *)&g_cfg, 0, sizeof(struct nct_esio_flash_params));
	memset((uint8_t *)g_buf, 0, sizeof(g_buf));
	g_cfg.state = FLASH_ALGO_STATE_IDLE;
}

int main(void)
{
	uint32_t id;
	uint8_t status;

	/* Initialize the flashloader parameters and buffer */
	flashloader_init();

	/* Set SPIM clock to 48MHz */
	spim_clk_div(1);

	while (1) {
		while (g_cfg.state == FLASH_ALGO_STATE_IDLE);

		switch (g_cfg.cmd) {
		case FLASH_ALGO_CMD_INIT:
			status = spim_get_id(&id);
			if (status == 0) {
				g_buf[0] = id & 0xff;
				g_buf[1] = (id >> 8) & 0xff;
				g_buf[2] = (id >> 16) & 0xff;
				g_buf[3] = 0x00;
			}
			break;
		case FLASH_ALGO_CMD_ERASE:
			status = spim_erase_sector(g_cfg.addr);
			break;
		case FLASH_ALGO_CMD_PROGRAM:
			status = spim_program(g_cfg.addr, (uint8_t *)g_buf, g_cfg.len);
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

