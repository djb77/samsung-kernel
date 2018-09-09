/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __DMABUF_CONTAINER_PRIV_H__
#define __DMABUF_CONTAINER_PRIV_H__

#include "../../uapi/exynos_ion_uapi.h"
/*
 * struct dmabuf_container - container description
 * @table:	dummy sg_table for container
 * @bufs:	dmabuf array representing each buffers
 * @bufcount:	the number of the buffers
 */

struct dmabuf_container {
	struct sg_table	table;
	int		bufcount;
	struct dma_buf	*bufs[0];
};

int dmabuf_container_create(void __user *arg);

#ifdef CONFIG_ION_EXYNOS
bool is_dmabuf_container(struct dma_buf *dmabuf);
#else
#define is_dmabuf_container(dmabuf) false
#endif

#endif
