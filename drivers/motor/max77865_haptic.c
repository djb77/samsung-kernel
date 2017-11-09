/*
 * haptic motor driver for max77865_haptic.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define pr_fmt(fmt) "[VIB] " fmt
#define DEBUG	1

#include <linux/module.h>
#include <linux/kernel.h>
#include "../staging/android/timed_output.h"
#include <linux/hrtimer.h>
#include <linux/pwm.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/i2c.h>
#include <linux/regulator/consumer.h>
#include <linux/mfd/max77865.h>
#include <linux/mfd/max77865-private.h>
#include <linux/sec_sysfs.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/kthread.h>
#if defined(CONFIG_SSP_MOTOR_CALLBACK)
#include <linux/ssp_motorcallback.h>
#endif

#define MAX_INTENSITY                   10000
#define PACKET_MAX_SIZE                 1000

#define MOTOR_LRA                       (1<<7)
#define MOTOR_EN                        (1<<6)
#define EXT_PWM                         (0<<5)
#define DIVIDER_128                     (1<<1)
#define MRDBTMER_MASK                   (0x7)
#define MREN                            (1 << 3)
#define BIASEN                          (1 << 7)

#define VIB_BUFSIZE                     30

#define BOOST_ON                        1
#define BOOST_OFF                       0

#define HOMEKEY_PRESS_FREQ              5
#define HOMEKEY_RELEASE_FREQ            6
#define HOMEKEY_DURATION                7

static struct max77865_haptic_drvdata *g_drvdata;

struct vib_packet {
	int time;
	int intensity;
	int freq;
	int overdrive;
};

struct max77865_haptic_drvdata {
	struct max77865_dev *max77865;
	struct i2c_client *i2c;
	struct device *dev;
	struct max77865_haptic_pdata *pdata;
	struct pwm_device *pwm;
	struct regulator *regulator;
	struct timed_output_dev tout_dev;
	struct hrtimer timer;
	struct kthread_worker kworker;
	struct kthread_work kwork;
	struct mutex mutex;
	u32 intensity;
	u32 timeout;
	int duty;
	int force_touch_intensity;

	bool f_packet_en;
	int packet_size;
	int packet_cnt;
	bool packet_running;
	struct vib_packet test_pac[PACKET_MAX_SIZE];
};

static int max77865_haptic_set_frequency(struct max77865_haptic_drvdata *drvdata, int num)
{
	struct max77865_haptic_pdata *pdata = drvdata->pdata;

	if (num >= 0 && num < pdata->multi_frequency) {
		pdata->period = pdata->multi_freq_period[num];
		pdata->duty = pdata->multi_freq_duty[num];
		pdata->freq_num = num;
	}
	else if (num >= 1200 && num <= 3500) {
		pdata->period = 78125000/num;
		if(pdata->overdrive_state)
			pdata->duty = pdata->period * 95 / 100;
		else
			pdata->duty = pdata->period * 79 / 100;
		drvdata->intensity = 10000;
		drvdata->duty = pdata->duty;
		pdata->freq_num = num;
	}
	else {
		pr_err("%s out of range %d\n", __func__, num);
		return -EINVAL;
	}

	return 0;
}

static int max77865_haptic_set_intensity(struct max77865_haptic_drvdata *drvdata, int intensity)
{
	int duty = drvdata->pdata->period >> 1;

	if (intensity < -(MAX_INTENSITY) || MAX_INTENSITY < intensity) {
		pr_err("%s out of range %d\n", __func__, intensity);
		return -EINVAL;
	}

	if (MAX_INTENSITY == intensity)
		duty = drvdata->pdata->duty;
	else if (intensity == -(MAX_INTENSITY))
		duty = drvdata->pdata->period - drvdata->pdata->duty;
	else if (0 != intensity) {
		duty += (drvdata->pdata->duty - duty) * intensity / MAX_INTENSITY;
	}

	pwm_config(drvdata->pwm, duty, drvdata->pdata->period);

	if (intensity == 0)
		pwm_disable(drvdata->pwm);
	else
		pwm_enable(drvdata->pwm);

	drvdata->intensity = intensity;
	drvdata->duty = duty;

	return 0;
}

static int max77865_haptic_set_overdrive_state(struct max77865_haptic_drvdata *drvdata, int overdrive)
{
	if(overdrive)
		drvdata->pdata->overdrive_state = true;
	else
		drvdata->pdata->overdrive_state = false;
	return 0;
}

/*
static int motor_vdd_en(struct max77865_haptic_drvdata *drvdata, bool en)
{
	int ret = 0;

	if (drvdata->pdata->gpio)
		ret = gpio_direction_output(drvdata->pdata->gpio, en);
	else {
		if (en)
			ret = regulator_enable(drvdata->regulator);
		else
			ret = regulator_disable(drvdata->regulator);

		if (ret < 0)
			pr_err("failed to %sable regulator %d\n",
				en ? "en" : "dis", ret);
	}

	return ret;
}*/

int max77865_motor_boost_control(struct max77865_haptic_drvdata *drvdata, int control)
{
	if (control == BOOST_ON) {
		max77865_update_reg(drvdata->i2c, MAX77865_PMIC_REG_BOOSTCONTROL1, 0x00, MAX77865_PMIC_REG_BSTOUT_MASK);
		max77865_update_reg(drvdata->i2c, MAX77865_PMIC_REG_BOOSTCONTROL2, 0x08, MAX77865_PMIC_REG_FORCE_EN_MASK);
		return 0;
	}
	else if (control == BOOST_OFF) {
		max77865_update_reg(drvdata->i2c, MAX77865_PMIC_REG_BOOSTCONTROL2, 0x00, MAX77865_PMIC_REG_FORCE_EN_MASK);
		return 0;
	}
	return -1;
}

static void max77865_haptic_init_reg(struct max77865_haptic_drvdata *drvdata)
{
	int ret;

//	motor_vdd_en(drvdata, true);

	usleep_range(6000, 6000);

	ret = max77865_update_reg(drvdata->i2c,
			MAX77865_PMIC_REG_MAINCTRL1, 0xff, BIASEN);
	if (ret)
		pr_err("i2c REG_BIASEN update error %d\n", ret);

	ret = max77865_update_reg(drvdata->i2c,
		MAX77865_PMIC_REG_MCONFIG, 0x0, MOTOR_EN);
	if (ret)
		pr_err("i2c MOTOR_EN update error %d\n", ret);

	ret = max77865_update_reg(drvdata->i2c,
		MAX77865_PMIC_REG_MCONFIG, 0xff, MOTOR_LRA);
	if (ret)
		pr_err("i2c MOTOR_LPA update error %d\n", ret);

	ret = max77865_update_reg(drvdata->i2c,
		MAX77865_PMIC_REG_MCONFIG, 0xff, DIVIDER_128);
	if (ret)
		pr_err("i2c DIVIDER_128 update error %d\n", ret);
}

static void max77865_haptic_i2c(struct max77865_haptic_drvdata *drvdata,
	bool en)
{

	if (en)
		max77865_update_reg(drvdata->i2c,
			MAX77865_PMIC_REG_MCONFIG, 0xff, MOTOR_EN);
	else
		max77865_update_reg(drvdata->i2c,
			MAX77865_PMIC_REG_MCONFIG, 0x0, MOTOR_EN);
}

static void max77865_haptic_engine_set_packet(struct max77865_haptic_drvdata *drvdata,
		struct vib_packet haptic_packet)
{
	int frequency = haptic_packet.freq;
	int intensity = haptic_packet.intensity;
	int overdrive = haptic_packet.overdrive;

	if (!drvdata->f_packet_en) {
		pr_err("haptic packet is empty\n");
		return;
	}

	max77865_haptic_set_overdrive_state(drvdata, overdrive);
	max77865_haptic_set_frequency(drvdata, frequency);
	max77865_haptic_set_intensity(drvdata, intensity);

	if (intensity == 0) {
		if (drvdata->packet_running) {
			pr_info("[haptic engine] motor stop\n");
			max77865_haptic_i2c(drvdata, false);
		}
		drvdata->packet_running = false;
	} else {
		if (!drvdata->packet_running) {
			pr_info("[haptic engine] motor run\n");
			max77865_haptic_i2c(drvdata, true);
		}
		drvdata->packet_running = true;
	}

	pr_info("%s [haptic_engine] freq:%d, intensity:%d, time:%d overdrive: %d\n", __func__, frequency, intensity, drvdata->timeout, drvdata->pdata->overdrive_state);
}

static ssize_t store_duty(struct device *dev,
	struct device_attribute *devattr, const char *buf, size_t count)
{
	struct max77865_haptic_drvdata *drvdata
		= dev_get_drvdata(dev);

	int ret;
	u16 duty;

	ret = kstrtou16(buf, 0, &duty);
	if (ret != 0) {
		dev_err(dev, "fail to get duty.\n");
		return count;
	}
	drvdata->pdata->duty = duty;

	return count;
}

static ssize_t store_period(struct device *dev,
	struct device_attribute *devattr, const char *buf, size_t count)
{
	struct max77865_haptic_drvdata *drvdata
		= dev_get_drvdata(dev);

	int ret;
	u16 period;

	ret = kstrtou16(buf, 0, &period);
	if (ret != 0) {
		dev_err(dev, "fail to get period.\n");
		return count;
	}
	drvdata->pdata->period = period;

	return count;
}

static ssize_t show_duty_period(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct max77865_haptic_drvdata *drvdata = dev_get_drvdata(dev);

	return snprintf(buf, VIB_BUFSIZE, "duty: %u, period: %u\n",
			drvdata->pdata->duty,
			drvdata->pdata->period);
}

/* below nodes is SAMSUNG specific nodes */
static DEVICE_ATTR(set_duty, 0220, NULL, store_duty);
static DEVICE_ATTR(set_period, 0220, NULL, store_period);
static DEVICE_ATTR(show_duty_period, 0440, show_duty_period, NULL);

static struct attribute *sec_motor_attributes[] = {
	&dev_attr_set_duty.attr,
	&dev_attr_set_period.attr,
	&dev_attr_show_duty_period.attr,
	NULL,
};

static struct attribute_group sec_motor_attr_group = {
	.attrs = sec_motor_attributes,
};

static ssize_t intensity_store(struct device *dev,
	struct device_attribute *devattr, const char *buf, size_t count)
{
	struct timed_output_dev *tdev = dev_get_drvdata(dev);
	struct max77865_haptic_drvdata *drvdata
		= container_of(tdev, struct max77865_haptic_drvdata, tout_dev);
	int intensity = 0, ret = 0;

	ret = kstrtoint(buf, 0, &intensity);
	if (ret) {
		pr_err("fail to get intensity\n");
		return -EINVAL;
	}

	pr_info("%s %d\n", __func__, intensity);

	ret = max77865_haptic_set_intensity(drvdata, intensity);
	if (ret)
		return ret;

	return count;
}

static ssize_t intensity_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct timed_output_dev *tdev = dev_get_drvdata(dev);
	struct max77865_haptic_drvdata *drvdata
		= container_of(tdev, struct max77865_haptic_drvdata, tout_dev);

	return snprintf(buf, VIB_BUFSIZE, "intensity: %u\n", drvdata->intensity);
}

static DEVICE_ATTR(intensity, 0660, intensity_show, intensity_store);

static ssize_t force_touch_intensity_store(struct device *dev,
		struct device_attribute *devattr, const char *buf, size_t count)
{
	struct timed_output_dev *tdev = dev_get_drvdata(dev);
	struct max77865_haptic_drvdata *drvdata
		= container_of(tdev, struct max77865_haptic_drvdata, tout_dev);
	int intensity = 0, ret = 0;

	ret = kstrtoint(buf, 0, &intensity);
	if (ret) {
		pr_err("fail to get intensity\n");
		return -EINVAL;
	}

	pr_info("%s %d\n", __func__, intensity);

	drvdata->force_touch_intensity  = intensity;
	ret = max77865_haptic_set_intensity(drvdata, intensity);
	if (ret)
		return ret;

	return count;
}
static ssize_t force_touch_intensity_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct timed_output_dev *tdev = dev_get_drvdata(dev);
	struct max77865_haptic_drvdata *drvdata
		= container_of(tdev, struct max77865_haptic_drvdata, tout_dev);

	return snprintf(buf, VIB_BUFSIZE, "force touch intensity: %u\n", drvdata->force_touch_intensity);
}

static DEVICE_ATTR(force_touch_intensity, 0660, force_touch_intensity_show, force_touch_intensity_store);

static ssize_t frequency_store(struct device *dev,
	struct device_attribute *devattr, const char *buf, size_t count)
{
	struct timed_output_dev *tdev = dev_get_drvdata(dev);
	struct max77865_haptic_drvdata *drvdata
		= container_of(tdev, struct max77865_haptic_drvdata, tout_dev);
	int num, ret;

	ret = kstrtoint(buf, 0, &num);
	if (ret) {
		pr_err("fail to get frequency\n");
		return -EINVAL;
	}

	pr_info("%s %d\n", __func__, num);

	ret = max77865_haptic_set_frequency(drvdata, num);
	if (ret)
		return ret;

	return count;
}

static ssize_t frequency_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct timed_output_dev *tdev = dev_get_drvdata(dev);
	struct max77865_haptic_drvdata *drvdata
		= container_of(tdev, struct max77865_haptic_drvdata, tout_dev);
	struct max77865_haptic_pdata *pdata = drvdata->pdata;

	return snprintf(buf, VIB_BUFSIZE, "frequency: %d\n", pdata->freq_num);
}

static DEVICE_ATTR(multi_freq, 0660, frequency_show, frequency_store);

static ssize_t haptic_engine_store(struct device *dev,
	struct device_attribute *devattr, const char *buf, size_t count)
{
	struct timed_output_dev *tdev = dev_get_drvdata(dev);
	struct max77865_haptic_drvdata *drvdata
		= container_of(tdev, struct max77865_haptic_drvdata, tout_dev);
	int i = 0, _data = 0, tmp = 0;

	if (sscanf(buf, "%6d", &_data) == 1) {
		if (_data > PACKET_MAX_SIZE * 4)
			pr_info("%s, [%d] packet size over\n", __func__, _data);
		else {
			drvdata->packet_size = _data / 4;
			drvdata->packet_cnt = 0;
			drvdata->f_packet_en = true;

			buf = strstr(buf, " ");

			for (i = 0; i < drvdata->packet_size; i++) {
				for (tmp = 0; tmp < 4; tmp++) {
					if (buf == NULL) {
						pr_err("%s, buf is NULL, Please check packet data again\n",
							__func__);
						drvdata->f_packet_en = false;
						return count;
					}

					if (sscanf(buf++, "%6d", &_data) == 1) {
						switch (tmp){
							case 0:
								drvdata->test_pac[i].time = _data;
								break;
							case 1:
								drvdata->test_pac[i].intensity = _data;
								break;
							case 2:
								drvdata->test_pac[i].freq = _data;
								break;
							case 3:
								drvdata->test_pac[i].overdrive = _data;
								break;
						}
						buf = strstr(buf, " ");
					} else {
						pr_err("%s, packet data error, Please check packet data again\n",
							__func__);
						drvdata->f_packet_en = false;
						return count;
					}
				}
			}
		}
	}

	return count;
}

static ssize_t haptic_engine_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct timed_output_dev *tdev = dev_get_drvdata(dev);
	struct max77865_haptic_drvdata *drvdata
		= container_of(tdev, struct max77865_haptic_drvdata, tout_dev);
	int i = 0;
	char *bufp = buf;

	bufp += snprintf(bufp, VIB_BUFSIZE, "\n");
	for (i = 0; i < drvdata->packet_size && drvdata->f_packet_en; i++) {
		bufp+= snprintf(bufp, VIB_BUFSIZE, "%u,", drvdata->test_pac[i].time);
		bufp+= snprintf(bufp, VIB_BUFSIZE, "%u,", drvdata->test_pac[i].intensity);
		bufp+= snprintf(bufp, VIB_BUFSIZE, "%u,", drvdata->test_pac[i].freq);
		bufp+= snprintf(bufp, VIB_BUFSIZE, "%u,", drvdata->test_pac[i].overdrive);
	}
	bufp += snprintf(bufp, VIB_BUFSIZE, "\n");

	return strlen(buf);
}

static DEVICE_ATTR(haptic_engine, 0660, haptic_engine_show, haptic_engine_store);

static int haptic_get_time(struct timed_output_dev *tout_dev)
{
	struct max77865_haptic_drvdata *drvdata
		= container_of(tout_dev, struct max77865_haptic_drvdata, tout_dev);
	struct hrtimer *timer = &drvdata->timer;

	if (hrtimer_active(timer)) {
		ktime_t remain = hrtimer_get_remaining(timer);
		struct timeval t = ktime_to_timeval(remain);

		return t.tv_sec * 1000 + t.tv_usec / 1000;
	}
	return 0;
}

static void haptic_enable(struct timed_output_dev *tout_dev, int value)
{
	struct max77865_haptic_drvdata *drvdata
		= container_of(tout_dev, struct max77865_haptic_drvdata, tout_dev);
	struct hrtimer *timer = &drvdata->timer;
	struct max77865_haptic_pdata *pdata = drvdata->pdata;
	int period = pdata->period;
	int duty = drvdata->duty;
	int ret = 0;

	flush_kthread_worker(&drvdata->kworker);
	ret = hrtimer_cancel(timer);

	value = min_t(int, value, (int)pdata->max_timeout);
	drvdata->timeout = value;
	drvdata->packet_running = false;

	if (value > 0) {
		mutex_lock(&drvdata->mutex);

		if (drvdata->f_packet_en) {
			drvdata->timeout = drvdata->test_pac[0].time;
			max77865_haptic_engine_set_packet(drvdata, drvdata->test_pac[0]);

#if defined(CONFIG_SSP_MOTOR_CALLBACK)
			if(drvdata->intensity == 0)
				setSensorCallback(false, 0);
			else
				setSensorCallback(true, drvdata->timeout);
#endif
		}
		else {
			pwm_config(drvdata->pwm, duty, period);
			ret = pwm_enable(drvdata->pwm);
			if (ret)
				pr_err("%s: error to enable pwm %d\n", __func__, ret);

			if (pdata->multi_frequency)
				pr_debug("%d %u %ums\n", pdata->freq_num, drvdata->duty, value);
			else
				pr_debug("%u %ums\n", drvdata->duty, value);

			max77865_haptic_i2c(drvdata, true);
#if defined(CONFIG_SSP_MOTOR_CALLBACK)
			setSensorCallback(true, value);
#endif
		}

		mutex_unlock(&drvdata->mutex);
		hrtimer_start(timer, ns_to_ktime((u64)drvdata->timeout * NSEC_PER_MSEC),
			HRTIMER_MODE_REL);
	}
	else if (value == 0) {
		mutex_lock(&drvdata->mutex);

		drvdata->f_packet_en = 0;
		drvdata->packet_cnt = 0;
		drvdata->packet_size = 0;

		max77865_haptic_i2c(drvdata, false);

		pwm_config(drvdata->pwm, period >> 1, period);
		pwm_disable(drvdata->pwm);

#if defined(CONFIG_SSP_MOTOR_CALLBACK)
		setSensorCallback(false, 0);
#endif
		pr_debug("off\n");

		mutex_unlock(&drvdata->mutex);
	}
}

static enum hrtimer_restart haptic_timer_func(struct hrtimer *timer)
{
	struct max77865_haptic_drvdata *drvdata
		= container_of(timer, struct max77865_haptic_drvdata, timer);

	pr_info("%s\n", __func__);

	queue_kthread_work(&drvdata->kworker, &drvdata->kwork);
	return HRTIMER_NORESTART;
}

static void haptic_work(struct kthread_work *work)
{
	struct max77865_haptic_drvdata *drvdata
		= container_of(work, struct max77865_haptic_drvdata, kwork);
	int period = drvdata->pdata->period;
	struct hrtimer *timer = &drvdata->timer;

	mutex_lock(&drvdata->mutex);

	if (drvdata->f_packet_en) {
		if (++drvdata->packet_cnt >= drvdata->packet_size) {
			drvdata->f_packet_en = 0;
			drvdata->packet_cnt = 0;
			drvdata->packet_size = 0;
		} else {
			max77865_haptic_engine_set_packet(drvdata, drvdata->test_pac[drvdata->packet_cnt]);
			hrtimer_start(timer, ns_to_ktime((u64)drvdata->test_pac[drvdata->packet_cnt].time * NSEC_PER_MSEC), HRTIMER_MODE_REL);
#if defined(CONFIG_SSP_MOTOR_CALLBACK)
			if(drvdata->intensity == 0)
				setSensorCallback(false, 0);
			else
				setSensorCallback(true, drvdata->test_pac[drvdata->packet_cnt].time);
#endif
			goto unlock;
		}
	}

	max77865_haptic_i2c(drvdata, false);
	pwm_config(drvdata->pwm, period >> 1, period);
	pwm_disable(drvdata->pwm);

#if defined(CONFIG_SSP_MOTOR_CALLBACK)
	setSensorCallback(false,0);
#endif
	pr_debug("off\n");
unlock:
	mutex_unlock(&drvdata->mutex);
}

#if defined(CONFIG_OF)
static struct max77865_haptic_pdata *of_max77865_haptic_dt(struct device *dev)
{
	struct device_node *np_root = dev->parent->of_node;
	struct device_node *np;
	struct max77865_haptic_pdata *pdata;
	u32 temp;
//	const char *temp_str;
	int ret, i;

	pdata = kzalloc(sizeof(struct max77865_haptic_pdata),
		GFP_KERNEL);
	if (!pdata) {
		pr_err("%s: failed to allocate driver data\n", __func__);
		return NULL;
	}

	np = of_find_node_by_name(np_root,
			"haptic");
	if (np == NULL) {
		pr_err("%s : error to get dt node\n", __func__);
		goto err_parsing_dt;
	}

	ret = of_property_read_u32(np,
			"haptic,model", &temp);
	if (IS_ERR_VALUE(ret)) {
		pr_err("%s : error to get dt node max_timeout\n", __func__);
		goto err_parsing_dt;
	} else
		pdata->model = (u16)temp;

	ret = of_property_read_u32(np,
			"haptic,max_timeout", &temp);
	if (IS_ERR_VALUE(ret)) {
		pr_err("%s : error to get dt node max_timeout\n", __func__);
		goto err_parsing_dt;
	} else
		pdata->max_timeout = (u16)temp;

	ret = of_property_read_u32(np, "haptic,multi_frequency", &temp);
	if (IS_ERR_VALUE(ret)) {
		pr_err("%s : error to get dt node multi_frequency\n", __func__);
		pdata->multi_frequency = 0;
	} else
		pdata->multi_frequency = (int)temp;

	if (pdata->multi_frequency) {
		pdata->multi_freq_duty
			= devm_kzalloc(dev, sizeof(u32)*pdata->multi_frequency, GFP_KERNEL);
		if (!pdata->multi_freq_duty) {
			pr_err("%s: failed to allocate duty data\n", __func__);
			goto err_parsing_dt;
		}
		pdata->multi_freq_period 
			= devm_kzalloc(dev, sizeof(u32)*pdata->multi_frequency, GFP_KERNEL);
		if (!pdata->multi_freq_period) {
			pr_err("%s: failed to allocate period data\n", __func__);
			goto err_parsing_dt;
		}

		ret = of_property_read_u32_array(np, "haptic,duty", pdata->multi_freq_duty,
				pdata->multi_frequency);
		if (IS_ERR_VALUE(ret)) {
			pr_err("%s : error to get dt node duty\n", __func__);
			goto err_parsing_dt;
		}

		ret = of_property_read_u32_array(np, "haptic,period", pdata->multi_freq_period,
				pdata->multi_frequency);
		if (IS_ERR_VALUE(ret)) {
			pr_err("%s : error to get dt node period\n", __func__);
			goto err_parsing_dt;
		}

		pdata->duty = pdata->multi_freq_duty[0];
		pdata->period = pdata->multi_freq_period[0];
		pdata->freq_num = 0;
	}
	else {
		ret = of_property_read_u32(np,
				"haptic,duty", &temp);
		if (IS_ERR_VALUE(ret)) {
			pr_err("%s : error to get dt node duty\n", __func__);
			goto err_parsing_dt;
		} else
			pdata->duty = (u16)temp;

		ret = of_property_read_u32(np,
				"haptic,period", &temp);
		if (IS_ERR_VALUE(ret)) {
			pr_err("%s : error to get dt node period\n", __func__);
			goto err_parsing_dt;
		} else
			pdata->period = (u16)temp;
	}

	ret = of_property_read_u32(np,
			"haptic,pwm_id", &temp);
	if (IS_ERR_VALUE(ret)) {
		pr_err("%s : error to get dt node pwm_id\n", __func__);
		goto err_parsing_dt;
	} else
		pdata->pwm_id = (u16)temp;

/*	if (2 == pdata->model) {
		np = of_find_compatible_node(NULL, NULL, "vib,ldo_en");
		if (np == NULL) {
			pr_debug("%s: error to get motorldo dt node\n", __func__);
			goto err_parsing_dt;
		}
		pdata->gpio = of_get_named_gpio(np, "ldo_en", 0);
		if (!gpio_is_valid(pdata->gpio))
			pr_err("[VIB]failed to get ldo_en gpio\n");
	} else {
		pdata->gpio = 0;
		ret = of_property_read_string(np,
				"haptic,regulator_name", &temp_str);
		if (IS_ERR_VALUE(ret)) {
			pr_err("%s : error to get dt node regulator_name\n", __func__);
			goto err_parsing_dt;
		} else
			pdata->regulator_name = (char *)temp_str;

		pr_debug("regulator_name = %s\n", pdata->regulator_name);
	}*/

	/* debugging */
	pr_debug("model = %d\n", pdata->model);
	pr_debug("max_timeout = %d\n", pdata->max_timeout);
	if (pdata->multi_frequency) {
		pr_debug("multi frequency = %d\n", pdata->multi_frequency);
		for (i=0; i<pdata->multi_frequency; i++) {
			pr_debug("duty[%d] = %d\n", i, pdata->multi_freq_duty[i]);
			pr_debug("period[%d] = %d\n", i, pdata->multi_freq_period[i]);
		}
	}
	else {
		pr_debug("duty = %d\n", pdata->duty);
		pr_debug("period = %d\n", pdata->period);
	}
	pr_debug("pwm_id = %d\n", pdata->pwm_id);

	return pdata;

err_parsing_dt:
	kfree(pdata);
	return NULL;
}
#endif

static int max77865_haptic_probe(struct platform_device *pdev)
{
	int error = 0;
	struct max77865_dev *max77865 = dev_get_drvdata(pdev->dev.parent);
	struct max77865_platform_data *max77865_pdata
		= dev_get_platdata(max77865->dev);
	struct max77865_haptic_pdata *pdata
		= max77865_pdata->haptic_data;
	struct max77865_haptic_drvdata *drvdata;
	struct task_struct *kworker_task;

#if defined(CONFIG_OF)
	if (pdata == NULL) {
		pdata = of_max77865_haptic_dt(&pdev->dev);
		if (!pdata) {
			pr_err("max77865-haptic : %s not found haptic dt!\n",
					__func__);
			return -1;
		}
	}
#else
	if (pdata == NULL) {
		pr_err("%s: no pdata\n", __func__);
		return -ENODEV;
	}
#endif /* CONFIG_OF */

	drvdata = kzalloc(sizeof(struct max77865_haptic_drvdata), GFP_KERNEL);
	if (!drvdata) {
		kfree(pdata);
		return -ENOMEM;
	}
	platform_set_drvdata(pdev, drvdata);
	drvdata->max77865 = max77865;
	drvdata->i2c = max77865->i2c;
	drvdata->pdata = pdata;
	drvdata->intensity = 8000;
	drvdata->duty = pdata->duty;

	init_kthread_worker(&drvdata->kworker);
	mutex_init(&drvdata->mutex);
	kworker_task = kthread_run(kthread_worker_fn,
		   &drvdata->kworker, "max77865_haptic");
	if (IS_ERR(kworker_task)) {
		pr_err("Failed to create message pump task\n");
		error = -ENOMEM;
		goto err_kthread;
	}

	init_kthread_work(&drvdata->kwork, haptic_work);

	drvdata->pwm = pwm_request(pdata->pwm_id, "vibrator");
	if (IS_ERR(drvdata->pwm)) {
		error = -EFAULT;
		pr_err("Failed to request pwm, err num: %d\n", error);
		goto err_pwm_request;
	} else
		pwm_config(drvdata->pwm, pdata->period >> 1, pdata->period);

/*	if (!pdata->gpio) {
		drvdata->regulator	= regulator_get(NULL, pdata->regulator_name);

		if (IS_ERR(drvdata->regulator)) {
			error = -EFAULT;
			pr_err("Failed to get vmoter regulator, err num: %d\n", error);
			goto err_regulator_get;
		}
	}*/

	max77865_haptic_init_reg(drvdata);

	/* hrtimer init */
	hrtimer_init(&drvdata->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	drvdata->timer.function = haptic_timer_func;

	/* timed_output_dev init*/
	drvdata->tout_dev.name = "vibrator";
	drvdata->tout_dev.get_time = haptic_get_time;
	drvdata->tout_dev.enable = haptic_enable;

	g_drvdata = drvdata;
	
	drvdata->dev = sec_device_create(drvdata, "motor");
	if (IS_ERR(drvdata->dev)) {
		error = -ENODEV;
		pr_err("Failed to create device %d\n", error);
		goto exit_sec_devices;
	}
	error = sysfs_create_group(&drvdata->dev->kobj, &sec_motor_attr_group);
	if (error) {
		error = -ENODEV;
		pr_err("Failed to create sysfs %d\n", error);
		goto exit_sysfs;
	}

	error = timed_output_dev_register(&drvdata->tout_dev);
	if (error < 0) {
		error = -EFAULT;
		pr_err("Failed to register timed_output : %d\n", error);
		goto err_timed_output_register;
	}

	error = sysfs_create_file(&drvdata->tout_dev.dev->kobj,
				&dev_attr_intensity.attr);
	if (error < 0) {
		pr_err("Failed to register intensity sysfs : %d\n", error);
		goto err_timed_output_register;
	}

	error = sysfs_create_file(&drvdata->tout_dev.dev->kobj,
				&dev_attr_force_touch_intensity.attr);
	if (error < 0) {
		pr_err("Failed to register force touch intensity sysfs : %d\n", error);
		goto err_timed_output_register;
	}

	if (pdata->multi_frequency) {
		error = sysfs_create_file(&drvdata->tout_dev.dev->kobj,
					&dev_attr_multi_freq.attr);
		if (error < 0) {
			pr_err("Failed to register multi_freq sysfs : %d\n", error);
			goto err_timed_output_register;
		}

		error = sysfs_create_file(&drvdata->tout_dev.dev->kobj,
					&dev_attr_haptic_engine.attr);
		if (error < 0) {
			pr_err("Failed to register haptic_engine sysfs : %d\n", error);
			goto err_timed_output_register;
		}
	}

	return error;

err_timed_output_register:
	sysfs_remove_group(&drvdata->dev->kobj, &sec_motor_attr_group);
exit_sysfs:
	sec_device_destroy(drvdata->dev->devt);
exit_sec_devices:
//	regulator_put(drvdata->regulator);
//err_regulator_get:
	pwm_free(drvdata->pwm);
err_kthread:
err_pwm_request:
	kfree(drvdata);
	kfree(pdata);
	return error;
}

static int max77865_haptic_remove(struct platform_device *pdev)
{
	struct max77865_haptic_drvdata *drvdata
		= platform_get_drvdata(pdev);

	max77865_motor_boost_control(drvdata, BOOST_OFF);
	timed_output_dev_unregister(&drvdata->tout_dev);
	sysfs_remove_group(&drvdata->dev->kobj, &sec_motor_attr_group);
	sec_device_destroy(drvdata->dev->devt);
//	regulator_put(drvdata->regulator);
	pwm_free(drvdata->pwm);
	max77865_haptic_i2c(drvdata, false);
	kfree(drvdata);
	return 0;
}

extern int haptic_homekey_press(void)
{
	struct hrtimer *timer;

	if(g_drvdata == NULL)
		return -1;
	timer = &g_drvdata->timer;

	max77865_haptic_init_reg(g_drvdata);
	max77865_motor_boost_control(g_drvdata, BOOST_ON);

	mutex_lock(&g_drvdata->mutex);
	g_drvdata->timeout = HOMEKEY_DURATION;
	max77865_haptic_set_frequency(g_drvdata, HOMEKEY_PRESS_FREQ);
	max77865_haptic_set_intensity(g_drvdata, g_drvdata->force_touch_intensity);
	max77865_haptic_i2c(g_drvdata, true);

	pr_info("%s homekey press freq:%d, intensity:%d, time:%d\n", __func__, HOMEKEY_PRESS_FREQ, g_drvdata->force_touch_intensity, g_drvdata->timeout);
	mutex_unlock(&g_drvdata->mutex);

	hrtimer_start(timer, ns_to_ktime((u64)g_drvdata->timeout * NSEC_PER_MSEC),
		HRTIMER_MODE_REL);

	return 0;
}

extern int haptic_homekey_release(void)
{
	struct hrtimer *timer;

	if(g_drvdata == NULL)
		return -1;
	timer = &g_drvdata->timer;

	mutex_lock(&g_drvdata->mutex);
	g_drvdata->timeout = HOMEKEY_DURATION;
	max77865_haptic_set_frequency(g_drvdata, HOMEKEY_RELEASE_FREQ);
	max77865_haptic_set_intensity(g_drvdata, g_drvdata->force_touch_intensity);
	max77865_haptic_i2c(g_drvdata, true);

	pr_info("%s homekey release freq:%d, intensity:%d, time:%d\n", __func__, HOMEKEY_RELEASE_FREQ, g_drvdata->force_touch_intensity, g_drvdata->timeout);
	mutex_unlock(&g_drvdata->mutex);

	hrtimer_start(timer, ns_to_ktime((u64)g_drvdata->timeout * NSEC_PER_MSEC),
		HRTIMER_MODE_REL);

	return 0;
}

static int max77865_haptic_suspend(struct platform_device *pdev,
		pm_message_t state)
{
	struct max77865_haptic_drvdata *drvdata
		= platform_get_drvdata(pdev);

//	motor_vdd_en(drvdata, false);
	flush_kthread_worker(&drvdata->kworker);
	hrtimer_cancel(&drvdata->timer);
	max77865_motor_boost_control(drvdata, BOOST_OFF);
	return 0;
}

static int max77865_haptic_resume(struct platform_device *pdev)
{
	struct max77865_haptic_drvdata *drvdata
		= platform_get_drvdata(pdev);

	max77865_haptic_init_reg(drvdata);
	max77865_motor_boost_control(drvdata, BOOST_ON);
	return 0;
}

static void max77865_haptic_shutdown(struct platform_device *pdev)
{
	struct max77865_haptic_drvdata *drvdata
		= platform_get_drvdata(pdev);

	max77865_motor_boost_control(drvdata, BOOST_OFF);
}

static struct platform_driver max77865_haptic_driver = {
	.probe		= max77865_haptic_probe,
	.remove		= max77865_haptic_remove,
	.suspend	= max77865_haptic_suspend,
	.resume		= max77865_haptic_resume,
	.shutdown	= max77865_haptic_shutdown,
	.driver = {
		.name	= "max77865-haptic",
		.owner	= THIS_MODULE,
	},
};

static int __init max77865_haptic_init(void)
{
	return platform_driver_register(&max77865_haptic_driver);
}
module_init(max77865_haptic_init);

static void __exit max77865_haptic_exit(void)
{
	platform_driver_unregister(&max77865_haptic_driver);
}
module_exit(max77865_haptic_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("max77865 haptic driver");
