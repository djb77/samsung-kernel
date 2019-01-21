/*
 * drivers/battery/sm5708_charger_oper.c
 *
 * SM5708 Charger Operation Mode controller
 *
 * Copyright (C) 2015 Siliconmitus Technology Corp.
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 */

#include <linux/kernel.h>
#include "include/charger/sm5708_charger.h"
#include "include/charger/sm5708_charger_oper.h"

enum {
	BST_OUT_4000mV              = 0x0,
	BST_OUT_4100mV              = 0x1,
	BST_OUT_4200mV              = 0x2,
	BST_OUT_4300mV              = 0x3,
	BST_OUT_4400mV              = 0x4,
	BST_OUT_4500mV              = 0x5,
	BST_OUT_4600mV              = 0x6,
	BST_OUT_4700mV              = 0x7,
	BST_OUT_4800mV              = 0x8,
	BST_OUT_4900mV              = 0x9,
	BST_OUT_5000mV              = 0xA,
	BST_OUT_5100mV              = 0xB,
};

enum {
	OTG_CURRENT_500mA           = 0x0,
	OTG_CURRENT_700mA           = 0x1,
	OTG_CURRENT_900mA           = 0x2,
	OTG_CURRENT_1500mA          = 0x3,
};

struct sm5708_charger_oper_table_info {
	unsigned char status;
	unsigned char oper_mode;
	unsigned char BST_OUT;
	unsigned char OTG_CURRENT;
};

struct sm5708_charger_oper_info {
	struct i2c_client *i2c;

	bool suspend_mode;
	int max_table_num;
	struct sm5708_charger_oper_table_info current_table;
};
static struct sm5708_charger_oper_info oper_info;

/**
 *  (VBUS in/out) (FLASH on/off) (TORCH on/off) (OTG cable in/out) (Power Sharing cable in/out)
 **/
static struct sm5708_charger_oper_table_info sm5708_charger_operation_mode_table[] = {
	/* Charger mode : Charging ON */
	{ make_OP_STATUS(0, 0, 0, 0, 0), SM5708_CHARGER_OP_MODE_CHG_ON, BST_OUT_4500mV, OTG_CURRENT_500mA},
	{ make_OP_STATUS(1, 0, 0, 0, 0), SM5708_CHARGER_OP_MODE_CHG_ON, BST_OUT_4500mV, OTG_CURRENT_500mA},
	{ make_OP_STATUS(1, 0, 1, 0, 0), SM5708_CHARGER_OP_MODE_CHG_ON, BST_OUT_4500mV, OTG_CURRENT_500mA},

	/* Charger mode : Flash Boost */
	{ make_OP_STATUS(0, 1, 0, 0, 0), SM5708_CHARGER_OP_MODE_FLASH_BOOST, BST_OUT_4500mV, OTG_CURRENT_500mA},
	{ make_OP_STATUS(0, 1, 0, 1, 0), SM5708_CHARGER_OP_MODE_FLASH_BOOST, BST_OUT_4500mV, OTG_CURRENT_900mA},
	{ make_OP_STATUS(0, 1, 0, 0, 1), SM5708_CHARGER_OP_MODE_FLASH_BOOST, BST_OUT_4500mV, OTG_CURRENT_900mA},
	{ make_OP_STATUS(1, 1, 0, 0, 0), SM5708_CHARGER_OP_MODE_FLASH_BOOST, BST_OUT_4500mV, OTG_CURRENT_500mA},
	{ make_OP_STATUS(0, 0, 1, 0, 0), SM5708_CHARGER_OP_MODE_FLASH_BOOST, BST_OUT_4500mV, OTG_CURRENT_500mA},

	/* Charger mode : USB OTG */
	{ make_OP_STATUS(0, 0, 1, 1, 0), SM5708_CHARGER_OP_MODE_USB_OTG, BST_OUT_5100mV, OTG_CURRENT_900mA},
	{ make_OP_STATUS(0, 0, 1, 0, 1), SM5708_CHARGER_OP_MODE_USB_OTG, BST_OUT_5100mV, OTG_CURRENT_900mA},
	{ make_OP_STATUS(0, 0, 0, 1, 0), SM5708_CHARGER_OP_MODE_USB_OTG, BST_OUT_5100mV, OTG_CURRENT_900mA},
	{ make_OP_STATUS(0, 0, 0, 0, 1), SM5708_CHARGER_OP_MODE_USB_OTG, BST_OUT_5100mV, OTG_CURRENT_900mA},
};

/**
 * SM5708 Charger operation mode controller relative I2C setup
 */

static int sm5708_charger_oper_set_mode(struct i2c_client *i2c, unsigned char mode)
{
	return sm5708_update_reg(i2c, SM5708_REG_CNTL, (mode << 0), (0x07 << 0));
}

static int sm5708_charger_oper_set_BSTOUT(struct i2c_client *i2c, unsigned char BSTOUT)
{
	return sm5708_update_reg(i2c, SM5708_REG_FLEDCNTL6, (BSTOUT << 0), (0x0F << 0));
}

static int sm5708_charger_oper_set_OTG_CURRENT(struct i2c_client *i2c, unsigned char OTG_CURRENT)
{
	return sm5708_update_reg(i2c, SM5708_REG_CHGCNTL5, (OTG_CURRENT << 2), (0x03 << 2));
}

/**
 * SM5708 Charger operation mode controller API functions.
 */

static inline unsigned char _update_status(int event_type, bool enable)
{
	if (event_type > SM5708_CHARGER_OP_EVENT_VBUS) {
		return oper_info.current_table.status;
	}

	if (enable) {
		return (oper_info.current_table.status | (1 << event_type));
	} else {
		return (oper_info.current_table.status & ~(1 << event_type));
	}
}

static inline void sm5708_charger_oper_change_state(unsigned char new_status)
{
	int i;

	for (i = 0; i < oper_info.max_table_num; ++i) {
		if (new_status == sm5708_charger_operation_mode_table[i].status) {
			break;
		}
	}
	if (i == oper_info.max_table_num) {
		pr_err("sm5708-charger: %s: can't find matched Charger Operation Mode Table (status = 0x%x)\n", __func__, new_status);
		return;
	}

	if (oper_info.suspend_mode) {
		pr_info("sm5708-charger: %s: skip setting by suspend mode(MODE: %d)\n",
			__func__, oper_info.current_table.oper_mode);
		goto skip_oper_change_state;
	}

	if (sm5708_charger_operation_mode_table[i].BST_OUT != oper_info.current_table.BST_OUT) {
		sm5708_charger_oper_set_BSTOUT(oper_info.i2c, sm5708_charger_operation_mode_table[i].BST_OUT);
		oper_info.current_table.BST_OUT = sm5708_charger_operation_mode_table[i].BST_OUT;
	}
	if (sm5708_charger_operation_mode_table[i].OTG_CURRENT != oper_info.current_table.OTG_CURRENT) {
		sm5708_charger_oper_set_OTG_CURRENT(oper_info.i2c, sm5708_charger_operation_mode_table[i].OTG_CURRENT);
		oper_info.current_table.OTG_CURRENT = sm5708_charger_operation_mode_table[i].OTG_CURRENT;
	}

	/* USB_OTG to CHG_ON work-around for BAT_REG stabilize */
	if (oper_info.current_table.oper_mode == SM5708_CHARGER_OP_MODE_USB_OTG && \
		sm5708_charger_operation_mode_table[i].oper_mode == SM5708_CHARGER_OP_MODE_CHG_ON) {
		pr_info("sm5708-charger: %s: trans op_mode:suspend for BAT_REG stabilize (time=100ms)\n", __func__);
		sm5708_charger_oper_set_mode(oper_info.i2c, SM5708_CHARGER_OP_MODE_SUSPEND);
		msleep(100);
	}


	if (sm5708_charger_operation_mode_table[i].oper_mode != oper_info.current_table.oper_mode) {
		sm5708_charger_oper_set_mode(oper_info.i2c, sm5708_charger_operation_mode_table[i].oper_mode);
		oper_info.current_table.oper_mode = sm5708_charger_operation_mode_table[i].oper_mode;
	}
skip_oper_change_state:
	oper_info.current_table.status = new_status;

	pr_info("sm5708-charger: %s: New table[%d] info (STATUS: 0x%x, MODE: %d, BST_OUT: 0x%x, OTG_CURRENT: 0x%x\n", \
			__func__, i, oper_info.current_table.status, oper_info.current_table.oper_mode, oper_info.current_table.BST_OUT, oper_info.current_table.OTG_CURRENT);
}

int sm5708_charger_oper_push_event(int event_type, bool enable)
{
	unsigned char new_status;

	if (oper_info.i2c == NULL) {
		pr_err("sm5708-charger: %s: required sm5708 charger operation table initialize\n", __func__);
		return -ENOENT;
	}

	pr_info("sm5708-charger: %s: event_type=%d, enable=%d\n", __func__, event_type, enable);

	if (event_type == SM5708_CHARGER_OP_EVENT_SUSPEND_MODE) {
		oper_info.suspend_mode = enable;
		if (oper_info.suspend_mode) {
			oper_info.current_table.oper_mode = SM5708_CHARGER_OP_MODE_SUSPEND;
			sm5708_charger_oper_set_mode(oper_info.i2c, SM5708_CHARGER_OP_MODE_SUSPEND);
		} else if (oper_info.current_table.oper_mode == SM5708_CHARGER_OP_MODE_SUSPEND) {
			sm5708_charger_oper_change_state(oper_info.current_table.status);
		}
		goto out;
	}

	new_status = _update_status(event_type, enable);
	if (new_status == oper_info.current_table.status) {
		goto out;
	}

	sm5708_charger_oper_change_state(new_status);

out:
	return 0;
}
EXPORT_SYMBOL(sm5708_charger_oper_push_event);

int sm5708_charger_oper_table_init(struct i2c_client *i2c)
{
	if (i2c == NULL) {
		pr_err("sm5708-charger: %s: invalid i2c client handler=n", __func__);
		return -EINVAL;
	}
	oper_info.i2c = i2c;

	/* set default operation mode condition */
	oper_info.suspend_mode = false;
	oper_info.max_table_num = ARRAY_SIZE(sm5708_charger_operation_mode_table);
	oper_info.current_table.status = make_OP_STATUS(0, 0, 0, 0, 0);
	oper_info.current_table.oper_mode = SM5708_CHARGER_OP_MODE_CHG_ON;
	oper_info.current_table.BST_OUT = BST_OUT_4500mV;
	oper_info.current_table.OTG_CURRENT = OTG_CURRENT_500mA;

	sm5708_charger_oper_set_mode(oper_info.i2c, oper_info.current_table.oper_mode);
	sm5708_charger_oper_set_BSTOUT(oper_info.i2c, oper_info.current_table.BST_OUT);
	sm5708_charger_oper_set_OTG_CURRENT(oper_info.i2c, oper_info.current_table.OTG_CURRENT);

	pr_info("sm5708-charger: %s: current table info (STATUS: 0x%x, MODE: %d, BST_OUT: 0x%x, OTG_CURRENT: 0x%x\n", \
			__func__, oper_info.current_table.status, oper_info.current_table.oper_mode, oper_info.current_table.BST_OUT, oper_info.current_table.OTG_CURRENT);

	return 0;
}
EXPORT_SYMBOL(sm5708_charger_oper_table_init);

int sm5708_charger_oper_get_current_status(void)
{
	return oper_info.current_table.status;
}
EXPORT_SYMBOL(sm5708_charger_oper_get_current_status);

int sm5708_charger_oper_get_current_op_mode(void)
{
	return oper_info.current_table.oper_mode;
}
EXPORT_SYMBOL(sm5708_charger_oper_get_current_op_mode);
