/*
 * Driver for keys on GPIO lines capable of generating interrupts.
 *
 * Copyright 2005 Phil Blundell
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>

#include <linux/init.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/pm.h>
#include <linux/sysctl.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/gpio.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>

#ifndef USE_OPEN_CLOSE
#define USE_OPEN_CLOSE
#undef CONFIG_HAS_EARLYSUSPEND
#undef CONFIG_PM
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <linux/io.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/firmware.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/sec_sysfs.h>
#include <linux/sec_batt.h>

#ifdef CONFIG_INPUT_BOOSTER
#include <linux/input/input_booster.h>
#endif

#include "cypress_touchkey.h"

#ifdef  TK_HAS_FIRMWARE_UPDATE
#if defined(CONFIG_KEYBOARD_CYPRESS_TOUCH_MBR31X5)
#include "cy8cmbr_swd.h"
#else
#include "issp_extern.h"
#endif
u8 *tk_fw_name = FW_PATH;
u8 module_divider[] = {0, 0xff};
#endif

static int touchkey_keycode[] = { 0,
	KEY_RECENT, KEY_BACK,
};
static const int touchkey_count = ARRAY_SIZE(touchkey_keycode);

int touchkey_mode_change(struct touchkey_i2c *tkey_i2c, int cmd);

char *str_states[] = {"on_irq", "off_irq", "on_i2c", "off_i2c"};
enum {
	I_STATE_ON_IRQ = 0,
	I_STATE_OFF_IRQ,
	I_STATE_ON_I2C,
	I_STATE_OFF_I2C,
};

static void cypress_config_gpio_i2c(struct touchkey_i2c *tkey_i2c, int onoff);
static int touchkey_i2c_update(struct touchkey_i2c *tkey_i2c);

static bool touchkey_probe;

static const struct i2c_device_id sec_touchkey_id[] = {
	{"sec_touchkey", 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, sec_touchkey_id);
extern int get_touchkey_firmware(char *version);
static int touchkey_led_status;
static int touchled_cmd_reversed;

#ifdef LED_LDO_WITH_REGULATOR
static void change_touch_key_led_voltage(struct device *dev, int vol_mv)
{
	struct regulator *tled_regulator;

	tled_regulator = regulator_get(NULL, TK_LED_REGULATOR_NAME);
	if (IS_ERR(tled_regulator)) {
		tk_debug_err(true, dev, "%s: failed to get resource %s\n", __func__,
		       "touchkey_led");
		return;
	}
	regulator_set_voltage(tled_regulator, vol_mv * 1000, vol_mv * 1000);
	regulator_put(tled_regulator);
}

static ssize_t brightness_control(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t size)
{
	int data;

	if (sscanf(buf, "%d\n", &data) == 1) {
		tk_debug_err(true, dev, "%s: %d\n", __func__, data);
		change_touch_key_led_voltage(dev, data);
	} else {
		tk_debug_err(true, dev, "%s Error\n", __func__);
	}

	return size;
}
#endif

static int i2c_touchkey_read(struct i2c_client *client,
		u8 reg, u8 *val, unsigned int len)
{
	int ret = 0;
	int retry = 3;
	struct touchkey_i2c *tkey_i2c = i2c_get_clientdata(client);

	mutex_lock(&tkey_i2c->i2c_lock);

	if ((client == NULL) || !(tkey_i2c->enabled)) {
		tk_debug_err(true, &client->dev, "Touchkey is not enabled. %d\n",
		       __LINE__);
		ret = -ENODEV;
		goto out_i2c_read;
	}

	while (retry--) {
		ret = i2c_smbus_read_i2c_block_data(client,
				reg, len, val);
		if (ret < 0) {
			dev_err(&client->dev, "%s:error(%d)\n", __func__, ret);
			usleep_range(10000, 10000);
			continue;
		}
		break;
	}

out_i2c_read:
	mutex_unlock(&tkey_i2c->i2c_lock);
	return ret;
}

static int i2c_touchkey_write(struct i2c_client *client,
		u8 *val, unsigned int len)
{
	int ret = 0;
	int retry = 3;
	struct touchkey_i2c *tkey_i2c = i2c_get_clientdata(client);

	mutex_lock(&tkey_i2c->i2c_lock);

	if ((client == NULL) || !(tkey_i2c->enabled)) {
		tk_debug_err(true, &client->dev, "Touchkey is not enabled. %d\n",
		       __LINE__);
		ret = -ENODEV;
		goto out_i2c_write;
	}

	while (retry--) {
		ret = i2c_smbus_write_i2c_block_data(client,
				BASE_REG, len, val);
		if (ret < 0) {
			dev_err(&client->dev, "%s:error(%d)\n", __func__, ret);
			usleep_range(10000, 10000);
			continue;
		}
		break;
	}

out_i2c_write:
	mutex_unlock(&tkey_i2c->i2c_lock);
	return ret;
}

static int touchkey_i2c_check(struct touchkey_i2c *tkey_i2c)
{
	char data[4] = { 0, };
	int ret = 0;
	int retry  = 3;

	while (retry--) {
		ret = i2c_touchkey_read(tkey_i2c->client, KEYCODE_REG, data, 4);
		if (ret < 0) {
			tk_debug_err(true, &tkey_i2c->client->dev, "Failed to read Module version retry %d\n", retry);
			if (retry == 1) {
				tkey_i2c->fw_ver_ic = 0;
				tkey_i2c->md_ver_ic = 0;
				tkey_i2c->device_ver = 0;
				return ret;
			}
			msleep(30);
			continue;
		}
		break;
	}

	tkey_i2c->fw_ver_ic = data[1];
	tkey_i2c->md_ver_ic = data[2];
	tkey_i2c->device_ver = data[3];

#ifdef CRC_CHECK_INTERNAL
	ret = i2c_touchkey_read(tkey_i2c->client, 0x30, data, 2);
	if (ret < 0) {
		tk_debug_err(true, &tkey_i2c->client->dev, "Failed to read crc\n");
		tkey_i2c->crc = 0;
		return ret;
	}

	tkey_i2c->crc = ((0xFF & data[1]) << 8) | data[0];
#endif

	tk_debug_dbg(true, &tkey_i2c->client->dev, "%s: ic_fw_ver = 0x%02x, module_ver = 0x%02x, CY device = 0x%02x\n",
		__func__, tkey_i2c->fw_ver_ic, tkey_i2c->md_ver_ic, tkey_i2c->device_ver);

	return ret;
}

#if defined(TK_INFORM_CHARGER)
static int touchkey_ta_setting(struct touchkey_i2c *tkey_i2c)
{
	u8 data[6] = { 0, };
	int count = 0;
	int ret = 0;
	unsigned short retry = 0;

    if (tkey_i2c->charging_mode) {
        data[1] = TK_BIT_CMD_TA_ON;
		data[2] = TK_BIT_WRITE_CONFIRM;
    } else {
        data[1] = TK_BIT_CMD_REGULAR;
		data[2] = TK_BIT_WRITE_CONFIRM;
    }

    count = i2c_touchkey_write(tkey_i2c->client, data, 3);

	while (retry < 3) {
		msleep(30);

		ret = i2c_touchkey_read(tkey_i2c->client, TK_STATUS_FLAG, data, 1);

		if (tkey_i2c->charging_mode) {
			if (data[0] & TK_BIT_TA_ON) {
				tk_debug_dbg(true, &tkey_i2c->client->dev, "%s: TA mode is Enabled\n", __func__);
				break;
			} else {
				tk_debug_err(true, &tkey_i2c->client->dev, "%s: Error to enable TA mode, retry %d\n",
					__func__, retry);
			}
		} else {
			if (!(data[0] & TK_BIT_TA_ON)) {
				tk_debug_dbg(true, &tkey_i2c->client->dev, "%s: TA mode is Disabled\n", __func__);
				break;
			} else {
				tk_debug_err(true, &tkey_i2c->client->dev, "%s: Error to disable TA mode, retry %d\n",
					__func__, retry);
			}
		}
		retry = retry + 1;
	}

	if (retry == 3)
		tk_debug_err(true, &tkey_i2c->client->dev, "%s: Failed to set the TA mode\n", __func__);

	return count;

}

static void touchkey_ta_cb(struct touchkey_callbacks *cb, bool ta_status)
{
	struct touchkey_i2c *tkey_i2c =
			container_of(cb, struct touchkey_i2c, callbacks);
	struct i2c_client *client = tkey_i2c->client;

	tkey_i2c->charging_mode = ta_status;

	if (tkey_i2c->enabled)
		touchkey_ta_setting(tkey_i2c);
}
#endif

#ifdef TKEY_1MM_MODE
void touchkey_1mm_mode(struct touchkey_i2c *tkey_i2c, int value)
{
	u8 data[6] = { 0, };
	int ret = 0;
	u8 retry = 0;
	u8 stylus_status;

	if (!(tkey_i2c->enabled)) {
		tk_debug_info(true, &tkey_i2c->client->dev, "%s : Touchkey is not enabled.\n",
				__func__);
		return ;
	}

	if (value == 1) {
	/* Send 1mm mode Command */
		data[1] = TK_BIT_CMD_1mmSTYLUS;
		data[2] = TK_BIT_WRITE_CONFIRM;
	} else {
		data[1] = TK_BIT_CMD_REGULAR;
		data[2] = TK_BIT_WRITE_CONFIRM;
	}

	ret = i2c_touchkey_write(tkey_i2c->client, data, 3);
	if (ret < 0) {
		tk_debug_info(true, &tkey_i2c->client->dev, "%s: Failed to write 1mm mode command.\n",
				__func__);
		return;
	}

	while (retry < 3) {
		msleep(30);
		/* Check status flag mode */
		ret = i2c_touchkey_read(tkey_i2c->client, TK_STATUS_FLAG, data, 1);
		if (ret < 0) {
			tk_debug_info(true, &tkey_i2c->client->dev, "%s: Failed to check status flag.\n",
					__func__);
			return;
		}
		stylus_status = !!(data[0] & TK_BIT_1mmSTYLUS);

		tk_debug_err(true, &tkey_i2c->client->dev,
			"data[0]=%x, 1mm: %x, flip: %x, glove: %x, ta: %x\n\n",
			data[0], data[0] & 0x20, data[0] & 0x10, data[0] & 0x08, data[0] & 0x04);

		if (value == stylus_status) {
			tk_debug_info(true, &tkey_i2c->client->dev,
				"%s: 1MM mode is %s\n", __func__, stylus_status ? "enabled" : "disabled");
				break;
			} else {
			tk_debug_err(true, &tkey_i2c->client->dev,
				"%s: Error to set 1MM mode, val %d, 1mm bit %d, retry %d\n",
				__func__, value, stylus_status, retry);
		}
		retry = retry + 1;
	}

	if (retry == 3)
		tk_debug_err(true, &tkey_i2c->client->dev, "[Touchkey] 1mm mode failed\n");

	return;
}
#endif

static int touchkey_enable_status_update(struct touchkey_i2c *tkey_i2c)
{
	u8 data[4] = { 0, };
	int ret = 0;

    ret = i2c_touchkey_read(tkey_i2c->client, BASE_REG, data, 4);
    if (ret < 0) {
        tk_debug_err(true, &tkey_i2c->client->dev, "%s: Failed to read Keycode_reg\n",
            __func__);
        return ret;
    }

    tk_debug_dbg(true, &tkey_i2c->client->dev,
            "data[0]=%x, data[1]=%x\n",
            data[0], data[1]);

	data[1] = TK_BIT_CMD_INSPECTION;
	data[2] = TK_BIT_WRITE_CONFIRM;

    ret = i2c_touchkey_write(tkey_i2c->client, data, 3);
	if (ret < 0) {
		tk_debug_err(true, &tkey_i2c->client->dev, "%s, err(%d)\n", __func__, ret);
		tkey_i2c->status_update = false;
		return ret;
	}

	tkey_i2c->status_update = true;
	tk_debug_dbg(true, &tkey_i2c->client->dev, "%s\n", __func__);

	msleep(20);

	return 0;
}

enum {
	TK_CMD_READ_THRESHOLD = 0,
	TK_CMD_READ_DIFF,
	TK_CMD_READ_RAW,
	TK_CMD_READ_IDAC,
	TK_CMD_COMP_IDAC,
	TK_CMD_BASELINE,
};

const u8 fac_reg_index[] = {
	TK_THRESHOLD,
	TK_DIFF_DATA,
	TK_RAW_DATA,
	TK_IDAC,
	TK_COMP_IDAC,
	TK_BASELINE_DATA,
};

struct FAC_CMD {
	u8 cmd;
	u8 opt1; // 0, 1, 2, 3
	u16 result;
};

static u8 touchkey_get_read_size(u8 cmd)
{
	switch (cmd) {
	case TK_CMD_READ_RAW:
	case TK_CMD_READ_DIFF:
	case TK_BASELINE_DATA:
		return 2;
	case TK_CMD_READ_IDAC:
	case TK_CMD_COMP_IDAC:
	case TK_CMD_READ_THRESHOLD:
		return 1;
		break;
	default:
		break;
	}
	return 0;
}

static int touchkey_fac_read_data(struct device *dev,
		char *buf, struct FAC_CMD *cmd)
{
	struct touchkey_i2c *tkey_i2c = dev_get_drvdata(dev);
	int ret;
	u8 size;
	u8 base_index;
	u8 data[26] = { 0, };
	int i, j = 0;
	u16 max_val = 0;

	if (unlikely(!tkey_i2c->status_update)) {
		ret = touchkey_enable_status_update(tkey_i2c);
		if (ret < 0)
			goto out_fac_read_data;
	}

	size = touchkey_get_read_size(cmd->cmd);
	if (size == 0) {
		tk_debug_err(true, &tkey_i2c->client->dev, "wrong size %d\n", size);
		goto out_fac_read_data;
	}

	if (cmd->opt1 > 4) {
		tk_debug_err(true, &tkey_i2c->client->dev, "wrong opt1 %d\n", cmd->opt1);
		goto out_fac_read_data;
	}

	base_index = fac_reg_index[cmd->cmd] + size * cmd->opt1;
	if (base_index > 46) {
		tk_debug_err(true, &tkey_i2c->client->dev, "wrong index %d, cmd %d, size %d, opt1 %d\n",
			base_index, cmd->cmd, size, cmd->opt1);
		goto out_fac_read_data;
	}

	ret = i2c_touchkey_read(tkey_i2c->client, base_index, data, size);
	if (ret <  0) {
		tk_debug_err(true, &tkey_i2c->client->dev, "i2c read failed\n");
		goto out_fac_read_data;
	}

	/* make value */
	cmd->result = 0;
	for (i = size - 1; i >= 0; --i) {
		cmd->result = cmd->result | (data[j++] << (8 * i));
		max_val |= 0xff << (8 * i);
	}

	/* garbage check */
	if (unlikely(cmd->result == max_val)) {
		tk_debug_err(true, &tkey_i2c->client->dev, "cmd %d opt1 %d, max value\n",
				cmd->cmd, cmd->opt1);
		cmd->result = 0;
	}

 out_fac_read_data:
	return sprintf(buf, "%d\n", cmd->result);
}

static ssize_t touchkey_raw_data0_show(struct device *dev,
				       struct device_attribute *attr, char *buf)
{
	struct FAC_CMD cmd = {TK_CMD_READ_RAW, 0, 0}; /* recent outer*/
	return touchkey_fac_read_data(dev, buf, &cmd);
}

static ssize_t touchkey_raw_data1_show(struct device *dev,
				       struct device_attribute *attr, char *buf)
{
	struct FAC_CMD cmd = {TK_CMD_READ_RAW, 1, 0}; /* recent inner*/
	return touchkey_fac_read_data(dev, buf, &cmd);
}

#if defined(TKEY_1MM_MODE)
static ssize_t touchkey_raw_data2_show(struct device *dev,
				       struct device_attribute *attr, char *buf)
{
	struct FAC_CMD cmd = {TK_CMD_READ_RAW, 2, 0}; /* back outer*/
	return touchkey_fac_read_data(dev, buf, &cmd);
}

static ssize_t touchkey_raw_data3_show(struct device *dev,
				       struct device_attribute *attr, char *buf)
{
	struct FAC_CMD cmd = {TK_CMD_READ_RAW, 3, 0}; /* back inner*/
	return touchkey_fac_read_data(dev, buf, &cmd);
}
#endif

static ssize_t touchkey_diff_data0_show(struct device *dev,
				       struct device_attribute *attr, char *buf)
{
	struct FAC_CMD cmd = {TK_CMD_READ_DIFF, 0, 0}; /* recent outer*/
	return touchkey_fac_read_data(dev, buf, &cmd);
}

static ssize_t touchkey_diff_data1_show(struct device *dev,
				       struct device_attribute *attr, char *buf)
{
	struct FAC_CMD cmd = {TK_CMD_READ_DIFF, 1, 0}; /* recent inner*/
	return touchkey_fac_read_data(dev, buf, &cmd);
}

#if defined(TKEY_1MM_MODE)
static ssize_t touchkey_diff_data2_show(struct device *dev,
				       struct device_attribute *attr, char *buf)
{
	struct FAC_CMD cmd = {TK_CMD_READ_DIFF, 2, 0}; /* back outer*/
	return touchkey_fac_read_data(dev, buf, &cmd);
}

static ssize_t touchkey_diff_data3_show(struct device *dev,
				       struct device_attribute *attr, char *buf)
{
	struct FAC_CMD cmd = {TK_CMD_READ_DIFF, 3, 0}; /* back inner*/
	return touchkey_fac_read_data(dev, buf, &cmd);
}
#endif

static ssize_t touchkey_idac0_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct FAC_CMD cmd = {TK_CMD_READ_IDAC, 0, 0}; /* recent outer*/
	return touchkey_fac_read_data(dev, buf, &cmd);
}

static ssize_t touchkey_idac1_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct FAC_CMD cmd = {TK_CMD_READ_IDAC, 1, 0}; /* recent inner*/
	return touchkey_fac_read_data(dev, buf, &cmd);
}

#if defined(TKEY_1MM_MODE)
static ssize_t touchkey_idac2_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct FAC_CMD cmd = {TK_CMD_READ_IDAC, 2, 0}; /* back outer*/
	return touchkey_fac_read_data(dev, buf, &cmd);
}

static ssize_t touchkey_idac3_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct FAC_CMD cmd = {TK_CMD_READ_IDAC, 3, 0}; /* back inner*/
	return touchkey_fac_read_data(dev, buf, &cmd);
}

static ssize_t touchkey_comp_idac0_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct FAC_CMD cmd = {TK_CMD_COMP_IDAC, 0, 0}; /* recent outer*/
	return touchkey_fac_read_data(dev, buf, &cmd);
}

static ssize_t touchkey_comp_idac1_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct FAC_CMD cmd = {TK_CMD_COMP_IDAC, 1, 0}; /* recent inner*/
	return touchkey_fac_read_data(dev, buf, &cmd);
}

static ssize_t touchkey_comp_idac2_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct FAC_CMD cmd = {TK_CMD_COMP_IDAC, 2, 0}; /* back outer*/
	return touchkey_fac_read_data(dev, buf, &cmd);
}

static ssize_t touchkey_comp_idac3_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct FAC_CMD cmd = {TK_CMD_COMP_IDAC, 3, 0}; /* back inner*/
	return touchkey_fac_read_data(dev, buf, &cmd);
}

static ssize_t touchkey_baseline_data0_show(struct device *dev,
				       struct device_attribute *attr, char *buf)
{
	struct FAC_CMD cmd = {TK_CMD_BASELINE, 0, 0}; /* recent outer*/
	return touchkey_fac_read_data(dev, buf, &cmd);
}

static ssize_t touchkey_baseline_data1_show(struct device *dev,
				       struct device_attribute *attr, char *buf)
{
	struct FAC_CMD cmd = {TK_CMD_BASELINE, 1, 0};  /* recent inner*/
	return touchkey_fac_read_data(dev, buf, &cmd);
}


static ssize_t touchkey_baseline_data2_show(struct device *dev,
				       struct device_attribute *attr, char *buf)
{
	struct FAC_CMD cmd = {TK_CMD_BASELINE, 2, 0}; /* back outer*/
	return touchkey_fac_read_data(dev, buf, &cmd);

}

static ssize_t touchkey_baseline_data3_show(struct device *dev,
				       struct device_attribute *attr, char *buf)
{
	struct FAC_CMD cmd = {TK_CMD_BASELINE, 3, 0}; /* back inner*/
	return touchkey_fac_read_data(dev, buf, &cmd);
}
#endif

static ssize_t touchkey_threshold0_show(struct device *dev,
				       struct device_attribute *attr, char *buf)
{
	struct FAC_CMD cmd = {TK_CMD_READ_THRESHOLD, 0, 0};
	return touchkey_fac_read_data(dev, buf, &cmd);
}

#if defined(TKEY_1MM_MODE)
static ssize_t touchkey_threshold1_show(struct device *dev,
				       struct device_attribute *attr, char *buf)
{
	struct FAC_CMD cmd = {TK_CMD_READ_THRESHOLD, 1, 0};
	return touchkey_fac_read_data(dev, buf, &cmd);
}

static ssize_t touchkey_threshold2_show(struct device *dev,
				       struct device_attribute *attr, char *buf)
{
	struct FAC_CMD cmd = {TK_CMD_READ_THRESHOLD, 2, 0};
	return touchkey_fac_read_data(dev, buf, &cmd);
}

static ssize_t touchkey_threshold3_show(struct device *dev,
				       struct device_attribute *attr, char *buf)
{
	struct FAC_CMD cmd = {TK_CMD_READ_THRESHOLD, 3, 0};
	return touchkey_fac_read_data(dev, buf, &cmd);
}
#endif
#if defined(TK_HAS_FIRMWARE_UPDATE)
#if 0
static int get_module_ver(void)
{
	if (likely(system_rev >= TKEY_MODULE07_HWID))
		return 0x09;
	else
		return 0x04;
}

static void touchkey_init_fw_name(struct touchkey_i2c *tkey_i2c)
{
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
	/* check module ver of ic */
/*	if (tkey_i2c->md_ver_ic == 0) {
		tkey_i2c->md_ver_ic = get_module_ver();
		printk(KERN_DEBUG"touchkey:failed to read module_ver. seperate by hwid(ver %x)\n", tkey_i2c->md_ver_ic);
	}
*/
	/*set fw by module_ver*/
	switch(tkey_i2c->md_ver_ic) {
	case 0x08:
	case 0x09:
	case 0x0A:
		break;
	case 0x07:
		tk_fw_name = "cypress/cypress_ha_m07.fw";
		fw_ver_file = 0xA;
		md_ver_file = 0x7;
		break;
	default:
		printk(KERN_DEBUG"touchkey:%s, unknown module ver %x\n", __func__, tkey_i2c->md_ver_ic);
		return ;
	}
#endif
}
#endif
#endif
/* To check firmware compatibility */
int get_module_class(u8 ver)
{
	static int size = ARRAY_SIZE(module_divider);
	int i;

	if (size == 2)
		return 0;

	for (i = size - 1; i > 0; --i) {
		if (ver < module_divider[i] &&
			ver >= module_divider[i-1])
			return i;
	}

	return 0;
}

bool is_same_module_class(struct touchkey_i2c *tkey_i2c)
{
	int class_ic, class_file;

	if (tkey_i2c->src_md_ver == tkey_i2c->md_ver_ic)
		return true;

	class_file = get_module_class(tkey_i2c->src_md_ver);
	class_ic = get_module_class(tkey_i2c->md_ver_ic);

	tk_debug_dbg(true, &tkey_i2c->client->dev, "module class, IC %d, File %d\n",
				class_ic, class_file);

	if (class_file == class_ic)
		return true;

	return false;
}

int tkey_load_fw_built_in(struct touchkey_i2c *tkey_i2c)
{
	int retry = 3;
	int ret;

	while (retry--) {
		ret =
			request_firmware(&tkey_i2c->firm_data, tkey_i2c->pdata->fw_path,
			&tkey_i2c->client->dev);
		if (ret < 0) {
			tk_debug_err(true, &tkey_i2c->client->dev,
				"Unable to open firmware. ret %d retry %d\n",
				ret, retry);
			continue;
		}
		break;
	}
	tk_debug_info(true, &tkey_i2c->client->dev, "touchkey:fw path loaded %s\n", tkey_i2c->pdata->fw_path );
	tkey_i2c->fw_img = (struct fw_image *)tkey_i2c->firm_data->data;
	tkey_i2c->src_fw_ver = tkey_i2c->fw_img->first_fw_ver;
	tkey_i2c->src_md_ver = tkey_i2c->fw_img->second_fw_ver;

	return ret;
}

int tkey_load_fw_ffu(struct touchkey_i2c *tkey_i2c)
{
	int retry = 3;
	int ret;

	while (retry--) {
		ret =
			request_firmware(&tkey_i2c->firm_data, TKEY_FW_FFU_PATH,
			&tkey_i2c->client->dev);
		if (ret < 0) {
			tk_debug_err(true, &tkey_i2c->client->dev,
				"Unable to open firmware. ret %d retry %d\n",
				ret, retry);
			continue;
		}
		break;
	}
	tk_debug_info(true, &tkey_i2c->client->dev, "touchkey:fw path loaded %s\n", tkey_i2c->pdata->fw_path);
	tkey_i2c->fw_img = (struct fw_image *)tkey_i2c->firm_data->data;
	tkey_i2c->src_fw_ver = tkey_i2c->fw_img->first_fw_ver;
	tkey_i2c->src_md_ver = tkey_i2c->fw_img->second_fw_ver;

	return ret;
}

int tkey_load_fw_sdcard(struct touchkey_i2c *tkey_i2c)
{
	struct file *fp;
	mm_segment_t old_fs;
	long fsize, nread;
	int ret = 0;
/*	unsigned int nSize;*/

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	fp = filp_open(TKEY_FW_PATH, O_RDONLY, S_IRUSR);

	if (IS_ERR(fp)) {
		tk_debug_err(true, &tkey_i2c->client->dev, "failed to open %s.\n", TKEY_FW_PATH);
		ret = -ENOENT;
		set_fs(old_fs);
		return ret;
	}

	fsize = fp->f_path.dentry->d_inode->i_size;
	tk_debug_dbg(true, &tkey_i2c->client->dev,
		"start, file path %s, size %ld Bytes\n",
		TKEY_FW_PATH, fsize);

	tkey_i2c->fw_img = kmalloc(fsize, GFP_KERNEL);
	if (!tkey_i2c->fw_img) {
		tk_debug_err(true, &tkey_i2c->client->dev,
			"%s, kmalloc failed\n", __func__);
		ret = -EFAULT;
		goto malloc_error;
	}

	nread = vfs_read(fp, (char __user *)tkey_i2c->fw_img,
		fsize, &fp->f_pos);
	tk_debug_dbg(true, &tkey_i2c->client->dev, "nread %ld Bytes\n", nread);
	if (nread != fsize) {
		tk_debug_err(true, &tkey_i2c->client->dev,
			"failed to read firmware file, nread %ld Bytes\n",
			nread);
		ret = -EIO;
		kfree(tkey_i2c->fw_img);
		goto read_err;
	}

	filp_close(fp, current->files);
	set_fs(old_fs);

	return 0;

read_err:
malloc_error:
//size_error:
	filp_close(fp, current->files);
	set_fs(old_fs);
	return ret;
}

int touchkey_load_fw(struct touchkey_i2c *tkey_i2c, u8 fw_path)
{
	int ret = 0;

	switch (fw_path) {
	case FW_BUILT_IN:
		ret = tkey_load_fw_built_in(tkey_i2c);
		break;
	case FW_IN_SDCARD:
		ret = tkey_load_fw_sdcard(tkey_i2c);
		break;
	case FW_FFU:
		ret = tkey_load_fw_ffu(tkey_i2c);
	default:
		tk_debug_dbg(true, &tkey_i2c->client->dev,
			"unknown path(%d)\n", fw_path);
		break;
	}

	return ret;
}

void touchkey_unload_fw(struct touchkey_i2c *tkey_i2c)
{
	switch (tkey_i2c->fw_path) {
	case FW_BUILT_IN:
		release_firmware(tkey_i2c->firm_data);
		tkey_i2c->firm_data = NULL;
		break;
	case FW_IN_SDCARD:
		kfree(tkey_i2c->fw_img);
		tkey_i2c->fw_img = NULL;
		break;
	default:
		break;
	}
	tkey_i2c->fw_path = FW_NONE;
}

/* update condition */
int check_update_condition(struct touchkey_i2c *tkey_i2c)
{
	int ret = 0;

	/* check update condition */
	ret = is_same_module_class(tkey_i2c);
	if (ret == false) {
		printk(KERN_DEBUG"touchkey:md classes are different\n");
		return TK_EXIT_UPDATE;
	}

/*	if (tkey_i2c->md_ver_ic != tkey_i2c->src_md_ver)
		return TK_EXIT_UPDATE;*/

	if (tkey_i2c->fw_ver_ic < tkey_i2c->src_fw_ver)
		return TK_RUN_UPDATE;

	/* if ic ver is higher than file, exit */
	if (tkey_i2c->fw_ver_ic > tkey_i2c->src_fw_ver)
		return TK_EXIT_UPDATE;

	return TK_RUN_CHK;
}

int touchkey_fw_update(struct touchkey_i2c *tkey_i2c, u8 fw_path, bool bforced)
{
	int ret = 0;

	if (tkey_i2c->device_ver == 0x10) /* skip fw update(20075) */
		goto out;

	ret = touchkey_load_fw(tkey_i2c, fw_path);
	if (ret < 0) {
		tk_debug_err(true, &tkey_i2c->client->dev,
			"failed to load fw data\n");
		goto out;
	}
	tkey_i2c->fw_path = fw_path;

	/* f/w info */
	tk_debug_info(true, &tkey_i2c->client->dev, "fw ver %#x, new fw ver %#x\n",
		tkey_i2c->fw_ver_ic, tkey_i2c->src_fw_ver);
	tk_debug_info(true, &tkey_i2c->client->dev, "module ver %#x, new module ver %#x\n",
		tkey_i2c->md_ver_ic, tkey_i2c->src_md_ver);
#ifdef CRC_CHECK_INTERNAL
	tk_debug_info(true, &tkey_i2c->client->dev, "checkksum %#x, new checksum %#x\n",
		tkey_i2c->crc, tkey_i2c->fw_img->checksum);
#else
	tk_debug_info(true, &tkey_i2c->client->dev, "new checksum %#x\n",
		tkey_i2c->fw_img->checksum);
#endif

	if (unlikely(bforced))
		goto run_fw_update;

	ret = check_update_condition(tkey_i2c);

	if (ret == TK_EXIT_UPDATE) {
		dev_info(&tkey_i2c->client->dev, "pass fw update\n");
		touchkey_unload_fw(tkey_i2c);
		goto out;
	}

	if (ret == TK_RUN_CHK) {
#ifdef CRC_CHECK_INTERNAL
		if (tkey_i2c->crc == tkey_i2c->fw_img->checksum) {
			dev_info(&tkey_i2c->client->dev, "pass fw update\n");
			touchkey_unload_fw(tkey_i2c);
			goto out;
		}
#else
		tkey_i2c->do_checksum = true;
#endif
	}
	/* else do update */

 run_fw_update:
#ifdef TK_USE_FWUPDATE_DWORK
	queue_work(tkey_i2c->fw_wq, &tkey_i2c->update_work);
#else
	ret = touchkey_i2c_update(tkey_i2c);
	if (ret < 0) {
		tk_debug_err(true, &tkey_i2c->client->dev,
			"touchkey_i2c_update fail\n");
	}
#endif
out:
	return ret;
}

#ifdef TK_USE_FWUPDATE_DWORK
static void touchkey_i2c_update_work(struct work_struct *work)
{
	struct touchkey_i2c *tkey_i2c =
		container_of(work, struct touchkey_i2c, update_work);

	touchkey_i2c_update(tkey_i2c);
}
#endif

static int touchkey_i2c_update(struct touchkey_i2c *tkey_i2c)
{
	int ret;
	int retry = 3;

	disable_irq(tkey_i2c->irq);
	wake_lock(&tkey_i2c->fw_wakelock);

	tk_debug_dbg(true, &tkey_i2c->client->dev, "%s\n", __func__);
	tkey_i2c->update_status = TK_UPDATE_DOWN;

	cypress_config_gpio_i2c(tkey_i2c, 0);

	while (retry--) {
#if defined(CONFIG_KEYBOARD_CYPRESS_TOUCH_MBR31X5)
		ret = cy8cmbr_swd_program(tkey_i2c);
#else
		ret = ISSP_main(tkey_i2c);
#endif
		if (ret != 0) {
			msleep(50);
			tk_debug_err(true, &tkey_i2c->client->dev, "failed to update f/w. retry\n");
			continue;
		}

		tk_debug_info(true, &tkey_i2c->client->dev, "finish f/w update\n");
		tkey_i2c->update_status = TK_UPDATE_PASS;
		break;
	}

	if (retry <= 0) {
		tkey_i2c->pdata->power_on(tkey_i2c, 0);
		tkey_i2c->update_status = TK_UPDATE_FAIL;
		tk_debug_err(true, &tkey_i2c->client->dev, "failed to update f/w\n");
		ret = TK_UPDATE_FAIL;
		goto err_fw_update;
	}

	cypress_config_gpio_i2c(tkey_i2c, 1);

	msleep(150);

	ret = touchkey_i2c_check(tkey_i2c);
	if (ret < 0)
		goto err_fw_update;

	tk_debug_info(true, &tkey_i2c->client->dev, "f/w ver = %#X, module ver = %#X\n",
		tkey_i2c->fw_ver_ic, tkey_i2c->md_ver_ic);

	enable_irq(tkey_i2c->irq);
 err_fw_update:
	touchkey_unload_fw(tkey_i2c);
	wake_unlock(&tkey_i2c->fw_wakelock);
	return ret;
}


static irqreturn_t touchkey_interrupt(int irq, void *dev_id)
{
	struct touchkey_i2c *tkey_i2c = dev_id;
	u8 data[3];
	int ret;
	int i;
	int keycode_data[touchkey_count];

	if (unlikely(!touchkey_probe)) {
		tk_debug_err(true, &tkey_i2c->client->dev, "%s: Touchkey is not probed\n", __func__);
		return IRQ_HANDLED;
	}

	ret = i2c_touchkey_read(tkey_i2c->client, KEYCODE_REG, data, 1);
	if (ret < 0)
		return IRQ_HANDLED;

	keycode_data[1] = data[0] & 0x3;
	keycode_data[2] = (data[0] >> 2) & 0x3;

	for (i = 1; i < touchkey_count; i++) {
		if (keycode_data[i]) {
			input_report_key(tkey_i2c->input_dev, touchkey_keycode[i], (keycode_data[i] % 2));
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
			tk_debug_err(true, &tkey_i2c->client->dev, "keycode: %d %s, %#x %#x %d\n",
					touchkey_keycode[i], (keycode_data[i] % 2) ? "PRESS" : "RELEASE",
					tkey_i2c->fw_ver_ic, tkey_i2c->md_ver_ic, tkey_i2c->mc_data.cur_mode);
#else
			tk_debug_err(true, &tkey_i2c->client->dev, " %s, %#x %#x %d\n",
					(keycode_data[i] % 2) ? "PRESS" : "RELEASE", tkey_i2c->fw_ver_ic,
						tkey_i2c->md_ver_ic, tkey_i2c->mc_data.cur_mode);
#endif
#if defined(CONFIG_INPUT_BOOSTER)
			input_booster_send_event(BOOSTER_DEVICE_TOUCHKEY, (keycode_data[i] % 2));
#endif
		}
	}

	input_sync(tkey_i2c->input_dev);

	return IRQ_HANDLED;
}

static int touchkey_stop(struct touchkey_i2c *tkey_i2c)
{
	int i;

	mutex_lock(&tkey_i2c->lock);

	if (!tkey_i2c->enabled) {
		tk_debug_err(true, &tkey_i2c->client->dev, "Touch key already disabled\n");
		goto err_stop_out;
	}
	if (wake_lock_active(&tkey_i2c->fw_wakelock)) {
		tk_debug_dbg(true, &tkey_i2c->client->dev, "wake_lock active\n");
		goto err_stop_out;
	}

	disable_irq(tkey_i2c->irq);

	/* release keys */
	for (i = 1; i < touchkey_count; ++i) {
		input_report_key(tkey_i2c->input_dev,
				 touchkey_keycode[i], 0);
	}
#if defined(CONFIG_INPUT_BOOSTER)
	input_booster_send_event(BOOSTER_DEVICE_TOUCHKEY, BOOSTER_MODE_FORCE_OFF);
#endif
	input_sync(tkey_i2c->input_dev);

#if defined(CONFIG_GLOVE_TOUCH)
	if (false == tkey_i2c->pdata->glove_mode_keep_status)
		tkey_i2c->tsk_enable_glove_mode = false;
	touchkey_mode_change(tkey_i2c, CMD_GLOVE_OFF);
#endif

	tkey_i2c->enabled = false;
	tkey_i2c->status_update = false;

	if (touchkey_led_status == TK_CMD_LED_ON)
		touchled_cmd_reversed = 1;

	tkey_i2c->pdata->power_on(tkey_i2c, 0);

 err_stop_out:
	mutex_unlock(&tkey_i2c->lock);

	return 0;
}

int touchkey_read_status(struct touchkey_i2c *tkey_i2c)
{
	char data[6] = {0, };
	int ret = 0;
	int retry = 3;

	while(retry--) {
		ret = i2c_touchkey_read(tkey_i2c->client, TK_STATUS_FLAG, data, 1);
		if (ret < 0) {
			dev_err(&tkey_i2c->client->dev, "%s: Failed to read Keycode_reg %d times\n",
				__func__, retry);
			continue;
		}
	}
	if (ret < 0)
		return ret;

	if (data[0] & TK_BIT_GLOVE)
		tkey_i2c->ic_mode = MODE_GLOVE;
	else if (data[0] & TK_BIT_FLIP)
		tkey_i2c->ic_mode = MODE_FLIP;
	else
		tkey_i2c->ic_mode = MODE_NORMAL;

	printk(KERN_DEBUG"touchkey:ic mode is %c%d\n", "0ngf"[tkey_i2c->ic_mode], tkey_i2c->ic_mode);
	return ret;
}

static int touchkey_start(struct touchkey_i2c *tkey_i2c)
{
	mutex_lock(&tkey_i2c->lock);

	if (tkey_i2c->enabled) {
		tk_debug_err(true, &tkey_i2c->client->dev, "Touch key already enabled\n");
		goto err_start_out;
	}
	if (wake_lock_active(&tkey_i2c->fw_wakelock)) {
		tk_debug_dbg(true, &tkey_i2c->client->dev, "wake_lock active\n");
		goto err_start_out;
	}

	tkey_i2c->pdata->power_on(tkey_i2c, 1);
	msleep(tkey_i2c->pdata->stabilizing_time);

	tkey_i2c->enabled = true;

	if (touchled_cmd_reversed) {
		touchled_cmd_reversed = 0;
		i2c_touchkey_write(tkey_i2c->client,
			(u8 *) &touchkey_led_status, 1);
		tk_debug_err(true, &tkey_i2c->client->dev, "%s: Turning LED is reserved\n", __func__);
		msleep(30);
	}

#ifdef TEST_JIG_MODE
    touchkey_enable_status_update(tkey_i2c);
#endif

#if defined(TK_INFORM_CHARGER)
	touchkey_ta_setting(tkey_i2c);
#endif

#if defined(CONFIG_GLOVE_TOUCH)
	if (tkey_i2c->tsk_enable_glove_mode)
		touchkey_mode_change(tkey_i2c, CMD_GLOVE_ON);
	touchkey_read_status(tkey_i2c);
	touchkey_mode_change(tkey_i2c, CMD_MODE_RESERVED);
#endif

	enable_irq(tkey_i2c->irq);
 err_start_out:
	mutex_unlock(&tkey_i2c->lock);

	return 0;
}
#ifdef USE_OPEN_CLOSE
#ifdef TK_USE_OPEN_DWORK
static void touchkey_open_work(struct work_struct *work)
{
	int retval;
	struct touchkey_i2c *tkey_i2c =
			container_of(work, struct touchkey_i2c,
			open_work.work);

	if (tkey_i2c->enabled) {
		tk_debug_err(true, &tkey_i2c->client->dev, "Touch key already enabled\n");
		return;
	}

	retval = touchkey_start(tkey_i2c);
	if (retval < 0)
		tk_debug_err(true, &tkey_i2c->client->dev,
				"%s: Failed to start device\n", __func__);
}
#endif

static int touchkey_input_open(struct input_dev *dev)
{
	struct touchkey_i2c *data = input_get_drvdata(dev);
	int ret;

	if (!touchkey_probe) {
		tk_debug_err(true, &data->client->dev, "%s: Touchkey is not probed\n", __func__);
		return 0;
	}

/*	ret = wait_for_completion_interruptible_timeout(&data->init_done,
			msecs_to_jiffies(90 * MSEC_PER_SEC));

	if (ret < 0) {
		tk_debug_err(true, &data->client->dev,
			"error while waiting for device to init (%d)\n", ret);
		ret = -ENXIO;
		goto err_open;
	}
	if (ret == 0) {
		tk_debug_err(true, &data->client->dev,
			"timedout while waiting for device to init\n");
		ret = -ENXIO;
		goto err_open;
	}*/
#ifdef TK_USE_OPEN_DWORK
	schedule_delayed_work(&data->open_work,
					msecs_to_jiffies(TK_OPEN_DWORK_TIME));
#else
	ret = touchkey_start(data);
	if (ret)
		goto err_open;
#endif

	tk_debug_dbg(true, &data->client->dev, "%s\n", __func__);

	return 0;

err_open:
	return ret;
}

static void touchkey_input_close(struct input_dev *dev)
{
	struct touchkey_i2c *data = input_get_drvdata(dev);

	if (!touchkey_probe) {
		tk_debug_err(true, &data->client->dev, "%s: Touchkey is not probed\n", __func__);
		return;
	}

#ifdef TK_USE_OPEN_DWORK
	cancel_delayed_work(&data->open_work);
#endif
	touchkey_stop(data);

	tk_debug_dbg(true, &data->client->dev, "%s\n", __func__);
}
#endif

#ifdef CONFIG_PM
#ifdef CONFIG_HAS_EARLYSUSPEND
#define touchkey_suspend	NULL
#define touchkey_resume	NULL

static int sec_touchkey_early_suspend(struct early_suspend *h)
{
	struct touchkey_i2c *tkey_i2c =
		container_of(h, struct touchkey_i2c, early_suspend);

	touchkey_stop(tkey_i2c);

	tk_debug_dbg(true, &tkey_i2c->client->dev, "%s\n", __func__);

	return 0;
}

static int sec_touchkey_late_resume(struct early_suspend *h)
{
	struct touchkey_i2c *tkey_i2c =
		container_of(h, struct touchkey_i2c, early_suspend);

	tk_debug_dbg(true, &tkey_i2c->client->dev, "%s\n", __func__);

	touchkey_start(tkey_i2c);

	return 0;
}
#else
static int touchkey_suspend(struct device *dev)
{
	struct touchkey_i2c *tkey_i2c = dev_get_drvdata(dev);

	if (touchkey_probe != true) {
		tk_debug_err(true, &tkey_i2c->client->dev,
			"%s Touchkey is not enabled. \n", __func__);
		return 0;
	}
	mutex_lock(&tkey_i2c->input_dev->mutex);

	if (tkey_i2c->input_dev->users)
		touchkey_stop(tkey_i2c);

	mutex_unlock(&tkey_i2c->input_dev->mutex);

	tk_debug_dbg(true, &tkey_i2c->client->dev, "%s\n", __func__);

	return 0;
}

static int touchkey_resume(struct device *dev)
{
	struct touchkey_i2c *tkey_i2c = dev_get_drvdata(dev);

	if (touchkey_probe != true) {
		tk_debug_err(true, &tkey_i2c->client->dev,
			"%s Touchkey is not enabled. \n", __func__);
		return 0;
	}
	mutex_lock(&tkey_i2c->input_dev->mutex);

	if (tkey_i2c->input_dev->users)
		touchkey_start(tkey_i2c);

	mutex_unlock(&tkey_i2c->input_dev->mutex);

	tk_debug_dbg(true, &tkey_i2c->client->dev, "%s\n", __func__);

	return 0;
}
#endif
static SIMPLE_DEV_PM_OPS(touchkey_pm_ops, touchkey_suspend, touchkey_resume);
#endif

static ssize_t touchkey_led_control(struct device *dev,
				 struct device_attribute *attr, const char *buf,
				 size_t size)
{
	struct touchkey_i2c *tkey_i2c = dev_get_drvdata(dev);
	int data,on;
	int ret;
	static const int ledCmd[] = {TK_CMD_LED_OFF, TK_CMD_LED_ON};

	if (wake_lock_active(&tkey_i2c->fw_wakelock)) {
		tk_debug_dbg(true, &tkey_i2c->client->dev,
			"%s, wakelock active\n", __func__);
		return size;
	}

	ret = sscanf(buf, "%d", &data);
	tk_debug_dbg(true, &tkey_i2c->client->dev,
			"%s, %d\n", __func__, data);

	if (ret != 1) {
		tk_debug_err(true, &tkey_i2c->client->dev, "%s, %d err\n",
			__func__, __LINE__);
		return size;
	}

	if (data != 0 && data != 1) {
		tk_debug_err(true, &tkey_i2c->client->dev, "%s wrong cmd %x\n",
			__func__, data);
		return size;
	}

	on = data; /* data back-up to control led by ldo */
	data = ledCmd[data];
	touchkey_led_status = data;

	if (!tkey_i2c->enabled) {
		touchled_cmd_reversed = 1;
		goto out_led_control;
	}

	ret = i2c_touchkey_write(tkey_i2c->client, (u8 *) &data, 1);
	if (ret < 0) {
		tk_debug_err(true, &tkey_i2c->client->dev, "%s: Error turn on led %d\n",
			__func__, ret);
		touchled_cmd_reversed = 1;
		goto out_led_control;
	}
		msleep(30);

out_led_control:
	return size;
}

#ifdef CONFIG_GLOVE_TOUCH
int tkey_cmd_to_mode(int cmd)
{
	switch (cmd) {
	case CMD_GLOVE_ON:
		return MODE_GLOVE;
	case CMD_FLIP_ON:
		return MODE_FLIP;
	default:
		return MODE_NORMAL;
	}
	return MODE_NORMAL;
}

bool tkey_is_enabled(struct touchkey_i2c *tkey_i2c)
{
	if (!touchkey_probe) {
		return false;
	}
	if (!tkey_i2c->enabled) {
		return false;
	}
	if (wake_lock_active(&tkey_i2c->fw_wakelock)) {
		return false;
	}

	return true;
}

int touchkey_mode_change(struct touchkey_i2c *tkey_i2c, int cmd)
{
	int mode;
	int final_mode;
	bool changed = false;
	struct cmd_mode_change *pTarget;
	int is_enabled;
	static bool skip_by_status = false;
	static bool g_reserved_by_f1;
	struct mode_change_data *mc_data = &tkey_i2c->mc_data;

	is_enabled = tkey_is_enabled(tkey_i2c);

	mutex_lock(&tkey_i2c->mc_data.mc_lock);
	mutex_lock(&tkey_i2c->mc_data.mcf_lock);

	/* etc cmd handling */
	if (cmd == CMD_GET_LAST_MODE) {
		int mode;
		if (mc_data->mtr.mode)
			mode = mc_data->mtr.mode;
		else if (mc_data->mtc.mode)
			mode = mc_data->mtc.mode;
		else
			mode = tkey_i2c->ic_mode;

		mutex_unlock(&tkey_i2c->mc_data.mcf_lock);
		mutex_unlock(&tkey_i2c->mc_data.mc_lock);
		return mode;
	}
	if (cmd == CMD_MODE_RESET) {
		mc_data->cur_mode = MODE_NORMAL;
		mc_data->mtc.mode = MODE_NONE;
		if (mc_data->busy)
			mc_data->mtr.cmd = CMD_MODE_RESET;
		else
			mc_data->mtr.mode = MODE_NONE;
		tk_debug_dbg(true, &tkey_i2c->client->dev, "touchkey:reset mode change\n");
		goto out_mode_change;
	}
	if (cmd == CMD_MODE_RESERVED) {
		if (skip_by_status)
			goto out_mode_change;
		if (mc_data->mtc.mode)
			goto out_mode_change;
		if (tkey_i2c->ic_mode == mc_data->cur_mode)
			goto out_mode_change;

		tk_debug_dbg(true, &tkey_i2c->client->dev, "touchkey:cmd reserved\n");

		if (mc_data->cur_mode == MODE_FLIP) {
			mc_data->cur_mode = MODE_NORMAL;
			cmd = CMD_FLIP_ON;
		} else if (mc_data->cur_mode == MODE_GLOVE) {
			mc_data->cur_mode = MODE_NORMAL;
			cmd = CMD_GLOVE_ON;
		} else
			goto out_mode_change;
	}

	/*tk_debug_dbg(true, &tkey_i2c->client->dev, "touchkey:mc:cur mode %d, cmd %d\n", mc_data->cur_mode, cmd);*/
	mode = tkey_cmd_to_mode(cmd);

	/* setup target pointer */
	if (!mc_data->busy && !skip_by_status) {
		pTarget = &mc_data->mtc;
		final_mode = mc_data->cur_mode;
	} else if (mc_data->busy || skip_by_status) {
		if (!mc_data->mtr.mode) {
			pTarget = &mc_data->mtr;
			final_mode = mc_data->mtc.mode;
		} else if (mc_data->mtr.mode) {
			pTarget = &mc_data->mtr;
			final_mode = mc_data->mtr.mode;
		}
	}

	/* check last command */
	if (final_mode == mode) {
		tk_debug_dbg(true, &tkey_i2c->client->dev, "touchkey:final mode %d, cmd %d, same, skip\n", final_mode, mode);
		goto out_mode_change;
	}

	/* set cmd by priority flip > glove, normal */
	if (final_mode == MODE_FLIP) {
		if (cmd == CMD_FLIP_OFF) {
			if (g_reserved_by_f1) {
				g_reserved_by_f1 = false;
				pTarget->mode = MODE_GLOVE;
				pTarget->cmd = CMD_GLOVE_ON;
			} else {
				pTarget->mode = MODE_NORMAL;
				pTarget->cmd = cmd;
			}
			changed = true;
		} else if (cmd == CMD_GLOVE_ON) {
			g_reserved_by_f1 = true;
		} else {
			g_reserved_by_f1 = false;
			goto out_mode_change;
		}
	}  else if (final_mode == MODE_GLOVE) {
		if (cmd == CMD_FLIP_OFF)
			goto out_mode_change;
		pTarget->mode = mode;
		pTarget->cmd = cmd;
		changed = true;
	}  else {
		pTarget->mode = mode;
		pTarget->cmd = cmd;
		changed = true;
	}

	/* queue size control */
	if (mc_data->busy) {
		if (mc_data->mtc.mode == mc_data->mtr.mode) {
			tk_debug_dbg(true, &tkey_i2c->client->dev, "touchkey:previous mode is same with this mode, %d %d\n",
				mc_data->mtc.mode, mc_data->mtr.mode);
			mc_data->mtr.mode = MODE_NONE;
			goto out_mode_change;
		}
	}

	/* execute mode change work */
	if (!mc_data->busy && changed) {
		mc_data->busy = true;
		/* execute by IC status */
		if (is_enabled) {
			/*cancel_work_sync(&tkey_i2c->mode_change_work);*/
			schedule_work(&tkey_i2c->mode_change_work);
		} else {
			skip_by_status = true;
			tk_debug_dbg(true, &tkey_i2c->client->dev, "touchkey:skipped by flag\n");
		}
	}

out_mode_change:
	if (skip_by_status && is_enabled) {
		skip_by_status = false;
		if (mc_data->mtr.mode) {
			mc_data->mtc.mode = mc_data->mtr.mode;
			mc_data->mtr.mode = MODE_NONE;
		}
		/* do not need to check status of work ? */
		/*cancel_work_sync(&tkey_i2c->mode_change_work);*/
		schedule_work(&tkey_i2c->mode_change_work);
		tk_debug_dbg(true, &tkey_i2c->client->dev, "touchkey:execute change work by skipped flag\n");
	}

	mutex_unlock(&tkey_i2c->mc_data.mcf_lock);
	mutex_unlock(&tkey_i2c->mc_data.mc_lock);
	return 0;
}

int touchkey_mode_control(struct touchkey_i2c *tkey_i2c, int mode)
{
	u8 command[] = {0, TK_BIT_CMD_REGULAR, TK_BIT_CMD_GLOVE, TK_BIT_CMD_FLIP};
	u8 data[6] = { 0, };
	unsigned short retry = 3;
	u8 status;
	int ret = 0;

	while (retry--) {
		data[1] = command[mode];
		data[2] = TK_BIT_WRITE_CONFIRM;

		ret = i2c_touchkey_write(tkey_i2c->client, data, 3);
		if (ret < 0) {
			tk_debug_err(true, &tkey_i2c->client->dev, "%s: Failed to write command Keycode_reg %d times\n",
				__func__, retry);
			continue;
		}

		msleep(30);

		dev_dbg(&tkey_i2c->client->dev,
			"data[0]=%x data[1]=%x data[2]=%x data[3]=%x\n",
			data[0], data[1], data[2], data[3]);

		/* Check status */
		ret = i2c_touchkey_read(tkey_i2c->client, TK_STATUS_FLAG, data, 1);
		if (ret < 0) {
			tk_debug_err(true, &tkey_i2c->client->dev, "%s: Failed to read Keycode_reg %d times\n",
				__func__, retry);
			continue;
		}

		switch (mode) {
		case MODE_NORMAL:
			status = data[0] & TK_BIT_GLOVE;
			status |= data[0] & TK_BIT_FLIP;
			if (status) {
				tk_debug_err(true, &tkey_i2c->client->dev, "%s: error to set normal mode, bit g:%d, f:%d\n",
					__func__, !!(data[0] & TK_BIT_GLOVE), !!(data[0] & TK_BIT_FLIP));
				continue;
			}
			break;
		case MODE_GLOVE:
			status = !!(data[0] & TK_BIT_GLOVE);
			if (!status) {
				tk_debug_err(true, &tkey_i2c->client->dev, "%s: error to set glove mode, bit g:%d, f:%d\n",
					__func__, !!(data[0] & TK_BIT_GLOVE), !!(data[0] & TK_BIT_FLIP));
				continue;
			}
			break;
		case MODE_FLIP:
			status = !!(data[0] & TK_BIT_FLIP);
			if (!status) {
				tk_debug_err(true, &tkey_i2c->client->dev, "%s: error to set flip mode, bit g:%d, f:%d\n",
					__func__, !!(data[0] & TK_BIT_GLOVE), !!(data[0] & TK_BIT_FLIP));
				continue;
			}
			break;
		default:
			tk_debug_dbg(true, &tkey_i2c->client->dev, "touchkey:%s, unknown mode %d\n", __func__, mode);
			break;
		}
		break;
	}

	if (retry == 3)
		tk_debug_err(true, &tkey_i2c->client->dev, "%s: Failed to change mode to %d\n", __func__, mode);

	return ret;
}

static void touchkey_i2c_mode_change_work(struct work_struct *work)
{
	struct touchkey_i2c *tkey_i2c =
		container_of(work, struct touchkey_i2c, mode_change_work);
	struct mode_change_data *mc_data = &tkey_i2c->mc_data;
	struct cmd_mode_change mode;
	int ret = 0;

	while (1) {
		/* dequeue */
		mutex_lock(&tkey_i2c->mc_data.mcf_lock);
		if (mc_data->mtc.mode == MODE_NONE)
			goto out_mode_change_work;
		mc_data->busy = true;
		mode.mode = mc_data->mtc.mode;
		mode.cmd = mc_data->mtc.cmd;
		mutex_unlock(&tkey_i2c->mc_data.mcf_lock);

		/* change mode */
		ret = touchkey_mode_control(tkey_i2c, mode.mode);
		if (ret < 0) {
			tk_debug_err(true, &tkey_i2c->client->dev, "%s: error to change mode to %d\n",
				__func__, mode.mode);
		}
		/*tk_debug_dbg(true, &tkey_i2c->client->dev, "touchkey:change mode, %d %d\n", mc_data->cur_mode, mode.mode);*/

		mutex_lock(&tkey_i2c->mc_data.mcf_lock);
		mc_data->cur_mode = mode.mode;
		mutex_unlock(&tkey_i2c->mc_data.mcf_lock);

		tk_debug_dbg(true, &tkey_i2c->client->dev, "touchkey:mcf:%c%d\n", "0ngf"[mc_data->cur_mode], mc_data->cur_mode);

		/* after job */
		mutex_lock(&tkey_i2c->mc_data.mcf_lock);
		if (mc_data->mtr.cmd == CMD_MODE_RESET) {
			mc_data->cur_mode = MODE_NORMAL;
			tk_debug_dbg(true, &tkey_i2c->client->dev, "touchkey:%s, reset reserved, exit\n", __func__);
			goto out_mode_change_work;
		}
		if (mc_data->mtr.mode) {
			mc_data->mtc.mode = mc_data->mtr.mode;
			mc_data->mtc.cmd = mc_data->mtr.cmd;
			mc_data->mtr.mode = MODE_NONE;
		} else {
			mc_data->mtc.mode = MODE_NONE;
			goto out_mode_change_work;
		}
		mutex_unlock(&tkey_i2c->mc_data.mcf_lock);
	}

out_mode_change_work:
	mc_data->busy = false;
	mutex_unlock(&tkey_i2c->mc_data.mcf_lock);
	return ;
}
#endif

#if defined(CONFIG_GLOVE_TOUCH)
static ssize_t glove_mode_enable(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t size)
{
	struct touchkey_i2c *tkey_i2c = dev_get_drvdata(dev);
	int data;
	int mode;

	sscanf(buf, "%d\n", &data);
	tk_debug_dbg(true, &tkey_i2c->client->dev, "%s %d\n", __func__, data);

	mode = touchkey_mode_change(tkey_i2c, CMD_GET_LAST_MODE);
	if (mode == MODE_FLIP && data == 0) {
		printk(KERN_DEBUG"touchkey:%s, pass glove off by flip on\n", __func__);
		return size;
	}

	tkey_i2c->tsk_enable_glove_mode = data;
	touchkey_mode_change(tkey_i2c, (data == 1) ? CMD_GLOVE_ON : CMD_GLOVE_OFF);
	/*touchkey_glovemode(data);*/
	/*printk(KERN_DEBUG"touchkey:%s\n", __func__);*/

	return size;
}
#endif

#ifdef TKEY_FLIP_MODE
static ssize_t flip_cover_mode_enable(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t size)
{
	struct touchkey_i2c *tkey_i2c = dev_get_drvdata(dev);
	int flip_mode_on;

	sscanf(buf, "%d\n", &flip_mode_on);
	tk_debug_dbg(true, &tkey_i2c->client->dev, "%s %d\n", __func__, flip_mode_on);

	/* glove mode control */
	if (flip_mode_on) {
		touchkey_mode_change(tkey_i2c, CMD_FLIP_ON);
	} else {
		if (tkey_i2c->tsk_enable_glove_mode)
			touchkey_mode_change(tkey_i2c, CMD_GLOVE_ON);
		touchkey_mode_change(tkey_i2c, CMD_FLIP_OFF);
	}

	return size;
}
#endif

static ssize_t touch_sensitivity_control(struct device *dev,
					 struct device_attribute *attr,
					 const char *buf, size_t size)
{
	struct touchkey_i2c *tkey_i2c = dev_get_drvdata(dev);
	int ret, enable;
	u8 data[4] = { 0, };

	sscanf(buf, "%d\n", &enable);
	tk_debug_info(true, &tkey_i2c->client->dev, "%s %d\n", __func__, enable);

/*    ret = touchkey_enable_status_update(tkey_i2c, data);
    if (ret < 0)
        return ret;
*/
	if (enable == 1) {
		data[1] = TK_BIT_CMD_INSPECTION;
		data[2] = TK_BIT_WRITE_CONFIRM;
		tkey_i2c->status_update = true;
	} else {
		/* Exit inspection mode */
		data[1] = TK_BIT_CMD_INSPECTION;
		data[2] = TK_BIT_EXIT_CONFIRM;

		ret = i2c_touchkey_write(tkey_i2c->client, data, 3);
		if (ret < 0) {
			tk_debug_err(true, &tkey_i2c->client->dev, "%s, Failed to exit inspection mode command.\n", __func__);
			tkey_i2c->status_update = false;
		}

		/* Send inspection mode Command */
		data[1] = TK_BIT_CMD_REGULAR;
		data[2] = TK_BIT_WRITE_CONFIRM;
		tkey_i2c->status_update = false;
	}

    ret = i2c_touchkey_write(tkey_i2c->client, data, 3);
	if (ret < 0) {
		tk_debug_err(true, &tkey_i2c->client->dev, "%s, err(%d)\n", __func__, ret);
		tkey_i2c->status_update = false;
		return ret;
	}

	ret = i2c_smbus_read_i2c_block_data(tkey_i2c->client,
				BASE_REG, 4, data);
	if (ret < 0) {
		tk_debug_info(true, &tkey_i2c->client->dev,
			"[Touchkey] fail to CYPRESS_DATA_UPDATE.\n");
		return ret;
	}
	if ((data[1] & 0x20))
		tk_debug_info(true, &tkey_i2c->client->dev,
			"[Touchkey] Control Enabled!!\n");
	else
		tk_debug_info(true, &tkey_i2c->client->dev,
			"[Touchkey] Control Disabled!!\n");

	return size;
}

static ssize_t set_touchkey_firm_version_show(struct device *dev,
					      struct device_attribute *attr,
					      char *buf)
{
	struct touchkey_i2c *tkey_i2c = dev_get_drvdata(dev);

	tk_debug_dbg(true, &tkey_i2c->client->dev,
			"firm_ver_bin %0#4x\n", tkey_i2c->src_fw_ver);
	return sprintf(buf, "%0#4x\n", tkey_i2c->src_fw_ver);
}

static ssize_t set_touchkey_update_store(struct device *dev,
					 struct device_attribute *attr,
					 const char *buf, size_t size)
{
	struct touchkey_i2c *tkey_i2c = dev_get_drvdata(dev);

	u8 fw_path;

	switch(*buf) {
	case 's':
	case 'S':
		fw_path = FW_BUILT_IN;
		break;
	case 'i':
	case 'I':
		fw_path = FW_IN_SDCARD;
		break;
	case 'f':
	case 'F':
		fw_path = FW_FFU;
		break;
	default:
		return size;
	}

	touchkey_fw_update(tkey_i2c, fw_path, true);

	msleep(3000);
	cancel_work_sync(&tkey_i2c->update_work);

	return size;
}

static ssize_t set_touchkey_firm_version_read_show(struct device *dev,
						   struct device_attribute
						   *attr, char *buf)
{
	struct touchkey_i2c *tkey_i2c = dev_get_drvdata(dev);
	char data[3] = { 0, };
	int count;

	i2c_touchkey_read(tkey_i2c->client, TK_FW_VER, data, 2);
	count = sprintf(buf, "0x%02x\n", data[0]);

	tk_debug_info(true, &tkey_i2c->client->dev, "Touch_version_read 0x%02x\n", data[0]);
	tk_debug_info(true, &tkey_i2c->client->dev, "Module_version_read 0x%02x\n", data[1]);
	return count;
}

static ssize_t set_touchkey_firm_status_show(struct device *dev,
					     struct device_attribute *attr,
					     char *buf)
{
	struct touchkey_i2c *tkey_i2c = dev_get_drvdata(dev);
	int count = 0;

	tk_debug_info(true, &tkey_i2c->client->dev, "Touch_update_read: update_status %d\n",
	       tkey_i2c->update_status);

	if (tkey_i2c->update_status == TK_UPDATE_PASS)
		count = sprintf(buf, "PASS\n");
	else if (tkey_i2c->update_status == TK_UPDATE_DOWN)
		count = sprintf(buf, "Downloading\n");
	else if (tkey_i2c->update_status == TK_UPDATE_FAIL)
		count = sprintf(buf, "Fail\n");

	return count;
}
#ifdef TKEY_1MM_MODE
static ssize_t touchkey_1mm_mode_enable(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t size)
{
	struct touchkey_i2c *tkey_i2c = dev_get_drvdata(dev);
	int data;

	sscanf(buf, "%d\n", &data);
	tk_debug_info(true, &tkey_i2c->client->dev, "%s %d\n", __func__, data);

	touchkey_1mm_mode(tkey_i2c, data);

	return size;
}
#endif

static DEVICE_ATTR(brightness, S_IRUGO | S_IWUSR | S_IWGRP, NULL,
		   touchkey_led_control);
static DEVICE_ATTR(touch_sensitivity, S_IRUGO | S_IWUSR | S_IWGRP,
		   NULL, touch_sensitivity_control);
static DEVICE_ATTR(touchkey_firm_update, S_IRUGO | S_IWUSR | S_IWGRP,
		   NULL, set_touchkey_update_store);
static DEVICE_ATTR(touchkey_firm_update_status, S_IRUGO | S_IWUSR | S_IWGRP,
		   set_touchkey_firm_status_show, NULL);
static DEVICE_ATTR(touchkey_firm_version_phone, S_IRUGO | S_IWUSR | S_IWGRP,
		   set_touchkey_firm_version_show, NULL);
static DEVICE_ATTR(touchkey_firm_version_panel, S_IRUGO | S_IWUSR | S_IWGRP,
		   set_touchkey_firm_version_read_show, NULL);
#ifdef LED_LDO_WITH_REGULATOR
static DEVICE_ATTR(touchkey_brightness, S_IRUGO | S_IWUSR | S_IWGRP,
		   NULL, brightness_control);
#endif

#if !defined(TKEY_1MM_MODE)
static DEVICE_ATTR(touchkey_recent, S_IRUGO, touchkey_diff_data0_show, NULL);
static DEVICE_ATTR(touchkey_back, S_IRUGO, touchkey_diff_data1_show, NULL);
static DEVICE_ATTR(touchkey_recent_raw, S_IRUGO, touchkey_raw_data0_show, NULL);
static DEVICE_ATTR(touchkey_back_raw, S_IRUGO, touchkey_raw_data1_show, NULL);
static DEVICE_ATTR(touchkey_recent_idac, S_IRUGO, touchkey_idac0_show, NULL);
static DEVICE_ATTR(touchkey_back_idac, S_IRUGO, touchkey_idac1_show, NULL);
static DEVICE_ATTR(touchkey_threshold, S_IRUGO, touchkey_threshold0_show, NULL);
#else
static DEVICE_ATTR(touchkey_recent_outer, S_IRUGO, touchkey_diff_data0_show, NULL);
static DEVICE_ATTR(touchkey_recent_inner, S_IRUGO, touchkey_diff_data1_show, NULL);
static DEVICE_ATTR(touchkey_back_outer, S_IRUGO, touchkey_diff_data2_show, NULL);
static DEVICE_ATTR(touchkey_back_inner, S_IRUGO, touchkey_diff_data3_show, NULL);
static DEVICE_ATTR(touchkey_recent_raw_outer, S_IRUGO, touchkey_raw_data0_show, NULL);
static DEVICE_ATTR(touchkey_recent_raw_inner, S_IRUGO, touchkey_raw_data1_show, NULL);
static DEVICE_ATTR(touchkey_back_raw_outer, S_IRUGO, touchkey_raw_data2_show, NULL);
static DEVICE_ATTR(touchkey_back_raw_inner, S_IRUGO, touchkey_raw_data3_show, NULL);
static DEVICE_ATTR(touchkey_baseline_data0, S_IRUGO, touchkey_baseline_data0_show, NULL);
static DEVICE_ATTR(touchkey_baseline_data1, S_IRUGO, touchkey_baseline_data1_show, NULL);
static DEVICE_ATTR(touchkey_baseline_data2, S_IRUGO, touchkey_baseline_data2_show, NULL);
static DEVICE_ATTR(touchkey_baseline_data3, S_IRUGO, touchkey_baseline_data3_show, NULL);
static DEVICE_ATTR(touchkey_recent_idac_outer, S_IRUGO, touchkey_idac0_show, NULL);
static DEVICE_ATTR(touchkey_recent_idac_inner, S_IRUGO, touchkey_idac1_show, NULL);
static DEVICE_ATTR(touchkey_back_idac_outer, S_IRUGO, touchkey_idac2_show, NULL);
static DEVICE_ATTR(touchkey_back_idac_inner, S_IRUGO, touchkey_idac3_show, NULL);
static DEVICE_ATTR(touchkey_comp_idac0, S_IRUGO, touchkey_comp_idac0_show, NULL);
static DEVICE_ATTR(touchkey_comp_idac1, S_IRUGO, touchkey_comp_idac1_show, NULL);
static DEVICE_ATTR(touchkey_comp_idac2, S_IRUGO, touchkey_comp_idac2_show, NULL);
static DEVICE_ATTR(touchkey_comp_idac3, S_IRUGO, touchkey_comp_idac3_show, NULL);
static DEVICE_ATTR(touchkey_recent_threshold_outer, S_IRUGO, touchkey_threshold0_show, NULL);
static DEVICE_ATTR(touchkey_recent_threshold_inner, S_IRUGO, touchkey_threshold1_show, NULL);
static DEVICE_ATTR(touchkey_back_threshold_outer, S_IRUGO, touchkey_threshold2_show, NULL);
static DEVICE_ATTR(touchkey_back_threshold_inner, S_IRUGO, touchkey_threshold3_show, NULL);
#endif
#if defined(CONFIG_GLOVE_TOUCH)
static DEVICE_ATTR(glove_mode, S_IRUGO | S_IWUSR | S_IWGRP, NULL,
		   glove_mode_enable);
#endif
#ifdef TKEY_FLIP_MODE
static DEVICE_ATTR(flip_mode, S_IRUGO | S_IWUSR | S_IWGRP, NULL,
		   flip_cover_mode_enable);
#endif
#ifdef TKEY_1MM_MODE
static DEVICE_ATTR(1mm_mode, S_IRUGO | S_IWUSR | S_IWGRP, NULL,
		touchkey_1mm_mode_enable);
#endif

static struct attribute *touchkey_attributes[] = {
	&dev_attr_brightness.attr,
	&dev_attr_touch_sensitivity.attr,
	&dev_attr_touchkey_firm_update.attr,
	&dev_attr_touchkey_firm_update_status.attr,
	&dev_attr_touchkey_firm_version_phone.attr,
	&dev_attr_touchkey_firm_version_panel.attr,
#ifdef LED_LDO_WITH_REGULATOR
	&dev_attr_touchkey_brightness.attr,
#endif
#if !defined(TKEY_1MM_MODE)
	&dev_attr_touchkey_recent.attr,
	&dev_attr_touchkey_back.attr,
	&dev_attr_touchkey_recent_raw.attr,
	&dev_attr_touchkey_back_raw.attr,
	&dev_attr_touchkey_recent_idac.attr,
	&dev_attr_touchkey_back_idac.attr,
	&dev_attr_touchkey_threshold.attr,
#else
	&dev_attr_touchkey_recent_outer.attr,
	&dev_attr_touchkey_recent_inner.attr,
	&dev_attr_touchkey_back_outer.attr,
	&dev_attr_touchkey_back_inner.attr,
	&dev_attr_touchkey_recent_raw_outer.attr,
	&dev_attr_touchkey_recent_raw_inner.attr,
	&dev_attr_touchkey_back_raw_outer.attr,
	&dev_attr_touchkey_back_raw_inner.attr,
	&dev_attr_touchkey_baseline_data0.attr,
	&dev_attr_touchkey_baseline_data1.attr,
	&dev_attr_touchkey_baseline_data2.attr,
	&dev_attr_touchkey_baseline_data3.attr,
	&dev_attr_touchkey_recent_idac_outer.attr,
	&dev_attr_touchkey_recent_idac_inner.attr,
	&dev_attr_touchkey_back_idac_outer.attr,
	&dev_attr_touchkey_back_idac_inner.attr,
	&dev_attr_touchkey_comp_idac0.attr,
	&dev_attr_touchkey_comp_idac1.attr,
	&dev_attr_touchkey_comp_idac2.attr,
	&dev_attr_touchkey_comp_idac3.attr,
	&dev_attr_touchkey_recent_threshold_outer.attr,
	&dev_attr_touchkey_recent_threshold_inner.attr,
	&dev_attr_touchkey_back_threshold_outer.attr,
	&dev_attr_touchkey_back_threshold_inner.attr,
#endif
#if defined(CONFIG_GLOVE_TOUCH)
	&dev_attr_glove_mode.attr,
#endif
#ifdef TKEY_FLIP_MODE
	&dev_attr_flip_mode.attr,
#endif
#ifdef TKEY_1MM_MODE
	&dev_attr_1mm_mode.attr,
#endif
	NULL,
};

static struct attribute_group touchkey_attr_group = {
	.attrs = touchkey_attributes,
};

static int touchkey_pinctrl_init(struct touchkey_i2c *tkey_i2c)
{
	struct device *dev = &tkey_i2c->client->dev;
	int i;
	int ret;

	tkey_i2c->pinctrl_irq = devm_pinctrl_get(dev);
	if (IS_ERR(tkey_i2c->pinctrl_irq)) {
		tk_debug_err(true, dev, "%s: Failed to get pinctrl\n", __func__);
		goto err_pinctrl_get;
	}
	for (i = 0; i < 2; ++i) {
		tkey_i2c->pin_state[i] = pinctrl_lookup_state(tkey_i2c->pinctrl_irq, str_states[i]);
		if (IS_ERR(tkey_i2c->pin_state[i])) {
			tk_debug_err(true, dev, "%s: Failed to get pinctrl state\n", __func__);
			goto err_pinctrl_get_state;
		}
	}

	/* sub det pin set */
	if (gpio_is_valid(tkey_i2c->pdata->sub_det)) {
		tkey_i2c->pinctrl_det = devm_pinctrl_get(dev);
		if (IS_ERR(tkey_i2c->pinctrl_det)) {
			tk_debug_err(true, dev, "%s: Failed to get pinctrl\n", __func__);
			goto err_pinctrl_get_det;
		}

		tkey_i2c->pins_default = pinctrl_lookup_state(tkey_i2c->pinctrl_det, "sub_det");
		if (IS_ERR(tkey_i2c->pins_default)) {
			tk_debug_err(true, dev, "%s: Failed to get pinctrl state\n", __func__);
			goto err_pinctrl_get_state_det;
		}

		ret = pinctrl_select_state(tkey_i2c->pinctrl_det, tkey_i2c->pins_default);
		if (ret < 0)
			tk_debug_err(true, dev, "%s: Failed to configure sub_det pin\n", __func__);
	}

	dev = tkey_i2c->client->dev.parent->parent;
	tk_debug_err(true, dev, "%s: use dev's parent\n", __func__);

	tkey_i2c->pinctrl_i2c = devm_pinctrl_get(dev);
	if (IS_ERR(tkey_i2c->pinctrl_i2c)) {
		tk_debug_err(true, dev, "%s: Failed to get pinctrl\n", __func__);
		goto err_pinctrl_get_i2c;
	}
	for (i = 2; i < 4; ++i) {
		tkey_i2c->pin_state[i] = pinctrl_lookup_state(tkey_i2c->pinctrl_i2c, str_states[i]);
		if (IS_ERR(tkey_i2c->pin_state[i])) {
			tk_debug_err(true, dev, "%s: Failed to get pinctrl state\n", __func__);
			goto err_pinctrl_get_state_i2c;
		}
	}

	return 0;

err_pinctrl_get_state_det:
	devm_pinctrl_put(tkey_i2c->pinctrl_det);
err_pinctrl_get_det:
err_pinctrl_get_state_i2c:
	devm_pinctrl_put(tkey_i2c->pinctrl_i2c);
err_pinctrl_get_i2c:
err_pinctrl_get_state:
	devm_pinctrl_put(tkey_i2c->pinctrl_irq);
err_pinctrl_get:
	return -ENODEV;
}

int touchkey_pinctrl(struct touchkey_i2c *tkey_i2c, int state)
{
	struct pinctrl *pinctrl = tkey_i2c->pinctrl_irq;
	int ret = 0;

	if (state >= I_STATE_ON_I2C)
		pinctrl = tkey_i2c->pinctrl_i2c;

	ret = pinctrl_select_state(pinctrl, tkey_i2c->pin_state[state]);
	if (ret < 0) {
		tk_debug_err(true, &tkey_i2c->client->dev,
		"%s: Failed to configure irq pin\n", __func__);
		return ret;
	}

	return 0;
}

static int touchkey_power_on(void *data, bool on)
{
	struct touchkey_i2c *tkey_i2c = (struct touchkey_i2c *)data;
	struct device *dev = &tkey_i2c->client->dev;
	struct regulator *regulator_core = NULL;
	struct regulator *regulator_io = NULL;
	static bool enabled;
	int ret = 0;
	int state = on ? I_STATE_ON_IRQ : I_STATE_OFF_IRQ;

	if (enabled == on) {
		tk_debug_err(true, dev,
			"%s : TK power already %s\n", __func__,(on)?"on":"off");
		return ret;
	}

	tk_debug_info(true, dev, "%s: %s",__func__,(on)?"on":"off");

	regulator_core = regulator_get(NULL, TK_CORE_REGULATOR_NAME);
	if (IS_ERR_OR_NULL(regulator_core)) {
		tk_debug_err(true, dev,
			"%s : TK regulator_get failed\n", __func__);
		ret = -EIO;
		goto out;
	}

	if (on) {
		ret = regulator_enable(regulator_core);
		if (ret) {
			tk_debug_err(true, dev,
				"%s: Failed to enable avdd: %d\n", __func__, ret);
			goto out;
		}
	} else {
		if (regulator_is_enabled(regulator_core))
			regulator_disable(regulator_core);
	}

	if (!tkey_i2c->pdata->ap_io_power) {
		regulator_io = regulator_get(NULL, TK_IO_REGULATOR_NAME);
		if (IS_ERR_OR_NULL(regulator_io)) {
			tk_debug_err(true, dev,
				"%s : TK regulator_get failed\n", __func__);
			ret = -EIO;
			goto out;
		}
		if (on) {
			ret = regulator_enable(regulator_io);
			if (ret) {
				tk_debug_err(true, dev,
					"%s: Failed to enable avdd: %d\n", __func__, ret);
				goto out;
			}
		} else {
			if (regulator_is_enabled(regulator_io))
				regulator_disable(regulator_io);
		}
	}

	ret = touchkey_pinctrl(tkey_i2c, state);
	if (ret < 0)
		tk_debug_err(true, dev,
		"%s: Failed to configure irq pin\n", __func__);

	enabled = on;

out:
	regulator_put(regulator_core);
	if (!tkey_i2c->pdata->ap_io_power)
		regulator_put(regulator_io);

	return ret;
}

static void cypress_config_gpio_i2c(struct touchkey_i2c *tkey_i2c, int onoff)
{
	int ret;
	int state = onoff ? I_STATE_ON_I2C : I_STATE_OFF_I2C;

	ret = touchkey_pinctrl(tkey_i2c, state);
		if (ret < 0)
			tk_debug_err(true, &tkey_i2c->client->dev,
				"%s: Failed to configure i2c pin\n", __func__);
}

#ifdef CONFIG_OF
static void cypress_request_gpio(struct i2c_client *client, struct touchkey_platform_data *pdata)
{
	int ret;

	if (!pdata->i2c_gpio) {
		ret = gpio_request(pdata->gpio_scl, "touchkey_scl");
		if (ret) {
			tk_debug_err(true, &client->dev,
				"%s: unable to request touchkey_scl [%d]\n",
					__func__, pdata->gpio_scl);
			return;
		}

		ret = gpio_request(pdata->gpio_sda, "touchkey_sda");
		if (ret) {
			tk_debug_err(true, &client->dev,"%s: unable to request touchkey_sda [%d]\n",
					__func__, pdata->gpio_sda);
			return;
		}
	}

	ret = gpio_request(pdata->gpio_int, "touchkey_irq");
	if (ret) {
		tk_debug_err(true, &client->dev,"%s: unable to request touchkey_irq [%d]\n",
				__func__, pdata->gpio_int);
		return;
	}
}

static struct touchkey_platform_data *cypress_parse_dt(struct i2c_client *client)
{

	struct device *dev = &client->dev;
	struct touchkey_platform_data *pdata;
	struct device_node *np = dev->of_node;
	int ret;

	if (!np)
		return ERR_PTR(-ENOENT);

	pdata = devm_kzalloc(dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata) {
		tk_debug_err(true, dev, "failed to allocate platform data\n");
		return ERR_PTR(-ENOMEM);
	}
	dev->platform_data = pdata;

	/* reset, irq gpio info */
	pdata->gpio_scl = of_get_named_gpio_flags(np, "cypress,scl-gpio",
				0, &pdata->scl_gpio_flags);
	pdata->gpio_sda = of_get_named_gpio_flags(np, "cypress,sda-gpio",
				0, &pdata->sda_gpio_flags);
	pdata->gpio_int = of_get_named_gpio_flags(np, "cypress,irq-gpio",
				0, &pdata->irq_gpio_flags);
	pdata->sub_det = of_get_named_gpio_flags(np, "cypress,sub-det",
				0, &pdata->sub_det_flags);

	pdata->i2c_gpio = of_property_read_bool(np, "cypress,i2c-gpio");
	pdata->boot_on_ldo = of_property_read_bool(np, "cypress,boot-on-ldo");
	pdata->ap_io_power = of_property_read_bool(np, "cypress,ap-io-power");

	ret = of_property_read_u32(np, "cypress,ic-stabilizing-time", &pdata->stabilizing_time);
	if (ret) {
		printk(KERN_ERR"touchkey:failed to ic-stabilizing-time %d\n", ret);
		pdata->stabilizing_time = 150;
	}

	ret = of_property_read_string(np, "cypress,fw_path", (const char **)&pdata->fw_path);
	if (ret) {
		printk(KERN_ERR"touchkey:failed to read fw_path %d\n", ret);
		pdata->fw_path = FW_PATH;
	}
	tk_debug_info(true, &client->dev, "touchkey:fw path %s\n", pdata->fw_path);

	cypress_request_gpio(client, pdata);

	return pdata;
}
#endif
static int i2c_touchkey_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	struct touchkey_platform_data *pdata = client->dev.platform_data;
	struct touchkey_i2c *tkey_i2c;
	bool bforced = false;
	struct input_dev *input_dev;

	int i;
	int ret = 0;

	if (lpcharge == 1) {
		tk_debug_err(true, &client->dev, "%s : Do not load driver due to : lpm %d\n",
			 __func__, lpcharge);
		return -ENODEV;
	}

	if (!pdata) {
		pdata = cypress_parse_dt(client);
		if (!pdata) {
			tk_debug_err(true, &client->dev, "%s: no pdata\n", __func__);
			return -EINVAL;
		}
	}

	/*Check I2C functionality */
	ret = i2c_check_functionality(client->adapter, I2C_FUNC_I2C);
	if (ret == 0) {
		tk_debug_err(true, &client->dev, "No I2C functionality found\n");
		return -ENODEV;
	}

	/*Obtain kernel memory space for touchkey i2c */
	tkey_i2c = kzalloc(sizeof(struct touchkey_i2c), GFP_KERNEL);
	if (NULL == tkey_i2c) {
		tk_debug_err(true, &client->dev, "Failed to allocate memory\n");
		return -ENOMEM;
	}

	input_dev = input_allocate_device();
	if (!input_dev) {
		tk_debug_err(true, &client->dev, "Failed to allocate input device\n");
		ret = -ENOMEM;
		goto err_allocate_input_device;
	}

	client->irq = gpio_to_irq(pdata->gpio_int);

	input_dev->name = "sec_touchkey";
	input_dev->phys = "sec_touchkey/input0";
	input_dev->id.bustype = BUS_HOST;
	input_dev->dev.parent = &client->dev;
#ifdef USE_OPEN_CLOSE
	input_dev->open = touchkey_input_open;
	input_dev->close = touchkey_input_close;
#endif
	/*tkey_i2c*/
	tkey_i2c->pdata = pdata;
	tkey_i2c->input_dev = input_dev;
	tkey_i2c->client = client;
	tkey_i2c->irq = client->irq;
	tkey_i2c->name = "sec_touchkey";
	tkey_i2c->status_update = false;
	tkey_i2c->pdata->power_on = touchkey_power_on;
	tkey_i2c->mc_data.cur_mode = MODE_NORMAL;

	init_completion(&tkey_i2c->init_done);
	mutex_init(&tkey_i2c->lock);
	mutex_init(&tkey_i2c->i2c_lock);
	mutex_init(&tkey_i2c->mc_data.mc_lock);
	mutex_init(&tkey_i2c->mc_data.mcf_lock);
#ifdef TK_USE_OPEN_DWORK
	INIT_DELAYED_WORK(&tkey_i2c->open_work, touchkey_open_work);
#endif
	set_bit(EV_SYN, input_dev->evbit);
	set_bit(EV_LED, input_dev->evbit);
	set_bit(LED_MISC, input_dev->ledbit);
	set_bit(EV_KEY, input_dev->evbit);
#ifdef CONFIG_VT_TKEY_SKIP_MATCH
	set_bit(EV_TOUCHKEY, input_dev->evbit);
#endif
#ifdef TK_USE_FWUPDATE_DWORK
	INIT_WORK(&tkey_i2c->update_work, touchkey_i2c_update_work);
#endif
#ifdef CONFIG_GLOVE_TOUCH
	INIT_WORK(&tkey_i2c->mode_change_work, touchkey_i2c_mode_change_work);
#endif
	wake_lock_init(&tkey_i2c->fw_wakelock, WAKE_LOCK_SUSPEND, "touchkey");

	for (i = 1; i < touchkey_count; i++)
		set_bit(touchkey_keycode[i], input_dev->keybit);

	input_set_drvdata(input_dev, tkey_i2c);
	i2c_set_clientdata(client, tkey_i2c);

	ret = input_register_device(input_dev);
	if (ret) {
		tk_debug_err(true, &client->dev, "Failed to register input device\n");
		goto err_register_device;
	}

	/* pinctrl */
	ret = touchkey_pinctrl_init(tkey_i2c);
	if (ret < 0) {
		tk_debug_err(true, &tkey_i2c->client->dev,
			"%s: Failed to init pinctrl: %d\n", __func__, ret);
		goto err_register_device;
	}

	/* power */
	tkey_i2c->pdata->power_on(tkey_i2c, 1);

	if (!tkey_i2c->pdata->boot_on_ldo)
		msleep(pdata->stabilizing_time);

	tkey_i2c->enabled = true;

	tkey_i2c->fw_wq = create_singlethread_workqueue(client->name);
	if (!tkey_i2c->fw_wq) {
		tk_debug_err(true, &client->dev, "fail to create workqueue for fw_wq\n");
		ret = -ENOMEM;
		goto err_create_fw_wq;
	}

/*	tk_debug_err(true, &client->dev, "LCD type Print : 0x%06X\n", lcdtype);
	if (lcdtype == 0) {
		tk_debug_err(true, &client->dev, "Device wasn't connected to board\n");
		ret = -ENODEV;
		goto err_i2c_check;
	}
*/
	if (gpio_is_valid(pdata->sub_det)) {
		ret = gpio_get_value(pdata->sub_det);
		if (ret) {
			tk_debug_err(true, &client->dev, "Device wasn't connected to board \n");
			ret = -ENODEV;
			goto err_i2c_check;
		}
	}

	cypress_config_gpio_i2c(tkey_i2c, 1);

	ret = touchkey_i2c_check(tkey_i2c);
	if (ret < 0) {
		tk_debug_err(true, &client->dev, "i2c_check failed\n");
		bforced = true;
	}

	ret = request_threaded_irq(tkey_i2c->irq, NULL, touchkey_interrupt,
				IRQF_DISABLED | IRQF_TRIGGER_FALLING |
				IRQF_ONESHOT, tkey_i2c->name, tkey_i2c);
	if (ret < 0) {
		tk_debug_err(true, &client->dev, "Failed to request irq(%d) - %d\n",
			tkey_i2c->irq, ret);
		goto err_request_threaded_irq;
	}

#if defined(TK_HAS_FIRMWARE_UPDATE)
/*tkey_firmupdate_retry_byreboot:
	touchkey_init_fw_name(tkey_i2c);*/
	ret = touchkey_fw_update(tkey_i2c, FW_BUILT_IN, bforced);
	if (ret < 0) {
		tk_debug_err(true, &client->dev, "fw update fail\n");
		goto err_firmware_update;
	}
#endif

#if defined(TK_INFORM_CHARGER)
	tkey_i2c->callbacks.inform_charger = touchkey_ta_cb;
	if (tkey_i2c->pdata->register_cb) {
		tk_debug_info(true, &client->dev, "Touchkey TA information\n");
		tkey_i2c->pdata->register_cb(&tkey_i2c->callbacks);
	}
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
	tkey_i2c->early_suspend.suspend =
		(void *)sec_touchkey_early_suspend;
	tkey_i2c->early_suspend.resume =
		(void *)sec_touchkey_late_resume;
	register_early_suspend(&tkey_i2c->early_suspend);
#endif

	/*sysfs*/
	tkey_i2c->dev = sec_device_create(tkey_i2c, "sec_touchkey");

	ret = IS_ERR(tkey_i2c->dev);
	if (ret) {
		tk_debug_err(true, &client->dev, "Failed to create device(tkey_i2c->dev)!\n");
	} else {
		ret = sysfs_create_group(&tkey_i2c->dev->kobj,
					&touchkey_attr_group);
		if (ret)
			tk_debug_err(true, &client->dev, "Failed to create sysfs group\n");

		ret = sysfs_create_link(&tkey_i2c->dev->kobj,
				&tkey_i2c->input_dev->dev.kobj, "input");
		if (ret)
			tk_debug_err(true, &client->dev, "Failed to connect link\n");
	}

/*	touchkey_stop(tkey_i2c); */
	complete_all(&tkey_i2c->init_done);
	touchkey_probe = true;

	return 0;

#if defined(TK_HAS_FIRMWARE_UPDATE)
err_firmware_update:
	disable_irq(tkey_i2c->irq);
	free_irq(tkey_i2c->irq, tkey_i2c);
#endif
err_request_threaded_irq:
err_i2c_check:
	destroy_workqueue(tkey_i2c->fw_wq);
err_create_fw_wq:
	tkey_i2c->pdata->power_on(tkey_i2c, 0);
	input_unregister_device(input_dev);
	input_dev = NULL;
err_register_device:
	wake_lock_destroy(&tkey_i2c->fw_wakelock);
	mutex_destroy(&tkey_i2c->mc_data.mcf_lock);
	mutex_destroy(&tkey_i2c->mc_data.mc_lock);
	mutex_destroy(&tkey_i2c->i2c_lock);
	mutex_destroy(&tkey_i2c->lock);
	input_free_device(input_dev);
err_allocate_input_device:
	kfree(tkey_i2c);
	return ret;
}

void touchkey_shutdown(struct i2c_client *client)
{
	struct touchkey_i2c *tkey_i2c = i2c_get_clientdata(client);

	if (!touchkey_probe)
		return;

	tkey_i2c->pdata->power_on(tkey_i2c, 0);

	tk_debug_err(true, &tkey_i2c->client->dev, "%s\n", __func__);
}
#ifdef CONFIG_OF
static struct of_device_id cypress_touchkey_dt_ids[] = {
	{ .compatible = "cypress,cypress_touchkey" },
	{ }
};
#endif

struct i2c_driver touchkey_i2c_driver = {
	.driver = {
		.name = "sec_touchkey_driver",
		.owner = THIS_MODULE,
#ifdef CONFIG_PM
		.pm = &touchkey_pm_ops,
#endif
#ifdef CONFIG_OF
		.of_match_table = of_match_ptr(cypress_touchkey_dt_ids),
#endif

	},
	.id_table = sec_touchkey_id,
	.probe = i2c_touchkey_probe,
	.shutdown = &touchkey_shutdown,
};

static int __init touchkey_init(void)
{
#ifdef TEST_JIG_MODE
	int ret;
#endif

	i2c_add_driver(&touchkey_i2c_driver);

#ifdef TEST_JIG_MODE
    ret = touchkey_enable_status_update(tkey_i2c);
	if (ret < 0)
		return ret;
#endif
	return 0;
}

static void __exit touchkey_exit(void)
{
	i2c_del_driver(&touchkey_i2c_driver);
	touchkey_probe = false;
}

module_init(touchkey_init);
module_exit(touchkey_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("@@@");
MODULE_DESCRIPTION("touch keypad");
