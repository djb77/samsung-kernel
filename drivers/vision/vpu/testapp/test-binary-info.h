/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef VPU_TEST_BINARY_INFO_H_
#define VPU_TEST_BINARY_INFO_H_

struct vpu_binary_io_info {
	unsigned int		chain_idx;
	unsigned int		pu_idx;
	unsigned int		target;
	unsigned int		blist_cnt;
	unsigned int		size[6];
	unsigned int		addr[6];
};

#endif