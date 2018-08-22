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

#include <linux/slab.h>
#include <linux/io.h>
#include <linux/delay.h>

#include "score_log.h"
#include "score_dpmu.h"
#include "score_util.h"
#include "score_regs.h"
#include "score_packet.h"
#include "score_context.h"

enum host_info_type {
	MEMORY_TYPE = 1,
	TASK_ID_TYPE = 2
};

static void __score_context_frame_flush(struct score_context *sctx,
		struct score_frame *frame, bool unmap)
{
	struct score_mmu_context *mmu_ctx;
	struct score_mmu_buffer *buf, *tbuf;

	score_enter();
	mmu_ctx = sctx->mmu_ctx;
	if (frame->buffer_count) {
		list_for_each_entry_safe(buf, tbuf, &frame->buffer_list,
				frame_list) {
			score_frame_remove_buffer(frame, buf);
			if (unmap)
				score_mmu_unmap_buffer(mmu_ctx, buf);
		}
	}
	score_leave();
}

static int __score_context_translate_packet(struct score_context *sctx,
		struct score_frame *frame)
{
	int ret = 0;
	unsigned int loop;
	struct score_mmu_context *mmu_ctx;
	struct score_mmu_buffer *kbuf;

	struct score_host_packet *packet;
	struct score_host_packet_info *packet_info;
	struct score_host_buffer *buffer;
	unsigned int *taddr;
	unsigned char *target_packet;
	unsigned int type;
	int offset;
	int addr_offset;

	score_enter();
	mmu_ctx = sctx->mmu_ctx;
	packet = frame->packet;
	packet_info = (struct score_host_packet_info *)&packet->payload[0];
	buffer = (struct score_host_buffer *)&packet_info->payload[0];
	target_packet = (unsigned char *)packet_info + packet->packet_offset;

	for (loop = 0; loop < packet_info->buf_count; loop++) {
		type = buffer[loop].type;
		offset = buffer[loop].offset;
		addr_offset = buffer[loop].addr_offset;
		if (type == TASK_ID_TYPE) {
			target_packet[offset] = frame->frame_id;
		} else if (type == MEMORY_TYPE) {
			kbuf = score_mmu_map_buffer(mmu_ctx, &buffer[loop]);
			if (IS_ERR_OR_NULL(kbuf)) {
				ret = PTR_ERR(kbuf);
				goto p_err;
			}

			taddr = (unsigned int *)(&target_packet[offset]);
			*taddr = (unsigned int)kbuf->dvaddr + addr_offset;
			score_frame_add_buffer(frame, kbuf);
		} else {
			ret = -EINVAL;
			score_err("wrong host_info_type : %d\n", type);
			goto p_err;
		}
	}
	score_leave();
p_err:
	return ret;
}

static int __score_context_wait_for_done(struct score_context *sctx,
		struct score_frame *frame)
{
	int ret = 0;
	struct score_frame_manager *framemgr = frame->owner;

	score_enter();
	if (!score_frame_check_done(frame)) {
		ret = wait_event_interruptible_timeout(framemgr->done_wq,
				score_frame_check_done(frame),
				msecs_to_jiffies(sctx->wait_time));
		if (!ret) {
			score_err("Wait time(%d ms) is over (%d-%d)\n",
					sctx->wait_time, frame->sctx->id,
					frame->frame_id);
			ret = -ETIMEDOUT;
		} else if (ret > 0) {
			ret = 0;
		} else {
			score_err("waitting is ended abnormaly (%d)\n", ret);
			ret = -ECONNABORTED;
		}
	}

	score_leave();
	return ret;
}

static void score_context_write_thread(struct kthread_work *work)
{
	int ret;
	unsigned long flags;
	struct score_frame *frame;
	struct score_frame_manager *framemgr;
	struct score_context *sctx;
	struct score_scq *scq;

	score_enter();
	frame = container_of(work, struct score_frame, work);
	framemgr = frame->owner;

	spin_lock_irqsave(&framemgr->slock, flags);
	if (frame->state == SCORE_FRAME_STATE_COMPLETE)
		goto p_exit;

	ret = score_frame_trans_ready_to_process(frame);
	if (ret) {
		score_frame_trans_any_to_complete(frame, ret);
		goto p_exit;
	}

	sctx = frame->sctx;
	scq = &sctx->system->scq;
	ret = score_scq_write(scq, frame);
	if (ret) {
		if (ret == -EBUSY)
			score_frame_trans_process_to_pending(frame);
		else
			score_frame_trans_process_to_complete(frame, ret);

		goto p_exit;
	}

	score_leave();
p_exit:
	spin_unlock_irqrestore(&framemgr->slock, flags);
}

static int score_context_queue(struct score_context *sctx,
		struct score_frame *frame)
{
	int ret = 0;
	unsigned long flags;

	score_enter();
	ret = __score_context_translate_packet(sctx, frame);
	if (ret) {
		__score_context_frame_flush(sctx, frame, true);
		spin_lock_irqsave(&frame->owner->slock, flags);
		score_frame_trans_ready_to_complete(frame, ret);
		spin_unlock_irqrestore(&frame->owner->slock, flags);
	} else {
		kthread_init_work(&frame->work, score_context_write_thread);
		score_context_write_thread(&frame->work);
	}

	score_leave();
	return ret;
}

static int score_context_deque(struct score_context *sctx,
		struct score_frame *frame)
{
	int ret = 0;
	unsigned long flags;

	score_enter();
	ret = __score_context_wait_for_done(sctx, frame);
	if (ret) {
		spin_lock_irqsave(&frame->owner->slock, flags);
		score_err("frame(%d) is ended abnormaly (%d-%d)\n",
				frame->state, frame->sctx->id,
				frame->frame_id);
		switch (score_frame_get_state(frame)) {
		case SCORE_FRAME_STATE_READY:
			score_frame_trans_ready_to_complete(frame, ret);
			break;
		case SCORE_FRAME_STATE_PROCESS:
			score_dpmu_print_all();
			score_frame_trans_process_to_complete(frame, ret);
			score_frame_flush_process(frame->owner, -ENOSTR);
			wake_up_all(&frame->owner->done_wq);
			score_interface_target_halt(&sctx->system->interface,
					SCORE_KNIGHT1);
			score_interface_target_halt(&sctx->system->interface,
					SCORE_KNIGHT2);
			score_system_boot(sctx->system);
			break;
		case SCORE_FRAME_STATE_PENDING:
			score_frame_trans_pending_to_complete(frame, ret);
			break;
		case SCORE_FRAME_STATE_COMPLETE:
			frame->ret = ret;
			break;
		default:
			score_frame_trans_any_to_complete(frame, ret);
			break;
		}
		spin_unlock_irqrestore(&frame->owner->slock, flags);
	} else {
		if (sctx->core->dpmu_print)
			score_dpmu_print_all();
	}
#if defined(CONFIG_EXYNOS_SCORE)
	__score_context_frame_flush(sctx, frame, false);
#else
	__score_context_frame_flush(sctx, frame, true);
#endif

	score_leave();
	return ret;
}

const struct score_context_ops score_ctx_ops = {
	.queue		= score_context_queue,
	.deque		= score_context_deque
};

/**
 * score_context_create - Create new context and add that at score_core
 * @core:	[in]	object about score_core structure
 *
 * Returns context address created if secceeded, otherwise NULL
 */
struct score_context *score_context_create(struct score_core *core)
{
	int ret = 0;
	struct score_context *sctx;
	unsigned long flags;

	score_enter();
	sctx = kzalloc(sizeof(struct score_context), GFP_KERNEL);
	if (!sctx) {
		ret = -ENOMEM;
		score_err("Fail to alloc context\n");
		goto p_err;
	}

	sctx->mmu_ctx = score_mmu_create_context(&core->system->mmu);
	if (IS_ERR_OR_NULL(sctx->mmu_ctx)) {
		ret = PTR_ERR(sctx->mmu_ctx);
		goto p_err_mem_ctx;
	}

	sctx->id = score_util_bitmap_get_zero_bit(core->ctx_map,
			SCORE_MAX_CONTEXT);
	if (sctx->id == SCORE_MAX_CONTEXT) {
		score_err("Fail to alloc id of context(%d)\n", sctx->id);
		ret = -ENOMEM;
		goto p_err_bitmap;
	}

	spin_lock_irqsave(&core->ctx_slock, flags);
	list_add_tail(&sctx->list, &core->ctx_list);
	core->ctx_count++;
	spin_unlock_irqrestore(&core->ctx_slock, flags);

	atomic_set(&sctx->ref_count, 0);
	INIT_LIST_HEAD(&sctx->frame_list);
	sctx->frame_count = 0;
	sctx->wait_time = core->wait_time;

	sctx->ctx_ops = &score_ctx_ops;
	sctx->core = core;
	sctx->system = core->system;
	sctx->state = SCORE_CONTEXT_START;

	score_leave();
	return sctx;
p_err_bitmap:
	score_mmu_destroy_context(sctx->mmu_ctx);
p_err_mem_ctx:
	kfree(sctx);
p_err:
	return ERR_PTR(ret);
}

/**
 * score_context_destroy - Remove context and free memory of context
 * @sctx:	[in]	object about score_context structure
 */
void score_context_destroy(struct score_context *sctx)
{
	struct score_core *core;
	struct score_system *system;
	struct score_frame_manager *framemgr;
	struct score_frame *frame, *tframe;
	unsigned long flags;
	int timeout = 0;
	bool nonblock = false;

	score_enter();
	core = sctx->core;
	system = sctx->system;
	framemgr = &system->interface.framemgr;

	spin_lock_irqsave(&framemgr->slock, flags);
	if (sctx->frame_count) {
		list_for_each_entry_safe(frame, tframe, &sctx->frame_list,
				list) {
			if (score_frame_get_state(frame) ==
					SCORE_FRAME_STATE_PROCESS) {
				score_frame_flush_process(frame->owner,
						-ECONNABORTED);
				score_interface_target_halt(&system->interface,
						SCORE_KNIGHT1);
				score_interface_target_halt(&system->interface,
						SCORE_KNIGHT2);
				score_system_boot(system);
			} else {
				score_frame_trans_any_to_complete(frame,
						-ECONNABORTED);
			}
			if (score_frame_is_nonblock(frame))
				nonblock = true;
		}
	}
	spin_unlock_irqrestore(&framemgr->slock, flags);

	if (nonblock) {
		list_for_each_entry_safe(frame, tframe, &sctx->frame_list,
				list) {
			if (score_frame_is_nonblock(frame))
				score_frame_destroy(frame);
		}
	}

	while (timeout < 1000) {
		if (!atomic_read(&sctx->ref_count))
			break;
		timeout++;
		udelay(100);
	}
	if (timeout == 1000)
		score_warn("It is not stable to stop context\n");

	spin_lock_irqsave(&core->ctx_slock, flags);
	core->ctx_count--;
	list_del(&sctx->list);
	spin_unlock_irqrestore(&core->ctx_slock, flags);

	score_util_bitmap_clear_bit(core->ctx_map, sctx->id);
	score_mmu_destroy_context(sctx->mmu_ctx);
	kfree(sctx);
	score_leave();
}
