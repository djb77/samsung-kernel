/* tc300k.h -- Linux driver for coreriver chip as touchkey
 *
 * Copyright (C) 2013 Samsung Electronics Co.Ltd
 * Author: Junkyeong Kim <jk0430.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 */

#ifndef __LINUX_TC300K_H
#define __LINUX_TC300K_H

#define TC300K_NAME "sec_touchkey_driver"

struct tc300k_platform_data {
	int gpio_int;
	int gpio_sda;
	int gpio_scl;
	int gpio_pwr;
	int gpio_en;
	int gpio_sub_det;
	int i2c_gpio;
	int (*power) (void *, bool on);
	int (*power_isp) (bool on);
	int (*keyled) (void *, bool on);
	u32 irq_gpio_flags;
	u32 sda_gpio_flags;
	u32 scl_gpio_flags;
	const char *regulator_ic;
	const char *regulator_led;
	bool boot_on_ldo;

	int *keycode;
	int key_num;
	const char *fw_name;
	u32 sensing_ch_num;
	u32 use_bitmap;
	u32 tsk_ic_num;
	bool touchkey_led;
};

#endif /* __LINUX_TC300K_H */
