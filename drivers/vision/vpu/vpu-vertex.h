/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef VPU_VERTEX_H_
#define VPU_VERTEX_H_

#define VPU_VERTEX_NAME			"vertex"
#define VPU_VERTEX_MINOR		0

#include "vision-dev.h"
#include "vision-ioctl.h"
#include "vpu-queue.h"

struct vpu_vertex;

enum vpu_vertex_state {
	VPU_VERTEX_OPEN,
	VPU_VERTEX_GRAPH,
	VPU_VERTEX_FORMAT,
	VPU_VERTEX_START,
	VPU_VERTEX_STOP
};

struct vpu_vertex_refcount {
	atomic_t			refcount;
	struct vpu_vertex		*vertex;
	int				(*first)(struct vpu_vertex *vertex);
	int				(*final)(struct vpu_vertex *vertex);
};

struct vpu_vertex {
	struct mutex			lock;
	struct vision_device		vd;
	struct vpu_vertex_refcount	open_cnt;
	struct vpu_vertex_refcount	start_cnt;
};

struct vpu_vertex_ctx {
	u32				state;
	u32				id;
	struct mutex			lock;
	struct vpu_queue		queue;
	struct vpu_vertex		*vertex;
};

int vpu_vertex_probe(struct vpu_vertex *vertex, struct device *parent);

#endif