/*
 * STMicroelectronics lsm6ds3 i2c driver
 *
 * Copyright 2014 STMicroelectronics Inc.
 *
 * Denis Ciocca <denis.ciocca@st.com>
 *
 * Licensed under the GPL-2.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/iio/iio.h>

#include "st_lsm6ds3.h"

static int st_lsm6ds3_i2c_read(struct st_lsm6ds3_transfer_buffer *tb,
			struct device *dev, u8 reg_addr, int len, u8 *data)
{
	int ret = 0;
	struct i2c_msg msg[2];

	struct lsm6ds3_data *cdata = i2c_get_clientdata(to_i2c_client(dev));

	msg[0].addr = to_i2c_client(dev)->addr;
	msg[0].flags = to_i2c_client(dev)->flags;
	msg[0].len = 1;
	msg[0].buf = &reg_addr;

	msg[1].addr = to_i2c_client(dev)->addr;
	msg[1].flags = to_i2c_client(dev)->flags | I2C_M_RD;
	msg[1].len = len;
	msg[1].buf = data;

	if (cdata->err_count <= 3) {
		ret = i2c_transfer(to_i2c_client(dev)->adapter, msg, 2);

		if (ret < 0) {
			cdata->err_count++;
			SENSOR_ERR("i2c read: 0x%x, ret: %d\n", reg_addr, ret);
		} else {
			cdata->err_count = 0;
		}
	} else {
		SENSOR_ERR("skip i2c_read(): err_count %d\n", cdata->err_count);
		ret = -1;
	}

	return ret;
}

static int st_lsm6ds3_i2c_write(struct st_lsm6ds3_transfer_buffer *tb,
			struct device *dev, u8 reg_addr, int len, u8 *data)
{
	u8 send[len + 1];
	int ret;
	struct i2c_msg msg;

	send[0] = reg_addr;
	memcpy(&send[1], data, len * sizeof(u8));
	len++;

	msg.addr = to_i2c_client(dev)->addr;
	msg.flags = to_i2c_client(dev)->flags;
	msg.len = len;
	msg.buf = send;

	ret = i2c_transfer(to_i2c_client(dev)->adapter, &msg, 1);
	if (ret < 0)
		SENSOR_ERR("i2c read = 0x%x, ret = %d\n", reg_addr, ret);

	return ret;
}

static const struct st_lsm6ds3_transfer_function st_lsm6ds3_tf_i2c = {
	.write = st_lsm6ds3_i2c_write,
	.read = st_lsm6ds3_i2c_read,
};

static int st_lsm6ds3_i2c_probe(struct i2c_client *client,
						const struct i2c_device_id *id)
{
	int err;
	struct lsm6ds3_data *cdata;

	SENSOR_INFO("I2c Probe Start!\n");

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		err = -ENOSYS;
		SENSOR_ERR("I2c function error\n");
		goto out_no_free;
	}

	cdata = kzalloc(sizeof(*cdata), GFP_KERNEL);
	if (!cdata)
		return -ENOMEM;

	cdata->dev = &client->dev;
	cdata->name = client->name;
	cdata->client = client;
	i2c_set_clientdata(client, cdata);

	cdata->tf = &st_lsm6ds3_tf_i2c;

	err = st_lsm6ds3_common_probe(cdata, client->irq);
	if (err < 0)
		goto free_data;
	SENSOR_INFO("i2c Probe Done!\n");
	return 0;

free_data:
	kfree(cdata);
out_no_free:
	return err;
}

static int st_lsm6ds3_i2c_remove(struct i2c_client *client)
{
	struct lsm6ds3_data *cdata = i2c_get_clientdata(client);

	st_lsm6ds3_common_remove(cdata, client->irq);
	kfree(cdata);

	return 0;
}

static void st_lsm6ds3_i2c_shutdown(struct i2c_client *client)
{
	struct lsm6ds3_data *cdata = i2c_get_clientdata(client);

	st_lsm6ds3_common_shutdown(cdata);
}

static int st_lsm6ds3_suspend(struct device *dev)
{
	struct lsm6ds3_data *cdata = i2c_get_clientdata(to_i2c_client(dev));

	return st_lsm6ds3_common_suspend(cdata);
}

static int st_lsm6ds3_resume(struct device *dev)
{
	struct lsm6ds3_data *cdata = i2c_get_clientdata(to_i2c_client(dev));

	return st_lsm6ds3_common_resume(cdata);
}

static const struct dev_pm_ops st_lsm6ds3_pm_ops = {
	.suspend = st_lsm6ds3_suspend,
	.resume = st_lsm6ds3_resume
};

static const struct i2c_device_id st_lsm6ds3_id_table[] = {
	{ LSM6DS3_DEV_NAME },
	{ },
};

static struct of_device_id st_lsm6ds3_match_table[] = {
	{ .compatible = "st,lsm6ds3",},
	{ },
};

MODULE_DEVICE_TABLE(i2c, st_lsm6ds3_id_table);

static struct i2c_driver st_lsm6ds3_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "st-lsm6ds3-i2c",
		.pm = &st_lsm6ds3_pm_ops,
		.of_match_table = st_lsm6ds3_match_table,
	},
	.probe = st_lsm6ds3_i2c_probe,
	.remove = st_lsm6ds3_i2c_remove,
	.shutdown = st_lsm6ds3_i2c_shutdown,
	.id_table = st_lsm6ds3_id_table,
};
module_i2c_driver(st_lsm6ds3_driver);

MODULE_DESCRIPTION("lsm6ds3 accelerometer sensor driver");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL v2");
