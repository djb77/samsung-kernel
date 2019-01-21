/*
 * STMicroelectronics lsm6ds3 spi driver
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
#include <linux/spi/spi.h>
#include <linux/iio/iio.h>
#include <linux/delay.h>


#include "st_lsm6ds3.h"

#define ST_SENSORS_SPI_READ			0x80

static int st_lsm6ds3_spi_read(struct st_lsm6ds3_transfer_buffer *tb,
			struct device *dev, u8 reg_addr, int len, u8 *data)
{
	int err, retry = 3;

	struct spi_transfer xfers[] = {
		{
			.tx_buf = tb->tx_buf,
			.bits_per_word = 8,
			.len = 1,
		},
		{
			.rx_buf = tb->rx_buf,
			.bits_per_word = 8,
			.len = len,
		}
	};

	mutex_lock(&tb->buf_lock);
	tb->tx_buf[0] = reg_addr | ST_SENSORS_SPI_READ;

	err = spi_sync_transfer(to_spi_device(dev), xfers, ARRAY_SIZE(xfers));
	if (err) {
		SENSOR_INFO("err=%d\n",err);
		// Add workaround for delay in SPI wakeup
		while(err == -EINPROGRESS && retry) {	
			usleep_range(15000, 16000);
			err = spi_sync_transfer(to_spi_device(dev), xfers, ARRAY_SIZE(xfers));
			retry--;
		}

		if(err)
			goto acc_spi_read_error;
	}

	memcpy(data, tb->rx_buf, len*sizeof(u8));
	mutex_unlock(&tb->buf_lock);
	return len;

acc_spi_read_error:
	mutex_unlock(&tb->buf_lock);
	return err;
}

static int st_lsm6ds3_spi_write(struct st_lsm6ds3_transfer_buffer *tb,
			struct device *dev, u8 reg_addr, int len, u8 *data)
{
	int err;

	struct spi_transfer xfers = {
		.tx_buf = tb->tx_buf,
		.bits_per_word = 8,
		.len = len + 1,
	};

	if (len >= ST_LSM6DS3_RX_MAX_LENGTH)
		return -ENOMEM;

	mutex_lock(&tb->buf_lock);
	tb->tx_buf[0] = reg_addr;

	memcpy(&tb->tx_buf[1], data, len);

	err = spi_sync_transfer(to_spi_device(dev), &xfers, 1);
	mutex_unlock(&tb->buf_lock);

	return err;
}

static const struct st_lsm6ds3_transfer_function st_lsm6ds3_tf_spi = {
	.write = st_lsm6ds3_spi_write,
	.read = st_lsm6ds3_spi_read,
};

static int st_lsm6ds3_spi_probe(struct spi_device *spi)
{
	int err;
	struct lsm6ds3_data *cdata;

	cdata = kmalloc(sizeof(*cdata), GFP_KERNEL);
	if (!cdata)
		return -ENOMEM;

	cdata->dev = &spi->dev;
	cdata->name = spi->modalias;
	spi_set_drvdata(spi, cdata);

	cdata->tf = &st_lsm6ds3_tf_spi;

	err = st_lsm6ds3_common_probe(cdata, spi->irq);
	if (err < 0)
		goto free_data;

	return 0;

free_data:
	kfree(cdata);
	return err;
}

static int st_lsm6ds3_spi_remove(struct spi_device *spi)
{
	struct lsm6ds3_data *cdata = spi_get_drvdata(spi);

	st_lsm6ds3_common_remove(cdata, spi->irq);
	kfree(cdata);

	return 0;
}

static int st_lsm6ds3_suspend(struct device *dev)
{
	struct lsm6ds3_data *cdata = spi_get_drvdata(to_spi_device(dev));

	return st_lsm6ds3_common_suspend(cdata);
}

static int st_lsm6ds3_resume(struct device *dev)
{
	struct lsm6ds3_data *cdata = spi_get_drvdata(to_spi_device(dev));

	return st_lsm6ds3_common_resume(cdata);
}

static const struct dev_pm_ops st_lsm6ds3_pm_ops = {
	.suspend = st_lsm6ds3_suspend,
	.resume = st_lsm6ds3_resume
};

static const struct spi_device_id st_lsm6ds3_id_table[] = {
	{ "lsm6ds3" },
	{ },
};
MODULE_DEVICE_TABLE(spi, st_lsm6ds3_id_table);

static struct spi_driver st_lsm6ds3_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "st-lsm6ds3-spi",
		.pm = &st_lsm6ds3_pm_ops,
	},
	.probe = st_lsm6ds3_spi_probe,
	.remove = st_lsm6ds3_spi_remove,
	.id_table = st_lsm6ds3_id_table,
};
module_spi_driver(st_lsm6ds3_driver);

MODULE_DESCRIPTION("lsm6ds3 accelerometer sensor driver");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL v2");
