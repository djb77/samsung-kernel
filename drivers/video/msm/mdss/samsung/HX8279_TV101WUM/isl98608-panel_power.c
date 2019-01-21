/*
 * isl98608-panel_power.c - Platform data for isl98608 panel_power driver
 *
 * Copyright (C) 2011 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#include <linux/kernel.h>
#include <asm/unaligned.h>
#include <linux/input/mt.h>
#include <linux/of_gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/miscdevice.h>
#include <linux/regulator/consumer.h>
#include <linux/device.h>
#include <linux/of_gpio.h>

#include "../ss_dsi_panel_common.h"


struct isl98608_panel_power_platform_data {
	int gpio_en;
	u32 gpio_en_flags;

	int gpio_enn;
	u32 gpio_enn_flags;

	int gpio_enp;
	u32 gpio_enp_flags;
};

struct isl98608_panel_power_info {
	struct i2c_client			*client;
	struct isl98608_panel_power_platform_data	*pdata;
};

static struct isl98608_panel_power_info *pinfo;

static int panel_power_i2c_write(struct i2c_client *client,
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
	}
	return err;
}

static void panel_power_request_gpio(struct isl98608_panel_power_platform_data *pdata)
{
	int ret;

	if (gpio_is_valid(pdata->gpio_en)) {
		ret = gpio_request(pdata->gpio_en, "gpio_en");
		if (ret) {
			LCD_ERR("unable to request gpio_en [%d]\n", pdata->gpio_en);
			return;
		}
	}
}

static int isl98608_panel_power_parse_dt(struct device *dev,
			struct isl98608_panel_power_platform_data *pdata)
{
	struct device_node *np = dev->of_node;

	/* enable, en, enp */
	pdata->gpio_en = of_get_named_gpio_flags(np, "isl98608_en_gpio",
				0, &pdata->gpio_en_flags);

	LCD_ERR("gpio_en : %d\n",
		pdata->gpio_en);
	return 0;
}

int isl98608_panel_power(struct mdss_dsi_ctrl_pdata *ctrl, int enable)
{
	struct isl98608_panel_power_info *info = pinfo;

	if (!info) {
		LCD_ERR("error pinfo\n");
		return 0;
	}

	if (enable) {
		usleep_range(5000, 5000);
		panel_power_i2c_write(info->client, 0x06, 0x08, 1); /* VBST*/
		panel_power_i2c_write(info->client, 0x08, 0x08, 1); /* VN 5.8V*/
		panel_power_i2c_write(info->client, 0x09, 0x08, 1); /* VP 5.8V*/

		gpio_set_value(info->pdata->gpio_en, 1);
		usleep_range(3000, 3000);

	} else {
		usleep_range(3000, 3000);
		gpio_set_value(info->pdata->gpio_en, 0);
		usleep_range(5000, 5000);
	}
	LCD_INFO(" enable : %d \n", enable);

	return 0;
}

static int isl98608_panel_power_probe(struct i2c_client *client,
				  const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct isl98608_panel_power_platform_data *pdata;
	struct isl98608_panel_power_info *info;

	int error = 0;

	LCD_INFO("\n");

	if (!i2c_check_functionality(adapter, I2C_FUNC_I2C))
		return -EIO;

	if (client->dev.of_node) {
		pdata = devm_kzalloc(&client->dev,
			sizeof(struct isl98608_panel_power_platform_data),
				GFP_KERNEL);
		if (!pdata) {
			LCD_INFO("Failed to allocate memory\n");
			return -ENOMEM;
		}

		error = isl98608_panel_power_parse_dt(&client->dev, pdata);
		if (error)
			return error;
	} else
		pdata = client->dev.platform_data;

	panel_power_request_gpio(pdata);

	pinfo = info = kzalloc(sizeof(*info), GFP_KERNEL);
	if (!info) {
		LCD_INFO("Failed to allocate memory\n");
		return -ENOMEM;
	}

	info->client = client;
	info->pdata = pdata;

	i2c_set_clientdata(client, info);

	return error;
}

static int isl98608_panel_power_remove(struct i2c_client *client)
{
	return 0;
}

static const struct i2c_device_id isl98608_panel_power_id[] = {
	{"isl98608_panel_power", 0},
};
MODULE_DEVICE_TABLE(i2c, isl98608_panel_power_id);

static struct of_device_id isl98608_panel_power_match_table[] = {
	{ .compatible = "isl98608,panel_power-control",},
};

MODULE_DEVICE_TABLE(of, isl98608_panel_power_id);

struct i2c_driver isl98608_panel_power_driver = {
	.probe = isl98608_panel_power_probe,
	.remove = isl98608_panel_power_remove,
	.driver = {
		.name = "isl98608_panel_power",
		.owner = THIS_MODULE,
		.of_match_table = isl98608_panel_power_match_table,
		   },
	.id_table = isl98608_panel_power_id,
};

static int __init isl98608_panel_power_init(void)
{

	int ret = 0;

	ret = i2c_add_driver(&isl98608_panel_power_driver);
	if (ret) {
		LCD_ERR("isl98608_panel_power_init registration failed. ret= %d\n",
			ret);
	}

	return ret;
}

static void __exit isl98608_panel_power_exit(void)
{
	i2c_del_driver(&isl98608_panel_power_driver);
}

module_init(isl98608_panel_power_init);
module_exit(isl98608_panel_power_exit);

MODULE_DESCRIPTION("isl98608 panel_power driver");
MODULE_LICENSE("GPL");
