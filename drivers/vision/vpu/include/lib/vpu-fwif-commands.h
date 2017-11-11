/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of the
 * GNU General Public License version 2 as published by the Free Software Foundation.
 */

#ifndef __VPUL_FWIF_COMMANDS__
#define __VPUL_FWIF_COMMANDS__

/**
 * This file describe MailBox data structures.
 * The HOST sends to the CORE TASKs creation,allocation, update parameters and invocation requests.
 * (It also sends TASK abort, free, re-alloc and destroy commands and system init command).
 * Each TASK is a graph with processes / 3DNN processes as vertices,nested loops and data streaming
 * as edges. Some of the edges might be conditionals - the HOST should define which disable edges
 * conditions are active on each task invocation.
 * Remark : the symbol process(3D) is used here as either process or 3DNN process.
 * Each process(3D) is a sequence of HW & CPU sub-chains. HW operations are invocation of connected
 * HW blocks - each configured with explicit parameters for current operation - on one or more data
 * streams read from memory by DMAs. The CPU sub-chains are built from list of pre-defined simple
 * operations - e.g. copy statistics from registers to memory,create ROI list or check dynamic
 * condition for loop termination. Many processes(3D) include only HW operations. A process may
 * include only CPU sub-chains (on very special cases), 3DNN process always includes HW operations.
 * As will be described later in more details the process(3D) might be invoked in few levels of
 * loops (same operations for same or different inputs with optionally different parameters per
 * invocation). Few operations might be invoked before the 1st "top level" loop and few operations
 * might be invoked after the last "top level" loop.
 * On task creation the HOST describes in details the vertices, the edges and the process(3D).
 * On task allocation the HOST defines the maximal number of task instances that might be active
 * simultaneously. Each process has its own HW blocks and MPRBs allocation for each Task slot.
 * While a process(3D) is active the HW required for other processes(3D) in the task might be used
 * by other tasks or by other slots of the same task.
 * For each task invocation request that is sent by the HOST - the CORE creates invocation request
 * for the TASK "start" vertex. When vertex that isn't the TASK "end" vertex is finished the CORE
 * find from the task edges that are output of finished vertex the next vertices that should be
 * invoked. Notice that some vertices may require few inputs for invocation and might be invoked
 * from different vertices. On this case they will have expected input mask for each "group" of
 * input edges and they will be invoked only when all edges in the mask are ready. For each vertex
 * that should be invoked it is invoked if it might be invoked immediately-otherwise its invocation
 * request is sent to the queue (in case the required resources are not available).
 * When few processes(3D) race on HW resources the winner has the higher priority where priority is
 * defined both in task level and process level. When the racing processes have same priority the
 * older wins. We should consider supporting "abort" of process(3D) by other process(3D) in the
 * same task.
 * On each TASK invocation a full set of parameters is used. The default vector of parameters is
 * defined on task creation, some parameters are updated on allocation and each parameter might be
 * dynamically updated by the HOST. Optionally some of the parameters are explicitly updated on
 * each TASK invocation.
 * Some of the parameters are updated during vertex internal loops and some are updated during Task
 * loops. All the data that might be dynamically updated by the HOST is set as part of parameters
 * vector (that include disabled edges mask, disabled sub-chains, input and output image sizes,
 * memory line offsets and filter coefficients). The information that is updated on each task
 * invocation is "each invocation" parameters,mask of conditions that disable the conditional edges
 * and few dynamic memory addresses. Other memory addresses are derived from the current slot.
 *
 * Each TASK has lists of vertices, edges, edges groups(with masks), loops,
 * CPU sub chains, processes, 3DNN processes, sub-chains, HW sub-chains, HW3DNN
 * sub-chains, HW 3DNN Group Sub-chains, HW blocks, connections and parameters
 * (divided between Chains and Sub chains), all parameters values, parameters
 * indices for loops descriptions, MPRBs required for SRAM and HW block usage,
 * input & output memory addresses set on each invocation, internal memory
 * addresses in DRAM and SRAM (for each task slot), internal memory "blocks"
 * (DMA address and sequence of MPRBs), line offsets in memories, input image
 * sizes, image sizes operations (copy, Increase / Crop, Down / Up scale),
 * input XY sizes and indices (for 3DNN),base & 3DNN input / output types
 * description, ROIs list (might be loaded from DRAM or dynamically created
 * by CPU sub-chain), tiles, sizes, filters descriptions, dynamic and static
 * filters coefficients and 3DNN layers description.
 *
 * Each process is invoked for few (1 or more) input sets where each input set
 * includes few (1 or more) input & output types (e.g. 2 separate lists of ROIs
 * each taken from another map). Each input / output is read from "map ROI"
 * list and has its own sizes and map. The maps in "map ROI" list may include
 * single or few maps / pyramid levels.The ROIs might be the the entire map or
 * ROIs that are either pre-defined, calculated by CORE or read from DRAM
 * (on this case the map ROIs list must be read for each Task invocation as
 * part of pre-load operations). All the inputs of each inout type type are
 * written continuously in the "map ROI" list. Some of the inout types have
 * different "map ROI" for each input set, some might be updated once per X
 * input sets (e.g. each ROI in Map1 is compared to 4 ROIs in Map2) and some
 * might be fixed. The offsets to the 1st "map ROI" of each input type are
 * derived from VPUI_InOutType description.
 * ASSUMPTION : Total number of input sets must divides all NumInpSetsEachRoi
 * of all inout types without reminder. For that the process includes
 * InputSetsDivider field that divides NumInpSetsEachRoi of each
 * VPUI_InOutType. The actual total number of InputSets and the maximal number
 * of InputSets must be divided by this number.
 * For each input set an "inputs loop" might be invoked (for example each input
 * set may include 2 64 * 64 windows, for each a loop on all internal 16 * 16
 * windows is invoked).
 * In case of "input loop" some of the inout types might be fixed along the
 * loop(for example - 16*16 window is compared with all 16 * 16 windows in
 * 64*64 input ROI) but all the input types that are updated have same movement
 * along the loops.
 * ASSUMPTION : When input loop is performed all the inout types that are
 * updated along the loop have same behavior.
 * For each "inputs loop" inputs might be divided to tiles - optionally
 * different inputs might be divided by different tiles sizes but the total
 * number of tiles must be equal.
 * ASSUMPTION : The total number of tiles must be equal for all inout sets.
 * LIMITATION : for simplicity we assume that tile sizes are divided by scaler
 * ratios without reminder. As a result the output sizes of all tiles has same
 * dimensions and there is no fractional offset in 1st input to scalers.
 * Notice that the problem is more complicated in case of different input ROI
 * sizes that should be scaled to fix output size and even more complicated
 * assuming except the scaler we also have data reduction before & after
 * the scaler.
 * Some parameters might be updated according to current tile borders - each
 * tile location creates 4 bits - whether it has valid data in its left / right
 * / top / bottom edges.
 * For each tile an actual sizes list is created from sizes operation
 * description based on current input sizes. All external memory base
 * addresses are updated according to current tile location.
 * For each tile few (1 or more) parameters loops are performed. Some
 * operations might be invoked before the 1st parameters loop and some
 * operations might be invoked after the last loop. For the parameters loops
 * some parameters are updated for each loop, some parameters are updated
 * according to first loop / other loop and some of them are updated
 * according to first HW invocation (= first loop for first tile) / any other
 * HW invocation, some memory indices might be updated according to first loop
 * / not first loop and some other memory indices are updated according to last
 * loop / not last loop.
 * Each parameters loop requires HW explicit invocation by CORE.
 *
 * Each 3DNN process is invoked for few (1 or more) continuous layers from a
 * predefined list of layers (instead of dynamic input sets of process). Each
 * layer is invoked only once and may have its own HW & CPU Chain but usually
 * few layers share same HW & CPU Chain.
 * Each layer has its own XY sizes (and DMA XY sizes), total number of input
 * and output slices and their division to tiles, the input slices division to
 * groups, the number of coefficients, few predefined 3DNN inout types and some
 * CNN parameters (e.g.Relu, input factor). Each 3DNN inout includes IO type
 * (Input / Output /coefficients), base address and number of bytes per pixel.
 * Each 3DNN inout has its own DMA (coefficients are usually split between few
 * 3DNN inout types).
 * Each Layer might be divided to tiles in X*Y dimensions in a similar way to
 * 2D tiles division (notice that coefficients are not divided to tiles).
 * When input slices tiles are divided to groups - each group is either first,
 * middle,last or single. The process of different types of slices groups might
 * be different in few aspects- previous output result aren't used by first and
 * single slot, the number of slices in the last slot might be different from
 * other slots, different HW blocks and different HW connections might be used
 * (e.g. when previous output is used or when max polling is invoked on the
 * final CNN results only in last slot) and different parameters might be used.
 * For each slices group external memory addresses of input and coefficients
 * are updated. Each slices group requires HW explicit invocation by CORE.
 * As described - when new slices group type is set some HW connections and
 * parameters should be updated. When new layer is set some HW parameters and
 * some 3DNN process parameters are updated - the 3DNN process parameters are
 * those who replaced according to slices group type.
 *
 * OPEN ISSUE : Do we support "abort" process(3DNN)?
 * TODO - Optionally some different input memories for 1st loop / output
 * memories last loop
 * TODO - It seems that when using less inputs / outputs than max we will
 * always use the 1st (unless special cases where e.g. input2 is mask and
 * input 1 is disabled - on this case we will set 0 for the 2nd input index)
 */

/* VPUI_CMND_FW_VERSION_ID 1.0.17 delivered on 28/07/16, SVN version 61148
 * Updated on each FW delivery.
 */
#ifdef OLD_FWIF_IO
#define VPUI_CMND_FW_VERSION_ID		1018	/* 1.0.18 */
#else
#define VPUI_CMND_FW_VERSION_ID		1100	/* 1.1.00 */
#endif

/* VPUI_CMND_IF_VERSION_ID 1.0.14 delivered on on 28/07/16, SVN version 61148
 * Updated on each delivery when either current file, vpu-fwif-hw-bl-params.h
 * or vpu-fwif-cpu-params.h have any changes from previous delivery
 */
#ifdef OLD_FWIF_IO
#define VPUI_CMND_IF_VERSION_ID		1015	/* 1.0.15 */
#else
#define VPUI_CMND_IF_VERSION_ID		1100	/* 1.1.00 */
#endif

#include "vpu-fwif-mbox.h"

#define VPUI_INVALID_TASK_ID		0
enum VPUI_CmndErr {
	VPUI_CMND_NO_ERR		= 0,
	/* Unknown command type. */
	VPUI_CMND_UNKONWN_TYPE		= 1,
	/* The command includes invalid information (checked only when VERIFY_VALID_HOST_CMND) */
	VPUI_CMND_NOT_VALID		= 2,
	/* Init command can't update low priority queue size when it is not empty */
	VPUI_CMND_INIT_LOW_PR_NOT_EMPTY = 3,
	/* On init command can't update normal commands MBOX size */
	VPUI_CMND_INIT_CMND_MBOX_SIZE	= 4,
	/* On init command the total size required for MBOXes & tasks control is too big */
	VPUI_CMND_INIT_HEAP_TOT_SIZE	= 5,
	/* On init command there are created tasks not destroyed */
	VPUI_CMND_INIT_TASKS_EXIST	= 6,
	/* ON realloc low priority HOST MBOX - should invoke init command before */
	VPUI_CMND_CANT_ALLOCATE_MBOX	= 7,
	/* Can't create Task - already created */
	VPUI_CMND_TASK_ALREADY_CREATED	= 8,
	/* Can't create or allocate task or allocate Low Priority MBOX not enough memory */
	VPUI_CMND_NOT_ENOUGH_MEMORY	= 9,
	/* Can't create task-Max tasks (set on initialization) already exist */
	VPUI_CMND_MAX_TASKS_CREATED	= 10,
	/* Task Id must be != VPUI_INVALID_TASK_ID */
	VPUI_CMND_TASK_ID_NOT_VALID	= 11,
	/* Can't invoke command on task - not found */
	VPUI_CMND_TASK_ID_NOT_FOUND	= 12,
	/* Can't allocate task - already allocated */
	VPUI_CMND_TASK_ID_WAS_ALLOCTED	= 13,
	/* Can't invoke, abort or free task - not allocated */
	VPUI_CMND_TASK_ID_NOT_ALLOCTED	= 14,
	/* Can't invoke task - no free slot */
	VPUI_CMND_TASK_NO_FRRE_SLOT	= 15,
	/* Can't free or destroy task - there are active slots */
	VPUI_CMND_TASK_ID_ACTIVE	= 16
};

enum VPUI_CmndType {
	VPUI_CMND_INIT			= 0,
	VPUI_CMND_REALLOC_LOWPR_CMD_MBX	= 1,
	VPUI_CMND_CREATE_TASK		= 2,
	VPUI_CMND_DESTROY_TASK		= 3,
	VPUI_CMND_ALLOCATE_TASK		= 4,
	VPUI_CMND_REALLOC_TASK		= 5,
	VPUI_CMND_FREE_TASK		= 6,
	VPUI_CMND_UPDATE_TASK_PARAMS	= 7,
	VPUI_CMND_INVOKE_TASK		= 8,
	VPUI_CMND_ABORT_TASK		= 9
};

#define VPUI_DEF_HW_TIME_OUT		500
	/* Default Maximal HW running time is 500 Milli Seconds */
#define VPUI_DEF_PROCESS_TIME_OUT	5000
	/* Default Maximal Process running time is 5 Seconds */
#define VPUI_DEF_TASK_TIME_OUT		10000
	/* Default Maximal Task running time is 10 Seconds */

struct VPUI_TimeInfo {
	/* The input ISP clock. Core Clock is ISP clock / 2 */
	__u16	IspClkMhz;
	/* If set to 0 - VPUI_DEF_HW_TIME_OUT is used */
	__u16	HwTimeOutMilliSec;
	/* If set to 0 - VPUI_DEF_PROCESS_TIME_OUT is used */
	__u16	ProcessTimeOutMilliSec;
	/* If set to 0 - VPUI_DEF_TASK_TIME_OUT is used */
	__u16	TaskTimeOutMilliSec;
};

struct VPUI_MemInfo {
	__u16	MaxTasks;
	/* Maximal number of process vertices that might be kept - waiting for available HW
	 * resources to be processed
	 */
	__u16  QuProcVertices;
	/* Maximal number of control vertices (TVER_ST / TVER_END / TVER_HOST_REP / TVER_DUMMY)
	 * that might be kept in the queue
	 */
	__u16  QuControlVertices;
	/* When memory slot is allocated try to avoid (if possible) keeping free memory that is
	 * less than MinimalAllocSize and bigger than MaximalValidUnusedSize.
	 * If MinimalAllocSize is set to 0 - no limitations on remaining free memory size
	 */
	__u16	MinimalAllocSize;
	 __u16	MaximalValidUnusedSize;
};

/****************** Initialziation description ****************************************************
 * System may re-initialize only when no task created (or all all created tasks where destroyed)
 */
struct VPUI_InitCmnd {
	struct	VPUI_TimeInfo			Time;
	struct	VPUI_MemInfo			Mem;
	struct	VPUM_MboxInitCmndInfo	Mbox;
};

struct VPUI_ReAllocMbox {
	__u16	NewSize16b;	/* The Low priority commands MBOX new size in 16 bits units */
};

#define GET_BITS_VALUE(Value, Name) (((Value) >> Name##_LSB) & Name##_MASK)
#define GET_BITS_VALUE_EXP(Value, Lsb, Mask) (((Value) >> Lsb) & Mask)

/* Bit fields for WorkAreaInd field */
#define CPU_OP_WORK_AREA_IN_FST_IND_LSB		0
	/* Index of 1st input in Working Area */
#define CPU_OP_WORK_AREA_OUT_FST_IND_LSB	4
	/* Index of 1st output in Working Area */
#define CPU_OP_WORK_AREA_IND_MASK				0xf
	/* The size of the Working Area is 12 */

/**************** CPU operation description ******************************************************/
/* CPU operation are invoked by CPU Sub chain. */
struct VPUI_CpuOp {
	__u8 OpCode;		/* enum VPUC_CpuOpCode */
	__u8 WorkAreaInd;	/* CPU_OP_WORK_AREA_IN/OUT_FST_IND_LSB */
	__u16 FstParamOffset;	/* From the sub-chain 1st parameter */
};

/**************** CPU Sub chains description *****************************************************/
/* CPU sub-chains are invoked by Processes and 3DNN processes. */
struct VPUI_CpuSubCh {
	__u8 NumCpuOp;
	__u8 FstCpuOpOffset;	/* From the process 1st CPU operations */
	__u8 IntRamIndices;	/* Total number of internal RAMs used */
	__u8 FstIntRamInd;	/* From the process internal RAMs */
	__u16 FstParamOffset;	/* From the process 1st parameter */
};

/*********** HW Sub-Chain components and structure description ***********************************/
/************** General block usage description **************************************************/
/* Each block usage is part of specific sub-chain */
/* Bit fields for TypeAndParams field */
#define HW_BL_TYPE_LSB		0	/* VPUH_Blocks */
#define HW_BL_TYPE_MASK			0x3f	/* Assuming up to 64 types */
#define HW_BL_PARAMS_OFS_LSB	6
	/* Offset of block first parameter from sub chain start */
#define HW_BL_PARAMS_OFS_MASK		0x3ff
	/* up to 1024 parameters per all sub chain blocks are supported */
/* Bit fields for ConnectAndInSizes */
#define HW_BL_CONNECT_OFS_LSB	0
	/* Offset of block 1st connection index from sub chain start */
#define HW_BL_CONNECT_OFS_MASK		0x7f
	/* up to 128 connections per sub-chain are supported */
#define HW_BL_CONNECT_NUM_LSB	7
	/* total Number of block input connections */
#define HW_BL_CONNECT_NUM_MASK		0x7	/* Up to 7 inputs per block */
#define HW_BL_IN_SIZES_IND_LSB	10
	/* The index of the input sizes to the block in the entire process input sizes vector */
#define HW_BL_IN_SIZES_IND_MASK		0x1f
	/* Up to 32 different sizes are supported for each process (VPUI_MAX_SIZES_PER_PROC==32) */

struct VPUI_BlUsage {
	__u16	TypeAndParams;		/* HW_BL_TYPE + HW_BL_PARAMS_OFS */
	__u16	ConnectAndInSizes;
		/* HW_BL_CONNECT_OFS + HW_BL_CONNECT_NUM + HW_BL_IN_SIZES_IND*/
};

struct VPUI_HWConnect {
	/* The index + 1 in the HW subchain of the block that is the source of the connection
	 * (0 stands for disabled connection)
	 */
	__u8	SrcBlIndP1;
	/* The output port Index of the source block */
	__u8	OutPortInd;
};

struct VPUI_MprbGr {
	/* The id of the 1st MPRB in the group. 0 stands for group of unused ports */
	__u8	FstMprbId;
	/* Number of MPRBs (with consecutive Ids) in the group */
	__u8	MprbsNum;
};

/****************** Internal RAM description *****************************************************/
#define VPUI_NUM_IN_RAM_VIRTUAL_UNITS	128
	/* Seems to be more than enough - notice - 1st 64 units on AXI0,
	 * next 64 units on AXI1 - each AXI has 64 bits access size
	 */
#define VPUI_IN_RAM_VIRTUAL_UNIT_SIZE\
	(2048 * 1024 / VPUI_NUM_IN_RAM_VIRTUAL_UNITS)
	/* Total virtual address space is 2M => 16K per unit */

/**
 * Same Internal RAM might be used by few sub-chains. We use vector of indices so each sub-chain
 * access continuous part of the indices vector. The process has offset to the vector of task
 * VPUI_InternalRam
 */

struct VPUI_InternalRam {
	/* offset to the 1st MPRB indices group of internal RAM from the process offset to the
	 * vector of MPRB indices group for Rams
	 */
	__u16 FstMprbGrOfs;
	/* Number of MPRBs groups */
	__u8  MprbsGrNum;
	/* Each unit size is VPUI_IN_RAM_VIRTUAL_UNIT_SIZE */
	__u8  VirtualUnitsNum;
};

/*************** Filter coefficients sets description ********************************************/
#define FL_EACH_DIM_SIZE_LSB		0
	/* Each dimension size. Separable filter has FL_EACH_DIM_SIZE vertical coefficients and
	 * than FL_EACH_DIM_SIZE horizontal coefficients while General filter has
	 * FL_EACH_DIM_SIZE * FL_EACH_DIM_SIZE coefficients (for each filter in case of 2 filters)
	 */
#define FL_EACH_DIM_SIZE_MASK			0xf
	/* Maximal separable size is 7 and maximal general size is 11 */
#define FL_IS_GEN_2FILTERS_LSB		4
	/* 2 filters are allowed only to general filter and up to 7x7 size */
#define FL_IS_GEN_2FILTERS_MASK			0x1
#define	FL_COEF_FRAC_BITS_LSB		5
	/* Number of coefficients fraction bits.
	 * In separable filter the result is Sum(Input*VerFactor*HorFactor) >> (FracBits*2).
	 * In General filter the result is Sum(Input*Factor) >> FracBits.
	 */
#define	FL_COEF_FRAC_BITS_MASK			0x1f
	/* 0-15 for separable, 0-31 for general */
#define	FL_GEN_COEF_FRAC_SIGN_LSB	10
	/* Relevant only for general - Separable coeffs are always signed */
#define	FL_GEN_COEF_FRAC_SIGN_MASK		0x1
#define FL_IS_DYN_LSB			11
	/* 1 if filter coefficients are part of dynamic vector (part of parameters vector),
	 * 0 for static vector
	 */
#define FL_IS_DYN_MASK				0x1

struct VPUI_FiltCoeffsSets {
	/* FL_EACH_DIM_SIZE +FL_COEF_FRAC_BITS +FL_COEF_FRAC_SIGN +FL_IS_GEN_2FILTERS +FL_IS_DYN */
	__u16 FlBits;
	/* Offsets of the 1st coefficient of the filter in the coefficients vecto (either static or
	 * dynamic vector)
	 */
	__u16 FstCoeffOfs;
};

/***************** Disparity parameters description **********************************************/
/***************** Histogram parameters description **********************************************/
struct VPUI_HistParams {
	/* Hist Index = (Input - Offset) / BinSize. Input values below Offset are ignored */
	__s16 Offset;
	__u16 MaxValue;		/* input values above the max value are ignored. */
	__u16 InvBinSizeM1;	/* 2^16 / BinSize - 1 */
};

/***************** Disparity parameters description **********************************************/
/* Thresholds that are expected to be equal for all sub chains invocations. We allow more than one
 * set in case an error occurs and few thresholds (one or more) should get different values between
 * sub-chains
 */

#define DCU_FCFG_RL_BORDERS_TRIM_LSB	0	/* RlBordersTrim */
#define DCU_FCFG_RL_BORDERS_TRIM_MASK		3
#define DCU_FCFG_INFO_THR_IS_INFRM_LSB	2	/* InfoThrIsInformative */
#define DCU_FCFG_INFO_THR_IS_INFRM_MASK		0x3f

#define DPU_CFG_USE_REF_EN_LSB		0	/* UseRefEn */
#define DPU_CFG_USE_REF_EN_MASK			1
#define DPU_CFG_REF_CANDIDATE_EN_LSB	1	/* RefCandidateEn */
#define DPU_CFG_REF_CANDIDATE_EN_MASK		1
#define DPU_CFG_PRIORITIZATION_EN_LSB	2	/* RefDsprPrioritizationEn */
#define DPU_CFG_PRIORITIZATION_EN_MASK		1
#define DPU_CFG_REF_DSPR_UP_SH_LSB	3	/* RefDsprUpShift */
#define DPU_CFG_REF_DSPR_UP_SH_MASK		3
#define DPU_CFG_PRE_OUTPUT_UP_SH_LSB	5	/* PreOutputDsprUpShift */
#define DPU_CFG_PRE_OUTPUT_UP_SH_MASK		7

#define SUPERB_CFG_COST_FACTOR_LSB	0	/* SuperbDsprConstantCostFactor */
#define SUPERB_CFG_COST_FACTOR_MASK		7
#define SUPERB_CFG_PR_NON_INFO_ONLY_LSB	3	/* PrioritizeSuperbOnNonInfoOnly */
#define SUPERB_CFG_PR_NON_INFO_ONLY_MASK	1
#define SUPERB_CFG_DSPR_PR_EN_LSB	4	/* SuperbDsprPrioritizationEn */
#define SUPERB_CFG_DSPR_PR_EN_MASK		1

#define INP_FCFG_RL_VALIDATION_EN_LSB	0	/* RlValidationEn */
#define INP_FCFG_RL_VALIDATION_EN_MASK		1
#define INP_FCFG_ENHANCED_INFO_EN_LSB	1	/* EnhancedInfoEn */
#define INP_FCFG_ENHANCED_INFO_EN_MASK		1
#define INP_FCFG_CONTENT_MASK_EN_LSB	2	/* ContentMaskEn */
#define INP_FCFG_CONTENT_MASK_EN_MASK		1
#define INP_FCFG_X_LAST_LOGIC_EN_LSB	3	/* XLastLogicEn */
#define INP_FCFG_X_LAST_LOGIC_EN_MASK		1
#define INP_FCFG_HALF_STRONG_EN_LSB	4	/* HalfStrongEn */
#define INP_FCFG_HALF_STRONG_EN_MASK		1
#define INP_FCFG_UPDATE_INP_COST_EN_LSB	5	/* UpdateInpaintedCostEn */
#define INP_FCFG_UPDATE_INP_COST_EN_MASK	1

#define ANC_TYSEL_MAX_SUPPORT_LSB	0	/* AnchorTypeSelMaxSupport */
#define ANC_TYSEL_MAX_SUPPORT_MASK		3
#define ANC_TYSEL_MAX_NEAR_SUPPORT_LSB	2	/* AnchorTypeSelMaxNearSupport */
#define ANC_TYSEL_MAX_NEAR_SUPPORT_MASK		3
#define ANC_TYSEL_MIN_VALID_LSB		4	/* AnchorTypeSelMinValid */
#define ANC_TYSEL_MIN_VALID_MASK		7
#define ANC_TYSEL_MIN_NEAR_VALID_LSB	7	/* AnchorTypeSelMinNearValid */
#define ANC_TYSEL_MIN_NEAR_VALID_MASK		7

#define DEQ_MCNT_BILATERAL_MAX_EN_LSB	0	/* McntBilateralMaximizationEn */
#define DEQ_MCNT_BILATERAL_MAX_EN_MASK		1
#define DEQ_MCNT_CENTER_ONLY_EN_LSB	1	/* McntUseCenterOnlyEn */
#define DEQ_MCNT_CENTER_ONLY_EN_MASK		1
#define DEQ_DSPR_SH_DN_BEF_VALID_LSB	2	/* DsprShiftDownBeforeValidation */
#define DEQ_DSPR_SH_DN_BEF_VALID_MASK		3
#define DEQ_MCNT_MIN_MULT_DSPR_LSB	4	/* McntMinForMultipleDspr */
#define DEQ_MCNT_MIN_MULT_DSPR_MASK		7

struct VPUI_DisparityFixedThr {
	/* Parameters for Depth block */
	__u8 GrayDiffThrIsNearToCenter;
	__u8 GrayDiffThrIsEqualToCenter;
	__u16 AggregatedNeighborSelection;
	__u8 GrayDiffThrDoAggregation;
	__u8 DcuFixCfgBits;	/* DCU_FCFG_* fileds */
	__u8 CostFractionBits;
	__u8 CostMaxGoodForCount;
	__u8 CostCExpWeights[8];
	__u8 DsprDiffThreshIsEqual;
	__u8 RefCostThreshUseRefDsp;
	__u8 DsprDiffMaxNeighborIsEqual;
	__u8 DpuCfgBits;	/* DPU_CFG_* fileds */
	__u8 SuperbCfgBits;	/* SUPERB_CFG_* fileds */
	__u8 CostMaxGoodForSuperb;
	__u8 SuperbDsprConstantCostValue;
	__u8 McntSelectedIterations0_3;	/* Bit for each */
	__u16 LfsrSeeds[4];
	/* Parameters for InPaint block */
	__u8 InpFixCfgBits;	/* INP_FCFG_* fileds */
	__u8 GrayMaxSimilarity;
	__u8 GrayStepPenalty;
	__u8 GrayMaxTotalError;
	__u8 McntMinMaxSingle;		/* 4 LSBs for Min, 4 MSBs for Max */
	__u8 McntMinMaxForStatMask;	/* 4 LSBs for Min, 4 MSBs for Max */
	__u8 DsprMaxUpperSimilarity;
	__u8 DsprMaxAnchorsGradVal;
	__u8 DsprMaxDiffToCenter;
	__u8 DownScaledCostMinPerIndex[3];
	__u16 AnchorTypeSelectBits;	/* ANC_TYSL_* fileds */
	__u16 AnchorLogimTrim_0_0;
	__u16 AnchorLogimTrim_0_1;
	__u16 XLastLogicMaxEdgeDiff;
	__u8 CostMinIsHidden;
	__u8 CostMaxForStatMask;
	__u16 AnchorDistMaxForStrong;
	__u16 BackgroundAnchorMargins[4];
	__u8 OutputMode;	/* 2 bits */
	/* Parameters for DispEq block */
	__u8 DispEqBits;	/* DEQ_* fields */
	__u8 CostMinForValidForSmallDspr;
	__u8 CostMinForValidForLargeDspr;
	__u8 CostMinDiffForUpdate;
	__u8 WeightForCostUpdate;
};

/* Dynamic thresholds might be either pre-defined or explicitly set for each invocation and updated
 * by either CPU or HOST based on previous results. At the end of Depth and InPaint blocks
 * parameters there is optionally a copy of VPUI_DepthDynThr / VPUI_InpaintDynThr structure
 */
#define DCU_DCFG_PATCH_SIZE_MODE_LSB	0	/* PatchSizeMode */
#define DCU_DCFG_PATCH_SIZE_MODE_MASK		3
#define DCU_DCFG_MIN_PIXELS_COST_LSB	2	/* MinPixelsForCost */
#define DCU_DCFG_MIN_PIXELS_COST_MASK		0x3f

struct VPUI_DepthDynThr {
	__u8 GrayDiffThrIsEqualPixels;
	__u8 GrayDiffThrIsNearPixels;
	__u8 DcuDynCfgBits;	/* DCU_DCFG_* fileds */
	__u8 RefDsprConstantCostValue;
	__u8 CostMaxGoodMatch;
	__u8 RefDsprConstantCostFactor; /* 3 bits */
	__u8 SuperbCandidateEn; /* 1 bit */
	__u8 GrayDiffMaxIsLikeSuperb;
};

#define INP_DCFG_BACKGROUND_EN_LSB	0	/* BackgroundEn */
#define INP_DCFG_BACKGROUND_EN_MASK		1
#define INP_DCFG_STR_ANTI_LEAK_EN_LSB	1	/* StrongAntiLeakEn */
#define INP_DCFG_STR_ANTI_LEAK_EN_MASK		1
#define INP_DCFG_BG_ANTI_LEAK_EN_LSB	2	/* BackgroundAntiLeakEn */
#define INP_DCFG_BG_ANTI_LEAK_EN_MASK		1

struct VPUI_InPaintDynThr {
	/* Parameters for InPaint block */
	__u8 InpDynCfgBits;	/* INP_DCFG_* fileds */
	__u8 GrayMaxSimilarity;
	__u8 GrayMaxHighSimilarity;
	__u8 DsprMaxSimilarity;
	__u8 DsprMaxAnchorsSimilarity;
	__u8 CostMinFillHoles;
	__u8 CostMaxReliable;
	__u8 CostMaxForAnchor;
	__u8 BackgroundGrayMaxSimilarity;
	__u8 DummyAlign;
	__u16 HiddenLogicTrim[2];
};

#define HW_SUB_CH_INT_RAM_IND_LSB	0
	/* Total number of internal RAMs used - index for each */
#define HW_SUB_CH_INT_RAM_IND_MASK		0xf
#define HW_SUB_CH_READ_RES_BL_LSB	4
	/* Total number of blocks HW results should be read from */
#define HW_SUB_CH_READ_RES_BL_MASK		0xf

/***************** HW Sub Chain description ******************************************************/
struct VPUI_HwSubChTotals {
	/* Total number of HW blocks in the Sub Chain VPUI_BlUsage information for each HW block */
	__u8  Blocks;
	/* Total number of HW connection (sources of HW block inputs) VPUI_HWConnect information
	 * for each HW connection
	 */
	__u8  HwConnections;
	/* HW_SUB_CH_INT_RAM_IND + HW_SUB_CH_READ_RES_BL */
	__u8  IntRamReadResBits;
	/* Total number of HW MPRB groups (VPUI_MprbGr for each) */
	__u8  HwMprbsGr;
};

struct VPUI_HwSubChFstBlInd {
	/* Index of the 1st Block (VPUI_BlUsage) */
	__u16 Block;
	/* Offset to 1st index of the block (in sub-chain blocks array) HW result are read from */
	__u16 HwReadResBl;
	/* Offset of the 1st parameter value of the 1st Block (uint16) from process parameters */
	__u16 Params;
	/* Index of 1st HW connection (VPUI_HWConnect) */
	__u16	HwConnections;
};

struct VPUI_HwSubChFstOtherInd {
	/* Index of 1st HW MPRB group - HW blocks MPRBs groups offsets are part of block parameters
	 * for blocks that are using MPRBs
	 */
	__u16	HwMprbsGr;
	/* Index of 1st internal Ram index (uint8) */
	__u8	IntRamInd;
	__u8	DummyAlign;
};

/**
 * The VPUI_HwSubChFstIndOfs is used in Sub Chain level for offsets of the sub-chain from the
 * Process and in process level for offsets of the process from the Task
 */
struct VPUI_HwSubChFstIndOfs {
	struct VPUI_HwSubChFstBlInd	Bl;
	struct VPUI_HwSubChFstOtherInd	Other;
};

/**
 * The HW Sub Chain should get process pointers of
 * VPUI_BlUsage + parameters + VPUI_HWConnect + HwMprbsGr + internal RAM
 * indices (which are updated by the offsets in VPUI_HwSubChFstIndOfs) and
 * pointers of internal RAMs description + RAM MPRBs Gr + filters sets +filters
 * static and dynamic coefficients (used as is).
 * It should also get pointers for the dynamic memory vectors of the process
 * - sizes, and memories description - addresses, line offsets and pixels
 * bytes. These vectors are updated per each new ROIs set / Tile / Input loop.
 * They are accessed by blocks parameters by indices.
 */
struct VPUI_HwSubCh {
	struct VPUI_HwSubChTotals	SubChTotals;
	struct VPUI_HwSubChFstIndOfs	SubChFstIndOfs;
};

/*********** HW 3DNN Sub-Chain per slices group type description *************/
struct VPUI_Hw3DNNSlGrTypeSubCh {
	struct VPUI_HwSubChTotals	Totals;
	struct VPUI_HwSubChFstBlInd FstBlInd;
};

enum VPUI_3DNNSlGrType {
	VPUI_3D_SLGR_FIRST	= 0,
	VPUI_3D_SLGR_MIDDLE	= 1,
	VPUI_3D_SLGR_LAST	= 2,
	VPUI_3D_SLGR_SINGLE	= 3,
	VPUI_3D_SLGR_ALL	= 4
};

/**
 * ************* Hw 3DNN Sub-chain description *******************************
 * The HW 3DNN Sub-Chain should get 3DNN process pointers of
 * VPUI_BlUsage + parameters(+ offsets in VPUI_HwSubChFstBlInd),
 * pointers of VPUI_HWConnect + HwMprbsGr + Internal RAM MPRBs Gr (which are
 * updated by the offsets in VPUI_HwSubChFstOtherInd) and pointers of
 * VPUI_Hw3DNNSlGrTypeSubCh + filters sets + static and dynamic filters
 * coefficients and VPUI_Hw3DNNSlGrTypeSubCh (used as is).
 * It should also get pointers for the dynamic memory vectors of the 3DNN
 * process - XY Sizes and VPUI_InOut3DNN description - addresses sizes and
 * pixels bytes. These vectors are updated per each new Layer and each new
 * slice group. They are accessed by blocks parameters by indices.
 */
struct VPUI_Hw3DNNSubCh {
	/**
	 * Equal values for single / 1st / middle / last slice group. Notice
	 * that different HW connections might be use - they will be described
	 * by different indices in VPUI_BlUsage level
	 */
	struct VPUI_HwSubChFstOtherInd	OtherOfs;
	/**
	 * Since values in VPUI_Hw3DNNSlGrTypeSubCh might be different
	 * for first / middle / last / single slices group there is a vector of
	 * VPUI_Hw3DNNSlGrTypeSubCh and offset to this vector for
	 * first / middle / last / single slice group.
	 */
	__u8  SlGrInd[VPUI_3D_SLGR_ALL];
	      /* The indices for First / Middle / Last / Single */
};

enum VPUI_SubChType {
	/* VPUI_HwSubCh for VPUI_ProcDesc,
	 * VPUI_Hw3DNNSubCh for VPUI_Proc3DNNBase
	 */
	VPUI_SUB_CH_HW			= 0,
	VPUI_SUB_CH_HW_WITH_CPU	= 1,
	VPUI_SUB_CH_CPU_HIGH_PR = 2,
	VPUI_SUB_CH_CPU_LOW_PR	= 3
	/* VPUI_SUB_CH_DSP		= 4 */
};

/******************* Sub Chain description ***********************************/
struct VPUI_SubCh {
	/* sub-chain type - enum VPUI_SubChType */
	__u8  Type;
	/* The index of disable bit in the disabled sub chains mask + 1.
	* 0 stands for non conditional sub chain.
	* We may disable pre-load sub-chains or some CPU sub-chains in
	* all process operations or allow it only on its 1st invocation
	*/
	__u8  DisableBitIndP1;
	/* Index in HW(3DNN) sub-chains vector of current Process / 3DNN Process
	*  Relevant only for VPUI_SUB_CH_HW and VPUI_SUB_CH_HW_WITH_CPU
	*/
	__u8 HwIndex;
	/* Index in CPU sub-chain vector of current Process / 3DNN Process
	*  Not relevant for VPUI_SUB_CH_HW
	*/
	__u8 CpuIndex;
};

#ifdef OLD_FWIF_IO
/**
 * ***************Input & output description *********************************
 * Below there is description of all the data types require to describe the
 * input and output of the process.
 * The process is invoked for few (>=1) input sets optionally divided to tiles
 * optionally with input loop. Each input set have few input & output types.
 * For each input set single ROI for each input & output type is used.
 * Optionally before the input sets are invoked pre-load operations might be
 * required. These operations might include loading of the ROIs.
 */

struct VPUI_Sizes {
	__u16 Width;
	__u16 Height;
};

/***************** Input loop description ************************************/
struct VPUI_InpLoop {
	/**
	 * Assuming a single loop for all input types.
	 * Relevant only when PROC_IS_INPUT_LOOP is set
	 */
	struct VPUI_Sizes LoopSizes; /* Actual sizes of ROI in each loop */
	__u16 LoopOfsX;
	      /**
	       * In each loop - horizontal loops are performed until
	       * CrntOfsX + HorLoopSize > ROI Width
	       */
	__u16 LoopOfsY;
	      /**
	       * In each loop - vertical loops are performed until
	       * CrntOfsX + HorLoopSize > ROI Width
	       */
};

/**
 * ************************************ Tile description ******************************************
 * Tiles division is used for 2 reasons -internal memory is used as a temporary storage which can't
 * include the entire image or blocks are using lines buffers that can't include the entire image
 * columns.Tile description has optional limitations for both maximal number of samples and maximal
 * number of columns. It also describes whether the division is to horizontal or vertical stripes.
 * Tiles overlap is required when filters are part of process invoked per tiles. On this case some
 * columns (and lines) before the tile and some columns after the tiles are required as input. On
 * this case the tile size is cropped along the process chain of the tile and between each 2 tiles
 * there is an overlap (the formula for overlap calculation is described later).When overlap exists
 * optionally a minimal size of the short dimension might be set in order to avoid the overlap part
 * from being too expensive.
 * Some tiles might be used as source to chain that includes (down or up) scaler. On this case the
 * source of the scaler must be divided by its denominator without reminder for any tile that isn't
 * the last.In order to force such limitation a tile description includes values that the number of
 * columns / lines (optionally- after crop that is performed before the scaler) must divide without
 * reminder. For simplicity the assumption here that tiles will be performed only on chains with
 * pre-defined scalers : chains that scale dynamic input sizes to fixed sizes (=> dynamic scale)
 * won't be divided by tiles.
 * The offset between each 2 middle tiles should be set to TileSize - CropBefDs - CropAftDs*DsRatio
 * (for each dimension). For example in case of TileSize = 80 that is an input to filter that crops
 * 8 columns (4 each side),the 72 columns are sent to Down Scaler that performs 3:2 scaling and the
 * 48 output columns are sent to filter that crops 4 columns.On this case the offset will be set to
 * 80 - 8 - (4x3/2) = 66 (on this case the overlap on each side will be 14).
 * In case of crop that is performed after scaler the crop size must divide the denominator of the
 * scaler (in the example above we can't invoke 4:3 scaling for example). If such Scaler + cropper
 * are required they must be split between different chains.
 * In case crop is performed on the tile the crop might be either performed or not on the edges. It
 * is correct for both crop that is performed before / after Scale.If crop isn't performed on edges
 * the 1st tile requires special handling- its size and next tile's offset are shorter (in order to
 * match other tiles). On the example above if both crops are not performed on edges the 1st tile
 * size is (44 + 4/2) * 3/2 + 8/2 = 73 (or 80 - 8/2 - 4/2*3/2 = 73) => fst offset = 73-14=59. When
 * all crops are not performed on edges 1st tile size = Middle tile size - overlap / 2. Fst offset
 * is always 1st tile size - Overlap
 * Notice that if Crop after Scale exists and it is not performed on edges half crop size must be
 * divided by the denominator of the scaler(in the example described above can't invoke 5:4 scaling
 * in case the crop after scaler isn't invoked on edges).
 */

struct VPUI_TileAxis {
	/* Number of elements that are used for both each couple of neighbor tiles
	 * Offset between tile N+1 to tile N is MiddleTotal - Overlap (FstTotal - Overlap for 1st
	 */
	__u8 Overlap;
	/* Number that total number of elements - CropBefDivider must divide without reminder.
	 * Should be set to denominator of Scale ratio in tile chain. 1 if no scaler in the chain
	 */
	__u8 Divider;
	/* Number that is reduced from total number of elements before divided by divider above */
	__u8 CropBefDivider;
	/* FstTotal elements = MiddleTotal - FstOfs */
	__u8 FstOfs;
};

struct VPUI_Tile {
	/* When 0 - try to insert all columns (up to MaxCols) in same tile (horizontal stripes),
	   when 1 try to insert all lines in same tile (vertical stripes).
	*/
	__u8 IsColsDivided;
	/* Minimal number of lines in tile when IsColsDivided == 0 / Minimal number of columns in
	 * tile when IsColsDivided == 1. Might be set to 0.
	 */
	__u8 MinShortDim;
	/* Maximal columns in tile (for middle tile). 0 stands for no limitation for 2D tiles */
	__u16 MaxCols;
	/* LS word of maximal number of pixels in tile - 0 for no limitation on 2D tiles
	 * Might be used together with MaxCols to force accurate tile dimensions
	 */
	__u16 MaxPixelsLSW;
	/* MS word of maximal number of pixels in tile - 0 for no limitation on 2D tiles */
	__u16 MaxPixelsMSW;
	struct VPUI_TileAxis Cols;
	struct VPUI_TileAxis Lines;
};

/************* Input / output type description *******************************/
struct VPUI_InOutType {
	__u8  IsDyn;
	/**
	 * Whether the ROI is pre-defined or dynamically sets
	 * externally / by CPU. Relevant only when RoiFromPtIndP1 == 0
	 */
	__u8  NumInpSetsEachRoi;
	/**
	 * This is the number of input sets each ROI in the input type
	 * is used for. If set to 0 - fixed ROI is used.
	 * Relevant only when RoiFromPtIndP1 == 0. Usually it is 1 or 0
	 */
	__u8  TileIndP1;
	/* From entire task tiles VPUI_Tile. 0 stands for "no tiles". */
	__u8  IsUpdateByLoop;
	/* Whether ROI start coordinates of current type updated along
	 * the loop on each input set. Relevant only when
	 * PROC_IS_INPUT_LOOP of VPUI_ProcDesc is set.
	 * Assuming single loop for all input types that are updated
	 * along the loop.
	 */
	__u8  RoiFromPtIndP1;
	/* From entire task VPUI_RoiFromPt. Which Point current IO ROI
	 * is derived from. 0 stands for current IO ROI not derived from
	 * point
	 */
	__u8  DummyAlign;
};

/**
 * ****************** Image description ***************************************
 * Each image description is a set of continuous parameters that describe an
 * image. These parameters are sizes, line offset and number of bytes per
 * pixel.
 * Since image descriptions might be dynamically update by the HOST they are
 * described on the parameters vector of the task with few parameters for each
 * image description.
 */
enum VPUI_ImageDesc {
	VPUI_IMDESC_WIDTH		= 0,
	VPUI_IMDESC_HEIGHT		= 1,
	VPUI_IMDESC_LINE_OFS		= 2,		/* In bytes */
	/* Required for dynamic updates of address for ROIs / Tiles / Input Loop */
	VPUI_IMDESC_PIXEL_BYTES		= 3,
	VPUI_IMDESC_TOT_PARAMS		= 4
};

/********** Memory types used during input set process  **********************/
/* Types described below might be accessed by DMA along the entire process */
enum VPUI_MemTypes {
	VPUI_MEM_EXTERNAL		= 0,	/* Using external memory */
	VPUI_MEM_INTERNAL		= 1,	/* Using VPUI_InternalRam */
	/* Using part of Coefficients vector in SRAM (either static or dynamic vector) */
	VPUI_MEM_COEFF_VEC		= 2,
	VPUI_MEM_TYPES			= 3
};

/********* Memory types used during preload and poststore ********************/
/* Types described below might be accessed by DMA on pre-load + post-stroe */
enum VPUI_IntMemTypes {
	/* explicit VPUI_InternalRam */
	VPUI_INTMEM_MPRB	= 0,
	/* Using part of Coefficients vector in SRAM (either static or dynamic vector) */
	VPUI_INTMEM_COEFF_VEC	= 1,
	/* Using part of output vector in SRAM */
	VPUI_INTMEM_OUTPUT	= 2,
	/* The Process part in Dynamic ROIs vector in SRAM */
	VPUI_INTMEM_DYN_ROIS	= 3,
	/* The Process part in Dynamic points vector in SRAM */
	VPUI_INTMEM_DYN_POINTS	= 4
};

/***************** Memory vector description *********************************/
struct VPUI_MemVecDesc {
	__u8  ExtMemInd;	/* External memory address vector index */
	/* Whether Ext memory address is updated on each input sets slot or same address is used */
	__u8  IsExtUpdated;
	__u8  IntMemType;	/* VPUI_IntMemTypes values */
	/* for internal - VPUI_InternalRam index (entire task),
	 * for coeffs- VPUI_GenFiltCoeffsSets index (entire task),
	 * for outputs - the output index (entire task),
	 * Ignored for VPUI_INTMEM_DYN_ROIS + VPUI_INTMEM_DYN_POINTS
	 */
	__u8  IntMemIndex;
	/* Whether vector size is fixed or dependent by the current input sets number */
	__u8  IsFixedSize;
	__u8  DummyAlign;
	/* Vector size in bytes.If IsFixedSize is set this is the entire  size for input sets slot-
	 * otherwise it is the size for each InputSetsDivider input sets in the input sets slot.
	 */
	__u16 VectorBytesPerSetDiv;
};

/********************** Map description **************************************/
struct VPUI_MapDesc {
	/* VPUI_MemTypes */
	__u8  Type;
	/* For external - Address index, for internal - VPUI_InternalRam index (entire task) */
	__u8  Ind;
	/* In VPUI_ImageDesc vector. */
	__u8  SizesInd;
	__u8  DummyAlign;
};

/************** Input / Output ROI description *******************************/
struct VPUI_Roi {
	__u16 FstCol;			/* Offset in the map */
	__u16 FstLine;			/* Offset in the map */
	struct VPUI_Sizes Sizes;	/* 0 for using map sizes */
};

#define INVALID_PT_INT	0xffff	/* Used when ROI around Pt exceed map limits*/

/******************** Point description **************************************/
/* Task may include static points and dynamic points loaded from external memory */
struct VPUI_Point {
	__u16 ColInt;
	__u16 ColFrac;	/* FractionBits number derived from VPUI_PtList */
	__u16 LineInt;
	__u16 LineFrac;	/* FractionBits number derived from VPUI_PtList */
};

/***************** Points list description ***********************************/
struct VPUI_PtList {
	__u8  IsDyn;
	      /**
	       * Whether the points are pre-defined or dynamically set
	       * externally / by CPU
	       */
	__u8  NumInpSetsEachPt;
	      /**
	       * This is the number of input sets each point is used for.
	       * If set to 0 - fixed Point is used. Usually it is 0 or 1
	       */
	/* The index in "image description" sizes vector for sizes that represent "full image" */
	__u8  FullSizesInd;
	/* In each VPUI_Point in the list */
	__u8  FractionBits;
};

/**************** Roi from Point description *********************************/
/* Description of ROI derived from an input point. The input points are read
 * from the points list, the map index is from each set of maps. Few sets of
 * maps might be used for input based on the same point.
 */
struct VPUI_RoiFromPt {
	__u8  PtListInd;	/* Index of VPUI_PtList */
	__u8  MapInSetInd;	/* Index of the map (from "set" of maps) */
	__u8  SizesInd;		/* Index of the PtRoiSizes */
	/* In case the ROI exceed the map limits - if IsCropValid is set it is cropped otherwise
	 * Point is set to INVALID
	 */
	__u8  IsCropValid;
};
#else

struct VPUI_Sizes {
	__u16 Width;
	__u16 Height;
};

/* Used for map dynamic sizes on parameters vector - might be updated by the HOST */
enum VPUI_DynSizes {
	VPUI_SIZES_WIDTH		= 0,
	VPUI_SIZES_HEIGHT		= 1,
	VPUI_SIZES_TOT_PARAMS		= 2
};

#define SIZES_IND_LSB		0
	/* For static sizes-in sizes vector,for dynamic sizes-in sizes part of parameters vector */
#define SIZES_IND_MASK			0x7f
#define SIZES_IS_DYN_LSB	7
#define SIZES_IS_DYN_MASK		1
/********************** Maps description **************************************/
struct VPUI_ImageDesc {
	/* In Bytes. 0 stands for continuous memory (offset = line width * pixel bytes) */
	__u16 LineOfs;
	__u8  PixelBytes;
	__u8  SizesBits;	/* SIZES_IND + SIZES_IS_DYN */
};

#define MAP_IND_LSB		0
	/* For external- External memory address index ,for internal - VPUI_InternalRam index */
#define MAP_IND_MASK			0x7f
#define MAP_IS_EXT_LSB		7
#define MAP_IS_EXT_MASK			1
struct VPUI_MapDesc {
	__u8  MapBits;		/* MAP_IND + MAP_IS_EXT */
	__u8  ImDescInd;	/* In VPUI_ImageDesc vector. */
};

struct VPUI_MapsSet {
	__u8  MapsNumInSet;
	__u8  NumOfSets;
	__u8  FstMapIndOfs;	/* In maps indices vector . 0 = 1st index of current process */
	__u8  DummyAlign;
};

struct VPUI_RoiFromMapsSets {
	__u8  MapsSetInd;	/* In VPUI_MapsSet vector */
	__u8  IndInSet;		/* Index in set  0..MapsNumInSet - 1 */
};

/************** Input / Output ROI description *******************************/
struct VPUI_Roi {
	__u16 FstCol;
	__u16 FstLine;
	struct VPUI_Sizes Sizes;
};

/************** ROI extended information description *******************************/
struct VPUI_RoiExtInfo {
	/* Either in the set or in the entire MapDesc vector */
	__u8 MapInd;
	/* Used In case the Rois are divided to few groups (e.g. Frontal/ Profile Face) */
	__u8 GroupInd;
};

struct VPUI_FixExRoi {
	struct VPUI_RoiExtInfo Ext;
	/* Index in VPUI_Roi vector. */
	__u16 ROIInd;
};

struct VPUI_DynExRoi {
	struct VPUI_RoiExtInfo Ext;
	struct VPUI_Roi	ROI;
};

/* Used as part of both RoiList and PtList */
struct VPUI_List {
	/* In VPUI_MapsSet vector. 0 - no set is used (using direct map indices) */
	__u8  MapsSetIndP1;
	/* Number of loops + 1 for fixed number of loops. 0 stands for InputSetsNum loops */
	__u8  LoopsNumP1;
	/* Whether list is derived from pre-defined or dynamic vector */
	__u8  IsDyn;
	/* Dynamic vectors are used for loading data. Same vector may share few lists.
	 * Ignored when IsDyn is not set
	 */
	__u8  VectorInd;
	/* In case few lists are kept in same vector they might be kept interleaved or not */
	__u8  IsVectorInterleaved;
	/* The offset of the 1st record in the list. In case IsVectorInterleaved is not set it is
	 * multiplied by the number of input sets
	 */
	__u8  FstOfs;
	/* The number of records pointer is incremented on each loop. Will be set to 1 unless the
	 * vector is interleaved (however if list in interleaved vector may be also set to 1)
	 */
	__u8  IncPtrForLoop;
	/* The number of records pointer is incremented on the end of the loop + 1. 0 stands for
	 * pointer that is reset on end of loop. For vector that isn't interleaved will be always
	 * set to 0 (stands for reset) or 1 (stands for no increment)
	 */
	__u8  IncPtrForEndLoopP1;
};

struct VPUI_RoiList {
	struct VPUI_List List;
};

#define INVALID_PT_INT	0xffff	/* Used when ROI around Pt exceed map limits*/

/******************** Point description **************************************/
/* Task may include static points and dynamic points loaded from external memory */
struct VPUI_Point {
	__u16 ColInt;
	__u16 ColFrac;	/* FractionBits number derived from VPUI_PtList */
	__u16 LineInt;
	__u16 LineFrac;	/* FractionBits number derived from VPUI_PtList */
};

/***************** Points list description ***********************************/
struct VPUI_PtList {
	/* pre-defined / dynamic points + Input sets number each point is used for */
	struct VPUI_List List;
	/* The VPUI_ImageDesc index for sizes that represent "full image" */
	__u8  FullSizesImDescInd;
	/* In each VPUI_Point in the list */
	__u8  FractionBits;
};

struct VPUI_ListPtrOp {
	__u8  IsPt;	/* Or ROI */
	__u8  Index;
};

/**************** Roi from Point description *********************************/
/* Description of ROI derived from an input point. The input points are read
 * from the points list, the map index is from each set of maps. Few sets of
 * maps might be used for input based on the same point.
 */

struct VPUI_RoiFromPt {
	__u8  PtListInd;	/* Index of VPUI_PtList */
	__u8  MapInd;		/* either in Set or VPUI_MapDesc vector (e.g. Frame N / N - 1) */
	__u8  SizesInd;		/* Index of the PtRoiSizes */
	/* In case the ROI exceed the map limits - if IsCropValid is set it is cropped otherwise
	 * Point is set to INVALID
	 */
	__u8  IsCropValid;
};

/***************** Input loop description ************************************/
struct VPUI_Scan {
	struct VPUI_Sizes ScanSizes;	/* Actual sizes of ROI in each loop */
	__u16 ScanOfsCols;		/* Horizontal Step on each scan */
	__u16 ScanOfsLines;		/* Vertical Step on each "end of line" scan */
};

struct VPUI_RoiFromScan {
	__u8  RoiSourceInd;	/* Index of ROI that is scanned */
	__u8  ScanInd;		/* Index of VPUI_RoiScan */
};

/**
 * ************************************ Tile description ******************************************
 * Tiles division is used for 2 reasons -internal memory is used as a temporary storage which can't
 * include the entire image or blocks are using lines buffers that can't include the entire image
 * columns.Tile description has optional limitations for both maximal number of samples and maximal
 * number of columns. It also describes whether the division is to horizontal or vertical stripes.
 * Tiles overlap is required when filters are part of process invoked per tiles. On this case some
 * columns (and lines) before the tile and some columns after the tiles are required as input. On
 * this case the tile size is cropped along the process chain of the tile and between each 2 tiles
 * there is an overlap (the formula for overlap calculation is described later).When overlap exists
 * optionally a minimal size of the short dimension might be set in order to avoid the overlap part
 * from being too expensive.
 * Some tiles might be used as source to chain that includes (down or up) scaler. On this case the
 * source of the scaler must be divided by its denominator without reminder for any tile that isn't
 * the last.In order to force such limitation a tile description includes values that the number of
 * columns / lines (optionally- after crop that is performed before the scaler) must divide without
 * reminder. For simplicity the assumption here that tiles will be performed only on chains with
 * pre-defined scalers : chains that scale dynamic input sizes to fixed sizes (=> dynamic scale)
 * won't be divided by tiles.
 * The offset between each 2 middle tiles should be set to TileSize - CropBefDs - CropAftDs*DsRatio
 * (for each dimension). For example in case of TileSize = 80 that is an input to filter that crops
 * 8 columns (4 each side),the 72 columns are sent to Down Scaler that performs 3:2 scaling and the
 * 48 output columns are sent to filter that crops 4 columns.On this case the offset will be set to
 * 80 - 8 - (4x3/2) = 66 (on this case the overlap on each side will be 14).
 * In case of crop that is performed after scaler the crop size must divide the denominator of the
 * scaler (in the example above we can't invoke 4:3 scaling for example). If such Scaler + cropper
 * are required they must be split between different chains.
 * In case crop is performed on the tile the crop might be either performed or not on the edges. It
 * is correct for both crop that is performed before / after Scale.If crop isn't performed on edges
 * the 1st tile requires special handling- its size and next tile's offset are shorter (in order to
 * match other tiles). On the example above if both crops are not performed on edges the 1st tile
 * size is (44 + 4/2) * 3/2 + 8/2 = 73 (or 80 - 8/2 - 4/2*3/2 = 73) => fst offset = 73-14=59. When
 * all crops are not performed on edges 1st tile size = Middle tile size - overlap / 2. Fst offset
 * is always 1st tile size - Overlap
 * Notice that if Crop after Scale exists and it is not performed on edges half crop size must be
 * divided by the denominator of the scaler(in the example described above can't invoke 5:4 scaling
 * in case the crop after scaler isn't invoked on edges).
 */

struct VPUI_TileAxis {
	/* Number of elements that are used for both each couple of neighbor tiles
	 * Offset between tile N+1 to tile N is MiddleTotal - Overlap (FstTotal - Overlap for 1st
	 */
	__u8 Overlap;
	/* Number that total number of elements - CropBefDivider must divide without reminder.
	 * Should be set to denominator of Scale ratio in tile chain. 1 if no scaler in the chain
	 */
	__u8 Divider;
	/* Number that is reduced from total number of elements before divided by divider above */
	__u8 CropBefDivider;
	/* FstTotal elements = MiddleTotal - FstOfs */
	__u8 FstOfs;
};

struct VPUI_Tile {
	/* When 0 - try to insert all columns (up to MaxCols) in same tile (horizontal stripes),
	   when 1 try to insert all lines in same tile (vertical stripes).
	*/
	__u8 IsColsDivided;
	/* Minimal number of lines in tile when IsColsDivided == 0 / Minimal number of columns in
	 * tile when IsColsDivided == 1. Might be set to 0.
	 */
	__u8 MinShortDim;
	/* Maximal columns in tile (for middle tile). 0 stands for no limitation for 2D tiles */
	__u16 MaxCols;
	/* LS word of maximal number of pixels in tile - 0 for no limitation on 2D tiles
	 * Might be used together with MaxCols to force accurate tile dimensions
	 */
	__u16 MaxPixelsLSW;
	/* MS word of maximal number of pixels in tile - 0 for no limitation on 2D tiles */
	__u16 MaxPixelsMSW;
	struct VPUI_TileAxis Cols;
	struct VPUI_TileAxis Lines;
};

struct VPUI_RoiFromTile {
	__u8  RoiSourceInd;	/* Index of ROI that is divided to tiles */
	__u8  TileInd;		/* Index of VPUI_Tile */
};

struct VPUI_RoiCopy {
	__u8  RoiSourceInd;	/* Index of ROI that it sizes is copied */
	__u8  MapInd;		/* Index of VPUI_MapDesc */
};

/* ROI_MAP are set on initialization. Other types are set and updated as part of loops. */
enum VPUI_IORoiType {
	/* Using VPUI_MapDesc */
	VPUI_IO_ROI_MAP	= 0,
	/* Using VPUI_RoiFromMapsSets */
	VPUI_IO_ROI_MAPS_SET	= 1,
	/* Using VPUI_RoiList */
	VPUI_IO_ROI_LIST	= 2,
	/* Using VPUI_RoiFromPt */
	VPUI_IO_ROI_PT	= 3,
	/* Using VPUI_RoiFromScan */
	VPUI_IO_ROI_SCAN	= 4,
	/* Using VPUI_RoiFromTile */
	VPUI_IO_ROI_TILE	= 5,
	/* Using VPUI_RoiCopy */
	VPUI_IO_ROI_COPY = 6
};

struct VPUI_IORoi {
	/* enum VPUI_IORoiType */
	__u8  InOutType;
	/* In the appropriate type - VPUI_MapDesc, VPUI_RoiFromMapsSets, VPUI_RoiList etc. */
	__u8  Index;
};

/********* Memory types used during preload and poststore ********************/
/* Types described below is used in VPUI_InVector description */
enum VPUI_IntMemTypes {
	/* explicit VPUI_InternalRam */
	VPUI_INTMEM_MPRB	= 0,
	/* Loading part of dynamic Coefficients vector in SRAM */
	VPUI_INTMEM_DYN_GEN_COEFFS	= 1,
	/* Storing part of output vector from SRAM */
	VPUI_INTMEM_OUTPUT	= 2,
	/* The Process part in Dynamic ROIs vector in SRAM */
	VPUI_INTMEM_DYN_ROIS	= 3,
	/* The Process part in Dynamic points vector in SRAM */
	VPUI_INTMEM_DYN_POINTS	= 4
};

/********************** Vectors description **************************************/
/* On invoke all VPUI_Vector are set. They might be updated as part of loops. */
struct VPUI_Vector {
	__u8  ExtAdrInd;	/* Index in vector of External memory addresses */
	__u8  IsExtResetOnLoop;	/* Whether external memory address offset is reset on end of loop*/
	__u8  IntMemType;	/* VPUI_IntMemTypes values */
	/* for internal - VPUI_InternalRam index, for coeffs- VPUI_GenFiltCoeffsSets index,
	 * for outputs - the output index, for dyn ROIs / Dyn Points - the appropriate vector index
	 * Index is "entire task" index except from VPUI_GenFiltCoeffsSets index (procees base)
	 */
	__u8  IntTaskIndex;
	__u8  RecordBytes;
	/* Set to 0 in case the total number of records is not dependent by number of input sets.
	 * Set to appropriate input set index + 1 if number of records is dependent input sets.
	 */
	__u8  InputSetsIndOfsP1;
	/* Per each input set if InputSetsIndOfsP1 != 0. */
	__u16 TotalRecordsPerSet;
	__u16 MaxSlotRecords;
};

/**
 ****************** Dynamic Output List description ***********************************************
 * Some processes include dynamic output lists with unknown sizes. The actual total number of
 * records will be always written as VPUI_DynOutList in the beginning of the output list.
 * Currently only M2LI creates dynamic output list.Its RecordBytes should match M2LI configuration.
 * It might be 4 (if only coordinates are reported on output list), 6 (in case 8 / 16 bits value is
 * added to each record) or 8 (in case 32 bits value is added to each record)
 * Total allocated size for the list should be >= TotalSizeVal * RecordBytes + 258 since the first
 * 2 bytes are used for VPUI_DynOutList and up to 256 extra dummy bytes might be written after the
 * entire list
 */
struct VPUI_DynOutList {
	__u8  ExtAdrInd;	/* Index in vector of External memory addresses */
	__u8  RecordBytes;
	/* The actual size of each slot should be read from HW and the maximal slot size is
	 * MaxTotalRecords - total records already written
	 */
	__u16 MaxTotalRecords;
	/* The output index of the final total records of the list - 0 if not sent as output */
	__u8 OutIndexP1;
	__u8 DummyAlign;
};

struct	VPUI_DynOutListHdr {
	__u16	TotalRecords;
};

/* ROI_MAP are set on initialization. Other types are set and updated as part of loops. */
enum VPUI_InOutType {
	/* Using VPUI_IORoi */
	VPUI_INOUT_ROI		= 0,
	/* Using VPUI_Vector */
	VPUI_INOUT_EXT_VEC	= 1,
	/* Using VPUI_Vector */
	VPUI_INOUT_INT_VEC	= 2,
	/* Using VPUI_DynOutList */
	VPUI_INOUT_DYN_LIST	= 3
};

struct	VPUI_InOut {
	__u8 Type;	/* VPUI_InOutType */
	__u8 Index;	/* In the appropriate vector */
};

/* The enum below is a list the types of all the entities that are updated along loops */
enum VPUI_LoopUpType {
	/* Update vector part according to VPUI_Vector. Usually update slice of Input data
	 * (dynamic ROIs / dynamic points). Number of loops is derived from TotalRecordsPerSet and
	 * MaxSlotRecords. HW sub-chain (usually the 1st) should include loading the appropriate
	 * vector part to Internal memory.
	 */
	VPUI_LOOP_VECTOR_PART	= 0,
	/* Update dynamic list according to VPUI_DynOutList. Not expected as 1st loop element */
	VPUI_LOOP_UP_DYN_LIST	= 1,
	/* Update ROI according to VPUI_IORoiType. For ROI_SCAN Loop finished on Bottom Right
	 * corner, for ROI_TILE Loop finished on last tile. Other types must not be the 1st loop
	 * element and used just for ROI update. ROI_MAP isn't expected to be used in loops
	 */
	VPUI_LOOP_UPDATE_ROI	= 2,
	/* Initialize list pointer according to VPUI_ListPtrOp. Not expected as 1st loop element */
	VPUI_LOOP_INIT_LIST_PTR	= 3,
	/* Update list pointer according to VPUI_ListPtrOp. Number of loops is either fixed or
	 * number of input sets.
	 */
	VPUI_LOOP_UP_LIST_PTR	= 4,
	/* Update maps set according to VPUI_MapsSet e.g. next pyramid level for few maps.
	 * Number of loops = NumOfSets
	 */
	VPUI_LOOP_MAPS_SET	= 5,
	/* Number of loops is either pre-defined or CPU decision */
	VPUI_LOOP_COUNTER	= 6,
	VPUI_LOOP_TOTAL_TYPES	= 7
};

struct VPUI_PrLoopEntity {
	__u8  LoopType;		/* enum VPUI_LoopUpType */
	__u8  Index;		/* In the appropriate entity vector - according to LoopType */
};
#endif

/**
 * ***************** Loop description ****************************************
 * Loops might exist in both process and task. Process may include single loop
 * - invoked few times for each input set. Task may include many loops.
 * In each of the loop iterations few parameters values are updated. Some of
 * them gets explicit value per loop while some are updated by fix
 * increment / decrement each loop.
 * There are also parameters that are updated according to 1st loop / other
 * loop. For process these parameters are used by the different sub-chains.
 * This structure describe the number of parameters updated along the loop.
 */
struct VPUI_LoopTotParams {
	__u8 ExpPerSet;
	     /* The number of parameters explicitly described in each parameters set */
	__u8 SetsNum;
	     /**
	      * The number of explicitly described parameters sets. Loop Index % SetsNum chooses
	      * the current set. Usually Loop Index < SetsNum
	      */
	__u8 FixInc;
	     /* The number of parameters that has fix increment / decrement for each loop. */
	__u8 PerFst;
	     /* The number of parameters replaced according to 1st loop / other loop */
};

struct VPUI_ParamsOfs {
	__u16 FstInd;
	      /* Offset in vector of all loops parameters indices of the 1st parameter replaced */
	__u16 FstVal;
	      /* Offset in vector of all loop parameters values of the 1st parameter replaced */
};

/**
 * This structure describes the offsets in the vectors of parameters values and
 * parameter indices of loop parameters. For each loop ExpPerSet +FixInc values
 * overwrite parameters in the appropriate indices.For 1st and 2nd loop another
 * PerFst parameters are written. The parameters values of the process loop
 * (if exist) are written as part of process extra parameters. Notice that for
 * process the parameters that are overwritten are the HW blocks parameters
 * - not the process extra parameters - and the indices refer to this vector.
 * All task loops parameters values are written continuously as part of task
 * parameters.
 */
struct VPUI_LoopParamsOfs {
	struct VPUI_ParamsOfs PerSet;
	       /**
		* Replaced parameters for each set
		* (ExpPerSet indices, SetsNum*ExpPerSet values
		* - SetsNum of 1st param, SetsNum of 2nd param etc.)
		*/
	struct VPUI_ParamsOfs FixUpdate;
	       /**
		* Incremented parameters for each loop (FixInc indices,
		* 2 * FixInc values - for each parameter - base value for
		* 1st loop + signed increment value for each loop)
		*/
	struct VPUI_ParamsOfs FstLoop;
	       /**
		* Replaced parameters for first / other
		* (PerFst indices,2*PerFst values- for each parameter value
		* for 1st loop + value of other loops)
		*/
};

#ifndef OLD_FWIF_IO
/***************** Task loop description *************************************/
struct VPUI_LoopParams {
	struct	VPUI_LoopTotParams Tot;
		/**
		 * Number of parameters sets (usually equal LoopsNum)
		 * and number of parameters per each loop + 1st / not loop
		 */
	struct	VPUI_LoopParamsOfs Ofs;
		/**
		 * The offset in the vector of all parameters indices and
		 * values for "each loop" and "first loop" parameters
		 */
};

struct VPUI_ProcLoop {
	__u8  FstEntityIndex;	/* In VPUI_PrLoopEntity vector. The 1st defines the loop type */
	__u8  EntitiesNumber;	/* Number of loop entities handled per the loop */
	/* The index of the total input sets for the loop + 1. 0 stands for none  */
	__u8  TotInputSetIndP1;
	/* The index of the VPUI_LoopParams updated per each loop + 1. 0 stands for none  */
	__u8  ParamsIndP1;
	/* The 1st sub-chain of the loop  */
	__u8  FstSubCh;
	/* The last sub-chain of the loop  */
	__u8  LastSubCh;
};
#endif

#ifdef OLD_FWIF_IO
/**
 * ********** Input / Output Map ROI description ******************************
 * Each VPUI_InOutType has a continuous vector of Fix/DynMapRois per input sets
 * slot. These vectors are written one after the other (one FixMapRoi vector
 * for all InOut with fix ROIs and one DynMapRoi vector for all InOut with
 * dynamic ROIs). For each new input set / each new tile 2 dynamic memory
 * vectors are created with member for each InOut type based on Map ROI and set
 * tile. One vector includes sizes and the other memory addresses,lines offsets
 * and pixel bytes. The sizes vector is expanded by sizes operations- see sizes
 * description below. The addresses are also updated on each input loop. Blocks
 * parameters indices of sizes and DMAs blocks indices of addresses, line
 * offsets and pixel bytes refer to those vectors.
 * Notice that before and after the input sets are invoked there are optionally
 * few sub-chains used for load data from DRAM / store data to DRAM. For each
 * of these stages a set of VPUI_MemVecDesc describe the sizes and memories
 * used. The sizes vector length is equal to Pre/PostSetsMemVectors (no sizes
 * operations on pre and post input sets) and the memory description vector
 * length is equal to Pre/PostSetsMemVectors * 2 - for each the external memory
 * and than the internal memory. One of the pre-loads is the list of ROIs in
 * case of dynamic ROIs (since the CORE access to DRAM is very slow dynamic
 * ROIs are loaded from DRAM to internal SRAM before the process begins).
 * Since internal memory must be allocated a maximal number of "input sets" are
 * defined and in case VPUI_PROC_PAR_INPUT_SETS_NUM_OFS = the actual number of
 * input sets is bigger than MaxInputSetsInSlot the input sets are divided to
 * slots. When more than single dynamic input type exists and more than single
 * slot it is the user responsibility to divide the input ROIs list to slots -
 * he should put for each dynamic type MaxInputSetsInSlot / NumInpSetsEachRoi
 * ROIs in each slot.
 */
struct VPUI_FixMapRoi {
	/**
	 * Index in VPUI_MapDesc vector -
	 * memory description + sizes index (+pixel bytes)
	 */
	__u8 MapInd;
	/* Used In case the Rois are divided to few groups */
	__u8 GroupInd;
	/* Index in VPUI_Roi vector. 0 if the entire map is used */
	__u16 ROIIndP1;
};

struct VPUI_DynMapRoi {
	/**
	 * Index in VPUI_MapDesc vector -
	 * memory description + sizes index (+pixel bytes)
	 */
	__u8 MapInd;
	/* Used In case the Rois are divided to few groups */
	__u8 GroupInd;
	struct VPUI_Roi	ROI;
};
#endif

/**
 * ************** Sizes operations description ********************************
 * Images sizes might be  updated along the HW chain. Below is the description
 * of the sizes operations that might be implemented. After InOut types sizes
 * vector is created (on each new input set / each new tile) it is expanded by
 * sizes operations described below. Total sizes vector is number of
 * InOut types + number of sizes operations.
 */

/********************* Fix sizes description *********************************/
struct VPUI_FixOp {
	struct VPUI_Sizes Sizes;
};

/*********************** Crop description ************************************/
struct VPUI_CropOp {
	__u8 Left;
	__u8 Right;
	__u8 Top;
	__u8 Bottom;
};

/********************** Scales description ***********************************/
struct VPUI_ScaleOp {
	__u16 WidthNumer;
	__u16 WidthDenom;
	__u16 HeightNumer;
	/* Set to 0 to signal 3DProc DMA32 for 16 bits data scaler - (Width+1)/2 and keep Height */
	__u16 HeightDenom;
};

enum VPUI_SizesOpType {
	/* Copy current ROI from one of the InOut */
	VPUI_SIZEOP_INOUT	= 0,
	/* Use fix sizes */
	VPUI_SIZEOP_FIX		= 1,
	/* Use crop operation on the input sizes for all tiles */
	VPUI_SIZEOP_FORCE_CROP	= 2,
	/* Use crop operation on the input sizes on the edges tiles of the input */
	VPUI_SIZEOP_CROP_ON_EDGES_TILES	= 3,
	/* Use Scale operation on the input sizes */
	VPUI_SIZEOP_SCALE	= 4
};

/**************** Sizes operations description *******************************/
#define SIZES_OP_TYPE_LSB	0	/* VPUI_SizesOpType */
#define SIZES_OP_TYPE_MASK		0x7
#define SIZES_OP_IND_LSB	3
/* from InOut / Task Fix / Croppers / Scalers */
#define SIZES_OP_IND_MASK		0x1f

struct VPUI_SizesOp {
	/* SIZES_OP_TYPE + SIZES_OP_IND */
	__u8 SizesOpBits;
	/* the previous SizesOp - not relevant for INOUT and FIX sizes */
	__u8 SrcInd;
};

/**
 * ********* All Sub Chains totals description ********************************
 * Same to VPUI_HwSubChTotals but using unsigned short since it is the total
 * of all sub-chains
 */
struct VPUI_AllHwSubChTotals {
	__u16	Blocks;
		/**
		 * VPUI_BlUsage in all Hw Sub chains of process
		 * / 3DNN process / all processes + 3DNN processes
		 */
	__u16	HwConnections;
		/**
		 * Number of VPUI_HWConnect for all Blocks usages of Process
		 * / 3DNN process / all processes + 3DNN processes
		 */
	__u16	HwMprbsIndGr;
		/**
		 * Number of MPRBs indices groups in all Blocks usages of
		 * Process / 3DNN process / all processes + 3DNN processes
		 */

	__u8	InternalRamsInd;
		/**
		 * Number of all indices of internal RAMs of Process
		 * / 3DNN process / all processes + 3DNN processes
		 */
	__u8	HwReadResBl;
		/**
		 * Number of indices of all Blocks that read results for
		 * Process / 3DNN process / all processes + 3DNN processes
		 */
};

/*********** Totals used for Process, 3DNN Process and Task ******************/
struct VPUI_AllTotals {
	struct	VPUI_AllHwSubChTotals HwSubCh;
	__u8	InternalRams;		/* VPUI_InternalRam */
	__u8	CpuOps;			/* VPUI_CpuOp */
	__u8	CpuSubCh;		/* VPUI_CpuSubCh */

	__u8	SubCh;			/* VPUI_SubCh */
#ifdef OLD_FWIF_IO
	/* Number of parameters values in Process / 3DNN process / all processes
	 * For process : VPUI_PROC_PAR_ALL + blocks parameters + CPU sub chains parameters+ Loop
	 * parameters values (== VPUI_LoopTotParams.SetsNum*ExpPerSet + FixInc*2 + PerFst*2)
	 * For 3DNN process : CPU parameters + all slice group type parameters + (layers HW
	 * parameters values + layer process * parameters * values) * N Layers. All slice group
	 * type parameters might be derived from the summation of SlGrTypeParams different types.
	 * For task : VPUI_TASKPAR_ALL + sum of all processes + 3DNN processes + all loops values +
	 * dynamic filters coefficients + Image Desc
	 */
#else
	/* Number of parameters values in Process / 3DNN process / all processes
	 * For process : 1 - for Disable sub-chains mask + blocks parameters + CPU sub chains
	 * parameters + Loops parameters values (== sum of VPUI_LoopTotParams.SetsNum*ExpPerSet +
	 * FixInc*2 + PerFst*2)
	 * For 3DNN process : CPU parameters + all slice group type parameters + (layers HW
	 * parameters values + layer process * parameters * values) * N Layers. All slice group
	 * type parameters might be derived from the summation of SlGrTypeParams different types.
	 * For task: Masks parameters + total input sets parameters + sum of all processes + 3DNN
	 * processes + task loops values + dynamic filters coefficients + dynamic Sizes
	 */
#endif
	__u16	ParamsValues;
	/* For process - Loop parameters indices == VPUI_LoopTotParams.ExpPerSet + FixInc + PerFst
	 * For 3DNN process- all slice group type parameters + layers HW + layer process parameters
	 * For task : all Procs + task loops + allocate indices + invoke indices (described in VPUI_TaskParamsIndicesOfs)
		 */
	__u16	ParamsIndices;
};

/****************** Totals used for Process and Task *************************/
#define VPUI_MAX_INOUT_PER_PROC		32
	/**
	 * Maximal total number of inputs/outputs of each Proc is required for
	 * internal memory allocation. Used for both Proc and 3DProc.
	 */
#define VPUI_MAX_SIZES_PER_PROC		32
#define VPUI_SIZES_IND_MASK			0x1f
	/**
	 * Maximal total number of sizes used by each Proc is required for
	 * internal memory allocation. The mask is used by blocks parameters
	 */
#define VPUI_MAX_PT_LISTS_PER_PROC	4
	/**
	 * Maximal total number of point list of each Proc is required for
	 * internal memory allocation. Used for 2DProc only.
	 */

struct VPUI_ProcAndTaskTotals {
	/* number of HW sub-chains descriptors - VPUI_HwSubCh */
	__u8	HwSubCh;
#ifdef OLD_FWIF_IO
	/* Number of VPUI_MemVecDesc used in the sub chains before and
	 * after the Input Sets (in each Input Sets slot).
	 */
	__u8	PreSetsMemVecs;
	__u8	PostSetsMemVecs;
	/* Number of VPUI_InOutType for each input set. Each has its
	 * own ROI (per input set) and its own tiles division method.
	 * For Proc this is also the number of dynamic addresses used
	 * by DMAs block while input sets are processed
	 */
	__u8	InOutTypes;
	/* Number of VPUI_PtList used. For process this is the size of
	 * vector of float points used.
	 */
	__u8	PtLists;
	/* Number of VPUI_Sizes used */
	__u8	PtRoiSizes;
	/* Number of VPUI_RoiFromPt used */
	__u8	RoiFromPt;
	/* For each process: PtsMapsSets *  PtsMapsEachSet */
	__u8	PtMapIndices;
	/* VPUI_Point - for all VPUI_InOutType types
	 * with RoiFromPtIndP1 != 0 && IsDyn == 0 together.
	 */
	__u16	FixPoints;
	/* VPUI_Point - for all VPUI_InOutType types
	 * with RoiFromPtIndP1 != 0 && IsDyn == 1 together.
	 */
	__u16	MaxDynPoints;
	/* VPUI_Roi used as part of VPUI_FixMapRoi */
	__u16	Rois;
	/* VPUI_FixMapRoi - for all VPUI_InOutType types
	 * with RoiFromPtIndP1 == 0 and IsDyn == 0 together
	 */
	__u16	FixMapRois;
	/* VPUI_DynMapRoi - for all VPUI_InOutType types
	 * with RoiFromPtIndP1 == 0 and IsDyn == 1 together.
	 */
	__u16	MaxDynMapRois;
#else
	/* For each process: MapsNumInSet * NumOfSets */
	__u8	MapsSetsIndices;
	/* Number of VPUI_MapsSet used */
	__u8	MapsSets;
	/* Number of VPUI_RoiFromMapsSets used */
	__u8	RoisFromMapsSets;
	/* number of VPUI_RoiList (both Fixed and Dynamic)*/
	__u8	RoiLists;
	/* Number of VPUI_PtList used */
	__u8	PtLists;
	/* Number of init VPUI_ListPtrOp used */
	__u8	InitListsPtrOps;
	/* Number of update VPUI_ListPtrOp used */
	__u8	UpListsPtrOps;
	/* Number of VPUI_Sizes used */
	__u8	PtRoiSizes;
	/* Number of VPUI_RoiFromPt used */
	__u8	RoiFromPt;
	/* Number of VPUI_Scan used */
	__u8	Scans;
	/* Number of VPUI_RoiFromScan used */
	__u8	RoisFromScans;
	/* Number of VPUI_RoiFromTile used */
	__u8	RoisFromTiles;
	/* Number of VPUI_RoiCopy used */
	__u8	RoisCopy;
	/* number of VPUI_IORoi */
	__u8	IORois;
	/* number of VPUI_Vector */
	__u8	Vectors;
	/* number of VPUI_DynOutList */
	__u8	DynOutLists;
	/* number of VPUI_InOut */
	__u8	InOuts;
	/* For each loop - total number of iterations or 0 for CPU decision */
	__u8    IterLoops;
	/* number of VPUI_PrLoopEntity */
	__u8	PrLoopEntities;
	/* number of VPUI_LoopParams */
	__u8	LoopsParams;
	/* number of VPUI_ProcLoop */
	__u8	ProcLoops;
	/* InputSetsSizes = Size of each input set - Part of parameters vector */
	/* VPUI_Roi used as part of VPUI_FixExRoi */
	__u16	Rois;
	/* VPUI_FixExRoi - for all VPUI_RoiList with List.IsDyn == 0 */
	__u16	FixExRois;
	/* VPUI_Point - for all VPUI_PtList with IsDyn == 0 */
	__u16	FixPoints;
	/* VPUI_Point - for all VPUI_PtList with IsDyn == 1 */
	__u16	MaxDynPoints;
#endif
	/* VPUI_SizesOp. For process this is the size of the "vector of sizes"
	 * used by the blocks.
	 */
	__u16	SizesOp;
};

/* Working area is used for CPU operations and read HW results */
#define VPUI_MAX_WORK_AREA_INDICES	16

#ifdef OLD_FWIF_IO
/*************** Process explcit description *********************************/
struct  VPUI_ProcOnlyTotals {
	/* SubCh invoked only once for all input sets */
	/* Number of Sub Chains invoked once before the input sets.
	 * Required for pre-load data of Gamma, ROIs list etc.
	 */
	__u8	SubChBefInputSets;
	/* Number of Sub Chains invoked once after the input sets. */
	__u8	SubChAftInputSets;
	/* SubCh invoked only once per tile */
	/* Number of Sub Chains invoked before the process chain
	 * on 1st parameters loop of each input.
	 */
	__u8	SubChFstLoopOnly;
	/* Number of Sub Chains invoked after the process chain only
	 * on parameters last loop of each input set.
	 */
	__u8	SubChLastLoopOnly;
	/* Number of invocations per each input tile - each with
	 * different parameters - 0 stands for CPU decision
	 */
	__u8	LoopsNum;
	/* Maximal working area indices required for HW read and CPU
	 * operations in the process.
	 * Must be <= VPUI_MAX_WORK_AREA_INDICES
	 */
	__u8	MaxWorkSizeIndices;
	/* When points are used they might be used for few maps for
	 * example - N level pyramid. On this case PtsMapsSets = N
	 * 0 stands for no IO is using points
	 */
	__u8	PtsMapsSets;
	/* The number of maps in each set. For example if 2 pyramids
	 * are used PtsMapsEachSet == 2
	 */
	__u8	PtsMapsEachSet;
	/* Loop parameters description */
	struct	VPUI_LoopTotParams LoopsTot;
};
#endif

struct VPUI_ProcTotals {
	struct VPUI_AllTotals		All;
	struct VPUI_ProcAndTaskTotals	Task;
#ifdef OLD_FWIF_IO
	struct VPUI_ProcOnlyTotals	Only;
#else
	/* For each input set - a process parameter is set for current number of inputs */
	__u8	InputSetsNum;
	/* Number of entities that are initialized before all loops - usaully Maps & Lists */
	__u8	InitEntities;
#endif
};

/**************** Process offset description *********************************/
#define VPUI_MPRB_GR_OFS_MASK		0x7f
	/* Up to 128 MPRBs groups per HW sub-chain are supported */

#ifdef OLD_FWIF_IO
/* Offsets in parameters values vector for each Process */
enum VPUI_ProcSpParamsOfs {
	// else - few dynamic input vectors are allowed
	VPUI_PROC_PAR_INPUT_SETS_NUM_OFS	= 0,
	VPUI_PROC_PAR_DISABLE_SUB_CH_MASK_OFS	= 1,
	VPUI_PROC_PAR_ALL			= 2
};
#endif

struct  VPUI_ProcAnd3DNNProcFst {
	struct	VPUI_HwSubChFstIndOfs SubChFstIndOfs;
	/* VPUI_BlUsage + process parameters + VPUI_HWConnect
	 * + HwMPRBsGr + IntRamsInd
	 */
	__u16	RamMprbsGr;		/* Index of 1st Memory MPRBs group */
	__u16	ParametersIndices;	/* uint16 vector */
	/* First for VPUI_AllTotals */
	__u8	InternalRams;		/* VPUI_InternalRam */
	__u8	CpuOps;			/* VPUI_CpuOp */
	__u8	CpuSubCh;		/* VPUI_CpuSubCh */

	__u8	SubCh;			/* VPUI_SubCh */
};

struct  VPUI_ProcFst {
	struct	VPUI_ProcAnd3DNNProcFst Both;
	__u8	HwSubCh;		/* VPUI_HwSubCh */
#ifdef OLD_FWIF_IO
	__u8	PreSetsMemVecs;		/* VPUI_MemVecDesc */
	__u8	PostSetsMemVecs;	/* VPUI_MemVecDesc */
	__u8	InOutTypes;		/* VPUI_InOutType */
	__u8	PtLists;		/* VPUI_PtList */
	__u8	PtRoiSizes;		/* VPUI_Sizes */
	__u8	RoiFromPt;		/* VPUI_RoiFromPt */
	__u8	PtMapIndices;		/* uint8 vector */
	__u16	FixPoints;		/* VPUI_Point */
	__u16	DynPoints;		/* VPUI_Point */
	__u16	Roi;			/* VPUI_Roi */
	__u16	FixMapRoi;		/* VPUI_FixMapRoi */
	__u16	DynMapRoi;		/* VPUI_DynMapRoi */
#else
	__u8    IterLoops;		/* uint8 vector */
	__u8	MapsSetsIndices;	/* uint8 vector */
	__u8	MapsSets;		/* VPUI_MapsSet */
	__u8	RoisFromMapsSets;	/* VPUI_RoiFromMapsSets */
	__u8	RoiLists;		/* VPUI_RoiList */
	__u8	PtLists;		/* VPUI_PtList */
	/* Number of init VPUI_ListPtrOp used */
	__u8	InitListsPtrOps;	/* VPUI_ListPtrOp */
	__u8	UpListsPtrOps;		/* VPUI_ListPtrOp */
	__u8	PtRoiSizes;		/* VPUI_Sizes */
	__u8	RoiFromPt;		/* VPUI_RoiFromPt */
	__u8	RoisFromScans;		/* VPUI_RoiFromScan */
	__u8	RoisFromTiles;		/* VPUI_RoiFromTile */
	__u8	RoisCopy;		/* VPUI_RoiCopy */
	__u8	IORois;			/* VPUI_IORoi */
	__u8	Vectors;		/* VPUI_Vector */
	__u8	DynOutLists;		/* VPUI_DynOutList */
	__u8	InOuts;			/* VPUI_InOut */
	__u8	PrLoopEntities;		/* VPUI_PrLoopEntity */
	__u8	LoopsParams;		/* VPUI_LoopParams */
	__u8	ProcLoops;		/* VPUI_ProcLoop */
	__u8	InputSetsSizes;		/* uint8 vector */
	__u16	Roi;			/* VPUI_Roi */
	__u16	FixExRois;		/* VPUI_FixExRoi */
	__u16	FixPoints;		/* VPUI_Point */
#endif
	__u16	SizesOp;		/* VPUI_SizesOp */
};

#ifndef OLD_FWIF_IO
/* Assuming the first parameters of each process is always Disable sub-chains mask */
#define VPUI_PROCPAR_DISABLE_SUBCH_MASK_OFS 0
#endif
struct VPUI_ProcParamsOfs {
	__u16	HwParamsValues;
		/* From parameters values of the process */
	__u16	CpuParamsValues;
		/* From parameters values of the process */
	__u16	LoopParamsValues;
		/* From parameters values of the process */
#ifdef OLD_FWIF_IO
	/**
	 * Notice that the parameters indices are for Process parameters values
	 * - that is the 1st parameter of entire process will have index == 0
	 */
	struct	VPUI_LoopParamsOfs LoopParams;
		/**
		 * From LoopParamsValues and Process indices (all Process
		 * indices are loop indices)
		 */
#endif
};

#ifdef OLD_FWIF_IO
/**
 ****************** Output List description ***********************************
 * Some processes include dynamic output list with unknown size. Such list is
 * described in the structure below. Only single list is allowed per process.
 * The actual total number of records will be always written as VPUI_OutListHdr
 * in the beginning of the output list.
 */
struct	VPUI_OutListHdr {
	__u16	TotalRecords;
};

struct	VPUI_OutList {
	/* The index of the InOutType that is used for the output list + 1.
	 * 0 if no output list in the process. For this InoutType MapDesc isn't
	 * required (the external memory address is explicitly set) and ROI
	 * isn't required
	 */
	__u8	InOutTypeIndexP1;
	/* The index of the external memory address of the list */
	__u8	ExtAdrInd;
	/* Currently only M2LI creates dynamic output list. This number should
	 * should match M2LI configuration. It might be 4 (if only coordinates
	 * are reported on output list), 6 (in case 8 / 16 bits value is added
	 * to each record) or 8 (in case 32 bits value is added to each record)
	*/
	__u8	RecordBytes;
	/* The maximal number of records that will be written to the list. 0 if
	 * no output list in the process. The total allocated size for the list
	 * should be at least MaxRecords * RecordBytes + 258 since the first 2
	 * bytes are used for "header" and up to 256 extra dummy bytes might be
	 * written after the entire list.
	 */
	__u8	DummyAlign;
	__u16	MaxRecords;
};
#endif

#ifdef OLD_FWIF_IO
/* Is each input set is invoked in loop */
#define PROC_IS_INPUT_LOOP_LSB	0
#define PROC_IS_INPUT_LOOP_MASK		0x1
/* Is any input divided to tiles (in case it is active each input
 * in set has its own tile division)
 */
#define PROC_IS_TILE_DIV_LSB	1
#define PROC_IS_TILE_DIV_MASK		0x1
/* Does the process include only CPU sub-chains (no HW sub-chains) */
#define PROC_IS_CPU_ONLY_LSB	2
#define PROC_IS_CPU_ONLY_MASK		0x1
/* up to 16 priorities */
#define PROC_PRIORITY_LSB	4
#define PROC_PRIORITY_MASK		0xf
#endif

/**
 * ****************** Process description *************************************
 * Includes input & output, ROIs, sub-chains, totals and offsets to data types.
 * Also use the task vectors: VPUI_Tile, VPUI_FixOp, VPUI_CropOp,
 * VPUI_ScaleOp, addresses vector, VPUI_Sizes, VPUI_ImageDesc, VPUI_MapDesc
 */
struct VPUI_ProcDesc {
#ifdef OLD_FWIF_IO
	/* PROC_IS_INPUT_LOOP + PROC_IS_TILE_DIV + PROC_IS_CPU_ONLY + PROC_PRIORITY */
	__u8	ProcBits;
	/* The actual total number of InputSets and the maximal number of InputSets in each slot
	 * must be divided by this number. It should divide NumInpSetsEachRoi of each VPUI_InOut.
	 * Usually it is 1 unless any NumInpSetsEachRoi > 1.
	 */
	__u8	InputSetsDivider;
	/* Number of input sets allowed on each "input sets slot" */
	__u16	MaxInputSetsInSlot;
#else
	__u8 Priority;
	__u8 IsCpuOnly;
#endif
	/**
	 * TODO(SW-15) - Abort other chains ?? Maybe priority 0 stands for
	 * "aborted". Aborted by other Proc in current task + priority != 0 +
	 * Task in AbortTaskMask ??? Abort is on tile limits.
	 */
	struct	VPUI_ProcTotals		Totals;
	struct	VPUI_ProcFst		Fst;
		/**
		 * Include the offsets of process vectors from the entire
		 * task vectors
		 */
	struct	VPUI_ProcParamsOfs	ParsOfs;
		/**
		 * Includes description of offsets of different types of
		 * process parameters values and indices
		 */
#ifdef OLD_FWIF_IO
	struct	VPUI_InpLoop		InpLoop;
	struct	VPUI_OutList		OutList;
#endif
};

/*********** Describing the data required for 3DNN process *******************/
/**
 * ******************* IO XY sizes description ********************************
 * All 3DNN IO sizes are pre-defined. Each 3DNN layer has its own indices in IO
 * XY sizes vector (and input & output Z dimensions + slices groups division)
 */

struct VPUI_3DNNIoXYSizes {
	__u16  XDimSize;
	__u16  YDimSize;
};

enum VPUI_XySizes3DNNType {
	/* The XY sizes of the input slices */
	VPUI_XYSIZE_3DNN_INPUT		= 0,
	/* The XY sizes of the output slices for slices groups */
	VPUI_XYSIZE_3DNN_OUTPUT		= 1,
	/* The XY sizes of the output slices on last slices group */
	VPUI_XYSIZE_LAST_SLGR_OUTPUT	= 2,
	/* The XY sizes of coeffs for slices group */
	VPUI_XYSIZE_COEFFS		= 3,
	/* The XY sizes of coeffs for last slices group */
	VPUI_XYSIZE_LAST_SLGR_COEFFS	= 4,
	VPUI_XYSIZE_ALL			= 5
};

enum VPUI_InOut3DNNType {
	VPUI_IO_3DNN_INPUT	= 0,
	VPUI_IO_3DNN_OUTPUT	= 1,
	VPUI_IO_LASTSLGR_OUTPUT	= 2,
	VPUI_IO_3DNN_COEFFS	= 3
};

enum VPUI_3DXYSizesOpType {
	/* Copy XY sizes of VPUI_InOut3DNNType */
	VPUI_3DXY_SIZEOP_INOUT	= 0,
	/* Use XY+Input Z to XY translation */
	VPUI_3DXY_SIZEOP_ZIN_TO_XY	= 1,
	/* Use XY+Output Z to XY translation */
	VPUI_3DXY_SIZEOP_ZOUT_TO_XY	= 2,
	/* Use crop operation on the input sizes */
	VPUI_3DXY_SIZEOP_CROP	= 3,
	/* Use Scale operation on the input sizes */
	VPUI_3DXY_SIZEOP_SCALE	= 4
};

/**************** Sizes operations description *******************************/
#define SIZES_3D_OP_TYPE_LSB	0	/* VPUI_3DXYSizesOpType */
#define SIZES_3D_OP_TYPE_MASK		0x7
#define SIZES_3D_OP_IND_LSB	3
/* from InOut / Croppers / Scalers */
#define SIZES_3D_OP_IND_MASK		0x1f

struct VPUI_3DSizesOp {
	/* SIZES_3D_OP_TYPE + SIZES_3D_OP_IND */
	__u8 Sizes3DOpBits;
	/* the previous Sizes3DOp - not relevant for INOUT sizes */
	__u8 SrcInd;
};

/**
 * **************** 3DNN Layers description ***********************************
 * Instead of "Input sets" 3DNN has "Layers". The Layers are predefined and
 * their description is part of the 3DNNProc. Below data is different between
 * different Layers.
 */

/**
 * ********** 3DNN Input / output type description ****************************
 * Instead of VPUI_InOut. Each layer has its own description for each VPUI_InOut3DNN
 */
/**
 * Each layer has same number of VPUI_InOut3DNN. One of them describes the input, one describes
 * the output, one describes the output of last slice group (in case the layer is divided to slices
 * groups) and some other describe the coefficients. Coefficient are usually divided between few
 * InOut3DNN. Some of them may have 32 bits DMA and some may have 16 bits DMA.
 * Optionally not all InOut types are used in each layer - e.g. layer with single slice groups will
 * not need the output of last slice group.
 */
struct VPUI_InOut3DNN {
	__u8	InOut3DNNType;
		/* VPUI_InOut3DNNType */
	__u8	BytesNum;
		/**
		 * For Input / output - BytesNum of each pixel.
		 * For Coeffs - 4 for DMA 32 bit / 2 for DMA 16 bits.
		*/
	__u8	BaseAdrInd;
		/* In task all memory addresses vector */
	__u8	DummyAlign;
};

struct VPUI_CnnCropDesc {
	__u8	Left;
	__u8	Right;
	__u8	Top;
	__u8	Bottom;
};

#define CNN_RELU_SIGN_LSB	0	/* Sign bit */
#define CNN_RELU_SIGN_MASK		0x1
#define CNN_RELU_EXPONENET_LSB	1	/* Exponent */
#define CNN_RELU_EXPONENET_MASK		0x7f	/* 7 bits for exponent */
#define CNN_RELU_MANT_LSBS_LSB	8	/* LSBs of Mantissa */
#define CNN_RELU_MANT_LSBS_MASK		0x3
	/* 2 LSB bits of Mantissa (0:1) */

struct VPUI_ReluPosNegDesc {
	__u16	Bits;
		/* CNN_RELU_SIGN + CNN_RELU_EXPONENET + CNN_RELU_MANT_LSBS */
	__u16	MantissaMsbs;
		/* Bits 2:17 (16 MSBs out of 18 bits) */
};

struct VPUI_CnnReluDesc {
	struct VPUI_ReluPosNegDesc Pos;
	struct VPUI_ReluPosNegDesc Neg;
};

/***************** CNN parameters description ********************************/
/* Parameters used only for HW CNN invocation. Each layer has VPUI_CnnDesc */
#define CNN_CFG_IS_DOT_MODE_LSB		0
	/* filter size == Input size => single pixel for each output slot */
#define CNN_CFG_IS_DOT_MODE_MASK		0x1
#define CNN_CFG_IS_PASSTH_MODE_LSB	1
#define CNN_CFG_IS_PASSTH_MODE_MASK		0x1
#define CNN_CFG_IS_SLICE_SUM_LSB	2
	/* After each output slices is created with normal filter it is summed
	 * in post processing => final size is 1x1 (different from DOT mode)
	 */
#define CNN_CFG_IS_SLICE_SUM_MASK		0x1
#define CNN_CFG_IS_XY_CROP_LSB		3
	/* If set - output XY sizes are cropped according to CropIndP1
	 * If reset output XY sizes = XY Input sizes
	 */
#define CNN_CFG_IS_XY_CROP_MASK			0x1
#define CNN_CFG_IS_FILT_BIAS_MEM_LSB	4
	/* Coeffs in memory include 32 bits "filter bias" before each filter */
#define CNN_CFG_IS_FILT_BIAS_MEM_MASK		0x1
#define CNN_CFG_IS_FILT_BIAS_USED_LSB	5
	/* If set CNN_CFG_IS_FILT_BIAS_MEM_LSB must also be set */
#define CNN_CFG_IS_FILT_BIAS_USED_MASK		0x1
#define CNN_CFG_IS_RELU_ENABLED_LSB	6
#define CNN_CFG_IS_RELU_ENABLED_MASK		0x1
#define CNN_CFG_IS_LOC_DEP_COEFFS_LSB	7
	/* Coefficients are location dependent */
#define CNN_CFG_IS_LOC_DEP_COEFFS_MASK		0x1

#define CNN_FILTER_WIDTH_LSB	0
#define CNN_FILTER_WIDTH_MASK		0x7	/* Maximal filter size == 7 */
#define CNN_FILTER_HEIGHT_LSB	4
#define CNN_FILTER_HEIGHT_MASK		0x7	/* Maximal filter size == 7 */

#define CNN_DMA_COEFFS_16B_NUM_LSB	0
	/* Number of 16B DMAs used for loading the coefficients */
#define CNN_DMA_COEFFS_16B_NUM_MASK		0x7
#define CNN_DMA_COEFFS_32B_NUM_LSB	3
	/* Number of 32B DMAs used for loading the coefficients */
#define CNN_DMA_COEFFS_32B_NUM_MASK		0x7
#define CNN_DMA_IS_INPUT_16B_LSB	6
	/* Whether input data is loaded using 16B or 32B DMA.
	 * Input data is loaded by the 1st appropriate type DMA
	 */
#define CNN_DMA_IS_INPUT_16B_MASK		0x1
#define CNN_DMA_IS_ACC_16B_LSB		7
	/* Whether accumulated data is loaded using 16B or 32B DMA.
	 * Accumulated data is loaded by the 1st appropriate type DMA
	 */
#define CNN_DMA_IS_ACC_16B_MASK			0x1

#define CNN_FORMAT_INPUT_LSB		0	/* VPUH_CNN_FORMAT_* values */
#define CNN_FORMAT_INPUT_MASK			0x3
#define CNN_FORMAT_OUTPUT_LSB		2	/* VPUH_CNN_FORMAT_* values */
#define CNN_FORMAT_OUTPUT_MASK			0x3
#define CNN_FORMAT_SLGR_16F_LSB		4
	/* If set temporary slice groups results are save in 16F format
	 * else in 26F on 32 bits
	 */
#define CNN_FORMAT_SLGR_16F_MASK		0x1
#define CNN_FORMAT_IS_ACC_DATA_LSB	5
	/* Whether external accumulated data exists */
#define CNN_FORMAT_IS_ACC_DATA_MASK		0x1
#define CNN_FORMAT_ACC_16F_LSB		6
	/* If set the external accumulated data is in 16F format else in 26F format */
#define CNN_FORMAT_ACC_16F_MASK			0x1

struct VPUI_CnnDesc {
	__u8	CfgBits;
		/* CNN_CFG_* fields */
	__u8	FilterSizesBits;
		/* CNN_FILTER_WIDTH + CNN_FILTER_WIDTH */
	__u8	DmaBits;
		/* CNN_DMA_* fields */
	__u8	FormatBits;
		/* CNN_FORMAT_* fields */
	__u8	CropIndP1;
		/* In VPUI_CnnCropDesc vector. Relevant only when
		 * CNN_CFG_IS_XY_CROP_LSB is set. If set to 0
		 * output XY sizes = XY Input sizes - (filter sizes - 1)
		 */
	__u8	ReluInd;
		/* In VPUI_CnnReluDesc vector */
};

/******************* 3DNN Layer description **********************************/
struct VPUI_3DNNLayer {
	/* Index in 3dProc IoXySizes vector */
	__u8 IoXySizesIndices[VPUI_XYSIZE_ALL];
	/* From entire task tiles VPUI_Tile. 0 stands for "no tiles".*/
	__u8  InputTileIndP1;
	/* From entire task tiles VPUI_Tile. 0 stands for "no tiles".*/
	__u8  OutputTileIndP1;
	/* From entire task tiles VPUI_Tile. 0 stands for "no tiles".*/
	__u8  LastSlGrOutTileIndP1;
	/* Total number of input slices */
	__u16 TotInputSlicesNum;
	/* Input slices in each slices group (except the last) */
	__u16 GroupInputSlicesNum;
	/* Total number of output slices */
	__u16 TotOutputSlicesNum;
	/* The offset in bytes between the 1st coeff data of slice group to the 1st coeff data of
	 * next slice group for each DMA.It is used in order to update the coeffs IO pointers after
	 * each slices group. A different value is used by 16 bits DMAs and 32 bits DMAs since 32
	 * bits DMAs contain more data. The "coefficients data unit in 16 bits words" is equal to
	 * Ceil(All Coeffs size /Number of coeffs DMAs). 16 bits DMAs have single data unit and 32
	 * bits DMAs have 2 data units.
	 * In "Number of coeffs DMAs" each 32 bits DMA is counted as 2 coeffs DMAs.
	 * "All Coeffs size" = Bias * output slices number + FilterSize^2 * input slices number *
	 * output slices number where Bias is 2 (if 32 bits bias exists or 0). If bias exists- bias
	 * value for each output slice is located before the input slices filters of the output
	 * slice.The offset from DMA data of slices group to next slices group must be >= number of
	 * words read by the DMA.Since for location dependent the filters part in "All Coeffs size"
	 * is multiplied by In_X * In_Y the DMA offset may exceed 16 bits.
	 * Notice that CoeffsSlGrOfsBytesDma is equal for all DMAs of the same type so in case that
	 * Coeffs size /Number of coeffs DMAs has reminder some DMAs will require data padding.
	 */
	__u16 CoeffsSlGrOfsBytesDma16Lsbs;
	__u16 CoeffsSlGrOfsBytesDma16Msbs;
	__u16 CoeffsSlGrOfsBytesDma32Lsbs;
	__u16 CoeffsSlGrOfsBytesDma32Msbs;
	/* In the vector of CNN Factors. Factor in VPUH_CNN_FORMAT_16F is multiplied by the input */
	__u8	FstInputFlScaleFactor;
	/* In the vector of CNN Factors. Factor in VPUH_CNN_FORMAT_16F is multiplied by the input */
	__u8	LastInputFlScaleFactor;
	/* VPUI_CnnDesc is used for only for CNN HW values */
	struct VPUI_CnnDesc CnnDesc;
};

/***************** 3DNN Process description **********************************/
/************** 3DNN Process total description *******************************/
/*********** Totals used for 3DNN Process and Task ***************************/
struct  VPUI_Proc3DNNAndTaskTotals {
	__u8	Hw3DNNSlGrTypeSubCh;	/* VPUI_Hw3DNNSlGrTypeSubCh */
	__u8	Hw3DNNSubCh;		/* VPUI_Hw3DNNSubCh */
	__u8	IoXY3DNNSizes;		/* VPUI_3DNNIoXYSizes */
	__u8	InOut3DNN;
		/**
		 * VPUI_InOut3DNN - for 3DNN process equal to
		 * InOut3DNN of each Layer * Layers3DNN
		 */
	__u8	CropInd;
		/**
		 * for 3DNN process equal to
		 * CropIndices of each Layer * Layers3DNN
		 */
	__u8	ScaleInd;
		/**
		 * for 3DNN process equal to
		 * ScaleIndices of each Layer * Layers3DNN
		 */
	__u8	Sizes3DOp;
		/**
		 * for 3DNN process this is the size of the "vector of sizes"
		 * used by the blocks.
		 */
	__u8	Layers3DNN;			/* VPUI_3DNNLayer */
};

/************ Slices Group Type Parameters description ***********************/
/**
 * Some parameters may have different values for First / Middle / Last / Single
 * layer. One of these parameters might be for example whether CNN enables
 * accumulated data. Each such parameter has an offset to the HW blocks
 * parameters vector and default value for first slices group (of each layer).
 * There are few optional "updates" above default values - updates for single
 * slices group (that will be invoked for each layer with single slices group),
 * updates for 2nd slices group and updates for last slices group. Each update
 * has also indices and values.
 */
enum VPUI_3DNNSlGrParamsType {
	VPUI_SLGR_PARS_FST		= 0,
		/* Parameters for 1st invocation of each layer */
	VPUI_SLGR_PARS_SINGLE		= 1,
		/* Parameter for each layer with single slices group */
	VPUI_SLGR_PARS_SCND		= 2,
		/* Parameters for 2nd slice group of each layer (that doesn't
		 *  have single slices group) - even if has 2 slices groups
		 */
	VPUI_SLGR_PARS_LAST		= 3,
		/* Parameters for last slices group of each layer (that doesn't
		 * have single slices group)
		 */
	VPUI_SLGR_PARS_ALL		= 4
};

/****************** Totals per each layer ************************************/
struct  VPUI_Proc3DNNEachLayerTotals {
	__u8	InOut3DNN;
	/* Number of VPUI_InOut3DNN in the input vector of InOut types
	 * to HW 3DNN sub-chain (some of them are not used for each
	 * slices group)
	 */
	__u8	CropIndices;
	/* Number of Crop operations allowed for each layer. Indices in the
	 * entire task crop operations vector
	 */
	__u8	ScaleIndices;
	/* Number of Scale operations allowed for each layer. Indices in the
	 * entire task Scale operations vector
	 */
	__u8	HwParams;
	/* Number of HW parameters updated for each layer */
	__u8	ProcParams;
	/* Number of Process parameters updated for each layer.
	 * Used for parameters that are updated according to slices group type
	 */
	__u8	DummyAlign;
};

struct  VPUI_Proc3DNNOnlyTotals {
	__u8	SlGrTypeParams[VPUI_SLGR_PARS_ALL];
	/**
	 * Number of parameters updated in parameters vector for first /
	 * Single/ Second / Last slice group of each layer
	 */
	__u8	MaxWorkSizeIndices;
	/**
	 * Maximal working area indices required for HW read and CPU
	 * operations in the 3D process.
	 * Must be <= VPUI_MAX_WORK_AREA_INDICES
	 */
	__u8	DummyAlign;
	struct VPUI_Proc3DNNEachLayerTotals	EachLayer;
};

struct VPUI_Proc3DNNTotals {
	struct VPUI_AllTotals			All;
	struct VPUI_Proc3DNNAndTaskTotals	Task;
	struct VPUI_Proc3DNNOnlyTotals		Only;
};

/*************** 3DNN Proc offset description ********************************/
struct VPUI_3DNNParamsOfs {
	__u16	HwParamsValues;
		/* From parameters values of the process */
	__u16	CpuParamsValues;
		/* From parameters values of the process */
	struct VPUI_ParamsOfs SlGrType[VPUI_SLGR_PARS_ALL];
	       /**
		* Replaced parameters (in HW parameters) for 1st /
		* single / 2nd / last slice group (values + indices)
		*/
	struct VPUI_ParamsOfs LayersHwParams;
	       /**
		* Replaced parameters (in HW parameters) for each layer
		* (Each Layer HwParams * N Layers values / Each Layer HwParams
		* indices)
		*/
	struct VPUI_ParamsOfs LayersProcParams;
	       /**
		* Replaced parameters (in Proc parameters) for each layer
		* (Each Layer ProcParams * N Layers values / Each Layer
		* ProcParams indices)
		*/
};

struct  VPUI_Proc3DNNFst {
	struct VPUI_ProcAnd3DNNProcFst Both;
	/* First for VPUI_Proc3DNNAndTaskTotals */
	__u8	Hw3DNNSlGrTypeSubCh;	/* VPUI_Hw3DNNSlGrTypeSubCh */
	__u8	Hw3DNNSubCh;	/* VPUI_Hw3DNNSubCh */
	__u8	IoXY3DNNSizes;	/* VPUI_3DNNIoXYSizes */
	__u8	InOut3DNN;		/* VPUI_InOut3DNN */
	__u8	CropInd;		/* uint8 Crop indices */
	__u8	ScaleInd;		/* uint8 Scale indices */
	__u8	Sizes3DOp;		/* VPUI_3DSizesOp */
	__u8	Layers3DNN;		/* VPUI_3DNNLayer */
};

struct VPUI_Proc3DNNBase {
	struct VPUI_Proc3DNNTotals	Totals;
	struct VPUI_Proc3DNNFst		Fst;
	/**
	 * Notice that most the parameters indices are indices in block
	 * parameters of Process - that is 1st parameter of 1st block of Proc
	 * will have index == 0
	 */
	struct	VPUI_3DNNParamsOfs Params3DNNOfs;
		/* In parameters values & parameters indices vectors */
};

/***************** 3DNN Process description **********************************/
/* Also use some task vectors */
struct VPUI_Proc3DNNDesc {
	__u8	Base3DNNInd;		/* In VPUI_Proc3DNNBase vector */
	__u8	Priority;
	__u8	FstLayer;		/* From Layers3DNN of the VPUI_Proc3DNNBase */
	__u8	LastLayer;		/* From Layers3DNN of the VPUI_Proc3DNNBase */
	/*The assumption is that each layer might be invoked only once and the
	 * invocation order is fix (if more than 1 invocation of same layer or
	 * Layer K+ required before Layer K few Proc3DNN vertices are required)
	 */
};

/**
 * ************* 32bit address / offset description ***************************
 * Since all fields in interface are 16 bits we split the "DRAM Addresses
 * offsets from DRAM base" and the slot offsets to 2 fields
 */
struct VPUI_32bMem {
	__u16 LsWord;
	__u16 MsWord;
};

/**
 * ******************* Slot memory description ********************************
 * Describe which external memory addresses indices are different between the
 * different invocation slots of the task
 */
struct VPUI_SlotMem {
	__u8	BaseAdrInd;
		/* In external memory addresses vector */
	__u8	SlotOfsInd;
		/* In slots offset vector */
};

/***************** Task vertex description ***********************************/
enum VPUI_TaskVertexType {
	VPUI_TVER_ST			= 0,
	/**
	 * Start vertex - no operation + no input edges. For each task it
	 * should be the Type of the 1st vertex only
	 */
	VPUI_TVER_END			= 1,
	/**
	 * End task vertex - report on "end operation" + no output edges.
	 * For each task it should be the Type of the last vertex only
	 */
	VPUI_TVER_PROC			= 2,
	/* process */
	VPUI_TVER_3DNN_PROC		= 3,
	/* 3DNN process */
	VPUI_TVER_HOST_REP		= 4,
	/* Report to HOST */
	VPUI_TVER_DUMMY			= 5
	/**
	 * no operation - reduce number of edges in "many to many"
	 * connections
	 */
};

struct VPUI_TaskVertex {
	__u8	Type;
		/* VPUI_TaskVertexType */
	__u8	Index;
		/* The index of Process / 3DNN process / Host report */
	__u8	OutAllEdgesNum;
		/**
		 * Total number of edges. In case of LoopEndIndP1 != 0 includes
		 * the backward edge (will be always the 1st edge) and all
		 * other edges.
		 */
	__u8	FstOutEdgeIndex;
	__u8	EndLoopEdgesNum;
		/**
		 * Edges that are invoked only on the end of the loop
		 * when LoopEndIndP1 != 0. On this case the 1st edge is
		 * the backward edge, than OutAllEdgesNum - EndLoopEdgesNum - 1
		 * edges that are always invoked and than the EndLoopEdgesNum
		 * edges.
		 */
	__u8	LoopOpIndP1;
		/**
		 * The index of loop operated from this vertex.
		 * 0 means "no loop". Single vertex always "start" the loop
		 * (optionally "dummy" vertex)
		 */
	__u8	LoopEndIndP1;
		/**
		 * The index of loop ended at this vertex. 0 means "no loop".
		 * Loop is invoked by backward edge to the vertex that
		 * starts the loop
		 */
	__u8	InEdgesGroupsNum;
		/* Actually required only for validity verification code */
	__u8	FstInEdgesGrIndex;
		/* In uint8 "groups totals" vector */
	__u8	TotalParams;
		/**
		 * total number of parameters used by this vertex (sum of all
		 * parameters of the HW blocks + loop parameters + pre-loads)
		 * For VPUI_TVER_END / VPUI_TVER_HOST_REP this is the number of
		 * results that are copied from output vector to the report
		 */
	__u16	FstParamOfs;
		/**
		 * Offset to 1st parameter of the vertex in the entire task
		 * vector. For VPUI_TVER_END / VPUI_TVER_HOST_REP this is the
		 * index of the output slot of the values that should be copied
		 * to the report
		 */
};

/***************** Task edge description *************************************/
struct VPUI_TaskEdge {
	__u8	DestVerInd;
		/* The index of destination Vertex */
	__u8	DestGrIndP1;
		/**
		 * The index of the group in the destination Vertex + 1.
		 * 0 stands for "No Group" => Vertex should be invoked
		 */
	__u8	IndInDestGr;
		/**
		 * The index of the current edge in the Group in the
		 * destination Vertex
		 */
	__u8	DisableBitP1;
		/**
		 * The index of disable bit in the disabled edges mask + 1.
		 * 0 stands for non conditional edge.
		 */
};

/***************** Task loop description *************************************/
struct VPUI_TaskLoop {
	__u8	LoopsNum;
		/* 0 stands for CPU decision. */
#ifdef OLD_FWIF_IO
	__u8	DummyAlign;
	struct	VPUI_LoopTotParams ParamsTotals;
		/**
		 * Number of parameters sets (usually equal LoopsNum)
		 * and number of parameters per each loop + 1st / not loop
		 */
	struct	VPUI_LoopParamsOfs ParamsOffsets;
		/**
		 * The offset in the vector of all parameters indices and
		 * values for "each loop" and "first loop" parameters
		 */
#else
	__u8	ParamsIndP1;
		/* 0 stands for No parameters. */
#endif
};

/******************** Task totals description ********************************/
struct VPUI_TaskTotals {
	/* VPUI_BlUsage + VPUI_HWConnect +HwMprbsGr +RamsMprbsGr + InternalRamsInd + InternalRams +
	 * HwReadResBl + CpuOps + StaticFiltCoeffs + VPUI_SubCh + ParamsValues + ParamsIndices
	 * (Parameters Division described in VPUI_TaskParamsParts)
	 */
	struct	VPUI_AllTotals			AllTotals;
	/* VPUI_HwSubCh + PreSetsMemVecs + PostSetsMemVecs + VPUI_InOut + VPUI_Roi +
	 * VPUI_FixExRoi + VPUI_DynMapRoi + VPUI_SizesOp
	 */
	struct	VPUI_ProcAndTaskTotals		ProcTotals;
	/* VPUI_Hw3DNNSlGrTypeSubCh + VPUI_Hw3DNNSubCh + VPUI_3DNNIoXYSizes + VPUI_InOut3DNN +
	 * u8 CropInd + u8 ScaleInd
	 */
	struct	VPUI_Proc3DNNAndTaskTotals	Proc3DNNTotals;
	__u16	SepFiltCoeffSets;		/* VPUI_FiltCoeffsSets */
	__u16	GenFiltCoeffSets;		/* VPUI_FiltCoeffsSets */
	__u16	FiltStaticCoeffs;		/* uint16 vector. */
	/* part of uint16 parameters values vector - used only for verification */
	__u16	FiltDynCoeffs;
#ifdef OLD_FWIF_IO
	__u16	DisparityFixedThr;		/* VPUI_DisparityFixedThr */
	__u16	DepthDynThr;			/* VPUI_DepthDynThr */
	__u16	InPaintDynThr;			/* VPUI_InPaintDynThr */
#endif
	__u16	RamsMprbsIndGr;
	__u16	OutputTotal16BSize;		/* Allocated 16b output size */
#ifndef OLD_FWIF_IO
	__u16	MaxDynExRois;			/* VPUI_DynExRoi of VPUI_RoiList with List.IsDyn */
	__u8	DynRoisVecsOffsets;		/* uint16 vector */
	__u8	DynPtsVecsOffsets;		/* uint16 vector */
	__u8	DisparityFixedThr;		/* VPUI_DisparityFixedThr */
	__u8	DepthDynThr;			/* VPUI_DepthDynThr */
	__u8	InPaintDynThr;			/* VPUI_InPaintDynThr */
#endif
	__u8	HistParams;			/* VPUI_HistParams */
	__u8	OutputOffsets;			/* uint16 vector */
	/* Number of MPRBs indices groups in all internal RAMs of task */
	__u8	Tiles;				/* VPUI_Tile */
	__u8	FixOps;				/* VPUI_FixOp */
	__u8	CropOps;			/* VPUI_CropOp */
	__u8	ScaleOps;			/* VPUI_ScaleOp */
	__u8	Procs;				/* VPUI_ProcDesc */
	__u8	Procs3DNNBase;			/* VPUI_Proc3DNNBase */
	__u8	Procs3DNN;			/* VPUI_Proc3DNNDesc */
	__u8	CnnCropDesc;			/* VPUI_CnnCropDesc */
	__u8	ReluDesc;			/* VPUI_CnnReluDesc */
	__u8	CNNScalers;	/* vector of uint16 VPUH_CNN_FORMAT_16F */

	/* memory handling */
	/* VPUI_32bMem- ext memory offsets from DRAM base -used by VPUI_MapDesc and VPUI_SlotMem */
	__u8	ExternalBaseAdrOfs;
#ifdef OLD_FWIF_IO
	/* VPUI_ImageDesc - part of uint16 parameters values vector - used only for verification */
	__u8	ImageDesc;
#else
	__u8	ImageSizes;			/* VPUI_Sizes */
	__u8	ImageDesc;			/* VPUI_ImageDesc */
#endif
	__u8	MapDesc;			/* VPUI_MapDesc */

	/* memory handling - Task level */
	/* VPUI_32bMem - external memory offsets between slots (used by VPUI_SlotMem) */
	__u8	ExtSlotOfs;
	__u8	SlotMem;			/* VPUI_SlotMem */
	/* Number of external memory addresses explicitly set each task invocation. */
	__u8	EachInvokeExtMemAdr;

	/* Graph description */
	__u8	Vertex;				/* VPUI_TaskVertex */
	__u8	Edges;				/* VPUI_TaskEdge */
	/* Groups of input edges of all vertices - each has total number of edges in the group */
	__u8	InEdgesGroups;
	__u8	TaskLoops;			/* VPUI_TaskLoop */
};

/****** Task offsets in entire Task Creation data structure description ******/
struct VPUI_TaskCreateOfs {
	__u16	Blocks;			/* Offset to vector of VPUI_BlUsage in all processes */
	/* Offset to vector of uint16 - all types parameters values.
	 * Vector Division is described in VPUI_TaskParamsValuesOfs
	 */
	__u16	AllParamsValues;
	__u16	HwConnect;		/* Offset to vector of VPUI_HWConnect in all processes */
					/* HwMprbsGr + RamsMprbsGr set on TaskAllocate */
	__u16	InternalRamsInd;	/* Offset to vector of uint8 - internal RAMs indices */
	__u16	InternalRams;		/* Offset to vector of VPUI_InternalRam */
	__u16	HwReadResBl;		/* Offset to vector of Bl Indices */
	__u16	CpuOps;			/* Offset to vector of VPUI_CpuOp */
	__u16	CpuSubCh;		/* Offset to vector of VPUI_CpuSubCh */
	__u16	SepFiltCoeffSets;	/* VPUI_FiltCoeffsSets */
	__u16	GenFiltCoeffSets;	/* VPUI_FiltCoeffsSets */
	__u16	FiltStaticCoeffs;	/* Offset to vector of int16 */
					/* FiltDynCoeffs are part of AllParamsValues vector */
	__u16	HistParams;		/* VPUI_HistParams */
	__u16	DisparityFixedThr;	/* VPUI_DisparityFixedThr */
	__u16	DepthDynThr;		/* VPUI_DepthDynThr */
	__u16	InPaintDynThr;		/* VPUI_InPaintDynThr */
		/* Offset to vector of output allocated per each slot but no initialization data */
	__u16	OutputOffsets;		/* uint16 vector. */
#ifndef OLD_FWIF_IO
		/* Offset to vector of Rois allocated per each slot but no initialization data */
	__u16	DynRoisVecsOffsets;	/* uint16 vector */
		/* Offset to vector of Pts allocated per each slot but no initialization data */
	__u16	DynPtsVecsOffsets;	/* uint16 vector */
#endif
	__u16	SubCh;			/* Offset to vector of VPUI_SubCh */
	/* Data for Procs only */
	__u16	HwSubCh;		/* Offset to vector of VPUI_HwSubCh */
#ifdef OLD_FWIF_IO
	__u16	InOutTypes;		/* Offset to vector of VPUI_InOutType. */
	__u16	PtLists;		/* Offset to vector of VPUI_PtList. */
	__u16	PtRoiSizes;		/* Offset to vector of VPUI_Sizes. */
	__u16	RoiFromPt;		/* Offset to vector of VPUI_RoiFromPt. */
	__u16	PtMapIndices;		/* Offset to vector of uint8 */
	__u16	FixPoints;		/* Offset to vector of VPUI_Point. */
	__u16	Rois;			/* Offset to vector of VPUI_Roi (for VPUI_FixMapRoi) */
	__u16	FixMapRois;		/* Offset to vector of VPUI_FixMapRoi */
			/* VPUI_DynMapRoi allocated per each slot but no initialization data */
#else
	__u16	MapSetsIndices;		/* Offset to vector of uint8 */
	__u16	MapSets;		/* Offset to vector of VPUI_MapsSet */
	__u16	Rois;			/* Offset to vector of VPUI_Roi (for VPUI_FixExRoi) */
	__u16	RoisFromMapsSets;	/* Offset to vector of VPUI_RoiFromMapsSets */
	/* vector of VPUI_DynExRoi allocated per each slot but no initialization data */
	__u16	FixExRois;		/* Offset to vector of VPUI_FixExRoi */
	__u16	RoiLists;		/* Offset to vector of VPUI_RoiList */
	__u16	FixPoints;		/* Offset to vector of VPUI_Point. */
	__u16	PtLists;		/* Offset to vector of VPUI_PtList. */
	__u16	InitListsPtrOps;	/* Offset to vector of VPUI_ListPtrOp */
	__u16	UpListsPtrOps;		/* Offset to vector of VPUI_ListPtrOp */
	__u16	PtRoiSizes;		/* Offset to vector of VPUI_Sizes. */
	__u16	RoiFromPt;		/* Offset to vector of VPUI_RoiFromPt. */
	__u16	Scans;			/* Offset to vector of VPUI_Scan */
	__u16	RoisFromScans;		/* Offset to vector of VPUI_RoiFromScan */
	__u16	RoisFromTiles;		/* Offset to vector of VPUI_RoiFromTile */
	__u16	RoisCopy;		/* Offset to vector of VPUI_RoiCopy */
	__u16	IORois;			/* Offset to vector of VPUI_IORoi */
	__u16	Vectors;		/* Offset to vector of VPUI_Vector */
	__u16	DynOutLists;		/* Offset to vector of VPUI_DynOutList */
	__u16	InOuts;			/* Offset to vector of VPUI_InOut */
	__u16	IterLoops;		/* Offset to vector of uint8 */
	__u16	PrLoopEntities;		/* Offset to vector of VPUI_PrLoopEntity */
	__u16	LoopsParams;		/* Offset to vector of VPUI_LoopParams */
	__u16	ProcLoops;		/* Offset to vector of VPUI_ProcLoop */
					/* InputSetsSizes are part of AllParamsValues vector */
#endif
	__u16	SizesOp;		/* Offset to vector of VPUI_SizesOp */
	__u16	Tiles;			/* Offset to vector of VPUI_Tile */
	__u16	FixOps;			/* Offset to vector of VPUI_FixOp */
	__u16	CropOps;		/* Offset to vector of VPUI_CropOp */
	__u16	ScaleOps;		/* Offset to vector of VPUI_ScaleOp */
	__u16	Procs;			/* Offset to vector of VPUI_ProcDesc */

	/* Data for 3DProcs */
	__u16	Hw3DNNSlGrTypeSubCh;	/* Offset to vector of VPUI_Hw3DNNSlGrTypeSubCh */
	__u16	Hw3DNNSubCh;		/* Offset to vector of VPUI_Hw3DNNSubCh */
	__u16	IoXY3DNNSizes;		/* Offset to vector VPUI_3DNNIoXYSizes */
	__u16	InOut3DNN;		/* Offset to vector of VPUI_InOut3DNN */
	__u16	CropInd;		/* Offset to vector of uint8- indices of Crop operations */
	__u16	ScaleInd;		/* Offset to vector of uint8-indices of Scale operations */
	__u16	Sizes3DOp;		/* Offset to vector of VPUI_3DSizesOp */
	__u16	Layers3DNN;		/* Offset to vector of VPUI_3DNNLayer */
	__u16	Procs3DNNBase;		/* Offset to vector of VPUI_Proc3DNNBase */
	__u16	Procs3DNN;		/* Offset to vector of VPUI_Proc3DNNDesc */
	__u16	CnnCropDesc;		/* Offset to vector of VPUI_CnnCropDesc */
	__u16	ReluDesc;		/* Offset to vector of VPUI_CnnReluDesc */
	__u16	CNNScalers;		/* Offset to vector of VPUH_CNN_FORMAT_16F */

	/* memory handling */
	/* external memories addresses are set on task allocation */
#ifdef OLD_FWIF_IO
	/* Offset to vector of VPUI_ImageDesc in ParamsValues vector */
	__u16	PreMemVecDesc;		/* Offset to vector of VPUI_MemVecDesc */
	__u16	PostMemVecDesc;		/* Offset to vector of VPUI_MemVecDesc */
#else
	__u16	ImageSizes;		/* Offset to vector of VPUI_Sizes */
	__u16	ImageDesc;		/* Offset to vector of VPUI_ImageDesc */
#endif
	__u16	MapDesc;		/* Offset to vector of VPUI_MapDesc */

	/* memory handling - Task level */
	__u16	SlotOfs;		/* Offset to vector of VPUI_32bMem-offsets between slots */
	__u16	SlotMem;		/* Offset to vector of VPUI_SlotMem */
	/* Offset to vector of indices of external mem adr explicitly set each task invocation */
	__u16	EachInvokeExtMemAdrInd;

	 /* Graph description */
	__u16	Vertex;			/* Offset to vector of VPUI_TaskVertex */
	__u16	Edges;			/* Offset to vector of VPUI_TaskEdge */
	__u16	InEdgesGroups;		/* Offset to vector of uint8 - total number of edges in
					 * each of the input edges groups
					 */
	__u16	TaskLoops;		/* Offset to vector of VPUI_TaskLoop */

	/* Parameters total. */
	/* Offset to vector of uint16 - all types parameters indices.
	 * Vector Division is described in VPUI_TaskParamsIndicesOfs
	 */
	__u16	AllParamsIndices;
};

/****************** Task parameters description ******************************/
/* The parameters and indices vectors are divided to different parts */

#ifdef OLD_FWIF_IO
/* Offset in parameters values vector for each Task */
enum VPUI_TaskSpParamsOfs {
	VPUI_TASKPAR_DISABLE_EDGES_MASK_OFS	= 0,
	VPUI_TASKPAR_ALL			= 1
};
#else
/* Assuming the first parameters of each task is always Disable Edges mask */
#define VPUI_TASKPAR_DISABLE_EDGES_MASK_OFS 0
#endif

struct VPUI_TaskParamsValuesOfs {
#ifdef OLD_FWIF_IO
	/* Assuming VPUI_TASKPAR_ALL always at the beginning of the vector */
#endif
	/* Total = Proc.ParamsValues Each process parameters offset is from this point */
	__u16	Procs;
#ifndef OLD_FWIF_IO
	/* Total = Proc.InputSetsSizes. Each process InputSetsSizes offset is from this point */
	__u16	InputSetsSizes;
#endif
	/* summery of data derived for VPUI_TaskLoop.ParamsTotals -
	 * SetsNum * ExpPerSet + FixInc*2 + PerFst*2. Each loop values offset is from this point
	 */
	__u16	TaskLoops;
#ifdef OLD_FWIF_IO
	__u16	ImageDesc;	/* Total = ImageDesc */
#else
	__u16	DynSizes;	/* Total = Dynamic sizes *  VPUI_SIZES_TOT_PARAMS */
#endif
	__u16	DynFiltCoeffs;	/* Total = FiltDynCoeffs */
};

struct VPUI_TaskParamsIndicesOfs {
	__u16	Procs;	/* Total = Proc.ParamsIndices */
	/* summery of data derived for VPUI_TaskLoop.ParamsTotals - ExpPerSet + FixInc + PerFst */
	__u16	TaskLoops;
	__u16	TaskAllocate;
	__u16	TaskInvoke;
};

struct VPUI_TaskParamsParts {
	__u16	TaskLoopsTotal;	/* In all VPUI_TaskLoop together */
	__u16	TaskAllocate;	/* Number of parameters updated on task allocate */
	__u16	TaskInvoke;	/* Number of parameters updated on each task invocation */
	/* Offsets of different types of parameters in the entire parameters values vector */
	struct	VPUI_TaskParamsValuesOfs	ValuesOfs;
	/* Offsets of different types of parameters in the entire parameters indices vector */
	struct	VPUI_TaskParamsIndicesOfs	Indices;
};

/******************* Task header description *********************************/
struct VPUI_TaskCrHeader {
	/* Task priority (compared to other tasks priority on HW allocation race). */
	__u8	Priority;
	/* Whether same HW (BlInst/ HwConnections/ Hw/RamsMprbsGr) is used in all task instances */
	__u8	IsFixedHw;
};

/****************** Task creation description ********************************/
struct VPUI_TaskCreate {
	struct	VPUI_TaskCrHeader	Header;
	__u16	EntireDateSize;				/* In bytes. Just for validity check */
	struct	VPUI_TaskTotals		Totals;
	struct	VPUI_TaskCreateOfs	DataOfs;	/* Offsets of data sent on task creation */
	struct VPUI_TaskParamsParts	ParamsParts;
};

/***** Task offsets in entire Task Allocation data structure description *****/
struct VPUI_HwSlotsOfs {
	/**
	 * HW explicit Data (BlInstances/ HwConnections/ HwMprbsGr/ RamsMprbsGr) may have different
	 * values per each slot. We may use default values for all slot. On this case IsDefaultUsed
	 * is set, DefaultOfs is the offset of default values and their are TotalEachSlot elements
	 * for each slot that are explicitly described.Each slot description includes single vector
	 * of indices and continuous vectors of values - each vector describes single slot. Another
	 * option is to explicitly describe all elements of each slot. On this case IsDefaultUsed
	 * isn't set, DefaultOfs is ignored and TotalEachSlot should be equal to the total number
	 * of elements. Notice that when IsFixedHw in VPUI_TaskCreate is set IsDefaultUsed must be
	 * set and TotalEachSlot must be 0 for all types of HW.
	 */
	__u8	IsDefaultUsed;	/* whether default values for all slots are used */
	__u8	DummyAlign;
	__u16	DefaultOfs;	/* The offset to the vector of default values */
	__u16	TotalEachSlot;	/* Total number of explicit values per slot */
	__u16	EachSlotIndicesOfs; /* The offset to the vector of indices of "each slot" values */
	/* The offset to the vectors of values of "each slot" - 1st slot values, 2nd slot etc. */
	__u16	EachSlotValuesOfs;
};

struct VPUI_TaskAllocOfs {
	/* Offset to values of VPUI_32bMem external memory addresses offsets from DRAM_BASE */
	__u16	ExternalMemAdrOfs;
	/* Offset to uint16 vector of parameter values. Their indices are described on task creation. */
	__u16	ParamsValuesOfs;
	/* Describe the uint8 HW blocks instances indices - index per each block in VPUI_BlUsage vector */
	struct	VPUI_HwSlotsOfs	BlInstances;
	/* Describe the VPUI_MprbGr blocks MPRBsGr. */
	struct	VPUI_HwSlotsOfs	HwMprbsGr;
	/* Describe the VPUI_MprbGr internal RAM MPRBsGr. */
	struct	VPUI_HwSlotsOfs	RamsMprbsGr;
	/* Describe the uint8 internal RAM virtual base address in RAM unit. A virtual address for
	 * each internal RAM.
	 */
	struct	VPUI_HwSlotsOfs	IntRamFstVirtUnit;
};

/****************** Task allocate description ********************************/
struct VPUI_TaskAlloc {
	__u8	SlotsNum;
	__u8	DummyAlign;
	__u16	EntireDateSize;	/* In bytes. Just for validity check */
	/* The totals of data set in task allocation described on task creation */
	/* Offsets of data sent on task allocation */
	struct	VPUI_TaskAllocOfs	DataOfs;
};

/************* Task update parameters description ****************************/
struct VPUI_UpdateSingleParam {
	__u16	Value;	/* The updated value */
	__u16	Index;	/* The index in the entire task parameters vector of the parameter */
};

struct VPUI_UpdateParamsVector {
	__u16	ParamsNum;		/* Total number of parameters in the vector */
	/* The index in the entire task parameters vector of the 1st parameter to update
	* (the indices of vector are continuous)
	*/
	__u16	FstParamIndex;
	__u16	FstParamValueOfs;	/* The offset to the 1st value in VectorParamsValues */
};

struct VPUI_TaskUpdateParamsOfs {
	__u16	SingleParamsOfs;	/* Offset to vector of VPUI_UpdateSingleParam */
	__u16	VectorParamsDescOfs;	/* Offset to vector of VPUI_UpdateParamsVector */
	__u16	VectorParamsValuesOfs;	/* Offset to vector of uint16 parameters values */
};

struct VPUI_TaskUpdateParams {
	__u16	SingleParamsNum;	/* Each has VPUI_UpdateSingleParam description */
	__u16	VectorParamsNum;	/* Each has VPUI_UpdateParamsVector description */
	__u16	AllParamsNum;		/* Each is uint16 */
	__u16	EntireDateSize;		/* In bytes. Just for validity check */
	/* Offsets of data sent on task parameters update */
	struct	VPUI_TaskUpdateParamsOfs DataOfs;
};

/****************** Task invoke description **********************************/
struct VPUI_TaskInvokeOfs {
	__u16	ParamsValues;
		/**
		 * ParamsParts.TaskInvoke parameters values. The parameters
		 * indices in the vector are described on task creation.
		 */
	__u16	MemAddresses;
		/**
		 * Totals.EachInvokeExtMemAdr addresses values. The addresses
		 * indices in the vector described on task creation.
		 */
};

#define VPUI_INVALID_TASK_INVOKE_ID	0

struct VPUI_TaskInvoke {
	/* Unique (per task) invocation Id. Will be used on Core reports.
	 * Must be != VPUI_INVALID_TASK_INVOKE_ID
	 */
	__u16	InvokeIdLsb;
	__u16	InvokeIdMsb;
	__u16	EntireDateSize;			/* In bytes. Just for validity check */
	struct	VPUI_TaskInvokeOfs DataOfs;	/* Offsets of data sent on task invocation */
};

/* Prefix for all commands */
struct VPUI_CmndHdr {
	__u16	CmndType;	/* VPUI_CmndType */
	/* Not relevant for system commands - VPUI_CMND_INIT, VPUI_CMND_REALLOC_LOWPR_CMD_MBX */
	__u16	TaskId;
};

struct VPUI_Cmnd {
	struct VPUI_CmndHdr	Hdr;
	union	{
		/* none for VPUI_CMND_DESTROY_TASK / VPUI_CMND_FREE_TASK / VPUI_CMND_ABORT_TASK */
		/* VPUI_CMND_INIT */
		struct VPUI_InitCmnd		Init;
		/* VPUI_CMND_REALLOC_LOWPR_CMD_MBX */
		struct VPUI_ReAllocMbox		AllocMbox;
		/* VPUI_CMND_CREATE_TASK */
		struct VPUI_TaskCreate		Create;
		/* VPUI_CMND_ALLOCATE_TASK / VPUI_CMND_REALLOC_TASK */
		struct VPUI_TaskAlloc		Allocate;
		/* VPUI_CMND_UPDATE_TASK_PARAMS */
		struct VPUI_TaskUpdateParams	UpdateParams;
		/* VPUI_CMND_INVOKE_TASK */
		struct VPUI_TaskInvoke		Invoke;
	} Extra;
	/* The dynamic data of VPUI_CMND_CREATE_TASK / VPUI_CMND_ALLOCATE_TASK /
	 * VPUI_CMND_REALLOC_TASK / VPUI_CMND_UPDATE_TASK_PARAMS / VPUI_CMND_INVOKE_TASK
	 */
};

/**
 * TODO(SW-7) - Add command - Sleep + WakeUp - actually Abort all active tasks
 * + write 0 to reg_sys_cpu_ip_processing_ofs when all Tasks finished.
 * On Wakeup - write 1 ..
 */

#endif
