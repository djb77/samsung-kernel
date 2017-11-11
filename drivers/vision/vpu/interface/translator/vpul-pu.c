/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/bug.h>

#include "lib/vpul-def.h"
#include "lib/vpul-errno.h"
#include "lib/vpul-gen.h"
#include "lib/vpul-ds.h"
#include "lib/vpul-translator.h"
#include "lib/vpu-fwif-hw-bl-params.h"
#include "lib/vpu-fwif-commands.h"
#include "lib/vpul-hw-v2.1.h"
#include "vpul_ds_internal.h"

#define HHW(x) (((x) >> 16) & 0xFFFF)
#define LHW(x) ((x) & 0xFFFF)

#define VPUL_DISPARITY_PU_NUM	(3)

static const struct pu_ram_port_range pu_inst_2_ram_port[VPU_PU_NUMBER] = {
#define VPU_PU_INSTANCE(a, b, c, d, e, f, g, h, i) {f, g},
#include "lib/vpul_pu_instances.def"
#undef VPU_PU_INSTANCE
};

typedef __u32 (*__param_assigment_function)
	(const struct vpul_pu *pu, __u16 *gen_params, __u32 mprb_gr_ofs, __u32 special_param );

static void hist_special_param_set(const struct vpul_pu_histogram *hist, struct VPUI_HistParams *hist_params);
static void depth_special_params_set
(
	const struct vpul_pu_fast_depth *fast,
	struct VPUI_DisparityFixedThr *disp_params
);
static void inpaint_special_params_set
(
	const struct vpul_pu_in_paint *inpaint,
	struct VPUI_DisparityFixedThr *disp_params
);
static void dispeq_special_params_set
(
	const struct vpul_pu_dispeq *dispeq,
	struct VPUI_DisparityFixedThr *disp_params
);

#define PRE_FLD_NO_CHECK(FieldName, _Value) \
	(((_Value) & (FieldName ## _MASK)) << FieldName ## _LSB)

//#define VPUL_CHECK_REG_FIELDS
#ifdef VPUL_CHECK_REG_FIELDS

#define VPUL_STRINGIZE_NX(A) #A
#define VPUL_STRINGIZE(A) VPUL_STRINGIZE_NX(A)

#define CHECK_VAL_IN_MASK(FieldName, _Value) \
	(((_Value) & ~(FieldName ## _MASK)) != 0)

#define PRINT_ERR_AND_RET_VAL(FieldName, _Value) \
	((VPU_ERRO("bad PU field: %s val: %d\n",\
	VPUL_STRINGIZE(FieldName), _Value) &&  1) ? \
	PRE_FLD_NO_CHECK(FieldName, _Value) :\
	PRE_FLD_NO_CHECK(FieldName, _Value))


#define PREP_FLD(FieldName, _Value)\
	(CHECK_VAL_IN_MASK(FieldName, _Value) ? \
	PRINT_ERR_AND_RET_VAL(FieldName, _Value) : \
	PRE_FLD_NO_CHECK(FieldName, _Value))
#else
#define PREP_FLD(FieldName, _Value)	PRE_FLD_NO_CHECK(FieldName, _Value)
#endif


static const __u32 is_preload_needed_for_pu[VPU_PU_TYPES_NUMBER]
= {
#define VPU_PU_TYPE(a, b, c) c,
#include "lib/vpul_pu_types.def"
#undef VPU_PU_TYPE
};

static const enum VPU_PU_TYPES pu_inst2type[VPU_PU_NUMBER + 1] = {
#define VPU_PU_INSTANCE(a, b, c, d, e, f, g, h,i) b,
#include "lib/vpul_pu_instances.def"
	/* indicates "end of array" */
	END_OF_PU_INST2TYPE_TRANSLATOR
#undef VPU_PU_INSTANCE
};

/* Debug macros */


static __u32 get_offset_to_1st_ram_port_used(const struct vpul_pu *pu)
{
	__u32 index = 0;

	if (pu->n_mprbs) {
		for (index = 0; index < VPU_MAXIMAL_MPRB_CONNECTED; index++) {
			if (pu->mprbs[index] != NO_MPRB_CONNECTED)
				break;
		}
	}
	return index;
}

__u32 pu_get_num_mprb_groups(
	const struct vpul_pu *pu)
{
	__u32 num_of_mprb_connections;
	__u32 previous_entry;
	__u32 i;
	__u32 num_mprb = 0;
	__u32 num_groups = 0;

	num_of_mprb_connections =
			pu_inst_2_ram_port[pu->instance].number_of_ram_ports;
	if(pu->n_mprbs > num_of_mprb_connections){
		return 0;
	}

	if (pu->n_mprbs) {
		num_groups = 1;
		previous_entry = pu->mprbs[0];
		for (i = 1; i < num_of_mprb_connections; i++) {
			if (previous_entry != NO_MPRB_CONNECTED)
				num_mprb++; /* # of mprbs till (including) previous mprb */
			if (num_mprb == pu->n_mprbs) { /* previous entry is the last mprb */
				/* adding 1 - for all remaining RAM ports
				 * with no actively connected MPRBs
				 */
				num_groups++;
				break;
			}
			if (previous_entry == NO_MPRB_CONNECTED) {
				if (pu->mprbs[i] != NO_MPRB_CONNECTED)
					num_groups++;
			} else if (pu->mprbs[i] != previous_entry + 1)
				num_groups++;
			previous_entry = pu->mprbs[i];
		}
	}
	return num_groups;
}

__u32 pu_get_num_mprb_groups_and_fill_groups(const struct vpul_pu *pu,
						struct VPUI_MprbGr *hw_mprbs)
{
	__u32 num_of_mprb_connections;
	__u32 previous_entry;
	__u32 i;
	__u32 num_connections_in_group;
	__u32 is_new_group;
	__u32 num_mprb = 0;
	__u32 num_groups = 0;

	num_of_mprb_connections =
			pu_inst_2_ram_port[pu->instance].number_of_ram_ports;
	BUG_ON(pu->n_mprbs > num_of_mprb_connections);

	if (pu->n_mprbs) {
		previous_entry = pu->mprbs[0];
		if (previous_entry == NO_MPRB_CONNECTED)
			hw_mprbs[0].FstMprbId = 0;
		else
			hw_mprbs[0].FstMprbId = previous_entry + 1;
		num_groups = 0;
		num_connections_in_group = 1;
		for (i = 1; i < num_of_mprb_connections; i++) {
			is_new_group = 0;
			if (previous_entry != NO_MPRB_CONNECTED)
				num_mprb++;
			if (num_mprb == pu->n_mprbs)
				break;
			if (previous_entry == NO_MPRB_CONNECTED) {
				if (pu->mprbs[i] != NO_MPRB_CONNECTED)
					is_new_group = 1;
			} else if (pu->mprbs[i] != previous_entry + 1)
				is_new_group = 1;
			if (is_new_group) {
				hw_mprbs[num_groups].MprbsNum = num_connections_in_group;
				num_groups++;
				if (pu->mprbs[i] == NO_MPRB_CONNECTED)
					hw_mprbs[num_groups].FstMprbId = 0;
				else
					hw_mprbs[num_groups].FstMprbId = pu->mprbs[i] + 1;
				num_connections_in_group = 1;
			}
			else
				num_connections_in_group++;

			previous_entry = pu->mprbs[i];
		}
		hw_mprbs[num_groups].MprbsNum = num_connections_in_group;
		num_groups++;
		if (i < num_of_mprb_connections) {
			hw_mprbs[num_groups].FstMprbId = 0;
			hw_mprbs[num_groups].MprbsNum = num_of_mprb_connections - i;
			num_groups++;
		}
	}
	return num_groups;
}

/* Needs to be defined in FW interface files */
#define VPUI_INTIMG_CFG_DT_SH_RIGHT_MASK VPUI_INTIMG_CFG_DT_SHIFTERS_MASK
#define	VPUI_INTIMG_CFG_DT_SH_LEFT_MASK	 VPUI_INTIMG_CFG_DT_SHIFTERS_MASK
static __u32 VPUL_OP_INTEGRAL_IMG_PARAMS_ASSIGMENT(const struct vpul_pu *pu,
	__u16 *params,
	__u32 mprb_gr_ofs,
	 __u32 special_param )
{
	const union vpul_pu_parameters *pu_params = &pu->params;
	const struct vpul_pu_integral *intg_pr = &pu_params->integral;
	__u32 mprb_fst_ofs;

	mprb_fst_ofs = get_offset_to_1st_ram_port_used(pu);

	if (params != NULL) {
		params[VPUI_INTIMG_CFG] =
			PREP_FLD(VPUI_INTIMG_CFG_MODE,
				intg_pr->integral_image_mode) |
			PREP_FLD(VPUI_INTIMG_CFG_OVFLOW,
				intg_pr->overflow_mode) |
			PREP_FLD(VPUI_INTIMG_CFG_CC_MODE,
				intg_pr->cc_scan_mode) |
			PREP_FLD(VPUI_INTIMG_CFG_CC_SMART_LABLE,
				intg_pr->cc_smart_label_search_en) |
			PREP_FLD(VPUI_INTIMG_CFG_CC_RESET_LABLES,
				intg_pr->cc_reset_labels_array) |
			PREP_FLD(VPUI_INTIMG_CFG_DT_SH_LEFT,
				intg_pr->dt_left_shift) |
			PREP_FLD(VPUI_INTIMG_CFG_DT_SH_RIGHT,
				intg_pr->dt_right_shift);

		params[VPUI_INTIMG_MPRB] =
			PREP_FLD(VPUI_INTIMG_MPRB_FST_OFS, mprb_fst_ofs) |
			PREP_FLD(VPUI_INTIMG_MPRB_GR_OFS, mprb_gr_ofs);

		/* Defualt values for modes these ar enot needed */
		params[VPUI_INTIMG_DT_DIAG_LUT_OVERFLOW] = 0;
		params[VPUI_INTIMG_DT_LUT_UNDERFLOW] = 0;

		switch (intg_pr->integral_image_mode) {
		case VPUH_INTIMG_MODE_DIS_TRANS_MAN:
		case VPUH_INTIMG_MODE_DIS_TRANS_EUC:
			params[VPUI_INTIMG_DT_HOR_CC_MIN_LABEL_LUT_SIZE] =
				intg_pr->dt_coefficient0;
			params[VPUI_INTIMG_DT_VER_CC_LABEL_SIZE_LUT_ADD] =
				intg_pr->dt_coefficient1;
			params[VPUI_INTIMG_DT_DIAG_LUT_OVERFLOW] =
				intg_pr->dt_coefficient2;
			break;
		case VPUH_INTIMG_MODE_CONNECTED_COMP:
			params[VPUI_INTIMG_DT_HOR_CC_MIN_LABEL_LUT_SIZE] =
				intg_pr->cc_min_label;
			params[VPUI_INTIMG_DT_VER_CC_LABEL_SIZE_LUT_ADD] =
				intg_pr->cc_label_vector_size;
			break;
		case VPUH_INTIMG_MODE_LUT:
		default: /*fall-through*/
			params[VPUI_INTIMG_DT_HOR_CC_MIN_LABEL_LUT_SIZE] =
				intg_pr->lut_number_of_values;
			params[VPUI_INTIMG_DT_VER_CC_LABEL_SIZE_LUT_ADD] =
				intg_pr->lut_value_shift;
			params[VPUI_INTIMG_DT_DIAG_LUT_OVERFLOW] =
				intg_pr->lut_default_overflow;
			params[VPUI_INTIMG_DT_LUT_UNDERFLOW] =
				intg_pr->lut_default_underflow;
			break;
		}
	}

	return VPUI_INTIMG_PARAMS;
}

static __u32 VPUL_OP_JOIN_PARAMS_ASSIGMENT(const struct vpul_pu *pu,
	__u16 *params,
	__u32 mprb_gr_ofs,
 __u32 special_param ){
	const union vpul_pu_parameters *pu_params = &pu->params;
	const struct vpul_pu_joiner *joiner_prm = &pu_params->joiner;

	if (params != NULL) {
		__u32 inputEnable =
			(joiner_prm->input0_enable) |
			(joiner_prm->input1_enable<<1) |
			(joiner_prm->input2_enable<<2) |
			(joiner_prm->input3_enable<<3);

		__u32 outputEnableInv =
			((joiner_prm->out_byte0_source_stream&0x10)>>4) |
			(((joiner_prm->out_byte1_source_stream&0x10)>>4)<< 1)|
			(((joiner_prm->out_byte2_source_stream&0x10)>>4)<< 2)|
			(((joiner_prm->out_byte3_source_stream&0x10)>>4)<< 3);



		__u32 byteSource =
			((joiner_prm->out_byte0_source_stream&
				VPUI_JOIN_BYTES_OUT_MASK)<<0)|
			((joiner_prm->out_byte1_source_stream&
				VPUI_JOIN_BYTES_OUT_MASK)<<4)|
			((joiner_prm->out_byte2_source_stream&
				VPUI_JOIN_BYTES_OUT_MASK)<<8)|
			((joiner_prm->out_byte3_source_stream&
				VPUI_JOIN_BYTES_OUT_MASK)<<12);

		__u32 outputEnable = (~outputEnableInv)&0xF;

		params[VPUI_DWSC_CFG] =
			(PREP_FLD(VPUI_JOIN_CFG_MODE, joiner_prm->work_mode) |
			PREP_FLD(VPUI_JOIN_CFG_EN_INPUT, inputEnable) |
			PREP_FLD(VPUI_JOIN_CFG_EN_OUT_BYTES, outputEnable));
		params[VPUI_JOIN_BYTES] = byteSource;
	}
	return VPUI_JOIN_PARAMS;
}

static __u32 VPUL_OP_UPSCALER_PARAMS_ASSIGMENT(const struct vpul_pu *pu,
	__u16 *params,
	__u32 mprb_gr_ofs,
 __u32 special_param ){
	const union vpul_pu_parameters *pu_params = &pu->params;
	const struct vpul_pu_upscaler *upsc_prm = &pu_params->upscaler;
	__u32 interpolationModeVal = 0;
	if (params == NULL)
		return VPUI_UPSC_PARAMS;

	interpolationModeVal = upsc_prm->interpolation_method;
	if((upsc_prm->interpolation_method==VPUH_SC_MODE_AREA_NO_ROUND)&&
		(upsc_prm->do_rounding)){
		interpolationModeVal = VPUH_SC_MODE_AREA_DO_ROUND;
	}


	params[VPUI_UPSC_CFG] =
		((pu->out_size_idx << VPUI_UPSC_CFG_OUT_SIZES_IND_LSB) &
		VPUI_UPSC_CFG_OUT_SIZES_IND_MASK) |
		((mprb_gr_ofs << VPUI_UPSC_CFG_MPRB_GR_OFS_LSB) &
		VPUI_UPSC_CFG_MPRB_GR_OFS_MASK) |
		((interpolationModeVal << VPUI_UPSC_CFG_MODE_LSB) &
		VPUI_UPSC_CFG_MODE_MASK) |
		(((upsc_prm->border_fill_mode) << VPUI_UPSC_CFG_BORDER_LSB) &
		VPUI_UPSC_CFG_BORDER_MASK);

	params[VPUI_UPSC_BORDER_CONST] = upsc_prm->border_fill_constant;

	return VPUI_UPSC_PARAMS;
}

static __u32 VPUL_OP_DOWNSCALER_PARAMS_ASSIGMENT(const struct vpul_pu *pu,
	__u16 *params,
	__u32 mprb_gr_ofs,
 __u32 special_param ){
	const union vpul_pu_parameters *pu_params = &pu->params;
	const struct vpul_pu_downscaler *ds_prm = &pu_params->downScaler;

	__u32 interpolationModeVal;

	interpolationModeVal = ds_prm->interpolation_method;
	if((interpolationModeVal==VPUH_SC_MODE_AREA_NO_ROUND)&&
		(ds_prm->do_rounding)){
			interpolationModeVal = VPUH_SC_MODE_AREA_DO_ROUND;
	}


	if (params != NULL) {
		params[VPUI_DWSC_CFG] =	(PREP_FLD(VPUI_DWSC_CFG_OUT_SIZES_IND,
		pu->out_size_idx) |
		PREP_FLD(VPUI_DWSC_CFG_MPRB_GR_OFS, mprb_gr_ofs) |
		PREP_FLD(VPUI_DWSC_CFG_MODE, interpolationModeVal));
	}
	return VPUI_DWSC_PARAMS;
}

static __u32 VPUL_OP_DUPLICATE_PARAMS_ASSIGMENT(const struct vpul_pu *pu,
	__u16 *params,
	__u32 mprb_gr_ofs,
 __u32 special_param ){
	/*Duplicate has no parameters*/
	return 0;
}

/* TODO: needs to be defined in FW interface, not here */
#define VPUI_SPLIT_BYTES_FST_B0_MASK		VPUI_SPLIT_BYTES_OUT_MASK
#define VPUI_SPLIT_BYTES_FST_B1_MASK		VPUI_SPLIT_BYTES_OUT_MASK
#define VPUI_SPLIT_BYTES_SCND_B0_MASK		VPUI_SPLIT_BYTES_OUT_MASK
#define VPUI_SPLIT_BYTES_SCND_B1_MASK		VPUI_SPLIT_BYTES_OUT_MASK

static __u32 VPUL_OP_SPLIT_PARAMS_ASSIGMENT(const struct vpul_pu *pu,
	__u16 *params,
	__u32 mprb_gr_ofs,
 __u32 special_param ){
	const union vpul_pu_parameters *pu_params = &pu->params;
	const struct vpul_pu_spliter *splt_p = &pu_params->spliter;

	if (params != NULL) {
		params[VPUI_SPLIT_BYTES_OUT_0_1] = (
		PREP_FLD(VPUI_SPLIT_BYTES_FST_B0, splt_p->out0_byte0) |
		PREP_FLD(VPUI_SPLIT_BYTES_FST_B1, splt_p->out0_byte1) |
		PREP_FLD(VPUI_SPLIT_BYTES_SCND_B0, splt_p->out1_byte0) |
		PREP_FLD(VPUI_SPLIT_BYTES_SCND_B1, splt_p->out1_byte1));
		params[VPUI_SPLIT_BYTES_OUT_2_3] = (
		PREP_FLD(VPUI_SPLIT_BYTES_FST_B0, splt_p->out2_byte0) |
		PREP_FLD(VPUI_SPLIT_BYTES_FST_B1, splt_p->out2_byte1) |
		PREP_FLD(VPUI_SPLIT_BYTES_SCND_B0, splt_p->out3_byte0) |
		PREP_FLD(VPUI_SPLIT_BYTES_SCND_B1, splt_p->out3_byte1));
	}

	return VPUI_SPLIT_PARAMS;
}

/* set DMA parameters. returns the number of parameters */
static __u32 VPUL_OP_DMA_PARAMS_ASSIGMENT(const struct vpul_pu *pu,
					  __u16 *params,
	__u32 mprb_gr_ofs,
 __u32 special_param ){
	const union vpul_pu_parameters *pu_params = &pu->params;
	const struct vpul_pu_dma *pu_dma_params = &pu_params->dma;

	if (params == NULL)
		return VPUI_DMA_PARAMS;

	params[VPUI_DMA_CFG] =
		(pu_dma_params->inout_index << VPUI_DMA_CFG_INOUT_INDEX_LSB) |
		(pu_dma_params->offset_lines_inc << VPUI_DMA_CFG_OFS_LINES_LSB);


	return VPUI_DMA_PARAMS;
}

/**
 * set full SALB parameters. returns the number of parameters
 * this function performs the actual SALB parameters assignment operation
 * it is called by VPUL_OP_FULL_SALB_PARAMS_ASSIGMENT and
 * VPUL_OP_ADD_PARAMS_ASSIGMENT
 * It is implemented as a separate function so that it can take
 * struct vpul_pu_salb as input argument, instead of struct vpul_pu
 * this makes it easier for providing this input when called by
 * VPUL_OP_ADD_PARAMS_ASSIGMENT
 */
static __u32 full_salb_params_assign(const struct vpul_pu_salb *salb,
				     __u16 *param)
{
	/* LUT for dynamic parameters for salb block based on mode */
	const __u8 dynamic_params_salb[18] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 5, 0};
	__u16 *dy_pr;
	__u32 tot_param;
	__u32 dynamic_params = dynamic_params_salb[salb->operation_code];

	if ((dynamic_params == 0) && salb->const_in1)
		dynamic_params = 1;

	tot_param = VPUI_ALB_PARAMS + dynamic_params;
	if (param == NULL)
		return tot_param;

	param[VPUI_ALB_STREAM] =
		(((salb->input_enable & 0x2) ? 0x1 : 0x0) <<
		VPUI_ALB_STREAM_INPUT1_EN_LSB) |
		(salb->use_mask << VPUI_ALB_STREAM_MASK_EN_LSB) |
		(salb->bits_in0 << VPUI_ALB_STREAM_IN0_BITS_LSB) |
		(salb->bits_in1 << VPUI_ALB_STREAM_IN1_BITS_LSB) |
		(salb->bits_out0 << VPUI_ALB_STREAM_OUT_BITS_LSB) |
		(salb->signed_in0 << VPUI_ALB_STREAM_IN0_SIGN_LSB) |
		(salb->signed_in1 << VPUI_ALB_STREAM_IN1_SIGN_LSB) |
		(salb->signed_out0 << VPUI_ALB_STREAM_OUT_SIGN_LSB) |
		(0 << VPUI_ALB_STREAM_SI_SEL_IN_LSB);

	param[VPUI_ALB_OP] =
		(salb->operation_code << VPUI_ALB_OP_MODE_LSB) |
		(salb->abs_neg << VPUI_ALB_OP_ABS_NEG_LSB) |
		(salb->trunc_out << VPUI_ALB_OP_TRUNC_OUT_LSB) |
		(salb->org_val_med << VPUI_ALB_OP_ORG_VAL_LSB) |
		(salb->cmp_op << VPUI_ALB_OP_COMP_LSB) |
		(salb->const_in1 << VPUI_ALB_OP_VAL_MED_AS_IN1_LSB);

	param[VPUI_ALB_BITS_AND_DY] =
		(salb->shift_bits << VPUI_ALB_BITS_AND_DY_SH_BITS_LSB) |
		((salb->salbregs_custom_trunc_en ?
			salb->salbregs_custom_trunc_bittage:0)
			<< VPUI_ALB_BITS_AND_DY_OUT_BITS_LSB) |
		(dynamic_params << VPUI_ALB_BITS_AND_DY_DYN_PARS_LSB);

	dy_pr = param + VPUI_ALB_PARAMS;
	if (dynamic_params == 1)
		dy_pr[VPUI_SALB_DYN_VAL_MED] = salb->val_med_filler;

	if (dynamic_params == 5) {
		dy_pr[VPUI_SALB_DYN_VAL_MED] = salb->val_med_filler;
		dy_pr[VPUI_SALB_DYN_VAL_LO] = salb->val_lo;
		dy_pr[VPUI_SALB_DYN_VAL_HI] = salb->val_hi;
		dy_pr[VPUI_SALB_DYN_THR_LO] = salb->thresh_lo;
		dy_pr[VPUI_SALB_DYN_THR_HI] = salb->thresh_hi;
	}

	return tot_param;
}

/* set full SALB parameters. returns the number of parameters */
static __u32 VPUL_OP_FULL_SALB_PARAMS_ASSIGMENT(const struct vpul_pu *pu,
	__u16 *param,
	__u32 mprb_gr_ofs,
 __u32 special_param ){
	const union vpul_pu_parameters *pu_params = &pu->params;
	const struct vpul_pu_salb *salb = &(pu_params->salb);

	return full_salb_params_assign(salb, param);
}

/* set add parameters. returns the number of parameters */
static __u32 VPUL_OP_ADD_PARAMS_ASSIGMENT(const struct vpul_pu *pu,
	__u16 *param,
	__u32 mprb_gr_ofs,
 __u32 special_param ){
	const union vpul_pu_parameters *pu_params = &pu->params;
	const struct vpul_pu_add *add_in2 = &(pu_params->add);

	struct vpul_pu_salb add2 = {0};

	add2.input_enable = 0x3;
	add2.bits_in0 = add_in2->bits;
	add2.bits_in1 = add_in2->bits;
	add2.bits_out0 = add_in2->bits;
	add2.signed_in0 = add_in2->is_signed;
	add2.signed_in1 = add_in2->is_signed;
	add2.signed_out0 = add_in2->is_signed;
	add2.operation_code = 8; /* TODO: Add enum */

	return full_salb_params_assign(&add2, param);
}

/* TODO: these definitions should be don in FW interface definitions */
#define VPUI_ALB_STREAM_INPUT1_EN_MASK		1
#define VPUI_ALB_STREAM_MASK_EN_MASK		1
#define VPUI_ALB_STREAM_IN0_BITS_MASK		VPUI_ALB_STREAM_BITS_MASK
#define VPUI_ALB_STREAM_IN1_BITS_MASK		VPUI_ALB_STREAM_BITS_MASK
#define VPUI_ALB_STREAM_OUT_BITS_MASK		VPUI_ALB_STREAM_BITS_MASK

#define VPUI_ALB_STREAM_IN0_SIGN_MASK		VPUI_ALB_STREAM_SIGN_MASK
#define VPUI_ALB_STREAM_IN1_SIGN_MASK		VPUI_ALB_STREAM_SIGN_MASK
#define VPUI_ALB_STREAM_OUT_SIGN_MASK		VPUI_ALB_STREAM_SIGN_MASK
#define VPUI_ALB_STREAM_SI_SEL_IN_MASK		1

/* set full CALB parameters. returns the number of parameters */
static __u32 VPUL_OP_FULL_CALB_PARAMS_ASSIGMENT(const struct vpul_pu *pu,
	__u16 *param,
	__u32 mprb_gr_ofs,
 __u32 special_param ){
	const union vpul_pu_parameters *pu_params = &pu->params;
	const struct vpul_pu_calb *calb = &(pu_params->calb);

	/* LUT for dynamic parameters for calb block based on mode */
	const __u8 dynamic_params_calb[30] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 10, 0,
		0, 0, 4, 6, 4, 4, 4, 6, 0, 0, 0, 0};
	__u16 *dy_pr;
	__u32 dynamic_params = dynamic_params_calb[calb->operation_code];
	__u32 tot_param;

	if ((dynamic_params == 0) && calb->const_in1)
		dynamic_params = 2;

	tot_param = VPUI_ALB_PARAMS + dynamic_params;
	if (param == NULL)
		return tot_param;

	param[VPUI_ALB_STREAM] =
		PREP_FLD(VPUI_ALB_STREAM_INPUT1_EN,
		(calb->input_enable & 0x2) ? 0x1 : 0x0) |
		PREP_FLD(VPUI_ALB_STREAM_MASK_EN,   calb->use_mask) |
		PREP_FLD(VPUI_ALB_STREAM_IN0_BITS, calb->bits_in0) |
		PREP_FLD(VPUI_ALB_STREAM_IN1_BITS, calb->bits_in1) |
		PREP_FLD(VPUI_ALB_STREAM_OUT_BITS, calb->bits_out0) |
		PREP_FLD(VPUI_ALB_STREAM_IN0_SIGN, calb->signed_in0) |
		PREP_FLD(VPUI_ALB_STREAM_IN1_SIGN, calb->signed_in1) |
		PREP_FLD(VPUI_ALB_STREAM_OUT_SIGN, calb->signed_out0) |
		PREP_FLD(VPUI_ALB_STREAM_SI_SEL_IN, 0)|
		PREP_FLD(VPUI_ALB_STREAM_ROUND_MODE, calb->mult_round);

	param[VPUI_ALB_OP] =
		(calb->operation_code << VPUI_ALB_OP_MODE_LSB) |
		(calb->abs_neg << VPUI_ALB_OP_ABS_NEG_LSB) |
		(calb->trunc_out << VPUI_ALB_OP_TRUNC_OUT_LSB) |
		(calb->org_val_med << VPUI_ALB_OP_ORG_VAL_LSB) |
		(calb->cmp_op << VPUI_ALB_OP_COMP_LSB) |
		(calb->const_in1 << VPUI_ALB_OP_VAL_MED_AS_IN1_LSB);

	param[VPUI_ALB_BITS_AND_DY] =
		(calb->shift_bits << VPUI_ALB_BITS_AND_DY_SH_BITS_LSB) |
		(dynamic_params << VPUI_ALB_BITS_AND_DY_DYN_PARS_LSB);

	dy_pr = param + VPUI_ALB_PARAMS;
	switch (dynamic_params) {

	case 10:
		dy_pr[VPUI_CALB_DYN_VAL_HI_MSB] = HHW(calb->val_hi);
		dy_pr[VPUI_CALB_DYN_VAL_HI_LSB] = LHW(calb->val_hi);
		dy_pr[VPUI_CALB_DYN_VAL_LO_MSB] = HHW(calb->val_lo);
		dy_pr[VPUI_CALB_DYN_VAL_LO_LSB] = LHW(calb->val_lo);
		/* Intentional fall through */
	case 6:
		dy_pr[VPUI_CALB_DYN_W1_OR_THR_HI_MSB] = HHW(calb->thresh_hi);
		dy_pr[VPUI_CALB_DYN_W1_OR_THR_HI_LSB] = LHW(calb->thresh_hi);
		/* Intentional fall through */
	case 4:
		dy_pr[VPUI_CALB_DYN_W0_OR_THR_LO_MSB] = HHW(calb->thresh_lo);
		dy_pr[VPUI_CALB_DYN_W0_OR_THR_LO_LSB] = LHW(calb->thresh_lo);
		/* Intentional fall through */
	case 2:
		dy_pr[VPUI_CALB_DYN_VAL_MED_MSB] = HHW(calb->val_med_filler);
		dy_pr[VPUI_CALB_DYN_VAL_MED_LSB] = LHW(calb->val_med_filler);
		break;
	/* case (0) */
	default:
		break;
	}

	return tot_param;
}

/* set ROI parameters. returns the number of parameters */
static __u32 VPUL_OP_ROI_PARAMS_ASSIGMENT(const struct vpul_pu *pu,
					  __u16 *params,
	__u32 mprb_gr_ofs,
 __u32 special_param ){
	const union vpul_pu_parameters *pu_params = &pu->params;
	const struct vpul_pu_rois_out *rois_out = &(pu_params->rois_out);

	if (params == NULL)
		return VPUI_ROI_PARAMS;

	params[VPUI_ROI_STREAM] =
		(rois_out->bits_in0 << VPUI_ROI_STREAM_BITS_IN0_LSB) |
		(rois_out->signed_in0 << VPUI_ROI_STREAM_SIGN_IN0_LSB) |
		(rois_out->use_mask << VPUI_ROI_STREAM_USE_MASK_LSB);

	params[VPUI_ROI_CFG] =
		(rois_out->work_mode << VPUI_ROI_CFG_MODE_LSB) |
		(rois_out->first_min_max << VPUI_ROI_CFG_MIN_MAX_LSB) |
		(rois_out->thresh_lo_temp << VPUI_ROI_CFG_LOW_THR_LSB);

	return VPUI_ROI_PARAMS;
}

/* set CROP parameters. returns the number of parameters */
static __u32 VPUL_OP_CROP_PARAMS_ASSIGMENT(const struct vpul_pu *pu,
					   __u16 *params,
	__u32 mprb_gr_ofs,
 __u32 special_param ){
	const union vpul_pu_parameters *pu_params = &pu->params;
	const struct vpul_pu_crop *crop = &(pu_params->crop);

	if (params == NULL)
		return VPUI_CROP_PARAMS;

	params[VPUI_CROP_CFG] = (crop->work_mode << VPUI_CROP_CFG_MODE_LSB) |
			(pu->out_size_idx << VPUI_CROP_CFG_OUT_SIZES_IND_LSB);

	params[VPUI_CROP_ROI_START_X] = crop->roi_startx;
	params[VPUI_CROP_ROI_START_Y] = crop->roi_starty;

	params[VPUI_CROP_PAD_VALUE] = crop->pad_value;
	params[VPUI_CROP_MASK_IN_VALUE] = crop->mask_val_in;
	params[VPUI_CROP_MASK_OUT_VALUE] = crop->mask_val_out;

	return VPUI_CROP_PARAMS;
}

/* set MDE parameters. returns the number of parameters */
static __u32 VPUL_OP_MDE_PARAMS_ASSIGMENT(const struct vpul_pu *pu,
					  __u16 *params,
	__u32 mprb_gr_ofs,
 __u32 special_param ){
	const union vpul_pu_parameters *pu_params = &pu->params;
	const struct vpul_pu_mde *mde = &(pu_params->mde);

	if (params == NULL)
		return VPUI_MDE_PARAMS;

	params[VPUI_MDE_CFG] = (mde->work_mode << VPUI_MDE_CFG_MODE_LSB) |
		(mde->result_shift << VPUI_MDE_CFG_RESULT_SH_LSB) |
		(mde->use_thresh << VPUI_MDE_CFG_USE_THR_LSB) |
		(mde->calc_quantized_angle << VPUI_MDE_CFG_QUANT_ANGLE_LSB);

	params[VPUI_MDE_STREAM] =
		(mde->bits_in << VPUI_MDE_STREAM_BITS_IN_LSB) |
		(mde->bits_out << VPUI_MDE_STREAM_BITS_OUT_LSB) |
		(mde->signed_in << VPUI_MDE_STREAM_SIGN_IN_LSB) |
		(mde->signed_out << VPUI_MDE_STREAM_SIGN_OUT_LSB) |
		(mde->output_enable << VPUI_MDE_STREAM_OUT_EN_LSB);


	params[VPUI_MDE_EIG_COEFF_LSB] = mde->eig_coeff & 0xFFFF;
	params[VPUI_MDE_EIG_COEFF_MSB] = mde->eig_coeff >> 16;
	params[VPUI_MDE_THRESHOLD] = mde->thresh;

	return VPUI_MDE_PARAMS;
}

/* set NMS parameters. returns the number of parameters */
static __u32 VPUL_OP_NMS_PARAMS_ASSIGMENT(const struct vpul_pu *pu,
					  __u16 *params,
	__u32 mprb_gr_ofs,
 __u32 special_param ){
	const union vpul_pu_parameters *pu_params = &pu->params;
	const struct vpul_pu_nms *nms = &(pu_params->nms);
	__u32 mprb_fst_ofs;

	if (params == NULL)
		return VPUI_NMS_PARAMS;

	mprb_fst_ofs = get_offset_to_1st_ram_port_used(pu);

	params[VPUI_NMS_STREAM] =
		(nms->bits_in << VPUI_NMS_STREAM_BITS_IN_LSB) |
		(nms->bits_out << VPUI_NMS_STREAM_BITS_OUT_LSB) |
		(nms->signed_in << VPUI_NMS_STREAM_SIGN_IN_LSB) |
		(nms->support << VPUI_NMS_STREAM_WINDOW_LSB) |
		(nms->org_val_out << VPUI_NMS_STREAM_OUT_ORG_VAL_LSB) |
		(nms->trunc_out << VPUI_NMS_STREAM_TRUNC_OUT_LSB);

	params[VPUI_NMS_CFG] =
	(nms->census_mode << VPUI_NMS_CFG_IS_CENSUS_MODE_LSB) |
	(nms->work_mode << VPUI_NMS_CFG_MASK_COMP_LSB) |
	(nms->keep_equals << VPUI_NMS_CFG_KEEP_EQUALS_LSB) |
	(nms->directional_nms << VPUI_NMS_CFG_DIRECTIONAL_LSB) |
	(nms->census_mode << VPUI_NMS_CFG_CENSUS_MODE_LSB) |
	(nms->add_orig_pixel << VPUI_NMS_CFG_CENSUS_OUT_IMAGE_LSB) |
	(nms->border_fill << VPUI_NMS_CFG_BORDER_FILL_LSB) |
	(0 << VPUI_NMS_CFG_BORDER_IGN_TILE_LSB);

	params[VPUI_NMS_MPRB] = (pu->mprb_type << VPUI_FILT_MPRB_TYPE_LSB) |
		(mprb_fst_ofs << VPUI_FILT_MPRB_FST_OFS_LSB) |
		(mprb_gr_ofs << VPUI_FILT_MPRB_GR_OFS_LSB) |
		(pu->n_mprbs << VPUI_FILT_MPRB_NUM_LSB);

	if (nms->census_mode) {
		params[VPUI_NMS_THRESH_LSB] =
			nms->cens_thres_0 | (nms->cens_thres_1 << 4) |
			(nms->cens_thres_2 << 8) | (nms->cens_thres_3 << 12);
		params[VPUI_NMS_THRESH_MSB] =
			nms->cens_thres_4 | (nms->cens_thres_5 << 4) |
			(nms->cens_thres_6 << 8) | (nms->cens_thres_7 << 12);
	} else {
		params[VPUI_NMS_THRESH_LSB] = nms->thresh & 0xFFFF;
		params[VPUI_NMS_THRESH_MSB] = nms->thresh >> 16;
	}

	params[VPUI_NMS_BORDER_CONST] = nms->border_fill_constant;

	params[VPUI_NMS_STRICT_COMP_MASK] = nms->strict_comparison_mask;

	return VPUI_NMS_PARAMS;
}

/* set CCM parameters. returns the number of parameters */
static __u32 VPUL_OP_CCM_PARAMS_ASSIGMENT(const struct vpul_pu *pu,
					  __u16 *params,
	__u32 mprb_gr_ofs,
 __u32 special_param ){
	const union vpul_pu_parameters *pu_params = &pu->params;
	const struct vpul_pu_ccm *ccm = &(pu_params->ccm);

	if (params == NULL)
		return VPUI_CCM_PARAMS;

	params[VPUI_CCM_CFG] = (ccm->signed_in << VPUI_CCM_CFG_SIGN_IN_LSB) |
		(ccm->signed_out << VPUI_CCM_CFG_SIGN_OUT_LSB) |
		(ccm->input_enable << VPUI_CCM_CFG_INPUT_EN_MASK_LSB) |
		(ccm->output_enable << VPUI_CCM_CFG_OUTPUT_EN_MASK_LSB) |
		(ccm->coefficient_shift << VPUI_CCM_CFG_RIGHT_SH_LSB);

	params[VPUI_CCM_COEFF_0] = ccm->coefficient_0;
	params[VPUI_CCM_COEFF_1] = ccm->coefficient_1;
	params[VPUI_CCM_COEFF_2] = ccm->coefficient_2;
	params[VPUI_CCM_COEFF_3] = ccm->coefficient_3;
	params[VPUI_CCM_COEFF_4] = ccm->coefficient_4;
	params[VPUI_CCM_COEFF_5] = ccm->coefficient_5;
	params[VPUI_CCM_COEFF_6] = ccm->coefficient_6;
	params[VPUI_CCM_COEFF_7] = ccm->coefficient_7;
	params[VPUI_CCM_COEFF_8] = ccm->coefficient_8;

	params[VPUI_CCM_OFFSET_0] = ccm->offset_0;
	params[VPUI_CCM_OFFSET_1] = ccm->offset_1;
	params[VPUI_CCM_OFFSET_2] = ccm->offset_2;

	return VPUI_CCM_PARAMS;
}

/**
 * set separable filter parameters. returns the number of parameters
 * applies to both 5x5 and 7x7 separable filters
 */
static __u32 VPUL_OP_SEP_FLT_PARAMS_ASSIGMENT(const struct vpul_pu *pu,
					      __u16 *params,
	__u32 mprb_gr_ofs,
 __u32 special_param ){
	const union vpul_pu_parameters *pu_params = &pu->params;
	const struct vpul_pu_slf *slf = &(pu_params->slf);
	__u32 number_of_params = VPUI_FILT_PARAMS;
	__u32 mprb_fst_ofs;

	if (slf->work_mode == VPUH_SEP_FILT_WORK_MODE_MAX_POOL)
		number_of_params += VPUI_SFILT5_MAXP_PARMAS;

	if (params == NULL)
		return number_of_params;

	mprb_fst_ofs = get_offset_to_1st_ram_port_used(pu);


	params[VPUI_FILT_STREAM] =
		(slf->bits_in << VPUI_FILT_STREAM_BITS_IN_LSB) |
		(slf->bits_out << VPUI_FILT_STREAM_BITS_OUT_LSB) |
		(slf->signed_in << VPUI_FILT_STREAM_SIGN_IN_LSB) |
		(slf->signed_out << VPUI_FILT_STREAM_SIGN_OUT_LSB) |
		(slf->border_fill << VPUI_FILT_STREAM_BORDER_FILL_LSB) |
		(0 << VPUI_FILT_STREAM_BORDER_IGN_TILE_LSB) |
		(slf->do_rounding << VPUI_FILT_STREAM_ROUND_LSB);

	params[VPUI_FILT_CFG] =
		(slf->filter_size_mode << VPUI_FILT_CFG_FILTER_SIZE_LSB) |
		(slf->out_enable_1 << VPUI_FILT_CFG_OUTPUT_INPUT_LSB) |

		(slf->work_mode << VPUI_SEP_FILT_CFG_WORK_MODE_LSB) |
		(slf->invert_columns << VPUI_SEP_FILT_CFG_FLIP_COLS_LSB) |
		(slf->horizontal_only << VPUI_SEP_FILT_CFG_HOR_ONLY_LSB) |
		(slf->upsample_mode << VPUI_SEP_FILT_CFG_ADD_ZEROS_LSB) |
		(slf->downsample_cols << VPUI_SEP_FILT_CFG_DSAMPLE_COLS_LSB) |
		(slf->downsample_rows << VPUI_SEP_FILT_CFG_DSAMPLE_ROWS_LSB) |
		(slf->sampling_offset_x
			<< VPUI_SEP_FILT_CFG_SAMP_OFS_COLS_LSB) |
		(slf->sampling_offset_y
			<< VPUI_SEP_FILT_CFG_SAMP_OFS_ROWS_LSB);

	params[VPUI_FILT_COEFF_IND] = slf->coefficient_index;

	params[VPUI_FILT_MPRB] = (pu->mprb_type << VPUI_FILT_MPRB_TYPE_LSB) |
		(mprb_fst_ofs << VPUI_FILT_MPRB_FST_OFS_LSB) |
		(mprb_gr_ofs << VPUI_FILT_MPRB_GR_OFS_LSB) |
		(pu->n_mprbs << VPUI_FILT_MPRB_NUM_LSB);

	params[VPUI_FILT_BORDER_CONST] = slf->border_fill_constant;

	if (slf->work_mode == VPUH_SEP_FILT_WORK_MODE_MAX_POOL) {
		params[VPUI_FILT_PARAMS + VPUI_SFILT5_MAXP_BITS] =
		(slf->sepfregs_convert_16f_to_32f <<
		VPUI_SFILT5_MAXP_32F_OUTPUT_LSB) |
		(slf->sepfregs_convert_input_2scomp_to_sm <<
		VPUI_SFILT5_MAXP_2COMP_INPUT_LSB) |
		(slf->sepfregs_convert_output_sm_to_2scomp <<
		VPUI_SFILT5_MAXP_2COMP_OUTPUT_LSB) |
		(slf->sepfregs_stride_value <<
		VPUI_SFILT5_MAXP_STRIDE_SIZE_LSB) |
		(slf->sepfregs_stride_offset_width <<
		VPUI_SFILT5_MAXP_STRIDE_X_OFS_LSB) |
		(slf->sepfregs_stride_offset_height <<
		VPUI_SFILT5_MAXP_STRIDE_Y_OFS_LSB);

		params[VPUI_FILT_PARAMS + VPUI_SFILT5_MAXP_NUM_SLICES] =
				slf->maxp_num_slices;

		params[VPUI_FILT_PARAMS + VPUI_SFILT5_MAXP_SIZES] =
			(slf->maxp_sizes_filt_hor << VPUI_SFILT5_MAXP_SIZES_FILT_HOR_LSB) |
			(slf->maxp_sizes_filt_ver << VPUI_SFILT5_MAXP_SIZES_FILT_VER_LSB) |
			(pu->out_size_idx << VPUI_SFILT5_MAXP_SIZES_OUT_IND_LSB) |
			(slf->max_pooling_size_spoof_index << VPUI_SFILT5_MAXP_SIZES_SPF_IND_LSB);
	}
	return number_of_params;
}

/* set general filter parameters. returns the number of parameters */
static __u32 VPUL_OP_GEN_FLT_PARAMS_ASSIGMENT(const struct vpul_pu *pu,
					      __u16 *params,
	__u32 mprb_gr_ofs,
 __u32 special_param ){
	const union vpul_pu_parameters *pu_params = &pu->params;
	const struct vpul_pu_glf *glf = &(pu_params->glf);
	__u32 mprb_fst_ofs;

	if (params == NULL)
		return VPUI_FILT_PARAMS;

	mprb_fst_ofs = get_offset_to_1st_ram_port_used(pu);

	params[VPUI_FILT_STREAM] =
		(glf->bits_in << VPUI_FILT_STREAM_BITS_IN_LSB) |
		(glf->bits_out << VPUI_FILT_STREAM_BITS_OUT_LSB) |
		(glf->signed_in << VPUI_FILT_STREAM_SIGN_IN_LSB) |
		(glf->border_fill << VPUI_FILT_STREAM_BORDER_FILL_LSB) |
		(0 << VPUI_FILT_STREAM_BORDER_IGN_TILE_LSB) |
		(glf->signed_out << VPUI_FILT_STREAM_SIGN_OUT_LSB) |
		(glf->do_rounding << VPUI_FILT_STREAM_ROUND_LSB);

	params[VPUI_FILT_CFG] =
		(glf->filter_size_mode << VPUI_FILT_CFG_FILTER_SIZE_LSB) |
		(glf->out_enable_2 << VPUI_FILT_CFG_OUTPUT_INPUT_LSB) |
		(glf->sad_mode << VPUI_GEN_FILT_CFG_IS_SAD_LSB) |
		(glf->two_outputs << VPUI_GEN_FILT_CFG_2_FILTERS_LSB) |
		(glf->coeffs_from_dma << VPUI_GEN_FILT_CFG_COEFFS_DMA_LSB);

	params[VPUI_FILT_COEFF_IND] = glf->coefficient_index;

	params[VPUI_FILT_MPRB] = (pu->mprb_type << VPUI_FILT_MPRB_TYPE_LSB) |
		(mprb_fst_ofs << VPUI_FILT_MPRB_FST_OFS_LSB) |
		(mprb_gr_ofs << VPUI_FILT_MPRB_GR_OFS_LSB) |
		(pu->n_mprbs << VPUI_FILT_MPRB_NUM_LSB);

	params[VPUI_FILT_BORDER_CONST] = glf->border_fill_constant;

	return VPUI_FILT_PARAMS;
}

static __u32 VPUL_OP_NLF_FLT_PARAMS_ASSIGMENT(const struct vpul_pu *pu,
	__u16 *params,
	__u32 mprb_gr_ofs,
	__u32 special_param )
{

	const union vpul_pu_parameters *pu_params = &pu->params;
	const struct vpul_pu_nlf *nlf = &(pu_params->nlf);
	__u32 mprb_fst_ofs;
	__u32 return_size = VPUI_NLF_ALL_MODES_PARAMS + VPUI_NLF_NLF_MODE_PARAMS;

	if (params != NULL)
	{
		mprb_fst_ofs = get_offset_to_1st_ram_port_used(pu);

		params[VPUI_NLF_CFG] =
			(nlf->filter_mode          << VPUI_NLF_CFG_MODE_LSB
			| nlf->fast_score_direction<< VPUI_NLF_CFG_FAST_MODE_LSB
			| nlf->bits_in             << VPUI_NLF_CFG_BITS_IN_LSB
			| nlf->signed_in           << VPUI_NLF_CFG_SIGN_IN_LSB
			| nlf->census_mode         << VPUI_NLF_CFG_CENSUS_MODE_LSB
			| nlf->add_orig_pixel    << VPUI_NLF_CFG_CENSUS_OUT_IMAGE_LSB
			| nlf->border_fill         << VPUI_NLF_CFG_BORDER_FILL_LSB
			| nlf->border_tile         << VPUI_NLF_CFG_BORDER_IGN_TILE_LSB);


		params[VPUI_NLF_MPRB] =
			(pu->mprb_type << VPUI_FILT_MPRB_TYPE_LSB)
			| (mprb_fst_ofs << VPUI_FILT_MPRB_FST_OFS_LSB)
			| (mprb_gr_ofs << VPUI_FILT_MPRB_GR_OFS_LSB)
			| (pu->n_mprbs << VPUI_FILT_MPRB_NUM_LSB);

		params[VPUI_NLF_BORDER_CONST] = nlf->border_fill_constant;

		if(nlf->filter_mode == VPUH_NLF_MODE_CENSUS){
			params[VPUI_NLF_ALL_MODES_PARAMS + VPUI_NLF_CENSUS_THR_0_3] =
				((nlf->cens_thres_0&0xF)<<0)|
				((nlf->cens_thres_1&0xF)<<4)|
				((nlf->cens_thres_2&0xF)<<8)|
				((nlf->cens_thres_3&0xF)<<12);
			params[VPUI_NLF_ALL_MODES_PARAMS + VPUI_NLF_CENSUS_THR_4_7] =
				((nlf->cens_thres_4&0xF)<<0)|
				((nlf->cens_thres_5&0xF)<<4)|
				((nlf->cens_thres_6&0xF)<<8)|
				((nlf->cens_thres_7&0xF)<<12);

		}
		else{
			params[VPUI_NLF_ALL_MODES_PARAMS + VPUI_NLF_NLF_NEIGHBORS] = nlf->neighbors_mask;
		}
	}

	if (nlf->filter_mode == VPUH_NLF_MODE_CENSUS)
	{
		return_size = VPUI_NLF_ALL_MODES_PARAMS + VPUI_NLF_CENSUS_MODE_PARAMS;
	}
	else
	{
		return_size = VPUI_NLF_ALL_MODES_PARAMS + VPUI_NLF_NLF_MODE_PARAMS;
	}

	return return_size;
}

static __u32 VPUL_OP_MAP_2_LST_PARAMS_ASSIGMENT(const struct vpul_pu *pu,
	__u16 *params,
	__u32 mprb_gr_ofs,
 __u32 special_param ){

	const union vpul_pu_parameters *pu_params = &pu->params;
	const struct vpul_pu_map2list *m2lst = &(pu_params->map2list);

	if (params == NULL)
		return VPUI_M2LI_PARAMS;

	params[VPUI_M2LI_CFG] =
		 ((m2lst->bits_in        << VPUI_M2LI_CFG_BITS_IN_LSB)
		| (m2lst->signed_in      << VPUI_M2LI_CFG_SIGN_IN_LSB)
		| (m2lst->value_in       << VPUI_M2LI_CFG_VAL_IN_LIST_LSB)
		| (m2lst->enable_out_map << VPUI_M2LI_CFG_EN_MAP_OUT_LSB)
		| (m2lst->enable_out_lst << VPUI_M2LI_CFG_EN_LIST_OUT_LSB)
		| (m2lst->inout_indx     << VPUI_M2LI_CFG_INOUT_INDEX_LSB));


	params[VPUI_M2LI_CROP_BEF_DOWN_SH] = 0;
	params[VPUI_M2LI_CROP_AFT_UP_SH] = 0;
	params[VPUI_M2LI_COORDS_FINAL_WIDTH] = 0;

	params[VPUI_M2LI_THR_LOW_LSB]  = m2lst->threshold_low & 0xFFFF;
	params[VPUI_M2LI_THR_LOW_MSB] =  (m2lst->threshold_low >> 16)&0xFFFF;

	params[VPUI_M2LI_THR_HIGH_LSB]  = m2lst->threshold_high & 0xFFFF;
	params[VPUI_M2LI_THR_HIGH_MSB] =  (m2lst->threshold_high >> 16)&0xFFFF;

	return VPUI_M2LI_PARAMS;
}

static __u32 VPUL_OP_FIFO_PARAMS_ASSIGMENT(const struct vpul_pu *pu,
						__u16 *params,
						__u32 mprb_gr_ofs,
						__u32 special_param ){
	const union vpul_pu_parameters *pu_params = &pu->params;
	const struct vpul_pu_fifo *fifo = &(pu_params->fifo);

	if (params == NULL)
		return VPUI_FIFO_PARAMS;

	if (pu->n_mprbs)
		params[VPUI_FIFO_CFG] = (1 << VPUI_FIFO_CFG_MPRB_EN_LSB) |
			(pu->mprb_type << VPUI_FIFO_CFG_MPRB_TYPE_LSB) |
			(mprb_gr_ofs << VPUI_FIFO_CFG_MPRB_GR_OFS_LSB) |
			(fifo->bits_in << VPUI_FIFO_CFG_BITS_LSB);
	else
		params[VPUI_FIFO_CFG] =
				fifo->bits_in << VPUI_FIFO_CFG_BITS_LSB;

	return VPUI_FIFO_PARAMS;
}

#define VPUI_LUT_CFG_SIGN_IN_MASK VPUI_LUT_CFG_SIGN_MASK
#define VPUI_LUT_CFG_SIGN_OUT_MASK VPUI_LUT_CFG_SIGN_MASK

static __u32 VPUL_OP_CNN_PARAMS_ASSIGMENT(const struct vpul_pu *pu,
						__u16 *params,
						__u32 mprb_gr_ofs,
						__u32 special_param ){
	__u32 i;
	__u32 num_of_large_mprbs = 0;
	const union vpul_pu_parameters *pu_params = &pu->params;
	const struct vpul_pu_cnn *cnn = &(pu_params->cnn);

	if (params != NULL) {
		params[VPUI_CNN_INFO] = (cnn->is_fetch_mode << VPUI_CNN_INFO_IS_FETCH_LSB) |
					(cnn->is_bypass << VPUI_CNN_INFO_BYPASS_LSB);
		if ((!cnn->is_fetch_mode) && (pu->n_out_connect)) {
			params[VPUI_CNN_INFO] |=
					(pu->out_size_idx << VPUI_CNN_INFO_SP_SIZES_IND_LSB);
		}

		for (i = 0; i < pu->n_mprbs; i++) {
			if ((pu->mprbs[i] < VPU_HW_NUM_LARGE_MPRBS) &&
				(pu->mprbs[i] != NO_MPRB_CONNECTED))
				num_of_large_mprbs++;
		}
		params[VPUI_CNN_MPRBS] = (mprb_gr_ofs << VPUI_CNN_MPRBS_GR_OFS_LSB) |
					(num_of_large_mprbs << VPUI_CNN_DATA_MAX_MPRBS_LSB);
	}
	return VPUI_CNN_PARAMS;
}

static __u32 VPUL_OP_LUT_PARAMS_ASSIGMENT(const struct vpul_pu *pu,
	__u16 *params,
	__u32 mprb_gr_ofs,

__u32 special_param )
{
	const union vpul_pu_parameters *pu_params = &pu->params;
	const struct vpul_pu_lut *lut = &(pu_params->lut);

	if (params != NULL){
		params[VPUI_LUT_CFG] =
			PREP_FLD(VPUI_LUT_CFG_INTERPOLATION,lut->interpolation_mode)|
			PREP_FLD(VPUI_LUT_CFG_SIZE,lut->lut_size)|
			PREP_FLD(VPUI_LUT_CFG_SIGN_IN,lut->signed_in0)|
			PREP_FLD(VPUI_LUT_CFG_SIGN_OUT,lut->signed_out0)|
			PREP_FLD(VPUI_LUT_CFG_MPRB_GR_OFS,mprb_gr_ofs)|
			PREP_FLD(VPUI_LUT_CFG_BYPASS,0);
		params[VPUI_LUT_OFFSET] = lut->offset;
		params[VPUI_LUT_BIN_SIZE] = lut->binsize;
		params[VPUI_LUT_INV_BIN_SIZE]  = lut->inverse_binsize;
	}
	return VPUI_LUT_PARAMS;
}

static __u32 VPUL_OP_FLAMORB_PARAMS_ASSIGMENT(const struct vpul_pu *pu,
	__u16 *params,
	__u32 mprb_gr_ofs,
	__u32 special_param )
{
	const union vpul_pu_parameters *pu_params = &pu->params;
	const struct vpul_pu_flam_orb *flamorb = &(pu_params->flam_orb);

	if (params != NULL){
		params[VPUI_FLAMORB_CFG] =
			PREP_FLD(VPUI_FLAMORB_CFG_OUT,((~flamorb->work_mode))&1)|
			PREP_FLD(VPUI_FLAMORB_CFG_BITS, flamorb->bits_in )|
			PREP_FLD(VPUI_FLAMORB_CFG_PAD_WITH_CONST, flamorb->use_const_val)|
			PREP_FLD(VPUI_FLAMORB_CFG_PAD_VALUE, flamorb->const_val)|
			PREP_FLD(VPUI_FLAMORB_CFG_MPRB_TYPE, pu->mprb_type);

		params[VPUI_FLAMORB_PAIRS_PER_GROUPS] = flamorb->num_coord_pairs;

		params[VPUI_FLAMORB_IND] =
			  (((flamorb->roi_ind)    &
				VPUI_FLAMORB_IND_IN_ROI_MASK)
					<< VPUI_FLAMORB_IND_IN_ROI_LSB)
			| (((flamorb->scaler_ind) &
				VPUI_FLAMORB_IND_SCALER_P1_MASK)
					<< VPUI_FLAMORB_IND_SCALER_P1_LSB)
			| (((flamorb->full_roi)   &
				VPUI_FLAMORB_IND_FULL_ROI_MASK)
					<< VPUI_FLAMORB_IND_FULL_ROI_LSB)
			;


		params[VPUI_FLAMORB_MPRB_GR_OFS] = mprb_gr_ofs;
	}
	return VPUI_FLAMORB_PARAMS;
}



static __u32 VPUL_OP_FAST_DEPTH_PARAMS_ASSIGMENT(const struct vpul_pu *pu,
	__u16 *params,
	__u32 mprb_gr_ofs,
	__u32 special_param )
{
	const union vpul_pu_parameters *pu_params = &pu->params;
	const struct vpul_pu_fast_depth *depth = &(pu_params->depth);

	struct VPUI_DepthDynThr  *p_dep_dyn_str = NULL;

	if (params != NULL){
		params[VPUI_DEPTH_IND] =
			(special_param << VPUI_DEPTH_IND_FIXED_LSB) |
			(0 << VPUI_DEPTH_IND_DYN_P1_LSB)			|
			(0 << VPUI_DEPTH_IND_BYPASS_LSB);

		params[VPUI_DEPTH_MPRB] =
				(mprb_gr_ofs << VPUI_DEPTH_MPRB_GR_OFS_LSB) |
				(pu->mprb_type * 0x7f << VPUI_DEPTH_MPRB_TYPE_MASK_LSB);

		params[VPUI_DEPTH_MIN_MAX_DISP] =
			(depth->max_disparity << VPUI_DEPTH_MIN_MAX_DISP_MAX_LSB) |
			(depth->min_disparity << VPUI_DEPTH_MIN_MAX_DISP_MIN_LSB);

		p_dep_dyn_str = (struct VPUI_DepthDynThr*)(&params[VPUI_DEPTH_PARAMS]);

		p_dep_dyn_str->CostMaxGoodMatch			  =  depth->dpu_cost_thresh_max_good;
		p_dep_dyn_str->GrayDiffThrIsEqualPixels   =  depth->dcu_gray_diff_thresh_is_equal_pixels;
		p_dep_dyn_str->GrayDiffThrIsNearPixels	  =  depth->dcu_gray_diff_thresh_is_near_pixels;
		p_dep_dyn_str->DcuDynCfgBits			  =  depth->dcu_patch_size_mode |
													depth->dcu_min_pixels_for_cost << DCU_DCFG_MIN_PIXELS_COST_LSB;
		p_dep_dyn_str->RefDsprConstantCostValue	  =  depth->dpu_ref_dspr_constant_cost;
		p_dep_dyn_str->RefDsprConstantCostFactor  =  depth->dpu_ref_dspr_cost_factor;
		p_dep_dyn_str->SuperbCandidateEn		  =  depth->superb_candidate_en;
		p_dep_dyn_str->GrayDiffMaxIsLikeSuperb	  =  depth->dcu_gray_diff_thresh_is_like_superb;
	}

	return (VPUI_DEPTH_PARAMS + VPUI_DEPTH_DYN_PARAMS);
}

static __u32 VPUL_OP_IN_PAINT_PARAMS_ASSIGMENT(const struct vpul_pu *pu,
	__u16 *params,
	__u32 mprb_gr_ofs,
	__u32 special_param )
{
	const union vpul_pu_parameters *pu_params = &pu->params;
	const struct vpul_pu_in_paint *inpaint = &(pu_params->in_paint);

	struct VPUI_InPaintDynThr  *p_inpaint_dyn_str = NULL;

	if (params != NULL){
		params[VPUI_INPAINT_IND_MPRB] =
			(special_param << VPUI_INPAINT_IND_MPRB_FIXED_LSB) |
			(0 << VPUI_INPAINT_IND_MPRB_DYN_P1_LSB)			   |
			(mprb_gr_ofs << VPUI_INPAINT_IND_MPRB_GR_OFS_LSB)  |
			(inpaint->bypass << VPUI_INPAINT_IND_BYPASS_LSB);

		params[VPUI_INPAINT_DYN] =
			(inpaint->output_reliable_mask_en	<< VPUI_INP_DYN_OUTPUT_RELIABLE_MASK_EN_LSB) |
			(inpaint->output_stat_mask_en		<< VPUI_INP_DYN_OUTPUT_STAT_MASK_EN_LSB)	 |
			(inpaint->x_last_logic_right_margin << VPUI_INP_DYN_X_LAST_LOGIC_RIGHT_MARGIN_LSB);

		params[VPUI_INPAINT_MAX_ANCHOR_GRAD_SH] = inpaint->dspr_max_anchors_grad_shift;
		params[VPUI_INPAINT_BG_GRAY_VAL] =  inpaint->background_gray_val;
		params[VPUI_INPAINT_ALLOW_BG_EN] = 0;
		params[VPUI_INPAINT_BG_DSPR_VAL] = inpaint->background_dspr_val;

		p_inpaint_dyn_str = (struct VPUI_InPaintDynThr*)(&params[VPUI_INPAINT_PARAMS]);

		p_inpaint_dyn_str->InpDynCfgBits =
			(inpaint->background_en				<< INP_DCFG_BACKGROUND_EN_LSB)   |
			(inpaint->strong_anti_leak_en		<< INP_DCFG_STR_ANTI_LEAK_EN_LSB)|
			(inpaint->background_anti_leak_en	<< INP_DCFG_BG_ANTI_LEAK_EN_LSB);
		p_inpaint_dyn_str->GrayMaxSimilarity			 = 	inpaint->gray_max_similarity;
		p_inpaint_dyn_str->GrayMaxHighSimilarity		 = 	inpaint->gray_max_high_similarity;
		p_inpaint_dyn_str->DsprMaxSimilarity			 = 	inpaint->dspr_max_similarity;
		p_inpaint_dyn_str->DsprMaxAnchorsSimilarity		 = 	inpaint->dspr_max_anchors_similarity;
		p_inpaint_dyn_str->CostMinFillHoles				 = 	inpaint->cost_min_fill_holes;
		p_inpaint_dyn_str->CostMaxReliable				 = 	inpaint->cost_max_reliable;
		p_inpaint_dyn_str->CostMaxForAnchor				 = 	inpaint->cost_max_for_anchor;
		p_inpaint_dyn_str->BackgroundGrayMaxSimilarity	 = 	inpaint->background_gray_max_similarity;
		p_inpaint_dyn_str->DummyAlign					 = 	0;
		p_inpaint_dyn_str->HiddenLogicTrim[0]			 = 	inpaint->hidden_logic_trim_0_0;
		p_inpaint_dyn_str->HiddenLogicTrim[1]			 = 	inpaint->hidden_logic_trim_0_1;
	}

	return (VPUI_INPAINT_PARAMS + VPUI_INPAINT_DYN_PARAMS);
}

static __u32 VPUL_OP_DISPEQ_PARAMS_ASSIGMENT(const struct vpul_pu *pu,
	__u16 *params,
	__u32 mprb_gr_ofs,
	__u32 special_param )
{

	const union vpul_pu_parameters *pu_params = &pu->params;
	const struct vpul_pu_dispeq *dispeq = &(pu_params->disparity);

	if (params != NULL){
		params[VPUI_DISPEQ_IND_MPRB] =
			(special_param  << VPUI_DISPEQ_IND_MPRB_FIXED_LSB)	 |
			(mprb_gr_ofs    << VPUI_DISPEQ_IND_MPRB_GR_OFS_LSB)  |
			(dispeq->bypass << VPUI_DISPEQ_IND_BYPASS_LSB);

		params[VPUI_DISPEQ_DYN] =
			(dispeq->dspr_small_to_large_tresh	<<
				VPUI_DISPEQ_DYN_SM_TO_LR_THR_LSB)	|
			(dispeq->dspr_min_diff_for_valid	<<
			VPUI_DISPEQ_DYN_MIN_DIFF_VALID_LSB);

	}

	return VPUI_DISPEQ_PARAMS;
}

static __u32 VPUL_OP_HIST_PARAMS_ASSIGMENT(const struct vpul_pu *pu,
	__u16 *params,
	__u32 mprb_gr_ofs,
	__u32 special_param )
{
	const union vpul_pu_parameters *pu_params = &pu->params;
	const struct vpul_pu_histogram *hist = &(pu_params->histogram);

	if (params != NULL){

		params[VPUI_HIST_CFG] =
			PREP_FLD(VPUI_HIST_CFG_SIGN_IN, hist->signed_in0)|
			PREP_FLD(VPUI_HIST_CFG_ROUND,hist->round_index)|
			PREP_FLD(VPUI_HIST_CFG_USE_WEIGHTS,hist->dual_histogram)|
			PREP_FLD(VPUI_HIST_CFG_MPRB_GR_OFS, mprb_gr_ofs)|
			PREP_FLD(VPUI_HIST_CFG_HIST_PARAMS_IND_P1, special_param+1)|
			PREP_FLD(VPUI_LUT_CFG_MPRB_GR_OFS,mprb_gr_ofs);
	}

	return VPUI_HIST_PARAMS;
}

static void hist_special_param_set(const struct vpul_pu_histogram *hist, struct VPUI_HistParams *hist_params){

	if (hist_params != NULL){
		hist_params->InvBinSizeM1 = hist->inverse_binsize>0?hist->inverse_binsize-1:0;
		hist_params->MaxValue = hist->max_val;
		hist_params->Offset = hist->offset;
	}
	return;
}


static void depth_special_params_set
(
	const struct vpul_pu_fast_depth *fast,
	struct VPUI_DisparityFixedThr *disp_params
)
{
	if (disp_params != NULL){
		disp_params->GrayDiffThrIsNearToCenter		=	fast->dcu_gray_diff_thresh_is_near_to_center;
		disp_params->GrayDiffThrDoAggregation		=	fast->dcu_gray_diff_thresh_do_aggregation;
		disp_params->GrayDiffThrIsEqualToCenter		=	fast->dcu_gray_diff_thresh_is_equal_to_center;
		disp_params->AggregatedNeighborSelection	=	fast->dcu_aggregated_neighbor_selection;
		disp_params->DcuFixCfgBits	=
			fast->dcu_rl_borders_trim << DCU_FCFG_RL_BORDERS_TRIM_LSB |
			fast->dcu_info_thresh_is_informative << DCU_FCFG_INFO_THR_IS_INFRM_LSB |
			fast->dcu_min_pixels_for_cost << DCU_DCFG_MIN_PIXELS_COST_LSB;
		disp_params->CostFractionBits				=	fast->dcu_cost_fraction_bits;
		disp_params->CostCExpWeights[0]				=	fast->dcu_cost_c_exp_weights_table_0_0;
		disp_params->CostCExpWeights[1]				=	fast->dcu_cost_c_exp_weights_table_0_1;
		disp_params->CostCExpWeights[2]				=	fast->dcu_cost_c_exp_weights_table_0_2;
		disp_params->CostCExpWeights[3]				=	fast->dcu_cost_c_exp_weights_table_0_3;
		disp_params->CostCExpWeights[4]				=	fast->dcu_cost_c_exp_weights_table_0_4;
		disp_params->CostCExpWeights[5]				=	fast->dcu_cost_c_exp_weights_table_0_5;
		disp_params->CostCExpWeights[6]				=	fast->dcu_cost_c_exp_weights_table_0_6;
		disp_params->CostCExpWeights[7]				=	fast->dcu_cost_c_exp_weights_table_0_7;
		disp_params->CostMaxGoodForCount			=	fast->dpu_cost_thresh_count_good_cand;
		disp_params->DsprDiffThreshIsEqual			=	fast->dpu_ref_dspr_max_is_equal;
		disp_params->RefCostThreshUseRefDsp			=	fast->dpu_ref_cost_thresh_use_ref_dsp;
		disp_params->DsprDiffMaxNeighborIsEqual		=	fast->dpu_dspr_diff_max_neighbor_is_equal;
		disp_params->DpuCfgBits						=
					(fast->dpu_use_ref_en					<<
							DPU_CFG_USE_REF_EN_LSB)				|
					(fast->dpu_ref_candidate_en				<<
							DPU_CFG_REF_CANDIDATE_EN_LSB)		|
					(fast->dpu_ref_dspr_prioritization_en	<<
								DPU_CFG_PRIORITIZATION_EN_LSB)	|
					(fast->dpu_ref_dspr_up_shift			<<
						DPU_CFG_REF_DSPR_UP_SH_LSB)				|
					(fast->pre_output_dspr_up_shift			<<
						DPU_CFG_PRE_OUTPUT_UP_SH_LSB);
		disp_params->SuperbCfgBits					=
								(fast->superb_dspr_cost_factor			  <<
									SUPERB_CFG_COST_FACTOR_LSB)					|
								(fast->prioritize_superb_on_non_info_only <<
									SUPERB_CFG_PR_NON_INFO_ONLY_LSB)			|
								(fast->dpu_superb_dspr_prioritization_en   <<
								SUPERB_CFG_DSPR_PR_EN_LSB);
		disp_params->CostMaxGoodForSuperb			=	fast->cost_thresh_good_for_superb;
		disp_params->SuperbDsprConstantCostValue	=	fast->superb_dspr_constant_cost;
		disp_params->LfsrSeeds[0]					=	fast->dpu_lfsr_seed_0;
		disp_params->LfsrSeeds[1]					=	fast->dpu_lfsr_seed_1;
		disp_params->LfsrSeeds[2]					=	fast->dpu_lfsr_seed_2;
		disp_params->LfsrSeeds[3]					=	fast->dpu_lfsr_seed_3;
		disp_params->McntSelectedIterations0_3		=
						(fast->mcnt_selected_iterations_0_0	 << 0) |
						(fast->mcnt_selected_iterations_0_1	 << 1) |
						(fast->mcnt_selected_iterations_0_2	 << 2) |
						(fast->mcnt_selected_iterations_0_3	 << 3);
	}
}

static void inpaint_special_params_set
(
	const struct vpul_pu_in_paint *inpaint,
	struct VPUI_DisparityFixedThr *disp_params
)
{
	if (disp_params != NULL){
		disp_params->InpFixCfgBits					=
			(inpaint->rl_validation_en		<<
				INP_FCFG_RL_VALIDATION_EN_LSB)	|
			(inpaint->enhanced_info_en		<<
				INP_FCFG_ENHANCED_INFO_EN_LSB)	|
			(inpaint->content_mask_en		<<
				INP_FCFG_CONTENT_MASK_EN_LSB)	|
			(inpaint->x_last_logic_en		<<
				INP_FCFG_X_LAST_LOGIC_EN_LSB)	|
			(inpaint->update_inpainted_cost_en <<
				INP_FCFG_UPDATE_INP_COST_EN_LSB);
		disp_params->GrayMaxSimilarity		= inpaint->gray_max_similarity;
		disp_params->GrayStepPenalty		= inpaint->gray_step_penalty;
		disp_params->GrayMaxTotalError		= inpaint->gray_max_total_error;
		disp_params->McntMinMaxSingle		=
			inpaint->mcnt_min_single << 0 | inpaint->mcnt_max_single << 4;
		disp_params->McntMinMaxForStatMask	=
			inpaint->mcnt_min_for_stat_mask << 0 | inpaint->mcnt_max_for_stat_mask << 4;
		disp_params->DsprMaxUpperSimilarity		= inpaint->dspr_max_upper_similarity;
		disp_params->DsprMaxAnchorsGradVal		= inpaint->dspr_max_anchors_grad_val;
		disp_params->DsprMaxDiffToCenter	= inpaint->dspr_max_diff_to_center;
		disp_params->AnchorTypeSelectBits	=
				inpaint->anchor_type_sel_max_support	  << ANC_TYSEL_MAX_SUPPORT_LSB		|
				inpaint->anchor_type_sel_max_near_support << ANC_TYSEL_MAX_NEAR_SUPPORT_LSB |
				inpaint->anchor_type_sel_min_valid		  << ANC_TYSEL_MIN_VALID_LSB        |
				inpaint->anchor_type_sel_min_near_valid   << ANC_TYSEL_MIN_NEAR_VALID_LSB;
		disp_params->AnchorLogimTrim_0_0			= inpaint->anchor_logic_trim_0_0;
		disp_params->AnchorLogimTrim_0_1			= inpaint->anchor_logic_trim_0_1;
		disp_params->XLastLogicMaxEdgeDiff			= inpaint->x_last_logic_max_edge_diff;
		disp_params->DownScaledCostMinPerIndex[0]	= inpaint->downscaled_cost_min_per_index_0_0;
		disp_params->DownScaledCostMinPerIndex[1]	= inpaint->downscaled_cost_min_per_index_0_1;
		disp_params->DownScaledCostMinPerIndex[2]	= inpaint->downscaled_cost_min_per_index_0_2;
		disp_params->CostMinIsHidden				= inpaint->cost_min_is_hidden;
		disp_params->CostMaxForStatMask				= inpaint->cost_max_for_stat_mask;
		disp_params->AnchorDistMaxForStrong			= inpaint->anchor_dist_max_for_strong;
		disp_params->GrayMaxSimilarity	= inpaint->background_gray_max_similarity;
		disp_params->BackgroundAnchorMargins[0]	= inpaint->background_anchor_margins_0_0;
		disp_params->BackgroundAnchorMargins[1]	= inpaint->background_anchor_margins_0_1;
		disp_params->BackgroundAnchorMargins[2]	= inpaint->background_anchor_margins_0_2;
		disp_params->BackgroundAnchorMargins[3]	= inpaint->background_anchor_margins_0_3;
		disp_params->AnchorLogimTrim_0_0		= inpaint->anchor_logic_trim_0_0;
		disp_params->AnchorLogimTrim_0_1		= inpaint->anchor_logic_trim_0_1;
		disp_params->OutputMode					= inpaint->output_mode;

	}
}

static void dispeq_special_params_set
(
	const struct vpul_pu_dispeq *dispeq,
	struct VPUI_DisparityFixedThr *disp_params
)
{
	if (disp_params != NULL){
		disp_params->DispEqBits 	=
			dispeq->mcnt_bilateral_maximization_en << DEQ_MCNT_BILATERAL_MAX_EN_LSB	  |
			dispeq->mcnt_use_center_only_en		   << DEQ_MCNT_CENTER_ONLY_EN_LSB     |
			dispeq->dspr_shift_down_before_validation << DEQ_DSPR_SH_DN_BEF_VALID_LSB |
			dispeq->mcnt_min_for_multiple_dspr << DEQ_MCNT_MIN_MULT_DSPR_LSB;
		disp_params->CostMinForValidForSmallDspr	= dispeq->cost_min_for_valid_for_small_dspr;
		disp_params->CostMinForValidForLargeDspr	= dispeq->cost_min_for_valid_for_large_dspr;
		disp_params->CostMinDiffForUpdate	= dispeq->cost_min_diff_for_update;
		disp_params->WeightForCostUpdate	= dispeq->weight_for_cost_update;
	}
}

static __param_assigment_function param_assigment_func[NUMBER_OF_OP_TYPE] = {
#define ENTRY(a) a##_PARAMS_ASSIGMENT,
#include "lib/vpul_pu_opcodes.def"
#undef ENTRY
};

/* describe one pu instance info */
struct pu_instance_info_s {
	/* block type */
	enum VPUH_Blocks pu_type;
	/* index on instance in the specific block instances */
	__u32 internal_idx;
};

static struct pu_instance_info_s pu_instance_info[VPU_PU_NUMBER] = {
#define VPU_PU_INSTANCE(a, b, c, d, e, f, g, h, i) {c, e},
#include "lib/vpul_pu_instances.def"
#undef VPU_PU_INSTANCE
};

/* set one block descriptor in Mbox "Blocks descriptors vector" */
static __u32 set_block_desc(
	/* pointer to mbox block descriptor */
	struct VPUI_BlUsage *mb_bl,
	/* block type */
	enum VPUH_Blocks block_type,
	/* offset of block first parameter from sub chain prm vctr start */
	__u32 param_offset,
	/* offset of block 1st connection index from sub chain start */
	__u32 connection_offset,
	/* total number of block input connections */
	__u32 num_of_input_connections,
	/* The index of the input sizes to the block in the entire process
	 * input sizes vector
	 */
	__u32 input_size_index
	)
{
	mb_bl->TypeAndParams =
		(block_type << HW_BL_TYPE_LSB) |
		(param_offset << HW_BL_PARAMS_OFS_LSB);

	mb_bl->ConnectAndInSizes =
		(connection_offset << HW_BL_CONNECT_OFS_LSB)  |
		(num_of_input_connections << HW_BL_CONNECT_NUM_LSB) |
		(input_size_index << HW_BL_IN_SIZES_IND_LSB);

	return num_of_input_connections;
}

__u32 is_preload_needed(enum vpul_pu_instance instance)
{
	return is_preload_needed_for_pu[pu_inst2type[instance]];
}

__u32 set_subchain_blocks_and_params(
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
	__u32 *initial_mprb_grp_offset_in_subchain)
{
	__u32 sc_param_ofs_idx  = 0;
	__u32 dis_sets_count	= 0;
	struct VPUI_HistParams *histo_param = mbp->histo_param;
	struct VPUI_DisparityFixedThr *pt_disp_fixed_param = mbp->pt_disp_fixed_param;
	__u16 *mb_proc_fst_prm = mbp->first_proc_prm;
	__u16 *mb_bl_params = mbp->block_prm + mbsc->fst_bl_prm;
	__u16 *mb_blocks_desc_ptr = (__u16 *)mbp->block_desc;
	__u32 *ret_n_bl_params = &mbsc->n_bl_params;
	__u32 *ret_n_hw_read  = &mbsc->n_hw_read_params;

	/* for calculating offset for non updatability */
	__u32  lcl_NumOfUpdatablePusRemained = internal_context->tds_itrtr_stat.num_of_updatable_pus_remained;

	/* mprbs offset  - in order to enable pu update on invoke, it is agreed that */
	/* updateable pu's mprbs will be located at the start. That way, it can be   */
	/* re-calculated during invoke, even when not all the pu's are available.    */
	/* that means that we must keep 2 accs, beginning from each type start       */
	__u32 sc_mprb_group_ofs_idx[VPU_MPRB_ACC_COUNT] = {0,0};
	__u32 mprb_acc_idx;
	__u32 scs_pu_idx;
	__u32 temp_hw_reads_num;
	__u32 total_mprb_groups = 0;

	/*for invoke*/
	__u32 scs_pu_params_size;      /* how many invoke prms will be written to MB for current PU. */
	__u32 IsUpdatable;
	__u16                             *mb_invoke_params_idxs_ptr  = *mb_InvokePrms_ptr;
	struct vpul_pu_location *next_updatable_pu_location_ptr = *updatable_pu_list_ptr;
	struct vpul_pu_location *lcl_updatable_pu_ptr;
	__u32  run_time_prms_counter;
	struct runtime_updated_pu_entry*  current_table_entry;
	__u16 *temp_ptr;
	__u32 jj;
	/*for invoke*/

	struct vpul_pu *pu      = first_pu_in_sc;
	struct vpul_pu *temp_pu = first_pu_in_sc;

	struct VPUI_BlUsage *mb_blocks_desc =
		(struct VPUI_BlUsage *)mb_blocks_desc_ptr;
	struct pu_output_location_entry* entry_ptr =
		&(internal_context->pu_output_location_entry_table[internal_context->pu_output_location_next]);

	if (internal_context->tds_itrtr_stat.num_of_updatable_pus_remained == 0 )
		next_updatable_pu_location_ptr = NULL;
		/* if no updatable left,set next ptr=NULL               */
		/* next ptr!=NULL is one of "isupdatable pu" conditions */

	lcl_updatable_pu_ptr = next_updatable_pu_location_ptr; /*used for mprbs count for updatable */
	*ret_n_hw_read = 0;

	/*calculate offset for non updatables - sum updateable Size, non updatable located right after.*/
	while (lcl_updatable_pu_ptr !=NULL) /* i.e, till the end of upd. PU's locations list */
	{
		if (check_pu_updability(lcl_updatable_pu_ptr,temp_pu,task_ptr))
		{
			lcl_NumOfUpdatablePusRemained--;
			sc_mprb_group_ofs_idx[VPU_MPRB_ACC_NONUPDATABLE_PU] += pu_get_num_params(temp_pu);
			lcl_updatable_pu_ptr =
				get_next_updatable_pu_location(
				lcl_NumOfUpdatablePusRemained, /* inout */
				lcl_updatable_pu_ptr);
		}
		temp_pu++;
	}

	for (scs_pu_idx = 0; scs_pu_idx < n_pus_in_sc; scs_pu_idx++, pu++) {
		__u32 mprb_gr_num = 0;
		__u32 mprb_gr_ofs = 0;
		__u32 special_param = 0; /*Additional param for special PU*/
		internal_context->tds_itrtr_stat.current_pu_id_in_sc = scs_pu_idx;
		if ((pu->op_type < NUMBER_OF_OP_TYPE) && (pu->op_type >= 0)) {
			if ((VPUL_OP_DMA == pu->op_type) &&
				(pu->params.dma.is_output_vector_dma)){
				pu->in_size_idx = 0;
			}

			*sc_connect_ofs_idx += set_block_desc(
				mb_blocks_desc++,
				pu_instance_info[pu->instance].pu_type,
				sc_param_ofs_idx, /* offset of pu prms vctr in SC prms vctr */
				*sc_connect_ofs_idx,
				pu->n_in_connect,
				pu->in_size_idx);


			/* check if current pu is on the updatable pu's list */
			IsUpdatable = check_pu_updability(next_updatable_pu_location_ptr,pu,task_ptr);


			if (IsUpdatable)
				mprb_acc_idx = VPU_MPRB_ACC_UPDATABLE_PU;
			else
				mprb_acc_idx = VPU_MPRB_ACC_NONUPDATABLE_PU;


			/* update static variable of total indices counter */
			/* update counter for next pu*/
			if (pu->n_mprbs) {

				mprb_gr_ofs = sc_mprb_group_ofs_idx[mprb_acc_idx] +
						*initial_mprb_grp_offset_in_subchain;
				mprb_gr_num = pu_get_num_mprb_groups(pu); /*total # of mprb groups for PU, including unused*/
				sc_mprb_group_ofs_idx[mprb_acc_idx] += mprb_gr_num;
				total_mprb_groups += mprb_gr_num;

				if(is_preload_needed(pu->instance)){
					internal_context->mprb_pu_table[internal_context->n_entries].proc_idx
						= internal_context->tds_itrtr_stat.current_vtx_id;
					internal_context->mprb_pu_table[internal_context->n_entries].sc_idx
						= internal_context->tds_itrtr_stat.current_sc_id_in_vtx;
					internal_context->mprb_pu_table[internal_context->n_entries].pu_idx
						= scs_pu_idx;
					internal_context->mprb_pu_table[internal_context->n_entries].mprb_group_num
						= 1;

					/*assuming:preloading PU's has at most MAXNUM_OF_PORTS_FOR_PRELOADING_PU ports.*/
					for (jj=0;jj<MAXNUM_OF_PORTS_FOR_PRELOADING_PU;jj++)
						/* keeping mprb's allocation for preloaded PU  */
						internal_context->mprb_pu_table[internal_context->n_entries].preloaded_mprb[jj] =
							pu->mprbs[jj];

					internal_context->n_entries++;
				}
			}

			///* special case for HISTOGRAM PU, some Histogram params sits in HistoParameters vector */
			if(pu->op_type == VPUL_OP_HIST){
				hist_special_param_set(
					&pu->params.histogram,
					histo_param+*hist_param_index);
				special_param = *hist_param_index;
				(*hist_param_index)++;
			}


			if (pu->op_type == VPUL_OP_FAST_DEPTH)
			{
				depth_special_params_set(&pu->params.depth,
					pt_disp_fixed_param + *fixed_disp_pram_idx);
				special_param = *fixed_disp_pram_idx;
				dis_sets_count++;

			}

			if (pu->op_type == VPUL_OP_IN_PAINT)
			{
				inpaint_special_params_set
					(&pu->params.in_paint,
					pt_disp_fixed_param + *fixed_disp_pram_idx);
				special_param = *fixed_disp_pram_idx;
				dis_sets_count++;
			}

			if (pu->op_type == VPUL_OP_DISPEQ)
			{
				dispeq_special_params_set
					(&pu->params.disparity,
					pt_disp_fixed_param + *fixed_disp_pram_idx);
				special_param = *fixed_disp_pram_idx;
				dis_sets_count++;
			}

			if (VPUL_DISPARITY_PU_NUM == dis_sets_count)
			{
				(*fixed_disp_pram_idx)++;
				dis_sets_count = 0;
			}

			/* general PU param assignment */
			/* sc_param_ofs_idx - offset of pu prms vctr in SC prms vctr, without crnt pu */
			scs_pu_params_size =
				param_assigment_func[pu->op_type](pu,
				mb_bl_params + sc_param_ofs_idx, mprb_gr_ofs, special_param);



			/* update table if this pu reads other pu's results                      */
			/* loop pass on table and act for each pu that wants current pu prms ofst*/
			/* NOTE, table send error if there are not enough values                 */
			for (run_time_prms_counter = 0;
				run_time_prms_counter<(pu->n_run_time_params_from_hw);
				run_time_prms_counter++)
			{
				current_table_entry =
					fnd_fst_val_apprnce_in_runtime_upd_tbl(internal_context->runtime_updated_pu_table, pu);
				temp_ptr = current_table_entry->location_to_write_updated_prms_offset;

				/* write currently processed pu parms offset to linking CPU (location of 1st argument) */
				(*temp_ptr) =
					mb_bl_params -
					mb_proc_fst_prm +
					sc_param_ofs_idx +
					current_table_entry->offset_in_dst_pu_prms;

				/* free (invalidate) entry for future uses */
				current_table_entry->updated_pu_identifier = NULL;
			}

			/* update table if HW results are generated in this pu  */
			temp_hw_reads_num = pu_num_of_hw_res_to_read(pu);


			/* if the current PU has HW writes, update it on PU_2_work-area_offst LUT */
			if (0 != temp_hw_reads_num)
			{
				/* assign processId      */
				entry_ptr->process_id = internal_context->tds_itrtr_stat.current_vtx_id;
				/* assign pu offset      */
				entry_ptr->pu_idenifier = pu;
				/* assign work area ofst */
				entry_ptr->offset_in_work_area = internal_context ->next_wr_ofst_on_work_area;
				internal_context ->pu_output_location_next++;

				(*ret_n_hw_read)++; /* count # of output generating PU's */

				/* advance offset on work area in the # of written data longs*/
				internal_context ->next_wr_ofst_on_work_area +=
					temp_hw_reads_num;

			}
			if (IsUpdatable)
			{
				(internal_context->tds_itrtr_stat.num_of_updatable_pus_remained)--;
				mb_invoke_params_idxs_ptr =
					wr_invoke_indices_to_MB(
						/* ptr in MB,updated on return*/
						mb_invoke_params_idxs_ptr,
						/*first idx val to write*/
						current_offset_from_param_start + sc_param_ofs_idx,
						 /*# of conseq. values to write  */
						scs_pu_params_size);


				(*accumulated_n_of_invoke_params_ptr)+= scs_pu_params_size;
				next_updatable_pu_location_ptr =
					get_next_updatable_pu_location(
						internal_context->tds_itrtr_stat.num_of_updatable_pus_remained,
						next_updatable_pu_location_ptr);


			}
			sc_param_ofs_idx += scs_pu_params_size;

			/*count internal memories usage*/
			if(pu_inst2type[pu->instance]==VPU_PU_TYPE_DMAIN||
				pu_inst2type[pu->instance]==VPU_PU_TYPE_DMAOT){
				 struct vpul_process * current_proc =
					 &((vtx_ptr(task_ptr, internal_context->tds_itrtr_stat.current_vtx_id))->proc);
				 __u32 roi_index = current_proc->io.inout_types[pu->params.dma.inout_index].roi_index;
				 __u32 memmap_idx = current_proc->io.fixed_map_roi[roi_index].memmap_idx;
				 if(task_ptr->memmap_desc[memmap_idx].mtype==VPUL_MEM_PRELOAD_PU||
					 task_ptr->memmap_desc[memmap_idx].mtype==VPUL_MEM_INTERNAL){
						 internal_context->n_internal_mem++;
				 }
			}

		} else {
			return VPU_STATUS_ILLEGAL_TASK;
		}
	}

	*updatable_pu_list_ptr = next_updatable_pu_location_ptr; /* update list ptr for next SC as output */
	*ret_n_bl_params       = sc_param_ofs_idx;
	*initial_mprb_grp_offset_in_subchain += total_mprb_groups;

	return VPU_STATUS_SUCCESS;
}

__u32 pu_get_num_params(const struct vpul_pu *pu)
{
	if (pu->op_type < NUMBER_OF_OP_TYPE)
		return param_assigment_func[pu->op_type](pu, NULL, 0,0);

	return 0;
}

__u32 pu_get_instance_idx(__u32 instance)
{
	return pu_instance_info[instance].internal_idx;
}
static const __u32 pu_num_of_hw_to_read_table[VPU_PU_NUMBER] = {
#define VPU_PU_INSTANCE(a, b, c, d, e, f, g, h, i) d,
#include "lib/vpul_pu_instances.def"
#undef VPU_PU_INSTANCE
};

__u32 pu_num_of_hw_res_to_read(const struct vpul_pu *pu)
{
	return pu_num_of_hw_to_read_table[pu->instance];
}

/* assumption - updatable PU input is ordered! */
/* the function returns pointer to next updatableStruct, and decreases # of remained updatable. */
struct vpul_pu_location*  get_next_updatable_pu_location(__u32 arg_num_of_updatable_pus_remained,
								   struct vpul_pu_location *updatable_pu_list)
{
	struct vpul_pu_location *rt_val;
	if (arg_num_of_updatable_pus_remained == 0)
		rt_val = NULL; /* no updatables left, set NULL to nextPtr */
	else
		rt_val = ++updatable_pu_list;
	return rt_val ;
}

__u32 check_pu_updability(struct vpul_pu_location* next_updatable_pu_location_ptr,
						struct vpul_pu *curent_pu_ptr,
						const struct vpul_task *TaskPtr)
{
	__u32 rt_val;
	if (next_updatable_pu_location_ptr == NULL)
		rt_val = 0;
	else
	{
		struct vpul_pu *next_updatable_pu =
			find_pu_by_pu_location_struct_ptr(next_updatable_pu_location_ptr,TaskPtr);
				/* TO BE CONSIDERED - get pointer to PU location onc getting next pu_location */
				/* set is as NULL if not exist                                                */
		rt_val = (next_updatable_pu == curent_pu_ptr);
	}

	return rt_val;
}

__u16* wr_invoke_indices_to_MB(
			__u16 *updatable_indices_on_mb,
			__u32 first_index_val,
			__u32 n_vals_to_write)
{
	/* loop over size of PU params*/
	__u32 pu_prms_counter;
	for (pu_prms_counter = 0;
		pu_prms_counter<n_vals_to_write;
		pu_prms_counter++)
			*updatable_indices_on_mb++ = first_index_val + pu_prms_counter;
			/* Num_of_vals_to_write consecutive indices are written      */
			/* [first_index_val:first_index_val + Num_of_vals_to_write]  */
	return updatable_indices_on_mb;
	/* returning next point to write on MB*/
}


__u32 generate_and_wr_invoke_parms_vector(
	const struct vpul_task *task_ptr,
	const union vpul_pu_parameters *invoke_pu_params_vector,
	__u8 *mb_cp
	)
{
	__u32 pu_counter;
	struct vpul_pu_location* crnt_pu_lctn_ptr;
	struct vpul_pu *crnt_pu_ptr;
	struct vpul_pu updated_pu;
	__u8 *lcl_mb_cp = mb_cp;
	__u16 rt_val = VPU_STATUS_SUCCESS;
	const union vpul_pu_parameters *lcl_invoke_pu_params_vector = invoke_pu_params_vector;
	__u32 sc_mprb_group_ofs_idx = 0;
	__u16 pu_prms_size_on_mb_befor_invk;
	__u16 pu_prms_size_on_mb_after_invk;


	/*Loop on all update-able PU's in task */
	for (pu_counter = 0; pu_counter<task_ptr->t_num_of_pu_params_on_invoke;pu_counter++)
	{

		/*	Get next update-able PU location */
		crnt_pu_lctn_ptr = updateble_pu_location_ptr(task_ptr, pu_counter);

		/*	Get next PU */
		crnt_pu_ptr = find_pu_by_pu_location_struct_ptr(crnt_pu_lctn_ptr,task_ptr);

		/*	Copy next pu*/
		memcpy(&updated_pu, crnt_pu_ptr, sizeof(struct vpul_pu));

		/* sanity check for invoked pu params*/
		/* create params from original pu and save size to be compared later to new params*/
		pu_prms_size_on_mb_befor_invk =
			param_assigment_func[updated_pu.op_type](&updated_pu,NULL,0, 0);

		/* on NULL vector, send original values from task on invoke*/
		if (invoke_pu_params_vector != NULL)
		{
			memcpy(&(updated_pu.params), lcl_invoke_pu_params_vector++,sizeof(union vpul_pu_parameters));

			/* sanity check for invoked pu params*/
			/* create params from original pu and compare to saved size above*/
			pu_prms_size_on_mb_after_invk =
				param_assigment_func[updated_pu.op_type](&updated_pu,NULL,0,0);
			if (pu_prms_size_on_mb_after_invk != pu_prms_size_on_mb_befor_invk)
				rt_val = VPU_STATUS_BAD_PARAMS;


		}

		/* re-calc mprb offsets */
		/* Assign pu params to vector, advance MB ptr */
		lcl_mb_cp += sizeof(__u16) *param_assigment_func[updated_pu.op_type](
				&updated_pu,
				(__u16*)lcl_mb_cp,
				sc_mprb_group_ofs_idx,
				0 );

		/* update next mprb offset */
		if (updated_pu.n_mprbs) {
			sc_mprb_group_ofs_idx +=
				pu_get_num_mprb_groups(&updated_pu);
		}
	}

	return rt_val;
}

struct mprb_entry * get_next_mprb_match( struct internal_context * ctx,
	__u32 proc_idx,
	__u32 sc_idx,
	__u32 pu_idx)
{
	struct mprb_entry * return_val=NULL;
	if((ctx!=NULL)&&(ctx->mprb_iter<ctx->n_entries)){
		struct mprb_entry * entry = &ctx->mprb_pu_table[ctx->mprb_iter];
		/* find process */
		while((ctx->mprb_iter<ctx->n_entries)
			&&(entry->proc_idx < proc_idx)){
				ctx->mprb_iter++;
				entry++;
		}
		if(entry->proc_idx > proc_idx){
			/* process was not found */
			return return_val;
		}
		/* find subchain */
		while((ctx->mprb_iter<ctx->n_entries)
			&&(entry->sc_idx < sc_idx)){
				ctx->mprb_iter++;
				entry++;
		}
		if(entry->sc_idx > sc_idx){
			/* sc was not found */
			return return_val;
		}
		/* find processing unit */
		while((ctx->mprb_iter<ctx->n_entries)
			&&(entry->pu_idx < pu_idx)){
				ctx->mprb_iter++;
				entry++;
		}
		if(entry->pu_idx > pu_idx){
			/* pu was not found */
			return return_val;
		}
		if(ctx->mprb_iter<ctx->n_entries){
			return_val = entry;
		}
		ctx->mprb_iter++;
	}

	return return_val;
}
