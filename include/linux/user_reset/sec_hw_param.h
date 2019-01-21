/*
 * sec_hw_param.h
 *
 * COPYRIGHT(C) 2016-2017 Samsung Electronics Co., Ltd. All Right Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _SEC_HW_PARAM_H_
#define _SEC_HW_PARAM_H_

#define DEFAULT_LEN_STR		1023
#define SPECIAL_LEN_STR		2047
#define EXTRA_LEN_STR		((SPECIAL_LEN_STR) - (31 + 5))

#if defined(CONFIG_ARCH_SDM450)
#define NUM_PARAM0	(2)
#else
#define NUM_PARAM0	(2)
#endif

#define sysfs_scnprintf(buf, offset, fmt, ...)					\
do {										\
	offset += scnprintf(&(buf)[offset], DEFAULT_LEN_STR - (size_t)offset, fmt,	\
			##__VA_ARGS__);						\
} while (0)

#ifdef CONFIG_REGULATOR_CPR3

extern int cpr3_get_fuse_open_loop_voltage(int id, int fuse_corner);
extern int cpr3_get_fuse_corner_count(int id);
extern int cpr3_get_fuse_cpr_rev(int id);
extern int cpr3_get_fuse_speed_bin(int id);

#else

static inline int cpr3_get_fuse_open_loop_voltage(int id, int fuse_corner)
{
	return 0;
}
static inline int cpr3_get_fuse_corner_count(int id)
{
	return 0;
}
static inline int cpr3_get_fuse_cpr_rev(int id)
{
	return 0;
}
static inline int cpr3_get_fuse_speed_bin(int id)
{
	return 0;
}
#endif /* CONFIG_REGULATOR_CPR3 */

void battery_last_dcvs(int cap, int volt, int temp, int curr);

#endif /* _SEC_HW_PARAM_H_ */
