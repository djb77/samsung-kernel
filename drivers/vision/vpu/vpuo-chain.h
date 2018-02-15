/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef VPU_OBJ_CHAIN_H_
#define VPU_OBJ_CHAIN_H_

#include "lib/vpul-ds.h"
#include "vpuo-pu.h"

#define VPUO_CHAIN_MAX_PU		VPU_PU_NUMBER
#define VPUO_CHAIN_MAX_PORT		VPUO_PU_MAX_PORT
#define VPUO_CHAIN_MAX_LEVEL		20

enum vpuo_chain_state {
	VPUO_CHAIN_STATE_MAPPED
};

struct vpuo_chain {
	u32				id;
	u32				index;
	u32				type;
	u32				level;
	void				*parent;
	struct list_head		level_entry;
	unsigned long			state;

	struct vpul_subchain		*desc_uchain;
	struct vpul_subchain		*desc_mchain;

	u32				inchain_cnt;
	u32				otchain_cnt;
	struct vpuo_chain		*inchain[VPUO_CHAIN_MAX_PORT];
	struct vpuo_chain		*otchain[VPUO_CHAIN_MAX_PORT];

	u32				pu_cnt;
	struct vpuo_pu			*pu_array[VPUO_CHAIN_MAX_PU];
	struct vpuo_pu			*pu_table[VPUO_CHAIN_MAX_PU];

	struct list_head		lvl_list[VPUO_CHAIN_MAX_LEVEL];
	u32				lvl_cnt;

	u32				inleaf_cnt;
	struct list_head		inleaf_list;

	u32				otleaf_cnt;
	struct list_head		otleaf_list;
};

int vpuo_chain_create(struct vpuo_chain **chain,
	char *desc_ubase,
	char *desc_mbase,
	struct vpul_subchain *desc_uchain,
	struct vpul_subchain *desc_mchain,
	void *parent);
int vpuo_chain_destroy(struct vpuo_chain *chain);
int vpuo_chain_parse(struct vpuo_chain *chain);
int vpuo_chain_print(struct vpuo_chain *chain);
#endif