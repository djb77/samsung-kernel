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

#ifndef SCORE_QUEUE_H_
#define SCORE_QUEUE_H_

#include <linux/types.h>
#include <linux/mutex.h>

#include "vision-buffer.h"
#include "score-framemgr.h"
#include "score-memory.h"

struct score_queue;

struct score_queue_ops {
	int (*start)(struct score_queue *queue);
	int (*stop)(struct score_queue *queue);
	int (*format)(struct score_queue *queue, struct vs4l_format_list *f);
	int (*queue)(struct score_queue *queue, struct vb_container_list *clist);
	int (*deque)(struct score_queue *queue, struct vb_container_list *clist);
};

struct score_queue {
	struct vb_queue			queue;
	const struct score_queue_ops	*qops;
};

int score_queue_open(struct score_queue *queue, struct score_memory *memory, struct mutex *lock);
int score_queue_s_format(struct score_queue *queue, struct vs4l_format_list *f);
int score_queue_start(struct score_queue *queue);
int score_queue_stop(struct score_queue *queue);
int score_queue_poll(struct score_queue *queue, struct file *file, poll_table *poll);
int score_queue_qbuf(struct score_queue *queue, struct vs4l_container_list *c);
int score_queue_dqbuf(struct score_queue *queue, struct vs4l_container_list *c, bool nonblocking);
void score_queue_done(struct score_queue *queue, struct vb_container_list *clist, unsigned long flags);

#define CALL_QOPS(q, op, ...)	(((q)->qops->op) ? ((q)->qops->op((q), ##__VA_ARGS__)) : 0)

#endif
