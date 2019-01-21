/*
 * STMicroelectronics lsm6dsl driver
 *
 * Copyright 2016 STMicroelectronics Inc.
 *
 * Giuseppe Barba <giuseppe.barba@st.com>
 * Mario Tesi <mario.tesi@st.com>
 * v 1.0.1
 * Licensed under the GPL-2.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/mutex.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/irq.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <asm/unaligned.h>
#include <linux/hrtimer.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/regulator/consumer.h>
#include <linux/of_gpio.h>
#include "lsm6dsl_core.h"

#ifdef TAG
#undef TAG
#define TAG "[ACCEL]"
#endif

/* COMMON DEFINE FOR ACCEL-GYRO SENSORS */
#define LSM6DSL_EN_BIT			0x01
#define LSM6DSL_DIS_BIT			0x00

#define LSM6DSL_WHO_AM_I		0x0f
#define LSM6DSL_WHO_AM_I_DEF		0x6a

#define LSM6DSL_INT1_ADDR		0x0d
#define LSM6DSL_INT1_STEP_DET_MASK		0x80
#define LSM6DSL_INT1_FTH_MASK		0x08

#define LSM6DSL_INT2_ADDR		0x0e
#define LSM6DSL_INT2_SC_OV_MASK		0x60

#define LSM6DSL_MD1_ADDR		0x5e
#define LSM6DSL_INT1_WU_MASK		0x20

#define LSM6DSL_SRC_ACCEL_GYRO_REG		0x1e

#define LSM6DSL_WU_SRC_ADDR			0x1b
#define LSM6DSL_WU_THS_ADDR			0x5b
#define LSM6DSL_WU_THS_MASK			0x3f
#define LSM6DSL_WU_DUR_ADDR		0x5c
#define LSM6DSL_WU_DUR_MASK			0x60

#define LSM6DSL_ODR_LIST_NUM		5
#define LSM6DSL_ODR_POWER_OFF_VAL	0x00
#define LSM6DSL_ODR_13HZ_VAL		0x01
#define LSM6DSL_ODR_26HZ_VAL		0x02
#define LSM6DSL_ODR_52HZ_VAL		0x03
#define LSM6DSL_ODR_104HZ_VAL		0x04
#define LSM6DSL_ODR_208HZ_VAL		0x05
#define LSM6DSL_ODR_416HZ_VAL		0x06
#define LSM6DSL_FS_LIST_NUM		4

#define LSM6DSL_BDU_ADDR		0x12
#define LSM6DSL_BDU_MASK		0x40

#define LSM6DSL_FUNC_EN_ADDR		0x19
#define LSM6DSL_FUNC_EN_MASK		0x04
#define LSM6DSL_FUNC_CFG_ACCESS_ADDR	0x01
#define LSM6DSL_FUNC_CFG_REG_MASK	0x80

#define LSM6DSL_BASIC_INT_EN_ADDR		0x58
#define LSM6DSL_BASIC_INT_EN_MASK		0x80

#define LSM6DSL_LIR_ADDR		0x58
#define LSM6DSL_LIR_MASK		0x01

#define LSM6DSL_TIMER_EN_ADDR		0x19
#define LSM6DSL_TIMER_EN_MASK		0x20

#define LSM6DSL_PEDOMETER_EN_ADDR	0x19
#define LSM6DSL_PEDOMETER_EN_MASK	0x10

#define LSM6DSL_TIMER_PEDO_FIFO_EN_MASK	0x80
#define LSM6DSL_TIMER_PEDO_FIFO_DRDY_MASK	0x40
#define LSM6DSL_STOP_ON_FTH_MASK	0x80
#define LSM6DSL_DEC_DS3_FIFO_MASK	0x38
#define LSM6DSL_ODR_FIFO_MASK	0x78
#define LSM6DSL_FIFO_MODE_MASK	0x07

#define LSM6DSL_PEDOMETER_DEC_ADDR  0x09
#define LSM6DSL_PEDOMETER_DEC_MASK  0x38

#define LSM6DSL_FIFO_PEDO_E_ADDR	0x07
#define LSM6DSL_FIFO_PEDO_E_MASK	0x80

#define LSM6DSL_FIFO_PEDO_DRDY_ADDR	0x07
#define LSM6DSL_FIFO_PEDO_DRDY_MASK	0x40

#define LSM6DSL_FIFO_PEDO_MODE_ADDR	0x0a
#define LSM6DSL_FIFO_PEDO_MODE_MASK	0x07

#define LSM6DSL_TIMESTAMP2_ADDR	0x42
#define LSM6DSL_TIMESTAMP_RESET_VAL	0xaa

#define LSM6DSL_INT2_ON_INT1_ADDR	0x13
#define LSM6DSL_INT2_ON_INT1_MASK	0x20

#define LSM6DSL_MIN_DURATION_MS		1638
#define LSM6DSL_ROUNDING_ADDR		0x16
#define LSM6DSL_ROUNDING_MASK		0x04

/* CUSTOM VALUES FOR ACCEL SENSOR */
#define LSM6DSL_ACCEL_ODR_ADDR		0x10
#define LSM6DSL_ACCEL_ODR_MASK		0xf0
#define LSM6DSL_ACCEL_ODR_POWER_DOWN			0x00
#define LSM6DSL_ACCEL_ODR_13HZ_VAL			0x01
#define LSM6DSL_ACCEL_ODR_26HZ_VAL			0x02
#define LSM6DSL_ACCEL_ODR_52HZ_VAL			0x03
#define LSM6DSL_ACCEL_ODR_104HZ_VAL		0x04
#define LSM6DSL_ACCEL_ODR_208HZ_VAL		0x05
#define LSM6DSL_ACCEL_ODR_416HZ_VAL		0x06
#define LSM6DSL_ACCEL_ODR_833HZ_VAL		0x07
#define LSM6DSL_ACCEL_ODR_1660HZ_VAL		0x08
#define LSM6DSL_ACCEL_ODR_3330HZ_VAL		0x09
#define LSM6DSL_ACCEL_ODR_6660HZ_VAL		0x0a
#define LSM6DSL_ACCEL_FS_ADDR		0x10
#define LSM6DSL_ACCEL_FS_MASK		0x0c
#define LSM6DSL_ACCEL_FS_2G_VAL		0x00
#define LSM6DSL_ACCEL_FS_4G_VAL		0x02
#define LSM6DSL_ACCEL_FS_8G_VAL		0x03
#define LSM6DSL_ACCEL_FS_16G_VAL	0x01
#define LSM6DSL_ACCEL_FS_2G_GAIN	61
#define LSM6DSL_ACCEL_FS_4G_GAIN	122
#define LSM6DSL_ACCEL_FS_8G_GAIN	244
#define LSM6DSL_ACCEL_FS_16G_GAIN	488
#define LSM6DSL_ACCEL_BW_ADDR		0x10
#define LSM6DSL_ACCEL_BW_MASK		0x03
#define LSM6DSL_ACCEL_BW_50HZ_VAL		0x03
#define LSM6DSL_ACCEL_BW_100HZ_VAL		0x02
#define LSM6DSL_ACCEL_BW_200HZ_VAL		0x01
#define LSM6DSL_ACCEL_BW_400HZ_VAL		0x00
#define LSM6DSL_ACCEL_OUT_X_L_ADDR	0x28
#define LSM6DSL_ACCEL_OUT_Y_L_ADDR	0x2a
#define LSM6DSL_ACCEL_OUT_Z_L_ADDR	0x2c
#define LSM6DSL_ACCEL_AXIS_EN_ADDR	0x18
#define LSM6DSL_ACCEL_DRDY_IRQ_MASK	0x01
#define LSM6DSL_ACCEL_STD		1
#define LSM6DSL_ACCEL_STD_FROM_PD	2

/* CUSTOM VALUES FOR GYRO SENSOR */
#define LSM6DSL_GYRO_ODR_ADDR		0x11
#define LSM6DSL_GYRO_ODR_MASK		0xf0
#define LSM6DSL_GYRO_FS_ADDR		0x11
#define LSM6DSL_GYRO_FS_MASK		0x0c
#define LSM6DSL_GYRO_FS_245_VAL		0x00
#define LSM6DSL_GYRO_FS_500_VAL		0x01
#define LSM6DSL_GYRO_FS_1000_VAL		0x02
#define LSM6DSL_GYRO_FS_2000_VAL		0x03
#define LSM6DSL_GYRO_FS_245_GAIN		8750
#define LSM6DSL_GYRO_FS_500_GAIN		17500
#define LSM6DSL_GYRO_FS_1000_GAIN	35000
#define LSM6DSL_GYRO_FS_2000_GAIN	70000
#define LSM6DSL_GYRO_OUT_X_L_ADDR	0x22
#define LSM6DSL_GYRO_OUT_Y_L_ADDR	0x24
#define LSM6DSL_GYRO_OUT_Z_L_ADDR	0x26
#define LSM6DSL_GYRO_AXIS_EN_ADDR	0x19
#define LSM6DSL_GYRO_DRDY_IRQ_MASK	0x02
#define LSM6DSL_GYRO_STD		6
#define LSM6DSL_GYRO_STD_FROM_PD	2

#define LSM6DSL_OUT_XYZ_SIZE		6

/* CUSTOM VALUES FOR SIGNIFICANT MOTION SENSOR */
#define LSM6DSL_SIGN_MOTION_EN_ADDR	0x19
#define LSM6DSL_SIGN_MOTION_EN_MASK	0x01
#define LSM6DSL_SIGN_MOTION_DRDY_IRQ_MASK	0x40

/* CUSTOM VALUES FOR STEP DETECTOR SENSOR */
#define LSM6DSL_STEP_DETECTOR_DRDY_IRQ_MASK	0x80

/* CUSTOM VALUES FOR STEP COUNTER SENSOR */
#define LSM6DSL_STEP_COUNTER_OUT_L_ADDR		0x4b
#define LSM6DSL_STEP_COUNTER_OUT_SIZE	2
#define LSM6DSL_STEP_COUNTER_RES_ADDR	0x19
#define LSM6DSL_STEP_COUNTER_RES_MASK	0x06
#define LSM6DSL_STEP_COUNTER_RES_ALL_EN		0x03
#define LSM6DSL_STEP_COUNTER_RES_FUNC_EN	0x02
#define LSM6DSL_STEP_COUNTER_THS_MIN_ADDR	0x0f
#define LSM6DSL_STEP_COUNTER_DEB_ADDR	0x14
#define LSM6DSL_STEP_COUNTER_DURATION_ADDR	0x15

/* CUSTOM VALUES FOR STEP COUNTER SENSOR FROM FIFO */
#define LSM6DSL_FIFO_STATUS1_ADDR 0x3a
#define LSM6DSL_FIFO_STATUS2_ADDR 0x3b
#define LSM6DSL_FIFO_DATA_ADDR 0x3e

/* CUSTOM VALUES FOR TILT SENSOR */
#define LSM6DSL_TILT_EN_ADDR		0x19
#define LSM6DSL_TILT_EN_MASK		0x08
#define LSM6DSL_TILT_DRDY_IRQ_MASK	0x02

#define LSM6DSL_SRC_FUNC_ADDR		0x53
#define LSM6DSL_SRC2_FUNC_ADDR		0x54
#define LSM6DSL_SRC_SIGN_MOTION_DATA_VAL	0x40
#define LSM6DSL_SRC_TILT_DATA_VAL	0x20
#define LSM6DSL_SRC_STEP_DETECTOR_DATA_VAL	0x10
#define LSM6DSL_SRC_STEP_COUNTER_DATA_VAL	0x80
#define LSM6DSL_SRC_STEP_OVERFLOW_VAL	0x08
#define LSM6DSL_SRC_WU_VAL			0x0f

#define LSM6DSL_SRC_WAKE_UP_REG		0x1b
#define LSM6DSL_OUT_TEMP_L_ADDR	0x20

/* Sensor Software Reset Bit */
#define LSM6DSL_RESET_ADDR		0x12
#define LSM6DSL_RESET_MASK		0x01

#ifndef ABS
#define ABS(a)		((a) > 0 ? (a) : -(a))
#endif

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

struct lsm6dsl_odr_reg {
	u32 hz;
	u8 value;
};

static const struct lsm6dsl_odr_table {
	u8 addr[2];
	u8 mask[2];
	struct lsm6dsl_odr_reg odr_avl[6];
} lsm6dsl_odr_table = {
	.addr[LSM6DSL_ACCEL] = LSM6DSL_ACC_ODR_ADDR,
	.mask[LSM6DSL_ACCEL] = LSM6DSL_ACC_ODR_MASK,
	.addr[LSM6DSL_GYRO] = LSM6DSL_GYR_ODR_ADDR,
	.mask[LSM6DSL_GYRO] = LSM6DSL_GYR_ODR_MASK,
	.odr_avl[0] = { .hz = 13, .value = LSM6DSL_ODR_13HZ_VAL },
	.odr_avl[1] = { .hz = 26, .value = LSM6DSL_ODR_26HZ_VAL },
	.odr_avl[2] = { .hz = 52, .value = LSM6DSL_ODR_52HZ_VAL },
	.odr_avl[3] = { .hz = 104, .value = LSM6DSL_ODR_104HZ_VAL },
	.odr_avl[4] = { .hz = 208, .value = LSM6DSL_ODR_208HZ_VAL },
	.odr_avl[5] = { .hz = 416, .value = LSM6DSL_ODR_416HZ_VAL },
};

struct lsm6dsl_fs_reg {
	unsigned int gain;
	u8 value;
	int urv;
};

static struct lsm6dsl_fs_table {
	u8 addr;
	u8 mask;
	struct lsm6dsl_fs_reg fs_avl[LSM6DSL_FS_LIST_NUM];
} lsm6dsl_fs_table[LSM6DSL_SENSORS_NUMB] = {
	[LSM6DSL_ACCEL] = {
		.addr = LSM6DSL_ACCEL_FS_ADDR,
		.mask = LSM6DSL_ACCEL_FS_MASK,
		.fs_avl[0] = { .gain = LSM6DSL_ACCEL_FS_2G_GAIN,
			       .value = LSM6DSL_ACCEL_FS_2G_VAL,
			       .urv = 2, },
		.fs_avl[1] = { .gain = LSM6DSL_ACCEL_FS_4G_GAIN,
			       .value = LSM6DSL_ACCEL_FS_4G_VAL,
			       .urv = 4, },
		.fs_avl[2] = { .gain = LSM6DSL_ACCEL_FS_8G_GAIN,
			       .value = LSM6DSL_ACCEL_FS_8G_VAL,
			       .urv = 8, },
		.fs_avl[3] = { .gain = LSM6DSL_ACCEL_FS_16G_GAIN,
			       .value = LSM6DSL_ACCEL_FS_16G_VAL,
			       .urv = 16, },
	},
	[LSM6DSL_GYRO] = {
		.addr = LSM6DSL_GYRO_FS_ADDR,
		.mask = LSM6DSL_GYRO_FS_MASK,
		.fs_avl[0] = { .gain = LSM6DSL_GYRO_FS_245_GAIN,
			       .value = LSM6DSL_GYRO_FS_245_VAL,
			       .urv = 245, },
		.fs_avl[1] = { .gain = LSM6DSL_GYRO_FS_500_GAIN,
			       .value = LSM6DSL_GYRO_FS_500_VAL,
			       .urv = 500, },
		.fs_avl[2] = { .gain = LSM6DSL_GYRO_FS_1000_GAIN,
			       .value = LSM6DSL_GYRO_FS_1000_VAL,
			        .urv = 1000, },
		.fs_avl[3] = { .gain = LSM6DSL_GYRO_FS_2000_GAIN,
			       .value = LSM6DSL_GYRO_FS_2000_VAL,
			       .urv = 2000, },
	}
};

static int lsm6dsl_write_data_with_mask(struct lsm6dsl_data *cdata,
					u8 reg_addr, u8 mask, u8 data, bool b_lock)
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

static int lsm6dsl_reset_steps(struct lsm6dsl_data *cdata);
static int lsm6dsl_reset_timestamp(struct lsm6dsl_data *cdata);
static int lsm6dsl_enable_sensors(struct lsm6dsl_data *cdata, int sindex);
static int lsm6dsl_disable_sensors(struct lsm6dsl_data *cdata, int sindex);
int lsm6dsl_acc_open_calibration(struct lsm6dsl_data *cdata);


static int lsm6dsl_set_fs(struct lsm6dsl_data *cdata, int sindex, u8 fs)
{
	int err = 0;

	if (sindex == LSM6DSL_ACCEL)
		cdata->acc_fs = fs;
	else if (sindex == LSM6DSL_GYRO)
		cdata->gyro_fs = fs;
	else
		return err;

	err = lsm6dsl_write_data_with_mask(cdata,
			lsm6dsl_fs_table[sindex].addr,
			lsm6dsl_fs_table[sindex].mask,
			fs, true);
	if (err < 0)
		return err;

	return err;
}

static void lsm6dsl_get_step_c_data(struct lsm6dsl_data *cdata, u16 *steps)
{
	u8 data[2];
	int err = 0;
	err = cdata->tf->read(cdata,
				     LSM6DSL_STEP_COUNTER_OUT_L_ADDR,
				     LSM6DSL_STEP_COUNTER_OUT_SIZE,
				     data, true);
	if (err < 0)
		SENSOR_ERR("sm6dsl_i2c_read err: %d\n", err);
	else
		*steps = data[0] | (data[1] << 8);

}

static int lsm6dsl_get_step_c_data_fifo(struct lsm6dsl_data *cdata)
{
	u8 data[2];
	u8 *fifo_data;
	u16 fifo_cnt;
	u16 step_cnt = 0;
	u16 step_frame = 0;
	u64 timestamp = 0;
	int err = 0;
	int i = 0;
	int time_hi = 0, time_lo = 0;
	struct timespec ts;
	u64 timestamp_new = 0;

	err = cdata->tf->read(cdata, LSM6DSL_FIFO_STATUS1_ADDR, 2, data, true);
	if (err < 0)
		return err;

	fifo_cnt = data[0] | ((data[1] << 8) & 0x0700);
	step_frame = fifo_cnt / 3;

	SENSOR_INFO("step_frame : %d\n", step_frame);

	if (step_frame > 0) {
		fifo_data = (u8 *)kzalloc(6 * step_frame * sizeof(u8), GFP_KERNEL);
		err = cdata->tf->read(cdata,
				     LSM6DSL_FIFO_DATA_ADDR,
				     6 * step_frame,
				     fifo_data, true);
	} else
		return 0;

/*
 * Byte 1(fifo_data[0]) : TIMESTAMP [15:8]
 * Byte 2(fifo_data[1]) : TIMESTAMP [23:16]
 * Byte 3(fifo_data[2]) : not used
 * Byte 4(fifo_data[3]) : TIMESTAMP [7:0]
 * Byte 5(fifo_data[4]) : STEPS [7:0]
 * Byte 6(fifo_data[5]) : STEPS [15:8]
 * timestamp : timestamp * 6.4 ms	(1LSB = 6.4ms)
 */

	for (i = 0; i < step_frame; i++) {
		step_cnt = fifo_data[(i * 6) + 4]
			| (fifo_data[(i * 6) + 5] << 8);
		timestamp = fifo_data[(i * 6) + 3]
			| (fifo_data[(i * 6) + 0] << 8)
			| (fifo_data[(i * 6) + 1] << 16);
		ts = ns_to_timespec(cdata->step_timestamp
					+ (timestamp * TIMESTAMP_TO_NS));
		timestamp_new = ts.tv_sec * 1000000000ULL + ts.tv_nsec;
		if (cdata->enabled & (1 << LSM6DSL_STEP_DETECTOR)) {
			time_hi = (int)((timestamp_new & TIME_HI_MASK)
						>> TIME_HI_SHIFT);
			time_lo = (int)(timestamp_new & TIME_LO_MASK);
			input_report_rel(cdata->sd_input, REL_MISC, 1);
			input_report_rel(cdata->sd_input, REL_X, time_hi);
			input_report_rel(cdata->sd_input, REL_Y, time_lo);
			input_sync(cdata->sd_input);
		}
	}

	if (cdata->enabled & (1 << LSM6DSL_STEP_COUNTER)) {
		cdata->last_steps_c = cdata->steps_c + step_cnt;
		time_hi = (int)((timestamp_new & TIME_HI_MASK)
				> TIME_HI_SHIFT);
		time_lo = (int)(timestamp_new & TIME_LO_MASK);
		input_report_rel(cdata->sc_input, REL_MISC,
				cdata->last_steps_c + 1);
		input_report_rel(cdata->sc_input, REL_X, time_hi);
		input_report_rel(cdata->sc_input, REL_Y, time_lo);
		input_sync(cdata->sc_input);
	}

	kfree(fifo_data);

	return 0;
}

void lsm6dsl_set_irq(struct lsm6dsl_data *cdata, bool enable)
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

irqreturn_t lsm6dsl_threaded(int irq, void *private)
{
	struct lsm6dsl_data *cdata = private;

	queue_work(cdata->irq_wq, &cdata->data_work);

	return IRQ_HANDLED;
}

static void lsm6dsl_sa_irq_work(struct work_struct *work)
{
	struct lsm6dsl_data *cdata;

	cdata = container_of((struct delayed_work *)work,
				struct lsm6dsl_data, sa_irq_work);

	lsm6dsl_write_data_with_mask(cdata,
				   LSM6DSL_MD1_ADDR,
				   LSM6DSL_INT1_WU_MASK,
				   LSM6DSL_DIS_BIT, true);
}

static void lsm6dsl_irq_management(struct work_struct *data_work)
{
	struct lsm6dsl_data *cdata;
	u8 src_value = 0x00, src_wake_up = 0x00, fifo_status2 = 0x00;
	u16 step_cnt = 0;
	struct timespec ts = ktime_to_timespec(ktime_get_boottime());
	u64 timestamp_new = ts.tv_sec * 1000000000ULL + ts.tv_nsec;
	int time_hi, time_lo;

	cdata = container_of((struct work_struct *)data_work,
			     struct lsm6dsl_data, data_work);

	cdata->tf->read(cdata, LSM6DSL_SRC_FUNC_ADDR, 1, &src_value, true);
	cdata->tf->read(cdata, LSM6DSL_SRC_WAKE_UP_REG, 1, &src_wake_up, true);
	cdata->tf->read(cdata, LSM6DSL_FIFO_STATUS2_ADDR, 1,
					&fifo_status2, true);

	SENSOR_INFO("src_value : 0x%x, src_wake_up : 0x%x, fifo_status2 : 0x%x\n",
					src_value, src_wake_up, fifo_status2);

	time_hi = (int)((timestamp_new & TIME_HI_MASK) >> TIME_HI_SHIFT);
	time_lo = (int)(timestamp_new & TIME_LO_MASK);

	if ((src_wake_up & LSM6DSL_SRC_WU_VAL) || (cdata->sa_factory_flag)) {
		cdata->sa_irq_state = 1;
		wake_lock_timeout(&cdata->sa_wake_lock, msecs_to_jiffies(3000));
		schedule_delayed_work(&cdata->sa_irq_work,
					msecs_to_jiffies(100));
	}

	if ((cdata->enabled & (1 << LSM6DSL_STEP_COUNTER))
			&& (src_value & LSM6DSL_SRC_STEP_COUNTER_DATA_VAL)) {
		mutex_lock(&cdata->mutex_read);
		lsm6dsl_get_step_c_data(cdata, &step_cnt);
		mutex_unlock(&cdata->mutex_read);
		cdata->last_steps_c = cdata->steps_c + step_cnt;
		input_report_rel(cdata->sc_input, REL_MISC,
				cdata->last_steps_c + 1);
		input_report_rel(cdata->sc_input, REL_X, time_hi);
		input_report_rel(cdata->sc_input, REL_Y, time_lo);
		input_sync(cdata->sc_input);
	}

	if ((cdata->enabled & (1 << LSM6DSL_STEP_DETECTOR))
			&& (src_value & LSM6DSL_SRC_STEP_DETECTOR_DATA_VAL)) {
		input_report_rel(cdata->sd_input, REL_MISC, 1);
		input_report_rel(cdata->sd_input, REL_X, time_hi);
		input_report_rel(cdata->sd_input, REL_Y, time_lo);
		input_sync(cdata->sd_input);
	}

	if (src_value & LSM6DSL_SRC_SIGN_MOTION_DATA_VAL) {
		if (cdata->sign_motion_event_ready) {
			input_report_rel(cdata->smd_input, REL_MISC, 1);
			input_sync(cdata->smd_input);
			cdata->sign_motion_event_ready = false;
			lsm6dsl_disable_sensors(cdata, LSM6DSL_SIGN_MOTION);
			wake_lock_timeout(&cdata->sa_wake_lock,
					msecs_to_jiffies(3000));
		}
	}

	if (src_value & LSM6DSL_SRC_TILT_DATA_VAL) {
		input_report_rel(cdata->tilt_input, REL_MISC, 1);
		input_sync(cdata->tilt_input);
		wake_lock_timeout(&cdata->sa_wake_lock, msecs_to_jiffies(3000));
	}

	if (src_value & LSM6DSL_SRC_STEP_OVERFLOW_VAL) {
		mutex_lock(&cdata->mutex_read);
		cdata->steps_c = cdata->last_steps_c;
		lsm6dsl_reset_steps(cdata);
		mutex_unlock(&cdata->mutex_read);
	}

}

static int lsm6dsl_get_poll_data(struct lsm6dsl_data *cdata, int sindex,
					u8 *data)
{
	int err = 0;
	u8 reg_addr;

	switch (sindex) {
	case LSM6DSL_ACCEL:
		reg_addr = LSM6DSL_ACCEL_OUT_X_L_ADDR;

		break;
	case LSM6DSL_GYRO:
		reg_addr = LSM6DSL_GYRO_OUT_X_L_ADDR;

		break;
	default:
		return -EINVAL;
	}

	err = cdata->tf->read(cdata, reg_addr, LSM6DSL_OUT_XYZ_SIZE,
				     data, true);

	return err;
}

/* work func */
static enum hrtimer_restart lsm6dsl_gyro_timer_func(struct hrtimer *timer)
{
	struct lsm6dsl_data *cdata = container_of(timer,
					struct lsm6dsl_data, gyro_timer);

	if (!work_pending(&cdata->gyro_work))
		queue_work(cdata->gyro_wq, &cdata->gyro_work);

	hrtimer_forward_now(&cdata->gyro_timer, cdata->gyro_delay);

	return HRTIMER_RESTART;
}

static enum hrtimer_restart lsm6dsl_acc_timer_func(struct hrtimer *timer)
{
	struct lsm6dsl_data *cdata = container_of(timer,
					struct lsm6dsl_data, acc_timer);

	if (!work_pending(&cdata->acc_work))
		queue_work(cdata->accel_wq, &cdata->acc_work);

	hrtimer_forward_now(&cdata->acc_timer, cdata->acc_delay);

	return HRTIMER_RESTART;
}

static void lsm6dsl_acc_work_func(struct work_struct *work)
{
	struct lsm6dsl_data *cdata =
		container_of(work, struct lsm6dsl_data, acc_work);

	struct timespec ts = ktime_to_timespec(ktime_get_boottime());
	u64 timestamp_new = ts.tv_sec * 1000000000ULL + ts.tv_nsec;
	int time_hi, time_lo;
	int n;
	int err;
	u8 data[6];
	s16 tmp_data[3], raw_data[3];

	err = lsm6dsl_get_poll_data(cdata, LSM6DSL_ACCEL, data);
	if (err < 0)
		goto exit;

	for (n = 0; n < 3; n++) {
		raw_data[n] = *((s16 *)&data[2 * n]);
		if (raw_data[n] == -32768)
			raw_data[n] = -32767;
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

static void lsm6dsl_gyro_work_func(struct work_struct *work)
{
	struct lsm6dsl_data *cdata =
		container_of(work, struct lsm6dsl_data, gyro_work);

	struct timespec ts = ktime_to_timespec(ktime_get_boottime());
	u64 timestamp_new = ts.tv_sec * 1000000000ULL + ts.tv_nsec;
	int time_hi, time_lo;
	int n;
	int err;
	u8 data[6];
	s16 tmp_data[3], raw_data[3];

	err = lsm6dsl_get_poll_data(cdata, LSM6DSL_GYRO, data);
	if (err < 0)
		goto exit;

	/* Applying rotation matrix */
	for (n = 0; n < 3; n++) {
		raw_data[n] = *((s16 *)&data[2 * n]);
		if (raw_data[n] == -32768)
			raw_data[n] = -32767;
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

	cdata->gyro_data[0] = tmp_data[0];
	cdata->gyro_data[1] = tmp_data[1];
	cdata->gyro_data[2] = tmp_data[2];

	time_hi = (int)((timestamp_new & TIME_HI_MASK) >> TIME_HI_SHIFT);
	time_lo = (int)(timestamp_new & TIME_LO_MASK);

	input_report_rel(cdata->gyro_input, REL_RX, cdata->gyro_data[0]);
	input_report_rel(cdata->gyro_input, REL_RY, cdata->gyro_data[1]);
	input_report_rel(cdata->gyro_input, REL_RZ, cdata->gyro_data[2]);
	input_report_rel(cdata->gyro_input, REL_X, time_hi);
	input_report_rel(cdata->gyro_input, REL_Y, time_lo);
	input_sync(cdata->gyro_input);

exit:
	if ((ktime_to_ns(cdata->gyro_delay) * cdata->gyro_time_count)
			>= ((int64_t)ACCEL_LOG_TIME * NSEC_PER_SEC)) {
		SENSOR_INFO("x = %d, y = %d, z = %d\n",
			cdata->gyro_data[0],
			cdata->gyro_data[1], cdata->gyro_data[2]);
		cdata->gyro_time_count = 0;
	} else {
		cdata->gyro_time_count++;
	}
}

static int lsm6dsl_set_drdy_irq(struct lsm6dsl_data *cdata,
				int sindex, bool state)
{
	u8 reg_addr = 0, mask = 0, value;

	if (state)
		value = LSM6DSL_EN_BIT;
	else
		value = LSM6DSL_DIS_BIT;

	switch (sindex) {
	case LSM6DSL_ACCEL:
	case LSM6DSL_GYRO:
		return 0;
	case LSM6DSL_STEP_COUNTER:
	case LSM6DSL_STEP_DETECTOR:
		return 0;
	/* Route Step Detection/sig. Motion Interrupt on INT1 */
	case LSM6DSL_SIGN_MOTION:
		reg_addr = LSM6DSL_INT1_ADDR;
		mask = LSM6DSL_SIGN_MOTION_DRDY_IRQ_MASK;
		break;
	case LSM6DSL_TILT:
		reg_addr = LSM6DSL_MD1_ADDR;
		mask = LSM6DSL_TILT_DRDY_IRQ_MASK;
		break;
	default:
		return -EINVAL;
	}

	return lsm6dsl_write_data_with_mask(cdata, reg_addr, mask, value,
					    true);
}

static int lsm6dsl_set_extra_dependency(struct lsm6dsl_data *cdata,
					bool enable)
{
	int err;

	if (!(cdata->enabled & LSM6DSL_EXTRA_DEPENDENCY)) {
		/* Enable/Disable Embedded Function only once */
		if (enable) {
			err = lsm6dsl_write_data_with_mask(cdata,
						LSM6DSL_FUNC_EN_ADDR,
						LSM6DSL_FUNC_EN_MASK,
						LSM6DSL_EN_BIT, true);
			if (err < 0)
				return err;
		} else {
			err = lsm6dsl_write_data_with_mask(cdata,
						LSM6DSL_FUNC_EN_ADDR,
						LSM6DSL_FUNC_EN_MASK,
						LSM6DSL_DIS_BIT, true);
			if (err < 0)
				return err;
		}
	}

	if (!(cdata->enabled & LSM6DSL_ACCEL_DEPENDENCY)) {
		if (enable) {
			err = lsm6dsl_write_data_with_mask(cdata,
				lsm6dsl_odr_table.addr[LSM6DSL_ACCEL],
				lsm6dsl_odr_table.mask[LSM6DSL_ACCEL],
				LSM6DSL_ODR_26HZ_VAL, true);
			if (err < 0)
				return err;
		} else {
			err = lsm6dsl_write_data_with_mask(cdata,
				lsm6dsl_odr_table.addr[LSM6DSL_ACCEL],
				lsm6dsl_odr_table.mask[LSM6DSL_ACCEL],
				LSM6DSL_ODR_POWER_OFF_VAL, true);
			if (err < 0)
				return err;
		}
	}

	return 0;
}

static int lsm6dsl_enable_step_counter(struct lsm6dsl_data *cdata,
				    bool enable)
{
	cdata->steps_c = cdata->last_steps_c;
	lsm6dsl_reset_steps(cdata);

	SENSOR_INFO("enable : %d, steps_c : %d\n", enable, (int)cdata->steps_c);

	if (enable) {
		/* 1. enable accelerometer ODR at least 26Hz */
		if (!(cdata->enabled & LSM6DSL_ACCEL_DEPENDENCY)) {
			lsm6dsl_write_data_with_mask(cdata,
				lsm6dsl_odr_table.addr[LSM6DSL_ACCEL],
				lsm6dsl_odr_table.mask[LSM6DSL_ACCEL],
				LSM6DSL_ODR_26HZ_VAL, true);
		}
		lsm6dsl_write_data_with_mask(cdata,
				LSM6DSL_ACCEL_FS_ADDR,
				LSM6DSL_ACCEL_FS_MASK,
				LSM6DSL_ACCEL_FS_4G_VAL, true);
		/* 2. enable time stamp and pedometer */
		lsm6dsl_write_data_with_mask(cdata,
				LSM6DSL_PEDOMETER_EN_ADDR,
				LSM6DSL_PEDOMETER_EN_MASK,
				LSM6DSL_EN_BIT, true);
		lsm6dsl_write_data_with_mask(cdata,
				LSM6DSL_TIMER_EN_ADDR,
				LSM6DSL_TIMER_EN_MASK,
				LSM6DSL_EN_BIT, true);
		lsm6dsl_write_data_with_mask(cdata,
				LSM6DSL_INT1_ADDR,
				LSM6DSL_STEP_DETECTOR_DRDY_IRQ_MASK,
				LSM6DSL_EN_BIT, true);
		lsm6dsl_write_data_with_mask(cdata,
				LSM6DSL_INT2_ADDR,
				LSM6DSL_INT2_SC_OV_MASK,
				LSM6DSL_EN_BIT, true);
	} else {
		if (!(cdata->enabled & LSM6DSL_ACCEL_DEPENDENCY)) {
			lsm6dsl_write_data_with_mask(cdata,
				lsm6dsl_odr_table.addr[LSM6DSL_ACCEL],
				lsm6dsl_odr_table.mask[LSM6DSL_ACCEL],
				LSM6DSL_ODR_POWER_OFF_VAL, true);
		}
		lsm6dsl_write_data_with_mask(cdata,
				LSM6DSL_PEDOMETER_EN_ADDR,
				LSM6DSL_PEDOMETER_EN_MASK,
				LSM6DSL_DIS_BIT, true);
		lsm6dsl_write_data_with_mask(cdata,
				LSM6DSL_TIMER_EN_ADDR,
				LSM6DSL_TIMER_EN_MASK,
				LSM6DSL_DIS_BIT, true);
		lsm6dsl_write_data_with_mask(cdata,
				LSM6DSL_INT1_ADDR,
				LSM6DSL_STEP_DETECTOR_DRDY_IRQ_MASK,
				LSM6DSL_DIS_BIT, true);
		lsm6dsl_write_data_with_mask(cdata,
				LSM6DSL_INT2_ADDR,
				LSM6DSL_INT2_SC_OV_MASK,
				LSM6DSL_DIS_BIT, true);
	}

	return 0;
}

static int lsm6dsl_enable_step_counter_fifo(struct lsm6dsl_data *cdata,
				    bool enable)
{
	cdata->steps_c = cdata->last_steps_c;
	lsm6dsl_reset_steps(cdata);

	SENSOR_INFO("enable : %d, steps_c : %d\n", enable, (int)cdata->steps_c);

	if (enable) {
		cdata->step_timestamp = ktime_get_boot_ns();
		lsm6dsl_reset_timestamp(cdata);

		/* FIFO mode, 2046words, 4092bytes, 682steps (6bytes/1step) */
		lsm6dsl_write_data_with_mask(cdata,
				LSM6DSL_FIFO_CTRL1_ADDR,
				0xff, 0xfe, true);
		lsm6dsl_write_data_with_mask(cdata,
				LSM6DSL_FIFO_CTRL2_ADDR,
				0x07, 0x07, true);
		/* enable TIMER_PEDO_FIFO_EN & FIFO_DRDY */
		lsm6dsl_write_data_with_mask(cdata,
				LSM6DSL_FIFO_CTRL2_ADDR,
				LSM6DSL_TIMER_PEDO_FIFO_EN_MASK,
				LSM6DSL_EN_BIT, true);
		lsm6dsl_write_data_with_mask(cdata,
				LSM6DSL_FIFO_CTRL2_ADDR,
				LSM6DSL_TIMER_PEDO_FIFO_DRDY_MASK,
				LSM6DSL_EN_BIT, true);
		/* choose decimation factor, No decimation */
		lsm6dsl_write_data_with_mask(cdata,
				LSM6DSL_FIFO_CTRL4_ADDR,
				LSM6DSL_STOP_ON_FTH_MASK,
				LSM6DSL_EN_BIT, true);
		lsm6dsl_write_data_with_mask(cdata,
				LSM6DSL_FIFO_CTRL4_ADDR,
				LSM6DSL_DEC_DS3_FIFO_MASK,
				0x01, true);
		/* configure FIFO operating : Continuous Mode, FIFO ODR 26Hz */
		lsm6dsl_write_data_with_mask(cdata,
				LSM6DSL_FIFO_CTRL5_ADDR,
				LSM6DSL_ODR_FIFO_MASK,
				0x02, true);
		lsm6dsl_write_data_with_mask(cdata,
				LSM6DSL_FIFO_CTRL5_ADDR,
				LSM6DSL_FIFO_MODE_MASK,
				0x00, true);
		lsm6dsl_write_data_with_mask(cdata,
				LSM6DSL_FIFO_CTRL5_ADDR,
				LSM6DSL_FIFO_MODE_MASK,
				0x06, true);
	} else {
		lsm6dsl_write_data_with_mask(cdata,
				LSM6DSL_FIFO_CTRL2_ADDR,
				LSM6DSL_TIMER_PEDO_FIFO_EN_MASK,
				LSM6DSL_DIS_BIT, true);
		lsm6dsl_write_data_with_mask(cdata,
				LSM6DSL_FIFO_CTRL2_ADDR,
				LSM6DSL_TIMER_PEDO_FIFO_DRDY_MASK,
				LSM6DSL_DIS_BIT, true);
		lsm6dsl_write_data_with_mask(cdata,
				LSM6DSL_FIFO_CTRL4_ADDR,
				LSM6DSL_STOP_ON_FTH_MASK,
				LSM6DSL_DIS_BIT, true);
		lsm6dsl_write_data_with_mask(cdata,
				LSM6DSL_FIFO_CTRL5_ADDR,
				LSM6DSL_ODR_FIFO_MASK,
				0x00, true);
		lsm6dsl_write_data_with_mask(cdata,
				LSM6DSL_FIFO_CTRL5_ADDR,
				LSM6DSL_FIFO_MODE_MASK,
				0x00, true);
	}

	return 0;
}

static int lsm6dsl_enable_sensors(struct lsm6dsl_data *cdata, int sindex)
{
	int i, err;
	u8 data[6];

	if (cdata->enabled & (1 << sindex))
		return 0;

	switch (sindex) {
	case LSM6DSL_ACCEL:
		err = lsm6dsl_write_data_with_mask(cdata,
				lsm6dsl_odr_table.addr[LSM6DSL_ACCEL],
				lsm6dsl_odr_table.mask[LSM6DSL_ACCEL],
				cdata->acc_odr, true);
		if (err < 0)
			return err;

		/* Discard initial 3 samples */
		for (i = 0; i < 3; i++) {
			lsm6dsl_get_poll_data(cdata, LSM6DSL_ACCEL, data);
			usleep_range(10000, 11000);
		}

		hrtimer_start(&cdata->acc_timer, cdata->acc_delay,
							HRTIMER_MODE_REL);

		break;
	case LSM6DSL_GYRO:
		err = lsm6dsl_write_data_with_mask(cdata,
				lsm6dsl_odr_table.addr[LSM6DSL_GYRO],
				lsm6dsl_odr_table.mask[LSM6DSL_GYRO],
				cdata->gyro_odr, true);
		if (err < 0)
			return err;

		/* Discard initial 5 samples */
		for (i = 0; i < 5; i++) {
			lsm6dsl_get_poll_data(cdata, LSM6DSL_GYRO, data);
			usleep_range(10000, 11000);
		}

		hrtimer_start(&cdata->gyro_timer, cdata->gyro_delay,
							HRTIMER_MODE_REL);

		break;
	case LSM6DSL_SIGN_MOTION:
		err = lsm6dsl_write_data_with_mask(cdata,
						LSM6DSL_SIGN_MOTION_EN_ADDR,
						LSM6DSL_SIGN_MOTION_EN_MASK,
						LSM6DSL_EN_BIT, true);
		if (err < 0)
			return err;

		err = lsm6dsl_set_extra_dependency(cdata, true);
		if (err < 0)
			return err;

		err = lsm6dsl_set_drdy_irq(cdata, sindex, true);
		if (err < 0)
			return err;

		cdata->sign_motion_event_ready = true;

		break;
	case LSM6DSL_STEP_COUNTER:
	case LSM6DSL_STEP_DETECTOR:
		err = lsm6dsl_enable_step_counter(cdata, true);
		if (err < 0)
			return err;

		err = lsm6dsl_set_extra_dependency(cdata, true);
		if (err < 0)
			return err;

		break;
	case LSM6DSL_TILT:
		err = lsm6dsl_write_data_with_mask(cdata,
					LSM6DSL_TILT_EN_ADDR,
					LSM6DSL_TILT_EN_MASK,
					LSM6DSL_EN_BIT, true);
		if (err < 0)
			return err;

		err = lsm6dsl_set_extra_dependency(cdata, true);
		if (err < 0)
			return err;

		err = lsm6dsl_set_drdy_irq(cdata, sindex, true);
		if (err < 0)
			return err;

		break;
	default:
		return -EINVAL;
	}

	cdata->enabled |= (1 << sindex);

	SENSOR_INFO("cdata->enabled : 0x%x\n", cdata->enabled);

	return 0;
}

static int lsm6dsl_disable_sensors(struct lsm6dsl_data *cdata, int sindex)
{
	int err;

	if (!(cdata->enabled & (1 << sindex)))
		return 0;

	cdata->enabled &= ~(1 << sindex);

	switch (sindex) {
	case LSM6DSL_ACCEL:
		if ((cdata->enabled & LSM6DSL_EXTRA_DEPENDENCY)
		    || (cdata->sa_flag)) {
			err = lsm6dsl_write_data_with_mask(cdata,
				lsm6dsl_odr_table.addr[LSM6DSL_ACCEL],
				lsm6dsl_odr_table.mask[LSM6DSL_ACCEL],
				LSM6DSL_ODR_26HZ_VAL, true);
		} else {
			err = lsm6dsl_write_data_with_mask(cdata,
				lsm6dsl_odr_table.addr[LSM6DSL_ACCEL],
				lsm6dsl_odr_table.mask[LSM6DSL_ACCEL],
				LSM6DSL_ODR_POWER_OFF_VAL, true);
		}
		if (err < 0)
			return err;

		cancel_work_sync(&cdata->acc_work);
		hrtimer_cancel(&cdata->acc_timer);

		break;
	case LSM6DSL_GYRO:
		err = lsm6dsl_write_data_with_mask(cdata,
				lsm6dsl_odr_table.addr[LSM6DSL_GYRO],
				lsm6dsl_odr_table.mask[LSM6DSL_GYRO],
				LSM6DSL_ODR_POWER_OFF_VAL, true);
		if (err < 0)
			return err;

		cancel_work_sync(&cdata->gyro_work);
		hrtimer_cancel(&cdata->gyro_timer);

		break;
	case LSM6DSL_SIGN_MOTION:
		err = lsm6dsl_write_data_with_mask(cdata,
				LSM6DSL_SIGN_MOTION_EN_ADDR,
				LSM6DSL_SIGN_MOTION_EN_MASK,
				LSM6DSL_DIS_BIT, true);
		if (err < 0)
			return err;

		err = lsm6dsl_set_extra_dependency(cdata, false);
		if (err < 0)
			return err;

		err = lsm6dsl_set_drdy_irq(cdata, sindex, false);
		if (err < 0)
			return err;

		cdata->sign_motion_event_ready = false;

		break;
	case LSM6DSL_STEP_COUNTER:
	case LSM6DSL_STEP_DETECTOR:
		if (cdata->enabled & LSM6DSL_STEP_COUNTER_DEPENDENCY)
			break;

		err = lsm6dsl_enable_step_counter(cdata, false);
		if (err < 0)
			return err;

		err = lsm6dsl_set_extra_dependency(cdata, false);
		if (err < 0)
			return err;

		break;
	case LSM6DSL_TILT:
		err = lsm6dsl_write_data_with_mask(cdata,
				LSM6DSL_TILT_EN_ADDR,
				LSM6DSL_TILT_EN_MASK,
				LSM6DSL_DIS_BIT, true);
		if (err < 0)
			return err;

		err = lsm6dsl_set_extra_dependency(cdata, false);
		if (err < 0)
			return err;

		err = lsm6dsl_set_drdy_irq(cdata, sindex, false);
		if (err < 0)
			return err;

		break;
	default:
		return -EINVAL;
	}

	SENSOR_INFO("cdata->enabled : 0x%x\n", cdata->enabled);

	return 0;
}


static ssize_t lsm6dsl_acc_delay_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct lsm6dsl_data *cdata = input_get_drvdata(input);

	return snprintf(buf, 16, "%lld\n", ktime_to_ns(cdata->acc_delay));
}

static ssize_t lsm6dsl_acc_delay_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct input_dev *input = to_input_dev(dev);
	struct lsm6dsl_data *cdata = input_get_drvdata(input);
	int err;
	unsigned long data;

	err = kstrtoul(buf, 10, &data);
	if (err)
		return count;

	if (data == 0)
		return count;

	if (data > LSM6DSL_DELAY_DEFAULT)
		data = LSM6DSL_DELAY_DEFAULT;

	cdata->acc_delay = ns_to_ktime(data);

	SENSOR_INFO("new_delay = %lld\n", ktime_to_ns(cdata->acc_delay));

	return count;
}

static ssize_t lsm6dsl_acc_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct lsm6dsl_data *cdata = input_get_drvdata(input);

	return snprintf(buf, 16, "%d\n", atomic_read(&cdata->acc_wkqueue_en));

}

static ssize_t lsm6dsl_acc_enable_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct input_dev *input = to_input_dev(dev);
	struct lsm6dsl_data *cdata = input_get_drvdata(input);
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
			lsm6dsl_acc_open_calibration(cdata);
			lsm6dsl_set_fs(cdata, LSM6DSL_ACCEL, cdata->acc_fs);
			lsm6dsl_enable_sensors(cdata, LSM6DSL_ACCEL);
			atomic_set(&cdata->acc_wkqueue_en, 1);
		}
	} else {
		if (pre_enable == 1) {
			atomic_set(&cdata->acc_wkqueue_en, 0);
			lsm6dsl_disable_sensors(cdata, LSM6DSL_ACCEL);
		}
	}
	mutex_unlock(&cdata->mutex_enable);

	return count;
}

static ssize_t lsm6dsl_gyro_delay_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct lsm6dsl_data *cdata = input_get_drvdata(input);

	return snprintf(buf, 16, "%lld\n", ktime_to_ns(cdata->gyro_delay));
}

static ssize_t lsm6dsl_gyro_delay_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct input_dev *input = to_input_dev(dev);
	struct lsm6dsl_data *cdata = input_get_drvdata(input);
	int err;
	unsigned long data;

	err = kstrtoul(buf, 10, &data);
	if (err)
		return err;

	if (data == 0)
		return count;

	if (data > LSM6DSL_DELAY_DEFAULT)
		data = LSM6DSL_DELAY_DEFAULT;

	cdata->gyro_delay = ns_to_ktime(data);

	SENSOR_INFO("new_delay = %lld\n", ktime_to_ns(cdata->gyro_delay));

	return count;
}

static ssize_t lsm6dsl_gyro_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct lsm6dsl_data *cdata = input_get_drvdata(input);

	return snprintf(buf, 2, "%d\n", atomic_read(&cdata->gyro_wkqueue_en));
}

static ssize_t lsm6dsl_gyro_enable_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct input_dev *input = to_input_dev(dev);
	struct lsm6dsl_data *cdata = input_get_drvdata(input);
	int err;
	u8 enable;
	int pre_enable = atomic_read(&cdata->gyro_wkqueue_en);

	err = kstrtou8(buf, 10, &enable);
	if (err)
		return count;

	enable = enable ? 1 : 0;

	SENSOR_INFO("new_value = %d, pre_enable = %d\n", enable, pre_enable);

	mutex_lock(&cdata->mutex_enable);
	if (enable) {
		if (pre_enable == 0) {
			lsm6dsl_set_fs(cdata, LSM6DSL_GYRO, cdata->gyro_fs);
			lsm6dsl_enable_sensors(cdata, LSM6DSL_GYRO);
			atomic_set(&cdata->gyro_wkqueue_en, 1);
		}
	} else {
		if (pre_enable == 1) {
			atomic_set(&cdata->gyro_wkqueue_en, 0);
			lsm6dsl_disable_sensors(cdata, LSM6DSL_GYRO);
		}
	}
	mutex_unlock(&cdata->mutex_enable);

	return count;
}

static ssize_t lsm6dsl_smd_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct lsm6dsl_data *cdata = input_get_drvdata(input);
	int ret;

	if (cdata->enabled & (1 << LSM6DSL_SIGN_MOTION))
		ret = 1;
	else
		ret = 0;

	return snprintf(buf, 16, "%d\n", ret);
}

static ssize_t lsm6dsl_smd_enable_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct input_dev *input = to_input_dev(dev);
	struct lsm6dsl_data *cdata = input_get_drvdata(input);
	int err;
	u8 enable;

	err = kstrtou8(buf, 10, &enable);
	if (err)
		return count;

	enable = enable ? 1 : 0;

	SENSOR_INFO("new_value = %d\n",	enable);

	mutex_lock(&cdata->mutex_enable);
	if (enable) {
		lsm6dsl_enable_sensors(cdata, LSM6DSL_SIGN_MOTION);
		lsm6dsl_set_irq(cdata, 1);
	} else {
		lsm6dsl_set_irq(cdata, 0);
		lsm6dsl_disable_sensors(cdata, LSM6DSL_SIGN_MOTION);
	}
	mutex_unlock(&cdata->mutex_enable);

	return count;
}

static ssize_t lsm6dsl_tilt_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct lsm6dsl_data *cdata = input_get_drvdata(input);
	int ret;

	if (cdata->enabled & (1 << LSM6DSL_TILT))
		ret = 1;
	else
		ret = 0;

	return snprintf(buf, 16, "%d\n", ret);

}

static ssize_t lsm6dsl_tilt_enable_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct input_dev *input = to_input_dev(dev);
	struct lsm6dsl_data *cdata = input_get_drvdata(input);
	int err;
	u8 enable;

	err = kstrtou8(buf, 10, &enable);
	if (err)
		return count;

	enable = enable ? 1 : 0;

	SENSOR_INFO("new_value = %d\n",	enable);

	mutex_lock(&cdata->mutex_enable);
	if (enable) {
		lsm6dsl_enable_sensors(cdata, LSM6DSL_TILT);
		lsm6dsl_set_irq(cdata, 1);
	} else {
		lsm6dsl_set_irq(cdata, 0);
		lsm6dsl_disable_sensors(cdata, LSM6DSL_TILT);
	}
	mutex_unlock(&cdata->mutex_enable);

	return count;
}

static ssize_t lsm6dsl_step_counter_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct lsm6dsl_data *cdata = input_get_drvdata(input);
	int ret;

	if (cdata->enabled & (1 << LSM6DSL_STEP_COUNTER))
		ret = 1;
	else
		ret = 0;

	return snprintf(buf, 16, "%d\n", ret);

}

static ssize_t lsm6dsl_step_counter_enable_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct input_dev *input = to_input_dev(dev);
	struct lsm6dsl_data *cdata = input_get_drvdata(input);
	int err;
	u8 enable;

	err = kstrtou8(buf, 10, &enable);
	if (err)
		return count;

	enable = enable ? 1 : 0;

	SENSOR_INFO("new_value = %d\n",	enable);

	mutex_lock(&cdata->mutex_enable);
	if (enable) {
		lsm6dsl_enable_sensors(cdata, LSM6DSL_STEP_COUNTER);
		lsm6dsl_set_irq(cdata, 1);
		input_report_rel(cdata->sc_input, REL_MISC,
				cdata->last_steps_c + 1);
		input_sync(cdata->sc_input);
	} else {
		lsm6dsl_set_irq(cdata, 0);
		lsm6dsl_disable_sensors(cdata, LSM6DSL_STEP_COUNTER);
	}
	mutex_unlock(&cdata->mutex_enable);

	return count;
}

static ssize_t lsm6dsl_step_detector_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct lsm6dsl_data *cdata = input_get_drvdata(input);
	int ret;

	if (cdata->enabled & (1 << LSM6DSL_STEP_DETECTOR))
		ret = 1;
	else
		ret = 0;

	return snprintf(buf, 16, "%d\n", ret);

}

static ssize_t lsm6dsl_step_detector_enable_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct input_dev *input = to_input_dev(dev);
	struct lsm6dsl_data *cdata = input_get_drvdata(input);
	int err;
	u8 enable;

	err = kstrtou8(buf, 10, &enable);
	if (err)
		return count;

	enable = enable ? 1 : 0;

	SENSOR_INFO("new_value = %d\n",	enable);

	mutex_lock(&cdata->mutex_enable);
	if (enable) {
		lsm6dsl_enable_sensors(cdata, LSM6DSL_STEP_DETECTOR);
		lsm6dsl_set_irq(cdata, 1);
	} else {
		lsm6dsl_set_irq(cdata, 0);
		lsm6dsl_disable_sensors(cdata, LSM6DSL_STEP_DETECTOR);
	}
	mutex_unlock(&cdata->mutex_enable);

	return count;
}


static struct device_attribute dev_attr_acc_poll_delay =
	__ATTR(poll_delay, 0644, lsm6dsl_acc_delay_show,
	lsm6dsl_acc_delay_store);
static struct device_attribute dev_attr_acc_enable =
	__ATTR(enable, 0644, lsm6dsl_acc_enable_show,
	lsm6dsl_acc_enable_store);

static struct attribute *lsm6dsl_acc_attributes[] = {
	&dev_attr_acc_poll_delay.attr,
	&dev_attr_acc_enable.attr,
	NULL
};

static struct attribute_group lsm6dsl_accel_attribute_group = {
	.attrs = lsm6dsl_acc_attributes
};

static struct device_attribute dev_attr_gyro_poll_delay =
	__ATTR(poll_delay, 0644, lsm6dsl_gyro_delay_show,
	lsm6dsl_gyro_delay_store);
static struct device_attribute dev_attr_gyro_enable =
	__ATTR(enable, 0644, lsm6dsl_gyro_enable_show,
	lsm6dsl_gyro_enable_store);

static struct attribute *lsm6dsl_gyro_attributes[] = {
	&dev_attr_gyro_poll_delay.attr,
	&dev_attr_gyro_enable.attr,
	NULL
};

static struct attribute_group lsm6dsl_gyro_attribute_group = {
	.attrs = lsm6dsl_gyro_attributes
};


static struct device_attribute dev_attr_smd_enable =
	__ATTR(enable, 0644, lsm6dsl_smd_enable_show,
	lsm6dsl_smd_enable_store);

static struct attribute *lsm6dsl_smd_attributes[] = {
	&dev_attr_smd_enable.attr,
	NULL
};

static struct attribute_group lsm6dsl_smd_attribute_group = {
	.attrs = lsm6dsl_smd_attributes
};

static struct device_attribute dev_attr_tilt_enable =
	__ATTR(enable, 0644, lsm6dsl_tilt_enable_show,
	lsm6dsl_tilt_enable_store);

static struct attribute *lsm6dsl_tilt_attributes[] = {
	&dev_attr_tilt_enable.attr,
	NULL
};

static struct attribute_group lsm6dsl_tilt_attribute_group = {
	.attrs = lsm6dsl_tilt_attributes
};

static struct device_attribute dev_attr_sc_enable =
	__ATTR(enable, 0644, lsm6dsl_step_counter_enable_show,
	lsm6dsl_step_counter_enable_store);

static struct attribute *lsm6dsl_sc_attributes[] = {
	&dev_attr_sc_enable.attr,
	NULL
};

static struct attribute_group lsm6dsl_sc_attribute_group = {
	.attrs = lsm6dsl_sc_attributes
};

static struct device_attribute dev_attr_sd_enable =
	__ATTR(enable, 0644, lsm6dsl_step_detector_enable_show,
	lsm6dsl_step_detector_enable_store);

static struct attribute *lsm6dsl_sd_attributes[] = {
	&dev_attr_sd_enable.attr,
	NULL
};

static struct attribute_group lsm6dsl_sd_attribute_group = {
	.attrs = lsm6dsl_sd_attributes
};

/* FATORY SYSFS */
static ssize_t lsm6dsl_vendor_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", VENDOR_NAME);
}

static ssize_t lsm6dsl_name_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", LSM6DSL_DEV_NAME);
}

static ssize_t selftest_revised_show(struct device *dev,
  	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", SELFTEST_REVISED);
}

static ssize_t lsm6dsl_accel_dhr_sensor_info_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	u8 fs_reg_val = 0;
	u8 fs = 0;
	struct lsm6dsl_data *cdata = dev_get_drvdata(dev);

	cdata->tf->read(cdata, LSM6DSL_ACCEL_FS_ADDR, 1, &fs_reg_val, true);
	fs_reg_val &= LSM6DSL_ACCEL_FS_MASK;
	fs_reg_val = fs_reg_val >> 2;

	switch (fs_reg_val) {
	case LSM6DSL_ACCEL_FS_2G_VAL:
		fs = 2;
		break;
	case LSM6DSL_ACCEL_FS_4G_VAL:
		fs = 4;
		break;
	case LSM6DSL_ACCEL_FS_8G_VAL:
		fs = 8;
		break;
	case LSM6DSL_ACCEL_FS_16G_VAL:
		fs = 16;
		break;
	default:
		fs = 4;
		break;
	}

	return snprintf(buf, PAGE_SIZE, "\"FULL_SCALE\":\"%uG\"\n", fs);
}

static ssize_t lsm6dsl_temperature_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret;
	s16 temp;
	u8 data[2];

	struct lsm6dsl_data *cdata = dev_get_drvdata(dev);

	ret = cdata->tf->read(cdata, LSM6DSL_OUT_TEMP_L_ADDR, 2, data, true);
	if (ret < 0)
		SENSOR_ERR("lsm6dsl_i2c_read_dev failed ret = %d\n", ret);

	temp = (s16)(data[0] | (data[1] << 8))
				/ SENSITIVITY_TEMP + TEMP_OUTPUT_ZERO_LEVEL;

	return snprintf(buf, PAGE_SIZE, "%d\n", temp);
}

/* Accelerometer Calibraion */
int lsm6dsl_acc_open_calibration(struct lsm6dsl_data *cdata)
{
	int ret = 0;
	mm_segment_t old_fs;
	struct file *cal_filp = NULL;

	SENSOR_INFO("\n");

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

static int lsm6dsl_acc_do_calibrate(struct lsm6dsl_data *cdata, int enable)
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

static ssize_t lsm6dsl_acc_calibration_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int ret;
	struct lsm6dsl_data *cdata = dev_get_drvdata(dev);

	SENSOR_INFO("\n");

	ret = lsm6dsl_acc_open_calibration(cdata);
	if (ret < 0)
		SENSOR_ERR("calibration open failed = %d\n", ret);

	SENSOR_INFO("cal data %d %d %d, ret = %d\n", cdata->accel_cal_data[0],
		cdata->accel_cal_data[1], cdata->accel_cal_data[2], ret);

	return snprintf(buf, PAGE_SIZE, "%d %d %d %d\n", ret,
		cdata->accel_cal_data[0], cdata->accel_cal_data[1],
		cdata->accel_cal_data[2]);
}

static ssize_t lsm6dsl_acc_calibration_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int ret;
	int64_t dEnable;
	struct lsm6dsl_data *cdata = dev_get_drvdata(dev);

	SENSOR_INFO("\n");

	ret = kstrtoll(buf, 10, &dEnable);
	if (ret < 0) {
		SENSOR_ERR("kstrtoll failed\n");
		return size;
	}

	ret = lsm6dsl_acc_do_calibrate(cdata, (int)dEnable);
	if (ret < 0)
		SENSOR_ERR("accel calibrate failed\n");

	return size;
}

/* Accelerometer LPF */
static int lsm6dsl_set_lpf(struct lsm6dsl_data *cdata, int onoff)
{
	int err;
	u8 odr;

	SENSOR_INFO("onoff = %d\n", onoff);

	if (onoff)
		odr = LSM6DSL_ACCEL_ODR_104HZ_VAL;
	else
		odr = LSM6DSL_ACCEL_ODR_6660HZ_VAL;

	err = lsm6dsl_write_data_with_mask(cdata,
					   LSM6DSL_ACCEL_ODR_ADDR,
					   LSM6DSL_ACCEL_ODR_MASK,
					   odr, true);
	if (err < 0)
		return err;

	cdata->lpf_on = onoff;

	return err;
}

static ssize_t lsm6dsl_lowpassfilter_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int ret = 0;
	struct lsm6dsl_data *cdata = dev_get_drvdata(dev);

	if (cdata->lpf_on)
		ret = 1;
	else
		ret = 0;

	return snprintf(buf, PAGE_SIZE, "%d\n", ret);
}

static ssize_t lsm6dsl_lowpassfilter_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int ret;
	int64_t dEnable;
	struct lsm6dsl_data *cdata;

	cdata = dev_get_drvdata(dev);
	SENSOR_INFO("\n");

	ret = kstrtoll(buf, 10, &dEnable);
	if (ret < 0)
		SENSOR_ERR("kstrtoll failed, ret = %d\n", ret);

	ret = lsm6dsl_set_lpf(cdata, dEnable);
	if (ret < 0)
		SENSOR_ERR("lsm6dsl_set_lpf failed, ret = %d\n", ret);

	return size;
}

/* reactive alert */
static ssize_t lsm6dsl_smart_alert_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct lsm6dsl_data *cdata = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", cdata->sa_irq_state);
}

static ssize_t lsm6dsl_smart_alert_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct lsm6dsl_data *cdata = dev_get_drvdata(dev);

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

		lsm6dsl_write_data_with_mask(cdata,
					   LSM6DSL_ACCEL_ODR_ADDR,
					   LSM6DSL_ACCEL_ODR_MASK,
					   LSM6DSL_ACCEL_ODR_POWER_DOWN, true);
		mdelay(100);

		if (factory_mode == 1) {
			threshold = 0;
			odr = LSM6DSL_ODR_208HZ_VAL;
			duration = 0x00;
		} else {
			switch (cdata->acc_fs) {
			case LSM6DSL_ACCEL_FS_2G_VAL:
				fs = 2000;
				break;
			case LSM6DSL_ACCEL_FS_4G_VAL:
				fs = 4000;
				break;
			case LSM6DSL_ACCEL_FS_8G_VAL:
				fs = 8000;
				break;
			case LSM6DSL_ACCEL_FS_16G_VAL:
				fs = 16000;
				break;
			default:
				return size;
			}

			threshold = SA_DYNAMIC_THRESHOLD * 64;
			threshold += fs / 2;
			threshold = (threshold / fs) & 0x3f;
			SENSOR_INFO("fs= %d, thr= %d[mg] = %d\n",
				fs, SA_DYNAMIC_THRESHOLD, threshold);

			odr = LSM6DSL_ODR_26HZ_VAL;
			duration = 0x02;
		}

		lsm6dsl_write_data_with_mask(cdata,
					   LSM6DSL_BASIC_INT_EN_ADDR,
					   LSM6DSL_BASIC_INT_EN_MASK,
					   LSM6DSL_EN_BIT, true);

		lsm6dsl_write_data_with_mask(cdata,
					   LSM6DSL_MD1_ADDR,
					   LSM6DSL_INT1_WU_MASK,
					   LSM6DSL_EN_BIT, true);

		lsm6dsl_write_data_with_mask(cdata,
					   LSM6DSL_WU_THS_ADDR,
					   LSM6DSL_WU_THS_MASK,
					   threshold, true);

		lsm6dsl_write_data_with_mask(cdata,
					   LSM6DSL_WU_DUR_ADDR,
					   LSM6DSL_WU_DUR_MASK,
					   duration, true);

		lsm6dsl_write_data_with_mask(cdata,
					   LSM6DSL_ACCEL_ODR_ADDR,
					   LSM6DSL_ACCEL_ODR_MASK,
					   odr, true);
		mdelay(100);

		lsm6dsl_set_irq(cdata, 1);
		SENSOR_INFO("smart alert is on!\n");
	} else if ((enable == 0) && (cdata->sa_flag == 1)) {
		lsm6dsl_set_irq(cdata, 0);
		cdata->sa_flag = 0;

		lsm6dsl_write_data_with_mask(cdata,
					   LSM6DSL_MD1_ADDR,
					   LSM6DSL_INT1_WU_MASK,
					   LSM6DSL_DIS_BIT, true);

		lsm6dsl_write_data_with_mask(cdata,
					   LSM6DSL_BASIC_INT_EN_ADDR,
					   LSM6DSL_BASIC_INT_EN_MASK,
					   LSM6DSL_DIS_BIT, true);

		threshold = 0x3f;
		lsm6dsl_write_data_with_mask(cdata,
					   LSM6DSL_WU_THS_ADDR,
					   LSM6DSL_WU_THS_MASK,
					   threshold, true);


		if (cdata->enabled & (1 << LSM6DSL_ACCEL))
			odr = cdata->acc_odr;
		else if (cdata->enabled & LSM6DSL_EXTRA_DEPENDENCY)
			odr = LSM6DSL_ODR_26HZ_VAL;

		lsm6dsl_write_data_with_mask(cdata,
					   LSM6DSL_ACCEL_ODR_ADDR,
					   LSM6DSL_ACCEL_ODR_MASK,
					   odr, true);

		SENSOR_INFO("smart alert is off! irq = %d, odr 0x%x\n",
						cdata->sa_irq_state, odr);
	}

	return size;
}

/* raw_data */
static ssize_t lsm6dsl_acc_raw_data_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	signed short cx, cy, cz;
	struct lsm6dsl_data *cdata;

	cdata = dev_get_drvdata(dev);

	cx = cdata->accel_data[0];
	cy = cdata->accel_data[1];
	cz = cdata->accel_data[2];

	return snprintf(buf, PAGE_SIZE, "%d, %d, %d\n", cx, cy, cz);
}

static ssize_t lsm6dsl_gyro_raw_data_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	signed short cx, cy, cz;
	struct lsm6dsl_data *cdata;

	cdata = dev_get_drvdata(dev);

	cx = cdata->gyro_data[0];
	cy = cdata->gyro_data[1];
	cz = cdata->gyro_data[2];

	return snprintf(buf, PAGE_SIZE, "%d, %d, %d\n", cx, cy, cz);
}

/* Selftest */
static int lsm6dsl_set_selftest(struct lsm6dsl_data *cdata,
				int mode, int sindex)
{
	int err;
	u8 mask;

	switch (sindex) {
	case LSM6DSL_ACCEL:
		mask = LSM6DSL_SELFTEST_ACCEL_MASK;
		break;
	case LSM6DSL_GYRO:
		mask = LSM6DSL_SELFTEST_GYRO_MASK;
		break;
	default:
		return -EINVAL;
	}

	err = lsm6dsl_write_data_with_mask(cdata,
			LSM6DSL_SELFTEST_ADDR, mask, mode, true);
	if (err < 0)
		return err;

	return 0;
}

static int lsm6dsl_acc_hw_selftest(struct lsm6dsl_data *cdata,
		s32 *NOST, s32 *ST, s32 *N_ST, s32 *DIFF_ST, s32 *N_DIFF_ST)
{
	int err;
	int p_result = 0, n_result = 0;
	u8 buf = 0x00;
	s16 nOutData[3] = {0,};
	s32 i, retry;
	u8 testset_regs[10] = {0x30, 0x00, 0x44, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00};

	err = cdata->tf->write(cdata,
			LSM6DSL_CTRL1_ADDR,
			10, testset_regs, true);
	if (err < 0)
		goto XL_HW_SELF_EXIT;

	mdelay(200);

	retry = ACC_DA_RETRY_COUNT;
	do {
		usleep_range(1000, 1100);
		err = cdata->tf->read(cdata,
				LSM6DSL_SRC_ACCEL_GYRO_REG,
				1, &buf, true);
		if (err < 0)
			goto XL_HW_SELF_EXIT;

		retry--;
		if (!retry)
			break;
	} while (!(buf & 0x01));

	err = cdata->tf->read(cdata,
			LSM6DSL_ACCEL_OUT_X_L_ADDR,
			6, (u8 *)nOutData, true);
	if (err < 0)
		goto XL_HW_SELF_EXIT;

	for (i = 0; i < 6; i++) {
		retry = ACC_DA_RETRY_COUNT;
		do {
			usleep_range(1000, 1100);
			err = cdata->tf->read(cdata,
					LSM6DSL_SRC_ACCEL_GYRO_REG,
					1, &buf, true);
			if (err < 0)
				goto XL_HW_SELF_EXIT;
			retry--;
			if (!retry)
				break;
		} while (!(buf & 0x01));

		err = cdata->tf->read(cdata,
				LSM6DSL_ACCEL_OUT_X_L_ADDR,
				6, (u8 *)nOutData, true);
		if (err < 0)
			goto XL_HW_SELF_EXIT;

		if (i > 0) {
			NOST[0] += nOutData[0];
			NOST[1] += nOutData[1];
			NOST[2] += nOutData[2];
		}
	}

	NOST[0] /= 5;
	NOST[1] /= 5;
	NOST[2] /= 5;

	err = lsm6dsl_set_selftest(cdata,
			LSM6DSL_SELF_TEST_ACC_POS_SIGN_VAL, LSM6DSL_ACCEL);
	if (err < 0)
		goto XL_HW_SELF_EXIT;

	mdelay(200);

	retry = ACC_DA_RETRY_COUNT;
	do {
		usleep_range(1000, 1100);
		err = cdata->tf->read(cdata,
				LSM6DSL_SRC_ACCEL_GYRO_REG,
				1, &buf, true);
		if (err < 0)
			goto XL_HW_SELF_EXIT;
		retry--;
		if (!retry)
			break;
	} while (!(buf & 0x01));

	err = cdata->tf->read(cdata,
			LSM6DSL_ACCEL_OUT_X_L_ADDR,
			6, (u8 *)nOutData, true);
	if (err < 0)
		goto XL_HW_SELF_EXIT;

	for (i = 0; i < 6; i++) {
		retry = ACC_DA_RETRY_COUNT;
		do {
			usleep_range(1000, 1100);
			err = cdata->tf->read(cdata,
					LSM6DSL_SRC_ACCEL_GYRO_REG,
					1, &buf, true);
			if (err < 0)
				goto XL_HW_SELF_EXIT;

			retry--;
			if (!retry)
				break;
		} while (!(buf & 0x01));

		err = cdata->tf->read(cdata,
				LSM6DSL_ACCEL_OUT_X_L_ADDR,
				6, (u8 *)nOutData, true);
		if (err < 0)
			goto XL_HW_SELF_EXIT;

		if (i > 0) {
			ST[0] += nOutData[0];
			ST[1] += nOutData[1];
			ST[2] += nOutData[2];
		}
	}

	ST[0] /= 5;
	ST[1] /= 5;
	ST[2] /= 5;

	err = lsm6dsl_set_selftest(cdata,
			LSM6DSL_SELF_TEST_ACC_NEG_SIGN_VAL, LSM6DSL_ACCEL);
	if (err < 0)
		goto XL_HW_SELF_EXIT;

	mdelay(200);

	retry = ACC_DA_RETRY_COUNT;
	do {
		usleep_range(1000, 1100);
		err = cdata->tf->read(cdata,
				LSM6DSL_SRC_ACCEL_GYRO_REG,
				1, &buf, true);
		if (err < 0)
			goto XL_HW_SELF_EXIT;
		retry--;
		if (!retry)
			break;
	} while (!(buf & 0x01));

	err = cdata->tf->read(cdata,
			LSM6DSL_ACCEL_OUT_X_L_ADDR,
			6, (u8 *)nOutData, true);
	if (err < 0)
		goto XL_HW_SELF_EXIT;

	for (i = 0; i < 6; i++) {
		retry = ACC_DA_RETRY_COUNT;
		do {
			usleep_range(1000, 1100);
			err = cdata->tf->read(cdata,
					LSM6DSL_SRC_ACCEL_GYRO_REG,
					1, &buf, true);
			if (err < 0)
				goto XL_HW_SELF_EXIT;

			retry--;
			if (!retry)
				break;
		} while (!(buf & 0x01));

		err = cdata->tf->read(cdata,
				LSM6DSL_ACCEL_OUT_X_L_ADDR,
				6, (u8 *)nOutData, true);
		if (err < 0)
			goto XL_HW_SELF_EXIT;

		if (i > 0) {
			N_ST[0] += nOutData[0];
			N_ST[1] += nOutData[1];
			N_ST[2] += nOutData[2];
		}
	}

	N_ST[0] /= 5;
	N_ST[1] /= 5;
	N_ST[2] /= 5;

	buf = 0x00;
	err = cdata->tf->write(cdata,
			LSM6DSL_CTRL1_ADDR,
			1, &buf, true);
	if (err < 0)
		goto XL_HW_SELF_EXIT;

	err = lsm6dsl_set_selftest(cdata,
			LSM6DSL_SELF_TEST_DISABLED_VAL, LSM6DSL_ACCEL);
	if (err < 0)
		goto XL_HW_SELF_EXIT;

	for (i = 0; i < 3; i++) {
		DIFF_ST[i] = ABS(ST[i] - NOST[i]);
		if ((LSM6DSL_ACC_MIN_ST > DIFF_ST[i])
			|| (LSM6DSL_ACC_MAX_ST < DIFF_ST[i])) {
			p_result++;
		}
	}

	for (i = 0; i < 3; i++) {
		N_DIFF_ST[i] = ABS(N_ST[i] - NOST[i]);
		if ((LSM6DSL_ACC_MIN_ST > N_DIFF_ST[i])
			|| (LSM6DSL_ACC_MAX_ST < N_DIFF_ST[i])) {
			n_result++;
		}
	}

	SENSOR_INFO("p_result = %d,  n_result = %d\n", p_result, n_result);

	if (p_result > 0)
		return 0;
	else
		return 1;

XL_HW_SELF_EXIT:
	return err;
}

static int lsm6dsl_gyro_fifo_test(struct lsm6dsl_data *cdata,
		s16 *zero_rate_lsb, s32 *fifo_cnt, u8 *fifo_pass,
		s16 *slot_raw, char *out_str)
{
	int err;
	u8 buf[5] = {0x00,};
	s16 nFifoDepth = (LSM6DSL_FIFO_TEST_DEPTH + 1) * 3;
	bool zero_rate_read_2nd = 0;
	s16 raw[3] = {0,}, zero_rate_delta[3] = {0,}, length = 0;
	s16 data[LSM6DSL_FIFO_TEST_DEPTH * 3] = {0,};
	s32 i = 0, j = 0, sum_raw[3] = {0,};

	SENSOR_INFO("start\n");
	err = lsm6dsl_write_data_with_mask(cdata,
			LSM6DSL_CTRL4_ADDR, 0x40, LSM6DSL_DIS_BIT, true);
	if (err < 0)
		return err;

	err = lsm6dsl_write_data_with_mask(cdata,
			LSM6DSL_CTRL4_ADDR, 0x01, LSM6DSL_EN_BIT, true);
	if (err < 0)
		return err;

	err = lsm6dsl_write_data_with_mask(cdata,
			LSM6DSL_CTRL2_ADDR, 0xf0, LSM6DSL_ODR_104HZ_VAL, true);
	if (err < 0)
		return err;

	err = lsm6dsl_write_data_with_mask(cdata,
			LSM6DSL_CTRL2_ADDR, 0x0c,
			LSM6DSL_GYRO_FS_2000_VAL, true);
	if (err < 0)
		return err;

	err = lsm6dsl_write_data_with_mask(cdata,
			LSM6DSL_CTRL3_ADDR, 0x40, LSM6DSL_DIS_BIT, true);
	if (err < 0)
		return err;

	buf[0] = (u8)(nFifoDepth & 0xff);
	err = cdata->tf->write(cdata,
			LSM6DSL_FIFO_CTRL1_ADDR,
			1, buf, true);
	if (err < 0)
		return err;

	buf[0] = (u8)((nFifoDepth >> 8) & 0x0f);
	err = lsm6dsl_write_data_with_mask(cdata,
			LSM6DSL_FIFO_CTRL2_ADDR, 0x0f, buf[0], true);
	if (err < 0)
		return err;

	buf[0] = (0x01 << 3);
	err = cdata->tf->write(cdata,
			LSM6DSL_FIFO_CTRL3_ADDR,
			1, buf, true);
	if (err < 0)
		return err;

	buf[0] = 0x00;
	err = cdata->tf->write(cdata,
			LSM6DSL_FIFO_CTRL4_ADDR,
			1, buf, true);
	if (err < 0)
		return err;

	buf[0] = (0x04 << 3) | 0x01;
	err = cdata->tf->write(cdata,
			LSM6DSL_FIFO_CTRL5_ADDR,
			1, buf, true);
	if (err < 0)
		return err;

	mdelay(800);

read_zero_rate_again:
	err = lsm6dsl_write_data_with_mask(cdata,
			LSM6DSL_FIFO_CTRL5_ADDR, 0x07, 0x00, true);
	if (err < 0)
		return err;

	err = lsm6dsl_write_data_with_mask(cdata,
			LSM6DSL_FIFO_CTRL5_ADDR, 0x07, 0x01, true);
	if (err < 0)
		return err;

	length = LSM6DSL_FIFO_TEST_DEPTH * 3;
	while (1) {
		mdelay(20);
		err = cdata->tf->read(cdata,
				LSM6DSL_FIFO_STAT1_ADDR,
				4, buf, true);
		if (err < 0)
			return err;

		if (buf[1] & 0xA0)
			break;

		if ((--length) == 0) {
			SENSOR_ERR("fifo not filled\n");
			return -EBUSY;
		}
	}

	length = LSM6DSL_FIFO_TEST_DEPTH * 3;
	for (i = 0; i < length; i++) {
		err = cdata->tf->read(cdata,
				LSM6DSL_FIFO_OUT_L_ADDR,
				2, (u8 *)(data + i), true);
		if (err < 0) {
			SENSOR_ERR("Reading fifo output is fail\n");
			return err;
		}
	}

	*fifo_cnt = 0;
	/* Check fifo pass or fail */
	for (i = 0; i < length; i += 3) {
		*fifo_cnt += 1;

		raw[0] = data[i];
		raw[1] = data[i + 1];
		raw[2] = data[i + 2];

		sum_raw[0] += raw[0];
		sum_raw[1] += raw[1];
		sum_raw[2] += raw[2];

		for (j = 0; j < 3; j++) {
			if (raw[j] < LSM6DSL_GYR_MIN_ZRL
				|| raw[j] > LSM6DSL_GYR_MAX_ZRL) {
				slot_raw[0] = raw[0] * 7 / 100;
				slot_raw[1] = raw[1] * 7 / 100;
				slot_raw[2] = raw[2] * 7 / 100;
				*fifo_pass = 0;
				SENSOR_INFO("fifo fail = %d\n", *fifo_pass);
				return -EAGAIN;
			}
		}
	}

	for (i = 0; i < 3; i++) {
		zero_rate_lsb[i] = sum_raw[i];
		zero_rate_lsb[i] += (LSM6DSL_FIFO_TEST_DEPTH / 2);
		zero_rate_lsb[i] /= LSM6DSL_FIFO_TEST_DEPTH;
	}

	if (zero_rate_read_2nd == 1) {
		*fifo_pass = 1;
		SENSOR_INFO("fifo pass = %d\n", *fifo_pass);
		/* check zero_rate second time */
		zero_rate_delta[0] -= zero_rate_lsb[0];
		zero_rate_delta[1] -= zero_rate_lsb[1];
		zero_rate_delta[2] -= zero_rate_lsb[2];
		for (i = 0; i < 3; i++) {
			if (ABS(zero_rate_delta[i]) > LSM6DSL_GYR_ZRL_DELTA)
				return -EAGAIN;
		}
	} else {
		/* check zero_rate first time, go to check again */
		zero_rate_read_2nd = 1;
		sum_raw[0] = 0;
		sum_raw[1] = 0;
		sum_raw[2] = 0;
		zero_rate_delta[0] = zero_rate_lsb[0];
		zero_rate_delta[1] = zero_rate_lsb[1];
		zero_rate_delta[2] = zero_rate_lsb[2];

		goto read_zero_rate_again;
	}

	buf[0] = 0x00;
	buf[1] = 0x00;
	buf[2] = 0x00;
	buf[3] = 0x00;
	buf[4] = 0x00;
	err = cdata->tf->write(cdata, LSM6DSL_FIFO_CTRL5_ADDR, 1, buf, true);
	if (err < 0)
		return err;

	return 0;
}

static int lsm6dsl_gyro_hw_selftest(struct lsm6dsl_data *cdata,
		s32 *NOST, s32 *ST, s32 *DIFF_ST)
{
	int err;
	u8 buf = 0x00;
	s16 nOutData[3] = {0,};
	s32 i, retry;
	u8 testset_regs[10] = {0x00, 0x5C, 0x44, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00};

	SENSOR_INFO("start\n");

	err = cdata->tf->write(cdata,
					LSM6DSL_CTRL1_ADDR,
					10, testset_regs, true);
	if (err < 0)
		goto G_HW_SELF_EXIT;

	mdelay(800);

	retry = GYR_DA_RETRY_COUNT;
	do {
		usleep_range(1000, 1100);
		err = cdata->tf->read(cdata,
				LSM6DSL_SRC_ACCEL_GYRO_REG,
				1, &buf, true);
		if (err < 0)
			goto G_HW_SELF_EXIT;

		retry--;
		if (!retry)
			break;
	} while (!(buf & 0x02));

	err = cdata->tf->read(cdata,
			LSM6DSL_GYRO_OUT_X_L_ADDR,
			6, (u8 *)nOutData, true);
	if (err < 0)
		goto G_HW_SELF_EXIT;

	for (i = 0; i < 6; i++) {
		retry = GYR_DA_RETRY_COUNT;
		do {
			usleep_range(1000, 1100);
			err = cdata->tf->read(cdata,
					LSM6DSL_SRC_ACCEL_GYRO_REG,
					1, &buf, true);
			if (err < 0)
				goto G_HW_SELF_EXIT;

			retry--;
			if (!retry)
				break;
		} while (!(buf & 0x02));

		err = cdata->tf->read(cdata,
				LSM6DSL_GYRO_OUT_X_L_ADDR,
				6, (u8 *)nOutData, true);
		if (err < 0)
			goto G_HW_SELF_EXIT;

		if (i > 0) {
			NOST[0] += nOutData[0];
			NOST[1] += nOutData[1];
			NOST[2] += nOutData[2];
		}
	}

	NOST[0] /= 5;
	NOST[1] /= 5;
	NOST[2] /= 5;

	err = lsm6dsl_set_selftest(cdata,
			LSM6DSL_SELF_TEST_GYRO_POS_SIGN_VAL, LSM6DSL_GYRO);
	if (err < 0)
		goto G_HW_SELF_EXIT;

	mdelay(60);

	retry = GYR_DA_RETRY_COUNT;
	do {
		usleep_range(1000, 1100);
		err = cdata->tf->read(cdata,
				LSM6DSL_SRC_ACCEL_GYRO_REG,
				1, &buf, true);
		if (err < 0)
			goto G_HW_SELF_EXIT;

		retry--;
		if (!retry)
			break;
	} while (!(buf & 0x02));

	err = cdata->tf->read(cdata,
			LSM6DSL_GYRO_OUT_X_L_ADDR,
			6, (u8 *)nOutData, true);
	if (err < 0)
		goto G_HW_SELF_EXIT;

	for (i = 0; i < 6; i++) {
		retry = GYR_DA_RETRY_COUNT;
		do {
			usleep_range(1000, 1100);
			err = cdata->tf->read(cdata,
					LSM6DSL_SRC_ACCEL_GYRO_REG,
					1, &buf, true);
			if (err < 0)
				goto G_HW_SELF_EXIT;

			retry--;
			if (!retry)
				break;
		} while (!(buf & 0x02));

		err = cdata->tf->read(cdata,
				LSM6DSL_GYRO_OUT_X_L_ADDR,
				6, (u8 *)nOutData, true);
		if (err < 0)
			goto G_HW_SELF_EXIT;

		if (i > 0) {
			ST[0] += nOutData[0];
			ST[1] += nOutData[1];
			ST[2] += nOutData[2];
		}
	}

	ST[0] /= 5;
	ST[1] /= 5;
	ST[2] /= 5;

	buf = 0x00;
	err = cdata->tf->write(cdata,
			LSM6DSL_CTRL2_ADDR,
			1, &buf, true);

	if (err < 0)
		goto G_HW_SELF_EXIT;

	err = lsm6dsl_set_selftest(cdata,
			LSM6DSL_SELF_TEST_DISABLED_VAL, LSM6DSL_GYRO);
	if (err < 0)
		goto G_HW_SELF_EXIT;

	retry = 0;
	for (i = 0; i < 3; i++) {
		DIFF_ST[i] = ABS(ST[i] - NOST[i]);
		if ((LSM6DSL_GYR_MIN_ST > DIFF_ST[i])
			|| (LSM6DSL_GYR_MAX_ST < DIFF_ST[i])) {
			retry++;
		}
	}

	if (retry > 0)
		return 0;
	else
		return 1;

G_HW_SELF_EXIT:
	return err;
}


static int lsm6dsl_selftest_run(struct lsm6dsl_data *cdata,
			char *out_str, u8 sindex)
{
	int err;
	u8 zero[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	u8 backup_regs1[5] = {0,}, backup_regs2[10] = {0,};
	u8 backup_regs3[2] = {0,}, backup_regs4[2] = {0,};
	u8 init_status = 0, fifo_pass = 0;
	u8 self_test_ret = FAIL, self_test_zro_ret = FAIL;
	s16 zero_rate_data[3] = {0,}, slot_raw[3] = {0,};
	s32 NOST[3] = {0,}, ST[3] = {0,}, N_ST[3] = {0,};
	s32 DIFF_ST[3] = {0,}, N_DIFF_ST[3] = {0,};
	s32 fifo_count = 0, hw_st_ret = 0;
	int i = 0;

	SENSOR_INFO("start\n");

	/* check ID */
	err = cdata->tf->read(cdata,
			LSM6DSL_WHO_AM_I,
			1, &init_status, true);
	if (err < 0) {
		SENSOR_ERR("failed to read Who-Am-I register\n");
		return err;
	}

	if (init_status != LSM6DSL_WHO_AM_I_DEF) {
		SENSOR_ERR("Who-Am-I register is wrong %x\n", init_status);
		init_status = 0;
	} else {
		init_status = 1;
	}

	/* backup registers */
	err = cdata->tf->read(cdata,
			LSM6DSL_INT1_ADDR,
			2, backup_regs4, true);
	if (err < 0) {
		SENSOR_ERR("failed to read int registers\n");
		goto restore_exit;
	}

	err = cdata->tf->read(cdata,
			LSM6DSL_MD1_ADDR,
			2, backup_regs3, true);
	if (err < 0) {
		SENSOR_ERR("failed to read md registers\n");
		goto restore_int;
	}

	err = cdata->tf->read(cdata,
			LSM6DSL_CTRL1_ADDR,
			10, backup_regs2, true);
	if (err < 0) {
		SENSOR_ERR("failed to read ctrl registers\n");
		goto restore_md;
	}

	err = cdata->tf->read(cdata,
			LSM6DSL_FIFO_CTRL1_ADDR,
			5, backup_regs1, true);
	if (err < 0) {
		SENSOR_ERR("failed to read fifo registers\n");
		goto restore_ctrl;
	}

	/* write 0 to registers */
	err = cdata->tf->write(cdata,
			LSM6DSL_MD1_ADDR,
			2, zero, true);
	if (err < 0) {
		SENSOR_ERR("failed to write 0 into MD registers\n");
		goto restore_regs;
	}

	err = cdata->tf->write(cdata,
			LSM6DSL_INT1_ADDR,
			2, zero, true);
	if (err < 0) {
		SENSOR_ERR("failed to write 0 into INT registers\n");
		goto restore_regs;
	}

	err = cdata->tf->write(cdata,
			LSM6DSL_FIFO_CTRL1_ADDR,
			5, zero, true);
	if (err < 0) {
		SENSOR_ERR("failed to write 0 into fifo registers\n");
		goto restore_regs;
	}

	zero[2] = 0x04;
	zero[8] = 0x38;
	zero[9] = 0x38;
	err = cdata->tf->write(cdata,
			LSM6DSL_CTRL1_ADDR,
			10, zero, true);
	if (err < 0) {
		SENSOR_ERR("failed to write 0 into CTRL registers\n");
		goto restore_regs;
	}

	/* hw selftest */
	switch (sindex) {
	case LSM6DSL_ACCEL:
		hw_st_ret = lsm6dsl_acc_hw_selftest(cdata, NOST,
						ST, N_ST, DIFF_ST, N_DIFF_ST);

		for(i = 0; i < 3; i++) {
			DIFF_ST[i] = LSM6DSL_ACC_LSB_TO_MG(DIFF_ST[i]);
			N_DIFF_ST[i] = LSM6DSL_ACC_LSB_TO_MG(N_DIFF_ST[i]);
		}

		if (hw_st_ret > 0) {
			SENSOR_INFO("ST = 1 %d %d %d %d %d %d\n",
				DIFF_ST[0], DIFF_ST[1], DIFF_ST[2],
				N_DIFF_ST[0], N_DIFF_ST[1], N_DIFF_ST[2]);
		} else {
			SENSOR_INFO("ST = 0 %d %d %d %d %d %d\n",
				DIFF_ST[0], DIFF_ST[1], DIFF_ST[2],
				N_DIFF_ST[0], N_DIFF_ST[1], N_DIFF_ST[2]);
			goto restore_regs;
		}
		break;

	case LSM6DSL_GYRO:
		err = lsm6dsl_gyro_fifo_test(cdata, zero_rate_data,
			&fifo_count, &fifo_pass, slot_raw, out_str);
		if (fifo_pass) {
			usleep_range(10000, 11000);
			err = lsm6dsl_gyro_fifo_test(cdata, zero_rate_data,
				&fifo_count, &fifo_pass, slot_raw, out_str);
		}
		if (err < 0) {
			SENSOR_ERR("gyro fifo test fail = %d, fail raw = %d, %d, %d\n",
				fifo_count, slot_raw[0],
				slot_raw[1], slot_raw[2]);
			goto restore_regs;
		}

		hw_st_ret = lsm6dsl_gyro_hw_selftest(cdata, NOST, ST, DIFF_ST);

		if((LSM6DSL_GYR_MIN_ZRL <= NOST[0]) && (NOST[0] <= LSM6DSL_GYR_MAX_ZRL)
		&& (LSM6DSL_GYR_MIN_ZRL <= NOST[1]) && (NOST[1] <= LSM6DSL_GYR_MAX_ZRL)
		&& (LSM6DSL_GYR_MIN_ZRL <= NOST[2]) && (NOST[2] <= LSM6DSL_GYR_MAX_ZRL))
			self_test_zro_ret = PASS;

		for(i = 0; i < 3; i++) {
			zero_rate_data[i] = LSM6DSL_GYR_LSB_TO_DPS(zero_rate_data[i]);
			NOST[i] = LSM6DSL_GYR_LSB_TO_DPS(NOST[i]);
			ST[i] = LSM6DSL_GYR_LSB_TO_DPS(ST[i]);
			DIFF_ST[i] = LSM6DSL_GYR_LSB_TO_DPS(DIFF_ST[i]);
		}

		if (hw_st_ret > 0) {
			self_test_ret = PASS;
			SENSOR_INFO("gyro selftest pass\n");
		} else {
			self_test_ret = FAIL;
			SENSOR_INFO("gyro selftest fail\n");
			goto restore_regs;
		}

		break;
	}

restore_regs:
	err = cdata->tf->write(cdata, LSM6DSL_FIFO_CTRL1_ADDR, 5, backup_regs1,
					true);
	if (err < 0)
		SENSOR_ERR("failed to write fifo registers\n");
restore_ctrl:
	err = cdata->tf->write(cdata, LSM6DSL_CTRL1_ADDR, 10, backup_regs2,
					true);
	if (err < 0)
		SENSOR_ERR("failed to write ctrl registers\n");
restore_md:
	err = cdata->tf->write(cdata, LSM6DSL_MD1_ADDR, 2, backup_regs3, true);
	if (err < 0)
		SENSOR_ERR("failed to write md registers\n");
restore_int:
	err = cdata->tf->write(cdata, LSM6DSL_INT1_ADDR, 2, backup_regs4, true);
	if (err < 0)
		SENSOR_ERR("failed to write int registers\n");
restore_exit:
	switch (sindex) {
	case LSM6DSL_ACCEL:
		if (hw_st_ret > 0) {
			return snprintf(out_str, PAGE_SIZE, "1,%d,%d,%d,%d,%d,%d\n",
					DIFF_ST[0], DIFF_ST[1], DIFF_ST[2],
					N_DIFF_ST[0], N_DIFF_ST[1], N_DIFF_ST[2]);
		} else {
			return snprintf(out_str, PAGE_SIZE, "0,%d,%d,%d,%d,%d,%d\n",
					DIFF_ST[0], DIFF_ST[1], DIFF_ST[2],
					N_DIFF_ST[0], N_DIFF_ST[1], N_DIFF_ST[2]);
		}
		break;

	case LSM6DSL_GYRO:
		if (!fifo_pass) {
			return snprintf(out_str, PAGE_SIZE, "%d,%d,%d\n",
				slot_raw[0], slot_raw[1], slot_raw[2]);
		} else {
			SENSOR_INFO("zero_rate_data = %d, %d, %d, st = %d, %d, %d, "\
				"nost = %d, %d, %d, diff_st = %d, %d, %d, "\
				"self_test_ret = %d, self_test_zro_ret = %d\n",
				zero_rate_data[0], zero_rate_data[1],
				zero_rate_data[2], ST[0], ST[1], ST[2],
				NOST[0], NOST[1], NOST[2],
				DIFF_ST[0], DIFF_ST[1], DIFF_ST[2],
				self_test_ret, self_test_zro_ret);

			return snprintf(out_str, PAGE_SIZE,
				"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
				zero_rate_data[0], zero_rate_data[1],
				zero_rate_data[2], NOST[0], NOST[1], NOST[2],
				ST[0], ST[1], ST[2], DIFF_ST[0], DIFF_ST[1],
				DIFF_ST[2], self_test_ret, self_test_zro_ret);
		}
		break;
	}

	return 0;
}


static ssize_t lsm6dsl_acc_selftest_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct lsm6dsl_data *cdata = dev_get_drvdata(dev);
	ssize_t ret;

	mutex_lock(&cdata->mutex_enable);
	ret = lsm6dsl_selftest_run(cdata, buf, LSM6DSL_ACCEL);
	mutex_unlock(&cdata->mutex_enable);

	return ret;
}

static ssize_t lsm6dsl_gyro_selftest_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct lsm6dsl_data *cdata = dev_get_drvdata(dev);
	ssize_t ret;

	mutex_lock(&cdata->mutex_enable);
	ret = lsm6dsl_selftest_run(cdata, buf, LSM6DSL_GYRO);
	mutex_unlock(&cdata->mutex_enable);

	return ret;
}

static ssize_t lsm6dsl_write_register_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int reg, val;
	int ret;
	struct lsm6dsl_data *cdata = dev_get_drvdata(dev);

	if (sscanf(buf, "%2x,%2x", &reg, &val) != 2) {
		SENSOR_ERR("invalid value\n");
		return count;
	}

	ret = lsm6dsl_write_data_with_mask(cdata, reg, 0xff, val, true);

	if (ret < 0)
		SENSOR_ERR("failed %d\n", ret);
	else
		SENSOR_INFO("Register(0x%x) data(0x%x)\n", reg, val);

	return count;
}

static ssize_t lsm6dsl_read_register_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct lsm6dsl_data *cdata = dev_get_drvdata(dev);

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

static DEVICE_ATTR(acc_register, 0644,
	lsm6dsl_read_register_show, lsm6dsl_write_register_store);
static DEVICE_ATTR(vendor, 0444, lsm6dsl_vendor_show, NULL);
static DEVICE_ATTR(name, 0444, lsm6dsl_name_show, NULL);
static DEVICE_ATTR(selftest_revised, 0444, selftest_revised_show, NULL);
static DEVICE_ATTR(dhr_sensor_info, 0444,
				lsm6dsl_accel_dhr_sensor_info_show, NULL);
static DEVICE_ATTR(temperature, 0444, lsm6dsl_temperature_show, NULL);
static DEVICE_ATTR(calibration, 0644,
	lsm6dsl_acc_calibration_show, lsm6dsl_acc_calibration_store);
static DEVICE_ATTR(lowpassfilter, 0644,
	lsm6dsl_lowpassfilter_show, lsm6dsl_lowpassfilter_store);
static DEVICE_ATTR(reactive_alert, 0644,
				lsm6dsl_smart_alert_show,
				lsm6dsl_smart_alert_store);

static struct device_attribute dev_attr_acc_raw_data =
	__ATTR(raw_data, 0444, lsm6dsl_acc_raw_data_show, NULL);
static struct device_attribute dev_attr_acc_self_test =
	__ATTR(selftest, 0444,
	lsm6dsl_acc_selftest_show,
	NULL);

static struct device_attribute dev_attr_gyro_raw_data =
	__ATTR(raw_data, 0444, lsm6dsl_gyro_raw_data_show, NULL);
static struct device_attribute dev_attr_gyro_self_test =
	__ATTR(selftest, 0444,
	lsm6dsl_gyro_selftest_show,
	NULL);

static struct device_attribute *acc_sensor_attrs[] = {
	&dev_attr_vendor,
	&dev_attr_name,
	&dev_attr_dhr_sensor_info,
	&dev_attr_calibration,
	&dev_attr_lowpassfilter,
	&dev_attr_reactive_alert,
	&dev_attr_acc_raw_data,
	&dev_attr_acc_self_test,
	&dev_attr_acc_register,
	NULL
};

static struct device_attribute *gyro_sensor_attrs[] = {
	&dev_attr_vendor,
	&dev_attr_name,
	&dev_attr_selftest_revised,	
	&dev_attr_temperature,
	&dev_attr_gyro_raw_data,
	&dev_attr_gyro_self_test,
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
static int lsm6dsl_acc_input_init(struct lsm6dsl_data *cdata)
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
				&lsm6dsl_accel_attribute_group);
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

static int lsm6dsl_gyro_input_init(struct lsm6dsl_data *cdata)
{
	int ret = 0;
	struct input_dev *dev;

	dev = input_allocate_device();
	if (!dev)
		return -ENOMEM;

	dev->name = MODULE_NAME_GYRO;
	dev->id.bustype = BUS_I2C;

	input_set_capability(dev, EV_REL, REL_RX);
	input_set_capability(dev, EV_REL, REL_RY);
	input_set_capability(dev, EV_REL, REL_RZ);
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
				&lsm6dsl_gyro_attribute_group);
	if (ret < 0)
		goto err_create_sysfs_group;

	cdata->gyro_input = dev;

	return 0;

err_create_sysfs_group:
	sensors_remove_symlink(&dev->dev.kobj, dev->name);
err_create_sensor_symlink:
	input_unregister_device(dev);
err_register_input_dev:
	return ret;
}

static int lsm6dsl_smd_input_init(struct lsm6dsl_data *cdata)
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
				&lsm6dsl_smd_attribute_group);
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

static int lsm6dsl_tilt_input_init(struct lsm6dsl_data *cdata)
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
				&lsm6dsl_tilt_attribute_group);
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

static int lsm6dsl_step_counter_input_init(struct lsm6dsl_data *cdata)
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
				&lsm6dsl_sc_attribute_group);
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

static int lsm6dsl_step_detector_input_init(struct lsm6dsl_data *cdata)
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
				&lsm6dsl_sd_attribute_group);
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

static int lsm6dsl_reset_steps(struct lsm6dsl_data *cdata)
{
	int err;
	u8 reg_value = 0x00;

	err = cdata->tf->read(cdata, LSM6DSL_STEP_COUNTER_RES_ADDR, 1,
			      &reg_value, true);
	if (err < 0)
		return err;

	/* Check if embedded functionalities are enabled */
	if (reg_value & LSM6DSL_FUNC_EN_MASK)
		reg_value = LSM6DSL_STEP_COUNTER_RES_FUNC_EN;
	else
		reg_value = LSM6DSL_DIS_BIT;

	err = lsm6dsl_write_data_with_mask(cdata,
				LSM6DSL_STEP_COUNTER_RES_ADDR,
				LSM6DSL_STEP_COUNTER_RES_MASK,
				LSM6DSL_STEP_COUNTER_RES_ALL_EN, true);
	if (err < 0)
		return err;

	err = lsm6dsl_write_data_with_mask(cdata,
				LSM6DSL_STEP_COUNTER_RES_ADDR,
				LSM6DSL_STEP_COUNTER_RES_MASK,
				reg_value, true);
	if (err < 0)
		return err;

	cdata->reset_steps = true;

	return 0;
}

static int lsm6dsl_reset_timestamp(struct lsm6dsl_data *cdata)
{
	int err;
	u8 reg_value = LSM6DSL_TIMESTAMP_RESET_VAL;

	err = cdata->tf->write(cdata, LSM6DSL_TIMESTAMP2_ADDR, 1,
			      &reg_value, true);
	if (err < 0) {
		SENSOR_ERR("fail to reset timestamp\n");
		return err;
	}

	return 0;
}


static int lsm6dsl_init_sensors(struct lsm6dsl_data *cdata)
{
	int err;
	u8 default_reg_value = 0;

	SENSOR_INFO(" Start!\n");

	cdata->acc_fs = LSM6DSL_ACCEL_FS_4G_VAL;
	cdata->gyro_fs = LSM6DSL_GYRO_FS_1000_VAL;
	cdata->acc_odr = LSM6DSL_ACCEL_ODR_104HZ_VAL;
	cdata->gyro_odr = LSM6DSL_ACCEL_ODR_104HZ_VAL;
	cdata->steps_c = 0;
	cdata->last_steps_c = 0;
	cdata->reset_steps = false;

	/* Software reset */
	err = lsm6dsl_write_data_with_mask(cdata, LSM6DSL_RESET_ADDR,
					   LSM6DSL_RESET_MASK, LSM6DSL_EN_BIT,
					   true);
	if (err < 0)
		return err;

	/* Enable Latch Mode Bit */
	err = lsm6dsl_write_data_with_mask(cdata, LSM6DSL_LIR_ADDR,
					   LSM6DSL_LIR_MASK, LSM6DSL_EN_BIT,
					   true);
	if (err < 0)
		return err;

	/* Enable timestamp count */
	err = lsm6dsl_write_data_with_mask(cdata, LSM6DSL_TIMER_EN_ADDR,
					   LSM6DSL_TIMER_EN_MASK,
					   LSM6DSL_EN_BIT, true);
		if (err < 0)
			return err;

	/* Output data not updated until have been read */
	err = lsm6dsl_write_data_with_mask(cdata, LSM6DSL_BDU_ADDR,
					   LSM6DSL_BDU_MASK, LSM6DSL_EN_BIT,
					   true);
	if (err < 0)
		return err;

	/* Enable Source Rounding function */
	err = lsm6dsl_write_data_with_mask(cdata, LSM6DSL_ROUNDING_ADDR,
					   LSM6DSL_ROUNDING_MASK,
					   LSM6DSL_EN_BIT, true);
	if (err < 0)
		return err;

	/* Set all interrupt signals in logic or on INT1 pad */
	err = lsm6dsl_write_data_with_mask(cdata,
					   LSM6DSL_INT2_ON_INT1_ADDR,
					   LSM6DSL_INT2_ON_INT1_MASK,
					   LSM6DSL_EN_BIT, true);
	if (err < 0)
		return err;


	err = lsm6dsl_reset_steps(cdata);
	if (err < 0)
		return err;

	lsm6dsl_set_fs(cdata, LSM6DSL_ACCEL, cdata->acc_fs);
	lsm6dsl_set_fs(cdata, LSM6DSL_GYRO, cdata->gyro_fs);

	mutex_lock(&cdata->bank_registers_lock);
	err = lsm6dsl_write_data_with_mask(cdata,
					   LSM6DSL_FUNC_CFG_ACCESS_ADDR,
					   LSM6DSL_FUNC_CFG_REG_MASK,
					   LSM6DSL_EN_BIT, false);
	if (err < 0)
		goto lsm6dsl_init_sensor_mutex_unlock;

	/* 4G, 14*32mg=448mg, threshold could adjust 0x8E~0x90 */
	default_reg_value = 0x8e;
	err = cdata->tf->write(cdata,
				      LSM6DSL_STEP_COUNTER_THS_MIN_ADDR,
				      1, &default_reg_value, false);
	if (err < 0)
		goto lsm6dsl_init_sensor_mutex_unlock;

	/* DEB_TIME = 0x0B(11*80ms=880ms), DEB_STEP = 0x07(7Step) */
	default_reg_value = 0x5f;
	err = cdata->tf->write(cdata,
				      LSM6DSL_STEP_COUNTER_DEB_ADDR,
				      1, &default_reg_value, false);
	if (err < 0)
		goto lsm6dsl_init_sensor_mutex_unlock;

	default_reg_value = 0x00;
	err = cdata->tf->write(cdata,
				      LSM6DSL_STEP_COUNTER_DURATION_ADDR,
				      1, &default_reg_value, false);
	if (err < 0)
		goto lsm6dsl_init_sensor_mutex_unlock;

	err = lsm6dsl_write_data_with_mask(cdata,
					   LSM6DSL_FUNC_CFG_ACCESS_ADDR,
					   LSM6DSL_FUNC_CFG_REG_MASK,
					   LSM6DSL_DIS_BIT, false);
	if (err < 0)
		goto lsm6dsl_init_sensor_mutex_unlock;

	mutex_unlock(&cdata->bank_registers_lock);

	return 0;

lsm6dsl_init_sensor_mutex_unlock:
	mutex_unlock(&cdata->bank_registers_lock);

	return err;
}

static int lsm6dsl_vdd_onoff(struct lsm6dsl_data *cdata, int onoff)
{
	/* ldo control */
	if (cdata->lsm6dsl_ldo_pin) {
		gpio_set_value(cdata->lsm6dsl_ldo_pin, onoff);
		if (onoff)
			msleep(20);
		return 0;
	}

	return 0;
}

static int lsm6dsl_parse_dt(struct lsm6dsl_data *cdata)
{
	struct device_node *np;
	enum of_gpio_flags flags;
	u32 orientation[9], i = 0;
	int ret = 0;

	np = cdata->dev->of_node;
	if (!np)
		return -EINVAL;

	cdata->lsm6dsl_ldo_pin = of_get_named_gpio_flags(np,
				"st,vdd_ldo_pin", 0, &flags);
	if (cdata->lsm6dsl_ldo_pin < 0) {
		SENSOR_INFO("Cannot set vdd_ldo_pin through DTSI\n\n");
		cdata->lsm6dsl_ldo_pin = 0;
	} else {
		ret = gpio_request(cdata->lsm6dsl_ldo_pin, "st,vdd_ldo_pin");
		if (ret < 0)
			SENSOR_ERR("gpio %d request failed %d\n",
				cdata->lsm6dsl_ldo_pin, ret);
		else
			gpio_direction_output(cdata->lsm6dsl_ldo_pin, 0);
	}

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


int lsm6dsl_common_probe(struct lsm6dsl_data *cdata, int irq, u16 bustype)
{
	int32_t err;
	u8 wai = 0x00;
	int retry = 5;

	SENSOR_INFO(" Start!\n");

	dev_set_drvdata(cdata->dev, cdata);

	mutex_init(&cdata->bank_registers_lock);
	mutex_init(&cdata->tb.buf_lock);
	mutex_init(&cdata->mutex_enable);
	mutex_init(&cdata->mutex_read);

	err = lsm6dsl_parse_dt(cdata);
	if (err < 0)
		goto exit;

	lsm6dsl_vdd_onoff(cdata, ON);

	/* Read Chip ID register */
	while (retry--) {
		err = cdata->tf->read(cdata, LSM6DSL_WHO_AM_I, 1, &wai, true);
		if (err < 0)
			SENSOR_ERR("failed to read Who-Am-I register. err=%d\n", err);
		if (wai != LSM6DSL_WHO_AM_I_DEF)
			SENSOR_ERR("Who-Am-I value not valid. wai=%d, err=%d\n", wai, err);
		else
			break;
	}

	if (retry < 0)
		goto exit_err_chip_id_or_i2c_error;

	/* input device init */
	err = lsm6dsl_acc_input_init(cdata);
	if (err < 0)
		goto exit_acc_input_init;

	err = lsm6dsl_gyro_input_init(cdata);
	if (err < 0)
		goto exit_gyro_input_init;

	err = lsm6dsl_smd_input_init(cdata);
	if (err < 0)
		goto exit_smd_input_init;

	err = lsm6dsl_tilt_input_init(cdata);
	if (err < 0)
		goto exit_tilt_input_init;

	err = lsm6dsl_step_counter_input_init(cdata);
	if (err < 0)
		goto exit_sc_input_init;

	err = lsm6dsl_step_detector_input_init(cdata);
	if (err < 0)
		goto exit_sd_input_init;

	err = lsm6dsl_init_sensors(cdata);
	if (err < 0)
		goto exit_init_sensor;

	/* factory test sysfs node */
	err = sensors_register(&cdata->acc_factory_dev, cdata,
		acc_sensor_attrs, MODULE_NAME_ACC);
	if (err < 0) {
		SENSOR_ERR("failed to sensors_register = %d\n", err);
		goto exit_acc_sensor_register_failed;
	}

	err = sensors_register(&cdata->gyro_factory_dev, cdata,
		gyro_sensor_attrs, MODULE_NAME_GYRO);
	if (err < 0) {
		SENSOR_ERR("failed to sensors_register = %d\n", err);
		goto exit_gyro_sensor_register_failed;
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
	cdata->acc_delay = ns_to_ktime(LSM6DSL_DELAY_DEFAULT);
	cdata->acc_timer.function = lsm6dsl_acc_timer_func;

	hrtimer_init(&cdata->gyro_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	cdata->gyro_delay = ns_to_ktime(LSM6DSL_DELAY_DEFAULT);
	cdata->gyro_timer.function = lsm6dsl_gyro_timer_func;

	/* the timer just fires off a work queue request. */
	/* we need a thread to read the i2c (can be slow and blocking). */
	cdata->accel_wq = create_singlethread_workqueue("accel_wq");
	if (!cdata->accel_wq) {
		err = -ENOMEM;
		SENSOR_ERR("could not create accel workqueue\n");
		goto exit_create_workqueue_acc;
	}

	cdata->gyro_wq = create_singlethread_workqueue("gyro_wq");
	if (!cdata->gyro_wq) {
		err = -ENOMEM;
		SENSOR_ERR("could not create gyro workqueue\n");
		goto exit_create_workqueue_gyro;
	}

	/* this is the thread function we run on the work queue */
	INIT_WORK(&cdata->acc_work, lsm6dsl_acc_work_func);
	INIT_WORK(&cdata->gyro_work, lsm6dsl_gyro_work_func);

	atomic_set(&cdata->acc_wkqueue_en, 0);
	atomic_set(&cdata->gyro_wkqueue_en, 0);

	cdata->irq_wq = create_workqueue(cdata->name);
	if (!cdata->irq_wq) {
		err = -ENOMEM;
		SENSOR_ERR("could not create irq workqueue\n");
		goto exit_create_workqueue_irq;
	}

	if (cdata->irq > 0) {
		wake_lock_init(&cdata->sa_wake_lock, WAKE_LOCK_SUSPEND,
		       LSM6DSL_DEV_NAME "_sa_wake_lock");

		INIT_WORK(&cdata->data_work, lsm6dsl_irq_management);
		INIT_DELAYED_WORK(&cdata->sa_irq_work, lsm6dsl_sa_irq_work);

		err = request_threaded_irq(irq, lsm6dsl_threaded, NULL,
				IRQF_TRIGGER_RISING, cdata->name, cdata);
		disable_irq(cdata->irq);

		SENSOR_INFO("Smart alert init, irq = %d\n", cdata->irq);
	}

	SENSOR_INFO(" Done!\n");
	return 0;

exit_create_workqueue_irq:
	destroy_workqueue(cdata->gyro_wq);
exit_create_workqueue_gyro:
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
	sensors_unregister(cdata->gyro_factory_dev, gyro_sensor_attrs);
exit_gyro_sensor_register_failed:
	sensors_unregister(cdata->acc_factory_dev, acc_sensor_attrs);
exit_acc_sensor_register_failed:
exit_init_sensor:
	sensors_remove_symlink(&cdata->sd_input->dev.kobj,
					cdata->sd_input->name);
	sysfs_remove_group(&cdata->sd_input->dev.kobj,
					&lsm6dsl_sd_attribute_group);
	input_unregister_device(cdata->sd_input);
exit_sd_input_init:
	sensors_remove_symlink(&cdata->sc_input->dev.kobj,
					cdata->sc_input->name);
	sysfs_remove_group(&cdata->sc_input->dev.kobj,
					&lsm6dsl_sc_attribute_group);
	input_unregister_device(cdata->sc_input);
exit_sc_input_init:
	sensors_remove_symlink(&cdata->tilt_input->dev.kobj,
					cdata->tilt_input->name);
	sysfs_remove_group(&cdata->tilt_input->dev.kobj,
					&lsm6dsl_tilt_attribute_group);
	input_unregister_device(cdata->tilt_input);
exit_tilt_input_init:
	sensors_remove_symlink(&cdata->smd_input->dev.kobj,
					cdata->smd_input->name);
	sysfs_remove_group(&cdata->smd_input->dev.kobj,
					&lsm6dsl_smd_attribute_group);
	input_unregister_device(cdata->smd_input);
exit_smd_input_init:
	sensors_remove_symlink(&cdata->gyro_input->dev.kobj,
					cdata->gyro_input->name);
	sysfs_remove_group(&cdata->gyro_input->dev.kobj,
					&lsm6dsl_gyro_attribute_group);
	input_unregister_device(cdata->gyro_input);
exit_gyro_input_init:
	sensors_remove_symlink(&cdata->acc_input->dev.kobj,
					cdata->acc_input->name);
	sysfs_remove_group(&cdata->acc_input->dev.kobj,
					&lsm6dsl_accel_attribute_group);
	input_unregister_device(cdata->acc_input);
exit_acc_input_init:
exit_err_chip_id_or_i2c_error:
	if (cdata->lsm6dsl_ldo_pin)
		gpio_free(cdata->lsm6dsl_ldo_pin);
exit:
	mutex_destroy(&cdata->bank_registers_lock);
	mutex_destroy(&cdata->tb.buf_lock);
	mutex_destroy(&cdata->mutex_enable);
	mutex_destroy(&cdata->mutex_read);
	return err;
}
EXPORT_SYMBOL(lsm6dsl_common_probe);

int lsm6dsl_common_suspend(struct lsm6dsl_data *cdata)
{
	SENSOR_INFO("\n");
	if (atomic_read(&cdata->acc_wkqueue_en) == 1)
		lsm6dsl_disable_sensors(cdata, LSM6DSL_ACCEL);
	if (atomic_read(&cdata->gyro_wkqueue_en) == 1)
		lsm6dsl_disable_sensors(cdata, LSM6DSL_GYRO);

	if (cdata->enabled & LSM6DSL_STEP_COUNTER_DEPENDENCY) {
		lsm6dsl_write_data_with_mask(cdata,
				LSM6DSL_INT1_ADDR,
				LSM6DSL_STEP_DETECTOR_DRDY_IRQ_MASK,
				LSM6DSL_DIS_BIT, true);
		lsm6dsl_enable_step_counter_fifo(cdata, true);
	}

	return 0;
}
EXPORT_SYMBOL(lsm6dsl_common_suspend);

int lsm6dsl_common_resume(struct lsm6dsl_data *cdata)
{
	SENSOR_INFO("\n");

	if (atomic_read(&cdata->acc_wkqueue_en) == 1)
		lsm6dsl_enable_sensors(cdata, LSM6DSL_ACCEL);
	if (atomic_read(&cdata->gyro_wkqueue_en) == 1)
		lsm6dsl_enable_sensors(cdata, LSM6DSL_GYRO);

	if (cdata->enabled & LSM6DSL_STEP_COUNTER_DEPENDENCY) {
		mutex_lock(&cdata->mutex_read);
		lsm6dsl_get_step_c_data_fifo(cdata);
		mutex_unlock(&cdata->mutex_read);
		lsm6dsl_enable_step_counter_fifo(cdata, false);
		lsm6dsl_write_data_with_mask(cdata,
				LSM6DSL_INT1_ADDR,
				LSM6DSL_STEP_DETECTOR_DRDY_IRQ_MASK,
				LSM6DSL_EN_BIT, true);
	}

	return 0;
}
EXPORT_SYMBOL(lsm6dsl_common_resume);

void lsm6dsl_common_remove(struct lsm6dsl_data *cdata)
{
	if (cdata->enabled & (1 << LSM6DSL_ACCEL))
		lsm6dsl_disable_sensors(cdata, LSM6DSL_ACCEL);
	if (cdata->enabled & (1 << LSM6DSL_GYRO))
		lsm6dsl_disable_sensors(cdata, LSM6DSL_GYRO);
}
EXPORT_SYMBOL(lsm6dsl_common_remove);

void lsm6dsl_common_shutdown(struct lsm6dsl_data *cdata)
{
	u8 i;

	for (i = 0; i < LSM6DSL_SENSORS_NUMB; i++)
		lsm6dsl_disable_sensors(cdata, i);

}
EXPORT_SYMBOL(lsm6dsl_common_shutdown);

MODULE_DESCRIPTION("STMicroelectronics lsm6dsl driver");
MODULE_AUTHOR("Giuseppe Barba");
MODULE_AUTHOR("Mario Tesi");
MODULE_LICENSE("GPL v2");
