/* drivers/media/platform/exynos/astc/astc-helper.c
 *
 * Implementation of helper functions for Samsung ASTC encoder driver
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Author: Andrea Bernabei <a.bernabei@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

//#define DEBUG

#include <linux/kernel.h>
#include <linux/exynos_iovmm.h>
#include <linux/exynos_ion.h>
#include <linux/mm.h>
#include <linux/bitmap.h>
#include <asm/bitops.h>

#include "astc-helper.h"

#include "astc-exynos9810-workaround.h"

/* Pixel format to bits-per-pixel */
int ftm_to_bpp(struct device *dev, struct hwASTC_img_info *img_info) {
	switch (img_info->fmt) {
	case ASTC_ENCODE_INPUT_PIXEL_FORMAT_RGBA8888:  /* RGBA 8888 */
	case ASTC_ENCODE_INPUT_PIXEL_FORMAT_ARGB8888:  /* ARGB 8888 */
	case ASTC_ENCODE_INPUT_PIXEL_FORMAT_BGRA8888:  /* BGRA 8888 */
	case ASTC_ENCODE_INPUT_PIXEL_FORMAT_ABGR8888:  /* ABGR 8888 */
		return 4;
		break;
	default:
		dev_err(dev, "invalid input color format (%u)", img_info->fmt);
		return -EINVAL;
	}
}

/* Convert blksize enum to the corresponding block size in integer */
u32 blksize_enum_to_int(enum hwASTC_blk_size blk_size, int x_or_y)
{
	switch (blk_size) {
	case ASTC_ENCODE_MODE_BLK_SIZE_4x4: return 4;
	case ASTC_ENCODE_MODE_BLK_SIZE_6x6: return 6;
	case ASTC_ENCODE_MODE_BLK_SIZE_8x8: return 8;
	/*
	 * Example of future case, once the hw supports different block sizes for x/y
	 * case ASTC_ENCODE_MODE_BLK_SIZE_4x6: return x_or_y ? 6 : 4;
	 */
	}

	/* unsupported block size */
	return 0;
}
u32 blksize_enum_to_x(enum hwASTC_blk_size blk_size) {
	return blksize_enum_to_int(blk_size, 0);
}
u32 blksize_enum_to_y(enum hwASTC_blk_size blk_size) {
	return blksize_enum_to_int(blk_size, 1);
}

/**
 * astc_map_dma_attachment() - Map the DMA attachment, for buffers backed by a dmabuf
 * @dev: the platform device
 * @plane: the internal representation of the buffer
 * @dir: the direction of the DMA transfer that involves the buffer @plane
 *
 * This is only needed for buffers backed by a dmabuf.
 *
 * Allocate and return a scatterlist mapped in the device
 * address space. The scatterlist point to physical pages.
 * We will then use IOMMU to map a CONTIGUOUS chunk of virtual
 * memory to it, that we will use for the DMA transfers.
 *
 * Return:
 *	0 on success,
 *	a PTR_ERR value otherwise
 */
int astc_map_dma_attachment(struct device *dev,
		     struct astc_buffer_plane_dma *plane,
		     enum dma_data_direction dir)
{
	dev_dbg(dev, "%s: mapping dmabuf %p, attachment %p, DMA_TO_DEVICE? %d\n"
		, __func__, plane->dmabuf, plane->attachment
		, dir == DMA_TO_DEVICE);

	if (plane->dmabuf) {

		plane->sgt = dma_buf_map_attachment(plane->attachment, dir);
		if (IS_ERR(plane->sgt)) {
			dev_err(dev, "%s: failed to map attacment of dma_buf\n",
				__func__);
			return PTR_ERR(plane->sgt);
		}
		dev_dbg(dev, "%s: successfully got scatterlist %p for dmabuf %p, DMA_TO_DEVICE? %d\n"
			, __func__, plane->sgt, plane->dmabuf
			, dir == DMA_TO_DEVICE);
	}

	return 0;
}

/**
 * astc_unmap_dma_attachment() - Unmap the DMA attachment, for buffers backed by a dmabuf
 * @dev: the platform device
 * @plane: the internal representation of the buffer
 * @dir: the direction of the DMA transfer that involves the buffer @plane
 *
 * This is only needed for buffers backed by a dmabuf.
 * Unmap the DMA attachment to tell the exporter that we are done using
 * the dma buffer.
 */
void astc_unmap_dma_attachment(struct device *dev,
			struct astc_buffer_plane_dma *plane,
			enum dma_data_direction dir)
{
	dev_dbg(dev, "%s: unmapping dmabuf %p, attachment %p, scatter table %p, DMA_TO_DEVICE? %d\n"
		, __func__, plane->dmabuf, plane->attachment, plane->sgt
		, dir == DMA_TO_DEVICE);

	/* tell the exporter of the DMA buffer that we are done using it */
	if (plane->dmabuf)
		dma_buf_unmap_attachment(plane->attachment, plane->sgt, dir);
}

/**
 * astc_map_buf_to_device() - Use IOVMM to map a memory region to device address space
 * @astc_device: the astc device struct
 * @task: the task that is being setup
 * @buf: internal representation of the buffer
 * @dir: the direction of the DMA transfer that will involve @buffer
 *
 * Use IOVMM to map the memory region of @buffer to the device address
 * space and get the DMA address to it, so that the device can do
 * DMA transfers from/to it.
 *
 * When the user buffer is represented by a pointer to user memory and is
 * not backed by a dmabuf, use the Exynos IOVMM to map it to the device
 * address space via the IOMMU.
 * Otherwise, when it is backed by a dmabuf attachment,
 * use ION IOVMM to map it to device address space.
 *
 * When the driver is running on Exynos9810 SoC, this also takes
 * care of mapping additional pages (if needed) to workaround a H/W bug.
 *
 * TODO: add workaround for Exynos9810 H/W bug for buffers backed by
 * a dmabuf.
 *
 * Return:
 *	 0 on success,
 *	<0 error code otherwise
 */
int astc_map_buf_to_device(struct astc_dev *astc_device,
		      struct astc_task *task,
		      struct astc_buffer_dma *buf,
		      enum dma_data_direction dir)
{
	struct device *dev = astc_device->dev;
	struct astc_buffer_plane_dma *plane = &buf->plane;
	int prot = IOMMU_READ;
	dma_addr_t iova;

	if (!task) {
		dev_err(dev, "%s: null task\n", __func__);
		/*
		 * return a value outside of ERRNO, to avoid confusion
		 * with the values returned by exynos_iovmm_map_userptr
		 */
		return (-MAX_ERRNO - 1);
	}

	dev_dbg(dev, "%s: using iovmm to get dma address, DMA_TO_DEVICE? %d\n"
		, __func__, dir == DMA_TO_DEVICE);

	if (dir != DMA_TO_DEVICE)
		prot |= IOMMU_WRITE;

	/*
	 * IOMMU_CACHE will cause IOMMU to toggle a specific bit in the page
	 * descriptors of the buffer to signal coherence managers/protocols that
	 * those pages are shared so the transfers need to be coherent.
	 *
	 * NOTE: the assumption here is that there is something in the device or
	 * between the device and the rest of the SoC that will pick the hint up
	 * and will enforce coherence.
	 *
	 * In Exynos9810 the ASTC device itself is not I/O coherent (AXI),
	 * BUT it is connected to an ACE-LITE adapter, which provides coherence,
	 * so the combination is coherent and we can use this prot.
	 */
	if (device_get_dma_attr(dev) == DEV_DMA_COHERENT)
		prot |= IOMMU_CACHE;

	if (plane->dmabuf) {
		dev_dbg(dev, "%s: getting dma addr of plane, DMA attachment %p bytes %zu, DMA_TO_DEVICE? %d\n"
			, __func__, plane->attachment, plane->bytes_used
			, dir == DMA_TO_DEVICE);

		iova = ion_iovmm_map(plane->attachment, 0,
				     plane->bytes_used, dir, prot);
	} else {
		dev_dbg(dev, "%s: getting dma addr of plane, userptr %lu bytes %zu, DMA_TO_DEVICE? %d\n"
			, __func__, buf->buffer->userptr
			, plane->bytes_used, dir == DMA_TO_DEVICE);
		down_read(&current->mm->mmap_sem);
		iova = exynos_iovmm_map_userptr(dev,
						buf->buffer->userptr,
						plane->bytes_used, prot);
		up_read(&current->mm->mmap_sem);
	}

	if (IS_ERR_VALUE(iova)) {
		if (is_exynos9810bug_workaround_active(task)) {
			dev_err(dev, "%s: the image buffer is probably too small. The ASTC encoder of this SoC requires the image buffer size to be big enough to hold data of an image where width and height are rounded up to be multiple of block size.\n", __func__);
		}
		return (int)iova;
	}

	buf->plane.dma_addr = iova + plane->offset;

	/* Check that the addition did not wrap around */
	if (buf->plane.dma_addr < iova) {
		/* undo mapping */
		if (plane->dmabuf) {
			dev_err(dev, "%s: WRAPAROUND UNMAPPING NOT IMPLEMENTED\n", __func__);
		} else {
			exynos_iovmm_unmap_userptr(dev, iova);
		}

		dev_err(dev, "%s: dma addr computation wrapped around\n", __func__);
		return (-MAX_ERRNO - 4);
	}

	dev_dbg(dev, "%s: got dma address %pad, DMA_TO_DEVICE? %d\n"
		,  __func__, &buf->plane.dma_addr, dir == DMA_TO_DEVICE);
	return 0;
}

/**
 * astc_unmap_buf_from_device() - Use IOVMM to unmap the buffer at a given DMA address
 * @dev: the platform device struct
 * @buf: internal representation of the buffer
 *
 * Unmap the device-space buffer which is at the DMA address held in the
 * @buf struct.
 *
 * When the user buffer is represented by a pointer to user memory which is
 * not backed by a dmabuf, use the Exynos IOVMM to unmap it.
 * Otherwise, use ION IOVMM to unmap it.
 *
 * Return:
 *	 0 on success,
 *	-ERRNO if the mapping fails,
 *	values < -ERRNO for other errors.
 */
int astc_unmap_buf_from_device(struct device *dev,
			 struct astc_buffer_dma *buf)
{
	struct astc_buffer_plane_dma *plane = &buf->plane;
	dma_addr_t dma_addr = plane->dma_addr - plane->offset;

	if (dma_addr > plane->dma_addr) {
		dev_err(dev, "%s: dma addr computation wrapped around\n", __func__);
		return -1;
	}

	dev_dbg(dev, "%s: umapping dma address %pad\n"
		,  __func__, &dma_addr);

	if (plane->dmabuf)
		ion_iovmm_unmap(plane->attachment, dma_addr);
	else
		exynos_iovmm_unmap_userptr(dev, dma_addr);

	plane->dma_addr = 0;

	return 0;
}

/* CPU cache sync procedures, needed if the device is not dma-coherent.
 * Unused on Exynos981, as HW ASTC device on Exynos9810 is DMA coherent. */
void astc_sync_for_device(struct device *dev,
			  struct astc_buffer_plane_dma *plane,
			  enum dma_data_direction dir)
{
	if (plane->dmabuf)
		exynos_ion_sync_dmabuf_for_device(dev, plane->dmabuf,
						  plane->bytes_used, dir);
	else
		exynos_iommu_sync_for_device(dev, plane->dma_addr,
					     plane->bytes_used, dir);
}

void astc_sync_for_cpu(struct device *dev,
		       struct astc_buffer_plane_dma *plane,
		       enum dma_data_direction dir)
{
	if (plane->dmabuf)
		exynos_ion_sync_dmabuf_for_cpu(dev, plane->dmabuf,
					       plane->bytes_used, dir);
	else
		exynos_iommu_sync_for_cpu(dev, plane->dma_addr,
					  plane->bytes_used, dir);
}
