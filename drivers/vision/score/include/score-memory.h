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

#ifndef SCORE_MEMORY_H_
#define SCORE_MEMORY_H_

#include <linux/platform_device.h>
#include <media/videobuf2-core.h>
#if defined(CONFIG_VIDEOBUF2_CMA_PHYS)
#include <media/videobuf2-cma-phys.h>
#elif defined(CONFIG_VIDEOBUF2_ION)
#include <media/videobuf2-ion.h>
#endif

#include "score-config.h"

#define SCORE_MEMORY_INTERNAL_SIZE		0x00800000 /* 8 MB */

#define SCORE_MEMORY_FW_MSG_SIZE		0x00100000 /*  1 MB */

#define SCORE_ORGMEM_SIZE			0x00100000 /*  1 MB */

#ifdef ENABLE_MEM_OFFSET
#define SCORE_ORGMEM_OFFSET			0x00300000 /* 3 MB */
#define SCORE_MALLOC_OFFSET			0x00400000 /* 4 MB */
#define SCORE_PRINT_OFFSET			0x00600000 /* 6 MB */
#define SCORE_PROFILE_OFFSET			0x00700000 /* 7.0 MB */
#define SCORE_DUMP_OFFSET			0x00780000 /* 7.5 MB */
#define SCORE_DUMP_END				0x00800000 /* 8 MB */

#define SCORE_MALLOC_OFFSET_SHIFT		6  /* left shift */
#define SCORE_PRINT_OFFSET_SHIFT		0
#define SCORE_PROFILE_OFFSET_SHIFT		6  /* right shift */
#define SCORE_DUMP_OFFSET_SHIFT			12 /* right shift */
#define SCORE_DUMP_END_SHIFT			18 /* right shift */
#endif

#define SCORE_PM				1
#define SCORE_DM				2

enum score_memory_sync_type {
	SCORE_MEMORY_SYNC_FOR_CPU			= 0,
	SCORE_MEMORY_SYNC_FOR_DEVICE			= 1,
	SCORE_MEMORY_SYNC_FOR_BYDIRECTIONAL		= 2,
	SCORE_MEMORY_SYNC_FOR_NONE
};

struct score_memory_buffer {
	u32				fid;
	struct list_head		list;
	union {
		unsigned long		userptr;
		int			fd;
	} m;
	unsigned int			memory_type;

	dma_addr_t			dvaddr;
	void				*kvaddr;
	void				*mem_priv;
	void				*cookie;
	void				*dbuf;
	size_t				size;
};

struct score_memory_info {
	void				*cookie;
	void				*kvaddr;
	dma_addr_t			dvaddr;
};

struct score_mem_ops {
	void (*cleanup)(void *alloc_ctx);
	int (*resume)(void *alloc_ctx);
	void (*suspend)(void *alloc_ctx);
	void (*set_cacheable)(void *alloc_ctx, bool cacheable);
};

struct score_memory {
	struct device			*dev;
	struct vb2_alloc_ctx		*alloc_ctx;
	const struct score_mem_ops	*score_mem_ops;
	const struct vb2_mem_ops	*vb2_mem_ops;

	struct score_memory_info	info;
};

int score_memory_probe(struct score_memory *mem, struct device *dev);
void score_memory_release(struct score_memory *mem);
int score_memory_open(struct score_memory *mem);
void score_memory_close(struct score_memory *mem);

int score_memory_map_dmabuf(struct score_memory *mem,
		struct score_memory_buffer *buf);
int score_memory_unmap_dmabuf(struct score_memory *mem,
		struct score_memory_buffer *buf);
int score_memory_compare_dmabuf(struct score_memory_buffer *buf,
		struct score_memory_buffer *listed_buffer);
int score_memory_map_userptr(struct score_memory *mem,
		struct score_memory_buffer *buf);
int score_memory_unmap_userptr(struct score_memory *mem,
		struct score_memory_buffer *buf);
void score_memory_invalid_or_flush_userptr(struct score_memory_buffer *buf,
		int sync_for, enum dma_data_direction dir);

#define CALL_MEMOPS(g, op, ...) (((g)->score_mem_ops->op) ? \
		((g)->score_mem_ops->op((g)->alloc_ctx, ##__VA_ARGS__)) : 0)
#endif
