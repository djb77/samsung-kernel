/*
 * Copyright (c) 2016 Park Bumgyu, Samsung Electronics Co., Ltd <bumgyu.park@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * General Exynos wd dvfs driver implementation
 */

#ifdef CONFIG_EXYNOS_WD_DVFS
void exynos_wd_call_chain(void);
#else
#define exynos_wd_call_chain() do {} while(0)
#endif

