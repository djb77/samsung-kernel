/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/delay.h>
#include <linux/random.h>
#include <linux/slab.h>

#include "vpu-config.h"
#include "vpu-slab.h"

int vpu_slab_init(struct vpu_slab_allocator *allocator)
{
	int ret = 0, i;
	char name[100];
	size_t size;

	allocator->cache_size[0] = VPU_SLAB_LEVEL0_SIZE;
	allocator->cache_size[1] = VPU_SLAB_LEVEL1_SIZE;
	allocator->cache_size[2] = VPU_SLAB_LEVEL2_SIZE;

	for (i = 0; i < VPU_SLAB_MAX_LEVEL; ++i) {
		size = allocator->cache_size[i];
		snprintf(name, sizeof(name), "vpu=size-%zd", size);

		allocator->cache[i] = kmem_cache_create(name, size,
			ARCH_KMALLOC_MINALIGN, SLAB_POISON | SLAB_PANIC, NULL);
		if (!allocator->cache[i]) {
			probe_err("kmem_cache_create(%zd) is fail\n", size);
			ret = -ENOMEM;
			goto p_err;
		}
	}

p_err:
	return ret;
}

int vpu_slab_alloc(struct vpu_slab_allocator *allocator, void **target, size_t size)
{
	int ret = 0, i;

	for (i = 0; i < VPU_SLAB_MAX_LEVEL; ++i) {
		if (size <= allocator->cache_size[i])
			break;
	}

	if (i >= VPU_SLAB_MAX_LEVEL) {
		vpu_err("alloc size is invalid(%zd)\n", size);
		ret= -EINVAL;
		goto p_err;
	}

	*target = kmem_cache_alloc(allocator->cache[i], GFP_KERNEL);
	if (!(*target)) {
		vpu_err("kmem_cache_alloc is fail\n");
		ret = -ENOMEM;
		goto p_err;
	}

p_err:
	return ret;
}

int vpu_slab_free(struct vpu_slab_allocator *allocator, void *target, size_t size)
{
	int ret = 0, i;

	for (i = 0; i < VPU_SLAB_MAX_LEVEL; ++i) {
		if (size <= allocator->cache_size[i])
			break;
	}

	if (i >= VPU_SLAB_MAX_LEVEL) {
		vpu_err("alloc size is invalid(%zd)\n", size);
		BUG();
	}

	kmem_cache_free(allocator->cache[i], target);

	return ret;
}
