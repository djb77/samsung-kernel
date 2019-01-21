/*
 * sec_charger.h
 * Samsung Mobile Charger Header
 *
 * Copyright (C) 2012 Samsung Electronics, Inc.
 *
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

#ifndef __SEC_CHARGER_H
#define __SEC_CHARGER_H __FILE__

#include <linux/battery/sec_charging_common.h>

struct sec_charger_info {
	struct i2c_client		*client;
	sec_battery_platform_data_t *pdata;
	struct power_supply		psy_chg;
	struct delayed_work isr_work;

	int cable_type;
	int status;
	bool is_charging;

	/* charging current : + charging, - OTG */
	int charging_current;
	unsigned charging_current_max;

	/* register programming */
	int reg_addr;
	int reg_data;
	int irq_base;
};

bool sec_hal_chg_init(struct i2c_client *);
bool sec_hal_chg_suspend(struct i2c_client *);
bool sec_hal_chg_shutdown(struct i2c_client *);
bool sec_hal_chg_resume(struct i2c_client *);
bool sec_hal_chg_get_property(struct i2c_client *,
				enum power_supply_property,
				union power_supply_propval *);
bool sec_hal_chg_set_property(struct i2c_client *,
				enum power_supply_property,
				const union power_supply_propval *);

ssize_t sec_hal_chg_show_attrs(struct device *dev,
				const ptrdiff_t offset, char *buf);

ssize_t sec_hal_chg_store_attrs(struct device *dev,
				const ptrdiff_t offset,
				const char *buf, size_t count);

ssize_t sec_chg_show_attrs(struct device *dev,
				struct device_attribute *attr, char *buf);

ssize_t sec_chg_store_attrs(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count);

#define SEC_CHARGER_ATTR(_name)				\
{							\
	.attr = {.name = #_name, .mode = 0664},	\
	.show = sec_chg_show_attrs,			\
	.store = sec_chg_store_attrs,			\
}

enum {
	CHG_REG = 0,
	CHG_DATA,
	CHG_REGS,
};

extern sec_battery_platform_data_t sec_battery_pdata;
#endif /* __SEC_CHARGER_H */
