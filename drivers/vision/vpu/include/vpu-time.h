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
#include <linux/time.h>
#include <linux/ktime.h>
#include <linux/timekeeping.h>

enum vpu_time_measure_point {
	VPU_TMP_QUEUE,
	VPU_TMP_REQUEST,
	VPU_TMP_RESOURCE,
	VPU_TMP_PROCESS,
	VPU_TMP_DONE,
	VPU_TMP_COUNT
};

struct vpu_time {
	struct timeval		time;
};

void vpu_get_timestamp(struct vpu_time *time);

#define VPU_TIME_IN_US(v)	((v).time.tv_sec * 1000000 + (v).time.tv_usec)