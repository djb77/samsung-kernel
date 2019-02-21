/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#ifndef VPU_RESOURCE_H_
#define VPU_RESOURCE_H_

#include <linux/device.h>

#include "lib/vpul-ds.h"
#include "vpu-hardware.h"

struct vpu_resource {
	struct vpu_hardware	hardware;
	spinlock_t		slock;
	unsigned long		state;
};

int vpu_resource_probe(struct vpu_resource *resource, struct device *dev);
int vpu_resource_open(struct vpu_resource *resource);
int vpu_resource_close(struct vpu_resource *resource);
int vpu_resource_add(struct vpu_resource *resource, struct vpul_task *task);
int vpu_resource_del(struct vpu_resource *resource, struct vpul_task *task);
int vpu_resource_get(struct vpu_resource *resource, struct vpul_task *task, unsigned long flags);
int vpu_resource_put(struct vpu_resource *resource, struct vpul_task *task);
int vpu_resource_print(struct vpu_resource *resource);

#endif
