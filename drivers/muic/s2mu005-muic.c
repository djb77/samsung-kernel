/*
 * driver/misc/s2mu005.c - S2MU005 micro USB switch device driver
 *
 * Copyright (C) 2015 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/delay.h>
#if defined(CONFIG_BATTERY_SAMSUNG_V2)
#include "../battery_v2/include/sec_charging_common.h"
#endif

#include <linux/muic/muic.h>
#include <linux/mfd/samsung/s2mu005.h>
#include <linux/mfd/samsung/s2mu005-private.h>
#include <linux/muic/s2mu005-muic.h>

#if defined(CONFIG_MUIC_NOTIFIER)
#include <linux/muic/muic_notifier.h>
#endif /* CONFIG_MUIC_NOTIFIER */

#if defined(CONFIG_VBUS_NOTIFIER)
#include <linux/vbus_notifier.h>
#endif /* CONFIG_VBUS_NOTIFIER */

#if defined(CONFIG_MUIC_UART_SWITCH)
#include <mach/pinctrl-samsung.h>
#endif

#if defined (CONFIG_KEEP_JIG_LOW)
#include <linux/sec_param.h>
#endif

static void s2mu005_muic_handle_attach(struct s2mu005_muic_data *muic_data,
			muic_attached_dev_t new_dev, int adc, u8 vbvolt);
static void s2mu005_muic_handle_detach(struct s2mu005_muic_data *muic_data);
static void s2mu005_muic_detect_dev(struct s2mu005_muic_data *muic_data);
#if !defined (CONFIG_SEC_FACTORY) && !defined (CONFIG_MUIC_S2MU005_WATER_WA_DISABLE)
static void s2mu005_muic_set_water_wa(struct s2mu005_muic_data *muic_data, bool en);
static int s2mu005_i2c_update_bit(struct i2c_client *i2c,
			u8 reg, u8 mask, u8 shift, u8 value);
#endif

#if defined (CONFIG_KEEP_JIG_LOW)
static void set_jig_level_low(struct s2mu005_muic_data *muic_data,bool on);
static int always_keep_jigonb_low;
#endif

/*
#define DEBUG_MUIC
*/

#if defined(DEBUG_MUIC)
#define MAX_LOG 25
#define READ 0
#define WRITE 1

static u8 s2mu005_log_cnt;
static u8 s2mu005_log[MAX_LOG][3];

static void s2mu005_reg_log(u8 reg, u8 value, u8 rw)
{
	s2mu005_log[s2mu005_log_cnt][0] = reg;
	s2mu005_log[s2mu005_log_cnt][1] = value;
	s2mu005_log[s2mu005_log_cnt][2] = rw;
	s2mu005_log_cnt++;
	if (s2mu005_log_cnt >= MAX_LOG)
		s2mu005_log_cnt = 0;
}
static void s2mu005_print_reg_log(void)
{
	int i;
	u8 reg, value, rw;
	char mesg[256] = "";

	for (i = 0; i < MAX_LOG; i++) {
		reg = s2mu005_log[s2mu005_log_cnt][0];
		value = s2mu005_log[s2mu005_log_cnt][1];
		rw = s2mu005_log[s2mu005_log_cnt][2];
		s2mu005_log_cnt++;

		if (s2mu005_log_cnt >= MAX_LOG)
			s2mu005_log_cnt = 0;
		sprintf(mesg+strlen(mesg), "%x(%x)%x ", reg, value, rw);
	}
	pr_info("%s:%s:%s\n", MUIC_DEV_NAME, __func__, mesg);
}
void s2mu005_read_reg_dump(struct s2mu005_muic_data *muic, char *mesg)
{
	u8 val;

	s2mu005_read_reg(muic->i2c, S2MU005_REG_MUIC_CTRL1, &val);
	sprintf(mesg+strlen(mesg), "CTRL1:%x ", val);
	s2mu005_read_reg(muic->i2c, S2MU005_REG_MUIC_SW_CTRL, &val);
	sprintf(mesg+strlen(mesg), "SW_CTRL:%x ", val);
	s2mu005_read_reg(muic->i2c, S2MU005_REG_MUIC_INT1_MASK, &val);
	sprintf(mesg+strlen(mesg), "IM1:%x ", val);
	s2mu005_read_reg(muic->i2c, S2MU005_REG_MUIC_INT2_MASK, &val);
	sprintf(mesg+strlen(mesg), "IM2:%x ", val);
	s2mu005_read_reg(muic->i2c, S2MU005_REG_MUIC_CHG_TYPE, &val);
	sprintf(mesg+strlen(mesg), "CHG_T:%x ", val);
	s2mu005_read_reg(muic->i2c, S2MU005_REG_MUIC_DEVICE_APPLE, &val);
	sprintf(mesg+strlen(mesg), "APPLE_DT:%x ", val);
	s2mu005_read_reg(muic->i2c, S2MU005_REG_MUIC_ADC, &val);
	sprintf(mesg+strlen(mesg), "ADC:%x ", val);
	s2mu005_read_reg(muic->i2c, S2MU005_REG_MUIC_DEVICE_TYPE1, &val);
	sprintf(mesg+strlen(mesg), "DT1:%x ", val);
	s2mu005_read_reg(muic->i2c, S2MU005_REG_MUIC_DEVICE_TYPE2, &val);
	sprintf(mesg+strlen(mesg), "DT2:%x ", val);
	s2mu005_read_reg(muic->i2c, S2MU005_REG_MUIC_DEVICE_TYPE3, &val);
	sprintf(mesg+strlen(mesg), "DT3:%x ", val);
}
void s2mu005_print_reg_dump(struct s2mu005_muic_data *muic_data)
{
	char mesg[256] = "";

	s2mu005_read_reg_dump(muic_data, mesg);

	pr_info("%s:%s:%s\n", MUIC_DEV_NAME, __func__, mesg);
}
#endif

#if defined (CONFIG_KEEP_JIG_LOW)
static int set_jig_low(char *str)
{
	get_option(&str, &always_keep_jigonb_low);
	
	pr_info("%s: always_keep_jigonb_low: 0x%03x\n", __func__,
			always_keep_jigonb_low);
	return always_keep_jigonb_low;
}
__setup("keep_jig_low=",set_jig_low);
#endif

static int s2mu005_i2c_read_byte(struct i2c_client *client, u8 command)
{
	u8 ret;
	int retry = 0;

	s2mu005_read_reg(client, command, &ret);

	while (ret < 0) {
		pr_err("%s:%s: reg(0x%x), retrying...\n", MUIC_DEV_NAME, __func__, command);
		if (retry > 5) {
			pr_err("%s:%s: retry failed!!\n", MUIC_DEV_NAME, __func__);
			break;
		}
		msleep(100);
		s2mu005_read_reg(client, command, &ret);
		retry++;
	}

#ifdef DEBUG_MUIC
	s2mu005_reg_log(command, ret, retry << 1 | READ);
#endif
	return ret;
}

static int s2mu005_i2c_write_byte(struct i2c_client *client,
			u8 command, u8 value)
{
	int ret;
	int retry = 0;
	u8 written = 0;

	ret = s2mu005_write_reg(client, command, value);

	while (ret < 0) {
		pr_err("%s:%s: reg(0x%x), retrying...\n", MUIC_DEV_NAME, __func__, command);
		if (retry > 5) { 
			pr_err("%s:%s: retry failed!!\n", MUIC_DEV_NAME, __func__);
			break;
		}
		msleep(100);
		s2mu005_read_reg(client, command, &written);
		if (written < 0)
			pr_err("%s:%s: reg(0x%x)\n", MUIC_DEV_NAME, __func__, command);
		ret = s2mu005_write_reg(client, command, value);
		retry++;
	}
#ifdef DEBUG_MUIC
	s2mu005_reg_log(command, value, retry << 1 | WRITE);
#endif
	return ret;
}

#if !defined (CONFIG_SEC_FACTORY) && !defined (CONFIG_MUIC_S2MU005_WATER_WA_DISABLE)
static int s2mu005_i2c_update_bit(struct i2c_client *i2c,
			u8 reg, u8 mask, u8 shift, u8 value)
{
	int ret;
	u8 reg_val = 0;

	reg_val = s2mu005_i2c_read_byte(i2c, reg);
	reg_val &= ~mask;
	reg_val |= value << shift;
	ret = s2mu005_i2c_write_byte(i2c, reg, reg_val);
	pr_info("[update_bit:%s] reg(0x%x):  value(0x%x)\n", __func__, reg, reg_val);
	if (ret < 0) {
		pr_err("%s: Reg = 0x%X, mask = 0x%X, val = 0x%X write err : %d\n",
				__func__, reg, mask, value, ret);
	}

	return ret;
}
#endif

#if defined(GPIO_DOC_SWITCH)
static int s2mu005_set_gpio_doc_switch(int val)
{
	int doc_switch_gpio = muic_pdata.gpio_doc_switch;
	int doc_switch_val;
	int ret;

	ret = gpio_request(doc_switch_gpio, "GPIO_DOC_SWITCH");
	if (ret) {
		pr_err("%s:%s: failed to gpio_request GPIO_DOC_SWITCH\n", MUIC_DEV_NAME, __func__);
		return ret;
	}

	doc_switch_val = gpio_get_value(doc_switch_gpio);

	if (gpio_is_valid(doc_switch_gpio))
			gpio_set_value(doc_switch_gpio, val);

	doc_switch_val = gpio_get_value(doc_switch_gpio);

	gpio_free(doc_switch_gpio);

	pr_info("%s:%s: GPIO_DOC_SWITCH(%d)=%c\n",
		MUIC_DEV_NAME, __func__, doc_switch_gpio, (doc_switch_val == 0 ? 'L' : 'H'));

	return 0;
}
#endif /* GPIO_DOC_SWITCH */

#if defined (CONFIG_SEC_FACTORY) && defined (CONFIG_USB_HOST_NOTIFY)
static int set_otg_reg(struct s2mu005_muic_data *muic_data, bool on)
{
	struct i2c_client *i2c = muic_data->i2c;
	u8 reg_val;
	int ret = 0;

	/* 0x1e : hidden register */
	ret = s2mu005_i2c_read_byte(i2c, 0x1e);
	if (ret < 0)
		pr_err("%s:%s: err read 0x1e reg(%d)\n", MUIC_DEV_NAME, __func__, ret);

	/* Set 0x1e[5:4] bit to 0x11 or 0x01 */
	if (on)
		reg_val = ret | (0x1 << 5);
	else
		reg_val = ret & ~(0x1 << 5);

	if (reg_val ^ ret) {
		pr_info("%s:%s: 0x%x != 0x%x, update\n", MUIC_DEV_NAME, __func__, reg_val, ret);

		ret = s2mu005_i2c_write_byte(i2c, 0x1e, reg_val);
		if (ret < 0)
			pr_err("%s:%s: err write(%d)\n", MUIC_DEV_NAME, __func__, ret);
	} else {
		pr_info("%s:%s: 0x%x == 0x%x, just return\n", MUIC_DEV_NAME, __func__, reg_val, ret);
		return 0;
	}

	ret = s2mu005_i2c_read_byte(i2c, 0x1e);
	if (ret < 0)
		pr_err("%s:%s: err read reg 0x1e(%d)\n", MUIC_DEV_NAME, __func__, ret);
	else
		pr_info("%s:%s: after change(0x%x)\n", MUIC_DEV_NAME, __func__, ret);

	return ret;
}

static int init_otg_reg(struct s2mu005_muic_data *muic_data)
{
	struct i2c_client *i2c = muic_data->i2c;
	u8 reg_val;
	int ret = 0;

	/* 0x73 : check EVT0 or EVT1 */
	ret = s2mu005_i2c_read_byte(i2c, 0x73);
	if (ret < 0)
		pr_err("%s:%s: err read 'reg 0x73'(%d)\n", MUIC_DEV_NAME, __func__, ret);

	if ((ret&0xF) > 0)
		return 0;

	/* 0x89 : hidden register */
	ret = s2mu005_i2c_read_byte(i2c, 0x89);
	if (ret < 0)
		pr_err("%s:%s: err read 'reg 0x89'(%d)\n", MUIC_DEV_NAME, __func__, ret);

	/* Set 0x89[1] bit : T_DET_VAL */
	reg_val = ret | (0x1 << 1);

	if (reg_val ^ ret) {
		pr_info("%s:%s: 0x%x != 0x%x, update\n", MUIC_DEV_NAME, __func__, reg_val, ret);

		ret = s2mu005_i2c_write_byte(i2c, 0x89, reg_val);
		if (ret < 0)
			pr_err("%s:%s: err write(%d)\n", MUIC_DEV_NAME, __func__, ret);
	} else {
		pr_info("%s:%s: 0x%x == 0x%x, just return\n", MUIC_DEV_NAME, __func__, reg_val, ret);
		return 0;
	}

	ret = s2mu005_i2c_read_byte(i2c, 0x89);
	if (ret < 0)
		pr_err("%s:%s: err read 'reg 0x89'(%d)\n", MUIC_DEV_NAME, __func__, ret);
	else
		pr_info("%s:%s: after change(0x%x)\n", MUIC_DEV_NAME, __func__, ret);

	/* 0x92 : hidden register */
	ret = s2mu005_i2c_read_byte(i2c, 0x92);
	if (ret < 0)
		pr_err("%s:%s: err read 'reg 0x92'(%d)\n", MUIC_DEV_NAME, __func__, ret);

	/* Set 0x92[7] bit : EN_JIG_AP */
	reg_val = ret | (0x1 << 7);

	if (reg_val ^ ret) {
		pr_info("%s:%s: 0x%x != 0x%x, update\n", MUIC_DEV_NAME, __func__, reg_val, ret);

		ret = s2mu005_i2c_write_byte(i2c, 0x92, reg_val);
		if (ret < 0)
			pr_err("%s:%s: err write(%d)\n", MUIC_DEV_NAME, __func__, ret);
	} else {
		pr_info("%s:%s: 0x%x == 0x%x, just return\n", MUIC_DEV_NAME, __func__, reg_val, ret);
		return 0;
	}

	ret = s2mu005_i2c_read_byte(i2c, 0x92);
	if (ret < 0)
		pr_err("%s:%s: err read 'reg 0x92'(%d)\n", MUIC_DEV_NAME, __func__, ret);
	else
		pr_info("%s:%s: after change(0x%x)\n", MUIC_DEV_NAME, __func__, ret);

	return ret;
}
#endif

static int set_jig_sw(struct s2mu005_muic_data *muic_data, bool en)
{
	struct i2c_client *i2c = muic_data->i2c;
	bool cur = false;
	int reg_val = 0, ret = 0;

	reg_val = s2mu005_i2c_read_byte(i2c, S2MU005_REG_MUIC_SW_CTRL);
	if (reg_val < 0)
		pr_err("%s:%s failed to read 0x%x\n", MUIC_DEV_NAME, __func__, reg_val);

	cur = !!(reg_val & MANUAL_SW_JIG_EN);

	if (!muic_data->jigonb_enable)
		en = false;

	if (en != cur) {
		pr_info("%s:%s  0x%x != 0x%x, update\n", MUIC_DEV_NAME, __func__,
			reg_val, reg_val);

		if (en)
			reg_val |= (MANUAL_SW_JIG_EN);
		else
			reg_val &= ~(MANUAL_SW_JIG_EN);

		ret = s2mu005_i2c_write_byte(i2c, S2MU005_REG_MUIC_SW_CTRL, reg_val);
		if (ret < 0)
			pr_err("%s:%s failed to write 0x%x\n", MUIC_DEV_NAME, __func__, reg_val);
	}

	return ret;
}

static ssize_t s2mu005_muic_show_uart_en(struct device *dev,
						struct device_attribute *attr, char *buf)
{
	struct s2mu005_muic_data *muic_data = dev_get_drvdata(dev);
	int ret = 0;

	if (!muic_data->is_rustproof) {
		pr_info("%s:%s: UART ENABLE\n", MUIC_DEV_NAME, __func__);
		ret = sprintf(buf, "1\n");
	} else {
		pr_info("%s:%s: UART DISABLE\n", MUIC_DEV_NAME, __func__);
		ret = sprintf(buf, "0\n");
	}

	return ret;
}

static ssize_t s2mu005_muic_set_uart_en(struct device *dev,
						struct device_attribute *attr,
						const char *buf, size_t count)
{
	struct s2mu005_muic_data *muic_data = dev_get_drvdata(dev);

	if (!strncmp(buf, "1", 1))
		muic_data->is_rustproof = false;
	else if (!strncmp(buf, "0", 1))
		muic_data->is_rustproof = true;
	else
		pr_info("%s:%s: invalid value\n", MUIC_DEV_NAME, __func__);

	pr_info("%s:%s: uart_en(%d)\n", MUIC_DEV_NAME, __func__, !muic_data->is_rustproof);

	return count;
}

static ssize_t s2mu005_muic_set_usb_en(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t count)
{
	struct s2mu005_muic_data *muic_data = dev_get_drvdata(dev);
	muic_attached_dev_t new_dev = ATTACHED_DEV_USB_MUIC;

	if (!strncasecmp(buf, "1", 1))
		s2mu005_muic_handle_attach(muic_data, new_dev, 0, 0);
	else if (!strncasecmp(buf, "0", 1))
		s2mu005_muic_handle_detach(muic_data);
	else
		pr_info("%s:%s: invalid value\n", MUIC_DEV_NAME, __func__);

	pr_info("%s:%s: attached_dev(%d)\n", MUIC_DEV_NAME, __func__, muic_data->attached_dev);

	return count;
}

static ssize_t s2mu005_muic_show_adc(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	struct s2mu005_muic_data *muic_data = dev_get_drvdata(dev);
	int ret;

	mutex_lock(&muic_data->muic_mutex);
	ret = s2mu005_i2c_read_byte(muic_data->i2c, S2MU005_REG_MUIC_ADC);
	mutex_unlock(&muic_data->muic_mutex);
	if (ret < 0) {
		pr_err("%s:%s: err read adc reg(%d)\n", MUIC_DEV_NAME, __func__, ret);
		return sprintf(buf, "UNKNOWN\n");
	}

	return sprintf(buf, "%x\n", (ret & ADC_MASK));
}

static ssize_t s2mu005_muic_show_usb_state(struct device *dev,
					    struct device_attribute *attr,
					    char *buf)
{
	struct s2mu005_muic_data *muic_data = dev_get_drvdata(dev);
	static unsigned long swtich_slot_time;

	if (printk_timed_ratelimit(&swtich_slot_time, 5000))
		pr_info("%s:%s: muic_data->attached_dev(%d)\n",
			MUIC_DEV_NAME, __func__, muic_data->attached_dev);

	switch (muic_data->attached_dev) {
	case ATTACHED_DEV_USB_MUIC:
	case ATTACHED_DEV_CDP_MUIC:
	case ATTACHED_DEV_JIG_USB_OFF_MUIC:
	case ATTACHED_DEV_JIG_USB_ON_MUIC:
		return sprintf(buf, "USB_STATE_CONFIGURED\n");
	default:
		break;
	}

	return 0;
}

#ifdef DEBUG_MUIC
static ssize_t s2mu005_muic_show_mansw(struct device *dev,
					   struct device_attribute *attr,
					   char *buf)
{
	struct s2mu005_muic_data *muic_data = dev_get_drvdata(dev);
	int ret;

	mutex_lock(&muic_data->muic_mutex);
	ret = s2mu005_i2c_read_byte(muic_data->i2c, S2MU005_REG_MUIC_SW_CTRL);
	mutex_unlock(&muic_data->muic_mutex);

	pr_info("%s:%s: ret:%d buf%s\n", MUIC_DEV_NAME, __func__, ret, buf);

	if (ret < 0) {
		pr_err("%s:%s: fail to read muic reg\n", MUIC_DEV_NAME, __func__);
		return sprintf(buf, "UNKNOWN\n");
	}
	return sprintf(buf, "0x%x\n", ret);
}
static ssize_t s2mu005_muic_show_interrupt_status(struct device *dev,
					   struct device_attribute *attr,
					   char *buf)
{
	struct s2mu005_muic_data *muic_data = dev_get_drvdata(dev);
	int st1, st2;

	mutex_lock(&muic_data->muic_mutex);
	st1 = s2mu005_i2c_read_byte(muic_data->i2c, S2MU005_REG_MUIC_INT1);
	st2 = s2mu005_i2c_read_byte(muic_data->i2c, S2MU005_REG_MUIC_INT2);
	mutex_unlock(&muic_data->muic_mutex);

	pr_info("%s:%s: st1:0x%x st2:0x%x buf%s\n", MUIC_DEV_NAME, __func__, st1, st2, buf);

	if (st1 < 0 || st2 < 0) {
		pr_err("%s:%s: fail to read muic reg\n", MUIC_DEV_NAME, __func__);
		return sprintf(buf, "UNKNOWN\n");
	}
	return sprintf(buf, "st1:0x%x st2:0x%x\n", st1, st2);
}
static ssize_t s2mu005_muic_show_registers(struct device *dev,
					   struct device_attribute *attr,
					   char *buf)
{
	struct s2mu005_muic_data *muic_data = dev_get_drvdata(dev);
	char mesg[256] = "";

	mutex_lock(&muic_data->muic_mutex);
	s2mu005_read_reg_dump(muic_data, mesg);
	mutex_unlock(&muic_data->muic_mutex);
	pr_info("%s:%s: %s\n", MUIC_DEV_NAME, __func__, mesg);

	return sprintf(buf, "%s\n", mesg);
}

#endif

#if defined(CONFIG_USB_HOST_NOTIFY)
static ssize_t s2mu005_muic_show_otg_test(struct device *dev,
					   struct device_attribute *attr,
					   char *buf)
{
	struct s2mu005_muic_data *muic_data = dev_get_drvdata(dev);
	int ret;
	u8 val = 0;

	mutex_lock(&muic_data->muic_mutex);
	ret = s2mu005_i2c_read_byte(muic_data->i2c,
		S2MU005_REG_MUIC_INT2_MASK);
	mutex_unlock(&muic_data->muic_mutex);

	if (ret < 0) {
		pr_err("%s:%s: fail to read muic reg\n", MUIC_DEV_NAME, __func__);
		return sprintf(buf, "UNKNOWN\n");
	}

	pr_info("%s:%s: ret:%d val:%x buf%s\n", MUIC_DEV_NAME, __func__, ret, val, buf);

	val &= INT_VBUS_ON_MASK;
	return sprintf(buf, "%x\n", val);
}

static ssize_t s2mu005_muic_set_otg_test(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct s2mu005_muic_data *muic_data = dev_get_drvdata(dev);

	pr_info("%s:%s: buf:%s\n", MUIC_DEV_NAME, __func__, buf);

	/*
	*	The otg_test is set 0 durring the otg test. Not 1 !!!
	*/

	if (!strncmp(buf, "0", 1)) {
		muic_data->is_otg_test = 1;
#ifdef CONFIG_SEC_FACTORY
		set_otg_reg(muic_data, 1);
#endif
	}
	else if (!strncmp(buf, "1", 1)) {
		muic_data->is_otg_test = 0;
#ifdef CONFIG_SEC_FACTORY
		set_otg_reg(muic_data, 0);
#endif
	}
	else {
		pr_info("%s:%s: Wrong command\n", MUIC_DEV_NAME, __func__);
		return count;
	}

	return count;
}
#endif

static ssize_t s2mu005_muic_show_usb_en(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
		struct s2mu005_muic_data *muic_data = dev_get_drvdata(dev);

		return sprintf(buf, "%s:%s attached_dev = %d\n",
						MUIC_DEV_NAME, __func__, muic_data->attached_dev);
}

#if defined (CONFIG_KEEP_JIG_LOW)
static ssize_t s2mu005_muic_always_keep_jigonb_low_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{

	if (!always_keep_jigonb_low)
		return sprintf(buf, "DISABLE\n");
	else
		return sprintf(buf, "ENABLE\n");
}

static ssize_t s2mu005_muic_always_keep_jigonb_low_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct s2mu005_muic_data *muic_data = dev_get_drvdata(dev);
	int value =0;
	if (!strncasecmp(buf, "ENABLE", 7)) {
		always_keep_jigonb_low = true;
		set_jig_level_low(muic_data, true);
		value =1;
	} else {
		always_keep_jigonb_low = false;
		set_jig_level_low(muic_data, false);
	}
	sec_set_param(param_index_keep_jig_low, &value);
	pr_info("%s:%s : %s %d\n", MUIC_DEV_NAME, __func__, buf,value);
	return count;
}

#endif
static ssize_t s2mu005_muic_jig_disable_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct s2mu005_muic_data *muic_data = dev_get_drvdata(dev);

	if (muic_data->jig_disable)
		return sprintf(buf, "DISABLE\n");
	else
		return sprintf(buf, "ENABLE\n");
}

static ssize_t s2mu005_muic_jig_disable_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct s2mu005_muic_data *muic_data = dev_get_drvdata(dev);

	pr_info("%s:%s : %s\n", MUIC_DEV_NAME, __func__, buf);
	if (!strncasecmp(buf, "DISABLE", 7)) {
		muic_data->jig_disable = true;
		set_jig_sw(muic_data, false);
	} else {
		muic_data->jig_disable = false;
		set_jig_sw(muic_data, true);
	}

	return count;
}

static ssize_t s2mu005_muic_show_attached_dev(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	struct s2mu005_muic_data *muic_data = dev_get_drvdata(dev);

	pr_info("%s:%s: attached_dev[%d]\n", MUIC_DEV_NAME, __func__, muic_data->attached_dev);

	switch (muic_data->attached_dev) {
	case ATTACHED_DEV_NONE_MUIC:
		return sprintf(buf, "No VPS\n");
	case ATTACHED_DEV_USB_MUIC:
		return sprintf(buf, "USB\n");
	case ATTACHED_DEV_CDP_MUIC:
		return sprintf(buf, "CDP\n");
	case ATTACHED_DEV_OTG_MUIC:
		return sprintf(buf, "OTG\n");
	case ATTACHED_DEV_TA_MUIC:
		return sprintf(buf, "TA\n");
	case ATTACHED_DEV_JIG_UART_OFF_MUIC:
		return sprintf(buf, "JIG UART OFF\n");
	case ATTACHED_DEV_JIG_UART_OFF_VB_MUIC:
		return sprintf(buf, "JIG UART OFF/VB\n");
	case ATTACHED_DEV_JIG_UART_ON_MUIC:
		return sprintf(buf, "JIG UART ON\n");
	case ATTACHED_DEV_JIG_USB_OFF_MUIC:
		return sprintf(buf, "JIG USB OFF\n");
	case ATTACHED_DEV_JIG_USB_ON_MUIC:
		return sprintf(buf, "JIG USB ON\n");
	case ATTACHED_DEV_DESKDOCK_MUIC:
	case ATTACHED_DEV_DESKDOCK_VB_MUIC:
		return sprintf(buf, "DESKDOCK\n");
	case ATTACHED_DEV_CHARGING_CABLE_MUIC:
		return sprintf(buf, "PS CABLE\n");
	default:
		break;
	}

	return sprintf(buf, "UNKNOWN\n");
}

static ssize_t s2mu005_muic_show_is_jig_powered(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	struct s2mu005_muic_data *muic_data = dev_get_drvdata(dev);

	pr_info("%s:%s: attached_dev[%d]\n", MUIC_DEV_NAME, __func__, muic_data->attached_dev);

	switch (muic_data->attached_dev) {
	case ATTACHED_DEV_JIG_UART_ON_MUIC:
	case ATTACHED_DEV_JIG_USB_ON_MUIC:
		return sprintf(buf, "1");
	case ATTACHED_DEV_NONE_MUIC:
	case ATTACHED_DEV_USB_MUIC:
	case ATTACHED_DEV_CDP_MUIC:
	case ATTACHED_DEV_OTG_MUIC:
	case ATTACHED_DEV_TA_MUIC:
	case ATTACHED_DEV_DESKDOCK_MUIC:
	case ATTACHED_DEV_DESKDOCK_VB_MUIC:	
	case ATTACHED_DEV_CHARGING_CABLE_MUIC:		
	case ATTACHED_DEV_JIG_UART_OFF_MUIC:
	case ATTACHED_DEV_JIG_UART_OFF_VB_MUIC:	
	case ATTACHED_DEV_JIG_USB_OFF_MUIC:
	default:		
		break;
	}

	return sprintf(buf, "0");
}

static ssize_t s2mu005_muic_show_apo_factory(struct device *dev,
					     struct device_attribute *attr,
					     char *buf)
{
	struct s2mu005_muic_data *muic_data = dev_get_drvdata(dev);
	const char *mode;

	/* true: Factory mode, false: not Factory mode */
	if (muic_data->is_factory_start)
		mode = "FACTORY_MODE";
	else
		mode = "NOT_FACTORY_MODE";

	pr_info("%s:%s: mode status[%s]\n", MUIC_DEV_NAME, __func__, mode);

	return sprintf(buf, "%s\n", mode);
}

static ssize_t s2mu005_muic_set_apo_factory(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct s2mu005_muic_data *muic_data = dev_get_drvdata(dev);
	const char *mode;

	pr_info("%s:%s: buf:%s\n", MUIC_DEV_NAME, __func__, buf);

	/* "FACTORY_START": factory mode */
	if (!strncmp(buf, "FACTORY_START", 13)) {
		muic_data->is_factory_start = true;
		mode = "FACTORY_MODE";
	} else {
		pr_info( "%s:%s: Wrong command\n", MUIC_DEV_NAME, __func__);
		return count;
	}

	return count;
}

#if !defined (CONFIG_SEC_FACTORY) && !defined (CONFIG_MUIC_S2MU005_WATER_WA_DISABLE)
static void s2mu005_muic_set_water_wa(struct s2mu005_muic_data *muic_data, bool en)
{
	struct i2c_client *i2c = muic_data->i2c;

	muic_data->is_water_wa = en;
	pr_info("%s: en : (%d)\n", __func__, (int)en);
	if (en) {
		/* W/A apply */
		s2mu005_i2c_update_bit(i2c,
				S2MU005_REG_MUIC_LDOADC_VSETH, LDOADC_VSET_MASK, 0, LDOADC_VSET_1_2V);
		usleep_range(WATER_TOGGLE_WA_MIN_DURATION_US, WATER_TOGGLE_WA_MAX_DURATION_US);
		s2mu005_i2c_update_bit(i2c,
				S2MU005_REG_MUIC_LDOADC_VSETH, LDOADC_VSET_MASK, 0, LDOADC_VSET_1_5V);
	} else {
		/* W/A unapply */
		s2mu005_i2c_update_bit(i2c,
				S2MU005_REG_MUIC_LDOADC_VSETH, LDOADC_VSET_MASK, 0, LDOADC_VSET_3V);
	}
	return;
}
#endif

static DEVICE_ATTR(uart_en, 0664, s2mu005_muic_show_uart_en,
					s2mu005_muic_set_uart_en);
static DEVICE_ATTR(adc, 0664, s2mu005_muic_show_adc, NULL);
#ifdef DEBUG_MUIC
static DEVICE_ATTR(mansw, 0664, s2mu005_muic_show_mansw, NULL);
static DEVICE_ATTR(dump_registers, 0664, s2mu005_muic_show_registers, NULL);
static DEVICE_ATTR(int_status, 0664, s2mu005_muic_show_interrupt_status, NULL);
#endif
static DEVICE_ATTR(usb_state, 0664, s2mu005_muic_show_usb_state, NULL);
#if defined(CONFIG_USB_HOST_NOTIFY)
static DEVICE_ATTR(otg_test, 0664, s2mu005_muic_show_otg_test,
					s2mu005_muic_set_otg_test);
#endif
static DEVICE_ATTR(attached_dev, 0664, s2mu005_muic_show_attached_dev, NULL);
static DEVICE_ATTR(is_jig_powered, 0664, s2mu005_muic_show_is_jig_powered, NULL);
static DEVICE_ATTR(apo_factory, 0664, s2mu005_muic_show_apo_factory,
					s2mu005_muic_set_apo_factory);
static DEVICE_ATTR(usb_en, 0664, s2mu005_muic_show_usb_en,
					s2mu005_muic_set_usb_en);
static DEVICE_ATTR(jig_disable, 0664, s2mu005_muic_jig_disable_show,
					s2mu005_muic_jig_disable_store);
#if defined (CONFIG_KEEP_JIG_LOW)
static DEVICE_ATTR(keep_jigonb_low, 0664, s2mu005_muic_always_keep_jigonb_low_show,
					s2mu005_muic_always_keep_jigonb_low_store);
#endif

static struct attribute *s2mu005_muic_attributes[] = {
	&dev_attr_uart_en.attr,
	&dev_attr_adc.attr,
#ifdef DEBUG_MUIC
	&dev_attr_mansw.attr,
	&dev_attr_dump_registers.attr,
	&dev_attr_int_status.attr,
#endif
	&dev_attr_usb_state.attr,
#if defined(CONFIG_USB_HOST_NOTIFY)
	&dev_attr_otg_test.attr,
#endif
	&dev_attr_attached_dev.attr,
	&dev_attr_is_jig_powered.attr,
	&dev_attr_apo_factory.attr,
	&dev_attr_usb_en.attr,
	&dev_attr_jig_disable.attr,
#if defined (CONFIG_KEEP_JIG_LOW)
	&dev_attr_keep_jigonb_low.attr,
#endif
	NULL
};

static const struct attribute_group s2mu005_muic_group = {
	.attrs = s2mu005_muic_attributes,
};

static int set_adc_mode_oneshot(struct s2mu005_muic_data *muic_data, bool oneshot)
{
	struct i2c_client *i2c = muic_data->i2c;
	u8 reg_val;
	int ret = 0;

	ret = s2mu005_i2c_read_byte(i2c, S2MU005_REG_MUIC_CTRL3);
	if (ret < 0)
		pr_err("%s:%s: err read CTRL3(%d)\n", MUIC_DEV_NAME, __func__, ret);

	if (oneshot)
		reg_val = ret | (0x1 << CTRL_ONE_SHOT_SHIFT);
	else
		reg_val = ret & ~(0x1 << CTRL_ONE_SHOT_SHIFT);

	if (reg_val ^ ret) {
		pr_info("%s:%s: 0x%x != 0x%x, update\n", MUIC_DEV_NAME, __func__, reg_val, ret);

		ret = s2mu005_i2c_write_byte(i2c, S2MU005_REG_MUIC_CTRL3, reg_val);
		if (ret < 0)
			pr_err("%s:%s: err write(%d)\n", MUIC_DEV_NAME, __func__, ret);
	} else {
		pr_info("%s:%s: 0x%x == 0x%x, just return\n",
			MUIC_DEV_NAME, __func__, reg_val, ret);
		return 0;
	}

	ret = s2mu005_i2c_read_byte(i2c, S2MU005_REG_MUIC_CTRL3);
	if (ret < 0)
		pr_err("%s:%s: err read CTRL(%d)\n", MUIC_DEV_NAME, __func__, ret);
	else
		pr_info("%s:%s: after change(0x%x)\n", MUIC_DEV_NAME, __func__, ret);

	return ret;
}

static int set_ctrl_reg(struct s2mu005_muic_data *muic_data, int shift, bool on)
{
	struct i2c_client *i2c = muic_data->i2c;
	u8 reg_val;
	int ret = 0;

	ret = s2mu005_i2c_read_byte(i2c, S2MU005_REG_MUIC_CTRL1);
	if (ret < 0)
		pr_err("%s:%s: err read CTRL(%d)\n", MUIC_DEV_NAME, __func__, ret);

	if (on)
		reg_val = ret | (0x1 << shift);
	else
		reg_val = ret & ~(0x1 << shift);

	if (reg_val ^ ret) {
		pr_info("%s:%s: 0x%x != 0x%x, update\n", MUIC_DEV_NAME, __func__, reg_val, ret);

		ret = s2mu005_i2c_write_byte(i2c, S2MU005_REG_MUIC_CTRL1, reg_val);
		if (ret < 0)
			pr_err("%s:%s: err write(%d)\n", MUIC_DEV_NAME, __func__, ret);
	} else {
		pr_info("%s:%s: 0x%x == 0x%x, just return\n",
			MUIC_DEV_NAME, __func__, reg_val, ret);
		return 0;
	}

	ret = s2mu005_i2c_read_byte(i2c, S2MU005_REG_MUIC_CTRL1);
	if (ret < 0)
		pr_err("%s:%s: err read CTRL(%d)\n", MUIC_DEV_NAME, __func__, ret);
	else
		pr_info("%s:%s: after change(0x%x)\n", MUIC_DEV_NAME, __func__, ret);

	return ret;
}

#if defined (CONFIG_KEEP_JIG_LOW)
static void set_jig_level_low(struct s2mu005_muic_data *muic_data,bool on){
	bool jig_enable;

	jig_enable = muic_data->jigonb_enable;
	muic_data->jigonb_enable = true;
	set_jig_sw(muic_data, on);
	muic_data->jigonb_enable = jig_enable;

	set_ctrl_reg(muic_data, CTRL_MANUAL_SW_SHIFT, false);
	pr_info("%s:%s: JIG level(0x%x)\n", MUIC_DEV_NAME, __func__,on );
}
#endif

static int set_com_sw(struct s2mu005_muic_data *muic_data,
			enum s2mu005_reg_manual_sw_value reg_val)
{
	struct i2c_client *i2c = muic_data->i2c;
	int ret = 0;
	int temp = 0;
	int res = 0;
	/*  --- MANSW [7:5][4:2][1][0] : DM DP RSVD JIG  --- */
	temp = s2mu005_i2c_read_byte(i2c, S2MU005_REG_MUIC_SW_CTRL);
	if (temp < 0)
		pr_err("%s:%s: err read MANSW(0x%x)\n", MUIC_DEV_NAME, __func__, temp);
	if (muic_data->jigonb_enable && !muic_data->jig_disable)
		reg_val |= (MANUAL_SW_JIG_EN);

	if (reg_val != temp) {
		pr_info("%s:%s: 0x%x != 0x%x, update\n", MUIC_DEV_NAME, __func__,
			reg_val, temp);

		ret = s2mu005_i2c_write_byte(i2c, S2MU005_REG_MUIC_SW_CTRL, reg_val);
		if (ret < 0)
			pr_err("%s:%s: err write MANSW(0x%x)\n", MUIC_DEV_NAME, __func__, reg_val);
	}
	res = s2mu005_i2c_read_byte(i2c, S2MU005_REG_MUIC_SW_CTRL);
	if (res < 0)
		pr_err("%s:%s: err read MANSW(0x%x)\n", MUIC_DEV_NAME, __func__, res);
	return ret;
}

static int com_to_open(struct s2mu005_muic_data *muic_data)
{
	enum s2mu005_reg_manual_sw_value reg_val;
	int ret = 0;

	pr_info("%s:%s\n", MUIC_DEV_NAME, __func__);

	reg_val = MANSW_OPEN;
	ret = set_com_sw(muic_data, reg_val);
	if (ret)
		pr_err("%s:%s: set_com_sw err\n", MUIC_DEV_NAME, __func__);

	return ret;
}

static int com_to_usb(struct s2mu005_muic_data *muic_data)
{
	enum s2mu005_reg_manual_sw_value reg_val;
	int ret = 0;

	pr_info("%s:%s\n", MUIC_DEV_NAME, __func__);
	reg_val = MANSW_USB;
	ret = set_com_sw(muic_data, reg_val);
	if (ret)
		pr_err("%s:%s: set_com_usb err\n", MUIC_DEV_NAME, __func__);

	return ret;
}

#if defined(CONFIG_MUIC_S2MU005_ENABLE_AUTOSW)
static int com_to_open_jigen(struct s2mu005_muic_data *muic_data)
{
	enum s2mu005_reg_manual_sw_value reg_val;
	int ret = 0;
	struct i2c_client *i2c = muic_data->i2c;

	ret = set_ctrl_reg(muic_data, CTRL_MANUAL_SW_SHIFT, false);
	if (ret < 0)
		pr_err( "%s:%s: fail to update reg\n", MUIC_DEV_NAME, __func__);
	
	reg_val = (MANSW_OPEN | MANUAL_SW_JIG_EN);

	ret = s2mu005_i2c_write_byte(i2c, S2MU005_REG_MUIC_SW_CTRL, reg_val);
	if (ret < 0)
		pr_err("%s:%s: err write MANSW\n", MUIC_DEV_NAME, __func__);

	ret = s2mu005_i2c_read_byte(i2c, S2MU005_REG_MUIC_SW_CTRL);
	pr_info("%s:%s: MUIC_SW_CTRL=0x%x\n ", MUIC_DEV_NAME, __func__, ret);

	return ret;
}
#endif /* CONFIG_MUIC_S2MU005_ENABLE_AUTOSW */

static int com_to_uart(struct s2mu005_muic_data *muic_data)
{
	enum s2mu005_reg_manual_sw_value reg_val;
	int ret = 0;

	pr_info("%s:%s: rustproof mode[%d]\n", MUIC_DEV_NAME, __func__, muic_data->is_rustproof);

	if (muic_data->is_rustproof)
	{
#if defined(CONFIG_MUIC_S2MU005_ENABLE_AUTOSW)
		ret = com_to_open_jigen(muic_data);
		if (ret)
			pr_err("%s:%s: manual open set err\n", MUIC_DEV_NAME, __func__);
#endif /* CONFIG_MUIC_S2MU005_ENABLE_AUTOSW */
		return ret;
	}
	reg_val = MANSW_UART;
	ret = set_com_sw(muic_data, reg_val);
	if (ret)
		pr_err("%s:%s: set_com_uart err\n", MUIC_DEV_NAME, __func__);

	return ret;
}

static int switch_to_uart(struct s2mu005_muic_data *muic_data, int uart_status)
{
	int ret = 0;

	pr_info("%s:%s\n", MUIC_DEV_NAME, __func__);

	if (muic_data->pdata->gpio_uart_sel)
		muic_data->pdata->set_gpio_uart_sel(uart_status);

	ret = com_to_uart(muic_data);
	if (ret) {
		pr_err("%s:%s: com->uart set err\n", MUIC_DEV_NAME, __func__);
		return ret;
	}

	return ret;
}

static int attach_usb(struct s2mu005_muic_data *muic_data)
{
	int ret = 0;

	pr_info("%s:%s\n", MUIC_DEV_NAME, __func__);
	ret = com_to_usb(muic_data);

	return ret;
}

#if defined(CONFIG_S2MU005_SUPPORT_BC1P2_CERTI)
static int s2mu005_muic_dp_0_6V(struct s2mu005_muic_data *muic_data, bool en)
{
	struct i2c_client *i2c = muic_data->i2c;
	int val, ret;
	int devt1 = 0, devt2 = 0, devt3 = 0, devt_app = 0;
	int vbvolt = 0;
	u8 vbus_ldo = 0, dp = 0;

	cancel_delayed_work(&muic_data->dp_0p6v);

	pr_info("%s en : %d\n", __func__, (int)en);
	val = s2mu005_i2c_read_byte(i2c, 0x55);

	if (en) {
		val = 0xA0;
		schedule_delayed_work(&muic_data->dp_0p6v, msecs_to_jiffies(40000));
	} else
		val = 0x00;

	ret = s2mu005_i2c_write_byte(i2c, 0x55, (u8)val);

	msleep(50);

	devt1 = s2mu005_i2c_read_byte(i2c, S2MU005_REG_MUIC_DEVICE_TYPE1);
	devt2 = s2mu005_i2c_read_byte(i2c, S2MU005_REG_MUIC_DEVICE_TYPE2);
	devt3 = s2mu005_i2c_read_byte(i2c, S2MU005_REG_MUIC_DEVICE_TYPE3);
	devt_app = s2mu005_i2c_read_byte(i2c, S2MU005_REG_MUIC_DEVICE_APPLE);
	vbvolt = !!(devt_app & DEV_TYPE_APPLE_VBUS_WAKEUP);
	vbus_ldo = s2mu005_i2c_read_byte(i2c, 0x7E);
	dp = s2mu005_i2c_read_byte(i2c, 0x55);

	pr_info("dev[0x%x, 0x%x, 0x%x]\n", devt1, devt2, devt3);
	pr_info("vbvlot(0x%x), vbus_ldo(0x%x), dp(0x%x)\n", vbvolt, vbus_ldo, dp);

	return ret;
}

static void s2mu005_muic_dp_0p6v(struct work_struct *work)
{
	struct s2mu005_muic_data *muic_data =
		container_of(work, struct s2mu005_muic_data, dp_0p6v.work);

	s2mu005_muic_dp_0_6V(muic_data, false);
}

static void s2mu005_muic_bcd_rescan(struct s2mu005_muic_data *muic_data)
{
	struct i2c_client *i2c = muic_data->i2c;
	int ret = 0;
	u8 reg_val = 0;

	pr_info("%s\n", __func__);

	ret = set_com_sw(muic_data, MANSW_OPEN);
	if (ret < 0)
		pr_err("%s, fail to open mansw\n", __func__);

	msleep(50);

	reg_val = s2mu005_i2c_read_byte(i2c, S2MU005_REG_MUIC_BCD_RESCAN);
	reg_val |= 0x1;
	s2mu005_i2c_write_byte(i2c, S2MU005_REG_MUIC_BCD_RESCAN, reg_val);
}

static void s2mu005_muic_cable_recheck(struct work_struct *work)
{
	struct s2mu005_muic_data *muic_data =
		container_of(work, struct s2mu005_muic_data, cable_recheck.work);
	struct i2c_client *i2c = muic_data->i2c;
	u8 dp = 0;

	mutex_lock(&muic_data->recheck_mutex);

	pr_info("%s\n", __func__);

	dp = s2mu005_i2c_read_byte(i2c, 0x55);
	if (dp)
		s2mu005_muic_dp_0_6V(muic_data, false);

	s2mu005_muic_bcd_rescan(muic_data);
	msleep(190);

	s2mu005_muic_detect_dev(muic_data);

	mutex_unlock(&muic_data->recheck_mutex);
}

static void s2mu005_muic_handle_vbus_off(struct s2mu005_muic_data *muic_data)
{
	struct i2c_client *i2c = muic_data->i2c;
	int devt1 = 0, devt2 = 0, devt3 = 0, devt_app = 0;
	union power_supply_propval pval = {0, };
	int vbvolt = 0;

	devt1 = s2mu005_i2c_read_byte(i2c, S2MU005_REG_MUIC_DEVICE_TYPE1);
	devt2 = s2mu005_i2c_read_byte(i2c, S2MU005_REG_MUIC_DEVICE_TYPE2);
	devt3 = s2mu005_i2c_read_byte(i2c, S2MU005_REG_MUIC_DEVICE_TYPE3);
	devt_app = s2mu005_i2c_read_byte(i2c, S2MU005_REG_MUIC_DEVICE_APPLE);
	vbvolt = !!(devt_app & DEV_TYPE_APPLE_VBUS_WAKEUP);
	if (!devt1 && !devt2 && !devt3 && !vbvolt
		&& !(devt_app & DEV_TYPE_APPLE_APPLE_CHG)) {
		pval.intval = SEC_BAT_CHG_MODE_CHARGING_OFF;
		psy_do_property("s2mu005-charger", set,
			POWER_SUPPLY_PROP_CHARGING_ENABLED, pval);
	}
}
#endif

static int attach_jig_uart_boot_on_off(struct s2mu005_muic_data *muic_data)
{
	struct muic_platform_data *pdata = muic_data->pdata;
	int ret = 0;

	pr_info("%s:%s\n", MUIC_DEV_NAME, __func__);

	if (pdata->uart_path == MUIC_PATH_UART_AP)
		ret = switch_to_uart(muic_data, MUIC_PATH_UART_AP);
	else
		ret = switch_to_uart(muic_data, MUIC_PATH_UART_CP);

	return ret;
}


static int attach_jig_usb_boot_on_off(struct s2mu005_muic_data *muic_data)
{
	int ret = 0;

	pr_info("%s:%s\n", MUIC_DEV_NAME, __func__);

	ret = attach_usb(muic_data);
	if (ret)
		return ret;

	return ret;
}

#ifdef CONFIG_MUIC_S2MU005_INNER_BATTERY
/* Power-off SW work-around by disconnecting JIG cable for EVT4
 * in case of In-battery model
 */
static void s2mu005_muic_power_off(struct s2mu005_muic_data *muic_data, int on)
{
	struct i2c_client *i2c = muic_data->i2c;
	int ret = 0;

	if (muic_data->muic_version >= 4) {
		if (on) {
			/* 0x27[0]=0 */
			ret = s2mu005_i2c_read_byte(i2c, 0x27);
			ret |= 0x30;
			s2mu005_i2c_write_byte(i2c, 0x27, (u8)ret);
		} else {
			/* 0x27[0]=1 : default */
			ret = s2mu005_i2c_read_byte(i2c, 0x27);
			ret &= ~0x30;
			ret |= 0x10;
			s2mu005_i2c_write_byte(i2c, 0x27, (u8)ret);
		}
	}
	pr_info("%s:%s: 0x27:0x%x, rev_id(%d)\n", MUIC_DEV_NAME,
			__func__, ret, muic_data->muic_version);
}
#endif

static void s2mu005_muic_handle_attach(struct s2mu005_muic_data *muic_data,
			muic_attached_dev_t new_dev, int adc, u8 vbvolt)
{
	struct muic_platform_data *pdata = muic_data->pdata;
	int ret = 0;
	bool noti = (new_dev != muic_data->attached_dev) ? true : false;

	pr_info("%s:%s: muic_data->attached_dev: %d, new_dev: %d, muic_data->suspended: %d\n",
		MUIC_DEV_NAME, __func__, muic_data->attached_dev, new_dev, muic_data->suspended);

	if (new_dev == muic_data->attached_dev) {
		pr_info("%s:%s: Attach duplicated\n", MUIC_DEV_NAME, __func__);
		return;
	}

	/* Logically Detach Accessary */
	switch (muic_data->attached_dev) {
	case ATTACHED_DEV_USB_MUIC:
	case ATTACHED_DEV_CDP_MUIC:
	case ATTACHED_DEV_JIG_USB_OFF_MUIC:
	case ATTACHED_DEV_JIG_USB_ON_MUIC:
	case ATTACHED_DEV_CHARGING_CABLE_MUIC:
	case ATTACHED_DEV_VZW_ACC_MUIC:
	case ATTACHED_DEV_VZW_INCOMPATIBLE_MUIC:
	case ATTACHED_DEV_TA_MUIC:
	case ATTACHED_DEV_UNDEFINED_CHARGING_MUIC:
	case ATTACHED_DEV_TIMEOUT_OPEN_MUIC:
	case ATTACHED_DEV_JIG_UART_OFF_VB_MUIC:
	case ATTACHED_DEV_JIG_UART_OFF_MUIC:
	case ATTACHED_DEV_JIG_UART_ON_MUIC:
#if defined(CONFIG_SEC_FACTORY) && defined(CONFIG_MUIC_S2MU005_DISCHARGING_WA)		
	case ATTACHED_DEV_CARKIT_MUIC:
#endif
#ifdef CONFIG_MUIC_S2MU005_INNER_BATTERY
		s2mu005_muic_power_off(muic_data, 0);
#endif
		s2mu005_muic_handle_detach(muic_data);
                break;
	case ATTACHED_DEV_OTG_MUIC:
		if(new_dev == ATTACHED_DEV_USB_LANHUB_MUIC)
			break;
#ifdef CONFIG_MUIC_S2MU005_INNER_BATTERY
		s2mu005_muic_power_off(muic_data, 0);
#endif
		s2mu005_muic_handle_detach(muic_data);
		break;
	case ATTACHED_DEV_DESKDOCK_MUIC:
	case ATTACHED_DEV_DESKDOCK_VB_MUIC:
		switch (new_dev) {
		case ATTACHED_DEV_DESKDOCK_MUIC:
		case ATTACHED_DEV_DESKDOCK_VB_MUIC:
			break;
		default:
#ifdef CONFIG_MUIC_S2MU005_INNER_BATTERY
		s2mu005_muic_power_off(muic_data, 0);
#endif
			s2mu005_muic_handle_detach(muic_data);
			break;
		}
		break;
	default:
		break;
	}
	pr_info("%s:%s: new(%d)!=attached(%d)\n", MUIC_DEV_NAME, __func__,
							new_dev, muic_data->attached_dev);
	muic_data->jigonb_enable = false;

	/* Attach Accessary */
	noti = true;
	switch (new_dev) {
	case ATTACHED_DEV_OTG_MUIC:
	case ATTACHED_DEV_USB_LANHUB_MUIC:
		set_ctrl_reg(muic_data, CTRL_MANUAL_SW_SHIFT, false);
		set_adc_mode_oneshot(muic_data, false);
		ret = attach_usb(muic_data);
		break;
	case ATTACHED_DEV_USB_MUIC:
	case ATTACHED_DEV_CDP_MUIC:
#if defined(CONFIG_S2MU005_SUPPORT_BC1P2_CERTI)
		/*
		* USB BC 1.2 certification :
		* 100ms delay between attach dev,
		* and path setting / start chg.
		*/
		msleep(100);
#endif
		ret = attach_usb(muic_data);
		break;
	case ATTACHED_DEV_CHARGING_CABLE_MUIC:
	case ATTACHED_DEV_TA_MUIC:
#if defined(CONFIG_S2MU005_SUPPORT_BC1P2_CERTI)
		/*
		* USB BC 1.2 certification :
		* 100ms delay between attach dev,
		* and path setting / start chg.
		* DP 0.6V when DCP is detected.
		*/
		msleep(100);
		if (muic_data->is_dcp)
			s2mu005_muic_dp_0_6V(muic_data, true);
#endif
	case ATTACHED_DEV_UNDEFINED_CHARGING_MUIC:
	case ATTACHED_DEV_TIMEOUT_OPEN_MUIC:
	case ATTACHED_DEV_UNKNOWN_MUIC:
		com_to_open(muic_data);
		break;
	case ATTACHED_DEV_JIG_UART_OFF_VB_MUIC:
	case ATTACHED_DEV_JIG_UART_OFF_MUIC:
#ifdef CONFIG_MUIC_S2MU005_INNER_BATTERY 
		s2mu005_muic_power_off(muic_data, 1); 
#endif 
		if (pdata->is_new_factory)
#if !IS_ENABLED(CONFIG_SEC_FACTORY)
			muic_data->jigonb_enable = false;
#else
			muic_data->jigonb_enable = true;
#endif	
		else
			muic_data->jigonb_enable = true;
		ret = attach_jig_uart_boot_on_off(muic_data);
		break;
	case ATTACHED_DEV_JIG_UART_ON_MUIC:
		if (pdata->is_new_factory)
			muic_data->jigonb_enable = false;
		else
			muic_data->jigonb_enable = true;
		ret = attach_jig_uart_boot_on_off(muic_data);
		break;
	case ATTACHED_DEV_JIG_USB_OFF_MUIC:
#ifdef CONFIG_MUIC_S2MU005_INNER_BATTERY 
		s2mu005_muic_power_off(muic_data, 1); 
#endif 
#if defined(CONFIG_SEC_FACTORY) && defined(CONFIG_MUIC_S2MU005_DISCHARGING_WA)		
	case ATTACHED_DEV_CARKIT_MUIC:
#endif
	case ATTACHED_DEV_JIG_USB_ON_MUIC:
		if (pdata->is_new_factory)
			muic_data->jigonb_enable = false;
		else
			muic_data->jigonb_enable = true;
		ret = attach_jig_usb_boot_on_off(muic_data);
		break;
	case ATTACHED_DEV_DESKDOCK_MUIC:
	case ATTACHED_DEV_DESKDOCK_VB_MUIC:
	case ATTACHED_DEV_VZW_INCOMPATIBLE_MUIC:
	case ATTACHED_DEV_VZW_ACC_MUIC:
		break;
	default:
		noti = false;
		pr_info("%s:%s: unsupported dev=%d, adc=0x%x, vbus=%c\n",
				MUIC_DEV_NAME, __func__, new_dev, adc, (vbvolt ? 'O' : 'X'));
		break;
	}

	if (ret)
		pr_err("%s:%s: something wrong %d (ERR=%d)\n", MUIC_DEV_NAME, __func__, new_dev, ret);

#if defined(CONFIG_MUIC_NOTIFIER)
	if (noti) {
		if (!muic_data->suspended)
			muic_notifier_attach_attached_dev(new_dev);
		else
			muic_data->need_to_noti = true;
	}
#endif /* CONFIG_MUIC_NOTIFIER */

	muic_data->attached_dev = new_dev;
}

static void s2mu005_muic_handle_detach(struct s2mu005_muic_data *muic_data)
{
	int ret = 0;
	bool noti = true;

	muic_data->jigonb_enable = false;
#if defined(CONFIG_S2MU005_SUPPORT_BC1P2_CERTI)
	muic_data->usb_type_rechecked = false;
#endif	
	if (muic_data->attached_dev == ATTACHED_DEV_NONE_MUIC) {
		pr_info("%s:%s: Detach duplicated(NONE)\n", MUIC_DEV_NAME, __func__);
		goto out_without_noti;
	}

	pr_info("%s:%s: Detach device[%d]\n", MUIC_DEV_NAME, __func__, muic_data->attached_dev);
#if defined(CONFIG_S2MU005_SUPPORT_BC1P2_CERTI)
	if (muic_data->is_dcp) {
		s2mu005_muic_dp_0_6V(muic_data, false);
		muic_data->is_dcp = false;
	}
#endif
#if defined(CONFIG_MUIC_NOTIFIER)
	if (noti) {
		if (!muic_data->suspended)
			muic_notifier_detach_attached_dev(muic_data->attached_dev);
		else
			muic_data->need_to_noti = true;
	}
#endif /* CONFIG_MUIC_NOTIFIER */

out_without_noti:
	if (muic_data->attached_dev == ATTACHED_DEV_OTG_MUIC ||
			muic_data->attached_dev == ATTACHED_DEV_USB_LANHUB_MUIC) {
#ifdef CONFIG_MUIC_S2MU005_ENABLE_AUTOSW
		set_ctrl_reg(muic_data, CTRL_MANUAL_SW_SHIFT, true);
#endif
		set_adc_mode_oneshot(muic_data, true);
	}
	ret = com_to_open(muic_data);
	muic_data->attached_dev = ATTACHED_DEV_NONE_MUIC;
}

static void update_jig_state(struct s2mu005_muic_data *muic_data)
{
	struct muic_platform_data *pdata = muic_data->pdata;
	int jig_state;

	switch (muic_data->attached_dev) {
	case ATTACHED_DEV_JIG_UART_OFF_MUIC:
	case ATTACHED_DEV_JIG_UART_OFF_VB_MUIC:     /* VBUS enabled */
	case ATTACHED_DEV_JIG_UART_OFF_VB_OTG_MUIC:    /* for otg test */
	case ATTACHED_DEV_JIG_UART_OFF_VB_FG_MUIC:    /* for fg test */
	case ATTACHED_DEV_JIG_UART_ON_MUIC:
	case ATTACHED_DEV_JIG_UART_ON_VB_MUIC:       /* VBUS enabled */
	case ATTACHED_DEV_JIG_USB_OFF_MUIC:
	case ATTACHED_DEV_JIG_USB_ON_MUIC:
		jig_state = true;
		break;
	default:
		jig_state = false;
		break;
	}
	pr_info("%s jig_state : %d\n", __func__, jig_state);

	pdata->jig_uart_cb(jig_state);
}

static void s2mu005_muic_detect_dev(struct s2mu005_muic_data *muic_data)
{
	struct i2c_client *i2c = muic_data->i2c;
	muic_attached_dev_t new_dev = ATTACHED_DEV_UNKNOWN_MUIC;
	int vbvolt = 0, vmid = 0;
	int val1 = 0, val2 = 0, val3 = 0, val4 = 0, adc = 0;
	int val5 = 0, val6 = 0, val7 = 0;

	val1 = s2mu005_i2c_read_byte(i2c, S2MU005_REG_MUIC_DEVICE_TYPE1);
	val2 = s2mu005_i2c_read_byte(i2c, S2MU005_REG_MUIC_DEVICE_TYPE2);
	val3 = s2mu005_i2c_read_byte(i2c, S2MU005_REG_MUIC_DEVICE_TYPE3);
	val4 = s2mu005_i2c_read_byte(i2c, S2MU005_REG_REV_ID);
	adc = s2mu005_i2c_read_byte(i2c, S2MU005_REG_MUIC_ADC);
	val5 = s2mu005_i2c_read_byte(i2c, S2MU005_REG_MUIC_DEVICE_APPLE);
	val6 = s2mu005_i2c_read_byte(i2c, S2MU005_REG_MUIC_CHG_TYPE);
	val7 = s2mu005_i2c_read_byte(i2c, S2MU005_REG_SC_STATUS2);

	vbvolt = !!(val5 & DEV_TYPE_APPLE_VBUS_WAKEUP);
	vmid = (val7 & 0x7);

	pr_info("%s:%s: dev[1:0x%x, 2:0x%x, 3:0x%x]\n", MUIC_DEV_NAME, __func__, val1, val2, val3);
	pr_info("%s:%s: adc:0x%x, vbvolt:0x%x, apple:0x%x, chg_type:0x%x, vmid:0x%x, dev_id:0x%x\n",
				MUIC_DEV_NAME, __func__, adc, vbvolt, val5, val6, vmid, val4);

	if (ADC_CONVERSION_MASK & adc) {
		pr_err("%s:%s: ADC conversion error!\n", MUIC_DEV_NAME, __func__);
		return ;
	}

	/* Work-Around for EVT0 : only if VBUS connected, then apply USB */
	if(muic_data->muic_version == 0) {
		if (vbvolt) {
			new_dev = ATTACHED_DEV_USB_MUIC;
			pr_info("%s:%s: EVT0 Work-around USB DETECTED\n", MUIC_DEV_NAME, __func__);
		}
	}

	/* Detected */
	switch (val1) {
	case DEV_TYPE1_CDP:

		if (vbvolt) {
			new_dev = ATTACHED_DEV_CDP_MUIC;
			pr_info("%s:%s: USB_CDP DETECTED\n", MUIC_DEV_NAME, __func__);
		}
	
		break;
	case DEV_TYPE1_USB:
#if defined(CONFIG_S2MU005_SUPPORT_BC1P2_CERTI)
		if (vbvolt) {
		   if (muic_data->usb_type_rechecked) {
			   muic_data->usb_type_rechecked = false;
			   new_dev = ATTACHED_DEV_USB_MUIC;
		   } else {
			   muic_data->usb_type_rechecked = true;
			   schedule_delayed_work(&muic_data->cable_recheck, 0);
			   return;
		   }
		}
#else
		if (vbvolt) {
			new_dev = ATTACHED_DEV_USB_MUIC;
			pr_info("%s:%s: USB DETECTED\n", MUIC_DEV_NAME, __func__);
		}
#endif
		break;
	case DEV_TYPE1_DEDICATED_CHG:
	case DEV_TYPE1_DEDICATED_CHG2:
	case DEV_TYPE1_CHG_TYPES:
	if (vbvolt) {
#if defined(CONFIG_S2MU005_SUPPORT_BC1P2_CERTI)
			muic_data->is_dcp = true;
#endif
		new_dev = ATTACHED_DEV_TA_MUIC;
		pr_info("%s:%s:DEDICATED CHARGER DETECTED\n", MUIC_DEV_NAME, __func__);
		}
		break;
	case DEV_TYPE1_USB_OTG:
		new_dev = ATTACHED_DEV_OTG_MUIC;
		pr_info("%s:%s: USB_OTG DETECTED\n", MUIC_DEV_NAME, __func__);
		if (vmid == 0x4) {
			pr_info("%s:%s: VMID DETECTED[%d]\n", MUIC_DEV_NAME, __func__, vmid);
			vbvolt = 1;
		}
		break;
	case DEV_TYPE1_T1_T2_CHG:
	if (vbvolt) {
		/* 200K, 442K should be checkef */
#if defined(CONFIG_SEC_FACTORY) && defined(CONFIG_MUIC_S2MU005_DISCHARGING_WA)
		new_dev = ATTACHED_DEV_CARKIT_MUIC;
		pr_info("%s:%s:CARKIT DETECTED\n", MUIC_DEV_NAME, __func__);
#else
		if (ADC_CEA936ATYPE2_CHG == adc)
			new_dev = ATTACHED_DEV_TA_MUIC;
		else
			new_dev = ATTACHED_DEV_USB_MUIC;
		pr_info("%s:%s: T1_T2 CHARGER DETECTED\n", MUIC_DEV_NAME, __func__);
#endif
	}
	else {
		/* W/A, 442k without VB changes to 523K (JIG_UART_OFF)
		To prevent to keep sleep mode*/
		new_dev = ATTACHED_DEV_JIG_UART_OFF_MUIC;
		pr_info("%s:%s: 442K->523K JIG_USB_OFF DETECTED\n", MUIC_DEV_NAME, __func__);
	}
		break;
	case DEV_TYPE1_AUDIO_2:
		new_dev = ATTACHED_DEV_USB_LANHUB_MUIC;
		pr_info("%s:%s:LANHUB DETECTED DETECTED\n", MUIC_DEV_NAME, __func__);
		break;	
	default:
		break;
	}

	switch (val2) {

	case DEV_TYPE2_SDP_1P8S:
		if(vbvolt && adc == ADC_OPEN) {
			struct muic_platform_data *pdata = muic_data->pdata;

			if (pdata->dcd_timeout)
				new_dev = ATTACHED_DEV_TIMEOUT_OPEN_MUIC;
			else
				new_dev = ATTACHED_DEV_USB_MUIC;
			pr_info("%s:%s: SDP_1P8S DETECTED\n", MUIC_DEV_NAME, __func__);
		}
	
		break; 
	case DEV_TYPE2_JIG_UART_OFF:
		if (muic_data->is_otg_test) {
			mdelay(1000);
			val7 = s2mu005_i2c_read_byte(i2c, S2MU005_REG_SC_STATUS2);
			vmid = val7 & 0x7;
			pr_info("%s:%s: vmid : 0x%x \n", MUIC_DEV_NAME, __func__, vmid);
			if (vmid == 0x4) {
				pr_info("%s:%s: OTG_TEST DETECTED, vmid = %d\n",
							MUIC_DEV_NAME, __func__, vmid);
				vbvolt = 1;
			}
			new_dev = ATTACHED_DEV_JIG_UART_OFF_MUIC;
		}
		else if (vbvolt) {
			new_dev = ATTACHED_DEV_JIG_UART_OFF_VB_MUIC;
			pr_info("%s:%s: JIG_UART_OFF_VB DETECTED\n", MUIC_DEV_NAME, __func__);
		}
		else {
			new_dev = ATTACHED_DEV_JIG_UART_OFF_MUIC;
			pr_info("%s:%s: JIG_UART_OFF DETECTED\n", MUIC_DEV_NAME, __func__);
		}
		break;
	case DEV_TYPE2_JIG_UART_ON:
		if (new_dev != ATTACHED_DEV_JIG_UART_ON_MUIC) {
			new_dev = ATTACHED_DEV_JIG_UART_ON_MUIC;
			pr_info("%s:%s: JIG_UART_ON DETECTED\n", MUIC_DEV_NAME, __func__);
		}
		break;
	case DEV_TYPE2_JIG_USB_OFF:
		if (!vbvolt)
			break;
		new_dev = ATTACHED_DEV_JIG_USB_OFF_MUIC;
		pr_info("%s:%s: JIG_USB_OFF DETECTED\n", MUIC_DEV_NAME, __func__);
		break;
	case DEV_TYPE2_JIG_USB_ON:
		if (!vbvolt)
			break;
		new_dev = ATTACHED_DEV_JIG_USB_ON_MUIC;
		pr_info("%s:%s: JIG_USB_ON DETECTED\n", MUIC_DEV_NAME, __func__);
		break;
	default:
		break;
	}

	if(muic_data->muic_version > 0) {	// Start For EVT0

	/* This is for Apple cables */
	if (vbvolt && ((val5 & DEV_TYPE_APPLE_APPLE2P4A_CHG) || (val5 & DEV_TYPE_APPLE_APPLE2A_CHG) ||
		(val5 & DEV_TYPE_APPLE_APPLE1A_CHG) || (val5 & DEV_TYPE_APPLE_APPLE0P5A_CHG))) {
		new_dev = ATTACHED_DEV_TA_MUIC;
		pr_info("%s:%s: APPLE_CHG DETECTED\n", MUIC_DEV_NAME, __func__);
	}

	if ((val6 & DEV_TYPE_CHG_TYPE) &&
		(new_dev == ATTACHED_DEV_UNKNOWN_MUIC)) {
		/* This is workaround for LG USB cable which has 219k ohm ID */
		if (adc == ADC_CEA936ATYPE1_CHG || adc == ADC_JIG_USB_OFF) {
#if defined(CONFIG_SEC_FACTORY) && defined(CONFIG_MUIC_S2MU005_DISCHARGING_WA)
			new_dev = ATTACHED_DEV_CARKIT_MUIC;
			pr_err("[muic] CARKIT DETECTED\n");
#else
			new_dev = ATTACHED_DEV_USB_MUIC;
			pr_info("%s:%s: TYPE1_CHARGER DETECTED (USB)\n", MUIC_DEV_NAME, __func__);
#endif
		}
		else {
			new_dev = ATTACHED_DEV_TA_MUIC;
			pr_info("%s:%s: TYPE3_CHARGER DETECTED\n", MUIC_DEV_NAME, __func__);
		}
	}

	if (val2 & DEV_TYPE2_AV || val3 & DEV_TYPE3_AV_WITH_VBUS) {
		if (vbvolt) {
			new_dev = ATTACHED_DEV_DESKDOCK_VB_MUIC;
			pr_info("%s:%s: DESKDOCK+TA DETECTED\n", MUIC_DEV_NAME, __func__);
		}
		else {
			new_dev = ATTACHED_DEV_DESKDOCK_MUIC;
			pr_info("%s:%s: DESKDOCK DETECTED\n", MUIC_DEV_NAME, __func__);
		}

		/* If not support DESKDOCK */
//		new_dev = ATTACHED_DEV_UNKNOWN_MUIC;
	}

	/* If there is no matching device found using device type registers
		use ADC to find the attached device */
	if (new_dev == ATTACHED_DEV_UNKNOWN_MUIC) {
		switch (adc) {
#if IS_ENABLED(CONFIG_MUIC_VZW_ACC)
		case ADC_RESERVED_VZW:
			new_dev = ATTACHED_DEV_VZW_ACC_MUIC;
			pr_info("%s:%s: ADC VZW ACC DETECTED\n", MUIC_DEV_NAME, __func__);
			break;
#endif
#if IS_ENABLED(CONFIG_MUIC_INCOMPATIBLE_VZW)
		case ADC_INCOMPATIBLE_VZW:
			new_dev = ATTACHED_DEV_VZW_INCOMPATIBLE_MUIC;
			pr_info("%s:%s: ADC INCOMPATIBLE VZW DETECTED(USB)\n", MUIC_DEV_NAME, __func__);
			break;
#endif
		case ADC_CHARGING_CABLE:
//			new_dev = ATTACHED_DEV_CHARGING_CABLE_MUIC;
//			pr_info("%s:%s: ADC PS CABLE DETECTED\n", MUIC_DEV_NAME, __func__);
			break;
		case ADC_CEA936ATYPE1_CHG: /*200k ohm */
			/* This is workaround for LG USB cable
					which has 219k ohm ID */
			if (vbvolt) {
#if defined(CONFIG_SEC_FACTORY) && defined(CONFIG_MUIC_S2MU005_DISCHARGING_WA)
				new_dev = ATTACHED_DEV_CARKIT_MUIC;
				pr_info("%s:%s: CARKIT DETECTED\n", MUIC_DEV_NAME, __func__);
#else
				new_dev = ATTACHED_DEV_USB_MUIC;
				pr_info("%s:%s: ADC TYPE1 CHARGER DETECTED(USB)\n", MUIC_DEV_NAME, __func__);
#endif
			}
			break;
		case ADC_CEA936ATYPE2_CHG:
			if (vbvolt) {
				new_dev = ATTACHED_DEV_TA_MUIC;
				pr_info("%s:%s: ADC TYPE2 CHARGER DETECTED(TA)\n", MUIC_DEV_NAME, __func__);
			}
			break;
		case ADC_JIG_USB_OFF: /* 255k */
			if (!vbvolt)
				break;
			if (new_dev != ATTACHED_DEV_JIG_USB_OFF_MUIC) {
				new_dev = ATTACHED_DEV_JIG_USB_OFF_MUIC;
				pr_info("%s:%s: ADC JIG_USB_OFF DETECTED\n", MUIC_DEV_NAME, __func__);
			}
			break;
		case ADC_JIG_USB_ON:
			if (!vbvolt)
				break;
			if (new_dev != ATTACHED_DEV_JIG_USB_ON_MUIC) {
				new_dev = ATTACHED_DEV_JIG_USB_ON_MUIC;
				pr_info("%s:%s: ADC JIG_USB_ON DETECTED\n", MUIC_DEV_NAME, __func__);
			}
			break;
		case ADC_JIG_UART_OFF:
			if (muic_data->is_otg_test) {
				mdelay(1000);
				val7 = s2mu005_i2c_read_byte(i2c, S2MU005_REG_SC_STATUS2);
				vmid = val7 & 0x7;
				pr_info("%s:%s: vmid : 0x%x \n", MUIC_DEV_NAME, __func__, vmid);
				if (vmid == 0x4) {
					pr_info("%s:%s: ADC OTG_TEST DETECTED, vmid = %d\n",
							MUIC_DEV_NAME, __func__, vmid);
					vbvolt = 1;
				}
				new_dev = ATTACHED_DEV_JIG_UART_OFF_MUIC;
			}
			else if (vbvolt) {
				new_dev = ATTACHED_DEV_JIG_UART_OFF_VB_MUIC;
				pr_info("%s:%s: ADC JIG_UART_OFF_VB DETECTED\n",
							MUIC_DEV_NAME, __func__);
			}
			else {
				new_dev = ATTACHED_DEV_JIG_UART_OFF_MUIC;
				pr_info("%s:%s: ADC JIG_UART_OFF DETECTED\n",
							MUIC_DEV_NAME, __func__);
			}
			break;
		case ADC_JIG_UART_ON:
			if (new_dev != ATTACHED_DEV_JIG_UART_ON_MUIC) {
				new_dev = ATTACHED_DEV_JIG_UART_ON_MUIC;
				pr_info("%s:%s: ADC JIG_UART_ON DETECTED\n",
							MUIC_DEV_NAME, __func__);
			}
			break;
		case ADC_DESKDOCK:
			if (vbvolt) {
				new_dev = ATTACHED_DEV_DESKDOCK_VB_MUIC;
				pr_info("%s:%s: ADC DESKDOCK+TA DETECTED\n",
							MUIC_DEV_NAME, __func__);
			}
			else {
				new_dev = ATTACHED_DEV_DESKDOCK_MUIC;
				pr_info("%s:%s: ADC DESKDOCK DETECTED\n",
							MUIC_DEV_NAME, __func__);
			}
			break;
		case ADC_USB_LANHUB:
			new_dev = ATTACHED_DEV_USB_LANHUB_MUIC;
			pr_info("%s:%s: ADC LANHUB DETECTED\n", MUIC_DEV_NAME, __func__);
			break;
		case ADC_OPEN:
			break;
		default:
			pr_info("%s:%s: unsupported ADC(0x%02x)\n", MUIC_DEV_NAME, __func__, adc);
			break;
		}
	}

	}	// End For EVT0

	if ((ATTACHED_DEV_UNKNOWN_MUIC == new_dev) && (ADC_OPEN != adc)) {
		if (vbvolt) {
			new_dev = ATTACHED_DEV_UNDEFINED_CHARGING_MUIC;
			pr_info("%s:%s: UNDEFINED VB DETECTED\n", MUIC_DEV_NAME, __func__);
		}
	}

	if (new_dev != ATTACHED_DEV_UNKNOWN_MUIC) {
		pr_info("%s:%s ATTACHED\n", MUIC_DEV_NAME, __func__);
		s2mu005_muic_handle_attach(muic_data, new_dev, adc, vbvolt);
	}
	else {
		pr_info("%s:%s DETACHED\n", MUIC_DEV_NAME, __func__);
		s2mu005_muic_handle_detach(muic_data);
	}

	if (muic_data->pdata->jig_uart_cb)
		update_jig_state(muic_data);

#if defined(CONFIG_VBUS_NOTIFIER)
	vbus_notifier_handle((!!vbvolt) ? STATUS_VBUS_HIGH : STATUS_VBUS_LOW);
#endif /* CONFIG_VBUS_NOTIFIER */

}

static int s2mu005_muic_reg_init(struct s2mu005_muic_data *muic_data)
{
	struct i2c_client *i2c = muic_data->i2c;
	int ret;
	int val1, val2, val3, val4, adc, ctrl1;
#if defined(CONFIG_S2MU005_SUPPORT_BC1P2_CERTI)
	u8 reg_val = 0, tmp_val = 0, vldo_set = 0;
#endif

	pr_info("%s:%s\n", MUIC_DEV_NAME, __func__);

	val1 = s2mu005_i2c_read_byte(i2c, S2MU005_REG_MUIC_DEVICE_TYPE1);
	val2 = s2mu005_i2c_read_byte(i2c, S2MU005_REG_MUIC_DEVICE_TYPE2);
	val3 = s2mu005_i2c_read_byte(i2c, S2MU005_REG_MUIC_DEVICE_TYPE3);
	val4 = s2mu005_i2c_read_byte(i2c, S2MU005_REG_MUIC_SW_CTRL);
	val4 &= 0x03;
	adc = s2mu005_i2c_read_byte(i2c, S2MU005_REG_MUIC_ADC);
	pr_info("%s:%s: dev[1:0x%x, 2:0x%x, 3:0x%x], adc:0x%x\n", MUIC_DEV_NAME, __func__,
									val1, val2, val3, adc);

	if ((val1 & DEV_TYPE1_USB_TYPES) ||
				(val2 & DEV_TYPE2_JIG_USB_TYPES)) {
			ret = s2mu005_i2c_write_byte(i2c, S2MU005_REG_MUIC_SW_CTRL, val4 | MANUAL_SW_USB);
			if (ret < 0)
				pr_err("%s:%s usb type detect err %d\n",
					   MUIC_DEV_NAME, __func__, ret);
	} else if (val2 & DEV_TYPE2_JIG_UART_TYPES) {
			ret = s2mu005_i2c_write_byte(i2c, S2MU005_REG_MUIC_SW_CTRL, val4 | MANUAL_SW_UART);
			if (ret < 0)
				pr_err("%s:%s uart type detect err %d\n",
						MUIC_DEV_NAME, __func__, ret);
	}

	ret = s2mu005_i2c_write_byte(i2c,
			S2MU005_REG_MUIC_CTRL1, CTRL_MASK);

	if (ret < 0)
		pr_err( "%s:%s: failed to write ctrl(%d)\n", MUIC_DEV_NAME, __func__, ret);

	ctrl1 = s2mu005_i2c_read_byte(i2c, S2MU005_REG_MUIC_CTRL1);
	pr_info("%s:%s: CTRL1:0x%02x\n", MUIC_DEV_NAME, __func__, ctrl1);
#if defined(CONFIG_S2MU005_SUPPORT_BC1P2_CERTI)
	vldo_set = s2mu005_i2c_read_byte(i2c, 0x54);
	if (!(vldo_set & 0x80)) {
		vldo_set |= 0x80;
		s2mu005_i2c_write_byte(i2c, 0x54, vldo_set);

		muic_data->vbus_ldo = reg_val = s2mu005_i2c_read_byte(i2c, 0x7E);
		if ((reg_val >> 4) > 3)
			tmp_val = ((reg_val >> 4) - 3);
		else
			tmp_val = 0;

		reg_val &= ~(0xf << 4);
		reg_val |= (tmp_val << 4);
		s2mu005_i2c_write_byte(i2c, 0x7E, reg_val);
		pr_info("%s vbus_ldo(%#x->%#x)\n", __func__, muic_data->vbus_ldo, reg_val);
	}
#endif
#if !defined (CONFIG_SEC_FACTORY) && !defined (CONFIG_MUIC_S2MU005_WATER_WA_DISABLE)
	s2mu005_i2c_write_byte(i2c, S2MU005_REG_MUIC_LDOADC_VSETL, LDOADC_VSET_3V);
	s2mu005_i2c_write_byte(i2c, S2MU005_REG_MUIC_LDOADC_VSETH, LDOADC_VSET_3V);
#endif

	return ret;
}

static irqreturn_t s2mu005_muic_irq_thread(int irq, void *data)
{
	struct s2mu005_muic_data *muic_data = data;
	struct i2c_client *i2c = muic_data->i2c;
	enum s2mu005_reg_manual_sw_value reg_val;
	int ctrl, ret = 0;
#if !defined (CONFIG_SEC_FACTORY) && !defined (CONFIG_MUIC_S2MU005_WATER_WA_DISABLE)
	int vbvolt = 0, adc = 0;
#endif

	pr_info("%s:%s\n", MUIC_DEV_NAME, __func__);

	mutex_lock(&muic_data->muic_mutex);
	wake_lock(&muic_data->wake_lock);
#if !defined (CONFIG_SEC_FACTORY) && !defined (CONFIG_MUIC_S2MU005_WATER_WA_DISABLE)
	vbvolt = !!(s2mu005_i2c_read_byte(i2c, S2MU005_REG_MUIC_DEVICE_APPLE) 
				& DEV_TYPE_APPLE_VBUS_WAKEUP);
	adc = s2mu005_i2c_read_byte(i2c, S2MU005_REG_MUIC_ADC) & ADC_MASK;
	pr_info("%s:%s vbvolt : %d, adc: 0x%X, irq : %d\n",
				MFD_DEV_NAME, __func__, vbvolt, adc, irq);

	if (!vbvolt) {
		if (IS_AUDIO_ADC(adc) && !muic_data->is_water_wa) {
			if (irq == muic_data->irq_adc_change) {
				s2mu005_muic_set_water_wa(muic_data, true);
			}
		} else if (IS_WATER_ADC(adc) && !muic_data->is_water_wa) {
			if (irq == muic_data->irq_attach) {
				s2mu005_muic_set_water_wa(muic_data, true);
			}
		}
	}

	if (adc == ADC_OPEN
		&& irq ==  muic_data->irq_adc_change
		&& muic_data->is_water_wa) {
		usleep_range(WATER_TOGGLE_WA_MIN_DURATION_US, WATER_TOGGLE_WA_MAX_DURATION_US);
		s2mu005_muic_set_water_wa(muic_data, false);
	}
#endif
#if defined(CONFIG_S2MU005_SUPPORT_BC1P2_CERTI)
	if (irq == muic_data->irq_vbus_off)
		s2mu005_muic_handle_vbus_off(muic_data);
#endif
	/* check for muic reset and re-initialize registers */
	ctrl = s2mu005_i2c_read_byte(i2c, S2MU005_REG_MUIC_CTRL1);

	if (ctrl == 0xDF) {
		/* CONTROL register is reset to DF */
#ifdef DEBUG_MUIC
		s2mu005_print_reg_log();
		s2mu005_print_reg_dump(muic_data);
#endif
		pr_err("%s:%s: err muic could have been reseted. Initilize!!\n",
						MUIC_DEV_NAME, __func__);
		s2mu005_muic_reg_init(muic_data);
		if (muic_data->is_rustproof) {
			pr_info("%s:%s: rustproof is enabled\n", MUIC_DEV_NAME, __func__);
			reg_val = MANSW_OPEN;
			ret = s2mu005_i2c_write_byte(i2c,
				S2MU005_REG_MUIC_SW_CTRL, reg_val);
			if (ret < 0)
				pr_err("%s:%s: err write MANSW\n", MUIC_DEV_NAME, __func__);
			ret = s2mu005_i2c_read_byte(i2c, S2MU005_REG_MUIC_SW_CTRL);
			pr_info("%s:%s: MUIC_SW_CTRL=0x%x\n ", MUIC_DEV_NAME, __func__, ret);
		} else {
			reg_val = MANSW_UART;
			ret = s2mu005_i2c_write_byte(i2c,
				S2MU005_REG_MUIC_SW_CTRL, reg_val);
			if (ret < 0)
				pr_err("%s:%s: err write MANSW\n", MUIC_DEV_NAME, __func__);
			ret = s2mu005_i2c_read_byte(i2c, S2MU005_REG_MUIC_SW_CTRL);
			pr_info("%s:%s: MUIC_SW_CTRL=0x%x\n ", MUIC_DEV_NAME, __func__, ret);
		}
#ifdef DEBUG_MUIC
		s2mu005_print_reg_dump(muic_data);
#endif
		/* MUIC Interrupt On */
		ret = set_ctrl_reg(muic_data, CTRL_INT_MASK_SHIFT, false);
#if defined (CONFIG_SEC_FACTORY) && defined (CONFIG_USB_HOST_NOTIFY)
		init_otg_reg(muic_data);
#endif
	}

	/* device detection */
	s2mu005_muic_detect_dev(muic_data);

	wake_unlock(&muic_data->wake_lock);
	mutex_unlock(&muic_data->muic_mutex);

	return IRQ_HANDLED;
}

static int s2mu005_init_rev_info(struct s2mu005_muic_data *muic_data)
{
	u8 dev_id;
	int ret = 0;

	pr_info("%s:%s\n", MUIC_DEV_NAME, __func__);

	dev_id = s2mu005_i2c_read_byte(muic_data->i2c, S2MU005_REG_REV_ID);
	if (dev_id < 0) {
		pr_err( "%s:%s: dev_id(%d)\n", MUIC_DEV_NAME, __func__, dev_id);
		ret = -ENODEV;
	} else {
		muic_data->muic_vendor = 0x05;
		muic_data->muic_version = (dev_id & 0x0F);
		pr_info( "%s:%s: vendor=0x%x, ver=0x%x, dev_id=0x%x\n", MUIC_DEV_NAME, __func__,
					muic_data->muic_vendor, muic_data->muic_version, dev_id);
	}
	return ret;
}

#define REQUEST_IRQ(_irq, _dev_id, _name)				\
do {									\
	ret = request_threaded_irq(_irq, NULL, s2mu005_muic_irq_thread,	\
				0, _name, _dev_id);	\
	if (ret < 0) {							\
		pr_err("%s:%s Failed to request IRQ #%d: %d\n",		\
				MUIC_DEV_NAME, __func__, _irq, ret);	\
		_irq = 0;						\
	}								\
} while (0)

static int s2mu005_muic_irq_init(struct s2mu005_muic_data *muic_data)
{
	int ret = 0;

	pr_info("%s:%s\n", MUIC_DEV_NAME, __func__);

	if (muic_data->mfd_pdata && (muic_data->mfd_pdata->irq_base > 0)) {
		int irq_base = muic_data->mfd_pdata->irq_base;

		/* request MUIC IRQ */
		muic_data->irq_attach = irq_base + S2MU005_MUIC_IRQ1_ATTATCH;
		REQUEST_IRQ(muic_data->irq_attach, muic_data, "muic-attach");

		muic_data->irq_detach = irq_base + S2MU005_MUIC_IRQ1_DETACH;
		REQUEST_IRQ(muic_data->irq_detach, muic_data, "muic-detach");

		muic_data->irq_vbus_on = irq_base + S2MU005_MUIC_IRQ2_VBUS_ON;
		REQUEST_IRQ(muic_data->irq_vbus_on, muic_data, "muic-vbus_on");

		muic_data->irq_adc_change = irq_base + S2MU005_MUIC_IRQ2_ADC_CHANGE;
		REQUEST_IRQ(muic_data->irq_adc_change, muic_data, "muic-adc_change");

		muic_data->irq_vbus_off = irq_base + S2MU005_MUIC_IRQ2_VBUS_OFF;
		REQUEST_IRQ(muic_data->irq_vbus_off, muic_data, "muic-vbus_off");
	}

	pr_info("%s:%s: muic-attach(%d), muic-detach(%d), muic-vbus_on(%d), muic-adc_change(%d), muic-vbus_off(%d)\n",
			MUIC_DEV_NAME, __func__, muic_data->irq_attach, muic_data->irq_detach,
			muic_data->irq_vbus_on, muic_data->irq_adc_change, muic_data->irq_vbus_off);

	return ret;
}

#define FREE_IRQ(_irq, _dev_id, _name)					\
do {									\
	if (_irq) {							\
		free_irq(_irq, _dev_id);				\
		pr_info("%s:%s IRQ(%d):%s free done\n", MUIC_DEV_NAME,	\
				__func__, _irq, _name);			\
	}								\
} while (0)

static void s2mu005_muic_free_irqs(struct s2mu005_muic_data *muic_data)
{
	pr_info("%s:%s\n", MUIC_DEV_NAME, __func__);

	/* free MUIC IRQ */
	FREE_IRQ(muic_data->irq_attach, muic_data, "muic-attach");
	FREE_IRQ(muic_data->irq_detach, muic_data, "muic-detach");
	FREE_IRQ(muic_data->irq_vbus_on, muic_data, "muic-vbus_on");
	FREE_IRQ(muic_data->irq_adc_change, muic_data, "muic-adc_change");
	FREE_IRQ(muic_data->irq_vbus_off, muic_data, "muic-vbus_off");
}

#if defined(CONFIG_OF)
static int of_s2mu005_muic_dt(struct device *dev, struct s2mu005_muic_data *muic_data)
{
	struct device_node *np, *np_muic;
	struct muic_platform_data *pdata = muic_data->pdata;
	int ret = 0;
	pr_info("%s:%s\n", MUIC_DEV_NAME, __func__);

	np = dev->parent->of_node;
	if (!np) {
		pr_err("%s:%s: could not find np\n", MUIC_DEV_NAME, __func__);
		return -ENODEV;
	}

	np_muic = of_find_node_by_name(NULL, "muic");
	if (!np_muic) {
		pr_err("%s:%s: could not find muic sub-node np_muic\n", MUIC_DEV_NAME, __func__);
		return -EINVAL;
	}

#if !defined(CONFIG_MUIC_UART_SWITCH)
	if (of_gpio_count(np_muic) < 1) {
		pr_err("%s:%s: could not find muic gpio\n", MUIC_DEV_NAME, __func__);
		muic_data->pdata->gpio_uart_sel = 0;
	} else
		muic_data->pdata->gpio_uart_sel = of_get_gpio(np_muic, 0);
#endif
	pdata->is_new_factory = of_property_read_bool(np_muic, "new_factory");
	pdata->dcd_timeout = of_property_read_bool(np_muic, "dcd_timeout");
	return ret;
}
#endif /* CONFIG_OF */

/* if need to set s2mu005 pdata */
static struct of_device_id s2mu005_muic_match_table[] = {
	{ .compatible = "samsung,s2mu005-muic",},
	{},
};

static int s2mu005_muic_probe(struct platform_device *pdev)
{
	struct s2mu005_dev *s2mu005 = dev_get_drvdata(pdev->dev.parent);
	struct s2mu005_platform_data *mfd_pdata = dev_get_platdata(s2mu005->dev);
	struct s2mu005_muic_data *muic_data;
	int ret = 0;

	pr_info("%s:%s\n", MUIC_DEV_NAME, __func__);

	muic_data = kzalloc(sizeof(struct s2mu005_muic_data), GFP_KERNEL);
	if (!muic_data) {
		pr_err( "%s:%s: failed to allocate driver data\n", MUIC_DEV_NAME, __func__);
		ret = -ENOMEM;
		goto err_return;
	}

	if (!mfd_pdata) {
		pr_err("%s:%s: failed to get s2mu005 mfd platform data\n", MUIC_DEV_NAME, __func__);
		ret = -ENOMEM;
		goto err_kfree;
	}

	/* save platfom data for gpio control functions */

	muic_data->dev = &pdev->dev;
	muic_data->i2c = s2mu005->i2c;
	muic_data->mfd_pdata = mfd_pdata;
	muic_data->pdata = &muic_pdata;
#if defined(CONFIG_OF)
	ret = of_s2mu005_muic_dt(&pdev->dev, muic_data);
	if (ret < 0)
		pr_err( "%s:%s: no muic dt! ret[%d]\n", MUIC_DEV_NAME, __func__, ret);
#endif /* CONFIG_OF */

	mutex_init(&muic_data->muic_mutex);
	muic_data->is_factory_start = false;
	muic_data->attached_dev = ATTACHED_DEV_UNKNOWN_MUIC;
	muic_data->is_usb_ready = false;
	muic_data->jigonb_enable = false;
	muic_data->jig_disable = false;
#if defined(CONFIG_S2MU005_SUPPORT_BC1P2_CERTI)
	mutex_init(&muic_data->recheck_mutex);
	muic_data->is_dcp = false;
	muic_data->usb_type_rechecked = false;
#endif
#if !defined (CONFIG_SEC_FACTORY) && !defined (CONFIG_MUIC_S2MU005_WATER_WA_DISABLE)
	muic_data->is_water_wa = false;
#endif
	platform_set_drvdata(pdev, muic_data);

	if (muic_data->pdata->init_gpio_cb)
		ret = muic_data->pdata->init_gpio_cb(get_switch_sel());
	if (ret) {
		pr_err( "%s:%s: failed to init gpio(%d)\n", MUIC_DEV_NAME, __func__, ret);
		goto fail_init_gpio;
	}


	/* create sysfs group */
	ret = sysfs_create_group(&switch_device->kobj, &s2mu005_muic_group);
	if (ret) {
		pr_err( "%s:%s: failed to create sysfs\n", MUIC_DEV_NAME, __func__);
		goto fail;
	}
	dev_set_drvdata(switch_device, muic_data);


	ret = s2mu005_init_rev_info(muic_data);
	if (ret) {
		pr_err( "%s:%s: failed to init muic(%d)\n", MUIC_DEV_NAME, __func__, ret);
		goto fail;
	}

	ret = s2mu005_muic_reg_init(muic_data);
	if (ret) {
		pr_err( "%s:%s: failed to init muic(%d)\n", MUIC_DEV_NAME, __func__, ret);
		goto fail;
	}

	/* For Rustproof */
	muic_data->is_rustproof = muic_data->pdata->rustproof_on;
	if (muic_data->is_rustproof) {
		pr_info( "%s:%s: rustproof is enabled\n", MUIC_DEV_NAME, __func__);
		com_to_open(muic_data);
	}

	if (muic_data->pdata->init_switch_dev_cb)
		muic_data->pdata->init_switch_dev_cb();

	ret = s2mu005_muic_irq_init(muic_data);
	if (ret) {
		pr_err( "%s:%s: failed to init irq(%d)\n", MUIC_DEV_NAME, __func__, ret);
		goto fail_init_irq;
	}

	wake_lock_init(&muic_data->wake_lock, WAKE_LOCK_SUSPEND, "muic_wake");

	/* initial cable detection */
	ret = set_ctrl_reg(muic_data, CTRL_INT_MASK_SHIFT, false);
#if defined (CONFIG_SEC_FACTORY) && defined (CONFIG_USB_HOST_NOTIFY)
	init_otg_reg(muic_data);
#endif
#if defined(CONFIG_S2MU005_SUPPORT_BC1P2_CERTI)
	INIT_DELAYED_WORK(&muic_data->cable_recheck, s2mu005_muic_cable_recheck);
	INIT_DELAYED_WORK(&muic_data->dp_0p6v, s2mu005_muic_dp_0p6v);
#endif
	s2mu005_muic_irq_thread(-1, muic_data);

#if defined (CONFIG_KEEP_JIG_LOW)
	if(always_keep_jigonb_low)
		set_jig_level_low(muic_data,true);
#endif
	return 0;

fail_init_irq:
	s2mu005_muic_free_irqs(muic_data);
fail:

	sysfs_remove_group(&switch_device->kobj, &s2mu005_muic_group);

	mutex_destroy(&muic_data->muic_mutex);
#if defined(CONFIG_S2MU005_SUPPORT_BC1P2_CERTI)
	mutex_destroy(&muic_data->recheck_mutex);
#endif
fail_init_gpio:
err_kfree:
	kfree(muic_data);
err_return:
	return ret;
}

static int s2mu005_muic_remove(struct platform_device *pdev)
{
	struct s2mu005_muic_data *muic_data = platform_get_drvdata(pdev);

	sysfs_remove_group(&switch_device->kobj, &s2mu005_muic_group);


	if (muic_data) {
		pr_info("%s:%s\n", MUIC_DEV_NAME, __func__);
		disable_irq_wake(muic_data->i2c->irq);
		s2mu005_muic_free_irqs(muic_data);
		mutex_destroy(&muic_data->muic_mutex);
#if defined(CONFIG_S2MU005_SUPPORT_BC1P2_CERTI)
	mutex_destroy(&muic_data->recheck_mutex);
#endif
		i2c_set_clientdata(muic_data->i2c, NULL);
		kfree(muic_data);
	}
	return 0;
}

static void s2mu005_muic_shutdown(struct platform_device *pdev)
{
#if defined(CONFIG_S2MU005_SUPPORT_BC1P2_CERTI)
	struct s2mu005_muic_data *muic_data = platform_get_drvdata(pdev);
	struct i2c_client *i2c = muic_data->i2c;
	u8 reg_val, vldo_set = 0;
#endif
#if !defined(CONFIG_MUIC_S2MU005_JIGB_CONTROL)
		struct s2mu005_muic_data *muic_data = platform_get_drvdata(pdev);
		int ret;
#endif
	
		pr_info("%s:%s\n", MUIC_DEV_NAME, __func__);
		
#if !defined(CONFIG_MUIC_S2MU005_JIGB_CONTROL)
		if (!muic_data->i2c) {
			pr_err( "%s:%s: no muic i2c client\n", MUIC_DEV_NAME, __func__);
			return;
		}
	
		pr_info("%s:%s: open D+,D-,V_bus line\n", MUIC_DEV_NAME, __func__);
		ret = com_to_open(muic_data);
		if (ret < 0)
			pr_err( "%s:%s: fail to open mansw\n", MUIC_DEV_NAME, __func__);
	
		/* set auto sw mode before shutdown to make sure device goes into */
		/* LPM charging when TA or USB is connected during off state */
		pr_info("%s:%s: muic auto detection enable\n", MUIC_DEV_NAME, __func__);
		ret = set_ctrl_reg(muic_data, CTRL_MANUAL_SW_SHIFT, true);
		if (ret < 0) {
			pr_err( "%s:%s: fail to update reg\n", MUIC_DEV_NAME, __func__);
			return;
		}
#endif
#if defined(CONFIG_S2MU005_SUPPORT_BC1P2_CERTI)
	if (muic_data->is_dcp) {
		s2mu005_muic_dp_0_6V(muic_data, false);
		muic_data->is_dcp = false;
	}
	muic_data->usb_type_rechecked = false;

	vldo_set = s2mu005_i2c_read_byte(i2c, 0x54);
	if (vldo_set & 0x80) {
		vldo_set &= ~0x80;
		s2mu005_i2c_write_byte(i2c, 0x54, vldo_set);

		reg_val = s2mu005_i2c_read_byte(muic_data->i2c, 0x7E);
		s2mu005_i2c_write_byte(muic_data->i2c, 0x7E, muic_data->vbus_ldo);
		pr_info("%s vbus_ldo(%#x->%#x)\n", __func__, reg_val, muic_data->vbus_ldo);
	}
#endif
}

#if defined CONFIG_PM
static int s2mu005_muic_suspend(struct device *dev)
{
	struct s2mu005_muic_data *muic_data = dev_get_drvdata(dev);

	pr_info("%s:%s\n", MUIC_DEV_NAME, __func__);
	muic_data->suspended = true;

	return 0;
}

static int s2mu005_muic_resume(struct device *dev)
{
	struct s2mu005_muic_data *muic_data = dev_get_drvdata(dev);

	pr_info("%s:%s\n", MUIC_DEV_NAME, __func__);
	muic_data->suspended = false;

	if (muic_data->need_to_noti) {
		if (muic_data->attached_dev)
			muic_notifier_attach_attached_dev(muic_data->attached_dev);
		else
			muic_notifier_detach_attached_dev(muic_data->attached_dev);
		muic_data->need_to_noti = false;
	}

	return 0;
}
#else
#define s2mu005_muic_suspend NULL
#define s2mu005_muic_resume NULL
#endif

static SIMPLE_DEV_PM_OPS(s2mu005_muic_pm_ops, s2mu005_muic_suspend,
			 s2mu005_muic_resume);

static struct platform_driver s2mu005_muic_driver = {
	.driver = {
		.name = "s2mu005-muic",
		.owner	= THIS_MODULE,
		.of_match_table = s2mu005_muic_match_table,
#ifdef CONFIG_PM
		.pm = &s2mu005_muic_pm_ops,
#endif
	},
	.probe = s2mu005_muic_probe,
	.shutdown = s2mu005_muic_shutdown,
	.remove = s2mu005_muic_remove,
};

static int __init s2mu005_muic_init(void)
{
	pr_info("%s:%s\n", MUIC_DEV_NAME, __func__);
	return platform_driver_register(&s2mu005_muic_driver);
}
module_init(s2mu005_muic_init);

static void __exit s2mu005_muic_exit(void)
{
	pr_info("%s:%s\n", MUIC_DEV_NAME, __func__);
	platform_driver_unregister(&s2mu005_muic_driver);
}
module_exit(s2mu005_muic_exit);

MODULE_DESCRIPTION("S.LSI S2MU005 MUIC driver");
MODULE_AUTHOR("<insun77.choi@samsung.com>");
MODULE_LICENSE("GPL");
