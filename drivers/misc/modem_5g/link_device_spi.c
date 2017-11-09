/*
 * Copyright (C) 2011 Samsung Electronics.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>

#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#ifdef CONFIG_OF
#include <linux/of_gpio.h>
#endif
#include <linux/wakelock.h>
#include <linux/delay.h>
#include <linux/spi/spi.h>
#ifdef CONFIG_LINK_DEVICE_LLI
#include <linux/mipi-lli.h>
#endif

#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/if_arp.h>
#include "modem_utils.h"

#include "modem_prj.h"
#include "modem_link_device_spi_boot.h"

#define to_spi_boot_link_dev(linkdev) \
		container_of(linkdev, struct spi_boot_link_device, ld)

#define SPI_XMIT_DELEY	100


struct spi_boot_link_device *spi_boot_id;

static int spi_boot_init(struct link_device *ld, struct io_device *iod)
{
	return 0;
}

static void spi_boot_terminate(struct link_device *ld, struct io_device *iod)
{
}

static int spi_transmit_data(struct spi_device *spi,
		u8 *buf, u32 len)
{
	int ret;

	struct spi_message msg;
	struct spi_transfer t = {
		.len = len,
		.tx_buf = buf,
	};

	spi_message_init(&msg);
	spi_message_add_tail(&t, &msg);

	ret = spi_sync(spi, &msg);
	if (ret < 0)
		mif_err("spi_sync() fail(%d)\n", ret);

	return ret;
}

static int spi_boot_tx_skb(struct link_device *ld, struct io_device *iod,
			struct sk_buff *skb)
{
	int ret = -EINVAL;

	struct spi_boot_link_device *sbld = to_spi_boot_link_dev(ld);

	unsigned char *buf = skb->data + SIPC5_MIN_HEADER_SIZE;
	unsigned int len = skb->len - SIPC5_MIN_HEADER_SIZE;

	skb_queue_tail(&sbld->tx_q, skb);

	ret = spi_transmit_data(sbld->spi, buf, len);
	if (ret < 0) {
		mif_err("spi_transmit_data() failed(%d)\n", ret);
		goto exit;
	}

exit:
	skb_unlink(skb, &sbld->tx_q);

	return ret;
}

static int spi_boot_send(struct link_device *ld, struct io_device *iod,
		struct sk_buff *skb)
{
	int ret = -EINVAL;

	if (iod->format != IPC_BOOT) {
		mif_err("iod->format: %d",
			iod->format);
		goto err;
	}

	ret = spi_boot_tx_skb(ld, iod, skb);
	if (ret < 0) {
		mif_err("spi_boot_tx_skb() failed(%d)\n", ret);
		goto err;
	}

	return skb->len;
err:

	dev_kfree_skb_any(skb);
	return 0;
}

static int spi_boot_write(struct link_device *ld, struct io_device *iod,
			 unsigned long arg)
{
	struct spi_boot_link_device *sbld = to_spi_boot_link_dev(ld);
	struct modem_firmware img;
	int ret = 0;
	char *buff = NULL;
	int len = 0;

	ret = copy_from_user(&img, (const void __user *)arg,
				sizeof(struct modem_firmware));

	if (ret) {
		mif_err("ERR! copy_from_user fail (err %d)\n", ret);
		return -EFAULT;
	}

	len = img.len;
	buff = kzalloc(len, GFP_KERNEL);
	if (!buff) {
		mif_err("ERR! kzalloc(%d) fail\n", len);
		return -ENOMEM;
	}

	memcpy(buff, (void*)img.binary, len);

#if 0
	if (len <= sizeof(u32)) {
		u32 *tmp32 = (u32 *)buff;
		mif_err("spi write value : %x\n", *tmp32);
	}
#endif

	ret = spi_transmit_data(sbld->spi, buff, len);
	if (ret < 0) {
		mif_err("spi_transmit_data() failed(%d)\n", ret);
#if 0
	} else {
		mif_err("spi write success(size:%d)\n", len);
#endif
	}

	if (buff)
		kfree(buff);

	return ret;
}

static int spi_boot_probe(struct spi_device *spi)
{
	int ret = -ENODEV;

	spi->bits_per_word = 32;
	spi->mode = SPI_MODE_0;

	mif_err("+++\n");

	ret = spi_setup(spi);
	if (ret < 0) {
		mif_err("spi_setup() fail(%d)\n", ret);
		return ret;
	}

	spi_boot_id->spi = spi;

	spi_set_drvdata(spi, spi_boot_id);

	mif_err("---\n");
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id modem_boot_spi_match[] = {
	{ .compatible = "spi_boot_link", },
	{},
};
MODULE_DEVICE_TABLE(of, modem_boot_spi_match);
#endif

static struct spi_driver spi_boot_driver = {
	.probe = spi_boot_probe,
	.driver = {
		.name = "spi_modem_boot",
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = of_match_ptr(modem_boot_spi_match),
#endif
	},
};

struct link_device *spi_create_link_device(struct platform_device *pdev)
{
	int ret;

	struct modem_data *pdata = pdev->dev.platform_data;
	struct link_device *ld;

	mif_err("%s: +++\n", pdev->name);

	if (!pdata) {
		mif_err("modem_data is null\n");
		return NULL;
	}

	spi_boot_id = kzalloc(sizeof(struct spi_boot_link_device), GFP_KERNEL);
	if (!spi_boot_id) {
		mif_err("allocation fail for spi_boot_id\n");
		return NULL;
	}

	INIT_LIST_HEAD(&spi_boot_id->ld.list);

	ld = &spi_boot_id->ld;

	ld->name = "spi_boot";
	ld->init_comm = spi_boot_init;
	ld->terminate_comm = spi_boot_terminate;
	ld->send = spi_boot_send;
	ld->xmit_boot = spi_boot_write;

	spi_boot_id->gpio_cp_status = pdata->gpio_phone_active;
	mif_err("<KTG> SPI phone active info:%d\n", spi_boot_id->gpio_cp_status);

	ret = spi_register_driver(&spi_boot_driver);
	if (ret) {
		mif_err("spi_register_driver() fail(%d)\n", ret);
		goto err;
	}

	skb_queue_head_init(&spi_boot_id->tx_q);

	mif_err("---\n");

	return ld;
err:
	kfree(spi_boot_id);

	return NULL;
}

