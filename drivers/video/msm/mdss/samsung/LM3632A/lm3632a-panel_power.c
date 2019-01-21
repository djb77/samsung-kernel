/*
 * lm3632a-panel_power.c - Platform data for lm3632a panel_power driver
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

struct blic_message {
	char *address;
	char *data;
	int size;
};

struct lm3632a_panel_power_platform_data {
	int gpio_en;
	u32 gpio_en_flags;

	int gpio_enn;
	u32 gpio_enn_flags;

	int gpio_enp;
	u32 gpio_enp_flags;

	struct blic_message init_data;
};

struct lm3632a_panel_power_info {
	struct i2c_client			*client;
	struct lm3632a_panel_power_platform_data	*pdata;
};

static struct lm3632a_panel_power_info *pinfo;

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

static void panel_power_request_gpio(struct lm3632a_panel_power_platform_data *pdata)
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

static int lm3632a_panel_power_parse_dt(struct device *dev,
			struct lm3632a_panel_power_platform_data *pdata)
{
	struct device_node *parent = dev->of_node;
	struct device_node *node, *blic_data_node = NULL;
	const char *data;
	int  data_offset, size, len = 0 , i = 0;
	struct blic_message *input = &pdata->init_data;
	u32 *panel_ids = NULL;
	int panel_id_count,ret;
	

	/* enable, enn, enp */
	pdata->gpio_en = of_get_named_gpio_flags(parent, "lm3632a_en_gpio",
				0, &pdata->gpio_en_flags);
	pdata->gpio_enn = of_get_named_gpio_flags(parent, "lm3632a_enn_gpio",
				0, &pdata->gpio_enn_flags);
	pdata->gpio_enp = of_get_named_gpio_flags(parent, "lm3632a_enp_gpio",
				0, &pdata->gpio_enp_flags);

	LCD_ERR("gpio_en : %d gpio_enn: %d gpio_enp: %d\n",
		pdata->gpio_en,pdata->gpio_enn,pdata->gpio_enp);
	

	for_each_child_of_node(parent, node) {
		if (!of_find_property(node, "panel_id", &size)) {
				LCD_ERR("%s: Cannot find panel_id for this child\n ", __func__);
				continue;
			}

		panel_id_count = (size / sizeof(u32));			

		panel_ids = kzalloc(panel_id_count * sizeof(u32), GFP_KERNEL);
		if (!panel_ids)
			goto error;
		
		ret = of_property_read_u32_array(node,"panel_id", panel_ids, panel_id_count);
		if (unlikely(ret < 0)) {
			LCD_ERR("%s: failed to get panel_id\n",__func__);
			continue;
		}
		
		
		for (i = 0; i < panel_id_count; i++) {
			LCD_ERR("%s: lcd id = 0x%x\n ", __func__,panel_ids[i]);
			if (get_lcd_attached("GET") == panel_ids[i]) {
				blic_data_node = node;
				break;
			}
		}
		if (blic_data_node) {
			LCD_ERR("%s: %s node found successfully\n",__func__,blic_data_node->name);
			break;
		}
	}

	data = of_get_property(blic_data_node, "blic_init_data", &len);
	if (!data) {
		LCD_ERR("%s:%d, Unable to read table %s ", __func__, __LINE__, "blic_init_data");
		goto dtserr;
	}

	if ((len % 2) != 0) {
		LCD_ERR("%s:%d, Incorrect table entries for %s",
					__func__, __LINE__, "blic_init_data");
		goto dtserr;
	}

	input->size = len / 2;

	input->address = kzalloc(input->size, GFP_KERNEL);
	if (!input->address)
		goto error;

	input->data = kzalloc(input->size, GFP_KERNEL);
	if (!input->data)
		goto error;

	data_offset = 0;
	for (i = 0 ; i < input->size; i++) {
		input->address[i] = data[data_offset++];
		input->data[i] = data[data_offset++];

		LCD_ERR("BLIC address : 0x%x  value : 0x%x\n", input->address[i], input->data[i]);
	}
	
	kfree(panel_ids);
	return 0;

dtserr:
	kfree(panel_ids);
	return -EINVAL;

error:
	input->size = 0;
	kfree(panel_ids);
	kfree(input->address);
	kfree(input->data);

	return -ENOMEM;
}

int lm3632a_panel_power(struct mdss_dsi_ctrl_pdata *ctrl, int enable)
{
	struct lm3632a_panel_power_info *info = pinfo;
	char address, data;
	int init_loop;

	if (!info) {
		LCD_ERR("error pinfo\n");
		return 0;
	}

	if (enable) {
		gpio_set_value(info->pdata->gpio_en, 1);
		msleep(5);
		
		for (init_loop = 0; init_loop < info->pdata->init_data.size; init_loop++) {
			address = info->pdata->init_data.address[init_loop];
			data = info->pdata->init_data.data[init_loop];
			panel_power_i2c_write(info->client, address, data, 1);
		}

		gpio_set_value(info->pdata->gpio_enp, 1);
		msleep(5);
		gpio_set_value(info->pdata->gpio_enn, 1);
	} else {
		gpio_set_value(info->pdata->gpio_enn, 0);
		msleep(5);
		gpio_set_value(info->pdata->gpio_enp, 0);
		gpio_set_value(info->pdata->gpio_en, 0);
		msleep(5);
	}
	LCD_INFO(" enable : %d \n", enable);

	return 0;
}

static int lm3632a_panel_power_probe(struct i2c_client *client,
				  const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct lm3632a_panel_power_platform_data *pdata;
	struct lm3632a_panel_power_info *info;
	int error = 0;

	LCD_INFO("\n");

	if (!i2c_check_functionality(adapter, I2C_FUNC_I2C))
		return -EIO;

	if (client->dev.of_node) {
		pdata = devm_kzalloc(&client->dev,
			sizeof(struct lm3632a_panel_power_platform_data),
				GFP_KERNEL);
		if (!pdata) {
			LCD_INFO("Failed to allocate memory\n");
			return -ENOMEM;
		}

		error = lm3632a_panel_power_parse_dt(&client->dev, pdata);
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

static int lm3632a_panel_power_remove(struct i2c_client *client)
{
	return 0;
}

static const struct i2c_device_id lm3632a_panel_power_id[] = {
	{"lm3632a_panel_power", 0},
};
MODULE_DEVICE_TABLE(i2c, lm3632a_panel_power_id);

static struct of_device_id lm3632a_panel_power_match_table[] = {
	{ .compatible = "lm3632a,panel_power-control",},
};

MODULE_DEVICE_TABLE(of, lm3632a_panel_power_id);

struct i2c_driver lm3632a_panel_power_driver = {
	.probe = lm3632a_panel_power_probe,
	.remove = lm3632a_panel_power_remove,
	.driver = {
		.name = "lm3632a_panel_power",
		.owner = THIS_MODULE,
		.of_match_table = lm3632a_panel_power_match_table,
		   },
	.id_table = lm3632a_panel_power_id,
};

static int __init lm3632a_panel_power_init(void)
{

	int ret = 0;

	ret = i2c_add_driver(&lm3632a_panel_power_driver);
	if (ret) {
		LCD_ERR("lm3632a_panel_power_init registration failed. ret= %d\n",
			ret);
	}

	return ret;
}

static void __exit lm3632a_panel_power_exit(void)
{
	i2c_del_driver(&lm3632a_panel_power_driver);
}

module_init(lm3632a_panel_power_init);
module_exit(lm3632a_panel_power_exit);

MODULE_DESCRIPTION("lm3632a panel_power driver");
MODULE_LICENSE("GPL");
