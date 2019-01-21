/*
 * STMicroelectronics lsm6ds3 driver
 *
 * Copyright 2014 STMicroelectronics Inc.
 *
 * Denis Ciocca <denis.ciocca@st.com>
 * v. 1.1.2
 * Licensed under the GPL-2.
 */

#ifndef ST_LSM6DS3_H
#define ST_LSM6DS3_H

#include <linux/types.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/iio/trigger.h>
#include <linux/sensor/sensors_core.h>
#include <linux/wakelock.h>

#define LSM6DS3_DEV_NAME		"LSM6DS3"
#define VENDOR_NAME			"STM"
#define MODULE_NAME_ACC			"accelerometer_sensor"
#define MODULE_NAME_GYRO		"gyro_sensor"
#define MODULE_NAME_SMD                 "SignificantMotionDetector"
#define MODULE_NAME_TILT                "tilt_sensor"

#define CALIBRATION_FILE_PATH		"/efs/FactoryApp/accel_calibration_data"

#define CALIBRATION_DATA_AMOUNT		20
#define MAX_ACCEL_1G			8192

#define ST_INDIO_DEV_ACCEL		0
#define ST_INDIO_DEV_GYRO		1
#define ST_INDIO_DEV_SIGN_MOTION	2
#define ST_INDIO_DEV_STEP_COUNTER	3
#define ST_INDIO_DEV_STEP_DETECTOR	4
#define ST_INDIO_DEV_TILT		5
#define ST_INDIO_DEV_NUM		6

#define ST_LSM6DS3_ACCEL_DEPENDENCY	((1 << ST_INDIO_DEV_ACCEL) | \
					(1 << ST_INDIO_DEV_STEP_COUNTER) | \
					(1 << ST_INDIO_DEV_TILT) | \
					(1 << ST_INDIO_DEV_SIGN_MOTION) | \
					(1 << ST_INDIO_DEV_STEP_DETECTOR))

#define ST_LSM6DS3_STEP_COUNTER_DEPENDENCY \
					((1 << ST_INDIO_DEV_STEP_COUNTER) | \
					(1 << ST_INDIO_DEV_SIGN_MOTION) | \
					(1 << ST_INDIO_DEV_STEP_DETECTOR))

#define ST_LSM6DS3_EXTRA_DEPENDENCY	((1 << ST_INDIO_DEV_STEP_COUNTER) | \
					(1 << ST_INDIO_DEV_TILT) | \
					(1 << ST_INDIO_DEV_SIGN_MOTION) | \
					(1 << ST_INDIO_DEV_STEP_DETECTOR))

#define ST_LSM6DS3_USE_BUFFER		((1 << ST_INDIO_DEV_ACCEL) | \
					(1 << ST_INDIO_DEV_GYRO) | \
					(1 << ST_INDIO_DEV_STEP_COUNTER))

#define ST_LSM6DS3_TX_MAX_LENGTH	12
#define ST_LSM6DS3_RX_MAX_LENGTH	4097

#define ST_LSM6DS3_NUMBER_DATA_CHANNELS	3
#define ST_LSM6DS3_BYTE_FOR_CHANNEL	2

#define ST_LSM6DS3_DELAY_DEFAULT        200000000LL
#define ST_LSM6DS3_DELAY_MIN            5000000LL

struct st_lsm6ds3_transfer_buffer {
	struct mutex buf_lock;
	u8 rx_buf[ST_LSM6DS3_RX_MAX_LENGTH];
	u8 tx_buf[ST_LSM6DS3_TX_MAX_LENGTH] ____cacheline_aligned;
};

struct st_lsm6ds3_transfer_function {
	int (*write) (struct st_lsm6ds3_transfer_buffer *tb,
			struct device *dev, u8 reg_addr, int len, u8 *data);
	int (*read) (struct st_lsm6ds3_transfer_buffer *tb,
			struct device *dev, u8 reg_addr, int len, u8 *data);
};

struct lsm6ds3_steps {
	bool new_steps;
	u16 steps;

	struct hrtimer hr_timer;
	ktime_t ktime;

	s64 last_step_timestamp;
};

struct lsm6ds3_sensor_data {
	unsigned int acc_c_odr;
	unsigned int acc_c_gain;
	unsigned int gyr_c_odr;
	unsigned int gyr_c_gain;
	unsigned int bandwidth;
	int poll_delay;

	u8 sindex;
	u8 *buffer_data;

	int st_mode;
	int64_t old_timestamp;
};

struct lsm6ds3_v {
	s16 x;/**<accel X  data*/
	s16 y;/**<accel Y  data*/
	s16 z;/**<accel Z  data*/
};

struct lsm6ds3_data {

	struct input_dev *acc_input;
	atomic_t wkqueue_en;
	ktime_t delay;

	struct input_dev *gyro_input;
	atomic_t gyro_wkqueue_en;
	ktime_t gyro_delay;

	struct input_dev *smd_input;
	struct input_dev *tilt_input;

	int time_count;
	int gyro_time_count;

	const char *name;

	u8 drdy_int_pin;
	u8 sensors_enabled;
	struct mutex mutex_enable;

	u8 gyro_module;
	u16 gyro_selftest_delta[3];
	u8 gyro_selftest_status;
	u8 accel_selftest_status;

	bool patch_feature;
	bool sign_motion_event_ready;

	int err_count;

	int irq;
	int irq_gpio;
	s16 accel_data[3];
	s16 gyro_data[3];

	s16 accel_cal_data[3];

	s8 orientation[9];

	int64_t timestamp;

	struct work_struct acc_work;
	struct work_struct gyro_work;
	struct work_struct data_work;

	struct workqueue_struct *accel_wq;
	struct workqueue_struct *gyro_wq;
	struct workqueue_struct *irq_wq;

	struct hrtimer acc_timer;
	struct hrtimer gyro_timer;

	struct device *dev;
	struct device *acc_factory_dev;
	struct device *gyro_factory_dev;
	struct device *smd_factory_dev;
	struct device *tilt_factory_dev;
	struct lsm6ds3_steps step_c;

	const struct st_lsm6ds3_transfer_function *tf;
	struct st_lsm6ds3_transfer_buffer tb;

	struct delayed_work sa_irq_work;
	struct wake_lock sa_wake_lock;
	int sa_irq_state;
	int sa_flag;
	int sa_factory_flag;

	struct regulator *reg_vio;
	struct i2c_client *client;

	u8 odr;
	bool skip_gyro_data;
	int skip_gyro_cnt;
	int states;
	struct lsm6ds3_sensor_data *sdata;

	int lpf_on;
};

int st_lsm6ds3_common_probe(struct lsm6ds3_data *cdata, int irq);
void st_lsm6ds3_common_remove(struct lsm6ds3_data *cdata, int irq);
void st_lsm6ds3_common_shutdown(struct lsm6ds3_data *cdata);

int st_lsm6ds3_set_axis_enable(struct lsm6ds3_data *sdata, u8 value, int sindex);
void st_lsm6ds3_set_irq(struct lsm6ds3_data *cdata, bool enable);

int st_lsm6ds3_common_suspend(struct lsm6ds3_data *cdata);
int st_lsm6ds3_common_resume(struct lsm6ds3_data *cdata);

int st_lsm6ds3_acc_open_calibration(struct lsm6ds3_data *cdata);
int st_lsm6ds3_gyro_open_calibration(struct lsm6ds3_data *cdata);

#endif /* ST_LSM6DS3_H */
