/*
 * Copyright (C) 2017 Samsung Electronics.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/device.h>
#include <linux/sysfs.h>
#include <linux/module.h>
#include <linux/notifier.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_platform.h>

#include <linux/dev_ril_bridge.h>

#define LOG_TAG "drb: "

#define drb_err(fmt, ...) \
	pr_err(LOG_TAG "%s: " pr_fmt(fmt), __func__, ##__VA_ARGS__)

#define drb_debug(fmt, ...) \
	pr_debug(LOG_TAG "%s: " pr_fmt(fmt), __func__, ##__VA_ARGS__)

#define drb_info(fmt, ...) \
	pr_info(LOG_TAG "%s: " pr_fmt(fmt), __func__, ##__VA_ARGS__)

static RAW_NOTIFIER_HEAD(dev_ril_bridge_chain);

int register_dev_ril_bridge_event_notifier(struct notifier_block *nb)
{
	if (!nb)
		return -ENOENT;

	return raw_notifier_chain_register(&dev_ril_bridge_chain, nb);
}

static ssize_t notify_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	/* no need to prepare read function */
	return 0;
}

static ssize_t notify_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	/* buf head may be consist of some structures */

	drb_info("event notify (%ld) ++\n", size);
	raw_notifier_call_chain(&dev_ril_bridge_chain, size, (void *)buf);
	drb_info("event notify (%ld) --\n", size);

	return size;
}
static DEVICE_ATTR_RW(notify);

static struct attribute *dev_ril_bridge_attrs[] = {
	&dev_attr_notify.attr,
	NULL,
};
ATTRIBUTE_GROUPS(dev_ril_bridge);

static struct class *dev_ril_bridge_class;
static struct device *dev_ril_bridge_device;

static int dev_ril_bridge_probe(struct platform_device *pdev)
{
	int err = 0;

	drb_info("+++\n");

	/* node /sys/class/dev_ril_bridge/dev_ril_bridge/notify */

	dev_ril_bridge_class = class_create(THIS_MODULE, "dev_ril_bridge");
	if (IS_ERR(dev_ril_bridge_class)) {
		drb_err("couldn't register device class\n");
		err = PTR_ERR(dev_ril_bridge_class);
		goto out;
	}

	dev_ril_bridge_device = device_create_with_groups(dev_ril_bridge_class,
			NULL, 0, MKDEV(0, 0), dev_ril_bridge_groups, "%s",
			"dev_ril_bridge");
	if (IS_ERR(dev_ril_bridge_device)) {
		drb_err("couldn't register system device\n");
		err = PTR_ERR(dev_ril_bridge_device);
		goto out_class;
	}

	drb_info("---\n");

	return 0;

out_class:
	class_destroy(dev_ril_bridge_class);
out:
	drb_info("err = %d ---\n", err);
	return err;
}

static void dev_ril_bridge_shutdown(struct platform_device *pdev)
{
	device_unregister(dev_ril_bridge_device);
	class_destroy(dev_ril_bridge_class);
}

#ifdef CONFIG_PM
static int dev_ril_bridge_suspend(struct device *dev)
{
	return 0;
}

static int dev_ril_bridge_resume(struct device *dev)
{
	return 0;
}
#else
#define dev_ril_bridge_suspend NULL
#define dev_ril_bridge_resume NULL
#endif

static const struct dev_pm_ops dev_ril_bridge_pm_ops = {
	.suspend = dev_ril_bridge_suspend,
	.resume = dev_ril_bridge_resume,
};

static const struct of_device_id dev_ril_bridge_match[] = {
	{ .compatible = "samsung,dev_ril_bridge_pdata", },
	{},
};
MODULE_DEVICE_TABLE(of, dev_ril_bridge_match);

static struct platform_driver dev_ril_bridge_driver = {
	.probe = dev_ril_bridge_probe,
	.shutdown = dev_ril_bridge_shutdown,
	.driver = {
		.name = "dev_ril_bridge",
		.owner = THIS_MODULE,
		.suppress_bind_attrs = true,
		.pm = &dev_ril_bridge_pm_ops,
#ifdef CONFIG_OF
		.of_match_table = of_match_ptr(dev_ril_bridge_match),
#endif
	},
};
module_platform_driver(dev_ril_bridge_driver);

MODULE_DESCRIPTION("dev_ril_bridge driver");
MODULE_LICENSE("GPL");
