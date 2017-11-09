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

#include "score-debug.h"
#include "score-framemgr.h"
#include "score-vertex.h"
#include "score-utils.h"

static inline void score_frame_s_rdy(struct score_framemgr *framemgr,
		struct score_frame *frame)
{
	frame->state = SCORE_FRAME_STATE_READY;
	list_add_tail(&frame->state_list, &framemgr->rdy_list);
	framemgr->rdy_cnt++;
}

static inline void score_frame_s_pro(struct score_framemgr *framemgr,
		struct score_frame *frame)
{
	frame->state = SCORE_FRAME_STATE_PROCESS;
	list_add_tail(&frame->state_list, &framemgr->pro_list);
	framemgr->pro_cnt++;
}

static inline void score_frame_s_pend(struct score_framemgr *framemgr,
		struct score_frame *frame)
{
	frame->state = SCORE_FRAME_STATE_PENDING;
	list_add_tail(&frame->state_list, &framemgr->pend_list);
	framemgr->pend_cnt++;
}

static inline void score_frame_s_com(struct score_framemgr *framemgr,
		struct score_frame *frame)
{
	frame->state = SCORE_FRAME_STATE_COMPLETE;
	list_add_tail(&frame->state_list, &framemgr->com_list);
	framemgr->com_cnt++;
}

static int __score_frame_destroy_state(struct score_framemgr *framemgr,
		struct score_frame *frame)
{
	int ret = 0;

	switch (frame->state) {
	case SCORE_FRAME_STATE_READY:
		if (!framemgr->rdy_cnt) {
			ret = -EINVAL;
			goto p_err;
		}
		framemgr->rdy_cnt--;
		break;
	case SCORE_FRAME_STATE_PROCESS:
		if (!framemgr->pro_cnt) {
			ret = -EINVAL;
			goto p_err;
		}
		framemgr->pro_cnt--;
		break;
	case SCORE_FRAME_STATE_PENDING:
		if (!framemgr->pend_cnt) {
			ret = -EINVAL;
			goto p_err;
		}
		framemgr->pend_cnt--;
		break;
	case SCORE_FRAME_STATE_COMPLETE:
		if (!framemgr->com_cnt) {
			ret = -EINVAL;
			goto p_err;
		}
		framemgr->com_cnt--;
		break;
	default:
		ret = -EINVAL;
		goto p_err;
	}
	list_del(&frame->state_list);
p_err:
	return ret;
}

void score_frame_g_entire(struct score_framemgr *framemgr,
		int vid, int fid, struct score_frame **frame)
{
	struct score_frame *list_frame, *temp;

	list_for_each_entry_safe(list_frame, temp, &framemgr->entire_list, list) {
		if ((list_frame->fid == fid) && (list_frame->vid == vid)) {
			*frame = list_frame;
			return;
		}
	}
	*frame = NULL;
}

void score_frame_g_rdy(struct score_framemgr *framemgr,
		int vid, int fid, struct score_frame **frame)
{
	struct score_frame *list_frame, *temp;

	list_for_each_entry_safe(list_frame, temp, &framemgr->rdy_list, state_list) {
		if ((list_frame->fid == fid) && (list_frame->vid == vid)) {
			*frame = list_frame;
			return;
		}
	}
	*frame = NULL;
}

void score_frame_g_pro(struct score_framemgr *framemgr,
		int vid, int fid, struct score_frame **frame)
{
	struct score_frame *list_frame, *temp;

	list_for_each_entry_safe(list_frame, temp, &framemgr->pro_list, state_list) {
		if ((list_frame->fid == fid) && (list_frame->vid == vid)) {
			*frame = list_frame;
			return;
		}
	}
	*frame = NULL;
}

void score_frame_g_pend(struct score_framemgr *framemgr,
		int vid, int fid, struct score_frame **frame)
{
	struct score_frame *list_frame, *temp;

	list_for_each_entry_safe(list_frame, temp, &framemgr->pend_list, state_list) {
		if ((list_frame->fid == fid) && (list_frame->vid == vid)) {
			*frame = list_frame;
			return;
		}
	}
	*frame = NULL;
}

void score_frame_g_com(struct score_framemgr *framemgr,
		int vid, int fid, struct score_frame **frame)
{
	struct score_frame *list_frame, *temp;

	list_for_each_entry_safe(list_frame, temp, &framemgr->com_list, state_list) {
		if ((list_frame->fid == fid) && (list_frame->vid == vid)) {
			*frame = list_frame;
			return;
		}
	}
	*frame = NULL;
}

int score_frame_trans_rdy_to_pro(struct score_frame *frame)
{
	int ret = 0;
	struct score_framemgr *framemgr = frame->owner;

	if (frame->state != SCORE_FRAME_STATE_READY) {
		score_fwarn("not ready\n", frame);
		return -EINVAL;
	}

	if (!framemgr->rdy_cnt) {
		return -EINVAL;
	}

	list_del(&frame->state_list);
	framemgr->rdy_cnt--;
	score_frame_s_pro(framemgr, frame);
	frame->message = SCORE_FRAME_PROCESS;
	return ret;
}

int score_frame_trans_rdy_to_com(struct score_frame *frame)
{
	int ret = 0;
	struct score_framemgr *framemgr = frame->owner;

	if (frame->state != SCORE_FRAME_STATE_READY) {
		score_fwarn("not ready\n", frame);
		return -EINVAL;
	}

	if (!framemgr->rdy_cnt) {
		return -EINVAL;
	}

	list_del(&frame->state_list);
	framemgr->rdy_cnt--;
	score_frame_s_com(framemgr, frame);
	return ret;
}

int score_frame_trans_pro_to_rdy(struct score_frame *frame)
{
	int ret = 0;
	struct score_framemgr *framemgr = frame->owner;

	if (frame->state != SCORE_FRAME_STATE_PROCESS) {
		score_fwarn("not process\n", frame);
		return -EINVAL;
	}

	if (!framemgr->pro_cnt) {
		return -EINVAL;
	}

	list_del(&frame->state_list);
	framemgr->pro_cnt--;
	score_frame_s_rdy(framemgr, frame);
	frame->message = SCORE_FRAME_READY;
	return ret;
}

int score_frame_trans_pro_to_pend(struct score_frame *frame)
{
	int ret = 0;
	struct score_framemgr *framemgr = frame->owner;

	if (frame->state != SCORE_FRAME_STATE_PROCESS) {
		score_fwarn("not process\n", frame);
		return -EINVAL;
	}

	if (!framemgr->pro_cnt) {
		return -EINVAL;
	}

	list_del(&frame->state_list);
	framemgr->pro_cnt--;
	score_frame_s_pend(framemgr, frame);
	frame->message = SCORE_FRAME_PENDING;
	return ret;
}

int score_frame_trans_pro_to_com(struct score_frame *frame)
{
	int ret = 0;
	struct score_framemgr *framemgr = frame->owner;

	if (frame->state != SCORE_FRAME_STATE_PROCESS) {
		score_fwarn("not process\n", frame);
		return -EINVAL;
	}

	if (!framemgr->pro_cnt) {
		return -EINVAL;
	}

	list_del(&frame->state_list);
	framemgr->pro_cnt--;
	score_frame_s_com(framemgr, frame);
	return ret;
}

int score_frame_trans_pend_to_rdy(struct score_frame *frame)
{
	int ret = 0;
	struct score_framemgr *framemgr = frame->owner;

	if (frame->state != SCORE_FRAME_STATE_PENDING) {
		score_fwarn("not pending\n", frame);
		return -EINVAL;
	}

	if (!framemgr->pend_cnt) {
		return -EINVAL;
	}

	list_del(&frame->state_list);
	framemgr->pend_cnt--;
	score_frame_s_rdy(framemgr, frame);
	frame->message = SCORE_FRAME_READY;
	return ret;
}

int score_frame_trans_pend_to_com(struct score_frame *frame)
{
	int ret = 0;
	struct score_framemgr *framemgr = frame->owner;

	if (frame->state != SCORE_FRAME_STATE_PENDING) {
		score_fwarn("not pending\n", frame);
		return -EINVAL;
	}

	if (!framemgr->pend_cnt) {
		return -EINVAL;
	}

	list_del(&frame->state_list);
	framemgr->pend_cnt--;
	score_frame_s_com(framemgr, frame);
	return ret;
}

int score_frame_trans_any_to_com(struct score_frame *frame)
{
	int ret = 0;
	struct score_framemgr *framemgr = frame->owner;

	__score_frame_destroy_state(framemgr, frame);
	score_frame_s_com(framemgr, frame);
	return ret;
}

void score_frame_print_rdy_list(struct score_framemgr *framemgr)
{
	struct score_frame *frame, *temp;

	score_info("ready frame list(%d, %d) \n", framemgr->id, framemgr->rdy_cnt);
	list_for_each_entry_safe(frame, temp, &framemgr->rdy_list, state_list) {
		score_info("ready frame[%d,%d]\n", frame->fid, frame->vid);
	}
}

void score_frame_print_pro_list(struct score_framemgr *framemgr)
{
	struct score_frame *frame, *temp;

	score_info("process frame list(%d, %d) \n", framemgr->id, framemgr->pro_cnt);
	list_for_each_entry_safe(frame, temp, &framemgr->pro_list, state_list) {
		score_info("process frame[%d,%d]\n", frame->fid, frame->vid);
	}
}

void score_frame_print_pend_list(struct score_framemgr *framemgr)
{
	struct score_frame *frame, *temp;

	score_info("pending frame list(%d, %d) \n", framemgr->id, framemgr->pend_cnt);
	list_for_each_entry_safe(frame, temp, &framemgr->pend_list, state_list) {
		score_info("pending frame[%d,%d]\n", frame->fid, frame->vid);
	}
}

void score_frame_print_com_list(struct score_framemgr *framemgr)
{
	struct score_frame *frame, *temp;

	score_info("complete frame list(%d, %d) \n", framemgr->id, framemgr->com_cnt);
	list_for_each_entry_safe(frame, temp, &framemgr->com_list, state_list) {
		score_info("complete frame[%d,%d]\n", frame->fid, frame->vid);
	}
}

void score_frame_print_all(struct score_framemgr *framemgr)
{
	score_frame_print_rdy_list(framemgr);
	score_frame_print_pro_list(framemgr);
	score_frame_print_pend_list(framemgr);
	score_frame_print_com_list(framemgr);
}

struct score_frame *score_frame_create(struct score_framemgr *framemgr)
{
	unsigned long flags;
	struct score_frame *frame;

	frame = kmalloc(sizeof(struct score_frame), GFP_KERNEL);
	if (!frame) {
		return NULL;
	}

	frame->vid = framemgr->id;
	if (frame->vid != SCORE_MAX_VERTEX) {
		frame->fid = score_bitmap_g_zero_bit(framemgr->fid_map, SCORE_MAX_FRAME);
		if (frame->fid == SCORE_MAX_FRAME) {
			score_ferr("bitmap is full \n", frame);
			goto p_err;
		}
	}

	frame->owner = framemgr;
	frame->message = SCORE_FRAME_CREATE;
	init_kthread_work(&frame->work, framemgr->func);
	frame->frame = NULL;
	frame->buffer.m.userptr = 0;

	spin_lock_irqsave(&framemgr->slock, flags);
	list_add_tail(&frame->list, &framemgr->entire_list);
	framemgr->entire_cnt++;
	score_frame_s_rdy(framemgr, frame);
	spin_unlock_irqrestore(&framemgr->slock, flags);

	return frame;
p_err:
	kfree(frame);
	return NULL;
}

static int __score_frame_destroy(struct score_framemgr *framemgr, struct score_frame *frame)
{
	int ret = 0;
	unsigned long flags;
	struct score_frame *twin_frame = frame->frame;

	if (twin_frame) {
		flush_kthread_work(&twin_frame->work);
		twin_frame->frame = NULL;
	}
	flush_kthread_work(&frame->work);

	spin_lock_irqsave(&framemgr->slock, flags);
	__score_frame_destroy_state(framemgr, frame);

	list_del(&frame->list);
	framemgr->entire_cnt--;

	score_bitmap_clear_bit(framemgr->fid_map, frame->fid);

	if (frame->buffer.m.userptr) {
		kfree((void *)frame->buffer.m.userptr);
	}
	kfree(frame);
	spin_unlock_irqrestore(&framemgr->slock, flags);

	return ret;
}

int score_frame_destroy(struct score_frame *frame)
{
	struct score_framemgr *framemgr = frame->owner;
	return __score_frame_destroy(framemgr, frame);
}

void score_frame_pro_flush(struct score_framemgr *framemgr)
{
	struct score_frame *frame, *temp;

	if (framemgr->pro_cnt) {
		list_for_each_entry_safe(frame, temp, &framemgr->pro_list, state_list) {
			if (frame->message != SCORE_FRAME_FAIL &&
					frame->message != SCORE_FRAME_DONE &&
					frame->state == SCORE_FRAME_STATE_PROCESS) {
				score_fwarn("frame is flushed (%d) \n",
						frame, frame->message);
				frame->message = SCORE_FRAME_FAIL;
				score_frame_trans_pro_to_com(frame);
				if (frame->frame) {
					frame->frame->message = SCORE_FRAME_FAIL;
				}
			}
		}
	}
}

void score_frame_all_flush(struct score_framemgr *framemgr)
{
	struct score_frame *frame, *temp;

	if (framemgr->entire_cnt) {
		list_for_each_entry_safe(frame, temp, &framemgr->entire_list, list) {
			if (frame->message != SCORE_FRAME_FAIL &&
					frame->message != SCORE_FRAME_DONE) {
				score_fwarn("frame is flushed \n", frame);
				frame->message = SCORE_FRAME_FAIL;
				if (frame->frame) {
					frame->frame->message = SCORE_FRAME_FAIL;
					score_frame_trans_any_to_com(frame);
				}
			}
		}
	}
}

int score_framemgr_init(struct score_framemgr *framemgr)
{
	int ret = 0;

	mutex_init(&framemgr->lock);
	spin_lock_init(&framemgr->slock);
	init_waitqueue_head(&framemgr->done_wq);
	score_bitmap_init(framemgr->fid_map, SCORE_MAX_FRAME);

	INIT_LIST_HEAD(&framemgr->entire_list);
	INIT_LIST_HEAD(&framemgr->rdy_list);
	INIT_LIST_HEAD(&framemgr->pro_list);
	INIT_LIST_HEAD(&framemgr->pend_list);
	INIT_LIST_HEAD(&framemgr->com_list);

	framemgr->entire_cnt = 0;
	framemgr->rdy_cnt = 0;
	framemgr->pro_cnt = 0;
	framemgr->pend_cnt = 0;
	framemgr->com_cnt = 0;

	return ret;
}

void score_framemgr_deinit(struct score_framemgr *framemgr)
{
	struct score_frame *frame, *temp;

	if (framemgr->entire_cnt) {
		list_for_each_entry_safe(frame, temp, &framemgr->entire_list, list) {
			score_fwarn("destroy (count:%d) \n", frame, framemgr->entire_cnt);
			if (frame->frame) {
				__score_frame_destroy(framemgr, frame->frame);
			}
			__score_frame_destroy(framemgr, frame);
		}
	}
}
