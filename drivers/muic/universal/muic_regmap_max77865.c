/*
 * muic_regmap_max77865.c
 *
 * Copyright (C) 2016 Samsung Electronics
 * Insun Choi <insun77.choi@samsung.com>
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
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/host_notify.h>
#include <linux/string.h>

#include <linux/muic/muic.h>

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

/* Control2 Initialization */
#define INIT_CONTROL2	(0x00)

/*
    Control4 Initialization value : 00_1_1_0000
    [7:6] RSVD
    [5] FacAuto, 1:Factory Auto detectioni Enabled
    [4] USBAuto, 1:USB Auto detection Enabled(valid only if AccDet=1)
    [3:0] RSVD
*/
#define	INIT_CONTROL4	(0x30)

/* Workaround for CL65 */
#define INIT_CCDET	(0x00)

/* max77865 I2C registers */
enum max77865_muic_reg {
	REG_ID			= 0x00,
	REG_INT_MAIN		= 0x01,
	REG_INT_BC		= 0x02,
	REG_INT_FC		= 0x03,
	REG_INT_GP		= 0x04,
	REG_INTMASK_MAIN	= 0x07,
	REG_INTMASK_BC		= 0x08,
	REG_INTMASK_FC		= 0x09,
	REG_INTMASK_GP		= 0x0A,
	REG_STATUS1_BC		= 0x0D,
	REG_STATUS2_BC		= 0x0E,
	REG_STATUS_GP		= 0x0F,
	REG_CONTROL1_BC		= 0x13,
	REG_CONTROL2_BC		= 0x14,
	REG_CCDET		= 0x15,
	REG_CONTROL1		= 0x19,
	REG_CONTROL2		= 0x1A,
	REG_CONTROL3		= 0x1B,
	REG_CONTROL4		= 0x1C,
	REG_HVCONTROL1		= 0x1D,
	REG_HVCONTROL2		= 0x1E,

	REG_END,
};

/* UID table */
typedef enum {
	UID_JIG_USB_ON		= 0x0, /* 301Kohm */
	UID_JIG_UART_OFF	= 0x1, /* 523Kohm */
	UID_RSVD		= 0x2,
	UID_OPEN		= 0x3,
} muic_uid_t;

#define REG_ITEM(addr, bitp, mask) ((bitp<<16) | (mask<<8) | addr)

/* Field */
enum max77865_muic_reg_item {
	ID_CHIP_REV		= REG_ITEM(REG_ID, _BIT0, _MASK8),

	INT_MAIN_GP		= REG_ITEM(REG_INT_MAIN, _BIT4, _MASK1),
	INT_MAIN_FC		= REG_ITEM(REG_INT_MAIN, _BIT3, _MASK1),
	INT_MAIN_BC		= REG_ITEM(REG_INT_MAIN, _BIT1, _MASK1),

	INT_BC_VBVolt		= REG_ITEM(REG_INT_BC, _BIT7, _MASK1),
	INT_BC_DxOVP		= REG_ITEM(REG_INT_BC, _BIT6, _MASK1),
	INT_BC_DNVDATREF	= REG_ITEM(REG_INT_BC, _BIT5, _MASK1), /* = VDNMon */
	INT_BC_ChgDetRunF	= REG_ITEM(REG_INT_BC, _BIT4, _MASK1), /* Falling edge */
	INT_BC_ChgDetRunR	= REG_ITEM(REG_INT_BC, _BIT3, _MASK1), /* Rising edge */
	INT_BC_PrChgTyp		= REG_ITEM(REG_INT_BC, _BIT2, _MASK1),
	INT_BC_DCDTmr		= REG_ITEM(REG_INT_BC, _BIT1, _MASK1),
	INT_BC_ChgTyp		= REG_ITEM(REG_INT_BC, _BIT0, _MASK1),

	INT_FC_MRxRdy		= REG_ITEM(REG_INT_FC, _BIT7, _MASK1),
	INT_FC_MRxPerr		= REG_ITEM(REG_INT_FC, _BIT6, _MASK1),
	INT_FC_MRxTrf		= REG_ITEM(REG_INT_FC, _BIT5, _MASK1),
	INT_FC_MRxBufOw		= REG_ITEM(REG_INT_FC, _BIT4, _MASK1),
	INT_FC_MPNack		= REG_ITEM(REG_INT_FC, _BIT3, _MASK1),
	INT_FC_MPGDone		= REG_ITEM(REG_INT_FC, _BIT2, _MASK1),

	INT_GP_UIDGND		= REG_ITEM(REG_INT_GP, _BIT4, _MASK1),
	INT_GP_TSHUT		= REG_ITEM(REG_INT_GP, _BIT3, _MASK1),
	INT_GP_VbADC		= REG_ITEM(REG_INT_GP, _BIT2, _MASK1),
	INT_GP_RST		= REG_ITEM(REG_INT_GP, _BIT1, _MASK1),
	INT_GP_UID		= REG_ITEM(REG_INT_GP, _BIT0, _MASK1),

	INTMASK_MAIN_GP		= REG_ITEM(REG_INTMASK_MAIN, _BIT4, _MASK1),
	INTMASK_MAIN_FC		= REG_ITEM(REG_INTMASK_MAIN, _BIT3, _MASK1),
	INTMASK_MAIN_BC		= REG_ITEM(REG_INTMASK_MAIN, _BIT1, _MASK1),

	INTMASK_BC_VBVolt	= REG_ITEM(REG_INTMASK_BC, _BIT7, _MASK1),
	INTMASK_BC_DxOVP	= REG_ITEM(REG_INTMASK_BC, _BIT6, _MASK1),
	INTMASK_BC_DNVDATREF	= REG_ITEM(REG_INTMASK_BC, _BIT5, _MASK1),
	INTMASK_BC_ChgDetRunF	= REG_ITEM(REG_INTMASK_BC, _BIT4, _MASK1),
	INTMASK_BC_ChgDetRunR	= REG_ITEM(REG_INTMASK_BC, _BIT3, _MASK1),
	INTMASK_BC_PrChgTyp	= REG_ITEM(REG_INTMASK_BC, _BIT2, _MASK1),
	INTMASK_BC_DCDTmr	= REG_ITEM(REG_INTMASK_BC, _BIT1, _MASK1),
	INTMASK_BC_ChgTyp	= REG_ITEM(REG_INTMASK_BC, _BIT0, _MASK1),

	INTMASK_FC_MRxRdy	= REG_ITEM(REG_INTMASK_FC, _BIT7, _MASK1),
	INTMASK_FC_MRxPerr	= REG_ITEM(REG_INTMASK_FC, _BIT6, _MASK1),
	INTMASK_FC_MRxTrf	= REG_ITEM(REG_INTMASK_FC, _BIT5, _MASK1),
	INTMASK_FC_MRxBufOw	= REG_ITEM(REG_INTMASK_FC, _BIT4, _MASK1),
	INTMASK_FC_MPNack	= REG_ITEM(REG_INTMASK_FC, _BIT3, _MASK1),
	INTMASK_FC_MPGDone	= REG_ITEM(REG_INTMASK_FC, _BIT2, _MASK1),

	INTMASK_GP_UIDGND	= REG_ITEM(REG_INTMASK_GP, _BIT4, _MASK1),
	INTMASK_GP_TSHUT	= REG_ITEM(REG_INTMASK_GP, _BIT3, _MASK1),
	INTMASK_GP_VbADC	= REG_ITEM(REG_INTMASK_GP, _BIT2, _MASK1),
	INTMASK_GP_RST		= REG_ITEM(REG_INTMASK_GP, _BIT1, _MASK1),
	INTMASK_GP_UID		= REG_ITEM(REG_INTMASK_GP, _BIT0, _MASK1),

	STATUS1_BC_VBVolt	= REG_ITEM(REG_STATUS1_BC, _BIT7, _MASK1),
	STATUS1_BC_ChgDetRun	= REG_ITEM(REG_STATUS1_BC, _BIT6, _MASK1),
	STATUS1_BC_PrChgTyp	= REG_ITEM(REG_STATUS1_BC, _BIT3, _MASK3),
	STATUS1_BC_DCDTmr	= REG_ITEM(REG_STATUS1_BC, _BIT2, _MASK1),
	STATUS1_BC_ChgTyp	= REG_ITEM(REG_STATUS1_BC, _BIT0, _MASK2),

	STATUS2_BC_DxOVP	= REG_ITEM(REG_STATUS2_BC, _BIT1, _MASK1),
	STATUS2_BC_DNVDATREF	= REG_ITEM(REG_STATUS2_BC, _BIT0, _MASK1),

	STATUS_GP_UID		= REG_ITEM(REG_STATUS_GP, _BIT6, _MASK2),
	STATUS_GP_UIDGND	= REG_ITEM(REG_STATUS_GP, _BIT5, _MASK1),
	STATUS_GP_TSHUT		= REG_ITEM(REG_STATUS_GP, _BIT4, _MASK1),
	STATUS_GP_VbADC		= REG_ITEM(REG_STATUS_GP, _BIT0, _MASK4),

	CONTROL1_BC_DCDCpl	= REG_ITEM(REG_CONTROL1_BC, _BIT7, _MASK1),
	CONTROL1_BC_UIDEn	= REG_ITEM(REG_CONTROL1_BC, _BIT6, _MASK1),
	CONTROL1_BC_NoAutoIBUS	= REG_ITEM(REG_CONTROL1_BC, _BIT5, _MASK1),
	CONTROL1_BC_3ADCPDet	= REG_ITEM(REG_CONTROL1_BC, _BIT4, _MASK1),
	CONTROL1_BC_SfOutCtrl	= REG_ITEM(REG_CONTROL1_BC, _BIT2, _MASK2),
	CONTROL1_BC_ChgDetMan	= REG_ITEM(REG_CONTROL1_BC, _BIT1, _MASK1),
	CONTROL1_BC_ChgDetEn	= REG_ITEM(REG_CONTROL1_BC, _BIT0, _MASK1),

	CONTROL2_BC_DNMonEn	= REG_ITEM(REG_CONTROL2_BC, _BIT5, _MASK1),
	CONTROL2_BC_DPDNMan	= REG_ITEM(REG_CONTROL2_BC, _BIT4, _MASK1),
	CONTROL2_BC_DPDrv	= REG_ITEM(REG_CONTROL2_BC, _BIT2, _MASK2),
	CONTROL2_BC_DNDrv	= REG_ITEM(REG_CONTROL2_BC, _BIT0, _MASK2),

	CONTROL1_NoBCComp	= REG_ITEM(REG_CONTROL1, _BIT7, _MASK1),
	CONTROL1_RCPS		= REG_ITEM(REG_CONTROL1, _BIT6, _MASK1),
	CONTROL1_COMP2Sw	= REG_ITEM(REG_CONTROL1, _BIT3, _MASK3),
	CONTROL1_COMN1Sw	= REG_ITEM(REG_CONTROL1, _BIT0, _MASK3),

	CONTROL2_CPEn		= REG_ITEM(REG_CONTROL2, _BIT2, _MASK1),
	CONTROL2_CHGEnCtrl	= REG_ITEM(REG_CONTROL2, _BIT1, _MASK1),

	CONTROL3_RES3Set	= REG_ITEM(REG_CONTROL3, _BIT5, _MASK1),
	CONTROL3_RES2Set	= REG_ITEM(REG_CONTROL3, _BIT3, _MASK2),
	CONTROL3_RES1Set	= REG_ITEM(REG_CONTROL3, _BIT1, _MASK2),
	CONTROL3_JIGSet		= REG_ITEM(REG_CONTROL3, _BIT0, _MASK1),

	CONTROL4_FctAuto	= REG_ITEM(REG_CONTROL4, _BIT5, _MASK1),
	CONTROL4_USBAuto	= REG_ITEM(REG_CONTROL4, _BIT4, _MASK1),

	HVCONTROL1_VbusADCEn	= REG_ITEM(REG_HVCONTROL1, _BIT0, _MASK1),

	HVCONTROL2_FCRXDebEn	= REG_ITEM(REG_HVCONTROL2, _BIT7, _MASK1),
	HVCONTROL2_MPngEnb	= REG_ITEM(REG_HVCONTROL2, _BIT6, _MASK1),
	HVCONTROL2_MTxBusRes	= REG_ITEM(REG_HVCONTROL2, _BIT5, _MASK1),
	HVCONTROL2_MTxEn	= REG_ITEM(REG_HVCONTROL2, _BIT4, _MASK1),
	HVCONTROL2_MPing	= REG_ITEM(REG_HVCONTROL2, _BIT3, _MASK1),
	HVCONTROL2_DP06En	= REG_ITEM(REG_HVCONTROL2, _BIT1, _MASK1),
	HVCONTROL2_HVDigEn	= REG_ITEM(REG_HVCONTROL2, _BIT0, _MASK1),
};

/*
 * Path Switching (Control1)
 * D+ [5:3] / D- [2:0]
 * 000: Open all / 001: USB / 010: AUDIO / 011: UART / 100 : USB_CP / 101 : UART_CP
 */
enum {
	_ID_OPEN	= 0x0,
	_ID_BYPASS      = 0x1,
	_NO_BC_COMP_OFF	= 0x0,
	_NO_BC_COMP_ON	= 0x1,
	_D_OPEN	        = 0x0,
	_D_USB	        = 0x1,
	_D_AUDIO	= 0x2,
	_D_UART	        = 0x3,
	_D_USB_CP	= 0x4,
	_D_UART_CP      = 0x5,
};

/* COM patch Values */
#define COM_VALUE(dm, open, comp_onoff) ((dm<<3)|(dm<<0)|(open<<6)| \
					(comp_onoff<<7))

#define _COM_OPEN		COM_VALUE(_D_OPEN, _ID_OPEN, _NO_BC_COMP_ON)
#define _COM_OPEN_WITH_V_BUS	_COM_OPEN
#define _COM_UART_AP		COM_VALUE(_D_UART, _ID_OPEN, _NO_BC_COMP_ON)
#define _COM_UART_CP		COM_VALUE(_D_UART_CP, _ID_OPEN, _NO_BC_COMP_ON)
#define _COM_USB_CP		COM_VALUE(_D_USB_CP, _ID_OPEN, _NO_BC_COMP_ON)
#define _COM_USB_AP		COM_VALUE(_D_USB, _ID_OPEN, _NO_BC_COMP_ON)
#define _COM_AUDIO		COM_VALUE(_D_AUDIO, _ID_OPEN, _NO_BC_COMP_OFF)

static int max77865_com_value_tbl[] = {
	[COM_OPEN]		= _COM_OPEN,
	[COM_OPEN_WITH_V_BUS]	= _COM_OPEN_WITH_V_BUS,
	[COM_UART_AP]		= _COM_UART_AP,
	[COM_UART_CP]		= _COM_UART_CP,
	[COM_USB_AP]		= _COM_USB_AP,
	[COM_USB_CP]		= _COM_USB_CP,
	[COM_AUDIO]		= _COM_AUDIO,
};

static regmap_t max77865_muic_regmap_table[] = {
	[REG_ID]		= {"ID",		0x65, 0x00, INIT_NONE},
	[REG_INT_MAIN]		= {"INT_MAIN",		0x00, 0x00, INIT_INT_CLR,},
	[REG_INT_BC]		= {"INT_BC",		0x00, 0x00, INIT_INT_CLR,},
	[REG_INT_FC]		= {"INT_FC",		0x00, 0x00, INIT_INT_CLR,},
	[REG_INT_GP]		= {"INT_GP",		0x00, 0x00, INIT_INT_CLR,},
	[REG_INTMASK_MAIN]	= {"INTMASK_MAIN",	0x0F, 0x00, INIT_NONE,},
	[REG_INTMASK_BC]	= {"INTMASK_BC",	0xFF, 0x00, INIT_NONE,},
	[REG_INTMASK_FC]	= {"INTMASK_FC",	0xFF, 0x00, INIT_NONE,},
	[REG_INTMASK_GP]	= {"INTMASK_GP",	0xFD, 0x00, INIT_NONE,},
	[REG_STATUS1_BC]	= {"STATUS1_BC",	0x00, 0x00, INIT_NONE,},
	[REG_STATUS2_BC]	= {"STATUS2_BC",	0x00, 0x00, INIT_NONE,},
	[REG_STATUS_GP]		= {"STATUS_GP",		0xE0, 0x00, INIT_NONE,},
	[REG_CONTROL1_BC]	= {"CONTROL1_BC",	0xC5, 0x00, INIT_NONE,},
	[REG_CONTROL2_BC]	= {"CONTROL2_BC",	0x00, 0x00, INIT_NONE,},
	[REG_CCDET]		= {"CCDET",		0x00, 0x00, INIT_CCDET,},
	[REG_CONTROL1]		= {"CONTROL1",		0x00, 0x00, INIT_NONE,},
#if defined(CONFIG_SEC_FACTORY)
	[REG_CONTROL2]		= {"CONTROL2",		0x00, 0x00, INIT_NONE,},
#else
	[REG_CONTROL2]		= {"CONTROL2",		0x00, 0x00, INIT_CONTROL2,},
#endif
	[REG_CONTROL3]		= {"CONTROL3",		0x20, 0x00, INIT_NONE,},
	[REG_CONTROL4]		= {"CONTROL4",		0x30, 0x00, INIT_CONTROL4,},
	[REG_HVCONTROL1]	= {"HVCONTROL1",	0x00, 0x00, INIT_NONE,},
	[REG_HVCONTROL2]	= {"HVCONTROL2",	0x00, 0x00, INIT_NONE,},
	[REG_END]		= {NULL,		0, 0, INIT_NONE},
};

static int max77865_muic_ioctl(struct regmap_desc *pdesc,
		int arg1, int *arg2, int *arg3)
{
	int ret = 0;

	switch (arg1) {
	case GET_COM_VAL:
		*arg2 = max77865_com_value_tbl[*arg2];
		*arg3 = REG_CONTROL1;
		break;
	case GET_ADC:
		*arg3 = STATUS_GP_UID;
		break;
	case GET_REVISION:
		*arg3 = ID_CHIP_REV;
		break;
	case GET_OTG_STATUS:
		*arg3 = INTMASK_BC_VBVolt;
		break;

	case GET_CHGTYPE:
		*arg3 = STATUS1_BC_ChgTyp;
		break;

	case GET_PRCHGTYPE:
		*arg3 = STATUS1_BC_PrChgTyp;
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
static int max77865_attach_ta(struct regmap_desc *pdesc)
{
	int attr, value, ret;

	pr_info("%s\n", __func__);

	attr = REG_CONTROL1;
	value = _COM_OPEN;
	ret = regmap_write_value(pdesc, attr, value);
	if (ret < 0)
		pr_err("%s CNTR1 reg write fail.\n", __func__);
	else
		_REGMAP_TRACE(pdesc, 'w', ret, attr, value);

	return ret;
}

static int max77865_detach_ta(struct regmap_desc *pdesc)
{
	int attr, value, ret;

	pr_info("%s\n", __func__);

	attr = REG_CONTROL1 | _ATTR_OVERWRITE_M;
	value = _COM_OPEN;
	ret = regmap_write_value(pdesc, attr, value);
	if (ret < 0)
		pr_err("%s CNTR1 reg write fail.\n", __func__);
	else
		_REGMAP_TRACE(pdesc, 'w', ret, attr, value);

	return ret;
}

static int max77865_set_rustproof(struct regmap_desc *pdesc, int op)
{
	pr_info("%s: Not implemented.\n", __func__);

	return 0;
}

static int max77865_get_vps_data(struct regmap_desc *pdesc, void *pbuf)
{
	vps_data_t *pvps = (vps_data_t *)pbuf;
	int attr;

	attr = REG_ITEM(REG_STATUS1_BC, _BIT0, _MASK8);
	*(u8 *)&pvps->t.status[0] = regmap_read_value(pdesc, attr + 0); /* BC STATUS1 */
	*(u8 *)&pvps->t.status[1] = regmap_read_value(pdesc, attr + 1); /* BC STATUS2 */
	*(u8 *)&pvps->t.status[2] = regmap_read_value(pdesc, attr + 2); /* GP STATUS */

	attr = REG_ITEM(REG_CONTROL1_BC, _BIT0, _MASK8);
	*(u8 *)&pvps->t.bccontrol[0] = regmap_read_value(pdesc, attr + 0);
	*(u8 *)&pvps->t.bccontrol[1] = regmap_read_value(pdesc, attr + 1);

	attr = REG_ITEM(REG_CONTROL1, _BIT0, _MASK8);
	*(u8 *)&pvps->t.control[0] = regmap_read_value(pdesc, attr + 0);
	*(u8 *)&pvps->t.control[1] = regmap_read_value(pdesc, attr + 1);
	*(u8 *)&pvps->t.control[2] = regmap_read_value(pdesc, attr + 2);
	*(u8 *)&pvps->t.control[3] = regmap_read_value(pdesc, attr + 3);

	attr = REG_ITEM(REG_HVCONTROL1, _BIT0, _MASK8);
	*(u8 *)&pvps->t.hvcontrol[0] = regmap_read_value(pdesc, attr + 0);
	*(u8 *)&pvps->t.hvcontrol[1] = regmap_read_value(pdesc, attr + 1);

	attr = STATUS_GP_UID;
	*(u8 *)&pvps->t.uid = (pvps->t.status[2] >> _ATTR_BITP(attr)) & _ATTR_MASK(attr);
	switch(pvps->t.uid) {
	case 0: /* 301Kohm */
		pvps->t.adc = 0x19;
		break;
	case 1: /* 523Kohm */
		pvps->t.adc = 0x1c;
		break;
	case 2: /* RSVD */
		break;
	case 3: /* OPEN */
		pvps->t.adc = 0x1f;
		break;
	default:
		break;
	}

	attr = STATUS_GP_UIDGND;
	*(u8 *)&pvps->t.uidgnd = (pvps->t.status[2] >> _ATTR_BITP(attr)) & _ATTR_MASK(attr);

	attr = STATUS1_BC_VBVolt;
	*(u8 *)&pvps->t.vbvolt = (pvps->t.status[0] >> _ATTR_BITP(attr)) & _ATTR_MASK(attr);

	attr = STATUS1_BC_ChgDetRun;
	*(u8 *)&pvps->t.chgdetrun = (pvps->t.status[0] >> _ATTR_BITP(attr)) & _ATTR_MASK(attr);

	attr = STATUS1_BC_ChgTyp;
	*(u8 *)&pvps->t.chgtyp = (pvps->t.status[0] >> _ATTR_BITP(attr)) & _ATTR_MASK(attr);

	attr = STATUS1_BC_PrChgTyp;
	*(u8 *)&pvps->t.prchgtyp = (pvps->t.status[0] >> _ATTR_BITP(attr)) & _ATTR_MASK(attr);

	/* 1: timedout, 0: Not yet expired */
	attr = STATUS1_BC_DCDTmr;
	*(u8 *)&pvps->t.DCDTimedout = (pvps->t.status[0] >> _ATTR_BITP(attr)) & _ATTR_MASK(attr);

	return 0;
}

static int max77865_enable_accdet(struct regmap_desc *pdesc, int enable)
{
	pr_info("%s: %s\n", __func__, enable ? "Enable": "Disable");
	pr_info("%s: Not USE\n", __func__);
	return 0;
}

static int max77865_enable_chgdet(struct regmap_desc *pdesc, int enable)
{
	int attr, value, ret;

	pr_info("%s: %s\n", __func__, enable ? "Enable": "Disable");

	attr = CONTROL1_BC_ChgDetEn;
	value = enable ? 1: 0; /* 1: enable, 0: disable */
	ret = regmap_write_value(pdesc, attr, value);
	if (ret < 0)
		pr_err("%s CNTR1 reg write fail.\n", __func__);
	else
		_REGMAP_TRACE(pdesc, 'w', ret, attr, value);

	return ret;
}

#define BC_CONTROL1_CHGDETMAN_SHIFT	1
#define BC_CONTROL1_CHGDETMAN_MASK	(0x1 << BC_CONTROL1_CHGDETMAN_SHIFT)

static int max77865_run_chgdet(struct regmap_desc *pdesc, bool started)
{
	int value, ret, rvalue, wvalue;

	pr_info("%s: %s\n", __func__, started ? "started": "disabled");

	value = started ? 1 : 0; /* 0: Disabled, 1: Force a Manual Charger Detection */

	rvalue = muic_i2c_read_byte(pdesc->muic->i2c, REG_CONTROL1_BC);
	if (value)
		wvalue = rvalue | BC_CONTROL1_CHGDETMAN_MASK;
	else
		wvalue = rvalue & (~BC_CONTROL1_CHGDETMAN_MASK);

	ret = muic_i2c_write_byte(pdesc->muic->i2c, REG_CONTROL1_BC, wvalue);
	if (ret < 0)
		pr_err("%s CNTR1 reg write fail.\n", __func__);
	else
		pr_info("%s: 0x%02x -> 0x%02x\n", __func__, rvalue, wvalue);

	return ret;
}


static int max77865_start_otg_test(struct regmap_desc *pdesc, int started)
{
	pr_info("%s: %s\n", __func__, started ? "started": "stopped");

	if (started) {
		max77865_enable_chgdet(pdesc, 0);
		max77865_enable_accdet(pdesc, 1);
	} else 
		max77865_enable_chgdet(pdesc, 1);

	return 0;
}

static int max77865_get_adc_scan_mode(struct regmap_desc *pdesc)
{
	pr_info("%s: Not USE\n", __func__);
	return 0;
}

static void max77865_set_adc_scan_mode(struct regmap_desc *pdesc,
		const int mode)
{
	pr_info("%s: Not USE\n", __func__);
}

enum switching_mode_val{
	_SWMODE_AUTO = 0,
	_SWMODE_MANUAL = 1,
};

static int max77865_get_switching_mode(struct regmap_desc *pdesc)
{
	return SWMODE_AUTO;
}

static void max77865_set_switching_mode(struct regmap_desc *pdesc, int mode)
{
        int attr, value;
	int ret = 0;

	pr_info("%s\n",__func__);

	value = (mode == SWMODE_AUTO) ? _SWMODE_AUTO : _SWMODE_MANUAL;
	attr = CONTROL1_NoBCComp;
	ret = regmap_write_value(pdesc, attr, value);
	if (ret < 0)
		pr_err("%s REG_CTRL write fail.\n", __func__);
	else
		_REGMAP_TRACE(pdesc, 'w', ret, attr, value);
}

#if defined(CONFIG_MUIC_TEST_FUNC)
static int max77854_usb_to_ta(struct regmap_desc *pdesc, int mode)
{
	int ret = -1;
	muic_data_t *pmuic = pdesc->muic;
	vps_data_t *pmsr = &pmuic->vps;

	pr_info("%s\n", __func__);

	if (mode == 0) {
		pr_info("%s, Disable USB to TA\n", __func__);
		if (pmuic->attached_dev == ATTACHED_DEV_TA_MUIC && pmuic->usb_to_ta_state) {
			switch (pmsr->t.chgtyp) {
			case 1:
				pmuic->attached_dev = ATTACHED_DEV_USB_MUIC;
				break;
			case 2:
				pmuic->attached_dev = ATTACHED_DEV_CDP_MUIC;
				break;
			}
			muic_notifier_detach_attached_dev(ATTACHED_DEV_TA_MUIC);
			muic_notifier_attach_attached_dev(pmuic->attached_dev);
			pmuic->usb_to_ta_state = false;
		}
	} else if (mode == 1) {
		pr_info("%s, Enable USB to TA\n", __func__);
		if ((pdesc->muic->attached_dev == ATTACHED_DEV_CDP_MUIC ||
				pdesc->muic->attached_dev == ATTACHED_DEV_USB_MUIC)
				&& !pmuic->usb_to_ta_state) {
			muic_notifier_detach_attached_dev(pdesc->muic->attached_dev);
			muic_notifier_attach_attached_dev(ATTACHED_DEV_TA_MUIC);
			pmuic->attached_dev = ATTACHED_DEV_TA_MUIC;
			pmuic->usb_to_ta_state = true;
		}
	} else if (mode == 2) {
		pr_info("%s, USB to TA %s\n", __func__,
				pmuic->usb_to_ta_state?"Enabled":"Disabled");
		ret = pmuic->usb_to_ta_state;
	} else {
		pr_info("%s, Unknown CMD\n", __func__);
	}
	return ret;
}
#endif

static int max77865_get_vbus_value(struct regmap_desc *pdesc)
{
	muic_data_t *muic = pdesc->muic;
	int vbadc, result = 0;
	int adcen;
	pr_info("%s\n",__func__);

	adcen = i2c_smbus_read_byte_data(muic->i2c, REG_HVCONTROL1);
	i2c_smbus_write_byte_data(muic->i2c, REG_HVCONTROL1, adcen | 0x01);
	msleep(100);
	vbadc = regmap_read_value(pdesc, STATUS_GP_VbADC);

	switch (vbadc) {
	case 0:
		result = 0;
		break;
	case 1:
	case 2:
	case 3:
		result = 5;
		break;
	case 4:
		result = vbadc+3;
		break;
	case 5:
	case 6:
		result = 9;
		break;
	case 7:
	case 8:
	case 9:
		result = 12;
		break;
	case (10)...(15):
		result = vbadc+4;
		break;
	default:
		break;
	}

	i2c_smbus_write_byte_data(muic->i2c, REG_HVCONTROL1, adcen);
	return result;
}

static void max77865_get_fromatted_dump(struct regmap_desc *pdesc, char *mesg)
{
	muic_data_t *muic = pdesc->muic;
	int val;

	if (pdesc->trace)
		pr_info("%s\n", __func__);

	val = i2c_smbus_read_byte_data(muic->i2c, REG_STATUS1_BC);
	sprintf(mesg+strlen(mesg), "ST1_BC:%x ", val);
	val = i2c_smbus_read_byte_data(muic->i2c, REG_STATUS2_BC);
	sprintf(mesg+strlen(mesg), "ST2_BC:%x ", val);
	val = i2c_smbus_read_byte_data(muic->i2c, REG_STATUS_GP);
	sprintf(mesg+strlen(mesg), "ST_GP:%x ", val);

	val = i2c_smbus_read_byte_data(muic->i2c, REG_INTMASK_MAIN);
	sprintf(mesg+strlen(mesg), "IM_MAIN:%x ", val);
	val = i2c_smbus_read_byte_data(muic->i2c, REG_INTMASK_BC);
	sprintf(mesg+strlen(mesg), "IM_BC:%x ", val);
	val = i2c_smbus_read_byte_data(muic->i2c, REG_INTMASK_FC);
	sprintf(mesg+strlen(mesg), "IM_FC:%x ", val);
	val = i2c_smbus_read_byte_data(muic->i2c, REG_INTMASK_GP);
	sprintf(mesg+strlen(mesg), "IM_GP:%x ", val);

	val = i2c_smbus_read_byte_data(muic->i2c, REG_CONTROL1_BC);
	sprintf(mesg+strlen(mesg), "CT1_BC:%x ", val);
	val = i2c_smbus_read_byte_data(muic->i2c, REG_CONTROL2_BC);
	sprintf(mesg+strlen(mesg), "CT2_BC:%x ", val);

	val = i2c_smbus_read_byte_data(muic->i2c, REG_CONTROL1);
	sprintf(mesg+strlen(mesg), "CT1:%x ", val);
	val = i2c_smbus_read_byte_data(muic->i2c, REG_CONTROL2);
	sprintf(mesg+strlen(mesg), "CT2:%x ", val);
	val = i2c_smbus_read_byte_data(muic->i2c, REG_CONTROL3);
	sprintf(mesg+strlen(mesg), "CT3:%x ", val);
	val = i2c_smbus_read_byte_data(muic->i2c, REG_CONTROL4);
	sprintf(mesg+strlen(mesg), "CT4:%x ", val);

	val = i2c_smbus_read_byte_data(muic->i2c, REG_HVCONTROL1);
	sprintf(mesg+strlen(mesg), "HV1:%x ", val);
	val = i2c_smbus_read_byte_data(muic->i2c, REG_HVCONTROL2);
	sprintf(mesg+strlen(mesg), "HV2:%x ", val);
}

static int max77865_get_sizeof_regmap(void)
{
	pr_info("%s:%s\n", MUIC_DEV_NAME, __func__);
	return (int)ARRAY_SIZE(max77865_muic_regmap_table);
}

/* Need to implement */
void _max77865_muic_read_register(struct i2c_client *i2c)
{
	pr_info("%s: Not implemented.\n", __func__);
}

int max77865_muic_read_register(struct i2c_client *i2c)
	__attribute__((weak, alias("_max77865_muic_read_register")));

static int max77865_init_reg_func(struct regmap_desc *pdesc)
{
	pr_info("%s\n", __func__);

	return 0;
}

static struct regmap_ops max77865_muic_regmap_ops = {
	.get_size = max77865_get_sizeof_regmap,
	.ioctl = max77865_muic_ioctl,
	.get_formatted_dump = max77865_get_fromatted_dump,
	.init = max77865_init_reg_func,
};

static struct vendor_ops max77865_muic_vendor_ops = {
	.attach_ta = max77865_attach_ta,
	.detach_ta = max77865_detach_ta,
	.get_switch = max77865_get_switching_mode,
	.set_switch = max77865_set_switching_mode,
	.set_adc_scan_mode = max77865_set_adc_scan_mode,
	.get_adc_scan_mode =  max77865_get_adc_scan_mode,
	.set_rustproof = max77865_set_rustproof,
	.enable_accdet = max77865_enable_accdet,
	.enable_chgdet = max77865_enable_chgdet,
	.run_chgdet = max77865_run_chgdet,
	.start_otg_test = max77865_start_otg_test,
	.get_vps_data = max77865_get_vps_data,
	.get_vbus_value = max77865_get_vbus_value,
#if defined(CONFIG_MUIC_TEST_FUNC)
	.usb_to_ta = max77854_usb_to_ta,
#endif
};

static struct regmap_desc max77865_muic_regmap_desc = {
	.name = "max77865-MUIC",
	.regmap = max77865_muic_regmap_table,
	.size = REG_END,
	.regmapops = &max77865_muic_regmap_ops,
	.vendorops = &max77865_muic_vendor_ops,
};

void muic_register_max77865_regmap_desc(struct regmap_desc **pdesc)
{
	*pdesc = &max77865_muic_regmap_desc;
}
