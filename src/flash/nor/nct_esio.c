// SPDX-License-Identifier: GPL-2.0-or-later

/*
 * Copyright (C) 2025 by Nuvoton Technology Corporation
 * Ming Yu <tmyu0@nuvoton.com>
 * 
 * Based on npcx.c
 * Copyright (C) 2020 by Nuvoton Technology Corporation
 * Mulin Chao <mlchao@nuvoton.com>
 * Wealian Liao <WHLIAO@nuvoton.com>
 * Luca	Hung	<YCHUNG0@nuvoton.com>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "imp.h"
#include "nct_esio.h"
#include <helper/binarybuffer.h>
#include <helper/time_support.h>
#include <target/armv7m.h>

/* nct_esio SPIM flash loader */
static const uint8_t nct_esio_spim_algo[] = {
#include "../../../contrib/loaders/flash/nct_esio/nct_esio_spim_algo.inc"
};

/* nct_esio FIU flash loader */
static const uint8_t nct_esio_fiu_algo[] = {
#include "../../../contrib/loaders/flash/nct_esio/nct_esio_fiu_algo.inc"
};

struct nct_esio_flash_info {
	char *name;
	uint32_t id;
	uint32_t size;
	uint8_t adr_4b;
};

static const struct nct_esio_flash_info flash_info[] = {
	{ "NCT6812D Flash", 0xEF4013, 512 * 1024, 0 },
	{ "NCT6832D Flash", 0xEF4014, 1024 * 1024, 0 },
	{ "NCT6694D Flash", 0xEF4015, 2048 * 1024, 0 },
};

struct nct_esio_flash_bank {
	enum nct_esio_flash_type type;
	struct nct_esio_flash_info flash;
	struct working_area *working_area;
	struct armv7m_algorithm armv7m_info;
	const uint8_t *algo_code;
	uint32_t algo_size;
	uint32_t algo_working_size;
	uint32_t buffer_addr;
	uint32_t params_addr;
	bool probed;
};

static int nct_esio_init(struct flash_bank *bank)
{
	struct nct_esio_flash_bank *nct_esio_bank = bank->driver_priv;
	struct target *target = bank->target;

	target_free_working_area(target, nct_esio_bank->working_area);
	nct_esio_bank->working_area = NULL;

	int retval = target_alloc_working_area(target,
										   nct_esio_bank->algo_working_size,
										   &nct_esio_bank->working_area);
	if (retval != ERROR_OK)
		return retval;

	if (nct_esio_bank->working_area->address != NCT_ESIO_FLASH_ALGO_BASE_ADDR) {
		LOG_TARGET_ERROR(target, "Invalid working address");
		target_free_working_area(target, nct_esio_bank->working_area);
		nct_esio_bank->working_area = NULL;
		return ERROR_TARGET_RESOURCE_NOT_AVAILABLE;
	}

	retval = target_write_buffer(target, NCT_ESIO_FLASH_ALGO_PROGRAM_ADDR,
								 nct_esio_bank->algo_size, nct_esio_bank->algo_code);
	if (retval != ERROR_OK) {
		LOG_TARGET_ERROR(target, "Failed to load flash helper algorithm");
		target_free_working_area(target, nct_esio_bank->working_area);
		nct_esio_bank->working_area = NULL;
		return retval;
	}

	nct_esio_bank->armv7m_info.common_magic = ARMV7M_COMMON_MAGIC;
	nct_esio_bank->armv7m_info.core_mode = ARM_MODE_THREAD;

	retval = target_start_algorithm(target, 0, NULL, 0, NULL,
									NCT_ESIO_FLASH_ALGO_PROGRAM_ADDR, 0,
									&nct_esio_bank->armv7m_info);
	if (retval != ERROR_OK) {
		LOG_TARGET_ERROR(target, "Failed to start flash helper algorithm");
		target_free_working_area(target, nct_esio_bank->working_area);
		nct_esio_bank->working_area = NULL;
		return retval;
	}

	return retval;
}

static int nct_esio_quit(struct flash_bank *bank)
{
	struct nct_esio_flash_bank *nct_esio_bank = bank->driver_priv;
	struct target *target = bank->target;

	(void)target_halt(target);

	int retval = target_wait_algorithm(target, 0, NULL, 0, NULL, 0,
									   NCT_ESIO_FLASH_TIMEOUT_MS,
									   &nct_esio_bank->armv7m_info);

	target_free_working_area(target, nct_esio_bank->working_area);
	nct_esio_bank->working_area = NULL;

	return retval;
}

static int nct_esio_wait_algo_done(struct flash_bank *bank,
								  uint32_t params_addr)
{
	struct target *target = bank->target;
	uint32_t status_addr = params_addr + offsetof(struct nct_esio_flash_algo_params, state);
	int64_t start_ms = timeval_ms();
	uint8_t status;

	do {
		int retval = target_read_u8(target, status_addr, &status);
		if (retval != ERROR_OK)
			return retval;

		keep_alive();

		int64_t elapsed_ms = timeval_ms() - start_ms;
		if (elapsed_ms > NCT_ESIO_FLASH_TIMEOUT_MS)
			break;
	} while (status == NCT_ESIO_FLASH_ALGO_STATE_BUSY);

	if (status != NCT_ESIO_FLASH_ALGO_STATE_IDLE) {
		LOG_TARGET_ERROR(target, "Flash operation failed, status (%0x" PRIX32 ") ", status);
		return ERROR_FAIL;
	}

	return ERROR_OK;
}

static int nct_esio_get_flash_info(struct flash_bank *bank, uint32_t *flash_id,
								  uint32_t *flash_size)
{
	struct nct_esio_flash_bank *nct_esio_bank = bank->driver_priv;
	struct nct_esio_flash_algo_params algo_params;
	struct target *target = bank->target;
	int retval;

	if (target->state != TARGET_HALTED) {
		LOG_ERROR("%s: Target not halted", __func__);
		return ERROR_TARGET_NOT_HALTED;
	}

	retval = nct_esio_init(bank);
	if (retval != ERROR_OK)
		return retval;

	/* Initialize algorithm parameters to default values */
	algo_params.type = nct_esio_bank->type;
	algo_params.cmd = NCT_ESIO_FLASH_ALGO_CMD_INIT;
	algo_params.state = NCT_ESIO_FLASH_ALGO_STATE_IDLE;

	retval = target_write_buffer(target, nct_esio_bank->params_addr,
				sizeof(algo_params), (uint8_t *)&algo_params);
	if (retval != ERROR_OK) {
		(void)nct_esio_quit(bank);
		return retval;
	}

	/* Set algorithm parameters */
	algo_params.state = NCT_ESIO_FLASH_ALGO_STATE_BUSY;

	retval = target_write_buffer(target, nct_esio_bank->params_addr,
				sizeof(algo_params), (uint8_t *)&algo_params);
	if (retval != ERROR_OK) {
		(void)nct_esio_quit(bank);
		return retval;
	}

	/* Wait for algorithm to finish */
	retval = nct_esio_wait_algo_done(bank, nct_esio_bank->params_addr);
	if (retval != ERROR_OK) {
		(void)nct_esio_quit(bank);
		return retval;
	}

	/* Read flash informations */
	target_read_u32(target, nct_esio_bank->buffer_addr, flash_id);
	target_read_u32(target, nct_esio_bank->buffer_addr + 4, flash_size);
	target_read_u8(target, nct_esio_bank->buffer_addr + 8, &nct_esio_bank->flash.adr_4b);

	retval = nct_esio_quit(bank);
	if (retval != ERROR_OK)
		return retval;

	return ERROR_OK;
}

static int nct_esio_erase_sector(struct flash_bank *bank, int sector)
{
	struct nct_esio_flash_bank *nct_esio_bank = bank->driver_priv;
	struct nct_esio_flash_algo_params algo_params;
	struct target *target = bank->target;
	int retval;

	LOG_INFO("%s: Erasing sector %d at 0x%08x, size %d bytes", __func__, sector,
															   bank->sectors[sector].offset,
															   bank->sectors[sector].size);

	/* Initialize algorithm parameters to default values */
	algo_params.addr = bank->sectors[sector].offset;
	algo_params.len = bank->sectors[sector].size;
	algo_params.type = nct_esio_bank->type;
	algo_params.adr_4b = nct_esio_bank->flash.adr_4b;
	algo_params.cmd = NCT_ESIO_FLASH_ALGO_CMD_ERASE;
	algo_params.state = NCT_ESIO_FLASH_ALGO_STATE_IDLE;

	retval = target_write_buffer(target, nct_esio_bank->params_addr,
				     sizeof(algo_params), (uint8_t *)&algo_params);
	if (retval != ERROR_OK)
		return retval;

	/* Set algorithm parameters */
	algo_params.state = NCT_ESIO_FLASH_ALGO_STATE_BUSY;

	retval = target_write_buffer(target, nct_esio_bank->params_addr,
				     sizeof(algo_params), (uint8_t *)&algo_params);
	if (retval != ERROR_OK)
		return retval;

	return nct_esio_wait_algo_done(bank, nct_esio_bank->params_addr);
}

static int nct_esio_flash_erase(struct flash_bank *bank, unsigned int first,
							   unsigned int last)
{
	struct nct_esio_flash_bank *nct_esio_bank = bank->driver_priv;
	struct target *target = bank->target;
	int retval;

	LOG_INFO("%s: Erasing sectors %d to %d", __func__, first, last);

	if (target->state != TARGET_HALTED) {
		LOG_ERROR("Target not halted");
		return ERROR_TARGET_NOT_HALTED;
	}

	if (!(nct_esio_bank->probed)) {
		LOG_ERROR("Flash bank not probed");
		return ERROR_FLASH_BANK_NOT_PROBED;
	}

	retval = nct_esio_init(bank);
	if (retval != ERROR_OK)
		return retval;

	for (unsigned int sector = first; sector <= last; sector++) {
		retval = nct_esio_erase_sector(bank, sector);
		if (retval != ERROR_OK)
			goto done;
		keep_alive();
	}

done:
	(void)nct_esio_quit(bank);

	return retval;
}

static int nct_esio_flash_write(struct flash_bank *bank, const uint8_t *buffer,
							   uint32_t offset, uint32_t count)
{
	struct nct_esio_flash_bank *nct_esio_bank = bank->driver_priv;
	struct nct_esio_flash_algo_params algo_params;
	struct target *target = bank->target;
	int retval;

	LOG_INFO("%s: Writing %d bytes to 0x%08x", __func__, count, offset);

	if (target->state != TARGET_HALTED) {
		LOG_ERROR("Target not halted");
		return ERROR_TARGET_NOT_HALTED;
	}

	if (!(nct_esio_bank->probed)) {
		LOG_ERROR("Flash bank not probed");
		return ERROR_FLASH_BANK_NOT_PROBED;
	}

	retval = nct_esio_init(bank);
	if (retval != ERROR_OK)
		return retval;

	uint32_t address = offset;

	while (count > 0) {
		uint32_t size = (count > NCT_ESIO_FLASH_ALGO_BUFFER_SIZE) ?
						 NCT_ESIO_FLASH_ALGO_BUFFER_SIZE : count;

		/* Write data to target memory */
		retval = target_write_buffer(target, nct_esio_bank->buffer_addr,
					     size, buffer);
		if (retval != ERROR_OK) {
			LOG_ERROR("Unable to write data to target memory");
			break;
		}

		/* Initialize algorithm parameters to default values */
		algo_params.addr = address;
		algo_params.len = size;
		algo_params.type = nct_esio_bank->type;
		algo_params.adr_4b = nct_esio_bank->flash.adr_4b;
		algo_params.cmd = NCT_ESIO_FLASH_ALGO_CMD_PROGRAM;
		algo_params.state = NCT_ESIO_FLASH_ALGO_STATE_IDLE;

		LOG_INFO("%s: Writing %d bytes to 0x%08x", __func__, algo_params.len,
												  algo_params.addr);

		retval = target_write_buffer(target, nct_esio_bank->params_addr,
					     sizeof(algo_params), (uint8_t *)&algo_params);
		if (retval != ERROR_OK)
			break;

		/* Set algorithm parameters */
		algo_params.state = NCT_ESIO_FLASH_ALGO_STATE_BUSY;

		retval = target_write_buffer(target, nct_esio_bank->params_addr,
					     sizeof(algo_params), (uint8_t *)&algo_params);
		if (retval != ERROR_OK)
			break;

		/* Wait for flash write to complete */
		retval = nct_esio_wait_algo_done(bank, nct_esio_bank->params_addr);
		if (retval != ERROR_OK)
			break;

		count -= size;
		buffer += size;
		address += size;
	}

	(void)nct_esio_quit(bank);

	return retval;
}

static int nct_esio_flash_read(struct flash_bank *bank, uint8_t *buffer,
							  uint32_t offset, uint32_t count)
{
	struct nct_esio_flash_bank *nct_esio_bank = bank->driver_priv;
	struct nct_esio_flash_algo_params algo_params;
	struct target *target = bank->target;
	int retval;

	LOG_INFO("%s: Reading %d bytes from 0x%08x", __func__, count, offset);

	if (target->state != TARGET_HALTED) {
		LOG_ERROR("Target not halted");
		return ERROR_TARGET_NOT_HALTED;
	}

	if (!(nct_esio_bank->probed)) {
		LOG_ERROR("Flash bank not probed");
		return ERROR_FLASH_BANK_NOT_PROBED;
	}

	/* Check if flash supports 4-byte addressing */
	if (nct_esio_bank->flash.adr_4b) {
		retval = nct_esio_init(bank);
		if (retval != ERROR_OK)
			return retval;

		uint32_t address = offset;

		while (count > 0) {
			uint32_t size = (count > NCT_ESIO_FLASH_ALGO_BUFFER_SIZE) ?
							 NCT_ESIO_FLASH_ALGO_BUFFER_SIZE : count;

			/* Initialize algorithm parameters to default values */
			algo_params.addr = address;
			algo_params.len = size;
			algo_params.type = nct_esio_bank->type;
			algo_params.adr_4b = nct_esio_bank->flash.adr_4b;
			algo_params.cmd = NCT_ESIO_FLASH_ALGO_CMD_READ;
			algo_params.state = NCT_ESIO_FLASH_ALGO_STATE_IDLE;

			retval = target_write_buffer(target, nct_esio_bank->params_addr,
									     sizeof(algo_params), (uint8_t *)&algo_params);
			if (retval != ERROR_OK)
				break;

			/* Set algorithm parameters */
			algo_params.state = NCT_ESIO_FLASH_ALGO_STATE_BUSY;

			retval = target_write_buffer(target, nct_esio_bank->params_addr,
									     sizeof(algo_params), (uint8_t *)&algo_params);
			if (retval != ERROR_OK)
				break;

			/* Wait for flash read to complete */
			retval = nct_esio_wait_algo_done(bank, nct_esio_bank->params_addr);
			if (retval != ERROR_OK) {
				break;
			} else {
				retval = target_read_buffer(target, nct_esio_bank->buffer_addr, size, buffer);
				if (retval != ERROR_OK) {
					break;
				}
			}

			count -= size;
			buffer += size;
			address += size;
		}

		(void)nct_esio_quit(bank);
	} else {
		retval = target_read_buffer(bank->target, offset + bank->base, count, buffer);
		if (retval != ERROR_OK) {
			LOG_ERROR("Failed to read data from target memory");
		}
	}

	return retval;
}

static int nct_esio_flash_probe(struct flash_bank *bank)
{
	struct nct_esio_flash_bank *nct_esio_bank = bank->driver_priv;
	uint32_t sector_length = NCT_ESIO_FLASH_SECTOR_SIZE;
	uint32_t flash_id, flash_size, num_sectors;
	int retval;

	LOG_INFO("%s: Probing flash bank: %s", __func__, bank->name);

	nct_esio_bank->probed = false;

	/* Set up appropriate flash helper algorithm */
	switch (bank->base) {
	case NCT_ESIO_INTERNAL_FLASH_ADDR:
		nct_esio_bank->type = NCT_ESIO_FLASH_INTERNAL;
		break;
	case NCT_ESIO_PRIVATE_FLASH_ADDR:
		nct_esio_bank->type = NCT_ESIO_FLASH_PRIVATE;
		break;
	case NCT_ESIO_SHARED_FLASH_ADDR:
		nct_esio_bank->type = NCT_ESIO_FLASH_SHARED;
		break;
	case NCT_ESIO_BACKUP_FLASH_ADDR:
		nct_esio_bank->type = NCT_ESIO_FLASH_BACKUP;
		break;
	default:
		LOG_ERROR("%s: Invalid flash bank address", __func__);
		return ERROR_FAIL;
	}

	if (nct_esio_bank->type == NCT_ESIO_FLASH_INTERNAL) {
		nct_esio_bank->algo_code = nct_esio_spim_algo;
		nct_esio_bank->algo_size = sizeof(nct_esio_spim_algo);
	} else {
		nct_esio_bank->algo_code = nct_esio_fiu_algo;
		nct_esio_bank->algo_size = sizeof(nct_esio_fiu_algo);
	}

	nct_esio_bank->algo_working_size = NCT_ESIO_FLASH_ALGO_PARAMS_SIZE +
									   NCT_ESIO_FLASH_ALGO_BUFFER_SIZE +
									   NCT_ESIO_FLASH_ALGO_PROGRAM_SIZE;
	nct_esio_bank->params_addr = NCT_ESIO_FLASH_ALGO_PARAMS_ADDR;
	nct_esio_bank->buffer_addr = NCT_ESIO_FLASH_ALGO_BUFFER_ADDR;

	/* Get flash ID and size */
	retval = nct_esio_get_flash_info(bank, &flash_id, &flash_size);
	if (retval != ERROR_OK)
		return retval;

	/* Check flash info is valid */
	if (flash_size) {
		nct_esio_bank->flash.name = "Unknown Flash";
		nct_esio_bank->flash.id = flash_id;
		nct_esio_bank->flash.size = flash_size;
	} else {
		for (uint32_t i = 0; i < ARRAY_SIZE(flash_info); i++) {
			if (flash_info[i].id == flash_id) {
			nct_esio_bank->flash = flash_info[i];
			break;
			}
		}

		if (!nct_esio_bank->flash.name) {
			LOG_ERROR("Failed to find flash info for ID: 0x%08x", flash_id);
			return ERROR_FAIL;
		}
	}

	LOG_INFO("%s: Flash info: id = 0x%08x, size = 0x%08x", __func__,
														   nct_esio_bank->flash.id,
														   nct_esio_bank->flash.size);

	num_sectors = nct_esio_bank->flash.size / sector_length;

	bank->sectors = calloc(num_sectors, sizeof(struct flash_sector));
	if (!bank->sectors) {
		LOG_ERROR("Out of memory");
		return ERROR_FAIL;
	}

	bank->num_sectors = num_sectors;
	bank->size = num_sectors * sector_length;
	bank->write_start_alignment = 0;
	bank->write_end_alignment = 0;

	for (unsigned int i = 0; i < num_sectors; i++) {
		bank->sectors[i].offset = i * sector_length;
		bank->sectors[i].size = sector_length;
		bank->sectors[i].is_erased = -1;
		bank->sectors[i].is_protected = 0;
	}

	nct_esio_bank->probed = true;

	return ERROR_OK;
}

static int nct_esio_flash_auto_probe(struct flash_bank *bank)
{
	struct nct_esio_flash_bank *nct_esio_bank = bank->driver_priv;

	if (nct_esio_bank->probed)
		return ERROR_OK;

	return nct_esio_flash_probe(bank);
}

FLASH_BANK_COMMAND_HANDLER(nct_esio_flash_bank_command)
{
	struct nct_esio_flash_bank *nct_esio_bank;

	if (CMD_ARGC != 6)
		return ERROR_COMMAND_SYNTAX_ERROR;

	nct_esio_bank = calloc(1, sizeof(struct nct_esio_flash_bank));
	if (!nct_esio_bank) {
		LOG_ERROR("Out of memory");
		return ERROR_FAIL;
	}

	bank->driver_priv = nct_esio_bank;

	nct_esio_bank->probed = false;

	LOG_INFO("nct_esio flash bank initialized");

	return ERROR_OK;
}

static int get_nct_esio_info(struct flash_bank *bank,
							struct command_invocation *cmd)
{
	struct nct_esio_flash_bank *nct_esio_bank = bank->driver_priv;

	if (!(nct_esio_bank->probed)) {
		command_print_sameline(cmd, "\nnct_esio flash bank not probed yet\n");
		return ERROR_OK;
	}

	command_print_sameline(cmd, "\nnct_esio flash information:\n"
			"  Device \'%s\' (ID 0x%08" PRIx32 ")\n",
			nct_esio_bank->flash.name, nct_esio_bank->flash.id);

	return ERROR_OK;
}

const struct flash_driver nct_esio_flash = {
	.name = "nct_esio",
	.flash_bank_command = nct_esio_flash_bank_command,
	.erase = nct_esio_flash_erase,
	.write = nct_esio_flash_write,
	.read = nct_esio_flash_read,
	.probe = nct_esio_flash_probe,
	.auto_probe = nct_esio_flash_auto_probe,
	.erase_check = default_flash_blank_check,
	.info = get_nct_esio_info,
	.free_driver_priv = default_flash_free_driver_priv,
};

