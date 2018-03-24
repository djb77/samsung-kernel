/*
 * Copyright (c) 2014 Samsung Electronics Co. Ltd.
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#define DEBUG
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/pm_runtime.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/jack.h>
#include <sound/initval.h>
#include <sound/tlv.h>
#include <linux/i2c.h>
#include <linux/regulator/consumer.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/completion.h>

#include "cod3033x-regctl_jack.h"


static int cod3033_read_reg(struct i2c_client *client, u8 reg, u8 *dest)
{
	struct cod3033x_jack_priv *cod3033 = i2c_get_clientdata(client);
	int ret = 0;


	mutex_lock(&cod3033->io_lock);
	ret = i2c_smbus_read_byte_data(client, reg);
	mutex_unlock(&cod3033->io_lock);

	if (ret < 0) {
		pr_err("%s: can't read reg(0x%x), ret(%d)\n", __func__, reg, ret);
		return ret;
	}

	reg &= 0xFF;
	*dest = ret;

	return 0;
}
static int cod3033_write_reg(struct i2c_client *client, u8 reg, u8 data)
{
	struct cod3033x_jack_priv *cod3033 = i2c_get_clientdata(client);
	int ret = 0;

	mutex_lock(&cod3033->io_lock);
	ret = i2c_smbus_write_byte_data(client, reg, data);
	mutex_unlock(&cod3033->io_lock);

	if (ret < 0)
		pr_err("%s: can't write reg(0x%x), ret(%d)\n", __func__, reg, ret);

	return ret;
}

static inline void cod3033x_usleep(unsigned int u_sec)
{
	usleep_range(u_sec, u_sec + 10);
}

static void cod3033_power_init(struct cod3033x_jack_priv *cod3033)
{
	u8 value = 0;


	
	 cod3033_write_reg(cod3033->i2c , 0x10, 0x05);

	 cod3033_write_reg(cod3033->i2c , 0x3b, 0x02);


	 cod3033_read_reg(cod3033->i2c, 0x30, &value);
}

static int cod3033x_regctl_jack_i2c_probe(struct i2c_client *i2c,
		const struct i2c_device_id *id)
{
	struct cod3033x_jack_priv *cod3033x;
	int ret=0;

	dev_err(&i2c->dev, "%s register control driver probe\n", __func__);

	cod3033x = kzalloc(sizeof(struct cod3033x_jack_priv), GFP_KERNEL);
	if (cod3033x == NULL)
		return -ENOMEM;
	cod3033x->dev = &i2c->dev;
	cod3033x->i2c_addr = i2c->addr;

	dev_err(&i2c->dev, "%s register control driver probe\n", __func__);
	cod3033x->i2c = i2c;
	mutex_init(&cod3033x->io_lock);

	dev_err(&i2c->dev, "%s register control driver probe\n", __func__);
	i2c_set_clientdata(i2c, cod3033x);

	dev_err(&i2c->dev, "%s register control driver probe\n", __func__);
	dev_err(&i2c->dev, "%s register control driver probe\n", __func__);
	dev_err(&i2c->dev, "%s register control driver probe\n", __func__);
	dev_err(&i2c->dev, "%s register control driver probe\n", __func__);
	dev_err(&i2c->dev, "%s register control driver probe\n", __func__);
	cod3033_power_init(cod3033x);
	return ret;
}

static int cod3033x_regctl_jack_i2c_remove(struct i2c_client *i2c)
{
	snd_soc_unregister_codec(&i2c->dev);
	return 0;
}


static const struct i2c_device_id cod3033x_regctl_jack_i2c_id[] = {
	{ "cod3033x-jack", 3030 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, cod3033x_regctl_jack_i2c_id);

const struct of_device_id cod3033x_regctl_jack_of_match[] = {
	{ .compatible = "codec,cod3033x-jack",},
	{},
};

static struct i2c_driver cod3033x_regctl_jack_i2c_driver = {
	.driver = {
		.name = "cod3033x-jack",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(cod3033x_regctl_jack_of_match),
	},
	.probe = cod3033x_regctl_jack_i2c_probe,
	.remove = cod3033x_regctl_jack_i2c_remove,
	.id_table = cod3033x_regctl_jack_i2c_id,
};

module_i2c_driver(cod3033x_regctl_jack_i2c_driver);

MODULE_DESCRIPTION("ASoC COD3033X driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:COD3033X-codec");
MODULE_FIRMWARE(COD3033X_FIRMWARE_NAME);
