/*
 *  sec_board-8084.c
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

#include <linux/qpnp/pin.h>
#include <linux/qpnp/qpnp-adc.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/krait-regulator.h>

#define SHORT_BATTERY_STANDARD      100

#if defined(CONFIG_EXTCON)
extern int get_jig_state(void);
#endif

int current_cable_type = POWER_SUPPLY_TYPE_BATTERY;

static struct qpnp_vadc_chip *adc_client;
static enum qpnp_vadc_channels temp_channel;
static enum qpnp_vadc_channels chg_temp_channel;


void sec_bat_check_batt_id(struct sec_battery_info *battery)
{

}

static void sec_bat_adc_ap_init(struct platform_device *pdev,
			struct sec_battery_info *battery)
{
	if (!battery->dev) {
		pr_err("%s : can't get battery dev \n", __func__);
	} else {
		adc_client = qpnp_get_vadc(battery->dev, "sec-battery");
		if (IS_ERR(adc_client)) {
			int rc;
			rc = PTR_ERR(adc_client);
			if (rc != -EPROBE_DEFER)
				pr_err("%s: Fail to get vadc %d\n", __func__, rc);
		}
	}

#if defined(CONFIG_PROJECT_GTS28VE) || defined(CONFIG_PROJECT_GTS210VE) || defined(CONFIG_SEC_ELITELTE_PROJECT)
	temp_channel = P_MUX4_1_1;
#else
	temp_channel = P_MUX2_1_1;
#endif

	if (battery->pdata->chg_temp_check)
#if defined(CONFIG_PROJECT_GTS28VE) || defined(CONFIG_PROJECT_GTS210VE)
		chg_temp_channel = P_MUX2_1_1;
#else
		chg_temp_channel = P_MUX4_1_1;
#endif
}

static int sec_bat_adc_ap_read(struct sec_battery_info *battery, int channel)
{
	struct qpnp_vadc_result results;
	int data = -1;
	int rc;

	switch (channel)
	{
	case SEC_BAT_ADC_CHANNEL_TEMP :

		rc = qpnp_vadc_read(adc_client, temp_channel, &results);
		if (rc) {
			pr_err("%s: Unable to read batt temperature rc=%d, temp_channel=%d\n",
				__func__, rc, temp_channel);
			return 33000;
		}
		data = results.adc_code;
		break;
	case SEC_BAT_ADC_CHANNEL_TEMP_AMBIENT:
		data = 33000;
		break;
	case SEC_BAT_ADC_CHANNEL_BAT_CHECK :
		break;
	case SEC_BAT_ADC_CHANNEL_CHG_TEMP:
		rc = qpnp_vadc_read(adc_client, chg_temp_channel, &results);
		if (rc) {
			pr_err("%s: Unable to read chg temperature rc=%d\n",
				__func__, rc);
			return 33000;
		}
		data = results.adc_code;
		break;
	default :
		break;
	}

	pr_debug("%s: data(%d)\n", __func__, data);

	return data;
}

static void sec_bat_adc_ap_exit(void)
{
}

static void sec_bat_adc_none_init(struct platform_device *pdev,
			struct sec_battery_info *battery)
{
}

static int sec_bat_adc_none_read(struct sec_battery_info *battery, int channel)
{
	return 0;
}

static void sec_bat_adc_none_exit(void)
{
}

static void sec_bat_adc_ic_init(struct platform_device *pdev,
			struct sec_battery_info *battery)
{
}

static int sec_bat_adc_ic_read(struct sec_battery_info *battery, int channel)
{
	return 0;
}

static void sec_bat_adc_ic_exit(void)
{
}
static int adc_read_type(struct sec_battery_info *battery, int channel)
{
	int adc = 0;

	switch (battery->pdata->temp_adc_type)
	{
	case SEC_BATTERY_ADC_TYPE_NONE :
		adc = sec_bat_adc_none_read(battery, channel);
		break;
	case SEC_BATTERY_ADC_TYPE_AP :
		adc = sec_bat_adc_ap_read(battery, channel);
		break;
	case SEC_BATTERY_ADC_TYPE_IC :
		adc = sec_bat_adc_ic_read(battery, channel);
		break;
	case SEC_BATTERY_ADC_TYPE_NUM :
		break;
	default :
		break;
	}
	pr_debug("[%s] ADC = %d\n", __func__, adc);
	return adc;
}

static void adc_init_type(struct platform_device *pdev,
			struct sec_battery_info *battery)
{
	switch (battery->pdata->temp_adc_type)
	{
	case SEC_BATTERY_ADC_TYPE_NONE :
		sec_bat_adc_none_init(pdev, battery);
		break;
	case SEC_BATTERY_ADC_TYPE_AP :
		sec_bat_adc_ap_init(pdev, battery);
		break;
	case SEC_BATTERY_ADC_TYPE_IC :
		sec_bat_adc_ic_init(pdev, battery);
		break;
	case SEC_BATTERY_ADC_TYPE_NUM :
		break;
	default :
		break;
	}
}

static void adc_exit_type(struct sec_battery_info *battery)
{
	switch (battery->pdata->temp_adc_type)
	{
	case SEC_BATTERY_ADC_TYPE_NONE :
		sec_bat_adc_none_exit();
		break;
	case SEC_BATTERY_ADC_TYPE_AP :
		sec_bat_adc_ap_exit();
		break;
	case SEC_BATTERY_ADC_TYPE_IC :
		sec_bat_adc_ic_exit();
		break;
	case SEC_BATTERY_ADC_TYPE_NUM :
		break;
	default :
		break;
	}
}

int adc_read(struct sec_battery_info *battery, int channel)
{
	int adc = 0;

	adc = adc_read_type(battery, channel);

	pr_debug("[%s]adc = %d\n", __func__, adc);

	return adc;
}

void adc_exit(struct sec_battery_info *battery)
{
	adc_exit_type(battery);
}

bool sec_bat_check_jig_status(void)
{
	return false;
}

/* callback for battery check
 * return : bool
 * true - battery detected, false battery NOT detected
 */
bool sec_bat_check_callback(struct sec_battery_info *battery)
{

return true;
}
void sec_bat_check_cable_result_callback(struct device *dev,
		int cable_type)
{

}

int sec_bat_check_cable_callback(struct sec_battery_info *battery)
{
	return POWER_SUPPLY_TYPE_BATTERY;
}

void board_battery_init(struct platform_device *pdev, struct sec_battery_info *battery)
{
	adc_init_type(pdev, battery);
}

void board_fuelgauge_init(void *data)
{
}

void cable_initial_check(struct sec_battery_info *battery)
{
	union power_supply_propval value;

	pr_info("%s : current_cable_type : (%d)\n", __func__, current_cable_type);
	if (POWER_SUPPLY_TYPE_BATTERY != current_cable_type) {
		value.intval = current_cable_type;
		psy_do_property("battery", set,
				POWER_SUPPLY_PROP_ONLINE, value);
	} else {
		psy_do_property(battery->pdata->charger_name, get,
				POWER_SUPPLY_PROP_ONLINE, value);
		if (value.intval == POWER_SUPPLY_TYPE_WIRELESS) {
			value.intval = 1;
			psy_do_property("wireless", set,
					POWER_SUPPLY_PROP_ONLINE, value);
		}
	}
}

