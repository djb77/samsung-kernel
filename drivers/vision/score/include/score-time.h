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

#ifndef SCORE_TIME_H_
#define SCORE_TIME_H_

#include <linux/types.h>
#include <linux/time.h>
#include <linux/ktime.h>
#include <linux/timekeeping.h>

#include "score-config.h"

static inline struct timespec score_time_get(void)
{
	struct timespec time;
	getrawmonotonic(&time);
	return time;
}

static inline unsigned long long score_time_diff(
		const struct timespec start, const struct timespec end)
{
	return timespec_to_ns(&end) - timespec_to_ns(&start);
}

static inline void score_time_print_diff(const char *prefix,
		const struct timespec start, const struct timespec end)
{
	unsigned long long diff = timespec_to_ns(&end) - timespec_to_ns(&start);
	score_note("%s: %llu.%09llu\n",
			prefix, diff / NSEC_PER_SEC, diff % NSEC_PER_SEC);
}

#endif
