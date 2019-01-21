/*
 * MELFAS MIP4 Touchkey
 *
 * Copyright (C) 2016 MELFAS Inc.
 *
 * mip4_mod.c : Model dependent functions
 *
 * Version : 2016.05.16
 */

#include "mip4.h"

/*
 * Control regulator
 */
int mip4_tk_regulator_control(struct mip4_tk_info *info, int enable)
{
	struct mip4_tk_platform_data *pdata = info->pdata;
	struct i2c_client *client = info->client;
	struct device *dev = &client->dev;
	struct regulator *reg_pwr = NULL, *reg_bus = NULL;
	int ret = 0;
	static bool on;

	input_info(true, dev, "%s: %s\n", __func__, enable ? "on" : "off");

	if (on == enable) {
		input_info(true, dev,
			"%s: regulator already %s - skip\n",
			__func__, enable ? "enabled" : "disabled");
		return 0;
	}

	/* Get regulator */
	if (pdata->pwr_reg_name) {
		reg_pwr = regulator_get(dev, pdata->pwr_reg_name);
		if (IS_ERR(reg_pwr)) {
		input_err(true, dev,
				"%s [ERROR] regulator_get : %s\n",
				__func__, pdata->pwr_reg_name);
			ret = PTR_ERR(reg_pwr);
			reg_pwr = NULL;

			goto ERROR;
		}
	}

	if (pdata->bus_reg_name) {
		reg_bus = regulator_get(dev, pdata->bus_reg_name);
		if (IS_ERR(reg_bus)) {
			input_err(true, dev,
				"%s [ERROR] regulator_get : %s\n",
				__func__, pdata->bus_reg_name);
			ret = PTR_ERR(reg_bus);
			reg_bus = NULL;

			goto ERROR;
		}
	}

	/* Control regulator */
	if (enable) {
		if (reg_pwr) {
			ret = regulator_enable(reg_pwr);
			if (ret) {
				input_err(ture, dev,
					"%s [ERROR] regulator_enable : %s\n",
					__func__, pdata->pwr_reg_name);

				goto ERROR;
			}
		}

		if (reg_bus) {
			ret = regulator_enable(reg_bus);
			if (ret) {
				input_err(true, dev,
					"%s [ERROR] regulator_enable : %s\n",
					__func__, pdata->pwr_reg_name);

				goto ERROR;
			}
		}
	} else {
		if (reg_pwr)
			regulator_disable(reg_pwr);

		if (reg_bus)
			regulator_disable(reg_bus);
	}

	on = enable;
	if (reg_pwr)
		regulator_put(reg_pwr);

	if (reg_bus)
		regulator_put(reg_bus);

	return 0;

ERROR:
	if (reg_pwr) {
		if (regulator_is_enabled(reg_pwr))
			regulator_disable(reg_pwr);
	}

	if (reg_bus) {
		if (regulator_is_enabled(reg_bus))
			regulator_disable(reg_bus);
	}

	if (reg_pwr)
		regulator_put(reg_pwr);

	if (reg_bus)
		regulator_put(reg_bus);

	input_err(true, dev, "%s [ERROR]\n", __func__);

	return ret;
}

/**
* Turn off power supply
*/
int mip4_tk_power_off(struct mip4_tk_info *info)
{
	struct mip4_tk_platform_data *pdata = info->pdata;
	int ret = 0;

	input_info(true, &info->client->dev, "%s\n", __func__);

	if (pdata->pwr_reg_name || pdata->bus_reg_name) {
		ret = mip4_tk_regulator_control(info, false);
	} else {
		if (pdata->gpio_pwr_en)
			gpio_set_value(pdata->gpio_pwr_en, false);

		if (pdata->gpio_bus_en)
			gpio_set_value(pdata->gpio_bus_en, false);
	}

	return ret;
}

/**
* Turn on power supply
*/
int mip4_tk_power_on(struct mip4_tk_info *info)
{
	struct mip4_tk_platform_data *pdata = info->pdata;
	int ret = 0;

	input_info(true, &info->client->dev, "%s\n", __func__);

	if (pdata->pwr_reg_name || pdata->bus_reg_name) {
		ret = mip4_tk_regulator_control(info, true);
	} else {
		if (pdata->gpio_pwr_en)
			gpio_set_value(pdata->gpio_pwr_en, true);

		if (pdata->gpio_bus_en)
			gpio_set_value(pdata->gpio_bus_en, true);
	}

	msleep(100);

	return ret;
}

/**
* Clear key input event status
*/
void mip4_tk_clear_input(struct mip4_tk_info *info)
{
	int i;

	input_info(true, &info->client->dev, "%s\n", __func__);

	for (i = 0; i < info->key_num; i++) {
		input_report_key(info->input_dev, info->key_code[i], 0);
	}

	input_sync(info->input_dev);

	return;
}

/**
* Input event handler - Report input event
*/
void mip4_tk_input_event_handler(struct mip4_tk_info *info, u8 sz, u8 *buf)
{
	int i;
	int id, type;
	int state;
	int strength;
	int keycode;

	for (i = 0; i < sz; i += info->event_size) {
		u8 *packet = &buf[i];

		/* Event format & type */
		if (info->event_format == 4) {
			strength = packet[1];
		} else if (info->event_format == 9) {
			strength = (packet[1] << 8) | packet[2];
		} else {
			input_err(true, &info->client->dev,
				"%s [ERROR] Unknown event format [%d]\n",
				__func__, info->event_format);
			goto ERROR;
		}

		id = packet[0] & 0x0F;
		type = (packet[0] & 0x40) >> 6;
		state = (packet[0] & 0x80) >> 7;

		if ((id >= 1) && (id <= info->key_num)) {
			if (type == 0) { /* KEY event */
				keycode = info->key_code[id - 1];

				input_report_key(info->input_dev, keycode, state);
#ifdef CONFIG_SAMSUNG_PRODUCT_SHIP				
				input_info(true, &info->client->dev, "%s - Key : Event[%d] ver 0x%02x\n",
							__func__, state, info->fw_version[7]);
#else
				input_info(true, &info->client->dev, "%s - Key : Code[%d] Event[%d] Strength[%d] ver 0x%02x\n",
							__func__, keycode, state, strength, info->fw_version[7]);
#endif
			} else if (type == 1) { /*Grip event*/
				//Do something ...

				input_info(true, &info->client->dev,
					"%s - Grip : ID[%d] Event[%d] Strength[%d]\n",
					__func__, id, state, strength);
			} else {
				input_err(true, &info->client->dev,
					"%s [ERROR] Unknown input type [%d]\n",
					__func__, type);
				continue;
			}
		} else {
			input_err(true, &info->client->dev, "%s [ERROR] Unknown Key ID [%d]\n", __func__, id);
			continue;
		}
	}

	input_sync(info->input_dev);

	return;

ERROR:
	input_err(true, &info->client->dev, "%s [ERROR]\n", __func__);
	return;
}

#ifdef CONFIG_OF
/*
 * Parse device tree
 */
int mip4_tk_parse_devicetree(struct device *dev, struct mip4_tk_info *info)
{
	struct mip4_tk_platform_data *pdata = info->pdata;
	struct device_node *np = dev->of_node;
	const char *name;
	int ret;
	u32 val;

	/* Get number of keys */
	ret = of_property_read_u32(np, MIP_DEV_NAME",keynum", &val);
	if (ret)
		input_err(true, dev,
			"%s [ERROR] of_property_read_u32 : keynum\n",
			__func__);
	else
		info->key_num = val;

	/* Get key code */
	ret = of_property_read_u32_array(
				np, MIP_DEV_NAME",keycode",
				info->key_code, info->key_num);
	if (ret) {
		input_err(true, dev,
			"%s [ERROR] of_property_read_u32_array : keycode\n",
			__func__);
		info->key_code_loaded = false;
	} else {
		info->key_code_loaded = true;
	}

	ret = of_property_read_string(np, MIP_DEV_NAME",pwr-reg-name", &name);
	if (ret) {
		input_err(true, dev,
			"%s [ERROR] of_property_read_string : pwr-reg-name\n",
			__func__);
		pdata->pwr_reg_name = NULL;
	} else {
		pdata->pwr_reg_name = name;
	}

	ret = of_property_read_string(np, MIP_DEV_NAME",bus-reg-name", &name);
	if (ret) {
		input_err(true, dev,
			"%s [ERROR] of_property_read_string : bus-reg-name\n",
			__func__);
		pdata->bus_reg_name = NULL;
	} else {
		pdata->bus_reg_name = name;
	}

	ret = of_property_read_string(np, MIP_DEV_NAME",firmware-name", &name);
	if (ret) {
		input_err(true, dev,
			"%s [ERROR] of_property_read_string : firmware-name\n",
			__func__);
		pdata->firmware_name = NULL;
	} else {
		pdata->firmware_name = name;
	}

	/* Get GPIO for irq */
	val = of_get_named_gpio(np, MIP_DEV_NAME",irq-gpio", 0);
	if (!gpio_is_valid(val)) {
		input_err(true, dev,
			"%s [ERROR] of_get_named_gpio : irq-gpio\n",
			__func__);
		ret = -EINVAL;
		goto error;
	} else {
		pdata->gpio_intr = val;
	}

	/* Get GPIO for pwr*/
	val = of_get_named_gpio(np, MIP_DEV_NAME",pwr-en-gpio", 0);
	if (!gpio_is_valid(val)) {
		input_err(true, dev,
			"%s [ERROR] of_get_named_gpio : pwr-en-gpio\n",
			__func__);
		pdata->gpio_pwr_en = 0;
	} else {
		pdata->gpio_pwr_en = val;
	}

	/* Get GPIO I2C*/
	val = of_get_named_gpio(np, MIP_DEV_NAME",bus-en-gpio", 0);
	if (!gpio_is_valid(val)) {
		input_err(true, dev,
			"%s [ERROR] of_get_named_gpio : bus-en-gpio\n",
			__func__);
		pdata->gpio_bus_en = 0;
	} else {
		pdata->gpio_bus_en = val;
	}

	/* Config GPIO for irq */
	ret = devm_gpio_request(dev, pdata->gpio_intr, "irq-gpio");
	if (ret) {
		input_err(true, dev, "%s [ERROR] gpio_request : irq-gpio\n", __func__);
		goto error;
	}
	gpio_direction_input(pdata->gpio_intr);
	info->client->irq = gpio_to_irq(pdata->gpio_intr);

	/* Config GPIO for pwr */
	if (pdata->gpio_pwr_en) {
		ret = devm_gpio_request(dev, pdata->gpio_pwr_en, "pwr-en-gpio");
		if (ret) {
			input_err(true, dev,
				"%s [ERROR] gpio_request : pwr-en-gpio\n",
				__func__);
			goto error;
		}
		gpio_direction_output(pdata->gpio_pwr_en, 0);
	}

	/* Config GPIO for bus */
	if (pdata->gpio_bus_en) {
		ret = devm_gpio_request(dev, pdata->gpio_bus_en, "bus-en-gpio");
		if (ret) {
			input_err(true, dev,
				"%s [ERROR] gpio_request : bus-en-gpio\n",
				__func__);
			goto error;
		}
		gpio_direction_output(pdata->gpio_bus_en, 0);
	}

	return 0;

error:
	return ret;
}
#endif

/**
* Config input interface
*/
void mip4_tk_config_input(struct mip4_tk_info *info)
{
	struct input_dev *input_dev = info->input_dev;
	int i;

	set_bit(EV_SYN, input_dev->evbit);
	set_bit(EV_KEY, input_dev->evbit);

	set_bit(EV_LED, input_dev->evbit);
	set_bit(LED_MISC, input_dev->ledbit);

	if(info->key_code_loaded == false){
		info->key_code[0] = KEY_RECENT;
		info->key_code[1] = KEY_BACK;
		info->key_num = 2;
	}

	for(i = 0; i < info->key_num; i++)
		set_bit(info->key_code[i], input_dev->keybit);

	input_info(true, &info->client->dev, "%s [DONE]\n", __func__);

	return;
}

#if MIP_USE_CALLBACK
/**
* Callback - get charger status
*/
void mip4_tk_callback_charger(struct mip4_tk_callbacks *cb, int charger_status)
{
	struct mip4_tk_info *info = container_of(cb, struct mip4_tk_info, callbacks);

	input_dbg(true, &info->client->dev, "%s [START]\n", __func__);

	input_info(true, &info->client->dev, "%s - charger_status[%d]\n", __func__, charger_status);

	/* ... */

	input_dbg(true, &info->client->dev, "%s [DONE]\n", __func__);
}

/*
 * Config callback functions
 */
void mip4_tk_config_callback(struct mip4_tk_info *info)
{
	input_dbg(true, &info->client->dev, "%s [START]\n", __func__);

	info->register_callback = info->pdata->register_callback;

	/* callback functions */
	info->callbacks.inform_charger = mip4_tk_callback_charger;
	/* info->callbacks.inform_display = mip4_tk_callback_display;
	... */

	if (info->register_callback) {
		info->register_callback(&info->callbacks);
	}

	input_dbg(true, &info->client->dev, "%s [DONE]\n", __func__);
	return;
}
#endif

