
/*
 *  /drivers/battery_v2/sm5705_charger.c
 *
 *  SM5705 Charger driver for SEC_BATTERY Flatform support
 *
 *  Copyright (C) 2015 Silicon Mitus,
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#define pr_fmt(fmt)	"sm5705-charger: %s: " fmt, __func__

#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/power_supply.h>
#include <linux/mfd/sm5705/sm5705.h>
#include <linux/spinlock.h>
#ifdef CONFIG_USB_HOST_NOTIFY
#include <linux/usb_notify.h>
#endif
#if defined(CONFIG_VBUS_NOTIFIER)
#include <linux/vbus_notifier.h>
#endif
#include <linux/of_gpio.h>

#include "include/charger/sm5705_charger.h"
#include "include/charger/sm5705_charger_oper.h"


//#define SM5705_CHG_FULL_DEBUG 1

#define ENABLE 1
#define DISABLE 0

static enum power_supply_property sm5705_charger_props[] = {
};

static struct device_attribute sm5705_charger_attrs[] = {
	SM5705_CHARGER_ATTR(chip_id),
	SM5705_CHARGER_ATTR(charger_op_mode),
	SM5705_CHARGER_ATTR(data),
};

/**
 *  SM5705 Charger device register control functions
 */

static bool sm5705_CHG_get_INT_STATUS(struct sm5705_charger_data *charger,
				unsigned char index, unsigned char offset)
{
	unsigned char reg_val;
	int ret;

	ret = sm5705_read_reg(charger->i2c, SM5705_REG_STATUS1 + index, &reg_val);
	if (ret < 0) {
		pr_err("fail to I2C read REG:SM5705_REG_INT%d\n", 1 + index);
		return 0;
	}

	reg_val = (reg_val & (1 << offset)) >> offset;

	return reg_val;
}

static int sm5705_CHG_set_TOPOFF_TMR(struct sm5705_charger_data *charger,
				unsigned char topoff_timer)
{
	sm5705_update_reg(charger->i2c,
		SM5705_REG_CHGCNTL8, ((topoff_timer & 0x3) << 3), (0x3 << 3));
	pr_info("TOPOFF_TMR set (timer=%d)\n", topoff_timer);

	return 0;
}

static int sm5705_CHG_enable_AUTOSTOP(struct sm5705_charger_data *charger,
				bool enable)
{
	int ret;

	ret = sm5705_update_reg(charger->i2c, SM5705_REG_CHGCNTL4, (enable << 6), (1 << 6));
	if (ret < 0) {
		pr_err("fail to update REG:SM5705_REG_CHGCNTL4 in AUTOSTOP[6]\n");
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
		offset = 0x14;    /* default Offset : 4.19V */
	}

	return offset;
}

static int sm5705_CHG_set_BATREG(struct sm5705_charger_data *charger,
				unsigned short float_mV)
{
	unsigned char offset = _calc_BATREG_offset_to_float_mV(float_mV);
	int ret;

	pr_info("set BATREG voltage(%dmV - offset=0x%x)\n", float_mV, offset);

	ret = sm5705_update_reg(charger->i2c, SM5705_REG_CHGCNTL4, offset, 0x3F);
	if (ret < 0) {
		pr_err("fail to update REG:SM5705_REG_CHGCNTL4 in BATREG[5:0]\n");
		return ret;
	}

	return 0;
}

static unsigned short _calc_float_mV_to_BATREG_offset(unsigned char offset)
{
	return ((offset - 2) * 10) + 4010;
}

static unsigned short sm5705_CHG_get_BATREG(struct sm5705_charger_data *charger)
{
	unsigned char offset;
	int ret;

	ret = sm5705_read_reg(charger->i2c, SM5705_REG_CHGCNTL4, &offset);
	if (ret < 0) {
		pr_err("fail to read REG:SM5705_REG_CHGCNTL4\n");
		return 0;
	}

	return _calc_float_mV_to_BATREG_offset(offset & 0x3F);
}

static unsigned char _calc_TOPOFF_offset_to_topoff_mA(unsigned short mA)
{
	unsigned char offset;

	if (mA < 100) {
		offset = 0x0;
	} else if (mA < 480) {
		offset = (mA - 100) / 25;
	} else {
		offset = 0xF;
	}

	return offset;
}

static int sm5705_CHG_set_TOPOFF(struct sm5705_charger_data *charger,
				unsigned short topoff_mA)
{
	unsigned char offset = _calc_TOPOFF_offset_to_topoff_mA(topoff_mA);
	int ret;

	pr_info("set TOP-OFF current(%dmA - offset=0x%x)\n", topoff_mA, offset);

	ret = sm5705_update_reg(charger->i2c, SM5705_REG_CHGCNTL5, offset, 0xF);
	if (ret < 0) {
		pr_err("fail to update REG:SM5705_REG_CHGCNTL5 in TOPOFF[3:0]\n");
		return ret;
	}

	return 0;
}

static int sm5705_CHG_set_FREQSEL(struct sm5705_charger_data *charger, unsigned char freq_index)
{
	int ret;

	pr_info("set BUCK&BOOST freq=0x%x\n", freq_index);

	ret = sm5705_update_reg(charger->i2c, SM5705_REG_CHGCNTL5, ((freq_index & 0x3) << 4), (0x3 << 4));
	if (ret < 0) {
		pr_err("fail to update REG:SM5705_REG_CHGCNTL5 in FREQSEL[5:4]\n");
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
		offset = 0x3;
	}

	return offset;
}

static int sm5705_CHG_set_AICLTH(struct sm5705_charger_data *charger,
				unsigned short aiclth_mV)
{
	unsigned char offset = _calc_AICL_threshold_offset_to_mV(aiclth_mV);
	int ret;

	pr_info("set AICL threshold (%dmV - offset=0x%x)\n", aiclth_mV, offset);

	ret = sm5705_update_reg(charger->i2c, SM5705_REG_CHGCNTL7, (offset << 6), 0xC0);
	if (ret < 0) {
		pr_err("fail to update REG:SM5705_REG_CHGCNTL7 in AICLTH[7:6]\n");
		return ret;
	}

	return 0;
}

static int sm5705_CHG_set_OVPSEL(struct sm5705_charger_data *charger, bool enable)
{
	int ret;

	pr_info("set OVPSEL=%d\n", enable);

	ret = sm5705_update_reg(charger->i2c, SM5705_REG_CHGCNTL7, (enable << 2), (1 << 2));
	if (ret < 0) {
		pr_err("fail to update REG:SM5705_REG_CHGCNTL7 in OVPSEL[2]\n");
		return ret;
	}

	return 0;
}

static int sm5705_CHG_enable_AICL(struct sm5705_charger_data *charger, bool enable)
{
	int ret;

	ret = sm5705_update_reg(charger->i2c, SM5705_REG_CHGCNTL7, (enable << 5), (1 << 5));
	if (ret < 0) {
		pr_err("fail to update REG:SM5705_REG_CHGCNTL7 in AICLEN[5]\n");
		return ret;
	}

	return 0;
}

static int sm5705_CHG_set_BST_IQ3LIMIT(struct sm5705_charger_data *charger,
				unsigned char index)
{
	if (index > SM5705_CHG_BST_IQ3LIMIT_4_0A) {
		pr_err("invalid limit current index (index=0x%x)\n", index);
		return -EINVAL;
	}

	sm5705_update_reg(charger->i2c, SM5705_REG_CHGCNTL6, ((index & 0x3)), 0x3);

	pr_info("BST IQ3LIMIT set (index=0x%x)\n", index);

	return 0;
}

#if defined(SM5705_I2C_RESET_ACTIVATE)
static int sm5705_CHG_set_ENI2CRESET(struct sm5705_charger_data *charger, bool enable)
{
	sm5705_update_reg(charger->i2c, SM5705_REG_CHGCNTL6, (enable << 4), (0x1 << 4));
	pr_info("ENI2CRESET set (enable=%d)\n", enable);
	return 0;
}
#endif

#if defined(SM5705_MANUAL_RESET_ACTIVATE)
static int sm5705_CHG_set_ENMRSTB(struct sm5705_charger_data *charger,
				unsigned char timer)
{
	sm5705_update_reg(charger->i2c, SM5705_REG_CHGCNTL8, (timer & 0x3), 0x3);
	pr_info("ENMRSTB set (timer=%d)\n", timer);
	return 0;
}
#endif

static int sm5705_CHG_enable_AUTOSET(struct sm5705_charger_data *charger, bool enable)
{
	int ret;

	ret = sm5705_update_reg(charger->i2c, SM5705_REG_CHGCNTL7, (enable << 1), (1 << 1));
	if (ret < 0) {
		pr_err("fail to update REG:SM5705_REG_CHGCNTL7 in AUTOSET[1]\n");
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

static int sm5705_CHG_set_FASTCHG(struct sm5705_charger_data *charger,
				unsigned char index, unsigned short FASTCHG_mA)
{
	unsigned char offset = _calc_FASTCHG_current_offset_to_mA(FASTCHG_mA);

	pr_info("FASTCHG src=%d, current=%dmA offset=0x%x\n", index, FASTCHG_mA, offset);

	if (index > SM5705_CHG_SRC_WPC) {
		return -EINVAL;
	}

	sm5705_write_reg(charger->i2c, SM5705_REG_CHGCNTL2 + index, offset);

	return 0;
}

static unsigned char _calc_INPUT_LIMIT_current_offset_to_mA(unsigned char index,
				unsigned short mA)
{
	unsigned char offset;

	if (mA < 100) {
		offset = 0x10;
	} else {
		if (index == SM5705_CHG_SRC_VBUS) {
			mA = (mA > 3275) ? 3275 : mA;
			offset = ((mA - 100) / 25) & 0x7F;       /* max = 3275mA */
		} else {
			mA = (mA > 1650) ? 1650 : mA;
			offset = ((mA - 100) / 25) & 0x3F;       /* max = 1650mA */
		}
	}

	return offset;
}

static int sm5705_CHG_set_INPUT_LIMIT(struct sm5705_charger_data *charger,
				unsigned char index, unsigned short LIMIT_mA)
{
	unsigned char offset = _calc_INPUT_LIMIT_current_offset_to_mA(index, LIMIT_mA);

	pr_info("set Input LIMIT src=%d, current=%dmA offset=0x%x\n", index, LIMIT_mA, offset);

	if (index > SM5705_CHG_SRC_WPC) {
		return -EINVAL;
	}

	sm5705_write_reg(charger->i2c, SM5705_REG_VBUSCNTL + index, offset);

	return 0;
}

static unsigned short _calc_INPUT_LIMIT_current_mA_to_offset(unsigned char index,
				unsigned char offset)
{
	return (offset * 25) + 100;
}

static unsigned short sm5705_CHG_get_INPUT_LIMIT(struct sm5705_charger_data *charger,
				unsigned char index)
{
	unsigned short LIMIT_mA;
	unsigned char offset;

	if (index > SM5705_CHG_SRC_WPC) {
		pr_err("invalid charger source index = %d\n", index);
		return 0;
	}

	sm5705_read_reg(charger->i2c, SM5705_REG_VBUSCNTL + index, &offset);

	LIMIT_mA = _calc_INPUT_LIMIT_current_mA_to_offset(index, offset);

#ifdef SM5705_CHG_FULL_DEBUG
	pr_info("get INPUT LIMIT src=%d, offset=0x%x, current=%dmA\n", index, offset, LIMIT_mA);
#endif

	return LIMIT_mA;
}

static unsigned short _calc_FASTCHG_current_mA_to_offset(unsigned char index,
				unsigned char offset)
{
	return (offset * 50) + 100;
}

static unsigned short sm5705_CHG_get_FASTCHG(struct sm5705_charger_data *charger,
				unsigned char index)
{
	unsigned short FASTCHG_mA;
	unsigned char offset;

	if (index > SM5705_CHG_SRC_WPC) {
		pr_err("invalid charger source index = %d\n", index);
		return 0;
	}

	sm5705_read_reg(charger->i2c, SM5705_REG_CHGCNTL2 + index, &offset);

	FASTCHG_mA = _calc_FASTCHG_current_mA_to_offset(index, offset);

	pr_info("get FASTCHG src=%d, offset=0x%x, current=%dmA\n", index, offset, FASTCHG_mA);

	return FASTCHG_mA;
}

/* monitering REG_MAP */
static unsigned char sm5705_CHG_read_reg(struct sm5705_charger_data *charger,
				unsigned char reg)
{
	unsigned char reg_val = 0x0;

	sm5705_read_reg(charger->i2c, reg, &reg_val);

	return reg_val;
}

static void sm5705_chg_test_read(struct sm5705_charger_data *charger)
{
	char str[1000] = {0,};
	int i;

	for (i = SM5705_REG_INTMSK1; i <= SM5705_REG_FLED1CNTL1; i++) {
		sprintf(str+strlen(str), "0x%02X:0x%02x, ", i, sm5705_CHG_read_reg(charger, i));
	}

	sprintf(str+strlen(str), "0x%02X:0x%02x, ", SM5705_REG_FLEDCNTL6,
		sm5705_CHG_read_reg(charger, SM5705_REG_FLEDCNTL6));
	sprintf(str+strlen(str), "0x%02X:0x%02x, ", SM5705_REG_SBPSCNTL,
		sm5705_CHG_read_reg(charger, SM5705_REG_SBPSCNTL));

	pr_info("sm5705-charger: %s\n", str);
}

/**
 *  SM5705 Charger Driver support functions
 */

static bool sm5705_charger_check_oper_otg_mode_on(void)
{
	unsigned char current_status = sm5705_charger_oper_get_current_status();
	bool ret;

	if (current_status & (1 << SM5705_CHARGER_OP_EVENT_OTG)) {
		ret = true;
	} else {
		ret = false;
	}

	return ret;
}

static bool sm5705_charger_get_charging_on_status(struct sm5705_charger_data *charger)
{
	return sm5705_CHG_get_INT_STATUS(charger, SM5705_INT_STATUS2, SM5705_INT_STATUS2_CHGON);
}

static bool sm5705_charger_get_power_source_status(struct sm5705_charger_data *charger)
{
	int gpio = gpio_get_value(charger->pdata->chg_gpio_en);

	return ((gpio & 0x1)  == 0);    /* charging pin active LOW */
}

static int sm5705_get_input_current(struct sm5705_charger_data *charger)
{
	int get_current;

	if (!is_not_wireless_type(charger->cable_type)) {
		get_current = sm5705_CHG_get_INPUT_LIMIT(charger, SM5705_CHG_SRC_WPC);
	} else {
		get_current = sm5705_CHG_get_INPUT_LIMIT(charger, SM5705_CHG_SRC_VBUS);
	}
#ifdef SM5705_CHG_FULL_DEBUG
	pr_info("src_type=%d, current=%d\n",
		is_not_wireless_type(charger->cable_type), get_current);
#endif

	return get_current;
}

static int sm5705_get_charge_current(struct sm5705_charger_data *charger)
{
	int get_current;

	if (!(is_not_wireless_type(charger->cable_type))) {
		get_current = sm5705_CHG_get_FASTCHG(charger, SM5705_CHG_SRC_WPC);
	} else {
		get_current = sm5705_CHG_get_FASTCHG(charger, SM5705_CHG_SRC_VBUS);
	}
	pr_info("src_type=%d, current=%d\n",
		is_not_wireless_type(charger->cable_type), get_current);

	return get_current;
}

static void sm5705_enable_charging_on_switch(struct sm5705_charger_data *charger,
				bool enable)
{
	if ((enable == 0) & sm5705_CHG_get_INT_STATUS(charger, SM5705_INT_STATUS1, SM5705_INT_STATUS1_VBUSPOK)) {
		sm5705_CHG_set_FREQSEL(charger, SM5705_BUCK_BOOST_FREQ_1_5MHz);
	} else {
		sm5705_CHG_set_FREQSEL(charger, SM5705_BUCK_BOOST_FREQ_3MHz);
	}

	gpio_direction_output(charger->pdata->chg_gpio_en, !(enable));

	if (!enable) {
		/* SET: charging-off condition */
		charger->charging_current = 0;
		charger->topoff_pending = 0;
	}

	pr_info("turn-%s Charging enable pin\n", enable ? "ON" : "OFF");
}

static int sm5705_set_charge_current(struct sm5705_charger_data *charger,
				unsigned short charge_current)
{
	if (!(is_not_wireless_type(charger->cable_type))) {
		sm5705_CHG_set_FASTCHG(charger, SM5705_CHG_SRC_WPC, charge_current);
	} else {
		sm5705_CHG_set_FASTCHG(charger, SM5705_CHG_SRC_VBUS, charge_current);
	}

	return 0;
}

static int sm5705_set_input_current(struct sm5705_charger_data *charger,
				unsigned short input_current)
{
	if (!input_current) {
		pr_info("skip process, input_current = 0\n");
		return 0;
	}

	if (!is_not_wireless_type(charger->cable_type))
		sm5705_CHG_set_INPUT_LIMIT(charger, SM5705_CHG_SRC_WPC, input_current);
	else
		sm5705_CHG_set_INPUT_LIMIT(charger, SM5705_CHG_SRC_VBUS, input_current);

	return 0;
}

static void psy_chg_set_charging_enabled(struct sm5705_charger_data *charger, int charge_mode)
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
	if (buck_state == DISABLE)
		sm5705_charger_oper_push_event(SM5705_CHARGER_OP_EVENT_SUSPEND_MODE, true);
	else
		sm5705_charger_oper_push_event(SM5705_CHARGER_OP_EVENT_SUSPEND_MODE, false);
	sm5705_enable_charging_on_switch(charger, charger->is_charging);
}

static void psy_chg_set_cable_online(struct sm5705_charger_data *charger, int cable_type)
{
	int prev_cable_type = charger->cable_type;
	union power_supply_propval value;

	charger->slow_late_chg_mode = false;
	charger->cable_type = cable_type;

	charger->input_current = sm5708_get_input_current(charger);

	pr_info("[start] prev_cable_type(%d), cable_type(%d), op_mode(%d), op_status(0x%x)",
		prev_cable_type, charger->cable_type,
		sm5705_charger_oper_get_current_op_mode(),
		sm5705_charger_oper_get_current_status());

	if (charger->cable_type == SEC_BATTERY_CABLE_POWER_SHARING) {
		psy_do_property("ps", get, POWER_SUPPLY_PROP_STATUS, value);
		sm5705_charger_oper_push_event(SM5705_CHARGER_OP_EVENT_PWR_SHAR, value.intval);
	} else if (charger->cable_type == SEC_BATTERY_CABLE_OTG) {
#if 0//defined(CONFIG_SM5705_SUPPORT_GAMEPAD)
	if (prev_cable_type == POWER_SUPPLY_TYPE_MDOCK_TA) 
		sm5705_charger_oper_push_event(SM5705_CHARGER_OP_EVENT_VBUS, 0);
#endif
		pr_info("OTG enable, cable(%d)\n", charger->cable_type);
		sm5705_charger_oper_push_event(SM5705_CHARGER_OP_EVENT_OTG, 1);
	} else if (charger->cable_type == SEC_BATTERY_CABLE_NONE) {
		/* set default value */
		sm5705_charger_oper_push_event(SM5705_CHARGER_OP_EVENT_VBUS, 0);
		sm5705_charger_oper_push_event(SM5705_CHARGER_OP_EVENT_PWR_SHAR, 0);

		/* set default input current */
		sm5705_set_input_current(charger, 500);

		cancel_delayed_work(&charger->aicl_work);
	} else {
		sm5705_charger_oper_push_event(SM5705_CHARGER_OP_EVENT_VBUS, 1);

		pr_info("request aicl work for 5V-TA\n");
		queue_delayed_work(charger->wqueue, &charger->aicl_work, msecs_to_jiffies(3000));
	}

	pr_info("[end] is_charging=%d(%d), fc = %d, il = %d, cable = %d,"
		"op_mode(%d), op_status(0x%x)\n",
		charger->is_charging, sm5705_charger_get_power_source_status(charger),
		charger->charging_current,
		charger->input_current,
		charger->cable_type,
		sm5705_charger_oper_get_current_op_mode(),
		sm5705_charger_oper_get_current_status());
}

static void psy_chg_set_usb_hc(struct sm5705_charger_data *charger, int value)
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

#if defined(SM5705_SUPPORT_OTG_CONTROL)
static void psy_chg_set_charge_otg_control(struct sm5705_charger_data *charger, int otg_en)
{
	union power_supply_propval value;

	psy_do_property("wireless", get, POWER_SUPPLY_PROP_ONLINE, value);

	if (otg_en && !value.intval) {
		/* OTG - Enable */
		sm5705_charger_oper_push_event(SM5705_CHARGER_OP_EVENT_OTG, true);
		pr_info("OTG enable, cable(%d)\n", charger->cable_type);
	} else {
		/* OTG - Disable */
		sm5705_charger_oper_push_event(SM5705_CHARGER_OP_EVENT_OTG, false);
		pr_info("OTG disable, cable(%d)\n", charger->cable_type);
	}
}
#endif

#if defined(SM5705_SUPPORT_AICL_CONTROL)
static void psy_chg_set_aicl_control(struct sm5705_charger_data *charger, int aicl_en)
{
	if (aicl_en) {
		sm5705_CHG_enable_AICL(charger, 1);
		pr_info("CHGIN AICL ENABLE\n");
	} else {
		sm5705_CHG_enable_AICL(charger, 0);
		pr_info("CHGIN AICL DISABLE\n");
	}
}
#endif

#if defined(CONFIG_AFC_CHARGER_MODE)
extern void muic_charger_init(void);
static void psy_chg_set_afc_charger_mode(struct sm5705_charger_data *charger, int afc_mode)
{
	pr_info("afc_charger_mode value = %d\n", afc_mode);
#if defined(CONFIG_MUIC_UNIVERSAL_SM5705)
	muic_charger_init();
#endif
}
#endif

static int sm5705_chg_set_property(struct power_supply *psy,
				enum power_supply_property psp, const union power_supply_propval *val)
{
	struct sm5705_charger_data *charger =
		container_of(psy, struct sm5705_charger_data, psy_chg);


	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		dev_info(charger->dev, "CHG:POWER_SUPPLY_PROP_STATUS - status=%d\n", val->intval);
		charger->status = val->intval;
		break;
	case POWER_SUPPLY_PROP_CHARGING_ENABLED:
		dev_info(charger->dev, "CHG:POWER_SUPPLY_PROP_CHARGING_ENABLED (val=%d)\n", val->intval);
		psy_chg_set_charging_enabled(charger, val->intval);
		sm5705_chg_test_read(charger);
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		psy_chg_set_cable_online(charger, val->intval);
		break;
	/* set input current */
	case POWER_SUPPLY_PROP_CURRENT_MAX:
		dev_info(charger->dev, "CHG:POWER_SUPPLY_PROP_CURRENT_MAX (val=%d)\n", val->intval);
		charger->input_current = val->intval;
		sm5705_set_input_current(charger, charger->input_current);
		break;
	/* set charge current */
	case POWER_SUPPLY_PROP_CURRENT_AVG:
		dev_info(charger->dev, "CHG:POWER_SUPPLY_PROP_CURRENT_AVG (val=%d)\n", val->intval);
		charger->charging_current = val->intval;
		sm5705_set_charge_current(charger, charger->charging_current);
		break;
	/* set charge current but not for wireless */
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		dev_info(charger->dev, "CHG:POWER_SUPPLY_PROP_CURRENT_NOW (val=%d)\n", val->intval);
		if (!is_wireless_type(charger->cable_type)) {
			charger->charging_current = val->intval;
			sm5705_set_charge_current(charger, charger->charging_current);
		}
		break;
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
		break;
	case POWER_SUPPLY_PROP_CURRENT_FULL:
		dev_info(charger->dev, "CHG:POWER_SUPPLY_PROP_CURRENT_FULL (val=%d)\n", val->intval);
		sm5705_CHG_set_TOPOFF(charger, val->intval);
		break;
#if defined(CONFIG_AFC_CHARGER_MODE)
	case POWER_SUPPLY_PROP_AFC_CHARGER_MODE:
		psy_chg_set_afc_charger_mode(charger, val->intval);
		break;
#endif
#if defined(CONFIG_BATTERY_SWELLING)
	case POWER_SUPPLY_PROP_VOLTAGE_MAX:
#if defined(CONFIG_SEC_FACTORY)
		// in case of open the batt therm on sub pcb, keep the default float voltage
		dev_info(charger->dev, "keep the default float voltage\n");
		break;
#else
		dev_info(charger->dev, "CHG:POWER_SUPPLY_PROP_VOLTAGE_MAX (val=%d)\n", val->intval);
		charger->pdata->chg_float_voltage = val->intval;
		sm5705_CHG_set_BATREG(charger, val->intval);
		break;
#endif
#endif
	case POWER_SUPPLY_PROP_USB_HC:
		pr_info("POWER_SUPPLY_PROP_USB_HC - value=%d\n", val->intval);
		psy_chg_set_usb_hc(charger, val->intval);
		break;
#if defined(SM5705_SUPPORT_OTG_CONTROL)
	case POWER_SUPPLY_PROP_CHARGE_OTG_CONTROL:
		pr_info("POWER_SUPPLY_PROP_CHARGE_OTG_CONTROL - otg_en=%d\n", val->intval);
		psy_chg_set_charge_otg_control(charger, val->intval);
		break;
#endif
	case POWER_SUPPLY_PROP_CHARGE_NOW:
		return -EINVAL;
#if defined(SM5705_SUPPORT_AICL_CONTROL)
	case POWER_SUPPLY_PROP_CHARGE_AICL_CONTROL:
		pr_info("POWER_SUPPLY_PROP_CHARGE_AICL_CONTROL - aicl_en=%d\n", val->intval);
		psy_chg_set_aicl_control(charger, val->intval);
		break;
#endif
	case POWER_SUPPLY_PROP_CHARGE_COUNTER_SHADOW:
		return -EINVAL;
	case POWER_SUPPLY_PROP_ENERGY_NOW:
		/* WA: abnormal swiching case in JIG cable */
		if (sm5705_call_fg_device_id() <= 2 && !charger->is_rev2_wa_done) {
			if (val->intval) {
				pr_info("queue_delayed_work, op_mode_switch_work\n");
				cancel_delayed_work(&charger->op_mode_switch_work);
				queue_delayed_work(charger->wqueue, &charger->op_mode_switch_work,
						msecs_to_jiffies(8000)); /* delay 8sec */
			} else {
				cancel_delayed_work(&charger->op_mode_switch_work);
			}
		}
		break;
	default:
		pr_err("un-known Power-supply property type (psp=%d)\n", psp);
		return -EINVAL;
	}

	return 0;
}

static int psy_chg_get_online(struct sm5705_charger_data *charger)
{
	int src_type;

	if (sm5705_CHG_get_INT_STATUS(charger, SM5705_INT_STATUS1, SM5705_INT_STATUS1_VBUSPOK)) {
		src_type = SEC_BATTERY_CABLE_TA;
	} else if (sm5705_CHG_get_INT_STATUS(charger, SM5705_INT_STATUS1, SM5705_INT_STATUS1_WPCINPOK)) {
		src_type = SEC_BATTERY_CABLE_WIRELESS;
	} else {
		src_type = SEC_BATTERY_CABLE_NONE;
	}

	return src_type;
}

static bool _decide_charge_full_status(struct sm5705_charger_data *charger)
{
	if ((sm5705_CHG_get_INT_STATUS(charger, SM5705_INT_STATUS2, SM5705_INT_STATUS2_TOPOFF)) ||
		(sm5705_CHG_get_INT_STATUS(charger, SM5705_INT_STATUS2, SM5705_INT_STATUS2_DONE))) {
		return charger->topoff_pending;
	}

	return false;
}

static int psy_chg_get_charger_state(struct sm5705_charger_data *charger)
{
	int status = POWER_SUPPLY_STATUS_UNKNOWN;

	if (_decide_charge_full_status(charger)) {
		status = POWER_SUPPLY_STATUS_FULL;
	} else if(sm5705_charger_get_charging_on_status(charger)){
		status = POWER_SUPPLY_STATUS_CHARGING;
	} else {
		if (sm5705_charger_get_power_source_status(charger)) {
			status = POWER_SUPPLY_STATUS_NOT_CHARGING;
		} else {
			status = POWER_SUPPLY_STATUS_DISCHARGING;
		}
	}

	return status;
}

static int psy_chg_get_charge_type(struct sm5705_charger_data *charger)
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

static int psy_chg_get_charging_health(struct sm5705_charger_data *charger)
{
	int state;
	unsigned char reg_data;

	sm5705_read_reg(charger->i2c, SM5705_REG_STATUS1, &reg_data);

	if (charger->cable_type != SEC_BATTERY_CABLE_WIRELESS) {
		if (reg_data & (1 << SM5705_INT_STATUS1_VBUSPOK)) {
			state = POWER_SUPPLY_HEALTH_GOOD;
		} else if (reg_data & (1 << SM5705_INT_STATUS1_VBUSOVP)) {
			state = POWER_SUPPLY_HEALTH_OVERVOLTAGE;
		} else if (reg_data & (1 << SM5705_INT_STATUS1_VBUSUVLO)) {
			state = POWER_SUPPLY_HEALTH_UNDERVOLTAGE;
		} else {
			state = POWER_SUPPLY_HEALTH_UNKNOWN;
		}
	} else {
		if (reg_data & (1 << SM5705_INT_STATUS1_WPCINPOK)) {
			state = POWER_SUPPLY_HEALTH_GOOD;
		} else if (reg_data & (1 << SM5705_INT_STATUS1_WPCINOVP)) {
			state = POWER_SUPPLY_HEALTH_OVERVOLTAGE;
		} else if (reg_data & (1 << SM5705_INT_STATUS1_WPCINUVLO)) {
			state = POWER_SUPPLY_HEALTH_UNDERVOLTAGE;
		} else {
			state = POWER_SUPPLY_HEALTH_UNKNOWN;
		}
	}

	return (int)state;
}

static int sm5705_chg_create_attrs(struct device *dev)
{
	int i, rc;

	for (i = 0; i < ARRAY_SIZE(sm5705_charger_attrs); i++) {
		rc = device_create_file(dev, &sm5705_charger_attrs[i]);
		if (rc)
			goto create_attrs_failed;
	}
	return rc;

create_attrs_failed:
	pr_err("failed (%d)\n", rc);
	while (i--)
		device_remove_file(dev, &sm5705_charger_attrs[i]);
	return rc;
}

ssize_t sm5705_chg_show_attrs(struct device *dev, struct device_attribute *attr, char *buf)
{
	const ptrdiff_t offset = attr - sm5705_charger_attrs;
	struct power_supply *psy = dev_get_drvdata(dev);
	struct sm5705_charger_data *charger =
		container_of(psy, struct sm5705_charger_data, psy_chg);
	int i = 0;
	unsigned char reg_data;
	u8 addr, data;

	switch (offset){
	case CHIP_ID:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n", "SM5705");
		break;
	case CHARGER_OP_MODE:
	    sm5705_read_reg(charger->i2c, SM5705_REG_CNTL, &reg_data);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",reg_data);
		break;
	case DATA:
		for (addr = SM5705_REG_STATUS1; addr <= SM5705_REG_CHGCNTL8; addr++) {
			sm5705_read_reg(charger->i2c, addr, &data);
			i += scnprintf(buf + i, PAGE_SIZE - i, "0x%02x : 0x%02x\n", addr, data);
		}
		break;
	default:
		return -EINVAL;
	}
	return i;
}

ssize_t sm5705_chg_store_attrs(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	const ptrdiff_t offset = attr - sm5705_charger_attrs;
	int ret = 0;
	int x = 0, y = 0;
	struct power_supply *psy = dev_get_drvdata(dev);
	struct sm5705_charger_data *charger =
		container_of(psy, struct sm5705_charger_data, psy_chg);

	switch (offset) {
	case CHIP_ID:
		ret = count;
		break;
	case CHARGER_OP_MODE:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			if (x == 2) {
				sm5705_update_reg(charger->i2c, SM5705_REG_CNTL, SM5705_CHARGER_OP_MODE_SUSPEND, 0x07);
			} else if (x == 1) {
				sm5705_update_reg(charger->i2c, SM5705_REG_CNTL, SM5705_CHARGER_OP_MODE_CHG_ON, 0x07);
			} else {
				pr_info("change charger op mode fail\n");
				return -EINVAL;
			}
			ret = count;
		}
		break;
	case DATA:
		if (sscanf(buf, "0x%8x 0x%8x", &x, &y) == 2) {
			if (x >= SM5705_REG_STATUS1 && x <= SM5705_REG_CHGCNTL8) {
				u8 addr = x;
				u8 data = y;
				if (sm5705_write_reg(charger->i2c, addr, data) < 0) {
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

static int sm5705_chg_get_property(struct power_supply *psy,
				enum power_supply_property psp, union power_supply_propval *val)
{
	struct sm5705_charger_data *charger =
		container_of(psy, struct sm5705_charger_data, psy_chg);
	enum power_supply_ext_property ext_psp = psp;

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = psy_chg_get_online(charger);
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = sm5705_CHG_get_INT_STATUS(charger,
						SM5705_INT_STATUS2, SM5705_INT_STATUS2_NOBAT);
		break;
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = psy_chg_get_charger_state(charger);
		break;
	case POWER_SUPPLY_PROP_CHARGE_TYPE:
		val->intval = psy_chg_get_charge_type(charger);
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = psy_chg_get_charging_health(charger);
		sm5705_chg_test_read(charger);
		break;
	/* get input current which was set */
	case POWER_SUPPLY_PROP_CURRENT_MAX:
		val->intval = charger->input_current;
		break;
	/* get input current which was read */
	case POWER_SUPPLY_PROP_CURRENT_AVG:
		val->intval = sm5705_get_input_current(charger);
		break;
	/* get charge current which was set */
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		val->intval = charger->charging_current;
		break;
	/* get charge current which was read */
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
		val->intval = sm5705_get_charge_current(charger);
		break;
#if defined(CONFIG_BATTERY_SWELLING)
	/* get float voltage */
	case POWER_SUPPLY_PROP_VOLTAGE_MAX:
		val->intval = sm5705_CHG_get_BATREG(charger);
		break;
#endif
#if defined(CONFIG_AFC_CHARGER_MODE)
	case POWER_SUPPLY_PROP_AFC_CHARGER_MODE:
		return -ENODATA;
#endif
#if defined(SM5705_SUPPORT_OTG_CONTROL)
	case POWER_SUPPLY_PROP_CHARGE_OTG_CONTROL:
		val->intval = sm5705_charger_check_oper_otg_mode_on();
		//val->intval = 0; // disable new otg ui concept.
		break;
#endif
	case POWER_SUPPLY_PROP_USB_HC:
		return -ENODATA;
	/* get cv mode */
	case POWER_SUPPLY_PROP_CHARGE_NOW:
		return -ENODATA;
#if defined(SM5705_SUPPORT_AICL_CONTROL)
	case POWER_SUPPLY_PROP_CHARGE_AICL_CONTROL:
		return -ENODATA;
#endif
	case POWER_SUPPLY_PROP_CHARGE_COUNTER_SHADOW:
		return -ENODATA;
	case POWER_SUPPLY_PROP_ENERGY_NOW:
		return -ENODATA;
	case POWER_SUPPLY_PROP_MAX ... POWER_SUPPLY_EXT_PROP_MAX:
			switch (ext_psp) {
			case POWER_SUPPLY_EXT_PROP_CHIP_ID:
				{
					u8 reg_data;
					val->intval =(sm5705_read_reg(charger->i2c, SM5705_REG_CHGCNTL4, &reg_data) == 0);
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

static int sm5705_otg_get_property(struct power_supply *psy,
				enum power_supply_property psp, union power_supply_propval *val)
{
	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = sm5705_charger_check_oper_otg_mode_on();
		//val->intval = 0; // disable new otg ui concept.
		pr_info("POWER_SUPPLY_PROP_ONLINE - %s\n", (val->intval) ? "ON" : "OFF");
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int sm5705_otg_set_property(struct power_supply *psy,
				enum power_supply_property psp, const union power_supply_propval *val)
{
	struct sm5705_charger_data *charger =
		container_of(psy, struct sm5705_charger_data, psy_otg);
	union power_supply_propval value;

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		pr_info("POWER_SUPPLY_PROP_ONLINE - %s\n", (val->intval) ? "ON" : "OFF");
#if defined(SM5705_SUPPORT_OTG_CONTROL)
		value.intval = val->intval;
		psy_do_property("sm5705-charger", set, POWER_SUPPLY_PROP_CHARGE_OTG_CONTROL, value);
#else
		if (val->intval) {
			value.intval = SEC_BATTERY_CABLE_OTG;
		} else {
			value.intval = SEC_BATTERY_CABLE_NONE;
		}
		psy_do_property("sm5705-charger", set, POWER_SUPPLY_PROP_ONLINE, value);
#endif
		power_supply_changed(&charger->psy_otg);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static enum power_supply_property sm5705_otg_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

/**
 *  SM5705 Charger IRQ & Work-queue service management functions
 */
 static void sm5705_op_mode_switch_work(struct work_struct *work)
{
	struct sm5705_charger_data *charger =
		container_of(work, struct sm5705_charger_data, op_mode_switch_work.work);

	pr_info("schedule work start.\n");

	charger->is_rev2_wa_done = true;
	/* OP Mode switch : CHG_ON(init) -> USB_OTG -> CHG_ON -> FLASH_BOOST -> CHG_ON */
	sm5705_update_reg(charger->i2c, SM5705_REG_CNTL, SM5705_CHARGER_OP_MODE_USB_OTG, 0x07);
	msleep(2000);
	sm5705_update_reg(charger->i2c, SM5705_REG_CNTL, SM5705_CHARGER_OP_MODE_CHG_ON, 0x07);
	msleep(3000);
	sm5705_update_reg(charger->i2c, SM5705_REG_CNTL, SM5705_CHARGER_OP_MODE_FLASH_BOOST, 0x07);
	msleep(2000);
	sm5705_update_reg(charger->i2c, SM5705_REG_CNTL, SM5705_CHARGER_OP_MODE_CHG_ON, 0x07);

	pr_info("schedule work done.\n");
}

static void sm5705_topoff_work(struct work_struct *work)
{
	struct sm5705_charger_data *charger =
		container_of(work, struct sm5705_charger_data, topoff_work.work);
	bool topoff = 1;
	int i;

	pr_info("schedule work start.\n");

	for (i=0; i < 3; ++i) {
		topoff &= sm5705_CHG_get_INT_STATUS(charger, SM5705_INT_STATUS2, SM5705_INT_STATUS2_TOPOFF);
		msleep(150);

		pr_info("%dth Check TOP-OFF state=%d\n", i, topoff);
	}

	charger->topoff_pending = topoff;

	pr_info("schedule work done.\n");
}

static void _reduce_input_limit_current(struct sm5705_charger_data *charger, int cur)
{
	unsigned short vbus_limit_current = sm5705_CHG_get_INPUT_LIMIT(charger, SM5705_CHG_SRC_VBUS);

	if ((vbus_limit_current <= MINIMUM_INPUT_CURRENT) || (vbus_limit_current <= cur)) {
		return;
	}

	vbus_limit_current = ((vbus_limit_current - cur) < MINIMUM_INPUT_CURRENT) ?
		MINIMUM_INPUT_CURRENT : vbus_limit_current - cur;
	sm5705_CHG_set_INPUT_LIMIT(charger, SM5705_CHG_SRC_VBUS, vbus_limit_current);

	charger->input_current = sm5705_get_input_current(charger);

	dev_info(charger->dev, "vbus_limit_current=%d, input_current=%d\n",
		vbus_limit_current, charger->input_current);
}

static void _check_slow_charging(struct sm5705_charger_data *charger)
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

static bool _check_aicl_state(struct sm5705_charger_data *charger)
{
	return sm5705_CHG_get_INT_STATUS(charger, SM5705_INT_STATUS2, SM5705_INT_STATUS2_AICL);
}

static void sm5705_aicl_work(struct work_struct *work)
{
	struct sm5705_charger_data *charger =
		container_of(work, struct sm5705_charger_data, aicl_work.work);
	int prev_current_max, max_count, now_count = 0;

	if (sm5705_call_fg_device_id() <= 2) {
		pr_info("don't support AICL work at REV.2\n");
		return;
	}

	if (!sm5705_charger_get_charging_on_status(charger) ||
		is_hv_wire_type(charger->cable_type)) {
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
		value.intval = sm5705_CHG_get_INPUT_LIMIT(charger, SM5705_CHG_SRC_VBUS);
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

static unsigned char _get_valid_vbus_status(struct sm5705_charger_data *charger)
{
	unsigned char vbusin, prev_vbusin = 0xff;
	int stable_count = 0;

	while (1) {
		sm5705_read_reg(charger->i2c, SM5705_REG_STATUS1, &vbusin);
		vbusin &= 0xF;

		if (prev_vbusin == vbusin) {
			stable_count++;
		} else {
			pr_info("VBUS status mismatch (0x%x / 0x%x), Reset stable count\n",
				vbusin, prev_vbusin);
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

static int _check_vbus_power_supply_status(struct sm5705_charger_data *charger,
				unsigned char vbus_status, int prev_battery_health)
{
	int battery_health = prev_battery_health;

	if (vbus_status & (1 << SM5705_INT_STATUS1_VBUSPOK)) {
		if (prev_battery_health == POWER_SUPPLY_HEALTH_OVERVOLTAGE) {
			pr_info("overvoltage->normal\n");
			battery_health = POWER_SUPPLY_HEALTH_GOOD;
		} else if (prev_battery_health == POWER_SUPPLY_HEALTH_UNDERVOLTAGE){
			pr_info("undervoltage->normal\n");
			battery_health = POWER_SUPPLY_HEALTH_GOOD;
		}
	} else {
		if ((vbus_status & (1 << SM5705_INT_STATUS1_VBUSOVP)) &&
			(prev_battery_health != POWER_SUPPLY_HEALTH_OVERVOLTAGE)) {
			pr_info("charger is over voltage\n");
			battery_health = POWER_SUPPLY_HEALTH_OVERVOLTAGE;
		} else if ((vbus_status & (1 << SM5705_INT_STATUS1_VBUSUVLO)) &&
			(prev_battery_health != POWER_SUPPLY_HEALTH_UNDERVOLTAGE) &&
			is_not_wireless_type(charger->cable_type) &&
			(charger->cable_type != SEC_BATTERY_CABLE_NONE)) {
			pr_info("vBus is undervoltage\n");
			battery_health = POWER_SUPPLY_HEALTH_UNDERVOLTAGE;
		}
	}

	return battery_health;
}

static irqreturn_t sm5705_chg_vbus_in_isr(int irq, void *data)
{
	struct sm5705_charger_data *charger = data;
	union power_supply_propval value;
	unsigned char vbus_status;
	int prev_battery_health;

	pr_info("start.\n");

	vbus_status = _get_valid_vbus_status(charger);

	psy_do_property("battery", get,POWER_SUPPLY_PROP_HEALTH, value);
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

static irqreturn_t sm5705_chg_done_isr(int irq, void *data)
{
	struct sm5705_charger_data *charger = data;

	pr_info("%s: start.\n", __func__);

	/* nCHG pin toggle */
	gpio_direction_output(charger->pdata->chg_gpio_en, charger->is_charging);
	msleep(10);
	//gpio_direction_output(charger->pdata->chg_gpio_en, !(charger->is_charging));

	return IRQ_HANDLED;
}

static irqreturn_t sm5705_chg_otg_fail_isr(int irq, void *data)
{
	struct sm5705_charger_data *charger = data;
	union power_supply_propval value = {0, };
	u8 otg_state = 0;
#ifdef CONFIG_USB_HOST_NOTIFY
	struct otg_notify *o_notify;

	o_notify = get_otg_notify();
#endif

	pr_info("%s: start.\n", __func__);

	otg_state = sm5705_CHG_get_INT_STATUS(charger, SM5705_INT_STATUS3, SM5705_INT_STATUS3_OTGFAIL);
	if(otg_state) {
		dev_info(charger->dev, "%s OTG fail !! \n", __func__);
#ifdef CONFIG_USB_HOST_NOTIFY
		send_otg_notify(o_notify, NOTIFY_EVENT_OVERCURRENT, 0);
#endif
		/* disable the register values just related to OTG and
				keep the values about the charging */
		value.intval = 0;
		psy_do_property("otg", set,
				POWER_SUPPLY_PROP_ONLINE, value);
	}
	return IRQ_HANDLED;
}

/**
 *  SM5705 Charger driver management functions
 **/

#ifdef CONFIG_OF
static int _parse_sm5705_charger_node_propertys(struct device *dev,
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

	ret = of_property_read_string(np,"battery,wirelss_charger_name",
				(char const **)&pdata->wireless_charger_name);
	if (IS_ERR_VALUE(ret))
		dev_err(dev, "%s: can't parsing dt:battery,wirelss_charger_name\n", __func__);
	else
		dev_info(dev, "%s: wireless charger name=%s\n", __func__, pdata->wireless_charger_name);

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


static int sm5705_charger_parse_dt(struct sm5705_charger_data *charger,
				struct sec_charger_platform_data *pdata)
{
	struct device_node *np;
	int ret;

	np = of_find_node_by_name(NULL, "sm5705-charger");
	if (np == NULL) {
		pr_err("fail to find dt_node:sm5705-charger\n");
		return -ENOENT;
	} else {
		ret = _parse_sm5705_charger_node_propertys(charger->dev, np, pdata);
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

static sec_charger_platform_data_t *_get_sm5705_charger_platform_data
				(struct platform_device *pdev, struct sm5705_charger_data *charger)
{
#ifdef CONFIG_OF
	sec_charger_platform_data_t *pdata;
	int ret;

	pdata = kzalloc(sizeof(sec_charger_platform_data_t), GFP_KERNEL);
	if (!pdata) {
		pr_err("fail to memory allocate for sec_charger_platform_data\n");
		return NULL;
	}

	ret = sm5705_charger_parse_dt(charger, pdata);
	if (IS_ERR_VALUE(ret)) {
		pr_err("fail to parse sm5705 charger device tree (ret=%d)\n", ret);
		kfree(pdata);
		return NULL;
	}
#else
	struct sm5705_platform_data *sm5705_pdata = dev_get_platdata(sm5705->dev);
	sec_charger_platform_data_t *pdata;

	pdata = sm5705_pdata->charger_data;
	if (!pdata) {
		pr_err("fail to get sm5705 charger platform data\n");
		return NULL;
	}
#endif

	pr_info("Get valid platform data done. (pdata=%p)\n", pdata);
	return pdata;
}

static int _init_sm5705_charger_info(struct platform_device *pdev,
				struct sm5705_dev *sm5705, struct sm5705_charger_data *charger)
{
	struct sm5705_platform_data *pdata = dev_get_platdata(sm5705->dev);
	int ret;
	mutex_init(&charger->charger_mutex);

	if (pdata == NULL) {
		pr_err("can't get sm5705_platform_data\n");
		return -EINVAL;
	}

	pr_info("init process start..\n");

	/* setup default charger configuration parameter & flagment */
	charger->input_current = 500;
	charger->topoff_pending = false;
	charger->is_charging = false;
	charger->cable_type = SEC_BATTERY_CABLE_NONE;
	charger->is_rev2_wa_done = false;
	charger->slow_late_chg_mode = false;

	/* Request GPIO pin - CHG_IN */
	if (charger->pdata->chg_gpio_en) {
		ret = gpio_request(charger->pdata->chg_gpio_en, "sm5705_nCHGEN");
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

	//INIT_DELAYED_WORK(&charger->wpc_work, wpc_detect_work);
	INIT_DELAYED_WORK(&charger->aicl_work, sm5705_aicl_work);
	INIT_DELAYED_WORK(&charger->topoff_work, sm5705_topoff_work);
	INIT_DELAYED_WORK(&charger->op_mode_switch_work, sm5705_op_mode_switch_work);

#if defined(SM5705_SW_SOFT_START)
	wake_lock_init(&charger->softstart_wake_lock, WAKE_LOCK_SUSPEND, "charger-softstart");
#endif
	wake_lock_init(&charger->wpc_wake_lock, WAKE_LOCK_SUSPEND, "charger-wpc");
	wake_lock_init(&charger->afc_wake_lock, WAKE_LOCK_SUSPEND, "charger-afc");
	wake_lock_init(&charger->check_slow_wake_lock, WAKE_LOCK_SUSPEND, "charger-check-slow");
	wake_lock_init(&charger->aicl_wake_lock, WAKE_LOCK_SUSPEND, "charger-aicl");

	/* Get IRQ service routine number */
	charger->irq_wpcin_pok = pdata->irq_base + SM5705_WPCINPOK_IRQ;
	charger->irq_vbus_pok = pdata->irq_base + SM5705_VBUSPOK_IRQ;
	charger->irq_aicl = pdata->irq_base + SM5705_AICL_IRQ;
	charger->irq_topoff = pdata->irq_base + SM5705_TOPOFF_IRQ;
	charger->irq_done = pdata->irq_base + SM5705_DONE_IRQ;
	charger->irq_otgfail = pdata->irq_base + SM5705_OTGFAIL_IRQ;

	pr_info("init process done..\n");

	return 0;
}

static void sm5705_charger_initialize(struct sm5705_charger_data *charger)
{
	pr_info("charger initial hardware condition process start. (float_voltage=%d)\n",
		charger->pdata->chg_float_voltage);

	/* Auto-Stop configuration for Emergency status */
	sm5705_CHG_set_TOPOFF(charger, 300);
	sm5705_CHG_set_TOPOFF_TMR(charger, SM5705_TOPOFF_TIMER_45m);
	sm5705_CHG_enable_AUTOSTOP(charger, 1);
	sm5705_CHG_set_BATREG(charger, charger->pdata->chg_float_voltage);
	sm5705_CHG_set_AICLTH(charger, 4500);
	sm5705_CHG_enable_AICL(charger, 1);
	sm5705_CHG_enable_AUTOSET(charger, 0);
	sm5705_CHG_set_BST_IQ3LIMIT(charger, SM5705_CHG_BST_IQ3LIMIT_3_5A);
	sm5705_CHG_set_OVPSEL(charger, 1); /* fix OVPSEL */

	/* SM5705 Charger Reset contdition initialize */
#if defined(SM5705_I2C_RESET_ACTIVATE)
	sm5705_CHG_set_ENI2CRESET(charger, 1);
#endif

#if defined(SM5705_MANUAL_RESET_ACTIVATE)
	sm5705_CHG_set_ENMRSTB(charger, SM5705_MANUAL_RESET_TIMER);
#endif

	pr_info("charger initial hardware condition process done.\n");
}

static int sm5705_charger_probe(struct platform_device *pdev)
{
	struct sm5705_dev *sm5705 = dev_get_drvdata(pdev->dev.parent);
	struct sm5705_charger_data *charger;
	int ret = 0;

	pr_info("Sm5705 Charger Driver Probing start\n");

	charger = kzalloc(sizeof(struct sm5705_charger_data), GFP_KERNEL);
	if (!charger) {
		pr_err("fail to memory allocate for sm5705 charger handler\n");
		return -ENOMEM;
	}

	charger->dev = &pdev->dev;
	charger->i2c = sm5705->i2c;
	charger->pdata = _get_sm5705_charger_platform_data(pdev, charger);
	if (charger->pdata == NULL) {
		pr_err("fail to get charger platform data\n");
		return -ENOENT;
	}

	ret = _init_sm5705_charger_info(pdev, sm5705, charger);
	if (IS_ERR_VALUE(ret)) {
		pr_err("can't initailize sm5705 charger");
		goto err_free;
	}
	platform_set_drvdata(pdev, charger);

	sm5705_charger_initialize(charger);

	charger->psy_chg.name = "sm5705-charger";
	charger->psy_chg.type = POWER_SUPPLY_TYPE_UNKNOWN;
	charger->psy_chg.get_property = sm5705_chg_get_property;
	charger->psy_chg.set_property = sm5705_chg_set_property;
	charger->psy_chg.properties	= sm5705_charger_props;
	charger->psy_chg.num_properties	= ARRAY_SIZE(sm5705_charger_props);
	ret = power_supply_register(&pdev->dev, &charger->psy_chg);
	if (ret) {
		pr_err("fail to register psy_chg\n");
		goto err_power_supply_register;
	}

	charger->psy_otg.name = "otg";
	charger->psy_otg.type = POWER_SUPPLY_TYPE_OTG;
	charger->psy_otg.get_property = sm5705_otg_get_property;
	charger->psy_otg.set_property = sm5705_otg_set_property;
	charger->psy_otg.properties	 = sm5705_otg_props;
	charger->psy_otg.num_properties	= ARRAY_SIZE(sm5705_otg_props);
	ret = power_supply_register(&pdev->dev, &charger->psy_otg);
	if (ret) {
		pr_err("fail to register otg_chg\n");
		goto err_power_supply_register_chg;
	}

	/* Operation Mode Initialize */
	sm5705_charger_oper_table_init(charger->i2c);

	ret = sm5705_chg_create_attrs(charger->psy_chg.dev);
	if (ret) {
		pr_err("Failed to create_attrs\n");
		goto err_power_supply_register_otg;
	}

	/* Request IRQ */
	ret = request_threaded_irq(charger->irq_vbus_pok, NULL,
			sm5705_chg_vbus_in_isr, 0, "chgin-irq", charger);
	if (ret < 0) {
		pr_err("fail to request chgin IRQ: %d: %d\n", charger->irq_vbus_pok, ret);
		goto err_power_supply_register_otg;
	}

	ret = request_threaded_irq(charger->irq_done, NULL,
			sm5705_chg_done_isr, 0, "done-irq", charger);
	if (ret < 0) {
		pr_err("fail to request chgin IRQ: %d: %d\n", charger->irq_done, ret);
		goto err_power_supply_register_otg;
	}

	ret = request_threaded_irq(charger->irq_otgfail, NULL,
			sm5705_chg_otg_fail_isr, 0, "otgfail-irq", charger);
	if (ret < 0) {
		pr_err("fail to request otgfail IRQ: %d: %d\n", charger->irq_otgfail, ret);
		goto err_power_supply_register_otg;
	}

	pr_info("SM5705 Charger Driver Loaded Done\n");

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

static int sm5705_charger_remove(struct platform_device *pdev)
{
	struct sm5705_charger_data *charger = platform_get_drvdata(pdev);

	//cancel_delayed_work(&charger->wpc_work);
	cancel_delayed_work(&charger->aicl_work);
	cancel_delayed_work(&charger->topoff_work);
	cancel_delayed_work(&charger->op_mode_switch_work);
	destroy_workqueue(charger->wqueue);
//	free_irq(charger->irq_wpcin_pok, NULL);
	free_irq(charger->irq_vbus_pok, NULL);
//	free_irq(charger->irq_topoff, NULL);
	free_irq(charger->irq_done, NULL);
	free_irq(charger->irq_otgfail, NULL);
	power_supply_unregister(&charger->psy_chg);
	mutex_destroy(&charger->charger_mutex);
	kfree(charger);
	return 0;
}

static void sm5705_charger_shutdown(struct device *dev)
{
	struct sm5705_charger_data *charger = dev_get_drvdata(dev);

	sm5705_update_reg(charger->i2c, SM5705_REG_CNTL,
			SM5705_CHARGER_OP_MODE_CHG_ON, 0x07);
	pr_info("call shutdown\n");
}

#if defined CONFIG_PM
static int sm5705_charger_suspend(struct device *dev)
{
	pr_info("call suspend\n");
	return 0;
}

static int sm5705_charger_resume(struct device *dev)
{
	pr_info("call resume\n");
	return 0;
}
#else
#define sm5705_charger_suspend NULL
#define sm5705_charger_resume NULL
#endif

static SIMPLE_DEV_PM_OPS(sm5705_charger_pm_ops, sm5705_charger_suspend, sm5705_charger_resume);
static struct platform_driver sm5705_charger_driver = {
	.driver = {
		.name = "sm5705-charger",
		.owner = THIS_MODULE,
		.pm = &sm5705_charger_pm_ops,
		.shutdown = sm5705_charger_shutdown,
	},
	.probe = sm5705_charger_probe,
	.remove = sm5705_charger_remove,
};

static int __init sm5705_charger_init(void)
{
	return platform_driver_register(&sm5705_charger_driver);
}

static void __exit sm5705_charger_exit(void)
{
	platform_driver_unregister(&sm5705_charger_driver);
}


module_init(sm5705_charger_init);
module_exit(sm5705_charger_exit);

MODULE_DESCRIPTION("SM5705 Charger Driver");
MODULE_LICENSE("GPL v2");

