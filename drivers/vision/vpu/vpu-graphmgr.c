/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/sched/rt.h>
#include <linux/bug.h>

#include "vpu-framemgr.h"
#include "vpu-device.h"
#include "vpu-graphmgr.h"

#define TIMER_CHECK_POINT(v)	graphmgr->tick_pos = v
#define GRAPH_CHECK_POINT(v)	graphmgr->sched_pos = v

static void __vpu_gframe_s_free(struct vpu_graphmgr *graphmgr, struct vpu_gframe *gframe)
{
	BUG_ON(!graphmgr);
	BUG_ON(!gframe);

	gframe->state = VPU_GFRAME_STATE_FREE;

	list_add_tail(&gframe->list, &graphmgr->gfre_list);
	graphmgr->gfre_cnt++;
}

static void __vpu_gframe_free_head(struct vpu_graphmgr *graphmgr, struct vpu_gframe **gframe)
{
	BUG_ON(!graphmgr);
	BUG_ON(!gframe);

	if (graphmgr->gfre_cnt)
		*gframe = container_of(graphmgr->gfre_list.next, struct vpu_gframe, list);
	else
		*gframe = NULL;
}

static void __vpu_gframe_s_ready(struct vpu_graphmgr *graphmgr, struct vpu_gframe *gframe)
{
	BUG_ON(!graphmgr);
	BUG_ON(!gframe);

	gframe->state = VPU_GFRAME_STATE_FREE;

	list_add_tail(&gframe->list, &graphmgr->grdy_list);
	graphmgr->grdy_cnt++;
}

static void __vpu_gframe_g_ready(struct vpu_graphmgr *graphmgr,
	struct vpu_gframe **gframe)
{
	if (graphmgr->grdy_cnt &&
		(*gframe = container_of(graphmgr->grdy_list.next, struct vpu_gframe, list))) {
		list_del(&(*gframe)->list);
		graphmgr->grdy_cnt--;
		(*gframe)->state = VPU_FRAME_STATE_INVALID;
	} else {
		*gframe = NULL;
	}
}

static void __vpu_gframe_s_request(struct vpu_graphmgr *graphmgr,
	struct vpu_gframe *prev,
	struct vpu_gframe *gframe,
	struct vpu_gframe *next)
{
	gframe->state = VPU_GFRAME_STATE_REQUEST;

	if (prev && next) {
		next->list.prev = &gframe->list;
	        gframe->list.next = &next->list;
	        gframe->list.prev = &prev->list;
	        prev->list.next = &gframe->list;
	} else if (prev) {
		list_add_tail(&gframe->list, &graphmgr->greq_list);
	} else {
		list_add(&gframe->list, &graphmgr->greq_list);
	}

	graphmgr->greq_cnt++;
}

static void __vpu_gframe_s_alloc(struct vpu_graphmgr *graphmgr, struct vpu_gframe *gframe)
{
	gframe->state = VPU_GFRAME_STATE_ALLOC;
	list_add_tail(&gframe->list, &graphmgr->galc_list);
	graphmgr->galc_cnt++;
}

void __vpu_gframe_s_process(struct vpu_graphmgr *graphmgr, struct vpu_gframe *gframe)
{
	BUG_ON(!graphmgr);
	BUG_ON(!gframe);

	gframe->state = VPU_GFRAME_STATE_PROCESS;
	list_add_tail(&gframe->list, &graphmgr->gpro_list);
	graphmgr->gpro_cnt++;
}

static void __vpu_gframe_s_complete(struct vpu_graphmgr *graphmgr, struct vpu_gframe *gframe)
{
	BUG_ON(!graphmgr);
	BUG_ON(!gframe);

	gframe->state = VPU_GFRAME_STATE_COMPLETE;
	list_add_tail(&gframe->list, &graphmgr->gcom_list);
	graphmgr->gcom_cnt++;
}

void __vpu_gframe_trans_fre_to_rdy(struct vpu_graphmgr *graphmgr, struct vpu_gframe *gframe)
{
	BUG_ON(!graphmgr);
	BUG_ON(!gframe);
	BUG_ON(!graphmgr->gfre_cnt);
	BUG_ON(gframe->state != VPU_GFRAME_STATE_FREE);

	list_del(&gframe->list);
	graphmgr->gfre_cnt--;
	__vpu_gframe_s_ready(graphmgr, gframe);
}

void __vpu_gframe_trans_rdy_to_fre(struct vpu_graphmgr *graphmgr, struct vpu_gframe *gframe)
{
	BUG_ON(!graphmgr);
	BUG_ON(!gframe);
	BUG_ON(!graphmgr->grdy_cnt);
	BUG_ON(gframe->state != VPU_GFRAME_STATE_READY);

	list_del(&gframe->list);
	graphmgr->grdy_cnt--;
	__vpu_gframe_s_free(graphmgr, gframe);
}

void __vpu_gframe_trans_req_to_alc(struct vpu_graphmgr *graphmgr, struct vpu_gframe *gframe)
{
	BUG_ON(!graphmgr);
	BUG_ON(!gframe);
	BUG_ON(!graphmgr->greq_cnt);
	BUG_ON(gframe->state != VPU_GFRAME_STATE_REQUEST);

	list_del(&gframe->list);
	graphmgr->greq_cnt--;
	__vpu_gframe_s_alloc(graphmgr, gframe);
}

void __vpu_gframe_trans_req_to_pro(struct vpu_graphmgr *graphmgr, struct vpu_gframe *gframe)
{
	BUG_ON(!graphmgr);
	BUG_ON(!gframe);
	BUG_ON(!graphmgr->greq_cnt);
	BUG_ON(gframe->state != VPU_GFRAME_STATE_REQUEST);

	list_del(&gframe->list);
	graphmgr->greq_cnt--;
	__vpu_gframe_s_process(graphmgr, gframe);
}

void __vpu_gframe_trans_req_to_fre(struct vpu_graphmgr *graphmgr, struct vpu_gframe *gframe)
{
	BUG_ON(!graphmgr);
	BUG_ON(!gframe);
	BUG_ON(!graphmgr->greq_cnt);
	BUG_ON(gframe->state != VPU_GFRAME_STATE_REQUEST);

	list_del(&gframe->list);
	graphmgr->greq_cnt--;
	__vpu_gframe_s_free(graphmgr, gframe);
}

void __vpu_gframe_trans_alc_to_pro(struct vpu_graphmgr *graphmgr, struct vpu_gframe *gframe)
{
	BUG_ON(!graphmgr);
	BUG_ON(!gframe);
	BUG_ON(!graphmgr->galc_cnt);
	BUG_ON(gframe->state != VPU_GFRAME_STATE_ALLOC);

	list_del(&gframe->list);
	graphmgr->galc_cnt--;
	__vpu_gframe_s_process(graphmgr, gframe);
}

void __vpu_gframe_trans_alc_to_fre(struct vpu_graphmgr *graphmgr, struct vpu_gframe *gframe)
{
	BUG_ON(!graphmgr);
	BUG_ON(!gframe);
	BUG_ON(!graphmgr->galc_cnt);
	BUG_ON(gframe->state != VPU_GFRAME_STATE_ALLOC);

	list_del(&gframe->list);
	graphmgr->galc_cnt--;
	__vpu_gframe_s_free(graphmgr, gframe);
}

static void __vpu_gframe_trans_pro_to_com(struct vpu_graphmgr *graphmgr, struct vpu_gframe *gframe)
{
	BUG_ON(!graphmgr);
	BUG_ON(!gframe);
	BUG_ON(!graphmgr->gpro_cnt);
	BUG_ON(gframe->state != VPU_GFRAME_STATE_PROCESS);

	list_del(&gframe->list);
	graphmgr->gpro_cnt--;
	__vpu_gframe_s_complete(graphmgr, gframe);
}

void __vpu_gframe_trans_pro_to_fre(struct vpu_graphmgr *graphmgr, struct vpu_gframe *gframe)
{
	BUG_ON(!graphmgr);
	BUG_ON(!gframe);
	BUG_ON(!graphmgr->gpro_cnt);
	BUG_ON(gframe->state != VPU_GFRAME_STATE_PROCESS);

	list_del(&gframe->list);
	graphmgr->gpro_cnt--;
	__vpu_gframe_s_free(graphmgr, gframe);
}

static void __vpu_gframe_trans_com_to_fre(struct vpu_graphmgr *graphmgr, struct vpu_gframe *gframe)
{
	BUG_ON(!graphmgr);
	BUG_ON(!gframe);
	BUG_ON(!graphmgr->gcom_cnt);
	BUG_ON(gframe->state != VPU_GFRAME_STATE_COMPLETE);

	list_del(&gframe->list);
	graphmgr->gcom_cnt--;
	__vpu_gframe_s_free(graphmgr, gframe);
}

void vpu_gframe_print(struct vpu_graphmgr *graphmgr)
{
	DLOG_INIT();
	struct vpu_gframe *gframe, *gtemp;

	BUG_ON(!graphmgr);

	DLOG("READY LIST(%d) :", graphmgr->grdy_cnt);
	list_for_each_entry_safe(gframe, gtemp, &graphmgr->grdy_list, list) {
		BUG_ON(!gframe->graph);
		BUG_ON(!gframe->frame);
		DLOG(" %2d(%d, %d)", gframe->graph->id, gframe->index, gframe->frame->id);
	}
	vpu_info("%s\n", DLOG_OUT());

	DLOG("REQUEST LIST(%d) :", graphmgr->greq_cnt);
	list_for_each_entry_safe(gframe, gtemp, &graphmgr->greq_list, list) {
		BUG_ON(!gframe->graph);
		BUG_ON(!gframe->frame);
		DLOG(" %2d(%d, %d)", gframe->graph->id, gframe->index, gframe->frame->id);
	}
	vpu_info("%s\n", DLOG_OUT());

	DLOG("ALLOC LIST(%d) :", graphmgr->galc_cnt);
	list_for_each_entry_safe(gframe, gtemp, &graphmgr->gpro_list, list) {
		BUG_ON(!gframe->graph);
		BUG_ON(!gframe->frame);
		DLOG(" %2d(%d, %d)", gframe->graph->id, gframe->index, gframe->frame->id);
	}
	vpu_info("%s\n", DLOG_OUT());

	DLOG("PROCESS LIST(%d) :", graphmgr->gpro_cnt);
	list_for_each_entry_safe(gframe, gtemp, &graphmgr->gpro_list, list) {
		BUG_ON(!gframe->graph);
		BUG_ON(!gframe->frame);
		DLOG(" %2d(%d, %d)", gframe->graph->id, gframe->index, gframe->frame->id);
	}
	vpu_info("%s\n", DLOG_OUT());

	DLOG("COMPLETE LIST(%d) :", graphmgr->gcom_cnt);
	list_for_each_entry_safe(gframe, gtemp, &graphmgr->gcom_list, list) {
		BUG_ON(!gframe->graph);
		BUG_ON(!gframe->frame);
		DLOG(" %2d(%d, %d)", gframe->graph->id, gframe->index, gframe->frame->id);
	}
	vpu_info("%s\n", DLOG_OUT());
}

static int __vpu_itf_enum(struct vpu_interface *interface,
	struct vpu_graph *graph)
{
	int ret = 0;
	unsigned long flags;
	struct vpu_framemgr *iframemgr;
	struct vpu_frame *iframe;

	BUG_ON(!interface);
	BUG_ON(!graph);

	iframemgr = &interface->framemgr;

	ret = vpu_hw_enum(interface);
	if (ret) {
		vpu_err("vpu_hw_enum is fail(%d)\n", ret);
		goto p_err;
	}

	framemgr_e_barrier_irqs(iframemgr, 0, flags);
	vpu_frame_pick_fre_to_req(iframemgr, &iframe);
	framemgr_x_barrier_irqr(iframemgr, 0, flags);

	if (!iframe) {
		vpu_err("iframe is NULL\n");
		ret = -ENOMEM;
		goto p_err;
	}

	iframe->id = 0;
	iframe->lock = &graph->local_lock;
	iframe->findex = VPU_MAX_FRAME;
	iframe->gindex = VPU_MAX_GFRAME;
	iframe->message = VPU_FRAME_INIT;
	iframe->param0 = 0;
	iframe->param1 = graph->id;
	iframe->param2 = 0;
	iframe->param3 = 0;

	ret = vpu_hw_init(interface, iframe);
	if (ret) {
		vpu_err("vpu_hw_init is fail(%d)\n", ret);
		goto p_err;
	}

p_err:
	return ret;
}

static int __vpu_itf_create(struct vpu_interface *interface,
	struct vpu_graph *graph)
{
	int ret = 0;
	unsigned long flags;
	struct vpu_framemgr *iframemgr;
	struct vpu_frame *iframe;
	struct vpuo_pu *pu, *temp;
	struct vpu_mark_for_invoke_ext_mem_vec_ds mem_mark;
	u32 i;

	BUG_ON(!interface);
	BUG_ON(!graph);

	iframemgr = &interface->framemgr;
	mem_mark.num_of_buffers = 0;

	framemgr_e_barrier_irqs(iframemgr, 0, flags);
	vpu_frame_pick_fre_to_req(iframemgr, &iframe);
	framemgr_x_barrier_irqr(iframemgr, 0, flags);

	if (!iframe) {
		vpu_err("iframe is NULL\n");
		ret = -ENOMEM;
		goto p_err;
	}

#ifdef DBG_TIMEMEASURE
	vpu_get_timestamp(&iframe->time[VPU_TMP_REQUEST]);
#endif

	if (test_bit(VS4L_GRAPH_FLAG_PRIMITIVE, &graph->flags)) {
		for (i = 0; i < graph->iobuffer_cnt; ++i) {
			mem_mark.external_mem_index[i] = graph->iobuffer_idx[i];
			mem_mark.num_of_buffers++;
		}

		goto p_skip_primitive;
	}

	list_for_each_entry_safe(pu, temp, &graph->inleaf_list, gleaf_entry) {
		for (i = 0; i < pu->buffer_cnt; ++i) {
			if (!pu->buffer_ptr[i])
				continue;

			if (mem_mark.num_of_buffers >= VPUL_MAX_MAPS_DESC) {
				vpu_err("mem_mark.num_of_buffers is over1(%d)\n", mem_mark.num_of_buffers);
				break;
			}

			if (!pu->buffer_shm[i]) {
				mem_mark.external_mem_index[mem_mark.num_of_buffers] = pu->buffer_idx[i];
				mem_mark.num_of_buffers++;
			}
		}
	}

	list_for_each_entry_safe(pu, temp, &graph->otleaf_list, gleaf_entry) {
		for (i = 0; i < pu->buffer_cnt; ++i) {
			if (!pu->buffer_ptr[i])
				continue;

			if (mem_mark.num_of_buffers >= VPUL_MAX_MAPS_DESC) {
				vpu_err("mem_mark.num_of_buffers is over2(%d)\n", mem_mark.num_of_buffers);
				break;
			}

			if (!pu->buffer_shm[i]) {
				mem_mark.external_mem_index[mem_mark.num_of_buffers] = pu->buffer_idx[i];
				mem_mark.num_of_buffers++;
			}
		}
	}

p_skip_primitive:
	iframe->id = 0;
	iframe->lock = &graph->local_lock;
	iframe->findex = VPU_MAX_FRAME;
	iframe->gindex = VPU_MAX_GFRAME;
	iframe->message = VPU_FRAME_CREATE;
	iframe->param0 = (ulong)graph->desc_mtask;
	iframe->param1 = graph->id;
	iframe->param2 = (ulong)&mem_mark;
	iframe->param3 = 0;

	ret = vpu_hw_create(interface, iframe);
	if (ret) {
		vpu_err("vpu_hw_create is fail(%d)\n", ret);
		goto p_err;
	}

#ifdef DBG_TIMEMEASURE
	vpu_iinfo("[TM] C : %ldus, %ldus\n", graph,
		VPU_TIME_IN_US(iframe->time[VPU_TMP_PROCESS]) -
		VPU_TIME_IN_US(iframe->time[VPU_TMP_REQUEST]),
		VPU_TIME_IN_US(iframe->time[VPU_TMP_DONE]) -
		VPU_TIME_IN_US(iframe->time[VPU_TMP_PROCESS]));
#endif

p_err:
	return ret;
}

static int __vpu_itf_destroy(struct vpu_interface *interface,
	struct vpu_graph *graph)
{
	int ret = 0;
	unsigned long flags;
	struct vpu_framemgr *iframemgr;
	struct vpu_frame *iframe;

	BUG_ON(!interface);
	BUG_ON(!graph);

	iframemgr = &interface->framemgr;

	framemgr_e_barrier_irqs(iframemgr, 0, flags);
	vpu_frame_pick_fre_to_req(iframemgr, &iframe);
	framemgr_x_barrier_irqr(iframemgr, 0, flags);

	if (!iframe) {
		vpu_err("iframe is NULL\n");
		ret = -ENOMEM;
		goto p_err;
	}

#ifdef DBG_TIMEMEASURE
	vpu_get_timestamp(&iframe->time[VPU_TMP_REQUEST]);
#endif

	iframe->id = 0;
	iframe->lock = &graph->local_lock;
	iframe->findex = VPU_MAX_FRAME;
	iframe->gindex = VPU_MAX_GFRAME;
	iframe->message = VPU_FRAME_DESTROY;
	iframe->param0 = (ulong)graph->desc_mtask;
	iframe->param1 = graph->id;
	iframe->param2 = 0;
	iframe->param3 = 0;

	ret = vpu_hw_destroy(interface, iframe);
	if (ret) {
		vpu_err("vpu_hw_destory is fail(%d)\n", ret);
		goto p_err;
	}

#ifdef DBG_TIMEMEASURE
	vpu_iinfo("[TM] D : %ldus, %ldus\n", graph,
		VPU_TIME_IN_US(iframe->time[VPU_TMP_PROCESS]) -
		VPU_TIME_IN_US(iframe->time[VPU_TMP_REQUEST]),
		VPU_TIME_IN_US(iframe->time[VPU_TMP_DONE]) -
		VPU_TIME_IN_US(iframe->time[VPU_TMP_PROCESS]));
#endif

p_err:
	return ret;
}

static int __vpu_itf_allocate(struct vpu_interface *interface,
	struct vpu_graph *graph)
{
	int ret = 0;
	unsigned long flags;
	struct vpu_framemgr *iframemgr;
	struct vpu_frame *iframe;

	BUG_ON(!interface);
	BUG_ON(!graph);

	iframemgr = &interface->framemgr;

	framemgr_e_barrier_irqs(iframemgr, 0, flags);
	vpu_frame_pick_fre_to_req(iframemgr, &iframe);
	framemgr_x_barrier_irqr(iframemgr, 0, flags);

	if (!iframe) {
		vpu_err("iframe is NULL\n");
		ret = -ENOMEM;
		goto p_err;
	}

#ifdef DBG_TIMEMEASURE
	vpu_get_timestamp(&iframe->time[VPU_TMP_REQUEST]);
#endif

	iframe->id = 0;
	iframe->lock = &graph->local_lock;
	iframe->findex = VPU_MAX_FRAME;
	iframe->gindex = VPU_MAX_GFRAME;
	iframe->message = VPU_FRAME_ALLOCATE;
	iframe->param0 = (ulong)graph->desc_mtask;
	iframe->param1 = graph->id;
	iframe->param2 = 0;
	iframe->param3 = 0;

	ret = vpu_hw_allocate(interface, iframe);
	if (ret) {
		vpu_err("vpu_hw_allocate is fail(%d)\n", ret);
		goto p_err;
	}

#ifdef DBG_TIMEMEASURE
	vpu_iinfo("[TM] A : %ldus, %ldus\n", graph,
		VPU_TIME_IN_US(iframe->time[VPU_TMP_PROCESS]) -
		VPU_TIME_IN_US(iframe->time[VPU_TMP_REQUEST]),
		VPU_TIME_IN_US(iframe->time[VPU_TMP_DONE]) -
		VPU_TIME_IN_US(iframe->time[VPU_TMP_PROCESS]));
#endif

p_err:
	return ret;
}

static int __vpu_itf_process(struct vpu_interface *interface,
	struct vpu_graph *graph,
	struct vpu_frame *frame)
{
	int ret = 0;
	unsigned long flags;
	struct vpu_framemgr *iframemgr;
	struct vpu_frame *iframe;
	struct vpuo_pu *pu, *temp;
	struct vpu_invoke_external_mem_vector_ds mvectors;
	union vpul_pu_parameters *update_array;
	u32 i;

	BUG_ON(!interface);
	BUG_ON(!graph);
	BUG_ON(!frame);

	iframemgr = &interface->framemgr;
	mvectors.num_of_buffers = 0;
	update_array = NULL;

	framemgr_e_barrier_irqs(iframemgr, 0, flags);
	vpu_frame_pick_fre_to_req(iframemgr, &iframe);
	framemgr_x_barrier_irqr(iframemgr, 0, flags);

	if (!iframe) {
		vpu_err("iframe is NULL\n");
		ret = -ENOMEM;
		goto p_err;
	}

	if (test_bit(VS4L_GRAPH_FLAG_PRIMITIVE, &graph->flags)) {
		for (i = 0; i < graph->iobuffer_cnt; ++i) {
			mvectors.addresses_vector[i] = graph->iobuffer_dat[i];
			mvectors.num_of_buffers++;
		}

		goto p_skip_primitive;
	}

	list_for_each_entry_safe(pu, temp, &graph->inleaf_list, gleaf_entry) {
		for (i = 0; i < pu->buffer_cnt; ++i) {
			if (!pu->buffer_ptr[i])
				continue;

			if (mvectors.num_of_buffers >= VPUL_MAX_MAPS_DESC) {
				vpu_err("mvectors.num_of_buffers is over1(%d)\n", mvectors.num_of_buffers);
				break;
			}

			if (!pu->buffer_shm[i]) {
				mvectors.addresses_vector[mvectors.num_of_buffers] = *pu->buffer_ptr[i];
				mvectors.num_of_buffers++;
			}
		}
	}

	list_for_each_entry_safe(pu, temp, &graph->otleaf_list, gleaf_entry) {
		for (i = 0; i < pu->buffer_cnt; ++i) {
			if (!pu->buffer_ptr[i])
				continue;

			if (mvectors.num_of_buffers >= VPUL_MAX_MAPS_DESC) {
				vpu_err("mvectors.num_of_buffers is over2(%d)\n", mvectors.num_of_buffers);
				break;
			}

			if (!pu->buffer_shm[i]){
				mvectors.addresses_vector[mvectors.num_of_buffers] = *pu->buffer_ptr[i];
				mvectors.num_of_buffers++;
			}
		}
	}

p_skip_primitive:
	if (test_bit(VPU_GRAPH_FLAG_UPDATE_PARAM, &graph->flags)) {
		ret = CALL_GOPS(graph, update_param, frame);
		if (ret) {
			vpu_err("GOPS(update_param) is fail(%d)\n", ret);
			goto p_err;
		}

		update_array = graph->update_array;
		clear_bit(VPU_GRAPH_FLAG_UPDATE_PARAM, &graph->flags);
	}

	iframe->id = frame->id;
	iframe->lock = &graph->local_lock;
	iframe->findex = frame->index;
	iframe->gindex = frame->gindex;
	iframe->message = VPU_FRAME_PROCESS;
	iframe->param0 = (ulong)graph->desc_mtask;
	iframe->param1 = graph->id;
	iframe->param2 = (ulong)&mvectors; /* return : DONE or NDONE */
	iframe->param3 = (ulong)update_array; /* return : error code if param2 is NDONE */
	iframe->flags = frame->flags;
	iframe->incl = frame->incl;
	iframe->otcl = frame->otcl;

	ret = CALL_GOPS(graph, process, frame);
	if (ret) {
		vpu_err("GOPS(process) is fail(%d)\n", ret);
		goto p_err;
	}

	ret = vpu_hw_process(interface, iframe);
	if (ret) {
		vpu_err("vpu_hw_process is fail(%d)\n", ret);
		goto p_err;
	}

p_err:
	return ret;
}

static void __vpu_graphmgr_sched(struct vpu_graphmgr *graphmgr)
{
	int ret = 0;
	struct vpu_graph *graph;
	struct vpu_frame *frame;
	struct vpu_gframe *ready, *request, *process, *prev, *next, *temp;
	struct vpu_interface *interface;
	struct vpu_resource *resource;

	interface = graphmgr->interface;
	resource = graphmgr->resource;
	ready = NULL;
	request = NULL;
	prev = NULL;
	next = NULL;

	GRAPH_CHECK_POINT(20);
	mutex_lock(&graphmgr->glock);

	/* 1. priority order */
	while (1) {
		__vpu_gframe_g_ready(graphmgr, &ready);
		if (!ready)
			break;

		list_for_each_entry_safe(next, temp, &graphmgr->greq_list, list) {
			if (ready->priority > next->priority)
				break;

			prev = next;
			next = NULL;
		}

		__vpu_gframe_s_request(graphmgr, prev, ready, next);
	}

	GRAPH_CHECK_POINT(21);

	/* 2. resource allocation */
	list_for_each_entry_safe(request, temp, &graphmgr->greq_list, list) {
		graph = request->graph;
		frame = request->frame;

#ifdef VPU_DYNAMIC_RESOURCE
		ret = CALL_GOPS(graph, get_resource, frame);
		if (ret)
			continue;
#endif

		__vpu_gframe_trans_req_to_pro(graphmgr, request);
	}

	mutex_unlock(&graphmgr->glock);
	GRAPH_CHECK_POINT(22);

	/* 3. process graph */
	list_for_each_entry_safe(process, temp, &graphmgr->gpro_list, list) {
		graph = process->graph;
		frame = process->frame;

		ret = __vpu_itf_process(interface, graph, frame);
		if (ret) {
			vpu_err("__vpu_itf_process is fail(%d)\n", ret);

			ret = CALL_GOPS(graph, cancel, frame);
			if (ret) {
				vpu_err("CALL_GOPS(cancel) is fail(%d)\n", ret);
				BUG();
			}

			ret = CALL_GOPS(graph, put_resource, frame);
			if (ret) {
				vpu_err("CALL_GOPS(put_resource) is fail(%d)\n", ret);
				BUG();
			}

			process->graph = NULL;
			process->frame = NULL;
			__vpu_gframe_trans_pro_to_fre(graphmgr, process);
			continue;
		}

		__vpu_gframe_trans_pro_to_com(graphmgr, process);
	}

	GRAPH_CHECK_POINT(23);
	graphmgr->sched_cnt++;
}

static int vpu_time_thread(void *data)
{
	struct vpu_graphmgr *graphmgr = (struct vpu_graphmgr *)data;
	struct vpu_gframe *request, *temp1, *temp2;
	struct vpu_gframe *next, *prev;
	struct vpu_graph *graph;

	BUG_ON(!graphmgr);

	while (!kthread_should_stop()) {
		TIMER_CHECK_POINT(1);
		msleep(VPU_TIME_TICK);

		if (!test_bit(VPU_GRAPHMGR_OPEN, &graphmgr->state))
			return 0;

		if (!atomic_read(&graphmgr->periodics))
			goto p_noperiodic;

		mutex_lock(&graphmgr->glock);

		TIMER_CHECK_POINT(2);
		list_for_each_entry_safe(request, temp1, &graphmgr->greq_list, list) {
			graph = request->graph;

			if (!test_bit(VPU_GRAPH_STATE_START, &graph->state)) {
				vpu_err("graph %d is NOT start\n", graph->id);
				BUG();
			}

			if (test_bit(VS4L_GRAPH_FLAG_PERIODIC, &graph->flags)) {
				if (--request->ticks > 0)
					continue;

				TIMER_CHECK_POINT(3);
				vpu_iinfo("priority changed(%d -> %d)\n", graph,
					request->priority, VPU_GRAPH_MAX_PRIORITY + 1);
				request->priority = VPU_GRAPH_MAX_PRIORITY + 1;
				request->ticks = graph->period_ticks;

				list_del(&request->list);
				graphmgr->greq_cnt--;

				next = NULL;
				prev = NULL;
				list_for_each_entry_safe(next, temp2, &graphmgr->greq_list, list) {
					if (request->priority > next->priority)
						break;

					prev = next;
					next = NULL;
				}

				TIMER_CHECK_POINT(4);
				__vpu_gframe_s_request(graphmgr, prev, request, next);
			}
		}

		mutex_unlock(&graphmgr->glock);

p_noperiodic:
		TIMER_CHECK_POINT(5);
		graphmgr->tick_cnt++;
	}

	return 0;
}

static void vpu_graph_thread(struct kthread_work *work)
{
	int ret = 0;
	struct vpu_graphmgr *graphmgr;
	struct vpu_graph *graph;
	struct vpu_frame *frame;
	struct vpu_gframe *gframe, * temp;

	BUG_ON(!work);

	frame = container_of(work, struct vpu_frame, work);
	graph = frame->owner;
	graphmgr = graph->cookie;

	GRAPH_CHECK_POINT(1);
	switch (frame->message) {
	case VPU_FRAME_REQUEST:
		GRAPH_CHECK_POINT(2);
		ret = CALL_GOPS(graph, request, frame);
		if (ret) {
			vpu_err("CALL_GOPS(request) is fail(%d)\n", ret);
			BUG();
		}

		__vpu_gframe_free_head(graphmgr, &gframe);
		if (!gframe) {
			vpu_err("gframe is NULL\n");
			BUG();
		}

		frame->gindex = gframe->index;
		gframe->graph = graph;
		gframe->frame = frame;
		gframe->priority = graph->priority;
		gframe->ticks = graph->period_ticks;
		__vpu_gframe_trans_fre_to_rdy(graphmgr, gframe);
		GRAPH_CHECK_POINT(3);
		break;
	case VPU_CTRL_STOP:
		GRAPH_CHECK_POINT(4);
		list_for_each_entry_safe(gframe, temp, &graphmgr->grdy_list, list) {
			if (gframe->graph->id != graph->id)
				continue;

			ret = CALL_GOPS(graph, cancel, gframe->frame);
			if (ret) {
				vpu_err("CALL_GOPS(cancel) is fail(%d)\n", ret);
				BUG();
			}

			gframe->graph = NULL;
			gframe->frame = NULL;
			__vpu_gframe_trans_rdy_to_fre(graphmgr, gframe);
		}

		GRAPH_CHECK_POINT(5);
		mutex_lock(&graphmgr->glock);

		list_for_each_entry_safe(gframe, temp, &graphmgr->greq_list, list) {
			if (gframe->graph->id != graph->id)
				continue;

			ret = CALL_GOPS(graph, cancel, gframe->frame);
			if (ret) {
				vpu_err("CALL_GOPS(cancel) is fail(%d)\n", ret);
				BUG();
			}

			ret = CALL_GOPS(graph, put_resource, gframe->frame);
			if (ret) {
				vpu_err("CALL_GOPS(put_resource) is fail(%d)\n", ret);
				BUG();
			}

			gframe->graph = NULL;
			gframe->frame = NULL;
			__vpu_gframe_trans_req_to_fre(graphmgr, gframe);
		}

		mutex_unlock(&graphmgr->glock);
		GRAPH_CHECK_POINT(6);

		ret = CALL_GOPS(graph, control, frame);
		if (ret) {
			vpu_err("CALL_GOPS(control) is fail(%d)\n", ret);
			BUG();
		}
		return;
	default:
		BUG();
		break;
	}

	GRAPH_CHECK_POINT(7);
	__vpu_graphmgr_sched(graphmgr);
}

static void vpu_interface_thread(struct kthread_work *work)
{
	int ret = 0;
	u32 frame_index;
	u32 gframe_index;
	unsigned long flag;
	struct vpu_graphmgr *graphmgr;
	struct vpu_graph *graph;
	struct vpu_framemgr *iframemgr;
	struct vpu_frame *frame, *iframe;
	struct vpu_gframe *gframe;
	struct vpu_interface *interface;

	BUG_ON(!work);

	iframe = container_of(work, struct vpu_frame, work);
	interface = iframe->owner;
	iframemgr = &interface->framemgr;
	graphmgr = interface->cookie;

	GRAPH_CHECK_POINT(10);
	switch (iframe->message) {
	case VPU_FRAME_ALLOCATE:
		GRAPH_CHECK_POINT(11);
		frame_index = iframe->findex;
		gframe_index = iframe->gindex;

		if (gframe_index >= VPU_MAX_GFRAME) {
			vpu_err("gframe index(%d) is invalid\n", gframe_index);
			BUG();
		}

		if (frame_index >= VPU_MAX_FRAME) {
			vpu_err("frame index(%d) is invalid\n", frame_index);
			BUG();
		}

		gframe = &graphmgr->gframe[gframe_index];
		if (gframe->state != VPU_GFRAME_STATE_ALLOC) {
			vpu_err("gframe state is invalid(%d)\n", gframe->state);
			vpu_gframe_print(graphmgr);
			BUG();
		}

		graph = gframe->graph;
		if (!graph) {
			vpu_err("graph is NULL(%d)\n", gframe_index);
			BUG();
		}

		frame = gframe->frame;
		if (!frame) {
			vpu_err("frame is NULL(%d)\n", gframe_index);
			BUG();
		}

		frame->message = iframe->message;
		frame->param0 = iframe->param2;
		frame->param1 = iframe->param3;

		/* return status check */
		if (frame->param0) {
			vpu_err("allocation is fail(%ld, %ld)\n", frame->param0, frame->param1);

			ret = CALL_GOPS(graph, cancel, frame);
			if (ret) {
				vpu_err("CALL_GOPS(cancel) is fail(%d)\n", ret);
				BUG();
			}

			ret = CALL_GOPS(graph, put_resource, frame);
			if (ret) {
				vpu_err("CALL_GOPS(put_resource) is fail(%d)\n", ret);
				BUG();
			}

			/* gframe cleanup */
			mutex_lock(&graphmgr->glock);
			gframe->graph = NULL;
			gframe->frame = NULL;
			__vpu_gframe_trans_alc_to_fre(graphmgr, gframe);
			mutex_unlock(&graphmgr->glock);
		} else {
			/* gframe transition */
			mutex_lock(&graphmgr->glock);
			__vpu_gframe_trans_alc_to_pro(graphmgr, gframe);
			mutex_unlock(&graphmgr->glock);
		}

		/* iframe cleanup */
		framemgr_e_barrier_irqs(iframemgr, 0, flag);
		vpu_frame_trans_com_to_fre(iframemgr, iframe);
		framemgr_x_barrier_irqr(iframemgr, 0, flag);
		break;
	case VPU_FRAME_PROCESS:
		GRAPH_CHECK_POINT(12);
		frame_index = iframe->findex;
		gframe_index = iframe->gindex;

		if (gframe_index >= VPU_MAX_GFRAME) {
			vpu_err("gframe index(%d) is invalid\n", gframe_index);
			BUG();
		}

		if (frame_index >= VPU_MAX_FRAME) {
			vpu_err("frame index(%d) is invalid\n", frame_index);
			BUG();
		}

		gframe = &graphmgr->gframe[gframe_index];
		if (gframe->state != VPU_GFRAME_STATE_COMPLETE) {
			vpu_err("gframe state is invalid(%d)\n", gframe->state);
			vpu_gframe_print(graphmgr);
			BUG();
		}

		graph = gframe->graph;
		if (!graph) {
			vpu_err("graph is NULL(%d)\n", gframe_index);
			BUG();
		}

		frame = gframe->frame;
		if (!frame) {
			vpu_err("frame is NULL(%d)\n", gframe_index);
			BUG();
		}

		frame->message = iframe->message;
		frame->gindex = VPU_MAX_GFRAME;
		frame->param0 = iframe->param2;
		frame->param1 = iframe->param3;

#ifdef VPU_DYNAMIC_RESOURCE
		ret = CALL_GOPS(graph, put_resource, frame);
		if (ret) {
			vpu_err("CALL_GOPS(put_resource) is fail(%d)\n", ret);
			BUG();
		}
#endif

		ret = CALL_GOPS(graph, done, frame);
		if (ret) {
			vpu_err("CALL_GOPS(done) is fail(%d)\n", ret);
			BUG();
		}

		/* gframe cleanup */
		gframe->graph = NULL;
		gframe->frame = NULL;
		gframe->ticks = 0;
		__vpu_gframe_trans_com_to_fre(graphmgr, gframe);

		/* iframe cleanup */
		framemgr_e_barrier_irqs(iframemgr, 0, flag);
		vpu_frame_trans_com_to_fre(iframemgr, iframe);
		framemgr_x_barrier_irqr(iframemgr, 0, flag);
		break;
	default:
		BUG();
		break;
	}

	GRAPH_CHECK_POINT(13);
	__vpu_graphmgr_sched(graphmgr);
}

int vpu_graphmgr_grp_register(struct vpu_graphmgr *graphmgr, struct vpu_graph *graph)
{
	int ret = 0;
	u32 index;

	BUG_ON(!graphmgr);
	BUG_ON(!graph);

	mutex_lock(&graphmgr->mlock);
	for (index = 0; index < VPU_MAX_GRAPH; index++) {
		if (!graphmgr->graph[index]) {
			graphmgr->graph[index] = graph;
			graph->id = index;
			break;
		}
	}
	mutex_unlock(&graphmgr->mlock);

	if (index >= VPU_MAX_GRAPH) {
		vpu_err("graph slot is lack\n");
		ret = -EINVAL;
		goto p_err;
	}

	init_kthread_work(&graph->control.work, vpu_graph_thread);
	for (index = 0; index < VPU_MAX_FRAME; ++index)
		init_kthread_work(&graph->framemgr.frame[index].work, vpu_graph_thread);

	graph->global_lock = &graphmgr->mlock;

p_err:
	return ret;
}

int vpu_graphmgr_grp_unregister(struct vpu_graphmgr *graphmgr, struct vpu_graph *graph)
{
	int ret = 0;

	BUG_ON(!graphmgr);
	BUG_ON(!graph);

	mutex_lock(&graphmgr->mlock);
	graphmgr->graph[graph->id] = NULL;
	mutex_unlock(&graphmgr->mlock);

	return ret;
}

int vpu_graphmgr_grp_start(struct vpu_graphmgr *graphmgr, struct vpu_graph *graph)
{
	int ret = 0;

	if (test_bit(VS4L_GRAPH_FLAG_PERIODIC, &graph->flags))
		atomic_inc(&graphmgr->periodics);

	mutex_lock(&graphmgr->mlock);
	if (test_bit(VPU_GRAPHMGR_ENUM, &graphmgr->state)) {
		mutex_unlock(&graphmgr->mlock);
		goto p_skip_enum;
	}

	ret = __vpu_itf_enum(graphmgr->interface, graph);
	if (ret) {
		mutex_unlock(&graphmgr->mlock);
		vpu_err("__vpu_itf_enum is fail(%d)\n", ret);
		goto p_err;
	}

	set_bit(VPU_GRAPHMGR_ENUM, &graphmgr->state);
	mutex_unlock(&graphmgr->mlock);

p_skip_enum:
	ret = __vpu_itf_create(graphmgr->interface, graph);
	if (ret) {
		vpu_err("__vpu_itf_create is fail(%d)\n", ret);
		goto p_err;
	}

	ret = __vpu_itf_allocate(graphmgr->interface, graph);
	if (ret) {
		vpu_err("__vpu_itf_allocate is fail(%d)\n", ret);
		goto p_err;
	}

p_err:
	return ret;
}

int vpu_graphmgr_grp_stop(struct vpu_graphmgr *graphmgr, struct vpu_graph *graph)
{
	int ret = 0;

	if (test_bit(VS4L_GRAPH_FLAG_PERIODIC, &graph->flags))
		atomic_dec(&graphmgr->periodics);

	ret = __vpu_itf_destroy(graphmgr->interface, graph);
	if (ret) {
		vpu_err("__vpu_itf_destroy is fail(%d)\n", ret);
		goto p_err;
	}

p_err:
	return ret;
}

int vpu_graphmgr_itf_register(struct vpu_graphmgr *graphmgr, struct vpu_interface *interface)
{
	int ret = 0;
	u32 index;

	BUG_ON(!graphmgr);
	BUG_ON(!interface);

	graphmgr->interface = interface;
	for (index = 0; index < VPU_MAX_FRAME; ++index)
		init_kthread_work(&interface->framemgr.frame[index].work, vpu_interface_thread);

	return ret;
}

int vpu_graphmgr_itf_unregister(struct vpu_graphmgr *graphmgr, struct vpu_interface *interface)
{
	int ret = 0;

	BUG_ON(!graphmgr);
	BUG_ON(!interface);

	graphmgr->interface = NULL;

	return ret;
}

void vpu_graphmgr_queue(struct vpu_graphmgr *graphmgr, struct vpu_frame *frame)
{
	BUG_ON(!graphmgr);
	BUG_ON(!frame);

	queue_kthread_work(&graphmgr->worker, &frame->work);
}

int vpu_graphmgr_probe(struct vpu_graphmgr *graphmgr,
	struct vpu_resource *resource)
{
	int ret = 0;
	u32 index;
	struct vpu_device *device = container_of(graphmgr, struct vpu_device, graphmgr);

	BUG_ON(!graphmgr);
	BUG_ON(!device);

	graphmgr->tick_cnt = 0;
	graphmgr->tick_pos = 0;
	graphmgr->sched_cnt = 0;
	graphmgr->sched_pos = 0;
	graphmgr->task_graph = NULL;
	graphmgr->task_timer = NULL;
	graphmgr->resource = resource;
	atomic_set(&graphmgr->periodics, 0);
	mutex_init(&graphmgr->mlock);
	mutex_init(&graphmgr->glock);
	clear_bit(VPU_GRAPHMGR_OPEN, &graphmgr->state);
	clear_bit(VPU_GRAPHMGR_ENUM, &graphmgr->state);

	for (index = 0; index < VPU_MAX_GRAPH; ++index)
		graphmgr->graph[index] = NULL;

	INIT_LIST_HEAD(&graphmgr->gfre_list);
	INIT_LIST_HEAD(&graphmgr->grdy_list);
	INIT_LIST_HEAD(&graphmgr->greq_list);
	INIT_LIST_HEAD(&graphmgr->galc_list);
	INIT_LIST_HEAD(&graphmgr->gpro_list);
	INIT_LIST_HEAD(&graphmgr->gcom_list);

	graphmgr->gfre_cnt = 0;
	graphmgr->grdy_cnt = 0;
	graphmgr->greq_cnt = 0;
	graphmgr->galc_cnt = 0;
	graphmgr->gpro_cnt = 0;
	graphmgr->gcom_cnt = 0;

	for (index = 0; index < VPU_MAX_GFRAME; ++index) {
		graphmgr->gframe[index].index = index;
		graphmgr->gframe[index].graph = NULL;
		graphmgr->gframe[index].frame = NULL;
		graphmgr->gframe[index].state = VPU_GFRAME_STATE_INVALID;
		graphmgr->gframe[index].ticks = 0;
		__vpu_gframe_s_free(graphmgr, &graphmgr->gframe[index]);
	}

	return ret;
}

int vpu_graphmgr_open(struct vpu_graphmgr *graphmgr)
{
	int ret = 0;
	char name[30];
	struct sched_param param = { .sched_priority = MAX_RT_PRIO - 1 };

	BUG_ON(!graphmgr);

	init_kthread_worker(&graphmgr->worker);
	snprintf(name, sizeof(name), "vpu_graph");
	graphmgr->task_graph = kthread_run(kthread_worker_fn, &graphmgr->worker, name);
	if (IS_ERR_OR_NULL(graphmgr->task_graph)) {
		vpu_err("kthread_run is fail\n");
		ret = -EINVAL;
		goto p_err;
	}

	ret = sched_setscheduler_nocheck(graphmgr->task_graph, SCHED_FIFO, &param);
	if (ret) {
		vpu_err("sched_setscheduler_nocheck is fail(%d)\n", ret);
		goto p_err;
	}

	snprintf(name, sizeof(name), "vpu_timer");
	graphmgr->task_timer = kthread_run(vpu_time_thread, graphmgr, name);
	if (IS_ERR_OR_NULL(graphmgr->task_timer)) {
		vpu_err("kthread_run is fail\n");
		ret = -EINVAL;
		goto p_err;
	}

	ret = sched_setscheduler_nocheck(graphmgr->task_timer, SCHED_FIFO, &param);
	if (ret) {
		vpu_err("sched_setscheduler_nocheck is fail(%d)\n", ret);
		goto p_err;
	}

	set_bit(VPU_GRAPHMGR_OPEN, &graphmgr->state);

p_err:
	return ret;
}

int vpu_graphmgr_close(struct vpu_graphmgr *graphmgr)
{
	int ret = 0;
	struct vpu_gframe *gframe, *temp;

	BUG_ON(!graphmgr);

	if (graphmgr->grdy_cnt > 0) {
		ret++;
		vpu_err("ready gframe is NOT empty(%d)\n", graphmgr->grdy_cnt);

		list_for_each_entry_safe(gframe, temp, &graphmgr->grdy_list, list) {
			list_del(&gframe->list);
			graphmgr->grdy_cnt--;
			__vpu_gframe_s_free(graphmgr, gframe);
		}
	}

	if (graphmgr->greq_cnt > 0) {
		ret++;
		vpu_err("request gframe is NOT empty(%d)\n", graphmgr->greq_cnt);

		list_for_each_entry_safe(gframe, temp, &graphmgr->greq_list, list) {
			list_del(&gframe->list);
			graphmgr->greq_cnt--;
			__vpu_gframe_s_free(graphmgr, gframe);
		}
	}

	if (graphmgr->gpro_cnt > 0) {
		ret++;
		vpu_err("process gframe is NOT empty(%d)\n", graphmgr->gpro_cnt);

		list_for_each_entry_safe(gframe, temp, &graphmgr->gpro_list, list) {
			list_del(&gframe->list);
			graphmgr->gpro_cnt--;
			__vpu_gframe_s_free(graphmgr, gframe);
		}
	}

	kthread_stop(graphmgr->task_timer);
	kthread_stop(graphmgr->task_graph);

	clear_bit(VPU_GRAPHMGR_OPEN, &graphmgr->state);
	clear_bit(VPU_GRAPHMGR_ENUM, &graphmgr->state);

	return ret;
}
