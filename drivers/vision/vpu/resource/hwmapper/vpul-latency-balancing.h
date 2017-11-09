/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#ifndef VPUL_LATENCY_BALANCING_H_
#define VPUL_LATENCY_BALANCING_H_

#include "vpu-hardware.h"
#include "lib/vpul-ds.h"

/* on a "per subchain" basis */
#define NUMBER_OF_SPARE_PUS_FOR_LATENCY_BALANCING 5

__u32 is_pu_dma_in(const struct vpul_pu *pu);
__u32 is_pu_dma_out(const struct vpul_pu *pu);

__s32 latency_balancing(struct vpul_task *task,
			struct vpul_vertex *vertex,
			struct vpul_subchain *subchain,
			__u32 *calculated_sizes);

__u32 get_filter_size_index_for_linear_filter(__u32 filter_size);

__u32 get_filter_size_idx_for_non_linear_filter(__u32 nlf_mode);

#endif
