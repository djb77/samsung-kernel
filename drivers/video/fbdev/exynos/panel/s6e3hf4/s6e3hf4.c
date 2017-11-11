/*
 * linux/drivers/video/fbdev/exynos/panel/s6e3hf4/s6e3hf4.c
 *
 * S6E3HF4 Dimming Driver
 *
 * Copyright (c) 2016 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/of_gpio.h>
#include <video/mipi_display.h>
/* TODO : remove dsim dependent code */
#include "../../dpu/dsim.h"
#include "../panel.h"
#include "s6e3hf4.h"
#include "s6e3hf4_panel.h"
#ifdef CONFIG_PANEL_AID_DIMMING
#include "../dimming.h"
#include "../panel_dimming.h"
#endif
#include "../panel_drv.h"

#ifdef CONFIG_PANEL_AID_DIMMING
static int hbm_ctoi(s32 (*dst)[MAX_COLOR], u8 *hbm, int nr_tp)
{
	int i, c, pos = 0;

	for (i = nr_tp - 1; i > 0; i--) {
		for_each_color(c) {
			if (i == nr_tp - 1) {
				dst[i][c] = (hbm[pos] << 8) | hbm[pos + 1];
				pos += 2;
			} else {
				dst[i][c] = hbm[pos];
				pos += 1;
			}
		}
	}
	dst[0][RED] = (hbm[pos] >> 4) & 0xF;
	dst[0][GREEN] = hbm[pos] & 0xF;
	dst[0][BLUE] = hbm[pos + 1] & 0xF;

	return 0;
}

static void mtp_ctoi(s32 (*dst)[MAX_COLOR], char *src, int nr_tp)
{
	int i, c, pos = 0, sign;

	for (i = nr_tp - 1; i > 0; i--) {
		for_each_color(c) {
			if (i == nr_tp - 1) {
				sign = (src[pos] & 0x01) ? -1 : 1;
				dst[i][c] = sign * src[pos + 1];
				pos += 2;
			} else {
				sign = (src[pos] & 0x80) ? -1 : 1;
				dst[i][c] = sign * (src[pos] & 0x7F);
				pos += 1;
			}
		}
	}
	dst[0][RED] = (src[pos] >> 4) & 0xF;
	dst[0][GREEN] = src[pos] & 0xF;
	dst[0][BLUE] = src[pos + 1] & 0xF;
}

static int init_dimming(struct panel_info *panel_data, int id,
		s32 (*mtp)[MAX_COLOR], s32 (*hbm)[MAX_COLOR])
{
	int i, j, ret = 0;
	struct panel_dimming_info *panel_dim_info;
	char strbuf[1024], *p;
	struct dimming_info *dim_info;
	struct dimming_lut *dim_lut;
	struct maptbl *gamma_maptbl = NULL;
	u32 luminance, nr_luminance, nr_hbm_luminance, nr_extend_hbm_luminance;
	struct panel_device *panel = to_panel_device(panel_data);
	struct panel_bl_device *panel_bl;
	struct brightness_table *brt_tbl;
	struct panel_bl_sub_dev *subdev;

	if (!panel_data->dim_info[id]) {
		dim_info = (struct dimming_info *)kzalloc(sizeof(struct dimming_info), GFP_KERNEL);
		if (unlikely(!dim_info)) {
			pr_err("failed to allocate memory for dim data\n");
			return -ENOMEM;
		}
	}

	panel_bl = &panel->panel_bl;

	if (id > MAX_PANEL_BL_SUBDEV) {
		panel_err("%s invalid bl-%d\n", __func__, id);
		return -EINVAL;
	}

	subdev = &panel_bl->subdev[id];
	brt_tbl = &subdev->brt_tbl;

	panel_dim_info = panel_data->panel_dim_info[id];
	if (unlikely(!panel_dim_info)) {
		pr_err("%s, panel_dim_info not found\n", __func__);
		return -EINVAL;
	}

	pr_info("%s, %s panel_dim_info found\n", __func__, panel_dim_info->name);
	ret = init_dimming_info(dim_info, &panel_dim_info->dim_init_info);
	if (unlikely(ret)) {
		pr_err("%s, failed to init dimming info\n", __func__);
		return ret;
	}

	dim_lut = dim_info->dim_lut;
	panel_data->dim_info[id] = dim_info;

	brt_tbl->brt = panel_dim_info->brt_tbl->brt;
	brt_tbl->sz_brt = panel_dim_info->brt_tbl->sz_brt;
	brt_tbl->lum = panel_dim_info->brt_tbl->lum;
	brt_tbl->sz_lum = panel_dim_info->brt_tbl->sz_lum;
	brt_tbl->brt_to_brt= panel_dim_info->brt_tbl->brt_to_brt;
	brt_tbl->sz_brt_to_brt = panel_dim_info->brt_tbl->sz_brt_to_brt;
	brt_tbl->brt_to_step = panel_dim_info->brt_tbl->brt_to_step;
	brt_tbl->sz_brt_to_step = panel_dim_info->brt_tbl->sz_brt_to_step;
	nr_luminance = panel_dim_info->nr_luminance;
	nr_hbm_luminance = panel_dim_info->nr_hbm_luminance;
	nr_extend_hbm_luminance = panel_dim_info->nr_extend_hbm_luminance;

	if (id == PANEL_BL_SUBDEV_TYPE_HMD) {
#ifdef CONFIG_SUPPORT_HMD
		gamma_maptbl = find_panel_maptbl_by_index(panel_data, HMD_GAMMA_MAPTBL);
#endif
	} else {
		gamma_maptbl = find_panel_maptbl_by_index(panel_data, GAMMA_MAPTBL);
	}
	if (unlikely(!gamma_maptbl)) {
		pr_err("%s, gamma maptbl not found\n", __func__);
		return -EINVAL;
	}

	init_dimming_mtp(dim_info, mtp);
	if (hbm && (panel_dim_info->hbm_target_luminance >
			panel_dim_info->target_luminance))
		init_dimming_hbm_info(dim_info,
				hbm, panel_dim_info->hbm_target_luminance);
	process_dimming(dim_info);

	for (i = 0; i < brt_tbl->sz_lum; i++) {
		if (i >= nr_luminance + nr_hbm_luminance)
			luminance = brt_tbl->lum[nr_luminance + nr_hbm_luminance - 1];
		else
			luminance = brt_tbl->lum[i];
		get_dimming_gamma_compact(dim_info, luminance,
				(u8 *)&gamma_maptbl->arr[i * gamma_maptbl->ncol]);
	}

	for (i = 0; i < brt_tbl->sz_lum; i++) {
		luminance = brt_tbl->lum[i];
		p = strbuf;
		p += sprintf(p, "gamma[%3d] : ", luminance);
		for (j = 0; j < GAMMA_CMD_CNT - 1; j++)
			p += sprintf(p, "%02X ",
					gamma_maptbl->arr[i * gamma_maptbl->ncol + j]);
		pr_info("%s\n", strbuf);
	}
	return 0;
}
#endif /* CONFIG_PANEL_AID_DIMMING */

int init_common_table(struct maptbl *tbl)
{
	if (tbl == NULL) {
		panel_err("PANEL:ERR:%s:maptbl is null\n", __func__);
		return -EINVAL;
	}

	if (tbl->pdata == NULL) {
		panel_err("PANEL:ERR:%s:pdata is null\n", __func__);
		return -EINVAL;
	}

	return 0;
}

#ifdef CONFIG_PANEL_AID_DIMMING
int init_gamma_table(struct maptbl *tbl)
{
	struct resinfo *res;
	int ret, nr_tp;
	s32 (*mtp_offset)[MAX_COLOR];
	s32 (*hbm_gamma_tbl)[MAX_COLOR];
	struct panel_info *panel_data;
	struct panel_device *panel;

	if (tbl == NULL) {
		panel_err("PANEL:ERR:%s:maptbl is null\n", __func__);
		return -EINVAL;
	}

	if (tbl->pdata == NULL) {
		panel_err("PANEL:ERR:%s:pdata is null\n", __func__);
		return -EINVAL;
	}

	panel = tbl->pdata;
	panel_data = &panel->panel_data;

	if (unlikely(!panel_data || !panel_data->panel_dim_info[PANEL_BL_SUBDEV_TYPE_DISP])) {
		pr_err("%s, invalid paramter\n", __func__);
		return -ENODEV;
	}
	nr_tp = panel_data->panel_dim_info[PANEL_BL_SUBDEV_TYPE_DISP]->dim_init_info.nr_tp;

	pr_info("%s enter\n", __func__);
	res = find_panel_resource(panel_data, "mtp");
	if (unlikely(!res)) {
		pr_warn("%s, failed to find mtp\n", __func__);
		return -EIO;
	}
	mtp_offset = (s32 (*)[MAX_COLOR])kzalloc(sizeof(s32) * nr_tp * MAX_COLOR, GFP_KERNEL);
	mtp_ctoi(mtp_offset, res->data, nr_tp);

	res = find_panel_resource(panel_data, "hbm_gamma");
	if (unlikely(!res)) {
		pr_warn("%s, failed to find hbm_gamma\n", __func__);
		kfree(mtp_offset);
		return -EIO;
	}

	hbm_gamma_tbl = (s32 (*)[MAX_COLOR])kzalloc(sizeof(s32) * nr_tp * MAX_COLOR, GFP_KERNEL);
	hbm_ctoi(hbm_gamma_tbl, res->data, nr_tp);

	ret = init_dimming(panel_data, PANEL_BL_SUBDEV_TYPE_DISP,
			mtp_offset, hbm_gamma_tbl);
	if (unlikely(ret)) {
		pr_err("%s, failed to init dimming\n", __func__);
		kfree(mtp_offset);
		kfree(hbm_gamma_tbl);
		return -ENODEV;
	}

	kfree(mtp_offset);
	kfree(hbm_gamma_tbl);

	return 0;
}
#endif /* CONFIG_PANEL_AID_DIMMING */

int getidx_common_maptbl(struct maptbl *tbl)
{
	return 0;
}

int getidx_dimming_maptbl(struct maptbl *tbl)
{
	int row;
	struct panel_bl_device *panel_bl;
	struct panel_device *panel = (struct panel_device *)tbl->pdata;

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;	
	}
	panel_bl = &panel->panel_bl;
	row = panel_bl->props.actual_brightness_index;

	return tbl->ncol * row;
}

int getidx_hmd_dimming_mtptbl(struct maptbl *tbl)
{
	//struct panel_info *panel = (struct panel_info *)tbl->pdata;
	//int row = panel->hmd_br_index;
	int row = 0;
	return tbl->ncol * row;
}

int getidx_aor_inter_table(struct maptbl *tbl)
{
	struct panel_device *panel = (struct panel_device *)tbl->pdata;
	int row = panel->panel_bl.bd->props.brightness;

	if (row > UI_MAX_BRIGHTNESS)
		row = UI_MAX_BRIGHTNESS;
	return tbl->ncol * row;
}

int getidx_mps_table(struct maptbl *tbl)
{
	int row;
	struct panel_info *panel_data;
	struct panel_bl_device *panel_bl;
	struct panel_device *panel = (struct panel_device *)tbl->pdata;

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;	
	}
	panel_data = &panel->panel_data;
	panel_bl = &panel->panel_bl;
	row = (get_actual_brightness(panel_bl, panel_bl->props.brightness) <= 39) ? 0 : 1;

	return tbl->ncol * row;
}

int init_elvss_temp_table(struct maptbl *tbl)
{
	struct panel_info *panel_data;
	struct panel_device *panel;
	int i, ret;
	u8 elvss_temp_otp_value;

	if (tbl == NULL) {
		panel_err("PANEL:ERR:%s:maptbl is null\n", __func__);
		return -EINVAL;
	}

	if (tbl->pdata == NULL) {
		panel_err("PANEL:ERR:%s:pdata is null\n", __func__);
		return -EINVAL;
	}

	panel = tbl->pdata;
	panel_data = &panel->panel_data;

	ret = resource_copy_by_name(panel_data, &elvss_temp_otp_value, "elvss_temp");
	if (unlikely(ret)) {
		pr_err("%s, elvss_temp not found in panel resource\n", __func__);
		return -EINVAL;
	}
	for_each_maptbl(tbl, i)
		tbl->arr[i] += elvss_temp_otp_value;

	return 0;
}

int getidx_elvss_temp_table(struct maptbl *tbl)
{
	int row, plane;
	struct panel_info *panel_data;
	struct panel_bl_device *panel_bl;
	struct panel_device *panel = (struct panel_device *)tbl->pdata;

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;	
	}
	panel_data = &panel->panel_data;
	panel_bl = &panel->panel_bl;

	plane = (UNDER_MINUS_15(panel_data->props.temperature) ? UNDER_MINUS_FIFTEEN :
			(UNDER_0(panel_data->props.temperature) ? UNDER_ZERO : OVER_ZERO));
	row = panel_bl->props.actual_brightness_index;

	return maptbl_index(tbl, plane, row, 0);
}

int getidx_vint_table(struct maptbl *tbl)
{
	int row, nit;
	struct panel_info *panel_data;
	struct panel_bl_device *panel_bl;
	int vint_index[] = { 5, 6, 7, 8, 9, 10, 11, 12, 13 };
	struct panel_device *panel = (struct panel_device *)tbl->pdata;

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;	
	}
	panel_data = &panel->panel_data;
	panel_bl = &panel->panel_bl;

	nit = get_actual_brightness(panel_bl, panel_bl->props.brightness);
	
	if (UNDER_MINUS_15(panel_data->props.temperature))
		return sizeof_maptbl(tbl) - 1;

	for (row = 0; row < ARRAY_SIZE(vint_index); row++)
		if (nit <= vint_index[row])
			break;
	return row;
}

int getidx_acl_onoff_table(struct maptbl *tbl)
{
#if 0
	struct panel_info *panel = (struct panel_info *)tbl->pdata;
	int nit = get_actual_br_value(panel, panel->br_index);
	return ((nit < 465 && panel->adaptive_control == 0) ? 0 : 1);
#endif
	return 0;
}

int getidx_acl_opr_table(struct maptbl *tbl)
{
	int nit;
	struct panel_info *panel_data;
	struct panel_bl_device *panel_bl;
	struct panel_device *panel = (struct panel_device *)tbl->pdata;

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;	
	}
	panel_data = &panel->panel_data;
	panel_bl = &panel->panel_bl;

	nit = get_actual_brightness(panel_bl, panel_bl->props.brightness);

	return tbl->ncol * ((nit >= 465) ? ACL_OPR_08P :
			((panel_data->props.adaptive_control > ACL_OPR_15P) ?
			 ACL_OPR_15P : panel_data->props.adaptive_control));
}

int getidx_irc_table(struct maptbl *tbl)
{
	int row;
	struct panel_device *panel = (struct panel_device *)tbl->pdata;

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;	
	}

	row = panel->panel_bl.bd->props.brightness;
	return tbl->ncol * row;
}

int getidx_dsc_table(struct maptbl *tbl)
{
	struct decon_lcd *lcd_info;
	struct panel_device *panel = (struct panel_device *)tbl->pdata;

	lcd_info = &panel->lcd_info;

	return (lcd_info->dsc_enabled ? 1 : 0);
}

int getidx_resolution_table(struct maptbl *tbl)
{
	//struct panel_info *panel = (struct panel_info *)tbl->pdata;
	//struct dsim_device *dsim = (struct dsim_device *)panel->pdata;
	int row, xres = 1440;
	switch (xres) {
		case 720 :
			row = 0;
			break;
		case 1080 :
			row = 1;
			break;
		case 1440:
		default:
			row = 2;
			break;
	}
	return tbl->ncol * row;
}

int getidx_lpm_table(struct maptbl *tbl)
{
	//struct panel_info *panel = (struct panel_info *)tbl->pdata;
	//int row = panel->alpm_mode;
	int row = 0;
	return tbl->ncol * row;
}

void copy_common_maptbl(struct maptbl *tbl, u8 *dst)
{
	int idx;
	if (!tbl || !dst)
		return;

	idx = maptbl_getidx(tbl);
	if (idx < 0)
		return;

	memcpy(dst, &(tbl->arr)[idx], sizeof(u8) * tbl->sz_copy);
	pr_debug("%s, copy %d bytes to arr[%d]\n", __func__, tbl->sz_copy, idx);
}

void copy_tset_maptbl(struct maptbl *tbl, u8 *dst)
{
	struct panel_device *panel;
	struct panel_info *panel_data;
	if (!tbl || !dst)
		return;

	panel = (struct panel_device *)tbl->pdata;
	if (unlikely(!panel))
		return;

	panel_data = &panel->panel_data;

	*dst = (panel_data->props.temperature < 0) ?
		BIT(7) | abs(panel_data->props.temperature) :
		panel_data->props.temperature;
}

#ifdef CONFIG_EXYNOS_DECON_MDNIE_LITE
int init_color_blind_table(struct maptbl *tbl);
{
	struct mdnie_info *mdnie;
	int i;

	if (tbl == NULL) {
		panel_err("PANEL:ERR:%s:maptbl is null\n", __func__);
		return -EINVAL;
	}

	if (tbl->pdata == NULL) {
		panel_err("PANEL:ERR:%s:pdata is null\n", __func__);
		return -EINVAL;
	}

	mdnie = tbl->pdata;

	for (i = 0; i < mdnie->props.sz_scr; i++) {
		tbl->arr[S6E3HF4_SCR_CB_OFS + i * 2 + 0] = GET_MSB_8BIT(mdnie->props.scr[i]);
		tbl->arr[S6E3HF4_SCR_CB_OFS + i * 2 + 1] = GET_LSB_8BIT(mdnie->props.scr[i]);
	}

	return 0;
}

int getidx_mdnie_scenario_maptbl(struct maptbl *tbl)
{
	struct mdnie_info *mdnie = (struct mdnie_info *)tbl->pdata;
	return tbl->ncol * (mdnie->props.mode);
}

int getidx_mdnie_hmd_maptbl(struct maptbl *tbl)
{
	struct mdnie_info *mdnie = (struct mdnie_info *)tbl->pdata;
	return tbl->ncol * (mdnie->props.hmd);
}

int getidx_mdnie_hdr_maptbl(struct maptbl *tbl)
{
	struct mdnie_info *mdnie = (struct mdnie_info *)tbl->pdata;
	return tbl->ncol * (mdnie->props.hdr);
}

void copy_mdnie_night_maptbl(struct maptbl *tbl, u8 *dst)
{
	struct mdnie_info *mdnie = (struct mdnie_info *)tbl->pdata;
	int idx, level = mdnie->props.night_level;

	if (!tbl || !dst)
		return;

	idx = maptbl_getidx(tbl);
	if (idx < 0)
		return;

	memcpy(dst, &(tbl->arr)[idx], sizeof(u8) * tbl->sz_copy);

	/* overwrite night maptbl */
	if (level < ARRAY_SIZE(a2_da_night_mode_table))
		memcpy(dst + S6E3HF4_NIGHT_MODE_OFS,
				a2_da_night_mode_table[level],
				sizeof(u8) * S6E3HF4_NIGHT_MODE_LEN);
}

void update_current_scr_white(struct maptbl *tbl, u8 *dst)
{
	struct mdnie_info *mdnie;

	if (!tbl || !tbl->pdata) {
		pr_err("%s, invalid param\n", __func__);
		return;
	}
	mdnie = (struct mdnie_info *)tbl->pdata;
	mdnie->props.cur_white[0] = *dst;
	mdnie->props.cur_white[1] = *(dst + 2);
	mdnie->props.cur_white[2] = *(dst + 4);
}

int getidx_color_coordinate_maptbl(struct maptbl *tbl)
{
	struct mdnie_info *mdnie = (struct mdnie_info *)tbl->pdata;
	static int layer_index[MODE_MAX] = { 0, 1, 1, 0, 0, };
	int area = mdnie_get_area(mdnie);
	if ((mdnie->props.mode < 0) || (mdnie->props.mode >= MODE_MAX)) {
		pr_err("%s, out of mode range %d\n", __func__, mdnie->props.mode);
		return -EINVAL;
	}
	if ((area < 0) || (area >= MAX_COLOR_COORDINATE_AREA)) {
		pr_err("%s, failed to get tune area %d\n", __func__, area);
		return -EINVAL;
	}
	return maptbl_index(tbl, layer_index[mdnie->props.mode], area, 0);
}

void copy_color_coordinate_maptbl(struct maptbl *tbl, u8 *dst)
{
	struct mdnie_info *mdnie = (struct mdnie_info *)tbl->pdata;
	int i, idx;

	if (unlikely(!tbl || !dst))
		return;

	idx = maptbl_getidx(tbl);
	if (unlikely(idx < 0))
		return;

	for (i = 0; i < tbl->ncol; i++) {
		mdnie->props.cur_white[i] = tbl->arr[idx + i];
		*dst = tbl->arr[idx + i];
		dst += 2;
	}
}

int getidx_adjust_ldu_maptbl(struct maptbl *tbl)
{
	struct mdnie_info *mdnie = (struct mdnie_info *)tbl->pdata;
	static int layer_index[MODE_MAX] = { 0, 1, 1, 0, 0, };

	if (!IS_LDU_MODE(mdnie))
		return -EINVAL;

	if ((mdnie->props.mode < 0) || (mdnie->props.mode >= MODE_MAX)) {
		pr_err("%s, out of mode range %d\n", __func__, mdnie->props.mode);
		return -EINVAL;
	}
	if ((mdnie->props.ldu < 0) || (mdnie->props.ldu >= MAX_LDU_MODE)) {
		pr_err("%s, out of ldu mode range %d\n", __func__, mdnie->props.ldu);
		return -EINVAL;
	}
	return maptbl_index(tbl, layer_index[mdnie->props.mode], mdnie->props.ldu, 0);
}

void copy_adjust_ldu_maptbl(struct maptbl *tbl, u8 *dst)
{
	struct mdnie_info *mdnie = (struct mdnie_info *)tbl->pdata;
	int i, idx;

	if (unlikely(!tbl || !dst))
		return;

	idx = maptbl_getidx(tbl);
	if (unlikely(idx < 0))
		return;

	if (!IS_LDU_MODE(mdnie))
		return;

	for (i = 0; i < tbl->ncol; i++) {
		mdnie->props.cur_white[i] = tbl->arr[idx + i];
		*dst = tbl->arr[idx + i];
		dst += 2;
	}
}

void copy_sensor_rgb_maptbl(struct maptbl *tbl, u8 *dst)
{
	struct mdnie_info *mdnie = (struct mdnie_info *)tbl->pdata;
	int i;

	if (!tbl || !dst)
		return;

	for (i = 0; i < tbl->ncol; i++) {
		mdnie->props.cur_white[i] = mdnie->props.white[i];
		*dst = mdnie->props.white[i];
		dst += 2;
	}
}

int getidx_mdnie_0_maptbl(struct pkt_update_info *pktui)
{
	struct panel_device *panel = pktui->pdata;
	struct mdnie_info *mdnie = &panel->mdnie;
	int row = mdnie_get_maptbl_index(mdnie);

	if (row < 0) {
		pr_err("%s, invalid row %d\n", __func__, row);
		return -EINVAL;
	}
   	return row * NR_S6E3HF4_MDNIE_REG + 0;
}

int getidx_mdnie_1_maptbl(struct pkt_update_info *pktui)
{
	struct panel_device *panel = pktui->pdata;
	struct mdnie_info *mdnie = &panel->mdnie;
	int row = mdnie_get_maptbl_index(mdnie);

	if (row < 0) {
		pr_err("%s, invalid row %d\n", __func__, row);
		return -EINVAL;
	}
   	return row * NR_S6E3HF4_MDNIE_REG + 1;
}

int getidx_mdnie_2_maptbl(struct pkt_update_info *pktui)
{
	struct panel_device *panel = pktui->pdata;
	struct mdnie_info *mdnie = &panel->mdnie;
	int row = mdnie_get_maptbl_index(mdnie);

	if (row < 0) {
		pr_err("%s, invalid row %d\n", __func__, row);
		return -EINVAL;
	}
   	return row * NR_S6E3HF4_MDNIE_REG + 2;
}

int getidx_mdnie_scr_white_maptbl(struct pkt_update_info *pktui)
{
	struct panel_device *panel = pktui->pdata;
	struct mdnie_info *mdnie = &panel->mdnie;

	if (mdnie->props.scr_white_mode < 0 ||
			mdnie->props.scr_white_mode >= MAX_SCR_WHITE_MODE) {
		pr_warn("%s, out of range %d\n",
				__func__, mdnie->props.scr_white_mode);
		return -1;
	}

	if (mdnie->props.scr_white_mode == SCR_WHITE_MODE_COLOR_COORDINATE) {
		pr_debug("%s, coordinate maptbl\n", __func__);
		return MDNIE_COLOR_COORDINATE_MAPTBL;
	} else if (mdnie->props.scr_white_mode == SCR_WHITE_MODE_ADJUST_LDU) {
		pr_debug("%s, adjust ldu maptbl\n", __func__);
		return MDNIE_ADJUST_LDU_MAPTBL;
	} else if (mdnie->props.scr_white_mode == SCR_WHITE_MODE_SENSOR_RGB) {
		pr_debug("%s, sensor rgb maptbl\n", __func__);
		return MDNIE_SENSOR_RGB_MAPTBL;
	} else {
		pr_debug("%s, empty maptbl\n", __func__);
		return MDNIE_SCR_WHITE_NONE_MAPTBL;
	}
}
#endif /* CONFIG_EXYNOS_DECON_MDNIE_LITE */
