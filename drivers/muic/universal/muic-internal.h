/*
 * Copyright (C) 2010 Samsung Electronics
 * Thomas Ryu <smilesr.ryu@samsung.com>
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

#ifndef __MUIC_INTERNAL_H__
#define __MUIC_INTERNAL_H__

#include <linux/muic/muic.h>
#include <linux/mutex.h>
#if defined(CONFIG_MUIC_SUPPORT_EARJACK)
#include <linux/input.h>
#endif

#define MUIC_DEV_NAME   "muic-universal"

enum muic_op_mode {
	OPMODE_MUIC = 0<<0,
	OPMODE_CCIC = 1<<0,
};

/* Slave addr = 0x4A: MUIC */
enum ioctl_cmd {
	GET_COM_VAL = 0x01,
	GET_CTLREG = 0x02,
	GET_ADC = 0x03,
	GET_SWITCHING_MODE = 0x04,
	GET_INT_MASK = 0x05,
	GET_REVISION = 0x06,
	GET_OTG_STATUS = 0x7,
	GET_CHGTYPE = 0x08,
	GET_RESID3 = 0x09,
};

enum switching_mode{
	SWMODE_MANUAL =0,
	SWMODE_AUTO = 1,
};

#if defined(CONFIG_MUIC_SUPPORT_EARJACK)
enum earjackkey_index {
	EARJACK_KEY_ERROR,
	EARJACK_PRESS_SEND_END,
	EARJACK_PRESS_VOL_UP,
	EARJACK_PRESS_VOL_DN,
	EARJACK_LONG_PRESS_SEND,
	EARJACK_LONG_PRESS_VOL_UP,
	EARJACK_LONG_PRESS_VOL_DN,
	EARJACK_LONG_RELEASE_SEND,
	EARJACK_LONG_RELEASE_VOL_UP,
	EARJACK_LONG_RELEASE_VOL_DN,
	EARJACK_STUCK_KEY,
	EARJACK_NO_KEY,
};
#endif

/*
 * Manual Switch
 * D- [7:5] / D+ [4:2] / Vbus [1:0]
 * 000: Open all / 001: USB / 010: AUDIO / 011: UART / 100: V_AUDIO
 * 00: Vbus to Open / 01: Vbus to Charger / 10: Vbus to MIC / 11: Vbus to VBout
 */

/* COM port index */
enum com_index {
	COM_OPEN = 1,
	COM_OPEN_WITH_V_BUS = 2,
	COM_UART_AP = 3,
	COM_UART_CP = 4,
	COM_USB_AP  = 5,
	COM_USB_CP  = 6,
	COM_AUDIO   = 7,
};

enum{
	ADC_SCANMODE_CONTINUOUS = 0x0,
	ADC_SCANMODE_ONESHOT = 0x1,
	ADC_SCANMODE_PULSE = 0x2,
};

enum vps_type{
	VPS_TYPE_SCATTERED =0,
	VPS_TYPE_TABLE =1,
};

/* VPS data from a chip. */
typedef struct _muic_vps_scatterred_type {
        u8      val1;
        u8      val2;
        u8      val3;
        u8      adc;
        u8      vbvolt;
#if defined(CONFIG_SEC_DEBUG)
        u8      chgtyp;
#endif
}vps_scatterred_type;

typedef struct _muic_vps_table_t {
	u8  adc;
	u8  vbvolt;
	u8  adc1k;
	u8  adcerr;
	u8  adclow;
	u8  chgdetrun;
	u8  chgtyp;
	const char *vps_name;
	muic_attached_dev_t attached_dev;
	u8 control1;
}vps_table_type;

struct muic_intr_data {
	u8	intr1;
	u8	intr2;
	u8	intr3;
};

typedef union _muic_vps_t {
	vps_scatterred_type s;
	vps_table_type t;
	char vps_data[16];
}vps_data_t;

/* muic chip specific internal data structure
 * that setted at muic-xxxx.c file
 */
struct regmap_desc;

typedef struct _muic_data_t {

	struct device *dev;
	struct i2c_client *i2c; /* i2c addr: 0x4A; MUIC */
	struct mutex muic_mutex;

	/* model dependant muic platform data */
	struct muic_platform_data *pdata;

	/* muic current attached device */
	muic_attached_dev_t attached_dev;

	vps_data_t vps;
	int vps_table;

	struct muic_intr_data intr;

	/* regmap_desc_t */
	struct regmap_desc *regmapdesc;

	char *chip_name;

	int gpio_uart_sel;

	/* muic Device ID */
	u8 muic_vendor;			/* Vendor ID */
	u8 muic_version;		/* Version ID */

	bool			is_gamepad;
	bool			is_factory_start;
	bool			is_rustproof;
	bool			is_otg_test;
#if defined(CONFIG_MUIC_UNIVERSAL_MULTI_SUPPORT)
	bool			is_afc_support;
#endif
	struct delayed_work	init_work;
	struct delayed_work	usb_work;
#if defined(CONFIG_MUIC_UNIVERSAL_SM5705_AFC)
	int			max_afc_supported_volt;
	int			max_afc_supported_cur;
#endif
	bool			discard_interrupt;
	bool			is_dcdtmr_intr;
#if defined(CONFIG_USB_EXTERNAL_NOTIFY)
	/* USB Notifier */
	struct notifier_block	usb_nb;
#endif
#if defined(CONFIG_MUIC_SUPPORT_EARJACK)
	bool			is_earkeypressed;
	int			old_keycode;
	struct input_dev        *input;
	int			earkey_state;
#endif
#if defined(CONFIG_SEC_DEBUG)
	bool			usb_to_ta_state;
#endif

	int is_flash_on;
	int irq_n;
	int is_afc_device;
	struct delayed_work	afc_retry_work;
	struct delayed_work	afc_restart_work;
	struct delayed_work	afc_delay_check_work;
	int delay_check_count;
#if defined(CONFIG_MUIC_SUPPORT_CCIC)
	/* legacy TA or USB for CCIC */
	muic_attached_dev_t	legacy_dev;

	/* CCIC Notifier */
#ifdef CONFIG_USB_TYPEC_MANAGER_NOTIFIER
	struct notifier_block	manager_nb;
#else
	struct notifier_block	ccic_nb;
#endif

	struct delayed_work	ccic_work;

	/* Operation Mode */
	enum muic_op_mode	opmode;
	bool 			afc_water_disable;
#endif

#ifdef CONFIG_MUIC_SM5705_AFC_18W_TA_SUPPORT
    int is_18w_ta;
#endif

#ifdef CONFIG_MUIC_SM570X_SWITCH_CONTROL_GPIO
    int sm570x_switch_gpio;
#endif

	int muic_reset_count;
	struct mutex lock;
#if defined(CONFIG_MUIC_UNIVERSAL_SM5708)
	struct delayed_work	vdp_src_en_work;
#endif
}muic_data_t;

extern struct device *switch_device;

#if defined(CONFIG_SEC_DEBUG)
enum usb_to_ta_status_t {
	USB2TA_DISABLE,
	USB2TA_ENABLE,
	USB2TA_READ,
};
#endif

#endif /* __MUIC_INTERNAL_H__ */
