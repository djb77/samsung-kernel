/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/plist.h>

#ifdef CONFIG_CGROUP_SCHEDTUNE
enum stune_group {
	STUNE_ROOT,
	STUNE_FOREGROUND,
	STUNE_BACKGROUND,
	STUNE_TOPAPP,
	STUNE_GROUP_COUNT,
};
#endif

struct gb_qos_request {
	struct plist_node node;
	char *name;
	bool active;
};

#ifdef CONFIG_SCHED_EHMP
extern void exynos_init_entity_util_avg(struct sched_entity *se);
extern int exynos_need_active_balance(enum cpu_idle_type idle,
		struct sched_domain *sd, int src_cpu, int dst_cpu);

extern unsigned long global_boost(void);
extern int find_second_max_cap(void);

extern int exynos_select_cpu(struct task_struct *p, int prev_cpu,
					int sync, int sd_flag);

extern void ontime_migration(void);
extern int ontime_can_migration(struct task_struct *p, int cpu);
extern void ontime_update_load_avg(int cpu, struct sched_avg *sa,
			u64 delta, unsigned int period_contrib, int weight);
extern void ontime_new_entity_load(struct task_struct *parent,
					struct sched_entity *se);
extern void ontime_trace_task_info(struct task_struct *p);
extern void ehmp_update_max_cpu_capacity(int cpu, unsigned long val);

extern void ehmp_update_overutilized(int cpu, unsigned long capacity);
extern bool ehmp_trigger_lb(int src_cpu, int dst_cpu);

extern void gb_qos_update_request(struct gb_qos_request *req, u32 new_value);

extern void request_kernel_prefer_perf(int grp_idx, int enable);
#else
static inline void exynos_init_entity_util_avg(struct sched_entity *se) { }
static inline int exynos_need_active_balance(enum cpu_idle_type idle,
		struct sched_domain *sd, int src_cpu, int dst_cpu) { return 0; }

static inline unsigned long global_boost(void) { return 0; }
static inline int find_second_max_cap(void) { return -EINVAL; }

static inline int exynos_select_cpu(struct task_struct *p,
						int prev_cpu) { return -EINVAL; }
static inline int exynos_select_cpu(struct task_struct *p, int prev_cpu,
					int sync, int sd_flag) { return -EINVAL; }

static inline void ontime_migration(void) { }
static inline int ontime_can_migration(struct task_struct *p, int cpu) { return 1; }
static inline void ontime_update_load_avg(int cpu, struct sched_avg *sa,
			u64 delta, unsigned int period_contrib, int weight) { }
static inline void ontime_new_entity_load(struct task_struct *p,
					struct sched_entity *se) { }
static inline void ontime_trace_task_info(struct task_struct *p) { }

static inline void ehmp_update_max_cpu_capacity(int cpu, unsigned long val) { }

static inline void ehmp_update_overutilized(int cpu, unsigned long capacity) { }
static inline bool ehmp_trigger_lb(int src_cpu, int dst_cpu) { return false; }

static inline void gb_qos_update_request(struct gb_qos_request *req, u32 new_value) { }

extern void request_kernel_prefer_perf(int grp_idx, int enable) { }
#endif /* CONFIG_SCHED_EHMP */
