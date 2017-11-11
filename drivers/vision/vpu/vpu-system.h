/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef VPU_SYSTEM_H_
#define VPU_SYSTEM_H_

#include <linux/platform_device.h>

#include "vpu-interface.h"
#include "vpu-exynos.h"
#include "vpu-memory.h"
#include "vpu-binary.h"
#include "vpu-tv.h"

struct vpu_system {
	struct platform_device			*pdev;
	void __iomem				*ram0;
	void __iomem				*ram1;
	void __iomem				*code;
	void __iomem				*regs;
	resource_size_t 			ram0_size;
	resource_size_t 			ram1_size;
	resource_size_t 			code_size;
	resource_size_t 			regs_size;
	int					irq0;
	int					irq1;

	u32					cam_qos;
	u32					mif_qos;

	struct vpu_interface 			interface;
	struct vpu_exynos			exynos;
	struct vpu_memory			memory;
	struct vpu_binary			binary;
	struct vpu_tvset			tvset;
};

int vpu_system_probe(struct vpu_system *system, struct platform_device *pdev);
int vpu_system_open(struct vpu_system *system);
int vpu_system_close(struct vpu_system *system);
int vpu_system_resume(struct vpu_system *system, u32 mode);
int vpu_system_suspend(struct vpu_system *system);
int vpu_system_start(struct vpu_system *system);
int vpu_system_stop(struct vpu_system *system);

#endif