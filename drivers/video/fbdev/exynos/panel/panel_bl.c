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
#include "panel.h"
#include "panel_bl.h"
#include "copr.h"
#include "timenval.h"

#ifdef CONFIG_PANEL_AID_DIMMING
#include "dimming.h"
#include "panel_dimming.h"
#endif
#include "panel_drv.h"

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
static void print_tbl(int *tbl, int sz) {
	return;
}
#endif

static int max_brt_tbl(struct brightness_table *brt_tbl)
{
	if (unlikely(!brt_tbl || !brt_tbl->brt || !brt_tbl->sz_brt)) {
		pr_err("%s, invalid parameter\n", __func__);
		return -EINVAL;
	}

	return brt_tbl->brt[brt_tbl->sz_brt - 1];
}

static int get_subdev_max_brightness(struct panel_bl_device *panel_bl, int id)
{
	struct panel_bl_sub_dev *subdev;

	if (unlikely(!panel_bl)) {
		pr_err("%s, invalid parameter\n", __func__);
		return -EINVAL;
	}

	if (id < 0 || id >= MAX_PANEL_BL_SUBDEV) {
		panel_err("%s bl-%d exceeded max subdev\n", __func__, id);
		return -EINVAL;
	}
	subdev = &panel_bl->subdev[id];
	return max_brt_tbl(&subdev->brt_tbl);
}

int get_max_brightness(struct panel_bl_device *panel_bl)
{
	if (unlikely(!panel_bl)) {
		pr_err("%s, invalid parameter\n", __func__);
		return false;
	}

	return get_subdev_max_brightness(panel_bl, panel_bl->props.id);
}

static bool is_valid_brightness(struct panel_bl_device *panel_bl, int brightness)
{
	int max_brightness;

	if (unlikely(!panel_bl)) {
		pr_err("%s, invalid parameter\n", __func__);
		return false;
	}

	max_brightness = get_max_brightness(panel_bl);
	if (brightness < 0 || brightness > max_brightness) {
		pr_err("%s, out of range %d, (min:0, max:%d)\n",
				__func__, brightness, max_brightness);
		return false;
	}

	return true;
}

bool is_hbm_brightness(struct panel_bl_device *panel_bl, int brightness)
{
	struct panel_bl_sub_dev *subdev;
	int luminance;
	int sz_ui_lum;

	if (unlikely(!panel_bl)) {
		pr_err("%s, invalid parameter\n", __func__);
		return false;
	}

	subdev = &panel_bl->subdev[panel_bl->props.id];
	luminance = get_actual_brightness(panel_bl, brightness);

	sz_ui_lum = subdev->brt_tbl.sz_ui_lum;
	if (sz_ui_lum <= 0 || sz_ui_lum > subdev->brt_tbl.sz_lum) {
		pr_err("%s bl-%d out of range (sz_ui_lum %d)\n",
				__func__, panel_bl->props.id, sz_ui_lum);
		return false;
	}

	return (luminance > subdev->brt_tbl.lum[sz_ui_lum - 1]);
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

	if (value <= tbl[l])
		return l;

	if (value >= tbl[r])
		return r;

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
	if (brt_user >= subdev->sz_brt_cache_tbl) {
		pr_err("%s, bl-%d exceeded brt_cache_tbl size %d\n",
				__func__, id, brt_user);
		return -EINVAL;
	}

	if (subdev->brt_cache_tbl[brt_user] == -1) {
		index = search_brt_tbl(&subdev->brt_tbl, brightness);
		if (index < 0) {
			pr_err("%s, bl-%d failed to search %d, ret %d\n",
					__func__, id, brightness, index);
			return index;
		}
		subdev->brt_cache_tbl[brt_user] = index;
#ifdef DEBUG_PAC
		pr_info("%s, bl-%d brightness %d, brt_cache_tbl[%d] = %d\n",
				__func__, id, brightness, brt_user,
				subdev->brt_cache_tbl[brt_user]);
#endif
	} else {
		index = subdev->brt_cache_tbl[brt_user];
#ifdef DEBUG_PAC
		pr_info("%s, bl-%d brightness %d, brt_cache_tbl[%d] = %d\n",
				__func__, id, brightness, brt_user,
				subdev->brt_cache_tbl[brt_user]);
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
#ifdef CONFIG_SUPPORT_AOD_BL
	if (panel_bl->props.id == PANEL_BL_SUBDEV_TYPE_AOD) {
		panel_bl->props.acl_opr = ACL_OPR_OFF;
		panel_bl->props.acl_pwrsave = ACL_PWRSAVE_OFF;
		return;
	}
#endif

	if (is_hbm_brightness(panel_bl, panel_bl->props.brightness))
		panel_bl->props.acl_opr = ACL_OPR_08P;
	else
		panel_bl->props.acl_opr = ACL_OPR_15P;

	panel_bl->props.acl_pwrsave =
		(panel_data->props.adaptive_control == ACL_OPR_OFF) ?
		ACL_PWRSAVE_OFF : ACL_PWRSAVE_ON;
}

int panel_bl_get_acl_pwrsave(struct panel_bl_device *panel_bl)
{
	return panel_bl->props.acl_pwrsave;
}

int panel_bl_get_acl_opr(struct panel_bl_device *panel_bl)
{
	return panel_bl->props.acl_opr;
}

int panel_bl_set_subdev(struct panel_bl_device *panel_bl, int id)
{
	panel_bl->props.id = id;

	return 0;
}

int panel_bl_update_average(struct panel_bl_device *panel_bl, size_t index)
{
	struct timespec cur_ts;
	int cur_brt;

	if (index >= ARRAY_SIZE(panel_bl->tnv))
		return -EINVAL;

	ktime_get_ts(&cur_ts);
	cur_brt = panel_bl->props.actual_brightness_intrp / 100;
	timenval_update_snapshot(&panel_bl->tnv[index], cur_brt, cur_ts);

	return 0;
}

int panel_bl_clear_average(struct panel_bl_device *panel_bl, size_t index)
{
	if (index >= ARRAY_SIZE(panel_bl->tnv))
		return -EINVAL;

	timenval_clear_average(&panel_bl->tnv[index]);

	return 0;
}

int panel_bl_get_average_and_clear(struct panel_bl_device *panel_bl, size_t index)
{
	int avg;

	if (index >= ARRAY_SIZE(panel_bl->tnv))
		return -EINVAL;

	mutex_lock(&panel_bl->lock);
	panel_bl_update_average(panel_bl, index);
	avg = panel_bl->tnv[index].avg;
	panel_bl_clear_average(panel_bl, index);
	mutex_unlock(&panel_bl->lock);

	return avg;
}

//void g_tracing_mark_write(char id, char *str1, int value);
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

	//g_tracing_mark_write('C', "lcd_br", luminance);
#ifdef CONFIG_SUPPORT_HMD
	if (id == PANEL_BL_SUBDEV_TYPE_HMD)
		index = PANEL_HMD_BL_SEQ;
#endif
#ifdef CONFIG_SUPPORT_AOD_BL
	if (id == PANEL_BL_SUBDEV_TYPE_AOD)
		index = PANEL_ALPM_ENTER_SEQ;
#endif
	ret = panel_do_seqtbl_by_index_nolock(panel, index);
	if (unlikely(ret < 0)) {
		pr_err("%s, failed to write seqtbl\n", __func__);
		goto set_br_exit;
	}
	panel_bl_update_average(panel_bl, 0);
	panel_bl_update_average(panel_bl, 1);

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
	int id, brightness = bd->props.brightness;
	struct panel_bl_device *panel_bl = bl_get_data(bd);
	struct panel_device *panel = to_panel_device(panel_bl);

	mutex_lock(&panel_bl->lock);
	mutex_lock(&panel->op_lock);
	id = panel_bl->props.id;
	if (!is_valid_brightness(panel_bl, brightness)) {
		pr_alert("Brightness %d is out of range\n", brightness);
		ret = -EINVAL;
		goto exit_set;
	}

	panel_bl->subdev[PANEL_BL_SUBDEV_TYPE_DISP].brightness = brightness;
#ifdef CONFIG_SUPPORT_AOD_BL
	panel_bl->subdev[PANEL_BL_SUBDEV_TYPE_AOD].brightness = brightness;
#endif

	if (id == PANEL_BL_SUBDEV_TYPE_HMD) {
		pr_info("%s keep plat_br:%d\n", __func__, brightness);
		ret = -EINVAL;
		goto exit_set;
	}

	ret = panel_bl_set_brightness(panel_bl, id, 1);
	if (ret) {
		pr_err("%s, failed to set_brightness (ret %d)\n",
				__func__, ret);
		goto exit_set;
	}

exit_set:
	mutex_unlock(&panel->op_lock);
	mutex_unlock(&panel_bl->lock);
	return ret;
}

static const struct backlight_ops panel_backlight_ops = {
	.get_brightness = panel_get_brightness,
	.update_status = panel_set_brightness,
};

int panel_bl_probe(struct panel_device *panel)
{
	int id, size, ret = 0;
	struct panel_bl_device *panel_bl = &panel->panel_bl;

	mutex_lock(&panel_bl->lock);
	if (!panel_bl->bd) {
		panel_bl->bd = backlight_device_register("panel", panel->dev, panel_bl,
				&panel_backlight_ops, NULL);
		if (IS_ERR(panel_bl->bd)) {
			pr_err("%s:failed register backlight\n", __func__);
			ret = PTR_ERR(panel_bl->bd);
			goto err;
		}
	}
	panel_bl->props.id = PANEL_BL_SUBDEV_TYPE_DISP;
	panel_bl->bd->props.brightness = UI_DEF_BRIGHTNESS;
	panel_bl->bd->props.max_brightness =
		get_subdev_max_brightness(panel_bl, panel_bl->props.id);
	panel_bl->props.acl_pwrsave = ACL_PWRSAVE_OFF;
	panel_bl->props.acl_opr = ACL_OPR_OFF;

	for (id = 0; id < MAX_PANEL_BL_SUBDEV; id++) {
		size = BRT_USER(get_subdev_max_brightness(panel_bl, id)) + 1;
		if (size <= 0) {
			pr_warn("%s invalid brightness table (size %d)\n",
					__func__, size);
			panel_bl->subdev[id].sz_brt_cache_tbl = -1;
			continue;
		}

		if (panel_bl->subdev[id].brt_cache_tbl) {
			pr_info("%s bl-%d free brt_cache\n", __func__, id);
			devm_kfree(panel->dev, panel_bl->subdev[id].brt_cache_tbl);
			panel_bl->subdev[id].sz_brt_cache_tbl = 0;
		}

		panel_bl->subdev[id].brt_cache_tbl =
			(int *)devm_kmalloc(panel->dev, size * sizeof(int), GFP_KERNEL);
		memset(panel_bl->subdev[id].brt_cache_tbl, 0xFF, size * sizeof(int));
		panel_bl->subdev[id].sz_brt_cache_tbl = size;
	}
	pr_info("%s done\n", __func__);

err:
	mutex_unlock(&panel_bl->lock);
	return ret;
}
