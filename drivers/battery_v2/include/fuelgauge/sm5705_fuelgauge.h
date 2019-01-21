/*
 * drivers/battery/sm5705_fuelgauge.h
 *
 * Header of SiliconMitus SM5705 Fuelgauge Driver
 *
 * Copyright (C) 2015 SiliconMitus
 * Author: SW Jung
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef SM5705_FUELGAUGE_H
#define SM5705_FUELGAUGE_H

#include <linux/i2c.h>
//include <linux/mfd/sm5705.h>
#include "include/sec_charging_common.h"



#ifdef CONFIG_DEBUG_FS
#include <linux/debugfs.h>
#endif /* #ifdef CONFIG_DEBUG_FS */

#define FG_DRIVER_VER "0.0.0.1"
// #define ENABLE_FULL_OFFSET 1

/*To differentiate two battery Packs: SDI & ATL*/
enum {
	SDI_BATTERY_TYPE = 0,
	ATL_BATTERY_TYPE,
	UNKNOWN_TYPE
};

struct battery_data_t {
	const int battery_type; /* 4200 or 4350 or 4400*/
	const int battery_table[3][16];
	const int rce_value[3];
	const int dtcd_value;
	const int rs_value[4];
	const int vit_period;
	const int mix_value[2];
	const int topoff_soc[2];
	const int volt_cal;
	const int curr_cal;
	const int temp_std;
	const int temp_offset;
	const int temp_offset_cal;
};

struct sec_fg_info {
	/* Device_id */
	int device_id;
	/* State Of Connect */
	int online;
	/* battery SOC (capacity) */
	int batt_soc;
	/* battery voltage */
	int batt_voltage;
	/* battery AvgVoltage */
	int batt_avgvoltage;
	/* battery OCV */
	int batt_ocv;
	/* Current */
	int batt_current;
	/* battery Avg Current */
	int batt_avgcurrent;
	/* battery SOC cycle */
	int batt_soc_cycle;

	struct battery_data_t *comp_pdata;

	struct mutex param_lock;
	/* copy from platform data /
	 * DTS or update by shell script */

	struct mutex io_lock;
	struct device *dev;
	int32_t temperature;; /* 0.1 deg C*/
	int32_t temp_fg;; /* 0.1 deg C*/
	/* register programming */
	int reg_addr;
	u8 reg_data[2];

        int battery_typ;        /*SDI_BATTERY_TYPE or ATL_BATTERY_TYPE*/
        int batt_id_adc_check;
	int battery_table[3][16];
#if defined(CONFIG_BATTERY_AGE_FORECAST)
	int v_max_table[5];
	int q_max_table[5];
	int v_max_now;
	int q_max_now;
#endif
	int rce_value[3];
	int dtcd_value;
	int rs_value[5]; /*rs p_mix_factor n_mix_factor max min*/
	int vit_period;
	int mix_value[2]; /*mix_rate init_blank*/
	int misc;

	int enable_topoff_soc;
	int topoff_soc;
	int top_off;

	int cycle_high_limit;
	int cycle_low_limit;
	int cycle_limit_cntl;

	int enable_v_offset_cancel_p;
	int enable_v_offset_cancel_n;
	int v_offset_cancel_level;
	int v_offset_cancel_mohm;

	int volt_cal;
	int curr_offset;
	int p_curr_cal;
	int n_curr_cal;
	int curr_lcal_en;
	int curr_lcal_0;
	int curr_lcal_1;
	int curr_lcal_2;
	int en_auto_curr_offset;
	int cntl_value;
#ifdef ENABLE_FULL_OFFSET
	int full_offset_margin;
	int full_extra_offset;
#endif

	int temp_std;
	int en_fg_temp_volcal;
	int fg_temp_volcal_denom;
	int fg_temp_volcal_fact;
	int en_high_fg_temp_offset;
	int high_fg_temp_offset_denom;
	int high_fg_temp_offset_fact;
	int en_low_fg_temp_offset;
	int low_fg_temp_offset_denom;
	int low_fg_temp_offset_fact;
	int en_high_fg_temp_cal;
	int high_fg_temp_p_cal_denom;
	int high_fg_temp_p_cal_fact;
	int high_fg_temp_n_cal_denom;
	int high_fg_temp_n_cal_fact;
	int en_low_fg_temp_cal;
	int low_fg_temp_p_cal_denom;
	int low_fg_temp_p_cal_fact;
	int low_fg_temp_n_cal_denom;
	int low_fg_temp_n_cal_fact;
	int en_high_temp_cal;
	int high_temp_p_cal_denom;
	int high_temp_p_cal_fact;
	int high_temp_n_cal_denom;
	int high_temp_n_cal_fact;
	int en_low_temp_cal;
	int low_temp_p_cal_denom;
	int low_temp_p_cal_fact;
	int low_temp_n_cal_denom;
	int low_temp_n_cal_fact;


	int battery_type; /* 4200 or 4350 or 4400*/
	int data_ver;
	uint32_t soc_alert_flag : 1;  /* 0 : nu-occur, 1: occur */
	uint32_t volt_alert_flag : 1; /* 0 : nu-occur, 1: occur */
	uint32_t flag_full_charge : 1; /* 0 : no , 1 : yes*/
	uint32_t flag_chg_status : 1; /* 0 : discharging, 1: charging*/
	uint32_t flag_charge_health : 1; /* 0 : no , 1 : good*/

	int32_t irq_ctrl;
	int value_v_alarm;

	uint32_t is_FG_initialised;
	int iocv_error_count;

	int n_tem_poff;
	int n_tem_poff_offset;
	int l_tem_poff;
	int l_tem_poff_offset;

	/* previous battery voltage current*/
	int p_batt_voltage;
	int p_batt_current;

};

struct sec_fuelgauge_info {
	struct i2c_client		*client;
	sec_battery_platform_data_t *pdata;
	struct power_supply		psy_fg;
	struct delayed_work isr_work;

	int cable_type;
	bool is_charging;
	bool ta_exist;

	/* HW-dedicated fuel guage info structure
	 * used in individual fuel gauge file only
	 * (ex. dummy_fuelgauge.c)
	 */
	struct sec_fg_info	info;

	bool is_fuel_alerted;
	bool volt_alert_flag;
	struct wake_lock fuel_alert_wake_lock;

	unsigned int capacity_old;	/* only for atomic calculation */
	unsigned int capacity_max;	/* only for dynamic calculation */
#if defined(CONFIG_BATTERY_AGE_FORECAST)
	unsigned int chg_full_soc; /* BATTERY_AGE_FORECAST */
#endif

	bool initial_update_of_soc;
	struct mutex fg_lock;

	/* register programming */
	int reg_addr;
	u8 reg_data[2];

	int fg_irq;
};

ssize_t sm5705_fg_show_attrs(struct device *dev,
				struct device_attribute *attr, char *buf);

ssize_t sm5705_fg_store_attrs(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count);


#ifdef CONFIG_OF
extern void board_fuelgauge_init(void *fuelgauge);
extern bool sec_bat_check_jig_status(void);
#endif

#define SM5705_FG_ATTR(_name)				\
{							\
	.attr = {.name = #_name, .mode = 0664},	\
	.show = sm5705_fg_show_attrs,			\
	.store = sm5705_fg_store_attrs,			\
}

enum {
	FG_REG = 0,
	FG_DATA,
	FG_REGS,
};

#endif // SM5705_FUELGAUGE_H
