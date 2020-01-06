/*
 * Copyright (c) 2016 Park Bumgyu, Samsung Electronics Co., Ltd <bumgyu.park@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * General Exynos cpufreq driver implementation
 */

#ifdef CONFIG_ARM_EXYNOS_ACME
extern unsigned int exynos_cpufreq_get_max_freq(struct cpumask *mask);
extern bool exynos_cpufreq_allow_change_max(unsigned int cpu, unsigned long max);
#else
static inline unsigned int exynos_cpufreq_get_max_freq(struct cpumask *mask)
{
	return 0;
}
static inline bool exynos_cpufreq_allow_change_max(unsigned int cpu, unsigned long max)
{
	return 0;
}
#endif
