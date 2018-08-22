/*
 * Samsung Exynos5 SoC series Sensor driver
 *
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_PDP_H
#define FIMC_IS_PDP_H

#include "fimc-is-device-sensor.h"
#include "fimc-is-interface-sensor.h"

#define dbg_pdp(level, fmt, args...) \
	dbg_common(((debug_pdp) >= (level)) && (debug_pdp < 3), "[PDP]", fmt, ##args)

#define MAX_NUM_OF_PDP 2

int pdp_register(struct fimc_is_module_enum *module, int pdp_ch);
int pdp_unregister(struct fimc_is_module_enum *module);

#endif
