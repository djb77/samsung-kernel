/*
 * STMicroelectronics LIS2DS driver
 *
 * Copyright 2016 STMicroelectronics Inc.
 *
 * Giuseppe Barba <giuseppe.barba@st.com>
 * Mario Tesi <mario.tesi@st.com>
 * Lorenzo Bianconi <lorenzo.bianconi@st.com>
 * v.1.1.1
 * Licensed under the GPL-2.
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/hrtimer.h>
#include <linux/input.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/mutex.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/sensor/sensors_core.h>
#include "lis2ds_core.h"

#ifdef TAG
#undef TAG
#define TAG "[ACCEL]"
#endif

#define LIS2DS_SENSORHUB_OUT1_ADDR		0x06
#define LIS2DS_SENSORHUB_OUT2_ADDR		0x07
#define LIS2DS_SENSORHUB_OUT3_ADDR		0x08
#define LIS2DS_SENSORHUB_OUT4_ADDR		0x09
#define LIS2DS_SENSORHUB_OUT5_ADDR		0x0a
#define LIS2DS_SENSORHUB_OUT6_ADDR		0x0b
#define LIS2DS_MODULE_8_ADDR			0x0c
#define LIS2DS_WHO_AM_I_ADDR			0x0f
#define LIS2DS_WHO_AM_I_DEF			0x43
#define LIS2DS_CTRL1_ADDR			0x20
#define LIS2DS_CTRL2_ADDR			0x21
#define LIS2DS_CTRL3_ADDR			0x22
#define LIS2DS_CTRL4_INT1_PAD_ADDR		0x23
#define LIS2DS_CTRL5_INT2_PAD_ADDR		0x24
#define LIS2DS_FIFO_CTRL_ADDR			0x25
#define LIS2DS_OUT_T_ADDR			0x26
#define LIS2DS_STATUS_ADDR			0x27
#define LIS2DS_OUTX_L_ADDR			0x28
#define LIS2DS_FIFO_THR_ADDR			0x2e
#define LIS2DS_FIFO_SRC_ADDR			0x2f
#define LIS2DS_FIFO_SAMPLE_ADDR			0x30
#define LIS2DS_TAP_THS_6D_ADDR			0x31
#define LIS2DS_INT_DUR_ADDR			0x32
#define LIS2DS_WAKE_UP_THS_ADDR			0x33
#define LIS2DS_WAKE_UP_DUR_ADDR			0x34
#define LIS2DS_FREE_FALL_ADDR			0x35
#define LIS2DS_STATUS_DUP_ADDR			0x36
#define LIS2DS_WAKE_UP_SRC_ADDR			0x37
#define LIS2DS_TAP_SRC_ADDR			0x38
#define LIS2DS_6D_SRC_ADDR			0x39
#define LIS2DS_STEP_C_MINTHS_ADDR		0x3a
#define LIS2DS_STEP_C_MINTHS_RST_NSTEP_MASK	0x80
#define LIS2DS_STEP_C_OUT_L_ADDR		0x3b
#define LIS2DS_STEP_C_OUT_SIZE			2

#define LIS2DS_FUNC_CK_GATE_ADDR		0x3d
#define LIS2DS_FUNC_CK_GATE_TILT_INT_MASK	0x80
#define LIS2DS_FUNC_CK_GATE_SIGN_M_DET_MASK	0x10
#define LIS2DS_FUNC_CK_GATE_RST_SIGN_M_MASK	0x08
#define LIS2DS_FUNC_CK_GATE_RST_PEDO_MASK	0x04
#define LIS2DS_FUNC_CK_GATE_STEP_D_MASK		0x02
#define LIS2DS_FUNC_CK_GATE_MASK		(LIS2DS_FUNC_CK_GATE_TILT_INT_MASK | \
						LIS2DS_FUNC_CK_GATE_SIGN_M_DET_MASK | \
						LIS2DS_FUNC_CK_GATE_STEP_D_MASK)

#define LIS2DS_FUNC_SRC_ADDR			0x3e

#define LIS2DS_FUNC_CTRL_ADDR			0x3f
#define LIS2DS_FUNC_CTRL_TILT_MASK		0x10
#define LIS2DS_FUNC_CTRL_SIGN_MOT_MASK		0x02
#define LIS2DS_FUNC_CTRL_STEP_CNT_MASK		0x01
#define LIS2DS_FUNC_CTRL_EV_MASK		(LIS2DS_FUNC_CTRL_TILT_MASK | \
						LIS2DS_FUNC_CTRL_SIGN_MOT_MASK | \
						LIS2DS_FUNC_CTRL_STEP_CNT_MASK)

#define LIS2DS_FIFO_THS_ADDR			LIS2DS_STATUS_ADDR
#define LIS2DS_FIFO_THS_MASK			0x80

#define LIS2DS_INT_STATUS_ADDR			LIS2DS_STATUS_ADDR
#define LIS2DS_WAKE_UP_IA_MASK			0x40
#define LIS2DS_DOUBLE_TAP_MASK			0x10
#define LIS2DS_SINGLE_TAP_MASK			0x08
#define LIS2DS_6D_IA_MASK			0x04
#define LIS2DS_FF_IA_MASK			0x02
#define LIS2DS_DRDY_MASK			0x01
#define LIS2DS_EVENT_MASK			(LIS2DS_WAKE_UP_IA_MASK | \
						LIS2DS_DOUBLE_TAP_MASK | \
						LIS2DS_SINGLE_TAP_MASK | \
						LIS2DS_6D_IA_MASK | \
						LIS2DS_FF_IA_MASK)

#define LIS2DS_ODR_ADDR				LIS2DS_CTRL1_ADDR
#define LIS2DS_ODR_MASK				0xf0
#define LIS2DS_ODR_POWER_OFF_VAL		0x00
#define LIS2DS_ODR_1HZ_LP_VAL			0x08
#define LIS2DS_ODR_12HZ_LP_VAL			0x09
#define LIS2DS_ODR_25HZ_LP_VAL			0x0a
#define LIS2DS_ODR_50HZ_LP_VAL			0x0b
#define LIS2DS_ODR_100HZ_LP_VAL			0x0c
#define LIS2DS_ODR_200HZ_LP_VAL			0x0d
#define LIS2DS_ODR_400HZ_LP_VAL			0x0e
#define LIS2DS_ODR_800HZ_LP_VAL			0x0f
#define LIS2DS_ODR_LP_LIST_NUM			9

#define LIS2DS_ODR_12_5HZ_HR_VAL		0x01
#define LIS2DS_ODR_25HZ_HR_VAL			0x02
#define LIS2DS_ODR_50HZ_HR_VAL			0x03
#define LIS2DS_ODR_100HZ_HR_VAL			0x04
#define LIS2DS_ODR_200HZ_HR_VAL			0x05
#define LIS2DS_ODR_400HZ_HR_VAL			0x06
#define LIS2DS_ODR_800HZ_HR_VAL			0x07
#define LIS2DS_ODR_HR_LIST_NUM			8

#define LIS2DS_FS_ADDR				LIS2DS_CTRL1_ADDR
#define LIS2DS_FS_MASK				0x0c
#define LIS2DS_FS_2G_VAL			0x00
#define LIS2DS_FS_4G_VAL			0x02
#define LIS2DS_FS_8G_VAL			0x03
#define LIS2DS_FS_16G_VAL			0x01

#define LIS2DS_FS_2G_GAIN			61
#define LIS2DS_FS_4G_GAIN			122
#define LIS2DS_FS_8G_GAIN			244
#define LIS2DS_FS_16G_GAIN			488

#define LIS2DS_FS_LIST_NUM			4
enum {
	LIS2DS_LP_MODE = 0,
	LIS2DS_HR_MODE,
	LIS2DS_MODE_COUNT,
};
#define LIS2DS_MODE_DEFAULT			LIS2DS_LP_MODE

#define LIS2DS_INT1_SHUB_DRDY_MASK		0x80
#define LIS2DS_INT1_S_TAP_MASK			0x40
#define LIS2DS_INT1_WAKEUP_MASK			0x20
#define LIS2DS_INT1_FREE_FALL_MASK		0x10
#define LIS2DS_INT1_TAP_MASK			0x08
#define LIS2DS_INT1_6D_MASK			0x04
#define LIS2DS_INT1_FTH_MASK			0x02
#define LIS2DS_INT1_DRDY_MASK			0x01
#define LIS2DS_INT1_EVENTS_MASK			(LIS2DS_INT1_S_TAP_MASK | \
						LIS2DS_INT1_WAKEUP_MASK | \
						LIS2DS_INT1_FREE_FALL_MASK | \
						LIS2DS_INT1_TAP_MASK | \
						LIS2DS_INT1_6D_MASK | \
						LIS2DS_INT1_FTH_MASK | \
						LIS2DS_INT1_DRDY_MASK)

#define LIS2DS_INT2_ON_INT1_MASK		0x20
#define LIS2DS_INT2_TILT_MASK			0x10
#define LIS2DS_INT2_SIG_MOT_DET_MASK		0x08
#define LIS2DS_INT2_STEP_DET_MASK		0x04
#define LIS2DS_INT2_EVENTS_MASK			(LIS2DS_INT2_TILT_MASK | \
						LIS2DS_INT2_SIG_MOT_DET_MASK | \
						LIS2DS_INT2_STEP_DET_MASK)

#define LIS2DS_INT_DUR_SHOCK_MASK		0x03
#define LIS2DS_INT_DUR_QUIET_MASK		0x0c
#define LIS2DS_INT_DUR_LAT_MASK			0xf0
#define LIS2DS_INT_DUR_MASK			(LIS2DS_INT_DUR_SHOCK_MASK | \
						LIS2DS_INT_DUR_QUIET_MASK | \
						LIS2DS_INT_DUR_LAT_MASK)
#define LIS2DS_INT_DUR_STAP_DEFAULT		0x06
#define LIS2DS_INT_DUR_DTAP_DEFAULT		0x7f

#define LIS2DS_WAKE_UP_THS_S_D_TAP_MASK		0x80
#define LIS2DS_WAKE_UP_THS_SLEEP_MASK		0x40
#define LIS2DS_WAKE_UP_THS_WU_MASK		0x3f
#define LIS2DS_WAKE_UP_DUR_WU_MASK		0x60
#define LIS2DS_WAKE_UP_THS_WU_DEFAULT		0x02

#define LIS2DS_FREE_FALL_THS_MASK		0x07
#define LIS2DS_FREE_FALL_DUR_MASK		0xF8
#define LIS2DS_FREE_FALL_THS_DEFAULT		0x01
#define LIS2DS_FREE_FALL_DUR_DEFAULT		0x01

#define LIS2DS_WAKE_UP_THS_MASK			0x3f
#define LIS2DS_WAKE_UP_SD_TAP_MASK		0x80

#define LIS2DS_HF_ODR_ADDR			LIS2DS_CTRL1_ADDR
#define LIS2DS_HF_ODR_MASK			0x02
#define LIS2DS_BDU_ADDR				LIS2DS_CTRL1_ADDR
#define LIS2DS_BDU_MASK				0x01

#define LIS2DS_SOFT_RESET_ADDR			LIS2DS_CTRL2_ADDR
#define LIS2DS_SOFT_RESET_MASK			0x40

#define LIS2DS_LIR_ADDR				LIS2DS_CTRL3_ADDR
#define LIS2DS_LIR_MASK				0x04

#define LIS2DS_TAP_AXIS_ADDR			LIS2DS_CTRL3_ADDR
#define LIS2DS_TAP_AXIS_MASK			0x38
#define LIS2DS_TAP_AXIS_ANABLE_ALL		0x07

#define LIS2DS_SELF_TEST_ADDR			LIS2DS_CTRL3_ADDR
#define LIS2DS_SELF_TEST_MASK			0xc0
#define LIS2DS_SELF_TEST_NORM_M			0
#define LIS2DS_SELF_TEST_POS_SIGN		1
#define LIS2DS_SELF_TEST_NEG_SIGN		2
#define LIS2DS_TT_AXIS_EN_ADDR			LIS2DS_CTRL3_ADDR
#define LIS2DS_TT_AXIS_EN_MASK			0x38
#define LIS2DS_TT_AXIS_EN_VAL			0x07
#define LIS2DS_TAP_THS_ADDR			LIS2DS_TAP_THS_6D_ADDR
#define LIS2DS_TAP_THS_MASK			0x1f
#define LIS2DS_TAP_THS_DEFAULT			0x09

#define LIS2DS_INT2_ON_INT1_ADDR		LIS2DS_CTRL5_INT2_PAD_ADDR
#define LIS2DS_INT2_ON_INT1_MASK		0x20

#define LIS2DS_FIFO_MODE_ADDR			LIS2DS_FIFO_CTRL_ADDR
#define LIS2DS_FIFO_MODE_MASK			0xe0
#define LIS2DS_FIFO_MODE_BYPASS			0x00
#define LIS2DS_FIFO_MODE_STOP_ON_FULL		0x01
#define LIS2DS_FIFO_MODE_CONTINUOS		0x06

#define LIS2DS_ACCEL_STD			1
#define LIS2DS_ACCEL_STD_FROM_PD		2
#define LIS2DS_OUT_XYZ_SIZE			6
#define LIS2DS_EN_BIT				0x01
#define LIS2DS_DIS_BIT				0x00

#define LIS2DS_ACCEL_BIT			(1 << LIS2DS_ACCEL)
#define LIS2DS_FF_BIT				(1 << LIS2DS_FF)
#define LIS2DS_TAP_BIT				(1 << LIS2DS_TAP)
#define LIS2DS_WAKEUP_BIT			(1 << LIS2DS_WAKEUP)
#define LIS2DS_ACTIVITY_BIT			(1 << LIS2DS_ACTIVITY)
#define LIS2DS_ALL_EVENT_BIT_MASK		(LIS2DS_FF_BIT | \
						LIS2DS_TAP_BIT | \
						LIS2DS_WAKEUP_BIT | \
						LIS2DS_ACTIVITY_BIT)

#define LIS2DS_EVENT_FF_CODE			(1 << LIS2DS_EVENT_FF)
#define LIS2DS_EVENT_TAP_CODE			(1 << LIS2DS_EVENT_TAP)
#define LIS2DS_EVENT_DOUBLE_TAP_CODE		(1 << LIS2DS_EVENT_DOUBLE_TAP)
#define LIS2DS_EVENT_STEP_D_CODE		(1 << LIS2DS_EVENT_STEP_D)
#define LIS2DS_EVENT_TILT_CODE			(1 << LIS2DS_EVENT_TILT)
#define LIS2DS_EVENT_SIGN_M_CODE		(1 << LIS2DS_EVENT_SIGN_M)
#define LIS2DS_EVENT_WAKEUP_CODE		(1 << LIS2DS_EVENT_WAKEUP)
#define LIS2DS_EVENT_ACTIVITY_CODE		(1 << LIS2DS_EVENT_ACTIVITY)
#define LIS2DS_EVENT_ALL_CODE			(LIS2DS_EVENT_FF_CODE | \
						LIS2DS_EVENT_TAP_CODE | \
						LIS2DS_EVENT_DOUBLE_TAP_CODE | \
						LIS2DS_EVENT_STEP_D_CODE | \
						LIS2DS_EVENT_TILT_CODE | \
						LIS2DS_EVENT_SIGN_M_CODE | \
						LIS2DS_EVENT_WAKEUP_CODE | \
						LIS2DS_EVENT_ACTIVITY_CODE)

#define LIS2DS_ACCEL_ODR			200
#define LIS2DS_ACCEL_FS				4
#define LIS2DS_FF_ODR				25
#define LIS2DS_STEP_D_ODR			25
#define LIS2DS_TILT_ODR				25
#define LIS2DS_SIGN_M_ODR			25
#define LIS2DS_TAP_ODR				400
#define LIS2DS_WAKEUP_ODR			25
#define LIS2DS_ACTIVITY_ODR			12

#define LIS2DS_FIFO_ELEMENT_LEN_BYTE		2
#define LIS2DS_FIFO_BYTE_FOR_SAMPLE		6
#define LIS2DS_MAX_FIFO_LENGTH			256
#define LIS2DS_MAX_FIFO_SIZE			(LIS2DS_MAX_FIFO_LENGTH * \
						LIS2DS_FIFO_BYTE_FOR_SAMPLE)
#define LIS2DS_MIN_EVENT_ODR			25

enum {
	OFF = 0,
	ON = 1,
};

#ifndef ABS
#define ABS(a)		((a) > 0 ? (a) : -(a))
#endif

#ifndef MAX
#define MAX(a, b)				(((a) > (b)) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a, b)				(((a) < (b)) ? (a) : (b))
#endif

#define CHECK_BIT(x, y)				(x & (1 << y))

#define SENSOR_DATA_X(datax, datay, dataz, x1, y1, z1, x2, y2, z2, x3, y3, z3) \
				((x1 == 1 ? datax : (x1 == -1 ? -datax : 0)) + \
				(x2 == 1 ? datay : (x2 == -1 ? -datay : 0)) + \
				(x3 == 1 ? dataz : (x3 == -1 ? -dataz : 0)))

#define SENSOR_DATA_Y(datax, datay, dataz, x1, y1, z1, x2, y2, z2, x3, y3, z3) \
				((y1 == 1 ? datax : (y1 == -1 ? -datax : 0)) + \
				(y2 == 1 ? datay : (y2 == -1 ? -datay : 0)) + \
				(y3 == 1 ? dataz : (y3 == -1 ? -dataz : 0)))

#define SENSOR_DATA_Z(datax, datay, dataz, x1, y1, z1, x2, y2, z2, x3, y3, z3) \
				((z1 == 1 ? datax : (z1 == -1 ? -datax : 0)) + \
				(z2 == 1 ? datay : (z2 == -1 ? -datay : 0)) + \
				(z3 == 1 ? dataz : (z3 == -1 ? -dataz : 0)))

#define SENSOR_X_DATA(...)			SENSOR_DATA_X(__VA_ARGS__)
#define SENSOR_Y_DATA(...)			SENSOR_DATA_Y(__VA_ARGS__)
#define SENSOR_Z_DATA(...)			SENSOR_DATA_Z(__VA_ARGS__)

static const struct lis2ds_sensor_name {
	const char *name;
	const char *description;
} lis2ds_sensor_name[LIS2DS_SENSORS_NUMB] = {
	[LIS2DS_ACCEL] = {
			.name = "accel",
			.description = "ST LIS2DS Accelerometer Sensor",
	},
	[LIS2DS_STEP_C] = {
			.name = "step_c",
			.description = "ST LIS2DS Step Counter Sensor",
	},
	[LIS2DS_STEP_D] = {
			.name = "step_d",
			.description = "ST LIS2DS Step Detector Sensor",
	},
	[LIS2DS_TILT] = {
			.name = "tilt",
			.description = "ST LIS2DS Tilt Sensor",
	},
	[LIS2DS_SIGN_M] = {
			.name = "sign_m",
			.description = "ST LIS2DS Significant Motion Sensor",
	},
};

struct lis2ds_odr_reg {
	u32 hz;
	u8 value;
};

static const struct lis2ds_odr_table_t {
	u8 addr;
	u8 mask;
	struct lis2ds_odr_reg odr_avl[LIS2DS_MODE_COUNT][LIS2DS_ODR_LP_LIST_NUM];
} lis2ds_odr_table = {
	.addr = LIS2DS_ODR_ADDR,
	.mask = LIS2DS_ODR_MASK,

	.odr_avl[LIS2DS_LP_MODE][0] = { .hz = 0,
					.value = LIS2DS_ODR_POWER_OFF_VAL },
	.odr_avl[LIS2DS_LP_MODE][1] = { .hz = 1,
					.value = LIS2DS_ODR_1HZ_LP_VAL },
	.odr_avl[LIS2DS_LP_MODE][2] = { .hz = 12,
					.value = LIS2DS_ODR_12HZ_LP_VAL },
	.odr_avl[LIS2DS_LP_MODE][3] = { .hz = 25,
					.value = LIS2DS_ODR_25HZ_LP_VAL },
	.odr_avl[LIS2DS_LP_MODE][4] = { .hz = 50,
					.value = LIS2DS_ODR_50HZ_LP_VAL },
	.odr_avl[LIS2DS_LP_MODE][5] = { .hz = 100,
					.value = LIS2DS_ODR_100HZ_LP_VAL },
	.odr_avl[LIS2DS_LP_MODE][6] = { .hz = 200,
					.value = LIS2DS_ODR_200HZ_LP_VAL },
	.odr_avl[LIS2DS_LP_MODE][7] = { .hz = 400,
					.value = LIS2DS_ODR_400HZ_LP_VAL },
	.odr_avl[LIS2DS_LP_MODE][8] = { .hz = 800,
					.value = LIS2DS_ODR_800HZ_LP_VAL },

	.odr_avl[LIS2DS_HR_MODE][0] = { .hz = 0,
					.value = LIS2DS_ODR_POWER_OFF_VAL },
	.odr_avl[LIS2DS_HR_MODE][1] = { .hz = 12,
					.value = LIS2DS_ODR_12_5HZ_HR_VAL },
	.odr_avl[LIS2DS_HR_MODE][2] = { .hz = 25,
					.value = LIS2DS_ODR_25HZ_HR_VAL },
	.odr_avl[LIS2DS_HR_MODE][3] = { .hz = 50,
					.value = LIS2DS_ODR_50HZ_HR_VAL },
	.odr_avl[LIS2DS_HR_MODE][4] = { .hz = 100,
					.value = LIS2DS_ODR_100HZ_HR_VAL },
	.odr_avl[LIS2DS_HR_MODE][5] = { .hz = 200,
					.value = LIS2DS_ODR_200HZ_HR_VAL },
	.odr_avl[LIS2DS_HR_MODE][6] = { .hz = 400,
					.value = LIS2DS_ODR_400HZ_HR_VAL },
	.odr_avl[LIS2DS_HR_MODE][7] = { .hz = 800,
					.value = LIS2DS_ODR_800HZ_HR_VAL },
};

struct lis2ds_fs_reg {
	unsigned int gain;
	u8 value;
	int urv;
};

static struct lis2ds_fs_table {
	u8 addr;
	u8 mask;
	struct lis2ds_fs_reg fs_avl[LIS2DS_FS_LIST_NUM];
} lis2ds_fs_table = {
	.addr = LIS2DS_FS_ADDR,
	.mask = LIS2DS_FS_MASK,
	.fs_avl[0] = { .gain = LIS2DS_FS_2G_GAIN,
			.value = LIS2DS_FS_2G_VAL,
			.urv = 2, },
	.fs_avl[1] = { .gain = LIS2DS_FS_4G_GAIN,
			.value = LIS2DS_FS_4G_VAL,
			.urv = 4, },
	.fs_avl[2] = { .gain = LIS2DS_FS_8G_GAIN,
			.value = LIS2DS_FS_8G_VAL,
			.urv = 8, },
	.fs_avl[3] = { .gain = LIS2DS_FS_16G_GAIN,
			.value = LIS2DS_FS_16G_VAL,
			.urv = 16, },
};

static int lis2ds_enable_sensors(struct lis2ds_data *cdata, int sindex);
static int lis2ds_disable_sensors(struct lis2ds_data *cdata, int sindex);

static inline s64 lis2ds_get_time_ns(void)
{
	struct timespec ts;
	/*
	 * calls getnstimeofday.
	 * If hrtimers then up to ns accurate, if not microsecond.
	 */
	get_monotonic_boottime(&ts);

	return timespec_to_ns(&ts);
}

static int lis2ds_write_data_with_mask(struct lis2ds_data *cdata,
				       u8 reg_addr, u8 mask, u8 data,
				       bool b_lock)
{
	int err;
	u8 new_data = 0x00, old_data = 0x00;

	err = cdata->tf->read(cdata, reg_addr, 1, &old_data, b_lock);
	if (err < 0)
		return err;

	new_data = ((old_data & (~mask)) | ((data << __ffs(mask)) & mask));

	if (new_data == old_data)
		return 1;

	return cdata->tf->write(cdata, reg_addr, 1, &new_data, b_lock);
}

int lis2ds_acc_open_calibration(struct lis2ds_data *cdata);

static int lis2ds_report_step_c_data(struct lis2ds_data *cdata, u16 *steps)
{
	int err = 0;
	u8 data[2];

	err = cdata->tf->read(cdata, LIS2DS_STEP_C_OUT_L_ADDR,
			      LIS2DS_STEP_C_OUT_SIZE, data, true);
	if (err < 0)
		return err;

	*steps = ((u16)(data[1] << 8) | data[0]);
	SENSOR_INFO("steps : %d\n", (data[1] << 8) | data[0]);

	return 0;
}

static inline s32 lis2ds_data_align_bit(u8 ms, u8 ls)
{
	return (s32)((s16)(ls | ms << 8));
}

static int lis2ds_get_acc_data(struct lis2ds_data *cdata, int *xyz)
{
	u8 data[6];
	int err;

	err = cdata->tf->read(cdata, LIS2DS_OUTX_L_ADDR, LIS2DS_OUT_XYZ_SIZE,
			      data, true);
	if (err < 0) {
		SENSOR_ERR("get acc data failed %d\n", err);
		return err;
	} else {
		xyz[0] = lis2ds_data_align_bit(data[1], data[0]);
		xyz[1] = lis2ds_data_align_bit(data[3], data[2]);
		xyz[2] = lis2ds_data_align_bit(data[5], data[4]);
		return 0;
	}
}

static enum hrtimer_restart lis2ds_acc_timer_func(struct hrtimer *timer)
{
	struct lis2ds_data *cdata = container_of(timer,
					struct lis2ds_data, acc_timer);

	if (!work_pending(&cdata->acc_work))
		queue_work(cdata->accel_wq, &cdata->acc_work);

	hrtimer_forward_now(&cdata->acc_timer, cdata->acc_delay);

	return HRTIMER_RESTART;
}

static void lis2ds_acc_work_func(struct work_struct *work)
{
	struct lis2ds_data *cdata =
		container_of(work, struct lis2ds_data, acc_work);

	struct timespec ts = ktime_to_timespec(ktime_get_boottime());
	u64 timestamp_new = ts.tv_sec * 1000000000ULL + ts.tv_nsec;
	int time_hi, time_lo;
	int n;
	int err;
	int data[3];
	s16 tmp_data[3], raw_data[3];

	err = lis2ds_get_acc_data(cdata, data);
	if (err < 0)
		goto exit;

	for (n = 0; n < 3; n++) {
		raw_data[n] = *((s16 *)&data[n]);
	}

	tmp_data[0] = SENSOR_X_DATA(raw_data[0], raw_data[1], raw_data[2],
		cdata->orientation[0], cdata->orientation[1],
		cdata->orientation[2], cdata->orientation[3],
		cdata->orientation[4], cdata->orientation[5],
		cdata->orientation[6], cdata->orientation[7],
		cdata->orientation[8]);
	tmp_data[1] = SENSOR_Y_DATA(raw_data[0], raw_data[1], raw_data[2],
		cdata->orientation[0], cdata->orientation[1],
		cdata->orientation[2], cdata->orientation[3],
		cdata->orientation[4], cdata->orientation[5],
		cdata->orientation[6], cdata->orientation[7],
		cdata->orientation[8]);
	tmp_data[2] = SENSOR_Z_DATA(raw_data[0], raw_data[1], raw_data[2],
		cdata->orientation[0], cdata->orientation[1],
		cdata->orientation[2], cdata->orientation[3],
		cdata->orientation[4], cdata->orientation[5],
		cdata->orientation[6], cdata->orientation[7],
		cdata->orientation[8]);

	cdata->accel_data[0] = tmp_data[0] - cdata->accel_cal_data[0];
	cdata->accel_data[1] = tmp_data[1] - cdata->accel_cal_data[1];
	cdata->accel_data[2] = tmp_data[2] - cdata->accel_cal_data[2];

	time_hi = (int)((timestamp_new & TIME_HI_MASK) >> TIME_HI_SHIFT);
	time_lo = (int)(timestamp_new & TIME_LO_MASK);

	input_report_rel(cdata->acc_input, REL_X, cdata->accel_data[0]);
	input_report_rel(cdata->acc_input, REL_Y, cdata->accel_data[1]);
	input_report_rel(cdata->acc_input, REL_Z, cdata->accel_data[2]);
	input_report_rel(cdata->acc_input, REL_DIAL, time_hi);
	input_report_rel(cdata->acc_input, REL_MISC, time_lo);
	input_sync(cdata->acc_input);

exit:
	if ((ktime_to_ns(cdata->acc_delay) * cdata->acc_time_count)
			>= ((int64_t)ACCEL_LOG_TIME * NSEC_PER_SEC)) {
		SENSOR_INFO("x = %d, y = %d, z = %d\n",
			cdata->accel_data[0],
			cdata->accel_data[1], cdata->accel_data[2]);
		cdata->acc_time_count = 0;
	} else {
		cdata->acc_time_count++;
	}
}

static int lis2ds_update_drdy_irq(struct lis2ds_data *cdata, int sindex, bool enable)
{
	u8 reg_val = 0, reg_mask = 0;

	if (enable)
		reg_val = LIS2DS_EN_BIT;

	switch (sindex) {
	case LIS2DS_SIGN_M:
		reg_mask = LIS2DS_INT2_SIG_MOT_DET_MASK;
		SENSOR_INFO("reg_mask (%d)\n", reg_mask);
		break;
	case LIS2DS_TILT:
		reg_mask = LIS2DS_INT2_TILT_MASK;
		SENSOR_INFO("reg_mask (%d)\n", reg_mask);
		break;
	case LIS2DS_STEP_D:
		if (cdata->sensors[LIS2DS_STEP_C].enabled)
			return 0;
		reg_mask = LIS2DS_INT2_STEP_DET_MASK;
		SENSOR_INFO("reg_mask (%d)\n", reg_mask);
		break;
	case LIS2DS_STEP_C:
		if (cdata->sensors[LIS2DS_STEP_D].enabled)
			return 0;
		reg_mask = LIS2DS_INT2_STEP_DET_MASK;
		SENSOR_INFO("reg_mask (%d)\n", reg_mask);
		break;
	default:
		return -EINVAL;
	}
	SENSOR_INFO("sindex (%d), en: %d\n", sindex, enable);

	return lis2ds_write_data_with_mask(cdata,
					   LIS2DS_CTRL5_INT2_PAD_ADDR,
					   reg_mask,
					   reg_val,
					   true);
}

static int lis2ds_set_fs(struct lis2ds_data *cdata, int sindex, unsigned int fs)
{
	int err, i;

	for (i = 0; i < LIS2DS_FS_LIST_NUM; i++) {
		if (lis2ds_fs_table.fs_avl[i].urv == fs)
			break;
	}

	if (i == LIS2DS_FS_LIST_NUM)
		return -EINVAL;

	err = lis2ds_write_data_with_mask(cdata,
					  lis2ds_fs_table.addr,
					  lis2ds_fs_table.mask,
					  lis2ds_fs_table.fs_avl[i].value,
					  true);
	if (err < 0)
		return err;

	return 0;
}

static void lis2ds_sa_irq_work(struct work_struct *work)
{
	struct lis2ds_data *cdata;

	cdata = container_of((struct delayed_work *)work,
				struct lis2ds_data, sa_irq_work);

	lis2ds_write_data_with_mask(cdata,
					   LIS2DS_CTRL4_INT1_PAD_ADDR,
					   LIS2DS_INT1_WAKEUP_MASK,
					   0x00, true);
}

static void lis2ds_event_management(struct work_struct *data_work)
{
	struct lis2ds_data *cdata;
	u8 int_reg_val = 0x00, ck_gate_val = 0x00;
	u16 step_cnt = 0;
	struct timespec ts = ktime_to_timespec(ktime_get_boottime());
	u64 timestamp_new = ts.tv_sec * 1000000000ULL + ts.tv_nsec;
	int time_hi, time_lo;

	cdata = container_of((struct work_struct *)data_work,
			     struct lis2ds_data, data_work);

	time_hi = (int)((timestamp_new & TIME_HI_MASK) >> TIME_HI_SHIFT);
	time_lo = (int)(timestamp_new & TIME_LO_MASK);

	cdata->tf->read(cdata, LIS2DS_STATUS_DUP_ADDR, 1, &int_reg_val, true);
	cdata->tf->read(cdata, LIS2DS_FUNC_CK_GATE_ADDR, 1, &ck_gate_val, true);

	SENSOR_INFO("int_reg_val(0x%x), ck_gate_val(0x%x)\n", int_reg_val, ck_gate_val);

	if ((cdata->sa_factory_flag) ||
	    (int_reg_val & LIS2DS_WAKE_UP_IA_MASK)) {
		cdata->sa_irq_state = 1;
		wake_lock_timeout(&cdata->sa_wake_lock, msecs_to_jiffies(3000));
		schedule_delayed_work(&cdata->sa_irq_work,
					msecs_to_jiffies(100));
	}

	if ((cdata->sensors[LIS2DS_TILT].enabled) &&
	    (ck_gate_val & LIS2DS_FUNC_CK_GATE_TILT_INT_MASK)) {
		input_report_rel(cdata->tilt_input, REL_MISC, 1);
		input_sync(cdata->tilt_input);
		wake_lock_timeout(&cdata->sa_wake_lock, msecs_to_jiffies(3000));
	}

	if ((cdata->sensors[LIS2DS_SIGN_M].enabled) &&
	    (ck_gate_val & LIS2DS_FUNC_CK_GATE_SIGN_M_DET_MASK)) {
		input_report_rel(cdata->smd_input, REL_MISC, 1);
		input_sync(cdata->smd_input);
		wake_lock_timeout(&cdata->sa_wake_lock, msecs_to_jiffies(3000));
	}

	if ((cdata->sensors[LIS2DS_STEP_D].enabled) &&
	    (ck_gate_val & LIS2DS_FUNC_CK_GATE_STEP_D_MASK)) {
		input_report_rel(cdata->sd_input, REL_MISC, 1);
		input_report_rel(cdata->sd_input, REL_X, time_hi);
		input_report_rel(cdata->sd_input, REL_Y, time_lo);
		input_sync(cdata->sd_input);
	}

	if ((cdata->sensors[LIS2DS_STEP_C].enabled) &&
	    (ck_gate_val & LIS2DS_FUNC_CK_GATE_STEP_D_MASK)) {
		mutex_lock(&cdata->mutex_read);
		lis2ds_report_step_c_data(cdata, &step_cnt);
		mutex_unlock(&cdata->mutex_read);
		cdata->last_steps_c = cdata->steps_c + step_cnt;
		SENSOR_INFO("last_steps_c : %lld, steps_c : %lld, step_cnt : %d\n", cdata->last_steps_c, cdata->steps_c, step_cnt);
		input_report_rel(cdata->sc_input, REL_MISC, cdata->last_steps_c+1);
		input_report_rel(cdata->sc_input, REL_X, time_hi);
		input_report_rel(cdata->sc_input, REL_Y, time_lo);
		input_sync(cdata->sc_input);
	}

}

static irqreturn_t lis2ds_thread_fn(int irq, void *private)
{
	struct lis2ds_data *cdata = private;

	queue_work(cdata->irq_wq, &cdata->data_work);

	return IRQ_HANDLED;
}

static int lis2ds_reset_step_counter(struct lis2ds_data *cdata)
{
	int err;

	err = lis2ds_write_data_with_mask(cdata,
					  LIS2DS_STEP_C_MINTHS_ADDR,
					  LIS2DS_STEP_C_MINTHS_RST_NSTEP_MASK,
					  LIS2DS_EN_BIT, true);
	if (err < 0)
		return err;

	return 0;
}

void lis2ds_set_irq(struct lis2ds_data *cdata, bool enable)
{
	if (enable) {
		cdata->states++;
		SENSOR_INFO("enable state count:(%d)\n", cdata->states);
		if (cdata->states == 1) {
			SENSOR_INFO("enable irq come (%d)\n", cdata->irq);
			enable_irq_wake(cdata->irq);
			enable_irq(cdata->irq);
		}
	} else if (cdata->states != 0 && enable == 0) {
		cdata->states--;
		if (cdata->states == 0) {
			disable_irq_wake(cdata->irq);
			disable_irq_nosync(cdata->irq);
			SENSOR_INFO("disable irq come (%d)\n", cdata->irq);
		}
		SENSOR_INFO("disable state count:(%d)\n", cdata->states);
	}

}

static int lis2ds_update_event_functions(struct lis2ds_data *cdata, int sindex, bool enable)
{
	u8 reg_mask = 0, reg_val = 0;

	if (enable)
		reg_val = LIS2DS_EN_BIT;

	switch (sindex) {
	case LIS2DS_SIGN_M:
		reg_mask = LIS2DS_FUNC_CTRL_SIGN_MOT_MASK;
		SENSOR_INFO("reg_mask (%d)\n", reg_mask);
		break;
	case LIS2DS_TILT:
		reg_mask = LIS2DS_INT2_TILT_MASK;
		SENSOR_INFO("reg_mask (%d)\n", reg_mask);
		break;
	case LIS2DS_STEP_D:
		if (cdata->sensors[LIS2DS_STEP_C].enabled)
			return 0;
		reg_mask = LIS2DS_FUNC_CTRL_STEP_CNT_MASK;
		SENSOR_INFO("reg_mask (%d)\n", reg_mask);
		break;
	case LIS2DS_STEP_C:
		if (cdata->sensors[LIS2DS_STEP_D].enabled)
			return 0;
		reg_mask = LIS2DS_FUNC_CTRL_STEP_CNT_MASK;
		SENSOR_INFO("reg_mask (%d)\n", reg_mask);
		break;
	default:
		return -EINVAL;
	}
	SENSOR_INFO("sindex (%d), en: %d\n", sindex, enable);

	return lis2ds_write_data_with_mask(cdata,
					   LIS2DS_FUNC_CTRL_ADDR,
					   reg_mask,
					   reg_val,
					   true);
}

static int lis2ds_enable_step_counter(struct lis2ds_data *cdata,
				    bool enable)
{
	int err;

	cdata->steps_c = cdata->last_steps_c;
	lis2ds_reset_step_counter(cdata);

	if (!enable)
		return 0;

	/* Step counter */
	/* FS 4G / 7 x 62.5 = 437mg */
	err = lis2ds_write_data_with_mask(cdata,
					  LIS2DS_STEP_C_MINTHS_ADDR,
					  0x7f,
					  0x47, true);
	if (err < 0)
		return err;
	err = lis2ds_write_data_with_mask(cdata,
					  LIS2DS_CTRL2_ADDR,
					  0x10,
					  LIS2DS_EN_BIT, true);
	if (err < 0)
		return err;
	/* DEB_TIME = 0x0B(11*80ms=880ms), DEB_STEP = 0x07(7Step) */
	err = lis2ds_write_data_with_mask(cdata,
					  0x2b,
					  0xff,
					  0x5f, true);
	if (err < 0)
		return err;
	/* 0 sec (Default value) */
	err = lis2ds_write_data_with_mask(cdata,
					  0x3a,
					  0xff,
					  0x00, true);
	if (err < 0)
		return err;
	err = lis2ds_write_data_with_mask(cdata,
					  LIS2DS_FUNC_CTRL_ADDR,
					  0x10,
					  LIS2DS_DIS_BIT, true);
	if (err < 0)
		return err;

	return 0;
}

static int lis2ds_enable_sensors(struct lis2ds_data *cdata, int sindex)
{
	int err = 0;
	u8 func = 0x00, int2 = 0x00;

	SENSOR_INFO("sindex = %d\n", sindex);
	cdata->sensors[sindex].enabled = true;

	switch (sindex) {
	case LIS2DS_ACCEL:
		err = lis2ds_write_data_with_mask(cdata,
					  lis2ds_odr_table.addr,
					  lis2ds_odr_table.mask,
					  LIS2DS_ODR_200HZ_HR_VAL,
					  true);
		SENSOR_INFO("ODR = %d\n", LIS2DS_ODR_200HZ_HR_VAL);
		if (err < 0)
			return err;

		hrtimer_start(&cdata->acc_timer, cdata->acc_delay,
					HRTIMER_MODE_REL);
		break;
	case LIS2DS_TILT:
	case LIS2DS_SIGN_M:
		if (!cdata->sensors[LIS2DS_ACCEL].enabled) {
			err = lis2ds_write_data_with_mask(cdata,
					  lis2ds_odr_table.addr,
					  lis2ds_odr_table.mask,
					  LIS2DS_ODR_25HZ_HR_VAL,
					  true);
		SENSOR_INFO("ODR = %d\n", LIS2DS_ODR_25HZ_HR_VAL);
			if (err < 0)
				return err;
		}

		err = lis2ds_update_event_functions(cdata, sindex, true);
		if (err < 0)
			return err;

		err = lis2ds_update_drdy_irq(cdata, sindex, true);
		if (err < 0)
			return err;

		cdata->tf->read(cdata, LIS2DS_FUNC_CTRL_ADDR, 1, &func, true);
		cdata->tf->read(cdata, LIS2DS_CTRL5_INT2_PAD_ADDR, 1, &int2, true);
		SENSOR_INFO("func(0x%x), int2(0x%x)\n", func, int2);
		break;

	case LIS2DS_STEP_D:
	case LIS2DS_STEP_C:
		err = lis2ds_enable_step_counter(cdata, true);
		if (err < 0)
			return err;

		if (!cdata->sensors[LIS2DS_ACCEL].enabled) {
			err = lis2ds_write_data_with_mask(cdata,
					  lis2ds_odr_table.addr,
					  lis2ds_odr_table.mask,
					  LIS2DS_ODR_25HZ_HR_VAL,
					  true);

		SENSOR_INFO("ODR = %d\n", LIS2DS_ODR_25HZ_HR_VAL);
			if (err < 0)
				return err;
		}

		err = lis2ds_update_event_functions(cdata, sindex, true);
		if (err < 0)
			return err;

		err = lis2ds_update_drdy_irq(cdata, sindex, true);
		if (err < 0)
			return err;

		cdata->tf->read(cdata, LIS2DS_FUNC_CTRL_ADDR, 1, &func, true);
		cdata->tf->read(cdata, LIS2DS_CTRL5_INT2_PAD_ADDR, 1, &int2, true);
		SENSOR_INFO("func(0x%x), int2(0x%x)\n", func, int2);

		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static int lis2ds_disable_sensors(struct lis2ds_data *cdata, int sindex)
{
	int err;
	u8 func = 0x00, int2 = 0x00;

	SENSOR_INFO("sindex = %d\n", sindex);
	cdata->sensors[sindex].enabled = false;

	switch (sindex) {
	case LIS2DS_TILT:
	case LIS2DS_SIGN_M:
		if (!(cdata->sensors[LIS2DS_ACCEL].enabled
			|| cdata->sensors[LIS2DS_TILT].enabled
			|| cdata->sensors[LIS2DS_SIGN_M].enabled
			|| cdata->sensors[LIS2DS_STEP_C].enabled
			|| cdata->sensors[LIS2DS_STEP_D].enabled
			|| cdata->sa_flag)) {
			err = lis2ds_write_data_with_mask(cdata,
					  lis2ds_odr_table.addr,
					  lis2ds_odr_table.mask,
					  LIS2DS_ODR_POWER_OFF_VAL,
					  true);
		SENSOR_INFO("ODR = %d\n", LIS2DS_ODR_POWER_OFF_VAL);
			if (err < 0)
				return err;
		}
		err = lis2ds_update_event_functions(cdata, sindex, false);
		if (err < 0)
			return err;

		err = lis2ds_update_drdy_irq(cdata, sindex, false);
		if (err < 0)
			return err;

		cdata->tf->read(cdata, LIS2DS_FUNC_CTRL_ADDR, 1, &func, true);
		cdata->tf->read(cdata, LIS2DS_CTRL5_INT2_PAD_ADDR, 1, &int2, true);
		SENSOR_INFO("func(0x%x), int2(0x%x)\n", func, int2);

		break;

	case LIS2DS_STEP_D:
	case LIS2DS_STEP_C:
		err = lis2ds_enable_step_counter(cdata, false);
		if (err < 0)
			return err;

		if (!(cdata->sensors[LIS2DS_ACCEL].enabled
			|| cdata->sensors[LIS2DS_TILT].enabled
			|| cdata->sensors[LIS2DS_SIGN_M].enabled
			|| cdata->sensors[LIS2DS_STEP_C].enabled
			|| cdata->sensors[LIS2DS_STEP_D].enabled
			|| cdata->sa_flag)) {
			err = lis2ds_write_data_with_mask(cdata,
					  lis2ds_odr_table.addr,
					  lis2ds_odr_table.mask,
					  LIS2DS_ODR_POWER_OFF_VAL,
					  true);
		SENSOR_INFO("ODR = %d\n", LIS2DS_ODR_POWER_OFF_VAL);
			if (err < 0)
				return err;
		}
		err = lis2ds_update_event_functions(cdata, sindex, false);
		if (err < 0)
			return err;

		err = lis2ds_update_drdy_irq(cdata, sindex, false);
		if (err < 0)
			return err;

		cdata->tf->read(cdata, LIS2DS_FUNC_CTRL_ADDR, 1, &func, true);
		cdata->tf->read(cdata, LIS2DS_CTRL5_INT2_PAD_ADDR, 1, &int2, true);
		SENSOR_INFO("func(0x%x), int2(0x%x)\n", func, int2);

		break;

	case LIS2DS_ACCEL:
		if (cdata->sensors[LIS2DS_TILT].enabled
			|| cdata->sensors[LIS2DS_SIGN_M].enabled
			|| cdata->sensors[LIS2DS_STEP_C].enabled
			|| cdata->sensors[LIS2DS_STEP_D].enabled
			|| cdata->sa_flag) {
			err = lis2ds_write_data_with_mask(cdata,
					  lis2ds_odr_table.addr,
					  lis2ds_odr_table.mask,
					  LIS2DS_ODR_25HZ_HR_VAL,
					  true);
		SENSOR_INFO("ODR = %d\n", LIS2DS_ODR_25HZ_HR_VAL);

		} else {
	    		err = lis2ds_write_data_with_mask(cdata,
					  lis2ds_odr_table.addr,
					  lis2ds_odr_table.mask,
					  LIS2DS_ODR_POWER_OFF_VAL,
					  true);
		SENSOR_INFO("ODR = %d\n", LIS2DS_ODR_POWER_OFF_VAL);

		}

		if (err < 0)
			return err;

		cancel_work_sync(&cdata->acc_work);
		hrtimer_cancel(&cdata->acc_timer);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static int lis2ds_init_sensors(struct lis2ds_data *cdata, int sindex)
{
	int err;

	/*
	 * Soft reset the device on power on.
	 */
	err = lis2ds_write_data_with_mask(cdata,
					  LIS2DS_SOFT_RESET_ADDR,
					  LIS2DS_SOFT_RESET_MASK,
					  LIS2DS_EN_BIT, true);
	if (err < 0)
		return err;

	cdata->selftest_status = 0;
	cdata->power_mode = LIS2DS_HR_MODE;

	err = lis2ds_set_fs(cdata, LIS2DS_ACCEL, LIS2DS_ACCEL_FS);
	if (err < 0)
		return err;

	/*
	 * Enable latched interrupt mode.
	 */
	err = lis2ds_write_data_with_mask(cdata,
					  LIS2DS_LIR_ADDR,
					  LIS2DS_LIR_MASK,
					  LIS2DS_EN_BIT, true);
	if (err < 0)
		return err;
	/*
	 * Enable block data update feature.
	 */
	err = lis2ds_write_data_with_mask(cdata,
					  LIS2DS_BDU_ADDR,
					  LIS2DS_BDU_MASK,
					  LIS2DS_EN_BIT, true);
	if (err < 0)
		return err;

	/*
	 * Route interrupt from INT2 to INT1 pin.
	 */
	err = lis2ds_write_data_with_mask(cdata,
					  LIS2DS_INT2_ON_INT1_ADDR,
					  LIS2DS_INT2_ON_INT1_MASK,
					  LIS2DS_EN_BIT, true);
	if (err < 0)
		return err;

	/*
	 * Set the wake_up_ths max to be prevented from false alert
	 */
	err = lis2ds_write_data_with_mask(cdata,
					   LIS2DS_WAKE_UP_THS_ADDR,
					   LIS2DS_WAKE_UP_THS_WU_MASK,
					   0x3f, true);

	return 0;
}

static ssize_t lis2ds_acc_enable_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct lis2ds_data *cdata = input_get_drvdata(input);

	return sprintf(buf, "%u\n", atomic_read(&cdata->acc_wkqueue_en));
}

static ssize_t lis2ds_acc_enable_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct input_dev *input = to_input_dev(dev);
	struct lis2ds_data *cdata = input_get_drvdata(input);

	int err;
	u8 enable;
	int pre_enable = atomic_read(&cdata->acc_wkqueue_en);

	err = kstrtou8(buf, 10, &enable);
	if (err)
		return count;

	enable = enable ? 1 : 0;

	SENSOR_INFO("new_value = %d, pre_enable = %d\n", enable, pre_enable);

	mutex_lock(&cdata->mutex_enable);

	if (enable) {
		if (pre_enable == 0) {
			lis2ds_acc_open_calibration(cdata);
			err = lis2ds_set_fs(cdata, LIS2DS_ACCEL, LIS2DS_ACCEL_FS);
			err = lis2ds_enable_sensors(cdata, LIS2DS_ACCEL);
			atomic_set(&cdata->acc_wkqueue_en, 1);
		}
	} else {
		if (pre_enable == 1) {
			atomic_set(&cdata->acc_wkqueue_en, 0);
			err = lis2ds_disable_sensors(cdata, LIS2DS_ACCEL);
		}
	}
	mutex_unlock(&cdata->mutex_enable);
	return count;
}

static ssize_t lis2ds_smd_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct lis2ds_data *cdata = input_get_drvdata(input);
	int ret;

	if (cdata->sensors[LIS2DS_SIGN_M].enabled)
		ret = 1;
	else
		ret = 0;

	return snprintf(buf, 16, "%d\n", ret);
}

static ssize_t lis2ds_smd_enable_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct input_dev *input = to_input_dev(dev);
	struct lis2ds_data *cdata = input_get_drvdata(input);

	int err;
	u8 enable;

	err = kstrtou8(buf, 10, &enable);
	if (err)
		return count;

	enable = enable ? 1 : 0;

	SENSOR_INFO("new_value = %d\n", enable);

	mutex_lock(&cdata->mutex_enable);
	if (enable) {
		err = lis2ds_enable_sensors(cdata, LIS2DS_SIGN_M);
		lis2ds_set_irq(cdata, 1);
	} else {
		lis2ds_set_irq(cdata, 0);
		err = lis2ds_disable_sensors(cdata, LIS2DS_SIGN_M);
	}
	mutex_unlock(&cdata->mutex_enable);
	return count;
}


static ssize_t lis2ds_tilt_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct lis2ds_data *cdata = input_get_drvdata(input);
	int ret;

	if (cdata->sensors[LIS2DS_TILT].enabled)
		ret = 1;
	else
		ret = 0;

	return snprintf(buf, 16, "%d\n", ret);

}

static ssize_t lis2ds_tilt_enable_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct input_dev *input = to_input_dev(dev);
	struct lis2ds_data *cdata = input_get_drvdata(input);

	int err;
	u8 enable;

	err = kstrtou8(buf, 10, &enable);
	if (err)
		return count;

	enable = enable ? 1 : 0;

	SENSOR_INFO("new_value = %d\n", enable);

	mutex_lock(&cdata->mutex_enable);

	if (enable) {
			err = lis2ds_enable_sensors(cdata, LIS2DS_TILT);
			lis2ds_set_irq(cdata, 1);

	} else {

			lis2ds_set_irq(cdata, 0);
			err = lis2ds_disable_sensors(cdata, LIS2DS_TILT);
	}
	mutex_unlock(&cdata->mutex_enable);
	return count;
}


static ssize_t lis2ds_step_counter_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct lis2ds_data *cdata = input_get_drvdata(input);
	int ret;

	if (cdata->sensors[LIS2DS_STEP_C].enabled)
		ret = 1;
	else
		ret = 0;

	return snprintf(buf, 16, "%d\n", ret);

}

static ssize_t lis2ds_step_counter_enable_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct input_dev *input = to_input_dev(dev);
	struct lis2ds_data *cdata = input_get_drvdata(input);
	int err;
	u8 enable;

	err = kstrtou8(buf, 10, &enable);
	if (err)
		return count;

	enable = enable ? 1 : 0;

	SENSOR_INFO("new_value = %d\n", enable);
	mutex_lock(&cdata->mutex_enable);
	if (enable) {
		err = lis2ds_enable_sensors(cdata, LIS2DS_STEP_C);
		lis2ds_set_irq(cdata, 1);
		input_report_rel(cdata->sc_input, REL_MISC, cdata->last_steps_c+1);
		input_sync(cdata->sc_input);
		SENSOR_INFO("last_steps_c : %lld\n", cdata->last_steps_c);
	} else {
		lis2ds_set_irq(cdata, 0);
		err = lis2ds_disable_sensors(cdata, LIS2DS_STEP_C);
	}
	mutex_unlock(&cdata->mutex_enable);

	return count;
}


static ssize_t lis2ds_step_detector_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct lis2ds_data *cdata = input_get_drvdata(input);
	int ret;

	if (cdata->sensors[LIS2DS_STEP_D].enabled)
		ret = 1;
	else
		ret = 0;

	return snprintf(buf, 16, "%d\n", ret);

}

static ssize_t lis2ds_step_detector_enable_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct input_dev *input = to_input_dev(dev);
	struct lis2ds_data *cdata = input_get_drvdata(input);

	int err;
	u8 enable;

	err = kstrtou8(buf, 10, &enable);
	if (err)
		return count;

	enable = enable ? 1 : 0;

	SENSOR_INFO("new_value = %d\n", enable);
	mutex_lock(&cdata->mutex_enable);
	if (enable) {
			err = lis2ds_enable_sensors(cdata, LIS2DS_STEP_D);
			lis2ds_set_irq(cdata, 1);
	} else {
			lis2ds_set_irq(cdata, 0);
			err = lis2ds_disable_sensors(cdata, LIS2DS_STEP_D);
	}
	mutex_unlock(&cdata->mutex_enable);

	return count;
}

static ssize_t lis2ds_acc_delay_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct lis2ds_data *cdata = input_get_drvdata(input);

	return snprintf(buf, 16, "%lld\n", ktime_to_ns(cdata->acc_delay));
}

static ssize_t lis2ds_acc_delay_store(struct device *dev,
				struct device_attribute *attr, const char *buf,
				size_t count)
{
	struct input_dev *input = to_input_dev(dev);
	struct lis2ds_data *cdata = input_get_drvdata(input);
	int err;
	unsigned long data;

	err = kstrtoul(buf, 10, &data);
	if (err)
		return count;

	if (data == 0)
		return count;

	if (data > LIS2DS_DELAY_DEFAULT)
		data = LIS2DS_DELAY_DEFAULT;

	cdata->acc_delay = ns_to_ktime(data);

	SENSOR_INFO("new_delay = %lld\n", ktime_to_ns(cdata->acc_delay));

	return count;

}

static struct device_attribute dev_attr_acc_poll_delay =
	__ATTR(poll_delay, 0644,
	lis2ds_acc_delay_show,
	lis2ds_acc_delay_store);

static struct device_attribute dev_attr_acc_enable =
	__ATTR(enable, 0644,
	lis2ds_acc_enable_show,
	lis2ds_acc_enable_store);

static struct attribute *lis2ds_acc_attributes[] = {
	&dev_attr_acc_poll_delay.attr,
	&dev_attr_acc_enable.attr,
	NULL
};

static struct attribute_group lis2ds_accel_attribute_group = {
	.attrs = lis2ds_acc_attributes
};

static struct device_attribute dev_attr_smd_enable =
	__ATTR(enable, S_IRUGO|S_IWUSR,
	lis2ds_smd_enable_show,
	lis2ds_smd_enable_store);

static struct attribute *lis2ds_smd_attributes[] = {
	&dev_attr_smd_enable.attr,
	NULL
};

static struct attribute_group lis2ds_smd_attribute_group = {
	.attrs = lis2ds_smd_attributes
};

static struct device_attribute dev_attr_tilt_enable =
	__ATTR(enable, S_IRUGO|S_IWUSR,
	lis2ds_tilt_enable_show,
	lis2ds_tilt_enable_store);

static struct attribute *lis2ds_tilt_attributes[] = {
	&dev_attr_tilt_enable.attr,
	NULL
};

static struct attribute_group lis2ds_tilt_attribute_group = {
	.attrs = lis2ds_tilt_attributes
};

static struct device_attribute dev_attr_sc_enable =
	__ATTR(enable, S_IRUGO|S_IWUSR,
	lis2ds_step_counter_enable_show,
	lis2ds_step_counter_enable_store);

static struct attribute *lis2ds_sc_attributes[] = {
	&dev_attr_sc_enable.attr,
	NULL
};

static struct attribute_group lis2ds_sc_attribute_group = {
	.attrs = lis2ds_sc_attributes
};

static struct device_attribute dev_attr_sd_enable =
	__ATTR(enable, S_IRUGO|S_IWUSR,
	lis2ds_step_detector_enable_show,
	lis2ds_step_detector_enable_store);

static struct attribute *lis2ds_sd_attributes[] = {
	&dev_attr_sd_enable.attr,
	NULL
};

static struct attribute_group lis2ds_sd_attribute_group = {
	.attrs = lis2ds_sd_attributes
};

/* FATORY SYSFS */
static ssize_t lis2ds_vendor_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", VENDOR_NAME);
}

static ssize_t lis2ds_name_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", LIS2DS_DEV_NAME);
}

/* Accelerometer Calibraion */
int lis2ds_acc_open_calibration(struct lis2ds_data *cdata)
{
	int ret = 0;
	mm_segment_t old_fs;
	struct file *cal_filp = NULL;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(CALIBRATION_FILE_PATH, O_RDONLY, 0);
	if (IS_ERR(cal_filp)) {
		set_fs(old_fs);
		ret = PTR_ERR(cal_filp);

		cdata->accel_cal_data[0] = 0;
		cdata->accel_cal_data[1] = 0;
		cdata->accel_cal_data[2] = 0;

		SENSOR_INFO("Can't open calibration file\n");
		return ret;
	}

	ret = vfs_read(cal_filp, (char *)&cdata->accel_cal_data,
		3 * sizeof(s16), &cal_filp->f_pos);
	if (ret != 3 * sizeof(s16)) {

		cdata->accel_cal_data[0] = 0;
		cdata->accel_cal_data[1] = 0;
		cdata->accel_cal_data[2] = 0;

		SENSOR_ERR("Can't read the cal data\n");
		ret = -EIO;
	}

	filp_close(cal_filp, current->files);
	set_fs(old_fs);

	SENSOR_INFO("%d, %d, %d\n",
		cdata->accel_cal_data[0], cdata->accel_cal_data[1],
		cdata->accel_cal_data[2]);

	if ((cdata->accel_cal_data[0] == 0) && (cdata->accel_cal_data[1] == 0)
		&& (cdata->accel_cal_data[2] == 0))
		return -EIO;

	return ret;
}

static int lis2ds_acc_do_calibrate(struct lis2ds_data *cdata, int enable)
{
	int sum[3] = { 0, };
	int ret = 0, cnt;
	struct file *cal_filp = NULL;
	mm_segment_t old_fs;

	SENSOR_INFO("\n");

	if (enable) {
		cdata->accel_cal_data[0] = 0;
		cdata->accel_cal_data[1] = 0;
		cdata->accel_cal_data[2] = 0;

		for (cnt = 0; cnt < CALIBRATION_DATA_AMOUNT; cnt++) {
			sum[0] += cdata->accel_data[0];
			sum[1] += cdata->accel_data[1];
			sum[2] += cdata->accel_data[2];
			msleep(20);
		}

		cdata->accel_cal_data[0] = (sum[0] / CALIBRATION_DATA_AMOUNT);
		cdata->accel_cal_data[1] = (sum[1] / CALIBRATION_DATA_AMOUNT);
		cdata->accel_cal_data[2] = (sum[2] / CALIBRATION_DATA_AMOUNT);

		if (cdata->accel_cal_data[2] > 0)
			cdata->accel_cal_data[2] -= MAX_ACCEL_1G;
		else if (cdata->accel_cal_data[2] < 0)
			cdata->accel_cal_data[2] += MAX_ACCEL_1G;

	} else {
		cdata->accel_cal_data[0] = 0;
		cdata->accel_cal_data[1] = 0;
		cdata->accel_cal_data[2] = 0;
	}

	SENSOR_INFO("do accel calibrate %d, %d, %d\n", cdata->accel_cal_data[0],
		cdata->accel_cal_data[1], cdata->accel_cal_data[2]);

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(CALIBRATION_FILE_PATH,
		O_CREAT | O_TRUNC | O_WRONLY, 0660);
	if (IS_ERR(cal_filp)) {
		SENSOR_ERR("Can't open calibration file\n");
		set_fs(old_fs);
		ret = PTR_ERR(cal_filp);
		return ret;
	}

	ret = vfs_write(cal_filp, (char *)&cdata->accel_cal_data,
		3 * sizeof(s16), &cal_filp->f_pos);
	if (ret != 3 * sizeof(s16)) {
		SENSOR_ERR("Can't write the caldata to file\n");
		ret = -EIO;
	}

	filp_close(cal_filp, current->files);
	set_fs(old_fs);

	return ret;
}

static ssize_t lis2ds_acc_calibration_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int ret;
	struct lis2ds_data *cdata = dev_get_drvdata(dev);

	SENSOR_INFO("\n");

	ret = lis2ds_acc_open_calibration(cdata);
	if (ret < 0)
		SENSOR_ERR("calibration open failed = %d\n", ret);

	SENSOR_INFO("cal data %d %d %d, ret = %d\n", cdata->accel_cal_data[0],
		cdata->accel_cal_data[1], cdata->accel_cal_data[2], ret);

	return snprintf(buf, PAGE_SIZE, "%d %d %d %d\n", ret,
		cdata->accel_cal_data[0], cdata->accel_cal_data[1],
		cdata->accel_cal_data[2]);
}

static ssize_t lis2ds_acc_calibration_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int ret;
	int64_t dEnable;
	struct lis2ds_data *cdata = dev_get_drvdata(dev);

	SENSOR_INFO("\n");

	ret = kstrtoll(buf, 10, &dEnable);
	if (ret < 0) {
		SENSOR_ERR("kstrtoll failed\n");
		return size;
	}

	ret = lis2ds_acc_do_calibrate(cdata, (int)dEnable);
	if (ret < 0)
		SENSOR_ERR("accel calibrate failed\n");

	return size;
}

/* Accelerometer LPF */
static int lis2ds_set_lpf(struct lis2ds_data *cdata, int onoff)
{
	int err;
	u8 odr;

	SENSOR_INFO("onoff = %d\n", onoff);

	if (onoff)
		odr = LIS2DS_ODR_200HZ_HR_VAL;
	else
		odr = LIS2DS_ODR_800HZ_HR_VAL;

	err = lis2ds_write_data_with_mask(cdata,
				  lis2ds_odr_table.addr,
				  lis2ds_odr_table.mask,
				  odr,
				  true);
	if (err < 0)
		return err;

	cdata->lpf_on = onoff;

	return err;
}

static ssize_t lis2ds_lowpassfilter_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int ret = 0;
	struct lis2ds_data *cdata = dev_get_drvdata(dev);

	if (cdata->lpf_on)
		ret = 1;
	else
		ret = 0;

	return snprintf(buf, PAGE_SIZE, "%d\n", ret);
}

static ssize_t lis2ds_lowpassfilter_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int ret;
	int64_t dEnable;
	struct lis2ds_data *cdata;

	cdata = dev_get_drvdata(dev);
	SENSOR_INFO("\n");

	ret = kstrtoll(buf, 10, &dEnable);
	if (ret < 0)
		SENSOR_ERR("kstrtoll failed, ret = %d\n", ret);

	ret = lis2ds_set_lpf(cdata, dEnable);
	if (ret < 0)
		SENSOR_ERR("lis2ds_set_lpf failed, ret = %d\n", ret);

	return size;
}

/* selftest */
static int lis2ds_selftest_run(struct lis2ds_data *cdata,
			char *out_str, u8 sindex)
{
	int err, i, xyz[3], err_count = 0, result = 0;
	u8 init_status = 0, buf = 0x00;
	u8 backup_regs[1] = {0};
	u8 data[6];
	s32 NOST[3] = {0,}, POS_ST[3] = {0,}, NEG_ST[3] = {0,}, DIFF_POS_ST[3] = {0,}, DIFF_NEG_ST[3] = {0,};

	SENSOR_INFO("start\n");

	/* Read Chip ID register */
	err = cdata->tf->read(cdata, LIS2DS_WHO_AM_I_ADDR, 1, &init_status, true);
	if (err < 0) {
		SENSOR_ERR("failed to read Who-Am-I register\n");
		return err;
	}

	if (init_status != LIS2DS_WHO_AM_I_DEF)
		SENSOR_ERR("Who-Am-I register is wrong %x\n", init_status);

	/* Backup registers */
	err = cdata->tf->read(cdata,
			LIS2DS_CTRL1_ADDR,
			1, backup_regs, true);
	if (err < 0) {
		SENSOR_ERR("failed to read ctrl1 registers\n");
		goto restore_exit;
	}

	/* Initialize Sensor
	  * Set BDU=1, FS=2G, ODR = 50Hz
	  */
	buf = 0x31;
	err = cdata->tf->write(cdata, LIS2DS_CTRL1_ADDR, 1, &buf, true);
	if (err < 0) {
		SENSOR_ERR("failed to write ctrl1 register\n");
		goto restore_reg;
	}

	msleep(200);

	/* Read/discard first set of samples BEFORE self test */
	err = cdata->tf->read(cdata, LIS2DS_STATUS_ADDR, 1, data, true);
	err = cdata->tf->read(cdata, LIS2DS_OUTX_L_ADDR, LIS2DS_OUT_XYZ_SIZE,
				data, true);

	/* Read five set of samples BEFORE enabling self test */
	for (i = 0 ; i < 5; i++) {
		msleep(50);

		err = cdata->tf->read(cdata, LIS2DS_STATUS_ADDR, 1, data, true);
		err = cdata->tf->read(cdata, LIS2DS_OUTX_L_ADDR, LIS2DS_OUT_XYZ_SIZE,
				data, true);

		xyz[0] = (s32)((s16)(data[1] << 8) | data[0]);
		xyz[1] = (s32)((s16)(data[3] << 8) | data[2]);
		xyz[2] = (s32)((s16)(data[5] << 8) | data[4]);

		NOST[0] += xyz[0];
		NOST[1] += xyz[1];
		NOST[2] += xyz[2];
	}

	for (i = 0 ; i < 3; i ++) {
		NOST[i] /= 5;
		NOST[i] = LIS2DS_ACC_LSB_TO_MG(NOST[i]);
	}

	/* Enable positive sign self test */
	err = lis2ds_write_data_with_mask(cdata,
				LIS2DS_SELF_TEST_ADDR, LIS2DS_SELF_TEST_MASK,
				LIS2DS_SELF_TEST_POS_SIGN, true);

	msleep(200);

	/* Read/discard first set of samples AFTER self test */
	err = cdata->tf->read(cdata, LIS2DS_STATUS_ADDR, 1, data, true);
	err = cdata->tf->read(cdata, LIS2DS_OUTX_L_ADDR, LIS2DS_OUT_XYZ_SIZE,
				data, true);

	/* Read five set of samples AFTER enabling self test */
	for (i = 0 ; i < 5; i++) {
		msleep(50);

		err = cdata->tf->read(cdata, LIS2DS_STATUS_ADDR, 1, data, true);
		err = cdata->tf->read(cdata, LIS2DS_OUTX_L_ADDR, LIS2DS_OUT_XYZ_SIZE,
				data, true);

		xyz[0] = (s32)((s16)(data[1] << 8) | data[0]);
		xyz[1] = (s32)((s16)(data[3] << 8) | data[2]);
		xyz[2] = (s32)((s16)(data[5] << 8) | data[4]);

		POS_ST[0] += xyz[0];
		POS_ST[1] += xyz[1];
		POS_ST[2] += xyz[2];
	}

	for (i = 0 ; i < 3; i ++) {
		POS_ST[i] /= 5;
		POS_ST[i] = LIS2DS_ACC_LSB_TO_MG(POS_ST[i]);
	}

	/* Judge the selftest at positive position */
	for (i = 0; i < 3; i++) {
		DIFF_POS_ST[i] = ABS(POS_ST[i] - NOST[i]);
		if ((DIFF_POS_ST[i] < LIS2DS_ACC_MIN_ST)
			|| (DIFF_POS_ST[i] > LIS2DS_ACC_MAX_ST)) {
			err_count++;
		}
	}

	/* Enable Negative sign self test */
	err = lis2ds_write_data_with_mask(cdata,
				LIS2DS_SELF_TEST_ADDR, LIS2DS_SELF_TEST_MASK,
				LIS2DS_SELF_TEST_NEG_SIGN, true);

	msleep(200);

	/* Read/discard first set of samples AFTER self test */
	err = cdata->tf->read(cdata, LIS2DS_STATUS_ADDR, 1, data, true);
	err = cdata->tf->read(cdata, LIS2DS_OUTX_L_ADDR, LIS2DS_OUT_XYZ_SIZE,
				data, true);

	/* Read five set of samples AFTER enabling self test */
	for (i = 0 ; i < 5; i++) {
		msleep(50);

		err = cdata->tf->read(cdata, LIS2DS_STATUS_ADDR, 1, data, true);
		err = cdata->tf->read(cdata, LIS2DS_OUTX_L_ADDR, LIS2DS_OUT_XYZ_SIZE,
				data, true);

		xyz[0] = (s32)((s16)(data[1] << 8) | data[0]);
		xyz[1] = (s32)((s16)(data[3] << 8) | data[2]);
		xyz[2] = (s32)((s16)(data[5] << 8) | data[4]);

		NEG_ST[0] += xyz[0];
		NEG_ST[1] += xyz[1];
		NEG_ST[2] += xyz[2];
	}

	for (i = 0 ; i < 3; i ++) {
		NEG_ST[i] /= 5;
		NEG_ST[i] = LIS2DS_ACC_LSB_TO_MG(NEG_ST[i]);
	}

	/* Judge the selftest at negative position */
	for (i = 0; i < 3; i++) {
		DIFF_NEG_ST[i] = ABS(NEG_ST[i] - NOST[i]);
		if ((DIFF_NEG_ST[i] < LIS2DS_ACC_MIN_ST)
			|| (DIFF_NEG_ST[i] > LIS2DS_ACC_MAX_ST)) {
			err_count++;
		}
	}

	/* Disable sensor and self test */
	buf = 0x00;
	err = cdata->tf->write(cdata, LIS2DS_CTRL1_ADDR, 1, &buf, true);

	err = lis2ds_write_data_with_mask(cdata,
			LIS2DS_SELF_TEST_ADDR, LIS2DS_SELF_TEST_MASK,
			LIS2DS_SELF_TEST_NORM_M, true);

	if (err_count > 0)
		result = 0;
	else
		result = 1;

	SENSOR_INFO("average NOST %d,%d,%d\n", NOST[0], NOST[1], NOST[2]);

	SENSOR_INFO("average POS_ST %d,%d,%d\n", POS_ST[0], POS_ST[1], POS_ST[2]);
	SENSOR_INFO("DIFF_POS_ST %d,%d,%d\n", DIFF_POS_ST[0], DIFF_POS_ST[1], DIFF_POS_ST[2]);

	SENSOR_INFO("average NEG_ST %d,%d,%d\n", NEG_ST[0], NEG_ST[1], NEG_ST[2]);
	SENSOR_INFO("DIFF_NEG_ST %d,%d,%d\n", DIFF_NEG_ST[0], DIFF_NEG_ST[1], DIFF_NEG_ST[2]);

	SENSOR_INFO("err_count = %d, result = %d\n", err_count, result);

restore_reg:
	err = cdata->tf->write(cdata, LIS2DS_CTRL1_ADDR, 1, backup_regs, true);
	if (err < 0)
		SENSOR_ERR("failed to write ctrl1 registers\n");
restore_exit:
	return snprintf(out_str, PAGE_SIZE, "%d,%d,%d,%d,%d,%d,%d\n",
				result, DIFF_POS_ST[0], DIFF_POS_ST[1], DIFF_POS_ST[2], DIFF_NEG_ST[0], DIFF_NEG_ST[1], DIFF_NEG_ST[2]);
}

static ssize_t lis2ds_acc_selftest_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct lis2ds_data *cdata = dev_get_drvdata(dev);
	ssize_t ret;

	ret = lis2ds_selftest_run(cdata, buf, LIS2DS_ACCEL);

	return ret;
}

/* raw_data */
static ssize_t lis2ds_acc_raw_data_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	signed short cx, cy, cz;
	struct lis2ds_data *cdata;

	cdata = dev_get_drvdata(dev);

	cx = cdata->accel_data[0];
	cy = cdata->accel_data[1];
	cz = cdata->accel_data[2];

	return snprintf(buf, PAGE_SIZE, "%d, %d, %d\n", cx, cy, cz);
}

/* reactive alert */
static ssize_t lis2ds_smart_alert_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct lis2ds_data *cdata = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", cdata->sa_irq_state);
}

static ssize_t lis2ds_smart_alert_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct lis2ds_data *cdata = dev_get_drvdata(dev);

	u8 odr = 0x00, duration = 0x00;
	int fs, threshold;
	int enable = 0, factory_mode = 0;

	if (sysfs_streq(buf, "0")) {
		enable = 0;
		factory_mode = 0;
		if (cdata->sa_factory_flag)
			cdata->sa_factory_flag = 0;
		SENSOR_INFO("disable\n");
	} else if (sysfs_streq(buf, "1")) {
		enable = 1;
		factory_mode = 0;
		SENSOR_INFO("enable\n");
	} else if (sysfs_streq(buf, "2")) {
		enable = 1;
		factory_mode = 1;
		cdata->sa_factory_flag = 1;
		SENSOR_INFO("factory mode\n");
	} else {
		SENSOR_ERR("invalid value %s\n", buf);
		return size;
	}

	if ((enable == 1) && (cdata->sa_flag == 0)) {
		cdata->sa_irq_state = 0;
		cdata->sa_flag = 1;

		mdelay(100);

		if (factory_mode == 1) {
			threshold = 0x00;
			odr = LIS2DS_ODR_200HZ_HR_VAL;
			duration = 0x00;
		} else {
			fs = 4000;
			threshold = SA_DYNAMIC_THRESHOLD * 64;
			threshold += fs / 2;
			threshold = (threshold / fs) & 0x3f;
			SENSOR_INFO("fs= %d, thr= %d[mg] = %d\n",
				fs, SA_DYNAMIC_THRESHOLD, threshold);

			odr = LIS2DS_ODR_25HZ_HR_VAL;
			duration = 0x02;
		}

		lis2ds_write_data_with_mask(cdata,
					   LIS2DS_WAKE_UP_THS_ADDR,
					   LIS2DS_WAKE_UP_THS_WU_MASK,
					   threshold, true);

		lis2ds_write_data_with_mask(cdata,
					   LIS2DS_WAKE_UP_DUR_ADDR,
					   LIS2DS_WAKE_UP_DUR_WU_MASK,
					   duration, true);

		lis2ds_write_data_with_mask(cdata,
					   LIS2DS_CTRL4_INT1_PAD_ADDR,
					   LIS2DS_INT1_WAKEUP_MASK,
					   0x01, true);

		lis2ds_write_data_with_mask(cdata,
					  lis2ds_odr_table.addr,
					  lis2ds_odr_table.mask,
					  odr, true);

		mdelay(100);

		lis2ds_set_irq(cdata, 1);
		SENSOR_INFO("smart alert is on!\n");
	} else if ((enable == 0) && (cdata->sa_flag == 1)) {
		lis2ds_set_irq(cdata, 0);
		cdata->sa_flag = 0;

		lis2ds_write_data_with_mask(cdata,
					   LIS2DS_WAKE_UP_THS_ADDR,
					   LIS2DS_WAKE_UP_THS_WU_MASK,
					   0x3f, true);

		lis2ds_write_data_with_mask(cdata,
					   LIS2DS_CTRL4_INT1_PAD_ADDR,
					   LIS2DS_INT1_WAKEUP_MASK,
					   0x00, true);

		if(!cdata->sensors[LIS2DS_ACCEL].enabled) {
			lis2ds_write_data_with_mask(cdata,
					  lis2ds_odr_table.addr,
					  lis2ds_odr_table.mask,
					  LIS2DS_ODR_POWER_OFF_VAL,
					  true);
		}

		SENSOR_INFO("smart alert is off! irq = %d, odr 0x%x\n",
						cdata->sa_irq_state, odr);
	}

	return size;
}


static ssize_t lis2ds_write_register_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int reg, val;
	int ret;
	struct lis2ds_data *cdata = dev_get_drvdata(dev);

	if (sscanf(buf, "%2x,%2x", &reg, &val) != 2) {
		SENSOR_ERR("invalid value\n");
		return count;
	}

	ret = lis2ds_write_data_with_mask(cdata, reg, 0xff, val, true);

	if (ret < 0)
		SENSOR_ERR("failed %d\n", ret);
	else
		SENSOR_INFO("Register(0x%x) data(0x%x)\n", reg, val);

	return count;
}

static ssize_t lis2ds_read_register_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct lis2ds_data *cdata = dev_get_drvdata(dev);

	u8 reg;
	u8 reg_value = 0x00;
	int ret;

	for (reg = 0x00; reg <= 0x7f; reg++) {
		ret = cdata->tf->read(cdata, reg, 1, &reg_value, true);
	if (ret < 0)
		SENSOR_ERR("failed %d\n", ret);
	else
		SENSOR_INFO("Read Reg: 0x%x Value: 0x%x\n", reg, reg_value);
	}

	return snprintf(buf, PAGE_SIZE, "%d\n", 1);
}

static DEVICE_ATTR(acc_register, S_IRUGO | S_IWUSR | S_IWGRP,
	lis2ds_read_register_show, lis2ds_write_register_store);
static DEVICE_ATTR(vendor, 0444, lis2ds_vendor_show, NULL);
static DEVICE_ATTR(name, 0444, lis2ds_name_show, NULL);
static DEVICE_ATTR(calibration, 0644,
	lis2ds_acc_calibration_show, lis2ds_acc_calibration_store);
static DEVICE_ATTR(lowpassfilter, 0644,
	lis2ds_lowpassfilter_show, lis2ds_lowpassfilter_store);
static DEVICE_ATTR(reactive_alert, S_IWUSR | S_IRUGO,
				lis2ds_smart_alert_show,
				lis2ds_smart_alert_store);

static struct device_attribute dev_attr_acc_self_test =
	__ATTR(selftest, S_IRUGO, lis2ds_acc_selftest_show, NULL);
static struct device_attribute dev_attr_acc_raw_data =
	__ATTR(raw_data, S_IRUGO, lis2ds_acc_raw_data_show, NULL);

static struct device_attribute *acc_sensor_attrs[] = {
	&dev_attr_vendor,
	&dev_attr_name,
	&dev_attr_calibration,
	&dev_attr_lowpassfilter,
	&dev_attr_acc_self_test,
	&dev_attr_reactive_alert,
	&dev_attr_acc_raw_data,
	&dev_attr_acc_register,
	NULL
};

static struct device_attribute *smd_sensor_attrs[] = {
	&dev_attr_vendor,
	&dev_attr_name,
	NULL
};

static struct device_attribute *tilt_sensor_attrs[] = {
	&dev_attr_vendor,
	&dev_attr_name,
	NULL
};

static struct device_attribute *sc_sensor_attrs[] = {
	&dev_attr_vendor,
	&dev_attr_name,
	NULL
};

static struct device_attribute *sd_sensor_attrs[] = {
	&dev_attr_vendor,
	&dev_attr_name,
	NULL
};


/* input init */
static int lis2ds_acc_input_init(struct lis2ds_data *cdata)
{
	int ret = 0;
	struct input_dev *dev;

	dev = input_allocate_device();
	if (!dev)
		return -ENOMEM;

	dev->name = MODULE_NAME_ACC;
	dev->id.bustype = BUS_I2C;

	input_set_capability(dev, EV_REL, REL_X);
	input_set_capability(dev, EV_REL, REL_Y);
	input_set_capability(dev, EV_REL, REL_Z);
	input_set_capability(dev, EV_REL, REL_DIAL);
	input_set_capability(dev, EV_REL, REL_MISC);
	input_set_drvdata(dev, cdata);

	ret = input_register_device(dev);
	if (ret < 0) {
		input_free_device(dev);
		goto err_register_input_dev;
	}

	ret = sensors_create_symlink(&dev->dev.kobj, dev->name);
	if (ret < 0)
		goto err_create_sensor_symlink;

	/* sysfs node creation */
	ret = sysfs_create_group(&dev->dev.kobj,
				&lis2ds_accel_attribute_group);
	if (ret < 0)
		goto err_create_sysfs_group;

	cdata->acc_input = dev;

	return 0;

err_create_sysfs_group:
	sensors_remove_symlink(&dev->dev.kobj, dev->name);
err_create_sensor_symlink:
	input_unregister_device(dev);
err_register_input_dev:
	return ret;
}

static int lis2ds_smd_input_init(struct lis2ds_data *cdata)
{
	int ret = 0;
	struct input_dev *dev;

	dev = input_allocate_device();
	if (!dev)
		return -ENOMEM;

	dev->name = MODULE_NAME_SMD;
	dev->id.bustype = BUS_I2C;

	input_set_capability(dev, EV_REL, REL_MISC);
	input_set_drvdata(dev, cdata);

	ret = input_register_device(dev);
	if (ret < 0) {
		input_free_device(dev);
		goto err_register_input_dev;
	}

	ret = sensors_create_symlink(&dev->dev.kobj, dev->name);
	if (ret < 0)
		goto err_create_sensor_symlink;

	/* sysfs node creation */
	ret = sysfs_create_group(&dev->dev.kobj,
				&lis2ds_smd_attribute_group);
	if (ret < 0)
		goto err_create_sysfs_group;

	cdata->smd_input = dev;

	return 0;

err_create_sysfs_group:
	sensors_remove_symlink(&dev->dev.kobj, dev->name);
err_create_sensor_symlink:
	input_unregister_device(dev);
err_register_input_dev:
	return ret;
}

static int lis2ds_tilt_input_init(struct lis2ds_data *cdata)
{
	int ret = 0;
	struct input_dev *dev;

	dev = input_allocate_device();
	if (!dev)
		return -ENOMEM;

	dev->name = MODULE_NAME_TILT;
	dev->id.bustype = BUS_I2C;

	input_set_capability(dev, EV_REL, REL_MISC);
	input_set_drvdata(dev, cdata);

	ret = input_register_device(dev);
	if (ret < 0) {
		input_free_device(dev);
		goto err_register_input_dev;
	}

	ret = sensors_create_symlink(&dev->dev.kobj, dev->name);
	if (ret < 0)
		goto err_create_sensor_symlink;

	/* sysfs node creation */
	ret = sysfs_create_group(&dev->dev.kobj,
				&lis2ds_tilt_attribute_group);
	if (ret < 0)
		goto err_create_sysfs_group;

	cdata->tilt_input = dev;

	return 0;

err_create_sysfs_group:
	sensors_remove_symlink(&dev->dev.kobj, dev->name);
err_create_sensor_symlink:
	input_unregister_device(dev);
err_register_input_dev:
	return ret;
}

static int lis2ds_step_counter_input_init(struct lis2ds_data *cdata)
{
	int ret = 0;
	struct input_dev *dev;

	dev = input_allocate_device();
	if (!dev)
		return -ENOMEM;

	dev->name = MODULE_NAME_SC;
	dev->id.bustype = BUS_I2C;

	input_set_capability(dev, EV_REL, REL_MISC);
	input_set_capability(dev, EV_REL, REL_X);
	input_set_capability(dev, EV_REL, REL_Y);
	input_set_drvdata(dev, cdata);

	ret = input_register_device(dev);
	if (ret < 0) {
		input_free_device(dev);
		goto err_register_input_dev;
	}

	ret = sensors_create_symlink(&dev->dev.kobj, dev->name);
	if (ret < 0)
		goto err_create_sensor_symlink;

	/* sysfs node creation */
	ret = sysfs_create_group(&dev->dev.kobj,
				&lis2ds_sc_attribute_group);
	if (ret < 0)
		goto err_create_sysfs_group;

	cdata->sc_input = dev;

	return 0;

err_create_sysfs_group:
	sensors_remove_symlink(&dev->dev.kobj, dev->name);
err_create_sensor_symlink:
	input_unregister_device(dev);
err_register_input_dev:
	return ret;
}

static int lis2ds_step_detector_input_init(struct lis2ds_data *cdata)
{
	int ret = 0;
	struct input_dev *dev;

	dev = input_allocate_device();
	if (!dev)
		return -ENOMEM;

	dev->name = MODULE_NAME_SD;
	dev->id.bustype = BUS_I2C;

	input_set_capability(dev, EV_REL, REL_MISC);
	input_set_capability(dev, EV_REL, REL_X);
	input_set_capability(dev, EV_REL, REL_Y);
	input_set_drvdata(dev, cdata);

	ret = input_register_device(dev);
	if (ret < 0) {
		input_free_device(dev);
		goto err_register_input_dev;
	}

	ret = sensors_create_symlink(&dev->dev.kobj, dev->name);
	if (ret < 0)
		goto err_create_sensor_symlink;

	/* sysfs node creation */
	ret = sysfs_create_group(&dev->dev.kobj,
				&lis2ds_sd_attribute_group);
	if (ret < 0)
		goto err_create_sysfs_group;

	cdata->sd_input = dev;

	return 0;

err_create_sysfs_group:
	sensors_remove_symlink(&dev->dev.kobj, dev->name);
err_create_sensor_symlink:
	input_unregister_device(dev);
err_register_input_dev:
	return ret;
}

static u32 lis2ds_parse_dt(struct lis2ds_data *cdata)
{
	struct device_node *np;
	enum of_gpio_flags flags;
	u32 orientation[9], i = 0;

	np = cdata->dev->of_node;
	if (!np)
		return -EINVAL;

	cdata->irq_gpio = of_get_named_gpio_flags(np, "st,irq_gpio", 0, &flags);
	if (cdata->irq_gpio < 0) {
		SENSOR_ERR("get irq_gpio = %d error\n", cdata->irq_gpio);
		return -ENODEV;
	}
	cdata->irq = gpio_to_irq(cdata->irq_gpio);

	if (of_property_read_u32_array(np, "st,orientation",
						orientation, 9) < 0) {
		SENSOR_ERR("get orientation %d error\n", orientation[0]);
		return -ENODEV;
	}

	for (i = 0 ; i < 9 ; i++)
		cdata->orientation[i] = ((s8)orientation[i]) - 1;

	return 0;
}

int lis2ds_common_probe(struct lis2ds_data *cdata, int irq, u16 bustype)
{
	int32_t err, i;
	u8 wai = 0;
	int retry = 5;

	SENSOR_INFO("Start!\n");

	mutex_init(&cdata->bank_registers_lock);
	mutex_init(&cdata->tb.buf_lock);
	mutex_init(&cdata->mutex_enable);
	mutex_init(&cdata->mutex_read);

	/* Read Chip ID register */
	while (retry--) {
		err = cdata->tf->read(cdata, LIS2DS_WHO_AM_I_ADDR, 1, &wai, true);
		if (err < 0)
			SENSOR_ERR("failed to read Who-Am-I register.\n");

		if (wai != LIS2DS_WHO_AM_I_DEF)
			SENSOR_ERR("Who-Am-I value = 0x%x not valid.\n", wai);
		else
			break;
		
		msleep(20); //20msec delay
	}

	if (retry < 0)
		goto exit_err_chip_id_or_i2c_error;

	if (irq > 0) {
		err = lis2ds_parse_dt(cdata);
		if (err < 0)
			goto parse_dt_error;
	}

	/* input device init */
	err = lis2ds_acc_input_init(cdata);
	if (err < 0)
		goto exit_acc_input_init;

	err = lis2ds_smd_input_init(cdata);
	if (err < 0)
		goto exit_smd_input_init;

	err = lis2ds_tilt_input_init(cdata);
	if (err < 0)
		goto exit_tilt_input_init;

	err = lis2ds_step_counter_input_init(cdata);
	if (err < 0)
		goto exit_sc_input_init;

	err = lis2ds_step_detector_input_init(cdata);
	if (err < 0)
		goto exit_sd_input_init;

	err = lis2ds_init_sensors(cdata, i);
	if (err < 0)
		goto exit_init_sensor;

	/* factory test sysfs node */
	err = sensors_register(&cdata->acc_factory_dev, cdata,
		acc_sensor_attrs, MODULE_NAME_ACC);
	if (err < 0) {
		SENSOR_ERR("failed to sensors_register = %d\n", err);
		goto exit_acc_sensor_register_failed;
	}

	err = sensors_register(&cdata->smd_factory_dev, cdata,
		smd_sensor_attrs, MODULE_NAME_SMD);
	if (err) {
		SENSOR_ERR("failed to sensors_register = %d\n", err);
		goto exit_smd_sensor_register_failed;
	}

	err = sensors_register(&cdata->tilt_factory_dev, cdata,
		tilt_sensor_attrs, MODULE_NAME_TILT);
	if (err) {
		SENSOR_ERR("failed to sensors_register = %d\n", err);
		goto exit_tilt_sensor_register_failed;
	}

	err = sensors_register(&cdata->sc_factory_dev, cdata,
		sc_sensor_attrs, MODULE_NAME_SC);
	if (err) {
		SENSOR_ERR("failed to sensors_register = %d\n", err);
		goto exit_sc_sensor_register_failed;
	}

	err = sensors_register(&cdata->sd_factory_dev, cdata,
		sd_sensor_attrs, MODULE_NAME_SD);
	if (err) {
		SENSOR_ERR("failed to sensors_register = %d\n", err);
		goto exit_sd_sensor_register_failed;
	}

	/* workqueue init */
	hrtimer_init(&cdata->acc_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	cdata->acc_delay = ns_to_ktime(LIS2DS_DELAY_DEFAULT);
	cdata->acc_timer.function = lis2ds_acc_timer_func;

	/* the timer just fires off a work queue request.  we need a thread
	   to read the i2c (can be slow and blocking). */
	cdata->accel_wq = create_singlethread_workqueue("accel_wq");
	if (!cdata->accel_wq) {
		err = -ENOMEM;
		SENSOR_ERR("could not create accel workqueue\n");
		goto exit_create_workqueue_acc;
	}

	/* this is the thread function we run on the work queue */
	INIT_WORK(&cdata->acc_work, lis2ds_acc_work_func);

	atomic_set(&cdata->acc_wkqueue_en, 0);

	cdata->irq_wq = create_workqueue(cdata->name);
	if (!cdata->irq_wq) {
		err = -ENOMEM;
		SENSOR_ERR("could not create irq workqueue\n");
		goto exit_create_workqueue_irq;
	}

	if (cdata->irq > 0) {
		wake_lock_init(&cdata->sa_wake_lock, WAKE_LOCK_SUSPEND,
		       LIS2DS_DEV_NAME "_sa_wake_lock");

		INIT_WORK(&cdata->data_work, lis2ds_event_management);
		INIT_DELAYED_WORK(&cdata->sa_irq_work, lis2ds_sa_irq_work);

		err = request_threaded_irq(irq, lis2ds_thread_fn, NULL,
				IRQF_TRIGGER_RISING, cdata->name, cdata);
		disable_irq(cdata->irq);

		SENSOR_INFO("Smart alert init, irq = %d\n", cdata->irq);
	}

	SENSOR_INFO(" probed\n");
	return 0;

exit_create_workqueue_irq:
	destroy_workqueue(cdata->accel_wq);
exit_create_workqueue_acc:
	sensors_unregister(cdata->sd_factory_dev, sd_sensor_attrs);
exit_sd_sensor_register_failed:
	sensors_unregister(cdata->sc_factory_dev, sc_sensor_attrs);
exit_sc_sensor_register_failed:
	sensors_unregister(cdata->tilt_factory_dev, tilt_sensor_attrs);
exit_tilt_sensor_register_failed:
	sensors_unregister(cdata->smd_factory_dev, smd_sensor_attrs);
exit_smd_sensor_register_failed:
	sensors_unregister(cdata->acc_factory_dev, acc_sensor_attrs);
exit_acc_sensor_register_failed:
exit_init_sensor:
	sensors_remove_symlink(&cdata->sd_input->dev.kobj,
					cdata->sd_input->name);
	sysfs_remove_group(&cdata->sd_input->dev.kobj,
					&lis2ds_sd_attribute_group);
	input_unregister_device(cdata->sd_input);
exit_sd_input_init:
	sensors_remove_symlink(&cdata->sc_input->dev.kobj,
					cdata->sc_input->name);
	sysfs_remove_group(&cdata->sc_input->dev.kobj,
					&lis2ds_sc_attribute_group);
	input_unregister_device(cdata->sc_input);
exit_sc_input_init:
	sensors_remove_symlink(&cdata->tilt_input->dev.kobj,
					cdata->tilt_input->name);
	sysfs_remove_group(&cdata->tilt_input->dev.kobj,
					&lis2ds_tilt_attribute_group);
	input_unregister_device(cdata->tilt_input);
exit_tilt_input_init:
	sensors_remove_symlink(&cdata->smd_input->dev.kobj,
					cdata->smd_input->name);
	sysfs_remove_group(&cdata->smd_input->dev.kobj,
					&lis2ds_smd_attribute_group);
	input_unregister_device(cdata->smd_input);
exit_smd_input_init:
	sensors_remove_symlink(&cdata->acc_input->dev.kobj,
					cdata->acc_input->name);
	sysfs_remove_group(&cdata->acc_input->dev.kobj,
					&lis2ds_accel_attribute_group);
	input_unregister_device(cdata->acc_input);
exit_acc_input_init:
exit_err_chip_id_or_i2c_error:
parse_dt_error:
	mutex_destroy(&cdata->bank_registers_lock);
	mutex_destroy(&cdata->tb.buf_lock);
	mutex_destroy(&cdata->mutex_enable);
	mutex_destroy(&cdata->mutex_read);

	return err;
}
EXPORT_SYMBOL(lis2ds_common_probe);

void lis2ds_common_remove(struct lis2ds_data *cdata, int irq)
{
	u8 i;

	for (i = 0; i < LIS2DS_SENSORS_NUMB; i++)
		lis2ds_disable_sensors(cdata, i);

}
EXPORT_SYMBOL(lis2ds_common_remove);

void lis2ds_common_shutdown(struct lis2ds_data *cdata)
{
	u8 i;

	for (i = 0; i < LIS2DS_SENSORS_NUMB; i++)
		lis2ds_disable_sensors(cdata, i);
}
EXPORT_SYMBOL(lis2ds_common_shutdown);

static int lis2ds_resume_sensors(struct lis2ds_data *cdata)
{
	int err;
	u16 step_cnt = 0;
	struct timespec ts = ktime_to_timespec(ktime_get_boottime());
	u64 timestamp_new = ts.tv_sec * 1000000000ULL + ts.tv_nsec;
	int time_hi, time_lo;

	time_hi = (int)((timestamp_new & TIME_HI_MASK) >> TIME_HI_SHIFT);
	time_lo = (int)(timestamp_new & TIME_LO_MASK);

	if (cdata->sensors[LIS2DS_STEP_C].enabled
		|| cdata->sensors[LIS2DS_STEP_D].enabled) {

		if (cdata->sensors[LIS2DS_STEP_C].enabled) {
			mutex_lock(&cdata->mutex_read);
			lis2ds_report_step_c_data(cdata, &step_cnt);
			mutex_unlock(&cdata->mutex_read);
			cdata->last_steps_c = cdata->steps_c + step_cnt;
			input_report_rel(cdata->sc_input, REL_MISC, cdata->last_steps_c+1);
			input_report_rel(cdata->sc_input, REL_X, time_hi);
			input_report_rel(cdata->sc_input, REL_Y, time_lo);
			input_sync(cdata->sc_input);
		}

		lis2ds_write_data_with_mask(cdata,
			  LIS2DS_CTRL5_INT2_PAD_ADDR,
			  LIS2DS_INT2_STEP_DET_MASK,
			  0x01,
			  true);
	}

	if (atomic_read(&cdata->acc_wkqueue_en) == 1) {
		err = lis2ds_enable_sensors(cdata, LIS2DS_ACCEL);
		if (err < 0)
			return err;
	}

	return 0;
}

static int lis2ds_suspend_sensors(struct lis2ds_data *cdata)
{
	int err;

	if (atomic_read(&cdata->acc_wkqueue_en) == 1) {
		err = lis2ds_disable_sensors(cdata, LIS2DS_ACCEL);
		if (err < 0)
			return err;
	}

	if (cdata->sensors[LIS2DS_STEP_C].enabled
		|| cdata->sensors[LIS2DS_STEP_D].enabled) {

		lis2ds_write_data_with_mask(cdata,
			  LIS2DS_CTRL5_INT2_PAD_ADDR,
			  LIS2DS_INT2_STEP_DET_MASK,
			  0x00,
			  true);

		if (cdata->sensors[LIS2DS_STEP_C].enabled) {
			cdata->steps_c = cdata->last_steps_c;
			lis2ds_reset_step_counter(cdata);
		}
	}

	return 0;
}

int lis2ds_common_suspend(struct lis2ds_data *cdata)
{
	int err;
	err = lis2ds_suspend_sensors(cdata);
	if (err < 0) {
		SENSOR_ERR(": suspend failed\n");
		return err;
	}
	return 0;
}
EXPORT_SYMBOL(lis2ds_common_suspend);

int lis2ds_common_resume(struct lis2ds_data *cdata)
{
	int err;
	err = lis2ds_resume_sensors(cdata);
	if (err < 0) {
		SENSOR_ERR(": resume failed\n");
		return err;
	}

	return 0;
}
EXPORT_SYMBOL(lis2ds_common_resume);

MODULE_DESCRIPTION("STMicroelectronics lis2ds driver");
MODULE_AUTHOR("Giuseppe Barba");
MODULE_AUTHOR("Mario Tesi");
MODULE_LICENSE("GPL v2");
