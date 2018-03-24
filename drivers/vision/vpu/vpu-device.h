/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef VPU_DEVICE_H_
#define VPU_DEVICE_H_

#include "vpu-graphmgr.h"
#include "vpu-system.h"
#include "vpu-vertex.h"
#include "vpu-exynos.h"
#include "vpu-resource.h"
#include "vpu-debug.h"

enum vpu_device_state {
	VPU_DEVICE_STATE_OPEN,
	VPU_DEVICE_STATE_START
};

enum vpu_device_mode {
	VPU_DEVICE_MODE_NORMAL,
	VPU_DEVICE_MODE_TEST
};

struct vpu_device {
	struct device			*dev;
	unsigned long			state;
	u32				mode;

	struct vpu_graphmgr		graphmgr;
	struct vpu_system		system;
	struct vpu_resource		resource;
	struct vpu_vertex		vertex;
	struct vpu_debug		debug;
};

int vpu_device_open(struct vpu_device *device);
int vpu_device_close(struct vpu_device *device);
int vpu_device_start(struct vpu_device *device);
int vpu_device_stop(struct vpu_device *device);

#endif
