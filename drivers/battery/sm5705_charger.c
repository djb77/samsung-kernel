
/*
 *  /drivers/battery/sm5705_charger.c
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

#include <linux/battery/charger/sm5705_charger.h>
#include <linux/battery/charger/sm5705_charger_oper.h>

//#define SM5705_CHG_FULL_DEBUG 1

#define ENABLE 1
#define DISABLE 0

enum {
	SM5705_CHG_SRC_VBUS = 0x0,
	SM5705_CHG_SRC_WPC,
	SM5705_CHG_SRC_MAX,
};

enum {
	SM5705_CHG_OTG_CURRENT_0_5A     = 0x0,
	SM5705_CHG_OTG_CURRENT_0_7A,
	SM5705_CHG_OTG_CURRENT_0_9A,
	SM5705_CHG_OTG_CURRENT_1_5A,
};

enum {
	SM5705_CHG_BST_IQ3LIMIT_2_0A    = 0x0,
	SM5705_CHG_BST_IQ3LIMIT_2_8A,
	SM5705_CHG_BST_IQ3LIMIT_3_5A,
	SM5705_CHG_BST_IQ3LIMIT_4_0A,
};

/* Interrupt status Index & Offset */
enum {
	SM5705_INT_STATUS1 = 0x0,
	SM5705_INT_STATUS2,
	SM5705_INT_STATUS3,
	SM5705_INT_STATUS4,
	SM5705_INT_MAX,
};

enum {
	SM5705_INT_STATUS1_VBUSPOK          = 0x0,
	SM5705_INT_STATUS1_VBUSUVLO,
	SM5705_INT_STATUS1_VBUSOVP,
	SM5705_INT_STATUS1_VBUSLIMIT,
	SM5705_INT_STATUS1_WPCINPOK,
	SM5705_INT_STATUS1_WPCINUVLO,
	SM5705_INT_STATUS1_WPCINOVP,
	SM5705_INT_STATUS1_WPCINLIMIT,
};

enum {
	SM5705_INT_STATUS2_AICL             = 0x0,
	SM5705_INT_STATUS2_BATOVP,
	SM5705_INT_STATUS2_NOBAT,
	SM5705_INT_STATUS2_CHGON,
	SM5705_INT_STATUS2_Q4FULLON,
	SM5705_INT_STATUS2_TOPOFF,
	SM5705_INT_STATUS2_DONE,
	SM5705_INT_STATUS2_WDTMROFF,
};

enum {
	SM5705_INT_STATUS3_THEMREG          = 0x0,
	SM5705_INT_STATUS3_THEMSHDN,
	SM5705_INT_STATUS3_OTGFAIL,
	SM5705_INT_STATUS3_DISLIMIT,
	SM5705_INT_STATUS3_PRETMROFF,
	SM5705_INT_STATUS3_FASTTMROFF,
	SM5705_INT_STATUS3_LOWBATT,
	SM5705_INT_STATUS3_nEQ4,
};

enum {
	SM5705_INT_STATUS4_FLED1SHORT       = 0x0,
	SM5705_INT_STATUS4_FLED1OPEN,
	SM5705_INT_STATUS4_FLED2SHORT,
	SM5705_INT_STATUS4_FLED2OPEN,
	SM5705_INT_STATUS4_BOOSTPOK_NG,
	SM5705_INT_STATUS4_BOOSTPOL,
	SM5705_INT_STATUS4_ABSTMR1OFF,
	SM5705_INT_STATUS4_SBPS,
};

#if defined(SM5705_USED_WIRELESS_CHARGER)
#define __n_is_cable_type_for_wireless(cable_type) \
					((cable_type != POWER_SUPPLY_TYPE_WIRELESS) && \
					(cable_type != POWER_SUPPLY_TYPE_HV_WIRELESS) && \
					(cable_type != POWER_SUPPLY_TYPE_PMA_WIRELESS) && \
					(cable_type != POWER_SUPPLY_TYPE_HV_WIRELESS_ETX))

#define __is_cable_type_for_wireless(cable_type)  \
					((cable_type == POWER_SUPPLY_TYPE_WIRELESS) || \
					(cable_type == POWER_SUPPLY_TYPE_PMA_WIRELESS))

#define __is_cable_type_for_hv_wireless(cable_type)  \
					((cable_type == POWER_SUPPLY_TYPE_HV_WIRELESS) || \
					(cable_type == POWER_SUPPLY_TYPE_HV_WIRELESS_ETX))
#else
#define __n_is_cable_type_for_wireless(cable_type)  (true)
#define __is_cable_type_for_wireless(cable_type)    (false)
#define __is_cable_type_for_hv_wireless(cable_type) (false)
#endif

#define __is_cable_type_for_hv_mains(cable_type)  \
					((cable_type == POWER_SUPPLY_TYPE_HV_MAINS) || \
					(cable_type == POWER_SUPPLY_TYPE_HV_PREPARE_MAINS) || \
					(cable_type == POWER_SUPPLY_TYPE_HV_ERR))

static struct device_attribute sm5705_charger_attrs[] = {
	SM5705_CHARGER_ATTR(chip_id),
};

/**
 *  SM5705 Charger device register control functions
 */

#if defined(SM5705_WATCHDOG_RESET_ACTIVATE)
static struct sm5705_charger_data *g_sm5705_charger;
static int sm5705_CHG_set_WATCHDOG_TMR(struct sm5705_charger_data *charger,
				unsigned char wdt_timer)
{
	sm5705_update_reg(charger->i2c,
		SM5705_REG_CHGCNTL8, ((wdt_timer & 0x3) << 5), (0x3 << 5));
	pr_info("WATCHDOG_TMR set (timer=%d)\n", wdt_timer);

	return 0;
}

static int sm5705_CHG_set_ENWATCHDOG(struct sm5705_charger_data *charger,
				bool enable, bool enchg_reon)
{
	unsigned char reg_val = (enchg_reon << 4) | (enable << 3);

	sm5705_update_reg(charger->i2c, SM5705_REG_CNTL, reg_val, (0x3 << 3));
	pr_info("ENWATCHDOG set (enable=%d, enCHG_REON=%d)\n", enable, enchg_reon);

	return 0;
}

static void sm5705_CHG_set_WDTMR_RST(struct sm5705_charger_data *charger)
{
	sm5705_update_reg(charger->i2c, SM5705_REG_CHGCNTL8, (0x1 << 7), (0x1 << 7));
}
#endif

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
		offset = 0x15;    /* default Offset : 4.2V */
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

static int sm5705_CHG_set_FREQSEL(struct sm5705_charger_data *charger,
                unsigned char freq_index)
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

	pr_info("[CHG] %s\n", str);
}

/**
 *  SM5705 Charger Driver support functions
 */

#if defined(CONFIG_BATTERY_SWELLING_SELF_DISCHARGING)
static bool sm5705_charger_get_discharging_force_status(struct sm5705_charger_data *charger)
{
	unsigned char reg;
	if (sm5705_call_fg_device_id() <= 2) {
		pr_info("unsupported this function under rev02.\n");
		return false;
	}
	reg = sm5705_CHG_read_reg(charger, SM5705_REG_FACTORY);
	pr_info("enable:(%s), reg[0x%02X]:0x%02x\n",
		(reg & SM5705_EN_DISCHG_FORCE_MASK) ? "ON" : "OFF" , SM5705_REG_FACTORY, reg);

	if (reg & SM5705_EN_DISCHG_FORCE_MASK)
		return true;
	else
		return false;
}
#endif
#if defined(CONFIG_BATTERY_SWELLING_SELF_DISCHARGING) || defined(CONFIG_SEC_FACTORY)
static void sm5705_charger_en_discharging_force(struct sm5705_charger_data *charger, bool enable)
{
	unsigned char reg;
	if (sm5705_call_fg_device_id() <= 2) {
		pr_info("unsupported this function under rev02.\n");
		return;
	}

	if (enable)
		sm5705_update_reg(charger->i2c, SM5705_REG_FACTORY,
			SM5705_EN_DISCHG_FORCE_MASK, SM5705_EN_DISCHG_FORCE_MASK);
	else
		sm5705_update_reg(charger->i2c, SM5705_REG_FACTORY,
			0, SM5705_EN_DISCHG_FORCE_MASK);

	reg = sm5705_CHG_read_reg(charger, SM5705_REG_FACTORY);
	pr_info("enable(%s), reg[0x%02X]:0x%02x\n",
		enable ? "ON" : "OFF", SM5705_REG_FACTORY, reg);
}
#endif

#if defined(CONFIG_SEC_FACTORY)
static void sm5705_dis_discharging_force_work(struct work_struct *work)
{
	struct sm5705_charger_data *charger =
		container_of(work, struct sm5705_charger_data, dis_discharging_force_work.work);

	pr_info("schedule work start.\n");

	sm5705_charger_en_discharging_force(charger, false);
	
	pr_info("schedule work done.\n");
}
#endif

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

	if (!(__n_is_cable_type_for_wireless(charger->cable_type))) {
		get_current = sm5705_CHG_get_INPUT_LIMIT(charger, SM5705_CHG_SRC_WPC);
	} else {
		get_current = sm5705_CHG_get_INPUT_LIMIT(charger, SM5705_CHG_SRC_VBUS);
	}
#ifdef SM5705_CHG_FULL_DEBUG
	pr_info("src_type=%d, current=%d\n",
		__n_is_cable_type_for_wireless(charger->cable_type), get_current);
#endif

	return get_current;
}

static int sm5705_get_charge_current(struct sm5705_charger_data *charger)
{
	int get_current;

	if (!(__n_is_cable_type_for_wireless(charger->cable_type))) {
		get_current = sm5705_CHG_get_FASTCHG(charger, SM5705_CHG_SRC_WPC);
	} else {
		get_current = sm5705_CHG_get_FASTCHG(charger, SM5705_CHG_SRC_VBUS);
	}
	pr_info("src_type=%d, current=%d\n",
		__n_is_cable_type_for_wireless(charger->cable_type), get_current);

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

	sm5705_chg_test_read(charger);
	pr_info("turn-%s Charging enable pin\n", enable ? "ON" : "OFF");
}

static int sm5705_set_charge_current(struct sm5705_charger_data *charger,
				unsigned short charge_current)
{
	if (!(__n_is_cable_type_for_wireless(charger->cable_type))) {
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

	if (!(__n_is_cable_type_for_wireless(charger->cable_type))) {
		sm5705_CHG_set_INPUT_LIMIT(charger, SM5705_CHG_SRC_WPC, input_current);
	} else {
		sm5705_CHG_set_INPUT_LIMIT(charger, SM5705_CHG_SRC_VBUS, input_current);
	}

	return 0;
}

/**
 *  SM5705 Power-supply class management functions
 */
static void sm5705_set_operation_mode(struct sm5705_charger_data *charger, unsigned int prev_cable_type)
{
	union power_supply_propval value;
	
	int prev_op_mode, prev_status;
	prev_op_mode = sm5705_charger_oper_get_current_op_mode();
	prev_status = sm5705_charger_oper_get_current_status();

	if (charger->cable_type == POWER_SUPPLY_TYPE_POWER_SHARING) {
		psy_do_property("ps", get, POWER_SUPPLY_PROP_STATUS, value);
		sm5705_charger_oper_push_event(SM5705_CHARGER_OP_EVENT_PWR_SHAR, value.intval);
	} else if (charger->cable_type == POWER_SUPPLY_TYPE_OTG) {
		pr_info("OTG enable, cable(%d) prev_cable_type(%d)\n", charger->cable_type,prev_cable_type);
#if defined(CONFIG_SM5705_SUPPORT_GAMEPAD)
		if (prev_cable_type == POWER_SUPPLY_TYPE_MDOCK_TA) 
			sm5705_charger_oper_push_event(SM5705_CHARGER_OP_EVENT_VBUS, DISABLE);
#endif		
		sm5705_charger_oper_push_event(SM5705_CHARGER_OP_EVENT_OTG, ENABLE);
	} else if (charger->cable_type == POWER_SUPPLY_TYPE_BATTERY) {
		/* set default value */
		sm5705_charger_oper_push_event(SM5705_CHARGER_OP_EVENT_VBUS, DISABLE);
		sm5705_charger_oper_push_event(SM5705_CHARGER_OP_EVENT_PWR_SHAR, DISABLE);
		sm5705_charger_oper_push_event(SM5705_CHARGER_OP_EVENT_OTG, DISABLE);

		if (charger->pdata->support_slow_charging)
			cancel_delayed_work(&charger->aicl_work);
	} else {
#if defined(SM5705_USED_WIRELESS_CHARGER)
		if (__is_cable_type_for_wireless(charger->cable_type) ||
			__is_cable_type_for_hv_wireless(charger->cable_type)) {
			sm5705_charger_oper_push_event(SM5705_CHARGER_OP_EVENT_VBUS, DISABLE);
			sm5705_charger_oper_push_event(SM5705_CHARGER_OP_EVENT_WPC, ENABLE);
		} else {
			sm5705_charger_oper_push_event(SM5705_CHARGER_OP_EVENT_WPC, DISABLE);
			sm5705_charger_oper_push_event(SM5705_CHARGER_OP_EVENT_VBUS, ENABLE);
		}
#else
		sm5705_charger_oper_push_event(SM5705_CHARGER_OP_EVENT_VBUS, ENABLE);
#endif
		if (charger->pdata->support_slow_charging) {
			pr_info("request aicl work for 5V-TA\n");
			queue_delayed_work(charger->wqueue, &charger->aicl_work, msecs_to_jiffies(3000));
		}
	}

	pr_info("cable = %d, op_mode(%d->%d), op_status(0x%x->0x%x)\n",
		charger->cable_type,
		prev_op_mode, sm5705_charger_oper_get_current_op_mode(),
		prev_status, sm5705_charger_oper_get_current_status());
}

static void psy_chg_set_usb_hc(struct sm5705_charger_data *charger, int value)
{
	if (value) {
		/* set input/charging current for usb up to TA's current */
		charger->pdata->charging_current[POWER_SUPPLY_TYPE_USB].fast_charging_current =
			charger->pdata->charging_current[POWER_SUPPLY_TYPE_MAINS].fast_charging_current;
		charger->pdata->charging_current[POWER_SUPPLY_TYPE_USB].input_current_limit =
			charger->pdata->charging_current[POWER_SUPPLY_TYPE_MAINS].input_current_limit;
	} else {
		/* restore input/charging current for usb */
		charger->pdata->charging_current[POWER_SUPPLY_TYPE_USB].fast_charging_current =
			charger->pdata->charging_current[POWER_SUPPLY_TYPE_BATTERY].input_current_limit;
		charger->pdata->charging_current[POWER_SUPPLY_TYPE_USB].input_current_limit =
			charger->pdata->charging_current[POWER_SUPPLY_TYPE_BATTERY].input_current_limit;
	}
}

#if defined(SM5705_SUPPORT_OTG_CONTROL)
static void psy_chg_set_charge_otg_control(struct sm5705_charger_data *charger, int otg_en)
{
	union power_supply_propval value;

	psy_do_property("wireless", get, POWER_SUPPLY_PROP_ONLINE, value);

	if (otg_en && !value.intval) {
		/* OTG - Enable */
		sm5705_charger_oper_push_event(SM5705_CHARGER_OP_EVENT_VBUS, DISABLE);
		sm5705_charger_oper_push_event(SM5705_CHARGER_OP_EVENT_OTG, ENABLE);
		pr_info("OTG enable, cable(%d)\n", charger->cable_type);
	} else {
		/* OTG - Disable */
		if (charger->cable_type == POWER_SUPPLY_TYPE_BATTERY)
			sm5705_charger_oper_push_event(SM5705_CHARGER_OP_EVENT_VBUS, DISABLE);
		sm5705_charger_oper_push_event(SM5705_CHARGER_OP_EVENT_OTG, DISABLE);
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
	muic_charger_init();
}
#endif

static int sm5705_chg_set_property(struct power_supply *psy,
				enum power_supply_property psp, const union power_supply_propval *val)
{
	struct sm5705_charger_data *charger =
		container_of(psy, struct sm5705_charger_data, psy_chg);
	unsigned int prev_cable_type = charger->cable_type;;

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		pr_info("POWER_SUPPLY_PROP_STATUS - status=%d\n", val->intval);
		charger->status = val->intval;
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		pr_info("POWER_SUPPLY_PROP_ONLINE - cable(%d->%d)\n",
			charger->cable_type, val->intval);
		charger->cable_type = val->intval;
		charger->input_current = charger->pdata->charging_current
				[charger->cable_type].input_current_limit;

		sm5705_set_operation_mode(charger, prev_cable_type);
		break;
	case POWER_SUPPLY_PROP_CURRENT_MAX:
		pr_info("POWER_SUPPLY_PROP_CURRENT_MAX - current=%d\n", val->intval);
		{
			int input_current = val->intval;
			if (charger->input_current < input_current) {
				input_current = charger->input_current;
			}
			sm5705_set_input_current(charger, val->intval);
		}		
		break;
	case POWER_SUPPLY_PROP_CURRENT_AVG:
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		pr_info("POWER_SUPPLY_PROP_CURRENT_AVG/NOW - current=%d\n", val->intval);
		charger->charging_current = val->intval;
		if (charger->is_charging) {
			/* Fast charge and Termination current */
			pr_info("%s : fast charging current (%dmA)\n",
				__func__, charger->charging_current);
			sm5705_set_charge_current(charger, charger->charging_current);
		}
			break;
	case POWER_SUPPLY_PROP_CURRENT_FULL:
		if (charger->pdata->full_check_type_2nd == SEC_BATTERY_FULLCHARGED_CHGPSY) {
			sm5705_CHG_set_TOPOFF(charger, val->intval);
		}
		break;
#if defined(CONFIG_AFC_CHARGER_MODE)
	case POWER_SUPPLY_PROP_AFC_CHARGER_MODE:
		psy_chg_set_afc_charger_mode(charger, val->intval);
		break;
#endif
#if defined(CONFIG_BATTERY_SWELLING) || defined(CONFIG_BATTERY_SWELLING_SELF_DISCHARGING)
	case POWER_SUPPLY_PROP_VOLTAGE_MAX:
		charger->pdata->chg_float_voltage = val->intval;
		sm5705_CHG_set_BATREG(charger, val->intval);
		break;
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
#if defined(SM5705_SUPPORT_AICL_CONTROL)
	case POWER_SUPPLY_PROP_CHARGE_AICL_CONTROL:
		pr_info("POWER_SUPPLY_PROP_CHARGE_AICL_CONTROL - aicl_en=%d\n", val->intval);
		psy_chg_set_aicl_control(charger, val->intval);
		break;
#endif
    case POWER_SUPPLY_PROP_CHARGING_ENABLED:
		pr_info("POWER_SUPPLY_PROP_CHARGING_ENABLED - charge_mode=%d\n", val->intval);
		charger->charge_mode = val->intval;
		switch (charger->charge_mode) {
		case SEC_BAT_CHG_MODE_BUCK_OFF:
		case SEC_BAT_CHG_MODE_CHARGING_OFF:
			charger->is_charging = false;
			break;
		case SEC_BAT_CHG_MODE_CHARGING:
			charger->is_charging = true;
			break;
		}
		sm5705_enable_charging_on_switch(charger, charger->is_charging);
		break;
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
		/* charger off when jig detected */
		if (val->intval)
			sm5705_enable_charging_on_switch(charger, false);
		break;
#if defined(CONFIG_BATTERY_SWELLING_SELF_DISCHARGING)
	case POWER_SUPPLY_PROP_RESISTANCE:
		sm5705_charger_en_discharging_force(charger, val->intval);
		break;
#endif
	case POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT_MAX:
		break;
	default:
		pr_err("un-known Power-supply property type (psp=%d)\n", psp);
		return -EINVAL;
	}

	return 0;
}

static int psy_chg_get_charge_source_type(struct sm5705_charger_data *charger)
{
	int src_type;

	if (sm5705_CHG_get_INT_STATUS(charger, SM5705_INT_STATUS1, SM5705_INT_STATUS1_VBUSPOK)) {
		src_type = POWER_SUPPLY_TYPE_MAINS;
	} else if (sm5705_CHG_get_INT_STATUS(charger, SM5705_INT_STATUS1, SM5705_INT_STATUS1_WPCINPOK)) {
		src_type = POWER_SUPPLY_TYPE_WIRELESS;
	} else {
		src_type = POWER_SUPPLY_TYPE_BATTERY;
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

	if (sm5705_charger_get_charging_on_status(charger)) {
		if (sm5705_get_input_current(charger) == SLOW_CHARGING_CURRENT_STANDARD) {
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

	pr_info("is_charging=%d(%d), cable_type=%d, input_limit=%d, chg_curr=%d, REG_STATUS1=0x%x\n",
		charger->is_charging, sm5705_charger_get_power_source_status(charger),
		charger->cable_type, charger->input_current, charger->charging_current,
		reg_data);

	if (charger->cable_type != POWER_SUPPLY_TYPE_WIRELESS) {
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
	int i = 0;

	switch (offset){
	case CHIP_ID:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n", "SM5705");
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

	switch(offset){
	case CHIP_ID:
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

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = psy_chg_get_charge_source_type(charger);
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
		break;
	case POWER_SUPPLY_PROP_CURRENT_MAX:
		val->intval = sm5705_get_input_current(charger);
		break;
	case POWER_SUPPLY_PROP_CURRENT_AVG:
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		val->intval = sm5705_get_charge_current(charger);
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX:
		val->intval = sm5705_CHG_get_BATREG(charger);
		break;
#if defined(CONFIG_AFC_CHARGER_MODE)
	case POWER_SUPPLY_PROP_AFC_CHARGER_MODE:
		return -ENODATA;
#endif
#if defined(SM5705_SUPPORT_OTG_CONTROL)
	case POWER_SUPPLY_PROP_CHARGE_OTG_CONTROL:
		val->intval = sm5705_charger_check_oper_otg_mode_on();
		break;
#endif
#if defined(SM5705_SUPPORT_AICL_CONTROL)
	case POWER_SUPPLY_PROP_CHARGE_AICL_CONTROL:
		return -ENODATA;
#endif
	case POWER_SUPPLY_PROP_CHARGING_ENABLED:
		val->intval = sm5705_charger_get_power_source_status(charger);
		break;
	case POWER_SUPPLY_PROP_ENERGY_NOW:
		return -ENODATA;
#if defined(CONFIG_BATTERY_SWELLING_SELF_DISCHARGING)
	case POWER_SUPPLY_PROP_RESISTANCE:
		val->intval = sm5705_charger_get_discharging_force_status(charger);
		break;
#endif
	default:
		pr_err("un-known Power-supply property type (psp=%d)\n", psp);
		return -EINVAL;
	}

	return 0;
}

static enum power_supply_property sm5705_charger_props[] = {
};

static int sm5705_otg_get_property(struct power_supply *psy,
				enum power_supply_property psp, union power_supply_propval *val)
{
	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = sm5705_charger_check_oper_otg_mode_on();
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
	union power_supply_propval value;

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		pr_info("POWER_SUPPLY_PROP_ONLINE - %s\n", (val->intval) ? "ON" : "OFF");
#if defined(SM5705_SUPPORT_OTG_CONTROL)
		value.intval = val->intval;
		psy_do_property("sm5705-charger", set, POWER_SUPPLY_PROP_CHARGE_OTG_CONTROL, value);
#else
		if (val->intval) {
			value.intval = POWER_SUPPLY_TYPE_OTG;
		} else {
			value.intval = POWER_SUPPLY_TYPE_BATTERY;
		}
		psy_do_property("sm5705-charger", set, POWER_SUPPLY_PROP_ONLINE, value);
#endif
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

#if EN_TOPOFF_IRQ
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
#endif /*EN_TOPOFF_IRQ*/

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

	pr_info("vbus_limit_current=%d, charger->input_current=%d\n",
		vbus_limit_current, charger->input_current);
}

static void _check_slow_charging(struct sm5705_charger_data *charger, int input_current)
{
	/* under 400mA considered as slow charging concept for VZW */
	if (input_current <= SLOW_CHARGING_CURRENT_STANDARD &&
		charger->cable_type != POWER_SUPPLY_TYPE_BATTERY) {
		union power_supply_propval value;

		pr_info("slow charging on : input current(%dmA), cable type(%d)\n",
			input_current, charger->cable_type);

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

	if (!charger->pdata->support_slow_charging || sm5705_call_fg_device_id() <= 2) {
		pr_info("don't support AICL work at REV.2\n");
		return;
	}

	if (!sm5705_charger_get_charging_on_status(charger) ||
		__is_cable_type_for_hv_mains(charger->cable_type)) {
		pr_info("don't need AICL work\n");
		return;
	}

	pr_info("schedule work start.\n");

	/* Reduce input limit current */
	max_count = charger->input_current / REDUCE_CURRENT_STEP;
	prev_current_max = charger->input_current;
	while (_check_aicl_state(charger) && (now_count++ < max_count)) {
		_reduce_input_limit_current(charger, REDUCE_CURRENT_STEP);
		msleep(AICL_VALID_CHECK_DELAY_TIME);
	}
	if (prev_current_max > charger->input_current) {
		pr_info("charging_current_max(%d --> %d)\n",
			prev_current_max, charger->input_current);
		_check_slow_charging(charger, charger->input_current);
	}

	pr_info("schedule work done.\n");
}

#if defined(SM5705_USED_WIRELESS_CHARGER)
static void wc_afc_detect_work(struct work_struct *work)
{
	struct sm5705_charger_data *charger = container_of(work, struct sm5705_charger_data, wc_afc_work.work);

	pr_info("schedule work start.\n");

	if (__is_cable_type_for_wireless(charger->cable_type) && \
		sm5705_charger_get_charging_on_status(charger) && charger->wc_afc_detect) {
		charger->wc_afc_detect = false;

		if (charger->input_current >=
			charger->pdata->charging_current[POWER_SUPPLY_TYPE_WIRELESS].input_current_limit) {
			charger->input_current =
				charger->pdata->charging_current[POWER_SUPPLY_TYPE_WIRELESS].input_current_limit;
		}
		pr_info("current_max(%d)\n", charger->input_current);
	}

	pr_info("schedule work doen.\n");
}

static void wpc_detect_work(struct work_struct *work)
{
	struct sm5705_charger_data *charger = container_of(work, struct sm5705_charger_data, wpc_work.work);
	union power_supply_propval value;
	int wpcin_state;

	pr_info("schedule work start.\n");

#if defined(CONFIG_WIRELESS_CHARGER_P9220)
	wpcin_state = !gpio_get_value(charger->pdata->irq_gpio);
#else
	wpcin_state = gpio_get_value(charger->pdata->wpc_det);
#endif
	pr_info("wc_w_state = %d \n", wpcin_state);

	if ((charger->irq_wpcin_state == 0) && (wpcin_state == 1)) {
		value.intval = 1;
		psy_do_property("wireless", set, POWER_SUPPLY_PROP_ONLINE, value);
		value.intval = POWER_SUPPLY_TYPE_WIRELESS;
		psy_do_property(charger->pdata->wireless_charger_name, set, POWER_SUPPLY_PROP_ONLINE, value);

		pr_info("wpc activated, set V_INT as PN\n");
	} else if ((charger->irq_wpcin_state == 1) && (wpcin_state == 0)) {
		value.intval = 0;
		psy_do_property("wireless", set, POWER_SUPPLY_PROP_ONLINE, value);

		pr_info("wpc deactivated, set V_INT as PD\n");
	}

	pr_info("w(%d to %d)\n", charger->irq_wpcin_state, wpcin_state);

	charger->irq_wpcin_state = wpcin_state;

	wake_unlock(&charger->wpc_wake_lock);

	pr_info("%s: schedule work done.\n");
}
#endif

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
			__n_is_cable_type_for_wireless(charger->cable_type) &&
			(charger->cable_type != POWER_SUPPLY_TYPE_BATTERY)) {
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

	/*
	if (prev_battery_health == POWER_SUPPLY_HEALTH_UNDERVOLTAGE &&
		value.intval == POWER_SUPPLY_HEALTH_GOOD) {
		sm5705_set_input_current(charger, charger->input_current);
	}
	 */
	pr_info("done.\n");

	return IRQ_HANDLED;
}

#if EN_TOPOFF_IRQ
static irqreturn_t sm5705_chg_topoff_isr(int irq, void *data)
{
	struct sm5705_charger_data *charger = data;

	pr_info("IRQ=%d\n", irq);

	charger->topoff_pending = 0;
	queue_delayed_work(charger->wqueue, &charger->topoff_work, msecs_to_jiffies(500));

	return IRQ_HANDLED;
}
#endif /*EN_TOPOFF_IRQ*/

#if EN_OTGFAIL_IRQ
static irqreturn_t sm5705_chg_otgfail_isr(int irq, void *data)
{
	struct sm5705_charger_data *charger = data;
	unsigned char reg_data;

#ifdef CONFIG_USB_HOST_NOTIFY
	struct otg_notify *o_notify = get_otg_notify();
#endif
	pr_info("IRQ=%d : OTG Failed\n", irq);

	sm5705_read_reg(charger->i2c, SM5705_REG_STATUS3, &reg_data);
	if (reg_data & SM5705_STATUS3_OTGFAIL) {
		pr_info("otg overcurrent limit\n");
#ifdef CONFIG_USB_HOST_NOTIFY
		send_otg_notify(o_notify, NOTIFY_EVENT_OVERCURRENT, 0);
#endif
		psy_chg_set_charge_otg_control(charger, false);
	}

	return IRQ_HANDLED;
}
#endif /*EN_OTGFAIL_IRQ*/

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

#if defined(SM5705_USED_WIRELESS_CHARGER)
static irqreturn_t sm5705_chg_wpcin_pok_isr(int irq, void *data)
{
	struct sm5705_charger_data *charger = data;
	unsigned long delay;

#ifdef CONFIG_SAMSUNG_BATTERY_FACTORY
	delay = msecs_to_jiffies(0);
#else
	if (charger->irq_wpcin_state)
		delay = msecs_to_jiffies(500);
	else
		delay = msecs_to_jiffies(0);
#endif
	pr_info("IRQ=%d delay = %ld\n", irq, delay);

	wake_lock(&charger->wpc_wake_lock);
	queue_delayed_work(charger->wqueue, &charger->wpc_work, delay);

	return IRQ_HANDLED;
}
#endif

/**
 *  SM5705 Charger driver management functions
 **/
 #if defined(SM5705_WATCHDOG_RESET_ACTIVATE)
void sm5705_charger_watchdog_timer_keepalive(void)
{
    if (g_sm5705_charger) {
        sm5705_CHG_set_WDTMR_RST(g_sm5705_charger);
    }
}
#endif

#ifdef CONFIG_OF
static int _parse_sm5705_charger_node_propertys(struct device *dev,
				struct device_node *np, sec_charger_platform_data_t *pdata)
{
	pdata->chg_gpio_en = of_get_named_gpio(np, "battery,chg_gpio_en", 0); //nCHGEN
	if (IS_ERR_VALUE(pdata->chg_gpio_en)) {
		pr_info("can't parsing dt:battery,chg_gpio_en\n");
		return -ENOENT;
	}
	pr_info("battery charge enable pin = %d\n", pdata->chg_gpio_en);

#if defined(CONFIG_CHARGING_VZWCONCEPT)
	pdata->support_slow_charging = true;
#else
	pdata->support_slow_charging = of_property_read_bool(np, "battery,support_slow_charging");
#endif

	return 0;
}

static int _get_of_charging_current_table_max_size(struct device *dev, struct device_node *np)
{
	const unsigned int *propertys;
	int len;

	propertys = of_get_property(np, "battery,input_current_limit", &len);
	if (unlikely(!propertys)) {
		pr_info("can't parsing dt:battery,input_current_limit\n");
	} else {
		pr_info("dt:battery,input_current_limit length=%d\n", len);
	}

	return len / sizeof(unsigned int);
}

static int _parse_battery_node_propertys(struct device *dev, struct device_node *np,
				sec_charger_platform_data_t *pdata)
{
	int i, array_max_size, ret;

	ret = of_property_read_u32(np, "battery,chg_float_voltage", &pdata->chg_float_voltage);
	if (IS_ERR_VALUE(ret)) {
		pr_err("can't parsing dt:battery,chg_float_voltage\n");
	}

#if defined(SM5705_USED_WIRELESS_CHARGER)
	ret = of_property_read_string(np,"battery,wirelss_charger_name",
				(char const **)&pdata->wireless_charger_name);
	if (IS_ERR_VALUE(ret)) {
		pr_err("can't parsing dt:battery,wirelss_charger_name\n");
	}
	pr_info("wireless charger name=%s\n", pdata->wireless_charger_name);

#if defined(CONFIG_WIRELESS_CHARGER_INBATTERY)
	pdata->wpc_det = of_get_named_gpio(np, "battery,wpc_det", 0);
	if (IS_ERR_VALUE(pdata->wpc_det)) {
		pr_err("can't parsing dt:battery,wpc_det\n");
		return -ENOENT;
	}
	pr_info("WPC detect pin = %d\n", pdata->wpc_det);
#endif

#if defined(CONFIG_WIRELESS_CHARGER_HIGH_VOLTAGE)
	ret = of_property_read_u32(np, "battery,wpc_charging_limit_current",
				&pdata->wpc_charging_limit_current);
	if (IS_ERR_VALUE(ret)) {
		pr_err("can't parsing dt:battery,wpc_charging_limit_current\n");
	}

	ret = of_property_read_u32(np, "battery,sleep_mode_limit_current",
				&pdata->sleep_mode_limit_current);
	if (IS_ERR_VALUE(ret)) {
		pr_err("can't parsing dt:battery,sleep_mode_limit_current\n");
	}
#endif

	ret = of_property_read_u32(np, "battery,wireless_cc_cv", &pdata->wireless_cc_cv);
	if (IS_ERR_VALUE(ret)) {
		pr_err("can't parsing dt:battery,wireless_cc_cv\n");
	}
#endif

	ret = of_property_read_u32(np, "battery,full_check_type_2nd", &pdata->full_check_type_2nd);
	if (IS_ERR_VALUE(ret)) {
		pr_err("can't parsing dt:battery,full_check_type_2nd\n");
	}

	array_max_size = _get_of_charging_current_table_max_size(dev, np);
	if (array_max_size == 0) {
		return -ENOENT;
	}
	pr_info("charging current table max size = %d\n", array_max_size);

	pdata->charging_current = kzalloc(sizeof(sec_charging_current_t) * array_max_size, GFP_KERNEL);
	if (unlikely(!pdata->charging_current)) {
		pr_err("fail to allocate memory for charging current table\n");
		return -ENOMEM;
	}

	for(i = 0; i < array_max_size; ++i) {
		of_property_read_u32_index(np, "battery,input_current_limit",
			i, &pdata->charging_current[i].input_current_limit);
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
		if (IS_ERR_VALUE(ret)) {
			return ret;
		}
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
	charger->wc_afc_detect = false;
	charger->input_current = 500;
	charger->topoff_pending = false;
	charger->is_charging = false;
	charger->cable_type = POWER_SUPPLY_TYPE_BATTERY;
	charger->is_mdock = false;
	charger->is_rev2_wa_done = false;

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

#if defined(SM5705_USED_WIRELESS_CHARGER)
	INIT_DELAYED_WORK(&charger->wpc_work, wpc_detect_work);
	INIT_DELAYED_WORK(&charger->wc_afc_work, wc_afc_detect_work);
#endif
	if (charger->pdata->support_slow_charging)
		INIT_DELAYED_WORK(&charger->aicl_work, sm5705_aicl_work);
#if EN_TOPOFF_IRQ
	INIT_DELAYED_WORK(&charger->topoff_work, sm5705_topoff_work);
#endif
	INIT_DELAYED_WORK(&charger->op_mode_switch_work, sm5705_op_mode_switch_work);
#if defined(CONFIG_SEC_FACTORY)
	INIT_DELAYED_WORK(&charger->dis_discharging_force_work, sm5705_dis_discharging_force_work);
#endif
#if defined(SM5705_SW_SOFT_START)
	wake_lock_init(&charger->softstart_wake_lock, WAKE_LOCK_SUSPEND, "charger-softstart");
#endif
	wake_lock_init(&charger->wpc_wake_lock, WAKE_LOCK_SUSPEND, "charger-wpc");
	wake_lock_init(&charger->check_slow_wake_lock, WAKE_LOCK_SUSPEND, "charger-check-slow");
	wake_lock_init(&charger->aicl_wake_lock, WAKE_LOCK_SUSPEND, "charger-aicl");

	/* Get IRQ service routine number */
	charger->irq_wpcin_pok = pdata->irq_base + SM5705_WPCINPOK_IRQ;
	charger->irq_vbus_pok = pdata->irq_base + SM5705_VBUSPOK_IRQ;
	charger->irq_aicl = pdata->irq_base + SM5705_AICL_IRQ;
#if EN_TOPOFF_IRQ
	charger->irq_topoff = pdata->irq_base + SM5705_TOPOFF_IRQ;
#endif
#if EN_OTGFAIL_IRQ
	charger->irq_otgfail = pdata->irq_base + SM5705_OTGFAIL_IRQ;
#endif
	charger->irq_done = pdata->irq_base + SM5705_DONE_IRQ;

	pr_info("init process done..\n");

	return 0;
}

static void sm5705_charger_initialize(struct sm5705_charger_data *charger)
{
	pr_info("charger initial hardware condition process start. (float_voltage=%d)\n",
		charger->pdata->chg_float_voltage);

	/* Auto-Stop configuration for Emergency status */
	sm5705_CHG_set_TOPOFF(charger, 200);
	sm5705_CHG_set_TOPOFF_TMR(charger, SM5705_TOPOFF_TIMER_45m);
	sm5705_CHG_enable_AUTOSTOP(charger, 1);
#if defined(CONFIG_SEC_FACTORY)
	pr_info("queue_delayed_work, dis_discharging_force_work\n");
	cancel_delayed_work(&charger->dis_discharging_force_work);
	queue_delayed_work(charger->wqueue, &charger->dis_discharging_force_work,
			msecs_to_jiffies(20000)); /* delay 20sec */
#endif

	sm5705_CHG_set_BATREG(charger, charger->pdata->chg_float_voltage);

	sm5705_CHG_set_AICLTH(charger, 4500);
	sm5705_CHG_enable_AICL(charger, 1);

	sm5705_CHG_enable_AUTOSET(charger, 1);

	sm5705_CHG_set_BST_IQ3LIMIT(charger, SM5705_CHG_BST_IQ3LIMIT_3_5A);

	sm5705_CHG_set_OVPSEL(charger, 1); /* fix OVPSEL */

	/* SM5705 Charger Reset contdition initialize */
#if defined(SM5705_I2C_RESET_ACTIVATE)
	sm5705_CHG_set_ENI2CRESET(charger, 1);
#endif

#if defined(SM5705_MANUAL_RESET_ACTIVATE)
	sm5705_CHG_set_ENMRSTB(charger, SM5705_MANUAL_RESET_TIMER);
#endif

#if defined(SM5705_WATCHDOG_RESET_ACTIVATE)
	sm5705_CHG_set_WATCHDOG_TMR(charger, SM5705_WATCHDOG_RESET_TIMER);
	sm5705_CHG_set_ENWATCHDOG(charger, 1, 1);
	g_sm5705_charger= charger;
#endif

	sm5705_chg_test_read(charger);

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
		goto err_free;
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

	/* Request IRQ */
#if defined(SM5705_USED_WIRELESS_CHARGER)
	ret = request_threaded_irq(charger->irq_wpcin_pok, NULL,
			sm5705_chg_wpcin_pok_isr, IRQF_TRIGGER_FALLING, "wpc-int", charger);
	if (ret) {
		pr_err("fail to request wpcin IRQ: %d: %d\n", charger->irq_wpcin_pok, ret);
		goto err_power_supply_register_otg;
	}
#endif
	ret = request_threaded_irq(charger->irq_vbus_pok, NULL,
			sm5705_chg_vbus_in_isr, 0, "chgin-irq", charger);
	if (ret < 0) {
		pr_err("fail to request chgin IRQ: %d: %d\n", charger->irq_vbus_pok, ret);
		goto err_power_supply_register_otg;
	}
#if EN_TOPOFF_IRQ
	ret = request_threaded_irq(charger->irq_topoff, NULL,
			sm5705_chg_topoff_isr, 0, "topoff-irq", charger);
	if (ret < 0) {
		pr_err("fail to request topoff IRQ: %d: %d\n", charger->irq_topoff, ret);
		goto err_power_supply_register_otg;
	}
#endif /*EN_TOPOFF_IRQ*/
#if EN_OTGFAIL_IRQ
	ret = request_threaded_irq(charger->irq_otgfail, NULL,
			sm5705_chg_otgfail_isr, 0, "otgfail-irq", charger);
	if (ret < 0) {
		pr_err("fail to request otgfail IRQ: %d: %d\n", charger->irq_otgfail, ret);
		goto err_power_supply_register_otg;
	}
#endif
	ret = request_threaded_irq(charger->irq_done, NULL,
			sm5705_chg_done_isr, 0, "done-irq", charger);
	if (ret < 0) {
		pr_err("fail to request chgin IRQ: %d: %d\n", charger->irq_done, ret);
		goto err_power_supply_register_otg;
	}

	ret = sm5705_chg_create_attrs(charger->psy_chg.dev);
	if (ret){
		pr_err("Failed to create_attrs\n");
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
#ifdef CONFIG_OF
	kfree(pdata->charger_data);
#endif
	mutex_destroy(&charger->charger_mutex);
err_free:
	kfree(charger);

	return ret;
}

static int sm5705_charger_remove(struct platform_device *pdev)
{
	struct sm5705_charger_data *charger = platform_get_drvdata(pdev);

#if defined(SM5705_USED_WIRELESS_CHARGER)
	cancel_delayed_work(&charger->wpc_work);
	cancel_delayed_work(&charger->wc_afc_work);
#endif
	if (charger->pdata->support_slow_charging)
		cancel_delayed_work(&charger->aicl_work);
#if EN_TOPOFF_IRQ
	cancel_delayed_work(&charger->topoff_work);
#endif
	cancel_delayed_work(&charger->op_mode_switch_work);
#if defined(CONFIG_SEC_FACTORY)
	cancel_delayed_work(&charger->dis_discharging_force_work);
#endif
	destroy_workqueue(charger->wqueue);
#if defined(SM5705_USED_WIRELESS_CHARGER)
	free_irq(charger->irq_wpcin_pok, NULL);
#endif
	free_irq(charger->irq_vbus_pok, NULL);
#if EN_TOPOFF_IRQ
	free_irq(charger->irq_topoff, NULL);
#endif
#if EN_OTGFAIL_IRQ
	free_irq(charger->irq_otgfail, NULL);
#endif

	power_supply_unregister(&charger->psy_chg);
	mutex_destroy(&charger->charger_mutex);
	kfree(charger);
	return 0;
}

static void sm5705_charger_shutdown(struct device *dev)
{
	struct sm5705_charger_data *charger = dev_get_drvdata(dev);

#if defined(CONFIG_BATTERY_SWELLING_SELF_DISCHARGING)
	sm5705_charger_en_discharging_force(charger, false);
#endif
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

