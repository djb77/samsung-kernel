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

#ifndef SCORE_VERTEX_H_
#define SCORE_VERTEX_H_

#define SCORE_VERTEX_NAME			"vertex"
#define SCORE_VERTEX_MINOR			1

#include "vision-dev.h"
#include "vision-ioctl.h"
#include "score-memory.h"
#include "score-buftracker.h"
#include "score-framemgr.h"

struct score_vertex;
struct score_vertex_ctx;

enum score_vertex_state {
	SCORE_VERTEX_START,
	SCORE_VERTEX_STOP
};

struct score_vertex_refcount {
	atomic_t			refcount;
	struct score_vertex		*vertex;
	int				(*first)(struct score_vertex *vertex);
	int				(*final)(struct score_vertex *vertex);
};

struct score_vctx_ops {
	int (*get)(struct score_vertex_ctx *vctx);
	int (*put)(struct score_vertex_ctx *vctx);
	int (*start)(struct score_vertex_ctx *vctx);
	int (*stop)(struct score_vertex_ctx *vctx);
	int (*queue)(struct score_vertex_ctx *vctx, struct vs4l_container_list *clist);
	int (*deque)(struct score_vertex_ctx *vctx, struct vs4l_container_list *clist);
};

struct score_vertex {
	struct mutex			lock;
	struct vision_device		vd;
	struct score_vertex_refcount	open_cnt;
};

struct score_vertex_ctx {
	u32				state;
	u32				id;
	struct mutex			lock;
	struct score_vertex		*vertex;
	struct score_memory		*memory;
	struct score_vertexmgr		*vertexmgr;
	atomic_t			refcount;
	const struct score_vctx_ops	*vops;

	struct score_framemgr		framemgr;
	struct score_framemgr		*iframemgr;
	struct score_buftracker		buftracker;
	s32				blocking;
};

int score_vertex_probe(struct score_vertex *vertex, struct device *parent);
void score_vertex_release(struct score_vertex *vertex);

#define CALL_VOPS(g, op, ...)	(((g)->vops->op) ? ((g)->vops->op((g), ##__VA_ARGS__)) : 0)

#endif
