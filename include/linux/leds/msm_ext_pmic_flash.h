/* include/linux/leds/msm_ext_pmic_flash.h
 *
 * Header for msm_ext_pmic_flash.
 *
 * Copyright (C) 2016 Samsung Electronics
 *
 * This code is an extension to msm_flash.h.
 * msm_flash.c needs extra work to communicate with
 * the external LED chipset. ext_pmic_flash_func can
 * be extended to support custom functions in the respective
 * driver.
 * ext_pmic_flash_func will work with FLASH_DRIVER_EXT_PMIC.
 */

#ifndef LINUX_MSM_EXT_PMIC_FLASH_H
#define LINUX_MSM_EXT_PMIC_FLASH_H

#define IS_FLASH_VALID(p)               \
    if (IS_ERR_OR_NULL(p))              \
    {                                   \
        pr_err("Invalid Flash Object"); \
        WARN_ON(1);                     \
        return -ENODEV;                 \
    }                                   \

typedef struct ext_pmic_flash_ctrl {
    int index;
    int flash_current_mA;
} ext_pmic_flash_ctrl_t;

typedef struct ext_pmic_flash_func {
    int32_t (*ext_pmic_flash_on)(ext_pmic_flash_ctrl_t *);
    int32_t (*ext_pmic_torch_on)(ext_pmic_flash_ctrl_t *);
    int32_t (*ext_pmic_led_off)(ext_pmic_flash_ctrl_t *);
    int32_t(*ext_pmic_flash_on_set_current)(ext_pmic_flash_ctrl_t *);
    int32_t(*ext_pmic_pre_flash_on)(ext_pmic_flash_ctrl_t *);
} ext_pmic_flash_func_t;

#endif /* LINUX_MSM_EXT_PMIC_FLASH_H */
