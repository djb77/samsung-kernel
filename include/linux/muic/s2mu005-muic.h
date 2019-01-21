/*
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

#ifndef __S2MU005_MUIC_H__
#define __S2MU005_MUIC_H__

#include <linux/muic/muic.h>

#define MUIC_DEV_NAME	"muic-s2mu005"

/* s2mu005 muic register read/write related information defines. */

/* S2MU005 Control register */
#define CTRL_SWITCH_OPEN_SHIFT	4
#define CTRL_RAW_DATA_SHIFT		3
#define CTRL_MANUAL_SW_SHIFT	2
#define CTRL_WAIT_SHIFT			1
#define CTRL_INT_MASK_SHIFT		0

#define CTRL_SWITCH_OPEN_MASK	(0x1 << CTRL_SWITCH_OPEN_SHIFT)
#define CTRL_RAW_DATA_MASK		(0x1 << CTRL_RAW_DATA_SHIFT)
#define CTRL_MANUAL_SW_MASK		(0x1 << CTRL_MANUAL_SW_SHIFT)
#define CTRL_WAIT_MASK			(0x1 << CTRL_WAIT_SHIFT)
#define CTRL_INT_MASK_MASK		(0x1 << CTRL_INT_MASK_SHIFT)

#ifdef CONFIG_MUIC_S2MU005_ENABLE_AUTOSW
#define CTRL_MASK			(CTRL_SWITCH_OPEN_MASK | \
						CTRL_MANUAL_SW_MASK | CTRL_WAIT_MASK | \
						CTRL_INT_MASK_MASK)
#else
#define CTRL_MASK			(CTRL_SWITCH_OPEN_MASK | \
						CTRL_WAIT_MASK | CTRL_INT_MASK_MASK)
#endif

/* S2MU005 MUIC Interrupt 1 register */
#define INT_RID_CHG_SHIFT		5
#define INT_LKR_SHIFT			4
#define INT_LKP_SHIFT			3
#define INT_KP_SHIFT			2
#define INT_DETACH_SHIFT		1
#define INT_ATTACH_SHIFT		0

#define INT_RID_CHG_MASK		(0x1 << INT_OVP_EN_SHIFT)
#define INT_LKR_MASK			(0x1 << INT_LKR_SHIFT)
#define INT_LKP_MASK			(0x1 << INT_LKP_SHIFT)
#define INT_KP_MASK				(0x1 << INT_KP_SHIFT)
#define INT_DETACH_MASK			(0x1 << INT_DETACH_SHIFT)
#define INT_ATTACH_MASK			(0x1 << INT_ATTACH_SHIFT)

/* S2MU005 MUIC Interrupt 2 register */
#define INT_ADC_CHANGE_SHIFT	2
#define INT_RSRV_ATTACH_SHIFT	1
#define INT_CHG_DET_SHIFT		0

#define INT_ADC_CHANGE_MASK		(0x1 << INT_ADC_CHANGE_SHIFT)
#define INT_RSRV_ATTACH_MASK	(0x1 << INT_RSRV_ATTACH_SHIFT)
#define INT_VBUS_ON_MASK		(0x1 << INT_CHG_DET_SHIFT)

/* S2MU005 ADC register */
#define ADC_MASK				(0x1f)
#define ADC_CONVERSION_MASK	(0x1 << 7)

/* S2MU005 Timing Set 1 & 2 register Timing table */
#define KEY_PRESS_TIME_100MS		(0x00)
#define KEY_PRESS_TIME_200MS		(0x10)
#define KEY_PRESS_TIME_300MS		(0x20)
#define KEY_PRESS_TIME_700MS		(0x60)

#define LONGKEY_PRESS_TIME_300MS	(0x00)
#define LONGKEY_PRESS_TIME_500MS	(0x02)
#define LONGKEY_PRESS_TIME_1000MS	(0x07)
#define LONGKEY_PRESS_TIME_1500MS	(0x0C)

#define SWITCHING_WAIT_TIME_10MS	(0x00)
#define SWITCHING_WAIT_TIME_210MS	(0xa0)

/* S2MU005 MUIC Device Type 1 register */
#define DEV_TYPE1_USB_OTG		(0x1 << 7)
#define DEV_TYPE1_DEDICATED_CHG	(0x1 << 6)
#define DEV_TYPE1_CDP			(0x1 << 5)
#define DEV_TYPE1_T1_T2_CHG		(0x1 << 4)
#define DEV_TYPE1_UART			(0x1 << 3)
#define DEV_TYPE1_USB			(0x1 << 2)
#define DEV_TYPE1_AUDIO_2		(0x1 << 1)
#define DEV_TYPE1_AUDIO_1		(0x1 << 0)
#define DEV_TYPE1_USB_TYPES		(DEV_TYPE1_USB_OTG | DEV_TYPE1_CDP | DEV_TYPE1_USB)
#define DEV_TYPE1_CHG_TYPES		(DEV_TYPE1_DEDICATED_CHG | DEV_TYPE1_CDP)

/* S2MU005 MUIC Device Type 2 register */
#define DEV_TYPE2_SDP_1P8S		(0x1 << 7)
#define DEV_TYPE2_AV			(0x1 << 6)
#define DEV_TYPE2_TTY			(0x1 << 5)
#define DEV_TYPE2_PPD			(0x1 << 4)
#define DEV_TYPE2_JIG_UART_OFF		(0x1 << 3)
#define DEV_TYPE2_JIG_UART_ON		(0x1 << 2)
#define DEV_TYPE2_JIG_USB_OFF		(0x1 << 1)
#define DEV_TYPE2_JIG_USB_ON		(0x1 << 0)
#define DEV_TYPE2_JIG_USB_TYPES		(DEV_TYPE2_JIG_USB_OFF | DEV_TYPE2_JIG_USB_ON)
#define DEV_TYPE2_JIG_UART_TYPES	(DEV_TYPE2_JIG_UART_OFF)
#define DEV_TYPE2_JIG_TYPES		(DEV_TYPE2_JIG_UART_TYPES | DEV_TYPE2_JIG_USB_TYPES)

/* S2MU005 MUIC Device Type 3 register */
#define DEV_TYPE3_U200_CHG		(0x1 << 7)
#define DEV_TYPE3_AV_WITH_VBUS	(0x1 << 4)
#define DEV_TYPE3_VBUS_R255		(0x1 << 1)
#define DEV_TYPE3_MHL			(0x1 << 0)
#define DEV_TYPE3_CHG_TYPE		(DEV_TYPE3_U200_CHG | DEV_TYPE3_VBUS_R255)

/* S2MU005 MUIC APPLE Device Type register */
#define DEV_TYPE_APPLE_APPLE0P5A_CHG	(0x1 << 7)
#define DEV_TYPE_APPLE_APPLE1A_CHG		(0x1 << 6)
#define DEV_TYPE_APPLE_APPLE2A_CHG		(0x1 << 5)
#define DEV_TYPE_APPLE_APPLE2P4A_CHG	(0x1 << 4)
#define DEV_TYPE_APPLE_SDP_DCD_OUT		(0x1 << 3)
#define DEV_TYPE_APPLE_RID_WAKEUP		(0x1 << 2)
#define DEV_TYPE_APPLE_VBUS_WAKEUP		(0x1 << 1)
#define DEV_TYPE_APPLE_BCV1P2_OR_OPEN	(0x1 << 0)

/* S2MU005 MUIC CHG Type register */
#define CHG_TYPE_VBUS_R255	(0x1 << 7)
#define DEV_TYPE_U200		(0x1 << 4)
#define DEV_TYPE_SDP_1P8S	(0x1 << 3)
#define DEV_TYPE_USB		(0x1 << 2)
#define DEV_TYPE_CDPCHG		(0x1 << 1)
#define DEV_TYPE_DCPCHG		(0x1 << 0)
#define DEV_TYPE_CHG_TYPE		(CHG_TYPE_VBUS_R255 | DEV_TYPE_U200 | DEV_TYPE_SDP_1P8S)

#define MANUAL_SW_JIG_EN		(0x1 << 0)

/*
 * Manual Switch
 * D- [7:5] / D+ [4:2] / CHARGER[1] / OTGEN[0]
 * 000: Open all / 001: USB / 010: AUDIO / 011: UART / 100: V_AUDIO
 * 00: Vbus to Open / 01: Vbus to Charger / 10: Vbus to MIC / 11: Vbus to VBout
 */
#define MANUAL_SW_DM_SHIFT		5
#define MANUAL_SW_DP_SHIFT		2
#define MANUAL_SW_CHG_SHIFT		1
#define MANUAL_SW_DM_DP_MASK	0xFC

#define MANUAL_SW_OPEN			(0x0)
#define MANUAL_SW_USB			(0x1 << MANUAL_SW_DM_SHIFT | 0x1 << MANUAL_SW_DP_SHIFT)
#define MANUAL_SW_UART		(0x2 << MANUAL_SW_DM_SHIFT | 0x2 << MANUAL_SW_DP_SHIFT)
#define MANUAL_SW_UART2			(0x3 << MANUAL_SW_DM_SHIFT | 0x3 << MANUAL_SW_DP_SHIFT)
#define MANUAL_SW_AUDIO		(0x0 << MANUAL_SW_DM_SHIFT | 0x0 << MANUAL_SW_DP_SHIFT) /* Not Used */

#define MANUAL_SW_OTGEN		(0x1)
#define MANUAL_SW_CHARGER	(0x1 << MANUAL_SW_CHG_SHIFT)

enum s2mu005_reg_manual_sw_value {
	MANSW_OPEN		=	(MANUAL_SW_OPEN),
	MANSW_OPEN_WITH_VBUS	=	(MANUAL_SW_OPEN),
	MANSW_USB		=	(MANUAL_SW_USB),
	MANSW_AUDIO	=	(MANUAL_SW_AUDIO), /* Not Used */
	MANSW_OTG		=	(MANUAL_SW_USB),
	MANSW_UART		=	(MANUAL_SW_UART),
	MANSW_OPEN_RUSTPROOF	=	(MANUAL_SW_OPEN),
};

/* muic chip specific internal data structure
 * that setted at muic-xxxx.c file
 */
struct s2mu005_muic_data {
	struct device *dev;
	struct i2c_client *i2c; /* i2c addr: 0x7A; MUIC */
	struct mutex muic_mutex;

	/* model dependant mfd platform data */
	struct s2mu005_platform_data	*mfd_pdata;

	int irq_attach;
	int irq_detach;
	int irq_rid_chg;
	int irq_vbus_on;
	int irq_rsvd_attach;
	int irq_adc_change;
	int irq_av_charge;
	int irq_vbus_off;

	/* muic common callback driver internal data */
	struct sec_switch_data *switch_data;

	/* model dependant muic platform data */
	struct muic_platform_data *pdata;

	struct wake_lock wake_lock;

	/* muic support vps list */
	bool muic_support_list[ATTACHED_DEV_NUM];

	/* muic current attached device */
	muic_attached_dev_t	attached_dev;

	/* muic Device ID */
	u8 muic_vendor;			/* Vendor ID */
	u8 muic_version;		/* Version ID */

	bool	is_usb_ready;
	bool	is_factory_start;
	bool	is_rustproof;
	bool	is_otg_test;
	bool	undefined_range;

#if !defined(CONFIG_MUIC_S2MU005_ENABLE_AUTOSW)
	bool	is_jig_on;
#endif
	/* W/A waiting for the charger ic */
	bool suspended;
	bool need_to_noti;

	struct workqueue_struct *muic_wqueue;

	int rev_id;
};

#define MUIC_SUPPORTED_VPS	1
#define MUIC_UNSUPPORTED_VPS	0

struct vps_tbl_data {
	u8 adc;
	char *rid;
	char *name;
	bool supported;
};

#define MDEV(name) ATTACHED_DEV_##name##_MUIC

extern struct device *switch_device;
extern unsigned int system_rev;
extern struct muic_platform_data muic_pdata;

#endif /* __S2MU005_MUIC_H__ */
