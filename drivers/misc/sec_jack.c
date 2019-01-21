/*  drivers/misc/sec_jack.c
 *
 *  Copyright (C) 2015 Samsung Electronics Co.Ltd
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 */
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/irq.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/input.h>
#include <linux/platform_device.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/switch.h>
#include <linux/input.h>
#include <linux/timer.h>
#include <linux/wakelock.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/gpio_event.h>
#include <linux/sec_jack.h>
#include <linux/of_gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/pinctrl/consumer.h>


#define NUM_INPUT_DEVICE_ID	2
#define MAX_ZONE_LIMIT		10
#define DEFAULT_KEY_DEBOUNCE_TIME_MS	30
#define DEFAULT_DET_DEBOUNCE_TIME_MS	100
#define WAKE_LOCK_TIME		(HZ * 5)	/* 5 sec */

struct sec_jack_info {
	struct platform_device *client;
	struct sec_jack_platform_data *pdata;
	struct work_struct buttons_work;
	struct work_struct detect_work;
	struct workqueue_struct *detect_wq;
	struct workqueue_struct *buttons_wq;
	struct timer_list timer;
	struct input_dev *input_dev;
	struct wake_lock det_wake_lock;
	struct input_handler jack_handler;
	struct input_device_id ids[NUM_INPUT_DEVICE_ID];
	int det_irq;
	int dev_id;
	int pressed;
	int pressed_code;
	bool buttons_enable;
	struct platform_device *send_key_dev;
	unsigned int cur_jack_type;
};

/* define call back function for buuton notifcation */
static sec_jack_button_notify_cb	button_notify_cb;

/* with some modifications like moving all the gpio structs inside
 * the platform data and getting the name for the switch and
 * gpio_event from the platform data, the driver could support more than
 * one headset jack, but currently user space is looking only for
 * one key file and switch for a headset so it'd be overkill and
 * untestable so we limit to one instantiation for now.
 */
static atomic_t instantiated = ATOMIC_INIT(0);

/* sysfs name HeadsetObserver.java looks for to track headset state
 */
struct switch_dev switch_jack_detection = {
	.name = "h2w",
};

/* Samsung factory test application looks for to track button state
 */
struct switch_dev switch_sendend = {
	.name = "send_end", /* /sys/class/switch/send_end/state */
};

static struct gpio_event_direct_entry sec_jack_key_map[] = {
	{
		.code	= KEY_UNKNOWN,
	},
};

static struct gpio_event_input_info sec_jack_key_info = {
	.info.func = gpio_event_input_func,
	.info.no_suspend = true,
	.type = EV_KEY,
	.debounce_time.tv64 = DEFAULT_KEY_DEBOUNCE_TIME_MS * NSEC_PER_MSEC,
	.keymap = sec_jack_key_map,
	.keymap_size = ARRAY_SIZE(sec_jack_key_map)
};

static struct gpio_event_info *sec_jack_input_info[] = {
	&sec_jack_key_info.info,
};

static struct gpio_event_platform_data sec_jack_input_data = {
	.name = "sec_jack",
	.info = sec_jack_input_info,
	.info_count = ARRAY_SIZE(sec_jack_input_info),
};

struct sec_jack_control_data jack_controls;
EXPORT_SYMBOL_GPL(jack_controls);

int sec_jack_register_button_notify_cb(sec_jack_button_notify_cb func)
{
	if (func == NULL)
		return -1;
	button_notify_cb = func;
	return 0;
}
EXPORT_SYMBOL_GPL(sec_jack_register_button_notify_cb);

int get_jack_state(){
	return jack_controls.jack_type;
}
EXPORT_SYMBOL_GPL(get_jack_state);

static int sec_jack_gpio_init(struct sec_jack_platform_data *pdata)
{
	int ret;

	if (!IS_ERR_OR_NULL(pdata->jack_pinctrl)) {
		ret = pinctrl_select_state(pdata->jack_pinctrl,
			pdata->jack_pins_active);
		if (ret) {
			pr_err("%s(): Failed to pinctrl set_state\n", __func__);
			pdata->jack_pinctrl = NULL;
			return ret;
		}
	}

	if (pdata->det_gpio > 0) {
		ret = gpio_request(pdata->det_gpio, "jack_detect");
		if (ret) {
			pr_err("%s: failed to request det_gpio\n", __func__);
			return ret;
		}
	}

	if (pdata->ear_micbias_en_gpio > 0) {
		ret = gpio_request(pdata->ear_micbias_en_gpio,
			"ear_micbias_en");
		if (ret) {
			pr_err("%s: failed to request ear_micbias_en\n",
				__func__);
			goto err_request_ear_micbias;
		}
		gpio_direction_output(pdata->ear_micbias_en_gpio, 0);
	}

	return 0;

err_request_ear_micbias:
	if (pdata->det_gpio)
		gpio_free(pdata->det_gpio);

	return ret;
}

static int sec_jack_get_adc_value(struct sec_jack_info *hi)
{
	struct sec_jack_platform_data *pdata = hi->pdata;
	int adc_val;

	if (pdata->jack_controls->get_adc) {
		adc_val = pdata->jack_controls->get_adc();
		dev_info(&hi->client->dev, "%s: %d\n", __func__, adc_val);
	} else {
		dev_err(&hi->client->dev, "No function for get_adc\n");
		return -EFAULT;
	}

	return adc_val;
}

static void set_sec_micbias_state(struct sec_jack_info *hi, bool state)
{
	struct sec_jack_platform_data *pdata = hi->pdata;

	if (pdata->ear_micbias_en_gpio > 0)
		gpio_set_value_cansleep(pdata->ear_micbias_en_gpio, state);
	else if (pdata->jack_controls->set_micbias)
		pdata->jack_controls->set_micbias(state);
	else
		dev_err(&hi->client->dev, "No gpio or function for bias control\n");
}

/* gpio_input driver does not support to read adc value.
 * We use input filter to support 3-buttons of headset
 * without changing gpio_input driver.
 */
static bool sec_jack_buttons_filter(struct input_handle *jack_handle,
	unsigned int type, unsigned int code, int value)
{
	struct sec_jack_info *hi = jack_handle->handler->private;

	if (type != EV_KEY || code != KEY_UNKNOWN)
		return false;

	hi->pressed = value;

	/* This is called in timer handler of gpio_input driver.
	 * We use workqueue to read adc value.
	 */
	queue_work(hi->buttons_wq, &hi->buttons_work);

	return true;
}

static int sec_jack_buttons_connect(struct input_handler *jack_handler,
	struct input_dev *dev, const struct input_device_id *id)
{
	struct sec_jack_info *hi;
	struct sec_jack_platform_data *pdata;
	struct sec_jack_buttons_zone *btn_zones;
	int err;
	int i;
	int num_buttons_zones;
	struct input_handle *jack_handle;

	/* bind input_handler to input device related to only sec_jack */
	if (dev->name != sec_jack_input_data.name)
		return -ENODEV;

	hi = jack_handler->private;
	pdata = hi->pdata;
	btn_zones = pdata->jack_buttons_zones;
	num_buttons_zones = ARRAY_SIZE(pdata->jack_buttons_zones);

	hi->input_dev = dev;

	jack_handle = kzalloc(sizeof(*jack_handle), GFP_KERNEL);
	if (!jack_handle) {
		dev_err(&hi->client->dev, "Failed to alloc jack_handle\n");
		return -ENOMEM;
	}

	jack_handle->dev = dev;
	jack_handle->handler = jack_handler;
	jack_handle->name = "sec_jack_buttons";

	err = input_register_handle(jack_handle);
	if (err) {
		dev_err(&hi->client->dev,
			"Failed to input_register_handle %d\n", err);
		goto err_register_handle;
	}

	err = input_open_device(jack_handle);
	if (err) {
		dev_err(&hi->client->dev,
			"Failed to input_open_device %d\n", err);
		goto err_open_device;
	}

	for (i = 0; i < num_buttons_zones; i++)
		input_set_capability(dev, EV_KEY, btn_zones[i].code);

	return 0;

err_open_device:
	input_unregister_handle(jack_handle);
err_register_handle:
	kfree(jack_handle);

	return err;
}

static void sec_jack_buttons_disconnect(struct input_handle *jack_handle)
{
	input_close_device(jack_handle);
	input_unregister_handle(jack_handle);
	kfree(jack_handle);
}

static void sec_jack_set_type(struct sec_jack_info *hi,
	enum sec_jack_type jack_type)
{
	/* this can happen during slow inserts where we think we identified
	 * the type but then we get another interrupt and do it again
	 */
	if (jack_type == hi->cur_jack_type) {
		if ((jack_type != SEC_HEADSET_4POLE) &&
			(jack_type != SEC_EXTERNAL_ANTENNA))
			set_sec_micbias_state(hi, false);
		return;
	}

	if ((jack_type == SEC_HEADSET_4POLE) ||
		(jack_type == SEC_EXTERNAL_ANTENNA)) {
		/* for a 4 pole headset, enable detection of send/end key */
		if (hi->send_key_dev == NULL)
			/* enable to get events again */
			hi->send_key_dev = platform_device_register_data(NULL,
				GPIO_EVENT_DEV_NAME,
				hi->dev_id,
				&sec_jack_input_data,
				sizeof(sec_jack_input_data));
			mod_timer(&hi->timer,
				jiffies + msecs_to_jiffies(1000));
	} else {
		/* micbias is left enabled for 4pole and disabled otherwise */
		set_sec_micbias_state(hi, false);
	
		/* for all other jacks, disable send/end key detection */
		if (hi->send_key_dev != NULL) {
			/* disable to prevent false events on next insert */
			platform_device_unregister(hi->send_key_dev);
			hi->send_key_dev = NULL;
			del_timer_sync(&hi->timer);
			hi->buttons_enable = false;
		}
	}

	if ((jack_type == SEC_HEADSET_4POLE) ||
		(jack_type == SEC_HEADSET_3POLE)) {
		if (hi->pdata->jack_controls->hp_imp_detect)
			hi->pdata->jack_controls->hp_imp_detect();
	}

	hi->cur_jack_type = jack_type;
	jack_controls.jack_type = jack_type;
	dev_info(&hi->client->dev, "jack_type: %d\n", jack_type);

	switch_set_state(&switch_jack_detection, jack_type);

}

static void handle_jack_not_inserted(struct sec_jack_info *hi)
{
	if (hi->pdata->jack_controls->hp_imp_unplug)
			hi->pdata->jack_controls->hp_imp_unplug();

	sec_jack_set_type(hi, SEC_JACK_NO_DEVICE);
}

static void determine_jack_type(struct sec_jack_info *hi)
{
	struct sec_jack_platform_data *pdata = hi->pdata;
	struct sec_jack_zone *zones = pdata->jack_zones;
	int size = ARRAY_SIZE(pdata->jack_zones);
	int count[MAX_ZONE_LIMIT] = {0};
	int adc;
	int i;
	unsigned npolarity = !pdata->det_active_high;

	/* set mic bias to enable adc */
	set_sec_micbias_state(hi, true);

	while (gpio_get_value(pdata->det_gpio) ^ npolarity) {
		adc = sec_jack_get_adc_value(hi);
		if (adc < 0) {
			dev_err(&hi->client->dev, "Unavailable ADC %d\n", adc);
			return;
		}

		/* determine the type of headset based on the
		 * adc value.  An adc value can fall in various
		 * ranges or zones.  Within some ranges, the type
		 * can be returned immediately.  Within others, the
		 * value is considered unstable and we need to sample
		 * a few more types (up to the limit determined by
		 * the range) before we return the type for that range.
		 */

		for (i = 0; i < size; i++) {
			if (adc <= zones[i].adc_high) {
				if (++count[i] > zones[i].check_count) {
					sec_jack_set_type(hi,
						zones[i].jack_type);
					return;
				}
				if (zones[i].delay_us > 0)
					usleep_range(zones[i].delay_us,
					zones[i].delay_us);
				break;
			}
		}
	}

	/* jack removed before detection complete */
	dev_info(&hi->client->dev, "jack removed before detection complete\n");
	handle_jack_not_inserted(hi);
}

static ssize_t key_state_onoff_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct sec_jack_info *hi = dev_get_drvdata(dev);
	int value = 0;

	if (hi->pressed && hi->pressed_code == KEY_MEDIA)
		value = 1;

	return snprintf(buf, 4, "%d\n", value);
}

static DEVICE_ATTR(key_state, 0444 , key_state_onoff_show,
	NULL);

static ssize_t earjack_state_onoff_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct sec_jack_info *hi = dev_get_drvdata(dev);
	int value = 0;

	if (hi->cur_jack_type == SEC_HEADSET_4POLE)
		value = 1;

	return snprintf(buf, 4, "%d\n", value);
}

static DEVICE_ATTR(state, 0444 , earjack_state_onoff_show,
	NULL);

static ssize_t earjack_mic_adc_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct sec_jack_info *hi = dev_get_drvdata(dev);
	struct sec_jack_platform_data *pdata = hi->pdata;
	int adc_val = 0;

	if (pdata->jack_controls->get_adc) {
		adc_val = pdata->jack_controls->get_adc();
		dev_info(&hi->client->dev, "%s: %d\n", __func__, adc_val);
	}

	return snprintf(buf, 5, "%d\n", adc_val);
}

static DEVICE_ATTR(mic_adc, 0444 , earjack_mic_adc_show,
	NULL);

static void sec_jack_timer_handler(unsigned long data)
{
	struct sec_jack_info *hi = (struct sec_jack_info *)data;

	hi->buttons_enable = true;

}
/* thread run whenever the headset detect state changes (either insertion
 * or removal).
 */
static irqreturn_t sec_jack_detect_irq(int irq, void *dev_id)
{
	struct sec_jack_info *hi = dev_id;

	queue_work(hi->detect_wq, &hi->detect_work);

	return IRQ_HANDLED;
}

void sec_jack_detect_work(struct work_struct *work)
{
	struct sec_jack_info *hi =
		container_of(work, struct sec_jack_info, detect_work);
	struct sec_jack_platform_data *pdata = hi->pdata;
	unsigned npolarity = !hi->pdata->det_active_high;
	int time_left_ms;

	disable_irq(hi->det_irq);

	if (pdata->det_debounce_time_ms > 0)
		time_left_ms = pdata->det_debounce_time_ms;
	else
		time_left_ms = DEFAULT_DET_DEBOUNCE_TIME_MS;

	/* prevent suspend to allow user space to respond to switch */
	wake_lock_timeout(&hi->det_wake_lock, WAKE_LOCK_TIME);

	dev_info(&hi->client->dev, "%s: irq(%d)\n", __func__,
		gpio_get_value(pdata->det_gpio) ^ npolarity);

	/* debounce headset jack.  don't try to determine the type of
	 * headset until the detect state is true for a while.
	 */
	while (time_left_ms > 0) {
		if (!(gpio_get_value(hi->pdata->det_gpio) ^ npolarity)) {
			/* jack not detected. */
			handle_jack_not_inserted(hi);
			goto done;
		}
		usleep_range(10000, 11000);
		time_left_ms -= 10;
	}

	/* jack presence was detected the whole time, figure out which type */
	determine_jack_type(hi);
done:
	enable_irq(hi->det_irq);
}

/* thread run whenever the button of headset is pressed or released */
void sec_jack_buttons_work(struct work_struct *work)
{
	struct sec_jack_info *hi =
		container_of(work, struct sec_jack_info, buttons_work);
	struct sec_jack_platform_data *pdata = hi->pdata;
	struct sec_jack_buttons_zone *btn_zones = pdata->jack_buttons_zones;
	int num_buttons_zones = ARRAY_SIZE(pdata->jack_buttons_zones);
	int adc;
	int i;

	if (!hi->buttons_enable) {
		dev_info(&hi->client->dev, "key %d is skipped\n",
			hi->pressed_code);
		return;
	}
	/* prevent suspend to allow user space to respond to switch */
	wake_lock_timeout(&hi->det_wake_lock, WAKE_LOCK_TIME);

	/* when button is released */
	if (hi->pressed == 0) {
		input_report_key(hi->input_dev, hi->pressed_code, 0);
		input_sync(hi->input_dev);
		switch_set_state(&switch_sendend, 0);
		dev_info(&hi->client->dev, "key %d is released\n",
			hi->pressed_code);
		if (button_notify_cb)
			button_notify_cb(hi->pressed_code, 0);
		return;
	}

	/* when button is pressed */
	adc = sec_jack_get_adc_value(hi);

	if (adc < 0) {
		dev_err(&hi->client->dev, "Unavailable key ADC %d\n", adc);
		return;
	}

	for (i = 0; i < num_buttons_zones; i++)
		if (adc >= btn_zones[i].adc_low &&
			adc <= btn_zones[i].adc_high) {
			hi->pressed_code = btn_zones[i].code;
			input_report_key(hi->input_dev, btn_zones[i].code, 1);
			input_sync(hi->input_dev);
			switch_set_state(&switch_sendend, 1);
			dev_info(&hi->client->dev, "adc = %d, key %d is pressed\n",
				adc, btn_zones[i].code);
			if (button_notify_cb)
				button_notify_cb(btn_zones[i].code, true);
			return;
		}

	dev_warn(&hi->client->dev, "key skipped. ADC %d\n", adc);
}

#ifdef CONFIG_OF
static struct sec_jack_platform_data
	*sec_jack_populate_dt_pdata(struct device *dev)
{
	struct sec_jack_platform_data *pdata;
	struct of_phandle_args args;
	int i = 0;

	pdata = devm_kzalloc(dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata) {
		dev_err(dev, "Failed to allocate memory for pdata\n");
		goto alloc_err;
	}

	pdata->det_gpio = of_get_named_gpio(dev->of_node, "detect-gpio", 0);
	if (pdata->det_gpio < 0) {
		dev_err(dev, "Failed to find detect-gpio\n");
		goto of_parse_err;
	}

	pdata->key_gpio = of_get_named_gpio(dev->of_node, "key-gpio", 0);
	if (pdata->key_gpio < 0) {
		dev_err(dev, "Failed to find key-gpio\n");
		goto of_parse_err;
	}

	pdata->ear_micbias_en_gpio = of_get_named_gpio(dev->of_node,
		"ear-micbias-en-gpio", 0);
	if (pdata->ear_micbias_en_gpio < 0)
		dev_info(dev, "ear-micbias-en-gpio not found\n");

	if (of_property_read_u32(dev->of_node,
		"key-dbtime", &pdata->key_debounce_time_ms))
		dev_info(dev, "key-dbtime not found, use default\n");

	if (of_property_read_u32(dev->of_node,
		"det-dbtime", &pdata->det_debounce_time_ms))
		dev_info(dev, "det-dbtime not found, use default\n");

	for (i = 0; i < 4; i++) {
		of_parse_phandle_with_args(dev->of_node,
			"det-zones-list", "#list-det-cells", i, &args);
		pdata->jack_zones[i].adc_high = args.args[0];
		pdata->jack_zones[i].delay_us = args.args[1];
		pdata->jack_zones[i].check_count = args.args[2];
		pdata->jack_zones[i].jack_type = args.args[3];
		dev_dbg(dev, "det-zones-list %d, %d, %d, %d, %d\n",
				args.args_count, args.args[0],
				args.args[1], args.args[2], args.args[3]);
	}

	for (i = 0; i < 4; i++) {
		of_parse_phandle_with_args(dev->of_node,
			"but-zones-list", "#list-but-cells", i, &args);
		pdata->jack_buttons_zones[i].code = args.args[0];
		pdata->jack_buttons_zones[i].adc_low = args.args[1];
		pdata->jack_buttons_zones[i].adc_high = args.args[2];
		dev_dbg(dev, "but-zones-list %d, %d, %d, %d\n",
				args.args_count, args.args[0],
				args.args[1], args.args[2]);
	}

	if (of_find_property(dev->of_node, "det-active-high", NULL))
		pdata->det_active_high = true;

	if (of_find_property(dev->of_node, "send-end-active-high", NULL))
		pdata->send_end_active_high = true;

	return pdata;

of_parse_err:
	devm_kfree(dev, pdata);
alloc_err:
	return NULL;
}
#else
static struct sec_jack_platform_data
	*sec_jack_populate_dt_pdata(struct device *dev)
{
	return NULL;
}
#endif

static int sec_jack_get_pinctrl(struct sec_jack_platform_data *pdata,
	struct device *dev)
{
	pdata->jack_pinctrl = devm_pinctrl_get(dev);
	if (IS_ERR_OR_NULL(pdata->jack_pinctrl)) {
		if (PTR_ERR(pdata->jack_pinctrl) == -EPROBE_DEFER) {
			pdata->jack_pinctrl = NULL;
			return -EPROBE_DEFER;
		}
		dev_err(dev, "Target does not use pinctrl\n");
		goto pinctrl_fail;
	}

	pdata->jack_pins_active = pinctrl_lookup_state(pdata->jack_pinctrl,
					"earjack_gpio_active");
	if (IS_ERR(pdata->jack_pins_active)) {
		dev_err(dev, "could not get active pin state\n");
		goto pinctrl_fail;
	}

	return 0;

pinctrl_fail:
	pdata->jack_pinctrl = NULL;
	return -EINVAL;
}

static int sec_jack_probe(struct platform_device *pdev)
{
	struct sec_jack_info *hi;
	struct sec_jack_platform_data *pdata;
	struct class *audio;
	struct device *earjack;
	int ret;

	if (jack_controls.snd_card_registered != 1) {
		dev_info(&pdev->dev, "defer as sound card not registered\n");
		return -EPROBE_DEFER;
	}

	dev_info(&pdev->dev, "Registering sec_jack driver\n");

	if (dev_get_platdata(&pdev->dev))
		pdata = pdev->dev.platform_data;
	else
		pdata = sec_jack_populate_dt_pdata(&pdev->dev);

	if (!pdata) {
		dev_err(&pdev->dev, "pdata is NULL\n");
		return -ENODEV;
	}

	ret = sec_jack_get_pinctrl(pdata, &pdev->dev);
	if (ret) {
		dev_err(&pdev->dev, "cannot config earjack pinctrl\n");
		return -ENODEV;
	}

	ret = sec_jack_gpio_init(pdata);
	if (ret) {
		dev_err(&pdev->dev, "Failed to init gpio(%d)\n", ret);
		return -ENODEV;
	}

	if (atomic_xchg(&instantiated, 1)) {
		dev_err(&pdev->dev, "already instantiated, can only have one\n");
		return -ENODEV;
	}

	sec_jack_key_map[0].gpio = pdata->key_gpio;

	pdata->jack_controls = &jack_controls;

	hi = kzalloc(sizeof(struct sec_jack_info), GFP_KERNEL);
	if (hi == NULL) {
		dev_err(&pdev->dev, "Failed to allocate memory\n");
		ret = -ENOMEM;
		goto err_kzalloc;
	}

	hi->pdata = pdata;

	/* make the id of our gpi_event device the same as our platform device,
	 * which makes it the responsiblity of the board file to make sure
	 * it is unique relative to other gpio_event devices
	 */
	hi->dev_id = pdev->id;
	/* to read the vadc */
	hi->client = pdev;

	ret = switch_dev_register(&switch_jack_detection);
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to register h2w switch device\n");
		goto err_h2w_dev_register;
	}

	ret = switch_dev_register(&switch_sendend);
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to register key switch device\n");
		goto err_send_end_dev_register;
	}

	wake_lock_init(&hi->det_wake_lock, WAKE_LOCK_SUSPEND, "sec_jack_det");

	setup_timer(&hi->timer, sec_jack_timer_handler, (unsigned long)hi);

	INIT_WORK(&hi->detect_work, sec_jack_detect_work);
	INIT_WORK(&hi->buttons_work, sec_jack_buttons_work);

	hi->detect_wq = create_singlethread_workqueue("detect_wq");
	if (hi->detect_wq == NULL) {
		ret = -ENOMEM;
		dev_err(&pdev->dev, "Failed to create detect_wq\n");
		goto err_create_detect_wq_failed;
	}

	hi->buttons_wq = create_singlethread_workqueue("buttons_wq");
	if (hi->buttons_wq == NULL) {
		ret = -ENOMEM;
		dev_err(&pdev->dev, "Failed to create buttons_wq\n");
		goto err_create_buttons_wq_failed;
	}

	hi->det_irq = gpio_to_irq(pdata->det_gpio);

	set_bit(EV_KEY, hi->ids[0].evbit);
	hi->ids[0].flags = INPUT_DEVICE_ID_MATCH_EVBIT;
	hi->jack_handler.filter = sec_jack_buttons_filter;
	hi->jack_handler.connect = sec_jack_buttons_connect;
	hi->jack_handler.disconnect = sec_jack_buttons_disconnect;
	hi->jack_handler.name = "sec_jack_buttons";
	hi->jack_handler.id_table = hi->ids;
	hi->jack_handler.private = hi;

	ret = input_register_handler(&hi->jack_handler);
	if (ret) {
		dev_err(&pdev->dev, "Failed to input_register_handler\n");
		goto err_input_register_handler;
	}

	/* We are going to remove this code later */
	if (pdata->send_end_active_high == true)
		sec_jack_key_info.flags = 1;

	if (pdata->key_debounce_time_ms > 0)
		sec_jack_key_info.debounce_time.tv64
				= pdata->key_debounce_time_ms * NSEC_PER_MSEC;

	ret = request_irq(hi->det_irq, sec_jack_detect_irq,
		IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING |
		IRQF_ONESHOT, "sec_headset_detect", hi);
	if (ret) {
		dev_err(&pdev->dev, "Failed to request_irq\n");
		goto err_request_detect_irq;
	}

	/* to handle insert/removal when we're sleeping in a call */
	ret = enable_irq_wake(hi->det_irq);
	if (ret) {
		dev_err(&pdev->dev, "Failed to enable_irq_wake\n");
		goto err_enable_irq_wake;
	}

	dev_set_drvdata(&pdev->dev, hi);

	/* Create sysfs to support PBA test */
	audio = class_create(THIS_MODULE, "audio");
	if (IS_ERR(audio)) {
		dev_err(&pdev->dev, "Failed to create class audio\n");
		ret = -ENODEV;
		goto err_class_create;
	}

	earjack = device_create(audio, NULL, 0, NULL, "earjack");
	if (IS_ERR(earjack)) {
		dev_err(&pdev->dev, "Failed to create device earjack\n");
		ret = -ENODEV;
		goto err_device_create;
	}

	ret = device_create_file(earjack, &dev_attr_key_state);
	if (ret) {
		dev_err(&pdev->dev, "Failed to create device file in %s\n",
			dev_attr_key_state.attr.name);
		goto err_dev_attr_key_state;
	}

	ret = device_create_file(earjack, &dev_attr_state);
	if (ret) {
		dev_err(&pdev->dev, "Failed to create device file in %s\n",
			dev_attr_state.attr.name);
		goto err_dev_attr_state;
	}

	ret = device_create_file(earjack, &dev_attr_mic_adc);
	if (ret) {
		dev_err(&pdev->dev, "Failed to create device file in %s\n",
			dev_attr_mic_adc.attr.name);
		goto err_dev_attr_mic_adc;
	}
	dev_set_drvdata(earjack, hi);

	queue_work(hi->detect_wq, &hi->detect_work);

	dev_info(&pdev->dev, "Registering sec_jack driver\n");
	return 0;
err_dev_attr_mic_adc:
	device_remove_file(earjack, &dev_attr_state);
err_dev_attr_state:
	device_remove_file(earjack, &dev_attr_key_state);
err_dev_attr_key_state:
	device_destroy(audio, 0);
err_device_create:
	class_destroy(audio);
err_class_create:
	disable_irq_wake(hi->det_irq);
err_enable_irq_wake:
	free_irq(hi->det_irq, hi);
err_request_detect_irq:
	input_unregister_handler(&hi->jack_handler);
err_input_register_handler:
	destroy_workqueue(hi->buttons_wq);
err_create_buttons_wq_failed:
	destroy_workqueue(hi->detect_wq);
err_create_detect_wq_failed:
	wake_lock_destroy(&hi->det_wake_lock);
	switch_dev_unregister(&switch_sendend);
err_send_end_dev_register:
	switch_dev_unregister(&switch_jack_detection);
err_h2w_dev_register:
	kfree(hi);
err_kzalloc:
	atomic_set(&instantiated, 0);

	return ret;
}

static int sec_jack_remove(struct platform_device *pdev)
{

	struct sec_jack_info *hi = dev_get_drvdata(&pdev->dev);

	disable_irq_wake(hi->det_irq);
	free_irq(hi->det_irq, hi);
	destroy_workqueue(hi->detect_wq);
	destroy_workqueue(hi->buttons_wq);
	if (hi->send_key_dev) {
		platform_device_unregister(hi->send_key_dev);
		hi->send_key_dev = NULL;
	}
	input_unregister_handler(&hi->jack_handler);
	wake_lock_destroy(&hi->det_wake_lock);
	switch_dev_unregister(&switch_jack_detection);
	switch_dev_unregister(&switch_sendend);
	gpio_free(hi->pdata->det_gpio);
	if (hi->pdata->ear_micbias_en_gpio)
		gpio_free(hi->pdata->ear_micbias_en_gpio);
	hi->pdata->jack_pinctrl = NULL;
	kfree(hi);
	atomic_set(&instantiated, 0);

	return 0;
}

static const struct of_device_id sec_jack_dt_match[] = {
	{ .compatible = "sec_jack" },
	{ }
};
MODULE_DEVICE_TABLE(of, sec_jack_dt_match);

static struct platform_driver sec_jack_driver = {
	.probe = sec_jack_probe,
	.remove = sec_jack_remove,
	.driver = {
		.name = "sec_jack",
		.owner = THIS_MODULE,
		.of_match_table = sec_jack_dt_match,
	},
};

static int __init sec_jack_init(void)
{
	return platform_driver_register(&sec_jack_driver);
}

static void __exit sec_jack_exit(void)
{
	platform_driver_unregister(&sec_jack_driver);
}

late_initcall(sec_jack_init);
module_exit(sec_jack_exit);

MODULE_DESCRIPTION("Samsung Electronics Extcon driver");
MODULE_LICENSE("GPL");
