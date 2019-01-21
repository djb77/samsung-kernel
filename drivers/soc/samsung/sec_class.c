/*
 * driver/misc/sec_sysfs.c
 *
 * COPYRIGHT(C) 2014-2016 Samsung Electronics Co., Ltd. All Right Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <linux/device.h>
#include <linux/err.h>
#include <linux/sec_class.h>

/* CAUTION : Do not be declared as external sec_class  */
static struct class *sec_class;

static int sec_class_match_device_by_name(struct device *dev, const void *data)
{
	const char *name = data;

	return (0 == strcmp(dev->kobj.name, name));
}

struct device *sec_dev_get_by_name(char *name)
{
	return class_find_device(sec_class, NULL, name,
			sec_class_match_device_by_name);
}
EXPORT_SYMBOL(sec_dev_get_by_name);

struct device *sec_device_create(dev_t devt, void *drvdata, const char *fmt)
{
	struct device *dev;

	if (unlikely(!sec_class)) {
		pr_err("Not yet created class(sec)!\n");
		BUG();
	}

	if (unlikely(IS_ERR(sec_class))) {
		pr_err("Failed to create class(sec) %ld\n", PTR_ERR(sec_class));
		BUG();
	}

	dev = device_create(sec_class, NULL, devt, drvdata, fmt);
	if (IS_ERR(dev))
		pr_err("Failed to create device %s %ld\n", fmt, PTR_ERR(dev));
	else
		pr_debug("(%s) : %s : %d\n", __func__, fmt, dev->devt);

	return dev;
}
EXPORT_SYMBOL(sec_device_create);

void sec_device_destroy(dev_t devt)
{
	if (unlikely(!devt)) {
		pr_err("(%s): Not allowed to destroy dev %d\n", __func__, devt);
	} else {
		pr_info("(%s): %d\n", __func__, devt);
		device_destroy(sec_class, devt);
	}
}
EXPORT_SYMBOL(sec_device_destroy);

static int __init sec_class_create(void)
{
	sec_class = class_create(THIS_MODULE, "sec");
	if (unlikely(IS_ERR(sec_class))) {
		pr_err("Failed to create class(sec) %ld\n", PTR_ERR(sec_class));
		return PTR_ERR(sec_class);
	}
	return 0;
}
subsys_initcall(sec_class_create);

