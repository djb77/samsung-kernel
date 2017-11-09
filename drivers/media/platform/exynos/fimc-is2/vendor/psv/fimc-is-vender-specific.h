/*
 * Samsung Exynos SoC series FIMC-IS driver
 *
 * Exynos fimc-is PSV vender specification
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_VENDER_SPECIFIC_H
#define FIMC_IS_VENDER_SPECIFIC_H

#include "fimc-is-vector.h"

struct fimc_is_vender_specific {
	void *alloc_ctx;

#ifdef CONFIG_PSV_VECTOR_VERIFICATION
	struct vector_cfg vector_cfg;
#endif
};

#endif
