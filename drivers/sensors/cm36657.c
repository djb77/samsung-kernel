/*
 * Copyright (c) 2011 SAMSUNG
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/i2c.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/wakelock.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/of_gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/pinctrl/consumer.h>
#include "cm36657.h"
#include <linux/sensor/sensors_core.h>

#ifdef TAG
#undef TAG
#define TAG "[PROX]"
#endif

#define	VENDOR				"CAPELLA"
#define	CHIP_ID				"CM36657"

/* for i2c Write */
#define I2C_M_WR			0

/* register addresses */
/* Ambient light sensor */
#define REG_CS_CONF1			0x00
#define REG_ALS_RED_DATA		0xF0
#define REG_ALS_GREEN_DATA		0xF1
#define REG_ALS_BLUE_DATA		0xF2
#define REG_WHITE_DATA			0xF3

/* Proximity sensor */
#define REG_PS_CONF1			0x03
#define REG_PS_CONF3			0x04
#define REG_PS_THDL			0x05
#define REG_PS_THDH			0x06
#define REG_PS_CANC			0x07
#define REG_PS_DATA			0xF4

#define ALS_REG_NUM			2
#define PS_REG_NUM			5

#define INT_FLAG			0xF5
#define ID_REG				0xF6

#define MSK_L(x)			(x & 0xff)
#define MSK_H(x)			((x & 0xff00) >> 8)

/* Intelligent Cancellation*/
#define PROX_READ_NUM			40
#define PS_CANC_ARR_NUM			5

#define CONTROL_INT_ISR_REPORT		0x00
#define CONTROL_ALS			0x01
#define CONTROL_PS			0x02
#define CONTROL_ALS_REPORT		0x03

/*for INT FLAG*/
#define INT_FLAG_PS_SPFLAG		(1 << 14)

/* PS rises above PS_THDH INT trigger event */
#define INT_FLAG_PS_IF_CLOSE		(1 << 9)
/* PS drops below PS_THDL INT trigger event */
#define INT_FLAG_PS_IF_AWAY		(1 << 8)

 /*lightsensor log time 6SEC 200mec X 30*/
#define LIGHT_LOG_TIME			30

enum {
	LIGHT_ENABLED = BIT(0),
	PROXIMITY_ENABLED = BIT(1),
};

#define PS_CLOSE			0
#define PS_AWAY				1
#define PS_CLOSE_AND_AWAY		2

#define DEFAULT_ADC			50
#define ADC_MAX_VALUE			4095

#define LEVEL1_THDL			1
#define LEVEL1_THDH			110
#define LEVEL2_THDL			70
#define LEVEL2_THDH			3500
#define LEVEL3_THDL			2000
#define LEVEL3_THDH			ADC_MAX_VALUE

uint16_t cm36657_thd_tbl[3][2] = {
	{ LEVEL1_THDL, LEVEL1_THDH },
	{ LEVEL2_THDL, LEVEL2_THDH },
	{ LEVEL3_THDL, LEVEL3_THDH },
};

enum {
	REG_ADDR = 0,
	CMD,
};

enum {
	OFF = 0,
	ON = 1
};

/* driver data */
struct cm36657_data {
	struct i2c_client *i2c_client;
	struct wake_lock prox_wake_lock;
	struct input_dev *prox_input_dev;
	struct input_dev *light_input_dev;
	struct mutex power_lock;
	struct mutex read_lock;
	struct mutex control_mutex;
	struct hrtimer light_timer;
	struct hrtimer prox_timer;
	struct workqueue_struct *light_wq;
	struct workqueue_struct *prox_wq;
	struct workqueue_struct *prox_avg_wq;
	struct work_struct work_light;
	struct work_struct work_prox;
	struct work_struct work_prox_avg;
	struct device *prox_dev;
	struct device *light_dev;
	struct regulator *prox_vled;

	ktime_t light_poll_delay;
	ktime_t prox_poll_delay;
	int irq;
	int irq_gpio;
	int led_ldo_pin;
	u8 power_state;
	int avg[3];
	u16 red_data;
	u16 green_data;
	u16 blue_data;
	u16 white_data;
	int count_log_time;
	unsigned int prox_result;

	int offset;
	int vdd_ldo_pin;
	const char *led_vdd;

	int cur_lv;

	uint16_t ls_cmd;
	uint16_t ps_conf1_val;
	uint16_t ps_conf3_val;
};

static int cm36657_vdd_onoff(struct cm36657_data *cm36657, int onoff)
{
	/* ldo control */
	if (cm36657->vdd_ldo_pin) {
		gpio_set_value(cm36657->vdd_ldo_pin, onoff);
		if (onoff)
			msleep(20);
		return 0;
	}

	return 0;
}

static int cm36657_prox_vled_onoff(struct cm36657_data *cm36657, int onoff)
{
	int err;

	if (cm36657->led_ldo_pin) {
		gpio_set_value(cm36657->led_ldo_pin, onoff);
		if (onoff)
			msleep(10);
		return 0;
	} else if (cm36657->led_vdd) {	/* regulator(PMIC) control */
		if (onoff) {
			if (regulator_is_enabled(cm36657->prox_vled)) {
				SENSOR_INFO("Regulator already enabled\n");
				return 0;
			}
			err = regulator_enable(cm36657->prox_vled);
			if (err < 0)
				SENSOR_ERR("Failed to enable vled %d\n", err);
			usleep_range(10000, 11000);
		} else {
			err = regulator_disable(cm36657->prox_vled);
			if (err < 0)
				SENSOR_ERR("Failed to disable vled %d\n", err);
		}
	}

	return 0;
}


int cm36657_i2c_read_word(struct cm36657_data *cm36657, u8 command, u16 *val)
{
	int err = 0;
	int retry = 3;
	struct i2c_client *client = cm36657->i2c_client;
	struct i2c_msg msg[2];
	unsigned char data[2] = {0,};
	u16 value = 0;

	if ((client == NULL) || (!client->adapter))
		return -ENODEV;

	while (retry--) {

		/* send slave address & command */
		msg[0].addr = client->addr;
		msg[0].flags = I2C_M_WR;
		msg[0].len = 1;
		msg[0].buf = &command;

		/* read word data */
		msg[1].addr = client->addr;
		msg[1].flags = I2C_M_RD;
		msg[1].len = 2;
		msg[1].buf = data;

		err = i2c_transfer(client->adapter, msg, 2);

		if (err >= 0) {
			value = (u16)data[1];
			*val = (value << 8) | (u16)data[0];
			return err;
		}
	}
	SENSOR_ERR("i2c transfer error ret=%d\n", err);
	return err;
}

int cm36657_i2c_write_word(struct cm36657_data *cm36657, u8 command,
			   u16 val)
{
	int err = 0;
	struct i2c_client *client = cm36657->i2c_client;
	int retry = 3;

	if ((client == NULL) || (!client->adapter))
		return -ENODEV;

	while (retry--) {
		err = i2c_smbus_write_word_data(client, command, val);
		if (err >= 0)
			return 0;
	}
	SENSOR_ERR("i2c transfer error(%d)\n", err);
	return err;
}

static void cm36657_light_enable(struct cm36657_data *cm36657)
{
	/* enable setting */
	cm36657->ls_cmd &= CM36657_CS_SD_MASK;

	cm36657_i2c_write_word(cm36657, REG_CS_CONF1, cm36657->ls_cmd);
	/* 50ms*1.5, RGBIR integration time*1.5 recommended */
	hrtimer_start(&cm36657->light_timer, ns_to_ktime(75 * NSEC_PER_MSEC),
		HRTIMER_MODE_REL);
}

static void cm36657_light_disable(struct cm36657_data *cm36657)
{
	cm36657->ls_cmd |= CM36657_CS_SD;

	/* disable setting */
	cm36657_i2c_write_word(cm36657, REG_CS_CONF1, cm36657->ls_cmd);
	hrtimer_cancel(&cm36657->light_timer);
	cancel_work_sync(&cm36657->work_light);
}

static ssize_t cm36657_poll_delay_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct cm36657_data *cm36657 = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%lld\n",
		ktime_to_ns(cm36657->light_poll_delay));
}

static ssize_t cm36657_poll_delay_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct cm36657_data *cm36657 = dev_get_drvdata(dev);
	int64_t new_delay;
	int err;

	err = kstrtoll(buf, 10, &new_delay);
	if (err < 0)
		return err;

	mutex_lock(&cm36657->power_lock);
	if (new_delay != ktime_to_ns(cm36657->light_poll_delay)) {
		SENSOR_INFO("poll_delay = %lld\n", new_delay);
		cm36657->light_poll_delay = ns_to_ktime(new_delay);

		if (cm36657->power_state & LIGHT_ENABLED) {
			cm36657_light_disable(cm36657);
			cm36657_light_enable(cm36657);
		}
	}
	mutex_unlock(&cm36657->power_lock);

	return size;
}

static ssize_t light_enable_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct cm36657_data *cm36657 = dev_get_drvdata(dev);
	bool new_value;

	if (sysfs_streq(buf, "1"))
		new_value = true;
	else if (sysfs_streq(buf, "0"))
		new_value = false;
	else {
		SENSOR_ERR("invalid value %d\n", *buf);
		return -EINVAL;
	}

	SENSOR_INFO("new_value = %d, old state = %d\n",
		new_value, (cm36657->power_state & LIGHT_ENABLED) ? 1 : 0);

	mutex_lock(&cm36657->power_lock);

	if (new_value && !(cm36657->power_state & LIGHT_ENABLED)) {
		cm36657->power_state |= LIGHT_ENABLED;
		cm36657_light_enable(cm36657);
	} else if (!new_value && (cm36657->power_state & LIGHT_ENABLED)) {
		cm36657_light_disable(cm36657);
		cm36657->power_state &= ~LIGHT_ENABLED;
	}
	mutex_unlock(&cm36657->power_lock);
	return size;
}

static ssize_t light_enable_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct cm36657_data *cm36657 = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n",
		(cm36657->power_state & LIGHT_ENABLED) ? 1 : 0);
}

static void cm36657_get_avg_val(struct cm36657_data *cm36657)
{
	int min = 0, max = 0, avg = 0;
	int i;
	u16 ps_data = 0;

	for (i = 0; i < PROX_READ_NUM; i++) {
		msleep(40);
		cm36657_i2c_read_word(cm36657, REG_PS_DATA, &ps_data);
		avg += ps_data;

		if (!i)
			min = ps_data;
		else if (ps_data < min)
			min = ps_data;

		if (ps_data > max)
			max = ps_data;
	}
	avg /= PROX_READ_NUM;

	cm36657->avg[0] = min;
	cm36657->avg[1] = avg;
	cm36657->avg[2] = max;
}

static int cm36657_get_ps_adc_value(struct cm36657_data *cm36657,
					uint16_t *data)
{
	int ret = 0;

	if (data == NULL)
		return -EFAULT;

	ret = cm36657_i2c_read_word(cm36657, REG_PS_DATA, data);

	if (ret < 0) {
		SENSOR_ERR("i2c read fail\n");
		return -EIO;
	}

	return ret;
}

static int cm36657_get_canc_ps(struct cm36657_data *cm36657, int is_first)
{
	int ret = 0;
	int i = 0;
	uint16_t value[PS_CANC_ARR_NUM];
	uint32_t ps_average = 0;

	/* Disable INT */
	cm36657_i2c_read_word(cm36657, REG_PS_CONF1, &cm36657->ps_conf1_val);
	cm36657->ps_conf1_val &= CM36657_PS_INT_MASK;
	cm36657_i2c_write_word(cm36657, REG_PS_CONF1, cm36657->ps_conf1_val);

	/* Initialize */
	cm36657_i2c_write_word(cm36657, REG_PS_CANC, 0);

	for (i = 0; i < PS_CANC_ARR_NUM ; i++) {
		/* delay 32ms(PS_PERIOD) * 1.5 */
		msleep(48);
		ret = cm36657_get_ps_adc_value(cm36657, &value[i]);
		if (ret < 0) {
			pr_err("[PS_ERR][CM36657 error]%s: get_ps_adc_value\n",
				__func__);
			return ret;
		}
		ps_average += value[i];
	}
	ps_average /= PS_CANC_ARR_NUM;

	if (ps_average < DEFAULT_ADC)
		cm36657->offset = ps_average;
	else if (is_first && (ps_average >= 3000))
		cm36657->offset = ps_average / 2;
	else if (ps_average >= 4000)
		cm36657->offset = 4000;
	else
		cm36657->offset = ps_average - DEFAULT_ADC;

	SENSOR_INFO("ps_average : %d, offset : %d\n", ps_average,
			cm36657->offset);

	cm36657_i2c_write_word(cm36657, REG_PS_CANC, cm36657->offset);

	/* Enable INT */
	cm36657_i2c_read_word(cm36657, REG_PS_CONF1, &cm36657->ps_conf1_val);
	cm36657->ps_conf1_val |= CM36657_PS_INT_ENABLE;
	cm36657_i2c_write_word(cm36657, REG_PS_CONF1, cm36657->ps_conf1_val);
	return 0;
}

static int cm36657_control_and_report(struct cm36657_data *cm36657,
					uint8_t mode)
{
	int changeThd = 0;
	int direction = 1;
	uint16_t int_flag;
	u16 ps_data = 0;
	int err;

	mutex_lock(&cm36657->control_mutex);

	if (mode == CONTROL_PS) {
		changeThd = 1;

		cm36657_get_canc_ps(cm36657, 1);
		/* delay 32ms(PS_PERIOD) * 1.5 */
		msleep(48);
		mutex_lock(&cm36657->read_lock);
		cm36657_get_ps_adc_value(cm36657, &ps_data);
		mutex_unlock(&cm36657->read_lock);

		if (ps_data > cm36657_thd_tbl[cm36657->cur_lv][1])
			direction = 0;
	} else if (mode == CONTROL_INT_ISR_REPORT) {
		usleep_range(2900, 3100);
		err = cm36657_i2c_read_word(cm36657, INT_FLAG, &int_flag);
		if (err < 0) {
			SENSOR_ERR("INT_FLAG read error, ret=%d\n", err);
			mutex_unlock(&cm36657->control_mutex);
			return 0;
		}

		if (int_flag & INT_FLAG_PS_IF_CLOSE) {
			direction = 0;
			mutex_lock(&cm36657->read_lock);
			cm36657_get_ps_adc_value(cm36657, &ps_data);
			mutex_unlock(&cm36657->read_lock);
		} else if (int_flag & INT_FLAG_PS_IF_AWAY) {
			direction = 1;
			mutex_lock(&cm36657->read_lock);
			cm36657_get_ps_adc_value(cm36657, &ps_data);
			mutex_unlock(&cm36657->read_lock);
			if (ps_data == 0 || ((cm36657->cur_lv == 2) &&
					(ps_data < cm36657_thd_tbl[2][0])))
				cm36657_get_canc_ps(cm36657, 0);
		} else {
			mutex_lock(&cm36657->read_lock);
			cm36657_get_ps_adc_value(cm36657, &ps_data);
			mutex_unlock(&cm36657->read_lock);
			SENSOR_ERR("Unknown event, int_flag=0x%x, ps_data = %d\n",
					int_flag, ps_data);
			mutex_unlock(&cm36657->control_mutex);
			return 0;
		}
	}

#ifdef CONFIG_SEC_FACTORY
	SENSOR_INFO("FACTORY: dir = %d, ps_data = %d\n", direction, ps_data);
	{
#else
	SENSOR_INFO("dir = %d (0 : CLOSE, 1 : FAR), ps_data = %d\n",
			direction, ps_data);
	if ((direction && (ps_data < cm36657_thd_tbl[cm36657->cur_lv][0])) ||
			((!direction) &&
			(ps_data > cm36657_thd_tbl[cm36657->cur_lv][1])) ||
			(mode == CONTROL_PS)) {
#endif
		input_report_abs(cm36657->prox_input_dev, ABS_DISTANCE,
				direction);
		input_sync(cm36657->prox_input_dev);
	}

	SENSOR_INFO("cur_lv = %d, (high_thd, low_thd) = (%d,%d)\n",
				cm36657->cur_lv,
				cm36657_thd_tbl[cm36657->cur_lv][1],
				cm36657_thd_tbl[cm36657->cur_lv][0]);

	if ((direction == 0) &&
			(cm36657->cur_lv == 0 || cm36657->cur_lv == 1)) {
		cm36657->cur_lv++;
		changeThd = 1;
	} else if ((direction == 1) &&
			(cm36657->cur_lv == 1 || cm36657->cur_lv == 2)) {
		cm36657->cur_lv--;
		changeThd = 1;
	}

	if (changeThd == 1) {
		/* Disable INT */
		cm36657_i2c_read_word(cm36657, REG_PS_CONF1,
				&cm36657->ps_conf1_val);
		cm36657->ps_conf1_val |= CM36657_PS_SD;
		cm36657->ps_conf1_val &= CM36657_PS_INT_MASK;
		cm36657_i2c_write_word(cm36657, REG_PS_CONF1,
				cm36657->ps_conf1_val);

		/* Set THD */
		cm36657_i2c_write_word(cm36657, REG_PS_THDL,
				cm36657_thd_tbl[cm36657->cur_lv][0]);
		cm36657_i2c_write_word(cm36657, REG_PS_THDH,
				cm36657_thd_tbl[cm36657->cur_lv][1]);

		SENSOR_INFO("next_lv = %d, (high_thd, low_thd) = (%d,%d)\n",
				cm36657->cur_lv,
				cm36657_thd_tbl[cm36657->cur_lv][1],
				cm36657_thd_tbl[cm36657->cur_lv][0]);

		/* Enable INT */
		cm36657_i2c_read_word(cm36657, REG_PS_CONF1,
				&cm36657->ps_conf1_val);
		cm36657->ps_conf1_val &= CM36657_PS_SD_MASK;
		cm36657->ps_conf1_val |= CM36657_PS_INT_ENABLE;
		cm36657_i2c_write_word(cm36657, REG_PS_CONF1,
				cm36657->ps_conf1_val);
	}

	mutex_unlock(&cm36657->control_mutex);
	return 0;
}

static ssize_t proximity_enable_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct cm36657_data *cm36657 = dev_get_drvdata(dev);
	bool new_value;

	if (sysfs_streq(buf, "1"))
		new_value = true;
	else if (sysfs_streq(buf, "0"))
		new_value = false;
	else {
		SENSOR_ERR("invalid value %d\n", *buf);
		return size;
	}

	SENSOR_INFO("new_value = %d, old state = %d\n",
	    new_value, (cm36657->power_state & PROXIMITY_ENABLED) ? 1 : 0);

	/* initialize threshold */
	cm36657->cur_lv = 0;

	mutex_lock(&cm36657->power_lock);
	if (new_value && !(cm36657->power_state & PROXIMITY_ENABLED)) {
		cm36657_prox_vled_onoff(cm36657, ON);
		cm36657->power_state |= PROXIMITY_ENABLED;
		cm36657->ps_conf1_val &= CM36657_PS_SD_MASK;
		cm36657->ps_conf1_val |= CM36657_PS_INT_ENABLE;

		cm36657_i2c_write_word(cm36657, REG_PS_CONF1,
				cm36657->ps_conf1_val);
		cm36657_control_and_report(cm36657, CONTROL_PS);

		enable_irq(cm36657->irq);
		enable_irq_wake(cm36657->irq);

	} else if (!new_value && (cm36657->power_state & PROXIMITY_ENABLED)) {
		disable_irq_wake(cm36657->irq);
		disable_irq(cm36657->irq);

		cm36657->power_state &= ~PROXIMITY_ENABLED;
		cm36657->ps_conf1_val |= CM36657_PS_SD;
		cm36657->ps_conf1_val &= CM36657_PS_INT_MASK;

		cm36657_i2c_write_word(cm36657, REG_PS_CONF1,
				cm36657->ps_conf1_val);
		cm36657_prox_vled_onoff(cm36657, OFF);
	}
	mutex_unlock(&cm36657->power_lock);

	return size;
}

static ssize_t proximity_enable_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct cm36657_data *cm36657 = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n",
		(cm36657->power_state & PROXIMITY_ENABLED) ? 1 : 0);
}

static DEVICE_ATTR(poll_delay, 0644, cm36657_poll_delay_show,
	cm36657_poll_delay_store);

static struct device_attribute dev_attr_light_enable =
	__ATTR(enable, 0644, light_enable_show, light_enable_store);

static struct device_attribute dev_attr_proximity_enable =
	__ATTR(enable, 0644, proximity_enable_show, proximity_enable_store);

static struct attribute *light_sysfs_attrs[] = {
	&dev_attr_light_enable.attr,
	&dev_attr_poll_delay.attr,
	NULL
};

static struct attribute_group light_attribute_group = {
	.attrs = light_sysfs_attrs,
};

static struct attribute *proximity_sysfs_attrs[] = {
	&dev_attr_proximity_enable.attr,
	NULL
};

static struct attribute_group proximity_attribute_group = {
	.attrs = proximity_sysfs_attrs,
};

/* sysfs for vendor & name */
static ssize_t cm36657_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", VENDOR);
}

static ssize_t cm36657_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", CHIP_ID);
}
static struct device_attribute dev_attr_prox_sensor_vendor =
	__ATTR(vendor, 0444, cm36657_vendor_show, NULL);
static struct device_attribute dev_attr_light_sensor_vendor =
	__ATTR(vendor, 0444, cm36657_vendor_show, NULL);
static struct device_attribute dev_attr_prox_sensor_name =
	__ATTR(name, 0444, cm36657_name_show, NULL);
static struct device_attribute dev_attr_light_sensor_name =
	__ATTR(name, 0444, cm36657_name_show, NULL);

/* proximity sysfs */
static ssize_t proximity_trim_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct cm36657_data *cm36657 = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%u\n", cm36657->offset);
}
static ssize_t proximity_avg_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct cm36657_data *cm36657 = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d\n", cm36657->avg[0],
		cm36657->avg[1], cm36657->avg[2]);
}

static ssize_t proximity_avg_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct cm36657_data *cm36657 = dev_get_drvdata(dev);
	bool new_value = false;

	if (sysfs_streq(buf, "1"))
		new_value = true;
	else if (sysfs_streq(buf, "0"))
		new_value = false;
	else {
		SENSOR_ERR("invalid value %d\n", *buf);
		return -EINVAL;
	}

	SENSOR_INFO("average enable = %d\n", new_value);

	if (new_value) {
		hrtimer_start(&cm36657->prox_timer, cm36657->prox_poll_delay,
			HRTIMER_MODE_REL);

	} else if (!new_value) {
		hrtimer_cancel(&cm36657->prox_timer);
		cancel_work_sync(&cm36657->work_prox_avg);
	}

	return size;
}

static ssize_t proximity_state_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct cm36657_data *cm36657 = dev_get_drvdata(dev);
	u16 ps_data;

	mutex_lock(&cm36657->read_lock);
	cm36657_i2c_read_word(cm36657, REG_PS_DATA, &ps_data);
	mutex_unlock(&cm36657->read_lock);
	return snprintf(buf, PAGE_SIZE, "%u\n", ps_data);
}

static ssize_t proximity_thresh_high_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", cm36657_thd_tbl[0][1]);
}

static ssize_t proximity_thresh_high_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct cm36657_data *cm36657 = dev_get_drvdata(dev);
	u16 thresh_value;
	int err;

	err = kstrtou16(buf, 10, &thresh_value);
	if (err < 0)
		SENSOR_ERR("kstrtoint failed.\n");
	SENSOR_INFO("thresh_value:%u\n", thresh_value);

	if (thresh_value > 2) {
		cm36657_thd_tbl[0][1] = thresh_value;
		err = cm36657_i2c_write_word(cm36657, REG_PS_THDH,
				cm36657_thd_tbl[0][1]);
		if (err < 0) {
			SENSOR_ERR("thresh_high is failed. %d\n", err);
			return size;
		}
		SENSOR_INFO("new high threshold = 0x%x\n", thresh_value);
		msleep(150);
	} else
		SENSOR_ERR("wrong high threshold value(0x%x)\n", thresh_value);

	return size;
}

static ssize_t proximity_thresh_low_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", cm36657_thd_tbl[1][0]);
}

static ssize_t proximity_thresh_low_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct cm36657_data *cm36657 = dev_get_drvdata(dev);
	u16 thresh_value;
	int err;

	err = kstrtou16(buf, 10, &thresh_value);
	if (err < 0)
		SENSOR_ERR("kstrtoint failed.");
	SENSOR_INFO("thresh_value:%u\n", thresh_value);

	if (thresh_value > 2) {
		cm36657_thd_tbl[1][0] = thresh_value;
		err = cm36657_i2c_write_word(cm36657, REG_PS_THDH,
				cm36657_thd_tbl[1][0]);
		if (err < 0) {
			SENSOR_ERR("thresh_low is failed. %d\n", err);
			return size;
		}
		SENSOR_INFO("new low threshold = 0x%x\n", thresh_value);
		msleep(150);
	} else
		SENSOR_ERR("wrong low threshold value(0x%x)\n", thresh_value);

	return size;
}

static ssize_t proximity_thresh_detect_high_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", cm36657_thd_tbl[1][1]);
}

static ssize_t proximity_thresh_detect_high_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct cm36657_data *cm36657 = dev_get_drvdata(dev);
	u16 thresh_value;
	int err;

	err = kstrtou16(buf, 10, &thresh_value);
	if (err < 0)
		SENSOR_ERR("kstrtoint failed.\n");
	SENSOR_INFO("thresh_value:%u\n", thresh_value);

	if (thresh_value > 2) {
		cm36657_thd_tbl[1][1] = thresh_value;
		err = cm36657_i2c_write_word(cm36657, REG_PS_THDH,
				cm36657_thd_tbl[1][1]);
		if (err < 0) {
			SENSOR_ERR("thresh_detect_high is failed. %d\n", err);
			return size;
		}
		SENSOR_INFO("new high threshold = 0x%x\n", thresh_value);
		msleep(150);
	} else
		SENSOR_ERR("wrong high threshold value(0x%x)\n", thresh_value);

	return size;
}

static ssize_t proximity_thresh_detect_low_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", cm36657_thd_tbl[2][0]);
}

static ssize_t proximity_thresh_detect_low_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct cm36657_data *cm36657 = dev_get_drvdata(dev);
	u16 thresh_value;
	int err;

	err = kstrtou16(buf, 10, &thresh_value);
	if (err < 0)
		SENSOR_ERR("kstrtoint failed.");
	SENSOR_INFO("thresh_value:%u\n", thresh_value);

	if (thresh_value > 2) {
		cm36657_thd_tbl[2][0] = thresh_value;
		err = cm36657_i2c_write_word(cm36657, REG_PS_THDH,
				cm36657_thd_tbl[2][0]);
		if (err < 0) {
			SENSOR_ERR("thresh_detect_low is failed. %d\n", err);
			return size;
		}
		SENSOR_INFO("new low threshold = 0x%x\n", thresh_value);
		msleep(150);
	} else
		SENSOR_ERR("wrong low threshold value(0x%x)\n", thresh_value);

	return size;
}

static ssize_t proximity_dhr_sensor_info_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct cm36657_data *cm36657 = dev_get_drvdata(dev);
	u16 cs_conf1;
	u16 ps_conf1, ps_conf3;
	u8 als_it;
	u8 ps_period, ps_pers, ps_it;
	u8 ps_smart_pers, ps_led_current;
	u16 ps_canc;
	u16 ps_low_thresh, ps_hi_thresh;

	mutex_lock(&cm36657->read_lock);
	cm36657_i2c_read_word(cm36657, REG_CS_CONF1, &cs_conf1);
	cm36657_i2c_read_word(cm36657, REG_PS_CONF1, &ps_conf1);
	cm36657_i2c_read_word(cm36657, REG_PS_CONF3, &ps_conf3);
	cm36657_i2c_read_word(cm36657, REG_PS_CANC, &ps_canc);
	cm36657_i2c_read_word(cm36657, REG_PS_THDL, &ps_low_thresh);
	cm36657_i2c_read_word(cm36657, REG_PS_THDH, &ps_hi_thresh);
	mutex_unlock(&cm36657->read_lock);

	als_it = (cs_conf1 & 0x0c) >> 2;
	ps_period = (ps_conf1 & 0xc0) >> 6;
	ps_pers = (ps_conf1 & 0x30) >> 4;
	ps_it = (ps_conf1 & 0xc000) >> 14;
	ps_smart_pers = (ps_conf1 & 0x02) >> 1;
	ps_led_current = (ps_conf3 & 0x0700) >> 8;

	return snprintf(buf, PAGE_SIZE, "\"THD\":\"%u %u\","\
			"\"ALS_IT\":\"0x%x\",\"PS_PERIOD\":\"0x%x\","\
			"\"PS_PERS\":\"0x%x\", \"PS_IT\":\"0x%x\","\
			"\"PS_SMART_PERS\":\"0x%x\","\
			"\"PS_LED_CURRENT\":\"0x%x\","\
			"\"PS_CANC\":\"0x%x\"\n",
			ps_low_thresh, ps_hi_thresh,
			als_it, ps_period, ps_pers, ps_it,
			ps_smart_pers, ps_led_current, ps_canc);
}

static ssize_t proximity_write_register_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int reg, val, ret;
	struct cm36657_data *cm36657 = dev_get_drvdata(dev);

	if (sscanf(buf, "%4x,%4x", &reg, &val) != 2) {
		SENSOR_ERR("invalid value\n");
		return count;
	}

	ret = cm36657_i2c_write_word(cm36657, reg, val);
	if (ret < 0)
		SENSOR_ERR("failed %d\n", ret);
	else
		SENSOR_INFO("Register(0x%4x) data(0x%4x)\n", reg, val);

	return count;
}

static ssize_t proximity_read_register_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cm36657_data *cm36657 = dev_get_drvdata(dev);

	u8 reg;
	int offset = 0;
	u16 val = 0;

	for (reg = 0x00; reg <= 0x07; reg++) {
		cm36657_i2c_read_word(cm36657, reg, &val);
		SENSOR_INFO("Read Reg: 0x%4x Value: 0x%4x\n", reg, val);
		offset += snprintf(buf + offset, PAGE_SIZE - offset,
			"Reg: 0x%4x Value: 0x%4x\n", reg, val);
	}

	cm36657_i2c_read_word(cm36657, INT_FLAG, &val);
	offset += snprintf(buf + offset, PAGE_SIZE - offset,
			"INT_FLAG: 0x%4x Value: 0x%4x\n", INT_FLAG, val);

	return offset;
}

static DEVICE_ATTR(prox_register, 0644,
	proximity_read_register_show, proximity_write_register_store);
static DEVICE_ATTR(prox_avg, 0644,
	proximity_avg_show, proximity_avg_store);
static DEVICE_ATTR(state, 0444, proximity_state_show, NULL);
static struct device_attribute dev_attr_prox_raw = __ATTR(raw_data,
	0444, proximity_state_show, NULL);
static DEVICE_ATTR(thresh_high, 0644,
	proximity_thresh_high_show, proximity_thresh_high_store);
static DEVICE_ATTR(thresh_low, 0644,
	proximity_thresh_low_show, proximity_thresh_low_store);
static DEVICE_ATTR(thresh_detect_high, 0644,
	proximity_thresh_detect_high_show, proximity_thresh_detect_high_store);
static DEVICE_ATTR(thresh_detect_low, 0644,
	proximity_thresh_detect_low_show, proximity_thresh_detect_low_store);
static DEVICE_ATTR(prox_trim, 0444,
	proximity_trim_show, NULL);
static DEVICE_ATTR(dhr_sensor_info, 0444,
	proximity_dhr_sensor_info_show, NULL);

static struct device_attribute *prox_sensor_attrs[] = {
	&dev_attr_prox_sensor_vendor,
	&dev_attr_prox_sensor_name,
	&dev_attr_prox_avg,
	&dev_attr_state,
	&dev_attr_thresh_high,
	&dev_attr_thresh_low,
	&dev_attr_prox_raw,
	&dev_attr_prox_trim,
	&dev_attr_prox_register,
	&dev_attr_dhr_sensor_info,
	&dev_attr_thresh_detect_high,
	&dev_attr_thresh_detect_low,
	NULL,
};

/* light sysfs */
static ssize_t light_lux_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct cm36657_data *cm36657 = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%u,%u,%u,%u\n",
		cm36657->red_data, cm36657->green_data,
		cm36657->blue_data, cm36657->white_data);
}

static ssize_t light_data_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct cm36657_data *cm36657 = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%u,%u,%u,%u\n",
		cm36657->red_data, cm36657->green_data,
		cm36657->blue_data, cm36657->white_data);
}

static DEVICE_ATTR(lux, 0444, light_lux_show, NULL);
static DEVICE_ATTR(raw_data, 0444, light_data_show, NULL);

static struct device_attribute *light_sensor_attrs[] = {
	&dev_attr_light_sensor_vendor,
	&dev_attr_light_sensor_name,
	&dev_attr_lux,
	&dev_attr_raw_data,
	NULL,
};

static void cm36657_irq_do_work(struct work_struct *work)
{
	struct cm36657_data *cm36657 = container_of(work, struct cm36657_data,
						  work_prox);

	cm36657_control_and_report(cm36657, CONTROL_INT_ISR_REPORT);

	enable_irq(cm36657->irq);
}

irqreturn_t cm36657_irq_handler(int irq, void *data)
{
	struct cm36657_data *cm36657 = data;

	SENSOR_INFO("!!\n");

	disable_irq_nosync(cm36657->irq);
	wake_lock_timeout(&cm36657->prox_wake_lock, 3 * HZ);
	queue_work(cm36657->prox_wq, &cm36657->work_prox);

	return IRQ_HANDLED;
}

static int cm36657_setup_reg(struct cm36657_data *cm36657)
{
	int err = 0;
	u16 tmp = 0;

	/* ALS initialization */
	err = cm36657_i2c_write_word(cm36657,
			REG_CS_CONF1, CM36657_CS_START);
	if (err < 0) {
		SENSOR_ERR("cm36657_als_reg is failed. %d\n", err);
		return err;
	}
	
	/* PS initialization */
	err = cm36657_i2c_write_word(cm36657,
			REG_PS_CONF1, cm36657->ps_conf1_val);
	if (err < 0) {
		SENSOR_ERR("cm36657_ps_reg is failed. %d\n", err);
		return err;
	}

	err = cm36657_i2c_write_word(cm36657,
			REG_PS_CONF3, cm36657->ps_conf3_val);
	if (err < 0) {
		SENSOR_ERR("cm36657_ps_reg is failed. %d\n", err);
		return err;
	}

	/* printing the initial proximity value with no contact */
	msleep(50);
	mutex_lock(&cm36657->read_lock);
	err = cm36657_i2c_read_word(cm36657, REG_PS_DATA, &tmp);
	mutex_unlock(&cm36657->read_lock);
	if (err < 0) {
		SENSOR_ERR("read ps_data failed\n");
		err = -EIO;
	}
	SENSOR_INFO("initial proximity value = %d\n", tmp);

	/*must disable l-sensor interrupt before IST create*/
	/*disable ALS func*/
	cm36657->ls_cmd = CM36657_CS_START;
	cm36657->ls_cmd |= CM36657_CS_SD;
	cm36657_i2c_write_word(cm36657, REG_CS_CONF1, cm36657->ls_cmd);

	/*must disable p-sensor interrupt before IST create*/
	/*disable PS func*/
	cm36657->ps_conf1_val |= CM36657_PS_SD;
	cm36657->ps_conf1_val &= CM36657_PS_INT_MASK;
	cm36657_i2c_write_word(cm36657, REG_PS_CONF1, cm36657->ps_conf1_val);
	cm36657_i2c_write_word(cm36657, REG_PS_CONF3, cm36657->ps_conf3_val);
	cm36657_i2c_write_word(cm36657, REG_PS_THDL, cm36657_thd_tbl[1][0]);
	cm36657_i2c_write_word(cm36657, REG_PS_THDH, cm36657_thd_tbl[0][1]);

	SENSOR_INFO("is success\n");
	return err;
}

static int cm36657_setup_irq(struct cm36657_data *cm36657)
{
	int rc = -EIO;

	rc = gpio_request(cm36657->irq_gpio, "gpio_proximity_out");
	if (rc < 0) {
		SENSOR_ERR("gpio %d request failed err(%d)\n",
				cm36657->irq, rc);
		return rc;
	}

	rc = gpio_direction_input(cm36657->irq_gpio);
	if (rc < 0) {
		SENSOR_ERR("failed to set gpio %d as input(%d)\n",
			cm36657->irq_gpio, rc);
		goto err_gpio_direction_input;
	}

	cm36657->irq = gpio_to_irq(cm36657->irq_gpio);

	rc = request_any_context_irq(cm36657->irq, cm36657_irq_handler,
			IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
			"cm36657", cm36657);

	if (rc < 0) {
		SENSOR_ERR("irq %d failed for qpio %d err(%d)\n",
			cm36657->irq, cm36657->irq_gpio, rc);
		goto err_request_irq;
	}

	/* start with interrupts disabled */
	disable_irq(cm36657->irq);

	SENSOR_ERR("dir out success\n");

	goto done;

err_request_irq:
err_gpio_direction_input:
	gpio_free(cm36657->irq_gpio);
done:
	return rc;
}

/* This function is for light sensor.  It operates every a few seconds.
 * It asks for work to be done on a thread because i2c needs a thread
 * context (slow and blocking) and then reschedules the timer to run again.
 */
static enum hrtimer_restart cm36657_light_timer_func(struct hrtimer *timer)
{
	struct cm36657_data *cm36657
		= container_of(timer, struct cm36657_data, light_timer);
	queue_work(cm36657->light_wq, &cm36657->work_light);
	hrtimer_forward_now(&cm36657->light_timer, cm36657->light_poll_delay);
	return HRTIMER_RESTART;
}

static void cm36657_work_func_light(struct work_struct *work)
{
	struct cm36657_data *cm36657 = container_of(work, struct cm36657_data,
						work_light);
	mutex_lock(&cm36657->read_lock);

	cm36657_i2c_read_word(cm36657, REG_ALS_RED_DATA, &cm36657->red_data);
	cm36657_i2c_read_word(cm36657, REG_ALS_GREEN_DATA,
			&cm36657->green_data);
	cm36657_i2c_read_word(cm36657, REG_ALS_BLUE_DATA, &cm36657->blue_data);
	cm36657_i2c_read_word(cm36657, REG_WHITE_DATA, &cm36657->white_data);
	mutex_unlock(&cm36657->read_lock);

	input_report_rel(cm36657->light_input_dev, REL_HWHEEL,
		cm36657->red_data + 1);
	input_report_rel(cm36657->light_input_dev, REL_DIAL,
		cm36657->green_data + 1);
	input_report_rel(cm36657->light_input_dev, REL_WHEEL,
		cm36657->blue_data + 1);
	input_report_rel(cm36657->light_input_dev, REL_MISC,
		cm36657->white_data + 1);
	input_sync(cm36657->light_input_dev);

	if (cm36657->count_log_time >= LIGHT_LOG_TIME) {
		SENSOR_INFO("%u,%u,%u,%u\n",
			cm36657->red_data, cm36657->green_data,
			cm36657->blue_data, cm36657->white_data);
		cm36657->count_log_time = 0;
	} else
		cm36657->count_log_time++;
}

static void cm36657_work_func_prox(struct work_struct *work)
{
	struct cm36657_data *cm36657 = container_of(work, struct cm36657_data,
						  work_prox_avg);
	cm36657_get_avg_val(cm36657);
}

static enum hrtimer_restart cm36657_prox_timer_func(struct hrtimer *timer)
{
	struct cm36657_data *cm36657
			= container_of(timer, struct cm36657_data, prox_timer);
	queue_work(cm36657->prox_avg_wq, &cm36657->work_prox_avg);
	hrtimer_forward_now(&cm36657->prox_timer, cm36657->prox_poll_delay);
	return HRTIMER_RESTART;
}

/* device tree parsing function */
static int cm36657_parse_dt(struct cm36657_data *cm36657, struct device *dev)
{
	struct device_node *np;
	struct pinctrl *p;
	enum of_gpio_flags flags;
	u32 temp;
	int ret;

	np = dev->of_node;
	if (!np)
		return -EINVAL;

	cm36657->irq_gpio = of_get_named_gpio_flags(np,
					"cm36657,irq_gpio", 0, &flags);
	if (cm36657->irq_gpio < 0) {
		SENSOR_ERR("get prox_int error\n");
		return -ENODEV;
	}

	cm36657->vdd_ldo_pin = of_get_named_gpio_flags(np,
				"cm36657,vdd_ldo_pin", 0, &flags);
	if (cm36657->vdd_ldo_pin < 0) {
		SENSOR_INFO("Cannot set vdd_ldo_pin through DTSI\n");
		cm36657->vdd_ldo_pin = 0;
	} else {
		ret = gpio_request(cm36657->vdd_ldo_pin, "cm36657_vdd_en");
		if (ret < 0)
			SENSOR_ERR("gpio %d request failed %d\n",
				cm36657->vdd_ldo_pin, ret);
		else {
			gpio_direction_output(cm36657->vdd_ldo_pin, 0);
		}
	}

	cm36657->led_ldo_pin = of_get_named_gpio_flags(np,
				"cm36657,led_ldo_pin", 0, &flags);
	if (cm36657->led_ldo_pin < 0) {
		SENSOR_INFO("Cannot set vdd_ldo_pin through DTSI\n");
		cm36657->led_ldo_pin = 0;
	} else {
		ret = gpio_request(cm36657->led_ldo_pin, "cm36657_vdd_en");
		if (ret < 0)
			SENSOR_ERR("gpio %d request failed %d\n",
				cm36657->led_ldo_pin, ret);
		else
			gpio_direction_output(cm36657->led_ldo_pin, 0);
	}

	if (of_property_read_string(np, "cm36657,vled-regulator",
			&cm36657->led_vdd) < 0) {
		SENSOR_INFO("not use vled-regulator\n");
		cm36657->led_vdd = 0;
	} else {
		cm36657->prox_vled = regulator_get(NULL, cm36657->led_vdd);
		if (IS_ERR(cm36657->prox_vled) ||
				(cm36657->prox_vled) == NULL) {
			cm36657->prox_vled = 0;
			SENSOR_ERR("regulator_get fail\n");
		} else {
			SENSOR_INFO("vled-regulator ok\n");
		}
	}

	if (of_property_read_u32(np, "cm36657,thresh_high",
		&temp) < 0)
		SENSOR_INFO("Cannot set thresh_high through DTSI\n");
	else
		cm36657_thd_tbl[0][1] = temp;

	if (of_property_read_u32(np, "cm36657,thresh_low",
		&temp) < 0)
		SENSOR_INFO("Cannot set thresh_low through DTSI\n");
	else
		cm36657_thd_tbl[1][0] = temp;

	if (of_property_read_u32(np, "cm36657,thresh_detect_high",
		&temp) < 0)
		SENSOR_INFO("Cannot set thresh_detect_high through DTSI\n");
	else
		cm36657_thd_tbl[1][1] = temp;

	if (of_property_read_u32(np, "cm36657,thresh_detect_low",
		&temp) < 0)
		SENSOR_INFO("Cannot set thresh_detect_low through DTSI\n");
	else
		cm36657_thd_tbl[2][0] = temp;

	SENSOR_INFO("thresh_high:%d, thresh_low:%d\n",
		cm36657_thd_tbl[0][1], cm36657_thd_tbl[1][0]);
	SENSOR_INFO("thresh_detect_high:%d, thresh_detect_low:%d\n",
		cm36657_thd_tbl[1][1], cm36657_thd_tbl[2][0]);

	/* Proximity CONF1 register Setting */
	if (of_property_read_u32(np, "cm36657,ps_conf1_reg", &temp) < 0) {
		SENSOR_INFO("Cannot set ps_conf1_reg through DTSI\n");
		cm36657->ps_conf1_val = CM36657_PS_IT_4T |
				CM36657_PS_PERS_4 | CM36657_PS_START;
	} else {
		cm36657->ps_conf1_val = temp;
	}

	/* Proximity CONF3 register Setting */
	if (of_property_read_u32(np, "cm36657,ps_conf3_reg", &temp) < 0) {
		SENSOR_INFO("Cannot set ps_conf3_reg through DTSI\n");
		cm36657->ps_conf3_val = CM36657_LED_I_110 | CM36657_PS_START2;
	} else {
		cm36657->ps_conf3_val = temp;
	}

	cm36657->offset = 0;

	p = pinctrl_get_select_default(dev);
	if (IS_ERR(p)) {
		SENSOR_INFO("failed pinctrl_get\n");
		return -EINVAL;
	}

	return 0;
}

static int cm36657_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int ret = -ENODEV;
	struct cm36657_data *cm36657 = NULL;

	SENSOR_INFO("Probe Start!\n");
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		SENSOR_ERR("i2c functionality check failed!\n");
		return ret;
	}

	cm36657 = kzalloc(sizeof(struct cm36657_data), GFP_KERNEL);
	if (!cm36657) {
		SENSOR_ERR("failed to alloc memory for RGB sensor data\n");
		return -ENOMEM;
	}

	ret = cm36657_parse_dt(cm36657, &client->dev);
	if (ret)
		goto err_devicetree;

	cm36657_vdd_onoff(cm36657, ON);

	cm36657->i2c_client = client;
	i2c_set_clientdata(client, cm36657);
	cm36657->cur_lv = 0;

	mutex_init(&cm36657->power_lock);
	mutex_init(&cm36657->read_lock);
	mutex_init(&cm36657->control_mutex);

	/* wake lock init for proximity sensor */
	wake_lock_init(&cm36657->prox_wake_lock, WAKE_LOCK_SUSPEND,
		"prox_wake_lock");

	/* Check if the device is there or not. */
	ret = cm36657_i2c_write_word(cm36657, REG_CS_CONF1, 0x0001);
	if (ret < 0) {
		SENSOR_ERR("cm36657 is not connected.(%d)\n", ret);
		goto err_setup_reg;
	}

	/* setup initial registers */
	ret = cm36657_setup_reg(cm36657);
	if (ret < 0) {
		SENSOR_ERR("could not setup regs\n");
		goto err_setup_reg;
	}

	/* allocate proximity input_device */
	cm36657->prox_input_dev = input_allocate_device();
	if (!cm36657->prox_input_dev) {
		SENSOR_ERR("could not allocate proximity input device\n");
		goto err_input_allocate_device_proximity;
	}

	input_set_drvdata(cm36657->prox_input_dev, cm36657);
	cm36657->prox_input_dev->name = "proximity_sensor";
	input_set_capability(cm36657->prox_input_dev, EV_ABS, ABS_DISTANCE);
	input_set_abs_params(cm36657->prox_input_dev, ABS_DISTANCE, 0, 1, 0, 0);

	ret = input_register_device(cm36657->prox_input_dev);
	if (ret < 0) {
		input_free_device(cm36657->prox_input_dev);
		SENSOR_ERR("could not register input device\n");
		goto err_input_register_device_proximity;
	}

	ret = sensors_create_symlink(&cm36657->prox_input_dev->dev.kobj,
					cm36657->prox_input_dev->name);
	if (ret < 0) {
		SENSOR_ERR("create_symlink error\n");
		goto err_sensors_create_symlink_prox;
	}

	ret = sysfs_create_group(&cm36657->prox_input_dev->dev.kobj,
				 &proximity_attribute_group);
	if (ret) {
		SENSOR_ERR("could not create sysfs group\n");
		goto err_sysfs_create_group_proximity;
	}

	ret = cm36657_setup_irq(cm36657);
	if (ret) {
		SENSOR_ERR("could not setup irq\n");
		goto err_setup_irq;
	}

	/* For factory test mode, we use timer to get average proximity data. */
	/* prox_timer settings. we poll for light values using a timer. */
	hrtimer_init(&cm36657->prox_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	cm36657->prox_poll_delay = ns_to_ktime(2000 * NSEC_PER_MSEC);/*2 sec*/
	cm36657->prox_timer.function = cm36657_prox_timer_func;

	/* the timer just fires off a work queue request. */
	/*  we need a thread to read the i2c (can be slow and blocking). */
	cm36657->prox_wq = create_singlethread_workqueue("cm36657_prox_wq");
	if (!cm36657->prox_wq) {
		ret = -ENOMEM;
		SENSOR_ERR("could not create prox workqueue\n");
		goto err_create_prox_workqueue;
	}
	cm36657->prox_avg_wq =
		create_singlethread_workqueue("cm36657_prox_avg_wq");
	if (!cm36657->prox_avg_wq) {
		ret = -ENOMEM;
		SENSOR_ERR("could not create prox avg workqueue\n");
		goto err_create_prox_avg_workqueue;
	}
	/* this is the thread function we run on the work queue */
	INIT_WORK(&cm36657->work_prox, cm36657_irq_do_work);
	INIT_WORK(&cm36657->work_prox_avg, cm36657_work_func_prox);

	/* allocate lightsensor input_device */
	cm36657->light_input_dev = input_allocate_device();
	if (!cm36657->light_input_dev) {
		SENSOR_ERR("could not allocate light input device\n");
		goto err_input_allocate_device_light;
	}

	input_set_drvdata(cm36657->light_input_dev, cm36657);
	cm36657->light_input_dev->name = "light_sensor";
	input_set_capability(cm36657->light_input_dev, EV_REL, REL_HWHEEL);
	input_set_capability(cm36657->light_input_dev, EV_REL, REL_MISC);
	input_set_capability(cm36657->light_input_dev, EV_REL, REL_DIAL);
	input_set_capability(cm36657->light_input_dev, EV_REL, REL_WHEEL);

	ret = input_register_device(cm36657->light_input_dev);
	if (ret < 0) {
		input_free_device(cm36657->light_input_dev);
		SENSOR_ERR("could not register input device\n");
		goto err_input_register_device_light;
	}

	ret = sensors_create_symlink(&cm36657->light_input_dev->dev.kobj,
					cm36657->light_input_dev->name);
	if (ret < 0) {
		SENSOR_ERR("create_symlink error\n");
		goto err_sensors_create_symlink_light;
	}

	ret = sysfs_create_group(&cm36657->light_input_dev->dev.kobj,
				 &light_attribute_group);
	if (ret) {
		SENSOR_ERR("could not create sysfs group\n");
		goto err_sysfs_create_group_light;
	}

	/* light_timer settings. we poll for light values using a timer. */
	hrtimer_init(&cm36657->light_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	cm36657->light_poll_delay = ns_to_ktime(200 * NSEC_PER_MSEC);
	cm36657->light_timer.function = cm36657_light_timer_func;

	/* the timer just fires off a work queue request. */
	/* we need a thread to read the i2c (can be slow and blocking). */
	cm36657->light_wq = create_singlethread_workqueue("cm36657_light_wq");
	if (!cm36657->light_wq) {
		ret = -ENOMEM;
		SENSOR_ERR("could not create light workqueue\n");
		goto err_create_light_workqueue;
	}

	/* this is the thread function we run on the work queue */
	INIT_WORK(&cm36657->work_light, cm36657_work_func_light);

	/* set sysfs for proximity sensor */
	ret = sensors_register(&cm36657->prox_dev,
		cm36657, prox_sensor_attrs, "proximity_sensor");
	if (ret) {
		SENSOR_ERR("can't register prox sensor device(%d)\n", ret);
		goto prox_sensor_register_failed;
	}

	/* set sysfs for light sensor */
	ret = sensors_register(&cm36657->light_dev,
		cm36657, light_sensor_attrs, "light_sensor");
	if (ret) {
		SENSOR_ERR("can't register light sensor device(%d)\n", ret);
		goto light_sensor_register_failed;
	}

	SENSOR_INFO("is success.\n");
	goto done;

light_sensor_register_failed:
	sensors_unregister(cm36657->prox_dev, prox_sensor_attrs);
prox_sensor_register_failed:
	destroy_workqueue(cm36657->light_wq);
err_create_light_workqueue:
	sysfs_remove_group(&cm36657->light_input_dev->dev.kobj,
			   &light_attribute_group);
err_sysfs_create_group_light:
	sensors_remove_symlink(&cm36657->light_input_dev->dev.kobj,
			cm36657->light_input_dev->name);
err_sensors_create_symlink_light:
	input_unregister_device(cm36657->light_input_dev);
err_input_register_device_light:
err_input_allocate_device_light:
	destroy_workqueue(cm36657->prox_avg_wq);
err_create_prox_avg_workqueue:
	destroy_workqueue(cm36657->prox_wq);
err_create_prox_workqueue:
	free_irq(cm36657->irq, cm36657);
	gpio_free(cm36657->irq_gpio);
err_setup_irq:
	sysfs_remove_group(&cm36657->prox_input_dev->dev.kobj,
			   &proximity_attribute_group);
err_sysfs_create_group_proximity:
	sensors_remove_symlink(&cm36657->prox_input_dev->dev.kobj,
			cm36657->prox_input_dev->name);
err_sensors_create_symlink_prox:
	input_unregister_device(cm36657->prox_input_dev);
err_input_register_device_proximity:
err_input_allocate_device_proximity:
err_setup_reg:
	wake_lock_destroy(&cm36657->prox_wake_lock);
	mutex_destroy(&cm36657->control_mutex);
	mutex_destroy(&cm36657->read_lock);
	mutex_destroy(&cm36657->power_lock);
	if (cm36657->vdd_ldo_pin)
		gpio_free(cm36657->vdd_ldo_pin);
err_devicetree:
	kfree(cm36657);
done:
	return ret;
}

static int cm36657_i2c_remove(struct i2c_client *client)
{
	struct cm36657_data *cm36657 = i2c_get_clientdata(client);

	/* free irq */
	if (cm36657->power_state & PROXIMITY_ENABLED) {
		disable_irq_wake(cm36657->irq);
		disable_irq(cm36657->irq);
	}
	free_irq(cm36657->irq, cm36657);
	gpio_free(cm36657->irq_gpio);

	/* device off */
	if (cm36657->power_state & LIGHT_ENABLED)
		cm36657_light_disable(cm36657);
	if (cm36657->power_state & PROXIMITY_ENABLED)
		cm36657_i2c_write_word(cm36657, REG_PS_CONF1, 0x0001);

	/* destroy workqueue */
	destroy_workqueue(cm36657->light_wq);
	destroy_workqueue(cm36657->prox_wq);
	destroy_workqueue(cm36657->prox_avg_wq);

	/* sysfs destroy */
	sensors_unregister(cm36657->light_dev, light_sensor_attrs);
	sensors_unregister(cm36657->prox_dev, prox_sensor_attrs);
	sensors_remove_symlink(&cm36657->light_input_dev->dev.kobj,
			cm36657->light_input_dev->name);
	sensors_remove_symlink(&cm36657->prox_input_dev->dev.kobj,
			cm36657->prox_input_dev->name);

	/* input device destroy */
	sysfs_remove_group(&cm36657->light_input_dev->dev.kobj,
				&light_attribute_group);
	input_unregister_device(cm36657->light_input_dev);
	sysfs_remove_group(&cm36657->prox_input_dev->dev.kobj,
				&proximity_attribute_group);
	input_unregister_device(cm36657->prox_input_dev);
	/* lock destroy */
	mutex_destroy(&cm36657->read_lock);
	mutex_destroy(&cm36657->power_lock);
	wake_lock_destroy(&cm36657->prox_wake_lock);

	kfree(cm36657);

	return 0;
}

static void cm36657_i2c_shutdown(struct i2c_client *client)
{
	struct cm36657_data *cm36657 = i2c_get_clientdata(client);

	if (cm36657->power_state & LIGHT_ENABLED)
		cm36657_light_disable(cm36657);
	if (cm36657->power_state & PROXIMITY_ENABLED) {
		disable_irq_wake(cm36657->irq);
		disable_irq(cm36657->irq);
		cm36657_i2c_write_word(cm36657, REG_PS_CONF1, 0x0001);
		cm36657_prox_vled_onoff(cm36657, OFF);
	}

	SENSOR_INFO("is called.\n");
}

static int cm36657_suspend(struct device *dev)
{
	/* We disable power only if proximity is disabled.  If proximity
	 * is enabled, we leave power on because proximity is allowed
	 * to wake up device.  We remove power without changing
	 * cm36657->power_state because we use that state in resume.
	 */
	struct cm36657_data *cm36657 = dev_get_drvdata(dev);

	if (cm36657->power_state & LIGHT_ENABLED)
		cm36657_light_disable(cm36657);
	SENSOR_INFO("is called.\n");

	return 0;
}

static int cm36657_resume(struct device *dev)
{
	struct cm36657_data *cm36657 = dev_get_drvdata(dev);

	if (cm36657->power_state & LIGHT_ENABLED)
		cm36657_light_enable(cm36657);
	SENSOR_INFO("is called.\n");

	return 0;
}

static const struct of_device_id cm36657_match_table[] = {
	{ .compatible = "cm36657",},
	{},
};

static const struct i2c_device_id cm36657_device_id[] = {
	{"cm36657", 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, cm36657_device_id);

static const struct dev_pm_ops cm36657_pm_ops = {
	.suspend = cm36657_suspend,
	.resume = cm36657_resume
};

static struct i2c_driver cm36657_i2c_driver = {
	.driver = {
		   .name = "cm36657",
		   .owner = THIS_MODULE,
		   .of_match_table = cm36657_match_table,
		   .pm = &cm36657_pm_ops
	},
	.probe = cm36657_i2c_probe,
	.shutdown = cm36657_i2c_shutdown,
	.remove = cm36657_i2c_remove,
	.id_table = cm36657_device_id,
};

static int __init cm36657_init(void)
{
	return i2c_add_driver(&cm36657_i2c_driver);
}

static void __exit cm36657_exit(void)
{
	i2c_del_driver(&cm36657_i2c_driver);
}

module_init(cm36657_init);
module_exit(cm36657_exit);

MODULE_AUTHOR("Samsung Electronics");
MODULE_DESCRIPTION("RGB Sensor device driver for cm36657");
MODULE_LICENSE("GPL");
