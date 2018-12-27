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
#include "timenval.h"
#include <linux/ktime.h>
#include <linux/wait.h>

#define CONFIG_SUPPORT_COPR_AVG

enum {
	COPR_SET_SEQ,
	COPR_GET_SEQ,
	/* if necessary, add new seq */
	COPR_CLR_CNT_ON_SEQ,
	COPR_CLR_CNT_OFF_SEQ,
	COPR_SPI_GET_SEQ,
	COPR_DSI_GET_SEQ,
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
	u8 cnt_re:1;
	u8 copr_ilc:1;
	u8 copr_en:1;
	u8 copr_gamma:1;		/* 0:GAMMA_1, 1:GAMMA_2.2 */
	u16 copr_er:10;
	u16 copr_eg:10;
	u16 copr_eb:10;
	u16 copr_erc:10;
	u16 copr_egc:10;
	u16 copr_ebc:10;
	u16 max_cnt:16;
	u8 roi_on:1;
	u16 roi_xs:12;
	u16 roi_ys:12;
	u16 roi_xe:12;
	u16 roi_ye:12;
};

struct copr_ergb {
	u16 copr_er;
	u16 copr_eg;
	u16 copr_eb;
};

struct copr_roi {
	u32 roi_xs;
	u32 roi_ys;
	u32 roi_xe;
	u32 roi_ye;
};

struct copr_properties {
	bool support;
	u32 version;
	u32 enable;
	u32 state;
	struct copr_reg reg;

	u32 copr_ready;
	u32 cur_cnt;
	u32 cur_copr;
	u32 avg_copr;
	u32 s_cur_cnt;
	u32 s_avg_copr;
	u32 comp_copr;

	struct copr_roi roi[32];
	int nr_roi;
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
	atomic_t stop;
	struct copr_wq wq;
	struct timenval res;
	struct copr_properties props;
	struct notifier_block fb_notif;
	struct maptbl *maptbl;
	u32 nr_maptbl;
	struct seqinfo *seqtbl;
	u32 nr_seqtbl;
};

struct panel_copr_data {
	struct copr_reg reg;
	u32 version;
	struct seqinfo *seqtbl;
	u32 nr_seqtbl;
	struct maptbl *maptbl;
	u32 nr_maptbl;
};

static inline void SET_COPR_REG_GAMMA(struct copr_info *copr, bool copr_gamma)
{
	copr->props.reg.copr_gamma = copr_gamma;
}

static inline void SET_COPR_REG_E(struct copr_info *copr, int r, int g, int b)
{
	copr->props.reg.copr_er = r;
	copr->props.reg.copr_eg = g;
	copr->props.reg.copr_eb = b;
}

static inline void SET_COPR_REG_EC(struct copr_info *copr, int r, int g, int b)
{
	copr->props.reg.copr_erc = r;
	copr->props.reg.copr_egc = g;
	copr->props.reg.copr_ebc = b;
}

static inline void SET_COPR_REG_CNT_RE(struct copr_info *copr, int cnt_re)
{
	copr->props.reg.cnt_re = cnt_re;
}

static inline void SET_COPR_REG_ROI(struct copr_info *copr, struct copr_roi *roi)
{
	struct copr_properties *props = &copr->props;

	if (roi == NULL) {
		props->reg.roi_xs = 0;
		props->reg.roi_ys = 0;
		props->reg.roi_xe = 0;
		props->reg.roi_ye = 0;
		props->reg.roi_on = false;
	} else {
		props->reg.roi_xs = roi->roi_xs;
		props->reg.roi_ys = roi->roi_ys;
		props->reg.roi_xe = roi->roi_xe;
		props->reg.roi_ye = roi->roi_ye;
		props->reg.roi_on = true;
	}
}

static inline void SET_COPR_REG_RE(struct copr_info *copr, bool cnt_re)
{
	struct copr_properties *props = &copr->props;

	props->reg.cnt_re = !!cnt_re;
}

#ifdef CONFIG_EXYNOS_DECON_LCD_COPR
bool copr_is_enabled(struct copr_info *);
int copr_enable(struct copr_info *);
int copr_disable(struct copr_info *);
int copr_update_start(struct copr_info *, int);
int copr_update_average(struct copr_info *copr);
int copr_get_value(struct copr_info *);
int copr_get_average(struct copr_info *, int *, int *);
int copr_get_average_and_clear(struct copr_info *copr);
int copr_roi_get_value(struct copr_info *copr, struct copr_roi *roi, int size, u32 *out);
int copr_probe(struct panel_device *, struct panel_copr_data *);
int copr_res_update(struct copr_info *copr, int index, int cur_value, struct timespec cur_ts);
#else
static inline bool copr_is_enabled(struct copr_info *copr) { return 0; }
static inline int copr_enable(struct copr_info *copr) { return 0; }
static inline int copr_disable(struct copr_info *copr) { return 0; }
static inline int copr_update_start(struct copr_info *copr, int count) { return 0; }
static inline int copr_update_average(struct copr_info *copr) { return 0; }
static inline int copr_get_value(struct copr_info *copr) { return 0; }
static inline int copr_get_average_and_clear(struct copr_info *copr) { return 0; }
static inline int copr_roi_get_value(struct copr_info *copr, struct copr_roi *roi, int size, u32 *out) { return 0; }
static inline int copr_probe(struct panel_device *panel, struct panel_copr_data *copr_data) { return 0; }
static inline int copr_res_update(struct copr_info *copr, int index, int cur_value, struct timespec cur_ts) { return 0; }
#endif
#endif /* __COPR_H__ */
