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
#include <linux/wakelock.h>

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

#define GPIO_LEVEL_HIGH		1
#define GPIO_LEVEL_LOW		0

static void s2mu005_muic_handle_attach(struct s2mu005_muic_data *muic_data,
			muic_attached_dev_t new_dev, int adc, u8 vbvolt);
static void s2mu005_muic_handle_detach(struct s2mu005_muic_data *muic_data);

extern int muic_wakeup_noti;

void muic_set_wakeup_noti(int flag)
{
	pr_info("%s: %d\n", __func__, flag);
	muic_wakeup_noti = flag;
}

/*
#define DEBUG_MUIC
*/

#if defined(DEBUG_MUIC)
#define MAX_LOG 25
#define READ 0
#define WRITE 1

static u8 s2mu005_log_cnt;
static u8 s2mu005_log[MAX_LOG][3];

static int s2mu005_i2c_read_byte(struct i2c_client *client, u8 command);
static int s2mu005_i2c_write_byte(struct i2c_client *client,
			u8 command, u8 value);

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
	pr_info("%s:%s\n", __func__, mesg);
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

	pr_info("%s:%s\n", __func__, mesg);
}
#endif

static struct vps_tbl_data vps_table[] = {
	[MDEV(OTG)]			= {0x00, "GND",	"OTG", MUIC_UNSUPPORTED_VPS,},
	[MDEV(MHL)]			= {0xfe, "1K", "MHL", MUIC_UNSUPPORTED_VPS,},
	/* 0x01 ~ 0x0D : Remote Sx Button */
	[MDEV(VZW_ACC)]		= {0x0e, "28.7K", "VZW Accessory", MUIC_UNSUPPORTED_VPS,},
	[MDEV(VZW_INCOMPATIBLE)]	= {0x0f, "34K",	"VZW Incompatible", MUIC_UNSUPPORTED_VPS,},
	[MDEV(SMARTDOCK)]		= {0x10, "40.2K", "Smartdock",MUIC_UNSUPPORTED_VPS,},
	[MDEV(HMT)]			= {0x11, "49.9K", "HMT", MUIC_UNSUPPORTED_VPS,},
	[MDEV(AUDIODOCK)]		= {0x12, "64.9K", "Audiodock", MUIC_UNSUPPORTED_VPS,},
	[MDEV(USB_LANHUB)]		= {0x13, "80.07K", "USB LANHUB", MUIC_UNSUPPORTED_VPS,},
	[MDEV(CHARGING_CABLE)]	= {0x14, "102K", "Charging Cable", MUIC_UNSUPPORTED_VPS,},
	[MDEV(UNIVERSAL_MMDOCK)]	= {0x15, "121K", "Universal Multimedia dock", MUIC_UNSUPPORTED_VPS,},
	/* 0x16: UART Cable */
	/* 0x17: CEA-936A Type 1 Charger */
	[MDEV(JIG_USB_OFF)]		= {0x18, "255K", "Jig USB Off", MUIC_SUPPORTED_VPS,},
	[MDEV(JIG_USB_ON)]		= {0x19, "301K", "Jig USB On", MUIC_SUPPORTED_VPS,},
	[MDEV(DESKDOCK)]		= {0x1a, "365K", "Deskdock", MUIC_UNSUPPORTED_VPS,},
	[MDEV(TYPE2_CHG)]		= {0x1b, "442K", "TYPE2 Charger", MUIC_UNSUPPORTED_VPS,},
	[MDEV(JIG_UART_OFF)]		= {0x1c, "523K", "Jig UART Off", MUIC_SUPPORTED_VPS,},
	[MDEV(JIG_UART_ON)]		= {0x1d, "619K", "Jig UART On", MUIC_SUPPORTED_VPS,},
	/* 0x1e: Audio Mode with Remote */
	[MDEV(TA)]			= {0x1f, "OPEN", "TA", MUIC_SUPPORTED_VPS,},
	[MDEV(USB)]			= {0x1f, "OPEN", "USB", MUIC_SUPPORTED_VPS,},
	[MDEV(CDP)]			= {0x1f, "OPEN", "CDP", MUIC_SUPPORTED_VPS,},
	[MDEV(UNDEFINED_CHARGING)]	= {0xfe, "UNDEFINED", "Undefined Charging", MUIC_SUPPORTED_VPS,},
	[ATTACHED_DEV_NUM]		= {0x00, "NUM", "NAME", 0,},
};

static bool mdev_undefined_range(int adc)
{
	switch (adc) {
	case ADC_SEND_END ... ADC_REMOTE_S12:
	case ADC_UART_CABLE:
//	case ADC_AUDIOMODE_W_REMOTE:
		return true;
	default:
		break;
	}

	return false;
}

struct vps_tbl_data * mdev_to_vps(muic_attached_dev_t mdev)
{
	if (mdev >= ATTACHED_DEV_NUM) {
		pr_err("%s Out of range mdev=%d\n", __func__, mdev);
		return NULL;
	}

	return &vps_table[mdev];
}

bool vps_name_to_mdev(const char *name, int *sdev)
{
	struct vps_tbl_data *pvps;
	int mdev;

	for (mdev = MDEV(NONE); mdev < ATTACHED_DEV_NUM; mdev++) {
		pvps = &vps_table[mdev];
		if (!pvps->name)
			continue;

		if (!strcmp(pvps->name, name)){
			break;
		}
	}

	if (mdev >= ATTACHED_DEV_NUM) {
		pr_err("%s Out of range mdev=%d, %s\n", __func__,
						mdev, name);
		return false;
	}

	pr_info("%s:%s->[%2d]\n", __func__, pvps->name, mdev);

	*sdev = mdev;

	return true;
}

bool vps_is_supported_dev(muic_attached_dev_t mdev)
{
	struct vps_tbl_data *pvps = mdev_to_vps(mdev);

	if (!pvps)
		return false;

	return pvps->supported;
}

void vps_update_supported_attr(muic_attached_dev_t mdev, bool supported)
{
	struct vps_tbl_data *pvps = mdev_to_vps(mdev);

	if (!pvps)
		return;

	if (supported)
		pvps->supported = MUIC_SUPPORTED_VPS;
	else
		pvps->supported = MUIC_UNSUPPORTED_VPS;
}

/* Check it the resolved device type is treated as a different one
 * when VBUS also comes along.
 */
int resolve_twin_mdev(struct s2mu005_muic_data *muic_data, int mdev, bool vbus)
{
	if (vbus) {
		if (mdev == MDEV(DESKDOCK)) {
			pr_info("%s: mdev:%d vbus:%d\n",__func__, mdev, vbus);
			return MDEV(DESKDOCK_VB);
		} else if (mdev == MDEV(JIG_UART_OFF)) {
			pr_info("%s: mdev:%d vbus:%d\n",__func__, mdev, vbus);
			if (muic_data->is_otg_test)
				return MDEV(JIG_UART_OFF_VB_OTG);
			else
				return MDEV(JIG_UART_OFF_VB);
		}
	}

	return 0;
}

/*
  * vps_show_tablbe() functions displays the VPS table as a below format
dev   ADC      (RID)   supported             vps_name
 ----------------------------------------------------
[ 1] = 1f(      OPEN): S                          USB
[ 2] = 1f(      OPEN): S                          CDP
[ 3] = 00(       GND): S                          OTG
[ 4] = 1f(      OPEN): S                           TA
[12] = fe( UNDEFINED): S           Undefined Charging
[13] = 1a(      365K): S                     Deskdock
[16] = 1c(      523K): S                 Jig UART Off
[20] = 1d(      619K): S                  Jig UART On
[21] = 18(      255K): S                  Jig USB Off
[22] = 19(      301K): S                   Jig USB On
[23] = 10(     40.2K): _                    Smartdock
[27] = 15(      121K): S    Universal Multimedia dock
[28] = 12(     64.9K): _                    Audiodock
[29] = fe(        1K): S                          MHL
[30] = 14(      102K): S               Charging Cable
[45] = 11(     49.9K): _                          HMT
[46] = 0e(     28.7K): _                VZW Accessory
[47] = 0f(       34K): _             VZW Incompatible
[48] = 13(    80.07K): S                   USB LANHUB
[49] = 1b(      442K): S                TYPE2 Charger
*/
void vps_show_table(void)
{
	struct vps_tbl_data *pvps;
	int mdev;

	pr_info(" %4s%6s%10s   %s %20s\n", "dev", "ADC", "(RID)", "supported", "vps_name");
	for (mdev = MDEV(NONE); mdev < ATTACHED_DEV_NUM; mdev++) {
		pvps = &vps_table[mdev];

		if (!pvps->name)
			continue;

		pr_info(" [%2d] = %02x(%10s): %c %28s\n", mdev,
			pvps->adc, pvps->rid, pvps->supported ? 'S' : '_',
			pvps->name);
	}

	pr_info("done.\n");

}

static int s2mu005_i2c_read_byte(struct i2c_client *client, u8 command)
{
	u8 ret;
	int retry = 0;

	s2mu005_read_reg(client, command, &ret);

	while (ret < 0) {
		pr_err( "[muic] %s: reg(0x%x), retrying...\n",
			__func__, command);
		if (retry > 10) {
			pr_err( "[muic] %s: retry failed!!\n", __func__);
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
		pr_err( "[muic] %s: reg(0x%x), retrying...\n",
			__func__, command);
		s2mu005_read_reg(client, command, &written);
		if (written < 0)
			pr_err( "[muic] %s: reg(0x%x)\n",
				__func__, command);
		msleep(100);
		ret = s2mu005_write_reg(client, command, value);
		retry++;
	}
#ifdef DEBUG_MUIC
	s2mu005_reg_log(command, value, retry << 1 | WRITE);
#endif
	return ret;
}
static int s2mu005_i2c_guaranteed_wbyte(struct i2c_client *client,
			u8 command, u8 value)
{
	int ret;
	int retry = 0;
	int written;

	ret = s2mu005_i2c_write_byte(client, command, value);
	written = s2mu005_i2c_read_byte(client, command);

	while (written != value) {
		pr_err( "[muic] reg(0x%x): written(0x%x) != value(0x%x)\n",
			command, written, value);
		if (retry > 10) {
			pr_err( "[muic] %s: retry failed!!\n", __func__);
			break;
		}
		msleep(100);
		retry++;
		ret = s2mu005_i2c_write_byte(client, command, value);
		written = s2mu005_i2c_read_byte(client, command);
	}
	return ret;
}

#if defined(GPIO_USB_SEL)
static int s2mu005_set_gpio_usb_sel(int uart_sel)
{
	return 0;
}
#endif /* GPIO_USB_SEL */

static int s2mu005_set_gpio_uart_sel(int uart_sel)
{
	const char *mode;
#if !defined(CONFIG_MUIC_UART_SWITCH)
	int uart_sel_gpio = muic_pdata.gpio_uart_sel;
	int uart_sel_val;
	int ret;

	ret = gpio_request(uart_sel_gpio, "GPIO_UART_SEL");
	if (ret) {
		pr_err( "[muic] failed to gpio_request GPIO_UART_SEL\n");
		return ret;
	}

	uart_sel_val = gpio_get_value(uart_sel_gpio);

	switch (uart_sel) {
	case MUIC_PATH_UART_AP:
		mode = "AP_UART";
		if (gpio_is_valid(uart_sel_gpio))
			gpio_direction_output(uart_sel_gpio, 1);
		break;
	case MUIC_PATH_UART_CP:
		mode = "CP_UART";
		if (gpio_is_valid(uart_sel_gpio))
			gpio_direction_output(uart_sel_gpio, 0);
		break;
	default:
		mode = "Error";
		break;
	}

	uart_sel_val = gpio_get_value(uart_sel_gpio);

	gpio_free(uart_sel_gpio);

	pr_err("[muic] %s, GPIO_UART_SEL(%d)=%c\n",
		mode, uart_sel_gpio, (uart_sel_val == 0 ? 'L' : 'H'));
#else
	switch (uart_sel) {
	case MUIC_PATH_UART_AP:
		mode = "AP_UART";
		pin_config_set(muic_pdata.uart_addr, muic_pdata.uart_rxd,
					PINCFG_PACK(PINCFG_TYPE_FUNC, 0x2));
		pin_config_set(muic_pdata.uart_addr, muic_pdata.uart_txd,
					PINCFG_PACK(PINCFG_TYPE_FUNC, 0x2));
		break;
	case MUIC_PATH_UART_CP:
		mode = "CP_UART";
		pin_config_set(muic_pdata.uart_addr, muic_pdata.uart_rxd,
					PINCFG_PACK(PINCFG_TYPE_FUNC, 0x3));
		pin_config_set(muic_pdata.uart_addr, muic_pdata.uart_txd,
					PINCFG_PACK(PINCFG_TYPE_FUNC, 0x3));
		break;
	default:
		mode = "Error";
		break;
	}

	printk(KERN_DEBUG "[muic] %s %s\n", __func__, mode);
#endif
	return 0;
}
#if defined(GPIO_DOC_SWITCH)
static int s2mu005_set_gpio_doc_switch(int val)
{
	int doc_switch_gpio = muic_pdata.gpio_doc_switch;
	int doc_switch_val;
	int ret;

	ret = gpio_request(doc_switch_gpio, "GPIO_DOC_SWITCH");
	if (ret) {
		pr_err( "[muic] failed to gpio_request GPIO_DOC_SWITCH\n");
		return ret;
	}

	doc_switch_val = gpio_get_value(doc_switch_gpio);

	if (gpio_is_valid(doc_switch_gpio))
			gpio_set_value(doc_switch_gpio, val);

	doc_switch_val = gpio_get_value(doc_switch_gpio);

	gpio_free(doc_switch_gpio);

	pr_err("[muic] GPIO_DOC_SWITCH(%d)=%c\n",
		doc_switch_gpio, (doc_switch_val == 0 ? 'L' : 'H'));

	return 0;
}
#endif /* GPIO_DOC_SWITCH */

#if defined(CONFIG_SEC_FACTORY) && defined(CONFIG_USB_HOST_NOTIFY)
static int set_otg_reg(struct s2mu005_muic_data *muic_data, bool on)
{
	struct i2c_client *i2c = muic_data->i2c;
	u8 reg_val;
	int ret = 0;

	/* 0x1e : hidden register */
	ret = s2mu005_i2c_read_byte(i2c, 0x1e);
	if (ret < 0)
		pr_err( "[muic] %s err read 0x1e reg(%d)\n", __func__, ret);

	/* Set 0x1e[5:4] bit to 0x11 or 0x01 */
	if (on)
		reg_val = ret | (0x1 << 5);
	else
		reg_val = ret & ~(0x1 << 5);

	if (reg_val ^ ret) {
		pr_err("[muic] %s 0x%x != 0x%x, update\n", __func__, reg_val, ret);

		ret = s2mu005_i2c_guaranteed_wbyte(i2c, 0x1e, reg_val);
		if (ret < 0)
			pr_err( "[muic] %s err write(%d)\n", __func__, ret);
	} else {
		pr_err("[muic] %s 0x%x == 0x%x, just return\n", __func__, reg_val, ret);
		return 0;
	}

	ret = s2mu005_i2c_read_byte(i2c, 0x1e);
	if (ret < 0)
		pr_err( "[muic] %s err read reg 0x1e(%d)\n", __func__, ret);
	else
		pr_err("[muic] %s after change(0x%x)\n", __func__, ret);

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
		pr_err( "[muic] %s err read 'reg 0x73'(%d)\n", __func__, ret);

	if ((ret&0xF) > 0)
		return 0;

	/* 0x89 : hidden register */
	ret = s2mu005_i2c_read_byte(i2c, 0x89);
	if (ret < 0)
		pr_err( "[muic] %s err read 'reg 0x89'(%d)\n", __func__, ret);

	/* Set 0x89[1] bit : T_DET_VAL */
	reg_val = ret | (0x1 << 1);

	if (reg_val ^ ret) {
		pr_err("[muic] %s 0x%x != 0x%x, update\n", __func__, reg_val, ret);

		ret = s2mu005_i2c_guaranteed_wbyte(i2c, 0x89, reg_val);
		if (ret < 0)
			pr_err( "[muic] %s err write(%d)\n", __func__, ret);
	} else {
		pr_err("[muic] %s 0x%x == 0x%x, just return\n", __func__, reg_val, ret);
		return 0;
	}

	ret = s2mu005_i2c_read_byte(i2c, 0x89);
	if (ret < 0)
		pr_err( "[muic] %s err read 'reg 0x89'(%d)\n", __func__, ret);
	else
		pr_err("[muic] %s after change(0x%x)\n", __func__, ret);
	
	/* 0x92 : hidden register */
	ret = s2mu005_i2c_read_byte(i2c, 0x92);
	if (ret < 0)
		pr_err( "[muic] %s err read 'reg 0x92'(%d)\n", __func__, ret);

	/* Set 0x92[7] bit : EN_JIG_AP */
	reg_val = ret | (0x1 << 7);

	if (reg_val ^ ret) {
		pr_err("[muic] %s 0x%x != 0x%x, update\n", __func__, reg_val, ret);

		ret = s2mu005_i2c_guaranteed_wbyte(i2c, 0x92, reg_val);
		if (ret < 0)
			pr_err( "[muic] %s err write(%d)\n", __func__, ret);
	} else {
		pr_err("[muic] %s 0x%x == 0x%x, just return\n",	__func__, reg_val, ret);
		return 0;
	}

	ret = s2mu005_i2c_read_byte(i2c, 0x92);
	if (ret < 0)
		pr_err( "[muic] %s err read 'reg 0x92'(%d)\n", __func__, ret);
	else
		pr_err("[muic] %s after change(0x%x)\n", __func__, ret);

	return ret;
}
#endif

static int s2mu005_muic_jig_on(struct s2mu005_muic_data *muic_data)
{
	bool en = muic_data->is_jig_on;
	int reg = 0, temp = 0;

	pr_err("[muic] %s: %s\n", __func__, en ? "on" : "off");

	reg = s2mu005_i2c_read_byte(muic_data->i2c,
		S2MU005_REG_MUIC_SW_CTRL);

	if (en) {
		reg |= MANUAL_SW_JIG_EN;
		if(muic_data->muic_version >= 3) {
		/* 0xAF[7] = 0  when JIG connnected */
		temp = s2mu005_i2c_read_byte(muic_data->i2c, 0xAF);
		temp &= 0x7F;
		s2mu005_i2c_write_byte(muic_data->i2c, 0xAF, (u8)temp);
		temp = s2mu005_i2c_read_byte(muic_data->i2c, 0xAF);

		pr_err("[muic] %s: 0xAF(0x%x)\n", __func__, temp);
		}
	}
	else
		reg &= ~(MANUAL_SW_JIG_EN);

	return s2mu005_i2c_write_byte(muic_data->i2c,
		S2MU005_REG_MUIC_SW_CTRL, (u8)reg);
}

static ssize_t s2mu005_muic_show_uart_en(struct device *dev,
						struct device_attribute *attr, char *buf)
{
	struct s2mu005_muic_data *muic_data = dev_get_drvdata(dev);
	int ret = 0;

	if (!muic_data->is_rustproof) {
		pr_err("[muic] %s UART ENABLE\n",  __func__);
		ret = sprintf(buf, "1\n");
	} else {
		pr_err("[muic] %s UART DISABLE\n",  __func__);
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
		pr_err( "[muic] %s invalid value\n",  __func__);

	pr_err("[muic] %s uart_en(%d)\n",
		__func__, !muic_data->is_rustproof);

	return count;
}

static ssize_t s2mu005_muic_show_uart_sel(struct device *dev,
					   struct device_attribute *attr,
					   char *buf)
{
	struct s2mu005_muic_data *muic_data = dev_get_drvdata(dev);
	struct muic_platform_data *pdata = muic_data->pdata;
	int ret = 0;

	switch (pdata->uart_path) {
	case MUIC_PATH_UART_AP:
		pr_err("[muic] %s AP\n",  __func__);
		ret = sprintf(buf, "AP\n");
		break;
	case MUIC_PATH_UART_CP:
		pr_err("[muic] %s CP\n",  __func__);
		ret = sprintf(buf, "CP\n");
		break;
	default:
		pr_err("[muic] %s UNKNOWN\n",  __func__);
		ret = sprintf(buf, "UNKNOWN\n");
		break;
	}

	return ret;
}

static int switch_to_ap_uart(struct s2mu005_muic_data *muic_data);
static int switch_to_cp_uart(struct s2mu005_muic_data *muic_data);

static ssize_t s2mu005_muic_set_uart_sel(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t count)
{
	struct s2mu005_muic_data *muic_data = dev_get_drvdata(dev);
	struct muic_platform_data *pdata = muic_data->pdata;

	if (!strncasecmp(buf, "AP", 2)) {
		pdata->uart_path = MUIC_PATH_UART_AP;
		switch_to_ap_uart(muic_data);
	} else if (!strncasecmp(buf, "CP", 2)) {
		pdata->uart_path = MUIC_PATH_UART_CP;
		switch_to_cp_uart(muic_data);
	} else
		pr_err( "[muic] %s invalid value\n",  __func__);

	pr_err("[muic] %s uart_path(%d)\n",
		__func__, pdata->uart_path);

	return count;
}

static ssize_t s2mu005_muic_show_usb_sel(struct device *dev,
					   struct device_attribute *attr,
					   char *buf)
{
	struct s2mu005_muic_data *muic_data = dev_get_drvdata(dev);
	struct muic_platform_data *pdata = muic_data->pdata;

	switch (pdata->usb_path) {
	case MUIC_PATH_USB_AP:
		return sprintf(buf, "PDA\n");
	case MUIC_PATH_USB_CP:
		return sprintf(buf, "MODEM\n");
	default:
		break;
	}

	pr_err("[muic] %s UNKNOWN\n",  __func__);
	return sprintf(buf, "UNKNOWN\n");
}

static ssize_t s2mu005_muic_set_usb_sel(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t count)
{
	struct s2mu005_muic_data *muic_data = dev_get_drvdata(dev);
	struct muic_platform_data *pdata = muic_data->pdata;

	if (!strncasecmp(buf, "PDA", 3))
		pdata->usb_path = MUIC_PATH_USB_AP;
	else if (!strncasecmp(buf, "MODEM", 5))
		pdata->usb_path = MUIC_PATH_USB_CP;
	else
		pr_err( "[muic] %s invalid value\n", __func__);

	pr_err("[muic] %s usb_path(%d)\n",
		__func__, pdata->usb_path);

	return count;
}

static ssize_t s2mu005_muic_show_usb_en(struct device *dev,
					   struct device_attribute *attr,
					   char *buf)
{
	struct s2mu005_muic_data *muic_data = dev_get_drvdata(dev);

	return sprintf(buf, "%s:%s attached_dev = %d\n",
		MUIC_DEV_NAME, __func__, muic_data->attached_dev);
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
		pr_err( "[muic] %s invalid value\n", __func__);

	pr_err("[muic] %s attached_dev(%d)\n",
		__func__, muic_data->attached_dev);

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
		pr_err( "[muic] %s err read adc reg(%d)\n",
			__func__, ret);
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
		pr_err("[muic] %s muic_data->attached_dev(%d)\n",
			__func__, muic_data->attached_dev);

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

	pr_info("func:%s ret:%d buf%s\n", __func__, ret, buf);

	if (ret < 0) {
		pr_err("%s: fail to read muic reg\n", __func__);
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

	pr_info("func:%s st1:0x%x st2:0x%x buf%s\n", __func__, st1, st2, buf);

	if (st1 < 0 || st2 < 0) {
		pr_err("%s: fail to read muic reg\n", __func__);
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
	pr_info("%s:%s\n", __func__, mesg);

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
		pr_err( "[muic] %s: fail to read muic reg\n", __func__);
		return sprintf(buf, "UNKNOWN\n");
	}

	pr_err("[muic] func:%s ret:%d val:%x buf%s\n",
		__func__, ret, val, buf);

	val &= INT_VBUS_ON_MASK;
	return sprintf(buf, "%x\n", val);
}

static ssize_t s2mu005_muic_set_otg_test(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct s2mu005_muic_data *muic_data = dev_get_drvdata(dev);

	pr_err("[muic] %s buf:%s\n", __func__, buf);

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
		pr_err( "[muic] %s Wrong command\n", __func__);
		return count;
	}

	return count;
}
#endif

static ssize_t s2mu005_muic_show_attached_dev(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	struct s2mu005_muic_data *muic_data = dev_get_drvdata(dev);

	pr_err("[muic] %s :%d\n",
		__func__, muic_data->attached_dev);

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
	case ATTACHED_DEV_AUDIODOCK_MUIC:
		return sprintf(buf, "AUDIODOCK\n");
	case ATTACHED_DEV_CHARGING_CABLE_MUIC:
		return sprintf(buf, "PS CABLE\n");
	default:
		break;
	}

	return sprintf(buf, "UNKNOWN\n");
}

static ssize_t s2mu005_muic_show_audio_path(struct device *dev,
					     struct device_attribute *attr,
					     char *buf)
{
	return 0;
}

static ssize_t s2mu005_muic_set_audio_path(struct device *dev,
					    struct device_attribute *attr,
					    const char *buf, size_t count)
{
	return 0;
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

	pr_err("[muic] %s : %s\n",
		__func__, mode);

	return sprintf(buf, "%s\n", mode);
}

static ssize_t s2mu005_muic_set_apo_factory(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct s2mu005_muic_data *muic_data = dev_get_drvdata(dev);
	const char *mode;

	pr_err("[muic] %s buf:%s\n",
		__func__, buf);

	/* "FACTORY_START": factory mode */
	if (!strncmp(buf, "FACTORY_START", 13)) {
		muic_data->is_factory_start = true;
		mode = "FACTORY_MODE";
	} else {
		pr_err( "[muic] %s Wrong command\n",  __func__);
		return count;
	}

	return count;
}

static DEVICE_ATTR(uart_en, 0664, s2mu005_muic_show_uart_en,
					s2mu005_muic_set_uart_en);
static DEVICE_ATTR(uart_sel, 0664, s2mu005_muic_show_uart_sel,
		s2mu005_muic_set_uart_sel);
static DEVICE_ATTR(usb_sel, 0664,
		s2mu005_muic_show_usb_sel, s2mu005_muic_set_usb_sel);
static DEVICE_ATTR(adc, 0664, s2mu005_muic_show_adc, NULL);
#ifdef DEBUG_MUIC
static DEVICE_ATTR(mansw, 0664, s2mu005_muic_show_mansw, NULL);
static DEVICE_ATTR(dump_registers, 0664, s2mu005_muic_show_registers, NULL);
static DEVICE_ATTR(int_status, 0664, s2mu005_muic_show_interrupt_status, NULL);
#endif
static DEVICE_ATTR(usb_state, 0664, s2mu005_muic_show_usb_state, NULL);
#if defined(CONFIG_USB_HOST_NOTIFY)
static DEVICE_ATTR(otg_test, 0664,
		s2mu005_muic_show_otg_test, s2mu005_muic_set_otg_test);
#endif
static DEVICE_ATTR(attached_dev, 0664, s2mu005_muic_show_attached_dev, NULL);
static DEVICE_ATTR(audio_path, 0664,
		s2mu005_muic_show_audio_path, s2mu005_muic_set_audio_path);
static DEVICE_ATTR(apo_factory, 0664,
		s2mu005_muic_show_apo_factory,
		s2mu005_muic_set_apo_factory);
static DEVICE_ATTR(usb_en, 0664,
		s2mu005_muic_show_usb_en,
		s2mu005_muic_set_usb_en);

static struct attribute *s2mu005_muic_attributes[] = {
	&dev_attr_uart_en.attr,
	&dev_attr_uart_sel.attr,
	&dev_attr_usb_sel.attr,
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
	&dev_attr_audio_path.attr,
	&dev_attr_apo_factory.attr,
	&dev_attr_usb_en.attr,
	NULL
};

static const struct attribute_group s2mu005_muic_group = {
	.attrs = s2mu005_muic_attributes,
};

static int set_ctrl_reg(struct s2mu005_muic_data *muic_data, int shift, bool on)
{
	struct i2c_client *i2c = muic_data->i2c;
	u8 reg_val;
	int ret = 0;

	ret = s2mu005_i2c_read_byte(i2c, S2MU005_REG_MUIC_CTRL1);
	if (ret < 0)
		pr_err( "[muic] %s err read CTRL(%d)\n",
			__func__, ret);

	if (on)
		reg_val = ret | (0x1 << shift);
	else
		reg_val = ret & ~(0x1 << shift);

	if (reg_val ^ ret) {
		pr_err("[muic] %s 0x%x != 0x%x, update\n",
			__func__, reg_val, ret);

		ret = s2mu005_i2c_guaranteed_wbyte(i2c, S2MU005_REG_MUIC_CTRL1,
				reg_val);
		if (ret < 0)
			pr_err( "[muic] %s err write(%d)\n",
				__func__, ret);
	} else {
		pr_err("[muic] %s 0x%x == 0x%x, just return\n",
			__func__, reg_val, ret);
		return 0;
	}

	ret = s2mu005_i2c_read_byte(i2c, S2MU005_REG_MUIC_CTRL1);
	if (ret < 0)
		pr_err( "[muic] %s err read CTRL(%d)\n", __func__, ret);
	else
		pr_err("[muic] %s after change(0x%x)\n",
			__func__, ret);

	return ret;
}

static int set_int_mask(struct s2mu005_muic_data *muic_data, bool on)
{
	int shift = CTRL_INT_MASK_SHIFT;
	int ret = 0;

	ret = set_ctrl_reg(muic_data, shift, on);

	return ret;
}

static int set_manual_sw(struct s2mu005_muic_data *muic_data, bool on)
{
	int shift = CTRL_MANUAL_SW_SHIFT;
	int ret = 0;

	ret = set_ctrl_reg(muic_data, shift, on);

	return ret;
}

static int set_com_sw(struct s2mu005_muic_data *muic_data,
			enum s2mu005_reg_manual_sw_value reg_val)
{
	struct i2c_client *i2c = muic_data->i2c;
	int ret = 0;
	int temp = 0;

	/*  --- MANSW [7:5][4:2][1][0] : DM DP RSVD JIG  --- */
	temp = s2mu005_i2c_read_byte(i2c, S2MU005_REG_MUIC_SW_CTRL);
	if (temp < 0)
		pr_err( "[muic] %s err read MANSW(0x%x)\n",
			__func__, temp);

	if ((reg_val & MANUAL_SW_DM_DP_MASK) != (temp & MANUAL_SW_DM_DP_MASK)) {
		pr_err("[muic] %s 0x%x != 0x%x, update\n",
			__func__, (reg_val & MANUAL_SW_DM_DP_MASK), (temp & MANUAL_SW_DM_DP_MASK));

		ret = s2mu005_i2c_guaranteed_wbyte(i2c,
			S2MU005_REG_MUIC_SW_CTRL, ((reg_val & MANUAL_SW_DM_DP_MASK)|(temp & 0x03)));
		if (ret < 0)
			pr_err( "[muic] %s err write MANSW(0x%x)\n",
				__func__, ((reg_val & MANUAL_SW_DM_DP_MASK)|(temp & 0x03)));
	}
	else {
		pr_err("[muic] %s MANSW reg(0x%x), just pass\n",
			__func__, reg_val);
	}

	return ret;
}

static int com_to_open_with_vbus(struct s2mu005_muic_data *muic_data)
{
	enum s2mu005_reg_manual_sw_value reg_val;
	int ret = 0;

	reg_val = MANSW_OPEN_WITH_VBUS;
	ret = set_com_sw(muic_data, reg_val);
	if (ret)
		pr_err( "[muic] %s set_com_sw err\n", __func__);

	return ret;
}

#ifndef com_to_open
static int com_to_open(struct s2mu005_muic_data *muic_data)
{
	enum s2mu005_reg_manual_sw_value reg_val;
	int ret = 0;
	u8 vbvolt;

	vbvolt = s2mu005_i2c_read_byte(muic_data->i2c, S2MU005_REG_MUIC_DEVICE_APPLE);
	vbvolt &= DEV_TYPE_APPLE_VBUS_WAKEUP;
	if (vbvolt) {
		ret = com_to_open_with_vbus(muic_data);
		return ret;
	}

	reg_val = MANSW_OPEN;
	ret = set_com_sw(muic_data, reg_val);
	if (ret)
		pr_err( "[muic] %s set_com_sw err\n", __func__);

	return ret;
}
#endif

static int com_to_usb(struct s2mu005_muic_data *muic_data)
{
	enum s2mu005_reg_manual_sw_value reg_val;
	int ret = 0;

	reg_val = MANSW_USB;
	ret = set_com_sw(muic_data, reg_val);
	if (ret)
		pr_err( "[muic] %s set_com_usb err\n", __func__);

	return ret;
}

static int com_to_otg(struct s2mu005_muic_data *muic_data)
{
	enum s2mu005_reg_manual_sw_value reg_val;
	int ret = 0;

	reg_val = MANSW_OTG;
	ret = set_com_sw(muic_data, reg_val);
	if (ret)
		pr_err( "[muic] %s set_com_otg err\n", __func__);

	return ret;
}

static int com_to_uart(struct s2mu005_muic_data *muic_data)
{
	enum s2mu005_reg_manual_sw_value reg_val;
	int ret = 0;

	if (muic_data->is_rustproof) {
		pr_err("[muic] %s rustproof mode\n", __func__);
		return ret;
	}
	reg_val = MANSW_UART;
	ret = set_com_sw(muic_data, reg_val);
	if (ret)
		pr_err( "[muic] %s set_com_uart err\n", __func__);

	return ret;
}

static int com_to_audio(struct s2mu005_muic_data *muic_data)
{
	enum s2mu005_reg_manual_sw_value reg_val;
	int ret = 0;

	reg_val = MANSW_AUDIO;
	ret = set_com_sw(muic_data, reg_val);
	if (ret)
		pr_err( "[muic] %s set_com_audio err\n", __func__);

	return ret;
}

static int switch_to_dock_audio(struct s2mu005_muic_data *muic_data)
{
	int ret = 0;

	pr_err("[muic] %s\n", __func__);

	ret = com_to_audio(muic_data);
	if (ret) {
		pr_err( "[muic] %s com->audio set err\n", __func__);
		return ret;
	}

	return ret;
}

static int switch_to_system_audio(struct s2mu005_muic_data *muic_data)
{
	int ret = 0;

	pr_err("[muic] %s\n", __func__);

	return ret;
}


static int switch_to_ap_usb(struct s2mu005_muic_data *muic_data)
{
	int ret = 0;

	pr_err("[muic] %s\n", __func__);

	ret = com_to_usb(muic_data);
	if (ret) {
		pr_err( "[muic] %s com->usb set err\n", __func__);
		return ret;
	}

	return ret;
}

static int switch_to_cp_usb(struct s2mu005_muic_data *muic_data)
{
	int ret = 0;

	pr_err("[muic] %s\n", __func__);

	ret = com_to_usb(muic_data);
	if (ret) {
		pr_err( "[muic] %s com->usb set err\n", __func__);
		return ret;
	}

	return ret;
}

static int switch_to_ap_uart(struct s2mu005_muic_data *muic_data)
{
	int ret = 0;

	pr_err("[muic] %s\n", __func__);
#if !defined(CONFIG_MUIC_UART_SWITCH)
	if (muic_data->pdata->gpio_uart_sel)
#endif
		muic_data->pdata->set_gpio_uart_sel(MUIC_PATH_UART_AP);

	ret = com_to_uart(muic_data);
	if (ret) {
		pr_err( "[muic] %s com->uart set err\n", __func__);
		return ret;
	}

	return ret;
}

static int switch_to_cp_uart(struct s2mu005_muic_data *muic_data)
{
	int ret = 0;

	pr_err("[muic] %s\n", __func__);
#if !defined(CONFIG_MUIC_UART_SWITCH)
	if (muic_data->pdata->gpio_uart_sel)
#endif
		muic_data->pdata->set_gpio_uart_sel(MUIC_PATH_UART_CP);

	ret = com_to_uart(muic_data);
	if (ret) {
		pr_err( "[muic] %s com->uart set err\n", __func__);
		return ret;
	}

	return ret;
}

static int attach_charger(struct s2mu005_muic_data *muic_data,
			muic_attached_dev_t new_dev)
{
	int ret = 0;

	pr_err("[muic] %s : %d\n", __func__, new_dev);

	muic_data->attached_dev = new_dev;

	return ret;
}

static int detach_charger(struct s2mu005_muic_data *muic_data)
{
	int ret = 0;

	pr_err("[muic] %s\n", __func__);

	muic_data->attached_dev = ATTACHED_DEV_NONE_MUIC;

	return ret;
}

static int attach_usb_util(struct s2mu005_muic_data *muic_data,
			muic_attached_dev_t new_dev)
{
	int ret = 0;

	ret = attach_charger(muic_data, new_dev);
	if (ret)
		return ret;

	if (muic_data->pdata->usb_path == MUIC_PATH_USB_CP) {
		ret = switch_to_cp_usb(muic_data);
		return ret;
	}

	ret = switch_to_ap_usb(muic_data);
	return ret;
}

static int attach_usb(struct s2mu005_muic_data *muic_data,
			muic_attached_dev_t new_dev)
{
	int ret = 0;

	if (muic_data->attached_dev == new_dev) {
		pr_err("[muic] %s duplicated\n", __func__);
		return ret;
	}

	pr_err("%s:%s\n", MFD_DEV_NAME, __func__);

	pr_err("[muic] %s\n", __func__);

	ret = attach_usb_util(muic_data, new_dev);
	if (ret)
		return ret;

	return ret;
}
#if 0
static int set_vbus_interrupt(struct s2mu005_muic_data *muic_data, int enable)
{
	struct i2c_client *i2c = muic_data->i2c;
	int ret = 0;

	if (enable) {
		ret = s2mu005_i2c_write_byte(i2c, S2MU005_REG_MUIC_INT2_MASK,
			REG_INTMASK2_VBUS);
		if (ret < 0)
			pr_err( "[muic] %s(%d)\n", __func__, ret);
	} else {
		ret = s2mu005_i2c_write_byte(i2c, S2MU005_REG_MUIC_INT2_MASK,
			REG_INTMASK2_VALUE);
		if (ret < 0)
			pr_err( "[muic] %s(%d)\n", __func__, ret);
	}
	return ret;
}
#endif
static int attach_otg_usb(struct s2mu005_muic_data *muic_data,
			muic_attached_dev_t new_dev)
{
	int ret = 0;

	if (muic_data->attached_dev == new_dev) {
		pr_err("[muic] %s duplicated\n", __func__);
		return ret;
	}

	pr_err("[muic] %s\n", __func__);

	/* LANHUB doesn't work under AUTO switch mode, so turn it off */
	/* set MANUAL SW mode */
	set_manual_sw(muic_data, 0);

	/* enable RAW DATA mode, only for OTG LANHUB */
	set_ctrl_reg(muic_data, CTRL_RAW_DATA_SHIFT , 0);

	ret = com_to_otg(muic_data);

	muic_data->attached_dev = new_dev;

	return ret;
}

static int detach_otg_usb(struct s2mu005_muic_data *muic_data)
{
	struct muic_platform_data *pdata = muic_data->pdata;
	int ret = 0;

	pr_err("[muic] %s : %d\n",
		__func__, muic_data->attached_dev);

	ret = com_to_open(muic_data);
	if (ret)
		return ret;

	/* disable RAW DATA mode */
	set_ctrl_reg(muic_data, CTRL_RAW_DATA_SHIFT , 1);

#ifdef CONFIG_MACH_DEGAS
	/* System rev less than 0.1 cannot use Auto switch mode in DEGAS */
	if (system_rev >= 0x1)
#endif
		/* set AUTO SW mode */
		set_manual_sw(muic_data, 1);

	muic_data->attached_dev = ATTACHED_DEV_NONE_MUIC;

	if (pdata->usb_path == MUIC_PATH_USB_CP)
		return ret;

	return ret;
}

static int detach_usb(struct s2mu005_muic_data *muic_data)
{
	struct muic_platform_data *pdata = muic_data->pdata;
	int ret = 0;

	pr_err("[muic] %s : %d\n",
		__func__, muic_data->attached_dev);

	ret = detach_charger(muic_data);
	if (ret)
		return ret;

	ret = com_to_open(muic_data);
	if (ret)
		return ret;

	muic_data->attached_dev = ATTACHED_DEV_NONE_MUIC;

	if (pdata->usb_path == MUIC_PATH_USB_CP)
		return ret;

	return ret;
}

static int attach_deskdock(struct s2mu005_muic_data *muic_data,
			muic_attached_dev_t new_dev, u8 vbvolt)
{
	int ret = 0;

	pr_err("[muic] %s vbus(%x)\n", __func__, vbvolt);

#ifdef CONFIG_MACH_DEGAS
	/* Audio-out doesn't work under AUTO switch mode, so turn it off */
	/* set MANUAL SW mode */
	set_manual_sw(muic_data, 0);
#endif

	ret = switch_to_dock_audio(muic_data);
	if (ret)
		return ret;

	if (vbvolt)
		ret = attach_charger(muic_data, new_dev);
	else
		ret = detach_charger(muic_data);
	if (ret)
		return ret;

	muic_data->attached_dev = new_dev;

	return ret;
}

static int detach_deskdock(struct s2mu005_muic_data *muic_data)
{
	int ret = 0;

	pr_err("[muic] %s\n", __func__);

#ifdef CONFIG_MACH_DEGAS
	/* System rev less than 0.1 cannot use Auto switch mode in DEGAS */
	if (system_rev >= 0x1)
		set_manual_sw(muic_data, 1); /* set AUTO SW mode */
#endif

	ret = switch_to_system_audio(muic_data);
	if (ret)
		pr_err( "[muic] %s err changing audio path(%d)\n",
			__func__, ret);

	ret = detach_charger(muic_data);
	if (ret)
		pr_err( "[muic] %s err detach_charger(%d)\n",
			__func__, ret);

	muic_data->attached_dev = ATTACHED_DEV_NONE_MUIC;

	return ret;
}

static int attach_audiodock(struct s2mu005_muic_data *muic_data,
			muic_attached_dev_t new_dev, u8 vbus)
{
	int ret = 0;

	pr_err("[muic] %s\n", __func__);

	if (!vbus) {
		ret = detach_charger(muic_data);
		if (ret)
			pr_err( "[muic] %s err detach_charger(%d)\n",
				__func__, ret);

		ret = com_to_open(muic_data);
		if (ret)
			return ret;

		muic_data->attached_dev = new_dev;

		return ret;
	}

	ret = attach_usb_util(muic_data, new_dev);
	if (ret)
		pr_err("[muic] %s attach_usb_util(%d)\n",
			__func__, ret);

	muic_data->attached_dev = new_dev;

	return ret;
}

static int detach_audiodock(struct s2mu005_muic_data *muic_data)
{
	int ret = 0;

	pr_err("[muic] %s\n", __func__);

	ret = detach_charger(muic_data);
	if (ret)
		pr_err( "[muic] %s err detach_charger(%d)\n",
			__func__, ret);

	ret = com_to_open(muic_data);
	if (ret)
		return ret;

	muic_data->attached_dev = ATTACHED_DEV_NONE_MUIC;

	return ret;
}

static int attach_jig_uart_boot_off(struct s2mu005_muic_data *muic_data,
				muic_attached_dev_t new_dev)
{
	struct muic_platform_data *pdata = muic_data->pdata;
	int ret = 0;

	pr_err("[muic] %s(%d)\n",
		__func__, new_dev);

	if (muic_data->is_rustproof){
		ret = set_com_sw(muic_data, MANSW_OPEN_WITH_VBUS);
		pr_err( "[muic] %s UART is disconnected\n", __func__);
	}else{
		if (pdata->uart_path == MUIC_PATH_UART_AP)
			ret = switch_to_ap_uart(muic_data);
		else
			ret = switch_to_cp_uart(muic_data);	
	}

	ret = attach_charger(muic_data, new_dev);

	return ret;
}

static int detach_jig_uart_boot_off(struct s2mu005_muic_data *muic_data)
{
	int ret = 0;

	pr_err("[muic] %s\n", __func__);

	ret = detach_charger(muic_data);
	if (ret)
		pr_err( "[muic] %s err detach_charger(%d)\n", __func__, ret);

	muic_data->attached_dev = ATTACHED_DEV_NONE_MUIC;

	return ret;
}

static int attach_jig_uart_boot_on(struct s2mu005_muic_data *muic_data,
				muic_attached_dev_t new_dev)
{
	int ret = 0;

	pr_err("[muic] %s(%d)\n",
		__func__, new_dev);

	if(muic_data->is_rustproof){
		ret = set_com_sw(muic_data, MANSW_OPEN_WITH_VBUS);
		pr_err( "[muic] %s UART is disconnected\n", __func__);
	}else{
		ret = set_com_sw(muic_data, MANSW_UART);
	}

	if (ret)
		pr_err( "[muic] %s set_com_sw err\n", __func__);

	muic_data->attached_dev = ATTACHED_DEV_JIG_UART_ON_MUIC;

	return ret;
}

static int dettach_jig_uart_boot_on(struct s2mu005_muic_data *muic_data)
{
	pr_err("[muic] %s\n", __func__);

	muic_data->attached_dev = ATTACHED_DEV_NONE_MUIC;

	return 0;
}

static int attach_jig_usb_boot_off(struct s2mu005_muic_data *muic_data,
				u8 vbvolt)
{
	int ret = 0;

	if (muic_data->attached_dev == ATTACHED_DEV_JIG_USB_OFF_MUIC) {
		pr_err("[muic] %s duplicated\n", __func__);
		return ret;
	}

	pr_err("[muic] %s\n", __func__);

	ret = attach_usb_util(muic_data, ATTACHED_DEV_JIG_USB_OFF_MUIC);
	if (ret)
		return ret;

	return ret;
}

static int attach_jig_usb_boot_on(struct s2mu005_muic_data *muic_data,
				u8 vbvolt)
{
	int ret = 0;

	if (muic_data->attached_dev == ATTACHED_DEV_JIG_USB_ON_MUIC) {
		pr_err("[muic] %s duplicated\n", __func__);
		return ret;
	}

	pr_err("[muic] %s\n", __func__);

	ret = attach_usb_util(muic_data, ATTACHED_DEV_JIG_USB_ON_MUIC);
	if (ret)
		return ret;

	return ret;
}

static void s2mu005_muic_handle_attach(struct s2mu005_muic_data *muic_data,
			muic_attached_dev_t new_dev, int adc, u8 vbvolt)
{
	int ret = 0;
	bool noti = (new_dev != muic_data->attached_dev) ? true : false;

	muic_data->is_jig_on = false;
	pr_err("[muic] %s : muic_data->attached_dev: %d, new_dev: %d, muic_data->suspended: %d\n",
		__func__, muic_data->attached_dev, new_dev, muic_data->suspended);

	switch (muic_data->attached_dev) {
	case ATTACHED_DEV_USB_MUIC:
	case ATTACHED_DEV_CDP_MUIC:
	case ATTACHED_DEV_JIG_USB_OFF_MUIC:
	case ATTACHED_DEV_JIG_USB_ON_MUIC:
		if (new_dev != muic_data->attached_dev) {
			pr_err("[muic] %s new(%d)!=attached(%d)\n",
				__func__, new_dev, muic_data->attached_dev);
			ret = detach_usb(muic_data);
		}
		break;
	case ATTACHED_DEV_OTG_MUIC:
	/* OTG -> LANHUB, meaning TA is attached to LANHUB(OTG) */
		if (new_dev != muic_data->attached_dev) {
			pr_err("[muic] %s new(%d)!=attached(%d)",
				__func__, new_dev, muic_data->attached_dev);
			ret = detach_otg_usb(muic_data);
		}
		break;

	case ATTACHED_DEV_AUDIODOCK_MUIC:
		if (new_dev != muic_data->attached_dev) {
			pr_err("[muic] %s new(%d)!=attached(%d)\n",
				__func__, new_dev, muic_data->attached_dev);
			ret = detach_audiodock(muic_data);
		}
		break;

	case ATTACHED_DEV_CHARGING_CABLE_MUIC:
	case ATTACHED_DEV_TA_MUIC:
	case ATTACHED_DEV_UNDEFINED_CHARGING_MUIC:
	case ATTACHED_DEV_UNDEFINED_RANGE_MUIC:
		if (new_dev != muic_data->attached_dev) {
			pr_err("[muic] %s new(%d)!=attached(%d)\n",
				__func__, new_dev, muic_data->attached_dev);
			ret = detach_charger(muic_data);
		}
		break;

	case ATTACHED_DEV_JIG_UART_OFF_VB_OTG_MUIC:
	case ATTACHED_DEV_JIG_UART_OFF_VB_FG_MUIC:
	case ATTACHED_DEV_JIG_UART_OFF_VB_MUIC:
	case ATTACHED_DEV_JIG_UART_OFF_MUIC:
		if (new_dev != ATTACHED_DEV_JIG_UART_OFF_MUIC) {
			pr_err("[muic] %s new(%d)!=attached(%d)\n",
				__func__, new_dev, muic_data->attached_dev);
			ret = detach_jig_uart_boot_off(muic_data);
		}
		break;

	case ATTACHED_DEV_JIG_UART_ON_MUIC:
		if (new_dev != muic_data->attached_dev) {
			pr_err("[muic] %s new(%d)!=attached(%d)\n",
				__func__, new_dev, muic_data->attached_dev);

			if(muic_data->is_factory_start) {
				ret = detach_jig_uart_boot_off(muic_data);
//				ret = detach_deskdock(muic_data);
			} else
				ret = dettach_jig_uart_boot_on(muic_data);

			muic_set_wakeup_noti(muic_data->is_factory_start ? 1: 0);
		}
		break;
	case ATTACHED_DEV_DESKDOCK_MUIC:
	case ATTACHED_DEV_DESKDOCK_VB_MUIC:
		if (new_dev != muic_data->attached_dev) {
			pr_err("[muic] %s new(%d)!=attached(%d)\n",
				__func__, new_dev, muic_data->attached_dev);

			if (new_dev == ATTACHED_DEV_DESKDOCK_MUIC ||
				new_dev == ATTACHED_DEV_DESKDOCK_VB_MUIC)
				noti = false;
			else
				ret = detach_deskdock(muic_data);
		}
		break;
	default:
		break;
	}

#if defined(CONFIG_MUIC_NOTIFIER)
	if (noti) {
		if (!muic_data->suspended)
			muic_notifier_detach_attached_dev(muic_data->attached_dev);
		else
			muic_data->need_to_noti = true;
	}
#endif /* CONFIG_MUIC_NOTIFIER */

	noti = true;
	switch (new_dev) {
	case ATTACHED_DEV_USB_MUIC:
	case ATTACHED_DEV_CDP_MUIC:
		ret = attach_usb(muic_data, new_dev);
		break;
	case ATTACHED_DEV_OTG_MUIC:
		ret = attach_otg_usb(muic_data, new_dev);
		break;
	case ATTACHED_DEV_AUDIODOCK_MUIC:
		ret = attach_audiodock(muic_data, new_dev, vbvolt);
		break;
	case ATTACHED_DEV_CHARGING_CABLE_MUIC:
	case ATTACHED_DEV_TA_MUIC:
	case ATTACHED_DEV_UNDEFINED_CHARGING_MUIC:
	case ATTACHED_DEV_UNDEFINED_RANGE_MUIC:
		com_to_open_with_vbus(muic_data);
		ret = attach_charger(muic_data, new_dev);
		break;
	case ATTACHED_DEV_JIG_UART_OFF_VB_OTG_MUIC:
	case ATTACHED_DEV_JIG_UART_OFF_VB_FG_MUIC:
	case ATTACHED_DEV_JIG_UART_OFF_VB_MUIC:
	case ATTACHED_DEV_JIG_UART_OFF_MUIC:
		muic_data->is_jig_on = true;
		ret = attach_jig_uart_boot_off(muic_data, new_dev);
		break;
	case ATTACHED_DEV_JIG_UART_ON_MUIC:
		/* call attach_deskdock to wake up the device */
		muic_data->is_jig_on = true;
		if(muic_data->is_factory_start) {
			ret = attach_jig_uart_boot_off(muic_data, new_dev);
//			ret = attach_deskdock(muic_data, new_dev, vbvolt);
		} else
			ret = attach_jig_uart_boot_on(muic_data, new_dev);

		muic_set_wakeup_noti(muic_data->is_factory_start ? 1: 0);
		break;
	case ATTACHED_DEV_JIG_USB_OFF_MUIC:
		muic_data->is_jig_on = true;
		ret = attach_jig_usb_boot_off(muic_data, vbvolt);
		break;
	case ATTACHED_DEV_JIG_USB_ON_MUIC:
		muic_data->is_jig_on = true;
		ret = attach_jig_usb_boot_on(muic_data, vbvolt);
		break;
	case ATTACHED_DEV_DESKDOCK_MUIC:
	case ATTACHED_DEV_DESKDOCK_VB_MUIC:
		ret = attach_deskdock(muic_data, new_dev, vbvolt);
		break;
	case ATTACHED_DEV_UNKNOWN_MUIC:
		com_to_open_with_vbus(muic_data);
		ret = attach_charger(muic_data, new_dev);
		break;
	default:
		noti = false;
		pr_err("[muic] %s unsupported dev=%d, adc=0x%x, vbus=%c\n",
			__func__, new_dev, adc, (vbvolt ? 'O' : 'X'));
		break;
	}

#if !defined(CONFIG_MUIC_S2MU005_ENABLE_AUTOSW)
	ret = s2mu005_muic_jig_on(muic_data);
#endif

	if (ret)
		pr_err("[muic] %s something wrong %d (ERR=%d)\n",
			__func__, new_dev, ret);

	pr_err("%s:%s\n", MFD_DEV_NAME, __func__);


#if defined(CONFIG_MUIC_NOTIFIER)
	if (noti) {
		if (!muic_data->suspended)
			muic_notifier_attach_attached_dev(new_dev);
		else
			muic_data->need_to_noti = true;
	}
#endif /* CONFIG_MUIC_NOTIFIER */
}

static void s2mu005_muic_handle_detach(struct s2mu005_muic_data *muic_data)
{
	int ret = 0;
	bool noti = true;
//	muic_data->is_jig_on = false;
	ret = com_to_open(muic_data);

	switch (muic_data->attached_dev) {
	case ATTACHED_DEV_JIG_USB_OFF_MUIC:
	case ATTACHED_DEV_JIG_USB_ON_MUIC:
	case ATTACHED_DEV_USB_MUIC:
	case ATTACHED_DEV_CDP_MUIC:
		ret = detach_usb(muic_data);
		break;
	case ATTACHED_DEV_OTG_MUIC:
		ret = detach_otg_usb(muic_data);
		break;
	case ATTACHED_DEV_CHARGING_CABLE_MUIC:
	case ATTACHED_DEV_TA_MUIC:
	case ATTACHED_DEV_UNDEFINED_CHARGING_MUIC:
	case ATTACHED_DEV_UNDEFINED_RANGE_MUIC:
		ret = detach_charger(muic_data);
		break;
	case ATTACHED_DEV_JIG_UART_OFF_VB_OTG_MUIC:
	case ATTACHED_DEV_JIG_UART_OFF_VB_FG_MUIC:
	case ATTACHED_DEV_JIG_UART_OFF_VB_MUIC:
	case ATTACHED_DEV_JIG_UART_OFF_MUIC:
		ret = detach_jig_uart_boot_off(muic_data);
		break;
	case ATTACHED_DEV_JIG_UART_ON_MUIC:
		if(muic_data->is_factory_start) {
			ret = detach_jig_uart_boot_off(muic_data);
//			ret = detach_deskdock(muic_data);
		} else
			ret = dettach_jig_uart_boot_on(muic_data);

		muic_set_wakeup_noti(muic_data->is_factory_start ? 1: 0);
		break;
	case ATTACHED_DEV_DESKDOCK_MUIC:
	case ATTACHED_DEV_DESKDOCK_VB_MUIC:
		ret = detach_deskdock(muic_data);
		break;
	case ATTACHED_DEV_AUDIODOCK_MUIC:
		ret = detach_audiodock(muic_data);
		break;
	case ATTACHED_DEV_NONE_MUIC:
		pr_err("[muic] %s duplicated(NONE)\n", __func__);
		break;
	case ATTACHED_DEV_UNKNOWN_MUIC:
		pr_err("[muic] %s UNKNOWN\n", __func__);
		ret = detach_charger(muic_data);
		muic_data->attached_dev = ATTACHED_DEV_NONE_MUIC;
		break;
	default:
		noti = false;
		pr_err("[muic] %s invalid type(%d)\n",
			__func__, muic_data->attached_dev);
		muic_data->attached_dev = ATTACHED_DEV_NONE_MUIC;
		break;
	}
	if (ret)
		pr_err("[muic] %s something wrong %d (ERR=%d)\n",
			__func__, muic_data->attached_dev, ret);

#if defined(CONFIG_MUIC_NOTIFIER)
	if (noti) {
		if (!muic_data->suspended)
			muic_notifier_detach_attached_dev(muic_data->attached_dev);
		else
			muic_data->need_to_noti = true;
	}
#endif /* CONFIG_MUIC_NOTIFIER */
}

static void s2mu005_muic_detect_dev(struct s2mu005_muic_data *muic_data)
{
	struct i2c_client *i2c = muic_data->i2c;
	muic_attached_dev_t new_dev = ATTACHED_DEV_UNKNOWN_MUIC;
	int twin_mdev = 0;
	int intr = MUIC_INTR_DETACH;
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

	pr_err("[muic] %s dev[1:0x%x, 2:0x%x, 3:0x%x], adc:0x%x, vbvolt:0x%x, "
		"apple:0x%x, chg_type:0x%x, vmid:0x%x, dev_id:0x%x\n",
		__func__, val1, val2, val3, adc, vbvolt, val5, val6, vmid, val4);

	if (ADC_CONVERSION_MASK & adc) {
		pr_err("[muic] ADC conversion error!\n");
		return ;
	}

	/* Work-Around for EVT0 : only if VBUS connected, then apply USB */
	if(muic_data->muic_version == 0) {
		if (vbvolt) {
			intr = MUIC_INTR_ATTACH;
			new_dev = ATTACHED_DEV_USB_MUIC;
			pr_err("[muic] EVT0 Work-around USB DETECTED\n");
		}
	}

	switch (val1) {
	case DEV_TYPE1_CDP:
		if(!vbvolt)	
			break;
		intr = MUIC_INTR_ATTACH;
		new_dev = ATTACHED_DEV_CDP_MUIC;
		pr_err("[muic] USB_CDP DETECTED\n");
		break;
	case DEV_TYPE1_USB:
		if(!vbvolt)	
			break;
		intr = MUIC_INTR_ATTACH;
		new_dev = ATTACHED_DEV_USB_MUIC;
		pr_err("[muic] USB DETECTED\n");
		break;
	case DEV_TYPE1_DEDICATED_CHG:
		if(!vbvolt)	
			break;
		intr = MUIC_INTR_ATTACH;
		new_dev = ATTACHED_DEV_TA_MUIC;
		pr_err("[muic] DEDICATED CHARGER DETECTED\n");
		break;
	case DEV_TYPE1_USB_OTG:
		intr = MUIC_INTR_ATTACH;
		new_dev = ATTACHED_DEV_OTG_MUIC;
		pr_err("[muic] USB_OTG DETECTED\n");
		break;
	case DEV_TYPE1_T1_T2_CHG:
		if(!vbvolt)	
			break;
		intr = MUIC_INTR_ATTACH;
		/* 200K, 442K should be checkef */
		if (ADC_CEA936ATYPE2_CHG == adc)
			new_dev = ATTACHED_DEV_TA_MUIC;
		else
			new_dev = ATTACHED_DEV_USB_MUIC;
		break;
	default:
		break;
	}

	switch (val2) {
	case DEV_TYPE2_SDP_1P8S:
		intr = MUIC_INTR_ATTACH;
		new_dev = ATTACHED_DEV_USB_MUIC;
		pr_err("[muic] SDP_1P8S DETECTED\n");
		break;
	case DEV_TYPE2_JIG_UART_OFF:
		intr = MUIC_INTR_ATTACH;
		if (muic_data->is_otg_test) {
			mdelay(1000);
			val7 = s2mu005_i2c_read_byte(i2c, S2MU005_REG_SC_STATUS2);
			vmid = val7 & 0x7;
			pr_err("%s: vmid : 0x%x \n", __func__, vmid);
			if (vmid == 0x4) {
				pr_err("[muic] OTG_TEST DETECTED, vmid = %d\n", vmid);
				vbvolt = 1;
			}
		}
		new_dev = ATTACHED_DEV_JIG_UART_OFF_MUIC;
		pr_err("[muic] JIG_UART_OFF DETECTED\n");
		break;
	case DEV_TYPE2_JIG_USB_OFF:
		if (!vbvolt)
			break;
		intr = MUIC_INTR_ATTACH;
		new_dev = ATTACHED_DEV_JIG_USB_OFF_MUIC;
		pr_err("[muic] JIG_USB_OFF DETECTED\n");
		break;
	case DEV_TYPE2_JIG_USB_ON:
		if (!vbvolt)
			break;
		intr = MUIC_INTR_ATTACH;
		new_dev = ATTACHED_DEV_JIG_USB_ON_MUIC;
		pr_err("[muic] JIG_USB_ON DETECTED\n");
		break;

	case DEV_TYPE2_JIG_UART_ON:
		if (new_dev != ATTACHED_DEV_JIG_UART_ON_MUIC) {
			intr = MUIC_INTR_ATTACH;
			new_dev = ATTACHED_DEV_JIG_UART_ON_MUIC;
			pr_err("[muic] ADC JIG_UART_ON DETECTED\n");
		}
		break;
	default:
		break;
	}

	if(muic_data->muic_version == 0)
		goto evt0_skip;

	/* This is for Apple cables */
	if (vbvolt && ((val5 & DEV_TYPE_APPLE_APPLE2P4A_CHG) || (val5 & DEV_TYPE_APPLE_APPLE2A_CHG) || 
		(val5 & DEV_TYPE_APPLE_APPLE1A_CHG) || (val5 & DEV_TYPE_APPLE_APPLE0P5A_CHG))) {
		intr = MUIC_INTR_ATTACH;
		new_dev = ATTACHED_DEV_TA_MUIC;
		pr_err("[muic] APPLE_CHG DETECTED\n");
	}

	if ((val6 & DEV_TYPE_CHG_TYPE) &&
		vbvolt &&
		(new_dev == ATTACHED_DEV_UNKNOWN_MUIC)) {
		intr = MUIC_INTR_ATTACH;
		/* This is workaround for LG USB cable which has 219k ohm ID */
		if (adc == ADC_CEA936ATYPE1_CHG || adc == ADC_JIG_USB_OFF) {
			new_dev = ATTACHED_DEV_USB_MUIC;
			pr_err("[muic] TYPE1_CHARGER DETECTED (USB)\n");
		}
		else {
			new_dev = ATTACHED_DEV_TA_MUIC;
			pr_err("[muic] TYPE3_CHARGER DETECTED\n");
		}
	}

	if (val2 & DEV_TYPE2_AV || val3 & DEV_TYPE3_AV_WITH_VBUS) {
		intr = MUIC_INTR_ATTACH;
		new_dev = ATTACHED_DEV_DESKDOCK_MUIC;
		pr_err("[muic] DESKDOCK DETECTED\n");
	}

	/* If there is no matching device found using device type registers
		use ADC to find the attached device */
	if (new_dev == ATTACHED_DEV_UNKNOWN_MUIC) {
		switch (adc) {
		case ADC_RESERVED_VZW:
			intr = MUIC_INTR_ATTACH;
			new_dev = ATTACHED_DEV_VZW_ACC_MUIC;
			pr_err("[muic] ADC VZW ACC DETECTED\n");
			break;
		case ADC_INCOMPATIBLE_VZW:
			intr = MUIC_INTR_ATTACH;
			new_dev = ATTACHED_DEV_VZW_INCOMPATIBLE_MUIC;
			pr_err("[muic] ADC INCOMPATIBLE VZW DETECTED\n");
			break;
		case ADC_HMT:
			intr = MUIC_INTR_ATTACH;
			new_dev = ATTACHED_DEV_HMT_MUIC;
			pr_err("[muic] ADC HMT DETECTED\n");
			break;
		case ADC_USB_LANHUB:
			intr = MUIC_INTR_ATTACH;
			new_dev = ATTACHED_DEV_USB_LANHUB_MUIC;
			pr_err("[muic] ADC LANHUB DETECTED\n");
			break;
		case ADC_CHARGING_CABLE:
			intr = MUIC_INTR_ATTACH;
			new_dev = ATTACHED_DEV_CHARGING_CABLE_MUIC;
			pr_err("[muic] ADC PS CABLE DETECTED\n");
			break;
		case ADC_UNIVERSAL_MMDOCK:
			intr = MUIC_INTR_ATTACH;
			new_dev = ATTACHED_DEV_UNIVERSAL_MMDOCK_MUIC;
			pr_err("[muic] ADC UNIVERSAL MMDOCK DETECTED\n");
			break;
		case ADC_CEA936ATYPE1_CHG: /*200k ohm */
			if(!vbvolt)	
				break;
			intr = MUIC_INTR_ATTACH;
			/* This is workaournd for LG USB cable
					which has 219k ohm ID */
			new_dev = ATTACHED_DEV_USB_MUIC;
			pr_err("[muic] TYPE1 CHARGER DETECTED(USB)\n");
			break;
		case ADC_CEA936ATYPE2_CHG:
			if(!vbvolt)	
				break;
			intr = MUIC_INTR_ATTACH;
			new_dev = ATTACHED_DEV_TA_MUIC;
			pr_err("[muic] TYPE1/2 CHARGER DETECTED(TA)\n");
			break;
		case ADC_DESKDOCK:
			if(!vbvolt)	
				break;
			intr = MUIC_INTR_ATTACH;
			new_dev = ATTACHED_DEV_DESKDOCK_MUIC;
			pr_err("[muic] ADC DESKDOCK DETECTED(TA)\n");
			break;
		case ADC_JIG_USB_OFF: /* 255k */
			if (!vbvolt)
				break;
			if (new_dev != ATTACHED_DEV_JIG_USB_OFF_MUIC) {
				intr = MUIC_INTR_ATTACH;
				new_dev = ATTACHED_DEV_JIG_USB_OFF_MUIC;
				pr_err("[muic] ADC JIG_USB_OFF DETECTED\n");
			}
			break;
		case ADC_JIG_USB_ON:
			if (!vbvolt)
				break;
			if (new_dev != ATTACHED_DEV_JIG_USB_ON_MUIC) {
				intr = MUIC_INTR_ATTACH;
				new_dev = ATTACHED_DEV_JIG_USB_ON_MUIC;
				pr_err("[muic] ADC JIG_USB_ON DETECTED\n");
			}
			break;
		case ADC_JIG_UART_OFF:
			intr = MUIC_INTR_ATTACH;
			if (muic_data->is_otg_test) {
				mdelay(1000);
				val7 = s2mu005_i2c_read_byte(i2c, S2MU005_REG_SC_STATUS2);
				vmid = val7 & 0x7;
				pr_err("%s: vmid : 0x%x \n", __func__, vmid);
				if (vmid == 0x4) {
					pr_err("[muic] OTG_TEST DETECTED, vmid = %d\n", vmid);
					vbvolt = 1;
				}
			}
			new_dev = ATTACHED_DEV_JIG_UART_OFF_MUIC;
			pr_err("[muic] ADC JIG_UART_OFF DETECTED\n");
			break;
		case ADC_JIG_UART_ON:
			if (new_dev != ATTACHED_DEV_JIG_UART_ON_MUIC) {
				intr = MUIC_INTR_ATTACH;
				new_dev = ATTACHED_DEV_JIG_UART_ON_MUIC;
				pr_err("[muic] ADC JIG_UART_ON DETECTED\n");
			}
			break;
		case ADC_AUDIODOCK:
			intr = MUIC_INTR_ATTACH;
			new_dev = ATTACHED_DEV_AUDIODOCK_MUIC;
			pr_err("[muic] ADC AUDIODOCK DETECTED\n");
			break;
		case ADC_OPEN:
			/* sometimes muic fails to
				catch JIG_UART_OFF detaching */
			/* double check with ADC */
			if (new_dev == ATTACHED_DEV_JIG_UART_OFF_MUIC) {
				new_dev = ATTACHED_DEV_UNKNOWN_MUIC;
				intr = MUIC_INTR_DETACH;
				pr_err("[muic] ADC OPEN DETECTED\n");
			}
			break;
		default:
			pr_err("[muic] %s unsupported ADC(0x%02x)\n",
				__func__, adc);
			if (vbvolt) {
				intr = MUIC_INTR_ATTACH;
				new_dev = ATTACHED_DEV_UNDEFINED_CHARGING_MUIC;
				pr_info("[muic] UNDEFINED VB DETECTED\n");
			} else
				intr = MUIC_INTR_DETACH;
			break;
		}
	}
	
evt0_skip:
	/* Check it the cable type is supported. */
	if (vps_is_supported_dev(new_dev)) {
		if ((twin_mdev = resolve_twin_mdev(muic_data, new_dev, vbvolt))) {
			new_dev = twin_mdev;
			pr_info("%s:Supported twin mdev-> %d\n", __func__, twin_mdev);
		} else
			pr_info("%s:Supported.\n", __func__);
	} else if (vbvolt && (intr == MUIC_INTR_ATTACH)) {
		new_dev = ATTACHED_DEV_UNDEFINED_CHARGING_MUIC;
		pr_info("%s:Unsupported->UNDEFINED_CHARGING\n", __func__);
	} else {
		if(vbvolt){
			/* S/W WA to ignore detach interrupt when vbus exists */	
			pr_info("%s:[WA]Ignore this Interrupt\n", __func__);
			return;			
		}
		else{
			intr = MUIC_INTR_DETACH;
			new_dev = ATTACHED_DEV_UNKNOWN_MUIC;
			pr_info("%s:Unsupported->Discarded.\n", __func__);
		}
	}

	/* Check undefined range */
	if (muic_data->undefined_range) {
		if (vbvolt && (intr == MUIC_INTR_ATTACH)
			&& mdev_undefined_range(adc)) {
			new_dev = ATTACHED_DEV_UNDEFINED_RANGE_MUIC;
			pr_info("%s:Undefined range ADC(0x%02x)\n", __func__, adc);
		}
	}

	if (intr == MUIC_INTR_ATTACH)
		s2mu005_muic_handle_attach(muic_data, new_dev, adc, vbvolt);
	else
		s2mu005_muic_handle_detach(muic_data);

#if defined(CONFIG_VBUS_NOTIFIER)
	vbus_notifier_handle((!!vbvolt) ? STATUS_VBUS_HIGH : STATUS_VBUS_LOW);
#endif /* CONFIG_VBUS_NOTIFIER */

}

static int s2mu005_muic_reg_init(struct s2mu005_muic_data *muic_data)
{
	struct i2c_client *i2c = muic_data->i2c;
	int ret;
	int val1, val2, val3, adc;

	pr_err("[muic] %s\n", __func__);

#if 0
	ret = s2mu005_i2c_read_byte(i2c, S2MU005_MUIC_REG_DEVID);
	if (ret == 0x18)
		muic_data->rev_id = 2;
	else
		muic_data->rev_id = 0;
#endif

	val1 = s2mu005_i2c_read_byte(i2c, S2MU005_REG_MUIC_DEVICE_TYPE1);
	val2 = s2mu005_i2c_read_byte(i2c, S2MU005_REG_MUIC_DEVICE_TYPE2);
	val3 = s2mu005_i2c_read_byte(i2c, S2MU005_REG_MUIC_DEVICE_TYPE3);
	adc = s2mu005_i2c_read_byte(i2c, S2MU005_REG_MUIC_ADC);
	pr_err("[muic] dev[1:0x%x, 2:0x%x, 3:0x%x], adc:0x%x\n", val1, val2, val3, adc);

	ret = s2mu005_i2c_guaranteed_wbyte(i2c,
			S2MU005_REG_MUIC_CTRL1, CTRL_MASK);
	if (ret < 0)
		pr_err( "[muic] failed to write ctrl(%d)\n", ret);

	return ret;
}

static irqreturn_t s2mu005_muic_irq_thread(int irq, void *data)
{
	struct s2mu005_muic_data *muic_data = data;
	struct i2c_client *i2c = muic_data->i2c;
	enum s2mu005_reg_manual_sw_value reg_val;
	int ctrl, ret = 0;

	pr_err("%s:%s\n", MFD_DEV_NAME, __func__);


	mutex_lock(&muic_data->muic_mutex);
	wake_lock(&muic_data->wake_lock);

	/* check for muic reset and re-initialize registers */
	ctrl = s2mu005_i2c_read_byte(i2c, S2MU005_REG_MUIC_CTRL1);

	if (ctrl == 0xDF) {
		/* CONTROL register is reset to DF */
#ifdef DEBUG_MUIC
		s2mu005_print_reg_log();
		s2mu005_print_reg_dump(muic_data);
#endif
		pr_err("[muic] %s: err muic could have been reseted. Initilize!!\n",
			__func__);
		s2mu005_muic_reg_init(muic_data);
		if (muic_data->is_rustproof) {
			pr_err("[muic] %s: rustproof is enabled\n", __func__);
			reg_val = MANSW_OPEN_WITH_VBUS | MANUAL_SW_OTGEN;
			ret = s2mu005_i2c_guaranteed_wbyte(i2c,
				S2MU005_REG_MUIC_SW_CTRL, reg_val);
			if (ret < 0)
				pr_err("[muic] %s: err write MANSW\n", __func__);
			ret = s2mu005_i2c_read_byte(i2c, S2MU005_REG_MUIC_SW_CTRL);
			pr_err("[muic] %s: MUIC_SW_CTRL=0x%x\n ", __func__, ret);
		} else {
			reg_val = MANSW_UART | MANUAL_SW_OTGEN;
			ret = s2mu005_i2c_guaranteed_wbyte(i2c,
				S2MU005_REG_MUIC_SW_CTRL, reg_val);
			if (ret < 0)
				pr_err("[muic] %s: err write MANSW\n", __func__);
			ret = s2mu005_i2c_read_byte(i2c, S2MU005_REG_MUIC_SW_CTRL);
			pr_err("[muic] %s: MUIC_SW_CTRL=0x%x\n ", __func__, ret);
		}
#ifdef DEBUG_MUIC
		s2mu005_print_reg_dump(muic_data);
#endif
		/* MUIC Interrupt On */
		set_int_mask(muic_data, false);
#if defined(CONFIG_SEC_FACTORY) && defined(CONFIG_USB_HOST_NOTIFY)
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

	pr_err("[muic] %s\n", __func__);

	dev_id = s2mu005_i2c_read_byte(muic_data->i2c, S2MU005_REG_REV_ID);
	if (dev_id < 0) {
		pr_err( "[muic] %s(%d)\n", __func__, dev_id);
		ret = -ENODEV;
	} else {
		muic_data->muic_vendor = 0x05;
		muic_data->muic_version = (dev_id & 0x0F);
		pr_err( "[muic] %s : vendor=0x%x, ver=0x%x, dev_id=0x%x\n",
			__func__, muic_data->muic_vendor,
			muic_data->muic_version, dev_id);
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

		muic_data->irq_rid_chg = irq_base + S2MU005_MUIC_IRQ1_RID_CHG;
		REQUEST_IRQ(muic_data->irq_rid_chg, muic_data, "muic-rid_chg");

		muic_data->irq_vbus_on = irq_base + S2MU005_MUIC_IRQ2_VBUS_ON;
		REQUEST_IRQ(muic_data->irq_vbus_on, muic_data, "muic-vbus_on");

		muic_data->irq_rsvd_attach = irq_base + S2MU005_MUIC_IRQ2_RSVD_ATTACH;
		REQUEST_IRQ(muic_data->irq_rsvd_attach, muic_data, "muic-rsvd_attach");

		muic_data->irq_adc_change = irq_base + S2MU005_MUIC_IRQ2_ADC_CHANGE;
		REQUEST_IRQ(muic_data->irq_adc_change, muic_data, "muic-adc_change");

		muic_data->irq_av_charge = irq_base + S2MU005_MUIC_IRQ2_AV_CHARGE;
		REQUEST_IRQ(muic_data->irq_av_charge, muic_data, "muic-av_charge");

		muic_data->irq_vbus_off = irq_base + S2MU005_MUIC_IRQ2_VBUS_OFF;
		REQUEST_IRQ(muic_data->irq_vbus_off, muic_data, "muic-vbus_off");

	}

	pr_err("%s:%s\n", MFD_DEV_NAME, __func__);
	pr_err("%s:%s muic-attach(%d), muic-detach(%d), muic-rid_chg(%d), muic-vbus_on(%d)",
		MUIC_DEV_NAME, __func__, muic_data->irq_attach,	muic_data->irq_detach, muic_data->irq_rid_chg,	\
			muic_data->irq_vbus_on);
	pr_err("muic-rsvd_attach(%d), muic-adc_change(%d), muic-av_charge(%d), muic-vbus_off(%d)\n",
		muic_data->irq_rsvd_attach, muic_data->irq_adc_change, muic_data->irq_av_charge, muic_data->irq_vbus_off);

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
	FREE_IRQ(muic_data->irq_rid_chg, muic_data, "muic-rid_chg");
	FREE_IRQ(muic_data->irq_vbus_on, muic_data, "muic-vbus_on");
	FREE_IRQ(muic_data->irq_rsvd_attach, muic_data, "muic-rsvd_attach");
	FREE_IRQ(muic_data->irq_adc_change, muic_data, "muic-adc_change");
	FREE_IRQ(muic_data->irq_av_charge, muic_data, "muic-av_charge");
	FREE_IRQ(muic_data->irq_vbus_off, muic_data, "muic-vbus_off");
}

#if defined(CONFIG_OF)
int of_update_supported_list(struct s2mu005_muic_data *pdata)
{
	struct device_node *np_muic;
	const char *prop, *prop_end;
	char prop_buf[64];
	int i, prop_num;
	int ret = 0;
	int mdev = 0;

	pr_info("%s\n", __func__);

	np_muic = of_find_node_by_path("/muic");
	if (np_muic == NULL)
		return -EINVAL;

	prop_num = of_property_count_strings(np_muic, "muic,support-list");
	if (prop_num < 0) {
		pr_warn("%s:%s No 'support list dt node'[%d]\n",
				MUIC_DEV_NAME, __func__, prop_num);
		ret = prop_num;
		goto err;
	}

	memset(prop_buf, 0x00, sizeof(prop_buf));

	for (i = 0; i < prop_num; i++) {
		ret = of_property_read_string_index(np_muic, "muic,support-list", i,
							&prop);
		if (ret) {
			pr_err("%s:%s Cannot find string at [%d], ret[%d]\n",
					MUIC_DEV_NAME, __func__, i, ret);
			break;
		}

		prop_end = strstr(prop, ":");
		if (!prop_end) {
			pr_err("%s: Wrong prop format. %s\n", __func__, prop);
			break;
		}

		memcpy(prop_buf, prop, prop_end - prop);
		prop_buf[prop_end - prop] = 0x00;

		if (prop_buf[0] == '+') {
			if (vps_name_to_mdev(&prop_buf[1], &mdev))
				vps_update_supported_attr(mdev, true);
		} else if (prop_buf[0] == '-') {
			if (vps_name_to_mdev(&prop_buf[1], &mdev))
				vps_update_supported_attr(mdev, false);
		} else {
			pr_err("%s: %c Undefined prop attribute.\n", __func__, prop_buf[0]);
		}
	}
#if 0
	prop_num = of_property_count_strings(np_muic, "muic,support-list-variant");
	if (prop_num < 0) {
		pr_warn("%s:%s No support list-variant dt node'[%d]\n",
				MUIC_DEV_NAME, __func__, prop_num);
		ret = prop_num;
		goto err;
	}

	memset(prop_buf, 0x00, sizeof(prop_buf));

	for (i = 0; i < prop_num; i++) {
		ret = of_property_read_string_index(np_muic, "muic,support-list-variant", i,
							&prop);
		if (ret) {
			pr_err("%s:%s Cannot find variant-string at [%d], ret[%d]\n",
					MUIC_DEV_NAME, __func__, i, ret);
			break;
		}

		prop_end = strstr(prop, ":");
		if (!prop_end) {
			pr_err("%s: Wrong prop format. %s\n", __func__, prop);
			break;
		}

		memcpy(prop_buf, prop, prop_end - prop);
		prop_buf[prop_end - prop] = 0x00;

		if (prop_buf[0] == '+') {
			if (vps_name_to_mdev(&prop_buf[1], &mdev))
				vps_update_supported_attr(mdev, true);
		} else if (prop_buf[0] == '-') {
			if (vps_name_to_mdev(&prop_buf[1], &mdev))
				vps_update_supported_attr(mdev, false);
		} else {
			pr_err("%s: %c Undefined prop attribute.\n", __func__, prop_buf[0]);
		}
	}
#endif

err:
	of_node_put(np_muic);

	return ret;
}

static int of_s2mu005_muic_dt(struct device *dev, struct s2mu005_muic_data *muic_data)
{
	struct device_node *np, *np_muic;
	int ret = 0;
	pr_info("%s:%s\n", MUIC_DEV_NAME, __func__);

	np = dev->parent->of_node;
	if (!np) {
		pr_err("[muic] %s : could not find np\n", __func__);
		return -ENODEV;
	}

	np_muic = of_find_node_by_name(np, "muic");
	if (!np_muic) {
		pr_err("[muic] %s : could not find muic sub-node np_muic\n", __func__);
		return -EINVAL;
	}

#if !defined(CONFIG_MUIC_UART_SWITCH)
	if (of_gpio_count(np_muic) < 1) {
		pr_err("[muic] %s : could not find muic gpio\n", __func__);
		muic_data->pdata->gpio_uart_sel = 0;
	} else
		muic_data->pdata->gpio_uart_sel = of_get_gpio(np_muic, 0);
#else
	muic_data->pdata->uart_addr =
		(const char *)of_get_property(np_muic, "muic,uart_addr", NULL);
	muic_data->pdata->uart_txd =
		(const char *)of_get_property(np_muic, "muic,uart_txd", NULL);
	muic_data->pdata->uart_rxd =
		(const char *)of_get_property(np_muic, "muic,uart_rxd", NULL);
#endif
	muic_data->undefined_range = of_property_read_bool(np_muic, "muic,undefined_range");
	pr_err("[muic] %s : muic,undefined_range[%s]\n", __func__, muic_data->undefined_range ? "T" : "F");

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

	pr_err("[muic] %s:%s\n", MFD_DEV_NAME, __func__);

	muic_data = kzalloc(sizeof(struct s2mu005_muic_data), GFP_KERNEL);
	if (!muic_data) {
		pr_err( "[muic] %s: failed to allocate driver data\n", __func__);
		ret = -ENOMEM;
		goto err_return;
	}

	if (!mfd_pdata) {
		pr_err("%s: failed to get s2mu005 mfd platform data\n", __func__);
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
		pr_err( "[muic] no muic dt! ret[%d]\n", ret);

	of_update_supported_list(muic_data);
	vps_show_table();
#endif /* CONFIG_OF */

	mutex_init(&muic_data->muic_mutex);
	muic_data->is_factory_start = false;
	muic_data->attached_dev = ATTACHED_DEV_UNKNOWN_MUIC;
	muic_data->is_usb_ready = false;
	platform_set_drvdata(pdev, muic_data);

#if !defined(CONFIG_MUIC_UART_SWITCH)
	if (muic_pdata.gpio_uart_sel)
#endif
		muic_pdata.set_gpio_uart_sel = s2mu005_set_gpio_uart_sel;

	if (muic_data->pdata->init_gpio_cb)
		ret = muic_data->pdata->init_gpio_cb();
	if (ret) {
		pr_err( "[muic] %s: failed to init gpio(%d)\n", __func__, ret);
		goto fail_init_gpio;
	}

	/* create sysfs group */
	ret = sysfs_create_group(&switch_device->kobj, &s2mu005_muic_group);
	if (ret) {
		pr_err( "[muic] failed to create sysfs\n");
		goto fail;
	}
	dev_set_drvdata(switch_device, muic_data);

	ret = s2mu005_init_rev_info(muic_data);
	if (ret) {
		pr_err( "[muic] failed to init muic(%d)\n", ret);
		goto fail;
	}

	ret = s2mu005_muic_reg_init(muic_data);
	if (ret) {
		pr_err( "[muic] failed to init muic(%d)\n", ret);
		goto fail;
	}

#if 0
	ret = s2mu005_i2c_read_byte(muic_data->i2c, S2MU005_REG_MUIC_SW_CTRL);
	if (ret < 0)
		pr_err( "[muic] %s: err MANSW (%d)\n", __func__, ret);
	else {
		/* RUSTPROOF : disable UART connection if MANSW
			from BL is OPEN_RUSTPROOF */
		if (ret == MANSW_OPEN_RUSTPROOF) {
			muic_data->is_rustproof = true;
			com_to_open_with_vbus(muic_data);
		} else {
			muic_data->is_rustproof = false;
		}
	}
#else
	muic_data->is_rustproof = muic_data->pdata->rustproof_on;
	if (muic_data->is_rustproof) {
		pr_err( "[muic] %s rustproof is enabled\n", __func__);
		com_to_open_with_vbus(muic_data);
	}
#endif

	if (muic_data->pdata->init_switch_dev_cb)
		muic_data->pdata->init_switch_dev_cb();

	ret = s2mu005_muic_irq_init(muic_data);
	if (ret) {
		pr_err( "[muic] %s: failed to init irq(%d)\n", __func__, ret);
		goto fail_init_irq;
	}

	wake_lock_init(&muic_data->wake_lock, WAKE_LOCK_SUSPEND, "muic_wake");

	/* initial cable detection */
	set_int_mask(muic_data, false);
#if defined(CONFIG_SEC_FACTORY) && defined(CONFIG_USB_HOST_NOTIFY)
	init_otg_reg(muic_data);
#endif
	s2mu005_muic_irq_thread(-1, muic_data);

	return 0;

fail_init_irq:
	s2mu005_muic_free_irqs(muic_data);
fail:
#ifdef CONFIG_SEC_SYSFS
	sysfs_remove_group(&switch_device->kobj, &s2mu005_muic_group);
#endif
	mutex_destroy(&muic_data->muic_mutex);
fail_init_gpio:
err_kfree:
	kfree(muic_data);
err_return:
	return ret;
}

static int s2mu005_muic_remove(struct platform_device *pdev)
{
	struct s2mu005_muic_data *muic_data = platform_get_drvdata(pdev);
#ifdef CONFIG_SEC_SYSFS
	sysfs_remove_group(&switch_device->kobj, &s2mu005_muic_group);
#endif

	if (muic_data) {
		pr_err("[muic] %s\n", __func__);
		disable_irq_wake(muic_data->i2c->irq);
		s2mu005_muic_free_irqs(muic_data);
		mutex_destroy(&muic_data->muic_mutex);
		i2c_set_clientdata(muic_data->i2c, NULL);
		kfree(muic_data);
	}
	return 0;
}

static void s2mu005_muic_shutdown(struct device *dev)
{
	struct s2mu005_muic_data *muic_data = dev_get_drvdata(dev);
	int ret;

	pr_err("[muic] %s\n", __func__);
	if (!muic_data->i2c) {
		pr_err( "[muic] %s no muic i2c client\n", __func__);
		return;
	}

	pr_err("[muic] open D+,D-,V_bus line\n");
	ret = com_to_open(muic_data);
	if (ret < 0)
		pr_err( "[muic] fail to open mansw\n");

	/* set auto sw mode before shutdown to make sure device goes into */
	/* LPM charging when TA or USB is connected during off state */
	pr_err("[muic] muic auto detection enable\n");
	ret = set_manual_sw(muic_data, true);
	if (ret < 0) {
		pr_err( "[muic] %s fail to update reg\n", __func__);
		return;
	}
}

#if defined CONFIG_PM
static int s2mu005_muic_suspend(struct device *dev)
{
	struct s2mu005_muic_data *muic_data = dev_get_drvdata(dev);

	muic_data->suspended = true;

	return 0;
}

static int s2mu005_muic_resume(struct device *dev)
{
	struct s2mu005_muic_data *muic_data = dev_get_drvdata(dev);

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
		.shutdown = s2mu005_muic_shutdown,
	},
	.probe = s2mu005_muic_probe,
	.remove = s2mu005_muic_remove,
};

static int __init s2mu005_muic_init(void)
{
	return platform_driver_register(&s2mu005_muic_driver);
}
module_init(s2mu005_muic_init);

static void __exit s2mu005_muic_exit(void)
{
	platform_driver_unregister(&s2mu005_muic_driver);
}
module_exit(s2mu005_muic_exit);

MODULE_DESCRIPTION("S2MU005 USB Switch driver");
MODULE_LICENSE("GPL");
