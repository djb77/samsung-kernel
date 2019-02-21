#include <stdio.h>

#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/poll.h>
#include <fcntl.h>

#include <ctype.h>
#include <termios.h>
#include <signal.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "test-client.h"
#include "test-case.h"
#include "test-sample.h"
#include "test-util.h"
#include <vs4l.h>
#include <lib/vpul-ds.h>
#include <vpu-control.h>

#define vpu_info printf

void print_td_info(struct vpul_task *task)
{
	int i, j, k, l;
	int n_vertex, n_subchain, n_pu;
	int n_mem, n_edge, n_iteration;
	int n_slice_group;
	struct vpul_vertex *vertex;
	struct vpul_process *process;
	struct vpul_subchain *subchain;
	struct vpul_pu *pu;

	struct vpul_3dnn_process *process3dnn;
	struct vpul_3dnn_process_base *process3_base;
	struct vpul_3dnn_process_base *process3;
	struct vpul_3dnn_layer *layer;

	vpu_info("[TASK:%d]\n", task->id);
	vpu_info("priority : %d\n", task->priority);
	vpu_info("total_size : %d\n", task->total_size);
	vpu_info("t_num_of_vertices : %d\n", task->t_num_of_vertices);
	vpu_info("t_num_of_3dnn_process_bases : %d\n", task->t_num_of_3dnn_process_bases);
	vpu_info("t_num_of_subchains : %d\n", task->t_num_of_subchains);
	vpu_info("t_num_of_pus : %d\n", task->t_num_of_pus);

	n_mem = task->n_memmap_desc;
	for (i = 0; i < n_mem; ++i) {
		vpu_info("(MMAP:%d) type : %d, index : %d, w : %d, h : %d, n : %d, l : %d\n", i,
			task->memmap_desc[i].mtype,
			task->memmap_desc[i].index,
			task->memmap_desc[i].image_sizes.width,
			task->memmap_desc[i].image_sizes.height,
			task->memmap_desc[i].image_sizes.pixel_bytes,
			task->memmap_desc[i].image_sizes.line_offset);
	}

	n_mem = task->n_external_mem_addresses;
	for (i = 0; i < n_mem; ++i) {
		vpu_info("(EXT:%d) addr : 0x%X\n", i, task->external_mem_addr[i]);
	}

	n_mem = task->n_internal_rams;
	vpu_info("n_internal_rams : %d\n", n_mem);
	/* for (j = 0; j < n_mem; ++j) {
		vpu_info("(MMAP:%d) %d\n", j, vertex[i].proc.io.memmap_desc_idx[j]);
	} */

	n_vertex = task->t_num_of_vertices;
	vertex = (struct vpul_vertex *)((char *)task + task->vertices_vec_ofs);
	for (i = 0; i < n_vertex; ++i) {
		vpu_info("[VERTEX:%d] %ld\n", i, (ulong)&vertex[i] - (ulong)task);
		vpu_info("vtype : %d\n", vertex[i].vtype);
		vpu_info("n_out_edges : %d\n", vertex[i].n_out_edges);

		n_edge = vertex[i].n_out_edges;
		for (j = 0; j < n_edge; ++j) {
			vpu_info("(EDGE:%d) index : %d\n", j, vertex[i].out_edges[j].dst_vtx_idx);
		}

		vpu_info("loop.type : %d\n", vertex[i].loop.type);
		vpu_info("loop.id : %d\n", vertex[i].loop.id);
		vpu_info("loop.n_end_loop_edges : %d\n", vertex[i].loop.n_end_loop_edges);
		vpu_info("loop.iterations : %d\n", vertex[i].loop.iterations);

		vpu_info("num_of_subchains : %d\n", vertex[i].num_of_subchains);
		vpu_info("n_subchain_on_first_iteration : %d\n", vertex[i].n_subchain_on_first_iteration);
		vpu_info("n_subchain_on_last_iteration : %d\n", vertex[i].n_subchain_on_last_iteration);

		if (vertex[i].vtype == VPUL_VERTEXT_PROC)
			goto p_proc;
		else if (vertex[i].vtype == VPUL_VERTEXT_3DNN_PROC)
			goto p_proc_3dnn;
		else
			continue;

p_proc:
		process = &vertex[i].proc;

		vpu_info("\t" "[PROC] %ld\n", (ulong)process - (ulong)task);

		n_mem = process->io.n_dynamic_map_rois;
		vpu_info("\t" "n_dynamic_map_rois : %d\n", n_mem);
		for (j = 0; j < n_mem; ++j) {
			vpu_info("\t" "(DROI:%d) %d\n", j,
				process->io.dynamic_map_roi[j].memmap_idx);
		}

		n_mem = process->io.n_fixed_map_roi;
		vpu_info("\t" "n_fixed_map_roi : %d\n", n_mem);
		for (j = 0; j < n_mem; ++j) {
			vpu_info("\t" "(FROI:%d) %d %d %d %d %d\n", j, process->io.fixed_map_roi[j].memmap_idx,
				process->io.fixed_map_roi[j].roi.first_col,
				process->io.fixed_map_roi[j].roi.first_line,
				process->io.fixed_map_roi[j].roi.width,
				process->io.fixed_map_roi[j].roi.height);
		}

		n_iteration = process->io.n_inout_types;
		vpu_info("\t" "n_inout_types : %d\n", n_iteration);
		for (j = 0; j < n_iteration; ++j) {
			vpu_info("\t" "(IOTYPE:%d) %d %d %d\n", j,
				process->io.inout_types[j].is_dynamic,
				process->io.inout_types[j].roi_index,
				process->io.inout_types[j].n_insets_per_dynamic_roi);
		}

		n_iteration = process->io.n_sizes_op;
		vpu_info("\t" "n_sizes_op : %d\n", n_iteration);
		for (j = 0; j < n_iteration; ++j) {
			vpu_info("\t" "(SIZE:%d) %d %d %d\n", j,
				process->io.sizes[j].type,
				process->io.sizes[j].op_ind,
				process->io.sizes[j].src_idx);
		}

		n_iteration = process->io.n_scales;
		vpu_info("\t" "n_scales : %d\n", n_iteration);
		for (j = 0; j < n_iteration; ++j) {
			vpu_info("\t" "(SCALE:%d) %d/%d %d/%d\n", j,
				process->io.scales[j].horizontal.numerator,
				process->io.scales[j].horizontal.denominator,
				process->io.scales[j].vertical.numerator,
				process->io.scales[j].vertical.denominator);
		}

		n_iteration = process->io.n_croppers;
		vpu_info("\t" "n_croppers : %d\n", n_iteration);
		for (j = 0; j < n_iteration; ++j) {
			vpu_info("\t" "(CROP:%d) %d %d %d %d\n", j,
				process->io.croppers[j].Left,
				process->io.croppers[j].Right,
				process->io.croppers[j].Top,
				process->io.croppers[j].Bottom);
		}

		n_iteration = process->io.n_static_coff;
		vpu_info("\t" "n_static_coff : %d\n", n_iteration);
		for (j = 0; j < n_iteration; ++j) {
			vpu_info("\t" "(SCOFF:%d) %d\n", j,
				process->io.static_coff[j]);
		}

		vpu_info("\t" "param_loop.n_exp_per_sets : %d\n", process->io.param_loop.n_exp_per_sets);
		vpu_info("\t" "param_loop.n_sets : %d\n", process->io.param_loop.n_sets);
		vpu_info("\t" "param_loop.fix_inc : %d\n", process->io.param_loop.fix_inc);
		vpu_info("\t" "param_loop.per_fst : %d\n", process->io.param_loop.per_fst);

		vpu_info("\t" "n_tiles : %d\n", process->io.n_tiles);
		vpu_info("\t" "n_insets : %d\n", process->io.n_insets);
		vpu_info("\t" "n_presets_map_desc : %d\n", process->io.n_presets_map_desc);
		vpu_info("\t" "n_postsets_map_desc : %d\n", process->io.n_postsets_map_desc);
		vpu_info("\t" "n_subchain_before_insets : %d\n", process->io.n_subchain_before_insets);
		vpu_info("\t" "n_subchain_after_insets : %d\n", process->io.n_subchain_after_insets);
		vpu_info("\t" "n_invocations_per_input_tile : %d\n", process->io.n_invocations_per_input_tile);
		vpu_info("\t" "max_input_sets_slots : %d\n", process->io.max_input_sets_slots);

		goto p_chain;

p_proc_3dnn:
		process3dnn = &vertex[i].proc3dnn;
		process3_base = (struct vpul_3dnn_process_base *)((char *)task + task->process_bases_3dnn_vec_ofs);

		vpu_info("\t" "[PROC3] %ld\n", (ulong)process3dnn - (ulong)task);
		vpu_info("\t" "base_3dnn_ind : %d\n", process3dnn->base_3dnn_ind);
		vpu_info("\t" "first_3dnn_layer : %d\n", process3dnn->first_3dnn_layer);
		vpu_info("\t" "num_of_3dnn_layers : %d\n", process3dnn->num_of_3dnn_layers);
		vpu_info("\t" "Priority : %d\n", process3dnn->Priority);

		process3 = &process3_base[process3dnn->base_3dnn_ind];

		n_iteration = process3->io.n_static_coff;
		vpu_info("\t" "n_static_coff : %d\n", n_iteration);
		for (j = 0; j < n_iteration; ++j) {
			vpu_info("\t" "(SCOFF:%d) %d\n", j, process3->io.static_coff[j]);
		}

		n_iteration = process3->io.n_sizes_op;
		vpu_info("\t" "n_sizes_op : %d\n", n_iteration);
		for (j = 0; j < n_iteration; ++j) {
			vpu_info("\t" "(SIZE:%d) %d %d %d\n", j,
				process3->io.sizes_3dnn[j].type,
				process3->io.sizes_3dnn[j].op_ind,
				process3->io.sizes_3dnn[j].src_idx);
		}

		n_iteration = process3->io.n_scales;
		vpu_info("\t" "n_scales : %d\n", n_iteration);

		n_iteration = process3->io.n_croppers;
		vpu_info("\t" "n_croppers : %d\n", n_iteration);

		n_iteration = process3->io.n_3dnn_inouts;
		vpu_info("\t" "n_3dnn_inouts : %d\n", n_iteration);

		vpu_info("\t" "number_of_layers : %d\n", process3->number_of_layers);
		for (j = 0; j < process3->number_of_layers; ++j) {
			layer = &process3->layers[j];
			vpu_info("\t\t" "dim_size_input : %dx%d\n", layer->dim_size_input.x,
				layer->dim_size_input.y);
			vpu_info("\t\t" "n_total_input_slices : %d\n", layer->n_total_input_slices);
			vpu_info("\t\t" "n_slices_per_input_slice_group : %d\n", layer->n_slices_per_input_slice_group);
			vpu_info("\t\t" "n_output_slices : %d\n", layer->n_output_slices);
			vpu_info("\t\t" "coeff_size_per_slice_group_dma16 : %d\n", layer->coeff_size_per_slice_group_dma16);
			vpu_info("\t\t" "coeff_size_per_slice_group_dma32 : %d\n", layer->coeff_size_per_slice_group_dma32);

			vpu_info("\t\t" "n_scale_factors : %d\n", layer->n_scale_factors);
			for (k = 0; k < layer->n_scale_factors; ++k) {
				vpu_info("\t\t" "(SCALEF:%d) %d\n", k,
					layer->scale_factors[k]);
			}

			n_slice_group = layer->n_total_input_slices / layer->n_slices_per_input_slice_group;
			vpu_info("\t\t" "n_slice_group : %d\n", n_slice_group);
			for (k = 0; k < VPUL_MAX_3DNN_INOUT; ++k) {
				vpu_info("\t\t" "(INOUT:%d) %d, %d, %d, %d\n", k,
					layer->inout_3dnn[k].inout_3dnn_type,
					layer->inout_3dnn[k].bytes_per_pixel_or_dma_bytes_width.bytes_per_pixel,
					layer->inout_3dnn[k].bytes_per_pixel_or_dma_bytes_width.dma_bytes_width,
					layer->inout_3dnn[k].mem_descr_index);
			}
		}

p_chain:
		n_subchain = vertex[i].num_of_subchains;
		subchain = (struct vpul_subchain *)((char *)task + vertex[i].sc_ofs);

		for (j = 0; j < n_subchain; ++j) {
			vpu_info("\t\t" "[SC:%d] %ld\n", j, (ulong)&subchain[j] - (ulong)task);
			vpu_info("\t\t" "id : %d\n", subchain[j].id);
			vpu_info("\t\t" "stype : %d\n", subchain[j].stype);
			vpu_info("\t\t" "num_of_pus : %d\n", subchain[j].num_of_pus);

			if (subchain[j].stype == VPUL_SUB_CH_HW) {
				n_pu = subchain[j].num_of_pus;
				pu = (struct vpul_pu *)((char *)task + subchain[j].pus_ofs);

				for (k = 0; k < n_pu; ++k) {
					vpu_info("\t\t\t" "[PU:%d] %ld\n", k, (ulong)&pu[k] - (ulong)task);
					vpu_info("\t\t\t" "instance : %d\n", pu[k].instance);
					vpu_info("\t\t\t" "mprb_type : %d\n", pu[k].mprb_type);
					vpu_info("\t\t\t" "in_size_idx : %d\n", pu[k].in_size_idx);
					vpu_info("\t\t\t" "out_size_idx : %d\n", pu[k].out_size_idx);
					vpu_info("\t\t\t" "n_in_connect : %d\n", pu[k].n_in_connect);
					vpu_info("\t\t\t" "n_out_connect : %d\n", pu[k].n_out_connect);
					vpu_info("\t\t\t" "n_mprbs : %d\n", pu[k].n_mprbs);
					n_iteration = pu[k].n_mprbs;
					for (l = 0; l < n_iteration; ++l) {
						vpu_info("\t\t\t" "(MPRB:%d) %d\n", l, pu[k].mprbs[l]);
					}

					switch (pu[k].op_type) {
					case VPUL_OP_DMA:
						vpu_info("\t\t\t" "inout_index : %d\n", pu[k].params.dma.inout_index);
						vpu_info("\t\t\t" "offset_lines_inc : %d\n", pu[k].params.dma.offset_lines_inc);
						break;
					case VPUL_OP_FULL_SALB:
						vpu_info("\t\t\t" "bits_in0 : %d\n", pu[k].params.salb.bits_in0);
						vpu_info("\t\t\t" "bits_in1 : %d\n", pu[k].params.salb.bits_in1);
						vpu_info("\t\t\t" "bits_out0 : %d\n", pu[k].params.salb.bits_out0);
						vpu_info("\t\t\t" "signed_in0 : %d\n", pu[k].params.salb.signed_in0);
						vpu_info("\t\t\t" "signed_in1 : %d\n", pu[k].params.salb.signed_in1);
						vpu_info("\t\t\t" "signed_out0 : %d\n", pu[k].params.salb.signed_out0);
						vpu_info("\t\t\t" "input_enable : %d\n", pu[k].params.salb.input_enable);
						vpu_info("\t\t\t" "use_mask : %d\n", pu[k].params.salb.use_mask);
						vpu_info("\t\t\t" "operation_code : %d\n", pu[k].params.salb.operation_code);
						vpu_info("\t\t\t" "abs_neg : %d\n", pu[k].params.salb.abs_neg);
						vpu_info("\t\t\t" "trunc_out : %d\n", pu[k].params.salb.trunc_out);
						vpu_info("\t\t\t" "org_val_med : %d\n", pu[k].params.salb.org_val_med);
						vpu_info("\t\t\t" "shift_bits : %d\n", pu[k].params.salb.shift_bits);
						vpu_info("\t\t\t" "cmp_op : %d\n", pu[k].params.salb.cmp_op);
						vpu_info("\t\t\t" "const_in1 : %d\n", pu[k].params.salb.const_in1);
						vpu_info("\t\t\t" "thresh_lo : %d\n", pu[k].params.salb.thresh_lo);
						vpu_info("\t\t\t" "thresh_hi : %d\n", pu[k].params.salb.thresh_hi);
						vpu_info("\t\t\t" "val_lo : %d\n", pu[k].params.salb.val_lo);
						vpu_info("\t\t\t" "val_hi : %d\n", pu[k].params.salb.val_hi);
						vpu_info("\t\t\t" "val_med_filler : %d\n", pu[k].params.salb.val_med_filler);
						vpu_info("\t\t\t" "salbregs_custom_trunc_en : %d\n", pu[k].params.salb.salbregs_custom_trunc_en);
						vpu_info("\t\t\t" "salbregs_custom_trunc_bittage : %d\n", pu[k].params.salb.salbregs_custom_trunc_bittage);
						break;
					case VPUL_OP_FULL_CALB:
						vpu_info("\t\t\t" "bits_in0 : %d\n", pu[k].params.calb.bits_in0);
						vpu_info("\t\t\t" "bits_in1 : %d\n", pu[k].params.calb.bits_in1);
						vpu_info("\t\t\t" "bits_out0 : %d\n", pu[k].params.calb.bits_out0);
						vpu_info("\t\t\t" "signed_in0 : %d\n", pu[k].params.calb.signed_in0);
						vpu_info("\t\t\t" "signed_in1 : %d\n", pu[k].params.calb.signed_in1);
						vpu_info("\t\t\t" "signed_out0 : %d\n", pu[k].params.calb.signed_out0);
						vpu_info("\t\t\t" "input_enable : %d\n", pu[k].params.calb.input_enable);
						vpu_info("\t\t\t" "operation_code : %d\n", pu[k].params.calb.operation_code);
						vpu_info("\t\t\t" "abs_neg : %d\n", pu[k].params.calb.abs_neg);
						vpu_info("\t\t\t" "trunc_out : %d\n", pu[k].params.calb.trunc_out);
						vpu_info("\t\t\t" "org_val_med : %d\n", pu[k].params.calb.org_val_med);
						vpu_info("\t\t\t" "shift_bits : %d\n", pu[k].params.calb.shift_bits);
						vpu_info("\t\t\t" "mult_round : %d\n", pu[k].params.calb.mult_round);
						vpu_info("\t\t\t" "const_in1 : %d\n", pu[k].params.calb.const_in1);
						vpu_info("\t\t\t" "div_shift_bits : %d\n", pu[k].params.calb.div_shift_bits);
						vpu_info("\t\t\t" "div_overflow_remainder : %d\n", pu[k].params.calb.div_overflow_remainder);
						vpu_info("\t\t\t" "thresh_lo : %d\n", pu[k].params.calb.thresh_lo);
						vpu_info("\t\t\t" "thresh_hi : %d\n", pu[k].params.calb.thresh_hi);
						vpu_info("\t\t\t" "val_lo : %d\n", pu[k].params.calb.val_lo);
						vpu_info("\t\t\t" "val_hi : %d\n", pu[k].params.calb.val_hi);
						vpu_info("\t\t\t" "val_med_filler : %d\n", pu[k].params.calb.val_med_filler);
						break;
					case VPUL_OP_ROI:
						vpu_info("\t\t\t" "bits_in0 : %d\n", pu[k].params.rois_out.bits_in0);
						vpu_info("\t\t\t" "signed_in0 : %d\n", pu[k].params.rois_out.signed_in0);
						vpu_info("\t\t\t" "use_mask : %d\n", pu[k].params.rois_out.use_mask);
						vpu_info("\t\t\t" "work_mode : %d\n", pu[k].params.rois_out.work_mode);
						vpu_info("\t\t\t" "first_min_max : %d\n", pu[k].params.rois_out.first_min_max);
						vpu_info("\t\t\t" "thresh_lo_temp : %d\n", pu[k].params.rois_out.thresh_lo_temp);
						break;
					case VPUL_OP_CROP:
						vpu_info("\t\t\t" "work_mode : %d\n", pu[k].params.crop.work_mode);
						vpu_info("\t\t\t" "pad_value : %d\n", pu[k].params.crop.pad_value);
						vpu_info("\t\t\t" "mask_val_in : %d\n", pu[k].params.crop.mask_val_in);
						vpu_info("\t\t\t" "mask_val_out : %d\n", pu[k].params.crop.mask_val_out);
						vpu_info("\t\t\t" "roi_startx : %d\n", pu[k].params.crop.roi_startx);
						vpu_info("\t\t\t" "roi_starty : %d\n", pu[k].params.crop.roi_starty);
						break;
					case VPUL_OP_MDE:
						vpu_info("\t\t\t" "work_mode : %d\n", pu[k].params.mde.work_mode);
						vpu_info("\t\t\t" "result_shift : %d\n", pu[k].params.mde.result_shift);
						vpu_info("\t\t\t" "use_thresh : %d\n", pu[k].params.mde.use_thresh);
						vpu_info("\t\t\t" "calc_quantized_angle : %d\n", pu[k].params.mde.calc_quantized_angle);
						vpu_info("\t\t\t" "eig_coeff : %d\n", pu[k].params.mde.eig_coeff);
						vpu_info("\t\t\t" "bits_in : %d\n", pu[k].params.mde.bits_in);
						vpu_info("\t\t\t" "signed_in : %d\n", pu[k].params.mde.signed_in);
						vpu_info("\t\t\t" "bits_out : %d\n", pu[k].params.mde.bits_out);
						vpu_info("\t\t\t" "signed_out : %d\n", pu[k].params.mde.signed_out);
						vpu_info("\t\t\t" "output_enable : %d\n", pu[k].params.mde.output_enable);
						vpu_info("\t\t\t" "thresh : %d\n", pu[k].params.mde.thresh);
						break;
					case VPUL_OP_NMS:
						vpu_info("\t\t\t" "work_mode : %d\n", pu[k].params.nms.work_mode);
						vpu_info("\t\t\t" "keep_equals : %d\n", pu[k].params.nms.keep_equals);
						vpu_info("\t\t\t" "directional_nms : %d\n", pu[k].params.nms.directional_nms);
						vpu_info("\t\t\t" "census_mode : %d\n", pu[k].params.nms.census_mode);
						vpu_info("\t\t\t" "add_orig_pixel : %d\n", pu[k].params.nms.add_orig_pixel);
						vpu_info("\t\t\t" "bits_in : %d\n", pu[k].params.nms.bits_in);
						vpu_info("\t\t\t" "bits_out : %d\n", pu[k].params.nms.bits_out);
						vpu_info("\t\t\t" "signed_in : %d\n", pu[k].params.nms.signed_in);
						vpu_info("\t\t\t" "support : %d\n", pu[k].params.nms.support);
						vpu_info("\t\t\t" "org_val_out : %d\n", pu[k].params.nms.org_val_out);
						vpu_info("\t\t\t" "trunc_out : %d\n", pu[k].params.nms.trunc_out);
						vpu_info("\t\t\t" "image_height : %d\n", pu[k].params.nms.image_height);
						vpu_info("\t\t\t" "thresh : %d\n", pu[k].params.nms.thresh);
						vpu_info("\t\t\t" "border_mode_up : %d\n", pu[k].params.nms.border_mode_up);
						vpu_info("\t\t\t" "border_mode_down : %d\n", pu[k].params.nms.border_mode_down);
						vpu_info("\t\t\t" "border_mode_left : %d\n", pu[k].params.nms.border_mode_left);
						vpu_info("\t\t\t" "border_mode_right : %d\n", pu[k].params.nms.border_mode_right);
						vpu_info("\t\t\t" "border_fill : %d\n", pu[k].params.nms.border_fill);
						vpu_info("\t\t\t" "border_fill_constant : %d\n", pu[k].params.nms.border_fill_constant);
						vpu_info("\t\t\t" "strict_comparison_mask : %d\n", pu[k].params.nms.strict_comparison_mask);
						vpu_info("\t\t\t" "cens_thres_0 : %d\n", pu[k].params.nms.cens_thres_0);
						vpu_info("\t\t\t" "cens_thres_1 : %d\n", pu[k].params.nms.cens_thres_1);
						vpu_info("\t\t\t" "cens_thres_2 : %d\n", pu[k].params.nms.cens_thres_2);
						vpu_info("\t\t\t" "cens_thres_3 : %d\n", pu[k].params.nms.cens_thres_3);
						vpu_info("\t\t\t" "cens_thres_4 : %d\n", pu[k].params.nms.cens_thres_4);
						vpu_info("\t\t\t" "cens_thres_5 : %d\n", pu[k].params.nms.cens_thres_5);
						vpu_info("\t\t\t" "cens_thres_6 : %d\n", pu[k].params.nms.cens_thres_6);
						vpu_info("\t\t\t" "cens_thres_7 : %d\n", pu[k].params.nms.cens_thres_7);
						break;
					case VPUL_OP_CCM:
						vpu_info("\t\t\t" "signed_in : %d\n", pu[k].params.ccm.signed_in);
						vpu_info("\t\t\t" "signed_out : %d\n", pu[k].params.ccm.signed_out);
						vpu_info("\t\t\t" "output_enable : %d\n", pu[k].params.ccm.output_enable);
						vpu_info("\t\t\t" "input_enable : %d\n", pu[k].params.ccm.input_enable);
						vpu_info("\t\t\t" "coefficient_shift : %d\n", pu[k].params.ccm.coefficient_shift);
						vpu_info("\t\t\t" "coefficient_0 : %d\n", pu[k].params.ccm.coefficient_0);
						vpu_info("\t\t\t" "coefficient_1 : %d\n", pu[k].params.ccm.coefficient_1);
						vpu_info("\t\t\t" "coefficient_2 : %d\n", pu[k].params.ccm.coefficient_2);
						vpu_info("\t\t\t" "coefficient_3 : %d\n", pu[k].params.ccm.coefficient_3);
						vpu_info("\t\t\t" "coefficient_4 : %d\n", pu[k].params.ccm.coefficient_4);
						vpu_info("\t\t\t" "coefficient_5 : %d\n", pu[k].params.ccm.coefficient_5);
						vpu_info("\t\t\t" "coefficient_6 : %d\n", pu[k].params.ccm.coefficient_6);
						vpu_info("\t\t\t" "coefficient_7 : %d\n", pu[k].params.ccm.coefficient_7);
						vpu_info("\t\t\t" "coefficient_8 : %d\n", pu[k].params.ccm.coefficient_8);
						vpu_info("\t\t\t" "offset_0 : %d\n", pu[k].params.ccm.offset_0);
						vpu_info("\t\t\t" "offset_1 : %d\n", pu[k].params.ccm.offset_1);
						vpu_info("\t\t\t" "offset_2 : %d\n", pu[k].params.ccm.offset_2);
						break;
					case VPUL_OP_SEP_FLT:
						vpu_info("\t\t\t" "invert_columns : %d\n", pu[k].params.slf.invert_columns);
						vpu_info("\t\t\t" "upsample_mode : %d\n", pu[k].params.slf.upsample_mode);
						vpu_info("\t\t\t" "downsample_rows : %d\n", pu[k].params.slf.downsample_rows);
						vpu_info("\t\t\t" "downsample_cols : %d\n", pu[k].params.slf.downsample_cols);
						vpu_info("\t\t\t" "sampling_offset_x : %d\n", pu[k].params.slf.sampling_offset_x);
						vpu_info("\t\t\t" "sampling_offset_y : %d\n", pu[k].params.slf.sampling_offset_y);
						vpu_info("\t\t\t" "work_mode : %d\n", pu[k].params.slf.work_mode);
						vpu_info("\t\t\t" "filter_size_mode : %d\n", pu[k].params.slf.filter_size_mode);
						vpu_info("\t\t\t" "out_enable_1 : %d\n", pu[k].params.slf.out_enable_1);
						vpu_info("\t\t\t" "horizontal_only : %d\n", pu[k].params.slf.horizontal_only);
						vpu_info("\t\t\t" "bits_in : %d\n", pu[k].params.slf.bits_in);
						vpu_info("\t\t\t" "bits_out : %d\n", pu[k].params.slf.bits_out);
						vpu_info("\t\t\t" "signed_in : %d\n", pu[k].params.slf.signed_in);
						vpu_info("\t\t\t" "signed_out : %d\n", pu[k].params.slf.signed_out);
						vpu_info("\t\t\t" "do_rounding : %d\n", pu[k].params.slf.do_rounding);
						vpu_info("\t\t\t" "border_mode_up : %d\n", pu[k].params.slf.border_mode_up);
						vpu_info("\t\t\t" "border_mode_down : %d\n", pu[k].params.slf.border_mode_down);
						vpu_info("\t\t\t" "border_mode_left : %d\n", pu[k].params.slf.border_mode_left);
						vpu_info("\t\t\t" "border_mode_right : %d\n", pu[k].params.slf.border_mode_right);
						vpu_info("\t\t\t" "border_fill : %d\n", pu[k].params.slf.border_fill);
						vpu_info("\t\t\t" "border_fill_constant : %d\n", pu[k].params.slf.border_fill_constant);
						vpu_info("\t\t\t" "coefficient_fraction : %d\n", pu[k].params.slf.coefficient_fraction);
						vpu_info("\t\t\t" "sepfregs_is_max_pooling_mode : %d\n", pu[k].params.slf.sepfregs_is_max_pooling_mode);
						vpu_info("\t\t\t" "sepfregs_stride_value : %d\n", pu[k].params.slf.sepfregs_stride_value);
						vpu_info("\t\t\t" "sepfregs_stride_offset_height : %d\n", pu[k].params.slf.sepfregs_stride_offset_height);
						vpu_info("\t\t\t" "sepfregs_stride_offset_width : %d\n", pu[k].params.slf.sepfregs_stride_offset_width);
						vpu_info("\t\t\t" "sepfregs_subimage_height : %d\n", pu[k].params.slf.sepfregs_subimage_height);
						vpu_info("\t\t\t" "sepfregs_convert_16f_to_32f : %d\n", pu[k].params.slf.sepfregs_convert_16f_to_32f);
						vpu_info("\t\t\t" "sepfregs_convert_output_sm_to_2scomp : %d\n", pu[k].params.slf.sepfregs_convert_output_sm_to_2scomp);
						vpu_info("\t\t\t" "sepfregs_convert_input_2scomp_to_sm : %d\n", pu[k].params.slf.sepfregs_convert_input_2scomp_to_sm);
						vpu_info("\t\t\t" "maxp_num_slices : %d\n", pu[k].params.slf.maxp_num_slices);
						vpu_info("\t\t\t" "maxp_sizes_filt_hor : %d\n", pu[k].params.slf.maxp_sizes_filt_hor);
						vpu_info("\t\t\t" "maxp_sizes_filt_ver : %d\n", pu[k].params.slf.maxp_sizes_filt_ver);
						vpu_info("\t\t\t" "coefficient_index : %d\n", pu[k].params.slf.coefficient_index);
						break;
					case VPUL_OP_GEN_FLT:
						vpu_info("\t\t\t" "filter_size_mode : %d\n", pu[k].params.glf.filter_size_mode);
						vpu_info("\t\t\t" "sad_mode : %d\n", pu[k].params.glf.sad_mode);
						vpu_info("\t\t\t" "out_enable_2 : %d\n", pu[k].params.glf.out_enable_2);
						vpu_info("\t\t\t" "two_outputs : %d\n", pu[k].params.glf.two_outputs);
						vpu_info("\t\t\t" "input_enable1 : %d\n", pu[k].params.glf.input_enable1);
						vpu_info("\t\t\t" "bits_in : %d\n", pu[k].params.glf.bits_in);
						vpu_info("\t\t\t" "bits_out : %d\n", pu[k].params.glf.bits_out);
						vpu_info("\t\t\t" "signed_in : %d\n", pu[k].params.glf.signed_in);
						vpu_info("\t\t\t" "signed_out : %d\n", pu[k].params.glf.signed_out);
						vpu_info("\t\t\t" "do_rounding : %d\n", pu[k].params.glf.do_rounding);
						vpu_info("\t\t\t" "RAM_type : %d\n", pu[k].params.glf.RAM_type);
						vpu_info("\t\t\t" "RAM_offset : %d\n", pu[k].params.glf.RAM_offset);
						vpu_info("\t\t\t" "image_height : %d\n", pu[k].params.glf.image_height);
						vpu_info("\t\t\t" "border_mode_up : %d\n", pu[k].params.glf.border_mode_up);
						vpu_info("\t\t\t" "border_mode_down : %d\n", pu[k].params.glf.border_mode_down);
						vpu_info("\t\t\t" "border_mode_left : %d\n", pu[k].params.glf.border_mode_left);
						vpu_info("\t\t\t" "border_mode_right : %d\n", pu[k].params.glf.border_mode_right);
						vpu_info("\t\t\t" "border_fill : %d\n", pu[k].params.glf.border_fill);
						vpu_info("\t\t\t" "border_fill_constant : %d\n", pu[k].params.glf.border_fill_constant);
						vpu_info("\t\t\t" "coefficient_fraction : %d\n", pu[k].params.glf.coefficient_fraction);
						vpu_info("\t\t\t" "signed_coefficients : %d\n", pu[k].params.glf.signed_coefficients);
						vpu_info("\t\t\t" "coefficient_index : %d\n", pu[k].params.glf.coefficient_index);
						vpu_info("\t\t\t" "coeffs_from_dma : %d\n", pu[k].params.glf.coeffs_from_dma);
						break;
					case VPUL_OP_NLF_FLT:
						vpu_info("\t\t\t" "filter_mode : %d\n", pu[k].params.nlf.filter_mode);
						vpu_info("\t\t\t" "fast_score_direction : %d\n", pu[k].params.nlf.fast_score_direction);
						vpu_info("\t\t\t" "border_mode_up : %d\n", pu[k].params.nlf.border_mode_up);
						vpu_info("\t\t\t" "border_mode_left: %d\n", pu[k].params.nlf.border_mode_left);
						vpu_info("\t\t\t" "border_mode_right : %d\n", pu[k].params.nlf.border_mode_right);
						vpu_info("\t\t\t" "border_fill : %d\n", pu[k].params.nlf.border_fill);
						vpu_info("\t\t\t" "border_tile : %d\n", pu[k].params.nlf.border_tile);
						vpu_info("\t\t\t" "border_fill_constant : %d\n", pu[k].params.nlf.border_fill_constant);
						vpu_info("\t\t\t" "bits_in : %d\n", pu[k].params.nlf.bits_in);
						vpu_info("\t\t\t" "signed_in : %d\n", pu[k].params.nlf.signed_in);
						vpu_info("\t\t\t" "census_mode : %d\n", pu[k].params.nlf.census_mode);
						vpu_info("\t\t\t" "census_out_image : %d\n", pu[k].params.nlf.census_out_image);
						vpu_info("\t\t\t" "add_orig_pixel : %d\n", pu[k].params.nlf.add_orig_pixel);
						vpu_info("\t\t\t" "cens_thres_0 : %d\n", pu[k].params.nlf.cens_thres_0);
						vpu_info("\t\t\t" "cens_thres_1 : %d\n", pu[k].params.nlf.cens_thres_1);
						vpu_info("\t\t\t" "cens_thres_2 : %d\n", pu[k].params.nlf.cens_thres_2);
						vpu_info("\t\t\t" "cens_thres_3 : %d\n", pu[k].params.nlf.cens_thres_3);
						vpu_info("\t\t\t" "cens_thres_4 : %d\n", pu[k].params.nlf.cens_thres_4);
						vpu_info("\t\t\t" "cens_thres_5 : %d\n", pu[k].params.nlf.cens_thres_5);
						vpu_info("\t\t\t" "cens_thres_6 : %d\n", pu[k].params.nlf.cens_thres_6);
						vpu_info("\t\t\t" "cens_thres_7 : %d\n", pu[k].params.nlf.cens_thres_7);
						vpu_info("\t\t\t" "neighbors_mask : %d\n", pu[k].params.nlf.neighbors_mask);
						break;
					case VPUL_OP_UPSCALER:
						vpu_info("\t\t\t" "interpolation_method : %d\n", pu[k].params.upscaler.interpolation_method);
						vpu_info("\t\t\t" "border_fill_mode : %d\n", pu[k].params.upscaler.border_fill_mode);
						vpu_info("\t\t\t" "do_rounding : %d\n", pu[k].params.upscaler.do_rounding);
						vpu_info("\t\t\t" "border_fill_constant : %d\n", pu[k].params.upscaler.border_fill_constant);
						break;
					case VPUL_OP_DOWNSCALER:
						vpu_info("\t\t\t" "interpolation_method : %d\n", pu[k].params.downScaler.interpolation_method);
						vpu_info("\t\t\t" "do_rounding : %d\n", pu[k].params.downScaler.do_rounding);
						break;
					case VPUL_OP_DUPLICATE:
						break;
					case VPUL_OP_SPLIT:
						vpu_info("\t\t\t" "out0_byte0 : %d\n", pu[k].params.spliter.out0_byte0);
						vpu_info("\t\t\t" "out0_byte1 : %d\n", pu[k].params.spliter.out0_byte1);
						vpu_info("\t\t\t" "out1_byte0 : %d\n", pu[k].params.spliter.out1_byte0);
						vpu_info("\t\t\t" "out1_byte1 : %d\n", pu[k].params.spliter.out1_byte1);
						vpu_info("\t\t\t" "out2_byte0 : %d\n", pu[k].params.spliter.out2_byte0);
						vpu_info("\t\t\t" "out2_byte1 : %d\n", pu[k].params.spliter.out2_byte1);
						vpu_info("\t\t\t" "out3_byte0 : %d\n", pu[k].params.spliter.out3_byte0);
						vpu_info("\t\t\t" "out3_byte1 : %d\n", pu[k].params.spliter.out3_byte1);
						break;
					case VPUL_OP_JOIN:
						vpu_info("\t\t\t" "out_byte0_source_stream : %d\n", pu[k].params.joiner.out_byte0_source_stream);
						vpu_info("\t\t\t" "out_byte1_source_stream : %d\n", pu[k].params.joiner.out_byte1_source_stream);
						vpu_info("\t\t\t" "out_byte2_source_stream : %d\n", pu[k].params.joiner.out_byte2_source_stream);
						vpu_info("\t\t\t" "out_byte3_source_stream : %d\n", pu[k].params.joiner.out_byte3_source_stream);
						vpu_info("\t\t\t" "input0_enable : %d\n", pu[k].params.joiner.input0_enable);
						vpu_info("\t\t\t" "input1_enable : %d\n", pu[k].params.joiner.input1_enable);
						vpu_info("\t\t\t" "input2_enable : %d\n", pu[k].params.joiner.input2_enable);
						vpu_info("\t\t\t" "input3_enable : %d\n", pu[k].params.joiner.input3_enable);
						vpu_info("\t\t\t" "work_mode : %d\n", pu[k].params.joiner.work_mode);
						break;
					case VPUL_OP_INTEGRAL_IMG:
						vpu_info("\t\t\t" "integral_image_mode : %d\n", pu[k].params.integral.integral_image_mode);
						vpu_info("\t\t\t" "overflow_mode : %d\n", pu[k].params.integral.overflow_mode);
						vpu_info("\t\t\t" "dt_right_shift : %d\n", pu[k].params.integral.dt_right_shift);
						vpu_info("\t\t\t" "dt_left_shift : %d\n", pu[k].params.integral.dt_left_shift);
						vpu_info("\t\t\t" "dt_coefficient0 : %d\n", pu[k].params.integral.dt_coefficient0);
						vpu_info("\t\t\t" "dt_coefficient1 : %d\n", pu[k].params.integral.dt_coefficient1);
						vpu_info("\t\t\t" "dt_coefficient2 : %d\n", pu[k].params.integral.dt_coefficient2);
						vpu_info("\t\t\t" "cc_min_label : %d\n", pu[k].params.integral.cc_min_label);
						vpu_info("\t\t\t" "cc_scan_mode : %d\n", pu[k].params.integral.cc_scan_mode);
						vpu_info("\t\t\t" "cc_smart_label_search_en : %d\n", pu[k].params.integral.cc_smart_label_search_en);
						vpu_info("\t\t\t" "cc_reset_labels_array : %d\n", pu[k].params.integral.cc_reset_labels_array);
						vpu_info("\t\t\t" "cc_label_vector_size : %d\n", pu[k].params.integral.cc_label_vector_size);
						vpu_info("\t\t\t" "lut_init_en : %d\n", pu[k].params.integral.lut_init_en);
						vpu_info("\t\t\t" "lut_number_of_values : %d\n", pu[k].params.integral.lut_number_of_values);
						vpu_info("\t\t\t" "lut_value_shift : %d\n", pu[k].params.integral.lut_value_shift);
						vpu_info("\t\t\t" "lut_default_overflow : %d\n", pu[k].params.integral.lut_default_overflow);
						vpu_info("\t\t\t" "lut_default_underflow : %d\n", pu[k].params.integral.lut_default_underflow);
						break;
					case VPUL_OP_MAP_2_LST:
						vpu_info("\t\t\t" "bits_in : %d\n", pu[k].params.map2list.bits_in);
						vpu_info("\t\t\t" "signed_in : %d\n", pu[k].params.map2list.signed_in);
						vpu_info("\t\t\t" "value_in : %d\n", pu[k].params.map2list.value_in);
						vpu_info("\t\t\t" "enable_out_map : %d\n", pu[k].params.map2list.enable_out_map);
						vpu_info("\t\t\t" "enable_out_lst : %d\n", pu[k].params.map2list.enable_out_lst);
						vpu_info("\t\t\t" "inout_indx : %d\n", pu[k].params.map2list.inout_indx);
						vpu_info("\t\t\t" "threshold_low : %d\n", pu[k].params.map2list.threshold_low);
						vpu_info("\t\t\t" "threshold_high : %d\n", pu[k].params.map2list.threshold_high);
						vpu_info("\t\t\t" "num_of_point : %d\n", pu[k].params.map2list.num_of_point);
						break;
					case VPUL_OP_FIFO:
						break;
					case VPUL_OP_CNN:
						break;
					case VPUL_OP_LUT:
						vpu_info("\t\t\t" "interpolation_mode : %d\n", pu[k].params.lut.interpolation_mode);
						vpu_info("\t\t\t" "lut_size : %d\n", pu[k].params.lut.lut_size);
						vpu_info("\t\t\t" "signed_in0 : %d\n", pu[k].params.lut.signed_in0);
						vpu_info("\t\t\t" "signed_out0 : %d\n", pu[k].params.lut.signed_out0);
						vpu_info("\t\t\t" "offset : %d\n", pu[k].params.lut.offset);
						vpu_info("\t\t\t" "binsize : %d\n", pu[k].params.lut.binsize);
						vpu_info("\t\t\t" "inverse_binsize : %d\n", pu[k].params.lut.inverse_binsize);
						break;
					case VPUL_OP_HIST:
						vpu_info("\t\t\t" "offset : %d\n", pu[k].params.histogram.offset);
						vpu_info("\t\t\t" "inverse_binsize : %d\n", pu[k].params.histogram.inverse_binsize);
						vpu_info("\t\t\t" "variable_increment : %d\n", pu[k].params.histogram.variable_increment);
						vpu_info("\t\t\t" "dual_histogram : %d\n", pu[k].params.histogram.dual_histogram);
						vpu_info("\t\t\t" "signed_in0 : %d\n", pu[k].params.histogram.signed_in0);
						vpu_info("\t\t\t" "round_index : %d\n", pu[k].params.histogram.round_index);
						vpu_info("\t\t\t" "max_val : %d\n", pu[k].params.histogram.max_val);
						break;
					default:
						break;
					}
				}
			} else if (subchain[j].stype == VPUL_SUB_CH_CPU_OP) {
				for (k = 0; k < MAX_SUPPORT_CPU_OPER_NUM; ++k) {
					vpu_info("\t\t\t" "[CPU:%d]\n", k);
					vpu_info("\t\t\t" "opcode : %d\n", subchain[j].cpu.cpu_op_desc[k].opcode);
				}
			}
		}
	}
}

void vpu_graph_S0(struct client *client)
{
	int ret, task_size;
	struct vpul_task *task;
	struct vs4l_graph *graph;
	char *ptr;

	task = make_example0_task(&task_size);
	if (!task) {
		printf("task is NULL\n");
		return;
	}

	task->id = client->id;

	ptr = (char *)task;
	ptr[task_size - 4] = 'V';
	ptr[task_size - 3] = 'S';
	ptr[task_size - 2] = '4';
	ptr[task_size - 1] = 'L';

	client->pu_array = (struct vpul_pu *)(ptr + task->pus_vec_ofs);
	graph = &client->graph;
	graph->id = client->id;
	graph->addr = (unsigned long)ptr;
	graph->size = task_size;

	ret = client_s_ginfo(client, task);
	if (ret)
		printf("client_s_ginfo is fail(%d)\n", ret);

	ret = client_s_format(client);
	if (ret)
		printf("client_s_format is fail(%d)\n", ret);

	ret = client_s_buffer(client);
	if (ret)
		printf("client_s_buffer is fail(%d)\n", ret);

	ret = client_s_intermediate(client);
	if (ret)
		printf("client_s_intermediate is fail(%d)\n", ret);
}

void vpu_graph_S1(struct client *client)
{
	int ret, task_size;
	struct vpul_task *task;
	struct vs4l_graph *graph;
	char *ptr;

	task = make_example1_task(&task_size);
	if (!task) {
		printf("task is NULL\n");
		return;
	}

	ptr = (char *)task;
	ptr[task_size - 4] = 'V';
	ptr[task_size - 3] = 'S';
	ptr[task_size - 2] = '4';
	ptr[task_size - 1] = 'L';

	client->pu_array = (struct vpul_pu *)(ptr + task->pus_vec_ofs);
	graph = &client->graph;
	graph->id = client->id;
	graph->addr = (unsigned long)ptr;
	graph->size = task_size;

	ret = client_s_ginfo(client, task);
	if (ret)
		printf("client_s_ginfo is fail(%d)\n", ret);

	ret = client_s_format(client);
	if (ret)
		printf("client_s_format is fail(%d)\n", ret);

	ret = client_s_buffer(client);
	if (ret)
		printf("client_s_buffer is fail(%d)\n", ret);

	ret = client_s_intermediate(client);
	if (ret)
		printf("client_s_intermediate is fail(%d)\n", ret);
}

void vpu_graph_S2(struct client *client)
{
	int ret, task_size;
	struct vpul_task *task;
	struct vs4l_graph *graph;
	char *ptr;

	task = make_example2_task(&task_size);
	if (!task) {
		printf("task is NULL\n");
		return;
	}

	ptr = (char *)task;
	ptr[task_size - 4] = 'V';
	ptr[task_size - 3] = 'S';
	ptr[task_size - 2] = '4';
	ptr[task_size - 1] = 'L';

	client->pu_array = (struct vpul_pu *)(ptr + task->pus_vec_ofs);
	graph = &client->graph;
	graph->id = client->id;
	graph->addr = (unsigned long)ptr;
	graph->size = task_size;

	ret = client_s_ginfo(client, task);
	if (ret)
		printf("client_s_ginfo is fail(%d)\n", ret);

	ret = client_s_format(client);
	if (ret)
		printf("client_s_format is fail(%d)\n", ret);

	ret = client_s_buffer(client);
	if (ret)
		printf("client_s_buffer is fail(%d)\n", ret);

	ret = client_s_intermediate(client);
	if (ret)
		printf("client_s_intermediate is fail(%d)\n", ret);
}

void check_buffer(struct vs4l_container_list *clist)
{
	int i, j;
	struct vs4l_container *c;
	struct vs4l_buffer *b;

	printf("**************************************\n");

	printf("clist : %p\n", clist);
	printf("direction : %d\n", clist->direction);
	printf("index : %d\n", clist->index);
	printf("id : %d\n", clist->id);
	printf("count: %d\n", clist->count);

	c = clist->containers;
	for (i = 0; i < clist->count; ++i) {
		printf("\ncontainer : %p\n", &c[i]);
		printf("target : %d\n", c[i].target);
		printf("count : %d\n", c[i].count);

		b = c[i].buffers;
		for (j = 0; j < c[i].count; ++j) {
			printf("\nbuffer : %p\n", &b[i]);
			printf("fd : %d\n", b[i].m.fd);
		}
	}
}

void * test_S0(void *user_data)
{
	struct client *user = user_data;
	struct vs4l_container_list inresult, otresult;
	struct vs4l_container_list *inclist;
	struct vs4l_container_list *otclist;
	int index;

	inresult.direction = VS4L_DIRECTION_IN;
	inresult.count = 0;
	inresult.containers = NULL;
	otresult.direction = VS4L_DIRECTION_OT;
	otresult.count = 0;
	otresult.containers = NULL;

	vpu_graph_S0(user);

	inclist = user->incfg.clist;
	otclist = user->otcfg.clist;

	vpu_open(user);
	vpu_s_graph(user, &user->graph);
	vpu_s_format(user, &user->incfg.flist);
	vpu_s_format(user, &user->otcfg.flist);
	vpu_stream_on(user);

	while (user->str_loop < user->end_loop) {
		index = user->str_loop % user->buffer_cnt;
		inclist[index].id = user->str_loop;
		otclist[index].id = user->str_loop;

		vpu_qbuf(user, &inclist[index]);
		vpu_qbuf(user, &otclist[index]);

		vpu_dqbuf(user, &inresult);
		vpu_dqbuf(user, &otresult);

		user->str_loop++;
	}

	vpu_stream_off(user);
	vpu_close(user);
	client_p_buffer(user);
	return user;
}

void * test_S1(void *user_data)
{
	struct client *user = user_data;
	struct vs4l_container_list inresult, otresult;
	struct vs4l_container_list *inclist;
	struct vs4l_container_list *otclist;
	int index;

	inresult.direction = VS4L_DIRECTION_IN;
	inresult.count = 0;
	inresult.containers = NULL;
	otresult.direction = VS4L_DIRECTION_OT;
	otresult.count = 0;
	otresult.containers = NULL;

	vpu_graph_S1(user);

	inclist = user->incfg.clist;
	otclist = user->otcfg.clist;

	vpu_open(user);
	vpu_s_graph(user, &user->graph);
	vpu_s_format(user, &user->incfg.flist);
	vpu_s_format(user, &user->otcfg.flist);
	vpu_stream_on(user);

	while (user->str_loop < user->end_loop) {
		index = user->str_loop % user->buffer_cnt;
		inclist[index].id = user->str_loop;
		otclist[index].id = user->str_loop;

		vpu_qbuf(user, &inclist[index]);
		vpu_qbuf(user, &otclist[index]);

		vpu_dqbuf(user, &inresult);
		vpu_dqbuf(user, &otresult);

		user->str_loop++;
	}

	vpu_stream_off(user);
	vpu_close(user);
	client_p_buffer(user);
	return user;
}

void * test_S2(void *user_data)
{
	struct client *user = user_data;
	struct vs4l_container_list inresult, otresult;
	struct vs4l_container_list *inclist;
	struct vs4l_container_list *otclist;
	int index;

	inresult.direction = VS4L_DIRECTION_IN;
	inresult.count = 0;
	inresult.containers = NULL;
	otresult.direction = VS4L_DIRECTION_OT;
	otresult.count = 0;
	otresult.containers = NULL;

	vpu_graph_S2(user);

	inclist = user->incfg.clist;
	otclist = user->otcfg.clist;

	vpu_open(user);
	vpu_s_graph(user, &user->graph);
	vpu_s_format(user, &user->incfg.flist);
	vpu_s_format(user, &user->otcfg.flist);
	vpu_stream_on(user);

	while (user->str_loop < user->end_loop) {
		index = user->str_loop % user->buffer_cnt;
		inclist[index].id = user->str_loop;
		otclist[index].id = user->str_loop;

		vpu_qbuf(user, &inclist[index]);
		vpu_qbuf(user, &otclist[index]);

		vpu_dqbuf(user, &inresult);
		vpu_dqbuf(user, &otresult);

		user->str_loop++;
	}

	vpu_stream_off(user);
	vpu_close(user);
	client_p_buffer(user);
	return user;
}

/*
 * scenario name : graph S0 normal
 */
int test_S0_case(int thread_num)
{
	int ret = 0, i;
	struct client *user;
	ion_client iclient;

	user = malloc(sizeof(struct client) * thread_num);

	iclient = ion_client_create();
	if (iclient < 0) {
		printf("failed to open ion node\n");
		ret = -EINVAL;
		return ret;
	}

	for (i = 0; i < thread_num; ++i) {
		user[i].iclient = iclient;
		user[i].id = 0;
		user[i].graph.priority = 10;
		user[i].graph.flags = 0;
		user[i].str_loop = 0;
		user[i].end_loop = 20;
		user[i].buffer_cnt = 5;
	}

	for (i = 0; i < thread_num; ++i) {
		ret = pthread_create(&user[i].thread, NULL, test_S0, (void *)&user[i]);
		if (ret) {
			printf("pthread_create is fail(%d)\n", ret);
			abort();
		}

		printf("%s(%d) start\n", __func__, i);
	}

	for (i = 0; i < thread_num; ++i) {
		ret = pthread_join(user[i].thread, NULL);
		if (ret) {
			printf("thread join fail\n");
			abort();
		}

		printf("%s(%d) end\n", __func__, i);
	}

	ion_client_destroy(iclient);
	free(user);

	return ret;
}

/*
 * scenario name : graph S1 normal
 */
int test_S1_case(int thread_num)
{
	int ret = 0, i;
	struct client *user;
	ion_client iclient;

	user = malloc(sizeof(struct client) * thread_num);

	iclient = ion_client_create();
	if (iclient < 0) {
		printf("failed to open ion node\n");
		ret = -EINVAL;
		return ret;
	}

	for (i = 0; i < thread_num; ++i) {
		user[i].iclient = iclient;
		user[i].id = 0;
		user[i].graph.priority = 10;
		user[i].graph.flags = 0;
		user[i].str_loop = 0;
		user[i].end_loop = 20;
		user[i].buffer_cnt = 5;
	}

	for (i = 0; i < thread_num; ++i) {
		ret = pthread_create(&user[i].thread, NULL, test_S1, (void *)&user[i]);
		if (ret) {
			printf("pthread_create is fail(%d)\n", ret);
			abort();
		}

		printf("%s(%d) start\n", __func__, i);
	}

	for (i = 0; i < thread_num; ++i) {
		ret = pthread_join(user[i].thread, NULL);
		if (ret) {
			printf("thread join fail\n");
			abort();
		}

		printf("%s(%d) end\n", __func__, i);
	}

	ion_client_destroy(iclient);
	free(user);

	return ret;
}

/*
 * scenario name : graph S2 normal
 */
int test_S2_case(int thread_num)
{
	int ret = 0, i;
	struct client *user;
	ion_client iclient;

	user = malloc(sizeof(struct client) * thread_num);

	iclient = ion_client_create();
	if (iclient < 0) {
		printf("failed to open ion node\n");
		ret = -EINVAL;
		return ret;
	}

	for (i = 0; i < thread_num; ++i) {
		user[i].iclient = iclient;
		user[i].id = 0;
		user[i].graph.priority = 10;
		user[i].graph.flags = 0;
		user[i].str_loop = 0;
		user[i].end_loop = 20;
		user[i].buffer_cnt = 5;
	}

	for (i = 0; i < thread_num; ++i) {
		ret = pthread_create(&user[i].thread, NULL, test_S2, (void *)&user[i]);
		if (ret) {
			printf("pthread_create is fail(%d)\n", ret);
			abort();
		}

		printf("%s(%d) start\n", __func__, i);
	}

	for (i = 0; i < thread_num; ++i) {
		ret = pthread_join(user[i].thread, NULL);
		if (ret) {
			printf("thread join fail\n");
			abort();
		}

		printf("%s(%d) end\n", __func__, i);
	}

	ion_client_destroy(iclient);
	free(user);

	return ret;
}

int test_verify(struct vpu_test_case *test_case, int repeat)
{
	int ret = 0;
	struct client user;
	ion_client iclient;

	iclient = ion_client_create();
	if (iclient < 0) {
		printf("failed to open ion node\n");
		return -EINVAL;
	}

	user.iclient = iclient;
	user.id = test_case->id;
	user.graph.flags = test_case->flags;
	user.graph.priority = 10;
	user.graph.id = test_case->id;
	user.str_loop = 0;
	user.end_loop = repeat;
	user.buffer_cnt = 3;

	ret = (int)test_case->func(&user);

	ion_client_destroy(iclient);

	return ret;
}

int test_unit(struct vpu_test_case *test_case)
{
	int ret = 0;

	ret = (int)test_case->func(test_case);

	return ret;
}

int test_vector(int vector_id)
{
	int ret = 0;
	struct client user;
	struct vs4l_ctrl ctrl;

	ctrl.ctrl = VPU_CTRL_MODE;
	ctrl.value = 1;

	vpu_open(&user);
	vpu_s_ctrl(&user, &ctrl);
	vpu_close(&user);

	ctrl.ctrl = VPU_CTRL_TEST;
	ctrl.value = vector_id;

	vpu_open(&user);
	ret = vpu_s_ctrl(&user, &ctrl);
	vpu_close(&user);

	ctrl.ctrl = VPU_CTRL_MODE;
	ctrl.value = 0;

	vpu_open(&user);
	vpu_s_ctrl(&user, &ctrl);
	vpu_close(&user);

	return ret;
}

int main(int argc, char **argv)
{
	int ret = 0;
	int kind, select, repeat, count, i;
	struct vpu_test_set verify_set;
	struct vpu_test_set unit_set;
	int test_cnt, pass_cnt;

	vpu_test_verify_init(&verify_set);
	vpu_test_unit_init(&unit_set);

	printf("=========================== VPU =======================================\n");
	kind = atoi(argv[1]);
	select = atoi(argv[2]);
	repeat = 10;
	switch (kind) {
	case 0:
		printf("0. Scenario 0 normal\n");
		printf("1. Scenario 1 normal\n");
		printf("2. Scenario 2 normal\n");
		printf("select : %d\n", select);
		switch (select) {
		case 0:
			test_S0_case(1);
			break;
		case 1:
			test_S1_case(1);
			break;
		case 2:
			test_S2_case(1);
			break;
		}
		break;
	case 1:
		if (select >= VPU_MAX_TEST_CASES) {
			printf("select is invalid(%d)\n", select);
			break;
		}

		if (verify_set.tc[select].func == NULL) {
			printf("select is not registered(%d)\n", select);
			break;
		}

		printf("VERIFICATION TEST\n");
		printf("TEST NAME : %s\n", verify_set.tc[select].name);
		ret = test_verify(&verify_set.tc[select], 10);
		if (ret) printf("STATUS : FAIL(%d)\n", ret);
		else printf("STATUS: PASS\n");
		break;
	case 2:
		if (select >= VPU_MAX_TEST_CASES) {
			printf("select is invalid(%d)\n", select);
			break;
		}

		if (unit_set.tc[select].func == NULL) {
			printf("select is not registered(%d)\n", select);
			break;
		}

		printf("UNIT TEST\n");
		printf("TEST NAME : %s\n", unit_set.tc[select].name);
		ret = test_unit(&unit_set.tc[select]);
		if (ret) printf("STATUS : FAIL(%d)\n", ret);
		else printf("STATUS: PASS\n");
		break;
	case 3:
		ret = test_vector(select);
		if (ret) printf("STATUS : FAIL(%d)\n", ret);
		else printf("STATUS: PASS\n");
		break;
	default:
		switch (select) {
		case 0:
			count = 0;
			printf("UNIT TEST\n");
			for (i = 0; i < VPU_MAX_TEST_CASES; ++i) {
				if (unit_set.tc[i].func == NULL)
					continue;

				printf("TEST NAME : %s\n", unit_set.tc[i].name);
				ret = (int)unit_set.tc[select].func(NULL);
				if (ret) printf("STATUS : FAIL(%d)\n", ret);
				else printf("STATUS: PASS\n");
			}
			break;
		case 1:
			test_cnt = 0;
			pass_cnt = 0;
			loglevel = 0;
			printf("VERIFICATION TEST\n");
			for (i = 0; i < VPU_MAX_TEST_CASES; ++i) {
				if (verify_set.tc[i].func == NULL)
					continue;

				if ((i == 19) || (i == 24) || (i == 43) || (i == 44) || (i >= 50))
					continue;

				test_cnt++;

				printf("TEST NAME : %s\n", verify_set.tc[i].name);
				ret = test_verify(&verify_set.tc[i], repeat);
				if (ret) {
					printf("STATUS : FAIL(%d)\n", ret);
				} else {
					printf("STATUS: PASS\n");
					pass_cnt++;
				}
			}

			ret = test_cnt - pass_cnt;
			printf("RESULT : %d\n", ret);
			break;
		case 2:
			test_cnt = 0;
			pass_cnt = 0;
			loglevel = 0;
			repeat = 1;
			printf("ASB TEST\n");
			for (i = 0; i < VPU_MAX_TEST_CASES; ++i) {
				if (verify_set.tc[i].func == NULL)
					continue;

				if (!((i == 21) || (i == 23) || (i == 42)))
					continue;

				test_cnt++;

				printf("TEST NAME : %s\n", verify_set.tc[i].name);
				ret = test_verify(&verify_set.tc[i], repeat);
				if (ret) {
					printf("STATUS : FAIL(%d)\n", ret);
				} else {
					printf("STATUS: PASS\n");
					pass_cnt++;
				}
			}

			ret = test_cnt - pass_cnt;
			printf("RESULT : %d\n", ret);
			break;
		}
	}
	printf("=======================================================================\n");

	return ret;
}
