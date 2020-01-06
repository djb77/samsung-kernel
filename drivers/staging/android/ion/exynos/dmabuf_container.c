/*
 * Copyright (C) 2017 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/dma-buf.h>
#include <linux/slab.h>
#include <linux/fdtable.h>

#include <linux/dmabuf_container.h>
#include "dmabuf_container_priv.h"
#include "../../uapi/exynos_ion_uapi.h"
#include "../ion.h"

static void dmabuf_container_put_dmabuf(struct dmabuf_container *bufcon)
{
	int i;

	for (i = 0; i < bufcon->bufcount; i++)
		dma_buf_put(bufcon->bufs[i]);
}

static int dmabuf_container_get_dmabuf(struct dmabuf_container *bufcon, int *fd)
{
	int i;

	for (i = 0; i < bufcon->bufcount; i++) {
		bufcon->bufs[i] = dma_buf_get(fd[i]);
		if (IS_ERR(bufcon->bufs[i])) {
			int ret = PTR_ERR(bufcon->bufs[i]);

			pr_err("%s: Failed to get dmabuf of fd %d (%u/%u)\n",
			       __func__, fd[i], i, bufcon->bufcount);

			while (i-- > 0)
				dma_buf_put(bufcon->bufs[i]);

			return ret;
		}
	}

	return 0;
}

static void dmabuf_container_dma_buf_release(struct dma_buf *dmabuf)
{
	struct dmabuf_container *bufcon = dmabuf->priv;

	dmabuf_container_put_dmabuf(bufcon);

	kfree(bufcon);
}

static struct sg_table *dmabuf_container_map_dma_buf(
				    struct dma_buf_attachment *attachment,
				    enum dma_data_direction direction)
{
	struct dma_buf *dmabuf = attachment->dmabuf;
	struct dmabuf_container *bufcon = dmabuf->priv;

	return &bufcon->table;
}

static void dmabuf_container_unmap_dma_buf(
				    struct dma_buf_attachment *attachment,
				    struct sg_table *table,
				    enum dma_data_direction direction)
{
}

static void *dmabuf_container_dma_buf_kmap(struct dma_buf *dmabuf,
					   unsigned long offset)
{
	return NULL;
}

static int dmabuf_container_mmap(struct dma_buf *dmabuf,
				 struct vm_area_struct *vma)
{
	pr_err("%s: dmabuf container does not support mmap\n", __func__);

	return -EACCES;
}

static struct dma_buf_ops dma_buf_ops = {
	.map_dma_buf = dmabuf_container_map_dma_buf,
	.unmap_dma_buf = dmabuf_container_unmap_dma_buf,
	.release = dmabuf_container_dma_buf_release,
	.kmap_atomic = dmabuf_container_dma_buf_kmap,
	.kmap = dmabuf_container_dma_buf_kmap,
	.mmap = dmabuf_container_mmap,
};

bool is_dmabuf_container(struct dma_buf *dmabuf)
{
	return dmabuf && (dmabuf->ops == &dma_buf_ops);
}

static struct dmabuf_container *bufcon_get_container(struct dma_buf *dmabuf)
{
	if (!dmabuf || dmabuf->ops != &dma_buf_ops)
		return ERR_PTR(-EINVAL);

	return dmabuf->priv;
}

static struct dma_buf *dmabuf_container_export(struct dmabuf_container *bufcon)
{
	struct dma_buf *dmabuf;
	DEFINE_DMA_BUF_EXPORT_INFO(exp_info);
	unsigned long size = 0;
	int i;

	for (i = 0; i < bufcon->bufcount; i++)
		size += bufcon->bufs[i]->size;

	exp_info.ops = &dma_buf_ops;
	exp_info.size = size;
	exp_info.flags = O_RDWR;
	exp_info.priv = bufcon;

	dmabuf = dma_buf_export(&exp_info);

	return dmabuf;
}

int dmabuf_container_get_count(struct dma_buf *dmabuf)
{
	struct dmabuf_container *bufcon = bufcon_get_container(dmabuf);

	if (IS_ERR(bufcon))
		return 0;

	return bufcon->bufcount;
}

struct dma_buf *dmabuf_container_get_buffer(struct dma_buf *dmabuf, int index)
{
	struct dmabuf_container *bufcon = bufcon_get_container(dmabuf);

	if (IS_ERR(bufcon))
		return ERR_CAST(bufcon);

	if (index >= bufcon->bufcount) {
		pr_err("%s: invalid index %d given but has %d buffers\n",
		       __func__, index, bufcon->bufcount);
		return ERR_PTR(-EINVAL);
	}

	get_dma_buf(bufcon->bufs[index]);

	return bufcon->bufs[index];
}

int dmabuf_container_create(void __user *arg)
{
	struct dmabuf_container_data data;
	struct dmabuf_container *bufcon;
	struct dma_buf *dmabuf;
	int ret = 0;

	if (copy_from_user(&data, arg, sizeof(data))) {
		pr_err("%s: Failed to read dmabuf_container_data\n", __func__);
		return -EFAULT;
	}

	if (!data.count || data.count > MAX_BUFCON_BUFS) {
		pr_err("%s: Too many buffer count %d\n", __func__, data.count);
		return -EPERM;
	}

	bufcon = kzalloc(sizeof(*bufcon) + sizeof(bufcon->bufs[0]) * data.count,
			 GFP_KERNEL);
	if (!bufcon)
		return -ENOMEM;

	bufcon->bufcount = data.count;

	ret = dmabuf_container_get_dmabuf(bufcon, data.fds);
	if (ret < 0)
		goto err_dmabuf;

	dmabuf = dmabuf_container_export(bufcon);
	if (IS_ERR(dmabuf)) {
		pr_err("%s: Failed to export dmabuf\n", __func__);
		ret = PTR_ERR(dmabuf);
		goto err_export;
	}

	data.container_fd = dma_buf_fd(dmabuf, O_CLOEXEC);
	if (data.container_fd < 0) {
		pr_err("%s: Failed to get fd for dmabuf\n", __func__);
		ret = data.container_fd;
		goto err_fd;
	}

	if (copy_to_user(arg, &data, sizeof(data))) {
		pr_err("%s: Failed to copy to user\n", __func__);
		ret = -EFAULT;
		goto err_put;
	}

	return 0;
err_put:
	__close_fd(current->files, data.container_fd);
	return ret;
err_fd:
	dma_buf_put(dmabuf);
	return ret;
err_export:
	dmabuf_container_put_dmabuf(bufcon);
err_dmabuf:
	kfree(bufcon);

	return ret;
}
