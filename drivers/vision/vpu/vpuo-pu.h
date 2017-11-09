/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef VPU_OBJ_PU_H_
#define VPU_OBJ_PU_H_

#include "vs4l.h"

#define VPUO_PU_MAX_PORT		(VPUL_PU_MAX_PORTS * 2)
#define VPUO_PU_MAX_BUFFER		VPUL_MAX_3DNN_LAYERS_IN_PROC_BASE

enum vpuo_type {
	VPU_TYPE_SIGNED,
	VPU_TYPE_UNSIGNED
};

struct vpuo_channel {
	__u16			enable:1;
	__u16			type:1;
	__u16			effective_bit:6;
	__u16			width;
	__u16			height;
};

struct vpuo_border {
	/*
	 * [X] BORDER MODE
	 * <0> constant fill
	 * <1> not supported
	 * <2> replicating fill
	 */
	__u16 mode;
	struct {
		__u16		up:4;
		__u16		down:4;
		__u16		left:4;
		__u16		right:4;
	} direction;
	__u32 constant;
};

enum vpuo_sram_mode {
	VPU_SRAM_MODE_AUTO,
	VPU_SRAM_MODE_MANUAL
};

enum vpuo_sram_type {
	VPU_SRAM_TYPE_SMALL,
	VPU_SRAM_TYPE_LARGE
};

struct vpuo_sram {
	__u8			mode;
	__u8			type;
	__u8			port;
};

enum vpuo_tile_mode {
	VPU_TILE_MODE_AUTO,
	VPU_TILE_MODE_MANUAL
};

struct vpuo_tile {
	__u8			mode;
	__u16			width;
	__u16			height;
};

enum vpuo_pu_state {
	VPUO_PU_STATE_FORMAT,
	VPUO_PU_STATE_MAPPED,
	VPUO_PU_STATE_IN,
	VPUO_PU_STATE_OT,
	VPUO_PU_STATE_IM
};

struct vpuo_pu {
	u32				id;
	u32				index;
	u32				type;
	u32				level;
	u32				target;
	void				*sfr;
	void				*parent;
	struct list_head		level_entry; /* chain level entry */
	struct list_head		cleaf_entry; /* chain leaf entry */
	struct list_head		vleaf_entry; /* vertex leaf entry */
	struct list_head		gleaf_entry; /* graph leaf entry */
	unsigned long			state;

	struct vpul_pu			*desc_upu;
	struct vpul_pu			*desc_mpu;

	u32				inpu_cnt;
	u32				otpu_cnt;
	struct vpuo_pu			*inpu[VPUO_PU_MAX_PORT];
	struct vpuo_pu			*otpu[VPUO_PU_MAX_PORT];

	/* DMA parameter */
	struct vb_container		*container[VPU_MAX_BUFFER];
	u32				buffer_cnt;
	u32				buffer_shm[VPUO_PU_MAX_BUFFER];
	u32				buffer_idx[VPUO_PU_MAX_BUFFER];
	u32				*buffer_ptr[VPUO_PU_MAX_BUFFER];
	u32				buffer[VPUO_PU_MAX_BUFFER];
};

int vpuo_pu_create(struct vpuo_pu **pu,
	char *desc_ubase,
	char *desc_mbase,
	struct vpul_pu *desc_upu,
	struct vpul_pu *desc_mpu,
	void *parent);
int vpuo_pu_destroy(struct vpuo_pu *pu);

#endif
