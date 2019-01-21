/*
 * s2mu004-muic.c - MUIC driver for the Samsung s2mu004
 *
 *  Copyright (C) 2015 Samsung Electronics
 *
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
 * This driver is based on max77843-muic-afc.c
 *
 */

#define pr_fmt(fmt)	"[MUIC_HV] " fmt

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/wakelock.h>

#include <linux/mfd/samsung/s2mu004.h>
#include <linux/mfd/samsung/s2mu004-private.h>

/* MUIC header file */
#include <linux/muic/muic.h>
#include <linux/muic/s2mu004-muic.h>
#include <linux/muic/s2mu004-muic-hv.h>
#ifdef CONFIG_CCIC_NOTIFIER
#include <linux/muic/muic_interface.h>
#endif

#if defined(CONFIG_MUIC_NOTIFIER)
#include <linux/muic/muic_notifier.h>
#endif /* CONFIG_MUIC_NOTIFIER */
#include <linux/delay.h>

static bool debug_en_checklist = true;
static int is_charger_ready;

enum act_function_num {
	FUNC_TA_TO_PREPARE,
	FUNC_PREPARE_TO_PREPARE_DUPLI,
	FUNC_PREPARE_TO_AFC_5V,
	FUNC_PREPARE_TO_QC_PREPARE,
	FUNC_PREPARE_DUPLI_TO_PREPARE_DUPLI,
	FUNC_PREPARE_DUPLI_TO_AFC_5V,
	FUNC_PREPARE_DUPLI_TO_AFC_ERR_V,
	FUNC_PREPARE_DUPLI_TO_AFC_9V,
	FUNC_AFC_5V_TO_AFC_5V_DUPLI,
	FUNC_AFC_5V_TO_AFC_ERR_V,
	FUNC_AFC_5V_TO_AFC_9V,
	FUNC_AFC_5V_DUPLI_TO_AFC_5V_DUPLI,
	FUNC_AFC_5V_DUPLI_TO_AFC_ERR_V,
	FUNC_AFC_5V_DUPLI_TO_AFC_9V,
	FUNC_AFC_ERR_V_TO_AFC_ERR_V_DUPLI,
	FUNC_AFC_ERR_V_TO_AFC_9V,
	FUNC_AFC_ERR_V_DUPLI_TO_AFC_ERR_V_DUPLI,
	FUNC_AFC_ERR_V_DUPLI_TO_AFC_9V,
	FUNC_AFC_9V_TO_AFC_5V,
	FUNC_AFC_9V_TO_AFC_9V,
	FUNC_QC_PREPARE_TO_QC_9V,
	FUNC_QC_9V_TO_QC_5V,
};

muic_afc_data_t prepare_dupli_to_afc_9v;
/* afc_condition_checklist[ATTACHED_DEV_TA_MUIC] */
muic_afc_data_t ta_to_prepare = {
	.new_dev		= ATTACHED_DEV_AFC_CHARGER_PREPARE_MUIC,
	.afc_name		= "AFC charger Prepare",
	.afc_irq		= MUIC_AFC_IRQ_VDNMON,
	.status_vbadc		= VBADC_DONTCARE,
	.status_vdnmon          = VDNMON_LOW,
	.function_num		= FUNC_TA_TO_PREPARE,
	.next			= &ta_to_prepare,
};

/* afc_condition_checklist[ATTACHED_DEV_AFC_CHARGER_PREPARE_MUIC] */
muic_afc_data_t prepare_to_qc_prepare = {
	.new_dev		= ATTACHED_DEV_QC_CHARGER_PREPARE_MUIC,
	.afc_name		= "QC charger Prepare",
	.afc_irq		= MUIC_AFC_IRQ_MPNACK,
	.status_vbadc		= VBADC_DONTCARE,
	.status_vdnmon          = VDNMON_DONTCARE,
	.function_num		= FUNC_PREPARE_TO_QC_PREPARE,
	.next			= &prepare_dupli_to_afc_9v,
};

muic_afc_data_t prepare_to_afc_5v = {
	.new_dev		= ATTACHED_DEV_AFC_CHARGER_5V_MUIC,
	.afc_name		= "AFC charger 5V",
	.afc_irq		= MUIC_AFC_IRQ_VBADC,
	.status_vbadc		= VBADC_AFC_5V,
	.status_vdnmon          = VDNMON_DONTCARE,
	.function_num		= FUNC_PREPARE_TO_AFC_5V,
	.next			= &prepare_to_afc_5v,
};

muic_afc_data_t prepare_to_prepare_dupli = {
	.new_dev		= ATTACHED_DEV_AFC_CHARGER_PREPARE_DUPLI_MUIC,
	.afc_name		= "AFC charger prepare (mrxrdy)",
	.afc_irq		= MUIC_AFC_IRQ_MRXRDY,
	.status_vbadc		= VBADC_DONTCARE,
	.status_vdnmon          = VDNMON_DONTCARE,
	.function_num		= FUNC_PREPARE_TO_PREPARE_DUPLI,
	.next			= &prepare_to_qc_prepare,
};

muic_afc_data_t prepare_dupli_to_prepare_dupli = {
	.new_dev		= ATTACHED_DEV_AFC_CHARGER_PREPARE_DUPLI_MUIC,
	.afc_name		= "AFC charger prepare (mrxrdy)",
	.afc_irq		= MUIC_AFC_IRQ_MRXRDY,
	.status_vbadc		= VBADC_DONTCARE,
	.status_vdnmon          = VDNMON_DONTCARE,
	.function_num		= FUNC_PREPARE_DUPLI_TO_PREPARE_DUPLI,
	.next			= &prepare_to_qc_prepare,
};

muic_afc_data_t prepare_dupli_to_afc_early_9v = {
	.new_dev		= ATTACHED_DEV_AFC_CHARGER_9V_MUIC,
	.afc_name		= "AFC charger 9V (early in mrxrdy)",
	.afc_irq		= MUIC_AFC_IRQ_MRXRDY,
	.status_vbadc		= VBADC_AFC_9V,
	.status_vdnmon          = VDNMON_DONTCARE,
	.function_num		= FUNC_PREPARE_DUPLI_TO_AFC_9V,
	.next			= &prepare_dupli_to_prepare_dupli,
};

muic_afc_data_t prepare_dupli_to_afc_9v = {
	.new_dev		= ATTACHED_DEV_AFC_CHARGER_9V_MUIC,
	.afc_name		= "AFC charger 9V",
	.afc_irq		= MUIC_AFC_IRQ_VBADC,
	.status_vbadc		= VBADC_AFC_9V,
	.status_vdnmon          = VDNMON_DONTCARE,
	.function_num		= FUNC_PREPARE_DUPLI_TO_AFC_9V,
	.next			= &prepare_dupli_to_afc_early_9v,
};

muic_afc_data_t prepare_dupli_to_afc_err_v = {
	.new_dev		= ATTACHED_DEV_AFC_CHARGER_ERR_V_MUIC,
	.afc_name		= "AFC charger ERR V",
	.afc_irq		= MUIC_AFC_IRQ_VBADC,
	.status_vbadc		= VBADC_AFC_ERR_V,
	.status_vdnmon          = VDNMON_DONTCARE,
	.function_num		= FUNC_PREPARE_DUPLI_TO_AFC_ERR_V,
	.next			= &prepare_dupli_to_afc_9v,
};

muic_afc_data_t prepare_dupli_to_afc_5v = {
	.new_dev		= ATTACHED_DEV_AFC_CHARGER_5V_MUIC,
	.afc_name		= "AFC charger 5V",
	.afc_irq		= MUIC_AFC_IRQ_VBADC,
	.status_vbadc		= VBADC_AFC_5V,
	.status_vdnmon          = VDNMON_DONTCARE,
	.function_num		= FUNC_PREPARE_DUPLI_TO_AFC_5V,
	.next			= &prepare_dupli_to_afc_err_v,
};

muic_afc_data_t afc_5v_to_afc_9v = {
	.new_dev		= ATTACHED_DEV_AFC_CHARGER_5V_MUIC,
	.afc_name		= "AFC charger 9V",
	.afc_irq		= MUIC_AFC_IRQ_VBADC,
	.status_vbadc		= VBADC_AFC_5V,
	.status_vdnmon          = VDNMON_DONTCARE,
	.function_num		= FUNC_AFC_5V_TO_AFC_9V,
	.next			= &afc_5v_to_afc_9v,
};

muic_afc_data_t afc_5v_to_afc_err_v = {
	.new_dev		= ATTACHED_DEV_AFC_CHARGER_ERR_V_MUIC,
	.afc_name		= "AFC charger ERR V",
	.afc_irq		= MUIC_AFC_IRQ_VBADC,
	.status_vbadc		= VBADC_AFC_ERR_V,
	.status_vdnmon          = VDNMON_DONTCARE,
	.function_num		= FUNC_AFC_5V_TO_AFC_ERR_V,
	.next			= &afc_5v_to_afc_9v,
};

muic_afc_data_t afc_5v_to_afc_5v_dupli = {
	.new_dev		= ATTACHED_DEV_AFC_CHARGER_PREPARE_DUPLI_MUIC,
	.afc_name		= "AFC charger 5V (mrxrdy)",
	.afc_irq		= MUIC_AFC_IRQ_MRXRDY,
	.status_vbadc		= VBADC_AFC_5V,
	.status_vdnmon          = VDNMON_DONTCARE,
	.function_num		= FUNC_AFC_5V_TO_AFC_5V_DUPLI,
	.next			= &prepare_dupli_to_prepare_dupli,
};

muic_afc_data_t afc_5v_dupli_to_afc_5v_dupli = {
	.new_dev		= ATTACHED_DEV_AFC_CHARGER_5V_DUPLI_MUIC,
	.afc_name		= "AFC charger 5V (mrxrdy)",
	.afc_irq		= MUIC_AFC_IRQ_MRXRDY,
	.status_vbadc		= VBADC_AFC_5V,
	.status_vdnmon          = VDNMON_DONTCARE,
	.function_num		= FUNC_AFC_5V_DUPLI_TO_AFC_5V_DUPLI,
	.next			= &afc_5v_dupli_to_afc_5v_dupli,
};

muic_afc_data_t afc_5v_dupli_to_afc_9v = {
	.new_dev		= ATTACHED_DEV_AFC_CHARGER_9V_MUIC,
	.afc_name		= "AFC charger 9V",
	.afc_irq		= MUIC_AFC_IRQ_VBADC,
	.status_vbadc		= VBADC_AFC_9V,
	.status_vdnmon          = VDNMON_DONTCARE,
	.function_num		= FUNC_AFC_5V_DUPLI_TO_AFC_9V,
	.next			= &afc_5v_dupli_to_afc_5v_dupli,
};

muic_afc_data_t afc_5v_dupli_to_afc_err_v = {
	.new_dev		= ATTACHED_DEV_AFC_CHARGER_ERR_V_MUIC,
	.afc_name		= "AFC charger ERR V",
	.afc_irq		= MUIC_AFC_IRQ_VBADC,
	.status_vbadc		= VBADC_AFC_ERR_V,
	.status_vdnmon          = VDNMON_DONTCARE,
	.function_num		= FUNC_AFC_5V_DUPLI_TO_AFC_ERR_V,
	.next			= &afc_5v_dupli_to_afc_9v,
};

muic_afc_data_t afc_err_v_to_afc_9v = {
	.new_dev		= ATTACHED_DEV_AFC_CHARGER_9V_MUIC,
	.afc_name		= "AFC charger 9V",
	.afc_irq		= MUIC_AFC_IRQ_VBADC,
	.status_vbadc		= VBADC_AFC_9V,
	.status_vdnmon          = VDNMON_DONTCARE,
	.function_num		= FUNC_AFC_ERR_V_TO_AFC_9V,
	.next			= &afc_err_v_to_afc_9v,
};

muic_afc_data_t afc_err_v_to_afc_err_v_dupli = {
	.new_dev		= ATTACHED_DEV_AFC_CHARGER_ERR_V_DUPLI_MUIC,
	.afc_name		= "AFC charger ERR V (mrxrdy)",
	.afc_irq		= MUIC_AFC_IRQ_MRXRDY,
	.status_vbadc		= VBADC_AFC_ERR_V,
	.status_vdnmon          = VDNMON_DONTCARE,
	.function_num		= FUNC_AFC_ERR_V_TO_AFC_ERR_V_DUPLI,
	.next			= &afc_err_v_to_afc_9v,
};

muic_afc_data_t afc_err_v_dupli_to_afc_9v = {
	.new_dev		= ATTACHED_DEV_AFC_CHARGER_9V_MUIC,
	.afc_name		= "AFC charger 9V",
	.afc_irq		= MUIC_AFC_IRQ_VBADC,
	.status_vbadc		= VBADC_AFC_9V,
	.status_vdnmon          = VDNMON_DONTCARE,
	.function_num		= FUNC_AFC_ERR_V_DUPLI_TO_AFC_9V,
	.next			= &afc_err_v_dupli_to_afc_9v,
};

muic_afc_data_t afc_err_v_dupli_to_afc_err_v_dupli = {
	.new_dev		= ATTACHED_DEV_AFC_CHARGER_ERR_V_DUPLI_MUIC,
	.afc_name		= "AFC charger ERR V (mrxrdy)",
	.afc_irq		= MUIC_AFC_IRQ_MRXRDY,
	.status_vbadc		= VBADC_AFC_ERR_V,
	.status_vdnmon          = VDNMON_DONTCARE,
	.function_num	= FUNC_AFC_ERR_V_DUPLI_TO_AFC_ERR_V_DUPLI,
	.next			= &afc_err_v_dupli_to_afc_9v,
};

muic_afc_data_t qc_prepare_to_qc_9v = {
	.new_dev		= ATTACHED_DEV_QC_CHARGER_9V_MUIC,
	.afc_name		= "QC charger 9V",
	.afc_irq		= MUIC_AFC_IRQ_VBADC,
	.status_vbadc		= VBADC_QC_9V,
	.status_vdnmon          = VDNMON_DONTCARE,
	.function_num		= FUNC_QC_PREPARE_TO_QC_9V,
	.next			= &qc_prepare_to_qc_9v,
};

muic_afc_data_t qc_9v_to_qc_9v = {
	.new_dev		= ATTACHED_DEV_QC_CHARGER_9V_MUIC,
	.afc_name		= "QC charger 9V",
	.afc_irq		= MUIC_AFC_IRQ_DONTCARE,
	.status_vbadc		= VBADC_QC_9V,
	.status_vdnmon          = VDNMON_DONTCARE,
	.function_num		= FUNC_QC_PREPARE_TO_QC_9V,
	.next			= &qc_9v_to_qc_9v,
};

/* afc_condition_checklist[ATTACHED_DEV_AFC_CHARGER_9V_MUIC] */
muic_afc_data_t afc_9v_to_afc_5v = {
	.new_dev		= ATTACHED_DEV_AFC_CHARGER_5V_MUIC,
	.afc_name		= "AFC charger 5V",
	.afc_irq		= MUIC_AFC_IRQ_VBADC,
	.status_vbadc		= VBADC_AFC_5V,
	.status_vdnmon          = VDNMON_DONTCARE,
	.function_num		= FUNC_AFC_9V_TO_AFC_5V,
	.next			= &afc_5v_to_afc_5v_dupli,
};

/* afc_condition_checklist[ATTACHED_DEV_AFC_CHARGER_9V_MUIC] */
muic_afc_data_t afc_9v_to_afc_9v = {
	.new_dev		= ATTACHED_DEV_AFC_CHARGER_9V_MUIC,
	.afc_name		= "AFC charger 9V",
	.afc_irq		= MUIC_AFC_IRQ_DONTCARE,
	.status_vbadc		= VBADC_AFC_9V,
	.status_vdnmon          = VDNMON_DONTCARE,
	.function_num		= FUNC_AFC_9V_TO_AFC_9V,
	.next			= &afc_9v_to_afc_5v,
};

muic_afc_data_t		*afc_condition_checklist[ATTACHED_DEV_NUM] = {
	[ATTACHED_DEV_TA_MUIC]			= &ta_to_prepare,
	[ATTACHED_DEV_AFC_CHARGER_PREPARE_MUIC]	= &prepare_to_prepare_dupli,
	[ATTACHED_DEV_AFC_CHARGER_PREPARE_DUPLI_MUIC] = &prepare_dupli_to_afc_5v,
	[ATTACHED_DEV_AFC_CHARGER_5V_MUIC]	= &afc_5v_to_afc_5v_dupli,
	[ATTACHED_DEV_AFC_CHARGER_5V_DUPLI_MUIC] = &afc_5v_dupli_to_afc_err_v,
	[ATTACHED_DEV_AFC_CHARGER_ERR_V_MUIC]	= &afc_err_v_to_afc_err_v_dupli,
	[ATTACHED_DEV_AFC_CHARGER_ERR_V_DUPLI_MUIC] = &afc_err_v_dupli_to_afc_err_v_dupli,
	[ATTACHED_DEV_AFC_CHARGER_9V_MUIC]	= &afc_9v_to_afc_9v,
	[ATTACHED_DEV_QC_CHARGER_PREPARE_MUIC]	= &qc_prepare_to_qc_9v,
	[ATTACHED_DEV_QC_CHARGER_9V_MUIC]	= &qc_9v_to_qc_9v,
};

struct afc_init_data_s {
	struct work_struct muic_afc_init_work;
	struct s2mu004_muic_data *muic_data;
};
struct afc_init_data_s afc_init_data;

bool muic_check_is_hv_dev(struct s2mu004_muic_data *muic_data)
{
	bool ret = false;
	struct muic_platform_data *muic_pdata = muic_data->pdata;

	switch (muic_pdata->attached_dev) {
	case ATTACHED_DEV_AFC_CHARGER_PREPARE_MUIC:
	case ATTACHED_DEV_AFC_CHARGER_PREPARE_DUPLI_MUIC:
	case ATTACHED_DEV_AFC_CHARGER_5V_MUIC:
	case ATTACHED_DEV_AFC_CHARGER_5V_DUPLI_MUIC:
	case ATTACHED_DEV_AFC_CHARGER_9V_MUIC:
	case ATTACHED_DEV_AFC_CHARGER_ERR_V_MUIC:
	case ATTACHED_DEV_AFC_CHARGER_ERR_V_DUPLI_MUIC:
	case ATTACHED_DEV_HV_ID_ERR_UNDEFINED_MUIC:
	case ATTACHED_DEV_HV_ID_ERR_UNSUPPORTED_MUIC:
	case ATTACHED_DEV_HV_ID_ERR_SUPPORTED_MUIC:
		ret = true;
		break;
	default:
		ret = false;
		break;
	}

	if (debug_en_checklist)
		pr_info("%s attached_dev(%d)[%c]\n",
			__func__, muic_pdata->attached_dev, (ret ? 'T' : 'F'));

	return ret;
}

muic_attached_dev_t hv_muic_check_id_err(struct s2mu004_muic_data *muic_data,
	muic_attached_dev_t new_dev)
{
	muic_attached_dev_t after_new_dev = new_dev;
	struct muic_platform_data *muic_pdata = muic_data->pdata;

	if (!muic_check_is_hv_dev(muic_data))
		goto out;

	switch (new_dev) {
	case ATTACHED_DEV_TA_MUIC:
		pr_info("%s cannot change HV(%d)->TA(%d)!\n",
			__func__, muic_pdata->attached_dev, new_dev);
		after_new_dev = muic_pdata->attached_dev;
		break;
	case ATTACHED_DEV_UNDEFINED_CHARGING_MUIC:
		pr_info("%s Undefined\n", __func__);
		after_new_dev = ATTACHED_DEV_HV_ID_ERR_UNDEFINED_MUIC;
		break;
	case ATTACHED_DEV_UNSUPPORTED_ID_VB_MUIC:
		pr_info("%s Unsupported\n", __func__);
		after_new_dev = ATTACHED_DEV_HV_ID_ERR_UNSUPPORTED_MUIC;
		break;
	default:
		pr_info("%s Supported\n", __func__);
		after_new_dev = ATTACHED_DEV_HV_ID_ERR_SUPPORTED_MUIC;
		break;
	}
out:
	return after_new_dev;
}

static int s2mu004_hv_muic_write_reg(struct i2c_client *i2c, u8 reg, u8 value)
{
	u8 before_val, after_val;
	int ret;

	s2mu004_read_reg(i2c, reg, &before_val);
	ret = s2mu004_write_reg(i2c, reg, value);
	s2mu004_read_reg(i2c, reg, &after_val);

	pr_info("%s reg[0x%02x] = [0x%02x] + [0x%02x] -> [0x%02x]\n",
		__func__, reg, before_val, value, after_val);
	return ret;
}

int s2mu004_muic_hv_update_reg(struct i2c_client *i2c,
	const u8 reg, const u8 val, const u8 mask, const bool debug_en)
{
	u8 before_val, new_val, after_val;
	int ret = 0;

	ret = s2mu004_read_reg(i2c, reg, &before_val);
	if (ret)
		pr_err("%s failed to read REG(0x%02x) [%d]\n",
			__func__, reg, ret);

	new_val = (val & mask) | (before_val & (~mask));

	if (before_val ^ new_val) {
		ret = s2mu004_hv_muic_write_reg(i2c, reg, new_val);
		if (ret)
			pr_err("%s failed to write REG(0x%02x) [%d]\n",
					__func__, reg, ret);
	} else if (debug_en) {
		pr_info("%s REG(0x%02x): already [0x%02x], don't write reg\n",
				__func__, reg, before_val);
		goto out;
	}

	if (debug_en) {
		ret = s2mu004_read_reg(i2c, reg, &after_val);
		if (ret < 0)
			pr_err("%s failed to read REG(0x%02x) [%d]\n",
					__func__, reg, ret);

		pr_info("%s REG(0x%02x): [0x%02x]+[0x%02x:0x%02x]=[0x%02x]\n",
				__func__, reg, before_val,
				val, mask, after_val);
	}

out:
	return ret;
}

void s2mu004_hv_muic_reset_hvcontrol_reg(struct s2mu004_muic_data *muic_data)
{
	struct i2c_client *i2c = muic_data->i2c;

	s2mu004_hv_muic_write_reg(i2c, 0x4b, 0x00);
	s2mu004_hv_muic_write_reg(i2c, 0x49, 0x00);
	s2mu004_hv_muic_write_reg(i2c, 0x4a, 0x00);
	s2mu004_hv_muic_write_reg(i2c, 0x5f, 0x01);

	muic_data->is_afc_muic_prepare = false;
	s2mu004_muic_set_afc_ready(muic_data, false);

	cancel_delayed_work(&muic_data->afc_send_mpnack);
	cancel_delayed_work(&muic_data->afc_control_ping_retry);
	cancel_delayed_work(&muic_data->afc_qc_retry);
}

void s2mu004_muic_set_afc_ready(struct s2mu004_muic_data *muic_data, bool value)
{
	bool before, after;

	before = muic_data->is_afc_muic_ready;
	muic_data->is_afc_muic_ready = value;
	after = muic_data->is_afc_muic_ready;

	pr_info("%s afc_muic_ready[%d->%d]\n", __func__, before, after);
}

static int s2mu004_hv_muic_state_maintain(struct s2mu004_muic_data *muic_data)
{
	int ret = 0;
	struct muic_platform_data *muic_pdata = muic_data->pdata;

	pr_info("%s\n", __func__);

	if (muic_pdata->attached_dev == ATTACHED_DEV_NONE_MUIC) {
		pr_info("%s Detached(%d), need to check after\n",
				__func__, muic_pdata->attached_dev);
		return ret;
	}

	return ret;
}

static void s2mu004_mpnack_irq_mask(struct s2mu004_muic_data *muic_data, int enable)
{
	pr_info("%s: irq enable: %d\n", __func__, enable);
	if (enable)
		s2mu004_muic_hv_update_reg(muic_data->i2c, 0x05, 0x00, 0x08, 0);
	else
		s2mu004_muic_hv_update_reg(muic_data->i2c, 0x05, 0x08, 0x08, 0);
}

void s2mu004_hv_muic_set_afc_after_prepare(struct s2mu004_muic_data *muic_data)
{
#if !defined(CONFIG_SEC_FACTORY)
        struct muic_interface_t *muic_if = (struct muic_interface_t *)muic_data->if_data;                          
#endif
	pr_info("%s HV charger is detected\n", __func__);

	if (!is_charger_ready) {
		pr_info("%s charger is not ready\n", __func__);
		return;
	}

#if defined(CONFIG_SEC_FACTORY)
	if (1) { 
#else
	if (muic_if->is_ccic_rp56_enable) { 
#endif
		muic_notifier_attach_attached_dev(ATTACHED_DEV_AFC_CHARGER_PREPARE_MUIC);

		muic_data->retry_cnt = 0;
		s2mu004_mpnack_irq_mask(muic_data, 0);
		s2mu004_hv_muic_write_reg(muic_data->i2c, 0x5f, 0x05);
		s2mu004_hv_muic_write_reg(muic_data->i2c, 0x4A, 0x0e);
		schedule_delayed_work(&muic_data->afc_send_mpnack, msecs_to_jiffies(2000));
		schedule_delayed_work(&muic_data->afc_control_ping_retry, msecs_to_jiffies(50));
	}
}

void s2mu004_muic_afc_after_prepare(struct work_struct *work)
{
	struct s2mu004_muic_data *muic_data =
		container_of(work, struct s2mu004_muic_data, afc_after_prepare.work);
	int vbvolt;
	vbvolt = s2mu004_muic_get_vbus_state(muic_data);
	pr_info("%s\n ", __func__);
	if(vbvolt){
	mutex_lock(&muic_data->afc_mutex);
	s2mu004_hv_muic_set_afc_after_prepare(muic_data);
	mutex_unlock(&muic_data->afc_mutex);
	}
	else
	pr_info("%s No VBUS, just return\n", __func__);
		
}

#define RETRY_QC_CNT 3
void s2mu004_muic_afc_qc_retry(struct work_struct *work)
{
	struct s2mu004_muic_data *muic_data =
		container_of(work, struct s2mu004_muic_data, afc_qc_retry.work);
	pr_info("%s retry_qc_cnt : (%d)\n ", __func__, muic_data->retry_qc_cnt);
	if (muic_data->retry_qc_cnt < RETRY_QC_CNT) {
		muic_data->retry_qc_cnt++;
		s2mu004_hv_muic_write_reg(muic_data->i2c, 0x49, 0x00);
		msleep(20);
		s2mu004_hv_muic_write_reg(muic_data->i2c, 0x49, 0xbd);
		schedule_delayed_work(&muic_data->afc_qc_retry, msecs_to_jiffies(100));
	} else {
		muic_data->retry_qc_cnt = 0;
		muic_data->qc_prepare = 0;
	}

}

#define RETRY_PING_CNT 20
static void s2mu004_muic_afc_control_ping_retry(struct work_struct *work)
{
	struct s2mu004_muic_data *muic_data =
		container_of(work, struct s2mu004_muic_data, afc_control_ping_retry.work);
	pr_info("%s retry_cnt : %d\n", __func__, muic_data->retry_cnt);

	if (muic_data->retry_cnt <  RETRY_PING_CNT) {
		muic_data->retry_cnt++;
		s2mu004_hv_muic_write_reg(muic_data->i2c, 0x4A, 0x0e);
		schedule_delayed_work(&muic_data->afc_control_ping_retry, msecs_to_jiffies(50));
	} else {
		muic_data->retry_cnt = 0;
		s2mu004_mpnack_irq_mask(muic_data, 1); /* enable mpnack irq */
		s2mu004_hv_muic_write_reg(muic_data->i2c, 0x4A, 0x0e);
	}
}

static void s2mu004_hv_muic_afc_control_ping
		(struct s2mu004_muic_data *muic_data, bool ping_continue)
{
	pr_info("%s [%d, %c]\n", __func__,
		muic_data->afc_count, ping_continue ? 'T' : 'F');
	if (ping_continue) {
		msleep(30);
		s2mu004_hv_muic_write_reg(muic_data->i2c, 0x4A, 0x0e);
		schedule_delayed_work(&muic_data->afc_send_mpnack, msecs_to_jiffies(2000));
		schedule_delayed_work(&muic_data->afc_control_ping_retry, msecs_to_jiffies(50));
	}
}

static int s2mu004_hv_muic_handle_attach
		(struct s2mu004_muic_data *muic_data, const muic_afc_data_t *new_afc_data)
{
	int ret = 0;
	bool noti = true;
	muic_attached_dev_t new_dev = new_afc_data->new_dev;
	struct muic_platform_data *muic_pdata = muic_data->pdata;

	pr_info("%s %d\n", __func__, new_afc_data->function_num);

	if (muic_data->is_charger_ready == false) {
		if (new_afc_data->new_dev == ATTACHED_DEV_AFC_CHARGER_PREPARE_MUIC) {
			muic_data->is_afc_muic_prepare = true;
			pr_info("%s is_charger_ready[%c], is_afc_muic_prepare[%c]\n",
				__func__,
				(muic_data->is_charger_ready ? 'T' : 'F'),
				(muic_data->is_afc_muic_prepare ? 'T' : 'F'));

			return ret;
		}
		pr_info("%s is_charger_ready[%c], just return\n",
			__func__, (muic_data->is_charger_ready ? 'T' : 'F'));
		return ret;
	}

	switch (new_afc_data->function_num) {
	case FUNC_TA_TO_PREPARE:
		new_dev = ATTACHED_DEV_AFC_CHARGER_5V_MUIC;
		schedule_delayed_work(&muic_data->afc_after_prepare, msecs_to_jiffies(60));
		muic_data->afc_count = 0;
		break;
	case FUNC_PREPARE_TO_PREPARE_DUPLI:
		muic_data->afc_count++;
		if (muic_data->afc_count < 3) {
			s2mu004_hv_muic_afc_control_ping(muic_data, true);
			noti = false;
		}
		break;
	case FUNC_PREPARE_TO_AFC_5V:
		if (muic_data->afc_count > AFC_CHARGER_WA_PING) {
			s2mu004_hv_muic_afc_control_ping(muic_data, false);
		} else {
			s2mu004_hv_muic_afc_control_ping(muic_data, true);
			noti = false;
		}
		break;
	case FUNC_PREPARE_TO_QC_PREPARE:
		muic_data->afc_count = 0;
		muic_data->qc_prepare = 1;
		msleep(60);
		s2mu004_hv_muic_write_reg(muic_data->i2c, 0x5f, 0x01);
		s2mu004_hv_muic_write_reg(muic_data->i2c, 0x49, 0xbd);
		schedule_delayed_work(&muic_data->afc_qc_retry, msecs_to_jiffies(100));
		break;
	case FUNC_PREPARE_DUPLI_TO_PREPARE_DUPLI:
		pr_err("[DEBUG] %s(%d)dupli, dupli\n", __func__, __LINE__);
		muic_data->afc_count++;
		if (muic_data->afc_count < 3) {
			s2mu004_hv_muic_afc_control_ping(muic_data, true);
			noti = false;
		}
		break;
	case FUNC_PREPARE_DUPLI_TO_AFC_5V:
		break;
	case FUNC_PREPARE_DUPLI_TO_AFC_ERR_V:
		if (muic_data->afc_count > AFC_CHARGER_WA_PING) {
			s2mu004_hv_muic_afc_control_ping(muic_data, false);
		} else {
			s2mu004_hv_muic_afc_control_ping(muic_data, true);
			noti = false;
		}
		break;
	case FUNC_PREPARE_DUPLI_TO_AFC_9V:
		s2mu004_hv_muic_afc_control_ping(muic_data, false);
		break;
	case FUNC_AFC_5V_TO_AFC_5V_DUPLI:
		/*
		 * attached_dev is changed. MPING Missing did not happened
		 * Cancel delayed work
		 */
		pr_info("%s cancel_delayed_work(dev %d), Mping missing wa\n",
			__func__, new_dev);
		muic_data->afc_count++;
		if (muic_data->afc_count < 3) {
			s2mu004_hv_muic_afc_control_ping(muic_data, true);
			noti = false;
		}
		break;
	case FUNC_AFC_5V_TO_AFC_ERR_V:
		/*
		 * attached_dev is changed. MPING Missing did not happened
		 * Cancel delayed work
		 */
		pr_info("%s cancel_delayed_work(dev %d), Mping missing wa\n",
			__func__, new_dev);
		if (muic_data->afc_count > AFC_CHARGER_WA_PING) {
			s2mu004_hv_muic_afc_control_ping(muic_data, false);
		} else {
			s2mu004_hv_muic_afc_control_ping(muic_data, true);
			noti = false;
		}
		break;
	case FUNC_AFC_5V_TO_AFC_9V:
		/*
		 * attached_dev is changed. MPING Missing did not happened
		 * Cancel delayed work
		 */
		pr_info("%s cancel_delayed_work(dev %d), Mping missing wa\n",
			__func__, new_dev);
		s2mu004_hv_muic_write_reg(muic_data->i2c, 0x05, 0x00);

		break;
	case FUNC_AFC_5V_DUPLI_TO_AFC_5V_DUPLI:
		muic_data->afc_count++;
		if (muic_data->afc_count > AFC_CHARGER_WA_PING) {
			s2mu004_hv_muic_afc_control_ping(muic_data, false);
		} else {
			s2mu004_hv_muic_afc_control_ping(muic_data, true);
			noti = false;
		}
		break;
	case FUNC_AFC_5V_DUPLI_TO_AFC_ERR_V:
		if (muic_data->afc_count > AFC_CHARGER_WA_PING) {
			s2mu004_hv_muic_afc_control_ping(muic_data, false);
		} else {
			s2mu004_hv_muic_afc_control_ping(muic_data, true);
			noti = false;
		}
		break;
	case FUNC_AFC_5V_DUPLI_TO_AFC_9V:
		s2mu004_hv_muic_afc_control_ping(muic_data, false);
#ifdef CONFIG_MUIC_HV_FORCE_LIMIT
			if (muic_data->pdata->silent_chg_change_state == SILENT_CHG_CHANGING)
				noti = false;
#endif
		break;
	case FUNC_AFC_ERR_V_TO_AFC_ERR_V_DUPLI:
		muic_data->afc_count++;
		if (muic_data->afc_count > AFC_CHARGER_WA_PING) {
			s2mu004_hv_muic_afc_control_ping(muic_data, false);
		} else {
			s2mu004_hv_muic_afc_control_ping(muic_data, true);
			noti = false;
		}
		break;
	case FUNC_AFC_ERR_V_TO_AFC_9V:
		s2mu004_hv_muic_afc_control_ping(muic_data, false);
		break;
	case FUNC_AFC_ERR_V_DUPLI_TO_AFC_ERR_V_DUPLI:
		muic_data->afc_count++;
		if (muic_data->afc_count > AFC_CHARGER_WA_PING) {
			s2mu004_hv_muic_afc_control_ping(muic_data, false);
		} else {
			s2mu004_hv_muic_afc_control_ping(muic_data, true);
			noti = false;
		}
		break;
	case FUNC_AFC_ERR_V_DUPLI_TO_AFC_9V:
		s2mu004_hv_muic_afc_control_ping(muic_data, false);
		break;
	case FUNC_AFC_9V_TO_AFC_5V:
		break;
	case FUNC_AFC_9V_TO_AFC_9V:
#ifdef CONFIG_MUIC_HV_FORCE_LIMIT
		muic_data->afc_count++;
		if (muic_data->afc_count > AFC_CHARGER_WA_PING)
			s2mu004_hv_muic_afc_control_ping(muic_data, false);
		else
			pr_info("dummy int called [%d]\n", muic_data->afc_count);
#else
		muic_data->afc_count++;
		if (muic_data->afc_count < AFC_CHARGER_WA_PING)
			s2mu004_hv_muic_afc_control_ping(muic_data, true);
#endif
		break;
	case FUNC_QC_PREPARE_TO_QC_9V:
		pr_info("%s FUNC_QC_PREPARE_TO_QC_9V\n", __func__);
		cancel_delayed_work(&muic_data->afc_qc_retry);
		break;
	default:
		pr_warn("%s undefinded hv function num(%d)\n",
					__func__, new_afc_data->function_num);
		ret = -ESRCH;
		goto out;
	}

	if (muic_pdata->attached_dev == new_dev)
		noti = false;
	else if (new_dev == ATTACHED_DEV_AFC_CHARGER_PREPARE_DUPLI_MUIC ||
		new_dev == ATTACHED_DEV_AFC_CHARGER_5V_DUPLI_MUIC ||
		new_dev == ATTACHED_DEV_AFC_CHARGER_ERR_V_DUPLI_MUIC)
		noti = false;

	if (noti)
		muic_notifier_attach_attached_dev(new_dev);

	muic_pdata->attached_dev = new_dev;
out:
	return ret;
}

static bool muic_check_hv_irq
			(struct s2mu004_muic_data *muic_data,
			const muic_afc_data_t *afc_data, int irq)
{
	int afc_irq = 0;
	bool ret = false;

	pr_info("%s irq %d , irq_dnres %d, irq_mrxrdy %d, irq_vbadc %d irq_mpnack %d irq_vdnmon %d\n",
		__func__, irq, muic_data->irq_dnres, muic_data->irq_mrxrdy, muic_data->irq_vbadc,
		muic_data->irq_mpnack, muic_data->irq_vdnmon);
	/* change irq num to muic_afc_irq_t */

	if (irq == muic_data->irq_vbadc)
		afc_irq = MUIC_AFC_IRQ_VBADC;
	else if (irq == muic_data->irq_mrxrdy)
		afc_irq = MUIC_AFC_IRQ_MRXRDY;
	else if (irq == muic_data->irq_vdnmon)
		afc_irq = MUIC_AFC_IRQ_VDNMON;
	else if (irq == muic_data->irq_mpnack)
		afc_irq = MUIC_AFC_IRQ_MPNACK;
	else {
		pr_err("%s cannot find irq #(%d)\n", __func__, irq);
		ret = false;
		goto out;
	}

	pr_info("%s afc_irq %d , %d\n",
		__func__, afc_data->afc_irq, afc_irq);
	if (afc_data->afc_irq == afc_irq) {
		ret = true;
		goto out;
	}

	if (afc_data->afc_irq == MUIC_AFC_IRQ_DONTCARE) {
		ret = true;
		goto out;
	}

out:
	if (debug_en_checklist)
		pr_info("%s check_data dev(%d) irq(%d:%d) ret(%c)\n",
				__func__, afc_data->new_dev,
				afc_data->afc_irq, afc_irq, ret ? 'T' : 'F');
	return ret;
}

static bool muic_check_status_vbadc
			(const muic_afc_data_t *afc_data, u8 vbadc)
{
	bool ret = false;

	if (afc_data->status_vbadc == vbadc) {
		ret = true;
		goto out;
	}

	if (afc_data->status_vbadc == VBADC_AFC_5V) {
		switch (vbadc) {
		case VBADC_5_3V:
		case VBADC_5_7V_6_3V:
			ret = true;
			goto out;
		default:
			break;
		}
	}

	if (afc_data->status_vbadc == VBADC_AFC_9V) {
		switch (vbadc) {
		case VBADC_8_7V_9_3V:
		case VBADC_9_7V_10_3V:
			ret = true;
			goto out;
		default:
			break;
		}
	}

	if (afc_data->status_vbadc == VBADC_AFC_ERR_V_NOT_0) {
		switch (vbadc) {
		case VBADC_7_7V_8_3V:
		case VBADC_10_7V_11_3V:
		case VBADC_11_7V_12_3V:
		case VBADC_12_7V_13_3V:
		case VBADC_13_7V_14_3V:
		case VBADC_14_7V_15_3V:
		case VBADC_15_7V_16_3V:
		case VBADC_16_7V_17_3V:
		case VBADC_17_7V_18_3V:
		case VBADC_18_7V_19_3V:
			ret = true;
			goto out;
		default:
			break;
		}
	}

	if (afc_data->status_vbadc == VBADC_AFC_ERR_V) {
		switch (vbadc) {
		case VBADC_7_7V_8_3V:
		case VBADC_10_7V_11_3V:
		case VBADC_11_7V_12_3V:
		case VBADC_12_7V_13_3V:
		case VBADC_13_7V_14_3V:
		case VBADC_14_7V_15_3V:
		case VBADC_15_7V_16_3V:
		case VBADC_16_7V_17_3V:
		case VBADC_17_7V_18_3V:
		case VBADC_18_7V_19_3V:
		case VBADC_19_7V:
			ret = true;
			goto out;
		default:
			break;
		}
	}
#if 0
	if (afc_data->status_vbadc == VBADC_QC_5V) {
		switch (vbadc) {
		case VBADC_4V_5V:
		case VBADC_5V_6V:
			ret = true;
			goto out;
		default:
			break;
		}
	}

	if (afc_data->status_vbadc == VBADC_QC_9V) {
		switch (vbadc) {
		case VBADC_6V_7V:
		case VBADC_7V_8V:
		case VBADC_8V_9V:
		case VBADC_9V_10V:
			ret = true;
			goto out;
		default:
			break;
		}
	}
#endif
	if (afc_data->status_vbadc == VBADC_ANY) {
		switch (vbadc) {
		case VBADC_5_3V:
		case VBADC_5_7V_6_3V:
		case VBADC_6_7V_7_3V:
		case VBADC_7_7V_8_3V:
		case VBADC_8_7V_9_3V:
		case VBADC_9_7V_10_3V:
		case VBADC_10_7V_11_3V:
		case VBADC_11_7V_12_3V:
		case VBADC_12_7V_13_3V:
		case VBADC_13_7V_14_3V:
		case VBADC_14_7V_15_3V:
		case VBADC_15_7V_16_3V:
		case VBADC_16_7V_17_3V:
		case VBADC_17_7V_18_3V:
		case VBADC_18_7V_19_3V:
		case VBADC_19_7V:
			ret = true;
			goto out;
		default:
			break;
		}
	}

	if (afc_data->status_vbadc == VBADC_DONTCARE) {
		ret = true;
		goto out;
	}

out:
	if (debug_en_checklist)
		pr_info("%s check_data dev(%d) vbadc(0x%x:0x%x) ret(%c)\n",
				__func__, afc_data->new_dev,
				afc_data->status_vbadc, vbadc, ret ? 'T' : 'F');
	return ret;
}

static bool muic_check_status_vdnmon
			(const muic_afc_data_t *afc_data, u8 vdnmon)
{
	bool ret = false;

	if (afc_data->status_vdnmon == vdnmon) {
		ret = true;
		goto out;
	}

	if (afc_data->status_vdnmon == VDNMON_DONTCARE) {
		ret = true;
		goto out;
	}

out:
	if (debug_en_checklist)
		pr_info("%s check_data dev(%d) vdnmon(0x%x:0x%x) ret(%c)\n",
				__func__, afc_data->new_dev,
				afc_data->status_vdnmon, vdnmon, ret ? 'T' : 'F');

	return ret;
}

bool muic_check_dev_ta(struct s2mu004_muic_data *muic_data)
{
	u8 status1 = muic_data->status1;
	u8 status2 = muic_data->status2;
	u8 status4 = muic_data->status4;
	u8 adc, vbvolt, chgtyp;
	struct muic_platform_data *muic_pdata = muic_data->pdata;

	adc = status1 & ADC_MASK;
	vbvolt = status2 & DEV_TYPE_APPLE_VBUS_WAKEUP;
	chgtyp = status4 & DEV_TYPE_DCPCHG;

	pr_info("%s adc %d\n", __func__, adc);
	if (adc != ADC_OPEN) {
		s2mu004_muic_set_afc_ready(muic_data, false);
		return false;
	}
	if (vbvolt == 0 || chgtyp == 0) {
		s2mu004_muic_set_afc_ready(muic_data, false);
#if defined(CONFIG_MUIC_NOTIFIER)
		muic_notifier_detach_attached_dev(muic_pdata->attached_dev);
#endif
		muic_pdata->attached_dev = ATTACHED_DEV_NONE_MUIC;
		s2mu004_hv_muic_reset_hvcontrol_reg(muic_data);
		return false;
	}

	return true;
}

static void s2mu004_hv_muic_detect_dev(struct s2mu004_muic_data *muic_data, int irq)
{
	struct i2c_client *i2c = muic_data->i2c;
	struct muic_platform_data *muic_pdata = muic_data->pdata;
	const muic_afc_data_t *afc_data = afc_condition_checklist[muic_pdata->attached_dev];
#ifdef CONFIG_CCIC_NOTIFIER
	struct muic_interface_t *muic_if = (struct muic_interface_t *)muic_data->if_data;
#endif
	int intr = MUIC_INTR_DETACH;
	int ret;
	int i;
	u8 adc, dev_app, status, chgt;
	u8 hvcontrol[2];
	u8 vdnmon, vbadc;
	bool flag_next = true;
	bool muic_dev_ta = false;

	if (afc_data == NULL) {
		pr_err("%s non AFC Charger, just return!\n", __func__);
		return;
	}

	ret = s2mu004_read_reg(i2c, S2MU004_REG_MUIC_ADC, &adc);
	if (ret) {
		pr_err("%s fail to read muic reg(%d)\n", __func__, ret);
		return;
	}

	ret = s2mu004_read_reg(i2c, S2MU004_REG_MUIC_DEVICE_APPLE, &dev_app);
	if (ret) {
		pr_err("%s fail to read muic reg(%d)\n", __func__, ret);
		return;
	}

	ret = s2mu004_read_reg(i2c, S2MU004_REG_AFC_STATUS, &status);
	if (ret) {
		pr_err("%s fail to read muic reg(%d)\n", __func__, ret);
		return;
	}

	ret = s2mu004_read_reg(i2c, S2MU004_REG_MUIC_CHG_TYPE, &chgt);
	if (ret) {
		pr_err("%s fail to read muic reg(%d)\n", __func__, ret);
		return;
	}

	pr_info("STATUS1:0x%02x, 2:0x%02x, 3:0x%02x\n",
			adc, dev_app, status);

	muic_data->status1 = adc;
	muic_data->status2 = dev_app;
	muic_data->status3 = status;
	muic_data->status4 = chgt;

	/* check TA type */
#if defined(CONFIG_MUIC_KEYBOARD)
	if ((muic_pdata->attached_dev == ATTACHED_DEV_TA_MUIC)
		|| (muic_pdata->attached_dev == ATTACHED_DEV_AFC_CHARGER_PREPARE_MUIC)
		|| (muic_pdata->attached_dev == ATTACHED_DEV_AFC_CHARGER_PREPARE_DUPLI_MUIC)
		|| (muic_pdata->attached_dev == ATTACHED_DEV_AFC_CHARGER_5V_MUIC)
		|| (muic_pdata->attached_dev == ATTACHED_DEV_AFC_CHARGER_5V_DUPLI_MUIC)
		|| (muic_pdata->attached_dev == ATTACHED_DEV_AFC_CHARGER_9V_MUIC)
		|| (muic_pdata->attached_dev == ATTACHED_DEV_QC_CHARGER_PREPARE_MUIC)
		|| (muic_pdata->attached_dev == ATTACHED_DEV_QC_CHARGER_5V_MUIC)
		|| (muic_pdata->attached_dev == ATTACHED_DEV_QC_CHARGER_9V_MUIC)
		)
		muic_dev_ta = true;
#else
	muic_dev_ta = muic_check_dev_ta(muic_data);
#endif

	if (!muic_dev_ta) {
		pr_err("%s device type is not TA!, attached_dev: %d\n", __func__, muic_pdata->attached_dev);
		return;
	}

	vdnmon = status & STATUS_VDNMON_MASK;
	vbadc = status & STATUS_VBADC_MASK;

	ret = s2mu004_bulk_read(i2c, S2MU004_REG_AFC_CTRL1, 2, hvcontrol);
	if (ret) {
		pr_err("%s fail to read muic reg(%d)\n",
				__func__, ret);
		return;
	}

	pr_info("%s HVCONTROL1:0x%02x, 2:0x%02x\n", __func__,
		hvcontrol[0], hvcontrol[1]);

	/* attached - control */
	muic_data->hvcontrol1 = hvcontrol[0];
	muic_data->hvcontrol2 = hvcontrol[1];
	pr_info("%s vbadc:0x%x\n", __func__, vbadc);

	for (i = 0; i < 10; i++, afc_data = afc_data->next) {

		if (!flag_next) {
			pr_info("%s not found new_dev in afc_condition_checklist\n",
				__func__);
			break;
		}

		pr_err("%s afc_data->name %s\n",
			__func__, afc_data->afc_name);
		if (afc_data->next == afc_data)
			flag_next = false;

		if (!(muic_check_hv_irq(muic_data, afc_data, irq)))
			continue;

		if (!(muic_check_status_vbadc(afc_data, vbadc)))
			continue;

		if (!(muic_check_status_vdnmon(afc_data, vdnmon)))
			continue;

		pr_err("%s checklist match found at i(%d), %s(%d)\n",
			__func__, i, afc_data->afc_name,
			afc_data->new_dev);

		intr = MUIC_INTR_ATTACH;

		break;
	}

	if (intr == MUIC_INTR_ATTACH) {
		pr_err("%s AFC ATTACHED %d->%d\n", __func__,
			muic_pdata->attached_dev, afc_data->new_dev);
#ifdef CONFIG_CCIC_NOTIFIER
		muic_manager_set_legacy_dev(muic_if, afc_data->new_dev);
#endif
		ret = s2mu004_hv_muic_handle_attach(muic_data, afc_data);
		if (ret)
			pr_err("%s cannot handle attach(%d)\n",
				__func__, ret);
	} else {
		pr_info("%s AFC MAINTAIN (%d)\n", __func__,
				muic_pdata->attached_dev);
		ret = s2mu004_hv_muic_state_maintain(muic_data);
		if (ret)
			pr_err("%s cannot maintain state(%d)\n",
				__func__, ret);
		goto out;
	}

out:
	return;
}

#define MASK(width, shift)	(((0x1 << (width)) - 1) << shift)
#define CHGIN_STATUS_SHIFT	5
#define CHGIN_STATUS_WIDTH	3
#define CHGIN_STATUS_MASK	MASK(CHGIN_STATUS_WIDTH, CHGIN_STATUS_SHIFT)

void s2mu004_muic_afc_check_vbadc(struct work_struct *work)
{
	struct s2mu004_muic_data *muic_data =
		container_of(work, struct s2mu004_muic_data, afc_check_vbadc.work);
	int ret;
	u8 val_vbadc, chg_sts0;
	struct muic_platform_data *muic_pdata = muic_data->pdata;

	ret = s2mu004_read_reg(muic_data->i2c, S2MU004_REG_AFC_STATUS, &val_vbadc);
	if (ret)
		pr_err("%s fail to read muic reg(%d)\n", __func__, ret);

	ret = s2mu004_read_reg(muic_data->i2c, 0X0A, &chg_sts0);
	if (ret)
		pr_err("%s fail to read muic chg reg(%d)\n", __func__, ret);

	chg_sts0 = chg_sts0 & CHGIN_STATUS_MASK;
	val_vbadc = val_vbadc & STATUS_VBADC_MASK;
	pr_err("%s(%d) vbadc : %d, attached_dev : %d, chg in: %02x\n", __func__, __LINE__,
		val_vbadc, muic_pdata->attached_dev, chg_sts0);

	if ((muic_pdata->attached_dev != ATTACHED_DEV_AFC_CHARGER_9V_MUIC)
		&& (!(chg_sts0 == 0x0))
		&& (val_vbadc == VBADC_8_7V_9_3V)) {
		mutex_lock(&muic_data->muic_mutex);
		s2mu004_hv_muic_detect_dev(muic_data, muic_data->irq_vbadc);
		mutex_unlock(&muic_data->muic_mutex);
	}

}

void s2mu004_muic_afc_cable_type_work(struct work_struct *work)
{
	struct s2mu004_muic_data *muic_data =
		container_of(work, struct s2mu004_muic_data, afc_cable_type_work.work);
	int ret;
	u8 val_vbadc, chg_sts0;
	struct muic_platform_data *muic_pdata = muic_data->pdata;

	ret = s2mu004_read_reg(muic_data->i2c, S2MU004_REG_AFC_STATUS, &val_vbadc);
	if (ret)
		pr_err("%s fail to read muic reg(%d)\n", __func__, ret);

	ret = s2mu004_read_reg(muic_data->i2c, 0X0A, &chg_sts0);
	if (ret)
		pr_err("%s fail to read muic chg reg(%d)\n", __func__, ret);

	chg_sts0 = chg_sts0 & CHGIN_STATUS_MASK;
	val_vbadc = val_vbadc & STATUS_VBADC_MASK;
	pr_err("%s(%d) vbadc : %d, attached_dev : %d, chg in: %02x\n", __func__, __LINE__,
		val_vbadc, muic_pdata->attached_dev, chg_sts0);

	if (muic_pdata->attached_dev != ATTACHED_DEV_NONE_MUIC
		&& (val_vbadc != VBADC_8_7V_9_3V)
		&& (!(chg_sts0 == 0x0))) {
		mutex_lock(&muic_data->muic_mutex);
		MUIC_SEND_NOTI_ATTACH(ATTACHED_DEV_AFC_CHARGER_5V_MUIC);
		mutex_unlock(&muic_data->muic_mutex);
	}

}

void s2mu004_muic_afc_send_mpnack(struct work_struct *work)
{
	struct s2mu004_muic_data *muic_data =
		container_of(work, struct s2mu004_muic_data, afc_send_mpnack.work);
	pr_info("%s\n ", __func__);
	mutex_lock(&muic_data->afc_mutex);
	s2mu004_hv_muic_detect_dev(muic_data, muic_data->irq_mpnack);
	mutex_unlock(&muic_data->afc_mutex);
}

/* TA setting in s2mu004-muic.c */
void s2mu004_muic_prepare_afc_charger(struct s2mu004_muic_data *muic_data)
{
	struct i2c_client *i2c = muic_data->i2c;

	pr_info("%s\n", __func__);
	msleep(200);
	s2mu004_write_reg(i2c, 0x49, 0xa0);

	/* Set TX DATA */
	muic_data->tx_data = (HVTXBYTE_9V << 4) | HVTXBYTE_1_65A;
	s2mu004_write_reg(i2c, S2MU004_REG_TX_BYTE1, 0x46);
	s2mu004_write_reg(i2c, 0x49, 0xa1);
	s2mu004_write_reg(i2c, 0x4a, 0x06);
	s2mu004_muic_set_afc_ready(muic_data, true);
}

/* TA setting in s2mu004-muic.c */
bool s2mu004_muic_check_change_dev_afc_charger(struct s2mu004_muic_data *muic_data,
	muic_attached_dev_t new_dev)
{
	bool ret = true;

	if (new_dev == ATTACHED_DEV_TA_MUIC ||
		new_dev == ATTACHED_DEV_AFC_CHARGER_PREPARE_MUIC ||
		new_dev == ATTACHED_DEV_AFC_CHARGER_PREPARE_DUPLI_MUIC ||
		new_dev == ATTACHED_DEV_AFC_CHARGER_5V_MUIC ||
		new_dev == ATTACHED_DEV_AFC_CHARGER_5V_DUPLI_MUIC ||
		new_dev == ATTACHED_DEV_AFC_CHARGER_9V_MUIC ||
		new_dev == ATTACHED_DEV_AFC_CHARGER_ERR_V_MUIC ||
		new_dev == ATTACHED_DEV_AFC_CHARGER_ERR_V_DUPLI_MUIC){
		if (muic_check_dev_ta(muic_data))
			ret = false;
	}

	return ret;
}

static void s2mu004_hv_muic_detect_after_charger_init(struct work_struct *work)
{
	struct afc_init_data_s *init_data =
	    container_of(work, struct afc_init_data_s, muic_afc_init_work);
	struct s2mu004_muic_data *muic_data = init_data->muic_data;
	struct muic_platform_data *muic_pdata = muic_data->pdata;

	pr_info("%s\n", __func__);

	mutex_lock(&muic_data->muic_mutex);

	if ((muic_pdata->attached_dev == ATTACHED_DEV_AFC_CHARGER_PREPARE_MUIC)
		|| (muic_pdata->attached_dev == ATTACHED_DEV_AFC_CHARGER_5V_MUIC))
	{
		s2mu004_hv_muic_set_afc_after_prepare(muic_data);
	}

	mutex_unlock(&muic_data->muic_mutex);
}

#if defined(CONFIG_MUIC_HV)
void hv_muic_change_afc_voltage(int tx_data)
{
	struct i2c_client *i2c = afc_init_data.muic_data->i2c;
	struct s2mu004_muic_data *muic_data = afc_init_data.muic_data;
	u8 value;
	struct muic_platform_data *muic_pdata = muic_data->pdata;

	pr_info("%s %x\n", __func__, tx_data);

	/* QC */
	if (muic_pdata->attached_dev == ATTACHED_DEV_QC_CHARGER_9V_MUIC) {
		switch (tx_data) {
		case MUIC_HV_5V:
			s2mu004_hv_muic_write_reg(i2c, 0x49, 0xa1);
			break;
		case MUIC_HV_9V:
			s2mu004_hv_muic_write_reg(i2c, 0x49, 0xbd);
			break;
		default:
			break;
		}
	} else { /* AFC */
		muic_data->afc_count = 0;
		s2mu004_read_reg(i2c, S2MU004_REG_TX_BYTE1, &value);
		if (value == tx_data) {
			pr_info("%s: same to current voltage %x\n", __func__, value);
			return;
		}

		s2mu004_write_reg(i2c, S2MU004_REG_TX_BYTE1, tx_data);
		s2mu004_hv_muic_afc_control_ping(muic_data, true);
	}
}

int muic_afc_set_voltage(int vol)
{
	if (vol == 5) {
		hv_muic_change_afc_voltage(MUIC_HV_5V);
	} else if (vol == 9) {
		hv_muic_change_afc_voltage(MUIC_HV_9V);
	} else {
		pr_warn("%s invalid value\n", __func__);
		return 0;
	}

	return 1;
}
#endif /* CONFIG_MUIC_HV */

void s2mu004_hv_muic_charger_init(void)
{
	pr_info("%s\n", __func__);

	is_charger_ready = true;

	if (afc_init_data.muic_data) {
		afc_init_data.muic_data->is_charger_ready = true;
		schedule_work(&afc_init_data.muic_afc_init_work);
	}
}

void s2mu004_hv_muic_init_detect(struct s2mu004_muic_data *muic_data)
{

	pr_info("%s\n", __func__);
	/* TBD */
#if 0
	mutex_lock(&muic_data->muic_mutex);

	mutex_unlock(&muic_data->muic_mutex);
#endif
}
static irqreturn_t s2mu004_muic_hv_irq(int irq, void *data)
{
	struct s2mu004_muic_data *muic_data = data;
	int ret;
	u8 val_vbadc, vbvolt, read_val;

	mutex_lock(&muic_data->muic_mutex);
	if (muic_data->is_afc_muic_ready == false)
		pr_info("%s not ready yet(afc_muic_ready[%c])\n",
			 __func__, (muic_data->is_afc_muic_ready ? 'T' : 'F'));
	else if (muic_data->is_charger_ready == false && irq != muic_data->irq_vdnmon)
		pr_info("%s not ready yet(charger_ready[%c])\n",
			__func__, (muic_data->is_charger_ready ? 'T' : 'F'));
	else if (muic_data->pdata->afc_disable)
		pr_info("%s AFC disable by USER (afc_disable[%c]\n",
			__func__, (muic_data->pdata->afc_disable ? 'T' : 'F'));
	else {
		muic_data->afc_irq = irq;

		/* Read Vbus */
		read_val = s2mu004_i2c_read_byte(muic_data->i2c, S2MU004_REG_MUIC_DEVICE_APPLE);
		vbvolt = !!(read_val & DEV_TYPE_APPLE_VBUS_WAKEUP);
		pr_err("%s: vbvolt: %d\n", __func__, vbvolt);

		if (!vbvolt) {
			pr_err("%s: No vbvolt, just return!\n", __func__);

			mutex_unlock(&muic_data->muic_mutex);

			return IRQ_HANDLED;
		}

		/* Re-check vbadc voltage, if vbadc 9v interrupt does not occur when send pings. */
		if (irq == muic_data->irq_vbadc && muic_data->afc_count >= 2) {
			pr_err("%s: VbADC interrupt after sending master ping x3\n", __func__);
			ret = s2mu004_read_reg(muic_data->i2c, S2MU004_REG_AFC_STATUS, &val_vbadc);
			if (ret)
				pr_err("%s fail to read muic reg(%d)\n", __func__, ret);

			val_vbadc = val_vbadc & STATUS_VBADC_MASK;
			if (val_vbadc != VBADC_8_7V_9_3V) {
				pr_err("%s: vbadc: %d\n", __func__, val_vbadc);
				schedule_delayed_work(&muic_data->afc_check_vbadc, msecs_to_jiffies(100));
			}
		}
		/* After ping , if there is response then cancle mpnack work */
		if ((irq == muic_data->irq_mrxrdy) || (irq == muic_data->irq_mpnack)) {
			cancel_delayed_work(&muic_data->afc_send_mpnack);
			cancel_delayed_work(&muic_data->afc_control_ping_retry);
		}

		if ((irq == muic_data->irq_vbadc) && (muic_data->qc_prepare == 1)) {
			muic_data->qc_prepare = 0;
			cancel_delayed_work(&muic_data->afc_qc_retry);
		}

		s2mu004_hv_muic_detect_dev(muic_data, muic_data->afc_irq);

	}

	mutex_unlock(&muic_data->muic_mutex);

	return IRQ_HANDLED;
}

#define REQUEST_HV_IRQ(_irq, _dev_id, _name)				\
do {									\
	ret = request_threaded_irq(_irq, NULL, s2mu004_muic_hv_irq,	\
				IRQF_NO_SUSPEND, _name, _dev_id);	\
	if (ret < 0) {							\
		pr_err("%s Failed to request IRQ #%d: %d\n",		\
				__func__, _irq, ret);	\
		_irq = 0;						\
	}								\
} while (0)

int s2mu004_afc_muic_irq_init(struct s2mu004_muic_data *muic_data)
{
	int ret = 0;

	pr_info("%s\n", __func__);

	if (muic_data->mfd_pdata && (muic_data->mfd_pdata->irq_base > 0)) {
		int irq_base = muic_data->mfd_pdata->irq_base;

		/* request AFC MUIC IRQ */
		muic_data->irq_vdnmon = irq_base + S2MU004_AFC_IRQ_VDNMon;
		REQUEST_HV_IRQ(muic_data->irq_vdnmon, muic_data, "muic-vdnmon");
		muic_data->irq_mrxrdy = irq_base + S2MU004_AFC_IRQ_MRxRdy;
		REQUEST_HV_IRQ(muic_data->irq_mrxrdy, muic_data, "muic-mrxrdy");
		muic_data->irq_vbadc = irq_base + S2MU004_AFC_IRQ_VbADC;
		REQUEST_HV_IRQ(muic_data->irq_vbadc, muic_data, "muic-vbadc");

		muic_data->irq_mpnack = irq_base + S2MU004_AFC_IRQ_MPNack;
		REQUEST_HV_IRQ(muic_data->irq_mpnack, muic_data, "muic-mpnack");

		pr_info("mrxrdy(%d), mpnack(%d), vbadc(%d), vdnmon(%d)\n",
				muic_data->irq_dnres, muic_data->irq_mrxrdy,
				muic_data->irq_vbadc, muic_data->irq_vdnmon);
	}

	return ret;
}

#define FREE_HV_IRQ(_irq, _dev_id, _name)	\
do {									\
	if (_irq) {							\
		free_irq(_irq, _dev_id);				\
		pr_info("%s IRQ(%d):%s free done\n",	\
				__func__, _irq, _name);			\
	}								\
} while (0)

void s2mu004_hv_muic_free_irqs(struct s2mu004_muic_data *muic_data)
{
	pr_info("%s\n", __func__);

	/* free MUIC IRQ */
	FREE_HV_IRQ(muic_data->irq_vdnmon, muic_data, "muic-vdnmon");
	FREE_HV_IRQ(muic_data->irq_mrxrdy, muic_data, "muic-mrxrdy");
	FREE_HV_IRQ(muic_data->irq_mpnack, muic_data, "muic-mpnack");
	FREE_HV_IRQ(muic_data->irq_vbadc, muic_data, "muic-vbadc");
}

void s2mu004_hv_muic_initialize(struct s2mu004_muic_data *muic_data)
{
	pr_info("%s\n", __func__);

	muic_data->is_afc_handshaking = false;
	muic_data->is_afc_muic_prepare = false;
	muic_data->is_charger_ready = true;
	muic_data->pdata->afc_disable = false;

	s2mu004_write_reg(muic_data->i2c, 0xd8, 0x84); /* OTP */
	s2mu004_write_reg(muic_data->i2c, 0x2c, 0x55); /* OTP */
	s2mu004_write_reg(muic_data->i2c, 0xc3, 0x88); /* OTP */

	s2mu004_hv_muic_write_reg(muic_data->i2c, 0x4b, 0x00);
	s2mu004_hv_muic_write_reg(muic_data->i2c, 0x49, 0x00);
	s2mu004_hv_muic_write_reg(muic_data->i2c, 0x4a, 0x00);
	s2mu004_hv_muic_write_reg(muic_data->i2c, 0x5f, 0x01);

	afc_init_data.muic_data = muic_data;
	INIT_WORK(&afc_init_data.muic_afc_init_work, s2mu004_hv_muic_detect_after_charger_init);

	INIT_DELAYED_WORK(&muic_data->afc_check_vbadc, s2mu004_muic_afc_check_vbadc);
	INIT_DELAYED_WORK(&muic_data->afc_cable_type_work, s2mu004_muic_afc_cable_type_work);

	INIT_DELAYED_WORK(&muic_data->afc_send_mpnack, s2mu004_muic_afc_send_mpnack);
	INIT_DELAYED_WORK(&muic_data->afc_control_ping_retry, s2mu004_muic_afc_control_ping_retry);
	INIT_DELAYED_WORK(&muic_data->afc_qc_retry, s2mu004_muic_afc_qc_retry);
	INIT_DELAYED_WORK(&muic_data->afc_after_prepare, s2mu004_muic_afc_after_prepare);

	mutex_init(&muic_data->afc_mutex);
}

void s2mu004_hv_muic_remove(struct s2mu004_muic_data *muic_data)
{
	pr_info("%s\n", __func__);

	/* Set digital IVR when power off under revision EVT3 */
	if (muic_data->ic_rev_id < 3)
		s2mu004_muic_hv_update_reg(muic_data->i2c, 0xB3, 0x00, 0x08, 0);

	cancel_work_sync(&afc_init_data.muic_afc_init_work);
	s2mu004_hv_muic_free_irqs(muic_data);

	/* Afc reset */
	s2mu004_muic_hv_update_reg(muic_data->i2c, 0x5f, 0x80, 0x80, 0);
}
#if 0
void s2mu004_hv_muic_remove_wo_free_irq(struct s2mu004_muic_data *muic_data)
{
	pr_info("%s\n", __func__);
	cancel_work_sync(&afc_init_data.muic_afc_init_work);
}
#endif
