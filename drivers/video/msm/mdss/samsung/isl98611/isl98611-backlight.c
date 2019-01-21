/*
 * isl98611-backlight.c - Platform data for isl98611 backlight driver
 *
 * Copyright (C) 2011 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include "isl98611-backlight.h"

struct isl98611_backlight_info *pinfo;

int backlight_i2c_write(struct i2c_client *client,
		u8 reg,  u8 val, unsigned int len)
{
	int err = 0;
	int retry = 3;
	u8 temp_val = val;

	while (retry--) {
		err = i2c_smbus_write_i2c_block_data(client,
				reg, len, &temp_val);
		if (err >= 0)
			return err;

		LCD_ERR("i2c transfer error. %d\n", err);
		usleep_range(1000,1000);
	}
	return err;
}

int backlight_i2c_read(struct i2c_client *client,
		u8 reg,  u8 *val, unsigned int len)
{
	int err = 0;
	int retry = 3;

	while (retry--) {
		err = i2c_smbus_read_i2c_block_data(client,
				reg, len, val);
		if (err >= 0)
			return err;

		LCD_ERR("i2c transfer error. %d\n", err);
		usleep_range(1000,1000);
	}
	return err;
}

void debug_blic(struct blic_message *message)
{
	int init_loop;
	struct isl98611_backlight_info *info = pinfo;
	char address, value;

	for (init_loop = 0; init_loop < message->size; init_loop++) {
		address = message->address[init_loop];

		backlight_i2c_read(info->client, address, &value, 1);

		LCD_ERR("BLIC address : 0x%x  value : 0x%x\n", address, value);
	}
}

void backlight_request_gpio(struct isl98611_backlight_platform_data *pdata)
{
	int ret;

	if (gpio_is_valid(pdata->gpio_en)) {
		ret = gpio_request(pdata->gpio_en, "gpio_en");
		if (ret) {
			LCD_ERR("unable to request gpio_en [%d]\n", pdata->gpio_en);
			return;
		}
	}

	if (gpio_is_valid(pdata->gpio_enn)) {
		ret = gpio_request(pdata->gpio_enn, "gpio_enn");
		if (ret) {
			LCD_ERR("unable to request gpio_enn [%d]\n", pdata->gpio_enn);
			return;
		}
	}

	if (gpio_is_valid(pdata->gpio_enp)) {
		ret = gpio_request(pdata->gpio_enp, "gpio_enp");
		if (ret) {
			LCD_ERR("unable to request gpio_enp [%d]\n", pdata->gpio_enp);
			return;
		}
	}
}

int isl98611_backlight_data_parse(struct device *dev,
			struct blic_message *input, char *keystring)
{
	struct device_node *np = dev->of_node;
	const char *data;
	int  data_offset, len = 0 , i = 0;

	data = of_get_property(np, keystring, &len);
	if (!data) {
		LCD_ERR("%s:%d, Unable to read table %s ", __func__, __LINE__, keystring);
		return -EINVAL;
	}

	if ((len % 2) != 0) {
		LCD_ERR("%s:%d, Incorrect table entries for %s",
					__func__, __LINE__, keystring);
		return -EINVAL;
	}

	input->size = len / 2;

	input->address = kzalloc(input->size, GFP_KERNEL);
	if (!input->address)
		return -ENOMEM;

	input->data = kzalloc(input->size, GFP_KERNEL);
	if (!input->data)
		goto error;

	data_offset = 0;
	for (i = 0 ; i < input->size; i++) {
		input->address[i] = data[data_offset++];
		input->data[i] = data[data_offset++];

		LCD_ERR("BLIC address : 0x%x  value : 0x%x\n", input->address[i], input->data[i]);
	}

	return 0;
error:
	input->size = 0;
	kfree(input->address);

	return -ENOMEM;
}

int isl98611_backlight_parse_dt(struct device *dev,
			struct isl98611_backlight_platform_data *pdata)
{
	struct device_node *np = dev->of_node;

	/* enable, en, enp */
	pdata->gpio_en = of_get_named_gpio_flags(np, "isl98611_en_gpio",
				0, &pdata->gpio_en_flags);
	pdata->gpio_enp = of_get_named_gpio_flags(np, "isl98611_enp_gpio",
				0, &pdata->gpio_enp_flags);
	pdata->gpio_enn = of_get_named_gpio_flags(np, "isl98611_enn_gpio",
				0, &pdata->gpio_enn_flags);

	LCD_ERR("gpio_en : %d , gpio_enp : %d gpio_enn : %d\n",
		pdata->gpio_en, pdata->gpio_enp, pdata->gpio_enn);

	return 0;
}

int isl98611_backlight_pwm_power(struct mdss_dsi_ctrl_pdata *ctrl, int enable)
{
	struct isl98611_backlight_info *info = pinfo;


	if (!info) {
		LCD_ERR("error pinfo\n");
		return 0;
	}

	/* For PBA BOOTING */
	if (!mdss_panel_attached(ctrl->ndx))
		enable = 0;	

	if (enable)
		gpio_set_value(info->pdata->gpio_en, 1);
	else
		gpio_set_value(info->pdata->gpio_en, 0);

	LCD_INFO("enable : %d \n", enable);

	return 0;

}

int isl98611_backlight_power(struct mdss_dsi_ctrl_pdata *ctrl, int enable)
{
	struct isl98611_backlight_info *info = pinfo;
	char address, data;
	int init_loop;
	int debug = 0;

	if (!info) {
		LCD_ERR("error pinfo\n");
		return 0;
	}

	LCD_INFO("enable : %d \n", enable);

	if (enable) {
		for (init_loop = 0; init_loop < info->pdata->init_data.size; init_loop++) {
			address = info->pdata->init_data.address[init_loop];
			data = info->pdata->init_data.data[init_loop];
			backlight_i2c_write(info->client, address, data, 1);
		}

		if (debug)
			debug_blic(&info->pdata->init_data);

		/* gpio_en is enabled after DISPAY_ON */
		gpio_set_value(info->pdata->gpio_enp, 1);
		gpio_set_value(info->pdata->gpio_enn, 1);
	} else {
		gpio_set_value(info->pdata->gpio_enn, 0);
		gpio_set_value(info->pdata->gpio_enp, 0);
		usleep_range(3000,3000);
	}

	return 0;
}

static int isl98611_backlight_probe(struct i2c_client *client,
				  const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct isl98611_backlight_platform_data *pdata;
	struct isl98611_backlight_info *info;

	int error = 0;

	LCD_INFO("\n");

	if (!i2c_check_functionality(adapter, I2C_FUNC_I2C))
		return -EIO;

	pdata = devm_kzalloc(&client->dev,
		sizeof(struct isl98611_backlight_platform_data),
			GFP_KERNEL);
	if (!pdata) {
		LCD_INFO("Failed to allocate memory\n");
		return -ENOMEM;
	}

	pinfo = info = kzalloc(sizeof(*info), GFP_KERNEL);
	if (!info) {
		LCD_INFO("Failed to allocate memory\n");
		return -ENOMEM;
	}

	info->client = client;
	info->pdata = pdata;

	if (client->dev.of_node) {
		isl98611_backlight_parse_dt(&client->dev, pdata);
		backlight_request_gpio(pdata);

		isl98611_backlight_data_parse(&client->dev, &info->pdata->init_data, "blic_init_data");
		//isl98611_backlight_data_parse(&client->dev, &info->pdata->enable_data, "blic_enable_data");
	}

	i2c_set_clientdata(client, info);

	return error;
}

static int isl98611_backlight_remove(struct i2c_client *client)
{
	return 0;
}

static const struct i2c_device_id isl98611_backlight_id[] = {
	{"isl98611_backlight", 0},
};
MODULE_DEVICE_TABLE(i2c, isl98611_backlight_id);

static struct of_device_id isl98611_backlight_match_table[] = {
	{ .compatible = "isl98611,backlight-control",},
};

MODULE_DEVICE_TABLE(of, isl98611_backlight_id);

struct i2c_driver isl98611_backlight_driver = {
	.probe = isl98611_backlight_probe,
	.remove = isl98611_backlight_remove,
	.driver = {
		.name = "isl98611_backlight",
		.owner = THIS_MODULE,
		.of_match_table = isl98611_backlight_match_table,
		   },
	.id_table = isl98611_backlight_id,
};

static int __init isl98611_backlight_init(void)
{

	int ret = 0;

	ret = i2c_add_driver(&isl98611_backlight_driver);
	if (ret) {
		printk(KERN_ERR "isl98611_backlight_init registration failed. ret= %d\n",
			ret);
	}

	return ret;
}

static void __exit isl98611_backlight_exit(void)
{
	i2c_del_driver(&isl98611_backlight_driver);
}

module_init(isl98611_backlight_init);
module_exit(isl98611_backlight_exit);

MODULE_DESCRIPTION("isl98611 backlight driver");
MODULE_LICENSE("GPL");
