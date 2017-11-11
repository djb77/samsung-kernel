/**
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef VPU_FWIF_PROJ210_
#define VPU_FWIF_PROJ210_

/**
 * ************ General description - Explicit project information ************
 * This file includes explicit project information required for CORE / HOST
 * interface (e.g. number of instances of each block).
 * The HW information that is relevant for all projects and is required for
 * both writing the HW registers and for CORE / HOST interface is kept in
 * VPU_FWIF_HW_Gen.h
 */

#define VPUH_HW_CHAINS		6

#define VPUH_TOTAL_BLOCKS	79

#define VPUH_16BIT_OUTPUTS	78
#define VPUH_32BIT_OUTPUTS	26
	/**
	 * The single 80 bits output entry (of TN_4) isn't handled
	 * - fix connection (to TN_3)
	 */
#define VPUH_TOTAL_OUTPUTS	(VPUH_16BIT_OUTPUTS + VPUH_32BIT_OUTPUTS)
#define VPUH_16BIT_INPUTS	96
#define VPUH_32BIT_INPUTS	31
	/**
	 * The single 80 bits input entry (of TN_3) isn't handled
	 * - fix connection (from TN_4)
	 */
#define VPUH_TOTAL_INPUTS	(VPUH_16BIT_INPUTS + VPUH_32BIT_INPUTS)

#define VPUH_LARGE_MPRB_SIZE	4096
#define VPUH_SMALL_MPRB_SIZE	1024
	/**
	 * HW Indices are 1 - 24.
	 * Required to identify if MPRB block index is small or large.
	 */
#define VPUH_LARGE_MPRBS		24
	/* HW Indices are 24 - 47 */
#define VPUH_SMALL_MPRBS		23
#define VPUH_TOTAL_MPRBS		(VPUH_LARGE_MPRBS + VPUH_SMALL_MPRBS)
	/* Total number of connections of all HW blocks to MPRBs */
#define VPUH_HW_CONNECTIONS_MPRBS	250

#ifdef FPGA_PLATFORM
#define BL(Real, All)			Real
#if (FPGA_VERSION == 1)				/* Base */
#define V_BL(x1,x2,x3,x4,x5,All)	x1
#define LK_DP_BL(x13,x4,x5,All)		x13
#define LKDP_BL(x13,x45,All)		x13
#define DP_BL(x14,x5,All)		x14
#define CNN_BL(x12,x345,All)		x12
#elif (FPGA_VERSION == 2)			/* CNN */
#define V_BL(x1,x2,x3,x4,x5,All)	x2
#define LK_DP_BL(x13,x4,x5,All)		x13
#define LKDP_BL(x13,x45,All)		x13
#define DP_BL(x14,x5,All)		x14
#define CNN_BL(x12,x345,All)		x12
#elif (FPGA_VERSION == 3)			/* Harris + Fast + Canny */
#define V_BL(x1,x2,x3,x4,x5,All)	x3
#define LK_DP_BL(x13,x4,x5,All)		x13
#define LKDP_BL(x13,x45,All)		x13
#define DP_BL(x14,x5,All)		x14
#define CNN_BL(x12,x345,All)		x345
#elif (FPGA_VERSION == 4)			/* LK */
#define V_BL(x1,x2,x3,x4,x5,All)	x4
#define LK_DP_BL(x13,x4,x5,All)		x4
#define LKDP_BL(x13,x45,All)		x45
#define DP_BL(x14,x5,All)		x14
#define CNN_BL(x12,x345,All)		x345
#elif (FPGA_VERSION == 5)			/* Depth */
#define V_BL(x1,x2,x3,x4,x5,All)	x5
#define LK_DP_BL(x13,x4,x5,All)		x5
#define LKDP_BL(x13,x45,All)		x45
#define DP_BL(x14,x5,All)		x5
#define CNN_BL(x12,x345,All)		x345
#endif

#define VPUH_BL_INST_NUM {				\
	BL(1, 1),		/* VPUH_BL_I_DMA */	\
	BL(2, 2),		/* VPUH_BL_I_MNM_DMA */	\
	DP_BL(2, 1, 2),		/* VPUH_BL_WI_DMA */	\
	DP_BL(1, 0, 1),		/* VPUH_BL_O_DMA */	\
	DP_BL(2, 0, 2),		/* VPUH_BL_O_MNM_DMA */	\
	DP_BL(2, 1, 2),		/* VPUH_BL_WO_DMA */	\
	LK_DP_BL(2,6,1,12),	/* VPUH_BL_FIFO */	\
	BL(3, 3),		/* VPUH_BL_DUPL_16 */	\
	BL(2, 2),		/* VPUH_BL_DUPL_32 */	\
	BL(2, 2),		/* VPUH_BL_SPLIT */	\
	BL(2, 2),		/* VPUH_BL_JOIN */	\
	LK_DP_BL(3,7,1,9),	/* VPUH_BL_SALB */	\
	V_BL(1,1,3,3,0,3),	/* VPUH_BL_CALB */	\
	V_BL(1,1,2,3,0,3),	/* VPUH_BL_SEPFL_5x5 */	\
	V_BL(1,1,3,3,1,3),	/* VPUH_BL_SEPFL_7x7 */	\
	LK_DP_BL(1,2,0,2),	/* VPUH_BL_GENFL */	\
	V_BL(1,0,1,0,1,1),	/* VPUH_BL_NMS */	\
	V_BL(1,0,1,0,1,1),	/* VPUH_BL_NLF */	\
	V_BL(1,1,0,1,0,1),	/* VPUH_BL_CCM */	\
	LKDP_BL(1,0,1),		/* VPUH_BL_MDE */	\
	LKDP_BL(1,0,1),		/* VPUH_BL_INTIMG */	\
	CNN_BL(1,0,2),		/* VPUH_BL_DWSC */	\
	LKDP_BL(1,0,1),		/* VPUH_BL_UPSC */	\
	LKDP_BL(3,0,3),		/* VPUH_BL_CROP */	\
	LKDP_BL(1,0,1),		/* VPUH_BL_M2LI */	\
	CNN_BL(1,0,1),		/* VPUH_BL_LUT */	\
	CNN_BL(1,0,1),		/* VPUH_BL_HIST */	\
	V_BL(1,1,0,2,0,2),	/* VPUH_BL_ROI */	\
	CNN_BL(1,0,1),		/* VPUH_BL_FLAMORB */	\
	V_BL(0,1,0,0,0,1),	/* VPUH_BL_CNN */	\
	DP_BL(0, 1, 1),		/* VPUH_BL_DEPTH */	\
	DP_BL(0, 1, 1),		/* VPUH_BL_IN_PAINT */	\
	DP_BL(0, 1, 1),		/* VPUH_BL_DISP_EQ */	\
	BL(1, 1),		/* VPUH_BL_TN1_SBF */	\
	BL(2, 2),		/* VPUH_BL_TN2_ALF */	\
	BL(1, 1),		/* VPUH_BL_TN3_BRE */	\
	BL(1, 1),		/* VPUH_BL_TN4_MIC */	\
	BL(1, 1),		/* VPUH_BL_TN5_RCF */	\
	}
#else
#define VPUH_BL_INST_NUM {			\
	1,	/* VPUH_BL_I_DMA */		\
	2,	/* VPUH_BL_I_MNM_DMA */		\
	2,	/* VPUH_BL_WI_DMA */		\
	1,	/* VPUH_BL_O_DMA */		\
	2,	/* VPUH_BL_O_MNM_DMA */		\
	2,	/* VPUH_BL_WO_DMA */		\
	12,	/* VPUH_BL_FIFO */		\
	3,	/* VPUH_BL_DUPL_16 */		\
	2,	/* VPUH_BL_DUPL_32 */		\
	2,	/* VPUH_BL_SPLIT */		\
	2,	/* VPUH_BL_JOIN */		\
	9,	/* VPUH_BL_SALB */		\
	3,	/* VPUH_BL_CALB */		\
	3,	/* VPUH_BL_SEPFL_5x5 */		\
	3,	/* VPUH_BL_SEPFL_7x7 */		\
	2,	/* VPUH_BL_GENFL */		\
	1,	/* VPUH_BL_NMS */		\
	1,	/* VPUH_BL_NLF */		\
	1,	/* VPUH_BL_CCM */		\
	1,	/* VPUH_BL_MDE */		\
	1,	/* VPUH_BL_INTIMG */		\
	2,	/* VPUH_BL_DWSC */		\
	2,	/* VPUH_BL_UPSC */		\
	3,	/* VPUH_BL_CROP */		\
	1,	/* VPUH_BL_M2LI */		\
	1,	/* VPUH_BL_LUT */		\
	1,	/* VPUH_BL_HIST */		\
	2,	/* VPUH_BL_ROI */		\
	1,	/* VPUH_BL_FLAMORB */		\
	1,	/* VPUH_BL_CNN */		\
	1,	/* VPUH_BL_DEPTH */		\
	1,	/* VPUH_BL_IN_PAINT */		\
	1,	/* VPUH_BL_DISP_EQ */		\
	1,	/* VPUH_BL_TN1_SBF */		\
	2,	/* VPUH_BL_TN2_ALF */		\
	1,	/* VPUH_BL_TN3_BRE */		\
	1,	/* VPUH_BL_TN4_MIC */		\
	1,	/* VPUH_BL_TN5_RCF */		\
	}
#endif
#endif
