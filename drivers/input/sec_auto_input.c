/*
 * drivers/pinctrl/sec_auto_input.c
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#include "sec_auto_input.h"

struct auto_input_data *g_auto_input_data;

static int tsp_input_register(struct auto_input_data *data){
	int ret;
	pr_info("%s\n", __func__);

	data->input_dev = input_allocate_device();
	if (data->input_dev == NULL) {
		pr_err("%s: Failed to allocate input device\n",
				__func__);
		return -ENOMEM;
	}

	data->input_dev->name = "sec_auto_input";

	input_set_drvdata(data->input_dev, data);

	set_bit(EV_SYN, data->input_dev->evbit);
	set_bit(EV_KEY, data->input_dev->evbit);
	set_bit(EV_ABS, data->input_dev->evbit);
	set_bit(BTN_TOUCH, data->input_dev->keybit);
	set_bit(BTN_TOOL_FINGER, data->input_dev->keybit);

	// Touchkey
	set_bit(KEY_RECENT, data->input_dev->keybit);
	set_bit(KEY_BACK, data->input_dev->keybit);
	
	if(!data->max_x) data->max_x = DEFAULT_MAX_X;
	if(!data->max_y) data->max_y = DEFAULT_MAX_Y;

	input_set_abs_params(data->input_dev,
			ABS_MT_POSITION_X, 0,
			data->max_x - 1, 0, 0);
	input_set_abs_params(data->input_dev,
			ABS_MT_POSITION_Y, 0,
			data->max_y - 1, 0, 0);

	ret = input_mt_init_slots(data->input_dev,
			FINGER_NUM, INPUT_MT_DIRECT);
	if (ret < 0) {
		pr_err("%s: Failed to input_mt_init_slots, ret:%d\n",
				__func__, ret);
		goto input_err;
	}

	ret = input_register_device(data->input_dev);
	if (ret) {
		pr_err("%s: Failed to register input device\n",
				__func__);
		goto input_err;
	}

	return 0;

input_err:
	input_free_device(data->input_dev);
	return ret;
}

void auto_input_report(struct auto_input_data *data, unsigned int type, unsigned int code, int value){

	if(data->sync_end){
		data->sync_end = false;
		if(code != ABS_MT_SLOT) input_report_abs(data->input_dev, ABS_MT_SLOT, data->prev_slot);
	}

	switch(type){
		case EV_SYN:
			data->sync_end = true;
			input_sync(data->input_dev);
			break;
		case EV_KEY:
			input_report_key(data->input_dev, code, value);
			break;
		case EV_REL:
			input_report_rel(data->input_dev, code, value);
			break;
		case EV_ABS:
			if(code == ABS_MT_SLOT) {
				data->prev_slot = value;
				data->sync_end = false;
			}
			input_report_abs(data->input_dev, code, value);
			break;
		case EV_DELAY:
			msleep(value);
			break;
		default:
			pr_err("%s: unsupported EV_TYPE(%x)\n", __func__, type);
			break;
	}
}

static ssize_t auto_input_enable_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct auto_input_data *data = dev_get_drvdata(dev);

	if (sscanf(buf, "%4d %4d", &data->max_x, &data->max_y) != 2) {
		pr_err("%s: sscanf error\n", __func__);
		return -EINVAL;
	}

	pr_info("%s: enabled(%d) : (%d, %d)\n", __func__, data->enabled, data->max_x, data->max_y);

	if(!data->enabled){
		data->enabled = true;
		tsp_input_register(data);
	}

	return count;
}

static ssize_t auto_input_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct auto_input_data *data = dev_get_drvdata(dev);

	pr_info("%s: enable(%d) : (%d, %d)\n", __func__, data->enabled, data->max_x, data->max_y);

	return snprintf(buf, PAGE_SIZE, "%d %d %d\n",
			data->enabled, data->max_x, data->max_y);
}

/**
* 1. event format
*   type code value
*   0000 0000 00000000
*   ex) EV_ABS ABS_MT_POSITION_X 00000140
*        0003 0035 00000140
* 2. delay
*   ex) delay 100ms
*       00ff 00000000 00000100
*/
static ssize_t auto_input_tsp_event_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct auto_input_data *data = dev_get_drvdata(dev);
	int pos = 0;
	int offset;
	int buf_len = strlen(buf);
	unsigned int type, code;
	int value;

	if(buf_len >= BUF_MAX){
		pr_err("%s: buffer overflow\n", __func__);
		return -EINVAL;
	}

	if (data->enabled){
		while(1){
			if (sscanf(buf+pos, "%4x %4x %8x %n", &type, &code, &value, &offset) != 3) {
				break;
			}
			pos += offset;
			auto_input_report(data, type, code, value);
		}
	}

	return count;
}

static DEVICE_ATTR(enable, S_IRUSR | S_IWUSR, auto_input_enable_show, auto_input_enable_store);
static DEVICE_ATTR(tsp_event, S_IWUSR, NULL, auto_input_tsp_event_store);

static struct attribute *auto_input_attributes[] = {
	&dev_attr_enable.attr,
	&dev_attr_tsp_event.attr,
	NULL,
};

static struct attribute_group auto_input_attr_group = {
	.attrs = auto_input_attributes,
};


static int __init auto_input_init(void)
{
	struct auto_input_data *data;
	int ret;

	data = kzalloc(sizeof(struct auto_input_data), GFP_KERNEL);
	if (!data) {
		pr_err("%s: Failed to allocate data\n", __func__);
		return -ENOMEM;
	}

	data->dev = sec_device_create(NULL, "sec_auto_input");
	if (IS_ERR(data->dev)) {
		pr_err("%s: Failed to create device!\n", __func__);
		ret = PTR_ERR(data->dev);
		goto err_sec_device_create;
	}

	g_auto_input_data = data;
	dev_set_drvdata(data->dev, data);

	ret = sysfs_create_group(&data->dev->kobj, &auto_input_attr_group);
	if (ret < 0) {
		pr_err("%s: Failed to create sysfs group\n", __func__);
		goto err_sysfs_create_group;
	}
	
	data->enabled = false;
	data->max_x = DEFAULT_MAX_X;
	data->max_y = DEFAULT_MAX_Y;
	data->prev_slot = 0;
	data->sync_end = true;

	pr_info("%s: Success\n", __func__);

	return 0;

err_sysfs_create_group:
	sec_device_destroy(data->dev->devt);
err_sec_device_create:
	kfree(data);

	return ret;
}

static void __exit auto_input_exit(void)
{
}

module_init(auto_input_init);
module_exit(auto_input_exit);

MODULE_DESCRIPTION("Samsung auto input driver");
MODULE_AUTHOR("Sangsu Ha <sangsu.ha@samsung.com>");
MODULE_LICENSE("GPL");
