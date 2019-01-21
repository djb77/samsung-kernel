/**
 * Header file of the core driver API @file yas.h
 *
 * Copyright (c) 2013-2015 Yamaha Corporation
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#ifndef __YAS_H__
#define __YAS_H__
#include <linux/types.h>

#define YAS_VERSION			"12.0.0" /*!< MS-x Driver Version */
#define DEV_NAME			"yas537"
#define VENDOR_NAME			"YAMAHA"
#define MODULE_NAME_MAG			"magnetic_sensor"

#define YAS_TYPE_ACC			(0x00000001)
#define YAS_TYPE_MAG			(0x00000002)
#define YAS_TYPE_GYRO			(0x00000004)

#define YAS_NO_ERROR			(0) /*!< Succeed */
#define YAS_ERROR_ARG			(-1) /*!< Invalid argument */
#define YAS_ERROR_INITIALIZE		(-2) /*!< Invalid initialization status
					      */
#define YAS_ERROR_BUSY			(-3) /*!< Sensor is busy */
#define YAS_ERROR_DEVICE_COMMUNICATION	(-4) /*!< Device communication error */
#define YAS_ERROR_CHIP_ID		(-5) /*!< Invalid chip id */
#define YAS_ERROR_CALREG		(-6) /*!< Invalid CAL register */
#define YAS_ERROR_OVERFLOW		(-7) /*!< Overflow occured */
#define YAS_ERROR_UNDERFLOW		(-8) /*!< Underflow occured */
#define YAS_ERROR_DIRCALC		(-9) /*!< Direction calcuration error */
#define YAS_ERROR_ERROR			(-128) /*!< other error */

/*! YAS537 extension command: self test */
#define YAS537_SELF_TEST		(0x00000001)
/*! YAS537 extension command: self test noise */
#define YAS537_SELF_TEST_NOISE		(0x00000002)
/*! YAS537 extension command: obtains last raw data (x, y1, y2, t) */
#define YAS537_GET_LAST_RAWDATA		(0x00000003)
/*! YAS537 extension command: obtains the average samples */
#define YAS537_GET_AVERAGE_SAMPLE	(0x00000004)
/*! YAS537 extension command: sets the average samples */
#define YAS537_SET_AVERAGE_SAMPLE	(0x00000005)
/*! YAS537 extension command: obtains the hardware offset */
#define YAS537_GET_HW_OFFSET		(0x00000006)
/*! YAS537 extension command: obtains the ellipsoidal correction matrix */
#define YAS537_GET_STATIC_MATRIX	(0x00000007)
/*! YAS537 extension command: sets the ellipsoidal correction matrix */
#define YAS537_SET_STATIC_MATRIX	(0x00000008)
/*! YAS537 extension command: obtains the overflow and underflow threshold */
#define YAS537_GET_OUFLOW_THRESH	(0x00000009)

/*! Mangetic vdd in mV */
#define YAS_MAG_VCORE			(1800)
/*! Default sensor delay in [msec] */
#define YAS_DEFAULT_SENSOR_DELAY	(50)

#ifndef ABS
#define ABS(a)		((a) > 0 ? (a) : -(a)) /*!< Absolute value */
#endif
#ifndef CLIP
#define CLIP(in, min, max) \
		((in) < (min) ? (min) : ((max) < (in) ? (max) : (in)))
#endif

struct yas_matrix {
	int16_t m[9]; /*!< matrix data */
};

struct yas_vector {
	int32_t v[3]; /*!< vector data */
};

struct yas_data {
	int32_t type; /*!< Sensor type */
	struct yas_vector xyz; /*!< X, Y, Z measurement data of the sensor */
	uint32_t timestamp; /*!< Measurement time */
	uint8_t accuracy; /*!< Measurement data accuracy */
};

struct yas_self_test_result {
	int32_t id;
	int32_t dir;
	int32_t sx, sy;
	int32_t xyz[3];
};

struct yas_driver_callback {
	/**
	 * Open the device
	 * @param[in] type Sensor type
	 * @retval 0 Success
	 * @retval Negative Failure
	 */
	int (*device_open)(int32_t type);
	/**
	 * Close the device
	 * @param[in] type Sensor type
	 * @retval 0 Success
	 * @retval Negative Failure
	 */
	int (*device_close)(int32_t type);
	/**
	 * Send data to the device
	 * @param[in] type Sensor type
	 * @param[in] addr Register address
	 * @param[in] buf The pointer to the data buffer to be sent
	 * @param[in] len The length of the data buffer
	 * @retval 0 Success
	 * @retval Negative Failure
	 */
	int (*device_write)(int32_t type, uint8_t addr, const uint8_t *buf,
			int len);
	/**
	 * Receive data from the device
	 * @param[in] type Sensor type
	 * @param[in] addr Register address
	 * @param[out] buf The pointer to the data buffer to be received
	 * @param[in] len The length of the data buffer
	 * @retval 0 Success
	 * @retval Negative Failure
	 */
	int (*device_read)(int32_t type, uint8_t addr, uint8_t *buf, int len);
	/**
	 * Sleep in micro-seconds
	 * @param[in] usec Sleep time in micro-seconds
	 */
	void (*usleep)(int usec);
	/**
	 * Obtains the current time in milli-seconds
	 * @return The current time in milli-seconds
	 */
	uint32_t (*current_time)(void);
};

/**
 * @struct yas_mag_driver
 * @brief Magnetic sensor driver
 */
struct yas_mag_driver {
	/**
	 * Initializes the sensor, starts communication with the device
	 * @retval #YAS_NO_ERROR Success
	 * @retval Negative Failure
	 */
	int (*init)(void);
	/**
	 * Terminates the communications with the device
	 * @retval #YAS_NO_ERROR Success
	 * @retval Negative Failure
	 */
	int (*term)(void);
	/**
	 * Obtains measurment period in milli-seconds
	 * @retval Non-Negatiev Measurement period in milli-seconds
	 * @retval Negative Failure
	 */
	int (*get_delay)(void);
	/**
	 * Sets measurment period in milli-seconds
	 * @param[in] delay Measurement period in milli-seconds
	 * @retval #YAS_NO_ERROR Success
	 * @retval Negative Failure
	 */
	int (*set_delay)(int delay);
	/**
	 * Reports the sensor status (Enabled or Disabled)
	 * @retval 0 Disabled
	 * @retval 1 Enabled
	 * @retval Negative Failure
	 */
	int (*get_enable)(void);
	/**
	 * Enables or disables sensors
	 * @param[in] enable The status of the sensor (0: Disable, 1: Enable)
	 * @retval #YAS_NO_ERROR Success
	 * @retval Negative Failure
	 */
	int (*set_enable)(int enable);
	/**
	 * Obtains the sensor position
	 * @retval 0-7 The position of the sensor
	 * @retval Negative Failure
	 */
	int (*get_position)(void);
	/**
	 * Sets the sensor position
	 * @param[in] position The position of the sensor (0-7)
	 * @retval #YAS_NO_ERROR Success
	 * @retval Negative Failure
	 */
	int (*set_position)(int position);
	/**
	 * Measures the sensor
	 * @param[out] raw Measured sensor data
	 * @param[in] num The number of the measured sensor data
	 * @retval Non-Negative The number of the measured sensor data
	 * @retval Negative Failure
	 */
	int (*measure)(struct yas_data *raw, int num);
	/**
	 * Extension command execution specific to the part number
	 * @param[in] cmd Extension command id
	 * @param[out] result Extension command result
	 * @retval #YAS_NO_ERROR Success
	 * @retval Negative Failure
	 */
	int (*ext)(int32_t cmd, void *result);
	struct yas_driver_callback callback; /*!< Callback functions */
};

int yas_mag_driver_init(struct yas_mag_driver *f);

#endif /* #ifndef __YAS_H__ */
