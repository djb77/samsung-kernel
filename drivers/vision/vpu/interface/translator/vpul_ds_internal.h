/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#if !defined(__VPU_TASK_DS_INTERNAL__)
#define __VPU_TASK_DS_INTERNAL__



#include  "vpul_pu_internal.h"
#include  "vpul_cpu_internal.h"

 /*! \brief get pointer to vertex by it's index */
#define vtx_ptr(_task, _idx) \
	(struct vpul_vertex *)(fst_vtx_ptr(_task) + (_idx))



/* mb proc current info */
struct mb_ptrs {
	__u16 *first_proc_prm;
	__u16 *cpu_prm;
	__u16 *block_prm;
	/* offset to invoke parameters indices (during create).*/
	__u16 *param_idxs; /*pointer to the indices vector inside data vector*/
	/* offset to invoke parameters values  (during invoke).*/
	__u16 *invoke_prm;
	struct VPUI_ProcDesc *proc_desc;
	struct VPUI_HistParams *histo_param;
	struct VPUI_Proc3DNNDesc *proc_3dnn_desc;
	struct VPUI_Proc3DNNBase *proc_3dnn_base_desc;
	struct VPUI_SubCh *subch_desc;
	struct VPUI_HwSubCh *hw_subch_desc;
	struct VPUI_Hw3DNNSubCh *hw_3dnn_subch_desc;
	struct VPUI_Hw3DNNSlGrTypeSubCh *hw_3dnn_slgr_type_subch_desc;
	struct VPUI_CpuSubCh *cpu_subch_desc;
	struct VPUI_CpuOp *cpuop_desc;
	struct VPUI_BlUsage *block_desc;
	struct VPUI_HWConnect *connections_desc;
	struct VPUI_InOutType *inout_types;
	struct VPUI_Roi *roi;
	struct VPUI_FixMapRoi *fix_map_roi;
	struct VPUI_SizesOp *size_op;
	struct VPUI_MemVecDesc *post_mem_vec;
	struct VPUI_MemVecDesc *pre_mem_vec;
	__u8   *pt_map_indeces;
	struct VPUI_PtList *pt_list;
	struct  VPUI_RoiFromPt *roi_from_pt;
	struct VPUI_Sizes   *pt_roi_sizes;
	struct VPUI_DisparityFixedThr *pt_disp_fixed_param;
	__u32 cnn_crop_idx;
	__u32 cnn_relu_idx;
	__u32 cnn_scaler_idx;
	__u32 inout_types_idx;
};

/* mb sub-chain current info */
struct mbsc_cnts {
	__u32 cpu_prm_ofs;
	__u32 cpu_idx;
	__u32 hwsc_idx;
	__u32 fst_block;
	__u32 fst_hw_read_res;
	__u32 fst_bl_prm;
	__u32 fst_cpuoper_param;
	__u32 fst_connect;
	__u32 fst_hwmprb;
	__u32 fst_intramidx;
	__u32 n_bl_params;
	__u32 n_hw_read_params;
};

struct mprb_entry
{
	__u32 proc_idx:8;
	__u32 sc_idx:8;
	__u32 pu_idx:8;

	__u32 mprb_group_num:8;
	__u8 preloaded_mprb[MAXNUM_OF_PORTS_FOR_PRELOADING_PU];
};


struct tds_iterator_state
{
	__u32 current_vtx_id;
	__u32 current_sc_id_in_vtx;
	__u32 current_pu_id_in_sc;

	__u32 num_of_updatable_pus_remained;

	/*
	*  holds the index of next ROI to set on rois vector.
	 */
	__u32 next_roi_index;

};


#define MAXIMAL_PRELOADED_PU					(20)
#define MAXIMAL_OUTPUT_PUS						(20)
#define MAXIMAL_NUM_OF_RUNTIME_PRM_UPDATES		(20)
#define BYTES_IN_WORKING_AREA_DESCRIPTION		(2)
struct internal_context{

	struct tds_iterator_state tds_itrtr_stat;

	struct mprb_entry	    mprb_pu_table[MAXIMAL_PRELOADED_PU];

	/* lut for locating pu's  output on FW work area, if needed*/
	struct pu_output_location_entry	 pu_output_location_entry_table[MAXIMAL_OUTPUT_PUS];

	/* table for manging inter pu parameter passing on runtime */
	struct runtime_updated_pu_entry  runtime_updated_pu_table[MAXIMAL_NUM_OF_RUNTIME_PRM_UPDATES]; /* initialized on internal context memset*/

	/* next empty entry on pu_output_location_entry_table */
	/* NOTE: this is the offset on the pu-work-area lut, not on the work area! */
	__u32	pu_output_location_next;/* init value 0 by memset of intrnl_ctx struct*/

	/* number of entries in  mprb_pu_table */
	__u32	n_entries;

	/* iterator for finding pu  mprb_pu_table */
	__u32	mprb_iter;

	__u32	n_internal_mem;



	/*
	*  holds the offset on the FW work area where the next PU with HW results
	 * will write the results. since the working area is per-process,
	 * the value of this field is zeroed on each process.
	 * NOTE: this is the offset on the work area, not on the pu-work-area lut! .
	 */

	__u32			next_wr_ofst_on_work_area;

	/*
	*  holds the required memory allocation from FW, mainly for reporting results.
	 */

	__u32			required_output_mem_allc_16B;

};



struct mprb_entry * get_next_mprb_match( struct internal_context * ctx,
	__u32 proc_idx,
	__u32 sc_idx,
	__u32 pu_idx);

__u32 set_subchain_blocks_and_params
(
	struct internal_context *internal_context,
	struct vpul_pu *first_pu_in_sc,
	__u32 n_pus_in_sc,
	const struct vpul_task* task_ptr,
	struct mb_ptrs *mbp,
	__u32 *hist_param_index,
	struct mbsc_cnts *mbsc,
	__u16 **mb_InvokePrms_ptr, /* ptr is inout */
	__u16 *accumulated_n_of_invoke_params_ptr,
	struct vpul_pu_location **updatable_pu_list_ptr, /* ptr is inout */
	__u16 current_offset_from_param_start,
	__u32 *sc_connect_ofs_idx,
	__u32 *fixed_disp_pram_idx,
	__u32 *initial_mprb_grp_offset_in_subchain);

__u32 set_cpu_subchain_cpu_ops_params
(
	struct internal_context* intrnl_ctx, /* required for PU-workarea LUT */
	const struct vpul_task *task,	     /* required for getting PU from pu location struct */
	struct vpul_cpu_op *first_cpuoper_in_sc,
	__u32 n_cpuoper_in_sc,
	__u16 *mb_cpuoper_params,
	struct VPUI_CpuOp **mb_cpu_oper_desc,
	__u32 *sc_cpu_param_ofs_idx);



__u32 check_pu_updability(
	struct vpul_pu_location* next_updatable_pu_location_ptr,
	struct vpul_pu *curent_pu_ptr,
	const struct vpul_task *TaskPtr);



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





#endif /*  __VPU_TASK_DS_INTERNAL__ */
