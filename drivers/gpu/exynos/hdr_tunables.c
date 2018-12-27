/*
 * linux/drivers/gpu/exynos/hdr_tunables.c
 *
 * Copyright (C) 2017 Samsung Electronics Co., Ltd.
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

#include <linux/module.h>
#include <linux/stat.h>

#include <video/exynos_hdr_tunables.h>

static unsigned int nr_tm_lut_x_values = NR_TM_LUT_VALUES;
static unsigned int nr_tm_lut_y_values = NR_TM_LUT_VALUES;

static uint tm_lut_x[NR_TM_LUT_VALUES] = {0,1,2,4,8,16,32,64,96,128,192,256,384,512,768,1024,1280,1536,2048,2560,3072,3328,3584,4096,5120,5632,6144,7168,8192,10240,12288,14336,2048};
static uint tm_lut_y[NR_TM_LUT_VALUES] = {0,33,45,61,82,109,141,181,211,234,271,302,353,396,466,524,575,621,699,762,802,820,830,848,882,896,915,949,967,995,1010,1018,5};
static uint tm_override_enable = 0;

int exynos_hdr_get_tm_lut_xy(u32 lut_x[], u32 lut_y[])
{
	int i;

	if (!tm_override_enable)
		return 0;

	for (i = 0; i < nr_tm_lut_x_values; i++)
		lut_x[i] = tm_lut_x[i] & 0x3FFF;
	for (; i < NR_TM_LUT_VALUES; i++)
		lut_x[i] = 0;

	for (i = 0; i < nr_tm_lut_y_values; i++)
		lut_y[i] = tm_lut_y[i] & 0xFFF;
	for (; i < NR_TM_LUT_VALUES; i++)
		lut_y[i] = 0;

	return NR_TM_LUT_VALUES;
}

int exynos_hdr_get_tm_lut(u32 lut[])
{
	int i = 0;

	if (!tm_override_enable)
		return 0;

	for (i = 0; i < nr_tm_lut_x_values; i++)
		lut[i] = tm_lut_x[i] & 0x3FFF;
	for (; i < NR_TM_LUT_VALUES; i++)
		lut[i] = 0;

	for (i = 0; i < nr_tm_lut_y_values; i++)
		lut[i] |= (tm_lut_y[i] & 0xFFF) << 16;

	return NR_TM_LUT_VALUES;
}

module_param_array(tm_lut_x, uint, &nr_tm_lut_x_values, 0644);
module_param_array(tm_lut_y, uint, &nr_tm_lut_y_values, 0644);
module_param(tm_override_enable, uint, 0644);
