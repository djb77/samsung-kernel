/*
 * STMicroelectronics lsm6dsl driver
 *
 * Copyright 2016 STMicroelectronics Inc.
 *
 * Giuseppe Barba <giuseppe.barba@st.com>
 * Mario Tesi <giuseppe.barba@st.com>
 * v 1.2.2
 * Licensed under the GPL-2.
 */

#ifndef DRIVERS_INPUT_MISC_LSM6DSL_CORE_H_
#define DRIVERS_INPUT_MISC_LSM6DSL_CORE_H_

#include <linux/types.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/wakelock.h>
#include <linux/sensor/sensors_core.h>

#define LSM6DSL_DEV_NAME		"LSM6DSL"
#define VENDOR_NAME				"STM"
#define MODULE_NAME_ACC			"accelerometer_sensor"
#define MODULE_NAME_GYRO		"gyro_sensor"
#define MODULE_NAME_SMD			"SignificantMotionDetector"
#define MODULE_NAME_TILT		"tilt_sensor"
#define MODULE_NAME_SC			"step_cnt_sensor"
#define MODULE_NAME_SD			"step_det_sensor"

#define CALIBRATION_FILE_PATH		"/efs/FactoryApp/accel_calibration_data"
#define CALIBRATION_DATA_AMOUNT		20
#define MAX_ACCEL_1G			8192
#define SA_DYNAMIC_THRESHOLD		50 /* mg */

#define LSM6DSL_FIFO_TEST_DEPTH		32

/* Selftest: 90~1700mg @ 2G */
#define LSM6DSL_ACC_MIN_ST              ((int)(90/0.061f))
#define LSM6DSL_ACC_MAX_ST              ((int)(1700/0.061f) + 1)
#define LSM6DSL_ACC_LSB_TO_MG(LSB)      (LSB * 61 / 1000)
#define ACC_DA_RETRY_COUNT              5

/* Selftest: 150~700dps @ 2000dps */
#define LSM6DSL_GYR_MIN_ST              ((int)(150/0.07f))
#define LSM6DSL_GYR_MAX_ST              ((int)(700/0.07f) + 1)
#define LSM6DSL_GYR_LSB_TO_DPS(LSB)     (LSB * 7 / 100)
#define GYR_DA_RETRY_COUNT              5

/* ZRL: 40dps @ 2000dps */
#define LSM6DSL_GYR_MIN_ZRL             ((int)(-40/0.07f) - 1)
#define LSM6DSL_GYR_MAX_ZRL             ((int)(40/0.07f) + 1)
#define LSM6DSL_GYR_ZRL_DELTA           ((int)(6/0.07f) + 1)

#define LSM6DSL_DELAY_DEFAULT           200000000LL
#define LSM6DSL_DELAY_MIN               5000000LL

#define ACCEL_LOG_TIME                  15 /* 15 sec */

#define HZ_TO_PERIOD_NSEC(hz)		(1000 * 1000 * 1000 / ((u32)(hz)))
#define MS_TO_US(x)			({ typeof(x) _x = (x); ((_x) * \
							((typeof(x)) 1000)); })
#define US_TO_NS(x)			(MS_TO_US(x))
#define MS_TO_NS(x)			(US_TO_NS(MS_TO_US(x)))
#define US_TO_MS(x)			({ typeof(x) _x = (x); ((_x) / \
							((typeof(x)) 1000)); })
#define NS_TO_US(x)			(US_TO_MS(x))
#define NS_TO_MS(x)			(US_TO_MS(NS_TO_US(x)))

enum {
	LSM6DSL_ACCEL = 0,
	LSM6DSL_GYRO,
	LSM6DSL_SIGN_MOTION,
	LSM6DSL_STEP_COUNTER,
	LSM6DSL_STEP_DETECTOR,
	LSM6DSL_TILT,
	LSM6DSL_SENSORS_NUMB,
};

enum {
	FAIL = 0,
	PASS = 1,
};

enum {
	OFF = 0,
	ON = 1,
};

#define SELFTEST_REVISED 1

#define LSM6DSL_ACCEL_DEPENDENCY	((1 << LSM6DSL_ACCEL) | \
					(1 << LSM6DSL_STEP_COUNTER) | \
					(1 << LSM6DSL_TILT) | \
					(1 << LSM6DSL_SIGN_MOTION) | \
					(1 << LSM6DSL_STEP_DETECTOR))

#define LSM6DSL_EXTRA_DEPENDENCY	((1 << LSM6DSL_STEP_COUNTER) | \
					(1 << LSM6DSL_TILT) | \
					(1 << LSM6DSL_SIGN_MOTION) | \
					(1 << LSM6DSL_STEP_DETECTOR))

#define LSM6DSL_STEP_SMD_DEPENDENCY	((1 << LSM6DSL_STEP_COUNTER) | \
					(1 << LSM6DSL_SIGN_MOTION) | \
					(1 << LSM6DSL_STEP_DETECTOR))

#define LSM6DSL_STEP_COUNTER_DEPENDENCY	((1 << LSM6DSL_STEP_COUNTER) | \
					(1 << LSM6DSL_STEP_DETECTOR))



#define DEF_ZERO			0x00

#define LSM6DSL_FIFO_CTRL1_ADDR		0x06
#define LSM6DSL_FIFO_CTRL2_ADDR		0x07
#define LSM6DSL_FIFO_CTRL3_ADDR		0x08
#define LSM6DSL_FIFO_CTRL4_ADDR		0x09
#define LSM6DSL_FIFO_CTRL5_ADDR		0x0a
#define LSM6DSL_FIFO_STAT1_ADDR		0x3a
#define LSM6DSL_FIFO_STAT2_ADDR		0x3b
#define LSM6DSL_FIFO_STAT3_ADDR		0x3c
#define LSM6DSL_FIFO_STAT4_ADDR		0x3d
#define LSM6DSL_FIFO_OUT_L_ADDR		0x3e

#define LSM6DSL_CTRL1_ADDR			0x10
#define LSM6DSL_CTRL2_ADDR			0x11
#define LSM6DSL_CTRL3_ADDR			0x12
#define LSM6DSL_CTRL4_ADDR			0x13
#define LSM6DSL_CTRL5_ADDR			0x14
#define LSM6DSL_CTRL6_ADDR			0x15
#define LSM6DSL_CTRL7_ADDR			0x16
#define LSM6DSL_CTRL8_ADDR			0x17
#define LSM6DSL_CTRL9_ADDR			0x18
#define LSM6DSL_CTRL10_ADDR			0x19

/* Output data rate registers */
#define LSM6DSL_ACC_ODR_ADDR		LSM6DSL_CTRL1_ADDR
#define LSM6DSL_ACC_ODR_MASK		0xf0
#define LSM6DSL_GYR_ODR_ADDR		LSM6DSL_CTRL2_ADDR
#define LSM6DSL_GYR_ODR_MASK		0xf0

#define LSM6DSL_ACC_FS_ADDR		LSM6DSL_CTRL1_ADDR
#define LSM6DSL_GYR_FS_ADDR		LSM6DSL_CTRL2_ADDR

#define LSM6DSL_IF_INC_MASK		0x04

#define LSM6DSL_HPERF_GYR_ADDR		LSM6DSL_CTRL7_ADDR
#define LSM6DSL_HPERF_GYR_MASK		0x80

#define LSM6DSL_HPERF_ACC_ADDR		LSM6DSL_CTRL6_ADDR
#define LSM6DSL_HPERF_ACC_MASK		0x10
#define LSM6DSL_HPERF_ACC_ENABLE	0x00

#define LSM6DSL_SELFTEST_ADDR		0x14
#define LSM6DSL_SELFTEST_ACCEL_MASK		0x03
#define LSM6DSL_SELFTEST_GYRO_MASK		0x0c
#define LSM6DSL_SELF_TEST_DISABLED_VAL	0x00
#define LSM6DSL_SELF_TEST_ACC_POS_SIGN_VAL	0x01
#define LSM6DSL_SELF_TEST_ACC_NEG_SIGN_VAL	0x02
#define LSM6DSL_SELF_TEST_GYRO_POS_SIGN_VAL	0x01
#define LSM6DSL_SELF_TEST_GYRO_NEG_SIGN_VAL	0x03


/* Sensitivity Acc */
#define SENSITIVITY_ACC_2G		61	/** ug/LSB */
#define SENSITIVITY_ACC_4G		122	/** ug/LSB */
#define SENSITIVITY_ACC_8G		244	/** ug/LSB */
#define SENSITIVITY_ACC_16G		488	/** ug/LSB */
/* Sensitivity Gyr */
#define SENSITIVITY_GYR_125		437	/** 10udps/LSB */
#define SENSITIVITY_GYR_245		875	/** 10udps/LSB */
#define SENSITIVITY_GYR_500		1750	/** 10udps/LSB */
#define SENSITIVITY_GYR_1000		3500	/** 10udps/LSB */
#define SENSITIVITY_GYR_2000		7000	/** 10udps/LSB */

/* Sensitivity TEMP */
#define SENSITIVITY_TEMP		256	/** 256LSB/degrees Celsius  */
#define TEMP_OUTPUT_ZERO_LEVEL		25	/** 25 degrees Celsius  */

/* Timestamp */
#define TIMESTAMP_TO_NS 6400000

#define LSM6DSL_RX_MAX_LENGTH		500
#define LSM6DSL_TX_MAX_LENGTH		500

#define to_dev(obj)			container_of(obj, struct device, kobj)

struct reg_rw {
	u8 const address;
	u8 const init_val;
	u8 resume_val;
};

struct reg_r {
	const u8 address;
	const u8 init_val;
};

struct lsm6dsl_transfer_buffer {
	struct mutex buf_lock;
	u8 rx_buf[LSM6DSL_RX_MAX_LENGTH];
	u8 tx_buf[LSM6DSL_TX_MAX_LENGTH] ____cacheline_aligned;
};

struct lsm6dsl_data;

struct lsm6dsl_transfer_function {
	int (*write)(struct lsm6dsl_data *cdata, u8 reg_addr, int len, u8 *data,
		     bool b_lock);
	int (*read)(struct lsm6dsl_data *cdata, u8 reg_addr, int len, u8 *data,
		    bool b_lock);
};

struct lsm6dsl_data {
	struct input_dev *acc_input;
	struct input_dev *gyro_input;
	struct input_dev *smd_input;
	struct input_dev *tilt_input;
	struct input_dev *sc_input;
	struct input_dev *sd_input;

	atomic_t acc_wkqueue_en;
	ktime_t acc_delay;

	atomic_t gyro_wkqueue_en;
	ktime_t gyro_delay;

	struct work_struct acc_work;
	struct work_struct gyro_work;
	struct work_struct data_work;

	struct workqueue_struct *accel_wq;
	struct workqueue_struct *gyro_wq;
	struct workqueue_struct *irq_wq;

	struct hrtimer acc_timer;
	struct hrtimer gyro_timer;

	int acc_time_count;
	int gyro_time_count;

	struct device *acc_factory_dev;
	struct device *gyro_factory_dev;
	struct device *smd_factory_dev;
	struct device *tilt_factory_dev;
	struct device *sc_factory_dev;
	struct device *sd_factory_dev;

	const char *name;
	bool reset_steps;
	bool sign_motion_event_ready;
	u64 steps_c;
	u64 last_steps_c;
	u64 step_timestamp;
	struct mutex mutex_read;

	s16 accel_data[3];
	s16 gyro_data[3];

	u8 acc_odr;
	u8 gyro_odr;
	u8 acc_fs;
	u8 gyro_fs;

	u8 sc_odr;

	s8 orientation[9];

	s16 accel_cal_data[3];

	struct delayed_work sa_irq_work;
	struct wake_lock sa_wake_lock;
	int sa_irq_state;
	int sa_flag;
	int sa_factory_flag;
	int states;

	int lpf_on;

	u8 enabled;

	struct mutex mutex_enable;

	int irq;
	int irq_gpio;
	int64_t timestamp;
	struct work_struct input_work;
	struct device *dev;
	struct mutex bank_registers_lock;
	const struct lsm6dsl_transfer_function *tf;
	struct lsm6dsl_transfer_buffer tb;

	int lsm6dsl_ldo_pin;
};

int lsm6dsl_common_probe(struct lsm6dsl_data *cdata, int irq, u16 bustype);
void lsm6dsl_common_remove(struct lsm6dsl_data *cdata);
void lsm6dsl_common_shutdown(struct lsm6dsl_data *cdata);

#ifdef CONFIG_PM
int lsm6dsl_common_suspend(struct lsm6dsl_data *cdata);
int lsm6dsl_common_resume(struct lsm6dsl_data *cdata);
#endif /* CONFIG_PM */

#endif /* DRIVERS_INPUT_MISC_LSM6DSL_CORE_H_ */
