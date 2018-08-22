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

#include "s5p_mfc_mem.h"

struct vb2_mem_ops *s5p_mfc_mem_ops(void)
{
	return (struct vb2_mem_ops *)&vb2_ion_memops;
}

void s5p_mfc_mem_clean(struct s5p_mfc_dev *dev,
			struct s5p_mfc_special_buf *special_buf,
			off_t offset, size_t size)
{
	exynos_ion_sync_vaddr_for_device(dev->device, special_buf->dma_buf,
			special_buf->vaddr, size, offset, DMA_TO_DEVICE);
	return;
}

void s5p_mfc_mem_invalidate(struct s5p_mfc_dev *dev,
			struct s5p_mfc_special_buf *special_buf,
			off_t offset, size_t size)
{
	exynos_ion_sync_vaddr_for_device(dev->device, special_buf->dma_buf,
			special_buf->vaddr, size, offset, DMA_FROM_DEVICE);
	return;
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
		ion_import_dma_buf_fd(dev->mfc_ion_client, handle->fd);
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

void *s5p_mfc_mem_get_vaddr(struct s5p_mfc_dev *dev,
		struct s5p_mfc_special_buf *special_buf)
{
	return ion_map_kernel(dev->mfc_ion_client, special_buf->handle);
}

void s5p_mfc_mem_ion_free(struct s5p_mfc_dev *dev,
		struct s5p_mfc_special_buf *special_buf)
{
	if (!special_buf->handle)
		return;

	if (special_buf->daddr)
		ion_iovmm_unmap(special_buf->attachment, special_buf->daddr);

	if (special_buf->handle) {
		dma_buf_unmap_attachment(special_buf->attachment,
				special_buf->sgt, DMA_BIDIRECTIONAL);
		dma_buf_detach(special_buf->dma_buf, special_buf->attachment);
		dma_buf_put(special_buf->dma_buf);
		ion_free(dev->mfc_ion_client, special_buf->handle);
	}

	memset(&special_buf->handle, 0, sizeof(struct ion_handle *));
	memset(&special_buf->daddr, 0, sizeof(special_buf->daddr));

	special_buf->handle = NULL;
	special_buf->dma_buf = NULL;
	special_buf->attachment = NULL;
	special_buf->sgt = NULL;
	special_buf->daddr = 0;
	special_buf->vaddr = NULL;
}

int s5p_mfc_mem_ion_alloc(struct s5p_mfc_dev *dev,
		struct s5p_mfc_special_buf *special_buf)
{
	struct s5p_mfc_ctx *ctx = dev->ctx[dev->curr_ctx];
	int ion_mask, flag;

	switch (special_buf->buftype) {
	case MFCBUF_NORMAL:
		ion_mask = EXYNOS_ION_HEAP_SYSTEM_MASK;
		flag = 0;
		break;
	case MFCBUF_NORMAL_FW:
		ion_mask = EXYNOS_ION_HEAP_VIDEO_NFW_MASK;
		flag = 0;
		break;
	case MFCBUF_DRM:
		ion_mask = EXYNOS_ION_HEAP_VIDEO_FRAME_MASK;
		flag = ION_FLAG_PROTECTED;
		break;
	case MFCBUF_DRM_FW:
		ion_mask = EXYNOS_ION_HEAP_VIDEO_FW_MASK;
		flag = ION_FLAG_PROTECTED;
		break;
	default:
		mfc_err_ctx("not supported mfc mem type: %d\n",
				special_buf->buftype);
		return -EINVAL;
	}
	special_buf->handle = ion_alloc(dev->mfc_ion_client,
			special_buf->size, 0, ion_mask, flag);
	if (IS_ERR(special_buf->handle)) {
		mfc_err_ctx("Failed to allocate buffer (err %ld)",
				PTR_ERR(special_buf->handle));
		special_buf->handle = NULL;
		goto err_ion_alloc;
	}

	special_buf->dma_buf = ion_share_dma_buf(dev->mfc_ion_client,
			special_buf->handle);
	if (IS_ERR(special_buf->dma_buf)) {
		mfc_err_ctx("Failed to get dma_buf (err %ld)",
				PTR_ERR(special_buf->dma_buf));
		special_buf->dma_buf = NULL;
		goto err_ion_alloc;
	}

	special_buf->attachment = dma_buf_attach(special_buf->dma_buf, dev->device);
	if (IS_ERR(special_buf->attachment)) {
		mfc_err_ctx("Failed to get dma_buf_attach (err %ld)",
				PTR_ERR(special_buf->attachment));
		special_buf->attachment = NULL;
		goto err_ion_alloc;
	}

	special_buf->sgt = dma_buf_map_attachment(special_buf->attachment,
			DMA_BIDIRECTIONAL);
	if (IS_ERR(special_buf->sgt)) {
		mfc_err_ctx("Failed to get sgt (err %ld)",
				PTR_ERR(special_buf->sgt));
		special_buf->sgt = NULL;
		goto err_ion_alloc;
	}

	special_buf->daddr = ion_iovmm_map(special_buf->attachment, 0,
			special_buf->size, DMA_BIDIRECTIONAL, 0);
	if (IS_ERR_VALUE(special_buf->daddr)) {
		mfc_err_ctx("Failed to allocate iova (err %pa)",
				&special_buf->daddr);
		special_buf->daddr = 0;
		goto err_ion_alloc;
	}

	return 0;

err_ion_alloc:
	s5p_mfc_mem_ion_free(dev, special_buf);
	return -ENOMEM;
}
