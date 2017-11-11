/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#ifndef VPU_FRAMEMGR_H_
#define VPU_FRAMEMGR_H_

#include <linux/types.h>
#include <linux/kthread.h>

#include "vpu-config.h"
#include "vpu-time.h"
#include "vs4l.h"

#define FMGR_IDX_0		(1 << 0 ) /* graphmgr thread */
#define FMGR_IDX_1		(1 << 1 )
#define FMGR_IDX_2		(1 << 2 )
#define FMGR_IDX_3		(1 << 3 )
#define FMGR_IDX_4		(1 << 4 )
#define FMGR_IDX_5		(1 << 5 )
#define FMGR_IDX_6		(1 << 6 )
#define FMGR_IDX_7		(1 << 7 )
#define FMGR_IDX_8		(1 << 8 )
#define FMGR_IDX_9		(1 << 9 )
#define FMGR_IDX_10		(1 << 10)
#define FMGR_IDX_11		(1 << 11)
#define FMGR_IDX_12		(1 << 12)
#define FMGR_IDX_13		(1 << 13)
#define FMGR_IDX_14		(1 << 14)
#define FMGR_IDX_15		(1 << 15)
#define FMGR_IDX_16		(1 << 16)
#define FMGR_IDX_17		(1 << 17)
#define FMGR_IDX_18		(1 << 18)
#define FMGR_IDX_19		(1 << 19)
#define FMGR_IDX_20		(1 << 20)
#define FMGR_IDX_21		(1 << 21)
#define FMGR_IDX_22		(1 << 22)
#define FMGR_IDX_23		(1 << 23)
#define FMGR_IDX_24		(1 << 24)
#define FMGR_IDX_25		(1 << 25)
#define FMGR_IDX_26		(1 << 26)
#define FMGR_IDX_27		(1 << 27)
#define FMGR_IDX_28		(1 << 28)
#define FMGR_IDX_29		(1 << 29)
#define FMGR_IDX_30		(1 << 30)
#define FMGR_IDX_31		(1 << 31)

#define framemgr_e_barrier_irqs(framemgr, index, flag) \
	framemgr->sindex |= index; spin_lock_irqsave(&framemgr->slock, flag)
#define framemgr_x_barrier_irqr(framemgr, index, flag) \
	spin_unlock_irqrestore(&framemgr->slock, flag); framemgr->sindex &= ~index
#define framemgr_e_barrier_irq(framemgr, index) \
	framemgr->sindex |= index; spin_lock_irq(&framemgr->slock)
#define framemgr_x_barrier_irq(framemgr, index) \
	spin_unlock_irq(&framemgr->slock); framemgr->sindex &= ~index
#define framemgr_e_barrier(framemgr, index) \
	framemgr->sindex |= index; spin_lock(&framemgr->slock)
#define framemgr_x_barrier(framemgr, index) \
	spin_unlock(&framemgr->slock); framemgr->sindex &= ~index

enum vpu_frame_state {
	VPU_FRAME_STATE_FREE = 1,
	VPU_FRAME_STATE_REQUEST,
	VPU_FRAME_STATE_PREPARE,
	VPU_FRAME_STATE_PROCESS,
	VPU_FRAME_STATE_COMPLETE,
	VPU_FRAME_STATE_INVALID
};

enum vpu_frame_flag {
	VPU_FRAME_FLAG_IOCPY = 16
};

enum vpu_frame_message {
	VPU_FRAME_INIT = 1,
	VPU_FRAME_CREATE,
	VPU_FRAME_DESTROY,
	VPU_FRAME_ALLOCATE,
	VPU_FRAME_PROCESS,
	VPU_FRAME_REQUEST = 10,
	VPU_FRAME_DONE,
	VPU_FRAME_NDONE
};

enum vpu_control_message {
	VPU_CTRL_NONE = 100,
	VPU_CTRL_STOP,
	VPU_CTRL_STOP_DONE
};

struct vpu_frame {
	struct list_head		list;
	struct kthread_work		work;
	u32				state;

	u32				message;
	ulong				param0;
	ulong				param1;
	ulong				param2;
	ulong				param3;

	struct vb_container_list	*incl;
	struct vb_container_list	*otcl;
	ulong				flags;

	u32				id;
	struct mutex			*lock;
	u32				index;
	u32				findex;
	u32				gindex;
	void				*owner;

	struct vpu_time 		time[VPU_TMP_COUNT];
};

struct vpu_framemgr {
	u32				id;
	u32				sindex;
	spinlock_t			slock;
	struct vpu_frame		frame[VPU_MAX_FRAME];

	struct list_head		fre_list;
	struct list_head		req_list;
	struct list_head		pre_list;
	struct list_head		pro_list;
	struct list_head		com_list;

	u32				tot_cnt;
	u32				fre_cnt;
	u32				req_cnt;
	u32				pre_cnt;
	u32				pro_cnt;
	u32				com_cnt;
};

void vpu_frame_s_free(struct vpu_framemgr *framemgr, struct vpu_frame *frame);
void vpu_frame_g_free(struct vpu_framemgr *framemgr, struct vpu_frame **frame);
void vpu_frame_free_head(struct vpu_framemgr *framemgr, struct vpu_frame **frame);
void vpu_frame_free_tail(struct vpu_framemgr *framemgr, struct vpu_frame **frame);
void vpu_frame_print_free_list(struct vpu_framemgr *framemgr);

void vpu_frame_s_request(struct vpu_framemgr *framemgr, struct vpu_frame *frame);
void vpu_frame_g_request(struct vpu_framemgr *framemgr, struct vpu_frame **frame);
void vpu_frame_request_head(struct vpu_framemgr *framemgr, struct vpu_frame **frame);
void vpu_frame_request_tail(struct vpu_framemgr *framemgr, struct vpu_frame **frame);
void vpu_frame_print_request_list(struct vpu_framemgr *framemgr);

void vpu_frame_s_prepare(struct vpu_framemgr *framemgr, struct vpu_frame *frame);
void vpu_frame_g_prepare(struct vpu_framemgr *framemgr, struct vpu_frame **frame);
void vpu_frame_prepare_head(struct vpu_framemgr *framemgr, struct vpu_frame **frame);
void vpu_frame_prepare_tail(struct vpu_framemgr *framemgr, struct vpu_frame **frame);
void vpu_frame_print_prepare_list(struct vpu_framemgr *framemgr);

void vpu_frame_s_process(struct vpu_framemgr *framemgr, struct vpu_frame *frame);
void vpu_frame_g_process(struct vpu_framemgr *framemgr, struct vpu_frame **frame);
void vpu_frame_process_head(struct vpu_framemgr *framemgr, struct vpu_frame **frame);
void vpu_frame_process_tail(struct vpu_framemgr *framemgr, struct vpu_frame **frame);
void vpu_frame_print_process_list(struct vpu_framemgr *framemgr);

void vpu_frame_s_complete(struct vpu_framemgr *framemgr, struct vpu_frame *frame);
void vpu_frame_g_complete(struct vpu_framemgr *framemgr, struct vpu_frame **frame);
void vpu_frame_complete_head(struct vpu_framemgr *framemgr, struct vpu_frame **frame);
void vpu_frame_complete_tail(struct vpu_framemgr *framemgr, struct vpu_frame **frame);
void vpu_frame_print_complete_list(struct vpu_framemgr *framemgr);

void vpu_frame_trans_fre_to_req(struct vpu_framemgr *framemgr, struct vpu_frame *frame);
void vpu_frame_trans_req_to_pre(struct vpu_framemgr *framemgr, struct vpu_frame *frame);
void vpu_frame_trans_req_to_pro(struct vpu_framemgr *framemgr, struct vpu_frame *frame);
void vpu_frame_trans_req_to_com(struct vpu_framemgr *framemgr, struct vpu_frame *frame);
void vpu_frame_trans_req_to_fre(struct vpu_framemgr *framemgr, struct vpu_frame *frame);
void vpu_frame_trans_pre_to_pro(struct vpu_framemgr *framemgr, struct vpu_frame *frame);
void vpu_frame_trans_pre_to_com(struct vpu_framemgr *framemgr, struct vpu_frame *frame);
void vpu_frame_trans_pre_to_fre(struct vpu_framemgr *framemgr, struct vpu_frame *frame);
void vpu_frame_trans_pro_to_com(struct vpu_framemgr *framemgr, struct vpu_frame *frame);
void vpu_frame_trans_pro_to_fre(struct vpu_framemgr *framemgr, struct vpu_frame *frame);
void vpu_frame_trans_com_to_fre(struct vpu_framemgr *framemgr, struct vpu_frame *frame);
void vpu_frame_trans_any_to_fre(struct vpu_framemgr *framemgr, struct vpu_frame *frame);

void vpu_frame_pick_fre_to_req(struct vpu_framemgr *framemgr, struct vpu_frame **frame);
void vpu_frame_print_all(struct vpu_framemgr *framemgr);

int vpu_frame_init(struct vpu_framemgr *framemgr, void *owner);
int vpu_frame_deinit(struct vpu_framemgr *framemgr);
void vpu_frame_flush(struct vpu_framemgr *framemgr);

#endif
