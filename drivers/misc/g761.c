/*
 * g761.c - Fan Driver
 *
 *  Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *      http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/delay.h>

//#define TEST_G761

enum g761_registers {
	G761_FAN_SET_CNT_REG  = 0x00,
	G761_FAN_ACT_CNT_REG,
	G761_STATUS_REG,
	G761_FAN_SET_OUT_REG,
	G761_FAN_CMD1_REG,
	G761_FAN_CMD2_REG,
};

enum g761_fan_rpm {
	G761_FAN_RPM_OFF  = 255,
	G761_FAN_RPM_2000 = 246,
	G761_FAN_RPM_3000 = 164,
	G761_FAN_RPM_4000 = 123,
	G761_FAN_RPM_5000 = 98,
	G761_FAN_RPM_6000 = 82,
	G761_FAN_RPM_7000 = 70,
	G761_FAN_RPM_8000 = 61,
	G761_FAN_RPM_9000 = 55,
};

enum g761_fan_volt {
	G761_FAN_VOLT_OFF  = 0,
	G761_FAN_VOLT_3540 = 150,
	G761_FAN_VOLT_3700 = 160,
	G761_FAN_VOLT_3870 = 170,
	G761_FAN_VOLT_4030 = 180,
	G761_FAN_VOLT_4230 = 190,
};

#define MODEL_NAME               "G761"
#define MODULE_NAME              "fan"

#define FAN_NR_MODE	2

#define FAN_LINEAR_MODE	0
#define FAN_PWM_MODE	1

#define FAN_LINEAR_NR_RPM	6
#define FAN_PWM_NR_RPM	9

struct g761_p {
	struct i2c_client *client;
	int mode;
	int fan_rpm;
	int fan_cmd1;
	int fan_cmd2;
	int fan_boost_gpio;
};

#ifdef TEST_G761
static int g761_get_all_config(struct i2c_client *client)
{
	int i = 0, val = 0;

	for (i = 0 ; i <= G761_FAN_CMD2_REG ; i++) {
		val = i2c_smbus_read_byte_data(client, i);
		if (val < 0) {
			dev_err(&client->dev, "%s - failed to read reg0x%x=%x\n",
				__func__, i, val);
			return val;
		}
		dev_info(&client->dev, "%s - reg0x%x=0x%x\n", __func__, i, val);
	}
	return 0;
}
#endif

static int g761_get_config(struct i2c_client *client, u8 addr)
{
	int val = 0;

	val = i2c_smbus_read_byte_data(client, addr);
	if (val < 0) {
		dev_err(&client->dev, "%s - failed to read reg0x%x=%x\n",
			__func__, addr, val);
		return val;
	}
	dev_info(&client->dev, "%s - reg0x%x=0x%x\n", __func__, addr, val);

	return val;
}
static int g761_set_config(struct i2c_client *client, u8 addr, u8 data)
{
	int ret = 0;

	ret = i2c_smbus_write_byte_data(client, addr, data);
	if (ret < 0) {
		dev_err(&client->dev, "%s - failed to write reg0x%x=%x, ret=%x\n",
			__func__, addr, data, ret);
		return ret;
	}
	dev_info(&client->dev, "%s - write reg0x%x=0x%x\n", __func__, addr, data);

	return 0;
}

static int g761_fan_rpm_trans(struct i2c_client *client, int val)
{
	struct g761_p *data = i2c_get_clientdata(client);
	int ret = 0;
	int max_val = 0;
	int rpm_data[FAN_NR_MODE][FAN_PWM_NR_RPM] = {
		{G761_FAN_VOLT_OFF, G761_FAN_VOLT_3540, G761_FAN_VOLT_3700,
		G761_FAN_VOLT_3870, G761_FAN_VOLT_4030,
		G761_FAN_VOLT_4230, G761_FAN_VOLT_OFF, },
		{G761_FAN_RPM_OFF, G761_FAN_RPM_2000, G761_FAN_RPM_3000,
		G761_FAN_RPM_4000, G761_FAN_RPM_5000,
		G761_FAN_RPM_6000, G761_FAN_RPM_7000,
		G761_FAN_RPM_8000, G761_FAN_RPM_9000} };

	dev_info(&client->dev, "%s - original rpm val is %d\n", __func__, val);

	max_val = (data->mode == FAN_LINEAR_MODE) ?
		(FAN_LINEAR_NR_RPM -1) : (FAN_PWM_NR_RPM - 1);

	if (val < 0 || val > max_val)
		val = 0;

	ret = rpm_data[data->mode][val];

	dev_info(&client->dev, "%s - now rpm val is %d\n", __func__, ret);

	return ret;
}

static ssize_t g761_fan_rpm_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct g761_p *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;

	dev_info(&client->dev, "%s - fan_rpm=%d\n", __func__, data->fan_rpm);
#ifdef TEST_G761
	(void) g761_get_all_config(client);
#endif
	return sprintf(buf, "%d\n", data->fan_rpm);
}

static ssize_t g761_fan_rpm_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct g761_p *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int rpm = 0, reg = 0;
	int ret;

	ret = sscanf(buf, "%d\n", &rpm);
	if (!ret)
		return -EINVAL;

	if (data->fan_rpm == rpm) {
		dev_info(&client->dev, "%s - the same rpm value\n", __func__);
		return count;
	}

	data->fan_rpm = rpm;
	rpm = g761_fan_rpm_trans(client, data->fan_rpm);

	if (rpm == G761_FAN_RPM_OFF || rpm == G761_FAN_VOLT_OFF) {
		data->fan_rpm = 0;
		gpio_direction_output(data->fan_boost_gpio, 0);
		msleep(20);
		dev_info(&client->dev, "%s - fan off and fan_boost_gpio=%d\n",
			__func__, gpio_get_value(data->fan_boost_gpio));
		return count;
	}

	gpio_direction_output(data->fan_boost_gpio, 1);
	msleep(20);
	dev_info(&client->dev, "%s - fan on and fan_boost_gpio=%d\n",
			__func__, gpio_get_value(data->fan_boost_gpio));

	dev_info(&client->dev, "%s - fan_rpm=%d\n", __func__, data->fan_rpm);

	ret = g761_set_config(client, G761_FAN_CMD1_REG, data->fan_cmd1);
	if (ret) {
		dev_err(&client->dev, "%s - failed to set fan_cmd1, ret=%d\n",
			__func__, ret);
		return count;
	}

	ret = g761_set_config(client, G761_FAN_CMD2_REG, data->fan_cmd2);
	if (ret) {
		dev_err(&client->dev, "%s - failed to set fan_cmd2, ret=%d\n",
			__func__, ret);
		return count;
	}

	if (data->mode == FAN_PWM_MODE)
		reg = G761_FAN_SET_CNT_REG;
	else
		reg = G761_FAN_SET_OUT_REG;
	ret = g761_set_config(client, reg, rpm);
	if (ret) {
		dev_err(&client->dev, "%s - failed to set reg0x%x, ret=%d\n",
			__func__, reg, ret);
		return count;
	}

	dev_info(&client->dev, "%s - set fan_rpm as normal\n", __func__);

	return count;
}

static ssize_t g761_fan_cmd1_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct g761_p *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int val = 0;

	val = g761_get_config(client, G761_FAN_CMD1_REG);
	dev_info(&client->dev, "%s - CMD1=0x%x\n", __func__, val);

	return sprintf(buf, "%d\n", val);
}

static ssize_t g761_fan_cmd1_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct g761_p *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int val;
	int ret;

	ret = sscanf(buf, "%d\n", &val);
	if (!ret)
		return -EINVAL;

	ret = g761_set_config(client, G761_FAN_CMD1_REG, val);
	if (ret) {
		dev_err(&client->dev, "%s - failed to set config, ret=%d\n",
			__func__, ret);
	}

	return count;
}

static ssize_t g761_fan_cmd2_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct g761_p *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int val = 0;

	val = g761_get_config(client, G761_FAN_CMD2_REG);
	dev_info(&client->dev, "%s - CMD2=0x%x\n", __func__, val);

	return sprintf(buf, "%d\n", val);
}

static ssize_t g761_fan_cmd2_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct g761_p *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int val;
	int ret;

	ret = sscanf(buf, "%d\n", &val);
	if (!ret)
		return -EINVAL;

	ret = g761_set_config(client, G761_FAN_CMD2_REG, val);
	if (ret) {
		dev_err(&client->dev, "%s - failed to set config, ret=%d\n",
			__func__, ret);
	}

	return count;
}

static ssize_t g761_fan_act_cnt_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct g761_p *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int val = 0;

	val = g761_get_config(client, G761_FAN_ACT_CNT_REG);
	dev_info(&client->dev, "%s - act_cnt=0x%x\n", __func__, val);

	return sprintf(buf, "%d\n", val);
}

static ssize_t g761_fan_set_out_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct g761_p *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int val = 0;

	val = g761_get_config(client, G761_FAN_SET_OUT_REG);
	dev_info(&client->dev, "%s - set_out=0x%x\n", __func__, val);

	return sprintf(buf, "%d\n", val);
}

static ssize_t g761_fan_set_out_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct g761_p *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int val;
	int ret;

	ret = sscanf(buf, "%d\n", &val);
	if (!ret)
		return -EINVAL;

	ret = g761_set_config(client, G761_FAN_SET_OUT_REG, val);
	if (ret) {
		dev_err(&client->dev, "%s - failed to set config, ret=%d\n",
			__func__, ret);
	}

	return count;
}

static DEVICE_ATTR(fan_rpm, 0664, g761_fan_rpm_show, g761_fan_rpm_store);
static DEVICE_ATTR(fan_cmd1, 0664, g761_fan_cmd1_show, g761_fan_cmd1_store);
static DEVICE_ATTR(fan_cmd2, 0664, g761_fan_cmd2_show, g761_fan_cmd2_store);
static DEVICE_ATTR(fan_act_cnt, 0444, g761_fan_act_cnt_show, NULL);
static DEVICE_ATTR(fan_set_out, 0664, g761_fan_set_out_show, g761_fan_set_out_store);

static struct attribute *g761_attributes[] = {
	&dev_attr_fan_rpm.attr,
	&dev_attr_fan_cmd1.attr,
	&dev_attr_fan_cmd2.attr,
	&dev_attr_fan_act_cnt.attr,
	&dev_attr_fan_set_out.attr,
	NULL
};

static struct attribute_group g761_attribute_group = {
	.attrs = g761_attributes
};

static int g761_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct g761_p *data = NULL;
	struct device_node *np = of_find_node_by_name(NULL, "g761");
	int ret = -ENODEV, val = 0;

	dev_info(&client->dev, "%s - start\n", __func__);
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "%s - i2c_check_functionality error\n",
			__func__);
		ret = -EIO;
		goto exit;
	}

	/* create memory for main struct */
	data = kzalloc(sizeof(struct g761_p), GFP_KERNEL);
	if (!data) {
		ret = -ENOMEM;
		goto err_alloc_data;
	}

	i2c_set_clientdata(client, data);
	data->client = client;
	data->fan_rpm = 0;

	data->fan_boost_gpio = of_get_named_gpio(np, "g761,fan_boost_en_gpio", 0);
	if (!gpio_is_valid(data->fan_boost_gpio)) {
		dev_err(&client->dev, "%s - failed to get fan_boost_gpio\n", __func__);
		goto err_setup_config;
	}
	gpio_request(data->fan_boost_gpio, NULL);
	dev_info(&client->dev, "%s - fan_boost_gpio=%d\n",
		__func__, gpio_get_value(data->fan_boost_gpio));

	ret = of_property_read_u32(np, "g761,fan_cmd1", &val);
	if (ret) {
		dev_err(&client->dev, "no fan_cmd1 property set\n");
		val = 0x30; /* default PWM mode and Close-loop */
	}
	data->fan_cmd1 = val;
	data->mode = (val & (0x1 << 5)) ? FAN_PWM_MODE : FAN_LINEAR_MODE;

	ret = of_property_read_u32(np, "g761,fan_cmd2", &val);
	if (ret) {
		dev_err(&client->dev, "no fan_cmd2 property set\n");
		val = 0x21; /* default Internal 32kHz clock and 32 dac_code */
	}
	data->fan_cmd2 = val;

	ret = sysfs_create_group(&data->client->dev.kobj,	&g761_attribute_group);
	if (ret < 0) {
		dev_err(&client->dev, "%s - failed to create sysfs group, ret=%d\n",
			__func__, ret);
		goto err_create_sysfs_group;
	}

	dev_info(&client->dev, "%s - done\n", __func__);

	return 0;

err_create_sysfs_group:
	sysfs_remove_group(&data->client->dev.kobj, &g761_attribute_group);
err_setup_config:
	gpio_free(data->fan_boost_gpio);
err_alloc_data:
	kfree(data);
exit:
	dev_err(&client->dev, "%s - fail\n", __func__);
	return ret;
}

static int g761_remove(struct i2c_client *client)
{
	struct g761_p *data = (struct g761_p *)i2c_get_clientdata(client);

	dev_info(&client->dev, "%s\n", __func__);
	sysfs_remove_group(&client->dev.kobj, &g761_attribute_group);
	kfree(data);

	return 0;
}

#ifdef TEST_G761
static int g761_suspend(struct device *dev)
{
	struct g761_p *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;

	dev_info(&client->dev, "%s\n", __func__);
	return 0;
}

static int g761_resume(struct device *dev)
{
	struct g761_p *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;

	dev_info(&client->dev, "%s\n", __func__);
	return 0;
}
#endif

static void g761_shutdown(struct i2c_client *client)
{
	struct g761_p *data = (struct g761_p *)i2c_get_clientdata(client);

	gpio_direction_output(data->fan_boost_gpio, 0);
	msleep(20);
	gpio_free(data->fan_boost_gpio);

	dev_info(&client->dev, "%s\n", __func__);
}

static const struct of_device_id g761_match_table[] = {
	{ .compatible = "gmt,g761",},
	{},
};

static const struct i2c_device_id g761_id[] = {
	{ "g761_match_table", 0 },
	{ }
};
#ifdef TEST_G761
static const struct dev_pm_ops g761_pm_ops = {
	.suspend = g761_suspend,
	.resume = g761_resume,
};
#endif
static struct i2c_driver g761_driver = {
	.driver = {
		.name	= MODEL_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = g761_match_table,
#ifdef TEST_G761
		.pm = &g761_pm_ops
#endif
	},
	.probe		= g761_probe,
	.remove		= g761_remove,
	.shutdown	= g761_shutdown,
	.id_table	= g761_id,
};

static int __init g761_init(void)
{
	return i2c_add_driver(&g761_driver);
}

static void __exit g761_exit(void)
{
	i2c_del_driver(&g761_driver);
}

late_initcall(g761_init);
module_exit(g761_exit);

MODULE_DESCRIPTION("GMT Corp. G761 Fan Controller Driver");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");
