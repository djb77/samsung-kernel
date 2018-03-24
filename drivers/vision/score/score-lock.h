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

#ifndef _SCORE_LOCK_H
#define _SCORE_LOCK_H

// The number of heterogeneous devices using lock
#define BAKERYNUM	(3)
/// @enum bakery_id
///
/// devices is numbered 0,1,2...,BAKERYNUM
enum bakery_id{
	SCORE_ID =	0x0,
	CPU_ID =	0x1,
	IVA_ID =	0x2
};

/// @brief  Lock by bakery algorithm.
/// @param  id Device id like SCORE, CPU or IVA.
void score_bakery_lock(volatile void __iomem *addr, int id);

/// @brief  Unlock by bakery algorithm.
/// @param  id Device id like SCORE, CPU or IVA.
void score_bakery_unlock(volatile void __iomem *addr, int id);

#ifdef ENABLE_BAKERY_LOCK
#define SCORE_BAKERY_LOCK(addr, id)	score_bakery_lock(addr, id)
#define SCORE_BAKERY_UNLOCK(addr, id)	score_bakery_unlock(addr, id)
#else
#define SCORE_BAKERY_LOCK(addr, id)
#define SCORE_BAKERY_UNLOCK(addr, id)
#endif

#endif
