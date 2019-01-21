/*
 * Synaptics DSX touchscreen driver
 *
 * Copyright (C) 2012-2015 Synaptics Incorporated. All rights reserved.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * INFORMATION CONTAINED IN THIS DOCUMENT IS PROVIDED "AS-IS," AND SYNAPTICS
 * EXPRESSLY DISCLAIMS ALL EXPRESS AND IMPLIED WARRANTIES, INCLUDING ANY
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE,
 * AND ANY WARRANTIES OF NON-INFRINGEMENT OF ANY INTELLECTUAL PROPERTY RIGHTS.
 * IN NO EVENT SHALL SYNAPTICS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, PUNITIVE, OR CONSEQUENTIAL DAMAGES ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OF THE INFORMATION CONTAINED IN THIS DOCUMENT, HOWEVER CAUSED
 * AND BASED ON ANY THEORY OF LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, AND EVEN IF SYNAPTICS WAS ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE. IF A TRIBUNAL OF COMPETENT JURISDICTION DOES
 * NOT PERMIT THE DISCLAIMER OF DIRECT DAMAGES OR ANY OTHER DAMAGES, SYNAPTICS'
 * TOTAL CUMULATIVE LIABILITY TO ANY PARTY SHALL NOT EXCEED ONE HUNDRED U.S.
 * DOLLARS.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/ctype.h>
#include <linux/hrtimer.h>
#include <linux/platform_device.h>
#include <linux/input/synaptics_dsx_v2_6.h>
#include "synaptics_dsx_core.h"
#include "registerMap.h"
#include <linux/sec_class.h>

#define SYSFS_FOLDER_NAME "f54"

#define GET_REPORT_TIMEOUT_S 3
#define CALIBRATION_TIMEOUT_S 10
#define COMMAND_TIMEOUT_100MS 20

#define NO_SLEEP_OFF (0 << 2)
#define NO_SLEEP_ON (1 << 2)

#define STATUS_IDLE 0
#define STATUS_BUSY 1
#define STATUS_ERROR 2

#define REPORT_INDEX_OFFSET 1
#define REPORT_DATA_OFFSET 3

#define SENSOR_RX_MAPPING_OFFSET 1
#define SENSOR_TX_MAPPING_OFFSET 2

#define COMMAND_GET_REPORT 1
#define COMMAND_FORCE_CAL 2
#define COMMAND_FORCE_UPDATE 4

#define CONTROL_NO_AUTO_CAL 1

#define CONTROL_0_SIZE 1
#define CONTROL_1_SIZE 1
#define CONTROL_2_SIZE 2
#define CONTROL_3_SIZE 1
#define CONTROL_4_6_SIZE 3
#define CONTROL_7_SIZE 1
#define CONTROL_8_9_SIZE 3
#define CONTROL_10_SIZE 1
#define CONTROL_11_SIZE 2
#define CONTROL_12_13_SIZE 2
#define CONTROL_14_SIZE 1
#define CONTROL_15_SIZE 1
#define CONTROL_16_SIZE 1
#define CONTROL_17_SIZE 1
#define CONTROL_18_SIZE 1
#define CONTROL_19_SIZE 1
#define CONTROL_20_SIZE 1
#define CONTROL_21_SIZE 2
#define CONTROL_22_26_SIZE 7
#define CONTROL_27_SIZE 1
#define CONTROL_28_SIZE 2
#define CONTROL_29_SIZE 1
#define CONTROL_30_SIZE 1
#define CONTROL_31_SIZE 1
#define CONTROL_32_35_SIZE 8
#define CONTROL_36_SIZE 1
#define CONTROL_37_SIZE 1
#define CONTROL_38_SIZE 1
#define CONTROL_39_SIZE 1
#define CONTROL_40_SIZE 1
#define CONTROL_41_SIZE 1
#define CONTROL_42_SIZE 2
#define CONTROL_43_54_SIZE 13
#define CONTROL_55_56_SIZE 2
#define CONTROL_57_SIZE 1
#define CONTROL_58_SIZE 1
#define CONTROL_59_SIZE 2
#define CONTROL_60_62_SIZE 3
#define CONTROL_63_SIZE 1
#define CONTROL_64_67_SIZE 4
#define CONTROL_68_73_SIZE 8
#define CONTROL_74_SIZE 2
#define CONTROL_75_SIZE 1
#define CONTROL_76_SIZE 1
#define CONTROL_77_78_SIZE 2
#define CONTROL_79_83_SIZE 5
#define CONTROL_84_85_SIZE 2
#define CONTROL_86_SIZE 1
#define CONTROL_87_SIZE 1
#define CONTROL_88_SIZE 1
#define CONTROL_89_SIZE 1
#define CONTROL_90_SIZE 1
#define CONTROL_91_SIZE 1
#define CONTROL_92_SIZE 1
#define CONTROL_93_SIZE 1
#define CONTROL_94_SIZE 1
#define CONTROL_95_SIZE 1
#define CONTROL_96_SIZE 1
#define CONTROL_97_SIZE 1
#define CONTROL_98_SIZE 1
#define CONTROL_99_SIZE 1
#define CONTROL_100_SIZE 1
#define CONTROL_101_SIZE 1
#define CONTROL_102_SIZE 1
#define CONTROL_103_SIZE 1
#define CONTROL_104_SIZE 1
#define CONTROL_105_SIZE 1
#define CONTROL_106_SIZE 1
#define CONTROL_107_SIZE 1
#define CONTROL_108_SIZE 1
#define CONTROL_109_SIZE 1
#define CONTROL_110_SIZE 1
#define CONTROL_111_SIZE 1
#define CONTROL_112_SIZE 1
#define CONTROL_113_SIZE 1
#define CONTROL_114_SIZE 1
#define CONTROL_115_SIZE 1
#define CONTROL_116_SIZE 1
#define CONTROL_117_SIZE 1
#define CONTROL_118_SIZE 1
#define CONTROL_119_SIZE 1
#define CONTROL_120_SIZE 1
#define CONTROL_121_SIZE 1
#define CONTROL_122_SIZE 1
#define CONTROL_123_SIZE 1
#define CONTROL_124_SIZE 1
#define CONTROL_125_SIZE 1
#define CONTROL_126_SIZE 1
#define CONTROL_127_SIZE 1
#define CONTROL_128_SIZE 1
#define CONTROL_129_SIZE 1
#define CONTROL_130_SIZE 1
#define CONTROL_131_SIZE 1
#define CONTROL_132_SIZE 1
#define CONTROL_133_SIZE 1
#define CONTROL_134_SIZE 1
#define CONTROL_135_SIZE 1
#define CONTROL_136_SIZE 1
#define CONTROL_137_SIZE 1
#define CONTROL_138_SIZE 1
#define CONTROL_139_SIZE 1
#define CONTROL_140_SIZE 1
#define CONTROL_141_SIZE 1
#define CONTROL_142_SIZE 1
#define CONTROL_143_SIZE 1
#define CONTROL_144_SIZE 1
#define CONTROL_145_SIZE 1
#define CONTROL_146_SIZE 1
#define CONTROL_147_SIZE 1
#define CONTROL_148_SIZE 1
#define CONTROL_149_SIZE 1
#define CONTROL_163_SIZE 1
#define CONTROL_165_SIZE 1
#define CONTROL_166_SIZE 1
#define CONTROL_167_SIZE 1
#define CONTROL_176_SIZE 1
#define CONTROL_179_SIZE 1
#define CONTROL_188_SIZE 1

#define HIGH_RESISTANCE_DATA_SIZE 6
#define FULL_RAW_CAP_MIN_MAX_DATA_SIZE 4
#define TRX_OPEN_SHORT_DATA_SIZE 7
#define TSP_RAWCAP_MAX	6000
#define TSP_RAWCAP_MIN	300

#define concat(a, b) a##b

#define attrify(propname) (&dev_attr_##propname.attr)

#define show_prototype(propname)\
static ssize_t concat(test_sysfs, _##propname##_show)(\
		struct device *dev,\
		struct device_attribute *attr,\
		char *buf);\
\
static struct device_attribute dev_attr_##propname =\
		__ATTR(propname, S_IRUGO,\
		concat(test_sysfs, _##propname##_show),\
		NULL);

#define store_prototype(propname)\
static ssize_t concat(test_sysfs, _##propname##_store)(\
		struct device *dev,\
		struct device_attribute *attr,\
		const char *buf, size_t count);\
\
static struct device_attribute dev_attr_##propname =\
		__ATTR(propname, (S_IWUSR | S_IWGRP),\
		NULL,\
		concat(test_sysfs, _##propname##_store));

#define show_store_prototype(propname)\
static ssize_t concat(test_sysfs, _##propname##_show)(\
		struct device *dev,\
		struct device_attribute *attr,\
		char *buf);\
\
static ssize_t concat(test_sysfs, _##propname##_store)(\
		struct device *dev,\
		struct device_attribute *attr,\
		const char *buf, size_t count);\
\
static struct device_attribute dev_attr_##propname =\
		__ATTR(propname, (S_IRUGO | S_IWUSR | S_IWGRP),\
		concat(test_sysfs, _##propname##_show),\
		concat(test_sysfs, _##propname##_store));

#define disable_cbc(ctrl_num)\
do {\
	retval = synaptics_rmi4_reg_read(rmi4_data,\
			f54->control.ctrl_num->address,\
			f54->control.ctrl_num->data,\
			sizeof(f54->control.ctrl_num->data));\
	if (retval < 0) {\
		dev_err(rmi4_data->pdev->dev.parent,\
				"%s: Failed to disable CBC (" #ctrl_num ")\n",\
				__func__);\
		return retval;\
	} \
	f54->control.ctrl_num->cbc_tx_carrier_selection = 0;\
	retval = synaptics_rmi4_reg_write(rmi4_data,\
			f54->control.ctrl_num->address,\
			f54->control.ctrl_num->data,\
			sizeof(f54->control.ctrl_num->data));\
	if (retval < 0) {\
		dev_err(rmi4_data->pdev->dev.parent,\
				"%s: Failed to disable CBC (" #ctrl_num ")\n",\
				__func__);\
		return retval;\
	} \
} while (0)

enum f54_report_types {
	F54_8BIT_IMAGE = 1,
	F54_16BIT_IMAGE = 2,
	F54_RAW_16BIT_IMAGE = 3,
	F54_HIGH_RESISTANCE = 4,
	F54_TX_TO_TX_SHORTS = 5,
	F54_RX_TO_RX_SHORTS_1 = 7,
	F54_TRUE_BASELINE = 9,
	F54_FULL_RAW_CAP_MIN_MAX = 13,
	F54_RX_OPENS_1 = 14,
	F54_TX_OPENS = 15,
	F54_TX_TO_GND_SHORTS = 16,
	F54_RX_TO_RX_SHORTS_2 = 17,
	F54_RX_OPENS_2 = 18,
	F54_FULL_RAW_CAP = 19,
	F54_FULL_RAW_CAP_NO_RX_COUPLING = 20,
	F54_SENSOR_SPEED = 22,
	F54_ADC_RANGE = 23,
	F54_TRX_OPENS = 24,
	F54_TRX_TO_GND_SHORTS = 25,
	F54_TRX_SHORTS = 26,
	F54_ABS_RAW_CAP = 38,
	F54_ABS_DELTA_CAP = 40,
	F54_ABS_HYBRID_DELTA_CAP = 59,
	F54_ABS_HYBRID_RAW_CAP = 63,
	F54_AMP_FULL_RAW_CAP = 78,
	F54_AMP_RAW_ADC = 83,
	F54_FULL_RAW_CAP_TDDI = 92,
	INVALID_REPORT_TYPE = -1,
};

enum f54_afe_cal {
	F54_AFE_CAL,
	F54_AFE_IS_CAL,
};

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
			unsigned char cbc_cap:3;
			unsigned char cbc_polarity:1;
			unsigned char cbc_tx_carrier_selection:1;
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
	struct f54_control_7 *reg_7;
	struct f54_control_41 *reg_41;
	struct f54_control_57 *reg_57;
	struct f54_control_86 *reg_86;
	struct f54_control_88 *reg_88;
	struct f54_control_110 *reg_110;
	struct f54_control_149 *reg_149;
	struct f54_control_188 *reg_188;
};

#ifdef CONFIG_DRV_SAMSUNG

#define CMD_STR_LEN			32
#define CMD_RESULT_STR_LEN		2048
#define CMD_PARAM_NUM			8

struct cmd_data {
	struct device *ts_dev;
	short *rawcap_data;
	short *delta_data;
	int *abscap_data;
	int *absdelta_data;
	char *trx_short;
	unsigned int abscap_rx_min;
	unsigned int abscap_rx_max;
	unsigned int abscap_tx_min;
	unsigned int abscap_tx_max;
	bool cmd_is_running;
	unsigned char cmd_state;
	char cmd[CMD_STR_LEN];
	int cmd_param[CMD_PARAM_NUM];
	char cmd_buff[CMD_RESULT_STR_LEN];
	char cmd_result[CMD_RESULT_STR_LEN];
	int cmd_list_size;
	struct mutex cmd_lock;
	struct list_head cmd_list_head;
};
#endif

struct synaptics_rmi4_f54_handle {
	bool no_auto_cal;
	bool skip_preparation;
	unsigned char status;
	unsigned char intr_mask;
	unsigned char intr_reg_num;
	unsigned char tx_assigned;
	unsigned char rx_assigned;
	unsigned char *report_data;
	unsigned short query_base_addr;
	unsigned short control_base_addr;
	unsigned short data_base_addr;
	unsigned short command_base_addr;
	unsigned short fifoindex;
	unsigned int report_size;
	unsigned int data_buffer_size;
	unsigned int data_pos;
	enum f54_report_types report_type;
	struct f54_query query;
	struct f54_query_13 query_13;
	struct f54_query_15 query_15;
	struct f54_query_16 query_16;
	struct f54_query_21 query_21;
	struct f54_query_22 query_22;
	struct f54_query_23 query_23;
	struct f54_query_25 query_25;
	struct f54_query_27 query_27;
	struct f54_query_29 query_29;
	struct f54_query_30 query_30;
	struct f54_query_32 query_32;
	struct f54_query_33 query_33;
	struct f54_query_35 query_35;
	struct f54_query_36 query_36;
	struct f54_query_38 query_38;
	struct f54_query_39 query_39;
	struct f54_query_40 query_40;
	struct f54_query_43 query_43;
	struct f54_query_46 query_46;
	struct f54_query_47 query_47;
	struct f54_query_49 query_49;
	struct f54_query_50 query_50;
	struct f54_query_51 query_51;
	struct f54_data_31 data_31;
	struct f54_control control;
	struct mutex status_mutex;
	struct kobject *sysfs_dir;
	struct hrtimer watchdog;
	struct work_struct timeout_work;
	struct work_struct test_report_work;
	struct workqueue_struct *test_report_workqueue;
	struct synaptics_rmi4_data *rmi4_data;
#ifdef CONFIG_DRV_SAMSUNG
	struct cmd_data *cmd_data;
#endif
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

struct synaptics_rmi4_f55_handle {
	bool amp_sensor;
	bool extended_amp;
	bool has_force;
	unsigned char size_of_column2mux;
	unsigned char afe_mux_offset;
	unsigned char force_tx_offset;
	unsigned char force_rx_offset;
	unsigned char *tx_assignment;
	unsigned char *rx_assignment;
	unsigned char *force_tx_assignment;
	unsigned char *force_rx_assignment;
	unsigned short query_base_addr;
	unsigned short control_base_addr;
	unsigned short data_base_addr;
	unsigned short command_base_addr;
	struct f55_query query;
	struct f55_query_3 query_3;
	struct f55_query_5 query_5;
	struct f55_query_17 query_17;
	struct f55_query_18 query_18;
	struct f55_query_22 query_22;
	struct f55_query_23 query_23;
	struct f55_query_28 query_28;
	struct f55_query_30 query_30;
	struct f55_query_33 query_33;
};

struct f21_query_2 {
	union {
		struct {
			unsigned char size_of_query3;
			struct {
				unsigned char query0_is_present:1;
				unsigned char query1_is_present:1;
				unsigned char query2_is_present:1;
				unsigned char query3_is_present:1;
				unsigned char query4_is_present:1;
				unsigned char query5_is_present:1;
				unsigned char query6_is_present:1;
				unsigned char query7_is_present:1;
			} __packed;
			struct {
				unsigned char query8_is_present:1;
				unsigned char query9_is_present:1;
				unsigned char query10_is_present:1;
				unsigned char query11_is_present:1;
				unsigned char query12_is_present:1;
				unsigned char query13_is_present:1;
				unsigned char query14_is_present:1;
				unsigned char query15_is_present:1;
			} __packed;
		};
		unsigned char data[3];
	};
};

struct f21_query_5 {
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
		};
		unsigned char data[4];
	};
};

struct f21_query_11 {
	union {
		struct {
			unsigned char has_high_resolution_force:1;
			unsigned char has_force_sensing_txrx_mapping:1;
			unsigned char f21_query11_00_b2__7:6;
			unsigned char f21_query11_00_reserved;
			unsigned char max_number_of_force_sensors;
			unsigned char max_number_of_force_txs;
			unsigned char max_number_of_force_rxs;
			unsigned char f21_query11_01_reserved;
		} __packed;
		unsigned char data[6];
	};
};

struct synaptics_rmi4_f21_handle {
	bool has_force;
	unsigned char max_num_of_tx;
	unsigned char max_num_of_rx;
	unsigned char max_num_of_txrx;
	unsigned char *force_txrx_assignment;
	unsigned short query_base_addr;
	unsigned short control_base_addr;
	unsigned short data_base_addr;
	unsigned short command_base_addr;
};

show_prototype(num_of_mapped_tx)
show_prototype(num_of_mapped_rx)
show_prototype(tx_mapping)
show_prototype(rx_mapping)
show_prototype(force_tx_mapping)
show_prototype(force_rx_mapping)
show_prototype(report_size)
show_prototype(status)
store_prototype(do_preparation)
store_prototype(force_cal)
store_prototype(get_report)
store_prototype(resume_touch)
store_prototype(do_afe_calibration)
show_store_prototype(report_type)
show_store_prototype(fifoindex)
show_store_prototype(no_auto_cal)
show_store_prototype(read_report)

static struct attribute *attrs[] = {
	attrify(num_of_mapped_tx),
	attrify(num_of_mapped_rx),
	attrify(tx_mapping),
	attrify(rx_mapping),
	attrify(force_tx_mapping),
	attrify(force_rx_mapping),
	attrify(report_size),
	attrify(status),
	attrify(do_preparation),
	attrify(force_cal),
	attrify(get_report),
	attrify(resume_touch),
	attrify(do_afe_calibration),
	attrify(report_type),
	attrify(fifoindex),
	attrify(no_auto_cal),
	attrify(read_report),
	NULL,
};

static struct attribute_group attr_group = {
	.attrs = attrs,
};

static ssize_t test_sysfs_data_read(struct file *data_file,
		struct kobject *kobj, struct bin_attribute *attributes,
		char *buf, loff_t pos, size_t count);

static struct bin_attribute test_report_data = {
	.attr = {
		.name = "report_data",
		.mode = S_IRUGO,
	},
	.size = 0,
	.read = test_sysfs_data_read,
};

static struct synaptics_rmi4_f54_handle *f54;
static struct synaptics_rmi4_f55_handle *f55;
static struct synaptics_rmi4_f21_handle *f21;

DECLARE_COMPLETION(test_remove_complete);

static bool test_report_type_valid(enum f54_report_types report_type)
{
	switch (report_type) {
	case F54_8BIT_IMAGE:
	case F54_16BIT_IMAGE:
	case F54_RAW_16BIT_IMAGE:
	case F54_HIGH_RESISTANCE:
	case F54_TX_TO_TX_SHORTS:
	case F54_RX_TO_RX_SHORTS_1:
	case F54_TRUE_BASELINE:
	case F54_FULL_RAW_CAP_MIN_MAX:
	case F54_RX_OPENS_1:
	case F54_TX_OPENS:
	case F54_TX_TO_GND_SHORTS:
	case F54_RX_TO_RX_SHORTS_2:
	case F54_RX_OPENS_2:
	case F54_FULL_RAW_CAP:
	case F54_FULL_RAW_CAP_NO_RX_COUPLING:
	case F54_SENSOR_SPEED:
	case F54_ADC_RANGE:
	case F54_TRX_OPENS:
	case F54_TRX_TO_GND_SHORTS:
	case F54_TRX_SHORTS:
	case F54_ABS_RAW_CAP:
	case F54_ABS_DELTA_CAP:
	case F54_ABS_HYBRID_DELTA_CAP:
	case F54_ABS_HYBRID_RAW_CAP:
	case F54_AMP_FULL_RAW_CAP:
	case F54_AMP_RAW_ADC:
	case F54_FULL_RAW_CAP_TDDI:
		return true;
		break;
	default:
		f54->report_type = INVALID_REPORT_TYPE;
		f54->report_size = 0;
		return false;
	}
}

static void test_set_report_size(void)
{
	int retval;
	unsigned char tx = f54->tx_assigned;
	unsigned char rx = f54->rx_assigned;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	switch (f54->report_type) {
	case F54_8BIT_IMAGE:
		f54->report_size = tx * rx;
		break;
	case F54_16BIT_IMAGE:
	case F54_RAW_16BIT_IMAGE:
	case F54_TRUE_BASELINE:
	case F54_FULL_RAW_CAP:
	case F54_FULL_RAW_CAP_NO_RX_COUPLING:
	case F54_SENSOR_SPEED:
	case F54_AMP_FULL_RAW_CAP:
	case F54_AMP_RAW_ADC:
	case F54_FULL_RAW_CAP_TDDI:
		f54->report_size = 2 * tx * rx;
		break;
	case F54_HIGH_RESISTANCE:
		f54->report_size = HIGH_RESISTANCE_DATA_SIZE;
		break;
	case F54_TX_TO_TX_SHORTS:
	case F54_TX_OPENS:
	case F54_TX_TO_GND_SHORTS:
		f54->report_size = (tx + 7) / 8;
		break;
	case F54_RX_TO_RX_SHORTS_1:
	case F54_RX_OPENS_1:
		if (rx < tx)
			f54->report_size = 2 * rx * rx;
		else
			f54->report_size = 2 * tx * rx;
		break;
	case F54_FULL_RAW_CAP_MIN_MAX:
		f54->report_size = FULL_RAW_CAP_MIN_MAX_DATA_SIZE;
		break;
	case F54_RX_TO_RX_SHORTS_2:
	case F54_RX_OPENS_2:
		if (rx <= tx)
			f54->report_size = 0;
		else
			f54->report_size = 2 * rx * (rx - tx);
		break;
	case F54_ADC_RANGE:
		if (f54->query.has_signal_clarity) {
			retval = synaptics_rmi4_reg_read(rmi4_data,
					f54->control.reg_41->address,
					f54->control.reg_41->data,
					sizeof(f54->control.reg_41->data));
			if (retval < 0) {
				dev_dbg(rmi4_data->pdev->dev.parent,
						"%s: Failed to read control reg_41\n",
						__func__);
				f54->report_size = 0;
				break;
			}
			if (!f54->control.reg_41->no_signal_clarity) {
				if (tx % 4)
					tx += 4 - (tx % 4);
			}
		}
		f54->report_size = 2 * tx * rx;
		break;
	case F54_TRX_OPENS:
	case F54_TRX_TO_GND_SHORTS:
	case F54_TRX_SHORTS:
		f54->report_size = TRX_OPEN_SHORT_DATA_SIZE;
		break;
	case F54_ABS_RAW_CAP:
	case F54_ABS_DELTA_CAP:
	case F54_ABS_HYBRID_DELTA_CAP:
	case F54_ABS_HYBRID_RAW_CAP:
		f54->report_size = 4 * (tx + rx);
		break;
	default:
		f54->report_size = 0;
	}

	return;
}

static int test_set_interrupt(bool set)
{
	int retval;
	unsigned char ii;
	unsigned char zero = 0x00;
	unsigned char *intr_mask;
	unsigned short f01_ctrl_reg;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	intr_mask = rmi4_data->intr_mask;
	f01_ctrl_reg = rmi4_data->f01_ctrl_base_addr + 1 + f54->intr_reg_num;

	if (!set) {
		retval = synaptics_rmi4_reg_write(rmi4_data,
				f01_ctrl_reg,
				&zero,
				sizeof(zero));
		if (retval < 0)
			return retval;
	}

	for (ii = 0; ii < rmi4_data->num_of_intr_regs; ii++) {
		if (intr_mask[ii] != 0x00) {
			f01_ctrl_reg = rmi4_data->f01_ctrl_base_addr + 1 + ii;
			if (set) {
				retval = synaptics_rmi4_reg_write(rmi4_data,
						f01_ctrl_reg,
						&zero,
						sizeof(zero));
				if (retval < 0)
					return retval;
			} else {
				retval = synaptics_rmi4_reg_write(rmi4_data,
						f01_ctrl_reg,
						&(intr_mask[ii]),
						sizeof(intr_mask[ii]));
				if (retval < 0)
					return retval;
			}
		}
	}

	f01_ctrl_reg = rmi4_data->f01_ctrl_base_addr + 1 + f54->intr_reg_num;

	if (set) {
		retval = synaptics_rmi4_reg_write(rmi4_data,
				f01_ctrl_reg,
				&f54->intr_mask,
				1);
		if (retval < 0)
			return retval;
	}

	return 0;
}

static int test_wait_for_command_completion(void)
{
	int retval;
	unsigned char value;
	unsigned char timeout_count;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	timeout_count = 0;
	do {
		retval = synaptics_rmi4_reg_read(rmi4_data,
				f54->command_base_addr,
				&value,
				sizeof(value));
		if (retval < 0) {
			dev_err(rmi4_data->pdev->dev.parent,
					"%s: Failed to read command register\n",
					__func__);
			return retval;
		}

		if (value == 0x00)
			break;

		msleep(100);
		timeout_count++;
	} while (timeout_count < COMMAND_TIMEOUT_100MS);

	if (timeout_count == COMMAND_TIMEOUT_100MS) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: Timed out waiting for command completion\n",
				__func__);
		return -ETIMEDOUT;
	}

	return 0;
}

static int test_do_command(unsigned char command)
{
	int retval;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	retval = synaptics_rmi4_reg_write(rmi4_data,
			f54->command_base_addr,
			&command,
			sizeof(command));
	if (retval < 0) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: Failed to write command\n",
				__func__);
		return retval;
	}

	retval = test_wait_for_command_completion();
	if (retval < 0)
		return retval;

	return 0;
}

static int test_do_preparation(void)
{
	int retval;
	unsigned char value;
	unsigned char zero = 0x00;
	unsigned char device_ctrl;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	retval = synaptics_rmi4_reg_read(rmi4_data,
			rmi4_data->f01_ctrl_base_addr,
			&device_ctrl,
			sizeof(device_ctrl));
	if (retval < 0) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: Failed to set no sleep\n",
				__func__);
		return retval;
	}

	device_ctrl |= NO_SLEEP_ON;

	retval = synaptics_rmi4_reg_write(rmi4_data,
			rmi4_data->f01_ctrl_base_addr,
			&device_ctrl,
			sizeof(device_ctrl));
	if (retval < 0) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: Failed to set no sleep\n",
				__func__);
		return retval;
	}

	if (f54->skip_preparation)
		return 0;

	switch (f54->report_type) {
	case F54_16BIT_IMAGE:
	case F54_RAW_16BIT_IMAGE:
	case F54_SENSOR_SPEED:
	case F54_ADC_RANGE:
	case F54_ABS_RAW_CAP:
	case F54_ABS_DELTA_CAP:
	case F54_ABS_HYBRID_DELTA_CAP:
	case F54_ABS_HYBRID_RAW_CAP:
	case F54_FULL_RAW_CAP_TDDI:
		break;
	case F54_AMP_RAW_ADC:
		if (f54->query_49.has_ctrl188) {
			retval = synaptics_rmi4_reg_read(rmi4_data,
					f54->control.reg_188->address,
					f54->control.reg_188->data,
					sizeof(f54->control.reg_188->data));
			if (retval < 0) {
				dev_err(rmi4_data->pdev->dev.parent,
						"%s: Failed to set start production test\n",
						__func__);
				return retval;
			}
			f54->control.reg_188->start_production_test = 1;
			retval = synaptics_rmi4_reg_write(rmi4_data,
					f54->control.reg_188->address,
					f54->control.reg_188->data,
					sizeof(f54->control.reg_188->data));
			if (retval < 0) {
				dev_err(rmi4_data->pdev->dev.parent,
						"%s: Failed to set start production test\n",
						__func__);
				return retval;
			}
		}
		break;
	default:
		if (f54->query.touch_controller_family == 1)
			disable_cbc(reg_7);
		else if (f54->query.has_ctrl88)
			disable_cbc(reg_88);

		if (f54->query.has_0d_acquisition_control)
			disable_cbc(reg_57);

		if ((f54->query.has_query15) &&
				(f54->query_15.has_query25) &&
				(f54->query_25.has_query27) &&
				(f54->query_27.has_query29) &&
				(f54->query_29.has_query30) &&
				(f54->query_30.has_query32) &&
				(f54->query_32.has_query33) &&
				(f54->query_33.has_query36) &&
				(f54->query_36.has_query38) &&
				(f54->query_38.has_ctrl149)) {
			retval = synaptics_rmi4_reg_write(rmi4_data,
					f54->control.reg_149->address,
					&zero,
					sizeof(f54->control.reg_149->data));
			if (retval < 0) {
				dev_err(rmi4_data->pdev->dev.parent,
						"%s: Failed to disable global CBC\n",
						__func__);
				return retval;
			}
		}

		if (f54->query.has_signal_clarity) {
			retval = synaptics_rmi4_reg_read(rmi4_data,
					f54->control.reg_41->address,
					&value,
					sizeof(f54->control.reg_41->data));
			if (retval < 0) {
				dev_err(rmi4_data->pdev->dev.parent,
						"%s: Failed to disable signal clarity\n",
						__func__);
				return retval;
			}
			value |= 0x01;
			retval = synaptics_rmi4_reg_write(rmi4_data,
					f54->control.reg_41->address,
					&value,
					sizeof(f54->control.reg_41->data));
			if (retval < 0) {
				dev_err(rmi4_data->pdev->dev.parent,
						"%s: Failed to disable signal clarity\n",
						__func__);
				return retval;
			}
		}

		retval = test_do_command(COMMAND_FORCE_UPDATE);
		if (retval < 0) {
			dev_err(rmi4_data->pdev->dev.parent,
					"%s: Failed to do force update\n",
					__func__);
			return retval;
		}

		retval = test_do_command(COMMAND_FORCE_CAL);
		if (retval < 0) {
			dev_err(rmi4_data->pdev->dev.parent,
					"%s: Failed to do force cal\n",
					__func__);
			return retval;
		}
	}

	return 0;
}

static int test_do_afe_calibration(enum f54_afe_cal mode)
{
	int retval;
	unsigned char timeout = CALIBRATION_TIMEOUT_S;
	unsigned char timeout_count = 0;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	retval = synaptics_rmi4_reg_read(rmi4_data,
			f54->control.reg_188->address,
			f54->control.reg_188->data,
			sizeof(f54->control.reg_188->data));
	if (retval < 0) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: Failed to start calibration\n",
				__func__);
		return retval;
	}

	if (mode == F54_AFE_CAL)
		f54->control.reg_188->start_calibration = 1;
	else if (mode == F54_AFE_IS_CAL)
		f54->control.reg_188->start_is_calibration = 1;

	retval = synaptics_rmi4_reg_write(rmi4_data,
			f54->control.reg_188->address,
			f54->control.reg_188->data,
			sizeof(f54->control.reg_188->data));
	if (retval < 0) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: Failed to start calibration\n",
				__func__);
		return retval;
	}

	do {
		retval = synaptics_rmi4_reg_read(rmi4_data,
				f54->control.reg_188->address,
				f54->control.reg_188->data,
				sizeof(f54->control.reg_188->data));
		if (retval < 0) {
			dev_err(rmi4_data->pdev->dev.parent,
					"%s: Failed to complete calibration\n",
					__func__);
			return retval;
		}

		if (mode == F54_AFE_CAL) {
			if (!f54->control.reg_188->start_calibration)
				break;
		} else if (mode == F54_AFE_IS_CAL) {
			if (!f54->control.reg_188->start_is_calibration)
				break;
		}

		if (timeout_count == timeout) {
			dev_err(rmi4_data->pdev->dev.parent,
					"%s: Timed out waiting for calibration completion\n",
					__func__);
			return -EBUSY;
		}

		timeout_count++;
		msleep(1000);
	} while (true);

	/* check CRC */
	retval = synaptics_rmi4_reg_read(rmi4_data,
			f54->data_31.address,
			f54->data_31.data,
			sizeof(f54->data_31.data));
	if (retval < 0) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: Failed to read calibration CRC\n",
				__func__);
		return retval;
	}

	if (mode == F54_AFE_CAL) {
		if (f54->data_31.calibration_crc == 0)
			return 0;
	} else if (mode == F54_AFE_IS_CAL) {
		if (f54->data_31.is_calibration_crc == 0)
			return 0;
	}

	dev_err(rmi4_data->pdev->dev.parent,
			"%s: Failed to read calibration CRC\n",
			__func__);

	return -EINVAL;
}

static int test_check_for_idle_status(void)
{
	int retval;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	switch (f54->status) {
	case STATUS_IDLE:
		retval = 0;
		break;
	case STATUS_BUSY:
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: Status busy\n",
				__func__);
		retval = -EINVAL;
		break;
	case STATUS_ERROR:
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: Status error\n",
				__func__);
		retval = -EINVAL;
		break;
	default:
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: Invalid status (%d)\n",
				__func__, f54->status);
		retval = -EINVAL;
	}

	return retval;
}

static void test_timeout_work(struct work_struct *work)
{
	int retval;
	unsigned char command;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	mutex_lock(&f54->status_mutex);

	if (f54->status == STATUS_BUSY) {
		retval = synaptics_rmi4_reg_read(rmi4_data,
				f54->command_base_addr,
				&command,
				sizeof(command));
		if (retval < 0) {
			dev_err(rmi4_data->pdev->dev.parent,
					"%s: Failed to read command register\n",
					__func__);
		} else if (command & COMMAND_GET_REPORT) {
			dev_err(rmi4_data->pdev->dev.parent,
					"%s: Report type not supported by FW\n",
					__func__);
		} else {
			queue_work(f54->test_report_workqueue,
					&f54->test_report_work);
			goto exit;
		}
		f54->status = STATUS_ERROR;
		f54->report_size = 0;
	}

exit:
	mutex_unlock(&f54->status_mutex);

	return;
}

static enum hrtimer_restart test_get_report_timeout(struct hrtimer *timer)
{
	schedule_work(&(f54->timeout_work));

	return HRTIMER_NORESTART;
}

static ssize_t test_sysfs_num_of_mapped_tx_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%u\n", f54->tx_assigned);
}

static ssize_t test_sysfs_num_of_mapped_rx_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%u\n", f54->rx_assigned);
}

static ssize_t test_sysfs_tx_mapping_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int cnt;
	int count = 0;
	unsigned char ii;
	unsigned char tx_num;
	unsigned char tx_electrodes;

	if (!f55)
		return -EINVAL;

	tx_electrodes = f55->query.num_of_tx_electrodes;

	for (ii = 0; ii < tx_electrodes; ii++) {
		tx_num = f55->tx_assignment[ii];
		if (tx_num == 0xff)
			cnt = snprintf(buf, PAGE_SIZE - count, "xx ");
		else
			cnt = snprintf(buf, PAGE_SIZE - count, "%02u ", tx_num);
		buf += cnt;
		count += cnt;
	}

	snprintf(buf, PAGE_SIZE - count, "\n");
	count++;

	return count;
}

static ssize_t test_sysfs_rx_mapping_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int cnt;
	int count = 0;
	unsigned char ii;
	unsigned char rx_num;
	unsigned char rx_electrodes;

	if (!f55)
		return -EINVAL;

	rx_electrodes = f55->query.num_of_rx_electrodes;

	for (ii = 0; ii < rx_electrodes; ii++) {
		rx_num = f55->rx_assignment[ii];
		if (rx_num == 0xff)
			cnt = snprintf(buf, PAGE_SIZE - count, "xx ");
		else
			cnt = snprintf(buf, PAGE_SIZE - count, "%02u ", rx_num);
		buf += cnt;
		count += cnt;
	}

	snprintf(buf, PAGE_SIZE - count, "\n");
	count++;

	return count;
}

static ssize_t test_sysfs_force_tx_mapping_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int cnt;
	int count = 0;
	unsigned char ii;
	unsigned char tx_num;
	unsigned char tx_electrodes;

	if ((!f55 || !f55->has_force) && (!f21 || !f21->has_force))
		return -EINVAL;

	if (f55->has_force) {
		tx_electrodes = f55->query.num_of_tx_electrodes;

		for (ii = 0; ii < tx_electrodes; ii++) {
			tx_num = f55->force_tx_assignment[ii];
			if (tx_num == 0xff) {
				cnt = snprintf(buf, PAGE_SIZE - count, "xx ");
			} else {
				cnt = snprintf(buf, PAGE_SIZE - count, "%02u ",
						tx_num);
			}
			buf += cnt;
			count += cnt;
		}
	} else if (f21->has_force) {
		tx_electrodes = f21->max_num_of_tx;

		for (ii = 0; ii < tx_electrodes; ii++) {
			tx_num = f21->force_txrx_assignment[ii];
			if (tx_num == 0xff) {
				cnt = snprintf(buf, PAGE_SIZE - count, "xx ");
			} else {
				cnt = snprintf(buf, PAGE_SIZE - count, "%02u ",
						tx_num);
			}
			buf += cnt;
			count += cnt;
		}
	}

	snprintf(buf, PAGE_SIZE - count, "\n");
	count++;

	return count;
}

static ssize_t test_sysfs_force_rx_mapping_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int cnt;
	int count = 0;
	unsigned char ii;
	unsigned char offset;
	unsigned char rx_num;
	unsigned char rx_electrodes;

	if ((!f55 || !f55->has_force) && (!f21 || !f21->has_force))
		return -EINVAL;

	if (f55->has_force) {
		rx_electrodes = f55->query.num_of_rx_electrodes;

		for (ii = 0; ii < rx_electrodes; ii++) {
			rx_num = f55->force_rx_assignment[ii];
			if (rx_num == 0xff)
				cnt = snprintf(buf, PAGE_SIZE - count, "xx ");
			else
				cnt = snprintf(buf, PAGE_SIZE - count, "%02u ",
						rx_num);
			buf += cnt;
			count += cnt;
		}
	} else if (f21->has_force) {
		offset = f21->max_num_of_tx;
		rx_electrodes = f21->max_num_of_rx;

		for (ii = offset; ii < (rx_electrodes + offset); ii++) {
			rx_num = f21->force_txrx_assignment[ii];
			if (rx_num == 0xff)
				cnt = snprintf(buf, PAGE_SIZE - count, "xx ");
			else
				cnt = snprintf(buf, PAGE_SIZE - count, "%02u ",
						rx_num);
			buf += cnt;
			count += cnt;
		}
	}

	snprintf(buf, PAGE_SIZE - count, "\n");
	count++;

	return count;
}

static ssize_t test_sysfs_report_size_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%u\n", f54->report_size);
}

static ssize_t test_sysfs_status_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int retval;

	mutex_lock(&f54->status_mutex);

	retval = snprintf(buf, PAGE_SIZE, "%u\n", f54->status);

	mutex_unlock(&f54->status_mutex);

	return retval;
}

static ssize_t test_sysfs_do_preparation_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int retval;
	unsigned long setting;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	retval = sstrtoul(buf, 10, &setting);
	if (retval)
		return retval;

	if (setting != 1)
		return -EINVAL;

	mutex_lock(&f54->status_mutex);

	retval = test_check_for_idle_status();
	if (retval < 0)
		goto exit;

	retval = test_do_preparation();
	if (retval < 0) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: Failed to do preparation\n",
				__func__);
		goto exit;
	}

	retval = count;

exit:
	mutex_unlock(&f54->status_mutex);

	return retval;
}

static ssize_t test_sysfs_force_cal_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int retval;
	unsigned long setting;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	retval = sstrtoul(buf, 10, &setting);
	if (retval)
		return retval;

	if (setting != 1)
		return -EINVAL;

	mutex_lock(&f54->status_mutex);

	retval = test_check_for_idle_status();
	if (retval < 0)
		goto exit;

	retval = test_do_command(COMMAND_FORCE_CAL);
	if (retval < 0) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: Failed to do force cal\n",
				__func__);
		goto exit;
	}

	retval = count;

exit:
	mutex_unlock(&f54->status_mutex);

	return retval;
}

static ssize_t test_sysfs_get_report_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int retval;
	unsigned char command;
	unsigned long setting;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	retval = sstrtoul(buf, 10, &setting);
	if (retval)
		return retval;

	if (setting != 1)
		return -EINVAL;

	mutex_lock(&f54->status_mutex);

	retval = test_check_for_idle_status();
	if (retval < 0)
		goto exit;

	if (!test_report_type_valid(f54->report_type)) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: Invalid report type\n",
				__func__);
		retval = -EINVAL;
		goto exit;
	}

	test_set_interrupt(true);

	command = (unsigned char)COMMAND_GET_REPORT;

	retval = synaptics_rmi4_reg_write(rmi4_data,
			f54->command_base_addr,
			&command,
			sizeof(command));
	if (retval < 0) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: Failed to write get report command\n",
				__func__);
		goto exit;
	}

	f54->status = STATUS_BUSY;
	f54->report_size = 0;
	f54->data_pos = 0;

	hrtimer_start(&f54->watchdog,
			ktime_set(GET_REPORT_TIMEOUT_S, 0),
			HRTIMER_MODE_REL);

	retval = count;

exit:
	mutex_unlock(&f54->status_mutex);

	return retval;
}

static ssize_t test_sysfs_resume_touch_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int retval;
	unsigned char device_ctrl;
	unsigned long setting;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	retval = sstrtoul(buf, 10, &setting);
	if (retval)
		return retval;

	if (setting != 1)
		return -EINVAL;

	retval = synaptics_rmi4_reg_read(rmi4_data,
			rmi4_data->f01_ctrl_base_addr,
			&device_ctrl,
			sizeof(device_ctrl));
	if (retval < 0) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: Failed to restore no sleep setting\n",
				__func__);
		return retval;
	}

	device_ctrl = device_ctrl & ~NO_SLEEP_ON;
	device_ctrl |= rmi4_data->no_sleep_setting;

	retval = synaptics_rmi4_reg_write(rmi4_data,
			rmi4_data->f01_ctrl_base_addr,
			&device_ctrl,
			sizeof(device_ctrl));
	if (retval < 0) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: Failed to restore no sleep setting\n",
				__func__);
		return retval;
	}

	test_set_interrupt(false);

	if (f54->skip_preparation)
		return count;

	switch (f54->report_type) {
	case F54_16BIT_IMAGE:
	case F54_RAW_16BIT_IMAGE:
	case F54_SENSOR_SPEED:
	case F54_ADC_RANGE:
	case F54_ABS_RAW_CAP:
	case F54_ABS_DELTA_CAP:
	case F54_ABS_HYBRID_DELTA_CAP:
	case F54_ABS_HYBRID_RAW_CAP:
	case F54_FULL_RAW_CAP_TDDI:
		break;
	case F54_AMP_RAW_ADC:
		if (f54->query_49.has_ctrl188) {
			retval = synaptics_rmi4_reg_read(rmi4_data,
					f54->control.reg_188->address,
					f54->control.reg_188->data,
					sizeof(f54->control.reg_188->data));
			if (retval < 0) {
				dev_err(rmi4_data->pdev->dev.parent,
						"%s: Failed to set start production test\n",
						__func__);
				return retval;
			}
			f54->control.reg_188->start_production_test = 0;
			retval = synaptics_rmi4_reg_write(rmi4_data,
					f54->control.reg_188->address,
					f54->control.reg_188->data,
					sizeof(f54->control.reg_188->data));
			if (retval < 0) {
				dev_err(rmi4_data->pdev->dev.parent,
						"%s: Failed to set start production test\n",
						__func__);
				return retval;
			}
		}
		break;
	default:
		rmi4_data->reset_device(rmi4_data, false);
	}

	return count;
}

static ssize_t test_sysfs_do_afe_calibration_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int retval;
	unsigned long setting;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	retval = sstrtoul(buf, 10, &setting);
	if (retval)
		return retval;

	if (!f54->query_49.has_ctrl188) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: F54_ANALOG_Ctrl188 not found\n",
				__func__);
		return -EINVAL;
	}

	if (setting == 0 || setting == 1)
		retval = test_do_afe_calibration((enum f54_afe_cal)setting);
	else
		return -EINVAL;

	if (retval)
		return retval;
	else
		return count;
}

static ssize_t test_sysfs_report_type_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%u\n", f54->report_type);
}

static ssize_t test_sysfs_report_type_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int retval;
	unsigned char data;
	unsigned long setting;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	retval = sstrtoul(buf, 10, &setting);
	if (retval)
		return retval;

	mutex_lock(&f54->status_mutex);

	retval = test_check_for_idle_status();
	if (retval < 0)
		goto exit;

	if (!test_report_type_valid((enum f54_report_types)setting)) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: Report type not supported by driver\n",
				__func__);
		retval = -EINVAL;
		goto exit;
	}

	f54->report_type = (enum f54_report_types)setting;
	data = (unsigned char)setting;
	retval = synaptics_rmi4_reg_write(rmi4_data,
			f54->data_base_addr,
			&data,
			sizeof(data));
	if (retval < 0) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: Failed to write report type\n",
				__func__);
		goto exit;
	}

	retval = count;

exit:
	mutex_unlock(&f54->status_mutex);

	return retval;
}

static ssize_t test_sysfs_fifoindex_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int retval;
	unsigned char data[2];
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	retval = synaptics_rmi4_reg_read(rmi4_data,
			f54->data_base_addr + REPORT_INDEX_OFFSET,
			data,
			sizeof(data));
	if (retval < 0) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: Failed to read report index\n",
				__func__);
		return retval;
	}

	batohs(&f54->fifoindex, data);

	return snprintf(buf, PAGE_SIZE, "%u\n", f54->fifoindex);
}

static ssize_t test_sysfs_fifoindex_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int retval;
	unsigned char data[2];
	unsigned long setting;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	retval = sstrtoul(buf, 10, &setting);
	if (retval)
		return retval;

	f54->fifoindex = setting;

	hstoba(data, (unsigned short)setting);

	retval = synaptics_rmi4_reg_write(rmi4_data,
			f54->data_base_addr + REPORT_INDEX_OFFSET,
			data,
			sizeof(data));
	if (retval < 0) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: Failed to write report index\n",
				__func__);
		return retval;
	}

	return count;
}

static ssize_t test_sysfs_no_auto_cal_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%u\n", f54->no_auto_cal);
}

static ssize_t test_sysfs_no_auto_cal_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int retval;
	unsigned char data;
	unsigned long setting;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	retval = sstrtoul(buf, 10, &setting);
	if (retval)
		return retval;

	if (setting > 1)
		return -EINVAL;

	retval = synaptics_rmi4_reg_read(rmi4_data,
			f54->control_base_addr,
			&data,
			sizeof(data));
	if (retval < 0) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: Failed to read no auto cal setting\n",
				__func__);
		return retval;
	}

	if (setting)
		data |= CONTROL_NO_AUTO_CAL;
	else
		data &= ~CONTROL_NO_AUTO_CAL;

	retval = synaptics_rmi4_reg_write(rmi4_data,
			f54->control_base_addr,
			&data,
			sizeof(data));
	if (retval < 0) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: Failed to write no auto cal setting\n",
				__func__);
		return retval;
	}

	f54->no_auto_cal = (setting == 1);

	return count;
}

static ssize_t test_sysfs_read_report_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	unsigned int ii;
	unsigned int jj;
	int cnt;
	int count = 0;
	int tx_num = f54->tx_assigned;
	int rx_num = f54->rx_assigned;
	char *report_data_8;
	short *report_data_16;
	int *report_data_32;
	unsigned short *report_data_u16;
	unsigned int *report_data_u32;

	switch (f54->report_type) {
	case F54_8BIT_IMAGE:
		report_data_8 = (char *)f54->report_data;
		for (ii = 0; ii < f54->report_size; ii++) {
			cnt = snprintf(buf, PAGE_SIZE - count, "%03d: %d\n",
					ii, *report_data_8);
			report_data_8++;
			buf += cnt;
			count += cnt;
		}
		break;
	case F54_AMP_RAW_ADC:
		report_data_u16 = (unsigned short *)f54->report_data;
		cnt = snprintf(buf, PAGE_SIZE - count, "tx = %d\nrx = %d\n",
				tx_num, rx_num);
		buf += cnt;
		count += cnt;

		for (ii = 0; ii < tx_num; ii++) {
			for (jj = 0; jj < (rx_num - 1); jj++) {
				cnt = snprintf(buf, PAGE_SIZE - count, "%-4d ",
						*report_data_u16);
				report_data_u16++;
				buf += cnt;
				count += cnt;
			}
			cnt = snprintf(buf, PAGE_SIZE - count, "%-4d\n",
					*report_data_u16);
			report_data_u16++;
			buf += cnt;
			count += cnt;
		}
		break;
	case F54_16BIT_IMAGE:
	case F54_RAW_16BIT_IMAGE:
	case F54_TRUE_BASELINE:
	case F54_FULL_RAW_CAP:
	case F54_FULL_RAW_CAP_NO_RX_COUPLING:
	case F54_SENSOR_SPEED:
	case F54_AMP_FULL_RAW_CAP:
	case F54_FULL_RAW_CAP_TDDI:
		report_data_16 = (short *)f54->report_data;
		cnt = snprintf(buf, PAGE_SIZE - count, "tx = %d\nrx = %d\n",
				tx_num, rx_num);
		buf += cnt;
		count += cnt;

		for (ii = 0; ii < tx_num; ii++) {
			for (jj = 0; jj < (rx_num - 1); jj++) {
				cnt = snprintf(buf, PAGE_SIZE - count, "%-4d ",
						*report_data_16);
				report_data_16++;
				buf += cnt;
				count += cnt;
			}
			cnt = snprintf(buf, PAGE_SIZE - count, "%-4d\n",
					*report_data_16);
			report_data_16++;
			buf += cnt;
			count += cnt;
		}
		break;
	case F54_HIGH_RESISTANCE:
	case F54_FULL_RAW_CAP_MIN_MAX:
		report_data_16 = (short *)f54->report_data;
		for (ii = 0; ii < f54->report_size; ii += 2) {
			cnt = snprintf(buf, PAGE_SIZE - count, "%03d: %d\n",
					ii / 2, *report_data_16);
			report_data_16++;
			buf += cnt;
			count += cnt;
		}
		break;
	case F54_ABS_RAW_CAP:
		report_data_u32 = (unsigned int *)f54->report_data;
		cnt = snprintf(buf, PAGE_SIZE - count, "rx ");
		buf += cnt;
		count += cnt;
		for (ii = 0; ii < rx_num; ii++) {
			cnt = snprintf(buf, PAGE_SIZE - count, "     %2d", ii);
			buf += cnt;
			count += cnt;
		}
		cnt = snprintf(buf, PAGE_SIZE - count, "\n");
		buf += cnt;
		count += cnt;

		cnt = snprintf(buf, PAGE_SIZE - count, "   ");
		buf += cnt;
		count += cnt;
		for (ii = 0; ii < rx_num; ii++) {
			cnt = snprintf(buf, PAGE_SIZE - count, "  %5u",
					*report_data_u32);
			report_data_u32++;
			buf += cnt;
			count += cnt;
		}
		cnt = snprintf(buf, PAGE_SIZE - count, "\n");
		buf += cnt;
		count += cnt;

		cnt = snprintf(buf, PAGE_SIZE - count, "tx ");
		buf += cnt;
		count += cnt;
		for (ii = 0; ii < tx_num; ii++) {
			cnt = snprintf(buf, PAGE_SIZE - count, "     %2d", ii);
			buf += cnt;
			count += cnt;
		}
		cnt = snprintf(buf, PAGE_SIZE - count, "\n");
		buf += cnt;
		count += cnt;

		cnt = snprintf(buf, PAGE_SIZE - count, "   ");
		buf += cnt;
		count += cnt;
		for (ii = 0; ii < tx_num; ii++) {
			cnt = snprintf(buf, PAGE_SIZE - count, "  %5u",
					*report_data_u32);
			report_data_u32++;
			buf += cnt;
			count += cnt;
		}
		cnt = snprintf(buf, PAGE_SIZE - count, "\n");
		buf += cnt;
		count += cnt;
		break;
	case F54_ABS_DELTA_CAP:
	case F54_ABS_HYBRID_DELTA_CAP:
	case F54_ABS_HYBRID_RAW_CAP:
		report_data_32 = (int *)f54->report_data;
		cnt = snprintf(buf, PAGE_SIZE - count, "rx ");
		buf += cnt;
		count += cnt;
		for (ii = 0; ii < rx_num; ii++) {
			cnt = snprintf(buf, PAGE_SIZE - count, "     %2d", ii);
			buf += cnt;
			count += cnt;
		}
		cnt = snprintf(buf, PAGE_SIZE - count, "\n");
		buf += cnt;
		count += cnt;

		cnt = snprintf(buf, PAGE_SIZE - count, "   ");
		buf += cnt;
		count += cnt;
		for (ii = 0; ii < rx_num; ii++) {
			cnt = snprintf(buf, PAGE_SIZE - count, "  %5d",
					*report_data_32);
			report_data_32++;
			buf += cnt;
			count += cnt;
		}
		cnt = snprintf(buf, PAGE_SIZE - count, "\n");
		buf += cnt;
		count += cnt;

		cnt = snprintf(buf, PAGE_SIZE - count, "tx ");
		buf += cnt;
		count += cnt;
		for (ii = 0; ii < tx_num; ii++) {
			cnt = snprintf(buf, PAGE_SIZE - count, "     %2d", ii);
			buf += cnt;
			count += cnt;
		}
		cnt = snprintf(buf, PAGE_SIZE - count, "\n");
		buf += cnt;
		count += cnt;

		cnt = snprintf(buf, PAGE_SIZE - count, "   ");
		buf += cnt;
		count += cnt;
		for (ii = 0; ii < tx_num; ii++) {
			cnt = snprintf(buf, PAGE_SIZE - count, "  %5d",
					*report_data_32);
			report_data_32++;
			buf += cnt;
			count += cnt;
		}
		cnt = snprintf(buf, PAGE_SIZE - count, "\n");
		buf += cnt;
		count += cnt;
		break;
	default:
		for (ii = 0; ii < f54->report_size; ii++) {
			cnt = snprintf(buf, PAGE_SIZE - count, "%03d: 0x%02x\n",
					ii, f54->report_data[ii]);
			buf += cnt;
			count += cnt;
		}
	}

	snprintf(buf, PAGE_SIZE - count, "\n");
	count++;

	return count;
}

static ssize_t test_sysfs_read_report_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int retval;
	unsigned char timeout = GET_REPORT_TIMEOUT_S * 10;
	unsigned char timeout_count;
	const char cmd[] = {'1', 0};
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	retval = test_sysfs_report_type_store(dev, attr, buf, count);
	if (retval < 0)
		goto exit;

	retval = test_sysfs_do_preparation_store(dev, attr, cmd, 1);
	if (retval < 0)
		goto exit;

	retval = test_sysfs_get_report_store(dev, attr, cmd, 1);
	if (retval < 0)
		goto exit;

	timeout_count = 0;
	do {
		if (f54->status != STATUS_BUSY)
			break;
		msleep(100);
		timeout_count++;
	} while (timeout_count < timeout);

	if ((f54->status != STATUS_IDLE) || (f54->report_size == 0)) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: Failed to read report\n",
				__func__);
		retval = -EINVAL;
		goto exit;
	}

	retval = test_sysfs_resume_touch_store(dev, attr, cmd, 1);
	if (retval < 0)
		goto exit;

	return count;

exit:
	rmi4_data->reset_device(rmi4_data, false);

	return retval;
}

static ssize_t test_sysfs_data_read(struct file *data_file,
		struct kobject *kobj, struct bin_attribute *attributes,
		char *buf, loff_t pos, size_t count)
{
	int retval;
	unsigned int read_size;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	mutex_lock(&f54->status_mutex);

	retval = test_check_for_idle_status();
	if (retval < 0)
		goto exit;

	if (!f54->report_data) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: Report type %d data not available\n",
				__func__, f54->report_type);
		retval = -EINVAL;
		goto exit;
	}

	if ((f54->data_pos + count) > f54->report_size)
		read_size = f54->report_size - f54->data_pos;
	else
		read_size = min_t(unsigned int, count, f54->report_size);

	retval = secure_memcpy(buf, count, f54->report_data + f54->data_pos,
			f54->data_buffer_size - f54->data_pos, read_size);
	if (retval < 0) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: Failed to copy report data\n",
				__func__);
		goto exit;
	}
	f54->data_pos += read_size;
	retval = read_size;

exit:
	mutex_unlock(&f54->status_mutex);

	return retval;
}

static void test_report_work(struct work_struct *work)
{
	int retval;
	unsigned char report_index[2];
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	mutex_lock(&f54->status_mutex);

	if (f54->status != STATUS_BUSY) {
		retval = f54->status;
		goto exit;
	}

	retval = test_wait_for_command_completion();
	if (retval < 0) {
		retval = STATUS_ERROR;
		goto exit;
	}

	test_set_report_size();
	if (f54->report_size == 0) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: Report data size = 0\n",
				__func__);
		retval = STATUS_ERROR;
		goto exit;
	}

	if (f54->data_buffer_size < f54->report_size) {
		if (f54->data_buffer_size)
			kfree(f54->report_data);
		f54->report_data = kzalloc(f54->report_size, GFP_KERNEL);
		if (!f54->report_data) {
			dev_err(rmi4_data->pdev->dev.parent,
					"%s: Failed to alloc mem for data buffer\n",
					__func__);
			f54->data_buffer_size = 0;
			retval = STATUS_ERROR;
			goto exit;
		}
		f54->data_buffer_size = f54->report_size;
	}

	report_index[0] = 0;
	report_index[1] = 0;

	retval = synaptics_rmi4_reg_write(rmi4_data,
			f54->data_base_addr + REPORT_INDEX_OFFSET,
			report_index,
			sizeof(report_index));
	if (retval < 0) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: Failed to write report data index\n",
				__func__);
		retval = STATUS_ERROR;
		goto exit;
	}

	retval = synaptics_rmi4_reg_read(rmi4_data,
			f54->data_base_addr + REPORT_DATA_OFFSET,
			f54->report_data,
			f54->report_size);
	if (retval < 0) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: Failed to read report data\n",
				__func__);
		retval = STATUS_ERROR;
		goto exit;
	}

	retval = STATUS_IDLE;

exit:
	mutex_unlock(&f54->status_mutex);

	if (retval == STATUS_ERROR)
		f54->report_size = 0;

	f54->status = retval;

	return;
}

static void test_remove_sysfs(void)
{
	sysfs_remove_group(f54->sysfs_dir, &attr_group);
	sysfs_remove_bin_file(f54->sysfs_dir, &test_report_data);
	kobject_put(f54->sysfs_dir);

	return;
}

static int test_set_sysfs(void)
{
	int retval;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	f54->sysfs_dir = kobject_create_and_add(SYSFS_FOLDER_NAME,
			&rmi4_data->input_dev->dev.kobj);
	if (!f54->sysfs_dir) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: Failed to create sysfs directory\n",
				__func__);
		goto exit_directory;
	}

	retval = sysfs_create_bin_file(f54->sysfs_dir, &test_report_data);
	if (retval < 0) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: Failed to create sysfs bin file\n",
				__func__);
		goto exit_bin_file;
	}

	retval = sysfs_create_group(f54->sysfs_dir, &attr_group);
	if (retval < 0) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: Failed to create sysfs attributes\n",
				__func__);
		goto exit_attributes;
	}

	return 0;

exit_attributes:
	sysfs_remove_group(f54->sysfs_dir, &attr_group);
	sysfs_remove_bin_file(f54->sysfs_dir, &test_report_data);

exit_bin_file:
	kobject_put(f54->sysfs_dir);

exit_directory:
	return -ENODEV;
}

static void test_free_control_mem(void)
{
	struct f54_control control = f54->control;

	kfree(control.reg_7);
	kfree(control.reg_41);
	kfree(control.reg_57);
	kfree(control.reg_86);
	kfree(control.reg_88);
	kfree(control.reg_110);
	kfree(control.reg_149);
	kfree(control.reg_188);

	return;
}

static void test_set_data(void)
{
	unsigned short reg_addr;

	reg_addr = f54->data_base_addr + REPORT_DATA_OFFSET + 1;

	/* data 4 */
	if (f54->query.has_sense_frequency_control)
		reg_addr++;

	/* data 5 reserved */

	/* data 6 */
	if (f54->query.has_interference_metric)
		reg_addr += 2;

	/* data 7 */
	if (f54->query.has_one_byte_report_rate |
			f54->query.has_two_byte_report_rate)
		reg_addr++;
	if (f54->query.has_two_byte_report_rate)
		reg_addr++;

	/* data 8 */
	if (f54->query.has_variance_metric)
		reg_addr += 2;

	/* data 9 */
	if (f54->query.has_multi_metric_state_machine)
		reg_addr += 2;

	/* data 10 */
	if (f54->query.has_multi_metric_state_machine |
			f54->query.has_noise_state)
		reg_addr++;

	/* data 11 */
	if (f54->query.has_status)
		reg_addr++;

	/* data 12 */
	if (f54->query.has_slew_metric)
		reg_addr += 2;

	/* data 13 */
	if (f54->query.has_multi_metric_state_machine)
		reg_addr += 2;

	/* data 14 */
	if (f54->query_13.has_cidim)
		reg_addr++;

	/* data 15 */
	if (f54->query_13.has_rail_im)
		reg_addr++;

	/* data 16 */
	if (f54->query_13.has_noise_mitigation_enhancement)
		reg_addr++;

	/* data 17 */
	if (f54->query_16.has_data17)
		reg_addr++;

	/* data 18 */
	if (f54->query_21.has_query24_data18)
		reg_addr++;

	/* data 19 */
	if (f54->query_21.has_data19)
		reg_addr++;

	/* data_20 */
	if (f54->query_25.has_ctrl109)
		reg_addr++;

	/* data 21 */
	if (f54->query_27.has_data21)
		reg_addr++;

	/* data 22 */
	if (f54->query_27.has_data22)
		reg_addr++;

	/* data 23 */
	if (f54->query_29.has_data23)
		reg_addr++;

	/* data 24 */
	if (f54->query_32.has_data24)
		reg_addr++;

	/* data 25 */
	if (f54->query_35.has_data25)
		reg_addr++;

	/* data 26 */
	if (f54->query_35.has_data26)
		reg_addr++;

	/* data 27 */
	if (f54->query_46.has_data27)
		reg_addr++;

	/* data 28 */
	if (f54->query_46.has_data28)
		reg_addr++;

	/* data 29 30 reserved */

	/* data 31 */
	if (f54->query_49.has_data31) {
		f54->data_31.address = reg_addr;
		reg_addr++;
	}

	return;
}

static int test_set_controls(void)
{
	int retval;
	unsigned char length;
	unsigned char num_of_sensing_freqs;
	unsigned short reg_addr = f54->control_base_addr;
	struct f54_control *control = &f54->control;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	num_of_sensing_freqs = f54->query.number_of_sensing_frequencies;

	/* control 0 */
	reg_addr += CONTROL_0_SIZE;

	/* control 1 */
	if ((f54->query.touch_controller_family == 0) ||
			(f54->query.touch_controller_family == 1))
		reg_addr += CONTROL_1_SIZE;

	/* control 2 */
	reg_addr += CONTROL_2_SIZE;

	/* control 3 */
	if (f54->query.has_pixel_touch_threshold_adjustment)
		reg_addr += CONTROL_3_SIZE;

	/* controls 4 5 6 */
	if ((f54->query.touch_controller_family == 0) ||
			(f54->query.touch_controller_family == 1))
		reg_addr += CONTROL_4_6_SIZE;

	/* control 7 */
	if (f54->query.touch_controller_family == 1) {
		control->reg_7 = kzalloc(sizeof(*(control->reg_7)),
				GFP_KERNEL);
		if (!control->reg_7)
			goto exit_no_mem;
		control->reg_7->address = reg_addr;
		reg_addr += CONTROL_7_SIZE;
	}

	/* controls 8 9 */
	if ((f54->query.touch_controller_family == 0) ||
			(f54->query.touch_controller_family == 1))
		reg_addr += CONTROL_8_9_SIZE;

	/* control 10 */
	if (f54->query.has_interference_metric)
		reg_addr += CONTROL_10_SIZE;

	/* control 11 */
	if (f54->query.has_ctrl11)
		reg_addr += CONTROL_11_SIZE;

	/* controls 12 13 */
	if (f54->query.has_relaxation_control)
		reg_addr += CONTROL_12_13_SIZE;

	/* controls 14 15 16 */
	if (f54->query.has_sensor_assignment) {
		reg_addr += CONTROL_14_SIZE;
		reg_addr += CONTROL_15_SIZE * f54->query.num_of_rx_electrodes;
		reg_addr += CONTROL_16_SIZE * f54->query.num_of_tx_electrodes;
	}

	/* controls 17 18 19 */
	if (f54->query.has_sense_frequency_control) {
		reg_addr += CONTROL_17_SIZE * num_of_sensing_freqs;
		reg_addr += CONTROL_18_SIZE * num_of_sensing_freqs;
		reg_addr += CONTROL_19_SIZE * num_of_sensing_freqs;
	}

	/* control 20 */
	reg_addr += CONTROL_20_SIZE;

	/* control 21 */
	if (f54->query.has_sense_frequency_control)
		reg_addr += CONTROL_21_SIZE;

	/* controls 22 23 24 25 26 */
	if (f54->query.has_firmware_noise_mitigation)
		reg_addr += CONTROL_22_26_SIZE;

	/* control 27 */
	if (f54->query.has_iir_filter)
		reg_addr += CONTROL_27_SIZE;

	/* control 28 */
	if (f54->query.has_firmware_noise_mitigation)
		reg_addr += CONTROL_28_SIZE;

	/* control 29 */
	if (f54->query.has_cmn_removal)
		reg_addr += CONTROL_29_SIZE;

	/* control 30 */
	if (f54->query.has_cmn_maximum)
		reg_addr += CONTROL_30_SIZE;

	/* control 31 */
	if (f54->query.has_touch_hysteresis)
		reg_addr += CONTROL_31_SIZE;

	/* controls 32 33 34 35 */
	if (f54->query.has_edge_compensation)
		reg_addr += CONTROL_32_35_SIZE;

	/* control 36 */
	if ((f54->query.curve_compensation_mode == 1) ||
			(f54->query.curve_compensation_mode == 2)) {
		if (f54->query.curve_compensation_mode == 1) {
			length = max(f54->query.num_of_rx_electrodes,
					f54->query.num_of_tx_electrodes);
		} else if (f54->query.curve_compensation_mode == 2) {
			length = f54->query.num_of_rx_electrodes;
		}
		reg_addr += CONTROL_36_SIZE * length;
	}

	/* control 37 */
	if (f54->query.curve_compensation_mode == 2)
		reg_addr += CONTROL_37_SIZE * f54->query.num_of_tx_electrodes;

	/* controls 38 39 40 */
	if (f54->query.has_per_frequency_noise_control) {
		reg_addr += CONTROL_38_SIZE * num_of_sensing_freqs;
		reg_addr += CONTROL_39_SIZE * num_of_sensing_freqs;
		reg_addr += CONTROL_40_SIZE * num_of_sensing_freqs;
	}

	/* control 41 */
	if (f54->query.has_signal_clarity) {
		control->reg_41 = kzalloc(sizeof(*(control->reg_41)),
				GFP_KERNEL);
		if (!control->reg_41)
			goto exit_no_mem;
		control->reg_41->address = reg_addr;
		reg_addr += CONTROL_41_SIZE;
	}

	/* control 42 */
	if (f54->query.has_variance_metric)
		reg_addr += CONTROL_42_SIZE;

	/* controls 43 44 45 46 47 48 49 50 51 52 53 54 */
	if (f54->query.has_multi_metric_state_machine)
		reg_addr += CONTROL_43_54_SIZE;

	/* controls 55 56 */
	if (f54->query.has_0d_relaxation_control)
		reg_addr += CONTROL_55_56_SIZE;

	/* control 57 */
	if (f54->query.has_0d_acquisition_control) {
		control->reg_57 = kzalloc(sizeof(*(control->reg_57)),
				GFP_KERNEL);
		if (!control->reg_57)
			goto exit_no_mem;
		control->reg_57->address = reg_addr;
		reg_addr += CONTROL_57_SIZE;
	}

	/* control 58 */
	if (f54->query.has_0d_acquisition_control)
		reg_addr += CONTROL_58_SIZE;

	/* control 59 */
	if (f54->query.has_h_blank)
		reg_addr += CONTROL_59_SIZE;

	/* controls 60 61 62 */
	if ((f54->query.has_h_blank) ||
			(f54->query.has_v_blank) ||
			(f54->query.has_long_h_blank))
		reg_addr += CONTROL_60_62_SIZE;

	/* control 63 */
	if ((f54->query.has_h_blank) ||
			(f54->query.has_v_blank) ||
			(f54->query.has_long_h_blank) ||
			(f54->query.has_slew_metric) ||
			(f54->query.has_slew_option) ||
			(f54->query.has_noise_mitigation2))
		reg_addr += CONTROL_63_SIZE;

	/* controls 64 65 66 67 */
	if (f54->query.has_h_blank)
		reg_addr += CONTROL_64_67_SIZE * 7;
	else if ((f54->query.has_v_blank) ||
			(f54->query.has_long_h_blank))
		reg_addr += CONTROL_64_67_SIZE;

	/* controls 68 69 70 71 72 73 */
	if ((f54->query.has_h_blank) ||
			(f54->query.has_v_blank) ||
			(f54->query.has_long_h_blank))
		reg_addr += CONTROL_68_73_SIZE;

	/* control 74 */
	if (f54->query.has_slew_metric)
		reg_addr += CONTROL_74_SIZE;

	/* control 75 */
	if (f54->query.has_enhanced_stretch)
		reg_addr += CONTROL_75_SIZE * num_of_sensing_freqs;

	/* control 76 */
	if (f54->query.has_startup_fast_relaxation)
		reg_addr += CONTROL_76_SIZE;

	/* controls 77 78 */
	if (f54->query.has_esd_control)
		reg_addr += CONTROL_77_78_SIZE;

	/* controls 79 80 81 82 83 */
	if (f54->query.has_noise_mitigation2)
		reg_addr += CONTROL_79_83_SIZE;

	/* controls 84 85 */
	if (f54->query.has_energy_ratio_relaxation)
		reg_addr += CONTROL_84_85_SIZE;

	/* control 86 */
	if (f54->query_13.has_ctrl86) {
		control->reg_86 = kzalloc(sizeof(*(control->reg_86)),
				GFP_KERNEL);
		if (!control->reg_86)
			goto exit_no_mem;
		control->reg_86->address = reg_addr;
		retval = synaptics_rmi4_reg_read(rmi4_data,
				f54->control.reg_86->address,
				f54->control.reg_86->data,
				sizeof(f54->control.reg_86->data));
		if (retval < 0) {
			dev_err(rmi4_data->pdev->dev.parent,
					"%s: Failed to read sense display ratio\n",
					__func__);
			return retval;
		}
		reg_addr += CONTROL_86_SIZE;
	}

	/* control 87 */
	if (f54->query_13.has_ctrl87)
		reg_addr += CONTROL_87_SIZE;

	/* control 88 */
	if (f54->query.has_ctrl88) {
		control->reg_88 = kzalloc(sizeof(*(control->reg_88)),
				GFP_KERNEL);
		if (!control->reg_88)
			goto exit_no_mem;
		control->reg_88->address = reg_addr;
		reg_addr += CONTROL_88_SIZE;
	}

	/* control 89 */
	if (f54->query_13.has_cidim ||
			f54->query_13.has_noise_mitigation_enhancement ||
			f54->query_13.has_rail_im)
		reg_addr += CONTROL_89_SIZE;

	/* control 90 */
	if (f54->query_15.has_ctrl90)
		reg_addr += CONTROL_90_SIZE;

	/* control 91 */
	if (f54->query_21.has_ctrl91)
		reg_addr += CONTROL_91_SIZE;

	/* control 92 */
	if (f54->query_16.has_ctrl92)
		reg_addr += CONTROL_92_SIZE;

	/* control 93 */
	if (f54->query_16.has_ctrl93)
		reg_addr += CONTROL_93_SIZE;

	/* control 94 */
	if (f54->query_16.has_ctrl94_query18)
		reg_addr += CONTROL_94_SIZE;

	/* control 95 */
	if (f54->query_16.has_ctrl95_query19)
		reg_addr += CONTROL_95_SIZE;

	/* control 96 */
	if (f54->query_21.has_ctrl96)
		reg_addr += CONTROL_96_SIZE;

	/* control 97 */
	if (f54->query_21.has_ctrl97)
		reg_addr += CONTROL_97_SIZE;

	/* control 98 */
	if (f54->query_21.has_ctrl98)
		reg_addr += CONTROL_98_SIZE;

	/* control 99 */
	if (f54->query.touch_controller_family == 2)
		reg_addr += CONTROL_99_SIZE;

	/* control 100 */
	if (f54->query_16.has_ctrl100)
		reg_addr += CONTROL_100_SIZE;

	/* control 101 */
	if (f54->query_22.has_ctrl101)
		reg_addr += CONTROL_101_SIZE;


	/* control 102 */
	if (f54->query_23.has_ctrl102)
		reg_addr += CONTROL_102_SIZE;

	/* control 103 */
	if (f54->query_22.has_ctrl103_query26) {
		f54->skip_preparation = true;
		reg_addr += CONTROL_103_SIZE;
	}

	/* control 104 */
	if (f54->query_22.has_ctrl104)
		reg_addr += CONTROL_104_SIZE;

	/* control 105 */
	if (f54->query_22.has_ctrl105)
		reg_addr += CONTROL_105_SIZE;

	/* control 106 */
	if (f54->query_25.has_ctrl106)
		reg_addr += CONTROL_106_SIZE;

	/* control 107 */
	if (f54->query_25.has_ctrl107)
		reg_addr += CONTROL_107_SIZE;

	/* control 108 */
	if (f54->query_25.has_ctrl108)
		reg_addr += CONTROL_108_SIZE;

	/* control 109 */
	if (f54->query_25.has_ctrl109)
		reg_addr += CONTROL_109_SIZE;

	/* control 110 */
	if (f54->query_27.has_ctrl110) {
		control->reg_110 = kzalloc(sizeof(*(control->reg_110)),
				GFP_KERNEL);
		if (!control->reg_110)
			goto exit_no_mem;
		control->reg_110->address = reg_addr;
		reg_addr += CONTROL_110_SIZE;
	}

	/* control 111 */
	if (f54->query_27.has_ctrl111)
		reg_addr += CONTROL_111_SIZE;

	/* control 112 */
	if (f54->query_27.has_ctrl112)
		reg_addr += CONTROL_112_SIZE;

	/* control 113 */
	if (f54->query_27.has_ctrl113)
		reg_addr += CONTROL_113_SIZE;

	/* control 114 */
	if (f54->query_27.has_ctrl114)
		reg_addr += CONTROL_114_SIZE;

	/* control 115 */
	if (f54->query_29.has_ctrl115)
		reg_addr += CONTROL_115_SIZE;

	/* control 116 */
	if (f54->query_29.has_ctrl116)
		reg_addr += CONTROL_116_SIZE;

	/* control 117 */
	if (f54->query_29.has_ctrl117)
		reg_addr += CONTROL_117_SIZE;

	/* control 118 */
	if (f54->query_30.has_ctrl118)
		reg_addr += CONTROL_118_SIZE;

	/* control 119 */
	if (f54->query_30.has_ctrl119)
		reg_addr += CONTROL_119_SIZE;

	/* control 120 */
	if (f54->query_30.has_ctrl120)
		reg_addr += CONTROL_120_SIZE;

	/* control 121 */
	if (f54->query_30.has_ctrl121)
		reg_addr += CONTROL_121_SIZE;

	/* control 122 */
	if (f54->query_30.has_ctrl122_query31)
		reg_addr += CONTROL_122_SIZE;

	/* control 123 */
	if (f54->query_30.has_ctrl123)
		reg_addr += CONTROL_123_SIZE;

	/* control 124 reserved */

	/* control 125 */
	if (f54->query_32.has_ctrl125)
		reg_addr += CONTROL_125_SIZE;

	/* control 126 */
	if (f54->query_32.has_ctrl126)
		reg_addr += CONTROL_126_SIZE;

	/* control 127 */
	if (f54->query_32.has_ctrl127)
		reg_addr += CONTROL_127_SIZE;

	/* controls 128 129 130 131 reserved */

	/* control 132 */
	if (f54->query_33.has_ctrl132)
		reg_addr += CONTROL_132_SIZE;

	/* control 133 */
	if (f54->query_33.has_ctrl133)
		reg_addr += CONTROL_133_SIZE;

	/* control 134 */
	if (f54->query_33.has_ctrl134)
		reg_addr += CONTROL_134_SIZE;

	/* controls 135 136 reserved */

	/* control 137 */
	if (f54->query_35.has_ctrl137)
		reg_addr += CONTROL_137_SIZE;

	/* control 138 */
	if (f54->query_35.has_ctrl138)
		reg_addr += CONTROL_138_SIZE;

	/* control 139 */
	if (f54->query_35.has_ctrl139)
		reg_addr += CONTROL_139_SIZE;

	/* control 140 */
	if (f54->query_35.has_ctrl140)
		reg_addr += CONTROL_140_SIZE;

	/* control 141 reserved */

	/* control 142 */
	if (f54->query_36.has_ctrl142)
		reg_addr += CONTROL_142_SIZE;

	/* control 143 */
	if (f54->query_36.has_ctrl143)
		reg_addr += CONTROL_143_SIZE;

	/* control 144 */
	if (f54->query_36.has_ctrl144)
		reg_addr += CONTROL_144_SIZE;

	/* control 145 */
	if (f54->query_36.has_ctrl145)
		reg_addr += CONTROL_145_SIZE;

	/* control 146 */
	if (f54->query_36.has_ctrl146)
		reg_addr += CONTROL_146_SIZE;

	/* control 147 */
	if (f54->query_38.has_ctrl147)
		reg_addr += CONTROL_147_SIZE;

	/* control 148 */
	if (f54->query_38.has_ctrl148)
		reg_addr += CONTROL_148_SIZE;

	/* control 149 */
	if (f54->query_38.has_ctrl149) {
		control->reg_149 = kzalloc(sizeof(*(control->reg_149)),
				GFP_KERNEL);
		if (!control->reg_149)
			goto exit_no_mem;
		control->reg_149->address = reg_addr;
		reg_addr += CONTROL_149_SIZE;
	}

	/* controls 150 to 162 reserved */

	/* control 163 */
	if (f54->query_40.has_ctrl163_query41)
		reg_addr += CONTROL_163_SIZE;

	/* control 164 reserved */

	/* control 165 */
	if (f54->query_40.has_ctrl165_query42)
		reg_addr += CONTROL_165_SIZE;

	/* control 166 */
	if (f54->query_40.has_ctrl166)
		reg_addr += CONTROL_166_SIZE;

	/* control 167 */
	if (f54->query_40.has_ctrl167)
		reg_addr += CONTROL_167_SIZE;

	/* controls 168 to 175 reserved */

	/* control 176 */
	if (f54->query_46.has_ctrl176)
		reg_addr += CONTROL_176_SIZE;

	/* controls 177 178 reserved */

	/* control 179 */
	if (f54->query_46.has_ctrl179)
		reg_addr += CONTROL_179_SIZE;

	/* controls 180 to 187 reserved */

	/* control 188 */
	if (f54->query_49.has_ctrl188) {
		control->reg_188 = kzalloc(sizeof(*(control->reg_188)),
				GFP_KERNEL);
		if (!control->reg_188)
			goto exit_no_mem;
		control->reg_188->address = reg_addr;
		reg_addr += CONTROL_188_SIZE;
	}

	return 0;

exit_no_mem:
	dev_err(rmi4_data->pdev->dev.parent,
			"%s: Failed to alloc mem for control registers\n",
			__func__);
	return -ENOMEM;
}

static int test_set_queries(void)
{
	int retval;
	unsigned char offset;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	retval = synaptics_rmi4_reg_read(rmi4_data,
			f54->query_base_addr,
			f54->query.data,
			sizeof(f54->query.data));
	if (retval < 0)
		return retval;

	offset = sizeof(f54->query.data);

	/* query 12 */
	if (f54->query.has_sense_frequency_control == 0)
		offset -= 1;

	/* query 13 */
	if (f54->query.has_query13) {
		retval = synaptics_rmi4_reg_read(rmi4_data,
				f54->query_base_addr + offset,
				f54->query_13.data,
				sizeof(f54->query_13.data));
		if (retval < 0)
			return retval;
		offset += 1;
	}

	/* query 14 */
	if (f54->query_13.has_ctrl87)
		offset += 1;

	/* query 15 */
	if (f54->query.has_query15) {
		retval = synaptics_rmi4_reg_read(rmi4_data,
				f54->query_base_addr + offset,
				f54->query_15.data,
				sizeof(f54->query_15.data));
		if (retval < 0)
			return retval;
		offset += 1;
	}

	/* query 16 */
	if (f54->query_15.has_query16) {
		retval = synaptics_rmi4_reg_read(rmi4_data,
				f54->query_base_addr + offset,
				f54->query_16.data,
				sizeof(f54->query_16.data));
		if (retval < 0)
			return retval;
		offset += 1;
	}

	/* query 17 */
	if (f54->query_16.has_query17)
		offset += 1;

	/* query 18 */
	if (f54->query_16.has_ctrl94_query18)
		offset += 1;

	/* query 19 */
	if (f54->query_16.has_ctrl95_query19)
		offset += 1;

	/* query 20 */
	if (f54->query_15.has_query20)
		offset += 1;

	/* query 21 */
	if (f54->query_15.has_query21) {
		retval = synaptics_rmi4_reg_read(rmi4_data,
				f54->query_base_addr + offset,
				f54->query_21.data,
				sizeof(f54->query_21.data));
		if (retval < 0)
			return retval;
		offset += 1;
	}

	/* query 22 */
	if (f54->query_15.has_query22) {
		retval = synaptics_rmi4_reg_read(rmi4_data,
				f54->query_base_addr + offset,
				f54->query_22.data,
				sizeof(f54->query_22.data));
		if (retval < 0)
			return retval;
		offset += 1;
	}

	/* query 23 */
	if (f54->query_22.has_query23) {
		retval = synaptics_rmi4_reg_read(rmi4_data,
				f54->query_base_addr + offset,
				f54->query_23.data,
				sizeof(f54->query_23.data));
		if (retval < 0)
			return retval;
		offset += 1;
	}

	/* query 24 */
	if (f54->query_21.has_query24_data18)
		offset += 1;

	/* query 25 */
	if (f54->query_15.has_query25) {
		retval = synaptics_rmi4_reg_read(rmi4_data,
				f54->query_base_addr + offset,
				f54->query_25.data,
				sizeof(f54->query_25.data));
		if (retval < 0)
			return retval;
		offset += 1;
	}

	/* query 26 */
	if (f54->query_22.has_ctrl103_query26)
		offset += 1;

	/* query 27 */
	if (f54->query_25.has_query27) {
		retval = synaptics_rmi4_reg_read(rmi4_data,
				f54->query_base_addr + offset,
				f54->query_27.data,
				sizeof(f54->query_27.data));
		if (retval < 0)
			return retval;
		offset += 1;
	}

	/* query 28 */
	if (f54->query_22.has_query28)
		offset += 1;

	/* query 29 */
	if (f54->query_27.has_query29) {
		retval = synaptics_rmi4_reg_read(rmi4_data,
				f54->query_base_addr + offset,
				f54->query_29.data,
				sizeof(f54->query_29.data));
		if (retval < 0)
			return retval;
		offset += 1;
	}

	/* query 30 */
	if (f54->query_29.has_query30) {
		retval = synaptics_rmi4_reg_read(rmi4_data,
				f54->query_base_addr + offset,
				f54->query_30.data,
				sizeof(f54->query_30.data));
		if (retval < 0)
			return retval;
		offset += 1;
	}

	/* query 31 */
	if (f54->query_30.has_ctrl122_query31)
		offset += 1;

	/* query 32 */
	if (f54->query_30.has_query32) {
		retval = synaptics_rmi4_reg_read(rmi4_data,
				f54->query_base_addr + offset,
				f54->query_32.data,
				sizeof(f54->query_32.data));
		if (retval < 0)
			return retval;
		offset += 1;
	}

	/* query 33 */
	if (f54->query_32.has_query33) {
		retval = synaptics_rmi4_reg_read(rmi4_data,
				f54->query_base_addr + offset,
				f54->query_33.data,
				sizeof(f54->query_33.data));
		if (retval < 0)
			return retval;
		offset += 1;
	}

	/* query 34 */
	if (f54->query_32.has_query34)
		offset += 1;

	/* query 35 */
	if (f54->query_32.has_query35) {
		retval = synaptics_rmi4_reg_read(rmi4_data,
				f54->query_base_addr + offset,
				f54->query_35.data,
				sizeof(f54->query_35.data));
		if (retval < 0)
			return retval;
		offset += 1;
	}

	/* query 36 */
	if (f54->query_33.has_query36) {
		retval = synaptics_rmi4_reg_read(rmi4_data,
				f54->query_base_addr + offset,
				f54->query_36.data,
				sizeof(f54->query_36.data));
		if (retval < 0)
			return retval;
		offset += 1;
	}

	/* query 37 */
	if (f54->query_36.has_query37)
		offset += 1;

	/* query 38 */
	if (f54->query_36.has_query38) {
		retval = synaptics_rmi4_reg_read(rmi4_data,
				f54->query_base_addr + offset,
				f54->query_38.data,
				sizeof(f54->query_38.data));
		if (retval < 0)
			return retval;
		offset += 1;
	}

	/* query 39 */
	if (f54->query_38.has_query39) {
		retval = synaptics_rmi4_reg_read(rmi4_data,
				f54->query_base_addr + offset,
				f54->query_39.data,
				sizeof(f54->query_39.data));
		if (retval < 0)
			return retval;
		offset += 1;
	}

	/* query 40 */
	if (f54->query_39.has_query40) {
		retval = synaptics_rmi4_reg_read(rmi4_data,
				f54->query_base_addr + offset,
				f54->query_40.data,
				sizeof(f54->query_40.data));
		if (retval < 0)
			return retval;
		offset += 1;
	}

	/* query 41 */
	if (f54->query_40.has_ctrl163_query41)
		offset += 1;

	/* query 42 */
	if (f54->query_40.has_ctrl165_query42)
		offset += 1;

	/* query 43 */
	if (f54->query_40.has_query43) {
		retval = synaptics_rmi4_reg_read(rmi4_data,
				f54->query_base_addr + offset,
				f54->query_43.data,
				sizeof(f54->query_43.data));
		if (retval < 0)
			return retval;
		offset += 1;
	}

	/* queries 44 45 reserved */

	/* query 46 */
	if (f54->query_43.has_query46) {
		retval = synaptics_rmi4_reg_read(rmi4_data,
				f54->query_base_addr + offset,
				f54->query_46.data,
				sizeof(f54->query_46.data));
		if (retval < 0)
			return retval;
		offset += 1;
	}

	/* query 47 */
	if (f54->query_46.has_query47) {
		retval = synaptics_rmi4_reg_read(rmi4_data,
				f54->query_base_addr + offset,
				f54->query_47.data,
				sizeof(f54->query_47.data));
		if (retval < 0)
			return retval;
		offset += 1;
	}

	/* query 48 reserved */

	/* query 49 */
	if (f54->query_47.has_query49) {
		retval = synaptics_rmi4_reg_read(rmi4_data,
				f54->query_base_addr + offset,
				f54->query_49.data,
				sizeof(f54->query_49.data));
		if (retval < 0)
			return retval;
		offset += 1;
	}

	/* query 50 */
	if (f54->query_49.has_query50) {
		retval = synaptics_rmi4_reg_read(rmi4_data,
				f54->query_base_addr + offset,
				f54->query_50.data,
				sizeof(f54->query_50.data));
		if (retval < 0)
			return retval;
		offset += 1;
	}

	/* query 51 */
	if (f54->query_50.has_query51) {
		retval = synaptics_rmi4_reg_read(rmi4_data,
				f54->query_base_addr + offset,
				f54->query_51.data,
				sizeof(f54->query_51.data));
		if (retval < 0)
			return retval;
		offset += 1;
	}

	return 0;
}

static void test_f54_set_regs(struct synaptics_rmi4_data *rmi4_data,
		struct synaptics_rmi4_fn_desc *fd,
		unsigned int intr_count,
		unsigned char page)
{
	unsigned char ii;
	unsigned char intr_offset;

	f54->query_base_addr = fd->query_base_addr | (page << 8);
	f54->control_base_addr = fd->ctrl_base_addr | (page << 8);
	f54->data_base_addr = fd->data_base_addr | (page << 8);
	f54->command_base_addr = fd->cmd_base_addr | (page << 8);

	f54->intr_reg_num = (intr_count + 7) / 8;
	if (f54->intr_reg_num != 0)
		f54->intr_reg_num -= 1;

	f54->intr_mask = 0;
	intr_offset = intr_count % 8;
	for (ii = intr_offset;
			ii < (fd->intr_src_count + intr_offset);
			ii++) {
		f54->intr_mask |= 1 << ii;
	}

	return;
}

static int test_f55_set_controls(void)
{
	unsigned char offset = 0;

	/* controls 0 1 2 */
	if (f55->query.has_sensor_assignment)
		offset += 3;

	/* control 3 */
	if (f55->query.has_edge_compensation)
		offset++;

	/* control 4 */
	if (f55->query.curve_compensation_mode == 0x1 ||
			f55->query.curve_compensation_mode == 0x2)
		offset++;

	/* control 5 */
	if (f55->query.curve_compensation_mode == 0x2)
		offset++;

	/* control 6 */
	if (f55->query.has_ctrl6)
		offset++;

	/* control 7 */
	if (f55->query.has_alternate_transmitter_assignment)
		offset++;

	/* control 8 */
	if (f55->query_3.has_ctrl8)
		offset++;

	/* control 9 */
	if (f55->query_3.has_ctrl9)
		offset++;

	/* control 10 */
	if (f55->query_5.has_corner_compensation)
		offset++;

	/* control 11 */
	if (f55->query.curve_compensation_mode == 0x3)
		offset++;

	/* control 12 */
	if (f55->query_5.has_ctrl12)
		offset++;

	/* control 13 */
	if (f55->query_5.has_ctrl13)
		offset++;

	/* control 14 */
	if (f55->query_5.has_ctrl14)
		offset++;

	/* control 15 */
	if (f55->query_5.has_basis_function)
		offset++;

	/* control 16 */
	if (f55->query_17.has_ctrl16)
		offset++;

	/* control 17 */
	if (f55->query_17.has_ctrl17)
		offset++;

	/* controls 18 19 */
	if (f55->query_17.has_ctrl18_ctrl19)
		offset += 2;

	/* control 20 */
	if (f55->query_17.has_ctrl20)
		offset++;

	/* control 21 */
	if (f55->query_17.has_ctrl21)
		offset++;

	/* control 22 */
	if (f55->query_17.has_ctrl22)
		offset++;

	/* control 23 */
	if (f55->query_18.has_ctrl23)
		offset++;

	/* control 24 */
	if (f55->query_18.has_ctrl24)
		offset++;

	/* control 25 */
	if (f55->query_18.has_ctrl25)
		offset++;

	/* control 26 */
	if (f55->query_18.has_ctrl26)
		offset++;

	/* control 27 */
	if (f55->query_18.has_ctrl27_query20)
		offset++;

	/* control 28 */
	if (f55->query_18.has_ctrl28_query21)
		offset++;

	/* control 29 */
	if (f55->query_22.has_ctrl29)
		offset++;

	/* control 30 */
	if (f55->query_22.has_ctrl30)
		offset++;

	/* control 31 */
	if (f55->query_22.has_ctrl31)
		offset++;

	/* control 32 */
	if (f55->query_22.has_ctrl32)
		offset++;

	/* controls 33 34 35 36 reserved */

	/* control 37 */
	if (f55->query_28.has_ctrl37)
		offset++;

	/* control 38 */
	if (f55->query_30.has_ctrl38)
		offset++;

	/* control 39 */
	if (f55->query_30.has_ctrl39)
		offset++;

	/* control 40 */
	if (f55->query_30.has_ctrl40)
		offset++;

	/* control 41 */
	if (f55->query_30.has_ctrl41)
		offset++;

	/* control 42 */
	if (f55->query_30.has_ctrl42)
		offset++;

	/* controls 43 44 */
	if (f55->query_30.has_ctrl43_ctrl44) {
		f55->afe_mux_offset = offset;
		offset += 2;
	}

	/* controls 45 46 */
	if (f55->query_33.has_ctrl45_ctrl46) {
		f55->has_force = true;
		f55->force_tx_offset = offset;
		f55->force_rx_offset = offset + 1;
		offset += 2;
	}

	return 0;
}

static int test_f55_set_queries(void)
{
	int retval;
	unsigned char offset;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	retval = synaptics_rmi4_reg_read(rmi4_data,
			f55->query_base_addr,
			f55->query.data,
			sizeof(f55->query.data));
	if (retval < 0)
		return retval;

	offset = sizeof(f55->query.data);

	/* query 3 */
	if (f55->query.has_single_layer_multi_touch) {
		retval = synaptics_rmi4_reg_read(rmi4_data,
				f55->query_base_addr + offset,
				f55->query_3.data,
				sizeof(f55->query_3.data));
		if (retval < 0)
			return retval;
		offset += 1;
	}

	/* query 4 */
	if (f55->query_3.has_ctrl9)
		offset += 1;

	/* query 5 */
	if (f55->query.has_query5) {
		retval = synaptics_rmi4_reg_read(rmi4_data,
				f55->query_base_addr + offset,
				f55->query_5.data,
				sizeof(f55->query_5.data));
		if (retval < 0)
			return retval;
		offset += 1;
	}

	/* queries 6 7 */
	if (f55->query.curve_compensation_mode == 0x3)
		offset += 2;

	/* query 8 */
	if (f55->query_3.has_ctrl8)
		offset += 1;

	/* query 9 */
	if (f55->query_3.has_query9)
		offset += 1;

	/* queries 10 11 12 13 14 15 16 */
	if (f55->query_5.has_basis_function)
		offset += 7;

	/* query 17 */
	if (f55->query_5.has_query17) {
		retval = synaptics_rmi4_reg_read(rmi4_data,
				f55->query_base_addr + offset,
				f55->query_17.data,
				sizeof(f55->query_17.data));
		if (retval < 0)
			return retval;
		offset += 1;
	}

	/* query 18 */
	if (f55->query_17.has_query18) {
		retval = synaptics_rmi4_reg_read(rmi4_data,
				f55->query_base_addr + offset,
				f55->query_18.data,
				sizeof(f55->query_18.data));
		if (retval < 0)
			return retval;
		offset += 1;
	}

	/* query 19 */
	if (f55->query_18.has_query19)
		offset += 1;

	/* query 20 */
	if (f55->query_18.has_ctrl27_query20)
		offset += 1;

	/* query 21 */
	if (f55->query_18.has_ctrl28_query21)
		offset += 1;

	/* query 22 */
	if (f55->query_18.has_query22) {
		retval = synaptics_rmi4_reg_read(rmi4_data,
				f55->query_base_addr + offset,
				f55->query_22.data,
				sizeof(f55->query_22.data));
		if (retval < 0)
			return retval;
		offset += 1;
	}

	/* query 23 */
	if (f55->query_22.has_query23) {
		retval = synaptics_rmi4_reg_read(rmi4_data,
				f55->query_base_addr + offset,
				f55->query_23.data,
				sizeof(f55->query_23.data));
		if (retval < 0)
			return retval;
		offset += 1;

		f55->amp_sensor = f55->query_23.amp_sensor_enabled;
		f55->size_of_column2mux = f55->query_23.size_of_column2mux;
	}

	/* queries 24 25 26 27 reserved */

	/* query 28 */
	if (f55->query_22.has_query28) {
		retval = synaptics_rmi4_reg_read(rmi4_data,
				f55->query_base_addr + offset,
				f55->query_28.data,
				sizeof(f55->query_28.data));
		if (retval < 0)
			return retval;
		offset += 1;
	}

	/* query 29 */
	if (f55->query_28.has_query29)
		offset += 1;

	/* query 30 */
	if (f55->query_28.has_query30) {
		retval = synaptics_rmi4_reg_read(rmi4_data,
				f55->query_base_addr + offset,
				f55->query_30.data,
				sizeof(f55->query_30.data));
		if (retval < 0)
			return retval;
		offset += 1;
	}

	/* queries 31 32 */
	if (f55->query_30.has_query31_query32)
		offset += 2;

	/* query 33 */
	if (f55->query_30.has_query33) {
		retval = synaptics_rmi4_reg_read(rmi4_data,
				f55->query_base_addr + offset,
				f55->query_33.data,
				sizeof(f55->query_33.data));
		if (retval < 0)
			return retval;
		offset += 1;

		f55->extended_amp = f55->query_33.has_extended_amp_pad;
	}

	return 0;
}

static void test_f55_init(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	unsigned char ii;
	unsigned char rx_electrodes;
	unsigned char tx_electrodes;
	struct f55_control_43 ctrl_43;

	retval = test_f55_set_queries();
	if (retval < 0) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: Failed to read F55 query registers\n",
				__func__);
		return;
	}

	if (!f55->query.has_sensor_assignment)
		return;

	retval = test_f55_set_controls();
	if (retval < 0) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: Failed to set up F55 control registers\n",
				__func__);
		return;
	}

	tx_electrodes = f55->query.num_of_tx_electrodes;
	rx_electrodes = f55->query.num_of_rx_electrodes;

	f55->tx_assignment = kzalloc(tx_electrodes, GFP_KERNEL);
	f55->rx_assignment = kzalloc(rx_electrodes, GFP_KERNEL);

	retval = synaptics_rmi4_reg_read(rmi4_data,
			f55->control_base_addr + SENSOR_TX_MAPPING_OFFSET,
			f55->tx_assignment,
			tx_electrodes);
	if (retval < 0) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: Failed to read F55 tx assignment\n",
				__func__);
		return;
	}

	retval = synaptics_rmi4_reg_read(rmi4_data,
			f55->control_base_addr + SENSOR_RX_MAPPING_OFFSET,
			f55->rx_assignment,
			rx_electrodes);
	if (retval < 0) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: Failed to read F55 rx assignment\n",
				__func__);
		return;
	}

	f54->tx_assigned = 0;
	for (ii = 0; ii < tx_electrodes; ii++) {
		if (f55->tx_assignment[ii] != 0xff)
			f54->tx_assigned++;
	}

	f54->rx_assigned = 0;
	for (ii = 0; ii < rx_electrodes; ii++) {
		if (f55->rx_assignment[ii] != 0xff)
			f54->rx_assigned++;
	}

	if (f55->amp_sensor) {
		f54->tx_assigned = f55->size_of_column2mux;
		f54->rx_assigned /= 2;
	}

	if (f55->extended_amp) {
		retval = synaptics_rmi4_reg_read(rmi4_data,
				f55->control_base_addr + f55->afe_mux_offset,
				ctrl_43.data,
				sizeof(ctrl_43.data));
		if (retval < 0) {
			dev_err(rmi4_data->pdev->dev.parent,
					"%s: Failed to read F55 AFE mux sizes\n",
					__func__);
			return;
		}

		f54->tx_assigned = ctrl_43.afe_l_mux_size +
				ctrl_43.afe_r_mux_size;
	}

	/* force mapping */
	if (f55->has_force) {
		f55->force_tx_assignment = kzalloc(tx_electrodes, GFP_KERNEL);
		f55->force_rx_assignment = kzalloc(rx_electrodes, GFP_KERNEL);

		retval = synaptics_rmi4_reg_read(rmi4_data,
				f55->control_base_addr + f55->force_tx_offset,
				f55->force_tx_assignment,
				tx_electrodes);
		if (retval < 0) {
			dev_err(rmi4_data->pdev->dev.parent,
					"%s: Failed to read F55 force tx assignment\n",
					__func__);
			return;
		}

		retval = synaptics_rmi4_reg_read(rmi4_data,
				f55->control_base_addr + f55->force_rx_offset,
				f55->force_rx_assignment,
				rx_electrodes);
		if (retval < 0) {
			dev_err(rmi4_data->pdev->dev.parent,
					"%s: Failed to read F55 force rx assignment\n",
					__func__);
			return;
		}

		for (ii = 0; ii < tx_electrodes; ii++) {
			if (f55->force_tx_assignment[ii] != 0xff)
				f54->tx_assigned++;
		}

		for (ii = 0; ii < rx_electrodes; ii++) {
			if (f55->force_rx_assignment[ii] != 0xff)
				f54->rx_assigned++;
		}
	}

	return;
}

static void test_f55_set_regs(struct synaptics_rmi4_data *rmi4_data,
		struct synaptics_rmi4_fn_desc *fd,
		unsigned char page)
{
	f55 = kzalloc(sizeof(*f55), GFP_KERNEL);
	if (!f55) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: Failed to alloc mem for F55\n",
				__func__);
		return;
	}

	f55->query_base_addr = fd->query_base_addr | (page << 8);
	f55->control_base_addr = fd->ctrl_base_addr | (page << 8);
	f55->data_base_addr = fd->data_base_addr | (page << 8);
	f55->command_base_addr = fd->cmd_base_addr | (page << 8);

	return;
}

static void test_f21_init(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	unsigned char ii;
	unsigned char size_of_query2;
	unsigned char size_of_query5;
	unsigned char query_11_offset;
	unsigned char ctrl_4_offset;
	struct f21_query_2 *query_2 = NULL;
	struct f21_query_5 *query_5 = NULL;
	struct f21_query_11 *query_11 = NULL;

	query_2 = kzalloc(sizeof(*query_2), GFP_KERNEL);
	if (!query_2) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: Failed to alloc mem for query_2\n",
				__func__);
		goto exit;
	}

	query_5 = kzalloc(sizeof(*query_5), GFP_KERNEL);
	if (!query_5) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: Failed to alloc mem for query_5\n",
				__func__);
		goto exit;
	}

	query_11 = kzalloc(sizeof(*query_11), GFP_KERNEL);
	if (!query_11) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: Failed to alloc mem for query_11\n",
				__func__);
		goto exit;
	}

	retval = synaptics_rmi4_reg_read(rmi4_data,
			f21->query_base_addr + 1,
			&size_of_query2,
			sizeof(size_of_query2));
	if (retval < 0)
		goto exit;

	if (size_of_query2 > sizeof(query_2->data))
		size_of_query2 = sizeof(query_2->data);

	retval = synaptics_rmi4_reg_read(rmi4_data,
			f21->query_base_addr + 2,
			query_2->data,
			size_of_query2);
	if (retval < 0)
		goto exit;

	if (!query_2->query11_is_present) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: No F21 force capabilities\n",
				__func__);
		goto exit;
	}

	query_11_offset = query_2->query0_is_present +
			query_2->query1_is_present +
			query_2->query2_is_present +
			query_2->query3_is_present +
			query_2->query4_is_present +
			query_2->query5_is_present +
			query_2->query6_is_present +
			query_2->query7_is_present +
			query_2->query8_is_present +
			query_2->query9_is_present +
			query_2->query10_is_present;

	retval = synaptics_rmi4_reg_read(rmi4_data,
			f21->query_base_addr + 11,
			query_11->data,
			sizeof(query_11->data));
	if (retval < 0)
		goto exit;

	if (!query_11->has_force_sensing_txrx_mapping) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: No F21 force mapping\n",
				__func__);
		goto exit;
	}

	f21->max_num_of_tx = query_11->max_number_of_force_txs;
	f21->max_num_of_rx = query_11->max_number_of_force_rxs;
	f21->max_num_of_txrx = f21->max_num_of_tx + f21->max_num_of_rx;

	f21->force_txrx_assignment = kzalloc(f21->max_num_of_txrx, GFP_KERNEL);

	retval = synaptics_rmi4_reg_read(rmi4_data,
			f21->query_base_addr + 4,
			&size_of_query5,
			sizeof(size_of_query5));
	if (retval < 0)
		goto exit;

	if (size_of_query5 > sizeof(query_5->data))
		size_of_query5 = sizeof(query_5->data);

	retval = synaptics_rmi4_reg_read(rmi4_data,
			f21->query_base_addr + 5,
			query_5->data,
			size_of_query5);
	if (retval < 0)
		goto exit;

	ctrl_4_offset = query_5->ctrl0_is_present +
			query_5->ctrl1_is_present +
			query_5->ctrl2_is_present +
			query_5->ctrl3_is_present;

	retval = synaptics_rmi4_reg_read(rmi4_data,
			f21->control_base_addr + ctrl_4_offset,
			f21->force_txrx_assignment,
			f21->max_num_of_txrx);
	if (retval < 0) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: Failed to read F21 force txrx assignment\n",
				__func__);
		goto exit;
	}

	f21->has_force = true;

	for (ii = 0; ii < f21->max_num_of_tx; ii++) {
		if (f21->force_txrx_assignment[ii] != 0xff)
			f54->tx_assigned++;
	}

	for (ii = f21->max_num_of_tx; ii < f21->max_num_of_txrx; ii++) {
		if (f21->force_txrx_assignment[ii] != 0xff)
			f54->rx_assigned++;
	}

exit:
	kfree(query_2);
	kfree(query_5);
	kfree(query_11);

	return;
}

static void test_f21_set_regs(struct synaptics_rmi4_data *rmi4_data,
		struct synaptics_rmi4_fn_desc *fd,
		unsigned char page)
{
	f21 = kzalloc(sizeof(*f21), GFP_KERNEL);
	if (!f21) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: Failed to alloc mem for F21\n",
				__func__);
		return;
	}

	f21->query_base_addr = fd->query_base_addr | (page << 8);
	f21->control_base_addr = fd->ctrl_base_addr | (page << 8);
	f21->data_base_addr = fd->data_base_addr | (page << 8);
	f21->command_base_addr = fd->cmd_base_addr | (page << 8);

	return;
}

static int test_scan_pdt(void)
{
	int retval;
	unsigned char intr_count = 0;
	unsigned char page;
	unsigned short addr;
	bool f54found = false;
	bool f55found = false;
	struct synaptics_rmi4_fn_desc rmi_fd;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	for (page = 0; page < PAGES_TO_SERVICE; page++) {
		for (addr = PDT_START; addr > PDT_END; addr -= PDT_ENTRY_SIZE) {
			addr |= (page << 8);

			retval = synaptics_rmi4_reg_read(rmi4_data,
					addr,
					(unsigned char *)&rmi_fd,
					sizeof(rmi_fd));
			if (retval < 0)
				return retval;

			addr &= ~(MASK_8BIT << 8);

			if (!rmi_fd.fn_number)
				break;

			switch (rmi_fd.fn_number) {
			case SYNAPTICS_RMI4_F54:
				test_f54_set_regs(rmi4_data,
						&rmi_fd, intr_count, page);
				f54found = true;
				break;
			case SYNAPTICS_RMI4_F55:
				test_f55_set_regs(rmi4_data,
						&rmi_fd, page);
				f55found = true;
				break;
			case SYNAPTICS_RMI4_F21:
				test_f21_set_regs(rmi4_data,
						&rmi_fd, page);
				break;
			default:
				break;
			}

			if (f54found && f55found)
				goto pdt_done;

			intr_count += rmi_fd.intr_src_count;
		}
	}

	if (!f54found) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: Failed to find F54\n",
				__func__);
		return -EINVAL;
	}

pdt_done:
	return 0;
}

static void synaptics_rmi4_test_attn(struct synaptics_rmi4_data *rmi4_data,
		unsigned char intr_mask)
{
	if (!f54)
		return;

	if (f54->intr_mask & intr_mask)
		queue_work(f54->test_report_workqueue, &f54->test_report_work);

	return;
}

#ifdef CONFIG_DRV_SAMSUNG
#include "registerMap.h"
#include <linux/uaccess.h>
#include <linux/firmware.h>
#include <linux/sec_class.h>

#define DEFAULT_FFU_FW_PATH		"ffu_tsp.bin"
#define DEFAULT_UMS_FW_PATH		"/sdcard/Firmware/TSP/synaptics.bin"

#define TREX_DATA_SIZE			7
#define RPT_DATA_STRNCAT_LENGTH		9

#define DEBUG_RESULT_STR_LEN		1024
#define MAX_VAL_OFFSET_AND_LENGTH	10
#define DEBUG_STR_LEN			(CMD_STR_LEN * 2)

#define DEBUG_PRNT_SCREEN(_dest, _temp, _length, fmt, ...)	\
({								\
	snprintf(_temp, _length, fmt, ## __VA_ARGS__);		\
	strcat(_dest, _temp);					\
})

enum {
	BUILT_IN = 0,
	UMS,
	BUILT_IN_FAC,
	FFU,
};

enum CMD_STATUS {
	CMD_STATUS_WAITING = 0,
	CMD_STATUS_RUNNING,
	CMD_STATUS_OK,
	CMD_STATUS_FAIL,
	CMD_STATUS_NOT_APPLICABLE,
};

#define SEC_CMD(name, func) .cmd_name = name, .cmd_func = func

struct sec_cmd {
	const char *cmd_name;
	void (*cmd_func)(void *dev_data);
	struct list_head list;
};

static inline void set_default_result(struct cmd_data *info)
{
	char delim = ':';

	memset(info->cmd_buff, 0x00, sizeof(info->cmd_buff));
	memset(info->cmd_result, 0x00, sizeof(info->cmd_result));
	memcpy(info->cmd_result, info->cmd, strlen(info->cmd));
	strncat(info->cmd_result, &delim, 1);

	return;
}

static inline void set_cmd_result(struct cmd_data *info, int length)
{
	strncat(info->cmd_result, info->cmd_buff, length);

	return;
}

/*
 * Factory CMD
 *
 * fw_update :	0 (Update with internal firmware).
 *				1 (Update with external firmware).
 *				2 (Update with Internal factory firmware).
 *				3 (Update with FFU firmware from air).
 * get_fw_ver_bin : Display firmware version in binary.
 * get_fw_ver_ic : Display firmware version in IC.
 * get_config_ver : Display configuration version.
 * get_checksum_data : Display PR number.
 * get_threshold : Display threshold of mutual.
 * module_off/on_master/slave : Control ot touch IC's power.
 * get_chip_vendor : Display vendor name.
 * get_chip_name : Display chip name.
 * get_x/y_num : Return RX/TX line of IC.
 * get_rawcap : Return the rawcap(mutual) about selected.
 * run_rawcap_read : Get the rawcap(mutual value about entire screen.
 * get_delta : Return the delta(mutual jitter) about selected.
 * run_delta_read : Get the delta value about entire screen.
 * run_abscap_read : Get the abscap(self) value about entire screen.
 * get_abscap_read_test : Return the abscap(self) RX/TX MIN & MAX value about entire screen.
 * run_absdelta_read : Get the absdelta(self jitter) value about entire screen.
 * run_trx_short_test : Test for open/short state each node.
 *		(each node return the valu ->  0: ok 1: not ok).
 * dead_zone_enable : Set dead zone mode on(1)/off(0).
 * hover_enable : To control the hover functionality dinamically.
 *		( 0: disalbe, 1: enable)
 * hover_no_sleep_enable : To keep the no sleep state before enter the hover test.
 *		This command was requested by Display team /HW.
 * hover_set_edge_rx : To change grip edge exclustion RX value during hover factory test.
 * glove_mode : Set glove mode on/off
 * clear_cover_mode : Set the touch sensitivity mode. we are supporting various mode
		in sensitivity such as (glove, flip cover, clear cover mode) and they are controled
		by this sysfs.
 * get_glove_sensitivity : Display glove's sensitivity.
 * fast_glove_mode : Set the fast glove mode such as incomming screen.
 * secure_mode : Set the secure mode.
 * boost_level : Control touch booster level.
 * handgrip_enable : Enable reporting the grip infomation based on hover shape.
 * set_tsp_test_result : Write the result of tsp test in config area.
 * get_tsp_test_result : Read the result of tsp test in config area.
 */

#ifndef CONFIG_TOUCHSCREEN_SYNAPTICS_DSX_FW_UPDATE_v26
int synaptics_fw_updater(const unsigned char *fw_data, bool force)
{
	return -EPERM;
}

void get_device_fw_ver(char *buff, int buff_size)
{
	memset(buff, 0x00, buff_size);
}

void get_image_fw_ver(char *buff, int buff_size)
{
	memset(buff, 0x00, buff_size);
}
#endif

static int load_fw_from_kernel(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;

	retval = synaptics_fw_updater(NULL, true);
	if (retval)
		dev_err(rmi4_data->pdev->dev.parent,
			"%s: Failed to update firmware\n", __func__);

	return retval;
}

static int load_fw_from_ffu(struct synaptics_rmi4_data *rmi4_data)
{
	const struct firmware *fw_entry = NULL;
	const char *fw_path = DEFAULT_FFU_FW_PATH;
	int retval;

	retval = request_firmware(&fw_entry, fw_path,
				rmi4_data->pdev->dev.parent);
	if (retval) {
		dev_err(rmi4_data->pdev->dev.parent,
			"%s: Firmware %s not available\n",
			__func__, fw_path);
		goto out;
	}

	retval = synaptics_fw_updater(fw_entry->data, true);
	if (retval)
		dev_err(rmi4_data->pdev->dev.parent,
			"%s: Failed to update firmware\n", __func__);
out:
	if (fw_entry)
		release_firmware(fw_entry);

	return retval;
}

static int load_fw_from_ums(struct synaptics_rmi4_data *rmi4_data)
{
	mm_segment_t old_fs;
	struct file *fp;
	const char *fw_path = DEFAULT_UMS_FW_PATH;
	int fw_size, nread;
	int retval = 0;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	fp = filp_open(fw_path, O_RDONLY, S_IRUSR);
	if (IS_ERR(fp)) {
		dev_err(rmi4_data->pdev->dev.parent,
			"%s: Failed to open %s\n",
			__func__, fw_path);
		retval = PTR_ERR(fp);
		goto exit_open;
	}

	fw_size = fp->f_path.dentry->d_inode->i_size;

	if (fw_size) {
		unsigned char *fw_data;

		fw_data = kzalloc(fw_size, GFP_KERNEL);
		nread = vfs_read(fp, (char __user *)fw_data,
						fw_size, &fp->f_pos);

		dev_info(rmi4_data->pdev->dev.parent,
			"%s: start, file path %s, size %u Bytes\n",
			__func__, fw_path, fw_size);

		if (nread != fw_size) {
			dev_err(rmi4_data->pdev->dev.parent,
				"%s: Failed to read firmware, nread %u Bytes\n",
			       __func__, nread);
			retval = -EIO;
		} else {
			retval = synaptics_fw_updater(fw_data, true);
		}

		if (retval)
			dev_err(rmi4_data->pdev->dev.parent,
				"%s: Failed to update firmware\n", __func__);

		kfree(fw_data);
	}

	filp_close(fp, current->files);

exit_open:
	set_fs(old_fs);

	return retval;
}

static int synaptics_rmi4_fw_update_on_hidden_menu(struct synaptics_rmi4_data *rmi4_data,
		int update_type)
{
	int retval = 0;

	/* Factory cmd for firmware update
	 * argument represent what is source of firmware like below.
	 *
	 * 0, 2 : Getting firmware which is for user.
	 * 1 : Getting firmware from sd card.
	 */
	switch (update_type) {
	case BUILT_IN:
	case BUILT_IN_FAC:
		retval = load_fw_from_kernel(rmi4_data);
		break;
	case UMS:
		retval = load_fw_from_ums(rmi4_data);
		break;
	case FFU:
		retval = load_fw_from_ffu(rmi4_data);
		break;
	default:
		dev_err(rmi4_data->pdev->dev.parent,
			"%s: Not support update type[%d]\n",
			__func__, update_type);
		break;
	}

	return retval;
}

static void fw_update(void *dev_data)
{
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;
	struct cmd_data *data = f54->cmd_data;
	int retval = 0;

	set_default_result(data);

	retval = synaptics_rmi4_fw_update_on_hidden_menu(rmi4_data,
			data->cmd_param[0]);

	if (retval) {
		snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", "NA");
		data->cmd_state = CMD_STATUS_FAIL;
		dev_err(rmi4_data->pdev->dev.parent,
			"%s: Failed\n", __func__);
		goto out;
	}

	snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", "OK");
	data->cmd_state = CMD_STATUS_OK;
	dev_info(rmi4_data->pdev->dev.parent,
			"%s: Succeeded\n", __func__);

out:
	set_cmd_result(data, strlen(data->cmd_buff));
}

static void get_fw_ver_bin(void *dev_data)
{
	struct cmd_data *data = f54->cmd_data;
	char buff[3];

	set_default_result(data);

	get_image_fw_ver(buff, sizeof(buff));

	snprintf(data->cmd_buff, CMD_RESULT_STR_LEN,
			"SY00%02X%02X", buff[0], buff[1]);
	data->cmd_state = CMD_STATUS_OK;
	set_cmd_result(data, strlen(data->cmd_buff));
}

static void get_fw_ver_ic(void *dev_data)
{
	struct cmd_data *data = f54->cmd_data;
	char buff[3];

	set_default_result(data);

	get_device_fw_ver(buff, sizeof(buff));

	snprintf(data->cmd_buff, CMD_RESULT_STR_LEN,
			"SY00%02X%02X", buff[0], buff[1]);
	data->cmd_state = CMD_STATUS_OK;
	data->cmd_state = CMD_STATUS_OK;
	set_cmd_result(data, strlen(data->cmd_buff));
}

static void get_config_ver(void *dev_data)
{
	struct cmd_data *data = f54->cmd_data;

	set_default_result(data);
	snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", "NA");
	data->cmd_state = CMD_STATUS_OK;
	set_cmd_result(data, strlen(data->cmd_buff));
}

static void get_checksum_data(void *dev_data)
{
	struct cmd_data *data = f54->cmd_data;

	set_default_result(data);
	snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", "NA");
	data->cmd_state = CMD_STATUS_OK;
	set_cmd_result(data, strlen(data->cmd_buff));
}

static void get_threshold(void *dev_data)
{
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;
	struct cmd_data *data = f54->cmd_data;
	unsigned char saturationcap_lsb;
	unsigned char saturationcap_msb;
	unsigned char amplitudethreshold;
	unsigned int saturationcap;
	unsigned int threshold_integer;
	unsigned int threshold_fraction;
	int retval;

	set_default_result(data);

	retval = synaptics_rmi4_reg_read(rmi4_data,
					SYNA_F12_2D_CTRL15_00_00,
					&amplitudethreshold,
					sizeof(amplitudethreshold));
	if (retval < 0)
		goto out;
	
	retval = synaptics_rmi4_reg_read(rmi4_data,
					SYNA_F54_ANALOG_CTRL02_00,
					&saturationcap_lsb,
					sizeof(saturationcap_lsb));
	if (retval < 0)
		goto out;

	retval = synaptics_rmi4_reg_read(rmi4_data,
					SYNA_F54_ANALOG_CTRL02_01,
					&saturationcap_msb,
					sizeof(saturationcap_msb));
	if (retval < 0)
		goto out;

	saturationcap = (saturationcap_lsb & 0xFF) |
				((saturationcap_msb & 0xFF) << 8);
	threshold_integer = (amplitudethreshold * saturationcap) / 256;
	threshold_fraction = ((amplitudethreshold * saturationcap * 1000) /
				256) % 1000;

	dev_info(rmi4_data->pdev->dev.parent,
			"%s: FingerAmp : %d, Satruration cap : %d\n",
			__func__, amplitudethreshold, saturationcap);

	snprintf(data->cmd_buff, CMD_RESULT_STR_LEN,
			"%u.%u", threshold_integer, threshold_fraction);
	data->cmd_state = CMD_STATUS_OK;
	set_cmd_result(data, strlen(data->cmd_buff));

	return;

out:
	dev_err(rmi4_data->pdev->dev.parent,
			"%s: Failed to read\n", __func__);
	snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", "NA");
	data->cmd_state = CMD_STATUS_FAIL;
	set_cmd_result(data, strlen(data->cmd_buff));
}

static void module_off_master(void *dev_data)
{
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;
	struct input_dev *dev = rmi4_data->input_dev;
	struct cmd_data *data = f54->cmd_data;

	set_default_result(data);

	mutex_lock(&rmi4_data->input_dev->mutex);

	synaptics_rmi4_close(dev);

	mutex_unlock(&rmi4_data->input_dev->mutex);

	snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", "OK");
	data->cmd_state = CMD_STATUS_OK;

	set_cmd_result(data, strlen(data->cmd_buff));
}

static void module_on_master(void *dev_data)
{
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;
	struct input_dev *dev = rmi4_data->input_dev;
	struct cmd_data *data = f54->cmd_data;
	int retval;

	set_default_result(data);

	mutex_lock(&rmi4_data->input_dev->mutex);

	retval = synaptics_rmi4_open(dev);
	if (retval < 0)
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: Failed to start device\n", __func__);

	mutex_unlock(&rmi4_data->input_dev->mutex);

	snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", "OK");
	data->cmd_state = CMD_STATUS_OK;

	set_cmd_result(data, strlen(data->cmd_buff));
}

static void get_chip_vendor(void *dev_data)
{
	struct cmd_data *data = f54->cmd_data;

	set_default_result(data);

	snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", "SYNAPTICS");
	data->cmd_state = CMD_STATUS_OK;
	set_cmd_result(data, strlen(data->cmd_buff));
}

static void get_chip_name(void *dev_data)
{
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;
	struct cmd_data *data = f54->cmd_data;
	char buff[7] = { 0, };
	int retval;

	set_default_result(data);

	retval = synaptics_rmi4_reg_read(rmi4_data,
					SYNA_F01_RMI_QUERY11, buff, 6);
	if (retval < 0) {
		snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", "NA");
		dev_err(rmi4_data->pdev->dev.parent,
			"%s: Failed to read\n", __func__);
		goto out;
	}

	snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", buff);
	data->cmd_state = CMD_STATUS_OK;
out:
	set_cmd_result(data, strlen(data->cmd_buff));
}

static void get_x_num(void *dev_data)
{
	struct cmd_data *data = f54->cmd_data;

	set_default_result(data);
	snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%d", f54->tx_assigned);
	data->cmd_state = CMD_STATUS_OK;
	set_cmd_result(data, strlen(data->cmd_buff));
}

static void get_y_num(void *dev_data)
{
	struct cmd_data *data = f54->cmd_data;

	set_default_result(data);
	snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%d", f54->rx_assigned);
	data->cmd_state = CMD_STATUS_OK;
	set_cmd_result(data, strlen(data->cmd_buff));
}

static int check_rx_tx_num(struct synaptics_rmi4_data *rmi4_data)
{
	struct cmd_data *data = f54->cmd_data;

	int node;

	dev_info(rmi4_data->pdev->dev.parent, "%s: param[0] = %d, param[1] = %d\n",
			__func__, data->cmd_param[0], data->cmd_param[1]);

	if (data->cmd_param[0] < 0 ||
			data->cmd_param[0] >= f54->tx_assigned ||
			data->cmd_param[1] < 0 ||
			data->cmd_param[1] >= f54->rx_assigned) {
		dev_info(rmi4_data->pdev->dev.parent, "%s: parameter error: %u,%u\n",
			__func__, data->cmd_param[0], data->cmd_param[1]);
		node = -1;
	} else {
		node = data->cmd_param[0] * f54->rx_assigned +
						data->cmd_param[1];
		dev_info(rmi4_data->pdev->dev.parent, "%s: node = %d\n",
				__func__, node);
	}
	return node;
}


static void get_rawcap(void *dev_data)
{
	int node;
	short report_data;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;
	struct cmd_data *data = f54->cmd_data;	

	set_default_result(data);

	node = check_rx_tx_num(rmi4_data);

	if (node < 0) {
		snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", "NA");
		data->cmd_state = CMD_STATUS_FAIL;
		goto out;
	}

	report_data = data->rawcap_data[node];

	snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%d", report_data);
	data->cmd_state = CMD_STATUS_OK;

out:
	set_cmd_result(data, strlen(data->cmd_buff));
}

static void run_rawcap_read(void *dev_data)
{
	int retval;
	unsigned char timeout = GET_REPORT_TIMEOUT_S * 10;
	unsigned char timeout_count;

	unsigned char command;

	unsigned char ii;
	unsigned char jj;
	unsigned char kk;
	unsigned char num_of_tx;
	unsigned char num_of_rx;
	short *report_data;
	short max_value;
	short min_value;
	short cur_value;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;
	unsigned char cmd_state = CMD_STATUS_RUNNING;
	struct cmd_data *data = f54->cmd_data;	
	unsigned long setting;

	setting = (enum f54_report_types)F54_FULL_RAW_CAP_TDDI;

	set_default_result(data);

	retval = test_check_for_idle_status();
	if (retval < 0)
		goto exit;

	if (!test_report_type_valid((enum f54_report_types)setting)) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: Report type not supported by driver\n",
				__func__);
		retval = -EINVAL;
		goto exit;
	}

	f54->report_type = (enum f54_report_types)setting;
	command = (unsigned char)setting;
	retval = synaptics_rmi4_reg_write(rmi4_data,
			f54->data_base_addr,
			&command,
			sizeof(command));
	if (retval < 0) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: Failed to write report type\n",
				__func__);
		goto exit;
	}

	retval = test_do_preparation();
	if (retval < 0) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: Failed to do preparation\n",
				__func__);
		goto exit;
	}

	test_set_interrupt(true);

	command = (unsigned char)COMMAND_GET_REPORT;

	retval = synaptics_rmi4_reg_write(rmi4_data,
			f54->command_base_addr,
			&command,
			sizeof(command));
	if (retval < 0) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: Failed to write get report command\n",
				__func__);
		goto exit;
	}

	f54->status = STATUS_BUSY;
	f54->report_size = 0;
	f54->data_pos = 0;

	hrtimer_start(&f54->watchdog,
			ktime_set(GET_REPORT_TIMEOUT_S, 0),
			HRTIMER_MODE_REL);

	timeout_count = 0;
	do {
		if (f54->status != STATUS_BUSY)
			break;
		msleep(100);
		timeout_count++;
	} while (timeout_count < timeout);

	if ((f54->status != STATUS_IDLE) || (f54->report_size == 0)) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: Failed to read report\n",
				__func__);
		retval = -EINVAL;
		goto exit;
	}

	retval = synaptics_rmi4_reg_read(rmi4_data,
			rmi4_data->f01_ctrl_base_addr,
			&command,
			sizeof(command));
	if (retval < 0) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: Failed to restore no sleep setting\n",
				__func__);
		goto exit;
	}

	command = command & ~NO_SLEEP_ON;
	command |= rmi4_data->no_sleep_setting;

	retval = synaptics_rmi4_reg_write(rmi4_data,
			rmi4_data->f01_ctrl_base_addr,
			&command,
			sizeof(command));
	if (retval < 0) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: Failed to restore no sleep setting\n",
				__func__);
		goto exit;
	}

	if ((f54->query.has_query13) &&
			(f54->query_13.has_ctrl86)) {
		retval = synaptics_rmi4_reg_write(rmi4_data,
				f54->control.reg_86->address,
				f54->control.reg_86->data,
				sizeof(f54->control.reg_86->data));
		if (retval < 0) {
			dev_err(rmi4_data->pdev->dev.parent,
					"%s: Failed to restore sense display ratio\n",
					__func__);
			goto exit;
		}
	}

	test_set_interrupt(false);

	report_data = data->rawcap_data;
	memcpy(report_data, f54->report_data, f54->report_size);
	num_of_tx = f54->tx_assigned;
	num_of_rx = f54->rx_assigned;
	max_value = min_value = report_data[0];

	for (ii = 0; ii < num_of_tx; ii++) {
		for (jj = 0; jj < num_of_rx; jj++) {
			cur_value = *report_data;
			max_value = max(max_value, cur_value);
			min_value = min(min_value, cur_value);
			report_data++;

			if (cur_value > TSP_RAWCAP_MAX || cur_value < TSP_RAWCAP_MIN)
				dev_info(rmi4_data->pdev->dev.parent,
					"tx = %02d, rx = %02d, data[%d] = %d\n",
					ii, jj, kk, cur_value);
			kk++;
		}
	}

	snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%d,%d", min_value, max_value);
	cmd_state = CMD_STATUS_OK;

exit:
	rmi4_data->reset_device(rmi4_data, false);

	data->cmd_state = cmd_state;
	set_cmd_result(data, strlen(data->cmd_buff));

}

#if 0

static void get_delta(void *dev_data)
{
	int node;
	short report_data;
	struct synaptics_rmi4_data *rmi4_data = (struct synaptics_rmi4_data *)dev_data;
	struct factory_data *data = rmi4_data->f54->factory_data;

	set_default_result(data);

	node = check_rx_tx_num(rmi4_data);
	if (node < 0) {
		snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", tostring(NA));
		data->cmd_state = CMD_STATUS_FAIL;
		goto out;
	}

	report_data = data->delta_data[node];

	snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%d", report_data);
	data->cmd_state = CMD_STATUS_OK;

out:
	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));
}

static void run_delta_read(void *dev_data)
{
	struct synaptics_rmi4_data *rmi4_data = (struct synaptics_rmi4_data *)dev_data;
	struct synaptics_rmi4_f54_handle *f54 = rmi4_data->f54;
	struct factory_data *data = f54->factory_data;
	short *report_data;
	short cur_value;
	unsigned char ii;
	unsigned char jj;
	unsigned char num_of_tx;
	unsigned char num_of_rx;
	int kk = 0;

	set_default_result(data);

	if (rmi4_data->touch_stopped || rmi4_data->sensor_sleep) {
		dev_err(rmi4_data->pdev->dev.parent, "%s: [ERROR] Touch is %s\n",
			__func__, rmi4_data->touch_stopped ? "stopped" : "Sleep state");
		snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "TSP is %s",
			rmi4_data->touch_stopped ? "off" : "sleep");
		data->cmd_state = CMD_STATUS_NOT_APPLICABLE;
		goto out;
	}

	if (!synaptics_rmi4_f54_get_report_type(rmi4_data, F54_16BIT_IMAGE)) {
		snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", "Error get report type");
		data->cmd_state = CMD_STATUS_FAIL;
		goto out;
	}

	report_data = data->delta_data;
	memcpy(report_data, f54->report_data, f54->report_size);

	num_of_tx = f54->tx_assigned;
	num_of_rx = f54->rx_assigned;

	for (ii = 0; ii < num_of_tx; ii++) {
		for (jj = 0; jj < num_of_rx; jj++) {
			cur_value = *report_data;
			report_data++;
			if (cur_value > TSP_DELTA_MAX || cur_value < TSP_DELTA_MIN)
				dev_info(rmi4_data->pdev->dev.parent, "tx = %02d, rx = %02d, data[%d] = %d\n",
					ii, jj, kk, cur_value);
			kk++;
		}
	}

	snprintf(data->cmd_buff, sizeof(data->cmd_buff), "OK");
	data->cmd_state = CMD_STATUS_OK;

out:
	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));
}

static void run_abscap_read(void *dev_data)
{
	struct synaptics_rmi4_data *rmi4_data = (struct synaptics_rmi4_data *)dev_data;
	struct synaptics_rmi4_f54_handle *f54 = rmi4_data->f54;
	struct factory_data *data = f54->factory_data;

	unsigned int *report_data;
	char temp[CMD_STR_LEN];
	char temp2[CMD_RESULT_STR_LEN];
	unsigned char ii;
	unsigned short num_of_tx;
	unsigned short num_of_rx;
	int retval;
	unsigned char cmd_state = CMD_STATUS_RUNNING;

	set_default_result(data);

	if (rmi4_data->touch_stopped || rmi4_data->sensor_sleep) {
		dev_err(rmi4_data->pdev->dev.parent, "%s: [ERROR] Touch is %s\n",
			__func__, rmi4_data->touch_stopped ? "stopped" : "Sleep state");
		snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "TSP is %s",
			rmi4_data->touch_stopped ? "off" : "sleep");
		cmd_state = CMD_STATUS_NOT_APPLICABLE;
		goto out;
	}

	if (!synaptics_rmi4_f54_get_report_type(rmi4_data, F54_ABS_CAP)) {
		snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", "Error get report type");
		cmd_state = CMD_STATUS_FAIL;
		goto sw_reset;
	}

	report_data = data->abscap_data;
	memcpy(report_data, f54->report_data, f54->report_size);
	memset(temp, 0, CMD_STR_LEN);
	memset(temp2, 0, CMD_RESULT_STR_LEN);

	num_of_tx = f54->tx_assigned;
	num_of_rx = f54->rx_assigned;

	data->abscap_rx_min = data->abscap_rx_max = report_data[0];
	data->abscap_tx_min = data->abscap_tx_max = report_data[num_of_rx];

	for (ii = 0; ii < num_of_rx + num_of_tx ; ii++) {
		dev_info(rmi4_data->pdev->dev.parent, "%s: %s [%d] = %d\n",
				__func__, ii >= num_of_rx ? "Tx" : "Rx",
				ii < num_of_rx ? ii : ii - num_of_rx,
				*report_data);

		if (ii >= num_of_rx) {
			data->abscap_tx_min = min(data->abscap_tx_min , *report_data);
			data->abscap_tx_max = max(data->abscap_tx_max , *report_data);
		} else {
			data->abscap_rx_min = min(data->abscap_rx_min , *report_data);
			data->abscap_rx_max = max(data->abscap_rx_max , *report_data);
		}

		if (ii == num_of_rx + num_of_tx -1)
			snprintf(temp, CMD_STR_LEN, "%d", *report_data);
		else
			snprintf(temp, CMD_STR_LEN, "%d,", *report_data);
		strncat(temp2, temp, RPT_DATA_STRNCAT_LENGTH);
		report_data++;
	}

	dev_info(rmi4_data->pdev->dev.parent, "%s: RX:[%d][%d], TX:[%d][%d]\n",
					__func__, data->abscap_rx_min, data->abscap_rx_max,
					data->abscap_tx_min, data->abscap_tx_max);


	snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", temp2);
	cmd_state = CMD_STATUS_OK;

sw_reset:
	retval = rmi4_data->reset_device(rmi4_data);
	if (retval < 0) {
		dev_err(rmi4_data->pdev->dev.parent, "%s: Failed to issue reset command, error = %d\n",
				__func__, retval);
	}
	mutex_lock(&f54->status_mutex);
	f54->status = STATUS_IDLE;
	mutex_unlock(&f54->status_mutex);

out:
	data->cmd_state = cmd_state;
	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));
}

static void get_abscap_read_test(void *dev_data)
{
	struct synaptics_rmi4_data *rmi4_data = (struct synaptics_rmi4_data *)dev_data;
	struct synaptics_rmi4_f54_handle *f54 = rmi4_data->f54;
	struct factory_data *data = f54->factory_data;

	unsigned char cmd_state = CMD_STATUS_RUNNING;

	set_default_result(data);

	if (rmi4_data->touch_stopped || rmi4_data->sensor_sleep) {
		dev_err(rmi4_data->pdev->dev.parent, "%s: [ERROR] Touch is %s\n",
			__func__, rmi4_data->touch_stopped ? "stopped" : "Sleep state");
		snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "TSP is %s",
			rmi4_data->touch_stopped ? "off" : "sleep");
		cmd_state = CMD_STATUS_NOT_APPLICABLE;
		goto out;
	}

	dev_info(rmi4_data->pdev->dev.parent, "%s: RX:[%d][%d], TX:[%d][%d]\n",
					__func__, data->abscap_rx_min, data->abscap_rx_max,
					data->abscap_tx_min, data->abscap_tx_max);

	snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%d,%d,%d,%d",
								data->abscap_rx_min, data->abscap_rx_max,
								data->abscap_tx_min, data->abscap_tx_max);
	cmd_state = CMD_STATUS_OK;

	data->abscap_rx_min = data->abscap_rx_max = data->abscap_tx_min = data->abscap_tx_max = 0;

out:
	data->cmd_state = cmd_state;
	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));
}

static void run_absdelta_read(void *dev_data)
{
	struct synaptics_rmi4_data *rmi4_data = (struct synaptics_rmi4_data *)dev_data;
	struct synaptics_rmi4_f54_handle *f54 = rmi4_data->f54;
	struct synaptics_rmi4_f51_handle *f51 = rmi4_data->f51;
	struct factory_data *data = f54->factory_data;

	int *report_data;
	char temp[CMD_STR_LEN];
	char temp2[CMD_RESULT_STR_LEN];
	unsigned char ii;
	unsigned short num_of_tx;
	unsigned short num_of_rx;
	unsigned char proximity_enables = FINGER_HOVER_EN;

	int retval;
	unsigned char cmd_state = CMD_STATUS_RUNNING;

	set_default_result(data);

	if (rmi4_data->touch_stopped || rmi4_data->sensor_sleep) {
		dev_err(rmi4_data->pdev->dev.parent, "%s: [ERROR] Touch is %s\n",
			__func__, rmi4_data->touch_stopped ? "stopped" : "Sleep state");
		snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "TSP is %s",
			rmi4_data->touch_stopped ? "off" : "sleep");
		cmd_state = CMD_STATUS_NOT_APPLICABLE;
		goto out;
	}

	/* Enable hover before read abs delta */
	retval = rmi4_data->i2c_write(rmi4_data, f51->proximity_enables_addr,
			&proximity_enables,	sizeof(proximity_enables));
	if (retval < 0) {
		dev_err(rmi4_data->pdev->dev.parent, "%s: Failed to write proximity_enables\n",
			__func__);
		snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "NG");
		cmd_state = CMD_STATUS_FAIL;
		goto out;
	}
	/* at least 5 frame time are needed after enable hover
	 * to get creadible abs delta data( 16.6 * 5 = 88 msec )
	 */
	msleep(150);

	if (!synaptics_rmi4_f54_get_report_type(rmi4_data, F54_ABS_DELTA)) {
		snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", "Error get report type");
		cmd_state = CMD_STATUS_FAIL;
		goto sw_reset;
	}

	report_data = data->absdelta_data;
	memcpy(report_data, f54->report_data, f54->report_size);
	memset(temp, 0, CMD_STR_LEN);
	memset(temp2, 0, CMD_RESULT_STR_LEN);

	num_of_tx = f54->tx_assigned;
	num_of_rx = f54->rx_assigned;

	for (ii = 0; ii < num_of_rx + num_of_tx; ii++) {
		dev_info(rmi4_data->pdev->dev.parent, "%s: %s [%d] = %d\n",
				__func__, ii >= num_of_rx ? "Tx" : "Rx",
				ii < num_of_rx ? ii : ii - num_of_rx,
				*report_data);
		snprintf(temp, CMD_STR_LEN, "%d,", *report_data);
		strncat(temp2, temp, RPT_DATA_STRNCAT_LENGTH);
		report_data++;
	}
	snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", temp2);
	cmd_state = CMD_STATUS_OK;

sw_reset:
	retval = rmi4_data->reset_device(rmi4_data);

	if (retval < 0) {
		dev_err(rmi4_data->pdev->dev.parent, "%s: Failed to issue reset command, error = %d\n",
				__func__, retval);
	}
	mutex_lock(&f54->status_mutex);
	f54->status = STATUS_IDLE;
	mutex_unlock(&f54->status_mutex);

out:
	data->cmd_state = cmd_state;
	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));
}

/* trx_short_test register mapping
 * 0 : not used ( using 5.2 inch)
 * 1 ~ 28 : Rx
 * 29 ~ 31 : Side Button 0, 1, 2
 * 32 ~ 33 : Guard
 * 34 : Charge Substraction
 * 35 ~ 50 : Tx
 * 51 ~ 53 : Side Button 3, 4, 5
 */

static void run_trx_short_test(void *dev_data)
{
	struct synaptics_rmi4_data *rmi4_data = (struct synaptics_rmi4_data *)dev_data;
	struct synaptics_rmi4_f54_handle *f54 = rmi4_data->f54;
	struct factory_data *data = f54->factory_data;

	char *report_data;
	unsigned char ii, jj;
	int retval = 0;
	unsigned char cmd_state = CMD_STATUS_RUNNING;

	char temp[CMD_STR_LEN];
	char temp2[CMD_RESULT_STR_LEN];

	set_default_result(data);

	if (rmi4_data->touch_stopped || rmi4_data->sensor_sleep) {
		dev_err(rmi4_data->pdev->dev.parent, "%s: [ERROR] Touch is %s\n",
			__func__, rmi4_data->touch_stopped ? "stopped" : "Sleep state");
		snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "TSP is %s",
			rmi4_data->touch_stopped ? "off" : "sleep");
		cmd_state = CMD_STATUS_NOT_APPLICABLE;
		goto out;
	}

	disable_irq(rmi4_data->i2c_client->irq);
	if (!synaptics_rmi4_f54_get_report_type(rmi4_data, F54_TREX_SHORTS)) {
		snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", "Error get report type");
		cmd_state = CMD_STATUS_FAIL;
		goto sw_reset;
	}

	report_data = data->trx_short;
	memcpy(report_data, f54->report_data, f54->report_size);
	memset(temp, 0, CMD_STR_LEN);
	memset(temp2, 0, CMD_RESULT_STR_LEN);

	for (ii = 0; ii < f54->report_size; ii++) {
		dev_info(rmi4_data->pdev->dev.parent, "%s: [%d]: [%x][%x][%x][%x][%x][%x][%x][%x]\n",
			__func__, ii, *report_data & 0x1, (*report_data & 0x2) >> 1,
			(*report_data & 0x4) >> 2, (*report_data & 0x8) >> 3,
			(*report_data & 0x10) >> 4, (*report_data & 0x20) >> 5,
			(*report_data & 0x40) >> 6, (*report_data & 0x80) >> 7);

		for (jj = 0; jj < 8; jj++) {
			snprintf(temp, CMD_STR_LEN, "%d,", (*report_data >> jj) & 0x01);
			strncat(temp2, temp, RPT_DATA_STRNCAT_LENGTH);
		}
		report_data++;
	}

	snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", temp2);
	cmd_state = CMD_STATUS_OK;

sw_reset:
	enable_irq(rmi4_data->i2c_client->irq);
	retval = rmi4_data->reset_device(rmi4_data);
	if (retval < 0) {
		dev_err(rmi4_data->pdev->dev.parent, "%s: Failed to issue reset command, error = %d\n",
				__func__, retval);
	}
	mutex_lock(&f54->status_mutex);
	f54->status = STATUS_IDLE;
	mutex_unlock(&f54->status_mutex);

out:
	data->cmd_state = cmd_state;
	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));
}

static void dead_zone_enable(void *dev_data)
{
	struct synaptics_rmi4_data *rmi4_data = (struct synaptics_rmi4_data *)dev_data;
	struct factory_data *data = rmi4_data->f54->factory_data;

	int retval = 0;
	unsigned char dead_zone_en = 0;

	set_default_result(data);

	if (data->cmd_param[0] < 0 || data->cmd_param[0] > 2) {
		snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NG");
		data->cmd_state = CMD_STATUS_FAIL;
		goto out;
	}

	retval = rmi4_data->i2c_read(rmi4_data,
				rmi4_data->f51->general_control_addr, &dead_zone_en, sizeof(dead_zone_en));

	if (retval <= 0) {
		dev_err(rmi4_data->pdev->dev.parent, "%s: fail to read no_sleep[ret:%d]\n",
				__func__, retval);
		snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NG");
		data->cmd_state = CMD_STATUS_FAIL;
		goto out;
	}

	/* 0: Disable dead Zone for factory app   , 1: Enable dead Zone (default) */
	if (data->cmd_param[0])
		dead_zone_en |= DEAD_ZONE_EN;
	else
		dead_zone_en &= ~(DEAD_ZONE_EN);

	retval = rmi4_data->i2c_write(rmi4_data,
			rmi4_data->f51->general_control_addr, &dead_zone_en, sizeof(dead_zone_en));
	if (retval <= 0) {
		dev_err(rmi4_data->pdev->dev.parent, "%s: fail to read dead_zone_en[ret:%d]\n",
				__func__, retval);
		snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NG");
		data->cmd_state = CMD_STATUS_FAIL;
		goto out;
	}
	snprintf(data->cmd_buff, sizeof(data->cmd_buff), "OK");
	data->cmd_state = CMD_STATUS_OK;

out:
	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));

	mutex_lock(&data->cmd_lock);
	data->cmd_is_running = false;
	mutex_unlock(&data->cmd_lock);

	data->cmd_state = CMD_STATUS_WAITING;
}

#ifdef PROXIMITY_MODE
static void hover_enable(void *dev_data)
{
	struct synaptics_rmi4_data *rmi4_data = (struct synaptics_rmi4_data *)dev_data;
	struct factory_data *data = rmi4_data->f54->factory_data;

	int retval = 0, enables = 0;

	set_default_result(data);

	if (data->cmd_param[0] < 0 || data->cmd_param[0] > 1) {
		snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NG");
		data->cmd_state = CMD_STATUS_FAIL;
		goto out;
	}

	enables = data->cmd_param[0];
	retval = synaptics_rmi4_proximity_enables(rmi4_data, enables);
	if (retval < 0) {
		dev_err(rmi4_data->pdev->dev.parent, "%s failed, retval = %d\n",
			__func__, retval);
		snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NG");
		data->cmd_state = CMD_STATUS_FAIL;
		goto out;
	}

	snprintf(data->cmd_buff, sizeof(data->cmd_buff), "OK");
	data->cmd_state = CMD_STATUS_OK;

out:
	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));

	mutex_lock(&data->cmd_lock);
	data->cmd_is_running = false;
	mutex_unlock(&data->cmd_lock);

	data->cmd_state = CMD_STATUS_WAITING;
}

static void hover_no_sleep_enable(void *dev_data)
{
	struct synaptics_rmi4_data *rmi4_data = (struct synaptics_rmi4_data *)dev_data;
	struct synaptics_rmi4_f54_handle *f54 = rmi4_data->f54;
	struct factory_data *data = f54->factory_data;

	unsigned char no_sleep = 0;
	int retval = 0;

	set_default_result(data);

	if (data->cmd_param[0] < 0 || data->cmd_param[0] > 1) {
		snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NG");
		data->cmd_state = CMD_STATUS_FAIL;
		goto out;
	}
	retval = rmi4_data->i2c_read(rmi4_data,
				rmi4_data->f01_ctrl_base_addr, &no_sleep, sizeof(no_sleep));
	if (retval <= 0) {
		dev_err(rmi4_data->pdev->dev.parent, "%s: fail to read no_sleep[ret:%d]\n",
				__func__, retval);
		snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NG");
		data->cmd_state = CMD_STATUS_FAIL;
		goto out;
	}

	if (data->cmd_param[0])
		no_sleep |= NO_SLEEP_ON;
	else
		no_sleep &= ~(NO_SLEEP_ON);

	retval = rmi4_data->i2c_write(rmi4_data,
			rmi4_data->f01_ctrl_base_addr, &no_sleep, sizeof(no_sleep));
	if (retval <= 0) {
		dev_err(rmi4_data->pdev->dev.parent, "%s: fail to read no_sleep[ret:%d]\n",
				__func__, retval);
		snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NG");
		data->cmd_state = CMD_STATUS_FAIL;
		goto out;
	}

	snprintf(data->cmd_buff, sizeof(data->cmd_buff), "OK");
	data->cmd_state = CMD_STATUS_OK;

out:
	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));
}

static void hover_set_edge_rx(void *dev_data)
{
	struct synaptics_rmi4_data *rmi4_data = (struct synaptics_rmi4_data *)dev_data;
	struct factory_data *data = rmi4_data->f54->factory_data;

	unsigned char edge_exculsion_rx = 0x10;
	int retval = 0;

	set_default_result(data);

	if (data->cmd_param[0] < 0 || data->cmd_param[0] > 1) {
		snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NG");
		data->cmd_state = CMD_STATUS_FAIL;
		goto out;
	}

	if (data->cmd_param[0])
		edge_exculsion_rx = 0x0;
	else
		edge_exculsion_rx = 0x10;

	retval = synaptics_rmi4_access_register(rmi4_data, SYNAPTICS_ACCESS_WRITE,
				rmi4_data->f51->grip_edge_exclusion_rx_addr, sizeof(edge_exculsion_rx), &edge_exculsion_rx);
	if (retval < 0) {
		dev_err(rmi4_data->pdev->dev.parent, "%s: Failed to write grip edge exclustion rx with [0x%02X].\n",
				__func__, edge_exculsion_rx);
		snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NG");
		data->cmd_state = CMD_STATUS_FAIL;
		goto out;
	}

	snprintf(data->cmd_buff, sizeof(data->cmd_buff), "OK");
	data->cmd_state = CMD_STATUS_OK;

out:
	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));
}
#endif

static void set_jitter_level(void *dev_data)
{
	struct synaptics_rmi4_data *rmi4_data = (struct synaptics_rmi4_data *)dev_data;
	struct factory_data *data = rmi4_data->f54->factory_data;

	int retval = 0, level = 0;

	set_default_result(data);

	if (data->cmd_param[0] < 0 || data->cmd_param[0] > 255) {
		dev_err(rmi4_data->pdev->dev.parent, "%s failed, the range of jitter level is 0~255\n",
				__func__);
		snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NG");
		data->cmd_state = CMD_STATUS_FAIL;
		goto out;
	}

	level = data->cmd_param[0];

	retval = synaptics_rmi4_f12_ctrl11_set(rmi4_data, level);
	if (retval < 0) {
		dev_err(rmi4_data->pdev->dev.parent, "%s failed, retval = %d\n",
			__func__, retval);
		snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NG");
		data->cmd_state = CMD_STATUS_FAIL;
		goto out;
	}

	snprintf(data->cmd_buff, sizeof(data->cmd_buff), "OK");
	data->cmd_state = CMD_STATUS_OK;

out:
	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));

	mutex_lock(&data->cmd_lock);
	data->cmd_is_running = false;
	mutex_unlock(&data->cmd_lock);

	data->cmd_state = CMD_STATUS_WAITING;
}

#ifdef GLOVE_MODE
static void glove_mode(void *dev_data)
{
	struct synaptics_rmi4_data *rmi4_data = (struct synaptics_rmi4_data *)dev_data;
	struct factory_data *data = rmi4_data->f54->factory_data;

	int retval = 0;

	set_default_result(data);

	if (rmi4_data->f12.feature_enable & CLOSED_COVER_EN) {
		snprintf(data->cmd_buff, sizeof(data->cmd_buff), "OK");
		data->cmd_state = CMD_STATUS_OK;
		dev_info(rmi4_data->pdev->dev.parent, "%s Skip glove mode set (cover bit enabled)\n",
				__func__);
		goto out;
	}

	if (data->cmd_param[0] < 0 || data->cmd_param[0] > 1) {
		snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NG");
		data->cmd_state = CMD_STATUS_FAIL;
		goto out;
	}

	if (data->cmd_param[0]){
		rmi4_data->f12.feature_enable |= GLOVE_DETECTION_EN;
		rmi4_data->f12.obj_report_enable |= OBJ_TYPE_GLOVE;
	} else {
		rmi4_data->f12.feature_enable &= ~(GLOVE_DETECTION_EN);
		rmi4_data->f12.obj_report_enable &= ~(OBJ_TYPE_GLOVE);
	}

	retval = synaptics_rmi4_glove_mode_enables(rmi4_data);
	if (retval < 0) {
		dev_err(rmi4_data->pdev->dev.parent, "%s failed, retval = %d\n",
			__func__, retval);
		snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NG");
		data->cmd_state = CMD_STATUS_FAIL;
		goto out;
	}

	snprintf(data->cmd_buff, sizeof(data->cmd_buff), "OK");
	data->cmd_state = CMD_STATUS_OK;

out:
	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));

	mutex_lock(&data->cmd_lock);
	data->cmd_is_running = false;
	mutex_unlock(&data->cmd_lock);

	data->cmd_state = CMD_STATUS_WAITING;
}

static void fast_glove_mode(void *dev_data)
{
	struct synaptics_rmi4_data *rmi4_data = (struct synaptics_rmi4_data *)dev_data;
	struct factory_data *data = rmi4_data->f54->factory_data;

	int retval = 0;

	set_default_result(data);

	if (data->cmd_param[0] < 0 || data->cmd_param[0] > 1) {
		snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NG");
		data->cmd_state = CMD_STATUS_FAIL;
		goto out;
	}

	if (data->cmd_param[0]) {
		rmi4_data->f12.feature_enable |= FAST_GLOVE_DECTION_EN | GLOVE_DETECTION_EN;
		rmi4_data->f12.obj_report_enable |= OBJ_TYPE_GLOVE;
		rmi4_data->fast_glove_state = true;
	} else {
		rmi4_data->f12.feature_enable &= ~(FAST_GLOVE_DECTION_EN);
		rmi4_data->f12.obj_report_enable |= OBJ_TYPE_GLOVE;
		rmi4_data->fast_glove_state = false;
	}

	retval = synaptics_rmi4_glove_mode_enables(rmi4_data);
	if (retval < 0) {
		dev_err(rmi4_data->pdev->dev.parent, "%s failed, retval = %d\n",
			__func__, retval);
		snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NG");
		data->cmd_state = CMD_STATUS_FAIL;
		goto out;
	}

	snprintf(data->cmd_buff, sizeof(data->cmd_buff), "OK");
	data->cmd_state = CMD_STATUS_OK;

out:
	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));

	mutex_lock(&data->cmd_lock);
	data->cmd_is_running = false;
	mutex_unlock(&data->cmd_lock);

	data->cmd_state = CMD_STATUS_WAITING;
}

static void clear_cover_mode(void *dev_data)
{
	struct synaptics_rmi4_data *rmi4_data = (struct synaptics_rmi4_data *)dev_data;
	struct factory_data *data = rmi4_data->f54->factory_data;

	int retval = 0;

	set_default_result(data);

	if (data->cmd_param[0] < 0 || data->cmd_param[0] > 3) {
		snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NG");
		data->cmd_state = CMD_STATUS_FAIL;
		goto out;
	}

	rmi4_data->f12.feature_enable = data->cmd_param[0];

	if (data->cmd_param[0] && rmi4_data->fast_glove_state)
		rmi4_data->f12.feature_enable |= FAST_GLOVE_DECTION_EN;

	retval = synaptics_rmi4_glove_mode_enables(rmi4_data);
	if (retval < 0) {
		dev_err(rmi4_data->pdev->dev.parent, "%s failed, retval = %d\n",
			__func__, retval);
		snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NG");
		data->cmd_state = CMD_STATUS_FAIL;
		goto out;
	}
	snprintf(data->cmd_buff, sizeof(data->cmd_buff), "OK");
	data->cmd_state = CMD_STATUS_OK;

	/* Sync user setting value when wakeup with flip cover opened */
	if (rmi4_data->f12.feature_enable == CLOSED_COVER_EN
		|| rmi4_data->f12.feature_enable == (CLOSED_COVER_EN | FAST_GLOVE_DECTION_EN)) {

		rmi4_data->f12.feature_enable &= ~(CLOSED_COVER_EN);
		if (rmi4_data->fast_glove_state)
			rmi4_data->f12.feature_enable |= GLOVE_DETECTION_EN;
	}

out:
	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));

	mutex_lock(&data->cmd_lock);
	data->cmd_is_running = false;
	mutex_unlock(&data->cmd_lock);

	data->cmd_state = CMD_STATUS_WAITING;
}
#endif

#ifdef TSP_BOOSTER
static void boost_level(void *dev_data)
{
	struct synaptics_rmi4_data *rmi4_data = (struct synaptics_rmi4_data *)dev_data;
	struct factory_data *data = rmi4_data->f54->factory_data;


	set_default_result(data);

	if (data->cmd_param[0] < 0 || data->cmd_param[0] >= BOOSTER_LEVEL_MAX) {
		snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NG");
		data->cmd_state = CMD_STATUS_FAIL;
		goto out;
	}

	change_booster_level_for_tsp(data->cmd_param[0]);
	tsp_debug_dbg(false, &rmi4_data->i2c_client->dev, "%s %d\n",
					__func__, data->cmd_param[0]);

	snprintf(data->cmd_buff, sizeof(data->cmd_buff), "OK");
	data->cmd_state = CMD_STATUS_OK;

out:
	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));

	mutex_lock(&data->cmd_lock);
	data->cmd_is_running = false;
	mutex_unlock(&data->cmd_lock);

	data->cmd_state = CMD_STATUS_WAITING;
}
#endif

#ifdef SIDE_TOUCH
static void sidekey_enable(void *dev_data)
{
	struct synaptics_rmi4_data *rmi4_data = (struct synaptics_rmi4_data *)dev_data;
	struct synaptics_rmi4_f54_handle *f54 = rmi4_data->f54;
	struct factory_data *data = f54->factory_data;

	int retval = 0;
	unsigned char general_control_2;

	set_default_result(data);

	if (data->cmd_param[0] < 0 || data->cmd_param[0] > 1) {
		snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NG");
		data->cmd_state = CMD_STATUS_FAIL;
		goto out;
	}

	retval = rmi4_data->i2c_read(rmi4_data, rmi4_data->f51->general_control_2_addr,
			&general_control_2, sizeof(general_control_2));
	if (retval < 0) {
		dev_err(rmi4_data->pdev->dev.parent, "%s: Failed to read general control 2.\n",
				__func__);
		snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NA");
		data->cmd_state = CMD_STATUS_FAIL;
		goto out;
	}

	if (data->cmd_param[0])
		general_control_2 |= SIDE_BUTTONS_EN;
	else
		general_control_2 &= ~(SIDE_BUTTONS_EN);

	retval = rmi4_data->i2c_write(rmi4_data, rmi4_data->f51->general_control_2_addr,
			&general_control_2,	sizeof(general_control_2));
	if (retval < 0) {
		dev_err(rmi4_data->pdev->dev.parent, "%s: Failed to write general control 2 register with [0x%02X].\n",
				__func__, general_control_2);
		snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NA");
		data->cmd_state = CMD_STATUS_FAIL;
		goto out;
	}

	rmi4_data->f51->general_control_2 = general_control_2;
	dev_info(rmi4_data->pdev->dev.parent, "%s: General Control2 [0x%02X]\n",
			__func__, rmi4_data->f51->general_control_2);

	if (!data->cmd_param[0])
		synpatics_rmi4_release_all_event(rmi4_data, RELEASE_TYPE_SIDEKEY);

	snprintf(data->cmd_buff, sizeof(data->cmd_buff), "OK");
	data->cmd_state = CMD_STATUS_OK;

out:
	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));

	mutex_lock(&data->cmd_lock);
	data->cmd_is_running = false;
	mutex_unlock(&data->cmd_lock);

	data->cmd_state = CMD_STATUS_WAITING;
}

static void set_sidekey_only_enable(void *dev_data)
{
	struct synaptics_rmi4_data *rmi4_data = (struct synaptics_rmi4_data *)dev_data;
	struct synaptics_rmi4_f54_handle *f54 = rmi4_data->f54;
	struct factory_data *data = f54->factory_data;

	int retval = 0;
	unsigned char device_control = 0;
	bool sidekey_only_enable;

	set_default_result(data);

	mutex_lock(&rmi4_data->rmi4_device_mutex);

	if (data->cmd_param[0] < 0 || data->cmd_param[0] > 1) {
		snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NG");
		data->cmd_state = CMD_STATUS_FAIL;
		goto out;
	}

	sidekey_only_enable = data->cmd_param[0] ? true : false;

	/* Control device_control value */
	retval = rmi4_data->i2c_read(rmi4_data,
				rmi4_data->f01_ctrl_base_addr, &device_control, sizeof(device_control));
	if (retval < 0) {
		dev_err(rmi4_data->pdev->dev.parent, "%s: Failed to read Device Control register.\n",
				__func__);
		snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NA");
		data->cmd_state = CMD_STATUS_FAIL;
		goto out;
	}

	if (sidekey_only_enable)
		device_control &= ~(SENSOR_SLEEP);
	else
		device_control |= SENSOR_SLEEP;

	retval = rmi4_data->i2c_write(rmi4_data,
			rmi4_data->f01_ctrl_base_addr, &device_control, sizeof(device_control));

	if (retval < 0) {
		dev_err(rmi4_data->pdev->dev.parent, "%s: Failed to write Device Control register [0x%02X].\n",
				__func__, device_control);
		snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NA");
		data->cmd_state = CMD_STATUS_FAIL;
		goto out;
	}

	if (sidekey_only_enable)
		rmi4_data->sensor_sleep = false;
	else
		rmi4_data->sensor_sleep = true;

	dev_info(rmi4_data->pdev->dev.parent, "%s : [F01_CTRL] 0x%02X, [F51_CTRL] 0x%02X/0x%02X/0x%02X]\n",
		__func__, device_control, rmi4_data->f51->proximity_enables, rmi4_data->f51->general_control, rmi4_data->f51->general_control_2);

	snprintf(data->cmd_buff, sizeof(data->cmd_buff), "OK");
	data->cmd_state = CMD_STATUS_OK;

out:
	mutex_unlock(&rmi4_data->rmi4_device_mutex);

	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));
}

static void get_sidekey_threshold(void *dev_data)
{
	struct synaptics_rmi4_data *rmi4_data = (struct synaptics_rmi4_data *)dev_data;
	struct factory_data *data = rmi4_data->f54->factory_data;

	unsigned char sidekey_threshold[NUM_OF_ACTIVE_SIDE_BUTTONS];
	char temp[CMD_STR_LEN];
	char temp2[CMD_RESULT_STR_LEN];
	int retval = 0, ii = 0;

	set_default_result(data);

	if (rmi4_data->touch_stopped) {
		dev_err(rmi4_data->pdev->dev.parent, "%s: [ERROR] Touch is stopped\n",
			__func__);
		snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", "TSP turned off");
		data->cmd_state = CMD_STATUS_NOT_APPLICABLE;
		goto out;
	}

	memset(temp, 0, CMD_STR_LEN);
	memset(temp2, 0, CMD_RESULT_STR_LEN);

	retval = synaptics_rmi4_access_register(rmi4_data, SYNAPTICS_ACCESS_READ,
				rmi4_data->f51->sidebutton_tapthreshold_addr,
				NUM_OF_ACTIVE_SIDE_BUTTONS, sidekey_threshold);

	if (retval < 0) {
		dev_err(rmi4_data->pdev->dev.parent, "%s: Failed to write general control 2 register with [0x%02X].\n",
				__func__, rmi4_data->f51->general_control_2);
		snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NA");
		data->cmd_state = CMD_STATUS_FAIL;
		goto out;
	}

	while (ii < NUM_OF_ACTIVE_SIDE_BUTTONS) {
		snprintf(temp, CMD_STR_LEN, "%u ", sidekey_threshold[ii]);
		strcat(temp2, temp);
		ii++;
	}

	snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", temp2);
	data->cmd_state = CMD_STATUS_OK;

out:
	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));
}

static void run_sidekey_delta_read(void *dev_data)
{
	struct synaptics_rmi4_data *rmi4_data = (struct synaptics_rmi4_data *)dev_data;
	struct synaptics_rmi4_f54_handle *f54 = rmi4_data->f54;
	struct factory_data *data = f54->factory_data;

	int *report_data;
	char temp[CMD_STR_LEN];
	char temp2[CMD_RESULT_STR_LEN];
	unsigned char ii;
	unsigned short num_of_tx;
	unsigned short num_of_rx;
	unsigned char sidekey_production_test;
	int retval;

	set_default_result(data);

	if (rmi4_data->touch_stopped || rmi4_data->sensor_sleep) {
		dev_err(rmi4_data->pdev->dev.parent, "%s: [ERROR] Touch is %s\n",
			__func__, rmi4_data->touch_stopped ? "stopped" : "Sleep state");
		snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "TSP is %s",
			rmi4_data->touch_stopped ? "off" : "sleep");
		data->cmd_state = CMD_STATUS_NOT_APPLICABLE;
		goto out;
	}

	/* Set sidekey production test */
	retval = rmi4_data->i2c_read(rmi4_data, rmi4_data->f51->general_control_2_addr,
			&sidekey_production_test, sizeof(sidekey_production_test));
	if (retval < 0) {
		dev_err(rmi4_data->pdev->dev.parent, "%s: Failed to read general control 2.\n",
				__func__);
		snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NA");
		data->cmd_state = CMD_STATUS_FAIL;
		goto out;
	}

	sidekey_production_test |= SIDE_BUTTONS_PRODUCTION_TEST;

	retval = rmi4_data->i2c_write(rmi4_data, rmi4_data->f51->general_control_2_addr,
			&sidekey_production_test,
			sizeof(sidekey_production_test));
	if (retval < 0) {
		dev_err(rmi4_data->pdev->dev.parent, "%s: Failed to write general control 2 register with [0x%02X].\n",
				__func__, sidekey_production_test);
		snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NA");
		data->cmd_state = CMD_STATUS_FAIL;
		goto out;
	}

	msleep(100);

	if (!synaptics_rmi4_f54_get_report_type(rmi4_data, F54_ABS_DELTA)) {
		snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", "Error get report type");
		data->cmd_state = CMD_STATUS_FAIL;
		goto out;
	}

	report_data = f54->factory_data->absdelta_data;
	memcpy(report_data, f54->report_data, f54->report_size);
	memset(temp, 0, CMD_STR_LEN);
	memset(temp2, 0, CMD_RESULT_STR_LEN);

	num_of_tx = f54->tx_assigned;
	num_of_rx = f54->rx_assigned;

	for (ii = 0; ii < (num_of_rx + num_of_tx + NUM_OF_ACTIVE_SIDE_BUTTONS); ii++) {
		if (rmi4_data->product_id < SYNAPTICS_PRODUCT_ID_S5100)
			*report_data &= 0x0FFFF;

		if (ii < (num_of_rx + num_of_tx)) {
			report_data++;
			continue;
		}

		dev_info(rmi4_data->pdev->dev.parent,
				"%s: %s [%d] = %d\n", __func__,	"SIDE",
				 ii - (num_of_rx + num_of_tx), *report_data);
		snprintf(temp, CMD_STR_LEN, "%d,", *report_data);
		strncat(temp2, temp, RPT_DATA_STRNCAT_LENGTH);
		report_data++;
	}

	snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", temp2);
	data->cmd_state = CMD_STATUS_OK;

out:
	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));
}

static void run_sidekey_abscap_read(void *dev_data)
{
	struct synaptics_rmi4_data *rmi4_data = (struct synaptics_rmi4_data *)dev_data;
	struct synaptics_rmi4_f54_handle *f54 = rmi4_data->f54;
	struct factory_data *data = f54->factory_data;

	unsigned int *report_data;
	char temp[CMD_STR_LEN];
	char temp2[CMD_RESULT_STR_LEN];
	unsigned char ii;
	unsigned short num_of_tx;
	unsigned short num_of_rx;
	unsigned char cmd_state = CMD_STATUS_RUNNING;

	unsigned char sidekey_production_test;
	int retval;

	set_default_result(data);

	if (rmi4_data->touch_stopped || rmi4_data->sensor_sleep) {
		dev_err(rmi4_data->pdev->dev.parent, "%s: [ERROR] Touch is %s\n",
			__func__, rmi4_data->touch_stopped ? "stopped" : "Sleep state");
		snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "TSP is %s",
			rmi4_data->touch_stopped ? "off" : "sleep");
		cmd_state = CMD_STATUS_NOT_APPLICABLE;
		goto out;
	}

	/* Set sidekey production test */
	retval = rmi4_data->i2c_read(rmi4_data, rmi4_data->f51->general_control_2_addr,
			&sidekey_production_test, sizeof(sidekey_production_test));
	if (retval < 0) {
		dev_err(rmi4_data->pdev->dev.parent, "%s: Failed to read general control 2.\n",
				__func__);
		snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NA");
		cmd_state = CMD_STATUS_FAIL;
		goto out;
	}

	sidekey_production_test |= SIDE_BUTTONS_PRODUCTION_TEST;

	retval = rmi4_data->i2c_write(rmi4_data, rmi4_data->f51->general_control_2_addr,
			&sidekey_production_test,
			sizeof(sidekey_production_test));
	if (retval < 0) {
		dev_err(rmi4_data->pdev->dev.parent, "%s: Failed to write general control 2 register with [0x%02X].\n",
				__func__, sidekey_production_test);
		snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NA");
		cmd_state = CMD_STATUS_FAIL;
		goto out;
	}

	msleep(100);

	if (!synaptics_rmi4_f54_get_report_type(rmi4_data, F54_ABS_CAP)) {
		snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", "Error get report type");
		cmd_state = CMD_STATUS_FAIL;
		goto sw_reset;
	}

	report_data = f54->factory_data->abscap_data;
	memcpy(report_data, f54->report_data, f54->report_size);
	memset(temp, 0, CMD_STR_LEN);
	memset(temp2, 0, CMD_RESULT_STR_LEN);

	num_of_tx = f54->tx_assigned;
	num_of_rx = f54->rx_assigned;

	for (ii = 0; ii < num_of_rx + num_of_tx + NUM_OF_ACTIVE_SIDE_BUTTONS; ii++) {
		if (rmi4_data->product_id < SYNAPTICS_PRODUCT_ID_S5100)
			*report_data &= 0x0FFFF;

		if (ii < (num_of_rx + num_of_tx)) {
			report_data++;
			continue;
		}

		dev_info(rmi4_data->pdev->dev.parent,
				"%s: %s [%d] = %d\n", __func__,	"SIDE",
				 ii - (num_of_rx + num_of_tx), *report_data);
		snprintf(temp, CMD_STR_LEN, "%d,", *report_data);
		strncat(temp2, temp, RPT_DATA_STRNCAT_LENGTH);
		report_data++;
	}

	snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", temp2);
	cmd_state = CMD_STATUS_OK;

sw_reset:
	retval = rmi4_data->reset_device(rmi4_data);

	if (retval < 0) {
		dev_err(rmi4_data->pdev->dev.parent, "%s: Failed to issue reset command, error = %d\n",
				__func__, retval);
	}
	mutex_lock(&f54->status_mutex);
	f54->status = STATUS_IDLE;
	mutex_unlock(&f54->status_mutex);

out:
	data->cmd_state = cmd_state;
	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));
}

static void set_sidekey_only_enable(void *dev_data)
{
	struct synaptics_rmi4_data *rmi4_data = (struct synaptics_rmi4_data *)dev_data;
	struct factory_data *data = rmi4_data->f54->factory_data;

	set_default_result(data);

	if (rmi4_data->touch_stopped) {
		dev_err(rmi4_data->pdev->dev.parent, "%s: [ERROR] Touch is %s\n",
			__func__, rmi4_data->touch_stopped ? "stopped" : "Sleep state");
		snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "TSP is %s",
			rmi4_data->touch_stopped ? "off" : "sleep");
		data->cmd_state = CMD_STATUS_NOT_APPLICABLE;
		goto out;
	}

	rmi4_data->use_deepsleep = data->cmd_param[0] ? true : false;

	snprintf(data->cmd_buff, sizeof(data->cmd_buff), "OK");
	data->cmd_state = CMD_STATUS_OK;

out:
	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));
}

static void lozemode_enable(void *dev_data)
{
	struct synaptics_rmi4_data *rmi4_data = (struct synaptics_rmi4_data *)dev_data;
	struct synaptics_rmi4_f54_handle *f54 = rmi4_data->f54;
	struct factory_data *data = f54->factory_data;

	unsigned char general_control_2 = 0;
	int retval;

	set_default_result(data);

	retval = rmi4_data->i2c_read(rmi4_data, rmi4_data->f51->general_control_2_addr,
			&general_control_2, sizeof(general_control_2));
	if (retval < 0) {
		dev_err(rmi4_data->pdev->dev.parent, "%s: Failed to read general control 2.\n",
				__func__);
		snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NA");
		data->cmd_state = CMD_STATUS_FAIL;
		goto out;
	}

	if (data->cmd_param[0])
		general_control_2 |= ENTER_SLEEP_MODE;
	else
		general_control_2 &= ~ENTER_SLEEP_MODE;

	retval = rmi4_data->i2c_write(rmi4_data, rmi4_data->f51->general_control_2_addr,
			&general_control_2,	sizeof(general_control_2));
	if (retval < 0) {
		dev_err(rmi4_data->pdev->dev.parent, "%s: Failed to write general control 2 register with [0x%02X].\n",
				__func__, general_control_2);
		snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NA");
		data->cmd_state = CMD_STATUS_FAIL;
		goto out;
	}

	rmi4_data->f51->general_control_2 = general_control_2;
	dev_info(rmi4_data->pdev->dev.parent, "%s: General Control2 [0x%02X]\n",
			__func__, rmi4_data->f51->general_control_2);

	snprintf(data->cmd_buff, sizeof(data->cmd_buff), "OK");
	data->cmd_state = CMD_STATUS_OK;

out:
	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));
}
#endif

static void set_tsp_test_result(void *dev_data)
{
	struct synaptics_rmi4_data *rmi4_data = (struct synaptics_rmi4_data *)dev_data;
	struct factory_data *data = rmi4_data->f54->factory_data;

	int retval = 0;
	unsigned char device_status = 0;

	set_default_result(data);

	if (data->cmd_param[0] < TSP_FACTEST_RESULT_NONE
		 || data->cmd_param[0] > TSP_FACTEST_RESULT_PASS) {
		snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NG");
		data->cmd_state = CMD_STATUS_FAIL;
		return;
	}

	if (rmi4_data->touch_stopped || rmi4_data->sensor_sleep) {
		dev_err(rmi4_data->pdev->dev.parent, "%s: [ERROR] Touch is %s\n",
			__func__, rmi4_data->touch_stopped ? "stopped" : "Sleep state");
		snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "TSP is %s",
			rmi4_data->touch_stopped ? "off" : "sleep");
		data->cmd_state = CMD_STATUS_NOT_APPLICABLE;
		goto out;
	}

	retval = rmi4_data->i2c_read(rmi4_data,
			rmi4_data->f01_data_base_addr,
			&device_status,
			sizeof(device_status));
	if (device_status != 0) {
		snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", tostring(NR));
		data->cmd_state = CMD_STATUS_FAIL;
		dev_err(rmi4_data->pdev->dev.parent, "%s: IC not ready[%d]\n",
				__func__, device_status);
		goto out;
	}

	dev_info(rmi4_data->pdev->dev.parent, "%s: check status register[%d]\n",
			__func__, device_status);

	retval = synaptics_rmi4_set_tsp_test_result_in_config(rmi4_data, data->cmd_param[0]);
	msleep(200);

	if (retval < 0) {
		snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", tostring(NA));
		data->cmd_state = CMD_STATUS_FAIL;
		dev_err(rmi4_data->pdev->dev.parent, "%s: failed [%d]\n",
				__func__, retval);
		goto out;
	}

	snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", tostring(OK));
	data->cmd_state = CMD_STATUS_OK;
	dev_info(rmi4_data->pdev->dev.parent, "%s: success to save test result\n",
			__func__);

out:
	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));
}

static void get_tsp_test_result(void *dev_data)
{
	struct synaptics_rmi4_data *rmi4_data = (struct synaptics_rmi4_data *)dev_data;
	struct factory_data *data = rmi4_data->f54->factory_data;

	int result = 0;

	set_default_result(data);

	if (rmi4_data->touch_stopped) {
		dev_err(rmi4_data->pdev->dev.parent, "%s: [ERROR] Touch is stopped\n",
			__func__);
		snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", "TSP turned off");
		data->cmd_state = CMD_STATUS_NOT_APPLICABLE;
		goto out;
	}

	result = synaptics_rmi4_read_tsp_test_result(rmi4_data);

	if (result < TSP_FACTEST_RESULT_NONE || result > TSP_FACTEST_RESULT_PASS) {
		snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", tostring(NG));
		data->cmd_state = CMD_STATUS_FAIL;
		dev_err(rmi4_data->pdev->dev.parent, "%s: failed [%d]\n",
				__func__, result);
		goto out;
	}

	snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s",
		result == TSP_FACTEST_RESULT_PASS ? tostring(PASS) :
		result == TSP_FACTEST_RESULT_FAIL ? tostring(FAIL) :
		tostring(NONE));
	data->cmd_state = CMD_STATUS_OK;
	dev_info(rmi4_data->pdev->dev.parent, "%s: success [%s][%d]",	__func__,
		result == TSP_FACTEST_RESULT_PASS ? "PASS" :
		result == TSP_FACTEST_RESULT_FAIL ? "FAIL" :
		"NONE", result);

out:
	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));
}

#ifdef USE_ACTIVE_REPORT_RATE
int change_report_rate(struct synaptics_rmi4_data *rmi4_data, int mode)
{
	struct synaptics_rmi4_f54_handle *f54 = rmi4_data->f54;
	struct factory_data *data = f54->factory_data;

	int retval;
	unsigned char command = COMMAND_FORCE_UPDATE;
	unsigned char rpt_rate = 0;

	retval = rmi4_data->i2c_read(rmi4_data, f54->control.reg_94->address,
			f54->control.reg_94->data, sizeof(f54->control.reg_94->data));
	if (retval < 0) {
		dev_err(rmi4_data->pdev->dev.parent, "%s: Failed to read control_94 register.\n",
				__func__);
		goto out;
	}

	switch (mode) {
	case SYNAPTICS_RPT_RATE_90HZ:
		rpt_rate = SYNAPTICS_RPT_RATE_90HZ_VAL;
		break;
	case SYNAPTICS_RPT_RATE_60HZ:
		rpt_rate = SYNAPTICS_RPT_RATE_60HZ_VAL;
		break;
	case SYNAPTICS_RPT_RATE_30HZ:
		rpt_rate = SYNAPTICS_RPT_RATE_30HZ_VAL;
		break;
	}

	if (f54->control.reg_94->noise_bursts_per_cluster == rpt_rate) {
		return 0;
	}

	dev_info(rmi4_data->pdev->dev.parent,
		"%s: Set report rate %sHz [0x%02X->0x%02X]\n", __func__,
		data->cmd_param[0] == SYNAPTICS_RPT_RATE_90HZ ? "90" :
		data->cmd_param[0] == SYNAPTICS_RPT_RATE_60HZ ? "60" : "30",
		f54->control.reg_94->noise_bursts_per_cluster, rpt_rate);

	f54->control.reg_94->noise_bursts_per_cluster = rpt_rate;

	retval = rmi4_data->i2c_write(rmi4_data, f54->control.reg_94->address,
			f54->control.reg_94->data, sizeof(f54->control.reg_94->data));
	if (retval < 0) {
		dev_err(rmi4_data->pdev->dev.parent, "%s: Failed to write control_94 register.\n",
				__func__);
		goto out;
	}

	retval = rmi4_data->i2c_write(rmi4_data,
			f54->command_base_addr,
			&command, sizeof(command));
	if (retval < 0) {
		dev_err(rmi4_data->pdev->dev.parent, "%s: Failed to write force update command\n",
				__func__);
		goto out;
	}
	return 0;
out:
	return -1;
}
static void report_rate(void *dev_data)
{
	struct synaptics_rmi4_data *rmi4_data = (struct synaptics_rmi4_data *)dev_data;
	struct synaptics_rmi4_f54_handle *f54 = rmi4_data->f54;
	struct factory_data *data = f54->factory_data;

	int retval;

	set_default_result(data);

	if (data->cmd_param[0] < SYNAPTICS_RPT_RATE_START
		 || data->cmd_param[0] >= SYNAPTICS_RPT_RATE_END) {
		snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NG");
		data->cmd_state = CMD_STATUS_FAIL;
		goto out;
	}

	if (rmi4_data->tsp_change_report_rate == data->cmd_param[0]){
		dev_info(rmi4_data->pdev->dev.parent, "%s: already change report rate[%d]\n",
			__func__, rmi4_data->tsp_change_report_rate);
		goto success;
	} else
		rmi4_data->tsp_change_report_rate = data->cmd_param[0];

	if (rmi4_data->touch_stopped) {
		dev_err(rmi4_data->pdev->dev.parent, "%s: [ERROR] Touch is stopped\n",
			__func__);
		snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", "TSP turned off");
		data->cmd_state = CMD_STATUS_NOT_APPLICABLE;
		goto out;
	}

	retval = change_report_rate(rmi4_data, rmi4_data->tsp_change_report_rate);
	if (retval < 0) {
		dev_err(rmi4_data->pdev->dev.parent, "%s: change_report_rate func() error!\n",
				__func__);
		snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NG");
		data->cmd_state = CMD_STATUS_FAIL;
		goto out;
	}

success:
	snprintf(data->cmd_buff, sizeof(data->cmd_buff), "OK");
	data->cmd_state = CMD_STATUS_OK;

out:
	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));

	mutex_lock(&data->cmd_lock);
	data->cmd_is_running = false;
	mutex_unlock(&data->cmd_lock);

	data->cmd_state = CMD_STATUS_WAITING;
}
#endif

#ifdef TSP_SUPPROT_MULTIMEDIA
static void brush_enable(void *dev_data)
{
	struct synaptics_rmi4_data *rmi4_data = (struct synaptics_rmi4_data *)dev_data;
	struct factory_data *data = rmi4_data->f54->factory_data;

	int retval = 0;
	unsigned char boost_up_en = 0;

	set_default_result(data);

	if (data->cmd_param[0] < 0 || data->cmd_param[0] > 2) {
		snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NG");
		data->cmd_state = CMD_STATUS_FAIL;
		goto out;
	}

	if (rmi4_data->use_brush != (data->cmd_param[0] ? true : false)) {
		rmi4_data->use_brush = data->cmd_param[0] ? true : false;
		synpatics_rmi4_release_all_event(rmi4_data, RELEASE_TYPE_FINGER);
	}

	if (rmi4_data->touch_stopped) {
		dev_err(rmi4_data->pdev->dev.parent, "%s: Touch is stopped & Set flag[%d]\n",
			__func__, data->cmd_param[0]);
		snprintf(data->cmd_buff, sizeof(data->cmd_buff), "OK");
		data->cmd_state = CMD_STATUS_OK;
		goto out;
	}

	retval = rmi4_data->i2c_read(rmi4_data,
	rmi4_data->f51->general_control_addr, &boost_up_en, sizeof(boost_up_en));

	if (retval <= 0) {
		dev_err(rmi4_data->pdev->dev.parent, "%s: fail to read no_sleep[ret:%d]\n",
		__func__, retval);
		snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NG");
		data->cmd_state = CMD_STATUS_FAIL;
		goto out;
	}

	/* 0: Default   , 1: Enable Boost Up 1phi */
	if (data->cmd_param[0])
		boost_up_en |= BOOST_UP_EN;
	else
		boost_up_en &= ~(BOOST_UP_EN);

	retval = rmi4_data->i2c_write(rmi4_data,
	rmi4_data->f51->general_control_addr, &boost_up_en, sizeof(boost_up_en));

	if (retval < 0) {
		dev_err(rmi4_data->pdev->dev.parent, "%s: Failed to write.\n",
					__func__);
		snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NG");
		data->cmd_state = CMD_STATUS_FAIL;
		goto out;
	}

	snprintf(data->cmd_buff, sizeof(data->cmd_buff), "OK");
	data->cmd_state = CMD_STATUS_OK;

out:
	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));
	mutex_lock(&data->cmd_lock);
	data->cmd_is_running = false;
	mutex_unlock(&data->cmd_lock);

	data->cmd_state = CMD_STATUS_WAITING;
}

static void velocity_enable(void *dev_data)
{
	struct synaptics_rmi4_data *rmi4_data = (struct synaptics_rmi4_data *)dev_data;
	struct factory_data *data = rmi4_data->f54->factory_data;

#ifdef MM_MODE_CHANGE
	int retval = 0;
	unsigned char boost_up_en = 0;
#endif

	set_default_result(data);

	if (data->cmd_param[0] < 0 || data->cmd_param[0] > 2) {
		snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NG");
		data->cmd_state = CMD_STATUS_FAIL;
		goto out;
	}

	if (rmi4_data->use_velocity != (data->cmd_param[0] ? true : false)) {
		rmi4_data->use_velocity = data->cmd_param[0] ? true : false;
		synpatics_rmi4_release_all_event(rmi4_data, RELEASE_TYPE_FINGER);
	}

#ifdef MM_MODE_CHANGE
	if (rmi4_data->touch_stopped) {
		dev_err(rmi4_data->pdev->dev.parent, "%s: Touch is stopped & Set flag[%d]\n",
			__func__, data->cmd_param[0]);
		snprintf(data->cmd_buff, sizeof(data->cmd_buff), "OK");
		data->cmd_state = CMD_STATUS_OK;
		goto out;
	}

	retval = rmi4_data->i2c_read(rmi4_data,
	rmi4_data->f51->general_control_addr, &boost_up_en, sizeof(boost_up_en));

	if (retval <= 0) {
		dev_err(rmi4_data->pdev->dev.parent, "%s: fail to read no_sleep[ret:%d]\n",
						__func__, retval);
		snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NG");
		data->cmd_state = CMD_STATUS_FAIL;
		goto out;
	}

	/* 0: Default   , 1: Enable Boost Up 1phi */
	if (data->cmd_param[0])
		boost_up_en |= BOOST_UP_EN;
	else
		boost_up_en &= ~(BOOST_UP_EN);

	retval = rmi4_data->i2c_write(rmi4_data,
	rmi4_data->f51->general_control_addr, &boost_up_en, sizeof(boost_up_en));

	if (retval < 0) {
		dev_err(rmi4_data->pdev->dev.parent, "%s: Failed to write.\n",
						__func__);
		snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NG");
		data->cmd_state = CMD_STATUS_FAIL;
		goto out;
	}
#endif

	snprintf(data->cmd_buff, sizeof(data->cmd_buff), "OK");
	data->cmd_state = CMD_STATUS_OK;

out:
	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));
	mutex_lock(&data->cmd_lock);
	data->cmd_is_running = false;
	mutex_unlock(&data->cmd_lock);

	data->cmd_state = CMD_STATUS_WAITING;
}
#endif

#ifdef USE_STYLUS
static void stylus_enable(void *dev_data)
{
	struct synaptics_rmi4_data *rmi4_data = (struct synaptics_rmi4_data *)dev_data;
	struct factory_data *data = rmi4_data->f54->factory_data;

	unsigned char value;
	int retval = 0;

	set_default_result(data);

	if (data->cmd_param[0] < 0 || data->cmd_param[0] > 2) {
		snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NG");
		data->cmd_state = CMD_STATUS_FAIL;
		goto out;
	}

	if (rmi4_data->use_stylus != (data->cmd_param[0] ? true : false)) {
		rmi4_data->use_stylus = data->cmd_param[0] ? true : false;
		synpatics_rmi4_release_all_event(rmi4_data, RELEASE_TYPE_FINGER);
	}

	value = data->cmd_param[0] ? 0x01 : 0x00;

	retval = synaptics_rmi4_access_register(rmi4_data, SYNAPTICS_ACCESS_WRITE,
				rmi4_data->f51->forcefinger_onedge_addr, sizeof(value), &value);
	if (retval < 0) {
		dev_err(rmi4_data->pdev->dev.parent, "%s: Failed to write force finger on edge with [0x%02X].\n",
				__func__, value);
		snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NG");
		data->cmd_state = CMD_STATUS_FAIL;
		goto out;
	}

	snprintf(data->cmd_buff, sizeof(data->cmd_buff), "OK");
	data->cmd_state = CMD_STATUS_OK;

out:
	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));
}
#endif
#endif
static void not_support_cmd(void *dev_data)
{
	struct cmd_data *data = f54->cmd_data;

	set_default_result(data);
	snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", "NA");
	set_cmd_result(data, strlen(data->cmd_buff));
	data->cmd_state = CMD_STATUS_NOT_APPLICABLE;

	/* Some cmds are supported in specific IC and they are clear the cmd_is running flag
	 * itself(without show_cmd_result_) in their function such as hover_enable, glove_mode.
	 * So we need to clear cmd_is runnint flag if that command is replaced with
	 * not_support_cmd */
	mutex_lock(&data->cmd_lock);
	data->cmd_is_running = false;
	mutex_unlock(&data->cmd_lock);
}

static struct sec_cmd ts_cmds[] = {
	{SEC_CMD("fw_update", fw_update),},
	{SEC_CMD("get_fw_ver_bin", get_fw_ver_bin),},
	{SEC_CMD("get_fw_ver_ic", get_fw_ver_ic),},
	{SEC_CMD("get_config_ver", get_config_ver),},
	{SEC_CMD("get_checksum_data", get_checksum_data),},
	{SEC_CMD("get_threshold", get_threshold),},
	{SEC_CMD("module_off_master", module_off_master),},
	{SEC_CMD("module_on_master", module_on_master),},
	{SEC_CMD("module_off_slave", not_support_cmd),},
	{SEC_CMD("module_on_slave", not_support_cmd),},
	{SEC_CMD("get_chip_vendor", get_chip_vendor),},
	{SEC_CMD("get_chip_name", get_chip_name),},
	{SEC_CMD("get_x_num", get_x_num),},
	{SEC_CMD("get_y_num", get_y_num),},
	{SEC_CMD("get_rawcap", get_rawcap),},
	{SEC_CMD("run_rawcap_read", run_rawcap_read),},
#if 0	
	{SEC_CMD("get_delta", get_delta),},
	{SEC_CMD("run_delta_read", run_delta_read),},
	{SEC_CMD("run_abscap_read", run_abscap_read),},
	{SEC_CMD("get_abscap_read_test", get_abscap_read_test),},
	{SEC_CMD("run_absdelta_read", run_absdelta_read),},
	{SEC_CMD("run_trx_short_test", run_trx_short_test),},
	{SEC_CMD("dead_zone_enable", dead_zone_enable),},
#ifdef PROXIMITY_MODE
	{SEC_CMD("hover_enable", hover_enable),},
	{SEC_CMD("hover_no_sleep_enable", hover_no_sleep_enable),},
	{SEC_CMD("hover_set_edge_rx", hover_set_edge_rx),},
#endif
	{SEC_CMD("set_jitter_level", set_jitter_level),},
	{SEC_CMD("handgrip_enable", not_support_cmd),},
#ifdef GLOVE_MODE
	{SEC_CMD("glove_mode", glove_mode),},
	{SEC_CMD("clear_cover_mode", clear_cover_mode),},
	{SEC_CMD("fast_glove_mode", fast_glove_mode),},
	{SEC_CMD("get_glove_sensitivity", not_support_cmd),},
#endif
#ifdef TSP_BOOSTER
	{SEC_CMD("boost_level", boost_level),},
#endif
#ifdef SIDE_TOUCH
	{SEC_CMD("sidekey_enable", sidekey_enable),},
	{SEC_CMD("set_sidekey_only_enable", set_sidekey_only_enable),},
	{SEC_CMD("get_sidekey_threshold", get_sidekey_threshold),},
	{SEC_CMD("run_sidekey_delta_read", run_sidekey_delta_read),},
	{SEC_CMD("run_sidekey_abscap_read", run_sidekey_abscap_read),},
	{SEC_CMD("set_deepsleep_mode", set_deepsleep_mode),},
	{SEC_CMD("lozemode_enable", lozemode_enable),},
#endif
	{SEC_CMD("set_tsp_test_result", set_tsp_test_result),},
	{SEC_CMD("get_tsp_test_result", get_tsp_test_result),},
#ifdef USE_ACTIVE_REPORT_RATE
	{SEC_CMD("report_rate", report_rate),},
#endif
#ifdef USE_STYLUS
	{SEC_CMD("stylus_enable", stylus_enable),},
#endif
#ifdef TSP_SUPPROT_MULTIMEDIA
	{SEC_CMD("brush_enable", brush_enable),},
	{SEC_CMD("velocity_enable", velocity_enable),},
#endif
#endif
	{SEC_CMD("not_support_cmd", not_support_cmd),},
};

static ssize_t cmd_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;
	struct cmd_data *data = f54->cmd_data;
	struct sec_cmd *cmd_ptr;
	unsigned char param_cnt = 0;
	char *start, *end, *pos;
	char delim = ',';
	char buffer[CMD_STR_LEN];
	bool cmd_found = false;
	int *param;
	int length;

	if (strlen(buf) >= CMD_STR_LEN) {		
		dev_err(rmi4_data->pdev->dev.parent,
			"%s: cmd length is over (%s,%d)!!\n", __func__, buf, (int)strlen(buf));
		return -EINVAL;
	}

	if (data->cmd_is_running == true) {
		dev_err(rmi4_data->pdev->dev.parent,
			"%s: Still servicing previous command. Skip cmd - %s\n",
			__func__, buf);
		return count;
	}

	mutex_lock(&data->cmd_lock);
	data->cmd_is_running = true;
	mutex_unlock(&data->cmd_lock);

	data->cmd_state = CMD_STATUS_RUNNING;

	length = (int)count;
	if (*(buf + length - 1) == '\n')
		length--;

	memset(data->cmd, 0x00, sizeof(data->cmd));
	memcpy(data->cmd, buf, length);
	memset(data->cmd_param, 0, sizeof(data->cmd_param));

	memset(buffer, 0x00, sizeof(buffer));
	pos = strchr(buf, (int)delim);
	if (pos)
		memcpy(buffer, buf, pos - buf);
	else
		memcpy(buffer, buf, length);

	/* find command */
	list_for_each_entry(cmd_ptr, &data->cmd_list_head, list) {
		if (!cmd_ptr) {
			dev_err(rmi4_data->pdev->dev.parent, "%s: cmd_ptr is NULL\n", __func__);
			return count;
		}

		if (!cmd_ptr->cmd_name) {
			dev_err(rmi4_data->pdev->dev.parent, "%s: cmd_ptr->cmd_name is NULL\n",
				__func__);
			return count;
		}

		if (!strcmp(buffer, cmd_ptr->cmd_name)) {
			cmd_found = true;
			break;
		}
	}

	/* set not_support_cmd */
	if (!cmd_found) {
		list_for_each_entry(cmd_ptr,
			&data->cmd_list_head, list) {
			if (!strcmp("not_support_cmd", cmd_ptr->cmd_name))
				break;
		}
	}

	/* parsing parameters */
	if (cmd_found && pos) {
		pos++;
		start = pos;

		do {
			if ((*pos == delim) || (pos - buf == length)) {
				end = pos;
				memset(buffer, 0x00, sizeof(buffer));
				memcpy(buffer, start, end - start);
				*(buffer + strlen(buffer)) = '\0';
				param = data->cmd_param + param_cnt;
				if (kstrtoint(buffer, 10, param) < 0)
					break;
				param_cnt++;
				start = pos + 1;
			}

			pos++;
		} while ((pos - buf <= length) && (param_cnt < CMD_PARAM_NUM));
	}

	dev_info(rmi4_data->pdev->dev.parent, "%s: Command = %s", __func__, buf);

	cmd_ptr->cmd_func(data);

	return count;
}

static ssize_t cmd_status_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;
	struct cmd_data *data = f54->cmd_data;
	char buff[CMD_STR_LEN];

	switch (data->cmd_state) {
	case CMD_STATUS_WAITING:
		snprintf(buff, sizeof(buff), "WAITING");
		break;
	case CMD_STATUS_RUNNING:
		snprintf(buff, sizeof(buff), "RUNNING");
		break;
	case CMD_STATUS_OK:
		snprintf(buff, sizeof(buff), "OK");
		break;
	case CMD_STATUS_FAIL:
		snprintf(buff, sizeof(buff), "FAIL");
		break;
	case CMD_STATUS_NOT_APPLICABLE:
		snprintf(buff, sizeof(buff), "NOT_APPLICABLE");
		break;
	default:
		snprintf(buff, sizeof(buff), "NOT_APPLICABLE");
		break;
	}

	dev_info(rmi4_data->pdev->dev.parent,
		"%s: Command status = %d\n", __func__, data->cmd_state);

	return snprintf(buf, PAGE_SIZE, "%s\n", buff);
}

static ssize_t cmd_result_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;
	struct cmd_data *data = f54->cmd_data;

	mutex_lock(&data->cmd_lock);
	data->cmd_is_running = false;
	mutex_unlock(&data->cmd_lock);

	data->cmd_state = CMD_STATUS_WAITING;

	dev_info(rmi4_data->pdev->dev.parent,
		"%s: Command result = %s\n", __func__, data->cmd_result);

	return snprintf(buf, PAGE_SIZE, "%s\n", data->cmd_result);
}

static ssize_t cmd_list_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cmd_data *data = f54->cmd_data;
	struct sec_cmd *cmd_ptr;
	char buff_name[CMD_STR_LEN];
	char *str = "++ sec command list ++";
	char *buff;
	int buff_size;
	int retval;

	buff_size = data->cmd_list_size + strlen(str) + 1;
	buff = kzalloc(buff_size, GFP_KERNEL);

	snprintf(buff, buff_size, "%s\n", str);

	list_for_each_entry(cmd_ptr, &data->cmd_list_head, list) {
		if (strcmp(cmd_ptr->cmd_name, "not_support_cmd")) {
			snprintf(buff_name, CMD_STR_LEN,
				"%s\n", cmd_ptr->cmd_name);
			strcat(buff, buff_name);
		}
	}

	retval = snprintf(buf, PAGE_SIZE, "%s", buff);

	kfree(buff);

	return retval;
}
#if 0
static ssize_t debug_address_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);
	struct synaptics_rmi4_f51_handle *f51 = rmi4_data->f51;
	char buffer_temp[DEBUG_STR_LEN] = {0,};

	memset(debug_buffer, 0, DEBUG_RESULT_STR_LEN);

	DEBUG_PRNT_SCREEN(debug_buffer, buffer_temp, DEBUG_STR_LEN, "\n### F12 User control Registers ###\n");
	DEBUG_PRNT_SCREEN(debug_buffer, buffer_temp, DEBUG_STR_LEN, "#F12_2D_CTRL11(jitter)\t 0x%04x, 0x%02x\n",
			rmi4_data->f12.ctrl11_addr, 0xFF);
	DEBUG_PRNT_SCREEN(debug_buffer, buffer_temp, DEBUG_STR_LEN, "#F12_2D_CTRL15(threshold)\t 0x%04x, 0x%02x\n",
			rmi4_data->f12.ctrl15_addr, 0xFF);
#ifdef GLOVE_MODE
	DEBUG_PRNT_SCREEN(debug_buffer, buffer_temp, DEBUG_STR_LEN, "#F12_2D_CTRL23(obj_type)\t 0x%04x, 0x%02x\n",
			rmi4_data->f12.ctrl23_addr, rmi4_data->f12.obj_report_enable);
	DEBUG_PRNT_SCREEN(debug_buffer, buffer_temp, DEBUG_STR_LEN, "#F12_2D_CTRL26(glove)\t 0x%04x, 0x%02x\n",
			rmi4_data->f12.ctrl26_addr, rmi4_data->f12.feature_enable);
#endif
	DEBUG_PRNT_SCREEN(debug_buffer, buffer_temp, DEBUG_STR_LEN, "#F12_2D_CTRL28(report)\t 0x%04x, 0x%02x\n",
			rmi4_data->f12.ctrl28_addr, rmi4_data->f12.report_enable);

	DEBUG_PRNT_SCREEN(debug_buffer, buffer_temp, DEBUG_STR_LEN, "\n### F51 User control Registers ###\n");
	DEBUG_PRNT_SCREEN(debug_buffer, buffer_temp, DEBUG_STR_LEN, "#F51_CUSTOM_CTRL00(proximity)\t 0x%04x, 0x%02x\n",
			f51->proximity_enables_addr, f51->proximity_enables);
	DEBUG_PRNT_SCREEN(debug_buffer, buffer_temp, DEBUG_STR_LEN, "#F51_CUSTOM_CTRL01(general)\t 0x%04x, 0x%02x\n",
			f51->general_control_addr, f51->general_control);
	DEBUG_PRNT_SCREEN(debug_buffer, buffer_temp, DEBUG_STR_LEN, "#F51_CUSTOM_CTRL02(general2)\t 0x%04x, 0x%02x\n",
			f51->general_control_2_addr, f51->general_control_2);
#ifdef SIDE_TOUCH
	DEBUG_PRNT_SCREEN(debug_buffer, buffer_temp, DEBUG_STR_LEN, "#F51_CUSTOM_DATA (Side button)\t 0x%04x, 0x%02x\n",
			f51->side_button_data_addr, 0xFF);
#endif
#ifdef USE_DETECTION_FLAG_2
	DEBUG_PRNT_SCREEN(debug_buffer, buffer_temp, DEBUG_STR_LEN, "#F51_CUSTOM_DATA (Detection flag2)\t 0x%04x, 0x%02x\n",
			f51->detection_flag_2_addr, 0xFF);
#endif
#ifdef EDGE_SWIPE
	DEBUG_PRNT_SCREEN(debug_buffer, buffer_temp, DEBUG_STR_LEN, "#F51_CUSTOM_DATA (Edge swipe)\t 0x%04x, 0x%02x\n",
			f51->edge_swipe_data_addr, 0xFF);
#endif

	DEBUG_PRNT_SCREEN(debug_buffer, buffer_temp, DEBUG_STR_LEN, "\n### Manual defined offset ###\n");
#ifdef PROXIMITY_MODE
	DEBUG_PRNT_SCREEN(debug_buffer, buffer_temp, DEBUG_STR_LEN, "#F51_CUSTOM_CTRL24(Grip edge exclusion RX)\t 0x%04x\n",
			f51->grip_edge_exclusion_rx_addr);
#endif
#ifdef SIDE_TOUCH
	DEBUG_PRNT_SCREEN(debug_buffer, buffer_temp, DEBUG_STR_LEN, "#F51_CUSTOM_CTRL78(Side button tap threshold)\t 0x%04x\n",
			f51->sidebutton_tapthreshold_addr);
#endif
#ifdef USE_STYLUS
	DEBUG_PRNT_SCREEN(debug_buffer, buffer_temp, DEBUG_STR_LEN, "#F51_CUSTOM_CTRL87(ForceFingeronEdge)\t 0x%04x\n",
			f51->forcefinger_onedge_addr);
#endif

	return snprintf(buf, PAGE_SIZE, "%s\n", debug_buffer);
}

static ssize_t debug_register_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", debug_buffer);
}

static ssize_t debug_register_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{

	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);
	unsigned int mode, page, addr, offset, param;
	unsigned char i;
	unsigned short register_addr;
	unsigned char *register_val;

	char buffer_temp[DEBUG_STR_LEN] = {0,};

	int retval = 0;

	memset(debug_buffer, 0, DEBUG_RESULT_STR_LEN);

	if (sscanf(buf, "%x%x%x%x%x", &mode, &page, &addr, &offset, &param) != 5) {
		DEBUG_PRNT_SCREEN(debug_buffer, buffer_temp, DEBUG_STR_LEN, "\n### keep below format !!!! ###\n");
		DEBUG_PRNT_SCREEN(debug_buffer, buffer_temp, DEBUG_STR_LEN, "### mode page_num address offset data ###\n");
		DEBUG_PRNT_SCREEN(debug_buffer, buffer_temp, DEBUG_STR_LEN, "### (EX: 1 4 15 1 10 > debug_address) ###\n");
		DEBUG_PRNT_SCREEN(debug_buffer, buffer_temp, DEBUG_STR_LEN, "### Write 0x10 value at 0x415[1] address ###\n");
		DEBUG_PRNT_SCREEN(debug_buffer, buffer_temp, DEBUG_STR_LEN, "### if packet register, offset mean [register/offset] ###\n");
		DEBUG_PRNT_SCREEN(debug_buffer, buffer_temp, DEBUG_STR_LEN, "### (EX: 0 4 15 0 a > debug_address) ###\n");
		DEBUG_PRNT_SCREEN(debug_buffer, buffer_temp, DEBUG_STR_LEN, "### Read 10byte from 0x415 address ###\n");
		goto out;
	}

	if (rmi4_data->touch_stopped) {
		DEBUG_PRNT_SCREEN(debug_buffer, buffer_temp, DEBUG_STR_LEN, "\n### ERROR : Sensor stopped\n");
		goto out;
	}

	register_addr = (page << 8) | addr;

	if (mode) {
		DEBUG_PRNT_SCREEN(debug_buffer, buffer_temp, DEBUG_STR_LEN, "\n### Write [0x%02x]value at [0x%04x/0x%02x]address.\n",
			param, register_addr, offset);

		if (offset) {
			if (offset > MAX_VAL_OFFSET_AND_LENGTH) {
				DEBUG_PRNT_SCREEN(debug_buffer, buffer_temp, DEBUG_STR_LEN, "\n### offset is too large. [ < %d]\n", MAX_VAL_OFFSET_AND_LENGTH);
				goto out;
			}
			register_val = kzalloc(offset + 1, GFP_KERNEL);

			retval = synaptics_rmi4_access_register(rmi4_data, SYNAPTICS_ACCESS_READ, register_addr, offset + 1, register_val);
			if (retval < 0) {
				DEBUG_PRNT_SCREEN(debug_buffer, buffer_temp, DEBUG_STR_LEN, "\n### Failed to read\n");
				goto free_mem;
			}
			register_val[offset] = param;

			for (i = 0; i < offset + 1; i++)
				DEBUG_PRNT_SCREEN(debug_buffer, buffer_temp, DEBUG_STR_LEN, "### offset[%d] --> 0x%02x ###\n", i, register_val[i]);

			retval = synaptics_rmi4_access_register(rmi4_data, SYNAPTICS_ACCESS_WRITE, register_addr, offset + 1, register_val);
			if (retval < 0) {
				DEBUG_PRNT_SCREEN(debug_buffer, buffer_temp, DEBUG_STR_LEN, "\n### Failed to write\n");
				goto free_mem;
			}
		} else {
			register_val = kzalloc(1, GFP_KERNEL);

			*register_val = param;

			DEBUG_PRNT_SCREEN(debug_buffer, buffer_temp, DEBUG_STR_LEN, "### 0x%04x --> 0x%02x ###\n", register_addr, *register_val);

			retval = synaptics_rmi4_access_register(rmi4_data, SYNAPTICS_ACCESS_WRITE, register_addr, 1, register_val);
			if (retval < 0) {
				DEBUG_PRNT_SCREEN(debug_buffer, buffer_temp, DEBUG_STR_LEN, "\n### Failed to write\n");
				goto free_mem;
			}
		}
	} else {
		DEBUG_PRNT_SCREEN(debug_buffer, buffer_temp, DEBUG_STR_LEN, "\n### Read [%u]byte from [0x%04x]address.\n",
			param, register_addr);

		if (param > MAX_VAL_OFFSET_AND_LENGTH) {
			DEBUG_PRNT_SCREEN(debug_buffer, buffer_temp, DEBUG_STR_LEN, "\n### length is too large. [ < %d]\n", MAX_VAL_OFFSET_AND_LENGTH);
			goto out;
		}

		register_val = kzalloc(param, GFP_KERNEL);

		retval = synaptics_rmi4_access_register(rmi4_data, SYNAPTICS_ACCESS_READ, register_addr, param, register_val);
		if (retval < 0) {
			DEBUG_PRNT_SCREEN(debug_buffer, buffer_temp, DEBUG_STR_LEN, "\n### Failed to read\n");
			goto free_mem;
		}

		for (i = 0; i < param; i++)
			DEBUG_PRNT_SCREEN(debug_buffer, buffer_temp, DEBUG_STR_LEN, "### offset[%d] --> 0x%02x ###\n", i, register_val[i]);
	}

free_mem:
	kfree(register_val);

out:
	return count;
}
#endif
static DEVICE_ATTR(cmd, (S_IWUSR | S_IWGRP), NULL, cmd_store);
static DEVICE_ATTR(cmd_status, S_IRUGO, cmd_status_show, NULL);
static DEVICE_ATTR(cmd_result, S_IRUGO, cmd_result_show, NULL);
static DEVICE_ATTR(cmd_list, S_IRUGO, cmd_list_show, NULL);
#if 0
static DEVICE_ATTR(debug_address, S_IRUGO, debug_address_show, NULL);
static DEVICE_ATTR(debug_register, (S_IRUGO | S_IWUSR | S_IWGRP),
			debug_register_show, debug_register_store);
#endif
static struct attribute *cmd_attributes[] = {
	&dev_attr_cmd.attr,
	&dev_attr_cmd_status.attr,
	&dev_attr_cmd_result.attr,
	&dev_attr_cmd_list.attr,
#if 0
	&dev_attr_debug_address.attr,
	&dev_attr_debug_register.attr,
#endif
	NULL,
};

static struct attribute_group cmd_attr_group = {
	.attrs = cmd_attributes,
};

static void synaptics_rmi4_remove_ts_cmd(void)
{
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;
	struct cmd_data *data = f54->cmd_data;

	sysfs_delete_link(&data->ts_dev->kobj,
				&rmi4_data->input_dev->dev.kobj, "input");
	sysfs_remove_group(&data->ts_dev->kobj, &cmd_attr_group);
	sec_device_destroy((dev_t)((size_t)&data->ts_dev));
	kfree(data->trx_short);
	kfree(data->absdelta_data);
	kfree(data->abscap_data);
	kfree(data->delta_data);
	kfree(data->rawcap_data);
	kfree(data);
	data = NULL;
}

static int synaptics_rmi4_init_ts_cmd(void)
{
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;
	struct cmd_data *data;
	unsigned char rx;
	unsigned char tx;
	int retval = 0, i;

	rx = f54->rx_assigned;
	tx = f54->tx_assigned;

	retval = -ENOMEM;

	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (!data) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: Failed to alloc mem for cmd_data\n",
				__func__);
		return retval;
	}

	data->rawcap_data = kzalloc(2 * rx * tx, GFP_KERNEL);
	if (!data->rawcap_data) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: Failed to alloc mem for rawcap_data\n",
				__func__);

		goto exit_rawcap_data;
	}

	data->delta_data = kzalloc(2 * rx * tx, GFP_KERNEL);
	if (!data->delta_data) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: Failed to alloc mem for delta_data\n",
				__func__);
		goto exit_delta_data;
	}

	data->abscap_data = kzalloc(4 * rx * tx, GFP_KERNEL);
	if (!data->abscap_data) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: Failed to alloc mem for abscap_data\n",
				__func__);
		goto exit_abscap_data;
	}

	data->absdelta_data = kzalloc(4 * rx * tx, GFP_KERNEL);
	if (!data->abscap_data) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: Failed to alloc mem for abscap_data\n",
				__func__);
		goto exit_absdelta_data;
	}

	data->trx_short = kzalloc(TREX_DATA_SIZE, GFP_KERNEL);
	if (!data->trx_short) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: Failed to alloc mem for trx_short\n",
				__func__);
		goto exit_trx_short;
	}

	data->cmd_list_size = 0;
	INIT_LIST_HEAD(&data->cmd_list_head);
	for (i = 0; i < ARRAY_SIZE(ts_cmds); i++) {
		list_add_tail(&ts_cmds[i].list, &data->cmd_list_head);
		data->cmd_list_size += strlen(ts_cmds[i].cmd_name) + 1;
	}

	mutex_init(&data->cmd_lock);
	data->cmd_is_running = false;
	data->ts_dev = sec_device_create((dev_t)((size_t)&data->ts_dev), f54, "tsp");
	if (IS_ERR(data->ts_dev)) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: Failed to create device for the sysfs\n",
				__func__);
		retval = PTR_ERR(data->ts_dev);
		goto exit_ts_dev;
	}

	retval = sysfs_create_group(&data->ts_dev->kobj, &cmd_attr_group);
	if (retval) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: Failed to create sysfs attributes\n",
				__func__);
		goto exit_cmd_attr_group;
	}

	retval = sysfs_create_link(&data->ts_dev->kobj,
				&rmi4_data->input_dev->dev.kobj, "input");
	if (retval) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: Failed to create link\n", __func__);
		goto exit_sysfs_create_link;
	}

	dev_set_drvdata(data->ts_dev, rmi4_data);
	f54->cmd_data = data;

	return 0;

exit_sysfs_create_link:
	sysfs_remove_group(&data->ts_dev->kobj, &cmd_attr_group);
exit_cmd_attr_group:
	sec_device_destroy((dev_t)((size_t)&data->ts_dev));
exit_ts_dev:
	kfree(data->trx_short);
exit_trx_short:
	kfree(data->absdelta_data);
exit_absdelta_data:
	kfree(data->abscap_data);
exit_abscap_data:
	kfree(data->delta_data);
exit_delta_data:
	kfree(data->rawcap_data);
exit_rawcap_data:
	kfree(data);
	data = NULL;

	return retval;
}
#endif

static int synaptics_rmi4_test_init(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;

	if (f54) {
		dev_dbg(rmi4_data->pdev->dev.parent,
				"%s: Handle already exists\n",
				__func__);
		return 0;
	}

	f54 = kzalloc(sizeof(*f54), GFP_KERNEL);
	if (!f54) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: Failed to alloc mem for F54\n",
				__func__);
		retval = -ENOMEM;
		goto exit;
	}

	f54->rmi4_data = rmi4_data;

	f55 = NULL;

	f21 = NULL;

	retval = test_scan_pdt();
	if (retval < 0)
		goto exit_free_mem;

	retval = test_set_queries();
	if (retval < 0) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: Failed to read F54 query registers\n",
				__func__);
		goto exit_free_mem;
	}

	f54->tx_assigned = f54->query.num_of_tx_electrodes;
	f54->rx_assigned = f54->query.num_of_rx_electrodes;

	retval = test_set_controls();
	if (retval < 0) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: Failed to set up F54 control registers\n",
				__func__);
		goto exit_free_control;
	}

	test_set_data();

	if (f55)
		test_f55_init(rmi4_data);

	if (f21)
		test_f21_init(rmi4_data);

	if (rmi4_data->external_afe_buttons)
		f54->tx_assigned++;

	retval = test_set_sysfs();
	if (retval < 0) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: Failed to create sysfs entries\n",
				__func__);
		goto exit_sysfs;
	}

	f54->test_report_workqueue =
			create_singlethread_workqueue("test_report_workqueue");
	INIT_WORK(&f54->test_report_work, test_report_work);

	hrtimer_init(&f54->watchdog, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	f54->watchdog.function = test_get_report_timeout;
	INIT_WORK(&f54->timeout_work, test_timeout_work);

	mutex_init(&f54->status_mutex);
	f54->status = STATUS_IDLE;

#ifdef CONFIG_DRV_SAMSUNG
	retval = synaptics_rmi4_init_ts_cmd();
	if (retval) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: Failed to init ts cmd\n", __func__);
		goto exit_init_ts_cmd;
	}
#endif

	return 0;

#ifdef CONFIG_DRV_SAMSUNG
exit_init_ts_cmd:
	test_remove_sysfs();
#endif
exit_sysfs:
	if (f21)
		kfree(f21->force_txrx_assignment);

	if (f55) {
		kfree(f55->tx_assignment);
		kfree(f55->rx_assignment);
		kfree(f55->force_tx_assignment);
		kfree(f55->force_rx_assignment);
	}

exit_free_control:
	test_free_control_mem();

exit_free_mem:
	kfree(f21);
	f21 = NULL;
	kfree(f55);
	f55 = NULL;
	kfree(f54);
	f54 = NULL;

exit:
	return retval;
}

static void synaptics_rmi4_test_remove(struct synaptics_rmi4_data *rmi4_data)
{
	if (!f54)
		goto exit;

#ifdef CONFIG_DRV_SAMSUNG
	synaptics_rmi4_remove_ts_cmd();
#endif

	hrtimer_cancel(&f54->watchdog);

	cancel_work_sync(&f54->test_report_work);
	flush_workqueue(f54->test_report_workqueue);
	destroy_workqueue(f54->test_report_workqueue);

	test_remove_sysfs();

	if (f21)
		kfree(f21->force_txrx_assignment);

	if (f55) {
		kfree(f55->tx_assignment);
		kfree(f55->rx_assignment);
		kfree(f55->force_tx_assignment);
		kfree(f55->force_rx_assignment);
	}

	test_free_control_mem();

	if (f54->data_buffer_size)
		kfree(f54->report_data);

	kfree(f21);
	f21 = NULL;

	kfree(f55);
	f55 = NULL;

	kfree(f54);
	f54 = NULL;

exit:
	complete(&test_remove_complete);

	return;
}

static void synaptics_rmi4_test_reset(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;

	if (!f54) {
		synaptics_rmi4_test_init(rmi4_data);
		return;
	}

	if (f21)
		kfree(f21->force_txrx_assignment);

	if (f55) {
		kfree(f55->tx_assignment);
		kfree(f55->rx_assignment);
		kfree(f55->force_tx_assignment);
		kfree(f55->force_rx_assignment);
	}

	test_free_control_mem();

	kfree(f55);
	f55 = NULL;

	kfree(f21);
	f21 = NULL;

	retval = test_scan_pdt();
	if (retval < 0)
		goto exit_free_mem;

	retval = test_set_queries();
	if (retval < 0) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: Failed to read F54 query registers\n",
				__func__);
		goto exit_free_mem;
	}

	f54->tx_assigned = f54->query.num_of_tx_electrodes;
	f54->rx_assigned = f54->query.num_of_rx_electrodes;

	retval = test_set_controls();
	if (retval < 0) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: Failed to set up F54 control registers\n",
				__func__);
		goto exit_free_control;
	}

	test_set_data();

	if (f55)
		test_f55_init(rmi4_data);

	if (f21)
		test_f21_init(rmi4_data);

	if (rmi4_data->external_afe_buttons)
		f54->tx_assigned++;

	f54->status = STATUS_IDLE;

	return;

exit_free_control:
	test_free_control_mem();

exit_free_mem:
	hrtimer_cancel(&f54->watchdog);

	cancel_work_sync(&f54->test_report_work);
	flush_workqueue(f54->test_report_workqueue);
	destroy_workqueue(f54->test_report_workqueue);

	test_remove_sysfs();

	if (f54->data_buffer_size)
		kfree(f54->report_data);

	kfree(f21);
	f21 = NULL;

	kfree(f55);
	f55 = NULL;

	kfree(f54);
	f54 = NULL;

	return;
}

static struct synaptics_rmi4_exp_fn test_module = {
	.fn_type = RMI_TEST_REPORTING,
	.init = synaptics_rmi4_test_init,
	.remove = synaptics_rmi4_test_remove,
	.reset = synaptics_rmi4_test_reset,
	.reinit = NULL,
	.early_suspend = NULL,
	.suspend = NULL,
	.resume = NULL,
	.late_resume = NULL,
	.attn = synaptics_rmi4_test_attn,
};

static int __init rmi4_test_module_init(void)
{
	synaptics_rmi4_new_function(&test_module, true);

	return 0;
}

static void __exit rmi4_test_module_exit(void)
{
	synaptics_rmi4_new_function(&test_module, false);

	wait_for_completion(&test_remove_complete);

	return;
}

#ifdef CONFIG_SEC_INCELL
late_initcall_sync(rmi4_test_module_init);
#else
module_init(rmi4_test_module_init);
#endif
module_exit(rmi4_test_module_exit);

MODULE_AUTHOR("Synaptics, Inc.");
MODULE_DESCRIPTION("Synaptics DSX Test Reporting Module");
MODULE_LICENSE("GPL v2");
