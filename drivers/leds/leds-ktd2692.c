/*
 * LED driver - leds-ktd2692.c
 *
 * Copyright (C) 2013 Sunggeun Yim <sunggeun.yim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/pwm.h>
#include <linux/vmalloc.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/videodev2.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <linux/of_gpio.h>
#include <linux/leds-ktd2692.h>
#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_gpio.h>
#endif

extern struct class *camera_class; /*sys/class/camera*/
#if defined(CONFIG_LEDS_KTD2692_FOR_FRONT)
extern struct device *flash_dev;
#else
struct device *flash_dev;
#endif
struct ktd2692_platform_data *global_ktd2692data;
struct device *ktd2692_dev;

#if defined(CONFIG_ACTIVE_FLASH)
// Tuning factor
#if defined(CONFIG_SEC_GTA2SLTE_PROJECT)||defined(CONFIG_SEC_GTA2SWIFI_PROJECT)
static int torchlevel[] = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09};
#else
static int torchlevel[] = {0x00,0x01,0x01,0x02,0x02,0x03,0x04,0x05,0x06,0x08};
#endif
#endif
#if !(defined CONFIG_SEC_J3Y17QLTE_PROJECT || defined  CONFIG_SEC_J2Y18LTE_PROJECT)
int assistive_light = 0;
#endif
void ktd2692_setGpio(int onoff)
{
	if (onoff) {
		__gpio_set_value(global_ktd2692data->flash_control, 1);
	} else {
		__gpio_set_value(global_ktd2692data->flash_control, 0);
	}
}
void ktd2692_set_low_bit(void)
{
	__gpio_set_value(global_ktd2692data->flash_control, 0);
#if defined(CONFIG_SEC_J3Y17QLTE_PROJECT)
	ndelay(T_L_LB*1000);	/* 12ms */
#else
	udelay(T_L_LB);	/* 80ms */
#endif
	__gpio_set_value(global_ktd2692data->flash_control, 1);
#if defined(CONFIG_SEC_J3Y17QLTE_PROJECT)
	ndelay(T_H_LB*1000);	/* 4ms */
#else
	udelay(T_H_LB);	/* 5ms */
#endif
}
void ktd2692_set_high_bit(void)
{
	__gpio_set_value(global_ktd2692data->flash_control, 0);
#if defined(CONFIG_SEC_J3Y17QLTE_PROJECT)
	ndelay(T_L_HB*1000);	/* 4ms */
#else
	udelay(T_L_HB);	/* 5ms */
#endif
	__gpio_set_value(global_ktd2692data->flash_control, 1);
#if defined(CONFIG_SEC_J3Y17QLTE_PROJECT)
	ndelay(T_H_HB*1000);	/* 12ms */
#else
	udelay(T_H_HB);	/* 80ms */
#endif
}
static int ktd2692_set_bit(unsigned int bit)
{
	if (bit) {
		ktd2692_set_high_bit();
	} else {
		ktd2692_set_low_bit();
	}
	return 0;
}
int ktd2692_write_data(unsigned data)
{
	int err = 0;
	unsigned int bit = 0;
	/* Data Start Condition */
	__gpio_set_value(global_ktd2692data->flash_control, 1);
	ndelay(T_SOD*1000); //15us
	/* BIT 7*/
	bit = ((data>> 7) & 0x01);
	ktd2692_set_bit(bit);
	/* BIT 6 */
	bit = ((data>> 6) & 0x01);
	ktd2692_set_bit(bit);
	/* BIT 5*/
	bit = ((data>> 5) & 0x01);
	ktd2692_set_bit(bit);
	/* BIT 4 */
	bit = ((data>> 4) & 0x01);
	ktd2692_set_bit(bit);
	/* BIT 3*/
	bit = ((data>> 3) & 0x01);
	ktd2692_set_bit(bit);
	/* BIT 2 */
	bit = ((data>> 2) & 0x01);
	ktd2692_set_bit(bit);
	/* BIT 1*/
	bit = ((data>> 1) & 0x01);
	ktd2692_set_bit(bit);
	/* BIT 0 */
	bit = ((data>> 0) & 0x01);
	ktd2692_set_bit(bit);
	 __gpio_set_value(global_ktd2692data->flash_control, 0);
	ndelay(T_EOD_L*1000); //4us
	/* Data End Condition */
	__gpio_set_value(global_ktd2692data->flash_control, 1);
	udelay(T_EOD_H);

	return err;
}

EXPORT_SYMBOL_GPL(ktd2692_write_data);
void ktd2692_flash_on(unsigned data)
{
	int ret;
	unsigned long flags = 0;
	struct pinctrl *pinctrl;
	if(data == 0){
		ret = gpio_request(global_ktd2692data->flash_control, "ktd2692_led_control");
		if (ret) {
			printk("Failed to requeset ktd2692_led_control\n");
		} else {
			printk("<ktd2692_flash_on> KTD2692-TORCH OFF. : E(%d)\n", data);
			global_ktd2692data->mode_status = KTD2692_DISABLES_MOVIE_FLASH_MODE;
			spin_lock_irqsave(&global_ktd2692data->int_lock, flags);
			ktd2692_write_data(global_ktd2692data->mode_status|KTD2692_ADDR_MOVIE_FLASHMODE_CONTROL);
			spin_unlock_irqrestore(&global_ktd2692data->int_lock, flags);
			ktd2692_setGpio(0);
			gpio_free(global_ktd2692data->flash_control);
			printk("<ktd2692_flash_on> KTD2692-TORCH OFF. : X(%d)\n", data);
#if defined(CONFIG_SEC_J3Y17QLTE_PROJECT)
			pinctrl = devm_pinctrl_get_select(ktd2692_dev, "front_fled_sleep");
			if (IS_ERR(pinctrl))
				pr_err("%s: flash %s pins are not configured\n", __func__, "front_fled_sleep");
#else
			pinctrl = devm_pinctrl_get_select(ktd2692_dev, "fled_sleep");
			if (IS_ERR(pinctrl))
				pr_err("%s: flash %s pins are not configured\n", __func__, "fled_sleep");
#endif
		}
   }else{
#if defined(CONFIG_SEC_J3Y17QLTE_PROJECT)
		pinctrl = devm_pinctrl_get_select(ktd2692_dev, "front_fled_default");
		if (IS_ERR(pinctrl))
			pr_err("%s: flash %s pins are not configured\n", __func__, "front_fled_default");
#else
		pinctrl = devm_pinctrl_get_select(ktd2692_dev, "fled_default");
		if (IS_ERR(pinctrl))
			pr_err("%s: flash %s pins are not configured\n", __func__, "fled_default");
#endif
		ret = gpio_request(global_ktd2692data->flash_control, "ktd2692_led_control");
		if (ret) {
			printk("Failed to requeset ktd2692_led_control\n");
		} else {
			printk("<ktd2692_flash_on> KTD2692-TORCH ON. : E(%d)\n", data);
			global_ktd2692data->mode_status = KTD2692_ENABLE_MOVIE_MODE;
			spin_lock_irqsave(&global_ktd2692data->int_lock, flags);
			ktd2692_write_data(global_ktd2692data->LVP_Voltage|KTD2692_ADDR_LVP_SETTING);
			#if 0	/* use the internel defualt setting */
				ktd2692_write_data(global_ktd2692data->flash_timeout|KTD2692_ADDR_FLASH_TIMEOUT_SETTING);
			#endif
			ktd2692_write_data(global_ktd2692data->movie_current_value|KTD2692_ADDR_MOVIE_CURRENT_SETTING);
			ktd2692_write_data(global_ktd2692data->mode_status|KTD2692_ADDR_MOVIE_FLASHMODE_CONTROL);
			spin_unlock_irqrestore(&global_ktd2692data->int_lock, flags);
			gpio_free(global_ktd2692data->flash_control);
			printk("<ktd2692_flash_on> KTD2692-TORCH ON. : X(%d)\n", data);
		}
	}
}
#if !(defined CONFIG_SEC_J3Y17QLTE_PROJECT)
int ktd2692_fled_led_off(unsigned char index)
{
	int ret;
	unsigned long flags = 0;
	struct pinctrl *pinctrl;
	if(index == 0){
		ret = gpio_request(global_ktd2692data->flash_control, "ktd2692_led_control");
		if (ret) {
			printk("Failed to requeset ktd2692_led_control\n");
		} else {
			printk("<ktd2692_fled_led_off> KTD2692-FLASH OFF: E(%d)\n", index);
			global_ktd2692data->mode_status = KTD2692_DISABLES_MOVIE_FLASH_MODE;
			spin_lock_irqsave(&global_ktd2692data->int_lock, flags);

			ktd2692_write_data(global_ktd2692data->mode_status|KTD2692_ADDR_MOVIE_FLASHMODE_CONTROL);
			spin_unlock_irqrestore(&global_ktd2692data->int_lock, flags);
			ktd2692_setGpio(0);
			gpio_free(global_ktd2692data->flash_control);
			printk("<ktd2692_fled_led_off>KTD2692-FLASH OFF OFF: X(%d)\n", index);
			pinctrl = devm_pinctrl_get_select(ktd2692_dev, "fled_sleep");
			if (IS_ERR(pinctrl))
				pr_err("%s: flash %s pins are not configured\n", __func__, "fled_sleep");
		}
	}
	return 0;
}
EXPORT_SYMBOL(ktd2692_fled_led_off);

int msm_fled_led_off_ktd2692(ext_pmic_flash_ctrl_t *flash_ctrl)
{
    int ret = 0;
#if !(defined  CONFIG_SEC_J2Y18LTE_PROJECT)
    if(!assistive_light)
      ret = ktd2692_fled_led_off(flash_ctrl->index);
#endif
    return ret;
}
EXPORT_SYMBOL(msm_fled_led_off_ktd2692);

int ktd2692_fled_torch_on(unsigned char index)
{
	int ret;
	unsigned long flags = 0;
	struct pinctrl *pinctrl;	
	pinctrl = devm_pinctrl_get_select(ktd2692_dev, "fled_default");
	if (IS_ERR(pinctrl))
		pr_err("%s: flash %s pins are not configured\n", __func__, "fled_default");

	ret = gpio_request(global_ktd2692data->flash_control, "ktd2692_led_control");
	if (ret) {
		pr_err("Failed to requeset ktd2692_led_control\n");
	} else {
		pr_err("<ktd2692_flash_on> KTD2692-TORCH ON. : E(%d)\n", index);
		global_ktd2692data->mode_status = KTD2692_ENABLE_MOVIE_MODE;
		spin_lock_irqsave(&global_ktd2692data->int_lock, flags);
		ktd2692_write_data(global_ktd2692data->LVP_Voltage|KTD2692_ADDR_LVP_SETTING);
		ktd2692_write_data(global_ktd2692data->movie_current_value|KTD2692_ADDR_MOVIE_CURRENT_SETTING);
		ktd2692_write_data(global_ktd2692data->mode_status|KTD2692_ADDR_MOVIE_FLASHMODE_CONTROL);
		spin_unlock_irqrestore(&global_ktd2692data->int_lock, flags);
		gpio_free(global_ktd2692data->flash_control);
                /*pr_err("LVP: %x, current %x, mode %x",global_ktd2692data->LVP_Voltage|KTD2692_ADDR_LVP_SETTING, 
                   global_ktd2692data->movie_current_value|KTD2692_ADDR_FLASH_CURRENT_SETTING,
                   global_ktd2692data->mode_status|KTD2692_ADDR_MOVIE_FLASHMODE_CONTROL); */
		pr_err("<ktd2692_flash_on> KTD2692-TORCH ON. : X(%d)\n", index);
	}

	return 0;	
}

EXPORT_SYMBOL(ktd2692_fled_torch_on);

int msm_fled_torch_on_ktd2692(ext_pmic_flash_ctrl_t *flash_ctrl)
{
    int ret = 0;
#if !(defined  CONFIG_SEC_J2Y18LTE_PROJECT)
    if(!assistive_light)
       ret = ktd2692_fled_torch_on(flash_ctrl->index);
#endif
    return ret;
}
EXPORT_SYMBOL(msm_fled_torch_on_ktd2692);

int ktd2692_fled_flash_on(unsigned char index)
{
	int ret;
	unsigned long flags = 0;
	struct pinctrl *pinctrl;
	pinctrl = devm_pinctrl_get_select(ktd2692_dev, "fled_default");
	if (IS_ERR(pinctrl))
		pr_err("%s: flash %s pins are not configured\n", __func__, "fled_default");

	ret = gpio_request(global_ktd2692data->flash_control, "ktd2692_led_control");
	if (ret) {
		pr_err("Failed to requeset ktd2692_led_control\n");
	} else {
		pr_err("<ktd2692_flash_on> KTD2692-FLASH ON. : E(%d)\n", index);
		global_ktd2692data->mode_status = KTD2692_ENABLE_FLASH_MODE;
		spin_lock_irqsave(&global_ktd2692data->int_lock, flags);
		ktd2692_write_data(global_ktd2692data->LVP_Voltage|KTD2692_ADDR_LVP_SETTING);
		#if 0	/* use the internel defualt setting */
			ktd2692_write_data(global_ktd2692data->flash_timeout|KTD2692_ADDR_FLASH_TIMEOUT_SETTING);
		#endif
		ktd2692_write_data(global_ktd2692data->flash_current_value|KTD2692_ADDR_FLASH_CURRENT_SETTING);
		ktd2692_write_data(global_ktd2692data->mode_status|KTD2692_ADDR_MOVIE_FLASHMODE_CONTROL);
		spin_unlock_irqrestore(&global_ktd2692data->int_lock, flags);
		gpio_free(global_ktd2692data->flash_control);
                /*pr_err("LVP: %x, current %x, mode %x",global_ktd2692data->LVP_Voltage|KTD2692_ADDR_LVP_SETTING, 
                  global_ktd2692data->flash_current_value|KTD2692_ADDR_FLASH_CURRENT_SETTING,
                  global_ktd2692data->mode_status|KTD2692_ADDR_MOVIE_FLASHMODE_CONTROL); */
		pr_err("<ktd2692_flash_on> KTD2692-FLASH ON. : X(%d)\n", index);
	}
	return 0;
}
EXPORT_SYMBOL(ktd2692_fled_flash_on);

int ktd2692_fled_pre_flash_on(unsigned char index, int32_t pre_flash_current_mA)
{
	int ret;
	unsigned long flags = 0;
	struct pinctrl *pinctrl;
	pinctrl = devm_pinctrl_get_select(ktd2692_dev, "fled_default");
	if (IS_ERR(pinctrl))
		pr_err("%s: flash %s pins are not configured\n", __func__, "fled_default");

	ret = gpio_request(global_ktd2692data->flash_control, "ktd2692_led_control");
	if (ret) {
		pr_err("Failed to requeset ktd2692_led_control\n");
	} else {
		pr_err("<ktd2692_flash_on> KTD2692-PREFLASH ON. : E(%d)\n", index);
		global_ktd2692data->mode_status = KTD2692_ENABLE_FLASH_MODE;
		spin_lock_irqsave(&global_ktd2692data->int_lock, flags);
		ktd2692_write_data(global_ktd2692data->LVP_Voltage|KTD2692_ADDR_LVP_SETTING);

		global_ktd2692data->pre_flash_current_value = pre_flash_current_mA;

		ktd2692_write_data(global_ktd2692data->pre_flash_current_value|KTD2692_ADDR_FLASH_CURRENT_SETTING);
		ktd2692_write_data(global_ktd2692data->mode_status|KTD2692_ADDR_MOVIE_FLASHMODE_CONTROL);
		spin_unlock_irqrestore(&global_ktd2692data->int_lock, flags);
                gpio_free(global_ktd2692data->flash_control);
       		pr_err("<ktd2692_flash_on> KTD2692-PREFLASH ON. : X(%d)\n", index);
	}
	return 0;
}

int msm_fled_pre_flash_on_ktd2692(ext_pmic_flash_ctrl_t *flash_ctrl)
{
    int ret = 0;
#if !(defined  CONFIG_SEC_J2Y18LTE_PROJECT)
    if(!assistive_light)
      ret = ktd2692_fled_pre_flash_on(flash_ctrl->index, flash_ctrl->flash_current_mA);
#endif
    return ret;
}
EXPORT_SYMBOL(msm_fled_pre_flash_on_ktd2692);

int msm_fled_flash_on_ktd2692(ext_pmic_flash_ctrl_t *flash_ctrl)
{
    int ret = 0;
#if !(defined  CONFIG_SEC_J2Y18LTE_PROJECT)
    if(!assistive_light)
      ret = ktd2692_fled_flash_on(flash_ctrl->index);
#endif
    return ret;
}
EXPORT_SYMBOL(msm_fled_flash_on_ktd2692);

int ktd2692_fled_flash_on_set_current(unsigned char index, int32_t flash_current_mA)
{
	return 0;
}
EXPORT_SYMBOL(ktd2692_fled_flash_on_set_current);

int msm_fled_flash_on_set_current_ktd2692(ext_pmic_flash_ctrl_t *flash_ctrl)
{
	return ktd2692_fled_flash_on_set_current(flash_ctrl->index, flash_ctrl->flash_current_mA);
}
EXPORT_SYMBOL(msm_fled_flash_on_set_current_ktd2692);
#endif

ssize_t ktd2692_store(struct device *dev,
			struct device_attribute *attr, const char *buf,
			size_t count)
{
#if defined(CONFIG_ACTIVE_FLASH)
	int sel = 0;
#endif
	int value = 0;
	int ret = 0;
	unsigned long flags = 0;
	struct pinctrl *pinctrl;
	if ((buf == NULL) || kstrtouint(buf, 10, &value)) {
		return -1;
	}
    
	global_ktd2692data->sysfs_input_data = value;
	if (value <= 0) {
#if !(defined CONFIG_SEC_J3Y17QLTE_PROJECT || defined  CONFIG_SEC_J2Y18LTE_PROJECT)
		assistive_light = 0;
#endif
		ret = gpio_request(global_ktd2692data->flash_control, "ktd2692_led_control");
		if (ret) {
			printk("Failed to requeset ktd2692_led_control\n");
		} else {
			pr_err("sysfs KTD2692-TORCH OFF. : E(%d)\n", value);
			global_ktd2692data->mode_status = KTD2692_DISABLES_MOVIE_FLASH_MODE;
			spin_lock_irqsave(&global_ktd2692data->int_lock, flags);
			ktd2692_write_data(global_ktd2692data->mode_status|KTD2692_ADDR_MOVIE_FLASHMODE_CONTROL);
			spin_unlock_irqrestore(&global_ktd2692data->int_lock, flags);
			ktd2692_setGpio(0);
			gpio_free(global_ktd2692data->flash_control);
			printk("KTD2692-TORCH OFF. : X(%d)\n", value);
		}
#if defined(CONFIG_SEC_J3Y17QLTE_PROJECT)
		pinctrl = devm_pinctrl_get_select(ktd2692_dev, "front_fled_sleep");
#else
		pinctrl = devm_pinctrl_get_select(ktd2692_dev, "fled_sleep");
#endif
		if (IS_ERR(pinctrl))
			pr_err("%s: flash %s pins are not configured\n", __func__, "is");
	} else if((value == 1) || (value == 100) || (value == 200)){
#if !(defined CONFIG_SEC_J3Y17QLTE_PROJECT || defined  CONFIG_SEC_J2Y18LTE_PROJECT)
		if(value == 1)
		   assistive_light = 1;
#endif
#if defined(CONFIG_SEC_J3Y17QLTE_PROJECT)
		pinctrl = devm_pinctrl_get_select(ktd2692_dev, "front_fled_default");
#else
		pinctrl = devm_pinctrl_get_select(ktd2692_dev, "fled_default");
#endif
		if (IS_ERR(pinctrl))
			pr_err("%s: flash %s pins are not configured\n", __func__, "host");
		ret = gpio_request(global_ktd2692data->flash_control, "ktd2692_led_control");
		if (ret) {
			printk("Failed to requeset ktd2692_led_control\n");
		} else {
			printk("KTD2692-TORCH ON. : E(%d)\n", value);
			global_ktd2692data->mode_status = KTD2692_ENABLE_MOVIE_MODE;
			spin_lock_irqsave(&global_ktd2692data->int_lock, flags);
			ktd2692_write_data(global_ktd2692data->LVP_Voltage|KTD2692_ADDR_LVP_SETTING);
#if 0	/* use the internel defualt setting */
			ktd2692_write_data(global_ktd2692data->flash_timeout|KTD2692_ADDR_FLASH_TIMEOUT_SETTING);
#endif
			if(value == 1)
				ktd2692_write_data(global_ktd2692data->movie_current_value|KTD2692_ADDR_MOVIE_CURRENT_SETTING);
			else
				ktd2692_write_data(global_ktd2692data->factory_movie_current_value|KTD2692_ADDR_MOVIE_CURRENT_SETTING);
			ktd2692_write_data(global_ktd2692data->mode_status|KTD2692_ADDR_MOVIE_FLASHMODE_CONTROL);
			spin_unlock_irqrestore(&global_ktd2692data->int_lock, flags);
			gpio_free(global_ktd2692data->flash_control);
			printk("KTD2692-TORCH FACTORY ON. : X(%d)\n", value);
		}
	}
#if defined(CONFIG_ACTIVE_FLASH)
	else if (value>1000 && value<=1010) {
#if defined(CONFIG_SEC_J3Y17QLTE_PROJECT)
		pinctrl = devm_pinctrl_get_select(ktd2692_dev, "front_fled_default");
#else
		pinctrl = devm_pinctrl_get_select(ktd2692_dev, "fled_default");
#endif
		if (IS_ERR(pinctrl))
			pr_err("%s: flash %s pins are not configured\n", __func__, "host");
		ret = gpio_request(global_ktd2692data->flash_control, "ktd2692_led_control");
		if (ret) {
			printk("Failed to requeset ktd2692_led_control\n");
		} else {
			pr_err("Torch ON-F active\n");
			printk("KTD2692-TORCH ON. : E(%d)\n", value);
			global_ktd2692data->mode_status = KTD2692_ENABLE_MOVIE_MODE;
			spin_lock_irqsave(&global_ktd2692data->int_lock, flags);
			ktd2692_write_data(global_ktd2692data->LVP_Voltage|KTD2692_ADDR_LVP_SETTING);
			sel = torchlevel[value - 1001];
			//global_ktd2692data->factory_movie_current_value = sel;
			ktd2692_write_data(sel|KTD2692_ADDR_MOVIE_CURRENT_SETTING);
			ktd2692_write_data(global_ktd2692data->mode_status|KTD2692_ADDR_MOVIE_FLASHMODE_CONTROL);
			spin_unlock_irqrestore(&global_ktd2692data->int_lock, flags);
			gpio_free(global_ktd2692data->flash_control);
			//assistive_light = true;
			printk("KTD2692-TORCH ON. : X(%d)\n", value);
		}
	}
#endif	
	else{
		printk("KTD2692-TORCH ON. : X(%d)\n", value);
	}
	if ((value <= 0 || value == 1 || value == 100) && !IS_ERR(pinctrl))
		devm_pinctrl_put(pinctrl);

	return count;
}
ssize_t ktd2692_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", global_ktd2692data->sysfs_input_data);
}
#if defined(CONFIG_LEDS_KTD2692_FOR_FRONT)
static DEVICE_ATTR(front_flash, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH,	ktd2692_show, ktd2692_store);
#else
static DEVICE_ATTR(rear_flash, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH, ktd2692_show, ktd2692_store);
#endif

static int ktd2692_parse_dt(struct device *dev,
                                struct ktd2692_platform_data *pdata)
{
	struct device_node *dnode = dev->of_node;
	int ret = 0;
	/* Defulat Value */
	pdata->LVP_Voltage = KTD2692_DISABLE_LVP;
	pdata->flash_timeout = KTD2692_TIMER_1049ms;	/* default */
	pdata->min_current_value = KTD2692_MIN_CURRENT_240mA;
#if defined(CONFIG_SEC_J2Y18LTE_PROJECT) || defined(CONFIG_SEC_J3Y17QLTE_PROJECT)
	pdata->movie_current_value = KTD2692_MOVIE_CURRENT9;
#else
	pdata->movie_current_value = KTD2692_MOVIE_CURRENT6;
#endif
#if defined(CONFIG_SEC_J3Y17QLTE_PROJECT)
	pdata->factory_movie_current_value = KTD2692_MOVIE_CURRENT10;
	pdata->flash_current_value = KTD2692_FLASH_CURRENT16;
#else
	pdata->factory_movie_current_value = KTD2692_MOVIE_CURRENT6;
	pdata->flash_current_value = KTD2692_FLASH_CURRENT15;
	pdata->pre_flash_current_value = KTD2692_FLASH_CURRENT2;
#endif		
	pdata->mode_status = KTD2692_DISABLES_MOVIE_FLASH_MODE;

	/* get gpio */
	pdata->flash_control = of_get_named_gpio(dnode, "flash-en-gpio", 0);
	if (!gpio_is_valid(pdata->flash_control)) {
		dev_err(dev, "failed to get flash_control\n");
		return -1;
	}
	return ret;
}
static int ktd2692_probe(struct platform_device *pdev)
{
	struct ktd2692_platform_data *pdata;
	int ret = 0;
	if (pdev->dev.of_node) {
		pdata = devm_kzalloc(&pdev->dev, sizeof(*pdata), GFP_KERNEL);
		if (!pdata) {
			dev_err(&pdev->dev, "Failed to allocate memory\n");
			return -ENOMEM;
		}
		ret = ktd2692_parse_dt(&pdev->dev, pdata);
		if (ret < 0) {
			return -EFAULT;
		}
	} else {
	pdata = pdev->dev.platform_data;
		if (pdata == NULL) {
			return -EFAULT;
		}
	}
	global_ktd2692data = pdata;
	ktd2692_dev = &pdev->dev;
	printk("KTD2692_LED Probe\n");
#if defined(CONFIG_LEDS_KTD2692_FOR_FRONT)
/*	if (IS_ERR(flash_dev)) {
		printk("Failed to access device(flash)!\n");
	}
*/
	if (IS_ERR_OR_NULL(camera_class)) {
		pr_err("flash_sysfs: error, camera class not exist");
		return -ENODEV;
	}
	
	flash_dev = device_create(camera_class, NULL, 0, NULL, "flash");
	if (IS_ERR(flash_dev)) {
		pr_err("flash_sysfs: failed to create device(flash)\n");
		return -ENODEV;
	}
	if (device_create_file(flash_dev, &dev_attr_front_flash) < 0) {
		printk("failed to create device file, %s\n",
				dev_attr_front_flash.attr.name);
	}
#else
	if (IS_ERR_OR_NULL(camera_class)) {
		pr_err("flash_sysfs: error, camera class not exist");
		return -ENODEV;
	}

	flash_dev = device_create(camera_class, NULL, 0, NULL, "flash");
	if (IS_ERR(flash_dev)) {
		printk("Failed to create device(flash)!\n");
	}
	if (device_create_file(flash_dev, &dev_attr_rear_flash) < 0) {
		printk("failed to create device file, %s\n",
				dev_attr_rear_flash.attr.name);
	}
#endif
	spin_lock_init(&pdata->int_lock);
	return 0;
}
static int ktd2692_remove(struct platform_device *pdev)
{
#if defined(CONFIG_LEDS_KTD2692_FOR_FRONT)
	device_remove_file(flash_dev, &dev_attr_front_flash);
#else
	device_remove_file(flash_dev, &dev_attr_rear_flash);
#endif
	device_destroy(camera_class, 0);
	class_destroy(camera_class);

	return 0;
}
#ifdef CONFIG_OF
static struct of_device_id ktd2692_flash_of_match[] = {
	{ .compatible = "ktd2692",},
	{},
};
MODULE_DEVICE_TABLE(of, ktd2692_flash_of_match);
#endif
static struct platform_driver ktd2692_flash_driver = {
	.probe = ktd2692_probe,
	.remove = ktd2692_remove,
	.driver = {
		.name = "ktd2692-flash",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(ktd2692_flash_of_match),
	},
};
module_platform_driver(ktd2692_flash_driver);
/*
static int __init ktd2692_init(void)
{
	return platform_driver_register(&ktd2692_driver);
}
static void __exit ktd2692_exit(void)
{
	platform_driver_unregister(&ktd2692_driver);
}
*/
MODULE_DESCRIPTION("KTD2692 Flash Driver");
MODULE_AUTHOR("Daniel Jeong");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:lm3632-flash");
