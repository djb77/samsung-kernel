/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __VPU_GEN_H__
#define __VPU_GEN_H__

#if defined(__KERNEL__)


#else
#define ARRAY_SIZE(x)	(sizeof(x) / sizeof((x)[0]))
#define min(_a, _b)       (((_a) < (_b)) ? (_a) : (_b))
#define max(_a, _b)      (((_a) > (_b)) ? (_a) : (_b))


#endif /* __linux__ */

#define PTR2UINT(_p)		((uintptr_t)(_p))
#define UINT2PTR(_val)		((void *)(uintptr_t)(_val))

#define MV_PTR(_p, _off)	(void *)((uint8_t *)(_p) + (_off))

#define TRUE	1
#define FALSE	0

#endif /* __VPU_GEN_H__ */
