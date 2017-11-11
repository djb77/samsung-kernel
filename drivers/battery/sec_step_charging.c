/*
 *  sec_step_charging.c
 *  Samsung Mobile Battery Driver
 *
 *  Copyright (C) 2012 Samsung Electronics
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/battery/sec_battery.h>

void sec_bat_reset_step_charging(struct sec_battery_info *battery)
{
	battery->step_charging_status = -1;
}
/*
 * true: step is changed
 * false: not changed
 */
bool sec_bat_check_step_charging(struct sec_battery_info *battery)
{
	int new = battery->step_charging_status, i;

	if (!battery->step_charging_step)
		return false;

	if (battery->step_charging_status < 0)
		i = 0;
	else
		i = battery->step_charging_status;

	while(i < battery->step_charging_step) {
		if (battery->voltage_avg < battery->pdata->step_charging_condition[i]){
			new = i;
			break;
		}
		i++;
	}

	if (new != battery->step_charging_status) {
		pr_info("%s : prev=%d, new=%d, vavg=%d, current=%d\n", __func__,
			battery->step_charging_status, new, battery->voltage_avg, battery->pdata->step_charging_current[new]);
		battery->pdata->charging_current[POWER_SUPPLY_TYPE_HV_MAINS].fast_charging_current = battery->pdata->step_charging_current[new];
		battery->pdata->charging_current[POWER_SUPPLY_TYPE_HV_ERR].fast_charging_current = battery->pdata->step_charging_current[new];
		battery->step_charging_status = new;
		return true;
	}
	return false;
}

void sec_step_charging_init(struct sec_battery_info *battery, struct device *dev)
{
	struct device_node *np = dev->of_node;
	int ret, len;
	sec_battery_platform_data_t *pdata = battery->pdata;
	unsigned int i;
	const u32 *p;

	p = of_get_property(np, "battery,step_charging_condtion", &len);
	if (!p) {
		battery->step_charging_step = 0;
	} else {
		len = len / sizeof(u32);
		battery->step_charging_step = len;
		pdata->step_charging_condition = kzalloc(sizeof(u32) * len, GFP_KERNEL);
		ret = of_property_read_u32_array(np, "battery,step_charging_condtion",
				pdata->step_charging_condition, len);
		if (ret) {
			pr_info("%s : step_charging_condtion read fail\n", __func__);
			battery->step_charging_step = 0;
		} else {
			pdata->step_charging_current = kzalloc(sizeof(u32) * len, GFP_KERNEL);
			ret = of_property_read_u32_array(np, "battery,step_charging_current",
					pdata->step_charging_current, len);
			if (ret) {
				pr_info("%s : step_charging_current read fail\n", __func__);
				battery->step_charging_step = 0;
			} else {
				battery->step_charging_status = -1;
				for (i = 0; i < len; i++) {
					pr_info("%s : step condition(%d), current(%d)\n",
					__func__, pdata->step_charging_condition[i],
					pdata->step_charging_current[i]);
				}
			}
		}
	}

}

