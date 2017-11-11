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

#include "score-regs.h"
#include "score-profiler.h"

void __iomem *score_profile_reg;
void *score_profile_kvaddr;

int score_profile_open(struct score_system *system)
{
	score_profile_reg = system->regs + SCORE_DSP_INT;
	//score_profile_kvaddr = system->profile_kvaddr;
	score_profile_kvaddr = system->profile_kvaddr + 1024 * 10;  // 10k offset

	return 0;
}

void score_profile_close(void)
{
	score_profile_kvaddr = NULL;
	score_profile_reg = NULL;
}
