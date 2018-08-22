/*
 * drivers/media/platform/exynos/mfc/s5p_mfc_mem.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __S5P_MFC_MEM_H
#define __S5P_MFC_MEM_H __FILE__

#include <media/videobuf2-ion.h>

#include "s5p_mfc_common.h"

/* Offset base used to differentiate between CAPTURE and OUTPUT
*  while mmaping */
#define DST_QUEUE_OFF_BASE      (TASK_SIZE / 2)

static inline dma_addr_t s5p_mfc_mem_get_daddr_vb(
	struct vb2_buffer *v, unsigned int n)
{
	void *cookie = vb2_plane_cookie(v, n);
	dma_addr_t addr = 0;

	WARN_ON(vb2_ion_dma_address(cookie, &addr) != 0);

	return addr;
}

static inline int s5p_mfc_mem_buf_prepare(struct vb2_buffer *vb)
{
	return vb2_ion_buf_prepare(vb);
}

static inline void s5p_mfc_mem_buf_finish(struct vb2_buffer *vb)
{
	vb2_ion_buf_finish(vb);
}

struct vb2_mem_ops *s5p_mfc_mem_ops(void);

void s5p_mfc_mem_set_cacheable(bool cacheable);
void s5p_mfc_mem_clean(struct s5p_mfc_dev *dev,
			struct s5p_mfc_special_buf *specail_buf,
			off_t offset, size_t size);
void s5p_mfc_mem_invalidate(struct s5p_mfc_dev *dev,
			struct s5p_mfc_special_buf *specail_buf,
			off_t offset, size_t size);
int s5p_mfc_mem_clean_vb(struct vb2_buffer *vb, u32 num_planes);
int s5p_mfc_mem_inv_vb(struct vb2_buffer *vb, u32 num_planes);

int s5p_mfc_mem_get_user_shared_handle(struct s5p_mfc_ctx *ctx,
		struct mfc_user_shared_handle *handle);
int s5p_mfc_mem_cleanup_user_shared_handle(struct s5p_mfc_ctx *ctx,
		struct mfc_user_shared_handle *handle);

void *s5p_mfc_mem_get_vaddr(struct s5p_mfc_dev *dev,
		struct s5p_mfc_special_buf *special_buf);
void s5p_mfc_mem_ion_free(struct s5p_mfc_dev *dev,
		struct s5p_mfc_special_buf *special_buf);
int s5p_mfc_mem_ion_alloc(struct s5p_mfc_dev *dev,
		struct s5p_mfc_special_buf *special_buf);

#endif /* __S5P_MFC_MEM_H */
