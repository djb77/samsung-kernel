/*
 * maxim_dsm_power.c -- Module for Power Measurement
 *
 * Copyright 2015 Maxim Integrated Products
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __SOUND_MAXIM_DSM_POWER_H__

#define MAXDSM_POWER_WQ_NAME	"maxdsm_power_wq"
#define MAXDSM_POWER_CLASS_NAME "maxdsm_power"
#define MAXDSM_POWER_DSM_NAME	"dsm"

#define MAXDSM_POWER_START_DELAY 1000

enum maxdsm_ppr_channel {
	MAXDSM_LEFT = 0,
	MAXDSM_RIGHT = 1,
	MAXDSM_CHANNEL = 2,
};

enum maxdsm_ppr_state {
	PPR_OFF = 0,
	PPR_ON,
	PPR_NORMAL,
	PPR_RUN,
};

struct maxim_dsm_power_info {
	int interval;
	int duration;
	int remaining;
	unsigned long previous_jiffies;
};

struct maxim_dsm_power_values {
	uint32_t status;
	int power;
	int power_r;
	uint64_t avg;
	uint64_t avg_r;
	int count;
};

struct maxim_dsm_power_ppr_info {
	int interval;
	int duration;
	int target_min_time_ms;
	int global_enable;
	unsigned long previous_jiffies;
	unsigned long amp_uptime_jiffies;
};

struct maxim_dsm_ppr_values {
	int status;
	int target_temp;
	int exit_temp;
	int cutoff_frequency;
	int threshold_level;
	int env_temp;
	int spk_t;
};

struct maxim_dsm_power {
	struct device *dev;
	struct class *class;
	struct mutex mutex;
	struct workqueue_struct *wq;
	struct delayed_work work;
	struct delayed_work work_ppr;
	struct maxim_dsm_power_values values;
	struct maxim_dsm_power_info info;
	struct maxim_dsm_ppr_values values_ppr[MAXDSM_CHANNEL];
	struct maxim_dsm_power_ppr_info info_ppr;
	uint32_t platform_type;
};

void maxdsm_power_ppr_control(int state);
void maxdsm_power_control(int state);

#endif /* __SOUND_MAXIM_DSM_POWER_H__ */
