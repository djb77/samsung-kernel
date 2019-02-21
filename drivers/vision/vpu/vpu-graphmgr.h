/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#ifndef VPU_GRAPHMGR_H_
#define VPU_GRAPHMGR_H_

#include <linux/kthread.h>
#include <linux/types.h>

#include "vpu-framemgr.h"
#include "vpu-graph.h"
#include "vpu-interface.h"
#include "vpu-resource.h"

#define VPU_MAX_GFRAME			(VPU_MAX_GRAPH * 2)

enum vpu_gframe_state {
	VPU_GFRAME_STATE_INVALID,
	VPU_GFRAME_STATE_FREE,
	VPU_GFRAME_STATE_READY,
	VPU_GFRAME_STATE_REQUEST,
	VPU_GFRAME_STATE_ALLOC,
	VPU_GFRAME_STATE_PROCESS,
	VPU_GFRAME_STATE_COMPLETE
};

struct vpu_gframe {
	struct list_head		list;
	u32				index;
	u32				priority;
	struct vpu_graph		*graph;
	struct vpu_frame		*frame;
	u32				ticks;
	u32				state;
};

enum vpu_graphmgr_state {
	VPU_GRAPHMGR_OPEN,
	VPU_GRAPHMGR_ENUM
};

enum vpu_graphmgr_client {
	VPU_GRAPHMGR_CLIENT_GRAPH = 1,
	VPU_GRAPHMGR_CLIENT_INTERFACE
};

struct vpu_graphmgr {
	struct vpu_graph		*graph[VPU_MAX_GRAPH];
	atomic_t			periodics;
	unsigned long			state;
	struct mutex			mlock;

	u32				tick_cnt;
	u32				tick_pos;
	u32				sched_cnt;
	u32				sched_pos;
	struct kthread_worker		worker;
	struct task_struct		*task_graph;
	struct task_struct		*task_timer;

	struct vpu_resource		*resource;
	struct vpu_interface		*interface;

	struct mutex			glock;
	struct vpu_gframe		gframe[VPU_MAX_GFRAME];
	struct list_head		gfre_list;
	struct list_head		grdy_list;
	struct list_head		greq_list;
	struct list_head		galc_list;
	struct list_head		gpro_list;
	struct list_head		gcom_list;
	u32				gfre_cnt;
	u32				grdy_cnt;
	u32				greq_cnt;
	u32				galc_cnt;
	u32				gpro_cnt;
	u32				gcom_cnt;
};

void vpu_gframe_print(struct vpu_graphmgr *graphmgr);
int vpu_graphmgr_probe(struct vpu_graphmgr *graphmgr, struct vpu_resource *resource);
int vpu_graphmgr_open(struct vpu_graphmgr *graphmgr);
int vpu_graphmgr_close(struct vpu_graphmgr *graphmgr);
int vpu_graphmgr_grp_register(struct vpu_graphmgr *graphmgr, struct vpu_graph *graph);
int vpu_graphmgr_grp_unregister(struct vpu_graphmgr *graphmgr, struct vpu_graph *graph);
int vpu_graphmgr_grp_start(struct vpu_graphmgr *graphmgr, struct vpu_graph *graph);
int vpu_graphmgr_grp_stop(struct vpu_graphmgr *graphmgr, struct vpu_graph *graph);
int vpu_graphmgr_itf_register(struct vpu_graphmgr *graphmgr, struct vpu_interface *interface);
int vpu_graphmgr_itf_unregister(struct vpu_graphmgr *graphmgr, struct vpu_interface *interface);
void vpu_graphmgr_queue(struct vpu_graphmgr *graphmgr, struct vpu_frame *frame);
#endif
