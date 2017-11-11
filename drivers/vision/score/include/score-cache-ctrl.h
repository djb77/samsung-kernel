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

#ifndef _SCORE_CACHE_CTRL_H_
#define _SCORE_CACHE_CTRL_H_

#include "../score-system.h"
#include "../score-fw-queue.h"
#include "score-regs.h"

/// @brief  flush data cache
void score_dcache_flush(struct score_system *system);

/// @brief  flush some region of data cache of SCore
/// @param  addr  start address of dcache flush region (device virtual address)
/// @param  size  size of dcache flush data (bytes)
void score_dcache_flush_region(struct score_system *system, void *addr, unsigned int size);

#endif
