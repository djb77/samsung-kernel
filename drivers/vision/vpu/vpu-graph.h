/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#ifndef VPU_GRAPH_H_
#define VPU_GRAPH_H_

#include <linux/types.h>

#include "vs4l.h"
#include "vpu-framemgr.h"
#include "vpu-vertex.h"
#include "vpu-interface.h"
#include "vpuo-chain.h"

#define VPU_GRAPH_MAX_VERTEX		20
#define VPU_GRAPH_MAX_PRIORITY		20
#define VPU_GRAPH_MAX_INTERMEDIATE	64
#define VPU_GRAPH_MAX_LEVEL		20
#define VPU_GRAPH_STOP_TIMEOUT		(3 * HZ)

struct vpu_graph;

enum vpu_graph_state {
	VPU_GRAPH_STATE_CONFIG,
	VPU_GRAPH_STATE_HENROLL,
	VPU_GRAPH_STATE_HMAPPED,
	VPU_GRAPH_STATE_MMAPPED,
	VPU_GRAPH_STATE_START,
};

enum vpu_graph_flag {
	VPU_GRAPH_FLAG_UPDATE_PARAM = VS4L_GRAPH_FLAG_END
};

struct vpu_graph_ops {
	int (*control)(struct vpu_graph *graph, struct vpu_frame *frame);
	int (*request)(struct vpu_graph *graph, struct vpu_frame *frame);
	int (*process)(struct vpu_graph *graph, struct vpu_frame *frame);
	int (*cancel)(struct vpu_graph *graph, struct vpu_frame *frame);
	int (*done)(struct vpu_graph *graph, struct vpu_frame *frame);
	int (*get_resource)(struct vpu_graph *graph, struct vpu_frame *frame);
	int (*put_resource)(struct vpu_graph *graph, struct vpu_frame *frame);
	int (*update_param)(struct vpu_graph *graph, struct vpu_frame *frame);
};

struct vpu_graph_intermediate {
	u32				buffer_index;
	u32				*buffer;
	void				*handle;
	int				fd;
};

struct vpu_graph {
	u32				id;
	u32				uid;
	unsigned long			state;
	unsigned long			flags;
	u32				priority;
	u32				period_ticks;
	struct mutex			local_lock;
	struct mutex			*global_lock;

	/* for debugging */
	u32				input_cnt;
	u32				cancel_cnt;
	u32				done_cnt;
	u32				recent;
	u8				pu_map[VPU_PU_NUMBER];

	const struct vpu_graph_ops	*gops;

	void				*cookie;
	void				*resource;
	void				*memory;
	void				*exynos;
	struct vpu_pipe			*pipe;
	struct vpu_vertex_ctx 		vctx;

	u32				inhash[VPU_MAX_FRAME];
	u32				othash[VPU_MAX_FRAME];
	struct vpu_framemgr		framemgr;
	struct vpu_frame		control;
	wait_queue_head_t		control_wq;

	u32				size;
	struct vpul_task		*desc_utask;
	struct vpul_task		*desc_mtask;

	u32				update_cnt;
	void				*update_array;

	u32				vertex_cnt;
	u32				vertex_table[VPU_GRAPH_MAX_VERTEX];
	struct vpuo_vertex		*vertex_array[VPU_GRAPH_MAX_VERTEX];

	u32				inleaf_cnt;
	struct list_head		inleaf_list;

	u32				otleaf_cnt;
	struct list_head		otleaf_list;

	u32				lvl_cnt;
	struct list_head		lvl_list[VPU_GRAPH_MAX_LEVEL];

	u32				imbuffer_cnt;
	struct vpu_graph_intermediate	imbuffer_table[VPU_GRAPH_MAX_INTERMEDIATE];

	u32				iobuffer_cnt;
	u32				iobuffer_idx[VPUL_MAX_TASK_EXTERNAL_RAMS];
	u32				iobuffer_dat[VPUL_MAX_TASK_EXTERNAL_RAMS];
};

void vpu_graph_print(struct vpu_graph *graph);
struct vpuo_chain * vpu_graph_g_chain(struct vpu_graph *graph, u32 chain_id);
int vpu_graph_create(struct vpu_graph **graph, void *cookie, void *resource, void *memory, void *exynos);
int vpu_graph_destroy(struct vpu_graph *graph);
int vpu_graph_config(struct vpu_graph *graph, struct vs4l_graph *info);
int vpu_graph_param(struct vpu_graph *graph, struct vs4l_param_list *plist);

#define CALL_GOPS(g, op, ...)	(((g)->gops->op) ? ((g)->gops->op(g, ##__VA_ARGS__)) : 0)

#endif
