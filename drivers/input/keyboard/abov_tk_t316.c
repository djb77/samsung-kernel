/* abov_touchkey.c -- Linux driver for abov chip as touchkey
 *
 * Copyright (C) 2013 Samsung Electronics Co.Ltd
 * Author: Junkyeong Kim <jk0430.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 */

#include <linux/delay.h>
#include <linux/firmware.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <asm/unaligned.h>
#include <linux/regulator/consumer.h>
#include <linux/sec_class.h>
#include <linux/wakelock.h>
#include <linux/pinctrl/consumer.h>

#ifdef CONFIG_OF
#include <linux/of_gpio.h>
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#ifdef CONFIG_VBUS_NOTIFIER
#include <linux/muic/muic.h>
#include <linux/muic/muic_notifier.h>
#include <linux/vbus_notifier.h>
#endif

#include "abov_tk_t316.h"

/* registers */
#define ABOV_BTNSTATUS		0x00
#define ABOV_FW_VER			0x01
//#define ABOV_PCB_VER		0x02
//#define ABOV_COMMAND		0x03
#define ABOV_THRESHOLD		0x02
//#define ABOV_SENS			0x05
//#define ABOV_SETIDAC		0x06
#define ABOV_BTNSTATUS_NEW	0x07
#define ABOV_LED_RECENT		0x08		//LED Dimming (0x01~0x1F)
#define ABOV_LED_BACK		0x09		//LED Dimming (0x01~0x1F)
#define ABOV_DIFFDATA		0x0A
#define ABOV_RAWDATA		0x0E
#define ABOV_VENDORID		0x12
#define ABOV_TSPTA			0x13
#define ABOV_GLOVE			0x13
#define ABOV_KEYBOARD		0x13
#define ABOV_MODEL_NUMBER	0x14		//Model No.
#define ABOV_FLIP			0x15
#define ABOV_SW_RESET		0x1A

/* command */
#define CMD_LED_ON			0x10
#define CMD_LED_OFF			0x20

#define CMD_SAR_TOTALCAP	0x16
#define CMD_SAR_MODE		0x17
#define CMD_SAR_TOTALCAP_READ		0x18
#define CMD_SAR_ENABLE		0x24
#define CMD_SAR_SENSING		0x25
#define CMD_SAR_NOISE_THRESHOLD	0x26
#define CMD_SAR_BASELINE	0x28
#define CMD_SAR_DIFFDATA	0x2A
#define CMD_SAR_RAWDATA		0x2E
#define CMD_SAR_THRESHOLD	0x32

#define CMD_DATA_UPDATE		0x40
#define CMD_MODE_CHECK		0x41
#define CMD_LED_CTRL_ON		0x60
#define CMD_LED_CTRL_OFF	0x70
#define CMD_STOP_MODE		0x80
#define CMD_GLOVE_OFF		0x10
#define CMD_GLOVE_ON		0x20
#define CMD_MOBILE_KBD_OFF	0x10
#define CMD_MOBILE_KBD_ON	0x20
#define CMD_FLIP_OFF		0x10
#define CMD_FLIP_ON			0x20
#define CMD_OFF			0x10
#define CMD_ON			0x20

#define ABOV_BOOT_DELAY		45
#define ABOV_RESET_DELAY	150

#ifdef CONFIG_KEYBOARD_ABOV_TOUCH_T316
#define ABOV_FLASH_MODE		0x31
#else
#define ABOV_FLASH_MODE		0x18
#endif

//static struct device *sec_touchkey;

#ifndef CONFIG_TOUCHKEY_LED
#undef LED_TWINKLE_BOOTING
#endif /* CONFIG_TOUCHKEY_LED */

#ifdef LED_TWINKLE_BOOTING
static void led_twinkle_work(struct work_struct *work);
#endif /* LED_TWINKLE_BOOTING */

#define TK_FW_PATH_BIN "abov/abov_noble.fw"
#define TK_FW_PATH_SDCARD "/sdcard/abov_fw.bin"

#define I2C_M_WR 0		/* for i2c */

enum {
	BUILT_IN = 0,
	SDCARD,
};

#define ABOV_ISP_FIRMUP_ROUTINE	0

extern unsigned int system_rev;
extern struct class *sec_class;
static bool g_ta_connected =0;
static int touchkey_keycode[] = { 0,
	KEY_RECENT, KEY_BACK
#ifdef CONFIG_TOUCHKEY_GRIP
	, KEY_CP_GRIP
#endif
};

struct abov_tk_info {
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct device *dev;
	struct abov_touchkey_platform_data *pdata;
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif
	struct mutex lock;
	struct pinctrl *pinctrl;
	struct pinctrl *pinctrl_det;
	struct pinctrl_state *pins_default;

	const struct firmware *firm_data_bin;
	const u8 *firm_data_ums;
	char phys[32];
	long firm_size;
	int irq;
	u16 menu_s;
	u16 back_s;
	u16 menu_raw;
	u16 back_raw;
#ifdef CONFIG_TOUCHKEY_GRIP
	struct wake_lock touckey_wake_lock;
	u16 grip_p_thd;
	u16 grip_r_thd;
	u16 grip_n_thd;
	u16 grip_s1;
	u16 grip_s2;
	u16 grip_baseline;
	u16 grip_raw1;
	u16 grip_raw2;
	u16 grip_event;
	bool sar_mode;
	bool sar_enable;
	bool sar_enable_off;
	bool sar_sensing;
#endif /* CONFIG_TOUCHKEY_GRIP */
	int (*power) (bool on);
	void (*input_event)(void *data);
	int touchkey_count;
	u8 fw_update_state;
	u8 fw_ver;
	u8 fw_ver_bin;
	u8 fw_model_number;
	u8 checksum_h;
	u8 checksum_h_bin;
	u8 checksum_l;
	u8 checksum_l_bin;
	bool enabled;
	bool glovemode;
	bool keyboard_mode;
	bool flip_mode;
	
	unsigned int key_num;
	bool irq_checked;
	int irq_key_count[MAX_KEY_NUM];
	
#ifdef LED_TWINKLE_BOOTING
	struct delayed_work led_twinkle_work;
	bool led_twinkle_check;
#endif /* LED_TWINKLE_BOOTING */
#ifdef CONFIG_VBUS_NOTIFIER
	struct notifier_block vbus_nb;
#endif
#ifdef CONFIG_TOUCHKEY_LIGHT_EFS
	struct delayed_work efs_open_work;
	int light_version_efs;
	char light_version_full_efs[LIGHT_VERSION_LEN];
	char light_version_full_bin[LIGHT_VERSION_LEN];
	int light_table_crc;
	u8 light_reg;
#endif /* CONFIG_TOUCHKEY_LIGHT_EFS */
};


#ifdef CONFIG_HAS_EARLYSUSPEND
static void abov_tk_early_suspend(struct early_suspend *h);
static void abov_tk_late_resume(struct early_suspend *h);
#endif

#ifdef USE_OPEN_CLOSE
static int abov_tk_input_open(struct input_dev *dev);
static void abov_tk_input_close(struct input_dev *dev);
#endif /* USE_OPEN_CLOSE */

static int abov_tk_i2c_read_checksum(struct abov_tk_info *info);
static void abov_tk_reset(struct abov_tk_info *info);
static void abov_set_ta_status(struct abov_tk_info *info);

#ifdef CONFIG_TOUCHKEY_LED
static int abov_touchkey_led_status;
static int abov_touchled_cmd_reserved;
#endif /* CONFIG_TOUCHKEY_LED */

static int abov_mode_enable(struct i2c_client *client,u8 cmd_reg, u8 cmd)
{
	return i2c_smbus_write_byte_data(client, cmd_reg, cmd);
}

#if ABOV_ISP_FIRMUP_ROUTINE
static void abov_config_gpio_i2c(struct abov_tk_info *info, int onoff)
{
	struct device *i2c_dev = info->client->dev.parent->parent;
	struct pinctrl *pinctrl_i2c;

	if (onoff) {
		pinctrl_i2c = devm_pinctrl_get_select(i2c_dev, "on_i2c");
		if (IS_ERR(pinctrl_i2c))
			input_err(true, &info->client->dev, "%s: Failed to configure i2c pin\n", __func__);
	} else {
		pinctrl_i2c = devm_pinctrl_get_select(i2c_dev, "off_i2c");
		if (IS_ERR(pinctrl_i2c))
			input_err(true, &info->client->dev, "%s: Failed to configure i2c pin\n", __func__);
	}
}
#endif

static int abov_tk_i2c_read(struct i2c_client *client,
		u8 reg, u8 *val, unsigned int len)
{
	struct abov_tk_info *info = i2c_get_clientdata(client);
	struct i2c_msg msg;
	int ret;
	int retry = 3;

	mutex_lock(&info->lock);
	msg.addr = client->addr;
	msg.flags = I2C_M_WR;
	msg.len = 1;
	msg.buf = &reg;
	while (retry--) {
		ret = i2c_transfer(client->adapter, &msg, 1);
		if (ret >= 0)
			break;

		input_err(true, &client->dev, "%s fail(address set)(%d)\n",
			__func__, retry);
		msleep(10);
	}
	if (ret < 0) {
		mutex_unlock(&info->lock);
		return ret;
	}
	retry = 3;
	msg.flags = 1;/*I2C_M_RD*/
	msg.len = len;
	msg.buf = val;
	while (retry--) {
		ret = i2c_transfer(client->adapter, &msg, 1);
		if (ret >= 0) {
			mutex_unlock(&info->lock);
			return 0;
		}
		input_err(true, &client->dev, "%s fail(data read)(%d)\n",
			__func__, retry);
		msleep(10);
	}
	mutex_unlock(&info->lock);
	return ret;
}

static int abov_tk_i2c_read_data(struct i2c_client *client, u8 *val, unsigned int len)
{
	struct abov_tk_info *info = i2c_get_clientdata(client);
	struct i2c_msg msg;
	int ret;
	int retry = 3;

	mutex_lock(&info->lock);
	msg.addr = client->addr;
	msg.flags = 1;/*I2C_M_RD*/
	msg.len = len;
	msg.buf = val;
	while (retry--) {
		ret = i2c_transfer(client->adapter, &msg, 1);
		if (ret >= 0) {
			mutex_unlock(&info->lock);
			return 0;
		}
		dev_err(&client->dev, "%s fail(data read)(%d)\n",
			__func__, retry);
		msleep(10);
	}
	mutex_unlock(&info->lock);
	return ret;
}

static int abov_tk_i2c_write(struct i2c_client *client,
		u8 reg, u8 *val, unsigned int len)
{
	struct abov_tk_info *info = i2c_get_clientdata(client);
	struct i2c_msg msg[1];
	unsigned char data[2];
	int ret;
	int retry = 3;

	mutex_lock(&info->lock);
	data[0] = reg;
	data[1] = *val;
	msg->addr = client->addr;
	msg->flags = I2C_M_WR;
	msg->len = 2;
	msg->buf = data;

	while (retry--) {
		ret = i2c_transfer(client->adapter, msg, 1);
		if (ret >= 0) {
			mutex_unlock(&info->lock);
			return 0;
		}
		input_err(true, &client->dev, "%s fail(%d)\n",
			__func__, retry);
		msleep(10);
	}
	mutex_unlock(&info->lock);
	return ret;
}

#ifdef CONFIG_TOUCHKEY_LIGHT_EFS
static int efs_read_light_table_version(struct abov_tk_info *info);

static void change_touch_key_led_brightness(struct device *dev, int led_reg)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);
	int ret;
	
	input_info(true, dev, "%s: 0x%02x\n", __func__, led_reg);
	info->light_reg = led_reg;

	/*led dimming */
	ret = abov_tk_i2c_write(info->client, ABOV_LED_BACK, &info->light_reg, 1);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s led dimming back key write fail(%d)\n", __func__, ret);
	}

	ret = abov_tk_i2c_write(info->client, ABOV_LED_RECENT, &info->light_reg, 1);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s led dimming recent key write fail(%d)\n", __func__, ret);
	}
}

static int read_window_type(void)
{
	struct file *type_filp = NULL;
	mm_segment_t old_fs;
	int ret = 0;
	char window_type[2] = {0, };

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	type_filp = filp_open("/sys/class/lcd/panel/window_type", O_RDONLY, 0440);
	if (IS_ERR(type_filp)) {
		set_fs(old_fs);
		ret = PTR_ERR(type_filp);
		return ret;
	}

	ret = type_filp->f_op->read(type_filp, window_type,
			sizeof(window_type), &type_filp->f_pos);
	if (ret != 2 * sizeof(char)) {
		pr_err("%s touchkey %s: fd read fail\n", SECLOG, __func__);
		ret = -EIO;
		return ret;
	}

	filp_close(type_filp, current->files);
	set_fs(old_fs);

	if (window_type[1] < '0' || window_type[1] >= 'f')
		return -EAGAIN;

	ret = (window_type[1] - '0') & 0x0f;
	pr_info("%s touchkey %s: %d\n", SECLOG, __func__, ret);
	return ret;
}

static int efs_calculate_crc (struct abov_tk_info *info)
{
	struct file *temp_file = NULL;
	int crc = info->light_version_efs;
	mm_segment_t old_fs;
	char predefine_value_path[LIGHT_TABLE_PATH_LEN];
	int ret = 0, i;
	char temp_vol[LIGHT_CRC_SIZE] = {0, };
	int table_size;

	efs_read_light_table_version(info);
	table_size = (int)strlen(info->light_version_full_efs) - 8;

	for (i = 0; i < table_size; i++) {
		char octa_temp = info->light_version_full_efs[8 + i];
		int octa_temp_i;

		if (octa_temp >= 'A')
			octa_temp_i = octa_temp - 'A' + 0x0A;
		else
			octa_temp_i = octa_temp - '0';
		
		input_info(true, &info->client->dev, "%s: octa %d\n", __func__, octa_temp_i);

		snprintf(predefine_value_path, LIGHT_TABLE_PATH_LEN, "%s%d",
				LIGHT_TABLE_PATH, octa_temp_i);
		old_fs = get_fs();
		set_fs(KERNEL_DS);
		temp_file = filp_open(predefine_value_path, O_RDONLY, 0440);
		if (!IS_ERR(temp_file)) {
			temp_file->f_op->read(temp_file, temp_vol,
					sizeof(temp_vol), &temp_file->f_pos);
			filp_close(temp_file, current->files);
			if (kstrtoint(temp_vol, 0, &ret) < 0) {
				ret = -EIO;
			} else {
				crc += octa_temp_i;
				crc += ret;
				ret = 0;
			}
		}
		set_fs(old_fs);
	}

	if (!ret)
		ret = crc;

	return ret;
}

static int efs_read_crc(struct abov_tk_info *info)
{
	struct file *temp_file = NULL;
	char crc[LIGHT_CRC_SIZE] = {0, };
	mm_segment_t old_fs;
	int ret = 0;
	
	old_fs = get_fs();
	set_fs(KERNEL_DS);
	temp_file = filp_open(LIGHT_CRC_PATH, O_RDONLY, 0440);
	if (IS_ERR(temp_file)) {
		ret = PTR_ERR(temp_file);
		input_info(true, &info->client->dev,
				"%s: failed to open efs file %d\n", __func__, ret);
	} else {
		temp_file->f_op->read(temp_file, crc, sizeof(crc), &temp_file->f_pos);
		filp_close(temp_file, current->files);
		if (kstrtoint(crc, 0, &ret) < 0)
			ret = -EIO;
	}
	set_fs(old_fs);

	return ret;
}

static bool check_light_table_crc(struct abov_tk_info *info)
{
	int crc_efs = efs_read_crc(info);

	if (info->light_version_efs == info->pdata->dt_light_version) {
		/* compare efs crc file with binary crc*/
		input_info(true, &info->client->dev,
				"%s: efs:%d, bin:%d\n",
				__func__, crc_efs, info->light_table_crc);
		if (crc_efs != info->light_table_crc)
			return false;
	}

	return true;
}

static int efs_write_light_table_crc(struct abov_tk_info *info, int crc_cal)
{
	struct file *temp_file = NULL;
	char crc[LIGHT_CRC_SIZE] = {0, };
	mm_segment_t old_fs;
	int ret = 0;

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	temp_file = filp_open(LIGHT_CRC_PATH, O_TRUNC | O_RDWR | O_CREAT, 0660);
	if (IS_ERR(temp_file)) {
		ret = PTR_ERR(temp_file);
		input_info(true, &info->client->dev,
				"%s: failed to open efs file %d\n", __func__, ret);
	} else {
		snprintf(crc, sizeof(crc), "%d", crc_cal);
		temp_file->f_op->write(temp_file, crc, sizeof(crc), &temp_file->f_pos);
		filp_close(temp_file, current->files);
		input_info(true, &info->client->dev, "%s: %s\n", __func__, crc);
	}
	set_fs(old_fs);
	return ret;
}

static int efs_write_light_table_version(struct abov_tk_info *info, char *full_version)
{
	struct file *temp_file = NULL;
	mm_segment_t old_fs;
	int ret = 0;

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	temp_file = filp_open(LIGHT_VERSION_PATH, O_TRUNC | O_RDWR | O_CREAT, 0660);
	if (IS_ERR(temp_file)) {
		ret = -ENOENT;
	} else {
		temp_file->f_op->write(temp_file, full_version,
				LIGHT_VERSION_LEN, &temp_file->f_pos);
		filp_close(temp_file, current->files);
		input_info(true, &info->client->dev, "%s: version = %s\n",
				__func__, full_version);
	}
	set_fs(old_fs);
	return ret;
}

static int efs_write_light_table(struct abov_tk_info *info, struct light_info table)
{
	struct file *type_filp = NULL;
	mm_segment_t old_fs;
	int ret = 0;
	char predefine_value_path[LIGHT_TABLE_PATH_LEN];
	char led_reg[LIGHT_DATA_SIZE] = {0, };

	snprintf(predefine_value_path, LIGHT_TABLE_PATH_LEN,
			"%s%d", LIGHT_TABLE_PATH, table.octa_id);
	snprintf(led_reg, sizeof(led_reg), "%d", table.led_reg);

	input_info(true, &info->client->dev, "%s: make %s\n", __func__, predefine_value_path);

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	type_filp = filp_open(predefine_value_path, O_TRUNC | O_RDWR | O_CREAT, 0660);
	if (IS_ERR(type_filp)) {
		set_fs(old_fs);
		ret = PTR_ERR(type_filp);
		input_err(true, &info->client->dev, "%s: open fail :%d\n",
			__func__, ret);
		return ret;
	}

	type_filp->f_op->write(type_filp, led_reg, sizeof(led_reg), &type_filp->f_pos);
	filp_close(type_filp, current->files);
	set_fs(old_fs);

	return ret;
}

static int efs_write(struct abov_tk_info *info)
{
	int ret = 0;
	int i, crc_cal;

	ret = efs_write_light_table_version(info, info->light_version_full_bin);
	if (ret < 0)
		return ret;
	info->light_version_efs = info->pdata->dt_light_version;

	for (i = 0; i < info->pdata->dt_light_table; i++) {
		ret = efs_write_light_table(info, tkey_light_reg_table[i]);
		if (ret < 0)
			break;
	}
	if (ret < 0)
		return ret;

	crc_cal = efs_calculate_crc(info);
	if (crc_cal < 0)
		return crc_cal;

	ret = efs_write_light_table_crc(info, crc_cal);
	if (ret < 0)
		return ret;

	if (!check_light_table_crc(info))
		ret = -EIO;

	return ret;
}

static int pick_light_table_version(char* str)
{
	static char* str_addr;
	char* token = NULL;
	int ret = 0;
	
	if (str != NULL)
		str_addr = str;
	else if (str_addr == NULL)
		return 0;

	token = str_addr;
	while (true) {
		if (!(*str_addr)) {
			break;
 		} else if (*str_addr == 'T') {
			*str_addr = '0';
		} else if (*str_addr == '.') {
			*str_addr = '\0';
			str_addr = str_addr + 1;
			break;
		}
		str_addr++;
	}

	if (kstrtoint(token + 1, 0, &ret) < 0)
		return 0;

	return ret;
}

static int efs_read_light_table_version(struct abov_tk_info *info)
{
	struct file *temp_file = NULL;
	char version[LIGHT_VERSION_LEN] = {0, };
	mm_segment_t old_fs;
	int ret = 0;
	
	old_fs = get_fs();
	set_fs(KERNEL_DS);
	temp_file = filp_open(LIGHT_VERSION_PATH, O_RDONLY, 0440);
	if (IS_ERR(temp_file)) {
		ret = PTR_ERR(temp_file);
	} else {
		temp_file->f_op->read(temp_file, version, sizeof(version), &temp_file->f_pos);
		filp_close(temp_file, current->files);
		input_info(true, &info->client->dev,
				"%s: table full version = %s\n", __func__, version);
		snprintf(info->light_version_full_efs,
				sizeof(info->light_version_full_efs), version);
		info->light_version_efs = pick_light_table_version(version);
		input_dbg(true, &info->client->dev,
				"%s: table version = %d\n", __func__, info->light_version_efs);
	}
	set_fs(old_fs);

	return ret;
}

static int efs_read_light_table(struct abov_tk_info *info, int octa_id)
{
	struct file *type_filp = NULL;
	mm_segment_t old_fs;
	char predefine_value_path[LIGHT_TABLE_PATH_LEN];
	char led_reg[LIGHT_DATA_SIZE] = {0, };
	int ret;

	snprintf(predefine_value_path, LIGHT_TABLE_PATH_LEN,
		"%s%d", LIGHT_TABLE_PATH, octa_id);

	input_info(true, &info->client->dev, "%s: %s\n", __func__, predefine_value_path);

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	type_filp = filp_open(predefine_value_path, O_RDONLY, 0440);
	if (IS_ERR(type_filp)) {
		set_fs(old_fs);
		ret = PTR_ERR(type_filp);
		input_err(true, &info->client->dev,
				"%s: fail to open light data %d\n", __func__, ret);
		return ret;
	}

	type_filp->f_op->read(type_filp, led_reg, sizeof(led_reg), &type_filp->f_pos);
	filp_close(type_filp, current->files);
	set_fs(old_fs);

	if (kstrtoint(led_reg, 0, &ret) < 0)
		return -EIO;

	return ret;
}

static int efs_read_light_table_with_default(struct abov_tk_info *info, int octa_id)
{
	bool set_default = false;
	int ret;

retry:
	if (set_default)
		octa_id = WINDOW_COLOR_DEFAULT;

	ret = efs_read_light_table(info, octa_id);
	if (ret < 0) {
		if (!set_default) {
			set_default = true;
			goto retry;
		}
	}

	return ret;
}


static bool need_update_light_table(struct abov_tk_info *info)
{
	/* Check version file exist*/
	if (efs_read_light_table_version(info) < 0) {
		return true;
	}

	/* Compare version */
	input_info(true, &info->client->dev,
			"%s: efs:%d, bin:%d\n", __func__,
			info->light_version_efs, info->pdata->dt_light_version);
	if (info->light_version_efs < info->pdata->dt_light_version)
		return true;

	/* Check CRC */
	if (!check_light_table_crc(info)) {
		input_info(true, &info->client->dev,
				"%s: crc is diffrent\n", __func__);
		return true;
	}

	return false;
}

static void touchkey_efs_open_work(struct work_struct *work)
{
	struct abov_tk_info *info =
			container_of(work, struct abov_tk_info, efs_open_work.work);
	int window_type;
	static int count = 0;
	int led_reg;

	if (need_update_light_table(info)) {
		if (efs_write(info) < 0)
			goto out;
	}

	window_type = read_window_type();
	if (window_type < 0)
		goto out;

	led_reg = efs_read_light_table_with_default(info, window_type);
	if ((led_reg >= LIGHT_REG_MIN_VAL) && (led_reg <= LIGHT_REG_MAX_VAL)) {
		change_touch_key_led_brightness(&info->client->dev, led_reg);
		input_info(true, &info->client->dev,
				"%s: read done for window_type=%d\n", __func__, window_type);
	} else {
		input_err(true, &info->client->dev,
				"%s: fail. key led brightness reg is %d\n", __func__, led_reg);
	}
	return;

out:
	if (count < 50) {
		schedule_delayed_work(&info->efs_open_work, msecs_to_jiffies(2000));
		count++;
 	} else {
		input_err(true, &info->client->dev,
				"%s: retry %d times but can't check efs\n", __func__, count);
 	}
}
#endif /* CONFIG_TOUCHKEY_LIGHT_EFS */

#ifdef CONFIG_TOUCHKEY_GRIP
static void abov_sar_olny_mode(struct abov_tk_info *info, int on)
{
	struct i2c_client *client = info->client;
	int retry =3;
	int ret;
	u8 cmd;
	u8 r_buf;
	int mode_retry = 5;

	if(info->sar_mode == on){
		input_info(true, &client->dev, "[TK] %s : skip already %s\n", __func__, (on==1)? "sar only mode":"normal mode");
		return;
	}

	if(on == 1)	cmd = 0x20;
	else	cmd = 0x10;

	input_info(true, &client->dev, "[TK] %s : %s, cmd=%x\n", __func__, (on==1)? "sar only mode":"normal mode", cmd);
sar_mode:
	while(retry>0){
		ret = abov_tk_i2c_write(info->client, CMD_SAR_MODE, &cmd, 1);
		if (ret < 0) {
			input_err(true, &info->client->dev, "%s fail(%d), retry %d\n", __func__, ret, retry);
			retry--;
			msleep(20);
			continue;
		}
		break;
	}

	msleep(40);

	ret = abov_tk_i2c_read(info->client, CMD_SAR_MODE, &r_buf, 1);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s fail(%d)\n", __func__, ret);
	}
	input_info(true, &client->dev, "%s read reg = %x\n", __func__,r_buf);

	if((r_buf != cmd) && (mode_retry > 0)){
		input_info(true, &info->client->dev, "%s change fail retry\n", __func__);
		mode_retry--;

		if(mode_retry == 0){
			abov_tk_reset(info);
		}
		goto sar_mode;
	}

	if(r_buf == 0x20)
		info->sar_mode = 1;
	else if(r_buf == 0x10)
		info->sar_mode = 0;
}

static void touchkey_sar_sensing(struct abov_tk_info *info, int on)
{
	struct i2c_client *client = info->client;
	int ret;
	u8 cmd;

	if(on==1)	cmd = CMD_ON;
	else	cmd = CMD_OFF;

	input_info(true, &client->dev, "[TK] %s : %s\n", __func__, (on)? "on":"off");

	ret = abov_tk_i2c_write(info->client, CMD_SAR_SENSING, &cmd, 1);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s fail(%d)\n", __func__, ret);
	}
	info->sar_sensing = on;
}
#endif /* CONFIG_TOUCHKEY_GRIP */

static void release_all_fingers(struct abov_tk_info *info)
{
	struct i2c_client *client = info->client;
	int i;

	input_info(true, &client->dev, "%s called (touchkey_count=%d)\n", __func__,info->touchkey_count);

	for (i = 1; i < info->touchkey_count; i++) {
		input_report_key(info->input_dev,
			touchkey_keycode[i], 0);
	}
	input_sync(info->input_dev);
}

static int abov_tk_reset_for_bootmode(struct abov_tk_info *info)
{
	int ret=0;

	info->pdata->power(info, false);
	msleep(50);
	info->pdata->power(info, true);

	return ret;

}

static void abov_tk_reset(struct abov_tk_info *info)
{
	struct i2c_client *client = info->client;
#ifdef CONFIG_TOUCHKEY_LIGHT_EFS
	int ret;
#endif

	if (info->enabled == false)
		return;

	input_info(true,&client->dev, "%s start\n", __func__);
	disable_irq_nosync(info->irq);

	info->enabled = false;

	release_all_fingers(info);

	abov_tk_reset_for_bootmode(info);
	msleep(ABOV_RESET_DELAY);

#ifdef CONFIG_TOUCHKEY_LIGHT_EFS
	/*led dimming */
	ret = abov_tk_i2c_write(info->client, ABOV_LED_BACK, &info->light_reg, 1);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s led dimming back key write fail(%d)\n", __func__, ret);
	}

	ret = abov_tk_i2c_write(info->client, ABOV_LED_RECENT, &info->light_reg, 1);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s led dimming recent key write fail(%d)\n", __func__, ret);
	}
#endif /* CONFIG_TOUCHKEY_LIGHT_EFS */

#ifdef CONFIG_TOUCHKEY_GRIP
	if(info->sar_enable)
		abov_mode_enable(client, CMD_SAR_ENABLE, CMD_ON);

	if(info->sar_sensing != 1)
		touchkey_sar_sensing(info, 0);
#else
	if (info->pdata->ta_notifier && g_ta_connected) {
		abov_set_ta_status(info);
	}
	if (info->flip_mode){
		abov_mode_enable(client, ABOV_FLIP, CMD_FLIP_ON);
	} else {
		if (info->glovemode)
			abov_mode_enable(client, ABOV_GLOVE, CMD_GLOVE_ON);
	}
	if (info->keyboard_mode)
		abov_mode_enable(client, ABOV_KEYBOARD, CMD_MOBILE_KBD_ON);
#endif
	info->enabled = true;

	enable_irq(info->irq);
	input_info(true,&client->dev, "%s end\n", __func__);
}

static irqreturn_t abov_tk_interrupt(int irq, void *dev_id)
{
	struct abov_tk_info *info = dev_id;
	struct i2c_client *client = info->client;
	int ret, retry;
	u8 buf;
#ifdef CONFIG_TOUCHKEY_GRIP
	int grip_data;
	u8 grip_press = 0;

	wake_lock(&info->touckey_wake_lock);
#endif

	ret = abov_tk_i2c_read(client, ABOV_BTNSTATUS_NEW, &buf, 1);
	if (ret < 0) {
		retry = 3;
		while (retry--) {
			input_err(true, &client->dev, "%s read fail(%d)\n",
				__func__, retry);
			ret = abov_tk_i2c_read(client, ABOV_BTNSTATUS_NEW, &buf, 1);
			if (ret == 0)
				break;
			else
				msleep(10);
		}
		if (retry == 0) {
			abov_tk_reset(info);
#ifdef CONFIG_TOUCHKEY_GRIP
			wake_unlock(&info->touckey_wake_lock);
#endif
			return IRQ_HANDLED;
		}
	}

	input_info(true, &client->dev, " %s buf = 0x%02x\n",__func__, buf);

	{
		int menu_data = buf & 0x03;
		int back_data = (buf >> 2) & 0x03;
		u8 menu_press = !(menu_data % 2);
		u8 back_press = !(back_data % 2);

		if(info->irq_checked){
			if(menu_data==2)
			info->irq_key_count[0]++;
		    else if(back_data==2)
			info->irq_key_count[1]++;	
		}

#ifdef CONFIG_TOUCHKEY_GRIP
		grip_data = (buf >> 4) & 0x03;
		grip_press = !(grip_data % 2);
#endif
		if (menu_data)
			input_report_key(info->input_dev,
				touchkey_keycode[1], menu_press);
		if (back_data)
			input_report_key(info->input_dev,
				touchkey_keycode[2], back_press);
#ifdef CONFIG_TOUCHKEY_GRIP
		if (grip_data){
			input_report_key(info->input_dev,
				touchkey_keycode[3], grip_press);
			info->grip_event =  grip_press;
		}
#endif
#ifdef CONFIG_SAMSUNG_PRODUCT_SHIP
		input_info(true, &client->dev,
			"keycode : %s%s ver 0x%02x\n",
			menu_data ? (menu_press ? "P" : "R") : "",
			back_data ? (back_press ? "P" : "R") : "",
			info->fw_ver);
#else
		input_info(true, &client->dev,
			"keycode : %s%s%x ver 0x%02x\n",
			menu_data ? (menu_press ? "menu P " : "menu R ") : "",
			back_data ? (back_press ? "back P " : "back R ") : "",
			buf, info->fw_ver);
#endif
#ifdef CONFIG_TOUCHKEY_GRIP
		if (grip_data){
			input_info(true, &client->dev, "%s%x \n",
				grip_press ? "grip P " : "grip R ",	buf);
		}
#endif
	}

	input_sync(info->input_dev);

#ifdef CONFIG_TOUCHKEY_GRIP
	wake_unlock(&info->touckey_wake_lock);
#endif
	return IRQ_HANDLED;

}

#ifdef CONFIG_TOUCHKEY_LED
static int touchkey_led_set(struct abov_tk_info *info, int data)
{
	u8 cmd;
	int ret;

	if (data == 1)
		cmd = CMD_LED_ON;
	else
		cmd = CMD_LED_OFF;

	if (!info->enabled) {
		abov_touchled_cmd_reserved = 1;
		return 1;
	}

	ret = abov_tk_i2c_write(info->client, ABOV_BTNSTATUS, &cmd, 1);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s fail(%d)\n", __func__, ret);
		abov_touchled_cmd_reserved = 1;
		return 1;
	}

	return 0;
}

static ssize_t touchkey_led_control(struct device *dev,
		 struct device_attribute *attr, const char *buf,
		 size_t count)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);
	int data;
	u8 cmd;
	int ret;

	ret = sscanf(buf, "%d", &data);
	if (ret != 1) {
		input_err(true, &info->client->dev, "%s: cmd read err\n", __func__);
		return count;
	}

	if (!(data == 0 || data == 1)) {
		input_err(true, &info->client->dev, "%s: wrong command(%d)\n",
			__func__, data);
		return count;
	}

	if (data == 1)
		cmd = CMD_LED_ON;
	else
		cmd = CMD_LED_OFF;

#ifdef LED_TWINKLE_BOOTING
	if(info->led_twinkle_check == 1){
		info->led_twinkle_check = 0;
		cancel_delayed_work(&info->led_twinkle_work);
	}
#endif /* LED_TWINKLE_BOOTING */

	if(touchkey_led_set(info, data))
		goto out;

	msleep(20);

	abov_touchled_cmd_reserved = 0;
	input_info(true, &info->client->dev, "%s data(%d)\n",__func__,data);

out:
	abov_touchkey_led_status =  cmd;

	return count;
}
#endif /* CONFIG_TOUCHKEY_LED */

#ifdef CONFIG_TOUCHKEY_GRIP
static ssize_t touchkey_sar_enable(struct device *dev,
		 struct device_attribute *attr, const char *buf,
		 size_t count)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);
	struct i2c_client *client = info->client;
	int data;
	int ret;
	u8 cmd;

	ret = sscanf(buf, "%d", &data);
	if (ret != 1) {
		dev_err(&client->dev, "%s: cmd read err\n", __func__);
		return count;
	}

	if (!(data >= 0 && data <= 3)) {
		dev_err(&client->dev, "%s: wrong command(%d)\n",
			__func__, data);
		return count;
	}

	/*data 0:0ff, 1:on, 2:force off, 3:force off->on  */

	if(data == 3){
		info->sar_enable_off = 0;
		input_info(true, &info->client->dev, "%s : Power back off _ force off -> on (%d)\n", __func__, info->sar_enable);
		if(info->sar_enable)
			data = 1;
		else
			return count;
	}

	if(info->sar_enable_off){
		if(data == 1)
			info->sar_enable = true;
		else
			info->sar_enable = false;
		input_info(true, &info->client->dev, "%s skip, Power back off _ force off mode (%d)\n", __func__, info->sar_enable);
		return count;
	}

	if(data == 1)
		cmd = 0x20;
	else if(data == 2){			//test app : Power back off _ force off
		cmd = 0x10;
		info->sar_enable_off = 1;
	}
	else cmd = 0x10;

	input_info(true, &info->client->dev, "%s data(%d)\n",__func__,data);

	ret = abov_mode_enable(client, CMD_SAR_ENABLE, cmd);
	if (ret < 0) {
		dev_err(&info->client->dev, "%s fail(%d)\n", __func__, ret);
		return count;
	}

	if(data == 1)
		info->sar_enable = true;
	else{
		input_report_key(info->input_dev, touchkey_keycode[3], 0);
		info->grip_event = 0;
		info->sar_enable = false;
	}

	dev_notice(&client->dev, "%s data(%d)\n",__func__,data);

	return count;
}
#endif

static ssize_t touchkey_threshold_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);
	struct i2c_client *client = info->client;
	u8 r_buf;
	int ret;

	ret = abov_tk_i2c_read(client, ABOV_THRESHOLD, &r_buf, 1);
	if (ret < 0) {
		input_err(true, &client->dev, "%s fail(%d)\n", __func__, ret);
		r_buf = 0;
	}
	return sprintf(buf, "%d\n", r_buf);
}

static void get_diff_data(struct abov_tk_info *info)
{
	struct i2c_client *client = info->client;
	u8 r_buf[4];
	int ret;

	ret = abov_tk_i2c_read(client, ABOV_DIFFDATA, r_buf, 4);
	if (ret < 0) {
		input_err(true, &client->dev, "%s fail(%d)\n", __func__, ret);
		info->menu_s = 0;
		info->back_s = 0;
		return;
	}

	info->menu_s = (r_buf[0] << 8) | r_buf[1];
	info->back_s = (r_buf[2] << 8) | r_buf[3];
}

static ssize_t touchkey_menu_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);

	get_diff_data(info);

	return sprintf(buf, "%d\n", info->menu_s);
}

static ssize_t touchkey_back_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);

	get_diff_data(info);

	return sprintf(buf, "%d\n", info->back_s);
}

static void get_raw_data(struct abov_tk_info *info)
{
	struct i2c_client *client = info->client;
	u8 r_buf[4];
	int ret;

	ret = abov_tk_i2c_read(client, ABOV_RAWDATA, r_buf, 4);
	if (ret < 0) {
		input_err(true, &client->dev, "%s fail(%d)\n", __func__, ret);
		info->menu_raw = 0;
		info->back_raw = 0;
		return;
	}

	info->menu_raw = (r_buf[0] << 8) | r_buf[1];
	info->back_raw = (r_buf[2] << 8) | r_buf[3];
}

static ssize_t touchkey_menu_raw_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);

	get_raw_data(info);

	return sprintf(buf, "%d\n", info->menu_raw);
}

static ssize_t touchkey_back_raw_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);

	get_raw_data(info);

	return sprintf(buf, "%d\n", info->back_raw);
}

#ifdef CONFIG_TOUCHKEY_GRIP
static ssize_t touchkey_grip_threshold_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);

	u8 r_buf[4];
	int ret;

	ret = abov_tk_i2c_read(info->client, CMD_SAR_THRESHOLD, r_buf, 4);
	if (ret < 0) {
		dev_err(&info->client->dev, "%s fail(%d)\n", __func__, ret);
		info->grip_p_thd = 0;
		info->grip_r_thd = 0;
		return sprintf(buf, "%d\n", 0);
	}
	info->grip_p_thd = (r_buf[0] << 8) | r_buf[1];
	info->grip_r_thd = (r_buf[2] << 8) | r_buf[3];

	ret = abov_tk_i2c_read(info->client, CMD_SAR_NOISE_THRESHOLD, r_buf, 2);
	if (ret < 0) {
		dev_err(&info->client->dev, "%s fail(%d)\n", __func__, ret);
		info->grip_n_thd = 0;
		return sprintf(buf, "%d\n", 0);
	}
	info->grip_n_thd = (r_buf[0] << 8) | r_buf[1];

	return sprintf(buf, "%d,%d,%d\n", info->grip_p_thd, info->grip_r_thd, info->grip_n_thd );
}
static ssize_t touchkey_total_cap_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);
	u8 r_buf[2];
	u8 cmd;
	int ret;
	int value;

	cmd = 0x20;
	ret = abov_tk_i2c_write(info->client, CMD_SAR_TOTALCAP, &cmd, 1);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s write fail(%d)\n", __func__, ret);
	}

	usleep_range(10, 10);

	ret = abov_tk_i2c_read(info->client, CMD_SAR_TOTALCAP_READ, r_buf, 2);
	if (ret < 0) {
		dev_err(&info->client->dev, "%s fail(%d)\n", __func__, ret);
		return sprintf(buf, "%d\n", 0);
	}
	value = (r_buf[0] << 8) | r_buf[1];

	return sprintf(buf, "%d\n", value/100);
}
static ssize_t touchkey_grip_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);

	u8 r_buf[4];
	int ret;

	ret = abov_tk_i2c_read(info->client, CMD_SAR_DIFFDATA, r_buf, 4);
	if (ret < 0) {
		dev_err(&info->client->dev, "%s fail(%d)\n", __func__, ret);
		info->grip_s1 = 0;
		info->grip_s2 = 0;
		return sprintf(buf, "%d\n", 0);
	}
	info->grip_s1 = (r_buf[0] << 8) | r_buf[1];
	info->grip_s2 = (r_buf[2] << 8) | r_buf[3];


	return sprintf(buf, "%d,%d\n", info->grip_s1, info->grip_s2);
}

static ssize_t touchkey_grip_baseline_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);

	u8 r_buf[2];
	int ret;

	ret = abov_tk_i2c_read(info->client, CMD_SAR_BASELINE, r_buf, 2);
	if (ret < 0) {
		dev_err(&info->client->dev, "%s fail(%d)\n", __func__, ret);
		info->grip_baseline = 0;
		return sprintf(buf, "%d\n", 0);
	}
	info->grip_baseline = (r_buf[0] << 8) | r_buf[1];

	return sprintf(buf, "%d\n", info->grip_baseline);
}

static ssize_t touchkey_grip_raw_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);

	u8 r_buf[4];
	int ret;

	ret = abov_tk_i2c_read(info->client, CMD_SAR_RAWDATA, r_buf, 4);
	if (ret < 0) {
		dev_err(&info->client->dev, "%s fail(%d)\n", __func__, ret);
		info->grip_raw1 = 0;
		info->grip_raw2 = 0;
		return sprintf(buf, "%d\n", 0);
	}
	info->grip_raw1 = (r_buf[0] << 8) | r_buf[1];
	info->grip_raw2 = 0;	//(r_buf[2] << 8) | r_buf[3]; NA

	return sprintf(buf, "%d,%d\n", info->grip_raw1, info->grip_raw2);
}

static ssize_t touchkey_grip_gain_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d,%d,%d,%d\n", 0, 0, 0, 0);
}

static ssize_t touchkey_grip_check_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);

	//dev_info(&info->client->dev, "%s event:%d\n", __func__, info->grip_event);

	return sprintf(buf, "%d\n", info->grip_event);
}

static ssize_t touchkey_grip_sw_reset(struct device *dev,
		 struct device_attribute *attr, const char *buf,
		 size_t count)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);
	struct i2c_client *client = info->client;
	int data;
	int ret;
	u8 cmd = 0x10;

	ret = sscanf(buf, "%d", &data);
	if (ret != 1) {
		dev_err(&client->dev, "%s: cmd read err\n", __func__);
		return count;
	}

	if (!(data == 1)) {
		dev_err(&client->dev, "%s: wrong command(%d)\n",
			__func__, data);
		return count;
	}

	ret = abov_tk_i2c_write(info->client, ABOV_SW_RESET, &cmd, 1);
	if (ret < 0) {
		dev_err(&info->client->dev, "%s fail(%d)\n", __func__, ret);
		return count;
	}

	input_info(true, &info->client->dev, "%s data(%d)\n",__func__,data);

	return count;
}

static ssize_t touchkey_sensing_change(struct device *dev,
		 struct device_attribute *attr, const char *buf,
		 size_t count)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);
	struct i2c_client *client = info->client;
	int ret, data;

	ret = sscanf(buf, "%d", &data);
	if (ret != 1) {
		dev_err(&client->dev, "%s: cmd read err\n", __func__);
		return count;
	}

	if (!(data == 0 || data == 1)) {
		dev_err(&client->dev, "%s: wrong command(%d)\n",
			__func__, data);
		return count;
	}

	if(data == 1)	//EarJack inserted
		touchkey_sar_sensing(info, 0);
	else
		touchkey_sar_sensing(info, 1);

	input_info(true, &info->client->dev, "%s earjack (%d)\n",__func__,data);

	return count;
}

#ifndef CONFIG_SAMSUNG_PRODUCT_SHIP
static ssize_t touchkey_sar_press_threshold_store(struct device *dev,
		 struct device_attribute *attr, const char *buf,
		 size_t count)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);

	int ret;
	int threshold;
	u8 cmd[2];

	ret = sscanf(buf, "%d", &threshold);
	if (ret != 1) {
		input_err(true, &info->client->dev, "%s: failed to read thresold, buf is %s\n", __func__,buf);
		return count;
	}

	if(threshold > 0xff) {
		cmd[0] = (threshold >> 8) & 0xff;
		cmd[1] = 0xff & threshold;
	}else if(threshold < 0) {
		cmd[0] = 0x0;
		cmd[1] = 0x0;
	}else{
		cmd[0] = 0x0;
		cmd[1] = (u8)threshold;
	}

	input_info(true,&info->client->dev, "%s buf : %d, threshold : %d\n",
		__func__, threshold,(cmd[0]<<8 )| cmd[1]);

	ret = abov_tk_i2c_write(info->client, CMD_SAR_THRESHOLD, &cmd[0], 1);
	if (ret != 0) {
		input_info(true,&info->client->dev, "%s failed to write press_threhold data1", __func__);
		goto press_threshold_out;
	}
	ret = abov_tk_i2c_write(info->client, CMD_SAR_THRESHOLD + 0x01, &cmd[1], 1);
	if (ret != 0) {
		input_info(true,&info->client->dev, "%s failed to write press_threhold data2", __func__);
		goto press_threshold_out;
	}
press_threshold_out:
	return count;
}

static ssize_t touchkey_sar_release_threshold_store(struct device *dev,
		 struct device_attribute *attr, const char *buf,
		 size_t count)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);

	int ret;
	int threshold;
	u8 cmd[2];

	ret = sscanf(buf, "%d", &threshold);
	if (ret != 1) {
		input_err(true, &info->client->dev, "%s: failed to read thresold, buf is %s\n", __func__,buf);
		return count;
	}

	if(threshold > 0xff) {
		cmd[0] = (threshold >> 8) & 0xff;
		cmd[1] = 0xff & threshold;
	}else if(threshold < 0) {
		cmd[0] = 0x0;
		cmd[1] = 0x0;
	}else{
		cmd[0] = 0x0;
		cmd[1] = (u8)threshold;
	}

	input_info(true,&info->client->dev, "%s buf : %d, threshold : %d\n",
		__func__, threshold,(cmd[0] << 8) | cmd[1]);

	ret = abov_tk_i2c_write(info->client, CMD_SAR_THRESHOLD+0x02, &cmd[0], 1);
	input_info(true,&info->client->dev, "%s ret : %d\n", __func__,ret);

	if (ret != 0) {
		input_info(true,&info->client->dev, "%s failed to write release_threshold_data1", __func__);
		goto release_threshold_out;
	}
	ret = abov_tk_i2c_write(info->client, CMD_SAR_THRESHOLD+0x03, &cmd[1], 1);
	input_info(true,&info->client->dev, "%s ret : %d\n", __func__,ret);
	if (ret != 0) {
		input_info(true,&info->client->dev, "%s failed to write release_threshold_data2", __func__);
		goto release_threshold_out;
	}
release_threshold_out:
	return count;
}

static ssize_t touchkey_mode_change(struct device *dev,
		 struct device_attribute *attr, const char *buf,
		 size_t count)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);
	struct i2c_client *client = info->client;
	int ret, data;

	ret = sscanf(buf, "%d", &data);
	if (ret != 1) {
		dev_err(&client->dev, "%s: cmd read err\n", __func__);
		return count;
	}

	if (!(data == 0 || data == 1)) {
		dev_err(&client->dev, "%s: wrong command(%d)\n",
			__func__, data);
		return count;
	}

	input_info(true, &info->client->dev, "%s data(%d)\n",__func__,data);

	abov_sar_olny_mode(info, data);

	return count;
}
#endif

#endif

static ssize_t touchkey_chip_name(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);
	struct i2c_client *client = info->client;

	input_dbg(true, &client->dev, "%s\n", __func__);
#ifdef CONFIG_KEYBOARD_ABOV_TOUCH_T316
	return sprintf(buf, "A96T316\n");
#else
	return sprintf(buf, "FT1804\n");
#endif
}

static ssize_t bin_fw_ver(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);
	struct i2c_client *client = info->client;

	input_dbg(true, &client->dev, "fw version bin : 0x%x\n", info->fw_ver_bin);

	return sprintf(buf, "0x%02x\n", info->fw_ver_bin);
}

static int get_tk_fw_version(struct abov_tk_info *info, bool bootmode)
{
	struct i2c_client *client = info->client;
	u8 buf;
	int ret;
	int retry = 3;

	ret = abov_tk_i2c_read(client, ABOV_FW_VER, &buf, 1);
	if (ret < 0) {
		while (retry--) {
			input_err(true, &client->dev, "%s read fail(%d)\n",
				__func__, retry);
			if (!bootmode)
				abov_tk_reset(info);
			else
				return -1;
			ret = abov_tk_i2c_read(client, ABOV_FW_VER, &buf, 1);
			if (ret == 0)
				break;
		}
		if (retry <= 0)
			return -1;
	}

	info->fw_ver = buf;
	input_info(true, &client->dev, "%s : 0x%x\n", __func__, buf);
	return 0;
}

static ssize_t read_fw_ver(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);
	struct i2c_client *client = info->client;
	int ret;

	ret = get_tk_fw_version(info, false);
	if (ret < 0) {
		input_err(true, &client->dev, "%s read fail\n", __func__);
		info->fw_ver = 0;
	}

	return sprintf(buf, "0x%02x\n", info->fw_ver);
}

static int abov_load_fw(struct abov_tk_info *info, u8 cmd)
{
	struct i2c_client *client = info->client;
	struct file *fp;
	mm_segment_t old_fs;
	long fsize, nread;
	int ret = 0;

	switch(cmd) {
	case BUILT_IN:
		ret = request_firmware(&info->firm_data_bin,
			info->pdata->fw_path, &client->dev);
		if (ret) {
			input_err(true, &client->dev,
				"%s request_firmware fail(%d)\n", __func__, cmd);
			return ret;
		}
		/* Header info
		* 0x00 0x91 : model info,
		* 0x00 0x00 : module info (Rev 0.0),
		* 0x00 0xF3 : F/W
		* 0x00 0x00 0x17 0x10 : checksum
		* ~ 22byte 0x00 */
		info->fw_model_number = info->firm_data_bin->data[1];
		info->fw_ver_bin = info->firm_data_bin->data[5];
		info->checksum_h_bin = info->firm_data_bin->data[8];
		info->checksum_l_bin = info->firm_data_bin->data[9];
		info->firm_size = info->firm_data_bin->size;

		input_info(true, &client->dev, "%s, bin version:%2X,%2X,%2X   crc:%2X,%2X\n", __func__, \
			info->firm_data_bin->data[1], info->firm_data_bin->data[3], info->fw_ver_bin, \
			info->checksum_h_bin, info->checksum_l_bin);
		break;

	case SDCARD:
		old_fs = get_fs();
		set_fs(get_ds());
		fp = filp_open(TK_FW_PATH_SDCARD, O_RDONLY, S_IRUSR);
		if (IS_ERR(fp)) {
			input_err(true, &client->dev,
				"%s %s open error\n", __func__, TK_FW_PATH_SDCARD);
			ret = -ENOENT;
			goto fail_sdcard_open;
		}

		fsize = fp->f_path.dentry->d_inode->i_size;
		info->firm_data_ums = kzalloc((size_t)fsize, GFP_KERNEL);
		if (!info->firm_data_ums) {
			input_err(true, &client->dev,
				"%s fail to kzalloc for fw\n", __func__);
			ret = -ENOMEM;
			goto fail_sdcard_kzalloc;
		}

		nread = vfs_read(fp,
			(char __user *)info->firm_data_ums, fsize, &fp->f_pos);
		if (nread != fsize) {
			input_err(true, &client->dev,
				"%s fail to vfs_read file\n", __func__);
			ret = -EINVAL;
			goto fail_sdcard_size;
		}
		filp_close(fp, current->files);
		set_fs(old_fs);

		info->firm_size = nread;
		info->checksum_h_bin = info->firm_data_ums[8];
		info->checksum_l_bin = info->firm_data_ums[9];

		input_info(true, &client->dev,"%s, bin version:%2X,%2X,%2X   crc:%2X,%2X\n", __func__, \
			info->firm_data_ums[1], info->firm_data_ums[3], info->firm_data_ums[5], \
			info->checksum_h_bin, info->checksum_l_bin);
		break;

	default:
		ret = -1;
		break;
	}
	input_info(true, &client->dev, "fw_size : %lu\n", info->firm_size);
	input_info(true, &client->dev, "%s success\n", __func__);
	return ret;

fail_sdcard_size:
	kfree(&info->firm_data_ums);
fail_sdcard_kzalloc:
	filp_close(fp, current->files);
fail_sdcard_open:
	set_fs(old_fs);
	return ret;
}

#if ABOV_ISP_FIRMUP_ROUTINE
void abov_i2c_start(int scl, int sda)
{
	gpio_direction_output(sda, 1);
	gpio_direction_output(scl, 1);
	usleep_range(15, 17);
	gpio_direction_output(sda, 0);
	usleep_range(10, 12);
	gpio_direction_output(scl, 0);
	usleep_range(10, 12);
}

void abov_i2c_stop(int scl, int sda)
{
	gpio_direction_output(scl, 0);
	usleep_range(10, 12);
	gpio_direction_output(sda, 0);
	usleep_range(10, 12);
	gpio_direction_output(scl, 1);
	usleep_range(10, 12);
	gpio_direction_output(sda, 1);
}

void abov_testdelay(void)
{
	u8 i;
	u8 delay;

	/* 120nms */
	for (i = 0; i < 15; i++)
		delay = 0;
}


void abov_byte_send(u8 data, int scl, int sda)
{
	u8 i;
	for (i = 0x80; i != 0; i >>= 1) {
		gpio_direction_output(scl, 0);
		usleep_range(1,1);

		if (data & i)
			gpio_direction_output(sda, 1);
		else
			gpio_direction_output(sda, 0);

		usleep_range(1,1);
		gpio_direction_output(scl, 1);
		usleep_range(1,1);
	}
	usleep_range(1,1);

	gpio_direction_output(scl, 0);
	gpio_direction_input(sda);
	usleep_range(1,1);

	gpio_direction_output(scl, 1);
	usleep_range(1,1);

	gpio_get_value(sda);
	abov_testdelay();

	gpio_direction_output(scl, 0);
	gpio_direction_output(sda, 0);
	usleep_range(20,20);
}

u8 abov_byte_read(bool type, int scl, int sda)
{
	u8 i;
	u8 data = 0;
	u8 index = 0x7;

	gpio_direction_output(scl, 0);
	gpio_direction_input(sda);
	usleep_range(1,1);

	for (i = 0; i < 8; i++) {
		gpio_direction_output(scl, 0);
		usleep_range(1,1);
		gpio_direction_output(scl, 1);
		usleep_range(1,1);

		data = data | (u8)(gpio_get_value(sda) << index);
		index -= 1;
	}
		usleep_range(1,1);
	gpio_direction_output(scl, 0);

	gpio_direction_output(sda, 0);
		usleep_range(1,1);

	if (type) { /*ACK */
		gpio_direction_output(sda, 0);
		usleep_range(1,1);
		gpio_direction_output(scl, 1);
		usleep_range(1,1);
		gpio_direction_output(scl, 0);
		usleep_range(1,1);
	} else { /* NAK */
		gpio_direction_output(sda, 1);
		usleep_range(1,1);
		gpio_direction_output(scl, 1);
		usleep_range(1,1);
		gpio_direction_output(scl, 0);
		usleep_range(1,1);
		gpio_direction_output(sda, 0);
		usleep_range(1,1);
	}
	usleep_range(20,20);

	return data;
}

void abov_enter_mode(int scl, int sda)
{
	abov_i2c_start(scl, sda);
	abov_testdelay();
	abov_byte_send(ABOV_ID, scl, sda);
	abov_byte_send(0xAC, scl, sda);
	abov_byte_send(0x5B, scl, sda);
	abov_byte_send(0x2D, scl, sda);
	abov_i2c_stop(scl, sda);
}

void abov_firm_write(const u8 *fw_data, int block, int scl, int sda)
{
	int i, j;
	u16 pos = 0x20;
	u8 addr[2];
	u8 data[32] = {0, };

	addr[0] = 0x10;
	addr[1] = 0x00;
	for (i = 0; i < (block - 0x20); i++) {
		if (i % 8 == 0) {
			addr[0] = 0x10 + i/8;
			addr[1] = 0;
		} else
			addr[1] = addr[1] + 0x20;
		memcpy(data, fw_data + pos, 32);
		abov_i2c_start(scl, sda);
		abov_testdelay();
		abov_byte_send(ABOV_ID, scl, sda);
		abov_byte_send(0xAC, scl, sda);
		abov_byte_send(0x7A, scl, sda);
		abov_byte_send(addr[0], scl, sda);
		abov_byte_send(addr[1], scl, sda);
		for (j = 0; j < 32; j++)
			abov_byte_send(data[j], scl, sda);
		abov_i2c_stop(scl, sda);

		pos += 0x20;

		usleep_range(3000,3000);; //usleep(2000); //msleep(2);
	}
}

void abov_read_address_set(int scl, int sda)
{
		abov_i2c_start(scl, sda);
		abov_testdelay();
		abov_byte_send(ABOV_ID, scl, sda);
		abov_byte_send(0xAC, scl, sda);
		abov_byte_send(0x9E, scl, sda);
		abov_byte_send(0x10, scl, sda); /* start addr H */
		abov_byte_send(0x00, scl, sda); /* start addr L */
		abov_byte_send(0x3F, scl, sda); /* end addr H  */
		abov_byte_send(0xFF, scl, sda); /* end addr L  */
		abov_i2c_stop(scl, sda);
}

void abov_checksum(struct abov_tk_info *info, int scl, int sda)
{
	struct i2c_client *client = info->client;

	u8 status;
	u8 bootver;
	u8 firmver;
	u8 checksumh;
	u8 checksuml;

	abov_read_address_set(scl, sda);
	msleep(5);

	abov_i2c_start(scl, sda);
	abov_testdelay();
	abov_byte_send(ABOV_ID, scl, sda);
	abov_byte_send(0x00, scl, sda);

	abov_i2c_start(scl, sda); /* restart */
	abov_testdelay();
	abov_byte_send(ABOV_ID + 1, scl, sda);
	status = abov_byte_read(true, scl, sda);
	bootver = abov_byte_read(true, scl, sda);
	firmver = abov_byte_read(true, scl, sda);
	checksumh = abov_byte_read(true, scl, sda);
	checksuml = abov_byte_read(false, scl, sda);
	abov_i2c_stop(scl, sda);
	msleep(3);

	info->checksum_h = checksumh;
	info->checksum_l = checksuml;

	input_dbg(true, &client->dev,
		"%s status(0x%x), boot(0x%x), firm(0x%x), cs_h(0x%x), cs_l(0x%x)\n",
		__func__, status, bootver, firmver, checksumh, checksuml);
}

void abov_exit_mode(int scl, int sda)
{
	abov_i2c_start(scl, sda);
	abov_testdelay();
	abov_byte_send(ABOV_ID, scl, sda);
	abov_byte_send(0xAC, scl, sda);
	abov_byte_send(0x5B, scl, sda);
	abov_byte_send(0xE1, scl, sda);
	abov_i2c_stop(scl, sda);
}

static int abov_fw_update(struct abov_tk_info *info,
				const u8 *fw_data, int block, int scl, int sda)
{
	input_dbg(true, &info->client->dev, "%s start (%d)\n",__func__,__LINE__);
	abov_config_gpio_i2c(info, 0);
	msleep(ABOV_BOOT_DELAY);
	abov_enter_mode(scl, sda);
	msleep(1100); //msleep(600);
	abov_firm_write(fw_data, block, scl, sda);
	abov_checksum(info, scl, sda);
	abov_config_gpio_i2c(info, 1);
	input_dbg(true, &info->client->dev, "%s end (%d)\n",__func__,__LINE__);
	return 0;
}
#endif

static int abov_tk_check_busy(struct abov_tk_info *info)
{
	int ret, count = 0;
	unsigned char val = 0x00;

	do {
		ret = i2c_master_recv(info->client, &val, sizeof(val));

		if (val & 0x01) {
			count++;
			if (count > 1000)
				input_err(true, &info->client->dev,
					"%s: busy %d\n", __func__, count);
				return ret;
		} else {
			break;
		}

	} while(1);

	return ret;
}

static int abov_tk_i2c_read_checksum(struct abov_tk_info *info)
{
	unsigned char data[6] = {0xAC, 0x9E, 0x10, 0x00, 0x3F, 0xFF};
	unsigned char data2[1] = {0x00};
	unsigned char checksum[6] = {0, };
	int ret;

	i2c_master_send(info->client, data, 6);

	usleep_range(5000, 5000);

	i2c_master_send(info->client, data2, 1);
	usleep_range(5000, 5000);

	ret = abov_tk_i2c_read_data(info->client, checksum, 6);

	input_info(true, &info->client->dev, "%s: ret:%d [%X][%X][%X][%X][%X]\n",
			__func__, ret, checksum[0], checksum[1], checksum[2]
			, checksum[4], checksum[5]);
	info->checksum_h = checksum[4];
	info->checksum_l = checksum[5];
	return 0;
}

static int abov_tk_fw_write(struct abov_tk_info *info, unsigned char *addrH,
						unsigned char *addrL, unsigned char *val)
{
	int length = 36, ret = 0;
	unsigned char data[36];

	data[0] = 0xAC;
	data[1] = 0x7A;
	memcpy(&data[2], addrH, 1);
	memcpy(&data[3], addrL, 1);
	memcpy(&data[4], val, 32);

	ret = i2c_master_send(info->client, data, length);
	if (ret != length) {
		input_err(true, &info->client->dev,
			"%s: write fail[%x%x], %d\n", __func__, *addrH, *addrL, ret);
		return ret;
	}

	usleep_range(3000, 3000);

	abov_tk_check_busy(info);

	return 0;
}

static int abov_tk_fw_mode_enter(struct abov_tk_info *info)
{
	unsigned char data[2] = {0xAC, 0x5B};
	int ret = 0;

	ret = i2c_master_send(info->client, data, 2);
	if (ret != 2) {
		pr_err("%s: write fail\n", __func__);
		return -1;
	}

	return 0;

}


static int abov_tk_fw_mode_check(struct abov_tk_info *info)
{
	unsigned char buf[1] = {0};
	int ret;

	ret = abov_tk_i2c_read_data(info->client, buf, 1);
	if(ret<0){
		pr_err("%s: write fail\n", __func__);
		return 0;
	}

	dev_info(&info->client->dev, "%s: ret:%02X\n",__func__, buf[0]);

#ifdef CONFIG_KEYBOARD_ABOV_TOUCH_T316	//temp
	if((buf[0]==ABOV_FLASH_MODE) || (buf[0]==0x18))
		return 1;
#else
	if(buf[0]==ABOV_FLASH_MODE)	
		return 1;
#endif

	pr_err("%s: value is same same,  %X, %X \n", __func__, ABOV_FLASH_MODE, buf[0] );

	return 0;
}

static int abov_tk_flash_erase(struct abov_tk_info *info)
{
	unsigned char data[2] = {0xAC, 0x2D};
	int ret = 0;

	ret = i2c_master_send(info->client, data, 2);
	if (ret != 2) {
		pr_err("%s: write fail\n", __func__);
		return -1;
	}

	return 0;

}

static int abov_tk_fw_mode_exit(struct abov_tk_info *info)
{
	unsigned char data[2] = {0xAC, 0xE1};
	int ret = 0;

	ret = i2c_master_send(info->client, data, 2);
	if (ret != 2) {
		pr_err("%s: write fail\n", __func__);
		return -1;
	}

	return 0;

}

static int abov_tk_fw_update(struct abov_tk_info *info, u8 cmd)
{
	int ret, ii = 0;
	int count;
	unsigned short address;
	unsigned char addrH, addrL;
	unsigned char data[32] = {0, };


	input_info(true, &info->client->dev, "%s start\n", __func__);

	count = info->firm_size / 32;
	address = 0x800;

	input_info(true, &info->client->dev, "%s reset\n", __func__);
	abov_tk_reset_for_bootmode(info);
	msleep(ABOV_BOOT_DELAY);
	ret = abov_tk_fw_mode_enter(info);
	if(ret<0){
		input_err(true, &info->client->dev,
			"%s:abov_tk_fw_mode_enter fail\n", __func__);
		return ret;
	}
	usleep_range(5 * 1000, 5 * 1000);
	input_info(true, &info->client->dev, "%s fw_mode_cmd sended\n", __func__);

	if(abov_tk_fw_mode_check(info) != 1)
	{
		pr_err("%s: err, flash mode is not: %d\n", __func__, ret);
		return 0;
	}

	ret = abov_tk_flash_erase(info);
	msleep(1400);
	input_info(true, &info->client->dev, "%s fw_write start\n", __func__);

	for (ii = 1; ii < count; ii++) {
		/* first 32byte is header */
		addrH = (unsigned char)((address >> 8) & 0xFF);
		addrL = (unsigned char)(address & 0xFF);
		if (cmd == BUILT_IN)
			memcpy(data, &info->firm_data_bin->data[ii * 32], 32);
		else if (cmd == SDCARD)
			memcpy(data, &info->firm_data_ums[ii * 32], 32);

		ret = abov_tk_fw_write(info, &addrH, &addrL, data);
		if (ret < 0) {
			input_err(true, &info->client->dev,
				"%s: err, no device : %d\n", __func__, ret);
			return ret;
		}

		address += 0x20;

		memset(data, 0, 32);
	}
	ret = abov_tk_i2c_read_checksum(info);
	input_dbg(true, &info->client->dev, "%s checksum readed\n", __func__);

	ret = abov_tk_fw_mode_exit(info);
	input_info(true, &info->client->dev, "%s fw_write end\n", __func__);

	return ret;
}

static void abov_release_fw(struct abov_tk_info *info, u8 cmd)
{
	switch(cmd) {
	case BUILT_IN:
		release_firmware(info->firm_data_bin);
		break;

	case SDCARD:
		kfree(info->firm_data_ums);
		break;

	default:
		break;
	}
}

static int abov_flash_fw(struct abov_tk_info *info, bool probe, u8 cmd)
{
	struct i2c_client *client = info->client;
	int retry = 2;
	int ret;
	int block_count;
	const u8 *fw_data;

	switch(cmd) {
	case BUILT_IN:
		fw_data = info->firm_data_bin->data;
		break;

	case SDCARD:
		fw_data = info->firm_data_ums;
		break;

	default:
		return -1;
		break;
	}

	block_count = (int)(info->firm_size / 32);

	while (retry--) {
		ret = abov_tk_fw_update(info, cmd);
		if (ret < 0)
			break;
#if ABOV_ISP_FIRMUP_ROUTINE
		abov_tk_reset_for_bootmode(info);
		abov_fw_update(info, fw_data, block_count,
			info->pdata->gpio_scl, info->pdata->gpio_sda);
#endif

		if ((info->checksum_h != info->checksum_h_bin) ||
			(info->checksum_l != info->checksum_l_bin)) {
			input_err(true, &client->dev,
				"%s checksum fail.(0x%x,0x%x),(0x%x,0x%x) retry:%d\n",
				__func__, info->checksum_h, info->checksum_l,
				info->checksum_h_bin, info->checksum_l_bin, retry);
			ret = -1;
			continue;
		}else
			input_info(true, &client->dev,"%s checksum successed.\n",__func__);

		abov_tk_reset_for_bootmode(info);
		msleep(ABOV_RESET_DELAY);
		ret = get_tk_fw_version(info, true);
		if (ret) {
			input_err(true, &client->dev, "%s fw version read fail\n", __func__);
			ret = -1;
			continue;
		}

		if (info->fw_ver == 0) {
			input_err(true, &client->dev, "%s fw version fail (0x%x)\n",
				__func__, info->fw_ver);
			ret = -1;
			continue;
		}

		if ((cmd == BUILT_IN) && (info->fw_ver != info->fw_ver_bin)){
			input_err(true, &client->dev, "%s fw version fail 0x%x, 0x%x\n",
				__func__, info->fw_ver, info->fw_ver_bin);
			ret = -1;
			continue;
		}
		ret = 0;
		break;
	}

	return ret;
}

static ssize_t touchkey_fw_update(struct device *dev,
			struct device_attribute *attr, const char *buf, size_t count)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);
	struct i2c_client *client = info->client;
	int ret;
	u8 cmd;

	switch(*buf) {
	case 's':
	case 'S':
		cmd = BUILT_IN;
		break;
	case 'i':
	case 'I':
		cmd = SDCARD;
		break;
	default:
		info->fw_update_state = 2;
		goto touchkey_fw_update_out;
	}

	ret = abov_load_fw(info, cmd);
	if (ret) {
		input_err(true, &client->dev,
			"%s fw load fail\n", __func__);
		info->fw_update_state = 2;
		goto touchkey_fw_update_out;
	}

	info->fw_update_state = 1;
	disable_irq(info->irq);
	info->enabled = false;
	ret = abov_flash_fw(info, false, cmd);
	if (info->flip_mode){
		abov_mode_enable(client, ABOV_FLIP, CMD_FLIP_ON);
	} else {
		if (info->glovemode)
			abov_mode_enable(client, ABOV_GLOVE, CMD_GLOVE_ON);
	}
	if (info->keyboard_mode)
		abov_mode_enable(client, ABOV_KEYBOARD, CMD_MOBILE_KBD_ON);

	info->enabled = true;
	enable_irq(info->irq);
	if (ret) {
		input_err(true, &client->dev, "%s fail\n", __func__);
//		info->fw_update_state = 2;
		info->fw_update_state = 0;

	} else {
		input_info(true, &client->dev, "%s success\n", __func__);
		info->fw_update_state = 0;
	}

	abov_release_fw(info, cmd);

touchkey_fw_update_out:
	input_dbg(true, &client->dev, "%s : %d\n", __func__, info->fw_update_state);

	return count;
}

static ssize_t touchkey_fw_update_status(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);
	struct i2c_client *client = info->client;
	int count = 0;

	input_info(true, &client->dev, "%s : %d\n", __func__, info->fw_update_state);

	if (info->fw_update_state == 0)
		count = sprintf(buf, "PASS\n");
	else if (info->fw_update_state == 1)
		count = sprintf(buf, "Downloading\n");
	else if (info->fw_update_state == 2)
		count = sprintf(buf, "Fail\n");

	return count;
}

static ssize_t abov_glove_mode(struct device *dev,
	 struct device_attribute *attr, const char *buf, size_t count)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);
	struct i2c_client *client = info->client;
	int scan_buffer;
	int ret;
	u8 cmd;

	ret = sscanf(buf, "%d", &scan_buffer);
	input_info(true, &client->dev, "%s : %d\n", __func__, scan_buffer);


	if (ret != 1) {
		input_err(true, &client->dev, "%s: cmd read err\n", __func__);
		return count;
	}

	if (!(scan_buffer == 0 || scan_buffer == 1)) {
		input_err(true, &client->dev, "%s: wrong command(%d)\n",
			__func__, scan_buffer);
		return count;
	}

	if (!info->enabled)
		return count;

	if (info->glovemode == scan_buffer) {
		input_dbg(true, &client->dev, "%s same command(%d)\n",
			__func__, scan_buffer);
		return count;
	}

	if (scan_buffer == 1) {
		cmd = CMD_GLOVE_ON;
	} else {
		cmd = CMD_GLOVE_OFF;
	}

	ret = abov_mode_enable(client, ABOV_GLOVE, cmd);
	if (ret < 0) {
		input_err(true, &client->dev, "%s fail(%d)\n", __func__, ret);
		return count;
	}

	info->glovemode = scan_buffer;

	return count;
}

static ssize_t abov_glove_mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", info->glovemode);
}

static ssize_t keyboard_cover_mode_enable(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t count)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);
	struct i2c_client *client = info->client;
	int keyboard_mode_on;
	int ret;
	u8 cmd;

	input_dbg(true, &client->dev, "%s : Mobile KBD sysfs node called\n",__func__);

	sscanf(buf, "%d", &keyboard_mode_on);
	input_info(true, &client->dev, "%s : %d\n",
		__func__, keyboard_mode_on);

	if (!(keyboard_mode_on == 0 || keyboard_mode_on == 1)) {
		input_err(true, &client->dev, "%s: wrong command(%d)\n",
			__func__, keyboard_mode_on);
		return count;
	}

	if (!info->enabled)
		goto out;

	if (info->keyboard_mode == keyboard_mode_on) {
		input_dbg(true, &client->dev, "%s same command(%d)\n",
			__func__, keyboard_mode_on);
		goto out;
	}

	if (keyboard_mode_on == 1) {
		cmd = CMD_MOBILE_KBD_ON;
	} else {
		cmd = CMD_MOBILE_KBD_OFF;
	}

	/* mobile keyboard use same register with glove mode */
	ret = abov_mode_enable(client, ABOV_KEYBOARD, cmd);
	if (ret < 0) {
		input_err(true, &client->dev, "%s fail(%d)\n", __func__, ret);
		goto out;
	}

out:
	info->keyboard_mode = keyboard_mode_on;
	return count;
}

static ssize_t flip_cover_mode_enable(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t count)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);
	struct i2c_client *client = info->client;
	int flip_mode_on;
	int ret;
	u8 cmd;

	sscanf(buf, "%d\n", &flip_mode_on);
	input_info(true, &client->dev, "%s : %d\n", __func__, flip_mode_on);

	if (!info->enabled)
		goto out;

#ifdef CONFIG_TOUCHKEY_GRIP
	if (flip_mode_on) {
		cmd = 0x10;
		ret = abov_tk_i2c_write(info->client, ABOV_SW_RESET, &cmd, 1);
		if (ret < 0) {
			input_err(true, &client->dev, "%s sw_reset fail(%d)\n", __func__, ret);
		}
		abov_sar_olny_mode(info, 1);
	} else {
		abov_sar_olny_mode(info, 0);
	}
#else
	/* glove mode control */
	if (flip_mode_on) {
		cmd = CMD_FLIP_ON;
	} else {
		if (info->glovemode)
			cmd = CMD_GLOVE_ON;
		cmd = CMD_FLIP_OFF;
	}

	if (info->glovemode){
		ret = abov_mode_enable(client, ABOV_GLOVE, cmd);
		if (ret < 0) {
			input_err(true, &client->dev, "%s glove mode fail(%d)\n", __func__, ret);
			goto out;
		}
	} else{
		ret = abov_mode_enable(client, ABOV_FLIP, cmd);
		if (ret < 0) {
			input_err(true, &client->dev, "%s fail(%d)\n", __func__, ret);
			goto out;
		}
	}
#endif

out:
	info->flip_mode = flip_mode_on;
	return count;
}

static ssize_t touchkey_irq_count_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);
	struct i2c_client *client = info->client;
	int homekey = 0;
	input_info(true, &client->dev, "%s - Recent : %d, Home : %d, Back : %d \n", __func__, info->irq_key_count[0], homekey, info->irq_key_count[1]);

	return sprintf(buf, "%d,%d,%d\n", info->irq_key_count[0], homekey, info->irq_key_count[1]);
}

static ssize_t touchkey_irq_count_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);
	struct i2c_client *client = info->client;

	u8 onoff = 0;
	int i;

	if (buf[0] == 48) {
		onoff = 0;
	} else if (buf[0] == 49) {
		onoff = 1;
	} else {
		input_err(true,&client->dev, "%s [ERROR] Unknown value [%c]\n", __func__, buf[0]);
		goto exit;
	}
	
	if (onoff == 0) {
		info->irq_checked= 0;
	} else if (onoff == 1) {
		info->irq_checked= 1;
		for(i=0; i < MAX_KEY_NUM; i++)
			info->irq_key_count[i] = 0;
	} else {
		input_err(true, &client->dev, "%s - unknown value %d\n", __func__, onoff);
		goto error;
	}

exit:
	input_info(true,&client->dev, "%s - %d [DONE]\n", __func__,onoff);
	return count;

error:
	input_err(true,&client->dev, "%s - %d [ERROR]\n", __func__,onoff);
	return count;	
}

#ifdef CONFIG_TOUCHKEY_LIGHT_EFS
static ssize_t touchkey_light_version_read(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);
	int count;
	int crc_cal, crc_efs;

	if (efs_read_light_table_version(info) < 0) {
		count = sprintf(buf, "NG");
		goto out;
	} else {
		if (info->light_version_efs == info->pdata->dt_light_version) {
			if (!check_light_table_crc(info)) {
				count = sprintf(buf, "NG_CS");
				goto out;
			}
		} else {
			crc_cal = efs_calculate_crc(info);
			crc_efs = efs_read_crc(info);
			input_info(true, &info->client->dev,
					"CRC compare: efs[%d], bin[%d]\n",
					crc_cal, crc_efs);
			if (crc_cal != crc_efs) {
				count = sprintf(buf, "NG_CS");
				goto out;
			}
		}
	}

	count = sprintf(buf, "%s,%s",
			info->light_version_full_efs,
			info->light_version_full_bin);
out:
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buf);
	return count;
}

static ssize_t touchkey_light_update(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t size)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);
	int ret;
	int led_reg;
	int window_type = read_window_type();

	ret = efs_write(info);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: fail %d\n", __func__, ret);
		return -EIO;
	}

	led_reg = efs_read_light_table_with_default(info, window_type);
	if ((led_reg >= LIGHT_REG_MIN_VAL) && (led_reg <= LIGHT_REG_MAX_VAL)) {
		change_touch_key_led_brightness(&info->client->dev, led_reg);
		input_info(true, &info->client->dev,
				"%s: read done for %d\n", __func__, window_type);
	} else {
		input_err(true, &info->client->dev,
				"%s: fail. key led brightness reg is %d\n", __func__, led_reg);
	}

	return size;
}

static ssize_t touchkey_light_id_compare(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);
	int count, ret;
	int window_type = read_window_type();

	if (window_type < 0) {
		input_err(true, &info->client->dev,
				"%s: window_type:%d, NG\n", __func__, window_type);
		return sprintf(buf, "NG");
	}

	ret = efs_read_light_table(info, window_type);
	if (ret < 0) {
		count = sprintf(buf, "NG");
	} else {
		count = sprintf(buf, "OK");
	}

	input_info(true, &info->client->dev,
			"%s: window_type:%d, %s\n", __func__, window_type, buf);
	return count;
}

static char* tokenizer(char* str, char delim)
{
	static char* str_addr;
	char* token = NULL;
	
	if (str != NULL)
		str_addr = str;
	else if (str_addr == NULL)
		return 0;

	token = str_addr;
	while (true) {
		if (!(*str_addr)) {
			break;
		} else if (*str_addr == delim) {
			*str_addr = '\0';
			str_addr = str_addr + 1;
			break;
		}
		str_addr++;
	}

	return token;
}

static ssize_t touchkey_light_table_write(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t size)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);
	struct light_info table[16];
	int ret;
	int led_reg;
	int window_type;
	char *full_version;
	char data[150] = {0, };
	int i, crc, crc_cal;
	char *octa_id;
	int table_size = 0;

	snprintf(data, sizeof(data), buf);

	input_info(true, &info->client->dev, "%s: %s\n",
			__func__, data);

	full_version = tokenizer(data, ',');
	if (!full_version)
		return -EIO;

	table_size = (int)strlen(full_version) - 8;
	input_info(true, &info->client->dev, "%s: version:%s size:%d\n",
			__func__, full_version, table_size);
	if (table_size < 0 || table_size > 16) {
		input_err(true, &info->client->dev, "%s: table_size is unavailable\n", __func__);
		return -EIO;
	}

	if (kstrtoint(tokenizer(NULL, ','), 0, &crc))
		return -EIO;

	input_info(true, &info->client->dev, "%s: crc:%d\n",
			__func__, crc);
	if (!crc)
		return -EIO;

	for (i = 0; i < table_size; i++) {
		octa_id = tokenizer(NULL, '_');
		if (!octa_id)
			break;

		if (octa_id[0] >= 'A')
			table[i].octa_id = octa_id[0] - 'A' + 0x0A;
		else
			table[i].octa_id = octa_id[0] - '0';
		if (table[i].octa_id < 0 || table[i].octa_id > 0x0F)
			break;
		if (kstrtoint(tokenizer(NULL, ','), 0, &table[i].led_reg))
			break;
	}

	if (!table_size) {
		input_err(true, &info->client->dev, "%s: no data in table\n", __func__);
		return -ENODATA;
	}

	for (i = 0; i < table_size; i++) {
		input_info(true, &info->client->dev, "%s: [%d] %X - %x\n",
				__func__, i, table[i].octa_id, table[i].led_reg);
	}

	/* write efs */
	ret = efs_write_light_table_version(info, full_version);
	if (ret < 0) {
		input_err(true, &info->client->dev,
				"%s: failed to write table ver %s. %d\n",
				__func__, full_version, ret);
		return ret;
	}

	info->light_version_efs = pick_light_table_version(full_version);

	for (i = 0; i < table_size; i++) {
		ret = efs_write_light_table(info, table[i]);
		if (ret < 0)
			break;
	}
	if (ret < 0) {
		input_err(true, &info->client->dev,
				"%s: failed to write table%d. %d\n",
				__func__, i, ret);
		return ret;
	}

	ret = efs_write_light_table_crc(info, crc);
	if (ret < 0) {
		input_err(true, &info->client->dev,
				"%s: failed to write table crc. %d\n",
				__func__, ret);
		return ret;
	}

	crc_cal = efs_calculate_crc(info);
	input_info(true, &info->client->dev,
			"%s: efs crc:%d, caldulated crc:%d\n",
			__func__, crc, crc_cal);
	if (crc_cal != crc)
		return -EIO;

	window_type = read_window_type();
	led_reg = efs_read_light_table_with_default(info, window_type);
	if ((led_reg >= LIGHT_REG_MIN_VAL) && (led_reg <= LIGHT_REG_MAX_VAL)) {
		change_touch_key_led_brightness(&info->client->dev, led_reg);
		input_info(true, &info->client->dev,
				"%s: read done for %d\n", __func__, window_type);
	} else {
		input_err(true, &info->client->dev,
				"%s: fail. key led brightness reg is %d\n", __func__, led_reg);
	}

	return size;
}
#endif /* CONFIG_TOUCHKEY_LIGHT_EFS */

static DEVICE_ATTR(touchkey_threshold, S_IRUGO, touchkey_threshold_show, NULL);
#ifdef CONFIG_TOUCHKEY_LED
static DEVICE_ATTR(brightness, S_IRUGO | S_IWUSR | S_IWGRP, NULL,
			touchkey_led_control);
#endif /* CONFIG_TOUCHKEY_LED */
#ifdef CONFIG_TOUCHKEY_GRIP
static DEVICE_ATTR(touchkey_grip_threshold, S_IRUGO, touchkey_grip_threshold_show, NULL);
static DEVICE_ATTR(touchkey_total_cap, S_IRUGO, touchkey_total_cap_show, NULL);
static DEVICE_ATTR(sar_enable, S_IRUGO | S_IWUSR | S_IWGRP, NULL, touchkey_sar_enable);
static DEVICE_ATTR(sw_reset, S_IRUGO | S_IWUSR | S_IWGRP, NULL, touchkey_grip_sw_reset);
static DEVICE_ATTR(touchkey_earjack, S_IRUGO | S_IWUSR | S_IWGRP, NULL, touchkey_sensing_change);
static DEVICE_ATTR(touchkey_grip, S_IRUGO, touchkey_grip_show, NULL);
static DEVICE_ATTR(touchkey_grip_baseline, S_IRUGO, touchkey_grip_baseline_show, NULL);
static DEVICE_ATTR(touchkey_grip_raw, S_IRUGO, touchkey_grip_raw_show, NULL);
static DEVICE_ATTR(touchkey_grip_gain, S_IRUGO, touchkey_grip_gain_show, NULL);
static DEVICE_ATTR(touchkey_grip_check, S_IRUGO, touchkey_grip_check_show, NULL);
#ifndef CONFIG_SAMSUNG_PRODUCT_SHIP
static DEVICE_ATTR(touchkey_sar_only_mode,  S_IRUGO | S_IWUSR | S_IWGRP ,
						NULL, touchkey_mode_change);
static DEVICE_ATTR(touchkey_sar_press_threshold,  S_IRUGO | S_IWUSR | S_IWGRP ,
						NULL, touchkey_sar_press_threshold_store);
static DEVICE_ATTR(touchkey_sar_release_threshold,  S_IRUGO | S_IWUSR | S_IWGRP ,
						NULL, touchkey_sar_release_threshold_store);
#endif
#endif
static DEVICE_ATTR(touchkey_recent, S_IRUGO, touchkey_menu_show, NULL);
static DEVICE_ATTR(touchkey_back, S_IRUGO, touchkey_back_show, NULL);
static DEVICE_ATTR(touchkey_recent_raw, S_IRUGO, touchkey_menu_raw_show, NULL);
static DEVICE_ATTR(touchkey_back_raw, S_IRUGO, touchkey_back_raw_show, NULL);
static DEVICE_ATTR(touchkey_chip_name, S_IRUGO, touchkey_chip_name, NULL);
static DEVICE_ATTR(touchkey_firm_version_phone, S_IRUGO, bin_fw_ver, NULL);
static DEVICE_ATTR(touchkey_firm_version_panel, S_IRUGO, read_fw_ver, NULL);
static DEVICE_ATTR(touchkey_firm_update, S_IRUGO | S_IWUSR | S_IWGRP, NULL,
			touchkey_fw_update);
static DEVICE_ATTR(touchkey_firm_update_status, S_IRUGO | S_IWUSR | S_IWGRP,
			touchkey_fw_update_status, NULL);
static DEVICE_ATTR(glove_mode, S_IRUGO | S_IWUSR | S_IWGRP,
			abov_glove_mode_show, abov_glove_mode);
static DEVICE_ATTR(keyboard_mode, S_IRUGO | S_IWUSR | S_IWGRP, NULL,
			keyboard_cover_mode_enable);
static DEVICE_ATTR(flip_mode, S_IRUGO | S_IWUSR | S_IWGRP, NULL,
		    flip_cover_mode_enable);
static DEVICE_ATTR(touchkey_irq_count, S_IRUGO | S_IWUSR | S_IWGRP, 
            touchkey_irq_count_show, touchkey_irq_count_store);
#ifdef CONFIG_TOUCHKEY_LIGHT_EFS
static DEVICE_ATTR(touchkey_light_version, S_IRUGO, touchkey_light_version_read, NULL);
static DEVICE_ATTR(touchkey_light_update, S_IWUSR | S_IWGRP, NULL, touchkey_light_update);
static DEVICE_ATTR(touchkey_light_id_compare, S_IRUGO, touchkey_light_id_compare, NULL);
static DEVICE_ATTR(touchkey_light_table_write, S_IWUSR | S_IWGRP, NULL, touchkey_light_table_write);
#endif /* CONFIG_TOUCHKEY_LIGHT_EFS */

static struct attribute *sec_touchkey_attributes[] = {
	&dev_attr_touchkey_threshold.attr,
#ifdef CONFIG_TOUCHKEY_LED	
	&dev_attr_brightness.attr,
#endif /* CONFIG_TOUCHKEY_LED */	
#ifdef CONFIG_TOUCHKEY_GRIP
	&dev_attr_touchkey_grip_threshold.attr,
	&dev_attr_touchkey_total_cap.attr,
	&dev_attr_sar_enable.attr,
	&dev_attr_sw_reset.attr,
	&dev_attr_touchkey_earjack.attr,
	&dev_attr_touchkey_grip.attr,
	&dev_attr_touchkey_grip_baseline.attr,
	&dev_attr_touchkey_grip_raw.attr,
	&dev_attr_touchkey_grip_gain.attr,
	&dev_attr_touchkey_grip_check.attr,
#ifndef CONFIG_SAMSUNG_PRODUCT_SHIP
	&dev_attr_touchkey_sar_only_mode.attr,
	&dev_attr_touchkey_sar_press_threshold.attr,
	&dev_attr_touchkey_sar_release_threshold.attr,
#endif /* CONFIG_SAMSUNG_PRODUCT_SHIP */
#endif /* CONFIG_TOUCHKEY_GRIP */
	&dev_attr_touchkey_recent.attr,
	&dev_attr_touchkey_back.attr,
	&dev_attr_touchkey_recent_raw.attr,
	&dev_attr_touchkey_back_raw.attr,
	&dev_attr_touchkey_chip_name.attr,
	&dev_attr_touchkey_firm_version_phone.attr,
	&dev_attr_touchkey_firm_version_panel.attr,
	&dev_attr_touchkey_firm_update.attr,
	&dev_attr_touchkey_firm_update_status.attr,
	&dev_attr_glove_mode.attr,
	&dev_attr_keyboard_mode.attr,
	&dev_attr_flip_mode.attr,
	&dev_attr_touchkey_irq_count.attr,
#ifdef CONFIG_TOUCHKEY_LIGHT_EFS
	&dev_attr_touchkey_light_version.attr,
	&dev_attr_touchkey_light_update.attr,
	&dev_attr_touchkey_light_id_compare.attr,
	&dev_attr_touchkey_light_table_write.attr,
#endif /* CONFIG_TOUCHKEY_LIGHT_EFS */
	NULL,
};

static struct attribute_group sec_touchkey_attr_group = {
	.attrs = sec_touchkey_attributes,
};

extern int get_samsung_lcd_attached(void);

static int abov_tk_fw_check(struct abov_tk_info *info)
{
	struct i2c_client *client = info->client;
	int ret, fw_update=0;
	u8 buf;

	ret = abov_load_fw(info, BUILT_IN);
	if (ret) {
		input_err(true, &client->dev,
			"%s fw load fail\n", __func__);
		return ret;
	}

	ret = get_tk_fw_version(info, true);

#ifdef LED_TWINKLE_BOOTING
	if(ret)
		input_err(true, &client->dev,
			"%s: i2c fail...[%d], addr[%d]\n",
			__func__, ret, info->client->addr);
		input_err(true, &client->dev,
			"%s: touchkey driver unload\n", __func__);

		if (get_samsung_lcd_attached() == 0) {
			input_err(true, &client->dev, "%s : get_samsung_lcd_attached()=0 \n", __func__);
			abov_release_fw(info, BUILT_IN);
			return ret;
		}
#endif /* LED_TWINKLE_BOOTING */

	//Check Model No.
	ret = abov_tk_i2c_read(client, ABOV_MODEL_NUMBER, &buf, 1);
	if (ret < 0) {
		input_err(true, &client->dev, "%s fail(%d)\n", __func__, ret);
	}
	if(info->fw_model_number != buf){
		input_info(true, &client->dev, "fw model number = %x ic model number = %x \n", info->fw_model_number, buf);
		fw_update = 1;
		goto flash_fw;
	}

	if ((info->fw_ver == 0) || info->fw_ver < info->fw_ver_bin){
		input_dbg(true, &client->dev, "excute tk firmware update (0x%x -> 0x%x\n",
			info->fw_ver, info->fw_ver_bin);
		fw_update = 1;
	}

#ifdef CONFIG_SAMSUNG_PRODUCT_SHIP
	if(info->fw_ver >= 0xd0){		//test firmware
		input_dbg(true, &client->dev, "excute tk firmware update (0x%x -> 0x%x\n",
			info->fw_ver, info->fw_ver_bin);
		fw_update = 1;
	}
#endif

	if (info->pdata->bringup) {
		input_info(true, &client->dev, "%s: firmware update skip, bring up\n", __func__);
		fw_update = 0;
	}

flash_fw:
	if(fw_update){
		ret = abov_flash_fw(info, true, BUILT_IN);
		if (ret) {
			input_err(true, &client->dev,
				"failed to abov_flash_fw (%d)\n", ret);
		} else {
			input_info(true, &client->dev,
				"fw update success\n");
		}
	}

	abov_release_fw(info, BUILT_IN);

	return ret;
}

#ifdef CONFIG_TOUCHKEY_LED
static int abov_led_power(void *data, bool on)
{
	struct abov_tk_info *info = (struct abov_tk_info *)data;
	struct i2c_client *client = info->client;

	int ret = 0;

	info->pdata->avdd_vreg = regulator_get(NULL, "vtouch_3.3v");
	if (IS_ERR(info->pdata->avdd_vreg)) {
		info->pdata->avdd_vreg = NULL;
		input_err(true, &client->dev, "%s : avdd_vreg get error, ignoring\n", __func__);
	}


	if(regulator_is_enabled(info->pdata->avdd_vreg)==on){
		input_info(true, &client->dev, "%s %s skip\n", __func__, on ? "on" : "off");
		return ret;
	}


	if (on) {
		if (info->pdata->avdd_vreg) {
			ret = regulator_enable(info->pdata->avdd_vreg);
			if(ret){
				input_err(true, &client->dev, "%s : avdd reg enable fail\n", __func__);
				return ret;
			}
		}
	} else {
		if (info->pdata->avdd_vreg) {
			input_info(true, &client->dev, "%s 1\n", __func__);
			ret = regulator_disable(info->pdata->avdd_vreg);
			if(ret){
				input_err(true, &client->dev, "%s : avdd reg disable fail\n", __func__);
				return ret;
			}
		}
	}
	regulator_put(info->pdata->avdd_vreg);

	input_info(true, &client->dev, "%s %s\n", __func__, on ? "on" : "off");

	return ret;
}
#endif /* CONFIG_TOUCHKEY_LED */

static int abov_power_regulator_ctrl(void *data, bool on)
{
	struct abov_tk_info *info = (struct abov_tk_info *)data;
	struct i2c_client *client = info->client;
	int ret = 0;

	info->pdata->dvdd_vreg = regulator_get(NULL, "vtouch_2.8v");
	if (IS_ERR(info->pdata->dvdd_vreg)) {
		info->pdata->dvdd_vreg = NULL;
		input_err(true, &client->dev, "%s : dvdd_vreg get error, ignoring\n",__func__);
	}

	if (on) {
		if (info->pdata->dvdd_vreg) {
			ret = regulator_enable(info->pdata->dvdd_vreg);
			if(ret){
				input_err(true, &client->dev, "%s : dvdd reg enable fail\n", __func__);
				return ret;
			}
		}
	} else {
		if (info->pdata->dvdd_vreg) {
			ret = regulator_disable(info->pdata->dvdd_vreg);
			if(ret){
				input_err(true, &client->dev, "%s : dvdd reg disable fail\n", __func__);
				return ret;
			}
		}
	}
	regulator_put(info->pdata->dvdd_vreg);

#ifdef CONFIG_TOUCHKEY_LED	
	abov_led_power(info, on);
#endif /* CONFIG_TOUCHKEY_LED */	

	input_info(true, &client->dev, "%s %s\n", __func__, on ? "on" : "off");

	return ret;
}

static int abov_power_gpio_ctrl(void *data, bool on)
{
	struct abov_tk_info *info = (struct abov_tk_info *)data;
	
	gpio_set_value(info->pdata->gpio_en, on);

#ifdef CONFIG_TOUCHKEY_LED
	abov_led_power(info, on);
#endif /* CONFIG_TOUCHKEY_LED */	

	input_info(true, &info->client->dev, "%s %s\n", __func__, on ? "on" : "off");

	return 0;
}

static void abov_set_ta_status(struct abov_tk_info *info)
{
	u8 cmd_data = 0x10;
	u8 cmd_ta;
	int ret = 0;

	input_info(true, &info->client->dev, "%s g_ta_connected %d\n", __func__, g_ta_connected);

	if (info->enabled == false)
	{
		input_info(true, &info->client->dev, "%s status of ic is off\n", __func__);
		return;
	}

	if (g_ta_connected) {
		cmd_ta = 0x10;
	}
	else {
		cmd_ta = 0x20;
	}

	ret = abov_tk_i2c_write(info->client, ABOV_TSPTA, &cmd_ta, 1);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s fail(%d)\n", __func__, ret);
	}

	ret = abov_tk_i2c_write(info->client, ABOV_SW_RESET, &cmd_data, 1);
	if (ret < 0) {
		dev_err(&info->client->dev, "%s sw reset fail(%d)\n", __func__, ret);
	}
}

#ifdef CONFIG_VBUS_NOTIFIER
int abov_touchkey_vbus_notification(struct notifier_block *nb,
		unsigned long cmd, void *data)
{
	struct abov_tk_info *info = container_of(nb, struct abov_tk_info, vbus_nb);
	vbus_status_t vbus_type = *(vbus_status_t *)data;

	input_info(true, &info->client->dev, "%s cmd=%lu, vbus_type=%d\n", __func__, cmd, vbus_type);

	switch (vbus_type) {
	case STATUS_VBUS_HIGH:
		input_info(true, &info->client->dev, "%s : attach\n",__func__);
		g_ta_connected = true;
		break;
	case STATUS_VBUS_LOW:
		input_info(true, &info->client->dev, "%s : detach\n",__func__);
		g_ta_connected = false;

		break;
	default:
		break;
	}

	abov_set_ta_status(info);

	return 0;
}
#endif


static int abov_pinctrl_configure(struct abov_tk_info *info,
							bool active)
{
	struct pinctrl_state *set_state;
	int retval;

	if (active) {
		set_state =
			pinctrl_lookup_state(info->pinctrl,
						"idle");
		if (IS_ERR(set_state)) {
			input_err(true, &info->client->dev,
				"%s: cannot get ts pinctrl active state\n", __func__);
			return PTR_ERR(set_state);
		}
	} else {
		set_state =
			pinctrl_lookup_state(info->pinctrl,
						"sleep");
		if (IS_ERR(set_state)) {
			input_err(true, &info->client->dev,
				"%s: cannot get gpiokey pinctrl sleep state\n", __func__);
			return PTR_ERR(set_state);
		}
	}
	retval = pinctrl_select_state(info->pinctrl, set_state);
	if (retval) {
		input_err(true, &info->client->dev,
			"%s: cannot set ts pinctrl active state\n", __func__);
		return retval;
	}

	return 0;
}


#ifdef CONFIG_OF
static int abov_parse_dt(struct device *dev,
			struct abov_touchkey_platform_data *pdata)
{
	struct device_node *np = dev->of_node;
	int ret;
#ifdef CONFIG_TOUCHKEY_LIGHT_EFS
	int i;
	u32 tmp[LIGHT_TABLE_MAX] = {0, };
#endif /* CONFIG_TOUCHKEY_LIGHT_EFS */

	if(of_property_read_bool(np, "abov,power-gpioen"))
	{
		pdata->gpio_en = of_get_named_gpio(np, "abov,power-gpio", 0);
		if(pdata->gpio_en < 0){
			input_err(true, dev, "unable to get gpio_int\n");
			return pdata->gpio_en;
		}
		
		if (gpio_is_valid(pdata->gpio_en)) {
			ret = gpio_request(pdata->gpio_en, "tkey_power_en");
			if(ret < 0){
				input_err(true, dev,
					"unable to request gpio_en\n");
				return ret;
			}
			
			ret = gpio_direction_output(pdata->gpio_en, 0);
			if (ret) {
				input_err(true, dev, "%s: unable to set direction for gpio [%d]\n",
						__func__, pdata->gpio_en);
			}
		}
		
		/* Set power control function */
		pdata->power = abov_power_gpio_ctrl;
		
		input_info(true, dev, "%s: power control by gpio (%d) \n", __func__, pdata->gpio_en);
	}
	else
	{
		// TO DO : Regulator get if any 
		pdata->power = abov_power_regulator_ctrl;
		
		input_info(true, dev, "%s: power control by regulator\n", __func__);
	}
	
	pdata->gpio_int = of_get_named_gpio(np, "abov,irq-gpio", 0);
	if(pdata->gpio_int < 0){
		input_err(true, dev, "unable to get gpio_int\n");
		return pdata->gpio_int;
	}
	else
	{
		if (gpio_is_valid(pdata->gpio_int)) {
			ret = gpio_request(pdata->gpio_int, "tkey_gpio_int");
			if(ret < 0){
				input_err(true, dev,
					"unable to request gpio_int\n");
				return ret;
			}
			
			ret = gpio_direction_input(pdata->gpio_int);
			if (ret) {
				input_err(true, dev, "%s: unable to set direction for gpio [%d]\n",
						__func__, pdata->gpio_int);
			}
		}
	}

	pdata->gpio_scl = of_get_named_gpio(np, "abov,scl-gpio", 0);
	if(pdata->gpio_scl < 0){
		input_err(true, dev, "unable to get gpio_scl\n");
		return pdata->gpio_scl;
	}

	pdata->gpio_sda = of_get_named_gpio(np, "abov,sda-gpio", 0);
	if(pdata->gpio_sda < 0){
		input_err(true, dev, "unable to get gpio_sda\n");
		return pdata->gpio_sda;
	}

	pdata->sub_det = of_get_named_gpio(np, "abov,sub-det", 0);
	if(pdata->sub_det < 0){
		input_info(true, dev, "unable to get sub_det\n");
	}else{
		input_info(true, dev, "%s: sub_det:%d\n",__func__,pdata->sub_det);
	}

	ret = of_property_read_string(np, "abov,fw_path", (const char **)&pdata->fw_path);

	if (ret) {
		input_err(true, dev, "touchkey:failed to read fw_path %d\n", ret);
		pdata->fw_path = TK_FW_PATH_BIN;
	}
	input_info(true, dev, "%s: fw path %s\n", __func__, pdata->fw_path);

	pdata->boot_on_ldo = of_property_read_bool(np, "abov,boot-on-ldo");
	pdata->bringup = of_property_read_bool(np, "abov,bringup");
	pdata->ta_notifier = of_property_read_bool(np, "abov,ta-notifier");

	input_info(true, dev, "%s: gpio_int:%d, gpio_scl:%d, gpio_sda:%d\n",
			__func__, pdata->gpio_int, pdata->gpio_scl,
			pdata->gpio_sda);

#ifdef CONFIG_TOUCHKEY_LIGHT_EFS
	ret = of_property_read_u32_array(np, "abov,light_version", tmp, 2);
	if (ret) {
		input_err(true, dev, "touchkey:failed to read light_version %d\n", ret);
	}
	pdata->dt_light_version = tmp[0];
	pdata->dt_light_table = tmp[1];

	input_info(true, dev, "%s: light_version:%d, light_table:%d\n",
			__func__, pdata->dt_light_version, pdata->dt_light_table);

	if(pdata->dt_light_table > 0){
		ret = of_property_read_u32_array(np, "abov,octa_id", tmp, pdata->dt_light_table);
		if (ret) {
			input_err(true, dev, "touchkey:failed to read light_version %d\n", ret);
		}
		for(i = 0 ; i < pdata->dt_light_table ; i++){
			tkey_light_reg_table[i].octa_id = tmp[i];
		}

		ret = of_property_read_u32_array(np, "abov,light_reg", tmp, pdata->dt_light_table);
		if (ret) {
			input_err(true, dev, "touchkey:failed to read light_version %d\n", ret);
		}
		for(i = 0 ; i < pdata->dt_light_table ; i++){
			tkey_light_reg_table[i].led_reg = tmp[i];
		}

		for(i = 0 ; i < pdata->dt_light_table ; i++){
			input_info(true, dev, "%s: tkey_light_reg_table: %d 0x%02x\n",
				__func__, tkey_light_reg_table[i].octa_id, tkey_light_reg_table[i].led_reg);
		}
	}
#endif /* CONFIG_TOUCHKEY_LIGHT_EFS */

	return 0;
}
#else
static int abov_parse_dt(struct device *dev,
			struct abov_touchkey_platform_data *pdata)
{
	return -ENODEV;
}
#endif /* CONFIG_OF */

unsigned int boot_mode;
static int __init get_bootmode(char *arg)
{
	get_option(&arg, &boot_mode);

	return 0;
}
early_param("bootmode", get_bootmode);

extern int get_lcd_attached(char *mode);


static int abov_tk_probe(struct i2c_client *client,
				  const struct i2c_device_id *id)
{
	struct abov_tk_info *info;
	struct input_dev *input_dev;
	int ret = 0;
#ifdef CONFIG_TOUCHKEY_LIGHT_EFS
	int i;
	char tmp[2] = {0, };
#endif /* CONFIG_TOUCHKEY_LIGHT_EFS */

	if (boot_mode == 2) {
		input_err(true, &client->dev,
			"%s : Do not load driver due to : device entered recovery mode %d\n",
			__func__, boot_mode);
		return -ENODEV;
	}

#ifndef LED_TWINKLE_BOOTING	
	/* when lcd moudle was taken off, lcd id is received 0xFFFFFF */
	if (get_lcd_attached("GET") == 0xFFFFFF) {
		input_err(true, &client->dev, 
				"%s: LCD module was taken off\n",
				__func__);
		return -EIO;
	}	
#endif /* LED_TWINKLE_BOOTING */	

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		input_err(true, &client->dev,
			"i2c_check_functionality fail\n");
		return -EIO;
	}

	info = kzalloc(sizeof(struct abov_tk_info), GFP_KERNEL);
	if (!info) {
		input_err(true, &client->dev, "Failed to allocate memory\n");
		ret = -ENOMEM;
		goto err_alloc;
	}

	input_dev = input_allocate_device();
	if (!input_dev) {
		input_err(true, &client->dev,
			"Failed to allocate memory for input device\n");
		ret = -ENOMEM;
		goto err_input_alloc;
	}

	info->client = client;
	info->input_dev = input_dev;

#ifdef CONFIG_TOUCHKEY_GRIP
	wake_lock_init(&info->touckey_wake_lock, WAKE_LOCK_SUSPEND, "touchkey wake lock");
#endif

	if (client->dev.of_node) {
		struct abov_touchkey_platform_data *pdata;
		pdata = devm_kzalloc(&client->dev,
			sizeof(struct abov_touchkey_platform_data), GFP_KERNEL);
		if (!pdata) {
			input_err(true, &client->dev, "Failed to allocate memory\n");
			ret = -ENOMEM;
			goto err_config;
		}

		ret = abov_parse_dt(&client->dev, pdata);
		if (ret){
			input_err(true, &client->dev, "failed to abov_parse_dt\n");
			ret = -ENOMEM;
			goto err_config;
		}

		info->pdata = pdata;
	} else
		info->pdata = client->dev.platform_data;

	if (info->pdata == NULL) {
		input_err(true, &client->dev, "failed to get platform data\n");
		goto err_config;
	}

	/* Get pinctrl if target uses pinctrl */
		info->pinctrl = devm_pinctrl_get(&client->dev);
		if (IS_ERR(info->pinctrl)) {
			if (PTR_ERR(info->pinctrl) == -EPROBE_DEFER)
				goto err_config;

			input_err(true, &client->dev, "%s: Target does not use pinctrl\n", __func__);
			info->pinctrl = NULL;
		}
		else
		{
			ret = abov_pinctrl_configure(info, true);
			if (ret)
				input_err(true, &client->dev,
					"%s: cannot set ts pinctrl active state\n", __func__);
		}

		/* sub-det pinctrl */
		if (gpio_is_valid(info->pdata->sub_det)) {
			info->pinctrl_det = devm_pinctrl_get(&client->dev);
			if (IS_ERR(info->pinctrl_det)) {
				input_err(true, &client->dev, "%s: Failed to get pinctrl\n", __func__);
				goto err_config;
			}

			info->pins_default = pinctrl_lookup_state(info->pinctrl_det, "sub_det");
			if (IS_ERR(info->pins_default)) {
				input_err(true, &client->dev, "%s: Failed to get pinctrl state\n", __func__);
				devm_pinctrl_put(info->pinctrl_det);
				goto err_config;
			}

			ret = pinctrl_select_state(info->pinctrl_det, info->pins_default);
			if (ret < 0)
				input_err(true, &client->dev, "%s: Failed to configure sub_det pin\n", __func__);
		}

	if (info->pdata->power)
		info->pdata->power(info, true);

	if(!info->pdata->boot_on_ldo)
		msleep(ABOV_RESET_DELAY);

	if (gpio_is_valid(info->pdata->sub_det)) {
		ret = gpio_get_value(info->pdata->sub_det);
		if (ret) {
			input_err(true, &client->dev, "Device wasn't connected to board \n");
			ret = -ENODEV;
			goto err_i2c_check;
		}
	}

	info->enabled = true;
	info->irq = -1;
	client->irq = gpio_to_irq(info->pdata->gpio_int);

	mutex_init(&info->lock);

	info->input_event = info->pdata->input_event;
	info->touchkey_count = sizeof(touchkey_keycode) / sizeof(int);
	i2c_set_clientdata(client, info);

	ret = abov_tk_fw_check(info);
	if (ret) {
		input_err(true, &client->dev,
			"failed to firmware check (%d)\n", ret);
		goto err_reg_input_dev;
	}

	ret = get_tk_fw_version(info, false);
	if (ret < 0) {
		input_err(true, &client->dev, "%s read fail\n", __func__);
		goto err_reg_input_dev;
	}

	snprintf(info->phys, sizeof(info->phys),
		 "%s/input0", dev_name(&client->dev));
	input_dev->name = "sec_touchkey";
	input_dev->phys = info->phys;
	input_dev->id.bustype = BUS_HOST;
	input_dev->dev.parent = &client->dev;
#ifdef USE_OPEN_CLOSE
	input_dev->open = abov_tk_input_open;
	input_dev->close = abov_tk_input_close;
#endif /* USE_OPEN_CLOSE */
	set_bit(EV_KEY, input_dev->evbit);
	set_bit(KEY_RECENT, input_dev->keybit);
	set_bit(KEY_BACK, input_dev->keybit);
	set_bit(KEY_CP_GRIP, input_dev->keybit);
	set_bit(EV_LED, input_dev->evbit);
	set_bit(LED_MISC, input_dev->ledbit);
	input_set_drvdata(input_dev, info);

	ret = input_register_device(input_dev);
	if (ret) {
		input_err(true, &client->dev, "failed to register input dev (%d)\n",
			ret);
		goto err_reg_input_dev;
	}

	if (!info->pdata->irq_flag) {
		input_err(true, &client->dev, "no irq_flag\n");
		ret = request_threaded_irq(client->irq, NULL, abov_tk_interrupt,
			IRQF_TRIGGER_FALLING | IRQF_ONESHOT, ABOV_TK_NAME, info);
	} else {
		ret = request_threaded_irq(client->irq, NULL, abov_tk_interrupt,
			info->pdata->irq_flag, ABOV_TK_NAME, info);
	}
	if (ret < 0) {
		input_err(true, &client->dev, "Failed to register interrupt\n");
		goto err_req_irq;
	}
	info->irq = client->irq;

#ifdef CONFIG_HAS_EARLYSUSPEND
	info->early_suspend.level = EARLY_SUSPEND_LEVEL_STOP_DRAWING;
	info->early_suspend.suspend = abov_tk_early_suspend;
	info->early_suspend.resume = abov_tk_late_resume;
	register_early_suspend(&info->early_suspend);
#endif /* CONFIG_HAS_EARLYSUSPEND */

	info->dev = sec_device_create((dev_t)(uintptr_t)&info->dev, info, "sec_touchkey");
	if (IS_ERR(info->dev))
		input_err(true, &client->dev,
		"Failed to create device for the touchkey sysfs\n");

	ret = sysfs_create_group(&info->dev ->kobj,
		&sec_touchkey_attr_group);
	if (ret)
		input_err(true, &client->dev, "Failed to create sysfs group\n");

	ret = sysfs_create_link(&info->dev ->kobj,
		&info->input_dev->dev.kobj, "input");
	if (ret < 0) {
		input_err(true, &client->dev,
			"%s: Failed to create input symbolic link\n",
			__func__);
	}

#ifdef LED_TWINKLE_BOOTING
	if (get_lcd_attached("GET") == 0xFFFFFF) {
		input_err(true, &client->dev,
			"%s : get_samsung_lcd_attached()=0, so start LED twinkle \n", __func__);

		INIT_DELAYED_WORK(&info->led_twinkle_work, led_twinkle_work);
		info->led_twinkle_check =  1;

		schedule_delayed_work(&info->led_twinkle_work, msecs_to_jiffies(400));
	}
#endif /* LED_TWINKLE_BOOTING */

#ifdef CONFIG_TOUCHKEY_GRIP
	info->sar_sensing = 1;
	device_init_wakeup(&client->dev, true);
#endif /* CONFIG_TOUCHKEY_GRIP */

	input_err(true, &client->dev, "%s done\n", __func__);
	
	
#ifdef CONFIG_TOUCHKEY_GRIP
	{
		extern int poweroff_charging;
		
		if (poweroff_charging) {
			disable_irq(info->irq);
			input_err(true, &client->dev, "%s disable_irq\n", __func__);
			abov_sar_olny_mode(info, 1);
		}
	}
#endif /* CONFIG_TOUCHKEY_GRIP */

#ifdef CONFIG_VBUS_NOTIFIER
	if (info->pdata->ta_notifier) {
		vbus_notifier_register(&info->vbus_nb, abov_touchkey_vbus_notification,
					VBUS_NOTIFY_DEV_CHARGER);
	}
#endif /* CONFIG_VBUS_NOTIFIER */

#ifdef CONFIG_TOUCHKEY_LIGHT_EFS
	INIT_DELAYED_WORK(&info->efs_open_work, touchkey_efs_open_work);

	info->light_table_crc = info->pdata->dt_light_version;
	sprintf(info->light_version_full_bin, "T%d.", info->pdata->dt_light_version);
	for (i = 0; i < info->pdata->dt_light_table; i++) {
		info->light_table_crc += tkey_light_reg_table[i].octa_id;
		info->light_table_crc += tkey_light_reg_table[i].led_reg;
		snprintf(tmp, 2, "%X", tkey_light_reg_table[i].octa_id);
		strncat(info->light_version_full_bin, tmp, 1);
	}
	input_info(true, &client->dev, "%s: light version of kernel : %s\n",
			__func__, info->light_version_full_bin);

	schedule_delayed_work(&info->efs_open_work, msecs_to_jiffies(2000));
#endif /* CONFIG_TOUCHKEY_LIGHT_EFS */

	return 0;

err_req_irq:
	input_unregister_device(input_dev);
err_reg_input_dev:
	mutex_destroy(&info->lock);
	gpio_free(info->pdata->gpio_int);
err_i2c_check:
	if (info->pdata->power)
		info->pdata->power(info, false);
err_config:
#ifdef CONFIG_TOUCHKEY_GRIP
	wake_lock_destroy(&info->touckey_wake_lock);
#endif
	input_free_device(input_dev);
err_input_alloc:
	kfree(info);
err_alloc:
	input_err(true, &client->dev, "%s fail\n",__func__);
	return ret;

}


#ifdef LED_TWINKLE_BOOTING
static void led_twinkle_work(struct work_struct *work)
{
	struct abov_tk_info *info = container_of(work, struct abov_tk_info,
						led_twinkle_work.work);
	static bool led_on = 1;
	static int count = 0;
	input_info(true, &info->client->dev, "%s, on=%d, c=%d\n",__func__, led_on, count++ );

	if(info->led_twinkle_check == 1){

		touchkey_led_set(info,led_on);
		
		led_on = (led_on == 0);

		schedule_delayed_work(&info->led_twinkle_work, msecs_to_jiffies(400));
	}else{

		if(led_on == 0)
			touchkey_led_set(info, 0);
	}

}
#endif /* LED_TWINKLE_BOOTING */

static int abov_tk_remove(struct i2c_client *client)
{
	struct abov_tk_info *info = i2c_get_clientdata(client);

/*	if (info->enabled)
		info->pdata->power(0);
*/
	info->enabled = false;
#ifdef CONFIG_TOUCHKEY_GRIP
	device_init_wakeup(&client->dev, false);
	wake_lock_destroy(&info->touckey_wake_lock);
#endif
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&info->early_suspend);
#endif
	if (info->irq >= 0)
		free_irq(info->irq, info);
	input_unregister_device(info->input_dev);
	input_free_device(info->input_dev);
	kfree(info);

	return 0;
}

static void abov_tk_shutdown(struct i2c_client *client)
{
	struct abov_tk_info *info = i2c_get_clientdata(client);
#ifdef CONFIG_TOUCHKEY_LED	
	u8 cmd = CMD_LED_OFF;
#endif
	input_info(true, &client->dev, "Inside abov_tk_shutdown \n");

	if (info->enabled){
		disable_irq(info->irq);
#ifdef CONFIG_TOUCHKEY_LED		
	abov_tk_i2c_write(client, ABOV_BTNSTATUS, &cmd, 1);
#endif	
		info->pdata->power(info, false);
	}
	info->enabled = false;
#ifdef CONFIG_TOUCHKEY_LIGHT_EFS
	cancel_delayed_work(&info->efs_open_work);
#endif
// just power off.
//	if (info->irq >= 0)
//		free_irq(info->irq, info);
//	kfree(info);
}

#if defined(CONFIG_PM) && !defined(CONFIG_TOUCHKEY_GRIP)
static int abov_tk_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct abov_tk_info *info = i2c_get_clientdata(client);

	if (!info->enabled) {
		input_info(true, &client->dev, "%s: already power off\n", __func__);
		return 0;
	}

	input_info(true, &client->dev, "%s: users=%d\n", __func__,
		   info->input_dev->users);

	disable_irq(info->irq);
	info->enabled = false;
	release_all_fingers(info);

	if (info->pdata->power)
		info->pdata->power(info, false);
	return 0;
}

static int abov_tk_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct abov_tk_info *info = i2c_get_clientdata(client);
#ifdef CONFIG_TOUCHKEY_LED	
	u8 led_data;
#endif

	if (info->enabled) {
		input_info(true, &client->dev, "%s: already power on\n", __func__);
		return 0;
	}

	input_info(true, &info->client->dev, "%s: users=%d\n", __func__,
		   info->input_dev->users);

	if (info->pdata->power) {
		info->pdata->power(info, true);
		msleep(ABOV_RESET_DELAY);
	} else /* touchkey on by i2c */
		get_tk_fw_version(info, true);

	info->enabled = true;

#ifdef CONFIG_TOUCHKEY_LED	
	if (abov_touchled_cmd_reserved && \
		abov_touchkey_led_status == CMD_LED_ON) {
		abov_touchled_cmd_reserved = 0;
		led_data=abov_touchkey_led_status;

		abov_tk_i2c_write(client, ABOV_BTNSTATUS, &led_data, 1);

		input_dbg(true, &info->client->dev, "%s: LED reserved on\n", __func__);
	}
#endif /* CONFIG_TOUCHKEY_LED */	
	enable_irq(info->irq);

	return 0;
}
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
static void abov_tk_early_suspend(struct early_suspend *h)
{
	struct abov_tk_info *info;
	info = container_of(h, struct abov_tk_info, early_suspend);
	abov_tk_suspend(&info->client->dev);

}

static void abov_tk_late_resume(struct early_suspend *h)
{
	struct abov_tk_info *info;
	info = container_of(h, struct abov_tk_info, early_suspend);
	abov_tk_resume(&info->client->dev);
}
#endif

#ifdef USE_OPEN_CLOSE
static int abov_tk_input_open(struct input_dev *dev)
{
	struct abov_tk_info *info = input_get_drvdata(dev);

	input_dbg(true, &info->client->dev, "%s %d \n",__func__,__LINE__);
	input_info(true, &info->client->dev, "%s: users=%d, v:0x%02x, g(%d), f(%d), k(%d)\n", __func__,
		info->input_dev->users, info->fw_ver, info->flip_mode, info->glovemode, info->keyboard_mode);
#ifdef CONFIG_TOUCHKEY_GRIP
	if (lpcharge == 1) {
		input_info(true, &info->client->dev, "%s(lpcharge): sar_enable(%d)\n", __func__, info->sar_enable);
		return 0;
	}

	input_info(true, &info->client->dev, "%s: sar_enable(%d)\n", __func__, info->sar_enable);

#ifdef CONFIG_TOUCHKEY_LED
	abov_led_power(info, true);
#endif

	if (info->flip_mode)
		abov_sar_olny_mode(info, 1);
	else
		abov_sar_olny_mode(info, 0);

	if (device_may_wakeup(&info->client->dev))
		disable_irq_wake(info->irq );
#else
	abov_tk_resume(&info->client->dev);
	if (info->pinctrl)
		abov_pinctrl_configure(info, true);

	if (info->pdata->ta_notifier && g_ta_connected) {
		abov_set_ta_status(info);
	}
	if (info->flip_mode){
		abov_mode_enable(info->client, ABOV_FLIP, CMD_FLIP_ON);
	} else {
		if (info->glovemode)
			abov_mode_enable(info->client, ABOV_GLOVE, CMD_GLOVE_ON);
	}
	if (info->keyboard_mode)
		abov_mode_enable(info->client, ABOV_KEYBOARD, CMD_MOBILE_KBD_ON);
#endif /* CONFIG_TOUCHKEY_GRIP */
	return 0;
}
static void abov_tk_input_close(struct input_dev *dev)
{
	struct abov_tk_info *info = input_get_drvdata(dev);

	input_dbg(true, &info->client->dev, "%s %d \n",__func__,__LINE__);
	input_info(true, &info->client->dev, "%s: users=%d\n", __func__,
		   info->input_dev->users);
#ifdef CONFIG_TOUCHKEY_GRIP
	input_info(true, &info->client->dev, "%s: sar_enable(%d)\n", __func__, info->sar_enable);
	abov_sar_olny_mode(info, 1);

	if (device_may_wakeup(&info->client->dev))
		enable_irq_wake(info->irq );

#ifdef CONFIG_TOUCHKEY_LED
	abov_led_power(info, false);
#endif /* CONFIG_TOUCHKEY_LED */

#else
	abov_tk_suspend(&info->client->dev);
	if (info->pinctrl)
		abov_pinctrl_configure(info, false);
#endif /* CONFIG_TOUCHKEY_GRIP */

#ifdef LED_TWINKLE_BOOTING
	info->led_twinkle_check = 0;
#endif /* LED_TWINKLE_BOOTING */

}
#endif /* USE_OPEN_CLOSE */
 
#ifndef CONFIG_TOUCHKEY_GRIP
#if defined(CONFIG_PM) && !defined(CONFIG_HAS_EARLYSUSPEND) &&!defined(USE_OPEN_CLOSE)
static const struct dev_pm_ops abov_tk_pm_ops = {
	.suspend = abov_tk_suspend,
	.resume = abov_tk_resume,
};
#endif
#endif /* CONFIG_TOUCHKEY_GRIP */

static const struct i2c_device_id abov_tk_id[] = {
	{ABOV_TK_NAME, 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, abov_tk_id);

#ifdef CONFIG_OF
static struct of_device_id abov_match_table[] = {
	{ .compatible = "abov,a96t316xx",},
	{ },
};
#else
#define abov_match_table NULL
#endif

static struct i2c_driver abov_tk_driver = {
	.probe = abov_tk_probe,
	.remove = abov_tk_remove,
	.shutdown = abov_tk_shutdown,
	.driver = {
		   .name = ABOV_TK_NAME,
		   .of_match_table = abov_match_table,
#ifndef CONFIG_TOUCHKEY_GRIP		   
#if defined(CONFIG_PM) && !defined(CONFIG_HAS_EARLYSUSPEND) &&!defined(USE_OPEN_CLOSE)
		   .pm = &abov_tk_pm_ops,
#endif 
#endif /* CONFIG_TOUCHKEY_GRIP */
	},
	.id_table = abov_tk_id,
};

static int __init touchkey_init(void)
{
#if defined(CONFIG_BATTERY_SAMSUNG) && !defined(CONFIG_TOUCHKEY_GRIP)
	{
		extern int poweroff_charging;
		
		if (poweroff_charging) {
			pr_notice("%s : Do not load driver due to : lpm %d\n",
                __func__, poweroff_charging);
			return 0;
		}
	}
#endif

	return i2c_add_driver(&abov_tk_driver);
}

static void __exit touchkey_exit(void)
{
	i2c_del_driver(&abov_tk_driver);
}

module_init(touchkey_init);
module_exit(touchkey_exit);

/* Module information */
MODULE_AUTHOR("Samsung Electronics");
MODULE_DESCRIPTION("Touchkey driver for Abov MF18xx chip");
MODULE_LICENSE("GPL");
