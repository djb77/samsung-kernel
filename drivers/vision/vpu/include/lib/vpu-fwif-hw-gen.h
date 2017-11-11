
/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef VS4L_FWIF_HW_GEN_
#define VS4L_FWIF_HW_GEN_

/*************** General description - HW Values ******************************
 * This file include generic HW information that is relevant for all projects.
 * It describes information that is used both for writing the HW registers
 * and for CORE - HOST interface : enum of HW blocks types, information
 * per block (number of inputs & outputs etc.) and values of explicit HW fields
 * Notice that ALL the HW blocks should be described here - not all projects
 * include all the blocks.
 * Explicit project information required for writing the HW registers and for
 * CORE / HOST interface (e.g. number of instances of each block) is kept in
 * vpu-fwif-hw-proj.h
 */

/* The enum below describe all the HW blocks that may appear in HW chain */
enum VPUH_Blocks {
	/* 16 bits Input DMA */
	VPUH_BL_I_DMA		= 0,
	/* 16 bits Input DMA with MNM */
	VPUH_BL_I_MNM_DMA	= 1,
	/* 32 bits (Wide) Input DMA */
	VPUH_BL_WI_DMA		= 2,
	/* 16 bits Output DMA */
	VPUH_BL_O_DMA		= 3,
	/* 16 bits Output DMA with MNM */
	VPUH_BL_O_MNM_DMA	= 4,
	/* 32 bits (Wide) Output DMA */
	VPUH_BL_WO_DMA		= 5,
	/* keep on streams synchronization. Assumed to be the 1st ISP block
	 * after all DMA blocks
	 */
	VPUH_BL_FIFO		= 6,
	/* Duplicate 16 bits input stream to 2 16 bits output streams */
	VPUH_BL_DUPL_16		= 7,
	/* Duplicate 32 bits input stream to 2 32 bits output streams */
	VPUH_BL_DUPL_32		= 8,
	/* Input 32 splitted to up to 4 output * 16 bits */
	VPUH_BL_SPLIT		= 9,
	/* Up to 4 input stream joined to 32 output bits */
	VPUH_BL_JOIN		= 10,
	/* Simple operation on 2 * 16 bits + 2 bits mask input */
	VPUH_BL_SALB		= 11,
	/* Complex operation = Simple operation on 2 * 32 bits or Multiply */
	VPUH_BL_CALB		= 12,
	/* 1*1 / 3*3 / 5*5 separable filter  */
	VPUH_BL_SEPFL_5x5	= 13,
	/* 1*1 / 3*3 / 5*5 / 7x7 separable filter  */
	VPUH_BL_SEPFL_7x7	= 14,
	/* Up to 11*11 single matrix filter or 2 * up to 7*7 matrix filter */
	VPUH_BL_GENFL		= 15,
	/* No Max suppresion (in 7*7 area) */
	VPUH_BL_NMS		= 16,
	/* Min / Max / Median (in 3*3 area) / Corner detector (in 7*7 area) */
	VPUH_BL_NLF		= 17,
	/* 3 Input Stream * Matrix 3 * 3 -> 3 ouput streams */
	VPUH_BL_CCM		= 18,
	/* Magnitude & Angle of 2 input streams / Eigen value */
	VPUH_BL_MDE		= 19,
	/* Integral Image */
	VPUH_BL_INTIMG		= 20,
	/* Down Scaler */
	VPUH_BL_DWSC		= 21,
	/* Up Scaler */
	VPUH_BL_UPSC		= 22,
	/* Cropper */
	VPUH_BL_CROP		= 23,
	/* Map to list - out0 is stream of 0 / 0xffff and out1 is the List
	 * written to memory
	 */
	VPUH_BL_M2LI		= 24,
	/* Look Up Table */
	VPUH_BL_LUT		= 25,
	/* Histogram */
	VPUH_BL_HIST		= 26,
	/* Statistics in single ROI. For LK In0 = Gt, In1 = Gx, In2 = Gy, For
	 * ROI modes In0 = Image, In1 = Mask
	 */
	VPUH_BL_ROI		= 27,
	/* Required for Facial Landmarks. Compare list of points and output
	 * single bit per comparison
	 */
	VPUH_BL_FLAMORB		= 28,
	/* CNN application. 3d filters multiplied by set of input maps */
	VPUH_BL_CNN		= 29,
	/* Depth application. Disparity of 2 images in X axis per pixel */
	VPUH_BL_DEPTH		= 30,
	/* Depth application. Replace disparity values of unreliable pixels  */
	VPUH_BL_IN_PAINT	= 31,
	/* Depth application. Integrate disparity values of "left to right" and
	 * "right to left" comparisons.
	 */
	VPUH_BL_DISP_EQ		= 32,
	VPUH_BL_TN1_SBF		= 33,
	VPUH_BL_TN2_ALF		= 34,
	VPUH_BL_TN3_BRE		= 35,
	VPUH_BL_TN4_MIC		= 36,
	VPUH_BL_TN5_RCF		= 37,
	VPUH_BL_ALL		= 38
};

struct VPUH_GenBlType {
	/* The number of columns 16b input indices per each instance*/
	__u8 uNum16bInputs;
	/* The number of columns 32b input indices per each instance*/
	__u8 uNum32bInputs;
	/* The number of interconnect 16b output indices per each instance */
	__u8 uNum16bOutputs;
	/* The number of interconnect 32b output indices per each instance */
	__u8 uNum32bOutputs;
	/* The number of MPRBs connections per each instance */
	__u8 uNumMprbsCon;
	/* The number of Hw results of the blocks */
	__u8 uNumHwRes;
};

#define VPUH_GEN_BL_TYPES_INFO	{				\
	{0, 0, 1, 0,  0, 0},	/*  VPUH_BL_I_DMA */		\
	{0, 0, 1, 0,  0, 0},	/*  VPUH_BL_I_MNM_DMA */	\
	{0, 0, 0, 1,  0, 0},	/*  VPUH_BL_WI_DMA */		\
	{1, 0, 0, 0,  0, 0},	/*  VPUH_BL_O_DMA */		\
	{1, 0, 0, 0,  0, 0},	/*  VPUH_BL_O_MNM_DMA */	\
	{0, 1, 0, 0,  0, 0},	/*  VPUH_BL_WO_DMA */		\
	{1, 0, 1, 0,  1, 0},	/*  VPUH_BL_FIFO */		\
	{1, 0, 2, 0,  0, 0},	/*  VPUH_BL_DUPL_16 */		\
	{0, 1, 0, 2,  0, 0},	/*  VPUH_BL_DUPL_32 */		\
	{0, 1, 4, 0,  0, 0},	/*  VPUH_BL_SPLIT */		\
	{4, 0, 0, 1,  0, 0},	/*  VPUH_BL_JOIN */		\
	{3, 0, 1, 0,  0, 0},	/*  VPUH_BL_SALB */		\
	{0, 3, 0, 1,  0, 0},	/*  VPUH_BL_CALB */		\
	{1, 0, 2, 0,  8, 1},	/*  VPUH_BL_SEPFL_5x5 */	\
			/*  HwRes = MaxValue */			\
	{0, 1, 0, 2,  6, 0},	/*  VPUH_BL_SEPFL_7x7 */	\
	{2, 0, 3, 0,  6, 0},	/*  VPUH_BL_GENFL */		\
	{0, 1, 0, 1,  8, 0},	/*  VPUH_BL_NMS */		\
	{1, 0, 1, 0, 12, 0},	/*  VPUH_BL_NLF */		\
	{3, 0, 3, 0,  0, 0},	/*  VPUH_BL_CCM */		\
	{0, 3, 0, 2,  0, 0},	/*  VPUH_BL_MDE */		\
	{1, 0, 0, 1, 20, 1},	/*  VPUH_BL_INTIMG */		\
			/*  HwRes = IsOverflow */		\
	{1, 0, 1, 0,  4, 0},	/*  VPUH_BL_DWSC */		\
	{1, 0, 1, 0,  4, 0},	/*  VPUH_BL_UPSC */		\
	{1, 0, 1, 0,  0, 0},	/*  VPUH_BL_CROP */		\
	{0, 1, 1, 1,  0, 1},	/*  VPUH_BL_M2LI */		\
			/*  HwRes = Num of points found */	\
	{1, 0, 1, 0,  1, 0},	/*  VPUH_BL_LUT */		\
	{2, 0, 0, 0,  2, 0},	/*  VPUH_BL_HIST */		\
	{2, 1, 0, 0,  0, 6},	/*  VPUH_BL_ROI */		\
			/*  HwRes=VPUH_ROI_OUT_LK/STD/MINMAX_*/	\
	{1, 0, 2, 0,  3, 0},	/*  VPUH_BL_FLAMORB */		\
	{3, 2, 0, 1, 41, 2},	/*  VPUH_BL_CNN */		\
			/*  HwRes = BadInput + IsOverflow */	\
	{2, 1, 0, 1, 43, 0},	/*  VPUH_BL_DEPTH */		\
	{1, 1, 0, 1,  8, 0},	/*  VPUH_BL_IN_PAINT */		\
	{0, 2, 0, 1,  2, 0},	/*  VPUH_BL_DISP_EQ */		\
	{3, 0, 1, 0,  0, 0},	/*  VPUH_BL_TN1_SBF */		\
	{1, 0, 1, 0,  4, 0},	/*  VPUH_BL_TN2_ALF */		\
	{1, 0, 1, 0,  4, 0},	/*  VPUH_BL_TN3_BRE */		\
	{2, 0, 4, 0,  8, 0},	/*  VPUH_BL_TN4_MIC  */		\
	{2, 0, 2, 0,  8, 0}	/*  VPUH_BL_TN5_RCF */		\
	}
/* NOTE: VPUH_BL_TN3_BRE, VPUH_BL_TN4_MIC includes also an input entry of 80
 * bits not connected to columns
 */

/* TODO(SW-15) - Notice to handle the maximal input lines to filters:
 * (for N lines filter N-1 lines required as internal memory)
 * 16 bits : 4K to FL5_8M (8*4 = 32 = 4*4*2), 1K to FL5_2M, 2K to FL7
 * (6*4=24=6*2*2), 2K to FLGEN, 4K to DS, US, INTIMG, NLF, NMS
 * The valid ISP / MPRB connections are actually required only for HOST and for
 * "chain verification validity" Debug SW !!
 * Same is TRUE for MPRB information that describe for each MPRB to which HW
 * blocks it is connected Same is TRUE for uMaxDelayCycles for each HW block
 *
 * uMaxInput && uMaxOutputs required for interface && maybe required also for
 * HW description (when writing HW block for each of its input should write ACK
 * The interconnect indices
 * (input / output / MPRB : VPUH_Blocks_Interconnect_str) -
 * are required only for CORE code
 * (HOST describes that 2nd instance of FILT5 is connected to MPRB 17 but CORE
 * has to connect MPRB 17 to Filt5 etc ..)
 */


/*************** General ISP fields Description **************************/

/* VPUH_SH_BITS values (for 8/16 bits values) + VPUH_LONG_BITS value*/
#define	VPUH_BITS_8_BITS			0
#define	VPUH_BITS_16_BITS			1
#define	VPUH_LONG_BITS_32_BITS			2
/* Support also the 32 bits */
#define	VPUH_BITS_MASK					3
/* VPUH_SIGN value*/
#define	VPUH_SIGN_UNSIGNED			0
#define	VPUH_SIGN_SIGNED			1
#define VPUH_SIGN_MASK					1
/* VPUH_MPRB_TYPE values */
#define	VPUH_MPRB_TYPE_1K			0
#define	VPUH_MPRB_TYPE_4K			1
#define VPUH_MPRB_TYPE_MASK				1
/* VPUH_TRUNC_OUT value*/
/* according to number of bits and sign of the output */
#define	VPUH_TRUNC_OUT_CLIP			0
#define	VPUH_TRUNC_OUT_TRUNCATE			1
#define	VPUH_TRUNC_OUT_TRU_NO_SIGN		2
#define	VPUH_TRUNC_OUT_MASK				3
/* VPUH_DMA_OFS_LINES value*/
/* Address is incremented between each 2 lines */
#define	VPUH_DMA_OFS_LINES_INC			0
#define	VPUH_DMA_OFS_LINES_DEC			1
#define	VPUH_DMA_OFS_LINES_MASK				1
/* VPUH_JOIN_MODE value*/
/* 32 bit output for each 4 input 16 bit words */
#define	VPUH_JOIN_MODE_1_IN_1_OUT		0
/* 32 bit output for each 2 sets of 4 input 16 bit words */
#define	VPUH_JOIN_MODE_2_IN_1_OUT		1
/* 2 * 32 bit output (where 2 * 16 MSBs are set to 0) for each 2 sets
 * of 4 input 16 bit words
 */
#define	VPUH_JOIN_MODE_2_IN_2_OUT		2
#define VPUH_JOIN_MODE_MASK				3
/* VPUH_ALB_MODE values : All modes if (mask) below operation else SI, */
/* In0 - In1 */
#define	VPUH_ALB_MODE_0_MINUS_1			0
/* In1 - In0 */
#define	VPUH_ALB_MODE_1_MINUS_0			1
#define	VPUH_ALB_MODE_MIN			2
#define	VPUH_ALB_MODE_MAX			3
#define	VPUH_ALB_MODE_OR			4
#define	VPUH_ALB_MODE_XOR			5
#define	VPUH_ALB_MODE_AND			6
#define	VPUH_ALB_MODE_NOT			7
#define	VPUH_ALB_MODE_ADD			8
	/* In0 + Val Med */
#define	VPUH_ALB_MODE_IN_PLUS_MED		9
	/* Val Med */
#define	VPUH_ALB_MODE_MED			10
	/**
	 * For each line in "boolean matrix" - count number of "1" in
	 * vector & ~Mask
	 */
#define	VPUH_SALB_MODE_MAT_AND_NOT_MASK		11
	/* In >= 0 ? 1 : 0 */
#define	VPUH_ALB_MODE_IS_POS			12
	/**
	 * For each line in "boolean matrix" - count number of "1"
	 * in vector & Mask
	 */
#define	VPUH_SALB_MODE_MAT_AND_MASK		13
	/* May invoke Neg / Abs */
#define	VPUH_CALB_MODE_PASS_THROGH		13
	/* In << ShBits */
#define	VPUH_ALB_MODE_SH_UP			14
	/* In >> ShBits */
#define	VPUH_ALB_MODE_SH_DOWN			15
	/**
	 * In > ThrH ? (ValH : (In < ThrL ? ValL : Med))
	 * Med is In or Val Med
	 */
#define	VPUH_ALB_MODE_CLIP			16
	/* Comp(In0,In1) ? 1 : 0 where Comp defined by VPUH_ALB_COMP */
#define	VPUH_ALB_MODE_COMP			17
/*** Calb Only MODES **/
#define	VPUH_CALB_ONLY_1ST_MODE			VPUH_CALB_MODE_MULT0_SH
	/* In * W0 >> Sh */
#define	VPUH_CALB_MODE_MULT0_SH			20
	/* (In0 * W0 + In1 * W1) >> Sh */
#define	VPUH_CALB_MODE_MULT_ADD_SH		21
	/* In0 * In1 * W0 >> Sh */
#define	VPUH_CALB_MODE_MULT0_1_SH		22
	/* In * In * W0 >> Sh */
#define	VPUH_CALB_MODE_MULT0_SQR_SH		23
	/**
	 * In * W0 >> Sh like mode 20 but if In * W0 > MAX_VALUE extra bits are
	 * lost instead of setting MAX_VALUE
	 */
#define	VPUH_CALB_MODE_MULT0_NC_SH		24
	/**
	 * (In0 * In1 * W0 + W1) >> Sh - like mode 22 but adding W1
	 * to allow rounding
	 */
#define	VPUH_CALB_MODE_MULT01_R_SH		25
/* TODO(Evgeny) - add description for modes 26-29 */
	/**
	 * (In0 * In1 * W0 + W1) >> Sh - like mode 22 but adding W1
	 * to allow rounding
	 */
#define	VPUH_CALB_MODE_MULT_MAT_BY_VEC		26
	/**
	 * (In0 * In2 + In1 * W1) >> Sh - like mode 22 but adding W1
	 * to allow rounding
	 */
#define	VPUH_CALB_MODE_MULT_02_ADD_1		27
#define	VPUH_CALB_MODE_DIV_1_BY_0		28
#define	VPUH_CALB_MODE_ANOTHER_DIV		29
#define	VPUH_ALB_MODE_MASK				0x1f

/* VPUH_ALB_ABS_NEG values */
#define	VPUH_ALB_ABS_NEG_NONE			0
#define	VPUH_ALB_ABS_NEG_ABS			1
#define	VPUH_ALB_ABS_NEG_NEG			2
#define	VPUH_ALB_ABS_NEG_MASK				3

/* VPUH_ALB_CLIP_MED values */
	/* On mode VPUH_ALB_MODE_CLIP Med = VPUH_ALB_VAL_MED */
#define	VPUH_ALB_CLIP_MED_VAL_MED		0
	/* On mode VPUH_ALB_MODE_CLIP Med = In */
#define	VPUH_ALB_CLIP_MED_INPUT			1
#define	VPUH_ALB_CLIP_MED_MASK				1

/* VPUH_ALB_COMP values */
	/* On mode VPUH_ALB_MODE_COMP Compare function is in0 == in1 */
#define	VPUH_ALB_COMP_EQ			0
	/* On mode VPUH_ALB_MODE_COMP Compare function is in0 != in1 */
#define	VPUH_ALB_COMP_NE			1
	/* On mode VPUH_ALB_MODE_COMP Compare function is in0 > in1 */
#define	VPUH_ALB_COMP_GT			2
	/* On mode VPUH_ALB_MODE_COMP Compare function is in0 <= in1 */
#define	VPUH_ALB_COMP_LE			3
	/* On mode VPUH_ALB_MODE_COMP Compare function is in0 < in1 */
#define	VPUH_ALB_COMP_LT			4
	/* On mode VPUH_ALB_MODE_COMP Compare function is in0 >= in1 */
#define	VPUH_ALB_COMP_GE			5
#define	VPUH_ALB_COMP_MASK				7

/* VPUH_CALB_ROUND_MODE values */
	/* Nearest integer + 4.5->5.0, -4.5->-5.0, 3.5->4.0, -3.5->-4.0 */
#define	VPUH_CALB_ROUND_MODE_NEAR_BIG_ABS	0
	/* Nearest integer + 4.5->5.0, -4.5->-4.0, 3.5->4.0, -3.5->-3.0 */
#define	VPUH_CALB_ROUND_MODE_NEAR_BIGGER	1
	/* Nearest integer + 4.5->4.0, -4.5->-4.0, 3.5->4.0, -3.5->-4.0 */
#define	VPUH_CALB_ROUND_MODE_NEAR_EVEN		2
	/* 4.5->4.0, -4.5->-5.0 */
#define	VPUH_CALB_ROUND_MODE_FLOOR		3
	/* 4.5->4.0, -4.5->-4.0 */
#define	VPUH_CALB_ROUND_MODE_INT		4
#define	VPUH_CALB_ROUND_MODE_MASK			7

/* VPUH_FILTER_SIZE values SEPFL_5x5, SEPFL_7x7 and GENFL */
/* Not allowed for NMS */
#define	VPUH_FILTER_SIZE_1x1			0
#define	VPUH_FILTER_SIZE_3x3			1
#define	VPUH_FILTER_SIZE_5x5			2
/* Not allowed for SEPFL_5x5 */
#define	VPUH_FILTER_SIZE_7x7			3
/* Only for GENFL */
#define	VPUH_FILTER_SIZE_9x9			4
/* Only for GENFL */
#define	VPUH_FILTER_SIZE_11x11			5
#define	VPUH_FILTER_SIZE_MASK				7

/* VPUH_SEP_FILT_ADD_ZEROS values - SEPFL_5x5, SEPFL_7x7 */
#define	VPUH_SEP_FILT_ADD_ZEROS_NONE		0
	/* Insert Column and Line of 0 after each Column / Line */
#define	VPUH_SEP_FILT_ADD_ZEROS_BOTH		1
	/* Insert Column of 0 after each Column */
#define	VPUH_SEP_FILT_ADD_ZEROS_COLS		2
#define	VPUH_SEP_FILT_ADD_ZEROS_MASK			3

/* VPUH_SEP_FILT_WORK_MODE values */
#define	VPUH_SEP_FILT_WORK_MODE_NORMAL		0
#define	VPUH_SEP_FILT_WORK_MODE_FIFO		1
#define	VPUH_SEP_FILT_WORK_MODE_DUPLICATOR	2
	/* Allowed only for SEPFL5 */
#define	VPUH_SEP_FILT_WORK_MODE_MAX_POOL	3
#define	VPUH_SEP_FILT_WORK_MODE_MASK			3

/* VPUH_FILT_BORDER_FILL values */
#define	VPUH_FILT_BORDER_FILL_CONSTANT		0
	/* Not allowed for NLF and NMS */
#define	VPUH_FILT_BORDER_FILL_MIRROR		1
#define	VPUH_FILT_BORDER_FILL_REPLICATE		2
	/* On this case - total image size is reduced */
#define	VPUH_FILT_BORDER_FILL_NONE		3

/* VPUH_NMS_STRICT_COMP values - 1 bit for neighbor */
#define	VPUH_NMS_STRICT_COMP_L			0
#define	VPUH_NMS_STRICT_COMP_DL			1
#define	VPUH_NMS_STRICT_COMP_D			2
#define	VPUH_NMS_STRICT_COMP_DR			3
#define	VPUH_NMS_STRICT_COMP_R			4
#define	VPUH_NMS_STRICT_COMP_UR			5
#define	VPUH_NMS_STRICT_COMP_U			6
#define	VPUH_NMS_STRICT_COMP_UL			7
#define	VPUH_NMS_STRICT_COMP_ALL_MASK			0xff

/* VPUH_NLF_MODE values */
#define	VPUH_NLF_MODE_MIN			0
#define	VPUH_NLF_MODE_MAX			1
#define	VPUH_NLF_MODE_MED			2
#define	VPUH_NLF_MODE_FAST			3
#define	VPUH_NLF_MODE_CENSUS			4
/* Replace each pixel by the average of either pixels with near value
 * or pixels with far value
 */
#define	VPUH_NLF_MODE_SM_SMOOTH			5

/* VPUH_NLF_FAST_MODE values */
#define	VPUH_NLF_FAST_MODE_MIN_OF_MAX		0
#define	VPUH_NLF_FAST_MODE_MAX_OF_MIN		1
/**
 * VPUH_CENSUS_MODE values - defined the set of 8 pixels
 * to compare with central pixel in 5x5 filter
 */
/* all the 8 near neighbors */
#define	VPUH_CENSUS_MODE_INTERNAL		0
	/* the 4 diagonal near neighbors + 4 other neighbors on edges */
#define	VPUH_CENSUS_MODE_DIAG_INTERNAL		1
	/* the 4 near neighbors + 4 other on corners */
#define	VPUH_CENSUS_MODE_DIAG_EXTERNAL		2
	/* 2 neighbors of each corner */
#define	VPUH_CENSUS_MODE_NEAR_CORNERS		3

/* VPUH_NLF_NEIGHBORS values - 1 bit for each neighbor */
	/* Down Right */
#define	VPUH_NLF_NEIGHBORS_DR			0
	/* Down */
#define	VPUH_NLF_NEIGHBORS_D			1
	/* Down Left */
#define	VPUH_NLF_NEIGHBORS_DL			2
	/* Right */
#define	VPUH_NLF_NEIGHBORS_R			3
	/* Center */
#define	VPUH_NLF_NEIGHBORS_C			4
	/* Left */
#define	VPUH_NLF_NEIGHBORS_L			5
	/* Up Right */
#define	VPUH_NLF_NEIGHBORS_UR			6
	/* Up */
#define	VPUH_NLF_NEIGHBORS_U			7
	/* Up Left */
#define	VPUH_NLF_NEIGHBORS_UL			8
#define	VPUH_NLF_NEIGHBORS_ALL_MASK			0x1ff

/* VPUH_MDE_MODE values */
	/**
	 * Mag = Sqrt(In0^2 + In1^2), Angle = InvTan(In1 / In0).
	 * Angle : 0-180 translated to 0-128 (8 bits output)
	 */
#define	VPUH_MDE_MODE_MAG_ANGLE			0
	 /* Min Eigen Value & Max Eigen Values */
#define	VPUH_MDE_MODE_EIGEN_VALUES		1
	 /* Harris Response Function */
#define	VPUH_MDE_MODE_HRF			2
	 /* In0 + In1^2 */
#define	VPUH_MDE_MODE_ADD_SQR			3
	 /* In0^2 + In1^2 */
#define	VPUH_MDE_MODE_SUM_SQR			4
	 /* sqrt(in0+(1n1 << 16)) */
#define	VPUH_MDE_MODE_SQRT			5
	 /* Mag = |In0| + |In1|, Angle = InvTan(In1 / In0) */
#define	VPUH_MDE_MODE_L1_MAG_ANGLE		6
	 /* Mag = In0^2 + In1^2, Angle = InvTan(In1 / In0) */
#define	VPUH_MDE_MODE_SQR_MAG_ANGLE		7

/* VPUH_INTIMG_MODE values */
#define	VPUH_INTIMG_MODE_INT_IMG		0
#define	VPUH_INTIMG_MODE_INT_COLS		1
#define	VPUH_INTIMG_MODE_INT_ROWS		2
	/**
	 * Some pixels are marked as "objects".
	 * Find the distance to the nearest "object".
	 */
#define	VPUH_INTIMG_MODE_DIS_TRANS_MAN		3
#define	VPUH_INTIMG_MODE_DIS_TRANS_EUC		4
#define	VPUH_INTIMG_MODE_MIN_ENERGY		5
	/* Input includes FG and BG pixels. Label connected FG objects. */
#define	VPUH_INTIMG_MODE_CONNECTED_COMP		6
	/* Allow using up to 40K LUT */
#define	VPUH_INTIMG_MODE_LUT			7

/* VPUH_INTIMG_OVFLOW values */
#define	VPUH_INTIMG_OVFLOW_OUT_LSBS		0
#define	VPUH_INTIMG_OVFLOW_OUT_MAX_VAL		1

/* VPUH_INTIMG_CC_MODE values */
#define	VPUH_INTIMG_CC_MODE_FST_SCAN		0
#define	VPUH_INTIMG_CC_MODE_OTHER_SCAN		1
#define	VPUH_INTIMG_CC_MODE_RELABLE		2

/* VPUH_SC_MODE values (DWSC and UPSC) */
#define	VPUH_SC_MODE_BILINEAR			0
#define	VPUH_SC_MODE_AREA_NO_ROUND		1
#define	VPUH_SC_MODE_NEAREST_NEIGHBOR		2
#define	VPUH_SC_MODE_AREA_DO_ROUND		3

/* VPUH_UPSC_BORDER_FILL values */
#define	VPUH_UPSC_BORDER_FILL_CONSTANT		0
#define	VPUH_UPSC_BORDER_FILL_REPLICATE		1

/* VPUH_CROP_MODE values */
	/* Output image is smaller than input */
#define	VPUH_CROP_MODE_CROP			0
	/* Replace the outside values by pattern */
#define	VPUH_CROP_MODE_PAD_OUTSIDE		1
	/* Replace the inside values by pattern */
#define	VPUH_CROP_MODE_PAD_INSIDE		2
	/* Inside values - mask_value_in, Outside values - mask_value_out */
#define	VPUH_CROP_MODE_MASK				3

/* VPUH_LUT_MODE_SIZE values */
#define	VPUH_LUT_MODE_SIZE_256			0
#define	VPUH_LUT_MODE_SIZE_1024			1

/* VPUH_ROI_MODE values */
/* Output:Sum Gt*Gx, Sum Gt*Gy, Sum Gx*Gy, Sum Gx*Gx, Sum Gy*Gy, Sum Abs(Gt)*/
#define	VPUH_ROI_MODE_LUKAS_KANADA		0
/*
 * Output is : LSW(Sum (Pi^2)), MSW(Sum (Pi^2)), LSW(Sum(Pi)),
 * MSW(Sum (Pi)), Number of valid pixels
 */
#define	VPUH_ROI_MODE_ROI_STD			1
/* Output is : Number of valid pixels, Min coordinates,
 * Max coordinates (x in 16 lsbs), Min Value, Max Value
 */
#define	VPUH_ROI_MODE_ROI_MIN_MAX		2
/* VPUH_ROI_MIN_MAX values */
#define	VPUH_ROI_MIN_MAX_LAST			0
#define	VPUH_ROI_MIN_MAX_FST			1
/* VPUH_ROI_OUTPUT values for different modes */
/* Lukas Kanada results */
#define VPUH_ROI_OUT_LK_SUM_GT_GX_OFS		0
#define VPUH_ROI_OUT_LK_SUM_GT_GY_OFS		1
#define VPUH_ROI_OUT_LK_SUM_GX_GY_OFS		2
#define VPUH_ROI_OUT_LK_SUM_GX_GX_OFS		3
#define VPUH_ROI_OUT_LK_SUM_GY_GY_OFS		4
#define VPUH_ROI_OUT_LK_SUM_ABS_GT_OFS		5
/* ROI Std results */
#define VPUH_ROI_OUT_STD_SUM_SQR_LSB_OFS	0
#define VPUH_ROI_OUT_STD_SUM_SQR_MSB_OFS	1
#define VPUH_ROI_OUT_STD_SUM_LSB_OFS		2
#define VPUH_ROI_OUT_STD_SUM_MSB_OFS		3
#define VPUH_ROI_OUT_STD_SUM_NUM_VALID		4
/* ROI Min Max results. */
#define VPUH_ROI_OUT_MINMAX_CORDS_X_SH		0
#define VPUH_ROI_OUT_MINMAX_CORDS_Y_SH		16
#define VPUH_ROI_OUT_MINMAX_NUM_VALID		0
#define VPUH_ROI_OUT_MINMAX_MIN_CORDS		1
#define VPUH_ROI_OUT_MINMAX_MAX_CORDS		2
#define VPUH_ROI_OUT_MINMAX_MIN_VALUE		3
#define VPUH_ROI_OUT_MINMAX_MAX_VALUE		4

/* VPUH_FLAMORB_OUT values */
#define VPUH_FLAMORB_OUT_COMP_BIT			0
#define VPUH_FLAMORB_OUT_BOTH_PIXELS		1

/* 16 bits floating point - sign + 10 bits mantissa + 5 bits exponent */
#define VPUH_CNN_FORMAT_16F			0
/* 32 bits floating point - sign + 23 bits mantissa + 8 bits exponent */
#define VPUH_CNN_FORMAT_32F			1
/* 16 bits integer */
#define VPUH_CNN_FORMAT_16I			2
/* 32 bits integer */
#define VPUH_CNN_FORMAT_32I			3

#endif /* VS4L_FWIF_HW_GEN*/
