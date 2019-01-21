/*
 * muic_regmap_sm5708.c
 *
 * Copyright (C) 2014 Samsung Electronics
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
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/host_notify.h>
#include <linux/string.h>

#include <linux/muic/muic.h>
#include <linux/muic/muic_afc.h>

#if defined(CONFIG_MUIC_NOTIFIER)
#include <linux/muic/muic_notifier.h>
#endif /* CONFIG_MUIC_NOTIFIER */

#if defined(CONFIG_OF)
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#endif /* CONFIG_OF */

#include "muic-internal.h"
#include "muic_i2c.h"
#include "muic_regmap.h"

#define ADC_DETECT_TIME_200MS (0x03)
#define KEY_PRESS_TIME_100MS  (0x00)

enum sm5708_muic_reg_init_value {
	REG_INTMASK1_VALUE		= (0xFC),
	REG_INTMASK2_VALUE		= (0x00),
	REG_INTMASK3AFC_VALUE	= (0xFF),
	REG_TIMING1_VALUE		= (ADC_DETECT_TIME_200MS | KEY_PRESS_TIME_100MS),
	REG_RSVDID2_VALUE		= (0x26),
	REG_AFCCNTL_VALUE		= (0x20),
};

/* sm5708 I2C registers */
enum sm5708_muic_reg {
	REG_DEVID		= 0x01,
	REG_CTRL		= 0x02,
	REG_INT1		= 0x03,
	REG_INT2		= 0x04,
	REG_INT3		= 0x05,
	REG_INTMASK1	= 0x06,
	REG_INTMASK2	= 0x07,
	REG_INTMASK3	= 0x08,
	REG_ADC			= 0x09,
	REG_DEVT1		= 0x0a,
	REG_DEVT2		= 0x0b,
	REG_DEVT3		= 0x0c,
	REG_TIMING1		= 0x0d,
	REG_TIMING2		= 0x0e,
	/* 0x0f is reserved. */
	REG_BTN1		= 0x10,
	REG_BTN2		= 0x11,
	REG_CarKit		= 0x12,
	REG_MANSW1		= 0x13,
	REG_MANSW2		= 0x14,
	REG_RSVDID1		= 0x15,
	REG_RSVDID2		= 0x16,
	REG_CHGTYPE		= 0x17,

	REG_AFCCNTL		= 0x18,
	REG_VBUSSTAT	= 0x1b,
	REG_RESET		= 0x22,
	REG_END,
};

#define REG_ITEM(addr, bitp, mask) ((bitp<<16) | (mask<<8) | addr)

/* Field */
enum sm5708_muic_reg_item {
	DEVID_VendorID		= REG_ITEM(REG_DEVID, _BIT0, _MASK3),

	CTRL_SW_OPEN		= REG_ITEM(REG_CTRL, _BIT4, _MASK1),
	CTRL_RAWDATA		= REG_ITEM(REG_CTRL, _BIT3, _MASK1),
	CTRL_ManualSW		= REG_ITEM(REG_CTRL, _BIT2, _MASK1),
	CTRL_WAIT			= REG_ITEM(REG_CTRL, _BIT1, _MASK1),
	CTRL_MASK_INT		= REG_ITEM(REG_CTRL, _BIT0, _MASK1),

	INT1_DETACH			= REG_ITEM(REG_INT1, _BIT1, _MASK1),
	INT1_ATTACH			= REG_ITEM(REG_INT1, _BIT0, _MASK1),

	INT2_VBUSDET_ON		= REG_ITEM(REG_INT2, _BIT7, _MASK1),
	INT2_RID_CHARGER	= REG_ITEM(REG_INT2, _BIT6, _MASK1),
	INT2_MHL			= REG_ITEM(REG_INT2, _BIT5, _MASK1),
	INT2_ADC_CHG		= REG_ITEM(REG_INT2, _BIT2, _MASK1),
	INT2_VBUS_OFF		= REG_ITEM(REG_INT2, _BIT0, _MASK1),

	INT3_AFC_ERROR			= REG_ITEM(REG_INT3, _BIT5, _MASK1),
	INT3_AFC_STA_CHG		= REG_ITEM(REG_INT3, _BIT4, _MASK1),
	INT3_MULTI_BYTE			= REG_ITEM(REG_INT3, _BIT3, _MASK1),
	INT3_VBUS_UPDATE		= REG_ITEM(REG_INT3, _BIT2, _MASK1),
	INT3_AFC_ACCEPTED		= REG_ITEM(REG_INT3, _BIT1, _MASK1),
	INT3_AFC_TA_ATTACHED	= REG_ITEM(REG_INT3, _BIT0, _MASK1),

	INTMASK1_DETACH_M		= REG_ITEM(REG_INTMASK1, _BIT1, _MASK1),
	INTMASK1_ATTACH_M		= REG_ITEM(REG_INTMASK1, _BIT0, _MASK1),

	INTMASK2_VBUSDET_ON_M	= REG_ITEM(REG_INTMASK2, _BIT7, _MASK1),
	INTMASK2_RID_CHARGERM	= REG_ITEM(REG_INTMASK2, _BIT6, _MASK1),
	INTMASK2_MHL_M			= REG_ITEM(REG_INTMASK2, _BIT5, _MASK1),
	INTMASK2_ADC_CHG_M		= REG_ITEM(REG_INTMASK2, _BIT2, _MASK1),
	INTMASK2_REV_ACCE_M		= REG_ITEM(REG_INTMASK2, _BIT1, _MASK1),
	INTMASK2_VBUS_OFF_M		= REG_ITEM(REG_INTMASK2, _BIT0, _MASK1),

	INT3_AFC_ERROR_M		= REG_ITEM(REG_INTMASK3, _BIT5, _MASK1),
	INT3_AFC_STA_CHG_M		= REG_ITEM(REG_INTMASK3, _BIT4, _MASK1),
	INT3_MULTI_BYTE_M		= REG_ITEM(REG_INTMASK3, _BIT3, _MASK1),
	INT3_VBUS_UPDATE_M		= REG_ITEM(REG_INTMASK3, _BIT2, _MASK1),
	INT3_AFC_ACCEPTED_M		= REG_ITEM(REG_INTMASK3, _BIT1, _MASK1),
	INT3_AFC_TA_ATTACHED_M	= REG_ITEM(REG_INTMASK3, _BIT0, _MASK1),

	ADC_ADC_VALUE		=  REG_ITEM(REG_ADC, _BIT0, _MASK5),

	DEVT1_USB_OTG		= REG_ITEM(REG_DEVT1, _BIT7, _MASK1),
	DEVT1_DEDICATED_CHG	= REG_ITEM(REG_DEVT1, _BIT6, _MASK1),
	DEVT1_USB_CHG		= REG_ITEM(REG_DEVT1, _BIT5, _MASK1),
	DEVT1_CAR_KIT_CHARGER	= REG_ITEM(REG_DEVT1, _BIT4, _MASK1),
	DEVT1_UART			= REG_ITEM(REG_DEVT1, _BIT3, _MASK1),
	DEVT1_USB			= REG_ITEM(REG_DEVT1, _BIT2, _MASK1),
	DEVT1_AUDIO_TYPE2	= REG_ITEM(REG_DEVT1, _BIT1, _MASK1),
	DEVT1_AUDIO_TYPE1	= REG_ITEM(REG_DEVT1, _BIT0, _MASK1),

	DEVT2_AV			= REG_ITEM(REG_DEVT2, _BIT6, _MASK1),
	DEVT2_TTY			= REG_ITEM(REG_DEVT2, _BIT5, _MASK1),
	DEVT2_PPD			= REG_ITEM(REG_DEVT2, _BIT4, _MASK1),
	DEVT2_JIG_UART_OFF	= REG_ITEM(REG_DEVT2, _BIT3, _MASK1),
	DEVT2_JIG_UART_ON	= REG_ITEM(REG_DEVT2, _BIT2, _MASK1),
	DEVT2_JIG_USB_OFF	= REG_ITEM(REG_DEVT2, _BIT1, _MASK1),
	DEVT2_JIG_USB_ON	= REG_ITEM(REG_DEVT2, _BIT0, _MASK1),

	DEVT3_AFC_TA		= REG_ITEM(REG_DEVT3, _BIT7, _MASK1),
	DEVT3_U200_CHG		= REG_ITEM(REG_DEVT3, _BIT6, _MASK1),
	DEVT3_LO_TA			= REG_ITEM(REG_DEVT3, _BIT5, _MASK1),
	DEVT3_AV_CABLE_VBUS	= REG_ITEM(REG_DEVT3, _BIT4, _MASK1),
	DEVT3_DCD_OUT_SDP	= REG_ITEM(REG_DEVT3, _BIT2, _MASK1),
	DEVT3_QC20_TA		= REG_ITEM(REG_DEVT3, _BIT1, _MASK1),
	DEVT3_MHL			= REG_ITEM(REG_DEVT3, _BIT0, _MASK1),

	TIMING1_KEY_PRESS_T	= REG_ITEM(REG_TIMING1, _BIT4, _MASK4),
	TIMING1_ADC_DET_T	= REG_ITEM(REG_TIMING1, _BIT0, _MASK4),

	TIMING2_SW_WAIT_T	= REG_ITEM(REG_TIMING2, _BIT4, _MASK4),
	TIMING2_LONG_KEY_T	= REG_ITEM(REG_TIMING2, _BIT0, _MASK4),

	MANSW1_DM_CON_SW	=  REG_ITEM(REG_MANSW1, _BIT5, _MASK3),
	MANSW1_DP_CON_SW	=  REG_ITEM(REG_MANSW1, _BIT2, _MASK3),

	MANSW2_BOOT_SW		=  REG_ITEM(REG_MANSW2, _BIT3, _MASK1),
	MANSW2_JIG_ON		=  REG_ITEM(REG_MANSW2, _BIT2, _MASK1),
	MANSW2_SINGLE_MODE	=  REG_ITEM(REG_MANSW2, _BIT1, _MASK1),
	MANSW2_ID_SW		=  REG_ITEM(REG_MANSW2, _BIT0, _MASK1),

	RSVDID1_VBUS_VALID	= REG_ITEM(REG_RSVDID1, _BIT0, _MASK1),
	RSVDID2_DCD_TIME_EN	= REG_ITEM(REG_RSVDID2, _BIT2, _MASK1),
	RSVDID2_BCD_RESCAN	= REG_ITEM(REG_RSVDID2, _BIT4, _MASK1),
	RSVDID2_CHGPUMP_nEN	= REG_ITEM(REG_RSVDID2, _BIT5, _MASK1),
	CHGTYPE_CHG_TYPE	= REG_ITEM(REG_CHGTYPE, _BIT0, _MASK5),

	AFCCNTL_DISAFC		= REG_ITEM(REG_AFCCNTL, _BIT5, _MASK1),
	AFCCNTL_VBUS_READ	= REG_ITEM(REG_AFCCNTL, _BIT3, _MASK1),
	VBUSSTAT_STATUS		= REG_ITEM(REG_VBUSSTAT, _BIT0, _MASK4),

	RESET_RESET = REG_ITEM(REG_RESET, _BIT0, _MASK1),
};

/* sm5708 Control register */
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
#define CTRL_MASK				(CTRL_SWITCH_OPEN_MASK | CTRL_RAW_DATA_MASK | \
								/*CTRL_MANUAL_SW_MASK |*/ CTRL_WAIT_MASK | \
								CTRL_INT_MASK_MASK)
#define DEV_TYPE3_AFC_TA		(0x1 << 7)

struct reg_value_set {
	int value;
	char *alias;
};

/* ADC Scan Mode Values : b'1 */
static struct reg_value_set sm5708_adc_scanmode_tbl[] = {
	[ADC_SCANMODE_CONTINUOUS] = {0x01, "Periodic"},
	[ADC_SCANMODE_ONESHOT]    = {0x00, "Oneshot."},
	[ADC_SCANMODE_PULSE]      = {0x00, "Oneshot.."},
};

/*
 * Manual Switch
 * D- [7:5] / D+ [4:2] / Reserved [1:0]
 * 000: Open all / 001: USB / 010: AUDIO / 011: UART / 100: V_AUDIO
 * No Vbus switching in SM5708
 */
#define _D_OPEN		(0x0)
#define _D_USB		(0x1)
#define _D_AUDIO	(0x2)
#define _D_UART		(0x3)

/* COM patch Values */
#define COM_VALUE(dm) ((dm<<5) | (dm<<2))

#define _COM_OPEN		COM_VALUE(_D_OPEN)
#define _COM_UART_AP	COM_VALUE(_D_UART)
#define _COM_UART_CP	_COM_UART_AP
#define _COM_USB_AP		COM_VALUE(_D_USB)
#define _COM_USB_CP		_COM_USB_AP
#define _COM_AUDIO		COM_VALUE(_D_AUDIO)

static int sm5708_com_value_tbl[] = {
	[COM_OPEN]			= _COM_OPEN,
	[COM_UART_AP]		= _COM_UART_AP,
	[COM_UART_CP]		= _COM_UART_CP,
	[COM_USB_AP]		= _COM_USB_AP,
	[COM_USB_CP]		= _COM_USB_CP,
	[COM_AUDIO]			= _COM_AUDIO,
};

#define REG_CTRL_INITIAL (CTRL_MASK | CTRL_MANUAL_SW_MASK)

static regmap_t sm5708_muic_regmap_table[] = {
	[REG_DEVID]		= {"DeviceID",		0x01, 0x00, INIT_NONE},
	[REG_CTRL]		= {"CONTROL",		0x1F, 0x00, REG_CTRL_INITIAL,},
	[REG_INT1]		= {"INT1",			0x00, 0x00, INIT_NONE,},
	[REG_INT2]		= {"INT2",			0x00, 0x00, INIT_NONE,},
	[REG_INT3]		= {"INT3_AFC",		0x00, 0x00, INIT_NONE,},
	[REG_INTMASK1]	= {"INTMASK1",		0x00, 0x00, REG_INTMASK1_VALUE,},
	[REG_INTMASK2]	= {"INTMASK2",		0x00, 0x00, REG_INTMASK2_VALUE,},
	[REG_INTMASK3]	= {"INTMASK3_AFC",	0x00, 0x00, REG_INTMASK3AFC_VALUE,},
	[REG_ADC]		= {"ADC",			0x1F, 0x00, INIT_NONE,},
	[REG_DEVT1]		= {"DEVICETYPE1",	0xFF, 0x00, INIT_NONE,},
	[REG_DEVT2]		= {"DEVICETYPE2",	0xFF, 0x00, INIT_NONE,},
	[REG_DEVT3]		= {"DEVICETYPE3",	0xFF, 0x00, INIT_NONE,},
	[REG_TIMING1]	= {"TimingSet1",	0x00, 0x00, REG_TIMING1_VALUE,},
	[REG_TIMING2]	= {"TimingSet2",	0x00, 0x00, INIT_NONE,},
	/* 0x0F: Reserved */
	[REG_BTN1]		= {"BUTTON1",		0xFF, 0x00, INIT_NONE,},
	[REG_BTN2]		= {"BUTTON2",		0xFF, 0x00, INIT_NONE,},
	[REG_CarKit]	= {"CarKitStatus",	0x00, 0x00, INIT_NONE,},
	[REG_MANSW1]	= {"ManualSW1",		0x00, 0x00, INIT_NONE,},
	[REG_MANSW2]	= {"ManualSW2",		0x00, 0x00, INIT_NONE,},
	[REG_RSVDID1]	= {"Reserved_ID1",	0xFF, 0x00, INIT_NONE,},
	[REG_RSVDID2]	= {"Reserved_ID2",	0x24, 0x00, REG_RSVDID2_VALUE,},
	[REG_CHGTYPE]	= {"REG_CHG_TYPE",	0xFF, 0x00, INIT_NONE,},
	[REG_AFCCNTL]	= {"AFC_CNTL",		0x00, 0x00, REG_AFCCNTL_VALUE,},
	[REG_RESET]		= {"RESET",			0x00, 0x00, INIT_NONE,},
	[REG_END]		= {NULL,			0x00, 0x00, INIT_NONE}
};

static void sm5708_vdp_src_en_work(struct work_struct *work);

static int sm5708_muic_ioctl(struct regmap_desc *pdesc,
		int arg1, int *arg2, int *arg3)
{
	int ret = 0;

	switch (arg1) {
	case GET_COM_VAL:
		*arg2 = sm5708_com_value_tbl[*arg2];
		*arg3 = REG_MANSW1;
		break;
	case GET_CTLREG:
		*arg3 = REG_CTRL;
		break;

	case GET_ADC:
		*arg3 = ADC_ADC_VALUE;
		break;

	case GET_SWITCHING_MODE:
		*arg3 = CTRL_ManualSW;
		break;

	case GET_INT_MASK:
		*arg3 = CTRL_MASK_INT;
		break;

	case GET_REVISION:
		*arg3 = DEVID_VendorID;
		break;

	case GET_OTG_STATUS:
		*arg3 = INTMASK2_VBUS_OFF_M;
		break;

	case GET_CHGTYPE:
		*arg3 = CHGTYPE_CHG_TYPE;
		break;

	case GET_RESID3:
		*arg3 = RSVDID2_BCD_RESCAN;
		break;

	default:
		ret = -1;
		break;
	}

	if (pdesc->trace) {
		pr_info("%s: ret:%d arg1:%x,", __func__, ret, arg1);

		if (arg2)
			pr_info(" arg2:%x,", *arg2);

		if (arg3)
			pr_info(" arg3:%x - %s", *arg3,
				regmap_to_name(pdesc, _ATTR_ADDR(*arg3)));
		pr_info("\n");
	}

	return ret;
}

static int sm5708_run_chgdet(struct regmap_desc *pdesc, bool started)
{
	int attr, value, ret;

	pr_info("%s: start. %s\n", __func__, started ? "enabled" : "disabled");

	attr = MANSW1_DM_CON_SW;
	value = 0;
	ret = regmap_write_value(pdesc, attr, value);
	if (ret < 0)
		pr_err("%s Reset reg write fail.\n", __func__);
	else
		_REGMAP_TRACE(pdesc, 'w', ret, attr, value);

	pr_info("%s: reset [DP]\n", __func__);
	attr = MANSW1_DP_CON_SW;
	value = 0;
	ret = regmap_write_value(pdesc, attr, value);
	if (ret < 0)
		pr_err("%s Reset reg write fail.\n", __func__);
	else
		_REGMAP_TRACE(pdesc, 'w', ret, attr, value);

	pr_info("%s: reset [MannualSW]\n", __func__);
	attr = CTRL_ManualSW;
	value = 0;
	ret = regmap_write_value(pdesc, attr, value);
	if (ret < 0)
		pr_err("%s Reset reg write fail.\n", __func__);
	else
		_REGMAP_TRACE(pdesc, 'w', ret, attr, value);

	pr_info("%s: reset [RESET]\n", __func__);
	attr = RESET_RESET;
	value = started ? 1 : 0;
	ret = regmap_write_value_ex(pdesc, attr, value);
	if (ret < 0)
		pr_err("%s Reset reg write fail.\n", __func__);
	else
		_REGMAP_TRACE(pdesc, 'w', ret, attr, value);

	pr_info("%s: init all register\n", __func__);
	muic_reg_init(pdesc->muic);

	pr_info("%s: enable INT\n", __func__);
	set_int_mask(pdesc->muic, 0);

	return ret;
}

static int sm5708_attach_ta(struct regmap_desc *pdesc)
{
	pr_info("%s\n", __func__);
	return 0;
}

static int sm5708_detach_ta(struct regmap_desc *pdesc)
{
	muic_data_t *pmuic = pdesc->muic;

	pr_info("%s:%s attached_dev 0x(%x).\n", MUIC_DEV_NAME, __func__, pmuic->attached_dev);

	pmuic->is_afc_device = 0;
	pmuic->attached_dev = ATTACHED_DEV_NONE_MUIC;

	return 0;
}

static int sm5708_set_rustproof(struct regmap_desc *pdesc, int op)
{
	int attr = 0, value = 0, ret = 0;

	pr_info("%s\n", __func__);

	do {
		attr = MANSW2_JIG_ON;
		value = op ? 1 : 0;
		ret = regmap_write_value(pdesc, attr, value);
		if (ret < 0) {
			pr_err("%s MANSW2_JIG_ON write fail.\n", __func__);
			break;
		}

		_REGMAP_TRACE(pdesc, 'w', ret, attr, value);

		attr = CTRL_ManualSW;
		value = op ? 0 : 1;
		ret = regmap_write_value(pdesc, attr, value);
		if (ret < 0) {
			pr_err("%s CTRL_ManualSW write fail.\n", __func__);
			break;
		}

		_REGMAP_TRACE(pdesc, 'w', ret, attr, value);

	} while (0);

	return ret;
}

static int sm5708_get_vps_data(struct regmap_desc *pdesc, void *pbuf)
{
	muic_data_t *pmuic = pdesc->muic;
	vps_data_t *pvps = (vps_data_t *)pbuf;
	int attr;

	if (pdesc->trace)
		pr_info("%s\n", __func__);

	*(u8 *)&pvps->s.val1 = muic_i2c_read_byte(pmuic->i2c, REG_DEVT1);
	*(u8 *)&pvps->s.val2 = muic_i2c_read_byte(pmuic->i2c, REG_DEVT2);
	*(u8 *)&pvps->s.val3 = muic_i2c_read_byte(pmuic->i2c, REG_DEVT3);

	attr = RSVDID1_VBUS_VALID;
	*(u8 *)&pvps->s.vbvolt = regmap_read_value(pdesc, attr);

	attr = ADC_ADC_VALUE;
	*(u8 *)&pvps->s.adc = regmap_read_value(pdesc, attr);

	return 0;
}

static int sm5708_get_adc_scan_mode(struct regmap_desc *pdesc)
{
	struct reg_value_set *pvset;
	int attr, value, mode = 0;

	attr = MANSW2_SINGLE_MODE;
	value = regmap_read_value(pdesc, attr);

	for ( ; mode < ARRAY_SIZE(sm5708_adc_scanmode_tbl); mode++) {
		pvset = &sm5708_adc_scanmode_tbl[mode];
		if (pvset->value == value)
			break;
	}

	pr_info("%s: [%2d]=%02x,%02x\n", __func__, mode, value, pvset->value);
	pr_info("%s:       %s\n", __func__, pvset->alias);

	return mode;
}

static void sm5708_set_adc_scan_mode(struct regmap_desc *pdesc,
		const int mode)
{
	struct reg_value_set *pvset;
	int attr, ret, value;

	if (mode > ADC_SCANMODE_PULSE) {
		pr_err("%s Out of range(%d).\n", __func__, mode);
		return;
	}

	pvset = &sm5708_adc_scanmode_tbl[mode];
	pr_info("%s: [%2d] %s\n", __func__, mode, pvset->alias);

	do {
		value = pvset->value;
		attr = MANSW2_SINGLE_MODE;
		ret = regmap_write_value(pdesc, attr, value);
		if (ret < 0) {
			pr_err("%s MANSW2_SINGLE_MODE write fail.\n", __func__);
			break;
		}

		_REGMAP_TRACE(pdesc, 'w', ret, attr, value);

#define _ENABLE_PERIODIC_SCAN (0)
#define _DISABLE_PERIODIC_SCAN (1)

		attr = CTRL_RAWDATA;
		if (mode == ADC_SCANMODE_CONTINUOUS)
			value = _ENABLE_PERIODIC_SCAN;
		else
			value = _DISABLE_PERIODIC_SCAN;

		ret = regmap_write_value(pdesc, attr, value);
		if (ret < 0) {
			pr_err("%s CTRL_RAWDATA write fail.\n", __func__);
			break;
		}

		_REGMAP_TRACE(pdesc, 'w', ret, attr, value);

	} while (0);
}

enum switching_mode_val {
	_SWMODE_AUTO = 1,
	_SWMODE_MANUAL = 0,
};

static int sm5708_get_switching_mode(struct regmap_desc *pdesc)
{
	int attr, value, mode;

	pr_info("%s\n", __func__);

	attr = CTRL_ManualSW;
	value = regmap_read_value(pdesc, attr);

	mode = (value == _SWMODE_AUTO) ? SWMODE_AUTO : SWMODE_MANUAL;

	return mode;
}

static void sm5708_set_switching_mode(struct regmap_desc *pdesc, int mode)
{
	int attr, value;
	int ret = 0;

	pr_info("%s\n", __func__);

	value = (mode == SWMODE_AUTO) ? _SWMODE_AUTO : _SWMODE_MANUAL;
	attr = CTRL_ManualSW;
	ret = regmap_write_value(pdesc, attr, value);
	if (ret < 0)
		pr_err("%s REG_CTRL write fail.\n", __func__);
	else
		_REGMAP_TRACE(pdesc, 'w', ret, attr, value);
}

static int sm5708_run_BCD_rescan(struct regmap_desc *pdesc, int value)
{
	int attr, ret;

	attr = RSVDID2_BCD_RESCAN;

	ret = regmap_write_value(pdesc, attr, value);
	if (ret < 0)
		pr_err("%s BCD_RESCAN write fail.\n", __func__);
	else
		_REGMAP_TRACE(pdesc, 'w', ret, attr, value);

	return ret;
}

#define VBUS_OFFSET		5
static int sm5708_get_vbus_value(struct regmap_desc *pdesc)
{
	muic_data_t *muic = pdesc->muic;
	u8 val = -1;
	int ret;
	int attr, value;
	int i, retry_cnt = 20;

	/* write '1' to INT3_VBUS_UPDATE_M: masking */
	attr = INT3_VBUS_UPDATE_M;
	value = 1;
	ret = regmap_write_value(pdesc, attr, value);
	if (ret < 0) {
		pr_err("%s: failed to set VBUS_UPDATE_M to 1\n", __func__);
		return -EINVAL;
	}


	/* write '1' -> '0' to VBUS_READ of AFC_CNTL */
	attr = AFCCNTL_VBUS_READ;
	value = 1;
	ret = regmap_write_value(pdesc, attr, value);
	if (ret < 0) {
		pr_err("%s: failed to set VBUS_READ to 1\n", __func__);
		return -EINVAL;
	}

	msleep(20);

	value = 0;
	ret = regmap_write_value(pdesc, attr, value);
	if (ret < 0) {
		pr_err("%s: failed to set VBUS_READ to 0\n", __func__);
		return -EINVAL;
	}


	/* check VBUS_UPDATE */
	attr = INT3_VBUS_UPDATE;
	for (i = 0 ; i < retry_cnt ; i++) {
		ret = regmap_read_value(pdesc, attr);
		if (ret > 0)
			break;
		msleep(50);
	}

	/* read VBUS Value */
	if (ret && i < retry_cnt) {
		val = muic_i2c_read_byte(muic->i2c, REG_VBUSSTAT);
		val &= 0xF;
		pr_info("%s: vbus:%d\n", __func__, val);
	}

	/* write '0' to INT3_VBUS_UPDATE_M: unmasking */
	attr = INT3_VBUS_UPDATE_M;
	value = 0;
	ret = regmap_write_value(pdesc, attr, value);
	if (ret < 0) {
		pr_err("%s: failed to set VBUS_UPDATE_M to 0\n", __func__);
		return -EINVAL;
	}

	if (val > 0xF)
		return -EINVAL;

	/* H/W reqest: 8V return 9V */
	if (val == 3)
		val++;

	return val + VBUS_OFFSET;
}

#define DCD_OUT_SDP	(1 << 2)

static bool sm5708_get_dcdtmr_irq(struct regmap_desc *pdesc)
{
	muic_data_t *muic = pdesc->muic;
	int ret;

	ret = muic_i2c_read_byte(muic->i2c, REG_DEVT3);
	if (ret < 0) {
		pr_err("%s: failed to read REG_DEVT3\n", __func__);
		return false;
	}

	pr_info("%s: REG_DEVT3: 0x%02x\n", __func__, ret);

	if (ret & DCD_OUT_SDP)
		return true;

	return false;
}

static void sm5708_get_fromatted_dump(struct regmap_desc *pdesc, char *mesg)
{
	muic_data_t *muic = pdesc->muic;
	int val;

	if (pdesc->trace)
		pr_info("%s\n", __func__);

	val = i2c_smbus_read_byte_data(muic->i2c, REG_CTRL);
	snprintf(mesg, 16, "CT:%x ", val);
	val = i2c_smbus_read_byte_data(muic->i2c, REG_INTMASK1);
	snprintf(mesg+strlen(mesg), 16, "IM1:%x ", val);
	val = i2c_smbus_read_byte_data(muic->i2c, REG_INTMASK2);
	snprintf(mesg+strlen(mesg), 16, "IM2:%x ", val);
	val = i2c_smbus_read_byte_data(muic->i2c, REG_MANSW1);
	snprintf(mesg+strlen(mesg), 16, "MS1:%x ", val);
	val = i2c_smbus_read_byte_data(muic->i2c, REG_MANSW2);
	snprintf(mesg+strlen(mesg), 16, "MS2:%x ", val);
	val = i2c_smbus_read_byte_data(muic->i2c, REG_ADC);
	snprintf(mesg+strlen(mesg), 16, "ADC:%x ", val);
	val = i2c_smbus_read_byte_data(muic->i2c, REG_DEVT1);
	snprintf(mesg+strlen(mesg), 16, "DT1:%x ", val);
	val = i2c_smbus_read_byte_data(muic->i2c, REG_DEVT2);
	snprintf(mesg+strlen(mesg), 16, "DT2:%x ", val);
	val = i2c_smbus_read_byte_data(muic->i2c, REG_DEVT3);
	snprintf(mesg+strlen(mesg), 16, "DT3:%x ", val);
	val = i2c_smbus_read_byte_data(muic->i2c, REG_RSVDID1);
	snprintf(mesg+strlen(mesg), 16, "RS1:%x ", val);
	val = i2c_smbus_read_byte_data(muic->i2c, REG_RSVDID2);
	snprintf(mesg+strlen(mesg), 16, "RS2:%x", val);
	val = i2c_smbus_read_byte_data(muic->i2c, REG_AFCCNTL);
	snprintf(mesg+strlen(mesg), 16, "ACNT:%x", val);
}

static int sm5708_get_sizeof_regmap(void)
{
	pr_info("%s:%s\n", MUIC_DEV_NAME, __func__);
	return (int)ARRAY_SIZE(sm5708_muic_regmap_table);
}

void sm5708_vdp_src_en_state(muic_data_t *muic, int state)
{
	int ret;

	pr_info("%s:%s  state = %d\n", MUIC_DEV_NAME, __func__, state);

	if (state == 1) {
		ret = muic_i2c_read_byte(muic->i2c, REG_RSVDID2);
		ret = ret | 0x08;
		muic_i2c_write_byte(muic->i2c, REG_RSVDID2, ret); /* VDP_SRC_EN '0' -> '1' */

		INIT_DELAYED_WORK(&muic->vdp_src_en_work, sm5708_vdp_src_en_work);
		schedule_delayed_work(&muic->vdp_src_en_work, msecs_to_jiffies(40000)); /* 40.0 sec */
		pr_info("%s:%s sm5708_vdp_src_en_work(40sec) start\n", MUIC_DEV_NAME, __func__);
	} else {
		ret = muic_i2c_read_byte(muic->i2c, REG_RSVDID2);
		ret = ret & 0xF7;
		muic_i2c_write_byte(muic->i2c, REG_RSVDID2, ret); /* VDP_SRC_EN '1' -> '0' */
		cancel_delayed_work(&muic->vdp_src_en_work);
		pr_info("%s:%s sm5708_vdp_src_en_work cancel\n", MUIC_DEV_NAME, __func__);
	}
}

static void sm5708_vdp_src_en_work(struct work_struct *work)
{
	muic_data_t *muic =
		container_of(work, muic_data_t, vdp_src_en_work.work);

	pr_info("%s:%s\n", MUIC_DEV_NAME, __func__);

	sm5708_vdp_src_en_state(muic, 0);
}

static int sm5708_set_afc_ctrl_reg(struct regmap_desc *pdesc, int shift, bool on)
{
	pr_info("%s:%s Does not working.\n", MUIC_DEV_NAME, __func__);

	return 0;
}

static int sm5708_afc_ta_attach(struct regmap_desc *pdesc)
{
	pr_info("%s:%s Does not working.\n", MUIC_DEV_NAME, __func__);

	return 0;
}

static int sm5708_afc_ta_accept(struct regmap_desc *pdesc)
{
	pr_info("%s:%s Does not working.\n", MUIC_DEV_NAME, __func__);

	return 0;
}

static int sm5708_afc_vbus_update(struct regmap_desc *pdesc)
{
	pr_info("%s:%s Does not working.\n", MUIC_DEV_NAME, __func__);

	return 0;
}

static int sm5708_afc_multi_byte(struct regmap_desc *pdesc)
{
	pr_info("%s:%s Does not working.\n", MUIC_DEV_NAME, __func__);

	return 0;
}

static int sm5708_afc_error(struct regmap_desc *pdesc)
{
	pr_info("%s:%s Does not working.\n", MUIC_DEV_NAME, __func__);

	return 0;
}

static int sm5708_afc_init_check(struct regmap_desc *pdesc)
{
	pr_info("%s:%s Does not working.\n", MUIC_DEV_NAME, __func__);

	return 0;
}

static struct regmap_ops sm5708_muic_regmap_ops = {
	.get_size = sm5708_get_sizeof_regmap,
	.ioctl = sm5708_muic_ioctl,
	.get_formatted_dump = sm5708_get_fromatted_dump,
};

static struct vendor_ops sm5708_muic_vendor_ops = {
	.attach_ta = sm5708_attach_ta,
	.detach_ta = sm5708_detach_ta,
	.get_switch = sm5708_get_switching_mode,
	.set_switch = sm5708_set_switching_mode,
	.set_adc_scan_mode = sm5708_set_adc_scan_mode,
	.get_adc_scan_mode = sm5708_get_adc_scan_mode,
	.set_rustproof = sm5708_set_rustproof,
	.get_vps_data = sm5708_get_vps_data,
	.rescan = sm5708_run_BCD_rescan,
	.get_vbus_value = sm5708_get_vbus_value,
	.run_chgdet = sm5708_run_chgdet,
	.get_dcdtmr_irq = sm5708_get_dcdtmr_irq,
};

static struct afc_ops sm5708_muic_afc_ops = {
	.afc_ta_attach = sm5708_afc_ta_attach,
	.afc_ta_accept = sm5708_afc_ta_accept,
	.afc_vbus_update = sm5708_afc_vbus_update,
	.afc_multi_byte = sm5708_afc_multi_byte,
	.afc_error = sm5708_afc_error,
	.afc_ctrl_reg = sm5708_set_afc_ctrl_reg,
	.afc_init_check = sm5708_afc_init_check,
};

static struct regmap_desc sm5708_muic_regmap_desc = {
	.name = "sm5708-MUIC",
	.regmap = sm5708_muic_regmap_table,
	.size = REG_END,
	.regmapops = &sm5708_muic_regmap_ops,
	.vendorops = &sm5708_muic_vendor_ops,
	.afcops = &sm5708_muic_afc_ops,
};

void muic_register_sm5708_regmap_desc(struct regmap_desc **pdesc)
{
	*pdesc = &sm5708_muic_regmap_desc;
}
