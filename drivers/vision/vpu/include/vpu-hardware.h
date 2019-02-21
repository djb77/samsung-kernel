/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#ifndef VPU_HARDWARE_H_
#define VPU_HARDWARE_H_

#include <linux/device.h>

#include "vs4l.h"

#define VPU_HW_MAX_PU_COUNT 	50
#define VPU_HW_MAX_RB_COUNT 	3
#define VPU_HW_MAX_CH_COUNT 	30
#define VPU_HW_MAX_NM_COUNT 	15

/* enum used for indexing MPRBs in vpu_hw_mprb according to size-type */
enum mprb_index_values {
	MPRB_none,
	MPRB_large,
	MPRB_small
};

struct vpu_hw_channel {
	u8			flags;
	u8			graph;
};

struct vpu_hw_block {
	char			name[VPU_HW_MAX_NM_COUNT];
	struct attribute_group	attr_group;
	u32			total;
	u32			refer;
	u32			start;
	DECLARE_BITMAP(allocated, VPU_HW_MAX_CH_COUNT);
	DECLARE_BITMAP(pre_allocated, VPU_HW_MAX_CH_COUNT);
	DECLARE_BITMAP(preempted, VPU_HW_MAX_CH_COUNT);
	struct vpu_hw_channel	table[VPU_HW_MAX_CH_COUNT];
};

struct vpu_hw_pu {
	u32			total;
	/**
	 * pointer to translator
	 * translator itself is defined in vpu-lib (hw mapper)
	 * the translation is based on PU enumeration as defined in vpu-lib
	 * (in file vpul_pu_instances.def)
	 * translator is used by hw mapper (functions __vpu_resource_pu_get
	 * and __vpu_resource_pu_put) - IS THERE ANY OTHER USE FOR IT ?
	 */
	const u32		*translator;
	struct vpu_hw_block	table[VPU_HW_MAX_PU_COUNT];
};

struct vpu_hw_mprb {
	u32			total;
	struct vpu_hw_block	table[VPU_HW_MAX_RB_COUNT];
};

struct vpu_hardware {
	struct vpu_hw_pu	pu;
	struct vpu_hw_mprb	mprb;
};

int vpu_hardware_init(struct vpu_hardware *hardware);
int vpu_hardware_host_init(struct vpu_hardware *hardware);

#endif