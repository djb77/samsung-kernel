/*
 * muic_task.c
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
#include "muic_apis.h"
#include "muic_state.h"
#include "muic_vps.h"
#include "muic_i2c.h"
#include "muic_sysfs.h"
#include "muic_debug.h"
#include "muic_dt.h"
#include "muic_regmap.h"
#include "muic_coagent.h"
#include "muic_debug.h"

#if defined(CONFIG_MUIC_SUPPORT_CCIC)
#include "muic_ccic.h"
#endif
#if defined(CONFIG_USB_EXTERNAL_NOTIFY)
#include "muic_usb.h"
#endif

#define MUIC_INT_DETACH_MASK	(0x1 << 1)
#define MUIC_INT_ATTACH_MASK	(0x1 << 0)
#if !defined(CONFIG_MUIC_UNIVERSAL_SM5504)
#define MUIC_INT_OVP_EN_MASK	(0x1 << 5)
#define MUIC_INT_VBUS_OFF_MASK	(0x1 << 0)
#else
#define MUIC_INT_OVP_EN_MASK	(0x1 << 7)
#endif

#if defined(CONFIG_MUIC_UNIVERSAL_SM5504)
#define MUIC_REG_CTRL_RESET_VALUE	(0xE5)
#else
#define MUIC_REG_CTRL_RESET_VALUE	(0x1F)
#endif

#define MUIC_REG_CTRL	0x02
#define MUIC_REG_INT1	0x03
#define MUIC_REG_INT2	0x04
#define MUIC_REG_INT3	0x05
#define MUIC_REG_MANSW1	0x13

#if defined(CONFIG_MUIC_SUPPORT_EARJACK)
#define REG_BTN1		0x0c
#define REG_BTN2		0x0d
#define REG_VBUS		0x1d
#endif
extern struct muic_platform_data muic_pdata;

static bool muic_online = false;

bool muic_is_online(void)
{
	return muic_online;
}

#if defined(CONFIG_MUIC_UNIVERSAL_SM5705_AFC)
extern void muic_afc_delay_check_state(int state);
/* SM5705 Interrupt 3  AFC register */
#define INT3_AFC_ERROR_SHIFT         5
#define INT3_AFC_STA_CHG_SHIFT       4
#define INT3_AFC_MULTI_BYTE_SHIFT    3
#define INT3_AFC_VBUS_UPDATE_SHIFT   2
#define INT3_AFC_ACCEPTED_SHIFT      1
#define INT3_AFC_TA_ATTACHED_SHIFT   0

#define INT3_AFC_ERROR_MASK          (1 << INT3_AFC_ERROR_SHIFT)
#define INT3_AFC_STA_CHG_MASK        (1 << INT3_AFC_STA_CHG_SHIFT)
#define INT3_AFC_MULTI_BYTE_MASK     (1 << INT3_AFC_MULTI_BYTE_SHIFT)
#define INT3_AFC_VBUS_UPDATE_MASK    (1 << INT3_AFC_VBUS_UPDATE_SHIFT)
#define INT3_AFC_ACCEPTED_MASK       (1 << INT3_AFC_ACCEPTED_SHIFT)
#define INT3_AFC_TA_ATTACHED_MASK    (1 << INT3_AFC_TA_ATTACHED_SHIFT)

static int muic_irq_handler_afc(muic_data_t *pmuic, int irq)
{
	struct i2c_client *i2c = pmuic->i2c;
	struct afc_ops *afcops = pmuic->regmapdesc->afcops;
	int intr1, intr2, intr3, ret;

	pr_info("%s:%s irq(%d)\n", pmuic->chip_name, __func__, irq);

	/* read and clear interrupt status bits */
	intr1 = muic_i2c_read_byte(i2c, MUIC_REG_INT1);
	intr2 = muic_i2c_read_byte(i2c, MUIC_REG_INT2);
	intr3 = muic_i2c_read_byte(i2c, MUIC_REG_INT3);

	if ((intr1 < 0) || (intr2 < 0)) {
		pr_err("%s: err read interrupt status [1:0x%x, 2:0x%x]\n",
				__func__, intr1, intr2);
		return INT_REQ_DISCARD;
	}

	if (intr1 & MUIC_INT_ATTACH_MASK)
	{
		int intr_tmp;
		intr_tmp = muic_i2c_read_byte(i2c, MUIC_REG_INT1);
		if (intr_tmp & 0x2)
		{
			pr_info("%s:%s attach/detach interrupt occurred\n",
				pmuic->chip_name, __func__);
			intr1 &= 0xFE;
		}
		intr1 |= intr_tmp;
	}

	if (intr1 & MUIC_INT_DETACH_MASK) {
		cancel_delayed_work(&pmuic->afc_retry_work);
		cancel_delayed_work(&pmuic->afc_restart_work);
		muic_afc_delay_check_state(0);
	}

	pr_info("%s:%s intr[1:0x%x, 2:0x%x, 3:0x%x]\n", pmuic->chip_name, __func__,
			intr1, intr2, intr3);

	/* check for muic reset and recover for every interrupt occurred */
	if ((intr1 & MUIC_INT_OVP_EN_MASK) ||
		((intr1 == 0) && (intr2 == 0) && (intr3 == 0) && (irq != -1)))
	{
		int ctrl;
		ctrl = muic_i2c_read_byte(i2c, MUIC_REG_CTRL);
		if (ctrl == 0x1F)
		{
			/* CONTROL register is reset to 1F */
			muic_print_reg_log();
			muic_print_reg_dump(pmuic);
			pr_err("%s: err muic could have been reseted. Initilize!!\n",
				__func__);
			muic_reg_init(pmuic);
			muic_print_reg_dump(pmuic);

			/* MUIC Interrupt On */
			set_int_mask(pmuic, false);
		}

		if ((intr1 & MUIC_INT_ATTACH_MASK) == 0)
			return INT_REQ_DISCARD;
	}

	// scan fail check
	if( (intr1==0x01) && (intr2==0x00) && (intr3==0x00) )
	{
		int val1, val2, val3, adc, vbvolt;

		val1   = muic_i2c_read_byte(i2c,0x0A);   //REG_DEVT1
		val2   = muic_i2c_read_byte(i2c,0x0B);   //REG_DEVT2
		val3   = muic_i2c_read_byte(i2c,0x0C);   //REG_DEVT3
		adc    = muic_i2c_read_byte(i2c,0x09);   //REG_ADC
		vbvolt = muic_i2c_read_byte(i2c,0x15);   //REG_RSVDID1
        
		if ( (val1==0x00) && (val2==0x00) && (val3==0x00) && (adc=0x1F) && (vbvolt==0x00) )
		{
			pr_info("%s:Scan Fail :  MUIC reset \n", __func__);
			//MUIC Reset
			muic_i2c_write_byte(i2c, 0x22, 0x01);

			muic_reg_init(pmuic);
			muic_print_reg_dump(pmuic);

			/* MUIC Interrupt On */
			set_int_mask(pmuic, false);

			return INT_REQ_DISCARD;
		}
	}

	pmuic->intr.intr1 = intr1;
	pmuic->intr.intr2 = intr2;
	pmuic->intr.intr3 = intr3;

	if ((irq == -1) && (intr3 & INT3_AFC_TA_ATTACHED_MASK))
	{
		ret = muic_i2c_write_byte(i2c, 0x0F, 0x01);
		if(ret < 0){
			pr_err("%s: err write register value \n", __func__);
			return INT_REQ_DISCARD;
		}
		return INT_REQ_DONE;
	}

	if ((intr1 & MUIC_INT_DETACH_MASK) && (intr2 & MUIC_INT_VBUS_OFF_MASK))
		return INT_REQ_DONE;

	if (pmuic->pdata->afc_disable)
		pr_err("%s: Ignore AFC INTS, AFC is diabled\n", __func__);
	else {
		if (intr3 & INT3_AFC_TA_ATTACHED_MASK) {  /*AFC_TA_ATTACHED*/
			ret = afcops->afc_ta_attach(pmuic->regmapdesc);
			if (intr1 & MUIC_INT_DETACH_MASK) {
				return INT_REQ_DONE;
			} else {
				return ret;
			}
		} else if (intr3 & INT3_AFC_ACCEPTED_MASK) {  /*AFC_ACCEPTED*/
			ret = afcops->afc_ta_accept(pmuic->regmapdesc);
			if (intr1 & MUIC_INT_DETACH_MASK) {
				return INT_REQ_DONE;
			} else {
				return ret;
			}
		} else if (intr3 & INT3_AFC_VBUS_UPDATE_MASK) {  /*AFC_VBUS_UPDATE*/
			ret = afcops->afc_vbus_update(pmuic->regmapdesc);
			if (intr1 & MUIC_INT_DETACH_MASK) {
				return INT_REQ_DONE;
			} else {
				return ret;
			}
		} else if (intr3 & INT3_AFC_MULTI_BYTE_MASK) {  /*AFC_MULTI_BYTE*/
			ret = afcops->afc_multi_byte(pmuic->regmapdesc);
			if (intr1 & MUIC_INT_DETACH_MASK) {
				return INT_REQ_DONE;
			} else {
				return ret;
			}
		} else if (intr3 & INT3_AFC_ERROR_MASK) {  /*AFC_ERROR*/
			cancel_delayed_work(&pmuic->afc_retry_work);
			ret = afcops->afc_error(pmuic->regmapdesc);
			if (intr1 & MUIC_INT_DETACH_MASK) {
				return INT_REQ_DONE;
			} else {
				return ret;
			}
		} else if (intr3 & INT3_AFC_STA_CHG_MASK) {  /*AFC_STA_CHG*/
			if (intr1 & MUIC_INT_DETACH_MASK) {
				return INT_REQ_DONE;
			} else {
				return 0;
			}
		}
	}

	return INT_REQ_DONE;
}
#endif

#if defined(CONFIG_MUIC_UNIVERSAL_SM5708)
static int sm5708_muic_irq_handler(muic_data_t *pmuic, int irq)
{
	struct i2c_client *i2c = pmuic->i2c;
	int intr1, intr2, intr3;

	pr_info("%s:%s irq(%d)\n", pmuic->chip_name, __func__, irq);

	/* read and clear interrupt status bits */
	intr1 = muic_i2c_read_byte(i2c, MUIC_REG_INT1);
	intr2 = muic_i2c_read_byte(i2c, MUIC_REG_INT2);
	intr3 = muic_i2c_read_byte(i2c, MUIC_REG_INT3);

	if ((intr1 < 0) || (intr2 < 0)) {
		pr_err("%s: err read interrupt status [1:0x%x, 2:0x%x]\n",
				__func__, intr1, intr2);
		return INT_REQ_DISCARD;
	}

	if (intr1 & MUIC_INT_ATTACH_MASK) {
		int intr_tmp;

		intr_tmp = muic_i2c_read_byte(i2c, MUIC_REG_INT1);
		if (intr_tmp & 0x2) {
			pr_info("%s:%s attach/detach interrupt occurred\n",
				pmuic->chip_name, __func__);
			intr1 &= 0xFE;
		}
		intr1 |= intr_tmp;
	}

	//if (intr1 & MUIC_INT_DETACH_MASK) {
	//
	//}

	pr_info("%s:%s intr[1:0x%x, 2:0x%x, 3:0x%x]\n", pmuic->chip_name, __func__,
			intr1, intr2, intr3);

	/* check for muic reset and recover for every interrupt occurred */
	if ((intr1 == 0) && (intr2 == 0) && (intr3 == 0) && (irq != -1)) {
		int ctrl;

		ctrl = muic_i2c_read_byte(i2c, MUIC_REG_CTRL);
		if (ctrl == 0x1F) {
			/* CONTROL register is reset to 1F */
			muic_print_reg_log();
			muic_print_reg_dump(pmuic);
			pr_err("%s: err muic could have been reseted. Initialize!!\n",
				__func__);
			muic_reg_init(pmuic);
			muic_print_reg_dump(pmuic);

			/* MUIC Interrupt On */
			set_int_mask(pmuic, false);
		}

		if ((intr1 & MUIC_INT_ATTACH_MASK) == 0)
			return INT_REQ_DISCARD;
	}

	// scan fail check
	if ((intr1 == 0x01) && (intr2 == 0x00) && (intr3 == 0x00)) {
		int val1, val2, val3, adc, vbvolt;

		val1   = muic_i2c_read_byte(i2c, 0x0A);   //REG_DEVT1
		val2   = muic_i2c_read_byte(i2c, 0x0B);   //REG_DEVT2
		val3   = muic_i2c_read_byte(i2c, 0x0C);   //REG_DEVT3
		adc    = muic_i2c_read_byte(i2c, 0x09);   //REG_ADC
		vbvolt = muic_i2c_read_byte(i2c, 0x15);   //REG_RSVDID1

		if ((val1 == 0x00) && (val2 == 0x00) && (val3 == 0x00) && (adc == 0x1F) && (vbvolt == 0x00)) {
			pr_info("%s:Scan Fail :  MUIC reset\n", __func__);
			//MUIC Reset
			muic_i2c_write_byte(i2c, 0x22, 0x01);

			muic_reg_init(pmuic);
			muic_print_reg_dump(pmuic);

			/* MUIC Interrupt On */
			set_int_mask(pmuic, false);

			return INT_REQ_DISCARD;
		}
	}

	pmuic->intr.intr1 = intr1;
	pmuic->intr.intr2 = intr2;
	pmuic->intr.intr3 = intr3;

	return INT_REQ_DONE;
}
#endif

#if (defined(CONFIG_MUIC_UNIVERSAL_MULTI_SUPPORT) || !(defined(CONFIG_MUIC_UNIVERSAL_SM5705) || defined(CONFIG_MUIC_UNIVERSAL_SM5708)))
static int muic_irq_handler(muic_data_t *pmuic, int irq)
{
	struct i2c_client *i2c = pmuic->i2c;
	int intr1, intr2;
	int ctrl;
	int intr_tmp;
#if defined(CONFIG_MUIC_SUPPORT_EARJACK)
	int btn1, btn2, vbus;
#endif

	pr_info("%s:%s irq(%d)\n", pmuic->chip_name, __func__, irq);

#if (defined(CONFIG_MUIC_UNIVERSAL_SM5504) || (defined(CONFIG_MUIC_UNIVERSAL_SM5703) && defined(CONFIG_MUIC_SUPPORT_EARJACK)))
	/* SM5504 needs 100ms delay 
	 * SM5703 : sometimes VBUS interrupt is delayed too long, when EARJACK CONFIG is enabled.
	 */
	msleep(100);
#endif

	/* read and clear interrupt status bits */
	intr1 = muic_i2c_read_byte(i2c, MUIC_REG_INT1);
	intr2 = muic_i2c_read_byte(i2c, MUIC_REG_INT2);

	if ((intr1 < 0) || (intr2 < 0)) {
		pr_err("%s: err read interrupt status [1:0x%x, 2:0x%x]\n",
				__func__, intr1, intr2);
		return INT_REQ_DISCARD;
	}

	if (intr1 & MUIC_INT_ATTACH_MASK) {
		intr_tmp = muic_i2c_read_byte(i2c, MUIC_REG_INT1);
		if (intr_tmp & 0x2) {
			pr_info("%s:%s attach/detach interrupt occurred\n",
				pmuic->chip_name, __func__);
			intr1 &= 0xFE;
		}
		intr1 |= intr_tmp;
	}
#if defined(CONFIG_MUIC_SUPPORT_EARJACK)
	btn1 = muic_i2c_read_byte(i2c, REG_BTN1);
	btn2 = muic_i2c_read_byte(i2c, REG_BTN2);
	vbus = muic_i2c_read_byte(i2c, REG_VBUS);
	ctrl = muic_i2c_read_byte(i2c, MUIC_REG_CTRL);

	if (irq == -1) {
		if (!vbus && !intr1 && !intr2 && (btn1 || btn2)) {
			ctrl = ctrl | (0 << 0);
			pr_info("MUIC : need restart for setting earjack.\n");
			muic_i2c_write_byte(i2c, 0x1B, 0x1);
			muic_i2c_write_byte(i2c, MUIC_REG_CTRL, ctrl);
			msleep(100);
	
			btn1 = muic_i2c_read_byte(i2c, REG_BTN1);
			btn2 = muic_i2c_read_byte(i2c, REG_BTN2);
		}
	}

	muic_print_reg_dump(pmuic);
	set_earjack_state(pmuic, intr1, intr2, btn1, btn2);
#endif

	pr_info("%s:%s intr[1:0x%x, 2:0x%x]\n", pmuic->chip_name, __func__,
			intr1, intr2);

	ctrl = muic_i2c_read_byte(i2c, MUIC_REG_CTRL);

	/* check for muic reset and recover for every interrupt occurred */
	if ((intr1 & MUIC_INT_OVP_EN_MASK) ||
		((ctrl == MUIC_REG_CTRL_RESET_VALUE) && (irq != -1))) {
		if (ctrl == MUIC_REG_CTRL_RESET_VALUE) {
			/* CONTROL register is reset to 1F */
			muic_print_reg_log();
			muic_print_reg_dump(pmuic);
			pr_err("%s: err muic could have been reseted. Initilize!!\n",
				__func__);
			muic_reg_init(pmuic);
			muic_print_reg_dump(pmuic);

			/* MUIC Interrupt On */
			set_int_mask(pmuic, false);
		}

		if ((intr1 & MUIC_INT_ATTACH_MASK) == 0)
			return INT_REQ_DISCARD;
	}

	// scan fail check
	if( (intr1==0x01) && (intr2==0x00) )
	{
		int val1, val2, val3, adc;

		val1   = muic_i2c_read_byte(i2c,0x0A);   //REG_DEVT1
		val2   = muic_i2c_read_byte(i2c,0x0B);   //REG_DEVT2
		val3   = muic_i2c_read_byte(i2c,0x15);   //REG_DEVT3
		adc    = muic_i2c_read_byte(i2c,0x07);   //REG_ADC
        
		if ( (val1==0x00) && (val2==0x00) && (val3==0x00) && (adc=0x1F) && (pmuic->muic_reset_count < 1) )
		{
			pr_info("%s:Scan Fail :  MUIC reset \n", __func__);
			//MUIC Reset
			muic_i2c_write_byte(i2c, 0x1B, 0x01);

			muic_reg_init(pmuic);
			muic_print_reg_dump(pmuic);

			/* MUIC Interrupt On */
			set_int_mask(pmuic, false);

			pmuic->muic_reset_count++;
			pr_info("%s:%s muic_reset_count = %d \n", MUIC_DEV_NAME, __func__, pmuic->muic_reset_count);

			return INT_REQ_DISCARD;
		}
	}

	pmuic->intr.intr1 = intr1;
	pmuic->intr.intr2 = intr2;

	return INT_REQ_DONE;
}
#endif

enum max_intr_bits {
	INTR1_ADC1K_MASK = (1<<3),
	INTR1_ADCERR_MASK = (1<<2),
	INTR1_ADC_MASK   = (1<<0),

	INTR2_VBVOLT_MASK    = (1<<4),
	INTR2_OVP_MASK       = (1<<3),
	INTR2_DCTTMR_MASK    = (1<<2),
	INTR2_CHGDETRUN_MASK = (1<<1),
	INTR2_CHGTYP_MASK    = (1<<0),
};

#define MAX77849_REG_INT1 (0x01)
#define MAX77849_REG_INT2 (0x02)

static irqreturn_t max77849_muic_irq_handler(muic_data_t *pmuic, int irq)
{
	u8 intr1, intr2;

	pr_info("%s:%s irq:%d\n", MUIC_DEV_NAME, __func__, irq);

	intr1 = muic_i2c_read_byte(pmuic->i2c, MAX77849_REG_INT1);
	intr2 = muic_i2c_read_byte(pmuic->i2c, MAX77849_REG_INT2);

	if (intr1 & INTR1_ADC1K_MASK)
		pr_info("%s ADC1K interrupt occured\n", __func__);

	if (intr1 & INTR1_ADCERR_MASK)
		pr_info("%s ADC1K interrupt occured\n", __func__);

	if (intr1 & INTR1_ADCERR_MASK)
		pr_info("%s ADCERR interrupt occured\n", __func__);

	if (intr2 & INTR2_VBVOLT_MASK)
		pr_info("%s VBVOLT interrupt occured\n", __func__);

	if (intr2 & INTR2_OVP_MASK)
		pr_info("%s OVP interrupt occured\n", __func__);

	if (intr2 & INTR2_DCTTMR_MASK)
		pr_info("%s DCTTMR interrupt occured\n", __func__);

	if (intr2 & INTR2_CHGTYP_MASK)
		pr_info("%s DCTTMR interrupt occured\n", __func__);

	return INT_REQ_DONE;
}

static irqreturn_t muic_irq_thread(int irq, void *data)
{
	muic_data_t *pmuic = data;

	mutex_lock(&pmuic->muic_mutex);
	pmuic->irq_n = irq;
	if (pmuic->vps_table == VPS_TYPE_TABLE) {
		if (max77849_muic_irq_handler(pmuic, irq) & INT_REQ_DONE)
			muic_detect_dev(pmuic);
	} else{
#if defined(CONFIG_MUIC_UNIVERSAL_MULTI_SUPPORT)
        if (pmuic->is_afc_support) {
 		if (muic_irq_handler_afc(pmuic, irq) & INT_REQ_DONE)
        		muic_detect_dev(pmuic);
        } else {
		if (muic_irq_handler(pmuic, irq) & INT_REQ_DONE)
			muic_detect_dev(pmuic);
	}
#elif defined(CONFIG_MUIC_UNIVERSAL_SM5705_AFC)
        if (muic_irq_handler_afc(pmuic, irq) & INT_REQ_DONE)
             	muic_detect_dev(pmuic);
#elif defined(CONFIG_MUIC_UNIVERSAL_SM5708)
	if (sm5708_muic_irq_handler(pmuic, irq) & INT_REQ_DONE)
		muic_detect_dev(pmuic);
#else
		if (muic_irq_handler(pmuic, irq) & INT_REQ_DONE)
			muic_detect_dev(pmuic);
#endif
	}

	mutex_unlock(&pmuic->muic_mutex);

	return IRQ_HANDLED;
}

static void muic_init_detect(struct work_struct *work)
{
	muic_data_t *pmuic =
		container_of(work, muic_data_t, init_work.work);

	pr_info("%s:%s\n", pmuic->chip_name, __func__);

	/* MUIC Interrupt On */
	set_int_mask(pmuic, false);

	muic_irq_thread(-1, pmuic);
}

static int muic_irq_init(muic_data_t *pmuic)
{
	struct i2c_client *i2c = pmuic->i2c;
	struct muic_platform_data *pdata = pmuic->pdata;
	int ret = 0;

	pr_info("%s:%s\n", pmuic->chip_name, __func__);

	if (!pdata->irq_gpio) {
		pr_warn("%s:%s No interrupt specified\n", pmuic->chip_name,
				__func__);
		return -ENXIO;
	}

	i2c->irq = gpio_to_irq(pdata->irq_gpio);

	if (i2c->irq) {
		ret = request_threaded_irq(i2c->irq, NULL,
				muic_irq_thread,
				(IRQF_TRIGGER_LOW | IRQF_ONESHOT),
				"muic-irq", pmuic);
		if (ret < 0) {
			pr_err("%s:%s failed to reqeust IRQ(%d)\n",
					pmuic->chip_name, __func__, i2c->irq);
			return ret;
		}

		ret = enable_irq_wake(i2c->irq);
		if (ret < 0)
			pr_err("%s:%s failed to enable wakeup src\n",
					pmuic->chip_name, __func__);
	}

	return ret;
}

#if defined(CONFIG_MUIC_SLAVE_MODE_CONTROL_VBUS)
static muic_data_t *g_pmuic;
int manual_control_vbus_sw(bool slate_mode)
{
	u8 reg_data;
	
	if (slate_mode) {
		reg_data = muic_i2c_read_byte(g_pmuic->i2c, MUIC_REG_MANSW1);
		reg_data = (reg_data & 0xfc) | (0x0 & 0x3);
		muic_i2c_write_byte(g_pmuic->i2c, MUIC_REG_MANSW1, reg_data);
		pr_info("%s SM5703 : VBUS_SW = OPEN\n",__func__);
	} else {
		reg_data = muic_i2c_read_byte(g_pmuic->i2c, MUIC_REG_MANSW1);
		reg_data = (reg_data & 0xfc) | (0x1 & 0x3);
		muic_i2c_write_byte(g_pmuic->i2c, MUIC_REG_MANSW1, reg_data);
		pr_info("%s SM5703 : VBUS_SW = CONNECTED VBUS_OUT\n",__func__);
	}
	return 0;
}
#endif
static int muic_probe(struct i2c_client *i2c,
				const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(i2c->dev.parent);
	struct muic_platform_data *pdata = &muic_pdata;
	muic_data_t *pmuic;
	struct regmap_desc *pdesc = NULL;
	struct regmap_ops *pops = NULL;
	int ret = 0;

#if defined(CONFIG_MUIC_SUPPORT_EARJACK)
	struct input_dev *input;
	struct class *audio;
	struct device *earjack;
#endif
	pr_info("%s:%s\n", MUIC_DEV_NAME, __func__);

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
		pr_err("%s: i2c functionality check error\n", __func__);
		ret = -EIO;
		goto err_return;
	}

	pmuic = kzalloc(sizeof(muic_data_t), GFP_KERNEL);
	if (!pmuic) {
		pr_err("%s: failed to allocate driver data\n", __func__);
		ret = -ENOMEM;
		goto err_return;
	}

	i2c->dev.platform_data = pdata;

	i2c_set_clientdata(i2c, pmuic);

#if defined(CONFIG_MUIC_SUPPORT_EARJACK)
	input = input_allocate_device();
	if (!input) {
		pr_err("%s: failed to allocate input\n", __func__);
		ret = -ENOMEM;
		goto err_return;
	}
#endif

#if defined(CONFIG_OF)
	ret = of_muic_dt(i2c, pdata);
	if (ret < 0) {
		pr_err("%s:%s Failed to get device of_node \n",
				MUIC_DEV_NAME, __func__);
		goto err_io;
	}

#if defined(CONFIG_MUIC_PINCTRL)
	ret = of_muic_pinctrl(i2c);
	if (ret < 0) {
		pr_err("%s:%s Failed to set pinctrl\n",
				MUIC_DEV_NAME, __func__);
		goto err_io;
	}
#endif

	of_update_supported_list(i2c, pdata);
	vps_show_table();
#endif /* CONFIG_OF */
	if (!pdata) {
		pr_err("%s: failed to get i2c platform data\n", __func__);
		ret = -ENODEV;
		goto err_io;
	}

	mutex_init(&pmuic->muic_mutex);

	pmuic->pdata = pdata;
	pmuic->i2c = i2c;
	pmuic->is_factory_start = false;
	pmuic->is_otg_test = false;
	pmuic->attached_dev = ATTACHED_DEV_UNKNOWN_MUIC;
	pmuic->is_gamepad = false;
	pmuic->is_dcdtmr_intr = false;
#if defined(CONFIG_SEC_DEBUG)
	pmuic->usb_to_ta_state = false;
#endif

#if defined(CONFIG_MUIC_SUPPORT_EARJACK)
	pmuic->is_earkeypressed = false;
	pmuic->old_keycode = 0;

	/* input */
	pmuic->input = input;
	input->phys = "deskdock-key/input0";
	input->id.bustype = BUS_HOST;
	input->id.vendor = 0x0001;
	input->id.product = 0x0001;
	input->id.version = 0x0001;

	/* Enable auto repeat feature of Linux input subsystem */
	__set_bit(EV_REP, input->evbit);

	pr_info("%s, input->name:%s\n", __func__, input->name);

	input_set_capability(input, EV_KEY, KEY_MEDIA);
	input_set_capability(input, EV_KEY, KEY_VOLUMEUP);
	input_set_capability(input, EV_KEY, KEY_VOLUMEDOWN);

	ret = input_register_device(input);
	if (ret) {
		pr_err("%s: Unable to register input device, "
				"error: %d\n", __func__, ret);
	}
#endif
	if (!strcmp(pmuic->chip_name, "max,max77849"))
		pmuic->vps_table = VPS_TYPE_TABLE;
	else
		pmuic->vps_table = VPS_TYPE_SCATTERED;

	pr_info("%s: VPS_TYPE=%d\n", __func__, pmuic->vps_table);

	if (get_afc_mode() & CH_MODE_AFC_DISABLE_VAL) {
		pr_info("AFC mode disabled\n");
		pmuic->pdata->afc_disable = true;
	} else {
		pr_info("AFC mode enabled\n");
		pmuic->pdata->afc_disable = false;
	}

	if (!pdata->set_gpio_uart_sel) {
		if (pmuic->gpio_uart_sel) {
			pr_info("%s: gpio_uart_sel registered.\n", __func__);
			pdata->set_gpio_uart_sel = muic_set_gpio_uart_sel;
		} else
			pr_info("%s: gpio_uart_sel is not supported.\n", __func__);
	}

	if (pmuic->pdata->init_gpio_cb) {
		ret = pmuic->pdata->init_gpio_cb(get_switch_sel());
		if (ret) {
			pr_err("%s: failed to init gpio(%d)\n", __func__, ret);
		goto fail_init_gpio;
		}
	}

	if (pmuic->pdata->init_switch_dev_cb)
		pmuic->pdata->init_switch_dev_cb();


#if !defined(CONFIG_SEC_FACTORY)
	if (!(get_switch_sel() & SWITCH_SEL_RUSTPROOF_MASK)) {
		pr_info("  Enable rustproof mode\n");
		pmuic->is_rustproof = true;
	} else {
		pr_info("  Disable rustproof mode\n");
		pmuic->is_rustproof = false;
	}
#endif

	/* Register chipset register map. */
	muic_register_regmap(&pdesc, pmuic);
	pdesc->muic = pmuic;
	pops = pdesc->regmapops;
	pmuic->regmapdesc = pdesc;
#if defined(CONFIG_MUIC_SLAVE_MODE_CONTROL_VBUS)	
	g_pmuic = pmuic;
#endif	
#if defined(CONFIG_MUIC_SUPPORT_EARJACK)
        audio = class_create(THIS_MODULE, "audio");
        if (IS_ERR(audio)) {
                pr_err("Failed to create class(audio)!\n");
        }

        earjack = device_create(audio, NULL, 0, NULL, "earjack");
        if (IS_ERR(earjack)) {
                pr_err("Failed to create device(earjack)!\n");
        }
        dev_set_drvdata(earjack, pmuic);
#endif

#if defined(CONFIG_MUIC_SUPPORT_CCIC)
	pmuic->opmode = get_ccic_info() & 0x0F;
	pmuic->afc_water_disable = false;
#endif
	/* set switch device's driver_data */
	dev_set_drvdata(switch_device, pmuic);

	/* create sysfs group */
	ret = sysfs_create_group(&switch_device->kobj, &muic_sysfs_group);
	if (ret) {
		pr_err("%s: failed to create sm5703 muic attribute group\n",
			__func__);
		goto fail;
	}

	ret = pops->revision(pdesc);
	if (ret) {
		pr_err("%s: failed to init muic rev register(%d)\n", __func__,
			ret);
		goto fail;
	}

	ret = muic_reg_init(pmuic);
	if (ret) {
		pr_err("%s: failed to init muic register(%d)\n", __func__, ret);
		goto fail;
	}

	pops->update(pdesc);
	pops->show(pdesc);

	coagent_update_ctx(pmuic);

	ret = muic_irq_init(pmuic);
	if (ret) {
		pr_err("%s: failed to init muic irq(%d)\n", __func__, ret);
		goto fail_init_irq;
	}

#ifdef CONFIG_MUIC_UNIVERSAL_MULTI_SUPPORT
	if (pmuic->is_afc_support == true)
		muic_init_afc_state(pmuic);
#elif defined(CONFIG_MUIC_UNIVERSAL_SM5705_AFC)
	    muic_init_afc_state(pmuic);
#endif

	pmuic->muic_reset_count = 0;
	
	/* initial cable detection */
	INIT_DELAYED_WORK(&pmuic->init_work, muic_init_detect);
	schedule_delayed_work(&pmuic->init_work, msecs_to_jiffies(300));
#ifdef DEBUG_MUIC
	INIT_DELAYED_WORK(&pmuic->usb_work, muic_show_debug_info);
	schedule_delayed_work(&pmuic->usb_work, msecs_to_jiffies(10000));
#endif


#if defined(CONFIG_MUIC_SUPPORT_CCIC)
	muic_is_ccic_supported_jig(pmuic, pmuic->attached_dev);

	if (pmuic->opmode & OPMODE_CCIC)
		muic_register_ccic_notifier(pmuic);
	else
		pr_info("%s: OPMODE_MUIC. CCIC is not used.\n", __func__);
#endif

#if defined(CONFIG_USB_EXTERNAL_NOTIFY)
	muic_register_usb_notifier(pmuic);
#endif
	muic_online =  true;
#ifdef CONFIG_SEC_BSP
	muic_dump = ipc_log_context_create(5, "muic_dump", 0);
#endif
	return 0;

fail_init_irq:
	if (i2c->irq)
		free_irq(i2c->irq, pmuic);
fail:
	if (pmuic->pdata->cleanup_switch_dev_cb)
		pmuic->pdata->cleanup_switch_dev_cb();
	sysfs_remove_group(&switch_device->kobj, &muic_sysfs_group);
	mutex_unlock(&pmuic->muic_mutex);
	mutex_destroy(&pmuic->muic_mutex);
fail_init_gpio:
	i2c_set_clientdata(i2c, NULL);
err_io:
	kfree(pmuic);
err_return:
	return ret;
}

static int muic_remove(struct i2c_client *i2c)
{
	muic_data_t *pmuic = i2c_get_clientdata(i2c);

	sysfs_remove_group(&switch_device->kobj, &muic_sysfs_group);

	if (pmuic) {
		pr_info("%s:%s\n", pmuic->chip_name, __func__);
		cancel_delayed_work(&pmuic->init_work);
		cancel_delayed_work(&pmuic->usb_work);
		disable_irq_wake(pmuic->i2c->irq);
		free_irq(pmuic->i2c->irq, pmuic);

		if (pmuic->pdata->cleanup_switch_dev_cb)
			pmuic->pdata->cleanup_switch_dev_cb();

#if defined(CONFIG_USB_EXTERNAL_NOTIFY)
		muic_unregister_usb_notifier(pmuic);
#endif
		mutex_destroy(&pmuic->muic_mutex);
		i2c_set_clientdata(pmuic->i2c, NULL);
		kfree(pmuic);
	}
	muic_online = false;
	return 0;
}

static const struct i2c_device_id muic_i2c_id[] = {
	{ MUIC_DEV_NAME, 0 },
	{}
};
MODULE_DEVICE_TABLE(i2c, muic_i2c_id);

#if defined(CONFIG_OF)
MODULE_DEVICE_TABLE(of, muic_i2c_dt_ids);
#endif /* CONFIG_OF */

static void muic_shutdown(struct i2c_client *i2c)
{
	muic_data_t *pmuic = i2c_get_clientdata(i2c);
	int ret;

	pr_info("%s:%s\n", pmuic->chip_name, __func__);
	if (!pmuic->i2c) {
		pr_err("%s:%s no muic i2c client\n", pmuic->chip_name, __func__);
		return;
	}

	pr_info("%s:%s open D+,D-\n", pmuic->chip_name, __func__);
	ret = com_to_open_with_vbus(pmuic);
	if (ret < 0)
		pr_err("%s:%s fail to open mansw1 reg\n", pmuic->chip_name,
				__func__);

	/* set auto sw mode before shutdown to make sure device goes into */
	/* LPM charging when TA or USB is connected during off state */
	pr_info("%s:%s muic auto detection enable\n", pmuic->chip_name, __func__);
	set_switch_mode(pmuic, SWMODE_AUTO);

	if (pmuic->pdata && pmuic->pdata->cleanup_switch_dev_cb)
		pmuic->pdata->cleanup_switch_dev_cb();

	muic_online =  false;
	pr_info("%s:%s -\n", MUIC_DEV_NAME, __func__);
}
#if defined(CONFIG_PM)

static int muic_suspend(struct device *dev)
{
	muic_data_t *pmuic = dev_get_drvdata(dev);
	struct i2c_client *i2c = pmuic->i2c;

	disable_irq_nosync(i2c->irq);

	return 0;
}

static int muic_resume(struct device *dev)
{
	muic_data_t *pmuic = dev_get_drvdata(dev);
	struct i2c_client *i2c = pmuic->i2c;

	enable_irq(i2c->irq);

	return 0;
}

const struct dev_pm_ops muic_pm = {
	.suspend = muic_suspend,
	.resume = muic_resume,
};
#endif /* CONFIG_PM */

static struct i2c_driver muic_driver = {
	.driver		= {
		.name	= MUIC_DEV_NAME,
#if defined(CONFIG_OF)
		.of_match_table	= muic_i2c_dt_ids,
#endif /* CONFIG_OF */
#if defined(CONFIG_PM)
		.pm = &muic_pm,
#endif /* CONFIG_PM */
	},
	.probe		= muic_probe,
	.remove		= muic_remove,
	.shutdown	= muic_shutdown,
	.id_table	= muic_i2c_id,
};

static int __init muic_init(void)
{
	pr_info("%s:%s\n", MUIC_DEV_NAME, __func__);
	return i2c_add_driver(&muic_driver);
}
module_init(muic_init);

static void muic_exit(void)
{
	pr_info("%s:%s\n", MUIC_DEV_NAME, __func__);
	i2c_del_driver(&muic_driver);
}
module_exit(muic_exit);

MODULE_DESCRIPTION("MUIC driver");
MODULE_AUTHOR("<smilesr.ryu@samsung.com>");
MODULE_LICENSE("GPL");
