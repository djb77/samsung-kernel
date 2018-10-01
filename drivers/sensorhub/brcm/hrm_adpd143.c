/**
*ADPD143 driver source
* (c) copyright 2016 Analog Devices Inc.
*
* Based on ADPD142RI driver
* Added ADPD143 specific by Joshua Yoon(joshua.yoon@analog.com)
* 25th/Feb/2016 : Added TIA_ADC/float mode support for ALS by Joshua Yoon(joshua.yoon@analog.com)
* 25th/Feb/2016 : Added DC normalization support by Joshua Yoon(joshua.yoon@analog.com)
* 23rd/Mar/2016 : Added AGC(Automatic Gain Control) support by Joshua Yoon(joshua.yoon@analog.com)
* 25th/Mar/2016 : Added EOL test support by Joshua Yoon(joshua.yoon@analog.com)
* 30th/Mar/2016 : Enhanced EOL clock calibration by Joshua Yoon(joshua.yoon@analog.com)
* 30th/Mar/2016 : Added Object Proximity Detection with the dynamic AGC trigger by Joshua Yoon(joshua.yoon@analog.com)
* 1st/Apr/2016 : Enhanced AGC by Joshua Yoon(joshua.yoon@analog.com)
* 12nd/Apr/2016 : Enhanced AGC/Proximity integration and some run-time bug fix by Joshua Yoon(joshua.yoon@analog.com)
* 25nd/Apr/2016 : Added melanin sensor mode by Joshua Yoon(joshua.yoon@analog.com)
* 11th/May/2016 : Bug fix of the unwanted change of 'cur_proximity_dc_level' by AGC  by Joshua Yoon(joshua.yoon@analog.com)
* 11th/May/2016 : Added the option in calculating PD channel sum to select the best perfusion index by Joshua Yoon(joshua.yoon@analog.com)
* 11th/May/2016 : Added FIR lowpass filter for reducing high frequency channel noise by Joshua Yoon(joshua.yoon@analog.com)
* 11th/May/2016 : Enhanced dark calibration to remove DC noise by Joshua Yoon(joshua.yoon@analog.com)
* 11th/May/2016 : AGC fix to address some corner cases by Joshua Yoon(joshua.yoon@analog.com)
* 11th/May/2016 : Enhanced the handle of PD saturation during HR measurement by Joshua Yoon(joshua.yoon@analog.com)
* 11th/May/2016 : Enhanced AGC to handle the slow LED intensity settledown when driving high LED current by Joshua Yoon(joshua.yoon@analog.com)
* 12nd/May/2016 : Added time domain SNR analysis by Joshua Yoon(joshua.yoon@analog.com)
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*/

#include "hrmsensor.h"
#include "hrm_adpd143.h"

#define MELANIN_IDX_SUPPORT

#define __USE_PD34__
//#define __USE_LOWPASS_FIR_FILTER__
//#define __USE_TIME_DOMAIN_SNR__

extern int hrm_debug;
extern int hrm_info;

#define MAX_NUM_SAMPLE 4

#define CHIP_NAME			"ADPD143RI"
#define VENDOR				"ADI"

#define ADPD143_SLAVE_ADDR		0x64

#define ADPD_CHIPID_0			0x0016
#define ADPD_CHIPID_1			0x0116
#define ADPD_CHIPID_2			0x0216
#define ADPD_CHIPID_3			0x0316
#define ADPD_CHIPID_4			0x0416

#define ADPD_CHIPID(id)			ADPD_CHIPID_##id

#define ADPD_INT_STATUS_ADDR		0x00
#define ADPD_INT_MASK_ADDR			0x01
#define ADPD_INT_IO_CTRL_ADDR		0x02
#define ADPD_FIFO_THRESH_ADDR		0x06
#define ADPD_CHIPID_ADDR			0x08
#define ADPD_OP_MODE_ADDR			0x10
#define ADPD_OP_MODE_CFG_ADDR		0x11
#define ADPD_SAMPLING_FREQ_ADDR		0x12
#define ADPD_PD_SELECT_ADDR			0x14
#define ADPD_DEC_MODE_ADDR			0x15
#define ADPD_CH1_OFFSET_A_ADDR		0x18
#define ADPD_CH2_OFFSET_A_ADDR		0x19
#define ADPD_CH3_OFFSET_A_ADDR		0x1A
#define ADPD_CH4_OFFSET_A_ADDR		0x1B
#define ADPD_CH1_OFFSET_B_ADDR		0x1E
#define ADPD_CH2_OFFSET_B_ADDR		0x1F
#define ADPD_CH3_OFFSET_B_ADDR		0x20
#define ADPD_CH4_OFFSET_B_ADDR		0x21
#define ADPD_LED3_DRV_ADDR			0x22
#define ADPD_LED1_DRV_ADDR			0x23 /* Red */
#define ADPD_LED2_DRV_ADDR			0x24 /* IR */
#define ADPD_LED_TRIM_ADDR			0x25
#define ADPD_GEST_CTRL_ADDR			0x27
#define ADPD_GEST_THRESH_ADDR		0x28
#define ADPD_GEST_SIZE_ADDR			0x29
#define ADPD_PROX_ON_TH1_ADDR		0x2A
#define ADPD_PROX_OFF_TH1_ADDR		0x2B
#define ADPD_PROX_ON_TH2_ADDR		0x2C
#define ADPD_PROX_OFF_TH2_ADDR		0x2D
#define ADPD_PULSE_OFFSET_A_ADDR	0x30
#define ADPD_PULSE_PERIOD_A_ADDR	0x31
#define ADPD_LED_DISABLE_ADDR		0x34
#define ADPD_PULSE_OFFSET_B_ADDR	0x35
#define ADPD_PULSE_PERIOD_B_ADDR	0x36
#define ADPD_AFE_CTRL_A_ADDR		0x39
#define ADPD_AFE_CTRL_B_ADDR		0x3B
#define ADPD_SLOTA_GAIN_ADDR		0x42
#define ADPD_SLOTA_AFE_CON_ADDR		0x43
#define ADPD_SLOTB_GAIN_ADDR		0x44
#define ADPD_SLOTB_AFE_CON_ADDR		0x45
#define ADPD_OSC32K_ADDR			0x4B
#define ADPD_TEST_PD_ADDR			0x52
#define ADPD_EFUSE_CTRL_ADDR		0x57
#define ADPD_ACCESS_CTRL_ADDR		0x5F
#define ADPD_DATA_BUFFER_ADDR		0x60
#define ADPD_P2P_REG_ADDR			0x67
#define ADPD_RED_SLOPE_ADDR			0x71
#define ADPD_IR_SLOPE_ADDR			0x72
#define ADPD_RED_INTRCPT_ADDR		0x73
#define ADPD_IR_INTRCPT_ADDR		0x74

#define MAX_CONFIG_REG_CNT			128

#define I2C_RETRY_DELAY			5
#define I2C_RETRIES			0

#define MAX_BUFFER			128
#define GLOBAL_OP_MODE_OFF		0 /* standby */
#define GLOBAL_OP_MODE_IDLE		1 /* program */
#define GLOBAL_OP_MODE_GO			2 /* normal operation */

#define SET_GLOBAL_OP_MODE(val, cmd)	((val & 0xFC) | cmd)

#define EN_FIFO_CLK			0x3B5B
#define DS_FIFO_CLK			0x3050
#define DISABLE_NONE			0x0
#define DISABLE_SLOTA_LED		BIT(8) /* RED */
#define DISABLE_SLOTB_LED		BIT(9) /* IR */
#define LED_SCALE_100			0x3030 /* 100% strength */


/* USR MODE */
#define IDLE_MODE				0x00
#define SAMPLE_XY_A_MODE		0x30 /* RED */
#define SAMPLE_XY_AB_MODE		0x31 /* RED/IR */
#define SAMPLE_XY_B_MODE		0x32 /* IR */
#define SAMPLE_TIA_ADC_MODE	0x51 /* Ambient rejection off */
#define SAMPLE_EOL_MODE		0x71
#define GET_USR_MODE(usr_val)	((usr_val & 0xF0) >> 4)
#define GET_USR_SUB_MODE(usr_val)	(usr_val & 0xF)

/* OPERATING MODE & DATA MODE */
#define R_RDATA_FLAG_EN		(0x1 << 15)
#define R_PACK_START_EN		(0x0 << 14)
#define	R_RDOUT_MODE			(0x0 << 13)
#define R_FIFO_PREVENT_EN		(0x1 << 12)
#define	R_SAMP_OUT_MODE		(0x0 << 9)
#define DEFAULT_OP_MODE_CFG_VAL(cfg_x)  (R_RDATA_FLAG_EN + R_PACK_START_EN +\
											R_RDOUT_MODE + R_FIFO_PREVENT_EN+\
											R_SAMP_OUT_MODE)

#define SLOTA_EN					1
#define SLOTA_FIFO_CONFIG			4
#define SLOTB_EN					1
#define SLOTB_FIFO_CONFIG			4

/* INTERRUPT MASK */
#define INTR_MASK_IDLE_USR			0x0000
#define INTR_MASK_SAMPLE_USR			0x0020
#define INTR_MASK_S_SAMP_XY_A_USR	0x0020
#define INTR_MASK_S_SAMP_XY_AB_USR	0x0040
#define	INTR_MASK_S_SAMP_XY_B_USR	0x0040
#define SET_INTR_MASK(usr_val)		((~(INTR_MASK_##usr_val)) & 0x1FF)


/* ADPD143 DIFFERENT MODE */
/* Main mode */
#define IDLE_USR			0
#define SAMPLE_USR			3
#define TIA_ADC_USR			5
#define EOL_USR				7
/* Sub mode */
#define S_SAMP_XY_A			0
#define S_SAMP_XY_AB			1
#define S_SAMP_XY_B			2
/*
OPERATING MODE ==>      MAIN_mode[20:16] |
			OP_mode_A[15:12] |
			DATA_mode_A[11:8]|
			OP_mode_B[7:4]   |
			DATA_mode_B[3:0]
*/
#define IDLE_OFF			((IDLE_USR << 16) | (0x0000))

#define SAMPLE_XY_A		((SLOTA_EN) | (SLOTA_FIFO_CONFIG << 2))
#define SAMPLE_XY_AB		((SLOTA_EN) | (SLOTA_FIFO_CONFIG << 2) | (SLOTB_EN << 5) | (SLOTB_FIFO_CONFIG << 6))
#define SAMPLE_XY_B		((SLOTB_EN << 5) | (SLOTB_FIFO_CONFIG << 6))

#define MAX_MODE                        8

#define MAX_IDLE_MODE			1
#define MAX_SAMPLE_MODE		3

#define MAX_LIB_VER			20
#define OSC_TRIM_ADDR26_REG		0x26
#define OSC_TRIM_ADDR28_REG		0x28
#define OSC_TRIM_ADDR29_REG		0x29
#define OSC_TRIM_32K_REG			0x4B
#define OSC_TRIM_32M_REG			0x4D
#define OSC_DEFAULT_32K_SET		0x2695
#define OSC_DEFAULT_32M_SET		0x4272
#define OSCILLATOR_TRIM_FILE_PATH	"/efs/FactoryApp/hrm_osc_trim"

#define FLOAT_MODE_SATURATION_CONTROL_CODE			8500
#define TIA_ADC_MODE_DARK_CALIBRATION_CNT		6
#define FLOAT_MODE_DARK_CALIBRATION_CNT			6

#define SF_RED_CH	100
#define SF_IR_CH	100
#define ADPD_READ_REGFILE_PATH "/data/HRM/ADPD_READ_REG.txt"
#define ADPD_WRITE_REGFILE_PATH "/data/HRM/ADPD_WRITE_REG.txt"

struct adpd_platform_data {
	unsigned short config_size;
	unsigned int config_data[MAX_CONFIG_REG_CNT];
};


enum {
	DEBUG_WRITE_REG_TO_FILE = 1,
	DEBUG_WRITE_FILE_TO_REG = 2,
	DEBUG_SHOW_DEBUG_INFO = 3,
	DEBUG_ENABLE_AGC = 4,
	DEBUG_DISABLE_AGC = 5,
};

/* AGC mode */
enum {
	M_HRM, // finger detection, IR&Red No Saturation
	M_HRMLED_IR, //IR Only No Saturation
	M_HRMLED_RED, //RED Only No Saturation
	M_HRMLED_BOTH, //IR&Red No Saturation
	M_NONE
};

/* MAX == 262140 */

#if defined(__USE_PD1234__) || defined(__USE_PD123__)
#define ADPD_THRESHOLD_DEFAULT 100000
#elif defined(__USE_PD234__)
#define ADPD_THRESHOLD_DEFAULT 80000
#elif defined(__USE_PD34__)
#define ADPD_THRESHOLD_DEFAULT 50000
#elif defined(__USE_PD4__)
#define ADPD_THRESHOLD_DEFAULT 20000
#endif

static u8 agc_is_enabled = 1;
static u8 fifo_is_32bit;

struct mode_mapping {
	char *name;
	int *mode_code;
	unsigned int size;
};
int _mode_IDLE[MAX_IDLE_MODE] = { IDLE_OFF };
int _mode_SAMPLE[MAX_SAMPLE_MODE] = { SAMPLE_XY_A, SAMPLE_XY_AB, SAMPLE_XY_B };

struct mode_mapping __mode_recv_frm_usr[MAX_MODE] = {
	{
		.name = "IDLE",
		.mode_code = _mode_IDLE,
		.size = MAX_IDLE_MODE
	},
	{	.name = "UNSUPPORTED1",
		.mode_code = 0,
		.size = 0,
	},
	{	.name = "UNSUPPORTED2",
		.mode_code = 0,
		.size = 0,
	},
	{
		.name = "SAMPLE",
		.mode_code = _mode_SAMPLE,
		.size = MAX_SAMPLE_MODE
	},
	{	.name = "UNSUPPORTED3",
		.mode_code = 0,
		.size = 0,
	},
	{
		.name = "TIA_ADC_SAMPLE",
		.mode_code = _mode_SAMPLE,
		.size = MAX_SAMPLE_MODE
	},
	{	.name = "UNSUPPORTED4",
		.mode_code = 0,
		.size = 0,
	},
	{
		.name = "EOL_SAMPLE",
		.mode_code = _mode_SAMPLE,
		.size = MAX_SAMPLE_MODE
	}
};

/**
general data buffer that is required to store the recieved buffer
*/
static unsigned short data_buffer[MAX_BUFFER] = { 0 };

struct adpd_stats {
	atomic_t fifo_requires_sync;
	atomic_t fifo_bytes[4];
};

typedef enum __EOL_state{
	ST_EOL_IDLE = 0,

	ST_EOL_CLOCK_CAL_INIT,
	ST_EOL_CLOCK_CAL_RUNNING,
	ST_EOL_CLOCK_CAL_DONE,

	ST_EOL_LOW_DC_INIT,
	ST_EOL_LOW_DC_RUNNING,
	ST_EOL_LOW_DC_DONE,

	ST_EOL_MED_DC_INIT,
	ST_EOL_MED_DC_RUNNING,
	ST_EOL_MED_DC_DONE,

	ST_EOL_HIGH_DC_INIT,
	ST_EOL_HIGH_DC_RUNNING,
	ST_EOL_HIGH_DC_DONE,

	ST_EOL_AC_INIT,
	ST_EOL_AC_RUNNING,
	ST_EOL_AC_DONE,

#ifdef MELANIN_IDX_SUPPORT
	ST_EOL_MELANIN_DC_INIT,
	ST_EOL_MELANIN_DC_RUNNING,
	ST_EOL_MELANIN_DC_DONE,
#endif

	ST_EOL_DONE,
} EOL_state;

unsigned short g_eol_regNumbers[32] = {
	0x1,
	0x2,
	0x6,
	0x11,
	0x12,
	0x14,
	0x15,
	0x18,
	0x19,
	0x1a,
	0x1b,
	0x1e,
	0x1f,
	0x20,
	0x21,
	0x23,
	0x24,
	0x25,
	0x27,
	0x30,
	0x31,
	0x34,
	0x35,
	0x36,
	0x39,
	0x3b,
	0x42,
	0x43,
	0x44,
	0x45,
	0x4b,
	0x52
};

#define EOL_ODR_TARGET				200
#define EOL_ALLOWABLE_ODR_ERROR		4

struct timeval prev_interrupt_trigger_ts;
struct timeval curr_interrupt_trigger_ts;

typedef enum __OBJ_PROXIMITY_STATUS__{
	OBJ_DARKOFFSET_CAL = -1,
	OBJ_DETACHED = 0,
	OBJ_DETECTED,
	OBJ_PROXIMITY_RED_ON,
	OBJ_PROXIMITY_ON,
	OBJ_HARD_PRESSURE,
	DEVICE_ON_TABLE,
} OBJ_PROXIMITY_STATUS;

#define OBJ_DETACH_FROM_DETECTION_DC_PERCENT      	80
#define OBJ_DETACH_FROM_PROXIMITY_DC_PERCENT      	70
#define OBJ_DETECTION_CNT_DC_PERCENT    			90
#define OBJ_PROXIMITY_CNT_DC_PERCENT     			80
#define CNT_THRESHOLD_OBJ_PROXIMITY_ON   			5
#define CNT_THRESHOLD_AGC_STEP_ON   				1
static unsigned cntThreshAgcStepOn = CNT_THRESHOLD_AGC_STEP_ON;

#define CNT_THRESHOLD_DC_SATURATION					10
#define CNT_ENABLE_SKIP_SAMPLE   					5

#define __USE_EOL_US_INT_SPACE__
#define __USE_EOL_ADC_OFFSET__
#define CNT_SAMPLE_PER_EOL_CYCLE   	50
#define CNT_EOL_SKIP_SAMPLE   		5

unsigned short g_org_regValues[33];
unsigned g_eol_ADCOffset[8];
unsigned g_dc_buffA[CNT_SAMPLE_PER_EOL_CYCLE];
unsigned g_dc_buffB[CNT_SAMPLE_PER_EOL_CYCLE];
unsigned g_ac_buffA[CNT_SAMPLE_PER_EOL_CYCLE];
unsigned g_ac_buffB[CNT_SAMPLE_PER_EOL_CYCLE];

/**
structure hold adpd143 chip structure, and parameter used
to control the adpd143 software part.
*/
struct adpd_device_data {
	struct i2c_client *client;
	struct mutex mutex;/*for chip structure*/

	unsigned short *ptr_data_buffer;

	struct adpd_stats stats;

	unsigned short intr_mask_val;
	unsigned short intr_status_val;
	unsigned short fifo_size;

	atomic_t adpd_mode;

	/*Efuse read for DC normalization */
	unsigned short efuseIrSlope;
	unsigned short efuseRedSlope;
	unsigned short efuseIrIntrcpt;
	unsigned short efuseRedIntrcpt;
	unsigned short efuse32KfreqOffset;

	OBJ_PROXIMITY_STATUS st_obj_proximity;
	unsigned obj_proximity_theshld;
	unsigned cur_proximity_dc_level;
	unsigned cur_proximity_red_dc_level;
	unsigned char cnt_enable;
	unsigned char cnt_proximity_on;
	unsigned char cnt_slotA_dc_saturation;
	unsigned char cnt_slotB_dc_saturation;
	unsigned char cnt_proximity_buf;
	unsigned char begin_proximity_buf;
	unsigned char Is_Hrm_Dark_Calibrated;

	int (*read)(struct adpd_device_data *, u8 reg_addr, int len, u16 *buf);
	int (*write)(struct adpd_device_data *, u8 reg_addr, int len, u16 data);
	u8 eol_test_is_enable;
	u8 eol_test_status;

	u8 osc_trim_open_enable;

	int hrm_mode;
#ifdef __USE_ADPD_32BIT_FIFO__
	unsigned long sum_slot_a;
	unsigned long sum_slot_b;
#ifdef __USE_LOWPASS_FIR_FILTER__
	unsigned long fltr_sum_slot_a;
	unsigned long fltr_sum_slot_b;
#endif
#else
	unsigned int sum_slot_a;
	unsigned int sum_slot_b;
#ifdef __USE_LOWPASS_FIR_FILTER__
	unsigned fltr_sum_slot_a;
	unsigned fltr_sum_slot_b;
#endif
#endif
	unsigned int slot_data[8];
	unsigned agc_threshold;
	u8 agc_mode;

	struct class *adpd_class;
	struct device *adpd_dev;

	/* for HRM mode */
	EOL_state eol_state;
	unsigned char eol_counter;
	int msec_eol_int_space;
	int usec_eol_int_space;
	long eol_measured_period;

	unsigned char eol_res_odr;
	unsigned short oscRegValue;

	unsigned eol_res_ir_high_dc;
	unsigned eol_res_ir_med_dc;
	unsigned eol_res_ir_low_dc;
	unsigned short eol_res_ir_low_noise;
	unsigned short eol_res_ir_med_noise;
	unsigned short eol_res_ir_high_noise;
	unsigned short eol_res_ir_ac;

	unsigned eol_res_red_high_dc;
	unsigned eol_res_red_med_dc;
	unsigned eol_res_red_low_dc;
	unsigned short eol_res_red_low_noise;
	unsigned short eol_res_red_med_noise;
	unsigned short eol_res_red_high_noise;
	unsigned short eol_res_red_ac;

#ifdef MELANIN_IDX_SUPPORT
	unsigned eol_res_melanin_red_dc;
	unsigned eol_res_melanin_ir_dc;
	unsigned eol_res_melanin_red_noise;
	unsigned eol_res_melanin_ir_noise;
#endif

	u8 led_current_1;
	u8 led_current_2;
	u64 device_id;

	struct mutex flickerdatalock;
	struct miscdevice miscdev;
	int *flicker_data;
	int flicker_data_cnt;

	u32 i2c_err_cnt;
	u16 ir_curr;
	u16 red_curr;
	u32 ir_adc;
	u32 red_adc;
};

#define FLICKER_DATA_CNT	56
#define ADPD143_IOCTL_MAGIC		0xFD
#define ADPD143_IOCTL_READ_FLICKER	_IOR(ADPD143_IOCTL_MAGIC, 0x01, int *)


static struct adpd_device_data *adpd_data;

typedef struct _adpd_eol_result_{
unsigned char eol_res_odr;

unsigned eol_res_red_low_dc;
unsigned eol_res_ir_low_dc;
unsigned short eol_res_red_low_noise;
unsigned short eol_res_ir_low_noise;

unsigned eol_res_red_med_dc;
unsigned eol_res_ir_med_dc;
unsigned short eol_res_red_med_noise;
unsigned short eol_res_ir_med_noise;

unsigned eol_res_red_high_dc;
unsigned eol_res_ir_high_dc;
unsigned short eol_res_red_high_noise;
unsigned short eol_res_ir_high_noise;

unsigned short eol_res_red_ac;
unsigned short eol_res_ir_ac;

#ifdef MELANIN_IDX_SUPPORT
unsigned eol_res_melanin_red_dc;
unsigned eol_res_melanin_ir_dc;
#endif
} adpd_eol_result;


typedef union __ADPD_EOL_RESULT__{
	adpd_eol_result st_eol_res;
	char buf[49];
} ADPD_EOL_RESULT;

/**
structure hold adpd143 configuration data to be written during probe.
*/
static struct adpd_platform_data adpd_config_data = {
	.config_size = 42,
	.config_data = {
		0x00100001,
		0x000100ff,
		0x00020005,
		0x00060000,
		0x00111131,
		0x0012000a,
		0x00130320,
		0x00140449,
		0x00150330,
		0x00168080,
		0x00178080,
		0x00182100,
		0x00192100,
		0x001A2000,
		0x001B2000,
		0x001C8080,
		0x001D8080,
		0x001E2100,
		0x001F2100,
		0x00202000,
		0x00212000,
		0x00223030,
		0x00233035,
		0x0024303f,
		0x00250000,
		0x00300330,
		0x00310813,
		0x00340300,
		0x00350330,
		0x00360813,
		0x003924D4,
		0x003B24D4,
		0x003C0006,
		0x00421C37,
		0x0043ADA5,
		0x00441C37,
		0x0045ADA5,
		0x004B2694,
		0x004e0040,
		0x00520040,
		0x00540020,
		0x00580000,
	},
};

static struct adpd_platform_data adpd_tia_adc_config_data = {
	.config_size = 68,
	.config_data = {
		0x00100001,
		0x000080ff,
		0x000101ff,
		0x00020005,
		0x00063600,
		0x00111131,
		0x00120020,
		0x00130320,
		0x00140000,
		0x00150000,
		0x00168080,
		0x00178080,
		0x00180000,
		0x00190000,
		0x001a0000,
		0x001b0000,
		0x001c8080,
		0x001d8080,
		0x001e1f40,
		0x001f1f40,
		0x00201f40,
		0x00211f40,
		0x00231030,
		0x00241030,
		0x00250000,
		0x00260000,
		0x00270800,
		0x00280000,
		0x00294003,
		0x00300220,
		0x00310420,
		0x00320320,
		0x00330113,
		0x00340100,
		0x00350220,
		0x00360260,
		0x00370000,
		0x003924d2,
		0x003a22d4,
		0x003b1a70,
		0x003c3206,
		0x003d0000,
		0x00401010,
		0x0041004c,
		0x00421c34,
		0x0043b065,
		0x00441c16,
		0x0045ae65,
		0x00460000,
		0x00470080,
		0x00480000,
		0x00490000,
		0x004a0000,
		0x004b2694,
		0x004c0004,
		0x004d4272,
		0x004e0040,
		0x004f2090,
		0x00500000,
		0x00523000,
		0x0053e400,
		0x0054002a,
		0x00580080,
		0x00597f08,
		0x005a0000,
		0x005b0000,
		0x005d0000,
		0x005e0808,
	},
};

/* for TIA_ADC runtime Dark Calibration */
static unsigned short Is_TIA_ADC_Dark_Calibrated;
static unsigned short Is_Float_Dark_Calibrated;
static unsigned rawDarkCh[8];
static unsigned rawDataCh[8];

#define CH1_APPROXIMATE_FLOAT_SATURATION_VALUE_IN_TIA_ADC   (1800*5/4)
#define CH2_APPROXIMATE_FLOAT_SATURATION_VALUE_IN_TIA_ADC   (2400*5/4)
#define CH3_APPROXIMATE_FLOAT_SATURATION_VALUE_IN_TIA_ADC   (3300*5/4)
#define CH4_APPROXIMATE_FLOAT_SATURATION_VALUE_IN_TIA_ADC   (6700*5/4)

static unsigned ch1FloatSat = CH1_APPROXIMATE_FLOAT_SATURATION_VALUE_IN_TIA_ADC;
static unsigned ch2FloatSat = CH2_APPROXIMATE_FLOAT_SATURATION_VALUE_IN_TIA_ADC;
static unsigned ch3FloatSat = CH3_APPROXIMATE_FLOAT_SATURATION_VALUE_IN_TIA_ADC;
static unsigned ch4FloatSat = CH4_APPROXIMATE_FLOAT_SATURATION_VALUE_IN_TIA_ADC;

static int __adpd_set_current(u8 d1, u8 d2, u8 d3, u8 d4);

static int __adpd_i2c_reg_read(struct adpd_device_data *adpd_data, u8 reg_addr, int len, u16 *buf)
{
	int err;
	int tries = 0;
	int icnt = 0;

	struct i2c_msg msgs[] = {
		{
			.addr = adpd_data->client->addr,
			.flags = adpd_data->client->flags & I2C_M_TEN,
			.len = 1,
			.buf = (s8 *)&reg_addr,
		},
		{
			.addr = adpd_data->client->addr,
			.flags = (adpd_data->client->flags & I2C_M_TEN) | I2C_M_RD,
			.len = len * sizeof(unsigned short),
			.buf = (s8 *)buf,
		},
	};

	do {
		err = i2c_transfer(adpd_data->client->adapter, msgs, 2);

		if (err < 0)
			HRM_dbg("%s - i2c_transfer error = %d\n", __func__, err);
	} while ((err != 2) && (++tries < I2C_RETRIES));

	if (err != 2) {
		HRM_dbg("%s - read transfer error\n", __func__);
		err = -EIO;
		adpd_data->i2c_err_cnt++;
	} else {
		err = 0;
	}

	for (icnt = 0; icnt < len; icnt++) {
		/*convert big endian to CPU format*/
		buf[icnt] = be16_to_cpu(buf[icnt]);
	}

	return err;
}

static int __adpd_i2c_reg_write(struct adpd_device_data *adpd_data, u8 reg_addr, int len,
		   u16 data)
{
	struct i2c_msg msgs[1];
	int err;
	int tries = 0;
	unsigned short data_to_write = cpu_to_be16(data);
	char buf[4];

	buf[0] = (s8) reg_addr;
	memcpy(buf + 1, &data_to_write, sizeof(unsigned short));
	msgs[0].addr = adpd_data->client->addr;
	msgs[0].flags = adpd_data->client->flags & I2C_M_TEN;
	msgs[0].len = 1 + (1 * sizeof(unsigned short));
	msgs[0].buf = buf;

	do {
		err = i2c_transfer(adpd_data->client->adapter, msgs, 1);
		if (err != 1)
			msleep_interruptible(I2C_RETRY_DELAY);
		if (err < 0)
			HRM_dbg("%s - i2c_transfer error = %d\n", __func__, err);
	} while ((err != 1) && (++tries < I2C_RETRIES));

	if (err != 1) {
		dev_err(&adpd_data->client->dev, "write transfer error\n");
		err = -EIO;
		adpd_data->i2c_err_cnt++;
	} else {
		err = 0;
	}

	return err;
}

static inline u16 __adpd_reg_read(struct adpd_device_data *adpd_data, u16 reg)
{
	u16 value;
	if (!adpd_data->read(adpd_data, reg, 1, &value))
		return value;
	return 0xFFFF;
}

static inline u16 __adpd_multi_reg_read(struct adpd_device_data *adpd_data, u16 reg, u16 len, u16 *buf)
{
	return adpd_data->read(adpd_data, reg, len, buf);
}

static inline u16 __adpd_reg_write(struct adpd_device_data *adpd_data, u16 reg, u16 value)
{
	return adpd_data->write(adpd_data, reg, 1, value);
}


static unsigned int __adpd_normalized_data(struct adpd_device_data *adpd_data, unsigned int ir_red, unsigned int raw_data)
{
	unsigned long long normal_data;

	if (ir_red) {
		normal_data = (unsigned long long)raw_data * 256 * SF_IR_CH / (adpd_data->efuseIrSlope + 128) / 100;

		HRM_info("%s - efuseIrSlope = 0x%x, rawdata = %d, normal_data = %lld\n", __func__, adpd_data->efuseIrSlope, raw_data, normal_data);
	} else {
		normal_data = (unsigned long long)raw_data * 256 * SF_RED_CH / (adpd_data->efuseRedSlope + 128) / 100;

		HRM_info("%s - efuseRedSlope = 0x%x, rawdata = %d, normal_data = %lld\n", __func__, adpd_data->efuseRedSlope, raw_data, normal_data);
	}

	return (unsigned int)normal_data;
}

#define MAX_LED 0
#define MAX_TIA 1

#define POWER_PREF MAX_LED

typedef enum {
	MwErrPass = 0x0000,
	MwErrFail,
	MwErrProcessing,

	/* LED Error Code */
	MwErrLedAOutofRange = 0x0100,
	MwErrLedATrimOutofRange = 0x0200,
	MwErrLedBOutofRange = 0x0400,
	MwErrLedBTrimOutofRange = 0x0800,
} AGC_ErrCode_t;

typedef enum {
	AGC_Slot_A = 0x01,
	AGC_Slot_B = 0x02,
} AGC_mode_t;


#define DOC_INIT_VALUE      0x2000

static unsigned maxPulseCodeACh1;
static unsigned maxPulseCodeACh2;
static unsigned maxPulseCodeACh3;
static unsigned maxPulseCodeACh4;
static unsigned maxPulseCodeBCh1;
static unsigned maxPulseCodeBCh2;
static unsigned maxPulseCodeBCh3;
static unsigned maxPulseCodeBCh4;

/*#define CH_RAW_32BIT_DATA_SIZE */
#define MAX_CODE_AFTER_ADC_OFFSET_PER_PULSE  0x2000
#define MAX_CODE_16BIT  0xffff
#define MAX_CODE_32BIT  0xffffffff
#define MAX_CODE_PERCENTAGE_A  90
#define MAX_CODE_PERCENTAGE_B  90
#define AGC_STOP_MARGIN_PERCENTAGE_A 10
#define AGC_STOP_MARGIN_PERCENTAGE_B 10

/* LED driver current - 25~250mA */
#define POWER_PREF_MAX_LED_DRV_CURRENT_A 250
#define POWER_PREF_MIN_LED_DRV_CURRENT_A 25
#define POWER_PREF_MAX_LED_DRV_CURRENT_B 250
#define POWER_PREF_MIN_LED_DRV_CURRENT_B 25
 /* if prefMinPower = 0 */
#define PERF_PREF_MAX_LED_DRV_CURRENT_A 250
#define PERF_PREF_MIN_LED_DRV_CURRENT_A 40
#define PERF_PREF_MAX_LED_DRV_CURRENT_B 250
#define PERF_PREF_MIN_LED_DRV_CURRENT_B 40

/* TIA gain - 200k : x8, 100k :x 4, 50k : x2, 25k : x1 */
#define MAX_TIA_GAIN_A        8
#define MIN_TIA_GAIN_A        1
#define MAX_TIA_GAIN_B        8
#define MIN_TIA_GAIN_B        1
/* AFE gain - x1~x4 */
#define MAX_AFE_GAIN_A        4
#define MIN_AFE_GAIN_A        1
#define MAX_AFE_GAIN_B        4
#define MIN_AFE_GAIN_B        1
/* # of samples to skip */
#define NO_OF_SAMPLES_TO_SKIP_IN_AGC  0

/* Public function prototypes -----------------------------------------------*/
AGC_ErrCode_t __adpd_initAGC(unsigned char  param_prefMinPower, AGC_mode_t agc_mode);
/* AGC_ErrCode_t confAGC(); */
AGC_ErrCode_t __adpd_stepAGC(unsigned *rawDataA, unsigned *rawDataB);
/* AGC_ErrCode_t runAGC(); */

/* Private function prototypes ----------------------------------------------*/
static AGC_ErrCode_t __adpd_stepAgcLedA(unsigned *rawData);
static AGC_ErrCode_t __adpd_stepAgcLedB(unsigned *rawData);

/* static variable related with AGC */
static unsigned char RegLedTrimBitPosA, RegLedTrimBitPosB;
static unsigned RegAddrLedDrvA, RegAddrLedDrvB;
/* static unsigned RegAddrAfeTiaA, RegAddrAfeTiaB; */
static unsigned CurrLedDrvValA, CurrLedDrvValB;
/* static unsigned CurrLedTrimValA, CurrLedTrimValB; */
static unsigned CurrLedScaleA, CurrLedScaleB;
static unsigned CurrLedCoarseA, CurrLedCoarseB;
static unsigned CurrTIAgainA, CurrTIAgainB;
static unsigned CurrAFEgainA, CurrAFEgainB;

static unsigned PrevLedScaleA, PrevLedScaleB;
static unsigned PrevLedCoarseA, PrevLedCoarseB;
static unsigned PrevTIAgainA, PrevTIAgainB;
static unsigned PrevAFEgainA, PrevAFEgainB;
static unsigned char cntStepAgcA, cntStepAgcB;
static unsigned prevStepDiffAgcA, prevStepDiffAgcB;

static unsigned TargetDcLevelA, TargetDcLevelB;
static unsigned char  pulseA, pulseB;
static unsigned char  sumA, sumB;

static unsigned short maxLedCurrentRegValA;
static unsigned short minLedCurrentRegValA;
static unsigned short maxTiaGainRegValA;
static unsigned short minTiaGainRegValA;
static unsigned short maxAfeGainRegValA;
static unsigned short minAfeGainRegValA;
static unsigned short maxLedCurrentRegValB;
static unsigned short minLedCurrentRegValB;
static unsigned short maxTiaGainRegValB;
static unsigned short minTiaGainRegValB;
static unsigned short maxAfeGainRegValB;
static unsigned short minAfeGainRegValB;

static unsigned char  prefMinPower;
static unsigned char  NoSkipSamples; /* # of samples to skip */
static unsigned char  maxLEDcurrDrvA, minLEDcurrDrvA;
static unsigned char  maxLEDcurrDrvB, minLEDcurrDrvB;

typedef enum {
	AgcStage_LedA_Init = 0,
	AgcStage_LedA,
	AgcStage_LedA_Trim_Init,
	AgcStage_LedA_Trim,
	AgcStage_LedB_Init,
	AgcStage_LedB,
	AgcStage_LedB_Trim_Init,
	AgcStage_LedB_Trim,
	AgcStage_Done,
} AgcStage_t;

static AgcStage_t AgcStage;
static AGC_mode_t AgcMode;
AgcStage_t __adpd_getAGCstate(void)
{
	return AgcStage;
}

/* #define CH_RAW_32BIT_DATA_SIZE */
/* unsinged uAgcRawDataA[4]; */
/* unsinged uAgcRawDataB[4]; */

AGC_ErrCode_t __adpd_initAGC(unsigned char  param_prefMinPower, AGC_mode_t agc_mode)
{
#ifdef CH_RAW_32BIT_DATA_SIZE /* 20150730 ADI Joshua Yoon for Gear4 AGC */
	unsigned temp32;
#endif
	unsigned short temp16, slotA_led, slotB_led;
	unsigned reg_value, targetDcLevelA, targetDcLevelB;

	if (!adpd_data) {
		HRM_dbg("%s - ADPD should be initialized first before __adpd_initAGC()!!!!\r\n", __func__);
		return MwErrFail;
	}

	prefMinPower = param_prefMinPower;

    /* read reg14 */
	reg_value = __adpd_reg_read(adpd_data, ADPD_PD_SELECT_ADDR);
	slotA_led = reg_value & 0x03;
	slotB_led = (reg_value & 0x0C) >> 2;

	slotA_led = 1;
	slotB_led = 2;//Grace hardcode

	HRM_info("%s - slotA_led : %d, slotB_led : %d\n", __func__, slotA_led, slotB_led);
	/* Find the current LED allocation to each slotA/B */
	if (slotA_led == 1) {
		RegAddrLedDrvA = ADPD_LED1_DRV_ADDR;
		RegLedTrimBitPosA = 0;
		/* LedTrimMaskA = 0xFFC0; */
	} else if (slotA_led == 2) {
		RegAddrLedDrvA = ADPD_LED2_DRV_ADDR;
		RegLedTrimBitPosA = 6;
		/* LedTrimMaskA = 0xF81F; */
	} else if (slotA_led == 3) {
		RegAddrLedDrvA = ADPD_LED3_DRV_ADDR;
		RegLedTrimBitPosA = 11;
		/* LedTrimMaskA = 0x07DF; */
	} else {
		RegAddrLedDrvA = 0;
	}

	if (slotB_led == 1) {
		RegAddrLedDrvB = ADPD_LED1_DRV_ADDR;
		RegLedTrimBitPosB = 0;
		/* LedTrimMaskB = 0xFFC0; */
	} else if (slotB_led == 2) {
		RegAddrLedDrvB = ADPD_LED2_DRV_ADDR;
		RegLedTrimBitPosB = 6;
		/* LedTrimMaskB = 0xF81F; */
	} else if (slotB_led == 3) {
		RegAddrLedDrvB = ADPD_LED3_DRV_ADDR;
		RegLedTrimBitPosB = 11;
		/* LedTrimMaskB = 0x07DF; */
	} else {
		RegAddrLedDrvB = 0;
	}

	if (prefMinPower) {
		maxLEDcurrDrvA = POWER_PREF_MAX_LED_DRV_CURRENT_A;
		minLEDcurrDrvA = POWER_PREF_MIN_LED_DRV_CURRENT_A;
		maxLedCurrentRegValA = (POWER_PREF_MAX_LED_DRV_CURRENT_A-25)/15;
		minLedCurrentRegValA = (POWER_PREF_MIN_LED_DRV_CURRENT_A-25)/15;
		maxTiaGainRegValA = (MAX_TIA_GAIN_A >= 4) ? (8-MAX_TIA_GAIN_A)>>2 : (MAX_TIA_GAIN_A >= 2) ? MAX_TIA_GAIN_A : (8-MAX_TIA_GAIN_A)>>1;
		minTiaGainRegValA = (MIN_TIA_GAIN_A >= 4) ? (8-MIN_TIA_GAIN_A)>>2 : (MIN_TIA_GAIN_A >= 2) ? MIN_TIA_GAIN_A : (8-MIN_TIA_GAIN_A)>>1;
		maxAfeGainRegValA = (MAX_AFE_GAIN_A == 1) ? 0 : MAX_AFE_GAIN_A>>1;
		minAfeGainRegValA = (MIN_AFE_GAIN_A == 1) ? 0 : MIN_AFE_GAIN_A>>1;

		maxLEDcurrDrvB = POWER_PREF_MAX_LED_DRV_CURRENT_B;
		minLEDcurrDrvB = POWER_PREF_MIN_LED_DRV_CURRENT_B;
		maxLedCurrentRegValB = (POWER_PREF_MAX_LED_DRV_CURRENT_B-25) / 15;
		minLedCurrentRegValB = (POWER_PREF_MIN_LED_DRV_CURRENT_B-25) / 15;
		maxTiaGainRegValB = (MAX_TIA_GAIN_B >= 4) ? (8-MAX_TIA_GAIN_B)>>2 : (MAX_TIA_GAIN_B >= 2) ? MAX_TIA_GAIN_B : (8-MAX_TIA_GAIN_B)>>1;
		minTiaGainRegValB = (MIN_TIA_GAIN_B >= 4) ? (8-MIN_TIA_GAIN_B)>>2 : (MIN_TIA_GAIN_B >= 2) ? MIN_TIA_GAIN_B : (8-MIN_TIA_GAIN_B)>>1;
		maxAfeGainRegValB = (MAX_AFE_GAIN_B == 1) ? 0 : MAX_AFE_GAIN_B>>1;
		minAfeGainRegValB = (MIN_AFE_GAIN_B == 1) ? 0 : MIN_AFE_GAIN_B>>1;
	} else {
		maxLEDcurrDrvA = PERF_PREF_MAX_LED_DRV_CURRENT_A;
		minLEDcurrDrvA = PERF_PREF_MIN_LED_DRV_CURRENT_A;
		maxLedCurrentRegValA = (PERF_PREF_MAX_LED_DRV_CURRENT_A-25) / 15;
		minLedCurrentRegValA = (PERF_PREF_MIN_LED_DRV_CURRENT_A-25) / 15;
		maxTiaGainRegValA = (MAX_TIA_GAIN_A >= 4) ? (8-MAX_TIA_GAIN_A)>>2 : (MAX_TIA_GAIN_A >= 2) ? MAX_TIA_GAIN_A : (8-MAX_TIA_GAIN_A)>>1;
		minTiaGainRegValA = (MIN_TIA_GAIN_A >= 4) ? (8-MIN_TIA_GAIN_A)>>2 : (MIN_TIA_GAIN_A >= 2) ? MIN_TIA_GAIN_A : (8-MIN_TIA_GAIN_A)>>1;
		maxAfeGainRegValA = (MAX_AFE_GAIN_A == 1) ? 0 : MAX_AFE_GAIN_A>>1;
		minAfeGainRegValA = (MIN_AFE_GAIN_A == 1) ? 0 : MIN_AFE_GAIN_A>>1;

		maxLEDcurrDrvB = PERF_PREF_MAX_LED_DRV_CURRENT_B;
		minLEDcurrDrvB = PERF_PREF_MIN_LED_DRV_CURRENT_B;
		maxLedCurrentRegValB = (PERF_PREF_MAX_LED_DRV_CURRENT_B-25) / 15;
		minLedCurrentRegValB = (PERF_PREF_MIN_LED_DRV_CURRENT_B-25) / 15;
		maxTiaGainRegValB = (MAX_TIA_GAIN_B >= 4) ? (8-MAX_TIA_GAIN_B)>>2 : (MAX_TIA_GAIN_B >= 2) ? MAX_TIA_GAIN_B : (8-MAX_TIA_GAIN_B)>>1;
		minTiaGainRegValB = (MIN_TIA_GAIN_B >= 4) ? (8-MIN_TIA_GAIN_B)>>2 : (MIN_TIA_GAIN_B >= 2) ? MIN_TIA_GAIN_B : (8-MIN_TIA_GAIN_B)>>1;
		maxAfeGainRegValB = (MAX_AFE_GAIN_B == 1) ? 0 : MAX_AFE_GAIN_B>>1;
		minAfeGainRegValB = (MIN_AFE_GAIN_B == 1) ? 0 : MIN_AFE_GAIN_B>>1;
	}

	/* find number of pulses adpd_data */
	reg_value = __adpd_reg_read(adpd_data, ADPD_PULSE_PERIOD_A_ADDR);
	pulseA = (reg_value >> 8) & 0xFF;

	reg_value = __adpd_reg_read(adpd_data, ADPD_DEC_MODE_ADDR);
	sumA = (reg_value >> 4) & 0x7;

#ifndef CH_RAW_32BIT_DATA_SIZE /* for 16bit raw data */
	targetDcLevelA = MAX_CODE_AFTER_ADC_OFFSET_PER_PULSE * MAX_CODE_PERCENTAGE_A * pulseA / 100;
	temp16 = MAX_CODE_16BIT / 100 * MAX_CODE_PERCENTAGE_A;
	if (targetDcLevelA > temp16)
		targetDcLevelA = temp16;
#else /* for 32bit raw data */
	targetDcLevelA = MAX_CODE_AFTER_ADC_OFFSET_PER_PULSE * MAX_CODE_PERCENTAGE_A * pulseA / 100;
	targetDcLevelA <<= pulseA;
	temp32 = MAX_CODE_32BIT / 100 * MAX_CODE_PERCENTAGE_A;
	if (targetDcLevelA > temp32)
		targetDcLevelA = temp32;
#endif

	reg_value = __adpd_reg_read(adpd_data, ADPD_PULSE_PERIOD_B_ADDR);
	pulseB = (reg_value >> 8) & 0xFF;

	reg_value = __adpd_reg_read(adpd_data, ADPD_DEC_MODE_ADDR);
	sumB = (reg_value >> 8) & 0x7;

#ifndef CH_RAW_32BIT_DATA_SIZE /* for 16bit raw data */
	targetDcLevelB = MAX_CODE_AFTER_ADC_OFFSET_PER_PULSE*MAX_CODE_PERCENTAGE_B * pulseB / 100;
	temp16 = MAX_CODE_16BIT / 100 * MAX_CODE_PERCENTAGE_B;

	if (targetDcLevelB > temp16)
		targetDcLevelB = temp16;

#else /*for 32bit raw data */
	targetDcLevelB = MAX_CODE_AFTER_ADC_OFFSET_PER_PULSE*MAX_CODE_PERCENTAGE_B * pulseB / 100;
	targetDcLevelB <<= pulseB;
	temp32 = MAX_CODE_32BIT / 100 * MAX_CODE_PERCENTAGE_B;

	if (targetDcLevelB > temp32)
		targetDcLevelB = temp32;
#endif

	TargetDcLevelA = targetDcLevelA;
	TargetDcLevelB = targetDcLevelB;
	HRM_info("%s - LED PulseA = %d PulseB = %d\n", __func__, pulseA, pulseB);
	HRM_info("%s - LED TargetA = %d TargetB = %d\n", __func__, TargetDcLevelA, TargetDcLevelB);

	__adpd_reg_write(adpd_data, ADPD_OP_MODE_ADDR, 0x1);

	/* set slotB pusle count */
	/* regRead(ADPD_PULSE_PERIOD_B_ADDR, &reg_value); */
	/* pulseB=(reg_value>>8)&0xFF; */
	/* regWrite(ADPD_PULSE_PERIOD_B_ADDR, 0x1013); */

	if (prefMinPower) { /* if power preference, start from 100% LED scale,  min LED power & max TIA gain */
		reg_value = __adpd_reg_read(adpd_data, ADPD_SLOTA_GAIN_ADDR);
		__adpd_reg_write(adpd_data, ADPD_SLOTA_GAIN_ADDR, (reg_value & 0xfffc) | maxTiaGainRegValA);

		reg_value = __adpd_reg_read(adpd_data, ADPD_SLOTB_GAIN_ADDR);
		__adpd_reg_write(adpd_data, ADPD_SLOTB_GAIN_ADDR, (reg_value & 0xfffc) | maxTiaGainRegValB);
	} else { /* if no power preference, start from 100% LED scale, max LED power & min TIA gain */
		reg_value = __adpd_reg_read(adpd_data, ADPD_SLOTA_GAIN_ADDR);
		__adpd_reg_write(adpd_data, ADPD_SLOTA_GAIN_ADDR, (reg_value & 0xfffc) | minTiaGainRegValA);

		reg_value = __adpd_reg_read(adpd_data, ADPD_SLOTB_GAIN_ADDR);
		__adpd_reg_write(adpd_data, ADPD_SLOTB_GAIN_ADDR, (reg_value & 0xfffc) | minTiaGainRegValB);
	}

	__adpd_reg_write(adpd_data, ADPD_OP_MODE_ADDR, 0x2);

	NoSkipSamples = 0;
	cntStepAgcA = cntStepAgcB = 0;
	prevStepDiffAgcA = prevStepDiffAgcB = 0xffffffff;

	AgcMode = agc_mode;
	if (AgcMode & AGC_Slot_A  && AgcMode & AGC_Slot_B) {
		if (RegAddrLedDrvA != 0)
			AgcStage = AgcStage_LedA;
		else if (RegAddrLedDrvB != 0)
			AgcStage = AgcStage_LedB;
		else {
			AgcStage = AgcStage_Done;
			return MwErrFail;
		}
	} else if (AgcMode & AGC_Slot_A) {
		if (RegAddrLedDrvA != 0)
			AgcStage = AgcStage_LedA;
		else {
			AgcStage = AgcStage_Done;
			return MwErrFail;
		}
	} else if (AgcMode & AGC_Slot_B) {
		if (RegAddrLedDrvB != 0)
			AgcStage = AgcStage_LedB;
		else {
			AgcStage = AgcStage_Done;
			return MwErrFail;
		}
	}

	return MwErrPass;
}

AGC_ErrCode_t __adpd_stepAGC(unsigned *rawDataA, unsigned *rawDataB)
{
	unsigned char retVal = 0;

	if (NoSkipSamples++ < NO_OF_SAMPLES_TO_SKIP_IN_AGC)
		return MwErrProcessing;

	__adpd_reg_write(adpd_data, ADPD_OP_MODE_ADDR, 0x1);

	if (AgcStage == AgcStage_LedA) {
		if (rawDataA != NULL) {
			retVal = __adpd_stepAgcLedA(rawDataA);
			HRM_info("%s - __adpd_stepAgcLedA = 0x%x\n", __func__, retVal);

			if (retVal == MwErrLedAOutofRange || retVal == MwErrPass) {
				if (RegAddrLedDrvB != 0 && AgcMode & AGC_Slot_B)
					AgcStage = AgcStage_LedB;
				else
					AgcStage = AgcStage_Done;
			}
		} else
			AgcStage = AgcStage_LedB;
	}

	HRM_info("%s - AgcStage = %d\n", __func__, AgcStage);

	if (AgcStage == AgcStage_LedB) {
		if (rawDataB != NULL) {
			retVal = __adpd_stepAgcLedB(rawDataB);
			HRM_info("%s - __adpd_stepAgcLedB = 0x%x\n", __func__, retVal);

			if (retVal == MwErrLedAOutofRange || retVal == MwErrPass)
				AgcStage = AgcStage_Done;
		} else
			AgcStage = AgcStage_Done;
	}

	HRM_info("%s - AgcStage = %d\n", __func__, AgcStage);

	__adpd_reg_write(adpd_data, ADPD_OP_MODE_ADDR, 0x2);

	return retVal;
}

AGC_ErrCode_t __adpd_stepAGC2(unsigned *rawDataA, unsigned *rawDataB)
{
	unsigned char retVal = 0;

	if (AgcStage == AgcStage_LedA) {
		if (rawDataA != NULL) {
			retVal = __adpd_stepAgcLedA(rawDataA);
			HRM_info("%s - __adpd_stepAgcLedA = 0x%x\n", __func__, retVal);

			if (retVal == MwErrLedAOutofRange || retVal == MwErrPass) {
				if (RegAddrLedDrvB != 0 && AgcMode & AGC_Slot_B)
					AgcStage = AgcStage_LedB;
				else
					AgcStage = AgcStage_Done;
			}
		} else
			AgcStage = AgcStage_LedB;
	}

	HRM_info("%s - AgcStage = %d\n", __func__, AgcStage);

	if (AgcStage == AgcStage_LedB) {
		if (rawDataB != NULL) {
			retVal = __adpd_stepAgcLedB(rawDataB);
			HRM_info("%s - __adpd_stepAgcLedB = 0x%x\n", __func__, retVal);

			if (retVal == MwErrLedAOutofRange || retVal == MwErrPass)
				AgcStage = AgcStage_Done;
		} else
			AgcStage = AgcStage_Done;
	}

	HRM_info("%s - AgcStage = %d\n", __func__, AgcStage);

	return retVal;
}


static AGC_ErrCode_t __adpd_stepAgcLedA(unsigned *rawData)
{
	unsigned raw_max, diff;
	unsigned char toInc = 0;
	unsigned reg_val;
	unsigned short currLedCoarseCurrent;
	unsigned short tgtLedCoarseCurrent;

	if (RegAddrLedDrvA != 0) {
		CurrLedDrvValA = __adpd_reg_read(adpd_data, RegAddrLedDrvA);
		CurrLedScaleA = (CurrLedDrvValA & 0x2000) >> 12;
		CurrLedCoarseA = CurrLedDrvValA & 0x000f;
	}

	reg_val = __adpd_reg_read(adpd_data, ADPD_SLOTA_GAIN_ADDR);

	CurrTIAgainA = reg_val & 0x3;
	CurrAFEgainA = (reg_val & 0x0300) >> 8;

	reg_val = __adpd_reg_read(adpd_data, ADPD_PULSE_PERIOD_A_ADDR);
	pulseA = (reg_val >> 8) & 0xFF;

	reg_val = __adpd_reg_read(adpd_data, ADPD_PULSE_PERIOD_B_ADDR);
	pulseB = (reg_val >> 8) & 0xFF;

#if defined(__USE_PD1234__) || defined(__USE_PD123__)
	raw_max = rawData[0]; //__adpd_maxValueAGC(rawData, MAX_NUM_SAMPLE);
#elif defined(__USE_PD234__)
	raw_max = rawData[1];//__adpd_maxValueAGC(rawData, MAX_NUM_SAMPLE-1);
#elif defined(__USE_PD34__)
	raw_max = rawData[2];//__adpd_maxValueAGC(rawData, MAX_NUM_SAMPLE-2);
#elif defined(__USE_PD4__)
	raw_max = rawData[3];//__adpd_maxValueAGC(rawData, MAX_NUM_SAMPLE-3);
#endif

	HRM_info("%s - [start : %u]\n", __func__, cntStepAgcA++);
	HRM_info("%s - maxLedCurrentRegValA : %u minLedCurrentRegValA : %u maxTiaGainRegValA : %u minTiaGainRegValA : %u\n",
				__func__, maxLedCurrentRegValA, minLedCurrentRegValA, maxTiaGainRegValA, minTiaGainRegValA);
	HRM_info("%s - LED PulseA = %d PulseB = %d\n", __func__, pulseA, pulseB);
	HRM_info("%s - current sample : %d target : %d\n", __func__, raw_max, TargetDcLevelA);

	if (raw_max > TargetDcLevelA) {
		diff = raw_max - TargetDcLevelA;
		toInc = 0;
	} else {
		diff = TargetDcLevelA - raw_max;
		toInc = 1;
	}

	HRM_info("%s - CurrLedScaleA : %d,  CurrLedCoarseA : %d, CurrTIAgainA : %d, CurrAFEgainA : %d\n",
		__func__, CurrLedScaleA, CurrLedCoarseA, CurrTIAgainA, CurrAFEgainA);
	HRM_info("%s - PrevLedScaleA : %d,  PrevLedCoarseA : %d, PrevTIAgainA : %d, PrevAFEgainA : %d\n",
		__func__, PrevLedScaleA, PrevLedCoarseA, PrevTIAgainA, PrevAFEgainA);

	HRM_info("%s - current diff : %d,  toInc : %d\n", __func__, diff, toInc);

	if (prevStepDiffAgcA < diff) {
		reg_val = __adpd_reg_read(adpd_data, ADPD_SLOTA_GAIN_ADDR);
		__adpd_reg_write(adpd_data, ADPD_SLOTA_GAIN_ADDR, (reg_val & 0xfffc) | PrevTIAgainA);
		reg_val = __adpd_reg_read(adpd_data, RegAddrLedDrvA);
		__adpd_reg_write(adpd_data, RegAddrLedDrvA, (reg_val & 0xfff0) | PrevLedCoarseA);

		adpd_data->led_current_2 = PrevLedCoarseA; /* RED */
		HRM_info("%s - stopped at best!!!!![End]\n", __func__);
		NoSkipSamples = 0;

		return MwErrLedAOutofRange;
	}

	PrevLedScaleA = CurrLedScaleA;
	PrevLedCoarseA = CurrLedCoarseA;
	PrevTIAgainA = CurrTIAgainA;
	PrevAFEgainA = CurrAFEgainA;

	prevStepDiffAgcA = diff;

	if (!toInc) {
		if (diff >= raw_max >> 1) {
			if (!prefMinPower) { /* min TIA gain, max LED current */
				if (CurrTIAgainA < minTiaGainRegValA) {
					reg_val = __adpd_reg_read(adpd_data, ADPD_SLOTA_GAIN_ADDR);
					__adpd_reg_write(adpd_data, ADPD_SLOTA_GAIN_ADDR, (reg_val & 0xfffc) | (CurrTIAgainA + 1));

					NoSkipSamples = 0;

					HRM_info("%s - MwErrProcessing1[End]\n", __func__);

					return MwErrProcessing;
				} else {
					if (CurrLedCoarseA > minLedCurrentRegValA) {
						currLedCoarseCurrent = CurrLedCoarseA * 15 + 25;
						tgtLedCoarseCurrent = currLedCoarseCurrent >> 1;

						if (tgtLedCoarseCurrent < minLEDcurrDrvA)
							tgtLedCoarseCurrent = minLEDcurrDrvA;

						reg_val = __adpd_reg_read(adpd_data, RegAddrLedDrvA);
						__adpd_reg_write(adpd_data, RegAddrLedDrvA, (reg_val & 0xfff0) | ((tgtLedCoarseCurrent - 25) / 15));

						adpd_data->led_current_2 = (tgtLedCoarseCurrent - 25) / 15; /* RED */
						NoSkipSamples = 0;

						HRM_info("%s - MwErrProcessing2[End]\n", __func__);

						return MwErrProcessing;
					} else {
						NoSkipSamples = 0;
						HRM_dbg("%s - MwErrLedAOutofRange1[End]\n", __func__);

						return MwErrLedAOutofRange;
					}
				}
			} else { /* max TIA gain, min LED current */
				if (CurrLedCoarseA > minLedCurrentRegValA) {
					currLedCoarseCurrent = CurrLedCoarseA * 15 + 25;
					tgtLedCoarseCurrent = currLedCoarseCurrent >> 1;

					if (tgtLedCoarseCurrent < minLEDcurrDrvA)
						tgtLedCoarseCurrent = minLEDcurrDrvA;

					reg_val = __adpd_reg_read(adpd_data, RegAddrLedDrvA);
					__adpd_reg_write(adpd_data, RegAddrLedDrvA, (reg_val & 0xfff0) | ((tgtLedCoarseCurrent - 25) / 15));

					adpd_data->led_current_2 = (tgtLedCoarseCurrent - 25) / 15; /* RED */
					NoSkipSamples = 0;
					HRM_info("%s - MwErrProcessing3[End]\n", __func__);

					return MwErrProcessing;
				} else {
					if (CurrTIAgainA < minTiaGainRegValA) {
						reg_val = __adpd_reg_read(adpd_data, ADPD_SLOTA_GAIN_ADDR);
						__adpd_reg_write(adpd_data, ADPD_SLOTA_GAIN_ADDR, (reg_val & 0xfffc) | (CurrTIAgainA + 1));

						NoSkipSamples = 0;

						HRM_info("%s - MwErrProcessing4[End]\n", __func__);
						return MwErrProcessing;
					} else {
						NoSkipSamples = 0;
						HRM_dbg("%s - MwErrLedAOutofRange2[End]\n", __func__);

						return MwErrLedAOutofRange;
					}
				}
			}
		} else if (diff > TargetDcLevelA * AGC_STOP_MARGIN_PERCENTAGE_A / 100) {
			if (!prefMinPower) { /* min TIA gain, max LED current */
				if (CurrTIAgainA < minTiaGainRegValA) {
					reg_val = __adpd_reg_read(adpd_data, ADPD_SLOTA_GAIN_ADDR);
					__adpd_reg_write(adpd_data, ADPD_SLOTA_GAIN_ADDR, (reg_val & 0xfffc) | (CurrTIAgainA + 1));

					NoSkipSamples = 0;
					HRM_dbg("%s - MwErrLedAOutofRange3[End]\n", __func__);
					return MwErrLedAOutofRange;
				} else{
					if (CurrLedCoarseA > minLedCurrentRegValA) {
						reg_val = __adpd_reg_read(adpd_data, RegAddrLedDrvA);
						__adpd_reg_write(adpd_data, RegAddrLedDrvA, (reg_val & 0xfff0) | (CurrLedCoarseA - 1));

						adpd_data->led_current_2 = CurrLedCoarseA - 1; /* RED */
						NoSkipSamples = 0;

						HRM_info("%s - MwErrProcessing5[End]\n", __func__);

						return MwErrProcessing;
					} else {
						NoSkipSamples = 0;//<--here
						HRM_dbg("%s - MwErrLedAOutofRange4[End]\n", __func__);
						return MwErrLedAOutofRange;
					}
				}
			} else { /* max TIA gain, min LED current */
				if (CurrLedCoarseB > minLedCurrentRegValA) {
					reg_val = __adpd_reg_read(adpd_data, RegAddrLedDrvA);
					__adpd_reg_write(adpd_data, RegAddrLedDrvA, (reg_val & 0xfff0) | (CurrLedCoarseA - 1));

					adpd_data->led_current_2 = CurrLedCoarseA - 1; /* RED */
					NoSkipSamples = 0;

					HRM_info("%s - MwErrProcessing6[End]\n", __func__);

					return MwErrProcessing;
				} else {
					if (CurrTIAgainB < minTiaGainRegValA) {
						reg_val = __adpd_reg_read(adpd_data, ADPD_SLOTA_GAIN_ADDR);
						__adpd_reg_write(adpd_data, ADPD_SLOTA_GAIN_ADDR, (reg_val & 0xfffc) | (CurrTIAgainA + 1));
					}
					NoSkipSamples = 0;
					HRM_dbg("%s - MwErrLedAOutofRange5[End]\n", __func__);

					return MwErrLedAOutofRange;
				}
			}
		} else {
			NoSkipSamples = 0;
			HRM_info("%s - MwErrPass!!!!![End]\n", __func__);

			return MwErrPass;
		}
	} else { /* toInc */
		if (diff >= raw_max >> 1) {
			if (!prefMinPower) { /* min TIA gain, max LED current */
				if (CurrLedCoarseA < maxLedCurrentRegValA) {
					currLedCoarseCurrent = CurrLedCoarseA * 15 + 25;
					tgtLedCoarseCurrent = currLedCoarseCurrent << 1;

					if (tgtLedCoarseCurrent > maxLEDcurrDrvA)
						tgtLedCoarseCurrent = maxLEDcurrDrvA;

					reg_val = __adpd_reg_read(adpd_data, RegAddrLedDrvA);
					__adpd_reg_write(adpd_data, RegAddrLedDrvA, (reg_val & 0xfff0) | ((tgtLedCoarseCurrent-25) / 15));

					adpd_data->led_current_2 = (tgtLedCoarseCurrent-25) / 15; /* RED */
					NoSkipSamples = 0;

					HRM_info("%s - MwErrProcessing7[End]\n", __func__);

					return MwErrProcessing;
				} else {
					if (CurrTIAgainA > maxTiaGainRegValA) {
						reg_val = __adpd_reg_read(adpd_data, ADPD_SLOTA_GAIN_ADDR);
						__adpd_reg_write(adpd_data, ADPD_SLOTA_GAIN_ADDR, (reg_val & 0xfffc) | (CurrTIAgainA - 1));

						NoSkipSamples = 0;

						HRM_info("%s - MwErrProcessing8[End]\n", __func__);

						return MwErrProcessing;
					} else {
						NoSkipSamples = 0;
						HRM_dbg("%s - MwErrLedAOutofRange6[End]\n", __func__);

						return MwErrLedAOutofRange;
					}
				}
			} else { /* max TIA gain, min LED current*/
				if (CurrTIAgainA > maxTiaGainRegValA) {
					reg_val = __adpd_reg_read(adpd_data, ADPD_SLOTA_GAIN_ADDR);
					__adpd_reg_write(adpd_data, ADPD_SLOTA_GAIN_ADDR, (reg_val & 0xfffc) | (CurrTIAgainA - 1));

					NoSkipSamples = 0;

					HRM_info("%s - MwErrProcessing9[End]\n", __func__);

					return MwErrProcessing;
				} else {
					if (CurrLedCoarseA < maxLedCurrentRegValA - 5) {
						reg_val = __adpd_reg_read(adpd_data, RegAddrLedDrvA);
						__adpd_reg_write(adpd_data, RegAddrLedDrvA, (reg_val & 0xfff0) | (CurrLedCoarseA + 2));

						adpd_data->led_current_2 = CurrLedCoarseA + 2; /* RED */
						NoSkipSamples = 0;

						HRM_info("%s - MwErrProcessing10[End]\n", __func__);
						return MwErrProcessing;
					} else {
						NoSkipSamples = 0;
						HRM_dbg("%s - MwErrLedAOutofRange7[End]\n", __func__);

						return MwErrLedAOutofRange;
					}
				}
			}
		} else if (diff > TargetDcLevelA * AGC_STOP_MARGIN_PERCENTAGE_A / 100) {
			if (!prefMinPower) { /* min TIA gain, max LED current */
				if (CurrLedCoarseA < maxLedCurrentRegValA) {
					reg_val = __adpd_reg_read(adpd_data, RegAddrLedDrvA);
					__adpd_reg_write(adpd_data, RegAddrLedDrvA, (reg_val & 0xfff0) | (CurrLedCoarseA + 1));

					adpd_data->led_current_2 = CurrLedCoarseA + 1; /* RED */
					NoSkipSamples = 0;

					HRM_info("%s - MwErrProcessing11[End]\n", __func__);

					return MwErrProcessing;
				} else {
					NoSkipSamples = 0;
					HRM_dbg("%s - MwErrLedAOutofRange8[End]\n", __func__);

					return MwErrLedAOutofRange;
				}
			} else { /* max TIA gain, min LED current */
				if (CurrTIAgainA > maxTiaGainRegValA) {
					reg_val = __adpd_reg_read(adpd_data, ADPD_SLOTA_GAIN_ADDR);
					__adpd_reg_write(adpd_data, ADPD_SLOTA_GAIN_ADDR, (reg_val & 0xfffc) | (CurrTIAgainA - 1));

					NoSkipSamples = 0;
					HRM_info("%s - MwErrProcessing12[End]\n", __func__);

					return MwErrProcessing;
				} else {
					if (CurrLedCoarseA < maxLedCurrentRegValA) {
						reg_val = __adpd_reg_read(adpd_data, RegAddrLedDrvA);
						__adpd_reg_write(adpd_data, RegAddrLedDrvA, (reg_val & 0xfff0) | (CurrLedCoarseA + 1));

						adpd_data->led_current_2 = CurrLedCoarseA + 1; /* RED */
						NoSkipSamples = 0;

						HRM_info("%s - MwErrProcessing13[End]\n", __func__);

						return MwErrProcessing;
					} else {
						NoSkipSamples = 0;//<--here
						HRM_dbg("%s - MwErrLedAOutofRange9[End]\n", __func__);
						return MwErrLedAOutofRange;
					}
				}
			}
		} else {
			NoSkipSamples = 0;
			HRM_info("%s - MwErrPass!!!!![End]\n", __func__);

			return MwErrPass;
		}
	}
}

static AGC_ErrCode_t __adpd_stepAgcLedB(unsigned *rawData)
{
	unsigned raw_max, diff;
	unsigned char toInc = 0;
	unsigned reg_val;
	unsigned short currLedCoarseCurrent;
	unsigned short tgtLedCoarseCurrent;

	if (RegAddrLedDrvB != 0) {
		CurrLedDrvValB = __adpd_reg_read(adpd_data, RegAddrLedDrvB);
		CurrLedScaleB = (CurrLedDrvValB & 0x2000) >> 12;
		CurrLedCoarseB = CurrLedDrvValB & 0x000f;
	}

	reg_val = __adpd_reg_read(adpd_data, ADPD_SLOTB_GAIN_ADDR);
	CurrTIAgainB = reg_val & 0x3;
	CurrAFEgainB = (reg_val & 0x0300) >> 8;

	reg_val = __adpd_reg_read(adpd_data, ADPD_PULSE_PERIOD_A_ADDR);
	pulseA = (reg_val >> 8) & 0xFF;

	reg_val = __adpd_reg_read(adpd_data, ADPD_PULSE_PERIOD_B_ADDR);
	pulseB = (reg_val >> 8) & 0xFF;

#if defined(__USE_PD1234__) || defined(__USE_PD123__)
	raw_max = rawData[0];//__adpd_maxValueAGC(rawData, MAX_NUM_SAMPLE);
#elif defined(__USE_PD234__)
	raw_max = rawData[1];//__adpd_maxValueAGC(rawData, MAX_NUM_SAMPLE-1);
#elif defined(__USE_PD34__)
	raw_max = rawData[2];//__adpd_maxValueAGC(rawData, MAX_NUM_SAMPLE-2);
#elif defined(__USE_PD4__)
	raw_max = rawData[3];//__adpd_maxValueAGC(rawData, MAX_NUM_SAMPLE-3);
#endif

	HRM_info("%s - [start : %u]\n", __func__, cntStepAgcB++);
	HRM_info("%s - maxLedCurrentRegValB : %u minLedCurrentRegValB : %u maxTiaGainRegValB : %u minTiaGainRegValB : %u\n",
		__func__, maxLedCurrentRegValB, minLedCurrentRegValB, maxTiaGainRegValB, minTiaGainRegValB);
	HRM_info("%s - LED PulseA = %d PulseB = %d\n", __func__, pulseA, pulseB);
	HRM_info("%s - current sample : %d target : %d\n", __func__, raw_max, TargetDcLevelB);

	if (raw_max > TargetDcLevelB) {
		diff = raw_max - TargetDcLevelB;
		toInc = 0;
	} else {
		diff = TargetDcLevelB - raw_max;
		toInc = 1;
	}

	HRM_info("%s - CurrLedScaleB : %d,  CurrLedCoarseB : %d, CurrTIAgainB : %d, CurrAFEgainB : %d\n",
		__func__, CurrLedScaleB, CurrLedCoarseB, CurrTIAgainB, CurrAFEgainB);
	HRM_info("%s - PrevLedScaleB : %d,  PrevLedCoarseB : %d, PrevTIAgainB : %d, PrevAFEgainB : %d\n",
		__func__, PrevLedScaleB, PrevLedCoarseB, PrevTIAgainB, PrevAFEgainB);

	HRM_info("%s - current diff : %d,  toInc : %d\n", __func__, diff, toInc);

	if (prevStepDiffAgcB < diff) {
		reg_val = __adpd_reg_read(adpd_data, RegAddrLedDrvB);
		__adpd_reg_write(adpd_data, RegAddrLedDrvB, (reg_val & 0xfff0) | PrevLedCoarseB);

		adpd_data->led_current_1 = PrevLedCoarseB; /* IR */
		HRM_info("%s - stopped at best!!!!![End]\n", __func__);
		NoSkipSamples = 0;

		return MwErrLedAOutofRange;
	}

	PrevLedScaleB = CurrLedScaleB;
	PrevLedCoarseB = CurrLedCoarseB;
	PrevTIAgainB = CurrTIAgainB;
	PrevAFEgainB = CurrAFEgainB;

	prevStepDiffAgcB = diff;

	if (!toInc) {
		if (diff >= raw_max >> 1) {
			if (!prefMinPower) { /* min TIA gain, max LED current */
				if (CurrLedCoarseB > minLedCurrentRegValB) {
					currLedCoarseCurrent = CurrLedCoarseB * 15 + 25;
					tgtLedCoarseCurrent = currLedCoarseCurrent>>1;

					if (tgtLedCoarseCurrent < minLEDcurrDrvB)
						tgtLedCoarseCurrent = minLEDcurrDrvB;

					reg_val = __adpd_reg_read(adpd_data, RegAddrLedDrvB);
					__adpd_reg_write(adpd_data, RegAddrLedDrvB, (reg_val & 0xfff0) | ((tgtLedCoarseCurrent - 25) / 15));

					adpd_data->led_current_1 = (tgtLedCoarseCurrent - 25) / 15; /* IR */
					NoSkipSamples = 0;

					HRM_info("%s - MwErrProcessing2[End]\n", __func__);

					return MwErrProcessing;
				} else {
					NoSkipSamples = 0;//<--here
					HRM_dbg("%s - MwErrLedAOutofRange1[End]\n", __func__);
					return MwErrLedAOutofRange;
				}
			} else { /* max TIA gain, min LED current */
				if (CurrLedCoarseB > minLedCurrentRegValB) {
					currLedCoarseCurrent = CurrLedCoarseB * 15 + 25;
					tgtLedCoarseCurrent = currLedCoarseCurrent >> 1;

					if (tgtLedCoarseCurrent < minLEDcurrDrvB)
						tgtLedCoarseCurrent = minLEDcurrDrvB;

					reg_val = __adpd_reg_read(adpd_data, RegAddrLedDrvB);
					__adpd_reg_write(adpd_data, RegAddrLedDrvB, (reg_val & 0xfff0) | ((tgtLedCoarseCurrent - 25) / 15));

					adpd_data->led_current_1 = (tgtLedCoarseCurrent - 25) / 15; /* IR */
					NoSkipSamples = 0;

					HRM_info("%s - MwErrProcessing3[End]\n", __func__);

					return MwErrProcessing;
				} else {
					if (CurrTIAgainB < minTiaGainRegValB) {
						reg_val = __adpd_reg_read(adpd_data, ADPD_SLOTB_GAIN_ADDR);
						__adpd_reg_write(adpd_data, ADPD_SLOTB_GAIN_ADDR, (reg_val & 0xfffc) | (CurrTIAgainB + 1));

						NoSkipSamples = 0;

						HRM_info("%s - MwErrProcessing4[End]\n", __func__);

						return MwErrProcessing;
					} else {
						NoSkipSamples = 0;//<--here
						HRM_dbg("%s - MwErrLedAOutofRange2[End]\n", __func__);
						return MwErrLedAOutofRange;
					}
				}
			}
		} else if (diff > TargetDcLevelB * AGC_STOP_MARGIN_PERCENTAGE_B / 100) {
			if (!prefMinPower) { /* min TIA gain, max LED current */
				if (CurrLedCoarseB > minLedCurrentRegValB) {
					reg_val = __adpd_reg_read(adpd_data, RegAddrLedDrvB);
					__adpd_reg_write(adpd_data, RegAddrLedDrvB, (reg_val & 0xfff0) | (CurrLedCoarseB - 1));

					adpd_data->led_current_1 = CurrLedCoarseB - 1; /* IR */
					NoSkipSamples = 0;

					HRM_info("%s - MwErrProcessing5[End]\n", __func__);

					return MwErrProcessing;
				} else {
					NoSkipSamples = 0;//<--here
					HRM_dbg("%s - MwErrLedAOutofRange4[End]\n", __func__);
					return MwErrLedAOutofRange;
				}
			} else { /* max TIA gain, min LED current */
				if (CurrLedCoarseB > minLedCurrentRegValB) {
					reg_val = __adpd_reg_read(adpd_data, RegAddrLedDrvB);
					__adpd_reg_write(adpd_data, RegAddrLedDrvB, (reg_val & 0xfff0) | (CurrLedCoarseB - 1));

					adpd_data->led_current_1 = CurrLedCoarseB - 1; /* IR */
					NoSkipSamples = 0;

					HRM_info("%s - MwErrProcessing6[End]\n", __func__);

					return MwErrProcessing;
				} else {
					if (CurrTIAgainB < minTiaGainRegValB) {
						reg_val = __adpd_reg_read(adpd_data, ADPD_SLOTB_GAIN_ADDR);
						__adpd_reg_write(adpd_data, ADPD_SLOTB_GAIN_ADDR, (reg_val & 0xfffc) | (CurrTIAgainB + 1));
					}
					NoSkipSamples = 0;
					HRM_dbg("%s - MwErrLedAOutofRange5[End]\n", __func__);

					return MwErrLedAOutofRange;
				}
			}
		} else {
			NoSkipSamples = 0;//<<--here
			HRM_info("%s - MwErrPass[End]\n", __func__);
			return MwErrPass;
		}
	} else { /* toInc */
		if (diff >= raw_max >> 1) {
			if (!prefMinPower) { /* min TIA gain, max LED current */
				if (CurrLedCoarseB < maxLedCurrentRegValB) {
					currLedCoarseCurrent = CurrLedCoarseB * 15 + 25;
					tgtLedCoarseCurrent = currLedCoarseCurrent << 1;

					if (tgtLedCoarseCurrent > maxLEDcurrDrvB)
						tgtLedCoarseCurrent = maxLEDcurrDrvB;

					reg_val = __adpd_reg_read(adpd_data, RegAddrLedDrvB);
					__adpd_reg_write(adpd_data, RegAddrLedDrvB, (reg_val & 0xfff0) | ((tgtLedCoarseCurrent - 25) / 15));

					adpd_data->led_current_1 = (tgtLedCoarseCurrent - 25) / 15; /* IR */
					NoSkipSamples = 0;

					HRM_info("%s - MwErrProcessing7[End]\n", __func__);

					return MwErrProcessing;
				} else {
					NoSkipSamples = 0;//<--here
					HRM_dbg("%s - MwErrLedAOutofRange6[End]\n", __func__);

					return MwErrLedAOutofRange;
				}
			} else { /* max TIA gain, min LED current */
				if (CurrTIAgainB > maxTiaGainRegValB) {
					reg_val = __adpd_reg_read(adpd_data, ADPD_SLOTB_GAIN_ADDR);
					__adpd_reg_write(adpd_data, ADPD_SLOTB_GAIN_ADDR, (reg_val & 0xfffc) | (CurrTIAgainB - 1));

					NoSkipSamples = 0;

					HRM_info("%s - MwErrProcessing9[End]\n", __func__);

					return MwErrProcessing;
				} else {
					if (CurrLedCoarseB < maxLedCurrentRegValB - 5) {
						reg_val = __adpd_reg_read(adpd_data, RegAddrLedDrvB);
						__adpd_reg_write(adpd_data, RegAddrLedDrvB, (reg_val & 0xfff0) | (CurrLedCoarseB + 2));

						adpd_data->led_current_1 = CurrLedCoarseB + 2; /* IR */
						NoSkipSamples = 0;

						HRM_info("%s - MwErrProcessing10[End]\n", __func__);

						return MwErrProcessing;
					} else {
						NoSkipSamples = 0;//<--here
						HRM_dbg("%s - MwErrLedAOutofRange7[End]\n", __func__);
						return MwErrLedAOutofRange;
					}
				}
			}
		} else if (diff > TargetDcLevelB*AGC_STOP_MARGIN_PERCENTAGE_B / 100) {
			if (!prefMinPower) { /* min TIA gain, max LED current */
				if (CurrLedCoarseB < maxLedCurrentRegValB) {
					reg_val = __adpd_reg_read(adpd_data, RegAddrLedDrvB);
					__adpd_reg_write(adpd_data, RegAddrLedDrvB, (reg_val & 0xfff0) | (CurrLedCoarseB + 1));

					adpd_data->led_current_1 = CurrLedCoarseB + 1; /* IR */
					NoSkipSamples = 0;

					HRM_info("%s - MwErrProcessing11[End]\n", __func__);

					return MwErrProcessing;
				} else {
					NoSkipSamples = 0;
					HRM_dbg("%s - MwErrLedAOutofRange8[End]\n", __func__);

					return MwErrLedAOutofRange;
				}
			} else { /* max TIA gain, min LED current */
				if (CurrLedCoarseB < maxLedCurrentRegValB) {
					reg_val = __adpd_reg_read(adpd_data, RegAddrLedDrvB);
					__adpd_reg_write(adpd_data, RegAddrLedDrvB, (reg_val & 0xfff0) | (CurrLedCoarseB + 1));

					adpd_data->led_current_1 = CurrLedCoarseB + 1; /* IR */
					NoSkipSamples = 0;

					HRM_info("%s - MwErrProcessing13[End]\n", __func__);

					return MwErrProcessing;
				} else {
					NoSkipSamples = 0;//<--here
					HRM_dbg("%s - MwErrLedAOutofRange9[End]\n", __func__);
					return MwErrLedAOutofRange;
				}
			}
		} else {
			NoSkipSamples = 0;//<-here
			HRM_info("%s - MwErrPass[End]\n", __func__);
			return MwErrPass;
		}
	}
}

#ifdef __USE_LOWPASS_FIR_FILTER__
static unsigned firFilterIrBuf[100], firFilterRedBuf[100];
#ifdef __USE_ADPD_32BIT_FIFO__
static unsigned long firFilterIrBufSum = 0, firFilterRedBufSum = 0;
#else
static unsigned firFilterIrBufSum = 0, firFilterRedBufSum = 0;
#endif
static unsigned firFilterBufBegin = 0, firFilterBufEnd = 0, firFilterBufCnt = 0;
#define CNT_FIR_FLTR_TAB_FOR_PPG_SAMPLE  7
#endif

#ifdef __USE_LOWPASS_FIR_FILTER__
static void __adpd_initLPF(void)
{
	firFilterBufBegin = firFilterBufEnd = firFilterBufCnt = 0;
}

static void __adpd_getLPFdata
#ifdef __USE_ADPD_32BIT_FIFO__
(unsigned long *sum_a, unsigned long *sum_b)
#else
(unsigned *sum_a, unsigned *sum_b)
#endif
{
	unsigned i, j;

	firFilterIrBufSum = firFilterRedBufSum = 0;
	if (firFilterBufCnt++ < CNT_FIR_FLTR_TAB_FOR_PPG_SAMPLE) {
		firFilterIrBuf[firFilterBufEnd] = *sum_b;
		firFilterRedBuf[firFilterBufEnd++] = *sum_a;
	} else {
		firFilterBufCnt = CNT_FIR_FLTR_TAB_FOR_PPG_SAMPLE;
		if (firFilterBufBegin >= CNT_FIR_FLTR_TAB_FOR_PPG_SAMPLE)
			firFilterBufBegin = 0;
		firFilterIrBuf[firFilterBufBegin] = *sum_b;
		firFilterRedBuf[firFilterBufBegin++] = *sum_a;
	}

	for (i = (firFilterBufBegin > 0 ? firFilterBufBegin - 1 : 0), j = 0; i < firFilterBufCnt; i++, j++) {
		firFilterIrBufSum += firFilterIrBuf[i];
		firFilterRedBufSum += firFilterRedBuf[i];
	}
	for (i = 0; i < (firFilterBufBegin > 0 ? firFilterBufBegin - 1 : 0); i++) {
		firFilterIrBufSum += firFilterIrBuf[i];
		firFilterRedBufSum += firFilterRedBuf[i];
	}

	firFilterIrBufSum /= firFilterBufCnt;
	firFilterRedBufSum /= firFilterBufCnt;

	*sum_a = firFilterRedBufSum;
	*sum_b = firFilterIrBufSum;

}
#endif

#ifdef __USE_TIME_DOMAIN_SNR__
#ifdef __USE_LOWPASS_FIR_FILTER__
static unsigned sigDataChBackup[200][4], nosDataChBackup[30][4];
static unsigned sigCurrAMax = 0, sigCurrAMin = 0xffffffff, sigCurrBMax = 0, sigCurrBMin = 0xffffffff;
static unsigned sigFltrCurrAMax = 0, sigFltrCurrAMin = 0xffffffff, sigFltrCurrBMax = 0, sigFltrCurrBMin = 0xffffffff;
static unsigned nosCurrAMax = 0, nosCurrAMin = 0xffffffff, nosCurrBMax = 0, nosCurrBMin = 0xffffffff;
static unsigned nosFltrCurrAMax = 0, nosFltrCurrAMin = 0xffffffff, nosFltrCurrBMax = 0, nosFltrCurrBMin = 0xffffffff;
static unsigned short sigDataChBackupIdx = 0, nosDataChBackupIdx = 0;
static unsigned short sigFltrDataChBackupIdx = 0, nosFltrDataChBackupIdx = 0;
static unsigned sigCurrA = 0, nosCurrA = 0, snrCurrA = 0;
static unsigned sigFltrCurrA = 0, nosFltrCurrA = 0, snrFltrCurrA = 0;
static unsigned sigCurrB = 0, nosCurrB = 0, snrCurrB = 0;
static unsigned sigFltrCurrB = 0, nosFltrCurrB = 0, snrFltrCurrB = 0;
#else
static unsigned sigDataChBackup[200][2], nosDataChBackup[30][2];
static unsigned sigCurrAMax = 0, sigCurrAMin = 0xffffffff, sigCurrBMax = 0, sigCurrBMin = 0xffffffff;
static unsigned nosCurrAMax = 0, nosCurrAMin = 0xffffffff, nosCurrBMax = 0, nosCurrBMin = 0xffffffff;
static unsigned short sigDataChBackupIdx = 0, nosDataChBackupIdx = 0;
static unsigned sigCurrA = 0, nosCurrA = 0, snrCurrA = 0;
static unsigned sigCurrB = 0, nosCurrB = 0, snrCurrB = 0;
#endif
#define NOISE_CAL_TD_RANGE 20
#define SIGNAL_CAL_TD_RANGE 150
#endif

#ifdef __USE_TIME_DOMAIN_SNR__
static void __adpd_initSnrAtTD(void)
{
	sigDataChBackupIdx = nosDataChBackupIdx = 0;
	sigCurrA = nosCurrA = snrCurrA = 0;
	sigCurrB = nosCurrB = snrCurrB = 0;
#ifdef __USE_LOWPASS_FIR_FILTER__
	sigFltrDataChBackupIdx = nosFltrDataChBackupIdx = 0;
	sigFltrCurrA = nosFltrCurrA = snrFltrCurrA = 0;
	sigFltrCurrB = nosFltrCurrB = snrFltrCurrB = 0;
#endif
}

static void __adpd_updateSnrAtTD
#ifdef __USE_ADPD_32BIT_FIFO__
#ifdef __USE_LOWPASS_FIR_FILTER__
(unsigned long sum_a, unsigned long sum_b, unsigned long fltr_sum_a, unsigned long fltr_sum_b)
#else
(unsigned long sum_a, unsigned long sum_b)
#endif
#else
#ifdef __USE_LOWPASS_FIR_FILTER__
(unsigned sum_a, unsigned sum_b, unsigned fltr_sum_a, unsigned fltr_sum_b)
#else
(unsigned sum_a, unsigned sum_b)
#endif
#endif
{
    unsigned i;
	unsigned nosOut = 0;
#ifdef __USE_LOWPASS_FIR_FILTER__
	unsigned nosFltrOut = 0;
#endif

#ifdef __USE_LOWPASS_FIR_FILTER__
	nosDataChBackup[nosDataChBackupIdx][0] = sum_a;
	nosDataChBackup[nosDataChBackupIdx++][1] = sum_b;
	nosDataChBackup[nosFltrDataChBackupIdx][2] = fltr_sum_a;
	nosDataChBackup[nosFltrDataChBackupIdx++][3] = fltr_sum_b;
#else
	nosDataChBackup[nosDataChBackupIdx][0] = sum_a;
	nosDataChBackup[nosDataChBackupIdx++][1] = sum_b;
#endif

	if (nosDataChBackupIdx >= NOISE_CAL_TD_RANGE) {
		nosCurrAMax = nosCurrBMax = 0; nosCurrAMin = nosCurrBMin = 0xffffffff;
		for (i = 0; i < nosDataChBackupIdx; i++) {
			if (nosDataChBackup[i][0] > nosCurrAMax)
				nosCurrAMax = nosDataChBackup[i][0];
			if (nosDataChBackup[i][0] < nosCurrAMin)
				nosCurrAMin = nosDataChBackup[i][0];
			if (nosDataChBackup[i][1] > nosCurrBMax)
				nosCurrBMax = nosDataChBackup[i][1];
			if (nosDataChBackup[i][1] < nosCurrBMin)
				nosCurrBMin = nosDataChBackup[i][1];
		}
		nosCurrA = nosCurrAMax - nosCurrAMin;
		nosCurrB = nosCurrBMax - nosCurrBMin;
		HRM_info("%s - [ nosCurrAMax : %d, nosCurrAMin : %d, nosCurrBMax : %d, nosCurrBMin : %d ]\n", __func__,
			nosCurrAMax, nosCurrAMin,
			nosCurrBMax, nosCurrBMin);

		nosOut = 1;
		nosDataChBackupIdx = 0;
	}
#ifdef __USE_LOWPASS_FIR_FILTER__
	sigDataChBackup[sigDataChBackupIdx][0] = sum_a;
	sigDataChBackup[sigDataChBackupIdx++][1] = sum_b;
	sigDataChBackup[sigFltrDataChBackupIdx][2] = fltr_sum_a;
	sigDataChBackup[sigFltrDataChBackupIdx++][3] = fltr_sum_b;
#else
	sigDataChBackup[sigDataChBackupIdx][0] = sum_a;
	sigDataChBackup[sigDataChBackupIdx++][1] = sum_b;
#endif
	if (sigDataChBackupIdx >= SIGNAL_CAL_TD_RANGE) {
		sigCurrAMax = sigCurrBMax = 0; sigCurrAMin = sigCurrBMin = 0xffffffff;
		for (i = 0; i < sigDataChBackupIdx; i++) {
			if (sigDataChBackup[i][0] > sigCurrAMax)
				sigCurrAMax = sigDataChBackup[i][0];
			if (sigDataChBackup[i][0] < sigCurrAMin)
				sigCurrAMin = sigDataChBackup[i][0];
			if (sigDataChBackup[i][1] > sigCurrBMax)
				sigCurrBMax = sigDataChBackup[i][1];
			if (sigDataChBackup[i][1] < sigCurrBMin)
				sigCurrBMin = sigDataChBackup[i][1];
		}

		sigCurrA = sigCurrAMax - sigCurrAMin;
		sigCurrB = sigCurrBMax - sigCurrBMin;
		HRM_info("%s - [ sigCurrAMax : %d, sigCurrAMin : %d, sigCurrBMax : %d, sigCurrBMin : %d ]\n", __func__,
			sigCurrAMax, sigCurrAMin,
			sigCurrBMax, sigCurrBMin);

		if (nosOut) {
			snrCurrA = sigCurrA / nosCurrA;
			snrCurrB = sigCurrB / nosCurrB;

			HRM_info("%s - [ SNR_A : %d, SNR_B : %d, PI_A : %d, PI_B : %d ]\n", __func__,
				snrCurrA, snrCurrB,
				sigCurrA * 10000 / sigCurrAMax, sigCurrB*10000 / sigCurrBMax);
			nosOut = 0;
		}

		sigDataChBackupIdx = 0;
	}

#ifdef __USE_LOWPASS_FIR_FILTER__
	if (nosFltrDataChBackupIdx >= NOISE_CAL_TD_RANGE) {
		nosFltrCurrAMax = nosFltrCurrBMax = 0; nosFltrCurrAMin = nosFltrCurrBMin = 0xffffffff;
		for (i = 0; i < nosFltrDataChBackupIdx; i++) {
			if (nosDataChBackup[i][2] > nosFltrCurrAMax)
				nosFltrCurrAMax = nosDataChBackup[i][2];
			if (nosDataChBackup[i][2] < nosFltrCurrAMin)
				nosFltrCurrAMin = nosDataChBackup[i][2];
			if (nosDataChBackup[i][3] > nosFltrCurrBMax)
				nosFltrCurrBMax = nosDataChBackup[i][3];
			if (nosDataChBackup[i][3] < nosFltrCurrBMin)
				nosFltrCurrBMin = nosDataChBackup[i][3];
		}
		nosFltrCurrA = nosFltrCurrAMax - nosFltrCurrAMin;
		nosFltrCurrB = nosFltrCurrBMax - nosFltrCurrBMin;
		HRM_info("%s - [ nosFltrCurrAMax : %d, nosFltrCurrAMin : %d, nosFltrCurrBMax : %d, nosFltrCurrBMin : %d ]\n", __func__,
			nosFltrCurrAMax, nosFltrCurrAMin,
			nosFltrCurrBMax, nosFltrCurrBMin);

		nosFltrOut = 1;
		nosFltrDataChBackupIdx = 0;
	}
	if (sigFltrDataChBackupIdx >= SIGNAL_CAL_TD_RANGE) {
		sigFltrCurrAMax = sigFltrCurrBMax = 0; sigFltrCurrAMin = sigFltrCurrBMin = 0xffffffff;
		for (i = 0; i < sigFltrDataChBackupIdx; i++) {
			if (sigDataChBackup[i][2] > sigFltrCurrAMax)
				sigFltrCurrAMax = sigDataChBackup[i][2];
			if (sigDataChBackup[i][2] < sigFltrCurrAMin)
				sigFltrCurrAMin = sigDataChBackup[i][2];
			if (sigDataChBackup[i][3] > sigFltrCurrBMax)
				sigFltrCurrBMax = sigDataChBackup[i][3];
			if (sigDataChBackup[i][3] < sigFltrCurrBMin)
				sigFltrCurrBMin = sigDataChBackup[i][3];
		}

		sigFltrCurrA = sigFltrCurrAMax - sigFltrCurrAMin;
		sigFltrCurrB = sigFltrCurrBMax - sigFltrCurrBMin;
		HRM_info("%s - [ sigFltrCurrAMax : %d, sigFltrCurrAMin : %d, sigFltrCurrBMax : %d, sigFltrCurrBMin : %d ]\n", __func__,
			sigFltrCurrAMax, sigFltrCurrAMin,
			sigFltrCurrBMax, sigFltrCurrBMin);

		if (nosFltrOut) {
			snrFltrCurrA = sigFltrCurrA / nosFltrCurrA;
			snrFltrCurrB = sigFltrCurrB / nosFltrCurrB;

			HRM_info("%s - [Flitered] [ SNR_A : %d, SNR_B : %d, PI_A : %d, PI_B : %d ]\n", __func__,
				snrFltrCurrA, snrFltrCurrB,
				sigFltrCurrA * 10000 / sigFltrCurrAMax, sigFltrCurrB * 10000 / sigFltrCurrBMax);
			nosFltrOut = 0;
		}
		sigFltrDataChBackupIdx = 0;
	}
#endif
}
#endif

void __adpd_setObjProximityThreshold(unsigned thresh)
{
	 adpd_data->obj_proximity_theshld = thresh;
}

OBJ_PROXIMITY_STATUS __adpd_getObjProximity(void)
{
	return adpd_data->st_obj_proximity;
}
static EOL_state __adpd_dark_offset_calibration(unsigned *dataInA, unsigned *dataInB);

static unsigned prev_sum_dataIn = 0;

#define CH1_OFFSET_PER_PULSE_FOR_DC_NOISE_REMOVAL 100
#define CH2_OFFSET_PER_PULSE_FOR_DC_NOISE_REMOVAL 100
#define CH3_OFFSET_PER_PULSE_FOR_DC_NOISE_REMOVAL 50
#define CH4_OFFSET_PER_PULSE_FOR_DC_NOISE_REMOVAL 50

#define MARGIN_FOR_CH_SATURATION_PER_PULSE   100
#define ST_LED1_CURRENT_SETTING 	0x3035
#define ST_LED2_CURRENT_SETTING 	0x303f

static void __adpd_InitProximityDetection(void)
{
	__adpd_reg_write(adpd_data, ADPD_OP_MODE_ADDR, 0x1);
	__adpd_reg_write(adpd_data, ADPD_INT_MASK_ADDR, 0x1ff);
	__adpd_reg_write(adpd_data, ADPD_INT_STATUS_ADDR, 0x80ff);
	__adpd_reg_write(adpd_data, ADPD_INT_MASK_ADDR, ~(0x0040) & 0x1ff);

	if (agc_is_enabled) {
		__adpd_reg_write(adpd_data, ADPD_LED2_DRV_ADDR, ST_LED2_CURRENT_SETTING);
		adpd_data->led_current_1 = ST_LED2_CURRENT_SETTING & 0xf; /* IR */
	}

	__adpd_reg_write(adpd_data, ADPD_LED_DISABLE_ADDR, 0x0100);
	__adpd_reg_write(adpd_data, ADPD_OP_MODE_ADDR, 0x2);
}

static void __adpd_SetProximityOn(void)
{
	__adpd_reg_write(adpd_data, ADPD_OP_MODE_ADDR, 0x1);
	__adpd_reg_write(adpd_data, ADPD_INT_MASK_ADDR, 0x1ff);
	__adpd_reg_write(adpd_data, ADPD_INT_STATUS_ADDR, 0x80ff);
	__adpd_reg_write(adpd_data, ADPD_INT_MASK_ADDR, ~(0x0040) & 0x1ff);

	if (agc_is_enabled) {
		__adpd_reg_write(adpd_data, ADPD_LED1_DRV_ADDR, ST_LED1_CURRENT_SETTING);
		__adpd_reg_write(adpd_data, ADPD_LED2_DRV_ADDR, ST_LED2_CURRENT_SETTING);
		adpd_data->led_current_1 = ST_LED2_CURRENT_SETTING & 0xf; /* IR */
		adpd_data->led_current_2 = ST_LED1_CURRENT_SETTING & 0xf; /* RED */
		cntThreshAgcStepOn = CNT_THRESHOLD_AGC_STEP_ON;
	} else
		cntThreshAgcStepOn = 1;

	__adpd_reg_write(adpd_data, ADPD_LED_DISABLE_ADDR, 0x0);
	__adpd_reg_write(adpd_data, ADPD_OP_MODE_ADDR, 0x2);
}

static void __adpd_ClearFifo(void)
{
	__adpd_reg_write(adpd_data, ADPD_OP_MODE_ADDR, 0x1);
	__adpd_reg_write(adpd_data, ADPD_INT_STATUS_ADDR, 0x80ff);
	__adpd_reg_write(adpd_data, ADPD_OP_MODE_ADDR, 0x2);
}

static unsigned char isClearSaturationRed = 0;
static unsigned char isClearSaturationIr = 0;

static void __adpd_ClearSaturation(unsigned short isSlotB)
{
	unsigned short reg_val;
	__adpd_reg_write(adpd_data, ADPD_OP_MODE_ADDR, 0x1);

	/* RED */
	if (!isSlotB) {
		isClearSaturationRed = 1;

		reg_val = __adpd_reg_read(adpd_data, ADPD_SLOTA_GAIN_ADDR);
		
		if ((reg_val & 0x0003) < 3)
			__adpd_reg_write(adpd_data, ADPD_SLOTA_GAIN_ADDR, reg_val + 1);

		else {
			reg_val = __adpd_reg_read(adpd_data, RegAddrLedDrvA);

			if ((reg_val & 0x000f) > 0) {
				__adpd_reg_write(adpd_data, RegAddrLedDrvA, reg_val - 1);
				adpd_data->led_current_2 = (reg_val & 0xf) - 1;
			} else
				HRM_dbg("%s : Current SlotA DC Saturation can't be resolved...PPG SNR will be decreased...", __func__);
		}
	/* IR */	
	} else {
		isClearSaturationIr = 1;

		reg_val = __adpd_reg_read(adpd_data, ADPD_SLOTB_GAIN_ADDR);
		
		if ((reg_val & 0x0003) < 3)
			__adpd_reg_write(adpd_data, ADPD_SLOTB_GAIN_ADDR, reg_val + 1);

		else {
			reg_val = __adpd_reg_read(adpd_data, RegAddrLedDrvB);

			if ((reg_val & 0x000f) > 0) {
				__adpd_reg_write(adpd_data, RegAddrLedDrvB, reg_val - 1);
				adpd_data->led_current_1 = (reg_val & 0xf) - 1;
			} else
				HRM_dbg("%s : Current SlotB DC Saturation can't be resolved...PPG SNR will be decreased...", __func__);
		}
	}

	__adpd_reg_write(adpd_data, ADPD_OP_MODE_ADDR, 0x2);
}


#define PROXIMITY_STABLIZATION_THRESHOLD 0

OBJ_PROXIMITY_STATUS __adpd_checkObjProximity(unsigned *dataInA, unsigned *dataIn)
{
	unsigned dataInMax = 0;
	unsigned dataInAMax = 0;
	unsigned sum_dataIn = 0, sum_dataIn_raw = 0;
	unsigned sum_dataInA = 0, sum_dataInA_raw = 0;

	unsigned tempData[4] = {0, 0, 0, 0};

	if (AgcMode & AGC_Slot_B && !dataIn)
		return adpd_data->st_obj_proximity;
	if (AgcMode & AGC_Slot_A && !dataInA)
		return adpd_data->st_obj_proximity;

#if defined(__USE_PD1234__) || defined(__USE_PD123__)
	if (AgcMode & AGC_Slot_B && dataIn) {
		dataInMax = dataIn[0];
		sum_dataIn_raw = dataIn[0]+dataIn[1]+dataIn[2]+dataIn[3];
	}
	if (AgcMode & AGC_Slot_A && dataInA) {
		dataInAMax = dataInA[0];
		sum_dataInA_raw = dataInA[0]+dataInA[1]+dataInA[2]+dataInA[3];
	}
#elif defined(__USE_PD234__)
	if (AgcMode & AGC_Slot_B && dataIn) {
		dataInMax = dataIn[1];
		sum_dataIn_raw = dataIn[1]+dataIn[2]+dataIn[3];
	}
	if (AgcMode & AGC_Slot_A && dataInA) {
		dataInAMax = dataInA[1];
		sum_dataInA_raw = dataInA[1]+dataInA[2]+dataInA[3];
	}
#elif defined(__USE_PD34__)
	if (AgcMode & AGC_Slot_B && dataIn) {
		dataInMax = dataIn[2];
		sum_dataIn_raw = dataIn[2]+dataIn[3];
	}
	if (AgcMode & AGC_Slot_A && dataInA) {
		dataInAMax = dataInA[2];
		sum_dataInA_raw = dataInA[2]+dataInA[3];
	}
#elif defined(__USE_PD4__)
	if (AgcMode & AGC_Slot_B && dataIn) {
		dataInMax = dataIn[3];
		sum_dataIn_raw = dataIn[3];
	}
	if (AgcMode & AGC_Slot_A && dataInA) {
		dataInAMax = dataInA[3];
		sum_dataInA_raw = dataInA[3];
	}
#endif

	if (AgcMode & AGC_Slot_A && dataInA)
		sum_dataInA = __adpd_normalized_data(adpd_data, 0, sum_dataIn_raw);
	if (AgcMode & AGC_Slot_B && dataIn)
		sum_dataIn = __adpd_normalized_data(adpd_data, 1, sum_dataIn_raw);

	HRM_info("%s - dataInMax = %d, dataInAMax = %d, priv st_obj_proximity = %d\n", __func__, dataInMax, dataInAMax, adpd_data->st_obj_proximity);

	switch (adpd_data->st_obj_proximity) {
	case OBJ_DARKOFFSET_CAL:
		if (AgcMode & AGC_Slot_A && AgcMode & AGC_Slot_B && dataInA && dataIn) {
			HRM_info("%s - OBJ_DARKOFFSET_CAL AB( %u %u %u %u %u %u %u %u )\n", __func__,
			dataInA[0], dataInA[1], dataInA[2], dataInA[3], dataIn[0], dataIn[1], dataIn[2], dataIn[3]);
		} else if (AgcMode & AGC_Slot_A && dataInA) {
			HRM_info("%s - OBJ_DARKOFFSET_CAL A( %u %u %u %u )\n", __func__,
			dataInA[0], dataInA[1], dataInA[2], dataInA[3]);
		} else if (AgcMode & AGC_Slot_B && dataIn) {
			HRM_info("%s - OBJ_DARKOFFSET_CAL B( %u %u %u %u )\n", __func__,
			dataIn[0], dataIn[1], dataIn[2], dataIn[3]);
		}

		if (!dataInA)
			dataInA = tempData;

		if (!dataIn)
			dataIn = tempData;

		if (__adpd_dark_offset_calibration(dataInA, dataIn) == ST_EOL_CLOCK_CAL_DONE) {
			HRM_info("%s - Measured DarkOffset ( %u %u %u %u	%u %u %u %u )\n", __func__,
				g_eol_ADCOffset[0], g_eol_ADCOffset[1], g_eol_ADCOffset[2], g_eol_ADCOffset[3],
				g_eol_ADCOffset[4], g_eol_ADCOffset[5], g_eol_ADCOffset[6], g_eol_ADCOffset[7]);

			__adpd_reg_write(adpd_data, ADPD_OP_MODE_ADDR, 0x1);

			//More safety offet to guarantee any remaning DC noise after dark calibration
			g_eol_ADCOffset[0] += CH1_OFFSET_PER_PULSE_FOR_DC_NOISE_REMOVAL;
			g_eol_ADCOffset[1] += CH2_OFFSET_PER_PULSE_FOR_DC_NOISE_REMOVAL;
			g_eol_ADCOffset[2] += CH3_OFFSET_PER_PULSE_FOR_DC_NOISE_REMOVAL;
			g_eol_ADCOffset[3] += CH4_OFFSET_PER_PULSE_FOR_DC_NOISE_REMOVAL;
			g_eol_ADCOffset[4] += CH1_OFFSET_PER_PULSE_FOR_DC_NOISE_REMOVAL;
			g_eol_ADCOffset[5] += CH2_OFFSET_PER_PULSE_FOR_DC_NOISE_REMOVAL;
			g_eol_ADCOffset[6] += CH3_OFFSET_PER_PULSE_FOR_DC_NOISE_REMOVAL;
			g_eol_ADCOffset[7] += CH4_OFFSET_PER_PULSE_FOR_DC_NOISE_REMOVAL;
			__adpd_reg_write(adpd_data, ADPD_CH1_OFFSET_A_ADDR, g_eol_ADCOffset[0]);
			__adpd_reg_write(adpd_data, ADPD_CH2_OFFSET_A_ADDR, g_eol_ADCOffset[1]);
			__adpd_reg_write(adpd_data, ADPD_CH3_OFFSET_A_ADDR, g_eol_ADCOffset[2]);
			__adpd_reg_write(adpd_data, ADPD_CH4_OFFSET_A_ADDR, g_eol_ADCOffset[3]);
			__adpd_reg_write(adpd_data, ADPD_CH1_OFFSET_B_ADDR, g_eol_ADCOffset[4]);
			__adpd_reg_write(adpd_data, ADPD_CH2_OFFSET_B_ADDR, g_eol_ADCOffset[5]);
			__adpd_reg_write(adpd_data, ADPD_CH3_OFFSET_B_ADDR, g_eol_ADCOffset[6]);
			__adpd_reg_write(adpd_data, ADPD_CH4_OFFSET_B_ADDR, g_eol_ADCOffset[7]);

			maxPulseCodeACh1 = 0x4000 - g_eol_ADCOffset[0];
			maxPulseCodeACh2 = 0x4000 - g_eol_ADCOffset[1];
			maxPulseCodeACh3 = 0x4000 - g_eol_ADCOffset[2];
			maxPulseCodeACh4 = 0x4000 - g_eol_ADCOffset[3];
			maxPulseCodeBCh1 = 0x4000 - g_eol_ADCOffset[4];
			maxPulseCodeBCh2 = 0x4000 - g_eol_ADCOffset[5];
			maxPulseCodeBCh3 = 0x4000 - g_eol_ADCOffset[6];
			maxPulseCodeBCh4 = 0x4000 - g_eol_ADCOffset[7];

			HRM_info("%s - Measured Max Codes Per Pulse ( %u %u %u %u %u %u %u %u )\n", __func__,
				maxPulseCodeACh1, maxPulseCodeACh2, maxPulseCodeACh3, maxPulseCodeACh4,
				maxPulseCodeBCh1, maxPulseCodeBCh2, maxPulseCodeBCh3, maxPulseCodeBCh4);

			__adpd_reg_write(adpd_data, ADPD_SAMPLING_FREQ_ADDR, 0xa);
			__adpd_reg_write(adpd_data, ADPD_DEC_MODE_ADDR, 0x330);
			__adpd_reg_write(adpd_data, ADPD_PULSE_PERIOD_A_ADDR, 0x813);
			__adpd_reg_write(adpd_data, ADPD_PULSE_PERIOD_B_ADDR, 0x813);
			__adpd_reg_write(adpd_data, ADPD_OP_MODE_ADDR, 0x2);
			adpd_data->Is_Hrm_Dark_Calibrated = 1;

			__adpd_InitProximityDetection();
			adpd_data->st_obj_proximity = OBJ_DETACHED;
		}
		break;

	case OBJ_DETACHED:
		prev_sum_dataIn = 0;

		HRM_info("%s - Measured Max Codes ( %u %u %u %u %u %u %u %u )\n", __func__,
			maxPulseCodeACh1*pulseA, maxPulseCodeACh2*pulseA, maxPulseCodeACh3*pulseA, maxPulseCodeACh4*pulseA,
			maxPulseCodeBCh1*pulseB, maxPulseCodeBCh2*pulseB, maxPulseCodeBCh3*pulseB, maxPulseCodeBCh4*pulseB);

		if (sum_dataIn > adpd_data->obj_proximity_theshld) {
			HRM_info("%s - OBJ_DETECTED ( %u %u )\n", __func__,
			sum_dataIn, adpd_data->obj_proximity_theshld);
			adpd_data->st_obj_proximity = OBJ_DETECTED;
			adpd_data->cnt_proximity_on = 0;
		} else {
			HRM_info("%s - OBJ_DETACHED ( %u %u )\n", __func__,
			sum_dataIn, adpd_data->obj_proximity_theshld);
		}
		break;

	case OBJ_DETECTED:
		HRM_info("%s - OBJ_DETECTED ( %u %u )\n", __func__,
			sum_dataIn, adpd_data->obj_proximity_theshld);

		if (sum_dataIn < adpd_data->obj_proximity_theshld * OBJ_DETACH_FROM_DETECTION_DC_PERCENT / 100)
			adpd_data->st_obj_proximity = OBJ_DETACHED;
		else if (sum_dataIn < adpd_data->obj_proximity_theshld * OBJ_DETECTION_CNT_DC_PERCENT / 100)
			adpd_data->cnt_proximity_on = (adpd_data->cnt_proximity_on > 0) ? adpd_data->cnt_proximity_on - 1 : 0;
		else if (sum_dataIn >= prev_sum_dataIn + PROXIMITY_STABLIZATION_THRESHOLD)
			adpd_data->cnt_proximity_on = (adpd_data->cnt_proximity_on > 100) ? 100 : adpd_data->cnt_proximity_on + 1;

		if (adpd_data->cnt_proximity_on >= CNT_THRESHOLD_OBJ_PROXIMITY_ON) {
			HRM_info("%s - OBJ_DETECTED : OBJ_PROXIMITY_RED_ON ( %u )\n",
				__func__, adpd_data->cnt_proximity_on);
			adpd_data->st_obj_proximity = OBJ_PROXIMITY_RED_ON;
			adpd_data->cnt_proximity_on = 0;
			adpd_data->cur_proximity_dc_level = sum_dataIn * 70 / 100;
			adpd_data->cur_proximity_red_dc_level = sum_dataInA * 70 / 100;
			__adpd_SetProximityOn();

			if (AgcMode & AGC_Slot_A && AgcMode & AGC_Slot_B) {
				if (RegAddrLedDrvA != 0)
					AgcStage = AgcStage_LedA;
				else if (RegAddrLedDrvB != 0)
					AgcStage = AgcStage_LedB;
				else
					AgcStage = AgcStage_Done;
			} else if (AgcMode & AGC_Slot_A) {
				if (RegAddrLedDrvA != 0)
					AgcStage = AgcStage_LedA;
				else {
					AgcStage = AgcStage_Done;
				}
			} else if (AgcMode & AGC_Slot_B) {
				if (RegAddrLedDrvB != 0)
					AgcStage = AgcStage_LedB;
				else
					AgcStage = AgcStage_Done;
			}

			if (agc_is_enabled)
				__adpd_initAGC(POWER_PREF, AgcMode);

			prev_sum_dataIn = 0;

		} else
			prev_sum_dataIn = sum_dataIn;

		break;

	case OBJ_PROXIMITY_RED_ON:
		if (agc_is_enabled && __adpd_getAGCstate() != AgcStage_Done &&
			 (adpd_data->cnt_proximity_on >= CNT_THRESHOLD_AGC_STEP_ON || !prev_sum_dataIn) &&
			 adpd_data->agc_mode != M_NONE) {
			HRM_info("%s - OBJ_PROXIMITY_RED_ON - in AGC\n", __func__);
			__adpd_stepAGC(dataInA, dataIn);
			__adpd_ClearFifo();
			adpd_data->cnt_proximity_on = 0;
		}

		if (sum_dataIn < adpd_data->cur_proximity_dc_level * OBJ_DETACH_FROM_PROXIMITY_DC_PERCENT / 100) {
			HRM_info("%s - OBJ_PROXIMITY_RED_ON ( %u < %u )\n", __func__,
				sum_dataIn, adpd_data->cur_proximity_dc_level * OBJ_DETACH_FROM_PROXIMITY_DC_PERCENT / 100);
			adpd_data->st_obj_proximity = OBJ_DETACHED;

			__adpd_InitProximityDetection();
			break;

		} else if (sum_dataIn < adpd_data->cur_proximity_dc_level * OBJ_PROXIMITY_CNT_DC_PERCENT / 100) {
			HRM_info("%s - OBJ_PROXIMITY_RED_ON( %u < %u )\n", __func__,
			sum_dataIn, adpd_data->cur_proximity_dc_level);
			adpd_data->cnt_proximity_on = (adpd_data->cnt_proximity_on > 0) ? adpd_data->cnt_proximity_on - 1 : 0;
		} else {
			HRM_info("%s - OBJ_PROXIMITY_RED_ON ( %u %u )\n", __func__,
				sum_dataIn, adpd_data->cur_proximity_dc_level);
			adpd_data->cnt_proximity_on = (adpd_data->cnt_proximity_on > 100) ? 100 : adpd_data->cnt_proximity_on + 1;
		}

		if ((adpd_data->cnt_proximity_on >= CNT_THRESHOLD_AGC_STEP_ON && __adpd_getAGCstate() == AgcStage_Done) ||
			!agc_is_enabled) {
			HRM_info("%s - OBJ_PROXIMITY_RED_ON ( %u ), agc_is_enabled : %d\n", __func__,
				adpd_data->cnt_proximity_on, agc_is_enabled);
			adpd_data->st_obj_proximity = OBJ_PROXIMITY_ON;
			adpd_data->cnt_proximity_on = 3;
			adpd_data->cur_proximity_dc_level = (sum_dataIn > adpd_data->obj_proximity_theshld) ? sum_dataIn : adpd_data->obj_proximity_theshld;
			adpd_data->cur_proximity_red_dc_level = (sum_dataInA > adpd_data->cur_proximity_red_dc_level) ? sum_dataInA : adpd_data->cur_proximity_red_dc_level;
			adpd_data->cnt_slotA_dc_saturation = 0;
			adpd_data->cnt_slotB_dc_saturation = 0;
			
			adpd_data->ir_curr = adpd_data->led_current_1;
			adpd_data->red_curr = adpd_data->led_current_2;
			adpd_data->ir_adc = sum_dataIn;
			adpd_data->red_adc = sum_dataInA;
			
			HRM_info("%s - ir_curr = 0x%2x, red_curr = 0x%2x, ir_adc = %d, red_adc = %d\n",
				__func__, adpd_data->ir_curr, adpd_data->red_curr, adpd_data->ir_adc, adpd_data->red_adc);

#ifdef __USE_LOWPASS_FIR_FILTER__
			__adpd_initLPF();
#endif
#ifdef __USE_TIME_DOMAIN_SNR__
			__adpd_initSnrAtTD();
#endif
		}
		prev_sum_dataIn = sum_dataIn;

		break;

	case OBJ_PROXIMITY_ON:
#ifdef __ALL_TIME_AGC__
		AgcStage = AgcStage_LedA;
		__adpd_stepAGC2(dataInA, dataIn);
#endif

		if (isClearSaturationIr) {
			adpd_data->cur_proximity_dc_level = (sum_dataIn < adpd_data->cur_proximity_dc_level) ? sum_dataIn : adpd_data->cur_proximity_dc_level;
			isClearSaturationIr = 0;
		}
		if (isClearSaturationRed) {
			adpd_data->cur_proximity_red_dc_level = (sum_dataInA < adpd_data->cur_proximity_red_dc_level) ? sum_dataInA : adpd_data->cur_proximity_red_dc_level;
			isClearSaturationRed = 0;
		}
		if (sum_dataIn < adpd_data->cur_proximity_dc_level * OBJ_DETACH_FROM_PROXIMITY_DC_PERCENT / 100) {
			HRM_info("%s - OBJ_DETACHED ( %u < %u )\n", __func__,
				sum_dataIn, adpd_data->cur_proximity_dc_level * OBJ_DETACH_FROM_PROXIMITY_DC_PERCENT / 100);
			adpd_data->st_obj_proximity = OBJ_DETACHED;

			__adpd_InitProximityDetection();

			break;
		} else {
#ifdef __USE_TIME_DOMAIN_SNR__
			HRM_info("%s - OBJ_PROXIMITY_ON ( %u %u ), SNR_A : %d, SNR_B : %d, PI_A : %d, PI_B : %d\n", __func__,
				sum_dataIn, adpd_data->cur_proximity_dc_level,
				snrCurrA, snrCurrB,
				sigCurrA*10000/sigCurrAMax, sigCurrB*10000/sigCurrBMax);
#else
			HRM_info("%s - OBJ_PROXIMITY_ON ( %u %u )\n", __func__,
				sum_dataIn, adpd_data->cur_proximity_dc_level);
#endif
		}
		if (sum_dataIn > adpd_data->cur_proximity_dc_level) {
			adpd_data->cur_proximity_dc_level = adpd_data->cur_proximity_dc_level*7 + sum_dataIn;
			adpd_data->cur_proximity_dc_level >>= 3;
		}
		if (sum_dataInA > adpd_data->cur_proximity_red_dc_level) {
			adpd_data->cur_proximity_red_dc_level = adpd_data->cur_proximity_red_dc_level*7 + sum_dataInA;
			adpd_data->cur_proximity_red_dc_level >>= 3;
		}

#if defined(__USE_PD1234__) || defined(__USE_PD123__)
		if (dataInMax >= (maxPulseCodeBCh1 - MARGIN_FOR_CH_SATURATION_PER_PULSE) * pulseB)
#elif defined(__USE_PD234__)
		if (dataInMax >= (maxPulseCodeBCh2 - MARGIN_FOR_CH_SATURATION_PER_PULSE) * pulseB)
#elif defined(__USE_PD34__)
		if (dataInMax >= (maxPulseCodeBCh3 - MARGIN_FOR_CH_SATURATION_PER_PULSE) * pulseB)
#elif defined(__USE_PD4__)
		if (dataInMax >= (maxPulseCodeBCh4 - MARGIN_FOR_CH_SATURATION_PER_PULSE) * pulseB)
#endif
#if defined(__USE_PD1234__) || defined(__USE_PD234__) || defined(__USE_PD34__) || defined(__USE_PD123__) || defined(__USE_PD4__)
		{
			adpd_data->cnt_slotB_dc_saturation = (adpd_data->cnt_slotB_dc_saturation > 100) ? 100 : adpd_data->cnt_slotB_dc_saturation + 1;
			if (adpd_data->cnt_slotB_dc_saturation > CNT_THRESHOLD_DC_SATURATION) {
				HRM_info("%s - Slot B saturation!!! (ChB %u >= %u )\n", __func__,
#if defined(__USE_PD1234__) || defined(__USE_PD123__)
				dataInMax, (maxPulseCodeBCh1 - MARGIN_FOR_CH_SATURATION_PER_PULSE) * pulseB);
#elif defined(__USE_PD234__)
				dataInMax, (maxPulseCodeBCh2 - MARGIN_FOR_CH_SATURATION_PER_PULSE) * pulseB);
#elif defined(__USE_PD34__)
				dataInMax, (maxPulseCodeBCh3 - MARGIN_FOR_CH_SATURATION_PER_PULSE) * pulseB);
#elif defined(__USE_PD4__)
				dataInMax, (maxPulseCodeBCh4 - MARGIN_FOR_CH_SATURATION_PER_PULSE) * pulseB);
#endif
				__adpd_ClearSaturation(1);
#ifdef __USE_LOWPASS_FIR_FILTER__
				__adpd_initLPF();
#endif
#ifdef __USE_TIME_DOMAIN_SNR__
				__adpd_initSnrAtTD();
#endif
			}
			//break;
		} else
			adpd_data->cnt_slotB_dc_saturation = (adpd_data->cnt_slotB_dc_saturation > 0) ? adpd_data->cnt_slotB_dc_saturation - 1 : 0;
#endif

#if defined(__USE_PD1234__) || defined(__USE_PD123__)
			if (dataInAMax >= (maxPulseCodeACh1 - MARGIN_FOR_CH_SATURATION_PER_PULSE) * pulseA)
#elif defined(__USE_PD234__)
			if (dataInAMax >= (maxPulseCodeACh2 - MARGIN_FOR_CH_SATURATION_PER_PULSE) * pulseA)
#elif defined(__USE_PD34__)
			if (dataInAMax >= (maxPulseCodeACh3 - MARGIN_FOR_CH_SATURATION_PER_PULSE) * pulseA)
#elif defined(__USE_PD4__)
			if (dataInAMax >= (maxPulseCodeACh4 - MARGIN_FOR_CH_SATURATION_PER_PULSE) * pulseA)
#endif
#if defined(__USE_PD1234__) || defined(__USE_PD234__) || defined(__USE_PD34__) || defined(__USE_PD123__) || defined(__USE_PD4__)
			{
				adpd_data->cnt_slotA_dc_saturation = (adpd_data->cnt_slotA_dc_saturation > 100) ? 100 : adpd_data->cnt_slotA_dc_saturation + 1;
				if (adpd_data->cnt_slotA_dc_saturation > CNT_THRESHOLD_DC_SATURATION) {
					HRM_info("%s - Slot A saturation!!! (ChA %u >= %u )\n", __func__,
#if defined(__USE_PD1234__) || defined(__USE_PD123__)
						dataInAMax, (maxPulseCodeACh1 - MARGIN_FOR_CH_SATURATION_PER_PULSE) * pulseA);
#elif defined(__USE_PD234__)
						dataInAMax, (maxPulseCodeACh2 - MARGIN_FOR_CH_SATURATION_PER_PULSE) * pulseA);
#elif defined(__USE_PD34__)
						dataInAMax, (maxPulseCodeACh3 - MARGIN_FOR_CH_SATURATION_PER_PULSE) * pulseA);
#elif defined(__USE_PD4__)
						dataInAMax, (maxPulseCodeACh4 - MARGIN_FOR_CH_SATURATION_PER_PULSE) * pulseA);
#endif
					__adpd_ClearSaturation(0);
#ifdef __USE_LOWPASS_FIR_FILTER__
					__adpd_initLPF();
#endif
#ifdef __USE_TIME_DOMAIN_SNR__
					__adpd_initSnrAtTD();
#endif
				}
				//break;
			}
		else
			adpd_data->cnt_slotA_dc_saturation = (adpd_data->cnt_slotA_dc_saturation > 0) ? adpd_data->cnt_slotA_dc_saturation - 1 : 0;
#endif
		break;

	case OBJ_HARD_PRESSURE:
		break;

	case DEVICE_ON_TABLE:
		break;

	default:
		break;
	}
	return adpd_data->st_obj_proximity;
}

/* read & write efs for hrm sensor */
static int __adpd_osc_trim_efs_register_open(struct adpd_device_data *adpd_data)
{
	struct file *osc_filp = NULL;
	char buffer[60] = {0, };
	int err = 0;
	int osc_trim_32k;
	mm_segment_t old_fs;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	osc_filp = filp_open(OSCILLATOR_TRIM_FILE_PATH, O_RDONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (IS_ERR(osc_filp)) {
		err = PTR_ERR(osc_filp);

		if (err != -ENOENT) {
			HRM_dbg("%s - Can't open oscillator trim file, err = %d\n", __func__, err);
		} else {
			HRM_dbg("%s - No such file or directory, err = %d\n", __func__, err);
		}
		set_fs(old_fs);
		return err;
	}

	err = osc_filp->f_op->read(osc_filp,
		(char *)buffer,
		60 * sizeof(char), &osc_filp->f_pos);
	if (err != (60 * sizeof(char))) {
		HRM_dbg("%s - Can't read the oscillator trim data from file\n", __func__);
		err = -EIO;
	}
	sscanf(buffer, "%d", &osc_trim_32k);
	HRM_info("%s - osc_trim_32k = 0x%04x\n", __func__, osc_trim_32k);

	adpd_data->oscRegValue = osc_trim_32k;

	filp_close(osc_filp, current->files);
	set_fs(old_fs);

	return err;
}

static int __adpd_osc_trim_efs_register_save(struct adpd_device_data *adpd_data)
{
	struct file *osc_filp = NULL;
	mm_segment_t old_fs;
	char buffer[60] = {0, };
	int osc_trim_32k;
	int err = 0;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	osc_filp = filp_open(OSCILLATOR_TRIM_FILE_PATH,
			O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (IS_ERR(osc_filp)) {
		HRM_dbg("%s - Can't open oscillator trim file\n", __func__);
		set_fs(old_fs);
		err = PTR_ERR(osc_filp);
		return err;
	}

	osc_trim_32k = __adpd_reg_read(adpd_data, OSC_TRIM_32K_REG);

	sprintf(buffer, "%d", osc_trim_32k);
	HRM_info("%s - osc_trim_32k = 0x%04x\n", __func__, osc_trim_32k);

	err = osc_filp->f_op->write(osc_filp,
		(char *)&buffer,
		60 * sizeof(char), &osc_filp->f_pos);
	if (err != (60 * sizeof(char))) {
		HRM_dbg("%s - Can't write the oscillator trim data to file\n",	__func__);
		err = -EIO;
	}
	adpd_data->oscRegValue = osc_trim_32k;

	filp_close(osc_filp, current->files);
	set_fs(old_fs);

	return err;
}

static void __adpd_clr_intr_fifo(struct adpd_device_data *adpd_data)
{
	__adpd_reg_write(adpd_data, ADPD_INT_STATUS_ADDR, 0x80FF);
}

static void __adpd_clr_intr_status(struct adpd_device_data *adpd_data, unsigned short mode)
{
	__adpd_reg_write(adpd_data, ADPD_INT_STATUS_ADDR,
		  adpd_data->intr_status_val);
}

static unsigned short __adpd_rd_intr_fifo(struct adpd_device_data *adpd_data)
{
	unsigned short regval = 0;
	unsigned short fifo_size = 0;
	unsigned short usr_mode = 0;
	regval = __adpd_reg_read(adpd_data, ADPD_INT_STATUS_ADDR);
	if (GET_USR_MODE(atomic_read(&adpd_data->adpd_mode)) != TIA_ADC_USR) {
		fifo_size = ((regval & 0xFF00) >> 8);
		adpd_data->fifo_size = ((fifo_size / 2) + (fifo_size & 1));
		adpd_data->intr_status_val = (regval & 0xFF);

		if (fifo_size <= 16)
			atomic_inc(&adpd_data->stats.fifo_bytes[0]);
		else if (fifo_size <= 32)
			atomic_inc(&adpd_data->stats.fifo_bytes[1]);
		else if (fifo_size <= 64)
			atomic_inc(&adpd_data->stats.fifo_bytes[2]);
		else
			atomic_inc(&adpd_data->stats.fifo_bytes[3]);

		usr_mode = atomic_read(&adpd_data->adpd_mode);

		if (0 != usr_mode) {
			unsigned int mask = 0;
			unsigned int mod = 0;

			switch (GET_USR_SUB_MODE(usr_mode)) {
			case S_SAMP_XY_AB:
				mask = 0x07;
				break;
			default:
				mask = 0x03;
				break;
			};

			mod = adpd_data->fifo_size & mask;
			if (mod) {
				adpd_data->fifo_size &= ~mask;
				atomic_inc(&adpd_data->stats.fifo_requires_sync);
			}
		}
	} else
		adpd_data->fifo_size = ((regval & 0xFF00) >> 8) >> 1;

	adpd_data->intr_status_val = regval;

	HRM_info("%s - FIFO_SIZE=%d\n", __func__,
		adpd_data->fifo_size);

	return adpd_data->intr_status_val;
}

#ifdef __USE_ADPD_32BIT_FIFO__
static unsigned short __adpd_rd_intr_fifo32(struct adpd_device_data *adpd_data)
{
	unsigned short regval = 0;
	unsigned short fifo_size = 0;
	unsigned short usr_mode = 0;
	regval = __adpd_reg_read(adpd_data, ADPD_INT_STATUS_ADDR);
	fifo_size = ((regval & 0xFF00) >> 8);
	adpd_data->fifo_size = ((fifo_size >> 2) + (fifo_size & 3));
	adpd_data->intr_status_val = (regval & 0xFF);

	usr_mode = atomic_read(&adpd_data->adpd_mode);
	if (0 != usr_mode) {
		unsigned int mask = 0;
		unsigned int mod = 0;

		switch (GET_USR_SUB_MODE(usr_mode)) {
		case S_SAMP_XY_AB:
			mask = 0x07;
			break;
		default:
			mask = 0x03;
			break;
		};

		mod = adpd_data->fifo_size & mask;
		if (mod) {
			adpd_data->fifo_size &= ~mask;
			atomic_inc(&adpd_data->stats.fifo_requires_sync);
		}
	}

	return adpd_data->intr_status_val;
}
#endif

static unsigned int __adpd_rd_fifo_data(struct adpd_device_data *adpd_data)
{
	unsigned short for_p2p_clkfifo = 0;

	if (!adpd_data->fifo_size)
		goto err_rd_fifo_data;

	if (0 != atomic_read(&adpd_data->adpd_mode) && adpd_data->fifo_size & 0x3) {
		HRM_dbg("%s - unexpected FIFO_SIZE=%d, should be multiple of four (channels)!\n", __func__,
			adpd_data->fifo_size);
	}

	for_p2p_clkfifo = __adpd_reg_read(adpd_data, ADPD_ACCESS_CTRL_ADDR);
	__adpd_reg_write(adpd_data, ADPD_ACCESS_CTRL_ADDR, for_p2p_clkfifo|1);
	__adpd_reg_write(adpd_data, ADPD_ACCESS_CTRL_ADDR, for_p2p_clkfifo|1);

	__adpd_multi_reg_read(adpd_data, ADPD_DATA_BUFFER_ADDR,
		adpd_data->fifo_size, adpd_data->ptr_data_buffer);

	return adpd_data->fifo_size;

err_rd_fifo_data:
	return 0;
}

#ifdef __USE_ADPD_32BIT_FIFO__
static unsigned int __adpd_rd_fifo32_data(struct adpd_device_data *adpd_data)
{
	unsigned short for_p2p_clkfifo = 0;

	if (!adpd_data->fifo_size)
		goto err_rd_fifo_data;

	if (0 != atomic_read(&adpd_data->adpd_mode) &&
		adpd_data->fifo_size & 0x3) {
			HRM_dbg("%s - unexpected FIFO_SIZE=%d, should be multiple of four (channels)!\n", __func__,
			adpd_data->fifo_size);
	}

	for_p2p_clkfifo = __adpd_reg_read(adpd_data, ADPD_ACCESS_CTRL_ADDR);
	__adpd_reg_write(adpd_data, ADPD_ACCESS_CTRL_ADDR, for_p2p_clkfifo|1);
	__adpd_reg_write(adpd_data, ADPD_ACCESS_CTRL_ADDR, for_p2p_clkfifo|1);
	__adpd_multi_reg_read(adpd_data, ADPD_DATA_BUFFER_ADDR,
		adpd_data->fifo_size, adpd_data->ptr_data_buffer);

	return adpd_data->fifo_size;
err_rd_fifo_data:
	return 0;
}
#endif

static void __adpd_config_prerequisite(struct adpd_device_data *adpd_data, u32 global_mode)
{
	unsigned short regval = 0;
	regval = __adpd_reg_read(adpd_data, ADPD_OP_MODE_ADDR);
	regval = SET_GLOBAL_OP_MODE(regval, global_mode);

	HRM_info("%s - reg 0x%04x,\tafter set 0x%04x\n", __func__, regval, SET_GLOBAL_OP_MODE(regval, global_mode));

	__adpd_reg_write(adpd_data, ADPD_OP_MODE_ADDR, regval);
}

static int __adpd_configuration(struct adpd_device_data *adpd_data, unsigned char config)
{
	struct adpd_platform_data *ptr_config;
	unsigned short addr;
	unsigned short data;
	unsigned short cnt = 0;
	int ret = 0;

	if (config == SAMPLE_USR)
		ptr_config = &adpd_config_data;
	else if (config == TIA_ADC_USR)
		ptr_config = &adpd_tia_adc_config_data;
	else
		return -EINVAL;

	for (cnt = 0; cnt < ptr_config->config_size; cnt++) {
		addr = (unsigned short) ((0xFFFF0000 &
					  ptr_config->config_data[cnt]) >> 16);
		data = (unsigned short) (0x0000FFFF &
					 ptr_config->config_data[cnt]);

		__adpd_reg_write(adpd_data, addr, data);
	}

	if (adpd_data->oscRegValue || __adpd_osc_trim_efs_register_open(adpd_data) >= 0)
		__adpd_reg_write(adpd_data, ADPD_OSC32K_ADDR, adpd_data->oscRegValue);

	HRM_info("%s - oscRegValue : 0x%04x\n",	__func__, adpd_data->oscRegValue);

	return ret;
}

static void __adpd_mode_switching(struct adpd_device_data *adpd_data, u32 usr_mode)
{
	unsigned int opmode_val = 0;
	unsigned short mode_val = 0;
	unsigned short intr_mask_val = 0;
	unsigned short i;

	unsigned short main_mode = GET_USR_MODE(usr_mode);
	unsigned short sub_mode = GET_USR_SUB_MODE(usr_mode);

	u32 prev_mode = atomic_read(&adpd_data->adpd_mode);
	if (prev_mode == usr_mode) {
		HRM_dbg("%s - skip usr_mode = 0x%x prev_mode = 0x%x\n", __func__, usr_mode, prev_mode);
		return;
	}

	HRM_info("%s - usr_mode = 0x%x\n", __func__, usr_mode);

	atomic_set(&adpd_data->adpd_mode, 0);
	mutex_lock(&adpd_data->mutex);

	/*Depending upon the mode get the value need to write Operatin mode*/
	opmode_val = *(__mode_recv_frm_usr[main_mode].mode_code + (sub_mode));

	__adpd_config_prerequisite(adpd_data, GLOBAL_OP_MODE_IDLE);

	/*switch to IDLE mode */
	mode_val = DEFAULT_OP_MODE_CFG_VAL(adpd_data);
	mode_val += IDLE_OFF;
	intr_mask_val = SET_INTR_MASK(IDLE_USR);

	__adpd_reg_write(adpd_data, ADPD_OP_MODE_CFG_ADDR, mode_val);
	__adpd_reg_write(adpd_data, ADPD_INT_MASK_ADDR, intr_mask_val);

	/*clear FIFO and flush buffer */
	__adpd_clr_intr_fifo(adpd_data);
	__adpd_rd_intr_fifo(adpd_data);
	if (adpd_data->fifo_size != 0) {
		__adpd_clr_intr_status(adpd_data, IDLE_USR);
		__adpd_rd_fifo_data(adpd_data);
	}

	__adpd_config_prerequisite(adpd_data, GLOBAL_OP_MODE_GO);
	msleep(20);
	__adpd_config_prerequisite(adpd_data, GLOBAL_OP_MODE_IDLE);

	/*Find Interrupt mask value */
	switch (main_mode) {
	case IDLE_USR:
		HRM_dbg("%s - IDLE MODE\n", __func__);
		intr_mask_val = SET_INTR_MASK(IDLE_USR);
		fifo_is_32bit = 0;
		break;

	case SAMPLE_USR:
		HRM_dbg("%s - SAMPLE MODE\n", __func__);
		/*enable interrupt only when data written to FIFO */
		switch (sub_mode) {
		case S_SAMP_XY_A:
			intr_mask_val = SET_INTR_MASK(S_SAMP_XY_A_USR);
			if (!adpd_data->Is_Hrm_Dark_Calibrated) {
				adpd_data->st_obj_proximity = OBJ_DARKOFFSET_CAL;
				adpd_data->eol_state = ST_EOL_IDLE;
			} else {
				adpd_data->st_obj_proximity = OBJ_DARKOFFSET_CAL;
				adpd_data->eol_state = ST_EOL_CLOCK_CAL_DONE;
			}


			__adpd_initAGC(POWER_PREF, AGC_Slot_A);

			fifo_is_32bit = 0;
			break;

		case S_SAMP_XY_AB:
			intr_mask_val = SET_INTR_MASK(S_SAMP_XY_AB_USR);
			if (!adpd_data->Is_Hrm_Dark_Calibrated) {
				adpd_data->st_obj_proximity = OBJ_DARKOFFSET_CAL;
				adpd_data->eol_state = ST_EOL_IDLE;
			} else {
				adpd_data->st_obj_proximity = OBJ_DARKOFFSET_CAL;
				adpd_data->eol_state = ST_EOL_CLOCK_CAL_DONE;
			}

			__adpd_initAGC(POWER_PREF, AGC_Slot_A | AGC_Slot_B);

			fifo_is_32bit = 0;
			break;

		case S_SAMP_XY_B:
			intr_mask_val = SET_INTR_MASK(S_SAMP_XY_B_USR);
			if (!adpd_data->Is_Hrm_Dark_Calibrated) {
				adpd_data->st_obj_proximity = OBJ_DARKOFFSET_CAL;
				adpd_data->eol_state = ST_EOL_IDLE;
			} else {
				adpd_data->st_obj_proximity = OBJ_DARKOFFSET_CAL;
				adpd_data->eol_state = ST_EOL_CLOCK_CAL_DONE;
			}


			__adpd_initAGC(POWER_PREF, AGC_Slot_B);

			fifo_is_32bit = 0;
			break;

		default:
			intr_mask_val = SET_INTR_MASK(SAMPLE_USR);
			break;
		};

		intr_mask_val |= 0xC000;

		__adpd_configuration(adpd_data, SAMPLE_USR);

		break;

	case TIA_ADC_USR:
		HRM_dbg("%s - TIA_ADC_USR SAMPLE MODE\n", __func__);
		ch1FloatSat = CH1_APPROXIMATE_FLOAT_SATURATION_VALUE_IN_TIA_ADC;
		ch2FloatSat = CH2_APPROXIMATE_FLOAT_SATURATION_VALUE_IN_TIA_ADC;
		ch3FloatSat = CH3_APPROXIMATE_FLOAT_SATURATION_VALUE_IN_TIA_ADC;
		ch4FloatSat = CH4_APPROXIMATE_FLOAT_SATURATION_VALUE_IN_TIA_ADC;
		Is_TIA_ADC_Dark_Calibrated = Is_Float_Dark_Calibrated = 0;
		for (i = 0; i < 8; i++)
			rawDarkCh[i] = rawDataCh[i] = 0;

		intr_mask_val = ~(0x100) & 0x1ff;

		intr_mask_val |= 0xC000;

		fifo_is_32bit = 0;
		HRM_dbg("%s - TIA_ADC_configuration\n", __func__);
		__adpd_configuration(adpd_data, TIA_ADC_USR);
		break;

	case EOL_USR:
		HRM_dbg("%s - EOL SAMPLE MODE\n", __func__);
		adpd_data->eol_state = ST_EOL_IDLE;
		adpd_data->eol_res_odr = 0;
		prev_interrupt_trigger_ts.tv_sec = -1;
		switch (sub_mode) {
		case S_SAMP_XY_A:
			intr_mask_val = SET_INTR_MASK(S_SAMP_XY_A_USR);
			break;

		case S_SAMP_XY_AB:
			intr_mask_val = SET_INTR_MASK(S_SAMP_XY_AB_USR);
			break;

		case S_SAMP_XY_B:
			intr_mask_val = SET_INTR_MASK(S_SAMP_XY_B_USR);
			break;

		default:
			intr_mask_val = SET_INTR_MASK(SAMPLE_USR);
			break;
		};
		intr_mask_val |= 0xC000;

		fifo_is_32bit = 0;
		HRM_dbg("%s - EOL_configuration\n", __func__);
		__adpd_configuration(adpd_data, SAMPLE_USR);
		break;

	default:
		/*This case won't occur */
		HRM_dbg("%s - invalid mode\n", __func__);
		break;
	};

	HRM_info("%s - INT_MASK_ADDR[0x%04x] = 0x%04x\n", __func__,
		ADPD_INT_MASK_ADDR, intr_mask_val);
	adpd_data->intr_mask_val = intr_mask_val;

	/*Fetch Default opmode configuration other than OP mode
	   and DATA_OUT mode */
	mode_val = DEFAULT_OP_MODE_CFG_VAL(adpd_data);
	/*Get Mode value from the opmode value,
	to hardocde the macro, change SET_MODE_VALUE macro and concern
	mode macro
	*/
	mode_val += opmode_val;

	HRM_info("%s - OP_MODE_CFG[0x%04x] = 0x%04x\n", __func__,
		ADPD_OP_MODE_CFG_ADDR, mode_val);

	atomic_set(&adpd_data->adpd_mode, usr_mode);

	__adpd_reg_write(adpd_data, ADPD_INT_MASK_ADDR, adpd_data->intr_mask_val);
	__adpd_reg_write(adpd_data, ADPD_OP_MODE_CFG_ADDR, mode_val);

	mutex_unlock(&adpd_data->mutex);

	if (main_mode != IDLE_USR)
		__adpd_config_prerequisite(adpd_data, GLOBAL_OP_MODE_GO);
	else
		__adpd_config_prerequisite(adpd_data, GLOBAL_OP_MODE_OFF);

}

unsigned short __adpd_32khzTrim(void)
{
	unsigned short oscTrimValue;
	int error;
	unsigned short oscRegValue;

	oscRegValue = __adpd_reg_read(adpd_data, ADPD_OSC32K_ADDR);
	oscTrimValue = oscRegValue & 0x3F;

	if (EOL_ODR_TARGET - adpd_data->eol_res_odr > 0)
		error = 100 * EOL_ODR_TARGET - 100 * adpd_data->eol_res_odr;
	else
		error = 100 * adpd_data->eol_res_odr - 100 * EOL_ODR_TARGET;

	if (EOL_ODR_TARGET - adpd_data->eol_res_odr > 0)
		oscTrimValue -= (error/625 > 0) ? ((error - (error / 625) * 625 > 320) ? (error / 625) + 1 : (error / 625)) : ((error > 320) ? 1 : 0);
	else
		oscTrimValue += (error/625 > 0) ? ((error - (error / 625) * 625 > 320) ? (error / 625) + 1 : (error / 625)) : ((error > 320) ? 1 : 0);

	oscRegValue &= 0xFFC0;
	oscRegValue |= oscTrimValue;

	HRM_info("%s - oscRegValue : 0x%04x, error : %d\n",
				__func__, oscRegValue, error);

	__adpd_reg_write(adpd_data, ADPD_OSC32K_ADDR, oscRegValue);
	return oscRegValue; /* Joshua, for saving Clock Calibration result */
}

static EOL_state __adpd_dark_offset_calibration(unsigned *dataInA, unsigned *dataInB)
{
	/*adpd_data->mutex is necessary for clock cal. */
	unsigned short i = 0;

	if (adpd_data->eol_state == ST_EOL_IDLE) {
		adpd_data->eol_state = ST_EOL_CLOCK_CAL_INIT;

		for (i = 0; i < 8; i++)
			g_eol_ADCOffset[i] = 0;

		for (i = 0; i < 32; i++)
			g_org_regValues[i] = __adpd_reg_read(adpd_data, g_eol_regNumbers[i]);

		__adpd_reg_write(adpd_data, ADPD_OP_MODE_ADDR, 0x1);
		__adpd_reg_write(adpd_data, ADPD_SAMPLING_FREQ_ADDR, 8000 / EOL_ODR_TARGET);
		__adpd_reg_write(adpd_data, ADPD_DEC_MODE_ADDR, 0x0);

		if (dataInA)
			__adpd_reg_write(adpd_data, ADPD_PULSE_PERIOD_A_ADDR, 0x0113);

		if (dataInB)
			__adpd_reg_write(adpd_data, ADPD_PULSE_PERIOD_B_ADDR, 0x0113);

		__adpd_reg_write(adpd_data, ADPD_LED_DISABLE_ADDR, 0x0300);

		if (dataInA) {
			__adpd_reg_write(adpd_data, ADPD_CH1_OFFSET_A_ADDR, 0);
			__adpd_reg_write(adpd_data, ADPD_CH2_OFFSET_A_ADDR, 0);
			__adpd_reg_write(adpd_data, ADPD_CH3_OFFSET_A_ADDR, 0);
			__adpd_reg_write(adpd_data, ADPD_CH4_OFFSET_A_ADDR, 0);
		}

		if (dataInB) {
			__adpd_reg_write(adpd_data, ADPD_CH1_OFFSET_B_ADDR, 0);
			__adpd_reg_write(adpd_data, ADPD_CH2_OFFSET_B_ADDR, 0);
			__adpd_reg_write(adpd_data, ADPD_CH3_OFFSET_B_ADDR, 0);
			__adpd_reg_write(adpd_data, ADPD_CH4_OFFSET_B_ADDR, 0);
		}

		adpd_data->eol_counter = 0;
		__adpd_reg_write(adpd_data, ADPD_OP_MODE_ADDR, 0x2);
	} else if (adpd_data->eol_state == ST_EOL_CLOCK_CAL_INIT) {
		if (adpd_data->eol_counter++ > 20) {
			adpd_data->eol_state = ST_EOL_CLOCK_CAL_RUNNING;
			adpd_data->eol_counter = 0;
		}
	} else if (adpd_data->eol_state == ST_EOL_CLOCK_CAL_RUNNING) {
		adpd_data->eol_counter++;
		if (adpd_data->eol_counter <= 16) {
			for (i = 0; i < 4; i++) {
				if (dataInA)
					g_eol_ADCOffset[i] += dataInA[i];

				if (dataInB)
					g_eol_ADCOffset[i+4] += dataInB[i];
			}
		} else if (adpd_data->eol_counter == 17) {
			for (i = 0; i < 8; i++) {
				g_eol_ADCOffset[i] >>= 4;
				HRM_info("%s - g_eol_ADCOffset[i] : 0x%04x\n", __func__, g_eol_ADCOffset[i]);
				adpd_data->eol_state = ST_EOL_CLOCK_CAL_DONE;
			}
		}
	}

	return adpd_data->eol_state;
}
static EOL_state __adpd_step_EOL_clock_calibration(unsigned *dataInA, unsigned *dataInB)
{
	/*adpd_data->mutex is necessary for clock cal. */
	unsigned short i = 0;

	if (adpd_data->eol_state == ST_EOL_IDLE) {
		adpd_data->eol_state = ST_EOL_CLOCK_CAL_INIT;

		for (i = 0; i < 8; i++)
			g_eol_ADCOffset[i] = 0;

		for (i = 0; i < 32; i++)
			g_org_regValues[i] = __adpd_reg_read(adpd_data, g_eol_regNumbers[i]);

		__adpd_reg_write(adpd_data, ADPD_OP_MODE_ADDR, GLOBAL_OP_MODE_IDLE);
		__adpd_reg_write(adpd_data, ADPD_SAMPLING_FREQ_ADDR, 8000 / EOL_ODR_TARGET);
		__adpd_reg_write(adpd_data, ADPD_DEC_MODE_ADDR, 0x0);

		if (dataInA)
			__adpd_reg_write(adpd_data, ADPD_PULSE_PERIOD_A_ADDR, 0x0113);

		if (dataInB)
			__adpd_reg_write(adpd_data, ADPD_PULSE_PERIOD_B_ADDR, 0x0113);

		__adpd_reg_write(adpd_data, ADPD_LED_DISABLE_ADDR, 0x0300);

		if (dataInA) {
			__adpd_reg_write(adpd_data, ADPD_CH1_OFFSET_A_ADDR, 0);
			__adpd_reg_write(adpd_data, ADPD_CH2_OFFSET_A_ADDR, 0);
			__adpd_reg_write(adpd_data, ADPD_CH3_OFFSET_A_ADDR, 0);
			__adpd_reg_write(adpd_data, ADPD_CH4_OFFSET_A_ADDR, 0);
		}

		if (dataInB) {
			__adpd_reg_write(adpd_data, ADPD_CH1_OFFSET_B_ADDR, 0);
			__adpd_reg_write(adpd_data, ADPD_CH2_OFFSET_B_ADDR, 0);
			__adpd_reg_write(adpd_data, ADPD_CH3_OFFSET_B_ADDR, 0);
			__adpd_reg_write(adpd_data, ADPD_CH4_OFFSET_B_ADDR, 0);
		}

		adpd_data->eol_counter = 0;

		__adpd_reg_write(adpd_data, ADPD_INT_MASK_ADDR, 0x1ff);
		__adpd_reg_write(adpd_data, ADPD_INT_STATUS_ADDR, 0x80ff);
		__adpd_reg_write(adpd_data, ADPD_INT_MASK_ADDR, ~(0x0040) & 0x1ff);

		__adpd_reg_write(adpd_data, ADPD_OP_MODE_ADDR, GLOBAL_OP_MODE_GO);

	} else if (adpd_data->eol_state == ST_EOL_CLOCK_CAL_INIT) {
		if (adpd_data->eol_counter++ > 20) {
			adpd_data->eol_state = ST_EOL_CLOCK_CAL_RUNNING;
			adpd_data->eol_counter = 0;
		}
	} else if (adpd_data->eol_state == ST_EOL_CLOCK_CAL_RUNNING) {
		adpd_data->eol_counter++;

#ifndef __USE_EOL_US_INT_SPACE__
		adpd_data->eol_measured_period += adpd_data->msec_eol_int_space;
		HRM_info("%s - eol_counter : %d, msec_eol_int_space : %dms, eol_measured_period : %ldms, adpd_data->eol_res_odr : %dhz\n",
					__func__, adpd_data->eol_counter, adpd_data->msec_eol_int_space, adpd_data->eol_measured_period, adpd_data->eol_res_odr);
#else
		adpd_data->eol_measured_period += adpd_data->usec_eol_int_space;
		HRM_info("%s - eol_counter : %d, usec_eol_int_space : %dus, eol_measured_period : %ldus, adpd_data->eol_res_odr : %dhz\n",
					__func__, adpd_data->eol_counter, adpd_data->usec_eol_int_space, adpd_data->eol_measured_period, adpd_data->eol_res_odr);
#endif
		if (adpd_data->eol_counter <= 16) {
			for (i = 0; i < 4; i++) {
				if (dataInA)
					g_eol_ADCOffset[i] += dataInA[i];

				if (dataInB)
					g_eol_ADCOffset[i+4] += dataInB[i];
			}
		} else if (adpd_data->eol_counter == 17) {
			for (i = 0; i < 8; i++)	{
			  g_eol_ADCOffset[i] >>= 4;

			  HRM_info("%s - g_eol_ADCOffset[i] : 0x%04x\n", __func__, g_eol_ADCOffset[i]);
			}
		}
#ifndef __USE_EOL_US_INT_SPACE__
		if (adpd_data->eol_measured_period >= 498) {
			adpd_data->eol_res_odr = (adpd_data->eol_counter * 1000 / adpd_data->eol_measured_period);
#else
		if (adpd_data->eol_measured_period >= 498000) {
			adpd_data->eol_res_odr = (adpd_data->eol_counter * 1000*1000 / adpd_data->eol_measured_period);
#endif

			if ((adpd_data->eol_res_odr - EOL_ODR_TARGET) > EOL_ALLOWABLE_ODR_ERROR || (EOL_ODR_TARGET - adpd_data->eol_res_odr) > EOL_ALLOWABLE_ODR_ERROR) {
				__adpd_32khzTrim();
				adpd_data->eol_counter = 0;
				adpd_data->eol_measured_period = 0;
				adpd_data->eol_state = ST_EOL_CLOCK_CAL_INIT;
			} else {
				adpd_data->eol_counter = 0;
				adpd_data->eol_measured_period = 0;
				adpd_data->eol_state = ST_EOL_CLOCK_CAL_DONE;
			}
		}
	}

	return adpd_data->eol_state;
}

static EOL_state __adpd_step_EOL_low_DC(unsigned *dataInA, unsigned *dataInB)
{
	unsigned short i = 0;

	if (adpd_data->eol_state == ST_EOL_CLOCK_CAL_DONE) {

		adpd_data->eol_state = ST_EOL_LOW_DC_INIT;
		__adpd_reg_write(adpd_data, ADPD_OP_MODE_ADDR, GLOBAL_OP_MODE_IDLE);

		__adpd_reg_write(adpd_data, ADPD_SAMPLING_FREQ_ADDR, 8000 / (EOL_ODR_TARGET<<3));
		__adpd_reg_write(adpd_data, ADPD_DEC_MODE_ADDR, 0x333);

		if (dataInA)
			__adpd_reg_write(adpd_data, ADPD_PULSE_PERIOD_A_ADDR, 0x0813);
		if (dataInB)
			__adpd_reg_write(adpd_data, ADPD_PULSE_PERIOD_B_ADDR, 0x0813);
		if (dataInA)
			__adpd_reg_write(adpd_data, ADPD_LED1_DRV_ADDR, 0x3031);
		if (dataInB)
			__adpd_reg_write(adpd_data, ADPD_LED2_DRV_ADDR, 0x3031);

		__adpd_reg_write(adpd_data, ADPD_LED_DISABLE_ADDR, 0x0000);

#ifdef __USE_EOL_ADC_OFFSET__
		__adpd_reg_write(adpd_data, ADPD_CH1_OFFSET_A_ADDR, 0x1fff);
		__adpd_reg_write(adpd_data, ADPD_CH2_OFFSET_A_ADDR, 0x1fff);
		__adpd_reg_write(adpd_data, ADPD_CH3_OFFSET_A_ADDR, 0x1fff);
		__adpd_reg_write(adpd_data, ADPD_CH4_OFFSET_A_ADDR, 0x1fff);

		__adpd_reg_write(adpd_data, ADPD_CH1_OFFSET_B_ADDR, 0x1fff);
		__adpd_reg_write(adpd_data, ADPD_CH2_OFFSET_B_ADDR, 0x1fff);
		__adpd_reg_write(adpd_data, ADPD_CH3_OFFSET_B_ADDR, 0x1fff);
		__adpd_reg_write(adpd_data, ADPD_CH4_OFFSET_B_ADDR, 0x1fff);
#else
		__adpd_reg_write(adpd_data, ADPD_CH1_OFFSET_A_ADDR, g_eol_ADCOffset[0]);
		__adpd_reg_write(adpd_data, ADPD_CH2_OFFSET_A_ADDR, g_eol_ADCOffset[1]);
		__adpd_reg_write(adpd_data, ADPD_CH3_OFFSET_A_ADDR, g_eol_ADCOffset[2]);
		__adpd_reg_write(adpd_data, ADPD_CH4_OFFSET_A_ADDR, g_eol_ADCOffset[3]);

		__adpd_reg_write(adpd_data, ADPD_CH1_OFFSET_B_ADDR, g_eol_ADCOffset[4]);
		__adpd_reg_write(adpd_data, ADPD_CH2_OFFSET_B_ADDR, g_eol_ADCOffset[5]);
		__adpd_reg_write(adpd_data, ADPD_CH3_OFFSET_B_ADDR, g_eol_ADCOffset[6]);
		__adpd_reg_write(adpd_data, ADPD_CH4_OFFSET_B_ADDR, g_eol_ADCOffset[7]);
#endif

		__adpd_reg_write(adpd_data, ADPD_AFE_CTRL_A_ADDR, 0x24D4);
		__adpd_reg_write(adpd_data, ADPD_AFE_CTRL_B_ADDR, 0x24D4);
		__adpd_reg_write(adpd_data, ADPD_SLOTA_GAIN_ADDR, 0x1C37);
		__adpd_reg_write(adpd_data, ADPD_SLOTA_AFE_CON_ADDR, 0xADA5);
		__adpd_reg_write(adpd_data, ADPD_TEST_PD_ADDR, 0x0040);
		__adpd_reg_write(adpd_data, ADPD_SLOTB_GAIN_ADDR, 0x1C37);
		__adpd_reg_write(adpd_data, ADPD_SLOTB_AFE_CON_ADDR, 0xADA5);
		__adpd_reg_write(adpd_data, ADPD_TEST_PD_ADDR, 0x0040);

		__adpd_reg_write(adpd_data, ADPD_INT_MASK_ADDR, 0x1ff);
		__adpd_reg_write(adpd_data, ADPD_INT_STATUS_ADDR, 0x80ff);
		__adpd_reg_write(adpd_data, ADPD_INT_MASK_ADDR, ~(0x0040) & 0x1ff);

		adpd_data->eol_counter = 0;
		__adpd_reg_write(adpd_data, ADPD_OP_MODE_ADDR, GLOBAL_OP_MODE_GO);
	}	else if (adpd_data->eol_state == ST_EOL_LOW_DC_INIT) {
		if (adpd_data->eol_counter++ > CNT_EOL_SKIP_SAMPLE) {
			adpd_data->eol_state = ST_EOL_LOW_DC_RUNNING;
			adpd_data->eol_counter = 0;
		}
	} else if (adpd_data->eol_state == ST_EOL_LOW_DC_RUNNING) {
		if (dataInA)
			g_dc_buffA[adpd_data->eol_counter] = *(dataInA) + *(dataInA+1) + *(dataInA+2) + *(dataInA+3);

		if (dataInB)
			g_dc_buffB[adpd_data->eol_counter] = *(dataInB) + *(dataInB+1) + *(dataInB+2) + *(dataInB+3);

		if (++adpd_data->eol_counter == CNT_SAMPLE_PER_EOL_CYCLE) {
			if (dataInA) {
				adpd_data->eol_res_red_low_dc = 0;
				adpd_data->eol_res_red_low_noise = 0;

				for (i = 0; i < CNT_SAMPLE_PER_EOL_CYCLE; i++)
					adpd_data->eol_res_red_low_dc += g_dc_buffA[i];

				adpd_data->eol_res_red_low_dc /= CNT_SAMPLE_PER_EOL_CYCLE;

				for (i = 0; i < CNT_SAMPLE_PER_EOL_CYCLE; i++)
					adpd_data->eol_res_red_low_noise += (g_dc_buffA[i] - adpd_data->eol_res_red_low_dc) * (g_dc_buffA[i] - adpd_data->eol_res_red_low_dc);

				adpd_data->eol_res_red_low_noise /= CNT_SAMPLE_PER_EOL_CYCLE;
			}
			if (dataInB) {
				adpd_data->eol_res_ir_low_dc = 0;
				adpd_data->eol_res_ir_low_noise = 0;

				for (i = 0; i < CNT_SAMPLE_PER_EOL_CYCLE; i++)
					adpd_data->eol_res_ir_low_dc += g_dc_buffB[i];

				adpd_data->eol_res_ir_low_dc /= CNT_SAMPLE_PER_EOL_CYCLE;

				for (i = 0; i < CNT_SAMPLE_PER_EOL_CYCLE; i++)
					adpd_data->eol_res_ir_low_noise += (g_dc_buffB[i] - adpd_data->eol_res_ir_low_dc) * (g_dc_buffB[i] - adpd_data->eol_res_ir_low_dc);

				adpd_data->eol_res_ir_low_noise /= CNT_SAMPLE_PER_EOL_CYCLE;
			}
			adpd_data->eol_state = ST_EOL_LOW_DC_DONE;
			adpd_data->eol_counter = 0;
		}
	}

	return adpd_data->eol_state;
}

static EOL_state __adpd_step_EOL_med_DC(unsigned *dataInA, unsigned *dataInB)
{
	unsigned short i = 0;

	if (adpd_data->eol_state == ST_EOL_LOW_DC_DONE) {
		adpd_data->eol_state = ST_EOL_MED_DC_INIT;

		__adpd_reg_write(adpd_data, ADPD_OP_MODE_ADDR, GLOBAL_OP_MODE_IDLE);

		if (dataInA)
			__adpd_reg_write(adpd_data, ADPD_LED1_DRV_ADDR, 0x3034);

		if (dataInB)
			__adpd_reg_write(adpd_data, ADPD_LED2_DRV_ADDR, 0x3034);

		__adpd_reg_write(adpd_data, ADPD_INT_MASK_ADDR, 0x1ff);
		__adpd_reg_write(adpd_data, ADPD_INT_STATUS_ADDR, 0x80ff);
		__adpd_reg_write(adpd_data, ADPD_INT_MASK_ADDR, ~(0x0040) & 0x1ff);

		adpd_data->eol_counter = 0;
		__adpd_reg_write(adpd_data, ADPD_OP_MODE_ADDR, GLOBAL_OP_MODE_GO);
	} else if (adpd_data->eol_state == ST_EOL_MED_DC_INIT) {
		if (adpd_data->eol_counter++ > CNT_EOL_SKIP_SAMPLE) {
			adpd_data->eol_state = ST_EOL_MED_DC_RUNNING;
			adpd_data->eol_counter = 0;
		}
	} else if (adpd_data->eol_state == ST_EOL_MED_DC_RUNNING) {
		if (dataInA)
			g_dc_buffA[adpd_data->eol_counter] = *(dataInA) + *(dataInA+1) + *(dataInA+2) + *(dataInA+3);

		if (dataInB)
			g_dc_buffB[adpd_data->eol_counter] = *(dataInB) + *(dataInB+1) + *(dataInB+2) + *(dataInB+3);

		if (++adpd_data->eol_counter == CNT_SAMPLE_PER_EOL_CYCLE) {
			if (dataInA) {
				adpd_data->eol_res_red_med_dc = 0;
				adpd_data->eol_res_red_med_noise = 0;

				for (i = 0; i < CNT_SAMPLE_PER_EOL_CYCLE; i++)
					adpd_data->eol_res_red_med_dc += g_dc_buffA[i];

				adpd_data->eol_res_red_med_dc /= CNT_SAMPLE_PER_EOL_CYCLE;

				for (i = 0; i < CNT_SAMPLE_PER_EOL_CYCLE; i++)
					adpd_data->eol_res_red_med_noise += (g_dc_buffA[i] - adpd_data->eol_res_red_med_dc) * (g_dc_buffA[i] - adpd_data->eol_res_red_med_dc);

				adpd_data->eol_res_red_med_noise /= CNT_SAMPLE_PER_EOL_CYCLE;
			}
			if (dataInB) {
				adpd_data->eol_res_ir_med_dc = 0;
				adpd_data->eol_res_ir_med_noise = 0;

				for (i = 0; i < CNT_SAMPLE_PER_EOL_CYCLE; i++)
					adpd_data->eol_res_ir_med_dc += g_dc_buffB[i];

				adpd_data->eol_res_ir_med_dc /= CNT_SAMPLE_PER_EOL_CYCLE;

				for (i = 0; i < CNT_SAMPLE_PER_EOL_CYCLE; i++)
					adpd_data->eol_res_ir_med_noise += (g_dc_buffB[i] - adpd_data->eol_res_ir_med_dc) * (g_dc_buffB[i] - adpd_data->eol_res_ir_med_dc);

				adpd_data->eol_res_ir_med_noise /= CNT_SAMPLE_PER_EOL_CYCLE;
			}
			adpd_data->eol_state = ST_EOL_MED_DC_DONE;
			adpd_data->eol_counter = 0;
		}
	}

	return adpd_data->eol_state;
}

static EOL_state __adpd_step_EOL_high_DC(unsigned *dataInA, unsigned *dataInB)
{
	unsigned short i = 0;

	if (adpd_data->eol_state == ST_EOL_MED_DC_DONE) {
		adpd_data->eol_state = ST_EOL_HIGH_DC_INIT;

		__adpd_reg_write(adpd_data, ADPD_OP_MODE_ADDR, GLOBAL_OP_MODE_IDLE);

		if (dataInA)
			__adpd_reg_write(adpd_data, ADPD_LED1_DRV_ADDR, 0x3037);

		if (dataInB)
			__adpd_reg_write(adpd_data, ADPD_LED2_DRV_ADDR, 0x3037);

		__adpd_reg_write(adpd_data, ADPD_INT_MASK_ADDR, 0x1ff);
		__adpd_reg_write(adpd_data, ADPD_INT_STATUS_ADDR, 0x80ff);
		__adpd_reg_write(adpd_data, ADPD_INT_MASK_ADDR, ~(0x0040) & 0x1ff);

		adpd_data->eol_counter = 0;
		__adpd_reg_write(adpd_data, ADPD_OP_MODE_ADDR, GLOBAL_OP_MODE_GO);
	} else if (adpd_data->eol_state == ST_EOL_HIGH_DC_INIT) {
		if (adpd_data->eol_counter++ > CNT_EOL_SKIP_SAMPLE) {
			adpd_data->eol_state = ST_EOL_HIGH_DC_RUNNING;
			adpd_data->eol_counter = 0;
		}
	} else if (adpd_data->eol_state == ST_EOL_HIGH_DC_RUNNING) {
		if (dataInA)
			g_dc_buffA[adpd_data->eol_counter] = *(dataInA) + *(dataInA+1) + *(dataInA+2) + *(dataInA+3);

		if (dataInB)
			g_dc_buffB[adpd_data->eol_counter] = *(dataInB) + *(dataInB+1) + *(dataInB+2) + *(dataInB+3);

		if (++adpd_data->eol_counter == CNT_SAMPLE_PER_EOL_CYCLE) {
			if (dataInA) {
				adpd_data->eol_res_red_high_dc = 0;
				adpd_data->eol_res_red_high_noise = 0;

				for (i = 0; i < CNT_SAMPLE_PER_EOL_CYCLE; i++)
					adpd_data->eol_res_red_high_dc += g_dc_buffA[i];
				adpd_data->eol_res_red_high_dc /= CNT_SAMPLE_PER_EOL_CYCLE;

				for (i = 0; i < CNT_SAMPLE_PER_EOL_CYCLE; i++)
					adpd_data->eol_res_red_high_noise += (g_dc_buffA[i] - adpd_data->eol_res_red_high_dc) * (g_dc_buffA[i] - adpd_data->eol_res_red_high_dc);

				adpd_data->eol_res_red_high_noise /= CNT_SAMPLE_PER_EOL_CYCLE;
			}
			if (dataInB) {
				adpd_data->eol_res_ir_high_dc = 0;
				adpd_data->eol_res_ir_high_noise = 0;

				for (i = 0; i < CNT_SAMPLE_PER_EOL_CYCLE; i++)
					adpd_data->eol_res_ir_high_dc += g_dc_buffB[i];

				adpd_data->eol_res_ir_high_dc /= CNT_SAMPLE_PER_EOL_CYCLE;

				for (i = 0; i < CNT_SAMPLE_PER_EOL_CYCLE; i++)
					adpd_data->eol_res_ir_high_noise += (g_dc_buffB[i] - adpd_data->eol_res_ir_high_dc) * (g_dc_buffB[i] - adpd_data->eol_res_ir_high_dc);

				adpd_data->eol_res_ir_high_noise /= CNT_SAMPLE_PER_EOL_CYCLE;
			}
			adpd_data->eol_state = ST_EOL_HIGH_DC_DONE;
			adpd_data->eol_counter = 0;
		}
	}

	return adpd_data->eol_state;
}

static EOL_state __adpd_step_EOL_AC(unsigned *dataInA, unsigned *dataInB)
{
	unsigned short i = 0;

	if (adpd_data->eol_state == ST_EOL_HIGH_DC_DONE) {
		adpd_data->eol_state = ST_EOL_AC_INIT;

		__adpd_reg_write(adpd_data, ADPD_OP_MODE_ADDR, GLOBAL_OP_MODE_IDLE);

		if (dataInA)
			__adpd_reg_write(adpd_data, ADPD_LED1_DRV_ADDR, 0x3035);
		if (dataInB)
			__adpd_reg_write(adpd_data, ADPD_LED2_DRV_ADDR, 0x3035);

		__adpd_reg_write(adpd_data, ADPD_INT_MASK_ADDR, 0x1ff);
		__adpd_reg_write(adpd_data, ADPD_INT_STATUS_ADDR, 0x80ff);
		__adpd_reg_write(adpd_data, ADPD_INT_MASK_ADDR, ~(0x0040) & 0x1ff);

		adpd_data->eol_counter = 0;
		__adpd_reg_write(adpd_data, ADPD_OP_MODE_ADDR, GLOBAL_OP_MODE_GO);
	} else if (adpd_data->eol_state == ST_EOL_AC_INIT) {
		if (adpd_data->eol_counter++ > CNT_EOL_SKIP_SAMPLE) {
			adpd_data->eol_state = ST_EOL_AC_RUNNING;
			adpd_data->eol_counter = 0;
		}
	} else if (adpd_data->eol_state == ST_EOL_AC_RUNNING) {
		if (dataInA)
			g_ac_buffA[adpd_data->eol_counter] = *(dataInA) + *(dataInA+1) + *(dataInA+2) + *(dataInA+3);

		if (dataInB)
			g_ac_buffB[adpd_data->eol_counter] = *(dataInB) + *(dataInB+1) + *(dataInB+2) + *(dataInB+3);

		if (++adpd_data->eol_counter == CNT_SAMPLE_PER_EOL_CYCLE) {
			if (dataInA) {
				adpd_data->eol_res_red_ac = 0;

				for (i = 0; i < CNT_SAMPLE_PER_EOL_CYCLE; i++)
					adpd_data->eol_res_red_ac += (g_dc_buffA[i]-g_ac_buffA[i]);

				adpd_data->eol_res_red_ac /= CNT_SAMPLE_PER_EOL_CYCLE;
			}
			if (dataInB) {
				adpd_data->eol_res_ir_ac = 0;

				for (i = 0; i < CNT_SAMPLE_PER_EOL_CYCLE; i++)
					adpd_data->eol_res_ir_ac += (g_dc_buffB[i]-g_ac_buffB[i]);

				adpd_data->eol_res_ir_ac /= CNT_SAMPLE_PER_EOL_CYCLE;
			}
			adpd_data->eol_state = ST_EOL_AC_DONE;
			adpd_data->eol_counter = 0;
		}
	}

	return adpd_data->eol_state;

}

#ifdef MELANIN_IDX_SUPPORT
static EOL_state __adpd_step_Melanin_DC(unsigned *dataInA, unsigned *dataInB)
{
	unsigned short i = 0;

	if (adpd_data->eol_state == ST_EOL_MELANIN_DC_DONE) {
		adpd_data->eol_state = ST_EOL_MELANIN_DC_INIT;

		__adpd_reg_write(adpd_data, ADPD_OP_MODE_ADDR, GLOBAL_OP_MODE_IDLE);

		if (dataInA)
			__adpd_reg_write(adpd_data, ADPD_LED1_DRV_ADDR, 0x3037);//130mA

		if (dataInB)
			__adpd_reg_write(adpd_data, ADPD_LED2_DRV_ADDR, 0x3037);//130mA

		__adpd_reg_write(adpd_data, ADPD_INT_MASK_ADDR, 0x1ff);
		__adpd_reg_write(adpd_data, ADPD_INT_STATUS_ADDR, 0x80ff);
		__adpd_reg_write(adpd_data, ADPD_INT_MASK_ADDR, ~(0x0040) & 0x1ff);

		adpd_data->eol_counter = 0;
		__adpd_reg_write(adpd_data, ADPD_OP_MODE_ADDR, GLOBAL_OP_MODE_GO);
	} else if (adpd_data->eol_state == ST_EOL_MELANIN_DC_INIT) {
		if (adpd_data->eol_counter++ > CNT_EOL_SKIP_SAMPLE) {
			adpd_data->eol_state = ST_EOL_MELANIN_DC_RUNNING;
			adpd_data->eol_counter = 0;
		}
	} else if (adpd_data->eol_state == ST_EOL_MELANIN_DC_RUNNING) {
		if (dataInA)
			g_dc_buffA[adpd_data->eol_counter] = *(dataInA+2) + *(dataInA+3);

		if (dataInB)
			g_dc_buffB[adpd_data->eol_counter] = *(dataInB+2) + *(dataInB+3);

		if (++adpd_data->eol_counter == CNT_SAMPLE_PER_EOL_CYCLE) {
			if (dataInA) {
				adpd_data->eol_res_melanin_red_dc = 0;
				adpd_data->eol_res_melanin_red_noise = 0;

				for (i = 0; i < CNT_SAMPLE_PER_EOL_CYCLE; i++)
					adpd_data->eol_res_melanin_red_dc += g_dc_buffA[i];

				adpd_data->eol_res_melanin_red_dc /= CNT_SAMPLE_PER_EOL_CYCLE;

				for (i = 0; i < CNT_SAMPLE_PER_EOL_CYCLE; i++)
					adpd_data->eol_res_melanin_red_noise += (g_dc_buffA[i] - adpd_data->eol_res_melanin_red_dc) * (g_dc_buffA[i] - adpd_data->eol_res_melanin_red_dc);

				adpd_data->eol_res_melanin_red_noise /= CNT_SAMPLE_PER_EOL_CYCLE;
			}
			if (dataInB) {
				adpd_data->eol_res_melanin_ir_dc = 0;
				adpd_data->eol_res_melanin_ir_noise = 0;

				for (i = 0; i < CNT_SAMPLE_PER_EOL_CYCLE; i++)
					adpd_data->eol_res_melanin_ir_dc += g_dc_buffB[i];

				adpd_data->eol_res_melanin_ir_dc /= CNT_SAMPLE_PER_EOL_CYCLE;

				for (i = 0; i < CNT_SAMPLE_PER_EOL_CYCLE; i++)
					adpd_data->eol_res_melanin_ir_noise += (g_dc_buffB[i] - adpd_data->eol_res_melanin_ir_dc) * (g_dc_buffB[i] - adpd_data->eol_res_melanin_ir_dc);

				adpd_data->eol_res_melanin_ir_noise /= CNT_SAMPLE_PER_EOL_CYCLE;
			}
			adpd_data->eol_state = ST_EOL_MELANIN_DC_DONE;
			adpd_data->eol_counter = 0;
		}
	}

	return adpd_data->eol_state;

}
#endif

static EOL_state __adpd_stepEOL(unsigned *dataInA, unsigned *dataInB)
{
	unsigned short i = 0;

	switch (adpd_data->eol_state) {
	case ST_EOL_IDLE:
		HRM_dbg("******************EOL started*****************\n");
	case ST_EOL_CLOCK_CAL_INIT:
		HRM_dbg("******************Clock Calibration Init*****************\n");
	case ST_EOL_CLOCK_CAL_RUNNING:
		HRM_dbg("******************Clock Calibration Running*****************\n");
		__adpd_step_EOL_clock_calibration(dataInA, dataInB);
		break;

	case ST_EOL_CLOCK_CAL_DONE:
		if (__adpd_osc_trim_efs_register_save(adpd_data) < 0)
			adpd_data->oscRegValue = __adpd_reg_read(adpd_data, ADPD_OSC32K_ADDR);
		HRM_dbg("******************Clock Calibration Finished : %dhz, 0x%04x*****************\n", adpd_data->eol_res_odr, adpd_data->oscRegValue);
	case ST_EOL_LOW_DC_INIT:
		HRM_dbg("******************Low DC Statistics Init : %d*****************\n", adpd_data->eol_counter);
	case ST_EOL_LOW_DC_RUNNING:
		HRM_dbg("******************Low DC Statistics Running : %d*****************\n", adpd_data->eol_counter);
		__adpd_step_EOL_low_DC(dataInA, dataInB);
		break;

	case ST_EOL_LOW_DC_DONE:
		HRM_dbg("****************** [Low DC statistics] Red DC : %d, Red Noise : %d, IR DC : %d, IR noise : %d *****************\n",
			adpd_data->eol_res_red_low_dc, adpd_data->eol_res_red_low_noise, adpd_data->eol_res_ir_low_dc, adpd_data->eol_res_ir_low_noise);
	case ST_EOL_MED_DC_INIT:
		HRM_dbg("******************Medium DC Statistics Init : %d*****************\n", adpd_data->eol_counter);
	case ST_EOL_MED_DC_RUNNING:
		HRM_dbg("******************Medium DC Statistics Running : %d*****************\n", adpd_data->eol_counter);
		__adpd_step_EOL_med_DC(dataInA, dataInB);
		break;

	case ST_EOL_MED_DC_DONE:
		HRM_dbg("****************** [Medium DC statistics] Red DC : %d, Red Noise : %d, IR DC : %d, IR noise : %d *****************\n",
			adpd_data->eol_res_red_med_dc, adpd_data->eol_res_red_med_noise, adpd_data->eol_res_ir_med_dc, adpd_data->eol_res_ir_med_noise);
	case ST_EOL_HIGH_DC_INIT:
		HRM_dbg("******************High DC Statistics Init : %d*****************\n", adpd_data->eol_counter);
	case ST_EOL_HIGH_DC_RUNNING:
		HRM_dbg("******************High DC Statistics Running : %d*****************\n", adpd_data->eol_counter);
		__adpd_step_EOL_high_DC(dataInA, dataInB);
		break;

	case ST_EOL_HIGH_DC_DONE:
		HRM_dbg("****************** [High DC statistics] Red DC : %d, Red Noise : %d, IR DC : %d, IR noise : %d *****************\n",
			adpd_data->eol_res_red_high_dc, adpd_data->eol_res_red_high_noise, adpd_data->eol_res_ir_high_dc, adpd_data->eol_res_ir_high_noise);
	case ST_EOL_AC_INIT:
		HRM_dbg("******************AC Statistics Init : %d*****************\n", adpd_data->eol_counter);
	case ST_EOL_AC_RUNNING:
		HRM_dbg("******************AC Statistics Running : %d*****************\n", adpd_data->eol_counter);
		__adpd_step_EOL_AC(dataInA, dataInB);
		break;

#ifdef MELANIN_IDX_SUPPORT
	case ST_EOL_AC_DONE:
		HRM_dbg("****************** [AC statistics] Red AC : %d, IR AC : %d *****************\n",
			adpd_data->eol_res_red_ac, adpd_data->eol_res_ir_ac);
		adpd_data->eol_state = ST_EOL_MELANIN_DC_INIT;
	case ST_EOL_MELANIN_DC_INIT:
		HRM_dbg("******************Melanin DC Statistics Init : %d*****************\n", adpd_data->eol_counter);
	case ST_EOL_MELANIN_DC_RUNNING:
		HRM_dbg("******************Melanin DC Statistics Running : %d*****************\n", adpd_data->eol_counter);
		__adpd_step_Melanin_DC(dataInA, dataInB);
		break;
	case ST_EOL_MELANIN_DC_DONE:
		HRM_dbg("****************** [ Melanin DC ]: Red DC :%d, IR DC : %d *****************\n",
			adpd_data->eol_res_melanin_red_dc, adpd_data->eol_res_melanin_ir_dc);
		adpd_data->eol_state = ST_EOL_DONE;
		adpd_data->eol_test_status = 1;
		break;

#else
	case ST_EOL_AC_DONE:
		HRM_dbg("****************** [AC statistics] Red AC : %d, IR AC : %d *****************\n",
			adpd_data->eol_res_red_ac, adpd_data->eol_res_ir_ac);
		adpd_data->eol_state = ST_EOL_DONE;
		adpd_data->eol_test_status = 1;
		break;
#endif

	case ST_EOL_DONE:
		HRM_dbg("******************EOL Completed*****************\n");
		for (i = 0; i < 32; i++)
			if ((g_eol_regNumbers[i] < 0x18 || g_eol_regNumbers[i] > 0x21) && (g_eol_regNumbers[i] != 0x4B))
				__adpd_reg_write(adpd_data, g_eol_regNumbers[i], g_org_regValues[i]);
		break;

	default:
		break;
	}
	return adpd_data->eol_state;
}

static EOL_state __adpd_getEOLstate(void)
{
	return adpd_data->eol_state;
}

static void __adpd_sample_data(struct adpd_device_data *adpd_data)
{
	unsigned short usr_mode = 0;
	unsigned short sub_mode = 0;
	signed short cnt = 0;
	unsigned int channel;
	unsigned short uncoated_ch3 = 0;
	unsigned short coated_ch4 = 0;
	unsigned short k = 0;

	unsigned uAgcRawDataA[4] = {0,};
	unsigned uAgcRawDataB[4] = {0,};

	unsigned int sum_slot_a = 0;
	unsigned int sum_slot_b = 0;

	adpd_data->sum_slot_a = 0;
	adpd_data->sum_slot_b = 0;
	for (channel = 0; channel < 8; channel++)
		adpd_data->slot_data[channel] = 0;

	usr_mode = atomic_read(&adpd_data->adpd_mode);
	sub_mode = GET_USR_SUB_MODE(usr_mode);

	switch (sub_mode) {
	case S_SAMP_XY_A:
		if (adpd_data->fifo_size < 4 || (adpd_data->fifo_size & 0x3)) {
			HRM_dbg("%s - Unexpected FIFO_SIZE=%d\n", __func__,	adpd_data->fifo_size);

			break;
		}

		if (GET_USR_MODE(usr_mode) != TIA_ADC_USR)	{
			if (GET_USR_MODE(usr_mode) == EOL_USR) {
				if (__adpd_getEOLstate() != ST_EOL_DONE) {
					for (cnt = 0; cnt < adpd_data->fifo_size; cnt += 4) {
						for (k = 0; k < 4; k++)
#ifdef __USE_EOL_RAW_DATA__
							uAgcRawDataA[k] = (unsigned)adpd_data->ptr_data_buffer[cnt + k];
#else
							uAgcRawDataA[k] = __adpd_normalized_data(adpd_data, 0, adpd_data->ptr_data_buffer[cnt + k]);
#endif

						if (cnt > 0)
#ifndef __USE_EOL_US_INT_SPACE__
							adpd_data->msec_eol_int_space = 0;
#else
							adpd_data->usec_eol_int_space = 0;
#endif

						__adpd_stepEOL(uAgcRawDataA, NULL);
					}
				}

				break;
			}

			for (cnt = 0; cnt < adpd_data->fifo_size; cnt += 4) {
				 sum_slot_a = 0;

#if defined(__USE_PD1234__)
				for (channel = 0; channel < 4; ++channel) {
					adpd_data->slot_data[channel] = adpd_data->ptr_data_buffer[cnt + channel];
					sum_slot_a += adpd_data->ptr_data_buffer[cnt + channel];
				}
#elif defined(__USE_PD123__)
				for (channel = 0; channel < 3; ++channel) {
					adpd_data->slot_data[channel] = adpd_data->ptr_data_buffer[cnt + channel];
					sum_slot_a += adpd_data->ptr_data_buffer[cnt + channel];
				}
				adpd_data->slot_data[channel] = adpd_data->ptr_data_buffer[cnt + channel];
				sum_slot_a += adpd_data->ptr_data_buffer[cnt + channel - 1];
#elif defined(__USE_PD234__)
				for (channel = 1; channel < 4; ++channel) {
					adpd_data->slot_data[channel] = adpd_data->ptr_data_buffer[cnt + channel];
					sum_slot_a += adpd_data->ptr_data_buffer[cnt + channel];
				}
#elif defined(__USE_PD34__)
			    for (channel = 2; channel < 4; ++channel) {
				    adpd_data->slot_data[channel] = adpd_data->ptr_data_buffer[cnt + channel];
					sum_slot_a += adpd_data->ptr_data_buffer[cnt + channel];
			    }
#elif defined(__USE_PD4__)
				for (channel = 3; channel < 4; ++channel) {
				    adpd_data->slot_data[channel] = adpd_data->ptr_data_buffer[cnt + channel];
					sum_slot_a += adpd_data->ptr_data_buffer[cnt + channel];
				}
#endif

				adpd_data->sum_slot_a = __adpd_normalized_data(adpd_data, 0, sum_slot_a);

#ifdef __USE_LOWPASS_FIR_FILTER__
				 adpd_data->fltr_sum_slot_a = adpd_data->sum_slot_a;
				 adpd_data->fltr_sum_slot_b = adpd_data->sum_slot_b;
				 __adpd_getLPFdata(&adpd_data->fltr_sum_slot_a, &adpd_data->fltr_sum_slot_b);
#endif

#ifdef __USE_TIME_DOMAIN_SNR__
#ifdef __USE_LOWPASS_FIR_FILTER__
				 __adpd_updateSnrAtTD(adpd_data->sum_slot_a, 0, adpd_data->fltr_sum_slot_a, 0);
#else
				 __adpd_updateSnrAtTD(adpd_data->sum_slot_a, 0);
#endif
#endif
				HRM_info("%s - fifo_size %d, sum_a - %d\n", __func__, adpd_data->fifo_size, adpd_data->sum_slot_a);
			}

			cnt -= 4;
			for (k = 0; k < 4; k++)
				uAgcRawDataA[k] = (unsigned)adpd_data->ptr_data_buffer[cnt+k];

			if (adpd_data->agc_mode == M_HRM)
			    __adpd_checkObjProximity(uAgcRawDataA, NULL);
			else {
				if (agc_is_enabled && __adpd_getAGCstate() != AgcStage_Done) {
					__adpd_stepAGC(uAgcRawDataA, NULL);
					__adpd_ClearFifo();
					break;
				}
#if defined(__USE_PD1234__) || defined(__USE_PD123__)
				if (uAgcRawDataA[0] >= (8000 - MARGIN_FOR_CH_SATURATION_PER_PULSE) * pulseA)
#elif defined(__USE_PD234__)
				if (uAgcRawDataA[1] >= (8000 - MARGIN_FOR_CH_SATURATION_PER_PULSE) * pulseA)
#elif defined(__USE_PD34__)
				if (uAgcRawDataA[2] >= (8000 - MARGIN_FOR_CH_SATURATION_PER_PULSE) * pulseA)
#elif defined(__USE_PD4__)
				if (uAgcRawDataA[3] >= (8000 - MARGIN_FOR_CH_SATURATION_PER_PULSE) * pulseA)
#endif
#if defined(__USE_PD1234__) || defined(__USE_PD234__) || defined(__USE_PD34__) || defined(__USE_PD123__) || defined(__USE_PD4__)
				{
					adpd_data->cnt_slotA_dc_saturation = (adpd_data->cnt_slotA_dc_saturation > 100) ? 100 : adpd_data->cnt_slotA_dc_saturation + 1;
					if (adpd_data->cnt_slotA_dc_saturation > CNT_THRESHOLD_DC_SATURATION) {
						HRM_info("%s - Slot A saturation!!! (ChA %u >= %u )\n", __func__,
#if defined(__USE_PD1234__) || defined(__USE_PD123__)
							uAgcRawDataA[0], (8000 - MARGIN_FOR_CH_SATURATION_PER_PULSE) * pulseA);
#elif defined(__USE_PD234__)
							uAgcRawDataA[1], (8000 - MARGIN_FOR_CH_SATURATION_PER_PULSE) * pulseA);
#elif defined(__USE_PD34__)
							uAgcRawDataA[2], (8000 - MARGIN_FOR_CH_SATURATION_PER_PULSE) * pulseA);
#elif defined(__USE_PD4__)
							uAgcRawDataA[3], (8000 - MARGIN_FOR_CH_SATURATION_PER_PULSE) * pulseA);
#endif
						__adpd_ClearSaturation(0);
					}
				} else
					adpd_data->cnt_slotA_dc_saturation = (adpd_data->cnt_slotA_dc_saturation > 0) ? adpd_data->cnt_slotA_dc_saturation - 1 : 0;
#endif

			}
		} else {
			for (cnt = 0; cnt < adpd_data->fifo_size; cnt += 4) {
				if (Is_TIA_ADC_Dark_Calibrated < TIA_ADC_MODE_DARK_CALIBRATION_CNT && Is_TIA_ADC_Dark_Calibrated > 1) {
					for (k = 4; k < 8; k++) {
						if (rawDarkCh[k] < adpd_data->ptr_data_buffer[cnt + k] || !rawDarkCh[k])
							rawDarkCh[k] = adpd_data->ptr_data_buffer[cnt + k];
						HRM_info("%s - SLOT A(FloatA mode) : rawDataCh%d : %d, rawDarkCh%d : %d\n", __func__, k, adpd_data->ptr_data_buffer[cnt + k], k, rawDarkCh[k]);
					}
					Is_TIA_ADC_Dark_Calibrated++;
					continue;

				} else if (Is_TIA_ADC_Dark_Calibrated >= TIA_ADC_MODE_DARK_CALIBRATION_CNT && Is_TIA_ADC_Dark_Calibrated < 100) {
					Is_TIA_ADC_Dark_Calibrated = 100;
					continue;

				} else if (Is_TIA_ADC_Dark_Calibrated < TIA_ADC_MODE_DARK_CALIBRATION_CNT) {
					Is_TIA_ADC_Dark_Calibrated++;
					continue;

				} else if (Is_TIA_ADC_Dark_Calibrated == 100) {
					__adpd_reg_write(adpd_data, ADPD_OP_MODE_ADDR, GLOBAL_OP_MODE_IDLE);
					__adpd_reg_write(adpd_data, ADPD_PD_SELECT_ADDR, 0x0441);
					__adpd_reg_write(adpd_data, ADPD_OP_MODE_ADDR, GLOBAL_OP_MODE_GO);

					Is_TIA_ADC_Dark_Calibrated++;
					continue;
				}

				for (channel = 0; channel < 4; ++channel) {
					HRM_info("%s - SLOT A(Float mode) : rawDataCh%d : %d, rawDarkCh%d : %d\n", __func__,	channel, adpd_data->ptr_data_buffer[cnt + channel], channel, rawDarkCh[channel]);
					rawDataCh[channel] = adpd_data->ptr_data_buffer[cnt + channel] - rawDarkCh[channel];
				}

				uncoated_ch3 = rawDataCh[2];
				coated_ch4 = rawDataCh[3];

				for (channel = 0; channel < 4; ++channel)
					adpd_data->slot_data[channel] = rawDataCh[channel];

				adpd_data->sum_slot_a = (((unsigned)coated_ch4) * 100) / uncoated_ch3;

				HRM_info("%s - adpd143 SLOT A(TIA mode) uncoated_ch3:%d, coated_ch4=%d\n", __func__, uncoated_ch3, coated_ch4);
			}
		}
		break;

	case S_SAMP_XY_AB:
		if ((adpd_data->fifo_size < 8 || (adpd_data->fifo_size & 0x7)) && GET_USR_MODE(usr_mode) != TIA_ADC_USR) {
			HRM_dbg("%s - Unexpected FIFO_SIZE=%d\n", __func__,
				adpd_data->fifo_size);
			break;
		}

		if (GET_USR_MODE(usr_mode) != TIA_ADC_USR) {
			if (GET_USR_MODE(usr_mode) == EOL_USR) {
				if (__adpd_getEOLstate() != ST_EOL_DONE) {
					for (cnt = 0; cnt < adpd_data->fifo_size; cnt += 8) {
						for (k = 0; k < 4; k++)
#ifdef __USE_EOL_RAW_DATA__
							uAgcRawDataA[k] = (unsigned)adpd_data->ptr_data_buffer[cnt + k];
#else
							uAgcRawDataA[k] = __adpd_normalized_data(adpd_data, 0, adpd_data->ptr_data_buffer[cnt + k]);
#endif

						for (k = 4; k < 8; k++)
#ifdef __USE_EOL_RAW_DATA__
							uAgcRawDataB[k-4] = (unsigned)adpd_data->ptr_data_buffer[cnt + k];
#else
							uAgcRawDataB[k-4] = __adpd_normalized_data(adpd_data, 1, adpd_data->ptr_data_buffer[cnt + k]);
#endif

						if (cnt > 0)
#ifndef __USE_EOL_US_INT_SPACE__
							adpd_data->msec_eol_int_space = 0;
#else
							adpd_data->usec_eol_int_space = 0;
#endif

						__adpd_stepEOL(uAgcRawDataA, uAgcRawDataB);
					}
				}

				break;
			}

			for (cnt = 0; cnt < adpd_data->fifo_size; cnt += 8) {
				sum_slot_a = 0;
				sum_slot_b = 0;

				for (channel = 0; channel < 4; ++channel) {
					if (fifo_is_32bit)
						uAgcRawDataA[channel] = (((unsigned)adpd_data->ptr_data_buffer[cnt*2+channel*2+1])<<16)+(unsigned)adpd_data->ptr_data_buffer[cnt*2+channel*2];
					else
						uAgcRawDataA[channel] = (unsigned)adpd_data->ptr_data_buffer[cnt+channel];

					adpd_data->slot_data[channel] = adpd_data->ptr_data_buffer[cnt + channel];

#if defined(__USE_PD1234__)
					sum_slot_a += adpd_data->ptr_data_buffer[cnt + channel];
#elif defined(__USE_PD123__)
					if (channel < 3)
						sum_slot_a += adpd_data->ptr_data_buffer[cnt + channel];
					else
						sum_slot_a += adpd_data->ptr_data_buffer[cnt + channel - 1];
#elif defined(__USE_PD234__)
					if (channel > 0)
						sum_slot_a += adpd_data->ptr_data_buffer[cnt + channel];
#elif defined(__USE_PD34__)
					if (channel > 1)
						sum_slot_a += adpd_data->ptr_data_buffer[cnt + channel];
#elif defined(__USE_PD4__)
					if (channel > 2)
						sum_slot_a += adpd_data->ptr_data_buffer[cnt + channel];
#endif
					HRM_info("%s - SLOT A(HRM mode) : rawDataCh%d : %d\n", __func__, channel, adpd_data->ptr_data_buffer[cnt + channel]);
				}

				adpd_data->sum_slot_a = __adpd_normalized_data(adpd_data, 0, sum_slot_a);

				HRM_info("%s - fifo_size %d, sum_a - %d\n", __func__, adpd_data->fifo_size, adpd_data->sum_slot_a);

				for (channel = 4; channel < 8; ++channel) {

					if (fifo_is_32bit)
						uAgcRawDataB[channel-4] = (((unsigned)adpd_data->ptr_data_buffer[cnt*2+channel*2+1])<<16)+(unsigned)adpd_data->ptr_data_buffer[cnt*2+channel*2];
					else
						uAgcRawDataB[channel-4] = (unsigned)adpd_data->ptr_data_buffer[cnt+channel];

					adpd_data->slot_data[channel] = adpd_data->ptr_data_buffer[cnt + channel];

#if defined(__USE_PD1234__)
					sum_slot_b += adpd_data->ptr_data_buffer[cnt + channel];
#elif defined(__USE_PD123__)
					if (channel < 7)
						sum_slot_b += adpd_data->ptr_data_buffer[cnt + channel];
					else
						sum_slot_b += adpd_data->ptr_data_buffer[cnt + channel - 1];
#elif defined(__USE_PD234__)
					if (channel > 4)
						sum_slot_b += adpd_data->ptr_data_buffer[cnt + channel];
#elif defined(__USE_PD34__)
					if (channel > 5)
						sum_slot_b += adpd_data->ptr_data_buffer[cnt + channel];
#elif defined(__USE_PD4__)
					if (channel > 6)
						sum_slot_b += adpd_data->ptr_data_buffer[cnt + channel];
#endif
					HRM_info("%s - SLOT B(HRM mode) : rawDataCh%d : %d\n", __func__, channel, adpd_data->ptr_data_buffer[cnt + channel]);
				}

				adpd_data->sum_slot_b = __adpd_normalized_data(adpd_data, 1, sum_slot_b);

				HRM_info("%s - fifo_size %d, sum_b - %d\n", __func__, adpd_data->fifo_size, adpd_data->sum_slot_b);
#ifdef __USE_LOWPASS_FIR_FILTER__
				adpd_data->fltr_sum_slot_a = adpd_data->sum_slot_a;
				adpd_data->fltr_sum_slot_b = adpd_data->sum_slot_b;
				__adpd_getLPFdata(&adpd_data->fltr_sum_slot_a, &adpd_data->fltr_sum_slot_b);
#endif

#ifdef __USE_TIME_DOMAIN_SNR__
#ifdef __USE_LOWPASS_FIR_FILTER__
				__adpd_updateSnrAtTD (adpd_data->sum_slot_a, adpd_data->sum_slot_b, adpd_data->fltr_sum_slot_a, adpd_data->fltr_sum_slot_b);
#else
				__adpd_updateSnrAtTD (adpd_data->sum_slot_a, adpd_data->sum_slot_b);
#endif
#endif
			}

			if (adpd_data->agc_mode == M_HRM)
				__adpd_checkObjProximity(uAgcRawDataA, uAgcRawDataB);
			else {
				if (agc_is_enabled && __adpd_getAGCstate() != AgcStage_Done) {
					__adpd_stepAGC(uAgcRawDataA, uAgcRawDataB);
					__adpd_ClearFifo();
					break;
				}

#if defined(__USE_PD1234__) || defined(__USE_PD123__)
				if (uAgcRawDataB[0] >= (8000 - MARGIN_FOR_CH_SATURATION_PER_PULSE) * pulseB)
#elif defined(__USE_PD234__)
				if (uAgcRawDataB[1] >= (8000 - MARGIN_FOR_CH_SATURATION_PER_PULSE) * pulseB)
#elif defined(__USE_PD34__)
				if (uAgcRawDataB[2] >= (8000 - MARGIN_FOR_CH_SATURATION_PER_PULSE) * pulseB)
#elif defined(__USE_PD4__)
				if (uAgcRawDataB[3] >= (8000 - MARGIN_FOR_CH_SATURATION_PER_PULSE) * pulseB)
#endif
#if defined(__USE_PD1234__) || defined(__USE_PD234__) || defined(__USE_PD34__) || defined(__USE_PD123__) || defined(__USE_PD4__)
				{
					adpd_data->cnt_slotB_dc_saturation = (adpd_data->cnt_slotB_dc_saturation > 100) ? 100 : adpd_data->cnt_slotB_dc_saturation + 1;
					if (adpd_data->cnt_slotB_dc_saturation > CNT_THRESHOLD_DC_SATURATION) {
						HRM_info("%s - Slot B saturation!!! (ChB %u >= %u )\n", __func__,
#if defined(__USE_PD1234__) || defined(__USE_PD123__)
						uAgcRawDataB[0], (8000 - MARGIN_FOR_CH_SATURATION_PER_PULSE) * pulseB);
#elif defined(__USE_PD234__)
						uAgcRawDataB[1], (8000 - MARGIN_FOR_CH_SATURATION_PER_PULSE) * pulseB);
#elif defined(__USE_PD34__)
						uAgcRawDataB[2], (8000 - MARGIN_FOR_CH_SATURATION_PER_PULSE) * pulseB);
#elif defined(__USE_PD4__)
						uAgcRawDataB[3], (8000 - MARGIN_FOR_CH_SATURATION_PER_PULSE) * pulseB);
#endif
						__adpd_ClearSaturation(1);
					}
				} else
					adpd_data->cnt_slotB_dc_saturation = (adpd_data->cnt_slotB_dc_saturation > 0) ? adpd_data->cnt_slotB_dc_saturation - 1 : 0;
#endif

#if defined(__USE_PD1234__) || defined(__USE_PD123__)
				if (uAgcRawDataA[0] >= (8000 - MARGIN_FOR_CH_SATURATION_PER_PULSE) * pulseA)
#elif defined(__USE_PD234__)
				if (uAgcRawDataA[1] >= (8000 - MARGIN_FOR_CH_SATURATION_PER_PULSE) * pulseA)
#elif defined(__USE_PD34__)
				if (uAgcRawDataA[2] >= (8000 - MARGIN_FOR_CH_SATURATION_PER_PULSE) * pulseA)
#elif defined(__USE_PD4__)
				if (uAgcRawDataA[3] >= (8000 - MARGIN_FOR_CH_SATURATION_PER_PULSE) * pulseA)
#endif
#if defined(__USE_PD1234__) || defined(__USE_PD234__) || defined(__USE_PD34__) || defined(__USE_PD123__) || defined(__USE_PD4__)
				{
					adpd_data->cnt_slotA_dc_saturation = (adpd_data->cnt_slotA_dc_saturation > 100) ? 100 : adpd_data->cnt_slotA_dc_saturation + 1;
					if (adpd_data->cnt_slotA_dc_saturation > CNT_THRESHOLD_DC_SATURATION) {
						HRM_info("%s - Slot A saturation!!! (ChA %u >= %u )\n", __func__,
#if defined(__USE_PD1234__) || defined(__USE_PD123__)
							uAgcRawDataA[0], (8000 - MARGIN_FOR_CH_SATURATION_PER_PULSE) * pulseA);
#elif defined(__USE_PD234__)
							uAgcRawDataA[1], (8000 - MARGIN_FOR_CH_SATURATION_PER_PULSE) * pulseA);
#elif defined(__USE_PD34__)
							uAgcRawDataA[2], (8000 - MARGIN_FOR_CH_SATURATION_PER_PULSE) * pulseA);
#elif defined(__USE_PD4__)
							uAgcRawDataA[3], (8000 - MARGIN_FOR_CH_SATURATION_PER_PULSE) * pulseA);
#endif
						__adpd_ClearSaturation(0);
					}
				} else
					adpd_data->cnt_slotA_dc_saturation = (adpd_data->cnt_slotA_dc_saturation > 0) ? adpd_data->cnt_slotA_dc_saturation - 1 : 0;
#endif

			}

		} else {
			for (cnt = 0; cnt < adpd_data->fifo_size; cnt += 8) {
				if (Is_TIA_ADC_Dark_Calibrated < TIA_ADC_MODE_DARK_CALIBRATION_CNT && Is_TIA_ADC_Dark_Calibrated > 1) {
					for (k = 0; k < 4; k++) {
						if (rawDarkCh[k] < adpd_data->ptr_data_buffer[cnt + k] || !rawDarkCh[k])
							rawDarkCh[k] = adpd_data->ptr_data_buffer[cnt + k];
						HRM_info("%s - SLOT A(TIA mode) : rawDataCh%d : %d, rawDarkCh%d : %d\n", __func__,	k, adpd_data->ptr_data_buffer[cnt + k], k, rawDarkCh[k]);
					}
					Is_TIA_ADC_Dark_Calibrated++;

				} else if (Is_TIA_ADC_Dark_Calibrated >= TIA_ADC_MODE_DARK_CALIBRATION_CNT && Is_TIA_ADC_Dark_Calibrated < 100) {

					rawDarkCh[0] -= 1070;
					rawDarkCh[1] -= 205;
					rawDarkCh[2] -= 210;
					rawDarkCh[3] -= 230;

					Is_TIA_ADC_Dark_Calibrated = 100;

				} else if (Is_TIA_ADC_Dark_Calibrated < TIA_ADC_MODE_DARK_CALIBRATION_CNT) {
					Is_TIA_ADC_Dark_Calibrated++;
				}
				if (Is_Float_Dark_Calibrated < FLOAT_MODE_DARK_CALIBRATION_CNT && Is_Float_Dark_Calibrated > 1) {
					for (k = 4; k < 8; k++) {
						if (rawDarkCh[k] < adpd_data->ptr_data_buffer[cnt + k] || !rawDarkCh[k])
							rawDarkCh[k] = adpd_data->ptr_data_buffer[cnt + k];
						HRM_info("%s - SLOT B(Float mode) : rawDataCh%d : %d, rawDarkCh%d : %d\n", __func__, k, adpd_data->ptr_data_buffer[cnt + k], k, rawDarkCh[k]);
					}
					Is_Float_Dark_Calibrated++;
					continue;

				} else if (Is_Float_Dark_Calibrated >= FLOAT_MODE_DARK_CALIBRATION_CNT && Is_Float_Dark_Calibrated < 100) {
					Is_Float_Dark_Calibrated = 100;
					continue;

				} else if (Is_Float_Dark_Calibrated < FLOAT_MODE_DARK_CALIBRATION_CNT) {
					Is_Float_Dark_Calibrated++;
					continue;

				} else if (Is_Float_Dark_Calibrated == 100) {
					__adpd_reg_write(adpd_data, ADPD_OP_MODE_ADDR, GLOBAL_OP_MODE_IDLE);
					__adpd_reg_write(adpd_data, ADPD_PD_SELECT_ADDR, 0x0441);
					__adpd_reg_write(adpd_data, ADPD_OP_MODE_ADDR, GLOBAL_OP_MODE_GO);

					Is_Float_Dark_Calibrated++;
					continue;
				}

				if (adpd_data->ptr_data_buffer[cnt + 4] > 12000 || adpd_data->ptr_data_buffer[cnt + 5] > 12000 ||
					adpd_data->ptr_data_buffer[cnt + 6] > 12000 || adpd_data->ptr_data_buffer[cnt + 7] > 12000) {
					cnt -= 4;
					continue;
				}

				for (channel = 0; channel < 4; ++channel) {
					HRM_info("%s - SLOT A(TIA mode) : rawDataCh%d : %d, rawDarkCh%d : %d\n", __func__,	channel, adpd_data->ptr_data_buffer[cnt + channel], channel, rawDarkCh[channel]);
					rawDataCh[channel] = (rawDarkCh[channel] > adpd_data->ptr_data_buffer[cnt + channel]) ? (rawDarkCh[channel] - adpd_data->ptr_data_buffer[cnt + channel]) : 0;
				}
				mutex_lock(&adpd_data->flickerdatalock);

				for (channel = 0; channel < 4; ++channel) {
					if (channel == 2)
						uncoated_ch3 = rawDataCh[2];

					if (channel == 3)
						coated_ch4 = rawDataCh[3];

					rawDataCh[channel] *= 3;
					adpd_data->slot_data[channel] = rawDataCh[channel];
					if (adpd_data->flicker_data_cnt * 8 < FLICKER_DATA_CNT)
						adpd_data->flicker_data[adpd_data->flicker_data_cnt * 8 + channel] = (int)rawDataCh[channel];

					HRM_info("%s - flicker_data : %d, cnt : %d", __func__, adpd_data->flicker_data[adpd_data->flicker_data_cnt * 8 + channel], adpd_data->flicker_data_cnt * 8 + channel);

					HRM_info("%s - (After Dark Cal) adpd143 SLOT A %d:%d\n", __func__,	channel, rawDataCh[channel]);
				}

				if (uncoated_ch3 == 0) {
					uncoated_ch3++;
					coated_ch4++;
				}
				adpd_data->sum_slot_a = (((unsigned)coated_ch4) * 100) / uncoated_ch3;

				HRM_info("%s - ch %u = %d, sum - %d\n", __func__, (unsigned)coated_ch4, uncoated_ch3, adpd_data->sum_slot_a);

				for (channel = 4; channel < 8; ++channel) {
					HRM_info("%s - SLOT B(float mode) : rawDataCh%d : %d, rawDarkCh%d : %d\n", __func__, channel, adpd_data->ptr_data_buffer[cnt + channel], channel, rawDarkCh[channel]);

					rawDataCh[channel] = adpd_data->ptr_data_buffer[cnt + channel] - rawDarkCh[channel];
					rawDataCh[channel] = ((int)rawDataCh[channel] < 0) ? 0 : rawDataCh[channel];

					if (rawDataCh[channel] >= (FLOAT_MODE_SATURATION_CONTROL_CODE-rawDarkCh[channel]))
						rawDataCh[channel] = FLOAT_MODE_SATURATION_CONTROL_CODE;
				}

				for (channel = 4; channel < 8; ++channel) {
					if (GET_USR_MODE(usr_mode) == TIA_ADC_USR) {
						if (channel == 6)
							uncoated_ch3 = rawDataCh[6];
						if (channel == 7)
							coated_ch4 = rawDataCh[7];

						adpd_data->slot_data[channel] = rawDataCh[channel];
						if (adpd_data->flicker_data_cnt * 8 < FLICKER_DATA_CNT)
							adpd_data->flicker_data[adpd_data->flicker_data_cnt * 8 + channel] = (int)rawDataCh[channel];

						HRM_info("%s - flicker_data : %d, cnt : %d", __func__, adpd_data->flicker_data[adpd_data->flicker_data_cnt * 8 + channel], adpd_data->flicker_data_cnt * 8 + channel);
						HRM_info("%s - (After Dark Cal) adpd143 SLOT B %d:%d\n", __func__,	channel, rawDataCh[channel]);
					}
				}

				if (adpd_data->flicker_data_cnt * 8 < FLICKER_DATA_CNT)
					adpd_data->flicker_data_cnt++;

				mutex_unlock(&adpd_data->flickerdatalock);

				if (GET_USR_MODE(usr_mode) == TIA_ADC_USR) {
					if (uncoated_ch3 == 0) {
						uncoated_ch3++;
						coated_ch4++;
					}
					adpd_data->sum_slot_b = (((unsigned)coated_ch4) * 100) / uncoated_ch3;

					HRM_info("%s - ch %u = %d, sum - %d\n", __func__, (unsigned)coated_ch4, uncoated_ch3, adpd_data->sum_slot_b);
				}
			}
		}
		break;

	case S_SAMP_XY_B:
		if (adpd_data->fifo_size < 4 || (adpd_data->fifo_size & 0x3)) {
			HRM_dbg("%s - Unexpected FIFO_SIZE=%d\n", __func__,
				adpd_data->fifo_size);

			break;
		}

		if (GET_USR_MODE(usr_mode) != TIA_ADC_USR) {
			if (GET_USR_MODE(usr_mode) == EOL_USR) {
				if (__adpd_getEOLstate() != ST_EOL_DONE) {
					for (cnt = 0; cnt < adpd_data->fifo_size; cnt += 4) {
						for (k = 0; k < 4; k++)
#ifdef __USE_EOL_RAW_DATA__
							uAgcRawDataB[k] = (unsigned)adpd_data->ptr_data_buffer[cnt + k];
#else
							uAgcRawDataB[k] = __adpd_normalized_data(adpd_data, 1, adpd_data->ptr_data_buffer[cnt + k]);
#endif

						if (cnt > 0)
#ifndef __USE_EOL_US_INT_SPACE__
							adpd_data->msec_eol_int_space = 0;
#else
							adpd_data->usec_eol_int_space = 0;
#endif
						__adpd_stepEOL(NULL, uAgcRawDataB);
					}
				}

				break;
			}

			for (cnt = 0; cnt < adpd_data->fifo_size; cnt += 4) {
				 sum_slot_b = 0;
#if defined(__USE_PD1234__)
				 for (channel = 0; channel < 4; ++channel) {
					 adpd_data->slot_data[channel+4] = adpd_data->ptr_data_buffer[cnt + channel];
					 sum_slot_b += adpd_data->ptr_data_buffer[cnt + channel];
				 }
#elif defined(__USE_PD123__)
				for (channel = 0; channel < 3; ++channel) {
					adpd_data->slot_data[channel+4] = adpd_data->ptr_data_buffer[cnt + channel];
					sum_slot_b += adpd_data->ptr_data_buffer[cnt + channel];
				}
				adpd_data->slot_data[channel+4] = adpd_data->ptr_data_buffer[cnt + channel];
				sum_slot_b += adpd_data->ptr_data_buffer[cnt + channel - 1];
#elif defined(__USE_PD234__)
				for (channel = 1; channel < 4; ++channel) {
					adpd_data->slot_data[channel+4] = adpd_data->ptr_data_buffer[cnt + channel];
					sum_slot_b += adpd_data->ptr_data_buffer[cnt + channel];
				}
#elif defined(__USE_PD34__)
				for (channel = 2; channel < 4; ++channel) {
					adpd_data->slot_data[channel+4] = adpd_data->ptr_data_buffer[cnt + channel];
					sum_slot_b += adpd_data->ptr_data_buffer[cnt + channel];
				}
#elif defined(__USE_PD4__)
				for (channel = 3; channel < 4; ++channel) {
					adpd_data->slot_data[channel+4] = adpd_data->ptr_data_buffer[cnt + channel];
					sum_slot_b += adpd_data->ptr_data_buffer[cnt + channel];
				}

#endif
				adpd_data->sum_slot_b = __adpd_normalized_data(adpd_data, 1, sum_slot_b);

#ifdef __USE_LOWPASS_FIR_FILTER__
				adpd_data->fltr_sum_slot_a = adpd_data->sum_slot_a;
				adpd_data->fltr_sum_slot_b = adpd_data->sum_slot_b;
				__adpd_getLPFdata(&adpd_data->fltr_sum_slot_a, &adpd_data->fltr_sum_slot_b);
#endif

#ifdef __USE_TIME_DOMAIN_SNR__
#ifdef __USE_LOWPASS_FIR_FILTER__
				__adpd_updateSnrAtTD(0, adpd_data->sum_slot_b, 0, adpd_data->fltr_sum_slot_b);
#else
				__adpd_updateSnrAtTD(0, adpd_data->sum_slot_b);
#endif
#endif
				HRM_info("%s - fifo_size %d, sum_b - %d\n", __func__, adpd_data->fifo_size, adpd_data->sum_slot_b);
			}

			cnt -= 4;
			for (k = 0; k < 4; k++)
				uAgcRawDataB[k] = (unsigned)adpd_data->ptr_data_buffer[cnt+k];

			if (adpd_data->agc_mode == M_HRM)
				__adpd_checkObjProximity(NULL, uAgcRawDataB);
			else {
				if (agc_is_enabled && __adpd_getAGCstate() != AgcStage_Done) {
					__adpd_stepAGC(NULL, uAgcRawDataB);
					__adpd_ClearFifo();
					break;
				}
#if defined(__USE_PD1234__) || defined(__USE_PD123__)
				if (uAgcRawDataB[0] >= (8000 - MARGIN_FOR_CH_SATURATION_PER_PULSE) * pulseB)
#elif defined(__USE_PD234__)
				if (uAgcRawDataB[1] >= (8000 - MARGIN_FOR_CH_SATURATION_PER_PULSE) * pulseB)
#elif defined(__USE_PD34__)
				if (uAgcRawDataB[2] >= (8000 - MARGIN_FOR_CH_SATURATION_PER_PULSE) * pulseB)
#elif defined(__USE_PD4__)
				if (uAgcRawDataB[3] >= (8000 - MARGIN_FOR_CH_SATURATION_PER_PULSE) * pulseB)
#endif
#if defined(__USE_PD1234__) || defined(__USE_PD234__) || defined(__USE_PD34__) || defined(__USE_PD123__) || defined(__USE_PD4__)
				{
					adpd_data->cnt_slotB_dc_saturation = (adpd_data->cnt_slotB_dc_saturation > 100) ? 100 : adpd_data->cnt_slotB_dc_saturation + 1;
					if (adpd_data->cnt_slotB_dc_saturation > CNT_THRESHOLD_DC_SATURATION) {
						HRM_info("%s - Slot B saturation!!! (ChB %u >= %u )\n", __func__,
#if defined(__USE_PD1234__) || defined(__USE_PD123__)
						uAgcRawDataB[0], (8000 - MARGIN_FOR_CH_SATURATION_PER_PULSE) * pulseB);
#elif defined(__USE_PD234__)
						uAgcRawDataB[1], (8000 - MARGIN_FOR_CH_SATURATION_PER_PULSE) * pulseB);
#elif defined(__USE_PD34__)
						uAgcRawDataB[2], (8000 - MARGIN_FOR_CH_SATURATION_PER_PULSE) * pulseB);
#elif defined(__USE_PD4__)
						uAgcRawDataB[3], (8000 - MARGIN_FOR_CH_SATURATION_PER_PULSE) * pulseB);
#endif
						__adpd_ClearSaturation(1);
					}
				} else
					adpd_data->cnt_slotB_dc_saturation = (adpd_data->cnt_slotB_dc_saturation > 0) ? adpd_data->cnt_slotB_dc_saturation - 1 : 0;
#endif
			}
		} else {
			for (cnt = 0; cnt < adpd_data->fifo_size; cnt += 4) {
				if (Is_Float_Dark_Calibrated < FLOAT_MODE_DARK_CALIBRATION_CNT && Is_Float_Dark_Calibrated > 1) {
					for (k = 4; k < 8; k++) {
						if (rawDarkCh[k] < adpd_data->ptr_data_buffer[cnt + k - 4] || !rawDarkCh[k])
							rawDarkCh[k] = adpd_data->ptr_data_buffer[cnt + k - 4];
					}
					Is_Float_Dark_Calibrated++;
					continue;

				} else if (Is_Float_Dark_Calibrated >= FLOAT_MODE_DARK_CALIBRATION_CNT && Is_Float_Dark_Calibrated < 100) {
					Is_Float_Dark_Calibrated = 100;
					continue;

				} else if (Is_Float_Dark_Calibrated < FLOAT_MODE_DARK_CALIBRATION_CNT) {
					Is_Float_Dark_Calibrated++;
					continue;

				} else if (Is_Float_Dark_Calibrated == 100) {
					__adpd_reg_write(adpd_data, ADPD_OP_MODE_ADDR, GLOBAL_OP_MODE_IDLE);
					__adpd_reg_write(adpd_data, ADPD_PD_SELECT_ADDR, 0x0441);
					__adpd_reg_write(adpd_data, ADPD_OP_MODE_ADDR, GLOBAL_OP_MODE_GO);
					Is_Float_Dark_Calibrated++;
				}

				for (channel = 0; channel < 4; ++channel) {
					rawDataCh[channel] = adpd_data->ptr_data_buffer[cnt + channel] - rawDarkCh[channel];
					rawDataCh[channel] = ((int)rawDataCh[channel] < 0) ? 0 : rawDataCh[channel];

					if ((int)rawDataCh[channel] >= (FLOAT_MODE_SATURATION_CONTROL_CODE - rawDarkCh[channel]))
						rawDataCh[channel] = FLOAT_MODE_SATURATION_CONTROL_CODE - rawDarkCh[channel];
				}

				uncoated_ch3 = rawDataCh[6];
				coated_ch4 = rawDataCh[7];

				for (channel = 4; channel < 8; ++channel)
					adpd_data->slot_data[channel] = rawDataCh[channel];

				adpd_data->sum_slot_b = (((unsigned)coated_ch4) * 100) / uncoated_ch3;

				HRM_info("%s - SLOT B(floating mode) uncoated_ch3:%d, coated_ch4=%d\n", __func__, uncoated_ch3, coated_ch4);
			}
		}
		break;

	default:
		break;
	};
}

static int __adpd_init(void)
{
	int err = 0;

	/* set default register here */
	HRM_dbg("%s - init done, err  = %d\n", __func__, err);


	return err;
}

static int __adpd_set_reg_hrm(void)
{
	int err = 0;

	err = __adpd_init();

	if (err != 0) {
		HRM_dbg("%s __adpd_init failed\n", __func__);
		return -EIO;
	}

	__adpd_mode_switching(adpd_data, SAMPLE_XY_AB_MODE);
	adpd_data->st_obj_proximity = OBJ_DARKOFFSET_CAL;
	adpd_data->eol_state = ST_EOL_IDLE;
	adpd_data->agc_mode = M_HRM;
	__adpd_reg_write(adpd_data, ADPD_OP_MODE_ADDR, 0x1);
	__adpd_reg_write(adpd_data, ADPD_LED1_DRV_ADDR, ST_LED1_CURRENT_SETTING);
	__adpd_reg_write(adpd_data, ADPD_LED2_DRV_ADDR, ST_LED2_CURRENT_SETTING);
	__adpd_reg_write(adpd_data, ADPD_OP_MODE_ADDR, 0x2);

	adpd_data->led_current_1 = ST_LED2_CURRENT_SETTING & 0xf; /* IR */
	adpd_data->led_current_2 = ST_LED1_CURRENT_SETTING & 0xf; /* RED */

	return err;
}
static int __adpd_set_reg_hrmled_both(void)
{
	int err = 0;

	err = __adpd_init();
	if (err != 0) {
		HRM_dbg("%s __adpd_init failed\n", __func__);
		return -EIO;
	}
	__adpd_mode_switching(adpd_data, SAMPLE_XY_AB_MODE);
	adpd_data->agc_mode = M_HRMLED_BOTH;
	__adpd_reg_write(adpd_data, ADPD_LED_DISABLE_ADDR, 0x000);
	adpd_data->cnt_slotB_dc_saturation = adpd_data->cnt_slotA_dc_saturation = 0;

	__adpd_reg_write(adpd_data, ADPD_OP_MODE_ADDR, 0x1);
	__adpd_reg_write(adpd_data, ADPD_LED1_DRV_ADDR, ST_LED1_CURRENT_SETTING);
	__adpd_reg_write(adpd_data, ADPD_LED2_DRV_ADDR, ST_LED2_CURRENT_SETTING);
	__adpd_reg_write(adpd_data, ADPD_OP_MODE_ADDR, 0x2);

	adpd_data->led_current_1 = ST_LED2_CURRENT_SETTING & 0xf; /* IR */
	adpd_data->led_current_2 = ST_LED1_CURRENT_SETTING & 0xf; /* RED */

	return err;
}

static int __adpd_set_reg_hrmled_red(void)
{
	int err = 0;

	err = __adpd_init();
	if (err != 0) {
		HRM_dbg("%s __adpd_init failed\n", __func__);
		return -EIO;
	}

	adpd_data->agc_mode = M_HRMLED_RED;
	__adpd_mode_switching(adpd_data, SAMPLE_XY_A_MODE);
	__adpd_reg_write(adpd_data, ADPD_LED_DISABLE_ADDR, 0x200);
	adpd_data->cnt_slotA_dc_saturation = 0;

	return err;
}

static int __adpd_set_reg_hrmled_ir(void)
{
	int err = 0;

	err = __adpd_init();
	if (err != 0) {
		HRM_dbg("%s __adpd_init failed\n", __func__);
		return -EIO;
	}

	adpd_data->agc_mode = M_HRMLED_IR;
	__adpd_mode_switching(adpd_data, SAMPLE_XY_B_MODE);
	__adpd_reg_write(adpd_data, ADPD_LED_DISABLE_ADDR, 0x100);
	adpd_data->cnt_slotB_dc_saturation = 0;

	return err;
}

static int __adpd_set_reg_ambient(void)
{
	int err = 0;
	err = __adpd_init();
	if (err != 0) {
		HRM_dbg("%s __adpd_init failed\n", __func__);
		return -EIO;
	}
	/* turn off ambient rejection */

	adpd_data->agc_mode = M_NONE;
	__adpd_mode_switching(adpd_data, SAMPLE_TIA_ADC_MODE);

	adpd_data->flicker_data_cnt = 0;

	return err;
}

static int __adpd_set_reg_prox(void)
{
	int err = 0;

	err = __adpd_init();
	if (err != 0) {
		HRM_dbg("%s __adpd_init failed\n", __func__);
		return -EIO;
	}
	adpd_data->agc_mode = M_NONE;
	__adpd_mode_switching(adpd_data, SAMPLE_XY_B_MODE);
	__adpd_set_current(0xf, 0, 0, 0);
	return err;
}

static int __adpd_write_reg_to_file(void)
{
	static char file_data[128 * 20];
	int fd;
	int i = 0, j = 0;
	mm_segment_t old_fs = get_fs();
	unsigned int reg_read[128];

	for (i = 0; i < 128; i++)
		reg_read[i] = 0;

	HRM_dbg("%s\n", __func__);
	if (adpd_data->hrm_mode == 0) {
		HRM_dbg("%s - need to power on.\n", __func__);
		goto error_old_fs;
	}
	for (i = 0; i < 128; i++) {
		reg_read[i] = __adpd_reg_read(adpd_data, i);
		HRM_info("%s - 0x%02x 0x%04x\n", __func__, i, reg_read[i]);
	}

	for (i = 0; i < 128; i++) {
		if (j > ARRAY_SIZE(file_data))
			break;
		j += snprintf(file_data + j, ARRAY_SIZE(file_data) - j, "0x%02x 0x%04x\n", i, reg_read[i]);
	}

	if (j <= ARRAY_SIZE(file_data))
		strlcpy(file_data + j, "Do not remove this line : For check end of file.\n", ARRAY_SIZE(file_data) - j);

	set_fs(KERNEL_DS);
	fd = sys_open(ADPD_READ_REGFILE_PATH, O_WRONLY | O_CREAT, S_IRUGO | S_IWUSR | S_IWGRP);

	if (fd >= 0)
		sys_write(fd, file_data, strlen(file_data));

	sys_close(fd);
error_old_fs:
	set_fs(old_fs);
	return 0;
}


static int __adpd_write_file_to_reg(void)
{
	int fd;
	int line = 0, i = 0, j = 0, k = 0;
	int reg_flag = 1;
	char reg_add[5];
	char reg_data[9];
	static char file_data_int[128 * 20];
	unsigned int reg[128];
	mm_segment_t old_fs = get_fs();
	unsigned short main_mode = 0;
	struct adpd_platform_data *reg_config_data;

	set_fs(KERNEL_DS);

	HRM_dbg("%s\n", __func__);

	/*read file*/
	fd = sys_open(ADPD_WRITE_REGFILE_PATH, O_RDONLY, 0);
	if (fd >= 0) {
		while (i < ARRAY_SIZE(file_data_int) && sys_read(fd, &file_data_int[i], 1) == 1)
			i++;

		sys_close(fd);
		set_fs(old_fs);
	} else {
		HRM_dbg("%s - file read error.\n", __func__);
		goto error;
	}

	main_mode = GET_USR_MODE(atomic_read(&adpd_data->adpd_mode));

	if (main_mode == SAMPLE_USR || main_mode == IDLE_USR)
		reg_config_data = &adpd_config_data;
	else if (main_mode == TIA_ADC_USR)
		reg_config_data = &adpd_tia_adc_config_data;
	else {
		HRM_dbg("%s - invailid mode.\n", __func__);
		goto error;
	}

	/*parse data*/
	i = 0;
	for (line = 0; line < 128; i++) {
		if (file_data_int[i] == 32) { /* 0x20, space, address */
			reg_flag = 0;
			sscanf(reg_add + 2, "%02x", &reg[line]);
			reg_config_data->config_data[line] = reg[line] << 16;
			for (j = 0; j < 5; j++)
				reg_add[j] = 0;
			j = 0;
			continue;
		} else if (file_data_int[i] == 10) { /* 0x0a, LF, value ( 0x0D CR x) */
			sscanf(reg_data + 2, "%04x", &reg[line]);
			reg_config_data->config_data[line] |= reg[line];
			HRM_info("%s - 0x%08x\n", __func__, reg_config_data->config_data[line]);
			for (k = 0; k < 9; k++)
				reg_data[k] = 0;
			reg_flag = 1;
			k = 0;
			line++;
			continue;
		}

		if (reg_flag) {
			if (j < ARRAY_SIZE(reg_add))
				reg_add[j] = file_data_int[i];
			j++;
		} else {
			if (k < ARRAY_SIZE(reg_data))
				reg_data[k] = file_data_int[i];
			k++;
		}
	}

	reg_config_data->config_size = line;

	return 0;

error:
	sys_close(fd);
	set_fs(old_fs);
	return -EIO;
}

static int __adpd_write_default_regs(void)
{
	unsigned short main_mode = 0;

	main_mode = GET_USR_MODE(atomic_read(&adpd_data->adpd_mode));

	__adpd_config_prerequisite(adpd_data, GLOBAL_OP_MODE_IDLE);

	__adpd_configuration(adpd_data, main_mode);

	__adpd_config_prerequisite(adpd_data, GLOBAL_OP_MODE_GO);

	return 0;
}

static void __adpd_print_mode(int mode)
{
	HRM_dbg("%s - ===== mode(%d) =====\n", __func__, mode);
	if (mode == MODE_HRM)
		HRM_dbg("%s - %s(%d)\n", __func__, "HRM", MODE_HRM);
	if (mode == MODE_AMBIENT)
		HRM_dbg("%s - %s(%d)\n", __func__, "AMBIENT", MODE_AMBIENT);
	if (mode == MODE_PROX)
		HRM_dbg("%s - %s(%d)\n", __func__, "PROX", MODE_PROX);
	if (mode == MODE_HRMLED_IR)
		HRM_dbg("%s - %s(%d)\n", __func__, "HRMLED_IR", MODE_HRMLED_IR);
	if (mode == MODE_HRMLED_RED)
		HRM_dbg("%s - %s(%d)\n", __func__, "HRMLED_RED", MODE_HRMLED_RED);
	if (mode == MODE_UNKNOWN)
		HRM_dbg("%s - %s(%d)\n", __func__, "UNKNOWN", MODE_UNKNOWN);
	HRM_dbg("%s - ====================\n", __func__);
}

static int __adpd_set_current(u8 d1, u8 d2, u8 d3, u8 d4) /* d1 : IR, d2 : Red */
{
	unsigned short disable_led = 0;

	adpd_data->led_current_1 = d1; /* IR */
	adpd_data->led_current_2 = d2; /* RED */

	if (d1 == 0x0)
		disable_led |= DISABLE_SLOTB_LED;
	if (d2 == 0x0)
		disable_led |= DISABLE_SLOTA_LED;

	__adpd_config_prerequisite(adpd_data, GLOBAL_OP_MODE_IDLE);

	__adpd_reg_write(adpd_data, ADPD_LED_DISABLE_ADDR, disable_led);
	__adpd_reg_write(adpd_data, ADPD_LED1_DRV_ADDR, LED_SCALE_100 | (d2 & 0xf)); /*Red*/
	__adpd_reg_write(adpd_data, ADPD_LED2_DRV_ADDR, LED_SCALE_100 | (d1 & 0xf)); /*IR*/

	__adpd_config_prerequisite(adpd_data, GLOBAL_OP_MODE_GO);

	HRM_info("%s - IR=0x%x Red=0x%x\n", __func__, LED_SCALE_100 | (d1 & 0xf), LED_SCALE_100 | (d2 & 0xf));

	return 0;
}

int adpd_i2c_read(u32 reg, u32 *value, u32 *size)
{
	int err;

	err = __adpd_i2c_reg_read(adpd_data, (u8)reg, 1, (u16 *)value);

	*size = 2;

	return err;
}

int adpd_i2c_write(u32 reg, u32 value)
{
	int err;

	err = __adpd_i2c_reg_write(adpd_data, (u8)reg, 1, (u16)value);

	return err;
}

static long __adpd_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	int ret = 0;

	struct adpd_device_data *data = container_of(file->private_data,
	struct adpd_device_data, miscdev);

	HRM_info("%s - ioctl start\n", __func__);
	mutex_lock(&data->flickerdatalock);
	switch (cmd) {
	case ADPD143_IOCTL_READ_FLICKER:
		ret = copy_to_user(argp,
			data->flicker_data,
			sizeof(int)*FLICKER_DATA_CNT);
		if (unlikely(ret)) {
			HRM_dbg("%s - read flicker data err(%d)\n", __func__, ret);
			goto ioctl_error;
		}
		break;
	default:
		HRM_dbg("%s - invalid cmd\n", __func__);
		break;
	}
	mutex_unlock(&data->flickerdatalock);
	return ret;

ioctl_error:
	mutex_unlock(&data->flickerdatalock);
	return -ret;
}

static const struct file_operations __adpd_fops = {
	.owner = THIS_MODULE,
	.open = nonseekable_open,
	.unlocked_ioctl = __adpd_ioctl,
};

int adpd_init_device(struct i2c_client *client)
{
	int err = 0, i = 0;
	unsigned short u16_regval = 0;
	u16 for_p2p_reg_value, for_p2p_clk32K, for_p2p_clkfifo;

	HRM_dbg("%s - client  = %p\n", __func__, client);

	adpd_data = kzalloc(sizeof(struct adpd_device_data), GFP_KERNEL);

	if (adpd_data == NULL) {
		err = -ENOMEM;
		HRM_dbg("%s - exit_mem_allocate.\n", __func__);
		goto exit_mem_allocate_failed;
	}

	adpd_data->miscdev.minor = MISC_DYNAMIC_MINOR;
	adpd_data->miscdev.name = "max_hrm";
	adpd_data->miscdev.fops = &__adpd_fops;
	adpd_data->miscdev.mode = S_IRUGO;
	err = misc_register(&adpd_data->miscdev);
	if (err < 0) {
		pr_err("%s - failed to register Device\n", __func__);
		goto err_misc_register_fail;
	}

	adpd_data->flicker_data = kzalloc(sizeof(int)*FLICKER_DATA_CNT, GFP_KERNEL);
	if (adpd_data->flicker_data == NULL) {
		pr_err("%s - couldn't allocate flicker memory\n", __func__);
		goto err_flicker_alloc_fail;
	}

	mutex_init(&adpd_data->flickerdatalock);

	mutex_init(&adpd_data->mutex);
	mutex_lock(&adpd_data->mutex);

	adpd_data->client = client;
	adpd_data->agc_threshold = ADPD_THRESHOLD_DEFAULT;
	adpd_data->read = __adpd_i2c_reg_read;
	adpd_data->write = __adpd_i2c_reg_write;

	adpd_data->i2c_err_cnt = 0;
	adpd_data->ir_curr= 0;
	adpd_data->red_curr= 0;
	adpd_data->ir_adc= 0;
	adpd_data->red_adc = 0;

	/*chip ID verification */
	u16_regval = __adpd_reg_read(adpd_data, ADPD_CHIPID_ADDR);

	switch (u16_regval) {
	case ADPD_CHIPID(0):
	case ADPD_CHIPID(1):
	case ADPD_CHIPID(2):
		adpd_data->osc_trim_open_enable = 1;

	case ADPD_CHIPID(3):
	case ADPD_CHIPID(4):
		for (i = 0; i < 8; i++)
			rawDarkCh[i] = rawDataCh[i] = 0;

		err = 0;
		HRM_info("%s - chipID = 0x%x\n", __func__, u16_regval);
	break;

	default:
		err = -ENODEV;
	break;
	};
	if (err) {
		HRM_dbg("%s - chipID value = 0x%x\n", __func__, u16_regval);
		HRM_dbg("%s - exit_chipid_verification.\n", __func__);
		goto exit_chipid_verification;
	}
	HRM_info("%s - chipID value = 0x%x\n", __func__, u16_regval);

	/*Efuse read for DC normalization */
	for_p2p_clk32K = __adpd_reg_read(adpd_data, ADPD_OSC32K_ADDR);
	__adpd_reg_write(adpd_data, ADPD_OSC32K_ADDR, for_p2p_clk32K | 0x80);

	for_p2p_clkfifo = __adpd_reg_read(adpd_data, ADPD_ACCESS_CTRL_ADDR);
	__adpd_reg_write(adpd_data, ADPD_ACCESS_CTRL_ADDR, for_p2p_clkfifo|1);
	__adpd_reg_write(adpd_data, ADPD_EFUSE_CTRL_ADDR, 0x7);

	do {
		for_p2p_reg_value = __adpd_reg_read(adpd_data, ADPD_P2P_REG_ADDR);
		HRM_info("%s - Reg0x67 : 0x%x\n", __func__, for_p2p_reg_value);
	} while ((for_p2p_reg_value & 0x7) != 0x4);

	if ((for_p2p_reg_value & 0x7) == 0x4) {
		adpd_data->efuseRedSlope = __adpd_reg_read(adpd_data, ADPD_RED_SLOPE_ADDR);
		adpd_data->efuseIrSlope = __adpd_reg_read(adpd_data, ADPD_IR_SLOPE_ADDR);
		adpd_data->efuseRedIntrcpt = __adpd_reg_read(adpd_data, ADPD_RED_INTRCPT_ADDR);
		adpd_data->efuseIrIntrcpt = __adpd_reg_read(adpd_data, ADPD_IR_INTRCPT_ADDR);
		adpd_data->device_id  = (u64)(__adpd_reg_read(adpd_data, 0x7B) & 0xff) << 32;
		adpd_data->device_id |= (u64)(__adpd_reg_read(adpd_data, 0x7C) & 0xff) << 24;
		adpd_data->device_id |= (u64)(__adpd_reg_read(adpd_data, 0x7D) & 0xff) << 16;
		adpd_data->device_id |= (u64)(__adpd_reg_read(adpd_data, 0x79) & 0xff) << 8;
		adpd_data->device_id |= (u64)(__adpd_reg_read(adpd_data, 0x7A) & 0xff);

		HRM_info("%s - device_id : %lld\n", __func__, adpd_data->device_id);
		HRM_info("%s - efuseRedSlope : %d\n", __func__, adpd_data->efuseRedSlope);
		HRM_info("%s - efuseIrSlope : %d\n", __func__, adpd_data->efuseIrSlope);
		HRM_info("%s - efuseRedIntrcpt : %d\n", __func__, adpd_data->efuseRedIntrcpt);
		HRM_info("%s - efuseIrIntrcpt : %d\n", __func__, adpd_data->efuseIrIntrcpt);
	}
	__adpd_reg_write(adpd_data, ADPD_EFUSE_CTRL_ADDR, 0x0);

	if (adpd_data->efuse32KfreqOffset) {
		/* use efuse clock offset */
		for_p2p_clk32K &= 0xFFC0;
		for_p2p_clk32K |= ((34 + (adpd_data->efuse32KfreqOffset-0x80)/30) & 0x3F);
		__adpd_reg_write(adpd_data, ADPD_OSC32K_ADDR, for_p2p_clk32K);
	} else
		adpd_data->oscRegValue = 0;

	mutex_unlock(&adpd_data->mutex);

	adpd_data->ptr_data_buffer = data_buffer;

	adpd_data->Is_Hrm_Dark_Calibrated = 0;
	__adpd_config_prerequisite(adpd_data, GLOBAL_OP_MODE_OFF);

	__adpd_setObjProximityThreshold(ADPD_THRESHOLD_DEFAULT);


	HRM_dbg("%s - success\n", __func__);

	goto done;

exit_chipid_verification:
	mutex_unlock(&adpd_data->mutex);
	mutex_destroy(&adpd_data->mutex);
	mutex_destroy(&adpd_data->flickerdatalock);
	kfree(adpd_data->flicker_data);
err_flicker_alloc_fail:
	misc_deregister(&adpd_data->miscdev);
err_misc_register_fail:
	kfree(adpd_data);
exit_mem_allocate_failed:
	HRM_dbg("%s - failed\n", __func__);

done:
	return err;
}

int adpd_deinit_device(void)
{
	HRM_dbg("%s\n", __func__);

	__adpd_mode_switching(adpd_data, IDLE_MODE);
	i2c_set_clientdata(adpd_data->client, NULL);

	mutex_destroy(&adpd_data->mutex);
	mutex_destroy(&adpd_data->flickerdatalock);

	kfree(adpd_data->flicker_data);
	misc_deregister(&adpd_data->miscdev);
	kfree(adpd_data);
	adpd_data = NULL;

	return 0;
}

int adpd_enable(enum hrm_mode mode)
{
	int err = 0;

	adpd_data->hrm_mode = mode;
	adpd_data->cnt_enable = 0;

	HRM_dbg("%s - enable_m : 0x%x\t cur_m : 0x%x\n", __func__, mode, adpd_data->hrm_mode);
	__adpd_print_mode(adpd_data->hrm_mode);

	if (mode == MODE_HRM)
		err = __adpd_set_reg_hrm();
	else if (mode == MODE_AMBIENT)
		err = __adpd_set_reg_ambient();
	else if (mode == MODE_PROX)
		err = __adpd_set_reg_prox();
	else if (mode == MODE_HRMLED_IR)
		err = __adpd_set_reg_hrmled_ir();
	else if (mode == MODE_HRMLED_RED)
		err = __adpd_set_reg_hrmled_red();
	else if (mode == MODE_HRMLED_BOTH)
		err = __adpd_set_reg_hrmled_both();
	else
		HRM_dbg("%s - MODE_UNKNOWN\n", __func__);

	return err;
}

int adpd_disable(enum hrm_mode mode)
{
	int err = 0;

	adpd_data->hrm_mode = 0;
	HRM_dbg("%s - disable_m : 0x%x\t cur_m : 0x%x\n", __func__, mode, adpd_data->hrm_mode);
	__adpd_print_mode(adpd_data->hrm_mode);

	if (adpd_data->hrm_mode == 0) {
		__adpd_mode_switching(adpd_data, IDLE_MODE);
		return err;
	}

	adpd_data->flicker_data_cnt = 0;

	return err;
}

int adpd_get_current(u8 *d1, u8 *d2, u8 *d3, u8 *d4)
{
	*d1 = adpd_data->led_current_1; /* IR */
	*d2 = adpd_data->led_current_2; /* RED */
	*d3 = 0;
	*d4 = 0;

	return 0;
}
int adpd_set_current(u8 d1, u8 d2, u8 d3, u8 d4) /* d1 : IR, d2 : Red */
{
	return __adpd_set_current(d1, d2, d3, d4);
}

int adpd_read_data(struct hrm_output_data *data)
{
	int err = 0;
	unsigned short usr_mode = 0;
	unsigned short main_mode = 0;
	unsigned short sub_mode = 0;
	int ch;

	data->mode = adpd_data->hrm_mode;

	data->main_num = 0;
	data->sub_num = 0;

	usr_mode = atomic_read(&adpd_data->adpd_mode);
	main_mode = GET_USR_MODE(usr_mode);
	sub_mode = GET_USR_SUB_MODE(usr_mode);

	__adpd_rd_intr_fifo(adpd_data);

	if (adpd_data->intr_status_val == 0) {
		HRM_dbg("%s - intr_status_val : 0x%x\n", __func__, adpd_data->intr_status_val);
	}

	if (adpd_data->fifo_size < FLICKER_DATA_CNT && main_mode == TIA_ADC_USR) {
		HRM_info("%s - ERROR FIFO_SIZE = %d\n", __func__,
			adpd_data->fifo_size);
		__adpd_clr_intr_status(adpd_data, main_mode);
		return -EIO;
	}

	__adpd_clr_intr_status(adpd_data, main_mode);

	do_gettimeofday(&curr_interrupt_trigger_ts);

	if (prev_interrupt_trigger_ts.tv_sec > 0) {
#ifndef __USE_EOL_US_INT_SPACE__
		adpd_data->msec_eol_int_space = (curr_interrupt_trigger_ts.tv_sec - prev_interrupt_trigger_ts.tv_sec) * USEC_PER_SEC
										+ (curr_interrupt_trigger_ts.tv_usec - prev_interrupt_trigger_ts.tv_usec);
		if (adpd_data->msec_eol_int_space - ((int)(adpd_data->msec_eol_int_space / USEC_PER_MSEC)) * USEC_PER_MSEC >= USEC_PER_MSEC / 2)
			adpd_data->msec_eol_int_space = ((int)(adpd_data->msec_eol_int_space / USEC_PER_MSEC)) + 1;
		else
			adpd_data->msec_eol_int_space = ((int)(adpd_data->msec_eol_int_space / USEC_PER_MSEC));

		if (adpd_data->msec_eol_int_space < 2)
			HRM_dbg("%s - msec_eol_int_space : %dms\n", __func__, adpd_data->msec_eol_int_space);
#else
		adpd_data->usec_eol_int_space = (curr_interrupt_trigger_ts.tv_sec - prev_interrupt_trigger_ts.tv_sec) * USEC_PER_SEC
										+ (curr_interrupt_trigger_ts.tv_usec - prev_interrupt_trigger_ts.tv_usec);

		if (adpd_data->usec_eol_int_space < 2000)
			HRM_dbg("%s - usec_eol_int_space : %dus\n", __func__, adpd_data->usec_eol_int_space);
#endif
	} else
#ifndef __USE_EOL_US_INT_SPACE__
		adpd_data->msec_eol_int_space = -1;
#else
		adpd_data->usec_eol_int_space = -1;
#endif

	memcpy(&prev_interrupt_trigger_ts, &curr_interrupt_trigger_ts, sizeof(curr_interrupt_trigger_ts));

	switch (main_mode) {
	case IDLE_USR:
#ifdef __USE_ADPD_32BIT_FIFO__
		if (fifo_is_32bit)
			 __adpd_rd_fifo32_data(adpd_data);
		else
#endif
		__adpd_rd_fifo_data(adpd_data);
		break;

	case EOL_USR:
		HRM_info("%s - EOL SAMPLE MODE\n", __func__);

	case SAMPLE_USR:
	    HRM_info("HRM SAMPLE MODE - [ agc_mode : %d ]\n", adpd_data->agc_mode);
#ifdef __USE_ADPD_32BIT_FIFO__
		if (fifo_is_32bit)
			 __adpd_rd_fifo32_data(adpd_data);
		else
#endif
		__adpd_rd_fifo_data(adpd_data);
		__adpd_sample_data(adpd_data);

		switch (sub_mode) {
		case S_SAMP_XY_A:
			if (((adpd_data->agc_mode == M_HRM && adpd_data->st_obj_proximity != OBJ_DARKOFFSET_CAL) ||
				(adpd_data->agc_mode != M_HRM && ((agc_is_enabled && __adpd_getAGCstate() == AgcStage_Done) || !agc_is_enabled))) &&
				 adpd_data->cnt_enable >= CNT_ENABLE_SKIP_SAMPLE){
				data->main_num = 2;
	#ifdef __USE_LOWPASS_FIR_FILTER__
				data->data_main[1] = adpd_data->fltr_sum_slot_a;
	#else
				data->data_main[1] = adpd_data->sum_slot_a;
	#endif
				data->sub_num = 4;
				for (ch = 0; ch < data->sub_num; ch++)
					data->data_sub[ch] = adpd_data->slot_data[ch];
			} else {
				if (adpd_data->cnt_enable < CNT_ENABLE_SKIP_SAMPLE)
					adpd_data->cnt_enable++;
				data->main_num = 0;
				data->sub_num = 0;
			}
			break;

		case S_SAMP_XY_AB:
			if (((adpd_data->agc_mode == M_HRM && adpd_data->st_obj_proximity != OBJ_DARKOFFSET_CAL) ||
				(adpd_data->agc_mode != M_HRM && ((agc_is_enabled && __adpd_getAGCstate() == AgcStage_Done) || !agc_is_enabled))) &&
				adpd_data->cnt_enable >= CNT_ENABLE_SKIP_SAMPLE){
				data->main_num = 2;
	#ifdef __USE_LOWPASS_FIR_FILTER__
				data->data_main[1] = adpd_data->fltr_sum_slot_a;
				data->data_main[0] = adpd_data->fltr_sum_slot_b;
	#else
				data->data_main[1] = adpd_data->sum_slot_a;
				data->data_main[0] = adpd_data->sum_slot_b;
	#endif
				data->sub_num = 8;
				for (ch = 0; ch < data->sub_num; ch++)
					data->data_sub[ch] = adpd_data->slot_data[ch];
			} else {
				if (adpd_data->cnt_enable < CNT_ENABLE_SKIP_SAMPLE)
					adpd_data->cnt_enable++;
				data->main_num = 0;
				data->sub_num = 0;
			}
			break;

		case S_SAMP_XY_B:
			if (((adpd_data->agc_mode == M_HRM && adpd_data->st_obj_proximity != OBJ_DARKOFFSET_CAL) ||
				(adpd_data->agc_mode != M_HRM && ((agc_is_enabled && __adpd_getAGCstate() == AgcStage_Done) || !agc_is_enabled))) &&
				adpd_data->cnt_enable >= CNT_ENABLE_SKIP_SAMPLE){
				data->main_num = 1;
	#ifdef __USE_LOWPASS_FIR_FILTER__
				data->data_main[0] = adpd_data->fltr_sum_slot_b;
	#else
				data->data_main[0] = adpd_data->sum_slot_b;
	#endif
				data->sub_num = 4;
				for (ch = 0; ch < data->sub_num; ch++)
					data->data_sub[ch+4] = adpd_data->slot_data[ch+4];
			} else {
				if (adpd_data->cnt_enable < CNT_ENABLE_SKIP_SAMPLE)
					adpd_data->cnt_enable++;
				data->main_num = 0;
				data->sub_num = 0;
			}
			break;

		 default:
			break;
		}
		break;

	 case TIA_ADC_USR:
		HRM_info("TIA_ADC SAMPLE MODE\n");
#ifdef __USE_ADPD_32BIT_FIFO__
		if (fifo_is_32bit)
			__adpd_rd_fifo32_data(adpd_data);
		else
#endif
		__adpd_rd_fifo_data(adpd_data);
		__adpd_sample_data(adpd_data);

		switch (sub_mode) {
		case S_SAMP_XY_A: /* TIA_ADC */
			if (Is_TIA_ADC_Dark_Calibrated > 100) {
				data->main_num = 2;
				data->data_main[1] = adpd_data->sum_slot_a;
				data->sub_num = 4;
				for (ch = 0; ch < data->sub_num; ch++)
					data->data_sub[ch] = adpd_data->slot_data[ch];
			} else {
				data->main_num = 0;
				data->sub_num = 0;
			}
			break;

		case S_SAMP_XY_AB: /* TNF */
			if (Is_Float_Dark_Calibrated > 100) {
				data->main_num = 2;
				data->data_main[1] = adpd_data->sum_slot_a;
				data->data_main[0] = adpd_data->sum_slot_b;
				if (adpd_data->flicker_data_cnt * 8 == FLICKER_DATA_CNT) {
					data->data_main[1] = -2;
					adpd_data->flicker_data_cnt = 0;
				} else
					data->data_main[1] = 0;
				HRM_info("%s - flicker_data_cnt : %d\n", __func__, adpd_data->flicker_data_cnt);
				data->sub_num = 8;
				for (ch = 0; ch < data->sub_num; ch++)
					data->data_sub[ch] = adpd_data->slot_data[ch];
			} else {
				data->main_num = 0;
				data->sub_num = 0;
			}
			break;

		case S_SAMP_XY_B: /* Float */
			if (Is_Float_Dark_Calibrated > 100) {
				data->main_num = 1;
				data->data_main[0] = adpd_data->sum_slot_b;
				data->sub_num = 4;
				for (ch = 0; ch < data->sub_num; ch++)
					data->data_sub[ch] = adpd_data->slot_data[ch+4];
			} else {
				data->main_num = 0;
				data->sub_num = 0;
			}
			break;

		default:
			break;
		}
		break;

		default:
#ifdef __USE_ADPD_32BIT_FIFO__
		if (fifo_is_32bit)
			__adpd_rd_fifo32_data(adpd_data);
		else
#endif
			__adpd_rd_fifo_data(adpd_data);
		break;
	 };

	HRM_info("%s - sum_slot_a : %d,\t sum_slot_b : %d\n", __func__, adpd_data->sum_slot_a, adpd_data->sum_slot_b);

	return err;
}

int adpd_get_chipid(u64 *chip_id)
{
	int err = 0;

	*chip_id = adpd_data->device_id;

	return err;
}

int adpd_get_part_type(u16 *part_type)
{
	int err = 0;

	*part_type = __adpd_reg_read(adpd_data, ADPD_CHIPID_ADDR);

	return err;
}

int adpd_get_i2c_err_cnt(u32 *err_cnt)
{
	int err = 0;

	*err_cnt = adpd_data->i2c_err_cnt;

	return err;
}

int adpd_get_curr_adc(u16 *ir_curr, u16 *red_curr, u32 *ir_adc, u32 *red_adc)
{
	int err = 0;

	*ir_curr = adpd_data->ir_curr;
	*red_curr = adpd_data->red_curr;
	*ir_adc = adpd_data->ir_adc;
	*red_adc = adpd_data->red_adc;

	return err;
}

int adpd_get_name_chipset(char *name)
{
	strlcpy(name, CHIP_NAME, strlen(CHIP_NAME) + 1);
	if (strncmp(name, CHIP_NAME, strlen(CHIP_NAME) + 1))
		return -EINVAL;
	else
		return 0;
}

int adpd_get_name_vendor(char *name)
{
	strlcpy(name, VENDOR, strlen(VENDOR) + 1);
	if (strncmp(name, VENDOR, strlen(VENDOR) + 1))
		return -EINVAL;
	else
		return 0;
}

int adpd_get_threshold(s32 *agc_threshold)
{
	*agc_threshold = adpd_data->agc_threshold;
	if (*agc_threshold != adpd_data->agc_threshold)
		return -EINVAL;
	else
		return 0;
}

int adpd_set_threshold(s32 agc_threshold)
{
	adpd_data->agc_threshold = agc_threshold;

	__adpd_setObjProximityThreshold(agc_threshold);

	if (agc_threshold != adpd_data->agc_threshold)
		return -EINVAL;
	else
		return 0;
}

int adpd_set_eol_enable(u8 enable)
{
	HRM_dbg("%s - enable = %d)\n", __func__, enable);

	if (adpd_data) {
		adpd_data->eol_test_is_enable = enable;

		if (enable) {
			__adpd_mode_switching(adpd_data, SAMPLE_EOL_MODE);
			adpd_data->eol_test_status = 0;
		} else
			__adpd_mode_switching(adpd_data, IDLE_MODE);
	}

	return 0;
}

int adpd_get_eol_result(char *result)
{
	if (adpd_data->eol_test_status == 0) {
		HRM_info("%s - adpd_data->eol_test_status is 0\n",
			__func__);
		adpd_data->eol_test_status = 0;
		return snprintf(result, PAGE_SIZE, "%s\n", "NO_EOL_TEST");
	}

	if (adpd_data && result) {
		ADPD_EOL_RESULT temp;
		temp.st_eol_res.eol_res_odr = adpd_data->eol_res_odr;

		temp.st_eol_res.eol_res_red_low_dc = adpd_data->eol_res_red_low_dc;
		temp.st_eol_res.eol_res_ir_low_dc = adpd_data->eol_res_ir_low_dc;

		temp.st_eol_res.eol_res_red_med_dc = adpd_data->eol_res_red_med_dc;
		temp.st_eol_res.eol_res_ir_med_dc = adpd_data->eol_res_ir_med_dc;
		temp.st_eol_res.eol_res_red_med_noise = adpd_data->eol_res_red_med_noise;
		temp.st_eol_res.eol_res_ir_med_noise = adpd_data->eol_res_ir_med_noise;

		temp.st_eol_res.eol_res_red_high_dc = adpd_data->eol_res_red_high_dc;
		temp.st_eol_res.eol_res_ir_high_dc = adpd_data->eol_res_ir_high_dc;

		temp.st_eol_res.eol_res_melanin_red_dc = adpd_data->eol_res_melanin_red_dc;
		temp.st_eol_res.eol_res_melanin_ir_dc = adpd_data->eol_res_melanin_ir_dc;

		snprintf(result, PAGE_SIZE, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
			adpd_data->eol_res_odr,
			adpd_data->eol_res_red_low_dc, adpd_data->eol_res_ir_low_dc,
			adpd_data->eol_res_red_med_dc, adpd_data->eol_res_ir_med_dc,
			adpd_data->eol_res_red_med_noise, adpd_data->eol_res_ir_med_noise,
			adpd_data->eol_res_red_high_dc,	adpd_data->eol_res_ir_high_dc,
			adpd_data->eol_res_melanin_red_dc, adpd_data->eol_res_melanin_ir_dc);

		adpd_data->eol_test_is_enable = 0;
		adpd_data->eol_test_status = 0;
	}

	return 0;
}

int adpd_get_eol_status(u8 *status)
{
	*status = adpd_data->eol_test_status;
	if (*status != adpd_data->eol_test_status)
		return -EINVAL;
	else
		return 0;
}

int adpd_debug_set(u8 mode)
{

	HRM_dbg("%s - mode = %d\n", __func__, mode);

	switch (mode) {
	case DEBUG_WRITE_REG_TO_FILE:
		__adpd_write_reg_to_file();
		break;
	case DEBUG_WRITE_FILE_TO_REG:
		__adpd_write_file_to_reg();
		__adpd_write_default_regs();
		break;
	case DEBUG_SHOW_DEBUG_INFO:
		break;
	case DEBUG_ENABLE_AGC:
		agc_is_enabled = 1;
		break;
	case DEBUG_DISABLE_AGC:
		agc_is_enabled = 0;
		break;
	default:
		break;
	}

	return 0;
}

int adpd_get_fac_cmd(char *cmd_result)
{
	HRM_dbg("%s - efuseRedSlope : %d\n", __func__, adpd_data->efuseRedSlope);
	HRM_dbg("%s - efuseIrSlope : %d\n", __func__, adpd_data->efuseIrSlope);

	return snprintf(cmd_result, PAGE_SIZE, "%u,%u",
		adpd_data->efuseRedSlope, adpd_data->efuseIrSlope);
}
