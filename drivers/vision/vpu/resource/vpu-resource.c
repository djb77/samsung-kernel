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
#include "vpu-debug.h"
#include "vpu-resource.h"
#include "vpu-hardware.h"
#include "lib/vpul-errno.h"
#include "lib/vpul-hwmapper.h"

static int __vpu_resource_pu_add(struct vpu_hw_pu *pu_device, struct vpul_pu *pu)
{
	int ret = 0;
	u32 block_id;
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
	block->refer++;

p_err:
	return ret;
}

static int __vpu_resource_pu_del(struct vpu_hw_pu *pu_device, struct vpul_pu *pu)
{
	int ret = 0;
	u32 block_id;
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
	block->refer--;

p_err:
	return ret;
}

static int __vpu_resource_add(struct vpu_resource *resource, struct vpul_task *task)
{
	int ret = 0;
	u32 i, j, chain_cnt, pu_cnt;
	struct vpul_subchain *chain;
	struct vpul_pu *pu;
	struct vpu_hw_pu *pu_device;

	pu_device = &resource->hardware.pu;
	chain_cnt = task->t_num_of_subchains;
	chain = (struct vpul_subchain *)((char *)task + task->sc_vec_ofs);

	for (i = 0; i < chain_cnt; ++i) {
		if (chain[i].stype != VPUL_SUB_CH_HW)
			continue;

		pu_cnt = chain[i].num_of_pus;
		pu = (struct vpul_pu *)((char *)task + chain[i].pus_ofs);
		for (j = 0; j < pu_cnt; ++j) {
			ret = __vpu_resource_pu_add(pu_device, &pu[j]);
			if (ret) {
				vpu_err("__vpu_resource_pu_add is fail(%d)\n", ret);
				goto p_err;
			}
		}
	}

p_err:
	return ret;
}

static int __vpu_resource_del(struct vpu_resource *resource, struct vpul_task *task)
{
	int ret = 0;
	u32 i, j, chain_cnt, pu_cnt;
	struct vpul_subchain *chain;
	struct vpul_pu *pu;
	struct vpu_hw_pu *pu_device;

	pu_device = &resource->hardware.pu;
	chain_cnt = task->t_num_of_subchains;
	chain = (struct vpul_subchain *)((char *)task + task->sc_vec_ofs);

	for (i = 0; i < chain_cnt; ++i) {
		if (chain[i].stype != VPUL_SUB_CH_HW)
			continue;

		pu_cnt = chain[i].num_of_pus;
		pu = (struct vpul_pu *)((char *)task + chain[i].pus_ofs);
		for (j = 0; j < pu_cnt; ++j) {
			ret = __vpu_resource_pu_del(pu_device, &pu[j]);
			if (ret) {
				vpu_err("__vpu_resource_pu_del is fail(%d)\n", ret);
				goto p_err;
			}
		}
	}

p_err:
	return ret;
}

int vpu_resource_print(struct vpu_resource *resource)
{
	int i;
	char buf[VPU_HW_MAX_CH_COUNT];
	struct vpu_hw_pu *pu;
	struct vpu_hw_mprb *mprb;

	pu = &resource->hardware.pu;
	mprb = &resource->hardware.mprb;

	for (i = 0; i < pu->total; ++i) {
		bitmap_scnprintf(buf, sizeof(buf), pu->table[i].allocated, VPU_HW_MAX_CH_COUNT);
		vpu_info("PU%02d : %s\n", i, buf);
	}

	for (i = 0; i < mprb->total; ++i) {
		bitmap_scnprintf(buf, sizeof(buf), mprb->table[i].allocated, VPU_HW_MAX_CH_COUNT);
		vpu_info("MPRB%02d : %s\n", i, buf);
	}

	return 0;
}

int vpu_resource_probe(struct vpu_resource *resource, struct device *dev)
{
	int ret = 0;
	struct vpu_hardware *hardware;
	struct vpu_hw_pu *pu_device;
	struct vpu_hw_mprb *mprb_device;
	struct vpu_hw_block *block;
	u32 i;

	BUG_ON(!resource);

	hardware = &resource->hardware;
	pu_device = &hardware->pu;
	mprb_device = &hardware->mprb;
	spin_lock_init(&resource->slock);

	ret = vpu_hardware_init(hardware);
	if (ret != VPU_STATUS_SUCCESS) {
		probe_err("vpu_hardware_init is fail(%d)\n", ret);
		goto p_err;
	}

	ret = vpu_hardware_host_init(hardware);
	if (ret) {
		probe_err("vpu_hardware_host_init is fail(%d)\n", ret);
		goto p_err;
	}

	for (i = 1; i < pu_device->total; ++i) {
		block = &pu_device->table[i];

		if (!block->attr_group.attrs)
			continue;

		ret = sysfs_create_group(&dev->kobj, &block->attr_group);
		if (ret) {
			probe_err("sysfs_create_group1(%d.%s) is fail(%d)\n", i, block->name, ret);
			goto p_err;
		}
	}

	for (i = 1; i < mprb_device->total; ++i) {
		block = &mprb_device->table[i];

		if (!block->attr_group.attrs)
			continue;

		ret = sysfs_create_group(&dev->kobj, &block->attr_group);
		if (ret) {
			probe_err("sysfs_create_group2(%d.%s) is fail(%d)\n", i, block->name, ret);
			goto p_err;
		}
	}

p_err:
	probe_info("%s():%d\n", __func__, ret);
	return ret;
}

int vpu_resource_open(struct vpu_resource *resource)
{
	int ret = 0;

	BUG_ON(!resource);

	ret = vpu_hardware_init(&resource->hardware);
	if (ret != VPU_STATUS_SUCCESS) {
		vpu_err("vpu_hardware_init is fail(%d)\n", ret);
		goto p_err;
	}

	ret = 0;

p_err:
	return ret;
}

int vpu_resource_close(struct vpu_resource *resource)
{
	BUG_ON(!resource);

	return 0;
}

int vpu_resource_add(struct vpu_resource *resource, struct vpul_task *task)
{
	int ret = 0;
	unsigned long flags;

	BUG_ON(!resource);
	BUG_ON(!task);

	spin_lock_irqsave(&resource->slock, flags);
	ret = __vpu_resource_add(resource, task);
	spin_unlock_irqrestore(&resource->slock, flags);

	return ret;
}

int vpu_resource_del(struct vpu_resource *resource, struct vpul_task *task)
{
	int ret = 0;
	unsigned long flags;

	BUG_ON(!resource);
	BUG_ON(!task);

	spin_lock_irqsave(&resource->slock, flags);
	ret = __vpu_resource_del(resource, task);
	spin_unlock_irqrestore(&resource->slock, flags);

	return ret;
}

int vpu_resource_get(struct vpu_resource *resource, struct vpul_task *task, unsigned long flags)
{
	int ret = 0;
	unsigned long sflags;

	BUG_ON(!resource);
	BUG_ON(!task);

	spin_lock_irqsave(&resource->slock, sflags);
	ret = __vpu_resource_get(&resource->hardware, task, flags);
	spin_unlock_irqrestore(&resource->slock, sflags);

#ifdef DBG_RESOURCE
	vpu_info("%s:\n", __func__);
	vpu_resource_print(resource);
#endif

	if (ret == VPU_STATUS_SUCCESS)
		return 0;
	else if (ret == VPU_STATUS_FAILURE)
		return 1;
	else
		return -ret;
}

int vpu_resource_put(struct vpu_resource *resource, struct vpul_task *task)
{
	int ret = 0;
	unsigned long sflags;

	BUG_ON(!resource);
	BUG_ON(!task);

	spin_lock_irqsave(&resource->slock, sflags);
	ret = __vpu_resource_put(&resource->hardware, task);
	spin_unlock_irqrestore(&resource->slock, sflags);

#ifdef DBG_RESOURCE
	vpu_info("%s:\n", __func__);
	vpu_resource_print(resource);
#endif

	if (ret == VPU_STATUS_SUCCESS)
		return 0;
	else if (ret == VPU_STATUS_FAILURE)
		return 1;
	else
		return -ret;
}