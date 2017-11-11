/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * EXYNOS IPs Traffic Monitor Driver for Samsung EXYNOS SoC
 * By Hosung Kim (hosung0.kim@samsung.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef EXYNOS_ITMON__H
#define EXYNOS_ITMON__H

#ifdef CONFIG_EXYNOS_ITMON
struct itmon_notifier {
	char *init_desc;
	char *target_desc;
	char *masterip_desc;
	unsigned int masterip_idx;
	unsigned long target_addr;
};
extern void itmon_notifier_chain_register(struct notifier_block *n);
extern void itmon_enable(bool enabled);
extern void itmon_set_errcnt(int cnt);
#else
static inline void itmon_enable(bool enabled)
{
	return;
}
#define itmon_notifier_chain_register(x)		do { } while(0)
#define itmon_set_errcnt(x) 				do { } while(0)
#endif

#endif
