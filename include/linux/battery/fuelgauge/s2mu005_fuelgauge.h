/*
 * s2mu005_fuelgauge.h
 * Samsung S2MU005 Fuel Gauge Header
 *
 * Copyright (C) 2015 Samsung Electronics, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __S2MU005_FUELGAUGE_H
#define __S2MU005_FUELGAUGE_H __FILE__

#if defined(ANDROID_ALARM_ACTIVATED)
#include <linux/android_alarm.h>
#endif

#include <linux/battery/sec_charging_common.h>

/* Slave address should be shifted to the right 1bit.
 * R/W bit should NOT be included.
 */

#define S2MU005_REG_STATUS		0x00
#define S2MU005_REG_IRQ			0x02
#define S2MU005_REG_RVBAT		0x04
#define S2MU005_REG_RCUR_CC		0x06
#define S2MU005_REG_RSOC		0x08
#define S2MU005_REG_MONOUT		0x0A
#define S2MU005_REG_MONOUT_SEL		0x0C
#define S2MU005_REG_RBATCAP		0x0E
#define S2MU005_REG_RZADJ		0x12
#define S2MU005_REG_RBATZ0		0x16
#define S2MU005_REG_RBATZ1		0x18
#define S2MU005_REG_IRQ_LVL		0x1A
#define S2MU005_REG_START		0x1E

enum {
	CURRENT_MODE = 0,
	LOW_SOC_VOLTAGE_MODE,
	HIGH_SOC_VOLTAGE_MODE,
	END_MODE,
};

struct sec_fg_info {
	/* test print count */
	int pr_cnt;
	/* full charge comp */
	/* struct delayed_work     full_comp_work; */
	u32 previous_fullcap;
	u32 previous_vffullcap;
	/* low battery comp */
	int low_batt_comp_flag;
	/* low battery boot */
	int low_batt_boot_flag;
	bool is_low_batt_alarm;

	/* battery info */
	int soc;
#if !defined(CONFIG_BATTERY_AGE_FORECAST)
	/* copy from platform data /
	 * DTS or update by shell script */
	int battery_table1[88];
	int battery_table2[22];
	int battery_table3[88];
	int battery_table4[22];
	int soc_arr_evt2[22];
	int ocv_arr_evt2[22];
	int batcap[4];
#endif
	/* miscellaneous */
	unsigned long fullcap_check_interval;
	int full_check_flag;
	bool is_first_check;
};

#if defined(CONFIG_BATTERY_AGE_FORECAST)
struct fg_age_data_info {
        int battery_table3[88]; // evt2
        int battery_table4[22]; // evt2
        int batcap[4];
        int accum[2];
	int soc_arr_val[22];
        int ocv_arr_val[22];
};

#define	fg_age_data_info_t \
	struct fg_age_data_info
#endif

struct s2mu005_platform_data {
	int capacity_max;
	int capacity_max_margin;
	int capacity_min;
	int capacity_calculation_type;
	int fuel_alert_soc;
	int fullsocthr;
	int fg_irq;
	int fg_log_enable;

	char *fuelgauge_name;

	bool repeated_fuelalert;

	struct sec_charging_current *charging_current;
};

struct s2mu005_fuelgauge_data {
	struct device           *dev;
	struct i2c_client       *i2c;
	struct i2c_client       *pmic;
	struct mutex            fuelgauge_mutex;
	struct s2mu005_platform_data *pdata;
	struct power_supply	psy_fg;
	/* struct delayed_work isr_work; */

	int cable_type;
	bool is_charging;
	int mode;
	int revision;

	/* HW-dedicated fuel guage info structure
	 * used in individual fuel gauge file only
	 * (ex. dummy_fuelgauge.c)
	 */
	struct sec_fg_info      info;
#if defined(CONFIG_BATTERY_AGE_FORECAST)
	fg_age_data_info_t*	age_data_info;
	int fg_num_age_step;
	int fg_age_step;
	int age_reset_status;
#endif
	bool is_fuel_alerted;
	struct wake_lock fuel_alert_wake_lock;

	unsigned int capacity_old;      /* only for atomic calculation */
	unsigned int capacity_max;      /* only for dynamic calculation */
	unsigned int standard_capacity;

	bool initial_update_of_soc;
	bool sleep_initial_update_of_soc;
	struct mutex fg_lock;
	struct delayed_work isr_work;

	/* register programming */
	int reg_addr;
	u8 reg_data[2];

	unsigned int pre_soc;
	int fg_irq;
	int diff_soc;
	int target_ocv;
	int vm_soc;
	bool cc_on;
	
	sec_bat_adc_table_data_t *temp_table;
	unsigned int temp_table_size;

	bool temperature_compensation;
};
#endif /* __S2MU005_FUELGAUGE_H */
