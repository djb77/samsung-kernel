/*
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * EXYNOS - CPU Hotplug support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __EXYNOS_CPU_HOTPLUG_H
#define __EXYNOS_CPU_HOTPLUG_H __FILE__

#define FAST_HP		0xFA57

struct kobject *exynos_cpu_hotplug_kobj(void);
bool exynos_cpu_hotplug_enabled(void);
void exynos_cpu_hotplug_gov_activated(void);

void exynos_hpgov_update_rq_load(int cpu);
int exynos_hpgov_update_cpu_capacity(int cpu);
#ifdef CONFIG_EXYNOS_HOTPLUG_GOVERNOR
void exynos_hpgov_validate_hpin(unsigned int cpu);
void exynos_hpgov_validate_scale(unsigned int cpu, unsigned int target_freq);
#else
static inline void exynos_hpgov_validate_hpin(unsigned int cpu) {};
static inline void exynos_hpgov_validate_scale(unsigned int cpu, unsigned int target_freq) {};
#endif

#define UPDATE_ONLINE_CPU (1)
static BLOCKING_NOTIFIER_HEAD(exynos_cpuhotplug_notifier_list);
int exynos_cpuhotplug_register_notifier(struct notifier_block *nb, unsigned int list);
int exynos_cpuhotplug_unregister_notifier(struct notifier_block *nb, unsigned int list);

#endif /* __EXYNOS_CPU_HOTPLUG_H */
