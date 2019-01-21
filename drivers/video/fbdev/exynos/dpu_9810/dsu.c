/*
 * linux/drivers/video/fbdev/exynos/dpu_9810/dsu.c
 *
 * Source file for DSU Driver
 *
 * Copyright (c) 2016 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "dsu.h"
#include "decon.h"


#define DSU_INFO_WIN_ID	(DECON_WIN_UPDATE_IDX + 1)


static void dump_dsu_info(struct decon_device *decon, struct decon_win_config *windata)
{
	int i;
	struct decon_win_config *dsu_info;
	struct decon_win_config *win_info;

	dsu_info = &windata[DSU_INFO_WIN_ID];

#if 0
	if (likely(dsu_info->mode == DSU_MODE_NONE))
		return;
	if (likely(dsu_info->mode == decon->dsu.mode))
		return;
#endif

	for (i = 0; i < decon->dt.max_win; i++) {
		win_info = &windata[i];
		if (win_info->state != DECON_WIN_STATE_DISABLED)
			decon_info("Win:%d, State:%d, Dst: %04d, %04d, %04d, %04d\n",
				i, win_info->state, win_info->dst.x, win_info->dst.y,
				win_info->dst.w, win_info->dst.h);
	}

	decon_info("DSU Mode:%d, Res: %04d, %04d, %04d, %04d\n",
		dsu_info->mode, dsu_info->left, dsu_info->top,
		dsu_info->right, dsu_info->bottom);


}


static int check_dsu_param(struct decon_device *decon, struct decon_win_config *windata)
{
	int ret = 0;

	return ret;
}

static int update_dsu_reg_data(struct decon_win_config *src, struct dsu_info *dst)
{
	int ret = 0;

	if ((!src) || (!dst)) {
		decon_err("DECON:ERR:%s:invalid param", __func__);
		return -EINVAL;
	}

	dst->left = src->left;
	dst->top = src->top;
	dst->right = src->right;
	dst->bottom = src->bottom;
	dst->mode = src->mode;
	dst->needupdate = 1;

	return ret;
}

static int update_dsu_decon_data(struct decon_device *decon, struct dsu_info *dst)
{
	int ret = 0;

	if ((!decon) || (!dst)) {
		decon_err("DECON:ERR:%s:invalid param", __func__);
		return -EINVAL;
	}

	memcpy(&decon->dsu, dst, sizeof(struct dsu_info));

	return ret;
}


int set_dsu_config(struct decon_device *decon, struct decon_reg_data *regs)
{
	int ret = 0;
	int dsi_cnt, i;
	struct decon_param param;
	struct dsu_info *dsu = &regs->dsu;

	dsi_cnt = (decon->dt.dsi_mode == DSI_MODE_DUAL_DSI) ? 2 : 1;

	if (!dsu) {
		decon_err("DECON:ERR:%s:dsu is null\n", __func__);
		goto exit_dsu_config;
	}

	if ((dsu->needupdate == 0) || (decon->dt.out_type != DECON_OUT_DSI))
		goto exit_dsu_config;

	if (decon->dsu.mode == dsu->mode) {
		decon_info("DECON:INFO:%s:dsu mode : %d already set\n",
			__func__, dsu->mode);
		goto exit_dsu_config;
	}

	ret = decon_reg_wait_idle_status_timeout(decon->id, IDLE_WAIT_TIMEOUT);
	if (ret)
		decon_err("DECON:ERR:%s:faied to wait decon idle status : %d\n",
			__func__, ret);

	for (i = 0; i < dsi_cnt; i++) {
		ret = v4l2_subdev_call(decon->out_sd[i], core, ioctl,
							DSIM_IOC_DSU, dsu);
		if (ret)
			decon_err("DECON:ERR:%s:faield to DSIM_IOC_DSU\n", __func__);
	}

	decon_to_init_param(decon, &param);

	decon_reg_set_dsu(decon->id, decon->dt.dsi_mode, &param);

	dpu_init_win_update(decon);

	ret = update_dsu_decon_data(decon, dsu);
	if (ret)
		decon_err("DECON:ERR:%s:failed to update decon data\n", __func__);

exit_dsu_config:
	dsu->needupdate = 0;
	return ret;
}


void init_dsu_info(struct decon_device *decon)
{
	decon->dsu.left = 0;
	decon->dsu.top = 0;
	decon->dsu.right = decon->lcd_info->xres;
	decon->dsu.bottom = decon->lcd_info->yres;
	decon->dsu.mode = DSU_MODE_1;
}

int set_dsu_win_config(struct decon_device *decon,
	struct decon_win_config *windata, struct decon_reg_data *regs)
{
	int ret = 0;
	struct dsu_info *cur_dsu;
	struct decon_win_config *new_dsu;

	new_dsu = &windata[DSU_INFO_WIN_ID];
	cur_dsu = &decon->dsu;

	if ((!new_dsu) || (!cur_dsu)) {
		decon_err("DECON:ERR:%s:invalid param\n", __func__);
		goto set_err;
	}

	if (cur_dsu->mode == new_dsu->mode)
		return 0;

	decon_info("DECON:INFO:%s:DSU Mode:(%d->%d)-(%d,%d,%d,%d)\n",
		__func__, decon->dsu.mode, new_dsu->mode, new_dsu->top,
		new_dsu->left, new_dsu->bottom, new_dsu->right);

	dump_dsu_info(decon, windata);

	ret = check_dsu_param(decon, windata);
	if (ret) {
		decon_err("DECON:ERR:%s:wrong dsu param\n", __func__);
		goto set_err;
	}

	ret = update_dsu_reg_data(new_dsu, &regs->dsu);
	if (ret) {
		decon_err("DECON:ERR:%s:failed to update dsu data\n", __func__);
		goto set_err;
	}

set_err:
	return ret;
}

