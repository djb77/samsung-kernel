/*
 * linux/drivers/video/fbdev/exynos/aod/aod_drv.c
 *
 * Source file for AOD Driver
 *
 * Copyright (c) 2016 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/io.h>

#include "aod_drv_ioctl.h"
#include "../panel_drv.h"
#include "aod_drv.h"

int panel_do_aod_seqtbl_by_index_nolock(struct aod_dev_info *aod, int index)
{
	int ret;
	struct seqinfo *tbl;
	struct panel_info *panel_data;
	struct panel_device *panel = to_panel_device(aod);
	struct timespec cur_ts, last_ts, delta_ts;
	s64 elapsed_usec;

	if (panel == NULL) {
		panel_err("ERR:PANEL:%s:panel is null\n", __func__);
		return -EINVAL;
	}

	if (!IS_PANEL_ACTIVE(panel))
		return 0;

	panel_data = &panel->panel_data;
	tbl = panel->aod.seqtbl;
	ktime_get_ts(&cur_ts);

	ktime_get_ts(&last_ts);

	if (unlikely(index < 0 || index >= MAX_AOD_SEQ)) {
		panel_err("%s, invalid paramter (panel %p, index %d)\n",
				__func__, panel, index);
		ret = -EINVAL;
		goto do_exit;
	}

	delta_ts = timespec_sub(last_ts, cur_ts);
	elapsed_usec = timespec_to_ns(&delta_ts) / 1000;
	if (elapsed_usec > 34000)
		pr_debug("seq:%s warn:elapsed %lld.%03lld msec to acquire lock\n",
				tbl[index].name, elapsed_usec / 1000, elapsed_usec % 1000);

	if (panel_data->props.key[CMD_LEVEL_1] != 0 ||
			panel_data->props.key[CMD_LEVEL_2] != 0 ||
			panel_data->props.key[CMD_LEVEL_3] != 0) {
		panel_warn("%s before seq:%s unbalanced key [1]%s(%d) [2]%s(%d) [3]%s(%d)\n",
				__func__, tbl[index].name,
				(panel_data->props.key[CMD_LEVEL_1] > 0 ? "ENABLED" : "DISABLED"),
				panel_data->props.key[CMD_LEVEL_1],
				(panel_data->props.key[CMD_LEVEL_2] > 0 ? "ENABLED" : "DISABLED"),
				panel_data->props.key[CMD_LEVEL_2],
				(panel_data->props.key[CMD_LEVEL_3] > 0 ? "ENABLED" : "DISABLED"),
				panel_data->props.key[CMD_LEVEL_3]);
		panel_data->props.key[CMD_LEVEL_1] = 0;
		panel_data->props.key[CMD_LEVEL_2] = 0;
		panel_data->props.key[CMD_LEVEL_3] = 0;
	}

	ret = panel_do_seqtbl(panel, &tbl[index]);
	if (unlikely(ret < 0)) {
		pr_err("%s, failed to excute seqtbl %s\n",
				__func__, tbl[index].name);
		ret = -EIO;
		goto do_exit;
	}

do_exit:

	if (panel_data->props.key[CMD_LEVEL_1] != 0 ||
			panel_data->props.key[CMD_LEVEL_2] != 0 ||
			panel_data->props.key[CMD_LEVEL_3] != 0) {
		panel_warn("%s after seq:%s unbalanced key [1]%s(%d) [2]%s(%d) [3]%s(%d)\n",
				__func__, tbl[index].name,
				(panel_data->props.key[CMD_LEVEL_1] > 0 ? "ENABLED" : "DISABLED"),
				panel_data->props.key[CMD_LEVEL_1],
				(panel_data->props.key[CMD_LEVEL_2] > 0 ? "ENABLED" : "DISABLED"),
				panel_data->props.key[CMD_LEVEL_2],
				(panel_data->props.key[CMD_LEVEL_3] > 0 ? "ENABLED" : "DISABLED"),
				panel_data->props.key[CMD_LEVEL_3]);
		panel_data->props.key[CMD_LEVEL_1] = 0;
		panel_data->props.key[CMD_LEVEL_2] = 0;
		panel_data->props.key[CMD_LEVEL_3] = 0;
	}

	ktime_get_ts(&last_ts);
	delta_ts = timespec_sub(last_ts, cur_ts);
	elapsed_usec = timespec_to_ns(&delta_ts) / 1000;
	pr_debug("seq:%s done (elapsed %2lld.%03lld msec)\n",
			tbl[index].name, elapsed_usec / 1000, elapsed_usec % 1000);

	return 0;
}

int panel_do_aod_seqtbl_by_index(struct aod_dev_info *aod, int index)
{
	int ret = 0;
	struct panel_device *panel = to_panel_device(aod);

	if (panel == NULL) {
		panel_err("ERR:PANEL:%s:panel is null\n", __func__);
		return -EINVAL;
	}

	mutex_lock(&panel->op_lock);
	ret = panel_do_aod_seqtbl_by_index_nolock(aod, index);
	mutex_unlock(&panel->op_lock);

	return ret;
}

struct seqinfo *find_aod_seqtbl(struct aod_dev_info *aod, char *name)
{
	int i;

	if (unlikely(!aod->seqtbl)) {
		panel_err("%s, seqtbl not exist\n", __func__);
		return NULL;
	}

	for (i = 0; i < aod->nr_seqtbl; i++) {
		if (unlikely(!aod->seqtbl[i].name))
			continue;

		if (!strcmp(aod->seqtbl[i].name, name)) {
			return &aod->seqtbl[i];
		}
	}
	return NULL;
}

static int aod_init_panel(struct aod_dev_info *aod_dev)
{
	int ret = 0;
	struct aod_ioctl_props *props = &aod_dev->props;

	mutex_lock(&aod_dev->lock);
	panel_info("%s was called\n", __func__);

	if (props->self_mask_en) {
		ret = panel_do_aod_seqtbl_by_index(aod_dev, SELF_MASK_IMG_SEQ);
		if (ret)
			panel_err("PANEL:ERR:%s:failed to write self mask image\n", __func__);

		ret = panel_do_aod_seqtbl_by_index(aod_dev, SELF_MASK_ENA_SEQ);
		if (ret)
			panel_err("PANEL:ERR:%s:failed to enable self mask\n", __func__);
	}
	mutex_unlock(&aod_dev->lock);

	return 0;
}

static int aod_enter_to_lpm(struct aod_dev_info *aod_dev)
{
	int ret = 0;
	struct aod_ioctl_props *props = &aod_dev->props;

	mutex_lock(&aod_dev->lock);

	panel_info("%s was called\n", __func__);

	if (props->self_mask_en) {
		ret = panel_do_aod_seqtbl_by_index(aod_dev, SELF_MASK_DIS_SEQ);
		if (ret)
			panel_err("PANEL:ERR:%s:failed to disable self mask\n", __func__);
	}

	ret = panel_do_aod_seqtbl_by_index(aod_dev, ENTER_AOD_SEQ);
	if (ret)
		panel_err("PANEL:ERR:%s:faield to write aod seq\n", __func__);

/* write image to side ram */
	if (aod_dev->reset_flag) {
		if (aod_dev->icon_img.up_flag) {
			panel_info("AOD:INFO:%s:write self icon img\n", __func__);
			ret = panel_do_aod_seqtbl_by_index(aod_dev, SELF_ICON_IMG_SEQ);
			if (ret)
				panel_err("AOD:ERR:%s:failed to seq self icon img\n", __func__);
		}
		if (aod_dev->ac_img.up_flag) {
			panel_info("AOD:INFO:%s:write analog clock img\n", __func__);
			ret = panel_do_aod_seqtbl_by_index(aod_dev, ANALOG_IMG_SEQ);
			if (ret)
				panel_err("AOD:ERR:%s:failed to seq analog clk img\n", __func__);
		}
		if (aod_dev->dc_img.up_flag) {
			panel_info("AOD:INFO:%s:write digital clock img\n", __func__);
			ret = panel_do_aod_seqtbl_by_index(aod_dev, DIGITAL_IMG_SEQ);
			if (ret)
				panel_err("AOD:ERR:%s:failed to seq digital clk img\n", __func__);
		}
		aod_dev->reset_flag = 0;
	}
#if 0
	if (props->self_move_en || props->self_icon.en) {
		ret = panel_do_aod_seqtbl_by_index(aod_dev, SELF_MOVE_ON_SEQ);
		if (ret)
			panel_err("PANEL:ERR:%s:failed to enable self move\n", __func__);
	}
#endif
	if (props->self_icon.en || props->self_grid.en) {
		ret = panel_do_aod_seqtbl_by_index(aod_dev, ICON_GRID_ON_SEQ);
		if (ret)
			panel_err("AOD:ERR:%s:failed to seq icon on\n", __func__);
	}

	if (props->analog.en && aod_dev->ac_img.up_flag) {
		ret = panel_do_aod_seqtbl_by_index(aod_dev, ANALOG_CTRL_SEQ);
		if (ret)
			panel_err("AOD:ERR:%s:failed to seq analog ctrl\n", __func__);
	}

	if (props->digital.en && aod_dev->dc_img.up_flag) {
		ret = panel_do_aod_seqtbl_by_index(aod_dev, DIGITAL_CTRL_SEQ);
		if (ret)
			panel_err("AOD:ERR:%s:failed to seq digital ctrl\n", __func__);
	}

	mutex_unlock(&aod_dev->lock);

	return ret;
}

static int aod_exit_from_lpm(struct aod_dev_info *aod_dev)
{
	int ret = 0;

	struct aod_ioctl_props *props = &aod_dev->props;

	mutex_lock(&aod_dev->lock);

	if (props->analog.en) {
		panel_err("AOD:WARN:%s:still anlaog was enabled\n", __func__);
		props->analog.en = 0;
	}

	if (props->digital.en) {
		panel_err("AOD:WARN:%s:still digital was enabled\n", __func__);
		props->digital.en = 0;
	}

	if (props->self_icon.en || props->self_grid.en) {
		ret = panel_do_aod_seqtbl_by_index(aod_dev, ICON_GRID_OFF_SEQ);
		if (ret)
			panel_err("AOD:ERR:%s:failed to seq icon on\n", __func__);
	}

	if (props->self_move_en || props->self_icon.en) {
		ret = panel_do_aod_seqtbl_by_index(aod_dev, SELF_MOVE_OFF_SEQ);
		if (ret)
			panel_err("PANEL:ERR:%s:failed to disable self mode\n", __func__);

		if (props->self_move_en)
			props->self_move_en = 0;

		if (props->self_icon.en)
			props->self_icon.en = 0;
	}

	ret = panel_do_aod_seqtbl_by_index(aod_dev, EXIT_AOD_SEQ);
	if (ret)
		panel_err("PANEL:ERR:%s:faield to write exit aod seq\n", __func__);

	if (props->self_mask_en) {
		ret = panel_do_aod_seqtbl_by_index(aod_dev, SELF_MASK_ENA_SEQ);
		if (ret)
			panel_err("PANEL:ERR:%s:failed to enable self mask\n", __func__);
	}

	props->first_clk_update = 1;

	mutex_unlock(&aod_dev->lock);

	return ret;
}

static int aod_doze_suspend(struct aod_dev_info *aod_dev)
{
	int ret = 0;

	mutex_lock(&aod_dev->lock);

	panel_info("AOD:INFO:%s : was called\n", __func__);

	mutex_unlock(&aod_dev->lock);

	return ret;
}

static int aod_power_off(struct aod_dev_info *aod_dev)
{
	int ret = 0;
	struct aod_ioctl_props *props = &aod_dev->props;

	mutex_lock(&aod_dev->lock);

	panel_info("AOD:INFO:%s : was called\n", __func__);

	if (props->analog.en) {
		panel_err("AOD:WARN:%s:still anlaog was enabled\n", __func__);
		props->analog.en = 0;
	}

	if (props->digital.en) {
		panel_err("AOD:WARN:%s:still digital was enabled\n", __func__);
		props->digital.en = 0;
	}

	if (props->self_move_en)
		props->self_move_en = 0;

	if (props->self_icon.en)
		props->self_icon.en = 0;

	/* set flag reseted */
	aod_dev->reset_flag = 1;
	props->first_clk_update = 1;
	props->self_reset_cnt = 0;

	mutex_unlock(&aod_dev->lock);
	return ret;
}

static struct aod_ops aod_drv_ops = {
	.init_panel = aod_init_panel,
	.enter_to_lpm = aod_enter_to_lpm,
	.exit_from_lpm = aod_exit_from_lpm,
	.doze_suspend = aod_doze_suspend,
	.power_off = aod_power_off,
};


static int __seq_aod_self_move_en(struct aod_dev_info *aod, unsigned long arg)
{
	int ret = 0;
	struct aod_ioctl_props *props = &aod->props;
	struct panel_device *panel = to_panel_device(aod);

	panel_info("AOD:INFO:%s:inteval : %d\n",
		__func__, props->cur_time.interval);

	if (panel == NULL) {
		panel_err("AOD:PANEL:%s:panel is null\n", __func__);
		goto move_on_exit;
	}

	if (panel->state.cur_state != PANEL_STATE_ALPM) {
		panel_info("AOD:WARN:%s:self move on ignored\n", __func__);
		goto move_on_exit;
	}
	if (props->first_clk_update) {
		ret = panel_do_aod_seqtbl_by_index(aod, SET_TIME_SEQ);
		if (ret) {
			panel_info("AOD:ERR:%s:failed to set time\n", __func__);
			goto move_on_exit;
		}
	}
	ret = panel_do_aod_seqtbl_by_index(aod, SELF_MOVE_ON_SEQ);
	if (ret) {
		panel_info("AOD:ERR:%s:failed to seq_self_move on\n", __func__);
		goto move_on_exit;
	}

move_on_exit:
	return ret;
}


static int __seq_aod_self_move_off(struct aod_dev_info *aod)
{
	int ret = 0;

	struct panel_device *panel = to_panel_device(aod);

	if (panel == NULL) {
		panel_err("AOD:PANEL:%s:panel is null\n", __func__);
		goto move_on_exit;
	}

	if (panel->state.cur_state != PANEL_STATE_ALPM) {
		panel_info("AOD:WARN:%s:self move off ignored\n", __func__);
		goto move_on_exit;
	}

	ret = panel_do_aod_seqtbl_by_index(aod, SELF_MOVE_RESET_SEQ);
	if (ret) {
		panel_info("AOD:ERR:%s:failed to seq_self_move off\n", __func__);
		goto move_on_exit;
	}

move_on_exit:
	return ret;
}

int __get_icon_img_info(struct aod_dev_info *aod)
{
	int ret = 0;
	struct seqinfo *img_seqtbl = NULL;
	struct pktinfo *img_pktinfo = NULL;
	//struct aod_ioctl_props *props = &aod->props;

	img_seqtbl = find_aod_seqtbl(aod, "self_icon_img");
	if (!img_seqtbl) {
		panel_err("AOD:ERR:%s:failed to get icon seqtbl\n", __func__);
		ret = -EINVAL;
		goto get_exit;
	}

	img_pktinfo = find_packet_suffix(img_seqtbl, "self_icon_img");
	if (!img_pktinfo) {
		panel_err("AOD:ERR:%s:failed to get icon pktinfo\n", __func__);
		ret = -EINVAL;
		goto get_exit;
	}
	aod->icon_img.buf = img_pktinfo->data;
	aod->icon_img.size = img_pktinfo->dlen;

get_exit:
	return ret;
}

int __get_ac_img_info(struct aod_dev_info *aod)
{
	int ret = 0;
	struct seqinfo *img_seqtbl = NULL;
	struct pktinfo *img_pktinfo = NULL;
	//struct aod_ioctl_props *props = &aod->props;

	img_seqtbl = find_aod_seqtbl(aod, "analog_img");
	if (!img_seqtbl) {
		panel_err("AOD:ERR:%s:failed to get analog img seqtbl\n", __func__);
		ret = -EINVAL;
		goto get_exit;
	}

	img_pktinfo = find_packet_suffix(img_seqtbl, "analog_img");
	if (!img_pktinfo) {
		panel_err("AOD:ERR:%s:failed to get analog img pktinfo\n", __func__);
		ret = -EINVAL;
		goto get_exit;
	}

	aod->ac_img.buf = img_pktinfo->data;
	aod->ac_img.size = img_pktinfo->dlen;

get_exit:
	return ret;
}

int __get_dc_img_info(struct aod_dev_info *aod)
{
	int ret = 0;
	struct seqinfo *img_seqtbl = NULL;
	struct pktinfo *img_pktinfo = NULL;
	//struct aod_ioctl_props *props = &aod->props;

	img_seqtbl = find_aod_seqtbl(aod, "digital_img");
	if (!img_seqtbl) {
		panel_err("AOD:ERR:%s:failed to get analog img seqtbl\n", __func__);
		ret = -EINVAL;
		goto get_exit;
	}

	img_pktinfo = find_packet_suffix(img_seqtbl, "digital_img");
	if (!img_pktinfo) {
		panel_err("AOD:ERR:%s:failed to get analog img pktinfo\n", __func__);
		ret = -EINVAL;
		goto get_exit;
	}

	aod->dc_img.buf = img_pktinfo->data;
	aod->dc_img.size = img_pktinfo->dlen;

get_exit:
	return ret;
}


static int aod_drv_fops_open(struct inode *inode, struct file *file)
{
	int ret = 0;
	struct miscdevice *dev = file->private_data;
	struct aod_dev_info *aod = container_of(dev, struct aod_dev_info, dev);
	struct panel_device *panel = to_panel_device(aod);

	mutex_lock(&panel->io_lock);
	mutex_lock(&aod->lock);

	panel_info("%s was called\n", __func__);

	file->private_data = aod;

	ret = __get_icon_img_info(aod);
	if (ret)
		panel_err("AOD:ERR:%s:failed to get icon img info\n", __func__);

	ret = __get_ac_img_info(aod);
	if (ret)
		panel_err("AOD:ERR:%s:failed to get ac img info\n", __func__);

	ret = __get_dc_img_info(aod);
	if (ret)
		panel_err("AOD:ERR:%s:failed to get dc img info\n", __func__);

	mutex_unlock(&aod->lock);
	mutex_unlock(&panel->io_lock);

	return ret;
}

#define AOD_DRV_SELF_MOVE_ON	1
#define AOD_DRV_SELF_MOVE_OFF	0


static int __aod_ioctl_set_grid(struct aod_dev_info *aod, unsigned long arg)
{
	int ret = 0;
	struct self_grid_info grid_info;
	struct aod_ioctl_props *props = &aod->props;

	if (copy_from_user(&grid_info, (struct self_grid_info __user *)arg,
			sizeof(struct self_grid_info))) {
		panel_err("PANEL:ERR:%s:failed to get user's icon info\n",
			__func__);
		ret = -EINVAL;
		goto exit_grid_ioctl;
	}

	panel_info("AOD:INFO:%s:%d:%d:%d:%d:%d\n", __func__,
		grid_info.en, grid_info.s_pos_x, grid_info.s_pos_y,
		grid_info.e_pos_x, grid_info.e_pos_y);

	memcpy(&props->self_grid, &grid_info, sizeof(struct self_grid_info));

exit_grid_ioctl:
	return ret;
}


static int __aod_ioctl_set_icon(struct aod_dev_info *aod, unsigned long arg)
{
	int ret = 0;
	struct self_icon_info icon_info;
	struct aod_ioctl_props *props = &aod->props;

	if (copy_from_user(&icon_info, (struct self_icon_info __user *)arg,
			sizeof(struct self_icon_info))) {
		panel_err("PANEL:ERR:%s:failed to get user's icon info\n",
			__func__);
		ret = -EINVAL;
		goto exit_icon_ioctl;
	}
	panel_info("AOD:INFO:%s:%d:%d:%d:%d:%d\n", __func__, icon_info.en,
		icon_info.height, icon_info.width, icon_info.pos_x, icon_info.pos_y);

	memcpy(&props->self_icon, &icon_info, sizeof(struct self_icon_info));

exit_icon_ioctl:
	return ret;
}

static int __aod_ioctl_set_time(struct aod_dev_info *aod, unsigned long arg)
{
	int ret = 0;
	struct aod_cur_time cur_time;
	struct aod_ioctl_props *props = &aod->props;
	struct panel_device *panel = to_panel_device(aod);

	if (copy_from_user(&cur_time, (struct aod_cur_time __user *)arg,
			sizeof(struct aod_cur_time))) {
		panel_err("PANEL:ERR:%s:failed to get user's curtime\n",
			__func__);
		ret = -EINVAL;
		goto exit_time_ioctl;
	}
	panel_info("AOD:INFO:%s:%d:%d:%d:%d\n", __func__, cur_time.cur_h,
		cur_time.cur_m, cur_time.cur_s, cur_time.cur_ms);

	memcpy(&props->cur_time, &cur_time, sizeof(struct aod_cur_time));

	if (panel->state.cur_state != PANEL_STATE_ALPM) {
		panel_info("AOD:WARN:%s:set time ignored\n", __func__);
		goto exit_time_ioctl;
	}

	ret = panel_do_aod_seqtbl_by_index(aod, SET_TIME_SEQ);
	if (ret) {
		panel_info("AOD:ERR:%s:failed to seq_self_move off\n", __func__);
		goto exit_time_ioctl;
	}

exit_time_ioctl:
	return ret;
}


static int __aod_ioctl_set_analog_clk(struct aod_dev_info *aod, unsigned long arg)
{
	int ret = 0;
	struct analog_clk_info clk;
	struct aod_ioctl_props *props = &aod->props;
	struct panel_device *panel = to_panel_device(aod);

	if (copy_from_user(&clk, (struct analog_clk_info __user *)arg,
		sizeof(struct analog_clk_info))) {
		panel_err("PANEL:ERR:%s:failed to get user's info\n", __func__);
		ret = -EINVAL;
		goto exit_analog_ioctl;
	}
	panel_info("AOD:INFO:%s:en:%d,pos:%d,%d rot:%d\n", __func__,
		clk.en, clk.pos_x, clk.pos_y, clk.rotate);

	memcpy(&props->analog, &clk, sizeof(struct analog_clk_info));

	if (panel->state.cur_state != PANEL_STATE_ALPM) {
		panel_info("AOD:WARN:%s:set time ignored\n", __func__);
		goto exit_analog_ioctl;
	}

	ret = panel_do_aod_seqtbl_by_index(aod, ANALOG_CTRL_SEQ);
	if (ret) {
		panel_info("AOD:ERR:%s:failed to seq analog clk\n", __func__);
		goto exit_analog_ioctl;
	}

exit_analog_ioctl:
	return ret;
}

static int __aod_ioctl_set_digital_clk(struct aod_dev_info *aod, unsigned long arg)
{
	int ret = 0;
	struct digital_clk_info clk;
	struct aod_ioctl_props *props = &aod->props;
	struct panel_device *panel = to_panel_device(aod);

	if (copy_from_user(&clk, (struct digital_clk_info __user *)arg,
		sizeof(struct digital_clk_info))) {
		panel_err("PANEL:ERR:%s:failed to get user's info\n", __func__);
		ret = -EINVAL;
		goto exit_digital_ioctl;
	}
	panel_info("AOD:INFO:%s:en:%d,pos:%d,%d\n", __func__,
		clk.en, clk.pos1_x, clk.pos1_y);

	panel_info("AOD:INFO:%s:width:%d, height:%d\n", __func__,
		clk.img_height, clk.img_width);

	panel_info("AOD:INFO:%s:hh_en:%d, mm_en:%d\n", __func__,
		clk.en_hh, clk.en_mm);

	memcpy(&props->digital, &clk, sizeof(struct digital_clk_info));

	if (panel->state.cur_state != PANEL_STATE_ALPM) {
		panel_info("AOD:WARN:%s:set time ignored\n", __func__);
		goto exit_digital_ioctl;
	}

	ret = panel_do_aod_seqtbl_by_index(aod, DIGITAL_CTRL_SEQ);
	if (ret) {
		panel_info("AOD:ERR:%s:failed to seq digital clk\n", __func__);
		goto exit_digital_ioctl;
	}

exit_digital_ioctl:
	return ret;
}


static long aod_drv_fops_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	struct aod_dev_info *aod = file->private_data;
	struct aod_ioctl_props *props = &aod->props;
	struct panel_device *panel = to_panel_device(aod);

	mutex_lock(&panel->io_lock);
	mutex_lock(&aod->lock);

	switch (cmd) {
	case IOCTL_SELF_MOVE_EN:
		panel_info("AOD:INFO:%s:IOCTL_SELF_MOVE_EN\n", __func__);
		props->self_move_en = AOD_DRV_SELF_MOVE_ON;
		ret = __seq_aod_self_move_en(aod, arg);
		if (ret) {
			panel_err("AOD:ERR:%s:faield to seq self move en\n", __func__);
			goto exit_ioctl;
		}
		break;
	case IOCTL_SELF_MOVE_OFF:
		panel_info("AOD:INFO:%s:IOCTL_SELF_MOVE_OFF\n", __func__);
		if (props->self_move_en != AOD_DRV_SELF_MOVE_OFF) {
			props->self_move_en = AOD_DRV_SELF_MOVE_OFF;
			ret = __seq_aod_self_move_off(aod);
			if (ret) {
				panel_err("AOD:ERR:%s:faield to seq self move off\n", __func__);
				goto exit_ioctl;
			}
		}
		break;
	case IOCTL_SET_TIME:
		panel_info("AOD:INFO:%s:IOCTL_SET_TIME\n", __func__);
		ret = __aod_ioctl_set_time(aod, arg);
		break;

	case IOCTL_SET_ICON:
		panel_info("AOD:INFO:%s:IOCTL_SET_ICON\n", __func__);
		ret = __aod_ioctl_set_icon(aod, arg);
		break;

	case IOCTL_SET_GRID:
		panel_info("AOD:INFO:%s:IOCTL_SET_GRID\n", __func__);
		ret = __aod_ioctl_set_grid(aod, arg);
		break;

	case IOCTL_SET_ANALOG_CLK:
		panel_info("AOD:INFO:%s:IOCTL_SET_ANALOG_CLK\n", __func__);
		ret = __aod_ioctl_set_analog_clk(aod, arg);
		break;

	case IOCTL_SET_DIGITAL_CLK:
		panel_info("AOD:INFO:%s:IOCTL_SET_DIGITAL_CLK\n", __func__);
		ret = __aod_ioctl_set_digital_clk(aod, arg);
		break;
	}

exit_ioctl:
	mutex_unlock(&aod->lock);
	mutex_unlock(&panel->io_lock);
	return ret;
}


#define IMG_HD_SZ	2

#define HEADER_SELF_ICON	"IC"
#define HEADER_ANALOG_CLK	"AC"
#define HEADER_DIGITAL_CLK	"DC"


static ssize_t aod_drv_fops_write(struct file *file, const char __user *buf,
		  size_t count, loff_t *ppos)
{
	size_t size;
	u8 header[IMG_HD_SZ];
	struct img_info *img_info;
	struct aod_dev_info *aod = file->private_data;
	struct panel_device *panel = to_panel_device(aod);

	mutex_lock(&panel->io_lock);
	mutex_lock(&aod->lock);

	if (copy_from_user(header, buf, IMG_HD_SZ)) {
		panel_err("AOD:ERR:%s:failed to get user's header\n", __func__);
		goto exit_write;
	}

	panel_info("AOD:INFO:%s:header:[0]: %x:%c, [1]: %x:%c\n", __func__,
		header[0], header[0], header[1], header[1]);


	if (!strncmp(header, HEADER_SELF_ICON, IMG_HD_SZ)) {
		img_info = &aod->icon_img;
	} else if (!strncmp(header, HEADER_ANALOG_CLK, IMG_HD_SZ)) {
		img_info = &aod->ac_img;
		/* to make mutually exclusion */
		if (aod->dc_img.up_flag)
			aod->dc_img.up_flag = 0;

	} else if (!strncmp(header, HEADER_DIGITAL_CLK, IMG_HD_SZ)) {
		img_info = &aod->dc_img;
		/* to make mutually exclusion */
		if (aod->ac_img.up_flag)
			aod->ac_img.up_flag = 0;
	} else {
		panel_info("AOD:ERR:%s:invalid header : %c%c\n", __func__,
			header[0], header[1]);
		goto exit_write;
	}

	if (img_info->buf == NULL) {
		panel_info("AOD:ERR:%s:img buf is null\n", __func__);
		goto exit_write;
	}

	size = count - IMG_HD_SZ;
	if (size > img_info->size) {
		panel_info("AOD:ERR:%s: exceed buffer size (%d:%d)\n", __func__,
			(u32)size, img_info->size);
		goto exit_write;
	}

	if (copy_from_user(img_info->buf, buf + IMG_HD_SZ, size)) {
		panel_err("AOD:ERR:%s:failed to copy img body\n", __func__);
		goto exit_write;
	}

	img_info->up_flag = 1;

exit_write:
	mutex_unlock(&aod->lock);
	mutex_unlock(&panel->io_lock);
	return count;
}

static int aod_drv_fops_release(struct inode *inode, struct file *file)
{
	int ret = 0;

	return ret;
}

static const struct file_operations aod_drv_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = aod_drv_fops_ioctl,
	.open = aod_drv_fops_open,
	.release = aod_drv_fops_release,
	.write = aod_drv_fops_write,
};

#define AOD_DEV_NAME	"self_display"

int aod_drv_probe(struct panel_device *panel, struct aod_tune *aod_tune)
{
	int i;
	int ret = 0;
	struct aod_dev_info *aod;
	struct aod_ioctl_props *props;

	if (!panel || !aod_tune) {
		pr_err("%s panel(%p) or aod_tune(%p) not exist\n",
				__func__, panel, aod_tune);
		goto rest_init;
	}

	aod = &panel->aod;
	props = &aod->props;

	aod->seqtbl = aod_tune->seqtbl;
	aod->nr_seqtbl = aod_tune->nr_seqtbl;

	aod->maptbl = aod_tune->maptbl;
	aod->nr_maptbl = aod_tune->nr_maptbl;

	aod->ops = &aod_drv_ops;
	aod->reset_flag = 1;
	props->first_clk_update = 1;

	props->self_mask_en = aod_tune->self_mask_en;
	props->self_reset_cnt = 0;

	mutex_init(&aod->lock);

#if 0
	//for test
	props->self_move_en = 1;
	props->self_move_interval = INTERVAL_DEBUG;

	//for test
	props->self_move_en = 1;

	// self icon
	props->icon_img_updated = 1;
	props->self_icon.en = 1;
	props->self_icon.pos_x = 100;
	props->self_icon.pos_y = 1500;
	props->self_icon.width = 0x34;
	props->self_icon.height = 0x34;

	// self grid
	props->self_grid.en = 1;
	props->self_grid.end_pos_x = 1440;
	props->self_grid.end_pos_y = 2960;

	// for analog clock
	aod->ac_img.up_flag = 1;
	props->analog.en = 1;
	props->analog.pos_x = 1440/2;
	props->analog.pos_y = 2960/2;
	props->debug_interval = ALG_INTERVAL_1000;
	props->analog.rotate = ALG_ROTATE_0;

	// for digital clock
	aod->dc_img.up_flag = 1;
	props->digital.en = 1;
	props->digital.en_hh = 1;
	props->digital.en_mm = 1;
	props->digital.pos1_x = 280;
	props->digital.pos1_y = 1480;
	props->digital.pos2_x = 500;
	props->digital.pos2_y = 1480;
	props->digital.pos3_x = 740;
	props->digital.pos3_y = 1480;
	props->digital.pos4_x = 960;
	props->digital.pos4_y = 1480;
	props->digital.img_width = 200;
	props->digital.img_height = 356;
	props->digital.b_en = 1;
	props->digital.b1_pos_x = 720;
	props->digital.b1_pos_y = 1580;
	props->digital.b2_pos_x = 720;
	props->digital.b2_pos_y = 1480 + 256;
	props->digital.b_color = 0x00ff00;
	props->digital.b_radius = 0x0a;
#endif

	for (i = 0; i < aod->nr_maptbl; i++) {
		aod->maptbl[i].pdata = aod;
		maptbl_init(&aod->maptbl[i]);
	}

	aod->dev.minor = MISC_DYNAMIC_MINOR;
	aod->dev.fops = &aod_drv_fops;
	aod->dev.name = AOD_DEV_NAME;
	ret = misc_register(&aod->dev);
	if (ret) {
		panel_err("PANEL:ERR:%s:faield to register for aod_dev\n", __func__);
		goto rest_init;
	}

rest_init:
	return ret;
}
