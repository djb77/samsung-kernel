/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#if !defined(__VPU_PROCESSING_UNIT_CONFIGURATION__)
#define __VPU_PROCESSING_UNIT_CONFIGURATION__

/* VPU 2.1 TODO: move definition to high level file */
#define VPU_2v1

/** \addtogroup VPUL-DS
 *  @{
 */
/** \addtogroup PU Processing units
 *  \brief Structures used to to define PU functionality, resources and parameters
 *  @{
 */

#define VPU_MAXIMAL_MPRB_CONNECTED	43
/* Maximal PUs connected as inputs to one PU */
#define VPUL_PU_MAX_PORTS		5


enum vpul_logical_op_types {
#define ENTRY(a) a,
#include "vpul_pu_opcodes.def"
#undef ENTRY
	NUMBER_OF_OP_TYPE
};


struct pu_ram_port_range {
	__u32	first_ram_port;
	__u32	number_of_ram_ports;
};

enum vpul_pu_instance {
#define VPU_PU_INSTANCE(a, b, c, d, e, f, g, h, i) a,
#include "vpul_pu_instances.def"
#undef VPU_PU_INSTANCE
	VPU_PU_NUMBER
};

/** \enum VPU_PU_TYPES
 * \brief Different types of PUs as HwMapper knows them. Defined using X
 *        macros based on vpul_pu_types.def.
 */
enum VPU_PU_TYPES {
#define VPU_PU_TYPE(a, b, c) a, \

#include "vpul_pu_types.def"
#undef VPU_PU_TYPE
	VPU_PU_TYPES_NUMBER,
	END_OF_PU_INST2TYPE_TRANSLATOR = 0XFFFFFFFF
};


enum vpul_mprb_acc_type {
	VPU_MPRB_ACC_UPDATABLE_PU    = 0,
	VPU_MPRB_ACC_NONUPDATABLE_PU = 1,
	VPU_MPRB_ACC_COUNT
};

/*! \brief DMA parameters */
struct vpul_pu_dma {
	/*! \brief The index of the current IO type description that includes
	 * the start address and the line offset
	 */
	__u32 inout_index;
	/*! 0 = Address is incremented between each 2 lines
	 * 1 = Address is decremented between each 2 lines
	 */
	__u32 offset_lines_inc;

	/*! \brief use to copy data from internal_mem to ext */
	__u32 is_output_vector_dma;
};

/*!
 * \ brief SALB parameters - fully described in user guide
 */
struct vpul_pu_salb {
	__u32 bits_in0			:1;
	__u32 bits_in1			:1;
	__u32 bits_out0			:1;
	__u32 signed_in0		:1;
	__u32 signed_in1		:1;
	__u32 signed_out0		:1;
	__u32 input_enable		:3;
	__u32 use_mask			:1;
	__u32 operation_code		:5;
	__u32 abs_neg			:2;
	__u32 trunc_out			:2;
	__u32 org_val_med		:1;
	__u32 shift_bits		:4;
	__u32 cmp_op			:3;
	__u32 const_in1			:1;
	__u32 thresh_lo			:16;
	__u32 thresh_hi			:16;
	__u32 val_lo			:16;
	__u32 val_hi			:16;
	__u32 val_med_filler		:16;
	__u32 salbregs_custom_trunc_en		:1;
	__u32 salbregs_custom_trunc_bittage	:5;
};

/*!
 * \ brief CALB parameters - fully described in user guide
 */
struct vpul_pu_calb {
	__u32 bits_in0			  :2;
	__u32 bits_in1                    :2;
	__u32 bits_out0                   :2;
	__u32 signed_in0                  :1;
	__u32 signed_in1                  :1;
	__u32 signed_out0                 :1;
	__u32 input_enable                :3;
	__u32 use_mask                    :1;
	__u32 operation_code              :5;
	__u32 abs_neg                     :2;
	__u32 trunc_out                   :2;
	__u32 org_val_med                 :1;
	__u32 shift_bits                  :6;
	__u32 mult_round                  :3;
	__u32 cmp_op                      :3;
	__u32 const_in1                   :1;
	__u32 div_shift_bits              :5;
	__u32 div_overflow_remainder      :1;
	__u32 thresh_lo;
	__u32 thresh_hi;
	__u32 val_lo;
	__u32 val_hi;
	__u32 val_med_filler;
};


/*! \brief Add parameters (Adding two inputs) */
struct vpul_pu_add {
	/*!  input/output bits: 0 = 8 bits. 1 = 16 bits. */
	__u32 bits;
	/*!  input/output sign : 0 = unsigned. 1 = signed. */
	__u32 is_signed;
};

/*!
 * \ brief ROIS parameters - fully described in user guide
 */
struct vpul_pu_rois_out {
	__u32 bits_in0	     :2;
	__u32 signed_in0     :1;
	__u32 use_mask       :1;
	__u32 work_mode      :2;
	__u32 first_min_max  :1;
	__u32 thresh_lo_temp :12;
};

/*!
 * \ brief CROP parameters - fully described in user guide
 */
struct vpul_pu_crop {
	__u32 work_mode	   :2;
	__u32 pad_value    :16;
	__u32 mask_val_in  :16;
	__u32 mask_val_out :16;
	__u32 roi_startx   :14;
	__u32 roi_starty   :14;
};

/*!
 * \ brief MDE parameters - fully described in user guide
 */
struct vpul_pu_mde {
	__u32 work_mode		    :3;
	__u32 result_shift          :6;
	__u32 use_thresh            :1;
	__u32 calc_quantized_angle  :1;
	__u32 eig_coeff             :25;
	__u32 bits_in               :2;
	__u32 signed_in             :1;
	__u32 bits_out              :2;
	__u32 signed_out            :1;
	__u32 output_enable         :2;
	__u32 thresh;
};

/*!
 * \ brief NMS parameters - fully described in user guide
 */
struct vpul_pu_nms {
	__u32 work_mode		     :4;
	__u32 keep_equals            :1;
	__u32 directional_nms        :1;
	__u32 census_mode            :2;
	__u32 add_orig_pixel         :1;
	__u32 bits_in                :2;
	__u32 bits_out               :2;
	__u32 signed_in              :1;
	__u32 support                :2;
	__u32 org_val_out            :1;
	__u32 trunc_out              :1;
	__u32 image_height           :14;
	__u32 thresh;
	__u32 border_mode_up         :1;
	__u32 border_mode_down       :1;
	__u32 border_mode_left       :1;
	__u32 border_mode_right      :1;
	__u32 border_fill            :2;
	__u32 border_fill_constant   :8;
	__u32 strict_comparison_mask :3;
	__u32 cens_thres_0           :3;
	__u32 cens_thres_1           :3;
	__u32 cens_thres_2           :3;
	__u32 cens_thres_3           :3;
	__u32 cens_thres_4           :3;
	__u32 cens_thres_5           :3;
	__u32 cens_thres_6           :3;
	__u32 cens_thres_7           :3;
};

/*! /brief Description for filter coefficients */
struct coeff_item {
	/*!  \brief
         * 0=coefficients are part of static vector.
	 * 1=coefficients are part of dynamic vector (parameters vector)
	 */
	__u32 is_dynamic;
	/*!  \brief coefficients vector dimension size.
	 */
	__u32 vsize;
	/*!  Offsets of the first coefficient from the offset of the process
	 * coefficients vector (either static or dynamic)
	 * TODO: check if it's the same as coefficient_index
	 */
	__u32 offset;
};

/*!
 * \ brief Separable Filter parameters - fully described in user guide
 */
struct vpul_pu_slf {
	__u32 invert_columns                            :1;
	__u32 upsample_mode                             :2;
	__u32 downsample_rows                           :1;
	__u32 downsample_cols                           :1;
	__u32 sampling_offset_x                         :1;
	__u32 sampling_offset_y                         :2;
	__u32 work_mode                                 :2;
	__u32 filter_size_mode                          :2;
	__u32 out_enable_1                              :1;
	__u32 horizontal_only                           :1;
	__u32 bits_in                                   :2;
	__u32 bits_out                                  :2;
	__u32 signed_in                                 :1;
	__u32 signed_out                                :1;
	__u32 do_rounding                               :1;
	__u32 border_mode_up                            :1;
	__u32 border_mode_down                          :1;
	__u32 border_mode_left                          :1;
	__u32 border_mode_right                         :1;
	__u32 border_fill                               :2;
	__u32 border_fill_constant                      :16;
	__u32 coefficient_fraction                      :4;
	__u32 sepfregs_is_max_pooling_mode              :1;
	__u32 sepfregs_stride_value                     :3;
	__u32 sepfregs_stride_offset_height             :2;
	__u32 sepfregs_stride_offset_width              :2;
	__u32 sepfregs_subimage_height                  :17;
	__u32 sepfregs_convert_16f_to_32f               :1;
	__u32 sepfregs_convert_output_sm_to_2scomp      :1;
	__u32 sepfregs_convert_input_2scomp_to_sm       :1;
	__u32 max_pooling_size_spoof_index              :5;

	__u32 maxp_num_slices;
	__u32 maxp_sizes_filt_hor;
	__u32 maxp_sizes_filt_ver;
	/*!  horizontal filter coefficients */
	struct coeff_item horizontal_filter_coeff;

	/*!  vertical filter coefficients */
	struct coeff_item vertical_filter_coeff;
	__u32 coefficient_index;
};

/*!
 * \ brief General Filter parameters - fully described in user guide
 */
struct vpul_pu_glf {
	__u32 filter_size_mode	:3;
	__u32 sad_mode			:1;
	__u32 out_enable_2		:1;
	__u32 two_outputs		:1;
	__u32 input_enable1		:1;
	__u32 bits_in			:1;
	__u32 bits_out			:1;
	__u32 signed_in			:1;
	__u32 signed_out		:1;
	__u32 do_rounding		:1;
	__u32 RAM_type			:1;
	__u32 RAM_offset		:3;
	__u32 image_height		:14;
	__u32 border_mode_up		:1;
	__u32 border_mode_down		:1;
	__u32 border_mode_left		:1;
	__u32 border_mode_right		:1;
	__u32 border_fill		:2;
	__u32 border_fill_constant	:16;
	__u32 coefficient_fraction	:5;
	__u32 signed_coefficients	:1;


	__u32 coefficient_index;
	/*!
	 * value 1 : the coefficients are read by DMA
	 * 0 : coefficients should be explicitly written to registers by CORE
	 */
	__u32 coeffs_from_dma;

	/*!  index into the process level coefficients structure */
	struct coeff_item filter_coeff;
};

/*!
 * \ brief CCM parameters - fully described in user guide
 */
struct vpul_pu_ccm {
	__u32 signed_in			:1;
	__u32 signed_out		:1;
	__u32 output_enable		:3;
	__u32 input_enable		:3;
	__u32 coefficient_shift		:4;
	__u32 coefficient_0		:12;
	__u32 coefficient_1		:12;
	__u32 coefficient_2		:12;
	__u32 coefficient_3		:12;
	__u32 coefficient_4		:12;
	__u32 coefficient_5		:12;
	__u32 coefficient_6		:12;
	__u32 coefficient_7		:12;
	__u32 coefficient_8		:12;
	__u32 offset_0			:16;
	__u32 offset_1			:16;
	__u32 offset_2			:16;
};

/*!
 * \ brief LUT parameters - fully described in user guide
 */
struct vpul_pu_lut {
	__u32 interpolation_mode	:1;
	__u32 lut_size			:1;
	__u32 signed_in0		:1;
	__u32 signed_out0		:1;
	__u32 offset			:16;
	__u32 binsize			:16;
	__u32 inverse_binsize		:16;
};

/*!
 * \ brief Integral Image parameters - fully described in user guide
 */
struct vpul_pu_integral {
	__u32 integral_image_mode		:3;
	__u32 overflow_mode			:1;
	__u32 dt_right_shift			:3;
	__u32 dt_left_shift			:3;
	__u32 dt_coefficient0			:16;
	__u32 dt_coefficient1			:16;
	__u32 dt_coefficient2			:16;
	__u32 cc_min_label			:15;
	__u32 cc_scan_mode			:2;
	__u32 cc_smart_label_search_en		:1;
	__u32 cc_reset_labels_array		:1;
	__u32 cc_label_vector_size		:16;
	__u32 lut_init_en			:1;
	__u32 lut_number_of_values		:16;
	__u32 lut_value_shift			:16;
	__u32 lut_default_overflow		:16;
	__u32 lut_default_underflow		:16;

};

/*!
 * \ brief Upscaler parameters - fully described in user guide
 */
struct vpul_pu_upscaler {
	__u32 interpolation_method		:2;
	__u32 border_fill_mode			:1;
	__u32 do_rounding			:1;
	__u32 border_fill_constant		:16;
};

/*!
 * \ brief Downscaler parameters - fully described in user guide
 */
struct vpul_pu_downscaler {
	__u32 interpolation_method	:2;
	__u32 do_rounding		:1;
};

/*!
 * \ brief Joiner parameters - fully described in user guide
 */
struct vpul_pu_joiner {
	__u32 out_byte0_source_stream	:5;
	__u32 out_byte1_source_stream	:5;
	__u32 out_byte2_source_stream	:5;
	__u32 out_byte3_source_stream	:5;
	__u32 input0_enable	:1;
	__u32 input1_enable	:1;
	__u32 input2_enable	:1;
	__u32 input3_enable	:1;
	__u32 work_mode	:2;

};

/*!
 * \brief Splitter parameters - fully described in user guide
 */
struct vpul_pu_spliter {
	__u32 out0_byte0:3;
	__u32 out0_byte1:3;
	__u32 out1_byte0:3;
	__u32 out1_byte1:3;
	__u32 out2_byte0:3;
	__u32 out2_byte1:3;
	__u32 out3_byte0:3;
	__u32 out3_byte1:3;
};


/*!
 * \brief Histogram parameters - fully described in user guide
 */
struct vpul_pu_histogram {
	__u32 offset			:16;
	__u32 inverse_binsize		:17;
	__u32 variable_increment	:1;
	__u32 dual_histogram		:1;
	__u32 signed_in0		:1;

	__u32 round_index		:1;
	__u32 max_val			:16;
};

/*!
 * \brief Non linear filter(fast) - fully described in user guide
 */
struct vpul_pu_nlf {
	__u32 filter_mode			:3;
	__u32 fast_score_direction	:1;
	__u32 border_mode_up		:1;
	__u32 border_mode_down		:1;
	__u32 border_mode_left		:1;
	__u32 border_mode_right		:1;
	__u32 border_fill			:2;
	__u32 border_tile			:1;
	__u32 border_fill_constant	:8;
	__u32 bits_in				:1;
	__u32 signed_in				:1;
	__u32 census_mode			:2;
	__u32 census_out_image		:1;
	__u32 add_orig_pixel		:1;
	__u32 cens_thres_0			:3;
	__u32 cens_thres_1			:3;
	__u32 cens_thres_2			:3;
	__u32 cens_thres_3			:3;
	__u32 cens_thres_4			:3;
	__u32 cens_thres_5			:3;
	__u32 cens_thres_6			:3;
	__u32 cens_thres_7			:3;
	__u32 neighbors_mask		:9;
};

/*!
 * \brief fast depth - fully described in user guide
 */
struct vpul_pu_fast_depth {
	__u32 max_disparity								:8;
	__u32 min_disparity								:8;
	__u32 dcu_gray_diff_thresh_is_equal_pixels		:8;
	__u32 dcu_gray_diff_thresh_is_near_pixels		:8;
	__u32 dcu_gray_diff_thresh_is_near_to_center	:8;
	__u32 dcu_gray_diff_thresh_do_aggregation		:8;
	__u32 dcu_gray_diff_thresh_is_equal_to_center	:8;
	__u32 dcu_aggregated_neighbor_selection			:11;
	__u32 dcu_patch_size_mode						:2;
	__u32 dcu_rl_borders_trim						:2;
	__u32 dcu_info_thresh_is_informative			:6;
	__u32 dcu_min_pixels_for_cost					:6;
	__u32 dcu_cost_fraction_bits					:4;
	__u32 dcu_cost_c_exp_weights_table_0_0			:8;
	__u32 dcu_cost_c_exp_weights_table_0_1			:8;
	__u32 dcu_cost_c_exp_weights_table_0_2			:8;
	__u32 dcu_cost_c_exp_weights_table_0_3			:8;
	__u32 dcu_cost_c_exp_weights_table_0_4			:8;
	__u32 dcu_cost_c_exp_weights_table_0_5			:8;
	__u32 dcu_cost_c_exp_weights_table_0_6			:8;
	__u32 dcu_cost_c_exp_weights_table_0_7			:8;
	__u32 dpu_cost_thresh_max_good					:8;
	__u32 dpu_ref_dspr_constant_cost				:8;
	__u32 dpu_cost_thresh_count_good_cand			:8;
	__u32 dpu_ref_dspr_max_is_equal					:7;
	__u32 dpu_ref_cost_thresh_use_ref_dsp			:8;
	__u32 dpu_dspr_diff_max_neighbor_is_equal		:7;
	__u32 dpu_ref_dspr_cost_factor					:3;
	__u32 dpu_use_ref_en							:1;
	__u32 dpu_ref_candidate_en						:1;
	__u32 dpu_ref_dspr_prioritization_en			:1;
	__u32 dpu_ref_dspr_up_shift						:2;
	__u32 pre_output_dspr_up_shift					:3;
	__u32 superb_candidate_en						:1;
	__u32 superb_dspr_cost_factor					:3;
	__u32 prioritize_superb_on_non_info_only		:1;
	__u32 dpu_superb_dspr_prioritization_en			:1;
	__u32 dcu_gray_diff_thresh_is_like_superb		:8;
	__u32 cost_thresh_good_for_superb				:8;
	__u32 superb_dspr_constant_cost					:8;
	__u32 dpu_lfsr_seed_0							:16;
	__u32 dpu_lfsr_seed_1							:16;
	__u32 dpu_lfsr_seed_2							:16;
	__u32 dpu_lfsr_seed_3							:16;
	__u32 mcnt_selected_iterations_0_0				:1;
	__u32 mcnt_selected_iterations_0_1				:1;
	__u32 mcnt_selected_iterations_0_2				:1;
	__u32 mcnt_selected_iterations_0_3				:1;
};

/*!
 * \brief FIFO - fully described in user guide
 */
struct vpul_pu_fifo {
	__u32 bits_in                 :1;
};

/*!
 * \brief duplicator - No specific parameters - place holder
 */
struct vpul_pu_duplicator {
	__u32 no_params;
};

struct vpul_pu_dispeq {
	__u32 bypass							:1;
	__u32 mcnt_bilateral_maximization_en	:1;
	__u32 mcnt_use_center_only_en			:1;
	__u32 dspr_shift_down_before_validation	:2;
	__u32 cost_min_for_valid_for_small_dspr	:8;
	__u32 cost_min_for_valid_for_large_dspr	:8;
	__u32 dspr_small_to_large_tresh			:7;
	__u32 dspr_min_diff_for_valid			:7;
	__u32 cost_min_diff_for_update			:8;
	__u32 weight_for_cost_update			:8;
	__u32 mcnt_min_for_multiple_dspr		:3;
};

struct vpul_pu_in_paint {
	__u32 bypass							:1;
	__u32 pic_x_size						:12;
	__u32 pic_y_size						:14;
	__u32 mem_size							:1;
	__u32 rl_validation_en					:1;
	__u32 enhanced_info_en					:1;
	__u32 content_mask_en					:1;
	__u32 x_last_logic_en					:1;
	__u32 background_en						:1;
	__u32 strong_anti_leak_en				:1;
	__u32 background_anti_leak_en			:1;
	__u32 half_strong_en					:1;
	__u32 update_inpainted_cost_en			:1;
	__u32 output_reliable_mask_en			:1;
	__u32 output_stat_mask_en				:1;
	__u32 gray_max_similarity				:8;
	__u32 gray_max_high_similarity			:8;
	__u32 gray_step_penalty					:8;
	__u32 gray_max_total_error				:8;
	__u32 mcnt_min_single					:3;
	__u32 mcnt_max_single					:3;
	__u32 mcnt_min_for_stat_mask			:3;
	__u32 mcnt_max_for_stat_mask			:3;
	__u32 dspr_max_vertical_similarity		:7;
	__u32 dspr_max_diff_from_vertical_avrg	:7;
	__u32 dspr_max_similarity				:7;
	__u32 dspr_max_upper_similarity			:7;
	__u32 dspr_max_anchors_similarity		:7;
	__u32 dspr_max_anchors_grad_val			:7;
	__u32 dspr_max_anchors_grad_shift		:4;
	__u32 dspr_max_diff_to_center			:7;
	__u32 anchor_type_sel_max_support		:2;
	__u32 anchor_type_sel_max_near_support	:2;
	__u32 anchor_type_sel_min_valid			:3;
	__u32 anchor_type_sel_min_near_valid	:3;
	__u32 anchor_logic_trim_0_0				:12;
	__u32 anchor_logic_trim_0_1				:12;
	__u32 x_last_logic_max_edge_diff		:12;
	__u32 x_last_logic_right_margin			:12;
	__u32 downscaled_cost_min_per_index_0_0	:8;
	__u32 downscaled_cost_min_per_index_0_1	:8;
	__u32 downscaled_cost_min_per_index_0_2	:8;
	__u32 cost_min_fill_holes				:8;
	__u32 cost_max_reliable					:8;
	__u32 cost_max_for_anchor				:8;
	__u32 cost_min_is_hidden				:8;
	__u32 cost_max_for_stat_mask			:8;
	__u32 anchor_dist_max_for_strong		:12;
	__u32 hidden_padding_direction			:2;
	__u32 background_gray_val				:8;
	__u32 background_gray_max_similarity	:8;
	__u32 background_dspr_val				:7;
	__u32 background_anchor_margins_0_0		:12;
	__u32 background_anchor_margins_0_1		:12;
	__u32 background_anchor_margins_0_2		:12;
	__u32 background_anchor_margins_0_3		:12;
	__u32 hidden_logic_trim_0_0				:12;
	__u32 hidden_logic_trim_0_1				:12;
	__u32 output_mode						:2;
};

struct vpul_pu_cnn {
	__u32 is_fetch_mode			:1;
	__u32 is_bypass				:1;
};

/*! * \brief FLAMORB parameters
 * work_mode-->pack_output
 * work_mode-->first_minus_second
 * enable_out1 = !work_mode
 * eff_mode = bits_in==8 & mprb==Large
 * pack_bitage = 16
 * bit_diff = 1
 * enable_out1 =1
 * coord_pairs_offset = group_index * flamorb->num_coord_pairs
 */
struct vpul_pu_flam_orb {
	__u32 work_mode				:1;
	__u32 eff_mode				:1;//need to remove after HW mapper change
	__u32 first_minus_second	:1;//need to remove after HW mapper change
	__u32 bit_diff				:1;//need to remove after HW mapper change
	__u32 pack_output			:1;//need to remove after HW mapper change
	__u32 pack_bitage			:1;//need to remove after HW mapper change
	__u32 bits_in				:1;
	__u32 start_x				:8; //need to remove after HW mapper change
	__u32 start_y				:8; //need to remove after HW mapper change
	__u32 end_x				:8; //need to remove after HW mapper change
	__u32 end_y				:8; //need to remove after HW mapper change
	__u32 const_val				:8;
	__u32 use_const_val			:1;
	//__u32 enable_out1			:1;
	//__u32 coord_pairs_offset		:10;
	__u32 num_coord_pairs			:11;
	__u32 scaler_ind				:1;
	__u32 roi_ind					:5;
	__u32 full_roi					:5;


	__u32 inout_index;
};

/*!
 * \brief Map to List parameters
 * \details <B>Calculating buffer size needed:</B> \n Output buffer size written by block calculated in the following way:\n
 * \li if value_in==0 then BytesPerReocrd = 4
 * \li if value_in==1 then BytesPerReocrd = 6 (for bits_in==0 or 1) or 8 (for bits_in==2)
 * \li BytesPerLine = 240(if BytesPerRecord ==8) or 252 (otherwise)
 * \li NumberOfLines = CIEL_DIV(BytesPerLine/BytesPerRecord, num_of_points)
 * \li totalSizeRequired = (NumberOfLines*BytesPerLine)+2
 */
struct vpul_pu_map2list {
	__u32 bits_in				:2;
	__u32 signed_in				:1;
	__u32 value_in				:1;
	__u32 enable_out_map			:1;
	__u32 enable_out_lst			:1;
	__u32 inout_indx			:5;
	__u32 threshold_low;
	__u32 threshold_high;
	__u32 num_of_point;
};

/*! \brief Union for parameters of all different types of Processing units*/
union vpul_pu_parameters {
	struct vpul_pu_dma		dma;
	struct vpul_pu_salb		salb;
	struct vpul_pu_calb		calb;
	struct vpul_pu_rois_out		rois_out;
	struct vpul_pu_crop		crop;
	struct vpul_pu_mde		mde;
	struct vpul_pu_map2list		map2list;
	struct vpul_pu_nms		nms;
	struct vpul_pu_slf		slf;
	struct vpul_pu_glf		glf;
	struct vpul_pu_ccm		ccm;
	struct vpul_pu_lut		lut;
	struct vpul_pu_integral		integral;
	struct vpul_pu_upscaler		upscaler;
	struct vpul_pu_downscaler	downScaler;
	struct vpul_pu_joiner		joiner;
	struct vpul_pu_spliter		spliter;
	struct vpul_pu_histogram	histogram;
	struct vpul_pu_nlf		nlf;
	struct vpul_pu_fast_depth	depth;
	struct vpul_pu_dispeq		disparity;
	struct vpul_pu_in_paint		in_paint;
	struct vpul_pu_cnn		cnn;
	struct vpul_pu_flam_orb		flam_orb;
	struct vpul_pu_fifo		fifo;

	struct vpul_pu_add		add;
};

#define NO_PU_CONNECTED			0xFF

/*!\brief describe input connections of one PU */
struct vpu_pu_input {
	/*! the index of the source PU in the sub-chains PU's list.
	 * A value of NO_PU_CONNECTED (= 0xFF) means that this input is disabled;
	 * this is useful in case there is only 1 active connection to this PU,
	 * on input port other than port #0 - for instance port #2 :
	 * in this case n_in_connect will be set to 3 in vpul_pu structure, and
	 * pu_idx will be set to 0xFF for first 2 instances of vpu_pu_input
	 */
	__u32				pu_idx;
	/*! the index in the source PU output connections, that connected
	 * to this PU.
	 */
	__u32				s_pu_out_idx;
};

#define NO_MPRB_CONNECTED		0xFF

/*!\brief This structure contains one processing unit description */
struct vpul_pu {

	/*!\brief if !=0 bypass block   */
	__u32 bypass;
	/*!\brief operation type */
	enum vpul_logical_op_types	op_type;
	/*!\brief block instance index */
	enum vpul_pu_instance		instance;
	/*!\brief number of MPRBs used */
	__u32				n_mprbs;
	/*!\brief 1K / 4K - see VPUH_MPRB_TYPE values. if both small and large MPRBs
	 * are used :will indicate "large" (4K)
	 */
	__u32				mprb_type;

	 /*! memory connectivity for line buffers; contents of entry #N in array is MPRB actively
	  * connected to ram port #N; if no such MPRB, contents = NO_MPRB_CONNECTED (= 0xFF)
	  * The effective size of the array is max #of ram ports per PU.
	  */
	__u32				mprbs[VPU_MAXIMAL_MPRB_CONNECTED];
	/*!  number of input connections */
	__u32				n_in_connect;
	/*!  input connection information */
	struct vpu_pu_input		in_connect[VPUL_PU_MAX_PORTS];
	/*!  number of output connections */
	__u32				n_out_connect;
	/*!  index of the block input sizes information in process
	 * sizes vector - vpul_sizes
	 */
	__u32				in_size_idx;
	/*!  index of the block output sizes in process
	 * sizes vector - vpul_sizes
	 */
	__u32				out_size_idx;

	/*!  number of params that arrive to the prm of current pu from HW results of
	 * previous pu's
	 */
	__u32				n_run_time_params_from_hw;

	/*!  PU parameter structure */
	union vpul_pu_parameters	params;
};

/*!
 * \brief PU location in TDS, implemented as indexes of vertex, subchain and PU.
 */
struct vpul_pu_location {
	/*!  the index of the vertex the PU is in. */
	__u32				vtx_idx;
	/*!  the index of the subchain in the vertex the PU is in. */
	__u32				sc_idx_in_vtx;
	/*!  the index of the subchain in the sc the PU is in. */
	__u32				pu_idx_in_sc;
};

/** @}*/
/** @}*/

#endif
