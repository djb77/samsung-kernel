/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#ifndef VPUL_HW_V2_1_H_
#define VPUL_HW_V2_1_H_

#define VPU_HW_NUM_LARGE_MPRBS	24
#define VPU_HW_NUM_SMALL_MPRBS	23
#define VPU_HW_TOT_NUM_MPRBS	(VPU_HW_NUM_LARGE_MPRBS + \
				VPU_HW_NUM_SMALL_MPRBS)
#define TOTAL_NUM_RAM_PORTS	250

#define OFFSET_TO_LARGE_MEM_BLOCKS	0
#define OFFSET_TO_SMALL_MEM_BLOCKS	24


#endif
