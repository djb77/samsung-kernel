/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef VPU_OBJ_VERTEX_H_
#define VPU_OBJ_VERTEX_H_

#include "vs4l.h"
#include "vpuo-chain.h"
#include "vpuo-pu.h"

#define VPU_VERTEX_MAX_CHAIN		10
#define VPU_VERTEX_MAX_LEVEL		VPU_VERTEX_MAX_CHAIN
#define VPU_VERTEX_MAX_PORT		10

struct vpuo_vertex {
	u32				id;
	u32				type;
	u32				level;
	u32				layers;
	void				*parent;
	struct list_head		level_entry;
	unsigned long			state;

	struct vpul_vertex		*desc_uvertex;
	struct vpul_vertex		*desc_mvertex;

	u32				invertex_cnt;
	u32				otvertex_cnt;
	struct vpuo_vertex		*invertex[VPU_VERTEX_MAX_PORT];
	struct vpuo_vertex		*otvertex[VPU_VERTEX_MAX_PORT];

	u32				chain_cnt;
	u32				chain_table[VPU_VERTEX_MAX_CHAIN];
	struct vpuo_chain		*chain_array[VPU_VERTEX_MAX_CHAIN];

	u32				inleaf_cnt;
	struct list_head		inleaf_list;

	u32				otleaf_cnt;
	struct list_head		otleaf_list;
};

int vpuo_vertex_create(struct vpuo_vertex **vertex,
	char *desc_ubase,
	char *desc_mbase,
	struct vpul_vertex *desc_uvertex,
	struct vpul_vertex *desc_mvertex,
	void *parent);
int vpuo_vertex_destroy(struct vpuo_vertex *vertex);
int vpuo_vertex_parse(struct vpuo_vertex *vertex);
int vpuo_vertex_print(struct vpuo_vertex *vertex);
#endif