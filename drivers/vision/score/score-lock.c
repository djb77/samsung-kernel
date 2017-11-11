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

#include <linux/io.h>

#include "score-regs-user-def.h"
#include "score-lock.h"

// The SFRw address used in the bakery algorithm
#define BAKERY_CHOOSING_SCORE	SCORE_USERDEFINED58
#define BAKERY_CHOOSING_CPU	SCORE_USERDEFINED59
#define BAKERY_CHOOSING_IVA	SCORE_USERDEFINED60
#define BAKERY_NUM_SCORE	SCORE_USERDEFINED61
#define BAKERY_NUM_CPU		SCORE_USERDEFINED62
#define BAKERY_NUM_IVA		SCORE_USERDEFINED63

int max_num = 0;

// each device have own variables which are choosing and num.
// choosing and num are writable by a device only and
// readable by all other devices.
unsigned int CHOOSING[3] = {BAKERY_CHOOSING_SCORE,
	BAKERY_CHOOSING_CPU,
	BAKERY_CHOOSING_IVA
};
unsigned int NUM[3] = {BAKERY_NUM_SCORE,
	BAKERY_NUM_CPU,
	BAKERY_NUM_IVA
};

/// @brief  Lock by bakery algorithm.
/// @param  id Device id like SCORE, CPU or IVA. it is defined in score_lock.h
void score_bakery_lock(volatile void __iomem *addr, int id)
{
	int i, j;
	//while num is being set to non-zero value, choosing is 1
	writel(1, addr + CHOOSING[id]);

	//sets num to one higher then
	//all the nums of other devices
	for (i = 0; i < BAKERYNUM; i++) {
		max_num = (readl(addr + NUM[i]) > max_num) ?
			readl(addr + NUM[i]) : max_num;
	}

	writel(max_num + 1, addr + NUM[id]);
	writel(0, addr + CHOOSING[id]);

	for (j = 0; j < BAKERYNUM; j++) {
		if (j != id) {
			while (readl(addr + CHOOSING[j])) {
				// device first busy waits until device j is not
				// in the middle of choosing a num
			}

			// busy waits until process j's BAKERY_NUM is either zero
			// or greater than BAKERY_NUM[i]
			while (readl(addr + NUM[j]) != 0 &&
					((readl(addr + NUM[j]) < readl(addr + NUM[id])) ||
					 ((readl(addr + NUM[j]) == readl(addr + NUM[id])) &&
					  (j < id)))) {
			}
		}
	}
}

/// @brief  Unlock by bakery algorithm.
/// @param  id Device id like SCORE, CPU or IVA. it is defined in score_lock.h
void score_bakery_unlock(volatile void __iomem *addr, int id)
{
	//upon leaving the critical section, device id zeroes num.
	writel(0, addr + NUM[id]);
}
