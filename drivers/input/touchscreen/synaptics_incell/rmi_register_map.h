/*
 * Synaptics DSX touchscreen driver
 *
 * Copyright (C) 2012 Synaptics Incorporated
 *
 * Copyright (C) 2012 Alexandra Chin <alexandra.chin@tw.synaptics.com>
 * Copyright (C) 2012 Scott Lin <scott.lin@tw.synaptics.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/*
 ***************************************************
 * Structures for F01 registers.
 ***************************************************
 */

struct synaptics_rmi4_f01_device_status {
	union {
		struct {
			unsigned char status_code:4;
			unsigned char reserved:2;
			unsigned char flash_prog:1;
			unsigned char unconfigured:1;
		} __packed;
		unsigned char data[1];
	};
};

struct synaptics_rmi4_f01_device_control {
	union {
		struct {
			unsigned char sleep_mode:2;
			unsigned char nosleep:1;
			unsigned char reserved:2;
			unsigned char charger_connected:1;
			unsigned char report_rate:1;
			unsigned char configured:1;
		} __packed;
		unsigned char data[1];
	};
};
/*
 ***************************************************
 * Structures for F12 registers.
 ***************************************************
 */
struct synaptics_rmi4_f12_query_5 {
	union {
		struct {
			unsigned char size_of_query6;
			struct {
				unsigned char ctrl0_is_present:1;
				unsigned char ctrl1_is_present:1;
				unsigned char ctrl2_is_present:1;
				unsigned char ctrl3_is_present:1;
				unsigned char ctrl4_is_present:1;
				unsigned char ctrl5_is_present:1;
				unsigned char ctrl6_is_present:1;
				unsigned char ctrl7_is_present:1;
			} __packed;
			struct {
				unsigned char ctrl8_is_present:1;
				unsigned char ctrl9_is_present:1;
				unsigned char ctrl10_is_present:1;
				unsigned char ctrl11_is_present:1;
				unsigned char ctrl12_is_present:1;
				unsigned char ctrl13_is_present:1;
				unsigned char ctrl14_is_present:1;
				unsigned char ctrl15_is_present:1;
			} __packed;
			struct {
				unsigned char ctrl16_is_present:1;
				unsigned char ctrl17_is_present:1;
				unsigned char ctrl18_is_present:1;
				unsigned char ctrl19_is_present:1;
				unsigned char ctrl20_is_present:1;
				unsigned char ctrl21_is_present:1;
				unsigned char ctrl22_is_present:1;
				unsigned char ctrl23_is_present:1;
			} __packed;
			struct {
				unsigned char ctrl24_is_present:1;
				unsigned char ctrl25_is_present:1;
				unsigned char ctrl26_is_present:1;
				unsigned char ctrl27_is_present:1;
				unsigned char ctrl28_is_present:1;
				unsigned char ctrl29_is_present:1;
				unsigned char ctrl30_is_present:1;
				unsigned char ctrl31_is_present:1;
			} __packed;
		};
		unsigned char data[5];
	};
};

struct synaptics_rmi4_f12_query_8 {
	union {
		struct {
			unsigned char size_of_query9;
			struct {
				unsigned char data0_is_present:1;
				unsigned char data1_is_present:1;
				unsigned char data2_is_present:1;
				unsigned char data3_is_present:1;
				unsigned char data4_is_present:1;
				unsigned char data5_is_present:1;
				unsigned char data6_is_present:1;
				unsigned char data7_is_present:1;
			} __packed;
			struct {
				unsigned char data8_is_present:1;
				unsigned char data9_is_present:1;
				unsigned char data10_is_present:1;
				unsigned char data11_is_present:1;
				unsigned char data12_is_present:1;
				unsigned char data13_is_present:1;
				unsigned char data14_is_present:1;
				unsigned char data15_is_present:1;
			} __packed;
		};
		unsigned char data[3];
	};
};

struct synaptics_rmi4_f12_query_10 {
	union {
		struct {
			unsigned char f12_query10_b0__4:5;
			unsigned char glove_mode_feature:1;
			unsigned char f12_query10_b6__7:2;
		} __packed;
		unsigned char data[1];
	};
};

struct synaptics_rmi4_f12_ctrl_8 {
	union {
		struct {
			unsigned char max_x_coord_lsb;
			unsigned char max_x_coord_msb;
			unsigned char max_y_coord_lsb;
			unsigned char max_y_coord_msb;
			unsigned char rx_pitch_lsb;
			unsigned char rx_pitch_msb;
			unsigned char tx_pitch_lsb;
			unsigned char tx_pitch_msb;
			unsigned char low_rx_clip;
			unsigned char high_rx_clip;
			unsigned char low_tx_clip;
			unsigned char high_tx_clip;
			unsigned char num_of_rx;
			unsigned char num_of_tx;
		};
		unsigned char data[14];
	};
};

struct synaptics_rmi4_f12_ctrl_9 {
	union {
		struct {
			unsigned char touch_threshold;
			unsigned char lift_hysteresis;
			unsigned char small_z_scale_factor_lsb;
			unsigned char small_z_scale_factor_msb;
			unsigned char large_z_scale_factor_lsb;
			unsigned char large_z_scale_factor_msb;
			unsigned char small_large_boundary;
			unsigned char wx_scale;
			unsigned char wx_offset;
			unsigned char wy_scale;
			unsigned char wy_offset;
			unsigned char x_size_lsb;
			unsigned char x_size_msb;
			unsigned char y_size_lsb;
			unsigned char y_size_msb;
			unsigned char gloved_finger;
		};
		unsigned char data[16];
	};
};

struct synaptics_rmi4_f12_ctrl_11 {
	union {
		struct {
			unsigned char small_corner;
			unsigned char large_corner;
			unsigned char jitter_filter_strength;
			unsigned char x_minimum_z;
			unsigned char y_minimum_z;
			unsigned char x_maximum_z;
			unsigned char y_maximum_z;
			unsigned char x_amplitude;
			unsigned char y_amplitude;
			unsigned char gloved_finger_jitter_filter_strength;
		};
		unsigned char data[10];
	};
};

struct synaptics_rmi4_f12_ctrl_23 {
	union {
		struct {
			unsigned char obj_type_enable;
			unsigned char max_reported_objects;
		};
		unsigned char data[2];
	};
};

struct synaptics_rmi4_f12_ctrl_26 {
	union {
		struct {
			unsigned char feature_enable;
		};
		unsigned char data[1];
	};
};

struct synaptics_rmi4_f12_finger_data {
	unsigned char object_type_and_status;
	unsigned char x_lsb;
	unsigned char x_msb;
	unsigned char y_lsb;
	unsigned char y_msb;
#ifdef REPORT_2D_Z
	unsigned char z;
#endif
#ifdef REPORT_2D_W
	unsigned char wx;
	unsigned char wy;
#endif
};

struct synaptics_rmi4_f12_extra_data {
	unsigned char data1_offset;
	unsigned char data15_offset;
	unsigned char data15_size;
	unsigned char data15_data[(F12_FINGERS_TO_SUPPORT + 7) / 8];
};
/*
 ***************************************************
 * Structures for F1A registers.
 ***************************************************
 */
struct synaptics_rmi4_f1a_query {
	union {
		struct {
			unsigned char max_button_count:3;
			unsigned char reserved:5;
			unsigned char has_general_control:1;
			unsigned char has_interrupt_enable:1;
			unsigned char has_multibutton_select:1;
			unsigned char has_tx_rx_map:1;
			unsigned char has_perbutton_threshold:1;
			unsigned char has_release_threshold:1;
			unsigned char has_strongestbtn_hysteresis:1;
			unsigned char has_filter_strength:1;
		} __packed;
		unsigned char data[2];
	};
};

struct synaptics_rmi4_f1a_control_0 {
	union {
		struct {
			unsigned char multibutton_report:2;
			unsigned char filter_mode:2;
			unsigned char reserved:4;
		} __packed;
		unsigned char data[1];
	};
};

struct synaptics_rmi4_f1a_control {
	struct synaptics_rmi4_f1a_control_0 general_control;
	unsigned char button_int_enable;
	unsigned char multi_button;
	unsigned char *txrx_map;
	unsigned char *button_threshold;
	unsigned char button_release_threshold;
	unsigned char strongest_button_hysteresis;
	unsigned char filter_strength;
};

/*
 ***************************************************
 * Structures for F34 registers.
 ***************************************************
 */
struct synaptics_rmi4_f34_ctrl_0 {
	union {
		struct {
			unsigned char fw_release_month;
			unsigned char fw_release_date;
			unsigned char fw_release_revision;
			unsigned char fw_release_version;
		};
		unsigned char data[4];
	};
};

struct synaptics_rmi4_f34_query_01 {
	union {
		struct {
			unsigned char reg_map:1;
			unsigned char unlocked:1;
			unsigned char has_config_id:1;
			unsigned char has_perm_config:1;
			unsigned char has_bl_config:1;
			unsigned char has_disp_config:1;
			unsigned char has_ctrl1:1;
			unsigned char has_flash_query4:1;
		} __packed;
		unsigned char data[1];
	};
};

struct synaptics_rmi4_f34_query_04 {
	union {
		struct {
			unsigned char has_guest_code:1;
			unsigned char reserved:1;
		} __packed;
		unsigned char data[1];
	};
};
/*
 ***************************************************
 * Structures for F51 registers.
 ***************************************************
 */
struct synaptics_rmi4_f51_query {
	union {
		struct {
			unsigned char query_register_count;
			unsigned char data_register_count;
			unsigned char control_register_count;
			unsigned char command_register_count;
		};
		unsigned char data[4];
	};
};

struct synaptics_rmi4_f51_data {
	union {
		struct {
			unsigned char temp_data;
		};
		unsigned char data[1];
	};
};

/*
 ***************************************************
 * Structures for F54 registers.
 ***************************************************
 */
struct f54_query {
	union {
		struct {
			/* query 0 */
			unsigned char num_of_rx_electrodes;

			/* query 1 */
			unsigned char num_of_tx_electrodes;

			/* query 2 */
			unsigned char f54_query2_b0__1:2;
			unsigned char has_baseline:1;
			unsigned char has_image8:1;
			unsigned char f54_query2_b4__5:2;
			unsigned char has_image16:1;
			unsigned char f54_query2_b7:1;

			/* queries 3.0 and 3.1 */
			unsigned short clock_rate;

			/* query 4 */
			unsigned char touch_controller_family;

			/* query 5 */
			unsigned char has_pixel_touch_threshold_adjustment:1;
			unsigned char f54_query5_b1__7:7;

			/* query 6 */
			unsigned char has_sensor_assignment:1;
			unsigned char has_interference_metric:1;
			unsigned char has_sense_frequency_control:1;
			unsigned char has_firmware_noise_mitigation:1;
			unsigned char has_ctrl11:1;
			unsigned char has_two_byte_report_rate:1;
			unsigned char has_one_byte_report_rate:1;
			unsigned char has_relaxation_control:1;

			/* query 7 */
			unsigned char curve_compensation_mode:2;
			unsigned char f54_query7_b2__7:6;

			/* query 8 */
			unsigned char f54_query8_b0:1;
			unsigned char has_iir_filter:1;
			unsigned char has_cmn_removal:1;
			unsigned char has_cmn_maximum:1;
			unsigned char has_touch_hysteresis:1;
			unsigned char has_edge_compensation:1;
			unsigned char has_per_frequency_noise_control:1;
			unsigned char has_enhanced_stretch:1;

			/* query 9 */
			unsigned char has_force_fast_relaxation:1;
			unsigned char has_multi_metric_state_machine:1;
			unsigned char has_signal_clarity:1;
			unsigned char has_variance_metric:1;
			unsigned char has_0d_relaxation_control:1;
			unsigned char has_0d_acquisition_control:1;
			unsigned char has_status:1;
			unsigned char has_slew_metric:1;

			/* query 10 */
			unsigned char has_h_blank:1;
			unsigned char has_v_blank:1;
			unsigned char has_long_h_blank:1;
			unsigned char has_startup_fast_relaxation:1;
			unsigned char has_esd_control:1;
			unsigned char has_noise_mitigation2:1;
			unsigned char has_noise_state:1;
			unsigned char has_energy_ratio_relaxation:1;

			/* query 11 */
			unsigned char has_excessive_noise_reporting:1;
			unsigned char has_slew_option:1;
			unsigned char has_two_overhead_bursts:1;
			unsigned char has_query13:1;
			unsigned char has_one_overhead_burst:1;
			unsigned char f54_query11_b5:1;
			unsigned char has_ctrl88:1;
			unsigned char has_query15:1;

			/* query 12 */
			unsigned char number_of_sensing_frequencies:4;
			unsigned char f54_query12_b4__7:4;
		} __packed;
		unsigned char data[14];
	};
};

struct f54_query_13 {
	union {
		struct {
			/* query 13 */
			unsigned char has_ctrl86:1;
			unsigned char has_ctrl87:1;
			unsigned char has_ctrl87_sub0:1;
			unsigned char has_ctrl87_sub1:1;
			unsigned char has_ctrl87_sub2:1;
			unsigned char has_cidim:1;
			unsigned char has_noise_mitigation_enhancement:1;
			unsigned char has_rail_im:1;
		} __packed;
		unsigned char data[1];
	};
};

struct f54_query_15 {
	union {
		struct {
			unsigned char has_ctrl90:1;
			unsigned char has_transmit_strength:1;
			unsigned char has_ctrl87_sub3:1;
			unsigned char has_query16:1;
			unsigned char has_query20:1;
			unsigned char has_query21:1;
			unsigned char has_query22:1;
			unsigned char has_query25:1;
		} __packed;
		unsigned char data[1];
	};
};

struct f54_query_16 {
	union {
		struct {
			unsigned char has_query17:1;
			unsigned char has_data17:1;
			unsigned char has_ctrl92:1;
			unsigned char has_ctrl93:1;
			unsigned char has_ctrl94_query18:1;
			unsigned char has_ctrl95_query19:1;
			unsigned char has_ctrl99:1;
			unsigned char has_ctrl100:1;
		} __packed;
		unsigned char data[1];
	};
};

struct f54_query_21 {
	union {
		struct {
			unsigned char has_abs_rx:1;
			unsigned char has_abs_tx:1;
			unsigned char has_ctrl91:1;
			unsigned char has_ctrl96:1;
			unsigned char has_ctrl97:1;
			unsigned char has_ctrl98:1;
			unsigned char has_data19:1;
			unsigned char has_query24_data18:1;
		} __packed;
		unsigned char data[1];
	};
};

struct f54_query_22 {
	union {
		struct {
			unsigned char has_packed_image:1;
			unsigned char has_ctrl101:1;
			unsigned char has_dynamic_sense_display_ratio:1;
			unsigned char has_query23:1;
			unsigned char has_ctrl103_query26:1;
			unsigned char has_ctrl104:1;
			unsigned char has_ctrl105:1;
			unsigned char has_query28:1;
		} __packed;
		unsigned char data[1];
	};
};

struct f54_query_23 {
	union {
		struct {
			unsigned char has_ctrl102:1;
			unsigned char has_ctrl102_sub1:1;
			unsigned char has_ctrl102_sub2:1;
			unsigned char has_ctrl102_sub4:1;
			unsigned char has_ctrl102_sub5:1;
			unsigned char has_ctrl102_sub9:1;
			unsigned char has_ctrl102_sub10:1;
			unsigned char has_ctrl102_sub11:1;
		} __packed;
		unsigned char data[1];
	};
};

struct f54_query_25 {
	union {
		struct {
			unsigned char has_ctrl106:1;
			unsigned char has_ctrl102_sub12:1;
			unsigned char has_ctrl107:1;
			unsigned char has_ctrl108:1;
			unsigned char has_ctrl109:1;
			unsigned char has_data20:1;
			unsigned char f54_query25_b6:1;
			unsigned char has_query27:1;
		} __packed;
		unsigned char data[1];
	};
};

struct f54_query_27 {
	union {
		struct {
			unsigned char has_ctrl110:1;
			unsigned char has_data21:1;
			unsigned char has_ctrl111:1;
			unsigned char has_ctrl112:1;
			unsigned char has_ctrl113:1;
			unsigned char has_data22:1;
			unsigned char has_ctrl114:1;
			unsigned char has_query29:1;
		} __packed;
		unsigned char data[1];
	};
};

struct f54_query_29 {
	union {
		struct {
			unsigned char has_ctrl115:1;
			unsigned char has_ground_ring_options:1;
			unsigned char has_lost_bursts_tuning:1;
			unsigned char has_aux_exvcom2_select:1;
			unsigned char has_ctrl116:1;
			unsigned char has_data23:1;
			unsigned char has_ctrl117:1;
			unsigned char has_query30:1;
		} __packed;
		unsigned char data[1];
	};
};

struct f54_query_30 {
	union {
		struct {
			unsigned char has_ctrl118:1;
			unsigned char has_ctrl119:1;
			unsigned char has_ctrl120:1;
			unsigned char has_ctrl121:1;
			unsigned char has_ctrl122_query31:1;
			unsigned char has_ctrl123:1;
			unsigned char f54_query30_b6:1;
			unsigned char has_query32:1;
		} __packed;
		unsigned char data[1];
	};
};

struct f54_query_32 {
	union {
		struct {
			unsigned char has_ctrl125:1;
			unsigned char has_ctrl126:1;
			unsigned char has_ctrl127:1;
			unsigned char has_abs_charge_pump_disable:1;
			unsigned char has_query33:1;
			unsigned char has_data24:1;
			unsigned char has_query34:1;
			unsigned char has_query35:1;
		} __packed;
		unsigned char data[1];
	};
};

struct f54_query_33 {
	union {
		struct {
			unsigned char f54_query33_b0:1;
			unsigned char f54_query33_b1:1;
			unsigned char f54_query33_b2:1;
			unsigned char f54_query33_b3:1;
			unsigned char has_ctrl132:1;
			unsigned char has_ctrl133:1;
			unsigned char has_ctrl134:1;
			unsigned char has_query36:1;
		} __packed;
		unsigned char data[1];
	};
};

struct f54_query_35 {
	union {
		struct {
			unsigned char has_data25:1;
			unsigned char f54_query35_b1:1;
			unsigned char f54_query35_b2:1;
			unsigned char has_ctrl137:1;
			unsigned char has_ctrl138:1;
			unsigned char has_ctrl139:1;
			unsigned char has_data26:1;
			unsigned char has_ctrl140:1;
		} __packed;
		unsigned char data[1];
	};
};

struct f54_query_36 {
	union {
		struct {
			unsigned char f54_query36_b0:1;
			unsigned char has_ctrl142:1;
			unsigned char has_query37:1;
			unsigned char has_ctrl143:1;
			unsigned char has_ctrl144:1;
			unsigned char has_ctrl145:1;
			unsigned char has_ctrl146:1;
			unsigned char has_query38:1;
		} __packed;
		unsigned char data[1];
	};
};

struct f54_query_38 {
	union {
		struct {
			unsigned char has_ctrl147:1;
			unsigned char has_ctrl148:1;
			unsigned char has_ctrl149:1;
			unsigned char f54_query38_b3__6:4;
			unsigned char has_query39:1;
		} __packed;
		unsigned char data[1];
	};
};

struct f54_query_39 {
	union {
		struct {
			unsigned char f54_query39_b0__6:7;
			unsigned char has_query40:1;
		} __packed;
		unsigned char data[1];
	};
};

struct f54_query_40 {
	union {
		struct {
			unsigned char f54_query40_b0:1;
			unsigned char has_ctrl163_query41:1;
			unsigned char f54_query40_b2:1;
			unsigned char has_ctrl165_query42:1;
			unsigned char has_ctrl166:1;
			unsigned char has_ctrl167:1;
			unsigned char f54_query40_b6:1;
			unsigned char has_query43:1;
		} __packed;
		unsigned char data[1];
	};
};

struct f54_query_43 {
	union {
		struct {
			unsigned char f54_query43_b0__6:7;
			unsigned char has_query46:1;
		} __packed;
		unsigned char data[1];
	};
};

struct f54_query_46 {
	union {
		struct {
			unsigned char has_ctrl176:1;
			unsigned char f54_query46_b1:1;
			unsigned char has_ctrl179:1;
			unsigned char f54_query46_b3:1;
			unsigned char has_data27:1;
			unsigned char has_data28:1;
			unsigned char f54_query46_b6:1;
			unsigned char has_query47:1;
		} __packed;
		unsigned char data[1];
	};
};

struct f54_query_47 {
	union {
		struct {
			unsigned char f54_query47_b0__6:7;
			unsigned char has_query49:1;
		} __packed;
		unsigned char data[1];
	};
};

struct f54_query_49 {
	union {
		struct {
			unsigned char f54_query49_b0__1:2;
			unsigned char has_ctrl188:1;
			unsigned char has_data31:1;
			unsigned char f54_query49_b4__6:3;
			unsigned char has_query50:1;
		} __packed;
		unsigned char data[1];
	};
};

struct f54_query_50 {
	union {
		struct {
			unsigned char f54_query50_b0__6:7;
			unsigned char has_query51:1;
		} __packed;
		unsigned char data[1];
	};
};

struct f54_query_51 {
	union {
		struct {
			unsigned char f54_query51_b0__4:5;
			unsigned char has_query53_query54_ctrl198:1;
			unsigned char f54_query51_b6__7:2;
		} __packed;
		unsigned char data[1];
	};
};

struct f54_data_31 {
	union {
		struct {
			unsigned char is_calibration_crc:1;
			unsigned char calibration_crc:1;
			unsigned char short_test_row_number:5;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_0 {
	union {
		struct {
			unsigned char no_relax:1;
			unsigned char no_scan:1;
			unsigned char force_fast_relaxation:1;
			unsigned char startup_fast_relaxation:1;
			unsigned char gesture_cancels_sfr:1;
			unsigned char enable_energy_ratio_relaxation:1;
			unsigned char excessive_noise_attn_enable:1;
			unsigned char f54_control0_b7:1;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_1 {
	union {
		struct {
			unsigned char bursts_per_cluster:4;
			unsigned char f54_ctrl1_b4__7:4;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_2 {
	union {
		struct {
			unsigned short saturation_cap;
		} __packed;
		struct {
			unsigned char data[2];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_3 {
	union {
		struct {
			unsigned char pixel_touch_threshold;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_4__6 {
	union {
		struct {
			/* control 4 */
			unsigned char rx_feedback_cap:2;
			unsigned char bias_current:2;
			unsigned char f54_ctrl4_b4__7:4;

			/* control 5 */
			unsigned char low_ref_cap:2;
			unsigned char low_ref_feedback_cap:2;
			unsigned char low_ref_polarity:1;
			unsigned char f54_ctrl5_b5__7:3;

			/* control 6 */
			unsigned char high_ref_cap:2;
			unsigned char high_ref_feedback_cap:2;
			unsigned char high_ref_polarity:1;
			unsigned char f54_ctrl6_b5__7:3;
		} __packed;
		struct {
			unsigned char data[3];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_7 {
	union {
		struct {
			unsigned char cbc_cap:3;
			unsigned char cbc_polarity:1;
			unsigned char cbc_tx_carrier_selection:1;
			unsigned char f54_ctrl7_b5__7:3;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_8__9 {
	union {
		struct {
			/* control 8 */
			unsigned short integration_duration:10;
			unsigned short f54_ctrl8_b10__15:6;

			/* control 9 */
			unsigned char reset_duration;
		} __packed;
		struct {
			unsigned char data[3];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_10 {
	union {
		struct {
			unsigned char noise_sensing_bursts_per_image:4;
			unsigned char f54_ctrl10_b4__7:4;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_11 {
	union {
		struct {
			unsigned short f54_ctrl11;
		} __packed;
		struct {
			unsigned char data[2];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_12__13 {
	union {
		struct {
			/* control 12 */
			unsigned char slow_relaxation_rate;

			/* control 13 */
			unsigned char fast_relaxation_rate;
		} __packed;
		struct {
			unsigned char data[2];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_14 {
	union {
		struct {
			unsigned char rxs_on_xaxis:1;
			unsigned char curve_comp_on_txs:1;
			unsigned char f54_ctrl14_b2__7:6;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_15n {
	unsigned char sensor_rx_assignment;
};

struct f54_control_15 {
	struct f54_control_15n *data;
	unsigned short address;
	unsigned char length;
};

struct f54_control_16n {
	unsigned char sensor_tx_assignment;
};

struct f54_control_16 {
	struct f54_control_16n *data;
	unsigned short address;
	unsigned char length;
};

struct f54_control_17n {
	unsigned char burst_count_b8__10:3;
	unsigned char disable:1;
	unsigned char f54_ctrl17_b4:1;
	unsigned char filter_bandwidth:3;
};

struct f54_control_17 {
	struct f54_control_17n *data;
	unsigned short address;
	unsigned char length;
};

struct f54_control_18n {
	unsigned char burst_count_b0__7;
};

struct f54_control_18 {
	struct f54_control_18n *data;
	unsigned short address;
	unsigned char length;
};

struct f54_control_19n {
	unsigned char stretch_duration;
};

struct f54_control_19 {
	struct f54_control_19n *data;
	unsigned short address;
	unsigned char length;
};

struct f54_control_20 {
	union {
		struct {
			unsigned char disable_noise_mitigation:1;
			unsigned char f54_ctrl20_b1__7:7;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_21 {
	union {
		struct {
			unsigned short freq_shift_noise_threshold;
		} __packed;
		struct {
			unsigned char data[2];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_22__26 {
	union {
		struct {
			/* control 22 */
			unsigned char f54_ctrl22;

			/* control 23 */
			unsigned short medium_noise_threshold;

			/* control 24 */
			unsigned short high_noise_threshold;

			/* control 25 */
			unsigned char noise_density;

			/* control 26 */
			unsigned char frame_count;
		} __packed;
		struct {
			unsigned char data[7];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_27 {
	union {
		struct {
			unsigned char iir_filter_coef;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_28 {
	union {
		struct {
			unsigned short quiet_threshold;
		} __packed;
		struct {
			unsigned char data[2];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_29 {
	union {
		struct {
			/* control 29 */
			unsigned char f54_ctrl29_b0__6:7;
			unsigned char cmn_filter_disable:1;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_30 {
	union {
		struct {
			unsigned char cmn_filter_max;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_31 {
	union {
		struct {
			unsigned char touch_hysteresis;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_32__35 {
	union {
		struct {
			/* control 32 */
			unsigned short rx_low_edge_comp;

			/* control 33 */
			unsigned short rx_high_edge_comp;

			/* control 34 */
			unsigned short tx_low_edge_comp;

			/* control 35 */
			unsigned short tx_high_edge_comp;
		} __packed;
		struct {
			unsigned char data[8];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_36n {
	unsigned char axis1_comp;
};

struct f54_control_36 {
	struct f54_control_36n *data;
	unsigned short address;
	unsigned char length;
};

struct f54_control_37n {
	unsigned char axis2_comp;
};

struct f54_control_37 {
	struct f54_control_37n *data;
	unsigned short address;
	unsigned char length;
};

struct f54_control_38n {
	unsigned char noise_control_1;
};

struct f54_control_38 {
	struct f54_control_38n *data;
	unsigned short address;
	unsigned char length;
};

struct f54_control_39n {
	unsigned char noise_control_2;
};

struct f54_control_39 {
	struct f54_control_39n *data;
	unsigned short address;
	unsigned char length;
};

struct f54_control_40n {
	unsigned char noise_control_3;
};

struct f54_control_40 {
	struct f54_control_40n *data;
	unsigned short address;
	unsigned char length;
};

struct f54_control_41 {
	union {
		struct {
			unsigned char no_signal_clarity:1;
			unsigned char f54_ctrl41_b1__7:7;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_57 {
	union {
		struct {
			unsigned char cbc_cap_0d:3;
			unsigned char cbc_polarity_0d:1;
			unsigned char cbc_tx_carrier_selection_0d:1;
			unsigned char f54_ctrl57_b5__7:3;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_86 {
	union {
		struct {
			unsigned char enable_high_noise_state:1;
			unsigned char dynamic_sense_display_ratio:2;
			unsigned char f54_ctrl86_b3__7:5;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_88 {
	union {
		struct {
			unsigned char tx_low_reference_polarity:1;
			unsigned char tx_high_reference_polarity:1;
			unsigned char abs_low_reference_polarity:1;
			unsigned char abs_polarity:1;
			unsigned char cbc_polarity:1;
			unsigned char cbc_tx_carrier_selection:1;
			unsigned char charge_pump_enable:1;
			unsigned char cbc_abs_auto_servo:1;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_91 {
	union {
		struct {
			unsigned char reflo_transcap_capacitance;
			unsigned char refhi_transcap_capacitance;
			unsigned char receiver_feedback_capacitance;
			unsigned char reference_receiver_feedback_capacitance;
			unsigned char gain_ctrl;
		} __packed;
		struct {
			unsigned char data[5];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_94 {
	union {
		struct {
			unsigned char abs_rx_bursts_per_cluster;
			unsigned char abs_tx_bursts_per_cluster;
			unsigned char trans_cap_bursts_per_cluster;
			unsigned char noise_bursts_per_cluster;
		} __packed;
		struct {
			unsigned char data[4];
			unsigned short address;
		} __packed;
	};
};

/* syna customized for sec 20160819 + */
struct f54_control_95 {
	union {
		struct {
			unsigned char freq_ctrl_00[11];
			unsigned char freq_ctrl_01[11];
			unsigned char freq_ctrl_02[11];
			unsigned char freq_ctrl_03[11];
			unsigned char freq_ctrl_04[11];
			unsigned char freq_ctrl_05[11];
			unsigned char freq_ctrl_06[11];
			unsigned char freq_ctrl_07[11];	
			unsigned char freq_ctrl_08[11];
			unsigned char freq_ctrl_09[11];
			unsigned char freq_ctrl_10[11];
			unsigned char freq_ctrl_11[11];				
		} __packed;
		struct {
			unsigned char data[132];
			unsigned short address;
		} __packed;
	};
};
/* syna customized for sec 20160819 - */

struct f54_control_96 {
	union {
		struct {
			unsigned char cbc_transcap[64];
		} __packed;
		struct {
			unsigned char data[64];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_99 {
	union {
		struct {
			unsigned char integration_duration_lsb;
			unsigned char integration_duration_msb;
			unsigned char reset_duration;
		} __packed;
		struct {
			unsigned char data[3];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_110 {
	union {
		struct {
			unsigned char active_stylus_rx_feedback_cap;
			unsigned char active_stylus_rx_feedback_cap_reference;
			unsigned char active_stylus_low_reference;
			unsigned char active_stylus_high_reference;
			unsigned char active_stylus_gain_control;
			unsigned char active_stylus_gain_control_reference;
			unsigned char active_stylus_timing_mode;
			unsigned char active_stylus_discovery_bursts;
			unsigned char active_stylus_detection_bursts;
			unsigned char active_stylus_discovery_noise_multiplier;
			unsigned char active_stylus_detection_envelope_min;
			unsigned char active_stylus_detection_envelope_max;
			unsigned char active_stylus_lose_count;
		} __packed;
		struct {
			unsigned char data[13];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_149 {
	union {
		struct {
			unsigned char trans_cbc_global_cap_enable:1;
			unsigned char f54_ctrl149_b1__7:7;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_188 {
	union {
		struct {
			unsigned char start_calibration:1;
			unsigned char start_is_calibration:1;
			unsigned char frequency:2;
			unsigned char start_production_test:1;
			unsigned char short_test_calibration:1;
			unsigned char f54_ctrl188_b7:1;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control {
	struct f54_control_0 *reg_0;
	struct f54_control_1 *reg_1;
	struct f54_control_2 *reg_2;
	struct f54_control_3 *reg_3;
	struct f54_control_4__6 *reg_4__6;
	struct f54_control_7 *reg_7;
	struct f54_control_8__9 *reg_8__9;
	struct f54_control_10 *reg_10;
	struct f54_control_11 *reg_11;
	struct f54_control_12__13 *reg_12__13;
	struct f54_control_14 *reg_14;
	struct f54_control_15 *reg_15;
	struct f54_control_16 *reg_16;
	struct f54_control_17 *reg_17;
	struct f54_control_18 *reg_18;
	struct f54_control_19 *reg_19;
	struct f54_control_20 *reg_20;
	struct f54_control_21 *reg_21;
	struct f54_control_22__26 *reg_22__26;
	struct f54_control_27 *reg_27;
	struct f54_control_28 *reg_28;
	struct f54_control_29 *reg_29;
	struct f54_control_30 *reg_30;
	struct f54_control_31 *reg_31;
	struct f54_control_32__35 *reg_32__35;
	struct f54_control_36 *reg_36;
	struct f54_control_37 *reg_37;
	struct f54_control_38 *reg_38;
	struct f54_control_39 *reg_39;
	struct f54_control_40 *reg_40;
	struct f54_control_41 *reg_41;
	struct f54_control_57 *reg_57;
	struct f54_control_86 *reg_86;
	struct f54_control_88 *reg_88;
	struct f54_control_91 *reg_91;
	struct f54_control_94 *reg_94;
	struct f54_control_95 *reg_95;	/* syna customized for sec 20160819 + */
	struct f54_control_96 *reg_96;
    struct f54_control_99 *reg_99;
	struct f54_control_110 *reg_110;
	struct f54_control_149 *reg_149;
	struct f54_control_188 *reg_188;	
};

struct f55_query {
	union {
		struct {
			/* query 0 */
			unsigned char num_of_rx_electrodes;

			/* query 1 */
			unsigned char num_of_tx_electrodes;

			/* query 2 */
			unsigned char has_sensor_assignment:1;
			unsigned char has_edge_compensation:1;
			unsigned char curve_compensation_mode:2;
			unsigned char has_ctrl6:1;
			unsigned char has_alternate_transmitter_assignment:1;
			unsigned char has_single_layer_multi_touch:1;
			unsigned char has_query5:1;
		} __packed;
		unsigned char data[3];
	};
};

struct f55_query_3 {
	union {
		struct {
			unsigned char has_ctrl8:1;
			unsigned char has_ctrl9:1;
			unsigned char has_oncell_pattern_support:1;
			unsigned char has_data0:1;
			unsigned char has_single_wide_pattern_support:1;
			unsigned char has_mirrored_tx_pattern_support:1;
			unsigned char has_discrete_pattern_support:1;
			unsigned char has_query9:1;
		} __packed;
		unsigned char data[1];
	};
};

struct f55_query_5 {
	union {
		struct {
			unsigned char has_corner_compensation:1;
			unsigned char has_ctrl12:1;
			unsigned char has_trx_configuration:1;
			unsigned char has_ctrl13:1;
			unsigned char f55_query5_b4:1;
			unsigned char has_ctrl14:1;
			unsigned char has_basis_function:1;
			unsigned char has_query17:1;
		} __packed;
		unsigned char data[1];
	};
};

struct f55_query_17 {
	union {
		struct {
			unsigned char f55_query17_b0:1;
			unsigned char has_ctrl16:1;
			unsigned char has_ctrl18_ctrl19:1;
			unsigned char has_ctrl17:1;
			unsigned char has_ctrl20:1;
			unsigned char has_ctrl21:1;
			unsigned char has_ctrl22:1;
			unsigned char has_query18:1;
		} __packed;
		unsigned char data[1];
	};
};

struct f55_query_18 {
	union {
		struct {
			unsigned char has_ctrl23:1;
			unsigned char has_ctrl24:1;
			unsigned char has_query19:1;
			unsigned char has_ctrl25:1;
			unsigned char has_ctrl26:1;
			unsigned char has_ctrl27_query20:1;
			unsigned char has_ctrl28_query21:1;
			unsigned char has_query22:1;
		} __packed;
		unsigned char data[1];
	};
};

struct f55_query_22 {
	union {
		struct {
			unsigned char has_ctrl29:1;
			unsigned char has_query23:1;
			unsigned char has_guard_disable:1;
			unsigned char has_ctrl30:1;
			unsigned char has_ctrl31:1;
			unsigned char has_ctrl32:1;
			unsigned char has_query24_through_query27:1;
			unsigned char has_query28:1;
		} __packed;
		unsigned char data[1];
	};
};

struct f55_query_23 {
	union {
		struct {
			unsigned char amp_sensor_enabled:1;
			unsigned char image_transposed:1;
			unsigned char first_column_at_left_side:1;
			unsigned char size_of_column2mux:5;
		} __packed;
		unsigned char data[1];
	};
};

struct f55_query_28 {
	union {
		struct {
			unsigned char f55_query28_b0__4:5;
			unsigned char has_ctrl37:1;
			unsigned char has_query29:1;
			unsigned char has_query30:1;
		} __packed;
		unsigned char data[1];
	};
};

struct f55_query_30 {
	union {
		struct {
			unsigned char has_ctrl38:1;
			unsigned char has_query31_query32:1;
			unsigned char has_ctrl39:1;
			unsigned char has_ctrl40:1;
			unsigned char has_ctrl41:1;
			unsigned char has_ctrl42:1;
			unsigned char has_ctrl43_ctrl44:1;
			unsigned char has_query33:1;
		} __packed;
		unsigned char data[1];
	};
};

struct f55_query_33 {
	union {
		struct {
			unsigned char has_extended_amp_pad:1;
			unsigned char has_extended_amp_btn:1;
			unsigned char has_ctrl45_ctrl46:1;
			unsigned char f55_query33_b3:1;
			unsigned char has_ctrl47_sub0_sub1:1;
			unsigned char f55_query33_b5__7:3;
		} __packed;
		unsigned char data[1];
	};
};

struct f55_control_43 {
	union {
		struct {
			unsigned char swap_sensor_side:1;
			unsigned char f55_ctrl43_b1__7:7;
			unsigned char afe_l_mux_size:4;
			unsigned char afe_r_mux_size:4;
		} __packed;
		unsigned char data[2];
	};
};

