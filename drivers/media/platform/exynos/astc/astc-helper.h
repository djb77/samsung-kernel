/* drivers/media/platform/exynos/astc/astc-helper.h
 *
 * Internal header for Samsung ASTC encoder driver
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

#ifndef ASTC_HELPER_H_
#define ASTC_HELPER_H_

#include "astc-core.h"

/**
 * astc_set_dma_address - set DMA address
 */
static inline void astc_set_dma_address(
		struct astc_buffer_dma *buffer_dma,
		dma_addr_t dma_addr)
{
	buffer_dma->plane.dma_addr = dma_addr;
}

int astc_map_dma_buf(struct device *dev,
		     struct astc_buffer_plane_dma *plane,
		     enum dma_data_direction dir);

void astc_unmap_dma_buf(struct device *dev,
			struct astc_buffer_plane_dma *plane,
			enum dma_data_direction dir);

int astc_dma_addr_map(struct device *dev,
		      struct astc_buffer_dma *buf,
		      enum dma_data_direction dir);

void astc_dma_addr_unmap(struct device *dev,
			 struct astc_buffer_dma *buf);

void astc_sync_for_device(struct device *dev,
			  struct astc_buffer_plane_dma *plane,
			  enum dma_data_direction dir);

void astc_sync_for_cpu(struct device *dev,
		       struct astc_buffer_plane_dma *plane,
		       enum dma_data_direction dir);

#endif /* ASTC_HELPER_H_ */
