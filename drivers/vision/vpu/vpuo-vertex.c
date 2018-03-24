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
#include "vpu-debug.h"
#include "vpu-graph.h"
#include "vpuo-vertex.h"
#include "vpuo-chain.h"
#include "vpuo-pu.h"

int vpuo_vertex_create(struct vpuo_vertex **vertex,
	char *desc_ubase,
	char *desc_mbase,
	struct vpul_vertex *desc_uvertex,
	struct vpul_vertex *desc_mvertex,
	void *parent)
{
	int ret = 0;
	u32 i, chain_cnt;
	struct vpu_graph *graph;
	struct vpul_subchain *desc_uchain, *desc_mchain;
	struct vpuo_chain *chain;

	BUG_ON(!vertex);
	BUG_ON(!desc_ubase);
	BUG_ON(!desc_mbase);
	BUG_ON(!desc_uvertex);
	BUG_ON(!desc_mvertex);
	BUG_ON(!parent);

	graph = parent;

	chain_cnt = desc_uvertex->num_of_subchains;
	if (chain_cnt >= VPU_VERTEX_MAX_CHAIN) {
		vpu_err("chain count is invalid(%d)\n", chain_cnt);
		return -EINVAL;
	}

	*vertex = kzalloc(sizeof(struct vpuo_vertex), GFP_KERNEL);
	if (*vertex == NULL) {
		vpu_err("kzalloc is fail");
		ret = -ENOMEM;
		goto p_err;
	}

	(*vertex)->id = graph->vertex_cnt;
	(*vertex)->type = desc_uvertex->vtype;
	(*vertex)->level = VPU_GRAPH_MAX_LEVEL;
	(*vertex)->parent = parent;
	(*vertex)->desc_uvertex = desc_uvertex;
	(*vertex)->desc_mvertex = desc_mvertex;

	if ((*vertex)->type == VPUL_VERTEXT_3DNN_PROC) {
		struct vpul_task *desc_task;
		struct vpul_3dnn_process_base *process3_base, *process3;

		desc_task = (struct vpul_task *)desc_mbase;
		process3_base = (struct vpul_3dnn_process_base *)(desc_mbase + desc_task->process_bases_3dnn_vec_ofs);
		process3 = &process3_base[desc_uvertex->proc3dnn.base_3dnn_ind];
		(*vertex)->layers = process3->number_of_layers;
	} else {
		(*vertex)->layers = 1;
	}

	(*vertex)->invertex_cnt = 0;
	(*vertex)->otvertex_cnt = 0;
	for (i = 0; i < VPU_VERTEX_MAX_PORT; ++i) {
		(*vertex)->invertex[i] = NULL;
		(*vertex)->otvertex[i] = NULL;
	}

	(*vertex)->chain_cnt = 0;
	for (i = 0; i < VPU_VERTEX_MAX_CHAIN; ++i) {
		(*vertex)->chain_table[i] = VPU_VERTEX_MAX_CHAIN;
		(*vertex)->chain_array[i] = NULL;
	}

	desc_uchain = (struct vpul_subchain *)(desc_ubase + desc_uvertex->sc_ofs);
	desc_mchain = (struct vpul_subchain *)(desc_mbase + desc_mvertex->sc_ofs);
	for (i = 0; i < chain_cnt; ++i) {
		ret = vpuo_chain_create(&chain, desc_ubase, desc_mbase, &desc_uchain[i], &desc_mchain[i], *vertex);
		if (ret) {
			vpu_err("vpuo_chain_create is fail(%d)\n", ret);
			ret = -EINVAL;
			goto p_err;
		}

		if (!chain) {
			vpu_err("chain is NULL\n");
			ret = -EINVAL;
			goto p_err;
		}

		(*vertex)->chain_array[i] = chain;
		(*vertex)->chain_table[i] = chain->id;
		(*vertex)->chain_cnt++;

		BUG_ON((*vertex)->chain_cnt >= VPU_VERTEX_MAX_CHAIN);
	}

	return 0;

p_err:
	vpuo_vertex_destroy(*vertex);
	return ret;
}

int vpuo_vertex_destroy(struct vpuo_vertex *vertex)
{
	int ret = 0;
	u32 i, chain_cnt;
	struct vpuo_chain *chain;

	chain_cnt = vertex->chain_cnt;
	for (i = 0; i < chain_cnt; ++i) {
		chain = vertex->chain_array[i];
		if (!chain) {
			vpu_err("chain is NULL\n");
			BUG();
		}

		ret = vpuo_chain_destroy(chain);
		if (ret) {
			vpu_err("vpuo_chain_destroy is fail(%d)\n", ret);
			continue;
		}

		vertex->chain_table[i] = VPU_VERTEX_MAX_CHAIN;
		vertex->chain_array[i] = NULL;
		vertex->chain_cnt--;
	}

	kfree(vertex);

	return ret;
}

int vpuo_vertex_parse(struct vpuo_vertex *vertex)
{
	int ret = 0;
	struct list_head *inleaf_list, *otleaf_list;
	struct vpuo_chain *chain, *next, *prev;
	struct vpuo_pu *inpu, *otpu, *t1, *t2;
	u32 i, j, chain_cnt;
	u32 next_layer, prev_layer, matched;

	BUG_ON(!vertex);

	/* 1. parsing each chain */
	chain_cnt = vertex->chain_cnt;
	for (i = 0; i < chain_cnt; ++i) {
		chain = vertex->chain_array[i];
		if (!chain) {
			vpu_err("chain is NULL(%d)\n", i);
			BUG();
		}

		ret = vpuo_chain_parse(chain);
		if (ret) {
			vpu_err("vpuo_chain_parse is fail(%d)\n", ret);
			goto p_err;
		}
	}

	/* 2. weaving */
	prev = NULL;
	for (i = 0; i < chain_cnt; ++i) {
		next = vertex->chain_array[i];
		if (!next) {
			vpu_err("next is NULL(%d)\n", i);
			BUG();
		}

		if (!prev) {
			prev = next;
			continue;
		}

		if (next->inchain_cnt >= VPUO_CHAIN_MAX_PORT) {
			vpu_err("inchain_cnt is invalid(%d)\n", next->inchain_cnt);
			ret = -EINVAL;
			goto p_err;
		}

		if (prev->otchain_cnt >= VPUO_CHAIN_MAX_PORT) {
			vpu_err("otchain_cnt is invalid(%d)\n", prev->otchain_cnt);
			ret = -EINVAL;
			goto p_err;
		}

		next->inchain[next->inchain_cnt] = prev;
		next->inchain_cnt++;
		prev->otchain[prev->otchain_cnt] = next;
		prev->otchain_cnt++;

		prev = next;

		BUG_ON(next->inchain_cnt >= VPUO_CHAIN_MAX_PORT);
		BUG_ON(prev->otchain_cnt >= VPUO_CHAIN_MAX_PORT);
	}

	/* 3. stitching */
	prev = NULL;
	prev_layer = 0;
	for (i = 0; i < vertex->layers; ++i) {
		for (j = 0; j < chain_cnt; ++j) {
			next_layer = i;
			next = vertex->chain_array[j];
			if (!next) {
				vpu_err("next is NULL\n");
				BUG();
			}

			if (!prev) {
				prev_layer = next_layer;
				prev = next;
				continue;
			}

			otleaf_list = &prev->otleaf_list;
			inleaf_list = &next->inleaf_list;
			list_for_each_entry_safe(otpu, t1, otleaf_list, cleaf_entry) {
				if (otpu->buffer_cnt == 0)
					continue;

				matched = 0;
				list_for_each_entry_safe(inpu, t2, inleaf_list, cleaf_entry) {
					if (inpu->buffer_cnt == 0)
						continue;

					if ((otpu->buffer[prev_layer] == inpu->buffer[next_layer]) &&
						(otpu->buffer[prev_layer] != 0))
						matched =  1;

					if (otpu->buffer_idx[prev_layer] == inpu->buffer_idx[next_layer])
						matched =  1;

					if (matched) {
						inpu->buffer_shm[next_layer]++;
						inpu->inpu[inpu->inpu_cnt] = otpu;
						inpu->inpu_cnt++;
						if (inpu->inpu_cnt >= inpu->buffer_cnt)
							set_bit(VPUO_PU_STATE_IM, &inpu->state);

						otpu->buffer_shm[prev_layer]++;
						otpu->otpu[otpu->otpu_cnt] = inpu;
						otpu->otpu_cnt++;
						if (otpu->otpu_cnt >= otpu->buffer_cnt)
							set_bit(VPUO_PU_STATE_IM, &otpu->state);

						BUG_ON(inpu->inpu_cnt >= VPUO_PU_MAX_PORT);
						BUG_ON(otpu->otpu_cnt >= VPUO_PU_MAX_PORT);
						break;
					}
				}
			}

			prev_layer = next_layer;
			prev = next;
		}
	}

	/* 4. pick up leaf */
	vertex->inleaf_cnt = 0;
	INIT_LIST_HEAD(&vertex->inleaf_list);
	vertex->otleaf_cnt = 0;
	INIT_LIST_HEAD(&vertex->otleaf_list);

	for (i = 0; i < chain_cnt; ++i) {
		chain = vertex->chain_array[i];
		if (!chain) {
			vpu_err("chain is NULL\n");
			BUG();
		}

		inleaf_list = &chain->inleaf_list;
		list_for_each_entry_safe(inpu, t1, inleaf_list, cleaf_entry) {
			if (test_bit(VPUO_PU_STATE_IM, &inpu->state)) {
				vpu_info("skip im %d\n", inpu->id);
				continue;
			}

			vertex->inleaf_cnt++;
			list_add_tail(&inpu->vleaf_entry, &vertex->inleaf_list);
		}

		otleaf_list = &chain->otleaf_list;
		list_for_each_entry_safe(otpu, t2, otleaf_list, cleaf_entry) {
			if (test_bit(VPUO_PU_STATE_IM, &otpu->state))
				continue;

			vertex->otleaf_cnt++;
			list_add_tail(&otpu->vleaf_entry, &vertex->otleaf_list);
		}
	}

p_err:
	return ret;
}

int vpuo_vertex_print(struct vpuo_vertex *vertex)
{
	DLOG_INIT();
	u32 i, j, chain_cnt;
	struct vpuo_chain *chain;

	vpu_info("[VERTEX : %d]\n", vertex->id);

	chain_cnt = vertex->chain_cnt;
	for (i = 0; i < chain_cnt; ++i) {
		chain = vertex->chain_array[i];
		if (!chain) {
			vpu_err("chain is NULL(%d)\n", i);
			BUG();
		}

		DLOG("lvl%d : %d(", i, chain->id);
		if (chain->otchain_cnt >= 1) {
			for (j = 0; j < (chain->otchain_cnt - 1); ++j)
				DLOG("%d ", chain->otchain[j]->id);
			DLOG("%d) ", chain->otchain[j]->id);
		} else {
			DLOG(") ");
		}

		vpu_info("%s\n", DLOG_OUT());
	}

	for (i = 0; i < chain_cnt; ++i) {
		chain = vertex->chain_array[i];
		vpuo_chain_print(chain);
	}

	return 0;
}