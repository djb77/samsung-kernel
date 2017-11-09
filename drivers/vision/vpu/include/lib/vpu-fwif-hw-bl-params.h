/**
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef VPU_FWIF_HW_BL_PARAMS_H
#define VPU_FWIF_HW_BL_PARAMS_H

#include "vpu-fwif-hw-gen.h"

/************* Explicit HW blocks parameters interface description ***********/

/**
 * *********************** DMA Parameters *************************************
 * Few planes will be handled by "few" DMAs. Joined YUV will be split to
 * components in the Splitter
 */
enum VPUI_Dma_Params {
	/* VPUI_DMA_CFG	fields */
	VPUI_DMA_CFG	= 0,
	VPUI_DMA_PARAMS	= 1
};

/* VPUI_DMA_CFG fields */
	/* The index of IO type description that includes the start address and the line offset */
#define VPUI_DMA_CFG_INOUT_INDEX_LSB	0
	/* VPUI_MAX_INOUT_PER_PROC == 32 */
#define VPUI_DMA_CFG_INOUT_INDEX_MASK		0x1f
	/* VPUH_DMA_OFS_LINES values */
#define VPUI_DMA_CFG_OFS_LINES_LSB	6
	/* 1 */
#define VPUI_DMA_CFG_OFS_LINES_MASK		VPUH_DMA_OFS_LINES_MASK

/*********************** FIFO Parameters *************************************/
enum VPUI_Fifo_Params {
	/* VPUI_FIFO_CFG fields */
	VPUI_FIFO_CFG		= 0,
	VPUI_FIFO_PARAMS	= 1
};

/* VPUI_FIFO_CFG fields */
	/* VPUH_SH_BITS values (0 for 8 bits, 1 for 16 bits) */
#define VPUI_FIFO_CFG_BITS_LSB		0
#define VPUI_FIFO_CFG_BITS_MASK			1
	/* If set - use memory block, else use internal RAM */
#define VPUI_FIFO_CFG_MPRB_EN_LSB	2
#define VPUI_FIFO_CFG_MPRB_EN_MASK		1
	/* VPUH_MPRB_TYPE values */
#define VPUI_FIFO_CFG_MPRB_TYPE_LSB	4
	/* 1 */
#define VPUI_FIFO_CFG_MPRB_TYPE_MASK		VPUH_MPRB_TYPE_MASK
	/* Required for interconnect. Single MPRB or none */
#define VPUI_FIFO_CFG_MPRB_GR_OFS_LSB	8
	/* 0x7f */
#define VPUI_FIFO_CFG_MPRB_GR_OFS_MASK		VPUI_MPRB_GR_OFS_MASK

/*********************** DUPL Parameters *************************************/
	/* DUPL has no parameters at all. Even its bypass has no meaning */
#define VPUI_DUPL_PARAMS		0

/*********************** SPLIT Parameters ************************************/
enum VPUI_Split_Params {
	/* VPUI_SPLIT_BYTES fields. */
	VPUI_SPLIT_BYTES_OUT_0_1	= 0,
	/* VPUI_SPLIT_BYTES fields. */
	VPUI_SPLIT_BYTES_OUT_2_3	= 1,
	VPUI_SPLIT_PARAMS		= 2
};


/* VPUI_SPLIT_BYTES fields */
#define VPUI_SPLIT_BYTES_FST_B0_LSB	0
#define VPUI_SPLIT_BYTES_FST_B1_LSB	4
#define VPUI_SPLIT_BYTES_SCND_B0_LSB	8
#define VPUI_SPLIT_BYTES_SCND_B1_LSB	12
	/* 0-3 for byte index from 32 bits Input, 4 for "set to zero" */
#define VPUI_SPLIT_BYTES_OUT_MASK		7

/*********************** JOIN Parameters *************************************/
enum VPUI_Join_Params {
	/* VPUI_JOIN_CFG fields */
	VPUI_JOIN_CFG		= 0,
	/* VPUI_JOIN_BYTES fields. */
	VPUI_JOIN_BYTES		= 1,
	VPUI_JOIN_PARAMS	= 2
};

/* VPUI_JOIN_CFG fields */
	/* VPUH_JOIN_MODE values */
#define VPUI_JOIN_CFG_MODE_LSB		0
	/* 3 */
#define VPUI_JOIN_CFG_MODE_MASK			VPUH_JOIN_MODE_MASK
	/* Bit for each input stream */
#define VPUI_JOIN_CFG_EN_INPUT_LSB	4
#define VPUI_JOIN_CFG_EN_INPUT_MASK		0xf
	/* Bit for each output byte. Bytes that are not enabled are set to 0 */
#define VPUI_JOIN_CFG_EN_OUT_BYTES_LSB	8
#define VPUI_JOIN_CFG_EN_OUT_BYTES_MASK		0xf

/* VPUI_JOIN_BYTES fields */
#define VPUI_JOIN_BYTE_0_SRC_LSB	0
#define VPUI_JOIN_BYTE_1_SRC_LSB	4
#define VPUI_JOIN_BYTE_2_SRC_LSB	8
#define VPUI_JOIN_BYTE_3_SRC_LSB	12
	/* 0-15 for byte input source (input stream index * 4  + Pixel Index * 2 + byte index)
	 * In VPUH_JOIN_MODE_1_IN_1_OUT only values of pixel index 0 are allowed
	 */
#define VPUI_JOIN_BYTES_OUT_MASK		0xf

/**
 * ********************** CALB / SALB  Parameters *****************************
 * Notice that CALB and SALB are similar. CALB has few more operation modes,
 * Round modes and 32 bits support => the dynamic values have single SALB
 * parameter each but 2 CALB parameters each to support 32 bits. Only SALB
 * supports truncated bits. All unified values are marked as ALB
 */
enum VPUI_Alb_Params {
	/* VPUI_ALB_STREAM fields. Enabled Streams, Default Stream,Number of Bits & Signs. */
	VPUI_ALB_STREAM		= 0,
	/* VPUI_ALB_OP fields */
	VPUI_ALB_OP		= 1,
	/* VPUI_ALB_BITS_AND_DY. Shift and truncate bits + number of dynamic parameters. */
	VPUI_ALB_BITS_AND_DY	= 2,
	VPUI_ALB_PARAMS		= 3
};

enum VPUI_Salb_Dyn_Params {
	/* relevant in modes 9 + 10 + 16 or in case VAL_MED is used as in1 */
	VPUI_SALB_DYN_VAL_MED	= 0,
	/* Below 4 parameters are relevant only for mode 16 and has single field */
	VPUI_SALB_DYN_VAL_LO	= 1,
	VPUI_SALB_DYN_VAL_HI	= 2,
	VPUI_SALB_DYN_THR_LO	= 3,
	VPUI_SALB_DYN_THR_HI	= 4,
	VPUI_SALB_DYN_PARAMS	= 5
};

enum VPUI_Calb_Dyn_Params {
	/* relevant in modes 9 + 10 + 16 or in case VAL_MED is used as in1 */
	VPUI_CALB_DYN_VAL_MED_LSB	= 0,
	/* relevant in modes 9 + 10 + 16 or in case VAL_MED is used as in1 */
	VPUI_CALB_DYN_VAL_MED_MSB	= 1,
	/* relevant only for mode 16 and modes 20-25 */
	VPUI_CALB_DYN_W0_OR_THR_LO_LSB	= 2,
	/* relevant only for mode 16 and modes 20-25 */
	VPUI_CALB_DYN_W0_OR_THR_LO_MSB	= 3,
	/* relevant only for mode 16 and modes 16, 21, 25 */
	VPUI_CALB_DYN_W1_OR_THR_HI_LSB	= 4,
	/* relevant only for mode 16 and modes 16, 21, 25 */
	VPUI_CALB_DYN_W1_OR_THR_HI_MSB	= 5,
	/* relevant only for mode 16 */
	VPUI_CALB_DYN_VAL_LO_LSB	= 6,
	/* relevant only for mode 16 */
	VPUI_CALB_DYN_VAL_LO_MSB	= 7,
	/* relevant only for mode 16 */
	VPUI_CALB_DYN_VAL_HI_LSB	= 8,
	/* relevant only for mode 16 */
	VPUI_CALB_DYN_VAL_HI_MSB	= 9,
	VPUI_CALB_DYN_PARAMS		= 10
};

/* VPUI_ALB_STREAM fields */
	/* 1 = stream enabled */
#define VPUI_ALB_STREAM_INPUT1_EN_LSB		0
	/* 1 = stream enabled */
#define VPUI_ALB_STREAM_MASK_EN_LSB		1
#define VPUI_ALB_STREAM_EN_MASK				1
	/* VPUH_SH_BITS values for SALB / VPUH_LONG_BITS values for CALB */
#define VPUI_ALB_STREAM_IN0_BITS_LSB		2
	/* VPUH_SH_BITS values for SALB / VPUH_LONG_BITS values for CALB */
#define VPUI_ALB_STREAM_IN1_BITS_LSB		4
	/* VPUH_SH_BITS values for SALB / VPUH_LONG_BITS values for CALB */
#define VPUI_ALB_STREAM_OUT_BITS_LSB		6
	/* 3 */
#define VPUI_ALB_STREAM_BITS_MASK			VPUH_BITS_MASK
	/* VPUH_SIGN values */
#define VPUI_ALB_STREAM_IN0_SIGN_LSB		8
	/* VPUH_SIGN values */
#define VPUI_ALB_STREAM_IN1_SIGN_LSB		9
	/* VPUH_SIGN values */
#define VPUI_ALB_STREAM_OUT_SIGN_LSB		10
	/* 1 */
#define VPUI_ALB_STREAM_SIGN_MASK			VPUH_SIGN_MASK
	/* In 0 / In 1 - for default value when Mask == 0 */
#define VPUI_ALB_STREAM_SI_SEL_IN_LSB		11
	/* VPUH_CALB_ROUND_MODE values - relevant only for CALB modes */
#define VPUI_ALB_STREAM_ROUND_MODE_LSB		12
	/* 7 */
#define VPUI_ALB_STREAM_ROUND_MODE_MASK			VPUH_CALB_ROUND_MODE_MASK
#define VPUI_ALB_STREAM_BYPASS_LSB		15
#define VPUI_ALB_STREAM_BYPASS_MASK			1

/* VPUI_ALB_OP fields */
	/* VPUH_ALB_MODE values */
#define VPUI_ALB_OP_MODE_LSB			0
	/* 0x1f */
#define VPUI_ALB_OP_MODE_MASK				VPUH_ALB_MODE_MASK
	/* VPUH_ALB_ABS_NEG values */
#define VPUI_ALB_OP_ABS_NEG_LSB			6
	/* 3 */
#define VPUI_ALB_OP_ABS_NEG_MASK			VPUH_ALB_ABS_NEG_MASK
	/* VPUH_TRUNC_OUT values */
#define VPUI_ALB_OP_TRUNC_OUT_LSB		8
	/* 3 */
#define VPUI_ALB_OP_TRUNC_OUT_MASK			VPUH_TRUNC_OUT_MASK
	/* VPUH_ALB_CLIP_MED values */
#define VPUI_ALB_OP_ORG_VAL_LSB			10
	/* 1 */
#define VPUI_ALB_OP_ORG_VAL_MASK			VPUH_ALB_CLIP_MED_MASK
	/* VPUH_ALB_COMP values - relevant only in mode 17 */
#define VPUI_ALB_OP_COMP_LSB			12
	/* 7 */
#define VPUI_ALB_OP_COMP_MASK				VPUH_ALB_COMP_MASK
	/* Whether VAL_MED is used as in1 */
#define VPUI_ALB_OP_VAL_MED_AS_IN1_LSB		15
#define VPUI_ALB_OP_VAL_MED_AS_IN1_MASK			1

/* VPUI_ALB_BITS_AND_DY fields */
	/* 0 - 32 */
#define VPUI_ALB_BITS_AND_DY_SH_BITS_LSB	0
#define VPUI_ALB_BITS_AND_DY_SH_BITS_MASK		0x3f
	/**
	 * relevant only for SALB -Number of output bits 0-15 (0 stands for all 16 bits are output)
	 * other bits are padded by zeros.Crop isn't invoked for modes VPUH_ALB_MODE_OR/XOR/AND/NOT
	 */
#define VPUI_ALB_BITS_AND_DY_OUT_BITS_LSB	6
#define VPUI_ALB_BITS_AND_DY_OUT_BITS_MASK		0xf
	/* Number of dynamic parameters - 0 / 1 / 5 for SALB,  0 / 2 / 4 / 6 / 10 for CALB */
#define VPUI_ALB_BITS_AND_DY_DYN_PARS_LSB	10
#define VPUI_ALB_BITS_AND_DY_DYN_PARS_MASK		0xf

/****************** SEPFL5,SEPFL7 and GENFL Parameters ***********************/
/** SEPFL5 and SEPFL7 have same parameters (SEPFL7 has more coefficients). **/
enum VPUI_Filt_Params {
	/* VPUI_FILT_STREAM fields. */
	VPUI_FILT_STREAM	= 0,
	/* VPUI_FILT_CFG + VPUI_SEP_FILT_CFG / VPUI_GEN_FILT_CFG fields. */
	VPUI_FILT_CFG		= 1,
	/* Index in	appropriate VPUI_FiltCoeffsSets vector of the task */
	VPUI_FILT_COEFF_IND	= 2,
	/* VPUI_FILT_MPRB fields. */
	VPUI_FILT_MPRB		= 3,
	/* For border mode "fill with constant" - this is the const value */
	VPUI_FILT_BORDER_CONST	= 4,
	VPUI_FILT_PARAMS	= 5
};

enum VPUI_SepFilt5MaxPool_Params {
	/* VPUI_SFILT5_MAXP fields. */
	VPUI_SFILT5_MAXP_BITS		= 0,
	/* Number of processed slices (images) - should be updated for each Layer & Slot */
	VPUI_SFILT5_MAXP_NUM_SLICES	= 1,
	/* VPUI_SFILT5_MAXP_SIZES fields -
	 * actual filter sizes in both axis + indices for spoof & output sizes
	 */
	VPUI_SFILT5_MAXP_SIZES		= 2,
	VPUI_SFILT5_MAXP_PARMAS		= 3
};

/* VPUI_FILT_STREAM fields.  SEPFL5, SEPFL7 and GENFL */
	/* VPUH_SH_BITS values for SEPFL5 and GENFL, VPUH_LONG_BITS values for FL7 */
#define	VPUI_FILT_STREAM_BITS_IN_LSB		0
	/* VPUH_SH_BITS values for SEPFL5 and GENFL, VPUH_LONG_BITS values for FL7 */
#define	VPUI_FILT_STREAM_BITS_OUT_LSB		2
	/* 3 */
#define	VPUI_FILT_STREAM_BITS_MASK			VPUH_BITS_MASK
	/* VPUH_SIGN values */
#define	VPUI_FILT_STREAM_SIGN_IN_LSB		4
	/* VPUH_SIGN values */
#define	VPUI_FILT_STREAM_SIGN_OUT_LSB		5
	/* 1 */
#define	VPUI_FILT_STREAM_SIGN_MASK			VPUH_SIGN_MASK
	/* 1 for Round */
#define	VPUI_FILT_STREAM_ROUND_LSB		6
#define	VPUI_FILT_STREAM_ROUND_MASK			1
	/* VPUH_FILT_BORDER_FILL values. */
#define	VPUI_FILT_STREAM_BORDER_FILL_LSB	8
#define	VPUI_FILT_STREAM_BORDER_FILL_MASK		3
	/* If set - the Border is forced to "Border Fill" for all tiles */
#define	VPUI_FILT_STREAM_BORDER_IGN_TILE_LSB	9
#define	VPUI_FILT_STREAM_BORDER_IGN_TILE_MASK		1

/* VPUI_FILT_CFG - fields for both SEPFL5, SEPFL7 and GENFL */
	/* VPUH_FILTER_SIZE values */
#define	VPUI_FILT_CFG_FILTER_SIZE_LSB		0
	/* 7 */
#define	VPUI_FILT_CFG_FILTER_SIZE_MASK			VPUH_FILTER_SIZE_MASK
	/* if 1 output on 2nd output the input "as is"(cropped sizes if replication not invoked) */
#define	VPUI_FILT_CFG_OUTPUT_INPUT_LSB		3
#define	VPUI_FILT_CFG_OUTPUT_INPUT_MASK			1
#define VPUI_FILT_CFG_BYPASS_LSB		4
#define VPUI_FILT_CFG_BYPASS_MASK			1

/* VPUI_SEP_FILT_CFG - extra CFG fields for SEPFL5 and SEPFL7 */
	/* VPUH_SEP_FILT_WORK_MODE values */
#define	VPUI_SEP_FILT_CFG_WORK_MODE_LSB		5
	/* 3 */
#define	VPUI_SEP_FILT_CFG_WORK_MODE_MASK		VPUH_SEP_FILT_WORK_MODE_MASK
	/* 1 = Flip. If set- no filter / add_zero / dn_sample operation, both outputs are equal */
#define	VPUI_SEP_FILT_CFG_FLIP_COLS_LSB		7
#define	VPUI_SEP_FILT_CFG_FLIP_COLS_MASK		1
	/* if set the filter is 1 x Filter Size - only horizontal coefficients are used */
#define	VPUI_SEP_FILT_CFG_HOR_ONLY_LSB		8
#define	VPUI_SEP_FILT_CFG_HOR_ONLY_MASK			1
	/* VPUH_SEP_FILT_ADD_ZEROS values. Insert Column / Line of 0 after or before each
	 * Column / Line  => 100 Input -> 196 output.
	 * Not allowed for VPUH_FILTER_SIZE_1x1 / VPUH_FILTER_SIZE_7x7 / VPUH_LONG_BITS_32_BITS.
	 */
#define	VPUI_SEP_FILT_CFG_ADD_ZEROS_LSB		9
	/* 3 */
#define	VPUI_SEP_FILT_CFG_ADD_ZEROS_MASK		VPUH_SEP_FILT_ADD_ZEROS_MASK
	/* 1 = Get the 1st,3rd,5th .. and ignore 2nd,4th .. samples after the filter as output
	 * => 200 Input -> 98 output
	 */
#define	VPUI_SEP_FILT_CFG_DSAMPLE_COLS_LSB	11
	/**
	 * (or get the 2nd,4th,6th .. and ignore 1st,3rd,5th .. samples
	 * - according to VPUI_SEP_FILT_SAMP_OFS_COLS/ROWS_SH)
	 * Not allowed for VPUH_FILTER_SIZE_1x1 / VPUH_FILTER_SIZE_7x7 / VPUH_LONG_BITS_32_BITS.
	 */
#define	VPUI_SEP_FILT_CFG_DSAMPLE_ROWS_LSB	12
#define	VPUI_SEP_FILT_CFG_DSAMPLE_MASK			1
	/**
	 * 0 / 1. If Down Sample is active - choose whether the	1st (0)
	 * / 2nd (1) of each couple it used. If Zeros added choose whether
	 *  the Zeros are added after the value (0) or before the value (1)
	 */
#define	VPUI_SEP_FILT_CFG_SAMP_OFS_COLS_LSB	13
#define	VPUI_SEP_FILT_CFG_SAMP_OFS_ROWS_LSB	14
#define	VPUI_SEP_FILT_CFG_SAMPL_OFS_MASK		1

/* VPUI_GEN_FILT_CFG - extra CFG fields for GENFL */
	/* If set - working in SAD mode */
#define	VPUI_GEN_FILT_CFG_IS_SAD_LSB		5
#define	VPUI_GEN_FILT_CFG_IS_SAD_MASK			1
	/**
	 * if set 2n filters are invoked and 2 outputs are send (allowed
	 * up to 7x7. For 7x7 with 2 filters must load coeffs with DMA)
	 */
#define	VPUI_GEN_FILT_CFG_2_FILTERS_LSB		6
#define	VPUI_GEN_FILT_CFG_2_FILTERS_MASK		1
	/**
	 * if set the coefficients are read by DMA - otherwise should be
	 * explicitly written to registers by CORE
	 */
#define	VPUI_GEN_FILT_CFG_COEFFS_DMA_LSB	7
#define	VPUI_GEN_FILT_CFG_COEFFS_DMA_MASK		1

/* VPUI_FILT_MPRB fields. SEPFL5, SEPFL7, GENFL, NMS and NLF */
	/* VPUH_MPRB_TYPE values */
#define	VPUI_FILT_MPRB_TYPE_LSB			0
	/* 1 */
#define	VPUI_FILT_MPRB_TYPE_MASK			VPUH_MPRB_TYPE_MASK
	/**
	 * 0/2/4/6 for SEPFL5, 0/2/4 for SEPFL7 and GENFL, 0/2/4/6/8/10 for NLF
	 * Notice - on FIFO mode this field stands for "Number of MPRBs
	 * connected to the block - 1" !!
	 */
#define	VPUI_FILT_MPRB_FST_OFS_LSB		1
#define	VPUI_FILT_MPRB_FST_OFS_MASK			0xf
	/**
	 * Required for interconnect - offset in sub chain vector to the vector
	 * of MPRBs groups connected to the block
	 */
#define VPUI_FILT_MPRB_GR_OFS_LSB		5
	/* 0x7f */
#define VPUI_FILT_MPRB_GR_OFS_MASK			VPUI_MPRB_GR_OFS_MASK
	/**
	 * Required for FIFO mode for SEPFL - number of MPRBs
	 * connected to the block
	 */
#define VPUI_FILT_MPRB_NUM_LSB			12
#define VPUI_FILT_MPRB_NUM_MASK				0xf

/* VPUI_SFILT5_MAXP fields. SEPFL5 only */
	/* If set - should convert the 16 block bits to 32 float output bits */
#define	VPUI_SFILT5_MAXP_32F_OUTPUT_LSB		0
#define	VPUI_SFILT5_MAXP_32F_OUTPUT_MASK		1
	/**
	 * If set - input should be converted from 2's compliment format
	 * (to sign & magnitude format)
	 */
#define	VPUI_SFILT5_MAXP_2COMP_INPUT_LSB	1
#define	VPUI_SFILT5_MAXP_2COMP_INPUT_MASK		1
	/**
	 * If set - output should be converted from S&M format to
	 * 2's complement format
	 */
#define	VPUI_SFILT5_MAXP_2COMP_OUTPUT_LSB	2
#define	VPUI_SFILT5_MAXP_2COMP_OUTPUT_MASK		1
	/* 1-4 */
#define	VPUI_SFILT5_MAXP_STRIDE_SIZE_LSB	3
#define	VPUI_SFILT5_MAXP_STRIDE_SIZE_MASK		7
	/* 0-3 */
#define	VPUI_SFILT5_MAXP_STRIDE_X_OFS_LSB	6
#define	VPUI_SFILT5_MAXP_STRIDE_X_OFS_MASK		3
	/* 0-3 */
#define	VPUI_SFILT5_MAXP_STRIDE_Y_OFS_LSB	8
#define	VPUI_SFILT5_MAXP_STRIDE_Y_OFS_MASK		3

/* VPUI_SFILT5_MAXP_SIZES fields. SEPFL5 only. */
	/* 1 - 5. Value must be <= filter size that is sent to HW */
#define	VPUI_SFILT5_MAXP_SIZES_FILT_HOR_LSB	0
	/* 1 - 5. Value must be <= filter size that is sent to HW */
#define	VPUI_SFILT5_MAXP_SIZES_FILT_VER_LSB	3
#define	VPUI_SFILT5_MAXP_SIZES_FILT_MASK		7
#define	VPUI_SFILT5_MAXP_SIZES_SPF_IND_LSB	6
#define	VPUI_SFILT5_MAXP_SIZES_OUT_IND_LSB	11
	/* 0x1f */
#define	VPUI_SFILT5_MAXP_SIZES_IND_MASK			VPUI_SIZES_IND_MASK

/****************** NMS Parameters *******************************************/
enum VPUI_Nms_Params {
	/* VPUI_NMS_STREAM fields. */
	VPUI_NMS_STREAM			= 0,
	/* VPUI_NMS_CFG fields. */
	VPUI_NMS_CFG			= 1,
	/* VPUI_FILT_MPRB fields. */
	VPUI_NMS_MPRB			= 2,
	/**
	 * For NMS - Check P >= N_i + Thr, For CENSUS mode - thresholds 0-3
	 * with shifters 0 / 4 / 8 / 12
	 */
	VPUI_NMS_THRESH_LSB		= 3,
	/**
	 * For NMS - upper 16 bits of The,  For CENSUS mode - thresholds 4-7
	 * with shifters 0 / 4 / 8 / 12
	 */
	VPUI_NMS_THRESH_MSB		= 4,
	/**
	 * Relevant only for Census mode / directional mode and for border mode
	 * "fill with constant" - this is the const value.
	 */
	VPUI_NMS_BORDER_CONST		= 5,
	/* relevant only for mode directional. VPUH_NMS_STRICT_COMP fields */
	VPUI_NMS_STRICT_COMP_MASK	= 6,
	VPUI_NMS_PARAMS			= 7
};

/* VPUI_NMS_STREAM fields. */
	/* VPUH_LONG_BITS values */
#define	VPUI_NMS_STREAM_BITS_IN_LSB		0
	/* VPUH_LONG_BITS values */
#define	VPUI_NMS_STREAM_BITS_OUT_LSB		4
	/* 3 */
#define	VPUI_NMS_STREAM_BITS_MASK			VPUH_BITS_MASK
	/* VPUH_SIGN values */
#define	VPUI_NMS_STREAM_SIGN_IN_LSB		6
	/* 1 */
#define	VPUI_NMS_STREAM_SIGN_MASK			VPUH_SIGN_MASK
	/* VPUH_FILTER_SIZE values */
#define	VPUI_NMS_STREAM_WINDOW_LSB		8
#define	VPUI_NMS_STREAM_WINDOW_MASK			3
	/* If not set - output pre-fixed value */
#define	VPUI_NMS_STREAM_OUT_ORG_VAL_LSB		10
#define	VPUI_NMS_STREAM_OUT_ORG_VAL_MASK		1
	/**
	 * VPUH_TRUNC_OUT values (1 Truncate , 0 Clip) - Relevant only when
	 * ORG_VAL is set
	 */
#define	VPUI_NMS_STREAM_TRUNC_OUT_LSB		12
#define	VPUI_NMS_STREAM_TRUNC_OUT_MASK			1

/* VPUI_NMS_CFG fields. */
	/* if set - Census mode is invoked. Otherwise - NMS */
#define	VPUI_NMS_CFG_IS_CENSUS_MODE_LSB		0
#define	VPUI_NMS_CFG_IS_CENSUS_MODE_MASK		1
	/**
	 * 0-4. Describe the neighbors that are compared.
	 * 0 stands for full window,
	 * 1-4 - ignore pixels in corners (more pixels for bigger number)
	 */
#define	VPUI_NMS_CFG_MASK_COMP_LSB		1
#define	VPUI_NMS_CFG_MASK_COMP_MASK			7
#define	VPUI_NMS_CFG_KEEP_EQUALS_LSB		4
	/* if set check P >= N_i + Thr else check P > N_i + Thr */
#define	VPUI_NMS_CFG_KEEP_EQUALS_MASK			1
	/**
	 * When set the neighbors compare mask is ignored
	 * and directional NMS is used
	 */
#define	VPUI_NMS_CFG_DIRECTIONAL_LSB		5
#define	VPUI_NMS_CFG_DIRECTIONAL_MASK			1
	/* VPUH_CENSUS_MODE fields */
#define	VPUI_NMS_CFG_CENSUS_MODE_LSB		6
#define	VPUI_NMS_CFG_CENSUS_MODE_MASK			3
	/**
	 * if set - in census mode the input pixel is send as 8 MSBs
	 * of the output
	 */
#define	VPUI_NMS_CFG_CENSUS_OUT_IMAGE_LSB	8
#define	VPUI_NMS_CFG_CENSUS_OUT_IMAGE_MASK		1
	/* VPUH_FILT_BORDER_FILL fields */
#define	VPUI_NMS_CFG_BORDER_FILL_LSB		10
#define	VPUI_NMS_CFG_BORDER_FILL_MASK			3
	/* If set - the Border is forced to "Border Fill" for all tiles */
#define	VPUI_NMS_CFG_BORDER_IGN_TILE_LSB	12
#define	VPUI_NMS_CFG_BORDER_IGN_TILE_MASK		1
#define VPUI_NMS_CFG_BYPASS_LSB			13
#define VPUI_NMS_CFG_BYPASS_MASK			1

/*************** NLF Parameters **********************************************/

enum VPUI_Nlf_Params {
	/* VPUI_NLF_CFG fields. */
	VPUI_NLF_CFG			= 0,
	/* VPUI_FILT_MPRB fields. */
	VPUI_NLF_MPRB			= 1,
	/* For border mode "fill with constant" - this is the const value */
	VPUI_NLF_BORDER_CONST		= 2,
	VPUI_NLF_ALL_MODES_PARAMS	= 3
};

enum VPUI_NlfNlf_Params {
	/**
	 * VPUH_NLF_NEIGHBORS values - 1 bit for each neighbor
	 * (not relevant in FAST mode)
	 */
	VPUI_NLF_NLF_NEIGHBORS		= 0,
	VPUI_NLF_NLF_MODE_PARAMS	= 1
};

enum VPUI_NlfCensus_Params {
	/* Thresholds 0-3 with shifters 0 / 4 / 8 / 12 */
	VPUI_NLF_CENSUS_THR_0_3		= 0,
	/* Thresholds 4-7 with shifters 0 / 4 / 8 / 12 */
	VPUI_NLF_CENSUS_THR_4_7		= 1,
	VPUI_NLF_CENSUS_MODE_PARAMS	= 2
};

enum VPUI_NlfSmooth_Params {
	/**
	 * VPUI_NLF_SM_BITS fields - Shift input left,
	 * output right and "noise"
	 */
	VPUI_NLF_SMOOTH_BITS		= 0,
	/* The value that represents "unknown" */
	VPUI_NLF_SMOOTH_UNKONWN_VAL	= 1,
	/* The offset to pixel that will be defined as "far" */
	VPUI_NLF_SMOOTH_FAR_THRESH	= 2,
	VPUI_NLF_SMOOTH_MODE_PARAMS	= 3
};

/* VPUI_NLF_CFG fields. */
	/* VPUH_NLF_MODE values */
#define	VPUI_NLF_CFG_MODE_LSB			0
#define	VPUI_NLF_CFG_MODE_MASK				7
	/* VPUH_NLF_FAST_MODE values */
#define	VPUI_NLF_CFG_FAST_MODE_LSB		4
#define	VPUI_NLF_CFG_FAST_MODE_MASK			1
	/* VPUH_SH_BITS values */
#define	VPUI_NLF_CFG_BITS_IN_LSB		5
#define	VPUI_NLF_CFG_BITS_IN_MASK			1
	/* VPUH_SIGN values */
#define	VPUI_NLF_CFG_SIGN_IN_LSB		6
#define	VPUI_NLF_CFG_SIGN_IN_MASK			1
	/* VPUH_CENSUS_MODE fields */
#define	VPUI_NLF_CFG_CENSUS_MODE_LSB		8
#define	VPUI_NLF_CFG_CENSUS_MODE_MASK			3
	/**
	 * if set - in census mode the input pixel is send as 8 MSBs of
	 * the output
	 */
#define	VPUI_NLF_CFG_CENSUS_OUT_IMAGE_LSB	10
#define	VPUI_NLF_CFG_CENSUS_OUT_IMAGE_MASK		1
	/* VPUH_FILT_BORDER_FILL fields */
#define	VPUI_NLF_CFG_BORDER_FILL_LSB		11
#define	VPUI_NLF_CFG_BORDER_FILL_MASK			3
	/* If set - the Border is forced to "Border Fill" for all tiles */
#define	VPUI_NLF_CFG_BORDER_IGN_TILE_LSB	13
#define	VPUI_NLF_CFG_BORDER_IGN_TILE_MASK		1
	/**
	 * Mode for unknown input in Smooth mode - 0 - return unknown ,
	 * 1 - return average of neighbors that are not unknown
	 */
#define	VPUI_NLF_CFG_SM_UNKNOWN_AV_LSB		14
#define	VPUI_NLF_CFG_SM_UNKNOWN_AV_MASK			1
#define VPUI_NLF_CFG_BYPASS_LSB			15
#define VPUI_NLF_CFG_BYPASS_MASK			1

/* VPUI_NLF_SM_BITS fields. */
	/* Input shift left in Smooth mode */
#define	VPUI_NLF_SM_BITS_IN_SH_LEFT_LSB		0
#define	VPUI_NLF_SM_BITS_IN_SH_LEFT_MASK		0xf
	/* Output shift right in Smooth mode */
#define	VPUI_NLF_SM_BITS_OUT_SH_RIGHT_LSB	4
#define	VPUI_NLF_SM_BITS_OUT_SH_RIGHT_MASK		0xf
	/**
	 * 1-25. Number of pixels from 5x5 filter that if pixel is "far" from
	 * them - it is replaced by "far" pixels average
	 */
#define	VPUI_NLF_SM_BITS_NOISE_MIN_COUNT_LSB	8
#define	VPUI_NLF_SM_BITS_NOISE_MIN_COUNT_MASK		0x1f

/*************** CCM Parameters **********************************************/
enum VPUH_Ccm_Params {
	/* 0x10 VPUI_CCM_CFG fields. */
	VPUI_CCM_CFG		= 0,
	VPUI_CCM_COEFF_0	= 1,
	VPUI_CCM_COEFF_1	= 2,
	VPUI_CCM_COEFF_2	= 3,
	VPUI_CCM_COEFF_3	= 4,
	VPUI_CCM_COEFF_4	= 5,
	VPUI_CCM_COEFF_5	= 6,
	VPUI_CCM_COEFF_6	= 7,
	VPUI_CCM_COEFF_7	= 8,
	VPUI_CCM_COEFF_8	= 9,
	VPUI_CCM_OFFSET_0	= 10,
	VPUI_CCM_OFFSET_1	= 11,
	VPUI_CCM_OFFSET_2	= 12,
	VPUI_CCM_PARAMS		= 13
};

/* VPUI_CCM_CFG fields */
	/* VPUH_SIGN values */
#define	VPUI_CCM_CFG_SIGN_IN_LSB		0
	/* VPUH_SIGN values */
#define	VPUI_CCM_CFG_SIGN_OUT_LSB		1
	/* 1 */
#define	VPUI_CCM_CFG_SIGN_MASK				VPUH_SIGN_MASK
	/* bit for each of the 3 input streams */
#define VPUI_CCM_CFG_INPUT_EN_MASK_LSB		2
#define VPUI_CCM_CFG_INPUT_EN_MASK_MASK			7
	/* bit for each of the 3 output streams */
#define VPUI_CCM_CFG_OUTPUT_EN_MASK_LSB		5
#define VPUI_CCM_CFG_OUTPUT_EN_MASK_MASK		7
	/* Right Shift of final results. 0-11 */
#define	VPUI_CCM_CFG_RIGHT_SH_LSB		8
#define	VPUI_CCM_CFG_RIGHT_SH_MASK			0xf
#define VPUI_CCM_CFG_BYPASS_LSB			12
#define VPUI_CCM_CFG_BYPASS_MASK			1

/*************** MDE Parameters **********************************************/
enum VPUI_Mde_Params {
	/* VPUI_MDE_CFG fields. */
	VPUI_MDE_CFG		= 0,
	/* VPUI_MDE_STREAM fields. */
	VPUI_MDE_STREAM		= 1,
	VPUI_MDE_EIG_COEFF_LSB	= 2,
	VPUI_MDE_EIG_COEFF_MSB	= 3,
	VPUI_MDE_THRESHOLD	= 4,
	VPUI_MDE_PARAMS		= 5
};

/* VPUI_MDE_CFG fields */
	/* VPUH_MDE_MODE values */
#define	VPUI_MDE_CFG_MODE_LSB		0
#define	VPUI_MDE_CFG_MODE_MASK			7
	/* Relevant only in mode 2 */
#define	VPUI_MDE_CFG_RESULT_SH_LSB	4
#define	VPUI_MDE_CFG_RESULT_SH_MASK		0x3f
#define	VPUI_MDE_CFG_USE_THR_LSB	12
#define	VPUI_MDE_CFG_USE_THR_MASK		1
	/* Whether angle is quantized to one of 4 values */
#define	VPUI_MDE_CFG_QUANT_ANGLE_LSB	13
#define	VPUI_MDE_CFG_QUANT_ANGLE_MASK		1
#define VPUI_MDE_CFG_BYPASS_LSB		14
#define VPUI_MDE_CFG_BYPASS_MASK		1

/* VPUI_MDE_STREAM fields */
	/* VPUH_LONG_BITS values */
#define	VPUI_MDE_STREAM_BITS_IN_LSB		0
	/* VPUH_LONG_BITS values */
#define	VPUI_MDE_STREAM_BITS_OUT_LSB		2
#define	VPUI_MDE_STREAM_BITS_MASK			3
	/* VPUH_SIGN values */
#define	VPUI_MDE_STREAM_SIGN_IN_LSB		4
	/* VPUH_SIGN values */
#define	VPUI_MDE_STREAM_SIGN_OUT_LSB		5
#define	VPUI_MDE_STREAM_SIGN_MASK			1
	/* Bit for each of the 2 outputs - this field is not relevant for
	 * 2 <= VPUI_MDE_CFG_MODE <= 5 where only output 0 is enabled
	 */
#define	VPUI_MDE_STREAM_OUT_EN_LSB		6
#define	VPUI_MDE_STREAM_OUT_EN_MASK			3

/*************** INTGIMG Parameters ******************************************/
enum VPUI_IntImg_Params {
	/* VPUI_INTIMG_CFG fields. */
	VPUI_INTIMG_CFG					= 0,
	/* VPUI_INTIMG_MPRB fields. */
	VPUI_INTIMG_MPRB				= 1,
	/**
	 * Horizontal Increment in distance transform modes / Min Label in
	 * CC mode / LUT size in LUT mode
	 */
	VPUI_INTIMG_DT_HOR_CC_MIN_LABEL_LUT_SIZE	= 2,
	/**
	 * Vertical Increment in distance transform modes / Label Size in
	 * CC mode / Add value in LUT mode
	 */
	VPUI_INTIMG_DT_VER_CC_LABEL_SIZE_LUT_ADD	= 3,
	/**
	 * Diagonal Increment in distance transform modes / Overflow value
	 * in LUT mode
	 */
	VPUI_INTIMG_DT_DIAG_LUT_OVERFLOW		= 4,
	/* Underflow value in LUT mode */
	VPUI_INTIMG_DT_LUT_UNDERFLOW			= 5,
	VPUI_INTIMG_PARAMS				= 6
};

/* VPUI_INTIMG_CFG fields */
	/* VPUH_INTIMG_MODE values */
#define VPUI_INTIMG_CFG_MODE_LSB		0
#define VPUI_INTIMG_CFG_MODE_MASK			7
	/* VPUH_INTIMG_OVFLOW values. Relevant only for Integral modes */
#define VPUI_INTIMG_CFG_OVFLOW_LSB		3
#define VPUI_INTIMG_CFG_OVFLOW_MASK			1
	/* VPUH_INTIMG_CC_MODE - Relevant only for CC mode */
#define VPUI_INTIMG_CFG_CC_MODE_LSB		4
#define VPUI_INTIMG_CFG_CC_MODE_MASK			3
	/* Relevant only for CC mode */
#define VPUI_INTIMG_CFG_CC_SMART_LABLE_LSB	6
#define VPUI_INTIMG_CFG_CC_SMART_LABLE_MASK		1
	/* Relevant only for CC mode */
#define VPUI_INTIMG_CFG_CC_RESET_LABLES_LSB	7
#define VPUI_INTIMG_CFG_CC_RESET_LABLES_MASK		1
	/* Left Shift of input. Relevant only in distance transform modes */
#define	VPUI_INTIMG_CFG_DT_SH_LEFT_LSB		8
	/* Right Shift of output. Relevant only in distance transform modes */
#define VPUI_INTIMG_CFG_DT_SH_RIGHT_LSB		12
#define VPUI_INTIMG_CFG_DT_SHIFTERS_MASK		7
#define VPUI_INTIMG_CFG_BYPASS_LSB		15
#define VPUI_INTIMG_CFG_BYPASS_MASK			1

/* VPUI_INTIMG_MPRB fields. */
	/* 0 / 3 / 7 / 11 / 15 */
#define	VPUI_INTIMG_MPRB_FST_OFS_LSB		0
#define	VPUI_INTIMG_MPRB_FST_OFS_MASK			0xf
	/* Required for interconnect */
#define VPUI_INTIMG_MPRB_GR_OFS_LSB		4
	/* 0x7f */
#define VPUI_INTIMG_MPRB_GR_OFS_MASK			VPUI_MPRB_GR_OFS_MASK

/*************** DWSC Parameters *********************************************/
enum VPUI_Dwsc_Params {
	/* VPUI_DWSC_CFG values. */
	VPUI_DWSC_CFG		= 0,
	VPUI_DWSC_PARAMS	= 1
};

/* VPUI_DWSC_CFG values */
	/* VPUH_SC_MODE values. */
#define VPUI_DWSC_CFG_MODE_LSB			0
#define VPUI_DWSC_CFG_MODE_MASK				3
	/* Index in configuration sizes list */
#define VPUI_DWSC_CFG_OUT_SIZES_IND_LSB		2
	/* 0x1f */
#define VPUI_DWSC_CFG_OUT_SIZES_IND_MASK		VPUI_SIZES_IND_MASK
	/* Required for interconnect */
#define VPUI_DWSC_CFG_MPRB_GR_OFS_LSB		7
	/* 0x7f */
#define VPUI_DWSC_CFG_MPRB_GR_OFS_MASK			VPUI_MPRB_GR_OFS_MASK
#define VPUI_DWSC_CFG_BYPASS_LSB		14
#define VPUI_DWSC_CFG_BYPASS_MASK			1

/*************** UPSC Parameters *********************************************/
enum VPUH_Upsc_Params {
	/* VPUI_UPSC_CFG fields. */
	VPUI_UPSC_CFG		= 0,
	VPUI_UPSC_BORDER_CONST	= 1,
	VPUI_UPSC_PARAMS	= 2
};

/* VPUI_UPSC_CFG fields */
	/* VPUH_SC_MODE values. */
#define VPUI_UPSC_CFG_MODE_LSB			0
#define VPUI_UPSC_CFG_MODE_MASK				3
	/* VPUH_UPSC_BORDER_FILL values */
#define VPUI_UPSC_CFG_BORDER_LSB		2
#define VPUI_UPSC_CFG_BORDER_MASK			1
	/* Index in configuration list of sizes */
#define VPUI_UPSC_CFG_OUT_SIZES_IND_LSB		3
	/* 0x1f */
#define VPUI_UPSC_CFG_OUT_SIZES_IND_MASK		VPUI_SIZES_IND_MASK
	/* Required for interconnect (always 4 blocks) */
#define VPUI_UPSC_CFG_MPRB_GR_OFS_LSB		8
	/* 0x7f */
#define VPUI_UPSC_CFG_MPRB_GR_OFS_MASK			VPUI_MPRB_GR_OFS_MASK
#define VPUI_UPSC_CFG_BYPASS_LSB		15
#define VPUI_UPSC_CFG_BYPASS_MASK			1

/*************** CROP Parameters *********************************************/
enum VPUI_Crop_Params {
	/* VPUI_CROP_CFG fields. */
	VPUI_CROP_CFG			= 0,
	VPUI_CROP_ROI_START_X		= 1,
	VPUI_CROP_ROI_START_Y		= 2,
	/**
	 * relevant for VPUH_CROP_MODE_PAD_OUTSIDE + VPUH_CROP_MODE_PAD_INSIDE
	 */
	VPUI_CROP_PAD_VALUE		= 3,
	/* relevant for VPUH_CROP_MODE_MASK */
	VPUI_CROP_MASK_IN_VALUE		= 4,
	/* relevant for VPUH_CROP_MODE_MASK */
	VPUI_CROP_MASK_OUT_VALUE	= 5,
	VPUI_CROP_PARAMS		= 6
};

/* VPUI_CROP_CFG fields */
	/* VPUH_CROP_MODE values. */
#define VPUI_CROP_CFG_MODE_LSB			0
#define VPUI_CROP_CFG_MODE_MASK				3
	/* Index in configuration sizes list */
#define VPUI_CROP_CFG_OUT_SIZES_IND_LSB		2
	/* 0x1f */
#define VPUI_CROP_CFG_OUT_SIZES_IND_MASK		VPUI_SIZES_IND_MASK
	/* If set - Crop start values are used for all tiles */
#define VPUI_CROP_CFG_IGN_TILE_LSB		7
#define VPUI_CROP_CFG_IGN_TILE_MASK			1
#define VPUI_CROP_CFG_BYPASS_LSB		8
#define VPUI_CROP_CFG_BYPASS_MASK			1

/*************** M2LI Parameters *********************************************/
/* Notice that when VPUI_M2LI_CFG_EN_LIST_OUT is set M2LI parameters must match
 * VPUI_OutList values. When VPUI_M2LI_CFG_EN_LIST_OUT is reset VPUI_OutList
 * must not be used. The RecordBytes filed in VPUI_OutList must match to the
 * values of VPUI_M2LI_CFG_VAL_IN_LIST + VPUI_M2LI_CFG_BITS_IN (4 when value is
 * not written to output, 6 when value is written and has 8 or 16 bits and 8
 * when value is written and has 32 bits
 */
enum VPUI_M2li_Params {
	/* VPUI_M2LI_CFG fields */
	VPUI_M2LI_CFG			= 0,
	/* VPUI_M2LI_COORDS fields */
	VPUI_M2LI_CROP_BEF_DOWN_SH	= 1,
	/* VPUI_M2LI_COORDS fields */
	VPUI_M2LI_CROP_AFT_UP_SH	= 2,
	/* Used for coordinates up-shifter as compensation for the list sizes
	 * compared to the current input sizes. Set to 0 if such compensation
	 * isn't required
	 */
	VPUI_M2LI_COORDS_FINAL_WIDTH	= 3,
	VPUI_M2LI_THR_LOW_LSB		= 4,
	VPUI_M2LI_THR_LOW_MSB		= 5,
	VPUI_M2LI_THR_HIGH_LSB		= 6,
	VPUI_M2LI_THR_HIGH_MSB		= 7,
	VPUI_M2LI_PARAMS		= 8
};

/* VPUH_M2LI_CFG fields */
	/* VPUH_LONG_BITS values */
#define VPUI_M2LI_CFG_BITS_IN_LSB	0
#define VPUI_M2LI_CFG_BITS_IN_MASK		VPUH_BITS_MASK
	/* VPUH_SIGN values */
#define VPUI_M2LI_CFG_SIGN_IN_LSB	2
#define VPUI_M2LI_CFG_SIGN_IN_MASK		1
	/**
	 * 0 => List include pairs of (x,y).
	 * 1 => List include Triplets of (x,y,value).
	 */
#define VPUI_M2LI_CFG_VAL_IN_LIST_LSB	3
#define VPUI_M2LI_CFG_VAL_IN_LIST_MASK		1
	/* Enable / disable each of the 2 outputs */
#define VPUI_M2LI_CFG_EN_MAP_OUT_LSB	4
#define VPUI_M2LI_CFG_EN_LIST_OUT_LSB	5
#define VPUI_M2LI_CFG_EN_OUT_MASK		1
#define VPUI_M2LI_CFG_INOUT_INDEX_LSB	6
	/* VPUI_MAX_INOUT_PER_PROC == 32 */
#define VPUI_M2LI_CFG_INOUT_INDEX_MASK		0x1f
#define VPUI_M2LI_CFG_BYPASS_LSB	11
#define VPUI_M2LI_CFG_BYPASS_MASK		1
#ifndef OLD_FWIF_IO
#define VPUI_M2LI_CFG_OUT_LIST_IND_LSB	12
#define VPUI_M2LI_CFG_OUT_LIST_IND_MASK		0xf
#endif

/* VPUI_M2LI_COORDS fields */
	/* Number of columns that are cropped along the process before / after
	 * the scale operation and should be compensated in M2LI coordinates.
	 */
#define VPUI_M2LI_COORDS_CROP_COLS_LSB	0
	/* Number of lines that are cropped along the process before / after
	 * the scale operation and should be compensated in M2LI coordinates.
	 */
#define VPUI_M2LI_COORDS_CROP_LINES_LSB	6
	/* Support crop up to 63 */
#define VPUI_M2LI_COORDS_CROP_MASK		0x3f
	/* Compensation in M2LI coordinates for any Down / Up scale that was
	 * performed along the process before M2LI. Set to 0 when Down scale
	 * compensation is not required. The compensation is in "power 2" units
	 */
#define VPUI_M2LI_COORDS_SH_LSB		12
#define VPUI_M2LI_COORDS_SH_MASK		0xf

/*************** LUT Parameters **********************************************/
enum VPUI_Lut_Params {
	/* VPUI_LUT_CFG fields */
	VPUI_LUT_CFG		= 0,
	/* Index in LUT is calculated as (Input - Offset) / BinSize */
	VPUI_LUT_OFFSET		= 1,
	/* Index in LUT is calculated as (Input - Offset) / BinSize */
	VPUI_LUT_BIN_SIZE	= 2,
	/* 65536 / Bin Size */
	VPUI_LUT_INV_BIN_SIZE	= 3,
	VPUI_LUT_PARAMS		= 4
};

/* VPUI_LUT_CFG fields */
	/**
	 * For 0 Output is LUT[Index],
	 * For 1 interpolation of LUT[Index] and LUT[Index+1] is performed
	 */
#define VPUI_LUT_CFG_INTERPOLATION_LSB		0
#define VPUI_LUT_CFG_INTERPOLATION_MASK			1
	/* VPUH_LUT_MODE_SIZE values (0 = 256, 1 = 1024) */
#define VPUI_LUT_CFG_SIZE_LSB			1
#define VPUI_LUT_CFG_SIZE_MASK				1
	/* VPUH_SIGN values */
#define VPUI_LUT_CFG_SIGN_IN_LSB		2
	/* VPUH_SIGN values */
#define VPUI_LUT_CFG_SIGN_OUT_LSB		3
#define VPUI_LUT_CFG_SIGN_MASK				1
	/* Required for interconnect (always single block) */
#define VPUI_LUT_CFG_MPRB_GR_OFS_LSB		4
	/* 0x7f */
#define VPUI_LUT_CFG_MPRB_GR_OFS_MASK			VPUI_MPRB_GR_OFS_MASK
#define VPUI_LUT_CFG_BYPASS_LSB			11
#define VPUI_LUT_CFG_BYPASS_MASK			1

/*************** HIST Parameters *********************************************/
enum VPUH_Hist_Params {
	/* VPUI_HIST_CFG fields */
	VPUI_HIST_CFG		= 0,
	VPUI_HIST_PARAMS	= 1
};

/* VPUI_HIST_CFG fields */
	/* VPUI_HistParams index + 1. 0 stands for VPUI_HistParams set as dynamic parameters
	 * after VPUI_HIST_PARAMS
	 */
#define VPUI_HIST_CFG_HIST_PARAMS_IND_P1_LSB	0
#define VPUI_HIST_CFG_HIST_PARAMS_IND_P1_MASK		0x3f
	/* If set to 0 - normal operation. If set to 1- 2nd input stream used as a weight factor */
#define VPUI_HIST_CFG_USE_WEIGHTS_LSB		6
#define VPUI_HIST_CFG_USE_WEIGHTS_MASK			1
	/* VPUH_SIGN values */
#define VPUI_HIST_CFG_SIGN_IN_LSB		7
#define VPUI_HIST_CFG_SIGN_IN_MASK			1
	/* 1 - Round the result of Hist Index calculation, 0 - no round */
#define VPUI_HIST_CFG_ROUND_LSB			8
#define VPUI_HIST_CFG_ROUND_MASK			1
	/* Required for interconnect (always 2 blocks) */
#define VPUI_HIST_CFG_MPRB_GR_OFS_LSB	9
	/* 0x7f */
#define VPUI_HIST_CFG_MPRB_GR_OFS_MASK		VPUI_MPRB_GR_OFS_MASK

/* Set after VPUI_HIST_PARAMS if dynamic VPUI_HistParams set of parameters is
 * used => VPUI_HIST_CFG_HIST_PARAMS_IND_P1 == 0
 */
#define VPUI_HIST_DYN_PARAMS	(sizeof(struct VPUI_HistParams) / 2)


/*************** ROI Parameters **********************************************/
enum VPUI_Roi_Params {
	/* VPUI_ROI_STREAM fields */
	VPUI_ROI_STREAM		= 0,
	/* VPUI_ROI_CFG fields */
	VPUI_ROI_CFG		= 1,
	VPUI_ROI_PARAMS		= 2
};

/* VPUI_ROI_STREAM fields */
	/* VPUH_LONG_BITS (not relevant for mode LK - 3*12 bits,stream 1 in other modes = 1 bit) */
#define VPUI_ROI_STREAM_BITS_IN0_LSB	0
#define VPUI_ROI_STREAM_BITS_IN0_MASK		3
	/* VPUH_SIGN (in mode LK - always signed) */
#define VPUI_ROI_STREAM_SIGN_IN0_LSB	4
#define VPUI_ROI_STREAM_SIGN_IN0_MASK		1
	/* If set to 1 2nd input is enabled: only pixels with mask != 0 are processed (mode!=LK) */
#define VPUI_ROI_STREAM_USE_MASK_LSB	8
#define VPUI_ROI_STREAM_USE_MASK_MASK		1
/* VPUI_ROI_CFG fields */
	/* VPUH_ROI_MODE values */
#define VPUI_ROI_CFG_MODE_LSB		0
#define VPUI_ROI_CFG_MODE_MASK			3
	/* VPUH_ROI_MIN_MAX values */
#define VPUI_ROI_CFG_MIN_MAX_LSB	2
#define VPUI_ROI_CFG_MIN_MAX_MASK		1
	/* 12 bits threshold for temporal difference in mode LK if Gt < LowThr Gt = 0 */
#define VPUI_ROI_CFG_LOW_THR_LSB	4
#define VPUI_ROI_CFG_LOW_THR_MASK		0xfff

/************** FALMORB Parameters *******************************************/
/* Notice that Flamorb assumption is that its 3rd MPRB was filled with list of
 * points before the block invocation - that is its 3rd MPRB should be defined
 * as Internal Memory and loaded by DMA on "pre load" stage
 */
enum VPUI_Flamorb_Params {
	/* VPUI_FLAMORB_CFG fields */
	VPUI_FLAMORB_CFG		= 0,
	/* VPUI_FLAMORB_IND fields */
	VPUI_FLAMORB_IND		= 1,
	/* Up to 1024 */
	VPUI_FLAMORB_PAIRS_PER_GROUPS	= 2,
	/* Required for interconnect - using 8 bits */
	VPUI_FLAMORB_MPRB_GR_OFS	= 3,
	VPUI_FLAMORB_PARAMS		= 4
};

/* VPUI_FLAMORB_CFG fields */
	/* VPUH_FLAMORB_OUT values */
#define VPUI_FLAMORB_CFG_OUT_LSB		0
#define VPUI_FLAMORB_CFG_OUT_MASK			1
	/* VPUH_MPRB_TYPE values */
#define VPUI_FLAMORB_CFG_MPRB_TYPE_LSB		1
#define VPUI_FLAMORB_CFG_MPRB_TYPE_MASK			1
	/* VPUH_SH_BITS values (0 for 8 bits, 1 for 16 bits) */
#define VPUI_FLAMORB_CFG_BITS_LSB		2
#define VPUI_FLAMORB_CFG_BITS_MASK			1
#define VPUI_FLAMORB_CFG_PAD_WITH_CONST_LSB	3
#define VPUI_FLAMORB_CFG_PAD_WITH_CONST_MASK		1
	/* Usually is set to 0 and might be overwritten by CPU */
#define VPUI_FLAMORB_CFG_GR_IND_LSB		4
#define VPUI_FLAMORB_CFG_GR_IND_MASK			0xf
	/* Assuming up to 16 groups */
#define VPUI_FLAMORB_CFG_PAD_VALUE_LSB		8
#define VPUI_FLAMORB_CFG_PAD_VALUE_MASK			0xff

/* VPUI_FLAMORB_IND fields */
	/* The index of the full ROI sizes */
#define VPUI_FLAMORB_IND_IN_ROI_LSB		0
	/* VPUI_MAX_INOUT_PER_PROC == 32 */
#define VPUI_FLAMORB_IND_IN_ROI_MASK			0x1f
	/* Set to 0 if no scaler in the way from DMA to FLAMORB
	 * Notice - the assumption is that input is never cropped before FALMORB !!
	 */
#define VPUI_FLAMORB_IND_SCALER_P1_LSB		5
#define VPUI_FLAMORB_IND_SCALER_P1_MASK			0x1f
#define VPUI_FLAMORB_IND_FULL_ROI_LSB		10
#define VPUI_FLAMORB_IND_FULL_ROI_MASK			0x1f

/*************** CNN Parameters **********************************************/
enum VPUI_Cnn_Params {
	/* VPUI_CNN_INFO fields */
	VPUI_CNN_INFO	= 0,
	/* VPUI_CNN_MPRBS fields */
	VPUI_CNN_MPRBS	= 1,
	VPUI_CNN_PARAMS	= 2
};

/* VPUI_CNN_INFO fields */
	/* Whether CNN Fetch mode is invoked or Convolution */
#define VPUI_CNN_INFO_IS_FETCH_LSB	0
#define VPUI_CNN_INFO_IS_FETCH_MASK		1
	/* Index in configuration sizes list */
#define VPUI_CNN_INFO_SP_SIZES_IND_LSB	1
	/* 0x1f */
#define VPUI_CNN_INFO_SP_SIZES_IND_MASK		VPUI_SIZES_IND_MASK
#define VPUI_CNN_INFO_BYPASS_LSB	6
#define VPUI_CNN_INFO_BYPASS_MASK		1

/* VPUI_CNN_MPRBS fields */
	/* DATA_MAX_MPRBS large MPRBs are allocated per input data, DATA_MAX_MPRBS small MPRBs are
	 * allocated per coefficients. Last small MPRB is for line buffers and must be allocated.
	 */
#define VPUI_CNN_DATA_MAX_MPRBS_LSB	0
#define VPUI_CNN_DATA_MAX_MPRBS_MASK		0x1f	/* Up to 20 */
	/**
	 * Required for interconnect - offset in sub chain vector to the vector
	 * of MPRBs groups connected to the block
	 */
#define VPUI_CNN_MPRBS_GR_OFS_LSB	8
#define VPUI_CNN_MPRBS_GR_OFS_MASK		VPUI_MPRB_GR_OFS_MASK

/************** DEPTH Parameters *********************************************/
enum VPUI_Depth_Params {
	/* VPUI_DEPTH_IND fields */
	VPUI_DEPTH_IND		= 0,
	/* VPUI_DEPTH_MPRB fields */
	VPUI_DEPTH_MPRB		= 1,
	/* VPUI_DEPTH_MIN_MAX_DISP fields */
	VPUI_DEPTH_MIN_MAX_DISP	= 2,
	VPUI_DEPTH_PARAMS	= 3
};
	/* VPUI_DisparityFixedThr index */
#define VPUI_DEPTH_IND_FIXED_LSB	0
	/* Actually single fixed threshold set is expected */
#define VPUI_DEPTH_IND_FIXED_MASK		0x3
	/* VPUI_DepthDynThr + 1 index - 0 stands for VPUI_DepthDynThr set as
	 * dynamic parameters after the fixed VPUI_DEPTH_PARAMS
	 */
#define VPUI_DEPTH_IND_DYN_P1_LSB	2
	/* Actually single dynamic threshold set is expected */
#define VPUI_DEPTH_IND_DYN_P1_MASK		0x3
#define VPUI_DEPTH_IND_BYPASS_LSB	4
#define VPUI_DEPTH_IND_BYPASS_MASK		1

/* Depth has 43 ports connected to MPRBs. Ports 0-12 always used and are always
 * connected to large MPRBs (used for general usage and 8 internal RAMs). There
 * are another internal 10 RAMs which has flexible connections. Each internal
 * RAM requires size of up to 1K for "small" image (image <= 512), up to 2K for
 * "medium" image (image <= 1024) and up to 3K for "big" image. Each of the
 * internal RAMs 8-14 are connected to either large MPRB (13-19) or 2 small
 * MPRBs (20+21 - 32+33). A bit should be set for each of those RAMs whether it
 * uses the large MPRB or the small MPRBs. Internal RAMs 15-17 are connected to
 * 3 small MPRBs (34-36, 37-39, 40-42).
 */
#define VPUI_DEPTH_MPRB_GR_OFS_LSB	0
	/* 0x7f */
#define VPUI_DEPTH_MPRB_GR_OFS_MASK		VPUI_MPRB_GR_OFS_MASK
	/* The 7 bits for each of RAMs 8- 14 (0 = connected to the appropriate
	 * 2 Small MPRBs, 1 = connected to appropriate large MPRB)
	 */
#define VPUI_DEPTH_MPRB_TYPE_MASK_LSB	8
#define VPUI_DEPTH_MPRB_TYPE_MASK_MASK		0x7f

/* VPUI_DEPTH_MIN_MAX_DISP fields */
/* == MAX(MaxDisparity / Div, 5) Where Div = 4 for 1/4 map, 2 for 1/2 map, 1 for full map */
#define VPUI_DEPTH_MIN_MAX_DISP_MAX_LSB	0
/* == 0 for 1/4 & 1/2 maps, 1 for full map + derived from CPU operation on chain 7 */
#define VPUI_DEPTH_MIN_MAX_DISP_MIN_LSB	8
	/* Maximal value is 119 */
#define VPUI_DEPTH_MIN_MAX_DISP_MASK		0x7f

/* Set after VPUI_DEPTH_PARAMS if dynamic VPUI_DepthDynThr set of parameters is
 * used => VPUI_DEPTH_IND_DYN_P1 == 0
 */
#define VPUI_DEPTH_DYN_PARAMS	(sizeof(struct VPUI_DepthDynThr) / 2)

/************* INPAINT Parameters ********************************************/
enum VPUI_Inpaint_Params {
	/* VPUI_INPAINT_IND_MPRB fields */
	VPUI_INPAINT_IND_MPRB		= 0,
	/* VPUI_INP_DYN fields */
	VPUI_INPAINT_DYN		= 1,
	/* == round(2 - log2(MaxDisparity / X / 150)) X = 2 for 1/4 map, 1 for 1/2 or full */
	VPUI_INPAINT_MAX_ANCHOR_GRAD_SH = 2,
	/* Set by CPU operation */
	VPUI_INPAINT_BG_GRAY_VAL	= 3,
	/* 0 on 1st invocation, Updated by CPU operation for 2nd invocation */
	VPUI_INPAINT_ALLOW_BG_EN	= 4,
	/* 0 on 1st invocation, Updated by CPU operation for 2nd invocation */
	VPUI_INPAINT_BG_DSPR_VAL	= 5,
	VPUI_INPAINT_PARAMS		= 6
};

	/* VPUI_DisparityFixedThr index */
#define VPUI_INPAINT_IND_MPRB_FIXED_LSB		0
	/* Actually single fixed threshold set is expected */
#define VPUI_INPAINT_IND_MPRB_FIXED_MASK		0x3
	/* VPUI_InPaintDynThr + 1 index - 0 stands for VPUI_InPaintDynThr set
	 * as dynamic parameters after the fixed VPUI_INPAINT_PARAMS
	 */
#define VPUI_INPAINT_IND_MPRB_DYN_P1_LSB	2
	/* Actually single dynamic threshold set is expected */
#define VPUI_INPAINT_IND_MPRB_DYN_P1_MASK		0x3
#define VPUI_INPAINT_IND_MPRB_GR_OFS_LSB	4
	/* 0x7f */
#define VPUI_INPAINT_IND_MPRB_GR_OFS_MASK		VPUI_MPRB_GR_OFS_MASK
/* Inpaint has 8 ports connected to MPRBs. All 8 connection are required only for "big" image -
   otherwise only the 4 even connections (0,2,4,6) should be connected
*/
#define VPUI_INPAINT_IND_BYPASS_LSB		11
#define VPUI_INPAINT_IND_BYPASS_MASK			1

/* VPUI_INP_DYN fields get different values for different block invocations */
#define VPUI_INP_DYN_OUTPUT_RELIABLE_MASK_EN_LSB	0
#define VPUI_INP_DYN_OUTPUT_RELIABLE_MASK_EN_MASK		1
#define VPUI_INP_DYN_OUTPUT_STAT_MASK_EN_LSB		1
#define VPUI_INP_DYN_OUTPUT_STAT_MASK_EN_MASK			1
#define VPUI_INP_DYN_X_LAST_LOGIC_RIGHT_MARGIN_LSB	2
#define VPUI_INP_DYN_X_LAST_LOGIC_RIGHT_MARGIN_MASK		0x1fff

/* Set after VPUI_INPAINT_PARAMS if dynamic VPUI_InPaintDynThr set of
 * parameters is used => VPUI_INPAINT_IND_MPRB_DYN_P1 == 0
 */
#define VPUI_INPAINT_DYN_PARAMS	(sizeof(struct VPUI_InPaintDynThr) / 2)

/************** DISPEQ Parameters ********************************************/
enum VPUI_Dispeq_Params {
	/* VPUI_DISPEQ_IND_MPRB fields */
	VPUI_DISPEQ_IND_MPRB	= 0,
	/* VPUI_DISPEQ_DYN fields */
	VPUI_DISPEQ_DYN		= 1,
	VPUI_DISPEQ_PARAMS	= 2
};
	/* VPUI_DisparityFixedThr index */
#define VPUI_DISPEQ_IND_MPRB_FIXED_LSB		0
	/* Actually single fixed threshold set is expected */
#define VPUI_DISPEQ_IND_MPRB_FIXED_MASK			0x3
#define VPUI_DISPEQ_IND_MPRB_GR_OFS_LSB		2
	/* 0x7f */
#define VPUI_DISPEQ_IND_MPRB_GR_OFS_MASK		VPUI_MPRB_GR_OFS_MASK
#define VPUI_DISPEQ_IND_BYPASS_LSB		9
#define VPUI_DISPEQ_IND_BYPASS_MASK			1
	/* DsprSmallToLargeTresh */
	/* == floor(max(3,MaxDisparity/X)) X = 20 for 1/4 map, 10 for 1/2 map and full map
	 * For full Map MaxDisparity is override by CPU calculation
	 */
#define VPUI_DISPEQ_DYN_SM_TO_LR_THR_LSB	0
#define VPUI_DISPEQ_DYN_SM_TO_LR_THR_MASK		0x7f
	/* == floor(max(1,MaxDisparity/X)) X = 32 for 1/4 map, 16 for 1/2 map and full map */
#define VPUI_DISPEQ_DYN_MIN_DIFF_VALID_LSB	8
#define VPUI_DISPEQ_DYN_MIN_DIFF_VALID_MASK		0x7f

/************* TN1_SBF Parameters ********************************************/
enum VPUI_Tn1Sbf_Params {
	/* VPUI_TN1SBF_CFG fields */
	VPUI_TN1SBF_CFG		= 0,
	VPUI_TN1SBF_OFFSET	= 1,
	/* VPUI_TN1SBF_IN_VALS fields */
	VPUI_TN1SBF_IN_VALS	= 2,
	VPUI_TN1SBF_NORM_OFS	= 3,
	/* VPUI_TN1SBF_CLIP_OFS fields */
	VPUI_TN1SBF_CLIP_OFS	= 4,
	VPUI_TN1SBF_PARAMS	= 5
};

/* VPUH_TN1SBF_CFG fields */
#define VPUI_TN1SBF_CFG_IN1_SEL_LSB		0
#define VPUI_TN1SBF_CFG_IN2_SEL_LSB		1
#define VPUI_TN1SBF_CFG_IN3_SEL_LSB		2
#define VPUI_TN1SBF_CFG_IN_SEL_MASK			1
#define VPUI_TN1SBF_CFG_YC_SEL_LSB		3
#define VPUI_TN1SBF_CFG_YC_SEL_MASK			1
#define VPUI_TN1SBF_CFG_MODE_LSB		4
#define VPUI_TN1SBF_CFG_MODE_MASK			3
#define VPUI_TN1SBF_CFG_CMODE_LSB		6
#define VPUI_TN1SBF_CFG_CMODE_MASK			1
#define VPUI_TN1SBF_CFG_CBLEND_LSB		7
#define VPUI_TN1SBF_CFG_CBLEND_MASK			1
#define VPUI_TN1SBF_CFG_SHIFT_DIR_LSB		8
#define VPUI_TN1SBF_CFG_SHIFT_DIR_MASK			1
#define VPUI_TN1SBF_CFG_SHIFT_VAL_LSB		9
#define VPUI_TN1SBF_CFG_SHIFT_VAL_MASK			0xf
#define VPUI_TN1SBF_CFG_BYPASS_LSB		13
#define VPUI_TN1SBF_CFG_BYPASS_MASK			1

/* VPUH_TN1SBF_IN_VALS fields */
#define VPUH_TN1SBF_IN_VALS_IN2_LSB		0
#define VPUH_TN1SBF_IN_VALS_IN3_LSB		8
#define VPUH_TN1SBF_IN_VALS_IN_MASK			0xff

/* VPUI_TN1SBF_CLIP_OFS fields */
#define VPUH_TN1SBF_CLIP_OFS_MIN_LSB		0
#define VPUH_TN1SBF_CLIP_OFS_MAX_LSB		0
#define VPUH_TN1SBF_CLIP_OFS_MASK			0xff

enum VPUI_Tn2Alf_Params {
	/* VPUI_TN_CFG fields */
	VPUI_TN2ALF_CFG			= 0,
	VPUI_TN2ALF_IO_INDEX		= 1,
	VPUI_TN2ALF_BORDER_COSNT	= 2,
	VPUI_TN2ALF_FFILTER_TH_Y	= 3,
	VPUI_TN2ALF_FFILTER_TH_F_CB	= 4,
	VPUI_TN2ALF_FFILTER_OFS_Y	= 5,
	VPUI_TN2ALF_FFILTER_OFS_F_CB	= 6,
	VPUI_TN2ALF_FFILTER_CLIP_MIN	= 7,
	VPUI_TN2ALF_FFILTER_CLIP_MAX	= 8,
	/* VPUI_TN2ALF_FF_SH fields */
	VPUI_TN2ALF_FFILTER_SH		= 9,
	VPUI_TN2ALF_FFILTER_THRESHOLD	= 10,
	VPUI_TN2ALF_MPRB_GR_OFS		= 11,
	VPUI_TN2ALF_PARAMS		= 12
};

/* VPUH_TN_CFG fields */
#define VPUI_TN_CFG_PIXEL_WIDTH_LSB	0
#define VPUI_TN_CFG_PIXEL_WIDTH_MASK		1
#define VPUI_TN_CFG_SIGNED_IN_LSB	1
#define VPUI_TN_CFG_SIGNED_IN_MASK		1
#define VPUI_TN_CFG_DATA_PASSTH_LSB	2
#define VPUI_TN_CFG_DATA_PASSTH_MASK		1
#define VPUI_TN_CFG_HORIZ_ONLY_LSB	3
#define VPUI_TN_CFG_HORIZ_ONLY_MASK		1
#define VPUI_TN_CFG_3x3_MODE_LSB	4
#define VPUI_TN_CFG_3x3_MODE_MASK		1
#define VPUI_TN_CFG_5x5_MODE_LSB	5
#define VPUI_TN_CFG_5x5_MODE_MASK		1
#define VPUI_TN_CFG_RAM_TYPE_LSB	6
#define VPUI_TN_CFG_RAM_TYPE_MASK		1
#define VPUI_TN_CFG_RAM_OFFSET_LSB	7
#define VPUI_TN_CFG_RAM_OFFSET_MASK		3
#define VPUI_TN_CFG_SEL_LBM_INPUT_LSB	9
	/* This field is relevant only for TN3_BRE block */
#define VPUI_TN_CFG_SEL_LBM_INPUT_MASK		1
	/* This field is relevant only for TN4_MIC block */
#define VPUI_TN_CFG_FF_THR_B_420_LSB	10
#define VPUI_TN_CFG_FF_THR_B_420_MASK		1
#define VPUI_TN_CFG_BYPASS_LSB		11
#define VPUI_TN_CFG_BYPASS_MASK			1

#define VPUI_TN_BORDER_FILL_VALUE		2
	/* The border fill value assumed to be always 2 */

/* VPUI_TN2ALF_FF_SH fields */
#define VPUI_TN2ALF_FF_SH_Y_VAL_LSB	0
#define VPUI_TN2ALF_FF_SH_C_VAL_LSB	4
#define VPUI_TN2ALF_FF_SH_VAL_MASK		0xf
#define VPUI_TN2ALF_FF_SH_Y_DIR_LSB	8
#define VPUI_TN2ALF_FF_SH_C_DIR_LSB	9
#define VPUI_TN2ALF_FF_SH_DIR_MASK		0x1
#define VPUI_TN2ALF_FF_SH_CMODE_LSB	10
#define VPUI_TN2ALF_FF_SH_CMODE_MASK		0x1

enum VPUI_Tn3VBre_Params {
	/* VPUI_TN_CFG fields */
	VPUI_TN3BRE_CFG			= 0,
	VPUI_TN3BRE_IO_INDEX		= 1,
	VPUI_TN3BRE_BORDER_COSNT	= 2,
	/* VPUI_TN3BRE_FF fields */
	VPUI_TN3BRE_FFILTER_BITS	= 3,
	VPUI_TN3BRE_FFILTER_OFS		= 4,
	VPUI_TN3BRE_MPRB_GR_OFS		= 5,
	VPUI_TN3BRE_PARAMS		= 6
};

/* VPUI_TN3BRE_FF fields */
/* VPUI_TN2ALF_FF_SH fields */
#define VPUI_TN3BRE_FF_E_MODE_LSB	0
#define VPUI_TN3BRE_FF_E_MODE_MASK		1
#define VPUI_TN3BRE_FF_E_420_LSB	1
#define VPUI_TN3BRE_FF_E_420_MASK		1
#define VPUI_TN3BRE_FF_E_CMODE_LSB	2
#define VPUI_TN3BRE_FF_E_CMODE_MASK		1

enum VPUI_Tn4Mic_Params {
	/* VPUI_TN_CFG fields  + */
	VPUI_TN4MIC_CFG			= 0,
	VPUI_TN4MIC_IO_INDEX		= 1,
	VPUI_TN4MIC_BORDER_COSNT	= 2,
	/* VPUI_TN4MIC_FF_THR fields (B_420 in VPUI_TN4MIC_FBLOCKM) */
	VPUI_TN4MIC_FFILTER_THR		= 3,
	/* VPUI_TN_FBLOCKM fields */
	VPUI_TN4MIC_FBLOCKM		= 4,
	VPUI_TN4MIC_MPRB_GR_OFS		= 5,
	VPUI_TN4MIC_PARAMS		= 6
};

/* VPUI_TN4MIC_FF_THR fields (B_420 in VPUI_TN4MIC_FBLOCKM) */
#define VPUI_TN4MIC_FF_THR_MIN_LSB	0
#define VPUI_TN4MIC_FF_THR_MAX_LSB	0
#define VPUI_TN4MIC_FF_THR_MASK			0xff

/* VPUI_TN_FBLOCKM fields (+ THR_B_420) */
#define VPUI_TN_FBLOCKM_EN_IN_SEL1_LSB	0
#define VPUI_TN_FBLOCKM_EN_IN_SEL2_LSB	1
#define VPUI_TN_FBLOCKM_EN_IN_SEL_MASK		1
#define VPUI_TN_FBLOCKM_EN_DEMUX_0_LSB	2
#define VPUI_TN_FBLOCKM_EN_DEMUX_0_MASK		3
#define VPUI_TN_FBLOCKM_EN_DEMUX_1_LSB	4
#define VPUI_TN_FBLOCKM_EN_DEMUX_1_MASK		3

enum VPUI_Tn5Rcf_Params {
	/* VPUI_TN_CFG fields */
	VPUI_TN5RCF_CFG			= 0,
	VPUI_TN5RCF_IO_INDEX		= 1,
	VPUI_TN5RCF_BORDER_COSNT	= 2,
	/* VPUI_TN4MIC_FM fields */
	VPUI_TN5RCF_FBLOCKM		= 3,
	/* Offset in process vector of filter coefficients */
	VPUI_TN5RCF_DYN_FILT_OFS	= 4,
	VPUI_TN5RCF_MPRB_GR_OFS		= 5,
	VPUI_TN5RCF_PARAMS		= 6
};
#endif
