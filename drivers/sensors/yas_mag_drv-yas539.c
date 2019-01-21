/*
 * Copyright (c) 2015 Yamaha Corporation
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

#include "yas.h"

#if YAS_MAG_DRIVER == YAS_MAG_DRIVER_YAS539

#define YAS539_REG_DIDR			(0x80)
#define YAS539_REG_CMDR			(0x81)
#define YAS539_REG_RCOILR		(0x82)
#define YAS539_REG_INTRVLR		(0x83)
#define YAS539_REG_RCAUTOR		(0x85)
#define YAS539_REG_AVRR			(0x87)
#define YAS539_REG_SRSTR		(0x90)
#define YAS539_REG_ADCCALR		(0x91)
#define YAS539_REG_TRMR			(0x9f)
#define YAS539_REG_RCDRVR		(0xa0)
#define YAS539_REG_RCCURR		(0xaa)
#define YAS539_REG_DATAR		(0xb0)
#define YAS539_REG_CALR			(0xc0)

#define YAS539_DATA_UNDERFLOW		(0)
#define YAS539_DATA_OVERFLOW		(32767)
#define YAS539_DEVICE_ID		(0x08)	/* YAS539 (MS-3S) */

#define YAS_X_OVERFLOW			(0x01)
#define YAS_X_UNDERFLOW			(0x02)
#define YAS_Y1_OVERFLOW			(0x04)
#define YAS_Y1_UNDERFLOW		(0x08)
#define YAS_Y2_OVERFLOW			(0x10)
#define YAS_Y2_UNDERFLOW		(0x20)
#define YAS_OVERFLOW	(YAS_X_OVERFLOW|YAS_Y1_OVERFLOW|YAS_Y2_OVERFLOW)
#define YAS_UNDERFLOW	(YAS_X_UNDERFLOW|YAS_Y1_UNDERFLOW|YAS_Y2_UNDERFLOW)

#define YAS539_MAG_STATE_NORMAL		(0)
#define YAS539_MAG_STATE_INIT_COIL	(1)
#define YAS539_MAG_INITCOIL_TIMEOUT	(1000)	/* msec */
#define YAS539_MAG_POWER_ON_RESET_TIME	(4000)	/* usec */
#define YAS539_MAG_SOFT_RESET_TIME	(150)	/* usec */
#define YAS539_MAG_NOTRANS_POSITION	(3)

#define YAS539_MAG_AVERAGE_16		(0)
#define YAS539_MAG_AVERAGE_64		(1)
#define YAS539_MAG_AVERAGE_128		(2)
#define YAS539_MAG_AVERAGE_256		(3)

#define YAS539_MAG_RCOIL_TIME0		(65)
#define YAS539_MAG_RCOIL_TIME3		(550)
#define YAS539_MAG_RCOIL_RETRY		(2)
#define YAS539_MAG_TEST_TIME		(5000) /* usec */

#define set_vector(to, from) \
	{int _l; for (_l = 0; _l < 3; _l++) (to)[_l] = (from)[_l]; }

struct yas_cdriver {
	int initialized;
	struct yas_driver_callback cbk;
	int measure_state;
	int invalid_data;
	uint32_t invalid_data_time;
	int position;
	int32_t delay;
	int enable;
	uint8_t dev_id;
	const int8_t *transform;
	int average;
	uint32_t current_time;
	uint16_t last_raw[4];
	uint16_t last_after_rcoil[3];
	int16_t overflow[3], underflow[3];
	struct yas_matrix static_matrix;
	int noise_rcoil_flag;
	int flowflag;
};

static const struct yas_matrix no_conversion
	= { {10000, 0, 0, 0, 10000, 0, 0, 0, 10000} };
static const int measure_time_worst[] = {682, 1210, 2387, 4752};

static const int8_t YAS539_TRANSFORMATION[][9] = {
	{ 0,  1,  0, -1,  0,  0,  0,  0,  1 },
	{-1,  0,  0,  0, -1,  0,  0,  0,  1 },
	{ 0, -1,  0,  1,  0,  0,  0,  0,  1 },
	{ 1,  0,  0,  0,  1,  0,  0,  0,  1 },
	{ 0, -1,  0, -1,  0,  0,  0,  0, -1 },
	{ 1,  0,  0,  0, -1,  0,  0,  0, -1 },
	{ 0,  1,  0,  1,  0,  0,  0,  0, -1 },
	{-1,  0,  0,  0,  1,  0,  0,  0, -1 },
};
static struct yas_cdriver driver;

static int yas_set_enable_wrap(int enable, int rcoil);
static int yas_set_enable(int enable);
static int single_read(int *busy, uint16_t *t, uint16_t *xy1y2, int *ouflow);

static int yas_open(void)
{
	if (driver.cbk.device_open(YAS_TYPE_MAG) < 0)
		return YAS_ERROR_DEVICE_COMMUNICATION;
	driver.cbk.usleep(YAS539_MAG_POWER_ON_RESET_TIME);
	return YAS_NO_ERROR;
}
#define yas_read(a, b, c) \
	(driver.cbk.device_read(YAS_TYPE_MAG, (a), (b), (c)))
static int yas_single_write(uint8_t addr, uint8_t data)
{
	return driver.cbk.device_write(YAS_TYPE_MAG, addr, &data, 1);
}

static void apply_matrix(struct yas_vector *xyz, struct yas_matrix *m)
{
	int32_t tmp[3];
	int i;
	if (m == NULL)
		return;
	for (i = 0; i < 3; i++)
		tmp[i] = ((m->m[i*3]/10) * (xyz->v[0]/10)
				+ (m->m[i*3+1]/10) * (xyz->v[1]/10)
				+ (m->m[i*3+2]/10) * (xyz->v[2]/10)) / 100;
	for (i = 0; i < 3; i++)
		xyz->v[i] = tmp[i];
}

static uint32_t curtime(void)
{
	if (driver.cbk.current_time)
		return driver.cbk.current_time();
	else
		return driver.current_time;
}

static int invalid_magnetic_field(uint16_t *cur, uint16_t *last)
{
	int16_t invalid_thresh[] = {2000, 2000, 2000};
	int i;
	for (i = 0; i < 3; i++)
		if (invalid_thresh[i] <= ABS(cur[i] - last[i]))
			return 1;
	return 0;
}

static int start_yas539(int tcsen, int cont)
{
	uint8_t data = 0x81;
	data = (uint8_t)(data | (cont<<5));
	data = (uint8_t)(data | (tcsen<<6));
	if (yas_single_write(YAS539_REG_CMDR, data) < 0)
		return YAS_ERROR_DEVICE_COMMUNICATION;
	return YAS_NO_ERROR;
}

static int cont_start_yas539(void)
{
	if (start_yas539(0, 1) < 0)
		return YAS_ERROR_DEVICE_COMMUNICATION;
	/* wait for the first measurement */
	driver.cbk.usleep(measure_time_worst[driver.average]);
	return YAS_NO_ERROR;
}

static void sensitivity_correction(uint16_t *xy1y2, uint16_t t)
{
	int32_t h[3];
	int i;
	for (i = 0; i < 3; i++) {
		h[i] = (int32_t) xy1y2[i] - 16384;
		h[i] = 100000 * h[i] / (100000 - 11 * (t - 8170));
		xy1y2[i] = CLIP(h[i], -16384, 16383) + 16384;
	}
}

static int read_yas539(int *busy, uint16_t *t, uint16_t *xy1y2, int *ouflow)
{
	uint8_t data[8];
	int i;
	if (yas_read(YAS539_REG_DATAR, data, 8) < 0)
		return YAS_ERROR_DEVICE_COMMUNICATION;
	*t = (data[0]<<8) | data[1];
	*busy = data[2]>>7;
	xy1y2[0] = (uint16_t)(((data[2]&0x7f)<<8) | data[3]);
	xy1y2[1] = (uint16_t)(((data[4]&0x7f)<<8) | data[5]);
	xy1y2[2] = (uint16_t)(((data[6]&0x7f)<<8) | data[7]);
	*ouflow = 0;
	for (i = 0; i < 3; i++) {
		if (driver.overflow[i] <= xy1y2[i])
			*ouflow |= (1<<(i*2));
		if (xy1y2[i] <= driver.underflow[i])
			*ouflow |= (1<<(i*2+1));
		driver.last_raw[i] = xy1y2[i];
	}
	driver.last_raw[i] = *t;
	return YAS_NO_ERROR;
}

static int update_intrvlr(int32_t delay)
{
	int ds = (delay * 1000 - measure_time_worst[driver.average]) / 1000;
	int l, h;
	if (ds < 1)
		ds = 1;
	for (l = 0; l < 7; l++) {
		if (16 * (1<<l) < ds)
			continue;
		else
			break;
	}
	for (h = 15; 0 < h; h--) {
		if (ds < (h+1) * (1<<l))
			continue;
		else
			break;
	}
	if (yas_single_write(YAS539_REG_INTRVLR, (h<<3) | l) < 0)
		return YAS_ERROR_DEVICE_COMMUNICATION;
	return YAS_NO_ERROR;
}

static int update_avrr(int average)
{
	static const uint8_t avrr[] = {0x05, 0x2d, 0x2e, 0x2f};
	if (yas_single_write(YAS539_REG_AVRR, avrr[average]) < 0)
		return YAS_ERROR_DEVICE_COMMUNICATION;
	return YAS_NO_ERROR;
}

static int rcoil(int mode, int rnum)
{
	if (yas_single_write(YAS539_REG_RCDRVR, mode == 0 ? 0x00 : 0x01) < 0)
		return YAS_ERROR_DEVICE_COMMUNICATION;
	if (yas_single_write(YAS539_REG_RCOILR, 0x0a | ((rnum&0x3)<<6)) < 0)
		return YAS_ERROR_DEVICE_COMMUNICATION;
	if (rnum == 0)
		driver.cbk.usleep(YAS539_MAG_RCOIL_TIME0);
	else
		driver.cbk.usleep(YAS539_MAG_RCOIL_TIME3);
	return YAS_NO_ERROR;
}

static int rcoil_and_record_data(int flowflag, uint16_t *xy1y2, int *ouflow)
{
	uint16_t t;
	int rt, busy;

	if (flowflag) {
		if (rcoil(0, 0) < 0)
			return YAS_ERROR_DEVICE_COMMUNICATION;
		if (rcoil(1, 3) < 0)
			return YAS_ERROR_DEVICE_COMMUNICATION;
	} else {
		if (rcoil(1, 0) < 0)
			return YAS_ERROR_DEVICE_COMMUNICATION;
	}
	rt = single_read(&busy, &t, xy1y2, ouflow);
	if (rt < 0)
		return rt;
	if (busy)
		return YAS_ERROR_BUSY;
	sensitivity_correction(xy1y2, t);
	set_vector(driver.last_after_rcoil, xy1y2);
	return YAS_NO_ERROR;
}

static int enable_rcoil(void)
{
	uint16_t xy1y2[3], xy1y2_after[3], t;
	int rt, busy, overflow;

	rt = single_read(&busy, &t, xy1y2, &overflow);
	if (rt < 0)
		return rt;
	if (busy)
		return YAS_ERROR_BUSY;
	sensitivity_correction(xy1y2, t);
	if (!overflow) {
		rt = rcoil_and_record_data(0, xy1y2_after, &overflow);
		if (rt < 0)
			return rt;
		if (!overflow && !invalid_magnetic_field(xy1y2, xy1y2_after))
			return 1;
	}
	rt = rcoil_and_record_data(1, xy1y2, &overflow);
	if (rt < 0)
		return rt;
	if (overflow)
		return 0;
	return 1;
}

static int reset_yas539(void)
{
	if (yas_single_write(YAS539_REG_SRSTR, 0x80) < 0)
		return YAS_ERROR_DEVICE_COMMUNICATION;
	driver.cbk.usleep(YAS539_MAG_SOFT_RESET_TIME);
	if (yas_single_write(YAS539_REG_ADCCALR, 0x03) < 0)
		return YAS_ERROR_DEVICE_COMMUNICATION;
	if (yas_single_write(YAS539_REG_ADCCALR+1, 0xe0) < 0)
		return YAS_ERROR_DEVICE_COMMUNICATION;
	if (yas_single_write(YAS539_REG_TRMR, 0xff) < 0)
		return YAS_ERROR_DEVICE_COMMUNICATION;
	if (yas_single_write(YAS539_REG_RCAUTOR, 0x8a) < 0)
		return YAS_ERROR_DEVICE_COMMUNICATION;
	if (yas_single_write(YAS539_REG_RCCURR, 0x03) < 0)
		return YAS_ERROR_DEVICE_COMMUNICATION;
	if (update_intrvlr(driver.delay) < 0)
		return YAS_ERROR_DEVICE_COMMUNICATION;
	if (update_avrr(driver.average) < 0)
		return YAS_ERROR_DEVICE_COMMUNICATION;
	return YAS_NO_ERROR;
}

static int single_read(int *busy, uint16_t *t, uint16_t *xy1y2, int *ouflow)
{
	if (start_yas539(0, 0) < 0)
		return YAS_ERROR_DEVICE_COMMUNICATION;
	driver.cbk.usleep(measure_time_worst[driver.average]);
	if (read_yas539(busy, t, xy1y2, ouflow) < 0)
		return YAS_ERROR_DEVICE_COMMUNICATION;
	return YAS_NO_ERROR;
}

static void xy1y2_to_xyz(uint16_t *xy1y2, int32_t *xyz)
{
	xyz[0] = ((int32_t)xy1y2[0] - 16384) * 150;
	xyz[1] = ((int32_t)xy1y2[1] - xy1y2[2]) * 866 / 10;
	xyz[2] = ((int32_t)-xy1y2[1] - xy1y2[2] + 32768) * 150;
}

static int sensitivity_measuremnet(uint16_t *xy1y2)
{
	uint16_t t;
	int rt, busy, ouflow;
	rt = start_yas539(1, 0);
	if (rt < 0)
		return rt;
	driver.cbk.usleep(YAS539_MAG_TEST_TIME);
	rt = read_yas539(&busy, &t, xy1y2, &ouflow);
	if (rt < 0)
		return rt;
	if (busy)
		return YAS_ERROR_BUSY;
	return YAS_NO_ERROR;
}

static int yas_get_position(void)
{
	if (!driver.initialized)
		return YAS_ERROR_INITIALIZE;
	return driver.position;
}

static int yas_set_position(int position)
{
	if (!driver.initialized)
		return YAS_ERROR_INITIALIZE;
	if (position < 0 || 7 < position)
		return YAS_ERROR_ARG;
	if (position == YAS539_MAG_NOTRANS_POSITION)
		driver.transform = NULL;
	else
		driver.transform = YAS539_TRANSFORMATION[position];
	driver.position = position;
	return YAS_NO_ERROR;
}

static int yas_measure(struct yas_data *data, int num, int *ouflow)
{
	int32_t xyz_tmp[3];
	int i, busy, rt;
	uint16_t xy1y2[3], t;
	uint32_t tm;
	*ouflow = 0;
	if (!driver.initialized)
		return YAS_ERROR_INITIALIZE;
	if (data == NULL || num < 0)
		return YAS_ERROR_ARG;
	if (driver.cbk.current_time == NULL)
		driver.current_time += (uint32_t)driver.delay;
	if (num == 0)
		return 0;
	if (!driver.enable)
		return 0;
	if (driver.measure_state == YAS539_MAG_STATE_INIT_COIL) {
		tm = curtime();
		if (YAS539_MAG_INITCOIL_TIMEOUT
				<= tm - driver.invalid_data_time) {
			driver.invalid_data_time = tm;
			/* stop the continuous measurement */
			if (yas_single_write(YAS539_REG_CMDR, 0x00) < 0)
				return YAS_ERROR_DEVICE_COMMUNICATION;
			rt = rcoil_and_record_data(driver.flowflag, xy1y2,
					ouflow);
			if (rt < 0)
				return rt;
			if (!*ouflow) {
				driver.flowflag = 0;
				driver.invalid_data = 0;
				driver.measure_state = YAS539_MAG_STATE_NORMAL;
			}
			if (cont_start_yas539() < 0)
				return YAS_ERROR_DEVICE_COMMUNICATION;
		}
	}
	if (read_yas539(&busy, &t, xy1y2, ouflow) < 0)
		return YAS_ERROR_DEVICE_COMMUNICATION;
	sensitivity_correction(xy1y2, t);
	xy1y2_to_xyz(xy1y2, data->xyz.v);
	if (driver.transform != NULL) {
		for (i = 0; i < 3; i++) {
			xyz_tmp[i] = driver.transform[i*3] * data->xyz.v[0]
				+ driver.transform[i*3+1] * data->xyz.v[1]
				+ driver.transform[i*3+2] * data->xyz.v[2];
		}
		set_vector(data->xyz.v, xyz_tmp);
	}
	apply_matrix(&data->xyz, &driver.static_matrix);
	for (i = 0; i < 3; i++) {
		data->xyz.v[i] -= data->xyz.v[i] % 10;
		if (*ouflow & (1<<(i*2)))
			data->xyz.v[i] += 1; /* set overflow */
		if (*ouflow & (1<<(i*2+1)))
			data->xyz.v[i] += 2; /* set underflow */
	}
	tm = curtime();
	data->type = YAS_TYPE_MAG;
	if (driver.cbk.current_time)
		data->timestamp = tm;
	else
		data->timestamp = 0;
	data->accuracy = 0;
	if (busy)
		return YAS_ERROR_BUSY;
	if (*ouflow || invalid_magnetic_field(xy1y2, driver.last_after_rcoil)) {
		if (!driver.invalid_data) {
			driver.invalid_data_time = tm;
			driver.invalid_data = 1;
		}
		if (*ouflow)
			driver.flowflag = 1;
		driver.measure_state = YAS539_MAG_STATE_INIT_COIL;
		for (i = 0; i < 3; i++) {
			if (!*ouflow)
				data->xyz.v[i] += 3;
		}
	}
	return 1;
}

static int yas_measure_wrap(struct yas_data *data, int num)
{
	int ouflow;
	return yas_measure(data, num, &ouflow);
}

static int yas_get_delay(void)
{
	if (!driver.initialized)
		return YAS_ERROR_INITIALIZE;
	return driver.delay;
}

static int yas_set_delay(int delay)
{
	if (!driver.initialized)
		return YAS_ERROR_INITIALIZE;
	if (delay < 0)
		return YAS_ERROR_ARG;
	driver.delay = delay;
	if (!driver.enable)
		return YAS_NO_ERROR;
	/* stop the continuous measurement */
	if (yas_single_write(YAS539_REG_CMDR, 0x00) < 0)
		return YAS_ERROR_DEVICE_COMMUNICATION;
	if (update_intrvlr(driver.delay) < 0)
		return YAS_ERROR_DEVICE_COMMUNICATION;
	if (cont_start_yas539() < 0)
		return YAS_ERROR_DEVICE_COMMUNICATION;
	return YAS_NO_ERROR;
}

static int yas_get_enable(void)
{
	if (!driver.initialized)
		return YAS_ERROR_INITIALIZE;
	return driver.enable;
}

static int yas_set_enable_wrap(int enable, int rcoil)
{
	int rt;
	if (!driver.initialized)
		return YAS_ERROR_INITIALIZE;
	enable = !!enable;
	if (driver.enable == enable)
		return YAS_NO_ERROR;
	if (enable) {
		if (yas_open() < 0)
			return YAS_ERROR_DEVICE_COMMUNICATION;
		rt = reset_yas539();
		if (rt < 0) {
			driver.cbk.device_close(YAS_TYPE_MAG);
			return rt;
		}
		if (rcoil) {
			rt = enable_rcoil();
			if (rt < 0) {
				driver.cbk.device_close(YAS_TYPE_MAG);
				return rt;
			}
			if (rt) {
				driver.flowflag = 0;
				driver.invalid_data = 0;
				driver.measure_state = YAS539_MAG_STATE_NORMAL;
			} else {
				driver.invalid_data_time = curtime();
				driver.flowflag = 1;
				driver.invalid_data = 1;
				driver.measure_state
					= YAS539_MAG_STATE_INIT_COIL;
			}
		}
		if (cont_start_yas539() < 0) {
			driver.cbk.device_close(YAS_TYPE_MAG);
			return YAS_ERROR_DEVICE_COMMUNICATION;
		}
	} else {
		yas_single_write(YAS539_REG_CMDR, 0x00);
		driver.cbk.device_close(YAS_TYPE_MAG);
	}
	driver.enable = enable;
	return YAS_NO_ERROR;
}

static int yas_set_enable(int enable)
{
	return yas_set_enable_wrap(enable, 1);
}

static int yas_ext(int32_t cmd, void *p)
{
	struct yas539_self_test_result *r;
	struct yas_data data;
	int32_t *xyz;
	int16_t *ouflow, *m;
	int8_t average;
	int rt, i, enable, overflow, busy, position;
	uint16_t xy1y2[3], t;
	if (!driver.initialized)
		return YAS_ERROR_INITIALIZE;
	if (p == NULL)
		return YAS_ERROR_ARG;
	switch (cmd) {
	case YAS539_SELF_TEST:
		r = (struct yas539_self_test_result *) p;
		r->id = driver.dev_id;
		enable = driver.enable;
		if (!enable) {
			if (yas_open() < 0)
				return YAS_ERROR_DEVICE_COMMUNICATION;
		}
		rt = reset_yas539();
		if (rt < 0)
			goto self_test_exit;
		rt = enable_rcoil();
		if (rt < 0)
			goto self_test_exit;
		rt = single_read(&busy, &t, xy1y2, &overflow);
		if (rt < 0)
			goto self_test_exit;
		if (busy) {
			rt = YAS_ERROR_BUSY;
			goto self_test_exit;
		}
		sensitivity_correction(xy1y2, t);
		xy1y2_to_xyz(xy1y2, r->xyz);
		for (i = 0; i < 3; i++)
			r->xyz[i] = r->xyz[i] / 1000;
		if (overflow & YAS_OVERFLOW) {
			rt = YAS_ERROR_OVERFLOW;
			goto self_test_exit;
		}
		if (overflow & YAS_UNDERFLOW) {
			rt = YAS_ERROR_UNDERFLOW;
			goto self_test_exit;
		}
		if (r->xyz[0] == 0 && r->xyz[1] == 0 && r->xyz[2] == 0) {
			rt = YAS_ERROR_DIRCALC;
			goto self_test_exit;
		}
		r->dir = 99;
		rt = sensitivity_measuremnet(r->sxy1y2);
		if (rt < 0)
			goto self_test_exit;
		rt = YAS_NO_ERROR;
self_test_exit:
		if (enable)
			cont_start_yas539();
		else
			driver.cbk.device_close(YAS_TYPE_MAG);
		return rt;
	case YAS539_SELF_TEST_NOISE:
		xyz = (int32_t *) p;
		enable = driver.enable;
		if (!enable) {
			if (driver.noise_rcoil_flag)
				rt = yas_set_enable_wrap(1, 1);
			else
				rt = yas_set_enable_wrap(1, 0);
			if (rt < 0)
				return rt;
			driver.noise_rcoil_flag = 0;
		}
		position = yas_get_position();
		yas_set_position(YAS539_MAG_NOTRANS_POSITION);
		rt = yas_measure(&data, 1, &overflow);
		yas_set_position(position);
		if (rt < 0) {
			if (!enable)
				yas_set_enable(0);
			return rt;
		}
		for (i = 0; i < 3; i++)
			xyz[i] = data.xyz.v[i] / 150;
		if (!enable)
			yas_set_enable(0);
		return YAS_NO_ERROR;
	case YAS539_GET_LAST_RAWDATA:
		for (i = 0; i < 4; i++)
			((uint16_t *) p)[i] = driver.last_raw[i];
		return YAS_NO_ERROR;
	case YAS539_GET_AVERAGE_SAMPLE:
		*(int8_t *) p = driver.average;
		return YAS_NO_ERROR;
	case YAS539_SET_AVERAGE_SAMPLE:
		average = *(int8_t *) p;
		if (average < YAS539_MAG_AVERAGE_16
				|| YAS539_MAG_AVERAGE_256 < average)
			return YAS_ERROR_ARG;
		driver.average = average;
		if (!driver.enable)
			return YAS_NO_ERROR;
		/* stop the continuous measurement */
		if (yas_single_write(YAS539_REG_CMDR, 0x00) < 0)
			return YAS_ERROR_DEVICE_COMMUNICATION;
		if (update_avrr(driver.average) < 0)
			return YAS_ERROR_DEVICE_COMMUNICATION;
		if (cont_start_yas539() < 0)
			return YAS_ERROR_DEVICE_COMMUNICATION;
		return YAS_NO_ERROR;
	case YAS539_GET_STATIC_MATRIX:
		m = (int16_t *) p;
		for (i = 0; i < 9; i++)
			m[i] = driver.static_matrix.m[i];
		return YAS_NO_ERROR;
	case YAS539_SET_STATIC_MATRIX:
		m = (int16_t *) p;
		for (i = 0; i < 9; i++)
			driver.static_matrix.m[i] = m[i];
		return YAS_NO_ERROR;
	case YAS539_GET_OUFLOW_THRESH:
		ouflow = (int16_t *) p;
		for (i = 0; i < 3; i++) {
			ouflow[i] = driver.overflow[i];
			ouflow[i+3] = driver.underflow[i];
		}
		return YAS_NO_ERROR;
	default:
		break;
	}
	return YAS_ERROR_ARG;
}

static int yas_init(void)
{
	static const int16_t h[] = {16000, 16256, 16512, 16768};
	int i, cal_valid = 0;
	uint8_t data[18];
	int32_t c[3], a[9], f[3];
	int32_t of[3], uf[3], eof[3], euf[3];
	uint8_t k;
	if (driver.initialized)
		return YAS_ERROR_INITIALIZE;
	if (yas_open() < 0)
		return YAS_ERROR_DEVICE_COMMUNICATION;
	if (yas_read(YAS539_REG_DIDR, data, 1) < 0) {
		driver.cbk.device_close(YAS_TYPE_MAG);
		return YAS_ERROR_DEVICE_COMMUNICATION;
	}
	driver.dev_id = data[0] & 0x7f;
	if (driver.dev_id != YAS539_DEVICE_ID) {
		driver.cbk.device_close(YAS_TYPE_MAG);
		return YAS_ERROR_CHIP_ID;
	}
	if (yas_single_write(YAS539_REG_SRSTR, 0x80) < 0) {
		driver.cbk.device_close(YAS_TYPE_MAG);
		return YAS_ERROR_DEVICE_COMMUNICATION;
	}
	driver.cbk.usleep(YAS539_MAG_SOFT_RESET_TIME);
	if (yas_read(YAS539_REG_CALR, data, 18) < 0) {
		driver.cbk.device_close(YAS_TYPE_MAG);
		return YAS_ERROR_DEVICE_COMMUNICATION;
	}
	for (i = 0; i < 13; i++)
		if (data[i] != 0)
			cal_valid = 1;
	if (!cal_valid) {
		driver.cbk.device_close(YAS_TYPE_MAG);
		return YAS_ERROR_CALREG;
	}
	driver.cbk.device_close(YAS_TYPE_MAG);

	c[0] = ((data[0]<<1) | (data[1]>>7)) - 256;
	c[1] = (((data[1]<<2)&0x1fc) | (data[2]>>6)) - 256;
	c[2] = (((data[2]<<3)&0x1f8) | (data[3]>>5)) - 256;
	a[0] = 128;
	a[1] = (((data[3]<<2)&0x7c) | (data[4]>>6)) - 64;
	a[2] = (((data[4]<<1)&0x7e) | (data[5]>>7)) - 64;
	a[3] = (((data[5]<<1)&0xfe) | (data[6]>>7)) - 128;
	a[4] = (((data[6]<<2)&0x1fc) | (data[7]>>6)) - 112;
	a[5] = (((data[7]<<1)&0x7e) | (data[8]>>7)) - 64;
	a[6] = (((data[8]<<1)&0xfe) | (data[9]>>7)) - 128;
	a[7] = (data[9]&0x7f) - 64;
	a[8] = (((data[10]<<1)&0x1fe) | (data[11]>>7)) - 112;
	k = data[11]&0x7f;
	f[0] = (data[12]>>4)&0x03;
	f[1] = (data[12]>>2)&0x03;
	f[2] = data[12]&0x03;
	for (i = 0; i < 3; i++) {
		eof[i] = 32768 - h[f[i]] - ABS(c[i]) * 225 / 8;
		euf[i] = h[f[i]] - ABS(c[i]) * 225 / 8;
	}
	of[0] = 16384 + k * (a[0] * eof[0] - ABS(a[1]) * eof[1]
			- ABS(a[2]) * eof[2]) / 8192;
	of[1] = 16384 + k * (-ABS(a[3]) * eof[0] + a[4] * eof[1]
			- ABS(a[5]) * eof[2]) / 8192;
	of[2] = 16384 + k * (-ABS(a[6]) * eof[0] - ABS(a[7]) * eof[1]
			+ a[8] * eof[2]) / 8192;
	uf[0] = 16384 - k * (a[0] * euf[0] - ABS(a[1]) * euf[1]
			- ABS(a[2]) * euf[2]) / 8192;
	uf[1] = 16384 - k * (-ABS(a[3]) * euf[0] + a[4] * euf[1]
			- ABS(a[5]) * euf[2]) / 8192;
	uf[2] = 16384 - k * (-ABS(a[6]) * euf[0] - ABS(a[7]) * euf[1]
			+ a[8] * euf[2]) / 8192;
	for (i = 0; i < 3; i++) {
		if (YAS539_DATA_OVERFLOW < of[i])
			driver.overflow[i] = YAS539_DATA_OVERFLOW;
		else
			driver.overflow[i] = (int16_t) of[i];
		if (uf[i] < YAS539_DATA_UNDERFLOW)
			driver.underflow[i] = YAS539_DATA_UNDERFLOW;
		else
			driver.underflow[i] = (int16_t) uf[i];
	}

	driver.measure_state = YAS539_MAG_STATE_NORMAL;
	if (driver.cbk.current_time)
		driver.current_time =  driver.cbk.current_time();
	else
		driver.current_time = 0;
	driver.invalid_data = 0;
	driver.invalid_data_time = driver.current_time;
	driver.position = YAS539_MAG_NOTRANS_POSITION;
	driver.delay = YAS_DEFAULT_SENSOR_DELAY;
	driver.enable = 0;
	driver.transform = NULL;
	driver.average = YAS539_MAG_AVERAGE_64;
	driver.noise_rcoil_flag = 1;
	driver.flowflag = 0;
	for (i = 0; i < 3; i++)
		driver.last_after_rcoil[i] = 0;
	for (i = 0; i < 4; i++)
		driver.last_raw[i] = 0;
	driver.static_matrix = no_conversion;
	driver.initialized = 1;
	return YAS_NO_ERROR;
}

static int yas_term(void)
{
	int rt;
	if (!driver.initialized)
		return YAS_ERROR_INITIALIZE;
	rt = yas_set_enable(0);
	driver.initialized = 0;
	return rt;
}

int yas_mag_driver_init(struct yas_mag_driver *f)
{
	if (f == NULL || f->callback.device_open == NULL
			|| f->callback.device_close == NULL
			|| f->callback.device_read == NULL
			|| f->callback.device_write == NULL
			|| f->callback.usleep == NULL)
		return YAS_ERROR_ARG;

	f->init = yas_init;
	f->term = yas_term;
	f->get_delay = yas_get_delay;
	f->set_delay = yas_set_delay;
	f->get_enable = yas_get_enable;
	f->set_enable = yas_set_enable;
	f->get_position = yas_get_position;
	f->set_position = yas_set_position;
	f->measure = yas_measure_wrap;
	f->ext = yas_ext;
	driver.cbk = f->callback;
	yas_term();
	return YAS_NO_ERROR;
}
#endif
