/*
 * maxim_dsm_cal.c -- Module for Rdc calibration
 *
 * Copyright 2014 Maxim Integrated Products
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __SOUND_MAXIM_DSM_CAL_H__
#define __SOUND_MAXIM_DSM_CAL_H__

#define DRIVER_AUTHOR		"Kyounghun Jeon<hun.jeon@maximintegrated.com>"
#define DRIVER_DESC			"For Rdc calibration of MAX98xxx"
#define DRIVER_SUPPORTED	"MAX98xxx"

#define WQ_NAME				"maxdsm_wq"

#define FILEPATH_TEMP_CAL	"/efs/maxim/temp_cal"
#define FILEPATH_RDC_CAL	"/efs/maxim/rdc_cal"
#define FILEPATH_RDC_CAL_R	"/efs/maxim/rdc_cal_r"

#define CLASS_NAME			"maxdsm_cal"
#define DSM_NAME			"dsm"

#define ADDR_RDC			0x2A0050
#define ADDR_FEATURE_ENABLE 0x2A006A

struct maxim_dsm_cal_info {
	uint32_t min;
	uint32_t max;
	uint32_t feature_en;
	int interval;
	int duration;
	int remaining;
	int ignored_t;
	unsigned long previous_jiffies;
};

struct maxim_dsm_cal_values {
	uint32_t status;
	int rdc;
	int rdc_r;
	int temp;
	uint64_t avg;
	uint64_t avg_r;
	int count;
};

struct maxim_dsm_info_v_validation {
	int duration;
	int screen_duration;
	int screen_remaining;
	int screen_interval;
	unsigned long screen_previous_jiffies;
};

struct maxim_dsm_values_v_validation {
	uint32_t status;
	uint32_t amp_screen_status;
	int boost_bypass;
	int v_validation;
	int v_validation_r;
	int amp_screen_validation;
	int amp_screen_validation_r;
};

struct maxim_dsm_values_thermal_safe {
	int enable;
	uint32_t thermal_min_gain;
};


struct maxim_dsm_cal {
	struct device *dev;
	struct class *class;
	struct mutex mutex;
	struct workqueue_struct *wq;
	struct delayed_work work;
	struct delayed_work work_v_validation;
	struct delayed_work work_amp_screen_validation;
	struct maxim_dsm_cal_values values;
	struct maxim_dsm_cal_info info;
	struct maxim_dsm_info_v_validation info_v_validation;
	struct maxim_dsm_values_v_validation values_v_validation;
	struct maxim_dsm_values_thermal_safe values_thermal_safe_mode;
	struct regmap *regmap;
	uint32_t platform_type;
};

extern struct class *maxdsm_cal_get_class(void);
extern struct regmap *maxdsm_cal_set_regmap(
		struct regmap *regmap);
extern int maxdsm_cal_get_temp(int *temp);
extern int maxdsm_cal_set_temp(int temp);
extern int maxdsm_cal_get_rdc(int *rdc);
extern int maxdsm_cal_get_rdc_r(int *rdc);
extern int maxdsm_cal_set_rdc(int rdc);
extern int maxdsm_cal_set_rdc_r(int rdc);
extern int maxdsm_cal_get_safe_mode(void);
extern uint32_t maxdsm_cal_get_thermal_min_gain(void);
extern void maxdsm_cal_set_thermal_min_gain(uint32_t thermal_min_gain);
extern int maxdsm_cal_get_temp_from_power_supply(void);
extern void max98512_dc_blocker_enable(int enable);
extern void max98512_boost_bypass(int mode);

#endif /* __SOUND_MAXIM_DSM_CAL_H__ */
