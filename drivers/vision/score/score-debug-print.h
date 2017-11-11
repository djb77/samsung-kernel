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

#ifndef _SCORE_DEBUG_PRINT_H_
#define _SCORE_DEBUG_PRINT_H_

#include <linux/spinlock.h>
#include "score-fw-queue.h"

#define MAX_PRINT_BUF_SIZE		(128)
#define SCORE_LOG_BUF_SIZE		SZ_1M
#define SCORE_PRINT_TIME_STEP		(HZ/10)

struct score_print_buf {
	volatile int			*front;
	volatile int			*rear;
	volatile unsigned char		*kva;
	int				size;
	struct timer_list		timer;
	int				init;
};

struct score_system;

void score_print_probe(struct score_system *system);
void score_print_release(struct score_system *system);
int score_print_open(struct score_system *system);
void score_print_close(struct score_system *system);
void score_print_flush(struct score_system *system);

#endif
