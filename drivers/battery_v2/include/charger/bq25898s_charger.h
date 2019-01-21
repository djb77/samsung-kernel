/*
 * bq25898s_charger.h
 * Samsung BQ25898S Charger Header
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

#ifndef __BQ25898S_CHARGER_H
#define __BQ25898S_CHARGER_H __FILE__

#include "include/sec_charging_common.h"

#define BQ25898S_CHG_REG_00			0x00
#define BQ25898S_CHG_ENABLE_HIZ_MODE_SHIFT	7
#define BQ25898S_CHG_ENABLE_HIZ_MODE_MASK	(1 << BQ25898S_CHG_ENABLE_HIZ_MODE_SHIFT)
#define BQ25898S_CHG_IINLIM_MASK		0x3F

#define BQ25898S_CHG_REG_02			0x02

#define BQ25898S_CHG_REG_03			0x03
#define BQ25898S_CHG_CONFIG_SHIFT		4
#define BQ25898S_CHG_CONFIG_MASK		(1 << BQ25898S_CHG_CONFIG_SHIFT)

#define BQ25898S_CHG_REG_04			0x04
#define BQ25898S_CHG_ICHG_MASK			0x3F

#define BQ25898S_CHG_REG_05			0x05
#define BQ25898S_CHG_ITERM_MASK			0x0F

#define BQ25898S_CHG_REG_06			0x06
#define BQ25898S_CHG_VREG_MASK			0xFC

#define BQ25898S_CHG_REG_07			0x07
#define BQ25898S_CHG_WATCHDOG_SHIFT		4
#define BQ25898S_CHG_WATCHDOG_MASK		(0x3 << BQ25898S_CHG_WATCHDOG_SHIFT)

#define BQ25898S_CHG_REG_0C			0x0C
#define BQ25898S_CHG_REG_11			0x11
#define BQ25898S_CHG_REG_14			0x14
enum bq25898s_watchdog_timer {
	WATCHDOG_TIMER_DISABLE = 0,
	WATCHDOG_TIMER_40S,
	WATCHDOG_TIMER_80S,
	WATCHDOG_TIMER_160S,
};

struct bq25898s_charger_platform_data {
	int irq_gpio;
	unsigned int float_voltage;
	unsigned int full_check_current;
};

struct bq25898s_charger {
	struct device           *dev;
	struct i2c_client       *i2c;
	struct mutex            i2c_lock;

	struct bq25898s_charger_platform_data *pdata;

	struct power_supply	psy_chg;

	unsigned int siop_level;
	unsigned int chg_irq;
	unsigned int is_charging;
	unsigned int charging_type;
	unsigned int cable_type;
	int input_current;
	int charging_current;
	unsigned int float_voltage;
	unsigned int full_check_current;

	u8 addr;
	int size;
};
#endif /* __BQ25898S_CHARGER_H */
