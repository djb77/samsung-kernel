/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef VPU_QUEUE_H_
#define VPU_QUEUE_H_

#include <linux/types.h>
#include <linux/mutex.h>

#include "vision-buffer.h"
#include "vpu-framemgr.h"
#include "vpu-memory.h"

struct vpu_queue;

struct vpu_queue_ops {
	int (*start)(struct vpu_queue *queue);
	int (*stop)(struct vpu_queue *queue);
	int (*format)(struct vpu_queue *queue, struct vs4l_format_list *f);
	int (*queue)(struct vpu_queue *queue, struct vb_container_list *incl, struct vb_container_list *otcl);
	int (*deque)(struct vpu_queue *queue, struct vb_container_list *clist);
};

struct vpu_queue {
	struct vb_queue			inqueue;
	struct vb_queue			otqueue;
	const struct vpu_queue_ops	*qops;
};

int vpu_queue_open(struct vpu_queue *queue, struct vpu_memory *memory, struct mutex *lock);
int vpu_queue_s_format(struct vpu_queue *queue, struct vs4l_format_list *f);
int vpu_queue_start(struct vpu_queue *queue);
int vpu_queue_stop(struct vpu_queue *queue);
int vpu_queue_poll(struct vpu_queue *queue, struct file *file, poll_table *poll);
int vpu_queue_qbuf(struct vpu_queue *queue, struct vs4l_container_list *c);
int vpu_queue_dqbuf(struct vpu_queue *queue, struct vs4l_container_list *c, bool nonblocking);
void vpu_queue_done(struct vpu_queue *queue, struct vb_container_list *incl, struct vb_container_list *otcl, unsigned long flags);

#define CALL_QOPS(q, op, ...)	(((q)->qops->op) ? ((q)->qops->op(q, ##__VA_ARGS__)) : 0)

#endif
