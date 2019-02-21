/******************** (C) COPYRIGHT 2012 STMicroelectronics ********************
*
* File Name		: fts.c
* Authors		: AMS(Analog Mems Sensor) Team
* Description	: FTS Capacitive touch screen controller (FingerTipS)
*
********************************************************************************
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* THE PRESENT SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES
* OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, FOR THE SOLE
* PURPOSE TO SUPPORT YOUR APPLICATION DEVELOPMENT.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*
* THIS SOFTWARE IS SPECIFICALLY DESIGNED FOR EXCLUSIVE USE WITH ST PARTS.
********************************************************************************
* REVISON HISTORY
* DATE		| DESCRIPTION
* 03/09/2012| First Release
* 08/11/2012| Code migration
* 23/01/2013| SEC Factory Test
* 29/01/2013| Support Hover Events
* 08/04/2013| SEC Factory Test Add more - hover_enable, glove_mode, clear_cover_mode, fast_glove_mode
* 09/04/2013| Support Blob Information
*******************************************************************************/

#include <linux/init.h>
#include <linux/errno.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/serio.h>
#include <linux/init.h>
#include <linux/pm.h>
#include <linux/delay.h>
#include <linux/ctype.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/power_supply.h>
#include <linux/firmware.h>
#include <linux/regulator/consumer.h>
#include <linux/of_gpio.h>
#include <linux/input/mt.h>
#ifdef CONFIG_SEC_SYSFS
#include <linux/sec_sysfs.h>
#endif
#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
#include <linux/t-base-tui.h>
struct fts_ts_info *tui_tsp_info;
#endif
#include "fts_ts.h"

#if defined(CONFIG_SECURE_TOUCH)
#include <linux/clk.h>
#include <linux/pm_runtime.h>
#include <linux/errno.h>
#include <linux/atomic.h>
#include <soc/qcom/scm.h>
/*#include <asm/system.h>*/

enum subsystem {
	TZ = 1,
	APSS = 3
};

#define TZ_BLSP_MODIFY_OWNERSHIP_ID		3

#endif

#ifdef CONFIG_OF
#ifndef USE_OPEN_CLOSE
#define USE_OPEN_CLOSE
#undef CONFIG_PM
#endif
#endif

#ifdef FTS_SUPPORT_TOUCH_KEY
struct fts_touchkey fts_touchkeys[] = {
	{
		.value = 0x01,
		.keycode = KEY_RECENT,
		.name = "recent",
	},
	{
		.value = 0x02,
		.keycode = KEY_BACK,
		.name = "back",
	},
};
#endif

#ifdef CONFIG_GLOVE_TOUCH
enum TOUCH_MODE {
	FTS_TM_NORMAL = 0,
	FTS_TM_GLOVE,
};
#endif

#ifdef USE_OPEN_CLOSE
static int fts_input_open(struct input_dev *dev);
static void fts_input_close(struct input_dev *dev);
#ifdef USE_OPEN_DWORK
static void fts_open_work(struct work_struct *work);
#endif
#endif

static int fts_stop_device(struct fts_ts_info *info, bool lpmode);
static int fts_start_device(struct fts_ts_info *info);

static void fts_reset(struct fts_ts_info *info, unsigned int ms);
static void fts_reset_work(struct work_struct *work);
static void fts_read_info_work(struct work_struct *work);

static void dump_tsp_rawdata(struct work_struct *work);
struct delayed_work *p_debug_work;

#if (!defined(CONFIG_PM)) && !defined(USE_OPEN_CLOSE)
static int fts_suspend(struct i2c_client *client, pm_message_t mesg);
static int fts_resume(struct i2c_client *client);
#endif

static int fts_wait_for_ready(struct fts_ts_info *info);

#if defined(CONFIG_SECURE_TOUCH)
static irqreturn_t fts_filter_interrupt(struct fts_ts_info *info);

static irqreturn_t fts_interrupt_handler(int irq, void *handle);

static ssize_t fts_secure_touch_enable_show(struct device *dev,
			struct device_attribute *attr, char *buf);

static ssize_t fts_secure_touch_enable_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);

static ssize_t fts_secure_touch_show(struct device *dev,
			struct device_attribute *attr, char *buf);

static struct device_attribute attrs[] = {
	__ATTR(secure_touch_enable, (S_IRUGO | S_IWUSR | S_IWGRP),
			fts_secure_touch_enable_show,
			fts_secure_touch_enable_store),
	__ATTR(secure_touch, (S_IRUGO),
			fts_secure_touch_show,
			NULL),
};

static int fts_change_pipe_owner(struct fts_ts_info *info, enum subsystem subsystem)
{
	/* scm call disciptor */
	struct scm_desc desc;
	int ret = 0;

	/* number of arguments */
	desc.arginfo = SCM_ARGS(2);
	/* BLSPID (1 - 12) */
	desc.args[0] = info->client->adapter->nr - 1;
	/* Owner if TZ or APSS */
	desc.args[1] = subsystem;

	ret = scm_call2(SCM_SIP_FNID(SCM_SVC_TZ, TZ_BLSP_MODIFY_OWNERSHIP_ID), &desc);
	if (ret) {
		input_err(true, &info->client->dev, "%s: return: %d\n", __func__, ret);
		return ret;
	}

	return desc.ret[0];
}


static int fts_secure_touch_clk_prepare_enable(struct fts_ts_info *info)
{
	int ret;

	if (!info->iface_clk || !info->core_clk) {
		input_err(true, &info->client->dev,
			"%s: error clk. iface:%d, core:%d\n", __func__,
			IS_ERR_OR_NULL(info->iface_clk), IS_ERR_OR_NULL(info->core_clk));
		return -ENODEV;
	}

	ret = clk_prepare_enable(info->iface_clk);
	if (ret) {
		input_err(true, &info->client->dev,
			"%s: error on clk_prepare_enable(iface_clk):%d\n", __func__, ret);
		return ret;
	}

	ret = clk_prepare_enable(info->core_clk);
	if (ret) {
		clk_disable_unprepare(info->iface_clk);
		input_err(true, &info->client->dev,
			"%s: error clk_prepare_enable(core_clk):%d\n", __func__, ret);
		return ret;
	}
	return ret;
}

static void fts_secure_touch_clk_disable_unprepare(
		struct fts_ts_info *info)
{
	clk_disable_unprepare(info->core_clk);
	clk_disable_unprepare(info->iface_clk);
}

static ssize_t fts_secure_touch_enable_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct fts_ts_info *info = dev_get_drvdata(dev);

	return scnprintf(buf, PAGE_SIZE, "%d", atomic_read(&info->st_enabled));
}

/*
 * Accept only "0" and "1" valid values.
 * "0" will reset the st_enabled flag, then wake up the reading process.
 * The bus driver is notified via pm_runtime that it is not required to stay
 * awake anymore.
 * It will also make sure the queue of events is emptied in the controller,
 * in case a touch happened in between the secure touch being disabled and
 * the local ISR being ungated.
 * "1" will set the st_enabled flag and clear the st_pending_irqs flag.
 * The bus driver is requested via pm_runtime to stay awake.
 */
static ssize_t fts_secure_touch_enable_store(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	struct fts_ts_info *info = dev_get_drvdata(dev);
	unsigned long value;
	int err = 0;

	if (count > 2) {
		input_err(true, &info->client->dev,
			"%s: cmd length is over (%s,%d)!!\n",
			__func__, buf, (int)strlen(buf));
		return -EINVAL;
	}

	err = kstrtoul(buf, 10, &value);
	if (err != 0) {
		input_err(true, &info->client->dev, "%s: failed to read:%d\n",
					__func__, err);
		return err;
	}

	err = count;

	switch (value) {
	case 0:
		if (atomic_read(&info->st_enabled) == 0) {
			input_err(true, &info->client->dev, "%s: secure_touch is not enabled, pending:%d\n",
				__func__, atomic_read(&info->st_pending_irqs));
			break;
		}

		fts_change_pipe_owner(info, APSS);

		fts_secure_touch_clk_disable_unprepare(info);

		pm_runtime_put_sync(info->client->adapter->dev.parent);

		atomic_set(&info->st_enabled, 0);

		sysfs_notify(&info->input_dev->dev.kobj, NULL, "secure_touch");

		fts_delay(10);

		fts_interrupt_handler(info->client->irq, info);

		complete(&info->st_powerdown);
		complete(&info->st_interrupt);

		input_info(true, &info->client->dev, "%s: secure_touch is disabled\n", __func__);

		break;

	case 1:
		if (info->reset_is_on_going) {
			input_err(true, &info->client->dev, "%s: reset is on goning becuse i2c fail\n",
				__func__);
			return -EBUSY;
		}

		if (atomic_read(&info->st_enabled)) {
			input_err(true, &info->client->dev, "%s: secure_touch is already enabled, pending:%d\n",
				__func__, atomic_read(&info->st_pending_irqs));
			err = -EBUSY;
			break;
		}

		/* syncronize_irq -> disable_irq + enable_irq
		 * concern about timing issue.
		 */
		disable_irq(info->client->irq);

		/* Release All Finger */
		fts_release_all_finger(info);

		if (pm_runtime_get_sync(info->client->adapter->dev.parent) < 0) {
			input_err(true, &info->client->dev, "%s: pm_runtime_get failed\n", __func__);
			err = -EIO;
			enable_irq(info->client->irq);
			break;
		}

		if (fts_secure_touch_clk_prepare_enable(info) < 0) {
			input_err(true, &info->client->dev, "%s: clk_prepare_enable failed\n", __func__);
			pm_runtime_put_sync(info->client->adapter->dev.parent);
			err = -EIO;
			enable_irq(info->client->irq);
			break;
		}

		fts_change_pipe_owner(info, TZ);

		reinit_completion(&info->st_powerdown);
		reinit_completion(&info->st_interrupt);

		atomic_set(&info->st_enabled, 1);
		atomic_set(&info->st_pending_irqs, 0);

		enable_irq(info->client->irq);

		input_info(true, &info->client->dev, "%s: secure_touch is enabled\n", __func__);

		break;

	default:
		input_err(true, &info->client->dev, "%s: unsupported value: %lu\n", __func__, value);
		err = -EINVAL;
		break;
	}

	return err;
}

static ssize_t fts_secure_touch_show(struct device *dev,
						struct device_attribute *attr, char *buf)
{
	struct fts_ts_info *info = dev_get_drvdata(dev);
	int val = 0;

	if (atomic_read(&info->st_enabled) == 0) {
		input_err(true, &info->client->dev, "%s: secure_touch is not enabled, st_pending_irqs: %d\n",
			__func__, atomic_read(&info->st_pending_irqs));
		return -EBADF;
	}

	if (atomic_cmpxchg(&info->st_pending_irqs, -1, 0) == -1) {
		input_err(true, &info->client->dev, "%s: st_pending_irqs: %d\n",
			__func__, atomic_read(&info->st_pending_irqs));
		return -EINVAL;
	}

	if (atomic_cmpxchg(&info->st_pending_irqs, 1, 0) == 1)
		val = 1;

	input_info(true, &info->client->dev, "%s: st_pending_irqs: %d, val: %d\n",
		__func__, atomic_read(&info->st_pending_irqs), val);
	complete(&info->st_interrupt);

	return scnprintf(buf, PAGE_SIZE, "%u", val);
}

static void fts_secure_touch_init(struct fts_ts_info *info)
{
	int ret;

	init_completion(&info->st_powerdown);
	init_completion(&info->st_interrupt);

	info->core_clk = clk_get(&info->client->adapter->dev, "core_clk");
	if (IS_ERR(info->core_clk)) {
		ret = PTR_ERR(info->core_clk);
		input_err(true, &info->client->dev, "%s: error on clk_get(core_clk):%d\n",
			__func__, ret);
		return;
	}

	info->iface_clk = clk_get(&info->client->adapter->dev, "iface_clk");
	if (IS_ERR(info->iface_clk)) {
		ret = PTR_ERR(info->core_clk);
		input_err(true, &info->client->dev, "%s: error on clk_get(iface_clk):%d\n",
			__func__, ret);
		goto err_iface_clk;
	}

	return;

err_iface_clk:
	clk_put(info->core_clk);
	info->core_clk = NULL;
}

static void fts_secure_touch_stop(struct fts_ts_info *info, int blocking)
{
	if (atomic_read(&info->st_enabled)) {
		atomic_set(&info->st_pending_irqs, -1);
		sysfs_notify(&info->input_dev->dev.kobj, NULL, "secure_touch");

		if (blocking)
			wait_for_completion_interruptible(&info->st_powerdown);
	}
}

static irqreturn_t fts_filter_interrupt(struct fts_ts_info *info)
{
	if (atomic_read(&info->st_enabled)) {
		if (atomic_cmpxchg(&info->st_pending_irqs, 0, 1) == 0)
			sysfs_notify(&info->input_dev->dev.kobj, NULL, "secure_touch");
		else
			input_info(true, &info->client->dev, "%s: st_pending_irqs: %d\n",
							__func__, atomic_read(&info->st_pending_irqs));

		return IRQ_HANDLED;
	}
	return IRQ_NONE;
}
#endif

int fts_write_reg(struct fts_ts_info *info,
		  unsigned char *reg, unsigned short num_com)
{
	struct i2c_msg xfer_msg[2];
	int ret;
	int retry = FTS_TS_I2C_RETRY_CNT;

	if (info->touch_stopped) {
		input_err(true, &info->client->dev, "%s: Sensor stopped\n", __func__);
		goto exit;
	}

#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
	if (TRUSTEDUI_MODE_INPUT_SECURED & trustedui_get_current_mode()) {
		input_err(true, &info->client->dev,
			"%s: TSP no accessible from Linux, TUI is enabled!\n", __func__);
		return -EIO;
	}
#endif
#ifdef CONFIG_SECURE_TOUCH
	if (atomic_read(&info->st_enabled)) {
		input_err(true, &info->client->dev,
			"%s: TSP no accessible from Linux, TUI is enabled!\n", __func__);
		return -EIO;
	}
#endif

	mutex_lock(&info->i2c_mutex);

	xfer_msg[0].addr = info->client->addr;
	xfer_msg[0].len = num_com;
	xfer_msg[0].flags = 0;
	xfer_msg[0].buf = reg;

	do {
		ret = i2c_transfer(info->client->adapter, xfer_msg, 1);
		if (ret < 0) {
			info->comm_err_count++;
			input_err(true, &info->client->dev,
				"%s failed(%d). ret:%d, addr:%x, cnt:%d\n",
				__func__, retry, ret, xfer_msg[0].addr, info->comm_err_count);
			usleep_range(10 * 1000, 10 * 1000);
		} else {
			break;
		}
	} while (--retry > 0);

	mutex_unlock(&info->i2c_mutex);

	if (retry == 0) {
		input_err(true, &info->client->dev, "%s: I2C read over retry limit\n", __func__);
		ret = -EIO;
#ifdef USE_POR_AFTER_I2C_RETRY
		if (info->probe_done && !info->reset_is_on_going)
			schedule_delayed_work(&info->reset_work, msecs_to_jiffies(10));
#endif
	}
	return ret;

 exit:
	return 0;
}

int fts_read_reg(struct fts_ts_info *info, unsigned char *reg, int cnum,
		 unsigned char *buf, int num)
{
	struct i2c_msg xfer_msg[2];
	int ret;
	int retry = FTS_TS_I2C_RETRY_CNT;

	if (info->touch_stopped) {
		input_err(true, &info->client->dev, "%s: Sensor stopped\n", __func__);
		goto exit;
	}

#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
	if (TRUSTEDUI_MODE_INPUT_SECURED & trustedui_get_current_mode()) {
		input_err(true, &info->client->dev,
			"%s: TSP no accessible from Linux, TUI is enabled!\n", __func__);
		return -EIO;
	}
#endif
#ifdef CONFIG_SECURE_TOUCH
	if (atomic_read(&info->st_enabled)) {
		input_err(true, &info->client->dev,
			"%s: TSP no accessible from Linux, TUI is enabled!\n", __func__);
		return -EIO;
	}
#endif

	mutex_lock(&info->i2c_mutex);

	xfer_msg[0].addr = info->client->addr;
	xfer_msg[0].len = cnum;
	xfer_msg[0].flags = 0;
	xfer_msg[0].buf = reg;

	xfer_msg[1].addr = info->client->addr;
	xfer_msg[1].len = num;
	xfer_msg[1].flags = I2C_M_RD;
	xfer_msg[1].buf = buf;

	do {
		ret = i2c_transfer(info->client->adapter, xfer_msg, 2);
		if (ret < 0) {
			info->comm_err_count++;
			input_err(true, &info->client->dev,
				"%s failed(%d). ret:%d, addr:%x, cnt:%d\n",
				__func__, retry, ret, xfer_msg[0].addr, info->comm_err_count);
			usleep_range(10 * 1000, 10 * 1000);
		} else {
			break;
		}
	} while (--retry > 0);

	mutex_unlock(&info->i2c_mutex);

	if (retry == 0) {
		input_err(true, &info->client->dev, "%s: I2C read over retry limit\n", __func__);
		ret = -EIO;
#ifdef USE_POR_AFTER_I2C_RETRY
		if (info->probe_done && !info->reset_is_on_going)
			schedule_delayed_work(&info->reset_work, msecs_to_jiffies(10));
#endif
	}
	return ret;

 exit:
	return 0;
}

#ifdef FTS_SUPPORT_STRINGLIB
#ifdef CONFIG_SEC_FACTORY
static void fts_disable_string(struct fts_ts_info *info)
{
	unsigned char regAdd[4] = {0xb0, 0x01, 0x10, 0x8f};
	int ret = 0;

	ret = fts_write_reg(info, &regAdd[0], 4);
	input_info(true, &info->client->dev, "%s: String Library Disabled, ret = %d\n", __func__, ret);
}
#endif

static int fts_read_from_string(struct fts_ts_info *info,
					unsigned short *reg, unsigned char *data, int length)
{
	unsigned char string_reg[3];
	unsigned char *buf;
	int rtn;

#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
	if (TRUSTEDUI_MODE_INPUT_SECURED & trustedui_get_current_mode()) {
		input_err(true, &info->client->dev,
			"%s: TSP no accessible from Linux, TUI is enabled!\n", __func__);
		return -EIO;
	}
#endif
#ifdef CONFIG_SECURE_TOUCH
	if (atomic_read(&info->st_enabled)) {
		input_err(true, &info->client->dev,
			"%s: TSP no accessible from Linux, TUI is enabled!\n", __func__);
		return -EIO;
	}
#endif

	string_reg[0] = 0xD0;
	string_reg[1] = (*reg >> 8) & 0xFF;
	string_reg[2] = *reg & 0xFF;

	buf = kzalloc(length + 1, GFP_KERNEL);
	if (buf == NULL) {
		input_info(true, &info->client->dev,
				"%s: kzalloc error.\n", __func__);
		return -ENOMEM;
	}

	rtn = fts_read_reg(info, string_reg, 3, buf, length + 1);
	if (rtn >= 0)
		memcpy(data, &buf[1], length);

	kfree(buf);
	return rtn;
}

/*
 * int fts_write_to_string(struct fts_ts_info *, unsigned short *, unsigned char *, int)
 * send command or write specfic value to the string area.
 * string area means guest image or display lab firmware.. etc..
 */
static int fts_write_to_string(struct fts_ts_info *info,
					unsigned short *reg, unsigned char *data, int length)
{
	struct i2c_msg xfer_msg[3];
	unsigned char *regAdd;
	int ret;

	if (info->touch_stopped) {
		input_err(true, &info->client->dev, "%s: Sensor stopped\n", __func__);
		return 0;
	}

#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
	if (TRUSTEDUI_MODE_INPUT_SECURED & trustedui_get_current_mode()) {
		input_err(true, &info->client->dev,
			"%s: TSP no accessible from Linux, TUI is enabled!\n", __func__);
		return -EIO;
	}
#endif
#ifdef CONFIG_SECURE_TOUCH
	if (atomic_read(&info->st_enabled)) {
		input_err(true, &info->client->dev,
			"%s: TSP no accessible from Linux, TUI is enabled!\n", __func__);
		return -EIO;
	}
#endif

	regAdd = kzalloc(length + 6, GFP_KERNEL);
	if (regAdd == NULL) {
		input_info(true, &info->client->dev, "%s: kzalloc error.\n", __func__);
		return -ENOMEM;
	}

	mutex_lock(&info->i2c_mutex);

/* msg[0], length 3*/
	regAdd[0] = 0xB3;
	regAdd[1] = 0x20;
	regAdd[2] = 0x01;

	xfer_msg[0].addr = info->client->addr;
	xfer_msg[0].len = 3;
	xfer_msg[0].flags = 0;
	xfer_msg[0].buf = &regAdd[0];
/* msg[0], length 3*/

/* msg[1], length 4*/
	regAdd[3] = 0xB1;
	regAdd[4] = (*reg >> 8) & 0xFF;
	regAdd[5] = *reg & 0xFF;

	memcpy(&regAdd[6], data, length);

/*regAdd[3] : B1 address, [4], [5] : String Address, [6]...: data */

	xfer_msg[1].addr = info->client->addr;
	xfer_msg[1].len = 3 + length;
	xfer_msg[1].flags = 0;
	xfer_msg[1].buf = &regAdd[3];
/* msg[1], length 4*/

	ret = i2c_transfer(info->client->adapter, xfer_msg, 2);
	if (ret == 2) {
		input_dbg(true, &info->client->dev,
				"%s: string command is OK.\n", __func__);

		regAdd[0] = FTS_CMD_NOTIFY;
		regAdd[1] = *reg & 0xFF;
		regAdd[2] = (*reg >> 8) & 0xFF;

		xfer_msg[0].addr = info->client->addr;
		xfer_msg[0].len = 3;
		xfer_msg[0].flags = 0;
		xfer_msg[0].buf = regAdd;

		ret = i2c_transfer(info->client->adapter, xfer_msg, 1);
		if (ret != 1)
			input_err(true, &info->client->dev,
					"%s: string notify is failed.\n", __func__);
		else
			input_info(true, &info->client->dev,
					"%s: string notify is OK[0x%02X].\n", __func__, *data);

	} else {
		input_err(true, &info->client->dev,
				"%s: string command is failed. ret: %d\n", __func__, ret);
	}

	mutex_unlock(&info->i2c_mutex);
	kfree(regAdd);

	return ret;
}

int fts_check_custom_library(struct fts_ts_info *info)
{
	u8 regAdd[3] = {0xD0, 0x00, 0x00};
	u8 data[15] = { 0 };
	int ret = -1;
	unsigned short addr = FTS_CMD_STRING_ACCESS;

	regAdd[2] = 0x7A;
	ret = fts_read_reg(info, regAdd, 3, data, 15);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: failed. ret: %d\n", __func__, ret);
		goto out;
	}

	regAdd[1] = data[2];
	regAdd[2] = data[1];
	ret = fts_read_reg(info, regAdd, 3, data, 5);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: failed. ret: %d\n", __func__, ret);
		goto out;
	}

	input_info(true, &info->client->dev,
			"%s: (%d) %02X%02X %c%c%c%c\n",
			__func__, ret, regAdd[1], regAdd[2], data[1], data[2], data[3], data[4]);

	/* compare model name with device tree */
	if (info->board->model_name)
		ret = strncmp(&data[1], info->board->model_name, 4);

	if (ret == 0)
		info->use_sponge = true;
	else
		info->use_sponge = false;

	if (info->use_sponge) {
		ret = fts_write_to_string(info, &addr, &info->lowpower_flag, sizeof(info->lowpower_flag));
		if (ret < 0)
			input_err(true, &info->client->dev, "%s: failed. ret: %d\n", __func__, ret);
	}

out:
	input_err(true, &info->client->dev, "%s: use %s\n",
				__func__, info->use_sponge ? "SPONGE" : "VENDOR");

	return ret;
}
#endif

void fts_delay(unsigned int ms)
{
	if (ms < 20)
		usleep_range(ms * 1000, ms * 1000);
	else
		msleep(ms);
}

void fts_command(struct fts_ts_info *info, unsigned char cmd)
{
	unsigned char regAdd = 0;
	int ret = 0;

	regAdd = cmd;
	ret = fts_write_reg(info, &regAdd, 1);
	input_info(true, &info->client->dev, "%s: (%02X), ret = %d\n", __func__, cmd, ret);
}

void fts_enable_feature(struct fts_ts_info *info, unsigned char cmd, int enable)
{
	unsigned char regAdd[2] = {0xC1, 0x00};
	int ret = 0;

	if (!enable)
		regAdd[0] = 0xC2;
	regAdd[1] = cmd;

	ret = fts_write_reg(info, &regAdd[0], 2);
	input_info(true, &info->client->dev, "%s: %s (%02X %02X), ret = %d\n", __func__,
		(enable) ? "Enable":"Disable", regAdd[0], regAdd[1], ret);
}

static void fts_set_cover_type(struct fts_ts_info *info, bool enable)
{
	bool enabled = enable;
	input_info(true, &info->client->dev, "%s: %d\n", __func__, info->cover_type);

	switch (info->cover_type) {
	case FTS_VIEW_WIRELESS:
	case FTS_VIEW_COVER:
		fts_enable_feature(info, FTS_FEATURE_COVER_GLASS, enable);
		break;
	case FTS_VIEW_WALLET:
		fts_enable_feature(info, FTS_FEATURE_COVER_WALLET, enable);
		break;
	case FTS_FLIP_WALLET:
	case FTS_LED_COVER:
	case FTS_MONTBLANC_COVER:
		fts_enable_feature(info, FTS_FEATURE_COVER_LED, enable);
		break;
	case FTS_CLEAR_FLIP_COVER:
		fts_enable_feature(info, FTS_FEATURE_COVER_CLEAR_FLIP, enable);
		break;
	case FTS_QWERTY_KEYBOARD_EUR:
	case FTS_QWERTY_KEYBOARD_KOR:
		fts_enable_feature(info, 0x0D, enable);
		break;
	case FTS_CHARGER_COVER:
	case FTS_COVER_NOTHING1:
	case FTS_COVER_NOTHING2:
	default:
		enabled = false;
		input_err(true, &info->client->dev, "%s: not change touch state, %d\n",
				__func__, info->cover_type);
		break;
	}

#ifdef FTS_SUPPORT_PRESSURE_SENSOR
	if (enable && enabled)
		fts_command(info, FTS_CMD_PRESSURE_SENSE_OFF);
	else
		fts_command(info, FTS_CMD_PRESSURE_SENSE_ON);
#endif
}

void fts_set_grip_type(struct fts_ts_info *info, u8 set_type)
{
	u8 mode = G_NONE;

	input_info(true, &info->client->dev, "%s: re-init grip(%d), edh:%d, edg:%d, lan:%d\n", __func__,\
		set_type, info->grip_edgehandler_direction, info->grip_edge_range, info->grip_landscape_mode);

	/* edge handler */
	if (info->grip_edgehandler_direction != 0)
		mode |= G_SET_EDGE_HANDLER;

	if (set_type == GRIP_ALL_DATA) {
		/* edge */
		if (info->grip_edge_range != 60)
			mode |= G_SET_EDGE_ZONE;

		/* dead zone */
		if (info->grip_landscape_mode == 1)	/* default 0 mode, 32 */
			mode |= G_SET_LANDSCAPE_MODE;
		else
			mode |= G_SET_NORMAL_MODE;
	}

	if (mode)
		fts_set_grip_data_to_ic(info, mode);

}

static void fts_wirelesscharger_mode(struct fts_ts_info *info)
{
	unsigned char regAdd[2] = {0xC2, 0x10};
	int ret;

	if (info->wirelesscharger_mode == 0)
		regAdd[0] = 0xC2;
	else
		regAdd[0] = 0xC1;

	input_info(true, &info->client->dev, "%s: Set W-Charger Status CMD[%2X]\n", __func__, regAdd[0]);
	ret = fts_write_reg(info, regAdd, 2);
	if (ret < 0)
		input_err(true, &info->client->dev, "%s: failed. ret: %d\n", __func__, ret);
}

void fts_change_scan_rate(struct fts_ts_info *info, unsigned char cmd)
{
	unsigned char regAdd[2] = {0xC3, 0x00};
	int ret = 0;

	regAdd[1] = cmd;
	ret = fts_write_reg(info, &regAdd[0], 2);

	input_dbg(true, &info->client->dev, "%s: %s (%02X %02X), ret = %d\n", __func__,
		(cmd == FTS_CMD_FAST_SCAN) ? "90Hz" : (cmd == FTS_CMD_SLOW_SCAN) ? "60Hz" : "30Hz",
		regAdd[0], regAdd[1], ret);
}

int fts_systemreset(struct fts_ts_info *info, unsigned int delay)
{
	unsigned char regAdd[4] = { 0xB6, 0x00, 0x28, 0x80 };
	int rc;
#ifdef FTS_SUPPORT_STRINGLIB
	int ret;
	unsigned short addr = FTS_CMD_STRING_ACCESS;
#endif

	fts_write_reg(info, &regAdd[0], 4);
	fts_delay(delay);
	rc = fts_wait_for_ready(info);

#ifdef FTS_SUPPORT_STRINGLIB
	ret = info->fts_write_to_string(info, &addr, &info->lowpower_flag, sizeof(info->lowpower_flag));
	if (ret < 0)
		input_err(true, &info->client->dev, "%s: failed. ret: %d\n", __func__, ret);
#endif
	/* do not consider about sponge read/write fail after syste reset routine. */
	return rc;

}

void fts_interrupt_set(struct fts_ts_info *info, int enable)
{
	unsigned char regAdd[4] = { 0xB6, 0x00, 0x2C, enable };

	if (enable) {
		regAdd[3] = INT_ENABLE_D3;
		input_info(true, &info->client->dev, "%s: Enable\n", __func__);
	} else {
		regAdd[3] = INT_DISABLE_D3;
		input_info(true, &info->client->dev, "%s: Disable\n", __func__);
	}

	fts_write_reg(info, &regAdd[0], 4);
}

static int fts_read_chip_id(struct fts_ts_info *info)
{
	unsigned char regAdd[3] = {0xB6, 0x00, 0x04};
	unsigned char val[7] = {0};
	int ret;

	ret = fts_read_reg(info, regAdd, 3, (unsigned char *)val, 7);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: failed. ret: %d\n",
			__func__, ret);
		return ret;
	}

	input_info(true, &info->client->dev, "%s: %02X%02X%02X = %02X %02X %02X %02X %02X %02X\n", __func__,
	       regAdd[0], regAdd[1], regAdd[2], val[1], val[2], val[3], val[4], val[5], val[6]);

	if (val[1] == FTS_ID0 && val[2] == FTS_ID1)
		input_info(true, &info->client->dev, "%s: %02X %02X\n", __func__, val[1], val[2]);
	else
		return -FTS_ERROR_INVALID_CHIP_ID;

	return ret;
}

int fts_read_analog_chip_id(struct fts_ts_info *info, unsigned char id)
{
	unsigned char regAdd[6] = {0xB3, 0x20, 0x07, 0x00, 0x00, 0x00};
	unsigned char val[4] = {0};
	int ret = -1;

	switch(id) {
	case ANALOG_ID_FTS8:
		ret = fts_write_reg(info, &regAdd[0], 3);
		if (ret < 0) {
			input_err(true, &info->client->dev, "%s: failed. ret: %d\n",
				__func__, ret);
			return ret;
		}

		regAdd[0] = 0xB1;
		regAdd[1] = 0x00;
		regAdd[2] = 0x84;
		ret = fts_write_reg(info, &regAdd[0], 6);
		if (ret < 0) {
			input_err(true, &info->client->dev, "%s: failed. ret: %d\n",
				__func__, ret);
			return ret;
		}

		regAdd[0] = 0xB1;
		regAdd[1] = 0x00;
		regAdd[2] = 0x04;
		ret = fts_read_reg(info, regAdd, 3, (unsigned char *)val, 4);
		if (ret < 0) {
			input_err(true, &info->client->dev, "%s: failed. ret: %d\n",
				__func__, ret);
			return ret;
		}

		input_info(true, &info->client->dev, "%s: %02X%02X%02X = %02X %02X %02X\n", __func__,
			   regAdd[0], regAdd[1], regAdd[2], val[1], val[2], val[3]);

		if (val[1] == id) {
			input_info(true, &info->client->dev, "%s: FTS8 %02X %02X\n", __func__, val[1], val[2]);
			ret = 1;
		}
		break;
	case ANALOG_ID_FTS9:
		regAdd[0] = 0xB6;
		regAdd[1] = 0x00;
		regAdd[2] = 0x89;
		regAdd[3] = 0x14;
		ret = fts_write_reg(info, &regAdd[0], 4);
		if (ret < 0) {
			input_err(true, &info->client->dev, "%s: failed. ret: %d\n",
				__func__, ret);
			return ret;
		}

		regAdd[0] = 0xB3;
		regAdd[1] = 0x20;
		regAdd[2] = 0x03;
		ret = fts_write_reg(info, &regAdd[0], 3);
		if (ret < 0) {
			input_err(true, &info->client->dev, "%s: failed. ret: %d\n",
				__func__, ret);
			return ret;
		}

		regAdd[0] = 0xB1;
		regAdd[1] = 0xF8;
		regAdd[2] = 0x00;
		ret = fts_read_reg(info, regAdd, 3, (unsigned char *)val, 4);
		if (ret < 0) {
			input_err(true, &info->client->dev, "%s: failed. ret: %d\n",
				__func__, ret);
			return ret;
		}

		input_info(true, &info->client->dev, "%s: %02X%02X%02X = %02X %02X %02X\n", __func__,
		       regAdd[0], regAdd[1], regAdd[2], val[1], val[2], val[3]);

		if (val[1] == id) {
			input_info(true, &info->client->dev, "%s: FTS9 %02X %02X\n", __func__, val[1], val[2]);
			ret = 0;
		}
		break;
	default:
		break;
	}
	return ret;
}

static int fts_wait_for_ready(struct fts_ts_info *info)
{
	int rc;
	unsigned char regAdd;
	unsigned char data[FTS_EVENT_SIZE];
	int retry = 0;
	int err_cnt = 0;

	memset(data, 0x0, FTS_EVENT_SIZE);

	regAdd = READ_ONE_EVENT;
	rc = -1;
	while (fts_read_reg(info, &regAdd, 1, (unsigned char *)data, FTS_EVENT_SIZE)) {
		if (data[0] == EVENTID_CONTROLLER_READY) {
			rc = 0;
			break;
		}

		if (data[0] == EVENTID_ERROR) {
			if (data[1] == EVENTID_ERROR_FLASH_CORRUPTION) {
				rc = -FTS_ERROR_EVENT_ID;
				info->boot_crc_check_fail = FTS_BOOT_CRC_FAIL;
				input_err(true, &info->client->dev, "%s: flash corruption:%02X,%02X,%02X\n",
						__func__, data[0], data[1], data[2]);
				break;
			}

			if (err_cnt++ > 32) {
				rc = -FTS_ERROR_EVENT_ID;
				break;
			}
			continue;
		}

		if (retry++ > FTS_RETRY_COUNT) {
			rc = -FTS_ERROR_TIMEOUT;
			input_err(true, &info->client->dev, "%s: Time Over\n", __func__);
			if (info->fts_power_state == FTS_POWER_STATE_LOWPOWER)
				schedule_delayed_work(&info->reset_work, msecs_to_jiffies(10));

			break;
		}
		fts_delay(20);
	}

	input_info(true, &info->client->dev,
		"%s: %02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X\n",
		__func__, data[0], data[1], data[2], data[3],
		data[4], data[5], data[6], data[7]);

	return rc;
}

int fts_get_sysinfo_data(struct fts_ts_info *info, unsigned char sysinfo_addr, unsigned char read_cnt, unsigned char *data)
{
	int ret;
	unsigned char regAdd[3] = { 0xD0, 0x00, sysinfo_addr };
	unsigned char *buff;

	buff = kzalloc(read_cnt + 1, GFP_KERNEL);
	if (!buff) {
		input_err(true, &info->client->dev, "%s: kzalloc error.\n", __func__);
		return -ENOMEM;
	}

	ret = fts_read_reg(info, &regAdd[0], 3, buff, read_cnt + 1);
	if (ret <= 0) {
		input_err(true, &info->client->dev, "%s: failed. ret: %d\n",
			__func__, ret);
		kfree(buff);
		return ret;
	}

	memcpy(data, &buff[1], read_cnt);

	kfree(buff);
	return ret;
}

int fts_get_version_info(struct fts_ts_info *info)
{
	int rc;
	unsigned char regAdd[3];
	unsigned char data[FTS_EVENT_SIZE] = { 0 };
	int retry = 0;

	fts_command(info, FTS_CMD_RELEASEINFO);

	memset(data, 0x0, FTS_EVENT_SIZE);

	regAdd[0] = READ_ONE_EVENT;
	rc = -1;
	while (fts_read_reg(info, &regAdd[0], 1, (unsigned char *)data, FTS_EVENT_SIZE)) {
		if (data[0] == EVENTID_INTERNAL_RELEASE_INFO) {
			/* Internal release Information */
			info->fw_version_of_ic = (data[3] << 8) + data[4];
			info->config_version_of_ic = (data[6] << 8) + data[5];
			info->ic_product_id = data[2];
		} else if (data[0] == EVENTID_EXTERNAL_RELEASE_INFO) {
			/* External release Information */
			info->fw_main_version_of_ic = (data[1] << 8) + data[2];
			rc = 0;
			break;
		}

		if (retry++ > FTS_RETRY_COUNT) {
			rc = -FTS_ERROR_TIMEOUT;
			input_err(true, &info->client->dev, "%s: Time Over\n", __func__);
			break;
		}
	}

	input_info(true, &info->client->dev,
		"%s: [IC] product id: 0x%02X, Firmware Ver: 0x%04X, Config Ver: 0x%04X, Main Ver: 0x%04X\n", __func__,
		info->ic_product_id, info->fw_version_of_ic,
		info->config_version_of_ic, info->fw_main_version_of_ic);

	return rc;
}

/* warmboot crc check() is default disable in FTS8. Enable after power on */
int fts_set_warmboot_crc_enable(struct fts_ts_info *info)
{
	int ret;
	unsigned char regAdd[4] = {0xB6, 0x00, 0x1E, 0x20};

	
	ret = fts_write_reg(info, &regAdd[0], 4);
	if (ret <= 0)
		input_err(true, &info->client->dev, "%s: failed\n", __func__);

	fts_delay(10);

	return ret;
}

#ifdef FTS_SUPPORT_NOISE_PARAM
int fts_get_noise_param_address(struct fts_ts_info *info)
{
	int rc;
	unsigned char regAdd[3];
	unsigned char data[6] = { 0 };
	struct fts_noise_param *noise_param;

	noise_param = (struct fts_noise_param *)&info->noise_param;

	regAdd[0] = 0xD0;
	regAdd[1] = 0x00;
	regAdd[2] = FTS_SI_NOISE_PARAM_ADDR;

	rc = fts_read_reg(info, regAdd, 3, (unsigned char *)data, 4);

	noise_param->pAddr = (unsigned short)(data[1] + (data[2] << 8));

	return rc;
}

static int fts_get_noise_param(struct fts_ts_info *info)
{
	int rc;
	unsigned char regAdd[3];
	unsigned char data[6] = { 0 };
	struct fts_noise_param *noise_param;

	noise_param = (struct fts_noise_param *)&info->noise_param;

	regAdd[0] = 0xB3;
	regAdd[1] = 0x00;
	regAdd[2] = 0x10;
	rc = fts_write_reg(info, regAdd, 3);

	regAdd[0] = 0xB1;
	regAdd[1] = (noise_param->pAddr >> 8) & 0xff;
	regAdd[2] = noise_param->pAddr & 0xff;
	rc = fts_read_reg(info, regAdd, 3, data, 5);

	noise_param->bestRIdx = data[1];
	noise_param->mtNoiseLvl = data[2];
	noise_param->sstNoiseLvl = data[3];
	noise_param->bestRMutual = data[4];

	return rc;
}

static int fts_set_noise_param(struct fts_ts_info *info)
{
	int rc;
	unsigned char regAdd[7];
	struct fts_noise_param *noise_param;

	noise_param = (struct fts_noise_param *)&info->noise_param;

	regAdd[0] = 0xB3;
	regAdd[1] = 0x00;
	regAdd[2] = 0x10;
	rc = fts_write_reg(info, regAdd, 3);

	regAdd[0] = 0xB1;
	regAdd[1] = (noise_param->pAddr >> 8) & 0xff;
	regAdd[2] = noise_param->pAddr & 0xff;
	regAdd[3] = noise_param->bestRIdx;
	regAdd[4] = noise_param->mtNoiseLvl;
	regAdd[5] = noise_param->sstNoiseLvl;
	regAdd[6] = noise_param->bestRMutual;

	rc = fts_write_reg(info, regAdd, 7);

	return 0;
}
#endif

#ifdef FTS_SUPPORT_TOUCH_KEY
void fts_release_all_key(struct fts_ts_info *info)
{
	unsigned char key_recent = TOUCH_KEY_RECENT;
	unsigned char key_back = TOUCH_KEY_BACK;

	if (info->board->support_mskey && info->tsp_keystatus != TOUCH_KEY_NULL) {
		if (info->tsp_keystatus & key_recent) {
			input_report_key(info->input_dev, KEY_RECENT, KEY_RELEASE);
			input_info(true, &info->client->dev, "[TSP_KEY] Recent R!\n");
		}

		if (info->tsp_keystatus & key_back) {
			input_report_key(info->input_dev, KEY_BACK, KEY_RELEASE);
			input_info(true, &info->client->dev, "[TSP_KEY] back R!\n");
		}

		input_sync(info->input_dev);

		info->tsp_keystatus = TOUCH_KEY_NULL;
	}
}
#endif

/* Added for samsung dependent codes such as Factory test,
 * Touch booster, Related debug sysfs.
 */
#include "fts_sec.c"

static int fts_init(struct fts_ts_info *info)
{
	unsigned char retry = 3;
	unsigned char val[16] = { 0 };
	unsigned char regAdd[8] = { 0 };
	int rc;

	do {
		fts_set_warmboot_crc_enable(info);

		rc = fts_systemreset(info, 10);

		/* below register address is  used only fts8cd56. */
		regAdd[0] = 0xB6;
		regAdd[1] = 0x00;
		regAdd[2] = 0x08;

		fts_read_reg(info, regAdd, 3, (unsigned char *)val, 3);
		if (!val[1] && !val[2]) {
			input_info(true, &info->client->dev, "%s: corrunption:%02X%02X\n",
				__func__, val[1], val[2]);
			info->boot_crc_check_fail = FTS_BOOT_CRC_FAIL;
		}
		memset(regAdd, 0x00, 8);

		/* about fts_systemreset() */
		if (rc != FTS_NOT_ERROR) {
			info->boot_crc_check_fail = FTS_BOOT_CRC_FAIL;

			fts_reset(info, 20);

			if (rc == -FTS_ERROR_EVENT_ID) {
				info->fw_version_of_ic = 0;
				info->config_version_of_ic = 0;
				info->fw_main_version_of_ic = 0;
			}
		} else {
			fts_get_version_info(info);
			break;
		}
	} while (retry--);

	if (retry == 0) {
		input_err(true, &info->client->dev, "%s: Failed to system reset\n", __func__);
		return FTS_ERROR_TIMEOUT;
	}

	rc = fts_read_chip_id(info);
	if (rc < 0) {
		fts_reset(info, 500);	/* Delay to discharge the IC from ESD or On-state.*/

		input_err(true, &info->client->dev, "%s: Reset caused by chip id error\n", __func__);

		rc = fts_read_chip_id(info);
		if (rc < 0)
			return 1;
	}

	rc = fts_fw_update_on_probe(info);
	if (rc < 0)
		input_err(true, &info->client->dev, "%s: Failed to firmware update\n",
				__func__);

#ifdef SEC_TSP_FACTORY_TEST
	rc = fts_get_channel_info(info);
	if (rc >= 0) {
		input_info(true, &info->client->dev, "%s: Sense(%02d) Force(%02d)\n", __func__,
			   info->SenseChannelLength, info->ForceChannelLength);
	} else {
		input_err(true, &info->client->dev, "%s: read failed rc = %d\n", __func__, rc);
		return 1;
	}

	info->pFrame = kzalloc(info->SenseChannelLength * info->ForceChannelLength * 2, GFP_KERNEL);
	if (info->pFrame == NULL) {
		input_err(true, &info->client->dev, "%s: pFrame kzalloc Failed\n", __func__);
		return 1;
	}
	info->cx_data = kzalloc(info->SenseChannelLength * info->ForceChannelLength, GFP_KERNEL);
	if (!info->cx_data)
		input_err(true, &info->client->dev, "%s: cx_data kzalloc Failed\n", __func__);

#if defined(FTS_SUPPORT_STRINGLIB) && defined(CONFIG_SEC_FACTORY)
	fts_disable_string(info);
#endif
#endif

	fts_command(info, FLUSHBUFFER);
	fts_delay(10);
	fts_command(info, SENSEON);
#ifdef FTS_SUPPORT_PRESSURE_SENSOR
	fts_command(info, FTS_CMD_PRESSURE_SENSE_ON);
#endif
#ifdef FTS_SUPPORT_TOUCH_KEY
	if (info->board->support_mskey)
		fts_command(info, FTS_CMD_KEY_SENSE_ON);
#endif

#ifdef FTS_SUPPORT_NOISE_PARAM
	fts_get_noise_param_address(info);
#endif
	/* fts driver set functional feature */
	info->touch_count = 0;
#ifdef FTS_SUPPORT_HOVER
	info->hover_enabled = false;
	info->hover_ready = false;
#endif
	info->flip_enable = false;
	info->mainscr_disable = false;

	info->deepsleep_mode = false;
	info->wirelesscharger_mode = false;

	if (info->board->support_pressure) {
		info->lowpower_flag |= FTS_MODE_PRESSURE;
		info->pressure_caller_id = -1;
	} else {
		info->lowpower_flag = 0x00;
		info->pressure_caller_id = 0;
	}
	info->external_factory = false;

#ifdef FTS_SUPPORT_TOUCH_KEY
	info->tsp_keystatus = 0x00;
#endif

	fts_command(info, FORCECALIBRATION);

	fts_interrupt_set(info, INT_ENABLE);

	memset(val, 0x0, 4);
	regAdd[0] = READ_STATUS;
	fts_read_reg(info, regAdd, 1, (unsigned char *)val, 4);
	input_info(true, &info->client->dev, "%s: ReadStatus(0x84) : %02X %02X %02X %02X\n", __func__, val[0],
	       val[1], val[2], val[3]);

	memset(val, 0x0, 4);
	fts_get_sysinfo_data(info, FTS_SI_REPORT_PRESSURE_RAW_DATA, 2, val);
	if (val[0] == 1)
		info->report_pressure = true;

	memset(val, 0x0, 4);
	rc = fts_get_sysinfo_data(info, FTS_SI_POSITION_RESOLUTION_Y, 4, val);
	if (rc < 0) {
		input_err(true, &info->client->dev, "%s: no not read resolution information\n", __func__);
	} else {
/*
		info->board->max_y = (val[0] | (val[1] << 8));
		info->board->max_x = (val[2] | (val[3] << 8));
*/
		input_info(true, &info->client->dev, "%s: resolution:(IC)%02X%02X%02X%02X, (DT)x:%d,y:%d\n",
				__func__, val[0], val[1], val[2], val[3], info->board->max_x, info->board->max_y);
	}

	input_info(true, &info->client->dev, "%s: Initialized\n", __func__);

	return 0;
}

static void fts_debug_msg_event_handler(struct fts_ts_info *info,
				      unsigned char data[])
{
#ifdef CONFIG_SAMSUNG_PRODUCT_SHIP
	input_dbg(true, &info->client->dev,
	       "%s: %02X %02X %02X %02X %02X %02X %02X %02X\n", __func__,
	       data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
#else
	input_info(true, &info->client->dev,
	       "%s: %02X %02X %02X %02X %02X %02X %02X %02X\n", __func__,
	       data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
#endif
}

static unsigned char fts_event_handler_type_b(struct fts_ts_info *info,
					      unsigned char data[],
					      unsigned char LeftEvent)
{
	unsigned char EventNum = 0;
	unsigned char TouchID = 0, EventID = 0, status = 0, event_type = 0;
	unsigned char LastLeftEvent = 0;
	int x = 0, y = 0, z = 0;
	int bw = 0, bh = 0, palm = 0;
	int custom = 0;
	unsigned char force_strength = 1;
#ifdef FTS_SUPPORT_STRINGLIB
	unsigned short string_addr;
	unsigned char string_event_id;
	unsigned char string_data[5];
#endif

	for (EventNum = 0; EventNum < LeftEvent; EventNum++) {

		/* for event debugging */
		if (info->debug_string & 0x1)
			input_info(true, &info->client->dev, "[%d] %2x %2x %2x %2x %2x %2x %2x %2x\n",
				EventNum, data[EventNum * FTS_EVENT_SIZE], data[EventNum * FTS_EVENT_SIZE+1],
				data[EventNum * FTS_EVENT_SIZE+2], data[EventNum * FTS_EVENT_SIZE+3],
				data[EventNum * FTS_EVENT_SIZE+4], data[EventNum * FTS_EVENT_SIZE+5],
				data[EventNum * FTS_EVENT_SIZE+6], data[EventNum * FTS_EVENT_SIZE+7]);

		EventID = data[EventNum * FTS_EVENT_SIZE] & 0x0F;

		if ((EventID >= 3) && (EventID <= 5)) {
			LastLeftEvent = 0;
			TouchID = (data[EventNum * FTS_EVENT_SIZE] >> 4) & 0x0F;

			if (info->fts_power_state != FTS_POWER_STATE_ACTIVE) {
				input_err(true, &info->client->dev, "%s: skip id %d in low power mode\n", __func__, EventID);
				fts_release_all_finger(info);
				continue;
			}
		} else {
			LastLeftEvent = data[7 + EventNum * FTS_EVENT_SIZE] & 0x0F;
			TouchID = data[1 + EventNum * FTS_EVENT_SIZE] & 0x0F;
			EventID = data[EventNum * FTS_EVENT_SIZE] & 0xFF;
			status = data[1 + EventNum * FTS_EVENT_SIZE] & 0xFF;
		}

		event_type = data[1 + EventNum * FTS_EVENT_SIZE];

		switch (EventID) {
		case EVENTID_NO_EVENT:
			break;

		case EVENTID_GESTURE_WAKEUP:
			input_info(true, &info->client->dev, "%s: GESTURE_WAKEUP detected![EventID=%x]\n", __func__, EventID);

			/* defined in Istor & JF-synaptics */
			info->scrub_id = 0x07;

			input_report_key(info->input_dev, KEY_BLACK_UI_GESTURE, 1);
			input_sync(info->input_dev);
			input_report_key(info->input_dev, KEY_BLACK_UI_GESTURE, 0);
			input_sync(info->input_dev);
			break;

		case EVENTID_PRESSURE:
			if (event_type == 0x01)
				info->scrub_id = SPECIAL_EVENT_TYPE_PRESSURE_TOUCHED;
			else
				info->scrub_id = SPECIAL_EVENT_TYPE_PRESSURE_RELEASED;

			info->scrub_x = (data[4] & 0xFF) << 8 | (data[3] & 0xFF);
			info->scrub_y = (data[6] & 0xFF) << 8 | (data[5] & 0xFF);

			input_report_key(info->input_dev, KEY_BLACK_UI_GESTURE, 1);
			input_sync(info->input_dev);
			input_report_key(info->input_dev, KEY_BLACK_UI_GESTURE, 0);

			input_info(true, &info->client->dev,
				"%s: [PRESSURE] %02X %02X %02X %02X %02X %02X %02X %02X\n", __func__,
				data[EventNum * FTS_EVENT_SIZE],
				data[EventNum * FTS_EVENT_SIZE+1],
				data[EventNum * FTS_EVENT_SIZE+2],
				data[EventNum * FTS_EVENT_SIZE+3],
				data[EventNum * FTS_EVENT_SIZE+4],
				data[EventNum * FTS_EVENT_SIZE+5],
				data[EventNum * FTS_EVENT_SIZE+6],
				data[EventNum * FTS_EVENT_SIZE+7]);
			break;
		case EVENTID_PRESSURE_MAX:
			info->pressure_max = (data[EventNum * FTS_EVENT_SIZE + 2] & 0xFF) << 8 |
									(data[EventNum * FTS_EVENT_SIZE + 1] & 0xFF);
			break;

		case EVENTID_GESTURE_HOME:
			info->scrub_x = (data[4] & 0xFF) << 8 | (data[3] & 0xFF);
			info->scrub_y = (data[6] & 0xFF) << 8 | (data[5] & 0xFF);

			if (event_type == 0x01) {
				input_report_key(info->input_dev, KEY_HOMEPAGE, 1);
				info->scrub_id = SPECIAL_EVENT_TYPE_AOD_HOMEKEY_PRESS;
			}
			else {
				input_report_key(info->input_dev, KEY_HOMEPAGE, 0);
				info->scrub_id = SPECIAL_EVENT_TYPE_AOD_HOMEKEY_RELEASE;
			}

			input_report_key(info->input_dev, KEY_BLACK_UI_GESTURE, 1);

			input_sync(info->input_dev);

			input_report_key(info->input_dev, KEY_BLACK_UI_GESTURE, 0);

			input_info(true, &info->client->dev,
				"%s: [HOME key] %02X %02X %02X %02X %02X %02X %02X %02X\n", __func__,
				data[EventNum * FTS_EVENT_SIZE],
				data[EventNum * FTS_EVENT_SIZE+1],
				data[EventNum * FTS_EVENT_SIZE+2],
				data[EventNum * FTS_EVENT_SIZE+3],
				data[EventNum * FTS_EVENT_SIZE+4],
				data[EventNum * FTS_EVENT_SIZE+5],
				data[EventNum * FTS_EVENT_SIZE+6],
				data[EventNum * FTS_EVENT_SIZE+7]);
			break;

#ifdef FTS_SUPPORT_TOUCH_KEY
		case EVENTID_MSKEY:
			if (info->board->support_mskey) {
				unsigned char input_keys;

				input_keys = data[2 + EventNum * FTS_EVENT_SIZE];

				if (input_keys == 0x00)
					fts_release_all_key(info);
				else {
					unsigned char change_keys;
					unsigned char key_state;
					unsigned char key_recent = TOUCH_KEY_RECENT;
					unsigned char key_back = TOUCH_KEY_BACK;

					change_keys = input_keys ^ info->tsp_keystatus;

					if (change_keys & key_recent) {
						key_state = input_keys & key_recent;

						input_report_key(info->input_dev, KEY_RECENT, key_state != 0 ? KEY_PRESS : KEY_RELEASE);
						input_info(true, &info->client->dev, "[TSP_KEY] RECENT %s\n", key_state != 0 ? "P" : "R");
					}

					if (change_keys & key_back) {
						key_state = input_keys & key_back;

						input_report_key(info->input_dev, KEY_BACK, key_state != 0 ? KEY_PRESS : KEY_RELEASE);
						input_info(true, &info->client->dev, "[TSP_KEY] BACK %s\n", key_state != 0 ? "P" : "R");
					}

					input_sync(info->input_dev);
				}

				info->tsp_keystatus = input_keys;
			}
			break;
#endif
		case EVENTID_DEBUG:
			if (info->board->support_sidegesture) {
				if ((event_type == FTS_SIDEGESTURE_EVENT_SINGLE_STROKE) ||
					(event_type == FTS_SIDEGESTURE_EVENT_DOUBLE_STROKE) ||
					(event_type == FTS_SIDEGESTURE_EVENT_INNER_STROKE)) {
					int direction = data[2 + EventNum * FTS_EVENT_SIZE];
				//	int distance = *(int *)&data[3 + EventNum * FTS_EVENT_SIZE];

					if (direction)
						input_report_key(info->input_dev, KEY_SIDE_GESTURE_RIGHT, 1);
					else
						input_report_key(info->input_dev, KEY_SIDE_GESTURE_LEFT, 1);

					input_info(true, &info->client->dev,
						"%s: [Gesture] %02X %02X %02X %02X %02X %02X %02X %02X\n", __func__,
						data[EventNum * FTS_EVENT_SIZE],
						data[EventNum * FTS_EVENT_SIZE+1],
						data[EventNum * FTS_EVENT_SIZE+2],
						data[EventNum * FTS_EVENT_SIZE+3],
						data[EventNum * FTS_EVENT_SIZE+4],
						data[EventNum * FTS_EVENT_SIZE+5],
						data[EventNum * FTS_EVENT_SIZE+6],
						data[EventNum * FTS_EVENT_SIZE+7]);

					input_sync(info->input_dev);
					fts_delay(1);

					if (direction)
						input_report_key(info->input_dev, KEY_SIDE_GESTURE_RIGHT, 0);
					else
						input_report_key(info->input_dev, KEY_SIDE_GESTURE_LEFT, 0);
				}
			} else {
				fts_debug_msg_event_handler(info,
						  &data[EventNum * FTS_EVENT_SIZE]);
			}
			break;
		case EVENTID_ERROR:
			if (data[1 + EventNum * FTS_EVENT_SIZE] == 0x08) {
				/* Get Auto tune fail event */
				if (data[2 + EventNum * FTS_EVENT_SIZE] == 0x00)
					input_info(true, &info->client->dev, "%s: Fail Mutual Auto tune\n", __func__);
				else if (data[2 + EventNum * FTS_EVENT_SIZE] == 0x01)
					input_info(true, &info->client->dev, "%s: Fail Self Auto tune\n", __func__);
			} else if (data[1 + EventNum * FTS_EVENT_SIZE] == 0x09) {
				/* Get detect SYNC fail event */
				input_info(true, &info->client->dev, "%s: Fail detect SYNC\n", __func__);
			}
			break;
#ifdef FTS_SUPPORT_HOVER
		case EVENTID_HOVER_ENTER_POINTER:
		case EVENTID_HOVER_MOTION_POINTER:
			x = ((data[4 + EventNum * FTS_EVENT_SIZE] & 0xF0) >> 4)
			    | ((data[2 + EventNum * FTS_EVENT_SIZE]) << 4);
			y = ((data[4 + EventNum * FTS_EVENT_SIZE] & 0x0F) |
			     ((data[3 + EventNum * FTS_EVENT_SIZE]) << 4));

			z = data[5 + EventNum * FTS_EVENT_SIZE];

			input_mt_slot(info->input_dev, 0);
			input_mt_report_slot_state(info->input_dev,
						   MT_TOOL_FINGER, 1);

			input_report_key(info->input_dev, BTN_TOUCH, 0);
			input_report_key(info->input_dev, BTN_TOOL_FINGER, 1);

			input_report_abs(info->input_dev, ABS_MT_POSITION_X, x);
			input_report_abs(info->input_dev, ABS_MT_POSITION_Y, y);
			input_report_abs(info->input_dev, ABS_MT_DISTANCE, 255 - z);
			break;

		case EVENTID_HOVER_LEAVE_POINTER:
			input_mt_slot(info->input_dev, 0);
			input_mt_report_slot_state(info->input_dev,
						   MT_TOOL_FINGER, 0);
			break;
#endif
		case EVENTID_ENTER_POINTER:
			info->touch_count++;
			info->all_finger_count++;

			info->max_z_value = max((unsigned int)z, info->max_z_value);
			info->min_z_value = min((unsigned int)z, info->min_z_value);
			info->sum_z_value += (unsigned int)z;

			do_gettimeofday(&info->time_pressed[TouchID]);

			if ((info->touch_count > 4) && (info->check_multi == 0)) {
				info->check_multi = 1;
				info->multi_count++;
			}
		case EVENTID_MOTION_POINTER:
			if (info->touch_count == 0) {
				input_err(true, &info->client->dev, "%s: count 0\n", __func__);
				fts_release_all_finger(info);
				break;
			}

			if ((EventID == EVENTID_MOTION_POINTER) &&
				(info->finger[TouchID].state == EVENTID_LEAVE_POINTER)) {
				input_err(true, &info->client->dev, "%s: state leave but point is moved.\n", __func__);
				break;
			}

			x = data[1 + EventNum * FTS_EVENT_SIZE] +
				((data[2 + EventNum * FTS_EVENT_SIZE] & 0x0f) << 8);
			y = ((data[2 + EventNum * FTS_EVENT_SIZE] & 0xf0) >> 4)
				+ (data[3 + EventNum * FTS_EVENT_SIZE] << 4);
			bw = data[4 + EventNum * FTS_EVENT_SIZE];
			bh = data[5 + EventNum * FTS_EVENT_SIZE];
			palm = (data[6 + EventNum * FTS_EVENT_SIZE] >> 7) & 0x01;
			z = data[7 + EventNum * FTS_EVENT_SIZE];

#ifdef FTS_SUPPORT_STRINGLIB
			if (info->pressure_setting_mode) {
				unsigned short addr;
				unsigned char data[2] = { 0 };
				short temp;
				int ret;

				addr = FTS_CMD_STRING_ACCESS + FTS_CMD_OFFSET_PRESSURE_DATA;
				data[0] = data[1] = 0;

				ret = info->fts_read_from_string(info, &addr, data, sizeof(data));
				if (ret < 0)
					input_err(true, &info->client->dev, "%s: i2c read sponge event failed\n", __func__);

				temp = (data[1] << 8 | data[0]);
				if (temp < 0)
					force_strength = 0;
				else if (temp > 255)
					force_strength = 255;
				else
					force_strength = temp;

				input_dbg(true, &info->client->dev, "%s: force_strength: %d\n", __func__, force_strength);
			}
#endif

			input_report_key(info->input_dev, BTN_TOUCH, 1);
			input_mt_slot(info->input_dev, TouchID);
			input_mt_report_slot_state(info->input_dev,
						   MT_TOOL_FINGER, 1 + (palm << 1));

			input_report_key(info->input_dev, BTN_TOOL_FINGER, 1);
			input_report_abs(info->input_dev, ABS_MT_POSITION_X, x);
			input_report_abs(info->input_dev, ABS_MT_POSITION_Y, y);

			input_report_abs(info->input_dev,
					 ABS_MT_TOUCH_MAJOR, max(bw, bh));

			input_report_abs(info->input_dev,
					 ABS_MT_TOUCH_MINOR, min(bw, bh));

			if (info->brush_mode)
				custom = (force_strength << 8) | ((z & 0xFF) << 1) | palm;
			else
				custom = (force_strength << 8) | (BRUSH_Z_DATA << 1) | palm;

			input_report_abs(info->input_dev, ABS_MT_CUSTOM, custom);

			if (info->board->support_mt_pressure) {
				int pressure = z & 0xFF;

#if defined(FTS_SUPPORT_PRESSURE_SENSOR) && !defined(CONFIG_SEC_FACTORY)
				pressure = fts_read_pressure_data(info);
#endif
				if (pressure <= 0)
					pressure = 1;

				if ((info->finger[TouchID].lx != x) || (info->finger[TouchID].ly != y))
					input_report_abs(info->input_dev, ABS_MT_PRESSURE, pressure);
				else
					input_report_abs(info->input_dev, ABS_MT_PRESSURE, info->finger[TouchID].lp);

				info->finger[TouchID].lp = pressure;
			}

			info->finger[TouchID].lx = x;
			info->finger[TouchID].ly = y;

			break;

		case EVENTID_LEAVE_POINTER:
			if (info->touch_count <= 0) {
				input_err(true, &info->client->dev, "%s: count 0\n", __func__);
				fts_release_all_finger(info);
				break;
			}

			do_gettimeofday(&info->time_released[TouchID]);
			
			if (info->time_longest < (info->time_released[TouchID].tv_sec - info->time_pressed[TouchID].tv_sec))
				info->time_longest = (info->time_released[TouchID].tv_sec - info->time_pressed[TouchID].tv_sec);

			info->touch_count--;

			input_mt_slot(info->input_dev, TouchID);

			if (info->board->support_mt_pressure)
				input_report_abs(info->input_dev, ABS_MT_PRESSURE, 0);

			input_mt_report_slot_state(info->input_dev,
						   MT_TOOL_FINGER, 0);

			if (info->touch_count == 0) {
				/* Clear BTN_TOUCH when All touch are released  */
				input_report_key(info->input_dev, BTN_TOUCH, 0);
				input_report_key(info->input_dev, BTN_TOOL_FINGER, 0);

				info->check_multi = 0;

				if (info->board->support_sidegesture) {
					input_report_key(info->input_dev, KEY_SIDE_GESTURE, 0);
					input_report_key(info->input_dev, KEY_SIDE_GESTURE_RIGHT, 0);
					input_report_key(info->input_dev, KEY_SIDE_GESTURE_LEFT, 0);
				}
			}
			break;
		case EVENTID_STATUS_EVENT:
			fts_debug_msg_event_handler(info, &data[EventNum * FTS_EVENT_SIZE]);

			if(status == STATUS_EVENT_ESD_DETECT) {
				input_err(true, &info->client->dev,
					"%s: ESD detected!  %02X %02X %02X %02X %02X %02X %02X %02X\n", __func__,
					data[EventNum * FTS_EVENT_SIZE],
					data[EventNum * FTS_EVENT_SIZE+1],
					data[EventNum * FTS_EVENT_SIZE+2],
					data[EventNum * FTS_EVENT_SIZE+3],
					data[EventNum * FTS_EVENT_SIZE+4],
					data[EventNum * FTS_EVENT_SIZE+5],
					data[EventNum * FTS_EVENT_SIZE+6],
					data[EventNum * FTS_EVENT_SIZE+7]);

				schedule_delayed_work(&info->reset_work, msecs_to_jiffies(10));
			} else if (status == STATUS_EVENT_RAW_DATA_READY) {
				unsigned char regAdd[4] = {0xB0, 0x01, 0x29, 0x01};

				fts_write_reg(info, &regAdd[0], 4);
#ifdef FTS_SUPPORT_HOVER
				info->hover_ready = true;
#endif

				input_info(true, &info->client->dev, "%s: Received the Hover Raw Data Ready Event\n", __func__);
			} else if (status == STATUS_EVENT_GLOVE_MODE) {
#ifdef CONFIG_GLOVE_TOUCH
				if (data[2 + EventNum * FTS_EVENT_SIZE] == 0x01)
					info->touch_mode = FTS_TM_GLOVE;
				else
					info->touch_mode = FTS_TM_NORMAL;

				input_report_switch(info->input_dev, SW_GLOVE, info->touch_mode);
#endif
			} else if (status == STATUS_EVENT_FORCE_CAL_TRIGGER) {
				input_info(true, &info->client->dev, "%s: Received Force Calibration trigger Event\n", __func__);
			} else if (status == STATUS_EVENT_FORCE_CAL_DONE_D3) {
				input_info(true, &info->client->dev, "%s: Received Force Calibration finished Event\n", __func__);
			} else if (status == STATUS_EVENT_WETMODE) {
				if (data[2 + EventNum * FTS_EVENT_SIZE] == 0x01) {
					input_info(true, &info->client->dev, "%s: Received Wet Mode On Event\n", __func__);
					info->wet_mode = true;
					info->wet_count++;
				}
				else {
					input_info(true, &info->client->dev, "%s: Received Wet Mode Off Event\n", __func__);
					info->wet_mode = false;
				}
			}
			break;

#ifdef SEC_TSP_FACTORY_TEST
		case EVENTID_RESULT_READ_REGISTER:

			break;
#endif

#ifdef FTS_SUPPORT_STRINGLIB
		case EVENTID_FROM_STRING:
			string_addr = FTS_CMD_STRING_ACCESS + 1;
			fts_read_from_string(info, &string_addr, &string_event_id, sizeof(string_event_id));
			input_dbg(true, &info->client->dev,
					"%s: [String] %02X %02X %02X %02X %02X %02X %02X %02X || %04X: %02X\n",
					__func__, data[0], data[1], data[2], data[3],
					data[4], data[5], data[6], data[7],
					string_addr, string_event_id);

			switch (string_event_id) {
			case FTS_STRING_EVENT_AOD_TRIGGER:
				string_addr = FTS_CMD_STRING_ACCESS + 10;
				fts_read_from_string(info, &string_addr, string_data, sizeof(string_data));

				input_report_key(info->input_dev, KEY_BLACK_UI_GESTURE, 1);
				info->scrub_id = SPECIAL_EVENT_TYPE_AOD_DOUBLETAB;
				info->scrub_x = (string_data[1] & 0xFF) << 8 | (string_data[0] & 0xFF);
				info->scrub_y = (string_data[3] & 0xFF) << 8 | (string_data[2] & 0xFF);

#ifdef CONFIG_SAMSUNG_PRODUCT_SHIP
				input_info(true, &info->client->dev, "%s: AOD[%d]\n", __func__,	info->scrub_id);
#else
				input_info(true, &info->client->dev, "%s: AOD[%d %d %d]\n", __func__,
						info->scrub_id, info->scrub_x, info->scrub_y);
#endif
				info->all_aod_tap_count++;
				break;
			case FTS_STRING_EVENT_SINGLETAP:
				string_addr = FTS_CMD_STRING_ACCESS + 10;
				fts_read_from_string(info, &string_addr, string_data, sizeof(string_data));

				input_report_key(info->input_dev, KEY_BLACK_UI_GESTURE, 1);
				info->scrub_id = SPECIAL_EVENT_TYPE_SINGLETAP;
				info->scrub_x = (string_data[1] & 0xFF) << 8 | (string_data[0] & 0xFF);
				info->scrub_y = (string_data[3] & 0xFF) << 8 | (string_data[2] & 0xFF);

#ifdef CONFIG_SAMSUNG_PRODUCT_SHIP
				input_info(true, &info->client->dev, "%s: SINGLE_TAP[%d]\n", __func__, info->scrub_id);
#else
				input_info(true, &info->client->dev, "%s: SINGLE_TAP[%d %d %d]\n", __func__,
						info->scrub_id, info->scrub_x, info->scrub_y);
#endif
				info->all_aod_tap_count++;
				break;
			case FTS_STRING_EVENT_SPAY:
				input_report_key(info->input_dev, KEY_BLACK_UI_GESTURE, 1);
				info->scrub_id = SPECIAL_EVENT_TYPE_SPAY;

				input_info(true, &info->client->dev, "%s: SPAY[%d]\n", __func__, info->scrub_id);
				break;
			case FTS_STRING_EVENT_PRESSURE_TOUCHED:
			case FTS_STRING_EVENT_PRESSURE_RELEASED:
				string_addr = FTS_CMD_STRING_ACCESS + 82;
				fts_read_from_string(info, &string_addr, string_data, sizeof(string_data));

				if (info->fts_power_state == FTS_POWER_STATE_ACTIVE) {
					if (string_event_id & FTS_STRING_EVENT_PRESSURE_TOUCHED) {
						info->all_force_count++;

						if (info->scrub_id == SPECIAL_EVENT_TYPE_PRESSURE_TOUCHED) {
							input_info(true, &info->client->dev, "%s: Skip duplicate PRESSURE[%d]\n", __func__, info->scrub_id);
							break;
						} else {
							info->scrub_id = SPECIAL_EVENT_TYPE_PRESSURE_TOUCHED;
						}
					} else {
						if (info->scrub_id == SPECIAL_EVENT_TYPE_PRESSURE_TOUCHED) {
							info->scrub_id = SPECIAL_EVENT_TYPE_PRESSURE_RELEASED;
						}
						else if (info->scrub_id == SPECIAL_EVENT_TYPE_AOD_HOMEKEY_PRESS) {
							input_report_key(info->input_dev, KEY_HOMEPAGE, 0);
							info->scrub_id = SPECIAL_EVENT_TYPE_AOD_HOMEKEY_RELEASE;
						} else {
							info->scrub_id = 0;
							break;
						}
					}
				} else {
					if (info->scrub_id == SPECIAL_EVENT_TYPE_AOD_HOMEKEY_PRESS) {
						input_report_key(info->input_dev, KEY_HOMEPAGE, 0);
						info->scrub_id = SPECIAL_EVENT_TYPE_AOD_HOMEKEY_RELEASE;
					} else {
						input_report_key(info->input_dev, KEY_HOMEPAGE, 1);
						info->scrub_id = SPECIAL_EVENT_TYPE_AOD_HOMEKEY_PRESS;
						info->all_force_count++;
					}
				}

				info->scrub_x = (string_data[1] & 0xFF) << 8 | (string_data[0] & 0xFF);
				info->scrub_y = (string_data[3] & 0xFF) << 8 | (string_data[2] & 0xFF);

				input_report_key(info->input_dev, KEY_BLACK_UI_GESTURE, 1);

#ifdef CONFIG_SAMSUNG_PRODUCT_SHIP
				input_info(true, &info->client->dev, "%s: PRESSURE[%d]\n", __func__, info->scrub_id);
#else
				input_info(true, &info->client->dev, "%s: PRESSURE[%d %d %d]\n", __func__,
						info->scrub_id, info->scrub_x, info->scrub_y);
#endif
				break;
			default:
				input_info(true, &info->client->dev,
					"%s: [No event] %02X %02X %02X %02X %02X %02X %02X %02X || %04X: %02X\n",
					__func__, data[0], data[1], data[2], data[3],
					data[4], data[5], data[6], data[7],
					string_addr, string_event_id);
				break;
			}

			input_sync(info->input_dev);
			input_report_key(info->input_dev, KEY_BLACK_UI_GESTURE, 0);

			break;

		case EVENTID_FROM_SPONGE:
			input_dbg(true, &info->client->dev, "%s: [Sponge] %2x %2x %2x %2x %2x %2x %2x %2x\n",
				__func__, data[EventNum * FTS_EVENT_SIZE], data[EventNum * FTS_EVENT_SIZE + 1],
				data[EventNum * FTS_EVENT_SIZE + 2], data[EventNum * FTS_EVENT_SIZE + 3],
				data[EventNum * FTS_EVENT_SIZE + 4], data[EventNum * FTS_EVENT_SIZE + 5],
				data[EventNum * FTS_EVENT_SIZE + 6], data[EventNum * FTS_EVENT_SIZE + 7]);

			switch (event_type) {
			case FTS_STRING_EVENT_AOD_TRIGGER:
				input_report_key(info->input_dev, KEY_BLACK_UI_GESTURE, 1);
				info->scrub_id = SPECIAL_EVENT_TYPE_AOD_DOUBLETAB;
				info->scrub_x = (data[EventNum * FTS_EVENT_SIZE + 4] & 0xF0) << 4 |
						(data[EventNum * FTS_EVENT_SIZE + 3] & 0xFF);
				info->scrub_y = (data[EventNum * FTS_EVENT_SIZE + 4] & 0x0F) << 8 |
						(data[EventNum * FTS_EVENT_SIZE + 2] & 0xFF);

#ifdef CONFIG_SAMSUNG_PRODUCT_SHIP
				input_info(true, &info->client->dev, "%s: AOD[%d]\n", __func__, info->scrub_id);
#else
				input_info(true, &info->client->dev, "%s: AOD[%d %d %d]\n", __func__,
						info->scrub_id, info->scrub_x, info->scrub_y);
#endif
				info->all_aod_tap_count++;
				break;
			case FTS_STRING_EVENT_SPAY:
				input_report_key(info->input_dev, KEY_BLACK_UI_GESTURE, 1);
				info->scrub_id = SPECIAL_EVENT_TYPE_SPAY;

				input_info(true, &info->client->dev, "%s: SPAY[%d]\n", __func__, info->scrub_id);
				info->all_spay_count++;
				break;
			case FTS_STRING_EVENT_PRESSURE_TOUCHED:
			case FTS_STRING_EVENT_PRESSURE_RELEASED:
				if (info->fts_power_state == FTS_POWER_STATE_ACTIVE) {
					if (event_type & FTS_STRING_EVENT_PRESSURE_TOUCHED) {
						info->all_force_count++;
						info->scrub_id = SPECIAL_EVENT_TYPE_PRESSURE_TOUCHED;
					} else {
						if (info->scrub_id == SPECIAL_EVENT_TYPE_AOD_HOMEKEY_PRESS) {
							input_report_key(info->input_dev, KEY_HOMEPAGE, 0);
							info->scrub_id = SPECIAL_EVENT_TYPE_AOD_HOMEKEY_RELEASE;
						} else {
							info->scrub_id = SPECIAL_EVENT_TYPE_PRESSURE_RELEASED;
						}
					}

					if (info->pressure_setting_mode)
						input_info(true, &info->client->dev, "%s: skip force events in pressure setting mode\n", __func__);
					else
						input_report_key(info->input_dev, KEY_BLACK_UI_GESTURE, 1);
				} else {
					if (event_type & FTS_STRING_EVENT_PRESSURE_RELEASED) {
						input_report_key(info->input_dev, KEY_HOMEPAGE, 0);
						input_report_key(info->input_dev, KEY_BLACK_UI_GESTURE, 1);
						info->scrub_id = SPECIAL_EVENT_TYPE_AOD_HOMEKEY_RELEASE_NO_HAPTIC;
						input_sync(info->input_dev);

						haptic_homekey_release();
					} else {
						input_report_key(info->input_dev, KEY_HOMEPAGE, 1);
						input_sync(info->input_dev);

						info->scrub_id = SPECIAL_EVENT_TYPE_AOD_HOMEKEY_PRESS;
						haptic_homekey_press();
						info->all_force_count++;
					}
				}

				info->scrub_x = (data[EventNum * FTS_EVENT_SIZE + 4] & 0xF0) << 4 |
						(data[EventNum * FTS_EVENT_SIZE + 3] & 0xFF);
				info->scrub_y = (data[EventNum * FTS_EVENT_SIZE + 4] & 0x0F) << 8 |
						(data[EventNum * FTS_EVENT_SIZE + 2] & 0xFF);

#ifdef CONFIG_SAMSUNG_PRODUCT_SHIP
				input_info(true, &info->client->dev, "%s: PRESSURE[%d]\n", __func__, info->scrub_id);
#else
				input_info(true, &info->client->dev, "%s: PRESSURE[%d %d %d]\n", __func__,
						info->scrub_id, info->scrub_x, info->scrub_y);
#endif
				break;
			default:
				input_info(true, &info->client->dev, "%s: [No event] %2x %2x %2x %2x %2x %2x %2x %2x\n",
				__func__, data[EventNum * FTS_EVENT_SIZE], data[EventNum * FTS_EVENT_SIZE + 1],
				data[EventNum * FTS_EVENT_SIZE + 2], data[EventNum * FTS_EVENT_SIZE + 3],
				data[EventNum * FTS_EVENT_SIZE + 4], data[EventNum * FTS_EVENT_SIZE + 5],
				data[EventNum * FTS_EVENT_SIZE + 6], data[EventNum * FTS_EVENT_SIZE + 7]);
				break;
			}

			input_sync(info->input_dev);
			input_report_key(info->input_dev, KEY_BLACK_UI_GESTURE, 0);

			break;
#endif

		default:
			fts_debug_msg_event_handler(info, &data[EventNum * FTS_EVENT_SIZE]);
			continue;
		}

#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
		if (EventID == EVENTID_ENTER_POINTER)
			input_info(true, &info->client->dev,
				"%s[P] tID:%d x:%d y:%d w:%d h:%d z:%d p:%d tc:%d tm:%d\n",
				info->dex_name,
				TouchID, x, y, bw, bh, z, palm, info->touch_count, info->touch_mode);

#ifdef FTS_SUPPORT_HOVER
		else if (EventID == EVENTID_HOVER_ENTER_POINTER)
			input_dbg(true, &info->client->dev,
				"[HP] tID:%d x:%d y:%d z:%d\n",
				TouchID, x, y, z);
#endif
#else
		if (EventID == EVENTID_ENTER_POINTER)
			input_info(true, &info->client->dev,
				"%s[P] tID:%d tc:%d tm:%d\n",
				info->dex_name,
				TouchID, info->touch_count, info->touch_mode);
#ifdef FTS_SUPPORT_HOVER
		else if (EventID == EVENTID_HOVER_ENTER_POINTER)
			input_dbg(true, &info->client->dev,
				"[HP] tID:%d\n", TouchID);
#endif
#endif
		else if (EventID == EVENTID_LEAVE_POINTER) {
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
			input_info(true, &info->client->dev,
				"%s[R] tID:%d mc:%d tc:%d lx:%d ly:%d f:%d lp:(%x/%d) Ver[%02X%04X|%01X] P%02XT%04X[%02X] F%02X%02X C%02X\n",
				info->dex_name, TouchID, info->finger[TouchID].mcount,
				info->touch_count, info->finger[TouchID].lx,
				info->finger[TouchID].ly, info->pressure_max,
				info->lowpower_flag, info->pressure_caller_id,
				info->panel_revision, info->fw_main_version_of_ic,
				info->flip_enable, info->cal_count, info->tune_fix_ver,
				info->test_result.data[0], info->pressure_cal_base,
				info->pressure_cal_delta, info->nv_crc_fail_count);
#else
			input_info(true, &info->client->dev,
				"%s[R] tID:%d mc:%d tc:%d f:%d lp:(%x/%d) Ver[%02X%04X|%01X] P%02XT%04X[%02X] F%02X%02X, C%02X\n",
				info->dex_name, TouchID, info->finger[TouchID].mcount,
				info->touch_count, info->pressure_max,
				info->lowpower_flag, info->pressure_caller_id,
				info->panel_revision, info->fw_main_version_of_ic,
				info->flip_enable, info->cal_count, info->tune_fix_ver,
				info->test_result.data[0], info->pressure_cal_base,
				info->pressure_cal_delta, info->nv_crc_fail_count);
#endif
			info->finger[TouchID].mcount = 0;
		}
#ifdef FTS_SUPPORT_HOVER
		else if (EventID == EVENTID_HOVER_LEAVE_POINTER) {
			input_dbg(true, &info->client->dev,
				"[HR] tID:%d Ver[%02X%04X|%01X]\n",
				TouchID,
				info->panel_revision, info->fw_main_version_of_ic,
				info->flip_enable);
			info->finger[TouchID].mcount = 0;
		}
#endif
		else if (EventID == EVENTID_MOTION_POINTER && info->fts_power_state == FTS_POWER_STATE_ACTIVE)
			info->finger[TouchID].mcount++;

		if (((EventID == EVENTID_ENTER_POINTER) ||
			(EventID == EVENTID_MOTION_POINTER) ||
			(EventID == EVENTID_LEAVE_POINTER)) && info->fts_power_state == FTS_POWER_STATE_ACTIVE)
			info->finger[TouchID].state = EventID;
	}

	input_sync(info->input_dev);

	return LastLeftEvent;
}

#ifdef FTS_SUPPORT_TA_MODE
static void fts_ta_cb(struct fts_callbacks *cb, int ta_status)
{
	struct fts_ts_info *info =
	    container_of(cb, struct fts_ts_info, callbacks);

	if (ta_status == 0x01 || ta_status == 0x03) {
		fts_command(info, FTS_CMD_CHARGER_PLUGGED);
		info->TA_Pluged = true;
		input_info(true, &info->client->dev,
			 "%s: device_control : CHARGER CONNECTED, ta_status : %x\n",
			 __func__, ta_status);
	} else {
		fts_command(info, FTS_CMD_CHARGER_UNPLUGGED);
		info->TA_Pluged = false;
		input_info(true, &info->client->dev,
			 "%s: device_control : CHARGER DISCONNECTED, ta_status : %x\n",
			 __func__, ta_status);
	}
}
#endif

/**
 * fts_interrupt_handler()
 *
 * Called by the kernel when an interrupt occurs (when the sensor
 * asserts the attention irq).
 *
 * This function is the ISR thread and handles the acquisition
 * and the reporting of finger data when the presence of fingers
 * is detected.
 */
static irqreturn_t fts_interrupt_handler(int irq, void *handle)
{
	struct fts_ts_info *info = handle;
	unsigned char regAdd[4] = {0xB6, 0x00, 0x23, READ_ALL_EVENT};
	unsigned short evtcount = 0;

#if defined(CONFIG_SECURE_TOUCH)
	if (fts_filter_interrupt(info) == IRQ_HANDLED) {
		int ret;

		ret = wait_for_completion_interruptible_timeout((&info->st_interrupt),
					msecs_to_jiffies(10 * MSEC_PER_SEC));
		return IRQ_HANDLED;
	}
#endif

	/* in LPM, waiting blsp block resume */
	if (info->fts_power_state == FTS_POWER_STATE_LOWPOWER) {
		int ret;

		input_dbg(true, &info->client->dev, "%s: run LPM interrupt handler\n", __func__);

		wake_lock_timeout(&info->wakelock, msecs_to_jiffies(3 * MSEC_PER_SEC));
		/* waiting for blsp block resuming, if not occurs i2c error */
		ret = wait_for_completion_interruptible_timeout(&info->resume_done, msecs_to_jiffies(3 * MSEC_PER_SEC));
		if (ret == 0) {
			input_err(true, &info->client->dev, "%s: LPM: pm resume is not handled\n", __func__);
			return IRQ_NONE;
		}

		if (ret < 0) {
			input_err(true, &info->client->dev, "%s: LPM: -ERESTARTSYS if interrupted, %d\n", __func__, ret);
			return IRQ_NONE;
		}

		input_info(true, &info->client->dev, "%s: run LPM interrupt handler, %d\n", __func__, ret);
		/* run lpm interrupt handler */
	}

	evtcount = 0;
	fts_read_reg(info, &regAdd[0], 3, (unsigned char *)&evtcount, 2);

	evtcount = evtcount >> 8;
	evtcount = evtcount / 2;

	if (evtcount > FTS_FIFO_MAX)
		evtcount = FTS_FIFO_MAX;

	if (evtcount > 0) {
		memset(info->data, 0x0, FTS_EVENT_SIZE * evtcount);
		fts_read_reg(info, &regAdd[3], 1, (unsigned char *)info->data,
				  FTS_EVENT_SIZE * evtcount);
		fts_event_handler_type_b(info, info->data, evtcount);
	}

	return IRQ_HANDLED;
}

int fts_irq_enable(struct fts_ts_info *info,
		bool enable)
{
	int retval = 0;

	if (enable) {
		if (info->irq_enabled)
			return retval;

		retval = request_threaded_irq(info->irq, NULL,
				fts_interrupt_handler, info->board->irq_type,
				FTS_TS_DRV_NAME, info);
		if (retval < 0) {
			input_err(true, &info->client->dev,
					"%s: Failed to create irq thread %d\n",
					__func__, retval);
			return retval;
		}

		info->irq_enabled = true;
	} else {
		if (info->irq_enabled) {
			disable_irq(info->irq);
			free_irq(info->irq, info);
			info->irq_enabled = false;
		}
	}

	return retval;
}

#ifdef CONFIG_OF
#ifdef FTS_SUPPORT_TA_MODE
struct fts_callbacks *fts_charger_callbacks;
void tsp_charger_infom(bool en)
{
	pr_err("%s: %s %s: ta:%d\n", FTS_TS_DRV_NAME, SECLOG, __func__, en);

	if (fts_charger_callbacks && fts_charger_callbacks->inform_charger)
		fts_charger_callbacks->inform_charger(fts_charger_callbacks, en);
}
static void fts_tsp_register_callback(void *cb)
{
	fts_charger_callbacks = cb;
}
#endif

#ifdef FTS_SUPPORT_TOUCH_KEY
static int fts_led_power_ctrl(void *data, bool on)
{
	struct fts_ts_info *info = (struct fts_ts_info *)data;
	const struct fts_i2c_platform_data *pdata = info->board;
	struct device *dev = &info->client->dev;
	struct regulator *regulator_tk_led = NULL;
	int retval = 0;

	if (info->tsk_led_enabled == on)
		return retval;

	regulator_tk_led = regulator_get(NULL, pdata->regulator_tk_led);
	if (IS_ERR_OR_NULL(regulator_tk_led)) {
		input_err(true, dev, "%s: Failed to get %s regulator.\n",
			 __func__, pdata->regulator_tk_led);
		goto out;
	}

	input_info(true, dev, "%s: %s\n", __func__, on ? "on" : "off");

	if (on) {
		retval = regulator_enable(regulator_tk_led);
		if (retval) {
			input_err(true, dev, "%s: Failed to enable led%d\n", __func__, retval);
			goto out;
		}
	} else {
		if (regulator_is_enabled(regulator_tk_led))
			regulator_disable(regulator_tk_led);
	}

	info->tsk_led_enabled = on;
out:
	regulator_put(regulator_tk_led);

	return retval;
}
#endif

static int fts_power_ctrl(void *data, bool on)
{
	struct fts_ts_info *info = (struct fts_ts_info *)data;
	const struct fts_i2c_platform_data *pdata = info->board;
	struct device *dev = &info->client->dev;
	struct regulator *regulator_dvdd = NULL;
	struct regulator *regulator_avdd = NULL;
	static bool enabled;
	int retval = 0;

	if (enabled == on)
		return retval;

	regulator_dvdd = regulator_get(NULL, pdata->regulator_dvdd);
	if (IS_ERR_OR_NULL(regulator_dvdd)) {
		input_err(true, dev, "%s: Failed to get %s regulator\n",
			 __func__, pdata->regulator_dvdd);
		goto out;
	}

	regulator_avdd = regulator_get(NULL, pdata->regulator_avdd);
	if (IS_ERR_OR_NULL(regulator_avdd)) {
		input_err(true, dev, "%s: Failed to get %s regulator\n",
			 __func__, pdata->regulator_avdd);
		goto out;
	}

	if (on) {
		retval = regulator_enable(regulator_avdd);
		if (retval) {
			input_err(true, dev, "%s: Failed to enable avdd: %d\n", __func__, retval);
			regulator_disable(regulator_avdd);
			goto out;
		}

		fts_delay(1);

		retval = regulator_enable(regulator_dvdd);
		if (retval) {
			input_err(true, dev, "%s: Failed to enable vdd: %d\n", __func__, retval);
			regulator_disable(regulator_dvdd);
			regulator_disable(regulator_avdd);
			goto out;
		}

		retval = pinctrl_select_state(pdata->pinctrl, pdata->pins_default);
		if (retval < 0)
			input_err(true, dev, "%s: Failed to configure tsp_attn pin\n", __func__);

		fts_delay(5);
	} else {
		regulator_disable(regulator_dvdd);
		regulator_disable(regulator_avdd);

		retval = pinctrl_select_state(pdata->pinctrl, pdata->pins_sleep);
		if (retval < 0)
			input_err(true, dev, "%s: Failed to configure tsp_attn pin\n", __func__);
	}

	enabled = on;

	input_err(true, dev, "%s: %s: avdd:%s, dvdd:%s\n", __func__, on ? "on" : "off",
		regulator_is_enabled(regulator_avdd) ? "on" : "off",
		regulator_is_enabled(regulator_dvdd) ? "on" : "off");

out:
	regulator_put(regulator_dvdd);
	regulator_put(regulator_avdd);

	return retval;
}

static int fts_parse_dt(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	struct fts_i2c_platform_data *pdata = dev->platform_data;
	struct device_node *np = dev->of_node;
	u32 coords[2];
	u32 ic_match_value;
	int retval = 0;
	int lcdtype = 0;
#if defined(CONFIG_EXYNOS_DECON_FB)
	int connected;
#endif

	pdata->tsp_icid = of_get_named_gpio(np, "stm,tsp-icid_gpio", 0);
	if (gpio_is_valid(pdata->tsp_icid)) {
		input_info(true, dev, "%s: TSP_ICID : %d\n", __func__, gpio_get_value(pdata->tsp_icid));
		if (of_property_read_u32(np, "stm,icid_match_value", &ic_match_value)) {
			input_err(true, dev, "%s: Failed to get icid match value\n", __func__);
			return -EINVAL;
		} else {
			input_err(true, dev, "%s: IC matched value : %d\n", __func__, ic_match_value);
		}

		if (gpio_get_value(pdata->tsp_icid) != ic_match_value) {
			input_err(true, dev, "%s: Do not match TSP_ICID\n", __func__);
			return -EINVAL;
		}
	} else {
		input_err(true, dev, "%s: Failed to get tsp-icid gpio\n", __func__);
	}

	if (gpio_is_valid(pdata->tsp_icid)) {
		retval = gpio_request(pdata->tsp_icid, "TSP_ICID");
		if (retval)
			input_err(true, dev, "%s: Unable to request tsp_icid [%d]\n", __func__, pdata->tsp_icid);
	}

	pdata->tsp_id = of_get_named_gpio(np, "stm,tsp-id_gpio", 0);
	if (gpio_is_valid(pdata->tsp_id))
		input_info(true, dev, "%s: TSP_ID : %d\n", __func__, gpio_get_value(pdata->tsp_id));
	else
		input_err(true, dev, "%s: Failed to get tsp-id gpio\n", __func__);

	if (gpio_is_valid(pdata->tsp_id)) {
		retval = gpio_request(pdata->tsp_id, "TSP_ID");
		if (retval)
			input_err(true, dev, "%s: Unable to request tsp_id [%d]\n", __func__, pdata->tsp_id);
	}

	pdata->device_id = of_get_named_gpio(np, "stm,device_gpio", 0);
	if (gpio_is_valid(pdata->device_id))
		input_info(true, dev, "%s: Device ID : %d\n", __func__, gpio_get_value(pdata->device_id));
	else
		input_err(true, dev, "%s: skipped to get device-id gpio\n", __func__);

	pdata->irq_gpio = of_get_named_gpio(np, "stm,irq_gpio", 0);
	if (gpio_is_valid(pdata->irq_gpio)) {
		retval = gpio_request_one(pdata->irq_gpio, GPIOF_DIR_IN, "stm,tsp_int");
		if (retval) {
			input_err(true, dev, "%s: Unable to request tsp_int [%d]\n", __func__, pdata->irq_gpio);
			return -EINVAL;
		}
	} else {
		input_err(true, dev, "%s: Failed to get irq gpio\n", __func__);
		return -EINVAL;
	}
	client->irq = gpio_to_irq(pdata->irq_gpio);

	if (of_property_read_u32(np, "stm,irq_type", &pdata->irq_type)) {
		input_err(true, dev, "%s: Failed to get irq_type property\n", __func__);
		return -EINVAL;
	}

	if (of_property_read_u32_array(np, "stm,max_coords", coords, 2)) {
		input_err(true, dev, "%s: Failed to get max_coords property\n", __func__);
		return -EINVAL;
	}
	pdata->max_x = coords[0];
	pdata->max_y = coords[1];

	if (of_property_read_string(np, "stm,regulator_dvdd", &pdata->regulator_dvdd)) {
		input_err(true, dev, "%s: Failed to get regulator_dvdd name property\n", __func__);
		return -EINVAL;
	}

	if (of_property_read_string(np, "stm,regulator_avdd", &pdata->regulator_avdd)) {
		input_err(true, dev, "%s: Failed to get regulator_avdd name property\n", __func__);
		return -EINVAL;
	}
	pdata->power = fts_power_ctrl;

	/* Optional parmeters(those values are not mandatory)
	 * do not return error value even if fail to get the value
	 */
	if (gpio_is_valid(pdata->tsp_id))
		of_property_read_string_index(np, "stm,firmware_name", gpio_get_value(pdata->tsp_id), &pdata->firmware_name);
	else
		of_property_read_string_index(np, "stm,firmware_name", 0, &pdata->firmware_name);

	if (of_property_read_string_index(np, "stm,project_name", 0, &pdata->project_name))
		input_err(true, dev, "%s: skipped to get project_name property\n", __func__);
	if (of_property_read_string_index(np, "stm,project_name", 1, &pdata->model_name))
		input_err(true, dev, "%s: skipped to get model_name property\n", __func__);

	if (of_property_read_bool(np, "stm,support_gesture"))
		pdata->support_sidegesture = true;

	pdata->support_dex = of_property_read_bool(np, "support_dex_mode");

	of_property_read_u32(np, "stm,bringup", &pdata->bringup);

	if (of_property_read_u32(np, "stm,always_lpmode", &pdata->always_lpmode) < 0)
		pdata->always_lpmode = 0;

	if (of_property_read_string(np, "pressure-sensor", &pdata->support_pressure) < 0)
		input_err(true, dev, "%s: Failed to get pressure-sensor property\n", __func__);

	pdata->support_hover = false;
	pdata->support_glove = false;
#ifdef CONFIG_SEC_FACTORY
	pdata->support_mt_pressure = true;
#endif
#ifdef FTS_SUPPORT_TA_MODE
	pdata->register_cb = fts_tsp_register_callback;
#endif

#ifdef FTS_SUPPORT_TOUCH_KEY
	if (of_property_read_u32(np, "stm,num_touchkey", &pdata->num_touchkey))
		input_err(true, dev, "%s: skipped to get num_touchkey property\n", __func__);
	else {
		pdata->support_mskey = true;
		pdata->touchkey = fts_touchkeys;

		if (of_property_read_string(np, "stm,regulator_tk_led", &pdata->regulator_tk_led))
			input_err(true, dev, "%s: skipped to get regulator_tk_led name property\n", __func__);
		else
			pdata->led_power = fts_led_power_ctrl;
	}
#endif

	if (of_property_read_u32(np, "stm,device_num", &pdata->device_num))
		input_err(true, dev, "%s: Failed to get device_num property\n", __func__);

#ifdef PAT_CONTROL
	if (of_property_read_u32(np, "stm,pat_function", &pdata->pat_function) < 0){
		pdata->pat_function = PAT_CONTROL_NONE;
		input_err(true, dev, "%s: Failed to get pat_function property\n", __func__);
	}

	if (of_property_read_u32(np, "stm,afe_base", &pdata->afe_base) < 0){
		pdata->afe_base = 0;
		input_err(true, dev, "%s: Failed to get afe_base property\n", __func__);
	}
#endif

#if defined(CONFIG_FB_MSM_MDSS_SAMSUNG)
	lcdtype = get_lcd_attached("GET");
	if (lcdtype == 0xFFFFFF) {
		input_err(true, &client->dev, "%s: lcd is not attached\n", __func__);
		return -ENODEV;
	}
#endif

#if defined(CONFIG_EXYNOS_DECON_FB)
	connected = get_lcd_info("connected");
	if (connected < 0) {
		input_err(true, dev, "%s: Failed to get lcd info\n", __func__);
		return -EINVAL;
	}

	if (!connected) {
		input_err(true, &client->dev, "%s: lcd is disconnected\n", __func__);
		return -ENODEV;
	}

	input_info(true, &client->dev, "%s: lcd is connected\n", __func__);

	lcdtype = get_lcd_info("id");
	if (lcdtype < 0) {
		input_err(true, dev, "%s: Failed to get lcd info\n", __func__);
		return -EINVAL;
	}
#endif
	input_info(true, &client->dev, "%s: lcdtype 0x%08X\n", __func__, lcdtype);

	pdata->panel_revision = ((lcdtype >> 8) & 0xFF) >> 4;

	input_err(true, dev,
		"%s: irq :%d, irq_type: 0x%04x, max[x,y]: [%d,%d], project/model_name: %s/%s, pat_function(%d), panel_revision: %d, gesture: %d, device_num: %d, dex: %d pressure:%s\n",
		__func__, pdata->irq_gpio, pdata->irq_type, pdata->max_x, pdata->max_y,
		pdata->project_name, pdata->model_name, pdata->pat_function, pdata->panel_revision,
		pdata->support_sidegesture, pdata->device_num, pdata->support_dex, pdata->support_pressure);

	return retval;
}
#endif

#if defined(CONFIG_TOUCHSCREEN_DUMP_MODE)
#include <linux/sec_debug.h>
extern struct tsp_dump_callbacks dump_callbacks;
void tsp_dump(void);
#endif

static int fts_setup_drv_data(struct i2c_client *client)
{
	int retval = 0;
	struct fts_i2c_platform_data *pdata;
	struct fts_ts_info *info;

	/* parse dt */
	if (client->dev.of_node) {
		pdata = devm_kzalloc(&client->dev,
			sizeof(struct fts_i2c_platform_data), GFP_KERNEL);

		if (!pdata) {
			input_err(true, &client->dev, "%s: Failed to allocate platform data\n", __func__);
			return -ENOMEM;
		}

		client->dev.platform_data = pdata;
		retval = fts_parse_dt(client);
		if (retval) {
			input_err(true, &client->dev, "%s: Failed to parse dt\n", __func__);
			return retval;
		}
	} else {
		pdata = client->dev.platform_data;
	}

	if (!pdata) {
		input_err(true, &client->dev, "%s: No platform data found\n", __func__);
			return -EINVAL;
	}
	if (!pdata->power) {
		input_err(true, &client->dev, "%s: No power contorl found\n", __func__);
			return -EINVAL;
	}

	pdata->pinctrl = devm_pinctrl_get(&client->dev);
	if (IS_ERR(pdata->pinctrl)) {
		input_err(true, &client->dev, "%s: could not get pinctrl\n", __func__);
		return PTR_ERR(pdata->pinctrl);
	}

	pdata->pins_default = pinctrl_lookup_state(pdata->pinctrl, "on_state");
	if (IS_ERR(pdata->pins_default))
		input_err(true, &client->dev, "%s: could not get default pinstate\n", __func__);

	pdata->pins_sleep = pinctrl_lookup_state(pdata->pinctrl, "off_state");
	if (IS_ERR(pdata->pins_sleep))
		input_err(true, &client->dev, "%s: could not get sleep pinstate\n", __func__);

	info = kzalloc(sizeof(struct fts_ts_info), GFP_KERNEL);
	if (!info) {
		input_err(true, &client->dev,
				"%s: Failed to alloc mem for info\n",
				__func__);
		return -ENOMEM;
	}

	info->client = client;
	info->board = pdata;
	info->irq = client->irq;
	info->irq_type = info->board->irq_type;
	info->irq_enabled = false;
	info->touch_stopped = false;
	info->panel_revision = info->board->panel_revision;
	info->stop_device = fts_stop_device;
	info->start_device = fts_start_device;
	info->fts_command = fts_command;
	info->fts_enable_feature = fts_enable_feature;
	info->fts_read_reg = fts_read_reg;
	info->fts_write_reg = fts_write_reg;
	info->fts_systemreset = fts_systemreset;
	info->fts_get_version_info = fts_get_version_info;
	info->fts_get_sysinfo_data = fts_get_sysinfo_data;
	info->fts_wait_for_ready = fts_wait_for_ready;
#ifdef FTS_SUPPORT_STRINGLIB
	info->fts_read_from_string = fts_read_from_string;
	info->fts_write_to_string = fts_write_to_string;
#endif
#ifdef FTS_SUPPORT_NOISE_PARAM
	info->fts_get_noise_param_address = fts_get_noise_param_address;
#endif

#ifdef USE_OPEN_DWORK
	INIT_DELAYED_WORK(&info->open_work, fts_open_work);
#endif
	info->delay_time = 300;
	INIT_DELAYED_WORK(&info->reset_work, fts_reset_work);
	INIT_DELAYED_WORK(&info->work_read_info, fts_read_info_work);

	if (info->board->support_hover)
		input_info(true, &info->client->dev, "%s: Support Hover Event\n", __func__);
	else
		input_info(true, &info->client->dev, "%s: Not support Hover Event\n", __func__);

	i2c_set_clientdata(client, info);

	if (pdata->get_ddi_type) {
		info->ddi_type = pdata->get_ddi_type();
		input_info(true, &client->dev, "%s: DDI Type is %s[%d]\n",
			__func__, info->ddi_type ? "MAGNA" : "SDC", info->ddi_type);
	}

	return retval;
}

static void fts_set_input_prop(struct fts_ts_info *info, struct input_dev *dev, u8 propbit)
{
	static char fts_ts_phys[64] = { 0 };

	dev->dev.parent = &info->client->dev;

	snprintf(fts_ts_phys, sizeof(fts_ts_phys), "%s/input1",
		 dev->name);
	dev->phys = fts_ts_phys;
	dev->id.bustype = BUS_I2C;

#ifdef CONFIG_GLOVE_TOUCH
	input_set_capability(dev, EV_SW, SW_GLOVE);
#endif
	set_bit(EV_SYN, dev->evbit);
	set_bit(EV_KEY, dev->evbit);
	set_bit(EV_ABS, dev->evbit);
	set_bit(propbit, dev->propbit);
	set_bit(BTN_TOUCH, dev->keybit);
	set_bit(BTN_TOOL_FINGER, dev->keybit);
	set_bit(KEY_BLACK_UI_GESTURE, dev->keybit);
	set_bit(KEY_HOMEPAGE, dev->keybit);

#ifdef FTS_SUPPORT_TOUCH_KEY
	if (info->board->support_mskey) {
		int i;
		for (i = 0 ; i < info->board->num_touchkey ; i++)
			set_bit(info->board->touchkey[i].keycode, dev->keybit);

		set_bit(EV_LED, dev->evbit);
		set_bit(LED_MISC, dev->ledbit);
	}
#endif
	if (info->board->support_sidegesture) {
		set_bit(KEY_SIDE_GESTURE, dev->keybit);
		set_bit(KEY_SIDE_GESTURE_RIGHT, dev->keybit);
		set_bit(KEY_SIDE_GESTURE_LEFT, dev->keybit);
	}

	input_set_abs_params(dev, ABS_MT_POSITION_X,
				0, info->board->max_x, 0, 0);
	input_set_abs_params(dev, ABS_MT_POSITION_Y,
				0, info->board->max_y, 0, 0);
#ifdef CONFIG_SEC_FACTORY
	input_set_abs_params(dev, ABS_MT_PRESSURE, 0, 255, 0, 0);
#else
	if (info->board->support_mt_pressure)
		input_set_abs_params(dev, ABS_MT_PRESSURE, 0, 10000, 0, 0);
#endif

	input_set_abs_params(dev, ABS_MT_TOUCH_MAJOR,
				 0, 255, 0, 0);
	input_set_abs_params(dev, ABS_MT_TOUCH_MINOR,
				 0, 255, 0, 0);
	input_set_abs_params(dev, ABS_MT_CUSTOM, 0, 0xFFFFFFFF, 0, 0);

	if (info->board->support_hover)
		input_set_abs_params(dev, ABS_MT_DISTANCE, 0, 255, 0, 0);

	if (propbit == INPUT_PROP_POINTER)
		input_mt_init_slots(dev, FINGER_MAX, INPUT_MT_POINTER);
	else
		input_mt_init_slots(dev, FINGER_MAX, INPUT_MT_DIRECT);

	input_set_drvdata(dev, info);
}

static int fts_probe(struct i2c_client *client, const struct i2c_device_id *idp)
{
	int retval;
	struct fts_ts_info *info = NULL;
	int i = 0;

	input_info(true, &client->dev, "%s: FTS Driver [70%s]\n", __func__,
	       FTS_TS_DRV_VERSION);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		input_err(true, &client->dev, "%s: FTS err = EIO!\n", __func__);
		return -EIO;
	}
#ifdef CONFIG_BATTERY_SAMSUNG
	if (lpcharge == 1) {
		input_err(true, &client->dev, "%s: Do not load driver due to : lpm %d\n",
			 __func__, lpcharge);
		return -ENODEV;
	}
#endif
	/* Build up driver data */
	retval = fts_setup_drv_data(client);
	if (retval < 0) {
		input_err(true, &client->dev, "%s: Failed to set up driver data\n", __func__);
		goto err_setup_drv_data;
	}

	info = (struct fts_ts_info *)i2c_get_clientdata(client);
	if (!info) {
		input_err(true, &client->dev, "%s: Failed to get driver data\n", __func__);
		retval = -ENODEV;
		goto err_get_drv_data;
	}
	i2c_set_clientdata(client, info);

	info->probe_done = false;

	if (info->board->power)
		info->board->power(info, true);
	info->fts_power_state = FTS_POWER_STATE_ACTIVE;

	info->dev = &info->client->dev;

	info->input_dev = input_allocate_device();
	if (!info->input_dev) {
		input_err(true, &info->client->dev, "%s: Failed to alloc input_dev\n", __func__);
		retval = -ENOMEM;
		goto err_input_allocate_device;
	}

	if (info->board->support_dex) {
		info->input_dev_pad = input_allocate_device();
		if (!info->input_dev_pad) {
			input_err(true, &info->client->dev, "%s: Failed to alloc input_dev\n", __func__);
			retval = -ENOMEM;
			goto err_input_pad_allocate_device;
		}
	}

	mutex_init(&info->device_mutex);
	mutex_init(&info->i2c_mutex);

	retval = fts_init(info);
	if (retval) {
		input_err(true, &info->client->dev, "%s: fts_init fail!\n", __func__);
		goto err_fts_init;
	}
	mutex_lock(&info->device_mutex);
	info->reinit_done = true;
	mutex_unlock(&info->device_mutex);

	info->max_z_value = 0;
	info->min_z_value = 0xFFFFFFFF;
	info->sum_z_value = 0;

	wake_lock_init(&info->wakelock, WAKE_LOCK_SUSPEND, "tsp_wakelock");
	init_completion(&info->resume_done);
	complete_all(&info->resume_done);

	if (info->board->support_dex) {
		info->input_dev_pad->name = "sec_touchpad";
		fts_set_input_prop(info, info->input_dev_pad, INPUT_PROP_POINTER);
	}
	info->dex_name = "";

	if (info->board->device_num == 0)
		info->input_dev->name = "sec_touchscreen";
	else if (info->board->device_num == 2)
		info->input_dev->name = "sec_touchscreen2";
	else
		info->input_dev->name = "sec_touchscreen";
	fts_set_input_prop(info, info->input_dev, INPUT_PROP_DIRECT);
#ifdef USE_OPEN_CLOSE
	info->input_dev->open = fts_input_open;
	info->input_dev->close = fts_input_close;
#endif
	info->input_dev_touch = info->input_dev;

	retval = input_register_device(info->input_dev);
	if (retval) {
		input_err(true, &info->client->dev, "%s: input_register_device fail!\n", __func__);
		goto err_register_input;
	}
	if (info->board->support_dex) {
		retval = input_register_device(info->input_dev_pad);
		if (retval) {
			input_err(true, &info->client->dev, "%s: input_register_device fail!\n", __func__);
			goto err_register_input_pad;
		}
	}

	for (i = 0; i < FINGER_MAX; i++) {
		info->finger[i].state = EVENTID_LEAVE_POINTER;
		info->finger[i].mcount = 0;
	}

	retval = fts_irq_enable(info, true);
	if (retval < 0) {
		input_err(true, &info->client->dev,
						"%s: Failed to enable attention interrupt\n",
						__func__);
		goto err_enable_irq;
	}

#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
	tui_tsp_info = info;
	trustedui_set_tsp_irq(info->irq);
	input_info(true, &client->dev, "%s[%d] called!\n", __func__, info->irq);
#endif

#ifdef FTS_SUPPORT_TA_MODE
	info->register_cb = info->board->register_cb;

	info->callbacks.inform_charger = fts_ta_cb;
	if (info->register_cb)
		info->register_cb(&info->callbacks);
#endif

#ifdef SEC_TSP_FACTORY_TEST
	retval = sec_cmd_init(&info->sec, ft_commands,
			ARRAY_SIZE(ft_commands), SEC_CLASS_DEVT_TSP);
	if (retval < 0) {
		input_err(true, &info->client->dev,
			"%s: Failed to sec_cmd_init\n", __func__);
		retval = -ENODEV;
		goto err_sec_cmd;
	}

	retval = sysfs_create_group(&info->sec.fac_dev->kobj,
				 &sec_touch_factory_attr_group);
	if (retval < 0) {
		input_err(true, &info->client->dev, "%s: Failed to create sysfs group\n", __func__);
		goto err_sysfs;
	}

	retval = sysfs_create_link(&info->sec.fac_dev->kobj,
				&info->input_dev->dev.kobj, "input");

	if (retval < 0) {
		input_err(true, &info->client->dev,
				"%s: Failed to create link\n", __func__);
		goto err_sysfs;
	}

#ifdef FTS_SUPPORT_TOUCH_KEY
	if (info->board->support_mskey) {
#ifdef CONFIG_SEC_SYSFS
		info->fac_dev_tk = sec_device_create(info, "sec_touchkey");
#else
		info->fac_dev_tk = device_create(sec_class, NULL, 11, info, "sec_touchkey");
#endif
		if (IS_ERR(info->fac_dev_tk))
			input_err(true, &info->client->dev, "%s: Failed to create device for the touchkey sysfs\n", __func__);
		else {
			dev_set_drvdata(info->fac_dev_tk, info);

			retval = sysfs_create_group(&info->fac_dev_tk->kobj,
						 &sec_touchkey_factory_attr_group);
			if (retval < 0)
				input_err(true, &info->client->dev, "%s: Failed to create sysfs group\n", __func__);
			else {
				retval = sysfs_create_link(&info->fac_dev_tk->kobj,
							&info->input_dev->dev.kobj, "input");

				if (retval < 0)
					input_err(true, &info->client->dev,
							"%s: Failed to create link\n", __func__);
			}
		}
	}
#endif
#endif

#if defined(CONFIG_SECURE_TOUCH)
	for (i = 0; i < (int)ARRAY_SIZE(attrs); i++) {
		retval = sysfs_create_file(&info->input_dev->dev.kobj,
				&attrs[i].attr);
		if (retval < 0) {
			input_err(true, &info->client->dev,
				"%s: Failed to create sysfs attributes\n",
				__func__);
		}
	}

	fts_secure_touch_init(info);
#endif

	device_init_wakeup(&client->dev, true);
	info->probe_done = true;

	fts_check_custom_library(info);

	schedule_delayed_work(&info->work_read_info, msecs_to_jiffies(5 * MSEC_PER_SEC));

#if defined(CONFIG_TOUCHSCREEN_DUMP_MODE)
	dump_callbacks.inform_dump = tsp_dump;
	INIT_DELAYED_WORK(&info->debug_work, dump_tsp_rawdata);
	p_debug_work = &info->debug_work;
#endif

	input_err(true, &info->client->dev, "%s: done\n", __func__);
	input_log_fix();

	return 0;

#ifdef SEC_TSP_FACTORY_TEST
err_sysfs:
	sec_cmd_exit(&info->sec, SEC_CLASS_DEVT_TSP);
err_sec_cmd:
#endif
	if (info->irq_enabled)
		fts_irq_enable(info, false);
#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
	tui_tsp_info = NULL;
#endif	
err_enable_irq:
	if (info->board->support_dex) {
		input_unregister_device(info->input_dev_pad);
		info->input_dev_pad = NULL;
	}
err_register_input_pad:
	input_unregister_device(info->input_dev);
	info->input_dev = NULL;
	info->input_dev_touch = NULL;
err_register_input:
	wake_lock_destroy(&info->wakelock);

#ifdef SEC_TSP_FACTORY_TEST
	if (info->cx_data)
		kfree(info->cx_data);
	kfree(info->pFrame);
#endif
err_fts_init:
	mutex_destroy(&info->device_mutex);
	mutex_destroy(&info->i2c_mutex);
	if (info->board->support_dex) {
		if (info->input_dev_pad)
			input_free_device(info->input_dev_pad);
	}
err_input_pad_allocate_device:
	if (info->input_dev)
		input_free_device(info->input_dev);
err_input_allocate_device:
	if (info->board->power)
		info->board->power(info, false);
	if (gpio_is_valid(info->board->irq_gpio))
		gpio_free(info->board->irq_gpio);
	kfree(info);
err_get_drv_data:
err_setup_drv_data:
	input_err(true, &client->dev, "%s: failed(%d)\n", __func__, retval);
	input_log_fix();
	return retval;
}

static int fts_remove(struct i2c_client *client)
{
	struct fts_ts_info *info = i2c_get_clientdata(client);
#if defined(CONFIG_SECURE_TOUCH)
	int i = 0;
#endif

	input_info(true, &info->client->dev, "%s\n", __func__);
	info->shutdown_is_on_going = true;

	disable_irq_nosync(info->client->irq);
	free_irq(info->client->irq, info);

	cancel_delayed_work_sync(&info->work_read_info);
	flush_delayed_work(&info->work_read_info);


	cancel_delayed_work_sync(&info->reset_work);
	flush_delayed_work(&info->reset_work);

	wake_lock_destroy(&info->wakelock);

#if defined(CONFIG_SECURE_TOUCH)
	for (i = 0; i < (int)ARRAY_SIZE(attrs); i++) {
		sysfs_remove_file(&info->input_dev->dev.kobj,
				&attrs[i].attr);
	}
#endif

#ifdef SEC_TSP_FACTORY_TEST
#ifdef FTS_SUPPORT_TOUCH_KEY
	if (info->board->support_mskey) {
		sysfs_remove_link(&info->fac_dev_tk->kobj, "input");
		sysfs_remove_group(&info->fac_dev_tk->kobj,
				   &sec_touchkey_factory_attr_group);
#ifdef CONFIG_SEC_SYSFS
		sec_device_destroy(info->fac_dev_tk->devt);
#else
		device_destroy(sec_class, 11);
#endif

	}
#endif
	sysfs_remove_link(&info->sec.fac_dev->kobj, "input");
	sysfs_remove_group(&info->sec.fac_dev->kobj,
			   &sec_touch_factory_attr_group);
	sec_cmd_exit(&info->sec, SEC_CLASS_DEVT_TSP);

	if (info->cx_data)
		kfree(info->cx_data);
	kfree(info->pFrame);
#endif

	if (info->board->support_dex) {
		input_mt_destroy_slots(info->input_dev_pad);
		input_unregister_device(info->input_dev_pad);
	}
	info->input_dev_pad = NULL;

	info->input_dev = info->input_dev_touch;
	input_mt_destroy_slots(info->input_dev);
	input_unregister_device(info->input_dev);
	info->input_dev = NULL;
	info->input_dev_touch = NULL;

#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
	tui_tsp_info = NULL;
#endif

	if (info->board->power)
		info->board->power(info, false);
#ifdef FTS_SUPPORT_TOUCH_KEY
	if (info->board->led_power)
		info->board->led_power(info, false);
#endif
	info->shutdown_is_on_going = false;
	kfree(info);

	return 0;
}

#ifdef USE_OPEN_CLOSE
#ifdef USE_OPEN_DWORK
static void fts_open_work(struct work_struct *work)
{
	int retval;
	struct fts_ts_info *info = container_of(work, struct fts_ts_info,
						open_work.work);

	input_info(true, &info->client->dev, "%s\n", __func__);

	retval = fts_start_device(info);
	if (retval < 0)
		input_err(true, &info->client->dev,
			"%s: Failed to start device\n", __func__);
}
#endif
static int fts_input_open(struct input_dev *dev)
{
	struct fts_ts_info *info = input_get_drvdata(dev);
	int retval;

	if (!info->probe_done) {
		input_dbg(true, &info->client->dev, "%s: not probe\n", __func__);
		goto out;
	}

	input_dbg(false, &info->client->dev, "%s\n", __func__);
	
#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
	if(TRUSTEDUI_MODE_TUI_SESSION & trustedui_get_current_mode()){	
		input_err(true, &info->client->dev, "%s TUI cancel event call!\n", __func__);
		msleep(100);
		tui_force_close(1);
		msleep(200);
		if(TRUSTEDUI_MODE_TUI_SESSION & trustedui_get_current_mode()){	
			input_err(true, &info->client->dev, "%s TUI flag force clear!\n",	__func__);
			trustedui_clear_mask(TRUSTEDUI_MODE_VIDEO_SECURED|TRUSTEDUI_MODE_INPUT_SECURED);
			trustedui_set_mode(TRUSTEDUI_MODE_OFF);
		}
	}
#endif

#ifdef USE_OPEN_DWORK
	schedule_delayed_work(&info->open_work,
			      msecs_to_jiffies(TOUCH_OPEN_DWORK_TIME));
#else
	retval = fts_start_device(info);
	if (retval < 0) {
		input_err(true, &info->client->dev,
			"%s: Failed to start device\n", __func__);
		goto out;
	}
#endif

out:
	return 0;
}

static void fts_input_close(struct input_dev *dev)
{
	struct fts_ts_info *info = input_get_drvdata(dev);

	if (!info->probe_done || info->shutdown_is_on_going) {
		input_dbg(false, &info->client->dev, "%s: not probe\n", __func__);
		return;
	}

	input_dbg(false, &info->client->dev, "%s\n", __func__);

#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
	if(TRUSTEDUI_MODE_TUI_SESSION & trustedui_get_current_mode()){	
		input_err(true, &info->client->dev, "%s TUI cancel event call!\n", __func__);
		msleep(100);
		tui_force_close(1);
		msleep(200);
		if(TRUSTEDUI_MODE_TUI_SESSION & trustedui_get_current_mode()){	
			input_err(true, &info->client->dev, "%s TUI flag force clear!\n",	__func__);
			trustedui_clear_mask(TRUSTEDUI_MODE_VIDEO_SECURED|TRUSTEDUI_MODE_INPUT_SECURED);
			trustedui_set_mode(TRUSTEDUI_MODE_OFF);
		}
	}
#endif

#ifdef USE_OPEN_DWORK
	cancel_delayed_work(&info->open_work);
#endif
	cancel_delayed_work(&info->reset_work);

#ifndef CONFIG_SEC_FACTORY
	if (info->board->always_lpmode && info->board->support_pressure)
		info->lowpower_flag |= FTS_MODE_PRESSURE;
#endif

	info->pressure_setting_mode = 0;

	fts_stop_device(info, info->lowpower_flag);

#ifdef FTS_SUPPORT_HOVER
	info->retry_hover_enable_after_wakeup = 0;
#endif
}
#endif

#if 0//def CONFIG_SEC_FACTORY
static void fts_reinit_fac(struct fts_ts_info *info)
{
	info->touch_count = 0;

	fts_command(info, FLUSHBUFFER);
	fts_delay(10);
	fts_command(info, SENSEON);
#ifdef FTS_SUPPORT_PRESSURE_SENSOR
	fts_command(info, FTS_CMD_PRESSURE_SENSE_ON);
#endif
	fts_delay(50);

#ifdef FTS_SUPPORT_TOUCH_KEY
	if (info->board->support_mskey)
		info->fts_command(info, FTS_CMD_KEY_SENSE_ON);
#endif

#ifdef FTS_SUPPORT_NOISE_PARAM
	fts_get_noise_param_address(info);
#endif

	if (info->flip_enable)
		fts_set_cover_type(info, true);
#ifdef CONFIG_GLOVE_TOUCH
	/* enable glove touch when flip cover is closed */
	if (info->fast_glove_enabled)
		fts_command(info, FTS_CMD_SET_FAST_GLOVE_MODE);
	else if (info->glove_enabled)
		fts_command(info, FTS_CMD_GLOVE_ON);
#endif
	fts_command(info, FORCECALIBRATION);

	fts_interrupt_set(info, INT_ENABLE);

	input_info(true, &info->client->dev, "%s\n", __func__);
}
#endif

void fts_reinit(struct fts_ts_info *info)
{
	fts_systemreset(info, 10);

	fts_read_chip_id(info);

#ifdef FTS_SUPPORT_NOISE_PARAM
	fts_set_noise_param(info);
#endif

#if defined(FTS_SUPPORT_STRINGLIB) && defined(CONFIG_SEC_FACTORY)
	fts_disable_string(info);
#endif

	fts_command(info, FLUSHBUFFER);
	fts_delay(10);
	fts_command(info, SENSEON);
#ifdef FTS_SUPPORT_PRESSURE_SENSOR
	fts_command(info, FTS_CMD_PRESSURE_SENSE_ON);
#endif
	fts_delay(50);

#ifdef FTS_SUPPORT_TOUCH_KEY
	if (info->board->support_mskey)
		info->fts_command(info, FTS_CMD_KEY_SENSE_ON);
#endif

	if (info->flip_enable)
		fts_set_cover_type(info, true);
	else if (info->fast_glove_enabled)
		fts_command(info, FTS_CMD_SET_FAST_GLOVE_MODE);
	else if (info->glove_enabled)
		fts_command(info, FTS_CMD_GLOVE_ON);

#ifdef FTS_SUPPORT_TA_MODE
	if (info->TA_Pluged)
		fts_command(info, FTS_CMD_CHARGER_PLUGGED);
#endif

	if (info->dex_mode) {
		unsigned char regAdd[3] = {0xC1, 0x13, info->dex_mode};
		input_info(true, &info->client->dev, "%s: set dex mode\n", __func__);
		if (fts_write_reg(info, regAdd, 3) < 0)
			input_err(true, &info->client->dev, "%s: dex_enable failed\n", __func__);
	}

	if (info->brush_mode) {
		unsigned char regAdd[2] = {0xC1, 0x14};
		input_info(true, &info->client->dev, "%s: set brush mode\n", __func__);
		if (fts_write_reg(info, regAdd, 2) < 0)
			input_err(true, &info->client->dev, "%s: brush_enable failed\n", __func__);
	}

	if (info->touchable_area) {
		unsigned char regAdd[2] = {0xC1, 0x15};
		input_info(true, &info->client->dev, "%s: set 16:9 mode\n", __func__);
		if (fts_write_reg(info, regAdd, 2) < 0)
			input_err(true, &info->client->dev, "%s: set_touchable_area failed\n", __func__);
	}

	/* because edge and dead zone will recover soon */
	fts_set_grip_type(info, ONLY_EDGE_HANDLER);

	info->touch_count = 0;

	fts_interrupt_set(info, INT_ENABLE);
}

void fts_release_all_finger(struct fts_ts_info *info)
{
	int i;

	for (i = 0; i < FINGER_MAX; i++) {
		input_mt_slot(info->input_dev, i);

		if (info->board->support_mt_pressure)
			input_report_abs(info->input_dev, ABS_MT_PRESSURE, 0);

		input_mt_report_slot_state(info->input_dev, MT_TOOL_FINGER, 0);

		if ((info->finger[i].state == EVENTID_ENTER_POINTER) ||
			(info->finger[i].state == EVENTID_MOTION_POINTER)) {
			input_info(true, &info->client->dev,
				"[RA] tID:%d mc:%d tc:%d Ver[%02X%04X|%01X]\n",
				i, info->finger[i].mcount, info->touch_count,
				info->panel_revision, info->fw_main_version_of_ic,
				info->flip_enable);

			do_gettimeofday(&info->time_released[i]);
			
			if (info->time_longest < (info->time_released[i].tv_sec - info->time_pressed[i].tv_sec))
				info->time_longest = (info->time_released[i].tv_sec - info->time_pressed[i].tv_sec);
		}

		info->finger[i].state = EVENTID_LEAVE_POINTER;
		info->finger[i].mcount = 0;
	}

	info->touch_count = 0;

	input_report_key(info->input_dev, BTN_TOUCH, 0);
	input_report_key(info->input_dev, BTN_TOOL_FINGER, 0);

#ifdef CONFIG_GLOVE_TOUCH
	input_report_switch(info->input_dev, SW_GLOVE, false);
	info->touch_mode = FTS_TM_NORMAL;
#endif
	input_report_key(info->input_dev, KEY_HOMEPAGE, 0);

	if (info->board->support_sidegesture) {
		input_report_key(info->input_dev, KEY_SIDE_GESTURE, 0);
		input_report_key(info->input_dev, KEY_SIDE_GESTURE_RIGHT, 0);
		input_report_key(info->input_dev, KEY_SIDE_GESTURE_LEFT, 0);
	}

	input_sync(info->input_dev);

	info->check_multi = 0;
}

#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
void trustedui_mode_stm_on(void)
{
	if (!tui_tsp_info)
		return;

	fts_release_all_finger(tui_tsp_info);
}
EXPORT_SYMBOL(trustedui_mode_stm_on);
#endif

static void dump_tsp_rawdata(struct work_struct *work)
{
	struct fts_ts_info *info = container_of(work, struct fts_ts_info,
							debug_work.work);
	int i;

	if (info->rawdata_read_lock == true)
		input_err(true, &info->client->dev, "%s: ## checking.. ignored.\n", __func__);

	info->rawdata_read_lock = true;
	input_info(true, &info->client->dev, "%s: ## run CX data ##, %d\n", __func__, __LINE__);
	run_cx_data_read((void *)&info->sec);

	for (i = 0; i < 5; i++) {
		input_info(true, &info->client->dev, "%s: ## run Raw Cap data ##, %d\n", __func__, __LINE__);
		run_rawcap_read((void *)&info->sec);

		input_info(true, &info->client->dev, "%s: ## run Delta ##, %d\n", __func__, __LINE__);
		run_delta_read((void *)&info->sec);
		fts_delay(50);
	}
	input_info(true, &info->client->dev, "%s: ## Done ##, %d\n", __func__, __LINE__);

	info->rawdata_read_lock = false;
}

void tsp_dump(void)
{
#ifdef CONFIG_BATTERY_SAMSUNG
	if (lpcharge)
		return;
#endif
	if (!p_debug_work)
		return;

	pr_err("%s: %s %s: start\n", FTS_TS_DRV_NAME, SECLOG, __func__);
	schedule_delayed_work(p_debug_work, msecs_to_jiffies(100));
}

static void fts_reset(struct fts_ts_info *info, unsigned int ms)
{
	input_info(true, &info->client->dev, "%s: Recover IC, discharge time:%d\n", __func__, ms);

	if (info->board->power)
		info->board->power(info, false);

	fts_delay(ms);

	if (info->board->power)
		info->board->power(info, true);

	fts_delay(5);

	fts_set_warmboot_crc_enable(info);

}

static void fts_reset_work(struct work_struct *work)
{
	struct fts_ts_info *info = container_of(work, struct fts_ts_info,
						reset_work.work);

#ifdef CONFIG_SECURE_TOUCH
	if (atomic_read(&info->st_enabled)) {
		input_err(true, &info->client->dev, "%s: secure touch enabled \n",
			__func__);
		return;
	}
#endif

	input_info(true, &info->client->dev, "%s: Call Power-Off to recover IC\n", __func__);
	info->reset_is_on_going = true;
	
	fts_stop_device(info, false);

	msleep(100);	/* Delay to discharge the IC from ESD or On-state.*/
	if (fts_start_device(info) < 0)
		input_err(true, &info->client->dev, "%s: Failed to start device\n", __func__);

	if (info->input_dev_touch->disabled) {
		input_err(true, &info->client->dev , "%s: call input_close\n", __func__);

		fts_stop_device(info, info->lowpower_flag);

		if ((info->lowpower_flag & FTS_MODE_AOD) && info->use_sponge) {
			int i, ret = -1;
			u8 data[8] = {0, };
#ifdef FTS_SUPPORT_STRINGLIB
			unsigned short addr = FTS_CMD_STRING_ACCESS + 2;
#endif
			for (i = 0; i < 4; i++) {
				data[i * 2] = info->rect_data[i] & 0xFF;
				data[i * 2 + 1] = (info->rect_data[i] >> 8) & 0xFF;
			}

			disable_irq(info->client->irq);
#ifdef FTS_SUPPORT_STRINGLIB
			ret = info->fts_write_to_string(info, &addr, data, sizeof(data));
#endif
			if (ret < 0)
				input_err(true, &info->client->dev, "%s: failed. ret: %d\n", __func__, ret);
			enable_irq(info->client->irq);
		}
	}
	info->reset_is_on_going = false;
}

static void fts_read_info_work(struct work_struct *work)
{
	struct fts_ts_info *info = container_of(work, struct fts_ts_info,
						work_read_info.work);
	unsigned char *data = NULL;
	unsigned char index = 0;
	short minval = 0x7FFF;
	short maxval = 0x8000;
	int ret;

	fts_get_calibration_information(info);

	fts_get_factory_debug_information(info);
	if (info->boot_crc_check_fail == FTS_BOOT_CRC_FAIL) {

		info->nv_crc_fail_count++;
		if (info->nv_crc_fail_count > 0xFE)
			info->nv_crc_fail_count = 0xFE;

		fts_set_factory_debug_information(info, info->pressure_cal_base,
				info->pressure_cal_delta, info->nv_crc_fail_count);
	}

	ret = fts_get_tsp_test_result(info);
	if (ret < 0)
		input_err(true, &info->client->dev, "%s: failed to get result\n",
			__func__);

	input_raw_info(true, &info->client->dev, "%s: test result:%02X, cal: %02X, fix ver:%04X\n",
		__func__, info->test_result.data[0], info->cal_count, info->tune_fix_ver);

	ret = get_nvm_data(info, GROUP_INDEX, &index);

	/*
	 * index is 0, 0xFF : cleared, do not calibrated
	 * index is 1 : Ass'y
	 * index is 2 : Rear
	 * index is 3 : BackGlass
	 */
	data = kzalloc(nvm_data[PRESSURE_STRENGTH].length, GFP_KERNEL);
	if (!data)
		goto err_no_mem;;

	ret = get_nvm_data(info, PRESSURE_STRENGTH, data);
	if (ret <= 0)
		goto err_data;

	if ((index > 0) && (index <= 4)) {
		/* calibrated strength is saved by (index - 1) in flash memory */
		info->pressure_left = (short)(data[(index - 1) * 8 + 0] | ((data[(index - 1) * 8 + 1] << 8) & 0xFF00));
		info->pressure_center = (short)(data[(index - 1) * 8 + 2] | ((data[(index - 1) * 8 + 3] << 8) & 0xFF00));
		info->pressure_right = (short)(data[(index - 1) * 8 + 4] | ((data[(index - 1) * 8 + 5] << 8) & 0xFF00));

		input_raw_info(true, &info->client->dev, "%s: [pressure][index:%d]: %d, %d, %d\n",
			__func__, index, info->pressure_left, info->pressure_center, info->pressure_right);
	} else if (index == 0) {
		input_raw_info(true, &info->client->dev, "%s: [pressure] do not calibrated\n", __func__);
	} else {
		input_raw_info(true, &info->client->dev, "%s: [pressure]: invalid index: %d\n",
			__func__, index);
	}
	fts_panel_ito_test(info);

	input_raw_info(true, &info->client->dev, "%s: [ito] %02X, %02X, %02X, %02X\n",
		__func__, info->ito_test[0], info->ito_test[1], info->ito_test[2], info->ito_test[3]);

	fts_read_frame(info, TYPE_BASELINE_DATA, &minval, &maxval);

	kfree(data);

	input_log_fix();

	return;

err_data:
	kfree(data);
err_no_mem:
	input_info(true, &info->client->dev, "%s: failed\n", __func__);
	input_log_fix();

	return;
}

static int fts_stop_device(struct fts_ts_info *info, bool lpmode)
{
	input_info(true, &info->client->dev, "%s\n", __func__);

#if defined(CONFIG_SECURE_TOUCH)
	fts_secure_touch_stop(info, 1);
#endif
	mutex_lock(&info->device_mutex);

	if (info->touch_stopped) {
		input_err(true, &info->client->dev, "%s: already power off\n", __func__);
		goto out;
	}

	if (lpmode) {
		input_info(true, &info->client->dev, "%s: lowpower flag:%d\n", __func__, info->lowpower_flag);

		if (info->board->support_sidegesture) {
			fts_enable_feature(info, FTS_FEATURE_SIDE_GUSTURE, true);
			fts_delay(20);
		}

		if (device_may_wakeup(&info->client->dev))
			enable_irq_wake(info->irq);

		fts_release_all_finger(info);
#ifdef FTS_SUPPORT_TOUCH_KEY
		fts_release_all_key(info);
#endif
#ifdef FTS_SUPPORT_NOISE_PARAM
		fts_get_noise_param(info);
#endif

		fts_enable_feature(info, FTS_FEATURE_LPM_FUNCTION, true);
		fts_command(info, FTS_CMD_LOWPOWER_MODE);

		info->fts_power_state = FTS_POWER_STATE_LOWPOWER;

#ifdef FTS_SUPPORT_STRINGLIB
		{
			unsigned short addr = FTS_CMD_STRING_ACCESS;
			int ret;

			ret = info->fts_write_to_string(info, &addr, &info->lowpower_flag, sizeof(info->lowpower_flag));
			if (ret < 0)
				input_err(true, &info->client->dev, "%s: failed. ret: %d\n", __func__, ret);
		}
#endif
	} else {
		fts_interrupt_set(info, INT_DISABLE);
		disable_irq(info->irq);

		fts_command(info, FLUSHBUFFER);
		fts_release_all_finger(info);
#ifdef FTS_SUPPORT_TOUCH_KEY
		fts_release_all_key(info);
#endif
#ifdef FTS_SUPPORT_NOISE_PARAM
		fts_get_noise_param(info);
#endif
		info->touch_stopped = true;
		info->hover_enabled = false;
		info->hover_ready = false;
		info->fts_power_state = FTS_POWER_STATE_POWERDOWN;

		if (info->board->power)
			info->board->power(info, false);
	}
 out:
	mutex_unlock(&info->device_mutex);
	return 0;
}

static int fts_start_device(struct fts_ts_info *info)
{
	input_info(true, &info->client->dev, "%s%s\n",
			__func__, info->fts_power_state ? ": exit low power mode TP" : "");

#if defined(CONFIG_SECURE_TOUCH)
	fts_secure_touch_stop(info, 1);
#endif
	mutex_lock(&info->device_mutex);

	if (info->fts_power_state == FTS_POWER_STATE_ACTIVE) {
		input_err(true, &info->client->dev, "%s: already power on\n", __func__);
		goto out;
	}

	fts_release_all_finger(info);
#ifdef FTS_SUPPORT_TOUCH_KEY
	fts_release_all_key(info);
#endif

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		if (info->board->power)
			info->board->power(info, true);
		info->touch_stopped = false;

		fts_set_warmboot_crc_enable(info);

		info->reinit_done = false;
		fts_reinit(info);
		info->reinit_done = true;
	} else {	/* FTS_POWER_STATE_LOWPOWER */
		fts_interrupt_set(info, INT_DISABLE);
		disable_irq(info->irq);
		fts_command(info, FTS_CMD_NORMAL_MODE);

		if (fts_fw_wait_for_event(info, STATUS_EVENT_FORCE_CAL_DONE_D3)) {
			info->reinit_done = false;
			fts_reinit(info);
			info->reinit_done = true;
		} else {
			fts_interrupt_set(info, INT_ENABLE);
		}

		if (info->force_release) {
			haptic_homekey_release();
			info->force_release = false;
		}

		if (device_may_wakeup(&info->client->dev))
			disable_irq_wake(info->irq);
	}
	info->fts_power_state = FTS_POWER_STATE_ACTIVE;
	enable_irq(info->irq);

#ifdef FTS_SUPPORT_STRINGLIB
	{
		unsigned short addr = FTS_CMD_STRING_ACCESS;
		int ret;

		ret = info->fts_write_to_string(info, &addr, &info->lowpower_flag, sizeof(info->lowpower_flag));
		if (ret < 0)
			input_err(true, &info->client->dev, "%s: failed. ret: %d\n", __func__, ret);
	}
#endif

 out:
	mutex_unlock(&info->device_mutex);

#ifdef FTS_SUPPORT_HOVER
	input_info(true, &info->client->dev, "%s: cmd after wakeup : h%d\n", __func__, info->retry_hover_enable_after_wakeup);

	if (info->retry_hover_enable_after_wakeup == 1) {
		unsigned char regAdd[4] = {0xB0, 0x01, 0x29, 0x41};

		fts_write_reg(info, &regAdd[0], 4);
		fts_command(info, FTS_CMD_HOVER_ON);
		info->hover_enabled = true;
	}
#endif

	fts_wirelesscharger_mode(info);

	return 0;
}

static void fts_shutdown(struct i2c_client *client)
{
	struct fts_ts_info *info = i2c_get_clientdata(client);

	input_info(true, &info->client->dev, "%s\n", __func__);

	fts_remove(client);
}

#ifdef CONFIG_PM
static int fts_pm_suspend(struct device *dev)
{
	struct fts_ts_info *info = dev_get_drvdata(dev);

	input_dbg(false, &info->client->dev, "%s\n", __func__);

#ifdef USE_OPEN_CLOSE
	if (info->input_dev) {
		int retval = mutex_lock_interruptible(&info->input_dev->mutex);

		if (retval) {
			input_err(true, &info->client->dev,
					"%s : mutex error\n", __func__);
			goto out;
		}

		if (!info->input_dev->disabled) {
			info->input_dev->disabled = true;
			if (info->input_dev->users && info->input_dev->close) {
				input_err(true, &info->client->dev,
						"%s called without input_close\n",
						__func__);
				info->input_dev->close(info->input_dev);
			}
			info->input_dev->users = 0;
		}

		mutex_unlock(&info->input_dev->mutex);
	}
out:
#endif

	if (info->fts_power_state > FTS_POWER_STATE_POWERDOWN)
		reinit_completion(&info->resume_done);

	return 0;
}

static int fts_pm_resume(struct device *dev)
{
	struct fts_ts_info *info = dev_get_drvdata(dev);

	input_dbg(false, &info->client->dev, "%s\n", __func__);

	if (info->fts_power_state > FTS_POWER_STATE_POWERDOWN)
		complete_all(&info->resume_done);

	return 0;
}
#endif

#if (!defined(CONFIG_PM)) && !defined(USE_OPEN_CLOSE)
static int fts_suspend(struct i2c_client *client, pm_message_t mesg)
{
	struct fts_ts_info *info = i2c_get_clientdata(client);

	input_dbg(true, &info->client->dev, "%s\n", __func__);

	fts_stop_device(info, info->lowpower_flag);

	return 0;
}

static int fts_resume(struct i2c_client *client)
{

	struct fts_ts_info *info = i2c_get_clientdata(client);

	input_dbg(true, &info->client->dev, "%s\n", __func__);

	fts_start_device(info);

	return 0;
}
#endif

static const struct i2c_device_id fts_device_id[] = {
	{FTS_TS_DRV_NAME, 0},
	{}
};

#ifdef CONFIG_PM
static const struct dev_pm_ops fts_dev_pm_ops = {
	.suspend = fts_pm_suspend,
	.resume = fts_pm_resume,
};
#endif

#ifdef CONFIG_OF
static struct of_device_id fts_match_table[] = {
	{.compatible = "stm,fts_touch",},
	{},
};
#else
#define fts_match_table NULL
#endif

static struct i2c_driver fts_i2c_driver = {
	.driver = {
		   .name = FTS_TS_DRV_NAME,
		   .owner = THIS_MODULE,
#ifdef CONFIG_OF
		   .of_match_table = fts_match_table,
#endif
#ifdef CONFIG_PM
		   .pm = &fts_dev_pm_ops,
#endif
		   },
	.probe = fts_probe,
	.remove = fts_remove,
	.shutdown = fts_shutdown,
#if (!defined(CONFIG_PM)) && !defined(USE_OPEN_CLOSE)
	.suspend = fts_suspend,
	.resume = fts_resume,
#endif
	.id_table = fts_device_id,
};

static int __init fts_driver_init(void)
{
	return i2c_add_driver(&fts_i2c_driver);
}

static void __exit fts_driver_exit(void)
{
	i2c_del_driver(&fts_i2c_driver);
}

MODULE_DESCRIPTION("STMicroelectronics MultiTouch IC Driver");
MODULE_AUTHOR("STMicroelectronics, Inc.");
MODULE_LICENSE("GPL v2");

module_init(fts_driver_init);
module_exit(fts_driver_exit);
