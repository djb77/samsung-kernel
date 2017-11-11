/*
 * linux/drivers/video/fbdev/exynos/panel/spi.c
 *
 * Samsung Common LCD Driver.
 *
 * Copyright (c) 2016 Samsung Electronics
 *
 * JiHoon Kim <jihoonn.kim@samsung.com>
 * Gwanghui Lee <gwanghui.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/spi/spi.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/workqueue.h>

#if defined (CONFIG_OF)
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#endif /* CONFIG_OF */

#define DRIVER_NAME "panel_spi"
static struct spi_device *panel_spi;
static DEFINE_MUTEX(panel_spi_lock);
int panel_spi_write_data(struct spi_device *spi, const u8 *cmd, int size)
{
	int i, status;
	u16 tx_buf[256];
	struct spi_message msg;
	struct spi_transfer tx = { .len = 0, .tx_buf = tx_buf, };
	bool dc;

	if (unlikely(!spi || !cmd)) {
		pr_err("%s, invalid parameter\n", __func__);
		return -EINVAL;
	}

	pr_debug("%s, addr 0x%02X len %d\n", __func__, cmd[0], size);

	if (size > ARRAY_SIZE(tx_buf)) {
		pr_err("%s, write length(%d) should be less than %ld\n",
				__func__, size, ARRAY_SIZE(tx_buf));
		return -EINVAL;
	}

	for (i = 0; i < size; i++) {
		dc = (i == 0) ? 0 : 1;
		tx_buf[i] = (dc << 8) | cmd[i];
	}
	tx.len = size * 2;

	spi_message_init(&msg);
	spi_message_add_tail(&tx, &msg);
	status = spi_sync(spi, &msg);
	if (status < 0) {
		pr_err("%s, failed to spi_sync %d\n", __func__, status);
		return status;
	}

	return 0;
}

int panel_spi_read_data(struct spi_device *spi, u8 addr, u8 *buf, int size)
{
	int i, status;
	u16 cmd_addr;
	u16 rx_buf[256];
	struct spi_message msg;
	struct spi_transfer tx = { .len = 2, .tx_buf = (u8 *)&cmd_addr, };
	struct spi_transfer rx = { .len = 0, .rx_buf = rx_buf, };

	if (unlikely(!spi || !buf)) {
		pr_err("%s, invalid parameter\n", __func__);
		return -EINVAL;
	}

	pr_debug("%s, addr 0x%02X len %d\n", __func__, addr, size);

	if (size > ARRAY_SIZE(rx_buf)) {
		pr_err("%s, read length(%d) should be less than %ld\n",
				__func__, size, ARRAY_SIZE(rx_buf));
		return -EINVAL;
	}

	cmd_addr = (0x0 << 8) | addr;
	rx.len = size * 2;

	spi_message_init(&msg);
	spi_message_add_tail(&tx, &msg);
	spi_message_add_tail(&rx, &msg);

	status = spi_sync(spi, &msg);
	if (status < 0) {
		pr_err("%s, failed to spi_sync %d\n", __func__, status);
		return status;
	}

	for (i = 0; i < size; i++) {
		buf[i] = rx_buf[i] & 0xFF;
		pr_debug("%s, rx_buf[%d] 0x%02X\n", __func__, i, buf[i]);
	}

	return size;
}

static int panel_spi_probe_dt(struct spi_device *spi)
{
	struct device_node *nc = spi->dev.of_node;
	int rc;
	u32 value;

	rc = of_property_read_u32(nc, "bits-per-word", &value);
	if (rc) {
		dev_err(&spi->dev, "%s has no valid 'bits-per-word' property (%d)\n",
				nc->full_name, rc);
		return rc;
	}
	spi->bits_per_word = value;

	rc = of_property_read_u32(nc, "spi-max-frequency", &value);
	if (rc) {
		dev_err(&spi->dev, "%s has no valid 'spi-max-frequency' property (%d)\n",
				nc->full_name, rc);
		return rc;
	}
	spi->max_speed_hz = value;

	return 0;
}

static int panel_spi_probe(struct spi_device *spi)
{
	int ret;

	if (unlikely(!spi)) {
		pr_err("%s, invalid spi\n", __func__);
		return -EINVAL;
	}

	ret = panel_spi_probe_dt(spi);
	if (ret < 0) {
		dev_err(&spi->dev, "%s, failed to parse device tree, ret %d\n",
				__func__, ret);
		return ret;
	}

	ret = spi_setup(spi);
	if (ret < 0) {
		dev_err(&spi->dev, "%s, failed to setup spi, ret %d\n", __func__, ret);
		return ret;
	}

	panel_spi = spi;

	return 0;
}

static int panel_spi_remove(struct spi_device *spi)
{
	return 0;
}

static const struct of_device_id panel_spi_match_table[] = {
	{   .compatible = "panel_spi",},
	{}
};

static struct spi_driver panel_spi_driver = {
	.driver = {
		.name		= DRIVER_NAME,
		.owner		= THIS_MODULE,
		.of_match_table = panel_spi_match_table,
	},
	.probe		= panel_spi_probe,
	.remove		= panel_spi_remove,
};

struct spi_device *of_find_panel_spi_by_node(struct device_node *node)
{
	//return (spi->dev.of_node == node) ? panel_spi : NULL;
	return panel_spi;
}
EXPORT_SYMBOL(of_find_panel_spi_by_node);

static int __init panel_spi_init(void)
{
	return spi_register_driver(&panel_spi_driver);
}
subsys_initcall(panel_spi_init);

static void __exit panel_spi_exit(void)
{
	spi_unregister_driver(&panel_spi_driver);
}
module_exit(panel_spi_exit);

MODULE_DESCRIPTION("spi driver for panel");
MODULE_LICENSE("GPL");
