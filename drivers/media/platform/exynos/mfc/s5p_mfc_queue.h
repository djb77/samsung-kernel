/*
 * drivers/media/platform/exynos/mfc/s5p_mfc_queue.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __S5P_MFC_QUEUE_H
#define __S5P_MFC_QUEUE_H __FILE__

#include  "s5p_mfc_common.h"

/**
 * enum s5p_mfc_queue_used_type
 */
enum s5p_mfc_queue_used_type {
	MFC_BUF_NO_TOUCH_USED	= -1,
	MFC_BUF_RESET_USED	= 0,
	MFC_BUF_SET_USED	= 1,
};

/**
 * enum s5p_mfc_queue_top_type
 */
enum s5p_mfc_queue_top_type {
	MFC_QUEUE_ADD_BOTTOM	= 0,
	MFC_QUEUE_ADD_TOP	= 1,
};

static inline unsigned int s5p_mfc_get_queue_count(spinlock_t *plock, struct s5p_mfc_buf_queue *queue)
{
	unsigned long flags;
	unsigned int ret = 0;

	spin_lock_irqsave(plock, flags);
	ret = queue->count;
	spin_unlock_irqrestore(plock, flags);

	return ret;
}

static inline int s5p_mfc_is_queue_count_same(spinlock_t *plock, struct s5p_mfc_buf_queue *queue,
		unsigned int value)
{
	unsigned long flags;
	int ret = 0;

	spin_lock_irqsave(plock, flags);
	if (queue->count == value)
		ret = 1;
	spin_unlock_irqrestore(plock, flags);

	return ret;
}

static inline int s5p_mfc_is_queue_count_greater(spinlock_t *plock, struct s5p_mfc_buf_queue *queue,
		unsigned int value)
{
	unsigned long flags;
	int ret = 0;

	spin_lock_irqsave(plock, flags);
	if (queue->count > value)
		ret = 1;
	spin_unlock_irqrestore(plock, flags);

	return ret;
}

static inline int s5p_mfc_is_queue_count_smaller(spinlock_t *plock, struct s5p_mfc_buf_queue *queue,
		unsigned int value)
{
	unsigned long flags;
	int ret = 0;

	spin_lock_irqsave(plock, flags);
	if (queue->count < value)
		ret = 1;
	spin_unlock_irqrestore(plock, flags);

	return ret;
}

static inline void s5p_mfc_init_queue(struct s5p_mfc_buf_queue *queue)
{
	INIT_LIST_HEAD(&queue->head);
	queue->count = 0;
}

static inline void s5p_mfc_create_queue(struct s5p_mfc_buf_queue *queue)
{
	s5p_mfc_init_queue(queue);
}

static inline void s5p_mfc_delete_queue(struct s5p_mfc_buf_queue *queue)
{
	s5p_mfc_init_queue(queue);
}

void s5p_mfc_add_tail_buf(spinlock_t *plock, struct s5p_mfc_buf_queue *queue,
		struct s5p_mfc_buf *mfc_buf);

int s5p_mfc_peek_buf_csd(spinlock_t *plock, struct s5p_mfc_buf_queue *queue);

struct s5p_mfc_buf *s5p_mfc_get_buf(spinlock_t *plock, struct s5p_mfc_buf_queue *queue,
		enum s5p_mfc_queue_used_type used);
struct s5p_mfc_buf *s5p_mfc_get_del_buf(spinlock_t *plock, struct s5p_mfc_buf_queue *queue,
		enum s5p_mfc_queue_used_type used);
struct s5p_mfc_buf *s5p_mfc_get_del_if_consumed(spinlock_t *plock, struct s5p_mfc_buf_queue *queue,
		unsigned long consumed, unsigned int min_bytes, int err, int *deleted);
struct s5p_mfc_buf *s5p_mfc_get_move_buf(spinlock_t *plock,
		struct s5p_mfc_buf_queue *to_queue, struct s5p_mfc_buf_queue *from_queue,
		enum s5p_mfc_queue_used_type used, enum s5p_mfc_queue_top_type top);
struct s5p_mfc_buf *s5p_mfc_get_move_buf_used(spinlock_t *plock,
		struct s5p_mfc_buf_queue *to_queue, struct s5p_mfc_buf_queue *from_queue);
struct s5p_mfc_buf *s5p_mfc_get_move_buf_addr(spinlock_t *plock,
		struct s5p_mfc_buf_queue *to_queue, struct s5p_mfc_buf_queue *from_queue,
		dma_addr_t addr);

struct s5p_mfc_buf *s5p_mfc_find_buf_vb(spinlock_t *plock, struct s5p_mfc_buf_queue *queue,
		dma_addr_t addr);
struct s5p_mfc_buf *s5p_mfc_find_del_buf_raw(spinlock_t *plock, struct s5p_mfc_buf_queue *queue,
		dma_addr_t addr);
struct s5p_mfc_buf *s5p_mfc_find_del_buf_vb(spinlock_t *plock, struct s5p_mfc_buf_queue *queue,
		dma_addr_t addr);
struct s5p_mfc_buf *s5p_mfc_find_move_buf_vb(spinlock_t *plock,
		struct s5p_mfc_buf_queue *to_queue, struct s5p_mfc_buf_queue *from_queue,
		dma_addr_t addr, unsigned int released_flag);
struct s5p_mfc_buf *s5p_mfc_find_move_buf_vb_used(spinlock_t *plock,
		struct s5p_mfc_buf_queue *to_queue, struct s5p_mfc_buf_queue *from_queue,
		dma_addr_t addr);

void s5p_mfc_move_first_buf_used(spinlock_t *plock, struct s5p_mfc_buf_queue *to_queue,
		struct s5p_mfc_buf_queue *from_queue, enum s5p_mfc_queue_top_type top);
void s5p_mfc_move_all_bufs(spinlock_t *plock, struct s5p_mfc_buf_queue *to_queue,
		struct s5p_mfc_buf_queue *from_queue, enum s5p_mfc_queue_top_type top);

void s5p_mfc_cleanup_queue(spinlock_t *plock, struct s5p_mfc_buf_queue *queue);

void s5p_mfc_handle_released_info(struct s5p_mfc_ctx *ctx,
		unsigned int released_flag, int index);

struct s5p_mfc_buf *s5p_mfc_move_reuse_buffer(struct s5p_mfc_ctx *ctx, int release_index);

void s5p_mfc_cleanup_enc_src_queue(struct s5p_mfc_ctx *ctx);
void s5p_mfc_cleanup_enc_dst_queue(struct s5p_mfc_ctx *ctx);

struct s5p_mfc_buf *s5p_mfc_search_for_dpb(struct s5p_mfc_ctx *ctx, unsigned int dynamic_used);
struct s5p_mfc_buf *s5p_mfc_search_move_dpb_nal_q(struct s5p_mfc_ctx *ctx, unsigned int dynamic_used);
void s5p_mfc_store_dpb(struct s5p_mfc_ctx *ctx, struct vb2_buffer *vb);

void s5p_mfc_cleanup_nal_queue(struct s5p_mfc_ctx *ctx);
int s5p_mfc_is_last_frame(struct s5p_mfc_ctx *ctx);

#endif /* __S5P_MFC_QUEUE_H */
