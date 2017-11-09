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

#ifndef SCORE_SYSFS_H_
#define SCORE_SYSFS_H_

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_gpio.h>
#endif

#include "score-config.h"

#ifdef ENABLE_SYSFS_SYSTEM
struct score_sysfs_system {
	unsigned int en_dvfs;
	unsigned int en_clk_gate;
};

ssize_t show_sysfs_system_dvfs_en(struct device *dev,
		struct device_attribute *attr,
		char *buf);
ssize_t store_sysfs_system_dvfs_en(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count);

ssize_t show_sysfs_system_clk_gate_en(struct device *dev,
		struct device_attribute *attr,
		char *buf);
ssize_t store_sysfs_system_clk_gate_en(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count);

extern struct score_sysfs_system sysfs_system;

ssize_t show_sysfs_system_clk(struct device *dev,
		struct device_attribute *attr,
		char *buf);
ssize_t store_sysfs_system_clk(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count);

ssize_t show_sysfs_system_sfr(struct device *dev,
		struct device_attribute *attr,
		char *buf);
ssize_t store_sysfs_system_sfr(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count);
#endif
#ifdef ENABLE_SYSFS_STATE
struct score_sysfs_state {
	bool		is_en;
	unsigned int	frame_duration;
	unsigned int	long_time;
	unsigned int	short_time;
	unsigned int	long_v_rank;
	unsigned int	short_v_rank;
	unsigned int	long_r_rank;
	unsigned int	short_r_rank;
};

ssize_t show_sysfs_state_val(struct device *dev,
		struct device_attribute *attr,
		char *buf);
ssize_t store_sysfs_state_val(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count);

ssize_t show_sysfs_state_en(struct device *dev,
		struct device_attribute *attr,
		char *buf);
ssize_t store_sysfs_state_en(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count);
extern struct score_sysfs_state sysfs_state;
int mmap_sysfs_dump(struct file *flip,
		struct kobject *kobj,
		struct bin_attribute *bin_attr,
		struct vm_area_struct *vma);
int mmap_sysfs_profile(struct file *flip,
                struct kobject *kobj,
                struct bin_attribute *bin_attr,
                struct vm_area_struct *vma);
#endif

#ifdef ENABLE_SYSFS_DEBUG
enum score_sysfs_debug_sel {
	NORMAL_CNT_QBUF,
	ERROR_CNT_OPEN,
	ERROR_CNT_VCTX_QUEUE,
	ERROR_CNT_TIMEOUT,
	ERROR_CNT_FW_ENQ,
	ERROR_CNT_RET_FAIL
};

struct score_sysfs_debug {
	unsigned int	qbuf_occur : 1;
	unsigned int	open_occur : 1;
	unsigned int	vctx_queue_occur : 1;
	unsigned int	timeout_occur : 1;
	unsigned int	fw_enq_occur : 1;
	unsigned int	ret_fail_occur : 1;
	unsigned int	reserved : 26;

	unsigned int	qbuf_cnt;
	unsigned int	open_cnt;
	unsigned int	vctx_queue_cnt;
	unsigned int	timeout_cnt;
	unsigned int	fw_enq_cnt;
	unsigned int	ret_fail_cnt;
};

extern struct score_sysfs_debug score_sysfs_debug;
ssize_t show_sysfs_debug_count(struct device *dev,
		struct device_attribute *attr,
		char *buf);
ssize_t store_sysfs_debug_count(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count);
void sysfs_debug_count_add(int sel);
#else
#define sysfs_debug_count_add(sel)
#endif

#endif
