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

#include <linux/device.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/of.h>

struct sec_pmov_device {
	unsigned int 		ldo_num;
	struct regulator* 	ldo[10];
	struct pinctrl* 	pctl;
};

static struct sec_pmov_device pmov_info;

static int sec_pmov_probe(struct platform_device *pdev)
{
	struct regulator* vreg;
	struct pinctrl* pins;
	struct pinctrl_state * pctl_state;
	char ldo_name[8];
	unsigned int i;
	int ret;

	pr_info("%s\n", __func__);

	if (!pdev->dev.of_node) {
		pr_err("%s: no of_node\n", __func__);
		goto done;
	}

	ret = of_property_read_u32(pdev->dev.of_node, "ldo-num", &pmov_info.ldo_num);
	pr_info("%s: ldo_num:%u\n", __func__, pmov_info.ldo_num);
	if (ret < 0)
		pmov_info.ldo_num = 0;

	for(i=0; i<pmov_info.ldo_num; i++) {
		sprintf(ldo_name, "ldo%u", i);
		
		vreg = devm_regulator_get(&pdev->dev, ldo_name);
		if ( IS_ERR(vreg) ) {
			pr_err("%s: no ldo\n", __func__);
			goto no_regulator;
		}
		pmov_info.ldo[i] = vreg;
			
		ret = regulator_enable(vreg);
		if ( ret )
			pr_err("%s: enable failed.\n", __func__);

		ret = regulator_disable(vreg);
		if ( ret )
			pr_err("%s: enable failed.\n", __func__);

		pr_info("%s: %s init\n", __func__, ldo_name);
	}
no_regulator:

	pins = devm_pinctrl_get(&pdev->dev);
	if ( IS_ERR(pins) ) {
		pr_err("%s: no gpios\n", __func__);
		goto no_gpios;
	}
	pmov_info.pctl = pins;

	pctl_state = pinctrl_lookup_state(pins, "default");
	if ( IS_ERR(pctl_state) ) {
		pr_err("%s: cannot get default configuration\n", __func__);
		goto no_gpios;
	}
	
	pinctrl_select_state(pins, pctl_state);
no_gpios:

done:
	pr_info("%s done.\n", __func__);
	return 0;
}

static int sec_pmov_remove(struct platform_device *pdev)
{
	unsigned int i;

	if (pmov_info.pctl)
		devm_pinctrl_put(pmov_info.pctl);

	for(i=0; i<pmov_info.ldo_num; i++)
		if (pmov_info.ldo[i])
			devm_regulator_put(pmov_info.ldo[i]);

	return 0;
}

static struct of_device_id sec_pmov_match_table[] = {
	{.compatible = "sec,pmov"},
	{},
};

static struct platform_driver sec_pmov_driver = {
	.probe = sec_pmov_probe,
	.remove = sec_pmov_remove,
	.driver = {
		.name = "sec_pmov",
		.owner = THIS_MODULE,
		.of_match_table = sec_pmov_match_table,
	},
};

static int __init sec_pmov_init(void)
{
	return platform_driver_register(&sec_pmov_driver);
}
subsys_initcall(sec_pmov_init);

static void __exit sec_pmov_exit(void)
{
	platform_driver_unregister(&sec_pmov_driver);
}
module_exit(sec_pmov_exit);

MODULE_DESCRIPTION("Samsung Power Management Overrider");
MODULE_LICENSE("GPL v2");
