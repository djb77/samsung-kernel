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

#ifndef _SCORE_PROFILER_H_
#define _SCORE_PROFILER_H_

#include "score-system.h"

#define PROFILE_BUF_SIZE	0x34000

extern void *score_profile_kvaddr;

int score_profile_open(struct score_system *system);
void score_profile_close(void);

#endif

