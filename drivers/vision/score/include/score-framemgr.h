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

#ifndef SCORE_FRAMEMGR_H_
#define SCORE_FRAMEMGR_H_

#include <linux/types.h>
#include <linux/kthread.h>
#include <linux/mutex.h>

#include "score-config.h"
#include "vision-buffer.h"

enum score_frame_state {
	SCORE_FRAME_STATE_READY = 1,
	SCORE_FRAME_STATE_PROCESS,
	SCORE_FRAME_STATE_PENDING,
	SCORE_FRAME_STATE_COMPLETE
};

enum score_frame_message {
	SCORE_FRAME_CREATE = 1,
	SCORE_FRAME_READY,
	SCORE_FRAME_PROCESS,
	SCORE_FRAME_PENDING,
	SCORE_FRAME_DONE,
	SCORE_FRAME_FAIL
};

struct score_frame {
	struct list_head		list;
	struct list_head		state_list;
	struct kthread_work		work;
	u32				state;
	int				message;
	struct vb_buffer		buffer;
	u32				vid;
	u32				fid;

	struct score_frame		*frame;
	struct score_framemgr		*owner;

	struct timespec			ts[2];
};

struct score_framemgr {
	u32				id;
	struct mutex			lock;
	spinlock_t			slock;
	wait_queue_head_t		done_wq;
	void				*cookie;
	kthread_work_func_t		func;
	DECLARE_BITMAP(fid_map, SCORE_MAX_FRAME);

	struct list_head		entire_list;
	struct list_head		rdy_list;
	struct list_head		pro_list;
	struct list_head		pend_list;
	struct list_head		com_list;

	u32				entire_cnt;
	u32				rdy_cnt;
	u32				pro_cnt;
	u32				pend_cnt;
	u32				com_cnt;
};

void score_frame_g_entire(
		struct score_framemgr *framemgr,
		int vid, int fid, struct score_frame **frame);
void score_frame_g_rdy(
		struct score_framemgr *framemgr,
		int vid, int fid, struct score_frame **frame);
void score_frame_g_pro(
		struct score_framemgr *framemgr,
		int vid, int fid, struct score_frame **frame);
void score_frame_g_pend(
		struct score_framemgr *framemgr,
		int vid, int fid, struct score_frame **frame);
void score_frame_g_com(
		struct score_framemgr *framemgr,
		int vid, int fid, struct score_frame **frame);

int score_frame_trans_rdy_to_pro(struct score_frame *frame);
int score_frame_trans_rdy_to_com(struct score_frame *frame);
int score_frame_trans_pro_to_rdy(struct score_frame *frame);
int score_frame_trans_pro_to_pend(struct score_frame *frame);
int score_frame_trans_pro_to_com(struct score_frame *frame);
int score_frame_trans_pend_to_rdy(struct score_frame *frame);
int score_frame_trans_pend_to_com(struct score_frame *frame);
int score_frame_trans_any_to_com(struct score_frame *frame);

void score_frame_print_rdy_list(struct score_framemgr *framemgr);
void score_frame_print_pro_list(struct score_framemgr *framemgr);
void score_frame_print_pend_list(struct score_framemgr *framemgr);
void score_frame_print_com_list(struct score_framemgr *framemgr);
void score_frame_print_all_list(struct score_framemgr *framemgr);

struct score_frame *score_frame_create(struct score_framemgr *framemgr);
int score_frame_destroy(struct score_frame *frame);

static inline int score_frame_done(struct score_frame *frame)
{
	return ((frame->message == SCORE_FRAME_DONE) || (frame->message == SCORE_FRAME_FAIL));
}

void score_frame_pro_flush(struct score_framemgr *framemgr);
void score_frame_all_flush(struct score_framemgr *framemgr);

int score_framemgr_init(struct score_framemgr *framemgr);
void score_framemgr_deinit(struct score_framemgr *framemgr);

#endif
