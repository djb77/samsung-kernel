/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/slab.h>
#include <linux/time.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/kernel.h>

#include "vs4l.h"
#include "vpu-config.h"
#include "vpu-resource.h"
#include "vpu-hardware.h"

static int __vpu_resource_pu_get(struct vpu_hw_pu *pu_device, struct vpul_pu *pu, unsigned long flags)
{
	int ret = 0;
	u32 block_id, channel_id;
	struct vpu_hw_block *block;

	BUG_ON(!pu_device);
	BUG_ON(!pu);

	if (pu->instance >= VPU_PU_NUMBER) {
		vpu_err("pu id is invalid(%d)\n", pu->instance);
		ret = -EINVAL;
		goto p_err;
	}

	block_id = pu_device->translator[pu->instance];
	block = &pu_device->table[block_id];

	if (test_bit(VS4L_GRAPH_FLAG_FIXED, &flags)) {
		channel_id = pu->instance - block->start;
		if (test_bit(channel_id, block->preempted)) {
			ret = -pu->instance;
			goto p_err;
		}
	} else {
		channel_id = find_first_zero_bit(block->preempted, block->total);
		if (channel_id >= block->total) {
			ret = -pu->instance;
			goto p_err;
		}

		pu->instance = block->start + channel_id;
	}

	set_bit(channel_id, block->preempted);

p_err:
	return ret;
}

static int __vpu_resource_pu_put(struct vpu_hw_pu *pu_device, struct vpul_pu *pu)
{
	int ret = 0;
	u32 block_id, channel_id;
	struct vpu_hw_block *block;

	BUG_ON(!pu_device);
	BUG_ON(!pu);

	if (pu->instance >= VPU_PU_NUMBER) {
		vpu_err("pu id is invalid(%d)\n", pu->instance);
		ret = -EINVAL;
		goto p_err;
	}

	block_id = pu_device->translator[pu->instance];
	block = &pu_device->table[block_id];

	channel_id = pu->instance - block->start;
	if (channel_id >= block->total) {
		vpu_err("channel_id %d is invalid(%d, %d)\n", channel_id, pu->instance, block->start);
		ret = -pu->instance;
		goto p_err;
	}

	pu->instance = block->start;
	clear_bit(channel_id, block->allocated);

p_err:
	return ret;
}

int __vpu_resource_get(struct vpu_hardware *hardware, struct vpul_task *task, unsigned long flags)
{
	int ret = 0;
	u32 i, j, chain_cnt, pu_cnt;
	struct vpul_subchain *chain;
	struct vpul_pu *pu;
	struct vpu_hw_pu *pu_device;

	pu_device = &hardware->pu;
	chain_cnt = task->t_num_of_subchains;
	chain = (struct vpul_subchain *)((char *)task + task->sc_vec_ofs);

	/* prepare */
	for (i = 0; i < pu_device->total; ++i)
		bitmap_copy(pu_device->table[i].preempted, pu_device->table[i].allocated, pu_device->table[i].total);

	/* estimation */
	for (i = 0; i < chain_cnt; ++i) {
		pu_cnt = chain[i].num_of_pus;
		pu = (struct vpul_pu *)((char *)task + chain[i].pus_ofs);
		for (j = 0; j < pu_cnt; ++j) {
			ret = __vpu_resource_pu_get(pu_device, &pu[j], flags);
			if (ret)
				goto p_err;
		}
	}

	/* acquire */
	for (i = 0; i < pu_device->total; ++i)
		bitmap_copy(pu_device->table[i].allocated, pu_device->table[i].preempted, pu_device->table[i].total);

p_err:
	return ret;
}

int __vpu_resource_put(struct vpu_hardware *hardware, struct vpul_task *task)
{
	int ret = 0;
	u32 i, j, chain_cnt, pu_cnt;
	struct vpul_subchain *chain;
	struct vpul_pu *pu;
	struct vpu_hw_pu *pu_device;

	pu_device = &hardware->pu;
	chain_cnt = task->t_num_of_subchains;
	chain = (struct vpul_subchain *)((char *)task + task->sc_vec_ofs);

	for (i = 0; i < chain_cnt; ++i) {
		pu_cnt = chain[i].num_of_pus;
		pu = (struct vpul_pu *)((char *)task + chain[i].pus_ofs);
		for (j = 0; j < pu_cnt; ++j) {
			ret = __vpu_resource_pu_put(pu_device, &pu[j]);
			if (ret) {
				vpu_err("__vpu_resource_pu_put is fail(%d)\n", ret);
				goto p_err;
			}
		}
	}

p_err:
	return ret;
}