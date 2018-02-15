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

#include <linux/syscalls.h>
#include <linux/vmalloc.h>

#include "score-config.h"
#include "score-binary.h"
#include "score-debug.h"

int score_binary_init(struct score_binary *binary, struct device *dev,
		unsigned int index, void *target,
		void *org_target, size_t target_size)
{
	int ret = 0;

	binary->dev		= dev;
	binary->target		= target;
	binary->org_target	= org_target;
	binary->target_size	= target_size;
	binary->image_size	= 0;

	binary->info.fw_cou = (org_target == NULL) ? false : true;
	binary->info.fw_index = index;
	binary->info.fw_loaded = false;

	return ret;
}

int score_binary_set_text_path(struct score_binary *binary,
		unsigned int index, struct device *dev)
{
	int ret = 0;

	snprintf(binary->fpath, sizeof(binary->fpath),
			"%s%d%s",
			SCORE_FW_TEXT_NAME, index, SCORE_FW_EXT_NAME);

	return ret;
}

int score_binary_set_data_path(struct score_binary *binary,
		unsigned int index, struct device *dev)
{
	int ret = 0;

	snprintf(binary->fpath, sizeof(binary->fpath),
			"%s%d%s",
			SCORE_FW_DATA_NAME, index, SCORE_FW_EXT_NAME);

	return ret;
}

int score_binary_copy(const struct firmware *fw_blob,
		struct score_binary *binary)
{
	int ret = 0;
	void *addr;

	if (!fw_blob->data) {
		score_err("fw_blob->data is NULL\n");
		ret = -EINVAL;
		goto request_err;
	}

	if (fw_blob->size > binary->target_size) {
		score_err("image size is over(%ld > %ld)\n",
				binary->image_size, binary->target_size);
		ret = -EIO;
		goto request_err;
	}

	addr = (binary->info.fw_cou) ? binary->org_target : binary->target;

	binary->image_size = fw_blob->size;
	memcpy(addr, fw_blob->data, fw_blob->size);

request_err:
	return ret;
}

int score_binary_load(struct score_binary *binary)
{
	int ret = 0;
	const struct firmware *fw_blob;

	if (binary->info.fw_loaded)
		return 0;

	ret = request_firmware(&fw_blob, binary->fpath, binary->dev);
	if (ret) {
		score_err("request_firmware(%s) is fail(%d)\n",
				binary->fpath, ret);
		ret = -EINVAL;
		goto p_err;
	}

	if (!fw_blob) {
		score_err("fw_blob is NULL\n");
		ret = -EINVAL;
		goto p_err;
	}

	ret = score_binary_copy(fw_blob, binary);

	release_firmware(fw_blob);
p_err:
	if (ret) {
		binary->info.fw_loaded = false;
	} else {
		binary->info.fw_loaded = true;
	}
	return ret;
}

int score_binary_upload(struct score_binary *binary)
{
	if (!binary->info.fw_cou)
		return -EINVAL;
	if (binary->target == NULL || binary->org_target == NULL)
		return -EFAULT;
	memcpy(binary->target, binary->org_target, binary->image_size);
	return 0;
}
