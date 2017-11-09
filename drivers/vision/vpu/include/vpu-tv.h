/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef VPU_TV_H_
#define VPU_TV_H_

#include "vpu-binary.h"
#include "vpu-exynos.h"

#define VPU_TV_MAX		5
#define VPU_TV_IN_MAX		5
#define VPU_TV_OT_MAX		5

struct vpu_tv {
	u32			id;
	size_t			regs_size;
	void			*regs_buffer;
	struct vpu_binary	regs;

	u32			in_cnt;
	size_t			in_size[VPU_TV_IN_MAX];
	void			*in_cookie[VPU_TV_IN_MAX];
	void			*in_kvaddr[VPU_TV_IN_MAX];
	dma_addr_t		in_dvaddr[VPU_TV_IN_MAX];
	struct vpu_binary	in[VPU_TV_IN_MAX];

	u32			ot_cnt;
	size_t			ot_size[VPU_TV_IN_MAX];
	void			*ot_cookie[VPU_TV_IN_MAX];
	void			*ot_kvaddr[VPU_TV_IN_MAX];
	void			*ot_golden[VPU_TV_IN_MAX];
	dma_addr_t		ot_dvaddr[VPU_TV_IN_MAX];
	struct vpu_binary	gd[VPU_TV_OT_MAX];
	struct vpu_binary	ot[VPU_TV_OT_MAX];
};

struct vpu_tvset {
	struct device		*dev;
	u32			tv_cnt;
	struct vpu_tv		tv[VPU_TV_MAX];
	struct vpu_exynos	*exynos;
	struct vpu_memory	*memory;
};

int vpu_tv_probe(struct vpu_tvset *tvset,
	struct device *dev,
	struct vpu_exynos *exynos,
	struct vpu_memory *memory);

int vpu_tv_register_depth3(struct vpu_tvset *tvset);
int vpu_tv_do_depth3(struct vpu_tvset *tvset);
int vpu_tv_register_flamorb(struct vpu_tvset *tvset);
int vpu_tv_do_flamorb(struct vpu_tvset *tvset);

#endif
