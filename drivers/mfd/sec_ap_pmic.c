/*
 * Copyright (c) 2015, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/sec_class.h>
#include <linux/device.h>
#include <linux/qpnp/power-on.h>

static struct device *sec_ap_pmic_dev;

static ssize_t chg_det_show(struct device *in_dev,
				struct device_attribute *attr, char *buf)
{
	int ret = 0;

	ret = qpnp_pon_check_chg_det();

	pr_info("%s: ret = %d\n", __func__, ret);
	return sprintf(buf, "%d\n", ret);
}

static DEVICE_ATTR_RO(chg_det);

static int __init sec_ap_pmic_init(void)
{
	int err;

	sec_ap_pmic_dev = sec_device_create(0, NULL, "ap_pmic");

	if (unlikely(IS_ERR(sec_ap_pmic_dev))) {
		pr_err("%s: Failed to create ap_pmic device\n", __func__);
		err = PTR_ERR(sec_ap_pmic_dev);
		goto err_device_create;
	}

	err = device_create_file(sec_ap_pmic_dev, &dev_attr_chg_det);
	if (unlikely(err)) {
		pr_err("%s: Failed to create chg_det\n", __func__);
		goto err_device_create;
	}

	pr_info("%s: ap_pmic successfully inited.\n", __func__);

	return 0;

err_device_create:
	return err;
}
module_init(sec_ap_pmic_init);

MODULE_DESCRIPTION("sec_ap_pmic driver");
MODULE_AUTHOR("Jiman Cho <jiman85.cho@samsung.com");
MODULE_LICENSE("GPL");

