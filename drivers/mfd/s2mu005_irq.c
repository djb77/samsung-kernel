/*
 * s2mu005-irq.c - Interrupt controller support for s2mu005
 *
 * Copyright (C) 2015 Samsung Electronics Co.Ltd
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * This driver is based on s2mu005-irq.c
 */

#include <linux/err.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/mfd/samsung/s2mu005.h>
#include <linux/mfd/samsung/s2mu005-private.h>
//#include <plat/gpio-cfg.h>

static const u8 s2mu005_mask_reg[] = {
	/* TODO: Need to check other INTMASK */
	[CHG_INT] = S2MU005_REG_SC_INT_MASK,
	[FLED_INT] = S2MU005_REG_FLED_INT_MASK,
	[MUIC_INT1] = S2MU005_REG_MUIC_INT1_MASK,
	[MUIC_INT2] = S2MU005_REG_MUIC_INT2_MASK,
};

struct s2mu005_irq_data {
	int mask;
	enum s2mu005_irq_source group;
};

#define DECLARE_IRQ(idx, _group, _mask)		\
	[(idx)] = { .group = (_group), .mask = (_mask) }
static const struct s2mu005_irq_data s2mu005_irqs[] = {
	DECLARE_IRQ(S2MU005_CHG_IRQ_DET_BAT,	CHG_INT, 1 << 0),
	DECLARE_IRQ(S2MU005_CHG_IRQ_BAT,	CHG_INT, 1 << 1),
	DECLARE_IRQ(S2MU005_CHG_IRQ_IVR,	CHG_INT, 1 << 2),
	DECLARE_IRQ(S2MU005_CHG_IRQ_EVENT,	CHG_INT, 1 << 3),
	DECLARE_IRQ(S2MU005_CHG_IRQ_CHG,	CHG_INT, 1 << 4),
	DECLARE_IRQ(S2MU005_CHG_IRQ_VMID,	CHG_INT, 1 << 5),
	DECLARE_IRQ(S2MU005_CHG_IRQ_WCIN,	CHG_INT, 1 << 6),
	DECLARE_IRQ(S2MU005_CHG_IRQ_VBUS,	CHG_INT, 1 << 7),

	DECLARE_IRQ(S2MU005_FLED_IRQ_LBPROT,	FLED_INT, 1 << 2),
	DECLARE_IRQ(S2MU005_FLED_IRQ_OPEN_CH2,	FLED_INT, 1 << 4),
	DECLARE_IRQ(S2MU005_FLED_IRQ_OPEN_CH1,	FLED_INT, 1 << 5),
	DECLARE_IRQ(S2MU005_FLED_IRQ_SHORT_CH2,	FLED_INT, 1 << 6),
	DECLARE_IRQ(S2MU005_FLED_IRQ_SHORT_CH1,	FLED_INT, 1 << 7),

	DECLARE_IRQ(S2MU005_MUIC_IRQ1_ATTATCH,	MUIC_INT1, 1 << 0),
	DECLARE_IRQ(S2MU005_MUIC_IRQ1_DETACH,	MUIC_INT1, 1 << 1),
	DECLARE_IRQ(S2MU005_MUIC_IRQ1_KP,	MUIC_INT1, 1 << 2),
	DECLARE_IRQ(S2MU005_MUIC_IRQ1_LKP,	MUIC_INT1, 1 << 3),
	DECLARE_IRQ(S2MU005_MUIC_IRQ1_LKR,	MUIC_INT1, 1 << 4),
	DECLARE_IRQ(S2MU005_MUIC_IRQ1_RID_CHG,	MUIC_INT1, 1 << 5),

	DECLARE_IRQ(S2MU005_MUIC_IRQ2_VBUS_ON,	MUIC_INT2, 1 << 0),
	DECLARE_IRQ(S2MU005_MUIC_IRQ2_RSVD_ATTACH,	MUIC_INT2, 1 << 1),
	DECLARE_IRQ(S2MU005_MUIC_IRQ2_ADC_CHANGE,	MUIC_INT2, 1 << 2),
	DECLARE_IRQ(S2MU005_MUIC_IRQ2_STUCK,		MUIC_INT2, 1 << 3),
	DECLARE_IRQ(S2MU005_MUIC_IRQ2_STUCKRCV,		MUIC_INT2, 1 << 4),
	DECLARE_IRQ(S2MU005_MUIC_IRQ2_MHDL,		MUIC_INT2, 1 << 5),
	DECLARE_IRQ(S2MU005_MUIC_IRQ2_AV_CHARGE,	MUIC_INT2, 1 << 6),
	DECLARE_IRQ(S2MU005_MUIC_IRQ2_VBUS_OFF,		MUIC_INT2, 1 << 7),
};

static void s2mu005_irq_lock(struct irq_data *data)
{
	struct s2mu005_dev *s2mu005 = irq_get_chip_data(data->irq);

	mutex_lock(&s2mu005->irqlock);
}

static void s2mu005_irq_sync_unlock(struct irq_data *data)
{
	struct s2mu005_dev *s2mu005 = irq_get_chip_data(data->irq);
	int i;

	for (i = 0; i < S2MU005_IRQ_GROUP_NR; i++) {
		u8 mask_reg = s2mu005_mask_reg[i];
		struct i2c_client *i2c = s2mu005->i2c;

		if (mask_reg == S2MU005_REG_INVALID ||
				IS_ERR_OR_NULL(i2c))
			continue;
		s2mu005->irq_masks_cache[i] = s2mu005->irq_masks_cur[i];

		s2mu005_write_reg(i2c, s2mu005_mask_reg[i],
				s2mu005->irq_masks_cur[i]);
	}

	mutex_unlock(&s2mu005->irqlock);
}

static const inline struct s2mu005_irq_data *
irq_to_s2mu005_irq(struct s2mu005_dev *s2mu005, int irq)
{
	return &s2mu005_irqs[irq - s2mu005->irq_base];
}

static void s2mu005_irq_mask(struct irq_data *data)
{
	struct s2mu005_dev *s2mu005 = irq_get_chip_data(data->irq);
	const struct s2mu005_irq_data *irq_data =
	    irq_to_s2mu005_irq(s2mu005, data->irq);

	if (irq_data->group >= S2MU005_IRQ_GROUP_NR)
		return;

	s2mu005->irq_masks_cur[irq_data->group] |= irq_data->mask;
}

static void s2mu005_irq_unmask(struct irq_data *data)
{
	struct s2mu005_dev *s2mu005 = irq_get_chip_data(data->irq);
	const struct s2mu005_irq_data *irq_data =
	    irq_to_s2mu005_irq(s2mu005, data->irq);

	if (irq_data->group >= S2MU005_IRQ_GROUP_NR)
		return;

	s2mu005->irq_masks_cur[irq_data->group] &= ~irq_data->mask;
}

static struct irq_chip s2mu005_irq_chip = {
	.name			= MFD_DEV_NAME,
	.irq_bus_lock	= s2mu005_irq_lock,
	.irq_bus_sync_unlock	= s2mu005_irq_sync_unlock,
	.irq_mask		= s2mu005_irq_mask,
	.irq_unmask		= s2mu005_irq_unmask,
};

static irqreturn_t s2mu005_irq_thread(int irq, void *data)
{
	struct s2mu005_dev *s2mu005 = data;
	u8 irq_reg[S2MU005_IRQ_GROUP_NR] = {0};
//	u8 tmp_irq_reg[S2MU005_IRQ_GROUP_NR] = {};
	int i, ret;
	u8 temp, temp_2;

	pr_info("%s: irq gpio pre-state(0x%02x)\n", __func__,
				gpio_get_value(s2mu005->irq_gpio));

	/* CHG_INT */
	ret = s2mu005_read_reg(s2mu005->i2c, S2MU005_REG_SC_INT,
				&irq_reg[CHG_INT]);
	pr_info("%s: charger interrupt(0x%02x)\n",
			__func__, irq_reg[CHG_INT]);

	/* FLED_INT */
	ret = s2mu005_read_reg(s2mu005->i2c, S2MU005_REG_FLED_INT,
				&irq_reg[FLED_INT]);
	pr_info("%s: fled interrupt(0x%02x)\n", __func__, irq_reg[FLED_INT]);

	/* MUIC INT1 ~ INT2 */
	ret = s2mu005_bulk_read(s2mu005->i2c, S2MU005_REG_MUIC_INT1,
				S2MU005_NUM_IRQ_MUIC_REGS, &irq_reg[MUIC_INT1]);
	pr_info("%s: muic interrupt(0x%02x, 0x%02x)\n", __func__,
			irq_reg[MUIC_INT1], irq_reg[MUIC_INT2]);

	if(s2mu005->pmic_rev == 0) {
		s2mu005_read_reg(s2mu005->i2c, S2MU005_REG_MUIC_ADC, &temp);
		temp &= 0x1F;
		s2mu005_read_reg(s2mu005->i2c, 0x51, &temp_2); /* checking VBUS_WAKEUP bit of R(0x51) */
		if((temp_2 & 0x02) && (temp != 0x18)
			&& (temp != 0x19) && (temp != 0x1C) && (temp != 0x1D) )
			s2mu005_update_reg(s2mu005->i2c, 0x89, 0x01, 0x03);
		if(irq_reg[MUIC_INT2] & 0x80)
			s2mu005_update_reg(s2mu005->i2c, 0x89, 0x03, 0x03);
	}
	/* For OTGGTEST : VMID_INT */
	if (irq_reg[CHG_INT] == 0x20)
	{
		pr_info("%s: VMID_INT\n", __func__);
		irq_reg[MUIC_INT2] |= 0x01;
	}

	/* Apply masking */
	for (i = 0; i < S2MU005_IRQ_GROUP_NR; i++) {
		irq_reg[i] &= ~s2mu005->irq_masks_cur[i];
	}

	/* Report */
	for (i = 0; i < S2MU005_IRQ_NR; i++) {
		if (irq_reg[s2mu005_irqs[i].group] & s2mu005_irqs[i].mask)
			handle_nested_irq(s2mu005->irq_base + i);
	}

	pr_info("%s: s2mu005_irq_thread successfully returned\n", __func__);

	return IRQ_HANDLED;
}

int s2mu005_irq_init(struct s2mu005_dev *s2mu005)
{
	int i;
	int ret;
	struct i2c_client *i2c = s2mu005->i2c;
	struct pinctrl *pinctrl;
	struct pinctrl_state *set_state_i2c;

	if (!s2mu005->irq_gpio) {
		dev_warn(s2mu005->dev, "No interrupt specified.\n");
		s2mu005->irq_base = 0;
		return 0;
	}

	if (!s2mu005->irq_base) {
		dev_err(s2mu005->dev, "No interrupt base specified.\n");
		return 0;
	}

	mutex_init(&s2mu005->irqlock);

	s2mu005->irq = gpio_to_irq(s2mu005->irq_gpio);
	pr_info("%s:%s irq=%d, irq->gpio=%d\n", MFD_DEV_NAME, __func__,
			s2mu005->irq, s2mu005->irq_gpio);

	ret = gpio_request(s2mu005->irq_gpio, "if_pmic_irq");
	if (ret) {
		dev_err(s2mu005->dev, "%s: failed requesting gpio %d\n",
			__func__, s2mu005->irq_gpio);
		return ret;
	}
	gpio_direction_input(s2mu005->irq_gpio);
	gpio_free(s2mu005->irq_gpio);

	/* Get pinctrl if target uses pinctrl */
	printk("[s2mu005] pinctrl!\n");
	pinctrl = devm_pinctrl_get(&s2mu005->i2c->dev);

	if (pinctrl) {
		set_state_i2c = pinctrl_lookup_state(pinctrl, "s2mu005_interrupt_pins_default");

		ret = pinctrl_select_state(pinctrl, set_state_i2c);
		if (ret)
			pr_err("%s: cannot set pinctrl state\n", __func__);
	}else{
		printk("[S2MU005] pinctrl error\n");
	}

	/* Mask individual interrupt sources */
	for (i = 0; i < S2MU005_IRQ_GROUP_NR; i++) {

		s2mu005->irq_masks_cur[i] = 0xff;
		s2mu005->irq_masks_cache[i] = 0xff;

		if (IS_ERR_OR_NULL(i2c))
			continue;
		if (s2mu005_mask_reg[i] == S2MU005_REG_INVALID)
			continue;
		s2mu005_write_reg(i2c, s2mu005_mask_reg[i], 0xff);
	}

	/* Register with genirq */
	for (i = 0; i < S2MU005_IRQ_NR; i++) {
		int cur_irq;
		cur_irq = i + s2mu005->irq_base;
		irq_set_chip_data(cur_irq, s2mu005);
		irq_set_chip_and_handler(cur_irq, &s2mu005_irq_chip,
					 handle_level_irq);
		irq_set_nested_thread(cur_irq, 1);
#ifdef CONFIG_ARM
		set_irq_flags(cur_irq, IRQF_VALID);
#else
		irq_set_noprobe(cur_irq);
#endif
	}

	ret = request_threaded_irq(s2mu005->irq, NULL, s2mu005_irq_thread,
				   IRQF_TRIGGER_LOW | IRQF_ONESHOT,
				   "s2mu005-irq", s2mu005);
	if (ret) {
		dev_err(s2mu005->dev, "Failed to request IRQ %d: %d\n",
			s2mu005->irq, ret);
		return ret;
	}

	return 0;
}

void s2mu005_irq_exit(struct s2mu005_dev *s2mu005)
{
	if (s2mu005->irq)
		free_irq(s2mu005->irq, s2mu005);
}
