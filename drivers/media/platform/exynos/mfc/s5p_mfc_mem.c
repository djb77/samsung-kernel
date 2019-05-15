/*
 * drivers/media/platform/exynos/mfc/s5p_mfc_mem.c
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/iommu.h>

#include "s5p_mfc_mem.h"

struct vb2_mem_ops *s5p_mfc_mem_ops(void)
{
	return (struct vb2_mem_ops *)&vb2_ion_memops;
}

void s5p_mfc_mem_set_cacheable(void *alloc_ctx, bool cacheable)
{
	vb2_ion_set_cached(alloc_ctx, cacheable);
}

void s5p_mfc_mem_clean(void *vb_priv, void *start, off_t offset,
							size_t size)
{
	vb2_ion_sync_for_device(vb_priv, offset, size, DMA_TO_DEVICE);
}

void s5p_mfc_mem_invalidate(void *vb_priv, void *start, off_t offset,
							size_t size)
{
	vb2_ion_sync_for_device(vb_priv, offset, size, DMA_FROM_DEVICE);
}

int s5p_mfc_mem_clean_vb(struct vb2_buffer *vb, u32 num_planes)
{
	struct vb2_ion_cookie *cookie;
	int i;
	size_t size;

	for (i = 0; i < num_planes; i++) {
		cookie = vb2_plane_cookie(vb, i);
		if (!cookie)
			continue;

		size = vb->planes[i].length;
		vb2_ion_sync_for_device(cookie, 0, size, DMA_TO_DEVICE);
	}

	return 0;
}

int s5p_mfc_mem_inv_vb(struct vb2_buffer *vb, u32 num_planes)
{
	struct vb2_ion_cookie *cookie;
	int i;
	size_t size;

	for (i = 0; i < num_planes; i++) {
		cookie = vb2_plane_cookie(vb, i);
		if (!cookie)
			continue;

		size = vb->planes[i].length;
		vb2_ion_sync_for_device(cookie, 0, size, DMA_FROM_DEVICE);
	}

	return 0;
}

int s5p_mfc_mem_get_user_shared_handle(struct s5p_mfc_ctx *ctx,
	struct mfc_user_shared_handle *handle)
{
	struct s5p_mfc_dev *dev = ctx->dev;
	int ret = 0;

	handle->ion_handle =
		ion_import_dma_buf(dev->mfc_ion_client, handle->fd);
	if (IS_ERR(handle->ion_handle)) {
		mfc_err_ctx("Failed to import fd\n");
		ret = PTR_ERR(handle->ion_handle);
		goto import_dma_fail;
	}

	handle->vaddr =
		ion_map_kernel(dev->mfc_ion_client, handle->ion_handle);
	if (handle->vaddr == NULL) {
		mfc_err_ctx("Failed to get kernel virtual address\n");
		ret = -EINVAL;
		goto map_kernel_fail;
	}

	mfc_debug(2, "User Handle: fd = %d, virtual addr = 0x%p\n",
				handle->fd, handle->vaddr);

	return 0;

map_kernel_fail:
	ion_free(dev->mfc_ion_client, handle->ion_handle);

import_dma_fail:
	return ret;
}

int s5p_mfc_mem_cleanup_user_shared_handle(struct s5p_mfc_ctx *ctx,
		struct mfc_user_shared_handle *handle)
{
	struct s5p_mfc_dev *dev = ctx->dev;

	if (handle->fd == -1)
		return 0;

	if (handle->vaddr)
		ion_unmap_kernel(dev->mfc_ion_client,
					handle->ion_handle);

	ion_free(dev->mfc_ion_client, handle->ion_handle);

	return 0;
}

void s5p_mfc_put_iovmm(struct s5p_mfc_ctx *ctx, int num_planes, int index)
{
	struct s5p_mfc_dec *dec = ctx->dec_priv;
	int i;

	for (i = 0; i < num_planes; i++) {
		if (dec->assigned_addr[index][i]) {
			mfc_debug(2, "[IOVMM] index %d buf[%d] addr: %#llx\n",
					index, i, dec->assigned_addr[index][i]);
			ion_iovmm_unmap(dec->assigned_attach[index][i], dec->assigned_addr[index][i]);
		}
		if (dec->assigned_attach[index][i])
			dma_buf_detach(dec->assigned_dmabufs[index][i], dec->assigned_attach[index][i]);
		if (dec->assigned_dmabufs[index][i])
			dma_buf_put(dec->assigned_dmabufs[index][i]);

		dec->assigned_addr[index][i] = 0;
		dec->assigned_attach[index][i] = NULL;
		dec->assigned_dmabufs[index][i] = NULL;
	}

	dec->assigned_refcnt[index]--;
	mfc_debug(2, "[IOVMM] index %d ref %d\n", index, dec->assigned_refcnt[index]);
}

void s5p_mfc_get_iovmm(struct s5p_mfc_ctx *ctx, struct vb2_buffer *vb)
{
	struct s5p_mfc_dev *dev = ctx->dev;
	struct s5p_mfc_dec *dec = ctx->dec_priv;
	int i, mem_get_count = 0;
	int index = vb->index;
	int ioprot = IOMMU_WRITE;

	for (i = 0; i < ctx->dst_fmt->mem_planes; i++) {
		mem_get_count++;

		dec->assigned_dmabufs[index][i] = dma_buf_get(vb->planes[i].m.fd);
		if (IS_ERR(dec->assigned_dmabufs[index][i])) {
			mfc_err_ctx("[IOVMM] Failed to dma_buf_get (err %ld)\n",
					PTR_ERR(dec->assigned_dmabufs[index][i]));
			dec->assigned_dmabufs[index][i] = NULL;
			goto err_iovmm;
		}

		dec->assigned_attach[index][i] = dma_buf_attach(dec->assigned_dmabufs[index][i], dev->device);
		if (IS_ERR(dec->assigned_attach[index][i])) {
			mfc_err_ctx("[IOVMM] Failed to get dma_buf_attach (err %ld)\n",
					PTR_ERR(dec->assigned_attach[index][i]));
			dec->assigned_attach[index][i] = NULL;
			goto err_iovmm;
		}

		if (device_get_dma_attr(dev->device) == DEV_DMA_COHERENT)
			ioprot |= IOMMU_CACHE;

		dec->assigned_addr[index][i] = ion_iovmm_map(dec->assigned_attach[index][i],
				0, ctx->raw_buf.plane_size[i], DMA_FROM_DEVICE, ioprot);
		if (IS_ERR_VALUE(dec->assigned_addr[index][i])) {
			mfc_err_ctx("[IOVMM] Failed to allocate iova (err 0x%p)\n",
					&dec->assigned_addr[index][i]);
			dec->assigned_addr[index][i] = 0;
			goto err_iovmm;
		}
		mfc_debug(2, "[IOVMM] index %d buf[%d] addr: %#llx\n",
				index, i, dec->assigned_addr[index][i]);
	}

	dec->assigned_refcnt[index]++;
	mfc_debug(2, "[IOVMM] index %d ref %d\n", index, dec->assigned_refcnt[index]);

	return;

err_iovmm:
	dec->assigned_refcnt[index]++;
	s5p_mfc_put_iovmm(ctx, mem_get_count, index);
}

void s5p_mfc_cleanup_assigned_iovmm(struct s5p_mfc_ctx *ctx)
{
	struct s5p_mfc_dev *dev = ctx->dev;
	struct s5p_mfc_dec *dec = ctx->dec_priv;
	int i;

	mutex_lock(&dec->dpb_mutex);

	for (i = 0; i < MFC_MAX_DPBS; i++) {
		if (dec->assigned_refcnt[i] == 0) {
			continue;
		} else if (dec->assigned_refcnt[i] == 1) {
			s5p_mfc_put_iovmm(ctx, ctx->dst_fmt->mem_planes, i);
		} else {
			mfc_err_ctx("[IOVMM] index %d invalid refcnt %d\n", i, dec->assigned_refcnt[i]);
			MFC_TRACE_CTX("DPB[%d] %#llx invalid refcnt %d\n",
					i, dec->assigned_addr[i][0],
					dec->assigned_refcnt[i]);
		}
	}

	mutex_unlock(&dec->dpb_mutex);
}
