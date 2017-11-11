/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/delay.h>
#include <linux/slab.h>

#include "lib/vpul-ds.h"
#include "vpu-graphmgr.h"
#include "vpu-graph.h"
#include "vpuo-vertex.h"
#include "vpuo-chain.h"
#include "vpu-debug.h"
#include "vpu-exynos.h"
#include "vs4l.h"

const struct vpu_graph_ops vpu_graph_ops;

static struct vpuo_pu * __vpu_graph_find_iopu(struct list_head *list, u32 target)
{
	struct vpuo_pu *pu, *temp;

	list_for_each_entry_safe(pu, temp, list, gleaf_entry) {
		if (pu->target == target)
			return pu;
	}

	return NULL;
}

static struct vpuo_pu * __vpu_graph_find_pu(struct vpu_graph *graph, u32 target)
{
	struct vpuo_vertex *vertex;
	struct vpuo_chain *chain;
	struct vpuo_pu *pu;
	u32 chain_id, pu_id;
	u32 i, j;

	pu = NULL;
	chain_id = (target >> VS4L_TARGET_SC_SHIFT) & VS4L_TARGET_SC;
	pu_id = (target >> VS4L_TARGET_PU_SHIFT) & VS4L_TARGET_PU;

	for (i = 0; i < graph->vertex_cnt; ++i) {
		vertex = graph->vertex_array[i];
		for (j = 0; j < vertex->chain_cnt; ++j) {
			chain = vertex->chain_array[j];

			if (chain->id != chain_id)
				continue;

			pu = chain->pu_table[pu_id];
		}
	}

	return pu;
}

static void __vpu_graph_connect_chain(struct vpuo_chain *src_chain, struct vpuo_chain *dst_chain)
{
	u32 i, connected = 0;

	for (i = 0; i < src_chain->otchain_cnt; ++i) {
		if (src_chain->otchain[i] == dst_chain)
			connected++;
	}

	if (!connected) {
		src_chain->otchain[src_chain->otchain_cnt] = dst_chain;
		src_chain->otchain_cnt++;

		dst_chain->inchain[dst_chain->inchain_cnt] = src_chain;
		dst_chain->inchain_cnt++;

		BUG_ON(src_chain->otchain_cnt >= VPUO_CHAIN_MAX_PORT);
		BUG_ON(dst_chain->inchain_cnt >= VPUO_CHAIN_MAX_PORT);
	}
}

static int __vpu_graph_print(struct vpu_graph *graph)
{
	DLOG_INIT();
	u32 i, j;
	struct list_head *lvl_list, *list;
	struct vpuo_vertex *vertex, *vtemp;
	struct vpuo_chain *chain;
	struct vpuo_pu *pu, *ptemp;

	BUG_ON(!graph);
	BUG_ON(!graph->global_lock);

	mutex_lock(graph->global_lock);

	vpu_iinfo("[GRAPH : %d]\n", graph, graph->uid);

	lvl_list = graph->lvl_list;
	if (!lvl_list) {
		vpu_ierr("lvl_list is NULL\n", graph);
		mutex_unlock(graph->global_lock);
		return -EINVAL;
	}

	for (i = 0; i < VPU_GRAPH_MAX_LEVEL; ++i) {
		if (list_empty(&lvl_list[i]))
			continue;

		DLOG("lvl%d : ", i);
		list_for_each_entry_safe(vertex, vtemp, &lvl_list[i], level_entry) {
			DLOG("%d(", vertex->id);

			if (vertex->otvertex_cnt >= 1) {
				for (j = 0; j < (vertex->otvertex_cnt - 1); ++j)
					DLOG("%d ", vertex->otvertex[j]->id);
				DLOG("%d)  ", vertex->otvertex[j]->id);
			} else {
				DLOG(")  ");
			}
		}

		vpu_info("%s\n", DLOG_OUT());
	}

	for (i = 0; i < VPU_GRAPH_MAX_LEVEL; ++i) {
		if (list_empty(&lvl_list[i]))
			continue;

		list_for_each_entry_safe(vertex, vtemp, &lvl_list[i], level_entry) {
			vpuo_vertex_print(vertex);
		}
	}

	DLOG("in :");
	list = &graph->inleaf_list;
	list_for_each_entry_safe(pu, ptemp, list, gleaf_entry) {
		chain = pu->parent;
		DLOG(" %d(%d)", pu->id, chain->id);
	}
	vpu_info("%s\n", DLOG_OUT());

	DLOG("ot :");
	list = &graph->otleaf_list;
	list_for_each_entry_safe(pu, ptemp, list, gleaf_entry) {
		chain = pu->parent;
		DLOG(" %d(%d)", pu->id, chain->id);
	}
	vpu_info("%s\n", DLOG_OUT());

	mutex_unlock(graph->global_lock);

	return 0;
}

static int __vpu_graph_resource_get(struct vpu_graph *graph)
{
	int ret = 0;

	if (test_bit(VPU_GRAPH_STATE_HENROLL, &graph->state)) {
		vpu_ierr("resource is already refered\n", graph);
		ret = -EINVAL;
		goto p_err;
	}

	ret = vpu_resource_add(graph->resource, graph->desc_utask);
	if (ret) {
		vpu_ierr("vpu_resource_add ia fail(%d)\n", graph, ret);
		goto p_err;
	}

	set_bit(VPU_GRAPH_STATE_HENROLL, &graph->state);

	if (test_bit(VPU_GRAPH_STATE_HMAPPED, &graph->state)) {
		vpu_ierr("resource is already mapped\n", graph);
		ret = -EINVAL;
		goto p_err;
	}

	ret = vpu_resource_get(graph->resource, graph->desc_mtask, graph->flags);
	if (ret) {
		vpu_ierr("vpu_resource_get ia fail(%d)\n", graph, ret);
		goto p_err;
	}

	set_bit(VPU_GRAPH_STATE_HMAPPED, &graph->state);

p_err:
	return ret;
}

static int __vpu_graph_resource_put(struct vpu_graph *graph)
{
	int ret = 0;

	if (test_bit(VPU_GRAPH_STATE_HENROLL, &graph->state)) {
		ret = vpu_resource_del(graph->resource, graph->desc_utask);
		if (ret)
			vpu_ierr("vpu_resource_del ia fail(%d)\n", graph, ret);
	}

	clear_bit(VPU_GRAPH_STATE_HENROLL, &graph->state);

	if (test_bit(VPU_GRAPH_STATE_HMAPPED, &graph->state)) {
		ret = vpu_resource_put(graph->resource, graph->desc_mtask);
		if (ret)
			vpu_ierr("vpu_resource_put ia fail(%d)\n", graph, ret);
	}

	clear_bit(VPU_GRAPH_STATE_HMAPPED, &graph->state);

	return ret;
}

static int __vpu_graph_start(struct vpu_graph *graph)
{
	int ret = 0;

	BUG_ON(!graph);

	if (test_bit(VPU_GRAPH_STATE_START, &graph->state))
		return 0;

	ret = __vpu_graph_resource_get(graph);
	if (ret) {
		vpu_ierr("__vpu_graph_resource_get is fail(%d)\n", graph, ret);
		goto p_err;
	}

	ret = vpu_graphmgr_grp_start(graph->cookie, graph);
	if (ret) {
		vpu_ierr("vpu_graphmgr_grp_start is fail(%d)\n", graph, ret);
		goto p_err;
	}

	set_bit(VPU_GRAPH_STATE_START, &graph->state);

p_err:
	return ret;
}

static int __vpu_graph_stop(struct vpu_graph *graph)
{
	int ret = 0, errcnt = 0;
	u32 retry, timeout;
	struct vpu_framemgr *framemgr;
	struct vpu_frame *control;

	if (!test_bit(VPU_GRAPH_STATE_START, &graph->state))
		return 0;

	framemgr = &graph->framemgr;
	if (framemgr->req_cnt + framemgr->pre_cnt) {
		control = &graph->control;
		control->message = VPU_CTRL_STOP;
		vpu_graphmgr_queue(graph->cookie, control);
		timeout = wait_event_timeout(graph->control_wq,
			control->message == VPU_CTRL_STOP_DONE, VPU_GRAPH_STOP_TIMEOUT);
		if (!timeout) {
			vpu_ierr("wait_event_timeout is expired\n", graph);
			errcnt++;
		}
	}

	retry = VPU_STOP_WAIT_COUNT;
	while (--retry && framemgr->req_cnt) {
		vpu_iwarn("waiting %d request cancel...(%d)\n", graph, framemgr->req_cnt, retry);
		msleep(10);
	}

	if (!retry) {
		vpu_ierr("request cancel is fail\n", graph);
		errcnt++;
	}

	retry = VPU_STOP_WAIT_COUNT;
	while (--retry && framemgr->pre_cnt) {
		vpu_iwarn("waiting %d prepare cancel...(%d)\n", graph, framemgr->pre_cnt, retry);
		msleep(10);
	}

	if (!retry) {
		vpu_ierr("prepare cancel is fail\n", graph);
		errcnt++;
	}

	retry = VPU_STOP_WAIT_COUNT;
	while (--retry && framemgr->pro_cnt) {
		vpu_iwarn("waiting %d process done...(%d)\n", graph, framemgr->pro_cnt, retry);
		msleep(10);
	}

	if (!retry) {
		vpu_ierr("process done is fail\n", graph);
		errcnt++;
	}

	ret = __vpu_graph_resource_put(graph);
	if (ret) {
		vpu_ierr("__vpu_graph_resource_put is fail(%d)\n", graph, ret);
		errcnt++;
	}

	ret = vpu_graphmgr_grp_stop(graph->cookie, graph);
	if (ret) {
		vpu_ierr("vpu_graphmgr_grp_stop is fail(%d)\n", graph, ret);
		errcnt++;
	}

	vpu_frame_flush(framemgr);
	/* this copy is necessary for resetting task info otherwise execution of the task will be timeout */
	memcpy(graph->desc_mtask, graph->desc_utask, graph->size);

	clear_bit(VPU_GRAPH_STATE_START, &graph->state);

	return errcnt;
}

static int __vpu_graph_unmap(struct vpu_graph *graph)
{
	int ret = 0;
	struct vpu_memory_buffer *buffer;
	u32 i;

	for (i = 0; i < graph->imbuffer_cnt; ++i) {
		buffer = graph->imbuffer_table[i].handle;
		if (!buffer) {
			vpu_ierr("buffer is NULL(%d)\n", graph, i);
			continue;
		}

		ret = vpu_memory_unmap(graph->memory, buffer);
		if (ret)
			vpu_ierr("vpu_memory_unmap is fail(%d)\n", graph, ret);

		graph->imbuffer_table[i].fd = 0;
		graph->imbuffer_table[i].handle = NULL;
		kfree(buffer);
	}

	graph->imbuffer_cnt = 0;
	clear_bit(VPU_GRAPH_STATE_MMAPPED, &graph->state);

	return ret;
}

static int __vpu_graph_map(struct vpu_graph *graph)
{
	int ret = 0;
	struct vpul_task *task;
	struct vpu_memory_buffer *buffer;
	u32 i, j;

	if (test_bit(VPU_GRAPH_STATE_MMAPPED, &graph->state)) {
		vpu_iwarn("graph is already mapped\n", graph);
		ret = -EINVAL;
		goto p_err;
	}

	task = graph->desc_mtask;
	graph->imbuffer_cnt = 0;
	graph->iobuffer_cnt = 0;

	for (i = 0; i < task->n_external_mem_addresses; ++i) {
		if (task->external_mem_addr[i]) {
			buffer = kzalloc(sizeof(struct vpu_memory_buffer), GFP_KERNEL);
			if (!buffer) {
				vpu_err("kzalloc is fail\n");
				ret = -ENOMEM;
				goto p_err;
			}

			buffer->fd = task->external_mem_addr[i];

			ret = vpu_memory_map(graph->memory, buffer);
			if (ret) {
				vpu_ierr("vpu_memory_map(%d) is fail(%d)\n", graph, i, ret);
				for (j = 0; j < task->n_external_mem_addresses; ++j)
					vpu_info("[DUMP EXT%d] 0x%X\n", j, task->external_mem_addr[j]);
				goto p_err;
			}

			if (buffer->dvaddr < VPU_AHB_BASE_ADDR) {
				vpu_ierr("dvaddr is invalid(%pa)\n", graph, &buffer->dvaddr);
				ret = -EINVAL;
				goto p_err;
			}

			vpu_iinfo("imbuffer[%d] : %d(%d) -> %pa\n", graph, graph->imbuffer_cnt,
				buffer->fd, i, &buffer->dvaddr);

			task->external_mem_addr[i] = buffer->dvaddr - VPU_AHB_BASE_ADDR;
			graph->imbuffer_table[graph->imbuffer_cnt].buffer_index = i;
			graph->imbuffer_table[graph->imbuffer_cnt].buffer = &task->external_mem_addr[i];
			graph->imbuffer_table[graph->imbuffer_cnt].handle = buffer;
			graph->imbuffer_table[graph->imbuffer_cnt].fd = buffer->fd;
			graph->imbuffer_cnt++;

			BUG_ON(graph->imbuffer_cnt >= VPU_GRAPH_MAX_INTERMEDIATE);
		} else {
			graph->iobuffer_idx[graph->iobuffer_cnt] = i;
			graph->iobuffer_dat[graph->iobuffer_cnt] = 0;
			graph->iobuffer_cnt++;
		}
	}

	set_bit(VPU_GRAPH_STATE_MMAPPED, &graph->state);
	return 0;

p_err:
	__vpu_graph_unmap(graph);
	return ret;
}

static int __vpu_graph_alloc(struct vpu_graph *graph)
{
	int ret = 0;
	char *desc_ubase, *desc_mbase;
	u32 i, vertex_cnt;
	struct vpul_task *desc_utask, *desc_mtask;
	struct vpul_vertex *desc_uvertex, *desc_mvertex;
	struct vpuo_vertex *vertex;

	if (test_bit(VS4L_GRAPH_FLAG_PRIMITIVE, &graph->flags))
		return 0;

	desc_utask = graph->desc_utask;
	desc_mtask = graph->desc_mtask;
	desc_ubase = (char *)desc_utask;
	desc_mbase = (char *)desc_mtask;

	/* 1. checking data integration */
	if ((desc_ubase[graph->size - 4] != 'V') ||
		(desc_ubase[graph->size - 3] != 'S') ||
		(desc_ubase[graph->size - 2] != '4') ||
		(desc_ubase[graph->size - 1] != 'L')) {
		vpu_ierr("magic number(%c %c %c %c) is invalid\n", graph,
			desc_ubase[graph->size - 4], desc_ubase[graph->size - 3],
			desc_ubase[graph->size - 2], desc_ubase[graph->size - 1]);
		ret = -EINVAL;
		goto p_err;
	}

	if (desc_utask->id == 0) {
		vpu_ierr("task id is ZERO\n", graph);
		ret = -EINVAL;
		goto p_err;
	}

	vertex_cnt = desc_utask->t_num_of_vertices;
	if (!vertex_cnt) {
		vpu_ierr("vertex_cnt is ZERO\n", graph);
		ret = -EINVAL;
		goto p_err;
	}

	if (vertex_cnt >= VPU_GRAPH_MAX_VERTEX) {
		vpu_ierr("vertex_cnt is invalid(%d)\n", graph, vertex_cnt);
		ret = -EINVAL;
		goto p_err;
	}

	/* 2. allocation chain & aquire sequential chain information */
	desc_uvertex = (struct vpul_vertex *)(desc_ubase + desc_utask->vertices_vec_ofs);
	desc_mvertex = (struct vpul_vertex *)(desc_mbase + desc_mtask->vertices_vec_ofs);
	for (i = 0; i < vertex_cnt; ++i) {
		ret = vpuo_vertex_create(&vertex, desc_ubase, desc_mbase, &desc_uvertex[i], &desc_mvertex[i], graph);
		if (ret) {
			vpu_ierr("vpuo_vertex_create is fail(%d)\n", graph, ret);
			ret = -EINVAL;
			goto p_err;
		}

		if (!vertex) {
			vpu_ierr("vertex is NULL\n", graph);
			ret = -EINVAL;
			goto p_err;
		}

		graph->vertex_array[graph->vertex_cnt] = vertex;
		graph->vertex_table[graph->vertex_cnt] = i;
		graph->vertex_cnt++;

		BUG_ON(graph->vertex_cnt >= VPU_GRAPH_MAX_VERTEX);
	}


	graph->update_cnt = desc_utask->t_num_of_pu_params_on_invoke;
	if (graph->update_cnt) {
		u32 update_size = graph->update_cnt * sizeof(union vpul_pu_parameters);

		graph->update_array = kmalloc(update_size, GFP_KERNEL);
		if (!graph->update_array) {
			vpu_ierr("kmalloc is fail(%d)\n", graph, update_size);
			goto p_err;
		}
	}

p_err:
	return ret;
}

static int __vpu_graph_free(struct vpu_graph *graph)
{
	int ret = 0;
	u32 i, vertex_cnt;
	struct vpuo_vertex *vertex;

	if (test_bit(VS4L_GRAPH_FLAG_PRIMITIVE, &graph->flags))
		return 0;

	kfree(graph->update_array);
	graph->update_cnt = 0;

	vertex_cnt = graph->vertex_cnt;
	for (i = 0; i < vertex_cnt; ++i) {
		vertex = graph->vertex_array[i];

		ret = vpuo_vertex_destroy(vertex);
		if (ret) {
			vpu_err("vpuo_vertex_destroy is fail(%d)\n", ret);
			continue;
		}

		graph->vertex_array[i] = NULL;
		graph->vertex_table[i] = 0;
		graph->vertex_cnt--;
	}

	return ret;
}

static int __vpu_graph_parse(struct vpu_graph *graph)
{
	int ret = 0;
	struct list_head *lvl_list;
	struct list_head *inleaf_list, *otleaf_list;
	struct vpul_vertex *desc_src;
	struct vpul_edge *desc_uedge;
	struct vpuo_vertex *vertex, *src, *dst, *target, *temp;
	struct vpuo_chain *chain, *src_chain, *dst_chain;
	struct vpuo_pu *inpu, *inpu2, *otpu, *t1, *t2, *pu;
	u32 vertex_cnt,  chain_cnt, pu_cnt;
	u32 lvl_vertex_cnt, edge_cnt;
	u32 i, j, k;
	u32 matched;

	if (test_bit(VS4L_GRAPH_FLAG_PRIMITIVE, &graph->flags))
		return 0;

	/* 1. parsing each vertex */
	vertex_cnt = graph->vertex_cnt;
	for (i = 0; i < vertex_cnt; ++i) {
		vertex = graph->vertex_array[i];
		if (!vertex) {
			vpu_ierr("vertex is NULL(%d)\n", graph, i);
			BUG();
		}

		ret = vpuo_vertex_parse(vertex);
		if (ret) {
			vpu_ierr("vpuo_vertex_parse is fail(%d)\n", graph, ret);
			goto p_err;
		}
	}

	/* 2. weaving */
	for (i = 0; i < vertex_cnt; ++i) {
		src = graph->vertex_array[i];
		if (!src) {
			vpu_ierr("src is NULL(%d)\n", graph, i);
			BUG();
		}

		desc_src = src->desc_uvertex;
		if (!desc_src) {
			vpu_ierr("desc_uvertex is NULL(%d)\n", graph, i);
			BUG();
		}

		edge_cnt = desc_src->n_out_edges;
		desc_uedge = desc_src->out_edges;
		for (j = 0; j < edge_cnt; ++j) {
			if (desc_uedge[j].dst_vtx_idx >= vertex_cnt) {
				vpu_ierr("dst vertex index(%d) is invalid\n", graph, desc_uedge[j].dst_vtx_idx);
				BUG();
			}

			dst = graph->vertex_array[desc_uedge[j].dst_vtx_idx];
			if (!dst) {
				vpu_ierr("dst is NULL(%d)\n", graph, i);
				BUG();
			}

			src->otvertex[src->otvertex_cnt] = dst;
			src->otvertex_cnt++;
			dst->invertex[dst->invertex_cnt] = src;
			dst->invertex_cnt++;

			BUG_ON(src->otvertex_cnt >= VPU_VERTEX_MAX_PORT);
			BUG_ON(dst->invertex_cnt >= VPU_VERTEX_MAX_PORT);
		}
	}

	/* 3. stitching */
	for (i = 0; i < vertex_cnt; ++i) {
		src = graph->vertex_array[i];
		if (!src) {
			vpu_err("src is NULL\n");
			BUG();
		}

		for (j = 0; j < src->otvertex_cnt; ++j) {
			dst = src->otvertex[j];
			if (!dst) {
				vpu_err("dst is NULL\n");
				BUG();
			}

			otleaf_list = &src->otleaf_list;
			inleaf_list = &dst->inleaf_list;
			list_for_each_entry_safe(otpu, t1, otleaf_list, vleaf_entry) {
				if (otpu->buffer_cnt == 0)
					continue;

				list_for_each_entry_safe(inpu, t2, inleaf_list, vleaf_entry) {
					/* HACK : search buffer index only at last and first layer */
					for (k = 0; k < dst->layers; ++k) {
						matched = 0;

						if ((otpu->buffer[otpu->buffer_cnt - 1] == inpu->buffer[k]) &&
							(inpu->buffer[k] != 0))
							matched = 1;

						if (otpu->buffer_idx[otpu->buffer_cnt - 1] == inpu->buffer_idx[k])
							matched =  1;

						if (matched) {
							inpu->buffer_shm[k]++;
							inpu->inpu[inpu->inpu_cnt] = otpu;
							inpu->inpu_cnt++;
							if (inpu->inpu_cnt >= inpu->buffer_cnt)
								set_bit(VPUO_PU_STATE_IM, &inpu->state);

							otpu->buffer_shm[otpu->buffer_cnt - 1]++;
							otpu->otpu[otpu->otpu_cnt] = inpu;
							otpu->otpu_cnt++;
							if (otpu->otpu_cnt >= otpu->buffer_cnt)
								set_bit(VPUO_PU_STATE_IM, &otpu->state);

							src_chain = otpu->parent;
							dst_chain = inpu->parent;
							__vpu_graph_connect_chain(src_chain, dst_chain);

							BUG_ON(inpu->inpu_cnt >= VPUO_PU_MAX_PORT);
							BUG_ON(otpu->otpu_cnt >= VPUO_PU_MAX_PORT);
						}
					}
				}
			}
		}
	}

	/* 4. leveling */
	lvl_vertex_cnt = 0;
	graph->lvl_cnt = 0;
	lvl_list = graph->lvl_list;
	for (i = 0; i < VPU_GRAPH_MAX_LEVEL; ++i)
		INIT_LIST_HEAD(&lvl_list[i]);

	for (i = 0; i < vertex_cnt; ++i) {
		vertex = graph->vertex_array[i];
		if (!vertex) {
			vpu_ierr("vertex is NULL\n", graph);
			BUG();
		}

		if (!vertex->invertex_cnt) {
			list_add_tail(&vertex->level_entry, &lvl_list[0]);
			vertex->level = 0;
			lvl_vertex_cnt++;
		}
	}

	if (!lvl_vertex_cnt) {
		vpu_ierr("the number of first level entry is zero\n", graph);
		ret = -EINVAL;
		goto p_err;
	}

	for (i = 0; i < (VPU_GRAPH_MAX_LEVEL - 1); ++i) {
		if (list_empty(&lvl_list[i]))
			break;

		list_for_each_entry_safe(vertex, temp, &lvl_list[i], level_entry) {
			for (j = 0; j < vertex->invertex_cnt; ++j) {
				target = vertex->invertex[j];
				if (!target) {
					vpu_ierr("target is NULL\n", graph);
					BUG();
				}

				if (target->level >= i) {
					list_del(&vertex->level_entry);
					lvl_vertex_cnt--;

					list_add_tail(&vertex->level_entry, &lvl_list[i + 1]);
					vertex->level = i + 1;
					lvl_vertex_cnt++;
					break;
				}
			}

			if (vertex->level != i)
				continue;

			for (j = 0; j < vertex->otvertex_cnt; ++j) {
				target = vertex->otvertex[j];
				if (!target) {
					vpu_ierr("target is NULL\n", graph);
					BUG();
				}

				/* if already is added */
				if (target->level < VPU_GRAPH_MAX_LEVEL)
					continue;

				list_add_tail(&target->level_entry, &lvl_list[i + 1]);
				target->level = i + 1;
				lvl_vertex_cnt++;
			}
		}
	}

	if (lvl_vertex_cnt != vertex_cnt) {
		vpu_ierr("connection is invalid(%d, %d)\n", graph, lvl_vertex_cnt, vertex_cnt);
		ret = -EINVAL;
		goto p_err;
	}

	graph->lvl_cnt = i;

	/* 5. pick up leaf */
	graph->inleaf_cnt = 0;
	INIT_LIST_HEAD(&graph->inleaf_list);
	graph->otleaf_cnt = 0;
	INIT_LIST_HEAD(&graph->otleaf_list);

	for (i = 0; i < vertex_cnt; ++i) {
		vertex = graph->vertex_array[i];
		if (!vertex) {
			vpu_err("vertex is NULL\n");
			BUG();
		}

		inleaf_list = &vertex->inleaf_list;
		list_for_each_entry_safe(inpu, t1, inleaf_list, vleaf_entry) {
			if (test_bit(VPUO_PU_STATE_IM, &inpu->state))
				continue;

			graph->inleaf_cnt++;
			list_add_tail(&inpu->gleaf_entry, &graph->inleaf_list);
		}

		otleaf_list = &vertex->otleaf_list;
		list_for_each_entry_safe(otpu, t2, otleaf_list, vleaf_entry) {
			if (test_bit(VPUO_PU_STATE_IM, &otpu->state))
				continue;

			graph->otleaf_cnt++;
			list_add_tail(&otpu->gleaf_entry, &graph->otleaf_list);
		}
	}

	/* 6. remove duplicated io */
	inleaf_list = &graph->inleaf_list;
	list_for_each_entry_safe(inpu, t1, inleaf_list, gleaf_entry) {
		if (!test_bit(VPUO_PU_STATE_IN, &inpu->state))
			continue;

		inpu2 = list_next_entry(inpu, gleaf_entry);
		list_for_each_entry_safe_from(inpu2, t2, inleaf_list, gleaf_entry) {
			if (!test_bit(VPUO_PU_STATE_IN, &inpu2->state))
				continue;

			if (inpu->buffer_idx[0] == inpu2->buffer_idx[0])
				clear_bit(VPUO_PU_STATE_IN, &inpu2->state);
		}
	}

	list_for_each_entry_safe(inpu, t1, inleaf_list, gleaf_entry) {
		if (!test_bit(VPUO_PU_STATE_IN, &inpu->state)) {
			list_del(&inpu->gleaf_entry);
			graph->inleaf_cnt--;
		}
	}

	/* 7. verification */
	for (i = 0; i < vertex_cnt; ++i) {
		vertex = graph->vertex_array[i];
		if (!vertex) {
			vpu_err("vertex is NULL\n");
			BUG();
		}

		chain_cnt = vertex->chain_cnt;
		for (j = 0; j < chain_cnt; ++j) {
			chain = vertex->chain_array[j];
			if (!chain) {
				vpu_err("chain is NULL\n");
				BUG();
			}

			inleaf_list = &chain->inleaf_list;
			list_for_each_entry_safe(inpu, t1, inleaf_list, cleaf_entry) {
				if (test_bit(VPUO_PU_STATE_OT, &inpu->state)) {
					vpu_ierr("pu %d have invalid OT flag\n", graph, inpu->id);
					ret = -EINVAL;
					goto p_err;
				}

				if (test_bit(VPUO_PU_STATE_IM, &inpu->state) && !inpu->buffer_cnt) {
					vpu_ierr("pu %d have invalid buffer cnt(%d)\n", graph, inpu->id, inpu->buffer_cnt);
					ret = -EINVAL;
					goto p_err;
				}

				if (test_bit(VPUO_PU_STATE_IM, &inpu->state) && !inpu->inpu_cnt) {
					vpu_ierr("pu %d should HAVE inputs\n", graph, inpu->id);
					ret = -EINVAL;
					goto p_err;
				}
			}

			otleaf_list = &chain->otleaf_list;
			list_for_each_entry_safe(otpu, t2, otleaf_list, cleaf_entry) {
				if (test_bit(VPUO_PU_STATE_IN, &otpu->state)) {
					vpu_ierr("pu %d have invalid IN flag\n", graph, otpu->id);
					ret = -EINVAL;
					goto p_err;
				}

				if (test_bit(VPUO_PU_STATE_IM, &otpu->state) && !otpu->buffer_cnt) {
					vpu_ierr("pu %d have invalid buffer cnt(%d)\n", graph, otpu->id, otpu->buffer_cnt);
					ret = -EINVAL;
					goto p_err;
				}

				if (test_bit(VPUO_PU_STATE_IM, &otpu->state) && !otpu->otpu_cnt) {
					vpu_ierr("pu %d should HAVE outputs\n", graph, otpu->id);
					ret = -EINVAL;
					goto p_err;
				}
			}
		}
	}

	/* 8. pu bitmap for debugging */
	for (i = 0; i < vertex_cnt; ++i) {
		vertex = graph->vertex_array[i];
		if (!vertex) {
			vpu_err("vertex is NULL\n");
			BUG();
		}

		chain_cnt = vertex->chain_cnt;
		for (j = 0; j < chain_cnt; ++j) {
			chain = vertex->chain_array[j];
			if (!chain) {
				vpu_err("chain is NULL\n");
				BUG();
			}

			pu_cnt = chain->pu_cnt;
			for (k = 0; k < pu_cnt; ++k) {
				pu = chain->pu_array[k];
				if (!pu) {
					vpu_err("pu is NULL\n");
					BUG();
				}

				graph->pu_map[pu->id]++;
			}
		}
	}

p_err:
	__vpu_graph_print(graph);
	return ret;
}

void vpu_graph_task_print(struct vpu_graph *graph)
{
	int i, j, k, l;
	int n_vertex, n_subchain, n_pu;
	int n_mem, n_edge, n_iteration;
	struct vpul_task *task;
	struct vpul_vertex *vertex;
	struct vpul_process *process;
	struct vpul_subchain *subchain;
	struct vpul_pu *pu;

	task = graph->desc_mtask;

	vpu_info("[TASK:%d]\n", task->id);
	vpu_info("priority : %d\n", task->priority);
	vpu_info("total_size : %d\n", task->total_size);
	vpu_info("t_num_of_vertices : %d\n", task->t_num_of_vertices);
	vpu_info("t_num_of_subchains : %d\n", task->t_num_of_subchains);
	vpu_info("t_num_of_pus : %d\n", task->t_num_of_pus);

	n_mem = task->n_memmap_desc;
	for (i = 0; i < n_mem; ++i) {
		vpu_info("(MMAP:%d) type : %d, index : %d, w : %d, h : %d, n : %d, l : %d\n", i,
			task->memmap_desc[i].mtype,
			task->memmap_desc[i].index,
			task->memmap_desc[i].image_sizes.width,
			task->memmap_desc[i].image_sizes.height,
			task->memmap_desc[i].image_sizes.pixel_bytes,
			task->memmap_desc[i].image_sizes.line_offset);
	}

	n_mem = task->n_external_mem_addresses;
	for (i = 0; i < n_mem; ++i) {
		vpu_info("(EXT:%d) addr : 0x%X\n", i, task->external_mem_addr[i]);
	}

	n_mem = task->n_internal_rams;
	vpu_info("n_internal_rams : %d\n", n_mem);
	/* for (j = 0; j < n_mem; ++j) {
		vpu_info("(MMAP:%d) %d\n", j, vertex[i].proc.io.memmap_desc_idx[j]);
	} */

	n_vertex = task->t_num_of_vertices;
	vertex = (struct vpul_vertex *)((char *)task + task->vertices_vec_ofs);
	for (i = 0; i < n_vertex; ++i) {
		vpu_info("[VERTEX:%d] %ld\n", i, (ulong)&vertex[i] - (ulong)task);
		vpu_info("vtype : %d\n", vertex[i].vtype);
		vpu_info("n_out_edges : %d\n", vertex[i].n_out_edges);

		n_edge = vertex[i].n_out_edges;
		for (j = 0; j < n_edge; ++j) {
			vpu_info("(EDGE:%d) index : %d\n", j, vertex[i].out_edges[j].dst_vtx_idx);
		}

		vpu_info("loop.type : %d\n", vertex[i].loop.type);
		vpu_info("loop.id : %d\n", vertex[i].loop.id);
		vpu_info("loop.n_end_loop_edges : %d\n", vertex[i].loop.n_end_loop_edges);
		vpu_info("loop.iterations : %d\n", vertex[i].loop.iterations);

		vpu_info("num_of_subchains : %d\n", vertex[i].num_of_subchains);

		if (vertex[i].vtype != VPUL_VERTEXT_PROC)
			continue;

		process = &vertex[i].proc;

		vpu_info("\t" "[PROC] %ld\n", (ulong)process - (ulong)task);

		n_mem = process->io.n_dynamic_map_rois;
		vpu_info("\t" "n_dynamic_map_rois : %d\n", n_mem);
		for (j = 0; j < n_mem; ++j) {
			vpu_info("\t" "(DROI:%d) %d\n", j,
				process->io.dynamic_map_roi[j].memmap_idx);
		}

		n_mem = process->io.n_fixed_map_roi;
		vpu_info("\t" "n_fixed_map_roi : %d\n", n_mem);
		for (j = 0; j < n_mem; ++j) {
			vpu_info("\t" "(FROI:%d) %d %d %d %d %d\n", j, process->io.fixed_map_roi[j].memmap_idx,
				process->io.fixed_map_roi[j].roi.first_col,
				process->io.fixed_map_roi[j].roi.first_line,
				process->io.fixed_map_roi[j].roi.width,
				process->io.fixed_map_roi[j].roi.height);
		}

		n_iteration = process->io.n_inout_types;
		vpu_info("\t" "n_inout_types : %d\n", n_iteration);
		for (j = 0; j < n_iteration; ++j) {
			vpu_info("\t" "(IOTYPE:%d) %d %d %d\n", j,
				process->io.inout_types[j].is_dynamic,
				process->io.inout_types[j].roi_index,
				process->io.inout_types[j].n_insets_per_dynamic_roi);
		}

		n_iteration = process->io.n_sizes_op;
		vpu_info("\t" "n_sizes_op : %d\n", n_iteration);
		for (j = 0; j < n_iteration; ++j) {
			vpu_info("\t" "(SIZE:%d) %d %d %d\n", j,
				process->io.sizes[j].type,
				process->io.sizes[j].op_ind,
				process->io.sizes[j].src_idx);
		}

		n_iteration = process->io.n_scales;
		vpu_info("\t" "n_scales : %d\n", n_iteration);
		for (j = 0; j < n_iteration; ++j) {
			vpu_info("\t" "(SCALE:%d) %d/%d %d/%d\n", j,
				process->io.scales[j].horizontal.numerator,
				process->io.scales[j].horizontal.denominator,
				process->io.scales[j].vertical.numerator,
				process->io.scales[j].vertical.denominator);
		}

		n_iteration = process->io.n_croppers;
		vpu_info("\t" "n_croppers : %d\n", n_iteration);
		for (j = 0; j < n_iteration; ++j) {
			vpu_info("\t" "(CROP:%d) %d %d %d %d\n", j,
				process->io.croppers[j].Left,
				process->io.croppers[j].Right,
				process->io.croppers[j].Top,
				process->io.croppers[j].Bottom);
		}

		n_iteration = process->io.n_static_coff;
		vpu_info("\t" "n_static_coff : %d\n", n_iteration);
		for (j = 0; j < n_iteration; ++j) {
			vpu_info("\t" "(SCOFF:%d) %d\n", j,
				process->io.static_coff[j]);
		}

		vpu_info("\t" "param_loop.n_exp_per_sets : %d\n", process->io.param_loop.n_exp_per_sets);
		vpu_info("\t" "param_loop.n_sets : %d\n", process->io.param_loop.n_sets);
		vpu_info("\t" "param_loop.fix_inc : %d\n", process->io.param_loop.fix_inc);
		vpu_info("\t" "param_loop.per_fst : %d\n", process->io.param_loop.per_fst);

		vpu_info("\t" "n_tiles : %d\n", process->io.n_tiles);
		vpu_info("\t" "n_insets : %d\n", process->io.n_insets);
		vpu_info("\t" "n_presets_map_desc : %d\n", process->io.n_presets_map_desc);
		vpu_info("\t" "n_postsets_map_desc : %d\n", process->io.n_postsets_map_desc);
		vpu_info("\t" "n_subchain_before_insets : %d\n", process->io.n_subchain_before_insets);
		vpu_info("\t" "n_subchain_after_insets : %d\n", process->io.n_subchain_after_insets);
		vpu_info("\t" "n_invocations_per_input_tile : %d\n", process->io.n_invocations_per_input_tile);
		vpu_info("\t" "max_input_sets_slots : %d\n", process->io.max_input_sets_slots);

		n_subchain = vertex[i].num_of_subchains;
		subchain = (struct vpul_subchain *)((char *)task + vertex[i].sc_ofs);

		for (j = 0; j < n_subchain; ++j) {
			vpu_info("\t\t" "[SC:%d] %ld\n", j, (ulong)&subchain[j] - (ulong)task);
			vpu_info("\t\t" "id : %d\n", subchain[j].id);
			vpu_info("\t\t" "stype : %d\n", subchain[j].stype);
			vpu_info("\t\t" "num_of_pus : %d\n", subchain[j].num_of_pus);

			if (subchain[j].stype == VPUL_SUB_CH_HW) {
				n_pu = subchain[j].num_of_pus;
				pu = (struct vpul_pu *)((char *)task + subchain[j].pus_ofs);

				for (k = 0; k < n_pu; ++k) {
					vpu_info("\t\t\t" "[PU:%d] %ld\n", k, (ulong)&pu[k] - (ulong)task);
					vpu_info("\t\t\t" "instance : %d\n", pu[k].instance);
					vpu_info("\t\t\t" "mprb_type : %d\n", pu[k].mprb_type);
					vpu_info("\t\t\t" "in_size_idx : %d\n", pu[k].in_size_idx);
					vpu_info("\t\t\t" "out_size_idx : %d\n", pu[k].out_size_idx);
					vpu_info("\t\t\t" "n_mprbs : %d\n", pu[k].n_mprbs);
					n_iteration = pu[k].n_mprbs;
					for (l = 0; l < n_iteration; ++l) {
						vpu_info("\t\t\t" "(MPRB:%d) %d\n", l, pu[k].mprbs[l]);
					}

					switch (pu[k].op_type) {
					case VPUL_OP_DMA:
						vpu_info("\t\t\t" "inout_index : %d\n", pu[k].params.dma.inout_index);
						vpu_info("\t\t\t" "offset_lines_inc : %d\n", pu[k].params.dma.offset_lines_inc);
						break;
					case VPUL_OP_FULL_SALB:
						vpu_info("\t\t\t" "bits_in0 : %d\n", pu[k].params.salb.bits_in0);
						vpu_info("\t\t\t" "bits_in1 : %d\n", pu[k].params.salb.bits_in1);
						vpu_info("\t\t\t" "bits_out0 : %d\n", pu[k].params.salb.bits_out0);
						vpu_info("\t\t\t" "signed_in0 : %d\n", pu[k].params.salb.signed_in0);
						vpu_info("\t\t\t" "signed_in1 : %d\n", pu[k].params.salb.signed_in1);
						vpu_info("\t\t\t" "signed_out0 : %d\n", pu[k].params.salb.signed_out0);
						vpu_info("\t\t\t" "input_enable : %d\n", pu[k].params.salb.input_enable);
						vpu_info("\t\t\t" "use_mask : %d\n", pu[k].params.salb.use_mask);
						vpu_info("\t\t\t" "operation_code : %d\n", pu[k].params.salb.operation_code);
						vpu_info("\t\t\t" "abs_neg : %d\n", pu[k].params.salb.abs_neg);
						vpu_info("\t\t\t" "trunc_out : %d\n", pu[k].params.salb.trunc_out);
						vpu_info("\t\t\t" "org_val_med : %d\n", pu[k].params.salb.org_val_med);
						vpu_info("\t\t\t" "shift_bits : %d\n", pu[k].params.salb.shift_bits);
						vpu_info("\t\t\t" "cmp_op : %d\n", pu[k].params.salb.cmp_op);
						vpu_info("\t\t\t" "const_in1 : %d\n", pu[k].params.salb.const_in1);
						vpu_info("\t\t\t" "thresh_lo : %d\n", pu[k].params.salb.thresh_lo);
						vpu_info("\t\t\t" "thresh_hi : %d\n", pu[k].params.salb.thresh_hi);
						vpu_info("\t\t\t" "val_lo : %d\n", pu[k].params.salb.val_lo);
						vpu_info("\t\t\t" "val_hi : %d\n", pu[k].params.salb.val_hi);
						vpu_info("\t\t\t" "val_med_filler : %d\n", pu[k].params.salb.val_med_filler);
						vpu_info("\t\t\t" "salbregs_custom_trunc_en : %d\n", pu[k].params.salb.salbregs_custom_trunc_en);
						vpu_info("\t\t\t" "salbregs_custom_trunc_bittage : %d\n", pu[k].params.salb.salbregs_custom_trunc_bittage);
						break;
					case VPUL_OP_FULL_CALB:
						vpu_info("\t\t\t" "bits_in0 : %d\n", pu[k].params.calb.bits_in0);
						vpu_info("\t\t\t" "bits_in1 : %d\n", pu[k].params.calb.bits_in1);
						vpu_info("\t\t\t" "bits_out0 : %d\n", pu[k].params.calb.bits_out0);
						vpu_info("\t\t\t" "signed_in0 : %d\n", pu[k].params.calb.signed_in0);
						vpu_info("\t\t\t" "signed_in1 : %d\n", pu[k].params.calb.signed_in1);
						vpu_info("\t\t\t" "signed_out0 : %d\n", pu[k].params.calb.signed_out0);
						vpu_info("\t\t\t" "input_enable : %d\n", pu[k].params.calb.input_enable);
						vpu_info("\t\t\t" "operation_code : %d\n", pu[k].params.calb.operation_code);
						vpu_info("\t\t\t" "abs_neg : %d\n", pu[k].params.calb.abs_neg);
						vpu_info("\t\t\t" "trunc_out : %d\n", pu[k].params.calb.trunc_out);
						vpu_info("\t\t\t" "org_val_med : %d\n", pu[k].params.calb.org_val_med);
						vpu_info("\t\t\t" "shift_bits : %d\n", pu[k].params.calb.shift_bits);
						vpu_info("\t\t\t" "mult_round : %d\n", pu[k].params.calb.mult_round);
						vpu_info("\t\t\t" "const_in1 : %d\n", pu[k].params.calb.const_in1);
						vpu_info("\t\t\t" "div_shift_bits : %d\n", pu[k].params.calb.div_shift_bits);
						vpu_info("\t\t\t" "div_overflow_remainder : %d\n", pu[k].params.calb.div_overflow_remainder);
						vpu_info("\t\t\t" "thresh_lo : %d\n", pu[k].params.calb.thresh_lo);
						vpu_info("\t\t\t" "thresh_hi : %d\n", pu[k].params.calb.thresh_hi);
						vpu_info("\t\t\t" "val_lo : %d\n", pu[k].params.calb.val_lo);
						vpu_info("\t\t\t" "val_hi : %d\n", pu[k].params.calb.val_hi);
						vpu_info("\t\t\t" "val_med_filler : %d\n", pu[k].params.calb.val_med_filler);
						break;
					case VPUL_OP_ROI:
						vpu_info("\t\t\t" "bits_in0 : %d\n", pu[k].params.rois_out.bits_in0);
						vpu_info("\t\t\t" "signed_in0 : %d\n", pu[k].params.rois_out.signed_in0);
						vpu_info("\t\t\t" "use_mask : %d\n", pu[k].params.rois_out.use_mask);
						vpu_info("\t\t\t" "work_mode : %d\n", pu[k].params.rois_out.work_mode);
						vpu_info("\t\t\t" "first_min_max : %d\n", pu[k].params.rois_out.first_min_max);
						vpu_info("\t\t\t" "thresh_lo_temp : %d\n", pu[k].params.rois_out.thresh_lo_temp);
						break;
					case VPUL_OP_CROP:
						vpu_info("\t\t\t" "work_mode : %d\n", pu[k].params.crop.work_mode);
						vpu_info("\t\t\t" "pad_value : %d\n", pu[k].params.crop.pad_value);
						vpu_info("\t\t\t" "mask_val_in : %d\n", pu[k].params.crop.mask_val_in);
						vpu_info("\t\t\t" "mask_val_out : %d\n", pu[k].params.crop.mask_val_out);
						vpu_info("\t\t\t" "roi_startx : %d\n", pu[k].params.crop.roi_startx);
						vpu_info("\t\t\t" "roi_starty : %d\n", pu[k].params.crop.roi_starty);
						break;
					case VPUL_OP_MDE:
						vpu_info("\t\t\t" "work_mode : %d\n", pu[k].params.mde.work_mode);
						vpu_info("\t\t\t" "result_shift : %d\n", pu[k].params.mde.result_shift);
						vpu_info("\t\t\t" "use_thresh : %d\n", pu[k].params.mde.use_thresh);
						vpu_info("\t\t\t" "calc_quantized_angle : %d\n", pu[k].params.mde.calc_quantized_angle);
						vpu_info("\t\t\t" "eig_coeff : %d\n", pu[k].params.mde.eig_coeff);
						vpu_info("\t\t\t" "bits_in : %d\n", pu[k].params.mde.bits_in);
						vpu_info("\t\t\t" "signed_in : %d\n", pu[k].params.mde.signed_in);
						vpu_info("\t\t\t" "bits_out : %d\n", pu[k].params.mde.bits_out);
						vpu_info("\t\t\t" "signed_out : %d\n", pu[k].params.mde.signed_out);
						vpu_info("\t\t\t" "output_enable : %d\n", pu[k].params.mde.output_enable);
						vpu_info("\t\t\t" "thresh : %d\n", pu[k].params.mde.thresh);
						break;
					case VPUL_OP_NMS:
						vpu_info("\t\t\t" "work_mode : %d\n", pu[k].params.nms.work_mode);
						vpu_info("\t\t\t" "keep_equals : %d\n", pu[k].params.nms.keep_equals);
						vpu_info("\t\t\t" "directional_nms : %d\n", pu[k].params.nms.directional_nms);
						vpu_info("\t\t\t" "census_mode : %d\n", pu[k].params.nms.census_mode);
						vpu_info("\t\t\t" "add_orig_pixel : %d\n", pu[k].params.nms.add_orig_pixel);
						vpu_info("\t\t\t" "bits_in : %d\n", pu[k].params.nms.bits_in);
						vpu_info("\t\t\t" "bits_out : %d\n", pu[k].params.nms.bits_out);
						vpu_info("\t\t\t" "signed_in : %d\n", pu[k].params.nms.signed_in);
						vpu_info("\t\t\t" "support : %d\n", pu[k].params.nms.support);
						vpu_info("\t\t\t" "org_val_out : %d\n", pu[k].params.nms.org_val_out);
						vpu_info("\t\t\t" "trunc_out : %d\n", pu[k].params.nms.trunc_out);
						vpu_info("\t\t\t" "image_height : %d\n", pu[k].params.nms.image_height);
						vpu_info("\t\t\t" "thresh : %d\n", pu[k].params.nms.thresh);
						vpu_info("\t\t\t" "border_mode_up : %d\n", pu[k].params.nms.border_mode_up);
						vpu_info("\t\t\t" "border_mode_down : %d\n", pu[k].params.nms.border_mode_down);
						vpu_info("\t\t\t" "border_mode_left : %d\n", pu[k].params.nms.border_mode_left);
						vpu_info("\t\t\t" "border_mode_right : %d\n", pu[k].params.nms.border_mode_right);
						vpu_info("\t\t\t" "border_fill : %d\n", pu[k].params.nms.border_fill);
						vpu_info("\t\t\t" "border_fill_constant : %d\n", pu[k].params.nms.border_fill_constant);
						vpu_info("\t\t\t" "strict_comparison_mask : %d\n", pu[k].params.nms.strict_comparison_mask);
						vpu_info("\t\t\t" "cens_thres_0 : %d\n", pu[k].params.nms.cens_thres_0);
						vpu_info("\t\t\t" "cens_thres_1 : %d\n", pu[k].params.nms.cens_thres_1);
						vpu_info("\t\t\t" "cens_thres_2 : %d\n", pu[k].params.nms.cens_thres_2);
						vpu_info("\t\t\t" "cens_thres_3 : %d\n", pu[k].params.nms.cens_thres_3);
						vpu_info("\t\t\t" "cens_thres_4 : %d\n", pu[k].params.nms.cens_thres_4);
						vpu_info("\t\t\t" "cens_thres_5 : %d\n", pu[k].params.nms.cens_thres_5);
						vpu_info("\t\t\t" "cens_thres_6 : %d\n", pu[k].params.nms.cens_thres_6);
						vpu_info("\t\t\t" "cens_thres_7 : %d\n", pu[k].params.nms.cens_thres_7);
						break;
					case VPUL_OP_CCM:
						vpu_info("\t\t\t" "signed_in : %d\n", pu[k].params.ccm.signed_in);
						vpu_info("\t\t\t" "signed_out : %d\n", pu[k].params.ccm.signed_out);
						vpu_info("\t\t\t" "output_enable : %d\n", pu[k].params.ccm.output_enable);
						vpu_info("\t\t\t" "input_enable : %d\n", pu[k].params.ccm.input_enable);
						vpu_info("\t\t\t" "coefficient_shift : %d\n", pu[k].params.ccm.coefficient_shift);
						vpu_info("\t\t\t" "coefficient_0 : %d\n", pu[k].params.ccm.coefficient_0);
						vpu_info("\t\t\t" "coefficient_1 : %d\n", pu[k].params.ccm.coefficient_1);
						vpu_info("\t\t\t" "coefficient_2 : %d\n", pu[k].params.ccm.coefficient_2);
						vpu_info("\t\t\t" "coefficient_3 : %d\n", pu[k].params.ccm.coefficient_3);
						vpu_info("\t\t\t" "coefficient_4 : %d\n", pu[k].params.ccm.coefficient_4);
						vpu_info("\t\t\t" "coefficient_5 : %d\n", pu[k].params.ccm.coefficient_5);
						vpu_info("\t\t\t" "coefficient_6 : %d\n", pu[k].params.ccm.coefficient_6);
						vpu_info("\t\t\t" "coefficient_7 : %d\n", pu[k].params.ccm.coefficient_7);
						vpu_info("\t\t\t" "coefficient_8 : %d\n", pu[k].params.ccm.coefficient_8);
						vpu_info("\t\t\t" "offset_0 : %d\n", pu[k].params.ccm.offset_0);
						vpu_info("\t\t\t" "offset_1 : %d\n", pu[k].params.ccm.offset_1);
						vpu_info("\t\t\t" "offset_2 : %d\n", pu[k].params.ccm.offset_2);
						break;
					case VPUL_OP_SEP_FLT:
						vpu_info("\t\t\t" "invert_columns : %d\n", pu[k].params.slf.invert_columns);
						vpu_info("\t\t\t" "upsample_mode : %d\n", pu[k].params.slf.upsample_mode);
						vpu_info("\t\t\t" "downsample_rows : %d\n", pu[k].params.slf.downsample_rows);
						vpu_info("\t\t\t" "downsample_cols : %d\n", pu[k].params.slf.downsample_cols);
						vpu_info("\t\t\t" "sampling_offset_x : %d\n", pu[k].params.slf.sampling_offset_x);
						vpu_info("\t\t\t" "sampling_offset_y : %d\n", pu[k].params.slf.sampling_offset_y);
						vpu_info("\t\t\t" "work_mode : %d\n", pu[k].params.slf.work_mode);
						vpu_info("\t\t\t" "filter_size_mode : %d\n", pu[k].params.slf.filter_size_mode);
						vpu_info("\t\t\t" "out_enable_1 : %d\n", pu[k].params.slf.out_enable_1);
						vpu_info("\t\t\t" "horizontal_only : %d\n", pu[k].params.slf.horizontal_only);
						vpu_info("\t\t\t" "bits_in : %d\n", pu[k].params.slf.bits_in);
						vpu_info("\t\t\t" "bits_out : %d\n", pu[k].params.slf.bits_out);
						vpu_info("\t\t\t" "signed_in : %d\n", pu[k].params.slf.signed_in);
						vpu_info("\t\t\t" "signed_out : %d\n", pu[k].params.slf.signed_out);
						vpu_info("\t\t\t" "do_rounding : %d\n", pu[k].params.slf.do_rounding);
						vpu_info("\t\t\t" "border_mode_up : %d\n", pu[k].params.slf.border_mode_up);
						vpu_info("\t\t\t" "border_mode_down : %d\n", pu[k].params.slf.border_mode_down);
						vpu_info("\t\t\t" "border_mode_left : %d\n", pu[k].params.slf.border_mode_left);
						vpu_info("\t\t\t" "border_mode_right : %d\n", pu[k].params.slf.border_mode_right);
						vpu_info("\t\t\t" "border_fill : %d\n", pu[k].params.slf.border_fill);
						vpu_info("\t\t\t" "border_fill_constant : %d\n", pu[k].params.slf.border_fill_constant);
						vpu_info("\t\t\t" "coefficient_fraction : %d\n", pu[k].params.slf.coefficient_fraction);
						vpu_info("\t\t\t" "sepfregs_is_max_pooling_mode : %d\n", pu[k].params.slf.sepfregs_is_max_pooling_mode);
						vpu_info("\t\t\t" "sepfregs_stride_value : %d\n", pu[k].params.slf.sepfregs_stride_value);
						vpu_info("\t\t\t" "sepfregs_stride_offset_height : %d\n", pu[k].params.slf.sepfregs_stride_offset_height);
						vpu_info("\t\t\t" "sepfregs_stride_offset_width : %d\n", pu[k].params.slf.sepfregs_stride_offset_width);
						vpu_info("\t\t\t" "sepfregs_subimage_height : %d\n", pu[k].params.slf.sepfregs_subimage_height);
						vpu_info("\t\t\t" "sepfregs_convert_16f_to_32f : %d\n", pu[k].params.slf.sepfregs_convert_16f_to_32f);
						vpu_info("\t\t\t" "sepfregs_convert_output_sm_to_2scomp : %d\n", pu[k].params.slf.sepfregs_convert_output_sm_to_2scomp);
						vpu_info("\t\t\t" "sepfregs_convert_input_2scomp_to_sm : %d\n", pu[k].params.slf.sepfregs_convert_input_2scomp_to_sm);
						vpu_info("\t\t\t" "maxp_num_slices : %d\n", pu[k].params.slf.maxp_num_slices);
						vpu_info("\t\t\t" "maxp_sizes_filt_hor : %d\n", pu[k].params.slf.maxp_sizes_filt_hor);
						vpu_info("\t\t\t" "maxp_sizes_filt_ver : %d\n", pu[k].params.slf.maxp_sizes_filt_ver);
						vpu_info("\t\t\t" "coefficient_index : %d\n", pu[k].params.slf.coefficient_index);
						break;
					case VPUL_OP_GEN_FLT:
						vpu_info("\t\t\t" "filter_size_mode : %d\n", pu[k].params.glf.filter_size_mode);
						vpu_info("\t\t\t" "sad_mode : %d\n", pu[k].params.glf.sad_mode);
						vpu_info("\t\t\t" "out_enable_2 : %d\n", pu[k].params.glf.out_enable_2);
						vpu_info("\t\t\t" "two_outputs : %d\n", pu[k].params.glf.two_outputs);
						vpu_info("\t\t\t" "input_enable1 : %d\n", pu[k].params.glf.input_enable1);
						vpu_info("\t\t\t" "bits_in : %d\n", pu[k].params.glf.bits_in);
						vpu_info("\t\t\t" "bits_out : %d\n", pu[k].params.glf.bits_out);
						vpu_info("\t\t\t" "signed_in : %d\n", pu[k].params.glf.signed_in);
						vpu_info("\t\t\t" "signed_out : %d\n", pu[k].params.glf.signed_out);
						vpu_info("\t\t\t" "do_rounding : %d\n", pu[k].params.glf.do_rounding);
						vpu_info("\t\t\t" "RAM_type : %d\n", pu[k].params.glf.RAM_type);
						vpu_info("\t\t\t" "RAM_offset : %d\n", pu[k].params.glf.RAM_offset);
						vpu_info("\t\t\t" "image_height : %d\n", pu[k].params.glf.image_height);
						vpu_info("\t\t\t" "border_mode_up : %d\n", pu[k].params.glf.border_mode_up);
						vpu_info("\t\t\t" "border_mode_down : %d\n", pu[k].params.glf.border_mode_down);
						vpu_info("\t\t\t" "border_mode_left : %d\n", pu[k].params.glf.border_mode_left);
						vpu_info("\t\t\t" "border_mode_right : %d\n", pu[k].params.glf.border_mode_right);
						vpu_info("\t\t\t" "border_fill : %d\n", pu[k].params.glf.border_fill);
						vpu_info("\t\t\t" "border_fill_constant : %d\n", pu[k].params.glf.border_fill_constant);
						vpu_info("\t\t\t" "coefficient_fraction : %d\n", pu[k].params.glf.coefficient_fraction);
						vpu_info("\t\t\t" "signed_coefficients : %d\n", pu[k].params.glf.signed_coefficients);
						vpu_info("\t\t\t" "coefficient_index : %d\n", pu[k].params.glf.coefficient_index);
						vpu_info("\t\t\t" "coeffs_from_dma : %d\n", pu[k].params.glf.coeffs_from_dma);
						break;
					case VPUL_OP_NLF_FLT:
						vpu_info("\t\t\t" "filter_mode : %d\n", pu[k].params.nlf.filter_mode);
						vpu_info("\t\t\t" "fast_score_direction : %d\n", pu[k].params.nlf.fast_score_direction);
						vpu_info("\t\t\t" "border_mode_up : %d\n", pu[k].params.nlf.border_mode_up);
						vpu_info("\t\t\t" "border_mode_left: %d\n", pu[k].params.nlf.border_mode_left);
						vpu_info("\t\t\t" "border_mode_right : %d\n", pu[k].params.nlf.border_mode_right);
						vpu_info("\t\t\t" "border_fill : %d\n", pu[k].params.nlf.border_fill);
						vpu_info("\t\t\t" "border_tile : %d\n", pu[k].params.nlf.border_tile);
						vpu_info("\t\t\t" "border_fill_constant : %d\n", pu[k].params.nlf.border_fill_constant);
						vpu_info("\t\t\t" "bits_in : %d\n", pu[k].params.nlf.bits_in);
						vpu_info("\t\t\t" "signed_in : %d\n", pu[k].params.nlf.signed_in);
						vpu_info("\t\t\t" "census_mode : %d\n", pu[k].params.nlf.census_mode);
						vpu_info("\t\t\t" "census_out_image : %d\n", pu[k].params.nlf.census_out_image);
						vpu_info("\t\t\t" "add_orig_pixel : %d\n", pu[k].params.nlf.add_orig_pixel);
						vpu_info("\t\t\t" "cens_thres_0 : %d\n", pu[k].params.nlf.cens_thres_0);
						vpu_info("\t\t\t" "cens_thres_1 : %d\n", pu[k].params.nlf.cens_thres_1);
						vpu_info("\t\t\t" "cens_thres_2 : %d\n", pu[k].params.nlf.cens_thres_2);
						vpu_info("\t\t\t" "cens_thres_3 : %d\n", pu[k].params.nlf.cens_thres_3);
						vpu_info("\t\t\t" "cens_thres_4 : %d\n", pu[k].params.nlf.cens_thres_4);
						vpu_info("\t\t\t" "cens_thres_5 : %d\n", pu[k].params.nlf.cens_thres_5);
						vpu_info("\t\t\t" "cens_thres_6 : %d\n", pu[k].params.nlf.cens_thres_6);
						vpu_info("\t\t\t" "cens_thres_7 : %d\n", pu[k].params.nlf.cens_thres_7);
						vpu_info("\t\t\t" "neighbors_mask : %d\n", pu[k].params.nlf.neighbors_mask);
						break;
					case VPUL_OP_UPSCALER:
						vpu_info("\t\t\t" "interpolation_method : %d\n", pu[k].params.upscaler.interpolation_method);
						vpu_info("\t\t\t" "border_fill_mode : %d\n", pu[k].params.upscaler.border_fill_mode);
						vpu_info("\t\t\t" "do_rounding : %d\n", pu[k].params.upscaler.do_rounding);
						vpu_info("\t\t\t" "border_fill_constant : %d\n", pu[k].params.upscaler.border_fill_constant);
						break;
					case VPUL_OP_DOWNSCALER:
						vpu_info("\t\t\t" "interpolation_method : %d\n", pu[k].params.downScaler.interpolation_method);
						vpu_info("\t\t\t" "do_rounding : %d\n", pu[k].params.downScaler.do_rounding);
						break;
					case VPUL_OP_DUPLICATE:
						break;
					case VPUL_OP_SPLIT:
						vpu_info("\t\t\t" "out0_byte0 : %d\n", pu[k].params.spliter.out0_byte0);
						vpu_info("\t\t\t" "out0_byte1 : %d\n", pu[k].params.spliter.out0_byte1);
						vpu_info("\t\t\t" "out1_byte0 : %d\n", pu[k].params.spliter.out1_byte0);
						vpu_info("\t\t\t" "out1_byte1 : %d\n", pu[k].params.spliter.out1_byte1);
						vpu_info("\t\t\t" "out2_byte0 : %d\n", pu[k].params.spliter.out2_byte0);
						vpu_info("\t\t\t" "out2_byte1 : %d\n", pu[k].params.spliter.out2_byte1);
						vpu_info("\t\t\t" "out3_byte0 : %d\n", pu[k].params.spliter.out3_byte0);
						vpu_info("\t\t\t" "out3_byte1 : %d\n", pu[k].params.spliter.out3_byte1);
						break;
					case VPUL_OP_JOIN:
						vpu_info("\t\t\t" "out_byte0_source_stream : %d\n", pu[k].params.joiner.out_byte0_source_stream);
						vpu_info("\t\t\t" "out_byte1_source_stream : %d\n", pu[k].params.joiner.out_byte1_source_stream);
						vpu_info("\t\t\t" "out_byte2_source_stream : %d\n", pu[k].params.joiner.out_byte2_source_stream);
						vpu_info("\t\t\t" "out_byte3_source_stream : %d\n", pu[k].params.joiner.out_byte3_source_stream);
						vpu_info("\t\t\t" "input0_enable : %d\n", pu[k].params.joiner.input0_enable);
						vpu_info("\t\t\t" "input1_enable : %d\n", pu[k].params.joiner.input1_enable);
						vpu_info("\t\t\t" "input2_enable : %d\n", pu[k].params.joiner.input2_enable);
						vpu_info("\t\t\t" "input3_enable : %d\n", pu[k].params.joiner.input3_enable);
						vpu_info("\t\t\t" "work_mode : %d\n", pu[k].params.joiner.work_mode);
						break;
					case VPUL_OP_INTEGRAL_IMG:
						vpu_info("\t\t\t" "integral_image_mode : %d\n", pu[k].params.integral.integral_image_mode);
						vpu_info("\t\t\t" "overflow_mode : %d\n", pu[k].params.integral.overflow_mode);
						vpu_info("\t\t\t" "dt_right_shift : %d\n", pu[k].params.integral.dt_right_shift);
						vpu_info("\t\t\t" "dt_left_shift : %d\n", pu[k].params.integral.dt_left_shift);
						vpu_info("\t\t\t" "dt_coefficient0 : %d\n", pu[k].params.integral.dt_coefficient0);
						vpu_info("\t\t\t" "dt_coefficient1 : %d\n", pu[k].params.integral.dt_coefficient1);
						vpu_info("\t\t\t" "dt_coefficient2 : %d\n", pu[k].params.integral.dt_coefficient2);
						vpu_info("\t\t\t" "cc_min_label : %d\n", pu[k].params.integral.cc_min_label);
						vpu_info("\t\t\t" "cc_scan_mode : %d\n", pu[k].params.integral.cc_scan_mode);
						vpu_info("\t\t\t" "cc_smart_label_search_en : %d\n", pu[k].params.integral.cc_smart_label_search_en);
						vpu_info("\t\t\t" "cc_reset_labels_array : %d\n", pu[k].params.integral.cc_reset_labels_array);
						vpu_info("\t\t\t" "cc_label_vector_size : %d\n", pu[k].params.integral.cc_label_vector_size);
						vpu_info("\t\t\t" "lut_init_en : %d\n", pu[k].params.integral.lut_init_en);
						vpu_info("\t\t\t" "lut_number_of_values : %d\n", pu[k].params.integral.lut_number_of_values);
						vpu_info("\t\t\t" "lut_value_shift : %d\n", pu[k].params.integral.lut_value_shift);
						vpu_info("\t\t\t" "lut_default_overflow : %d\n", pu[k].params.integral.lut_default_overflow);
						vpu_info("\t\t\t" "lut_default_underflow : %d\n", pu[k].params.integral.lut_default_underflow);
						break;
					case VPUL_OP_MAP_2_LST:
						vpu_info("\t\t\t" "bits_in : %d\n", pu[k].params.map2list.bits_in);
						vpu_info("\t\t\t" "signed_in : %d\n", pu[k].params.map2list.signed_in);
						vpu_info("\t\t\t" "value_in : %d\n", pu[k].params.map2list.value_in);
						vpu_info("\t\t\t" "enable_out_map : %d\n", pu[k].params.map2list.enable_out_map);
						vpu_info("\t\t\t" "enable_out_lst : %d\n", pu[k].params.map2list.enable_out_lst);
						vpu_info("\t\t\t" "inout_indx : %d\n", pu[k].params.map2list.inout_indx);
						vpu_info("\t\t\t" "threshold_low : %d\n", pu[k].params.map2list.threshold_low);
						vpu_info("\t\t\t" "threshold_high : %d\n", pu[k].params.map2list.threshold_high);
						vpu_info("\t\t\t" "num_of_point : %d\n", pu[k].params.map2list.num_of_point);
						break;
					case VPUL_OP_FIFO:
						break;
					case VPUL_OP_CNN:
						break;
					case VPUL_OP_LUT:
						vpu_info("\t\t\t" "interpolation_mode : %d\n", pu[k].params.lut.interpolation_mode);
						vpu_info("\t\t\t" "lut_size : %d\n", pu[k].params.lut.lut_size);
						vpu_info("\t\t\t" "signed_in0 : %d\n", pu[k].params.lut.signed_in0);
						vpu_info("\t\t\t" "signed_out0 : %d\n", pu[k].params.lut.signed_out0);
						vpu_info("\t\t\t" "offset : %d\n", pu[k].params.lut.offset);
						vpu_info("\t\t\t" "binsize : %d\n", pu[k].params.lut.binsize);
						vpu_info("\t\t\t" "inverse_binsize : %d\n", pu[k].params.lut.inverse_binsize);
						break;
					case VPUL_OP_HIST:
						vpu_info("\t\t\t" "offset : %d\n", pu[k].params.histogram.offset);
						vpu_info("\t\t\t" "inverse_binsize : %d\n", pu[k].params.histogram.inverse_binsize);
						vpu_info("\t\t\t" "variable_increment : %d\n", pu[k].params.histogram.variable_increment);
						vpu_info("\t\t\t" "dual_histogram : %d\n", pu[k].params.histogram.dual_histogram);
						vpu_info("\t\t\t" "signed_in0 : %d\n", pu[k].params.histogram.signed_in0);
						vpu_info("\t\t\t" "round_index : %d\n", pu[k].params.histogram.round_index);
						vpu_info("\t\t\t" "max_val : %d\n", pu[k].params.histogram.max_val);
						break;
					default:
						break;
					}
				}
			} else if (subchain[j].stype == VPUL_SUB_CH_CPU_OP) {
				for (k = 0; k < MAX_SUPPORT_CPU_OPER_NUM; ++k) {
					vpu_info("\t\t\t" "[CPU:%d]\n", k);
					vpu_info("\t\t\t" "opcode : %d\n", subchain[j].cpu.cpu_op_desc[k].opcode);
				}
			}
		}
	}
}

void vpu_graph_print(struct vpu_graph *graph)
{
	u32 i;
	struct vpu_framemgr *framemgr;
	struct vpu_exynos *exynos;

	BUG_ON(!graph);

	exynos = graph->exynos;
	framemgr = &graph->framemgr;

	vpu_info("GRAPH[%02d]: %2d %2d %2d %2d %2d\n", graph->id,
		framemgr->fre_cnt,
		framemgr->pre_cnt,
		framemgr->req_cnt,
		framemgr->pro_cnt,
		framemgr->com_cnt);

	for (i = 0; i < VPU_PU_NUMBER; ++i) {
		if (graph->pu_map[i]) {
			CTL_OP(exynos, ctl_dump, i);
		}
	}
}

struct vpuo_chain * vpu_graph_g_chain(struct vpu_graph *graph, u32 chain_id)
{
	int i, j;
	struct vpuo_vertex *vertex;
	struct vpuo_chain *chain = NULL;

	for (i = 0; i < graph->vertex_cnt; ++i) {
		vertex = graph->vertex_array[i];
		if (!vertex) {
			vpu_ierr("vertex is NULL\n", graph);
			BUG();
		}

		for (j = 0; j < vertex->chain_cnt; ++j) {
			if (vertex->chain_table[j] == chain_id) {
				chain = vertex->chain_array[j];
				break;
			}
		}
	}

	return chain;
}

int vpu_graph_create(struct vpu_graph **graph, void *cookie, void *resource, void *memory, void *exynos)
{
	int ret = 0;
	u32 i;
	struct vpu_framemgr *framemgr;

	BUG_ON(!*graph);
	BUG_ON(!cookie);

	*graph = kzalloc(sizeof(struct vpu_graph), GFP_KERNEL);
	if (*graph == NULL) {
		vpu_err("kzalloc is fail");
		ret = -ENOMEM;
		goto p_err;
	}

	ret = vpu_graphmgr_grp_register(cookie, *graph);
	if (ret) {
		vpu_err("vpu_graphmgr_grp_register is fail(%d)\n", ret);
		kfree(*graph);
		goto p_err;
	}

	(*graph)->control.message = VPU_CTRL_NONE;
	(*graph)->cookie = cookie;
	(*graph)->resource = resource;
	(*graph)->memory = memory;
	(*graph)->exynos = exynos;
	(*graph)->gops = &vpu_graph_ops;
	mutex_init(&(*graph)->local_lock);

	/* frame manager init */
	framemgr = &(*graph)->framemgr;
	framemgr->id = (*graph)->id;
	framemgr->sindex = 0;
	spin_lock_init(&framemgr->slock);

	for (i = 0; i < VPU_MAX_FRAME; ++i) {
		(*graph)->inhash[i] = VPU_MAX_FRAME;
		(*graph)->othash[i] = VPU_MAX_FRAME;
	}

	(*graph)->control.owner = *graph;
	init_waitqueue_head(&(*graph)->control_wq);
	ret = vpu_frame_init(framemgr, *graph);
	if (ret) {
		vpu_err("vpu_frame_init is fail(%d)\n", ret);
		kfree(*graph);
		goto p_err;
	}

p_err:
	return ret;
}

int vpu_graph_destroy(struct vpu_graph *graph)
{
	int ret = 0;

	BUG_ON(!graph);

	ret = __vpu_graph_stop(graph);
	if (ret)
		vpu_ierr("__vpu_graph_stop is fail(%d)\n", graph, ret);

	ret = vpu_graphmgr_grp_unregister(graph->cookie, graph);
	if (ret)
		vpu_ierr("vpu_graphmgr_grp_unregister is fail(%d)\n", graph, ret);

	ret = __vpu_graph_unmap(graph);
	if (ret)
		vpu_ierr("__vpu_graph_map is fail(%d)\n", graph, ret);

	ret = __vpu_graph_free(graph);
	if (ret)
		vpu_ierr("__vpu_graph_free is fail(%d)\n", graph, ret);

	kfree(graph->desc_utask);
	kfree(graph->desc_mtask);
	kfree(graph);

	return ret;
}

int vpu_graph_config(struct vpu_graph *graph, struct vs4l_graph *info)
{
	int ret = 0;

	BUG_ON(!graph);
	BUG_ON(!graph->cookie);
	BUG_ON(!info);

	if (test_bit(VPU_GRAPH_STATE_CONFIG, &graph->state)) {
		vpu_ierr("graph is already configured\n", graph);
		ret = -EINVAL;
		goto p_err;
	}

	if (info->priority > VPU_GRAPH_MAX_PRIORITY) {
		vpu_iwarn("graph priority is over(%d)\n", graph, info->priority);
		info->priority = VPU_GRAPH_MAX_PRIORITY;
	}

	graph->uid = info->id;
	graph->flags = info->flags;
	graph->priority = info->priority;
	graph->size = info->size;
	graph->period_ticks = 0;
	graph->desc_utask = NULL;
	graph->desc_mtask = NULL;

	if (test_bit(VS4L_GRAPH_FLAG_PERIODIC, &graph->flags))
		graph->period_ticks = info->time >= VPU_TIME_TICK ? info->time / VPU_TIME_TICK : 1;

	/* 1. memory allocation */
	if (!graph->size) {
		vpu_ierr("task descriptor size is zero(%lX %d)\n", graph, info->addr, info->size);
		ret = -ENOMEM;
		goto p_err;
	}

	graph->desc_utask = kzalloc(graph->size, GFP_KERNEL);
	if (!graph->desc_utask) {
		vpu_ierr("kzalloc(desc_utask) is fail\n", graph);
		ret = -ENOMEM;
		goto p_err;
	}

	graph->desc_mtask = kzalloc(graph->size, GFP_KERNEL);
	if (!graph->desc_mtask) {
		vpu_ierr("kzalloc(desc_mtask) is fail\n", graph);
		ret = -ENOMEM;
		goto p_err;
	}

	ret = copy_from_user(graph->desc_utask, (void *)info->addr, graph->size);
	if (ret) {
		vpu_ierr("copy_from_user is fail(%d)\n", graph, ret);
		goto p_err;
	}

	if (info->id != graph->desc_utask->id) {
		vpu_iwarn("gid(%d) is not same with tid(%d)\n", graph,
			info->id, graph->desc_utask->id);
		graph->desc_utask->id = info->id;
	}

	memcpy(graph->desc_mtask, graph->desc_utask, graph->size);

#ifdef DBG_PRINT_TASK
	vpu_graph_task_print(graph);
#endif

	/* 2. graph allocation */
	ret = __vpu_graph_alloc(graph);
	if (ret) {
		vpu_ierr("__vpu_graph_alloc is fail(%d)\n", graph, ret);
		goto p_err;
	}

	/* 3. parsing */
	ret = __vpu_graph_parse(graph);
	if (ret) {
		vpu_ierr("__vpu_graph_parse is fail(%d)\n", graph, ret);
		goto p_err;
	}

	/* 4. Buffer Mapping */
	ret = __vpu_graph_map(graph);
	if (ret) {
		vpu_ierr("__vpu_graph_map is fail(%d)\n", graph, ret);
		goto p_err;
	}

	set_bit(VPU_GRAPH_STATE_CONFIG, &graph->state);

p_err:
	vpu_iinfo("%s(%d, %d, %X):%d\n", graph, __func__, info->id, info->priority, info->flags, ret);
	return ret;
}

int vpu_graph_param(struct vpu_graph *graph, struct vs4l_param_list *plist)
{
	int ret = 0;
	struct vs4l_param *param;
	struct vpuo_pu *pu;
	char *pu_base;
	u32 i;

	if (!graph->update_cnt) {
		ret = -EINVAL;
		vpu_ierr("update_cnt is zero\n", graph);
		goto p_err;
	}

	for (i = 0; i < plist->count; ++i) {
		param = &plist->params[i];

		pu = __vpu_graph_find_pu(graph, param->target);
		if (!pu) {
			vpu_ierr("__vpu_graph_find_pu is fail(0x%X)\n", graph, param->target);
			ret = -EINVAL;
			goto p_err;
		}

		if (!pu->desc_mpu) {
			vpu_ierr("desc_mpu is NULL\n", graph);
			ret = -EINVAL;
			goto p_err;
		}

		pu_base = (char *)&pu->desc_mpu->params;
		ret = copy_from_user(pu_base + param->offset, (void *)param->addr, param->size);
		if (ret) {
			vpu_ierr("copy_from_user() is fail(%d)\n", graph, ret);
			goto p_err;
		}
	}

	set_bit(VPU_GRAPH_FLAG_UPDATE_PARAM, &graph->flags);

p_err:
	return ret;
}

static int vpu_graph_prepare(struct vb_queue *q, struct vb_container_list *clist)
{
	int ret = 0;
	u32 i, target;
	struct vpu_queue *queue = q->private_data;
	struct vpu_vertex_ctx *vctx;
	struct list_head *list;
	struct vpu_graph *graph;
	struct vpuo_pu *pu;

	BUG_ON(!queue);
	BUG_ON(!clist);

	vctx = container_of(queue, struct vpu_vertex_ctx, queue);
	graph = container_of(vctx, struct vpu_graph, vctx);

	if (test_bit(VS4L_GRAPH_FLAG_PRIMITIVE, &graph->flags))
		return 0;

	if (clist->index >= VPU_MAX_BUFFER) {
		vpu_ierr("clist index is invalid(%d)\n", graph, clist->index);
		BUG();
	}

	if (clist->direction == VS4L_DIRECTION_IN)
		list = &graph->inleaf_list;
	else
		list = &graph->otleaf_list;

	for (i = 0; i < clist->count; ++i) {
		target = clist->containers[i].target;

		pu = __vpu_graph_find_iopu(list, target);
		if (!pu) {
			vpu_ierr("(%d) target(%d) is NULL\n", graph, clist->direction, target);
			ret = -EINVAL;
			goto p_err;
		}

		if (pu->container[clist->index]) {
			vpu_ierr("clist index is already prepared(%d)\n", graph, clist->index);
			BUG();
		}

		pu->container[clist->index] = &clist->containers[i];
	}

p_err:
	vpu_iinfo("%s(%d):%d\n", graph, __func__, clist->index, ret);
	return ret;
}

static int vpu_graph_unprepare(struct vb_queue *q, struct vb_container_list *clist)
{
	int ret = 0;
	u32 i, target;
	struct vpu_queue *queue = q->private_data;
	struct vpu_vertex_ctx *vctx;
	struct list_head *list;
	struct vpu_graph *graph;
	struct vpuo_pu *pu;

	BUG_ON(!queue);
	BUG_ON(!clist);

	vctx = container_of(queue, struct vpu_vertex_ctx, queue);
	graph = container_of(vctx, struct vpu_graph, vctx);

	if (test_bit(VS4L_GRAPH_FLAG_PRIMITIVE, &graph->flags))
		return 0;

	if (clist->index >= VPU_MAX_BUFFER) {
		vpu_ierr("clist index is invalid(%d)\n", graph, clist->index);
		BUG();
	}

	if (clist->direction == VS4L_DIRECTION_IN)
		list = &graph->inleaf_list;
	else
		list = &graph->otleaf_list;

	for (i = 0; i < clist->count; ++i) {
		target = clist->containers[i].target;

		pu = __vpu_graph_find_iopu(list, target);
		if (!pu) {
			vpu_ierr("(%d) target(%d) is NULL\n", graph, clist->direction, target);
			ret = -EINVAL;
			goto p_err;
		}

		pu->container[clist->index] = NULL;
	}

p_err:
	vpu_iinfo("%s(%d):%d\n", graph, __func__, clist->index, ret);
	return ret;
}

const struct vb_ops vb_ops = {
	.buf_prepare = vpu_graph_prepare,
	.buf_unprepare = vpu_graph_unprepare
};

int vpu_graph_start(struct vpu_queue *queue)
{
	int ret = 0;
	struct vpu_vertex_ctx *vctx;
	struct vpu_graph *graph;

	BUG_ON(!queue);

	vctx = container_of(queue, struct vpu_vertex_ctx, queue);
	graph = container_of(vctx, struct vpu_graph, vctx);

	ret = __vpu_graph_start(graph);
	if (ret)
		vpu_ierr("__vpu_graph_start is fail(%d)\n", graph, ret);

	vpu_iinfo("%s():%d\n", graph, __func__, ret);
	return ret;
}

int vpu_graph_stop(struct vpu_queue *queue)
{
	int ret = 0;
	struct vpu_graph *graph;
	struct vpu_vertex_ctx *vctx;

	BUG_ON(!queue);

	vctx = container_of(queue, struct vpu_vertex_ctx, queue);
	graph = container_of(vctx, struct vpu_graph, vctx);

	ret = __vpu_graph_stop(graph);
	if (ret)
		vpu_ierr("__vpu_graph_stop is fail(%d)\n", graph, ret);

	vpu_iinfo("%s():%d\n", graph, __func__, ret);
	return 0;
}

int vpu_graph_format(struct vpu_queue *queue, struct vs4l_format_list *flist)
{
	int ret = 0;
	u32 i, target;
	char *fourcc;
	struct vpu_vertex_ctx *vctx;
	struct list_head *list;
	struct vpu_graph *graph;
	struct vpuo_chain *chain;
	struct vpuo_pu *pu, *temp;

	BUG_ON(!queue);
	BUG_ON(!flist);

	vctx = container_of(queue, struct vpu_vertex_ctx, queue);
	graph = container_of(vctx, struct vpu_graph, vctx);

	if (test_bit(VS4L_GRAPH_FLAG_PRIMITIVE, &graph->flags))
		return 0;

	if (flist->direction == VS4L_DIRECTION_IN)
		list = &graph->inleaf_list;
	else
		list = &graph->otleaf_list;

	for (i = 0; i < flist->count; ++i) {
		target = flist->formats[i].target;
		fourcc = (char *)&flist->formats[i].format;

		pu = __vpu_graph_find_iopu(list, target);
		if (!pu) {
			vpu_ierr("(%d) target(%d) is NULL\n", graph, flist->direction, target);
			ret = -EINVAL;
			goto p_err;
		}

		chain = pu->parent;
		if (!chain) {
			vpu_ierr("(%d) chain(%d) is NULL\n", graph, flist->direction, target);
			ret = -EINVAL;
			goto p_err;
		}

		vpu_iinfo("C%dP%d : %dx%d@%c%c%c%c\n", graph,
			chain->id, pu->id,
			flist->formats[i].width,
			flist->formats[i].height,
			fourcc[0], fourcc[1], fourcc[2], fourcc[3]);

		set_bit(VPUO_PU_STATE_FORMAT, &pu->state);
	}

	list_for_each_entry_safe(pu, temp, list, gleaf_entry) {
		if (!test_bit(VPUO_PU_STATE_FORMAT, &pu->state)) {
			vpu_ierr("pu %d is not formatted\n", graph, pu->id);
			ret = -EINVAL;
			goto p_err;
		}
	}

p_err:
	return ret;
}

static int vpu_graph_queue(struct vpu_queue *queue, struct vb_container_list *incl, struct vb_container_list *otcl)
{
	int ret = 0;
	unsigned long flag;
	struct vpu_graph *graph;
	struct vpu_vertex_ctx *vctx;
	struct vpu_framemgr *framemgr;
	struct vpu_frame *frame;
	struct vpuo_pu *pu, *temp;
	struct list_head *list;
	dma_addr_t dvaddr;
	u32 i, j;

	BUG_ON(!queue);
	BUG_ON(!incl);
	BUG_ON(!incl->index >= VPU_MAX_FRAME);
	BUG_ON(!otcl);
	BUG_ON(!otcl->index >= VPU_MAX_FRAME);

	vctx = container_of(queue, struct vpu_vertex_ctx, queue);
	graph = container_of(vctx, struct vpu_graph, vctx);
	framemgr = &graph->framemgr;

	if (!test_bit(VPU_GRAPH_STATE_START, &graph->state)) {
		vpu_ierr("graph is NOT start\n", graph);
		ret = -EINVAL;
		goto p_err;
	}

	if (incl->id != otcl->id) {
		vpu_warn("buffer id is incoincidence(%d, %d)\n", incl->id, otcl->id);
		otcl->id = incl->id;
	}

	framemgr_e_barrier_irqs(framemgr, 0, flag);
	vpu_frame_pick_fre_to_req(framemgr, &frame);
	framemgr_x_barrier_irqr(framemgr, 0, flag);

	if (!frame) {
		vpu_ierr("frame is lack\n", graph);
		vpu_frame_print_all(framemgr);
		ret = -ENOMEM;
		goto p_err;
	}

	if (test_bit(VS4L_GRAPH_FLAG_PRIMITIVE, &graph->flags)) {
		u32 index;

		if (graph->iobuffer_cnt != (incl->count + otcl->count)) {
			vpu_ierr("iobuffer cnt is invalid(%d %d)\n", graph, incl->count, otcl->count);
			BUG();
		}

		for (i = 0; i < incl->count; ++i) {
			index = incl->containers[i].target & VS4L_TARGET_PU;

			for (j = 0; j < graph->iobuffer_cnt; ++j) {
				if (index == graph->iobuffer_idx[j]) {
					dvaddr = incl->containers[i].buffers[0].dvaddr;
					if (dvaddr < VPU_AHB_BASE_ADDR) {
						vpu_ierr("dvaddr is invalid(%pa)\n", graph, &dvaddr);
						ret = -EINVAL;
						goto p_err;
					}

					graph->iobuffer_dat[j] = dvaddr - VPU_AHB_BASE_ADDR;
					break;
				}
			}

			if (j >= graph->iobuffer_cnt) {
				vpu_ierr("target %d is not found\n", graph, index);
				BUG();
			}
		}

		for (i = 0; i < otcl->count; ++i) {
			index = otcl->containers[i].target & VS4L_TARGET_PU;

			for (j = 0; j < graph->iobuffer_cnt; ++j) {
				if (index == graph->iobuffer_idx[j]) {
					dvaddr = otcl->containers[i].buffers[0].dvaddr;
					if (dvaddr < VPU_AHB_BASE_ADDR) {
						vpu_ierr("dvaddr is invalid(%pa)\n", graph, &dvaddr);
						ret = -EINVAL;
						goto p_err;
					}

					graph->iobuffer_dat[j] = dvaddr - VPU_AHB_BASE_ADDR;
					break;
				}
			}

			if (j >= graph->iobuffer_cnt) {
				vpu_ierr("target %d is not found\n", graph, index);
				BUG();
			}
		}

		goto p_skip_primitive;
	}

	list = &graph->inleaf_list;
	list_for_each_entry_safe(pu, temp, list, gleaf_entry) {
		if (pu->container[incl->index]->type != VS4L_BUFFER_LIST) {
			vpu_err("this buffer type is not supported\n");
			BUG();
		}

		for (i = 0; i < pu->container[incl->index]->count; ++i) {
			dvaddr = pu->container[incl->index]->buffers[i].dvaddr;
			if (dvaddr < VPU_AHB_BASE_ADDR) {
				vpu_ierr("dvaddr is invalid(%pa)\n", graph, &dvaddr);
				ret = -EINVAL;
				goto p_err;
			}

			if (!pu->buffer_ptr[i]) {
				vpu_ierr("%d pu buffer is NULL\n", graph, pu->id);
				BUG();
			}

			*pu->buffer_ptr[i] = dvaddr - VPU_AHB_BASE_ADDR;
		}
	}

	list = &graph->otleaf_list;
	list_for_each_entry_safe(pu, temp, list, gleaf_entry) {
		if (pu->container[otcl->index]->type != VS4L_BUFFER_LIST) {
			vpu_err("this buffer type is not supported\n");
			BUG();
		}

		for (i = 0; i < pu->container[otcl->index]->count; ++i) {
			dvaddr = pu->container[otcl->index]->buffers[i].dvaddr;
			if (dvaddr < VPU_AHB_BASE_ADDR) {
				vpu_ierr("dvaddr is invalid(%pa)\n", graph, &dvaddr);
				ret = -EINVAL;
				goto p_err;
			}

			if (pu->type == VPUL_OP_ROI) {
				set_bit(VPU_FRAME_FLAG_IOCPY, &frame->flags);
				continue;
			}

			if (!pu->buffer_ptr[i]) {
				vpu_ierr("%d pu buffer is NULL\n", graph, pu->id);
				BUG();
			}

			*pu->buffer_ptr[i] = dvaddr - VPU_AHB_BASE_ADDR;
		}
	}

p_skip_primitive:
	graph->inhash[incl->index] = frame->index;
	graph->othash[otcl->index] = frame->index;
	graph->input_cnt++;

	frame->id = incl->id;
	frame->incl = incl;
	frame->otcl = otcl;
	frame->message = VPU_FRAME_REQUEST;
	frame->param0 = 0;
	frame->param1 = 0;
	frame->param2 = 0;
	frame->param3 = 0;
	clear_bit(VS4L_CL_FLAG_TIMESTAMP, &frame->flags);

	if ((incl->flags & (1 << VS4L_CL_FLAG_TIMESTAMP)) ||
		(otcl->flags & (1 << VS4L_CL_FLAG_TIMESTAMP))) {
		set_bit(VS4L_CL_FLAG_TIMESTAMP, &frame->flags);
		vpu_get_timestamp(&frame->time[VPU_TMP_QUEUE]);
	}

	vpu_graphmgr_queue(graph->cookie, frame);

p_err:
	return ret;
}

static int vpu_graph_deque(struct vpu_queue *queue, struct vb_container_list *clist)
{
	int ret = 0;
	u32 findex;
	unsigned long flags;
	struct vpu_graph *graph;
	struct vpu_vertex_ctx *vctx;
	struct vpu_framemgr *framemgr;
	struct vpu_frame *frame;

	BUG_ON(!queue);
	BUG_ON(!clist);
	BUG_ON(!clist->index >= VPU_MAX_FRAME);

	vctx = container_of(queue, struct vpu_vertex_ctx, queue);
	graph = container_of(vctx, struct vpu_graph, vctx);
	framemgr = &graph->framemgr;

	if (!test_bit(VPU_GRAPH_STATE_START, &graph->state)) {
		vpu_ierr("graph is NOT start\n", graph);
		ret = -EINVAL;
		goto p_err;
	}

	if (clist->direction == VS4L_DIRECTION_IN)
		findex = graph->inhash[clist->index];
	else
		findex = graph->othash[clist->index];

	if (findex >= VPU_MAX_FRAME) {
		vpu_ierr("frame index(%d) invalid\n", graph, findex);
		BUG();
	}

	frame = &framemgr->frame[findex];
	if (frame->state != VPU_FRAME_STATE_COMPLETE) {
		vpu_ierr("frame state(%d) is invalid\n", graph, frame->state);
		BUG();
	}

	if (clist->direction == VS4L_DIRECTION_IN) {
		if (frame->incl != clist) {
			vpu_ierr("incl ptr is invalid(%p != %p)\n", graph, frame->incl, clist);
			BUG();
		}

		graph->inhash[clist->index] = VPU_MAX_FRAME;
		frame->incl = NULL;
	} else {
		if (frame->otcl != clist) {
			vpu_ierr("otcl ptr is invalid(%p != %p)\n", graph, frame->otcl, clist);
			BUG();
		}

		graph->othash[clist->index] = VPU_MAX_FRAME;
		frame->otcl = NULL;
	}

	if (frame->incl || frame->otcl)
		goto p_err;

	framemgr_e_barrier_irqs(framemgr, 0, flags);
	vpu_frame_trans_com_to_fre(framemgr, frame);
	framemgr_x_barrier_irqr(framemgr, 0, flags);

p_err:
	return ret;
}

const struct vpu_queue_ops vpu_queue_ops = {
	.start		= vpu_graph_start,
	.stop		= vpu_graph_stop,
	.format 	= vpu_graph_format,
	.queue		= vpu_graph_queue,
	.deque		= vpu_graph_deque
};

static int vpu_graph_control(struct vpu_graph *graph, struct vpu_frame *frame)
{
	int ret = 0;
	struct vpu_framemgr *framemgr;

	BUG_ON(!graph);
	BUG_ON(!frame);

	framemgr = &graph->framemgr;

	if (&graph->control != frame) {
		vpu_ierr("control frame is invalid(%p == %p)\n", graph, &graph->control, frame);
		BUG();
	}

	switch (frame->message) {
	case VPU_CTRL_STOP:
		graph->control.message = VPU_CTRL_STOP_DONE;
		wake_up(&graph->control_wq);
		break;
	default:
		vpu_ierr("unresolved message(%d)\n", graph, frame->message);
		vpu_frame_print_all(framemgr);
		BUG();
		break;
	}

	return ret;
}

static int vpu_graph_request(struct vpu_graph *graph, struct vpu_frame *frame)
{
	int ret = 0;
	unsigned long flags;
	struct vpu_framemgr *framemgr;

	BUG_ON(!graph);
	BUG_ON(!frame);

	framemgr = &graph->framemgr;

	if (frame->state != VPU_FRAME_STATE_REQUEST) {
		vpu_ierr("frame state(%d) is invalid\n", graph, frame->state);
		BUG();
	}

	framemgr_e_barrier_irqs(framemgr, 0, flags);
	vpu_frame_trans_req_to_pre(framemgr, frame);
	framemgr_x_barrier_irqr(framemgr, 0, flags);

	if (test_bit(VS4L_CL_FLAG_TIMESTAMP, &frame->flags))
		vpu_get_timestamp(&frame->time[VPU_TMP_REQUEST]);

	return ret;
}

static int vpu_graph_process(struct vpu_graph *graph, struct vpu_frame *frame)
{
	int ret = 0;
	unsigned long flags;
	struct vpu_framemgr *framemgr;

	BUG_ON(!graph);
	BUG_ON(!frame);

	framemgr = &graph->framemgr;

	if (frame->state != VPU_FRAME_STATE_PREPARE) {
		vpu_ierr("frame state(%d) is invalid\n", graph, frame->state);
		BUG();
	}

	framemgr_e_barrier_irqs(framemgr, FMGR_IDX_0, flags);
	vpu_frame_trans_pre_to_pro(framemgr, frame);
	framemgr_x_barrier_irqr(framemgr, FMGR_IDX_0, flags);

#ifdef DBG_STREAMING
	vpu_iinfo("PROCESS(%d, %d)\n", graph, frame->index, frame->id);
#endif

	if (test_bit(VS4L_CL_FLAG_TIMESTAMP, &frame->flags))
		vpu_get_timestamp(&frame->time[VPU_TMP_PROCESS]);

	return ret;
}

static int vpu_graph_cancel(struct vpu_graph *graph, struct vpu_frame *frame)
{
	int ret = 0;
	unsigned long flags;
	unsigned long result;
	struct vpu_framemgr *framemgr;
	struct vpu_queue *queue;
	struct vb_container_list *incl, *otcl;

	BUG_ON(!graph);
	BUG_ON(!frame);

	framemgr = &graph->framemgr;
	queue = &graph->vctx.queue;
	incl = frame->incl;
	otcl = frame->otcl;
	result = 0;

	if (!test_bit(VPU_GRAPH_STATE_START, &graph->state)) {
		vpu_ierr("graph is NOT start\n", graph);
		BUG();
	}

	if (test_bit(VS4L_CL_FLAG_TIMESTAMP, &frame->flags)) {
		vpu_get_timestamp(&frame->time[VPU_TMP_DONE]);

		if (incl->flags & (1 << VS4L_CL_FLAG_TIMESTAMP))
			memcpy(incl->timestamp, frame->time, sizeof(frame->time));

		if (otcl->flags & (1 << VS4L_CL_FLAG_TIMESTAMP))
			memcpy(otcl->timestamp, frame->time, sizeof(frame->time));

#ifdef DBG_TIMEMEASURE
		vpu_irinfo("[TM] G%d : QR(%ld), RP(%ld), PD(%ld)\n", graph, frame, graph->uid,
			VPU_TIME_IN_US(frame->time[VPU_TMP_REQUEST]) -
			VPU_TIME_IN_US(frame->time[VPU_TMP_QUEUE]),
			VPU_TIME_IN_US(frame->time[VPU_TMP_PROCESS]) -
			VPU_TIME_IN_US(frame->time[VPU_TMP_REQUEST]),
			VPU_TIME_IN_US(frame->time[VPU_TMP_DONE]) -
			VPU_TIME_IN_US(frame->time[VPU_TMP_PROCESS]));
#endif
	}

	vpu_iinfo("CANCEL(%d, %d)\n", graph, frame->index, frame->id);
	set_bit(VS4L_CL_FLAG_INVALID, &result);

	switch (frame->state) {
	case VPU_FRAME_STATE_PREPARE:
		framemgr_e_barrier_irqs(framemgr, FMGR_IDX_0, flags);
		vpu_frame_trans_pre_to_com(framemgr, frame);
		framemgr_x_barrier_irqr(framemgr, FMGR_IDX_0, flags);
		break;
	case VPU_FRAME_STATE_PROCESS:
		framemgr_e_barrier_irqs(framemgr, FMGR_IDX_0, flags);
		vpu_frame_trans_pro_to_com(framemgr, frame);
		framemgr_x_barrier_irqr(framemgr, FMGR_IDX_0, flags);
		break;
	default:
		vpu_ierr("frame is invalid(%d)\n", graph, frame->state);
		vpu_frame_print_all(framemgr);
		BUG();
		break;
	}

	graph->cancel_cnt++;
	vpu_queue_done(queue, incl, otcl, result);

	return ret;
}

static int vpu_graph_done(struct vpu_graph *graph, struct vpu_frame *frame)
{
	int ret = 0;
	unsigned long flags;
	unsigned long result;
	struct vpu_framemgr *framemgr;
	struct vpu_queue *queue;
	struct vb_container_list *incl, *otcl;

	BUG_ON(!graph);
	BUG_ON(!frame);

	framemgr = &graph->framemgr;
	queue = &graph->vctx.queue;
	incl = frame->incl;
	otcl = frame->otcl;
	result = 0;

	if (!test_bit(VPU_GRAPH_STATE_START, &graph->state)) {
		vpu_ierr("graph is NOT start\n", graph);
		BUG();
	}

	if (frame->state != VPU_FRAME_STATE_PROCESS) {
		vpu_ierr("frame state(%d) is invalid\n", graph, frame->state);
		BUG();
	}

	if (test_bit(VS4L_CL_FLAG_TIMESTAMP, &frame->flags)) {
		vpu_get_timestamp(&frame->time[VPU_TMP_DONE]);

		if (incl->flags & (1 << VS4L_CL_FLAG_TIMESTAMP))
			memcpy(incl->timestamp, frame->time, sizeof(frame->time));

		if (otcl->flags & (1 << VS4L_CL_FLAG_TIMESTAMP))
			memcpy(otcl->timestamp, frame->time, sizeof(frame->time));

#ifdef DBG_TIMEMEASURE
		vpu_irinfo("[TM] G%d : QR(%ld), RR(%ld), RP(%ld), PD(%ld)\n", graph, frame, graph->uid,
			VPU_TIME_IN_US(frame->time[VPU_TMP_REQUEST]) -
			VPU_TIME_IN_US(frame->time[VPU_TMP_QUEUE]),
			VPU_TIME_IN_US(frame->time[VPU_TMP_RESOURCE]) -
			VPU_TIME_IN_US(frame->time[VPU_TMP_REQUEST]),
			VPU_TIME_IN_US(frame->time[VPU_TMP_PROCESS]) -
			VPU_TIME_IN_US(frame->time[VPU_TMP_RESOURCE]),
			VPU_TIME_IN_US(frame->time[VPU_TMP_DONE]) -
			VPU_TIME_IN_US(frame->time[VPU_TMP_PROCESS]));
#endif
	}

	if (frame->param0) {
#ifdef DBG_STREAMING
		vpu_iinfo("NDONE(%d, %d)\n", graph, frame->index, frame->id);
#endif
		set_bit(VS4L_CL_FLAG_DONE, &result);
		set_bit(VS4L_CL_FLAG_INVALID, &result);
	} else {
#ifdef DBG_STREAMING
		vpu_iinfo("DONE(%d, %d)\n", graph, frame->index, frame->id);
#endif
		set_bit(VS4L_CL_FLAG_DONE, &result);
	}

	if (test_bit(VPU_FRAME_FLAG_IOCPY, &frame->flags)) {
		/* HACK : direct fetch the result from rois register */
		u32 i, target, instance;
		u32 *result;
		struct vpuo_pu *pu;

		for (i = 0; i < frame->otcl->count; ++i) {
			result = frame->otcl->containers[i].buffers[0].kvaddr;
			target = frame->otcl->containers[i].target;
			instance = target & VS4L_TARGET_PU;
			if ((instance >= VPU_PU_ROIS0) && (instance <= VPU_PU_ROIS1)) {
				pu = __vpu_graph_find_iopu(&graph->otleaf_list, target);
				if (!pu) {
					vpu_err("pu %d is NOT found\n", instance);
					BUG();
				}

				result[0] = readl(pu->sfr + 0x1C);
				result[1] = readl(pu->sfr + 0x20);
				result[2] = readl(pu->sfr + 0x24);
				result[3] = readl(pu->sfr + 0x28);
				result[4] = readl(pu->sfr + 0x2C);
				result[5] = readl(pu->sfr + 0x30);
			}
		}
	}

	framemgr_e_barrier_irqs(framemgr, FMGR_IDX_0, flags);
	vpu_frame_trans_pro_to_com(framemgr, frame);
	framemgr_x_barrier_irqr(framemgr, FMGR_IDX_0, flags);

	graph->recent = frame->id;
	graph->done_cnt++;
	vpu_queue_done(queue, incl, otcl, result);

	return ret;
}

static int vpu_graph_get_resource(struct vpu_graph *graph, struct vpu_frame *frame)
{
	int ret = 0;

	if (!test_bit(VPU_GRAPH_STATE_START, &graph->state)) {
		vpu_ierr("graph is NOT start\n", graph);
		BUG();
	}

	if (test_bit(VPU_GRAPH_STATE_HMAPPED, &graph->state))
		goto p_err;

	if (test_bit(VS4L_GRAPH_FLAG_BYPASS, &graph->flags))
		goto p_skip;

	ret = vpu_resource_get(graph->resource, graph->desc_mtask, graph->flags);
	if (ret > 0)
		goto p_err;

	if (ret < 0) {
		vpu_ierr("vpu_resource_get is fail(%d)\n", graph, ret);
		goto p_err;
	}

p_skip:
	if (test_bit(VS4L_CL_FLAG_TIMESTAMP, &frame->flags))
		vpu_get_timestamp(&frame->time[VPU_TMP_RESOURCE]);

	set_bit(VPU_GRAPH_STATE_HMAPPED, &graph->state);

p_err:
	return ret;
}

static int vpu_graph_put_resource(struct vpu_graph *graph, struct vpu_frame *frame)
{
	int ret = 0;

	if (!test_bit(VPU_GRAPH_STATE_START, &graph->state)) {
		vpu_ierr("graph is NOT start\n", graph);
		BUG();
	}

	if (!test_bit(VPU_GRAPH_STATE_HMAPPED, &graph->state))
		goto p_err;

	if (test_bit(VS4L_GRAPH_FLAG_BYPASS, &graph->flags))
		goto p_skip;

	ret = vpu_resource_put(graph->resource, graph->desc_mtask);
	if (ret) {
		vpu_irerr("vpu_resource_put ia fail(%d)\n", graph, frame, ret);
		goto p_err;
	}

p_skip:
	clear_bit(VPU_GRAPH_STATE_HMAPPED, &graph->state);

p_err:
	return ret;
}

static int vpu_graph_update_param(struct vpu_graph *graph, struct vpu_frame *frame)
{
	int ret = 0;
	struct vpul_task *task;
	struct vpul_pu_location *pu_loc;
	union vpul_pu_parameters *update_array;
	struct vpuo_vertex *vertex;
	struct vpuo_chain *chain;
	struct vpuo_pu *pu;
	u32 i ;

	task = graph->desc_mtask;
	pu_loc = fst_updateble_pu_location_ptr(task);

	if (!graph->update_cnt)
		goto p_err;

	update_array = graph->update_array;
	if (!update_array) {
		vpu_err("update_vector is NULL\n");
		BUG();
	}

	for (i = 0; i < graph->update_cnt; ++i) {
		if (pu_loc[i].vtx_idx >= graph->vertex_cnt) {
			vpu_err("vertex index is invalid(%d, %d)\n",
				pu_loc[i].vtx_idx, graph->vertex_cnt);
			BUG();
		}

		vertex = graph->vertex_array[pu_loc[i].vtx_idx];
		if (!vertex) {
			vpu_err("vertex is NULL\n");
			BUG();
		}

		if (pu_loc[i].sc_idx_in_vtx >= vertex->chain_cnt) {
			vpu_err("chain index is invalid(%d, %d)\n",
				pu_loc[i].sc_idx_in_vtx, vertex->chain_cnt);
			BUG();
		}

		chain = vertex->chain_array[pu_loc[i].sc_idx_in_vtx];
		if (!chain) {
			vpu_err("chain is NULL\n");
			BUG();
		}

		if (pu_loc[i].pu_idx_in_sc >= chain->pu_cnt) {
			vpu_err("pu index is invalid(%d, %d)\n",
				pu_loc[i].pu_idx_in_sc, chain->pu_cnt);
			BUG();
		}

		pu = chain->pu_array[pu_loc[i].pu_idx_in_sc];
		if (!pu) {
			vpu_err("pu is NULL\n");
			BUG();
		}

		if (!pu->desc_mpu) {
			vpu_err("desc_mpu is NULL\n");
			BUG();
		}

		memcpy(&update_array[i], &pu->desc_mpu->params, sizeof(union vpul_pu_parameters));
	}

p_err:
	return ret;
}

const struct vpu_graph_ops vpu_graph_ops = {
	.control	= vpu_graph_control,
	.request	= vpu_graph_request,
	.process	= vpu_graph_process,
	.cancel 	= vpu_graph_cancel,
	.done		= vpu_graph_done,
	.get_resource	= vpu_graph_get_resource,
	.put_resource	= vpu_graph_put_resource,
	.update_param	= vpu_graph_update_param
};
