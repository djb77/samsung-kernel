/*
 * MELFAS MMS400 Touchscreen
 *
 * Copyright (C) 2014 MELFAS Inc.
 *
 *
 * Model dependent functions
 *
 */

#include "melfas_mms400.h"

#ifdef USE_TSP_TA_CALLBACKS
static bool ta_connected = 0;
#endif

/**
 * Control power supply
 */
int mms_power_control(struct mms_ts_info *info, int enable)
{
	int ret;
	struct i2c_client *client = info->client;

	gpio_direction_output(info->dtdata->gpio_vdd, enable);

	if (!IS_ERR_OR_NULL(info->dtdata->vreg_vio)) {
		if (enable) {
			ret = regulator_enable(info->dtdata->vreg_vio);
			if (ret)
				input_err(true, &client->dev,
						"%s [ERROR] regulator_enable [%d]\n",
						__func__, ret);
		} else{
			ret = regulator_disable(info->dtdata->vreg_vio);
			if (ret)
				input_err(true, &client->dev,
						"%s [ERROR] regulator_disable [%d]\n",
						__func__, ret);
		}
	}

	if (!enable)
		usleep_range(10 * 1000, 11 * 1000);
	else
		msleep(50);

	input_info(true, &client->dev, "%s: %s", __func__, enable? "on":"off");
	if (gpio_is_valid(info->dtdata->gpio_vdd))
		input_info(true, &client->dev, " vdd:%d \n", gpio_get_value(info->dtdata->gpio_vdd));
	if (!IS_ERR_OR_NULL(info->dtdata->vreg_vio))
		input_info(true, &client->dev, " vio_reg:%d \n",
				regulator_is_enabled(info->dtdata->vreg_vio));
	return 0;
}

/**
 * Clear touch input events
 */
void mms_clear_input(struct mms_ts_info *info)
{
	int i;

	input_info(true, &info->client->dev, "%s\n", __func__);

	input_report_key(info->input_dev, BTN_TOUCH, 0);
	input_report_key(info->input_dev, BTN_TOOL_FINGER, 0);

	for (i = 0; i< MAX_FINGER_NUM; i++) {
		info->finger[i].finger_state = 0;
		info->finger[i].move_count= 0;
		input_mt_slot(info->input_dev, i);
		input_mt_report_slot_state(info->input_dev, MT_TOOL_FINGER, false);
	}

	info->touch_count = 0;

	input_sync(info->input_dev);
}

#ifdef CONFIG_SAMSUNG_PRODUCT_SHIP
static char location_detect(struct mms_ts_info *info, int coord, bool flag)
{
	/* flag ? coord = Y : coord = X */
	int x_devide = info->max_x / 3;
	int y_devide = info->max_y / 3;

	if (flag) {
		if (coord < y_devide)
			return 'H';
		else if (coord < y_devide * 2)
			return 'M';
		else
			return 'L';
	} else {
		if (coord < x_devide)
			return '0';
		else if (coord < x_devide * 2)
			return '1';
		else
			return '2';
	}

	return 'E';
}
#endif


/**
 * Input event handler - Report touch input event
 */
void mms_input_event_handler(struct mms_ts_info *info, u8 sz, u8 *buf)
{
	struct i2c_client *client = info->client;
	int i;

	input_dbg(true, &client->dev, "%s [START]\n", __func__);
	input_dbg(true, &client->dev, "%s - sz[%d] buf[0x%02X]\n", __func__, sz, buf[0]);

	for (i = 1; i < sz; i += info->event_size) {
		u8 *tmp = &buf[i];

		int id = (tmp[0] & MIP_EVENT_INPUT_ID) - 1;
		int x = tmp[2] | ((tmp[1] & 0xf) << 8);
		int y = tmp[3] | (((tmp[1] >> 4) & 0xf) << 8);

		/* old protocal   	int touch_major = tmp[4];
		int pressure = tmp[5];  */

		int pressure = tmp[4];
		//int size = tmp[5];		// sumsize
		int touch_major = tmp[6];
		int touch_minor = tmp[7];

		int palm = (tmp[0] & MIP_EVENT_INPUT_PALM) >> 4;

		// Report input data
#if MMS_USE_TOUCHKEY
		if ((tmp[0] & MIP_EVENT_INPUT_SCREEN) == 0) {
			//Touchkey Event
			int key = tmp[0] & 0xf;
			int key_state = (tmp[0] & MIP_EVENT_INPUT_PRESS) ? 1 : 0;
			int key_code = 0;

			//Report touchkey event
			switch (key) {
			case 1:
				key_code = KEY_MENU;
				//dev_dbg(&client->dev, "Key : KEY_MENU\n");
				break;
			case 2:
				key_code = KEY_BACK;
				//dev_dbg(&client->dev, "Key : KEY_BACK\n");
				break;
			default:
				input_err(true, &client->dev,
					"%s [ERROR] Unknown key code [%d]\n",
					__func__, key);
				continue;
				break;
			}

			input_report_key(info->input_dev, key_code, key_state);

			input_dbg(true, &client->dev, "%s - Key : ID[%d] Code[%d] State[%d]\n",
				__func__, key, key_code, key_state);
		} else
#endif
		{
			//Report touchscreen event
			if ((tmp[0] & MIP_EVENT_INPUT_PRESS) == 0) {
				//Release
				input_mt_slot(info->input_dev, id);
#ifdef CONFIG_SEC_FACTORY
				input_report_abs(info->input_dev, ABS_MT_PRESSURE, 0);
#endif
				input_mt_report_slot_state(info->input_dev,
								MT_TOOL_FINGER, false);
				if (info->finger[id].finger_state != 0) {
					info->touch_count--;
					if (!info->touch_count) {
						input_report_key(info->input_dev, BTN_TOUCH, 0);
						input_report_key(info->input_dev,
									BTN_TOOL_FINGER, 0);
					}

					info->finger[id].finger_state = 0;
#ifdef CONFIG_SAMSUNG_PRODUCT_SHIP
					input_info(true, &client->dev,
						"R[%d] loc:%c%c V[%02x%02x%02x] tc:%d mc:%d\n",
						id, location_detect(info, info->finger[id].ly, 1),
						location_detect(info, info->finger[id].lx, 0),
						info->boot_ver_ic, info->core_ver_ic,
						info->config_ver_ic, info->touch_count,
						info->finger[id].move_count);
#else
					input_err(true, &client->dev,
						"R[%d] (%d, %d) V[%02x%02x%02x] tc:%d mc:%d\n",
						id, info->finger[id].lx, info->finger[id].ly,
						info->boot_ver_ic, info->core_ver_ic,
						info->config_ver_ic, info->touch_count,
						info->finger[id].move_count);
#endif
					info->finger[id].move_count = 0;
				}
				continue;
			}

			//Press or Move
			input_mt_slot(info->input_dev, id);
			input_mt_report_slot_state(info->input_dev, MT_TOOL_FINGER, true);
			input_report_key(info->input_dev, BTN_TOUCH, 1);
			input_report_key(info->input_dev, BTN_TOOL_FINGER, 1);
			input_report_abs(info->input_dev, ABS_MT_POSITION_X, x);
			input_report_abs(info->input_dev, ABS_MT_POSITION_Y, y);
#ifdef CONFIG_SEC_FACTORY
			if (pressure)
				input_report_abs(info->input_dev, ABS_MT_PRESSURE, pressure);
			else
				input_report_abs(info->input_dev, ABS_MT_PRESSURE, 1);
#endif
			input_report_abs(info->input_dev, ABS_MT_TOUCH_MAJOR, touch_major);
			input_report_abs(info->input_dev, ABS_MT_TOUCH_MINOR, touch_minor);
			input_report_abs(info->input_dev, ABS_MT_PALM, palm);

			info->finger[id].lx = x;
			info->finger[id].ly = y;

			if (info->finger[id].finger_state > 0)
				info->finger[id].move_count++;

			if (info->finger[id].finger_state == 0) {
				info->finger[id].finger_state = 1;
				info->finger[id].move_count = 0;
				info->touch_count++;
#ifdef CONFIG_SAMSUNG_PRODUCT_SHIP
				input_info(true, &client->dev,
					"P[%d] loc:%c%c z:%d p:%d m:%d,%d tc:%d\n",
					id, location_detect(info, y, 1), location_detect(info, x, 0),
					pressure, palm, touch_major, touch_minor, info->touch_count);
#else
				input_err(true, &client->dev,
					"P[%d] (%d, %d) z:%d p:%d m:%d,%d tc:%d\n",
					id, x, y, pressure, palm, touch_major, touch_minor, info->touch_count);
#endif
			}
		}
	}
	input_sync(info->input_dev);
	input_dbg(true, &client->dev, "%s [DONE]\n", __func__);
	return;
}

int mms_pinctrl_configure(struct mms_ts_info *info, int active)
{
	struct pinctrl_state *set_state_i2c;
	int retval;

	input_dbg(true, &info->client->dev, "%s: %d\n", __func__, active);

	if (active) {
		set_state_i2c =	pinctrl_lookup_state(info->pinctrl, "tsp_gpio_active");
		if (IS_ERR(set_state_i2c)) {
			input_err(true, &info->client->dev,
				"%s: cannot get pinctrl(i2c) active state\n", __func__);
			return PTR_ERR(set_state_i2c);
		}
	} else {
		set_state_i2c =	pinctrl_lookup_state(info->pinctrl, "tsp_gpio_suspend");
		if (IS_ERR(set_state_i2c)) {
			input_err(true, &info->client->dev,
				"%s: cannot get pinctrl(i2c) suspend state\n", __func__);
			return PTR_ERR(set_state_i2c);
		}
	}
	retval = pinctrl_select_state(info->pinctrl, set_state_i2c);
	if (retval) {
		input_err(true, &info->client->dev,
			"%s: cannot set pinctrl(i2c) %d state\n", __func__, active);
		return retval;
	}

	return 0;
}


#if MMS_USE_DEVICETREE
/**
 * Parse device tree
 */
int mms_parse_devicetree(struct device *dev, struct mms_ts_info *info)
{
	struct device_node *np = dev->of_node;
	int ret;

	input_dbg(true, dev, "%s [START]\n", __func__)
		;
	ret = of_property_read_u32(np, "melfas,max_x", &info->dtdata->max_x);
	if (ret) {
		input_err(true, dev, "%s [ERROR] max_x\n", __func__);
		info->dtdata->max_x = 1080 * 2;
	}

	ret = of_property_read_u32(np, "melfas,max_y", &info->dtdata->max_y);
	if (ret) {
		input_err(true, dev, "%s [ERROR] max_y\n", __func__);
		info->dtdata->max_y = 1920 * 2;
	}

	info->dtdata->gpio_intr = of_get_named_gpio(np, "melfas,irq-gpio", 0);
	gpio_request(info->dtdata->gpio_intr, "irq-gpio");
	gpio_direction_input(info->dtdata->gpio_intr);
	info->client->irq = gpio_to_irq(info->dtdata->gpio_intr);

	info->dtdata->gpio_vdd = of_get_named_gpio(np, "melfas,vdd_en", 0);
	gpio_request(info->dtdata->gpio_vdd, "melfas_vdd_gpio");

	info->dtdata->vreg_vio = regulator_get(dev, "vdd-io");
	if (IS_ERR(info->dtdata->vreg_vio))
		input_err(true, dev, "%s [ERROR] vdd-io get\n", __func__);

	ret = of_property_read_string(np, "melfas,fw_name", &info->dtdata->fw_name);
	if (ret < 0)
		info->dtdata->fw_name = NULL;

	ret = of_property_read_u32(np, "melfas,bringup", &info->dtdata->bringup);
	if (ret < 0)
		info->dtdata->bringup = 0;

	info->dtdata->support_lpm = of_property_read_bool(np, "melfas,support_lpm");

	input_info(true, dev, "%s: max_x:%d max_y:%d int:%d irq:%d sda:%d scl:%d"
		" vdd:%d gpio_vio:%d fwname:%s, bringup:%d, support_LPM:%d\n",
		__func__, info->dtdata->max_x, info->dtdata->max_y,
		info->dtdata->gpio_intr, info->client->irq, info->dtdata->gpio_sda,
		info->dtdata->gpio_scl, info->dtdata->gpio_vdd,
		info->dtdata->gpio_vio, info->dtdata->fw_name,
		info->dtdata->bringup, info->dtdata->support_lpm);

	return 0;
}
#endif

/**
 * Config input interface
 */
void mms_config_input(struct mms_ts_info *info)
{
	struct input_dev *input_dev = info->input_dev;

	input_dbg(true, &info->client->dev, "%s [START]\n", __func__);

	//Screen
	set_bit(EV_SYN, input_dev->evbit);
	set_bit(EV_ABS, input_dev->evbit);

	set_bit(BTN_TOUCH, input_dev->keybit);
	set_bit(BTN_TOOL_FINGER, input_dev->keybit);
	set_bit(INPUT_PROP_DIRECT, input_dev->propbit);

	input_mt_init_slots(input_dev, MAX_FINGER_NUM, INPUT_MT_DIRECT);

	input_set_abs_params(input_dev, ABS_MT_POSITION_X, 0, info->max_x -1, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y, 0, info->max_y -1, 0, 0);
#ifdef CONFIG_SEC_FACTORY
	input_set_abs_params(input_dev, ABS_MT_PRESSURE, 0, INPUT_PRESSURE_MAX, 0, 0);
#endif
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR, 0, INPUT_TOUCH_MAJOR_MAX, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MINOR, 0, INPUT_TOUCH_MINOR_MAX, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_PALM, 0, 1, 0, 0);

	//Key
	set_bit(EV_KEY, input_dev->evbit);
#if MMS_USE_TOUCHKEY
	set_bit(KEY_BACK, input_dev->keybit);
	set_bit(KEY_MENU, input_dev->keybit);
#endif
	set_bit(KEY_BLACK_UI_GESTURE, input_dev->keybit);
	input_dbg(true, &info->client->dev, "%s [DONE]\n", __func__);
	return;
}

/**
 * Callback - get charger status
 */
#ifdef USE_TSP_TA_CALLBACKS
void mms_charger_status_cb(struct tsp_callbacks *cb, int status)
{
	pr_err("%s %s: TA %s\n",
		SECLOG, __func__, status ? "connected" : "disconnected");

	if (status)
		ta_connected = true;
	else
		ta_connected = false;

	/* not yet defined functions */
}

void mms_register_callback(struct tsp_callbacks *cb)
{
	charger_callbacks = cb;
	pr_info("%s %s\n", SECLOG, __func__);
}
#endif

#ifdef CHARGER_NOTIFIER
static struct extcon_tsp_ta_cable support_cable_list[] = {
	{ .cable_type = EXTCON_USB, },
	{ .cable_type = EXTCON_TA, },
};

static struct mms_ts_info *tsp_driver = NULL;
void mms_set_tsp_info(struct mms_ts_info *tsp_data)
{
	if (tsp_data != NULL)
		tsp_driver = tsp_data;
	else
		pr_info("%s %s : tsp info is null\n", SECLOG, __func__);
}
static struct mms_ts_info * mms_get_tsp_info(void)
{
	return tsp_driver;
}

void set_charger_config(struct mms_ts_info *tsp_data)
{
	// not yet.
	input_err(true, &tsp_data->client->dev, "%s mode\n",	tsp_data->charger_mode ? "charging" : "battery");

}

static void mms_charger_notify_work(struct work_struct *work)
{
	struct extcon_tsp_ta_cable *cable =
			container_of(work, struct extcon_tsp_ta_cable, work);
	struct mms_ts_info *tsp_data = mms_get_tsp_info();
	//int rc;
	if (!tsp_data) {
		input_err(true, &tsp_data->client->dev, "%s tsp driver is null\n", __func__);
		return;
	}

	tsp_data->charger_mode = cable->cable_state;

	if (!tsp_data->enabled) {
		input_err(true, &tsp_data->client->dev, "%s tsp is stopped\n", __func__);
	//	schedule_delayed_work(&tsp_data->noti_dwork, HZ / 5);
		return ;
	}

	input_err(true, &tsp_data->client->dev, "%s mode\n",
		tsp_data->charger_mode ? "charging" : "battery");

	set_charger_config(tsp_data);
}

static int mms_charger_notify(struct notifier_block *nb,
					unsigned long stat, void *ptr)
{
	struct extcon_tsp_ta_cable *cable =
			container_of(nb, struct extcon_tsp_ta_cable, nb);
	pr_info("%s %s, %ld\n", SECLOG, __func__, stat);
	cable->cable_state = stat;

	schedule_work(&cable->work);

	return NOTIFY_DONE;

}

static int __init mms_init_charger_notify(void)
{
	struct mms_ts_info *tsp_data = mms_get_tsp_info();
	struct extcon_tsp_ta_cable *cable;
	int ret;
	int i;

	if (!tsp_data) {
		input_info(true, &tsp_data->client->dev, "%s tsp driver is null\n", __func__);
		return 0;
	}
	input_info(true, &tsp_data->client->dev, "%s register extcon notifier for usb and ta\n", __func__);
	for (i = 0; i < ARRAY_SIZE(support_cable_list); i++) {
		cable = &support_cable_list[i];
		INIT_WORK(&cable->work, mms_charger_notify_work);
		cable->nb.notifier_call = mms_charger_notify;

		ret = extcon_register_interest(&cable->extcon_nb,
				EXTCON_DEV_NAME,
				extcon_cable_name[cable->cable_type],
				&cable->nb);
		if (ret)
			input_err(true, &tsp_data->client->dev, "%s: fail to register extcon notifier(%s, %d)\n",
				__func__, extcon_cable_name[cable->cable_type],
				ret);

		cable->edev = cable->extcon_nb.edev;
		if (!cable->edev)
			input_err(true, &tsp_data->client->dev, "%s: fail to get extcon device\n", __func__);
	}
	return 0;
}
device_initcall_sync(mms_init_charger_notify);
#endif

void mms_set_cover_type(struct mms_ts_info *info)
{
	u8 wbuf[4];
	int ret = 0;

	if (info->flip_enable) {
		switch (info->cover_type) {
			case MMS_CLEAR_FLIP_COVER:
				break;
			default:
				input_err(true, &info->client->dev,
						"%s: touch is not supported for %d cover\n",
						__func__, info->cover_type);
				return;
		}
	}

	input_info(true, &info->client->dev, "%s: %d, type %d\n",
			__func__, info->flip_enable, info->cover_type);

	if (!info->enabled) {
		input_err(true, &info->client->dev, "%s: dev is not enabled\n", __func__);
		return;
	}

	wbuf[0] = MIP_R0_CTRL;
	wbuf[1] = MIP_R1_CTRL_WINDOW_MODE;
	wbuf[2] = info->flip_enable;

	ret = mms_i2c_write(info, wbuf, 3);
	if (ret) {
		input_err(true, &info->client->dev, "%s [ERROR] mms_i2c_write %x %x %x, ret %d\n",
				__func__, wbuf[0], wbuf[1], wbuf[2], ret);
		return;
	}
}

int mms_lowpower_mode(struct mms_ts_info *info, int on)
{
	u8 wbuf[3];
	u8 rbuf[1];

	if (!info->dtdata->support_lpm) {
		input_err(true, &info->client->dev, "%s not supported\n", __func__);
		return -EINVAL;
	}

	/*	bit	Power state
	  *	0	active
	  *	1	low power consumption
	  */

	wbuf[0] = MIP_R0_CTRL;
	wbuf[1] = MIP_R1_CTRL_POWER_STATE;
	wbuf[2] = on;

	if (mms_i2c_write(info, wbuf, 3)) {
		input_err(true, &info->client->dev, "%s [ERROR] write %x %x %x\n",
				__func__, wbuf[0], wbuf[1], wbuf[2]);
		return -EIO;
	}

	msleep(20);

	if (mms_i2c_read(info, wbuf, 2, rbuf, 1)) {
		input_err(true, &info->client->dev, "%s [ERROR] read %x %x, rbuf %x\n",
				__func__, wbuf[0], wbuf[1], rbuf[0]);
		return -EIO;
	}

	if (rbuf[0] != on) {
		input_err(true, &info->client->dev, "%s [ERROR] not changed to %s mode, rbuf %x\n",
				__func__, on ? "LPM" : "normal", rbuf[0]);
		return -EIO;
	}

	input_info(true, &info->client->dev, "%s: %s mode", __func__, on ? "LPM" : "normal");
	return 0;
}
