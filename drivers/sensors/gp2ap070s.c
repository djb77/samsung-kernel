/*
 * Copyright (C) 2010 Samsung Electronics. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/i2c.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/leds.h>
#include <linux/gpio.h>
#include <linux/wakelock.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/regulator/consumer.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/input.h>

#include <linux/sensor/sensors_core.h>
#include "gp2ap070s.h"

#ifdef TAG
#undef TAG
#define TAG "[PROX]"
#endif

#define I2C_M_WR        0 /* for i2c Write */

#define DEFAULT_HIGH_THD 150
#define DEFAULT_LOW_THD 100
#define CANCEL_HIGH_THD 80
#define CANCEL_LOW_THD 50
#define DEFAULT_OFFSET 0

#define CHIP_DEV_NAME   "GP2AP070S"
#define CHIP_DEV_VENDOR "SHARP"
#define MODULE_NAME     "proximity_sensor"

/* Intelligent Cancelation*/
#define PROXIMITY_CANCELATION
#ifdef PROXIMITY_CANCELATION
#define CANCELATION_FILE_PATH "/efs/FactoryApp/prox_cal"
enum {
	CAL_FAIL = 0,
	CAL_CANCELATION,
	CAL_SKIP,
};
#endif

#define PROX_READ_NUM   40
#undef PROXIMITY_FOR_TEST /* for HW to tune up */

enum {
	PS_COM1 = 0,
	PS_COM2,
	PS_COM3,
	PS_COM4,
	PS_PS1,
	PS_PS2,
	PS_PS3,
	PS_REG_NUM,
};

enum {
	REG_ADDR = 0,
	CMD,
};

static u16 ps_reg_init_setting[PS_REG_NUM][2] = {
	{REG_COM1, COM1_SD},
	{REG_COM2, COM2_INT_ALL_CLEAR},
	{REG_COM3, COM3_INT_PULSE},
	{REG_COM4, COM4_INTVAL33},
	{REG_PS1, PS1_RES14},
	{REG_PS2, (PS2_IS130 | PS2_SUM32)},
	{REG_PS3, (PS3_PRST3 | PS3_TGINTEN_PS1 | PS3_TGIRDRON0)},
};

struct gp2a_data {
	struct input_dev *proximity_input_dev;
	struct device *dev;
	struct i2c_client *i2c_client;
	struct wake_lock prx_wake_lock;
	struct hrtimer prox_timer;
	struct workqueue_struct *prox_wq;
	struct work_struct work_prox;
	struct regulator *vdd;
	struct regulator *vled;

	ktime_t prox_poll_delay;
	atomic_t prox_enable;
	int ps_gpio;
	int irq;

	u8 detect;
	u8 nondetect;
	u16 prox_thd_high;
	u16 prox_thd_low;
	u16 prox_offset;
	unsigned int prox_cal_result;

	int avg[3];

	int p_out;
	int default_low_thd;
	int default_high_thd;
	int prox_cancel_l;
	int prox_cancel_h;
	int default_trim;

	int vdd_always_on; /* 1: vdd is always on, 0: enable only when proximity is on */
	int vled_ldo; /*0: vled(anode) source regulator, other: get power by LDO control */
	int regulator_divided; /* 1: vdd & vled uses divided circuit, 0: vdd & vled uses seperate circuit */
};

enum {
	OFF = 0,
	ON,
};

static int proximity_vdd_onoff(struct device *dev, bool onoff)
{
	struct gp2a_data *data = dev_get_drvdata(dev);
	int ret;

	SENSOR_INFO("%s\n", (onoff) ? "on" : "off");

	if (!data->vdd) {
		SENSOR_INFO("VDD get regulator\n");
		data->vdd = devm_regulator_get(dev, "gp2a,vdd");
		if (IS_ERR(data->vdd)) {
			SENSOR_ERR("cannot get vdd\n");
			data->vdd = NULL;
			return -ENOMEM;
		}

		if (!regulator_get_voltage(data->vdd))
			regulator_set_voltage(data->vdd, 2850000, 2850000);
	}

	if (onoff) {
		if (regulator_is_enabled(data->vdd)) {
			SENSOR_INFO("Regulator already enabled\n");
			return 0;
		}

		ret = regulator_enable(data->vdd);
		if (ret)
			SENSOR_ERR("Failed to enable vdd.\n");
		usleep_range(10000, 11000);
	} else {
		ret = regulator_disable(data->vdd);
		if (ret)
			SENSOR_ERR("Failed to disable vdd.\n");
	}

	SENSOR_INFO("end\n");
	return 0;
}

static int proximity_vled_onoff(struct device *dev, bool onoff)
{
	struct gp2a_data *data = dev_get_drvdata(dev);
	int ret;

	SENSOR_INFO("%s, ldo:%d\n",
		(onoff) ? "on" : "off", data->vled_ldo);

	/* ldo control */
	if (data->vled_ldo) {
		gpio_set_value(data->vled_ldo, onoff);
		return 0;
	}

	/* regulator control */
	if (!data->vled) {
		SENSOR_INFO("VLED get regulator\n");
		data->vled = devm_regulator_get(dev, "gp2a,vled");
		if (IS_ERR(data->vled)) {
			SENSOR_ERR("cannot get vled\n");
			data->vled = NULL;
			return -ENOMEM;
		}
	}

	if (onoff) {
		if (regulator_is_enabled(data->vled)) {
			SENSOR_INFO("Regulator already enabled\n");
			return 0;
		}

		ret = regulator_enable(data->vled);
		if (ret)
			SENSOR_ERR("Failed to enable vled.\n");
		usleep_range(10000, 11000);
	} else {
		ret = regulator_disable(data->vled);
		if (ret)
			SENSOR_ERR("Failed to disable vled.\n");
	}

	return 0;
}

int gp2a_i2c_read(struct i2c_client *client, u8 reg, int len, u8 *val)
{
	u8 retry = 5;
	int ret;
	struct i2c_msg msgs[2];

	if ((client == NULL) || (!client->adapter))
		return -ENODEV;

	/* send slave address & command */
	msgs[0].addr = client->addr;
	msgs[0].flags = I2C_M_WR;
	msgs[0].len = 1;
	msgs[0].buf = &reg;

	/* read word data */
	msgs[1].addr = client->addr;
	msgs[1].flags = I2C_M_RD;
	msgs[1].len = len;
	msgs[1].buf = val;

	while (retry--) {
		ret = i2c_transfer(client->adapter, msgs, 2);
		if (ret >= 0)
			return ret;
	}
	SENSOR_ERR("i2c transfer error ret = %d\n", ret);
	return ret;
}

static int gp2a_i2c_read_byte(struct i2c_client *client, u8 reg)
{
	u8 value;
	int ret;

	ret = gp2a_i2c_read(client, reg, 1, &value);
	if (ret < 0)
		return ret;
	return value;
}

int gp2a_i2c_write(struct i2c_client *client, u8 reg, int len, u8 *val)
{
	int ret, index;
	int retry = 5;
	struct i2c_msg msg;
	unsigned char data[11];

	if ((client == NULL) || (!client->adapter))
		return -ENODEV;
	else if (len >= 10) {
		SENSOR_ERR("length %d exceeds 10\n", len);
		return -EINVAL;
	}

	data[0] = reg;
	for (index = 1; index <= len; index++)
		data[index] = val[index - 1];

	msg.addr = client->addr;
	msg.flags = I2C_M_WR;
	msg.len = len + 1;
	msg.buf = data;

	while (retry--) {
		ret = i2c_transfer(client->adapter, &msg, 1);
		if (ret >= 0)
			return ret;
	}

	SENSOR_ERR("i2c transfer error ret= %d\n", ret);
	return ret;
}

static int gp2a_i2c_write_byte(struct i2c_client *client, u8 reg, u8 value)
{
	int ret;

	ret = gp2a_i2c_write(client, reg, 1, &value);
	return ret;
}

static uint32_t gp2a_get_proximity_adc(struct gp2a_data *data)
{
	u8 value[2];
	int ret;

	ret = gp2a_i2c_read(data->i2c_client, REG_D0_LSB, 2, &value[0]);
	if (ret < 0) {
		SENSOR_ERR("fail, ret=%d\n", ret);
		return ret;
	}

	return (value[0] | (value[1] << 8));
}

static void gp2a_set_mode(struct gp2a_data *data, u8 onoff)
{
	int i, ret = 0;

	SENSOR_INFO("onoff = %d\n", onoff);
	if (onoff) {
		/* enable settings */
		for (i = 0; i < PS_REG_NUM; i++)
			ret += gp2a_i2c_write_byte(data->i2c_client,
				ps_reg_init_setting[i][REG_ADDR],
				ps_reg_init_setting[i][CMD]);

		/* PS mode */
		ret += gp2a_i2c_write_byte(data->i2c_client, REG_COM1,
			COM1_WAKEUP | COM1_PS);
	} else {
		/* disable settings */
		ret = gp2a_i2c_write_byte(data->i2c_client, REG_COM1, COM1_SD);
	}

	if (ret < 0)
		SENSOR_ERR("failed to set mode (%d)\n", ret);
}

static int32_t gp2a_set_data_offset(struct gp2a_data *data, u16 thd)
{
	u8 val[2];
	int ret;

	val[0] = thd & 0x00FF;
	val[1] = (thd & 0xFF00) >> 8;

	ret = gp2a_i2c_write(data->i2c_client, REG_OS_D0_LSB, 2, val);
	if (ret < 0)
		SENSOR_ERR("set low thd failed. %d\n", ret);
	else
		data->prox_offset = thd;

	SENSOR_INFO("offset = %d\n", data->prox_offset);
	return ret;
}

static int32_t gp2a_set_threshold_low(struct gp2a_data *data, u16 thd)
{
	u8 val[2];
	int ret;

	val[0] = thd & 0x00FF;
	val[1] = (thd & 0xFF00) >> 8;

	ret = gp2a_i2c_write(data->i2c_client, REG_PS_LT_LSB, 2, val);
	if (ret < 0)
		SENSOR_ERR("set low thd failed. %d\n", ret);
	else
		data->prox_thd_low = thd;

	SENSOR_INFO("thd = %d\n", data->prox_thd_low);
	return ret;
}

static int32_t gp2a_set_threshold_high(struct gp2a_data *data, u16 thd)
{
	u8 val[2];
	int ret;

	val[0] = thd & 0x00FF;
	val[1] = (thd & 0xFF00) >> 8;

	ret = gp2a_i2c_write(data->i2c_client, REG_PS_HT_LSB, 2, val);
	if (ret < 0)
		SENSOR_ERR("set low thd failed. %d\n", ret);
	else
		data->prox_thd_high = thd;

	SENSOR_INFO("thd = %d\n", data->prox_thd_high);
	return ret;
}

static ssize_t name_read(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", CHIP_DEV_NAME);
}

static ssize_t vendor_read(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", CHIP_DEV_VENDOR);
}

static ssize_t proximity_dhr_sensor_info_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct gp2a_data *data =  dev_get_drvdata(dev);
	int ret;
	u8 value[2];
	int low_thresh, hi_thresh;
	int ps_resolution, led_ctrl, persist_time, default_offset;

	ret = gp2a_i2c_read(data->i2c_client, REG_PS_HT_LSB, 2, &value[0]);
	if (ret < 0) {
		SENSOR_ERR("fail, ret=%d\n", ret);
		return ret;
	}
	hi_thresh = value[0] | (value[1] << 8);

	ret = gp2a_i2c_read(data->i2c_client, REG_PS_LT_LSB, 2, &value[0]);
	if (ret < 0) {
		SENSOR_ERR("fail, ret=%d\n", ret);
		return ret;
	}
	low_thresh = value[0] | (value[1] << 8);

	ps_resolution = gp2a_i2c_read_byte(data->i2c_client, REG_PS1);
	led_ctrl = gp2a_i2c_read_byte(data->i2c_client, REG_PS2);
	persist_time = gp2a_i2c_read_byte(data->i2c_client, REG_PS3);

	ret = gp2a_i2c_read(data->i2c_client, REG_OS_D0_LSB, 2, &value[0]);
	if (ret < 0) {
		SENSOR_ERR("fail, ret=%d\n", ret);
		return ret;
	}
	default_offset = value[0] | (value[1] << 8);

	return snprintf(buf, PAGE_SIZE,
			"\"THD\":\"%d %d\","\
			"\"PS_RESOLUTION\":\"0x%x\","\
			"\"LED_CTRL\":\"0x%x\","\
			"\"PERSIST_TIME\":\"0x%x\","\
			"\"DEFAULT_OFFSET\":\"%d\","\
			"\"CANCEL_THD\":\"%d %d\"\n",
			hi_thresh, low_thresh,
			ps_resolution,
			led_ctrl,
			persist_time,
			default_offset,
			data->prox_cancel_h, data->prox_cancel_l);
}

#if defined(PROXIMITY_CANCELATION)
static int proximity_open_cancelation(struct gp2a_data *data)
{
	struct file *cal_filp = NULL;
	mm_segment_t old_fs;
	uint16_t file_offset_data;
	int ret;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(CANCELATION_FILE_PATH, O_RDONLY, 0);
	if (IS_ERR(cal_filp)) {
		ret = PTR_ERR(cal_filp);
		if (ret != -ENOENT)
			SENSOR_ERR("Can't open calibration file\n");
		set_fs(old_fs);
		return ret;
	}

	ret = cal_filp->f_op->read(cal_filp,
		(char *)&file_offset_data,
		sizeof(u16), &cal_filp->f_pos);
	if (ret != sizeof(u16)) {
		SENSOR_ERR("Can't read the cal data from file(%d)\n", ret);
		ret = -EIO;
	}

	if (file_offset_data != data->default_trim) {
		data->prox_offset = file_offset_data;
		gp2a_set_threshold_high(data, data->prox_cancel_h);
		gp2a_set_threshold_low(data, data->prox_cancel_l);
	}

	SENSOR_INFO("file_offset = %d, ps_offset = %d, default_trim = %d\n",
		file_offset_data, data->prox_offset,
		data->default_trim);

	filp_close(cal_filp, current->files);
	set_fs(old_fs);

	return ret;
}

static int proximity_store_cancelation(struct device *dev, bool do_calib)
{
	struct gp2a_data *data =  dev_get_drvdata(dev);
	struct file *cal_filp = NULL;
	mm_segment_t old_fs;
	u8 value[2];
	u16 ps_data = 0;
	int ret;

	if (do_calib) {
		SENSOR_INFO("start\n");
		ret = gp2a_i2c_read(data->i2c_client, REG_D0_LSB, 2,
			&value[0]);
		if (ret < 0) {
			SENSOR_ERR("read adc fail, ret=%d\n", ret);
			return ret;
		}
		ps_data = (value[0] | (value[1] << 8));
		SENSOR_INFO("raw data =  %d\n", ps_data);

		if (ps_data * 10 < (data->default_low_thd * 6)) {
			data->prox_offset = data->default_trim;
			SENSOR_INFO("skip calibration = %d, crosstalk <\n",
				ps_data);
			data->prox_cal_result = CAL_SKIP;
		} else if (ps_data <= data->default_high_thd) {
			data->prox_offset = ps_data + data->default_trim;
			SENSOR_INFO("do calibration, crosstalk_offset = %u", ps_data);
			data->prox_cal_result = CAL_CANCELATION;
		} else {
			data->prox_offset = data->default_trim;
			SENSOR_INFO("fail calibration = %d, crosstalk >\n",
				ps_data);
			data->prox_cal_result = CAL_FAIL;
		}

		if (data->prox_cal_result == CAL_CANCELATION) {
			data->prox_thd_high = data->prox_cancel_h;
			data->prox_thd_low = data->prox_cancel_l;
		} else {
			data->prox_thd_high = data->default_high_thd;
			data->prox_thd_low = data->default_low_thd;
		}
	} else { /*reset*/
		SENSOR_INFO("reset\n");
		data->prox_offset = data->default_trim;
		data->prox_thd_high = data->default_high_thd;
		data->prox_thd_low = data->default_low_thd;
	}

	if ((data->prox_cal_result == CAL_CANCELATION) || !do_calib) {
		ret = gp2a_set_data_offset(data, data->prox_offset);
		if (ret < 0)
			SENSOR_ERR("fail : set proximity offset(%d)\n", ret);

		ret = gp2a_set_threshold_high(data, data->prox_thd_high);
		if (ret < 0)
			SENSOR_ERR("fail : set proximity high thd(%d)\n", ret);

		ret = gp2a_set_threshold_low(data, data->prox_thd_low);
		if (ret < 0)
			SENSOR_ERR("fail : set proximity low thd(%d)\n", ret);
	}

	SENSOR_INFO("prox_offset = 0x%x, high_thd = 0x%x, low_thd = 0x%x\n",
		data->prox_offset,
		data->prox_thd_high,
		data->prox_thd_low);

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(CANCELATION_FILE_PATH,
		O_CREAT | O_TRUNC | O_WRONLY | O_SYNC, 0660);
	if (IS_ERR(cal_filp)) {
		SENSOR_ERR("Can't open calibration file\n");
		set_fs(old_fs);
		ret = PTR_ERR(cal_filp);
		return ret;
	}

	ret = cal_filp->f_op->write(cal_filp,
		(char *)&data->prox_offset,
		sizeof(u16), &cal_filp->f_pos);
	if (ret != sizeof(u16)) {
		SENSOR_ERR("Can't write the cancel data to file\n");
		ret = -EIO;
	}

	filp_close(cal_filp, current->files);
	set_fs(old_fs);

	if (!do_calib) /* delay for clearing */
		msleep(150);

	return ret;
}

static ssize_t proximity_cancel_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	bool do_calib;
	int err;

	if (sysfs_streq(buf, "1")) /* calibrate cancelation value */
		do_calib = true;
	else if (sysfs_streq(buf, "0")) /* reset cancelation value */
		do_calib = false;
	else {
		SENSOR_ERR("invalid value %d\n", *buf);
		return size;
	}

	err = proximity_store_cancelation(dev, do_calib);
	if (err < 0) {
		SENSOR_ERR("proximity_store_cancelation() failed(%d)\n", err);
		return size;
	}

	return size;
}

static ssize_t proximity_cancel_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct gp2a_data *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%u,%u,%u\n",
		data->prox_offset,
		(data->prox_offset != data->default_trim) ? data->prox_cancel_h : data->prox_thd_high,
		(data->prox_offset != data->default_trim) ? data->prox_cancel_l : data->prox_thd_low);
}

static ssize_t proximity_cancel_pass_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct gp2a_data *data = dev_get_drvdata(dev);

	SENSOR_INFO("%u\n", data->prox_cal_result);
	return snprintf(buf, PAGE_SIZE, "%u\n", data->prox_cal_result);
}
#endif

#if defined(PROXIMITY_FOR_TEST)
static ssize_t proximity_register_write_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int regist = 0, val = 0;
	struct gp2a_data *data = dev_get_drvdata(dev);

	if (sscanf(buf, "%2x,%2x", &regist, &val) != 2) {
		SENSOR_ERR("The number of data are wrong\n");
		return count;
	}

	gp2a_i2c_write_byte(data->i2c_client, regist, val);
	SENSOR_INFO("Register(0x%2x) 8:data(0x%2x) 10:%d\n",
			regist, val, val);

	return count;
}

static ssize_t proximity_register_read_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	u8 val[PS_REG_NUM], i;
	struct gp2a_data *data = dev_get_drvdata(dev);

	for (i = 0; i < PS_REG_NUM; i++) {
		val[i] = gp2a_i2c_read_byte(data->i2c_client,
				ps_reg_init_setting[i][REG_ADDR]);
		SENSOR_INFO("Register(0x%2x) data(0x%2x)\n",
			ps_reg_init_setting[i][REG_ADDR], val[i]);
	}

	return snprintf(buf, PAGE_SIZE, "0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x\n",
		val[0], val[1], val[2], val[3], val[4], val[5], val[6]);
}
#endif

static void proximity_get_avg_val(struct gp2a_data *data)
{
	int min = 0, max = 0, avg = 0;
	int i;
	u16 ps_data;

	for (i = 0; i < PROX_READ_NUM; i++) {
		msleep(40);
		ps_data = gp2a_get_proximity_adc(data);
		avg += ps_data;

		if (!i)
			min = ps_data;
		else if (ps_data < min)
			min = ps_data;

		if (ps_data > max)
			max = ps_data;
	}
	avg /= PROX_READ_NUM;

	data->avg[0] = min;
	data->avg[1] = avg;
	data->avg[2] = max;
}

static ssize_t proximity_avg_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct gp2a_data *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d\n", data->avg[0],
		data->avg[1], data->avg[2]);
}

static ssize_t proximity_avg_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct gp2a_data *data =  dev_get_drvdata(dev);
	bool new_value = false;

	if (sysfs_streq(buf, "1"))
		new_value = true;
	else if (sysfs_streq(buf, "0"))
		new_value = false;
	else {
		SENSOR_ERR("invalid value %d\n", *buf);
		return size;
	}

	SENSOR_INFO("average enable = %d\n", new_value);
	if (new_value) {
		if (atomic_read(&data->prox_enable) == OFF) {
			if (!data->vdd_always_on)
				proximity_vdd_onoff(dev, ON);
			if (!data->regulator_divided)
				proximity_vled_onoff(dev, ON);
			gp2a_set_mode(data, ON);
		}
		hrtimer_start(&data->prox_timer, data->prox_poll_delay,
			HRTIMER_MODE_REL);
	} else if (!new_value) {
		hrtimer_cancel(&data->prox_timer);
		cancel_work_sync(&data->work_prox);
		if (atomic_read(&data->prox_enable) == OFF) {
			gp2a_set_mode(data, OFF);
			if (!data->regulator_divided)
				proximity_vled_onoff(dev, OFF);
			if (!data->vdd_always_on)
				proximity_vdd_onoff(dev, OFF);
		}
	}

	return size;
}

static ssize_t proximity_state_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct gp2a_data *data = dev_get_drvdata(dev);
	uint32_t ps_data;

	ps_data = gp2a_get_proximity_adc(data);

	return snprintf(buf, PAGE_SIZE, "%d\n", ps_data);
}

static ssize_t proximity_thresh_high_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct gp2a_data *data = dev_get_drvdata(dev);
	int ret;
	u8 value[2];

	ret = gp2a_i2c_read(data->i2c_client, REG_PS_HT_LSB, 2, &value[0]);
	if (ret < 0) {
		SENSOR_ERR("fail, ret=%d\n", ret);
		return ret;
	}

	return snprintf(buf, PAGE_SIZE, "%d\n", (value[0] | (value[1] << 8)));
}

static ssize_t proximity_thresh_high_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct gp2a_data *data = dev_get_drvdata(dev);
	u16 thresh_value;
	int err;

	err = kstrtou16(buf, 10, &thresh_value);
	if (err < 0) {
		SENSOR_ERR("kstrtoint failed(%d)\n", err);
		return size;
	}

	if (thresh_value > 2) {
		gp2a_set_threshold_high(data, thresh_value);
		SENSOR_INFO("new high threshold = %d\n",
			data->prox_thd_high);
		msleep(150);
	} else
		SENSOR_ERR("wrong high threshold value(%d)\n", thresh_value);

	return size;
}

static ssize_t proximity_thresh_low_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct gp2a_data *data = dev_get_drvdata(dev);
	int ret;
	u8 value[2];

	ret = gp2a_i2c_read(data->i2c_client, REG_PS_LT_LSB, 2, &value[0]);
	if (ret < 0) {
		SENSOR_ERR("fail, ret=%d\n", ret);
		return ret;
	}

	return snprintf(buf, PAGE_SIZE, "%d\n", (value[0] | (value[1] << 8)));
}

static ssize_t proximity_thresh_low_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct gp2a_data *data = dev_get_drvdata(dev);
	u16 thresh_value;
	int err;

	err = kstrtou16(buf, 10, &thresh_value);
	if (err < 0)
		SENSOR_ERR("kstrtoint failed\n");

	if (thresh_value > 2) {
		gp2a_set_threshold_low(data, thresh_value);
		SENSOR_INFO("new low threshold = %d\n",
			data->prox_thd_low);
		msleep(150);
	} else
		SENSOR_ERR("wrong low threshold value(%d)\n", thresh_value);

	return size;
}

static DEVICE_ATTR(name, S_IRUGO, name_read, NULL);
static DEVICE_ATTR(vendor, S_IRUGO, vendor_read, NULL);
#if defined(PROXIMITY_CANCELATION)
static DEVICE_ATTR(prox_cal, S_IRUGO | S_IWUSR | S_IWGRP,
	proximity_cancel_show, proximity_cancel_store);
static DEVICE_ATTR(prox_offset_pass, S_IRUGO, proximity_cancel_pass_show,
	NULL);
#endif
#if defined(PROXIMITY_FOR_TEST)
static DEVICE_ATTR(prox_register, S_IRUGO | S_IWUSR | S_IWGRP,
	proximity_register_read_show, proximity_register_write_store);
#endif
static DEVICE_ATTR(prox_avg, S_IRUGO | S_IWUSR | S_IWGRP,
	proximity_avg_show, proximity_avg_store);
static DEVICE_ATTR(raw_data, S_IRUGO, proximity_state_show, NULL);
static DEVICE_ATTR(state, S_IRUGO, proximity_state_show, NULL);
static DEVICE_ATTR(thresh_high, S_IRUGO | S_IWUSR | S_IWGRP,
	proximity_thresh_high_show, proximity_thresh_high_store);
static DEVICE_ATTR(thresh_low, S_IRUGO | S_IWUSR | S_IWGRP,
	proximity_thresh_low_show, proximity_thresh_low_store);
static DEVICE_ATTR(dhr_sensor_info, S_IRUSR | S_IRGRP,
	proximity_dhr_sensor_info_show, NULL);

static struct device_attribute *proximity_attrs[] = {
	&dev_attr_name,
	&dev_attr_vendor,
#if defined(PROXIMITY_CANCELATION)
	&dev_attr_prox_cal,
	&dev_attr_prox_offset_pass,
#endif
#if defined(PROXIMITY_FOR_TEST)
	&dev_attr_prox_register,
#endif
	&dev_attr_prox_avg,
	&dev_attr_raw_data,
	&dev_attr_state,
	&dev_attr_thresh_high,
	&dev_attr_thresh_low,
	&dev_attr_dhr_sensor_info,
	NULL,
};

static ssize_t proximity_enable_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct gp2a_data *gp2a = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n",
			atomic_read(&gp2a->prox_enable));
}

static ssize_t proximity_enable_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct gp2a_data *data = dev_get_drvdata(dev);
	bool new_value;
	int pre_enable;

	if (sysfs_streq(buf, "1"))
		new_value = true;
	else if (sysfs_streq(buf, "0"))
		new_value = false;
	else {
		SENSOR_ERR("invalid value %d\n", *buf);
		return size;
	}

	pre_enable = atomic_read(&data->prox_enable);
	SENSOR_INFO("new_value = %d, pre_enable = %d\n",
		new_value, pre_enable);

	if (new_value && !pre_enable) {
#if defined(PROXIMITY_CANCELATION)
		int ret;
#endif
		if (!data->vdd_always_on)
			proximity_vdd_onoff(dev, ON);
		if (!data->regulator_divided)
			proximity_vled_onoff(dev, ON);

#if defined(PROXIMITY_CANCELATION)
		/* open cancelation data */
		ret = proximity_open_cancelation(data);
		if (ret < 0 && ret != -ENOENT)
			SENSOR_INFO("proximity_open_cancelation() failed\n");
		ret = gp2a_set_data_offset(data, data->prox_offset);
		if (ret < 0)
			SENSOR_ERR("fail : set proximity offset(%d)\n", ret);
#endif
		gp2a_set_mode(data, ON);

		atomic_set(&data->prox_enable, ON);
		/* 0 is close, 1 is far */
		input_report_abs(data->proximity_input_dev, ABS_DISTANCE, 1);
		input_sync(data->proximity_input_dev);

		enable_irq_wake(data->irq);
		msleep(200);
		enable_irq(data->irq);
	} else if (!new_value && pre_enable) {
		disable_irq(data->irq);
		disable_irq_wake(data->irq);
		gp2a_set_mode(data, OFF);

		atomic_set(&data->prox_enable, OFF);
		if (!data->regulator_divided)
			proximity_vled_onoff(dev, OFF);
		if (!data->vdd_always_on)
			proximity_vdd_onoff(dev, OFF);
	}
	SENSOR_INFO("enabled = %d\n", atomic_read(&data->prox_enable));

	return size;
}

static DEVICE_ATTR(enable, S_IRUGO | S_IWUSR | S_IWGRP,
	proximity_enable_show, proximity_enable_store);

static struct attribute *proximity_sysfs_attrs[] = {
	&dev_attr_enable.attr,
	NULL
};

static struct attribute_group proximity_attribute_group = {
	.attrs = proximity_sysfs_attrs,
};

static void gp2a_work_func_prox(struct work_struct *work)
{
	struct gp2a_data *data = container_of(work,
		struct gp2a_data, work_prox);
	proximity_get_avg_val(data);
}

static enum hrtimer_restart gp2a_prox_timer_func(struct hrtimer *timer)
{
	struct gp2a_data *data = container_of(timer,
		struct gp2a_data, prox_timer);

	queue_work(data->prox_wq, &data->work_prox);
	hrtimer_forward_now(&data->prox_timer, data->prox_poll_delay);
	return HRTIMER_RESTART;
}

/* interrupt happened due to transition/change of near/far proximity state */
irqreturn_t proximity_irq_thread_fn(int irq, void *data)
{
	struct gp2a_data *gp2a = data;
	u8 val;
	u16 ps_data;
	int enabled;

	enabled = atomic_read(&gp2a->prox_enable);
	val = gpio_get_value(gp2a->p_out);
	ps_data = gp2a_get_proximity_adc(gp2a);

	if (enabled) {
#ifdef CONFIG_SEC_FACTORY
		SENSOR_INFO("FACTORY: near/far=%d, ps data = %d\n",
				val, ps_data);
#else
		SENSOR_INFO("near/far=%d, ps data = %d\n",
				val, ps_data);
		if (((!val) && (ps_data >= gp2a->prox_thd_high)) ||
			(val && (ps_data <= gp2a->prox_thd_low)))
#endif
		{
			/* 0 is close, 1 is far */
			input_report_abs(gp2a->proximity_input_dev, ABS_DISTANCE,
				val);
			input_sync(gp2a->proximity_input_dev);
		}
	}
	wake_lock_timeout(&gp2a->prx_wake_lock, 3 * HZ);

	return IRQ_HANDLED;
}

static int setup_register_gp2a(struct gp2a_data *data)
{
	int ret, i;

	/* PS initialization */
	for (i = 0; i < PS_REG_NUM; i++) {
		ret = gp2a_i2c_write_byte(data->i2c_client,
			ps_reg_init_setting[i][REG_ADDR],
			ps_reg_init_setting[i][CMD]);
		if (ret < 0) {
			SENSOR_ERR("failed. %d\n", ret);
			return ret;
		}
	}

	/* SET threshold */
	ret = gp2a_set_threshold_low(data, data->default_low_thd);
	ret += gp2a_set_threshold_high(data, data->default_high_thd);
	if (ret < 0)
		SENSOR_ERR("set thd failed. %d\n", ret);

	/* SET data OFFSET(0x8C) */
	ret = gp2a_set_data_offset(data, data->default_trim);
	if (ret < 0)
		SENSOR_ERR("set data offset failed. %d\n", ret);

	return ret;
}

static int gp2a_setup_irq(struct gp2a_data *gp2a)
{
	int ret;

	ret = gpio_request(gp2a->p_out, "gpio_proximity_out");
	if (ret < 0) {
		SENSOR_ERR("gpio %d request failed (%d)\n", gp2a->p_out, ret);
		return ret;
	}

	ret = gpio_direction_input(gp2a->p_out);
	if (ret < 0) {
		SENSOR_ERR("failed gpio %d as input (%d)\n", gp2a->p_out, ret);
		goto err_gpio_direction_input;
	}

	gp2a->irq = gpio_to_irq(gp2a->p_out);
	ret = request_threaded_irq(gp2a->irq, NULL, proximity_irq_thread_fn,
		IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING | IRQF_ONESHOT,
		"proximity_int", gp2a);
	if (ret < 0) {
		SENSOR_ERR("request_irq(%d) failed for gpio %d (%d)\n",
			gp2a->irq, gp2a->p_out, ret);
		goto err_request_irq;
	}

	SENSOR_INFO("request_irq(%d) success for gpio %d (%d)\n",
		gp2a->irq, gp2a->ps_gpio, gp2a->p_out);

	disable_irq(gp2a->irq);

	goto done;

err_request_irq:
err_gpio_direction_input:
	gpio_free(gp2a->p_out);
done:
	return ret;
}

static int gp2a_input_init(struct gp2a_data *gp2a)
{
	int ret;
	struct input_dev *dev;

	/* Create the input device */
	dev = input_allocate_device();
	if (!dev)
		return -ENOMEM;

	dev->name = MODULE_NAME;
	dev->id.bustype = BUS_I2C;

	input_set_drvdata(dev, gp2a);
	input_set_capability(dev, EV_ABS, ABS_DISTANCE);
	input_set_abs_params(dev, ABS_DISTANCE, 0, 1, 0, 0);

	ret = input_register_device(dev);
	if (ret < 0) {
		SENSOR_ERR("could not register input device\n");
		input_free_device(dev);
		return ret;
	}

	ret = sensors_create_symlink(&dev->dev.kobj, dev->name);
	if (ret < 0) {
		SENSOR_ERR("create sysfs symlink error\n");
		input_unregister_device(dev);
		return ret;
	}

	ret = sysfs_create_group(&dev->dev.kobj, &proximity_attribute_group);
	if (ret < 0) {
		SENSOR_ERR("create sysfs group error\n");
		sensors_remove_symlink(&dev->dev.kobj, dev->name);
		input_unregister_device(dev);
		return ret;
	}

	/* save the input pointer and finish initialization */
	gp2a->proximity_input_dev = dev;

	return ret;
}

static int gp2a_parse_dt(struct device *dev, struct gp2a_data *gp2a)
{
	struct device_node *np = dev->of_node;
	enum of_gpio_flags flags;
	int ret;
	u32 temp;

	if (np == NULL)
		return -ENODEV;

	gp2a->p_out = of_get_named_gpio_flags(np, "gp2a,irq-gpio", 0,
		&flags);
	if (gp2a->p_out < 0) {
		SENSOR_ERR("get irq_gpio(%d) error\n", gp2a->p_out);
		return -ENODEV;
	}

	ret = of_property_read_u32(np, "gp2a,default_high_thd",
		&gp2a->default_high_thd);
	if (ret < 0) {
		SENSOR_ERR("Cannot set default_high_thd\n");
		gp2a->default_high_thd = DEFAULT_HIGH_THD;
	}

	ret = of_property_read_u32(np, "gp2a,default_low_thd",
		&gp2a->default_low_thd);
	if (ret < 0) {
		SENSOR_ERR("Cannot set default_low_thd\n");
		gp2a->default_low_thd = DEFAULT_LOW_THD;
	}

	ret = of_property_read_u32(np, "gp2a,cancel_high_thd",
		&gp2a->prox_cancel_h);
	if (ret < 0) {
		SENSOR_ERR("Cannot set cancel_high_thd\n");
		gp2a->prox_cancel_h = CANCEL_HIGH_THD;
	}

	ret = of_property_read_u32(np, "gp2a,cancel_low_thd",
		&gp2a->prox_cancel_l);
	if (ret < 0) {
		SENSOR_ERR("Cannot set cancel_low_thd\n");
		gp2a->prox_cancel_l = CANCEL_LOW_THD;
	}

	ret = of_property_read_u32(np, "gp2a,default_offset",
		&gp2a->default_trim);
	if (ret < 0) {
		SENSOR_ERR("Cannot set default_trim\n");
		gp2a->default_trim = DEFAULT_OFFSET;
	}

	ret = of_property_read_u32(np, "gp2a,reg_intval", &temp);
	if (ret < 0) {
		SENSOR_ERR("Cannot set reg_intval(0x83)\n");
		ps_reg_init_setting[PS_COM4][CMD] = COM4_INTVAL33;
	} else
		ps_reg_init_setting[PS_COM4][CMD] = temp;

	ret = of_property_read_u32(np, "gp2a,reg_res_p", &temp);
	if (ret < 0) {
		SENSOR_ERR("Cannot set reg_res_p(0x85)\n");
		ps_reg_init_setting[PS_PS1][CMD] = PS1_RES14;
	} else
		ps_reg_init_setting[PS_PS1][CMD] = temp;

	ret = of_property_read_u32(np, "gp2a,reg_ledctrl", &temp);
	if (ret < 0) {
		SENSOR_ERR("Cannot set reg_ledctrl(0x86)\n");
		ps_reg_init_setting[PS_PS2][CMD] = (PS2_IS130 | PS2_SUM32);
	} else
		ps_reg_init_setting[PS_PS2][CMD] = temp;

	ret = of_property_read_u32(np, "gp2a,reg_prst", &temp);
	if (ret < 0) {
		SENSOR_ERR("Cannot set reg_prst(0x87)\n");
		ps_reg_init_setting[PS_PS3][CMD] = (PS3_PRST3 | PS3_TGINTEN_PS1 | PS3_TGIRDRON0);
	} else
		ps_reg_init_setting[PS_PS3][CMD] = temp;

	gp2a->vled_ldo = of_get_named_gpio_flags(np, "gp2a,vled_ldo",
			0, &flags);
	if (gp2a->vled_ldo < 0) {
		SENSOR_ERR("fail to get vled_ldo: means to use regulator as vLED\n");
		gp2a->vled_ldo = 0;
	} else {
		ret = gpio_request(gp2a->vled_ldo, "prox_vled_en");
		if (ret < 0) {
			SENSOR_ERR("gpio %d request failed (%d)\n",
				gp2a->vled_ldo, ret);
			return ret;
		}
		gpio_direction_output(gp2a->vled_ldo, 0);
	}

	ret = of_property_read_u32(np, "gp2a,regulator_divided",
		&gp2a->regulator_divided);

	ret = of_property_read_u32(np, "gp2a,vdd_always_on",
		&gp2a->vdd_always_on);

	SENSOR_INFO("vdd_alwayson_on: %d, regulator_divided: %d, vled_ldo: %d\n",
		gp2a->vdd_always_on, gp2a->regulator_divided,
		gp2a->vled_ldo);
	SENSOR_INFO("initial register 0x83 = 0x%x, 0x85 = 0x%x, 0x86 = 0x%x, 0x87 = 0x%x",
		ps_reg_init_setting[PS_COM4][CMD],
		ps_reg_init_setting[PS_PS1][CMD],
		ps_reg_init_setting[PS_PS2][CMD],
		ps_reg_init_setting[PS_PS3][CMD]);

	return 0;
}


static int gp2a_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	struct gp2a_data *gp2a;
	int ret;

	SENSOR_INFO("start\n");

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		SENSOR_ERR("i2c functionality failed\n");
		return -ENOMEM;
	}

	/* Allocate memory for driver data */
	gp2a = kzalloc(sizeof(struct gp2a_data), GFP_KERNEL);
	if (!gp2a) {
		SENSOR_ERR("failed memory alloc\n");
		ret = -ENOMEM;
		goto err_mem_alloc;
	}

	ret = gp2a_parse_dt(&client->dev, gp2a);
	if (ret) {
		SENSOR_ERR("error in device tree");
		goto err_device_tree;
	}

	gp2a->i2c_client = client;
	i2c_set_clientdata(client, gp2a);

	proximity_vdd_onoff(&client->dev, ON);
	if (!gp2a->regulator_divided)
		proximity_vled_onoff(&client->dev, ON);
	usleep_range(1000, 1100);

	/* Check if the device is there or not. (Shutdown operation) */
	ret = gp2a_i2c_write_byte(client, REG_COM1, COM1_SD);
	if (ret < 0) {
		SENSOR_ERR("gp2a is not connected.(%d)\n", ret);
		goto err_check_device;
	}

	/* setup initial registers */
	ret = setup_register_gp2a(gp2a);
	if (ret < 0) {
		SENSOR_ERR("could not setup regs\n");
		goto err_setup_register;
	}

	ret = gp2a_input_init(gp2a);
	if (ret < 0) {
		SENSOR_ERR("failed to get input dev\n");
		goto err_input_init;
	}

	ret = sensors_register(&gp2a->dev, gp2a, proximity_attrs, MODULE_NAME);
	if (ret < 0) {
		SENSOR_INFO("could not sensors_register\n");
		goto err_sensors_register;
	}

	wake_lock_init(&gp2a->prx_wake_lock, WAKE_LOCK_SUSPEND,
		"prx_wake_lock");

	/* For factory test mode, we use timer to get average proximity data. */
	/* prox_timer settings. we poll for light values using a timer. */
	hrtimer_init(&gp2a->prox_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	gp2a->prox_poll_delay = ns_to_ktime(2000 * NSEC_PER_MSEC);/*2 sec*/
	gp2a->prox_timer.function = gp2a_prox_timer_func;

	/* the timer just fires off a work queue request.  we need a thread
	   to read the i2c (can be slow and blocking). */
	gp2a->prox_wq = create_singlethread_workqueue("gp2a_prox_wq");
	if (!gp2a->prox_wq) {
		ret = -ENOMEM;
		SENSOR_ERR("could not create prox workqueue\n");
		goto err_create_prox_workqueue;
	}

	/* this is the thread function we run on the work queue */
	INIT_WORK(&gp2a->work_prox, gp2a_work_func_prox);

	ret = gp2a_setup_irq(gp2a);
	if (ret) {
		SENSOR_ERR("could not setup irq\n");
		goto err_setup_irq;
	}

	if (!gp2a->regulator_divided)
		proximity_vled_onoff(&client->dev, OFF);
	if (!gp2a->vdd_always_on)
		proximity_vdd_onoff(&client->dev, OFF);

	SENSOR_INFO("success\n");
	return ret;

err_setup_irq:
	destroy_workqueue(gp2a->prox_wq);
err_create_prox_workqueue:
	wake_lock_destroy(&gp2a->prx_wake_lock);
	sensors_unregister(gp2a->dev, proximity_attrs);
err_sensors_register:
	sensors_remove_symlink(&gp2a->proximity_input_dev->dev.kobj,
		gp2a->proximity_input_dev->name);
	sysfs_remove_group(&gp2a->proximity_input_dev->dev.kobj,
		&proximity_attribute_group);
	input_unregister_device(gp2a->proximity_input_dev);
err_input_init:
err_setup_register:
err_check_device:
	if (gp2a->vled_ldo)
		gpio_free(gp2a->vled_ldo);
	if (!gp2a->regulator_divided)
		proximity_vled_onoff(&client->dev, OFF);
	proximity_vdd_onoff(&client->dev, OFF);
err_device_tree:
	kfree(gp2a);
err_mem_alloc:
	SENSOR_ERR("failed\n");
	return ret;
}


static int gp2a_suspend(struct device *dev)
{
	struct gp2a_data *data = dev_get_drvdata(dev);
	int enable;

	SENSOR_INFO("is called.\n");
	enable = atomic_read(&data->prox_enable);
	if (enable)
		disable_irq(data->irq);

	return 0;
}

static int gp2a_resume(struct device *dev)
{
	struct gp2a_data *data = dev_get_drvdata(dev);
	int enable;

	SENSOR_INFO("is called.\n");
	enable = atomic_read(&data->prox_enable);
	if (enable)
		enable_irq(data->irq);

	return 0;
}

static const struct dev_pm_ops gp2a_pm_ops = {
	.suspend = gp2a_suspend,
	.resume = gp2a_resume
};

static const struct i2c_device_id gp2a_device_id[] = {
	{"gp2a", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, gp2a_device_id);

static struct of_device_id gp2a_i2c_match_table[] = {
	{ .compatible = "gp2a-i2c",},
	{},
};

MODULE_DEVICE_TABLE(of, gp2a_i2c_match_table);

static struct i2c_driver gp2a_i2c_driver = {
	.driver = {
		.name = "gp2a",
		.owner = THIS_MODULE,
		.of_match_table = gp2a_i2c_match_table,
		.pm = &gp2a_pm_ops
	},
	.probe		= gp2a_i2c_probe,
	.id_table	= gp2a_device_id,
};

module_i2c_driver(gp2a_i2c_driver);

MODULE_AUTHOR("Samsung Electronics");
MODULE_DESCRIPTION("Proximity Sensor driver for gp2ap070s");
MODULE_LICENSE("GPL");
