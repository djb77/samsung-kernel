/*
 * s2mu004_charger.c - S2MU004 Charger Driver
 *
 * Copyright (C) 2016 Samsung Electronics Co.Ltd
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <linux/mfd/samsung/s2mu004.h>
#include "include/charger/s2mu004_charger.h"
#include <linux/version.h>
#include <linux/of_gpio.h>
#ifdef CONFIG_USB_HOST_NOTIFY
#include <linux/usb_notify.h>
#endif

#define ENABLE_MIVR 0
#define EN_OVP_IRQ 1
#define EN_IEOC_IRQ 1
#define EN_TOPOFF_IRQ 1
#define EN_RECHG_REQ_IRQ 0
#define EN_TR_IRQ 0
#define EN_MIVR_SW_REGULATION 0
#define EN_BST_IRQ 0
#define EN_BAT_DET_IRQ 0
#define EN_OTG_FAULT_IRQ 1
#define EN_IVR_IRQ 1
#define MINVAL(a, b) ((a <= b) ? a : b)
#define EOC_DEBOUNCE_CNT 2
#define HEALTH_DEBOUNCE_CNT 1
#define DEFAULT_CHARGING_CURRENT 500
#define EOC_SLEEP 200
#define EOC_TIMEOUT (EOC_SLEEP * 6)
#ifndef EN_TEST_READ
#define EN_TEST_READ 0
#endif
#define ENABLE 1
#define DISABLE 0
extern unsigned int lpcharge;
extern int factory_mode;
int JIG_USB_ON;

static struct device_attribute s2mu004_charger_attrs[] = {
	S2MU004_CHARGER_ATTR(chip_id),
	S2MU004_CHARGER_ATTR(data),
};
static enum power_supply_property s2mu004_charger_props[] = {
};
static enum power_supply_property s2mu004_otg_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

static int s2mu004_get_charging_health(struct s2mu004_charger_data *charger);
static int s2mu004_get_input_current_limit(struct s2mu004_charger_data *charger);
static void s2mu004_set_input_current_limit(
	struct s2mu004_charger_data *charger, int charging_current);

#if EN_IVR_IRQ
static void s2mu004_enable_ivr_irq(struct s2mu004_charger_data *charger);
#endif

#if defined(CONFIG_SEC_FACTORY)
static int s2mu004_get_JIG_USB_ON(char *val)
{
	JIG_USB_ON = strncmp(val, "1", 1) ? 0 : 1;
	pr_info("%s, JIG_USB_ON:%d\n", __func__, JIG_USB_ON);
	return 1;
}
__setup("JIG_USB_ON=", s2mu004_get_JIG_USB_ON);
#endif
static void s2mu004_test_read(struct i2c_client *i2c)
{
	u8 data;
	char str[1016] = {0,};
	int i;
	
	for (i = 0x02; i <= 0x24; i++) {
		if (i > 0x03 && i < 0x0A)
			continue;
		
		s2mu004_read_reg(i2c, i, &data);
		sprintf(str+strlen(str), "0x%02x:0x%02x, ", i, data);
	}
	s2mu004_read_reg(i2c, 0x33, &data);
	sprintf(str+strlen(str), "0x%02x:0x%02x, ", 0x33, data);
	s2mu004_read_reg(i2c, 0x91, &data);
	sprintf(str+strlen(str), "0x%02x:0x%02x\n", 0x91, data);
	/* check bypass mode */
	s2mu004_read_reg(i2c, 0x29, &data);
	sprintf(str+strlen(str), "0x%02x:0x%02x, ", 0x29, data);
	s2mu004_read_reg(i2c, 0x2F, &data);
	sprintf(str+strlen(str), "0x%02x:0x%02x, ", 0x2F, data);
	s2mu004_read_reg(i2c, 0x71, &data);
	sprintf(str+strlen(str), "0x%02x:0x%02x, ", 0x71, data);
	s2mu004_read_reg(i2c, 0x72, &data);
	sprintf(str+strlen(str), "0x%02x:0x%02x, ", 0x72, data);
	s2mu004_read_reg(i2c, 0x8B, &data);
	sprintf(str+strlen(str), "0x%02x:0x%02x, ", 0x8B, data);
	s2mu004_read_reg(i2c, 0x93, &data);
	sprintf(str+strlen(str), "0x%02x:0x%02x, ", 0x93, data);
	s2mu004_read_reg(i2c, 0x94, &data);
	sprintf(str+strlen(str), "0x%02x:0x%02x, ", 0x94, data);
	s2mu004_read_reg(i2c, 0x9F, &data);
	sprintf(str+strlen(str), "0x%02x:0x%02x, ", 0x9F, data);
	s2mu004_read_reg(i2c, 0xC6, &data);
	sprintf(str+strlen(str), "0x%02x:0x%02x\n", 0xC6, data);
	pr_err("%s: %s", __func__, str);
}
static int s2mu004_charger_otg_control(struct s2mu004_charger_data *charger, bool enable)
{
	u8 chg_sts2, chg_ctrl0, temp;
	union power_supply_propval value;
	
	pr_info("%s: called charger otg control : %s\n", __func__, enable ? "ON" : "OFF");
	if (charger->otg_on == enable || lpcharge)
		return 0;
	charger->otg_on = enable;
	
	mutex_lock(&charger->charger_mutex);
	if (!enable) {
		pr_info("%s: chg_mode: %d, otg_on: %d\n", __func__, charger->charge_mode, charger->otg_on);
		if (charger->charge_mode == SEC_BAT_CHG_MODE_BUCK_OFF) {
			s2mu004_update_reg(charger->i2c, S2MU004_CHG_CTRL0, CHARGER_OFF_MODE, REG_MODE_MASK);
		} else if (charger->charge_mode == SEC_BAT_CHG_MODE_CHARGING_OFF) {
			s2mu004_update_reg(charger->i2c, S2MU004_CHG_CTRL0, BUCK_MODE, REG_MODE_MASK);
		} else {
			s2mu004_update_reg(charger->i2c, S2MU004_CHG_CTRL0, CHG_MODE, REG_MODE_MASK);
		}
		s2mu004_update_reg(charger->i2c, 0xAE, 0x80, 0xF0);

#if defined(CONFIG_NEW_FACTORY_MODE)
		/* ULDO ON a moment before OFF. (Issue was occured in NEW FACTORY MODE model)
		   ULDO EN condition : high than CHGIN UVLO && NO OTG MODE */
		msleep(1000);
		/* USB LDO on */
		s2mu004_update_reg(charger->i2c, S2MU004_PWRSEL_CTRL0,
				1 << PWRSEL_CTRL0_SHIFT, PWRSEL_CTRL0_MASK);
#endif
	} else {
		s2mu004_update_reg(charger->i2c, S2MU004_CHG_CTRL4,
					S2MU004_SET_OTG_OCP_1500mA << SET_OTG_OCP_SHIFT, SET_OTG_OCP_MASK);
		msleep(10);
		s2mu004_update_reg(charger->i2c, 0xAE, 0x00, 0xF0);
		s2mu004_update_reg(charger->i2c, S2MU004_CHG_CTRL0, OTG_BST_MODE, REG_MODE_MASK);

#if defined(CONFIG_NEW_FACTORY_MODE)
		/* USB LDO off */
		s2mu004_update_reg(charger->i2c, S2MU004_PWRSEL_CTRL0,
				0 << PWRSEL_CTRL0_SHIFT, PWRSEL_CTRL0_MASK);
#endif
	}
	mutex_unlock(&charger->charger_mutex);
	s2mu004_read_reg(charger->i2c, S2MU004_CHG_STATUS2, &chg_sts2);
	s2mu004_read_reg(charger->i2c, S2MU004_CHG_CTRL0, &chg_ctrl0);
	s2mu004_read_reg(charger->i2c, 0xAE, &temp);
	pr_info("%s S2MU004_CHG_STATUS2: 0x%x\n", __func__, chg_sts2);
	pr_info("%s S2MU004_CHG_CTRL0: 0x%x\n", __func__, chg_ctrl0);
	pr_info("%s 0xAE: 0x%x\n", __func__, temp);
	power_supply_changed(&charger->psy_otg);
	psy_do_property("battery", set, POWER_SUPPLY_PROP_CHARGE_TYPE, value);
	
	return enable;
}
#if EN_IVR_IRQ
static void reduce_input_current(struct s2mu004_charger_data *charger)
{
	int old_input_current, new_input_current;
	u8 data = 0;

	old_input_current = s2mu004_get_input_current_limit(charger);
	new_input_current = (old_input_current - REDUCE_CURRENT_STEP) > MINIMUM_INPUT_CURRENT ?
		(old_input_current - REDUCE_CURRENT_STEP) : MINIMUM_INPUT_CURRENT;


	pr_info("%s: input currents:(%d -> %d)\n", __func__, old_input_current, new_input_current);

	if (old_input_current == new_input_current) {
		pr_info("%s: Same old and new input currents:(%d -> %d)\n", __func__,
			old_input_current, new_input_current);
		return;
	}

	if (is_wcin_type(charger->cable_type)) {
		new_input_current = new_input_current * 10;
		data = (new_input_current - 250) / 125;
		s2mu004_update_reg(charger->i2c, S2MU004_CHG_CTRL3,
			data << INPUT_CURRENT_LIMIT_SHIFT, INPUT_CURRENT_LIMIT_MASK);
	} else {
		data = (new_input_current - 50) / 25;
		s2mu004_update_reg(charger->i2c, S2MU004_CHG_CTRL2,
				data << INPUT_CURRENT_LIMIT_SHIFT, INPUT_CURRENT_LIMIT_MASK);
	}
	
	charger->input_current = s2mu004_get_input_current_limit(charger);
	charger->ivr_on = true;
}
#endif
#if !defined(CONFIG_SEC_FACTORY)
static void s2mu004_analog_ivr_switch(struct s2mu004_charger_data *charger, int enable)
{
	u8 reg_data = 0;
	int cable_type = SEC_BATTERY_CABLE_NONE;
#if defined(CONFIG_BATTERY_SWELLING)
	int swelling_mode = 0;
#endif
	union power_supply_propval value;

	if (charger->dev_id >= 0x3) {
		/* control IVRl only under PMIC REV < 0x3 */
		return;
	}
	if (factory_mode || JIG_USB_ON) {
		pr_info("%s: Factory Mode Skip Analog IVR Control\n", __func__);
		return;
	}
#if defined(CONFIG_BATTERY_SWELLING)
	psy_do_property("battery", get,	POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT, value);
	swelling_mode = value.intval;
#endif
	psy_do_property("battery", get,	POWER_SUPPLY_PROP_ONLINE, value);
	cable_type = value.intval;
	if (charger->charge_mode == SEC_BAT_CHG_MODE_CHARGING_OFF ||
		charger->charge_mode == SEC_BAT_CHG_MODE_BUCK_OFF ||
#if defined(CONFIG_BATTERY_SWELLING)
		swelling_mode ||
#endif
		(is_hv_wire_type(cable_type)) ||
		(cable_type == SEC_BATTERY_CABLE_PDIC) ||
		(cable_type == SEC_BATTERY_CABLE_PREPARE_TA)) {
			pr_info("[DEBUG]%s(%d): digital IVR \n", __func__, __LINE__);
			enable = 0;
	}
	s2mu004_read_reg(charger->i2c, 0xB3, &reg_data);
	pr_info("%s : 0xB3 : 0x%x\n", __func__, reg_data);
	if (enable) {
		if (!(reg_data & 0x08)) {
			/* Enable Analog IVR */
			pr_info("[DEBUG]%s: Enable Analog IVR\n", __func__);
			s2mu004_update_reg(charger->i2c, 0xB3, 0x1 << 3, 0x1 << 3);
		}
	} else {
		if (reg_data & 0x08) {
			/* Disable Analog IVR - Digital IVR enable*/
			pr_info("[DEBUG]%s: Disable Analog IVR - Digital IVR enable\n", __func__);
			s2mu004_update_reg(charger->i2c, 0xB3, 0x0, 0x1 << 3);
		}
	}
}
#endif
static void s2mu004_enable_charger_switch(struct s2mu004_charger_data *charger, int onoff)
{
	if (factory_mode || JIG_USB_ON) {
		pr_info("%s: Factory Mode Skip CHG_EN Control\n", __func__);
		return;
	}
	if (charger->otg_on) {
		pr_info("[DEBUG] %s: skipped set(%d) : OTG is on\n", __func__, onoff);
		return;
	}
	if (onoff > 0) {
		pr_info("[DEBUG]%s: turn on charger\n", __func__);
#if !defined(CONFIG_SEC_FACTORY)
		if (charger->dev_id < 0x3) {
			int cable_type = SEC_BATTERY_CABLE_NONE;
#if defined(CONFIG_BATTERY_SWELLING)
			int swelling_mode = 0;
#endif
			union power_supply_propval value;
#if defined(CONFIG_BATTERY_SWELLING)
			psy_do_property("battery", get,
				POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT, value);
			swelling_mode = value.intval;
#endif
			psy_do_property("battery", get,
				POWER_SUPPLY_PROP_ONLINE, value);
			cable_type = value.intval;
			if ((is_hv_wire_type(cable_type)) ||
				(cable_type == SEC_BATTERY_CABLE_PREPARE_TA) ||
#if defined(CONFIG_BATTERY_SWELLING)
				swelling_mode ||
#endif
				(cable_type == SEC_BATTERY_CABLE_PDIC) ||
				(cable_type == SEC_BATTERY_CABLE_UARTOFF)) {
				/* Digital IVR */
				s2mu004_analog_ivr_switch(charger, DISABLE);
			}
		}
#endif
		/* forced ASYNC */
		s2mu004_update_reg(charger->i2c, 0x30, 0x03, 0x03);
		mdelay(30);
		s2mu004_update_reg(charger->i2c, S2MU004_CHG_CTRL0, CHG_MODE, REG_MODE_MASK);
		mdelay(100);
		/* Auto SYNC to ASYNC - default */
		s2mu004_update_reg(charger->i2c, 0x30, 0x01, 0x03);
		/* async off */
		s2mu004_update_reg(charger->i2c, 0x96, 0x00, 0x01 << 3);
	} else {
		pr_info("[DEBUG] %s: turn off charger\n", __func__);
#if !defined(CONFIG_SEC_FACTORY)
		if (charger->dev_id < 0x3) {
			/* Disable Analog IVR - Digital IVR enable*/
			s2mu004_analog_ivr_switch(charger, DISABLE);
		}
#endif
		mdelay(30);
		s2mu004_update_reg(charger->i2c, S2MU004_CHG_CTRL0, BUCK_MODE, REG_MODE_MASK);
		/* async on */
		s2mu004_update_reg(charger->i2c, 0x96, 0x01 << 3, 0x01 << 3);
		mdelay(100);
	}
}
static void s2mu004_set_buck(struct s2mu004_charger_data *charger, int enable)
{
	if (factory_mode || JIG_USB_ON) {
		pr_info("%s: Factory Mode Skip BUCK Control\n", __func__);
		return;
	}
	if (charger->otg_on) {
		pr_info("[DEBUG] %s: skipped set(%d) : OTG is on\n", __func__, charger->otg_on);
		return;
	}
	if (enable) {
		pr_info("[DEBUG]%s: set buck on\n", __func__);
		s2mu004_enable_charger_switch(charger, charger->is_charging);
	} else {
		pr_info("[DEBUG]%s: set buck off (charger off mode)\n", __func__);
#if !defined(CONFIG_SEC_FACTORY)
		if (charger->dev_id < 0x3) {
			/* Disable Analog IVR - Digital IVR enable*/
			s2mu004_analog_ivr_switch(charger, DISABLE);
		}
#endif
		s2mu004_update_reg(charger->i2c, S2MU004_CHG_CTRL0, CHARGER_OFF_MODE, REG_MODE_MASK);
		/* async on */
		s2mu004_update_reg(charger->i2c, 0x96, 0x01 << 3, 0x01 << 3);
		mdelay(100);
	}
}
static void s2mu004_set_regulation_vsys(struct s2mu004_charger_data *charger, int vsys)
{
	u8 data;
	
	pr_info("[DEBUG]%s: VSYS regulation %d \n", __func__, vsys);
	if (vsys <= 3800)
		data = 0;
	else if (vsys > 3800 && vsys <= 4400)
		data = (vsys - 3800) / 100;
	else
		data = 0x06;
	s2mu004_update_reg(charger->i2c, S2MU004_CHG_CTRL7, data << SET_VSYS_SHIFT, SET_VSYS_MASK);
}
static void s2mu004_set_regulation_voltage(struct s2mu004_charger_data *charger, int float_voltage)
{
	u8 data;
	
	if (factory_mode || JIG_USB_ON) {
		pr_info("%s: Factory Mode Skip folat_voltage Control\n", __func__);
		return;
	}
	pr_info("[DEBUG]%s: float_voltage %d \n", __func__, float_voltage);
	if (float_voltage <= 3900)
		data = 0;
	else if (float_voltage > 3900 && float_voltage <= 4530)
		data = (float_voltage - 3900) / 10;
	else
		data = 0x3f;
	s2mu004_update_reg(charger->i2c, S2MU004_CHG_CTRL6, data << SET_VF_VBAT_SHIFT, SET_VF_VBAT_MASK);
}
static int s2mu004_get_regulation_voltage(struct s2mu004_charger_data *charger)
{
	u8 reg_data = 0;
	int float_voltage;
	
	s2mu004_read_reg(charger->i2c, S2MU004_CHG_CTRL6, &reg_data);
	reg_data &= 0x3F;
	float_voltage = reg_data * 10 + 3900;
	pr_debug("%s: battery cv reg : 0x%x, float voltage val : %d\n",	__func__, reg_data, float_voltage);
	return float_voltage;
}
static void s2mu004_set_input_current_limit(struct s2mu004_charger_data *charger, int charging_current)
{
	u8 data;
	
	if (factory_mode || JIG_USB_ON) {
		pr_info("%s: Factory Mode Skip input_current Control\n", __func__);
		return;
	}
	mutex_lock(&charger->charger_mutex);
	if (is_wcin_type(charger->cable_type)) {
		pr_info("[DEBUG]%s: Wireless current limit %d \n", __func__, charging_current);
		if (charging_current <= 50)
			data = 0x02;
		else if (charging_current > 50 && charging_current <= 1250) {
			/* Need to re-write dts file if we need to use 5 digit current (eg. 1.0125A) */
			charging_current = charging_current * 10;
			data = (charging_current - 250) / 125;
			charging_current /= 10;
		} else {
			pr_err("%s: WC current limit is changed (%d -> 1250), set setting to maximum\n",
				__func__, charging_current);
			data = 0x62;
			charging_current = 1250;
		}
		s2mu004_update_reg(charger->i2c, S2MU004_CHG_CTRL3,
			data << INPUT_CURRENT_LIMIT_SHIFT, INPUT_CURRENT_LIMIT_MASK);
	} else {
		pr_info("[DEBUG]%s: Wired current limit %d \n", __func__, charging_current);
		if (charging_current <= 100)
			data = 0x02;
		else if (charging_current > 100 && charging_current <= 2500)
			data = (charging_current - 50) / 25;
		else {
			pr_err("%s: CHGIN current limit is changed (%d -> 2500), set setting to maximum\n",
				__func__, charging_current);
			data = 0x62;
			charging_current = 2500;
		}
		s2mu004_update_reg(charger->i2c, S2MU004_CHG_CTRL2,
				data << INPUT_CURRENT_LIMIT_SHIFT, INPUT_CURRENT_LIMIT_MASK);
	}
	mutex_unlock(&charger->charger_mutex);
	pr_info("[DEBUG]%s: current  %d, 0x%x\n", __func__, charging_current, data);
#if EN_TEST_READ
	s2mu004_test_read(charger->i2c);
#endif
}
static int s2mu004_get_input_current_limit(struct s2mu004_charger_data *charger)
{
	u8 data;
	int w_current;
	
	if (is_wcin_type(charger->cable_type)) {
		s2mu004_read_reg(charger->i2c, S2MU004_CHG_CTRL3, &data);
		data = data & INPUT_CURRENT_LIMIT_MASK;
		if (data > 0x62) {
			pr_err("%s: Invalid WC current limit in register: 0x%x\n", __func__, data);
			data = 0x62;
		}
		/* note: if use value with 5 digits the fractional 0.5 will be truncated */
		w_current = ((int)data * 125 + 250) / 10;

		pr_err("[DEBUG]%s: Wireless current limit out: %d\n", __func__, w_current);

		return w_current;
	} else {
		s2mu004_read_reg(charger->i2c, S2MU004_CHG_CTRL2, &data);
		data = data & INPUT_CURRENT_LIMIT_MASK;
		if (data > 0x62) {
			pr_err("%s: Invalid current limit in register: 0x%x\n",	__func__, data);
			data = 0x62;
		}
		return  data * 25 + 50;
	}
}
static void s2mu004_set_fast_charging_current(struct s2mu004_charger_data *charger, int charging_current)
{
	u8 data;
	
	if (factory_mode || JIG_USB_ON) {
		pr_info("%s: Factory Mode Skip charging_current Control\n", __func__);
		return;
	}
	if (charging_current <= 100)
		data = 0x03;
	else if (charging_current > 100 && charging_current <= 3150)
		data = (charging_current / 25) - 1;
	else
		data = 0x7D;
	s2mu004_update_reg(charger->i2c, S2MU004_CHG_CTRL9,
		data << FAST_CHARGING_CURRENT_SHIFT, FAST_CHARGING_CURRENT_MASK);
	pr_info("[DEBUG]%s: current  %d, 0x%02x\n", __func__, charging_current, data);
	if (data > 0x11)
		data = 0x11; /* 0x11 : 450mA */
	s2mu004_update_reg(charger->i2c, S2MU004_CHG_CTRL8,
		data << COOL_CHARGING_CURRENT_SHIFT, COOL_CHARGING_CURRENT_MASK);
#if EN_TEST_READ
	s2mu004_test_read(charger->i2c);
#endif
}
static int s2mu004_get_fast_charging_current(struct s2mu004_charger_data *charger)
{
	u8 data;
	
	s2mu004_read_reg(charger->i2c, S2MU004_CHG_CTRL9, &data);
	data = data & FAST_CHARGING_CURRENT_MASK;
	if (data > 0x7D) {
		pr_err("%s: Invalid fast charging current in register\n", __func__);
		data = 0x7D;
	}
	return (data + 1) * 25;
}
static void s2mu004_set_topoff_current(struct s2mu004_charger_data *charger,
						int eoc_1st_2nd, int current_limit)
{
	int data;
	
	pr_info("[DEBUG]%s: current  %d \n", __func__, current_limit);
	if (current_limit <= 100)
		data = 0;
	else if (current_limit > 100 && current_limit <= 475)
		data = (current_limit - 100) / 25;
	else
		data = 0x0F;
	switch (eoc_1st_2nd) {
	case 1:
		s2mu004_update_reg(charger->i2c, S2MU004_CHG_CTRL11,
			data << FIRST_TOPOFF_CURRENT_SHIFT, FIRST_TOPOFF_CURRENT_MASK);
		break;
	case 2:
		s2mu004_update_reg(charger->i2c, S2MU004_CHG_CTRL11,
			data << SECOND_TOPOFF_CURRENT_SHIFT, SECOND_TOPOFF_CURRENT_MASK);
		break;
	default:
		break;
	}
}
static int s2mu004_get_topoff_setting(struct s2mu004_charger_data *charger)
{
	u8 data;
	
	s2mu004_read_reg(charger->i2c, S2MU004_CHG_CTRL11, &data);
	data = data & FIRST_TOPOFF_CURRENT_MASK;
	
	return data * 25 + 100;
}
enum {
	S2MU004_CHG_2L_IVR_4300MV = 0,
	S2MU004_CHG_2L_IVR_4500MV,
	S2MU004_CHG_2L_IVR_4700MV,
	S2MU004_CHG_2L_IVR_4900MV,
};
#if ENABLE_MIVR
/* charger input regulation voltage setting */
static void s2mu004_set_ivr_level(struct s2mu004_charger_data *charger)
{
	int chg_2l_ivr = S2MU004_CHG_2L_IVR_4500MV;
	
	s2mu004_update_reg(charger->i2c, S2MU004_CHG_CTRL5,
		chg_2l_ivr << SET_CHG_2L_DROP_SHIFT, SET_CHG_2L_DROP_MASK);
}
#endif /*ENABLE_MIVR*/
static bool s2mu004_chg_init(struct s2mu004_charger_data *charger)
{
	u8 temp;
	
	/* Read Charger IC Dev ID */
	s2mu004_read_reg(charger->i2c, S2MU004_REG_REV_ID, &temp);
	charger->dev_id = (temp & 0xF0) >> 4;
	pr_info("%s : DEV ID : 0x%x\n", __func__, charger->dev_id);
	/* Poor-Chg-INT Masking */
	s2mu004_update_reg(charger->i2c, 0x32, 0x03, 0x03);
	/*
	 * When Self Discharge Function is activated, Charger doesn't stop charging.
	 * If you write 0xb0[4]=1, charger will stop the charging, when self discharge
	 * condition is satisfied.
	 */
	s2mu004_update_reg(charger->i2c, 0xb0, 0x0, 0x1 << 4);
	s2mu004_write_reg(charger->i2c, S2MU004_REG_SC_INT1_MASK, 0x0);
	s2mu004_write_reg(charger->i2c, S2MU004_REG_SC_INT2_MASK, 0x0);
	/* ready for self-discharge, 0x76 */
	s2mu004_update_reg(charger->i2c, S2MU004_REG_SELFDIS_CFG3,
			SELF_DISCHG_MODE_MASK, SELF_DISCHG_MODE_MASK);
	/* Set Top-Off timer to 90 minutes */
	s2mu004_update_reg(charger->i2c, S2MU004_CHG_CTRL17,
			S2MU004_TOPOFF_TIMER_90m << TOP_OFF_TIME_SHIFT,
			TOP_OFF_TIME_MASK);
	s2mu004_read_reg(charger->i2c, S2MU004_CHG_CTRL17, &temp);
	pr_info("%s : S2MU004_CHG_CTRL17 : 0x%x\n", __func__, temp);
	/* To prevent entering watchdog issue case we set WDT_CLR to not clear before enabling WDT */
	s2mu004_update_reg(charger->i2c, S2MU004_CHG_CTRL14,
			0x0 << WDT_CLR_SHIFT, WDT_CLR_MASK);
	/* enable Watchdog timer and only Charging off */
	s2mu004_update_reg(charger->i2c, S2MU004_CHG_CTRL13,
			ENABLE << SET_EN_WDT_SHIFT | DISABLE << SET_EN_WDT_AP_RESET_SHIFT,
			SET_EN_WDT_MASK | SET_EN_WDT_AP_RESET_MASK);
	s2mu004_read_reg(charger->i2c, S2MU004_CHG_CTRL13, &temp);
	pr_info("%s : S2MU004_CHG_CTRL13 : 0x%x\n", __func__, temp);
	/* set watchdog timer to 80 seconds */
	s2mu004_update_reg(charger->i2c, S2MU004_CHG_CTRL17,
			S2MU004_WDT_TIMER_80s << WDT_TIME_SHIFT, WDT_TIME_MASK);
	/* IVR Recovery enable */
	s2mu004_update_reg(charger->i2c, S2MU004_CHG_CTRL13,
		0x1 << SET_IVR_Recovery_SHIFT, SET_IVR_Recovery_MASK);
	/* Boost OSC 1Mhz */
	s2mu004_update_reg(charger->i2c, S2MU004_CHG_CTRL15,
		0x02 << SET_OSC_BST_SHIFT, SET_OSC_BST_MASK);
	/* QBAT switch speed config */
	s2mu004_update_reg(charger->i2c, 0xB2, 0x0, 0xf << 4);
	/* Top off debounce time set 1 sec */
	s2mu004_update_reg(charger->i2c, 0xC0, 0x3 << 6, 0x3 << 6);
	/* SC_CTRL21 register Minimum Charging OCP Level set to 6A */
	s2mu004_write_reg(charger->i2c, 0x29, 0x04);
	if (charger->pdata->chg_freq_ctrl) {
		switch (charger->pdata->chg_switching_freq) {
		case S2MU004_OSC_BUCK_FRQ_750kHz:
			s2mu004_update_reg(charger->i2c, S2MU004_CHG_CTRL12,
				S2MU004_OSC_BUCK_FRQ_750kHz << SET_OSC_BUCK_SHIFT, SET_OSC_BUCK_MASK);
			s2mu004_update_reg(charger->i2c, S2MU004_CHG_CTRL12,
				S2MU004_OSC_BUCK_FRQ_750kHz << SET_OSC_BUCK_3L_SHIFT, SET_OSC_BUCK_3L_MASK);
			break;
		default:
			/* Set OSC BUCK/BUCK 3L frequencies to default 1MHz */
			s2mu004_update_reg(charger->i2c, S2MU004_CHG_CTRL12,
				S2MU004_OSC_BUCK_FRQ_1MHz << SET_OSC_BUCK_SHIFT, SET_OSC_BUCK_MASK);
			s2mu004_update_reg(charger->i2c, S2MU004_CHG_CTRL12,
				S2MU004_OSC_BUCK_FRQ_1MHz << SET_OSC_BUCK_3L_SHIFT, SET_OSC_BUCK_3L_MASK);
			break;
		}
	}
	s2mu004_read_reg(charger->i2c, S2MU004_CHG_CTRL12, &temp);
	pr_info("%s : S2MU004_CHG_CTRL12 : 0x%x\n", __func__, temp);
	/*
	 * Disable auto-restart charging feature.
	 * Prevent charging restart after top-off timer expires
	 */
	s2mu004_update_reg(charger->i2c, S2MU004_CHG_CTRL7, 0x0 << 7, EN_CHG_RESTART_MASK);
	s2mu004_read_reg(charger->i2c, S2MU004_CHG_CTRL7, &temp);
	pr_info("%s : S2MU004_CHG_CTRL7 : 0x%x\n", __func__, temp);
	
	/* disable Pre Charge / Fast Charge Timer */
	s2mu004_update_reg(charger->i2c, 0x2B, 0x0, 0x20);
	s2mu004_read_reg(charger->i2c, 0x2B, &temp);
	pr_info("%s : REG 0x2B : 0x%x\n", __func__, temp);
	
	return true;
}
static int s2mu004_get_charging_status(struct s2mu004_charger_data *charger)
{
	int status = POWER_SUPPLY_STATUS_UNKNOWN;
	int ret;
	u8 chg_sts0, chg_sts1;
	union power_supply_propval value;
	
	ret = s2mu004_read_reg(charger->i2c, S2MU004_CHG_STATUS0, &chg_sts0);
	ret = s2mu004_read_reg(charger->i2c, S2MU004_CHG_STATUS1, &chg_sts1);
	psy_do_property(charger->pdata->fuelgauge_name, get,
		POWER_SUPPLY_PROP_CURRENT_AVG, value);
	if (ret < 0)
		return status;
	if (chg_sts1 & 0x80)
		status = POWER_SUPPLY_STATUS_DISCHARGING;
	else if (chg_sts1 & 0x02 || chg_sts1 & 0x01) {
		pr_info("%s: full check curr_avg(%d), topoff_curr(%d)\n",
			__func__, value.intval, charger->topoff_current);
		if (value.intval < charger->topoff_current)
			status = POWER_SUPPLY_STATUS_FULL;
		else
			status = POWER_SUPPLY_STATUS_CHARGING;
	} else if ((chg_sts0 & 0xE0) == 0xA0 || (chg_sts0 & 0xE0) == 0x60)
		status = POWER_SUPPLY_STATUS_CHARGING;
	else
		status = POWER_SUPPLY_STATUS_NOT_CHARGING;
#if EN_TEST_READ
	s2mu004_test_read(charger->i2c);
#endif

	return status;
}
#if 0
static int s2mu004_get_charge_type(struct s2mu004_charger_data *charger)
{
	int status = POWER_SUPPLY_CHARGE_TYPE_UNKNOWN;
	u8 ret;
	
	s2mu004_read_reg(charger->i2c, S2MU004_CHG_STATUS3, &ret);
	if (ret < 0)
		pr_err("%s fail\n", __func__);
	switch ((ret & BAT_STATUS_MASK) >> BAT_STATUS_SHIFT) {
	case 0x4:
	case 0x5:
		status = POWER_SUPPLY_CHARGE_TYPE_FAST;
		break;
	case 0x2:
		/* pre-charge mode */
		status = POWER_SUPPLY_CHARGE_TYPE_TRICKLE;
		break;
	}
	
	return status;
}
#endif
static bool s2mu004_get_batt_present(struct s2mu004_charger_data *charger)
{
	u8 ret;
	
	s2mu004_read_reg(charger->i2c, S2MU004_CHG_STATUS3, &ret);
	
	return (ret & DET_BAT_STATUS_MASK) ? true : false;
}
static void s2mu004_wdt_clear(struct s2mu004_charger_data *charger)
{
	u8 reg_data, chg_fault_status, en_chg;
	
	s2mu004_read_reg(charger->i2c, S2MU004_CHG_CTRL13, &reg_data);
	if (!(reg_data & 0x02)) {
		pr_info("%s : WDT disabled BUT Clear requested!\n", __func__);
		pr_info("%s : S2MU004_CHG_CTRL13 : 0x%x\n", __func__, reg_data);
		/* To prevent entering watchdog issue case we set WDT_CLR to not clear before enabling WDT */
		s2mu004_update_reg(charger->i2c, S2MU004_CHG_CTRL14, 0x0 << WDT_CLR_SHIFT, WDT_CLR_MASK);
		/* enable Watchdog timer and only Charging off */
		s2mu004_update_reg(charger->i2c, S2MU004_CHG_CTRL13,
				ENABLE << SET_EN_WDT_SHIFT | DISABLE << SET_EN_WDT_AP_RESET_SHIFT,
				SET_EN_WDT_MASK | SET_EN_WDT_AP_RESET_MASK);
		s2mu004_read_reg(charger->i2c, S2MU004_CHG_CTRL13, &reg_data);
		pr_info("%s : S2MU004_CHG_CTRL13 : 0x%x\n", __func__, reg_data);
	}
	/* watchdog kick */
	s2mu004_update_reg(charger->i2c, S2MU004_CHG_CTRL14, 0x1 << WDT_CLR_SHIFT, WDT_CLR_MASK);
	s2mu004_read_reg(charger->i2c, S2MU004_CHG_STATUS1, &reg_data);
	chg_fault_status = (reg_data & CHG_FAULT_STATUS_MASK) >> CHG_FAULT_STATUS_SHIFT;
	if ((chg_fault_status == CHG_STATUS_WD_SUSPEND) ||
		(chg_fault_status == CHG_STATUS_WD_RST)) {
		pr_info("%s: watchdog error status(0x%02x,%d)\n", __func__, reg_data, chg_fault_status);
		if (charger->is_charging) {
			pr_info("%s: toggle charger\n", __func__);
			s2mu004_enable_charger_switch(charger, false);
			s2mu004_enable_charger_switch(charger, true);
		}
	}
	s2mu004_read_reg(charger->i2c, S2MU004_CHG_CTRL0, &en_chg);
	if (!(en_chg & 0x80))
		s2mu004_update_reg(charger->i2c, S2MU004_CHG_CTRL0, 0x1 << EN_CHG_SHIFT, EN_CHG_MASK);
}
static int s2mu004_get_charging_health(struct s2mu004_charger_data *charger)
{
	u8 ret;
	union power_supply_propval value;
	
	if (charger->is_charging)
		s2mu004_wdt_clear(charger);
	/* To prevent disabling charging by top-off timer expiration */
	s2mu004_read_reg(charger->i2c, S2MU004_CHG_STATUS1, &ret);
	pr_info("[DEBUG] %s: S2MU004_CHG_STATUS1 0x%x\n", __func__, ret);
	if (ret & (DONE_STATUS_MASK)) {
		pr_err("add self chg done \n");
		/* add chg done code here */
		if (charger->status == POWER_SUPPLY_STATUS_CHARGING) {
			s2mu004_enable_charger_switch(charger, false);
			s2mu004_enable_charger_switch(charger, true);
			pr_err("Re-enable charging from Done_status \n");
		}
	}

	s2mu004_read_reg(charger->i2c, S2MU004_CHG_STATUS0, &ret);
	pr_info("[DEBUG] %s: S2MU004_CHG_STATUS0 0x%x\n", __func__, ret);
	if (is_wcin_type(charger->cable_type)) {
		ret = (ret & (WCIN_STATUS_MASK)) >> WCIN_STATUS_SHIFT;
	} else {
		ret = (ret & (CHGIN_STATUS_MASK)) >> CHGIN_STATUS_SHIFT;
	}
	switch (ret) {
	case 0x03:
	case 0x05:
		charger->ovp = false;
		charger->unhealth_cnt = 0;
		return POWER_SUPPLY_HEALTH_GOOD;
	default:
		break;
	}
	charger->unhealth_cnt++;
	if (charger->unhealth_cnt < HEALTH_DEBOUNCE_CNT)
		return POWER_SUPPLY_HEALTH_GOOD;
	/* 005 need to check ovp & health count */
	charger->unhealth_cnt = HEALTH_DEBOUNCE_CNT;
	if (charger->ovp)
		return POWER_SUPPLY_HEALTH_OVERVOLTAGE;
	psy_do_property("battery", get, POWER_SUPPLY_PROP_ONLINE, value);
	if (value.intval == SEC_BATTERY_CABLE_PDIC)
		return POWER_SUPPLY_HEALTH_UNDERVOLTAGE;
	else
		return POWER_SUPPLY_HEALTH_GOOD;
}
static int s2mu004_chg_create_attrs(struct device *dev)
{
	unsigned long i;
	int rc;
	
	for (i = 0; i < ARRAY_SIZE(s2mu004_charger_attrs); i++) {
		rc = device_create_file(dev, &s2mu004_charger_attrs[i]);
		if (rc)
			goto create_attrs_failed;
	}
	
	return rc;
create_attrs_failed:
	dev_err(dev, "%s: failed (%d)\n", __func__, rc);
	while (i--)
		device_remove_file(dev, &s2mu004_charger_attrs[i]);
	
	return rc;
}
ssize_t s2mu004_chg_show_attrs(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct s2mu004_charger_data *charger = container_of(psy, struct s2mu004_charger_data, psy_chg);
	const ptrdiff_t offset = attr - s2mu004_charger_attrs;
	int i = 0;
	u8 addr, data;
	
	switch (offset) {
	case CHIP_ID:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n", "S2MU004");
		break;
	case DATA:
		for (addr = 0x00; addr <= 0x24; addr++) {
			s2mu004_read_reg(charger->i2c, addr, &data);
			dev_err(dev, "%s: 0x%02x : 0x%02x, size: %ld\n", __func__, addr, data, PAGE_SIZE - i);
			i += scnprintf(buf + i, PAGE_SIZE - i,
					"0x%02x : 0x%02x\n", addr, data);
		}
		break;
	default:
		return -EINVAL;
	}
	
	return i;
}
ssize_t s2mu004_chg_store_attrs(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct s2mu004_charger_data *charger = container_of(psy, struct s2mu004_charger_data, psy_chg);
	const ptrdiff_t offset = attr - s2mu004_charger_attrs;
	int ret = 0;
	int x, y;
	
	switch (offset) {
	case CHIP_ID:
		ret = count;
		break;
	case DATA:
		if (sscanf(buf, "0x%8x 0x%8x", &x, &y) == 2) {
			if (x >= 0x00 && x <= 0x24) {
				u8 addr = x;
				u8 data = y;

				if (s2mu004_write_reg(charger->i2c, addr, data) < 0) {
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
static int s2mu004_chg_get_property(struct power_supply *psy, enum power_supply_property psp,
						union power_supply_propval *val)
{
	int chg_curr, aicr;
	u8 chg_sts0 = 0;
	struct s2mu004_charger_data *charger = container_of(psy, struct s2mu004_charger_data, psy_chg);
	enum power_supply_ext_property ext_psp = psp;

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = SEC_BATTERY_CABLE_NONE;
		if (s2mu004_read_reg(charger->i2c, S2MU004_CHG_STATUS0, &chg_sts0) == 0) {
			if (is_wcin_type(charger->cable_type)) {
				chg_sts0 = (chg_sts0 & (WCIN_STATUS_MASK)) >> WCIN_STATUS_SHIFT;
			} else {
				chg_sts0 = (chg_sts0 & (CHGIN_STATUS_MASK)) >> CHGIN_STATUS_SHIFT;
			}
			if (chg_sts0 == 0x05 || chg_sts0 == 0x03) {
				if (is_wcin_type(charger->cable_type))
					val->intval = SEC_BATTERY_CABLE_WIRELESS;
				else
					val->intval = SEC_BATTERY_CABLE_TA;
			}
		}
		break;
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = s2mu004_get_charging_status(charger);
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = s2mu004_get_charging_health(charger);
		s2mu004_test_read(charger->i2c);
		break;
	case POWER_SUPPLY_PROP_CURRENT_MAX:
		val->intval = s2mu004_get_input_current_limit(charger);
		break;
	case POWER_SUPPLY_PROP_CURRENT_AVG:
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		if (charger->charging_current) {
			aicr = s2mu004_get_input_current_limit(charger);
			chg_curr = s2mu004_get_fast_charging_current(charger);
			val->intval = MINVAL(aicr, chg_curr);
		} else
			val->intval = 0;
		break;
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
		val->intval = s2mu004_get_fast_charging_current(charger);
		break;
	case POWER_SUPPLY_PROP_CURRENT_FULL:
		val->intval = s2mu004_get_topoff_setting(charger);
		break;
	case POWER_SUPPLY_PROP_CHARGE_TYPE:
		break;
#if defined(CONFIG_BATTERY_SWELLING)
	case POWER_SUPPLY_PROP_VOLTAGE_MAX:
		val->intval = s2mu004_get_regulation_voltage(charger);
		break;
#endif
#if defined(CONFIG_AFC_CHARGER_MODE)
	case POWER_SUPPLY_PROP_AFC_CHARGER_MODE:
		return -ENODATA;
#endif
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = s2mu004_get_batt_present(charger);
		break;
	case POWER_SUPPLY_PROP_CHARGING_ENABLED:
		val->intval = charger->is_charging;
		break;
	case POWER_SUPPLY_PROP_INPUT_VOLTAGE_REGULATION:
		break;
	case POWER_SUPPLY_PROP_MAX ... POWER_SUPPLY_EXT_PROP_MAX: 
		switch (ext_psp) {
			case POWER_SUPPLY_EXT_PROP_CHIP_ID:
				{
					u8 reg_data = 0;
					if (s2mu004_read_reg(charger->i2c, S2MU004_REG_REV_ID, &reg_data) == 0) {
						val->intval = ((reg_data & 0xF0) >> 4 > 0 ? 1 : 0);
						pr_info("%s : device id = 0x%x \n", __func__, reg_data);
					} else {
						val->intval = 0;
						pr_info("%s :S2MU004 I2C fail \n", __func__);
					}
				}
				break;
			case POWER_SUPPLY_EXT_PROP_WDT_STATUS:
				break;
			default:
				return -ENODATA;
		}
		break;
	default:
		return -EINVAL;
	}
	
	return 0;
}
static int s2mu004_chg_set_property(struct power_supply *psy, enum power_supply_property psp,
						const union power_supply_propval *val)
{
	struct s2mu004_charger_data *charger = container_of(psy, struct s2mu004_charger_data, psy_chg);
	enum power_supply_ext_property ext_psp = psp;
	int buck_state = ENABLE;
	union power_supply_propval value;
	
	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		charger->status = val->intval;
		break;
		/* val->intval : type */
	case POWER_SUPPLY_PROP_ONLINE:
		charger->cable_type = val->intval;
		pr_info("[DEBUG] %s: charger->cable_type(%d)\n", __func__, charger->cable_type);
		charger->ivr_on = false;
#if defined(CONFIG_MUIC_S2MU004_SUPPORT_BC1P2_CERTI)
		if (charger->cable_type == SEC_BATTERY_CABLE_NONE)
			s2mu004_set_input_current_limit(charger, 100);
#endif
		if (charger->cable_type != SEC_BATTERY_CABLE_OTG) {
			if (is_nocharge_type(charger->cable_type)) {
				value.intval = 0;
			} else {
#if ENABLE_MIVR
				s2mu004_set_ivr_level(charger);
#endif
				value.intval = 1;
			}
			psy_do_property(charger->pdata->fuelgauge_name,
					set, POWER_SUPPLY_PROP_ENERGY_AVG, value);
		}
		break;
	case POWER_SUPPLY_PROP_CURRENT_MAX:
		{
			int input_current = val->intval;
			s2mu004_set_input_current_limit(charger, input_current);
			charger->input_current = input_current;
		}
		break;
	case POWER_SUPPLY_PROP_CURRENT_AVG:
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		pr_info("[DEBUG] %s: is_charging(%d)\n", __func__, charger->is_charging);
		charger->charging_current = val->intval;
		s2mu004_set_fast_charging_current(charger, charger->charging_current);
#if EN_TEST_READ
		s2mu004_test_read(charger->i2c);
#endif
		break;
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
		break;
	case POWER_SUPPLY_PROP_CURRENT_FULL:
		charger->topoff_current = val->intval;
		if (charger->pdata->chg_eoc_dualpath) {
			s2mu004_set_topoff_current(charger, 1, val->intval);
			s2mu004_set_topoff_current(charger, 2, 100);
		} else
			s2mu004_set_topoff_current(charger, 1, val->intval);
		break;
#if defined(CONFIG_BATTERY_SWELLING)
	case POWER_SUPPLY_PROP_VOLTAGE_MAX:
		pr_info("[DEBUG]%s: float voltage(%d)\n", __func__, val->intval);
		charger->pdata->chg_float_voltage = val->intval;
		s2mu004_set_regulation_voltage(charger,	charger->pdata->chg_float_voltage);
		break;
#endif
	case POWER_SUPPLY_PROP_CHARGE_OTG_CONTROL:
		s2mu004_charger_otg_control(charger, val->intval);
		break;
	case POWER_SUPPLY_PROP_CHARGE_UNO_CONTROL:
		{
			u8 chg_sts2, chg_ctrl0;
			pr_info("%s: called charger uno control : %s\n", __func__,
				val->intval > 0 ? "ON" : "OFF");

			if (factory_mode) {
				u8 reg_val;

				pr_info("%s: Can`t be tx boost when factory mode.\n", __func__);
				s2mu004_update_reg(charger->i2c, S2MU004_CHG_CTRL18,
					val->intval > 0 ? 0x3 : 0x1, 0x3);
				s2mu004_read_reg(charger->i2c, S2MU004_CHG_CTRL18, &reg_val);
				pr_info("%s S2MU004_CHG_CTRL18: 0x%x\n", __func__, reg_val);
				break;
			}
			s2mu004_update_reg(charger->i2c, S2MU004_CHG_CTRL0,
				val->intval > 0 ? TX_BST_MODE : CHG_MODE, REG_MODE_MASK);
#if defined(CONFIG_SEC_FACTORY)
			s2mu004_update_reg(charger->i2c, 0x2C,
				val->intval > 0 ? 0x3 << 4 : 0x1 << 4, 0x3 << 4);
#endif
			s2mu004_read_reg(charger->i2c, S2MU004_CHG_STATUS2, &chg_sts2);
			s2mu004_read_reg(charger->i2c, S2MU004_CHG_CTRL0, &chg_ctrl0);
			pr_info("%s S2MU004_CHG_STATUS2: 0x%x\n", __func__, chg_sts2);
			pr_info("%s S2MU004_CHG_CTRL0: 0x%x\n", __func__, chg_ctrl0);
		}
		break;
	case POWER_SUPPLY_PROP_CHARGING_ENABLED:
		charger->charge_mode = val->intval;
		switch (charger->charge_mode) {
			case SEC_BAT_CHG_MODE_BUCK_OFF:
				buck_state = DISABLE;
			case SEC_BAT_CHG_MODE_CHARGING_OFF:
				charger->is_charging = false;
				break;
			case SEC_BAT_CHG_MODE_CHARGING:
				charger->is_charging = true;
				break;
		}
		if (buck_state) {
			s2mu004_enable_charger_switch(charger, charger->is_charging);
		} else {
			/* set buck off only if SEC_BAT_CHG_MODE_BUCK_OFF */
			s2mu004_set_buck(charger, buck_state);
		}
		break;
	case POWER_SUPPLY_PROP_ENERGY_NOW:
#if 0
		/* Switch-off charger if JIG is connected */
		if (val->intval && factory_mode) {
			pr_info("%s: JIG Connection status: %d \n", __func__, val->intval);
			s2mu004_enable_charger_switch(charger, false);
		}
#endif
		break;
	case POWER_SUPPLY_PROP_INPUT_VOLTAGE_REGULATION:
		if (val->intval) {
			pr_info("%s: Relieve VBUS2BAT\n", __func__);
			s2mu004_write_reg(charger->i2c, 0x2F, 0xDD);
			s2mu004_update_reg(charger->i2c, 0xDA, 0x10, 0x10);
		}
		break;
	case POWER_SUPPLY_PROP_AUTHENTIC:
		if (val->intval) {
			pr_info("%s: set Bypass mode by AT CMD for measuring sleep/off current\n", __func__);
			s2mu004_update_reg(charger->i2c, S2MU004_CHG_CTRL18, 0xC0, 0xC0);
			s2mu004_update_reg(charger->i2c, 0x29, 0x01 << 1, 0x01 << 1);
			s2mu004_update_reg(charger->i2c, 0x9F, 0x0, 0x01 << 7);
			s2mu004_update_reg(charger->i2c, S2MU004_CHG_CTRL0, 0x01 << 5, 0x01 << 5);
			/* USB LDO off */
			s2mu004_update_reg(charger->i2c, S2MU004_PWRSEL_CTRL0,
				0 << PWRSEL_CTRL0_SHIFT, PWRSEL_CTRL0_MASK);
			
			pr_info("%s additional setting start %d\n", __func__, __LINE__);
			s2mu004_update_reg(charger->i2c, 0x93, 0x40, 0x40);
			s2mu004_update_reg(charger->i2c, 0xC6, 0x00, 0x40);
			s2mu004_update_reg(charger->i2c, 0x8B, 0x00, 0xFF);
			s2mu004_update_reg(charger->i2c, 0x71, 0x0F, 0xFF);
			/* CHGINOK set LOW active*/
			s2mu004_update_reg(charger->i2c, 0x94, 0x80, 0x80);
			pr_info("%s complete %d\n", __func__, __LINE__);
		}
		break;
#if EN_IVR_IRQ
	case POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT_MAX:
	{
		u8 reg_data = 0;
		s2mu004_enable_ivr_irq(charger);
		s2mu004_read_reg(charger->i2c, S2MU004_CHG_STATUS3, &reg_data);
		if (reg_data & IVR_STATUS)
			queue_delayed_work(charger->charger_wqueue,
				&charger->ivr_work, msecs_to_jiffies(50));
		break;
	}
#endif
#if defined(CONFIG_AFC_CHARGER_MODE)
	case POWER_SUPPLY_PROP_AFC_CHARGER_MODE:
		s2mu004_hv_muic_charger_init();
		break;
#endif
	case POWER_SUPPLY_PROP_CHARGE_TYPE:
		{
			u8 reg_data = 0;

			s2mu004_read_reg(charger->i2c,
				S2MU004_CHG_STATUS0, &reg_data);
			reg_data = (reg_data & (WCIN_STATUS_MASK)) >> WCIN_STATUS_SHIFT;

			pr_info("%s S2MU004_CHG_STATUS0: 0x%x\n", __func__, reg_data);

			if (reg_data == 0x05 || reg_data == 0x03) {
				wake_lock(&charger->wpc_wake_lock);
				queue_delayed_work(charger->charger_wqueue, &charger->wpc_work, 0);
			}
		}
		break;
	case POWER_SUPPLY_PROP_MAX ... POWER_SUPPLY_EXT_PROP_MAX:
		switch (ext_psp) {
		case POWER_SUPPLY_EXT_PROP_FUELGAUGE_RESET:
			s2mu004_write_reg(charger->i2c, 0x6F, 0xC4);
			msleep(1000);
			s2mu004_write_reg(charger->i2c, 0x6F, 0x04);
			msleep(50);
			pr_info("%s: reset fuelgauge when surge occur!\n", __func__);
			break;
		case POWER_SUPPLY_EXT_PROP_FACTORY_VOLTAGE_REGULATION:
			/* enable EN_JIG_AP */
			s2mu004_update_reg(charger->i2c, S2MU004_CHG_CTRL1,
					1 << EN_JIG_REG_AP_SHIFT, EN_JIG_REG_AP_MASK);
			pr_info("%s: factory voltage regulation (%d)\n", __func__, val->intval);
			s2mu004_set_regulation_vsys(charger, val->intval);
			break;
		case POWER_SUPPLY_EXT_PROP_ANDIG_IVR_SWITCH:
#if !defined(CONFIG_SEC_FACTORY)
			if (charger->dev_id < 0x3) {
				s2mu004_analog_ivr_switch(charger, val->intval);
			}
#endif
			break;
		case POWER_SUPPLY_EXT_PROP_CURRENT_MEASURE:
			if (val->intval) {
				pr_info("%s: set Bypass mode by keystring for measuring consumption current\n", __func__);
				/*
				 * Charger/muic interrupt can occur by entering Bypass mode
				 * Disable all interrupt mask for testing current measure.
				 */
				s2mu004_write_reg(charger->i2c, S2MU004_REG_SC_INT1_MASK, 0xFF);
				s2mu004_write_reg(charger->i2c, S2MU004_REG_SC_INT2_MASK, 0xFF);
				s2mu004_write_reg(charger->i2c, S2MU004_REG_AFC_INT_MASK, 0xFF);
				s2mu004_write_reg(charger->i2c, S2MU004_REG_MUIC_INT1_MASK, 0xFF);
				s2mu004_write_reg(charger->i2c, S2MU004_REG_MUIC_INT2_MASK, 0xFF);
	
				/* Enter Bypass mode set for current measure */
				s2mu004_update_reg(charger->i2c, S2MU004_CHG_CTRL0, 0x01 << 4, 0x01 << 4); /* enable factory mode */
				s2mu004_write_reg(charger->i2c, S2MU004_CHG_CTRL2, 0x7F);
				
				s2mu004_write_reg(charger->i2c, 0x2F, 0xDD); /* POWER_SUPPLY_PROP_INPUT_VOLTAGE_REGULATION */
				s2mu004_update_reg(charger->i2c, 0xDA, 0x10, 0x10);
				
				msleep(500);
				s2mu004_update_reg(charger->i2c, S2MU004_CHG_CTRL18, 0xc0, 0xc0); /* POWER_SUPPLY_PROP_AUTHENTIC */
				s2mu004_update_reg(charger->i2c, 0x29, 0x01 << 1, 0x01 << 1);
				s2mu004_update_reg(charger->i2c, 0x9F, 0x00, 0x80);
				s2mu004_update_reg(charger->i2c, S2MU004_CHG_CTRL0, 0x01 << 5, 0x01 << 5);
				/* USB LDO off */
				s2mu004_update_reg(charger->i2c, S2MU004_PWRSEL_CTRL0,
					0 << PWRSEL_CTRL0_SHIFT, PWRSEL_CTRL0_MASK);
				
				psy_do_property("s2mu004-fuelgauge", set,
					POWER_SUPPLY_EXT_PROP_FUELGAUGE_FACTORY, value);
			} else {
				pr_info("%s: Bypass exit for current measure\n", __func__);
				s2mu004_update_reg(charger->i2c, 0x29, 0x0, 0x01 << 1);
				s2mu004_write_reg(charger->i2c, S2MU004_CHG_CTRL0, 0x00);
				/* USB LDO on */
				s2mu004_update_reg(charger->i2c, S2MU004_PWRSEL_CTRL0,
					1 << PWRSEL_CTRL0_SHIFT, PWRSEL_CTRL0_MASK);
			}
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
static int s2mu004_otg_get_property(struct power_supply *psy, enum power_supply_property psp,
						union power_supply_propval *val)
{
	struct s2mu004_charger_data *charger =
		container_of(psy, struct s2mu004_charger_data, psy_otg);
	u8 reg;
	
	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = charger->otg_on;
		break;
	case POWER_SUPPLY_PROP_CHARGE_POWERED_OTG_CONTROL:
		s2mu004_read_reg(charger->i2c, S2MU004_CHG_STATUS2, &reg);
		pr_info("%s: S2MU004_CHG_STATUS2 : 0x%X\n", __func__, reg);
		if ((reg & 0xE0) == 0x60) {
			val->intval = 1;
		} else {
			val->intval = 0;
		}
		s2mu004_read_reg(charger->i2c, S2MU004_CHG_CTRL0, &reg);
		pr_info("%s: S2MU004_CHG_CTRL0 : 0x%X\n", __func__, reg);
		break;
	default:
		return -EINVAL;
	}
	
	return 0;
}
static int s2mu004_otg_set_property(struct power_supply *psy, enum power_supply_property psp,
						const union power_supply_propval *val)
{
	struct s2mu004_charger_data *charger = container_of(psy, struct s2mu004_charger_data, psy_otg);
	union power_supply_propval value;
	
	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		value.intval = val->intval;
		pr_info("%s: OTG %s\n", __func__, value.intval > 0 ? "ON" : "OFF");
		psy_do_property(charger->pdata->charger_name, set,
					POWER_SUPPLY_PROP_CHARGE_OTG_CONTROL, value);
		break;
	default:
		return -EINVAL;
	}
	
	return 0;
}
static void s2mu004_wpc_detect_work(struct work_struct *work)
{
	struct s2mu004_charger_data *charger = container_of(work, struct s2mu004_charger_data, wpc_work.work);
	int wc_w_state;
	union power_supply_propval value;
	u8 reg_data;
	
	s2mu004_read_reg(charger->i2c, S2MU004_CHG_STATUS0, &reg_data);
	reg_data = (reg_data & (WCIN_STATUS_MASK)) >> WCIN_STATUS_SHIFT;
	pr_info("%s S2MU004_CHG_STATUS0: 0x%x\n", __func__, reg_data);
	/*  101: WCIN is valid or okay, 011: WCIN Input current is limited or regulated */
	if (reg_data == 0x05 || reg_data == 0x03)
		wc_w_state = 1;
	else
		wc_w_state = 0;

	if ((charger->wc_w_state == 0) && (wc_w_state == 1)) {
		value.intval = 1;
		psy_do_property(charger->pdata->support_pogo ? "pogo" : "wireless", set,
				POWER_SUPPLY_PROP_ONLINE, value);
		pr_info("%s: wpc activated, set V_INT as PN\n", __func__);
	} else if ((charger->wc_w_state == 1) && (wc_w_state == 0)) {
		s2mu004_read_reg(charger->i2c, S2MU004_CHG_STATUS0, &reg_data);
		reg_data = (reg_data & (WCIN_STATUS_MASK)) >> WCIN_STATUS_SHIFT;
		pr_info("%s: reg_data: 0x%x, charging: %d\n", __func__,	reg_data, charger->is_charging);
		value.intval = 0;
		psy_do_property(charger->pdata->support_pogo ? "pogo" : "wireless", set,
				POWER_SUPPLY_PROP_ONLINE, value);
		pr_info("%s: wpc deactivated, set V_INT as PD\n", __func__);
	}
	pr_info("%s: w(%d to %d)\n", __func__, charger->wc_w_state, wc_w_state);
	charger->wc_w_state = wc_w_state;
	wake_unlock(&charger->wpc_wake_lock);
}
#if EN_BAT_DET_IRQ
/* s2mu004 interrupt service routine */
static irqreturn_t s2mu004_det_bat_isr(int irq, void *data)
{
	struct s2mu004_charger_data *charger = data;
	u8 val;
	
	s2mu004_read_reg(charger->i2c, S2MU004_CHG_STATUS3, &val);
	pr_info("[IRQ] %s , STATUS3 : %02x\n", __func__, val);
	if ((val & DET_BAT_STATUS_MASK) == 0) {
		s2mu004_enable_charger_switch(charger, 0);
		pr_err("charger-off if battery removed \n");
	}
	
	return IRQ_HANDLED;
}
#endif
#if EN_OTG_FAULT_IRQ
static irqreturn_t s2mu004_otg_fault_isr(int irq, void *data)
{
	struct s2mu004_charger_data *charger = data;
	union power_supply_propval value;
	u8 val;

#ifdef CONFIG_USB_HOST_NOTIFY
	struct otg_notify *o_notify = get_otg_notify();
#endif
	s2mu004_read_reg(charger->i2c, S2MU004_CHG_STATUS2, &val);
	pr_info("[IRQ] %s , STATUS2 : %02x \n ", __func__, val);

	if ((val & OTG_STATUS_MASK) == (0x3 << OTG_STATUS_SHIFT)) {
		msleep(20);
		 /* read one more after delay because SCP */
		s2mu004_read_reg(charger->i2c, S2MU004_CHG_STATUS2, &val);
		pr_info("[IRQ] %s , STATUS2 : %02x \n ", __func__, val);
		if ((val & OTG_STATUS_MASK) != (0x3 << OTG_STATUS_SHIFT))
			return IRQ_HANDLED;

		pr_info("%s: OTG OVERCURRENT LIMIT!!\n", __func__);
#ifdef CONFIG_USB_HOST_NOTIFY
		send_otg_notify(o_notify, NOTIFY_EVENT_OVERCURRENT, 0);
#endif
		value.intval = 0;
		psy_do_property("otg", set, POWER_SUPPLY_PROP_ONLINE, value);
	}	

	return IRQ_HANDLED;
}
#endif

static irqreturn_t s2mu004_done_isr(int irq, void *data)
{
	struct s2mu004_charger_data *charger = data;
	u8 val;
	
	s2mu004_read_reg(charger->i2c, S2MU004_CHG_STATUS1, &val);
	pr_info("[IRQ] %s , STATUS1 : %02x \n ", __func__, val);
	if (val & (DONE_STATUS_MASK)) {
		pr_err("add self chg done \n");
		/* add chg done code here */
	}
	
	return IRQ_HANDLED;
}
static irqreturn_t s2mu004_chg_isr(int irq, void *data)
{
	struct s2mu004_charger_data *charger = data;
	union power_supply_propval value;
	u8 val;
	
	s2mu004_read_reg(charger->i2c, S2MU004_CHG_STATUS0, &val);
	pr_info("[IRQ] %s , STATUS0 : %02x\n ", __func__, val);
#if EN_OVP_IRQ
	if ((val & CHGIN_STATUS_MASK) == (2 << CHGIN_STATUS_SHIFT))	{
		charger->ovp = true;
		pr_info("%s: OVP triggered\n", __func__);
		value.intval = POWER_SUPPLY_HEALTH_OVERVOLTAGE;
		s2mu004_update_reg(charger->i2c, 0xBE, 0x10, 0x10);
		psy_do_property("battery", set, POWER_SUPPLY_PROP_HEALTH, value);
	} else if ((val & CHGIN_STATUS_MASK) == (3 << CHGIN_STATUS_SHIFT) ||
			(val & CHGIN_STATUS_MASK) == (5 << CHGIN_STATUS_SHIFT)) {
		pr_info("%s: Vbus status 0x%x \n ", __func__, val);
		charger->unhealth_cnt = HEALTH_DEBOUNCE_CNT;
		if (charger->ovp == true)
			pr_info("%s: recover from OVP\n", __func__);
		charger->ovp = false;
		value.intval = POWER_SUPPLY_HEALTH_GOOD;
		s2mu004_update_reg(charger->i2c, 0xBE, 0x00, 0x10);
		psy_do_property("battery", set,	POWER_SUPPLY_PROP_HEALTH, value);
	}
#endif

	return IRQ_HANDLED;
}
static irqreturn_t s2mu004_event_isr(int irq, void *data)
{
	struct s2mu004_charger_data *charger = data;
	u8 val0, val1, val2, val3;
	
	s2mu004_read_reg(charger->i2c, S2MU004_CHG_STATUS0, &val0);
	s2mu004_read_reg(charger->i2c, S2MU004_CHG_STATUS1, &val1);
	s2mu004_read_reg(charger->i2c, S2MU004_CHG_STATUS2, &val2);
	s2mu004_read_reg(charger->i2c, S2MU004_CHG_STATUS3, &val3);
	pr_info("[IRQ] %s , STATUS0:0x%02x, STATUS1:0x%02x, STATUS2:0x%02x, STATUS3:0x%02x\n",
			__func__, val0, val1, val2, val3);
	
	return IRQ_HANDLED;
}
static irqreturn_t s2mu004_ovp_isr(int irq, void *data)
{
	pr_info("%s ovp!\n", __func__);
	
	return IRQ_HANDLED;
}
#if EN_IVR_IRQ
static void s2mu004_ivr_irq_work(struct work_struct *work)
{
	struct s2mu004_charger_data *charger = container_of(work,
				struct s2mu004_charger_data, ivr_work.work);
	u8 ivr_state;
	int ivr_cnt = 0;

	pr_info("%s:\n", __func__);
	wake_lock(&charger->ivr_wake_lock);
	mutex_lock(&charger->charger_mutex);
	/* Mask IRQ */
	s2mu004_update_reg(charger->i2c, S2MU004_REG_SC_INT2_MASK, S2MU004_IVR_M, S2MU004_IVR_M);
	s2mu004_read_reg(charger->i2c, S2MU004_CHG_STATUS3, &ivr_state);
	while ((ivr_state & IVR_STATUS) && !is_nocharge_type(charger->cable_type)) {
		if (s2mu004_read_reg(charger->i2c, S2MU004_CHG_STATUS3, &ivr_state)) {
			pr_err("%s: Error reading S2MU004_CHG_STATUS3\n", __func__);
			break;
		}
		pr_info("%s: S2MU004_CHG_STATUS3:0x%02x\n", __func__, ivr_state);
		if (!(ivr_state & IVR_STATUS)) {
			pr_info("%s: EXIT IVR WORK: check value (S2MU004_CHG_STATUS3:0x%02x, input current:%d)\n",
				__func__, ivr_state, charger->input_current);
			break;
		}
		if (s2mu004_get_input_current_limit(charger) <= MINIMUM_INPUT_CURRENT)
			break;
		if (++ivr_cnt >= 2) {
			reduce_input_current(charger);
			ivr_cnt = 0;
		}
		mdelay(50);
	}
	if (charger->ivr_on) {
		union power_supply_propval value;
		value.intval = s2mu004_get_input_current_limit(charger);
		psy_do_property("battery", set, POWER_SUPPLY_EXT_PROP_AICL_CURRENT, value);
	}
	/* Unmask IRQ */
	s2mu004_update_reg(charger->i2c, S2MU004_REG_SC_INT2_MASK, 0 << IVR_M_SHIFT, IVR_M_MASK);
	mutex_unlock(&charger->charger_mutex);
	wake_unlock(&charger->ivr_wake_lock);
}

static irqreturn_t s2mu004_ivr_isr(int irq, void *data)
{
	struct s2mu004_charger_data *charger = data;

	pr_info("%s: Start\n", __func__);
	queue_delayed_work(charger->charger_wqueue, &charger->ivr_work,
		msecs_to_jiffies(50));

	return IRQ_HANDLED;
}

static void s2mu004_enable_ivr_irq(struct s2mu004_charger_data *charger)
{
	int ret;

	ret = request_threaded_irq(charger->irq_ivr, NULL,
			s2mu004_ivr_isr, 0, "ivr-irq", charger);
	if (ret < 0) {
		pr_err("%s: Fail to request IVR_INT IRQ: %d: %d\n",
					__func__, charger->irq_ivr, ret);
		charger->irq_ivr_enabled = -1;
	} else {
		/* Unmask IRQ */
		s2mu004_update_reg(charger->i2c, S2MU004_REG_SC_INT2_MASK,
			0 << IVR_M_SHIFT, IVR_M_MASK);
		charger->irq_ivr_enabled = 1;
	}
	pr_info("%s enabled : %d\n", __func__, charger->irq_ivr_enabled);
}
#endif
static irqreturn_t s2mu004_chg_wpcin_isr(int irq, void *data)
{
	struct s2mu004_charger_data *charger = data;
	unsigned long delay;
	
	if (charger->wc_w_state)
		delay = msecs_to_jiffies(50);
	else
		delay = msecs_to_jiffies(0);
	pr_info("%s: IRQ=%d delay = %ld\n", __func__, irq, delay);

	cancel_delayed_work(&charger->wpc_work);
	wake_lock(&charger->wpc_wake_lock);
	queue_delayed_work(charger->charger_wqueue, &charger->wpc_work, delay);
	
	return IRQ_HANDLED;
}
static int s2mu004_charger_parse_dt(struct device *dev,	struct s2mu004_charger_platform_data *pdata)
{
	struct device_node *np = of_find_node_by_name(NULL, "s2mu004-charger");
	int ret = 0;
	
	if (!np) {
		pr_err("%s np NULL(s2mu004-charger)\n", __func__);
	} else {
		pdata->wcinokb = of_get_named_gpio(np, "charger,wcinokb", 0);
		pr_info("%s : pdata->wcinokb : %d\n", __func__, pdata->wcinokb);
		if (pdata->wcinokb < 0) {
			pr_info("%s : can't get wcinokb\n", __func__);
			pdata->wcinokb = 0;
		}
	}
	np = of_find_node_by_name(NULL, "battery");
	if (!np) {
		pr_err("%s np NULL\n", __func__);
	} else {
		ret = of_property_read_string(np, "battery,fuelgauge_name",
			(char const **)&pdata->fuelgauge_name);
		if (ret < 0)
			pr_info("%s: Fuel-gauge name is Empty\n", __func__);

		ret = of_property_read_u32(np, "battery,support_pogo",
			&pdata->support_pogo);
		if (ret) {
			pr_info("%s : support_pogo is Empty\n", __func__);
			pdata->support_pogo = false;
		}

		ret = of_property_read_u32(np, "battery,chg_float_voltage", &pdata->chg_float_voltage);
		if (ret) {
			pr_info("%s: battery,chg_float_voltage is Empty\n", __func__);
			pdata->chg_float_voltage = 4200;
		}
		pr_info("%s: battery,chg_float_voltage is %d\n", __func__, pdata->chg_float_voltage);
		pdata->chg_eoc_dualpath = of_property_read_bool(np, "battery,chg_eoc_dualpath");
	}
	pr_info("%s DT file parsed successfully, %d\n", __func__, ret);
	
	return ret;
}
/* if need to set s2mu004 pdata */
static struct of_device_id s2mu004_charger_match_table[] = {
	{ .compatible = "samsung,s2mu004-charger",},
	{},
};
static int s2mu004_charger_probe(struct platform_device *pdev)
{
	struct s2mu004_dev *s2mu004 = dev_get_drvdata(pdev->dev.parent);
	struct s2mu004_platform_data *pdata = dev_get_platdata(s2mu004->dev);
	struct s2mu004_charger_data *charger;
	int ret = 0;
	
	pr_info("%s:[BATT] S2MU004 Charger driver probe\n", __func__);
	charger = kzalloc(sizeof(*charger), GFP_KERNEL);
	if (!charger)
		return -ENOMEM;
	mutex_init(&charger->charger_mutex);
	charger->otg_on = false;
	charger->ivr_on = false;
	charger->cable_type = SEC_BATTERY_CABLE_NONE;
	charger->dev = &pdev->dev;
	charger->i2c = s2mu004->i2c;
	charger->pdata = devm_kzalloc(&pdev->dev, sizeof(*(charger->pdata)), GFP_KERNEL);
	if (!charger->pdata) {
		dev_err(&pdev->dev, "Failed to allocate memory\n");
		ret = -ENOMEM;
		goto err_parse_dt_nomem;
	}
	ret = s2mu004_charger_parse_dt(&pdev->dev, charger->pdata);
	if (ret < 0)
		goto err_parse_dt;
	platform_set_drvdata(pdev, charger);
	if (charger->pdata->charger_name == NULL)
		charger->pdata->charger_name = "s2mu004-charger";
	if (charger->pdata->fuelgauge_name == NULL)
		charger->pdata->fuelgauge_name = "s2mu004-fuelgauge";

	charger->psy_chg.name           = "s2mu004-charger";
	charger->psy_chg.type           = POWER_SUPPLY_TYPE_UNKNOWN;
	charger->psy_chg.get_property   = s2mu004_chg_get_property;
	charger->psy_chg.set_property   = s2mu004_chg_set_property;
	charger->psy_chg.properties     = s2mu004_charger_props;
	charger->psy_chg.num_properties = ARRAY_SIZE(s2mu004_charger_props);
	charger->psy_otg.name		= "otg";
	charger->psy_otg.type		= POWER_SUPPLY_TYPE_OTG;
	charger->psy_otg.get_property	= s2mu004_otg_get_property;
	charger->psy_otg.set_property	= s2mu004_otg_set_property;
	charger->psy_otg.properties	= s2mu004_otg_props;
	charger->psy_otg.num_properties	= ARRAY_SIZE(s2mu004_otg_props);

	s2mu004_chg_init(charger);
	charger->input_current = s2mu004_get_input_current_limit(charger);
	charger->charging_current = s2mu004_get_fast_charging_current(charger);
	ret = power_supply_register(&pdev->dev, &charger->psy_chg);
	if (ret) {
		goto err_power_supply_register;
	}

	ret = power_supply_register(&pdev->dev, &charger->psy_otg);
	if (ret) {
		goto err_power_supply_register_otg;
	}
	charger->charger_wqueue = create_singlethread_workqueue("charger-wq");
	if (!charger->charger_wqueue) {
		pr_info("%s: failed to create wq.\n", __func__);
		ret = -ESRCH;
		goto err_create_wq;
	}

	wake_lock_init(&charger->wpc_wake_lock, WAKE_LOCK_SUSPEND, "charger-wpc");
#if EN_IVR_IRQ
	wake_lock_init(&charger->ivr_wake_lock, WAKE_LOCK_SUSPEND, "charger-ivr");
	INIT_DELAYED_WORK(&charger->ivr_work, s2mu004_ivr_irq_work);
#endif
	/*
	 * irq request
	 * if you need to add an irq, please refer to the code below.
	 */
	INIT_DELAYED_WORK(&charger->wpc_work, s2mu004_wpc_detect_work);
	
	charger->irq_sys = pdata->irq_base + S2MU004_CHG1_IRQ_SYS;
	ret = request_threaded_irq(charger->irq_sys, NULL,
			s2mu004_ovp_isr, 0, "sys-irq", charger);
	if (ret < 0) {
		dev_err(s2mu004->dev, "%s: Fail to request SYS in IRQ: %d: %d\n",
					__func__, charger->irq_sys, ret);
		goto err_reg_irq;
	}

	if (charger->pdata->wcinokb > 0) {
		charger->irq_wcinokb = gpio_to_irq(charger->pdata->wcinokb);
		ret = request_threaded_irq(charger->irq_wcinokb, NULL,
				s2mu004_chg_wpcin_isr, 
				IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
				"wcin-irq", charger);
		if (ret < 0) {
			dev_err(s2mu004->dev, "%s: Fail to request WCINOKB in IRQ: %d: %d\n",
						__func__, charger->irq_wcinokb, ret);
			goto err_reg_irq;
		}
	}

	charger->irq_wcin = pdata->irq_base + S2MU004_CHG1_IRQ_WCIN;
	ret = request_threaded_irq(charger->irq_wcin, NULL,
			s2mu004_chg_wpcin_isr, 0, "wcin-irq", charger);
	if (ret < 0) {
		dev_err(s2mu004->dev, "%s: Fail to request WCIN in IRQ: %d: %d\n",
					__func__, charger->irq_wcin, ret);
		goto err_reg_irq;
	}
#if EN_BAT_DET_IRQ
	charger->irq_det_bat = pdata->irq_base + S2MU004_CHG2_IRQ_DET_BAT;
	ret = request_threaded_irq(charger->irq_det_bat, NULL,
			s2mu004_det_bat_isr, 0, "det_bat-irq", charger);
	if (ret < 0) {
		dev_err(s2mu004->dev, "%s: Fail to request DET_BAT in IRQ: %d: %d\n",
					__func__, charger->irq_det_bat, ret);
		goto err_reg_irq;
	}
#endif
#if EN_OTG_FAULT_IRQ
	charger->irq_otg_fault = pdata->irq_base + S2MU004_CHG2_IRQ_OTG_Fault;
	ret = request_threaded_irq(charger->irq_otg_fault, NULL,
			s2mu004_otg_fault_isr, 0, "otg-fault-irq", charger);
	if (ret < 0) {
		dev_err(s2mu004->dev, "%s: Fail to request OTG FAULT in IRQ: %d: %d\n",
					__func__, charger->irq_otg_fault, ret);
		goto err_reg_irq;
	}
#endif
	charger->irq_chgin = pdata->irq_base + S2MU004_CHG1_IRQ_CHGIN;
	ret = request_threaded_irq(charger->irq_chgin, NULL,
			s2mu004_chg_isr, 0, "chgin-irq", charger);
	if (ret < 0) {
		dev_err(s2mu004->dev, "%s: Fail to request CHGIN in IRQ: %d: %d\n",
					__func__, charger->irq_chgin, ret);
		goto err_reg_irq;
	}
	charger->irq_rst = pdata->irq_base + S2MU004_CHG1_IRQ_CHG_RSTART;
	ret = request_threaded_irq(charger->irq_rst, NULL,
			s2mu004_chg_isr, 0, "restart-irq", charger);
	if (ret < 0) {
		dev_err(s2mu004->dev, "%s: Fail to request CHG_Restart in IRQ: %d: %d\n",
					__func__, charger->irq_rst, ret);
		goto err_reg_irq;
	}
	charger->irq_done = pdata->irq_base + S2MU004_CHG1_IRQ_DONE;
	ret = request_threaded_irq(charger->irq_done, NULL,
			s2mu004_done_isr, 0, "done-irq", charger);
	if (ret < 0) {
		dev_err(s2mu004->dev, "%s: Fail to request DONE in IRQ: %d: %d\n",
					__func__, charger->irq_done, ret);
		goto err_reg_irq;
	}
	charger->irq_chg_fault = pdata->irq_base + S2MU004_CHG1_IRQ_CHG_Fault;
	ret = request_threaded_irq(charger->irq_chg_fault, NULL,
			s2mu004_event_isr, 0, "chg_fault-irq", charger);
	if (ret < 0) {
		dev_err(s2mu004->dev, "%s: Fail to request CHG_Fault in IRQ: %d: %d\n",
					__func__, charger->irq_chg_fault, ret);
		goto err_reg_irq;
	}
	charger->irq_bat = pdata->irq_base + S2MU004_CHG2_IRQ_BAT;
	ret = request_threaded_irq(charger->irq_bat, NULL,
			s2mu004_event_isr, 0, "bat-irq", charger);
	if (ret < 0) {
		dev_err(s2mu004->dev, "%s: Fail to request DET_BAT in IRQ: %d: %d\n",
					__func__, charger->irq_bat, ret);
		goto err_reg_irq;
	}
#if EN_IVR_IRQ
	charger->irq_ivr = pdata->irq_base + S2MU004_CHG2_IRQ_IVR;
#endif
	ret = s2mu004_chg_create_attrs(charger->psy_chg.dev);
	if (ret) {
		dev_err(charger->dev, "%s : Failed to create_attrs\n", __func__);
		goto err_reg_irq;
	}
	s2mu004_test_read(charger->i2c);
	pr_info("%s:[BATT] S2MU004 charger driver loaded OK\n", __func__);
	return 0;
err_reg_irq:
	destroy_workqueue(charger->charger_wqueue);
	power_supply_unregister(&charger->psy_otg);
err_create_wq:
err_power_supply_register_otg:
	power_supply_unregister(&charger->psy_chg);
err_power_supply_register:
err_parse_dt:
err_parse_dt_nomem:
	mutex_destroy(&charger->charger_mutex);
	kfree(charger);
	
	return ret;
}
static int s2mu004_charger_remove(struct platform_device *pdev)
{
	struct s2mu004_charger_data *charger =
		platform_get_drvdata(pdev);
	
	power_supply_unregister(&charger->psy_chg);
	mutex_destroy(&charger->charger_mutex);
	kfree(charger);
	
	return 0;
}
#if defined CONFIG_PM
static int s2mu004_charger_suspend(struct device *dev)
{
	return 0;
}
static int s2mu004_charger_resume(struct device *dev)
{
	return 0;
}
#else
#define s2mu004_charger_suspend NULL
#define s2mu004_charger_resume NULL
#endif
static void s2mu004_charger_shutdown(struct device *dev)
{
	struct s2mu004_charger_data *charger = dev_get_drvdata(dev);

	pr_info("%s: S2MU004 Charger driver shutdown\n", __func__);
	if (!charger->i2c) {
		pr_err("%s: no S2MU004 i2c client\n", __func__);
		return;
	}

	/* set default input current */
	s2mu004_update_reg(charger->i2c, S2MU004_CHG_CTRL2,
		0x12 << INPUT_CURRENT_LIMIT_SHIFT, INPUT_CURRENT_LIMIT_MASK);
	s2mu004_update_reg(charger->i2c, S2MU004_CHG_CTRL3,
		0x25 << INPUT_CURRENT_LIMIT_SHIFT, INPUT_CURRENT_LIMIT_MASK);
	
	/* maintain Bypass mode set for current measure */
#if !defined(CONFIG_SEC_FACTORY)
	s2mu004_write_reg(charger->i2c, S2MU004_CHG_CTRL0, 0x83);
	s2mu004_write_reg(charger->i2c, S2MU004_CHG_CTRL18, 0x55);
#endif
	s2mu004_update_reg(charger->i2c, S2MU004_CHG_CTRL1,
		0 << SEL_PRIO_WCIN_SHIFT, SEL_PRIO_WCIN_MASK);
}
static SIMPLE_DEV_PM_OPS(s2mu004_charger_pm_ops, s2mu004_charger_suspend,
		s2mu004_charger_resume);
static struct platform_driver s2mu004_charger_driver = {
	.driver         = {
		.name	= "s2mu004-charger",
		.owner	= THIS_MODULE,
		.of_match_table = s2mu004_charger_match_table,
		.pm		= &s2mu004_charger_pm_ops,
		.shutdown	=	s2mu004_charger_shutdown,
	},
	.probe          = s2mu004_charger_probe,
	.remove		= s2mu004_charger_remove,
};
static int __init s2mu004_charger_init(void)
{
	int ret = 0;

	pr_info("%s: S2MU004 Charger Init\n", __func__);
	ret = platform_driver_register(&s2mu004_charger_driver);
	
	return ret;
}
module_init(s2mu004_charger_init);
static void __exit s2mu004_charger_exit(void)
{
	platform_driver_unregister(&s2mu004_charger_driver);
}
module_exit(s2mu004_charger_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Samsung Electronics");
MODULE_DESCRIPTION("Charger driver for S2MU004");
