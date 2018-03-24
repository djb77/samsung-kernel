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

#ifndef SCORE_HARDWARE_H_
#define SCORE_HARDWARE_H_

#include <linux/device.h>

#include "vs4l.h"

#define SCORE_HW_MAX_PU_COUNT 	50
#define SCORE_HW_MAX_RB_COUNT 	3
#define SCORE_HW_MAX_CH_COUNT 	30
#define SCORE_HW_MAX_NM_COUNT 	15

/* enum used for indexing MPRBs in score_hw_mprb according to size-type */
enum mprb_index_values {
	MPRB_none,
	MPRB_large,
	MPRB_small
};

struct score_hw_channel {
	u8			flags;
	u8			graph;
};

struct score_hw_block {
	char			name[SCORE_HW_MAX_NM_COUNT];
	struct attribute_group	attr_group;
	u32			total;
	u32			refer;
	u32			start;
	DECLARE_BITMAP(allocated, SCORE_HW_MAX_CH_COUNT);
	DECLARE_BITMAP(pre_allocated, SCORE_HW_MAX_CH_COUNT);
	DECLARE_BITMAP(preempted, SCORE_HW_MAX_CH_COUNT);
	struct score_hw_channel	table[SCORE_HW_MAX_CH_COUNT];
};

struct score_hw_pu {
	u32			total;
	/*
	 * pointer to translator
	 * translator itself is defined in score-lib (hw mapper)
	 * the translation is based on PU enumeration as defined in score-lib
	 * (in file scorel_pu_instances.def)
	 * translator is used by hw mapper (functions __score_resource_pu_get
	 * and __score_resource_pu_put) - IS THERE ANY OTHER USE FOR IT ?
	 */
	const u32		*translator;
	struct score_hw_block	table[SCORE_HW_MAX_PU_COUNT];
};

struct score_hw_mprb {
	u32			total;
	struct score_hw_block	table[SCORE_HW_MAX_RB_COUNT];
};

struct score_hardware {
	struct score_hw_pu	pu;
	struct score_hw_mprb	mprb;
};

int score_hardware_init(struct score_hardware *hardware);
int score_hardware_host_init(struct score_hardware *hardware);

#endif
