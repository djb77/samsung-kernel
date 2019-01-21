/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *	      http://www.samsung.com/
 *
 * DDI operation : self clock, self mask, self icon.. etc.
 * Author: QC LCD driver <kr0124.cho@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __SS_DDI_OP_COMMON_H__
#define __SS_DDI_OP_COMMON_H__

#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/self_display/self_display.h>

/* payload should be 16-bytes aligned */
#define CMD_ALIGN 16

#define MAX_PAYLOAD_SIZE 250

struct self_dispaly_func {
	/* SELF DISPLAY OPERATION */
	void (*samsung_self_mask_on)(struct mdss_dsi_ctrl_pdata *ctrl, int enable);

	void (*samsung_self_icon_on)(struct mdss_dsi_ctrl_pdata *ctrl, int enable);
	int (*samsung_self_icon_grid_set)(void __user *p);

	void (*samsung_self_aclock_on)(struct mdss_dsi_ctrl_pdata *ctrl, int enable);
	int (*samsung_self_aclock_set)(void __user *p);

	void (*samsung_self_dclock_on)(struct mdss_dsi_ctrl_pdata *ctrl, int enable);
	int (*samsung_self_dclock_set)(void __user *p);
};

struct self_display_op {
	int on;
	u32 wpos;
	u32 wsize;

	char *img_buf;
	u32 img_size;

	int img_checksum;
};

struct self_display_debug {
	u32 SI_X_O;
	u32 SI_Y_O;
	u32 MEM_SUM_O;
	u32 SM_SUM_O;
	u32 MEM_WR_DONE_O;
	u32 mem_wr_icon_pk_err_o;
	u32 mem_wr_var_pk_err_o;
	u32 sv_dec_err_fg_o;
	u32 scd_dec_err_fg_o;
	u32 si_dec_err_fg_o;
	u32 CUR_HH_O;
	u32 CUR_MM_O;
	u32 CUR_MSS_O;
	u32 CUR_SS_O;
	u32 SM_WR_DONE_O;
	u32 SM_PK_ERR_O;
	u32 SM_DEC_ERR_O;
};

struct self_display {
	struct miscdevice dev;

	int is_support;
	int on;
	int file_open;

	struct mutex vdd_self_move_lock;
	struct mutex vdd_self_mask_lock;
	struct mutex vdd_self_aclock_lock;
	struct mutex vdd_self_dclock_lock;
	struct mutex vdd_self_icon_grid_lock;

	struct self_dispaly_func func;

	struct self_display_op operation[FLAG_SELF_DISP_MAX];

	struct self_display_debug debug;
//	struct samsung_display_dtsi_data *dtsi_data;
};

enum mipi_samsung_tx_cmd_list;
struct samsung_display_dtsi_data;

void ss_self_move_on(struct mdss_dsi_ctrl_pdata *ctrl, int enable);
int ss_self_move_set(const void __user *argp);

void ss_self_mask_on(struct mdss_dsi_ctrl_pdata *ctrl, int enable);

void ss_self_icon_on(struct mdss_dsi_ctrl_pdata *ctrl, int enable);
void ss_self_icon_grid_img_write(struct mdss_dsi_ctrl_pdata *ctrl);
int ss_self_icon_grid_set(const void __user *argp);

void ss_self_aclock_on(struct mdss_dsi_ctrl_pdata *ctrl, int enable);
void ss_self_aclock_img_write(struct mdss_dsi_ctrl_pdata *ctrl);
int ss_self_aclock_set(const void __user *argp);

void ss_self_dclock_on(struct mdss_dsi_ctrl_pdata *ctrl, int enable);
void ss_self_dclock_img_write(struct mdss_dsi_ctrl_pdata *ctrl);
int ss_self_dclock_set(const void __user *argp);

int ss_self_display_aod_enter(struct mdss_dsi_ctrl_pdata *ctrl);
int ss_self_display_aod_exit(struct mdss_dsi_ctrl_pdata *ctrl);

int ss_self_disp_debug(struct mdss_dsi_ctrl_pdata *ctrl);

void make_self_dispaly_img_cmds(enum mipi_samsung_tx_cmd_list cmd, u32 flag);
									  // u32 data_size,
									  // char *data);
void ss_self_display_parse_dt_cmds(struct device_node *np, struct mdss_dsi_ctrl_pdata *ctrl,
							int panel_rev, struct samsung_display_dtsi_data *dtsi_data);
int ss_self_display_init(void);

#endif // __SS_DDI_OP_COMMON_H__
