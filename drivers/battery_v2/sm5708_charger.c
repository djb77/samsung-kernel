/*
*  /drivers/battery_v2/sm5708_charger.c
*
*  SM5708 Charger driver for SEC_BATTERY Flatform support
*
*  Copyright (C) 2015 Silicon Mitus,
*
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
*/
#define pr_fmt(fmt) "sm5708-charger: %s: " fmt, __func__

#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/power_supply.h>
#include <linux/mfd/sm5708/sm5708.h>
#include <linux/spinlock.h>
#ifdef CONFIG_USB_HOST_NOTIFY
#include <linux/usb_notify.h>
#endif
#if defined(CONFIG_VBUS_NOTIFIER)
#include <linux/vbus_notifier.h>
#endif
#include <linux/of_gpio.h>

#include "include/charger/sm5708_charger.h"
#include "include/charger/sm5708_charger_oper.h"


/* #define SM5708_CHG_FULL_DEBUG 1 */

static enum power_supply_property sm5708_charger_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_CHARGE_TYPE,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_CURRENT_MAX,
	POWER_SUPPLY_PROP_CURRENT_AVG,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_CURRENT_FULL,
	POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT,
#if defined(SM5708_SUPPORT_OTG_CONTROL)
	POWER_SUPPLY_PROP_CHARGE_OTG_CONTROL,
#endif
	POWER_SUPPLY_PROP_USB_HC,
	POWER_SUPPLY_PROP_CHARGING_ENABLED,
#if defined(CONFIG_BATTERY_SWELLING)
	POWER_SUPPLY_PROP_VOLTAGE_MAX,
#endif
};

static struct device_attribute sm5708_charger_attrs[] = {
	SM5708_CHARGER_ATTR(chip_id),
	SM5708_CHARGER_ATTR(charger_op_mode),
	SM5708_CHARGER_ATTR(data),
};

/**
 *  SM5708 Charger device register control functions
 */

static bool sm5708_CHG_get_INT_STATUS(struct sm5708_charger_data *charger,
				unsigned char index, unsigned char offset)
{
	unsigned char reg_val;
	int ret;

	ret = sm5708_read_reg(charger->i2c, SM5708_REG_STATUS1 + index, &reg_val);
	if (ret < 0) {
		pr_err("fail to I2C read REG:SM5708_REG_INT%d\n", 1 + index);
		return 0;
	}

	reg_val = (reg_val & (1 << offset)) >> offset;

	return reg_val;
}

static int sm5708_CHG_set_TOPOFF_TMR(struct sm5708_charger_data *charger,
				unsigned char topoff_timer)
{
	sm5708_update_reg(charger->i2c,
		SM5708_REG_CHGCNTL7, ((topoff_timer & 0x3) << 3), (0x3 << 3));
	pr_info("TOPOFF_TMR set (timer=%d)\n", topoff_timer);

	return 0;
}

static int sm5708_CHG_enable_AUTOSTOP(struct sm5708_charger_data *charger,
				bool enable)
{
	int ret;

	ret = sm5708_update_reg(charger->i2c, SM5708_REG_CHGCNTL3, (enable << 6), (1 << 6));
	if (ret < 0) {
		pr_err("fail to update REG:SM5708_REG_CHGCNTL3 in AUTOSTOP[6]\n");
		return ret;
	}

	return 0;
}

static unsigned char _calc_BATREG_offset_to_float_mV(unsigned short mV)
{
	unsigned char offset;

	if (mV < 4000) {
		offset = 0x0;     /* BATREG = 3.8V */
	} else if (mV < 4010) {
		offset = 0x1;     /* BATREG = 4.0V */
	} else if (mV < 4630) {
		offset = (((mV - 4010) / 10) + 2);    /* BATREG = 4.01 ~ 4.62 */
	} else {
		pr_err("can't support BATREG at over voltage 4.62V \n");
		offset = 0x15;    /* default Offset : 4.2V */

	}

	return offset;
}

static int sm5708_CHG_set_BATREG(struct sm5708_charger_data *charger,
				unsigned short float_mV)
{
	unsigned char offset = _calc_BATREG_offset_to_float_mV(float_mV);
	int ret;

	pr_info("set BATREG voltage(%dmV - offset=0x%x)\n", float_mV, offset);

	ret = sm5708_update_reg(charger->i2c, SM5708_REG_CHGCNTL3, ((offset & 0x3F) << 0), (0x3F << 0));
	if (ret < 0) {
		pr_err("fail to update REG:SM5708_REG_CHGCNTL3 in BATREG[5:0]\n");
		return ret;
	}

	return 0;
}

static unsigned short _calc_float_mV_to_BATREG_offset(unsigned char offset)
{
	return ((offset - 2) * 10) + 4010;
}

static unsigned short sm5708_CHG_get_BATREG(struct sm5708_charger_data *charger)
{
	unsigned char offset;
	int ret;

	ret = sm5708_read_reg(charger->i2c, SM5708_REG_CHGCNTL3, &offset);
	if (ret < 0) {
		pr_err("fail to read REG:SM5708_REG_CHGCNTL3\n");
		return 0;
	}

	return _calc_float_mV_to_BATREG_offset(offset & 0x3F);
}

static unsigned char _calc_TOPOFF_offset_to_topoff_mA(unsigned short mA)
{
	unsigned char offset;

	if (mA < 100) {
		offset = 0x0;				/* Topoff = 100mA */
	} else if (mA < 480) {
		offset = (mA - 100) / 25;	/* Topoff = 125mA ~ 475mA in 25mA steps */
	} else {
		offset = 0xF;				/* Topoff = 475mA */
	}

	return offset;
}

static int sm5708_CHG_set_TOPOFF(struct sm5708_charger_data *charger,
				unsigned short topoff_mA)
{
	unsigned char offset = _calc_TOPOFF_offset_to_topoff_mA(topoff_mA);
	int ret;

	pr_info("set TOP-OFF current(%dmA - offset=0x%x)\n", topoff_mA, offset);

	ret = sm5708_update_reg(charger->i2c, SM5708_REG_CHGCNTL4, (offset << 0), (0xF << 0));
	if (ret < 0) {
		pr_err("fail to update REG:SM5708_REG_CHGCNTL4 in TOPOFF[3:0]\n");
		return ret;
	}

	return 0;
}

static int sm5708_CHG_set_FREQSEL(struct sm5708_charger_data *charger, unsigned char freq_index)
{
	int ret;

	pr_info("set BUCK&BOOST freq=0x%x\n", freq_index);

	ret = sm5708_update_reg(charger->i2c, SM5708_REG_CHGCNTL4, ((freq_index & 0x3) << 4), (0x3 << 4));
	if (ret < 0) {
		pr_err("fail to update REG:SM5708_REG_CHGCNTL4 in FREQSEL[5:4]\n");
		return ret;
	}
	return 0;
}

static unsigned char _calc_AICL_threshold_offset_to_mV(unsigned short aiclth_mV)
{
	unsigned char offset;

	if (aiclth_mV < 4500) {
		offset = 0x0;
	} else if (aiclth_mV < 4900) {
		offset = (aiclth_mV - 4500) / 100;
	} else {
		pr_err("can't support AICL at over voltage 4.8V \n");
		offset = 0x3;
	}

	return offset;
}

static int sm5708_CHG_set_AICLTH(struct sm5708_charger_data *charger,
				unsigned short aiclth_mV)
{
	unsigned char offset = _calc_AICL_threshold_offset_to_mV(aiclth_mV);
	int ret;

	pr_info("set AICL threshold (%dmV - offset=0x%x)\n", aiclth_mV, offset);

	ret = sm5708_update_reg(charger->i2c, SM5708_REG_CHGCNTL6, (offset << 6), 0xC0);
	if (ret < 0) {
		pr_err("fail to update REG:SM5708_REG_CHGCNTL6 in AICLTH[7:6]\n");
		return ret;
	}

	return 0;
}


static int sm5708_CHG_enable_AICL(struct sm5708_charger_data *charger, bool enable)
{
	int ret;

	ret = sm5708_update_reg(charger->i2c, SM5708_REG_CHGCNTL6, (enable << 5), (1 << 5));
	if (ret < 0) {
		pr_err("fail to update REG:SM5708_REG_CHGCNTL6 in AICLEN[5]\n");
		return ret;
	}

	return 0;
}

static int sm5708_CHG_set_BST_IQ3LIMIT(struct sm5708_charger_data *charger,
				unsigned char index)
{
	if (index > SM5708_CHG_BST_IQ3LIMIT_4_0A) {
		pr_err("invalid limit current index (index=0x%x)\n", index);
		return -EINVAL;
	}

	sm5708_update_reg(charger->i2c, SM5708_REG_CHGCNTL5, ((index & 0x3) << 0), (0x3 << 0));

	pr_info("BST IQ3LIMIT set (index=0x%x)\n", index);

	return 0;
}

static int sm5708_CHG_enable_AUTOSET(struct sm5708_charger_data *charger, bool enable)
{
	int ret;

	ret = sm5708_update_reg(charger->i2c, SM5708_REG_CHGCNTL6, (enable << 1), (1 << 1));
	if (ret < 0) {
		pr_err("fail to update REG:SM5708_REG_CHGCNTL6 in AUTOSET[1]\n");
		return ret;
	}

	return 0;
}

static unsigned char _calc_FASTCHG_current_offset_to_mA(unsigned short mA)
{
	unsigned char offset;

	if (mA < 200) {
		offset = 0x02;
	} else {
		mA = (mA > 3250) ? 3250 : mA;
		offset = ((mA - 100) / 50) & 0x3F;
	}

	return offset;
}

static int sm5708_CHG_set_FASTCHG(struct sm5708_charger_data *charger,
				unsigned short FASTCHG_mA)
{
	unsigned char offset = _calc_FASTCHG_current_offset_to_mA(FASTCHG_mA);

	pr_info("VBUS_FASTCHG, current=%dmA offset=0x%x\n", FASTCHG_mA, offset);

	sm5708_write_reg(charger->i2c, SM5708_REG_CHGCNTL2, offset);

	return 0;
}

static unsigned char _calc_INPUT_LIMIT_current_offset_to_mA(unsigned short mA)
{
	unsigned char offset;

	if (mA < 100) {
		offset = 0x10;
	} else {
		mA = (mA > 3275) ? 3275 : mA;
		offset = ((mA - 100) / 25) & 0x7F;       /* max = 3275mA */
	}

	return offset;
}

static int sm5708_CHG_set_INPUT_LIMIT(struct sm5708_charger_data *charger, unsigned short LIMIT_mA)
{
	unsigned char offset = _calc_INPUT_LIMIT_current_offset_to_mA(LIMIT_mA);

	pr_info("set Input LIMIT, current=%dmA offset=0x%x\n", LIMIT_mA, offset);

	sm5708_write_reg(charger->i2c, SM5708_REG_VBUSCNTL, offset);

	return 0;
}

static unsigned short _calc_INPUT_LIMIT_current_mA_to_offset(unsigned char offset)
{
	return (offset * 25) + 100;
}

static unsigned short sm5708_CHG_get_INPUT_LIMIT(struct sm5708_charger_data *charger)
{
	unsigned short LIMIT_mA;
	unsigned char offset;

	sm5708_read_reg(charger->i2c, SM5708_REG_VBUSCNTL, &offset);

	LIMIT_mA = _calc_INPUT_LIMIT_current_mA_to_offset(offset);

#ifdef SM5708_CHG_FULL_DEBUG
	pr_info("get INPUT LIMIT , offset=0x%x, current=%dmA\n", offset, LIMIT_mA);
#endif

	return LIMIT_mA;
}

static unsigned short _calc_FASTCHG_current_mA_to_offset(unsigned char offset)
{
	return (offset * 50) + 100;
}

static unsigned short sm5708_CHG_get_FASTCHG(struct sm5708_charger_data *charger)
{
	unsigned short FASTCHG_mA;
	unsigned char offset;

	sm5708_read_reg(charger->i2c, SM5708_REG_CHGCNTL2, &offset);

	FASTCHG_mA = _calc_FASTCHG_current_mA_to_offset(offset);

	pr_info("get FASTCHG, offset=0x%x, current=%dmA\n", offset, FASTCHG_mA);

	return FASTCHG_mA;
}

/* monitering REG_MAP */
static unsigned char sm5708_CHG_read_reg(struct sm5708_charger_data *charger, unsigned char reg)
{
	unsigned char reg_val = 0x0;

	sm5708_read_reg(charger->i2c, reg, &reg_val);

	return reg_val;
}

static void sm5708_chg_test_read(struct sm5708_charger_data *charger)
{
	char str[1000] = {0,};
	int i;

	for (i = SM5708_REG_INTMSK1; i <= SM5708_REG_CHGCNTL1; i++)
		snprintf(str+strlen(str), 1000, "0x%02X:0x%02x, ", i, sm5708_CHG_read_reg(charger, i));

	snprintf(str+strlen(str), 1000, "0x%02X:0x%02x, ", SM5708_REG_CHGCNTL1, sm5708_CHG_read_reg(charger, SM5708_REG_CHGCNTL1));
	snprintf(str+strlen(str), 1000, "0x%02X:0x%02x, ", SM5708_REG_CHGCNTL2, sm5708_CHG_read_reg(charger, SM5708_REG_CHGCNTL2));

	for (i = SM5708_REG_CHGCNTL3; i <= SM5708_REG_FLED1CNTL1; i++)
		snprintf(str+strlen(str), 1000, "0x%02X:0x%02x, ", i, sm5708_CHG_read_reg(charger, i));

	snprintf(str+strlen(str), 1000, "0x%02X:0x%02x, ", SM5708_REG_FLEDCNTL6, sm5708_CHG_read_reg(charger, SM5708_REG_FLEDCNTL6));
	snprintf(str+strlen(str), 1000, "0x%02X:0x%02x, ", SM5708_REG_SBPSCNTL, sm5708_CHG_read_reg(charger, SM5708_REG_SBPSCNTL));

	pr_info("[CHG] %s\n", str);
}

/**
 *  SM5708 Charger Driver support functions
 */

static bool sm5708_charger_check_oper_otg_mode_on(void)
{
	unsigned char current_status = sm5708_charger_oper_get_current_status();
	bool ret;

	if (current_status & (1 << SM5708_CHARGER_OP_EVENT_OTG)) {
		ret = true;
	} else {
		ret = false;
	}

	return ret;
}

static bool sm5708_charger_get_charging_on_status(struct sm5708_charger_data *charger)
{
	return sm5708_CHG_get_INT_STATUS(charger, SM5708_INT_STATUS2, SM5708_INT_STATUS2_CHGON);
}

static bool sm5708_charger_get_power_source_status(struct sm5708_charger_data *charger)
{
	int gpio = gpio_get_value(charger->pdata->chg_gpio_en);

	return ((gpio & 0x1)  == 0);    /* charging pin active LOW */
}

static int sm5708_get_input_current(struct sm5708_charger_data *charger)
{
	int get_current;

	get_current = sm5708_CHG_get_INPUT_LIMIT(charger);

	pr_info("src_type=%d, current=%d\n", charger->cable_type, get_current);

	return get_current;
}

static int sm5708_get_charge_current(struct sm5708_charger_data *charger)
{
	int get_current;

	get_current = sm5708_CHG_get_FASTCHG(charger);

	pr_info("src_type=%d, current=%d\n", charger->cable_type, get_current);

	return get_current;
}

static void sm5708_enable_charging_on_switch(struct sm5708_charger_data *charger,
				bool enable)
{
	if ((enable == 0) & sm5708_CHG_get_INT_STATUS(charger, SM5708_INT_STATUS1, SM5708_INT_STATUS1_VBUSPOK)) {
		sm5708_CHG_set_FREQSEL(charger, SM5708_BUCK_BOOST_FREQ_1_5MHz);
	} else {
		sm5708_CHG_set_FREQSEL(charger, SM5708_BUCK_BOOST_FREQ_3MHz);
	}

	gpio_direction_output(charger->pdata->chg_gpio_en, !(enable));

	if (!enable) {
		/* SET: charging-off condition */
		charger->charging_current = 0;
		charger->topoff_pending = 0;
	}

	pr_info("turn-%s Charging enable pin\n", enable ? "ON" : "OFF");
}

static int sm5708_set_charge_current(struct sm5708_charger_data *charger,
				unsigned short charge_current)
{
	sm5708_CHG_set_FASTCHG(charger, charge_current);

	return 0;
}

static int sm5708_set_input_current(struct sm5708_charger_data *charger,
				unsigned short input_current)
{
	if (!input_current) {
		pr_info("skip process, input_current = 0\n");
		return 0;
	}
	sm5708_CHG_set_INPUT_LIMIT(charger, input_current);
	return 0;
}


/**
 *  SM5708 Power-supply class management functions
 */

static void psy_chg_set_charging_enabled(struct sm5708_charger_data *charger, int charge_mode)
{
	int buck_state = true;

	dev_info(charger->dev, "charger_mode changed [%d] -> [%d]\n", charger->charge_mode, charge_mode);

	charger->charge_mode = charge_mode;

	switch (charger->charge_mode) {
	case SEC_BAT_CHG_MODE_BUCK_OFF:
		buck_state = false;
		charger->is_charging = false;
		break;
	case SEC_BAT_CHG_MODE_CHARGING_OFF:
		charger->is_charging = false;
		break;
	case SEC_BAT_CHG_MODE_CHARGING:
		charger->is_charging = true;
		break;
	}
	if (buck_state == false)
		sm5708_charger_oper_push_event(SM5708_CHARGER_OP_EVENT_SUSPEND_MODE, true);
	else
		sm5708_charger_oper_push_event(SM5708_CHARGER_OP_EVENT_SUSPEND_MODE, false);
	sm5708_enable_charging_on_switch(charger, charger->is_charging);
}

static void psy_chg_set_cable_online(struct sm5708_charger_data *charger, int cable_type)
{
	int prev_cable_type = charger->cable_type;
	union power_supply_propval value;

	charger->slow_late_chg_mode = false;
	charger->cable_type = cable_type;

	charger->input_current = sm5708_get_input_current(charger);

	pr_info("[start] prev_cable_type(%d), cable_type(%d), op_mode(%d), op_status(0x%x)",
		prev_cable_type, charger->cable_type,
		sm5708_charger_oper_get_current_op_mode(),
		sm5708_charger_oper_get_current_status());

	if (charger->cable_type == SEC_BATTERY_CABLE_POWER_SHARING) {
		psy_do_property("ps", get, POWER_SUPPLY_PROP_STATUS, value);
		sm5708_charger_oper_push_event(SM5708_CHARGER_OP_EVENT_PWR_SHAR, value.intval);
	} else if (charger->cable_type == SEC_BATTERY_CABLE_OTG) {
		pr_info("OTG enable, cable(%d)\n", charger->cable_type);
	} else if (charger->cable_type == SEC_BATTERY_CABLE_NONE) {
		/* set default value */
		sm5708_charger_oper_push_event(SM5708_CHARGER_OP_EVENT_VBUS, 0);
		sm5708_charger_oper_push_event(SM5708_CHARGER_OP_EVENT_PWR_SHAR, 0);
		sm5708_charger_oper_push_event(SM5708_CHARGER_OP_EVENT_OTG, 0);
		/* set default input current */
		sm5708_set_input_current(charger, 500);

		cancel_delayed_work(&charger->aicl_work);
	} else {
		sm5708_charger_oper_push_event(SM5708_CHARGER_OP_EVENT_VBUS, 1);
#if defined(SM5708_SW_SOFT_START)
		if (prev_cable_type == POWER_SUPPLY_TYPE_BATTERY) {
			wake_lock(&charger->softstart_wake_lock);
			sm5708_set_input_current(charger, 100);
			msleep(50);
			wake_unlock(&charger->softstart_wake_lock);
		}
#endif
			pr_info("request aicl work for 5V-TA\n");
			queue_delayed_work(charger->wqueue, &charger->aicl_work, msecs_to_jiffies(3000));
	}

	pr_info("[end] is_charging=%d(%d), fc = %d, il = %d, cable = %d,"
		"op_mode(%d), op_status(0x%x)\n",
		charger->is_charging, sm5708_charger_get_power_source_status(charger),
		charger->charging_current,
		charger->input_current,
		charger->cable_type,
		sm5708_charger_oper_get_current_op_mode(),
		sm5708_charger_oper_get_current_status());
}

static void psy_chg_set_usb_hc(struct sm5708_charger_data *charger, int value)
{
	if (value) {
		/* set input/charging current for usb up to TA's current */
		charger->pdata->charging_current[SEC_BATTERY_CABLE_USB].fast_charging_current =
			charger->pdata->charging_current[SEC_BATTERY_CABLE_TA].fast_charging_current;
		charger->pdata->charging_current[SEC_BATTERY_CABLE_USB].input_current_limit =
			charger->pdata->charging_current[SEC_BATTERY_CABLE_TA].input_current_limit;
	} else {
		/* restore input/charging current for usb */
		charger->pdata->charging_current[SEC_BATTERY_CABLE_USB].fast_charging_current =
			charger->pdata->charging_current[SEC_BATTERY_CABLE_NONE].input_current_limit;
		charger->pdata->charging_current[SEC_BATTERY_CABLE_USB].input_current_limit =
			charger->pdata->charging_current[SEC_BATTERY_CABLE_NONE].input_current_limit;
	}
}

#if defined(SM5708_SUPPORT_OTG_CONTROL)
static void psy_chg_set_charge_otg_control(struct sm5708_charger_data *charger, int otg_en)
{
	if (otg_en) {
		/* OTG - Enable */
		sm5708_charger_oper_push_event(SM5708_CHARGER_OP_EVENT_VBUS, false);
		sm5708_charger_oper_push_event(SM5708_CHARGER_OP_EVENT_OTG, true);
		pr_info("OTG enable, cable(%d)\n", charger->cable_type);
	} else {
		/* OTG - Disable */
		if (charger->cable_type == SEC_BATTERY_CABLE_NONE)
			sm5708_charger_oper_push_event(SM5708_CHARGER_OP_EVENT_VBUS, false);
		sm5708_charger_oper_push_event(SM5708_CHARGER_OP_EVENT_OTG, false);
		pr_info("OTG disable, cable(%d)\n", charger->cable_type);
	}
}
#endif

static int sm5708_chg_set_property(struct power_supply *psy,
				enum power_supply_property psp, const union power_supply_propval *val)
{
	struct sm5708_charger_data *charger =
		container_of(psy, struct sm5708_charger_data, psy_chg);

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		dev_info(charger->dev, "CHG:POWER_SUPPLY_PROP_STATUS - status=%d\n", val->intval);
		charger->status = val->intval;
		break;
	case POWER_SUPPLY_PROP_CHARGING_ENABLED:
		dev_info(charger->dev, "CHG:POWER_SUPPLY_PROP_CHARGING_ENABLED (val=%d)\n", val->intval);
		psy_chg_set_charging_enabled(charger, val->intval);
		sm5708_chg_test_read(charger);
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		psy_chg_set_cable_online(charger, val->intval);
		break;
	/* set input current */
	case POWER_SUPPLY_PROP_CURRENT_MAX:
		dev_info(charger->dev, "CHG:POWER_SUPPLY_PROP_CURRENT_MAX (val=%d)\n", val->intval);
		charger->input_current = val->intval;
		sm5708_set_input_current(charger, charger->input_current);
		break;
	/* set charge current */
	case POWER_SUPPLY_PROP_CURRENT_AVG:
		dev_info(charger->dev, "CHG:POWER_SUPPLY_PROP_CURRENT_AVG (val=%d)\n", val->intval);
		charger->charging_current = val->intval;
		sm5708_set_charge_current(charger, charger->charging_current);
		break;
	/* set charge current but not for wireless */
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		dev_info(charger->dev, "CHG:POWER_SUPPLY_PROP_CURRENT_NOW (val=%d)\n", val->intval);
		if (!is_wireless_type(charger->cable_type)) {
			charger->charging_current = val->intval;
			sm5708_set_charge_current(charger, charger->charging_current);
		}
		break;
	case POWER_SUPPLY_PROP_CURRENT_FULL:
		dev_info(charger->dev, "CHG:POWER_SUPPLY_PROP_CURRENT_FULL (val=%d)\n", val->intval);
		sm5708_CHG_set_TOPOFF(charger, val->intval);
		break;
#if defined(CONFIG_BATTERY_SWELLING)
	case POWER_SUPPLY_PROP_VOLTAGE_MAX:
#if defined(CONFIG_SEC_FACTORY)
		// in case of open the batt therm on sub pcb, keep the default float voltage
		dev_info(charger->dev, "keep the default float voltage\n");
		break;
#else
		dev_info(charger->dev, "CHG:POWER_SUPPLY_PROP_VOLTAGE_MAX (val=%d)\n", val->intval);
		charger->pdata->chg_float_voltage = val->intval;
		sm5708_CHG_set_BATREG(charger, val->intval);
		break;
#endif
#endif
	case POWER_SUPPLY_PROP_USB_HC:
		pr_info("POWER_SUPPLY_PROP_USB_HC - value=%d\n", val->intval);
		psy_chg_set_usb_hc(charger, val->intval);
		break;
#if defined(SM5708_SUPPORT_OTG_CONTROL)
	case POWER_SUPPLY_PROP_CHARGE_OTG_CONTROL:
		pr_info("POWER_SUPPLY_PROP_CHARGE_OTG_CONTROL - otg_en=%d\n", val->intval);
		psy_chg_set_charge_otg_control(charger, val->intval);
		break;
#endif
	case POWER_SUPPLY_PROP_CHARGE_NOW:
		return -EINVAL;
#if defined(SM5708_SUPPORT_AICL_CONTROL)
	case POWER_SUPPLY_PROP_CHARGE_AICL_CONTROL:
		pr_info("POWER_SUPPLY_PROP_CHARGE_AICL_CONTROL - aicl_en=%d\n", val->intval);
		psy_chg_set_aicl_control(charger, val->intval);
		break;
#endif
	default:
		pr_err("un-known Power-supply property type (psp=%d)\n", psp);
		return -EINVAL;
	}

	return 0;
}

static int psy_chg_get_charge_source_type(struct sm5708_charger_data *charger)
{
	int src_type;

	if (sm5708_CHG_get_INT_STATUS(charger, SM5708_INT_STATUS1, SM5708_INT_STATUS1_VBUSPOK))
		src_type = SEC_BATTERY_CABLE_TA;
	else
		src_type = SEC_BATTERY_CABLE_NONE;

	return src_type;
}

static bool _decide_charge_full_status(struct sm5708_charger_data *charger)
{
	if ((sm5708_CHG_get_INT_STATUS(charger, SM5708_INT_STATUS2, SM5708_INT_STATUS2_TOPOFF)) ||
		(sm5708_CHG_get_INT_STATUS(charger, SM5708_INT_STATUS2, SM5708_INT_STATUS2_DONE))) {
		return charger->topoff_pending;
	}

	return false;
}

static int psy_chg_get_charger_state(struct sm5708_charger_data *charger)
{
	int status = POWER_SUPPLY_STATUS_UNKNOWN;

	if (_decide_charge_full_status(charger)) {
		status = POWER_SUPPLY_STATUS_FULL;
	} else if (sm5708_charger_get_charging_on_status(charger)) {
		status = POWER_SUPPLY_STATUS_CHARGING;
	} else {
		if (sm5708_charger_get_power_source_status(charger)) {
			status = POWER_SUPPLY_STATUS_NOT_CHARGING;
		} else {
			status = POWER_SUPPLY_STATUS_DISCHARGING;
		}
	}

	return status;
}

static int psy_chg_get_charge_type(struct sm5708_charger_data *charger)
{
	int charge_type;

	if (charger->is_charging) {
		if (charger->slow_late_chg_mode) {
			dev_info(charger->dev, "%s: slow late charge mode\n", __func__);
			charge_type = POWER_SUPPLY_CHARGE_TYPE_SLOW;
		} else {
			charge_type = POWER_SUPPLY_CHARGE_TYPE_FAST;
		}
	} else {
		charge_type = POWER_SUPPLY_CHARGE_TYPE_NONE;
	}

	return charge_type;
}

static int psy_chg_get_charging_health(struct sm5708_charger_data *charger)
{
	int state;
	unsigned char reg_data;
	unsigned char reg2_data;

	sm5708_read_reg(charger->i2c, SM5708_REG_STATUS1, &reg_data);
	sm5708_read_reg(charger->i2c, SM5708_REG_STATUS2, &reg2_data);

	pr_info("is_charging=%d(%d), cable_type=%d, input_limit=%d, chg_curr=%d, REG_STATUS1=0x%x, REG_STATUS2=0x%x\n",
			charger->is_charging, sm5708_charger_get_power_source_status(charger),
			charger->cable_type, charger->input_current, charger->charging_current,
			reg_data, reg2_data);

	if (reg_data & (1 << SM5708_INT_STATUS1_VBUSPOK)) {
			state = POWER_SUPPLY_HEALTH_GOOD;
		} else if (reg_data & (1 << SM5708_INT_STATUS1_VBUSOVP)) {
			state = POWER_SUPPLY_HEALTH_OVERVOLTAGE;
		} else if (reg_data & (1 << SM5708_INT_STATUS1_VBUSUVLO)) {
			state = POWER_SUPPLY_HEALTH_UNDERVOLTAGE;
		} else {
			state = POWER_SUPPLY_HEALTH_UNKNOWN;
	}

	sm5708_chg_test_read(charger);

	return (int)state;
}

static int sm5708_chg_create_attrs(struct device *dev)
{
	int i, rc;

	for (i = 0; i < ARRAY_SIZE(sm5708_charger_attrs); i++) {
		rc = device_create_file(dev, &sm5708_charger_attrs[i]);
		if (rc)
			goto create_attrs_failed;
	}
	return rc;

create_attrs_failed:
	pr_err("failed (%d)\n", rc);
	while (i--)
		device_remove_file(dev, &sm5708_charger_attrs[i]);
	return rc;
}

ssize_t sm5708_chg_show_attrs(struct device *dev, struct device_attribute *attr, char *buf)
{
	const ptrdiff_t offset = attr - sm5708_charger_attrs;
	struct power_supply *psy = dev_get_drvdata(dev);
	struct sm5708_charger_data *charger = container_of(psy, struct sm5708_charger_data, psy_chg);
	int i = 0;
	unsigned char reg_data = 0;
	u8 addr = 0, data = 0;

	switch (offset) {
	case CHIP_ID:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n", "SM5708");
		break;
	case CHARGER_OP_MODE:
	    sm5708_read_reg(charger->i2c, SM5708_REG_CNTL, &reg_data);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", reg_data);
		break;
	case DATA:
		for (addr = SM5708_REG_STATUS1; addr <= SM5708_REG_CHGCNTL7; addr++) {
			sm5708_read_reg(charger->i2c, addr, &data);
			i += scnprintf(buf + i, PAGE_SIZE - i, "0x%02x : 0x%02x\n", addr, data);
		}
		break;
	default:
		return -EINVAL;
	}
	return i;
}

ssize_t sm5708_chg_store_attrs(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	const ptrdiff_t offset = attr - sm5708_charger_attrs;
	int ret = 0;
	int x = 0, y = 0;
	struct power_supply *psy = dev_get_drvdata(dev);
	struct sm5708_charger_data *charger = container_of(psy, struct sm5708_charger_data, psy_chg);

	switch (offset) {
	case CHIP_ID:
		ret = count;
		break;
	case CHARGER_OP_MODE:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			if (x == 2) {
				sm5708_update_reg(charger->i2c, SM5708_REG_CNTL, SM5708_CHARGER_OP_MODE_SUSPEND, 0x07);
			} else if (x == 1) {
				sm5708_update_reg(charger->i2c, SM5708_REG_CNTL, SM5708_CHARGER_OP_MODE_CHG_ON, 0x07);
			} else {
				pr_info("change charger op mode fail\n");
				return -EINVAL;
			}
			ret = count;
		}
		break;
	case DATA:
		if (sscanf(buf, "0x%8x 0x%8x", &x, &y) == 2) {
			if (x >= SM5708_REG_STATUS1 && x <= SM5708_REG_CHGCNTL7) {
				u8 addr = x;
				u8 data = y;
				if (sm5708_write_reg(charger->i2c, addr, data) < 0) {
					dev_info(charger->dev,
							"%s: addr: 0x%x write fail\n", __func__, addr);
				}
			} else {
				dev_info(charger->dev,
						"%s: addr: 0x%x is wrong\n", __func__, x);
			}
		}
		ret = count;
		break;
	default:
		ret = -EINVAL;
	}
	return ret;
}

static int sm5708_chg_get_property(struct power_supply *psy,
				enum power_supply_property psp, union power_supply_propval *val)
{
	struct sm5708_charger_data *charger =
		container_of(psy, struct sm5708_charger_data, psy_chg);
	enum power_supply_ext_property ext_psp = psp;

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = psy_chg_get_charge_source_type(charger);
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		if (sm5708_CHG_get_INT_STATUS(charger, SM5708_INT_STATUS2, SM5708_INT_STATUS2_NOBAT))
			val->intval = 0;
		else
			val->intval = 1;
		break;
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = psy_chg_get_charger_state(charger);
		break;
	case POWER_SUPPLY_PROP_CHARGE_TYPE:
		val->intval = psy_chg_get_charge_type(charger);
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = psy_chg_get_charging_health(charger);
		sm5708_chg_test_read(charger);
		break;
	/* get input current which was set */
	case POWER_SUPPLY_PROP_CURRENT_MAX:
		val->intval = charger->input_current;
		break;
	/* get input current which was read */
	case POWER_SUPPLY_PROP_CURRENT_AVG:
		val->intval = sm5708_get_input_current(charger);
		break;
	/* get charge current which was set */
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		val->intval = charger->charging_current;
		break;
	/* get charge current which was read */
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
		val->intval = sm5708_get_charge_current(charger);
		break;
#if defined(CONFIG_BATTERY_SWELLING)
	/* get float voltage */
	case POWER_SUPPLY_PROP_VOLTAGE_MAX:
		val->intval = sm5708_CHG_get_BATREG(charger);
		break;
#endif

#if defined(SM5708_SUPPORT_OTG_CONTROL)
	case POWER_SUPPLY_PROP_CHARGE_OTG_CONTROL:
		val->intval = sm5708_charger_check_oper_otg_mode_on();

		break;
#endif
	case POWER_SUPPLY_PROP_CHARGING_ENABLED:
		val->intval = sm5708_charger_get_power_source_status(charger);
		break;
#if defined(SM5708_SUPPORT_AICL_CONTROL)
	case POWER_SUPPLY_PROP_CHARGE_AICL_CONTROL:
		return -ENODATA;
#endif
	case POWER_SUPPLY_PROP_ENERGY_NOW:
		return -ENODATA;
	case POWER_SUPPLY_PROP_MAX ... POWER_SUPPLY_EXT_PROP_MAX:
			switch (ext_psp) {
			case POWER_SUPPLY_EXT_PROP_CHIP_ID:
				{
					u8 reg_data;
					val->intval = (sm5708_read_reg(charger->i2c, SM5708_REG_CHGCNTL4, &reg_data) == 0);
				}
				break;
			default:
				return -EINVAL;
			}
			break;
	default:
		pr_err("un-known Power-supply property type (psp=%d)\n", psp);
		return -EINVAL;
	}

	return 0;
}

static int sm5708_otg_get_property(struct power_supply *psy, enum power_supply_property psp, union power_supply_propval *val)
{
	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = sm5708_charger_check_oper_otg_mode_on();

		pr_info("POWER_SUPPLY_PROP_ONLINE - %s\n", (val->intval) ? "ON" : "OFF");
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int sm5708_otg_set_property(struct power_supply *psy,
					enum power_supply_property psp, const union power_supply_propval *val)
{
	struct sm5708_charger_data *charger =
		container_of(psy, struct sm5708_charger_data, psy_otg);
	union power_supply_propval value;

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		pr_info("POWER_SUPPLY_PROP_ONLINE - %s\n", (val->intval) ? "ON" : "OFF");
#if defined(SM5708_SUPPORT_OTG_CONTROL)
		value.intval = val->intval;
		psy_do_property("sm5708-charger", set, POWER_SUPPLY_PROP_CHARGE_OTG_CONTROL, value);
#else
		if (val->intval) {
			value.intval = SEC_BATTERY_CABLE_OTG;
		} else {
			value.intval = SEC_BATTERY_CABLE_NONE;
		}
		psy_do_property("sm5708-charger", set, POWER_SUPPLY_PROP_ONLINE, value);
#endif
		power_supply_changed(&charger->psy_otg);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static enum power_supply_property sm5708_otg_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
};


#if EN_TOPOFF_IRQ
static void sm5708_topoff_work(struct work_struct *work)
{
	struct sm5708_charger_data *charger =
		container_of(work, struct sm5708_charger_data, topoff_work.work);
	bool topoff = 1;
	int i;

	pr_info("schedule work start.\n");

	for (i = 0; i < 3; ++i) {
		topoff &= sm5708_CHG_get_INT_STATUS(charger, SM5708_INT_STATUS2, SM5708_INT_STATUS2_TOPOFF);
		msleep(150);

		pr_info("%dth Check TOP-OFF state=%d\n", i, topoff);
	}

	charger->topoff_pending = topoff;

	pr_info("schedule work done.\n");
}
#endif /*EN_TOPOFF_IRQ*/

static void _reduce_input_limit_current(struct sm5708_charger_data *charger, int cur)
{
	unsigned short vbus_limit_current = sm5708_CHG_get_INPUT_LIMIT(charger);

	if ((vbus_limit_current <= MINIMUM_INPUT_CURRENT) || (vbus_limit_current <= cur)) {
		return;
	}

	vbus_limit_current = ((vbus_limit_current - cur) < MINIMUM_INPUT_CURRENT) ?
		MINIMUM_INPUT_CURRENT : vbus_limit_current - cur;
	sm5708_CHG_set_INPUT_LIMIT(charger, vbus_limit_current);

	charger->input_current = sm5708_get_input_current(charger);

	pr_info("vbus_limit_current=%d, charger->input_current=%d\n",
		vbus_limit_current, charger->input_current);
}

static void _check_slow_charging(struct sm5708_charger_data *charger)
{
	union power_supply_propval value;
	/* under 400mA considered as slow charging concept for VZW */
	if (charger->input_current <= SLOW_CHARGING_CURRENT_STANDARD &&
			charger->cable_type != SEC_BATTERY_CABLE_NONE) {

		dev_info(charger->dev, "slow-rate charging on : input current(%dmA), cable-type(%d)\n",
				charger->input_current, charger->cable_type);

		charger->slow_late_chg_mode = true;
		value.intval = POWER_SUPPLY_CHARGE_TYPE_SLOW;
		psy_do_property("battery", set, POWER_SUPPLY_PROP_CHARGE_TYPE, value);
	}
}

static bool _check_aicl_state(struct sm5708_charger_data *charger)
{
	return sm5708_CHG_get_INT_STATUS(charger, SM5708_INT_STATUS2, SM5708_INT_STATUS2_AICL);
}

static void sm5708_aicl_work(struct work_struct *work)
{
	struct sm5708_charger_data *charger =
		container_of(work, struct sm5708_charger_data, aicl_work.work);
	int prev_current_max, max_count, now_count = 0;


	if (!sm5708_charger_get_charging_on_status(charger)) {

		pr_info("don't need AICL work\n");
		return;
	}

	dev_info(charger->dev, "%s - start\n", __func__);

	/* Reduce input limit current */
	max_count = charger->input_current / REDUCE_CURRENT_STEP;
	prev_current_max = charger->input_current;
	while (_check_aicl_state(charger) && (now_count++ < max_count)) {
		_reduce_input_limit_current(charger, REDUCE_CURRENT_STEP);
		msleep(AICL_VALID_CHECK_DELAY_TIME);
	}

	if (_check_aicl_state(charger)) {
		union power_supply_propval value;
		value.intval = sm5708_CHG_get_INPUT_LIMIT(charger);
		psy_do_property("battery", set,
			POWER_SUPPLY_EXT_PROP_AICL_CURRENT, value);
	}

	if (prev_current_max > charger->input_current) {
		pr_info("input_current(%d --> %d)\n",
			prev_current_max, charger->input_current);
		_check_slow_charging(charger);
	}

	dev_info(charger->dev, "%s - done\n", __func__);
}

static unsigned char _get_valid_vbus_status(struct sm5708_charger_data *charger)
{
	unsigned char vbusin, prev_vbusin = 0xff;
	int stable_count = 0;

	while (1) {
		sm5708_read_reg(charger->i2c, SM5708_REG_STATUS1, &vbusin);
		vbusin &= 0xF;

		if (prev_vbusin == vbusin) {
			stable_count++;
		} else {
			pr_info("VBUS status mismatch (0x%x / 0x%x), Reset stable count\n",	vbusin, prev_vbusin);
			stable_count = 0;
		}

		if (stable_count == 10) {
			break;
		}

		prev_vbusin = vbusin;
		msleep(10);
	}

	return vbusin;
}

static int _check_vbus_power_supply_status(struct sm5708_charger_data *charger,
			unsigned char vbus_status, int prev_battery_health)
{
	int battery_health = prev_battery_health;

	if (vbus_status & (1 << SM5708_INT_STATUS1_VBUSPOK)) {
		if (prev_battery_health == POWER_SUPPLY_HEALTH_OVERVOLTAGE) {
			pr_info("overvoltage->normal\n");
			battery_health = POWER_SUPPLY_HEALTH_GOOD;
		} else if (prev_battery_health == POWER_SUPPLY_HEALTH_UNDERVOLTAGE) {
			pr_info("undervoltage->normal\n");
			battery_health = POWER_SUPPLY_HEALTH_GOOD;
		}
	} else {
		if ((vbus_status & (1 << SM5708_INT_STATUS1_VBUSOVP)) &&
			(prev_battery_health != POWER_SUPPLY_HEALTH_OVERVOLTAGE)) {
			pr_info("charger is over voltage\n");
			battery_health = POWER_SUPPLY_HEALTH_OVERVOLTAGE;

		} else if ((vbus_status & (1 << SM5708_INT_STATUS1_VBUSUVLO)) &&
			(prev_battery_health != POWER_SUPPLY_HEALTH_UNDERVOLTAGE) &&
			(charger->cable_type != SEC_BATTERY_CABLE_NONE)) {
			pr_info("vBus is undervoltage\n");
			battery_health = POWER_SUPPLY_HEALTH_UNDERVOLTAGE;
		}
	}

	return battery_health;
}

static irqreturn_t sm5708_chg_done_isr(int irq, void *data)
{
	struct sm5708_charger_data *charger = data;

	pr_info("%s: start.\n", __func__);

	/* nCHG pin toggle */
	gpio_direction_output(charger->pdata->chg_gpio_en, charger->is_charging);
	msleep(10);
	/* gpio_direction_output(charger->pdata->chg_gpio_en, !(charger->is_charging)); */

	return IRQ_HANDLED;
}

static irqreturn_t sm5708_chg_vbus_in_isr(int irq, void *data)
{
	struct sm5708_charger_data *charger = data;
	union power_supply_propval value;
	unsigned char vbus_status;
	int prev_battery_health;

	pr_info("start.\n");

	vbus_status = _get_valid_vbus_status(charger);

	psy_do_property("battery", get, POWER_SUPPLY_PROP_HEALTH, value);
	prev_battery_health = value.intval;

	value.intval = _check_vbus_power_supply_status(charger, vbus_status, prev_battery_health);
	if (prev_battery_health != value.intval) {
		psy_do_property("battery", set, POWER_SUPPLY_PROP_HEALTH, value);
	}
	pr_info("battery change status [%d] -> [%d] (VBUS_REG:0x%x)\n",
		prev_battery_health, value.intval, vbus_status);

	pr_info("done.\n");

	return IRQ_HANDLED;
}

static irqreturn_t sm5708_chg_otgfail_isr(int irq, void *data)
{
	struct sm5708_charger_data *charger = data;
	unsigned char reg_data;

#ifdef CONFIG_USB_HOST_NOTIFY
	struct otg_notify *o_notify = get_otg_notify();
#endif
	pr_info("IRQ=%d : OTG Failed\n", irq);

	sm5708_read_reg(charger->i2c, SM5708_REG_STATUS3, &reg_data);
	if (reg_data & (1 << 2)) {
		pr_info("otg overcurrent limit\n");
#ifdef CONFIG_USB_HOST_NOTIFY
		send_otg_notify(o_notify, NOTIFY_EVENT_OVERCURRENT, 0);
#endif
		psy_chg_set_charge_otg_control(charger, false);
	}
	return IRQ_HANDLED;
}

/**
 *  SM5708 Charger driver management functions
 **/

#ifdef CONFIG_OF
static int _parse_sm5708_charger_node_propertys(struct device *dev,
				struct device_node *np, sec_charger_platform_data_t *pdata)
{
	int ret;

	pdata->chg_gpio_en = of_get_named_gpio(np, "battery,chg_gpio_en", 0); //nCHGEN
	if (IS_ERR_VALUE(pdata->chg_gpio_en)) {
		pr_info("can't parsing dt:battery,chg_gpio_en\n");
		return -ENOENT;
	}
	pr_info("battery charge enable pin = %d\n", pdata->chg_gpio_en);

	ret = of_property_read_u32(np, "battery,chg_float_voltage", &pdata->chg_float_voltage);
	if (IS_ERR_VALUE(ret)) {
		dev_err(dev, "%s: can't parsing dt:battery,chg_float_voltage\n", __func__);
	}

	return 0;
}

static int _parse_battery_node_propertys(struct device *dev, struct device_node *np, sec_charger_platform_data_t *pdata)
{
	int ret = 0, i = 0, len = 0;
	const u32 *p;
	u32 temp = 0;


	ret = of_property_read_u32(np, "battery,full_check_type_2nd", &pdata->full_check_type_2nd);
	if (IS_ERR_VALUE(ret)) {
		dev_err(dev, "%s: can't parsing dt:battery,full_check_type_2nd\n", __func__);
	}

	np = of_find_node_by_name(NULL, "cable-info");
	if (!np) {
		pr_err ("%s : np NULL\n", __func__);
	} else {
		struct device_node *child;
		u32 input_current = 0, charging_current = 0;

		ret = of_property_read_u32(np, "default_input_current", &input_current);
		ret = of_property_read_u32(np, "default_charging_current", &charging_current);

		pdata->charging_current =
			kzalloc(sizeof(sec_charging_current_t) * SEC_BATTERY_CABLE_MAX,
				GFP_KERNEL);

		for (i = 0; i < SEC_BATTERY_CABLE_MAX; i++) {
			pdata->charging_current[i].input_current_limit = (unsigned int)input_current;
			pdata->charging_current[i].fast_charging_current = (unsigned int)charging_current;
		}

		for_each_child_of_node(np, child) {
			ret = of_property_read_u32(child, "input_current", &input_current);
			ret = of_property_read_u32(child, "charging_current", &charging_current);

			p = of_get_property(child, "cable_number", &len);
			if (!p)
				return 1;

			len = len / sizeof(u32);

			for (i = 0; i <= len; i++) {
				ret = of_property_read_u32_index(child, "cable_number", i, &temp);
				pdata->charging_current[temp].input_current_limit = (unsigned int)input_current;
				pdata->charging_current[temp].fast_charging_current = (unsigned int)charging_current;
			}

		}
		for (i = 0; i < SEC_BATTERY_CABLE_MAX; i++) {
			pr_info("%s : CABLE_NUM(%d) INPUT(%d) CHARGING(%d)\n", __func__, i,
			pdata->charging_current[i].input_current_limit,
			pdata->charging_current[i].fast_charging_current);
		}
	}

	pr_info("dt:battery node parse done.\n");

	return 0;
}


static int sm5708_charger_parse_dt(struct sm5708_charger_data *charger,
				struct sec_charger_platform_data *pdata)
{
	struct device_node *np;
	int ret;

	np = of_find_node_by_name(NULL, "sm5708-charger");
	if (np == NULL) {
		pr_err("fail to find dt_node:sm5708-charger\n");
		return -ENOENT;
	} else {
		ret = _parse_sm5708_charger_node_propertys(charger->dev, np, pdata);
	}

	np = of_find_node_by_name(NULL, "battery");
	if (np == NULL) {
		pr_err("fail to find dt_node:battery\n");
		return -ENOENT;
	} else {
		ret = _parse_battery_node_propertys(charger->dev, np, pdata);
		if (IS_ERR_VALUE(ret))
			return ret;
	}

	return ret;
}
#endif

static sec_charger_platform_data_t *_get_sm5708_charger_platform_data
				(struct platform_device *pdev, struct sm5708_charger_data *charger)
{
#ifdef CONFIG_OF
	sec_charger_platform_data_t *pdata;
	int ret;

	pdata = kzalloc(sizeof(sec_charger_platform_data_t), GFP_KERNEL);
	if (!pdata) {
		pr_err("fail to memory allocate for sec_charger_platform_data\n");
		return NULL;
	}

	ret = sm5708_charger_parse_dt(charger, pdata);
	if (IS_ERR_VALUE(ret)) {
		pr_err("fail to parse sm5708 charger device tree (ret=%d)\n", ret);
		kfree(pdata);
		return NULL;
	}
#else
	struct sm5708_platform_data *sm5708_pdata = dev_get_platdata(sm5708->dev);
	sec_charger_platform_data_t *pdata;

	pdata = sm5708_pdata->charger_data;
	if (!pdata) {
		pr_err("fail to get sm5708 charger platform data\n");
		return NULL;
	}
#endif

	pr_info("Get valid platform data done. (pdata=%p)\n", pdata);
	return pdata;
}

static int _init_sm5708_charger_info(struct platform_device *pdev,
	struct sm5708_dev *sm5708, struct sm5708_charger_data *charger)
{
	struct sm5708_platform_data *pdata = dev_get_platdata(sm5708->dev);
	int ret;

	mutex_init(&charger->charger_mutex);

	if (pdata == NULL) {
		pr_err("can't get sm5708_platform_data\n");
		return -EINVAL;
	}

	pr_info("init process start..\n");

	/* setup default charger configuration parameter & flagment */

	charger->input_current = 500;
	charger->topoff_pending = false;
	charger->is_charging = false;
	charger->cable_type = SEC_BATTERY_CABLE_NONE;
	charger->is_mdock = false;
	charger->slow_late_chg_mode = false;

	/* Request GPIO pin - CHG_IN */
	if (charger->pdata->chg_gpio_en) {
		ret = gpio_request(charger->pdata->chg_gpio_en, "sm5708_nCHGEN");
		if (ret) {
			pr_err("fail to request GPIO %u\n", charger->pdata->chg_gpio_en);
			return ret;
		}
	}

	/* initialize delayed workqueue */
	charger->wqueue = create_singlethread_workqueue(dev_name(charger->dev));
	if (!charger->wqueue) {
		pr_err("fail to Create Workqueue\n");
		return -ENOMEM;
	}

	INIT_DELAYED_WORK(&charger->aicl_work, sm5708_aicl_work);
#if EN_TOPOFF_IRQ
	INIT_DELAYED_WORK(&charger->topoff_work, sm5708_topoff_work);
#endif

	wake_lock_init(&charger->check_slow_wake_lock, WAKE_LOCK_SUSPEND, "charger-check-slow");
	wake_lock_init(&charger->aicl_wake_lock, WAKE_LOCK_SUSPEND, "charger-aicl");

	/* Get IRQ service routine number */

	charger->irq_vbus_pok = pdata->irq_base + SM5708_VBUSPOK_IRQ;
	charger->irq_aicl = pdata->irq_base + SM5708_AICL_IRQ;
#if EN_TOPOFF_IRQ
	charger->irq_topoff = pdata->irq_base + SM5708_TOPOFF_IRQ;
#endif
	charger->irq_otgfail = pdata->irq_base + SM5708_OTGFAIL_IRQ;
	charger->irq_done = pdata->irq_base + SM5708_DONE_IRQ;

	pr_info("init process done..\n");

	return 0;
}

static void sm5708_charger_initialize(struct sm5708_charger_data *charger)
{
	pr_info("charger initial hardware condition process start.(float_voltage=%d)\n", charger->pdata->chg_float_voltage);

	/* Auto-Stop configuration for Emergency status */
	sm5708_CHG_set_TOPOFF(charger, 300);
	sm5708_CHG_set_TOPOFF_TMR(charger, SM5708_TOPOFF_TIMER_45m);
	sm5708_CHG_enable_AUTOSTOP(charger, 1);

	sm5708_CHG_set_BATREG(charger, charger->pdata->chg_float_voltage);

	sm5708_CHG_set_AICLTH(charger, 4500);
	sm5708_CHG_enable_AICL(charger, 1);

	sm5708_CHG_enable_AUTOSET(charger, 0);

	sm5708_CHG_set_BST_IQ3LIMIT(charger, SM5708_CHG_BST_IQ3LIMIT_3_5A);

	sm5708_chg_test_read(charger);

	pr_info("charger initial hardware condition process done.\n");
}

static int sm5708_charger_probe(struct platform_device *pdev)
{
	struct sm5708_dev *sm5708 = dev_get_drvdata(pdev->dev.parent);
	struct sm5708_charger_data *charger;
	int ret = 0;

	pr_info("Sm5708 Charger Driver Probing start\n");

	charger = kzalloc(sizeof(struct sm5708_charger_data), GFP_KERNEL);
	if (!charger) {
		pr_err("fail to memory allocate for sm5708 charger handler\n");
		return -ENOMEM;
	}

	charger->dev = &pdev->dev;
	charger->i2c = sm5708->i2c;
	charger->pdata = _get_sm5708_charger_platform_data(pdev, charger);
	if (charger->pdata == NULL) {
		pr_err("fail to get charger platform data\n");
		goto err_free;
	}

	ret = _init_sm5708_charger_info(pdev, sm5708, charger);
	if (IS_ERR_VALUE(ret)) {
		pr_err("can't initailize sm5708 charger");
		goto err_free;
	}
	platform_set_drvdata(pdev, charger);

	sm5708_charger_initialize(charger);

	charger->psy_chg.name = "sm5708-charger";
	charger->psy_chg.type = POWER_SUPPLY_TYPE_UNKNOWN;
	charger->psy_chg.get_property = sm5708_chg_get_property;
	charger->psy_chg.set_property = sm5708_chg_set_property;
	charger->psy_chg.properties	= sm5708_charger_props;
	charger->psy_chg.num_properties	= ARRAY_SIZE(sm5708_charger_props);
	ret = power_supply_register(&pdev->dev, &charger->psy_chg);
	if (ret) {
		pr_err("fail to register psy_chg\n");
		goto err_power_supply_register;
	}

	charger->psy_otg.name = "otg";
	charger->psy_otg.type = POWER_SUPPLY_TYPE_OTG;
	charger->psy_otg.get_property = sm5708_otg_get_property;
	charger->psy_otg.set_property = sm5708_otg_set_property;
	charger->psy_otg.properties	 = sm5708_otg_props;
	charger->psy_otg.num_properties	= ARRAY_SIZE(sm5708_otg_props);
	ret = power_supply_register(&pdev->dev, &charger->psy_otg);
	if (ret) {
		pr_err("fail to register otg_chg\n");
		goto err_power_supply_register_chg;
	}

	/* Operation Mode Initialize */
	sm5708_charger_oper_table_init(charger->i2c);

	/* Request IRQ */

	ret = request_threaded_irq(charger->irq_vbus_pok, NULL,
			sm5708_chg_vbus_in_isr, 0, "chgin-irq", charger);
	if (ret < 0) {
		pr_err("fail to request chgin IRQ: %d: %d\n",
				charger->irq_vbus_pok, ret);
		goto err_power_supply_register_otg;
	}

	ret = request_threaded_irq(charger->irq_done, NULL,
			sm5708_chg_done_isr, 0, "done-irq", charger);
	if (ret < 0) {
		pr_err("fail to request chgin IRQ: %d: %d\n",
				charger->irq_done, ret);
		goto err_power_supply_register_otg;
	}

	ret = request_threaded_irq(charger->irq_otgfail, NULL,
			sm5708_chg_otgfail_isr, 0, "otgfail-irq", charger);
	if (ret < 0) {
		pr_err("fail to request otgfail IRQ: %d: %d\n",
			charger->irq_otgfail, ret);
		goto err_power_supply_register_otg;
	}
	ret = sm5708_chg_create_attrs(charger->psy_chg.dev);
	if (ret) {
		pr_err("Failed to create_attrs\n");
		goto err_power_supply_register_otg;
	}

	pr_info("SM5708 Charger Driver Loaded Done\n");

	return 0;

err_power_supply_register_otg:
	power_supply_unregister(&charger->psy_otg);
err_power_supply_register_chg:
	power_supply_unregister(&charger->psy_chg);
err_power_supply_register:
	destroy_workqueue(charger->wqueue);

	mutex_destroy(&charger->charger_mutex);
err_free:
	kfree(charger);

	return ret;
}

static int sm5708_charger_remove(struct platform_device *pdev)
{
	struct sm5708_charger_data *charger = platform_get_drvdata(pdev);

	cancel_delayed_work(&charger->aicl_work);
#if EN_TOPOFF_IRQ
	cancel_delayed_work(&charger->topoff_work);
#endif
	destroy_workqueue(charger->wqueue);

	free_irq(charger->irq_vbus_pok, NULL);
	free_irq(charger->irq_done, NULL);
	free_irq(charger->irq_otgfail, NULL);

	power_supply_unregister(&charger->psy_chg);
	mutex_destroy(&charger->charger_mutex);
	kfree(charger);
	return 0;
}

static void sm5708_charger_shutdown(struct device *dev)
{
	struct sm5708_charger_data *charger = dev_get_drvdata(dev);

	sm5708_update_reg(charger->i2c, SM5708_REG_CNTL,
			SM5708_CHARGER_OP_MODE_CHG_ON, 0x07);
	pr_info("call shutdown\n");
}

#if defined CONFIG_PM
static int sm5708_charger_suspend(struct device *dev)
{
	pr_info("call suspend\n");
	return 0;
}

static int sm5708_charger_resume(struct device *dev)
{
	pr_info("call resume\n");
	return 0;
}
#else
#define sm5708_charger_suspend NULL
#define sm5708_charger_resume NULL
#endif

static SIMPLE_DEV_PM_OPS(sm5708_charger_pm_ops, sm5708_charger_suspend, sm5708_charger_resume);
static struct platform_driver sm5708_charger_driver = {
	.driver = {
		.name = "sm5708-charger",
		.owner = THIS_MODULE,
		.pm = &sm5708_charger_pm_ops,
		.shutdown = sm5708_charger_shutdown,
	},
	.probe = sm5708_charger_probe,
	.remove = sm5708_charger_remove,
};

static int __init sm5708_charger_init(void)
{
	return platform_driver_register(&sm5708_charger_driver);
}

static void __exit sm5708_charger_exit(void)
{
	platform_driver_unregister(&sm5708_charger_driver);
}


module_init(sm5708_charger_init);
module_exit(sm5708_charger_exit);

MODULE_DESCRIPTION("SM5708 Charger Driver");
MODULE_LICENSE("GPL v2");

