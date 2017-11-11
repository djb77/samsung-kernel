/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/types.h>
#include <linux/slab.h>

#include "vpu-config.h"
#include "vpu-exynos.h"
#include "vpu-graph.h"
#include "vpuo-vertex.h"
#include "vpuo-chain.h"
#include "vpuo-pu.h"

int vpuo_pu_create(struct vpuo_pu **pu,
	char *desc_ubase,
	char *desc_mbase,
	struct vpul_pu *desc_upu,
	struct vpul_pu *desc_mpu,
	void *parent)
{
	int ret = 0;
	struct vpul_task *desc_task;
	struct vpu_graph *graph;
	struct vpuo_vertex *vertex;
	struct vpuo_chain *chain;
	struct vpu_exynos *exynos;
	u32 i;

	BUG_ON(!pu);
	BUG_ON(!desc_ubase);
	BUG_ON(!desc_mbase);
	BUG_ON(!desc_upu);
	BUG_ON(!desc_mpu);
	BUG_ON(!parent);

	chain = parent;
	vertex = chain->parent;
	graph = vertex->parent;
	exynos = graph->exynos;
	desc_task = (struct vpul_task *)desc_mbase;

	*pu = kzalloc(sizeof(struct vpuo_pu), GFP_KERNEL);
	if (*pu == NULL) {
		vpu_err("kzalloc is fail");
		ret = -ENOMEM;
		goto p_err;
	}

	(*pu)->id = desc_upu->instance;
	(*pu)->index = ((char *)desc_upu - (desc_ubase + desc_task->pus_vec_ofs)) / sizeof(struct vpul_pu);
	(*pu)->type = desc_upu->op_type;
	(*pu)->level = VPUO_CHAIN_MAX_LEVEL;
	(*pu)->target = (chain->id << VS4L_TARGET_SC_SHIFT) | desc_upu->instance;
	(*pu)->parent = parent;
	(*pu)->buffer_cnt = 0;
	(*pu)->inpu_cnt = 0;
	(*pu)->otpu_cnt = 0;
	(*pu)->desc_upu = desc_upu;
	(*pu)->desc_mpu = desc_mpu;
	clear_bit(VPUO_PU_STATE_FORMAT, &(*pu)->state);
	clear_bit(VPUO_PU_STATE_MAPPED, &(*pu)->state);
	clear_bit(VPUO_PU_STATE_IN, &(*pu)->state);
	clear_bit(VPUO_PU_STATE_OT, &(*pu)->state);
	clear_bit(VPUO_PU_STATE_IM, &(*pu)->state);

	(*pu)->sfr = CTL_OP(exynos, ctl_remap, (*pu)->id);
	if (!(*pu)->sfr) {
		vpu_err("CTL_OP(remap) is fail(%d)\n", ret);
		goto p_err;
	}

	for (i = 0; i < VPUO_PU_MAX_PORT; ++i) {
		(*pu)->inpu[i] = NULL;
		(*pu)->otpu[i] = NULL;
	}

	for (i = 0; i < VPUO_PU_MAX_BUFFER; ++i) {
		(*pu)->buffer_idx[i] = VPUL_MAX_MAPS_DESC;
		(*pu)->buffer_ptr[i] = NULL;
		(*pu)->buffer_shm[i] = 0;
		(*pu)->buffer[i] = 0;
	}

	switch ((*pu)->type) {
	case VPUL_OP_DMA:
		do {
			struct vpuo_vertex *vertex;
			struct vpul_vertex *desc_vertex;
			u32 io_index, map_index, ext_index, mtype;

			vertex = chain->parent;
			if (!vertex) {
				vpu_err("vertex is NULL\n");
				BUG();
			}

			desc_vertex = vertex->desc_mvertex;
			if (!desc_vertex) {
				vpu_err("desc_vertex is NULL\n");
				BUG();
			}

			io_index = desc_upu->params.dma.inout_index;
			if (io_index >= VPUL_MAX_IN_OUT_TYPES) {
				vpu_err("io_index is invalid(%d)\n", io_index);
				ret = -EINVAL;
				goto p_err;
			}

			if (vertex->type == VPUL_VERTEXT_PROC) {
				u32 roi_index;

				roi_index = desc_vertex->proc.io.inout_types[io_index].roi_index;
				if (desc_vertex->proc.io.inout_types[io_index].is_dynamic) {
					if (roi_index >= desc_vertex->proc.io.n_dynamic_map_rois) {
						vpu_err("roi_index is invalid(%d > %d)\n", roi_index,
							desc_vertex->proc.io.n_dynamic_map_rois);
						ret = -EINVAL;
						goto p_err;
					}

					map_index = desc_vertex->proc.io.dynamic_map_roi[roi_index].memmap_idx;
				} else {
					if (roi_index >= desc_vertex->proc.io.n_fixed_map_roi) {
						vpu_err("roi_index is invalid(%d, %d)\n", roi_index,
							desc_vertex->proc.io.n_fixed_map_roi);
						ret = -EINVAL;
						goto p_err;
					}

					map_index = desc_vertex->proc.io.fixed_map_roi[roi_index].memmap_idx;
				}

				mtype = desc_task->memmap_desc[map_index].mtype;
				if (mtype != VPUL_MEM_EXTERNAL)
					break;

				ext_index = desc_task->memmap_desc[map_index].index;
				if (ext_index >= desc_task->n_external_mem_addresses) {
					vpu_err("memory index is invalid(%d)\n", ext_index);
					BUG();
				}

				(*pu)->buffer_cnt = 1;
				(*pu)->buffer_idx[0] = ext_index;
				(*pu)->buffer_ptr[0] = &desc_task->external_mem_addr[ext_index];
				(*pu)->buffer[0] = desc_task->external_mem_addr[ext_index];
			} else if (vertex->type == VPUL_VERTEXT_3DNN_PROC) {
				struct vpul_3dnn_process_base *process3_base, *process3;

				process3_base = (struct vpul_3dnn_process_base *)(desc_mbase + desc_task->process_bases_3dnn_vec_ofs);
				process3 = &process3_base[desc_vertex->proc3dnn.base_3dnn_ind];

				for (i = 0; i < process3->number_of_layers; ++i) {
					map_index = process3->layers[i].inout_3dnn[io_index].mem_descr_index;
					mtype = desc_task->memmap_desc[map_index].mtype;
					if (mtype != VPUL_MEM_EXTERNAL)
						continue;

					ext_index = desc_task->memmap_desc[map_index].index;
					if (ext_index >= desc_task->n_external_mem_addresses) {
						vpu_err("memory index is invalid(%d)\n", ext_index);
						BUG();
					}

					if (ext_index >= VPUL_MAX_TASK_EXTERNAL_RAMS) {
						vpu_err("ext_index is over(%d %d)\n", ext_index, VPUL_MAX_TASK_EXTERNAL_RAMS);
						BUG();
					}

					(*pu)->buffer_cnt++;
					(*pu)->buffer_idx[i] = ext_index;
					(*pu)->buffer_ptr[i] = &desc_task->external_mem_addr[ext_index];
					(*pu)->buffer[i] = desc_task->external_mem_addr[ext_index];
				}
			} else {
				vpu_err("vertex type is invalid(%d)\n", vertex->type);
				BUG();
			}

			if ((*pu)->id <= VPU_PU_DMAIN_WIDE1)
				set_bit(VPUO_PU_STATE_IN, &(*pu)->state);
			else
				set_bit(VPUO_PU_STATE_OT, &(*pu)->state);
		} while (0);
		break;
	case VPUL_OP_ROI:
		do {
			(*pu)->buffer_cnt = 0;

			set_bit(VPUO_PU_STATE_OT, &(*pu)->state);
		} while (0);
		break;
	default:
		break;
	}

p_err:
	return ret;
}

int vpuo_pu_destroy(struct vpuo_pu *pu)
{
	int ret = 0;

	BUG_ON(!pu);

	kfree(pu);

	return ret;
}