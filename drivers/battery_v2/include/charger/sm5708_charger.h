/*
 * sm5708_charger.h
 * Samsung SM5708 Charger Header
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

#ifndef __SM5708_CHARGER_H
#define __SM5708_CHARGER_H __FILE__

#include <linux/mfd/core.h>
#include <linux/mfd/sm5708/sm5708.h>
#include <linux/regulator/machine.h>
#include "include/sec_charging_common.h"

/* CONFIG: Kernel Feature & Target System configuration */

#define SM5708_SUPPORT_OTG_CONTROL        /*- New A series dosen't support, It's MUST be disabled */

/* SM5708 Charger - RESET condition configuaration */

enum {
	CHIP_ID = 0,
	CHARGER_OP_MODE = 1,
	DATA,
};
ssize_t sm5708_chg_show_attrs(struct device *dev,
				struct device_attribute *attr, char *buf);
ssize_t sm5708_chg_store_attrs(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count);

#define SM5708_CHARGER_ATTR(_name)				\
{							                    \
	.attr = {.name = #_name, .mode = 0664},	    \
	.show = sm5708_chg_show_attrs,			    \
	.store = sm5708_chg_store_attrs,			\
}
enum {
	SM5708_CHG_SRC_VBUS = 0x0,
};

enum {
	SM5708_CHG_OTG_CURRENT_0_5A     = 0x0,
	SM5708_CHG_OTG_CURRENT_0_7A,
	SM5708_CHG_OTG_CURRENT_0_9A,
	SM5708_CHG_OTG_CURRENT_1_5A,
};

enum {
	SM5708_CHG_BST_IQ3LIMIT_2_0A    = 0x0,
	SM5708_CHG_BST_IQ3LIMIT_2_8A,
	SM5708_CHG_BST_IQ3LIMIT_3_5A,
	SM5708_CHG_BST_IQ3LIMIT_4_0A,
};

/* Interrupt status Index & Offset */
enum {
	SM5708_INT_STATUS1 = 0x0,
	SM5708_INT_STATUS2,
	SM5708_INT_STATUS3,
	SM5708_INT_STATUS4,
	SM5708_INT_MAX,
};

enum {
	SM5708_INT_STATUS1_VBUSPOK          = 0x0,
	SM5708_INT_STATUS1_VBUSUVLO,
	SM5708_INT_STATUS1_VBUSOVP,
	SM5708_INT_STATUS1_VBUSLIMIT,
};

enum {
	SM5708_INT_STATUS2_AICL             = 0x0,
	SM5708_INT_STATUS2_BATOVP,
	SM5708_INT_STATUS2_NOBAT,
	SM5708_INT_STATUS2_CHGON,
	SM5708_INT_STATUS2_Q4FULLON,
	SM5708_INT_STATUS2_TOPOFF,
	SM5708_INT_STATUS2_DONE,
	SM5708_INT_STATUS2_WDTMROFF,
};

enum {
	SM5708_INT_STATUS3_THEMREG          = 0x0,
	SM5708_INT_STATUS3_THEMSHDN,
	SM5708_INT_STATUS3_OTGFAIL,
	SM5708_INT_STATUS3_DISLIMIT,
	SM5708_INT_STATUS3_PRETMROFF,
	SM5708_INT_STATUS3_FASTTMROFF,
	SM5708_INT_STATUS3_LOWBATT,
	SM5708_INT_STATUS3_nEQ4,
};

enum {
	SM5708_INT_STATUS4_FLED1SHORT       = 0x0,
	SM5708_INT_STATUS4_FLED1OPEN,
	SM5708_INT_STATUS4_FLED2SHORT,
	SM5708_INT_STATUS4_FLED2OPEN,
	SM5708_INT_STATUS4_BOOSTPOK_NG,
	SM5708_INT_STATUS4_BOOSTPOL,
	SM5708_INT_STATUS4_ABSTMR1OFF,
	SM5708_INT_STATUS4_SBPS,
};

enum {
	SM5708_MANUAL_RESET_TIME_7s = 0x1,
	SM5708_MANUAL_RESET_TIME_8s = 0x2,
	SM5708_MANUAL_RESET_TIME_9s = 0x3,
};

enum {
	SM5708_WATCHDOG_RESET_TIME_30s  = 0x0,
	SM5708_WATCHDOG_RESET_TIME_60s  = 0x1,
	SM5708_WATCHDOG_RESET_TIME_90s  = 0x2,
	SM5708_WATCHDOG_RESET_TIME_120s = 0x3,
};

enum {
	SM5708_TOPOFF_TIMER_10m         = 0x0,
	SM5708_TOPOFF_TIMER_20m         = 0x1,
	SM5708_TOPOFF_TIMER_30m         = 0x2,
	SM5708_TOPOFF_TIMER_45m         = 0x3,
};

enum {
	SM5708_BUCK_BOOST_FREQ_3MHz     = 0x0,
	SM5708_BUCK_BOOST_FREQ_2_4MHz   = 0x1,
	SM5708_BUCK_BOOST_FREQ_1_5MHz   = 0x2,
	SM5708_BUCK_BOOST_FREQ_1_8MHz   = 0x3,
};

/* for VZW support */
#define SLOW_CHARGING_CURRENT_STANDARD		400

/* SM5708 Charger - AICL reduce current configuration */
#define REDUCE_CURRENT_STEP			100
#define MINIMUM_INPUT_CURRENT			300
#define AICL_VALID_CHECK_DELAY_TIME             10

#define SM5708_EN_DISCHG_FORCE_MASK		0x02
#define SM5708_SBPS_MASK			0x07

#define EN_TOPOFF_IRQ 0

struct sm5708_charger_data {
	struct device *dev;
	struct i2c_client *i2c;
	struct sec_charger_platform_data *pdata;

	struct power_supply	psy_chg;
	struct power_supply	psy_otg;

	/* for IRQ-service handling */
	int irq_aicl;
	int irq_vbus_pok;

	int irq_topoff;
	int irq_done;
	int irq_otgfail;

	/* for Workqueue & wake-lock, mutex process */
	struct mutex charger_mutex;

	struct workqueue_struct *wqueue;
	struct delayed_work aicl_work;
	struct delayed_work topoff_work;
	struct delayed_work op_mode_switch_work;        /* for WA obnormal switch case in JIG cable */

#if defined(SM5708_SW_SOFT_START)
	struct wake_lock softstart_wake_lock;
#endif
	struct wake_lock check_slow_wake_lock;
	struct wake_lock aicl_wake_lock;

	/* for charging operation handling */
	int status;
	int charge_mode;
	unsigned int is_charging;
	unsigned int cable_type;
	unsigned int input_current;
	unsigned int charging_current;

	bool topoff_pending;
	bool slow_late_chg_mode;

	bool is_mdock;
};

extern int sm5708_call_fg_device_id(void);

#if defined(SM5708_WATCHDOG_RESET_ACTIVATE)
extern void sm5708_charger_watchdog_timer_keepalive(void);
#endif

#endif /* __SM5708_CHARGER_H */

