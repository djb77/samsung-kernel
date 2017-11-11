/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/time.h>
#include <linux/slab.h>

#include "vpu-config.h"
#include "vpu-hardware.h"
#include "../vpu-device.h"
#include "../vpu-vertex.h"
#include "lib/vpul-errno.h"

enum {
	PUID_none,
	PUID_dmain,
	PUID_dmaot,
	PUID_salb,
	PUID_calb,
	PUID_rois,
	PUID_crop,
	PUID_mde,
	PUID_map2list,
	PUID_nms,
	PUID_slf5,
	PUID_slf7,
	PUID_glf5,
	PUID_ccm,
	PUID_lut,
	PUID_integral,
	PUID_upscaler,
	PUID_dnscaler,
	PUID_joiner,
	PUID_spliter,
	PUID_duplicator,
	PUID_histogram,
	PUID_nlf,
	PUID_fastdepth,
	PUID_disparity,
	PUID_inpaint,
	PUID_cnn,
	PUID_fifo
};

#define PU_BLOCK_DEFINE(_block, _name, _count) _block.total = _count
#define MPRB_BLOCK_DEFINE(_block, _name, _count) _block.total = _count

int vpu_hardware_block_init(struct vpu_hw_block *block, u32 total)
{
	u32 i, j;
	struct vpu_hw_channel *channels;

	/* 1st block in table is "dummy" */
	block[0].total = 0;
	block[0].start = 1;

	for (i = 1; i < total; ++i) {
		block[i].start = block[i - 1].total + block[i - 1].start;
		block[i].refer = 0;
		bitmap_zero(block[i].allocated, block[i].total);
		bitmap_zero(block[i].pre_allocated, block[i].total);
		bitmap_zero(block[i].preempted, block[i].total);

		channels = block[i].table;
		for (j = 0; j < block[i].total; ++j) {
			channels[j].flags = 0;
			channels[j].graph = -1;
		}
	}
	return 0;
}

int vpu_hardware_pu_init(struct vpu_hw_pu *pu)
{
	int ret = 0;
	u32 *translator;
	u32 i, j, k, total_pu_cnt;
	struct vpu_hw_block *blocks;

	pu->total = 28;
	blocks = pu->table;

	/* 1. hardware information fill */
	PU_BLOCK_DEFINE(blocks[1], dmain, 5);
	PU_BLOCK_DEFINE(blocks[2], dmaot, 5);
	PU_BLOCK_DEFINE(blocks[3], salb, 9);
	PU_BLOCK_DEFINE(blocks[4], calb, 3);
	PU_BLOCK_DEFINE(blocks[5], rois, 2);
	PU_BLOCK_DEFINE(blocks[6], crop, 3);
	PU_BLOCK_DEFINE(blocks[7], mde, 1);
	PU_BLOCK_DEFINE(blocks[8], map2list, 1);
	PU_BLOCK_DEFINE(blocks[9], nms, 1);
	PU_BLOCK_DEFINE(blocks[10], slf5, 3);
	PU_BLOCK_DEFINE(blocks[11], slf7, 3);
	PU_BLOCK_DEFINE(blocks[12], glf5, 2);
	PU_BLOCK_DEFINE(blocks[13], ccm, 1);
	PU_BLOCK_DEFINE(blocks[14], lut, 1);
	PU_BLOCK_DEFINE(blocks[15], integral, 1);
	PU_BLOCK_DEFINE(blocks[16], upscaler, 1);
	PU_BLOCK_DEFINE(blocks[17], dnscaler, 2);
	PU_BLOCK_DEFINE(blocks[18], joiner, 2);
	PU_BLOCK_DEFINE(blocks[19], spliter, 2);
	PU_BLOCK_DEFINE(blocks[20], duplicator, 5);
	PU_BLOCK_DEFINE(blocks[21], histogram, 1);
	PU_BLOCK_DEFINE(blocks[22], nlf, 1);
	PU_BLOCK_DEFINE(blocks[23], fastdepth, 1);
	PU_BLOCK_DEFINE(blocks[24], disparity, 1);
	PU_BLOCK_DEFINE(blocks[25], inpaint, 1);
	PU_BLOCK_DEFINE(blocks[26], cnn, 1);
	PU_BLOCK_DEFINE(blocks[27], fifo, 13);

	vpu_hardware_block_init(blocks, pu->total);

	total_pu_cnt = 0;
	for (i = 1; i < pu->total; ++i)
		total_pu_cnt += blocks[i].total;

	translator = kmalloc(total_pu_cnt * sizeof(*translator), GFP_KERNEL);
	if (!translator) {
		probe_err("kzalloc is fail\n");
		ret = -ENOMEM;
		goto p_err;
	}

	translator[0] = 0;
	for (i = 1, j = 1; i < pu->total; ++i)
		for (k = 0; k < blocks[i].total; ++k)
			translator[j++] = i;

	pu->translator = translator;

p_err:
	return ret;
}

int vpu_hardware_mprb_init(struct vpu_hw_mprb *mprb)
{
	struct vpu_hw_block *blocks;

	/* 1st block in table is "dummy" */
	mprb->total = 3;
	blocks = mprb->table;

	MPRB_BLOCK_DEFINE(blocks[1], large, 24);
	MPRB_BLOCK_DEFINE(blocks[2], small, 23);

	vpu_hardware_block_init(blocks, mprb->total);

	return 0;
}

int vpu_hardware_init(struct vpu_hardware *hardware)
{
	vpu_hardware_pu_init(&hardware->pu);
	vpu_hardware_mprb_init(&hardware->mprb);
	return VPU_STATUS_SUCCESS;
}