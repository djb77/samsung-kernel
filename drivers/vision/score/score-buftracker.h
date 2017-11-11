/*
 * Samsung Exynos SoC series SCORE driver
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef SCORE_BUFTRACKER_H_
#define SCORE_BUFTRACKER_H_

#include "vision-buffer.h"
#include "score-memory.h"

enum score_buftracker_search_state {
	SCORE_BUFTRACKER_SEARCH_STATE_NEW			= 1,
	SCORE_BUFTRACKER_SEARCH_STATE_INSERT			= 2,
	SCORE_BUFTRACKER_SEARCH_STATE_ALREADY_REGISTERED	= 3,
	SCORE_BUFTRACKER_SEARCH_STATE_END
};

struct score_buftracker {
	struct list_head		buf_list;
	u32				buf_cnt;
	spinlock_t			buf_lock;

	struct list_head		userptr_list;
	u32				userptr_cnt;
	spinlock_t			userptr_lock;
};

int score_buftracker_init(struct score_buftracker *buftracker);
int score_buftracker_deinit(struct score_buftracker *buftracker);

int score_buftracker_add_dmabuf(struct score_buftracker *buftracker,
		struct score_memory_buffer *buffer);
int score_buftracker_remove_dmabuf(struct score_buftracker *buftracker,
		struct score_memory_buffer *buffer);
int score_buftracker_remove_dmabuf_all(struct score_buftracker *buftracker);

int score_buftracker_add_userptr(struct score_buftracker *buftracker,
		struct score_memory_buffer *userptr_buffer);
int score_buftracker_remove_userptr(struct score_buftracker *buftracker,
		struct score_memory_buffer *buffer);
int score_buftracker_remove_userptr_all(struct score_buftracker *buftracker);

void score_buftracker_invalid_or_flush_userptr_fid_all(struct score_buftracker *buftracker,
		int fid, int sync_for, enum dma_data_direction dir);
void score_buftracker_invalid_or_flush_userptr_all(struct score_buftracker *buftracker,
		int sync_for, enum dma_data_direction dir);
void score_buftracker_dump(struct score_buftracker *buftracker);

#endif
