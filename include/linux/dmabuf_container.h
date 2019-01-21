/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __DMABUF_CONTAINER_H__
#define __DMABUF_CONTAINER_H__

#include <linux/kernel.h>
#include <linux/clk.h>
#include <linux/device.h>
#include <linux/miscdevice.h>

#ifdef CONFIG_ION_EXYNOS
int dmabuf_container_get_count(struct dma_buf *dmabuf);
struct dma_buf *dmabuf_container_get_buffer(struct dma_buf *dmabuf, int index);
#else
int dmabuf_container_get_count(struct dma_buf *dmabuf)
{
	return 0;
}
struct dma_buf *dmabuf_container_get_buffer(struct dma_buf *dmabuf, int index)
{
	return NULL;
}
#endif
#endif
