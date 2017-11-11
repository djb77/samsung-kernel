/*
 * Copyright (C) 2011 Samsung Electronics Co. Ltd.
 *  Inchul Im <inchul.im@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#define pr_fmt(fmt) "usb_notifier: " fmt

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/usb_notify.h>
#ifdef CONFIG_OF
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#endif
#ifdef CONFIG_CCIC_NOTIFIER
#include <linux/muic/muic.h>
#include <linux/ccic/ccic_notifier.h>
#else
#ifdef CONFIG_MUIC_NOTIFIER
#include <linux/muic/muic.h>
#include <linux/muic/muic_notifier.h>
#endif
#endif
#ifdef CONFIG_VBUS_NOTIFIER
#include <linux/vbus_notifier.h>
#endif
#include <linux/battery/sec_charging_common.h>
#include "usb_notifier.h"

struct usb_notifier_platform_data {
	struct	notifier_block usb_nb;
	struct	notifier_block vbus_nb;
	int	gpio_redriver_en;
	int can_diable_usb;
	int is_host;
};

#ifdef CONFIG_OF
static void of_get_usb_redriver_dt(struct device_node *np,
		struct usb_notifier_platform_data *pdata)
{
	int gpio = 0;

	gpio = of_get_named_gpio(np, "gpios_redriver_en", 0);
	if (!gpio_is_valid(gpio)) {
		pdata->gpio_redriver_en = -1;
		pr_err("%s: usb30_redriver_en: Invalied gpio pins\n", __func__);
	} else
		pdata->gpio_redriver_en = gpio;

	pr_info("%s, gpios_redriver_en %d\n", __func__, gpio);

	pdata->can_diable_usb =
		!(of_property_read_bool(np, "samsung,unsupport-disable-usb"));
	pr_info("%s, can_diable_usb %d\n", __func__, pdata->can_diable_usb);
	return;
}

static int of_usb_notifier_dt(struct device *dev,
		struct usb_notifier_platform_data *pdata)
{
	struct device_node *np = dev->of_node;

	if (!np)
		return -EINVAL;

	of_get_usb_redriver_dt(np, pdata);
	return 0;
}
#endif

static struct device_node *exynos_udc_parse_dt(void)
{
	struct platform_device *pdev = NULL;
	struct device *dev = NULL;
	struct device_node *np = NULL;

	/**
	 * For previous chips such as Exynos7420 and Exynos7890
	 */
	np = of_find_compatible_node(NULL, NULL, "samsung,exynos5-dwusb3");
	if (np)
		return np;

	np = of_find_compatible_node(NULL, NULL, "samsung,usb-notifier");
	if (!np) {
		pr_err("%s: failed to get the usb-notifier device node\n",
			__func__);
		return NULL;
	}

	pdev = of_find_device_by_node(np);
	if (!pdev) {
		pr_err("%s: failed to get platform_device\n", __func__);
		return NULL;
	}

	dev = &pdev->dev;
	np = of_parse_phandle(dev->of_node, "udc", 0);
	if (!np) {
		dev_info(dev, "udc device is not available\n");
		return NULL;
	}

	return np;
}

static void check_usb_vbus_state(int state)
{
	struct device_node *np = NULL;
	struct platform_device *pdev = NULL;

	pr_info("%s vbus state = %d\n", __func__, state);

	np = exynos_udc_parse_dt();
	if (np) {
		pdev = of_find_device_by_node(np);
		of_node_put(np);
		if (pdev) {
			pr_info("%s: get the %s platform_device\n",
				__func__, pdev->name);
			dwc3_exynos_vbus_event(&pdev->dev, state);
			goto end;
		}
	}

	pr_err("%s: failed to get the platform_device\n", __func__);
end:
	return;
}

static void check_usb_id_state(int state)
{
	struct device_node *np = NULL;
	struct platform_device *pdev = NULL;

	pr_info("%s id state = %d\n", __func__, state);

	np = exynos_udc_parse_dt();
	if (np) {
		pdev = of_find_device_by_node(np);
		of_node_put(np);
		if (pdev) {
			pr_info("%s: get the %s platform_device\n",
			__func__, pdev->name);
			dwc3_exynos_id_event(&pdev->dev, state);
			goto end;
		}
	}
	pr_err("%s: failed to get the platform_device\n", __func__);
end:
	return;
}

#ifdef CONFIG_CCIC_NOTIFIER
static int usb_handle_notification(struct notifier_block *nb,
		unsigned long action, void *data)
{
	CC_NOTI_ATTACH_TYPEDEF noti = *(CC_NOTI_ATTACH_TYPEDEF *)data;
	struct otg_notify *o_notify;
	int mdev = ATTACHED_DEV_NONE_MUIC;
	struct usb_notifier_platform_data *pdata =
		container_of(nb, struct usb_notifier_platform_data, usb_nb);

	o_notify = get_otg_notify();

	pr_info("%s : ATTACH : %d, ID : %d, SRC : %d, DEST : %d, RPRD : %d\n",
		__func__, noti.attach, noti.id, noti.src, noti.dest, noti.rprd);

	if(noti.attach == CCIC_NOTIFY_ATTACH && noti.rprd == CCIC_NOTIFY_HOST) {
		pr_info("%s: Turn On OTG\n", __func__);
		mdev = ATTACHED_DEV_OTG_MUIC;
		pdata->is_host= 1;
	} else if(pdata->is_host){
		pr_info("%s: Turn Off OTG\n", __func__);
		mdev = ATTACHED_DEV_OTG_MUIC;
		pdata->is_host = 0;
	} else if(noti.rprd == CCIC_NOTIFY_DEVICE) {
		pr_info("%s: DEVICE\n", __func__);
		msleep(1500);
		if (noti.attach == CCIC_NOTIFY_ATTACH)
			send_otg_notify(o_notify, NOTIFY_EVENT_VBUS, 1);
		else if (noti.attach == CCIC_NOTIFY_DETACH)
			send_otg_notify(o_notify, NOTIFY_EVENT_VBUS, 0);
		else
			pr_err("%s - ACTION Error!\n", __func__);
	} else {
		pr_info("%s: default vallue\n", __func__);
	}

	switch (mdev) {
	case ATTACHED_DEV_OTG_MUIC:
		if (noti.attach == CCIC_NOTIFY_DETACH)
			send_otg_notify(o_notify, NOTIFY_EVENT_HOST, 0);
		else if (noti.attach == CCIC_NOTIFY_ATTACH)
			send_otg_notify(o_notify, NOTIFY_EVENT_HOST, 1);
		else
			pr_err("%s - ACTION Error!\n", __func__);
		break;
	default:
		break;
	}

	return 0;
}
#else
#ifdef CONFIG_MUIC_NOTIFIER
static int usb_handle_notification(struct notifier_block *nb,
		unsigned long action, void *data)
{
	muic_attached_dev_t attached_dev = *(muic_attached_dev_t *)data;
	struct otg_notify *o_notify;

	o_notify = get_otg_notify();

	pr_info("%s action=%lu, attached_dev=%d\n",
		__func__, action, attached_dev);

	switch (attached_dev) {
	case ATTACHED_DEV_USB_MUIC:
	case ATTACHED_DEV_CDP_MUIC:
	case ATTACHED_DEV_UNOFFICIAL_ID_USB_MUIC:
	case ATTACHED_DEV_UNOFFICIAL_ID_CDP_MUIC:
	case ATTACHED_DEV_JIG_USB_OFF_MUIC:
	case ATTACHED_DEV_JIG_USB_ON_MUIC:
		if (action == MUIC_NOTIFY_CMD_DETACH)
			send_otg_notify(o_notify, NOTIFY_EVENT_VBUS, 0);
		else if (action == MUIC_NOTIFY_CMD_ATTACH)
			send_otg_notify(o_notify, NOTIFY_EVENT_VBUS, 1);
		else
			pr_err("%s - ACTION Error!\n", __func__);
		break;
	case ATTACHED_DEV_OTG_MUIC:
		if (action == MUIC_NOTIFY_CMD_DETACH)
			send_otg_notify(o_notify, NOTIFY_EVENT_HOST, 0);
		else if (action == MUIC_NOTIFY_CMD_ATTACH)
			send_otg_notify(o_notify, NOTIFY_EVENT_HOST, 1);
		else
			pr_err("%s - ACTION Error!\n", __func__);
		break;
	case ATTACHED_DEV_HMT_MUIC:
		if (action == MUIC_NOTIFY_CMD_DETACH)
			send_otg_notify(o_notify, NOTIFY_EVENT_HMT, 0);
		else if (action == MUIC_NOTIFY_CMD_ATTACH)
			send_otg_notify(o_notify, NOTIFY_EVENT_HMT, 1);
		else
			pr_err("%s - ACTION Error!\n", __func__);
		break;
	case ATTACHED_DEV_JIG_UART_OFF_VB_OTG_MUIC:
		if (action == MUIC_NOTIFY_CMD_DETACH)
			pr_info("%s - USB_HOST_TEST_DETACHED\n", __func__);
		else if (action == MUIC_NOTIFY_CMD_ATTACH)
			pr_info("%s - USB_HOST_TEST_ATTACHED\n", __func__);
		else
			pr_err("%s - ACTION Error!\n", __func__);
		break;
	case ATTACHED_DEV_SMARTDOCK_TA_MUIC:
		if (action == MUIC_NOTIFY_CMD_DETACH)
			send_otg_notify(o_notify, NOTIFY_EVENT_SMARTDOCK_TA, 0);
		else if (action == MUIC_NOTIFY_CMD_ATTACH)
			send_otg_notify(o_notify, NOTIFY_EVENT_SMARTDOCK_TA, 1);
		else
			pr_err("%s - ACTION Error!\n", __func__);
		break;
	case ATTACHED_DEV_SMARTDOCK_USB_MUIC:
		if (action == MUIC_NOTIFY_CMD_DETACH)
			send_otg_notify
				(o_notify, NOTIFY_EVENT_SMARTDOCK_USB, 0);
		else if (action == MUIC_NOTIFY_CMD_ATTACH)
			send_otg_notify
				(o_notify, NOTIFY_EVENT_SMARTDOCK_USB, 1);
		else
			pr_err("%s - ACTION Error!\n", __func__);
		break;
	case ATTACHED_DEV_AUDIODOCK_MUIC:
		if (action == MUIC_NOTIFY_CMD_DETACH)
			send_otg_notify(o_notify, NOTIFY_EVENT_AUDIODOCK, 0);
		else if (action == MUIC_NOTIFY_CMD_ATTACH)
			send_otg_notify(o_notify, NOTIFY_EVENT_AUDIODOCK, 1);
		else
			pr_err("%s - ACTION Error!\n", __func__);
		break;
	case ATTACHED_DEV_UNIVERSAL_MMDOCK_MUIC:
		if (action == MUIC_NOTIFY_CMD_DETACH)
			send_otg_notify(o_notify, NOTIFY_EVENT_MMDOCK, 0);
		else if (action == MUIC_NOTIFY_CMD_ATTACH)
			send_otg_notify(o_notify, NOTIFY_EVENT_MMDOCK, 1);
		else
			pr_err("%s - ACTION Error!\n", __func__);
		break;
	case ATTACHED_DEV_GAMEPAD_MUIC:
		if (action == MUIC_NOTIFY_CMD_DETACH) {
			send_otg_notify(o_notify, NOTIFY_EVENT_GAMEPAD, 0);
		} else if (action == MUIC_NOTIFY_CMD_ATTACH) {
			send_otg_notify(o_notify, NOTIFY_EVENT_GAMEPAD, 1);
		} else
			pr_err("%s - ACTION Error!\n", __func__);
		break;
	default:
		break;
	}

	return 0;
}
#endif
#endif
#ifdef CONFIG_VBUS_NOTIFIER
static int vbus_handle_notification(struct notifier_block *nb,
		unsigned long cmd, void *data)
{
	vbus_status_t vbus_type = *(vbus_status_t *)data;
	struct otg_notify *o_notify;

	o_notify = get_otg_notify();

	pr_info("%s cmd=%lu, vbus_type=%s\n",
		__func__, cmd, vbus_type == STATUS_VBUS_HIGH ? "HIGH" : "LOW");

	switch (vbus_type) {
	case STATUS_VBUS_HIGH:
		send_otg_notify(o_notify, NOTIFY_EVENT_VBUSPOWER, 1);
		break;
	case STATUS_VBUS_LOW:
		send_otg_notify(o_notify, NOTIFY_EVENT_VBUSPOWER, 0);
		break;
	default:
		break;
	}
	return 0;
}
#endif

static int otg_accessory_power(bool enable)
{
	u8 on = (u8)!!enable;
	union power_supply_propval val;

	pr_info("otg accessory power = %d\n", on);

	val.intval = enable;
	psy_do_property("otg", set,
			POWER_SUPPLY_PROP_ONLINE, val);

	return 0;
}

static int set_online(int event, int state)
{
	union power_supply_propval val;
	struct device_node *np_charger = NULL;
	char *charger_name;

	if (event == NOTIFY_EVENT_SMTD_EXT_CURRENT)
		pr_info("request smartdock charging current = %s\n",
			state ? "1000mA" : "1700mA");
	else if (event == NOTIFY_EVENT_MMD_EXT_CURRENT)
		pr_info("request mmdock charging current = %s\n",
			state ? "900mA" : "1400mA");

	np_charger = of_find_node_by_name(NULL, "battery");
	if (!np_charger) {
		pr_err("%s: failed to get the battery device node\n", __func__);
		return 0;
	} else {
		if (!of_property_read_string(np_charger, "battery,charger_name",
					(char const **)&charger_name)) {
			pr_info("%s: charger_name = %s\n", __func__,
					charger_name);
		} else {
			pr_err("%s: failed to get the charger name\n",
								 __func__);
			return 0;
		}
	}

	if (state)
		val.intval = POWER_SUPPLY_TYPE_SMART_OTG;
	else
		val.intval = POWER_SUPPLY_TYPE_SMART_NOTG;

	psy_do_property(charger_name, set,
			POWER_SUPPLY_PROP_ONLINE, val);

	return 0;
}

static int exynos_set_host(bool enable)
{
	if (!enable) {
		pr_info("%s USB_HOST_DETACHED\n", __func__);
#ifdef CONFIG_OF
		check_usb_id_state(1);
#endif
	} else {
		pr_info("%s USB_HOST_ATTACHED\n", __func__);
#ifdef CONFIG_OF
		check_usb_id_state(0);
#endif
	}

	return 0;
}

extern void set_ncm_ready(bool ready);
static int exynos_set_peripheral(bool enable)
{
	if (enable) {
		pr_info("%s usb attached\n", __func__);
		check_usb_vbus_state(1);
	} else {
		pr_info("%s usb detached\n", __func__);
		check_usb_vbus_state(0);
		set_ncm_ready(false);
	}
	return 0;
}

static struct otg_notify dwc_lsi_notify = {
	.vbus_drive	= otg_accessory_power,
	.set_host = exynos_set_host,
	.set_peripheral	= exynos_set_peripheral,
	.vbus_detect_gpio = -1,
	.is_wakelock = 1,
	.booting_delay_sec = 10,
	.auto_drive_vbus = 1,
	.disable_control = 1,
	.device_check_sec = 3,
	.set_battcall = set_online,
};

static int usb_notifier_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct usb_notifier_platform_data *pdata = NULL;

	if (pdev->dev.of_node) {
		pdata = devm_kzalloc(&pdev->dev,
			sizeof(struct usb_notifier_platform_data), GFP_KERNEL);
		if (!pdata) {
			dev_err(&pdev->dev, "Failed to allocate memory\n");
			return -ENOMEM;
		}

		ret = of_usb_notifier_dt(&pdev->dev, pdata);
		if (ret < 0) {
			dev_err(&pdev->dev, "Failed to get device of_node\n");
			return ret;
		}

		pdev->dev.platform_data = pdata;
	} else
		pdata = pdev->dev.platform_data;

	dwc_lsi_notify.redriver_en_gpio = pdata->gpio_redriver_en;
	dwc_lsi_notify.disable_control = pdata->can_diable_usb;
	set_otg_notify(&dwc_lsi_notify);
	set_notify_data(&dwc_lsi_notify, pdata);
#ifdef CONFIG_CCIC_NOTIFIER
	pdata->is_host = 0;
	ccic_notifier_register(&pdata->usb_nb, usb_handle_notification,
				   CCIC_NOTIFY_DEV_USB);
	vbus_notifier_register(&pdata->vbus_nb, vbus_handle_notification,
			       CCIC_NOTIFY_DEV_USB);
#else
#ifdef CONFIG_MUIC_NOTIFIER
	muic_notifier_register(&pdata->usb_nb, usb_handle_notification,
			       MUIC_NOTIFY_DEV_USB);
#endif
#ifdef CONFIG_VBUS_NOTIFIER
	vbus_notifier_register(&pdata->vbus_nb, vbus_handle_notification,
			       MUIC_NOTIFY_DEV_USB);
#endif
#endif	/* CONFIG_CCIC_NOTIFIER*/

	dev_info(&pdev->dev, "usb notifier probe\n");
	return 0;
}

static int usb_notifier_remove(struct platform_device *pdev)
{
	struct usb_notifier_platform_data *pdata = dev_get_platdata(&pdev->dev);
#ifdef CONFIG_CCIC_NOTIFIER
	ccic_notifier_unregister(&pdata->usb_nb);
#else
#ifdef CONFIG_MUIC_NOTIFIER
	muic_notifier_unregister(&pdata->usb_nb);
#endif
#endif
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id usb_notifier_dt_ids[] = {
	{ .compatible = "samsung,usb-notifier",
	},
	{ },
};
MODULE_DEVICE_TABLE(of, usb_notifier_dt_ids);
#endif

static struct platform_driver usb_notifier_driver = {
	.probe		= usb_notifier_probe,
	.remove		= usb_notifier_remove,
	.driver		= {
		.name	= "usb_notifier",
		.owner	= THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table	= of_match_ptr(usb_notifier_dt_ids),
#endif
	},
};

static int __init usb_notifier_init(void)
{
	return platform_driver_register(&usb_notifier_driver);
}

static void __init usb_notifier_exit(void)
{
	platform_driver_unregister(&usb_notifier_driver);
}

late_initcall(usb_notifier_init);
module_exit(usb_notifier_exit);

MODULE_AUTHOR("inchul.im <inchul.im@samsung.com>");
MODULE_DESCRIPTION("USB notifier");
MODULE_LICENSE("GPL");
