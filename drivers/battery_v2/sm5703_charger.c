/* drivers/battery/sm5703_charger.c
 * SM5703 Charger Driver
 *
 * Copyright (C) 2013 Siliconmitus Technology Corp.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "include/charger/sm5703_charger.h"
#include <linux/wakelock.h>

#ifdef CONFIG_SM5703_MUIC
#include <linux/i2c/sm5703-muic.h>
#endif

#include <linux/mfd/sm5703.h>

#ifdef CONFIG_FLED_SM5703
#include <linux/leds/sm5703_fled.h>
#include <linux/leds/smfled.h>
#endif

#include <linux/version.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/of_gpio.h>

#ifdef CONFIG_USB_HOST_NOTIFY
#include <linux/usb_notify.h>
#endif

static enum power_supply_property sec_charger_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_CHARGE_TYPE,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_CURRENT_MAX,
	POWER_SUPPLY_PROP_CURRENT_AVG,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_CHARGE_OTG_CONTROL,
	POWER_SUPPLY_PROP_CHARGING_ENABLED,
};

static enum power_supply_property sm5703_otg_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

int otg_enable_flag;

static int sm5703_get_charging_health(
		struct sm5703_charger_data *charger);

static void sm5703_read_regs(struct i2c_client *i2c, char *str)
{
	u8 data = 0;
	int i = 0;
	for (i = SM5703_INTMSK1; i < ARRAY_SIZE(sm5703_reg_map); i++) {
		data = sm5703_reg_read(i2c, sm5703_reg_map[i]);
		sprintf(str+strlen(str), "0x%02x, ", data);
	}
}

static void sm5703_test_read(struct i2c_client *i2c)
{
	int data;
	char str[1000] = {0,};
	int i;

	/* SM5703 REG: 0x04 ~ 0x13 */
	for (i = SM5703_INTMSK1; i <= SM5703_CHGCNTL6; i++) {
		data = sm5703_reg_read(i2c, i);
		sprintf(str+strlen(str), "0x%0x = 0x%02x, ", i, data);
	}

	sprintf(str+strlen(str), "0x%0x = 0x%02x, ",SM5703_OTGCURRENTCNTL,
		sm5703_reg_read(i2c, SM5703_OTGCURRENTCNTL));
	sprintf(str+strlen(str), "0x%0x = 0x%02x, ", SM5703_STATUS5,
		sm5703_reg_read(i2c, SM5703_STATUS5));
	sprintf(str+strlen(str), "0x%0x = 0x%02x, ", SM5703_Q3LIMITCNTL,
		sm5703_reg_read(i2c, SM5703_Q3LIMITCNTL));
	pr_info("%s: %s\n", __func__, str);
}

#define SM5703_FLEDCNTL6			0x19
static void sm5703_charger_otg_control(struct sm5703_charger_data *charger,
		bool enable)
{
	pr_info("%s: called charger otg control : %s\n", __func__,
			enable ? "on" : "off");

	otg_enable_flag = enable;

	if (!enable) {
		sm5703_assign_bits(charger->i2c,
			SM5703_FLEDCNTL6, SM5703_BSTOUT_MASK,
			SM5703_BSTOUT_4P5);
#ifdef CONFIG_FLED_SM5703
		/* turn off OTG */
		if (charger->fled_info == NULL)
			charger->fled_info = sm_fled_get_info_by_name(NULL);
		if (charger->fled_info)
			sm5703_boost_notification(charger->fled_info, 0);
#else
		sm5703_assign_bits(charger->i2c,
			SM5703_CNTL, SM5703_OPERATION_MODE_MASK,
			SM5703_OPERATION_MODE_CHARGING_ON);
#endif
	} else {
		sm5703_assign_bits(charger->i2c,
			SM5703_FLEDCNTL6, SM5703_BSTOUT_MASK,
			SM5703_BSTOUT_5P0);
#ifdef CONFIG_FLED_SM5703
		if (charger->fled_info == NULL)
			charger->fled_info = sm_fled_get_info_by_name(NULL);
		if (charger->fled_info)
			sm5703_boost_notification(charger->fled_info, 1);
#else
		sm5703_assign_bits(charger->i2c,
			SM5703_CNTL, SM5703_OPERATION_MODE_MASK,
			SM5703_OPERATION_MODE_USB_OTG_MODE);
#endif
		charger->cable_type = SEC_BATTERY_CABLE_OTG;
	}
}

static void sm5703_charger_vbus_control(struct sm5703_charger_data *charger,
		int onoff)
{
	if (onoff > 0) {
		pr_info("%s: enable vbus\n", __func__);
#ifdef CONFIG_FLED_SM5703
		if (charger->fled_info == NULL)
			charger->fled_info = sm_fled_get_info_by_name(NULL);
		if (charger->fled_info)
			sm5703_charger_notification(charger->fled_info,1);
#else
		sm5703_assign_bits(charger->i2c,
			SM5703_CNTL, SM5703_OPERATION_MODE_MASK,
			SM5703_OPERATION_MODE_CHARGING_ON);
#endif
	} else {
		pr_info("%s: disable vbus\n", __func__);

#ifdef CONFIG_FLED_SM5703
		if (charger->fled_info == NULL)
			charger->fled_info = sm_fled_get_info_by_name(NULL);
		if (charger->fled_info)
			sm5703_charger_notification(charger->fled_info,0);
#endif
	}
}

static void sm5703_enable_charger_switch(struct sm5703_charger_data *charger,
		int onoff)
{
#ifdef CONFIG_CHARGER_SM5703_DUALPATH
	union power_supply_propval batt_pres;
#endif

	if (onoff > 0){
		pr_info("%s: turn on charger\n", __func__);

		charger->nchgen = false;
		gpio_direction_output(charger->pdata->chgen_gpio,
			charger->nchgen); /* nCHG enable */

		pr_info("%s : STATUS OF CHARGER ON(0)/OFF(1): %d\n",
			__func__, charger->nchgen);
	} else{
		pr_info("%s: turn off charger\n", __func__);

		charger->nchgen = true;
		gpio_direction_output(charger->pdata->chgen_gpio,
			charger->nchgen); /* nCHG disable */
		pr_info("%s : STATUS OF CHARGER ON(0)/OFF(1): %d\n",
			__func__, charger->nchgen);

#ifdef CONFIG_CHARGER_SM5703_DUALPATH
		psy_do_property("battery", get,
					POWER_SUPPLY_PROP_PRESENT, batt_pres);
		if(batt_pres.intval == false) {
			sm5703_assign_bits(charger->i2c,
					SM5703_CNTL, SM5703_OPERATION_MODE_MASK,
					SM5703_OPERATION_MODE_SUSPEND);
			pr_info("%s: Set SM5703 mode to Suspend, W/O for VF check \n", __func__);
		}
#endif
	}
}

static int sm5703_set_topoff_timer(struct sm5703_charger_data *charger,
		unsigned char topoff_timer)
{
	struct i2c_client *i2c = charger->i2c;

	sm5703_assign_bits(i2c,
		SM5703_CHGCNTL5, SM5703_TOPOFF_TIMER_MASK,
		((topoff_timer & SM5703_TOPOFF_TIMER) << SM5703_TOPOFF_TIMER_SHIFT));
	pr_info("TOPOFF_TMR set (timer=%d)\n", topoff_timer);

	return 0;
}

static void sm5703_enable_autostop(struct sm5703_charger_data *charger,
		int onoff)
{
	pr_info("%s:[BATT] Autostop set(%d)\n", __func__, onoff);

	mutex_lock(&charger->io_lock);

	if (onoff)
		sm5703_set_bits(charger->i2c, SM5703_CHGCNTL4, SM5703_AUTOSTOP_MASK);
	else
		sm5703_clr_bits(charger->i2c, SM5703_CHGCNTL4, SM5703_AUTOSTOP_MASK);

	mutex_unlock(&charger->io_lock);    
}

static void sm5703_enable_autoset(struct sm5703_charger_data *charger,
		int onoff)
{
	pr_info("%s:[BATT] Autoset set(%d)\n", __func__, onoff);

	mutex_lock(&charger->io_lock);

	if (onoff)
		sm5703_set_bits(charger->i2c, SM5703_CNTL, SM5703_AUTOSET_MASK);
	else
		sm5703_clr_bits(charger->i2c, SM5703_CNTL, SM5703_AUTOSET_MASK);

	mutex_unlock(&charger->io_lock);
}

static void sm5703_enable_aiclen(struct sm5703_charger_data *charger,
		int onoff)
{
	pr_info("%s:[BATT] AICLEN set(%d)\n", __func__, onoff);

	mutex_lock(&charger->io_lock);

	if (onoff)
		sm5703_set_bits(charger->i2c, SM5703_CHGCNTL5, SM5703_AICLEN_MASK);
	else
		sm5703_clr_bits(charger->i2c, SM5703_CHGCNTL5, SM5703_AICLEN_MASK);

	mutex_unlock(&charger->io_lock);    
}

static void sm5703_set_aiclth(struct sm5703_charger_data *charger,
		int aiclth)
{
	int data = 0, temp = 0;

	mutex_lock(&charger->io_lock);
	data = sm5703_reg_read(charger->i2c, SM5703_CHGCNTL5);
	data &= ~SM5703_AICLTH;

	if (aiclth >= 4900)
		aiclth = 4900;

	if(aiclth <= 4300)
		data &= ~SM5703_AICLTH;
	else {
		temp = (aiclth - 4300)/100;
		data |= temp;
	}

	sm5703_reg_write(charger->i2c, SM5703_CHGCNTL5, data);

	data = sm5703_reg_read(charger->i2c, SM5703_CHGCNTL5);
	pr_info("%s : SM5703_CHGCNTL5 (AICHTH) : 0x%02x\n",
			__func__, data);    
	mutex_unlock(&charger->io_lock);
}

static void sm5703_set_freqsel(struct sm5703_charger_data *charger,
		int freqsel_hz)
{
	int data = 0;

	mutex_lock(&charger->io_lock);
	data = sm5703_reg_read(charger->i2c, SM5703_CHGCNTL6);
	data &= ~SM5703_FREQSEL_MASK;
	data |= (freqsel_hz << SM5703_FREQSEL_SHIFT);

	sm5703_reg_write(charger->i2c, SM5703_CHGCNTL6, data);

	data = sm5703_reg_read(charger->i2c, SM5703_CHGCNTL6);
	pr_info("%s : SM5703_CHGCNTL6 (FREQSEL) : 0x%02x\n",
			__func__, data);    
	mutex_unlock(&charger->io_lock);
}

static void sm5703_set_input_current_limit(struct sm5703_charger_data *charger,
		int current_limit)
{
	int data = 0, temp = 0;

	mutex_lock(&charger->io_lock);
	data = sm5703_reg_read(charger->i2c, SM5703_VBUSCNTL);
	data &= ~SM5703_VBUSLIMIT;

	if (charger->input_current < current_limit && charger->aicl_state) {
		pr_info("%s: skip set input current limit(%d <--> %d), aicl_state(%d)\n",
			__func__, charger->input_current, current_limit, charger->aicl_state);
	} else {
		/* Soft Start Charging */
		if ((charger->cable_type != SEC_BATTERY_CABLE_NONE) && !poweroff_charging) {
			data = sm5703_reg_read(charger->i2c, SM5703_VBUSCNTL);
			data &= ~SM5703_VBUSLIMIT;
			sm5703_reg_write(charger->i2c, SM5703_VBUSCNTL, data);
			msleep(200);
		}
		if (current_limit > 100) {
			temp = ((current_limit - 100) / 50) | data;
			sm5703_reg_write(charger->i2c, SM5703_VBUSCNTL, temp);
		}

		data = sm5703_reg_read(charger->i2c, SM5703_VBUSCNTL);
		pr_info("%s : SM5703_VBUSCNTL (Input current limit) : 0x%02x\n",
				__func__, data);
	}

	mutex_unlock(&charger->io_lock);
}

static int sm5703_get_input_current_limit(struct sm5703_charger_data *charger)
{
	int ret, current_limit = 0;
	ret = sm5703_reg_read(charger->i2c, SM5703_VBUSCNTL);
	if (ret < 0)
		return ret;
	ret&=SM5703_VBUSLIMIT_MASK;

	current_limit = (100 + (ret*50));

	return current_limit;
}

static void sm5703_set_regulation_voltage(struct sm5703_charger_data *charger,
		int float_voltage)
{
	int data = 0;

	data = sm5703_reg_read(charger->i2c, SM5703_CHGCNTL3);

	data &= ~SM5703_BATREG_MASK;

	if ((float_voltage) <= 4120)
		data = 0x00;
	else if ((float_voltage) >= 4430)
		data = 0x1f;
	else
		data = ((float_voltage - 4120) / 10);

	mutex_lock(&charger->io_lock);
	sm5703_reg_write(charger->i2c, SM5703_CHGCNTL3, data);
	data = sm5703_reg_read(charger->i2c, SM5703_CHGCNTL3);
	pr_info("%s : SM5703_CHGCNTL3 (Battery regulation voltage) : 0x%02x\n",
			__func__, data);
	mutex_unlock(&charger->io_lock);
}

#if defined(CONFIG_BATTERY_SWELLING)
static int sm5703_get_regulation_voltage(struct sm5703_charger_data *charger)
{
	int data = 0;

	data = sm5703_reg_read(charger->i2c, SM5703_CHGCNTL3);
	data &= SM5703_BATREG_MASK;

	return (4120 + (data * 10));
}
#endif

static void sm5703_set_charge_current(struct sm5703_charger_data *charger,
		int charging_current)
{
	int data = 0;

	if(charging_current <= 100)
		charging_current = 100;
	else if (charging_current >= 2500)
		charging_current = 2500;

	data = (charging_current - 100) / 50;

	sm5703_reg_write(charger->i2c, SM5703_CHGCNTL2, data);

	data = sm5703_reg_read(charger->i2c, SM5703_CHGCNTL2);
	pr_info("%s : SM5703_CHGCNTL2 (fastchg current) : 0x%02x\n",
			__func__, data);

}

static int sm5703_get_fast_charging_current(struct sm5703_charger_data *charger)
{
	int data = sm5703_reg_read(charger->i2c, SM5703_CHGCNTL2);
	int charging_current = 0;

	if (data < 0)
		return data;

	data &= SM5703_FASTCHG_MASK;

	charging_current = (100 + (data*50));

	return charging_current;
}

static int sm5703_get_topoff_current(struct sm5703_charger_data *charger)
{
	int ret, data = 0, topoff_current = 0;
	mutex_lock(&charger->io_lock);
	ret = sm5703_reg_read(charger->i2c, SM5703_CHGCNTL4);
	mutex_unlock(&charger->io_lock);
	if (ret < 0) {
		pr_info("%s: warning --> fail to read i2c register(%d)\n", __func__, ret);
		return ret;
	}

	data = ((ret & SM5703_TOPOFF_MASK) >> SM5703_TOPOFF_SHIFT);  

	topoff_current = (100 + (data*25));

	return topoff_current;
}

static void sm5703_set_topoff_current(struct sm5703_charger_data *charger,
		int current_limit)
{
	int data = 0, temp = 0;

	pr_info("%s : Set Termination\n", __func__);

	data = sm5703_reg_read(charger->i2c, SM5703_CHGCNTL4);

	data &= ~SM5703_TOPOFF_MASK;

	if(current_limit <= 100)
		current_limit = 100;
	else if (current_limit >= 475)
		current_limit = 475;

	temp = (current_limit - 100) / 25;
	data |= (temp << SM5703_TOPOFF_SHIFT);

	sm5703_reg_write(charger->i2c, SM5703_CHGCNTL4, data);

	data = sm5703_reg_read(charger->i2c, SM5703_CHGCNTL4);
	pr_info("%s : SM5703_CHGCNTL4 (Top-off current threshold) : 0x%02x\n",
			__func__, data);    
}

static void sm5703_set_otgcurrent(struct sm5703_charger_data *charger,
		int otg_current)
{
	int data = 0;

	data = sm5703_reg_read(charger->i2c, SM5703_OTGCURRENTCNTL);

	data &= ~SM5703_OTGCURRENT_MASK;

	if (otg_current <= 500)
		data = 0x00;
	else if (otg_current <= 700)
		data = 0x01;
	else if (otg_current <= 900)
		data = 0x02;    
	else
		data = 0x3;

	mutex_lock(&charger->io_lock);
	sm5703_reg_write(charger->i2c, SM5703_OTGCURRENTCNTL, data);
	data = sm5703_reg_read(charger->i2c, SM5703_OTGCURRENTCNTL);
	pr_info("%s : SM5703_OTGCURRENTCNTL (OTG current) : 0x%02x\n",
			__func__, data);
	mutex_unlock(&charger->io_lock);
}

static void sm5703_set_bst_iq3limit(struct sm5703_charger_data *charger,
		int iq3limit)
{
	int data = 0;

	mutex_lock(&charger->io_lock);
	data = sm5703_reg_read(charger->i2c, SM5703_Q3LIMITCNTL);
	data &= ~SM5703_BST_IQ3LIMIT_MASK;
	data |= (iq3limit << SM5703_BST_IQ3LIMIT_SHIFT);

	sm5703_reg_write(charger->i2c, SM5703_Q3LIMITCNTL, data);

	data = sm5703_reg_read(charger->i2c, SM5703_Q3LIMITCNTL);
	pr_info("%s : SM5703_Q3LIMITCNTL (BST_IQ3LIMIT) : 0x%02x\n",
			__func__, data);
	mutex_unlock(&charger->io_lock);
}

#if ENABLE_AICL
/* Dedicated charger (non-USB) device
 * will use lower AICL level to get better performance
 */
static void sm5703_set_aicl_level(struct sm5703_charger_data *charger)
{
	int aicl;
#if 0 // need to check
	switch(charger->cable_type) {
	case SEC_BATTERY_CABLE_USB ... POWER_SUPPLY_TYPE_USB_ACA:
		aicl = (charger->pdata->chg_aiclth - 4300) / 100;
		break;
	default:
		aicl = (charger->pdata->chg_aiclth - 4300) / 100;
	}
#endif
	aicl = (charger->pdata->chg_aiclth - 4300) / 100;

	mutex_lock(&charger->io_lock);
	sm5703_assign_bits(charger->i2c,
			SM5703_CHGCNTL5, SM5703_AICLTH_MASK, aicl);
	mutex_unlock(&charger->io_lock);
}
#endif /*ENABLE_AICL*/

#if 0 //temp block
static void sm5703_configure_charger(struct sm5703_charger_data *charger)
{
	/* check aicl state */
	if ((sm5703_reg_read(charger->i2c, SM5703_STATUS1) & 0x01) &&
			!charger->aicl_state) {
		charger->aicl_state = true;
		wake_lock(&charger->aicl_wake_lock);
		queue_delayed_work_on(0, charger->wq, &charger->aicl_work, msecs_to_jiffies(50));
	}
}
#endif

/* here is set init charger data */
static bool sm5703_chg_init(struct sm5703_charger_data *charger)
{
	sm5703_mfd_chip_t *chip = i2c_get_clientdata(charger->i2c);
	int ret;
	chip->charger = charger;

	/* AUTOSTOP */
	sm5703_enable_autostop(chip->charger, (int)charger->pdata->chg_autostop);
	/* AUTOSET */
	sm5703_enable_autoset(chip->charger, (int)charger->pdata->chg_autoset);
	/* AICLEN */
	sm5703_enable_aiclen(chip->charger, (int)charger->pdata->chg_aiclen);
	/* AICLTH */
	sm5703_set_aiclth(chip->charger, (int)charger->pdata->chg_aiclth);
	/* FREQSEL */
	sm5703_set_freqsel(chip->charger, SM5703_FREQSEL_1P5MHZ);

	/* Auto-Stop configuration for Emergency status */
	sm5703_set_topoff_timer(charger, SM5703_TOPOFF_TIMER_45m);

	/* MUST set correct regulation voltage first
	 * Before MUIC pass cable type information to charger
	 * charger would be already enabled (default setting)
	 * it might cause EOC event by incorrect regulation voltage */
	sm5703_set_regulation_voltage(charger,
			charger->pdata->chg_float_voltage);

	sm5703_set_otgcurrent(charger, 1200); /* OTGCURRENT : 1.2A */

	sm5703_set_bst_iq3limit(charger, SM5703_BST_IQ3LIMIT_1X);

	ret = sm5703_reg_read(charger->i2c, SM5703_STATUS3);
	if (ret < 0)
		pr_info("Error : can't get charging status (%d)\n", ret);

	if (ret & SM5703_STATUS3_TOPOFF) {
		pr_info("%s: W/A Charger already topoff state. Charger Off\n",
				__func__);
		sm5703_enable_charger_switch(charger, 0);
		msleep(100);
		sm5703_enable_charger_switch(charger, 1);
	}

	sm5703_test_read(charger->i2c);

	return true;
}

static int sm5703_get_charging_status(struct sm5703_charger_data *charger)
{
	int status = POWER_SUPPLY_STATUS_UNKNOWN;
	int ret;
	int nCHG = 0;

	ret = sm5703_reg_read(charger->i2c, SM5703_STATUS3);
	if (ret<0) {
		pr_info("Error : can't get charging status (%d)\n", ret);

	}

	nCHG = gpio_get_value(charger->pdata->chgen_gpio);

	if ((ret & SM5703_STATUS3_DONE) || (ret & SM5703_STATUS3_TOPOFF)) {
		status = POWER_SUPPLY_STATUS_FULL;
		pr_info("%s : Status, Power Supply Full \n", __func__);
	} else if (ret & SM5703_STATUS3_CHGON) {
		status = POWER_SUPPLY_STATUS_CHARGING;    
	} else {
		if (nCHG)
			status = POWER_SUPPLY_STATUS_DISCHARGING;
		else
			status = POWER_SUPPLY_STATUS_NOT_CHARGING;
	}

	/* TEMP_TEST : when OTG is enabled(charging_current -1), handle OTG func. */
	if (charger->charging_current < 0) {
		/* For OTG mode, SM5703 would still report "charging" */
		status = POWER_SUPPLY_STATUS_DISCHARGING;
		ret = sm5703_reg_read(charger->i2c, SM5703_STATUS1);
		if (ret & SM5703_STATUS1_OTGFAIL) {
			pr_info("%s: otg overcurrent limit\n", __func__);
			sm5703_charger_otg_control(charger, false);
		}

	}

	return status;
}

static int sm5703_get_charging_health(struct sm5703_charger_data *charger)
{
	int vbus_status = sm5703_reg_read(charger->i2c, SM5703_STATUS5);
	int health = POWER_SUPPLY_HEALTH_GOOD;
	int chg_cntl = 0, nCHG = 0;

	chg_cntl = sm5703_reg_read(charger->i2c, SM5703_CNTL);
	nCHG = gpio_get_value(charger->pdata->chgen_gpio);

	if (vbus_status < 0) {
		health = POWER_SUPPLY_HEALTH_UNKNOWN;
		pr_info("%s : charging = %d, cable = %d, aicl = %d, CNTL = 0x%x, nCHG = %d, Health = %d, vbus = %d\n",
			__func__, charger->is_charging, charger->cable_type,
			charger->aicl_state, chg_cntl, nCHG, health, vbus_status);

		return (int)health;
	}

	if (vbus_status & SM5703_STATUS5_VBUSOK)
		health = POWER_SUPPLY_HEALTH_GOOD;
	else if (vbus_status & SM5703_STATUS5_VBUSOVP)
		health = POWER_SUPPLY_HEALTH_OVERVOLTAGE;
	else if (vbus_status & SM5703_STATUS5_VBUSUVLO)
		health = POWER_SUPPLY_HEALTH_UNDERVOLTAGE;
	else
		health = POWER_SUPPLY_HEALTH_UNKNOWN;

	pr_info("%s : charging = %d, cable = %d, aicl = %d, CNTL = 0x%x, nCHG = %d, Health = %d, vbus = %d\n",
		__func__, charger->is_charging, charger->cable_type,
		charger->aicl_state, chg_cntl, nCHG, health, vbus_status);

	return (int)health;
}

static void psy_chg_set_cable_online(struct sm5703_charger_data *charger, int cable_type)
{
	int prev_cable_type = charger->cable_type;

	charger->cable_type = cable_type;
	charger->aicl_state = false;
	charger->input_current = sm5703_get_input_current_limit(charger);

	pr_info("%s [start] prev_cable_type(%d), cable_type(%d)\n", __func__, prev_cable_type, cable_type);

#if ENABLE_AICL
	sm5703_set_aicl_level(charger);
#endif /*DISABLE_AICL*/
	sm5703_enable_autoset(charger, 0);

	if (charger->cable_type == SEC_BATTERY_CABLE_POWER_SHARING) {
		charger->is_charging = false;
		sm5703_charger_otg_control(charger, 1);
	} else if (charger->cable_type == SEC_BATTERY_CABLE_OTG) {
		charger->is_charging = false;
		pr_info("OTG enable, cable(%d)\n", charger->cable_type);
		sm5703_charger_otg_control(charger, 1);
	} else if (charger->cable_type == SEC_BATTERY_CABLE_NONE) {
		sm5703_enable_autoset(charger, (int)charger->pdata->chg_autoset);
		charger->is_charging = false;
		sm5703_charger_vbus_control(charger, 0);
		if (prev_cable_type != SEC_BATTERY_CABLE_LAN_HUB)
			sm5703_charger_otg_control(charger, 0);
	} else {
		sm5703_charger_vbus_control(charger, 1);
	}

	pr_info("[end] is_charging=%d(%d), il = %d, cable = %d\n",
		charger->is_charging,
		charger->charging_current,
		charger->input_current,
		charger->cable_type);
}

static void psy_chg_set_charging_enabled(struct sm5703_charger_data *charger, int charge_mode)
{
	int buck_off = false;

	pr_info("%s charger_mode changed [%d] -> [%d]\n", __func__, charger->charge_mode, charge_mode);

	charger->charge_mode = charge_mode;

	switch (charger->charge_mode) {
	case SEC_BAT_CHG_MODE_BUCK_OFF:
		buck_off = true;
		break;
	case SEC_BAT_CHG_MODE_CHARGING_OFF:
		charger->is_charging = false;
		break;
	case SEC_BAT_CHG_MODE_CHARGING:
		charger->is_charging = true;
		break;
	}

	sm5703_enable_charger_switch(charger, charger->is_charging);
}

static int sec_chg_get_property(struct power_supply *psy,
		enum power_supply_property psp,
		union power_supply_propval *val)
{

	int vbus_status;
	struct sm5703_charger_data *charger =
		container_of(psy, struct sm5703_charger_data, psy_chg);

	switch (psp) {
		case POWER_SUPPLY_PROP_ONLINE:
			vbus_status = sm5703_reg_read(charger->i2c, SM5703_STATUS5);
			if (charger->cable_type != SEC_BATTERY_CABLE_NONE &&
				!(vbus_status & SM5703_STATUS5_VBUSOK))
					charger->cable_type = SEC_BATTERY_CABLE_NONE;

			val->intval = charger->cable_type;
			pr_info("%s: Charger Cable type : %d\n", __func__, charger->cable_type);
			break;
		case POWER_SUPPLY_PROP_STATUS:
			val->intval = sm5703_get_charging_status(charger);
			break;
		case POWER_SUPPLY_PROP_HEALTH:
			val->intval = sm5703_get_charging_health(charger);
			break;
		/* get input current which was set */
		case POWER_SUPPLY_PROP_CURRENT_MAX:
			val->intval = charger->input_current;
			break;
		/* get input current which was read */
		case POWER_SUPPLY_PROP_CURRENT_AVG:
			val->intval = sm5703_get_input_current_limit(charger);
			break;
		/* get charge current which was set */
		case POWER_SUPPLY_PROP_CURRENT_NOW:
			val->intval = charger->charging_current;
			break;
		/* get charge current which was read */
		case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
			val->intval = sm5703_get_fast_charging_current(charger);
			break;
		case POWER_SUPPLY_PROP_CURRENT_FULL:
			val->intval = sm5703_get_topoff_current(charger);
			break;
#if defined(CONFIG_BATTERY_SWELLING)
		case POWER_SUPPLY_PROP_VOLTAGE_MAX:
			val->intval = sm5703_get_regulation_voltage(charger);
		break;
#endif
		case POWER_SUPPLY_PROP_CHARGE_TYPE:
			if (!charger->is_charging || charger->cable_type == SEC_BATTERY_CABLE_NONE) {
				val->intval = POWER_SUPPLY_CHARGE_TYPE_NONE;
			} else if (charger->input_current <= SLOW_CHARGING_CURRENT_STANDARD) {
				val->intval = POWER_SUPPLY_CHARGE_TYPE_SLOW;
				pr_info("%s: slow-charging mode\n", __func__);
			} else
				val->intval = POWER_SUPPLY_CHARGE_TYPE_FAST;
			break;
		case POWER_SUPPLY_PROP_CHARGING_ENABLED:
			val->intval = charger->is_charging;
			break;
		case POWER_SUPPLY_PROP_CHARGE_OTG_CONTROL:
			break;
		default:
			return -EINVAL;
	}

	return 0;
}
#if defined(CONFIG_MUIC_SLAVE_MODE_CONTROL_VBUS)
extern int manual_control_vbus_sw(bool enable);
#endif
static int sec_chg_set_property(struct power_supply *psy,
		enum power_supply_property psp,
		const union power_supply_propval *val)
{
	struct sm5703_charger_data *charger =
		container_of(psy, struct sm5703_charger_data, psy_chg);

	switch (psp) {
		case POWER_SUPPLY_PROP_STATUS:
			charger->status = val->intval;
			break;
		case POWER_SUPPLY_PROP_CHARGING_ENABLED:
			pr_info("%s CHG:POWER_SUPPLY_PROP_CHARGING_ENABLED (val=%d)\n", __func__, val->intval);
			psy_chg_set_charging_enabled(charger, val->intval);
			break;
		/* set online mode */
		case POWER_SUPPLY_PROP_ONLINE:
			psy_chg_set_cable_online(charger, val->intval);
			break;
		/* set input current */
		case POWER_SUPPLY_PROP_CURRENT_MAX:
			pr_info("%s CHG:POWER_SUPPLY_PROP_CURRENT_AVG (val=%d)\n", __func__, val->intval);
			charger->input_current = val->intval;
			sm5703_set_input_current_limit(charger, charger->input_current);
			break;
		/* set charge current */
		case POWER_SUPPLY_PROP_CURRENT_AVG:
			pr_info("%s CHG:POWER_SUPPLY_PROP_CURRENT_AVG (val=%d)\n", __func__, val->intval);
			charger->charging_current = val->intval;
			sm5703_set_charge_current(charger, charger->charging_current);
			break;
		case POWER_SUPPLY_PROP_CURRENT_NOW:
			break;
		case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
			break;
		/* set top-off current */
		case POWER_SUPPLY_PROP_CURRENT_FULL:
			pr_info("%s CHG:POWER_SUPPLY_PROP_CURRENT_FULL (val=%d)\n", __func__, val->intval);
			sm5703_set_topoff_current(charger, val->intval);
			break;
#if defined(CONFIG_BATTERY_SWELLING)
		case POWER_SUPPLY_PROP_VOLTAGE_MAX:
			pr_info("%s CHG:POWER_SUPPLY_PROP_VOLTAGE_MAX (val=%d)\n", __func__, val->intval);
			charger->pdata->chg_float_voltage = val->intval;
			sm5703_set_regulation_voltage(charger, val->intval);
			break;
#endif
		case POWER_SUPPLY_PROP_CHARGE_OTG_CONTROL:
			pr_info("POWER_SUPPLY_PROP_CHARGE_OTG_CONTROL - otg_en=%d\n", val->intval);
			sm5703_charger_otg_control(charger, val->intval);
			break;
		case POWER_SUPPLY_PROP_ENERGY_NOW:
			/* charger off when jig detected */
			if (val->intval)
				sm5703_enable_charger_switch(charger, false);
			break;
		default:
			return -EINVAL;
	}

	return 0;
}

static int sm5703_otg_get_property(struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = otg_enable_flag;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int sm5703_otg_set_property(struct power_supply *psy,
				enum power_supply_property psp,
				const union power_supply_propval *val)
{
	union power_supply_propval value;

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		value.intval = val->intval;
		pr_info("%s: OTG %s\n", __func__, value.intval > 0 ? "on" : "off");
		psy_do_property("sm5703-charger", set,
					POWER_SUPPLY_PROP_CHARGE_OTG_CONTROL, value);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

ssize_t sm5703_chg_show_attrs(struct device *dev,
		const ptrdiff_t offset, char *buf)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct sm5703_charger_data *charger =
		container_of(psy, struct sm5703_charger_data, psy_chg);
	int i = 0;
	char *str = NULL;

	switch (offset) {
	case CHG_REG:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%x\n",
				charger->reg_addr);
		break;
	case CHG_DATA:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%x\n",
				charger->reg_data);
		break;
	case CHG_REGS:
		str = kzalloc(sizeof(char) * 256, GFP_KERNEL);
		if (!str)
			return -ENOMEM;

		sm5703_read_regs(charger->i2c, str);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n",
				str);

		kfree(str);
		break;
	default:
		i = -EINVAL;
		break;
	}

	return i;
}

ssize_t sm5703_chg_store_attrs(struct device *dev,
		const ptrdiff_t offset,
		const char *buf, size_t count)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct sm5703_charger_data *charger =
		container_of(psy, struct sm5703_charger_data, psy_chg);

	int ret = 0;
	int x = 0;
	uint8_t data = 0;

	switch (offset) {
	case CHG_REG:
		if (sscanf(buf, "%10x\n", &x) == 1) {
			charger->reg_addr = x;
			data = sm5703_reg_read(charger->i2c,
					charger->reg_addr);
			charger->reg_data = data;
			dev_dbg(dev, "%s: (read) addr = 0x%x, data = 0x%x\n",
					__func__, charger->reg_addr, charger->reg_data);
			ret = count;
		}
		break;
	case CHG_DATA:
		if (sscanf(buf, "%10x\n", &x) == 1) {
			data = (u8)x;

			dev_dbg(dev, "%s: (write) addr = 0x%x, data = 0x%x\n",
					__func__, charger->reg_addr, data);
			ret = sm5703_reg_write(charger->i2c,
					charger->reg_addr, data);
			if (ret < 0) {
				dev_dbg(dev, "I2C write fail Reg0x%x = 0x%x\n",
						(int)charger->reg_addr, (int)data);
			}
			ret = count;
		}
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

struct sm5703_chg_irq_handler {
	char *name;
	int irq_index;
	irqreturn_t (*handler)(int irq, void *data);
};
#if EN_NOBAT_IRQ
static irqreturn_t sm5703_chg_nobat_irq_handler(int irq, void *data)
{
	struct sm5703_charger_data *info = data;
	struct i2c_client *iic = info->sm5703->i2c_client;

	/* set full charged flag
	 * until TA/USB unplug event / stop charging by PSY
	 */

	pr_info("%s : Nobat\n", __func__);

#if EN_TEST_READ
	sm5703_test_read(iic);
#endif

	return IRQ_HANDLED;
}
#endif /*EN_NOBAT_IRQ*/

#if EN_DONE_IRQ
static irqreturn_t sm5703_chg_done_irq_handler(int irq, void *data)
{
	struct sm5703_charger_data *info = data;
	struct i2c_client *iic = info->sm5703->i2c_client;

	/* set full charged flag
	 * until TA/USB unplug event / stop charging by PSY
	 */

	pr_info("%s : Full charged(done)\n", __func__);

#if EN_TEST_READ
	sm5703_test_read(iic);
#endif

	return IRQ_HANDLED;
}
#endif/*EN_DONE_IRQ*/

#if EN_TOPOFF_IRQ
static irqreturn_t sm5703_chg_topoff_irq_handler(int irq, void *data)
{
	struct sm5703_charger_data *info = data;
	struct i2c_client *iic = info->sm5703->i2c_client;

	/* set full charged flag
	 * until TA/USB unplug event / stop charging by PSY
	 */

	pr_info("%s : Full charged(topoff)\n", __func__);

#if EN_TEST_READ
	sm5703_test_read(iic);
#endif

	return IRQ_HANDLED;
}
#endif /*EN_TOPOFF_IRQ*/

#if EN_AICL_IRQ
static void sm5703_chg_aicl_work(struct work_struct *work)
{
	struct sm5703_charger_data *charger =
		container_of(work, struct sm5703_charger_data, aicl_work.work);
	int reg_data = 0, temp = 0, aicl_state;
	int current_limit = charger->input_current;

	mutex_lock(&charger->io_lock);
	/* disable aicl irq */
	sm5703_assign_bits(charger->i2c, SM5703_INTMSK1, 0x01, 0x01);

	/* reduce input current */
	reg_data = sm5703_reg_read(charger->i2c, SM5703_VBUSCNTL);
	reg_data &= ~SM5703_VBUSLIMIT;
	do {
		current_limit -= REDUCE_CURRENT_STEP;
		temp = ((current_limit - 100) / 50) | reg_data;
		sm5703_reg_write(charger->i2c, SM5703_VBUSCNTL, temp);
		msleep(200);
		aicl_state = sm5703_reg_read(charger->i2c, SM5703_STATUS1) & 0x01;
		pr_info("%s: aicl state(%d), current limit(%d), max(%d)\n",
			__func__, aicl_state, current_limit, charger->input_current);
	} while (aicl_state && current_limit > MINIMUM_INPUT_CURRENT);
	charger->input_current = current_limit;

	if (charger->aicl_state) {
		union power_supply_propval value;
		value.intval = sm5703_get_input_current_limit(charger);
		psy_do_property("battery", set,
			POWER_SUPPLY_EXT_PROP_AICL_CURRENT, value);
	}

	/* check slow charging */
	if (charger->input_current <= SLOW_CHARGING_CURRENT_STANDARD &&
		charger->cable_type != SEC_BATTERY_CABLE_NONE) {
		union power_supply_propval value;
		psy_do_property("battery", set,
			POWER_SUPPLY_PROP_CHARGE_TYPE, value);
		pr_info("%s: slow charging on : input current(%dmA), cable type(%d)\n",
			__func__, charger->input_current, charger->cable_type);
	}

	/* enable aicl irq */
	sm5703_assign_bits(charger->i2c, SM5703_INTMSK1, 0x01, 0x00);
	mutex_unlock(&charger->io_lock);

	wake_unlock(&charger->aicl_wake_lock);
}
#endif

#if EN_AICL_IRQ
static irqreturn_t sm5703_chg_aicl_irq_handler(int irq, void *data)
{
	struct sm5703_charger_data *info = data;
	struct i2c_client *iic = info->sm5703->i2c_client;

#if EN_TEST_READ
	sm5703_test_read(iic);
#endif

	pr_info("%s: cable type:%d, current max:%d, aicl_state:%d\n",
		__func__, info->cable_type, info->input_current, info->aicl_state);

	if (info->cable_type != SEC_BATTERY_CABLE_NONE &&
		info->input_current > MINIMUM_INPUT_CURRENT &&
		!info->aicl_state) {
		info->aicl_state = true;
		wake_lock(&info->aicl_wake_lock);
		queue_delayed_work_on(0, info->wq, &info->aicl_work, msecs_to_jiffies(50));
	}
	return IRQ_HANDLED;
}
#endif

#if EN_CHGON_IRQ
static irqreturn_t sm5703_chg_chgon_irq_handler(int irq, void *data)
{
	struct sm5703_charger_data *info = data;
	struct i2c_client *iic = info->sm5703->i2c_client;

	pr_info("%s : Chgon\n", __func__);

#if EN_TEST_READ
	sm5703_test_read(iic);
#endif

	return IRQ_HANDLED;
}
#endif /*EN_CHGON_IRQ*/

#if EN_OTGFAIL_IRQ
static irqreturn_t sm5703_chg_otgfail_irq_handler(int irq, void *data)
{
	struct sm5703_charger_data *info = data;
	/* struct i2c_client *iic = info->sm5703->i2c_client; */
	int ret;

#ifdef CONFIG_USB_HOST_NOTIFY
	struct otg_notify *o_notify;

	o_notify = get_otg_notify();
#endif
	pr_info("%s : OTG Failed\n", __func__);

	ret = sm5703_reg_read(info->sm5703->i2c_client, SM5703_STATUS1);
	if (ret & SM5703_STATUS1_OTGFAIL) {
		pr_info("%s: otg overcurrent limit\n", __func__);
#ifdef CONFIG_USB_HOST_NOTIFY
		send_otg_notify(o_notify, NOTIFY_EVENT_OVERCURRENT, 0);
#endif
		sm5703_charger_otg_control(info, false);
	} else {
		pr_info("%s: OTGFAIL, SM5703_STATUS1: 0x%x\n", __func__, ret);
	}

	return IRQ_HANDLED;
}
#endif /*EN_CHGON_IRQ*/

const struct sm5703_chg_irq_handler sm5703_chg_irq_handlers[] = {
#if EN_NOBAT_IRQ    
	{
		.name = "NOBAT",
		.handler = sm5703_chg_nobat_irq_handler,
		.irq_index = SM5703_NOBAT_IRQ,
	},
#endif /*EN_NOBAT_IRQ*/
#if EN_DONE_IRQ
	{
		.name = "DONE",
		.handler = sm5703_chg_done_irq_handler,
		.irq_index = SM5703_DONE_IRQ,
	},
#endif/*EN_DONE_IRQ*/	
#if EN_TOPOFF_IRQ	
	{
		.name = "TOPOFF",
		.handler = sm5703_chg_topoff_irq_handler,
		.irq_index = SM5703_TOPOFF_IRQ,
	},
#endif /*EN_TOPOFF_IRQ*/
#if EN_CHGON_IRQ
	{
		.name = "CHGON",
		.handler = sm5703_chg_chgon_irq_handler,
		.irq_index = SM5703_CHGON_IRQ,
	},
#endif /*EN_CHGON_IRQ*/
#if EN_AICL_IRQ
	{
		.name = SM5703_AICL_IRQ_NAME,
		.handler = sm5703_chg_aicl_irq_handler,
		.irq_index = SM5703_AICL_IRQ,
	},
#endif /* EN_AICL_IRQ */
#if EN_OTGFAIL_IRQ
	{
		.name = "OTGFAIL",
		.handler = sm5703_chg_otgfail_irq_handler,
		.irq_index = SM5703_OTGFAIL_IRQ,
	},
#endif /* EN_OTGFAIL_IRQ */
};


static int register_irq(struct platform_device *pdev,
		struct sm5703_charger_data *info)
{
	int irq;
	int i, j;
	int ret;
	const struct sm5703_chg_irq_handler *irq_handler = sm5703_chg_irq_handlers;
	const char *irq_name;
	for (i = 0; i < ARRAY_SIZE(sm5703_chg_irq_handlers); i++) {
		irq_name = sm5703_get_irq_name_by_index(irq_handler[i].irq_index);
		irq = platform_get_irq_byname(pdev, irq_name);
		ret = request_threaded_irq(irq, NULL, irq_handler[i].handler,
				IRQF_ONESHOT | IRQF_TRIGGER_FALLING |
				IRQF_NO_SUSPEND, irq_name, info);
		if (ret < 0) {
			pr_err("%s : Failed to request IRQ (%s): #%d: %d\n",
					__func__, irq_name, irq, ret);
			goto err_irq;
		}

		pr_info("%s : Register IRQ%d(%s) successfully\n",
				__func__, irq, irq_name);
	}

	return 0;
err_irq:
	for (j = 0; j < i; j++) {
		irq_name = sm5703_get_irq_name_by_index(irq_handler[j].irq_index);
		irq = platform_get_irq_byname(pdev, irq_name);
		free_irq(irq, info);
	}

	return ret;
}

static void unregister_irq(struct platform_device *pdev,
		struct sm5703_charger_data *info)
{
	int irq;
	int i;
	const char *irq_name;
	const struct sm5703_chg_irq_handler *irq_handler = sm5703_chg_irq_handlers;

	for (i = 0; i < ARRAY_SIZE(sm5703_chg_irq_handlers); i++) {
		irq_name = sm5703_get_irq_name_by_index(irq_handler[i].irq_index);
		irq = platform_get_irq_byname(pdev, irq_name);
		free_irq(irq, info);
	}
}

#ifdef CONFIG_OF
static int sm5703_charger_parse_dt(struct device *dev,
		struct sm5703_charger_platform_data *pdata)
{
	struct device_node *np = dev->of_node;
	int ret;

	ret = of_property_read_u32(np, "chg_autostop", &pdata->chg_autostop);
	if (ret < 0) {
		pr_info("%s : cannot get chg autostop\n", __func__);
		pdata->chg_autostop = 0;
	}

	ret = of_property_read_u32(np, "chg_autoset", &pdata->chg_autoset);
	if (ret < 0) {
		pr_info("%s : cannot get chg autoset\n", __func__);
		pdata->chg_autoset = 0;
	}

	ret = of_property_read_u32(np, "chg_aiclen", &pdata->chg_aiclen);
	if (ret < 0) {
		pr_info("%s : cannot get chg aiclen\n", __func__);
		pdata->chg_aiclen = 0;
	}

	ret = of_property_read_u32(np, "chg_aiclth", &pdata->chg_aiclth);
	if (ret < 0) {
		pr_info("%s : cannot get chg aiclth\n", __func__);
		pdata->chg_aiclth = 4500;
	}

	np = of_find_node_by_name(NULL, "battery");
	if (!np) {
		pr_info("%s : np NULL\n", __func__);
		return -ENODATA;
	}

	np = of_find_node_by_name(NULL, "sm5703-charger");
	if (!np) {
		pr_info("%s : np NULL\n", __func__);
		return -ENODATA;
	}

	/* battery,chg_float_voltage */
	ret = of_property_read_u32(np, "battery,chg_float_voltage",
			&pdata->chg_float_voltage);
	if (ret < 0) {
		pr_info("%s : cannot get chg float voltage\n", __func__);
		return -ENODATA;
	}

	ret = of_property_read_string(np,
		"battery,charger_name", (char const **)&pdata->charger_name);
	if (ret) {
		pdata->charger_name = "sm5703-charger";
		pr_info("%s: Charger name is Empty. Set default.\n", __func__);
	}

	pdata->chgen_gpio = of_get_named_gpio(np, "battery,chg_gpio_en", 0);
	if (pdata->chgen_gpio < 0) {
		pr_err("%s : cannot get chgen gpio : %d\n",
			__func__, pdata->chgen_gpio);
		return -ENODATA;
	} else {
		pr_info("%s: chgen gpio : %d\n", __func__, pdata->chgen_gpio);
	}

	pr_info("%s sm5703 charger parse dt retval = %d\n", __func__, ret);
	return ret;
}

static struct of_device_id sm5703_charger_match_table[] = {
	{ .compatible = "siliconmitus,sm5703-charger",},
	{},
};
#else
static int sm5703_charger_parse_dt(struct device *dev,
		struct sm5703_charger_platform_data *pdata)
{
	return -ENOSYS;
}
#define sm5703_charger_match_table NULL
#endif /* CONFIG_OF */

static int sm5703_charger_probe(struct platform_device *pdev)
{
	sm5703_mfd_chip_t *chip = dev_get_drvdata(pdev->dev.parent);
#ifndef CONFIG_OF
	struct sm5703_mfd_platform_data *mfd_pdata =
				dev_get_platdata(chip->dev);
#endif	
	struct sm5703_charger_data *charger;
	int ret = 0;

	otg_enable_flag = 0;

	pr_info("%s:[BATT] SM5703 Charger driver probe..\n", __func__);

	charger = kzalloc(sizeof(*charger), GFP_KERNEL);
	if (!charger)
		return -ENOMEM;

	mutex_init(&charger->io_lock);
	charger->sm5703= chip;
	charger->i2c = chip->i2c_client;

#ifdef CONFIG_OF	
	charger->pdata = devm_kzalloc(&pdev->dev,
			sizeof(*(charger->pdata)), GFP_KERNEL);
	if (!charger->pdata) {
		dev_err(&pdev->dev, "Failed to allocate memory\n");
		ret = -ENOMEM;
		goto err_parse_dt_nomem;
	}
	ret = sm5703_charger_parse_dt(&pdev->dev, charger->pdata);
	if (ret < 0)
		goto err_parse_dt;
#else		
	charger->pdata = mfd_pdata->charger_platform_data;
#endif

	platform_set_drvdata(pdev, charger);

	if (charger->pdata->charger_name == NULL)
		charger->pdata->charger_name = "sm5703-charger";

	charger->psy_chg.name			= charger->pdata->charger_name;
	charger->psy_chg.type			= POWER_SUPPLY_TYPE_UNKNOWN;
	charger->psy_chg.get_property	= sec_chg_get_property;
	charger->psy_chg.set_property	= sec_chg_set_property;
	charger->psy_chg.properties		= sec_charger_props;
	charger->psy_chg.num_properties	= ARRAY_SIZE(sec_charger_props);
	charger->psy_otg.name			= "otg";
	charger->psy_otg.type			= POWER_SUPPLY_TYPE_OTG;
	charger->psy_otg.get_property	= sm5703_otg_get_property;
	charger->psy_otg.set_property	= sm5703_otg_set_property;
	charger->psy_otg.properties		= sm5703_otg_props;
	charger->psy_otg.num_properties	= ARRAY_SIZE(sm5703_otg_props);

	charger->ovp = 0;
	charger->is_mdock = false;
	sm5703_chg_init(charger);

	charger->wq = create_workqueue("sm5703chg_workqueue");
	wake_lock_init(&charger->aicl_wake_lock, WAKE_LOCK_SUSPEND, "sm5703-aicl");
#if EN_AICL_IRQ
	INIT_DELAYED_WORK(&charger->aicl_work, sm5703_chg_aicl_work);
#endif
	ret = power_supply_register(&pdev->dev, &charger->psy_chg);
	if (ret) {
		pr_err("%s: Failed to Register psy_chg\n", __func__);
		goto err_power_supply_register;
	}

	ret = power_supply_register(&pdev->dev, &charger->psy_otg);
	if (ret) {
		pr_err("%s: Failed to Register otg_chg\n", __func__);
		goto err_power_supply_register_otg;
	}

	ret = register_irq(pdev, charger);
	if (ret < 0)
		goto err_reg_irq;

	ret = gpio_request(charger->pdata->chgen_gpio, "sm5703_nCHGEN");
	if (ret) {
		pr_info("%s : Request GPIO %d failed\n",
				__func__, (int)charger->pdata->chgen_gpio);
	}

	sm5703_test_read(charger->i2c);
	pr_info("%s:[BATT] SM5703 charger driver loaded OK\n", __func__);

	return 0;
err_reg_irq:
	power_supply_unregister(&charger->psy_otg);
err_power_supply_register_otg:
	power_supply_unregister(&charger->psy_chg);
err_power_supply_register:
	destroy_workqueue(charger->wq);
	wake_lock_destroy(&charger->aicl_wake_lock);
err_parse_dt:
err_parse_dt_nomem:
	mutex_destroy(&charger->io_lock);
	kfree(charger);
	return ret;
}

static int sm5703_charger_remove(struct platform_device *pdev)
{
	struct sm5703_charger_data *charger =
		platform_get_drvdata(pdev);
	unregister_irq(pdev, charger);
	power_supply_unregister(&charger->psy_chg);
	destroy_workqueue(charger->wq);
	wake_lock_destroy(&charger->aicl_wake_lock);
	mutex_destroy(&charger->io_lock);
	kfree(charger);
	return 0;
}

#if defined CONFIG_PM
static int sm5703_charger_suspend(struct device *dev)
{
	return 0;
}

static int sm5703_charger_resume(struct device *dev)
{
	return 0;
}
#else
#define sm5703_charger_suspend NULL
#define sm5703_charger_resume NULL
#endif

static void sm5703_charger_shutdown(struct device *dev)
{
	pr_info("%s: SM5703 Charger driver shutdown\n", __func__);
}

static SIMPLE_DEV_PM_OPS(sm5703_charger_pm_ops, sm5703_charger_suspend,
		sm5703_charger_resume);

static struct platform_driver sm5703_charger_driver = {
	.driver		= {
		.name	= "sm5703-charger",
		.owner	= THIS_MODULE,
		.of_match_table = sm5703_charger_match_table,
		.pm		= &sm5703_charger_pm_ops,
		.shutdown = sm5703_charger_shutdown,
	},
	.probe		= sm5703_charger_probe,
	.remove		= sm5703_charger_remove,
};

static int __init sm5703_charger_init(void)
{
	int ret = 0;

	pr_info("%s \n", __func__);
	ret = platform_driver_register(&sm5703_charger_driver);

	return ret;
}
device_initcall(sm5703_charger_init);

static void __exit sm5703_charger_exit(void)
{
	platform_driver_unregister(&sm5703_charger_driver);
}
module_exit(sm5703_charger_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Charger driver for SM5703");
