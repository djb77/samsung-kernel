/* sound/soc/samsung/abox/abox_mmap_fd.c
 *
 * ALSA SoC Audio Layer - Samsung Abox mmap_fd driver
 *
 * Copyright (c) 2018 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
/* #define DEBUG */
#include <sound/samsung/abox.h>
#include <sound/sounddev_abox.h>

#include <linux/dma-buf.h>
#include <uapi/linux/dma-buf.h>
#include <linux/exynos_ion.h>
#if defined(CONFIG_VIDEOBUF2_CMA_PHYS)
#include <media/videobuf2-cma-phys.h>
#elif defined(CONFIG_VIDEOBUF2_ION)
#include <media/videobuf2-ion.h>
#endif

#include "../../../../drivers/iommu/exynos-iommu.h"
#include "../../../../drivers/staging/android/uapi/ion.h"

#include "abox.h"

int abox_mmap_fd(struct abox_platform_data *data,
		struct snd_pcm_mmap_fd *mmap_fd)
{
	struct device *dev = &data->pdev->dev;
	int id = data->id;
	struct abox_ion_buf *buf = &data->ion_buf;
	struct dma_buf *temp_buf;
	int ret = 0;

	dev_dbg(dev, "%s id(%d)\n", __func__, id);

	if (buf->fd >= 0) {
		mmap_fd->dir = SNDRV_PCM_STREAM_PLAYBACK;
		mmap_fd->size = buf->size;
		mmap_fd->actual_size = buf->size;
		mmap_fd->fd = buf->fd;
	} else {
		mmap_fd->fd = dma_buf_fd(buf->dma_buf, O_CLOEXEC);
		if (mmap_fd->fd >= 0) {
			mmap_fd->dir = SNDRV_PCM_STREAM_PLAYBACK;
			mmap_fd->size = buf->size;
			mmap_fd->actual_size = buf->size;
		} else {
			ret = -EFAULT;
			dev_err(dev, "%s dma_buf_fd is failed\n", __func__);
			dma_buf_put(buf->dma_buf);

			goto error_get_fd;
		}

		buf->fd = mmap_fd->fd;
		dev_dbg(dev, "%s fd(%d)\n", __func__, buf->fd);
		temp_buf = dma_buf_get(buf->fd);
		data->mmap_fd_state = true;

		dev_dbg(dev, "%s-(%p)(%p)\n", __func__, buf->dma_buf, temp_buf);
	}
	dev_info(dev, "%s id(%d) fd(%d)\n", __func__, id, buf->fd);

error_get_fd:
	return ret;
}

int abox_ion_alloc(struct abox_platform_data *data,
		struct abox_ion_buf *buf,
		unsigned long iova,
		size_t size,
		size_t align)
{
	struct device *dev = &data->pdev->dev;
	struct device *dev_abox = &data->abox_data->pdev->dev;

	int heapflags = EXYNOS_ION_HEAP_SYSTEM_MASK;
	int ret = 0;

	if (!buf)
		return -ENOMEM;

	size = PAGE_ALIGN(size);

	buf->client = data->abox_data->client;
	buf->alignment = SZ_4K;
	buf->flags = ION_FLAG_SYNC_FORCE;

	buf->handle = ion_alloc(buf->client, size, buf->alignment,
				heapflags, buf->flags);
	if (IS_ERR(buf->handle)) {
		ret = -ENOMEM;
		goto error_alloc;
	}

	/* ion_share_dma_buf will call dam_buf_get */
	buf->dma_buf = ion_share_dma_buf(buf->client, buf->handle);
	if (IS_ERR(buf->dma_buf)) {
		ret = PTR_ERR(buf->dma_buf);
		goto error_share;
	}

	buf->attachment = dma_buf_attach(buf->dma_buf, dev_abox);
	if (IS_ERR(buf->attachment)) {
		ret = -ENOMEM;
		goto error_attach;
	}

	buf->cookie.sgt = dma_buf_map_attachment(
				buf->attachment, DMA_BIDIRECTIONAL);
	if (IS_ERR(buf->cookie.sgt)) {
		ret = -ENOMEM;
		goto error_map_dmabuf;
	}

	if (!buf->kva)
		buf->kva = ion_map_kernel(buf->client, buf->handle);
	if (IS_ERR_OR_NULL(buf->kva)) {
		ret = -ENOMEM;
		goto error_dma_buf_vmap;
	}

	buf->size = size;
	buf->cookie.ioaddr = iova;
	buf->direction = DMA_BIDIRECTIONAL;

	ret = abox_iommu_map_sg(&data->abox_data->pdev->dev,
			buf->cookie.ioaddr,
			buf->cookie.sgt->sgl,
			buf->cookie.sgt->nents,
			DMA_BIDIRECTIONAL,
			buf->size,
			buf->kva);

	if (ret < 0) {
		dev_err(dev, "Failed to iommu_map: %d\n", ret);

		ret = -ENOMEM;
		goto error_iommu_map_sg;
	}

	dev_info(dev, "%s buf(0x%lx, 0x%llx, %p)\n",
			__func__,
			buf->size,
			buf->cookie.ioaddr,
			buf->kva);

	return ret;

error_iommu_map_sg:
	ion_unmap_kernel(buf->client, buf->handle);
error_dma_buf_vmap:
	dma_buf_unmap_attachment(buf->attachment, buf->cookie.sgt,
				DMA_BIDIRECTIONAL);
error_map_dmabuf:
	dma_buf_detach(buf->dma_buf, buf->attachment);
error_attach:
	dma_buf_put(buf->dma_buf);
error_share:
	ion_free(buf->client, buf->handle);
error_alloc:

	dev_err(dev, "%s: Error occured while allocating\n", __func__);
	return ret;
}

int abox_ion_free(struct abox_platform_data *data)
{
	struct device *dev = &data->pdev->dev;

	int ret = 0;

	ret = abox_iommu_unmap(&data->abox_data->pdev->dev,
			data->ion_buf.cookie.ioaddr,
			data->ion_buf.cookie.paddr,
			data->ion_buf.size);
	if (ret < 0)
		dev_err(dev, "Failed to iommu_unmap: %d\n", ret);

	if (data->ion_buf.kva) {
		ion_unmap_kernel(data->ion_buf.client,
				data->ion_buf.handle);

		if (data->mmap_fd_state == true)
			dma_buf_put(data->ion_buf.dma_buf);
	}

	dma_buf_unmap_attachment(data->ion_buf.attachment,
			data->ion_buf.cookie.sgt,
			DMA_BIDIRECTIONAL);
	dma_buf_detach(data->ion_buf.dma_buf,
			data->ion_buf.attachment);
	dma_buf_put(data->ion_buf.dma_buf);

	return ret;
}
