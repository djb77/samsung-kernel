/*
 * Copyright (c) 2014-2015 Yamaha Corporation
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.
 */

#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/workqueue.h>

#include <linux/sensor/sensors_core.h>
#include "yas.h"

static struct i2c_client *this_client;

enum {
	OFF = 0,
	ON = 1
};

struct yas_state {
	struct mutex lock;
	struct mutex enable_lock;
	struct yas_mag_driver mag;
	struct input_dev *input;
	struct i2c_client *client;
	struct delayed_work work;
	struct device *yas_device;
	int poll_delay;
	int enable;
#if defined(CONFIG_SENSORS_YAS_RESET_DEFENCE_CODE)
	int reset_state;
#endif
	int32_t compass_data[3];

	int position;
	int16_t m[9];
};

static int yas_device_open(int32_t type)
{
	return 0;
}

static int yas_device_close(int32_t type)
{
	return 0;
}

static int yas_device_write(int32_t type, uint8_t addr, const uint8_t *buf,
		int len)
{
	uint8_t tmp[2];
	if (sizeof(tmp) - 1 < len)
		return -1;
	tmp[0] = addr;
	memcpy(&tmp[1], buf, len);
	if (i2c_master_send(this_client, tmp, len + 1) < 0)
		return -1;
	return 0;
}

static int yas_device_read(int32_t type, uint8_t addr, uint8_t *buf, int len)
{
	struct i2c_msg msg[2];
	int err;
	msg[0].addr = this_client->addr;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = &addr;
	msg[1].addr = this_client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = len;
	msg[1].buf = buf;

	err = i2c_transfer(this_client->adapter, msg, 2);
	if (err != 2) {
		dev_err(&this_client->dev,
				"i2c_transfer() read error: "
				"slave_addr=%02x, reg_addr=%02x, err=%d\n",
				this_client->addr, addr, err);
		return err;
	}
	return 0;
}

static void yas_usleep(int us)
{
	usleep_range(us, us + 1000);
}

static uint32_t yas_current_time(void)
{
	return jiffies_to_msecs(jiffies);
}

/* Sysfs interface */
static ssize_t yas_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct yas_state *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", data->enable);
}

static ssize_t yas_enable_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct yas_state *data = dev_get_drvdata(dev);
	int ret, pre_enable;
	u8 enable;

	SENSOR_INFO("pre_enable=%d\n", data->enable);
	
	ret = kstrtou8(buf, 2, &enable);
	if (ret) {
		SENSOR_ERR("Invalid Argument\n");
		return ret;
	}

#if defined(CONFIG_SENSORS_YAS_RESET_DEFENCE_CODE)
	if (data->reset_state) {
		data->enable = enable;
		return size;
	}
#endif
	mutex_lock(&data->enable_lock);

	SENSOR_INFO("new_value = %u\n", enable);
	pre_enable = data->enable;

	if (enable) {
		if (pre_enable == OFF) {
			data->mag.set_enable(1);
			data->enable = ON;
			schedule_delayed_work(&data->work, 0);
		}
	} else {
		if (pre_enable == ON) {
			data->mag.set_enable(0);
			cancel_delayed_work_sync(&data->work);
			data->enable = OFF;
		}
	}
	mutex_unlock(&data->enable_lock);

	return size;
} 

static ssize_t yas_delay_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct yas_state *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", data->poll_delay * 1000000);
}

static ssize_t yas_delay_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct yas_state *data = dev_get_drvdata(dev);
	int ret, delay;
	ret = kstrtoint(buf, 10, &delay);
	if (ret)
		return ret;
	SENSOR_INFO("delay = %d\n", delay);
	if (delay <= 0)
		return -EINVAL;

	else if (delay >= 200000000)
		delay = 200000000;
	
#if defined(CONFIG_SENSORS_YAS_RESET_DEFENCE_CODE)
	if (data->reset_state) {
		data->poll_delay = delay / NSEC_PER_MSEC;
		data->mag.set_delay(data->poll_delay);
		return size;
	}
#endif

	SENSOR_INFO("+ mutex_lock\n");
	mutex_lock(&data->lock);

	data->poll_delay = delay / NSEC_PER_MSEC;
	data->mag.set_delay(data->poll_delay);

	mutex_unlock(&data->lock);
	SENSOR_INFO("- mutex_unlock\n");

	return size;
}

static DEVICE_ATTR(poll_delay, S_IRUGO | S_IWUSR | S_IWGRP,
		yas_delay_show, yas_delay_store);
static DEVICE_ATTR(enable, S_IRUGO | S_IWUSR | S_IWGRP,
		yas_enable_show, yas_enable_store);

static struct attribute *yas_attributes[] = {
	&dev_attr_poll_delay.attr,
	&dev_attr_enable.attr,
	NULL
};

static const struct attribute_group yas_attribute_group = {
	.attrs = yas_attributes,
};

static ssize_t yas_vendor_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", VENDOR_NAME);
}

static ssize_t yas_name_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", DEV_NAME);
}

static ssize_t yas_self_test_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct yas_state *data = i2c_get_clientdata(this_client);
	struct yas539_self_test_result r;
	s8 err[7] = { 0, };

	int ret;

	mutex_lock(&data->lock);
	ret = data->mag.ext(YAS539_SELF_TEST, &r);

	mutex_unlock(&data->lock);

	if (unlikely(r.id != 0x8))
		err[0] = -1;
	if (unlikely(r.sxy1y2[0] > 17024 || r.sxy1y2[0] < 16544))
		err[5] = -1;
	if (unlikely(r.sxy1y2[1] > 17184 || r.sxy1y2[1] < 16517))
		err[5] = -2;
	if (unlikely(r.sxy1y2[2] > 16251 || r.sxy1y2[2] < 15584))
		err[5] = -4;
	if (unlikely(r.xyz[0] < -1000 || r.xyz[0] > 1000))
		err[6] = -1;
	if (unlikely(r.xyz[1] < -1000 || r.xyz[1] > 1000))
		err[6] = -1;
	if (unlikely(r.xyz[2] < -1000 || r.xyz[2] > 1000))
		err[6] = -1;

	pr_info("[SENSOR] %s\n"
		"[SENSOR] Test1 - err = %d, id = %d\n"
		"[SENSOR] Test3 - err = %d\n"
		"[SENSOR] Test4 - err = %d, offset = %d,%d,%d\n"
		"[SENSOR] Test5 - err = %d, direction = %d\n"
		"[SENSOR] Test6 - err = %d, sensitivity = %d,%d,%d\n"
		"[SENSOR] Test7 - err = %d, offset = %d,%d,%d\n"
		"[SENSOR] Test2 - err = %d\n",
		__func__,
		err[0], r.id,
		err[2],
		err[3], err[4], err[4], err[4],
		err[4], err[4],
		err[5], r.sxy1y2[0], r.sxy1y2[1], r.sxy1y2[2],
		err[6], r.xyz[0], r.xyz[1], r.xyz[2],
		err[1]);

	return snprintf(buf, PAGE_SIZE,
			"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
			err[0], r.id, err[2], err[3], err[4],
			err[4], err[4], err[4], err[4], err[5],
			r.sxy1y2[0], r.sxy1y2[1], r.sxy1y2[2], err[6],
			r.xyz[0], r.xyz[1], r.xyz[2], err[1]);
}

static ssize_t yas_self_test_noise_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct yas_state *data = i2c_get_clientdata(this_client);
	int32_t xyz_raw[3];
	int ret;

	mutex_lock(&data->lock);
	ret = data->mag.ext(YAS539_SELF_TEST_NOISE, xyz_raw);
	mutex_unlock(&data->lock);

	if (ret < 0)
		return -EFAULT;

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d\n",
		xyz_raw[0], xyz_raw[1], xyz_raw[2]);
}

#if defined(CONFIG_SENSORS_YAS_RESET_DEFENCE_CODE)
static ssize_t yas_power_reset_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct yas_state *data = dev_get_drvdata(dev);

	data->reset_state = ON;

	mutex_lock(&data->enable_lock);

	SENSOR_INFO("Start!\n");

	if (data->enable)
		cancel_delayed_work_sync(&data->work);

	mutex_unlock(&data->enable_lock);

	SENSOR_INFO("Done!\n");

	return snprintf(buf, PAGE_SIZE, "1");
}

static ssize_t yas_sw_reset_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct yas_state *data = dev_get_drvdata(dev);
	int position, i;
	int16_t m[9];

	mutex_lock(&data->enable_lock);

	SENSOR_INFO("Start!\n");
	data->reset_state = OFF;

	data->mag.term();
	data->mag.init();
	position = data->position;
	data->mag.set_position(position);

	for (i = 0; i < 9; i++)
		m[i] = data->m[i];

	data->mag.ext(YAS539_SET_STATIC_MATRIX, m);

	if (data->enable) {
		SENSOR_INFO("Magnetic enable\n");
		data->mag.set_enable(1);
		data->mag.set_delay(data->poll_delay);
		schedule_delayed_work(&data->work,
			msecs_to_jiffies(data->poll_delay));
	} else
		data->mag.set_enable(0);

	mutex_unlock(&data->enable_lock);

	SENSOR_INFO("Done!\n");

	return snprintf(buf, PAGE_SIZE, "1");
}
static DEVICE_ATTR(power_reset, S_IRUSR | S_IRGRP, yas_power_reset_show, NULL);
static DEVICE_ATTR(sw_reset, S_IRUSR | S_IRGRP, yas_sw_reset_show, NULL);
#endif
static DEVICE_ATTR(vendor, S_IRUGO, yas_vendor_show, NULL);
static DEVICE_ATTR(name, S_IRUGO, yas_name_show, NULL);
static DEVICE_ATTR(selftest, S_IRUSR, yas_self_test_show, NULL);
static DEVICE_ATTR(raw_data, S_IRUSR, yas_self_test_noise_show, NULL);

static struct device_attribute *mag_sensor_attrs[] = {
	&dev_attr_vendor,
	&dev_attr_name,
	&dev_attr_selftest,
	&dev_attr_raw_data,
#if defined(CONFIG_SENSORS_YAS_RESET_DEFENCE_CODE)
	&dev_attr_power_reset,
	&dev_attr_sw_reset,
#endif
	NULL
};

static void yas_work_func(struct work_struct *work)
{
	struct yas_data mag[1];
	struct yas_state *data
		= container_of((struct delayed_work *)work,
			struct yas_state, work);
	struct timespec ts = ktime_to_timespec(ktime_get_boottime());
	u64 timestamp = ts.tv_sec * 1000000000ULL + ts.tv_nsec;
	int ret, i, time_hi, time_lo;

	mutex_lock(&data->lock);

	ret = data->mag.measure(mag, 1);
	if (ret <= 0) {
		schedule_delayed_work(&data->work,
			msecs_to_jiffies(data->poll_delay));
		mutex_unlock(&data->lock);
		return;
	}

	for (i = 0; i < 3; i++)
		data->compass_data[i] = mag[0].xyz.v[i];

	mutex_unlock(&data->lock);

	time_hi = (int)((timestamp & TIME_HI_MASK) >> TIME_HI_SHIFT);
	time_lo = (int)(timestamp & TIME_LO_MASK);
	input_report_rel(data->input, REL_X, data->compass_data[0]);
	input_report_rel(data->input, REL_Y, data->compass_data[1]);
	input_report_rel(data->input, REL_Z, data->compass_data[2]);
	input_report_rel(data->input, REL_RX, time_hi);
	input_report_rel(data->input, REL_RY, time_lo);
	input_sync(data->input);

	schedule_delayed_work(&data->work, msecs_to_jiffies(data->poll_delay));
}

static int yas_input_init(struct yas_state *data)
{
	struct input_dev *dev;
	int ret = 0;

	dev = input_allocate_device();
	if (!dev)
		return -ENOMEM;

	dev->name = MODULE_NAME_MAG;
	dev->id.bustype = BUS_I2C;

	input_set_capability(dev, EV_REL, REL_X);
	input_set_capability(dev, EV_REL, REL_Y);
	input_set_capability(dev, EV_REL, REL_Z);
	input_set_capability(dev, EV_REL, REL_RX);
	input_set_capability(dev, EV_REL, REL_RY);
	input_set_drvdata(dev, data);

	ret = input_register_device(dev);
	if (ret < 0)
		goto err_register_input_dev;

	ret = sensors_create_symlink(&dev->dev.kobj, dev->name);
	if (ret < 0)
		goto err_create_sensor_symlink;

	/* sysfs node creation */
	ret = sysfs_create_group(&dev->dev.kobj, &yas_attribute_group);
	if (ret < 0)
		goto err_create_sysfs_group;

	data->input = dev;

	return 0;

err_create_sysfs_group:
	sensors_remove_symlink(&dev->dev.kobj, dev->name);
err_create_sensor_symlink:
	input_unregister_device(dev);
err_register_input_dev:
	input_free_device(dev);
	return ret;
}

static int yas_parse_dt(struct device *dev, struct yas_state *data)
{
	struct device_node *np = dev->of_node;
	u32 softiron[9] = {10000, 0, 0, 0, 10000, 0, 0, 0, 10000};
	u32 sign[9] = {2, 1, 1, 1, 2, 1, 1, 1, 2};
	int i = 0;

	if (!of_property_read_u32(np, "yas,orientation", &data->position))
		SENSOR_INFO("yas,orientation = %d\n", data->position);
	else
		return -EINVAL;

#if defined(CONFIG_SENSORS_YAS539_DEFAULT_MATRIX)
	SENSOR_INFO("yas softiron : default\n");
#else
	if (of_property_read_u32_array(np, "yas,softiron", softiron, 9) < 0)
		SENSOR_ERR("get softiron(%d) error\n", softiron[0]);

	if (of_property_read_u32_array(np, "yas,softiron_sign", sign, 9) < 0)
		SENSOR_ERR("get softiron(%d) error\n", sign[0]);
#endif

	for (i = 0; i < 9; i++) {
		data->m[i] = (softiron[i]) * (sign[i]-1);
		SENSOR_INFO("softiron = %5d, sign = %2d (%6d)\n",
			softiron[i], sign[i], data->m[i]);
	}

	return 0;
}

static int yas_probe(struct i2c_client *i2c, const struct i2c_device_id *id)
{
	struct yas_state *data;
	int ret, i;
	int position;
	int16_t m[9];

	SENSOR_INFO("PROBE START\n");

	this_client = i2c;
	data = kzalloc(sizeof(struct yas_state), GFP_KERNEL);
	if (!data) {
		SENSOR_ERR("kzalloc error\n");
		ret = -ENOMEM;
		goto err_ret;
	}
	i2c_set_clientdata(i2c, data);

	data->client = i2c;
	data->poll_delay = YAS_DEFAULT_SENSOR_DELAY;
	data->mag.callback.device_open = yas_device_open;
	data->mag.callback.device_close = yas_device_close;
	data->mag.callback.device_write = yas_device_write;
	data->mag.callback.device_read = yas_device_read;
	data->mag.callback.usleep = yas_usleep;
	data->mag.callback.current_time = yas_current_time;

	INIT_DELAYED_WORK(&data->work, yas_work_func);
	data->enable = OFF;
#if defined(CONFIG_SENSORS_YAS_RESET_DEFENCE_CODE)
	data->reset_state = OFF;
#endif
	mutex_init(&data->lock);
	mutex_init(&data->enable_lock);

	for (i = 0; i < 3; i++)
		data->compass_data[i] = 0;

	/* input device init */
	ret = yas_input_init(data);
	if (ret < 0) {
		SENSOR_ERR("input init fail (%d)\n", ret);
		goto err_input_init;
	}

	ret = yas_mag_driver_init(&data->mag);
	if (ret < 0) {
		ret = -EFAULT;
		goto err_unregister;
	}
	ret = data->mag.init();
	if (ret < 0) {
		ret = -EFAULT;
		goto err_unregister;
	}
	ret = yas_parse_dt(&i2c->dev, data);
	if (!ret) {
		position = data->position;
		ret = data->mag.set_position(position);
		SENSOR_INFO("set_position (%d)\n", position);

	   	for (i = 0; i < 9; i++)
			m[i] = data->m[i];

	   	ret = data->mag.ext(YAS539_SET_STATIC_MATRIX, m);
	} else {
		SENSOR_ERR("parse_dt error\n");
		ret = -ENODEV;
		goto err_parse_dt;
	}

	ret = sensors_register(&data->yas_device, data, mag_sensor_attrs,
		MODULE_NAME_MAG);
	if (ret < 0) {
		SENSOR_ERR("cound not register mag sensor device(%d).\n", ret);
		goto err_mag_sensor_register_failed;
	}

	SENSOR_INFO("PROBE END\n");
	return 0;

err_mag_sensor_register_failed:
err_parse_dt:
err_unregister:
	sensors_remove_symlink(&data->input->dev.kobj, data->input->name);
	sysfs_remove_group(&data->input->dev.kobj, &yas_attribute_group);
	input_unregister_device(data->input);
err_input_init:
	cancel_delayed_work_sync(&data->work);
	i2c_set_clientdata(i2c, NULL);
	mutex_destroy(&data->enable_lock);
	mutex_destroy(&data->lock);
	kfree(data);
err_ret:
	this_client = NULL;
	SENSOR_ERR("PROBE FAILED\n");
	return ret;
}

static int yas_remove(struct i2c_client *i2c)
{
	struct yas_state *data = (struct yas_state *)i2c_get_clientdata(i2c);

	if (data->enable) {
		data->mag.term();
		/* destroy workqueue */
		cancel_delayed_work_sync(&data->work);
		/* sysfs destroy */
		sensors_unregister(data->yas_device, mag_sensor_attrs);
		sysfs_remove_group(&data->input->dev.kobj,
				&yas_attribute_group);
		/* lock destroy */
		mutex_destroy(&data->enable_lock);
		mutex_destroy(&data->lock);
	
		sensors_remove_symlink(&data->input->dev.kobj,
			data->input->name);
		input_unregister_device(data->input);

		kfree(data);
		this_client = NULL;
	}
	return 0;
}

static int yas_suspend(struct device *dev)
{
	struct yas_state *data = dev_get_drvdata(dev);
	if (data->enable) {
		cancel_delayed_work_sync(&data->work);
		data->mag.set_enable(0);
	}
	return 0;
}

static int yas_resume(struct device *dev)
{
	struct yas_state *data = dev_get_drvdata(dev);
	if (data->enable) {
		data->mag.set_enable(1);
		schedule_delayed_work(&data->work, 0);
	}
	return 0;
}

static struct of_device_id yas_match_table[] = {
	{ .compatible = "yas_magnetometer",},
	{},
};

static const struct i2c_device_id yas_id[] = {
	{YAS_MAG_NAME, 0},
	{ }
};
MODULE_DEVICE_TABLE(i2c, yas_id);

static const struct dev_pm_ops yas_pm_ops = {
	.suspend = yas_suspend,
	.resume = yas_resume
};

static struct i2c_driver yas_driver = {
	.driver = {
		.name	= YAS_MAG_NAME,
		.owner	= THIS_MODULE,
		.pm	= &yas_pm_ops,
		.of_match_table = yas_match_table,
	},
	.probe		= yas_probe,
	.remove		= yas_remove,
	.id_table	= yas_id,
};
module_i2c_driver(yas_driver);

MODULE_DESCRIPTION("Yamaha Magnetometer I2C driver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0.0.1200");
