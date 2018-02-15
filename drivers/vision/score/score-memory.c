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

#include <linux/version.h>
#include <linux/device.h>
#include <media/videobuf2-ion.h>

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 9, 0))
#include <linux/exynos_iovmm.h>
#else
#include <plat/iovmm.h>
#endif

#include "score-config.h"
#include "score-binary.h"
#include "score-memory.h"
#include "score-debug.h"

static int __score_memory_alloc(struct score_memory *mem)
{
	int ret = 0;
#if defined(CONFIG_VIDEOBUF2_ION)
	void *cookie, *kvaddr;
	dma_addr_t dvaddr;
#endif

#if defined(CONFIG_VIDEOBUF2_ION)
	cookie = NULL;
	kvaddr = NULL;
	dvaddr = 0;

	cookie = vb2_ion_private_alloc(mem->alloc_ctx, SCORE_MEMORY_INTERNAL_SIZE);
	if (IS_ERR(cookie)) {
		probe_err("vb2_ion_private_alloc is failed\n");
		return PTR_ERR(cookie);
	}

	ret = vb2_ion_dma_address(cookie, &dvaddr);
	if (ret) {
		probe_err("vb2_ion_dma_address is fail(%d)\n", ret);
		vb2_ion_private_free(cookie);
		return ret;
	}

	kvaddr = vb2_ion_private_vaddr(cookie);
	if (IS_ERR_OR_NULL(kvaddr)) {
		probe_err("vb2_ion_private_vaddr is failed\n");
		vb2_ion_private_free(cookie);
		return PTR_ERR(kvaddr);
	}

	mem->info.cookie = cookie;
	mem->info.dvaddr = dvaddr;
	mem->info.kvaddr = kvaddr;
#else
	ret = -ENOMEM;
#endif
	return ret;
}

static void __score_memory_free(struct score_memory *mem)
{
	struct score_memory_info *info = &mem->info;
#if defined(CONFIG_VIDEOBUF2_ION)
	vb2_ion_private_free(info->cookie);
#endif
	info->cookie = NULL;
	info->dvaddr = 0;
	info->kvaddr = NULL;
}

int score_memory_map_dmabuf(struct score_memory *mem, struct score_memory_buffer *buf)
{
	int ret = 0;
	void *mem_priv;
	void *cookie;
	dma_addr_t dvaddr;
	struct dma_buf *dbuf;
	const struct vb2_mem_ops *mem_ops;

#ifdef BUF_MAP_KVADDR
	void *kvaddr = NULL;
#endif
	cookie = NULL;

	mem_ops = mem->vb2_mem_ops;

	if (buf->m.fd <= 0) {
		score_err("fd(%d) is invalid\n", buf->m.fd);
		ret = -EINVAL;
		goto p_err;
	}

	dbuf = dma_buf_get(buf->m.fd);
	if (IS_ERR_OR_NULL(dbuf)) {
		score_err("dma_buf_get is fail(%d:%p)\n", buf->m.fd, dbuf);
		ret = -EINVAL;
		goto p_err;
	}

	buf->dbuf = dbuf;

	if (dbuf->size < buf->size) {
		score_err("user buffer size is small(%zd, %zd)\n", dbuf->size, buf->size);
		ret = -EINVAL;
		goto p_err;
	}

	if (buf->size != dbuf->size) {
		buf->size = dbuf->size;
	}

	/* Acquire each plane's memory */
	mem_priv = mem_ops->attach_dmabuf(mem->alloc_ctx,
			dbuf, buf->size, DMA_TO_DEVICE);
	if (IS_ERR(mem_priv)) {
		score_err("call_memop(attach_dmabuf) is fail(%p, %zd)\n",
				mem->alloc_ctx, buf->size);
		ret = PTR_ERR(mem_priv);
		goto p_err;
	}

	buf->mem_priv = mem_priv;

	cookie = mem_ops->cookie(mem_priv);
	if (IS_ERR_OR_NULL(cookie)) {
		score_err("call_memop(cookie) is fail(%p, %p)\n", mem_priv, cookie);
		ret = PTR_ERR(mem_priv);
		goto p_err;
	}

	buf->cookie = cookie;

	ret = mem_ops->map_dmabuf(mem_priv);
	if (ret) {
		score_err("call_memop(map_dmabuf) is fail(%d)\n", ret);
		goto p_err;
	}

	ret = vb2_ion_dma_address(cookie, &dvaddr);
	if (ret) {
		score_err("vb2_ion_dma_address is fail(%d)\n", ret);
		goto p_err;
	}

	buf->dvaddr = dvaddr;

#ifdef BUF_MAP_KVADDR
	kvaddr = mem_ops->vaddr(mem_priv);
	if (IS_ERR_OR_NULL(kvaddr)) {
		score_err("vb2_ion_private_vaddr is failed\n");
		goto p_err;
	}

	buf->kvaddr = kvaddr;
#endif
	return 0;
p_err:
	if (buf->dvaddr)
		mem_ops->unmap_dmabuf(buf->mem_priv);

	if (buf->mem_priv)
		mem_ops->detach_dmabuf(buf->mem_priv);

	if (buf->dbuf)
		dma_buf_put(buf->dbuf);

	return ret;
}

int score_memory_unmap_dmabuf(struct score_memory *mem, struct score_memory_buffer *buf)
{
	int ret = 0;
	const struct vb2_mem_ops *mem_ops;

	mem_ops = mem->vb2_mem_ops;

	if (buf->dvaddr)
		mem_ops->unmap_dmabuf(buf->mem_priv);

	if (buf->mem_priv)
		mem_ops->detach_dmabuf(buf->mem_priv);

	if (buf->dbuf)
		dma_buf_put(buf->dbuf);

	return ret;
}

int score_memory_compare_dmabuf(struct score_memory_buffer *buf,
		struct score_memory_buffer *listed_buf)
{
	int ret = 0;
	struct dma_buf *dbuf, *listed_dbuf;

	if (buf->m.fd <= 0) {
		score_err("fd(%d) is invalid\n", buf->m.fd);
		ret = -EINVAL;
		goto p_err;
	}

	dbuf = dma_buf_get(buf->m.fd);
	if (IS_ERR_OR_NULL(dbuf)) {
		score_err("dma_buf_get is fail(%d:%p)\n", buf->m.fd, dbuf);
		ret = -EINVAL;
		goto p_err;
	}

	listed_dbuf = (struct dma_buf *)listed_buf->dbuf;

	if ((dbuf == listed_dbuf) &&
			(dbuf->size == listed_dbuf->size) &&
			(dbuf->file == listed_dbuf->file) &&
			(dbuf->priv == listed_dbuf->priv)) {
		ret = 0;
	} else {
		ret = 1;
	}
	dma_buf_put(dbuf);

p_err:
	return ret;
}

static int score_memory_check_userptr(struct score_memory_buffer *buf)
{
	int ret = 0;
	unsigned long addr;
	struct vm_area_struct *vma;

	addr = buf->m.userptr;
	vma = find_vma(current->mm, addr);

	if (vma) {
		if (vma->vm_file) {
			if (vma->vm_file->private_data) {
				ret = 0;
			} else {
				ret = -EINVAL;
			}
		} else {
			ret = -EINVAL;
		}
	} else {
		ret = -EINVAL;
	}

	return ret;
}

int score_memory_map_userptr(struct score_memory *mem, struct score_memory_buffer *buf)
{
	int ret = 0;
	void *mem_priv;
	void *cookie;
	dma_addr_t dvaddr;
	const struct vb2_mem_ops *mem_ops;

#ifdef BUF_MAP_KVADDR
	void *kvaddr = NULL;
#endif
	cookie = NULL;
	mem_ops = mem->vb2_mem_ops;

	ret = score_memory_check_userptr(buf);
	if (ret) {
		score_err("Memory using dma_buf is only supported. \n");
		goto p_err;
	}

	mem_priv = mem_ops->get_userptr(mem->alloc_ctx,
			buf->m.userptr, buf->size, DMA_TO_DEVICE);
	if (IS_ERR(mem_priv)) {
		score_err("call_memop(attach_dmabuf) is fail(%p, %zd)\n",
				mem->alloc_ctx, buf->size);
		ret = PTR_ERR(mem_priv);
		goto p_err;
	}

	buf->mem_priv = mem_priv;

	cookie = mem_ops->cookie(mem_priv);
	if (IS_ERR_OR_NULL(cookie)) {
		score_err("call_memop(cookie) is fail(%p, %p)\n", mem_priv, cookie);
		ret = PTR_ERR(mem_priv);
		goto p_err;
	}

	buf->cookie = cookie;

	ret = vb2_ion_dma_address(cookie, &dvaddr);
	if (ret) {
		score_err("vb2_ion_dma_address is fail(%d)\n", ret);
		goto p_err;
	}

	buf->dvaddr = dvaddr;

#ifdef BUF_MAP_KVADDR
	kvaddr = mem_ops->vaddr(mem_priv);
	if (IS_ERR_OR_NULL(kvaddr)) {
		score_err("vb2_ion_private_vaddr is failed\n");
		goto p_err;
	}

	buf->kvaddr = kvaddr;
#endif

	return 0;
p_err:
	if (buf->dvaddr)
		mem_ops->unmap_dmabuf(buf->mem_priv);

	if (buf->mem_priv)
		mem_ops->detach_dmabuf(buf->mem_priv);

	return ret;
}

int score_memory_unmap_userptr(struct score_memory *mem,
		struct score_memory_buffer *buf)
{
	int ret = 0;
	const struct vb2_mem_ops *mem_ops;

	mem_ops = mem->vb2_mem_ops;

	if (buf->mem_priv)
		mem_ops->put_userptr(buf->mem_priv);

	return ret;
}

void score_memory_invalid_or_flush_userptr(struct score_memory_buffer *buf,
		int sync_for, enum dma_data_direction dir)
{
#ifdef USE_DIRECT_CACHE_CALL
	struct device *dev;
	struct score_vertex_ctx *vctx;
	struct score_vertex *vertex;
	struct score_device *device;

	vctx = container_of(buftracker, struct score_vertex_ctx, buftracker);
	vertex = vctx->vertex;
	device = container_of(vertex, struct score_device, vertex);
	dev = device->dev;

	if (sync_for == SCORE_MEMORY_SYNC_FOR_CPU)
		exynos_iommu_sync_for_cpu(dev, buf->dvaddr, buf->size, dir);
	else if(sync_for == SCORE_MEMORY_SYNC_FOR_DEVICE)
		exynos_iommu_sync_for_device(dev, buf->dvaddr, buf->size, dir);
#else
	if (sync_for == SCORE_MEMORY_SYNC_FOR_CPU)
		vb2_ion_sync_for_cpu(buf->cookie, 0, buf->size, dir);
	else if(sync_for == SCORE_MEMORY_SYNC_FOR_DEVICE)
		vb2_ion_sync_for_device(buf->cookie, 0, buf->size, dir);
#endif
}

int score_memory_open(struct score_memory *mem)
{
	int ret = 0;

	ret = __score_memory_alloc(mem);
	if (ret) {
#if defined(CONFIG_VIDEOBUF2_ION)
		mem->score_mem_ops->cleanup(mem->alloc_ctx);
#endif
	}

	return ret;
}

void score_memory_close(struct score_memory *mem)
{
	__score_memory_free(mem);
}

#if defined(CONFIG_VIDEOBUF2_ION)
const struct score_mem_ops score_mem_ops = {
	.cleanup	= vb2_ion_destroy_context,
	.resume		= vb2_ion_attach_iommu,
	.suspend	= vb2_ion_detach_iommu,
	.set_cacheable	= vb2_ion_set_cached,
};
#endif

int score_memory_probe(struct score_memory *mem, struct device *dev)
{
	u32 ret = 0;
#if defined(CONFIG_VIDEOBUF2_ION)
	struct vb2_alloc_ctx *alloc_ctx;
#endif

#if defined(CONFIG_VIDEOBUF2_ION)
	alloc_ctx = vb2_ion_create_context(dev, SZ_4K,
			VB2ION_CTX_IOMMU |
			VB2ION_CTX_VMCONTIG |
			VB2ION_CTX_UNCACHED);
	if (IS_ERR(alloc_ctx)) {
		probe_err("vb2_ion_create_context is fail\n");
		return PTR_ERR(alloc_ctx);
	}

	mem->dev = dev;
	mem->alloc_ctx = alloc_ctx;
	mem->score_mem_ops = &score_mem_ops;
	mem->vb2_mem_ops = &vb2_ion_memops;
#else
	mem->dev = dev;
	mem->alloc_ctx = NULL;

	ret = -ENOMEM;
#endif

	return ret;
}

void score_memory_release(struct score_memory *mem)
{
#if defined(CONFIG_VIDEOBUF2_ION)
	mem->score_mem_ops->cleanup(mem->alloc_ctx);
#endif
	mem->alloc_ctx = NULL;
}
