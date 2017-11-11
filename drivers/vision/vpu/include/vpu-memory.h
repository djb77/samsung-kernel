/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef VPU_MEMORY_H_
#define VPU_MEMORY_H_

#include <linux/platform_device.h>
#include <media/videobuf2-core.h>
#if defined(CONFIG_VIDEOBUF2_CMA_PHYS)
#include <media/videobuf2-cma-phys.h>
#elif defined(CONFIG_VIDEOBUF2_ION)
#include <media/videobuf2-ion.h>
#endif

#define VPU_MEMORY_INTERNAL_SIZE	0

struct vpu_memory_buffer {
	struct list_head		list;
	int				fd;
	dma_addr_t			dvaddr;
	void				*kvaddr;
	void				*mem_priv;
	void				*cookie;
	void				*dbuf;
	size_t				size;
};

struct vpu_memory_info {
	void		*cookie;
	void		*kvaddr;
	dma_addr_t	dvaddr;

	u32		dvaddr_debug;
	ulong		kvaddr_debug;
};

struct vpu_mem_ops {
	void (*cleanup)(void *alloc_ctx);
	int (*resume)(void *alloc_ctx);
	void (*suspend)(void *alloc_ctx);
	void (*set_cacheable)(void *alloc_ctx, bool cacheable);
};

struct vpu_memory {
	struct device			*dev;
	struct vb2_alloc_ctx		*alloc_ctx;
	const struct vpu_mem_ops	*vpu_mem_ops;
	const struct vb2_mem_ops	*vb2_mem_ops;

	spinlock_t			im_lock;
	struct list_head		im_list;
	u32				im_count;
	struct vpu_memory_info		info;
};

int vpu_memory_probe(struct vpu_memory *memory, struct device *dev);
int vpu_memory_open(struct vpu_memory *memory);
int vpu_memory_close(struct vpu_memory *memory);
int vpu_memory_map(struct vpu_memory *memory, struct vpu_memory_buffer *buffer);
int vpu_memory_unmap(struct vpu_memory *memory, struct vpu_memory_buffer *buffer);

#endif