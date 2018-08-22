/*
 *  sec_cisd.c
 *  Samsung Mobile Battery Driver
 *
 *  Copyright (C) 2012 Samsung Electronics
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "include/sec_battery.h"
#include "include/sec_cisd.h"

#if defined(CONFIG_SEC_ABC)
#include <linux/sti/abc_common.h>
#endif

const char *cisd_data_str[] = {
	"RESET_ALG", "ALG_INDEX", "FULL_CNT", "CAP_MAX", "CAP_MIN", "RECHARGING_CNT", "VALERT_CNT",
	"BATT_CYCLE", "WIRE_CNT", "WIRELESS_CNT", "HIGH_SWELLING_CNT", "LOW_SWELLING_CNT",
	"SWELLING_CHARGING", "SWELLING_FULL_CNT", "SWELLING_RECOVERY_CNT", "AICL_CNT", "BATT_THM_MAX",
	"BATT_THM_MIN", "CHG_THM_MAX", "CHG_THM_MIN", "WPC_THM_MAX", "WPC_THM_MIN", "USB_THM_MAX", "USB_THM_MIN",
	"CHG_BATT_THM_MAX", "CHG_BATT_THM_MIN", "CHG_CHG_THM_MAX", "CHG_CHG_THM_MIN", "CHG_WPC_THM_MAX",
	"CHG_WPC_THM_MIN", "CHG_USB_THM_MAX", "CHG_USB_THM_MIN", "USB_OVERHEAT_CHARGING", "UNSAFETY_VOLT",
	"UNSAFETY_TEMP", "SAFETY_TIMER", "VSYS_OVP", "VBAT_OVP", "USB_OVERHEAT_RAPID_CHANGE", "BUCK_OFF",
	"USB_OVERHEAT_ALONE", "DROP_SENSOR"
};
const char *cisd_data_str_d[] = {
	"FULL_CNT_D", "CAP_MAX_D", "CAP_MIN_D", "RECHARGING_CNT_D", "VALERT_CNT_D", "WIRE_CNT_D", "WIRELESS_CNT_D",
	"HIGH_SWELLING_CNT_D", "LOW_SWELLING_CNT_D", "SWELLING_CHARGING_D", "SWELLING_FULL_CNT_D",
	"SWELLING_RECOVERY_CNT_D", "AICL_CNT_D", "BATT_THM_MAX_D",	"BATT_THM_MIN_D", "CHG_THM_MAX_D",
	"CHG_THM_MIN_D", "WPC_THM_MAX_D", "WPC_THM_MIN_D", "USB_THM_MAX_D", "USB_THM_MIN_D",
	"CHG_BATT_THM_MAX_D", "CHG_BATT_THM_MIN_D", "CHG_CHG_THM_MAX_D", "CHG_CHG_THM_MIN_D",
	"CHG_WPC_THM_MAX_D", "CHG_WPC_THM_MIN_D", "CHG_USB_THM_MAX_D", "CHG_USB_THM_MIN_D",
	"USB_OVERHEAT_CHARGING_D", "UNSAFETY_VOLT_D", "UNSAFETY_TEMP_D", "SAFETY_TIMER_D", "VSYS_OVP_D",
	"VBAT_OVP_D", "USB_OVERHEAT_RAPID_CHANGE_D", "BUCK_OFF_D", "USB_OVERHEAT_ALONE_D", "DROP_SENSOR_D"
};

const char *cisd_wc_data_str[] = {"INDEX", "SNGL_NOBLE", "SNGL_VEHICLE", "SNGL_MINI", "SNGL_ZERO", "SNGL_DREAM",
	"STAND_HERO", "STAND_DREAM", "EXT_PACK", "EXT_PACK_TA"};

bool sec_bat_cisd_check(struct sec_battery_info *battery)
{
	union power_supply_propval incur_val = {0, };
	union power_supply_propval chgcur_val = {0, };
	union power_supply_propval capcurr_val = {0, };
	union power_supply_propval vbat_val = {0, };
	struct cisd *pcisd = &battery->cisd;
	bool ret = false;

	if (battery->factory_mode || battery->is_jig_on || battery->skip_cisd) {
		dev_dbg(battery->dev, "%s: No need to check in factory mode\n",
			__func__);
		return ret;
	}

	if ((battery->status == POWER_SUPPLY_STATUS_CHARGING) ||
		(battery->status == POWER_SUPPLY_STATUS_FULL)) {

		/* check abnormal vbat */
		pcisd->ab_vbat_check_count = battery->voltage_now > pcisd->max_voltage_thr ?
				pcisd->ab_vbat_check_count + 1 : 0;

		if ((pcisd->ab_vbat_check_count >= pcisd->ab_vbat_max_count) &&
			!(pcisd->state & CISD_STATE_OVER_VOLTAGE)) {
			dev_info(battery->dev, "%s : [CISD] Battery Over Voltage Protction !! vbat(%d)mV\n",
				 __func__, battery->voltage_now);
			vbat_val.intval = true;
			psy_do_property("battery", set, POWER_SUPPLY_EXT_PROP_VBAT_OVP,
					vbat_val);
			pcisd->data[CISD_DATA_VBAT_OVP]++;
			pcisd->data[CISD_DATA_VBAT_OVP_PER_DAY]++;
			pcisd->state |= CISD_STATE_OVER_VOLTAGE;
#if defined(CONFIG_SEC_ABC)
			sec_abc_send_event("MODULE=battery@ERROR=over_voltage");
#endif
		}

		/* get actual input current */
		psy_do_property(battery->pdata->charger_name, get,
			POWER_SUPPLY_PROP_CURRENT_AVG, incur_val);

		/* get actual charging current */
		psy_do_property(battery->pdata->charger_name, get,
			POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT, chgcur_val);

		if (battery->temperature > pcisd->data[CISD_DATA_CHG_BATT_TEMP_MAX])
			pcisd->data[CISD_DATA_CHG_BATT_TEMP_MAX] = battery->temperature;
		if (battery->temperature < pcisd->data[CISD_DATA_CHG_BATT_TEMP_MIN])
			pcisd->data[CISD_DATA_CHG_BATT_TEMP_MIN] = battery->temperature;

		if (battery->chg_temp > pcisd->data[CISD_DATA_CHG_CHG_TEMP_MAX])
			pcisd->data[CISD_DATA_CHG_CHG_TEMP_MAX] = battery->chg_temp;
		if (battery->chg_temp < pcisd->data[CISD_DATA_CHG_CHG_TEMP_MIN])
			pcisd->data[CISD_DATA_CHG_CHG_TEMP_MIN] = battery->chg_temp;

		if (battery->wpc_temp > pcisd->data[CISD_DATA_CHG_WPC_TEMP_MAX])
			pcisd->data[CISD_DATA_CHG_WPC_TEMP_MAX] = battery->wpc_temp;
		if (battery->wpc_temp < pcisd->data[CISD_DATA_CHG_WPC_TEMP_MIN])
			pcisd->data[CISD_DATA_CHG_WPC_TEMP_MIN] = battery->wpc_temp;

		if (battery->usb_temp > pcisd->data[CISD_DATA_CHG_USB_TEMP_MAX])
			pcisd->data[CISD_DATA_CHG_USB_TEMP_MAX] = battery->usb_temp;
		if (battery->usb_temp < pcisd->data[CISD_DATA_CHG_USB_TEMP_MIN])
			pcisd->data[CISD_DATA_CHG_USB_TEMP_MIN] = battery->usb_temp;

		if (battery->temperature > pcisd->data[CISD_DATA_CHG_BATT_TEMP_MAX_PER_DAY])
			pcisd->data[CISD_DATA_CHG_BATT_TEMP_MAX_PER_DAY] = battery->temperature;
		if (battery->temperature < pcisd->data[CISD_DATA_BATT_TEMP_MIN_PER_DAY])
			pcisd->data[CISD_DATA_BATT_TEMP_MIN_PER_DAY] = battery->temperature;

		if (battery->chg_temp > pcisd->data[CISD_DATA_CHG_CHG_TEMP_MAX_PER_DAY])
			pcisd->data[CISD_DATA_CHG_CHG_TEMP_MAX_PER_DAY] = battery->chg_temp;
		if (battery->chg_temp < pcisd->data[CISD_DATA_CHG_CHG_TEMP_MIN_PER_DAY])
			pcisd->data[CISD_DATA_CHG_CHG_TEMP_MIN_PER_DAY] = battery->chg_temp;

		if (battery->wpc_temp > pcisd->data[CISD_DATA_CHG_WPC_TEMP_MAX_PER_DAY])
			pcisd->data[CISD_DATA_CHG_WPC_TEMP_MAX_PER_DAY] = battery->wpc_temp;
		if (battery->wpc_temp < pcisd->data[CISD_DATA_CHG_WPC_TEMP_MIN_PER_DAY])
			pcisd->data[CISD_DATA_CHG_WPC_TEMP_MIN_PER_DAY] = battery->wpc_temp;

		if (battery->usb_temp > pcisd->data[CISD_DATA_CHG_USB_TEMP_MAX_PER_DAY])
			pcisd->data[CISD_DATA_CHG_USB_TEMP_MAX_PER_DAY] = battery->usb_temp;
		if (battery->usb_temp < pcisd->data[CISD_DATA_CHG_USB_TEMP_MIN_PER_DAY])
			pcisd->data[CISD_DATA_CHG_USB_TEMP_MIN_PER_DAY] = battery->usb_temp;

		if (battery->usb_temp > 800 && !battery->usb_overheat_check) {
			battery->cisd.data[CISD_DATA_USB_OVERHEAT_CHARGING]++;
			battery->cisd.data[CISD_DATA_USB_OVERHEAT_CHARGING_PER_DAY]++;
			battery->usb_overheat_check = true;
		}

		dev_info(battery->dev, "%s: [CISD] iavg: %d, incur: %d, chgcur: %d,\n"
			"cc_T: %ld, lcd_off_T: %ld, passed_T: %ld, full_T: %ld, chg_end_T: %ld, cisd: 0x%x\n", __func__,
			battery->current_avg, incur_val.intval, chgcur_val.intval,
			pcisd->cc_start_time, pcisd->lcd_off_start_time, battery->charging_passed_time,
			battery->charging_fullcharged_time, pcisd->charging_end_time, pcisd->state);
	} else  {
		/* discharging */
		if (battery->status == POWER_SUPPLY_STATUS_NOT_CHARGING) {
			/* check abnormal vbat */
			pcisd->ab_vbat_check_count = battery->voltage_now > pcisd->max_voltage_thr ?
				pcisd->ab_vbat_check_count + 1 : 0;

			if ((pcisd->ab_vbat_check_count >= pcisd->ab_vbat_max_count) &&
					!(pcisd->state & CISD_STATE_OVER_VOLTAGE)) {
				pcisd->data[CISD_DATA_VBAT_OVP]++;
				pcisd->data[CISD_DATA_VBAT_OVP_PER_DAY]++;
				pcisd->state |= CISD_STATE_OVER_VOLTAGE;
#if defined(CONFIG_SEC_ABC)
				sec_abc_send_event("MODULE=battery@ERROR=over_voltage");
#endif
			}
		}

		capcurr_val.intval = SEC_BATTERY_CAPACITY_FULL;
		psy_do_property(battery->pdata->fuelgauge_name, get,
			POWER_SUPPLY_PROP_ENERGY_NOW, capcurr_val);
		if (capcurr_val.intval == -1) {
			dev_info(battery->dev, "%s: [CISD] FG I2C fail. skip cisd check \n", __func__);
			return ret;
		}

		if (capcurr_val.intval > pcisd->data[CISD_DATA_CAP_MAX])
			pcisd->data[CISD_DATA_CAP_MAX] = capcurr_val.intval;
		if (capcurr_val.intval < pcisd->data[CISD_DATA_CAP_MIN])
			pcisd->data[CISD_DATA_CAP_MIN] = capcurr_val.intval;

		if (capcurr_val.intval > pcisd->data[CISD_DATA_CAP_MAX_PER_DAY])
			pcisd->data[CISD_DATA_CAP_MAX_PER_DAY] = capcurr_val.intval;
		if (capcurr_val.intval < pcisd->data[CISD_DATA_CAP_MIN_PER_DAY])
			pcisd->data[CISD_DATA_CAP_MIN_PER_DAY] = capcurr_val.intval;
	}

	if (battery->temperature > pcisd->data[CISD_DATA_BATT_TEMP_MAX])
		pcisd->data[CISD_DATA_BATT_TEMP_MAX] = battery->temperature;
	if (battery->temperature < battery->cisd.data[CISD_DATA_BATT_TEMP_MIN])
		pcisd->data[CISD_DATA_BATT_TEMP_MIN] = battery->temperature;

	if (battery->chg_temp > pcisd->data[CISD_DATA_CHG_TEMP_MAX])
		pcisd->data[CISD_DATA_CHG_TEMP_MAX] = battery->chg_temp;
	if (battery->chg_temp < pcisd->data[CISD_DATA_CHG_TEMP_MIN])
		pcisd->data[CISD_DATA_CHG_TEMP_MIN] = battery->chg_temp;

	if (battery->wpc_temp > pcisd->data[CISD_DATA_WPC_TEMP_MAX])
		pcisd->data[CISD_DATA_WPC_TEMP_MAX] = battery->wpc_temp;
	if (battery->wpc_temp < battery->cisd.data[CISD_DATA_WPC_TEMP_MIN])
		pcisd->data[CISD_DATA_WPC_TEMP_MIN] = battery->wpc_temp;

	if (battery->usb_temp > pcisd->data[CISD_DATA_USB_TEMP_MAX])
		pcisd->data[CISD_DATA_USB_TEMP_MAX] = battery->usb_temp;
	if (battery->usb_temp < pcisd->data[CISD_DATA_USB_TEMP_MIN])
		pcisd->data[CISD_DATA_USB_TEMP_MIN] = battery->usb_temp;

	if (battery->temperature > pcisd->data[CISD_DATA_BATT_TEMP_MAX_PER_DAY])
		pcisd->data[CISD_DATA_BATT_TEMP_MAX_PER_DAY] = battery->temperature;
	if (battery->temperature < pcisd->data[CISD_DATA_BATT_TEMP_MIN_PER_DAY])
		pcisd->data[CISD_DATA_BATT_TEMP_MIN_PER_DAY] = battery->temperature;

	if (battery->chg_temp > pcisd->data[CISD_DATA_CHG_TEMP_MAX_PER_DAY])
		pcisd->data[CISD_DATA_CHG_TEMP_MAX_PER_DAY] = battery->chg_temp;
	if (battery->chg_temp < pcisd->data[CISD_DATA_CHG_TEMP_MIN_PER_DAY])
		pcisd->data[CISD_DATA_CHG_TEMP_MIN_PER_DAY] = battery->chg_temp;

	if (battery->wpc_temp > pcisd->data[CISD_DATA_WPC_TEMP_MAX_PER_DAY])
		pcisd->data[CISD_DATA_WPC_TEMP_MAX_PER_DAY] = battery->wpc_temp;
	if (battery->wpc_temp < pcisd->data[CISD_DATA_WPC_TEMP_MIN_PER_DAY])
		pcisd->data[CISD_DATA_WPC_TEMP_MIN_PER_DAY] = battery->wpc_temp;

	if (battery->usb_temp > pcisd->data[CISD_DATA_USB_TEMP_MAX_PER_DAY])
		pcisd->data[CISD_DATA_USB_TEMP_MAX_PER_DAY] = battery->usb_temp;
	if (battery->usb_temp < pcisd->data[CISD_DATA_USB_TEMP_MIN_PER_DAY])
		pcisd->data[CISD_DATA_USB_TEMP_MIN_PER_DAY] = battery->usb_temp;

	return ret;
}

struct cisd *gcisd;
void sec_battery_cisd_init(struct sec_battery_info *battery)
{
	union power_supply_propval capfull_val;

	battery->cisd.state = CISD_STATE_NONE;

	battery->cisd.delay_time = 600; /* 10 min */
	battery->cisd.diff_volt_now = 40;
	battery->cisd.diff_cap_now = 5;

	capfull_val.intval = SEC_BATTERY_CAPACITY_FULL;
	psy_do_property(battery->pdata->fuelgauge_name, get,
		POWER_SUPPLY_PROP_ENERGY_NOW, capfull_val);
	battery->cisd.curr_cap_max = capfull_val.intval;
	battery->cisd.err_cap_high_thr = battery->pdata->cisd_cap_high_thr;
	battery->cisd.err_cap_low_thr = battery->pdata->cisd_cap_low_thr;
	battery->cisd.cc_delay_time = 3600; /* 60 min */
	battery->cisd.lcd_off_delay_time = 10200; /* 230 min */
	battery->cisd.full_delay_time = 3600; /* 60 min */
	battery->cisd.recharge_delay_time = 9000; /* 150 min */
	battery->cisd.cc_start_time = 0;
	battery->cisd.full_start_time = 0;
	battery->cisd.lcd_off_start_time = 0;
	battery->cisd.overflow_start_time = 0;
	battery->cisd.charging_end_time = 0;
	battery->cisd.charging_end_time_2 = 0;
	battery->cisd.recharge_count = 0;
	battery->cisd.recharge_count_2 = 0;
	battery->cisd.recharge_count_thres = 2;
	battery->cisd.leakage_e_time = 3600; /* 60 min */
	battery->cisd.leakage_f_time = 7200; /* 120 min */
	battery->cisd.leakage_g_time = 14400; /* 240 min */
	battery->cisd.current_max_thres = 1600;
	battery->cisd.charging_current_thres = 1000;
	battery->cisd.current_avg_thres = 1000;

	battery->cisd.data[CISD_DATA_ALG_INDEX] = battery->pdata->cisd_alg_index;
	battery->cisd.data[CISD_DATA_FULL_COUNT] = 1;
	battery->cisd.data[CISD_DATA_BATT_TEMP_MAX] = -300;
	battery->cisd.data[CISD_DATA_CHG_TEMP_MAX] = -300;
	battery->cisd.data[CISD_DATA_WPC_TEMP_MAX] = -300;
	battery->cisd.data[CISD_DATA_BATT_TEMP_MIN] = 1000;
	battery->cisd.data[CISD_DATA_CHG_TEMP_MIN] = 1000;
	battery->cisd.data[CISD_DATA_WPC_TEMP_MIN] = 1000;
	battery->cisd.data[CISD_DATA_CAP_MIN] = 0xFFFF;

	battery->cisd.data[CISD_DATA_FULL_COUNT_PER_DAY] = 1;
	battery->cisd.data[CISD_DATA_BATT_TEMP_MAX_PER_DAY] = -300;
	battery->cisd.data[CISD_DATA_CHG_TEMP_MAX_PER_DAY] = -300;
	battery->cisd.data[CISD_DATA_WPC_TEMP_MAX_PER_DAY] = -300;
	battery->cisd.data[CISD_DATA_BATT_TEMP_MIN_PER_DAY] = 1000;
	battery->cisd.data[CISD_DATA_CHG_TEMP_MIN_PER_DAY] = 1000;
	battery->cisd.data[CISD_DATA_WPC_TEMP_MIN_PER_DAY] = 1000;
	battery->cisd.data[CISD_DATA_CAP_MIN] = 0xFFFF;

	battery->cisd.data[CISD_DATA_CHG_BATT_TEMP_MAX_PER_DAY] = -300;
	battery->cisd.data[CISD_DATA_CHG_CHG_TEMP_MAX_PER_DAY] = -300;
	battery->cisd.data[CISD_DATA_CHG_WPC_TEMP_MAX_PER_DAY] = -300;
	battery->cisd.data[CISD_DATA_CHG_BATT_TEMP_MIN_PER_DAY] = 1000;
	battery->cisd.data[CISD_DATA_CHG_CHG_TEMP_MIN_PER_DAY] = 1000;
	battery->cisd.data[CISD_DATA_CHG_WPC_TEMP_MIN_PER_DAY] = 1000;

	battery->cisd.capacity_now = capfull_val.intval;
	battery->cisd.overflow_cap_thr = capfull_val.intval > battery->pdata->cisd_cap_limit ?
		capfull_val.intval : battery->pdata->cisd_cap_limit;

	battery->cisd.ab_vbat_max_count = 2; /* should be 1 */
	battery->cisd.ab_vbat_check_count = 0;
	battery->cisd.max_voltage_thr = battery->pdata->max_voltage_thr;
	battery->cisd.cisd_alg_index = 6;
	pr_info("%s: cisd.err_cap_high_thr:%d, cisd.err_cap_low_thr:%d, cisd.overflow_cap_thr:%d\n", __func__,
		battery->cisd.err_cap_high_thr, battery->cisd.err_cap_low_thr, battery->cisd.overflow_cap_thr);

	/* set cisd pointer */
	gcisd = &battery->cisd;
}
