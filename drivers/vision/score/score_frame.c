/*
 * Samsung Exynos SoC series SCore driver
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/types.h>
#include <linux/slab.h>

#include "score_log.h"
#include "score_util.h"
#include "score_context.h"
#include "score_frame.h"

/**
 * score_frame_add_buffer - Add list of memory buffer to list of frame
 * @frame:	[in]	object about score_frame structure
 * @buf:	[in]	object about score_memory_buffer structure
 */
void score_frame_add_buffer(struct score_frame *frame,
		struct score_mmu_buffer *buf)
{
	score_enter();
	list_add_tail(&buf->frame_list, &frame->buffer_list);
	frame->buffer_count++;
	score_leave();
}

/**
 * score_frame_remove_buffer - Remove list of memory buffer at list of frame
 * @frame:	[in]	object about score_frame structure
 * @buf:	[in]	object about score_memory_buffer structure
 */
void score_frame_remove_buffer(struct score_frame *frame,
		struct score_mmu_buffer *buf)
{
	score_enter();
	list_del(&buf->frame_list);
	frame->buffer_count--;
	score_leave();
}

/**
 * score_frame_check_done - Check that task is done of frame
 * @frame:	[in]	object about score_frame structure
 *
 * Returns 1 if task is done of frame, otherwise 0
 */
int score_frame_check_done(struct score_frame *frame)
{
	return frame->state == SCORE_FRAME_STATE_COMPLETE;
}

void score_frame_done(struct score_frame *frame, int *ret)
{
	*ret = frame->ret;
	if (frame->ret)
		frame->owner->abnormal_count++;
	else
		frame->owner->normal_count++;
}

static inline void __score_frame_set_ready(struct score_frame_manager *framemgr,
		struct score_frame *frame)
{
	score_enter();
	frame->state = SCORE_FRAME_STATE_READY;
	list_add_tail(&frame->state_list, &framemgr->ready_list);
	framemgr->ready_count++;
	score_leave();
}

static inline void __score_frame_set_process(
		struct score_frame_manager *framemgr,
		struct score_frame *frame)
{
	score_enter();
	frame->state = SCORE_FRAME_STATE_PROCESS;
	list_add_tail(&frame->state_list, &framemgr->process_list);
	framemgr->process_count++;
	score_leave();
}

static inline void __score_frame_set_pending(
		struct score_frame_manager *framemgr,
		struct score_frame *frame)
{
	score_enter();
	frame->state = SCORE_FRAME_STATE_PENDING;
	list_add_tail(&frame->state_list, &framemgr->pending_list);
	framemgr->pending_count++;
	score_leave();
}

static inline void __score_frame_set_complete(
		struct score_frame_manager *framemgr,
		struct score_frame *frame, int result)
{
	score_enter();
	frame->state = SCORE_FRAME_STATE_COMPLETE;
	list_add_tail(&frame->state_list, &framemgr->complete_list);
	framemgr->complete_count++;
	frame->ret = result;
	score_leave();
}

/**
 * score_frame_trans_ready_to_process -
 *	Translate state of frame from ready to process
 * @frame:      [in]	object about score_frame structure
 *
 * Return 0 if succeeded, otherwise errno
 */
int score_frame_trans_ready_to_process(struct score_frame *frame)
{
	int ret = 0;
	struct score_frame_manager *framemgr = frame->owner;

	score_enter();
	if (frame->state != SCORE_FRAME_STATE_READY) {
		score_warn("frame state is not ready(%d-%d)\n",
				frame->sctx->id, frame->frame_id);
		ret = -EINVAL;
		goto p_err;
	}

	if (!framemgr->ready_count) {
		score_warn("frame manager doesn't have ready frame(%d-%d)\n",
				frame->sctx->id, frame->frame_id);
		ret = -EINVAL;
		goto p_err;
	}

	list_del(&frame->state_list);
	framemgr->ready_count--;
	__score_frame_set_process(framemgr, frame);

	score_leave();
	return ret;
p_err:
	return ret;
}

/**
 * score_frame_trans_ready_to_complete -
 *	Translate state of frame from ready to complete
 * @frame:	[in]	object about score_frame structure
 * @result:	[in]	result of task included in frame
 *
 * Return 0 if succeeded, otherwise errno
 */
int score_frame_trans_ready_to_complete(struct score_frame *frame, int result)
{
	int ret = 0;
	struct score_frame_manager *framemgr = frame->owner;

	score_enter();
	if (frame->state != SCORE_FRAME_STATE_READY) {
		score_warn("frame state is not ready(%d-%d)\n",
				frame->sctx->id, frame->frame_id);
		ret = -EINVAL;
		goto p_err;
	}

	if (!framemgr->ready_count) {
		score_warn("frame manager doesn't have ready frame(%d-%d)\n",
				frame->sctx->id, frame->frame_id);
		ret = -EINVAL;
		goto p_err;
	}

	list_del(&frame->state_list);
	framemgr->ready_count--;
	__score_frame_set_complete(framemgr, frame, result);

	score_leave();
	return ret;
p_err:
	return ret;
}

/**
 * score_frame_trans_process_to_pending -
 *	Translate state of frame from process to pending
 * @frame:      [in]	object about score_frame structure
 *
 * Return 0 if succeeded, otherwise errno
 */
int score_frame_trans_process_to_pending(struct score_frame *frame)
{
	int ret = 0;
	struct score_frame_manager *framemgr = frame->owner;

	score_enter();
	if (frame->state != SCORE_FRAME_STATE_PROCESS) {
		score_warn("frame state is not process(%d-%d)\n",
				frame->sctx->id, frame->frame_id);
		ret = -EINVAL;
		goto p_err;
	}

	if (!framemgr->process_count) {
		score_warn("frame manager doesn't have process frame(%d-%d)\n",
				frame->sctx->id, frame->frame_id);
		ret = -EINVAL;
		goto p_err;
	}

	list_del(&frame->state_list);
	framemgr->process_count--;
	__score_frame_set_pending(framemgr, frame);

	score_leave();
	return ret;
p_err:
	return ret;
}

/**
 * score_frame_trans_process_to_complete -
 *	Translate state of frame from process to complete
 * @frame:	[in]	object about score_frame structure
 * @result:	[in]	result of task included in frame
 *
 * Return 0 if succeeded, otherwise errno
 */
int score_frame_trans_process_to_complete(struct score_frame *frame, int result)
{
	int ret = 0;
	struct score_frame_manager *framemgr = frame->owner;

	score_enter();
	if (frame->state != SCORE_FRAME_STATE_PROCESS) {
		score_warn("frame state is not process(%d-%d)\n",
				frame->sctx->id, frame->frame_id);
		ret = -EINVAL;
		goto p_err;
	}

	if (!framemgr->process_count) {
		score_warn("frame manager doesn't have process frame(%d-%d)\n",
				frame->sctx->id, frame->frame_id);
		ret = -EINVAL;
		goto p_err;
	}

	list_del(&frame->state_list);
	framemgr->process_count--;
	__score_frame_set_complete(framemgr, frame, result);

	score_leave();
	return ret;
p_err:
	return ret;
}

/**
 * score_frame_trans_pending_to_ready -
 *	Translate state of frame from pending to ready
 * @frame:      [in]	object about score_frame structure
 *
 * Return 0 if succeeded, otherwise errno
 */
int score_frame_trans_pending_to_ready(struct score_frame *frame)
{
	int ret = 0;
	struct score_frame_manager *framemgr = frame->owner;

	score_enter();
	if (frame->state != SCORE_FRAME_STATE_PENDING) {
		score_warn("frame state is not pending(%d-%d)\n",
				frame->sctx->id, frame->frame_id);
		ret = -EINVAL;
		goto p_err;
	}

	if (!framemgr->pending_count) {
		score_warn("frame manager doesn't have pending frame(%d-%d)\n",
				frame->sctx->id, frame->frame_id);
		ret = -EINVAL;
		goto p_err;
	}

	list_del(&frame->state_list);
	framemgr->pending_count--;
	__score_frame_set_ready(framemgr, frame);

	score_leave();
	return ret;
p_err:
	return ret;
}

/**
 * score_frame_trans_pending_to_complete -
 *	Translate state of frame from pending to complete
 * @frame:	[in]	object about score_frame structure
 * @result:	[in]	result of task included in frame
 *
 * Return 0 if succeeded, otherwise errno
 */
int score_frame_trans_pending_to_complete(struct score_frame *frame, int result)
{
	int ret = 0;
	struct score_frame_manager *framemgr = frame->owner;

	score_enter();
	if (frame->state != SCORE_FRAME_STATE_PENDING) {
		score_warn("frame state is not pending(%d-%d)\n",
				frame->sctx->id, frame->frame_id);
		ret = -EINVAL;
		goto p_err;
	}

	if (!framemgr->pending_count) {
		score_warn("frame manager doesn't have pending frame(%d-%d)\n",
				frame->sctx->id, frame->frame_id);
		ret = -EINVAL;
		goto p_err;
	}

	list_del(&frame->state_list);
	framemgr->pending_count--;
	__score_frame_set_complete(framemgr, frame, result);

	score_leave();
	return ret;
p_err:
	return ret;
}

/**
 * score_frame_get_process_by_id -
 *	Get frame that state is process and frame id is same with input id
 * @framemgr:	[in]	object about score_frame_manager structure
 * @id:		[in]	id value searched
 *
 * Returns frame address if succeeded, otherwise NULL
 */
struct score_frame *score_frame_get_process_by_id(
		struct score_frame_manager *framemgr, int id)
{
	struct score_frame *list_frame, *tframe;

	score_enter();
	list_for_each_entry_safe(list_frame, tframe, &framemgr->process_list,
			state_list) {
		if (list_frame->frame_id == id)
			return list_frame;
	}
	score_warn("frame manager doesn't have process frame that id is %d\n",
			id);
	score_leave();
	return NULL;
}

/**
 * score_frame_get_by_id -
 *	Get frame that frame id is same with input id
 * @framemgr:	[in]	object about score_frame_manager structure
 * @id:		[in]	id value searched
 *
 * Returns frame address if succeeded, otherwise NULL
 */
struct score_frame *score_frame_get_by_id(
		struct score_frame_manager *framemgr, int id)
{
	struct score_frame *list_frame, *tframe;

	score_enter();
	list_for_each_entry_safe(list_frame, tframe, &framemgr->entire_list,
			entire_list) {
		if (list_frame->frame_id == id)
			return list_frame;
	}
	score_leave();
	return NULL;
}

/**
 * score_frame_get_first_pending - Get first frame that state is pending
 * @framemgr:	[in]	object about score_frame_manager structure
 *
 * Returns frame address if succeeded, otherwise NULL
 */

struct score_frame *score_frame_get_first_pending(
		struct score_frame_manager *framemgr)
{
	struct score_frame *pendig_frame;

	if (framemgr->pending_count) {
		pendig_frame = list_first_entry(&framemgr->pending_list,
				struct score_frame, state_list);
		return pendig_frame;
	}
	score_warn("frame manager doesn't have pending frame\n");
	return NULL;
}

static int __score_frame_destroy_state(struct score_frame_manager *framemgr,
		struct score_frame *frame)
{
	int ret = 0;

	score_enter();
	switch (frame->state) {
	case SCORE_FRAME_STATE_READY:
		if (!framemgr->ready_count) {
			ret = -EINVAL;
			score_warn("count of ready frame is zero (%d-%d)\n",
					frame->sctx->id, frame->frame_id);
			goto p_err;
		}
		framemgr->ready_count--;
		break;
	case SCORE_FRAME_STATE_PROCESS:
		if (!framemgr->process_count) {
			ret = -EINVAL;
			score_warn("count of process frame is zero (%d-%d)\n",
					frame->sctx->id, frame->frame_id);
			goto p_err;
		}
		framemgr->process_count--;
		break;
	case SCORE_FRAME_STATE_PENDING:
		if (!framemgr->pending_count) {
			ret = -EINVAL;
			score_warn("count of pending frame is zero (%d-%d)\n",
					frame->sctx->id, frame->frame_id);
			goto p_err;
		}
		framemgr->pending_count--;
		break;
	case SCORE_FRAME_STATE_COMPLETE:
		if (!framemgr->complete_count) {
			ret = -EINVAL;
			score_warn("count of complete frame is zero (%d-%d)\n",
					frame->sctx->id, frame->frame_id);
			goto p_err;
		}
		framemgr->complete_count--;
		break;
	default:
		ret = -EINVAL;
		score_warn("frame state is invalid (%d-%d)\n",
				frame->sctx->id, frame->frame_id);
		goto p_err;
	}
	list_del(&frame->state_list);
	score_leave();
p_err:
	return ret;
}

/**
 * score_frame_trans_any_to_complete -
 *	Translate state of frame to complete regardless of state
 * @frame:	[in]	object about score_frame structure
 * @result:	[in]	result of task included in frame
 *
 * Return 0 if succeeded, otherwise errno
 */
int score_frame_trans_any_to_complete(struct score_frame *frame, int result)
{
	int ret = 0;
	struct score_frame_manager *framemgr = frame->owner;

	score_enter();
	if (frame->state != SCORE_FRAME_STATE_COMPLETE) {
		ret = __score_frame_destroy_state(framemgr, frame);
		__score_frame_set_complete(framemgr, frame, result);
	}

	score_leave();
	return ret;
}

/**
 * score_frame_get_state - Get state of frame
 * @frame:      [in]	object about score_frame structure
 *
 * Return state of frame
 */
unsigned int score_frame_get_state(struct score_frame *frame)
{
	return frame->state;
}

/**
 * score_frame_is_nonblock - Check nonblock of frame
 * @frame:      [in]	object about score_frame structure
 *
 * Return true if frame is nonblock mode
 */
bool score_frame_is_nonblock(struct score_frame *frame)
{
	return !frame->block;
}

/**
 * score_frame_set_block - Set frame to blocking
 * @frame:      [in]	object about score_frame structure
 */
void score_frame_set_block(struct score_frame *frame)
{
	frame->block = true;
}

/**
 * score_frame_get_pending_count - Get count of pending frame
 * @framemgr:	[in]	object about score_frame_manager structure
 *
 * Return count of pending frame
 */
unsigned int score_frame_get_pending_count(struct score_frame_manager *framemgr)
{
	return framemgr->pending_count;
}

/**
 * score_frame_flush_process - Transfer all process frame to complete
 * @framemgr:	[in]	object about score_frame_manager structure
 * @result:	[in]	result of task included in frame
 */
void score_frame_flush_process(struct score_frame_manager *framemgr, int result)
{
	struct score_frame *list_frame, *tframe;

	score_enter();
	list_for_each_entry_safe(list_frame, tframe, &framemgr->process_list,
			state_list)
		score_frame_trans_process_to_complete(list_frame, result);
	score_leave();
}

/**
 * score_frame_flush_all - Transfer all frame to complete
 * @framemgr:	[in]	object about score_frame_manager structure
 * @result:	[in]	result of task included in frame
 */
void score_frame_flush_all(struct score_frame_manager *framemgr, int result)
{
	struct score_frame *list_frame, *tframe;

	score_enter();
	list_for_each_entry_safe(list_frame, tframe, &framemgr->entire_list,
			entire_list)
		if (list_frame->state != SCORE_FRAME_STATE_COMPLETE)
			score_frame_trans_any_to_complete(list_frame, result);
	score_leave();
}

/**
 * score_frame_remove_nonblock_all -
 * @framemgr:	[in]	object about score_frame_manager structure
 */
void score_frame_remove_nonblock_all(struct score_frame_manager *framemgr)
{
	struct score_frame *list_frame, *tframe;

	score_enter();
	list_for_each_entry_safe(list_frame, tframe, &framemgr->entire_list,
			entire_list)
		if (score_frame_is_nonblock(list_frame))
			score_frame_destroy(list_frame);
	score_leave();
}

/**
 * score_frame_block - Block to prevent creating frame
 * @framemgr:	[in]	object about score_frame_manager structure
 */
void score_frame_block(struct score_frame_manager *framemgr)
{
	score_enter();
	framemgr->block = true;
	score_leave();
}

/**
 * score_frame_unblock - Clear the blocking of frame manager
 * @framemgr:	[in]	object about score_frame_manager structure
 */
void score_frame_unblock(struct score_frame_manager *framemgr)
{
	score_enter();
	framemgr->block = false;
	score_leave();
}

/**
 * score_frame_create - Create new frame and add that at frame manager
 * @framemgr:	[in]	object about score_frame_manager structure
 * @sctx:	[in]	pointer for context
 * @block:	[in]	whether task of this frame is blocking or non-blocking
 *
 * Returns frame address created if succeeded, otherwise NULL
 */
struct score_frame *score_frame_create(struct score_frame_manager *framemgr,
		struct score_context *sctx, bool block)
{
	unsigned long flags;
	struct score_frame *frame;

	score_enter();
	frame = kzalloc(sizeof(struct score_frame), GFP_KERNEL);
	if (!frame)
		return NULL;

	frame->sctx = sctx;
	frame->frame_id = score_util_bitmap_get_zero_bit(framemgr->frame_map,
			SCORE_MAX_FRAME);
	if (frame->frame_id == SCORE_MAX_FRAME) {
		score_err("frame bitmap is full [sctx:%d]\n", sctx->id);
		goto p_err;
	}
	frame->block = block;

	frame->owner = framemgr;

	spin_lock_irqsave(&framemgr->slock, flags);
	if (framemgr->block) {
		spin_unlock_irqrestore(&framemgr->slock, flags);
		goto p_block;
	}

	list_add_tail(&frame->list, &sctx->frame_list);
	sctx->frame_count++;
	list_add_tail(&frame->entire_list, &framemgr->entire_list);
	framemgr->entire_count++;
	__score_frame_set_ready(framemgr, frame);
	spin_unlock_irqrestore(&framemgr->slock, flags);

	frame->packet = NULL;
	frame->pending_packet = NULL;
	frame->ret = 0;
	INIT_LIST_HEAD(&frame->buffer_list);
	frame->buffer_count = 0;
	framemgr->all_count++;
	frame->work.func = NULL;

	score_leave();
	return frame;
p_block:
	score_util_bitmap_clear_bit(framemgr->frame_map, frame->frame_id);
p_err:
	kfree(frame);
	return NULL;

}

static void __score_frame_destroy(struct score_frame_manager *framemgr,
		struct score_frame *frame)
{
	unsigned long flags;

	score_enter();
	if (frame->work.func)
		kthread_flush_work(&frame->work);

	spin_lock_irqsave(&framemgr->slock, flags);
	__score_frame_destroy_state(framemgr, frame);

	framemgr->entire_count--;
	list_del(&frame->entire_list);
	frame->sctx->frame_count--;
	list_del(&frame->list);

	spin_unlock_irqrestore(&framemgr->slock, flags);

	score_util_bitmap_clear_bit(framemgr->frame_map, frame->frame_id);
	kfree(frame);
	score_leave();
}

/**
 * score_frame_destroy - Remove frame at frame manager and free memory of frame
 * @frame:      [in]	object about score_frame structure
 */
void score_frame_destroy(struct score_frame *frame)
{
	__score_frame_destroy(frame->owner, frame);
}

int score_frame_manager_probe(struct score_frame_manager *framemgr)
{
	score_enter();
	INIT_LIST_HEAD(&framemgr->entire_list);
	framemgr->entire_count = 0;

	INIT_LIST_HEAD(&framemgr->ready_list);
	framemgr->ready_count = 0;

	INIT_LIST_HEAD(&framemgr->process_list);
	framemgr->process_count = 0;

	INIT_LIST_HEAD(&framemgr->pending_list);
	framemgr->pending_count = 0;

	INIT_LIST_HEAD(&framemgr->complete_list);
	framemgr->complete_count = 0;

	framemgr->all_count = 0;
	framemgr->normal_count = 0;
	framemgr->abnormal_count = 0;

	spin_lock_init(&framemgr->slock);
	score_util_bitmap_init(framemgr->frame_map, SCORE_MAX_FRAME);
	init_waitqueue_head(&framemgr->done_wq);
	framemgr->block = false;

	score_leave();
	return 0;
}

void score_frame_manager_remove(struct score_frame_manager *framemgr)
{
	struct score_frame *frame, *tframe;

	score_enter();
	if (framemgr->entire_count) {
		list_for_each_entry_safe(frame, tframe, &framemgr->entire_list,
				entire_list) {
			score_warn("[%d]frame is already destroyed(count:%d)\n",
					frame->frame_id,
					framemgr->entire_count);
			__score_frame_destroy(framemgr, frame);
		}
	}
	score_leave();
}
