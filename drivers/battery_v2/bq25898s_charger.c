/*
 *  bq25898s_charger.c
 *  Samsung bq25898s Charger Driver
 *
 *  Copyright (C) 2015 Samsung Electronics
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#define DEBUG

#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/power_supply.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include "include/charger/bq25898s_charger.h"

#define ENABLE 1
#define DISABLE 0

static enum power_supply_property bq25898s_charger_props[] = {
};

int bq25898s_read_reg(struct i2c_client *i2c, u8 reg, u8 *dest)
{
	struct bq25898s_charger *bq25898s = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&bq25898s->i2c_lock);
	ret = i2c_smbus_read_byte_data(i2c, reg);
	mutex_unlock(&bq25898s->i2c_lock);
	if (ret < 0) {
		pr_info("%s reg(0x%x), ret(%d)\n", __func__, reg, ret);
		return ret;
	}

	ret &= 0xff;
	*dest = ret;
	return 0;
}

int bq25898s_write_reg(struct i2c_client *i2c, u8 reg, u8 value)
{
	struct bq25898s_charger *bq25898s = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&bq25898s->i2c_lock);
	ret = i2c_smbus_write_byte_data(i2c, reg, value);
	mutex_unlock(&bq25898s->i2c_lock);
	if (ret < 0)
		pr_info("%s reg(0x%x), ret(%d)\n",
				__func__, reg, ret);

	return ret;
}

int bq25898s_update_reg(struct i2c_client *i2c, u8 reg, u8 val, u8 mask)
{
	struct bq25898s_charger *bq25898s = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&bq25898s->i2c_lock);
	ret = i2c_smbus_read_byte_data(i2c, reg);
	if (ret >= 0) {
		u8 old_val = ret & 0xff;
		u8 new_val = (val & mask) | (old_val & (~mask));
		ret = i2c_smbus_write_byte_data(i2c, reg, new_val);
	}
	mutex_unlock(&bq25898s->i2c_lock);
	return ret;
}

static void bq25898s_test_read(struct bq25898s_charger *charger)
{
	u8 reg;
	u8 reg_data;
	char str[1024]={0,};

	for (reg = 0x00; reg <= 0x14; reg++) {
		bq25898s_read_reg(charger->i2c, reg, &reg_data);
		sprintf(str + strlen(str), "0x%02x:0x%02x,", reg, reg_data);
	}
	pr_info("%s : %s\n", __func__, str);

}

static int bq25898s_get_charge_current(struct bq25898s_charger *charger)
{
	u8 data;
	int charge_current;

	bq25898s_read_reg(charger->i2c, BQ25898S_CHG_REG_04, &data);
	charge_current = (data & 0x3F) * 64;
	pr_info("%s : DATA(0x%02x), current(%d)\n", __func__, data, charge_current);
	return charge_current;
}

static int bq25898s_get_float_voltage(struct bq25898s_charger *charger)
{
	u8 data;
	int max_voltage;

	bq25898s_read_reg(charger->i2c, BQ25898S_CHG_REG_06, &data);
	max_voltage = (data >> 2) * 16 * 10 + 38400;
	pr_info("%s : DATA(0x%02x) VOLTAGE(%d)\n", __func__, data, max_voltage);

	return max_voltage;
}

static int bq25898s_get_input_current(struct bq25898s_charger *charger)
{
	u8 data;
	int input_current;

	bq25898s_read_reg(charger->i2c, BQ25898S_CHG_REG_00, &data);
	input_current = (data & 0x3F) * 50 + 100;
	pr_info("%s : DATA(0x%02x), current(%d)\n", __func__, data, input_current);

	return input_current;
}

static void bq25898s_set_charge_current(struct bq25898s_charger *charger, int charging_current)
{
	u8 data;

	data = charging_current / 64;
	pr_info("%s: charging_current(%d), 0x%x \n", __func__, charging_current, data);

	bq25898s_update_reg(charger->i2c, BQ25898S_CHG_REG_04,
			data, BQ25898S_CHG_ICHG_MASK);
}

static void bq25898s_set_input_current(struct bq25898s_charger *charger, int input_current)
{
	u8 data;

	if (input_current < 100)
		data = 0;
	else
		data = (input_current - 100) / 50;

	pr_info ("%s : SET INPUT CURRENT(%d), 0x%x\n", __func__, input_current, data);

	bq25898s_update_reg(charger->i2c, BQ25898S_CHG_REG_00,
			data, BQ25898S_CHG_IINLIM_MASK);
}

static void bq25898s_watchdog_reset(struct bq25898s_charger *charger)
{
	u8 data;

	bq25898s_update_reg(charger->i2c, BQ25898S_CHG_REG_03, 0x40, 0x40);
	bq25898s_read_reg(charger->i2c, BQ25898S_CHG_REG_03, &data);
	pr_info("%s : BQ25898S_CHG_REG_03(0x%02x)\n", __func__, data);
}

static void bq25898s_set_watchdog_timer_en(struct bq25898s_charger *charger, int time)
{
	bq25898s_update_reg(charger->i2c, BQ25898S_CHG_REG_07,
			time << BQ25898S_CHG_WATCHDOG_SHIFT, BQ25898S_CHG_WATCHDOG_MASK);
}

static void bq25898s_set_float_voltage(struct bq25898s_charger *charger, int float_voltage)
{
	u8 data;

	data = ((float_voltage - 38400) / 10 / 16) << 2;
	pr_info("%s: voltage(%d), 0x%x \n", __func__, float_voltage, data);
	bq25898s_update_reg(charger->i2c, BQ25898S_CHG_REG_06,
			data, BQ25898S_CHG_VREG_MASK);
}

static void bq25898s_set_charger_state(struct bq25898s_charger *charger,
	int enable)
{
	pr_info("%s: CHARGE_EN(%s)\n",__func__, enable > 0 ? "ENABLE" : "DISABLE");

	bq25898s_update_reg(charger->i2c, BQ25898S_CHG_REG_03,
			(enable << BQ25898S_CHG_CONFIG_SHIFT), BQ25898S_CHG_CONFIG_MASK);
	bq25898s_set_watchdog_timer_en(charger, enable? WATCHDOG_TIMER_80S: WATCHDOG_TIMER_DISABLE);
	bq25898s_test_read(charger);
}

static void bq25898s_set_topoff_current(struct bq25898s_charger *charger, int eoc)
{
	u8 data;

	data = (eoc - 64) / 64;
	pr_info("%s: eoc(%d), 0x%x \n", __func__, eoc, data);
	bq25898s_update_reg(charger->i2c, BQ25898S_CHG_REG_05,
		data, BQ25898S_CHG_ITERM_MASK);
}

static void bq25898s_charger_initialize(struct bq25898s_charger *charger)
{
	bq25898s_set_charger_state(charger, DISABLE);
	bq25898s_set_input_current(charger, 500);
	bq25898s_set_charge_current(charger, 500);
	/* Disable AUTO_DPDM_EN */
	bq25898s_update_reg(charger->i2c, BQ25898S_CHG_REG_02, 0x0 << 0, 0x1 << 0);
	/* termination current */
	bq25898s_set_topoff_current(charger, charger->full_check_current);
	/* set flolat voltage */
	bq25898s_set_float_voltage(charger, charger->float_voltage);
	bq25898s_test_read(charger);
}

static irqreturn_t bq25898s_irq_handler(int irq, void *data)
{
	struct bq25898s_charger *charger = data;
	u8 val;

	bq25898s_read_reg(charger->i2c, BQ25898S_CHG_REG_0C, &val);
	dev_info(charger->dev,
			"%s: 0x%x\n", __func__, val);

	return IRQ_HANDLED;
}

static int bq25898s_chg_get_property(struct power_supply *psy,
		enum power_supply_property psp,
		union power_supply_propval *val)
{
	struct bq25898s_charger *charger = container_of(psy, struct bq25898s_charger, psy_chg);
	enum power_supply_ext_property ext_psp = psp;

	val->intval = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = charger->cable_type;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
	case POWER_SUPPLY_PROP_STATUS:
		break;
	case POWER_SUPPLY_PROP_CHARGE_TYPE:
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		bq25898s_watchdog_reset(charger);
		bq25898s_test_read(charger);
		val->intval = POWER_SUPPLY_HEALTH_GOOD;
		break;
	case POWER_SUPPLY_PROP_CURRENT_MAX:
		val->intval = bq25898s_get_input_current(charger);
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:	
		val->intval = bq25898s_get_charge_current(charger);
		break;	
	case POWER_SUPPLY_PROP_CURRENT_AVG:
	case POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN:
		break;
#if defined(CONFIG_BATTERY_SWELLING)
	case POWER_SUPPLY_PROP_VOLTAGE_MAX:
		val->intval = bq25898s_get_float_voltage(charger);
		break;
#endif
	case POWER_SUPPLY_PROP_USB_HC:
		return -ENODATA;
	case POWER_SUPPLY_PROP_CHARGE_NOW:
		break;
	case POWER_SUPPLY_PROP_MAX ... POWER_SUPPLY_EXT_PROP_MAX:
		switch (ext_psp) {
			case POWER_SUPPLY_EXT_PROP_CHECK_SLAVE_I2C:
			{
				u8 reg_data;
				bq25898s_read_reg(charger->i2c, BQ25898S_CHG_REG_11, &reg_data);
				if((reg_data > 0x93) && (reg_data < 0x9D)) // 4.5V ~ 5.5V
					val->intval = 1;
				else
					val->intval = 0;
				pr_info("%s: reg_data : 0x%02X\n", __func__,reg_data);
			}
			break;
		case POWER_SUPPLY_EXT_PROP_CHECK_MULTI_CHARGE:
			val->intval = charger->is_charging ?
				POWER_SUPPLY_STATUS_CHARGING : POWER_SUPPLY_STATUS_DISCHARGING;
			break;
		default:
			return -EINVAL;
		}
	break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int bq25898s_chg_set_property(struct power_supply *psy,
		enum power_supply_property psp,
		const union power_supply_propval *val)
{
	struct bq25898s_charger *charger = container_of(psy, struct bq25898s_charger, psy_chg);

	switch (psp) {
	/* val->intval : type */
	case POWER_SUPPLY_PROP_CHARGING_ENABLED:
		charger->is_charging =
			(val->intval == SEC_BAT_CHG_MODE_CHARGING) ? ENABLE : DISABLE;
		bq25898s_set_charger_state(charger, charger->is_charging);
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		charger->charging_current = val->intval;
		bq25898s_set_charge_current(charger, charger->charging_current);
		break;
	case POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN:
		charger->siop_level = val->intval;
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		charger->cable_type = val->intval;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX:
		if (val->intval / 1000 < 10) /* chg_float_voltage_conv = 1 */
			charger->float_voltage = val->intval * 10;
		else /* chg_float_voltage_conv = 10 */
			charger->float_voltage = val->intval;
		bq25898s_set_float_voltage(charger, charger->float_voltage);
		break;
	case POWER_SUPPLY_PROP_STATUS:
	case POWER_SUPPLY_PROP_CURRENT_FULL:
		break;
	case POWER_SUPPLY_PROP_CURRENT_MAX:
		charger->input_current = val->intval;
		bq25898s_set_input_current(charger, charger->input_current);
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		return -ENODATA;
	default:
		return -EINVAL;
	}
	return 0;
}

static ssize_t bq25898s_store_addr(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct bq25898s_charger *charger = container_of(psy, struct bq25898s_charger, psy_chg);
	int x;
	if (sscanf(buf, "0x%x\n", &x) == 1) {
		charger->addr = x;
	}
	return count;
}

static ssize_t bq25898s_show_addr(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct bq25898s_charger *charger = container_of(psy, struct bq25898s_charger, psy_chg);
	return sprintf(buf, "0x%x\n", charger->addr);
}

static ssize_t bq25898s_store_size(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct bq25898s_charger *charger = container_of(psy, struct bq25898s_charger, psy_chg);
	int x;
	if (sscanf(buf, "%d\n", &x) == 1) {
		charger->size = x;
	}
	return count;
}

static ssize_t bq25898s_show_size(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct bq25898s_charger *charger = container_of(psy, struct bq25898s_charger, psy_chg);
	return sprintf(buf, "0x%x\n", charger->size);
}

static ssize_t bq25898s_store_data(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct bq25898s_charger *charger = container_of(psy, struct bq25898s_charger, psy_chg);
	int x;

	if (sscanf(buf, "0x%x", &x) == 1) {
		u8 data = x;
		if (bq25898s_write_reg(charger->i2c, charger->addr, data) < 0)
		{
			dev_info(charger->dev,
					"%s: addr: 0x%x write fail\n", __func__, charger->addr);
		}
	}
	return count;
}

static ssize_t bq25898s_show_data(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct bq25898s_charger *charger = container_of(psy, struct bq25898s_charger, psy_chg);
	u8 data;
	int i, count = 0;;
	if (charger->size == 0)
		charger->size = 1;

	for (i = 0; i <= charger->size; i++) {
		if (bq25898s_read_reg(charger->i2c, charger->addr+i, &data) < 0) {
			dev_info(charger->dev,
					"%s: read fail\n", __func__);
			count += sprintf(buf+count, "addr: 0x%x read fail\n", charger->addr+i);
			continue;
		}
		count += sprintf(buf+count, "0x%02x : 0x%02x\n", charger->addr+i,data);
	}
	return count;
}

static DEVICE_ATTR(addr, 0644, bq25898s_show_addr, bq25898s_store_addr);
static DEVICE_ATTR(size, 0644, bq25898s_show_size, bq25898s_store_size);
static DEVICE_ATTR(data, 0644, bq25898s_show_data, bq25898s_store_data);

static struct attribute *bq25898s_attributes[] = {
	&dev_attr_addr.attr,
	&dev_attr_size.attr,
	&dev_attr_data.attr,
	NULL
};
static const struct attribute_group bq25898s_attr_group = {
	.attrs = bq25898s_attributes,
};

#ifdef CONFIG_OF
static int bq25898s_charger_parse_dt(struct bq25898s_charger *charger,
	struct bq25898s_charger_platform_data *pdata)
{
	struct device_node *np = of_find_node_by_name(NULL, "bq25898s-charger");
	int ret = 0;

	if (!np) {
		pr_err("%s: np is NULL\n", __func__);
		return -1;
	} else {
		ret = of_get_named_gpio_flags(np, "bq25898s-charger,irq-gpio",
			0, NULL);
		if (ret < 0) {
			pr_err("%s: bq25898s-charger,irq-gpio is empty\n", __func__);
			pdata->irq_gpio = 0;
		} else {
			pdata->irq_gpio = ret;
			pr_info("%s: irq-gpio = %d\n", __func__, pdata->irq_gpio);
		}

		ret = of_property_read_u32(np, "bq25898s-charger,chg_float_voltage",
					   &pdata->float_voltage);
		if (ret) {
			pr_info("%s: bq25898s-charger,chg_float_voltage is empty\n", __func__);
			charger->float_voltage  = 43000;
		} else
			charger->float_voltage = pdata->float_voltage;

		ret = of_property_read_u32(np, "bq25898s-charger,full_check_current",
					   &pdata->full_check_current);
		if (ret) {
			pr_info("%s: bq25898s-charger,full_check_current is empty\n", __func__);
			charger->full_check_current  = 128;
		} else
			charger->full_check_current = pdata->full_check_current;
	}

	return 0;
}
#endif
static int bq25898s_charger_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct bq25898s_charger *charger;
	struct bq25898s_charger_platform_data *pdata = client->dev.platform_data;

	int ret = 0;

	pr_info("%s: bq25898s Charger Driver Loading\n", __func__);

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE))
		return -EIO;

	charger = kzalloc(sizeof(*charger), GFP_KERNEL);
	if (!charger)
		return -ENOMEM;

	mutex_init(&charger->i2c_lock);
	charger->dev = &client->dev;
	charger->i2c = client;

	if (client->dev.of_node) {

		pdata = kzalloc(sizeof(*pdata), GFP_KERNEL);
		if (!pdata) {
			dev_err(&client->dev, "Failed to allocate memory\n");
			ret = -ENOMEM;
			goto err_parse_dt_nomem;
		}
#if defined(CONFIG_OF)
		ret = bq25898s_charger_parse_dt(charger, pdata);
		if (ret < 0) {
			pr_err("%s not found charger dt! ret[%d]\n",
					__func__, ret);
			goto err_parse_dt;
		}
#endif
	}
	charger->pdata = pdata;
	i2c_set_clientdata(client, charger);

	charger->psy_chg.name           = "bq25898s-charger";
	charger->psy_chg.type           = POWER_SUPPLY_TYPE_UNKNOWN;
	charger->psy_chg.get_property   = bq25898s_chg_get_property;
	charger->psy_chg.set_property   = bq25898s_chg_set_property;
	charger->psy_chg.properties     = bq25898s_charger_props;
	charger->psy_chg.num_properties = ARRAY_SIZE(bq25898s_charger_props);

	charger->cable_type = SEC_BATTERY_CABLE_NONE;
	bq25898s_charger_initialize(charger);
	charger->input_current = bq25898s_get_input_current(charger);
	charger->charging_current = bq25898s_get_charge_current(charger);
	pr_info("%s: input: %d, charging: %d\n", __func__, charger->input_current, charger->charging_current);

	ret = power_supply_register(charger->dev, &charger->psy_chg);
	if (ret) {
		pr_err("%s: Failed to Register psy_chg\n", __func__);
		goto err_data_free;
	}
	if (pdata->irq_gpio) {
		charger->chg_irq = gpio_to_irq(pdata->irq_gpio);

		ret = request_threaded_irq(charger->chg_irq, NULL,
			bq25898s_irq_handler,
			IRQF_TRIGGER_LOW | IRQF_ONESHOT,
			"bq25898s-irq", charger);
		if (ret < 0) {
			pr_err("%s: Failed to Request IRQ(%d)\n", __func__, ret);
			goto err_req_irq;
		}
	}
	device_init_wakeup(charger->dev, 1);

	ret = sysfs_create_group(&charger->psy_chg.dev->kobj, &bq25898s_attr_group);
	if (ret) {
		dev_info(&client->dev,
			"%s: sysfs_create_group failed\n", __func__);
	}
	charger->size = BQ25898S_CHG_REG_14;
	pr_info("%s: bq25898s Charger Driver Loaded\n", __func__);

	return 0;

err_req_irq:
	power_supply_unregister(&charger->psy_chg);
err_data_free:
err_parse_dt:
	kfree(pdata);
err_parse_dt_nomem:
	mutex_destroy(&charger->i2c_lock);
	kfree(charger);

	return ret;
}

static const struct i2c_device_id bq25898s_charger_id[] = {
	{"bq25898s-charger", 0},
	{}
};

#ifdef CONFIG_OF
static struct of_device_id bq25898s_charger_match_table[] = {
	{.compatible = "ti,bq25898s-charger"},
	{},
};
#else
#define da9155_charger_match_table NULL
#endif

static void bq25898s_charger_shutdown(struct i2c_client *client)
{
	struct bq25898s_charger *charger = i2c_get_clientdata(client);

	if (charger->chg_irq)
		free_irq(charger->chg_irq, charger);
	pr_info("%s: bq25898s Charger driver shutdown\n", __func__);
	if (!charger->i2c) {
		pr_err("%s: no bq25898s i2c client\n", __func__);
		return;
	}
	/* reset register */
	bq25898s_update_reg(charger->i2c, BQ25898S_CHG_REG_14, 0x80, 0x80);
}

static int bq25898s_charger_remove(struct i2c_client *client)
{
	struct bq25898s_charger *charger = i2c_get_clientdata(client);

	if (charger->chg_irq)
		free_irq(charger->chg_irq, charger);
	power_supply_unregister(&charger->psy_chg);
	mutex_destroy(&charger->i2c_lock);
	kfree(charger->pdata);
	kfree(charger);

	return 0;
}

#if defined CONFIG_PM
static int bq25898s_charger_suspend(struct device *dev)
{
	struct bq25898s_charger *charger = dev_get_drvdata(dev);
	if (charger->chg_irq) {
		if (device_may_wakeup(dev))
			enable_irq_wake(charger->chg_irq);
		disable_irq(charger->chg_irq);
	}
	return 0;
}

static int bq25898s_charger_resume(struct device *dev)
{
	struct bq25898s_charger *charger = dev_get_drvdata(dev);
	if (charger->chg_irq) {
		if (device_may_wakeup(dev))
			disable_irq_wake(charger->chg_irq);
		enable_irq(charger->chg_irq);
	}
	return 0;
}
#else
#define bq25898s_charger_suspend NULL
#define bq25898s_charger_resume NULL
#endif


static SIMPLE_DEV_PM_OPS(bq25898s_charger_pm_ops, bq25898s_charger_suspend,
		bq25898s_charger_resume);

static struct i2c_driver bq25898s_charger_driver = {
	.driver = {
		.name = "bq25898s-charger",
		.owner = THIS_MODULE,
#ifdef CONFIG_PM
		.pm = &bq25898s_charger_pm_ops,
#endif
		.of_match_table = bq25898s_charger_match_table,
	},
	.probe = bq25898s_charger_probe,
	.remove = bq25898s_charger_remove,
	.shutdown = bq25898s_charger_shutdown,
	.id_table = bq25898s_charger_id,
};

static int __init bq25898s_charger_init(void)
{
	pr_info("%s : \n", __func__);
	return i2c_add_driver(&bq25898s_charger_driver);
}

static void __exit bq25898s_charger_exit(void)
{
	i2c_del_driver(&bq25898s_charger_driver);
}

module_init(bq25898s_charger_init);
module_exit(bq25898s_charger_exit);

MODULE_DESCRIPTION("Samsung BQ25898S Charger Driver");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");
