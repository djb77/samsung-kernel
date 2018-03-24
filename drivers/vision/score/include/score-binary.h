/*
 * Samsung Exynos SoC series SCORE driver
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef SCORE_BINARY_H_
#define SCORE_BINARY_H_

#include <linux/device.h>
#include <linux/firmware.h>

#define SCORE_FW_TEXT_NAME				"score_fw_pmw"
#define SCORE_FW_DATA_NAME				"score_fw_dmb"
#define SCORE_FW_EXT_NAME				".bin"
#define SCORE_FW_NAME_LEN				(100)

#define MAX_FW_COUNT					(1)

#define SCORE_FW_CACHE_TEXT_SIZE			(0x00080000) /* 512KB */
#define SCORE_FW_CACHE_DATA_SIZE			(0x00180000) /* 1.5MB */
#define SCORE_FW_CACHE_STACK_SIZE			(0x00008000)

#define SCORE_FW_SRAM_TEXT_SIZE				(0x000020A0)
#define SCORE_FW_SRAM_DATA_SIZE				(0x00005020)
#define SCORE_FW_SRAM_STACK_SIZE			(0x00004000)
#define SCORE_FW_SRAM_INSTACK_SIZE			(0x00002000)

#define FW_STACK_SIZE					(0x8000)
#define FW_SRAM_REGION_SIZE				(32 * 1024)

enum score_fw_type {
	FW_TYPE_CACHE,
	FW_TYPE_SRAM
};

enum score_fw_table_pos {
	FW_TABLE_INDEX			= 0,
	FW_TABLE_TYPE			= 1,
	FW_TABLE_TEXT_SIZE		= 2,
	FW_TABLE_DATA_SIZE		= 3,
	FW_TABLE_STACK_SIZE		= 4,
	FW_TABLE_IN_STACK_SIZE		= 5,
	FW_TABLE_COU_FLAGS		= 6,
	FW_TABLE_MAX
};

enum score_fw_state {
	FW_LD_STAT_SUCCESS = 0,
	FW_LD_STAT_INIT,
	FW_LD_STAT_ONDRAM,
	FW_LD_STAT_FAIL
};

struct score_binary {
	struct device			*dev;
	char				fpath[SCORE_FW_NAME_LEN];

	struct {
		int			reserved	: 22;
		unsigned int		fw_index	: 8;
		bool			fw_cou		: 1;
		bool			fw_loaded	: 1;
	} info;

	void				*target;
	void				*org_target;
	size_t				target_size;
	size_t				image_size;
};

struct score_fw_load_info {
	enum score_fw_type		fw_type;
	enum score_fw_state		fw_state;

	struct score_binary		binary_text;
	void				*fw_text_kvaddr;
	dma_addr_t			fw_text_dvaddr;
	resource_size_t			fw_text_size;
	void				*fw_org_text_kvaddr;

	struct score_binary		binary_data;
	void				*fw_data_kvaddr;
	dma_addr_t			fw_data_dvaddr;
	resource_size_t			fw_data_size;
	void				*fw_org_data_kvaddr;
};

int score_binary_init(struct score_binary *binary, struct device *dev,
		unsigned int index, void *target, void *org_target, size_t target_size);
int score_binary_load(struct score_binary *binary);
int score_binary_copy(const struct firmware *fw_blob,
		struct score_binary *binary);
int score_binary_upload(struct score_binary *binary);
int score_binary_set_text_path(struct score_binary *binary,
		unsigned int index, struct device *dev);
int score_binary_set_data_path(struct score_binary *binary,
		unsigned int index, struct device *dev);

#endif
