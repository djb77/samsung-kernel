/**
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef VPU_FWIF_CPU_PARAMS_H
#define VPU_FWIF_CPU_PARAMS_H

/*********** Explicit CPU operations parameters interface description ********/
/* The enum below describe all the CPU operations that might be invoked by
 * Processes and 3DNN processes
 */
enum VPUC_CpuOpCode {
	/****** Control operations ******/
	/* Copy from working area to Output or params vector using VPUC_CopyWorkAreaToVec_Params */
	VPUC_OP_CTRL_COPY_WORK_TO_VEC	= 0,
	/* Update single parameter in params vector using VPUC_UpdateParamField_Params */
	VPUC_OP_CTRL_UPDATE_PARAM_FIELD	= 1,
	/* Check whether another loop is required. using VPUC_ChLoop_Params */
	VPUC_OP_CTRL_CH_LOOP		= 2,
	/* Update 3x3 separable filter from -1<= X,Y Input<= 1 using VPUC_SepFiltFromFrac_Params */
	VPUC_OP_CTRL_SEP_FILT_FROM_FRAC	= 3,
	/***** Use HW results (sent on working area) for control *****/
	/* Update the total output of M2LI, Single parameter */
	VPUC_OP_HW_UPDATE_M2LI_RECORDS	= 4,
	VPUC_OP_HW_CNN_CHECK_BAD_INPUT	= 5,
	VPUC_OP_HW_CNN_CHECK_OVERFLOW	= 6,
	VPUC_OP_HW_INTIMG_CHECK_OVFL	= 7,
	/***** Update HW results ******/
	/* using VPUC_HwPostProcHist_Params + VPUC_HwPostProcHistRes_Output */
	VPUC_OP_HW_POST_PROC_HIST_RES	= 8,	/* Out = SumHist */
	/****** Specific applications operations ******/
	/* using VPUC_LkStatUpPtChThr_Params and VPUC_LkStatUpPtChThr_Output */
	VPUC_OP_LK_STAT_UP_PT_CH_THR	= 9,
	VPUC_OP_LK_UPDATE_PT_BY_XY_OFS	= 10,
	/* using VPUC_DsprGetBgGray_Params + VPUC_DsprGetBgGray_Output */
	VPUC_OP_DSPR_GET_BG_GRAY	= 11,
	/* using VPUC_DsprCompMinMax_Output */
	VPUC_OP_DSPR_COMP_MIN_MAX_DSPR	= 12,
	/* using VPUC_DsprCompBgDisp_Params + VPUC_DsprCompBgDisp_Output */
	VPUC_OP_DSPR_COMP_BG_DISP	= 13,
	/* using VPUC_HwPostProcHistRes_Output used as input parameters */
	VPUC_OP_EQHIST_CREATE_LUT	= 14,
	VPUC_OP_FLM_UPDATE_VEC		= 15,
	/****** General purpose calculations ******/
	/*** Copy data to ouput working area ***/
	/* using VPUC_GetSizes_Output */
	VPUC_OP_GET_SIZES		= 16,
	/* using VPUC_GetHistParams_Output */
	VPUC_OP_GET_HIST_PARAMS		= 17,
	/* Copy ROI Group Index to output area. Output = Group Index */
	VPUC_OP_GET_ROI_GR_IND		= 18,
	/*** Scalar operations - most of them don't have parameters ***/
	VPUC_OP_SC_ADD_INT		= 19,	/* Out = In0 + In1 */
	VPUC_OP_SC_ADD_FLOAT		= 20,
	VPUC_OP_SC_ADD_CONST_TO_INT	= 21,	/* Out = In0 + Const */
	VPUC_OP_SC_ADD_CONST_TO_FL	= 22,
	VPUC_OP_SC_WEIGHTED_ADD_INT	= 23, 	/* Out = In0 * W0 + In1 * W1 */
	VPUC_OP_SC_WEIGHTED_ADD_FLOAT	= 24,
	VPUC_OP_SC_SUBTRACT_INT		= 25, 	/* Out = In0 - In1 */
	VPUC_OP_SC_SUBTRACT_FLOAT	= 26,
	VPUC_OP_SC_MULTIPLY_INT		= 27, 	/* Out = In0 * In1 */
	VPUC_OP_SC_MULTIPLY_FLOAT	= 28,
	VPUC_OP_SC_DIVIDE_INT		= 29, 	/* Out = In0 / In1 */
	VPUC_OP_SC_DIVIDE_FLOAT		= 30,
	VPUC_OP_SC_ABS_INT		= 31, 	/* Out = abs(in0) */
	VPUC_OP_SC_ABS_FLOAT		= 32,
	VPUC_OP_SC_AND			= 33, 	/* Out = In0 & In1 */
	VPUC_OP_SC_OR			= 34, 	/* Out = In0 | In1 */
	VPUC_OP_SC_XOR			= 35, 	/* Out = In0 ^ In1 */
	VPUC_OP_SC_NOT			= 36, 	/* Out = ~In0 */
	VPUC_OP_SC_SH_LEFT		= 37, 	/* Out = In0 << In1 */
	VPUC_OP_SC_SH_RIGHT		= 38, 	/* Out = In0 >> In1 */
	VPUC_OP_SC_FLOAT		= 39, 	/* Out = Float(in0) */
	VPUC_OP_SC_ROUND		= 40, 	/* Out = Round(in0) */
	VPUC_OP_SC_FLOOR		= 41, 	/* Out = floor(in0) */
	VPUC_OP_SC_CEIL			= 42, 	/* Out = ceil(in0) */
	VPUC_OP_SC_TURNC		= 43,	/* Out = trunc(in0) */
	VPUC_OP_SC_SQRT			= 44, 	/* Out = Sqrt(in0) */
	VPUC_OP_SC_EXP			= 45, 	/* Out = exp(in0) */
	VPUC_OP_SC_LOG			= 46, 	/* Out = log(in0) */
	/*** Matrix operations ***/
	VPUC_OP_MAT_ADD			= 47,
	VPUC_OP_MAT_ADD_CONST		= 48,
	VPUC_OP_MAT_WEIGHTED_ADD	= 49,
	VPUC_OP_MAT_SUBTRACT		= 50,
	VPUC_OP_MAT_MULT		= 51,
	VPUC_OP_MAT_MULT_ELEMENTS	= 52,
	VPUC_OP_MAT_DIV_ELEMENTS	= 53,
	/*** Statistics operations ***/
	/* Using VPUC_MaxMin_Params + VPUC_MaxMin_Output */
	VPUC_OP_STAT_MAX_MIN		= 54,
	/* Using VPUC_MeanVar_Params + VPUC_MeanVar_Output */
	VPUC_OP_STAT_MEAN_VAR		= 55,
	VPUC_OP_STAT_CREATE_HISTOGRAM	= 56,
#if defined(HOST_INVOKE_CODE) && !defined(HOST_EXTERN_INPUT)
	VPUC_OP_MEMSET			= 57,
	VPUC_OP_COPY_MEM		= 58,
	VPUC_OP_ALL			= 59
#else
	VPUC_OP_ALL			= 57
#endif
};


/********** VPUC_OP_CTRL_COPY_WORK_TO_VEC parameters *************************/
enum VPUC_CopyWorkAreaToVec_Params {
	/* VPUC_COPY_TO_VEC_CFG fields */
	VPUC_COPY_TO_VEC_CFG			= 0,
	/* Offset to process parameters vector/ entire task parameters vector /
	 * output vector of the 1st copied parameter
	 */
	VPUC_COPY_TO_VEC_DEST_OFS		= 1,
	VPUC_OP_CTRL_COPY_WORK_TO_VEC_PARAMS	= 2
};

/* VPUC_COPY_TO_VEC_CFG fields */
/* Whether the copied data is 32 bits -> 2 continuous parameters */
#define VPUC_COPY_TO_VEC_CFG_IS_32B_LSB			0
#define VPUC_COPY_TO_VEC_CFG_IS_32B_MASK			1
/* Whether to copy to an output vector / one of parameters vectors */
#define VPUC_COPY_TO_VEC_CFG_IS_OUTPUT_DEST_LSB		1
#define VPUC_COPY_TO_VEC_CFG_IS_OUTPUT_DEST_MASK		1
/* Whether to copy to current process / entire task parameters vector.
 * Relevant only when IS_OUTPUT_DEST is not set
 */
#define VPUC_COPY_TO_VEC_CFG_IS_PROC_PARAMS_DEST_LSB	2
#define VPUC_COPY_TO_VEC_CFG_IS_PROC_PARAMS_DEST_MASK		1
/* The total number of parameter copied */
#define VPUC_COPY_TO_VEC_CFG_PARAMS_NUM_LSB		4
/* VPUI_MAX_WORK_AREA_INDICES == 16 */
#define VPUC_COPY_TO_VEC_CFG_PARAMS_NUM_MASK			0xf
/* The Index of the output vector
 * Relevant only when IS_OUTPUT_DEST is set
 */
#define VPUC_COPY_TO_VEC_CFG_OUT_VEC_IND_LSB		8
#define VPUC_COPY_TO_VEC_CFG_OUT_VEC_IND_MASK			0xff


/********** VPUC_OP_CTRL_UPDATE_PARAM_FIELD parameters *************************/
enum VPUC_UpdateParamField_Params {
	/* VPUC_UPDATE_PARAM_CFG fields */
	VPUC_UPDATE_PARAM_CFG			= 0,
	/* Offset to process parameters vector/ entire task parameters vector */
	VPUC_UPDATE_PARAM_DEST_OFS		= 1,
	VPUC_OP_CTRL_UPDATE_PARAM_FIELD_PARAMS	= 2
};

/* VPUC_UPDATE_PARAM_CFG fields */
/* Whether to copy to current process / entire task parameters vector */
#define VPUC_UPDATE_PARAM_CFG_IS_PROC_PARAMS_DEST_LSB	0
#define VPUC_UPDATE_PARAM_CFG_IS_PROC_PARAMS_DEST_MASK		1
/* The LSB of the parameter that should be updated */
#define VPUC_UPDATE_PARAM_CFG_UPDATE_SH_LSB		4
#define VPUC_UPDATE_PARAM_CFG_UPDATE_SH_MASK			0xf
#define VPUC_UPDATE_PARAM_CFG_UPDATE_BITS_NUM_LSB	8
#define VPUC_UPDATE_PARAM_CFG_UPDATE_BITS_NUM_MASK		0xf

/************** VPUC_OP_CTRL_CH_LOOP parameters ******************************/
enum VPUC_ChLoop_Params {
	/* VPUC_CH_LOOP_CFG fields */
	VPUC_CH_LOOP_CFG		= 0,
	/* 0 stands for "forever"-on this case "abort" condition must be met */
	VPUC_CH_LOOP_MAX_LOOPS_OFS	= 1,
	VPUC_OP_CTRL_CH_LOOP_PARAMS	= 2
};

/* VPUC_COPY_TO_VEC_CFG fields */
/* Set to 1 if loop might be aborted before maximal loops performed. On this
 * case if the first input to the function (on the working area) != 0 - the
 * loop is aborted
 */
#define VPUC_CH_LOOP_CFG_IS_LOOP_ABORT_LSB	0
#define VPUC_CH_LOOP_CFG_IS_LOOP_ABORT_MASK		1
/* Set to 1 for task loop, 0 for internal process loop */
#define VPUC_CH_LOOP_CFG_IS_TASK_LOOP_LSB	1
#define VPUC_CH_LOOP_CFG_IS_TASK_LOOP_MASK		1
#ifdef OLD_FWIF_IO
/* The index of task loop. Relevant only when CFG_IS_TASK_LOOP is set */
#else
/* The index of loop.*/
#endif
#define VPUC_CH_LOOP_CFG_LOOP_INDEX_LSB		8
#define VPUC_CH_LOOP_CFG_LOOP_INDEX_MASK		0xff

enum VPUC_SepFiltFromFrac_Params {
	/* VPUC_FL_FROM_FRAC_CFG fields */
	VPUC_SEP_FILT_FROM_FRAC_CFG		= 0,
	VPUC_OP_CTRL_SEP_FILT_FROM_FRAC_PARAMS	= 1
};
/* The index of the updated separable filter coefficient set. The set assumed to be dynamic, its
 * vector size is assumed to be 3 and its Fraction bits assumed to be 14.
 */
#define VPUC_FL_FROM_FRAC_CFG_FILT_IND_LSB		0
#define VPUC_FL_FROM_FRAC_CFG_FILT_IND_MASK			0xff
#ifdef OLD_FWIF_IO
/* The index of the Pt List the fractions are derived from + 1. In case this parameter is set to 0
 * the assumption is that the input X & Y fractions are already written on the input vector
 */
#define VPUC_FL_FROM_FRAC_CFG_PT_LST_IND_P1_LSB		8
#define VPUC_FL_FROM_FRAC_CFG_PT_LST_IND_P1_MASK		0xff
#else
/* The index of the Pt List the fractions are derived from + 1. In case this parameter is set to 0
 * the assumption is that the input X & Y fractions are already written on the input vector
 */
#define VPUC_FL_FROM_FRAC_CFG_ROI_FROM_PT_IND_P1_LSB	8
#define VPUC_FL_FROM_FRAC_CFG_ROI_FROM_PT_IND_P1_MASK		0xff
#endif

#ifdef OLD_FWIF_IO
/* The parameter describes the output index + 1 of the total number of records.
 * Set to 0 if not set as output. Usually total number is equal to TotalRecords
 * in VPUI_OutListHdr but TotalRecords is limited by MaxRecords (and optionally
 * map is written instead of list).
*/
#else
/* The parameter describes the dynamic output list index. */
#endif
#define VPUC_OP_HW_UPDATE_M2LI_RECORDS_PARAMS	1
#define VPUC_OP_HW_CNN_CHECK_BAD_INPUT_PARAMS	0
#define VPUC_OP_HW_CNN_CHECK_OVERFLOW_PARAMS	0
#define VPUC_OP_HW_INTIMG_CHECK_OVFL_PARAMS	0

/************ VPUC_OP_HW_POST_PROC_HIST_RES parameters ************************/
enum VPUC_HwPostProcHist_Params {
	/* VPUC_HW_POST_PROC_CFG fields */
	VPUC_HW_POST_PROC_CFG			= 0,
	VPUC_OP_HW_POST_PROC_HIST_RES_PARAMS	= 1
};
#define VPUC_HW_POST_PROC_CFG_INT_MEM_IND_LSB	0
#define VPUC_HW_POST_PROC_CFG_INT_MEM_IND_MASK		0xf
#define VPUC_HW_POST_PROC_CFG_TOTAL_BINS_LSB	4
	/* Up to 1024 */
#define VPUC_HW_POST_PROC_CFG_TOTAL_BINS_MASK		0x7ff

enum VPUC_HwPostProcHistRes_Output {
	VPUC_HW_POST_PROC_HIST_ADR		= 0,
	VPUC_HW_POST_PROC_HIST_SUM		= 1,
	VPUC_OP_HW_POST_PROC_HIST_RES_OUTS	= 2
};

/************ VPUC_OP_LK_STAT_UP_PT_CH_THR parameters ************************/
/* The function reads the HW statistics and derive x+y offsets which update the
 * total x+y offsets and are used to decide whether another loop is required
 */
enum VPUC_LkStatUpPtChThr_Params {
	/* 16 bits Fraction */
	/* VPUC_LK_STAT_CFG fields */
	VPUC_LK_STAT_UP_PT_CH_THR_CFG		= 0,
	/* In 1e-6 units => MaxValue = 0.065 Relevant only when LK_STAT_CFG_IS_CH_THR is set */
	VPUC_LK_STAT_UP_PT_CH_THR_THR		= 1,
	VPUC_OP_LK_STAT_UP_PT_CH_THR_PARAMS	= 2
};

#define VPUC_LK_STAT_CFG_IS_FST_LSB		0
#define VPUC_LK_STAT_CFG_IS_FST_MASK			1
#define VPUC_LK_STAT_CFG_IS_CH_THR_LSB		1
#define VPUC_LK_STAT_CFG_IS_CH_THR_MASK			1

enum VPUC_LkStatUpPtChThr_Output {
	/* 16 bits Fraction */
	VPUC_LK_STAT_OUT_IS_THR			= 0,
	VPUC_LK_STAT_OUT_X_OFS			= 1,
	VPUC_LK_STAT_OUT_Y_OFS			= 2,
	VPUC_LK_STAT_OUT_PREV_X_OFS		= 3,
	VPUC_LK_STAT_OUT_PREV_Y_OFS		= 4,
	VPUC_OP_LK_STAT_UP_PT_CH_THR_OUTS	= 5
};

/********** VPUC_OP_LK_UPDATE_PT_BY_XY_OFS parameters ************************/
enum VPUC_LkUpdatePtByXyOfs_Params {
	/* VPUC_LK_UPDATE_PT_CFG fields */
	VPUC_LK_UPDATE_PT_CFG			= 0,
	VPUC_OP_LK_UPDATE_PT_BY_XY_OFS_PARAMS	= 1
};

/* VPUC_LK_UPDATE_PT_CFG fields */
#ifdef OLD_FWIF_IO
/* The index of the list of the updated point. */
#else
/* The index of the VPUI_RoiFromPt of the updated point. */
#endif
#define VPUC_LK_UPDATE_PT_CFG_UP_PT_IND_LSB	0
#define VPUC_LK_UPDATE_PT_CFG_UP_PT_IND_MASK		0xf
#ifdef OLD_FWIF_IO
/* The index of the list of the "old" point. Might be equal to list index of the updated point */
#else
/* Index of VPUI_RoiFromPt of the "old" point. Might be equal to list index of the updated point */
#endif
#define VPUC_LK_UPDATE_PT_CFG_PREV_PT_IND_LSB	4
#define VPUC_LK_UPDATE_PT_CFG_PREV_PT_IND_MASK		0xf

/********** VPUC_OP_DSPR_GET_BG_GRAY parameters ******************************/
enum VPUC_DsprGetBgGray_Params {
	/* VPUC_DSPR_GET_BG_CFG fields */
	VPUC_DSPR_GET_BG_CFG		= 0,
	VPUC_OP_DSPR_GET_BG_GRAY_PARAMS	= 1
};

/* VPUC_DSPR_GET_BG_CFG fields */
	/* The index of VPUI_HistParams */
#define VPUC_DSPR_GET_BG_CFG_HIST_PARS_IND_LSB	0
#define VPUC_DSPR_GET_BG_CFG_HIST_PARS_IND_MASK		0xff
	/* The index of the sizes of the input image to the histogram */
#define VPUC_DSPR_GET_BG_CFG_SIZES_IND_LSB	8
	/* 0x1f */
#define VPUC_DSPR_GET_BG_CFG_SIZES_IND_MASK		VPUI_SIZES_IND_MASK

enum VPUC_DsprGetBgGray_Output{
	VPUC_DSPR_BG_OUT_GRAY_VAL	= 0,
	VPUC_DSPR_BG_OUT_ALLOW_EN	= 1,
	VPUC_OP_DSPR_GET_BG_GRAY_OUTS	= 2
};
/* The parameter describes the VPUI_HistParams index */
#define VPUC_OP_DSPR_COMP_MIN_MAX_DSPR_PARAMS	1

enum VPUC_DsprCompMinMax_Output{
	VPUC_DSPR_MIN_MAX_DSPR_HW		= 0,
	VPUC_DSPR_MIN_DSPR_PARAM		= 1,
	VPUC_DSPR_SM_TO_LARGE_VAL		= 2,
	VPUC_OP_DSPR_COMP_MIN_MAX_DSPR_OUTS	= 3
};

/********** VPUC_OP_DSPR_COMP_BG_DISP parameters ******************************/
enum VPUC_DsprCompBgDisp_Params {
	VPUC_DSPR_COMP_BG_HIST_PARAMS_IND	= 0,
	/* Updated by VPUC_OP_DSPR_GET_BG_GRAY operation */
	VPUC_DSPR_COMP_BG_ALLOW_BG_ENABLE	= 1,
	/* Updated by VPUC_OP_DSPR_COMP_MIN_MAX_DSPR operation */
	VPUC_DSPR_COMP_BG_MIN_DISPARITY		= 2,
	VPUC_OP_DSPR_COMP_BG_DISP_PARAMS	= 3
};

enum VPUC_DsprCompBgDisp_Output{
	VPUC_DSPR_COMP_BG_OUT_BG_DSPR	= 0,
	VPUC_DSPR_COMP_BG_OUT_ALLOW_BG	= 1,
	VPUC_OP_DSPR_COMP_BG_DISP_OUTS	= 2
};

/* The parameter describes the index of the internal RAM used for the LUT  */
#define VPUC_OP_EQHIST_CREATE_LUT_PARAMS	1
#define VPUC_OP_FLM_UPDATE_VEC_PARAMS		0
/* The parameter describes the sizes index */
#define VPUC_OP_GET_SIZES_PARAMS		1

enum VPUC_GetSizes_Output {
	VPUC_GET_SIZES_WIDTH_OFS	= 0,
	VPUC_GET_SIZES_HEIGHT_OFS	= 1,
	VPUC_OP_GET_SIZES_OUTS		= 2
};

/* The parameter describes the VPUI_HistParams index */
#define VPUC_OP_GET_HIST_PARAMS_PARAMS		1
enum VPUC_GetHistParams_Output {
	VPUC_GET_HIST_PARAMS_OFFSET_OFS		= 0,
	VPUC_GET_HIST_PARAMS_MAX_VAL_OFS	= 1,
	VPUC_GET_HIST_PARAMS_INV_BINSIZE_M1_OFS	= 2,
	VPUC_OP_GET_HIST_PARAMS_OUTS		= 3
};

/* The parameter describes the InOut index that Group Index is copied from its ROI */
#define VPUC_OP_GET_ROI_GR_IND_PARAMS		1

#define VPUC_OP_SC_ADD_INT_PARAMS		0
#define VPUC_OP_SC_ADD_FLOAT_PARAMS		0
/* The parameter describes the const value */
#define VPUC_OP_SC_ADD_CONST_TO_INT_PARAMS	1
/* The parameter describes an integer const value */
#define VPUC_OP_SC_ADD_CONST_TO_FL_PARAMS	1
/* The parameters describes 2 8 bits fraction weights  */
#define VPUC_OP_SC_WEIGHTED_ADD_INT_PARAMS	2
/* The parameters describes 2 8 bits fraction weights  */
#define VPUC_OP_SC_SUBTRACT_INT_PARAMS		0
#define VPUC_OP_SC_SUBTRACT_FLOAT_PARAMS	0
#define VPUC_OP_SC_MULTIPLY_INT_PARAMS		0
#define VPUC_OP_SC_MULTIPLY_FLOAT_PARAMS	0
#define VPUC_OP_SC_DIVIDE_INT_PARAMS		0
#define VPUC_OP_SC_DIVIDE_FLOAT_PARAMS		0
#define VPUC_OP_SC_ABS_INT_PARAMS		0
#define VPUC_OP_SC_ABS_FLOAT_PARAMS		0
#define VPUC_OP_SC_AND_PARAMS			0
#define VPUC_OP_SC_OR_PARAMS			0
#define VPUC_OP_SC_XOR_PARAMS			0
#define VPUC_OP_SC_NOT_PARAMS			0
#define VPUC_OP_SC_SH_LEFT_PARAMS		0
#define VPUC_OP_SC_SH_RIGHT_PARAMS		0
#define VPUC_OP_SC_FLOAT_PARAMS			0
#define VPUC_OP_SC_ROUND_PARAMS			0
#define VPUC_OP_SC_FLOOR_PARAMS			0
#define VPUC_OP_SC_CEIL_PARAMS			0
#define VPUC_OP_SC_TURNC_PARAMS			0
#define VPUC_OP_SC_SQRT_PARAMS			0
#define VPUC_OP_SC_EXP_PARAMS			0
#define VPUC_OP_SC_LOG_PARAMS			0

/****************** VPUC_OP_STAT_MAX_MIN Parameters **************************/
/* Max Min operation is invoked on either 2D or 1D input. The CPU calculates
 * either Max only / Min only or both. Its output is for each calculated value
 * (max, min or both) the value and the 2D or 1D indices.
 */
enum VPUC_MaxMin_Params {
	/* VPUC_MAXMIN_CFG	fields */
	VPUC_MAXMIN_CFG			= 0,
	VPUC_OP_STAT_MAX_MIN_PARAMS	= 1
};

/* VPUC_MAXMIN_CFG fields */
	/* whether max should be searched */
#define VPUC_MAXMIN_CFG_FIND_MAX_LSB		0
#define VPUC_MAXMIN_CFG_FIND_MAX_MASK			1
	/* whether min should be searched */
#define VPUC_MAXMIN_CFG_FIND_MIN_LSB		1
#define VPUC_MAXMIN_CFG_FIND_MIN_MASK			1
	/*	The index of IO type for map start address + line offset */
#define VPUC_MAXMIN_CFG_INOUT_INDEX_LSB		2
	/* VPUI_MAX_INOUT_PER_PROC == 32 */
#define VPUC_MAXMIN_CFG_INOUT_INDEX_MASK		0x1f
	/* The index of the input sizes of the input map */
#define VPUC_MAXMIN_CFG_SIZES_INDEX_LSB		8
	/* VPUI_MAX_SIZES_PER_PROC == 32 */
#define VPUC_MAXMIN_CFG_SIZES_INDEX_MASK		0x1f

enum VPUC_MaxMin_Output {
	VPUC_MAXMIN_OUT_MAX_VALUE	= 0,
	VPUC_MAXMIN_OUT_MAX_COLUMN	= 1,
	VPUC_MAXMIN_OUT_MAX_LINE	= 2,
	VPUC_MAXMIN_OUT_MIN_VALUE	= 3,
	VPUC_MAXMIN_OUT_MIN_COLUMN	= 4,
	VPUC_MAXMIN_OUT_MIN_LINE	= 5,
	VPUC_OP_STAT_MAX_MIN_OUTS	= 6
};

/***************** VPUC_OP_STAT_MEAN_VAR Parameters **************************/
/* MeanVar operation is invoked on either 2D or 1D input. The CPU calculates and
 * output Mean and optionally also Variance.
 */

enum VPUC_MeanVar_Params {
	/* VPUC_MEANVAR_CFG fields */
	VPUC_MEANVAR_CFG		= 0,
	VPUC_OP_STAT_MEAN_VAR_PARAMS	= 1
};

/* VPUC_MEANVAR_CFG fields */
	/* whether min should be searched */
#define VPUC_MEANVAR_CFG_CALC_VAR_LSB		1
#define VPUC_MEANVAR_CFG_CALC_VAR_MASK			1
	/* The index of IO type for map start address + line offset */
#define VPUC_MEANVAR_CFG_INOUT_INDEX_LSB	2
	/* VPUI_MAX_INOUT_PER_PROC == 32 */
#define VPUC_MEANVAR_CFG_INOUT_INDEX_MASK		0x1f
	/* The index of the input sizes of the input map */
#define VPUC_MEANVAR_CFG_SIZES_INDEX_LSB	8
	/* VPUI_MAX_SIZES_PER_PROC == 32 */
#define VPUC_MEANVAR_CFG_SIZES_INDEX_MASK		0x1f

enum VPUC_MeanVar_Output {
	VPUC_MEANVAR_OUT_MEAN		= 0,
	VPUC_MEANVAR_OUT_VAR		= 1,
	VPUC_OP_STAT_MEAN_VAR_OUTS	= 2
};

#if defined(HOST_INVOKE_CODE) && !defined(HOST_EXTERN_INPUT)
/************************* Memset Parameters *********************************/
/* Gets an index of IO type and the value that should be written to the map. */
enum VPUC_Memset_Params {
	/* VPUC_MEMSET_CFG fields */
	VPUC_MEMSET_CFG		= 0,
	/* The value that should be written */
	VPUC_MEMSET_VALUE	= 1,
	VPUC_OP_MEMSET_PARAMS	= 2
};

/* VPUC_MEMSET_CFG fields */
	/* The index of IO type for map start address + line offset */
#define VPUC_MEMSET_CFG_INOUT_INDEX_LSB		0
	/* VPUI_MAX_INOUT_PER_PROC == 32 */
#define VPUC_MEMSET_CFG_INOUT_INDEX_MASK		0x1f
	/* The index of the input sizes of the input map */
#define VPUC_MEMSET_CFG_SIZES_INDEX_LSB		8
	/* VPUI_MAX_SIZES_PER_PROC == 32 */
#define VPUC_MEMSET_CFG_SIZES_INDEX_MASK		0x1f

/********************** Copy Memory Parameters ********************************
 * Gets an index of IO type (for output address), list of values that should be
 * written to the memory and list size.
 * The values are written continuously after VPUC_OP_COPY_MEM_PARAMS.
 */

enum VPUC_CpMem_Params {
	/* VPUC_CPMEM_CFG fields */
	VPUC_CPMEM_CFG		= 0,
	VPUC_OP_COPY_MEM_PARAMS	= 1
};

/* VPUC_CPMEM_CFG fields */
	/* The number of parameters copied to the memory */
#define VPUC_CPMEM_CFG_WR_SIZE_LSB		0
#define VPUC_CPMEM_CFG_WR_SIZE_MASK			0x3f
	/*	The index of IO type for output start address */
#define VPUC_CPMEM_CFG_INOUT_INDEX_LSB		8
	/* VPUI_MAX_INOUT_PER_PROC == 32 */
#define VPUC_CPMEM_CFG_INOUT_INDEX_MASK			0x1f
#endif

#endif
