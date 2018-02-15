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
#include "lib/vpul-hwmapper.h"
#include "lib/vpul-translator.h"
#include "lib/vpul-pu.h"
#include "lib/vpu-fwif-hw-gen.h"
#include "lib/vpul-hw-v2.1.h"
#include "vpu-hardware.h"
#include "vpul-latency-balancing.h"

struct fifos_and_mprbs_for_latency_balancing {
	__u32 number_of_fifos;
	__u32 number_of_large_mprbs;
	__u32 number_of_small_mprbs;
};

__u32 is_pu_dma_in(const struct vpul_pu *pu)
{
	__u32 retval = 0;
	enum VPU_PU_TYPES pu_type;

	pu_type = pu_inst2type[pu->instance];

	if (pu_type == VPU_PU_TYPE_DMAIN)
		retval = 1;
	return retval;
};

__u32 is_pu_dma_out(const struct vpul_pu *pu)
{
	__u32 retval = 0;
	enum VPU_PU_TYPES pu_type;

	pu_type = pu_inst2type[pu->instance];

	if (pu_type == VPU_PU_TYPE_DMAOT)
		retval = 1;
	return retval;
};

/**
 * SALB : 9 - CALB : 3 - ROIS : 2 - MDE : 1 - CCM : 1 - Joiners : 2
 * Histogram : 1 - Fast Depth : 1 - Disparity Equalization : 1 - FLAM ORB : 1
 */
#define NUM_OF_PUS_WITH_MORE_THAN_1_INPUT_PORT  22

/**
 * MDE : 1 - MAP2LIST : 1 - SLF5x5 : 3 - SLF7x7 : 3 - GLF : 2 - CCM : 1
 * Splitters : 2 - Duplicators : 5
 */
#define NUM_OF_PUS_WITH_MORE_THAN_1_OUTPUT_PORT 18

/**
 * the following array translates pu instance to sequential number of
 * multiple-output-port PU (0xFF if not a multiple-output-port PU)
 */
static const __u32 pu_inst_2_mult_outp_pu_idx[VPU_PU_NUMBER] = {
#define VPU_PU_INSTANCE(a, b, c, d, e, f, g, h, i) h,
#include "lib/vpul_pu_instances.def"
#undef VPU_PU_INSTANCE
};

/**
 * the following array translates pu instance to sequential number of
 * multiple-input-port PU (0xFF if not a multiple-input-port PU)
 */
static const __u32 pu_inst_2_mult_inp_pu_idx[VPU_PU_NUMBER] = {
#define VPU_PU_INSTANCE(a, b, c, d, e, f, g, h, i) i,
#include "lib/vpul_pu_instances.def"
#undef VPU_PU_INSTANCE
};

struct latency_info {
	/* if 0xFFFFFFFF : starts at DMA-IN */
	__u32 orig_pu_idx;
	/* if 0xFFFFFFFF : latency not calculated yet */
	__u32 cumul_min_latency;
	__u32 cumul_max_latency;
};

struct pu_latency {
	__u32 min_latency;
	__u32 max_latency;
};

/**
 * the following function returns number of pixel-bytes (VPUH_BITS_8_BITS,
 * VPUH_BITS_16_BITS or VPUH_LONG_BITS_32_BITS) at input to specified PU
 * this is done according to PU type, and only for PU types that support
 * more than 1 input (don't care about others - for delay balancing purposes),
 * the input stream is specified by inp_port_indx
 */
static __u32 get_inp_pixel_bytes(struct vpul_pu *pu, __u32 inp_port_indx)
{
	enum VPU_PU_TYPES pu_type;
	/* retval set to default = 16 bits - most frequently used value */
	__u32 retval = VPUH_BITS_16_BITS;
	const union vpul_pu_parameters *pu_params = &pu->params;
	const struct vpul_pu_salb *salb = &(pu_params->salb);
	const struct vpul_pu_calb *calb = &(pu_params->calb);
	const struct vpul_pu_rois_out *rois = &(pu_params->rois_out);
	const struct vpul_pu_mde *mde = &(pu_params->mde);
	const struct vpul_pu_flam_orb *flam_orb = &(pu_params->flam_orb);

	pu_type = pu_inst2type[pu->instance];
	switch (pu_type) {
	case VPU_PU_TYPE_SALB:
		if (inp_port_indx == 0)
			retval = salb->bits_in0;
		else
			retval = salb->bits_in1;
		break;
	case VPU_PU_TYPE_CALB:
		if (inp_port_indx == 0)
			retval = calb->bits_in0;
		else
			retval = calb->bits_in1;
		break;
	case VPU_PU_TYPE_ROIS:
		if (rois->work_mode != VPUH_ROI_MODE_LUKAS_KANADA) {
			if (inp_port_indx == 0)
				retval = rois->bits_in0;
			else
				retval = VPUH_BITS_8_BITS;
		}
		break;
	case VPU_PU_TYPE_MDE:
		retval = mde->bits_in;
		break;
	case VPU_PU_TYPE_FASTDEPTH:
		if (inp_port_indx == 2)
			retval = VPUH_LONG_BITS_32_BITS;
		break;
	case VPU_PU_TYPE_DISPARITY:
		retval = VPUH_LONG_BITS_32_BITS;
		break;
	case VPU_PU_TYPE_FLAMORB:
		if (inp_port_indx == 0)
			retval = flam_orb->bits_in;
		break;
	default:
		break;
	}
	return retval;
}

#define MAX_SIZE_LARGE_MPRB 4096
#define MAX_SIZE_SMALL_MPRB 1024
#define FIFO_INTERNAL_STORAGE_SIZE 32
#define MAX_SIZE_FIFO_WITH_LARGE_MPRB \
		(MAX_SIZE_LARGE_MPRB+FIFO_INTERNAL_STORAGE_SIZE)
#define MAX_SIZE_FIFO_WITH_SMALL_MPRB \
		(MAX_SIZE_SMALL_MPRB+FIFO_INTERNAL_STORAGE_SIZE)

static void calc_num_fifos_and_mprbs_for_latency_balancing(__u32 memory_needed,
		struct fifos_and_mprbs_for_latency_balancing *fifos_and_mprbs)
{
	__u32 num_large_mprbs;
	__u32 num_small_mprbs;
	__u32 num_fifos_without_mprb;
	__u32 remainder_large;
	__u32 remainder_small;

	num_small_mprbs = 0;
	num_fifos_without_mprb = 0;

	num_large_mprbs = memory_needed / MAX_SIZE_FIFO_WITH_LARGE_MPRB;
	remainder_large = memory_needed % MAX_SIZE_FIFO_WITH_LARGE_MPRB;
	/* if 3 small MPRBs not enough for remaining size :
	 * take one more large MPRB instead
	 */
	if (remainder_large > 3 * MAX_SIZE_FIFO_WITH_SMALL_MPRB)
		num_large_mprbs++;
	else {
		num_small_mprbs =
			remainder_large / MAX_SIZE_FIFO_WITH_SMALL_MPRB;
		remainder_small =
			remainder_large % MAX_SIZE_FIFO_WITH_SMALL_MPRB;
		if (remainder_small > FIFO_INTERNAL_STORAGE_SIZE)
			num_small_mprbs++;
		else if (remainder_small != 0)
			num_fifos_without_mprb = 1;
	}
	fifos_and_mprbs->number_of_large_mprbs = num_large_mprbs;
	fifos_and_mprbs->number_of_small_mprbs = num_small_mprbs;
	fifos_and_mprbs->number_of_fifos = num_large_mprbs + num_small_mprbs +
					    num_fifos_without_mprb;
}

#define MIN_LOGIC_LATENCY 1
#define MAX_LOGIC_LATENCY 16
#define MODE_2_N_INDEX 6
static const __u32 filter_mode_2_line_latency[MODE_2_N_INDEX] = {0, 1, 2, 3, 4, 5};

static __s32 calc_pu_latency(const struct vpul_process *process,
				struct vpul_pu *pu,
				__u32 *calc_sizes,
				struct pu_latency *latency_values)
{
	enum VPU_PU_TYPES pu_type;
	__u32 width = calc_sizes[pu->in_size_idx];
	__u32 line_latency;
	__u32 pixel_latency;
	__u32 total_latency;
	__u32 filter_size_index;
	const union vpul_pu_parameters *pu_params = &pu->params;
	const struct vpul_pu_salb *salb = &(pu_params->salb);
	const struct vpul_pu_calb *calb = &(pu_params->calb);
	const struct vpul_pu_nms *nms = &(pu_params->nms);
	const struct vpul_pu_slf *slf = &(pu_params->slf);
	const struct vpul_pu_glf *glf = &(pu_params->glf);
	const struct vpul_pu_nlf *nlf = &(pu_params->nlf);
	const struct vpul_pu_crop *crop = &(pu_params->crop);
	__u32 down_scale_op_index;
	__u32 output_size;
	__u32 input_size;
	const struct vpul_sizes *size_op;
	const struct vpul_scales *scale_params;

	line_latency = 0;
	pixel_latency = 0;
	pu_type = pu_inst2type[pu->instance];
	switch (pu_type) {
	case VPU_PU_TYPE_SALB:
		if ((salb->operation_code == 11) ||
		    (salb->operation_code == 13))
			line_latency = 1;
		break;
	case VPU_PU_TYPE_CALB:
		if (calb->operation_code == 26)
			line_latency = 1;
		break;
	case VPU_PU_TYPE_NMS:
		filter_size_index = get_filter_size_index_for_linear_filter(
					nms->support);
		if(filter_size_index >= MODE_2_N_INDEX)
			return -1;
		line_latency = filter_mode_2_line_latency[filter_size_index];
		pixel_latency = line_latency;
		if (!nms->border_mode_up)
			line_latency <<= 1;
		if (!nms->border_mode_left)
			pixel_latency <<= 1;
		break;
	case VPU_PU_TYPE_SLF5:
	case VPU_PU_TYPE_SLF7:
		filter_size_index = get_filter_size_index_for_linear_filter(
					slf->filter_size_mode);
		if(filter_size_index >= MODE_2_N_INDEX)
			return -1;
		line_latency = filter_mode_2_line_latency[filter_size_index];
		pixel_latency = line_latency;
		if (!slf->border_mode_up)
			line_latency <<= 1;
		if (!slf->border_mode_left)
			pixel_latency <<= 1;
		break;
	case VPU_PU_TYPE_GLF5:
		filter_size_index = get_filter_size_index_for_linear_filter(
					glf->filter_size_mode);
		if(filter_size_index >= MODE_2_N_INDEX)
			return -1;
		line_latency = filter_mode_2_line_latency[filter_size_index];
		pixel_latency = line_latency;
		if (!glf->border_mode_up)
			line_latency <<= 1;
		if (!glf->border_mode_left)
			pixel_latency <<= 1;
		break;
	case VPU_PU_TYPE_NLF:
		filter_size_index = get_filter_size_idx_for_non_linear_filter(
					nlf->filter_mode);
		if(filter_size_index >= MODE_2_N_INDEX)
			return -1;
		line_latency = filter_mode_2_line_latency[filter_size_index];
		pixel_latency = line_latency;
		if (!nlf->border_mode_up)
			line_latency <<= 1;
		if (!nlf->border_mode_left)
			pixel_latency <<= 1;
		break;
	case VPU_PU_TYPE_CROP:
		if (crop->work_mode == VPUH_CROP_MODE_CROP) {
			line_latency = crop->roi_starty;
			pixel_latency = crop->roi_startx;
		}
		break;
	case VPU_PU_TYPE_DNSCALER:
		down_scale_op_index = pu->out_size_idx;
		/* access "size operation" according to out_size_idx */
		size_op = &process->io.sizes[down_scale_op_index];
		/* make sure it's a "scale" operation - if not, return error */
		if (size_op->type != VPUL_SIZEOP_SCALE)
			return -1;
		scale_params = &process->io.scales[size_op->op_ind];
		output_size = scale_params->vertical.numerator;
		input_size = scale_params->vertical.denominator;
		/* make sure it's downscaling - if not, return error */
		if (output_size > input_size)
			return -1;
		/* latency = ceil(input_size / output_size) - 1 */
		input_size += (output_size - 1);
		line_latency = (input_size / output_size) - 1;
		output_size = scale_params->horizontal.numerator;
		input_size = scale_params->horizontal.denominator;
		/* make sure it's downscaling - if not, return error */
		if (output_size > input_size)
			return -1;
		/* latency = ceil(input_size / output_size) - 1 */
		input_size += (output_size - 1);
		pixel_latency = (input_size / output_size) - 1;
		break;
	default:
		break;
	}
	total_latency = line_latency * width + pixel_latency;
	latency_values->min_latency = total_latency + MIN_LOGIC_LATENCY;
	latency_values->max_latency = total_latency + MAX_LOGIC_LATENCY;
	return 0;
}

static __s32 add_slf7x7_as_fifo(struct vpul_task *task,
				struct vpul_subchain *subchain,
				__u32 dest_pu_in_port_idx, __u32 dest_pu_idx,
				struct vpul_pu *dest_pu,
				struct vpul_pu *first_pu, __u32 memory_needed,
				__u32 *number_of_added_pus)
{
	/* TBA */
	return -1;
}

static __s32 add_fifo(struct vpul_task *task, struct vpul_subchain *subchain,
			__u32 dest_pu_in_port_idx, __u32 dest_pu_idx,
			struct vpul_pu *dest_pu, struct vpul_pu *first_pu,
			__u32 memory_needed, __u32 pixel_byte,
			__u32 *number_of_added_pus)
{
	struct fifos_and_mprbs_for_latency_balancing fifos_and_mprbs;
	struct vpul_pu *prev_pu;
	__u32 prev_pu_idx;
	struct vpul_pu *fifo_pu_to_add;
	__u32 curr_pu_idx;
	__u32 num_of_pus_in_subch;
	__u32 i;
	__u32 prev_pu_out_port_idx;
	union vpul_pu_parameters *pu_params;
	struct vpul_pu_fifo *fifo_params;

	calc_num_fifos_and_mprbs_for_latency_balancing(memory_needed,
							&fifos_and_mprbs);

	*number_of_added_pus += fifos_and_mprbs.number_of_fifos;
	if (*number_of_added_pus > NUMBER_OF_SPARE_PUS_FOR_LATENCY_BALANCING)
		return -1;
	num_of_pus_in_subch = subchain->num_of_pus;
	subchain->num_of_pus += fifos_and_mprbs.number_of_fifos;
	task->t_num_of_pus += fifos_and_mprbs.number_of_fifos;

	/* add as many FIFOs as required */
	for (i = 0; i < fifos_and_mprbs.number_of_fifos; i++) {
		/* index in subchain of FIFO PU to add */
		curr_pu_idx = num_of_pus_in_subch + i;

		/* insert FIFO PU to add between previous PU and dest PU */
		prev_pu_idx = dest_pu->in_connect[dest_pu_in_port_idx].pu_idx;
		prev_pu = first_pu + prev_pu_idx;

		prev_pu_out_port_idx =
			dest_pu->in_connect[dest_pu_in_port_idx].s_pu_out_idx;

		/* only 1 input connection to FIFO, its number is 0 */


		fifo_pu_to_add = first_pu + curr_pu_idx;
		fifo_pu_to_add->n_in_connect = 1;
		fifo_pu_to_add->in_connect[0].pu_idx = prev_pu_idx;
		fifo_pu_to_add->in_connect[0].s_pu_out_idx =
							prev_pu_out_port_idx;
		fifo_pu_to_add->n_out_connect = 1;

		dest_pu->in_connect[dest_pu_in_port_idx].pu_idx = curr_pu_idx;
		dest_pu->in_connect[dest_pu_in_port_idx].s_pu_out_idx = 0;

		/* fill vpul_pu struct for added FIFO PU */
		/* info related to MPRB needed (1 small, 1 large or none) */
		if (i < fifos_and_mprbs.number_of_large_mprbs) {
			fifo_pu_to_add->n_mprbs = 1;
			fifo_pu_to_add->mprb_type = VPUH_MPRB_TYPE_4K;
		} else if (i < (fifos_and_mprbs.number_of_large_mprbs +
				fifos_and_mprbs.number_of_small_mprbs)) {
			fifo_pu_to_add->n_mprbs = 1;
			fifo_pu_to_add->mprb_type = VPUH_MPRB_TYPE_1K;
		} else
			fifo_pu_to_add->n_mprbs = 0;

		/**
		 * an available instance (maybe other than VPU_PU_FIFO_0) will
		 * actually be allocated - VPUL_GRAPH_FLAG_FIXED option is
		 * ignored for PUs added due to latency balancing
		 */
		fifo_pu_to_add->instance = VPU_PU_FIFO_0;
		fifo_pu_to_add->in_size_idx = dest_pu->in_size_idx;
		fifo_pu_to_add->out_size_idx = prev_pu->out_size_idx;
		pu_params = &fifo_pu_to_add->params;
		fifo_params = &(pu_params->fifo);
		fifo_params->bits_in = pixel_byte;
	}
	return 0;
}

static __s32 calc_input_stream_latency(const struct vpul_process *process,
					__u32 *cumul_min_latencies,
					__u32 *cumul_max_latencies,
					__u32 *calc_sizes,
					struct vpul_pu *end_pu,
					struct vpul_pu *first_pu,
					__u32 inp_stream_idx,
					__u32 common_ancestor,
					__u32 inp_stream_bitmap,
					struct latency_info *latency_info_arr,
					struct pu_latency *min_max_latencies)
{
	__u32 pu_idx;
	__u32 mult_inp_pu_idx;
	struct vpul_pu *curr_pu;
	struct pu_latency latency_values;

	pu_idx = end_pu->in_connect[inp_stream_idx].pu_idx;

	/**
	 * proceed only if this input stream has a common ancestor with the
	 * other input streams; this is reflected by the bit for this stream
	 * being set in inp_stream_bitmap
	 */
	if (!(inp_stream_bitmap & (1 << inp_stream_idx))) {
		cumul_min_latencies[inp_stream_idx] = 0xFFFFFFFF;
		cumul_max_latencies[inp_stream_idx] = 0xFFFFFFFF;
		return 0;
	}
	cumul_min_latencies[inp_stream_idx] = 0;
	cumul_max_latencies[inp_stream_idx] = 0;
	/* note : common ancestor's latency is not included */
	while (pu_idx != common_ancestor) {
		curr_pu = first_pu + pu_idx;
		if ((is_pu_dma_in(curr_pu)) || (curr_pu->n_in_connect == 0))
			break;
		if (curr_pu->n_in_connect == 1) {
			if (min_max_latencies[pu_idx].min_latency ==
					0xFFFFFFFF) {
				if (calc_pu_latency(process, curr_pu,
							calc_sizes,
							&latency_values))
					return -1;
				cumul_min_latencies[inp_stream_idx] +=
						latency_values.min_latency;
				cumul_max_latencies[inp_stream_idx] +=
						latency_values.max_latency;
				/** store calculated values for
				 * possible future re-use
				 */
				min_max_latencies[pu_idx].min_latency =
						latency_values.min_latency;
				min_max_latencies[pu_idx].max_latency =
						latency_values.max_latency;
			} else {
				/** min/max latencies have already been
				 * calculated for this PU
				 */
				cumul_min_latencies[inp_stream_idx] +=
					min_max_latencies[pu_idx].min_latency;
				cumul_min_latencies[inp_stream_idx] +=
					min_max_latencies[pu_idx].max_latency;
			}
			pu_idx = curr_pu->in_connect[0].pu_idx;
		} else {
			/* multiple-input pu */
			mult_inp_pu_idx =
				pu_inst_2_mult_inp_pu_idx[curr_pu->instance];
			pu_idx = latency_info_arr[mult_inp_pu_idx].orig_pu_idx;
			if (pu_idx == 0xFFFFFFFF)
				break;
			cumul_min_latencies[inp_stream_idx] +=
			latency_info_arr[mult_inp_pu_idx].cumul_min_latency;
			cumul_max_latencies[inp_stream_idx] +=
			latency_info_arr[mult_inp_pu_idx].cumul_max_latency;
		}
	}
	return 0;
}

void calculate_latency_diff(__u32 *min_latencies,
				__u32 *max_latencies,
				__u32 *latency_diffs,
				__u32 num_input_streams)
{
	__u32 largest_max_val;
	__u32 i, j;

	for (i = 0; i < num_input_streams; i++) {
		/**
		 * find largest max latency for all input streams other
		 * than i
		 * all input streams that don't share common ancestor
		 * (indicated by "min_latencies[k] == 0xFFFFFFFF") are excluded
		 * from latency_diff calculation
		 */
		if (min_latencies[i] == 0xFFFFFFFF)
			latency_diffs[i] = 0;
		else {
			largest_max_val = 0;
			for (j = 0; j < num_input_streams; j++) {
				if ((max_latencies[j] > largest_max_val) &&
				    (min_latencies[j] != 0xFFFFFFFF) &&
				    (j != i))
					largest_max_val = max_latencies[j];
			}
			if (min_latencies[i] < largest_max_val)
				latency_diffs[i] =
					largest_max_val - min_latencies[i];
			else
				latency_diffs[i] = 0;
		}
	}
}

static __s32 latency_balance(struct vpul_task *task,
				const struct vpul_process *process,
				struct vpul_subchain *subchain,
				__u32 *calc_sizes,
				struct latency_info *latency_info_array,
				struct pu_latency *min_max_latencies,
				__u32 end_pu_index,
				struct vpul_pu *end_pu,
				struct vpul_pu *first_pu,
				__u32 recursion_count,
				__u32 *number_of_added_pus)
{
	struct vpul_pu *curr_pu;
	__u32 inp_stream_idx;
	__u32 pu_index;
	__u32 pix_bytes;
	__u32 common_ancestor;
	__u32 curr_inp_stream_bitmap;
	__u32 new_input_stream_bitmap;
	__u32 full_input_stream_bitmap;
	__u32 num_input_streams;
	__u32 end_mult_inp_pu_idx;
	__u32 mult_inp_pu_idx;
	__u32 mult_out_pu_idx;
	__u32 i;
	struct pu_latency latency_values;
	__u32 max_min_latency;
	__u32 max_max_latency;
	__u32 memory_needed;
	__u32 cumul_min_latencies[VPUL_PU_MAX_PORTS];
	__u32 cumul_max_latencies[VPUL_PU_MAX_PORTS];
	__u32 latency_diffs[VPUL_PU_MAX_PORTS];
	__u32 inp_stream_bitmap[NUM_OF_PUS_WITH_MORE_THAN_1_OUTPUT_PORT];

	/* limit check on number of recursions */
	recursion_count++;
	if (recursion_count > NUM_OF_PUS_WITH_MORE_THAN_1_INPUT_PORT)
		return -1;

	end_mult_inp_pu_idx = pu_inst_2_mult_inp_pu_idx[end_pu->instance];
	/* if latency already calculated for this multiple-input PU : exit */
	if (latency_info_array[end_mult_inp_pu_idx].cumul_min_latency !=
			0xFFFFFFFF)
		return 0;

	/* First, find common ancestor PU of all input streams */
	num_input_streams = end_pu->n_in_connect;
	curr_inp_stream_bitmap = 0;
	/* full input stream bitmap = all num_input_streams bits set */
	full_input_stream_bitmap = (1 << num_input_streams) - 1;
	memset(inp_stream_bitmap, 0, sizeof(inp_stream_bitmap));
	common_ancestor = 0xFFFFFFFF;
	for (inp_stream_idx = 0; inp_stream_idx < num_input_streams;
			inp_stream_idx++) {
		pu_index = end_pu->in_connect[inp_stream_idx].pu_idx;
		/**
		 * the following loop could be a "while 1" loop with "break"
		 * conditions; the limit-check on VPU_PU_NUMBER is meant only
		 * for avoiding the possibility of an endless loop
		 */
		for (i = 0; i < VPU_PU_NUMBER; i++) {
			curr_pu = first_pu + pu_index;
			if (curr_pu->n_out_connect > 1) {
				/* multiple output - is common ancestor ? */
				mult_out_pu_idx =
				pu_inst_2_mult_outp_pu_idx[curr_pu->instance];
				new_input_stream_bitmap =
					inp_stream_bitmap[mult_out_pu_idx] |
						(1 << inp_stream_idx);
				inp_stream_bitmap[mult_out_pu_idx] =
						new_input_stream_bitmap;
				/* check if other bits also set in bitmap */
				if ((new_input_stream_bitmap !=
					(1 << inp_stream_idx)) &&
				    (new_input_stream_bitmap !=
						curr_inp_stream_bitmap)) {
					curr_inp_stream_bitmap =
						new_input_stream_bitmap;
					common_ancestor = pu_index;
					if (curr_inp_stream_bitmap ==
					    full_input_stream_bitmap)
						break;
				}
			}
			if ((is_pu_dma_in(curr_pu)) ||
						(curr_pu->n_in_connect == 0))
				break;
			if (curr_pu->n_in_connect == 1)
				pu_index = curr_pu->in_connect[0].pu_idx;
			else {
				if (latency_balance(task, process, subchain,
						calc_sizes, latency_info_array,
						min_max_latencies,
						pu_index, curr_pu, first_pu,
						recursion_count,
						number_of_added_pus) != 0)
					return -1;
				mult_inp_pu_idx =
				pu_inst_2_mult_inp_pu_idx[curr_pu->instance];
				pu_index =
					latency_info_array
						[mult_inp_pu_idx].orig_pu_idx;
				if (pu_index == 0xFFFFFFFF)
					break;
			}
		}
		/* if didn't exit loop on "break" : cyclic chaining (illegal) */
		if (i == VPU_PU_NUMBER)
			return -1;
	}
	latency_info_array[end_mult_inp_pu_idx].orig_pu_idx = common_ancestor;
	if (common_ancestor == 0xFFFFFFFF) {
		latency_info_array[end_mult_inp_pu_idx].cumul_min_latency = 0;
		latency_info_array[end_mult_inp_pu_idx].cumul_max_latency = 0;
		/* try to find "hidden" common ancestors - TBA */
	} else {
		/* calculate cumulated latency for each of the inputs */
		for (inp_stream_idx = 0; inp_stream_idx < num_input_streams;
				inp_stream_idx++) {
			if (calc_input_stream_latency(process,
							cumul_min_latencies,
							cumul_max_latencies,
							calc_sizes,
							end_pu, first_pu,
							inp_stream_idx,
							common_ancestor,
							curr_inp_stream_bitmap,
							latency_info_array,
							min_max_latencies))
				return -1;
		}
		calculate_latency_diff(cumul_min_latencies, cumul_max_latencies,
				    latency_diffs, num_input_streams);
		/**
		 * cumulated latency has now been calculated for all input
		 * streams : proceed with latency balancing between all input
		 * streams to this PU
		 */
		for (i = 0; i < num_input_streams; i++) {
			pix_bytes = get_inp_pixel_bytes(end_pu, i);
			if (pix_bytes == VPUH_LONG_BITS_32_BITS) {
				memory_needed = latency_diffs[i] << 2;
				if (add_slf7x7_as_fifo(task, subchain, i,
						    end_pu_index, end_pu,
						    first_pu, memory_needed,
						    number_of_added_pus))
					return -1;
			} else {
				if (pix_bytes == VPUH_BITS_16_BITS)
					memory_needed = latency_diffs[i] << 1;
				else
					memory_needed = latency_diffs[i];
				if (add_fifo(task, subchain, i, end_pu_index,
						end_pu, first_pu,
						memory_needed, pix_bytes,
						number_of_added_pus))
					return -1;
			}
		}

		/* finally, calculate latency for end PU itself */
		calc_pu_latency(process, end_pu, calc_sizes,
					&latency_values);
		max_min_latency = 0;
		max_max_latency = 0;
		for (i = 0; i < num_input_streams; i++) {
			if (cumul_min_latencies[i] != 0xFFFFFFFF) {
				if (cumul_min_latencies[i] > max_min_latency)
					max_min_latency =
						cumul_min_latencies[i];
				if (cumul_max_latencies[i] > max_max_latency)
					max_max_latency =
						cumul_max_latencies[i];
			}
		}
		latency_info_array[end_mult_inp_pu_idx].cumul_min_latency =
				latency_values.min_latency + max_min_latency;
		latency_info_array[end_mult_inp_pu_idx].cumul_max_latency =
				latency_values.max_latency + max_max_latency;
	}
	return 0;
}

__s32 latency_balancing(struct vpul_task *task,
			struct vpul_vertex *vertex,
			struct vpul_subchain *subchain,
			__u32 *calculated_sizes)
{
	__u32 num_of_pus;
	struct vpul_pu *first_pu;
	struct vpul_pu *curr_pu;
	struct vpul_process *process;
	__u32 i;
	__u32 number_of_added_pus;
	struct latency_info
		latency_info_array[NUM_OF_PUS_WITH_MORE_THAN_1_INPUT_PORT];
	struct pu_latency min_max_latencies[VPU_PU_NUMBER];

	if (vertex->vtype != VPUL_VERTEXT_PROC)
		/* TBA for 3DNN process - will be implemented later */
		return 0;
	process = &vertex->proc;
	num_of_pus = subchain->num_of_pus;
	/* initialize latencies for all PUs to "not calculated" */
	for (i = 0; i < VPU_PU_NUMBER; i++)
		min_max_latencies[i].min_latency = 0xFFFFFFFF;
	/* initialize entries for all multiple-input PUs to "not calculated" */
	for (i = 0; i < NUM_OF_PUS_WITH_MORE_THAN_1_INPUT_PORT; i++)
		latency_info_array[i].cumul_min_latency = 0xFFFFFFFF;
	number_of_added_pus = 0;
	first_pu = fst_sc_pu_ptr(task, subchain);
	curr_pu = first_pu;
	if ((num_of_pus != 0) && (curr_pu != NULL)) {
		for (i = 0; i < num_of_pus; i++, curr_pu++) {
			/**
			 * if multiple inputs to this PU, and isn't DMA-IN :
			 * perform latency balancing at this PU
			 */
			if ((curr_pu->n_in_connect > 1) &&
			    (!is_pu_dma_in(curr_pu))) {
				if (latency_balance(task, process, subchain,
						calculated_sizes,
						latency_info_array,
						min_max_latencies,
						i, curr_pu, first_pu, 0,
						&number_of_added_pus) != 0)
					return -1;
			}
		}
	}
	return 0;
}
