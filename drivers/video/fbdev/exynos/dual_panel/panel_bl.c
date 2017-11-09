/*
 * linux/drivers/video/fbdev/exynos/panel/panel_bl.c
 *
 * Samsung Common LCD Driver.
 *
 * Copyright (c) 2016 Samsung Electronics
 * Gwanghui Lee <gwanghui.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/backlight.h>
#include "../dual_dpu/dsim.h"
#include "panel.h"
#include "panel_bl.h"
#include "copr.h"

#ifdef CONFIG_PANEL_AID_DIMMING
#include "dimming.h"
#include "panel_dimming.h"
#endif
#include "panel_drv.h"

static int brt_cache_tbl[MAX_PANEL_BL_SUBDEV][BRT_USER(EXTEND_BRIGHTNESS) + 1];

#ifdef DEBUG_PAC
static void print_tbl(int *tbl, int sz)
{
	int i;

	for (i = 0; i < sz; i++) {
		pr_info("%d", tbl[i]);
		if (!((i + 1) % 10))
			pr_info("\n");
	}
}
#else
static void print_tbl(int *tbl, int sz) {}
#endif

static int get_max_brightness(struct brightness_table *brt_tbl, int brightness)
{
	if (unlikely(!brt_tbl)) {
		pr_err("%s, brt_tbl is null\n", __func__);
		return -EINVAL;
	}

	if (!brt_tbl->brt || !brt_tbl->sz_brt) {
		panel_err("%s brightness table not exist\n", __func__);
		return -EINVAL;
	}

	return brt_tbl->brt[brt_tbl->sz_brt - 1];
}

static bool is_valid_brightness(struct panel_bl_device *panel_bl, int brightness)
{
	int id, max_brightness;
	struct panel_bl_sub_dev *subdev;

	if (unlikely(!panel_bl)) {
		pr_err("%s, invalid parameter\n", __func__);
		return false;
	}

	id = panel_bl->props.id;
	subdev = &panel_bl->subdev[id];
	if (!subdev->brt_tbl.brt || !subdev->brt_tbl.sz_brt) {
		panel_err("%s bl-%d brightness table not exist\n", __func__, id);
		return false;
	}

	max_brightness = get_max_brightness(&subdev->brt_tbl, brightness);

	if (brightness < 0 || brightness > max_brightness) {
		pr_err("%s, out of range %d, (min:0, max:%d)\n",
				__func__, brightness, max_brightness);
		return false;
	}

	return true;
}

/*
 * search_tbl - binary search an array of integer elements
 * @tbl : pointer to first element to search
 * @sz : number of elements
 * @type : type of searching type (i.e LOWER, UPPER, EXACT value)
 * @value : a value being searched for
 *
 * This function does a binary search on the given array.
 * The contents of the array should already be in ascending sorted order
 *
 * Note that the value need to be inside of the range of array ['start', 'end']
 * if the value is out of array range, return -1.
 * if not, this function returns array index.
 */
static int search_tbl(int *tbl, int sz, enum SEARCH_TYPE type, int value)
{
	int l = 0, m, r = sz - 1;

	if (unlikely(!tbl || sz == 0)) {
		pr_err("%s, invalid paramter(tbl %p, sz %d)\n",
				__func__, tbl, sz);
		return -EINVAL;
	}

	if ((tbl[l] > value) || (tbl[r] < value)) {
		pr_err("%s, out of range([%d, %d]) value (%d)\n",
				__func__, tbl[l], tbl[r], value);
		return -EINVAL;
	}

	while (l <= r) {
		m = l + (r - l) / 2;
		if (tbl[m] == value)
			return m;
		if (tbl[m] < value)
			l = m + 1;
		else
			r = m - 1;
	}

	if (type == SEARCH_TYPE_LOWER)
		return ((r < 0) ? -1 : r);
	else if (type == SEARCH_TYPE_UPPER)
		return ((l > sz - 1) ? -1 : l);
	else if (type == SEARCH_TYPE_EXACT)
		return -1;

	return -1;
}

#ifdef CONFIG_PANEL_AID_DIMMING
static int search_brt_tbl(struct brightness_table *brt_tbl, int brightness)
{
	if (unlikely(!brt_tbl || !brt_tbl->brt)) {
		pr_err("%s, invalid parameter\n", __func__);
		return -EINVAL;
	}

	return search_tbl(brt_tbl->brt, brt_tbl->sz_brt,
			SEARCH_TYPE_UPPER, brightness);
}

static int search_brt_to_step_tbl(struct brightness_table *brt_tbl, int brightness)
{
	if (unlikely(!brt_tbl || !brt_tbl->brt_to_step)) {
		pr_err("%s, invalid parameter\n", __func__);
		return -EINVAL;
	}

	return search_tbl(brt_tbl->brt_to_step, brt_tbl->sz_brt_to_step,
			SEARCH_TYPE_UPPER, brightness);
}

int get_subdev_actual_brightness_index(struct panel_bl_device *panel_bl,
		int id, int brightness)
{
	int index, brt_user;
	struct panel_bl_sub_dev *subdev;

	if (unlikely(!panel_bl)) {
		pr_err("%s, invalid parameter\n", __func__);
		return -EINVAL;
	}

	if (id < 0 || id >= MAX_PANEL_BL_SUBDEV) {
		panel_err("%s bl-%d exceeded max subdev\n", __func__, id);
		return -EINVAL;
	}

	if (!is_valid_brightness(panel_bl, brightness)) {
		pr_err("%s bl-%d invalid brightness\n", __func__, id);
		return -EINVAL;
	}

	subdev = &panel_bl->subdev[id];

	brt_user = BRT_TBL_INDEX(brightness);
	if (brt_user >= ARRAY_SIZE(brt_cache_tbl[id])) {
		pr_err("%s, bl-%d exceeded brt_cache_tbl size %d\n",
				__func__, id, brt_user);
		return -EINVAL;
	}

	if (brt_cache_tbl[id][brt_user] == -1) {
		index = search_brt_tbl(&subdev->brt_tbl, brightness);
		if (index < 0) {
			pr_err("%s, bl-%d failed to search %d, ret %d\n",
					__func__, id, brightness, index);
			return index;
		}
		brt_cache_tbl[id][brt_user] = index;
#ifdef DEBUG_PAC
		pr_info("%s, bl-%d brightness %d, brt_cache_tbl[%d] = %d\n",
				__func__, id, brightness, brt_user,
				brt_cache_tbl[id][brt_user]);
#endif
	} else {
		index = brt_cache_tbl[id][brt_user];
#ifdef DEBUG_PAC
		pr_info("%s, bl-%d brightness %d, brt_cache_tbl[%d] = %d\n",
				__func__, id, brightness, brt_user,
				brt_cache_tbl[id][brt_user]);
#endif
	}

	if (index > subdev->brt_tbl.sz_brt - 1)
		index = subdev->brt_tbl.sz_brt - 1;

	return index;
}

int get_actual_brightness_index(struct panel_bl_device *panel_bl, int brightness)
{
	if (unlikely(!panel_bl)) {
		pr_err("%s, invalid parameter\n", __func__);
		return -EINVAL;
	}

	if (!is_valid_brightness(panel_bl, brightness)) {
		pr_err("%s bl-%d invalid brightness\n",
				__func__, panel_bl->props.id);
		return -EINVAL;
	}

	return get_subdev_actual_brightness_index(panel_bl,
			panel_bl->props.id, brightness);
}

int get_brightness_pac_step(struct panel_bl_device *panel_bl, int brightness)
{
	int id, index;
	struct panel_bl_sub_dev *subdev;

	if (unlikely(!panel_bl)) {
		pr_err("%s, invalid parameter\n", __func__);
		return -EINVAL;
	}

	id = panel_bl->props.id;
	subdev = &panel_bl->subdev[id];

	if (!is_valid_brightness(panel_bl, brightness)) {
		pr_err("%s bl-%d invalid brightness\n", __func__, id);
		return -EINVAL;
	}

	index = search_brt_to_step_tbl(&subdev->brt_tbl, brightness);
	if (index < 0) {
		pr_err("%s, failed to search %d, ret %d\n",
				__func__, brightness, index);
		print_tbl(subdev->brt_tbl.brt_to_step,
				subdev->brt_tbl.sz_brt_to_step);
		return index;
	}

#ifdef DEBUG_PAC
	pr_info("%s, bl-%d brightness %d, pac step %d, brt %d\n",
			__func__, id, brightness, index,
			subdev->brt_tbl.brt_to_step[index]);
#endif

	if (index > subdev->brt_tbl.sz_brt_to_step - 1)
		index = subdev->brt_tbl.sz_brt_to_step - 1;

	return index;
}

int get_subdev_actual_brightness(struct panel_bl_device *panel_bl, int id, int brightness)
{
	int index;

	if (unlikely(!panel_bl)) {
		pr_err("%s, invalid parameter\n", __func__);
		return -EINVAL;
	}

	if (id < 0 || id >= MAX_PANEL_BL_SUBDEV) {
		panel_err("%s bl-%d exceeded max subdev\n", __func__, id);
		return -EINVAL;
	}

	index = get_subdev_actual_brightness_index(panel_bl, id, brightness);
	if (index < 0) {
		pr_err("%s, bl-%d failed to get actual_brightness_index %d\n",
				__func__, id, index);
		return index;
	}
	return panel_bl->subdev[id].brt_tbl.lum[index];
}

int get_subdev_actual_brightness_interpolation(struct panel_bl_device *panel_bl, int id, int brightness)
{
	int upper_idx, lower_idx;
	int upper_lum, lower_lum;
	int upper_brt, lower_brt;
	int step, upper_step, lower_step;

	if (unlikely(!panel_bl)) {
		pr_err("%s, invalid parameter\n", __func__);
		return -EINVAL;
	}

	if (id < 0 || id >= MAX_PANEL_BL_SUBDEV) {
		panel_err("%s bl-%d exceeded max subdev\n", __func__, id);
		return -EINVAL;
	}

	upper_idx = get_subdev_actual_brightness_index(panel_bl, id, brightness);
	if (upper_idx < 0) {
		pr_err("%s, bl-%d failed to get actual_brightness_index %d\n",
				__func__, id, upper_idx);
		return upper_idx;
	}
	lower_idx = max(0, (upper_idx - 1));

	lower_lum = panel_bl->subdev[id].brt_tbl.lum[lower_idx];
	upper_lum = panel_bl->subdev[id].brt_tbl.lum[upper_idx];

	lower_brt = panel_bl->subdev[id].brt_tbl.brt[lower_idx];
	upper_brt = panel_bl->subdev[id].brt_tbl.brt[upper_idx];

	lower_step = get_brightness_pac_step(panel_bl, lower_brt);
	step = get_brightness_pac_step(panel_bl, brightness);
	upper_step = get_brightness_pac_step(panel_bl, upper_brt);

	return (lower_lum * 100) + (s32)((upper_step == lower_step) ? 0 :
			((s64)(upper_lum - lower_lum) * (step - lower_step) * 100) /
			(s64)(upper_step - lower_step));
}

int get_actual_brightness(struct panel_bl_device *panel_bl, int brightness)
{
	if (unlikely(!panel_bl)) {
		pr_err("%s, invalid parameter\n", __func__);
		return -EINVAL;
	}

	return get_subdev_actual_brightness(panel_bl,
			panel_bl->props.id, brightness);
}

int get_actual_brightness_interpolation(struct panel_bl_device *panel_bl, int brightness)
{
	if (unlikely(!panel_bl)) {
		pr_err("%s, invalid parameter\n", __func__);
		return -EINVAL;
	}

	return get_subdev_actual_brightness_interpolation(panel_bl,
			panel_bl->props.id, brightness);
}
#endif /* CONFIG_PANEL_AID_DIMMING */

static void panel_bl_update_acl_state(struct panel_bl_device *panel_bl)
{
	struct panel_device *panel;
	struct panel_info *panel_data;

	panel = to_panel_device(panel_bl);
	panel_data = &panel->panel_data;

#ifdef CONFIG_SUPPORT_HMD
	if (panel_bl->props.id == PANEL_BL_SUBDEV_TYPE_HMD) {
		panel_bl->props.acl_opr = ACL_OPR_OFF;
		panel_bl->props.acl_pwrsave = ACL_PWRSAVE_OFF;
		return;
	}
#endif
	if (IS_HBM_BRIGHTNESS(panel_bl->props.brightness) ||
		IS_EXT_HBM_BRIGHTNESS(panel_bl->props.brightness)) {
		panel_bl->props.acl_opr = ACL_OPR_08P;
	}
	else {
		panel_bl->props.acl_opr = ACL_OPR_15P;
	}

	panel_bl->props.acl_pwrsave = ACL_PWRSAVE_ON;
	if(panel_bl->props.brightness == UI_MAX_BRIGHTNESS) {
		if(panel_data->props.adaptive_control == ACL_OPR_OFF) {
			panel_bl->props.acl_pwrsave = ACL_PWRSAVE_OFF;
		}
	}
}

int panel_bl_get_acl_pwrsave(struct panel_bl_device *panel_bl)
{
	return panel_bl->props.acl_pwrsave;
}

int panel_bl_get_acl_opr(struct panel_bl_device *panel_bl)
{
	return panel_bl->props.acl_opr;
}

void g_tracing_mark_write(char id, char *str1, int value);
int panel_bl_set_brightness(struct panel_bl_device *panel_bl, int id, int force)
{
	int ret = 0, ilum = 0, luminance = 0, brightness, index = PANEL_SET_BL_SEQ, step;
	struct panel_bl_sub_dev *subdev;
	struct panel_device *panel;
	int luminance_interp;

	if (panel_bl == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}

	if (id < 0 || id >= MAX_PANEL_BL_SUBDEV) {
		panel_err("%s bl-%d exceeded max subdev\n", __func__, id);
		return -EINVAL;
	}

	panel = to_panel_device(panel_bl);
	panel_bl->props.id = id;
	subdev = &panel_bl->subdev[id];
	brightness = subdev->brightness;

	if (!subdev->brt_tbl.brt || subdev->brt_tbl.sz_brt == 0) {
		panel_err("%s bl-%d brightness table not exist\n", __func__, id);
		return -EINVAL;
	}

	if (!is_valid_brightness(panel_bl, brightness)) {
		pr_err("%s bl-%d invalid brightness\n", __func__, id);
		return -EINVAL;
	}

	ilum = get_actual_brightness_index(panel_bl, brightness);
	luminance = get_actual_brightness(panel_bl, brightness);
	step = get_brightness_pac_step(panel_bl, brightness);
	if (step < 0) {
		pr_err("%s bl-%d invalid pac stap %d\n", __func__, id, step);
		return -EINVAL;
	}
	luminance_interp =
		get_actual_brightness_interpolation(panel_bl, brightness);

	panel_bl->props.brightness = brightness;
	panel_bl->props.actual_brightness = luminance;
	panel_bl->props.actual_brightness_index = ilum;
	panel_bl->props.actual_brightness_intrp = luminance_interp;
	panel_bl_update_acl_state(panel_bl);

	pr_info("%s bl-%d plat_br:%d br[%d]:%d nit:%d(%u.%02u) acl:%s(%d)\n",
			__func__, id, brightness, step, subdev->brt_tbl.brt_to_step[step],
			luminance, luminance_interp / 100, luminance_interp % 100,
			panel_bl->props.acl_pwrsave ? "on" : "off",
			panel_bl->props.acl_opr);

	if (unlikely(!force || !luminance))
		goto set_br_exit;

	g_tracing_mark_write('C', "lcd_br", luminance);
#ifdef CONFIG_SUPPORT_HMD
	if (id == PANEL_BL_SUBDEV_TYPE_HMD)
		index = PANEL_HMD_BL_SEQ;
#endif
	ret = panel_do_seqtbl_by_index(panel, index);
	if (unlikely(ret < 0)) {
		pr_err("%s, failed to write seqtbl\n", __func__);
		goto set_br_exit;
	}
#ifdef CONFIG_EXYNOS_DECON_LCD_COPR
	copr_update_start(&panel->copr, 3);
#endif

set_br_exit:
	return ret;
}

static int panel_get_brightness(struct backlight_device *bd)
{
	struct panel_bl_device *panel_bl = bl_get_data(bd);

	return get_actual_brightness(panel_bl, bd->props.brightness);
}

static int panel_set_brightness(struct backlight_device *bd)
{
	int ret = 0;
	int brightness = bd->props.brightness;
	struct panel_bl_device *panel_bl = bl_get_data(bd);

	return ret;

	mutex_lock(&panel_bl->lock);
	if (brightness < UI_MIN_BRIGHTNESS || brightness > EXTEND_BRIGHTNESS) {
		pr_alert("Brightness %d is out of range\n", brightness);
		ret = -EINVAL;
		goto exit_set;
	}

	panel_bl->subdev[PANEL_BL_SUBDEV_TYPE_DISP].brightness = brightness;

	if (panel_bl->props.id != PANEL_BL_SUBDEV_TYPE_DISP) {
		pr_info("%s bl-%d plat_br:%d - temporary\n",
				__func__, PANEL_BL_SUBDEV_TYPE_DISP, brightness);
		ret = -EINVAL;
		goto exit_set;
	}

	ret = panel_bl_set_brightness(panel_bl, PANEL_BL_SUBDEV_TYPE_DISP, 1);
	if (ret) {
		pr_err("%s : fail to set brightness\n", __func__);
		goto exit_set;
	}

exit_set:
	mutex_unlock(&panel_bl->lock);
	return ret;
}

static const struct backlight_ops panel_backlight_ops = {
	.get_brightness = panel_get_brightness,
	.update_status = panel_set_brightness,
};

int panel_bl_probe(struct panel_device *panel)
{
	int ret = 0;
	struct panel_bl_device *panel_bl = &panel->panel_bl;

	if (panel->id == 0) {
		panel_bl->bd = backlight_device_register("panel", panel->dev, panel_bl,
					&panel_backlight_ops, NULL);
	} else if (panel->id == 1) {
		panel_bl->bd = backlight_device_register("panel_1", panel->dev, panel_bl,
					&panel_backlight_ops, NULL);
	} else {
		panel_err("PANEL:ERR:%s:invalid panel id : %d\n", __func__, panel->id);
		return -EINVAL;
	}
	if (IS_ERR(panel_bl->bd)) {
		pr_err("%s:failed register backlight\n", __func__);
		ret = PTR_ERR(panel_bl->bd);
	}
	panel_bl->bd->props.max_brightness = EXTEND_BRIGHTNESS;
	panel_bl->bd->props.brightness = UI_DEF_BRIGHTNESS;
	panel_bl->props.id = PANEL_BL_SUBDEV_TYPE_DISP;
	panel_bl->props.acl_pwrsave = ACL_PWRSAVE_OFF;
	panel_bl->props.acl_opr = ACL_OPR_OFF;
	memset(brt_cache_tbl, 0xFF, sizeof(brt_cache_tbl));

	pr_info("%s done\n", __func__);

	return ret;
}
