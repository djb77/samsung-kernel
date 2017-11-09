/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#ifndef VPU_SLAB_H_
#define VPU_SLAB_H_

#define VPU_SLAB_MAX_LEVEL		3
#define VPU_SLAB_LEVEL0_SIZE		64
#define VPU_SLAB_LEVEL1_SIZE		512
#define VPU_SLAB_LEVEL2_SIZE		32768

struct vpu_slab_allocator {
	u32				cache_size[VPU_SLAB_MAX_LEVEL];
	struct kmem_cache		*cache[VPU_SLAB_MAX_LEVEL];
};

int vpu_slab_init(struct vpu_slab_allocator *allocator);
int vpu_slab_alloc(struct vpu_slab_allocator *allocator, void **target, size_t size);
int vpu_slab_free(struct vpu_slab_allocator *allocator, void *target, size_t size);

#endif