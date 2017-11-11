/*
 *  Copyright (C) 2012, Samsung Electronics Co. Ltd. All Rights Reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 */
#include "ssp.h"

#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>
#include <linux/iio/events.h>
#include <linux/iio/buffer.h>
#include <linux/iio/types.h>

/*************************************************************************/
/* SSP Kernel -> HAL input evnet function                                */
/*************************************************************************/
#define IIO_BUFFER_12_BYTES         20 /* 12 + timestamp 8*/
#define IIO_BUFFER_6_BYTES          14
#define IIO_BUFFER_1_BYTES          9
#define IIO_BUFFER_17_BYTES         25
#define IIO_BUFFER_24_BYTES         20
#define IIO_BUFFER_7_BYTES          15

static int ssp_push_iio_buffer(struct iio_dev *indio_dev, u64 timestamp, u8 *data, int data_len)
{
	u8 buf[data_len+sizeof(timestamp)];

	memcpy(buf, data, data_len);
	memcpy(&buf[data_len], &timestamp, sizeof(timestamp));

	mutex_lock(&indio_dev->mlock);
	iio_push_to_buffers(indio_dev, buf);
	mutex_unlock(&indio_dev->mlock);

	return 0;
}

void report_meta_data(struct ssp_data *data, struct sensor_value *s)
{
	pr_info("[SSP]: %s - what: %d, sensor: %d\n", __func__,
		s->meta_data.what, s->meta_data.sensor);

	if (s->meta_data.sensor == ACCELEROMETER_SENSOR) {
		s16 accel_buf[3];
		memset(accel_buf, 0, sizeof(s16) * 3);
		ssp_push_iio_buffer(data->accel_indio_dev, 0, (u8*)accel_buf, sizeof(accel_buf));
	} else if (s->meta_data.sensor == GYROSCOPE_SENSOR) {
		int gyro_buf[3];
		memset(gyro_buf, 0, sizeof(int) * 3);
		ssp_push_iio_buffer(data->gyro_indio_dev, 0, (u8*)gyro_buf, sizeof(gyro_buf));
	} else if (s->meta_data.sensor == GYRO_UNCALIB_SENSOR) {
		s16 uncal_gyro_buf[6];
		memset(uncal_gyro_buf, 0, sizeof(s16) * 6);
		ssp_push_iio_buffer(data->uncal_gyro_indio_dev,	0, (u8*)uncal_gyro_buf, sizeof(uncal_gyro_buf));
	} else if (s->meta_data.sensor == GEOMAGNETIC_SENSOR) {
		u8 mag_buf[7];
		memset(mag_buf, 0 , 7);
		ssp_push_iio_buffer(data->mag_indio_dev, 0, (u8*)mag_buf, sizeof(mag_buf));
	} else if (s->meta_data.sensor == GEOMAGNETIC_UNCALIB_SENSOR) {
		s16 uncal_mag_buf[6];
		memset(uncal_mag_buf, 0, sizeof(s16) * 6);
		ssp_push_iio_buffer(data->uncal_mag_indio_dev, 0, (u8*)uncal_mag_buf, sizeof(uncal_mag_buf));
	} else if (s->meta_data.sensor == PRESSURE_SENSOR) {
		int pressure_buf[3];
		memset(pressure_buf, 0, sizeof(int) * 3);
		ssp_push_iio_buffer(data->pressure_indio_dev, 0, (u8*)pressure_buf, sizeof(pressure_buf));
	} else if (s->meta_data.sensor == ROTATION_VECTOR) {
		int rot_buf[5];
		memset(rot_buf, 0, sizeof(int) * 5);
		ssp_push_iio_buffer(data->rot_indio_dev, 0, (u8*)rot_buf, sizeof(rot_buf));
	} else if (s->meta_data.sensor == GAME_ROTATION_VECTOR) {
		int grot_buf[5];
		memset(grot_buf, 0, sizeof(int) * 5);
		ssp_push_iio_buffer(data->game_rot_indio_dev, 0, (u8*)grot_buf, sizeof(grot_buf));
	} else if (s->meta_data.sensor == STEP_DETECTOR) {
		u8 step_buf[1] = {0};
		ssp_push_iio_buffer(data->step_det_indio_dev, 0, (u8*)step_buf, sizeof(step_buf));
	} else {
		input_report_rel(data->meta_input_dev, REL_DIAL,
			s->meta_data.what);
		input_report_rel(data->meta_input_dev, REL_HWHEEL,
			s->meta_data.sensor + 1);
		input_sync(data->meta_input_dev);
	}
}

void report_acc_data(struct ssp_data *data, struct sensor_value *accdata)
{
	s16 accel_buf[3];

	data->buf[ACCELEROMETER_SENSOR].x = accdata->x;
	data->buf[ACCELEROMETER_SENSOR].y = accdata->y;
	data->buf[ACCELEROMETER_SENSOR].z = accdata->z;
	
	accel_buf[0] = data->buf[ACCELEROMETER_SENSOR].x;
	accel_buf[1] = data->buf[ACCELEROMETER_SENSOR].y;
	accel_buf[2] = data->buf[ACCELEROMETER_SENSOR].z;

	ssp_push_iio_buffer(data->accel_indio_dev, accdata->timestamp, (u8*)accel_buf, sizeof(accel_buf));
}

void report_gyro_data(struct ssp_data *data, struct sensor_value *gyrodata)
{
	int lTemp[3] = {0,};

	data->buf[GYROSCOPE_SENSOR].x = gyrodata->x;
	data->buf[GYROSCOPE_SENSOR].y = gyrodata->y;
	data->buf[GYROSCOPE_SENSOR].z = gyrodata->z;

	if (data->uGyroDps == GYROSCOPE_DPS500) {
		lTemp[0] = (int)data->buf[GYROSCOPE_SENSOR].x >> 2;
		lTemp[1] = (int)data->buf[GYROSCOPE_SENSOR].y >> 2;
		lTemp[2] = (int)data->buf[GYROSCOPE_SENSOR].z >> 2;
	} else if (data->uGyroDps == GYROSCOPE_DPS250) {
		lTemp[0] = (int)data->buf[GYROSCOPE_SENSOR].x >> 3;
		lTemp[1] = (int)data->buf[GYROSCOPE_SENSOR].y >> 3;
		lTemp[2] = (int)data->buf[GYROSCOPE_SENSOR].z >> 3;
	} else {
		lTemp[0] = (int)data->buf[GYROSCOPE_SENSOR].x;
		lTemp[1] = (int)data->buf[GYROSCOPE_SENSOR].y;
		lTemp[2] = (int)data->buf[GYROSCOPE_SENSOR].z;
	}

	ssp_push_iio_buffer(data->gyro_indio_dev, gyrodata->timestamp, (u8*)lTemp, sizeof(lTemp));
}

#ifdef CONFIG_SENSORS_SSP_INTERRUPT_GYRO_SENSOR
void report_interrupt_gyro_data(struct ssp_data *data, struct sensor_value *gyrodata)
{
	int lTemp[3] = {0,};

	data->buf[INTERRUPT_GYRO_SENSOR].x = gyrodata->x;
	data->buf[INTERRUPT_GYRO_SENSOR].y = gyrodata->y;
	data->buf[INTERRUPT_GYRO_SENSOR].z = gyrodata->z;

	if (data->uGyroDps == GYROSCOPE_DPS500) {
		lTemp[0] = (int)data->buf[INTERRUPT_GYRO_SENSOR].x >> 2;
		lTemp[1] = (int)data->buf[INTERRUPT_GYRO_SENSOR].y >> 2;
		lTemp[2] = (int)data->buf[INTERRUPT_GYRO_SENSOR].z >> 2;
	} else if (data->uGyroDps == GYROSCOPE_DPS250) {
		lTemp[0] = (int)data->buf[INTERRUPT_GYRO_SENSOR].x >> 3;
		lTemp[1] = (int)data->buf[INTERRUPT_GYRO_SENSOR].y >> 3;
		lTemp[2] = (int)data->buf[INTERRUPT_GYRO_SENSOR].z >> 3;
	} else {
		lTemp[0] = (int)data->buf[INTERRUPT_GYRO_SENSOR].x;
		lTemp[1] = (int)data->buf[INTERRUPT_GYRO_SENSOR].y;
		lTemp[2] = (int)data->buf[INTERRUPT_GYRO_SENSOR].z;
	}

	input_report_rel(data->interrupt_gyro_input_dev, REL_RX, data->buf[INTERRUPT_GYRO_SENSOR].x);
	input_report_rel(data->interrupt_gyro_input_dev, REL_RY, data->buf[INTERRUPT_GYRO_SENSOR].y);
	input_report_rel(data->interrupt_gyro_input_dev, REL_RZ, data->buf[INTERRUPT_GYRO_SENSOR].z);

	input_sync(data->interrupt_gyro_input_dev);
}
#endif

void report_geomagnetic_raw_data(struct ssp_data *data,
	struct sensor_value *magrawdata)
{
	data->buf[GEOMAGNETIC_RAW].x = magrawdata->x;
	data->buf[GEOMAGNETIC_RAW].y = magrawdata->y;
	data->buf[GEOMAGNETIC_RAW].z = magrawdata->z;
}

void report_mag_data(struct ssp_data *data, struct sensor_value *magdata)
{
	u8 lTemp[7] = { 0, };
	
	data->buf[GEOMAGNETIC_SENSOR].cal_x = magdata->cal_x;
	data->buf[GEOMAGNETIC_SENSOR].cal_y = magdata->cal_y;
	data->buf[GEOMAGNETIC_SENSOR].cal_z = magdata->cal_z;
	data->buf[GEOMAGNETIC_SENSOR].accuracy = magdata->accuracy;

	memcpy(lTemp, magdata, sizeof(lTemp));

	ssp_push_iio_buffer(data->mag_indio_dev, magdata->timestamp, (u8*)lTemp, sizeof(lTemp));
}

void report_mag_uncaldata(struct ssp_data *data, struct sensor_value *magdata)
{
	s16 lTemp[6] = {0,};
	data->buf[GEOMAGNETIC_UNCALIB_SENSOR].uncal_x = magdata->uncal_x;
	data->buf[GEOMAGNETIC_UNCALIB_SENSOR].uncal_y = magdata->uncal_y;
	data->buf[GEOMAGNETIC_UNCALIB_SENSOR].uncal_z = magdata->uncal_z;
	data->buf[GEOMAGNETIC_UNCALIB_SENSOR].offset_x= magdata->offset_x;
	data->buf[GEOMAGNETIC_UNCALIB_SENSOR].offset_y= magdata->offset_y;
	data->buf[GEOMAGNETIC_UNCALIB_SENSOR].offset_z= magdata->offset_z;

	lTemp[0] = data->buf[GEOMAGNETIC_UNCALIB_SENSOR].uncal_x;
	lTemp[1] = data->buf[GEOMAGNETIC_UNCALIB_SENSOR].uncal_y;
	lTemp[2] = data->buf[GEOMAGNETIC_UNCALIB_SENSOR].uncal_z;
	lTemp[3] = data->buf[GEOMAGNETIC_UNCALIB_SENSOR].offset_x;
	lTemp[4] = data->buf[GEOMAGNETIC_UNCALIB_SENSOR].offset_y;
	lTemp[5] = data->buf[GEOMAGNETIC_UNCALIB_SENSOR].offset_z;

	ssp_push_iio_buffer(data->uncal_mag_indio_dev, magdata->timestamp, (u8*)lTemp, sizeof(lTemp));
}

void report_uncalib_gyro_data(struct ssp_data *data, struct sensor_value *gyrodata)
{
	s16 lTemp[6] = {0,};
	data->buf[GYRO_UNCALIB_SENSOR].uncal_x = gyrodata->uncal_x;
	data->buf[GYRO_UNCALIB_SENSOR].uncal_y = gyrodata->uncal_y;
	data->buf[GYRO_UNCALIB_SENSOR].uncal_z = gyrodata->uncal_z;
	data->buf[GYRO_UNCALIB_SENSOR].offset_x = gyrodata->offset_x;
	data->buf[GYRO_UNCALIB_SENSOR].offset_y = gyrodata->offset_y;
	data->buf[GYRO_UNCALIB_SENSOR].offset_z = gyrodata->offset_z;

	lTemp[0] = gyrodata->uncal_x;
	lTemp[1] = gyrodata->uncal_y;
	lTemp[2] = gyrodata->uncal_z;
	lTemp[3] = gyrodata->offset_x;
	lTemp[4] = gyrodata->offset_y;
	lTemp[5] = gyrodata->offset_z;

	ssp_push_iio_buffer(data->uncal_gyro_indio_dev,	gyrodata->timestamp, (u8*)lTemp, sizeof(lTemp));
}

void report_sig_motion_data(struct ssp_data *data,
	struct sensor_value *sig_motion_data)
{
	data->buf[SIG_MOTION_SENSOR].sig_motion = sig_motion_data->sig_motion;

	input_report_rel(data->sig_motion_input_dev, REL_MISC,
		data->buf[SIG_MOTION_SENSOR].sig_motion);
	input_sync(data->sig_motion_input_dev);

	wake_lock_timeout(&data->ssp_wake_lock, 2 * HZ);
}

void report_rot_data(struct ssp_data *data, struct sensor_value *rotdata)
{
	u8 rot_buf[17];

	data->buf[ROTATION_VECTOR].quat_a = rotdata->quat_a;
	data->buf[ROTATION_VECTOR].quat_b = rotdata->quat_b;
	data->buf[ROTATION_VECTOR].quat_c = rotdata->quat_c;
	data->buf[ROTATION_VECTOR].quat_d = rotdata->quat_d;
	data->buf[ROTATION_VECTOR].acc_rot = rotdata->acc_rot;

	rot_buf[0] = rotdata->quat_a;
	rot_buf[1] = rotdata->quat_b;
	rot_buf[2] = rotdata->quat_c;
	rot_buf[3] = rotdata->quat_d;
	rot_buf[4] = rotdata->acc_rot;
	
	memcpy(rot_buf, rotdata, sizeof(rot_buf));

	ssp_push_iio_buffer(data->rot_indio_dev, rotdata->timestamp, rot_buf, sizeof(rot_buf));
}

void report_game_rot_data(struct ssp_data *data, struct sensor_value *grvec_data)
{
	u8 grot_buf[17];

	data->buf[GAME_ROTATION_VECTOR].quat_a = grvec_data->quat_a;
	data->buf[GAME_ROTATION_VECTOR].quat_b = grvec_data->quat_b;
	data->buf[GAME_ROTATION_VECTOR].quat_c = grvec_data->quat_c;
	data->buf[GAME_ROTATION_VECTOR].quat_d = grvec_data->quat_d;
	data->buf[GAME_ROTATION_VECTOR].acc_rot = grvec_data->acc_rot;

	grot_buf[0] = grvec_data->quat_a;
	grot_buf[1] = grvec_data->quat_b;
	grot_buf[2] = grvec_data->quat_c;
	grot_buf[3] = grvec_data->quat_d;
	grot_buf[4] = grvec_data->acc_rot;
	
	memcpy(grot_buf, grvec_data, sizeof(grot_buf));

	ssp_push_iio_buffer(data->game_rot_indio_dev, grvec_data->timestamp, grot_buf, sizeof(grot_buf));
}

void report_gesture_data(struct ssp_data *data, struct sensor_value *gesdata)
{
	int i = 0;
	for (i=0; i<20; i++) {
		data->buf[GESTURE_SENSOR].data[i] = gesdata->data[i];
	}

	input_report_abs(data->gesture_input_dev,
		ABS_X, data->buf[GESTURE_SENSOR].data[0]);
	input_report_abs(data->gesture_input_dev,
		ABS_Y, data->buf[GESTURE_SENSOR].data[1]);
	input_report_abs(data->gesture_input_dev,
		ABS_Z, data->buf[GESTURE_SENSOR].data[2]);
	input_report_abs(data->gesture_input_dev,
		ABS_RX, data->buf[GESTURE_SENSOR].data[3]);
	input_report_abs(data->gesture_input_dev,
		ABS_RY, data->buf[GESTURE_SENSOR].data[4]);
	input_report_abs(data->gesture_input_dev,
		ABS_RZ, data->buf[GESTURE_SENSOR].data[5]);
	input_report_abs(data->gesture_input_dev,
		ABS_THROTTLE, data->buf[GESTURE_SENSOR].data[6]);
	input_report_abs(data->gesture_input_dev,
		ABS_RUDDER, data->buf[GESTURE_SENSOR].data[7]);
	input_report_abs(data->gesture_input_dev,
		ABS_WHEEL, data->buf[GESTURE_SENSOR].data[8]);
	input_report_abs(data->gesture_input_dev,
		ABS_GAS, data->buf[GESTURE_SENSOR].data[9]);
	input_report_abs(data->gesture_input_dev,
		ABS_BRAKE, data->buf[GESTURE_SENSOR].data[10]);
	input_report_abs(data->gesture_input_dev,
		ABS_HAT0X, data->buf[GESTURE_SENSOR].data[11]);
	input_report_abs(data->gesture_input_dev,
		ABS_HAT0Y, data->buf[GESTURE_SENSOR].data[12]);
	input_report_abs(data->gesture_input_dev,
		ABS_HAT1X, data->buf[GESTURE_SENSOR].data[13]);
	input_report_abs(data->gesture_input_dev,
		ABS_HAT1Y, data->buf[GESTURE_SENSOR].data[14]);
	input_report_abs(data->gesture_input_dev,
		ABS_HAT2X, data->buf[GESTURE_SENSOR].data[15]);
	input_report_abs(data->gesture_input_dev,
		ABS_HAT2Y, data->buf[GESTURE_SENSOR].data[16]);
	input_report_abs(data->gesture_input_dev,
		ABS_HAT3X, data->buf[GESTURE_SENSOR].data[17]);
	input_report_abs(data->gesture_input_dev,
		ABS_HAT3Y, data->buf[GESTURE_SENSOR].data[18]);
	input_report_abs(data->gesture_input_dev,
		ABS_PRESSURE, data->buf[GESTURE_SENSOR].data[19]);

	input_sync(data->gesture_input_dev);
}

void report_pressure_data(struct ssp_data *data, struct sensor_value *predata)
{
	int temp[3] = {0, };
	data->buf[PRESSURE_SENSOR].pressure[0] =
		predata->pressure[0] - data->iPressureCal;
	data->buf[PRESSURE_SENSOR].pressure[1] = predata->pressure[1];

	temp[0] = data->buf[PRESSURE_SENSOR].pressure[0];
	temp[1] = data->buf[PRESSURE_SENSOR].pressure[1];
	temp[2] = data->sealevelpressure;

	ssp_push_iio_buffer(data->pressure_indio_dev, predata->timestamp, (u8*)temp, sizeof(temp));
}

void report_light_data(struct ssp_data *data, struct sensor_value *lightdata)
{
	data->buf[LIGHT_SENSOR].r = lightdata->r;
	data->buf[LIGHT_SENSOR].g = lightdata->g;
	data->buf[LIGHT_SENSOR].b = lightdata->b;
	data->buf[LIGHT_SENSOR].w = lightdata->w;
	data->buf[LIGHT_SENSOR].a_time = lightdata->a_time;
	data->buf[LIGHT_SENSOR].a_gain = (0x03) & (lightdata->a_gain);

	input_report_rel(data->light_input_dev, REL_HWHEEL,
		data->buf[LIGHT_SENSOR].r + 1);
	input_report_rel(data->light_input_dev, REL_DIAL,
		data->buf[LIGHT_SENSOR].g + 1);
	input_report_rel(data->light_input_dev, REL_WHEEL,
		data->buf[LIGHT_SENSOR].b + 1);
	input_report_rel(data->light_input_dev, REL_MISC,
		data->buf[LIGHT_SENSOR].w + 1);

	input_report_rel(data->light_input_dev, REL_RY,
		data->buf[LIGHT_SENSOR].a_time + 1);
	input_report_rel(data->light_input_dev, REL_RZ,
		data->buf[LIGHT_SENSOR].a_gain + 1);

	if(data->light_log_cnt < 3)
	{
		ssp_dbg("[SSP] #>L r=%d g=%d b=%d c=%d atime=%d again=%d",
			data->buf[LIGHT_SENSOR].r,data->buf[LIGHT_SENSOR].g,data->buf[LIGHT_SENSOR].b,
			data->buf[LIGHT_SENSOR].w,data->buf[LIGHT_SENSOR].a_time,data->buf[LIGHT_SENSOR].a_gain);
		data->light_log_cnt++;
	}

	input_sync(data->light_input_dev);
}

#ifdef CONFIG_SENSORS_SSP_IRDATA_FOR_CAMERA
void report_light_ir_data(struct ssp_data *data, struct sensor_value *lightirdata)
{
	data->buf[LIGHT_IR_SENSOR].irdata = lightirdata->irdata;
	data->buf[LIGHT_IR_SENSOR].ir_r= lightirdata->ir_r;
	data->buf[LIGHT_IR_SENSOR].ir_g = lightirdata->ir_g;
	data->buf[LIGHT_IR_SENSOR].ir_b = lightirdata->ir_b;
	data->buf[LIGHT_IR_SENSOR].ir_w = lightirdata->ir_w;
	data->buf[LIGHT_IR_SENSOR].ir_a_time = lightirdata->ir_a_time;
	data->buf[LIGHT_IR_SENSOR].ir_a_gain = (0x03) & (lightirdata->ir_a_gain);

	input_report_rel(data->light_ir_input_dev, REL_RX,
		data->buf[LIGHT_IR_SENSOR].irdata + 1);
	input_report_rel(data->light_ir_input_dev, REL_HWHEEL,
		data->buf[LIGHT_IR_SENSOR].ir_r + 1);
	input_report_rel(data->light_ir_input_dev, REL_DIAL,
		data->buf[LIGHT_IR_SENSOR].ir_g + 1);
	input_report_rel(data->light_ir_input_dev, REL_WHEEL,
		data->buf[LIGHT_IR_SENSOR].ir_b + 1);

	input_report_rel(data->light_ir_input_dev, REL_MISC,
		data->buf[LIGHT_IR_SENSOR].ir_w + 1);
	input_report_rel(data->light_ir_input_dev, REL_RY,
		data->buf[LIGHT_IR_SENSOR].ir_a_time + 1);
	input_report_rel(data->light_ir_input_dev, REL_RZ,
		data->buf[LIGHT_IR_SENSOR].ir_a_gain + 1);

	input_sync(data->light_ir_input_dev);

	if(data->light_ir_log_cnt < 3)
	{
		ssp_dbg("[SSP]#>IR irdata=%d r=%d g=%d b=%d c=%d atime=%d again=%d \n",
			data->buf[LIGHT_IR_SENSOR].irdata,data->buf[LIGHT_IR_SENSOR].ir_r,data->buf[LIGHT_IR_SENSOR].ir_g,
			data->buf[LIGHT_IR_SENSOR].ir_b,data->buf[LIGHT_IR_SENSOR].ir_w,data->buf[LIGHT_IR_SENSOR].ir_a_time,
			data->buf[LIGHT_IR_SENSOR].ir_a_gain);
		data->light_ir_log_cnt++;
	}

}
#endif

void report_prox_data(struct ssp_data *data, struct sensor_value *proxdata)
{
	u32 ts_high, ts_low;
	ts_high = (u32)((proxdata->timestamp)>>32);
	ts_low = (u32)((proxdata->timestamp)&0x00000000ffffffff);
	
	ssp_dbg("[SSP] Proximity Sensor Detect : %u, raw : %u ts : %llu %d %d\n",
		proxdata->prox[0], proxdata->prox[1], proxdata->timestamp, ts_high, ts_low);

	data->buf[PROXIMITY_SENSOR].prox[0] = proxdata->prox[0];
	data->buf[PROXIMITY_SENSOR].prox[1] = proxdata->prox[1];

	input_report_rel(data->prox_input_dev, REL_DIAL,
		((!proxdata->prox[0]))+1);
	input_report_rel(data->prox_input_dev, REL_WHEEL,
		ts_high);
	input_report_rel(data->prox_input_dev, REL_MISC,
		ts_low);
	input_sync(data->prox_input_dev);

	wake_lock_timeout(&data->ssp_wake_lock, 2 * HZ);
}

void report_prox_raw_data(struct ssp_data *data,
	struct sensor_value *proxrawdata)
{
	if (data->uFactoryProxAvg[0]++ >= PROX_AVG_READ_NUM) {
		data->uFactoryProxAvg[2] /= PROX_AVG_READ_NUM;
		data->buf[PROXIMITY_RAW].prox[1] = (u16)data->uFactoryProxAvg[1];
		data->buf[PROXIMITY_RAW].prox[2] = (u16)data->uFactoryProxAvg[2];
		data->buf[PROXIMITY_RAW].prox[3] = (u16)data->uFactoryProxAvg[3];

		data->uFactoryProxAvg[0] = 0;
		data->uFactoryProxAvg[1] = 0;
		data->uFactoryProxAvg[2] = 0;
		data->uFactoryProxAvg[3] = 0;
	} else {
		data->uFactoryProxAvg[2] += proxrawdata->prox[0];

		if (data->uFactoryProxAvg[0] == 1)
			data->uFactoryProxAvg[1] = proxrawdata->prox[0];
		else if (proxrawdata->prox[0] < data->uFactoryProxAvg[1])
			data->uFactoryProxAvg[1] = proxrawdata->prox[0];

		if (proxrawdata->prox[0] > data->uFactoryProxAvg[3])
			data->uFactoryProxAvg[3] = proxrawdata->prox[0];
	}

	data->buf[PROXIMITY_RAW].prox[0] = proxrawdata->prox[0];
}

#ifdef CONFIG_SENSORS_SSP_SX9306
void report_grip_data(struct ssp_data *data, struct sensor_value *gripdata)
{
	pr_err("[SSP] grip = %d %d %d 0x%02x\n",
		gripdata->cap_main, gripdata->useful, gripdata->offset,
		gripdata->irq_stat);

	if (data->grip_off) {
		pr_err("[SSP] grip_off = true, skip reporting grip data\n");
		return;
	}

#if defined(CONFIG_MUIC_NOTIFIER)
	if (data->jig_is_attached) {
		pr_err("[SSP] jig_is_attached, skip reporting grip data\n");
		return;
	}
#endif

	data->buf[GRIP_SENSOR].cap_main = gripdata->cap_main;
	data->buf[GRIP_SENSOR].useful = gripdata->useful;
	data->buf[GRIP_SENSOR].offset = gripdata->offset;
	data->buf[GRIP_SENSOR].irq_stat = gripdata->irq_stat;

	input_report_rel(data->grip_input_dev, REL_MISC,
			data->buf[GRIP_SENSOR].irq_stat + 1);
	input_sync(data->grip_input_dev);

	wake_lock_timeout(&data->ssp_wake_lock, 3 * HZ);
}
#endif

void report_step_det_data(struct ssp_data *data,
		struct sensor_value *stepdet_data)
{
	data->buf[STEP_DETECTOR].step_det = stepdet_data->step_det;
	ssp_push_iio_buffer(data->step_det_indio_dev, stepdet_data->timestamp, (u8*)&stepdet_data->step_det, 1);
}

void report_step_cnt_data(struct ssp_data *data,
	struct sensor_value *sig_motion_data)
{
	data->buf[STEP_COUNTER].step_diff = sig_motion_data->step_diff;

	data->step_count_total += data->buf[STEP_COUNTER].step_diff;

	input_report_rel(data->step_cnt_input_dev, REL_MISC,
		data->step_count_total + 1);
	input_sync(data->step_cnt_input_dev);
}

void report_temp_humidity_data(struct ssp_data *data,
	struct sensor_value *temp_humi_data)
{
	data->buf[TEMPERATURE_HUMIDITY_SENSOR].x = temp_humi_data->x;
	data->buf[TEMPERATURE_HUMIDITY_SENSOR].y = temp_humi_data->y;
	data->buf[TEMPERATURE_HUMIDITY_SENSOR].z = temp_humi_data->z;

	/* Temperature */
	input_report_rel(data->temp_humi_input_dev, REL_HWHEEL,
		data->buf[TEMPERATURE_HUMIDITY_SENSOR].x);
	/* Humidity */
	input_report_rel(data->temp_humi_input_dev, REL_DIAL,
		data->buf[TEMPERATURE_HUMIDITY_SENSOR].y);
	input_sync(data->temp_humi_input_dev);
	if (data->buf[TEMPERATURE_HUMIDITY_SENSOR].z)
		wake_lock_timeout(&data->ssp_wake_lock, 2 * HZ);
}

#ifdef CONFIG_SENSORS_SSP_SHTC1
void report_bulk_comp_data(struct ssp_data *data)
{
	input_report_rel(data->temp_humi_input_dev, REL_WHEEL,
		data->bulk_buffer->len);
	input_sync(data->temp_humi_input_dev);
}
#endif

void report_tilt_data(struct ssp_data *data,
		struct sensor_value *tilt_data)
{
	data->buf[TILT_DETECTOR].tilt_detector = tilt_data->tilt_detector;
	ssp_push_iio_buffer(data->tilt_indio_dev, tilt_data->timestamp,
			&tilt_data->tilt_detector, 1);
	wake_lock_timeout(&data->ssp_wake_lock, 2 * HZ);
	pr_err("[SSP]: %s: %d", __func__,  tilt_data->tilt_detector);
}

void report_pickup_data(struct ssp_data *data,
		struct sensor_value *pickup_data)
{
	data->buf[PICKUP_GESTURE].pickup_gesture = pickup_data->pickup_gesture;
	ssp_push_iio_buffer(data->pickup_indio_dev, pickup_data->timestamp,
			&pickup_data->pickup_gesture, 1);
	wake_lock_timeout(&data->ssp_wake_lock, 2 * HZ);
	pr_err("[SSP]: %s: %d", __func__,  pickup_data->pickup_gesture);
}
int initialize_event_symlink(struct ssp_data *data)
{
	int iRet = 0;

	iRet = sensors_create_symlink(data->gesture_input_dev);
	if (iRet < 0)
		goto iRet_gesture_sysfs_create_link;

	iRet = sensors_create_symlink(data->light_input_dev);
	if (iRet < 0)
		goto iRet_light_sysfs_create_link;

#ifdef CONFIG_SENSORS_SSP_IRDATA_FOR_CAMERA
	iRet = sensors_create_symlink(data->light_ir_input_dev);
	if (iRet < 0)
		goto iRet_light_ir_sysfs_create_link;
#endif

	iRet = sensors_create_symlink(data->prox_input_dev);
	if (iRet < 0)
		goto iRet_prox_sysfs_create_link;

#ifdef CONFIG_SENSORS_SSP_SX9306
	iRet = sensors_create_symlink(data->grip_input_dev);
	if (iRet < 0)
		goto iRet_grip_sysfs_create_link;
#endif
	iRet = sensors_create_symlink(data->temp_humi_input_dev);
	if (iRet < 0)
		goto iRet_temp_humi_sysfs_create_link;

	iRet = sensors_create_symlink(data->sig_motion_input_dev);
	if (iRet < 0)
		goto iRet_sig_motion_sysfs_create_link;

	iRet = sensors_create_symlink(data->step_cnt_input_dev);
	if (iRet < 0)
		goto iRet_step_cnt_sysfs_create_link;
#ifdef CONFIG_SENSORS_SSP_INTERRUPT_GYRO_SENSOR
	iRet = sensors_create_symlink(data->interrupt_gyro_input_dev);
	if (iRet < 0)
		goto iRet_interrupt_gyro_sysfs_create_link;
#endif
	iRet = sensors_create_symlink(data->meta_input_dev);
	if (iRet < 0)
		goto iRet_meta_sysfs_create_link;

	return SUCCESS;

iRet_meta_sysfs_create_link:
#ifdef CONFIG_SENSORS_SSP_INTERRUPT_GYRO_SENSOR
	sensors_remove_symlink(data->interrupt_gyro_input_dev);
iRet_interrupt_gyro_sysfs_create_link:
#endif
	sensors_remove_symlink(data->step_cnt_input_dev);
iRet_step_cnt_sysfs_create_link:
	sensors_remove_symlink(data->sig_motion_input_dev);
iRet_sig_motion_sysfs_create_link:
	sensors_remove_symlink(data->temp_humi_input_dev);
iRet_temp_humi_sysfs_create_link:
#ifdef CONFIG_SENSORS_SSP_SX9306
	sensors_remove_symlink(data->grip_input_dev);
iRet_grip_sysfs_create_link:
#endif
	sensors_remove_symlink(data->prox_input_dev);
iRet_prox_sysfs_create_link:
#ifdef CONFIG_SENSORS_SSP_IRDATA_FOR_CAMERA
	sensors_remove_symlink(data->light_ir_input_dev);
iRet_light_ir_sysfs_create_link:
#endif
	sensors_remove_symlink(data->light_input_dev);
iRet_light_sysfs_create_link:
	sensors_remove_symlink(data->gesture_input_dev);
iRet_gesture_sysfs_create_link:
	pr_err("[SSP]: %s - could not create event symlink\n", __func__);

	return FAIL;
}

void remove_event_symlink(struct ssp_data *data)
{
	sensors_remove_symlink(data->gesture_input_dev);
	sensors_remove_symlink(data->light_input_dev);
#ifdef CONFIG_SENSORS_SSP_IRDATA_FOR_CAMERA
	sensors_remove_symlink(data->light_ir_input_dev);
#endif
	sensors_remove_symlink(data->prox_input_dev);
#ifdef CONFIG_SENSORS_SSP_SX9306
	sensors_remove_symlink(data->grip_input_dev);
#endif
	sensors_remove_symlink(data->temp_humi_input_dev);
	sensors_remove_symlink(data->sig_motion_input_dev);
	sensors_remove_symlink(data->step_cnt_input_dev);
#ifdef CONFIG_SENSORS_SSP_INTERRUPT_GYRO_SENSOR
	sensors_remove_symlink(data->interrupt_gyro_input_dev);
#endif
	sensors_remove_symlink(data->meta_input_dev);
}

static const struct iio_info accel_info = {
	.driver_module = THIS_MODULE,
};

static const struct iio_chan_spec accel_channels[] = {
	{
		.type = IIO_TIMESTAMP,
		.channel = -1,
		.scan_index = 3,
		.scan_type = IIO_ST('s', IIO_BUFFER_6_BYTES*8,
				IIO_BUFFER_6_BYTES*8, 0)
	}
};

static const struct iio_info gyro_info = {
	.driver_module = THIS_MODULE,
};

static const struct iio_chan_spec gyro_channels[] = {
	{
		.type = IIO_TIMESTAMP,
		.channel = -1,
		.scan_index = 3,
		.scan_type = IIO_ST('s', IIO_BUFFER_12_BYTES*8,
				IIO_BUFFER_12_BYTES*8, 0)
	}
};

static const struct iio_info uncal_gyro_info = {
	.driver_module = THIS_MODULE,
};

static const struct iio_chan_spec uncal_gyro_channels[] = {
	{
		.type = IIO_TIMESTAMP,
		.channel = -1,
		.scan_index = 3,
		.scan_type = IIO_ST('s', IIO_BUFFER_12_BYTES * 8,
			IIO_BUFFER_12_BYTES * 8, 0)
	}
};

static const struct iio_info mag_info = {
	.driver_module = THIS_MODULE,
};

static const struct iio_chan_spec mag_channels[] = {
	{
		.type = IIO_TIMESTAMP,
		.channel = -1,
		.scan_index = 3,
		.scan_type = IIO_ST('s', IIO_BUFFER_7_BYTES * 8,
			IIO_BUFFER_7_BYTES * 8, 0)
	}
};

static const struct iio_info uncal_mag_info = {
	.driver_module = THIS_MODULE,
};

static const struct iio_chan_spec uncal_mag_channels[] = {
	{
		.type = IIO_TIMESTAMP,
		.channel = -1,
		.scan_index = 3,
		.scan_type = IIO_ST('s', IIO_BUFFER_12_BYTES * 8,
			IIO_BUFFER_12_BYTES * 8, 0)
	}
};


static const struct iio_info game_rot_info = {
	.driver_module = THIS_MODULE,
};

static const struct iio_chan_spec game_rot_channels[] = {
	{
		.type = IIO_TIMESTAMP,
		.channel = -1,
		.scan_index = 3,
		.scan_type = IIO_ST('s', IIO_BUFFER_17_BYTES*8,
				IIO_BUFFER_17_BYTES*8, 0)
	}
};

static const struct iio_info rot_info = {
	.driver_module = THIS_MODULE,
};

static const struct iio_chan_spec rot_channels[] = {
	{
		.type = IIO_TIMESTAMP,
		.channel = -1,
		.scan_index = 3,
		.scan_type = IIO_ST('s', IIO_BUFFER_17_BYTES*8,
				IIO_BUFFER_17_BYTES*8, 0)
	}
};

static const struct iio_info step_det_info = {
	.driver_module = THIS_MODULE,
};

static const struct iio_chan_spec step_det_channels[] = {
	{
		.type = IIO_TIMESTAMP,
		.channel = -1,
		.scan_index = 3,
		.scan_type = IIO_ST('s', IIO_BUFFER_1_BYTES*8,
				IIO_BUFFER_1_BYTES*8, 0)
	}
};

static const struct iio_info pressure_info = {
	.driver_module = THIS_MODULE,
};

static const struct iio_chan_spec pressure_channels[] = {
	{
		.type = IIO_TIMESTAMP,
		.channel = -1,
		.scan_index = 3,
		.scan_type = IIO_ST('s', IIO_BUFFER_12_BYTES * 8,
			IIO_BUFFER_12_BYTES * 8, 0)
	}
};

static const struct iio_info tilt_info = {
	.driver_module = THIS_MODULE,
};

static const struct iio_chan_spec tilt_channels[] = {
	{
		.type = IIO_TIMESTAMP,
		.channel = -1,
		.scan_index = 3,
		.scan_type = IIO_ST('s', IIO_BUFFER_1_BYTES*8,
			IIO_BUFFER_1_BYTES*8, 0)
	}
};

static const struct iio_info pickup_info = {
	.driver_module = THIS_MODULE,
};

static const struct iio_chan_spec pickup_channels[] = {
	{
		.type = IIO_TIMESTAMP,
		.channel = -1,
		.scan_index = 3,
		.scan_type = IIO_ST('s', IIO_BUFFER_1_BYTES*8,
			IIO_BUFFER_1_BYTES*8, 0)
	}
};

int initialize_input_dev(struct ssp_data *data)
{
	int iRet = 0;

	/* Registering iio device - start */
	data->accel_indio_dev = iio_device_alloc(0);
	if (!data->accel_indio_dev)
		goto err_alloc_accel;

	data->accel_indio_dev->name = "accelerometer_sensor";
	data->accel_indio_dev->dev.parent = &data->spi->dev;
	data->accel_indio_dev->info = &accel_info;
	data->accel_indio_dev->channels = accel_channels;
	data->accel_indio_dev->num_channels = ARRAY_SIZE(accel_channels);
	data->accel_indio_dev->modes = INDIO_DIRECT_MODE;
	data->accel_indio_dev->currentmode = INDIO_DIRECT_MODE;

	iRet = ssp_iio_configure_ring(data->accel_indio_dev);
	if (iRet)
		goto err_config_ring_accel;

	iRet = iio_buffer_register(data->accel_indio_dev, data->accel_indio_dev->channels,
					data->accel_indio_dev->num_channels);
	if (iRet)
		goto err_register_buffer_accel;

	iRet = iio_device_register(data->accel_indio_dev);
	if (iRet)
		goto err_register_device_accel;

	data->gyro_indio_dev = iio_device_alloc(0);
	if (!data->gyro_indio_dev)
		goto err_alloc_gyro;

	data->gyro_indio_dev->name = "gyro_sensor";
	data->gyro_indio_dev->dev.parent = &data->spi->dev;
	data->gyro_indio_dev->info = &gyro_info;
	data->gyro_indio_dev->channels = gyro_channels;
	data->gyro_indio_dev->num_channels = ARRAY_SIZE(gyro_channels);
	data->gyro_indio_dev->modes = INDIO_DIRECT_MODE;
	data->gyro_indio_dev->currentmode = INDIO_DIRECT_MODE;

	iRet = ssp_iio_configure_ring(data->gyro_indio_dev);
	if (iRet)
		goto err_config_ring_gyro;

	iRet = iio_buffer_register(data->gyro_indio_dev, data->gyro_indio_dev->channels,
					data->gyro_indio_dev->num_channels);
	if (iRet)
		goto err_register_buffer_gyro;

	iRet = iio_device_register(data->gyro_indio_dev);
	if (iRet)
		goto err_register_device_gyro;

	data->uncal_gyro_indio_dev = iio_device_alloc(0);
	if (!data->uncal_gyro_indio_dev)
		goto err_alloc_uncal_gyro;

	data->uncal_gyro_indio_dev->name = "uncal_gyro_sensor";
	data->uncal_gyro_indio_dev->dev.parent = &data->spi->dev;
	data->uncal_gyro_indio_dev->info = &uncal_gyro_info;
	data->uncal_gyro_indio_dev->channels = uncal_gyro_channels;
	data->uncal_gyro_indio_dev->num_channels = ARRAY_SIZE(uncal_gyro_channels);
	data->uncal_gyro_indio_dev->modes = INDIO_DIRECT_MODE;
	data->uncal_gyro_indio_dev->currentmode = INDIO_DIRECT_MODE;

	iRet = ssp_iio_configure_ring(data->uncal_gyro_indio_dev);
	if (iRet)
		goto err_config_ring_uncal_gyro;

	iRet = iio_buffer_register(data->uncal_gyro_indio_dev,
		data->uncal_gyro_indio_dev->channels,
		data->uncal_gyro_indio_dev->num_channels);
	if (iRet)
		goto err_register_buffer_uncal_gyro;

	iRet = iio_device_register(data->uncal_gyro_indio_dev);
	if (iRet)
		goto err_register_device_uncal_gyro;

	data->mag_indio_dev = iio_device_alloc(0);
	if (!data->mag_indio_dev)
		goto err_alloc_mag;

	data->mag_indio_dev->name = "geomagnetic_sensor";
	data->mag_indio_dev->dev.parent = &data->spi->dev;
	data->mag_indio_dev->info = &mag_info;
	data->mag_indio_dev->channels = mag_channels;
	data->mag_indio_dev->num_channels = ARRAY_SIZE(mag_channels);
	data->mag_indio_dev->modes = INDIO_DIRECT_MODE;
	data->mag_indio_dev->currentmode = INDIO_DIRECT_MODE;

	iRet = ssp_iio_configure_ring(data->mag_indio_dev);
	if (iRet)
		goto err_config_ring_mag;

	iRet = iio_buffer_register(data->mag_indio_dev,
			data->mag_indio_dev->channels,
			data->mag_indio_dev->num_channels);
	if (iRet)
		goto err_register_buffer_mag;

	iRet = iio_device_register(data->mag_indio_dev);
	if (iRet)
		goto err_register_device_mag;

	data->uncal_mag_indio_dev = iio_device_alloc(0);
	if (!data->uncal_mag_indio_dev)
		goto err_alloc_uncal_mag;

	data->uncal_mag_indio_dev->name = "uncal_geomagnetic_sensor";
	data->uncal_mag_indio_dev->dev.parent = &data->spi->dev;
	data->uncal_mag_indio_dev->info = &uncal_mag_info;
	data->uncal_mag_indio_dev->channels = uncal_mag_channels;
	data->uncal_mag_indio_dev->num_channels = ARRAY_SIZE(uncal_mag_channels);
	data->uncal_mag_indio_dev->modes = INDIO_DIRECT_MODE;
	data->uncal_mag_indio_dev->currentmode = INDIO_DIRECT_MODE;

	iRet = ssp_iio_configure_ring(data->uncal_mag_indio_dev);
	if (iRet)
		goto err_config_ring_uncal_mag;

	iRet = iio_buffer_register(data->uncal_mag_indio_dev,
			data->uncal_mag_indio_dev->channels,
			data->uncal_mag_indio_dev->num_channels);
	if (iRet)
		goto err_register_buffer_uncal_mag;

	iRet = iio_device_register(data->uncal_mag_indio_dev);
	if (iRet)
		goto err_register_device_uncal_mag;

	data->game_rot_indio_dev = iio_device_alloc(0);
	if (!data->game_rot_indio_dev)
		goto err_alloc_game_rot;

	data->game_rot_indio_dev->name = "game_rotation_vector";
	data->game_rot_indio_dev->dev.parent = &data->spi->dev;
	data->game_rot_indio_dev->info = &game_rot_info;
	data->game_rot_indio_dev->channels = game_rot_channels;
	data->game_rot_indio_dev->num_channels = ARRAY_SIZE(game_rot_channels);
	data->game_rot_indio_dev->modes = INDIO_DIRECT_MODE;
	data->game_rot_indio_dev->currentmode = INDIO_DIRECT_MODE;

	iRet = ssp_iio_configure_ring(data->game_rot_indio_dev);
	if (iRet)
		goto err_config_ring_game_rot;

	iRet = iio_buffer_register(data->game_rot_indio_dev,
			data->game_rot_indio_dev->channels,
			data->game_rot_indio_dev->num_channels);
	if (iRet)
		goto err_register_buffer_game_rot;

	iRet = iio_device_register(data->game_rot_indio_dev);
	if (iRet)
		goto err_register_device_game_rot;

	data->rot_indio_dev = iio_device_alloc(0);
	if (!data->rot_indio_dev)
		goto err_alloc_rot;

	data->rot_indio_dev->name = "rotation_vector_sensor";
	data->rot_indio_dev->dev.parent = &data->spi->dev;
	data->rot_indio_dev->info = &rot_info;
	data->rot_indio_dev->channels = rot_channels;
	data->rot_indio_dev->num_channels = ARRAY_SIZE(rot_channels);
	data->rot_indio_dev->modes = INDIO_DIRECT_MODE;
	data->rot_indio_dev->currentmode = INDIO_DIRECT_MODE;

	iRet = ssp_iio_configure_ring(data->rot_indio_dev);
	if (iRet)
		goto err_config_ring_rot;

	iRet = iio_buffer_register(data->rot_indio_dev,
			data->rot_indio_dev->channels,
			data->rot_indio_dev->num_channels);
	if (iRet)
		goto err_register_buffer_rot;

	iRet = iio_device_register(data->rot_indio_dev);
	if (iRet)
		goto err_register_device_rot;

	data->step_det_indio_dev= iio_device_alloc(0);
	if (!data->step_det_indio_dev)
		goto err_alloc_step_det;

	data->step_det_indio_dev->name = "step_det_sensor";
	data->step_det_indio_dev->dev.parent = &data->spi->dev;
	data->step_det_indio_dev->info = &step_det_info;
	data->step_det_indio_dev->channels = step_det_channels;
	data->step_det_indio_dev->num_channels = ARRAY_SIZE(step_det_channels);
	data->step_det_indio_dev->modes = INDIO_DIRECT_MODE;
	data->step_det_indio_dev->currentmode = INDIO_DIRECT_MODE;

	iRet = ssp_iio_configure_ring(data->step_det_indio_dev);
	if (iRet)
		goto err_config_ring_step_det;

	iRet = iio_buffer_register(data->step_det_indio_dev,
			data->step_det_indio_dev->channels,
			data->step_det_indio_dev->num_channels);
	if (iRet)
		goto err_register_buffer_step_det;

	iRet = iio_device_register(data->step_det_indio_dev);
	if (iRet)
		goto err_register_device_step_det;

	data->pressure_indio_dev = iio_device_alloc(0);
	if (!data->pressure_indio_dev)
		goto err_alloc_pressure;
	data->pressure_indio_dev->name = "pressure_sensor";
	data->pressure_indio_dev->dev.parent = &data->spi->dev;
	data->pressure_indio_dev->info = &pressure_info;
	data->pressure_indio_dev->channels = pressure_channels;
	data->pressure_indio_dev->num_channels = ARRAY_SIZE(pressure_channels);
	data->pressure_indio_dev->modes = INDIO_DIRECT_MODE;
	data->pressure_indio_dev->currentmode = INDIO_DIRECT_MODE;

	iRet = ssp_iio_configure_ring(data->pressure_indio_dev);
	if (iRet)
		goto err_config_ring_pressure;
	iRet = iio_buffer_register(data->pressure_indio_dev,
			data->pressure_indio_dev->channels,
			data->pressure_indio_dev->num_channels);
	if (iRet)
		goto err_register_buffer_pressure;
	iRet = iio_device_register(data->pressure_indio_dev);
	if (iRet)
		goto err_register_device_pressure;
	/* Registering iio device - end */

	data->light_input_dev = input_allocate_device();
	if (data->light_input_dev == NULL)
		goto err_initialize_light_input_dev;

	data->light_input_dev->name = "light_sensor";
	input_set_capability(data->light_input_dev, EV_REL, REL_HWHEEL);
	input_set_capability(data->light_input_dev, EV_REL, REL_DIAL);
	input_set_capability(data->light_input_dev, EV_REL, REL_WHEEL);
	input_set_capability(data->light_input_dev, EV_REL, REL_MISC);
	input_set_capability(data->light_input_dev, EV_REL, REL_RY);
	input_set_capability(data->light_input_dev, EV_REL, REL_RZ);

	iRet = input_register_device(data->light_input_dev);
	if (iRet < 0) {
		input_free_device(data->light_input_dev);
		goto err_initialize_light_input_dev;
	}
	input_set_drvdata(data->light_input_dev, data);

#ifdef CONFIG_SENSORS_SSP_IRDATA_FOR_CAMERA
    data->light_ir_input_dev = input_allocate_device();
	if (data->light_ir_input_dev == NULL)
		goto err_initialize_light_ir_input_dev;

	data->light_ir_input_dev->name = "light_ir_sensor";
	input_set_capability(data->light_ir_input_dev, EV_REL, REL_HWHEEL);
	input_set_capability(data->light_ir_input_dev, EV_REL, REL_DIAL);
	input_set_capability(data->light_ir_input_dev, EV_REL, REL_WHEEL);
	input_set_capability(data->light_ir_input_dev, EV_REL, REL_MISC);
	input_set_capability(data->light_ir_input_dev, EV_REL, REL_RY);
	input_set_capability(data->light_ir_input_dev, EV_REL, REL_RZ);
	input_set_capability(data->light_ir_input_dev, EV_REL, REL_RX);

	iRet = input_register_device(data->light_ir_input_dev);
	if (iRet < 0) {
		input_free_device(data->light_ir_input_dev);
		goto err_initialize_light_ir_input_dev;
	}
	input_set_drvdata(data->light_ir_input_dev, data);
#endif

	data->prox_input_dev = input_allocate_device();
	if (data->prox_input_dev == NULL)
		goto err_initialize_proximity_input_dev;

	data->prox_input_dev->name = "proximity_sensor";
	input_set_capability(data->prox_input_dev, EV_REL, REL_DIAL);
	input_set_capability(data->prox_input_dev, EV_REL, REL_WHEEL);
	input_set_capability(data->prox_input_dev, EV_REL, REL_MISC);
	iRet = input_register_device(data->prox_input_dev);
	if (iRet < 0) {
		input_free_device(data->prox_input_dev);
		goto err_initialize_proximity_input_dev;
	}
	input_set_drvdata(data->prox_input_dev, data);

#ifdef CONFIG_SENSORS_SSP_SX9306
	data->grip_input_dev = input_allocate_device();
	if (data->grip_input_dev == NULL)
		goto err_initialize_grip_input_dev;

	data->grip_input_dev->name = "grip_sensor";
	input_set_capability(data->grip_input_dev, EV_REL, REL_MISC);
	iRet = input_register_device(data->grip_input_dev);
	if (iRet < 0) {
		input_free_device(data->grip_input_dev);
		goto err_initialize_grip_input_dev;
	}
	input_set_drvdata(data->grip_input_dev, data);
#endif

	data->temp_humi_input_dev = input_allocate_device();
	if (data->temp_humi_input_dev == NULL)
		goto err_initialize_temp_humi_input_dev;

	data->temp_humi_input_dev->name = "temp_humidity_sensor";
	input_set_capability(data->temp_humi_input_dev, EV_REL, REL_HWHEEL);
	input_set_capability(data->temp_humi_input_dev, EV_REL, REL_DIAL);
	input_set_capability(data->temp_humi_input_dev, EV_REL, REL_WHEEL);
	iRet = input_register_device(data->temp_humi_input_dev);
	if (iRet < 0) {
		input_free_device(data->temp_humi_input_dev);
		goto err_initialize_temp_humi_input_dev;
	}
	input_set_drvdata(data->temp_humi_input_dev, data);

	data->gesture_input_dev = input_allocate_device();
	if (data->gesture_input_dev == NULL)
		goto err_initialize_gesture_input_dev;

	data->gesture_input_dev->name = "gesture_sensor";
	input_set_capability(data->gesture_input_dev, EV_ABS, ABS_X);
	input_set_abs_params(data->gesture_input_dev, ABS_X, 0, 1024, 0, 0);
	input_set_capability(data->gesture_input_dev, EV_ABS, ABS_Y);
	input_set_abs_params(data->gesture_input_dev, ABS_Y, 0, 1024, 0, 0);
	input_set_capability(data->gesture_input_dev, EV_ABS, ABS_Z);
	input_set_abs_params(data->gesture_input_dev, ABS_Z, 0, 1024, 0, 0);
	input_set_capability(data->gesture_input_dev, EV_ABS, ABS_RX);
	input_set_abs_params(data->gesture_input_dev, ABS_RX, 0, 1024, 0, 0);
	input_set_capability(data->gesture_input_dev, EV_ABS, ABS_RY);
	input_set_abs_params(data->gesture_input_dev, ABS_RY, 0, 1024, 0, 0);
	input_set_capability(data->gesture_input_dev, EV_ABS, ABS_RZ);
	input_set_abs_params(data->gesture_input_dev, ABS_RZ, 0, 1024, 0, 0);
	input_set_capability(data->gesture_input_dev, EV_ABS, ABS_THROTTLE);
	input_set_abs_params(data->gesture_input_dev, ABS_THROTTLE, 0, 1024, 0, 0);
	input_set_capability(data->gesture_input_dev, EV_ABS, ABS_RUDDER);
	input_set_abs_params(data->gesture_input_dev, ABS_RUDDER, 0, 1024, 0, 0);
	input_set_capability(data->gesture_input_dev, EV_ABS, ABS_WHEEL);
	input_set_abs_params(data->gesture_input_dev, ABS_WHEEL, 0, 1024, 0, 0);
	input_set_capability(data->gesture_input_dev, EV_ABS, ABS_GAS);
	input_set_abs_params(data->gesture_input_dev, ABS_GAS, 0, 1024, 0, 0);
	input_set_capability(data->gesture_input_dev, EV_ABS, ABS_BRAKE);
	input_set_abs_params(data->gesture_input_dev, ABS_BRAKE, 0, 1024, 0, 0);
	input_set_capability(data->gesture_input_dev, EV_ABS, ABS_HAT0X);
	input_set_abs_params(data->gesture_input_dev, ABS_HAT0X, 0, 1024, 0, 0);
	input_set_capability(data->gesture_input_dev, EV_ABS, ABS_HAT0Y);
	input_set_abs_params(data->gesture_input_dev, ABS_HAT0Y, 0, 1024, 0, 0);
	input_set_capability(data->gesture_input_dev, EV_ABS, ABS_HAT1X);
	input_set_abs_params(data->gesture_input_dev, ABS_HAT1X, 0, 1024, 0, 0);
	input_set_capability(data->gesture_input_dev, EV_ABS, ABS_HAT1Y);
	input_set_abs_params(data->gesture_input_dev, ABS_HAT1Y, 0, 1024, 0, 0);
	input_set_capability(data->gesture_input_dev, EV_ABS, ABS_HAT2X);
	input_set_abs_params(data->gesture_input_dev, ABS_HAT2X, 0, 1024, 0, 0);
	input_set_capability(data->gesture_input_dev, EV_ABS, ABS_HAT2Y);
	input_set_abs_params(data->gesture_input_dev, ABS_HAT2Y, 0, 1024, 0, 0);
	input_set_capability(data->gesture_input_dev, EV_ABS, ABS_HAT3X);
	input_set_abs_params(data->gesture_input_dev, ABS_HAT3X, 0, 1024, 0, 0);
	input_set_capability(data->gesture_input_dev, EV_ABS, ABS_HAT3Y);
	input_set_abs_params(data->gesture_input_dev, ABS_HAT3Y, 0, 1024, 0, 0);
	input_set_capability(data->gesture_input_dev, EV_ABS, ABS_PRESSURE);
	input_set_abs_params(data->gesture_input_dev, ABS_PRESSURE, 0, 1024, 0, 0);
	iRet = input_register_device(data->gesture_input_dev);
	if (iRet < 0) {
		input_free_device(data->gesture_input_dev);
		goto err_initialize_gesture_input_dev;
	}
	input_set_drvdata(data->gesture_input_dev, data);

	data->sig_motion_input_dev = input_allocate_device();
	if (data->sig_motion_input_dev == NULL)
		goto err_initialize_sig_motion_input_dev;

	data->sig_motion_input_dev->name = "sig_motion_sensor";
	input_set_capability(data->sig_motion_input_dev, EV_REL, REL_MISC);
	iRet = input_register_device(data->sig_motion_input_dev);
	if (iRet < 0) {
		input_free_device(data->sig_motion_input_dev);
		goto err_initialize_sig_motion_input_dev;
	}
	input_set_drvdata(data->sig_motion_input_dev, data);

	data->step_cnt_input_dev = input_allocate_device();
	if (data->step_cnt_input_dev == NULL)
		goto err_initialize_step_cnt_input_dev;

	data->step_cnt_input_dev->name = "step_cnt_sensor";
	input_set_capability(data->step_cnt_input_dev, EV_REL, REL_MISC);
	iRet = input_register_device(data->step_cnt_input_dev);
	if (iRet < 0) {
		input_free_device(data->step_cnt_input_dev);
		goto err_initialize_step_cnt_input_dev;
	}
	input_set_drvdata(data->step_cnt_input_dev, data);

#ifdef CONFIG_SENSORS_SSP_INTERRUPT_GYRO_SENSOR
	data->interrupt_gyro_input_dev = input_allocate_device();
	if (data->interrupt_gyro_input_dev == NULL)
		goto err_initialize_interrupt_gyro_input_dev;

	data->interrupt_gyro_input_dev->name = "interrupt_gyro_sensor";
	input_set_capability(data->interrupt_gyro_input_dev, EV_REL, REL_RX);
	input_set_capability(data->interrupt_gyro_input_dev, EV_REL, REL_RY);
	input_set_capability(data->interrupt_gyro_input_dev, EV_REL, REL_RZ);
	iRet = input_register_device(data->interrupt_gyro_input_dev);
	if (iRet < 0) {
		input_free_device(data->interrupt_gyro_input_dev);
		goto err_initialize_interrupt_gyro_input_dev;
	}
	input_set_drvdata(data->interrupt_gyro_input_dev, data);
#endif

	data->meta_input_dev= input_allocate_device();
	if (data->meta_input_dev == NULL)
		goto err_initialize_meta_input_dev;

	data->meta_input_dev->name = "meta_event";
	input_set_capability(data->meta_input_dev, EV_REL, REL_HWHEEL);
	input_set_capability(data->meta_input_dev, EV_REL, REL_DIAL);
	iRet = input_register_device(data->meta_input_dev);
	if (iRet < 0) {
		input_free_device(data->meta_input_dev);
		goto err_initialize_meta_input_dev;
	}
	input_set_drvdata(data->meta_input_dev, data);

	data->tilt_indio_dev = iio_device_alloc(0);
	if (!data->tilt_indio_dev)
		goto err_alloc_tilt;

	data->tilt_indio_dev->name = "tilt_detector";
	data->tilt_indio_dev->dev.parent = &data->spi->dev;
	data->tilt_indio_dev->info = &tilt_info;
	data->tilt_indio_dev->channels = tilt_channels;
	data->tilt_indio_dev->num_channels = ARRAY_SIZE(tilt_channels);
	data->tilt_indio_dev->modes = INDIO_DIRECT_MODE;
	data->tilt_indio_dev->currentmode = INDIO_DIRECT_MODE;

	iRet = ssp_iio_configure_ring(data->tilt_indio_dev);
	if (iRet)
		goto err_config_ring_tilt;

	iRet = iio_buffer_register(data->tilt_indio_dev, data->tilt_indio_dev->channels,
					data->tilt_indio_dev->num_channels);
	if (iRet)
		goto err_register_buffer_tilt;

	iRet = iio_device_register(data->tilt_indio_dev);
	if (iRet)
		goto err_register_device_tilt;

	data->pickup_indio_dev = iio_device_alloc(0);
	if (!data->pickup_indio_dev)
		goto err_alloc_pickup;

	data->pickup_indio_dev->name = "pickup_gesture";
	data->pickup_indio_dev->dev.parent = &data->spi->dev;
	data->pickup_indio_dev->info = &pickup_info;
	data->pickup_indio_dev->channels = pickup_channels;
	data->pickup_indio_dev->num_channels = ARRAY_SIZE(pickup_channels);
	data->pickup_indio_dev->modes = INDIO_DIRECT_MODE;
	data->pickup_indio_dev->currentmode = INDIO_DIRECT_MODE;

	iRet = ssp_iio_configure_ring(data->pickup_indio_dev);
	if (iRet)
		goto err_config_ring_pickup;

	iRet = iio_buffer_register(data->pickup_indio_dev, data->pickup_indio_dev->channels,
					data->pickup_indio_dev->num_channels);
	if (iRet)
		goto err_register_buffer_pickup;

	iRet = iio_device_register(data->pickup_indio_dev);
	if (iRet)
		goto err_register_device_pickup;

	return SUCCESS;

err_register_device_pickup:
	pr_err("[SSP]: failed to register pickup device\n");
	iio_buffer_unregister(data->pickup_indio_dev);
err_register_buffer_pickup:
	pr_err("[SSP]: failed to register pickup buffer\n");
	ssp_iio_unconfigure_ring(data->pickup_indio_dev);
err_config_ring_pickup:
	pr_err("[SSP]: failed to configure pickup ring buffer\n");
	iio_device_free(data->pickup_indio_dev);
err_alloc_pickup:
	pr_err("[SSP]: failed to allocate memory for iio pickup device\n");
	iio_device_unregister(data->tilt_indio_dev);
err_register_device_tilt:
	pr_err("[SSP]: failed to register tilt device\n");
	iio_buffer_unregister(data->tilt_indio_dev);
err_register_buffer_tilt:
	pr_err("[SSP]: failed to register tilt buffer\n");
	ssp_iio_unconfigure_ring(data->tilt_indio_dev);
err_config_ring_tilt:
	pr_err("[SSP]: failed to configure tilt ring buffer\n");
	iio_device_free(data->tilt_indio_dev);
err_alloc_tilt:
	pr_err("[SSP]: failed to allocate memory for iio tilt device\n");
	input_unregister_device(data->meta_input_dev);
err_initialize_meta_input_dev:
	pr_err("[SSP]: %s - could not allocate meta event input device\n", __func__);
#ifdef CONFIG_SENSORS_SSP_INTERRUPT_GYRO_SENSOR
    input_unregister_device(data->interrupt_gyro_input_dev);
err_initialize_interrupt_gyro_input_dev:
    pr_err("[SSP]: %s - could not allocate interrupt_gyro input device\n", __func__);
#endif
	input_unregister_device(data->step_cnt_input_dev);
err_initialize_step_cnt_input_dev:
	pr_err("[SSP]: %s - could not allocate step cnt input device\n", __func__);
	input_unregister_device(data->sig_motion_input_dev);
err_initialize_sig_motion_input_dev:
	pr_err("[SSP]: %s - could not allocate sig motion input device\n", __func__);
	input_unregister_device(data->gesture_input_dev);
err_initialize_gesture_input_dev:
	pr_err("[SSP]: %s - could not allocate gesture input device\n", __func__);
	input_unregister_device(data->temp_humi_input_dev);
err_initialize_temp_humi_input_dev:
	pr_err("[SSP]: %s - could not allocate temp_humi input device\n", __func__);
#ifdef CONFIG_SENSORS_SSP_SX9306
	input_unregister_device(data->grip_input_dev);
err_initialize_grip_input_dev:
	pr_err("[SSP]: %s - could not allocate grip input device\n", __func__);
#endif
	input_unregister_device(data->prox_input_dev);
err_initialize_proximity_input_dev:
	pr_err("[SSP]: %s - could not allocate proximity input device\n", __func__);
#ifdef CONFIG_SENSORS_SSP_IRDATA_FOR_CAMERA
	input_unregister_device(data->light_ir_input_dev);
err_initialize_light_ir_input_dev:
	pr_err("[SSP]: %s - could not allocate light ir input device\n", __func__);
#endif
	input_unregister_device(data->light_input_dev);
err_initialize_light_input_dev:
	pr_err("[SSP]: %s - could not allocate light input device\n", __func__);
	iio_device_unregister(data->pressure_indio_dev);
err_register_device_pressure:
	pr_err("[SSP]: failed to register pressure_sensor device\n");
	iio_buffer_unregister(data->pressure_indio_dev);
err_register_buffer_pressure:
	pr_err("[SSP]: failed to register pressure_sensor buffer\n");
	ssp_iio_unconfigure_ring(data->pressure_indio_dev);
err_config_ring_pressure:
	pr_err("[SSP]: failed to configure pressure_sensor ring buffer\n");
	iio_device_free(data->pressure_indio_dev);
err_alloc_pressure:
	pr_err("[SSP]: failed to allocate memory for iio pressure_sensor device\n");
	iio_device_unregister(data->step_det_indio_dev);
err_register_device_step_det:
	pr_err("[SSP]: failed to register step_det device\n");
	iio_buffer_unregister(data->step_det_indio_dev);
err_register_buffer_step_det:
	pr_err("[SSP]: failed to register step_det buffer\n");
	ssp_iio_unconfigure_ring(data->step_det_indio_dev);
err_config_ring_step_det:
	pr_err("[SSP]: failed to configure step_det ring buffer\n");
	iio_device_free(data->step_det_indio_dev);
err_alloc_step_det:
	pr_err("[SSP]: failed to allocate memory for iio step_det device\n");
	iio_device_unregister(data->rot_indio_dev);
err_register_device_rot:
	pr_err("[SSP]: failed to register rot device\n");
	iio_buffer_unregister(data->rot_indio_dev);
err_register_buffer_rot:
	pr_err("[SSP]: failed to register rot buffer\n");
	ssp_iio_unconfigure_ring(data->rot_indio_dev);
err_config_ring_rot:
	pr_err("[SSP]: failed to configure rot ring buffer\n");
	iio_device_free(data->rot_indio_dev);
err_alloc_rot:
	pr_err("[SSP]: failed to allocate memory for iio rot device\n");
	iio_device_unregister(data->game_rot_indio_dev);
err_register_device_game_rot:
	pr_err("[SSP]: failed to register game_rot device\n");
	iio_buffer_unregister(data->game_rot_indio_dev);
err_register_buffer_game_rot:
	pr_err("[SSP]: failed to register game_rot buffer\n");
	ssp_iio_unconfigure_ring(data->game_rot_indio_dev);
err_config_ring_game_rot:
	pr_err("[SSP]: failed to configure game_rot ring buffer\n");
	iio_device_free(data->game_rot_indio_dev);
err_alloc_game_rot:
	pr_err("[SSP]: failed to allocate memory for iio game_rot device\n");
	iio_device_unregister(data->uncal_mag_indio_dev);
err_register_device_uncal_mag:
	pr_err("[SSP]: failed to register uncal mag device\n");
	iio_buffer_unregister(data->uncal_mag_indio_dev);
err_register_buffer_uncal_mag:
	pr_err("[SSP]: failed to register uncal mag buffer\n");
	ssp_iio_unconfigure_ring(data->uncal_mag_indio_dev);
err_config_ring_uncal_mag:
	pr_err("[SSP]: failed to configure uncal mag ring buffer\n");
	iio_device_free(data->uncal_mag_indio_dev);
err_alloc_uncal_mag:
	pr_err("[SSP]: failed to allocate memory for iio uncal mag device\n");
	iio_device_unregister(data->mag_indio_dev);
err_register_device_mag:
	pr_err("[SSP]: failed to register mag device\n");
	iio_buffer_unregister(data->mag_indio_dev);
err_register_buffer_mag:
	pr_err("[SSP]: failed to register mag buffer\n");
	ssp_iio_unconfigure_ring(data->mag_indio_dev);
err_config_ring_mag:
	pr_err("[SSP]: failed to configure mag ring buffer\n");
	iio_device_free(data->mag_indio_dev);
err_alloc_mag:
	pr_err("[SSP]: failed to allocate memory for iio uncal mag device\n");
	iio_device_unregister(data->uncal_gyro_indio_dev);
err_register_device_uncal_gyro:
	pr_err("[SSP]: failed to register uncal gyro device\n");
	iio_buffer_unregister(data->uncal_gyro_indio_dev);
err_register_buffer_uncal_gyro:
	pr_err("[SSP]: failed to register uncal gyro buffer\n");
	ssp_iio_unconfigure_ring(data->uncal_gyro_indio_dev);
err_config_ring_uncal_gyro:
	pr_err("[SSP]: failed to configure uncal gyro ring buffer\n");
	iio_device_free(data->uncal_gyro_indio_dev);
err_alloc_uncal_gyro:
	pr_err("[SSP]: failed to allocate memory for iio uncal gyro device\n");
	iio_device_unregister(data->gyro_indio_dev);
err_register_device_gyro:
	pr_err("[SSP]: failed to register gyro device\n");
	iio_buffer_unregister(data->gyro_indio_dev);
err_register_buffer_gyro:
	pr_err("[SSP]: failed to register gyro buffer\n");
	ssp_iio_unconfigure_ring(data->gyro_indio_dev);
err_config_ring_gyro:
	pr_err("[SSP]: failed to configure gyro ring buffer\n");
	iio_device_free(data->gyro_indio_dev);
err_alloc_gyro:
	pr_err("[SSP]: failed to allocate memory for iio gyro device\n");
	iio_device_unregister(data->accel_indio_dev);
err_register_device_accel:
	pr_err("[SSP]: failed to register accel device\n");
	iio_buffer_unregister(data->accel_indio_dev);
err_register_buffer_accel:
	pr_err("[SSP]: failed to register accel buffer\n");
	ssp_iio_unconfigure_ring(data->accel_indio_dev);
err_config_ring_accel:
	pr_err("[SSP]: failed to configure accel ring buffer\n");
	iio_device_free(data->accel_indio_dev);
err_alloc_accel:
	pr_err("[SSP]: failed to allocate memory for iio accel device\n");
	return ERROR;
}

void remove_input_dev(struct ssp_data *data)
{
	input_unregister_device(data->gesture_input_dev);
	input_unregister_device(data->light_input_dev);
#ifdef CONFIG_SENSORS_SSP_IRDATA_FOR_CAMERA
	input_unregister_device(data->light_ir_input_dev);
#endif
	input_unregister_device(data->prox_input_dev);
	input_unregister_device(data->temp_humi_input_dev);
	input_unregister_device(data->sig_motion_input_dev);
	input_unregister_device(data->step_cnt_input_dev);
#ifdef CONFIG_SENSORS_SSP_INTERRUPT_GYRO_SENSOR
	input_unregister_device(data->interrupt_gyro_input_dev);
#endif
	input_unregister_device(data->meta_input_dev);
}
