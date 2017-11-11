/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/** \addtogroup VPUL-DS
 *  @{
 */

#ifndef VS4L_TASK_DS_H_
#define VS4L_TASK_DS_H_

#include "vpul-pu.h"
#include "vpul-cpu_op.h"

/*! \brief get pointer to the start of vertices list */
#define fst_vtx_ptr(_task) \
	 (struct vpul_vertex *)((__u8 *)(_task) + (_task)->vertices_vec_ofs)

 /*! \brief get pointer to vertex by it's index */
#define vtx_ptr(_task, _idx) \
		(struct vpul_vertex *)(fst_vtx_ptr(_task) + (_idx))

/*! \brief get pointer to the start of 3DNN Process Bases list */
#define fst_3dnn_process_base_ptr(_task) \
	(struct vpul_3dnn_process_base *)((__u8 *)(_task) + (_task)->process_bases_3dnn_vec_ofs)

/*! \brief get pointer to the start of sub-chains list */
#define fst_sc_ptr(_task) \
	(struct vpul_subchain *)((__u8 *)(_task) + (_task)->sc_vec_ofs)

/*! \brief get pointer to sub-chain by it's index in all sub-chains vector */
#define sc_ptr(_task, _sc_idx) \
	(struct vpul_subchain *)(fst_sc_ptr(_task) + (_sc_idx))

/*! \brief get pointer to the sub-chains list of some vertex by it's pointer */
#define fst_vtx_sc_ptr(_task, _vtxptr) \
	(struct vpul_subchain *)(((__u8 *)(_task)) + ((_vtxptr)->sc_ofs))

/*! \brief get pointer to the sub-chains list of some vertex by it's index */
#define fst_vtxidx_sc_ptr(_task, _vtxidx) \
	(struct vpul_subchain *)((__u8 *)(_task) +\
		(vtx_ptr((_task), (_vtxidx)))->sc_ofs)

/*! \brief get pointer to the start of pus list */
#define fst_pu_ptr(_task) \
		(struct vpul_pu *)((__u8 *)(_task) + (_task)->pus_vec_ofs)

/*! \brief get pointer to the pus list of some subchain by it's pointer */
#define fst_sc_pu_ptr(_task, _scptr) \
		(struct vpul_pu *)(((__u8 *)(_task)) + ((_scptr)->pus_ofs))

/*! \brief get pointer to the start of updatable PUs locations list */
#define fst_updateble_pu_location_ptr(_task) \
	(struct vpul_pu_location *)((__u8 *)(_task) \
		+ (_task)->invoke_params_vec_ofs)

/*! \brief get pointer to the updatable PU location by it's index  */
#define updateble_pu_location_ptr(_task, _idx) \
		((struct vpul_pu_location *)(fst_updateble_pu_location_ptr(_task)  + (_idx)))

/*! \brief
 * Maximal number of input / output edges or edges groups per vertex
 */
#define VPUL_MAX_INOUT_EDGES			5
/*! \brief Maximal number of vertices on task graph */
#define VPUL_MAX_VERTEX_ON_TASK			164
/*! \brief Maximal task graph path length (from start to end vertex) */
#define VPUL_MAX_GRAPH_DEPTH			128
/*! \brief Maximal number of PUs in task */
#define VPUL_MAX_PU_IN_TASK			128

/*! \brief Maximal number of pre-set maps per process */
#define VPUL_MAX_PRESET_MAPS			64
/*! \brief Maximal input and output descriptors per task */
#define VPUL_MAX_IN_OUT_TYPES			64
/*! \brief Maximal fixed map ROIs */
#define VPUL_MAX_FIX_MAP_ROI			40
/*! \brief Maximal dynamic map ROIs */
#define VPUL_MAX_DYN_MAP_ROI			40
/*! \brief Max number of 3DNN layers per process base */
#define VPUL_MAX_3DNN_LAYERS_IN_PROC_BASE	8
/*! \brief Maximal 3DNN inout instances per 3DNN process */
#define VPUL_MAX_3DNN_INOUT			8
/*! \brief Max number of scaling factors for 3DNN layer */
#define VPUL_MAX_3DNN_SCALING_FACTORS		8
/*! \brief Maximal sizes operations per process */
#define VPUL_MAX_SIZES_OP			200
/*! \brief Maximal tiles descriptors per process */
#define VPUL_MAX_TILES_DESC			16
/*! \brief Maximal fix sizes descriptors per process */
#define VPUL_MAX_FIX_SIZES_DESC			16
/*! \brief Maximal crop descriptors per process */
#define VPUL_MAX_CROP_DESC			16
/*! \brief Maximal scale descriptors per process */
#define VPUL_MAX_SCALE_DESC			16
/*! \brief Maximal memory descriptors */
#define VPUL_MAX_MEMORIES_DESC			64
/*! \brief Maximal maps descriptors */
#define VPUL_MAX_MAPS_DESC			32
/*! \brief Maximal external slot offsets descriptors */
#define VPUL_MAX_EX_SLOTS_OFS_DESC		8
/*! \brief Maximal external slot offsets descriptors */
#define VPUL_MAX_SLOTS_MEMS_DESC		8
/*! \brief Maximal internal rams used by task */
#define VPUL_MAX_TASK_INTERNAL_RAMS		16
/*! \brief Maximal external rams used by task */
#define VPUL_MAX_TASK_EXTERNAL_RAMS		64
/*! \brief Maximal external MPRBS used by task */
#define VPUL_MAX_PROC_STATIC_COFF		512
/*! \brief Maximal external slot offsets descriptors */
#define VPUL_MAX_EX_SLOTS_OFS_DESC		8
/*! \brief Maximal number of input and output ports to a process */
#define VPUL_PROCESS_MAX_PORTS			5
/*! \brief Maximal parameters for CPU operation */
#define VPUL_MAX_CPU_OP_PARAMS			8
/*! \brief Minimum/ Maximum task priority.  */
#define VPUL_TASK_PRIORITY_MAX_VAL		16
#define VPUL_TASK_PRIORITY_MIN_VAL		1

/*! \brief Max map  pyramid indices */
#define VPUL_MAX_PTS_NDICES				10

/*! \brief Max points */
#define VPUL_LST_MAX_DYN_POINTS			10
#define VPUL_LST_MAX_ROI				5
#define VPUL_MAX_MAP_DESC				10



/*! \brief This structure describes parameters for update TBD */
struct vpu_update_parameters_vector_ds {
	/*! \brief The parameter offset (in bytes) on task data structure. */
	__u32				parameter_offset;
	/*! \brief Each bit represent one byte. 0x1 = update only first byte.
	 * 0x3 = update first to bytes. 0x9 = update first and 4th bytes
	 */
	__u32				parameter_mask;
	/*! \brief The parameter value */
	__u32				parameter_value;
};


/*! \brief This structure describes external memory addresses for update
 * on Invoke */
struct vpu_invoke_external_mem_vector_ds {
	/*! \brief number of buffers */
	__u32	num_of_buffers;
	/*! \brief Array holding address for all external buffers to update */
	__u32	addresses_vector[VPUL_MAX_MAPS_DESC];
};

/*! \brief This structure marks external memory addresses for to be updated
 * on Invoke */
struct vpu_mark_for_invoke_ext_mem_vec_ds {
	/*! \brief number of buffers */
	__u32	num_of_buffers;
	/*! \brief Index of mem address in task create vector */
	__u32	external_mem_index[VPUL_MAX_MAPS_DESC];
};

/** \addtogroup MBOX Mailbox Data-structures
 *  \brief Structures used to to construct mailbox messages
 *  @{
 */
/************************* Mail-box data structures **************************/
/*! \brief size of mbox messages for a task */
struct vpul_task_mbox_msg_sizes {
	/*! \brief size in bytes for task allocate message */
	__u32				allocate;
	/*! \brief size in bytes for task invoke message */
	__u32				invoke;
	/*! \brief size in bytes for destroy / abort / free messages */
	__u32				short_msg;
};
/*! \brief Mbox settings */
enum vpul_mbox_interrupt_type {
	/*! \brief Core should not update interrupt on MBOX update */
	VPUL_INTR_NONE			= 0,
	/*! \brief Core should set IRQ0 to HOST on each MBOX update */
	VPUL_INTR_IRQ0			= 1,
	/*! \brief Core should set IRQ1 to HOST on each MBOX update */
	VPUL_INTR_IRQ1			= 2
};

struct vpul_mbox_host_data {
	/*! \brief size of the mail box  in halfword (16bit) */
	__u32				size_in_u16;
	/*! \brief indicates whether to send response only when error occurs
	 */
	__u32				send_response_on_error_only;
};

struct vpul_mbox_core_data {
	/*! \brief size of the mail box  in halfword (16bit). For "core"
	 * mail-box values are in number of messages.
	 */
	__u32				n_messages;

	/*! \brief vpul_mbox_interrupt_type. Interrupt after CORE
	 * write message and updates appropriate next write pointers.
	 */
	__u32				interrupt_type;
};

/*! \brief reallocation information for Mbox */
struct vpul_mbox_reallocate {
	/*! \brief The Low priority commands MBOX new size in 16 bits units */
	__u32 new_host_normal_size_in_u16;
};

/*! \brief core init mail-box information */
struct vpul_mbox_init_info {
	/*! \brief Host Mail-box for normal commands */
	struct vpul_mbox_host_data	host_normal;
	/*! \brief Host Mail-box for low priority commands -
	 * Create Task and Task allocate
	 */
	struct vpul_mbox_host_data	host_lowpr;
	/*! \brief Core (FW) Mail-box for reports to host */
	struct vpul_mbox_core_data	core_create_sizes;
	/*! \brief Core (FW) Mail-box for reports to host */
	struct vpul_mbox_core_data	core_reports;
	/*! \brief Core  Mail-box for system errors */
	struct vpul_mbox_core_data	core_syserr;
	/*! \brief Core  Mail-box for reports of normal host commands */
	struct vpul_mbox_core_data	core_normal;
	/*! \brief Core  Mail-box for reports on low priority host commands */
	struct vpul_mbox_core_data	core_lowpr;
	/*! \brief Core  Mail-box for debug information */
	struct vpul_mbox_core_data	core_debug;
	/*! \brief If set VPU FW will verify for both Host MBOXes that
	 * CommandId is increased by 1 between each couple of commands
	 */
	__u32				verify_inc_cmd_id;
	/*! \brief If set - should update the MBOXes sizes according to Sizes
	 * in MBOXes array. If reset - MBOXes sizes are kept as is.
	 */
	__u32				update_sizes;
	/*! \brief The number of data parameters on each report.must be at
	 * least 1. if set to 0 - previous value is kept as is.
	 */
	__u32				report_data_size;
};

/*! \brief core init memory information */
struct vpul_memory_info {
	/*! \brief Maximal number of tasks that might be created
	 * (and not destroyed)
	 */
	__u32				max_tasks;
	/*! \brief Maximal number of process vertices that might be kept
	 * in the queue
	 */
	__u32				max_proc_vertices;
	/*! \brief Maximal number of control vertices (start, end, dummy,
	 *  host report) that might be kept in the queue
	*/
	__u32				max_control_vertices;
	/*! \brief When memory slot is allocated try to avoid (if possible)
	 * keeping free memory that is less than this value. If  0 - no
	 * limitations on remaining free memory size.
	 */
	__u32				min_alloc_size;
	/*! \brief When memory slot is allocated try to avoid (if possible)
	 * keeping free memory that is bigger than this value.
	 */
	__u32				max_valid_unused_size;
};

/*! \brief core init time information */
struct vpul_time_info {
	/*! \brief The input ISP clock. Core Clock is ISP clock / 2 */
	__u32				isp_clock_mhz;
	/*! \brief
	 * maximal HW chain running time. If set to 0 - 500 msec is used
	 */
	__u32				hw_timeout_msec;
	/*! \brief
	 * maximal Process running time. If set to 0 - 5 seconds is used
	 */
	__u32				process_timeout_msec;
	/*! \brief
	 * maximal Task running time. If set to 0 - 10 seconds is used
	 */
	__u32				task_timeout_msec;
};

/*! \brief core init command parameters */
struct vpul_core_init_command_params {
	/*! \brief core init memory information */
	struct vpul_memory_info		mem;
	/*! \brief core init time information */
	struct vpul_time_info		time;
	/*! \brief core init mail-box information */
	struct vpul_mbox_init_info	mbox;
};

/** @}*/

/********************* read-only data structures *****************************/
/*! \brief this structure holds information internally used by
 * translator function for updating mail-box message parameters.
 * Host shouldn't write to this structure
 */
struct vpul_proc_read_only {
	/*! \brief offsets to process hw parameters in "command parameters" */
	__u32				n_proc_hw_params;
	/*! \brief offsets to process CPU parameters in "command parameters" */
	__u32				n_proc_cpu_para;
	/*! \brief offsets to process loop parameters in "command parameters"*/
	__u32				n_proc_loop_para;
};

struct vpul_alloc_read_only {
	__u32				n_params;
	__u32				n_ext_base_addrs;
	__u32				n_blocks;
	__u32				n_hwmprbs;
	__u32				n_rammprbs;
	__u32				n_internal_rams;
};

struct vpul_task_read_only {
	/*! \brief describe MBOX messages sizes for some of the commands */
	struct vpul_task_mbox_msg_sizes	mbox_msg_sizes;
	__u32				n_invoke_params;
	__u32				n_invoke_extmem_addr;
	struct vpul_alloc_read_only	alloc;
};

struct vpul_loop_totals {
	/*! \brief
	 *  Number of parameters explicitly described in each parameters set
	 */
	__u32				n_exp_per_sets;
	/*! \brief The number of explicitly described parameters sets.
	 * Loop Index % SetsNum choose current set.
	 */
	__u32				n_sets;
	/*! \brief The number of parameters that has fix increment/decrement
	 * for each loop.
	 */
	__u32				fix_inc;
	/*! \brief
	 * Number of parameters replaced according to 1st loop/other loop
	 */
	__u32				per_fst;
};


/********************* sub-chain level data structures ***********************/

/*! \brief types of sub-chain */
enum  vpul_subchain_types {
	/*! \brief HW sub-chain */
	VPUL_SUB_CH_HW			= 0,
	/*! \brief SW Sub Chain */
	VPUL_SUB_CH_CPU_OP		= 1,

	VPUL_SUB_CH_LAST_OP
};

/*! \brief
 * structure used for 3DNN subchain only
 * used for specifying which type of slice group a subchain is intended for
 */
struct slice_group_type_ident {
	/*! \brief
	 * sequential ID (0, 1, ... N)
	 * several subchains may share the same seq_id, meaning that they are used interchangeably
	 * in first, middle or last slice group - according to slice_group_bitmap
	 */
	__u32 seq_id;
	/*! \brief
	 * bitmap specifying 1 or more slice group type (see subch_slice_group_bitmap_vals)
	 * bit 0 : 1st - bit 1 : middle - bit 2 : last - bit 3 : single
	 * more than 1 bit may be set; same bit shouldn't be set in 2 subchains sharing same seq-id
	 * if same subchain is to be used for all types of slice groups, all 4 bits have to be set
	 * examples :
	 * - if seq_id = 0 and slice_group_bitmap = 4 : activated as subchain #0 when
	 *   processing last slice group
	 * - if seq_id = 1 and slice_group_bitmap = 3 : activated as subchain #1 when
	 *   processing either 1st or middle slice group
	 * - if seq_id = 1 and slice_group_bitmap = 9 : activated as subchain #1 when
	 *   processing 1st slice group, as well as in case of single slice group
	 * - if seq_id = 2 and slice_group_bitmap = 0x0F : activated as subchain #2, no
	 *   matter which slice group is being processed
	 */
	__u32 slice_group_bitmap;
};

/*! \brief
* values for slice_group_bitmap in slice_group_type_ident
* combinations of those values can be used, e.g. SUBCH_SLICE_GROUP_FIRST + SUBCH_SLICE_GROUP_MIDDLE
*/
enum subch_slice_group_bitmap_vals {
	SUBCH_SLICE_GROUP_FIRST		= 1,
	SUBCH_SLICE_GROUP_MIDDLE	= 2,
	SUBCH_SLICE_GROUP_LAST		= 4,
	SUBCH_SLICE_GROUP_SINGLE	= 8
};

/** \addtogroup CPUOP CPU predefined operations
 *  \brief Structures used defined CPU operations performed by VPU internal CPU as part of
 *         VPU process
 *
 * \detail
 * \section Add-CPUOP Adding CPU Operation to process:
 *	The process involves few mechanisms:
 * 		1. Read HW results - set the number of blocks of HW read results
 * 		(HW_SUB_CH_READ_RES_BL field in the IntRamReadResBits field in VPUI_HwSubChTotals)
 * 		2. CPU operations - CPU operations are required to perform SW operation.
 *
 *	How to Add CPU Operation:
 *	 1. Set the number of blocks that their HW results should be read in the relevant HW sub-chain
 *	 2. Set the index of the Block to be read in the HW sub chain to the vector.
 *	 1. Add CPU sub-chain after the HW sub-chain
 *	 2. configure CPU Operation from the cpu operation from cpul_cpuop_opcodes.def
 *
 * The structures to add CPU Operation:
 *	1. vpul_cpu_subchain
 *	2. vpul_cpu_op
 *
 * Example:
 *
 * Configuring SW Subchain:
 *  \snippet SubChain.cpp Config SW Chain
 *
 *  @{
 */


/*! \brief The maximal number of CPU operations supported in a CPU SubChain */
#define MAX_SUPPORT_CPU_OPER_NUM		(3)

/*! \struct Describes CPU subchain */
struct vpul_cpu_subchain {
	/*! \brief first Internal ram index to consider*/
	__u32 FstIntRamInd;
	/*! \brief number of internal rams to consider*/
	__u32 IntRamIndices;
	/*! \brief number of internal defined espcially for the CPU operations*/
	__u32 CpuRamIndices;
	struct vpul_cpu_op cpu_op_desc[MAX_SUPPORT_CPU_OPER_NUM];
};

/** @}*/

/*! \brief This structure contains one sub-chain description */
struct vpul_subchain {
	/*! \brief
	 * sub-chain type 0=HW, 1=CPU low priority, 2= CPU high priority
	 */
	enum  vpul_subchain_types	stype;
	/*! \brief SubChain unique ID */


	__u32				id;
	/*! \brief
	 * The index of disable bit in the disabled sub chains mask + 1.
	 * 0 stands for non conditional sub chain. We may disable pre-load
	 * sub-chains or some CPU operations in all process operations or allow
	 * it only on its 1st invocation
	 */
	__u32				disable_bit_p1;
	/*! \brief Number of processing units (blocks) in sub-chain */

	union{
		__u32				num_of_pus;
		/*! \brief Number of CPU ops in sub-chain */
		__u32				num_of_cpu_op;

	};

	union{
		/*! \brief offset to this sub-chain first PU on the vector of PUs */
		__u32				pus_ofs;

	};

	/*! \brief chain info */
	union {
		/*! \brief for 3DNN subchain :
		 * identifier of slice group type this subchain is intended for
		 * (1st / middle / last / single)
		 */
		struct slice_group_type_ident		sl_group_type_ident;
		/*! \brief CPU operation  */
		struct vpul_cpu_subchain		cpu;
	};
};

/********************** process level data structures ************************/
/*! \brief memory types */
enum vpul_memory_types {
	VPUL_MEM_EXTERNAL		= 0, /*!< DRAM memory on HOST */
	VPUL_MEM_INTERNAL		= 1, /*!< VPU MPRB */
	VPUL_MEM_PRELOAD_PU		= 2, /*!< For pre-loading pu (i.e. LUT, HIST)*/
	VPUL_MEM_COEFF_VEC		= 3
};

/********* Memory types used during preload and poststore ********************/
/* Types described below might be accessed by DMA on pre-load + post-stroe */
enum vpul_int_mem_types {
	/* explicit VPUI_InternalRam */
	VPUL_INTMEM_MPRB	= 0,
	/* Using part of Coefficients vector in SRAM (either static or dynamic vector) */
	VPUL_INTMEM_COEFF_VEC	= 1,
	/* Using part of output vector in SRAM */
	VPUL_INTMEM_OUTPUT	= 2,
	/* The Process part in Dynamic ROIs vector in SRAM */
	VPUL_INTMEM_DYN_ROIS	= 3,
	/* The Process part in Dynamic points vector in SRAM */
	VPUL_INTMEM_DYN_POINTS	= 4
};

struct vpul_image_size_desc {
	__u32				width;
	__u32				height;
	__u32				pixel_bytes;
	__u32				line_offset;
};

/*! \brief ROI geometry definitions */
struct	vpul_roi {
	/*! \brief first column offset in the map */
	__u32 first_col;
	/*! \brief first line offset in the map */
	__u32 first_line;
	/*! \brief ROI width or 0 for using map width */
	__u32 width;
	/*! \brief ROI height or 0 for using map height */
	__u32 height;
};

struct vpul_dynamic_map_roi {
	__u32				memmap_idx;
};

struct vpul_fixed_map_roi {
	/*! \brief Index to relevant memory map */
	__u32				memmap_idx;
	/*! \brief If this differs from zero, roi field is used to define part of
	 * memory map as ROI. If not, entire memory map is defined as ROI
	 */
	__u32				use_roi;
	/*! \brief ROI geometry definitions to use */
	struct	vpul_roi		roi;
};

/*! \brief memory descriptor */
struct vpul_memory_map_desc {
	/*! \brief Memory type. 0=External memory (DRAM).
	 * 1=Internal memory (MPRBs).
	 * 2=Using part of coefficients vector in SRAM
	 */
	enum vpul_memory_types		mtype;
	/*! \brief if mtype = external: index in "External memory addresses"
	 * vector of task allocation.
	 * if mtype = internal: index in "Internal RAMs Indexes vector".
	 * if mtype = coefficients: index in "Separate/General Filter
	 * coefficients sets descriptors vector"
	 */
	union{
		__u32			index;
		struct{
			__u32 pu:8;
			__u32 sc:8;
			__u32 proc:8;
			__u32 ports_bit_map:8;
		} pu_index;
	};
	/*! \brief input size parameters */
	struct vpul_image_size_desc	image_sizes;
	 };

/*! \brief input / output types descriptor */
struct vpul_iotypes_desc {
	/*! \brief 0 = ROI is pre-defined. 1 = ROI dynamically sets externally
	 *  by CPU.
	 */
	__u32				is_dynamic;
	/*! \brief roi index (dynamic_map_roi or fixed_map_roi) */
	__u32				roi_index;
	/*! \brief number of input sets each ROI in the input type is used for.
	 * 0 = fixed ROI is used.
	 */
	__u32				n_insets_per_dynamic_roi;

	/*! \brief IO ROI is derived 0 stands for current IO ROI not derived from
	* point
	 */
	__u32				is_roi_derived;
};

enum vpul_sizes_op_type {
	/*! \brief copy current ROI from one of the InOut */
	VPUL_SIZEOP_INOUT = 0,
	/*! \brief use fix sizes */
	VPUL_SIZEOP_FIX = 1,
	/*! \brief use crop operation on the input sizes */
	VPUL_SIZEOP_FORCE_CROP = 2,
	/*! \brief Use crop operation on the input sizes on the edges tiles of the input */
	VPUL_SIZEOP_CROP_ON_EDGES_TILES    = 3,
	/*! \brief use Scale operation on the input sizes */
	VPUL_SIZEOP_SCALE = 4,
	/*! \brief number of supported size operations */
	VPUL_SIZEOP_NUM
};

/*! \brief descriptor of input sizes to blocks on this process */
struct vpul_sizes {
	/*! \brief the type of size operation */
	enum vpul_sizes_op_type		type;
	/*! \brief TODO: explain */
	__u32				op_ind;
	/*! \brief source size index in this sizes vector */
	__u32				src_idx;
};

/*! \brief describe image scale ratio (numerator / denominator) */
struct vpul_ratio {
	/*! \brief numerator */
	__u32				numerator;
	/*! \brief denominator */
	__u32				denominator;
};

/*! \brief scale operation descriptor */
struct vpul_scales {
	/*! \brief horizontal scaling ratio (of the image width) */
	struct vpul_ratio		horizontal;
	/*! \brief vertical scaling ratio (of the image height) */
	struct vpul_ratio		vertical;
};

/*! \brief crop operation descriptor */
struct vpul_croppers {
	/*! \brief crop pixels at the left */
	__u8 Left;
	/*! \brief crop pixels at the right */
	__u8 Right;
	/*! \brief crop pixels at the top */
	__u8 Top;
	/*! \brief crop pixels from bottom */
	__u8 Bottom;
};

/*! \brief struct to describe internal memory */
struct vpul_internal_ram {
	/*!\brief index of first MPRB group to be used by this internal memory*/
	__u32 first_mprb_group_idx:16;
	/*!\brief number of mprb groups to be used by this internal ram */
	__u32 n_mprb_groups:16;
};

struct vpul_roi_desc{
	/*! \brief width ROI point of pt list */
	__u32 roi_width:16;

	/*! \brief height ROI point of pt list */
	__u32 roi_height:16;
};

struct vpul_roi_sizes_desc{
	/*! \brief points roi desc */
	struct	vpul_roi_desc		roi_desc[VPUL_LST_MAX_ROI];
	/*! \brief The number of points that used */
	__u32	n_roi;
};

/*! \brief Memory vector description*/
struct vpul_mem_vec_desc {
	__u32  ext_mem_ind		:8;	/* External memory address vector index */

	/*! \brief Whether Ext memory address is updated on each input sets slot or same address is used */
	__u32  is_ext_updated	:8;

	/*! \brief for internal - Internal Ram index (entire task),
	 * for coeffs-   General Filt er Coeffs index (entire task),
	 * for outputs - the output index (entire task),
	 * Ignored for ROIS + DYN_POINTS
	 */
	__u32  int_mem_type		:8;

	/*! \brief Whether vector size is fixed or dependent by the current input sets number */
	__u32  int_mem_index	:8;

	/*! \brief Whether vector size is fixed or dependent by the current input sets number */
	__u32  is_fixed_size	:8;

	/*! \brief Vector size in bytes.If is_fixed_size is set this is the entire  size for input sets slot-
	 * otherwise it is the size for each input_set_div input sets in the input sets slot.
	 */
	__u32 vector_bytes_per_set_div	:16;
};


/*! \brief Structure to define process inputs
 */
struct vpul_process_inout {

	__u32				n_dynamic_map_rois;
	/*! \brief dynamic map ROIs information TODO: move to dynamic params*/
	struct vpul_dynamic_map_roi	dynamic_map_roi[VPUL_MAX_DYN_MAP_ROI];
	/*! \brief number of fixed map ROIs */
	__u32				n_fixed_map_roi;
	/*! \brief fixed map ROIs information */
	struct vpul_fixed_map_roi	fixed_map_roi[VPUL_MAX_FIX_MAP_ROI];
	/*! \brief number of process's input and output types */
	__u32				n_inout_types;
	/*! \brief input and output types */
	struct vpul_iotypes_desc	inout_types[VPUL_MAX_IN_OUT_TYPES];
	/*! \brief number of size operations descriptors */
	__u32				n_sizes_op;
	/*! \brief descriptor of input sizes to blocks on this process */
	struct vpul_sizes		sizes[VPUL_MAX_SIZES_OP];
	/*! \brief number of scale operation descriptors  */
	__u32				n_scales;
	/*! \brief scale operation descriptors */
	struct vpul_scales		scales[VPUL_MAX_SCALE_DESC];
	/*! \brief number of crop operation descriptors */
	__u32				n_croppers;
	/*! \brief crop operation descriptors */
	struct vpul_croppers		croppers[VPUL_MAX_CROP_DESC];
	/*! \brief number of tiles descriptors for this process */
	/* TODO: currently only 0 tiles supported */
	__u32				n_tiles;
	/*! \brief
	 * process parameter loops - number of parameters of each type
	 */
	struct vpul_loop_totals		param_loop;

	/* TODO implement below DS: */
	__u32				n_insets;

	__u32				n_presets_map_desc;
	struct vpul_mem_vec_desc		pre_sets_mem_vec_desc[VPUL_MAX_MAP_DESC];

	__u32				n_postsets_map_desc;
	struct vpul_mem_vec_desc		post_sets_mem_vec_desc[VPUL_MAX_MAP_DESC];

	/*! \brief
	 * total number of static coefficients for this process. (check only)
	 */
	__u32				n_static_coff;
	/*! \brief
	 * processes vector of static filter coefficients
	 */
	__s32				static_coff[VPUL_MAX_PROC_STATIC_COFF];

	/*! \brief number of sub-chains invoked once after the input sets. */
	__u32				n_subchain_before_insets;


	/*! \brief number of sub-chains invoked before the process chain
	 * on 1st parameters loop of each input. */
	__u32				n_subchain_after_insets;


	/*! \brief Number of invocations per each input tile - each with
	 * different parameters - 0 stands for CPU decision
	 */
	__u32				n_invocations_per_input_tile;
	/*! \brief number of input sets allowed on each iteration. minimum
	 * valid value 1. If set to 0 will be regarded as 1.
	 */
	__u32				max_input_sets_slots;

	/*! \brief points are used they might be used for few maps for
	 * example - N level pyramid. On this case PtsMapsSets = N
	 * 0 stands for no IO is using points
	*/
	__u32				pts_maps_sets;

	/*! \brief The number of maps in each set. For example if 2 pyramids
	 * are used PtsMapsEachSet == 2
	*/
	__u32				pts_maps_each_set;

	__u32               pts_map_indices[VPUL_MAX_PTS_NDICES];

	/*! \brief The number of map indices.*/
	__u32				n_map_indices;

	/*! \brief Number of PtList used */
	__u32				pt_list_num;

	/*! \brief now set always 1*/
	__u32				pt_roi_sizes;

	struct vpul_roi_sizes_desc		roi_sizes_desc;

	/*! \brief memory allocation for vector of points*/
	__u32 n_dyn_points;

};

/*! \brief Process description */
struct vpul_process {
	/*! \brief data in/out information: input sets, IO types, maps, ROIs,
	 * memory
	 */
	struct vpul_process_inout	io;
	/*! \brief this structure is used internally by translator.
	 * Host shouldn't write to this structure!
	 */
	struct vpul_proc_read_only	proc_ro;
};

/*! \brief "bytes per pixel" for data-IN or data-OUT, DMA bytes width for coefficients */
union vpul_inout_bytes_per_pixel_or_dma_bytes_width {
	/*! \brief values 1, 2 or 4 (1 byte per pixel, 2 bytes per pixel, 4 bytes per pixel)
	 * for inout_3dnn of type "data input" or "data output"
	 */
	__u32	bytes_per_pixel;
	/*! \brief for inout_3dnn of type "coefficients";
	 * values : 2 for 16-bit DMA, 4 for 32-bit DMA
	 */
	__u32	dma_bytes_width;
};

enum vpul_3d_sizes_op_type {
	/*! \brief Copy XY sizes of vpul_inout_3dnn type */
	VPUL_3DXY_SIZEOP_INOUT = 0,
	/*! \brief Use XY+Input Z to XY translation */
	VPUL_3DXY_SIZEOP_ZIN_TO_XY = 1,
	/*! \brief Use XY+Output Z to XY translation */
	VPUL_3DXY_SIZEOP_ZOUT_TO_XY = 2,
	/*! \brief use crop operation on the input sizes */
	VPUL_3DXY_SIZEOP_CROP = 3,
	/*! \brief use Scale operation on the input sizes */
	VPUL_3DXY_SIZEOP_SCALE = 4,
	/*! \brief number of supported 3dxy sizeop types */
	VPUL_3DXY_SIZEOP_NUM
};

enum vpul_inout_3dnn_type {
	VPUL_IO_3DNN_INPUT = 0,
	/** value VPUL_IO_3DNN_OUTPUT applies for intermediate slice group results,
	 * on both output and input sides
	 */
	VPUL_IO_3DNN_OUTPUT = 1,
	VPUL_IO_LASTSLGR_OUTPUT = 2,
	VPUL_IO_3DNN_COEFFS = 3,
	VPUL_IO_3DNN_ALL = 4
};

/*! \brief Structure to define 3DNN process inputs and outputs, referenced by DMA block */
struct vpul_inout_3dnn {
	enum vpul_inout_3dnn_type	inout_3dnn_type;
	/*! \brief bytes_per_pixel if inout_3dnn_type = VPUI_IO_3DNN_INPUT or VPUI_IO_3DNN_OUTPUT,
	* dma_bytes_width if inout_3dnn_type = VPUI_IO_3DNN_COEFFS
	*/
	union	vpul_inout_bytes_per_pixel_or_dma_bytes_width bytes_per_pixel_or_dma_bytes_width;
	/*! \brief index in "extern mem addresses" vector of task allocation */
	__u32	mem_descr_index;
};

/*! \brief descriptor of input sizes to blocks on this process */
struct vpul_3dnn_size {
	/*! \brief the type of size operation */
	enum vpul_3d_sizes_op_type	type;
	/*! \brief op_ind :
	 * if type = VPUL_3DXY_SIZEOP_CROP : index of vpul_croppers instance for this operation
	 * if type = VPUL_3DXY_SIZEOP_SCALE : index of vpul_scales instance for this operation
	 * else - don't care - not in use
	 */
	__u32				op_ind;
	/*! \brief
	 * used only if type = VPUL_3DXY_SIZEOP_INOUT
	 * (used for selecting appropriate INOUT size)
	 */
	enum vpul_inout_3dnn_type	inout_3dnn_type;
	/*! \brief
	 * source size index in this sizes vector
	 * not used if type = VPUL_3DXY_SIZEOP_INOUT
	 */
	__u32				src_idx;
};

/*! \brief Structure to define 3DNN process inputs */
struct vpul_3dnn_process_inout {
	/*! \brief
	 * total number of static coefficients for this process. (check only)
	 */
	__u32				n_static_coff;
	/*! \brief
	 * processes vector of static filter coefficients
	 */
	__s32				static_coff[VPUL_MAX_PROC_STATIC_COFF];

	/*! \brief number of size operations for each layer in this process
	 * (same size operations are performed for all layers in process)
	 */
	__u32				n_sizes_op;
	/*! \brief descriptor of input sizes to blocks on this process
	 * (same for all layers in process)
	 */
	struct vpul_3dnn_size		sizes_3dnn[VPUL_MAX_SIZES_OP];
	/*! \brief number of scale operation descriptors for each layer in this process
	 * same number of scale operations for all layers in process, but scaling
	 * parameters for those operations are defined per layer (don't have to be the
	 * same for all layers)
	 */
	__u32				n_scales;
	/*! \brief number of crop operation descriptors for each layer in this process
	 * same number of crop operations for all layers in process, but cropping
	 * parameters for those operations are defined per layer (don't have to be the
	 * same for all layers)
	 */
	__u32				n_croppers;
	/*! \brief largest number of inout descriptors for layer in this process */
	__u32				n_3dnn_inouts;
};

/*! \brief XY size descriptor for 3DNN I/O */
struct vpul_3dnn_io_dim_size {
	/*! \brief x-dim ("width") */
	__u32	x;
	/*! \brief y-dim ("height") */
	__u32	y;
};

/*! \brief cropper descriptor for CNN */
struct vpul_cnn_crop_desc {
	/*! \brief number of left side pixels to crop */
	__u32	left;
	/*! \brief number of right side pixels to crop */
	__u32	right;
	/*! \brief number of upper lines to crop */
	__u32	top;
	/*! \brief number of bottom lines to crop */
	__u32	bottom;
};

/*! \brief RELU parameters descriptor */
struct vpul_relu_pos_neg_desc {
	__u32	sign		:1;
	__u32	exponent	:7;
	__u32	mantissa	:18;
};

/*! \brief RELU descriptor for CNN */
struct vpul_cnn_relu_desc {
	/*! \brief for positive pixel values */
	struct vpul_relu_pos_neg_desc pos;
	/*! \brief for negative pixel values */
	struct vpul_relu_pos_neg_desc neg;
};

/*! \brief types of CNN crop operations */
enum	vpul_cnn_xy_crop {
	/*! \brief no cropping */
	VPU_CNN_XY_CROP_NONE = 0,
	/*! \brief "explicit" cropping, using crop params in vpul_cnn_crop_desc,
	 * and no cropping by filter
	 */
	VPU_CNN_XY_CROP_EXPLICIT = 1,
	/*! \brief cropping by filter, not using crop params in vpul_cnn_crop_desc */
	VPU_CNN_XY_CROP_BY_FILTER = 2
};

/*! \brief parameters describing CNN operations (on per-layer basis) */
struct	vpul_cnn_descr {
	/*! \brief if set : filter size == Input size => single pixel for each output slot */
	__u32		is_dot_mode				:1;
	/*! \brief if set : no convolution is performed */
	__u32		is_pass_thru_mode			:1;
	/*! \brief if set : after each output slice is created with normal filter,
	 * it is summed in post processing => final size is 1x1
	 */
	__u32		is_slice_sum				:1;
	/*! \brief if set : coefficients in memory include 32 bits "filter bias"
	 * before each filter
	 */
	__u32		is_filter_bias_in_coeffs_mem		:1;
	/*! \brief if set, is_filter_bias_in_coeffs_mem must also be set */
	__u32		is_filter_bias_used			:1;
	/*! \brief if set, RELU is enabled, using parameters from vpul_cnn_relu_desc */
	__u32		is_relu_enabled				:1;
	/*! \brief if set, coefficients are location dependent */
	__u32		is_location_dependent_coeffs		:1;
	/*! \brief max = 7 */
	__u32		filter_width				:3;
	/*! \brief max = 7, filter_height and filter_width are generally equal */
	__u32		filter_height				:3;
	/*! \brief number of 16-bit DMAs used for loading the coefficients */
	__u32		number_of_16_bit_dmas_for_coeffs	:3;
	/*! \brief number of 32-bit DMAs used for loading the coefficients */
	__u32		number_of_32_bit_dmas_for_coeffs	:3;
	/*! \brief if set, input data is loaded using 16-bit DMA, else 32-bit DMA
	 * if 16-bit DMA : MUST be VPU_PU_DMAIN0; else, MUST be VPU_PU_DMAIN_WIDE0
	 */
	__u32		is_16_bit_dma_for_data_input		:1;
	/*! \brief if set, accumulated data (from one slice group to next one)
	 * is loaded using 16-bit DMA, else 32-bit DMA
	 * if 16-bit DMA : MUST be VPU_PU_DMAIN0; else, MUST be VPU_PU_DMAIN_WIDE0
	 */
	__u32		is_16_bit_dma_for_accum_data		:1;
	/*! \brief coded as VPUH_CNN_FORMAT_* (values 16F, 32F, 16I, 32I) */
	__u32		input_format				:2;
	/*! \brief coded as VPUH_CNN_FORMAT_* (values 16F, 32F, 16I, 32I) */
	__u32		output_format				:2;
	/*! \brief if set : temporary slice groups results are saved in 16F format
	 * else in 26F on 32 bits
	 */
	__u32		is_temporary_slice_group_results_16f	:1;
	/*! \brief if set : external accumulated data is input to layer
	 * this isn't a commonly use option, normally layer N only gets
	 * output of layer N-1 as input data
	 */
	__u32		is_external_accumulated_data		:1;
	/*! \brief if set : the external accumulated data is in 16F format
	 * else in 26F on 32 bits
	 */
	__u32		is_extern_acc_data_16f			:1;
	/*! \brief type of "crop" operation for this layer (may be "none") */
	enum	vpul_cnn_xy_crop		xy_crop;
	/*! \brief crop parameters */
	struct	vpul_cnn_crop_desc		crop_desc;
	/*! \brief RELU parameters */
	struct	vpul_cnn_relu_desc		relu_desc;
};

/*! \brief 3DNN Layer description */
struct vpul_3dnn_layer {
	/*! \brief vpul_3dnn_io_dim_size instance for data input in layer */
	struct vpul_3dnn_io_dim_size	dim_size_input;
	/*! \brief vpul_3dnn_io_dim_size instance for intermediate data output in layer
	 * (between one slices group and the next one)
	 */
	struct vpul_3dnn_io_dim_size	dim_size_intermediate_output;
	/*! \brief vpul_3dnn_io_dim_size instance for last slices group data output in layer
	 * (also used in case of single slices group)
	 */
	struct vpul_3dnn_io_dim_size	dim_size_last_or_single_slgr_output;
	/*! \brief total number of input slices for this layer */
	__u32				n_total_input_slices;
	/*! \brief if input slices have to be divided into slice groups :
	 * number of input slices in each slice group - except perhaps in
	 * last slice group, which may consist of a smaller number of slices
	 */
	__u32				n_slices_per_input_slice_group;
	/*! \brief number of output slices for this layer */
	__u32				n_output_slices;
	/*! \brief coefficients sizes per slice group (in bytes) for DMA16, including bias values
	 * if present
	 * if last slice group is smaller than other slice groups, this number applies to all
	 * slice groups except the last one
	 * this number must also account for any padding added by user to each block of
	 * coefficients, such as in case of starting each block at a 16-byte or 32-byte boundary
	 */
	 __u32				coeff_size_per_slice_group_dma16;
	 /*! \brief coefficients sizes per slice group (in bytes) for DMA32, including bias values
	 * if present
	 * if last slice group is smaller than other slice groups, this number applies to all
	 * slice groups except the last one
	 * this number must also account for any padding added by user to each block of
	 * coefficients, such as in case of starting each block at a 16-byte or 32-byte boundary
	 */
	 __u32				coeff_size_per_slice_group_dma32;
	 /*! \brief first number of scaling factors */
	__u32				n_scale_factors;
	/*! \brief scaling factors - must fit in 16 bits */
	__u32				scale_factors[VPUL_MAX_3DNN_SCALING_FACTORS];
	/*! \brief CNN parameters descriptor for this layer */
	struct	vpul_cnn_descr		cnn_descr;
	/*! \brief scale operation descriptors - number of descriptors is defined
	 * at process level (must be the same for all layers of a process)
	 */
	struct vpul_scales		scales[VPUL_MAX_SCALE_DESC];
	/*! \brief crop operation descriptors - number of descriptors is defined
	 * at process level (must be the same for all layers of a process)
	 */
	struct vpul_croppers		croppers[VPUL_MAX_CROP_DESC];
	/*! \brief
	 * inout descriptors - number of descriptors is defined at process level
	 * as largest number for layers in this process; some layers may need less
	 * descriptors than this maximum, depending on slices group types
	 * (single / first / middle / last) configured for each layer
	 */
	struct vpul_inout_3dnn		inout_3dnn[VPUL_MAX_3DNN_INOUT];
};

/*! \brief 3DNN Process Base description */
struct vpul_3dnn_process_base {
	/*! \brief 3DNN inout descriptor for this process */
	struct vpul_3dnn_process_inout	io;
	/*! \brief number of 3DNN layers in process base */
	__u32		number_of_layers;
	/*! \brief 3DNN layers included in process base */
	struct vpul_3dnn_layer layers[VPUL_MAX_3DNN_LAYERS_IN_PROC_BASE];
};

/*! \brief 3DNN Process description */
struct vpul_3dnn_process {
	/*! \brief
	 * index of 3DNN Process Base on the vector of 3DNN Process
	 * Bases (instances of vpul_3dnn_process_base) in task
	 */
	__u32		base_3dnn_ind;
	/*! \brief
	 * index of 1st 3DNN layer invoked by this process, from layers
	 * included in vpul_3dnn_process_base specified by base_3dnn_ind
	 */
	__u32		first_3dnn_layer;
	/*! \brief
	 * number of 3DNN layers invoked by this process, must not exceed
	 * number_of_layers of vpul_3dnn_process_base specified by base_3dnn_ind
	 */
	__u32		num_of_3dnn_layers;
	/*! \brief 3DNN process priority */
	__u32		Priority;
};

/************************ task level data structures *************************/

/*! \brief Loops description:
 * Loops might exist in both process and task. Process may include single loop
 * invoked few times for each input set. Task may include many loops.
 * In each of the loop iterations few parameters values are updated.
 * Some of them gets explicit value per loop while some are updated by fix
 * increment / decrement each loop.There are also parameters that are updated
 * according to 1st loop / other loop. For process these parameters are used by
 * the different sub-chains. This structure describe the number of parameters
 * updated along the loop.
 */
/*! \brief task loop types */
enum vpul_loop_type {
	VPU_LOOP_NONE			= 0,
	VPU_LOOP_START		= 1,
	VPU_LOOP_END			= 2
};


/*! \brief This structure defines one task loop description. */
struct vpul_loop {
	/*! \brief Vertex loop type. no Loop, loop ends on this vertex or
	 * loop starts on this vertex
	 */
	enum vpul_loop_type		type;

	/*! \brief
	 * Loop id number
	 */
	__u32				id;

	/*! \brief
	 * Edges that are invoked only on the end of the loop when loop ended
	 * on this vertex.
	 */
	__u32				n_end_loop_edges;

	/*! \brief Number of iterations - relevant only for VPU_LOOP_END vertex*/
	__u32				iterations;

};

/*! \brief Host report description TODO: add parameters */
struct vpul_host_report {
	/*! \brief number of parameters */
	__u32				n_params;
};

/*! \brief This structure defined one edge that connects vertices. */
struct vpul_edge {
	/*! \brief The index of destination vertex */
	__u32				dst_vtx_idx;
};

/*! \brief Types of vertex */
enum vpu_task_vertex_type {
	/*
	 * task it is the Type of the 1st vertex only
	 */
	VPUL_VERTEXT_START		= 0,
	/*! \brief End task vertex - report on "end operation" +
	 * no output edges.For each task it is the Type of the last vertex only
	 */
	VPUL_VERTEXT_END		= 1,
	/*! \brief Process */
	VPUL_VERTEXT_PROC		= 2,
	/*! \brief 3DNN process */
	VPUL_VERTEXT_3DNN_PROC		= 3,
	/*! \brief Report to Host */
	VPUL_VERTEXT_HOST_REP		= 4,
	/*! \brief No operation-reduce number of edges in
	 * "many to many" connections
	 */
	VPUL_VERTEXT_DUMMY		= 5,
	/*! \brief Number of vertex types */
	VPUL_VERTEXT_NUM
};

/*! \brief This structure describes vertex in task graph */
struct vpul_vertex {
	/*! \brief The vertex type */
	enum vpu_task_vertex_type	vtype;
	/*! \brief
	 * Number of output edges  In case of loop_end_idx_p1 != 0 includes the
	 * backward edge (will be always the 1st edge) and all other edges.
	 */
	__u32				n_out_edges;

	/*! \brief
	 * list of output edges information. first, if loop end on this vertex
	 * the first edge it the backward edge that operate the loop.
	 * than n_out_edges - n_end_loop_edges - 1 edges that are always
	 * invoked and than the n_end_loop_edges edges
	 */
	struct vpul_edge		out_edges[VPUL_MAX_INOUT_EDGES];
	/*! \brief Describe loop information operated/ended on this vertex */
	struct vpul_loop		loop;
	/*! \brief
	 * Number of sub-chains (HW/CPU) in process (0 if isn't process)
	 */
	__u32				num_of_subchains;


	/*! \brief
	 * Number of sub-chains that are executed only on first iteration(0 if isn't process)
	 */
	__u32				n_subchain_on_first_iteration;

	/*! \brief
	 * Number of sub-chains that are executed only on last iteration(0 if isn't process)
	 */
	__u32				n_subchain_on_last_iteration;

	/*! \brief
	 * offset to this process first sub-chain on the vector of sub-chains
	 */
	__u32				sc_ofs;
	/*! \brief The vertex data according to vertex type */
	union {
		/*! \brief Process data structure */
		struct vpul_process	proc;
		/*! \brief 3DNN process data structure */
		struct vpul_3dnn_process proc3dnn;
		/*! \brief Host report data structure */
		struct vpul_host_report	hreport;
	};
};


struct vpul_mprb_group {
	/* The id of the 1st MPRB in the group.  */
	__u32	first_mprb_idx:16;
	/* Number of MPRBs (with consecutive Ids) in the group */
	__u32	group_n_mprbs:16;
};

/*! \brief This structure describes full task */
struct vpul_task {
	/*! \brief unique task identifier */
	__u32				id;
	/*! \brief task priority. 1 = Highest. 16= Lowest */
	__u32				priority;
	/*! \brief Total data structure size in bytes */
	__u32				total_size;
	/*! \brief Total number of vertices in task */
	__u32				t_num_of_vertices;
	/*! \brief Total number of 3DNN Process Bases in task */
	__u32				t_num_of_3dnn_process_bases;
	/*! \brief Total number of Sub-Chains */
	__u32				t_num_of_subchains;
	/*! \brief Total number of Processing units */
	__u32				t_num_of_pus;
	/*! \brief Total number of PU's that should be updated on invoke command*/
	__u32				t_num_of_pu_params_on_invoke;


	/*! \brief
	 * Offset to adjacency matrix for vertices in task data structure
	 * (each element is vpul_vertex)
	 */
	__u32				vertices_vec_ofs;
	/*! \brief
	 * Offset in bytes to the vector of 3DNN Process Bases in task data
	 * structure (each element is vpul_3dnn_process_base)
	 */
	__u32				process_bases_3dnn_vec_ofs;
	/*! \brief
	 * Offset in bytes to the vector of sub-chains in task data structure
	 * (each element is vpul_subchain)
	 */
	__u32				sc_vec_ofs;
	/*! \brief
	 * Offset to the vector of processing units in task data structure
	 * (each element is vpul_pu)
	 */
	__u32				pus_vec_ofs;
	/*! \brief
	 * Offset to the vector of locations of PU's that should be update
	 * during invoke, in task data structure.
	 * (each element is vpul_pu_location)
	 */
	__u32				invoke_params_vec_ofs;
	/*! \brief number of task's memory descriptors */
	__u32				n_memmap_desc;
	/*! \brief memory descriptors. each memory described by it's type,index
	 * in it's specific memory list and index of it's image_sizes list
	 * NOTE : if some of the descriptors are intended only for 3DNN processes,
	 * they must be located in array AFTER all other descriptors !
	 */
	struct vpul_memory_map_desc	memmap_desc[VPUL_MAX_MEMORIES_DESC];
	/*! \brief number of external memories addresses used by this task*/
	__u32				n_external_mem_addresses;
	/*! \brief external memories addresses used by this task.
	 * should be set for task allocation!
	 */
	__u32				external_mem_addr[VPUL_MAX_TASK_EXTERNAL_RAMS];

	/*! \brief number of internal RAMs used by this process. */
	__u32				n_internal_rams;
	/*! \brief internal RAMs information for this process. */
	struct vpul_internal_ram internal_rams[VPUL_MAX_TASK_INTERNAL_RAMS];

	/*! \brief number of of mprbs groups defined. */
	__u32				n_mprb_groups;
	/*! \brief array of MPRB groups definitions. */
	struct vpul_mprb_group mprbs_groups[VPUL_MAX_TASK_INTERNAL_RAMS];


	/*! \brief this structure is used internally by translator.
	 * Host shouldn't write to this structure!
	 */
	struct vpul_task_read_only	task_ro;
};

struct vpul_pu * get_pu_by_index(const struct vpul_task * task, __u32 proc_idx, __u32 sc_idx, __u32 pu_idx );



/**
  * \brief Return the size (in bytes) needed to contain Task Data structure
  * \param t_num_of_vertices total number of vertices in task
  * \param t_num_of_subchains total number of sub chains (in all processes)
  * \param t_num_of_pus total number of processing units
  * \param t_num_of_updatable_pus total number of PU's that
  *			are updated during invoke command.
  * \return 0 on success otherwise non-zero
  */
__u32 vpu_translator_get_task_ds_size(
	__u32 t_num_of_vertices,
	__u32 t_num_of_subchains,
	__u32 t_num_of_pus,
	__u32 t_num_of_updatable_pus

	);

/**
* \brief Return the size (in bytes) needed to contain Task Data structure
* (with possibly 3DNN processes)
* \param t_num_of_vertices total number of vertices in task
* \param t_num_of_3dnn_proc_bases total number of 3DNN process bases
* \param t_num_of_subchains total number of sub chains (in all processes)
* \param t_num_of_pus total number of processing units
* \param t_num_of_updatable_pus total number of PU's that are updated during invoke command.
* \return 0 on success otherwise non-zero
*/
__u32 vpu_translator_get_task_ds_size_for_cnn(__u32 t_num_of_vertices,
						__u32 t_num_of_3dnn_proc_bases,
						__u32 t_num_of_subchains,
						__u32 t_num_of_pus,
						__u32 t_num_of_updatable_pus);

/**
  * \brief vpu_translator_create_task_ds_from_array() - Initialize task data structure
  * \param task pointer to memory allocated for task data structure of size
  * received by calling vpu_translator_get_task_ds_size
  * \param size_allocated If 0 indicates method will perform allocation.
  *		If not will be verified to match needed allocation size
  * \param t_num_of_vertices total number of vertices in task
  * \param t_num_of_subchains total number of sub chains (in all processes)
  * \param t_num_of_pus total number of processing units
  * \param t_num_of_updatable_pus total number of PU's that
  *			it's parameters should be update during invoke command
  * \param subchains_for_vertices subchain number for every vertex
  * \param pus_for_subchains pus number for every subchain
  * \return 0 on success otherwise none zero
  */
__s32 vpu_translator_create_task_ds_from_array(
	struct vpul_task *task,
	__u32 size_allocated,
	__u32 t_num_of_vertices,
	__u32 t_num_of_subchains,
	__u32 t_num_of_pus,
	__u32 t_num_of_updatable_pus,
	const __u32 *subchains_for_vertices,
	const __u32 *pus_for_subchains);
/** @}*/

/**
* \brief vpu_translator_create_task_ds_from_array() - Initialize task data structure
*			for task that includes 1 or more 3DNN processes
* \param task pointer to memory allocated for task data structure of size
* received by calling vpu_translator_get_task_ds_size
* \param size_allocated If 0 indicates method will perform allocation.
*		If not will be verified to match needed allocation size
* \param t_num_of_vertices total number of vertices in task
* \param t_num_of_3dnn_proc_bases total number of 3DNN process bases in task
* \param t_num_of_subchains total number of sub chains (in all processes)
* \param t_num_of_pus total number of processing units
* \param t_num_of_updatable_pus total number of PU's that
*			it's parameters should be update during invoke command
* \param subchains_for_vertices subchain number for every vertex
* \param pus_for_subchains pus number for every subchain
* \return 0 on success otherwise none zero
*/
__s32 vpu_translator_create_task_ds_from_array_for_cnn(struct vpul_task *task,
						__u32 size_allocated,
						__u32 t_num_of_vertices,
						__u32 t_num_of_3dnn_proc_bases,
						__u32 t_num_of_subchains,
						__u32 t_num_of_pus,
						__u32 t_num_of_updatable_pus,
						const __u32 *subchains_for_vertices,
						const __u32 *pus_for_subchains);

#endif
