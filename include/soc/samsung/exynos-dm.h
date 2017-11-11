/* linux/include/soc/samsung/exynos-dm.h
 *
 * Copyright (C) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * EXYNOS5 - Header file for exynos DVFS Manager support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __EXYNOS_DM_H
#define __EXYNOS_DM_H

#define EXYNOS_DM_MODULE_NAME		"exynos-dm"
#define EXYNOS_DM_TYPE_NAME_LEN		16

#define EXYNOS_DM_RELATION_L		0
#define EXYNOS_DM_RELATION_H		1

enum exynos_dm_type {
	DM_CPU_CL0 = 0,
	DM_CPU_CL1,
	DM_MIF,
	DM_INT,
	DM_INTCAM,
	DM_DISP,
	DM_CAM,
	DM_GPU,
	DM_TYPE_END
};

static const char dm_type_name[DM_TYPE_END][EXYNOS_DM_TYPE_NAME_LEN] = {
	"dm_cpu_cl0",
	"dm_cpu_cl1",
	"dm_mif",
	"dm_int",
	"dm_intcam",
	"dm_disp",
	"dm_cam",
	"dm_gpu",
};

enum exynos_dvfs_type {
	DVFS_CPUFREQ = 0,
	DVFS_DEVFREQ,
	DVFS_GPU,
	DVFS_TYPE_END
};

enum exynos_constraint_type {
	CONSTRAINT_MIN = 0,
	CONSTRAINT_MAX,
	CONSTRAINT_END
};

enum dvfs_direction {
	DOWN = 0,
	UP,
	DIRECTION_END
};

struct exynos_dm_freq {
	u32				master_freq;
	u32				constraint_freq;
};

struct exynos_dm_constraint {
	struct list_head		node;

	bool				guidance;		/* check constraint table by hw guide */
	u32				table_length;

	enum exynos_constraint_type	constraint_type;
	enum exynos_dm_type		constraint_dm_type;
	char				dm_type_name[EXYNOS_DM_TYPE_NAME_LEN];
	struct exynos_dm_freq		*freq_table;
	u32				min_freq;
	u32				max_freq;

	struct exynos_dm_constraint	*sub_constraint;
};

struct exynos_dm_data {
	bool				available;		/* use for DVFS domain available */
#ifdef CONFIG_EXYNOS_ACPM
	bool				policy_use;
#endif
	enum exynos_dm_type		dm_type;
	enum exynos_dvfs_type		dvfs_type;
	char				dm_type_name[EXYNOS_DM_TYPE_NAME_LEN];

	u32				min_freq;
	u32				max_freq;
	u32				cur_freq;
	u32				target_freq;

	u32				gov_min_freq;

	u32				policy_min_freq;
	u32				policy_max_freq;

	int (*freq_scaler)		(enum exynos_dm_type dm_type, u32 target_freq, unsigned int relation);

	struct list_head		min_clist;
	struct list_head		max_clist;
	u32				constraint_checked;
#ifdef CONFIG_EXYNOS_ACPM
	u32				cal_id;
#endif
};

struct exynos_dm_device {
	struct device			*dev;
	struct mutex			lock;
	struct exynos_dm_data		dm_data[DM_TYPE_END];
};

/* External Function call */
#if defined(CONFIG_EXYNOS_DVFS_MANAGER)
int exynos_dm_data_init(enum exynos_dm_type dm_type,
			u32 min_freq, u32 max_freq, u32 cur_freq);
int register_exynos_dm_constraint_table(enum exynos_dm_type dm_type,
				struct exynos_dm_constraint *constraint);
int unregister_exynos_dm_constraint_table(enum exynos_dm_type dm_type,
				struct exynos_dm_constraint *constraint);
int register_exynos_dm_freq_scaler(enum exynos_dm_type dm_type,
			int (*scaler_func)(enum exynos_dm_type dm_type, u32 target_freq, unsigned int relation));
int unregister_exynos_dm_freq_scaler(enum exynos_dm_type dm_type);
int policy_update_call_to_DM(enum exynos_dm_type dm_type, u32 min_freq, u32 max_freq);
int DM_CALL(enum exynos_dm_type dm_type, unsigned long *target_freq);
#else
static inline
int exynos_dm_data_init(enum exynos_dm_type dm_type,
			u32 min_freq, u32 max_freq, u32 cur_freq)
{
	return 0;
}
static inline
int register_exynos_dm_constraint_table(enum exynos_dm_type dm_type,
				struct exynos_dm_constraint *constraint)
{
	return 0;
}
static inline
int unregister_exynos_dm_constraint_table(enum exynos_dm_type dm_type,
				struct exynos_dm_constraint *constraint)
{
	return 0;
}
static inline
int register_exynos_dm_freq_scaler(enum exynos_dm_type dm_type,
			int (*scaler_func)(enum exynos_dm_type dm_type, u32 target_freq, unsigned int relation))
{
	return 0;
}
static inline
int unregister_exynos_dm_freq_scaler(enum exynos_dm_type dm_type)
{
	return 0;
}
static inline
int policy_update_call_to_DM(enum exynos_dm_type dm_type, u32 min_freq, u32 max_freq)
{
	return 0;
}
static inline
int DM_CALL(enum exynos_dm_type dm_type, unsigned long *target_freq)
{
	return 0;
}
#endif

#endif /* __EXYNOS_DM_H */
