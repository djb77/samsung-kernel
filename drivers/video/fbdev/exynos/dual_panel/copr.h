/*
 * linux/drivers/video/fbdev/exynos/panel/copr.h
 *
 * Header file for Samsung Common LCD Driver.
 *
 * Copyright (c) 2016 Samsung Electronics
 * Gwanghui Lee <gwanghui.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __COPR_H__
#define __COPR_H__

#include "panel.h"
#include <linux/ktime.h>
#include <linux/wait.h>

enum {
	COPR_SET_SEQ,
	COPR_GET_SEQ,
	/* if necessary, add new seq */
	MAX_COPR_SEQ,
};

enum {
	COPR_MAPTBL,
	/* if necessary, add new maptbl */
	MAX_COPR_MAPTBL,
};

enum COPR_STATE {
	COPR_UNINITIALIZED,
	COPR_REG_ON,
	COPR_REG_OFF,
	MAX_COPR_REG_STATE,
};

struct copr_reg {
	u8 copr_en:1;
	u8 copr_gamma:1;		/* 0:GAMMA_1, 1:GAMMA_2.2 */
	u8 copr_er;
	u8 copr_eg;
	u8 copr_eb;
	u8 roi_on:1;
	u16 roi_xs:12;
	u16 roi_ys:12;
	u16 roi_xe:12;
	u16 roi_ye:12;
}; 

struct copr_properties {
	bool support;
	u32 enable;
	struct copr_reg reg;
	u32 state;
	struct timespec last_ts;

	int last_copr;
	int last_brt;

	u64 copr_sum;
	u64 brt_sum;
	u64 elapsed_msec;
	u64 copr_avg;
	u64 brt_avg;

	u64 total_copr_sum;
	u64 total_brt_sum;
	u64 total_elapsed_msec;
	u64 total_copr_avg;
	u64 total_brt_avg;
};

struct copr_wq {
	wait_queue_head_t wait;
	atomic_t count;
	struct task_struct *thread;
};

struct copr_info {
	struct device *dev;
	struct class *class;
	struct mutex lock;
	struct copr_wq wq;
	struct copr_properties props;
	struct notifier_block fb_notif;
	struct maptbl *maptbl;
	u32 nr_maptbl;
	struct seqinfo *seqtbl;
	u32 nr_seqtbl;
};

struct panel_copr_data {
	struct copr_reg reg;
	struct seqinfo *seqtbl;
	u32 nr_seqtbl;
	struct maptbl *maptbl;
	u32 nr_maptbl;
};

#ifdef CONFIG_EXYNOS_DECON_LCD_COPR
bool copr_is_enabled(struct copr_info *);
int copr_enable(struct copr_info *);
int copr_disable(struct copr_info *);
int copr_update_start(struct copr_info *, int);
int copr_update(struct copr_info *);
int copr_get_value(struct copr_info *);
int copr_get_average(struct copr_info *, int *, int *);
int copr_probe(struct panel_device *, struct panel_copr_data *);
#else
static inline bool copr_is_enabled(struct copr_info *copr) { return 0; }
static inline int copr_enable(struct copr_info *copr) { return 0; }
static inline int copr_disable(struct copr_info *copr) { return 0; }
static inline int copr_update_start(struct copr_info *copr, int count) { return 0; }
static inline int copr_update(struct copr_info *copr) { return 0; }
static inline int copr_get_value(struct copr_info *copr) { return 0; }
static inline int copr_get_average(struct copr_info *copr, int *c_avg, int *b_avg) { return 0; }
static inline int copr_probe(struct panel_device *panel, struct panel_copr_data *copr_data) { return 0; }
#endif
#endif /* __COPR_H__ */
