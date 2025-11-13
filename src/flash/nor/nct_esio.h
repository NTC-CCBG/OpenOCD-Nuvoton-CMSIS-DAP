/* SPDX-License-Identifier: GPL-2.0-or-later */

/****************************************************************************
 *	Copyright (C) 2025 by Nuvoton Technolgy Corp							*
 ****************************************************************************/

#ifndef OPENOCD_FLASH_NOR_NCT_ESIO_H
#define OPENOCD_FLASH_NOR_NCT_ESIO_H

#define NCT_ESIO_INTERNAL_FLASH_ADDR		0x00080000U
#define NCT_ESIO_PRIVATE_FLASH_ADDR		0x60000000U
#define NCT_ESIO_SHARED_FLASH_NCT6692_ADDR	0x64000000U
#define NCT_ESIO_SHARED_FLASH_NCT6694_ADDR	0x70000000U
#define NCT_ESIO_BACKUP_FLASH_ADDR		0x80000000U

#define NCT_ESIO_FLASH_ALGO_BASE_ADDR		0x200C0000U
#define NCT_ESIO_FLASH_ALGO_PARAMS_ADDR		NCT_ESIO_FLASH_ALGO_BASE_ADDR
#define NCT_ESIO_FLASH_ALGO_PARAMS_SIZE		20
#define NCT_ESIO_FLASH_ALGO_BUFFER_ADDR		(NCT_ESIO_FLASH_ALGO_PARAMS_ADDR + NCT_ESIO_FLASH_ALGO_PARAMS_SIZE)
#define NCT_ESIO_FLASH_ALGO_BUFFER_SIZE		0x1000
#define NCT_ESIO_FLASH_ALGO_PROGRAM_ADDR	(NCT_ESIO_FLASH_ALGO_BUFFER_ADDR + NCT_ESIO_FLASH_ALGO_BUFFER_SIZE)
#define NCT_ESIO_FLASH_ALGO_PROGRAM_SIZE	0x1000

#define NCT_ESIO_FLASH_TIMEOUT_MS		8000
#define NCT_ESIO_FLASH_SECTOR_SIZE		4096

enum nct_esio_flash_type {
	NCT_ESIO_FLASH_PRIVATE = 0,
	NCT_ESIO_FLASH_SHARED,
	NCT_ESIO_FLASH_BACKUP,
	NCT_ESIO_FLASH_INTERNAL = 0xFF,
};

enum nct_esio_flash_algo_state {
	NCT_ESIO_FLASH_ALGO_STATE_IDLE = 0,
	NCT_ESIO_FLASH_ALGO_STATE_BUSY,
};

enum nct_esio_flash_algo_cmd {
	NCT_ESIO_FLASH_ALGO_CMD_INIT = 0,
	NCT_ESIO_FLASH_ALGO_CMD_ERASE,
	NCT_ESIO_FLASH_ALGO_CMD_PROGRAM,
	NCT_ESIO_FLASH_ALGO_CMD_READ,
	NCT_ESIO_FLASH_ALGO_CMD_VERIFY,
};

struct nct_esio_flash_algo_params {
	uint8_t state;		/* Flash loader state */
	uint8_t cmd;		/* Flash loader command */
	uint8_t type;		/* Flash type */
	uint8_t adr_4b;		/* 4-byte address mode */
	uint32_t version;	/* Flash loader version */
	uint32_t addr;		/* Address in flash */
	uint32_t len;		/* Number of bytes */
	uint32_t reserved;	/* Reserved */
};

#endif /* OPENOCD_FLASH_NOR_NCT_ESIO_H */

