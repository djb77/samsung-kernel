/*
 * muic_afc.c
 *
 * Copyright (C) 2014 Samsung Electronics
 * Jeongrae Kim <jryu.kim@samsung.com>
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
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/module.h>

#include <linux/muic/muic.h>
#include <linux/muic/muic_afc.h>
#include <linux/sec_param.h>
#include "muic-internal.h"
#include "muic_regmap.h"
#include "muic_i2c.h"
#if defined(CONFIG_MUIC_NOTIFIER)
#include <linux/muic/muic_notifier.h>
#endif /* CONFIG_MUIC_NOTIFIER */

/* Bit 0 : VBUS_VAILD, Bit 1~7 : Reserved */
#define	REG_INT3	0x05
#define	REG_INTMASK3	0x08
#define	REG_DEVT1	0x0a
#define	REG_RSVDID1	0x15
#define	REG_RSVDID2	0x16
#define	REG_AFCTXD	0x19
#define	REG_AFCSTAT	0x1a
#define	REG_VBUSSTAT	0x1b

muic_data_t *gpmuic;
static int afc_work_state;

static int muic_is_afc_voltage(void);
static int muic_dpreset_afc(void);
static int muic_restart_afc(void);

/* To make AFC work properly on boot */
static int is_charger_ready;
static struct work_struct muic_afc_init_work;

void muic_afc_delay_check_state(int state);

int muic_disable_afc(int disable)
{
	struct afc_ops *afcops;

	int ret, retry;

#ifdef CONFIG_MUIC_UNIVERSAL_MULTI_SUPPORT
	if (gpmuic == NULL)
		return 1;
#endif
	afcops = gpmuic->regmapdesc->afcops;

	pr_info("%s disable = %d\n", __func__, disable);	

	if (disable) {
		/* AFC disable state */
		if (muic_is_afc_voltage() && gpmuic->is_afc_device) {
			ret = muic_dpreset_afc();
			if (ret < 0) {
				pr_err("%s:failed to AFC reset(%d)\n",
						__func__, ret);
			}
			msleep(60); // 60ms delay

			afcops->afc_ctrl_reg(gpmuic->regmapdesc, AFCCTRL_VBUS_READ, 1);
			afcops->afc_ctrl_reg(gpmuic->regmapdesc, AFCCTRL_VBUS_READ, 0);
			for (retry = 0; retry <20; retry++) {
				mdelay(20);
				ret = muic_is_afc_voltage();
				if (!ret) {
					pr_info("%s:AFC Reset Success(%d)\n",
							__func__, ret);
					return 1;
				} else {
					pr_info("%s:AFC Reset Failed(%d)\n",
							__func__, ret);
				}
			}
		} else {
			pr_info("%s:Not connected AFC\n",__func__);
			return 1;
		}
	} else {
		/*  AFC enable state */
		if ((gpmuic->attached_dev == ATTACHED_DEV_AFC_CHARGER_5V_MUIC) ||
				((gpmuic->is_afc_device) && (gpmuic->attached_dev != ATTACHED_DEV_AFC_CHARGER_9V_MUIC)))
			muic_restart_afc();
		return 1;
	}
	return 0;
}

int muic_check_afc_state(int state)
{
	struct afc_ops *afcops;

	int ret, retry;

#ifdef CONFIG_MUIC_UNIVERSAL_MULTI_SUPPORT
	if (gpmuic == NULL)
		return 1;
#endif
	afcops = gpmuic->regmapdesc->afcops;

	mutex_lock(&gpmuic->lock);
	pr_info("%s start. state = %d\n", __func__, state);

	if (gpmuic->is_flash_on == state) {
		pr_info("%s AFC state is already %d, skip\n", __func__, state);
		goto Out;
	}

	if (state) {
		/* Flash on state */
		if (muic_is_afc_voltage() && gpmuic->is_afc_device) {
			ret = muic_dpreset_afc();
			if (ret < 0) {
				pr_err("%s:failed to AFC reset(%d)\n",
						__func__, ret);
			}
			msleep(60); // 60ms delay
			afcops->afc_ctrl_reg(gpmuic->regmapdesc, AFCCTRL_VBUS_READ, 1);
			afcops->afc_ctrl_reg(gpmuic->regmapdesc, AFCCTRL_VBUS_READ, 0);
			for (retry = 0; retry <20; retry++) {
				mdelay(20);
				ret = muic_is_afc_voltage();
				if (!ret) {
					pr_info("%s:AFC Reset Success(%d)\n",
							__func__, ret);
					gpmuic->is_flash_on = 1;
					goto Out;
				} else {
					pr_info("%s:AFC Reset Failed(%d)\n",
							__func__, ret);
					gpmuic->is_flash_on = -1;
				}
			}
		} else {
			pr_info("%s:Not connected AFC\n",__func__);
			gpmuic->is_flash_on = 1;
			goto Out;
		}
	} else {
		if (!gpmuic->pdata->afc_disable) {
			/* Flash off state */
			if ((gpmuic->attached_dev == ATTACHED_DEV_AFC_CHARGER_5V_MUIC) ||
					((gpmuic->is_afc_device) &&
					(gpmuic->attached_dev != ATTACHED_DEV_AFC_CHARGER_9V_MUIC) &&
					(gpmuic->attached_dev != ATTACHED_DEV_AFC_CHARGER_9V_18W_MUIC) &&
					(gpmuic->attached_dev != ATTACHED_DEV_QC_CHARGER_9V_MUIC)))
				muic_restart_afc();
		} else {
			pr_info("%s:AFC is disabled in setting. Skip to enable.\n",__func__);		
		}
		
		gpmuic->is_flash_on = 0;
		goto Out;
	}

	pr_info("%s end.\n", __func__);
	mutex_unlock(&gpmuic->lock);
	return 0;

Out:
	pr_info("%s end.\n", __func__);
	mutex_unlock(&gpmuic->lock);
	return 1;
}
EXPORT_SYMBOL(muic_check_afc_state);

int muic_torch_prepare(int state)
{
	struct afc_ops *afcops = gpmuic->regmapdesc->afcops;
	int ret, retry;

	mutex_lock(&gpmuic->lock);
	pr_info("%s start. state = %d\n", __func__, state);

	if (gpmuic->is_flash_on == state) {
		pr_info("%s AFC state is already %d, skip\n", __func__, state);
		goto Out;
	}

	if (afc_work_state == 1) {
		pr_info("%s:%s cancel_delayed_work  afc_work_state=%d\n",MUIC_DEV_NAME, __func__, afc_work_state);
		cancel_delayed_work(&gpmuic->afc_restart_work);
		afc_work_state = 0;
	}

	if (state) {
		/* Torch on state */
		if (muic_is_afc_voltage() && gpmuic->is_afc_device) {
			ret = muic_dpreset_afc();
			msleep(60); // 60ms delay
			if (ret < 0) {
				pr_err("%s:failed to AFC reset(%d)\n",
						__func__, ret);
			}
			afcops->afc_ctrl_reg(gpmuic->regmapdesc, AFCCTRL_VBUS_READ, 1);
			afcops->afc_ctrl_reg(gpmuic->regmapdesc, AFCCTRL_VBUS_READ, 0);
			for (retry = 0; retry <20; retry++) {
				mdelay(20);
				ret = muic_is_afc_voltage();
				if (!ret) {
					pr_info("%s:AFC Reset Success(%d)\n",
							__func__, ret);
					gpmuic->is_flash_on = 1;
					goto Out;
				} else {
					pr_info("%s:AFC Reset Failed(%d)\n",
							__func__, ret);
					gpmuic->is_flash_on = -1;
				}
			}
		} else {
			pr_info("%s:Not connected AFC\n",__func__);
			gpmuic->is_flash_on = 1;
			goto Out;
		}
	} else {
		/* Torch off state */
		gpmuic->is_flash_on = 0;
		if ((gpmuic->attached_dev == ATTACHED_DEV_AFC_CHARGER_5V_MUIC) ||
				((gpmuic->is_afc_device) &&
				(gpmuic->attached_dev != ATTACHED_DEV_AFC_CHARGER_9V_MUIC) &&
				(gpmuic->attached_dev != ATTACHED_DEV_AFC_CHARGER_9V_18W_MUIC) &&
				(gpmuic->attached_dev != ATTACHED_DEV_QC_CHARGER_9V_MUIC))) {
			schedule_delayed_work(&gpmuic->afc_restart_work, msecs_to_jiffies(5000)); // 20sec
			pr_info("%s:%s AFC_torch_work start \n",MUIC_DEV_NAME, __func__ );
			afc_work_state = 1;
		}
		goto Out;
	}

	pr_info("%s end.\n", __func__);
	mutex_unlock(&gpmuic->lock);
	return 0;

Out:
	pr_info("%s end.\n", __func__);
	mutex_unlock(&gpmuic->lock);
	return 1;
}
EXPORT_SYMBOL(muic_torch_prepare);

static int muic_is_afc_voltage(void)
{
	struct i2c_client *i2c = gpmuic->i2c;
	int vbus_status;
	
	if (gpmuic->attached_dev == ATTACHED_DEV_NONE_MUIC) {
		pr_info("%s attached_dev None \n", __func__);
		return 0;
	}

	vbus_status = muic_i2c_read_byte(i2c, REG_VBUSSTAT);
	vbus_status = (vbus_status & 0x0F);
	pr_info("%s vbus_status (%d)\n", __func__, vbus_status);
	if (vbus_status == 0x00)
		return 0;
	else
		return 1;
}

static int muic_dpreset_afc(void)
{
	struct afc_ops *afcops = gpmuic->regmapdesc->afcops;

	pr_info("%s: gpmuic->attached_dev = %d\n", __func__, gpmuic->attached_dev);
	if ((gpmuic->attached_dev == ATTACHED_DEV_AFC_CHARGER_9V_MUIC) ||
		(gpmuic->attached_dev == ATTACHED_DEV_AFC_CHARGER_PREPARE_MUIC) ||
		(gpmuic->attached_dev == ATTACHED_DEV_AFC_CHARGER_9V_18W_MUIC) ||
		(gpmuic->attached_dev == ATTACHED_DEV_QC_CHARGER_9V_MUIC) ||
		(gpmuic->attached_dev == ATTACHED_DEV_QC_CHARGER_PREPARE_MUIC) ||
		(muic_is_afc_voltage()) ) {
		// ENAFC set '0'
		afcops->afc_ctrl_reg(gpmuic->regmapdesc, AFCCTRL_ENAFC, 0);
		msleep(50); // 50ms delay

		muic_afc_delay_check_state(0);

		afcops->afc_ctrl_reg(gpmuic->regmapdesc, AFCCTRL_ENQC20, 0);		

		// DP_RESET
		pr_info("%s:AFC Disable \n", __func__);
		afcops->afc_ctrl_reg(gpmuic->regmapdesc, AFCCTRL_DIS_AFC, 1);
		msleep(60);
		afcops->afc_ctrl_reg(gpmuic->regmapdesc, AFCCTRL_DIS_AFC, 0);

		if (gpmuic->attached_dev != ATTACHED_DEV_NONE_MUIC) {
			gpmuic->attached_dev = ATTACHED_DEV_AFC_CHARGER_5V_MUIC;
			muic_notifier_attach_attached_dev(ATTACHED_DEV_AFC_CHARGER_5V_MUIC);
		} else {
			pr_info("%s:Seems TA is removed.\n", __func__);
		}
	}

	return 0;
}

static int muic_restart_afc(void)
{
	struct i2c_client *i2c = gpmuic->i2c;
	int ret, value;
	struct afc_ops *afcops = gpmuic->regmapdesc->afcops;

	pr_info("%s:AFC Restart attached_dev = 0x%x\n", __func__, gpmuic->attached_dev);
	msleep(120); // 120ms delay
	if ((gpmuic->attached_dev == ATTACHED_DEV_NONE_MUIC) ||
		(gpmuic->attached_dev == ATTACHED_DEV_TA_MUIC)) {
		pr_info("%s:%s Device type is None\n",MUIC_DEV_NAME, __func__);
		return 0;
	}
	gpmuic->attached_dev = ATTACHED_DEV_AFC_CHARGER_PREPARE_MUIC;
	muic_notifier_attach_attached_dev(ATTACHED_DEV_AFC_CHARGER_PREPARE_MUIC);
	cancel_delayed_work(&gpmuic->afc_retry_work);
	schedule_delayed_work(&gpmuic->afc_retry_work, msecs_to_jiffies(5000)); // 5sec

#ifdef CONFIG_MUIC_SM5705_AFC_18W_TA_SUPPORT
	pr_info("%s:%s pmuic->is_18w_ta = %d \n",MUIC_DEV_NAME, __func__,gpmuic->is_18w_ta);

	if (gpmuic->is_18w_ta == 0) {
		// voltage(9.0V) + current(1.95A) setting : 0x48
		value = 0x48;
	} else {
		value = muic_i2c_read_byte(i2c, REG_AFCTXD);
		pr_info("%s:%s AFC_TXD read [0x%02x]\n",MUIC_DEV_NAME, __func__, value);
	}

	//reset multibyte retry count
	afcops->afc_reset_multibyte_retry_count(gpmuic->regmapdesc);
#else
	// voltage(9.0V) + current(1.65A) setting : 0x
	value = 0x46;
#endif
	ret = muic_i2c_write_byte(i2c, REG_AFCTXD, value);
	if (ret < 0)
		printk(KERN_ERR "[muic] %s: err write AFC_TXD(%d)\n", __func__, ret);
	pr_info("%s:AFC_TXD [0x%02x]\n", __func__, value);

	// ENAFC set '1'
	afcops->afc_ctrl_reg(gpmuic->regmapdesc, AFCCTRL_ENAFC, 1);

	return 0;
}

static void muic_afc_restart_work(struct work_struct *work)
{
	struct i2c_client *i2c = gpmuic->i2c;
	int ret, value;
	struct afc_ops *afcops = gpmuic->regmapdesc->afcops;

	pr_info("%s:AFC Restart\n", __func__);
	msleep(120); // 120ms delay
	if (gpmuic->attached_dev == ATTACHED_DEV_NONE_MUIC) {
		pr_info("%s:%s Device type is None\n",MUIC_DEV_NAME, __func__);
		return;
	}
	gpmuic->attached_dev = ATTACHED_DEV_AFC_CHARGER_PREPARE_MUIC;
	muic_notifier_attach_attached_dev(ATTACHED_DEV_AFC_CHARGER_PREPARE_MUIC);
	cancel_delayed_work(&gpmuic->afc_retry_work);
	schedule_delayed_work(&gpmuic->afc_retry_work, msecs_to_jiffies(5000)); // 5sec

#ifdef CONFIG_MUIC_SM5705_AFC_18W_TA_SUPPORT
	pr_info("%s:%s pmuic->is_18w_ta = %d \n",MUIC_DEV_NAME, __func__,gpmuic->is_18w_ta);

	if (gpmuic->is_18w_ta == 0) {
		// voltage(9.0V) + current(1.95A) setting : 0x48
		value = 0x48;
	} else {
		value = muic_i2c_read_byte(i2c, REG_AFCTXD);
		pr_info("%s:%s AFC_TXD read [0x%02x]\n",MUIC_DEV_NAME, __func__, value);
	}
#else
	// voltage(9.0V) + current(1.65A) setting : 0x
	value = 0x46;
#endif
	ret = muic_i2c_write_byte(i2c, REG_AFCTXD, value);
	if (ret < 0)
		printk(KERN_ERR "[muic] %s: err write AFC_TXD(%d)\n", __func__, ret);
	pr_info("%s:AFC_TXD [0x%02x]\n", __func__, value);

	// ENAFC set '1'
	afcops->afc_ctrl_reg(gpmuic->regmapdesc, AFCCTRL_ENAFC, 1);
	afc_work_state = 0;
}

static void muic_afc_retry_work(struct work_struct *work)
{
	struct afc_ops *afcops = gpmuic->regmapdesc->afcops;
	struct i2c_client *i2c = gpmuic->i2c;
	int ret, vbus;

	//Reason of AFC fail
	ret = muic_i2c_read_byte(i2c, 0x3C);
	pr_info("%s : Read 0x3C = [0x%02x]\n", __func__, ret);

	// read AFC_STATUS
	ret = muic_i2c_read_byte(i2c, REG_AFCSTAT);
	pr_info("%s: AFC_STATUS [0x%02x]\n", __func__, ret);
	
	pr_info("%s:AFC retry work\n", __func__);
	if (gpmuic->attached_dev == ATTACHED_DEV_AFC_CHARGER_PREPARE_MUIC) {
		vbus = muic_i2c_read_byte(i2c, REG_RSVDID1);
		if (!(vbus & 0x01)) {
			pr_info("%s:%s VBUS is nothing\n",MUIC_DEV_NAME, __func__);
			gpmuic->attached_dev = ATTACHED_DEV_NONE_MUIC;
			muic_notifier_attach_attached_dev(ATTACHED_DEV_NONE_MUIC);
			return;
		}

		afcops->afc_ctrl_reg(gpmuic->regmapdesc, AFCCTRL_ENAFC, 0);

		muic_afc_delay_check_state(0);

		afcops->afc_ctrl_reg(gpmuic->regmapdesc, AFCCTRL_ENQC20, 0);		

		pr_info("%s: [MUIC] devtype is afc prepare - Disable AFC\n", __func__);
		afcops->afc_ctrl_reg(gpmuic->regmapdesc, AFCCTRL_DIS_AFC, 1);
		msleep(20);
		afcops->afc_ctrl_reg(gpmuic->regmapdesc, AFCCTRL_DIS_AFC, 0);
	}
}

void muic_afc_delay_check_state(int state)
{
	struct i2c_client *i2c = gpmuic->i2c;

	pr_info("%s : state = %d \n", __func__, state);

	if (state == 1) {
		muic_i2c_write_byte(i2c, REG_RSVDID2, 0x2C); // VDP_SRC_EN '1'
		gpmuic->delay_check_count = 0;
		cancel_delayed_work(&gpmuic->afc_delay_check_work);
		schedule_delayed_work(&gpmuic->afc_delay_check_work,
					msecs_to_jiffies(1700)); // 1.7 sec
		pr_info("%s: afc_delay_check_work start\n", __func__);
	} else {
		muic_i2c_write_byte(i2c, REG_RSVDID2, 0x24); // VDP_SRC_EN '0'
		cancel_delayed_work(&gpmuic->afc_delay_check_work);
		pr_info("%s: afc_delay_check_work cancel (%d)\n",
					__func__ ,gpmuic->delay_check_count);
		gpmuic->delay_check_count = 0;
	}
}

static void muic_afc_delay_check_work(struct work_struct *work)
{
	struct afc_ops *afcops = gpmuic->regmapdesc->afcops;
	struct i2c_client *i2c = gpmuic->i2c;
	int val1;

	pr_info("%s: attached_dev = %d\n", __func__, gpmuic->attached_dev);
	pr_info("%s: delay_check_count = %d , is_flash_on=%d\n",
			__func__, gpmuic->delay_check_count, gpmuic->is_flash_on);
	if (gpmuic->delay_check_count > 5) {
		muic_afc_delay_check_state(0);
		return;
	}

	if (gpmuic->is_flash_on == 1) {
		muic_afc_delay_check_state(0);
		pr_info("%s: skip\n", __func__);
		return;
	}

	val1 = muic_i2c_read_byte(i2c, REG_DEVT1);
	pr_info("%s:val1 = 0x%x \n", __func__, val1);
	if ((gpmuic->attached_dev == ATTACHED_DEV_TA_MUIC) && (val1 == 0x40)) {
		pr_info("%s: DP_RESET\n", __func__);
		afcops->afc_ctrl_reg(gpmuic->regmapdesc, AFCCTRL_DIS_AFC, 1);
		msleep(60);
		afcops->afc_ctrl_reg(gpmuic->regmapdesc, AFCCTRL_DIS_AFC, 0);
		cancel_delayed_work(&gpmuic->afc_delay_check_work);
		schedule_delayed_work(&gpmuic->afc_delay_check_work,
					msecs_to_jiffies(1700)); // 1.7 sec
		gpmuic->delay_check_count++;
		pr_info("%s: afc_delay_check_work retry : %d \n",
					__func__, gpmuic->delay_check_count);
	}
}

static void muic_focrced_detection_by_charger(struct work_struct *work)
{
	struct afc_ops *afcops = gpmuic->regmapdesc->afcops;

	pr_info("%s\n", __func__);

	mutex_lock(&gpmuic->muic_mutex);

	afcops->afc_init_check(gpmuic->regmapdesc);

	mutex_unlock(&gpmuic->muic_mutex);
}

void muic_charger_init(void)
{
	pr_info("%s\n", __func__);

	if (!gpmuic) {
		pr_info("%s: MUIC AFC is not ready.\n", __func__);
		return;
	}

	if (is_charger_ready) {
		pr_info("%s: charger is already ready.\n", __func__);
		return;
	}

	is_charger_ready = true;

	if (gpmuic->attached_dev == ATTACHED_DEV_TA_MUIC)
		schedule_work(&muic_afc_init_work);
}

#ifdef CONFIG_MUIC_SM5705_AFC_18W_TA_SUPPORT

// voltage
// 5 : 5V
// 6 : 6V
// 7 : 7V
// 8 : 8V
// 9 : 9V
int muic_18W_TA_voltage_control(int voltage)
{
	struct afc_ops *afcops = gpmuic->regmapdesc->afcops;
	struct i2c_client *i2c = gpmuic->i2c;
	int ret;
	int vol;
	int int3,state;

	pr_info("%s voltage = %d V \n", __func__, voltage);
	pr_info("%s:%s pmuic->is_18w_ta = %d \n",MUIC_DEV_NAME, __func__,gpmuic->is_18w_ta);
	if (gpmuic->is_18w_ta == 0){
		return 0;
	}
	if ( (voltage < 5) || (voltage > 9) ){
		pr_info("%s voltage error(%dV) \n", __func__, voltage);    
		return 0;
	}
	switch(voltage){
		case 5 : 
			vol = 0x08;
			break;
		case 6 : 
			vol = 0x18;
			break;
		case 7 : 
			vol = 0x28;
			break;
		case 8 : 
			vol = 0x38;
			break;
		case 9 : 
			vol = 0x48;
			break;
	}

	ret = muic_i2c_write_byte(i2c, REG_AFCTXD, vol);
	if (ret < 0){
		printk(KERN_ERR "[muic] %s: err write AFC_TXD(%d)\n", __func__, ret);
	}
	pr_info("%s:%s AFC_TXD [0x%02x]\n",MUIC_DEV_NAME, __func__, vol);

	muic_i2c_write_byte(i2c, REG_INTMASK3, 0x20); // INTMASK3 : AFC_ERROR interrupt disable

	// ENAFC set '1'
	afcops->afc_ctrl_reg(gpmuic->regmapdesc, AFCCTRL_ENAFC, 1);
	msleep(300); // 300ms delay
	afcops->afc_ctrl_reg(gpmuic->regmapdesc, AFCCTRL_ENAFC, 0);

	msleep(50); // 50ms delay
	int3 = muic_i2c_read_byte(i2c, REG_INT3); // int3 read
	pr_info("%s:%s int3 = 0x%x \n",MUIC_DEV_NAME, __func__,int3);
	if (int3 & 0x20) { // error occur
		// read AFC_STATUS
		state = muic_i2c_read_byte(i2c, REG_AFCSTAT);
		if (state < 0)
			printk(KERN_ERR "%s: err read AFC_STATUS %d\n", __func__, state);
		pr_info("%s:%s AFC_STATUS [0x%02x]\n",MUIC_DEV_NAME, __func__, state);    

		voltage = 0;
	}

	muic_i2c_write_byte(i2c, REG_INTMASK3, 0x00); // INTMASK3 : AFC_ERROR interrupt enable

	return voltage;
}
EXPORT_SYMBOL(muic_18W_TA_voltage_control);

int muic_18W_TA_is_voltage(void)
{
	struct i2c_client *i2c = gpmuic->i2c;
	int value, vol;

	pr_info("%s:%s pmuic->is_18w_ta = %d \n",MUIC_DEV_NAME, __func__,gpmuic->is_18w_ta);
	if (gpmuic->is_18w_ta == 0){
		return 0;
	}

	value = muic_i2c_read_byte(i2c, REG_AFCTXD);
	pr_info("%s: AFC_TXD [0x%02x]\n",MUIC_DEV_NAME, value);
	value = (value&0xF0)>>4;
	switch(value){
		case 0 : 
			vol = 5;
			break;
		case 1 : 
			vol = 6;
			break;
		case 2 : 
			vol = 7;
			break;
		case 3 : 
			vol = 8;
			break;
		case 4 : 
			vol = 9;
			break;
		default:
			vol = -1;
			pr_info("%s:%s get a invalid value.\n", MUIC_DEV_NAME, __func__);
			break;
	}

	return vol;
}
EXPORT_SYMBOL(muic_18W_TA_is_voltage);

int muic_afc_is_18W_TA(void)
{	
	pr_info("%s:%s pmuic->is_18w_ta = %d \n",MUIC_DEV_NAME, __func__,gpmuic->is_18w_ta);

	return gpmuic->is_18w_ta;
}
EXPORT_SYMBOL(muic_afc_is_18W_TA);
#endif

static ssize_t afc_off_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	muic_data_t *pmuic = dev_get_drvdata(dev);
	return snprintf(buf, 4, "%d\n", pmuic->pdata->afc_disable);
}

static ssize_t afc_off_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t size)
{
	muic_data_t *pmuic = dev_get_drvdata(dev);
	struct muic_platform_data *pdata = pmuic->pdata;
	unsigned int param_val;
#ifdef CONFIG_MUIC_HV
	int ret = 0;
#endif
	if (!strncmp(buf, "1", 1)) {
		pr_info("%s, Disable AFC\n", __func__);
		param_val = 1;
		pdata->afc_disable = true;
#ifdef CONFIG_SEC_FACTORY
		muic_disable_afc(1);
#endif
	} else {
		pr_info("%s, Enable AFC\n", __func__);
		param_val = 0;
		pdata->afc_disable = false;
#ifdef CONFIG_SEC_FACTORY
		muic_disable_afc(0);
#endif
	}

	pr_info("%s: param_val:%d\n", __func__, param_val);
	#ifdef CONFIG_MUIC_HV
	ret = sec_set_param(param_index_afc_disable, &param_val);

	if (!ret) {
		pr_err("%s: set_param failed - %02x:(%d)\n",
					__func__, param_val, ret);
		return ret;
	} else {
		pr_info("%s:%s afc_disable:%d (AFC %s)\n", MUIC_DEV_NAME,
		__func__, pdata->afc_disable, pdata->afc_disable ? "Diabled": "Enabled");
	}
	#endif
	return size;
}
static DEVICE_ATTR(afc_disable, S_IRUGO | S_IWUSR | S_IWGRP,
		afc_off_show, afc_off_store);
void muic_init_afc_state(muic_data_t *pmuic)
{
	int ret;
	gpmuic = pmuic;
	gpmuic->is_flash_on = 0;

	mutex_init(&gpmuic->lock);
	
	/* To make AFC work properly on boot */
	INIT_WORK(&muic_afc_init_work, muic_focrced_detection_by_charger);
	gpmuic->is_afc_device = 0;
	INIT_DELAYED_WORK(&gpmuic->afc_restart_work, muic_afc_restart_work);
	INIT_DELAYED_WORK(&gpmuic->afc_retry_work, muic_afc_retry_work);
	INIT_DELAYED_WORK(&gpmuic->afc_delay_check_work, muic_afc_delay_check_work);

	ret = device_create_file(switch_device, &dev_attr_afc_disable);
	if (ret < 0) {
		pr_err("[MUIC] Failed to create file (disable AFC)!\n");
	}

	pr_info("%s:attached_dev = %d\n", __func__, gpmuic->attached_dev);
}

MODULE_DESCRIPTION("MUIC driver");
MODULE_AUTHOR("<jryu.kim@samsung.com>");
MODULE_LICENSE("GPL");
