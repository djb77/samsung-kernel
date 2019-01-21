/*
 * include/linux/sec_monitor_battery_removal.h
 *
 * COPYRIGHT(C) 2016 Samsung Electronics Co., Ltd. All Right Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef SEC_MONITOR_BATTERY_REMOVAL_H
#define SEC_MONITOR_BATTERY_REMOVAL_H

enum sec_normal_poweroff_t {
	ABNORMAL_POWEROFF,
	NORMAL_POWEROFF,
};

#ifdef CONFIG_SEC_MONITOR_BATTERY_REMOVAL
extern void sec_set_normal_pwroff(int value);
#else
static inline void sec_set_normal_pwroff(int value) {}
#endif

#endif /* SEC_MONITOR_BATTERY_REMOVAL_H */
