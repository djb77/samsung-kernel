/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "vpu-config.h"
#include "vpu-exynos.h"
#include "vpu-tv.h"

int vpu_tv_probe(struct vpu_tvset *tvset,
	struct device *dev,
	struct vpu_exynos *exynos,
	struct vpu_memory *memory)
{
	int ret = 0;

	tvset->dev = dev;
	tvset->exynos = exynos;
	tvset->memory = memory;
	tvset->tv_cnt = 0;

	ret = vpu_tv_register_depth3(tvset);
	if (ret) {
		vpu_err("vpu_tv_register_depth3 is fail(%d)\n", ret);
		goto p_err;
	}

	ret = vpu_tv_register_flamorb(tvset);
	if (ret) {
		vpu_err("vpu_tv_register_depth3 is fail(%d)\n", ret);
		goto p_err;
	}

p_err:
	return ret;
}
