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

#include <linux/slab.h>
#include "score-queue.h"
#include "score-framemgr.h"
#include "score-debug.h"

extern const struct vb_ops vb_score_ops;
extern const struct score_queue_ops score_queue_ops;

int score_queue_open(struct score_queue *queue,
	struct score_memory *memory,
	struct mutex *lock)
{
	int ret = 0;
	struct vb_queue *q;

	queue->qops = &score_queue_ops;

	q = &queue->queue;
	q->private_data = queue;

	ret = vb_queue_init(q, memory->alloc_ctx, memory->vb2_mem_ops, &vb_score_ops, lock, 0);
	if (ret) {
		score_err("vb_queue_init is fail(%d)\n", ret);
		goto p_err;
	}

p_err:
	return ret;
}

int score_queue_s_format(struct score_queue *queue, struct vs4l_format_list *f)
{
	int ret = 0;
	struct vb_queue *q;

	BUG_ON(!queue);
	BUG_ON(!f);

	q = &queue->queue;

	ret = CALL_QOPS(queue, format, f);
	if (ret) {
		score_err("CALL_QOPS(format) is fail(%d)\n", ret);
		goto p_err;
	}

	ret = vb_queue_s_format(q, f);
	if (ret) {
		score_err("vb_queue_s_format is fail(%d)\n", ret);
		goto p_err;
	}

p_err:
	return ret;
}

int score_queue_start(struct score_queue *queue)
{
	int ret = 0;
	struct vb_queue *q;

	q = &queue->queue;

	ret = vb_queue_start(q);
	if (ret) {
		score_err("vb_queue_start is fail(%d)\n", ret);
		goto p_err;
	}

	ret = CALL_QOPS(queue, start);
	if (ret) {
		score_err("CALL_QOPS(start) is fail(%d)\n", ret);
		goto p_err;
	}

p_err:
	return ret;
}

int score_queue_stop(struct score_queue *queue)
{
	int ret = 0;
	struct vb_queue *q;

	q = &queue->queue;

	ret = CALL_QOPS(queue, stop);
	if (ret) {
		score_err("CALL_QOPS(stop) is fail(%d)\n", ret);
		goto p_err;
	}

	ret = vb_queue_stop(q);
	if (ret) {
		score_err("vb_queue_stop is fail(%d)\n", ret);
		goto p_err;
	}

p_err:
	return ret;
}

int score_queue_poll(struct score_queue *queue, struct file *file, poll_table *poll)
{
	int ret = 0;
	struct vb_queue *q;
	unsigned long events;

	BUG_ON(!queue);
	BUG_ON(!file);
	BUG_ON(!poll);

	events = poll_requested_events(poll);

	q = &queue->queue;

	if (events & POLLIN) {
		if (list_empty(&q->done_list))
			poll_wait(file, &q->done_wq, poll);
	}

	if (events & POLLOUT) {
		if (list_empty(&q->done_list))
			poll_wait(file, &q->done_wq, poll);
	}

	return ret;
}

static int __score_buf_prepare(struct vb_queue *q, struct vb_bundle *bundle)
{
	int ret = 0;
	u32 i, j, k;
	struct vb_format *format;
	struct vb_container *container;
	struct vb_buffer *buffer;

	BUG_ON(!q);
	BUG_ON(!bundle);

	/* if (test_bit(VS4L_CL_FLAG_PREPARE, &bundle->flags)) */
	/*	goto p_err; */

	for (i = 0; i < bundle->clist.count; ++i) {
		container = &bundle->clist.containers[i];
		format = container->format;

		switch (container->type) {
		case VS4L_BUFFER_LIST:
			k = container->count;
			break;
		case VS4L_BUFFER_ROI:
			k = container->count;
			break;
		case VS4L_BUFFER_PYRAMID:
			k = container->count;
			break;
		default:
			score_err("unsupported container type\n");
			goto p_err;
		}

		switch (container->memory) {
		case VS4L_MEMORY_DMABUF:
			break;
		case VS4L_MEMORY_VIRTPTR:
			for (j = 0; j < k; ++j) {
				buffer = &container->buffers[j];
				score_note("(%d %d %d %d) (%d) 0x%lx \n",
					buffer->roi.x, buffer->roi.y,
					buffer->roi.w, buffer->roi.h,
					format->size[format->plane],
					buffer->m.userptr);
			}
			break;
		default:
			score_err("unsupported container memory type\n");
			goto p_err;
		}
	}

p_err:
	return ret;
}

static int __score_buf_unprepare(struct vb_queue *q, struct vb_bundle *bundle)
{
	int ret = 0;
	u32 i, j, k;
	struct vb_format *format;
	struct vb_container *container;
	struct vb_buffer *buffer;
	void *mbuf = NULL;

	BUG_ON(!q);
	BUG_ON(!bundle);

	/* if (!test_bit(VS4L_CL_FLAG_PREPARE, &bundle->flags)) */
	/*	goto p_err; */

	for (i = 0; i < bundle->clist.count; ++i) {
		container = &bundle->clist.containers[i];
		format = container->format;

		switch (container->type) {
		case VS4L_BUFFER_LIST:
			k = container->count;
			break;
		case VS4L_BUFFER_ROI:
			k = container->count;
			break;
		case VS4L_BUFFER_PYRAMID:
			k = container->count;
			break;
		default:
			score_err("unsupported container type\n");
			goto p_err;
		}

		switch (container->memory) {
		case VS4L_MEMORY_DMABUF:
			break;
		case VS4L_MEMORY_VIRTPTR:
			for (j = 0; j < k; ++j) {
				buffer = &container->buffers[j];
				score_note("(%d %d %d %d) 0x%lx \n",
					buffer->roi.x, buffer->roi.y,
					buffer->roi.w, buffer->roi.h,
					buffer->m.userptr);

				ret = copy_to_user((void *)buffer->reserved,
						(void *)buffer->m.userptr,
						format->size[format->plane]);
				if (ret) {
					score_err("copy_to_user() is fail(%d)\n", ret);
					goto p_err;
				}

				/* switch kernel space to user space */
				if (buffer->reserved != 0) {
					mbuf = (void *)buffer->m.userptr;
					kfree(mbuf);

					mbuf = NULL;
					buffer->m.userptr = buffer->reserved;;
					buffer->reserved = 0;
				}

				score_note("(%d %d %d %d) 0x%lx \n",
					buffer->roi.x, buffer->roi.y,
					buffer->roi.w, buffer->roi.h,
					buffer->m.userptr);

			}
			break;
		default:
			score_err("unsupported container memory type\n");
			goto p_err;
		}
	}

p_err:
	return ret;
}

int score_queue_qbuf(struct score_queue *queue, struct vs4l_container_list *c)
{
	int ret = 0;
	struct vb_queue *q;
	struct vb_bundle *bundle;
	struct vb_bundle *bundle_map;

	q = &queue->queue;

	ret = vb_queue_qbuf(q, c);
	if (ret) {
		score_err("vb_queue_qbuf is fail(%d)\n", ret);
		goto p_err;
	}

	bundle_map = q->bufs[c->index];
	ret = __score_buf_prepare(q, bundle_map);
	if (ret) {
		score_err("__score_buf_prepare is fail(%d)\n", ret);
		goto p_err;
	}

	if (list_empty(&q->queued_list))
		goto p_err;

	bundle = list_first_entry(&q->queued_list, struct vb_bundle, queued_entry);

	vb_queue_process(q, bundle);

	ret = CALL_QOPS(queue, queue, &bundle->clist);
	if (ret) {
		score_err("CALL_QOPS(queue) is fail(%d)\n", ret);
		goto p_err;
	}

p_err:
	return ret;
}

int score_queue_dqbuf(struct score_queue *queue, struct vs4l_container_list *c, bool nonblocking)
{
	int ret = 0;
	struct vb_queue *q;
	struct vb_bundle *bundle;
	struct vb_bundle *bundle_map;

	BUG_ON(!queue);

	q = &queue->queue;

	ret = vb_queue_dqbuf(q, c, nonblocking);
	if (ret) {
		score_err("vb_queue_dqbuf is fail(%d)\n", ret);
		goto p_err;
	}

	if (c->index >= SCORE_MAX_BUFFER) {
		score_err("container index(%d) is invalid\n", c->index);
		ret = -EINVAL;
		goto p_err;
	}

	bundle = q->bufs[c->index];
	if (!bundle) {
		score_err("bundle(%d) is NULL\n", c->index);
		ret = -EINVAL;
		goto p_err;
	}

	if (bundle->clist.index != c->index) {
		score_err("index is NOT matched(%d != %d)\n", bundle->clist.index, c->index);
		ret = -EINVAL;
		goto p_err;
	}

	bundle_map = q->bufs[c->index];
	ret = __score_buf_unprepare(q, bundle_map);
	if (ret) {
		score_err("__score_buf_unprepare is fail(%d)\n", ret);
		goto p_err;
	}

	ret = CALL_QOPS(queue, deque, &bundle->clist);
	if (ret) {
		score_err("CALL_QOPS(deque) is fail(%d)\n", ret);
		goto p_err;
	}

p_err:
	return ret;
}

void score_queue_done(struct score_queue *queue,
	struct vb_container_list *clist,
	unsigned long flags)
{
	struct vb_queue *q;
	struct vb_bundle *bundle;

	BUG_ON(!queue);
	BUG_ON(!clist);

	q = &queue->queue;

	if (list_empty(&q->process_list)) {
		score_err("vb_queue is empty\n");
		BUG();
	}

	bundle = container_of(clist, struct vb_bundle, clist);

	if (bundle->state != VB_BUF_STATE_PROCESS) {
		score_err("bundle state(%d) is invalid\n", bundle->state);
		BUG();
	}

	bundle->flags |= flags;
	vb_queue_done(q, bundle);
}
