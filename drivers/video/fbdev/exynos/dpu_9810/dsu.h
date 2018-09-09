/*
 * linux/drivers/video/fbdev/exynos/dpu_9810/dsu.h
 *
 * Source file for DSU Driver
 *
 * Copyright (c) 2016 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __DSU_H__
#define __DSU_H__

struct dsu_info {
	int left;
	int top;
	int right;
	int bottom;
	int mode;
	int needupdate;
};

enum {
	DSU_MODE_NONE = 0,
	DSU_MODE_1,
	DSU_MODE_2,
	DSU_MODE_3,
	DSU_MODE_MAX,
};

struct dsu_size {
	int width;
	int height;
};

#endif //__DSU_H__
