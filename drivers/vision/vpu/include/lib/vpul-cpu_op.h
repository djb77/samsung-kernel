/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#if !defined(__VPU_CPU_OPER__)
#define __VPU_CPU_OPER__

#include "vpul-ds.h"

#define CPU_OPER_NOT_SUPPORTED	(0)
#define CPU_OPER_SUPPORTED		(1)

#define CPU_COPY_ROIS_MIN (0)
#define CPU_COPY_ROIS_MAX (1)

#define CPU_COPY_ROIS_MIN_OFFST (VPUH_ROI_OUT_MINMAX_MIN_VALUE)
#define CPU_COPY_ROIS_MAX_OFFST (VPUH_ROI_OUT_MINMAX_MAX_VALUE)

#define VPUC_M2LI_THR_HIGH_LSB (6)
#define VPUC_ROI_OUT_MINMAX_MAX_VALUE (4)

/** \enum VPUL_CpuOpCode
 *  \brief Describes all the CPU operations that can be invoked by Processes
 *         and 3DNN processes. Defined using X macros based on cpul_cpuop_opcodes.def.
 */
enum vpul_cpu_op_code {
#define CPU_OPER_ENTRY(a,b,c)  a,
#include "lib/cpul_cpuop_opcodes.def"
#undef CPU_OPER_ENTRY
	CPUL_OPER_NUM
};

/*! \brief This structure describe  CPU OP VPUL_OP_CTRL_CH_LOOP */
struct vpul_cpu_loop {
	__u32 is_loop_abort				:1;
	__u32 is_task_loop				:1;
	__u32 loop_index				:8;
	__u32 loop_offset				:16;
};

/*! \brief This structure describe  CPU operation VPUL_OP_SEP_FILT_FROM_FRAC */
struct vpul_cpu_sep_flt_frac{
	__u32   total_num_of_records	:16;
};

/*! \brief This structure describe  CPU operation VPUL_OP_LK_STAT_UP_PT_CH_THR */
struct vpul_cpu_lk_stat{
	__u32	up_pt_ch_thr			:16;
	__u32   up_pt_ch_thr_is_fst		:16; /* See enum VPUC_LkStatUpPtChThr_Output*/
	__u32   up_pt_ch_thr_thr;
};


/** \brief This enum describes index option for WA description.
  */

enum   vpul_cpu_index_type_val{
	VPUL_DIRECT_INDEX,
	VPUL_PU_INDEX
};

/** \brief This structure describe  the how to set index in WA table
 */
struct vpul_src_dest_wa_index{
	/* \brief Use pu_location or wa index for src */
	enum   vpul_cpu_index_type_val   src_index_type;
	/* \brief This union describe WA index or pu index */
	union {
		__u32				src_wa_index;
		struct vpul_pu_location		src_pu_index;
	};
      __u32                dest_wa_index;
};

struct vpul_mprb_index{
	/* \brief Use pu_index or mprb index c */
	enum   vpul_cpu_index_type_val   index_type;
	/* \brief This union describe WA index or pu index */
	union {
		__u32				mprb_index:8;
		__u32				mprb_port:8;
		struct vpul_pu_location		pu_index;
	};
};

/** \brief This structure describe the mprbs used for src and dst
 */
struct vpul_src_dest_mprb_index{
	/* \brief SPecfication of index/location of source MPRB */
	struct vpul_mprb_index   src_mprb_index;
	/* \brief SPecfication of index/location of destination MPRB */
	struct vpul_mprb_index   dest_mprb_index;
};

/** \struct vpul_cpu_cpy_src_dest
.* \brief This structure describe  CPU operation VPUL_OP_CPY_PU_RSLTS_2_PU_PRMS
 *
*/
struct vpul_cpu_cpy_src_dest{

	struct vpul_pu_location dest_pu;

 /* the location of the written parameter in the dst PU required  input */
	__u32  offset_in_dest_pu_prms;

 /* the location of the read    parameter in the src PU generated output */
	__u32  offset_in_src_pu_prms;
};

/*! \brief This structure describe  CPU operation VPUC_OP_LK_UPDATE_PT_BY_XY_OFS parameters */
struct vpul_cpu_update_pt_by_xy_offs{
	__u32	cfg_up_pt_ind			:4;
	__u32   cfg_prev_pt_ind			:4;
};

/*! \brief This structure describe  CPU operation VPUL_OP_HW_POST_PROC_HIST_RES_PARAMS_ASSIGMENT parameters */
struct vpul_cpu_post_proc_hist{
	__u32 total_bins  : 11;

};


/* \brief Union for parameters of all different types of Processing units*/
union vpul_cpu_parameters {
	struct vpul_cpu_loop			loop;
	struct vpul_cpu_sep_flt_frac		flt_frac;
	struct vpul_cpu_lk_stat			lk_stat;
	struct vpul_cpu_cpy_src_dest		cpy_src_dest; /*for VPUL_OP_CPY_PU_RSLTS_2_PU_PRMS*/
	struct vpul_cpu_update_pt_by_xy_offs    update_pt_by_xy_offs;
	struct vpul_cpu_post_proc_hist		post_proc_hist;
	__u32  num_of_32B_reported_results; /* number of 32B writings to output buffer using VPUL_OP_WRITE_RPRT_PU_RSLTS */
};


/*! \brief This structure describe one CPU operation */
struct vpul_cpu_op{
	/*! \brief operation opcode */
	enum  vpul_cpu_op_code				opcode;
	struct vpul_src_dest_wa_index			wa_src_dest;
	struct vpul_src_dest_mprb_index			mprb_src_dest;
	union vpul_cpu_parameters			params;
};



#endif /*  __VPU_CPU_OPER__ */