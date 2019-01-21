/*
 * s2mu005_charger.h - Header of S2MU005 Charger Driver
 *
 * Copyright (C) 2017 Samsung Electronics Co.Ltd
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef S2MU005_CHARGER_H
#define S2MU005_CHARGER_H
#include <linux/mfd/samsung/s2mu005.h>
#include <linux/mfd/samsung/s2mu005-private.h>
#include "../sec_charging_common.h"

#define MASK(width, shift)      (((0x1 << (width)) - 1) << shift)

#define S2MU005_CHG_STATUS0		0x08
#define S2MU005_CHG_STATUS1		0x09
#define S2MU005_CHG_STATUS2		0x0A
#define S2MU005_CHG_STATUS3		0x0B
#define S2MU005_CHG_STATUS4		0x0C
#define S2MU005_CHG_STATUS5		0x0D
#define S2MU005_CHG_CTRL0       0x0E
#define S2MU005_CHG_CTRL1       0x0F
#define S2MU005_CHG_CTRL2       0x10
#define S2MU005_CHG_CTRL3       0x11
#define S2MU005_CHG_CTRL4       0x12
#define S2MU005_CHG_CTRL5       0x13
#define S2MU005_CHG_CTRL6       0x14
#define S2MU005_CHG_CTRL7       0x15
#define S2MU005_CHG_CTRL8       0x16
#define S2MU005_CHG_CTRL9		0x17
#define S2MU005_CHG_CTRL10		0x18
#define S2MU005_CHG_CTRL11		0x19
#define S2MU005_CHG_CTRL12		0x1A
#define S2MU005_CHG_CTRL13		0x1B
#define S2MU005_CHG_CTRL14		0x1C
#define S2MU005_CHG_CTRL15		0x1D
#define S2MU005_CHG_CTRL16		0x1E
#define S2MU005_CHG_CTRL17		0x1F
#define S2MU005_CHG_CTRL18		0x20

/* S2MU005_SC_INT_MASK */
#define VMID_M_SHIFT	5
#define VMID_M_MASK		BIT(VMID_M_SHIFT)

/* S2MU005_CHG_STATUS0 */
#define VBUS_OK_SHIFT	7
#define VBUS_OK_MASK	BIT(VBUS_OK_SHIFT)

#define WCIN_OK_SHIFT	6
#define S2MU005_WCIN_OK_MASK	BIT(WCIN_OK_SHIFT)

#define VMID_OK_SHIFT	5
#define VMID_OK_MASK	BIT(VMID_OK_SHIFT)

#define CHG_OK_SHIFT	4
#define CHG_OK_MASK	BIT(CHG_OK_SHIFT)

#define CHG_STATUS_SHIFT	0
#define CHG_STATUS_WIDTH	4
#define CHG_STATUS_MASK		MASK(CHG_STATUS_WIDTH, CHG_STATUS_SHIFT)

#define CHG_STATUS_PRE_CHARGE	2
#define	CHG_STATUS_COOL_CHARGE	3
#define CHG_STATUS_CC		4
#define CHG_STATUS_CV		5
#define CHG_STATUS_DONE_FLAG	6
#define CHG_STATUS_TOP_OFF	7
#define CHG_STATUS_DONE		8

/* S2MU005_CHG_STATUS1 */
#define DET_BAT_STATUS_SHIFT	7
#define DET_BAT_STATUS_MASK	BIT(DET_BAT_STATUS_SHIFT)


/* S2MU005_CHG_STATUS2 */
#define BAT_STATUS_SHIFT	4
#define BAT_STATUS_WIDTH	3
#define BAT_STATUS_MASK		MASK(BAT_STATUS_WIDTH, BAT_STATUS_SHIFT)

#define BAT_OVP_DET		0
#define BAT_SELF_DISCHARGING	1
#define LOW_BATTERY_DETECTION_IN_CHARGING	2
#define	COOL_CHARGE_DET		5
#define FAST_CHARGE_DET		6
#define	BAT_VOL_DET		7

/* S2MU005_CHG_STATUS3 */
#define CHG_EVENT_STATUS_SHIFT	0
#define CHG_EVENT_STATUS_WIDTH	4
#define CHG_EVENT_STATUS_MASK	MASK(CHG_EVENT_STATUS_WIDTH, CHG_EVENT_STATUS_SHIFT)

#define THREMAL_SHUT_DOWN	1
#define THERMAL_FOLDBACK	2
#define VSYS_OVER_VOVP		3
#define VSYS_OVER_VUVLO		4
#define WATCHDOG_SUSPENSION	5
#define WATCHDOG_AP_RESET	6

#define VBUS_OVP_MASK		0x70
#define VBUS_OVP_SHIFT		4

/* S2MU005_CHG_CTRL0 */
#define EN_CHG_SHIFT		4
#define EN_CHG_MASK		BIT(EN_CHG_SHIFT)

#define REG_MODE_SHIFT		0
#define REG_MODE_WIDTH		3
#define REG_MODE_MASK		MASK(REG_MODE_WIDTH, REG_MODE_SHIFT)

#define CHARGER_OFF_MODE	0
#define CHG_MODE		3
#define BUCK_MODE		1
#define OTG_BST_MODE		6

#define S2MU005_CHARGER_REG_MODE_OTG 2

/* S2MU005_CHG_CTRL1 */
#define SET_VIN_DROP_SHIFT	4
#define SET_VIN_DROP_WIDTH	3
#define SET_VIN_DROP_MASK	MASK(SET_VIN_DROP_WIDTH, SET_VIN_DROP_SHIFT)

/* S2MU005_CHG_CTRL2 */
#define INPUT_CURRENT_LIMIT_SHIFT	0
#define INPUT_CURRENT_LIMIT_WIDTH	6
#define INPUT_CURRENT_LIMIT_MASK	MASK(INPUT_CURRENT_LIMIT_WIDTH,\
					INPUT_CURRENT_LIMIT_SHIFT)

/* S2MU005_CHG_CTRL4 */
#define OTG_OCP_SW_ON_SHIFT		5
#define OTG_OCP_SW_ON_MASK		BIT(OTG_OCP_SW_ON_SHIFT)

#define OTG_OCP_SW_OFF_SHIFT	4
#define OTG_OCP_SW_OFF_MASK		BIT(OTG_OCP_SW_OFF_SHIFT)

#define SET_OTG_OCP_SHIFT	2
#define SET_OTG_OCP_WIDTH	2
#define SET_OTG_OCP_MASK	MASK(SET_OTG_OCP_WIDTH, SET_OTG_OCP_SHIFT)

/* S2MU005_CHG_CTRL5 */
#define SET_VF_VMID_BST_SHIFT	0
#define SET_VF_VMID_BST_WIDTH	5
#define SET_VF_VMID_BST_MASK	MASK(SET_VF_VMID_BST_WIDTH, SET_VF_VMID_BST_SHIFT)

/* S2MU005_CHG_CTRL6 */
#define COOL_CHARGING_CURRENT_SHIFT 0
#define COOL_CHARGING_CURRENT_WIDTH 6
#define COOL_CHARGING_CURRENT_MASK MASK(COOL_CHARGING_CURRENT_WIDTH,\
					COOL_CHARGING_CURRENT_SHIFT)

/* S2MU005_CHG_CTRL7 */
#define FAST_CHARGING_CURRENT_SHIFT	0
#define FAST_CHARGING_CURRENT_WIDTH	6
#define FAST_CHARGING_CURRENT_MASK	MASK(FAST_CHARGING_CURRENT_WIDTH,\
					FAST_CHARGING_CURRENT_SHIFT)

/* S2MU005_CHG_CTRL8 */
#define SET_VF_VBAT_SHIFT	1
#define SET_VF_VBAT_WIDTH	6
#define SET_VF_VBAT_MASK	MASK(SET_VF_VBAT_WIDTH, SET_VF_VBAT_SHIFT)

/* S2MU005_CHG_CTRL10 */
#define SECOND_TOPOFF_CURRENT_SHIFT	4
#define SECOND_TOPOFF_CURRENT_WIDTH	4
#define SECOND_TOPOFF_CURRENT_MASK	MASK(SECOND_TOPOFF_CURRENT_WIDTH,\
					SECOND_TOPOFF_CURRENT_SHIFT)

#define FIRST_TOPOFF_CURRENT_SHIFT	0
#define FIRST_TOPOFF_CURRENT_WIDTH	4
#define FIRST_TOPOFF_CURRENT_MASK	MASK(FIRST_TOPOFF_CURRENT_WIDTH,\
					FIRST_TOPOFF_CURRENT_SHIFT)

/* S2MU005_CHG_CTRL11 */
#define SET_OSC_BST_SHIFT	5
#define SET_OSC_BST_WIDTH	2
#define SET_OSC_BST_MASK	MASK(SET_OSC_BST_WIDTH, SET_OSC_BST_SHIFT)

#define SET_OSC_BUCK_SHIFT	3
#define SET_OSC_BUCK_WIDTH	2
#define SET_OSC_BUCK_MASK	MASK(SET_OSC_BUCK_WIDTH, SET_OSC_BUCK_SHIFT)

/* S2MU005_CHG_CTRL15 */
#define T_EN_OTG_SHIFT		2
#define T_EN_OTG_WIDTH		2
#define T_EN_OTG_MASK		MASK(T_EN_OTG_WIDTH, T_EN_OTG_SHIFT)

/* S2MU005_REG_SELFDIS_CFG2 */
#define FC_SELF_DISCHG_SHIFT	4
#define FC_SELF_DISCHG_MASK		BIT(FC_SELF_DISCHG_SHIFT)

/* S2MU005_REG_SELFDIS_CFG3 */
#define SELF_DISCHG_MODE_SHIFT	7
#define SELF_DISCHG_MODE_MASK	BIT(SELF_DISCHG_MODE_SHIFT)

/* S2MU004_REG_SC_INT2 */
#define S2MU005_IVR_I	(1 << 2)

#define S2MU005_IVR_M	(1 << 2)

#define IVR_STATUS	0x02

#define IVR_M_SHIFT	2
#define IVR_M_MASK	BIT(IVR_M_SHIFT)

#define REDUCE_CURRENT_STEP			50
#define MINIMUM_INPUT_CURRENT			300
#if defined(CONFIG_TABLET_MODEL_CONCEPT) && !defined(CONFIG_SEC_FACTORY)
#define SLOW_CHARGING_CURRENT_STANDARD	999
#else
#define SLOW_CHARGING_CURRENT_STANDARD	400
#endif

enum {
	CHG_REG = 0,
	CHG_DATA,
	CHG_REGS,
};

typedef struct s2mu005_charger_platform_data {
	sec_charging_current_t *charging_current;
	int chg_float_voltage;
	char *charger_name;
	bool chg_eoc_dualpath;
	uint32_t is_1MHz_switching:1;
	bool always_enable;
	/* 2nd full check */
	 sec_battery_full_charged_t full_check_type_2nd;
	/* Slow charging current */
	int slow_charging_current;
	int mivr_voltage;
} s2mu005_charger_platform_data_t;

struct s2mu005_charger_data {
	struct i2c_client       *client;
	struct device *dev;
	struct s2mu005_platform_data *s2mu005_pdata;
	struct delayed_work	charger_work;
	struct delayed_work	det_bat_work;
	struct workqueue_struct *charger_wqueue;
	struct power_supply	psy_chg;
	struct power_supply	psy_otg;
	s2mu005_charger_platform_data_t *pdata;
	int dev_id;
	int input_current;
	int charging_current;
	int topoff_current;
	int cable_type;
	bool is_charging;
	int charge_mode;
	struct mutex charger_mutex;

	/* register programming */
	int reg_addr;
	int reg_data;

	bool ovp;
	bool otg_on;

	int unhealth_cnt;
	int status;

	/* s2mu005 */
	int irq_det_bat;
	int irq_chg;
	u8 fg_clock;
	int fg_mode;

	int irq_ivr;

	struct delayed_work ivr_work;
	struct wake_lock ivr_wake_lock;
	int irq_ivr_enabled;
	int ivr_on;
	bool slow_charging;

	bool suspended;
	bool pending_chg_work;
};

#endif /*S2MU005_CHARGER_H*/
