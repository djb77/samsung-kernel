/*
 * Copyright (C) 2017-2018 SAMSUNG
 * Author: yunjae Hwang <yjz.hwang@samsung.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/irq.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/wakelock.h>
#include <linux/jiffies.h>
#include <linux/of_gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/pinctrl/consumer.h>
#include "cm36658.h"
#include <linux/sensor/sensors_core.h>

#ifdef TAG
#undef TAG
#define TAG "[PROX]"
#endif

#define VENDOR			"CAPELLA"
#define CHIP_ID			"CM36658"

/* for i2c Write */
#define I2C_M_WR		0

#define REL_RED		REL_HWHEEL
#define REL_GREEN	REL_DIAL
#define REL_BLUE	REL_WHEEL
#define REL_IR		REL_MISC
#define REL_GAIN	REL_Z

#define ABS_CUR_LEVEL		ABS_X
#define ABS_INT_DIRECTION	ABS_Y

#define PS_CANCELLATION_ARRAY_NUM 3
#define PS_CANCELLATION_POLLING_DELAY 10

#define CONTROL_INT_ISR_PS_REPORT	0x00
#define CONTROL_ALS					0x01
#define CONTROL_PS					0x02
#define CONTROL_ALS_REPORT			0x03

 /*lightsensor log time 6SEC 200mec X 30*/
#define LIGHT_LOG_TIME		30

#define PROX_READ_NUM		40

#define DEFAULT_ADC			10
#define ADC_MAX_VALUE		4095
#define CAL_FAIL_ADC		4000

#define LEVEL1_THDL 		1
#define LEVEL1_THDH 		110
#define LEVEL2_THDL 		70
#define LEVEL2_THDH 		3500
#define LEVEL3_THDL 		2000
#define LEVEL3_THDH 		ADC_MAX_VALUE

uint16_t cm36658_thd_tbl[3][2]={
{ LEVEL1_THDL, LEVEL1_THDH },
{ LEVEL2_THDL, LEVEL2_THDH },
{ LEVEL3_THDL, LEVEL3_THDH },
};

enum {
	OFF = 0,
	ON = 1
};

struct cm36658_data {
	struct i2c_client *i2c_client;
	struct device *light_dev;
	struct device *prox_dev;

	struct input_dev *light_input_dev;
	struct input_dev *prox_input_dev;
	struct mutex power_lock;
	struct mutex read_lock;
	struct mutex control_mutex;

	struct work_struct work_light;
	struct work_struct work_prox;
	struct work_struct work_prox_avg;
	struct hrtimer light_timer;
	struct hrtimer prox_timer;
	struct workqueue_struct *light_wq;
	struct workqueue_struct *prox_wq;
	struct workqueue_struct *prox_avg_wq;
	ktime_t light_poll_delay;
	ktime_t prox_poll_delay;

	int irq_gpio;
	int als_enable;
	int ps_enable;
	int ps_irq_flag;
	int als_enabled_before_suspend;
	int autocal;

	int irq;
	int vdd_ldo_pin;
	int led_ldo_pin;

	int (*power)(int, uint8_t); /* power to the chip */

	struct wake_lock prox_wake_lock;

	uint32_t current_lux;
	uint16_t current_adc;
	uint16_t inte_cancel_set;
	uint16_t ps_conf1_val;
	uint16_t ps_conf3_val;
	uint16_t red_data;
	uint16_t green_data;
	uint16_t blue_data;
	uint16_t ir_data;
	int avg[3];
	int count_log_time;

	int offset;
	int cur_lv;
	int cs_gain;
	uint16_t ls_cmd;
};
struct cm36658_data *lp_info;
static int control_and_report(struct cm36658_data *cm36658, uint8_t mode);

static int cm36658_vdd_onoff(struct cm36658_data *cm36658, int onoff)
{
	/* ldo control */
	if (cm36658->vdd_ldo_pin) {
		gpio_set_value(cm36658->vdd_ldo_pin, onoff);
		if (onoff)
			msleep(20);
		return 0;
	}
	return 0;
}

static int cm36658_led_onoff(struct cm36658_data *cm36658, int onoff)
{
	/* led_ldo control */
	if (cm36658->led_ldo_pin) {
		gpio_set_value(cm36658->led_ldo_pin, onoff);
		if (onoff)
			msleep(20);
		return 0;
	}
	return 0;
}

static int cm36658_i2c_read_word(struct cm36658_data *cm36658, u8 command,
				u16 *val)
{
	int err = 0;
	int retry = 3;
	struct i2c_client *client = cm36658->i2c_client;
	struct i2c_msg msg[2];
	unsigned char data[2] = {0,};
	u16 value = 0;

	if ((client == NULL) || (!client->adapter))
		return -ENODEV;

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

	while (retry--) {
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

static int cm36658_i2c_write_word(struct cm36658_data *cm36658, u8 command,
				u16 val)
{
	int err = 0;
	struct i2c_client *client = cm36658->i2c_client;
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

static void cm36658_get_avg_val(struct cm36658_data *cm36658)
{
	int min = 0, max = 0, avg = 0;
	int i;
	u16 ps_data = 0;

	for (i = 0; i < PROX_READ_NUM; i++) {
		msleep(40);
		cm36658_i2c_read_word(cm36658, PS_DATA, &ps_data);
		avg += ps_data;

		if (!i)
			min = ps_data;
		else if (ps_data < min)
			min = ps_data;

		if (ps_data > max)
			max = ps_data;
	}
	avg /= PROX_READ_NUM;

	cm36658->avg[0] = min;
	cm36658->avg[1] = avg;
	cm36658->avg[2] = max;
}

static int cm36658_get_ps_adc_value(uint16_t *data)
{
	int ret = 0;
	struct cm36658_data *cm36658 = lp_info;

	if (data == NULL)
		return -EFAULT;

	ret = cm36658_i2c_read_word(cm36658, PS_DATA, data);

	if (ret < 0) {
		SENSOR_ERR("cm36658_i2c_read_word fail\n");
		return -EIO;
	} else {
		SENSOR_INFO("cm36658_i2c_read_word OK 0x%04x\n", *data);
	}

	return ret;
}

static int cm36658_get_calibration_result(struct cm36658_data *cm36658, int is_first)
{
	int ret = 0;
	int i = 0;
	uint16_t value[PS_CANCELLATION_ARRAY_NUM];
	uint32_t ps_average = 0;

	/* Disable interrupt */
	cm36658_i2c_read_word(cm36658, PS_CONF1, &cm36658->ps_conf1_val);
	cm36658->ps_conf1_val &= CM36658_PS_INT_MASK;
	cm36658_i2c_write_word(cm36658, PS_CONF1, cm36658->ps_conf1_val);

	/* Initialize */
	cm36658_i2c_write_word(cm36658, PS_CANC, 0);
	cm36658->autocal = 1;

	for (i = 0; i < PS_CANCELLATION_ARRAY_NUM; i++) {
		/* delay 32ms(PS_PERIOD) * 1.5 */
		msleep(48);
		ret = cm36658_get_ps_adc_value(&value[i]);
		if (ret < 0) {
			pr_err("[PS_ERR][cm36658 error]%s: cm36658_get_ps_adc_value\n",
				__func__);
			return ret;
		}
		ps_average += value[i];
	}
	ps_average /= PS_CANCELLATION_ARRAY_NUM;

	if (ps_average < DEFAULT_ADC)
		cm36658->offset = ps_average;
	else if (is_first && (ps_average >= 3000))
		cm36658->offset = ps_average / 2;
	else if (ps_average >= 4000)
		cm36658->offset = 4000;
	else
		cm36658->offset = ps_average - DEFAULT_ADC;

	SENSOR_INFO("ps_average : %d, offset : %d\n", ps_average,
			cm36658->offset);

	cm36658_i2c_write_word(cm36658, PS_CANC, cm36658->offset);
	/* delay for update PS_CANC */
	usleep_range(9000, 9000);

	return 0;
}

irqreturn_t cm36658_irq_handler(int irq, void *data)
{
	struct cm36658_data *cm36658 = data;

	SENSOR_INFO("!!\n");
	disable_irq_nosync(cm36658->irq);
	wake_lock_timeout(&cm36658->prox_wake_lock, 3 * HZ);
	queue_work(cm36658->prox_wq, &cm36658->work_prox);

	return IRQ_HANDLED;
}

static void ls_initial_cmd(struct cm36658_data *cm36658)
{
	/*must disable l-sensor interrupt before IST create*//*disable ALS func*/
	cm36658->ls_cmd |= CM36658_CS_SD;
	cm36658_i2c_write_word(cm36658, CS_CONF, cm36658->ls_cmd);
}

static void psensor_initial_cmd(struct cm36658_data *cm36658)
{
	/* must disable p-sensor interrupt before IST create *//*disable PS func*/
	cm36658->ps_conf1_val |= CM36658_PS_SD;
	cm36658->ps_conf1_val &= CM36658_PS_INT_MASK;
	cm36658_i2c_write_word(cm36658, PS_CONF1, cm36658->ps_conf1_val);
	cm36658_i2c_write_word(cm36658, PS_CONF3, cm36658->ps_conf3_val);

	cm36658_i2c_write_word(cm36658, PS_THDL,  cm36658_thd_tbl[1][0]);
	cm36658_i2c_write_word(cm36658, PS_THDH,  cm36658_thd_tbl[0][1]);
}

static void cm36658_power_onoff(struct cm36658_data *cm36658, int onoff)
{
	if (onoff) {
		cm36658->ls_cmd |= CM36658_CS_STANDBY;
		cm36658_i2c_write_word(cm36658, CS_CONF, cm36658->ls_cmd);
		cm36658->ls_cmd |= CM36658_CS_START;
		cm36658_i2c_write_word(cm36658, CS_CONF, cm36658->ls_cmd);
	} else {
		cm36658->ls_cmd &= ~CM36658_CS_STANDBY;
		cm36658_i2c_write_word(cm36658, CS_CONF, cm36658->ls_cmd);
	}
}

static void cm36658_proximity_enable(struct cm36658_data *cm36658)
{
	mutex_lock(&cm36658->power_lock);
	SENSOR_INFO("\n");

	if (cm36658->ps_enable == ON) {
		SENSOR_INFO("already enabled\n");
	} else {
		cm36658_led_onoff(cm36658, ON);
		cm36658->ps_enable = ON;
		if (cm36658->als_enable == 0)
			cm36658_power_onoff(cm36658, 1);
		cm36658->ps_conf1_val &= CM36658_PS_SD_MASK;
		cm36658->ps_conf1_val |= CM36658_PS_INT_ENABLE;
		cm36658_i2c_write_word(cm36658, PS_CONF1, cm36658->ps_conf1_val);
		control_and_report(cm36658, CONTROL_PS);

		enable_irq(cm36658->irq);
		enable_irq_wake(cm36658->irq);
	}

	mutex_unlock(&cm36658->power_lock);
}

static void cm36658_proximity_disable(struct cm36658_data *cm36658)
{
	mutex_lock(&cm36658->power_lock);
	SENSOR_INFO("\n");

	if (cm36658->ps_enable == ON) {
		disable_irq_wake(cm36658->irq);
		disable_irq(cm36658->irq);
		cm36658->ps_conf1_val |= CM36658_PS_SD;
		cm36658->ps_conf1_val &= CM36658_PS_INT_MASK;
		cm36658_i2c_write_word(cm36658, PS_CONF1, cm36658->ps_conf1_val);
		cm36658->cur_lv = 0;
		cm36658_i2c_write_word(cm36658, PS_THDL,
				cm36658_thd_tbl[cm36658->cur_lv][0]);
		cm36658_i2c_write_word(cm36658, PS_THDH,
				cm36658_thd_tbl[cm36658->cur_lv][1]);
		cm36658->ps_enable = OFF;
		cm36658_led_onoff(cm36658, OFF);
	}

	if (cm36658->als_enable == 0)
			cm36658_power_onoff(cm36658, 0);

	mutex_unlock(&cm36658->power_lock);
}

static void cm36658_light_enable(struct cm36658_data *cm36658)
{
	mutex_lock(&cm36658->power_lock);

	if (cm36658->als_enable == OFF) {
		cm36658->als_enable = ON;
		if (cm36658->ps_enable == 0)
			cm36658_power_onoff(cm36658, 1);
		cm36658->ls_cmd &= CM36658_CS_SD_MASK;
		cm36658_i2c_write_word(cm36658, CS_CONF, cm36658->ls_cmd);
		hrtimer_start(&cm36658->light_timer, ns_to_ktime(75 * NSEC_PER_MSEC),
			HRTIMER_MODE_REL);
	}

	mutex_unlock(&cm36658->power_lock);
}

static void cm36658_light_disable(struct cm36658_data *cm36658)
{
	mutex_lock(&cm36658->power_lock);

	if (cm36658->als_enable == ON) {
		cm36658->ls_cmd |= CM36658_CS_SD;
		cm36658_i2c_write_word(cm36658, CS_CONF, cm36658->ls_cmd);
		hrtimer_cancel(&cm36658->light_timer);
		cancel_work_sync(&cm36658->work_light);
		cm36658->als_enable = OFF;
	}

	if (cm36658->ps_enable == 0)
			cm36658_power_onoff(cm36658, 0);

	mutex_unlock(&cm36658->power_lock);
}

static ssize_t cm36658_poll_delay_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct cm36658_data *cm36658 = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%lld\n",
		ktime_to_ns(cm36658->light_poll_delay));
}

static ssize_t cm36658_poll_delay_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct cm36658_data *cm36658 = dev_get_drvdata(dev);
	int64_t new_delay;
	int err;

	err = kstrtoll(buf, 10, &new_delay);
	if (err < 0)
		return err;

	mutex_lock(&cm36658->power_lock);
	if (new_delay != ktime_to_ns(cm36658->light_poll_delay)) {
		SENSOR_INFO("poll_delay = %lld\n", new_delay);
		cm36658->light_poll_delay = ns_to_ktime(new_delay);
	}
	mutex_unlock(&cm36658->power_lock);

	return size;
}

static ssize_t proximity_enable_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct cm36658_data *cm36658 = lp_info;

	return sprintf(buf, "%d\n", cm36658->ps_enable);
}

static ssize_t proximity_enable_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int ps_en;
	struct cm36658_data *cm36658 = lp_info;

	ps_en = -1;
	sscanf(buf, "%d", &ps_en);

	if (ps_en != 0 && ps_en != 1) {
		SENSOR_ERR("invalid value %d\n", *buf);
		return count;
	}

	SENSOR_INFO("proximity sensor new_value = %d, old state = %d\n",
			ps_en, cm36658->ps_enable);

	if (ps_en)
		cm36658_proximity_enable(cm36658);
	else
		cm36658_proximity_disable(cm36658);

	return count;
}

static ssize_t ps_canc_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	int ret = 0;
	struct cm36658_data *cm36658 = lp_info;
	uint16_t ps_canc;

	ret = cm36658_i2c_read_word(cm36658, PS_CANC, &ps_canc);

	ret = sprintf(buf, "[PS][CM36658]PS_CANC = 0x%04x(%d)\n", ps_canc, ps_canc);

	return ret;
}

static ssize_t light_enable_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct cm36658_data *cm36658 = lp_info;

	return sprintf(buf, "%d\n", cm36658->als_enable);
}

static ssize_t light_enable_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	int ls_en;
	struct cm36658_data *cm36658 = dev_get_drvdata(dev);

	ls_en = -1;
	sscanf(buf, "%d", &ls_en);

	if (ls_en != 0 && ls_en != 1) {
		SENSOR_ERR("invalid value %d\n", *buf);
		return count;
	}

	SENSOR_INFO("light sensor new_value = %d, old state = %d\n",
			ls_en, cm36658->als_enable);

	if (ls_en)
		cm36658_light_enable(cm36658);
	else
		cm36658_light_disable(cm36658);

	return count;
}

static ssize_t ls_conf_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct cm36658_data *cm36658 = lp_info;
	return sprintf(buf, "CS_CONF = %x\n", cm36658->ls_cmd);
}
static ssize_t ls_conf_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct cm36658_data *cm36658 = dev_get_drvdata(dev);
	int value = 0;
	sscanf(buf, "0x%x", &value);

	cm36658->ls_cmd = value;
	SENSOR_INFO("CS_CONF = %x\n", cm36658->ls_cmd);

	cm36658_i2c_write_word(cm36658, CS_CONF, cm36658->ls_cmd);
	return count;
}

static ssize_t proximity_avg_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct cm36658_data *cm36658 = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d\n", cm36658->avg[0],
		cm36658->avg[1], cm36658->avg[2]);
}

static ssize_t proximity_avg_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct cm36658_data *cm36658 = dev_get_drvdata(dev);
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
		hrtimer_start(&cm36658->prox_timer, cm36658->prox_poll_delay,
			HRTIMER_MODE_REL);

	} else if (!new_value) {
		hrtimer_cancel(&cm36658->prox_timer);
		cancel_work_sync(&cm36658->work_prox_avg);
	}

	return size;
}

static ssize_t proximity_state_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct cm36658_data *cm36658 = dev_get_drvdata(dev);
	u16 ps_data;

	mutex_lock(&cm36658->read_lock);
	cm36658_i2c_read_word(cm36658, PS_DATA, &ps_data);
	mutex_unlock(&cm36658->read_lock);
	return snprintf(buf, PAGE_SIZE, "%u\n", ps_data);
}

static ssize_t proximity_thresh_high_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", cm36658_thd_tbl[0][1]);
}

static ssize_t proximity_thresh_high_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct cm36658_data *cm36658 = dev_get_drvdata(dev);
	u16 thresh_value;
	int err;

	err = kstrtou16(buf, 10, &thresh_value);
	if (err < 0)
		SENSOR_ERR("kstrtoint failed.\n");
	SENSOR_INFO("thresh_value:%u\n", thresh_value);

	if (thresh_value > 2) {
		cm36658_thd_tbl[0][1] = thresh_value;
		err = cm36658_i2c_write_word(cm36658, PS_THDH,
				cm36658_thd_tbl[0][1]);
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
	return snprintf(buf, PAGE_SIZE, "%d\n", cm36658_thd_tbl[1][0]);
}

static ssize_t proximity_thresh_low_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct cm36658_data *cm36658 = dev_get_drvdata(dev);
	u16 thresh_value;
	int err;

	err = kstrtou16(buf, 10, &thresh_value);
	if (err < 0)
		SENSOR_ERR("kstrtoint failed.");
	SENSOR_INFO("thresh_value:%u\n", thresh_value);

	if (thresh_value > 2) {
		cm36658_thd_tbl[1][0] = thresh_value;
		err = cm36658_i2c_write_word(cm36658, PS_THDL, cm36658_thd_tbl[1][0]);
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
	return snprintf(buf, PAGE_SIZE, "%d\n", cm36658_thd_tbl[1][1]);
}

static ssize_t proximity_thresh_detect_high_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct cm36658_data *cm36658 = dev_get_drvdata(dev);
	u16 thresh_value;
	int err;

	err = kstrtou16(buf, 10, &thresh_value);
	if (err < 0)
		SENSOR_ERR("kstrtoint failed.\n");
	SENSOR_INFO("thresh_value:%u\n", thresh_value);

	if (thresh_value > 2) {
		cm36658_thd_tbl[1][1] = thresh_value;
		err = cm36658_i2c_write_word(cm36658, PS_THDH,
				cm36658_thd_tbl[1][1]);
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
	return snprintf(buf, PAGE_SIZE, "%d\n", cm36658_thd_tbl[2][0]);
}

static ssize_t proximity_thresh_detect_low_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct cm36658_data *cm36658 = dev_get_drvdata(dev);
	u16 thresh_value;
	int err;

	err = kstrtou16(buf, 10, &thresh_value);
	if (err < 0)
		SENSOR_ERR("kstrtoint failed.");
	SENSOR_INFO("thresh_value:%u\n", thresh_value);

	if (thresh_value > 2) {
		cm36658_thd_tbl[2][0] = thresh_value;
		err = cm36658_i2c_write_word(cm36658, PS_THDH,
				cm36658_thd_tbl[2][0]);
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

static ssize_t proximity_trim_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct cm36658_data *cm36658 = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%u\n", cm36658->offset);
}

static ssize_t proximity_write_register_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int reg, val, ret;
	struct cm36658_data *cm36658 = dev_get_drvdata(dev);

	if (sscanf(buf, "%4x,%4x", &reg, &val) != 2) {
		SENSOR_ERR("invalid value\n");
		return count;
	}

	ret = cm36658_i2c_write_word(cm36658, reg, val);
	if (ret < 0)
		SENSOR_ERR("failed %d\n", ret);
	else
		SENSOR_INFO("Register(0x%4x) data(0x%4x)\n", reg, val);

	return count;
}

static ssize_t proximity_read_register_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cm36658_data *cm36658 = dev_get_drvdata(dev);

	u8 reg;
	int offset = 0;
	u16 val = 0;

	for (reg = 0x00; reg <= 0x08; reg++) {
		cm36658_i2c_read_word(cm36658, reg, &val);
		SENSOR_INFO("Read Reg: 0x%4x Value: 0x%4x\n", reg, val);
		offset += snprintf(buf + offset, PAGE_SIZE - offset,
			"Reg: 0x%4x Value: 0x%4x\n", reg, val);
	}

	cm36658_i2c_read_word(cm36658, INT_FLAG, &val);
	offset += snprintf(buf + offset, PAGE_SIZE - offset,
			"INT_FLAG: 0x%4x Value: 0x%4x\n", INT_FLAG, val);
	return offset;
}

static struct device_attribute dev_attr_ps_enable =
__ATTR(enable, 0664, proximity_enable_show, proximity_enable_store);
static struct device_attribute dev_attr_ps_canc =
__ATTR(ps_canc, 0444, ps_canc_show, NULL);

static struct attribute *proximity_sysfs_attrs[] = {
	&dev_attr_ps_enable.attr,
	&dev_attr_ps_canc.attr,
	NULL
};

static struct attribute_group proximity_attribute_group = {
	.attrs = proximity_sysfs_attrs,
};

static struct device_attribute dev_attr_enable =
__ATTR(enable, 0664, light_enable_show, light_enable_store);
static struct device_attribute dev_attr_ls_conf =
__ATTR(ls_conf, 0664, ls_conf_show, ls_conf_store);
static DEVICE_ATTR(poll_delay, 0644, cm36658_poll_delay_show,
	cm36658_poll_delay_store);

static struct attribute *light_sysfs_attrs[] = {
	&dev_attr_enable.attr,
	&dev_attr_ls_conf.attr,
	&dev_attr_poll_delay.attr,
	NULL
};

static struct attribute_group light_attribute_group = {
	.attrs = light_sysfs_attrs,
};

/* sysfs for vendor & name */
static ssize_t cm36658_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", VENDOR);
}

static ssize_t cm36658_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", CHIP_ID);
}
static struct device_attribute dev_attr_prox_sensor_vendor =
	__ATTR(vendor, 0444, cm36658_vendor_show, NULL);
static struct device_attribute dev_attr_light_sensor_vendor =
	__ATTR(vendor, 0444, cm36658_vendor_show, NULL);
static struct device_attribute dev_attr_prox_sensor_name =
	__ATTR(name, 0444, cm36658_name_show, NULL);
static struct device_attribute dev_attr_light_sensor_name =
	__ATTR(name, 0444, cm36658_name_show, NULL);

static DEVICE_ATTR(prox_trim, 0444,
	proximity_trim_show, NULL);
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

static struct device_attribute *prox_sensor_attrs[] = {
	&dev_attr_prox_sensor_vendor,
	&dev_attr_prox_sensor_name,
	&dev_attr_prox_avg,
	&dev_attr_state,
	&dev_attr_thresh_high,
	&dev_attr_thresh_low,
	&dev_attr_prox_raw,
	&dev_attr_prox_register,
	&dev_attr_thresh_detect_high,
	&dev_attr_thresh_detect_low,
	&dev_attr_prox_trim,
	NULL,
};

/* light sysfs */
static ssize_t light_lux_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct cm36658_data *cm36658 = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%u,%u,%u,%u\n",
		cm36658->red_data, cm36658->green_data,
		cm36658->blue_data, cm36658->ir_data);
}

static ssize_t light_data_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct cm36658_data *cm36658 = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%u,%u,%u,%u,%u\n",
		cm36658->red_data, cm36658->green_data,
		cm36658->blue_data, cm36658->ir_data, cm36658->cs_gain);
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

static void cm36658_work_func_light(struct work_struct *work)
{
	struct cm36658_data *cm36658 = container_of(work, struct cm36658_data,
						work_light);
	mutex_lock(&cm36658->read_lock);
	cm36658_i2c_read_word(cm36658, CS_R_DATA, &cm36658->red_data);
	cm36658_i2c_read_word(cm36658, CS_G_DATA, &cm36658->green_data);
	cm36658_i2c_read_word(cm36658, CS_B_DATA, &cm36658->blue_data);
	cm36658_i2c_read_word(cm36658, CS_IR_DATA, &cm36658->ir_data);
	mutex_unlock(&cm36658->read_lock);

	input_report_rel(cm36658->light_input_dev, REL_RED,
		cm36658->red_data + 1);
	input_report_rel(cm36658->light_input_dev, REL_GREEN,
		cm36658->green_data + 1);
	input_report_rel(cm36658->light_input_dev, REL_BLUE,
		cm36658->blue_data + 1);
	input_report_rel(cm36658->light_input_dev, REL_IR,
		cm36658->ir_data + 1);
	input_report_rel(cm36658->light_input_dev, REL_GAIN,
		cm36658->cs_gain);
	input_sync(cm36658->light_input_dev);

	if (cm36658->cs_gain == 8) {
		if (cm36658->red_data < 100 || cm36658->green_data < 100 || cm36658->blue_data < 100) {
			cm36658->ls_cmd &= CM36658_CS_GAIN_MASK;
			cm36658_i2c_write_word(cm36658, CS_CONF, cm36658->ls_cmd);
			cm36658->cs_gain = 1;
		}
	} else {
		if (cm36658->red_data > 60000 || cm36658->green_data > 60000 || cm36658->blue_data > 60000) {
			cm36658->ls_cmd |= CM36658_CS_GAIN;
			cm36658_i2c_write_word(cm36658, CS_CONF, cm36658->ls_cmd);
			cm36658->cs_gain = 8;
		}
	}

	if (cm36658->count_log_time >= LIGHT_LOG_TIME) {
		SENSOR_INFO("%u,%u,%u,%u\n",
			cm36658->red_data, cm36658->green_data,
			cm36658->blue_data, cm36658->ir_data);
		cm36658->count_log_time = 0;
	} else
		cm36658->count_log_time++;
}

static void cm36658_work_func_prox(struct work_struct *work)
{
	struct cm36658_data *cm36658 = container_of(work, struct cm36658_data,
						  work_prox_avg);

	cm36658_get_avg_val(cm36658);
}

/* This function is for light sensor.  It operates every a few seconds.
 * It asks for work to be done on a thread because i2c needs a thread
 * context (slow and blocking) and then reschedules the timer to run again.
 */
static enum hrtimer_restart cm36658_light_timer_func(struct hrtimer *timer)
{
	struct cm36658_data *cm36658
		= container_of(timer, struct cm36658_data, light_timer);
	queue_work(cm36658->light_wq, &cm36658->work_light);
	hrtimer_forward_now(&cm36658->light_timer, cm36658->light_poll_delay);
	return HRTIMER_RESTART;
}

static enum hrtimer_restart cm36658_prox_timer_func(struct hrtimer *timer)
{
	struct cm36658_data *cm36658
			= container_of(timer, struct cm36658_data, prox_timer);
	queue_work(cm36658->prox_avg_wq, &cm36658->work_prox_avg);
	hrtimer_forward_now(&cm36658->prox_timer, cm36658->prox_poll_delay);
	return HRTIMER_RESTART;
}

static void cm36658_irq_do_work(struct work_struct *work)
{
	struct cm36658_data *cm36658 = lp_info;

	control_and_report(cm36658, CONTROL_INT_ISR_PS_REPORT);

	enable_irq(cm36658->irq);
}

static int cm36658_id_check(struct cm36658_data *cm36658)
{
	int ret;
	u16 temp;

	ret = cm36658_i2c_read_word(cm36658, ID_REG, &temp);
	if (ret < 0)
		return ret;
	if ((temp & 0xFF) != 0x58) {
		SENSOR_ERR("id_check fail 0x%x\n", temp);
		return -1;
	}
	return ret;
}

static int light_setup(struct cm36658_data *cm36658)
{
	int ret;

	cm36658->light_input_dev = input_allocate_device();
	if (!cm36658->light_input_dev) {
		SENSOR_ERR("could not allocate ls input device\n");
		return -ENOMEM;
	}
	input_set_drvdata(cm36658->light_input_dev, cm36658);
	cm36658->light_input_dev->name = "light_sensor";
	input_set_capability(cm36658->light_input_dev, EV_REL, REL_RED);
	input_set_capability(cm36658->light_input_dev, EV_REL, REL_GREEN);
	input_set_capability(cm36658->light_input_dev, EV_REL, REL_BLUE);
	input_set_capability(cm36658->light_input_dev, EV_REL, REL_IR);
	input_set_capability(cm36658->light_input_dev, EV_REL, REL_GAIN);

	ret = input_register_device(cm36658->light_input_dev);
	if (ret < 0) {
		SENSOR_ERR("can not register ls input device\n");
		goto err_free_light_input_device;
	}

	ret = sensors_create_symlink(&cm36658->light_input_dev->dev.kobj,
					cm36658->light_input_dev->name);
	if (ret < 0) {
		SENSOR_ERR("create_symlink error\n");
		goto err_sensors_create_symlink_light;
	}

	ret = sysfs_create_group(&cm36658->light_input_dev->dev.kobj,
				 &light_attribute_group);
	if (ret) {
		SENSOR_ERR("could not create sysfs group\n");
		goto err_sysfs_create_group_light;
	}

	/* light_timer settings. we poll for light values using a timer. */
	hrtimer_init(&cm36658->light_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	cm36658->light_poll_delay = ns_to_ktime(200 * NSEC_PER_MSEC);
	cm36658->light_timer.function = cm36658_light_timer_func;

	/* the timer just fires off a work queue request. */
	/* we need a thread to read the i2c (can be slow and blocking). */
	cm36658->light_wq = create_singlethread_workqueue("cm36658_light_wq");
	if (!cm36658->light_wq) {
		ret = -ENOMEM;
		SENSOR_ERR("could not create light workqueue\n");
		goto err_create_light_workqueue;
	}

	/* this is the thread function we run on the work queue */
	INIT_WORK(&cm36658->work_light, cm36658_work_func_light);

	/* set sysfs for light sensor */
	ret = sensors_register(&cm36658->light_dev,
		cm36658, light_sensor_attrs, "light_sensor");
	if (ret) {
		SENSOR_ERR("can't register light sensor device(%d)\n", ret);
		goto err_register_light_sensor_failed;
	}
	return ret;

err_register_light_sensor_failed:
	destroy_workqueue(cm36658->light_wq);
err_create_light_workqueue:
	sysfs_remove_group(&cm36658->light_input_dev->dev.kobj,
			   &light_attribute_group);
err_sysfs_create_group_light:
	sensors_remove_symlink(&cm36658->light_input_dev->dev.kobj,
			cm36658->light_input_dev->name);
err_sensors_create_symlink_light:
err_free_light_input_device:
	input_unregister_device(cm36658->light_input_dev);
	return ret;
}

static int proximity_setup(struct cm36658_data *cm36658)
{
	int ret;

	cm36658->prox_input_dev = input_allocate_device();
	if (!cm36658->prox_input_dev) {
		SENSOR_ERR("could not allocate ps input device\n");
		return -ENOMEM;
	}
	input_set_drvdata(cm36658->prox_input_dev, cm36658);
	cm36658->prox_input_dev->name = "proximity_sensor";
	input_set_capability(cm36658->prox_input_dev, EV_ABS, ABS_DISTANCE);
	input_set_abs_params(cm36658->prox_input_dev, ABS_DISTANCE, 0, 1, 0, 0);

	ret = input_register_device(cm36658->prox_input_dev);
	if (ret < 0) {
		SENSOR_ERR("could not register ps input device\n");
		goto err_free_prox_input_device;
	}

	ret = sensors_create_symlink(&cm36658->prox_input_dev->dev.kobj,
					cm36658->prox_input_dev->name);
	if (ret < 0) {
		SENSOR_ERR("create_symlink error\n");
		goto err_sensors_create_symlink_prox;
	}

	ret = sysfs_create_group(&cm36658->prox_input_dev->dev.kobj,
				 &proximity_attribute_group);
	if (ret) {
		SENSOR_ERR("could not create sysfs group\n");
		goto err_sysfs_create_group_proximity;
	}

	/* For factory test mode, we use timer to get average proximity data. */
	/* prox_timer settings. we poll for light values using a timer. */
	hrtimer_init(&cm36658->prox_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	cm36658->prox_poll_delay = ns_to_ktime(2000 * NSEC_PER_MSEC);/*2 sec*/
	cm36658->prox_timer.function = cm36658_prox_timer_func;

	/* the timer just fires off a work queue request. */
	/*  we need a thread to read the i2c (can be slow and blocking). */
	cm36658->prox_wq = create_singlethread_workqueue("cm36658_prox_wq");
	if (!cm36658->prox_wq) {
		ret = -ENOMEM;
		SENSOR_ERR("could not create prox workqueue\n");
		goto err_create_prox_workqueue;
	}
	cm36658->prox_avg_wq =
		create_singlethread_workqueue("cm36658_prox_avg_wq");
	if (!cm36658->prox_avg_wq) {
		ret = -ENOMEM;
		SENSOR_ERR("could not create prox avg workqueue\n");
		goto err_create_prox_avg_workqueue;
	}
	/* this is the thread function we run on the work queue */
	INIT_WORK(&cm36658->work_prox, cm36658_irq_do_work);
	INIT_WORK(&cm36658->work_prox_avg, cm36658_work_func_prox);

	/* set sysfs for proximity sensor */
	ret = sensors_register(&cm36658->prox_dev,
		cm36658, prox_sensor_attrs, "proximity_sensor");
	if (ret) {
		SENSOR_ERR("can't register prox sensor device(%d)\n", ret);
		goto err_register_prox_sensor_failed;
	}

	return ret;

err_register_prox_sensor_failed:
	destroy_workqueue(cm36658->prox_avg_wq);
err_create_prox_avg_workqueue:
	destroy_workqueue(cm36658->prox_wq);
err_create_prox_workqueue:
	sysfs_remove_group(&cm36658->prox_input_dev->dev.kobj,
			   &proximity_attribute_group);
err_sysfs_create_group_proximity:
	sensors_remove_symlink(&cm36658->prox_input_dev->dev.kobj,
			cm36658->prox_input_dev->name);
err_sensors_create_symlink_prox:
	input_unregister_device(cm36658->prox_input_dev);
err_free_prox_input_device:
	input_free_device(cm36658->prox_input_dev);
	return ret;
}

static int cm36658_setup_irq(struct cm36658_data *cm36658)
{
	int ret = 0;

	msleep(5);
	ret = gpio_request(cm36658->irq_gpio, "gpio_proximity_out");
	if (ret < 0) {
		SENSOR_ERR("gpio %d request failed (%d)\n", cm36658->irq_gpio, ret);
		return ret;
	}

	ret = gpio_direction_input(cm36658->irq_gpio);
	if (ret < 0) {
		SENSOR_ERR("fail to set gpio %d as input (%d)\n", cm36658->irq_gpio, ret);
		goto fail_free_irq_gpio;
	}

	/*Default disable P sensor and L sensor*/
	ls_initial_cmd(cm36658);
	psensor_initial_cmd(cm36658);

	cm36658->irq = gpio_to_irq(cm36658->irq_gpio);

	ret = request_any_context_irq(cm36658->irq, cm36658_irq_handler,
			IRQF_TRIGGER_FALLING | IRQF_ONESHOT, "cm36658", cm36658);

	if (ret < 0) {
		SENSOR_ERR("req_irq(%d) fail for gpio %d (%d)\n", cm36658->irq,
				cm36658->irq_gpio, ret);
		goto fail_free_irq_gpio;
	}

	/* start with interrupts disabled */
	disable_irq(cm36658->irq);

	return ret;

fail_free_irq_gpio:
	gpio_free(cm36658->irq_gpio);
	return ret;
}

static int cm36658_parse_dt(struct device *dev,
				struct cm36658_data *cm36658)
{
	struct device_node *np = dev->of_node;
	struct pinctrl *p;
	enum of_gpio_flags flags;
	u32 temp;
	int ret;

	if (!np)
		return -EINVAL;

	ret = of_get_named_gpio_flags(np, "cm36658,irq_pin", 0, NULL);
	if (ret < 0) {
		SENSOR_ERR("Unable to read interrupt pin number\n");
		return ret;
	} else {
		cm36658->irq_gpio = ret;
		SENSOR_INFO("GET INTR PIN\n");
	}

	cm36658->vdd_ldo_pin = of_get_named_gpio_flags(np,
				"cm36658,vdd_ldo_pin", 0, &flags);
	if (cm36658->vdd_ldo_pin < 0) {
		SENSOR_INFO("Cannot set vdd_ldo_pin through DTSI\n");
		cm36658->vdd_ldo_pin = 0;
	} else {
		ret = gpio_request(cm36658->vdd_ldo_pin, "cm36658_vdd_en");
		if (ret < 0)
			SENSOR_ERR("gpio %d request failed %d\n",
				cm36658->vdd_ldo_pin, ret);
		else
			gpio_direction_output(cm36658->vdd_ldo_pin, 0);
	}

	cm36658->led_ldo_pin = of_get_named_gpio_flags(np,
				"cm36658,led_ldo_pin", 0, &flags);
	if (cm36658->led_ldo_pin < 0) {
		SENSOR_INFO("Cannot set vdd_ldo_pin through DTSI\n");
		cm36658->led_ldo_pin = 0;
	} else {
		ret = gpio_request(cm36658->led_ldo_pin, "cm36658_vdd_en");
		if (ret < 0)
			SENSOR_ERR("gpio %d request failed %d\n",
				cm36658->led_ldo_pin, ret);
		else
			gpio_direction_output(cm36658->led_ldo_pin, 0);
	}

	if (of_property_read_u32(np, "cm36658,thresh_high",
		&temp) < 0)
		SENSOR_INFO("Cannot set thresh_high through DTSI\n");
	else
		cm36658_thd_tbl[0][1] = temp;

	if (of_property_read_u32(np, "cm36658,thresh_low",
		&temp) < 0)
		SENSOR_INFO("Cannot set thresh_low through DTSI\n");
	else
		cm36658_thd_tbl[1][0] = temp;

	if (of_property_read_u32(np, "cm36658,thresh_detect_high",
		&temp) < 0)
		SENSOR_INFO("Cannot set thresh_detect_high through DTSI\n");
	else
		cm36658_thd_tbl[1][1] = temp;

	if (of_property_read_u32(np, "cm36658,thresh_detect_low",
		&temp) < 0)
		SENSOR_INFO("Cannot set thresh_detect_low through DTSI\n");
	else
		cm36658_thd_tbl[2][0] = temp;

	SENSOR_INFO("thresh_high:%d, thresh_low:%d\n",
		cm36658_thd_tbl[0][1], cm36658_thd_tbl[1][0]);
	SENSOR_INFO("thresh_detect_high:%d, thresh_detect_low:%d\n",
		cm36658_thd_tbl[1][1], cm36658_thd_tbl[2][0]);

	/* Proximity CONF1 register Setting */
	if (of_property_read_u32(np, "cm36658,ps_conf1_reg", &temp) < 0) {
		SENSOR_INFO("Cannot set ps_conf1_reg through DTSI\n");
		cm36658->ps_conf1_val = CM36658_PS_IT_4T |
				CM36658_PS_PERS_4 | CM36658_PS_START | CM36658_PS_INT_SEL;
	} else {
		cm36658->ps_conf1_val = temp;
	}

	/* Proximity CONF3 register Setting */
	if (of_property_read_u32(np, "cm36658,ps_conf3_reg", &temp) < 0) {
		SENSOR_INFO("Cannot set ps_conf3_reg through DTSI\n");
		cm36658->ps_conf3_val = CM36658_LED_I_110 | CM36658_PS_START2 |
				CM36658_PS_SUNLIGHT_ENABLE;
	} else {
		cm36658->ps_conf3_val = temp;
	}

	cm36658->offset = 0;

	p = pinctrl_get_select_default(dev);
	if (IS_ERR(p)) {
		SENSOR_INFO("failed pinctrl_get\n");
		return -EINVAL;
	}

	SENSOR_INFO("parse_dt done\n");

	return 0;
}

static int cm36658_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int ret = 0;
	struct cm36658_data *cm36658;

	SENSOR_INFO("Probe Start!\n");
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		SENSOR_ERR("i2c functionality check failed!\n");
		return ret;
	}

	cm36658 = kzalloc(sizeof(struct cm36658_data), GFP_KERNEL);
	if (!cm36658)
		return -ENOMEM;

	ret = cm36658_parse_dt(&client->dev, cm36658);
	if (ret) {
		ret = -EBUSY;
		goto err_parse_dt_fail;
	}

	cm36658_vdd_onoff(cm36658, ON);

	cm36658->i2c_client = client;
	i2c_set_clientdata(client, cm36658);
	cm36658->cs_gain = 1;
	cm36658->cur_lv = 0;
	cm36658->irq = client->irq;
	cm36658->ls_cmd = CS_RESERVED_1;
	cm36658->power = NULL;
	cm36658->autocal = 0;

	mutex_init(&cm36658->power_lock);
	mutex_init(&cm36658->read_lock);

	SENSOR_INFO("ls_cmd 0x%x\n", cm36658->ls_cmd);

	if (cm36658->ls_cmd == 0) {
		cm36658->ls_cmd  = CS_RESERVED_1;
	}

	lp_info = cm36658;

	mutex_init(&cm36658->control_mutex);

	/* wake lock init for proximity sensor */
	wake_lock_init(&cm36658->prox_wake_lock, WAKE_LOCK_SUSPEND,
				"prox_wake_lock");

	/* Check if the device is there or not. */
	ret = cm36658_id_check(cm36658);
	if (ret < 0) {
		SENSOR_ERR("cm36658 is not connected or id not matched.(%d)\n", ret);
		goto err_setup_reg;
	}

	ret = cm36658_setup_irq(cm36658);
	if (ret < 0) {
		SENSOR_ERR("cm36658_setup_irq error!\n");
		goto err_cm36658_setup_irq;
	}

	ret = light_setup(cm36658);
	if (ret < 0) {
		SENSOR_ERR("light_setup error!!\n");
		goto err_light_setup;
	}

	ret = proximity_setup(cm36658);
	if (ret < 0) {
		SENSOR_ERR("proximity_setup error!!\n");
		goto err_proximity_setup;
	}

	SENSOR_INFO("Probe success!\n");

	return ret;

err_proximity_setup:
	sensors_unregister(cm36658->prox_dev, prox_sensor_attrs);
	destroy_workqueue(cm36658->light_wq);
	sysfs_remove_group(&cm36658->light_input_dev->dev.kobj,
			   &light_attribute_group);
	sensors_remove_symlink(&cm36658->light_input_dev->dev.kobj,
			cm36658->light_input_dev->name);
	input_unregister_device(cm36658->light_input_dev);
	mutex_destroy(&cm36658->control_mutex);
err_light_setup:
	gpio_free(cm36658->irq_gpio);
err_cm36658_setup_irq:
err_setup_reg:
	wake_lock_destroy(&cm36658->prox_wake_lock);
	mutex_destroy(&cm36658->read_lock);
	mutex_destroy(&cm36658->power_lock);
err_parse_dt_fail:
	kfree(cm36658);
	return ret;
}

static int control_and_report(struct cm36658_data *cm36658, uint8_t mode) {
	uint16_t int_flag = 0;
	uint16_t ps_data = 0;
	int changeThd = 0;
	int direction = 1;
	int err;

	mutex_lock(&cm36658->control_mutex);

	if (mode == CONTROL_PS) {
		changeThd = 1;

		cm36658_get_calibration_result(cm36658, 1);

		mutex_lock(&cm36658->read_lock);
		cm36658_get_ps_adc_value(&ps_data);
		mutex_unlock(&cm36658->read_lock);
		if (ps_data > cm36658_thd_tbl[cm36658->cur_lv][1])
			direction = 0;
	} else if (mode == CONTROL_INT_ISR_PS_REPORT) {
		usleep_range(2900, 3100);
		err = cm36658_i2c_read_word(cm36658, INT_FLAG, &int_flag);
		SENSOR_INFO("irq status: %d\n", int_flag);
		if (err < 0) {
			SENSOR_ERR("INT_FLAG read error, ret=%d\n", err);
			mutex_unlock(&cm36658->control_mutex);
			return 0;
		}

		if (int_flag & INT_FLAG_PS_IF_CLOSE) {
			direction = 0;
			mutex_lock(&cm36658->read_lock);
			cm36658_get_ps_adc_value(&ps_data);
			mutex_unlock(&cm36658->read_lock);
		} else if (int_flag & INT_FLAG_PS_IF_AWAY) {
			direction = 1;
			mutex_lock(&cm36658->read_lock);
			cm36658_get_ps_adc_value(&ps_data);
			mutex_unlock(&cm36658->read_lock);
			if (ps_data == 0 || ((cm36658->cur_lv == 2) &&
					(ps_data < cm36658_thd_tbl[2][0]))) {
				cm36658_get_calibration_result(cm36658, 0);
			}
		} else {
			mutex_lock(&cm36658->read_lock);
			cm36658_get_ps_adc_value(&ps_data);
			mutex_unlock(&cm36658->read_lock);
			SENSOR_ERR("Unknown event, int_flag=0x%x, ps_data = %d\n",
					int_flag, ps_data);
			mutex_unlock(&cm36658->control_mutex);
			return 0;
		}
	}

	SENSOR_INFO("dir = %d (0 : CLOSE, 1 : FAR), ps_data = %d\n",
			direction, ps_data);
	if ((direction && (ps_data < cm36658_thd_tbl[cm36658->cur_lv][0])) ||
			((!direction) &&
			(ps_data > cm36658_thd_tbl[cm36658->cur_lv][1])) ||
			(mode == CONTROL_PS)) {
		input_report_abs(cm36658->prox_input_dev, ABS_DISTANCE,
				direction);
		input_sync(cm36658->prox_input_dev);
	}

	SENSOR_INFO("cur_lv = %d, (high_thd, low_thd) = (%d,%d)\n",
				cm36658->cur_lv,
				cm36658_thd_tbl[cm36658->cur_lv][1],
				cm36658_thd_tbl[cm36658->cur_lv][0]);

	if ((direction == 0) && (ps_data > cm36658_thd_tbl[cm36658->cur_lv][1]) &&
			(cm36658->cur_lv == 0 || cm36658->cur_lv == 1)) {
		cm36658->cur_lv++;
		changeThd = 1;
	} else if ((direction == 1) && (ps_data < cm36658_thd_tbl[cm36658->cur_lv][0]) &&
			(cm36658->cur_lv == 1 || cm36658->cur_lv == 2)) {
		cm36658->cur_lv--;
		changeThd = 1;
	} else if (cm36658->autocal) {
		/* Enable INT */
		cm36658_i2c_read_word(cm36658, PS_CONF1, &cm36658->ps_conf1_val);
		cm36658->ps_conf1_val |= CM36658_PS_INT_ENABLE;
		cm36658_i2c_write_word(cm36658, PS_CONF1, cm36658->ps_conf1_val);
		cm36658->autocal = 0;
	}

	if (changeThd == 1) {
		/* Disable INT */
		cm36658_i2c_read_word(cm36658, PS_CONF1,
				&cm36658->ps_conf1_val);
		cm36658->ps_conf1_val |= CM36658_PS_SD;
		cm36658->ps_conf1_val &= CM36658_PS_INT_MASK;
		cm36658_i2c_write_word(cm36658, PS_CONF1,
				cm36658->ps_conf1_val);

		/* Set THD */
		cm36658_i2c_write_word(cm36658, PS_THDL,
				cm36658_thd_tbl[cm36658->cur_lv][0]);
		cm36658_i2c_write_word(cm36658, PS_THDH,
				cm36658_thd_tbl[cm36658->cur_lv][1]);

		SENSOR_INFO("next_lv = %d, (high_thd, low_thd) = (%d,%d)\n",
				cm36658->cur_lv,
				cm36658_thd_tbl[cm36658->cur_lv][1],
				cm36658_thd_tbl[cm36658->cur_lv][0]);

		/* Enable INT */
		cm36658_i2c_read_word(cm36658, PS_CONF1,
				&cm36658->ps_conf1_val);
		cm36658->ps_conf1_val &= CM36658_PS_SD_MASK;
		cm36658->ps_conf1_val |= CM36658_PS_INT_ENABLE;
		cm36658_i2c_write_word(cm36658, PS_CONF1,
				cm36658->ps_conf1_val);
	}
	mutex_unlock(&cm36658->control_mutex);
	return 0;
}

static int cm36658_i2c_remove(struct i2c_client *client)
{
	struct cm36658_data *cm36658 = i2c_get_clientdata(client);

	/* device off */
	if (cm36658->als_enable == ON)
		cm36658_light_disable(cm36658);
	if (cm36658->ps_enable == ON)
		cm36658_proximity_disable(cm36658);

	/* free irq */
	free_irq(cm36658->irq, cm36658);
	gpio_free(cm36658->irq_gpio);

	/* destroy workqueue */
	destroy_workqueue(cm36658->light_wq);
	destroy_workqueue(cm36658->prox_wq);
	destroy_workqueue(cm36658->prox_avg_wq);

	/* sysfs destroy */
	sensors_unregister(cm36658->light_dev, light_sensor_attrs);
	sensors_unregister(cm36658->prox_dev, prox_sensor_attrs);
	sensors_remove_symlink(&cm36658->light_input_dev->dev.kobj,
			cm36658->light_input_dev->name);
	sensors_remove_symlink(&cm36658->prox_input_dev->dev.kobj,
			cm36658->prox_input_dev->name);

	/* input device destroy */
	sysfs_remove_group(&cm36658->light_input_dev->dev.kobj,
				&light_attribute_group);
	input_unregister_device(cm36658->light_input_dev);
	sysfs_remove_group(&cm36658->prox_input_dev->dev.kobj,
				&proximity_attribute_group);
	input_unregister_device(cm36658->prox_input_dev);
	/* lock destroy */
	mutex_destroy(&cm36658->read_lock);
	mutex_destroy(&cm36658->power_lock);
	wake_lock_destroy(&cm36658->prox_wake_lock);

	kfree(cm36658);

	return 0;
}

static void cm36658_i2c_shutdown(struct i2c_client *client)
{
	struct cm36658_data *cm36658 = i2c_get_clientdata(client);

	if (cm36658->als_enable == ON)
		cm36658_light_disable(cm36658);
	if (cm36658->ps_enable == ON)
		cm36658_proximity_disable(cm36658);

	SENSOR_INFO("is called.\n");
}

static int cm36658_suspend(struct device *dev)
{
	struct cm36658_data *cm36658 = dev_get_drvdata(dev);

	cm36658->als_enabled_before_suspend = cm36658->als_enable;

	if (cm36658->als_enable == ON)
		cm36658_light_disable(cm36658);

	return 0;
}

static int cm36658_resume(struct device *dev)
{
	struct cm36658_data *cm36658;
	cm36658 = dev_get_drvdata(dev);

	if (cm36658->als_enabled_before_suspend)
		cm36658_light_enable(cm36658);

	return 0;
}

static UNIVERSAL_DEV_PM_OPS(cm36658_pm, cm36658_suspend, cm36658_resume, NULL);

static const struct i2c_device_id cm36658_device_id[] = {
	{"cm36658", 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, cm36658_device_id);

static struct of_device_id cm36658_match_table[] = {
	{ .compatible = "cm36658",},
	{ },
};

static struct i2c_driver cm36658_driver = {
	.driver = {
		.name = "cm36658",
		.owner = THIS_MODULE,
		.of_match_table = cm36658_match_table,
		.pm = &cm36658_pm,
	},
	.id_table = cm36658_device_id,
	.probe = cm36658_probe,
	.shutdown = cm36658_i2c_shutdown,
	.remove = cm36658_i2c_remove,

};

static int __init cm36658_init(void)
{
	return i2c_add_driver(&cm36658_driver);
}

static void __exit cm36658_exit(void)
{
	i2c_del_driver(&cm36658_driver);
}

module_init(cm36658_init);
module_exit(cm36658_exit);

MODULE_AUTHOR("Samsung Electronics");
MODULE_DESCRIPTION("CM36658 Optical Sensor Driver");
MODULE_LICENSE("GPL v2");
