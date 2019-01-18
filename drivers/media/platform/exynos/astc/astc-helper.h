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

int ftm_to_bpp(struct device *dev, struct hwASTC_img_info *img_info);

u32 blksize_enum_to_int(enum hwASTC_blk_size blk_size, int x_or_y);
u32 blksize_enum_to_x(enum hwASTC_blk_size blk_size);
u32 blksize_enum_to_y(enum hwASTC_blk_size blk_size);

static inline void astc_set_dma_address(
		struct astc_buffer_dma *buffer_dma,
		dma_addr_t dma_addr)
{
	buffer_dma->plane.dma_addr = dma_addr;
}

int astc_map_dma_attachment(struct device *dev,
		     struct astc_buffer_plane_dma *plane,
		     enum dma_data_direction dir);
void astc_unmap_dma_attachment(struct device *dev,
			struct astc_buffer_plane_dma *plane,
			enum dma_data_direction dir);


int astc_map_buf_to_device(struct astc_dev *astc_device,
		      struct astc_task *task,
		      struct astc_buffer_dma *buf,
		      enum dma_data_direction dir);
int astc_unmap_buf_from_device(struct device *dev,
			 struct astc_buffer_dma *buf);

void astc_sync_for_device(struct device *dev,
			  struct astc_buffer_plane_dma *plane,
			  enum dma_data_direction dir);

void astc_sync_for_cpu(struct device *dev,
		       struct astc_buffer_plane_dma *plane,
		       enum dma_data_direction dir);

#endif /* ASTC_HELPER_H_ */
