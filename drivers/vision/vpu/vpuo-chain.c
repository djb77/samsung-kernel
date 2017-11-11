/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/slab.h>

#include "vpu-config.h"
#include "vpu-debug.h"
#include "vpuo-vertex.h"
#include "vpuo-chain.h"
#include "vpuo-pu.h"

int vpuo_chain_create(struct vpuo_chain **chain,
	char *desc_ubase,
	char *desc_mbase,
	struct vpul_subchain *desc_uchain,
	struct vpul_subchain *desc_mchain,
	void *parent)
{
	int ret = 0;
	u32 i, pu_cnt;
	struct vpul_task *desc_task;
	struct vpul_pu *desc_upu, *desc_mpu;
	struct vpuo_pu *pu;

	BUG_ON(!chain);
	BUG_ON(!desc_ubase);
	BUG_ON(!desc_mbase);
	BUG_ON(!desc_uchain);
	BUG_ON(!desc_mchain);
	BUG_ON(!parent);

	desc_task = (struct vpul_task *)desc_ubase;

	pu_cnt = desc_uchain->num_of_pus;
	if (pu_cnt >= VPUO_CHAIN_MAX_PU) {
		vpu_err("pu count is invalid(%d)\n", pu_cnt);
		return -EINVAL;
	}

	*chain = kzalloc(sizeof(struct vpuo_chain), GFP_KERNEL);
	if (*chain == NULL) {
		vpu_err("kzalloc is fail");
		ret = -ENOMEM;
		goto p_err;
	}

	if (desc_uchain->id > 0xFFFF) {
		vpu_err("chain id should be 16bit width(%X)\n", desc_uchain->id);
		ret = -EINVAL;
		goto p_err;
	}

	(*chain)->id = desc_uchain->id;
	(*chain)->index = ((char *)desc_uchain - (desc_ubase + desc_task->sc_vec_ofs)) / sizeof(struct vpul_subchain);
	(*chain)->type = desc_uchain->stype;
	(*chain)->level = VPU_VERTEX_MAX_LEVEL;
	(*chain)->parent = parent;
	(*chain)->desc_uchain = desc_uchain;
	(*chain)->desc_mchain = desc_mchain;

	(*chain)->inchain_cnt = 0;
	(*chain)->otchain_cnt = 0;
	for (i = 0; i < VPUO_CHAIN_MAX_PORT; ++i) {
		(*chain)->inchain[i] = NULL;
		(*chain)->otchain[i] = NULL;
	}

	(*chain)->pu_cnt = 0;
	for (i = 0; i < VPUO_CHAIN_MAX_PU; ++i) {
		(*chain)->pu_array[i] = NULL;
		(*chain)->pu_table[i] = NULL;
	}

	switch (desc_uchain->stype) {
	case VPUL_SUB_CH_HW:
		desc_upu = (struct vpul_pu *)(desc_ubase + desc_uchain->pus_ofs);
		desc_mpu = (struct vpul_pu *)(desc_mbase + desc_mchain->pus_ofs);
		for (i = 0; i < pu_cnt; ++i) {
			ret = vpuo_pu_create(&pu, desc_ubase, desc_mbase, &desc_upu[i], &desc_mpu[i], *chain);
			if (ret) {
				vpu_err("vpu_pu_create is fail(%d)\n", ret);
				goto p_err;
			}

			if (!pu) {
				vpu_err("pu is NULL\n");
				ret = -EINVAL;
				goto p_err;
			}

			(*chain)->pu_array[i] = pu;
			(*chain)->pu_table[pu->id] = pu;
			(*chain)->pu_cnt++;

			BUG_ON((*chain)->pu_cnt >= VPUO_CHAIN_MAX_PU);
		}
		break;
	case VPUL_SUB_CH_CPU_OP:
		break;
	default:
		break;
	}

	return 0;

p_err:
	vpuo_chain_destroy(*chain);
	return ret;
}

int vpuo_chain_destroy(struct vpuo_chain *chain)
{
	int ret = 0;
	u32 i, pu_cnt;
	struct vpuo_pu *pu;

	pu_cnt = chain->pu_cnt;
	for (i = 0; i < pu_cnt; ++i) {
		pu = chain->pu_array[i];
		if (!pu) {
			vpu_err("pu of %d index is NULL\n", i);
			BUG();
		}

		if (pu->id >= VPUO_CHAIN_MAX_PU) {
			vpu_err("pu id %d is invalid\n", pu->id);
			BUG();
		}

		chain->pu_table[pu->id] = NULL;

		ret = vpuo_pu_destroy(pu);
		if (ret) {
			vpu_err("vpu_pu_destroy is fail(%d)\n", ret);
			continue;
		}

		chain->pu_array[i] = NULL;
		chain->pu_cnt--;
	}

	kfree(chain);

	return ret;
}

int vpuo_chain_parse(struct vpuo_chain *chain)
{
	int ret = 0;
	u32 i, j, lvl_pu_cnt, pu_cnt, ot_cnt;
	struct vpul_pu *desc_pu;
	struct vpuo_pu *pu, *src, *dst, *target, *temp;
	struct list_head *lvl_list;

	BUG_ON(!chain);

	lvl_pu_cnt = 0;
	pu_cnt = chain->pu_cnt;

	chain->lvl_cnt = 0;
	lvl_list = chain->lvl_list;
	for (i = 0; i < VPUO_CHAIN_MAX_LEVEL; ++i)
		INIT_LIST_HEAD(&lvl_list[i]);

	chain->inleaf_cnt = 0;
	INIT_LIST_HEAD(&chain->inleaf_list);

	chain->otleaf_cnt = 0;
	INIT_LIST_HEAD(&chain->otleaf_list);

	for (i = 0; i < pu_cnt; ++i) {
		dst = chain->pu_array[i];

		desc_pu = dst->desc_upu;
		if (desc_pu->n_in_connect > VPUO_PU_MAX_PORT) {
			vpu_err("n_in_connect is invalid(%d)\n", desc_pu->n_in_connect);
			ret = -EINVAL;
			goto p_err;
		}

		for (j = 0; j < desc_pu->n_in_connect; ++j) {
			if (desc_pu->in_connect[j].pu_idx == NO_PU_CONNECTED)
				continue;

			if (desc_pu->in_connect[j].pu_idx >= pu_cnt) {
				vpu_err("[C%dP%d] the index of src is invalid(%d > %d)\n",
					chain->id, desc_pu->instance, desc_pu->in_connect[j].pu_idx, pu_cnt);
				ret = -EINVAL;
				goto p_err;
			}

			src = chain->pu_array[desc_pu->in_connect[j].pu_idx];
			if (!src) {
				vpu_err("src is NULL\n");
				ret = -EINVAL;
				goto p_err;
			}

			if (dst->inpu[j]) {
				vpu_err("%d pu %d input is already connected\n", dst->id, j);
				ret = -EINVAL;
				goto p_err;
			}

			if (desc_pu->in_connect[j].s_pu_out_idx >= VPUO_PU_MAX_PORT) {
				vpu_err("s_pu_out_idx is invalid(%d)\n", desc_pu->in_connect[j].s_pu_out_idx);
				ret = -EINVAL;
				goto p_err;
			}

			if (src->otpu[desc_pu->in_connect[j].s_pu_out_idx]) {
				vpu_err("%d pu %d input is already connected\n", src->id,
					desc_pu->in_connect[j].s_pu_out_idx);
				ret = -EINVAL;
				goto p_err;
			}

			dst->inpu[j] = src;
			dst->inpu_cnt++;
			src->otpu[desc_pu->in_connect[j].s_pu_out_idx] = dst;
			src->otpu_cnt++;

			BUG_ON(dst->inpu_cnt >= VPUO_PU_MAX_PORT);
			BUG_ON(src->otpu_cnt >= VPUO_PU_MAX_PORT);
		}
	}

	for (i = 0; i < pu_cnt; ++i) {
		pu = chain->pu_array[i];
		if (!pu) {
			vpu_err("pu is NULL\n");
			BUG();
		}

		if (!pu->inpu_cnt) {
			list_add_tail(&pu->level_entry, &lvl_list[0]);
			pu->level = 0;
			lvl_pu_cnt++;
		}

		if (test_bit(VPUO_PU_STATE_IN, &pu->state)) {
			chain->inleaf_cnt++;
			list_add_tail(&pu->cleaf_entry, &chain->inleaf_list);
		} else if (test_bit(VPUO_PU_STATE_OT, &pu->state)) {
			chain->otleaf_cnt++;
			list_add_tail(&pu->cleaf_entry, &chain->otleaf_list);
		}
	}

	for (i = 0; i < (VPUO_CHAIN_MAX_LEVEL - 1); ++i) {
		if (list_empty(&lvl_list[i]))
			break;

		list_for_each_entry_safe(pu, temp, &lvl_list[i], level_entry) {
			for (j = 0; j < pu->inpu_cnt; ++j) {
				target = pu->inpu[j];
				if (!target)
					continue;

				if (target->level >= i) {
					list_del(&pu->level_entry);
					lvl_pu_cnt--;

					list_add_tail(&pu->level_entry, &lvl_list[i + 1]);
					pu->level = i + 1;
					lvl_pu_cnt++;
					break;
				}
			}

			if (pu->level != i)
				continue;

			ot_cnt = 0;
			for (j = 0; j < VPUO_PU_MAX_PORT; ++j) {
				target = pu->otpu[j];
				if (!target)
					continue;

				ot_cnt++;

				/* if already is added */
				if (target->level < VPUO_CHAIN_MAX_LEVEL)
					continue;

				list_add_tail(&target->level_entry, &lvl_list[i + 1]);
				target->level = i + 1;
				lvl_pu_cnt++;
			}

			if (ot_cnt  != pu->otpu_cnt) {
				vpu_err("otpu_cnt is invalid(%d, %d)\n", ot_cnt, pu->otpu_cnt);
				ret = -EINVAL;
				goto p_err;
			}
		}
	}

	if (lvl_pu_cnt != pu_cnt) {
		vpu_err("connection is invalid(%d, %d)\n", lvl_pu_cnt, chain->pu_cnt);
		ret = -EINVAL;
		goto p_err;
	}

	chain->lvl_cnt = i;

p_err:
	return ret;
}

int vpuo_chain_print(struct vpuo_chain *chain)
{
	DLOG_INIT();
	u32 i, j, otpu_cnt;
	struct list_head *lvl_list;
	struct vpuo_pu *pu, *temp;

	lvl_list = chain->lvl_list;

	vpu_info("[CHAIN : %d|%d]\n", chain->id, chain->index);

	switch (chain->type) {
	case VPUL_SUB_CH_HW:
		for (i = 0; i < VPUO_CHAIN_MAX_LEVEL; ++i) {
			if (list_empty(&lvl_list[i]))
				continue;

			DLOG("lvl%d : ", i);
			list_for_each_entry_safe(pu, temp, &lvl_list[i], level_entry) {
				DLOG("%d|%d(", pu->id, pu->index);

				otpu_cnt = pu->otpu_cnt;
				for (j = 0; j < VPUO_PU_MAX_PORT; ++j) {
					if (!pu->otpu[j])
						continue;

					if (otpu_cnt == 0) {
						vpu_err("otpu_cnt is invalid(%d)\n", pu->otpu_cnt);
						BUG();
					}

					if (otpu_cnt == 1)
						DLOG("%d", pu->otpu[j]->id);
					else
						DLOG("%d ", pu->otpu[j]->id);

					otpu_cnt--;
				}

				if (pu->type == VPUL_OP_DMA) {
					DLOG(")");
					for (j = 0; j < pu->buffer_cnt; ++j)
						DLOG("-%d",pu->buffer_idx[j]);
					DLOG(" ");
				} else {
					DLOG(") ");
				}

				if (otpu_cnt != 0) {
					vpu_err("otpu_cnt is invalid(%d)\n", pu->otpu_cnt);
					BUG();
				}
			}

			vpu_info("%s\n", DLOG_OUT());
		}
		break;
	case VPUL_SUB_CH_CPU_OP:
		vpu_info("lvl0 : CPU\n");
		break;
	default:
		break;
	}

	return 0;
}