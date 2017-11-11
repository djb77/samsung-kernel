/*
 * haptic motor driver for max77854_haptic.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define pr_fmt(fmt) "[VIB] " fmt
#define DEBUG	1

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/timed_output.h>
#include <linux/hrtimer.h>
#include <linux/pwm.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/i2c.h>
#include <linux/regulator/consumer.h>
#include <linux/mfd/max77854.h>
#include <linux/mfd/max77854-private.h>
#include <linux/sec_sysfs.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/kthread.h>
#if defined(CONFIG_MOTOR_DRV_SENSOR)
#include "max77854_haptic.h"
#include "../sensorhub/brcm/ssp_motor.h"
#endif

#define MAX_INTENSITY		10000

#define MOTOR_LRA			(1<<7)
#define MOTOR_EN			(1<<6)
#define EXT_PWM				(0<<5)
#define DIVIDER_128			(1<<1)
#define MRDBTMER_MASK		(0x7)
#define MREN					(1 << 3)
#define BIASEN				(1 << 7)

#if defined(CONFIG_MOTOR_DRV_SENSOR)
int (*sensorCallback)(int);
#endif

struct max77854_haptic_drvdata {
	struct max77854_dev *max77854;
	struct i2c_client *i2c;
	struct device *dev;
	struct max77854_haptic_pdata *pdata;
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
};

#if defined(CONFIG_MOTOR_DRV_SENSOR)
int setMotorCallback(int (*callback)(int state))
{
	if (!callback)
		sensorCallback = callback;
	else
		pr_debug("sensor callback is Null !%s\n", __func__);
	return 0;
}
#endif

static int motor_vdd_en(struct max77854_haptic_drvdata *drvdata, bool en)
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
}

static void max77854_haptic_init_reg(struct max77854_haptic_drvdata *drvdata)
{
	int ret;

	motor_vdd_en(drvdata, true);

	usleep_range(6000, 6000);

	ret = max77854_update_reg(drvdata->i2c,
			MAX77854_PMIC_REG_MAINCTRL1, 0xff, BIASEN);
	if (ret)
		pr_err("i2c REG_BIASEN update error %d\n", ret);

	ret = max77854_update_reg(drvdata->i2c,
		MAX77854_PMIC_REG_MCONFIG, 0x0, MOTOR_EN);
	if (ret)
		pr_err("i2c MOTOR_EN update error %d\n", ret);

	ret = max77854_update_reg(drvdata->i2c,
		MAX77854_PMIC_REG_MCONFIG, 0xff, MOTOR_LRA);
	if (ret)
		pr_err("i2c MOTOR_LPA update error %d\n", ret);
}

static void max77854_haptic_i2c(struct max77854_haptic_drvdata *drvdata,
	bool en)
{

	if (en)
		max77854_update_reg(drvdata->i2c,
			MAX77854_PMIC_REG_MCONFIG, 0xff, MOTOR_EN);
	else
		max77854_update_reg(drvdata->i2c,
			MAX77854_PMIC_REG_MCONFIG, 0x0, MOTOR_EN);
}

static ssize_t store_duty(struct device *dev,
	struct device_attribute *devattr, const char *buf, size_t count)
{
	struct max77854_haptic_drvdata *drvdata
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
	struct max77854_haptic_drvdata *drvdata
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
	struct max77854_haptic_drvdata *drvdata = dev_get_drvdata(dev);

	return sprintf(buf, "duty: %u, period%u\n",
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
	struct max77854_haptic_drvdata *drvdata
		= container_of(tdev, struct max77854_haptic_drvdata, tout_dev);
	int duty = drvdata->pdata->period >> 1;
	int intensity = 0, ret = 0;

	ret = kstrtoint(buf, 0, &intensity);

	if (intensity < 0 || MAX_INTENSITY < intensity) {
		pr_err("out of rage\n");
		return -EINVAL;
	}

	if (MAX_INTENSITY == intensity)
		duty = drvdata->pdata->duty;
	else if (0 != intensity) {
		long long tmp = drvdata->pdata->duty >> 1;

		tmp *= (intensity / 100);
		duty += (int)(tmp / 100);
	}

	drvdata->intensity = intensity;
	drvdata->duty = duty;

	pwm_config(drvdata->pwm, duty, drvdata->pdata->period);

	return count;
}

static ssize_t intensity_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct timed_output_dev *tdev = dev_get_drvdata(dev);
	struct max77854_haptic_drvdata *drvdata
		= container_of(tdev, struct max77854_haptic_drvdata, tout_dev);

	return sprintf(buf, "intensity: %u\n", drvdata->intensity);
}

static DEVICE_ATTR(intensity, 0660, intensity_show, intensity_store);

static int haptic_get_time(struct timed_output_dev *tout_dev)
{
	struct max77854_haptic_drvdata *drvdata
		= container_of(tout_dev, struct max77854_haptic_drvdata, tout_dev);
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
	struct max77854_haptic_drvdata *drvdata
		= container_of(tout_dev, struct max77854_haptic_drvdata, tout_dev);
	struct hrtimer *timer = &drvdata->timer;
	int period = drvdata->pdata->period;
	int duty = drvdata->duty;
	int ret = 0;

	flush_kthread_worker(&drvdata->kworker);
	ret = hrtimer_cancel(timer);

	value = min_t(int, value, (int)drvdata->pdata->max_timeout);
	drvdata->timeout = value;
	pr_debug("%u %ums\n", drvdata->duty, value);
	if (value > 0) {
		mutex_lock(&drvdata->mutex);

#if defined(CONFIG_MOTOR_DRV_SENSOR)
		if (sensorCallback != NULL)
			sensorCallback(true);
		else {
			pr_info("%s sensorCallback is null.start\n", __func__);
			sensorCallback = getMotorCallback();
			sensorCallback(true);
		}
#endif
		pwm_config(drvdata->pwm, duty, period);
		ret = pwm_enable(drvdata->pwm);
		if (ret)
			pr_err("%s: error to enable pwm %d\n", __func__, ret);
		max77854_haptic_i2c(drvdata, true);

		mutex_unlock(&drvdata->mutex);
		hrtimer_start(timer, ns_to_ktime((u64)drvdata->timeout * NSEC_PER_MSEC),
			HRTIMER_MODE_REL);
	}
	else if (value == 0) {
		mutex_lock(&drvdata->mutex);

		max77854_haptic_i2c(drvdata, false);
		pwm_config(drvdata->pwm, period >> 1, period);
		pwm_disable(drvdata->pwm);

#if defined(CONFIG_MOTOR_DRV_SENSOR)
		if (sensorCallback != NULL)
			sensorCallback(false);
		else {
			pr_debug("%s sensorCallback is null.start\n", __func__);
			sensorCallback = getMotorCallback();
			sensorCallback(true);
		}
#endif
		pr_debug("off\n");

		mutex_unlock(&drvdata->mutex);
	}
}

static enum hrtimer_restart haptic_timer_func(struct hrtimer *timer)
{
	struct max77854_haptic_drvdata *drvdata
		= container_of(timer, struct max77854_haptic_drvdata, timer);

	queue_kthread_work(&drvdata->kworker, &drvdata->kwork);
	return HRTIMER_NORESTART;
}

static void haptic_work(struct kthread_work *work)
{
	struct max77854_haptic_drvdata *drvdata
		= container_of(work, struct max77854_haptic_drvdata, kwork);
	int period = drvdata->pdata->period;

	mutex_lock(&drvdata->mutex);

	max77854_haptic_i2c(drvdata, false);
	pwm_config(drvdata->pwm, period >> 1, period);
	pwm_disable(drvdata->pwm);

#if defined(CONFIG_MOTOR_DRV_SENSOR)
	if (sensorCallback != NULL)
		sensorCallback(false);
	else {
		pr_debug("%s sensorCallback is null.start\n", __func__);
		sensorCallback = getMotorCallback();
		sensorCallback(true);
	}
#endif
	pr_debug("off\n");

	mutex_unlock(&drvdata->mutex);
}

#if defined(CONFIG_OF)
static struct max77854_haptic_pdata *of_max77854_haptic_dt(struct device *dev)
{
	struct device_node *np_root = dev->parent->of_node;
	struct device_node *np;
	struct max77854_haptic_pdata *pdata;
	u32 temp;
	const char *temp_str;
	int ret;

	pdata = kzalloc(sizeof(struct max77854_haptic_pdata),
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

	ret = of_property_read_u32(np,
			"haptic,pwm_id", &temp);
	if (IS_ERR_VALUE(ret)) {
		pr_err("%s : error to get dt node pwm_id\n", __func__);
		goto err_parsing_dt;
	} else
		pdata->pwm_id = (u16)temp;

	if (2 == pdata->model) {
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
	}

	/* debugging */
	pr_debug("model = %d\n", pdata->model);
	pr_debug("max_timeout = %d\n", pdata->max_timeout);
	pr_debug("duty = %d\n", pdata->duty);
	pr_debug("period = %d\n", pdata->period);
	pr_debug("pwm_id = %d\n", pdata->pwm_id);

	return pdata;

err_parsing_dt:
	kfree(pdata);
	return NULL;
}
#endif

static int max77854_haptic_probe(struct platform_device *pdev)
{
	int error = 0;
	struct max77854_dev *max77854 = dev_get_drvdata(pdev->dev.parent);
	struct max77854_platform_data *max77854_pdata
		= dev_get_platdata(max77854->dev);
	struct max77854_haptic_pdata *pdata
		= max77854_pdata->haptic_data;
	struct max77854_haptic_drvdata *drvdata;
	struct task_struct *kworker_task;

#if defined(CONFIG_OF)
	if (pdata == NULL) {
		pdata = of_max77854_haptic_dt(&pdev->dev);
		if (!pdata) {
			pr_err("max77854-haptic : %s not found haptic dt!\n",
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

	drvdata = kzalloc(sizeof(struct max77854_haptic_drvdata), GFP_KERNEL);
	if (!drvdata) {
		kfree(pdata);
		return -ENOMEM;
	}
	platform_set_drvdata(pdev, drvdata);
	drvdata->max77854 = max77854;
	drvdata->i2c = max77854->i2c;
	drvdata->pdata = pdata;
	drvdata->intensity = 8000;
	drvdata->duty = pdata->duty;

	init_kthread_worker(&drvdata->kworker);
	mutex_init(&drvdata->mutex);
	kworker_task = kthread_run(kthread_worker_fn,
		   &drvdata->kworker, "max77854_haptic");
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

	if (!pdata->gpio) {
		drvdata->regulator	= regulator_get(NULL, pdata->regulator_name);

		if (IS_ERR(drvdata->regulator)) {
			error = -EFAULT;
			pr_err("Failed to get vmoter regulator, err num: %d\n", error);
			goto err_regulator_get;
		}
	}

	max77854_haptic_init_reg(drvdata);

	/* hrtimer init */
	hrtimer_init(&drvdata->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	drvdata->timer.function = haptic_timer_func;

	/* timed_output_dev init*/
	drvdata->tout_dev.name = "vibrator";
	drvdata->tout_dev.get_time = haptic_get_time;
	drvdata->tout_dev.enable = haptic_enable;

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
		pr_err("Failed to register sysfs : %d\n", error);
		goto err_timed_output_register;
	}

	return error;

err_timed_output_register:
	sysfs_remove_group(&drvdata->dev->kobj, &sec_motor_attr_group);
exit_sysfs:
	sec_device_destroy(drvdata->dev->devt);
exit_sec_devices:
	regulator_put(drvdata->regulator);
err_regulator_get:
	pwm_free(drvdata->pwm);
err_kthread:
err_pwm_request:
	kfree(drvdata);
	kfree(pdata);
	return error;
}

static int max77854_haptic_remove(struct platform_device *pdev)
{
	struct max77854_haptic_drvdata *drvdata
		= platform_get_drvdata(pdev);

	timed_output_dev_unregister(&drvdata->tout_dev);
	sysfs_remove_group(&drvdata->dev->kobj, &sec_motor_attr_group);
	sec_device_destroy(drvdata->dev->devt);
	regulator_put(drvdata->regulator);
	pwm_free(drvdata->pwm);
	max77854_haptic_i2c(drvdata, false);
	kfree(drvdata);

	return 0;
}

static int max77854_haptic_suspend(struct platform_device *pdev,
		pm_message_t state)
{
	struct max77854_haptic_drvdata *drvdata
		= platform_get_drvdata(pdev);

	motor_vdd_en(drvdata, false);
	flush_kthread_worker(&drvdata->kworker);
	hrtimer_cancel(&drvdata->timer);
	return 0;
}
static int max77854_haptic_resume(struct platform_device *pdev)
{
	struct max77854_haptic_drvdata *drvdata
		= platform_get_drvdata(pdev);

	max77854_haptic_init_reg(drvdata);
	return 0;
}

static struct platform_driver max77854_haptic_driver = {
	.probe		= max77854_haptic_probe,
	.remove		= max77854_haptic_remove,
	.suspend		= max77854_haptic_suspend,
	.resume		= max77854_haptic_resume,
	.driver = {
		.name	= "max77854-haptic",
		.owner	= THIS_MODULE,
	},
};

static int __init max77854_haptic_init(void)
{
	return platform_driver_register(&max77854_haptic_driver);
}
module_init(max77854_haptic_init);

static void __exit max77854_haptic_exit(void)
{
	platform_driver_unregister(&max77854_haptic_driver);
}
module_exit(max77854_haptic_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("max77854 haptic driver");
