/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#if !defined(__VPU_CPU_INTERNAL__)
#define __VPU_CPU_INTERNAL__

#include "lib/vpu-fwif-commands.h"
#include "lib/vpul-cpu_op.h"

/* describes location of HW results for PU and identifies the PU that generates it*/
struct pu_output_location_entry
{
	/* describes pu offset; used as an identifier only*/
	struct vpul_pu* pu_idenifier;

	/* required for getting the correct work area index (there is work area per process)*/
	__u16 process_id;

	/* offset in WA of desired result */
	__u16 offset_in_work_area;
};


__u32 get_cpuop_param_number(struct vpul_cpu_op * cpu_oper);


struct runtime_updated_pu_entry* fnd_fst_val_apprnce_in_runtime_upd_tbl(
	struct runtime_updated_pu_entry* table_head,
	struct vpul_pu* desired_vlaue);



#endif /*  __VPU_PROCESSING_UNIT_INTERNAL__ */
