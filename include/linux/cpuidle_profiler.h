/*
 * Copyright (C) 2015 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef CPUIDLE_PROFILE_H
#define CPUIDLE_PROFILE_H __FILE__

#include <linux/ktime.h>
#include <linux/cpuidle.h>
#include <linux/psci.h>

#include <asm/cputype.h>

/*
 * cpuidle major state
 */
#define PROFILE_C1		0
#define PROFILE_C2		1
#define PROFILE_SYS		2

#define has_sub_state(_state)	( _state > PROFILE_C1 ? 1 : 0)

/*
 * C2 sub state
 */
#define C2_CPD			PSCI_CLUSTER_SLEEP
#define C2_SICD			PSCI_SYSTEM_IDLE

#define to_cluster(cpu)		MPIDR_AFFINITY_LEVEL(cpu_logical_map(cpu), 1)

struct cpuidle_profile_state_usage {
	unsigned int entry_count;
	unsigned int early_wakeup_count;
	unsigned long long time;
};

struct cpuidle_profile_info {
	ktime_t last_entry_time;
	int cur_state;
	int state_count;

	struct cpuidle_profile_state_usage *usage;
};

extern void cpuidle_profile_start(int cpu, int state, int sub_state);
extern void cpuidle_profile_finish(int cpuid, int early_wakeup);
extern void cpuidle_profile_register(struct cpuidle_driver *drv);

#ifdef CONFIG_CPU_IDLE
extern void cpuidle_profile_collect_idle_ip(int mode,
				int index, unsigned int idle_ip);
#else
static inline void cpuidle_profile_collect_idle_ip(int mode,
				int index, unsigned int idle_ip)
{
	/* Nothing to do */
}
#endif

#endif /* CPUIDLE_PROFILE_H */
