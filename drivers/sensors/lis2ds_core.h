/*
 * STMicroelectronics LIS2DS driver
 *
 * Copyright 2015 STMicroelectronics Inc.
 *
 * Giuseppe Barba <giuseppe.barba@st.com>
 * v.1.1.0
 * Licensed under the GPL-2.
 */

#ifndef DRIVERS_INPUT_MISC_LIS2DS_CORE_H_
#define DRIVERS_INPUT_MISC_LIS2DS_CORE_H_

#include <linux/types.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/wakelock.h>
#include <linux/sensor/sensors_core.h>

#define LIS2DS_DEV_NAME			"LIS2DS"
#define VENDOR_NAME			"STM"
#define MODULE_NAME_ACC			"accelerometer_sensor"
#define MODULE_NAME_SMD                 "SignificantMotionDetector"
#define MODULE_NAME_TILT                "tilt_sensor"
#define MODULE_NAME_SC                 "step_cnt_sensor"
#define MODULE_NAME_SD                "step_det_sensor"

#define CALIBRATION_FILE_PATH		"/efs/FactoryApp/accel_calibration_data"
#define CALIBRATION_DATA_AMOUNT		20
#define MAX_ACCEL_1G			8192
#define SA_DYNAMIC_THRESHOLD		50 /* mg */

#define LIS2DS_I2C_ADDR			0x1e

#define LIS2DS_DELAY_DEFAULT        200000000LL
#define LIS2DS_DELAY_MIN            5000000LL

/* Selftest: 70~1500mg @ 2G */
#define LIS2DS_ACC_MIN_ST			70
#define LIS2DS_ACC_MAX_ST			1500
#define LIS2DS_ACC_LSB_TO_MG(LSB)		(LSB * 61 / 1000)

#define ACCEL_LOG_TIME                15 /* 15 sec */

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
	LIS2DS_ACCEL = 0,
	LIS2DS_STEP_C,
	LIS2DS_FF,
	LIS2DS_TAP,
	LIS2DS_DOUBLE_TAP,
	LIS2DS_STEP_D,
	LIS2DS_TILT,
	LIS2DS_SIGN_M,
	LIS2DS_WAKEUP,
	LIS2DS_ACTIVITY,
	LIS2DS_SENSORS_NUMB,
};

enum fifo_mode {
	BYPASS = 0,
	CONTINUOS,
};

#define DEF_ZERO			0x00

#define INPUT_EVENT_TYPE		EV_MSC
#define INPUT_EVENT_X			MSC_SERIAL
#define INPUT_EVENT_Y			MSC_PULSELED
#define INPUT_EVENT_Z			MSC_GESTURE
#define INPUT_EVENT_TIME_MSB		MSC_SCAN
#define INPUT_EVENT_TIME_LSB		MSC_MAX

#define LIS2DS_RX_MAX_LENGTH		500
#define LIS2DS_TX_MAX_LENGTH		500

#define to_dev(obj) container_of(obj, struct device, kobj)

struct reg_rw {
	u8 const address;
	u8 const init_val;
	u8 resume_val;
};

struct reg_r {
	const u8 address;
	const u8 init_val;
};

struct lis2ds_transfer_buffer {
	struct mutex buf_lock;
	u8 rx_buf[LIS2DS_RX_MAX_LENGTH];
	u8 tx_buf[LIS2DS_TX_MAX_LENGTH] ____cacheline_aligned;
};

struct lis2ds_data;

struct lis2ds_transfer_function {
	int (*write)(struct lis2ds_data *cdata, u8 reg_addr, int len, u8 *data,
		      bool b_lock);
	int (*read)(struct lis2ds_data *cdata, u8 reg_addr, int len, u8 *data,
		     bool b_lock);
};

struct lis2ds_sensor_data {
	const char *name;
	s64 timestamp;
	u8 enabled;
	u32 odr_val;
	u32 c_gain;
	u8 sindex;
};

struct lis2ds_data {
	struct input_dev *acc_input;
	struct input_dev *smd_input;
	struct input_dev *tilt_input;
	struct input_dev *sc_input;
	struct input_dev *sd_input;

	atomic_t acc_wkqueue_en;
	ktime_t acc_delay;

	struct work_struct acc_work;
	struct work_struct data_work;

	struct workqueue_struct *accel_wq;
	struct workqueue_struct *irq_wq;

	struct hrtimer acc_timer;
	int acc_time_count;

	struct device *acc_factory_dev;
	struct device *smd_factory_dev;
	struct device *tilt_factory_dev;
	struct device *sc_factory_dev;
	struct device *sd_factory_dev;

	struct delayed_work sa_irq_work;
	struct wake_lock sa_wake_lock;
	int sa_irq_state;
	int sa_flag;
	int sa_factory_flag;
	int states;

	u64 steps_c;
	u64 last_steps_c;

	s16 accel_data[3];
	s16 accel_cal_data[3];

	const char *name;
	u8 drdy_int_pin;
	u8 selftest_status;
	u8 power_mode;

	struct mutex lock;
	int irq;
	s64 timestamp;

	int irq_gpio;

	int lpf_on;

	s8 orientation[9];

	struct device *dev;
	struct lis2ds_sensor_data sensors[LIS2DS_SENSORS_NUMB];
	struct mutex bank_registers_lock;
	const struct lis2ds_transfer_function *tf;
	struct lis2ds_transfer_buffer tb;

	struct mutex mutex_read;
	struct mutex mutex_enable;

	struct regulator *reg_vdd;
};

int lis2ds_common_probe(struct lis2ds_data *cdata, int irq, u16 bustype);
void lis2ds_common_remove(struct lis2ds_data *cdata, int irq);
void lis2ds_common_shutdown(struct lis2ds_data *cdata);

#ifdef CONFIG_PM
int lis2ds_common_suspend(struct lis2ds_data *cdata);
int lis2ds_common_resume(struct lis2ds_data *cdata);
#endif /* CONFIG_PM */

#endif /* DRIVERS_INPUT_MISC_LIS2DS_CORE_H_ */
