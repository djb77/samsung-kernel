/*
 *  sec_multi_charger.c
 *  Samsung Mobile Charger Driver
 *
 *  Copyright (C) 2015 Samsung Electronics
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#define DEBUG

#include "include/sec_multi_port_charger.h"

static enum power_supply_property sec_multi_charger_props[] = {
};

static int get_sub_charging_current_by_siop(struct sec_multi_charger_info *charger, int sub_charging_current, int siop_level)
{
	int charging_current = charger->pdata->siop_sub_current_max;
	if (siop_level != 100)
		charging_current = charger->pdata->siop_sub_current_min;

	return (sub_charging_current > charging_current) ? charging_current:sub_charging_current;
}

void get_prev_sub_charging_current(struct sec_battery_info *battery, int * total_current,
		int * main_charging_current, int * sub_charging_current)
{
	struct power_supply *psy_chg = NULL;
	struct sec_multi_charger_info *charger;
	pr_info("%s: total: %d, main: %d, sub: %d\n", __func__,
			*total_current, *main_charging_current, *sub_charging_current);
	psy_chg = get_power_supply_by_name("sec-multi-charger");
	if (!psy_chg) {
		pr_err("%s: Fail to get psy (%s)\n",
				__func__, "sec-multi-charger");
		return;
	}
	charger = container_of(psy_chg, struct sec_multi_charger_info, psy_chg);
	if (!battery->pogo_status)
		*sub_charging_current = 0;
	else if (battery->cable_type == SEC_BATTERY_CABLE_POGO_WCIN) {
		*sub_charging_current = (*total_current > charger->pdata->pogo_wcin_sub_current) ?
			charger->pdata->pogo_wcin_sub_current : *total_current;
		*main_charging_current = *total_current - *sub_charging_current;
	} else if (!is_nocharge_type(battery->cable_type)) {
		*total_current += *sub_charging_current;
		if (*total_current > charger->pdata->max_total_charging_current)
			*total_current = charger->pdata->max_total_charging_current;
	}
	pr_info("%s: total: %d, main: %d, sub: %d\n", __func__,
			*total_current, *main_charging_current, *sub_charging_current);
}
EXPORT_SYMBOL(get_prev_sub_charging_current);

void get_sub_charging_current(struct sec_battery_info *battery, int * total_current,
		int * main_charging_current, int * sub_charging_current, int * main_input_current)
{
	struct power_supply *psy_chg = NULL;
	struct sec_multi_charger_info *charger;
	pr_info("%s: total: %d, main: %d, sub: %d\n", __func__,
			*total_current, *main_charging_current, *sub_charging_current);
	psy_chg = get_power_supply_by_name("sec-multi-charger");
	if (!psy_chg) {
		pr_err("%s: Fail to get psy (%s)\n",
				__func__, "sec-multi-charger");
		return;
	}
	charger = container_of(psy_chg, struct sec_multi_charger_info, psy_chg);

	if (battery->mix_limit)
		*sub_charging_current = 0;
	if (*sub_charging_current <= *main_charging_current) {
		*sub_charging_current = get_sub_charging_current_by_siop(charger, *sub_charging_current, battery->siop_level);
	}
	if (*sub_charging_current > *total_current) {
		*sub_charging_current = *total_current;
		*main_charging_current = 0;
	} else {
		*main_charging_current = *total_current - *sub_charging_current;
	}

	if(*main_charging_current == 0) {
		*main_charging_current = charger->pdata->min_charging_current;
		*sub_charging_current -= *main_charging_current;
	}
	if (charger->cable_type == SEC_BATTERY_CABLE_POGO_WCIN) {
		if((*main_input_current < charger->pdata->max_wcin_input_current) && (*sub_charging_current > 0)) {
			*main_charging_current += *sub_charging_current;
			*sub_charging_current = 0;
		}
	}
	pr_info("%s: total: %d, main: %d, sub: %d\n", __func__,
			*total_current, *main_charging_current, *sub_charging_current);
}
EXPORT_SYMBOL(get_sub_charging_current);

static bool sec_multi_chg_check_main_charging(struct sec_multi_charger_info *charger)
{
	union power_supply_propval value;

	pr_info("%s: main condition(0x%x)\n", __func__, charger->pdata->main_charger_condition);
	if (!charger->pdata->main_charger_condition) {
		return true;
	}

	if (charger->pdata->main_charger_condition &
			SEC_MAIN_CHARGER_CONDITION_OTG_OFF) {
		psy_do_property("otg", get, POWER_SUPPLY_PROP_ONLINE, value);
		if (value.intval) {
			pr_info("%s: main charger off in case of OTG(%d)\n", __func__, value.intval);
			return false;
		}
	}

	return true;
}

static bool sec_multi_chg_check_sub_charging(struct sec_multi_charger_info *charger)
{
	union power_supply_propval value = {0,};

	if (!charger->pdata->sub_charger_condition) {
		pr_info("%s: sub charger off(default)\n", __func__);
		return false;
	}

	if (charger->multi_mode != SEC_MULTI_CHARGER_NORMAL) {
		pr_info("%s: skip multi check_sub_charging routine, because the multi_mode = %d\n",
			__func__, charger->multi_mode);
		return false;
	}

	if (charger->pdata->sub_charger_condition &
		SEC_SUB_CHARGER_CONDITION_CHARGE_DONE) {
		if (charger->sub_is_charging) {
			psy_do_property(charger->pdata->battery_name, get,
				POWER_SUPPLY_PROP_STATUS, value);
			if (value.intval == POWER_SUPPLY_STATUS_FULL) {
				pr_info("%s: sub charger off by CHARGE DONE\n", __func__);
				return false;
			}
		}
	}

	if (charger->pdata->sub_charger_condition &
		SEC_SUB_CHARGER_CONDITION_CV) {
		if (charger->sub_is_charging) {
			psy_do_property(charger->pdata->main_charger_name, get,
				POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE, value);
			if (value.intval > 0) {
				pr_info("%s: sub charger off by CV\n", __func__);
				return false;
			}
		}
	}

	if (charger->pdata->sub_charger_condition &
			SEC_SUB_CHARGER_CONDITION_CHARGE_TYPE) {
		psy_do_property("pogo", get, POWER_SUPPLY_PROP_ONLINE, value);
		if (!value.intval) {
			if (charger->sub_is_charging)
				pr_info("%s: sub charger off by CHARGE_TYPE\n", __func__);
			return false;
		} else {
			if (charger->sub_current.fast_charging_current <= 0 ) {
				pr_info("%s: sub charger off sub_charging_current(%d)\n",
						__func__, charger->sub_current.fast_charging_current);

				return false;
			}
		}
	}

	return true;
}

static void sec_multi_chg_set_input_current(struct sec_multi_charger_info *charger)
{
	union power_supply_propval value = {0,};
	int main_input_current = charger->main_current.input_current_limit,
		sub_input_current = charger->pdata->max_sub_input_current;

	if (charger->multi_mode != SEC_MULTI_CHARGER_NORMAL) {
		pr_info("%s: skip multi set_input_current routine, because the multi_mode = %d\n",
			__func__, charger->multi_mode);
		return;
	}

	mutex_lock(&charger->charger_mutex);
	pr_info("%s: main input curent: %d, sub input current: %d\n", __func__, main_input_current, sub_input_current);
	if (charger->sub_is_charging) {
		if (charger->cable_type == SEC_BATTERY_CABLE_POGO_WCIN) {
			int input_current = main_input_current;
			if (!sec_multi_chg_check_main_charging(charger)) {
				main_input_current = 0;
			} else {
				main_input_current =
					main_input_current > charger->pdata->max_wcin_input_current ?
					charger->pdata->max_wcin_input_current : charger->main_current.input_current_limit;
			}
			sub_input_current = input_current - main_input_current;
			sub_input_current = sub_input_current > 0 ? sub_input_current : 0;
		}
		if (sub_input_current > charger->pdata->max_sub_input_current)
			sub_input_current = charger->pdata->max_sub_input_current;
	} else {
		sub_input_current = 0;
	}
	pr_info("%s: main input curent: %d, sub input current: %d\n", __func__, main_input_current, sub_input_current);	

	/* set input current */
	if (charger->main_input_current > main_input_current) {
		pr_info("%s: set input current - main(%dmA -> %dmA)\n", __func__, charger->main_input_current, main_input_current);
		value.intval = main_input_current;
		psy_do_property(charger->pdata->main_charger_name, set,
				POWER_SUPPLY_PROP_CURRENT_MAX, value);
		if (main_input_current == 0) {
			value.intval = SEC_BAT_CHG_MODE_BUCK_OFF;
			psy_do_property(charger->pdata->main_charger_name, set,
					POWER_SUPPLY_PROP_CHARGING_ENABLED, value);
		}
		charger->main_input_current = main_input_current;
	}
	if (charger->sub_input_current > sub_input_current) {
		pr_info("%s: set input current - sub(%dmA -> %dmA)\n", __func__, charger->sub_input_current, sub_input_current);
		charger->sub_input_current = sub_input_current;
		value.intval = sub_input_current;
		psy_do_property(charger->pdata->sub_charger_name, set,
				POWER_SUPPLY_PROP_CURRENT_MAX, value);
	}

	/* set input current */
	if (charger->main_input_current != main_input_current) {
		pr_info("%s: set input current - main(%dmA -> %dmA)\n", __func__, charger->main_input_current, main_input_current);
		value.intval = main_input_current;
		psy_do_property(charger->pdata->main_charger_name, set,
			POWER_SUPPLY_PROP_CURRENT_MAX, value);
		if (charger->main_input_current == 0 &&
				main_input_current > 0) {
			value.intval = charger->chg_mode;
			psy_do_property(charger->pdata->main_charger_name, set,
					POWER_SUPPLY_PROP_CHARGING_ENABLED, value);
		}
		charger->main_input_current = main_input_current;
	}
	if (charger->sub_input_current != sub_input_current) {
		pr_info("%s: set input current - sub(%dmA -> %dmA)\n", __func__, charger->sub_input_current, sub_input_current);
		charger->sub_input_current = sub_input_current;
		value.intval = charger->sub_input_current;
		psy_do_property(charger->pdata->sub_charger_name, set,
			POWER_SUPPLY_PROP_CURRENT_MAX, value);
	}
	mutex_unlock(&charger->charger_mutex);
}
static void sec_multi_chg_set_charging_current(struct sec_multi_charger_info *charger)
{
	union power_supply_propval value = {0,};
	int main_charging_current = charger->main_current.fast_charging_current,
		sub_charging_current = charger->sub_current.fast_charging_current,
		total_charging_current = main_charging_current + sub_charging_current;

	if (charger->multi_mode != SEC_MULTI_CHARGER_NORMAL) {
		pr_info("%s: skip multi set_charging_current routine, because the multi_mode = %d\n",
			__func__, charger->multi_mode);
		return;
	}

	mutex_lock(&charger->charger_mutex);
	pr_info("%s: total charging current: %d, main charging curent: %d, sub charging current: %d, main input current: %d, sub input current: %d\n",
			__func__, total_charging_current, main_charging_current, sub_charging_current, charger->main_input_current, charger->sub_input_current);
	if (charger->sub_is_charging) {
		if (charger->cable_type == SEC_BATTERY_CABLE_POGO_WCIN) {
			if (charger->main_input_current == 0) {
				main_charging_current = 0;
				sub_charging_current = total_charging_current > charger->pdata->max_sub_charging_current ?
					charger->pdata->max_sub_charging_current : total_charging_current;
			}
		}
	} else {
		sub_charging_current = 0;
	}
	pr_info("%s: main charging current: %d, sub charging current: %d\n", __func__, main_charging_current, sub_charging_current);

	/* set charging current */
	if (charger->main_charging_current != main_charging_current) {
		pr_info("%s: set charging current - main(%dmA -> %dmA)\n", __func__,
			charger->main_charging_current, main_charging_current);
		charger->main_charging_current = main_charging_current;
		value.intval = charger->main_charging_current;
		psy_do_property(charger->pdata->main_charger_name, set,
			POWER_SUPPLY_PROP_CURRENT_NOW, value);
	}
	if (charger->sub_charging_current != sub_charging_current) {
		pr_info("%s: set charging current - sub(%dmA - > %dmA)\n", __func__,
			charger->sub_charging_current, sub_charging_current);
		charger->sub_charging_current = sub_charging_current;
		value.intval = charger->sub_charging_current;
		psy_do_property(charger->pdata->sub_charger_name, set,
			POWER_SUPPLY_PROP_CURRENT_NOW, value);
	}
	mutex_unlock(&charger->charger_mutex);
}

static bool sec_multi_chg_check_abnormal_case(struct sec_multi_charger_info *charger)
{
	union power_supply_propval value = {0,};
	bool check_val = false;

	if (charger->multi_mode != SEC_MULTI_CHARGER_NORMAL) {
		pr_info("%s: skip multi check_abnormal_case routine, because the multi_mode = %d\n",
			__func__, charger->multi_mode);
		return false;
	}

	/* check abnormal case */
	psy_do_property(charger->pdata->sub_charger_name, get,
		POWER_SUPPLY_EXT_PROP_CHECK_MULTI_CHARGE, value);

	check_val = (value.intval != POWER_SUPPLY_STATUS_CHARGING && charger->sub_is_charging) |
		(value.intval == POWER_SUPPLY_STATUS_CHARGING && !charger->sub_is_charging);
	pr_info("%s: check abnormal case(check_val:%d, status:%d, sub_is_charging:%d)\n",
		__func__, check_val, value.intval, charger->sub_is_charging);

	return check_val;
}

static void  sec_multi_chg_check_enable(struct sec_multi_charger_info *charger)
{
	union power_supply_propval value = {0,};
	bool sub_is_charging = charger->sub_is_charging;

	if ((charger->cable_type == SEC_BATTERY_CABLE_NONE) ||
		(charger->status == POWER_SUPPLY_STATUS_DISCHARGING) ||
		charger->chg_mode != SEC_BAT_CHG_MODE_CHARGING) {
		pr_info("%s: skip multi charging routine\n", __func__);
		return;
	}

#if defined(CONFIG_SEC_FACTORY)
	mutex_lock(&charger->charger_test_mutex);
#endif
	if (charger->multi_mode != SEC_MULTI_CHARGER_NORMAL) {
		pr_info("%s: skip multi charging routine, because the multi_mode = %d\n",
			__func__, charger->multi_mode);
#if defined(CONFIG_SEC_FACTORY)
		mutex_unlock(&charger->charger_test_mutex);
#endif
		return;
	}

	/* check sub charging */
	charger->sub_is_charging = sec_multi_chg_check_sub_charging(charger);

	sec_multi_chg_set_input_current(charger);
	sec_multi_chg_set_charging_current(charger);

	/* set sub charging */
	if (charger->sub_is_charging != sub_is_charging) {
		if (!charger->sub_is_charging) {
			value.intval = SEC_BAT_CHG_MODE_CHARGING_OFF;
			psy_do_property(charger->pdata->sub_charger_name, set,
				POWER_SUPPLY_PROP_CHARGING_ENABLED, value);
		}
		if (charger->sub_is_charging) {
			value.intval = SEC_BAT_CHG_MODE_CHARGING;
			psy_do_property(charger->pdata->sub_charger_name, set,
				POWER_SUPPLY_PROP_CHARGING_ENABLED, value);
		}

		pr_info("%s: change sub_is_charging(%d)\n", __func__, charger->sub_is_charging);
	} else if (sec_multi_chg_check_abnormal_case(charger)) {
		pr_info("%s: abnormal case, sub charger %s\n ", __func__,
			charger->sub_is_charging ? "on" : "off");

		if (charger->sub_is_charging)
			value.intval = SEC_BAT_CHG_MODE_CHARGING;
		else
			value.intval = SEC_BAT_CHG_MODE_CHARGING_OFF;

		psy_do_property(charger->pdata->sub_charger_name, set,
			POWER_SUPPLY_PROP_CHARGING_ENABLED, value);
	}
	
#if defined(CONFIG_SEC_FACTORY)
	mutex_unlock(&charger->charger_test_mutex);
#endif
	pr_info("%s: sub_is_charging:%d, main_input: %d, sub_input: %d, "
		"main_charge: %d, sub_charge: %d\n",
		__func__, charger->sub_is_charging, charger->main_input_current, charger->sub_input_current,
		charger->main_charging_current, charger->sub_charging_current);
}

static int sec_multi_chg_get_property(struct power_supply *psy,
			    enum power_supply_property psp,
			    union power_supply_propval *val)
{
	struct sec_multi_charger_info *charger = container_of(psy, struct sec_multi_charger_info, psy_chg);
	enum power_supply_ext_property ext_psp = psp;
	union power_supply_propval value = {0,};

	value.intval = val->intval;

	switch (psp) {
	case POWER_SUPPLY_PROP_HEALTH:
		psy_do_property(charger->pdata->sub_charger_name, get, psp, value);
		psy_do_property(charger->pdata->main_charger_name, get, psp, value);
		val->intval = value.intval;
		break;
	case POWER_SUPPLY_PROP_STATUS:
	case POWER_SUPPLY_PROP_ONLINE:
	case POWER_SUPPLY_PROP_PRESENT:
	case POWER_SUPPLY_PROP_CHARGE_TYPE:
	case POWER_SUPPLY_PROP_VOLTAGE_MAX:
	case POWER_SUPPLY_PROP_CURRENT_AVG:
	case POWER_SUPPLY_PROP_CHARGE_OTG_CONTROL:
	case POWER_SUPPLY_PROP_CHARGE_UNO_CONTROL:
	case POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT_MAX:
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
		psy_do_property(charger->pdata->main_charger_name, get, psp, value);
		val->intval = value.intval;
		break;
	case POWER_SUPPLY_PROP_CURRENT_MAX:
		psy_do_property(charger->pdata->main_charger_name, get, psp, value);
		val->intval = value.intval;
		sec_multi_chg_check_enable(charger);
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		psy_do_property(charger->pdata->main_charger_name, get, psp, value);
		val->intval = value.intval;
		break;
	case POWER_SUPPLY_PROP_TEMP:
		break;
	case POWER_SUPPLY_PROP_MAX ... POWER_SUPPLY_EXT_PROP_MAX:
		switch (ext_psp) {
		case POWER_SUPPLY_EXT_PROP_CHIP_ID:
			psy_do_property(charger->pdata->main_charger_name, get, psp, value);
			val->intval = value.intval;
			break;
		case POWER_SUPPLY_EXT_PROP_CHECK_SLAVE_I2C:
			psy_do_property(charger->pdata->sub_charger_name, get, psp, value);
			val->intval = value.intval;
			break;
		case POWER_SUPPLY_EXT_PROP_MULTI_CHARGER_MODE:
			switch (charger->multi_mode) {
				case SEC_MULTI_CHARGER_MAIN_ONLY:
					val->strval = "master";
					break;
				case SEC_MULTI_CHARGER_SUB_ONLY:
					val->strval = "slave";
					break;
				case SEC_MULTI_CHARGER_ALL_ENABLE:
					val->strval = "dual";
					break;
				case SEC_MULTI_CHARGER_NORMAL:
					if (!charger->sub_is_charging)
						val->strval = "master"; //Main Charger Default ON;	Sub charger depend on sub_charger_condition .
					else
						val->strval = "dual";
					break;
				default:
					val->strval = "master";
					break;
			}
			break;
		case POWER_SUPPLY_EXT_PROP_WDT_STATUS:
			break;
		case POWER_SUPPLY_EXT_PROP_SUB_CURRENT_MAX:
			val->intval = charger->sub_input_current;
			pr_info("%s: EXT_PROP_SUB_CURRENT_MAX (%d)\n", __func__, val->intval);
			break;
		case POWER_SUPPLY_EXT_PROP_SUB_CURRENT_NOW:
			val->intval = charger->sub_charging_current;
			pr_info("%s: EXT_PROP_SUB_CURRENT_NOW (%d)\n", __func__, val->intval);
			break;
		default:
			return -EINVAL;
		}
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sec_multi_chg_set_property(struct power_supply *psy,
			    enum power_supply_property psp,
			    const union power_supply_propval *val)
{
	struct sec_multi_charger_info *charger = container_of(psy, struct sec_multi_charger_info, psy_chg);
	enum power_supply_ext_property ext_psp = psp;
	union power_supply_propval value = {0,};

	value.intval = val->intval;
	switch (psp) {
	case POWER_SUPPLY_PROP_CHARGING_ENABLED:
		charger->chg_mode = val->intval;
		psy_do_property(charger->pdata->main_charger_name, set,
			psp, value);

		if (val->intval != SEC_BAT_CHG_MODE_CHARGING) {
			charger->sub_is_charging = false;
			psy_do_property(charger->pdata->sub_charger_name, set,
				psp, value);
		} else if (charger->sub_is_charging) {
			psy_do_property(charger->pdata->sub_charger_name, set,
				psp, value);
		} else
			sec_multi_chg_check_enable(charger);
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		pr_info("%s: POWER_SUPPLY_PROP_ONLINE(%d))\n",__func__, val->intval);
		psy_do_property(charger->pdata->main_charger_name, set,
			psp, value);
		psy_do_property(charger->pdata->sub_charger_name, set,
			psp, value);

		/* INIT */
		if (is_nocharge_type(val->intval)) {
			charger->sub_is_charging = false;
			charger->main_current.input_current_limit = 0;
			charger->main_current.fast_charging_current = 0;
			charger->sub_current.input_current_limit = 0;
			charger->sub_current.fast_charging_current = 0;
			charger->main_input_current = 0;
			charger->main_charging_current = 0;
			charger->sub_input_current = 0;
			charger->sub_charging_current = 0;
			charger->multi_mode = SEC_MULTI_CHARGER_NORMAL;
			value.intval = SEC_BAT_CHG_MODE_CHARGING_OFF;
			psy_do_property(charger->pdata->sub_charger_name, set,
				POWER_SUPPLY_PROP_CHARGING_ENABLED, value);
		} else if (charger->sub_is_charging &&
			charger->cable_type != val->intval &&
			charger->chg_mode == SEC_BAT_CHG_MODE_CHARGING &&
			charger->multi_mode == SEC_MULTI_CHARGER_NORMAL) {
			charger->sub_is_charging = false;
			/* disable sub charger */
			value.intval = SEC_BAT_CHG_MODE_CHARGING_OFF;
			psy_do_property(charger->pdata->sub_charger_name, set,
				POWER_SUPPLY_PROP_CHARGING_ENABLED, value);
		} else {
			pr_info("%s: invalid condition (sub_is_charging(%d), chg_mode(%d), multi_mode(%d))\n",
				__func__, charger->sub_is_charging, charger->chg_mode, charger->multi_mode);
		}
		charger->cable_type = val->intval;
		break;
	case POWER_SUPPLY_PROP_STATUS:
		charger->status = val->intval;
		psy_do_property(charger->pdata->main_charger_name, set, psp, value);
		break;
	case POWER_SUPPLY_PROP_HEALTH:
	case POWER_SUPPLY_PROP_PRESENT:
		psy_do_property(charger->pdata->main_charger_name, set, psp, value);
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX:
		psy_do_property(charger->pdata->main_charger_name, set, psp, value);
#if defined(PROJECT_GTA2XLLTE) || defined(PROJECT_GTA2XLWIFI)
		/* set sub fv less than main fv */
		psy_do_property(charger->pdata->sub_charger_name, set, psp, value - 1);
#else
		psy_do_property(charger->pdata->sub_charger_name, set, psp, value);
#endif
		break;
	case POWER_SUPPLY_PROP_CURRENT_FULL:
		pr_info("%s: POWER_SUPPLY_PROP_CURRENT_FULL(%d)\n",__func__, val->intval);
		charger->full_check_current_1st = val->intval;
		psy_do_property(charger->pdata->main_charger_name, set, psp, value);
		psy_do_property(charger->pdata->sub_charger_name, set, psp, value);
		break;
	case POWER_SUPPLY_PROP_CURRENT_MAX:
		mutex_lock(&charger->charger_mutex);
		pr_info("%s: POWER_SUPPLY_PROP_CURRENT_MAX(%d)\n",__func__, val->intval);
		charger->main_current.input_current_limit = val->intval;
		mutex_unlock(&charger->charger_mutex);
		sec_multi_chg_check_enable(charger);
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		mutex_lock(&charger->charger_mutex);
		pr_info("%s: POWER_SUPPLY_PROP_CURRENT_NOW(%d)\n",__func__, val->intval);
		charger->main_current.fast_charging_current = val->intval;
		mutex_unlock(&charger->charger_mutex);
		sec_multi_chg_check_enable(charger);
		break;
	case POWER_SUPPLY_PROP_CHARGE_OTG_CONTROL:
	case POWER_SUPPLY_PROP_CHARGE_UNO_CONTROL:
		psy_do_property(charger->pdata->main_charger_name, set, psp, value);
		break;
#if defined(CONFIG_AFC_CHARGER_MODE)
	case POWER_SUPPLY_PROP_AFC_CHARGER_MODE:
		psy_do_property(charger->pdata->main_charger_name, set, psp, value);
		break;
#endif
	case POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT_MAX:
		/* AICL Enable */
		if(!charger->pdata->aicl_disable)
			psy_do_property(charger->pdata->main_charger_name, set, psp, value);
		break;
	case POWER_SUPPLY_PROP_AUTHENTIC:
	case POWER_SUPPLY_PROP_INPUT_VOLTAGE_REGULATION:
		psy_do_property(charger->pdata->main_charger_name, set, psp, value);
		break;
	case POWER_SUPPLY_PROP_MAX ... POWER_SUPPLY_EXT_PROP_MAX:
		switch (ext_psp) {
		case POWER_SUPPLY_EXT_PROP_MULTI_CHARGER_MODE:
			if (charger->chg_mode == SEC_BAT_CHG_MODE_CHARGING && charger->multi_mode != val->intval) {
#if defined(CONFIG_SEC_FACTORY)
				mutex_lock(&charger->charger_test_mutex);
#endif
				charger->multi_mode = val->intval;
				switch (val->intval) {
				case SEC_MULTI_CHARGER_MAIN_ONLY:
					pr_info("%s: Only Use Main Charger \n", __func__);
					value.intval = SEC_BAT_CHG_MODE_BUCK_OFF;
					psy_do_property(charger->pdata->sub_charger_name, set,
						POWER_SUPPLY_PROP_CHARGING_ENABLED, value);

					value.intval = charger->pdata->max_wcin_input_current;
					psy_do_property(charger->pdata->main_charger_name, set,
						POWER_SUPPLY_PROP_CURRENT_MAX, value);
					psy_do_property(charger->pdata->main_charger_name, set,
						POWER_SUPPLY_PROP_CURRENT_NOW, value);
					value.intval = SEC_BAT_CHG_MODE_CHARGING;
					psy_do_property(charger->pdata->main_charger_name, set,
						POWER_SUPPLY_PROP_CHARGING_ENABLED, value);
					break;
				case SEC_MULTI_CHARGER_SUB_ONLY:
					pr_info("%s: Only Use Sub Charger \n", __func__);
					value.intval = SEC_BAT_CHG_MODE_BUCK_OFF;
					psy_do_property(charger->pdata->main_charger_name, set,
						POWER_SUPPLY_PROP_CHARGING_ENABLED, value);

					value.intval = charger->pdata->max_sub_input_current;
					psy_do_property(charger->pdata->sub_charger_name, set,
						POWER_SUPPLY_PROP_CURRENT_MAX, value);
					value.intval = charger->pdata->max_sub_charging_current;
					psy_do_property(charger->pdata->sub_charger_name, set,
						POWER_SUPPLY_PROP_CURRENT_NOW, value);
					value.intval = SEC_BAT_CHG_MODE_CHARGING;
					psy_do_property(charger->pdata->sub_charger_name, set,
						POWER_SUPPLY_PROP_CHARGING_ENABLED, value);
					break;
				case SEC_MULTI_CHARGER_ALL_ENABLE:
					pr_info("%s: Enable Main & Sub Charger together \n", __func__);
					value.intval = SEC_BAT_CHG_MODE_CHARGING;
					psy_do_property(charger->pdata->main_charger_name, set,
						POWER_SUPPLY_PROP_CHARGING_ENABLED, value);
					psy_do_property(charger->pdata->sub_charger_name, set,
						POWER_SUPPLY_PROP_CHARGING_ENABLED, value);
					break;
				default:
					charger->multi_mode = SEC_MULTI_CHARGER_NORMAL;
					value.intval = SEC_BAT_CHG_MODE_CHARGING_OFF;
					psy_do_property(charger->pdata->sub_charger_name, set,
						POWER_SUPPLY_PROP_CHARGING_ENABLED, value);
					value.intval = SEC_BAT_CHG_MODE_CHARGING;
					psy_do_property(charger->pdata->main_charger_name, set,
						POWER_SUPPLY_PROP_CHARGING_ENABLED, value);
					break;
				}
#if defined(CONFIG_SEC_FACTORY)
				mutex_unlock(&charger->charger_test_mutex);
#endif
			}
			pr_info("%s: set Multi Charger Mode (%d)\n", __func__, charger->multi_mode);
			break;
		case POWER_SUPPLY_EXT_PROP_FACTORY_VOLTAGE_REGULATION:
		case POWER_SUPPLY_EXT_PROP_CURRENT_MEASURE:
			psy_do_property(charger->pdata->main_charger_name, set,
						ext_psp, value);
			break;
		case POWER_SUPPLY_EXT_PROP_SUB_CURRENT_NOW:
			pr_info("%s: EXT_PROP_SUB_CURRENT_NOW (%d)\n", __func__, val->intval);
			charger->sub_current.fast_charging_current = val->intval;
			break;
		default:
			return -EINVAL;
		}
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

#ifdef CONFIG_OF
static int sec_multi_charger_parse_dt(struct device *dev,
		struct sec_multi_charger_info *charger)
{
	struct device_node *np = dev->of_node;
	struct sec_multi_charger_platform_data *pdata = charger->pdata;
	int ret = 0;
	int len;
	const u32 *p;
	
	if (!np) {
		pr_err("%s: np NULL\n", __func__);
		return 1;
	} else {
		ret = of_property_read_string(np, "charger,battery_name",
				(char const **)&charger->pdata->battery_name);
		if (ret)
			pr_err("%s: battery_name is Empty\n", __func__);

		ret = of_property_read_string(np, "charger,main_charger",
				(char const **)&charger->pdata->main_charger_name);
		if (ret)
			pr_err("%s: main_charger is Empty\n", __func__);

		ret = of_property_read_string(np, "charger,sub_charger",
				(char const **)&charger->pdata->sub_charger_name);
		if (ret)
			pr_err("%s: sub_charger is Empty\n", __func__);

		pdata->aicl_disable = of_property_read_bool(np,
			"charger,aicl_disable");

		ret = of_property_read_u32(np, "charger,sub_charger_condition",
				&pdata->sub_charger_condition);
		if (ret) {
			pr_err("%s: sub_charger_condition is Empty\n", __func__);
			pdata->sub_charger_condition = 0;
		}

		if (pdata->sub_charger_condition) {
			ret = of_property_read_u32(np, "charger,sub_charger_condition_current_max",
					&pdata->sub_charger_condition_current_max);
			if (ret) {
				pr_err("%s: sub_charger_condition_current_max is Empty\n", __func__);
				pdata->sub_charger_condition &= ~SEC_SUB_CHARGER_CONDITION_CURRENT_MAX;
				pdata->sub_charger_condition_current_max = 0;
			}

			ret = of_property_read_u32(np, "charger,sub_charger_condition_charge_power",
					&pdata->sub_charger_condition_charge_power);
			if (ret) {
				pr_err("%s: sub_charger_condition_charge_power is Empty\n", __func__);
				pdata->sub_charger_condition &= ~SEC_SUB_CHARGER_CONDITION_CHARGE_POWER;
				pdata->sub_charger_condition_charge_power = 15000;
			}

			p = of_get_property(np, "charger,sub_charger_condition_online", &len);
			if (p) {
				len = len / sizeof(u32);

				pdata->sub_charger_condition_online = kzalloc(sizeof(unsigned int) * len,
								  GFP_KERNEL);
				ret = of_property_read_u32_array(np, "charger,sub_charger_condition_online",
						 pdata->sub_charger_condition_online, len);

				pdata->sub_charger_condition_online_size = len;
			} else {
				pdata->sub_charger_condition &= ~SEC_SUB_CHARGER_CONDITION_ONLINE;
				pdata->sub_charger_condition_online_size = 0;
			}

			
			ret = of_property_read_u32(np, "charger,sub_charger_condition_current_margin",
					&pdata->sub_charger_condition_current_margin);
			if (ret) {
				pr_err("%s: sub_charger_condition_current_margin is Empty\n", __func__);
				pdata->sub_charger_condition_current_margin = SEC_SUB_CHARGER_CURRENT_MARGIN;
			}

			pr_info("%s: sub_charger_condition(0x%x)\n", __func__, pdata->sub_charger_condition);
		}
		ret = of_property_read_u32(np, "charger,main_charger_condition",
				&pdata->main_charger_condition);
		if (ret) {
			pr_err("%s: main_charger_condition is Empty\n", __func__);
			pdata->main_charger_condition = 0;
		}

		ret = of_property_read_u32(np, "charger,max_wcin_input_current",
				&pdata->max_wcin_input_current);
		if (ret)
			pr_err("%s: max_wcin_input_current is Empty\n", __func__);

		ret = of_property_read_u32(np, "charger,max_sub_input_current",
				&pdata->max_sub_input_current);
		if (ret)
			pr_err("%s: max_sub_input_current is Empty\n", __func__);

		ret = of_property_read_u32(np, "charger,max_sub_charging_current",
				&pdata->max_sub_charging_current);
		if (ret)
			pr_err("%s: max_sub_charging_current is Empty\n", __func__);

		ret = of_property_read_u32(np, "charger,max_total_charging_current",
				&pdata->max_total_charging_current);
		if (ret)
			pr_err("%s: max_total_charging_current is Empty\n", __func__);
		ret = of_property_read_u32(np, "charger,siop_sub_current_max",
				&pdata->siop_sub_current_max);
		if (ret)
			pr_err("%s: siop_sub_current_max is Empty\n", __func__);
		ret = of_property_read_u32(np, "charger,siop_sub_current_min",
				&pdata->siop_sub_current_min);
		if (ret)
			pr_err("%s: siop_sub_current_min is Empty\n", __func__);
		ret = of_property_read_u32(np, "charger,pogo_wcin_sub_current",
				&pdata->pogo_wcin_sub_current);
		if (ret)
			pr_err("%s: pogo_wcin_sub_current is Empty\n", __func__);

		ret = of_property_read_u32(np, "charger,min_charging_current",
				&pdata->min_charging_current);
		if (ret) {
			pr_err("%s: min_charging_current is Empty\n", __func__);
			pdata->min_charging_current = 100;
		}
		
		pr_err("%s: siop_sub_current_max, min: %d, %d, pogo_sub: %d\n",
				__func__, pdata->siop_sub_current_max, pdata->siop_sub_current_min,
				pdata->pogo_wcin_sub_current);
	}
	return 0;
}
#endif

static int sec_multi_charger_probe(struct platform_device *pdev)
{
	struct sec_multi_charger_info *charger;
	struct sec_multi_charger_platform_data *pdata = NULL;
	int ret = 0;

	dev_info(&pdev->dev,
		"%s: SEC Multi-Charger Driver Loading\n", __func__);

	charger = kzalloc(sizeof(*charger), GFP_KERNEL);
	if (!charger)
		return -ENOMEM;

	if (pdev->dev.of_node) {
		pdata = devm_kzalloc(&pdev->dev,
				sizeof(struct sec_multi_charger_platform_data),
				GFP_KERNEL);
		if (!pdata) {
			dev_err(&pdev->dev, "Failed to allocate memory\n");
			ret = -ENOMEM;
			goto err_charger_free;
		}

		charger->pdata = pdata;
		if (sec_multi_charger_parse_dt(&pdev->dev, charger)) {
			dev_err(&pdev->dev,
				"%s: Failed to get sec-multi-charger dt\n", __func__);
			ret = -EINVAL;
			goto err_charger_free;
		}
	} else {
		pdata = dev_get_platdata(&pdev->dev);
		charger->pdata = pdata;
	}

	charger->sub_is_charging = false;
	charger->multi_mode = SEC_MULTI_CHARGER_NORMAL;
	charger->main_input_current = -1;
	charger->sub_input_current = -1;
	charger->main_charging_current = -1;
	charger->sub_charging_current = -1;

	platform_set_drvdata(pdev, charger);
	charger->dev = &pdev->dev;

	mutex_init(&charger->charger_mutex);
#if defined(CONFIG_SEC_FACTORY)
	mutex_init(&charger->charger_test_mutex);
#endif

	charger->psy_chg.name           = "sec-multi-charger";
	charger->psy_chg.type           = POWER_SUPPLY_TYPE_UNKNOWN;
	charger->psy_chg.get_property   = sec_multi_chg_get_property;
	charger->psy_chg.set_property   = sec_multi_chg_set_property;
	charger->psy_chg.properties     = sec_multi_charger_props;
	charger->psy_chg.num_properties = ARRAY_SIZE(sec_multi_charger_props);

	ret = power_supply_register(&pdev->dev, &charger->psy_chg);
	if (ret) {
		dev_err(charger->dev,
			"%s: Failed to Register psy_chg\n", __func__);
		goto err_pdata_free;
	}

	dev_info(charger->dev,
		"%s: SEC Multi-Charger Driver Loaded\n", __func__);
	return 0;

err_pdata_free:
	mutex_destroy(&charger->charger_mutex);
#if defined(CONFIG_SEC_FACTORY)
	mutex_destroy(&charger->charger_test_mutex);
#endif
	kfree(pdata);
err_charger_free:
	kfree(charger);

	return ret;
}

static int sec_multi_charger_remove(struct platform_device *pdev)
{
	struct sec_multi_charger_info *charger = platform_get_drvdata(pdev);

	power_supply_unregister(&charger->psy_chg);
	mutex_destroy(&charger->charger_mutex);
#if defined(CONFIG_SEC_FACTORY)
	mutex_destroy(&charger->charger_test_mutex);
#endif

	dev_dbg(charger->dev, "%s: End\n", __func__);

	kfree(charger->pdata);
	kfree(charger);

	return 0;
}

static int sec_multi_charger_suspend(struct device *dev)
{
	return 0;
}

static int sec_multi_charger_resume(struct device *dev)
{
	return 0;
}

static void sec_multi_charger_shutdown(struct platform_device *pdev)
{
	struct sec_multi_charger_info *charger = platform_get_drvdata(pdev);
	union power_supply_propval value = {0,};

	charger->multi_mode = SEC_MULTI_CHARGER_MAIN_ONLY;
	charger->sub_is_charging = false;

	value.intval = SEC_BAT_CHG_MODE_CHARGING_OFF;
	psy_do_property(charger->pdata->sub_charger_name, set,
		POWER_SUPPLY_PROP_CHARGING_ENABLED, value);
}

#ifdef CONFIG_OF
static struct of_device_id sec_multi_charger_dt_ids[] = {
	{ .compatible = "samsung,sec-multi-charger" },
	{ }
};
MODULE_DEVICE_TABLE(of, sec_multi_charger_dt_ids);
#endif /* CONFIG_OF */

static const struct dev_pm_ops sec_multi_charger_pm_ops = {
	.suspend = sec_multi_charger_suspend,
	.resume = sec_multi_charger_resume,
};

static struct platform_driver sec_multi_charger_driver = {
	.driver = {
		.name = "sec-multi-charger",
		.owner = THIS_MODULE,
		.pm = &sec_multi_charger_pm_ops,
#ifdef CONFIG_OF
		.of_match_table = sec_multi_charger_dt_ids,
#endif
	},
	.probe = sec_multi_charger_probe,
	.remove = sec_multi_charger_remove,
	.shutdown = sec_multi_charger_shutdown,
};

static int __init sec_multi_charger_init(void)
{
	pr_info("%s: \n", __func__);
	return platform_driver_register(&sec_multi_charger_driver);
}

static void __exit sec_multi_charger_exit(void)
{
	platform_driver_unregister(&sec_multi_charger_driver);
}

device_initcall_sync(sec_multi_charger_init);
module_exit(sec_multi_charger_exit);

MODULE_DESCRIPTION("Samsung Multi Charger Driver");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");
