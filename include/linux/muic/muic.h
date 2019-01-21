/*
 * include/linux/muic/muic.h
 *
 * header file supporting MUIC common information
 *
 * Copyright (C) 2010 Samsung Electronics
 * Seoyoung Jeong <seo0.jeong@samsung.com>
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

#ifndef __MUIC_H__
#define __MUIC_H__

#ifdef CONFIG_IFCONN_NOTIFIER
#include <linux/ifconn/ifconn_notifier.h>
#endif

#define MUIC_CORE "MUIC_CORE"
/* Status of IF PMIC chip (suspend and resume) */
enum {
	MUIC_SUSPEND		= 0,
	MUIC_RESUME,
};

/* MUIC Interrupt */
enum {
	MUIC_INTR_DETACH	= 0,
	MUIC_INTR_ATTACH
};

/* MUIC Dock Observer Callback parameter */
enum {
	MUIC_DOCK_DETACHED	= 0,
	MUIC_DOCK_DESKDOCK	= 1,
	MUIC_DOCK_CARDOCK	= 2,
	MUIC_DOCK_AUDIODOCK	= 101,
	MUIC_DOCK_SMARTDOCK	= 102,
	MUIC_DOCK_HMT		= 105,
	MUIC_DOCK_ABNORMAL	= 106,
	MUIC_DOCK_GAMEPAD	= 107,
	MUIC_DOCK_GAMEPAD_WITH_EARJACK	= 108,
};

/* MUIC Path */
enum {
	MUIC_PATH_USB_AP	= 0,
	MUIC_PATH_USB_CP,
	MUIC_PATH_UART_AP,
	MUIC_PATH_UART_CP,
	MUIC_PATH_OPEN,
	MUIC_PATH_AUDIO,
};

/* bootparam SWITCH_SEL */
enum {
	SWITCH_SEL_USB_MASK	= 0x1,
	SWITCH_SEL_UART_MASK	= 0x2,
	SWITCH_SEL_RUSTPROOF_MASK	= 0x8,
	SWITCH_SEL_AFC_DISABLE_MASK	= 0x100,
};

/* bootparam CHARGING_MODE */
enum {
	CH_MODE_AFC_DISABLE_VAL = 0x31, /* char '1' */
};

/* MUIC ADC table */
typedef enum {
	ADC_GND			= 0x00,
	ADC_SEND_END		= 0x01, /* 0x00001 2K ohm */
	ADC_REMOTE_S1		= 0x02, /* 0x00010 2.604K ohm */
	ADC_REMOTE_S8		= 0x09, /* 0x01001 12.03K ohm */
	ADC_BUTTON_VOLDN	= 0x0a, /* 0x01010 14.46K ohm */
	ADC_BUTTON_VOLUP	= 0x0b, /* 0x01011 17.26K ohm */
	ADC_REMOTE_S11		= 0x0c, /* 0x01100 20.5K ohm */
	ADC_REMOTE_S12		= 0x0d, /* 0x01101 24.07K ohm */
	ADC_RESERVED_VZW	= 0x0e, /* 0x01110 28.7K ohm */
#ifdef CONFIG_MUIC_KEYBOARD
	ADC_POGO_CHARGER	= 0x0f, /* 0x01111 34K ohm */
#else
	ADC_INCOMPATIBLE_VZW	= 0x0f, /* 0x01111 34K ohm */
#endif
	ADC_SMARTDOCK		= 0x10, /* 0x10000 40.2K ohm */
	ADC_HMT			= 0x11, /* 0x10001 49.9K ohm */
	ADC_AUDIODOCK		= 0x12, /* 0x10010 64.9K ohm */
	ADC_USB_LANHUB		= 0x13, /* 0x10011 80.07K ohm */
	ADC_CHARGING_CABLE	= 0x14,	/* 0x10100 102K ohm */
	ADC_UNIVERSAL_MMDOCK	= 0x15, /* 0x10101 121K ohm */
	ADC_GAMEPAD		= 0x15, /* 0x10101 121K ohm */
	ADC_UART_CABLE		= 0x16, /* 0x10110 150K ohm */
	ADC_CEA936ATYPE1_CHG	= 0x17,	/* 0x10111 200K ohm */
	ADC_JIG_USB_OFF		= 0x18, /* 0x11000 255K ohm */
	ADC_JIG_USB_ON		= 0x19, /* 0x11001 301K ohm */
	ADC_DESKDOCK		= 0x1a, /* 0x11010 365K ohm */
	ADC_CEA936ATYPE2_CHG	= 0x1b, /* 0x11011 442K ohm */
	ADC_JIG_UART_OFF	= 0x1c, /* 0x11100 523K ohm */
	ADC_JIG_UART_ON		= 0x1d, /* 0x11101 619K ohm */
#ifdef CONFIG_MUIC_S2MU004
	ADC_AUDIOMODE_W_REMOTE	= 0x1e, /* 0x11110 1000K ohm */
#else
	ADC_EARJACK		= 0x1e,	/* 0x11110 1000k ohm*/
#endif
	ADC_OPEN		= 0x1f,
	ADC_OPEN_219		= 0xfb, /* ADC open or 219.3K ohm */
	ADC_219			= 0xfc, /* ADC open or 219.3K ohm */

	ADC_UNDEFINED		= 0xfd, /* Undefied range */
	ADC_DONTCARE		= 0xfe, /* ADC don't care for MHL */
	ADC_ERROR		= 0xff, /* ADC value read error */
} muic_adc_t;

/* MUIC attached device type */
typedef enum {
	ATTACHED_DEV_NONE_MUIC = 0,
	/* 1 */
	ATTACHED_DEV_USB_MUIC,
	ATTACHED_DEV_CDP_MUIC,
	ATTACHED_DEV_OTG_MUIC,
	ATTACHED_DEV_TA_MUIC,
	ATTACHED_DEV_UNOFFICIAL_MUIC,
	ATTACHED_DEV_UNOFFICIAL_TA_MUIC,
	ATTACHED_DEV_UNOFFICIAL_ID_MUIC,
	ATTACHED_DEV_UNOFFICIAL_ID_TA_MUIC,
	ATTACHED_DEV_UNOFFICIAL_ID_ANY_MUIC,
	ATTACHED_DEV_UNOFFICIAL_ID_USB_MUIC,
	/* 11 */
	ATTACHED_DEV_UNOFFICIAL_ID_CDP_MUIC,
	ATTACHED_DEV_UNDEFINED_CHARGING_MUIC,
	ATTACHED_DEV_DESKDOCK_MUIC,
	ATTACHED_DEV_UNKNOWN_VB_MUIC,
	ATTACHED_DEV_DESKDOCK_VB_MUIC,
	ATTACHED_DEV_CARDOCK_MUIC,
	ATTACHED_DEV_JIG_UART_START,
	ATTACHED_DEV_JIG_UART_OFF_MUIC,
	ATTACHED_DEV_JIG_UART_OFF_VB_MUIC,	/* VBUS enabled */
	ATTACHED_DEV_JIG_UART_OFF_VB_OTG_MUIC,	/* for otg test */
	/* 21 */
	ATTACHED_DEV_JIG_UART_OFF_VB_FG_MUIC,	/* for fuelgauge test */
	ATTACHED_DEV_JIG_UART_ON_MUIC,
	ATTACHED_DEV_JIG_UART_END,
	ATTACHED_DEV_JIG_USB_OFF_MUIC,
	ATTACHED_DEV_JIG_USB_ON_MUIC,
	ATTACHED_DEV_JIG_RID_OPEN_MUIC,	/* recovery factory mode 523k to open */
	ATTACHED_DEV_SMARTDOCK_MUIC,
	ATTACHED_DEV_SMARTDOCK_VB_MUIC,
	ATTACHED_DEV_SMARTDOCK_TA_MUIC,
	ATTACHED_DEV_SMARTDOCK_USB_MUIC,
	/* 31 */
	ATTACHED_DEV_UNIVERSAL_MMDOCK_MUIC,
	ATTACHED_DEV_AUDIODOCK_MUIC,
	ATTACHED_DEV_MHL_MUIC,
	ATTACHED_DEV_CHARGING_CABLE_MUIC,
	ATTACHED_DEV_AFC_CHARGER_PREPARE_MUIC,
	ATTACHED_DEV_AFC_CHARGER_PREPARE_DUPLI_MUIC,
	ATTACHED_DEV_AFC_CHARGER_5V_MUIC,
	ATTACHED_DEV_AFC_CHARGER_5V_DUPLI_MUIC,
	ATTACHED_DEV_AFC_CHARGER_9V_MUIC,
	ATTACHED_DEV_AFC_CHARGER_9V_DUPLI_MUIC,
	/* 41 */
	ATTACHED_DEV_AFC_CHARGER_9V_18W_MUIC,
	ATTACHED_DEV_AFC_CHARGER_12V_MUIC,
	ATTACHED_DEV_AFC_CHARGER_12V_DUPLI_MUIC,
	ATTACHED_DEV_AFC_CHARGER_ERR_V_MUIC,
	ATTACHED_DEV_AFC_CHARGER_ERR_V_DUPLI_MUIC,
	ATTACHED_DEV_QC_CHARGER_PREPARE_MUIC,
	ATTACHED_DEV_QC_CHARGER_5V_MUIC,
	ATTACHED_DEV_QC_CHARGER_ERR_V_MUIC,
	ATTACHED_DEV_QC_CHARGER_9V_MUIC,
	ATTACHED_DEV_HV_ID_ERR_UNDEFINED_MUIC,
	/* 51*/
	ATTACHED_DEV_HV_ID_ERR_UNSUPPORTED_MUIC,
	ATTACHED_DEV_HV_ID_ERR_SUPPORTED_MUIC,
	ATTACHED_DEV_HMT_MUIC,
	ATTACHED_DEV_VZW_ACC_MUIC,
	ATTACHED_DEV_VZW_INCOMPATIBLE_MUIC,
	ATTACHED_DEV_USB_LANHUB_MUIC,
	ATTACHED_DEV_TYPE1_CHG_MUIC,
	ATTACHED_DEV_TYPE2_CHG_MUIC,
	ATTACHED_DEV_TYPE3_MUIC,
	ATTACHED_DEV_TYPE3_MUIC_TA,
	/* 61 */
	ATTACHED_DEV_TYPE3_ADAPTER_MUIC,
	ATTACHED_DEV_TYPE3_CHARGER_MUIC,
	ATTACHED_DEV_NONE_TYPE3_MUIC,
	ATTACHED_DEV_UNSUPPORTED_ID_MUIC,
	ATTACHED_DEV_UNSUPPORTED_ID_VB_MUIC,
	ATTACHED_DEV_JIG_UART_ON_VB_MUIC,	/* VBUS enabled */
	ATTACHED_DEV_TIMEOUT_OPEN_MUIC,
	ATTACHED_DEV_WIRELESS_PAD_MUIC,
#if defined(CONFIG_SEC_FACTORY)
	ATTACHED_DEV_CARKIT_MUIC,
#endif
	ATTACHED_DEV_POWERPACK_MUIC,
	/* 71 */
	ATTACHED_DEV_UNDEFINED_RANGE_MUIC,
	ATTACHED_DEV_HICCUP_MUIC,
	ATTACHED_DEV_WATER_MUIC,
	ATTACHED_DEV_CHK_WATER_REQ,
	ATTACHED_DEV_CHK_WATER_DRY_REQ,
	ATTACHED_DEV_GAMEPAD_MUIC,
	ATTACHED_DEV_UNKNOWN_MUIC,
	ATTACHED_DEV_EARJACK_MUIC,
	ATTACHED_DEV_SEND_MUIC,
	ATTACHED_DEV_VOLUP_MUIC,
	ATTACHED_DEV_VOLDN_MUIC,
	/* 81 */
	ATTACHED_DEV_CHECK_OCP,
	ATTACHED_DEV_RDU_TA_MUIC,
	ATTACHED_DEV_UART_MUIC,		
#if defined(CONFIG_MUIC_SUPPORT_EARJACK)
#if defined(CONFIG_MUIC_UNIVERSAL_SM5703)
	ATTACHED_DEV_LP_SEND_MUIC,
	ATTACHED_DEV_LP_VOLUP_MUIC,
	ATTACHED_DEV_LP_VOLDN_MUIC,
	ATTACHED_DEV_LR_SEND_MUIC,
	ATTACHED_DEV_LR_VOLUP_MUIC,
	ATTACHED_DEV_LR_VOLDN_MUIC,
	ATTACHED_ERROR_EARJACK_MUIC,
	/* 91 */
	ATTACHED_STUCK_EARJACK_MUIC,
#endif
#endif
	ATTACHED_DEV_NUM,
} muic_attached_dev_t;

/* muic common callback driver internal data structure
 * that setted at muic-core.c file
 */
struct muic_platform_data {
	void *drv_data;
	void *muic_if;
	int irq_gpio;
	bool suspended;
	bool need_to_noti;
	int gpio_revers;
	
	/* muic current USB/UART path */
	int usb_path;
	int uart_path;

	int gpio_uart_sel;
	int gpio_usb_sel;

#if IS_ENABLED(CONFIG_MUIC_S2MU004_SUPPORT_BC1P2_CERTI)
	/* weak battery */
	u8 vbus_ldo;
#endif

	bool rustproof_on;
	bool afc_disable;
	bool is_new_factory;
	bool dcd_timeout;

#if defined(CONFIG_MUIC_UART_SWITCH)
	/* for the modAP */
	const char *uart_addr;
	const char *uart_txd;
	const char *uart_rxd;
#endif

	/* muic current attached device */
	muic_attached_dev_t	attached_dev;

	bool is_usb_ready;
	bool is_factory_start;
	bool is_rustproof;
	bool is_otg_test;
	bool jigonb_enable;
	bool jig_disable;
	bool is_jig_on;

	int vbvolt;
	int adc;

	/* muic switch dev register function for DockObserver */
	void (*init_switch_dev_cb) (void);
	void (*cleanup_switch_dev_cb) (void);

	/* muic GPIO control function */
	int (*init_gpio_cb) (int switch_sel);
	int (*set_gpio_usb_sel) (int usb_path);
	int (*set_gpio_uart_sel) (int uart_path);
	int (*set_safeout) (int safeout_path);
	void (*jig_uart_cb) (int jig_state);

	/* muic path switch function for waterproof */
	void (*set_path_switch_suspend) (struct device *dev);
	void (*set_path_switch_resume) (struct device *dev);
};

#if defined(CONFIG_MUIC_S2MU004) /* s2mu004 */
#define MUIC_PDATA_FUNC(func, param, ret) \
{\
	*ret = -1;	\
	if (func)	\
		*ret = func(param);	\
	else	\
		pr_err("[muic_core] func not defined %s\n", __func__);	\
}

#define MUIC_PDATA_FUNC_MULTI_PARAM(func, param1, param2, ret) \
{					\
	*ret = -1;	\
	if (func)	\
		*ret = func(param1, param2);	\
	else	\
		pr_err("[muic_core] func not defined %s\n", __func__);	\
}

#ifdef CONFIG_IFCONN_NOTIFIER
#define MUIC_SEND_NOTI_ATTACH(dev) \
{	\
	int ret;	\
	ret = ifconn_notifier_notify( \
					IFCONN_NOTIFY_MUIC,	\
					IFCONN_NOTIFY_MANAGER,	\
					IFCONN_NOTIFY_ID_ATTACH,	\
					dev,	\
					IFCONN_NOTIFY_PARAM_DATA,	\
					NULL);	\
	if (ret < 0) {	\
		pr_err("%s: Fail to send noti\n", \
				__func__);	\
	}	\
}

#define MUIC_SEND_NOTI_ATTACH_ALL(dev) \
{	\
	int ret;	\
	ret = ifconn_notifier_notify( \
					IFCONN_NOTIFY_MUIC,	\
					IFCONN_NOTIFY_ALL,	\
					IFCONN_NOTIFY_ID_ATTACH,	\
					dev,	\
					IFCONN_NOTIFY_PARAM_DATA,	\
					NULL);	\
	if (ret < 0) {	\
		pr_err("%s: Fail to send noti\n", \
				__func__);	\
	}	\
}

#define MUIC_SEND_NOTI_DETACH_ALL(dev) \
{	\
	int ret;	\
	ret = ifconn_notifier_notify( \
					IFCONN_NOTIFY_MUIC,	\
					IFCONN_NOTIFY_ALL,	\
					IFCONN_NOTIFY_ID_DETACH,	\
					dev,	\
					IFCONN_NOTIFY_PARAM_DATA,	\
					NULL);	\
	if (ret < 0) {	\
		pr_err("%s: Fail to send noti\n", \
				__func__);	\
	}	\
}

#define MUIC_SEND_NOTI_TO_CCIC_ATTACH(dev) \
{	\
	int ret;	\
	struct ifconn_notifier_template template;	\
	template.cable_type = dev;	\
	ret = ifconn_notifier_notify( \
					IFCONN_NOTIFY_MUIC,	\
					IFCONN_NOTIFY_CCIC,	\
					IFCONN_NOTIFY_ID_ATTACH,	\
					IFCONN_NOTIFY_EVENT_ATTACH,	\
					IFCONN_NOTIFY_PARAM_DATA,	\
					&template);	\
	if (ret < 0) {	\
		pr_err("%s: Fail to send noti\n", \
				__func__);	\
	}	\
}

#define MUIC_SEND_NOTI_TO_CCIC_DETACH(dev) \
{	\
	int ret;	\
	struct ifconn_notifier_template template;	\
	template.cable_type = dev;	\
	ret = ifconn_notifier_notify( \
					IFCONN_NOTIFY_MUIC,	\
					IFCONN_NOTIFY_CCIC,	\
					IFCONN_NOTIFY_ID_DETACH,	\
					IFCONN_NOTIFY_EVENT_DETACH,	\
					IFCONN_NOTIFY_PARAM_DATA,	\
					&template);	\
	if (ret < 0) {	\
		pr_err("%s: Fail to send noti\n", \
				__func__);	\
	}	\
}

#define MUIC_SEND_NOTI_DETACH(dev) \
{	\
	int ret;	\
	struct ifconn_notifier_template template;	\
	template.cable_type = dev;	\
	ret = ifconn_notifier_notify( \
					IFCONN_NOTIFY_MUIC,	\
					IFCONN_NOTIFY_MANAGER,	\
					IFCONN_NOTIFY_ID_DETACH,	\
					IFCONN_NOTIFY_EVENT_DETACH,	\
					IFCONN_NOTIFY_PARAM_DATA,	\
					&template);	\
	if (ret < 0) {	\
		pr_err("%s: Fail to send noti\n", \
				__func__);	\
	}	\
}
#else
#define MUIC_SEND_NOTI_ATTACH(dev)	\
		muic_notifier_attach_attached_dev(dev)
#define MUIC_SEND_NOTI_ATTACH_ALL(dev) \
		muic_notifier_attach_attached_dev(dev)
#define MUIC_SEND_NOTI_DETACH(dev) \
		muic_notifier_detach_attached_dev(dev)
#define MUIC_SEND_NOTI_DETACH_ALL(dev) \
		muic_notifier_detach_attached_dev(dev)
#define MUIC_SEND_NOTI_TO_CCIC_ATTACH(dev) \
		muic_pdic_notifier_attach_attached_dev(dev)
#define MUIC_SEND_NOTI_TO_CCIC_DETACH(dev) \
		muic_pdic_notifier_detach_attached_dev(dev)
#endif

int muic_core_handle_attach(struct muic_platform_data *muic_pdata,
			muic_attached_dev_t new_dev, int adc, u8 vbvolt);
int muic_core_handle_detach(struct muic_platform_data *muic_pdata);
bool muic_core_get_ccic_cable_state(struct muic_platform_data *muic_pdata);
struct muic_platform_data *muic_core_init(void *drv_data);
void muic_core_exit(struct muic_platform_data *muic_pdata);
extern void muic_disable_otg_detect(void);
#endif /* s2mu004 */

int get_switch_sel(void);
int get_afc_mode(void);
extern struct device *switch_device;
#ifdef CONFIG_SEC_FACTORY
extern void muic_send_attached_muic_cable_intent(int type);
#endif
#endif /* __MUIC_H__ */
