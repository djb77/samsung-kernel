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

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/sched/rt.h>
#include <linux/bug.h>
#include <linux/slab.h>

#include "score-framemgr.h"
#include "score-device.h"
#include "score-vertexmgr.h"
#include "score-debug.h"
#include "score-utils.h"
#include "score-time.h"

static int __score_vertexmgr_buf_convert(struct score_frame *frame)
{
	int ret = 0;
	struct score_framemgr *framemgr;
	struct score_vertex_ctx *vctx;
	struct score_buftracker *buftracker;

	struct vb_buffer *buffer = NULL;
	struct score_ipc_packet *packet = NULL;

	unsigned int loop, bitmap_site;

	framemgr = frame->owner;
	vctx = framemgr->cookie;
	buftracker = &vctx->buftracker;

	buffer = &frame->buffer;
	packet = (struct score_ipc_packet *)buffer->m.userptr;

	packet->header.vctx_id = frame->vid;
	packet->header.task_id = frame->fid;

	for (loop = 0; loop < packet->size.group_count; loop ++) {
		struct score_packet_group *group = NULL;
		struct score_packet_group_header *grp_header = NULL;
		struct score_packet_group_data *grp_data = NULL;

		group = &packet->group[loop];
		grp_header = &group->header;
		grp_data = &group->data;

		for (bitmap_site = 0; bitmap_site < 26; bitmap_site++) {
			if (grp_header->fd_bitmap & (0x1 << bitmap_site)) {
				struct score_memory_buffer *dev_buf = NULL;
				struct sc_packet_buffer *fw_buf = NULL;

				dev_buf = kzalloc(sizeof(struct score_memory_buffer), GFP_KERNEL);
				dev_buf->fid = frame->fid;

				fw_buf = (struct sc_packet_buffer *)(((int *)grp_data) + bitmap_site);
				dev_buf->memory_type = fw_buf->host_buf.memory_type;

				switch (dev_buf->memory_type) {
				case VS4L_MEMORY_DMABUF:
					dev_buf->m.fd = fw_buf->host_buf.m.fd;
					dev_buf->size = fw_buf->host_buf.memory_size;

					ret = score_buftracker_add_dmabuf(buftracker, dev_buf);
					if (ret) {
						kfree(dev_buf);
						goto p_err;
					}

					fw_buf->buf.addr = (unsigned int)dev_buf->dvaddr +
						fw_buf->host_buf.offset;
					break;
				case VS4L_MEMORY_USERPTR:
					dev_buf->m.userptr = fw_buf->host_buf.m.userptr;
					dev_buf->size = (size_t)fw_buf->host_buf.memory_size;

					ret = score_buftracker_add_userptr(buftracker, dev_buf);
					if (ret) {
						kfree(dev_buf);
						goto p_err;
					}

					fw_buf->buf.addr = (unsigned int)dev_buf->dvaddr +
						fw_buf->host_buf.offset;

					score_memory_invalid_or_flush_userptr(dev_buf,
							SCORE_MEMORY_SYNC_FOR_DEVICE,
							DMA_TO_DEVICE);
					break;
				default:
					score_ferr("Invalid memory type (%d)\n", frame, ret);
					ret = -EINVAL;
					kfree(dev_buf);
					goto p_err;
				}
			}
		}
	}

	return ret;
p_err:
	return ret;
}

int score_vertexmgr_queue(struct score_vertexmgr *vertexmgr, struct score_frame *frame)
{
	int ret = 0;

	ret = __score_vertexmgr_buf_convert(frame);
	if (ret) {
		frame->message = SCORE_FRAME_FAIL;
	} else {
		frame->message = SCORE_FRAME_READY;
		queue_kthread_work(&vertexmgr->worker, &frame->work);
	}
	return ret;
}

static int __score_vertex_send_command(struct score_frame *frame)
{
	int ret;
	struct vb_buffer *buffer = NULL;

	struct score_framemgr *framemgr;
	struct score_vertex_ctx *vctx;
	struct score_device *device;
	struct score_vertexmgr *vertexmgr;
	struct score_fw_dev *score_fw_device;
	struct score_ipc_packet *request_packet = NULL;

	framemgr = frame->owner;
	vctx = framemgr->cookie;
	vertexmgr = vctx->vertexmgr;

	device = container_of(vertexmgr, struct score_device, vertexmgr);
	score_fw_device = &device->fw_dev;

	buffer = &frame->buffer;
	request_packet = (struct score_ipc_packet *)buffer->m.userptr;

	ret = score_fw_queue_put(score_fw_device->in_queue, request_packet);
#ifdef ENABLE_TARGET_TIME
	if (!ret) {
		frame->ts[0] = score_time_get();
#ifdef DBG_SCV_KERNEL_TIME
		printk("@@@SCORE [PERF][ID : %d][Start] %ld.%09ld second\n",
				request_packet->header.kernel_name,
				frame->ts[0].tv_sec, frame->ts[0].tv_nsec);
#endif
	}
#endif

	return ret;
}

static void score_vertex_thread(struct kthread_work *work)
{
	int ret;
	unsigned long flags;
	struct score_vertex_ctx *vctx;
	struct score_framemgr *framemgr;
	struct score_frame *frame;
	struct score_framemgr *iframemgr;
	struct score_frame *iframe;

	frame = container_of(work, struct score_frame, work);

	framemgr = frame->owner;
	vctx = framemgr->cookie;
	CALL_VOPS(vctx, get);

	switch (frame->message) {
	case SCORE_FRAME_READY:
		iframe = frame->frame;
		iframemgr = iframe->owner;

		spin_lock_irqsave(&iframemgr->slock, flags);

		if (iframe->state != SCORE_FRAME_STATE_READY) {
			if (frame->message != SCORE_FRAME_FAIL &&
					frame->message != SCORE_FRAME_DONE &&
					frame->message > 0) {
				frame->message = SCORE_FRAME_FAIL;
			}
			iframe->message = SCORE_FRAME_FAIL;
			score_frame_trans_any_to_com(iframe);
			spin_unlock_irqrestore(&iframemgr->slock, flags);
			break;
		}

		score_frame_trans_rdy_to_pro(iframe);

		ret = __score_vertex_send_command(frame);
		if (ret) {
			if (ret == -EBUSY) {
				score_frame_trans_pro_to_pend(iframe);
			} else {
				score_ferr("__score_vertex_send_command is fail (%d) \n",
						frame, ret);
				if (frame->message != SCORE_FRAME_FAIL &&
						frame->message > 0) {
					frame->message = SCORE_FRAME_FAIL;
				}
				iframe->message = SCORE_FRAME_FAIL;
				score_frame_trans_pro_to_com(iframe);
				sysfs_debug_count_add(ERROR_CNT_FW_ENQ);
			}
		}
		spin_unlock_irqrestore(&iframemgr->slock, flags);

		break;
	default:
		if (frame->message != SCORE_FRAME_FAIL &&
				frame->message != SCORE_FRAME_DONE &&
				frame->message > 0) {
			frame->message = SCORE_FRAME_FAIL;
		}
		iframe = frame->frame;
		if (iframe) {
			iframemgr = iframe->owner;
			spin_lock_irqsave(&iframemgr->slock, flags);
			iframe->message = SCORE_FRAME_FAIL;
			score_frame_trans_any_to_com(iframe);
			spin_unlock_irqrestore(&iframemgr->slock, flags);
		}

		wake_up(&framemgr->done_wq);
	}

	CALL_VOPS(vctx, put);
	return;
}

void score_vertexmgr_queue_pending_frame(struct score_vertexmgr *vertexmgr)
{
	unsigned long flags;

	struct score_interface *interface;
	struct score_framemgr *iframemgr;

	struct score_frame *iframe, *temp, *frame;
	unsigned int try = 0, max_try = 5;

	interface = vertexmgr->interface;
	iframemgr = &interface->framemgr;

	spin_lock_irqsave(&iframemgr->slock, flags);
	list_for_each_entry_safe(iframe, temp, &iframemgr->pend_list, state_list) {
		frame = iframe->frame;
		if (iframe->state == SCORE_FRAME_STATE_PENDING) {
			score_frame_trans_pend_to_rdy(iframe);
			queue_kthread_work(&vertexmgr->worker, &frame->work);
			if (try++ > max_try) {
				break;
			}
		}
	}
	spin_unlock_irqrestore(&iframemgr->slock, flags);
}

static void score_interface_thread(struct kthread_work *work)
{
	struct score_frame *iframe;
	struct score_framemgr *framemgr;
	struct score_frame *frame;

	struct score_vertex_ctx *vctx;
	struct score_buftracker *buftracker;

	iframe = container_of(work, struct score_frame, work);
	frame = iframe->frame;
	if (!frame) {
		iframe->message = SCORE_FRAME_FAIL;
		goto p_err;
	}

	framemgr = frame->owner;
	vctx = framemgr->cookie;
	CALL_VOPS(vctx, get);

	buftracker = &vctx->buftracker;

	switch (iframe->message) {
	case SCORE_FRAME_PROCESS:
		score_buftracker_invalid_or_flush_userptr_fid_all(buftracker,
				frame->fid, SCORE_MEMORY_SYNC_FOR_CPU,
				DMA_FROM_DEVICE);

		frame->message = SCORE_FRAME_DONE;
		break;
	default:
		if (frame->message != SCORE_FRAME_FAIL &&
				frame->message != SCORE_FRAME_DONE &&
				frame->message > 0) {
			frame->message = SCORE_FRAME_FAIL;
		}
		break;
	}

	wake_up(&framemgr->done_wq);
	CALL_VOPS(vctx, put);

p_err:
	return;
}

int score_vertexmgr_vctx_register(struct score_vertexmgr *vertexmgr,
		struct score_vertex_ctx *vctx)
{
	int ret = 0;
	struct score_framemgr *framemgr;

	vctx->vertexmgr = vertexmgr;
	vctx->id = score_bitmap_g_zero_bit(vertexmgr->vid_map, SCORE_MAX_VERTEX);
	if (vctx->id == SCORE_MAX_VERTEX) {
		score_err("score_vertex_ctx bitmap is full \n");
		return -ENOSPC;
	}

	framemgr = &vctx->framemgr;
	framemgr->cookie = vctx;
	framemgr->id = vctx->id;
	framemgr->func = score_vertex_thread;

	score_framemgr_init(framemgr);

	vctx->iframemgr = &vertexmgr->interface->framemgr;

	return ret;
}

int score_vertexmgr_vctx_unregister(struct score_vertexmgr *vertexmgr,
		struct score_vertex_ctx *vctx)
{
	int ret = 0;
	unsigned long flags;
	struct score_framemgr *framemgr;
	struct score_framemgr *iframemgr;

	framemgr = &vctx->framemgr;
	iframemgr = vctx->iframemgr;

	spin_lock_irqsave(&iframemgr->slock, flags);
	score_framemgr_deinit(framemgr);
	spin_unlock_irqrestore(&iframemgr->slock, flags);

	score_bitmap_clear_bit(vertexmgr->vid_map, vctx->id);

	return ret;
}

int score_vertexmgr_itf_register(struct score_vertexmgr *vertexmgr,
		struct score_interface *interface)
{
	int ret = 0;
	struct score_framemgr *framemgr;

	vertexmgr->interface = interface;
	interface->vertexmgr = vertexmgr;

	framemgr = &interface->framemgr;
	framemgr->cookie = interface;
	framemgr->id = SCORE_MAX_VERTEX;
	framemgr->func = score_interface_thread;

	score_framemgr_init(framemgr);

	return ret;
}

int score_vertexmgr_itf_unregister(struct score_vertexmgr *vertexmgr,
		struct score_interface *interface)
{
	int ret = 0;
	struct score_framemgr *framemgr;

	framemgr = &interface->framemgr;
	score_framemgr_deinit(framemgr);

	return ret;
}

int score_vertexmgr_open(struct score_vertexmgr *vertexmgr)
{
	int ret = 0;
	return ret;
}

int score_vertexmgr_close(struct score_vertexmgr *vertexmgr)
{
	int ret = 0;
	return ret;
}

int score_vertexmgr_probe(struct score_vertexmgr *vertexmgr)
{
	int ret = 0;
	char name[30];
	struct sched_param param = { .sched_priority = MAX_RT_PRIO - 1 };

	init_kthread_worker(&vertexmgr->worker);
	snprintf(name, sizeof(name), "score_vertex");
	vertexmgr->task_vertex =
		kthread_run(kthread_worker_fn, &vertexmgr->worker, name);
	if (IS_ERR_OR_NULL(vertexmgr->task_vertex)) {
		probe_err("kthread_run is fail\n");
		return -EINVAL;
	}

	ret = sched_setscheduler_nocheck(vertexmgr->task_vertex, SCHED_FIFO, &param);
	if (ret) {
		probe_err("sched_setscheduler_nocheck is fail(%d)\n", ret);
		goto p_err;
	}

	score_bitmap_init(vertexmgr->vid_map, SCORE_MAX_VERTEX);

	return ret;
p_err:
	kthread_stop(vertexmgr->task_vertex);
	return ret;
}

void score_vertexmgr_remove(struct score_vertexmgr *vertexmgr)
{
	kthread_stop(vertexmgr->task_vertex);
}
