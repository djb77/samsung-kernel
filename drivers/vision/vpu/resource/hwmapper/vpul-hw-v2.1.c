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
#include <linux/bitmap.h>

#include "vpu-hardware.h"
#include "lib/vpul-hwmapper.h"
#include "lib/vpul-errno.h"
#include "lib/vpul-hw-v2.1.h"

#define PU_BLOCK_DEFINE(_block, _name, _count) _block.total = _count
#define MPRB_BLOCK_DEFINE(_block, _name, _count) _block.total = _count

static __s32 vpu_hardware_block_init(struct vpu_hw_block *block, __u32 total)
{
	__u32 i, j;
	struct vpu_hw_channel *channels;

	BUG_ON(block == NULL);

	/* 1st block in table is "dummy" */
	block[0].total = 0;
	block[0].start = 1;

	for (i = 1; i < total; ++i) {
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
	return VPU_STATUS_SUCCESS;
}

static __s32 vpu_hardware_pu_init(struct vpu_hw_pu *pu)
{
	__u32 i, j, last_index;
	__s32 ret_val = VPU_STATUS_SUCCESS;
	struct vpu_hw_block *blocks;

	pu->total = 0;
	blocks = pu->table;
	pu->translator = (__u32 *)pu_inst2type;

	for (i = 0; i < VPU_HW_MAX_PU_COUNT; i++)
		blocks[i].total = 0;

	/* hardware information fill */
	last_index = 0;
	BUG_ON(pu_inst2type[0] == END_OF_PU_INST2TYPE_TRANSLATOR);
	i = 1;
	while (pu_inst2type[i] != END_OF_PU_INST2TYPE_TRANSLATOR) {
		j = (__u32)pu_inst2type[i];
		if (j > last_index)
			last_index = j;
		if (blocks[j].total == 0)
			blocks[j].start = i;
		blocks[j].total++;
		i++;
	}
	pu->total = last_index + 1;

	ret_val = vpu_hardware_block_init(blocks, pu->total);
	BUG_ON(ret_val != VPU_STATUS_SUCCESS);
	return ret_val;
}

static __s32 vpu_hardware_mprb_init(struct vpu_hw_mprb *mprb)
{
	struct vpu_hw_block *blocks;

	/* 1st block in table is "dummy" */
	mprb->total = 3;
	blocks = mprb->table;

	MPRB_BLOCK_DEFINE(blocks[1], large, 24);
	MPRB_BLOCK_DEFINE(blocks[2], small, 23);

	vpu_hardware_block_init(blocks, mprb->total);
	blocks[1].start = 0;
	blocks[2].start = blocks[1].total;

	return VPU_STATUS_SUCCESS;
}

__s32 vpu_hardware_init(struct vpu_hardware *hardware)
{
	__s32 ret_val = VPU_STATUS_SUCCESS;

	if (hardware == NULL)
		return VPU_STATUS_BAD_PARAMS;

	ret_val = vpu_hardware_pu_init(&hardware->pu);
	BUG_ON(ret_val != VPU_STATUS_SUCCESS);
	ret_val = vpu_hardware_mprb_init(&hardware->mprb);
	BUG_ON(ret_val != VPU_STATUS_SUCCESS);
	return ret_val;
}