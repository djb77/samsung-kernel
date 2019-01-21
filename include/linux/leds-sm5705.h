/*
 * Flash-LED device driver for SM5705
 *
 * Copyright (C) 2015 Silicon Mitus 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 * 
 */

#ifndef __LEDS_SM5705_H__
#define __LEDS_SM5705_H__

#include <linux/leds/msm_ext_pmic_flash.h>

#define __MANGLE_NAME(_f_) _f_##_sm5705

enum {
    SM5705_FLED_0   = 0x0,
    SM5705_FLED_1,
    SM5705_FLED_MAX,
};

struct sm5705_fled_platform_data {
    struct {
        unsigned short flash_current_mA;
        unsigned short torch_current_mA;
        unsigned short preflash_current_mA;
        bool used_gpio;
        int flash_en_pin;
        int torch_en_pin;
    }led[SM5705_FLED_MAX];
};
#if 0 /* M OS - do not used it */
int sm5705_fled_prepare_flash(unsigned char index);
int sm5705_fled_close_flash(unsigned char index);
#endif

// For msm_led_trigger
int sm5705_fled_torch_on(unsigned char index);
int sm5705_fled_flash_on(unsigned char index);
int sm5705_fled_preflash(unsigned char index);

int sm5705_fled_led_off(unsigned char index);
int sm5705_fled_muic_camera_flash_work_on(void);
int sm5705_fled_muic_camera_flash_work_off(void);
int sm5705_fled_pre_flash_on(unsigned char index, int32_t pre_flash_current_mA);
int sm5705_fled_flash_on_set_current(unsigned char index, int32_t flash_current_mA);

// For msm_flash
int msm_fled_led_off_sm5705(ext_pmic_flash_ctrl_t *flash_ctrl);
int msm_fled_torch_on_sm5705(ext_pmic_flash_ctrl_t *flash_ctrl);
int msm_fled_flash_on_sm5705(ext_pmic_flash_ctrl_t *flash_ctrl);
int msm_fled_pre_flash_on_sm5705(ext_pmic_flash_ctrl_t *flash_ctrl);
int msm_fled_flash_on_set_current_sm5705(ext_pmic_flash_ctrl_t *flash_ctrl);
#endif
