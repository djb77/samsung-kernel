/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "vpu-debug.h"
#include "vpu-framemgr.h"
#include "vpu-graphmgr.h"

void vpu_frame_s_free(struct vpu_framemgr *framemgr, struct vpu_frame *frame)
{
	BUG_ON(!framemgr);
	BUG_ON(!frame);

	frame->state = VPU_FRAME_STATE_FREE;

	list_add_tail(&frame->list, &framemgr->fre_list);
	framemgr->fre_cnt++;
}

void vpu_frame_g_free(struct vpu_framemgr *framemgr, struct vpu_frame **frame)
{
	BUG_ON(!framemgr);
	BUG_ON(!frame);

	if (framemgr->fre_cnt &&
		(*frame = container_of(framemgr->fre_list.next, struct vpu_frame, list))) {
		list_del(&(*frame)->list);
		framemgr->fre_cnt--;
		(*frame)->state = VPU_FRAME_STATE_INVALID;
	} else {
		*frame = NULL;
	}
}

void vpu_frame_free_head(struct vpu_framemgr *framemgr, struct vpu_frame **frame)
{
	BUG_ON(!framemgr);
	BUG_ON(!frame);

	if (framemgr->fre_cnt)
		*frame = container_of(framemgr->fre_list.next, struct vpu_frame, list);
	else
		*frame = NULL;
}

void vpu_frame_free_tail(struct vpu_framemgr *framemgr, struct vpu_frame **frame)
{
	BUG_ON(!framemgr);
	BUG_ON(!frame);

	if (framemgr->fre_cnt)
		*frame = container_of(framemgr->fre_list.prev, struct vpu_frame, list);
	else
		*frame = NULL;
}

void vpu_frame_print_free_list(struct vpu_framemgr *framemgr)
{
	DLOG_INIT();
	struct vpu_frame *frame, *temp;

	DLOG("[FRM] fre(%d, %d) :", framemgr->id, framemgr->fre_cnt);
	list_for_each_entry_safe(frame, temp, &framemgr->fre_list, list) {
		DLOG("%d->", frame->index);
	}
	DLOG("X");

	vpu_info("%s\n", DLOG_OUT());
}

void vpu_frame_s_request(struct vpu_framemgr *framemgr, struct vpu_frame *frame)
{
	BUG_ON(!framemgr);
	BUG_ON(!frame);

	frame->state = VPU_FRAME_STATE_REQUEST;

	list_add_tail(&frame->list, &framemgr->req_list);
	framemgr->req_cnt++;
}

void vpu_frame_g_request(struct vpu_framemgr *framemgr, struct vpu_frame **frame)
{
	BUG_ON(!framemgr);
	BUG_ON(!frame);

	if (framemgr->req_cnt &&
		(*frame = container_of(framemgr->req_list.next, struct vpu_frame, list))) {
		list_del(&(*frame)->list);
		framemgr->req_cnt--;
		(*frame)->state = VPU_FRAME_STATE_INVALID;
	} else {
		*frame = NULL;
	}
}

void vpu_frame_request_head(struct vpu_framemgr *framemgr, struct vpu_frame **frame)
{
	if (framemgr->req_cnt)
		*frame = container_of(framemgr->req_list.next, struct vpu_frame, list);
	else
		*frame = NULL;
}

void vpu_frame_request_tail(struct vpu_framemgr *framemgr, struct vpu_frame **frame)
{
	if (framemgr->req_cnt)
		*frame = container_of(framemgr->req_list.prev, struct vpu_frame, list);
	else
		*frame = NULL;
}

void vpu_frame_print_request_list(struct vpu_framemgr *framemgr)
{
	DLOG_INIT();
	struct vpu_frame *frame, *temp;

	DLOG("[FRM] req(%d, %d) :", framemgr->id, framemgr->req_cnt);
	list_for_each_entry_safe(frame, temp, &framemgr->req_list, list) {
		DLOG("%d->", frame->index);
	}
	DLOG("X");

	vpu_info("%s\n", DLOG_OUT());
}

void vpu_frame_s_prepare(struct vpu_framemgr *framemgr, struct vpu_frame *frame)
{
	BUG_ON(!framemgr);
	BUG_ON(!frame);

	frame->state = VPU_FRAME_STATE_PREPARE;

	list_add_tail(&frame->list, &framemgr->pre_list);
	framemgr->pre_cnt++;
}

void vpu_frame_g_prepare(struct vpu_framemgr *framemgr, struct vpu_frame **frame)
{
	BUG_ON(!framemgr);
	BUG_ON(!frame);

	if (framemgr->pre_cnt &&
		(*frame = container_of(framemgr->pre_list.next, struct vpu_frame, list))) {
		list_del(&(*frame)->list);
		framemgr->pre_cnt--;
		(*frame)->state = VPU_FRAME_STATE_INVALID;
	} else {
		*frame = NULL;
	}
}

void vpu_frame_prepare_head(struct vpu_framemgr *framemgr, struct vpu_frame **frame)
{
	if (framemgr->pre_cnt)
		*frame = container_of(framemgr->pre_list.next, struct vpu_frame, list);
	else
		*frame = NULL;
}

void vpu_frame_prepare_tail(struct vpu_framemgr *framemgr, struct vpu_frame **frame)
{
	if (framemgr->pre_cnt)
		*frame = container_of(framemgr->pre_list.prev, struct vpu_frame, list);
	else
		*frame = NULL;
}

void vpu_frame_print_prepare_list(struct vpu_framemgr *framemgr)
{
	DLOG_INIT();
	struct vpu_frame *frame, *temp;

	DLOG("[FRM] pre(%d, %d) :", framemgr->id, framemgr->pre_cnt);
	list_for_each_entry_safe(frame, temp, &framemgr->pre_list, list) {
		DLOG("%d->", frame->index);
	}
	DLOG("X");

	vpu_info("%s\n", DLOG_OUT());
}

void vpu_frame_s_process(struct vpu_framemgr *framemgr, struct vpu_frame *frame)
{
	BUG_ON(!framemgr);
	BUG_ON(!frame);

	frame->state = VPU_FRAME_STATE_PROCESS;

	list_add_tail(&frame->list, &framemgr->pro_list);
	framemgr->pro_cnt++;
}

void vpu_frame_g_process(struct vpu_framemgr *framemgr, struct vpu_frame **frame)
{
	BUG_ON(!framemgr);
	BUG_ON(!frame);

	if (framemgr->pro_cnt &&
		(*frame = container_of(framemgr->pro_list.next, struct vpu_frame, list))) {
		list_del(&(*frame)->list);
		framemgr->pro_cnt--;
		(*frame)->state = VPU_FRAME_STATE_INVALID;
	} else {
		*frame = NULL;
	}
}

void vpu_frame_process_head(struct vpu_framemgr *framemgr, struct vpu_frame **frame)
{
	if (framemgr->pro_cnt)
		*frame = container_of(framemgr->pro_list.next, struct vpu_frame, list);
	else
		*frame = NULL;
}

void vpu_frame_process_tail(struct vpu_framemgr *framemgr, struct vpu_frame **frame)
{
	if (framemgr->pro_cnt)
		*frame = container_of(framemgr->pro_list.prev, struct vpu_frame, list);
	else
		*frame = NULL;
}

void vpu_frame_print_process_list(struct vpu_framemgr *framemgr)
{
	DLOG_INIT();
	struct vpu_frame *frame, *temp;

	DLOG("[FRM] pro(%d, %d) :", framemgr->id, framemgr->pro_cnt);
	list_for_each_entry_safe(frame, temp, &framemgr->pro_list, list) {
		DLOG("%d->", frame->index);
	}
	DLOG("X");

	vpu_info("%s\n", DLOG_OUT());
}

void vpu_frame_s_complete(struct vpu_framemgr *framemgr, struct vpu_frame *frame)
{
	BUG_ON(!framemgr);
	BUG_ON(!frame);

	frame->state = VPU_FRAME_STATE_COMPLETE;

	list_add_tail(&frame->list, &framemgr->com_list);
	framemgr->com_cnt++;
}

void vpu_frame_g_complete(struct vpu_framemgr *framemgr, struct vpu_frame **frame)
{
	BUG_ON(!framemgr);
	BUG_ON(!frame);

	if (framemgr->com_cnt &&
		(*frame = container_of(framemgr->com_list.next, struct vpu_frame, list))) {
		list_del(&(*frame)->list);
		framemgr->com_cnt--;
		(*frame)->state = VPU_FRAME_STATE_INVALID;
	} else {
		*frame = NULL;
	}
}

void vpu_frame_complete_head(struct vpu_framemgr *framemgr, struct vpu_frame **frame)
{
	if (framemgr->com_cnt)
		*frame = container_of(framemgr->com_list.next, struct vpu_frame, list);
	else
		*frame = NULL;
}

void vpu_frame_complete_tail(struct vpu_framemgr *framemgr, struct vpu_frame **frame)
{
	if (framemgr->com_cnt)
		*frame = container_of(framemgr->com_list.prev, struct vpu_frame, list);
	else
		*frame = NULL;
}

void vpu_frame_print_complete_list(struct vpu_framemgr *framemgr)
{
	DLOG_INIT();
	struct vpu_frame *frame, *temp;

	DLOG("[FRM] com(%d, %d) :", framemgr->id, framemgr->com_cnt);
	list_for_each_entry_safe(frame, temp, &framemgr->com_list, list) {
		DLOG("%d->", frame->index);
	}
	DLOG("X");

	vpu_info("%s\n", DLOG_OUT());
}

void vpu_frame_trans_fre_to_req(struct vpu_framemgr *framemgr, struct vpu_frame *frame)
{
	BUG_ON(!framemgr);
	BUG_ON(!frame);
	BUG_ON(!framemgr->fre_cnt);
	BUG_ON(frame->state != VPU_FRAME_STATE_FREE);

	list_del(&frame->list);
	framemgr->fre_cnt--;
	vpu_frame_s_request(framemgr, frame);
}

void vpu_frame_trans_req_to_pre(struct vpu_framemgr *framemgr, struct vpu_frame *frame)
{
	BUG_ON(!framemgr);
	BUG_ON(!frame);
	BUG_ON(!framemgr->req_cnt);
	BUG_ON(frame->state != VPU_FRAME_STATE_REQUEST);

	list_del(&frame->list);
	framemgr->req_cnt--;
	vpu_frame_s_prepare(framemgr, frame);
}

void vpu_frame_trans_req_to_pro(struct vpu_framemgr *framemgr, struct vpu_frame *frame)
{
	BUG_ON(!framemgr);
	BUG_ON(!frame);
	BUG_ON(!framemgr->req_cnt);
	BUG_ON(frame->state != VPU_FRAME_STATE_REQUEST);

	list_del(&frame->list);
	framemgr->req_cnt--;
	vpu_frame_s_process(framemgr, frame);
}

void vpu_frame_trans_req_to_com(struct vpu_framemgr *framemgr, struct vpu_frame *frame)
{
	BUG_ON(!framemgr);
	BUG_ON(!frame);
	BUG_ON(!framemgr->req_cnt);
	BUG_ON(frame->state != VPU_FRAME_STATE_REQUEST);

	list_del(&frame->list);
	framemgr->req_cnt--;
	vpu_frame_s_complete(framemgr, frame);
}

void vpu_frame_trans_req_to_fre(struct vpu_framemgr *framemgr, struct vpu_frame *frame)
{
	BUG_ON(!framemgr);
	BUG_ON(!frame);
	BUG_ON(!framemgr->req_cnt);
	BUG_ON(frame->state != VPU_FRAME_STATE_REQUEST);

	list_del(&frame->list);
	framemgr->req_cnt--;
	vpu_frame_s_free(framemgr, frame);
}

void vpu_frame_trans_pre_to_pro(struct vpu_framemgr *framemgr, struct vpu_frame *frame)
{
	BUG_ON(!framemgr);
	BUG_ON(!frame);
	BUG_ON(!framemgr->pre_cnt);
	BUG_ON(frame->state != VPU_FRAME_STATE_PREPARE);

	list_del(&frame->list);
	framemgr->pre_cnt--;
	vpu_frame_s_process(framemgr, frame);
}

void vpu_frame_trans_pre_to_com(struct vpu_framemgr *framemgr, struct vpu_frame *frame)
{
	BUG_ON(!framemgr);
	BUG_ON(!frame);
	BUG_ON(!framemgr->pre_cnt);
	BUG_ON(frame->state != VPU_FRAME_STATE_PREPARE);

	list_del(&frame->list);
	framemgr->pre_cnt--;
	vpu_frame_s_complete(framemgr, frame);
}

void vpu_frame_trans_pre_to_fre(struct vpu_framemgr *framemgr, struct vpu_frame *frame)
{
	BUG_ON(!framemgr);
	BUG_ON(!frame);
	BUG_ON(!framemgr->pre_cnt);
	BUG_ON(frame->state != VPU_FRAME_STATE_PREPARE);

	list_del(&frame->list);
	framemgr->pre_cnt--;
	vpu_frame_s_free(framemgr, frame);
}

void vpu_frame_trans_pro_to_com(struct vpu_framemgr *framemgr, struct vpu_frame *frame)
{
	BUG_ON(!framemgr);
	BUG_ON(!frame);
	BUG_ON(!framemgr->pro_cnt);
	BUG_ON(frame->state != VPU_FRAME_STATE_PROCESS);

	list_del(&frame->list);
	framemgr->pro_cnt--;
	vpu_frame_s_complete(framemgr, frame);
}

void vpu_frame_trans_pro_to_fre(struct vpu_framemgr *framemgr, struct vpu_frame *frame)
{
	BUG_ON(!framemgr);
	BUG_ON(!frame);
	BUG_ON(!framemgr->pro_cnt);
	BUG_ON(frame->state != VPU_FRAME_STATE_PROCESS);

	list_del(&frame->list);
	framemgr->pro_cnt--;
	vpu_frame_s_free(framemgr, frame);
}

void vpu_frame_trans_com_to_fre(struct vpu_framemgr *framemgr, struct vpu_frame *frame)
{
	BUG_ON(!framemgr);
	BUG_ON(!frame);
	BUG_ON(!framemgr->com_cnt);
	BUG_ON(frame->state != VPU_FRAME_STATE_COMPLETE);

	list_del(&frame->list);
	framemgr->com_cnt--;
	vpu_frame_s_free(framemgr, frame);
}

void vpu_frame_trans_any_to_fre(struct vpu_framemgr *framemgr, struct vpu_frame *frame)
{
	BUG_ON(!framemgr);
	BUG_ON(!frame);

	list_del(&frame->list);
	switch (frame->state) {
	case VPU_FRAME_STATE_REQUEST:
		framemgr->req_cnt--;
		break;
	case VPU_FRAME_STATE_PREPARE:
		framemgr->pre_cnt--;
		break;
	case VPU_FRAME_STATE_PROCESS:
		framemgr->pro_cnt--;
		break;
	case VPU_FRAME_STATE_COMPLETE:
		framemgr->com_cnt--;
		break;
	default:
		BUG();
		break;
	}

	vpu_frame_s_free(framemgr, frame);
}

void vpu_frame_pick_fre_to_req(struct vpu_framemgr *framemgr, struct vpu_frame **frame)
{
	BUG_ON(!framemgr);
	BUG_ON(!frame);

	vpu_frame_free_head(framemgr, frame);
	if (*frame)
		vpu_frame_trans_fre_to_req(framemgr, *frame);
}

void vpu_frame_print_all(struct vpu_framemgr *framemgr)
{
	BUG_ON(!framemgr);

	vpu_frame_print_request_list(framemgr);
	vpu_frame_print_prepare_list(framemgr);
	vpu_frame_print_process_list(framemgr);
	vpu_frame_print_complete_list(framemgr);
}

int vpu_frame_init(struct vpu_framemgr *framemgr, void *owner)
{
	int ret = 0;
	unsigned long flags;
	u32 index;

	spin_lock_irqsave(&framemgr->slock, flags);

	INIT_LIST_HEAD(&framemgr->fre_list);
	INIT_LIST_HEAD(&framemgr->req_list);
	INIT_LIST_HEAD(&framemgr->pre_list);
	INIT_LIST_HEAD(&framemgr->pro_list);
	INIT_LIST_HEAD(&framemgr->com_list);

	framemgr->tot_cnt = VPU_MAX_FRAME;
	framemgr->fre_cnt = 0;
	framemgr->req_cnt = 0;
	framemgr->pre_cnt = 0;
	framemgr->pro_cnt = 0;
	framemgr->com_cnt = 0;

	/* frame index 0 means invalid because firmware can't accept 0 invocation id */
	for (index = 1; index < framemgr->tot_cnt; ++index) {
		framemgr->frame[index].index = index;
		framemgr->frame[index].owner = owner;
		framemgr->frame[index].findex = VPU_MAX_FRAME;
		framemgr->frame[index].gindex = VPU_MAX_GFRAME;
		vpu_frame_s_free(framemgr, &framemgr->frame[index]);
	}

	spin_unlock_irqrestore(&framemgr->slock, flags);

	return ret;
}

int vpu_frame_deinit(struct vpu_framemgr *framemgr)
{
	int ret = 0;
	unsigned long flags;
	u32 index;

	spin_lock_irqsave(&framemgr->slock, flags);

	for (index = 0; index < framemgr->tot_cnt; ++index)
		vpu_frame_s_free(framemgr, &framemgr->frame[index]);

	spin_unlock_irqrestore(&framemgr->slock, flags);

	return ret;
}

void vpu_frame_flush(struct vpu_framemgr *framemgr)
{
	unsigned long flag;
	struct vpu_frame *frame, *temp;

	BUG_ON(!framemgr);

	spin_lock_irqsave(&framemgr->slock, flag);

	list_for_each_entry_safe(frame, temp, &framemgr->req_list, list) {
		vpu_frame_trans_req_to_fre(framemgr, frame);
		vpu_warn("req frame%d is flushed(count : %d)", frame->id, framemgr->req_cnt);
	}

	list_for_each_entry_safe(frame, temp, &framemgr->pre_list, list) {
		vpu_frame_trans_pre_to_fre(framemgr, frame);
		vpu_warn("pre frame%d is flushed(count : %d)", frame->id, framemgr->pre_cnt);
	}

	list_for_each_entry_safe(frame, temp, &framemgr->pro_list, list) {
		vpu_frame_trans_pro_to_fre(framemgr, frame);
		vpu_warn("pro frame%d is flushed(count : %d)", frame->id, framemgr->pro_cnt);
	}

	list_for_each_entry_safe(frame, temp, &framemgr->com_list, list) {
		vpu_frame_trans_com_to_fre(framemgr, frame);
		vpu_warn("com frame%d is flushed(count : %d)", frame->id, framemgr->com_cnt);
	}

	spin_unlock_irqrestore(&framemgr->slock, flag);

	BUG_ON(framemgr->fre_cnt != (framemgr->tot_cnt - 1));
}