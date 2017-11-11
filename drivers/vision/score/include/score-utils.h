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

#ifndef SCORE_UTILS_H_
#define SCORE_UTILS_H_

static inline void score_bitmap_init(unsigned long *bitmap, unsigned int nbits)
{
	bitmap_zero(bitmap, nbits);
}

static inline void score_bitmap_clear_bit(unsigned long *bitmap, int bit)
{
	clear_bit(bit, bitmap);
}

static inline int score_bitmap_full(unsigned long *bitmap, unsigned int nbits)
{
	return bitmap_full(bitmap, nbits);
}

static inline int score_bitmap_empty(unsigned long *bitmap, unsigned int nbits)
{
	return bitmap_empty(bitmap, nbits);
}

unsigned int score_bitmap_g_zero_bit(unsigned long *bitmap, unsigned int nbits);

#endif
