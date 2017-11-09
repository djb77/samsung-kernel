/*
 *  wacom_i2c_func.c - Wacom G5 Digitizer Controller (I2C bus)
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/delay.h>

#include "wacom.h"

static ssize_t epen_firm_update_status_show(struct device *dev,
					    struct device_attribute *attr,
					    char *buf)
{
	struct wacom_i2c *wac_i2c = dev_get_drvdata(dev);
	struct i2c_client *client = wac_i2c->client;
	int status = wac_i2c->wac_feature->update_status;
	int ret = 0;

	input_info(true, &client->dev, "%s:(%d)\n", __func__, status);

	if (status == FW_UPDATE_PASS)
		ret = snprintf(buf, PAGE_SIZE, "PASS\n");
	else if (status == FW_UPDATE_RUNNING)
		ret = snprintf(buf, PAGE_SIZE, "DOWNLOADING\n");
	else if (status == FW_UPDATE_FAIL)
		ret = snprintf(buf, PAGE_SIZE, "FAIL\n");
	else
		ret = 0;

	return ret;
}

static ssize_t epen_firm_version_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	struct wacom_i2c *wac_i2c = dev_get_drvdata(dev);
	struct i2c_client *client = wac_i2c->client;

	input_info(true, &client->dev, "%s: 0x%x|0x%X\n", __func__,
		   wac_i2c->wac_feature->fw_version, wac_i2c->fw_ver_file);

	return snprintf(buf, PAGE_SIZE, "%04X\t%04X\n",
			wac_i2c->wac_feature->fw_version, wac_i2c->fw_ver_file);
}

static ssize_t epen_firmware_update_store(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t count)
{
	struct wacom_i2c *wac_i2c = dev_get_drvdata(dev);
	struct i2c_client *client = wac_i2c->client;
	u8 fw_update_way = FW_NONE;

	input_info(true, &client->dev, "%s\n", __func__);

	switch (*buf) {
	case 'i':
	case 'I':
		fw_update_way = FW_IN_SDCARD;
		break;
	case 'k':
	case 'K':
		fw_update_way = FW_BUILT_IN;
		break;
#ifdef CONFIG_SEC_FACTORY
	case 't':
	case 'T':
		fw_update_way = FW_FACTORY_PROC;
		break;
#endif
	default:
		input_err(true, &client->dev, "wrong parameter\n");
		return count;
	}

	wacom_fw_update(wac_i2c, fw_update_way, true);
	return count;
}

static ssize_t epen_reset_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct wacom_i2c *wac_i2c = dev_get_drvdata(dev);
	struct i2c_client *client = wac_i2c->client;
	int val;

	if (kstrtoint(buf, 0, &val)) {
		input_err(true, &client->dev, "%s: failed to get param\n",
			  __func__);
		return count;
	}

	val = !(!val);
	if (!val)
		goto out;

	wacom_enable_irq(wac_i2c, false);
	wacom_enable_pdct_irq(wac_i2c, false);

	wac_i2c->function_result &= ~EPEN_EVENT_SURVEY;
	wac_i2c->survey_mode = EPEN_SURVEY_MODE_NONE;

	/* Reset IC */
	wacom_reset_hw(wac_i2c);
	/* I2C Test */
	wacom_i2c_query(wac_i2c);

	wacom_enable_irq(wac_i2c, true);
	wacom_enable_pdct_irq(wac_i2c, true);

	input_info(true, &client->dev,
		   "%s: result %d\n", __func__, wac_i2c->query_status);
	return count;

out:
	input_err(true, &client->dev, "%s: invalid value %d\n", __func__, val);

	return count;
}

static ssize_t epen_reset_result_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	struct wacom_i2c *wac_i2c = dev_get_drvdata(dev);
	struct i2c_client *client = wac_i2c->client;
	int ret = 0;

	if (wac_i2c->query_status) {
		input_info(true, &client->dev, "%s: PASS\n", __func__);
		ret = snprintf(buf, PAGE_SIZE, "PASS\n");
	} else {
		input_err(true, &client->dev, "%s: FAIL\n", __func__);
		ret = snprintf(buf, PAGE_SIZE, "FAIL\n");
	}

	return ret;
}

static ssize_t epen_checksum_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	struct wacom_i2c *wac_i2c = dev_get_drvdata(dev);
	struct i2c_client *client = wac_i2c->client;
	int val;

	if (kstrtoint(buf, 0, &val)) {
		input_err(true, &client->dev, "%s: failed to get param\n",
			  __func__);
		return count;
	}

	val = !(!val);
	if (!val)
		goto out;

	wacom_enable_irq(wac_i2c, false);
	wacom_enable_pdct_irq(wac_i2c, false);

	wacom_checksum(wac_i2c);

	wacom_enable_irq(wac_i2c, true);
	wacom_enable_pdct_irq(wac_i2c, true);

	input_info(true, &client->dev,
		   "%s: result %d\n", __func__, wac_i2c->checksum_result);

	return count;

out:
	input_err(true, &client->dev, "%s: invalid value %d\n", __func__, val);

	return count;
}

static ssize_t epen_checksum_result_show(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	struct wacom_i2c *wac_i2c = dev_get_drvdata(dev);
	struct i2c_client *client = wac_i2c->client;
	int ret;

	if (wac_i2c->checksum_result) {
		input_info(true, &client->dev, "checksum, PASS\n");
		ret = snprintf(buf, PAGE_SIZE, "PASS\n");
	} else {
		input_err(true, &client->dev, "checksum, FAIL\n");
		ret = snprintf(buf, PAGE_SIZE, "FAIL\n");
	}

	return ret;
}

static int wacom_open_test(struct wacom_i2c *wac_i2c)
{
	struct i2c_client *client = wac_i2c->client;
	u8 cmd = 0;
	u8 buf[4] = { 0, };
	int ret = 0, retry = 10;

	wac_i2c->connection_check = false;
	wac_i2c->fail_channel = 0;
	wac_i2c->min_adc_val = 0;

	cmd = COM_SAMPLERATE_STOP;
	ret = wacom_i2c_send(wac_i2c, &cmd, 1, WACOM_I2C_MODE_NORMAL);
	if (ret <= 0) {
		input_err(true, &client->dev, "failed to send stop command\n");
		return -1;
	}

	usleep_range(500, 500);

	cmd = COM_OPEN_CHECK_START;
	ret = wacom_i2c_send(wac_i2c, &cmd, 1, WACOM_I2C_MODE_NORMAL);
	if (ret <= 0) {
		input_err(true, &client->dev, "failed to send stop command\n");
		return -1;
	}

	msleep(2000);

	cmd = COM_OPEN_CHECK_STATUS;
	do {
		input_info(true, &client->dev,
			   "read status, retry %d\n", retry);
		ret = wacom_i2c_send(wac_i2c, &cmd, 1, WACOM_I2C_MODE_NORMAL);
		if (ret != 1) {
			input_err(true, &client->dev,
				  "failed to send cmd(ret:%d)\n", ret);
			continue;
		}
		usleep_range(500, 500);
		ret = wacom_i2c_recv(wac_i2c, buf, 4, WACOM_I2C_MODE_NORMAL);
		if (ret != 4) {
			input_err(true, &client->dev,
				  "failed to recv data(ret:%d)\n", ret);
			continue;
		}

		/*
		 *      status value
		 *      0 : data is not ready
		 *      1 : PASS
		 *      2 : Fail (coil function error)
		 *      3 : Fail (All coil function error)
		 */
		if (buf[0] == 1) {
			input_info(true, &client->dev, "Pass\n");
			break;
		}

		msleep(50);
	} while (retry--);

	if (ret > 0) {
		wac_i2c->connection_check = (buf[0] == 1);
	} else {
		wac_i2c->connection_check = false;
		return -1;
	}

	wac_i2c->fail_channel = buf[1];
	wac_i2c->min_adc_val = buf[2] << 8 | buf[3];

	input_info(true, &client->dev,
		   "%s: %s buf[0]:%d, buf[1]:%d, buf[2]:%d, buf[3]:%d\n",
		   __func__, wac_i2c->connection_check ? "Pass" : "Fail",
		   buf[0], buf[1], buf[2], buf[3]);

	cmd = COM_SAMPLERATE_STOP;
	ret = wacom_i2c_send(wac_i2c, &cmd, 1, WACOM_I2C_MODE_NORMAL);
	if (ret <= 0) {
		input_err(true, &client->dev,
			  "failed to send stop cmd for end\n");
		return -2;
	}

	cmd = COM_SAMPLERATE_START;
	ret = wacom_i2c_send(wac_i2c, &cmd, 1, WACOM_I2C_MODE_NORMAL);
	if (ret <= 0) {
		input_err(true, &client->dev,
			  "failed to send start cmd for end\n");
		return -2;
	}

	if (retry <= 0)
		return -1;

	return 0;
}

static ssize_t epen_connection_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	struct wacom_i2c *wac_i2c = dev_get_drvdata(dev);
	struct i2c_client *client = wac_i2c->client;
	int retry = 2;
	int ret, module_ver = 1;

	mutex_lock(&wac_i2c->lock);

	input_info(true, &client->dev, "%s\n", __func__);

	wacom_enable_irq(wac_i2c, false);
	wacom_enable_pdct_irq(wac_i2c, false);

	while (retry--) {
		ret = wacom_open_test(wac_i2c);
		if (!ret)
			break;

		input_err(true, &client->dev, "failed. retry %d\n", retry);

		wacom_power(wac_i2c, false);
		wac_i2c->function_result &= ~EPEN_EVENT_SURVEY;
		wac_i2c->survey_mode = EPEN_SURVEY_MODE_NONE;
 		/* recommended delay in spec */
 		msleep(100);
 		wacom_power(wac_i2c, true);

		if (ret == -1) {
			msleep(150);
			continue;
		} else if (ret == -2) {
			break;
		}
	}

	wacom_enable_irq(wac_i2c, true);
	wacom_enable_pdct_irq(wac_i2c, true);

	mutex_unlock(&wac_i2c->lock);

	input_info(true, &client->dev,
		   "connection_check : %d\n", wac_i2c->connection_check);

	return snprintf(buf, PAGE_SIZE, "%s %d %d %d\n",
			wac_i2c->connection_check ? "OK" : "NG",
			module_ver, wac_i2c->fail_channel,
			wac_i2c->min_adc_val);
}

static ssize_t epen_saving_mode_store(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t count)
{
	struct wacom_i2c *wac_i2c = dev_get_drvdata(dev);
	struct i2c_client *client = wac_i2c->client;
	static u64 call_count;
	int val;

	call_count++;

	if (kstrtoint(buf, 0, &val)) {
		input_err(true, &client->dev, "%s: failed to get param\n",
			  __func__);
		return count;
	}

#ifdef CONFIG_SEC_FACTORY
	input_info(true, &client->dev,
		   "%s : Not support power saving mode(%d) in Factory Bin [%lu]\n",
		   __func__, val, call_count);

	return count;
#endif

	wac_i2c->battery_saving_mode = !(!val);

	input_info(true, &client->dev, "%s: ps %s & pen %s [%lu]\n",
		   __func__, wac_i2c->battery_saving_mode ? "on" : "off",
		   (wac_i2c->function_result & EPEN_EVENT_PEN_OUT) ? "out" : "in",
		   call_count);

	if (!wac_i2c->power_enable && wac_i2c->battery_saving_mode) {
		input_err(true, &wac_i2c->client->dev,
			  "%s: already power off, save & return\n", __func__);
		return count;
	}

	if (!(wac_i2c->function_result & EPEN_EVENT_PEN_OUT))
		wacom_select_survey_mode(wac_i2c, wac_i2c->screen_on);

	return count;
}

static ssize_t epen_insert_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct wacom_i2c *wac_i2c = dev_get_drvdata(dev);
	int pen_state = (wac_i2c->function_result & EPEN_EVENT_PEN_OUT);

	input_info(true, &wac_i2c->client->dev, "%s : pen is %s\n", __func__,
		   pen_state ? "OUT" : "IN");

	return snprintf(buf, PAGE_SIZE, "%d\n", pen_state ? 0 : 1);
}

static ssize_t epen_screen_off_memo_enable_store(struct device *dev,
						 struct device_attribute *attr,
						 const char *buf, size_t count)
{
	struct wacom_i2c *wac_i2c = dev_get_drvdata(dev);
	struct i2c_client *client = wac_i2c->client;
	int val = 0;

	if (kstrtoint(buf, 0, &val)) {
		input_err(true, &client->dev, "%s: failed to get param\n",
			  __func__);
		return count;
	}

#ifdef CONFIG_SEC_FACTORY
	input_info(true, &client->dev,
		   "%s : Not support screen off memo mode(%d) in Factory Bin\n",
		   __func__, val);

	return count;
#endif

	val = !(!val);

	if (val)
		wac_i2c->function_set |= EPEN_SETMODE_AOP_OPTION_SCREENOFFMEMO;
	else
		wac_i2c->function_set &= (~EPEN_SETMODE_AOP_OPTION_SCREENOFFMEMO);

	input_info(true, &client->dev,
		   "%s: ps %s aop(%d) set(0x%x) ret(0x%x)\n", __func__,
		   wac_i2c->battery_saving_mode ? "on" : "off",
		   (wac_i2c->function_set & EPEN_SETMODE_AOP) ? 1 : 0,
		   wac_i2c->function_set, wac_i2c->function_result);

	if (!wac_i2c->screen_on)
		wacom_select_survey_mode(wac_i2c, false);

	return count;
}

static ssize_t epen_aod_enable_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	struct wacom_i2c *wac_i2c = dev_get_drvdata(dev);
	struct i2c_client *client = wac_i2c->client;
	int val = 0;

	if (kstrtoint(buf, 0, &val)) {
		input_err(true, &client->dev, "%s: failed to get param\n",
			  __func__);
		return count;
	}

#ifdef CONFIG_SEC_FACTORY
	input_info(true, &client->dev,
		   "%s : Not support aod mode(%d) in Factory Bin\n",
		   __func__, val);

	return count;
#endif

	val = !(!val);

	if (val)
		wac_i2c->function_set |= EPEN_SETMODE_AOP_OPTION_AOD;
	else
		wac_i2c->function_set &= (~EPEN_SETMODE_AOP_OPTION_AOD);

	input_info(true, &client->dev,
		   "%s: ps %s aop(%d) set(0x%x) ret(0x%x)\n", __func__,
		   wac_i2c->battery_saving_mode ? "on" : "off",
		   (wac_i2c->function_set & EPEN_SETMODE_AOP) ? 1 : 0,
		   wac_i2c->function_set, wac_i2c->function_result);

	if (!wac_i2c->screen_on)
		wacom_select_survey_mode(wac_i2c, false);

	return count;
}

static ssize_t epen_wcharging_mode_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct wacom_i2c *wac_i2c = dev_get_drvdata(dev);
	struct i2c_client *client = wac_i2c->client;

	input_info(true, &client->dev, "%s: %s\n", __func__,
		   !wac_i2c->wcharging_mode ? "NORMAL" : "LOWSENSE");

	return snprintf(buf, PAGE_SIZE, "%s\n",
			!wac_i2c->wcharging_mode ? "NORMAL" : "LOWSENSE");
}

static ssize_t epen_wcharging_mode_store(struct device *dev,
					 struct device_attribute *attr,
					 const char *buf, size_t count)
{
	struct wacom_i2c *wac_i2c = dev_get_drvdata(dev);
	struct i2c_client *client = wac_i2c->client;
	int retval = 0;

	if (kstrtoint(buf, 0, &retval)) {
		input_err(true, &client->dev, "%s: failed to get param\n",
			  __func__);
		return count;
	}

	wac_i2c->wcharging_mode = retval;

	input_info(true, &client->dev, "%s: %d\n", __func__,
		   wac_i2c->wcharging_mode);

	if (!wac_i2c->power_enable) {
		input_err(true, &wac_i2c->client->dev,
			  "%s: power off, save & return\n", __func__);
		return count;
	}

	retval = wacom_i2c_set_sense_mode(wac_i2c);
	if (retval < 0) {
		input_err(true, &client->dev,
			  "%s: do not set sensitivity mode, %d\n", __func__,
			  retval);
	}

	return count;

}

static ssize_t epen_keyboard_mode_store(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t count)
{
	struct wacom_i2c *wac_i2c = dev_get_drvdata(dev);
	struct i2c_client *client = wac_i2c->client;
	int val;

	if (kstrtoint(buf, 0, &val)) {
		input_err(true, &client->dev, "%s: failed to get param\n",
			  __func__);
		return count;
	}

	wac_i2c->keyboard_cover_mode = !(!val);

	input_info(true, &client->dev, "%s: %d\n", __func__, wac_i2c->keyboard_cover_mode);

	return count;
}

/* epen_disable_mode : Use in Secure mode(TUI) and Factory mode
 * 0 : enable wacom
 * 1 : disable wacom
 */
void epen_disable_mode(int mode)
{
	struct wacom_i2c *wac_i2c = wacom_get_drv_data(NULL);
	struct i2c_client *client = wac_i2c->client;

	input_info(true, &client->dev, "%s: %d\n", __func__, mode);

	wac_i2c->epen_blocked = mode;

	if (!wac_i2c->power_enable && wac_i2c->epen_blocked) {
		input_err(true, &wac_i2c->client->dev,
			  "%s: already power off, return\n", __func__);
		return;
	}

	wacom_select_survey_mode(wac_i2c, wac_i2c->screen_on);
}
EXPORT_SYMBOL(epen_disable_mode);

static ssize_t epen_disable_mode_store(struct device *dev,
				       struct device_attribute *attr,
				       const char *buf, size_t count)
{
	struct wacom_i2c *wac_i2c = dev_get_drvdata(dev);
	struct i2c_client *client = wac_i2c->client;
	int val = 0;

	if (kstrtoint(buf, 0, &val)) {
		input_err(true, &client->dev, "%s: failed to get param\n",
			  __func__);
		return count;
	}

	val = !(!val);

	input_info(true, &client->dev, "%s: %d\n", __func__, val);

	epen_disable_mode(val);

	return count;
}

static ssize_t epen_dex_enable_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct wacom_i2c *wac_i2c = dev_get_drvdata(dev);
	struct i2c_client *client = wac_i2c->client;

	input_info(true, &client->dev, "%s: %d\n",
			__func__, wac_i2c->dex_mode);

	return snprintf(buf, PAGE_SIZE, "%d\n", wac_i2c->dex_mode);
}

static ssize_t epen_dex_enable_store(struct device *dev,
					 struct device_attribute *attr,
					 const char *buf, size_t count)
{
	struct wacom_i2c *wac_i2c = dev_get_drvdata(dev);
	struct i2c_client *client = wac_i2c->client;
	int retval = 0;

	if (!wac_i2c->pdata->support_dex) {
		input_err(true, &client->dev, "%s: not support DeX mode\n",
				__func__);
		return count;
	}

	if (kstrtoint(buf, 0, &retval)) {
		input_err(true, &client->dev, "%s: failed to get param\n",
				__func__);
		return count;
	}

	if (retval < 0 || retval > DEX_MODE_ALL) {
		input_err(true, &client->dev, "%s: invalid param %d\n",
				__func__, retval);
		return count;
	}

	forced_release(wac_i2c);

	wac_i2c->dex_mode = retval;
	if (wac_i2c->dex_mode & DEX_MODE_MOUSE)
		wac_i2c->input_dev = wac_i2c->input_dev_pad;
	else
		wac_i2c->input_dev = wac_i2c->input_dev_pen;

	input_info(true, &client->dev, "%s: %s (%s mode%s%s)\n",
			__func__, wac_i2c->dex_mode ? "on" : "off",
			wac_i2c->dex_mode & DEX_MODE_MOUSE ? "mouse" : "stylus",
			wac_i2c->dex_mode & DEX_MODE_EDGE_CROP ? ", edge crop" : "",
			wac_i2c->dex_mode & DEX_MODE_IRIS ? ", Iris" : "");

	return count;

}

/* change&show dex_rate for test */
static ssize_t epen_dex_rate_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct wacom_i2c *wac_i2c = dev_get_drvdata(dev);
	struct i2c_client *client = wac_i2c->client;

	input_info(true, &client->dev, "%s: %d\n",
			__func__, wac_i2c->pdata->dex_rate);

	return snprintf(buf, PAGE_SIZE, "%d\n", wac_i2c->pdata->dex_rate);
}

static ssize_t epen_dex_rate_store(struct device *dev,
					 struct device_attribute *attr,
					 const char *buf, size_t count)
{
	struct wacom_i2c *wac_i2c = dev_get_drvdata(dev);
	struct i2c_client *client = wac_i2c->client;
	int retval = 0;

	if (!wac_i2c->pdata->support_dex) {
		input_err(true, &client->dev, "%s: not support DeX mode\n",
				__func__);
		return count;
	}

	if (kstrtoint(buf, 0, &retval)) {
		input_err(true, &client->dev, "%s: failed to get param\n",
				__func__);
		return count;
	}

	wac_i2c->pdata->dex_rate = retval;

	input_info(true, &client->dev, "%s: %d\n", __func__, wac_i2c->pdata->dex_rate);

	return count;

}

static ssize_t epen_abnormal_reset_count_show(struct device *dev,
					      struct device_attribute *attr,
					      char *buf)
{
	struct wacom_i2c *wac_i2c = dev_get_drvdata(dev);
	struct i2c_client *client = wac_i2c->client;

	input_info(true, &client->dev, "%s: %u\n", __func__,
		   wac_i2c->abnormal_reset_count);

	return snprintf(buf, PAGE_SIZE, "%u", wac_i2c->abnormal_reset_count);
}

static ssize_t epen_clear_abnormal_reset_count_store(struct device *dev,
						     struct device_attribute *attr,
						     const char *buf,
						     size_t count)
{
	struct wacom_i2c *wac_i2c = dev_get_drvdata(dev);
	struct i2c_client *client = wac_i2c->client;

	if (buf[0] != 'c') {
		input_err(true, &client->dev, "%s: failed to get param\n",
			  __func__);
		return count;
	}

	wac_i2c->abnormal_reset_count = 0;

	input_info(true, &client->dev, "%s: clear\n", __func__);

	return count;
}

static ssize_t epen_i2c_fail_count_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct wacom_i2c *wac_i2c = dev_get_drvdata(dev);
	struct i2c_client *client = wac_i2c->client;

	input_info(true, &client->dev, "%s: %u\n", __func__,
		   wac_i2c->i2c_fail_count);

	return snprintf(buf, PAGE_SIZE, "%u", wac_i2c->i2c_fail_count);
}

static ssize_t epen_clear_i2c_fail_count_store(struct device *dev,
					       struct device_attribute *attr,
					       const char *buf, size_t count)
{
	struct wacom_i2c *wac_i2c = dev_get_drvdata(dev);
	struct i2c_client *client = wac_i2c->client;

	if (buf[0] != 'c') {
		input_err(true, &client->dev, "%s: failed to get param\n",
			  __func__);
		return count;
	}

	wac_i2c->i2c_fail_count = 0;

	input_info(true, &client->dev, "%s: clear\n", __func__);

	return count;
}

#ifdef CONFIG_SEC_FACTORY
static ssize_t epen_fac_garage_mode_enable(struct device *dev,
					   struct device_attribute *attr,
					   const char *buf, size_t count)
{
	struct wacom_i2c *wac_i2c = dev_get_drvdata(dev);
	struct i2c_client *client = wac_i2c->client;
	int val;
	u8 cmd;
	int ret;

	if (kstrtoint(buf, 0, &val)) {
		input_err(true, &client->dev, "%s: failed to get param\n",
			  __func__);
		return count;
	}

	val = !(!val);
	if (!val)
		wac_i2c->fac_garage_mode = 0;

	cmd = COM_SAMPLERATE_STOP;
	ret = wacom_i2c_send(wac_i2c, &cmd, 1, WACOM_I2C_MODE_NORMAL);
	if (ret < 0) {
		input_err(true, &client->dev, "failed to send stop cmd %d\n",
			  ret);

		msleep(50);

		wac_i2c->fac_garage_mode = 0;

		return count;
	}

	if (val) {
		msleep(50);

		wac_i2c->fac_garage_mode = 1;
	} else {
		cmd = COM_SAMPLERATE_START;
		ret = wacom_i2c_send(wac_i2c, &cmd, 1, WACOM_I2C_MODE_NORMAL);
		if (ret < 0) {
			input_err(true, &client->dev,
				  "failed to sned start cmd %d\n",
				  ret);
			return count;
		}
	}

	input_info(true, &client->dev, "%s: %d\n", __func__, val);

	return count;
}

static ssize_t epen_fac_garage_mode_show(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	struct wacom_i2c *wac_i2c = dev_get_drvdata(dev);
	struct i2c_client *client = wac_i2c->client;

	input_info(true, &client->dev, "%s: garage mode %s\n", __func__,
		   wac_i2c->fac_garage_mode ? "IN" : "OUT");

	return snprintf(buf, PAGE_SIZE, "garage mode %s\n",
			wac_i2c->fac_garage_mode ? "IN" : "OUT");
}

static ssize_t epen_fac_garage_rawdata_show(struct device *dev,
					    struct device_attribute *attr,
					    char *buf)
{
	struct wacom_i2c *wac_i2c = dev_get_drvdata(dev);
	struct i2c_client *client = wac_i2c->client;
	int ret;
	u8 cmd;

	if (!wac_i2c->fac_garage_mode) {
		input_err(true, &client->dev, "not in factory garage mode\n");
		goto out;
	}

	cmd = COM_REQUESTGARAGEDATA;
	ret = wacom_i2c_send(wac_i2c, &cmd, 1, WACOM_I2C_MODE_NORMAL);
	if (ret < 0) {
		input_err(true, &client->dev,
			  "failed to send request garage data command %d\n",
			  ret);
		msleep(30);

		goto out;
	}

	msleep(30);

	input_info(true, &client->dev, "%s: %d, %d, %d, %d\n", __func__,
		   wac_i2c->garage_gain0, wac_i2c->garage_freq0,
		   wac_i2c->garage_gain1, wac_i2c->garage_freq1);

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d,%d\n", wac_i2c->garage_gain0,
			wac_i2c->garage_freq0, wac_i2c->garage_gain1,
			wac_i2c->garage_freq1);

out:
	return snprintf(buf, PAGE_SIZE, "NG\n");
}
#endif

static ssize_t get_epen_pos_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct wacom_i2c *wac_i2c = dev_get_drvdata(dev);
	struct i2c_client *client = wac_i2c->client;
	int max_x, max_y;

	if (wac_i2c->pdata->xy_switch) {
		max_x = wac_i2c->pdata->max_y;
		max_y = wac_i2c->pdata->max_x;
	} else {
		max_x = wac_i2c->pdata->max_x;
		max_y = wac_i2c->pdata->max_y;
	}

	input_info(true, &client->dev,
			"%s: id(%d), x(%d), y(%d), max_x(%d), max_y(%d)\n", __func__,
			wac_i2c->survey_pos.id, wac_i2c->survey_pos.x, wac_i2c->survey_pos.y,
			max_x, max_y);

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d,%d,%d\n",
				wac_i2c->survey_pos.id, wac_i2c->survey_pos.x, wac_i2c->survey_pos.y,
				max_x, max_y);
}

/* firmware update */
static DEVICE_ATTR(epen_firm_update, (S_IWUSR | S_IWGRP),
		   NULL, epen_firmware_update_store);
/* return firmware update status */
static DEVICE_ATTR(epen_firm_update_status, S_IRUGO,
		   epen_firm_update_status_show, NULL);
/* return firmware version */
static DEVICE_ATTR(epen_firm_version, S_IRUGO, epen_firm_version_show, NULL);
/* For SMD Test */
static DEVICE_ATTR(epen_reset, (S_IWUSR | S_IWGRP), NULL, epen_reset_store);
static DEVICE_ATTR(epen_reset_result, S_IRUGO, epen_reset_result_show, NULL);
/* For SMD Test. Check checksum */
static DEVICE_ATTR(epen_checksum, (S_IWUSR | S_IWGRP),
		   NULL, epen_checksum_store);
static DEVICE_ATTR(epen_checksum_result, S_IRUGO,
		   epen_checksum_result_show, NULL);
static DEVICE_ATTR(epen_connection, S_IRUGO, epen_connection_show, NULL);
static DEVICE_ATTR(epen_saving_mode, (S_IWUSR | S_IWGRP),
		   NULL, epen_saving_mode_store);
static DEVICE_ATTR(epen_insert, S_IRUGO,
			epen_insert_show, NULL);
static DEVICE_ATTR(epen_wcharging_mode, (S_IRUGO | S_IWUSR | S_IWGRP),
		   epen_wcharging_mode_show, epen_wcharging_mode_store);
static DEVICE_ATTR(keyboard_mode, (S_IWUSR | S_IWGRP),
		   NULL, epen_keyboard_mode_store);
static DEVICE_ATTR(epen_disable_mode, (S_IWUSR | S_IWGRP),
		   NULL, epen_disable_mode_store);
static DEVICE_ATTR(screen_off_memo_enable, (S_IWUSR | S_IWGRP),
		   NULL, epen_screen_off_memo_enable_store);
static DEVICE_ATTR(get_epen_pos,
			S_IRUGO, get_epen_pos_show, NULL);
static DEVICE_ATTR(aod_enable, (S_IWUSR | S_IWGRP),
		   NULL, epen_aod_enable_store);
static DEVICE_ATTR(dex_enable, (S_IRUGO | S_IWUSR | S_IWGRP),
		   epen_dex_enable_show, epen_dex_enable_store);
static DEVICE_ATTR(dex_rate, (S_IRUGO | S_IWUSR | S_IWGRP),
		   epen_dex_rate_show, epen_dex_rate_store);
static DEVICE_ATTR(abnormal_reset_count, (S_IRUGO | S_IWUSR | S_IWGRP),
		   epen_abnormal_reset_count_show,
		   epen_clear_abnormal_reset_count_store);
static DEVICE_ATTR(i2c_fail_count, (S_IRUGO | S_IWUSR | S_IWGRP),
		   epen_i2c_fail_count_show, epen_clear_i2c_fail_count_store);
#ifdef CONFIG_SEC_FACTORY
static DEVICE_ATTR(epen_fac_garage_mode, (S_IRUGO | S_IWUSR | S_IWGRP),
		   epen_fac_garage_mode_show, epen_fac_garage_mode_enable);
static DEVICE_ATTR(epen_fac_garage_rawdata, S_IRUGO,
		   epen_fac_garage_rawdata_show, NULL);
#endif

static struct attribute *epen_attributes[] = {
	&dev_attr_epen_firm_update.attr,
	&dev_attr_epen_firm_update_status.attr,
	&dev_attr_epen_firm_version.attr,
	&dev_attr_epen_reset.attr,
	&dev_attr_epen_reset_result.attr,
	&dev_attr_epen_checksum.attr,
	&dev_attr_epen_checksum_result.attr,
	&dev_attr_epen_connection.attr,
	&dev_attr_epen_saving_mode.attr,
	&dev_attr_epen_insert.attr,
	&dev_attr_epen_wcharging_mode.attr,
	&dev_attr_keyboard_mode.attr,
	&dev_attr_epen_disable_mode.attr,
	&dev_attr_screen_off_memo_enable.attr,
	&dev_attr_get_epen_pos.attr,
	&dev_attr_aod_enable.attr,
	&dev_attr_dex_enable.attr,
	&dev_attr_dex_rate.attr,
	&dev_attr_abnormal_reset_count.attr,
	&dev_attr_i2c_fail_count.attr,
#ifdef CONFIG_SEC_FACTORY
	&dev_attr_epen_fac_garage_mode.attr,
	&dev_attr_epen_fac_garage_rawdata.attr,
#endif
	NULL,
};

static struct attribute_group epen_attr_group = {
	.attrs = epen_attributes,
};

int wacom_sec_init(struct wacom_i2c *wac_i2c)
{
	struct i2c_client *client = wac_i2c->client;
	int retval = 0;

	wac_i2c->dev = sec_device_create(wac_i2c, "sec_epen");
	if (IS_ERR(wac_i2c->dev)) {
		input_err(true, &client->dev, "failed to create device\n");
		retval = -ENODEV;
		return retval;
	}

	dev_set_drvdata(wac_i2c->dev, wac_i2c);

	retval = sysfs_create_group(&wac_i2c->dev->kobj, &epen_attr_group);
	if (retval) {
		input_err(true, &client->dev, "failed to create sysfs group\n");
		goto err_sysfs_create_group;
	}

	retval = sysfs_create_link(&wac_i2c->dev->kobj,
				   &wac_i2c->input_dev->dev.kobj, "input");
	if (retval) {
		input_err(true, &client->dev, "failed to create sysfs link\n");
		goto err_create_symlink;
	}

	return retval;

err_create_symlink:
	sysfs_remove_group(&wac_i2c->dev->kobj, &epen_attr_group);
err_sysfs_create_group:
	sec_device_destroy(wac_i2c->dev->devt);

	return retval;
}

void wacom_sec_remove(struct wacom_i2c *wac_i2c)
{
	sysfs_delete_link(&wac_i2c->dev->kobj,
			  &wac_i2c->input_dev->dev.kobj, "input");
	sysfs_remove_group(&wac_i2c->dev->kobj, &epen_attr_group);
	sec_device_destroy(wac_i2c->dev->devt);
}
