/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/bitmap.h>

#include "lib/vpul-def.h"
#include "lib/vpul-errno.h"
#include "lib/vpul-gen.h"
#include "lib/vpul-ds.h"
#include "lib/vpul-hwmapper.h"
#include "vpul-latency-balancing.h"
#include "lib/vpul-translator.h"
#include "lib/vpul-pu.h"
#include "vpu-hardware.h"
#include "lib/vpul-hw-v2.1.h"
#include "lib/vpu-fwif-hw-gen.h"

struct port_index_2_pu {
	struct	vpul_pu	*pu;
	__u32	port_index_in_pu;
};

/**
 * The following structure is used for returning the results of calculation
 * of number of MPRBs needed.
 * in addition to number of large MPRBs and small MPRBs, it also returns an
 * array describing the PU RAM ports to use for MPRB connections; this is
 * useful for those exception cases to the rule of using consecutive ports,
 * as for fast disparity, upscaler and inpaint.
 * This structure is initialized by calling function to default values :
 * - num_lrg_mprbs and num_sm_mprbs both initialized to 0
 * - ram_ports_array = consecutive_mem_ports[]
 * the functions calculating number of MPRBs needed and RAM ports to use need
 * to write in struct result_of_calc_num_mprbs_needed only the data which is
 * different from default values; for example, if no small MPRBs are needed, no
 * need to write 0 to num_sm_mprbs.
 */
struct result_of_calc_num_mprbs_needed {
	__u32	num_lrg_mprbs;
	__u32	num_sm_mprbs;
	const __u32 *ram_ports_array;
};

typedef void (*__calc_nbr_of_mprbs_needed)(const struct vpul_pu *pu,
	__u32 width,
	struct result_of_calc_num_mprbs_needed *num_mprbs_needed_calc_result);

#include "lib/mem_inter_connect.def"

static const struct pu_ram_port_range pu_inst_2_ram_port[VPU_PU_NUMBER] = {
#define VPU_PU_INSTANCE(a, b, c, d, e, f, g, h, i) {f, g},
#include "lib/vpul_pu_instances.def"
#undef VPU_PU_INSTANCE
};

const enum VPU_PU_TYPES pu_inst2type[VPU_PU_NUMBER + 1] = {
#define VPU_PU_INSTANCE(a, b, c, d, e, f, g, h,i) b,
#include "lib/vpul_pu_instances.def"
	/* indicates "end of array" */
	END_OF_PU_INST2TYPE_TRANSLATOR
#undef VPU_PU_INSTANCE
};

/**
 * the following arrays specify patterns of PU RAM ports to be used for
 * MPRB connections
 */
const __u32 upscaler_2_mem_ports[] = {0, 2};

const __u32 inpaint_4_mem_ports[] = {0, 2, 4, 6};

const __u32 fast_disp__mem_ports_width_511_or_less[] = {0, 1, 2, 3, 4, 5, 6,
							7, 8, 9, 10, 11, 12,
							20, 22, 24, 26, 28,
							30, 32, 34, 37, 40};

const __u32 fast_disp__mem_ports_width_512_to_1023[] = {0, 1, 2, 3, 4, 5, 6,
							7, 8, 9, 10, 11, 12,
							20, 21, 22, 23, 24,
							25, 26, 27, 28, 29,
							30, 31, 32, 33, 34,
							35, 37, 38, 40, 41};

const __u32 fast_disp__mem_ports_width_1024_or_more[] = {0, 1, 2, 3, 4, 5, 6,
							7, 8, 9, 10, 11, 12,
							13, 14, 15, 16, 17, 18,
							19, 34, 35, 36, 37, 38,
							39, 40, 41, 42};

const __u32 integr_img_mem_ports_width_up_to_1024[] = {0, 4, 5, 6, 7, 8, 9, 10,
					11, 12, 13, 14, 15, 16, 17, 18, 19};

const __u32 integr_img_mem_ports_width_1025_to_2048[] = {0, 1, 4, 5, 6, 7, 8,
							9, 10, 11, 12, 13, 14,
							15, 16, 17, 18, 19};

const __u32 consecutive_mem_ports[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
					10, 11, 12, 13, 14, 15, 16, 17,
					18, 19, 20, 21, 22, 23, 24, 25,
					26, 27, 28, 29, 30, 31, 32, 33,
					34, 35, 36, 37, 38, 39, 40, 41,
					42, 43, 44, 45, 46};

/**
 * **************************************************************
 * following declarations are for filters (linear and non linear)
 * **************************************************************
 */
#define NUM_OF_WIDTH_RANGES_FOR_FILTERS_MEM_REQUIR	5
#define NUM_OF_FILTER_SIZES_FOR_FILTERS_MEM_REQUIR	6

struct mprb_requirements {
	__u32 mprb_num_lrg;
	__u32 mprb_num_sm;
};

struct mprb_req_per_pixel_size {
	struct mprb_requirements mprb_req_for_8bit_pixel;
	struct mprb_requirements mprb_req_for_16bit_pixel;
	struct mprb_requirements mprb_req_for_32bit_pixel;
};

const __u32 width_sizes_2_index[NUM_OF_WIDTH_RANGES_FOR_FILTERS_MEM_REQUIR] = {
						257, 513, 1025, 2049, 4097};
/**
 * the following LUT provides the number of MPRBs used for filters, according
 * to width and filter size
 * The contents of each entry are pairs of values :
 * - 1st value = number of large MPRBs needed
 * - 2nd value = number of small MPRBs needed
 * None of the pairs has its 2 values both different from 0
 */
const struct mprb_req_per_pixel_size mprb_req_per_pixel_size_values
			[NUM_OF_WIDTH_RANGES_FOR_FILTERS_MEM_REQUIR]
			[NUM_OF_FILTER_SIZES_FOR_FILTERS_MEM_REQUIR] = {

			/* 1 byte/pixel  -  2 bytes/pixel  -  4 bytes/pixel */

			/* width 1 to 256, filter size 1x1 */
			{
				{{0, 0},	{0, 0},		{0, 0} },
			/* width 1 to 64, filter size 3x3 */
				{{0, 1},	{0, 1},		{0, 2} },
			/* width 1 to 64, filter size 5x5 */
				{{0, 1},	{0, 2},		{0, 4} },
			/* width 1 to 64, filter size 7x7 */
				{{0, 4},	{0, 4},		{0, 6} },
			/* width 1 to 64, filter size 9x9 */
				{{0, 2},	{0, 4},		{0, 0} },
			/* width 1 to 64, filter size 11x11 */
				{{0, 4},	{0, 6},		{0, 0} },
			}, {
			/* width 257 to 512, filter size 1x1 */
				{{0, 0},	{0, 0},		{0, 0} },
			/* width 257 to 512, filter size 3x3 */
				{{0, 1},	{0, 2},		{2, 0} },
			/* width 257 to 512, filter size 5x5 */
				{{0, 2},	{0, 4},		{4, 0} },
			/* width 257 to 512, filter size 7x7 */
				{{0, 4},	{0, 4},		{6, 0} },
			/* width 257 to 512, filter size 9x9 */
				{{0, 4},	{4, 0},		{0, 0} },
			/* width 257 to 512, filter size 11x11 */
				{{4, 0},	{0, 6},		{0, 0} },
			}, {
			/* width 513 to 1024, filter size 1x1 */
				{{0, 0},	{0, 0},		{0, 0} },
			/* width 513 to 1024, filter size 3x3 */
				{{0, 2},	{1, 0},		{2, 0} },
			/* width 513 to 1024, filter size 5x5 */
				{{1, 0},	{2, 0},		{4, 0} },
			/* width 513 to 1024, filter size 7x7 */
				{{4, 0},	{4, 0},		{6, 0} },
			/* width 513 to 1024, filter size 9x9 */
				{{2, 0},	{4, 0},		{0, 0} },
			/* width 513 to 1024, filter size 11x11 */
				{{4, 0},	{6, 0},		{0, 0} },
			}, {
			/* width 1025 to 2048, filter size 1x1 */
				{{0, 0},	{0, 0},		{0, 0} },
			/* width 1025 to 2048, filter size 3x3 */
				{{1, 0},	{2, 0},		{4, 0} },
			/* width 1025 to 2048, filter size 5x5 */
				{{2, 0},	{4, 0},		{8, 0} },
			/* width 1025 to 2048, filter size 7x7 */
				{{4, 0},	{8, 0},		{12, 0} },
			/* width 1025 to 2048, filter size 9x9 */
				{{4, 0},	{8, 0},		{0, 0} },
			/* width 1025 to 2048, filter size 11x11 */
				{{8, 0},	{12, 0},	{0, 0} },
			}, {
			/* width 2049 to 4096, filter size 1x1 */
				{{0, 0},	{0, 0},		{0, 0} },
			/* width 2049 to 4096, filter size 3x3 */
				{{2, 0},	{4, 0},		{8, 0} },
			/* width 2049 to 4096, filter size 5x5 */
				{{4, 0},	{8, 0},		{16, 0} },
			/* width 2049 to 4096, filter size 7x7 */
				{{8, 0},	{12, 0},	{24, 0} },
			/* width 2049 to 4096, filter size 9x9 */
				{{8, 0},	{16, 0},	{0, 0} },
			/* width 2049 to 4096, filter size 11x11 */
				{{16, 0},	{24, 0},	{0, 0} }
			}
};

/**
 * ***********************************************
 * following declarations are for non-filter PUs
 * ***********************************************
 */
struct mprb_needed_per_img_width {
				__u32 width_upper_limit;
				__u32 nbr_lrg_mprbs;
				__u32 nbr_sm_mprbs;
				const __u32 *ram_ports_array;
};

const struct mprb_needed_per_img_width mprb_per_img_width_upsc[] = {
				{1025,       2, 0, upscaler_2_mem_ports},
				{0xFFFFFFFF, 4, 0, consecutive_mem_ports}
};

const struct mprb_needed_per_img_width mprb_per_img_width_downsc[] = {
				{1025,       1, 0, consecutive_mem_ports},
				{2049,       2, 0, consecutive_mem_ports},
				{0xFFFFFFFF, 4, 0, consecutive_mem_ports}
};

const struct mprb_needed_per_img_width mprb_per_img_width_integ_II_or_CC[] = {
		{1025,       1, 0, integr_img_mem_ports_width_up_to_1024},
		{2049,       2, 0, integr_img_mem_ports_width_1025_to_2048},
		{0xFFFFFFFF, 4, 0, consecutive_mem_ports}
};

const struct mprb_needed_per_img_width mprb_per_img_width_inpnt[] = {
				{621,        4, 0, inpaint_4_mem_ports},
				{0xFFFFFFFF, 8, 0, consecutive_mem_ports}
};

const struct mprb_needed_per_img_width mprb_per_img_width_fdepth[] = {
		{512,        13, 10, fast_disp__mem_ports_width_511_or_less},
		{1024,       13, 20, fast_disp__mem_ports_width_512_to_1023},
		{0xFFFFFFFF, 20, 9,  fast_disp__mem_ports_width_1024_or_more}
};

static __s32 set_size_for_dma(
		const struct vpul_task *task,
		const struct vpul_vertex *vertex,
		const struct vpul_pu *pu,
		__u32 *actual_sizes_array)
{
	__u32 inout_type;
	__u32 memmap_index;
	__u32 roi_idx;
	const union vpul_pu_parameters *pu_params;
	const struct vpul_pu_dma *pu_dma_in_params;
	const struct vpul_image_size_desc *size_desc;
	const struct vpul_process *process;
	__u32 in_size_index;

	in_size_index = pu->in_size_idx;
	pu_params = &pu->params;
	pu_dma_in_params = &pu_params->dma;
	inout_type = pu_dma_in_params->inout_index;
	process = &vertex->proc;
	roi_idx = process->io.inout_types[inout_type].roi_index;
	memmap_index = process->io.fixed_map_roi[roi_idx].memmap_idx;
	size_desc =
		&task->memmap_desc[memmap_index].image_sizes;
	if (process->io.sizes[in_size_index].type != VPUL_SIZEOP_INOUT){
		if (is_pu_dma_in(pu))
			return -1; /* err  */
	    else
			return 0; /* dma out case, just do not update */
	}
	actual_sizes_array[in_size_index] = size_desc->width;
	return 0;
}

static __s32 set_sizes_for_all_dma_pus(const struct vpul_task *task,
				const struct vpul_vertex *vertex,
				__u32 *actual_sizes_array)
{
	__u32 num_of_pus;
	const struct vpul_pu *curr_pu;
	__u32 i, j, subchain_count;
	const struct vpul_subchain *subchain;

	subchain_count = vertex->num_of_subchains;
	subchain = fst_vtx_sc_ptr(task, vertex);

	for (i = 0; i < subchain_count; i++, subchain++) {
		num_of_pus = subchain->num_of_pus;
		curr_pu = fst_sc_pu_ptr(task, subchain);

		if ((num_of_pus != 0) && (curr_pu != NULL)) {
			for (j = 0; j < num_of_pus; j++, curr_pu++) {
				if (is_pu_dma_in(curr_pu) || is_pu_dma_out(curr_pu))  // condition should check dma in OR out is_pu_dma_in(curr_pu) || is_pu_dma_out(curr_pu)
					if (set_size_for_dma(
							task,
							vertex,
							curr_pu,
							actual_sizes_array))
							return -1;
			}
		}
	}
	return 0;
}

static __s32 set_3dnn_data_inout_sizes(const struct vpul_task *task,
					const struct vpul_3dnn_process_base *proc3dnn_base,
					__u32 *actual_sizes_array)
{
	__u32 i;
	__u32 num_layers = proc3dnn_base->number_of_layers;
	const struct vpul_3dnn_size *size_op;
	__u32 largest_width = 0;
	__u32 num_operations = proc3dnn_base->io.n_sizes_op;

	for (i = 0; i < num_layers; i++) {
		if (proc3dnn_base->layers[i].dim_size_input.x > largest_width)
			largest_width = proc3dnn_base->layers[i].dim_size_input.x;
	}

	for (i = 0; i < num_operations; i++) {
		size_op = &proc3dnn_base->io.sizes_3dnn[i];
		if ((size_op->type == VPUL_3DXY_SIZEOP_INOUT) &&
			(size_op->inout_3dnn_type == VPUL_IO_3DNN_INPUT))
			actual_sizes_array[i] = largest_width;
	}
	return 0;
}

static __u32 set_size_crop(const struct vpul_sizes *size_op,
				const struct vpul_process *process,
				__u32 *actual_sizes_array,
				__u32 index)
{
	__u32 previous_size;
	const struct vpul_croppers *crop_params;

	previous_size = actual_sizes_array[size_op->src_idx];
	if (previous_size == 0xFFFFFFFF)
		return 0;
	crop_params = &process->io.croppers[size_op->op_ind];
	actual_sizes_array[index] = previous_size - crop_params->Left - crop_params->Right;
	return 1;
}

static __u32 set_size_scale(const struct vpul_sizes *size_op,
				const struct vpul_process *process,
				__u32 *actual_sizes_array,
				__u32 index)
{
	__u32 previous_size;
	const struct vpul_scales *scale_params;
	__u32 temp_val;

	previous_size = actual_sizes_array[size_op->src_idx];
	if (previous_size == 0xFFFFFFFF)
		return 0;
	scale_params = &process->io.scales[size_op->op_ind];
	temp_val = previous_size * scale_params->horizontal.numerator;
	temp_val += scale_params->horizontal.denominator - 1;
	actual_sizes_array[index] = temp_val / scale_params->horizontal.denominator;
	return 1;
}

static __s32 set_actual_sizes(const struct vpul_task *task,
				const struct vpul_vertex *vertex,
				__u32 *actual_sizes_array)
{
	__u32 i;
	const struct vpul_process *process = &vertex->proc;
	__u32 num_operations = process->io.n_sizes_op;
	const struct vpul_sizes *size_op;
	__u32 sizes_were_calculated_flag;
	__u32 sizes_left_to_calc_flag;

	/* initialize all entries in array to "not calculated yet" */
	for (i = 0; i < num_operations; i++)
		actual_sizes_array[i] = 0xFFFFFFFF;

	/* set actual sizes for all DMA-in PUs (should be of type "inout") */
	if (set_sizes_for_all_dma_pus(task, vertex, actual_sizes_array))
		return -1;

	sizes_left_to_calc_flag = 1;

	while (sizes_left_to_calc_flag) {
		sizes_were_calculated_flag = 0;
		sizes_left_to_calc_flag = 0;
		for (i = 0; i < num_operations; i++) {
			if (actual_sizes_array[i] == 0xFFFFFFFF) {
				size_op = &process->io.sizes[i];
				switch (size_op->type) {
				case VPUL_SIZEOP_INOUT:
					return -1;
				case VPUL_SIZEOP_FIX:
					/* TBD */
					break;
				case VPUL_SIZEOP_FORCE_CROP:
				case VPUL_SIZEOP_CROP_ON_EDGES_TILES:
					if (set_size_crop(size_op, process,
							actual_sizes_array, i))
						sizes_were_calculated_flag = 1;
					else
						sizes_left_to_calc_flag = 1;
					break;
				case VPUL_SIZEOP_SCALE:
					if (set_size_scale(size_op, process,
							actual_sizes_array, i))
						sizes_were_calculated_flag = 1;
					else
						sizes_left_to_calc_flag = 1;
					break;
				default:
					return -1;
				}
			}
		}
		if ((sizes_left_to_calc_flag) && (!sizes_were_calculated_flag))
			return -1;
	}
	return 0;
}

static __u32 set_size_3dnn_crop(const struct vpul_3dnn_size *size_op_3dnn,
				const struct vpul_3dnn_process_base *proc_3dnn_base,
				__u32 *actual_sizes_array,
				__u32 index)
{
	const struct vpul_croppers *crop_params;
	__u32 previous_size;
	__u32 i;
	__u32 crop_val;
	__u32 smallest_crop_val;

	previous_size = actual_sizes_array[size_op_3dnn->src_idx];
	if (previous_size == 0xFFFFFFFF)
		return 0;

	/* find smallest crop size --> largest remaining size
	 * ("worst case" for MPRBs allocation)
	 */
	crop_params = &proc_3dnn_base->layers[0].croppers[size_op_3dnn->op_ind];
	smallest_crop_val = crop_params->Left + crop_params->Right;

	for (i = 0; i < proc_3dnn_base->number_of_layers; i++) {
		crop_params = &proc_3dnn_base->layers[i].croppers[size_op_3dnn->op_ind];
		crop_val = crop_params->Left + crop_params->Right;
		if (crop_val < smallest_crop_val)
			smallest_crop_val = crop_val;
	}

	actual_sizes_array[index] = previous_size - smallest_crop_val;
	return 1;
}

static __u32 set_size_3dnn_scale(const struct vpul_3dnn_size *size_op_3dnn,
				const struct vpul_3dnn_process_base *proc_3dnn_base,
				__u32 *actual_sizes_array,
				__u32 index)
{
	__u32 previous_size, temp_val, i;
	const struct vpul_ratio *scale_val;
	const struct vpul_ratio *largest_scale_val;

	previous_size = actual_sizes_array[size_op_3dnn->src_idx];
	if (previous_size == 0xFFFFFFFF)
		return 0;
	/* find largest scale size --> "worst case" for MPRBs allocation */
	largest_scale_val = &proc_3dnn_base->layers[0].scales[size_op_3dnn->op_ind].horizontal;

	for (i = 0; i < proc_3dnn_base->number_of_layers; i++) {
		scale_val = &proc_3dnn_base->layers[i].scales[size_op_3dnn->op_ind].horizontal;
		if ((scale_val->numerator * largest_scale_val->denominator) >
			(scale_val->denominator * largest_scale_val->numerator)) {
			largest_scale_val = scale_val;
		}
	}

	temp_val = previous_size * largest_scale_val->numerator;
	temp_val += largest_scale_val->denominator - 1;
	actual_sizes_array[index] = temp_val / largest_scale_val->denominator;
	return 1;
}

static __s32 set_actual_sizes_3dnn(const struct vpul_task *task,
				const struct vpul_3dnn_process_base *proc_3dnn_base,
				__u32 *actual_sizes_array)
{
	__u32 i;
	const struct vpul_3dnn_size *size_op_3dnn;
	__u32 sizes_were_calculated_flag;
	__u32 sizes_left_to_calc_flag;
	__u32 num_operations = proc_3dnn_base->io.n_sizes_op;

	/* initialize all entries in array to "not calculated yet" */
	for (i = 0; i < num_operations; i++)
		actual_sizes_array[i] = 0xFFFFFFFF;

	/* set actual sizes for all DMA-in PUs for data input (should be of type "inout") */
	if (set_3dnn_data_inout_sizes(task, proc_3dnn_base, actual_sizes_array))
		return -1;

	sizes_left_to_calc_flag = 1;

	while (sizes_left_to_calc_flag) {
		sizes_were_calculated_flag = 0;
		sizes_left_to_calc_flag = 0;
		for (i = 0; i < num_operations; i++) {
			if (actual_sizes_array[i] == 0xFFFFFFFF) {
				size_op_3dnn = &proc_3dnn_base->io.sizes_3dnn[i];
				if (size_op_3dnn->type == VPUL_3DXY_SIZEOP_CROP) {
					if (set_size_3dnn_crop(size_op_3dnn,
								proc_3dnn_base,
								actual_sizes_array,
								i))
						sizes_were_calculated_flag = 1;
					else
						sizes_left_to_calc_flag = 1;
				} else if (size_op_3dnn->type == VPUL_3DXY_SIZEOP_SCALE) {
					if (set_size_3dnn_scale(size_op_3dnn,
								proc_3dnn_base,
								actual_sizes_array,
								i))
						sizes_were_calculated_flag = 1;
					else
						sizes_left_to_calc_flag = 1;
				}
			}
		}
		if ((sizes_left_to_calc_flag) && (!sizes_were_calculated_flag))
			return -1;
	}
	return 0;
}

static __u32 get_width_index_for_filter_mem_req(__u32 width)
{
	__u32 i;
	__u32 retval = 0xFF;

	for (i = 0; i < sizeof(width_sizes_2_index) / sizeof(__u32); i++) {
		if (width < width_sizes_2_index[i]) {
			retval = i;
			break;
		}
	}
	return retval;
}

/**
 * this function returns index in range 0 to 5, according to
 * VPUH_FILTER_SIZE values - or 0xFF in case of invalid input
 */
__u32 get_filter_size_index_for_linear_filter(__u32 filter_size)
{
	__u32 retval = 0xFF;

	switch (filter_size) {
	case VPUH_FILTER_SIZE_1x1:
		retval = 0;
		break;
	case VPUH_FILTER_SIZE_3x3:
		retval = 1;
		break;
	case VPUH_FILTER_SIZE_5x5:
		retval = 2;
		break;
	case VPUH_FILTER_SIZE_7x7:
		retval = 3;
		break;
	case VPUH_FILTER_SIZE_9x9:
		retval = 4;
		break;
	case VPUH_FILTER_SIZE_11x11:
		retval = 5;
		break;
	default:
		break;
	}
	BUG_ON(retval == 0xFF);
	return retval;
}

/**
 * this function returns index = 1, 2 or 3, according to
 * VPUH_NLF_MODE values - or 0xFF in case of invalid input
 */
__u32 get_filter_size_idx_for_non_linear_filter(__u32 nlf_mode)
{
	__u32 retval = 0xFF;

	switch (nlf_mode) {
	case VPUH_NLF_MODE_MIN:
	case VPUH_NLF_MODE_MAX:
	case VPUH_NLF_MODE_MED:
		retval = 1;
		break;
	case VPUH_NLF_MODE_FAST:
		retval = 3;
		break;
	case VPUH_NLF_MODE_CENSUS:
	case VPUH_NLF_MODE_SM_SMOOTH:
		retval = 2;
		break;
	default:
		break;
	}
	return retval;
}

void calc_num_mprbs_for_filters(__u32 img_width, __u32 pixel_bytes,
	__u32 filter_size_index,
	struct result_of_calc_num_mprbs_needed *num_mprbs_needed_calc_result)
{
	__u32 width_idx;
	const struct mprb_req_per_pixel_size *mprb_req_per_pix_size;
	const struct mprb_requirements *mprb_req;

	width_idx = get_width_index_for_filter_mem_req(img_width);
	mprb_req_per_pix_size =
		&mprb_req_per_pixel_size_values[width_idx][filter_size_index];

	if (pixel_bytes == VPUH_BITS_8_BITS)
		mprb_req = &mprb_req_per_pix_size->mprb_req_for_8bit_pixel;
	else if (pixel_bytes == VPUH_BITS_16_BITS)
		mprb_req = &mprb_req_per_pix_size->mprb_req_for_16bit_pixel;
	else
		mprb_req = &mprb_req_per_pix_size->mprb_req_for_32bit_pixel;

	num_mprbs_needed_calc_result->num_lrg_mprbs = mprb_req->mprb_num_lrg;
	num_mprbs_needed_calc_result->num_sm_mprbs = mprb_req->mprb_num_sm;
}

void get_num_mprbs_acc_to_width_for_non_filters(
	__u32 width,
	const struct mprb_needed_per_img_width *mprb_per_image_width,
	__u32 nbr_entries,
	struct result_of_calc_num_mprbs_needed *num_mprbs_needed_calc_result)
{
	__u32 i;

	for (i = 0; i < nbr_entries; i++) {
		if (width < mprb_per_image_width[i].width_upper_limit)
			break;
	}
	num_mprbs_needed_calc_result->num_lrg_mprbs =
				mprb_per_image_width[i].nbr_lrg_mprbs;
	num_mprbs_needed_calc_result->num_sm_mprbs =
				mprb_per_image_width[i].nbr_sm_mprbs;
	num_mprbs_needed_calc_result->ram_ports_array =
				mprb_per_image_width[i].ram_ports_array;
}

void vpu_resource_no_mprbs_needed(const struct vpul_pu *pu,
	__u32 img_width,
	struct result_of_calc_num_mprbs_needed *num_mprbs_needed_calc_result)
{
}

void vpu_resource_calc_num_mprbs_for_nms(const struct vpul_pu *pu,
	__u32 img_width,
	struct result_of_calc_num_mprbs_needed *num_mprbs_needed_calc_result)
{
	const union vpul_pu_parameters *pu_params = &pu->params;
	const struct vpul_pu_nms *nms = &(pu_params->nms);
	__u32 filter_size_index;

	filter_size_index =
		get_filter_size_index_for_linear_filter(nms->support);

	calc_num_mprbs_for_filters(img_width, nms->bits_in, filter_size_index,
					num_mprbs_needed_calc_result);
}

void vpu_resource_calc_num_mprbs_for_slf(const struct vpul_pu *pu,
	__u32 img_width,
	struct result_of_calc_num_mprbs_needed *num_mprbs_needed_calc_result)
{
	const union vpul_pu_parameters *pu_params = &pu->params;
	const struct vpul_pu_slf *slf = &(pu_params->slf);
	__u32 filter_size_index = 0;
	__u32 ram_size_for_1_line = 0;
	__u32 num_of_line_buffers = 0;

	if (slf->invert_columns) {
		ram_size_for_1_line = img_width;
		if (slf->bits_in == VPUH_BITS_16_BITS)
			/* double line size if 16 bits per pixel */
			ram_size_for_1_line <<= 1;
		/* divide by 1024 to find number of small MPRBs needed */
		num_of_line_buffers = (ram_size_for_1_line + 1023) >> 10;
		if (num_of_line_buffers < 4)
			num_mprbs_needed_calc_result->num_sm_mprbs = num_of_line_buffers;
		else
			/* divide by 4096 to find number of large MPRBs needed */
			num_mprbs_needed_calc_result->num_lrg_mprbs =
						(ram_size_for_1_line + 4095) >> 12;
	} else {
		filter_size_index =
				get_filter_size_index_for_linear_filter(slf->filter_size_mode);

		calc_num_mprbs_for_filters(img_width, slf->bits_in, filter_size_index,
					num_mprbs_needed_calc_result);
	}
}

void vpu_resource_calc_num_mprbs_for_glf5(const struct vpul_pu *pu,
	__u32 img_width,
	struct result_of_calc_num_mprbs_needed *num_mprbs_needed_calc_result)
{
	const union vpul_pu_parameters *pu_params = &pu->params;
	const struct vpul_pu_glf *glf = &(pu_params->glf);
	__u32 filter_size_index;

	filter_size_index =
		get_filter_size_index_for_linear_filter(glf->filter_size_mode);

	calc_num_mprbs_for_filters(img_width, glf->bits_in, filter_size_index,
					num_mprbs_needed_calc_result);
}

void vpu_resource_calc_num_mprbs_for_nlf(const struct vpul_pu *pu,
	__u32 img_width,
	struct result_of_calc_num_mprbs_needed *num_mprbs_needed_calc_result)
{
	const union vpul_pu_parameters *pu_params = &pu->params;
	const struct vpul_pu_nlf *nlf = &(pu_params->nlf);
	__u32 filter_size_index;

	filter_size_index = get_filter_size_idx_for_non_linear_filter(
					nlf->filter_mode);

	calc_num_mprbs_for_filters(img_width, nlf->bits_in, filter_size_index,
					num_mprbs_needed_calc_result);
}

void vpu_resource_calc_num_mprbs_for_upsc(const struct vpul_pu *pu,
	__u32 img_width,
	struct result_of_calc_num_mprbs_needed *num_mprbs_needed_calc_result)
{
	get_num_mprbs_acc_to_width_for_non_filters(img_width,
						mprb_per_img_width_upsc, 2,
						num_mprbs_needed_calc_result);
}

void vpu_resource_calc_num_mprbs_for_downsc(const struct vpul_pu *pu,
	__u32 img_width,
	struct result_of_calc_num_mprbs_needed *num_mprbs_needed_calc_result)
{
	get_num_mprbs_acc_to_width_for_non_filters(img_width,
						mprb_per_img_width_downsc, 3,
						num_mprbs_needed_calc_result);
}

void vpu_resource_calc_num_mprbs_for_intimg(const struct vpul_pu *pu,
	__u32 img_width,
	struct result_of_calc_num_mprbs_needed *num_mprbs_needed_calc_result)
{
	const union vpul_pu_parameters *pu_params = &pu->params;
	const struct vpul_pu_integral *intg_pr = &pu_params->integral;

	switch (intg_pr->integral_image_mode) {
	case VPUH_INTIMG_MODE_LUT:
		/* lut size / 2048 */
		num_mprbs_needed_calc_result->num_lrg_mprbs =
			(intg_pr->lut_number_of_values + 2047) >> 11;
		break;
	case VPUH_INTIMG_MODE_CONNECTED_COMP:
		get_num_mprbs_acc_to_width_for_non_filters(img_width,
					mprb_per_img_width_integ_II_or_CC, 3,
					num_mprbs_needed_calc_result);
		num_mprbs_needed_calc_result->num_lrg_mprbs +=
			    ((intg_pr->cc_label_vector_size + 2047) >> 11);
		break;
	default:
		/* II mode */
		get_num_mprbs_acc_to_width_for_non_filters(img_width,
					mprb_per_img_width_integ_II_or_CC, 3,
					num_mprbs_needed_calc_result);
		break;
	}
}

void vpu_resource_calc_num_mprbs_for_histog(const struct vpul_pu *pu,
	__u32 img_width,
	struct result_of_calc_num_mprbs_needed *num_mprbs_needed_calc_result)
{
	num_mprbs_needed_calc_result->num_lrg_mprbs = 2;
}

void vpu_resource_calc_num_mprbs_for_dispar(const struct vpul_pu *pu,
	__u32 img_width,
	struct result_of_calc_num_mprbs_needed *num_mprbs_needed_calc_result)
{
	num_mprbs_needed_calc_result->num_lrg_mprbs = 2;
}

void vpu_resource_calc_num_mprbs_for_inpnt(const struct vpul_pu *pu,
	__u32 img_width,
	struct result_of_calc_num_mprbs_needed *num_mprbs_needed_calc_result)
{
	get_num_mprbs_acc_to_width_for_non_filters(img_width,
						mprb_per_img_width_inpnt, 2,
						num_mprbs_needed_calc_result);
}

void vpu_resource_calc_num_mprbs_for_tn2(const struct vpul_pu *pu,
	__u32 img_width,
	struct result_of_calc_num_mprbs_needed *num_mprbs_needed_calc_result)
{
	/* all RAM ports in use */
	num_mprbs_needed_calc_result->num_lrg_mprbs = 4;
}

void vpu_resource_calc_num_mprbs_for_tn3(const struct vpul_pu *pu,
	__u32 img_width,
	struct result_of_calc_num_mprbs_needed *num_mprbs_needed_calc_result)
{
	/* all RAM ports in use */
	num_mprbs_needed_calc_result->num_lrg_mprbs = 4;
}

void vpu_resource_calc_num_mprbs_for_tn4(const struct vpul_pu *pu,
	__u32 img_width,
	struct result_of_calc_num_mprbs_needed *num_mprbs_needed_calc_result)
{
	/* all RAM ports in use */
	num_mprbs_needed_calc_result->num_lrg_mprbs = 8;
}

void vpu_resource_calc_num_mprbs_for_tn5(const struct vpul_pu *pu,
	__u32 img_width,
	struct result_of_calc_num_mprbs_needed *num_mprbs_needed_calc_result)
{
	/* all RAM ports in use */
	num_mprbs_needed_calc_result->num_lrg_mprbs = 8;
}

void vpu_resource_calc_num_mprbs_for_cnn(const struct vpul_pu *pu,
	__u32 img_width,
	struct result_of_calc_num_mprbs_needed *num_mprbs_needed_calc_result)
{
	/* all RAM ports in use */
	num_mprbs_needed_calc_result->num_lrg_mprbs = 20;
	num_mprbs_needed_calc_result->num_sm_mprbs = 21;
}

void vpu_resource_calc_num_mprbs_for_lut(const struct vpul_pu *pu,
	__u32 img_width,
	struct result_of_calc_num_mprbs_needed *num_mprbs_needed_calc_result)
{
	num_mprbs_needed_calc_result->num_sm_mprbs = 1;
}

void vpu_resource_calc_num_mprbs_for_flmorb(const struct vpul_pu *pu,
	__u32 img_width,
	struct result_of_calc_num_mprbs_needed *num_mprbs_needed_calc_result)
{
	num_mprbs_needed_calc_result->num_lrg_mprbs = 3;


}

void vpu_resource_calc_num_mprbs_for_fdepth(const struct vpul_pu *pu,
	__u32 img_width,
	struct result_of_calc_num_mprbs_needed *num_mprbs_needed_calc_result)
{
	get_num_mprbs_acc_to_width_for_non_filters(img_width,
						mprb_per_img_width_fdepth, 3,
						num_mprbs_needed_calc_result);
}

void vpu_resource_calc_num_mprbs_for_fifo(const struct vpul_pu *pu,
	__u32 img_width,
	struct result_of_calc_num_mprbs_needed *num_mprbs_needed_calc_result)
{
	if (pu->mprb_type == VPUH_MPRB_TYPE_4K)
		num_mprbs_needed_calc_result->num_lrg_mprbs = pu->n_mprbs;
	else
		num_mprbs_needed_calc_result->num_sm_mprbs = pu->n_mprbs;
}

static __s32 __vpu_resource_pu_get(struct vpu_hw_pu *pu_device,
			   struct vpul_pu *pu, __u32 flags)
{
	__s32 ret = 0;
	enum VPU_PU_TYPES block_id;
	__u32 channel_id;
	struct vpu_hw_block *block;

	BUG_ON(!pu);
	BUG_ON(!pu_device);

	if (pu->instance >= VPU_PU_NUMBER) {
		ret = -1;
		goto p_err;
	}

	block_id = pu_inst2type[pu->instance];
	block = &pu_device->table[block_id];

	channel_id = pu->instance - block->start;
	if (test_bit(VPUL_GRAPH_FLAG_FIXED,
				(const unsigned long *) &flags)) {
		if (test_bit(channel_id, block->preempted)) {
			ret = -1;
			goto p_err;
		}
	} else if (test_bit(channel_id, block->preempted)) {
		channel_id = find_first_zero_bit(block->preempted,
						 block->total);
		if (channel_id >= block->total) {
			ret = -1;
			goto p_err;
		}
		pu->instance =
			(enum vpul_pu_instance)(block->start + channel_id);
	}

	set_bit(channel_id, block->preempted);
	set_bit(channel_id, block->pre_allocated);

p_err:
	return ret;
}

static __s32 __vpu_resource_pu_put(struct vpu_hardware *vpu_hw,
				   struct vpul_pu *pu)
{
	__s32 ret = 0;
	enum VPU_PU_TYPES block_id;
	__u32 channel_id;
	struct vpu_hw_block *block;
	struct vpu_hw_pu *pu_device;

	BUG_ON(!vpu_hw);
	BUG_ON(!pu);

	pu_device = &vpu_hw->pu;

	if (pu->instance >= VPU_PU_NUMBER) {
		ret = -1;
		goto p_err;
	}

	block_id = pu_inst2type[pu->instance];
	block = &pu_device->table[block_id];

	channel_id = pu->instance - block->start;
	if (channel_id >= block->total) {
		/*
		 * vpu_err("channel_id %d is invalid(%d, %d)\n", channel_id,
		 * pu->instance, block->start);
		 */
		ret = 0 - channel_id;
		goto p_err;
	}

	pu->instance = (enum vpul_pu_instance)(block->start);
	clear_bit(channel_id, block->allocated);

p_err:
	return ret;
}

static void __vpu_resource_mprb_put(struct vpu_hardware *vpu_hw,
				    struct vpul_pu *pu)
{
	__u32 i, j;
	__u32 num_of_mprbs;
	__u32 mprb_num;
	struct vpu_hw_block *mprb_block;
	struct vpu_hw_mprb *mprb_device;

	BUG_ON(!vpu_hw);
	BUG_ON(!pu);

	mprb_device = &vpu_hw->mprb;
	num_of_mprbs = pu->n_mprbs;
	if (num_of_mprbs) {
		j = 0;
		for (i = 0; i < VPU_MAXIMAL_MPRB_CONNECTED; i++) {
			mprb_num = pu->mprbs[i];
			if (mprb_num != NO_MPRB_CONNECTED) {
				if (mprb_num < VPU_HW_NUM_LARGE_MPRBS)
					mprb_block =
					    &mprb_device->table[MPRB_large];
				else {
					mprb_block =
					    &mprb_device->table[MPRB_small];
					mprb_num -= VPU_HW_NUM_LARGE_MPRBS;
				}

				clear_bit(mprb_num, mprb_block->allocated);
				j++;
				if (j == num_of_mprbs)
					break;
			}
		}
	}
}

/**
 * This function initializes the "preempted" flags in bitmaps for all PUs
 * and MPRBs
 * it is called for each subchain, before allocation of all PUs and MPRBs
 * needed by the subchain
 * the bits set in the bitmaps indicate resources that are not available for
 * allocation
 * this is done with taking into account the options selected by "flags" :
 * - VPUL_GRAPH_FLAG_SHARED_AMONG_TASKS - resources allocated for other tasks
 *   will be regarded as "available"
 * - VPUL_GRAPH_FLAG_SHARED_AMONG_SUBCHAINS - resources allocated to other
 *   subchains in this task will be regarded as "available"
 */
static void vpu_resource_init_preempted(struct vpu_hw_pu *pu_device,
				struct vpu_hw_mprb *mprb_device,
				__u32 flags)
{
	__u32 i;

	for (i = 0; i < pu_device->total; ++i)
		bitmap_copy(pu_device->table[i].preempted,
			    pu_device->table[i].allocated,
			    pu_device->table[i].total);
	for (i = 0; i < mprb_device->total; ++i)
		bitmap_copy(mprb_device->table[i].preempted,
			    mprb_device->table[i].allocated,
			    mprb_device->table[i].total);

/*
 *	if (test_bit(VPUL_GRAPH_FLAG_SHARED_AMONG_TASKS,
 *		(const unsigned long int *)&flags)) {
 *		for (i = 0; i < pu_device->total; ++i)
 *			bitmap_zero(pu_device->table[i].preempted,
 *				    pu_device->table[i].total);
 *		for (i = 0; i < mprb_device->total; ++i)
 *			bitmap_zero(mprb_device->table[i].preempted,
 *				    mprb_device->table[i].total);
 *	}
 */
	if (!test_bit(VPUL_GRAPH_FLAG_SHARED_AMONG_SUBCHAINS,
		(const unsigned long *)&flags)) {
		for (i = 0; i < pu_device->total; ++i)
			bitmap_or(pu_device->table[i].preempted,
				  pu_device->table[i].preempted,
				  pu_device->table[i].pre_allocated,
				  pu_device->table[i].total);
		for (i = 0; i < mprb_device->total; ++i)
			bitmap_or(mprb_device->table[i].preempted,
				  mprb_device->table[i].preempted,
				  mprb_device->table[i].pre_allocated,
				  mprb_device->table[i].total);
	}
}

static __calc_nbr_of_mprbs_needed calc_nbr_of_mprbs_needed[VPU_PU_TYPES_NUMBER]
	= {
#define VPU_PU_TYPE(a, b, c) b,
#include "lib/vpul_pu_types.def"
#undef VPU_PU_TYPE
};

static void __vpu_resource_calc_num_mprbs_needed(
	struct vpul_pu *pu,
	__u32 *calculated_sizes,
	struct result_of_calc_num_mprbs_needed *num_mprbs_needed_calc_result)
{
	enum VPU_PU_TYPES pu_type;

	BUG_ON((pu->instance) >= (VPU_PU_NUMBER));
	pu_type = pu_inst2type[pu->instance];
	BUG_ON(pu_type >= VPU_PU_TYPES_NUMBER);
		calc_nbr_of_mprbs_needed[pu_type](pu, calculated_sizes[pu->in_size_idx],
						num_mprbs_needed_calc_result);
}

static void update_availability_matrix(__u8 *ram_port_row_in_matrix,
					__u32 first_ram_port,
					__u32 num_large_mprbs_needed,
					__u32 num_small_mprbs_needed,
					struct vpu_hw_mprb *mprb_device,
					const __u32 *ram_ports)
{
/**
 * adding rows to availability matrix
 * each row will reflect a PU RAM port and the MPRBs available for this port
 * input :
 * num_large_mprbs_needed : specifies number of rows to add for large MPRBs
 * (1 for each needed large MPRB)
 * num_small_mprbs_needed : specifies number of rows to add for small MPRBs
 * (1 for each needed small MPRB), these rows will follow the rows for large
 * MPRBs (if both "large" and "small" MPRBs are required)
 * the rows to be copied are specified by ram_ports
 * ram_port_row_in_matrix : points to beginning of 1st row to be added
 * first_ram_port : index in interconnect matrix to 1st RAM port of PU instance
 * operation :
 * the function copies num_mprbs_needed rows from interconnect matrix to
 * availability matrix
 * bits for all MPRBs marked as "preempted" will be cleared in the copied rows
 * bits for MPRBs of type (1K/4K) other than needed one will be cleared as well
 * in case mprb_size_type is specified as "both" :
 * bits for "small" MPRBs will be cleared in first num_large_mprbs_needed rows
 * bits for "large" MPRBs will be cleared in the other rows
 */
	__u32 i, j;
	__u8 *availability_data;
	const __u8 *interconnect_data;
	struct vpu_hw_block *mprb_block;
	__u32 interconnect_row;
	__u32 total_num_mprbs_needed;


	total_num_mprbs_needed = num_large_mprbs_needed +
					num_small_mprbs_needed;

	/* fill rows for ports used for large MPRBs (come first) */
	for (i = 0; i < num_large_mprbs_needed; i++) {
		interconnect_row = first_ram_port + ram_ports[i];
		/* mark all small MPRBs as "unavailable" in these rows */
		availability_data = ram_port_row_in_matrix +
					OFFSET_TO_SMALL_MEM_BLOCKS;
		memset(availability_data, 0, VPU_HW_NUM_SMALL_MPRBS);

		availability_data = ram_port_row_in_matrix +
					OFFSET_TO_LARGE_MEM_BLOCKS;
		interconnect_data =
		    &interconnect_matrix[interconnect_row]
					[OFFSET_TO_LARGE_MEM_BLOCKS];
		mprb_block = &mprb_device->table[MPRB_large];
		for (j = 0; j < VPU_HW_NUM_LARGE_MPRBS; j++) {
			if ((*interconnect_data) &&
			    (!test_bit(j, mprb_block->preempted)))
				*availability_data = 1;
			else
				*availability_data = 0;
			interconnect_data++;
			availability_data++;
		}
		ram_port_row_in_matrix += VPU_HW_TOT_NUM_MPRBS;
	}

	/* then fill next rows for ports used for small MPRBs */
	for (i = num_large_mprbs_needed; i < total_num_mprbs_needed; i++) {
		interconnect_row = first_ram_port + ram_ports[i];
		/* mark all large MPRBs as "unavailable" in these rows */
		availability_data = ram_port_row_in_matrix +
			OFFSET_TO_LARGE_MEM_BLOCKS;
		memset(availability_data, 0, VPU_HW_NUM_LARGE_MPRBS);

		availability_data = ram_port_row_in_matrix +
			OFFSET_TO_SMALL_MEM_BLOCKS;
		interconnect_data =
		    &interconnect_matrix[interconnect_row]
			[OFFSET_TO_SMALL_MEM_BLOCKS];
		mprb_block = &mprb_device->table[MPRB_small];
		for (j = 0; j < VPU_HW_NUM_SMALL_MPRBS; j++) {
			if ((*interconnect_data) &&
				(!test_bit(j, mprb_block->preempted)))
				*availability_data = 1;
			else
				*availability_data = 0;
			interconnect_data++;
			availability_data++;
		}
		ram_port_row_in_matrix += VPU_HW_TOT_NUM_MPRBS;
		interconnect_row++;
	}
}

static __s32 prepare_mprb_alloc_for_pu(
	__u32 cumul_num_mprbs,
	struct result_of_calc_num_mprbs_needed *num_mprbs_needed_calc_result,
	struct vpul_pu *pu,
	__u8 *ram_port_row_in_matrix,
	struct vpu_hw_mprb *mprb_device,
	struct port_index_2_pu *port_index_to_pu)
{
	__u32 first_ram_port;
	__u32 num_mprbs_large;
	__u32 num_mprbs_small;
	__u32 number_of_ram_ports;
	__u32 i;
	__s32 ret = 0;
	__u32 num_mprbs_needed;
	const __u32 *mem_ports_array;

	mem_ports_array = num_mprbs_needed_calc_result->ram_ports_array;
	num_mprbs_large = num_mprbs_needed_calc_result->num_lrg_mprbs;
	num_mprbs_small = num_mprbs_needed_calc_result->num_sm_mprbs;
	num_mprbs_needed = num_mprbs_large + num_mprbs_small;

	/* initialize all MPRB entries to "not in use" */
	for (i = 0; i < VPU_MAXIMAL_MPRB_CONNECTED; i++)
		pu->mprbs[i] = NO_MPRB_CONNECTED;

	number_of_ram_ports =
		pu_inst_2_ram_port[pu->instance].number_of_ram_ports;

	if (num_mprbs_needed > number_of_ram_ports) {
		/* failure */
		ret = -1;
		goto p_err;
	}
	if ((cumul_num_mprbs + num_mprbs_needed) > VPU_HW_TOT_NUM_MPRBS) {
		/* failure */
		ret = -1;
		goto p_err;
	}
	first_ram_port = pu_inst_2_ram_port[pu->instance].first_ram_port;

	if (num_mprbs_large)
		pu->mprb_type = VPUH_MPRB_TYPE_4K;
	else
		pu->mprb_type = VPUH_MPRB_TYPE_1K;

	update_availability_matrix(ram_port_row_in_matrix,
				first_ram_port,
				num_mprbs_large, num_mprbs_small,
				mprb_device,
				mem_ports_array);

	for (i = 0; i < num_mprbs_needed; i++) {
		port_index_to_pu[cumul_num_mprbs + i].pu = pu;
		port_index_to_pu[cumul_num_mprbs + i].port_index_in_pu =
							mem_ports_array[i];
	}
p_err:
	return ret;
}

static void allocate_mprb(__u32 port_num, __u32 mprb_num,
			  struct vpu_hw_mprb *mprb_device,
			  struct port_index_2_pu *port_index_to_pu)
{
	struct vpu_hw_block *mprb_block;
	struct vpul_pu *pu_for_this_port;
	__u32 port_num_for_pu;

	/* retrieve PU descr for which this MPRB is allocated */
	pu_for_this_port = port_index_to_pu[port_num].pu;
	port_num_for_pu = port_index_to_pu[port_num].port_index_in_pu;
	/* write MPRB num to mprbs array for PU, at index = port index */
	pu_for_this_port->mprbs[port_num_for_pu] = mprb_num;

	if (mprb_num < VPU_HW_NUM_LARGE_MPRBS)
		mprb_block = &mprb_device->table[MPRB_large];
	else {
		mprb_block = &mprb_device->table[MPRB_small];
		mprb_num -= VPU_HW_NUM_LARGE_MPRBS;
	}
	set_bit(mprb_num, mprb_block->preempted);
	set_bit(mprb_num, mprb_block->pre_allocated);
}

/**
 * this function is called after allocating an MPRB, in case of port with only
 * 1 suitable MPRB
 * its main task is to zero in availability matrix the row and column for
 * port / MPRB which was allocated (specified at input as i and j)
 * in the process, it also decrements "number of suitable MPRBs" for all ports
 * for which the allocated MPRB was previously available; if, in this process,
 * one of the "number of suitable MPRBs" values is decremented to 1, and its
 * port index k in availability matrix is < current port index (which is i),
 * this index will be the value returned by this function; otherwise, it will
 * return value of i itself.
 */
static __u32 upd_avail_matrix_after_alloc_0(
			__u8 availability_matrix[][VPU_HW_TOT_NUM_MPRBS],
			__u32 total_num_mprbs_needed,
			__u32 i, __u32 j,
			__u32 *total_mprbs_per_port,
			__u32 *total_ports_per_mprb)
{
	__u32 retval;
	__u32 k;

	retval = i;
	total_ports_per_mprb[j] = 0;
	for (k = 0; k < total_num_mprbs_needed; k++) {
		if (availability_matrix[k][j]) {
			availability_matrix[k][j] = 0;
			total_mprbs_per_port[k]--;
			/* retval shall be modified at most once */
			if ((total_mprbs_per_port[k] == 1) && (k < retval))
				retval = k;
		}
	}
	return retval;
}

/**
 * this function is called after allocating an MPRB, in case of MPRB with only
 * 1 port for which this MPRB is suitable
 * its main task is to zero in availability matrix the row and column for
 * port / MPRB which was allocated (specified at input as i and j)
 * in the process, it also decrements "number of ports" for all MPRBs that
 * could be used by this port (MPRBs for which there is a 1 in row i); if, in
 * this process, one of the "number of ports" values is decremented to 1, and
 * MPRB index k in availability matrix is < current MPRB index (which is j),
 * the value returned by this function will be k; otherwise, it will return
 * value of j itself.
 */
static __u32 upd_avail_matrix_after_alloc_1(
			__u8 availability_matrix[][VPU_HW_TOT_NUM_MPRBS],
			__u32 i, __u32 j,
			__u32 *total_mprbs_per_port,
			__u32 *total_ports_per_mprb)
{
	__u32 retval;
	__u32 k;

	retval = j;
	total_mprbs_per_port[i] = 0;
	for (k = 0; k < VPU_HW_TOT_NUM_MPRBS; k++) {
		if (availability_matrix[i][k]) {
			availability_matrix[i][k] = 0;
			total_ports_per_mprb[k]--;
			/* retval shall be modified at most once */
			if ((total_ports_per_mprb[k] == 1) && (k < retval))
				retval = k;
		}
	}
	return retval;
}

/**
 * this function is called after allocating an MPRB, in case of MPRB with
 * minimum number of ports (but > 1) for which this MPRB is suitable.
 * its main task is to zero in availability matrix the row for the port
 * to which this MPRB was allocated (specified at input as i)
 * in the process, it also decrements "number of ports" for all MPRBs that
 * could be used by this port (MPRBs for which there is a 1 in row i); if, in
 * this process, one of the "number of ports" values is decremented to 1, the
 * smallest value of index of such an MPRB is returned; otherwise, it returns
 * value 0xFF.
 */
static __u32 upd_avail_matrix_after_alloc_2(
			__u8 availability_matrix[][VPU_HW_TOT_NUM_MPRBS],
			__u32 i, __u32 *total_mprbs_per_port,
			__u32 *total_ports_per_mprb)
{
	__u32 retval;
	__u32 k;

	retval = 0xFF;
	total_mprbs_per_port[i] = 0;
	for (k = 0; k < VPU_HW_TOT_NUM_MPRBS; k++) {
		if (availability_matrix[i][k]) {
			availability_matrix[i][k] = 0;
			total_ports_per_mprb[k]--;
			if ((total_ports_per_mprb[k] == 1) && (retval == 0xFF))
				retval = k;
		}
	}
	return retval;
}

/**
 * this function is called after allocating an MPRB, in case of MPRB with
 * minimum number of ports (but > 1) for which this MPRB is suitable.
 * its main task is to zero in availability matrix the column of the MPRB that
 * was allocated (specified at input as j)
 * in the process, it also decrements "number of suitable MPRBs" for all ports
 * that could have used this MPRB (ports for which there is a 1 in column j);
 * if, in this process, one of the "number of suitable MPRBs" values is
 * decremented to 1, the smallest value of index of such a port is returned;
 * otherwise, it returns value 0xFF.
 */
static __u32 upd_avail_matrix_after_alloc_3(
			__u8 availability_matrix[][VPU_HW_TOT_NUM_MPRBS],
			__u32 j, __u32 total_num_mprbs_needed,
			__u32 *total_mprbs_per_port,
			__u32 *total_ports_per_mprb)
{
	__u32 retval;
	__u32 k;

	retval = 0xFF;
	total_ports_per_mprb[j] = 0;
	for (k = 0; k < total_num_mprbs_needed; k++) {
		if (availability_matrix[k][j]) {
			availability_matrix[k][j] = 0;
			total_mprbs_per_port[k]--;
			if ((total_mprbs_per_port[k] == 1) && (retval == 0xFF))
				retval = k;
		}
	}
	return retval;
}

static __s32 __vpu_resource_mprbs_get(
			__u8 availability_matrix[][VPU_HW_TOT_NUM_MPRBS],
			__u32 total_num_mprbs_needed,
			struct vpu_hw_mprb *mprb_device,
			struct port_index_2_pu *port_index_to_pu)
{
	/**
	 * used for comparing to total_num_mprbs_needed (successful
	 * completion), and also for back-tracking
	 */
	__u32 num_mprbs_allocated;
	__u32 total_mprbs_per_port[VPU_HW_TOT_NUM_MPRBS];
	__u32 total_ports_per_mprb[VPU_HW_TOT_NUM_MPRBS];
	__u32 i, j, k;
	__s32 ret = 0;
	__u32 min_num_ports_per_mprb;
	__u32 mprb_with_min_num_ports;
	__u32 index_of_port_with_1_mprb;
	__u32 index_of_mprb_with_1_port;

	num_mprbs_allocated = 0;

	BUG_ON( total_num_mprbs_needed > VPU_HW_TOT_NUM_MPRBS);

	/* calculate and store totals per row (for all ports) */
	for (i = 0; i < total_num_mprbs_needed; i++) {
		total_mprbs_per_port[i] = 0;
		for (j = 0; j < VPU_HW_TOT_NUM_MPRBS; j++)
			total_mprbs_per_port[i] +=
				availability_matrix[i][j];
	}

	/* calculate and store totals per column (for all MPRBs) */
	for (j = 0; j < VPU_HW_TOT_NUM_MPRBS; j++) {
		total_ports_per_mprb[j] = 0;
		for (i = 0; i < total_num_mprbs_needed; i++)
			total_ports_per_mprb[j] +=
				availability_matrix[i][j];
	}

	index_of_port_with_1_mprb = 0;
	/**
	 * the following loop is executed as long as there are ports for which
	 * there is only 1 MPRB available (total_mprbs_per_port = 1)
	 */
find_port_with_1_mprb:
	BUG_ON( total_num_mprbs_needed > VPU_HW_TOT_NUM_MPRBS);
	for (i = index_of_port_with_1_mprb; i < total_num_mprbs_needed;) {
		k = i;
		if (total_mprbs_per_port[i] == 1) {
			for (j = 0; j < VPU_HW_TOT_NUM_MPRBS; j++) {
				/* find THE mprb available for this port */
				if (availability_matrix[i][j]) {
					allocate_mprb(i, j, mprb_device,
						      port_index_to_pu);
					num_mprbs_allocated++;
					if (num_mprbs_allocated ==
						total_num_mprbs_needed)
						/* successfully completed */
						goto end_vpu_rsrc_mprbs_get;
					/* remove MPRB from
					 * availability matrix
					 */
					k = upd_avail_matrix_after_alloc_0(
						availability_matrix,
						total_num_mprbs_needed, i, j,
						total_mprbs_per_port,
						total_ports_per_mprb);
					break;
				}
			}
		}
		/**
		 * if found port #k for which number of suitable MPRBs was
		 * decremented to 1 : proceed from port #k
		 */
		if (k == i)
			i++;
		else
			i = k;
	}
	index_of_mprb_with_1_port = 0;

	/**
	 * the following loop is executed as long as there are mprbs available
	 * to only 1 port (total_ports_per_mprb == 1)
	 */
find_mprb_with_1_port:
	BUG_ON( total_num_mprbs_needed > VPU_HW_TOT_NUM_MPRBS);
	for (j = index_of_mprb_with_1_port; j < VPU_HW_TOT_NUM_MPRBS;) {
		k = j;
		if (total_ports_per_mprb[j] == 1) {
			for (i = 0; i < total_num_mprbs_needed; i++) {
				/* find THE port which can use this MPRB */
				if (availability_matrix[i][j]) {
					allocate_mprb(i, j, mprb_device,
						      port_index_to_pu);
					num_mprbs_allocated++;
					if (num_mprbs_allocated ==
						total_num_mprbs_needed)
						/* successfully completed */
						goto end_vpu_rsrc_mprbs_get;
					k = upd_avail_matrix_after_alloc_1(
					    availability_matrix, i, j,
					    total_mprbs_per_port,
					    total_ports_per_mprb);
					break;
				}
			}
		}
		/**
		 * if found MPRB #k with number of ports for which this MPRB is
		 * suitable was decremented to 1 : proceed from MPRB #k
		 */
		if (k == j)
			j++;
		else
			j = k;
	}

	/* find mprb with minimum number of ports */
find_mprb_with_min_num_ports:
	min_num_ports_per_mprb = 0xFF;
	mprb_with_min_num_ports = 0xFF;

	for (j = 0; j < VPU_HW_TOT_NUM_MPRBS; j++) {
		if ((total_ports_per_mprb[j] < min_num_ports_per_mprb) &&
		    (total_ports_per_mprb[j] != 0)) {
			min_num_ports_per_mprb = total_ports_per_mprb[j];
			mprb_with_min_num_ports = j;
		}
	}
	if (mprb_with_min_num_ports == 0xFF) {
		/* no more MPRBs available : alloc failed */
		ret = -1;
		goto end_vpu_rsrc_mprbs_get;
	}
	j = mprb_with_min_num_ports;
	/**
	 * found mprb with minimum number of ports (but non-zero) : allocate it
	 * to first suitable port
	 */
	for (i = 0; i < total_num_mprbs_needed; i++) {
		if (availability_matrix[i][j])
			break;
	}
	allocate_mprb(i, j, mprb_device, port_index_to_pu);
	num_mprbs_allocated++;
	if (num_mprbs_allocated == total_num_mprbs_needed)
		/* successfully completed */
		goto end_vpu_rsrc_mprbs_get;

	/* remove MPRB from availability matrix */
	availability_matrix[i][j] = 0;

	/* remove this row from availability matrix */
	index_of_mprb_with_1_port = upd_avail_matrix_after_alloc_2(
			availability_matrix, i,
			total_mprbs_per_port, total_ports_per_mprb);

	/* remove this column from availability matrix */
	index_of_port_with_1_mprb = upd_avail_matrix_after_alloc_3(
			availability_matrix, j, total_num_mprbs_needed,
			total_mprbs_per_port, total_ports_per_mprb);


	/* found a port for which only 1 MPRB is available ? */
	if (index_of_port_with_1_mprb != 0xFF)
		goto find_port_with_1_mprb;
	/* found an MPRB available to only 1 port ? */
	if (index_of_mprb_with_1_port != 0xFF)
		goto find_mprb_with_1_port;
	goto find_mprb_with_min_num_ports;
end_vpu_rsrc_mprbs_get:
	return ret;
}


static __u8 availability_matrix[VPU_HW_TOT_NUM_MPRBS][VPU_HW_TOT_NUM_MPRBS];
static __u32 actual_sizes[VPUL_MAX_SIZES_OP];

static __s32 vpu_resource_get_for_vertex(struct vpul_task *task,
					__u32 flags,
					struct vpul_vertex *vertex,
					const struct vpul_3dnn_process_base *proc_3dnn_base,
					struct vpu_hw_pu *pu_device,
					struct vpu_hw_mprb *mprb_device)
{
	struct result_of_calc_num_mprbs_needed mprbs_needed;
	__u32 j, k, subchain_cnt, pu_cnt, orig_pu_count;
	__u32 num_mprbs_needed;
	__u32 cumul_n_mprbs_needed;
	struct vpul_subchain *subchain;
	struct vpul_pu *pu;
	__u32 flags_copy;
	struct port_index_2_pu port_index_to_pu[VPU_HW_TOT_NUM_MPRBS];

	/* used for accessing vpul_pu and updating its contents when allocating MPRB */
	__s32 ret = 0;

	subchain_cnt = vertex->num_of_subchains;
	subchain = fst_vtx_sc_ptr(task, vertex);
	if (vertex->vtype == VPUL_VERTEXT_3DNN_PROC)
		ret = set_actual_sizes_3dnn(task, proc_3dnn_base, actual_sizes);
	else
		ret = set_actual_sizes(task, vertex, actual_sizes);
	if (ret)
		goto p_err;
	for (j = 0; j < subchain_cnt; j++, subchain++) {
		if (subchain->stype != VPUL_SUB_CH_CPU_OP) {
			/* mark "preempted" (= non-allocatable) resources */
			vpu_resource_init_preempted(pu_device, mprb_device, flags);
			orig_pu_count = subchain->num_of_pus;
			if (!test_bit(VPUL_GRAPH_FLAG_DSBL_LATENCY_BALANCING,
					(const unsigned long *) &flags)) {
				ret = latency_balancing(task, vertex, subchain, actual_sizes);
				if (ret)
					goto p_err;
			}
			/* new count includes PUs added for delay balancing */
			pu_cnt = subchain->num_of_pus;
			pu = fst_sc_pu_ptr(task, subchain);
			cumul_n_mprbs_needed = 0;
			for (k = 0; k < pu_cnt; k++, pu++) {
				flags_copy = flags;
				/* for PUs added for delay balancing :
				 * ignore VPUL_GRAPH_FLAG_FIXED
				 */
				if (k >= orig_pu_count)
					clear_bit(VPUL_GRAPH_FLAG_FIXED,
						(volatile unsigned long *)&flags_copy);
				ret = __vpu_resource_pu_get(pu_device, pu, flags_copy);
				if (ret)
					goto p_err;
				/**
				 * initializing result structure to default values,
				 * __vpu_resource_calc_num_mprbs_needed will update
				 * only members of this structure whose values are
				 * different from default values
				 */
				mprbs_needed.num_lrg_mprbs = 0;
				mprbs_needed.num_sm_mprbs = 0;
				mprbs_needed.ram_ports_array = consecutive_mem_ports;

				/* for PUs added for delay balancing : number of MPRBs needed
				 * already been calculated by delay balancing function
				 */
				if (k < orig_pu_count)
					__vpu_resource_calc_num_mprbs_needed(pu, actual_sizes, &mprbs_needed);
				else if (pu->mprb_type == VPUH_MPRB_TYPE_4K)
					mprbs_needed.num_lrg_mprbs = pu->n_mprbs;
				else
					mprbs_needed.num_sm_mprbs = pu->n_mprbs;

				if (pu->instance ==
						((flags >> VPUL_STATIC_ALLOC_PU_INSTANCE_LSB) &
							VPUL_STATIC_ALLOC_PU_INSTANCE_MASK)) {
					if (flags & VPUL_STATIC_ALLOC_LARGE_INSTEAD_SMALL_MPRB_MASK)
					{
						if (mprbs_needed.num_lrg_mprbs == 0) {
							mprbs_needed.num_lrg_mprbs = mprbs_needed.num_sm_mprbs;
							mprbs_needed.num_sm_mprbs = 0;
						}
					}
				}

				num_mprbs_needed = mprbs_needed.num_lrg_mprbs +
							mprbs_needed.num_sm_mprbs;
				pu->n_mprbs = num_mprbs_needed;
				if (num_mprbs_needed) {
					ret = prepare_mprb_alloc_for_pu(
						cumul_n_mprbs_needed,
						&mprbs_needed, pu,
						&availability_matrix[cumul_n_mprbs_needed][0],
						mprb_device,
						port_index_to_pu);
					if (ret)
						goto p_err;
					cumul_n_mprbs_needed += num_mprbs_needed;
				}
			}
			/**
			 * MPRBs allocation is performed after allocating all PU instances for
			 * subchain and calculating the number of MPRBs needed for each of them
			 */
			if (cumul_n_mprbs_needed) {
				ret = __vpu_resource_mprbs_get(availability_matrix,
								cumul_n_mprbs_needed,
								mprb_device,
								port_index_to_pu);
				if (ret)
					goto p_err;
			}
		}
	}
p_err:
	if (ret == 0)
		ret = VPU_STATUS_SUCCESS;
	else if (ret != VPU_STATUS_BAD_PARAMS)
		ret = VPU_STATUS_FAILURE;
	return ret;
}

__s32 __vpu_resource_get(struct vpu_hardware *vpu_hw, struct vpul_task *task, __u32 flags)
{
	__u32 i;
	struct vpu_hw_pu *pu_device;
	struct vpu_hw_mprb *mprb_device;
	struct vpul_vertex *vertex;
	const struct vpul_3dnn_process_base *proc_3dnn_base;
	__s32 ret = VPU_STATUS_SUCCESS;

	if ((vpu_hw) && (task)) {
		pu_device = &vpu_hw->pu;
		mprb_device = &vpu_hw->mprb;
		vertex = fst_vtx_ptr(task);
		proc_3dnn_base = fst_3dnn_process_base_ptr(task);

		/* prepare */
		for (i = 0; i < pu_device->total; ++i)
			bitmap_zero(pu_device->table[i].pre_allocated,
					pu_device->table[i].total);
		for (i = 0; i < mprb_device->total; ++i)
			bitmap_zero(mprb_device->table[i].pre_allocated,
					mprb_device->table[i].total);

		/* estimation */
		for (i = 0; i < task->t_num_of_vertices; i++, vertex++) {
			/* skip vertex if not of type "process"
			 *(3DNN processes handled separately)
			 */
			if (vertex->vtype == VPUL_VERTEXT_PROC) {
				ret = vpu_resource_get_for_vertex(task,
								flags,
								vertex,
								NULL,
								pu_device,
								mprb_device);
				if (ret != VPU_STATUS_SUCCESS)
					break;
			}
		}
		if (ret == VPU_STATUS_SUCCESS) {
			for (i = 0; i < task->t_num_of_3dnn_process_bases; i++, proc_3dnn_base++) {
				vertex = vertex_referencing_this_3dnn_proc_base(task, i);
				if (vertex) {
					ret = vpu_resource_get_for_vertex(task,
									flags,
									vertex,
									proc_3dnn_base,
									pu_device,
									mprb_device);
					if (ret != VPU_STATUS_SUCCESS)
						break;
				}
			}
			if (ret == VPU_STATUS_SUCCESS) {
				/* acquire */
				for (i = 0; i < pu_device->total; ++i)
					bitmap_or(pu_device->table[i].allocated,
						pu_device->table[i].allocated,
						pu_device->table[i].pre_allocated,
						pu_device->table[i].total);
				for (i = 0; i < mprb_device->total; ++i)
					bitmap_or(mprb_device->table[i].allocated,
						mprb_device->table[i].allocated,
						mprb_device->table[i].pre_allocated,
						mprb_device->table[i].total);
			}
		}
	} else
		ret = VPU_STATUS_BAD_PARAMS;

	return ret;
}

__s32 __vpu_resource_put(struct vpu_hardware *vpu_hw,
				struct vpul_task *task)
{
	__u32 i, j, subchain_cnt, pu_cnt;
	struct vpul_subchain *subchain;
	struct vpul_pu *pu;
	__s32 ret = 0;

	if ((!vpu_hw) || (!task)) {
		ret = VPU_STATUS_BAD_PARAMS;
		goto p_err;
	}

	subchain_cnt = task->t_num_of_subchains;
	subchain = fst_sc_ptr(task);
	for (i = 0; i < subchain_cnt; i++, subchain++) {
		pu_cnt = subchain->num_of_pus;
		if(subchain->stype!=VPUL_SUB_CH_CPU_OP){
			pu = fst_sc_pu_ptr(task, subchain);
			for (j = 0; j < pu_cnt; j++, pu++) {
				ret = __vpu_resource_pu_put(vpu_hw, pu);
				if (ret)
					goto p_err;
				__vpu_resource_mprb_put(vpu_hw, pu);
			}
		}
	}
p_err:
	if (ret == 0)
		ret = VPU_STATUS_SUCCESS;
	else if (ret != VPU_STATUS_BAD_PARAMS)
		ret = VPU_STATUS_FAILURE;
	return ret;
}
