/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 * exynos5 fimc-is core functions
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <video/videonode.h>
#include <media/exynos_mc.h>
#include <asm/cacheflush.h>
#include <asm/pgtable.h>
#include <linux/firmware.h>
#include <linux/dma-mapping.h>
#include <linux/scatterlist.h>
#include <linux/videodev2.h>
#include <linux/videodev2_exynos_camera.h>
#include <linux/vmalloc.h>
#include <linux/interrupt.h>
#include <linux/of.h>

#include "fimc-is-core.h"
#include "fimc-is-cmd.h"
#include "fimc-is-regs.h"
#include "fimc-is-err.h"

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 9, 0))
#include <linux/exynos_iovmm.h>
#else
#include <plat/iovmm.h>
#endif

#if defined(CONFIG_VIDEOBUF2_ION)
/* fimc-is vb2 buffer operations */
static inline ulong fimc_is_vb2_ion_plane_kvaddr(
		struct fimc_is_vb2_buf *vbuf, u32 plane)

{
	return (ulong)vb2_plane_vaddr(&vbuf->vb.vb2_buf, plane);
}

static inline ulong fimc_is_vb2_ion_plane_cookie(
		struct fimc_is_vb2_buf *vbuf, u32 plane)
{
	return (ulong)vb2_plane_cookie(&vbuf->vb.vb2_buf, plane);
}

static dma_addr_t fimc_is_vb2_ion_plane_dvaddr(
		struct fimc_is_vb2_buf *vbuf, u32 plane)

{
	dma_addr_t dva = 0;

	WARN_ON(vb2_ion_dma_address(vb2_plane_cookie(&vbuf->vb.vb2_buf, plane), &dva) != 0);

	return (ulong)dva;
}

static void fimc_is_vb2_ion_plane_prepare(struct fimc_is_vb2_buf *vbuf,
		u32 plane, bool exact)
{
	struct vb2_buffer *vb = &vbuf->vb.vb2_buf;
	enum dma_data_direction dir;
	unsigned long size;
	u32 spare;

	/* skip meta plane */
	spare = vb->num_planes - 1;
	if (plane == spare)
		return;

	dir = V4L2_TYPE_IS_OUTPUT(vb->type) ?
		DMA_TO_DEVICE : DMA_FROM_DEVICE;

	size = exact ?
		vb2_get_plane_payload(vb, plane) : vb2_plane_size(vb, plane);

	vb2_ion_sync_for_device((void *)vb2_plane_cookie(vb, plane), 0,
			size, dir);
}

static void fimc_is_vb2_ion_plane_finish(struct fimc_is_vb2_buf *vbuf,
		u32 plane, bool exact)
{
	struct vb2_buffer *vb = &vbuf->vb.vb2_buf;
	enum dma_data_direction dir;
	unsigned long size;
	u32 spare;

	/* skip meta plane */
	spare = vb->num_planes - 1;
	if (plane == spare)
		return;

	dir = V4L2_TYPE_IS_OUTPUT(vb->type) ?
		DMA_TO_DEVICE : DMA_FROM_DEVICE;

	size = exact ?
		vb2_get_plane_payload(vb, plane) : vb2_plane_size(vb, plane);

	vb2_ion_sync_for_cpu((void *)vb2_plane_cookie(vb, plane), 0,
			size, dir);
}

static void fimc_is_vb2_ion_buf_prepare(struct fimc_is_vb2_buf *vbuf, bool exact)
{
	struct vb2_buffer *vb = &vbuf->vb.vb2_buf;
	enum dma_data_direction dir;
	unsigned long size;
	u32 plane;
	u32 spare;

	dir = V4L2_TYPE_IS_OUTPUT(vb->type) ?
		DMA_TO_DEVICE : DMA_FROM_DEVICE;

	/* skip meta plane */
	spare = vb->num_planes - 1;

	for (plane = 0; plane < spare; plane++) {
		size = exact ?
			vb2_get_plane_payload(vb, plane) : vb2_plane_size(vb, plane);

		vb2_ion_sync_for_device((void *)vb2_plane_cookie(vb, plane), 0,
				size, dir);
	}
}

static void fimc_is_vb2_ion_buf_finish(struct fimc_is_vb2_buf *vbuf, bool exact)
{
	struct vb2_buffer *vb = &vbuf->vb.vb2_buf;
	enum dma_data_direction dir;
	unsigned long size;
	u32 plane;
	u32 spare;

	dir = V4L2_TYPE_IS_OUTPUT(vb->type) ?
		DMA_TO_DEVICE : DMA_FROM_DEVICE;

	/* skip meta plane */
	spare = vb->num_planes - 1;

	for (plane = 0; plane < spare; plane++) {
		size = exact ?
			vb2_get_plane_payload(vb, plane) : vb2_plane_size(vb, plane);

		vb2_ion_sync_for_cpu((void *)vb2_plane_cookie(vb, plane), 0,
				size, dir);
	}
}

const struct fimc_is_vb2_buf_ops fimc_is_vb2_buf_ops_ion = {
	.plane_kvaddr	= fimc_is_vb2_ion_plane_kvaddr,
	.plane_cookie	= fimc_is_vb2_ion_plane_cookie,
	.plane_dvaddr	= fimc_is_vb2_ion_plane_dvaddr,
	.plane_prepare	= fimc_is_vb2_ion_plane_prepare,
	.plane_finish	= fimc_is_vb2_ion_plane_finish,
	.buf_prepare	= fimc_is_vb2_ion_buf_prepare,
	.buf_finish	= fimc_is_vb2_ion_buf_finish,
};

/* fimc-is private buffer operations */
static void fimc_is_vb2_ion_free(struct fimc_is_priv_buf *pbuf)
{
	vb2_ion_private_free(pbuf->cookie);
	kfree(pbuf);
}

static ulong fimc_is_vb2_ion_kvaddr(struct fimc_is_priv_buf *pbuf)
{
	void *kva;

	if (!pbuf)
		return 0;

	kva = vb2_ion_private_vaddr(pbuf->cookie);
	if (IS_ERR_OR_NULL(kva)) {
		err("could not get kernel addr for @%p: %ld",
			pbuf->cookie, PTR_ERR(kva));
		return 0;
	}

	return (ulong)kva;
}

static dma_addr_t fimc_is_vb2_ion_dvaddr(struct fimc_is_priv_buf *pbuf)
{
	dma_addr_t dva = 0;

	if (!pbuf)
		return 0;

	WARN_ON(vb2_ion_dma_address((void *)pbuf->cookie, &dva) != 0);

	return (ulong)dva;
}

static phys_addr_t fimc_is_vb2_ion_phaddr(struct fimc_is_priv_buf *pbuf)
{
	/* PHCONTIG option is not supported in ION */
	return 0;
}

static void fimc_is_vb2_ion_sync_for_device(struct fimc_is_priv_buf *pbuf,
		off_t offset, size_t size, enum dma_data_direction dir)
{
	vb2_ion_sync_for_device(pbuf->cookie, offset, size, dir);
}

static void fimc_is_vb2_ion_sync_for_cpu(struct fimc_is_priv_buf *pbuf,
		off_t offset, size_t size, enum dma_data_direction dir)
{
	vb2_ion_sync_for_cpu(pbuf->cookie, offset, size, dir);
}

const struct fimc_is_priv_buf_ops fimc_is_priv_buf_ops_ion = {
	.free			= fimc_is_vb2_ion_free,
	.kvaddr			= fimc_is_vb2_ion_kvaddr,
	.dvaddr			= fimc_is_vb2_ion_dvaddr,
	.phaddr			= fimc_is_vb2_ion_phaddr,
	.sync_for_device	= fimc_is_vb2_ion_sync_for_device,
	.sync_for_cpu		= fimc_is_vb2_ion_sync_for_cpu,
};

/* fimc-is memory operations */
static void *fimc_is_vb2_ion_init(struct platform_device *pdev,
		long flag)
{
	return vb2_ion_create_context(&pdev->dev, SZ_4K,
			VB2ION_CTX_IOMMU |
			flag);
}

static struct fimc_is_priv_buf *fimc_is_vb2_ion_alloc(void *ctx,
		size_t size, size_t align)
{
	struct fimc_is_priv_buf *buf;
	int ret = 0;

	buf = kzalloc(sizeof(*buf), GFP_KERNEL);
	if (!buf)
		return ERR_PTR(-ENOMEM);

	buf->cookie = vb2_ion_private_alloc(ctx, size);
	if (IS_ERR(buf->cookie)) {
		err("failed to allocate a private buffer, size: %zd", size);
		ret = PTR_ERR(buf->cookie);
		goto err_priv_alloc;
	}

	buf->size = size;
	buf->align = align;
	buf->ctx = ctx;
	buf->ops = &fimc_is_priv_buf_ops_ion;

	return buf;

err_priv_alloc:
	kfree(buf);

	return ERR_PTR(ret);
}

static int fimc_is_vb2_ion_resume(void *ctx)
{
	if (ctx)
		return vb2_ion_attach_iommu(ctx);

	return -ENOENT;
}

static void fimc_is_vb2_ion_suspend(void *ctx)
{
	if (ctx)
		vb2_ion_detach_iommu(ctx);
}

const struct fimc_is_mem_ops fimc_is_mem_ops_ion = {
	.init			= fimc_is_vb2_ion_init,
	.cleanup		= vb2_ion_destroy_context,
	.resume			= fimc_is_vb2_ion_resume,
	.suspend		= fimc_is_vb2_ion_suspend,
	.set_cached		= vb2_ion_set_cached,
	.set_alignment		= vb2_ion_set_alignment,
	.alloc			= fimc_is_vb2_ion_alloc,
};
#endif


/* fimc-is private buffer operations */
static void fimc_is_vb2_km_free(struct fimc_is_priv_buf *pbuf)
{
	kfree(pbuf->kvaddr);
	kfree(pbuf);
}

static ulong fimc_is_vb2_km_kvaddr(struct fimc_is_priv_buf *pbuf)
{
	if (!pbuf)
		return 0;

	return (ulong)pbuf->kvaddr;
}

static phys_addr_t fimc_is_vb2_km_phaddr(struct fimc_is_priv_buf *pbuf)
{
	phys_addr_t pa = 0;

	if (!pbuf)
		return 0;

	pa = virt_to_phys(pbuf->kvaddr);

	return pa;
}
const struct fimc_is_priv_buf_ops fimc_is_priv_buf_ops_km = {
	.free			= fimc_is_vb2_km_free,
	.kvaddr			= fimc_is_vb2_km_kvaddr,
	.phaddr			= fimc_is_vb2_km_phaddr,
};

static struct fimc_is_priv_buf *fimc_is_kmalloc(size_t size, size_t align)
{
	struct fimc_is_priv_buf *buf = NULL;
	int ret = 0;

	buf = kzalloc(sizeof(*buf), GFP_KERNEL);
	if (!buf)
		return ERR_PTR(-ENOMEM);

	buf->kvaddr = kzalloc(DEBUG_REGION_SIZE + 0x10, GFP_KERNEL);
	if (!buf->kvaddr) {
		ret = -ENOMEM;
		goto err_priv_alloc;
	}

	buf->size = size;
	buf->align = align;
	buf->ops = &fimc_is_priv_buf_ops_km;

	return buf;

err_priv_alloc:
	kfree(buf);

	return ERR_PTR(ret);
}

int fimc_is_mem_init(struct fimc_is_mem *mem, struct platform_device *pdev)
{
#if defined(CONFIG_VIDEOBUF2_ION)
	mem->fimc_is_mem_ops = &fimc_is_mem_ops_ion;
	mem->vb2_mem_ops = &vb2_ion_memops;
	mem->fimc_is_vb2_buf_ops = &fimc_is_vb2_buf_ops_ion;
	mem->kmalloc = &fimc_is_kmalloc;
#endif

	mem->default_ctx = CALL_PTR_MEMOP(mem, init, pdev, VB2ION_CTX_VMCONTIG);
	if (IS_ERR_OR_NULL(mem->default_ctx)) {
		if (IS_ERR(mem->default_ctx))
			return PTR_ERR(mem->default_ctx);
		else
			return -EINVAL;
	}

	return 0;
}
