/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#if !defined(__VPU_PROCESSING_UNIT_INTERNAL__)
#define __VPU_PROCESSING_UNIT_INTERNAL__

#include "lib/vpu-fwif-commands.h"
#include  "lib/vpul-pu.h"
#include  "lib/vpul-cpu_op.h"

#define MAXNUM_OF_PORTS_FOR_PRELOADING_PU (3) /* only LUT, HIST and FLAMORB requires preloading. max # of ports is 3 in those PU's. */


/* describes pu that creates an output and pu that uses this output as a prm.*/
/* used for writing the dest pu prms location on the linking CPU operation */
struct runtime_updated_pu_entry
{
	/* this is the pu that it's prms are taken from previous pu.                           */
	/* when passing the TDS, we still do not know what is the location of dest pu's params.*/
	/* info kept on this structure and updated when the TDS parser arrives the dest pu     */
	/* NULL ptr means empty entry                                                          */
	struct vpul_pu* updated_pu_identifier;

	/* this is the location where (offst of (PU prms to be updated)) should be written */
	/* it is filled by the copy cpu operation.                                         */
	__u16*   location_to_write_updated_prms_offset;

	__u32    offset_in_dst_pu_prms;
};






__u32 is_preload_needed(enum vpul_pu_instance instance);

__u32 pu_get_num_params(const struct vpul_pu *pu);

__u32 pu_get_instance_idx(__u32 instance);

__u32 pu_get_num_mprb_groups(
	const struct vpul_pu *pu);

__u32 pu_get_num_mprb_groups_and_fill_groups(
	const struct vpul_pu *pu,
	struct VPUI_MprbGr *hw_mprbs);

struct vpul_pu_location*
	get_next_updatable_pu_location(
		__u32 arg_num_of_updatable_pus_remained,
		struct vpul_pu_location *updatable_pu_list);

__u16* wr_invoke_indices_to_MB(
	__u16 *updatable_indices_on_mb,
	__u32 first_index_val,
	__u32 Num_of_vals_to_write);

/**
  * find_pu_idx_by_pu_location_struct_ptr() - gets pu index in the pu list
  * according to index of pu_location struct.
  * \param task Pointer to Host Task data structure.
  * \param __u32 pu_loc_struct_idx: index of pu-location struct in TDS
  * \return index of pu in pu list.
  */
struct vpul_pu* find_pu_by_pu_location_struct_ptr(
struct vpul_pu_location *current_pu_loc_ptr,
	const struct vpul_task	*inp_task_ptr);


__u32 pu_num_of_hw_res_to_read(const struct vpul_pu *pu);


#endif /*  __VPU_PROCESSING_UNIT_INTERNAL__ */
