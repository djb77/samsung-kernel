/*
 * Copyright (C) 2017 Samsung Electronics. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#ifndef FINGERPRINT_H_
#define FINGERPRINT_H_

#include <linux/clk.h>
#include "fingerprint_sysfs.h"

/* fingerprint debug timer */
#define FPSENSOR_DEBUG_TIMER_SEC (10 * HZ)

/* For Sensor Type Check */
enum {
	SENSOR_OOO = -2,
	SENSOR_UNKNOWN,
	SENSOR_FAILED,
	SENSOR_VIPER,
	SENSOR_RAPTOR,
	SENSOR_EGIS,
	SENSOR_VIPER_WOG,
	SENSOR_NAMSAN,
	SENSOR_CPID,
	SENSOR_MAXIMUM,
};

#define SENSOR_STATUS_SIZE 9
static char sensor_status[SENSOR_STATUS_SIZE][10] = {"ooo", "unknown",
	"failed", "viper", "raptor", "egis", "viper_wog", "namsan", "cpid"};

#endif
