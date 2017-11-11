/*
 * linux/drivers/video/fbdev/exynos/panel/panel_dimming.h
 *
 * Header file for Samsung AID Dimming Driver.
 *
 * Copyright (c) 2016 Samsung Electronics
 * Gwanghui Lee <gwanghui.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __PANEL_DIMMING_H__
#define __PANEL_DIMMING_H__
#include "dimming.h"

struct panel_dimming_info {
	char *name;
	struct dimming_init_info dim_init_info;
	struct dimming_info *dim_info;
	s32 extend_hbm_target_luminance;
	s32 nr_extend_hbm_luminance;
	s32 hbm_target_luminance;
	s32 nr_hbm_luminance;
	s32 target_luminance;
	s32 nr_luminance;
	struct brightness_table *brt_tbl;
};

extern int register_panel_dimming(struct panel_dimming_info *info);
#endif /* __PANEL_DIMMING_H__ */
