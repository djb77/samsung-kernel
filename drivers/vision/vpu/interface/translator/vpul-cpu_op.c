/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "lib/vpul-def.h"
#include "lib/vpul-errno.h"
#include "lib/vpul-gen.h"
#include "lib/vpul-ds.h"
#include "lib/vpul-translator.h"
#include "lib/vpu-fwif-hw-bl-params.h"
#include "lib/vpu-fwif-commands.h"
#include "lib/vpul-hwmapper.h"
#include "lib/vpu-fwif-cpu-params.h"
#include "vpul_ds_internal.h"

/* for parameters offsets on passing parameters */
#include "lib/vpu-fwif-hw-gen.h"



const __u32 CPU_OPER_PARAM_SUPORTED[CPUL_OPER_NUM] = {
#define CPU_OPER_ENTRY(a,b,c)		c,
#include "lib/cpul_cpuop_opcodes.def"
#undef CPU_OPER_ENTRY
};



static const __u32 CPU_OPER_FWIF_OPCODE[CPUL_OPER_NUM ] = {
#define CPU_OPER_ENTRY(a,b,c)		b,
#include "lib/cpul_cpuop_opcodes.def"
#undef CPU_OPER_ENTRY
};

#define PREP_FLD(FieldName, _Value) \
	(((_Value) & (FieldName ## _MASK)) << FieldName ## _LSB)

typedef __u32 (*__cpu_param_assigment_function)(
	struct vpul_cpu_op *cpu_oper,
	__u16 *params,
	__u32 mprb_src,
	__u32 mprb_dest);


static __u32 VPUL_OP_CTRL_SEP_FILT_FROM_FRAC_PARAMS_ASSIGMENT
									(
	struct vpul_cpu_op *cpu_oper,
	__u16 *params,
	__u32 mprb_src,
	__u32 mprb_dest)
{
	const union vpul_cpu_parameters *cpu_params = &(cpu_oper->params);
	const struct vpul_cpu_sep_flt_frac *flt_frac = &(cpu_params->flt_frac);

	if (params != NULL){
		params[0] = flt_frac->total_num_of_records;

	}

	return VPUC_OP_CTRL_SEP_FILT_FROM_FRAC_PARAMS;
}

static __u32 VPUL_OP_LK_STAT_UP_PT_CH_THR_PARAMS_ASSIGMENT
								(
	struct vpul_cpu_op *cpu_oper,
	__u16 *params,
	__u32 mprb_src,
	__u32 mprb_dest)
{
	const union vpul_cpu_parameters *cpu_params = &cpu_oper->params;
	const struct vpul_cpu_lk_stat *lk_stat = &(cpu_params->lk_stat);

	if (params != NULL){

		params[VPUC_LK_STAT_UP_PT_CH_THR_CFG]    =
				lk_stat->up_pt_ch_thr_is_fst << VPUC_LK_STAT_CFG_IS_FST_LSB |
				lk_stat->up_pt_ch_thr  << VPUC_LK_STAT_CFG_IS_CH_THR_LSB;
		params[VPUC_LK_STAT_UP_PT_CH_THR_THR] =		lk_stat->up_pt_ch_thr_thr;
	}

	return VPUC_OP_LK_STAT_UP_PT_CH_THR_PARAMS;
}

static __u32 VPUL_OP_CTRL_CH_LOOP_PARAMS_ASSIGMENT(
	struct vpul_cpu_op *cpu_oper,
	__u16 *params,
	__u32 mprb_src,
	__u32 mprb_dest)
{
	const union vpul_cpu_parameters *cpu_params = &cpu_oper->params;
	const struct vpul_cpu_loop *loop = &(cpu_params->loop);

	if (params != NULL){
		params[VPUC_CH_LOOP_CFG] =
			(loop->is_loop_abort << VPUC_CH_LOOP_CFG_IS_LOOP_ABORT_LSB |
			loop->is_task_loop   << VPUC_CH_LOOP_CFG_IS_TASK_LOOP_LSB  |
			loop->loop_index	 << VPUC_CH_LOOP_CFG_LOOP_INDEX_LSB);

		params[VPUC_CH_LOOP_MAX_LOOPS_OFS] = loop->loop_offset;
	}
	return VPUC_OP_CTRL_CH_LOOP_PARAMS;
}


static __u32 VPUL_OP_LK_UPDATE_PT_BY_XY_OFS_PARAMS_ASSIGMENT(
	struct vpul_cpu_op *cpu_oper,
	__u16 *params,
	__u32 mprb_src,
	__u32 mprb_dest)
{
	const union vpul_cpu_parameters *cpu_params = &cpu_oper->params;
	const struct vpul_cpu_update_pt_by_xy_offs *update_pt_by_xy_offs =
		&(cpu_params->update_pt_by_xy_offs);

	if (params != NULL){
		params[VPUC_LK_UPDATE_PT_CFG] =
				(update_pt_by_xy_offs->cfg_up_pt_ind <<
					VPUC_LK_UPDATE_PT_CFG_UP_PT_IND_LSB)|
				(update_pt_by_xy_offs->cfg_prev_pt_ind <<
					VPUC_LK_UPDATE_PT_CFG_PREV_PT_IND_LSB);
	}

	return VPUC_OP_LK_UPDATE_PT_BY_XY_OFS_PARAMS;
}



static __u32 VPUL_OP_HW_UPDATE_M2LI_RECORD_PARAMS_ASSIGMENT(
	struct vpul_cpu_op *cpu_oper,
	__u16 *params,
	__u32 mprb_src,
	__u32 mprb_dest)
{
	return 0;
}




static __u32 VPUL_OP_HW_CNN_CHECK_BAD_INPU_PARAMS_ASSIGMENT(
	struct vpul_cpu_op *cpu_oper,
	__u16 *params,
	__u32 mprb_src,
	__u32 mprb_dest)
{BUG_ON(1); return 0;}
static __u32 VPUL_OP_CTRL_COPY_WORK_TO_VEC_PARAMS_ASSIGMENT(
	struct vpul_cpu_op *cpu_oper,
	__u16 *params,
	__u32 mprb_src,
	__u32 mprb_dest)
{BUG_ON(1); return 0;}
static __u32 VPUL_OP_HW_CNN_CHECK_OVERFLOW_PARAMS_ASSIGMENT(
	struct vpul_cpu_op *cpu_oper,
	__u16 *params,
	__u32 mprb_src,
	__u32 mprb_dest)
{BUG_ON(1); return 0;}
static __u32 VPUL_OP_HW_INTIMG_CHECK_OVFL_PARAMS_ASSIGMENT(
	struct vpul_cpu_op *cpu_oper,
	__u16 *params,
	__u32 mprb_src,
	__u32 mprb_dest)
{BUG_ON(1); return 0;}

static __u32 VPUL_OP_FLM_UPDATE_VEC_PARAMS_ASSIGMENT(
	struct vpul_cpu_op *cpu_oper,
	__u16 *params,
	__u32 mprb_src,
	__u32 mprb_dest)
{BUG_ON(1); return 0;}
static __u32 VPUL_OP_SC_ADD_INT_PARAMS_ASSIGMENT(
	struct vpul_cpu_op *cpu_oper,
	__u16 *params,
	__u32 mprb_src,
	__u32 mprb_dest)
{BUG_ON(1); return 0;}
static __u32 VPUL_OP_SC_ADD_FLOAT_PARAMS_ASSIGMENT(
	struct vpul_cpu_op *cpu_oper,
	__u16 *params,
	__u32 mprb_src,
	__u32 mprb_dest)
{BUG_ON(1); return 0;}
static __u32 VPUL_OP_SC_ADD_CONST_TO_INT_PARAMS_ASSIGMENT(
	struct vpul_cpu_op *cpu_oper,
	__u16 *params,
	__u32 mprb_src,
	__u32 mprb_dest)
{BUG_ON(1); return 0;}
static __u32 VPUL_OP_SC_ADD_CONST_TO_FL_PARAMS_ASSIGMENT(
	struct vpul_cpu_op *cpu_oper,
	__u16 *params,
	__u32 mprb_src,
	__u32 mprb_dest)
{BUG_ON(1); return 0;}
static __u32 VPUL_OP_SC_WEIGHTED_ADD_INT_PARAMS_ASSIGMENT(
	struct vpul_cpu_op *cpu_oper,
	__u16 *params,
	__u32 mprb_src,
	__u32 mprb_dest)
{BUG_ON(1); return 0;}
static __u32 VPUL_OP_SC_WEIGHTED_ADD_FLOAT_PARAMS_ASSIGMENT(
	struct vpul_cpu_op *cpu_oper,
	__u16 *params,
	__u32 mprb_src,
	__u32 mprb_dest)
{BUG_ON(1); return 0;}
static __u32 VPUL_OP_SC_SUBTRACT_INT_PARAMS_ASSIGMENT(
	struct vpul_cpu_op *cpu_oper,
	__u16 *params,
	__u32 mprb_src,
	__u32 mprb_dest)
{BUG_ON(1); return 0;}
static __u32 VPUL_OP_SC_SUBTRACT_FLOAT_PARAMS_ASSIGMENT(
	struct vpul_cpu_op *cpu_oper,
	__u16 *params,
	__u32 mprb_src,
	__u32 mprb_dest)
{BUG_ON(1); return 0;}
static __u32 VPUL_OP_SC_MULTIPLY_INT_PARAMS_ASSIGMENT(
	struct vpul_cpu_op *cpu_oper,
	__u16 *params,
	__u32 mprb_src,
	__u32 mprb_dest)
{BUG_ON(1); return 0;}
static __u32 VPUL_OP_SC_MULTIPLY_FLOAT_PARAMS_ASSIGMENT(
	struct vpul_cpu_op *cpu_oper,
	__u16 *params,
	__u32 mprb_src,
	__u32 mprb_dest)
{BUG_ON(1); return 0;}
static __u32 VPUL_OP_SC_DIVIDE_INT_PARAMS_ASSIGMENT(
	struct vpul_cpu_op *cpu_oper,
	__u16 *params,
	__u32 mprb_src,
	__u32 mprb_dest)
{BUG_ON(1); return 0;}
static __u32 VPUL_OP_SC_DIVIDE_FLOAT_PARAMS_ASSIGMENT(
	struct vpul_cpu_op *cpu_oper,
	__u16 *params,
	__u32 mprb_src,
	__u32 mprb_dest)
{BUG_ON(1); return 0;}
static __u32 VPUL_OP_SC_ABS_INT_PARAMS_ASSIGMENT(
	struct vpul_cpu_op *cpu_oper,
	__u16 *params,
	__u32 mprb_src,
	__u32 mprb_dest)
{BUG_ON(1); return 0;}
static __u32 VPUL_OP_SC_ABS_FLOAT_PARAMS_ASSIGMENT(
	struct vpul_cpu_op *cpu_oper,
	__u16 *params,
	__u32 mprb_src,
	__u32 mprb_dest)
{BUG_ON(1); return 0;}
static __u32 VPUL_OP_SC_AND_PARAMS_ASSIGMENT(
	struct vpul_cpu_op *cpu_oper,
	__u16 *params,
	__u32 mprb_src,
	__u32 mprb_dest)
{BUG_ON(1); return 0;}
static __u32 VPUL_OP_SC_OR_PARAMS_ASSIGMENT(
	struct vpul_cpu_op *cpu_oper,
	__u16 *params,
	__u32 mprb_src,
	__u32 mprb_dest)
{BUG_ON(1); return 0;}
static __u32 VPUL_OP_SC_XOR_PARAMS_ASSIGMENT(
	struct vpul_cpu_op *cpu_oper,
	__u16 *params,
	__u32 mprb_src,
	__u32 mprb_dest)
{BUG_ON(1); return 0;}
static __u32 VPUL_OP_SC_NOT_PARAMS_ASSIGMENT(
	struct vpul_cpu_op *cpu_oper,
	__u16 *params,
	__u32 mprb_src,
	__u32 mprb_dest)
{BUG_ON(1); return 0;}
static __u32 VPUL_OP_SC_SH_LEFT_PARAMS_ASSIGMENT(
	struct vpul_cpu_op *cpu_oper,
	__u16 *params,
	__u32 mprb_src,
	__u32 mprb_dest)
{BUG_ON(1); return 0;}
static __u32 VPUL_OP_SC_SH_RIGHT_PARAMS_ASSIGMENT(
	struct vpul_cpu_op *cpu_oper,
	__u16 *params,
	__u32 mprb_src,
	__u32 mprb_dest)
{BUG_ON(1); return 0;}
static __u32 VPUL_OP_SC_FLOAT_PARAMS_ASSIGMENT(
	struct vpul_cpu_op *cpu_oper,
	__u16 *params,
	__u32 mprb_src,
	__u32 mprb_dest)
{BUG_ON(1); return 0;}
static __u32 VPUL_OP_SC_ROUND_PARAMS_ASSIGMENT(
	struct vpul_cpu_op *cpu_oper,
	__u16 *params,
	__u32 mprb_src,
	__u32 mprb_dest){BUG_ON(1); return 0;}
static __u32 VPUL_OP_SC_FLOOR_PARAMS_ASSIGMENT(
	struct vpul_cpu_op *cpu_oper,
	__u16 *params,
	__u32 mprb_src,
	__u32 mprb_dest)
{BUG_ON(1); return 0;}
static __u32 VPUL_OP_SC_CEIL_PARAMS_ASSIGMENT(
	struct vpul_cpu_op *cpu_oper,
	__u16 *params,
	__u32 mprb_src,
	__u32 mprb_dest)
{BUG_ON(1); return 0;}
static __u32 VPUL_OP_SC_TURNC_PARAMS_ASSIGMENT(
	struct vpul_cpu_op *cpu_oper,
	__u16 *params,
	__u32 mprb_src,
	__u32 mprb_dest)
{BUG_ON(1); return 0;}
static __u32 VPUL_OP_SC_SQRT_PARAMS_ASSIGMENT(
	struct vpul_cpu_op *cpu_oper,
	__u16 *params,
	__u32 mprb_src,
	__u32 mprb_dest)
{BUG_ON(1); return 0;}
static __u32 VPUL_OP_SC_EXP_PARAMS_ASSIGMENT(
	struct vpul_cpu_op *cpu_oper,
	__u16 *params,
	__u32 mprb_src,
	__u32 mprb_dest)
{BUG_ON(1); return 0;}
static __u32 VPUL_OP_SC_LOG_PARAMS_ASSIGMENT(
	struct vpul_cpu_op *cpu_oper,
	__u16 *params,
	__u32 mprb_src,
	__u32 mprb_dest)
{BUG_ON(1); return 0;}
static __u32 VPUL_OP_MAT_ADD_PARAMS_ASSIGMENT(
	struct vpul_cpu_op *cpu_oper,
	__u16 *params,
	__u32 mprb_src,
	__u32 mprb_dest)
{BUG_ON(1); return 0;}
static __u32 VPUL_OP_MAT_ADD_CONST_PARAMS_ASSIGMENT(
	struct vpul_cpu_op *cpu_oper,
	__u16 *params,
	__u32 mprb_src,
	__u32 mprb_dest)
{BUG_ON(1); return 0;}
static __u32 VPUL_OP_MAT_WEIGHTED_ADD_PARAMS_ASSIGMENT(
	struct vpul_cpu_op *cpu_oper,
	__u16 *params,
	__u32 mprb_src,
	__u32 mprb_dest)
{BUG_ON(1); return 0;}
static __u32 VPUL_OP_MAT_SUBTRACT_PARAMS_ASSIGMENT(
	struct vpul_cpu_op *cpu_oper,
	__u16 *params,
	__u32 mprb_src,
	__u32 mprb_dest)
{BUG_ON(1); return 0;}
static __u32 VPUL_OP_MAT_MULT_PARAMS_ASSIGMENT(
	struct vpul_cpu_op *cpu_oper,
	__u16 *params,
	__u32 mprb_src,
	__u32 mprb_dest)
{BUG_ON(1); return 0;}
static __u32 VPUL_OP_MAT_MULT_ELEMENTS_PARAMS_ASSIGMENT(
	struct vpul_cpu_op *cpu_oper,
	__u16 *params,
	__u32 mprb_src,
	__u32 mprb_dest)
{BUG_ON(1); return 0;}
static __u32 VPUL_OP_MAT_DIV_ELEMENTS_PARAMS_ASSIGMENT(
	struct vpul_cpu_op *cpu_oper,
	__u16 *params,
	__u32 mprb_src,
	__u32 mprb_dest)
{BUG_ON(1); return 0;}
static __u32 VPUL_OP_STAT_MAX_MIN_PARAMS_ASSIGMENT(
	struct vpul_cpu_op *cpu_oper,
	__u16 *params,
	__u32 mprb_src,
	__u32 mprb_dest)
{BUG_ON(1); return 0;}
static __u32 VPUL_OP_STAT_MEAN_VAR_PARAMS_ASSIGMENT(
	struct vpul_cpu_op *cpu_oper,
	__u16 *params,
	__u32 mprb_src,
	__u32 mprb_dest)
{BUG_ON(1); return 0;}
static __u32 VPUL_OP_STAT_CREATE_HISTOGRAM_PARAMS_ASSIGMENT(
	struct vpul_cpu_op *cpu_oper,
	__u16 *params,
	__u32 mprb_src,
	__u32 mprb_dest)
{BUG_ON(1); return 0;}
static __u32 VPUL_OP_CPY_PU_RSLTS_2_PU_PRMS_PARAMS_ASSIGMENT(
	struct vpul_cpu_op *cpu_oper,
	__u16 *params,
	__u32 mprb_src,
	__u32 mprb_dest)
{


	if (params != NULL){
		params[VPUC_COPY_TO_VEC_CFG] = ( /* 0x15 */
			/* Whether the copied data is 32 bits -> 2 continuous parameters */
			((1&VPUC_COPY_TO_VEC_CFG_IS_32B_MASK)
			<< VPUC_COPY_TO_VEC_CFG_IS_32B_LSB) |

			/* Whether to copy to an output vector / one of parameters vectors */
			((0&VPUC_COPY_TO_VEC_CFG_IS_OUTPUT_DEST_MASK)
			<< VPUC_COPY_TO_VEC_CFG_IS_OUTPUT_DEST_LSB)  |

			/* Whether to copy to current process / entire task parameters vector.
			/ * Relevant only when IS_OUTPUT_DEST is not set*/
			((1&VPUC_COPY_TO_VEC_CFG_IS_PROC_PARAMS_DEST_MASK)
			<< VPUC_COPY_TO_VEC_CFG_IS_PROC_PARAMS_DEST_LSB)  |

			/* The total number of parameter copied */
			((1&VPUC_COPY_TO_VEC_CFG_PARAMS_NUM_MASK)
			<< VPUC_COPY_TO_VEC_CFG_PARAMS_NUM_LSB));


		/* Offset to process parameters vector/ entire task parameters vector /
		* output vector of the 1st copied parameter
		*/
		params[VPUC_COPY_TO_VEC_DEST_OFS] = 0x1111; /* value to be ran over */
	}
	return VPUC_OP_CTRL_COPY_WORK_TO_VEC_PARAMS;
}



static __u32 VPUL_OP_HW_POST_PROC_HIST_RES_PARAMS_ASSIGMENT(
	struct vpul_cpu_op *cpu_oper,
	__u16 *params,
	__u32 mprb_src,
	__u32 mprb_dest)
{

	if (params != NULL){
		params[VPUC_HW_POST_PROC_CFG]	=
			PREP_FLD(VPUC_HW_POST_PROC_CFG_INT_MEM_IND,
			mprb_src )|
			PREP_FLD(VPUC_HW_POST_PROC_CFG_TOTAL_BINS,
			cpu_oper->params.post_proc_hist.total_bins );
	}
	return  VPUC_OP_HW_POST_PROC_HIST_RES_PARAMS;
}

static __u32 VPUL_OP_EQHIST_CREATE_LUT_PARAMS_ASSIGMENT(
	struct vpul_cpu_op *cpu_oper,
	__u16 *params,
	__u32 mprb_src,
	__u32 mprb_dest)
{

	if (params != NULL){
		params[0] = mprb_dest;

	}
	return  VPUC_OP_EQHIST_CREATE_LUT_PARAMS;
}

static __u32 VPUL_OP_WRITE_RPRT_PU_RSLTS_PARAMS_ASSIGMENT(
	struct vpul_cpu_op *cpu_oper,
	__u16 *params,
	__u32 mprb_src,
	__u32 mprb_dest)
{


	if (params != NULL){
		params[VPUC_COPY_TO_VEC_CFG] = (
			/* Whether the copied data is 32 bits -> 2 continuous parameters */
			((1&VPUC_COPY_TO_VEC_CFG_IS_32B_MASK)
			<< VPUC_COPY_TO_VEC_CFG_IS_32B_LSB) |

			/* Whether to copy to an output vector (1) / one of parameters vectors (0) */
			((1&VPUC_COPY_TO_VEC_CFG_IS_OUTPUT_DEST_MASK)
			<< VPUC_COPY_TO_VEC_CFG_IS_OUTPUT_DEST_LSB)  |

			/* Whether to copy to current process / entire task parameters vector.
			/ * **NOT Relevant here**:IS_OUTPUT_DEST is set*/
			((0&VPUC_COPY_TO_VEC_CFG_IS_PROC_PARAMS_DEST_MASK)
			<< VPUC_COPY_TO_VEC_CFG_IS_PROC_PARAMS_DEST_LSB)  |

			/* The total number of parameter copied */
			((cpu_oper->params.num_of_32B_reported_results&VPUC_COPY_TO_VEC_CFG_PARAMS_NUM_MASK)
			<< VPUC_COPY_TO_VEC_CFG_PARAMS_NUM_LSB)  |

			/*memory slot - 0 on current implementation */
			((0&VPUC_COPY_TO_VEC_CFG_OUT_VEC_IND_MASK)
			<< VPUC_COPY_TO_VEC_CFG_OUT_VEC_IND_LSB));

		/* Offset to process parameters vector/ entire task parameters vector /
		* output vector of the 1st copied parameter
		*/
		params[VPUC_COPY_TO_VEC_DEST_OFS] = 0x0; /* on current implementation, offset 0 on memory buffer */
	}
	return VPUC_OP_CTRL_COPY_WORK_TO_VEC_PARAMS;
}



static __cpu_param_assigment_function cpu_param_assigment_func[CPUL_OPER_NUM] = {
#define CPU_OPER_ENTRY(a,b,c)		a##_PARAMS_ASSIGMENT,
#include "lib/cpul_cpuop_opcodes.def"
#undef CPU_OPER_ENTRY
};

__u32 get_mprb_index(
	const struct vpul_task* task_ptr,
	struct vpul_mprb_index * mprb_index )
{

	if(mprb_index->index_type==VPUL_DIRECT_INDEX){
		return mprb_index->mprb_index;
	}
	else{
		struct vpul_pu* src_pu_ptr=
			find_pu_by_pu_location_struct_ptr(
			&(mprb_index->pu_index),
			task_ptr);
		return src_pu_ptr->mprbs[mprb_index->mprb_port];

	}

};

static __u32 set_cpuoper_desc(
	struct internal_context* intrnl_ctxt,
	const struct vpul_task* task_ptr,
	struct vpul_cpu_op *cpu_oper,
	/* pointer to mbox block descriptor */
	struct  VPUI_CpuOp *mb_cpuoper, /* output */
	/* offset of block first parameter from sub chain start */
	__u32  *param_offset,
	__u16 *mb_cpuoper_params)
{
	__u32 work_area_src;
	__u32 work_area_dest;
	__u32 mprb_dest;
	__u32 mprb_src;
	__u32 lut_counter;
	__u32 src_pu_found = 0;



	mprb_dest = get_mprb_index(task_ptr, &(cpu_oper->mprb_src_dest.dest_mprb_index));
	mprb_src = get_mprb_index(task_ptr, &(cpu_oper->mprb_src_dest.src_mprb_index));
	/* fill WA src and dest */


	/* WA out is directly defined from TDS */
	work_area_dest = cpu_oper->wa_src_dest.dest_wa_index;
	/* WA in is handled differently in case of PU and CPU  wrote to CPU area*/
	/* that because PU out location in the WA is determined by FW, and CPU - by user. */
	if (VPUL_DIRECT_INDEX == cpu_oper->wa_src_dest.src_index_type){
		work_area_src = cpu_oper->wa_src_dest.src_wa_index;
	}
	else /* derive WA index from pu index */
	{
		struct vpul_pu* src_pu_ptr=
			find_pu_by_pu_location_struct_ptr(
				&(cpu_oper->wa_src_dest.src_pu_index),
				task_ptr);
		/* find our pu on the LUT */
		for (
			lut_counter = 0;
			(!src_pu_found) && lut_counter<intrnl_ctxt->pu_output_location_next;
			lut_counter++
			)
		{
			if (src_pu_ptr == intrnl_ctxt->pu_output_location_entry_table[lut_counter].pu_idenifier)
			{/* pu found in working area, get details */
				work_area_src =
					/* the specific parameter of PU to take inside PU*/
					cpu_oper->params.cpy_src_dest.offset_in_src_pu_prms +
					/* pu base offset in WA */
					intrnl_ctxt->pu_output_location_entry_table[lut_counter].offset_in_work_area;
					src_pu_found = 1; /* for breaking loop and handling no-found case */
			}
		}
		if (!src_pu_found) {BUG_ON(1);} /* src pu points to non-tabled PU */

	}

	if (VPUL_OP_WRITE_RPRT_PU_RSLTS == cpu_oper->opcode)
		/* currently, allocating max required in VPUL_OP_WRITE_RPRT_PU_RSLTS cpu ops */
		if (intrnl_ctxt->required_output_mem_allc_16B <(2*cpu_oper->params.num_of_32B_reported_results))
			intrnl_ctxt->required_output_mem_allc_16B =(2*cpu_oper->params.num_of_32B_reported_results);
	if (CPU_OPER_SUPPORTED == CPU_OPER_PARAM_SUPORTED[cpu_oper->opcode])
	{
		mb_cpuoper->OpCode = CPU_OPER_FWIF_OPCODE[cpu_oper->opcode];

		/* just way to see if we have to read workArea.to params */
		if (VPUL_OP_CPY_PU_RSLTS_2_PU_PRMS == cpu_oper->opcode)
		{
			struct vpul_pu_location* dest_pu = &(cpu_oper->params.cpy_src_dest.dest_pu);
			struct vpul_pu* dst_pu_ptr= find_pu_by_pu_location_struct_ptr(dest_pu,task_ptr);
			struct runtime_updated_pu_entry* free_entry;

			/* update table, in order to set output to param vector */

			/* find next empty entry (holds NULL) on  runtime_updated_pu_table*/
			free_entry = fnd_fst_val_apprnce_in_runtime_upd_tbl(intrnl_ctxt->runtime_updated_pu_table,NULL);

				/* assert if table full  */
			BUG_ON(free_entry == NULL);

			/* set dest identifier (also validates the table)*/
			free_entry->updated_pu_identifier = dst_pu_ptr;

			/* set src specific location for writing*/
			free_entry->location_to_write_updated_prms_offset =
				/* offset of all cpu params    */
				(mb_cpuoper_params       +
				/* offset of current cpu param */
				(*param_offset)          +
				/* offset of relevant param    */
				VPUC_COPY_TO_VEC_DEST_OFS);
			free_entry->offset_in_dst_pu_prms =
				cpu_oper->params.cpy_src_dest.offset_in_dest_pu_prms;


		}

		mb_cpuoper->WorkAreaInd =
			(((work_area_src <<CPU_OP_WORK_AREA_IN_FST_IND_LSB ) &
				(CPU_OP_WORK_AREA_IND_MASK<<CPU_OP_WORK_AREA_IN_FST_IND_LSB )) |
			 ((work_area_dest<<CPU_OP_WORK_AREA_OUT_FST_IND_LSB) &
				(CPU_OP_WORK_AREA_IND_MASK<<CPU_OP_WORK_AREA_OUT_FST_IND_LSB)));

		mb_cpuoper->FstParamOffset = *param_offset;

		*param_offset += cpu_param_assigment_func[cpu_oper->opcode](
			cpu_oper,
			mb_cpuoper_params + (*param_offset),
			mprb_src,
			mprb_dest);



	}
	else
	{
		return VPU_STATUS_CPU_OPER_NOT_SUPORTED;
	}

	return VPU_STATUS_SUCCESS;
}


/* fill Mbox command with CPU operation information */
__u32 set_cpu_subchain_cpu_ops_params
(
	struct internal_context* intrnl_ctx, /* required for PU-workarea LUT */
	const struct vpul_task *task,/* required for getting PU from pu location struct */
	struct vpul_cpu_op *first_cpuoper_in_sc,
	__u32 n_cpuoper_in_sc,
	__u16 *mb_cpuoper_params,
	struct VPUI_CpuOp **mb_cpu_oper_desc,
	__u32 *sc_cpu_param_ofs_idx
)
{
	__u32 sc_cpu_idx;
	struct vpul_cpu_op *cpu_oper = first_cpuoper_in_sc;
	__s32  stat;

	for (sc_cpu_idx = 0; sc_cpu_idx < n_cpuoper_in_sc; sc_cpu_idx++, cpu_oper++){
		stat = set_cpuoper_desc( intrnl_ctx,
					task,
					cpu_oper,
					*mb_cpu_oper_desc,
					sc_cpu_param_ofs_idx, /* ofst from  start of cpu params */
					mb_cpuoper_params);   /* start of cpu params */
		if (VPU_STATUS_IS_FAILURE(stat))
			return stat;

		(*mb_cpu_oper_desc)++;
	}

	return VPU_STATUS_SUCCESS;
}

__u32 get_cpuop_param_number(struct vpul_cpu_op * cpu_oper){

	if(cpu_oper==NULL||!CPU_OPER_PARAM_SUPORTED[cpu_oper->opcode]){
		return 0;
	}
	return cpu_param_assigment_func[cpu_oper->opcode](cpu_oper,NULL,0,0);

}

/* trn idx of first entry that holds "desired_vlaue" in (entry.updated_pu_identifier) */
struct runtime_updated_pu_entry* fnd_fst_val_apprnce_in_runtime_upd_tbl(
		struct runtime_updated_pu_entry* table_head,
		struct vpul_pu* desired_vlaue)
{
	__u32 i;
	__u32 entry_found = 0;
	for (i=0;
		((!entry_found) && i<MAXIMAL_NUM_OF_RUNTIME_PRM_UPDATES);
		i++)
			entry_found = (table_head[i].updated_pu_identifier == desired_vlaue);

	if (entry_found)
		return &(table_head[i-1]); /* i is increased by 1 after loop break  */
	else
		return NULL; /* entry in table if found, NULL if not found*/
}
