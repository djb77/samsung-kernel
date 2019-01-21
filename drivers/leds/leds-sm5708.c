/*
 * Flash-LED device driver for SM5708
 *
 * Copyright (C) 2015 Silicon Mitus
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/leds.h>
#include <linux/i2c.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/workqueue.h>
#include <linux/leds-sm5708.h>
#include <linux/mfd/sm5708/sm5708.h>
#include <linux/muic/muic_afc.h>
#include <linux/battery/charger/sm5708_charger_oper.h>
#if defined(CONFIG_MUIC_UNIVERSAL_SM5705_AFC)
#define DISABLE_AFC
#endif
#include <soc/qcom/camera2.h>

struct msm_pinctrl_info g_led_pinctrl_info;

enum {
	SM5708_FLED_OFF_MODE					= 0x0,
	SM5708_FLED_ON_MOVIE_MODE				= 0x1,
	SM5708_FLED_ON_FLASH_MODE				= 0x2,
	SM5708_FLED_ON_EXTERNAL_CONTROL_MODE	= 0x3,
};

struct sm5708_fled_info {
	struct device *dev;
	struct i2c_client *i2c;

	struct sm5708_fled_platform_data *pdata;
	struct device *rear_fled_dev;

	/* for Flash VBUS check */
	struct workqueue_struct *wqueue;
	struct delayed_work	fled0_vbus_check_work;
	struct delayed_work	fled1_vbus_check_work;
};

typedef struct {
	int level;
	int current_mA;
} FlashlightLevelInfo;

#define MAX_FLASHLIGHT_LEVEL 5
FlashlightLevelInfo calibData[MAX_FLASHLIGHT_LEVEL] = {
#if defined(CONFIG_SEC_JACKPOTQLTE_PROJECT)
	{1001, 20},
	{1002, 40},
	{1004, 60},
	{1006, 100},
	{1009, 130}
#elif defined(CONFIG_SEC_JACKPOT2QLTE_PROJECT)
	{1001, 20},
	{1002, 50},
	{1004, 70},
	{1006, 110},
	{1009, 140}
#else  //default case
	{1001, 20},
	{1002, 40},
	{1004, 60},
	{1006, 100},
	{1009, 130}
#endif
};


/*sys/class/camera*/
extern struct class *camera_class;
bool assistive_light[2] = {false, false};
bool factory_flash_test[2] = { false, false };

static struct sm5708_fled_info *g_sm5708_fled;
static bool muic_flash_on_status = false;

static int __try_muic_torch_prepare(int state); 

static inline int __get_revision_number(void)
{
	return 2;
}

/**
 * SM5708 Flash-LEDs device register control functions
 */
#if 0
static int sm5708_led_pinctrl_config(bool on){
	int ret = 0;

	pr_debug("%s, %d : Enter\n", __FUNCTION__, __LINE__);

	if (g_led_pinctrl_info.pinctrl == NULL) {
		pr_err("%s:%d Invalid pinctrl\n", __func__, __LINE__);
		return -EINVAL;
	}

	if (on) {
		pr_info("%s: gpio_state_active\n", __func__);
		ret = pinctrl_select_state(g_led_pinctrl_info.pinctrl,
				g_led_pinctrl_info.gpio_state_active);
		if (ret)
			pr_err("%s:%d cannot set pin to active state\n",
				__func__, __LINE__);
	} else {
		pr_info("%s: gpio_state_suspend\n", __func__);
		ret = pinctrl_select_state(g_led_pinctrl_info.pinctrl,
				g_led_pinctrl_info.gpio_state_suspend);
		if (ret)
			pr_err("%s:%d cannot set pin to suspend state\n",
				__func__, __LINE__);
	}
	pr_debug("%s, %d : Exit\n", __FUNCTION__, __LINE__);

	return ret;
}
#endif
static int sm5708_FLEDx_mode_enable(struct sm5708_fled_info *sm5708_fled, int index, unsigned char FLEDxEN)
{
	int ret;

	ret = sm5708_update_reg(sm5708_fled->i2c, SM5708_REG_FLED1CNTL1 + (index * 4),	(FLEDxEN & 0x3), 0x3);
	if (IS_ERR_VALUE(ret)) {
		dev_err(sm5708_fled->dev, "%s: fail to update REG:FLED%dEN (value=%d)\n",	__func__, index, FLEDxEN);
		return ret;
	}

	dev_info(sm5708_fled->dev, "%s: FLED[%d] set mode = %d\n",	__func__, index, FLEDxEN);

	return 0;
}

#if 0
static inline unsigned char _calc_oneshot_time_offset_to_ms(unsigned short ms)
{
	if (ms < 100) {
		return 0;
	} else {
		return (((ms - 100) / 100) & 0xF);
	}
}

static int sm5708_FLEDx_oneshot_config(struct sm5708_fled_info *sm5708_fled, int index, bool enable, unsigned short timer_ms)
{
	int ret;
	unsigned char reg_val;

	reg_val = (((!enable) & 0x1) << 4) | (_calc_oneshot_time_offset_to_ms(timer_ms) & 0xF);
	ret = sm5708_write_reg(sm5708_fled->i2c,
		SM5708_REG_FLED1CNTL2 + (index * 4), reg_val);
	if (IS_ERR_VALUE(ret)) {
		dev_err(sm5708_fled->dev, "%s: fail to write REG:FLED%dCNTL2 (value=%d)\n",	__func__, index, reg_val);
		return ret;
	}

	return 0;
}
#endif

static inline unsigned char _calc_flash_current_offset_to_mA(int index, unsigned short current_mA)
{
	if (index) {	/* FLED1 */
		return current_mA < 400 ? (((current_mA - 25) / 25) & 0x1F) : ((((current_mA - 400) / 25) + 0xF) & 0x1F);
	} else {		/* FLED0 */
		return current_mA < 700 ? (((current_mA - 300) / 25) & 0x1F) : ((((current_mA - 700) / 50) + 0xF) & 0x1F);
	}
}

static int sm5708_FLEDx_set_flash_current(struct sm5708_fled_info *sm5708_fled, int index, unsigned short current_mA)
{
	int ret;
	unsigned char reg_val;

	reg_val = _calc_flash_current_offset_to_mA(index, current_mA);
	ret = sm5708_write_reg(sm5708_fled->i2c, SM5708_REG_FLED1CNTL3 + (index * 4), reg_val);
	if (IS_ERR_VALUE(ret)) {
		dev_err(sm5708_fled->dev, "%s: fail to write REG:FLED%dCNTL3 (value=%d)\n",	__func__, index, reg_val);
		return ret;
	}

	return 0;
}

static inline unsigned char _calc_torch_current_offset_to_mA(unsigned short current_mA)
{
	return (((current_mA - 10) / 10) & 0x1F);
}

static inline unsigned short _calc_torch_current_mA_to_offset(unsigned char offset)
{
	return (((offset & 0x1F) + 1) * 10);
}

static int sm5708_FLEDx_set_torch_current(struct sm5708_fled_info *sm5708_fled, int index, unsigned short current_mA)
{
	int ret;
	unsigned char reg_val;

	reg_val = _calc_torch_current_offset_to_mA(current_mA);
	ret = sm5708_write_reg(sm5708_fled->i2c, SM5708_REG_FLED1CNTL4 + (index * 4), reg_val);
	if (IS_ERR_VALUE(ret)) {
		dev_err(sm5708_fled->dev, "%s: fail to write REG:FLED%dCNTL4 (value=%d)\n",	__func__, index, reg_val);
		return ret;
	}

	return 0;
}


/**
 * SM570 Flash-LED to MUIC interface functions
 */

static bool fimc_is_activated = 0;

int sm5708_fled_muic_camera_flash_work_on(void)
{
	pr_debug("sm5708-fled: %s\n", __func__);

#if defined(CONFIG_MUIC_UNIVERSAL_SM5705_AFC)
	muic_check_afc_state(1);
#endif
	fimc_is_activated = 1;  /* Camera sensor power ON */

	return 0;
}
EXPORT_SYMBOL(sm5708_fled_muic_camera_flash_work_on);

int sm5708_fled_muic_camera_flash_work_off(void)
{
	pr_debug("sm5708-fled: %s\n", __func__);

#if defined(CONFIG_MUIC_UNIVERSAL_SM5705_AFC)
	muic_check_afc_state(0);
#endif

	fimc_is_activated = 0;  /* Camera sensor power OFF */

	return 0;
}
EXPORT_SYMBOL(sm5708_fled_muic_camera_flash_work_off);

static int sm5708_fled_muic_flash_on_prepare(void)
{
	int ret = 0;

#if defined(CONFIG_MUIC_UNIVERSAL_SM5705_AFC)
	if (muic_torch_prepare(1) == 1) {
	muic_flash_on_status = true;
	ret = 0;
	} else {
		pr_err("%s: fail to prepare for AFC V_drop\n", __func__);
		ret = -1;
	}
#endif
	pr_debug("sm5708-fled: %s (muic_flash_on_status=%d)\n", __func__, muic_flash_on_status);

	return ret;
}


static void sm5708_fled_muic_flash_off_prepare(void)
{
	if (muic_flash_on_status == true) {
#if defined(CONFIG_MUIC_UNIVERSAL_SM5705_AFC)
		muic_torch_prepare(0);
#endif
		muic_flash_on_status = false;
	}
	pr_debug("sm5708-fled: %s \n", __func__);
}

/**
 * SM5708 Flash-LED operation control functions
 */
static int sm5708_fled_initialize(struct sm5708_fled_info *sm5708_fled)
{
	struct device *dev = sm5708_fled->dev;
	struct sm5708_fled_platform_data *pdata = sm5708_fled->pdata;
	int i, ret;

	for (i = 0; i < SM5708_FLED_MAX; ++i) {
		if (pdata->led[i].used_gpio) {
			ret = gpio_request(pdata->led[i].flash_en_pin, "sm5708_fled");
			if (IS_ERR_VALUE(ret)) {
				dev_err(dev, "%s: fail to request flash gpio pin = %d (ret=%d)\n",	__func__, pdata->led[i].flash_en_pin, ret);
				return ret;
			}
			gpio_direction_output(pdata->led[i].flash_en_pin, 0);

			ret = gpio_request(pdata->led[i].torch_en_pin, "sm5708_fled");
			if (IS_ERR_VALUE(ret)) {
				dev_err(dev, "%s: fail to request torch gpio pin = %d (ret=%d)\n",	__func__, pdata->led[i].torch_en_pin, ret);
				return ret;
			}
			gpio_direction_output(pdata->led[i].torch_en_pin, 0);

			dev_info(dev, "SM5708 FLED[%d] used External GPIO control Mode (Flash pin=%d, Torch pin=%d)\n",
				i, pdata->led[i].flash_en_pin, pdata->led[i].torch_en_pin);
		} else {
			dev_info(dev, "SM5708 FLED[%d] used I2C control Mode\n", i);
		}
		ret = sm5708_FLEDx_mode_enable(sm5708_fled, i, SM5708_FLED_OFF_MODE);
		if (IS_ERR_VALUE(ret)) {
			dev_err(dev, "%s: fail to set FLED[%d] external control mode\n", __func__, i);
			return ret;
		}
	}

	return 0;
}

static void sm5708_fled_deinitialize(struct sm5708_fled_info *sm5708_fled)
{
	struct device *dev = sm5708_fled->dev;
	struct sm5708_fled_platform_data *pdata = sm5708_fled->pdata;
	int i;

	for (i = 0; i < SM5708_FLED_MAX; ++i) {
		if (pdata->led[i].used_gpio) {
			gpio_free(pdata->led[i].flash_en_pin);
			gpio_free(pdata->led[i].torch_en_pin);
		}
		sm5708_FLEDx_mode_enable(sm5708_fled, i, SM5708_FLED_OFF_MODE);
	}

	dev_info(dev, "%s: FLEDs de-initialize done.\n", __func__);
}

static inline int _fled_turn_on_torch(struct sm5708_fled_info *sm5708_fled, int index)
{
	struct sm5708_fled_platform_data *pdata = sm5708_fled->pdata;
	struct device *dev = sm5708_fled->dev;
	int ret;

	pr_debug("%s: FLED[%d] Torch turn-on done.\n", __func__, index);

	gpio_set_value(pdata->led[index].flash_en_pin, 0);
	gpio_set_value(pdata->led[index].torch_en_pin, 1);

	if (pdata->led[index].used_gpio) {
		ret = sm5708_FLEDx_mode_enable(sm5708_fled, index,	SM5708_FLED_ON_EXTERNAL_CONTROL_MODE);
		if (IS_ERR_VALUE(ret)) {
			dev_err(dev, "%s: fail to set FLED[%d] External control mode\n", __func__, index);
			return ret;
		}

		gpio_set_value(pdata->led[index].flash_en_pin, 0);
		gpio_set_value(pdata->led[index].torch_en_pin, 1);
	} else {
		ret = sm5708_FLEDx_mode_enable(sm5708_fled, index,	SM5708_FLED_ON_MOVIE_MODE);
		if (IS_ERR_VALUE(ret)) {
			dev_err(dev, "%s: fail to set FLED[%d] Movie mode\n", __func__, index);
			return ret;
		}
	}

	sm5708_charger_oper_push_event(SM5708_CHARGER_OP_EVENT_TORCH, 1);
	//sm5708_led_pinctrl_config(1);
	
	dev_info(dev, "%s: FLED[%d] Torch turn-on done.\n", __func__, index);

	return 0;
}

static int sm5708_fled_turn_on_torch(struct sm5708_fled_info *sm5708_fled, int index, unsigned short current_mA)
{
	struct device *dev = sm5708_fled->dev;
	int ret;        

	dev_err(dev, "%s: set FLED[%d] torch current (current_mA=%d)\n", __func__, index, current_mA);
	pr_debug("sm5705-fled : %s fimc_is_activated=%d\n", __func__, fimc_is_activated);

	if (fimc_is_activated == 0) {
		/* used only Torch case - Ex> flashlight : Need to 9V -> 5V */
		sm5708_fled_muic_flash_on_prepare();
	}

	ret = sm5708_FLEDx_set_torch_current(sm5708_fled, index, current_mA);
	if (IS_ERR_VALUE(ret)) {
		if (fimc_is_activated == 0) {
			sm5708_fled_muic_flash_off_prepare();
		}

		dev_err(dev, "%s: fail to set FLED[%d] torch current (current_mA=%d)\n", __func__, index, current_mA);
		return ret;
	}

	_fled_turn_on_torch(sm5708_fled, index);

	return 0;
}


static int sm5708_fled_turn_on_flash(struct sm5708_fled_info *sm5708_fled, int index, unsigned short current_mA)
{
	struct device *dev = sm5708_fled->dev;
	struct sm5708_fled_platform_data *pdata = sm5708_fled->pdata;
	int ret;

	ret = sm5708_FLEDx_set_flash_current(sm5708_fled, index, current_mA);
	if (IS_ERR_VALUE(ret)) {
		dev_err(dev, "%s: fail to set FLED[%d] flash current (current_mA=%d)\n", __func__, index, current_mA);
		return ret;
	}
	/*
	Charger event need to push first before gpio enable.
	In flash capture mode, when connect some types charger or USB, from pre-flash -> main-flash ,
	torch on first and charger has been change as torch mode, if enable flash first, it will work
	on torch mode which use flash current , Vbus will pull down and charger disconnect.
	*/
	sm5708_charger_oper_push_event(SM5708_CHARGER_OP_EVENT_FLASH, 1);

	if (pdata->led[index].used_gpio) {
		ret = sm5708_FLEDx_mode_enable(sm5708_fled, index,	SM5708_FLED_ON_EXTERNAL_CONTROL_MODE);
		if (IS_ERR_VALUE(ret)) {
			dev_err(dev, "%s: fail to set FLED[%d] External control mode\n", __func__, index);
			return ret;
		}
		gpio_set_value(pdata->led[index].torch_en_pin, 0);
		gpio_set_value(pdata->led[index].flash_en_pin, 1);
	} else {
		ret = sm5708_FLEDx_mode_enable(sm5708_fled, index,	SM5708_FLED_ON_FLASH_MODE);
		if (IS_ERR_VALUE(ret)) {
			dev_err(dev, "%s: fail to set FLED[%d] Flash mode\n", __func__, index);
			return ret;
		}
	}

	dev_info(dev, "%s: FLED[%d] Flash turn-on done.\n", __func__, index);

	return 0;
}

static int sm5708_fled_turn_off(struct sm5708_fled_info *sm5708_fled, int index)
{
	struct device *dev = sm5708_fled->dev;
	struct sm5708_fled_platform_data *pdata = sm5708_fled->pdata;
	int ret;

	if (pdata->led[index].used_gpio) {
		gpio_set_value(pdata->led[index].flash_en_pin, 0);
		gpio_set_value(pdata->led[index].torch_en_pin, 0);
	}
        
        pr_debug("sm5708-fled : %s fimc_is_activated=%d\n", __func__, fimc_is_activated);

	ret = sm5708_FLEDx_mode_enable(sm5708_fled, index, SM5708_FLED_OFF_MODE);
	if (IS_ERR_VALUE(ret)) {
		dev_err(dev, "%s: fail to set FLED[%d] OFF mode\n", __func__, index);
		return ret;
	}

	if (fimc_is_activated == 0) {
		/* used only Torch case - Ex> flashlight : Need to 5V -> 9V */
		sm5708_fled_muic_flash_off_prepare();
	}

	ret = sm5708_FLEDx_set_flash_current(sm5708_fled, index, 0);
	if (IS_ERR_VALUE(ret)) {
		dev_err(dev, "%s: fail to set FLED[%d] flash current\n", __func__, index);
		return ret;
	}
	sm5708_charger_oper_push_event(SM5708_CHARGER_OP_EVENT_FLASH, 0);

	if ((assistive_light[0] == false) && (assistive_light[1] == false))	{
	           ret = sm5708_FLEDx_set_torch_current(sm5708_fled, index, 0);
	           if (IS_ERR_VALUE(ret)) {
		   dev_err(dev, "%s: fail to set FLED[%d] torch current\n", __func__, index);
		   return ret;
	}

	sm5708_charger_oper_push_event(SM5708_CHARGER_OP_EVENT_TORCH, 0);
}

dev_info(dev, "%s: FLED[%d] turn-off done.\n", __func__, index);
//sm5708_led_pinctrl_config(0);

return 0;
}

/**
 *  For Export Flash control functions (external GPIO control)
 */
#if 0 /* M OS - do not used it */
static bool fimc_is_activated = 0;

int sm5708_fled_prepare_flash(unsigned char index)
{
        if (fimc_is_activated == 1) {
               /* skip to overlapping function calls */
               return 0;
        }       
	if (g_sm5708_fled == NULL) {
		pr_err("sm5708-fled: %s: invalid g_sm5708_fled, maybe not registed fled \
			device driver\n", __func__);
		return -ENXIO;
	}

	dev_info(g_sm5708_fled->dev, "%s: check - GPIO used, set - Torch/Flash current\n",  __func__);

	if (g_sm5708_fled->pdata->led[index].used_gpio == 0) {
		pr_err("sm5708-fled: %s: can't used external GPIO control, check device tree\n", __func__);
		return -ENOENT;
	}


	pr_err("sm5708-fled: torch current : %d, flash_current : %d", g_sm5708_fled->pdata->led[index].torch_current_mA, g_sm5708_fled->pdata->led[index].flash_current_mA);

	sm5708_FLEDx_set_torch_current(g_sm5708_fled, index, g_sm5708_fled->pdata->led[index].torch_current_mA);
	sm5708_FLEDx_set_flash_current(g_sm5708_fled, index, g_sm5708_fled->pdata->led[index].flash_current_mA);

        fimc_is_activated = 1;
	return 0;
}
EXPORT_SYMBOL(sm5708_fled_prepare_flash);
#endif

int sm5708_fled_torch_on(unsigned char index)
{
	if (assistive_light[index] == false) {
		if (g_sm5708_fled == NULL) {
			pr_err("sm5708-fled: %s: invalid g_sm5708_fled, maybe not registed fled device driver\n", __func__);
			return -ENXIO;
		}
		dev_info(g_sm5708_fled->dev, "%s: Torch - ON(%d mA)\n", __func__, g_sm5708_fled->pdata->led[index].torch_current_mA);
		sm5708_fled_turn_on_torch(g_sm5708_fled, index, g_sm5708_fled->pdata->led[index].torch_current_mA);
	}

	return 0;
}
EXPORT_SYMBOL(sm5708_fled_torch_on);

int sm5708_fled_torch_set_current(unsigned char index, int current_ma)
{
    int ret;
	if (assistive_light[index] == false) {
		if (g_sm5708_fled == NULL) {
			pr_err("sm5708-fled: %s: invalid g_sm5708_fled, maybe not registed fled device driver\n", __func__);
			return -ENXIO;
		}
		dev_info(g_sm5708_fled->dev, "%s: Torch - setcurrenet(%d mA)\n", __func__, current_ma);

		ret = sm5708_FLEDx_set_torch_current(g_sm5708_fled, index, current_ma);
		if (IS_ERR_VALUE(ret)) {
			dev_err(g_sm5708_fled->dev, "ERROR %s: Torch - setcurrenet (%d mA)\n", __func__, current_ma);
		}
	}

	return 0;
}


int msm_fled_torch_on_sm5708(ext_pmic_flash_ctrl_t *flash_ctrl)
{
    if(flash_ctrl->flash_current_mA == -1) //default
		return sm5708_fled_torch_on(flash_ctrl->index);
	else
		return sm5708_fled_torch_set_current(flash_ctrl->index, flash_ctrl->flash_current_mA);
}
EXPORT_SYMBOL(msm_fled_torch_on_sm5708);

int sm5708_fled_pre_flash_on(unsigned char index, int32_t pre_flash_current_mA)
{
	if (assistive_light[index] == false) {
		dev_info(g_sm5708_fled->dev, "%s: Preflash - ON(%d mA)\n", __func__, g_sm5708_fled->pdata->led[index].preflash_current_mA);
		if (pre_flash_current_mA > 0)
			sm5708_fled_turn_on_torch(g_sm5708_fled, index, (unsigned short) pre_flash_current_mA);
		else
			sm5708_fled_turn_on_torch(g_sm5708_fled, index, g_sm5708_fled->pdata->led[index].preflash_current_mA);
	}

	return 0;
}
EXPORT_SYMBOL(sm5708_fled_pre_flash_on);

int msm_fled_pre_flash_on_sm5708(ext_pmic_flash_ctrl_t *flash_ctrl)
{
	return sm5708_fled_pre_flash_on(flash_ctrl->index, flash_ctrl->flash_current_mA);
}
EXPORT_SYMBOL(msm_fled_pre_flash_on_sm5708);

int sm5708_fled_flash_on_set_current(unsigned char index, int32_t flash_current_mA)
{
	if (assistive_light[index] == false) {
		dev_info(g_sm5708_fled->dev, "%s: Flash(user val)- ON(%d mA)\n", __func__, flash_current_mA);
		sm5708_fled_turn_on_flash(g_sm5708_fled, index, (unsigned short) flash_current_mA);
	}

	return 0;
}
EXPORT_SYMBOL(sm5708_fled_flash_on_set_current);

int msm_fled_flash_on_set_current_sm5708(ext_pmic_flash_ctrl_t *flash_ctrl)
{
	return sm5708_fled_flash_on_set_current(flash_ctrl->index, flash_ctrl->flash_current_mA);
}
EXPORT_SYMBOL(msm_fled_flash_on_set_current_sm5708);

int sm5708_fled_flash_on(unsigned char index)
{
	if (assistive_light[index] == false) {
		if (g_sm5708_fled == NULL) {
			pr_err("sm5708-fled: %s: invalid g_sm5708_fled, maybe not registed fled device driver\n", __func__);
			return -ENXIO;
		}

		dev_info(g_sm5708_fled->dev, "%s: Flash - ON(%d mA)\n", __func__, g_sm5708_fled->pdata->led[index].flash_current_mA);
		sm5708_fled_turn_on_flash(g_sm5708_fled, index, g_sm5708_fled->pdata->led[index].flash_current_mA);
	}

	return 0;
}
EXPORT_SYMBOL(sm5708_fled_flash_on);

int sm5708_fled_preflash(unsigned char index)
{
	if (assistive_light[index] == false) {
		dev_info(g_sm5708_fled->dev, "%s: Pre Flash or Movie - ON\n", __func__);
		sm5708_fled_turn_on_torch(g_sm5708_fled, index, g_sm5708_fled->pdata->led[index].preflash_current_mA);
	}
	return 0;
}
EXPORT_SYMBOL(sm5708_fled_preflash);

int msm_fled_flash_on_sm5708(ext_pmic_flash_ctrl_t *flash_ctrl)
{
	return sm5708_fled_flash_on(flash_ctrl->index);
}
EXPORT_SYMBOL(msm_fled_flash_on_sm5708);

int sm5708_fled_led_off(unsigned char index)
{
	if (g_sm5708_fled == NULL) {
		printk("%s:invalid g_sm5708_fled, maybe not registed fled device driver\n", __func__);
		return 0;
	}
	if (assistive_light[index] == false) {
		dev_info(g_sm5708_fled->dev, "%s: LED - OFF\n", __func__);
		sm5708_fled_turn_off(g_sm5708_fled, index);
    }

	return 0;
}
EXPORT_SYMBOL(sm5708_fled_led_off);

int msm_fled_led_off_sm5708(ext_pmic_flash_ctrl_t *flash_ctrl)
{
	return sm5708_fled_led_off(flash_ctrl->index);
}
EXPORT_SYMBOL(msm_fled_led_off_sm5708);

#if 0 /* M OS - do not used it */
int sm5708_fled_close_flash(unsigned char index)
{
    if (fimc_is_activated == 0) {
        /* skip to overlapping function calls */
        return 0;
    }

    dev_info(g_sm5708_fled->dev, "%s: Close Process\n", __func__);

    if (g_sm5708_fled == NULL) {
        pr_err("sm5708-fled: %s: invalid g_sm5708_fled, maybe not registed fled device driver\n", __func__);
        return -ENXIO;
    }

    sm5708_fled_muic_flash_work_off(g_sm5708_fled);

    fimc_is_activated = 0;

    return 0;
}
EXPORT_SYMBOL(sm5708_fled_close_flash);
#endif

/**
 * For Camera-class Rear Flash device file support functions
 */
static int __try_muic_torch_prepare(int state)
{
	int ret = 0;
#ifdef DISABLE_AFC
	int retry = 0;
	/* MUIC 9V -> 5V function */
	for (retry = 0; retry < 3; retry++) {
		if (muic_torch_prepare(state) == 1)
			break;
		pr_err("%s:%d ERROR: AFC disable unsuccessfull retrying after 30ms\n", __func__, __LINE__);
		msleep(30);
	}

	if (retry == 3) {
		pr_err("%s:%d ERROR: AFC disable failed\n", __func__, __LINE__);
		ret = -1;
	}
#endif

	return ret;
}
#define REAR_FLASH_INDEX	SM5708_FLED_0

/*
 ****  From the data sheet: Torch Mode control register mapping *****
		0x0: 10mA    0x8: 90mA    0x10: 170mA    0x18: 250mA
		0x1: 20mA    0x9: 100mA   0x11: 180mA    0x19: 260mA
		0x2: 30mA    0xA: 110mA   0x12: 190mA    0x1A: 270mA
		0x3: 40mA    0xB: 120mA   0x13: 200mA    0x1B: 280mA
		0x4: 50mA    0xC: 130mA   0x14: 210mA    0x1C: 290mA
		0x5: 60mA    0xD: 140mA   0x15: 220mA    0x1D: 300mA
		0x6: 70mA    0xE: 150mA   0x16: 230mA    0x1E: 310mA
		0x7: 80mA    0xF: 160mA   0x17: 240mA    0x1F: 320mA
*/

static ssize_t sm5708_rear_flash_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct sm5708_fled_info *sm5708_fled = dev_get_drvdata(dev->parent);

	int index = -1;
	int ret = -EINVAL, value_u32 = 0;
	
	/*
	* If None of DUAL config is enabled:
			rear_flash      : 0
			front_flash     : 1
	* If REAR_DUAL config is enabled:
			rear_flash      : 0
			rear_flash_2    : 1
	* If FRONT_DUAL config is enabled:
			front_flash     : 0
			front_flash_2   : 1
	*/
#if !defined(CONFIG_REAR_DUAL_LED_FLASH) && !defined(CONFIG_FRONT_DUAL_LED_FLASH)
	if (strcmp(attr->attr.name, "rear_flash") == 0) {
		pr_info("flash index is 0\n");
		index = SM5708_FLED_0;
	}
	else if (strcmp(attr->attr.name, "front_flash") == 0) {
		pr_info("flash index is 1\n");
		index = SM5708_FLED_1;
	}
#elif defined(CONFIG_REAR_DUAL_LED_FLASH)
	if (strcmp(attr->attr.name, "rear_flash") == 0) {
		pr_info("flash index is 0\n");
		index = SM5708_FLED_0;
	}
	else if (strcmp(attr->attr.name, "rear_flash_2") == 0) {
		pr_info("flash index is 1\n");
		index = SM5708_FLED_1;
	}
#elif defined(CONFIG_FRONT_DUAL_LED_FLASH)
	if (strcmp(attr->attr.name, "front_flash") == 0) {
		pr_info("flash index is 0\n");
		index = SM5708_FLED_0;
	}
	else if (strcmp(attr->attr.name, "front_flash_2") == 0) {
		pr_info("flash index is 1\n");
		index = SM5708_FLED_1;
	}
#else
	pr_err("Invalid FLED config\n");
#endif

	if ((buf == NULL) || kstrtouint(buf, 10, &value_u32) || (index == -1)) {
		pr_err("%s[%d]: FATAL ERROR. Return Fail\n", __FUNCTION__, __LINE__);
		return -EINVAL;
	}

	/* temp error canceling code */
	if (sm5708_fled != g_sm5708_fled) {
		dev_info(dev, "%s: sm5708_fled handler mismatched (g_handle:%p , l_handle:%p)\n", __func__, g_sm5708_fled, sm5708_fled);
		sm5708_fled = g_sm5708_fled;
	}

	dev_info(dev, "%s: value=%d\n", __func__, value_u32);
	/* We dont need lock here. Moreover, spinlock disables pre-emption */
	//spin_lock_irqsave(&fled_lock, flags);

	switch (value_u32) {
	case 0:
		// Reset early, so sm5708_fled_turn_off can use it
		assistive_light[index] = false;
#if defined(CONFIG_SEC_FACTORY)
		/*
		* 1. Work only for GPIO based FLED.
		* 2. Reset the Special Factory Mode setting
		*/
		if (sm5708_fled->pdata->led[index].used_gpio && factory_flash_test[index] == true) {
			/* ENABSTMR1:Enable | ABSTMR1:1.6 | FLED1EN: Enable by external control */
			ret = sm5708_write_reg(sm5708_fled->i2c, SM5708_REG_FLED1CNTL1, 0x1F);

			/* nONESHOT1:Enable | ONETIMER1:500 ms */
			ret = sm5708_write_reg(sm5708_fled->i2c, SM5708_REG_FLED1CNTL2, 0x14);
		}
#endif

		/* MUIC 5V -> 9V function */
		ret = __try_muic_torch_prepare(0);
		if (ret)
			goto EXIT;

		/* Turn off Torch */
		ret = sm5708_fled_turn_off(sm5708_fled, index);

		assistive_light[index] = false;
		break;
	case 1:
		/* MUIC 9V -> 5V function */
		ret = __try_muic_torch_prepare(1);
		if (ret)
			goto EXIT;

		/* Turn on Torch */
		/*enable flag first. to avoid timing issue on other threads that have access to flash driver */
		assistive_light[index] = true; 
		ret = sm5708_fled_turn_on_torch(sm5708_fled, index, 60);

		
		break;
	case 100:
		/* Factory mode Turn on Torch */
		/* MUIC 9V -> 5V function */
		ret = __try_muic_torch_prepare(1);
		if (ret)
			goto EXIT;

		/*Turn on Torch */
		ret = sm5708_fled_turn_on_torch(sm5708_fled, index, 240);

		assistive_light[index] = true;
		break;
#if defined(CONFIG_SEC_FACTORY)
	case 200:
		/* This is a special Factory Mode requirement, and may not be a real use-case */
		dev_info(dev, "%s: Special case for FACTORY to test FLASH with %d mA\n", __func__, value_u32);

		/* MUIC 9V -> 5V function */
		ret = __try_muic_torch_prepare(1);
		if (ret)
			goto EXIT;

		/* set the current */
		ret = sm5708_FLEDx_set_flash_current(sm5708_fled, index, value_u32);
		if (IS_ERR_VALUE(ret)) {
			dev_err(dev, "%s: fail to set FLED[%d] flash current (current_mA=%d)\n",
				__func__, index, value_u32);
			return ret;
		}

		/*
		* Charger event need to push first before gpio enable.
		* In flash capture mode, when connect some types charger or USB,
		* from pre-flash -> main-flash , torch on first and charger has
		* been change as torch mode, if enable flash first, it will work
		* on torch mode which use flash current , Vbus will pull down and
		* charger disconnect.
		*/
		sm5708_charger_oper_push_event(SM5708_CHARGER_OP_EVENT_FLASH, 1);

		/* Work only for GPIO based FLED */
		if (sm5708_fled->pdata->led[index].used_gpio) {
			/* ENABSTMR1:Disable | ABSTMR1:Dont Care | FLED1EN: Enable by external control */
			ret = sm5708_write_reg(sm5708_fled->i2c, SM5708_REG_FLED1CNTL1, 0x3);

			/* nONESHOT1:Disable | ONETIMER1:Dont Care */
			ret = sm5708_write_reg(sm5708_fled->i2c, SM5708_REG_FLED1CNTL2, 0x10);

			/* Enable the respective GPIOs */
			gpio_set_value(sm5708_fled->pdata->led[index].torch_en_pin, 0);
			gpio_set_value(sm5708_fled->pdata->led[index].flash_en_pin, 1);
		}

		assistive_light[index] = true;
		factory_flash_test[index] = true;
		break;
#endif
	default:
		if (value_u32 > 1000 && value_u32 < (1000 + 32)) {
			/* Turn on Torch : 20mA ~ 320mA */
#if 0
			/*
			* This is a very smart code to convert an offset(0-32) to corresponding mA value.
			* Unfortunately, we cannot use it now, because the app(FlashlightController.java)
			* does not comply with it.
			*/
			if (1 == index)
				value_u32 -= 1;
			ret = sm5708_fled_turn_on_torch(sm5708_fled, index, _calc_torch_current_mA_to_offset(value_u32 - 1000));
#else
			// get the right mA for the requested key
			{
				int i = 0;
				for (i = 0; i < MAX_FLASHLIGHT_LEVEL; i++) {
					if (calibData[i].level == value_u32)
						break;
				}

				if (i >= MAX_FLASHLIGHT_LEVEL) {
					dev_err(dev, "%s: can't process, invalid value=%d\n", __func__, value_u32);
					ret = -EINVAL;
					break;
				}
				else {
					ret = sm5708_fled_turn_on_torch(sm5708_fled, index, calibData[i].current_mA);
				}
			}

#endif

			assistive_light[index] = true;
		}
		else if (value_u32 >= 10 && value_u32 <= 320) {
			dev_info(dev, "%s: process, value=%d\n", __func__, value_u32);
			ret = sm5708_fled_turn_on_torch(sm5708_fled, index, value_u32);

			assistive_light[index] = true;
		}
		else {
			dev_err(dev, "%s: can't process, invalid value=%d\n", __func__, value_u32);
			ret = -EINVAL;
		}
		break;
	}

	if (IS_ERR_VALUE(ret)) {
		count = -1;
		dev_err(dev, "%s: fail to rear flash file operation:store (value=%d, ret=%d)\n", __func__, value_u32, ret);
	}

EXIT:
	return count;
}

static ssize_t sm5708_rear_flash_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	unsigned char offset = _calc_torch_current_offset_to_mA(320);
	int j = 0;
	int ret = 0;

	dev_info(dev, "%s: SM5708 Movie mode max current = 320mA(offset:%d)\n", __func__, offset);
	for (j = 0; j < 34; j++) {
		ret = _calc_torch_current_mA_to_offset(j);
		pr_info("%s:  For offset %d mA is %d\n", __FUNCTION__, j, ret);
	}
	return sprintf(buf, "%d\n", offset);
}

static DEVICE_ATTR(rear_flash, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH, sm5708_rear_flash_show, sm5708_rear_flash_store);
#if defined(CONFIG_REAR_DUAL_LED_FLASH)
static DEVICE_ATTR(rear_flash_2, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH, sm5708_rear_flash_show, sm5708_rear_flash_store);
#endif
#if defined(CONFIG_FRONT_LED_FLASH)
static DEVICE_ATTR(front_flash, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH, sm5708_rear_flash_show, sm5708_rear_flash_store);
#endif
#if defined(CONFIG_FRONT_DUAL_LED_FLASH)
static DEVICE_ATTR(front_flash_2, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH, sm5708_rear_flash_show, sm5708_rear_flash_store);
#endif

/**
 * SM5708 Flash-LED device driver management functions
 */

#ifdef CONFIG_OF
static int __parse_dt_common(struct device_node *np, struct sm5708_fled_platform_data *pdata, int index);

static inline int __get_child_count(const struct device_node *np)
{
	struct device_node *child;
	int num = 0;

	for_each_child_of_node(np, child)
		num++;

	return num;
}

static int __parse_dt_common(struct device_node *np, struct sm5708_fled_platform_data *pdata, int index)
{
	int ret = 0;
	unsigned int temp;

	ret = of_property_read_u32(np, "flash-mode-current-mA", &temp);
	if (ret) {
		pr_err("%s: fail to get dt:flash-mode-current-mA\n", __func__);
		return ret;
	}
	pdata->led[index].flash_current_mA = temp;

#if defined(CONFIG_LEDS_SEPARATE_PREFLASH)
	ret = of_property_read_u32(np, "preflash-mode-current-mA", &temp);
	if (ret) {
		pr_err("%s: fail to get dt:preflash-mode-current-mA\n", __func__);
		return ret;
	}
	pdata->led[index].preflash_current_mA = temp;
#endif

	ret = of_property_read_u32(np, "torch-mode-current-mA", &temp);
	if (ret) {
		pr_err("%s: fail to get dt:torch-mode-current-mA\n", __func__);
		return ret;
	}
	pdata->led[index].torch_current_mA = temp;

	ret = of_property_read_u32(np, "used-gpio-control", &temp);
	if (ret) {
		pr_err("%s: fail to get dt:used-gpio-control\n", __func__);
		return ret;
	}
	pdata->led[index].used_gpio = (bool)(temp & 0x1);

	if (pdata->led[index].used_gpio) {
		ret = of_get_named_gpio(np, "flash-en-gpio", 0);
		if (ret < 0) {
			pr_err("%s: fail to get dt:flash-en-gpio (ret=%d)\n", __func__, ret);
			return ret;
		}
		pdata->led[index].flash_en_pin = ret;

		ret = of_get_named_gpio(np, "torch-en-gpio", 0);
		if (ret < 0) {
			pr_err("%s: fail to get dt:torch-en-gpio (ret=%d)\n", __func__, ret);
			return ret;
		}
		pdata->led[index].torch_en_pin = ret;
	}

	return ret;
}

static int sm5708_fled_parse_dt(struct device *dev, struct sm5708_fled_platform_data *pdata)
{
	struct device_node *nproot = dev->of_node;
	struct device_node *np = NULL;
	int child_count = 0;
	int index = 0;
	int ret = 0;

	np = of_find_node_by_name(nproot, "sm5708_fled");
	if (unlikely(np == NULL)) {
		dev_err(dev, "%s: fail to find flash node\n", __func__);
		return ret;
	}

	/* child_count will be 0 if the property is a single node */
	child_count = __get_child_count(np);
	pr_info("%s[%d]: Child Count = %d\n", __FUNCTION__, __LINE__, child_count);

	if (child_count > 0)
	{
		struct device_node *c_np = NULL;

		for_each_child_of_node(np, c_np) {
			ret = of_property_read_u32(c_np, "id", &index);
			if (ret) {
				pr_err("%s: fail to get a id\n", __func__);
				return ret;
			}

			ret = __parse_dt_common(c_np, pdata, index);
		}
	}
	else
	{
		ret = of_property_read_u32(np, "id", &index);
		if (ret) {
			pr_err("%s : could not find led id\n", __func__);
			return ret;
		}

		ret = __parse_dt_common(np, pdata, index);
	}

	return ret;
}
#endif

static inline struct sm5708_fled_platform_data *_get_sm5708_fled_platform_data(struct device *dev, struct sm5708_dev *sm5708)
{
	struct sm5708_fled_platform_data *pdata;
	int i, ret = 0;

#ifdef CONFIG_OF
	pdata = devm_kzalloc(dev, sizeof(struct sm5708_fled_platform_data), GFP_KERNEL);
	if (unlikely(!pdata)) {
		dev_err(dev, "%s: fail to allocate memory for sm5708_fled_platform_data\n",	__func__);
		goto out_p;
	}

	ret = sm5708_fled_parse_dt(dev, pdata);
	if (IS_ERR_VALUE(ret)) {
		dev_err(dev, "%s: fail to parse dt for sm5708 flash-led (ret=%d)\n", __func__, ret);
		goto out_kfree_p;
	}
#else
	pdata = sm5708->pdata->fled_platform_data;
	if (unlikely(!pdata)) {
		dev_err(dev, "%s: fail to get sm5708_fled_platform_data\n", __func__);
		goto out_p;
	}
#endif

	dev_info(dev, "sm5708 flash-LED device platform data info,\n");
	for (i = 0; i < SM5708_FLED_MAX; ++i) {
		dev_info(dev, "[FLED-%d] Flash: %dmA, Torch: %dmA, used_gpio=%d, GPIO_PIN(%d, %d)\n",
			i, pdata->led[i].flash_current_mA,
			pdata->led[i].torch_current_mA, pdata->led[i].used_gpio,
			pdata->led[i].flash_en_pin, pdata->led[i].torch_en_pin);
	}

	return pdata;

out_kfree_p:
	devm_kfree(dev, pdata);
out_p:
	return NULL;
}

static int sm5708_fled_probe(struct platform_device *pdev)
{
	struct sm5708_dev *sm5708 = dev_get_drvdata(pdev->dev.parent);
	struct sm5708_fled_info *sm5708_fled;
	struct sm5708_fled_platform_data *sm5708_fled_pdata;
	struct device *dev = &pdev->dev;
	int i, ret = 0;

	if (IS_ERR_OR_NULL(camera_class)) {
		dev_err(dev, "%s: can't find camera_class sysfs object, didn't used rear_flash attribute\n",
			__func__);
		return -ENOENT;
	}

	sm5708_fled = devm_kzalloc(dev, sizeof(struct sm5708_fled_info), GFP_KERNEL);
	if (unlikely(!sm5708_fled)) {
		dev_err(dev, "%s: fail to allocate memory for sm5708_fled_info\n", __func__);
		return -ENOMEM;
	}

	dev_info(dev, "SM5708(rev.%d) Flash-LED devic driver Probing..\n",
		__get_revision_number());

	sm5708_fled_pdata = _get_sm5708_fled_platform_data(dev, sm5708);
	if (unlikely(!sm5708_fled_pdata)) {
		dev_info(dev, "%s: fail to get platform data\n", __func__);
		goto fled_platfrom_data_err;
	}

	sm5708_fled->dev = dev;
	sm5708_fled->i2c = sm5708->i2c;
	sm5708_fled->pdata = sm5708_fled_pdata;
	platform_set_drvdata(pdev, sm5708_fled);
	g_sm5708_fled = sm5708_fled;

	ret = sm5708_fled_initialize(sm5708_fled);
	if (IS_ERR_VALUE(ret)) {
		dev_err(dev, "%s: fail to initialize SM5708 Flash-LED[%d] (ret=%d)\n", __func__, i, ret);
		goto fled_init_err;
	}

	sm5708_fled->wqueue = create_singlethread_workqueue(dev_name(dev));
	if (!sm5708_fled->wqueue) {
		dev_err(dev, "%s: fail to Create Workqueue\n", __func__);
		goto fled_deinit_err;
	}

	/* create camera_class rear_flash device */
	sm5708_fled->rear_fled_dev = device_create(camera_class, NULL, 3, NULL, "flash");
	if (IS_ERR(sm5708_fled->rear_fled_dev)) {
		dev_err(dev, "%s fail to create device for rear_flash\n", __func__);
		goto fled_deinit_err;
	}
	sm5708_fled->rear_fled_dev->parent = dev;

	ret = device_create_file(sm5708_fled->rear_fled_dev, &dev_attr_rear_flash);
	if (IS_ERR_VALUE(ret)) {
		dev_err(dev, "%s fail to create device file for rear_flash\n", __func__);
		goto fled_rear_device_err;
	}
#if defined(CONFIG_REAR_DUAL_LED_FLASH)
    ret = device_create_file(sm5708_fled->rear_fled_dev, &dev_attr_rear_flash_2);
    if (IS_ERR_VALUE(ret)) {
        dev_err(dev, "%s fail to create device file for rear_flash_2\n", __func__);
        goto fled_rear_device_err2;
    }
#endif
#if defined(CONFIG_FRONT_LED_FLASH)
    ret = device_create_file(sm5708_fled->rear_fled_dev, &dev_attr_front_flash);
    if (IS_ERR_VALUE(ret)) {
        dev_err(dev, "%s fail to create device file for front_flash\n", __func__);
        goto fled_rear_device_err3;
    }
#endif
#if defined(CONFIG_FRONT_DUAL_LED_FLASH)
    ret = device_create_file(sm5708_fled->rear_fled_dev, &dev_attr_front_flash_2);
    if (IS_ERR_VALUE(ret)) {
        dev_err(dev, "%s fail to create device file for front_flash_2\n", __func__);
        goto fled_rear_device_err4;
    }
#endif
	pr_err("%s: Probe done.\n", __func__);

	return 0;

#if defined(CONFIG_FRONT_DUAL_LED_FLASH)
fled_rear_device_err4:
#endif
#if defined(CONFIG_FRONT_LED_FLASH)
    device_remove_file(sm5708_fled->rear_fled_dev, &dev_attr_front_flash);
fled_rear_device_err3:
#endif
#if defined(CONFIG_REAR_DUAL_LED_FLASH)
    device_remove_file(sm5708_fled->rear_fled_dev, &dev_attr_rear_flash_2);
fled_rear_device_err2:
#endif
	device_remove_file(sm5708_fled->rear_fled_dev, &dev_attr_rear_flash);

fled_rear_device_err:
	device_destroy(camera_class, sm5708_fled->rear_fled_dev->devt);

fled_deinit_err:
	sm5708_fled_deinitialize(sm5708_fled);

fled_init_err:
	platform_set_drvdata(pdev, NULL);
#ifdef CONFIG_OF
	devm_kfree(dev, sm5708_fled_pdata);
#endif

fled_platfrom_data_err:
	devm_kfree(dev, sm5708_fled);
	g_sm5708_fled = NULL;

	return ret;
}

static int sm5708_fled_remove(struct platform_device *pdev)
{
	struct sm5708_fled_info *sm5708_fled = platform_get_drvdata(pdev);
	struct device *dev = &pdev->dev;
	int i;

	device_remove_file(sm5708_fled->rear_fled_dev, &dev_attr_rear_flash);
#if defined(CONFIG_REAR_DUAL_LED_FLASH)
    device_remove_file(sm5708_fled->rear_fled_dev, &dev_attr_rear_flash_2);
#endif
#if defined(CONFIG_FRONT_LED_FLASH)
    device_remove_file(sm5708_fled->rear_fled_dev, &dev_attr_front_flash);
#endif
#if defined(CONFIG_FRONT_DUAL_LED_FLASH)
    device_remove_file(sm5708_fled->rear_fled_dev, &dev_attr_front_flash_2);
#endif
	device_destroy(camera_class, sm5708_fled->rear_fled_dev->devt);

	for (i = 0; i != SM5708_FLED_MAX; ++i) {
		sm5708_fled_turn_off(sm5708_fled, i);
	}
	sm5708_fled_deinitialize(sm5708_fled);

	platform_set_drvdata(pdev, NULL);
#ifdef CONFIG_OF
	devm_kfree(dev, sm5708_fled->pdata);
#endif
	devm_kfree(dev, sm5708_fled);

	return 0;
}

static void sm5708_fled_shutdown(struct device *dev)
{
	struct sm5708_fled_info *sm5708_fled = dev_get_drvdata(dev);
	int i;

	for (i = 0; i < SM5708_FLED_MAX; ++i) {
		sm5708_fled_turn_off(sm5708_fled, i);
	}
}

#ifdef CONFIG_OF
static struct of_device_id sm5708_fled_match_table[] = {
	{ .compatible = "siliconmitus,sm5708-fled",},
	{},
};
#else
#define sm5708_fled_match_table NULL
#endif

static struct platform_driver sm5708_fled_driver = {
	.probe		= sm5708_fled_probe,
	.remove		= sm5708_fled_remove,
	.driver		= {
		.name	= "sm5708-fled",
		.owner	= THIS_MODULE,
		.shutdown = sm5708_fled_shutdown,
		.of_match_table = sm5708_fled_match_table,
	},
};

static int __init sm5708_fled_init(void)
{
	printk("%s\n", __func__);
	return platform_driver_register(&sm5708_fled_driver);
}
module_init(sm5708_fled_init);

static void __exit sm5708_fled_exit(void)
{
	platform_driver_unregister(&sm5708_fled_driver);
}
module_exit(sm5708_fled_exit);

MODULE_DESCRIPTION("SM5708 FLASH-LED driver");
MODULE_ALIAS("platform:sm5708-flashLED");
MODULE_LICENSE("GPL");

