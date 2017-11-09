/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/version.h>
#include <linux/device.h>
#include <media/videobuf2-ion.h>
#include <linux/exynos_iovmm.h>

#include "vpu-config.h"
#include "vpu-binary.h"
#include "vpu-memory.h"

#if defined(CONFIG_VIDEOBUF2_ION)
const struct vpu_mem_ops vpu_mem_ops = {
	.cleanup	= vb2_ion_destroy_context,
	.resume		= vb2_ion_attach_iommu,
	.suspend	= vb2_ion_detach_iommu,
	.set_cacheable	= vb2_ion_set_cached,
};
#endif

int vpu_memory_probe(struct vpu_memory *memory, struct device *dev)
{
	u32 ret = 0;
	struct vb2_alloc_ctx *alloc_ctx;

	BUG_ON(!memory);
	BUG_ON(!dev);

	alloc_ctx = NULL;

#if defined(CONFIG_VIDEOBUF2_ION)
	alloc_ctx = vb2_ion_create_context(dev, SZ_4K, VB2ION_CTX_IOMMU |
		VB2ION_CTX_VMCONTIG | VB2ION_CTX_UNCACHED);
	if (IS_ERR(alloc_ctx)) {
		probe_err("vb2_ion_create_context is fail\n");
		return PTR_ERR(alloc_ctx);
	}

	memory->dev = dev;
	memory->alloc_ctx = alloc_ctx;
	memory->vpu_mem_ops = &vpu_mem_ops;
	memory->vb2_mem_ops = &vb2_ion_memops;
#endif

	spin_lock_init(&memory->im_lock);
	INIT_LIST_HEAD(&memory->im_list);
	memory->im_count = 0;

	return ret;
}

static int __vpu_memory_alloc(struct vpu_memory *memory)
{
	int ret = 0;
	void *cookie, *kvaddr;
	dma_addr_t dvaddr;
	struct vpu_memory_info *info;

	cookie = NULL;
	kvaddr = NULL;
	dvaddr = 0;
	info = &memory->info;

#if defined(CONFIG_VIDEOBUF2_ION)
	cookie = vb2_ion_private_alloc(memory->alloc_ctx, VPU_MEMORY_INTERNAL_SIZE);
	if (IS_ERR(cookie)) {
		vpu_err("vb2_ion_private_alloc is failed");
		return PTR_ERR(cookie);
	}

	ret = vb2_ion_dma_address(cookie, &dvaddr);
	if (ret) {
		vpu_err("vb2_ion_dma_address is fail(%d)\n", ret);
		vb2_ion_private_free(cookie);
		return ret;
	}

	kvaddr = vb2_ion_private_vaddr(cookie);
	if (IS_ERR_OR_NULL(kvaddr)) {
		vpu_err("vb2_ion_private_vaddr is failed");
		vb2_ion_private_free(cookie);
		return PTR_ERR(kvaddr);
	}

	info->cookie = cookie;
	info->dvaddr = dvaddr;
	info->kvaddr = kvaddr;
#endif

	vpu_info("internal dvaddr : 0x%llX\n", dvaddr);
	vpu_info("internal kvaddr : %p\n", kvaddr);
	return ret;
}

int vpu_memory_open(struct vpu_memory *memory)
{
	int ret = 0;

	if (VPU_MEMORY_INTERNAL_SIZE == 0)
		goto p_err;

	ret = __vpu_memory_alloc(memory);
	if (ret) {
		vpu_err("__vpu_memory_alloc is fail(%d)\n", ret);
		goto p_err;
	}

p_err:
	return ret;
}

int vpu_memory_close(struct vpu_memory *memory)
{
	int ret = 0;

	if (VPU_MEMORY_INTERNAL_SIZE == 0)
		goto p_err;

#if defined(CONFIG_VIDEOBUF2_ION)
	vb2_ion_private_free(memory->info.cookie);
#endif

p_err:
	return ret;
}

int vpu_memory_map(struct vpu_memory *memory, struct vpu_memory_buffer *buffer)
{
	int ret = 0;
	void *mem_priv;
	void *cookie;
	dma_addr_t dvaddr;
	struct dma_buf *dbuf;
	const struct vb2_mem_ops *mem_ops;
	unsigned long flags;

	BUG_ON(!memory);
	BUG_ON(!buffer);

	mem_ops = memory->vb2_mem_ops;

	if (buffer->fd <= 0) {
		vpu_err("fd(%d) is invalid\n", buffer->fd);
		ret = -EINVAL;
		goto p_err;
	}

	dbuf = dma_buf_get(buffer->fd);
	if (IS_ERR_OR_NULL(dbuf)) {
		vpu_err("dma_buf_get is fail(%d:%p)\n", buffer->fd, dbuf);
		ret = -EINVAL;
		goto p_err;
	}

	buffer->dbuf = dbuf;

	if (dbuf->size < buffer->size) {
		vpu_err("user buffer size is small(%zd, %zd)\n", dbuf->size, buffer->size);
		ret = -EINVAL;
		goto p_err;
	}

	/* Acquire each plane's memory */
	mem_priv = mem_ops->attach_dmabuf(memory->alloc_ctx, dbuf, buffer->size, 1);
	if (IS_ERR(mem_priv)) {
		vpu_err("call_memop(attach_dmabuf) is fail(%p, %zd)\n", memory->alloc_ctx, buffer->size);
		ret = PTR_ERR(mem_priv);
		goto p_err;
	}

	buffer->mem_priv = mem_priv;

	cookie = mem_ops->cookie(mem_priv);
	if (IS_ERR_OR_NULL(cookie)) {
		vpu_err("call_memop(cookie) is fail(%p, %p)\n", mem_priv, cookie);
		ret = PTR_ERR(mem_priv);
		goto p_err;
	}

	buffer->cookie = cookie;

	ret = mem_ops->map_dmabuf(mem_priv);
	if (ret) {
		vpu_err("call_memop(map_dmabuf) is fail(%d)\n", ret);
		goto p_err;
	}

	ret = vb2_ion_dma_address(cookie, &dvaddr);
	if (ret) {
		vpu_err("vb2_ion_dma_address is fail(%d)\n", ret);
		goto p_err;
	}

	buffer->dvaddr = dvaddr;

	spin_lock_irqsave(&memory->im_lock, flags);
	list_add_tail(&buffer->list, &memory->im_list);
	memory->im_count++;
	spin_unlock_irqrestore(&memory->im_lock, flags);

	return 0;

p_err:
	if (buffer->dvaddr)
		mem_ops->unmap_dmabuf(buffer->mem_priv);

	if (buffer->mem_priv)
		mem_ops->detach_dmabuf(buffer->mem_priv);

	if (buffer->dbuf)
		dma_buf_put(buffer->dbuf);

	return ret;
}

int vpu_memory_unmap(struct vpu_memory *memory, struct vpu_memory_buffer *buffer)
{
	int ret = 0;
	const struct vb2_mem_ops *mem_ops;
	unsigned long flags;

	BUG_ON(!memory);
	BUG_ON(!buffer);

	mem_ops = memory->vb2_mem_ops;

	if (buffer->dvaddr)
		mem_ops->unmap_dmabuf(buffer->mem_priv);

	if (buffer->mem_priv)
		mem_ops->detach_dmabuf(buffer->mem_priv);

	if (buffer->dbuf)
		dma_buf_put(buffer->dbuf);

	spin_lock_irqsave(&memory->im_lock, flags);
	list_del(&buffer->list);
	memory->im_count--;
	spin_unlock_irqrestore(&memory->im_lock, flags);

	return ret;
}