/*
 * Samsung Exynos SoC series SCore driver
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/firmware.h>

#include "score_log.h"
#include "score_mem_table.h"
#include "score_system.h"
#include "score_firmware.h"

static int __score_firmware_get_dvaddr(struct score_firmware *fw,
		dma_addr_t *text, dma_addr_t *data)
{
	int ret = 0;

	score_enter();
	if (fw->text_dvaddr == 0 || fw->data_dvaddr == 0) {
		ret = -ENOMEM;
		score_err("dvaddr is not created(core[%d] text:%x, data:%x)\n",
				fw->idx, (unsigned int)fw->text_dvaddr,
				(unsigned int)fw->data_dvaddr);
	} else {
		*text = fw->text_dvaddr;
		*data = fw->data_dvaddr;
	}

	score_leave();
	return ret;
}

/**
 * score_firmware_manager_get_dvaddr -
 *	Get dvaddr of text and data to deliver to SCore device
 * @fwmgr:	[in]	object about score_frame_manager structure
 * @core_id:	[in]	core id of SCore device
 * @text:	[out]	dvaddr for text area
 * @data:	[out]	dvaddr for data area
 *
 * Return 0 if succeeded, otherwise errno
 */
int score_firmware_manager_get_dvaddr(struct score_firmware_manager *fwmgr,
		int core_id, dma_addr_t *text, dma_addr_t *data)
{
	int ret = 0;
	struct score_firmware *fw;

	score_enter();
	if (core_id == SCORE_MASTER) {
		fw = &fwmgr->master;
		ret = __score_firmware_get_dvaddr(fw, text, data);
	} else if (core_id == SCORE_KNIGHT1) {
		fw = &fwmgr->knight1;
		ret = __score_firmware_get_dvaddr(fw, text, data);
	} else if (core_id == SCORE_KNIGHT2) {
		fw = &fwmgr->knight2;
		ret = __score_firmware_get_dvaddr(fw, text, data);
	} else {
		ret = -EINVAL;
	}

	score_leave();
	return ret;
}

int score_firmware_load(struct score_firmware *fw, int stack_size)
{
	int ret = 0;
	const struct firmware *text;
	const struct firmware *data;

	score_enter();
	if (fw->fw_loaded)
		return 0;
	if (fw->text_kvaddr == NULL || fw->data_kvaddr == NULL) {
		ret = -ENOMEM;
		score_err("kvaddr is NULL(core[%d] text(%p), data(%p))\n",
				fw->idx, fw->text_kvaddr, fw->data_kvaddr);
		goto p_err;
	}

	ret = request_firmware(&text, fw->text_path, fw->dev);
	if (ret) {
		score_err("request_firmware(%s) is fail(%d)\n",
				fw->text_path, ret);
		goto p_err;
	}

	if (text->size > fw->text_mem_size) {
		score_err("firmware(%s) size is over(%zd > %zd)\n",
				fw->text_path, text->size, fw->text_mem_size);
		ret = -EIO;
		release_firmware(text);
		goto p_err;
	} else {
		fw->text_fw_size = text->size;
	}

	memcpy(fw->text_kvaddr, text->data, text->size);
	release_firmware(text);

	ret = request_firmware(&data, fw->data_path, fw->dev);
	if (ret) {
		score_err("request_firmware(%s) is fail(%d)\n",
				fw->data_path, ret);
		goto p_err;
	}

	if (data->size > (fw->data_mem_size - stack_size)) {
		score_err("firmware(%s) size is over(%zd > (%zd - %d))\n",
				fw->data_path, data->size,
				fw->data_mem_size, stack_size);
		ret = -EIO;
		release_firmware(data);
		goto p_err;
	} else {
		fw->data_fw_size = data->size;
	}

	memcpy(fw->data_kvaddr + stack_size,
			data->data, data->size);
	release_firmware(data);

	fw->fw_loaded = true;

	score_leave();
	return ret;
p_err:
	fw->fw_loaded = false;
	return ret;
}

void score_firmware_init(struct score_firmware *fw,
		const char *text_name, size_t text_size,
		const char *data_name, size_t data_size)
{
	score_enter();
	fw->fw_loaded = false;

	snprintf(fw->text_path, SCORE_FW_NAME_LEN, "%s", text_name);
	fw->text_mem_size = text_size;

	snprintf(fw->data_path, SCORE_FW_NAME_LEN, "%s", data_name);
	fw->data_mem_size = data_size;

	score_leave();
}

static void __score_firmware_set_memory(struct score_firmware_manager *fwmgr,
		void *kvaddr, dma_addr_t dvaddr)
{
	struct score_firmware *mc, *kc1, *kc2;

	score_enter();
	mc = &fwmgr->master;
	kc1 = &fwmgr->knight1;
	kc2 = &fwmgr->knight2;

	mc->text_kvaddr = kvaddr;
	mc->text_dvaddr = dvaddr;
	mc->data_kvaddr = mc->text_kvaddr + mc->text_mem_size;
	mc->data_dvaddr = mc->text_dvaddr + mc->text_mem_size;

	kc1->text_kvaddr = mc->data_kvaddr + mc->data_mem_size;
	kc1->text_dvaddr = mc->data_dvaddr + mc->data_mem_size;
	kc1->data_kvaddr = kc1->text_kvaddr + kc1->text_mem_size;
	kc1->data_dvaddr = kc1->text_dvaddr + kc1->text_mem_size;

	kc2->text_kvaddr = kc1->data_kvaddr + kc1->data_mem_size;
	kc2->text_dvaddr = kc1->data_dvaddr + kc1->data_mem_size;
	kc2->data_kvaddr = kc2->text_kvaddr + kc2->text_mem_size;
	kc2->data_dvaddr = kc2->text_dvaddr + kc2->text_mem_size;

	score_leave();
}

int score_firmware_manager_open(struct score_firmware_manager *fwmgr,
		void *kvaddr, dma_addr_t dvaddr)
{
	int ret = 0;

	score_enter();
	if (!kvaddr || !dvaddr) {
		ret = -EINVAL;
		score_err("memory for firmware is invalid\n");
		goto p_err;
	}

	score_firmware_init(&fwmgr->master,
			SCORE_MASTER_TEXT_NAME, SCORE_MASTER_TEXT_SIZE,
			SCORE_MASTER_DATA_NAME, SCORE_MASTER_DATA_SIZE);
	score_firmware_init(&fwmgr->knight1,
			SCORE_KNIGHT1_TEXT_NAME, SCORE_KNIGHT1_TEXT_SIZE,
			SCORE_KNIGHT1_DATA_NAME, SCORE_KNIGHT1_DATA_SIZE);
	score_firmware_init(&fwmgr->knight2,
			SCORE_KNIGHT2_TEXT_NAME, SCORE_KNIGHT2_TEXT_SIZE,
			SCORE_KNIGHT2_DATA_NAME, SCORE_KNIGHT2_DATA_SIZE);

	__score_firmware_set_memory(fwmgr, kvaddr, dvaddr);

	ret = score_firmware_load(&fwmgr->master, SCORE_FW_DATA_STACK_SIZE);
	if (ret)
		goto p_err;

	ret = score_firmware_load(&fwmgr->knight1, SCORE_FW_DATA_STACK_SIZE);
	if (ret)
		goto p_err;

	ret = score_firmware_load(&fwmgr->knight2, SCORE_FW_DATA_STACK_SIZE);
	if (ret)
		goto p_err;

	score_leave();
p_err:
	return ret;
}

static void __score_firmware_deinit(struct score_firmware *fw)
{
	score_enter();
	fw->fw_loaded = false;

	fw->text_fw_size = 0;
	fw->text_kvaddr = NULL;
	fw->text_dvaddr = 0;

	fw->data_fw_size = 0;
	fw->data_kvaddr = NULL;
	fw->data_dvaddr = 0;
	score_leave();
}

void score_firmware_manager_close(struct score_firmware_manager *fwmgr)
{
	score_enter();
	__score_firmware_deinit(&fwmgr->knight2);
	__score_firmware_deinit(&fwmgr->knight1);
	__score_firmware_deinit(&fwmgr->master);
	score_leave();
}

int score_firmware_manager_probe(struct score_system *system)
{
	struct score_firmware_manager *fwmgr;
	struct device *dev;

	score_enter();
	fwmgr = &system->fwmgr;
	dev = system->dev;

	fwmgr->master.dev = dev;
	fwmgr->master.idx = SCORE_MASTER;

	fwmgr->knight1.dev = dev;
	fwmgr->knight1.idx = SCORE_KNIGHT1;

	fwmgr->knight2.dev = dev;
	fwmgr->knight2.idx = SCORE_KNIGHT2;

	score_leave();
	return 0;
}

void score_firmware_manager_remove(struct score_firmware_manager *fwmgr)
{
	score_enter();
	score_leave();
}
