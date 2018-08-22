/*
 * Samsung Exynos SoC series SCore driver
 *
 * Firmware manager module for firmware binary that SCore device executes
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __SCORE_FIRMWARE_H__
#define __SCORE_FIRMWARE_H__

#include <linux/device.h>

struct score_system;

/* Definition of firmware for three core of SCore device */
#define SCORE_MASTER_TEXT_NAME		"score_mc_pmw.bin"
#define SCORE_MASTER_DATA_NAME		"score_mc_dmb.bin"
#define SCORE_KNIGHT1_TEXT_NAME		"score_kc1_pmw.bin"
#define SCORE_KNIGHT1_DATA_NAME		"score_kc1_dmb.bin"
#define SCORE_KNIGHT2_TEXT_NAME		"score_kc2_pmw.bin"
#define SCORE_KNIGHT2_DATA_NAME		"score_kc2_dmb.bin"
#define SCORE_FW_NAME_LEN		(30)

/**
 * Data stack size. This value depends on SCore H/W
 */
#define SCORE_FW_DATA_STACK_SIZE	(0x8000)

/**
 * enum score_core_id - Unique id of three core included in SCore device
 * @SCORE_MASTER: master core id - 0
 * @SCORE_KNIGHT1: knight1 core id - 1
 * @SCORE_KNIGHT2: knight2 core id - 2
 * @SCORE_CORE_NUM: the number of core included in SCore device
 */
enum score_core_id {
	SCORE_MASTER = 0,
	SCORE_KNIGHT1,
	SCORE_KNIGHT2,
	SCORE_CORE_NUM
};

/**
 * struct score_firmware - Object including firmware information
 * @dev: device structure registered in platform
 * @idx: core id
 * @fw_loaded: state that firmware is loaded(true) or not(false)
 * @text_path: name of pm binary
 * @text_mem_size: size of memory allocated for pm binary
 * @text_fw_size: size of pm binary
 * @text_kvaddr: kernel virtual address of memory allocated for pm binary
 * @text_dvaddr: device virtual address of memory allocated for pm binary
 * @data_path: name of dm binary
 * @data_mem_size: size of memory allocated for dm binary
 * @data_fw_size: size of dm binary
 * @data_kvaddr: kernel virtual address of memory allocated for dm binary
 * @data_dvaddr: device virtual address of memory allocated for dm binary
 */
struct score_firmware {
	struct device			*dev;
	int				idx;
	bool				fw_loaded;

	char			        text_path[SCORE_FW_NAME_LEN];
	size_t			        text_mem_size;
	size_t			        text_fw_size;
	void				*text_kvaddr;
	dma_addr_t		        text_dvaddr;

	char			        data_path[SCORE_FW_NAME_LEN];
	size_t			        data_mem_size;
	size_t			        data_fw_size;
	void				*data_kvaddr;
	dma_addr_t		        data_dvaddr;
};

/**
 * struct score_firmware_manager -
 *	Manager controlling information of each firmware
 * @master: score_firmware structure for master core
 * @knight1: score_firmware structure for knight1 core
 * @knight2: score_firmware structure for knight2 core
 */
struct score_firmware_manager {
	struct score_firmware		master;
	struct score_firmware		knight1;
	struct score_firmware		knight2;
};

int score_firmware_manager_get_dvaddr(struct score_firmware_manager *fwmgr,
		int core_id, dma_addr_t *text_addr, dma_addr_t *data_addr);
int score_firmware_load(struct score_firmware *fw, int stack_size);

void score_firmware_init(struct score_firmware *fw,
		const char *text_name, size_t text_size,
		const char *data_name, size_t data_size);
int score_firmware_manager_open(struct score_firmware_manager *fwmgr,
		void *kvaddr, dma_addr_t dvaddr);
void score_firmware_manager_close(struct score_firmware_manager *fwmgr);

int score_firmware_manager_probe(struct score_system *system);
void score_firmware_manager_remove(struct score_firmware_manager *fwmgr);

#endif
