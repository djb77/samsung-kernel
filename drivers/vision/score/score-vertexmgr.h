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

#ifndef SCORE_VERTEXMGR_H_
#define SCORE_VERTEXMGR_H_

#include <linux/kthread.h>
#include <linux/types.h>

#include "score-framemgr.h"
#include "score-vertex.h"
#include "score-interface.h"

struct score_vertexmgr {
	struct kthread_worker		worker;
	struct task_struct		*task_vertex;
	DECLARE_BITMAP(vid_map, SCORE_MAX_VERTEX);

	struct score_interface		*interface;
};

int score_vertexmgr_probe(struct score_vertexmgr *vertexmgr);
void score_vertexmgr_remove(struct score_vertexmgr *vertexmgr);
int score_vertexmgr_open(struct score_vertexmgr *vertexmgr);
int score_vertexmgr_close(struct score_vertexmgr *vertexmgr);

int score_vertexmgr_vctx_register(struct score_vertexmgr *vertexmgr,
		struct score_vertex_ctx *vctx);
int score_vertexmgr_vctx_unregister(struct score_vertexmgr *vertexmgr,
		struct score_vertex_ctx *vctx);
int score_vertexmgr_itf_register(struct score_vertexmgr *vertexmgr,
		struct score_interface *interface);
int score_vertexmgr_itf_unregister(struct score_vertexmgr *vertexmgr,
		struct score_interface *interface);

int score_vertexmgr_queue(struct score_vertexmgr *vertexmgr, struct score_frame *frame);
void score_vertexmgr_queue_pending_frame(struct score_vertexmgr *vertexmgr);

#endif
