/*
 * Samsung Exynos SoC series SCORE driver
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/exynos_iovmm.h>
#include <linux/pm_runtime.h>

#include "score-config.h"
#include "score-utils.h"

unsigned int score_bitmap_g_zero_bit(unsigned long *bitmap, unsigned int nbits)
{
	int bit;
	int old_bit;

	while ((bit = find_first_zero_bit(bitmap, nbits)) != nbits) {
		old_bit = test_and_set_bit(bit, bitmap);
		if (!old_bit) {
			return bit;
		}
	}

	return nbits;
}
