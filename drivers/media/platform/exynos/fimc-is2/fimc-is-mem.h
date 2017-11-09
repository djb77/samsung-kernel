/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_MEM_H
#define FIMC_IS_MEM_H

#include <linux/platform_device.h>
#include <media/videobuf2-v4l2.h>
#if defined(CONFIG_VIDEOBUF2_CMA_PHYS)
#include <media/videobuf2-cma-phys.h>
#elif defined(CONFIG_VIDEOBUF2_ION)
#include <media/videobuf2-ion.h>
#endif

struct fimc_is_vb2_buf;
struct fimc_is_vb2_buf_ops {
	ulong (*plane_kvaddr)(struct fimc_is_vb2_buf *vbuf, u32 plane);
	ulong (*plane_cookie)(struct fimc_is_vb2_buf *vbuf, u32 plane);
	dma_addr_t (*plane_dvaddr)(struct fimc_is_vb2_buf *vbuf, u32 plane);
	void (*plane_prepare)(struct fimc_is_vb2_buf *vbuf, u32 plane,
						bool exact);
	void (*plane_finish)(struct fimc_is_vb2_buf *vbuf, u32 plane,
						bool exact);
	void (*buf_prepare)(struct fimc_is_vb2_buf *vbuf, bool exact);
	void (*buf_finish)(struct fimc_is_vb2_buf *vbuf, bool exact);
};

struct fimc_is_vb2_buf {
	struct vb2_v4l2_buffer		vb;
	ulong				kva[VIDEO_MAX_PLANES];
	dma_addr_t			dva[VIDEO_MAX_PLANES];

	const struct fimc_is_vb2_buf_ops *ops;
};

struct fimc_is_priv_buf;
struct fimc_is_priv_buf_ops {
	void (*free)(struct fimc_is_priv_buf *pbuf);
	ulong (*kvaddr)(struct fimc_is_priv_buf *pbuf);
	dma_addr_t (*dvaddr)(struct fimc_is_priv_buf *pbuf);
	phys_addr_t (*phaddr)(struct fimc_is_priv_buf *pbuf);
	void (*sync_for_device)(struct fimc_is_priv_buf *pbuf,
							off_t offset, size_t size,
							enum dma_data_direction dir);
	void (*sync_for_cpu)(struct fimc_is_priv_buf *pbuf,
							off_t offset, size_t size,
							enum dma_data_direction dir);
};

struct fimc_is_priv_buf {
	void	*cookie;
	size_t	size;
	size_t	align;
	void	*ctx;
	void	*kvaddr;

	const struct fimc_is_priv_buf_ops *ops;
	void	*priv;
};

#define vb_to_vb2_v4l2_buffer(x)				\
	container_of(x, struct vb2_v4l2_buffer, vb2_buf)

#define vb_to_fimc_is_vb2_buf(x)				\
	container_of(x, struct fimc_is_vb2_buf, vb)

#define CALL_BUFOP(buf, op, args...)			\
	((buf)->ops->op ? (buf)->ops->op(args) : 0)

#define CALL_PTR_BUFOP(buf, op, args...)		\
	((buf)->ops->op ? (buf)->ops->op(args) : NULL)

#define CALL_VOID_BUFOP(buf, op, args...)	\
	do {									\
		if ((buf)->ops->op)					\
			(buf)->ops->op(args);			\
	} while (0)

#define call_buf_op(buf, op, args...)			\
	((buf)->ops->op ? (buf)->ops->op((buf), args) : 0)

struct fimc_is_mem_ops {
	void *(*init)(struct platform_device *pdev, long flag);
	void (*cleanup)(void *ctx);
	int (*resume)(void *ctx);
	void (*suspend)(void *ctx);
	void (*set_cached)(void *ctx, bool cacheable);
	int (*set_alignment)(void *ctx, size_t alignment);
	struct fimc_is_priv_buf *(*alloc)(void *ctx, size_t size, size_t align);
};

struct fimc_is_mem {
	struct vb2_alloc_ctx				*default_ctx;
	struct vb2_alloc_ctx				*phcontig_ctx;
	const struct fimc_is_mem_ops			*fimc_is_mem_ops;
	const struct vb2_mem_ops			*vb2_mem_ops;
	const struct fimc_is_vb2_buf_ops		*fimc_is_vb2_buf_ops;
	void						*priv;
	struct fimc_is_priv_buf *(*kmalloc)(size_t size, size_t align);
};

#define CALL_MEMOP(mem, op, args...)			\
	((mem)->fimc_is_mem_ops->op ?				\
		(mem)->fimc_is_mem_ops->op(args) : 0)

#define CALL_PTR_MEMOP(mem, op, args...)		\
	((mem)->fimc_is_mem_ops->op ?				\
		(mem)->fimc_is_mem_ops->op(args) : NULL)

#define CALL_VOID_MEMOP(mem, op, args...)		\
	do {										\
		if ((mem)->fimc_is_mem_ops->op)			\
			(mem)->fimc_is_mem_ops->op(args);	\
	} while (0)

struct fimc_is_minfo {
	struct fimc_is_priv_buf *pb_fw;
	struct fimc_is_priv_buf *pb_setfile;
	struct fimc_is_priv_buf *pb_rear_cal;
	struct fimc_is_priv_buf *pb_front_cal;
	struct fimc_is_priv_buf *pb_debug;
	struct fimc_is_priv_buf *pb_fshared;
	struct fimc_is_priv_buf *pb_dregion;
	struct fimc_is_priv_buf *pb_pregion;
	struct fimc_is_priv_buf *pb_lib;
	struct fimc_is_priv_buf *pb_taaisp;
	struct fimc_is_priv_buf *pb_lhfd;
	struct fimc_is_priv_buf *pb_vra;
	struct fimc_is_priv_buf *pb_tpu;

	size_t		total_size;
	ulong		kvaddr_debug_cnt;

	dma_addr_t	dvaddr;
	ulong		kvaddr;
	dma_addr_t	dvaddr_lib;
	ulong		kvaddr_lib;
	phys_addr_t	phaddr_debug;
	dma_addr_t	dvaddr_debug;
	ulong		kvaddr_debug;
	dma_addr_t	dvaddr_fshared;
	ulong		kvaddr_fshared;
	dma_addr_t	dvaddr_region;
	ulong		kvaddr_region;

	dma_addr_t	dvaddr_taaisp;
	ulong		kvaddr_taaisp;
	dma_addr_t	dvaddr_tpu;
	ulong		kvaddr_tpu;
	dma_addr_t	dvaddr_lhfd;	/* FD map buffer region */
	ulong		kvaddr_lhfd;	/* NUM_FD_INTERNAL_BUF = 3 */
	dma_addr_t	dvaddr_vra;
	ulong		kvaddr_vra;

	ulong		kvaddr_setfile;
	ulong		kvaddr_rear_cal;
	ulong		kvaddr_front_cal;
};

int fimc_is_mem_init(struct fimc_is_mem *mem, struct platform_device *pdev);
#endif
