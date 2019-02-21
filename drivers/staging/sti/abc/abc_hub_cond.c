/* abc_hub_cond.c
 *
 * Abnormal Behavior Catcher Hub Driver Sub Module(Conncet Detect)
 *
 * Copyright (C) 2017 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/sti/abc_hub.h>

int parse_cond_data(struct device *dev,
		    struct abc_hub_platform_data *pdata,
		    struct device_node *np)
{
	/* implement parse dt for sub module */
	/* the following code is just example. replace it to your code */

	struct device_node *cond_np;

	cond_np = of_find_node_by_name(np, "cond");

	pdata->cond_pdata.name = of_get_property(cond_np, "cond,name", NULL);

	if (of_property_read_u32(cond_np, "cond,param1", &pdata->cond_pdata.param1)) {
		dev_err(dev, "Failed to get cond,param1: node not exist\n");
		return -EINVAL;
	}

	if (of_property_read_u32(cond_np, "cond,param2", &pdata->cond_pdata.param2)) {
		dev_err(dev, "Failed to get cond,param2: node not exist\n");
		return -EINVAL;
	}

	return 0;
}

int abc_hub_cond_init(struct device *dev)
{
	/* implement init sequence : return 0 if success, return -1 if fail */
	// struct abc_hub_info *pinfo = dev_get_drvdata(dev);

	return 0;
}

int abc_hub_cond_suspend(struct device *dev)
{
	/* implement suspend sequence */

	return 0;
}

int abc_hub_cond_resume(struct device *dev)
{
	/* implement resume sequence */

	return 0;
}

void abc_hub_cond_enable(struct device *dev, int enable)
{
	/* common sequence */
	struct abc_hub_info *pinfo = dev_get_drvdata(dev);

	pinfo->pdata->cond_pdata.enabled = enable;

	/* implement enable sequence */
}

MODULE_DESCRIPTION("Samsung ABC Hub Sub Module1(cond) Driver");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");
