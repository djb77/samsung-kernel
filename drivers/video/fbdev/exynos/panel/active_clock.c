/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *	      http://www.samsung.com/
 *
 * Samsung's Active Clock Driver
 * Author: Minwoo Kim <minwoo7945.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/highmem.h>
#include <linux/bug.h>

#include "panel.h"
#include "panel_bl.h"
#include "panel_drv.h"
#include "active_clock.h"

static int check_need_update(struct panel_device *panel,
	struct act_clk_info *act_info, struct ioctl_act_clk *ioctl_info)
{
	int ret = 0;
	struct panel_state *state;

	if (!panel || !act_info|| !ioctl_info) {
		panel_err("LIVE-CLK:ERR:%s:invalid argument\n", __func__);
		return -1;
	}
	state = &panel->state;

	if (act_info->en != ioctl_info->en)
		ret = 1;

	if ((act_info->pos_x != ioctl_info->pos_x) ||
		(act_info->pos_y != ioctl_info->pos_y))
		ret = 1;

	if ((act_info->time_hr != ioctl_info->time_hr) ||
		(act_info->time_min != ioctl_info->time_min) ||
		(act_info->time_sec != ioctl_info->time_sec) ||
		(act_info->time_ms != ioctl_info->time_ms)) {
		if (state->cur_state == PANEL_STATE_ALPM) {
			act_info->update_time = 1;
		}
		ret = 1;
	}
	if (act_info->interval != ioctl_info->interval)
		ret = 1;

	act_info->en = ioctl_info->en;
	act_info->pos_x = ioctl_info->pos_x;
	act_info->pos_y = ioctl_info->pos_y;
	act_info->time_hr = ioctl_info->time_hr;
	act_info->time_min = ioctl_info->time_min;
	act_info->time_sec = ioctl_info->time_sec;
	act_info->time_ms = ioctl_info->time_ms;
	act_info->interval = ioctl_info->interval;

	panel_info("ACT-CLK:En : %d\n", act_info->en);
	panel_info("ACT-CLK:Axis:x:%d,y:%d\n", act_info->pos_x,
		act_info->pos_y);
	panel_info("ACT-CLK:Time::%d:%d:%d:%d\n",act_info->time_hr,
		act_info->time_min, act_info->time_sec, act_info->time_ms);
	panel_info("ACT-CLK:Interval : %d\n", act_info->interval);

	return ret;
}

static int set_active_clock(struct act_clock_dev *act_dev, void __user *arg)
{
	int ret= 0;
	int need_update = 0;
	struct panel_device *panel;
	struct panel_state *state;
	struct ioctl_act_clk ioctl_info;
	struct act_clk_info *act_info = &act_dev->act_info;


	panel = container_of(act_dev, struct panel_device, act_clk_dev);
	if (panel == NULL) {
		panel_err("LIVE-CLK:ERR:%s: can't found panel drvier\n", __func__);
		ret = -EINVAL;
		goto set_exit;
	}

	if (copy_from_user(&ioctl_info,(struct ioctl_act_clk __user *)arg,
			sizeof(struct ioctl_act_clk))) {
		panel_err("LIVE-CLK:ERR:%s:failed to copy from user's active param\n", __func__);
		goto set_exit;
	}

	need_update = check_need_update(panel, act_info, &ioctl_info);
	if (!need_update) {
		panel_info("ACT-CLK:INFO:%s:need_update : %d\n",
			__func__, need_update);
		goto set_exit;
	}
	state = &panel->state;

	if (state->cur_state != PANEL_STATE_ALPM) {
		panel_info("ACT-CLK:INFO:%s:setting for active clock was ignored : %d\n",
			__func__, state->cur_state);
		goto set_exit;
	}

	panel_info("PANEL:INFO:%s:Update Active Clock info\n", __func__);

	if ((act_info->en == 1) &&
		(act_info->update_img == IMG_UPDATE_NEED)) {
		panel_info("PANEL:INFO:%s:Update Active Clock image \n", __func__);
		ret = panel_do_seqtbl_by_index(panel, PANEL_ACTIVE_CLK_IMG_SEQ);
		if (unlikely(ret < 0)) {
			panel_err("ACT-CLK:ERR:%s, failed to write init seqtbl\n", __func__);
		}
		act_info->update_img = IMG_UPDATE_DONE;
	}

	if (act_info->update_time) {
		ret = panel_do_seqtbl_by_index(panel, PANEL_ACTIVE_CLK_UPDATE_SEQ);
		if (unlikely(ret < 0)) {
			panel_err("ACT-CLK:ERR:%s, failed to write init seqtbl\n", __func__);
		}
		act_info->update_time = 0;
	} else {
		ret = panel_do_seqtbl_by_index(panel, PANEL_ACTIVE_CLK_CTRL_SEQ);
		if (unlikely(ret < 0)) {
			panel_err("ACT-CLK:ERR:%s, failed to write init seqtbl\n", __func__);
		}
	}

set_exit:
	return 0;
}

static int set_blink_clock(struct act_clock_dev *act_dev, struct ioctl_blink_clock *ioctl_info)
{
	struct act_blink_info *blink_info = &act_dev->blink_info;

	blink_info->en = ioctl_info->en;
	blink_info->interval = ioctl_info->interval;
	blink_info->radius = ioctl_info->radius;
	blink_info->color = ioctl_info->color;
	blink_info->line_width = ioctl_info->line_width;
	blink_info->pos1_x = ioctl_info->pos1_x;
	blink_info->pos1_y = ioctl_info->pos1_y;
	blink_info->pos2_x = ioctl_info->pos2_x;
	blink_info->pos2_y = ioctl_info->pos2_y;

	return 0;
}

static int set_self_drawer(struct act_clock_dev *act_dev, void __user *arg)
{
	int ret = 0;
	struct panel_device *panel;
	struct ioctl_self_drawer ioctl_info;
	struct act_drawer_info *draw_info = &act_dev->draw_info;

	panel = container_of(act_dev, struct panel_device, act_clk_dev);
	if (panel == NULL) {
		panel_err("LIVE-CLK:ERR:%s: can't found panel drvier\n", __func__);
		ret = -EINVAL;
		goto set_exit;
	}

	if (copy_from_user(&ioctl_info,(struct ioctl_self_drawer __user *)arg,
			sizeof(struct ioctl_self_drawer))) {
		panel_err("LIVE-CLK:ERR:%s:failed to copy from user's active param\n", __func__);
		goto set_exit;
	}

	panel_info("PANEL:INFO:%s:Drawer Info : color:%x, color2:%x, radius:%x\n",
		__func__, ioctl_info.sd_line_color,
		ioctl_info.sd2_line_color, ioctl_info.sd_raduis);

	draw_info->sd_line_color = ioctl_info.sd_line_color;
	draw_info->sd2_line_color = ioctl_info.sd2_line_color;
	draw_info->sd_radius = ioctl_info.sd_raduis;


set_exit:
	return ret;
}

static long active_clock_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = -EFAULT;
	struct panel_device *panel;
	struct ioctl_blink_clock ioctl_blink;
	struct act_clock_dev *act_dev = file->private_data;

	if (!act_dev->opened) {
		panel_err("LIVE-CLK:ERR:%s: ioctl error\n", __func__);
		return -EIO;
	}

	panel = container_of(act_dev, struct panel_device, act_clk_dev);
	if (panel == NULL) {
		panel_err("LIVE-CLK:ERR:%s: can't found panel drvier\n", __func__);
		return -EINVAL;
	}

	if (!IS_PANEL_ACTIVE(panel)) {
		panel_err("LIVE-CLK:WARN:%s:panel is not active\n", __func__);
		return -EAGAIN;
	}

	mutex_lock(&panel->io_lock);

	switch (cmd) {
		case IOCTL_ACT_CLK :
			panel_info("ACT-CLK:INFO:%s:IOCTL_ACT_CLK\n", __func__);
			ret = set_active_clock(act_dev, (void __user *)arg);
			break;

		case IOCTL_BLINK_CLK :
			panel_info("ACT-CLK:INFO:%s:IOCTL_BLINK_CLK\n", __func__);
			if (copy_from_user(&ioctl_blink, (struct ioctl_blink_clock __user *)arg,
				sizeof(struct ioctl_blink_clock))) {
				panel_err("ACT-CLK:ERR:%s:failed to copy from user's blink param\n", __func__);
				goto exit_ioctl;
			}
			ret = set_blink_clock(act_dev, &ioctl_blink);

			break;

		case IOCTL_SELF_DRAWER_CLK:
			panel_info("ACT-CLK:INFO:%s:IOCTL_SELF_DRAWER_CLK\n", __func__);
			ret = set_self_drawer(act_dev, (void __user *)arg);
			break;

		default:
			panel_err("ACT-CLK:ERR:%s : invalid cmd : %d\n", __func__, cmd);
			ret = -EINVAL;
			goto exit_ioctl;

	}

exit_ioctl:
	mutex_unlock(&panel->io_lock);
	return ret;
}


static int active_clock_open(struct inode *inode, struct file *file)
{
	int ret = 0;
	struct panel_device *panel;
	struct seqinfo *img_seqtbl = NULL;
	struct pktinfo *img_pktinfo = NULL;
	struct miscdevice *dev = file->private_data;
	struct act_clock_dev *act_dev = container_of(dev, struct act_clock_dev, dev);
	struct act_clk_info *act_info = &act_dev->act_info;

	panel_info("%s was called\n", __func__);

	if (act_dev->opened) {
		panel_err("LIVE-CLK:ERR:%s: already opend\n", __func__);
		ret = -EBUSY;
		goto exit_open;
	}

	panel = container_of(act_dev, struct panel_device, act_clk_dev);
	if (panel == NULL) {
		panel_err("LIVE-CLK:ERR:%s: can't found panel drvier\n", __func__);
		ret = -EINVAL;
		goto exit_open;
	}

	img_seqtbl = find_panel_seqtbl(&panel->panel_data, "active-clk-img-seq");
	if (img_seqtbl == NULL) {
		panel_err("LIVE-CLK:ERR:%s: can't found live clock image seqtbl\n", __func__);
		ret = -EINVAL;
		goto exit_open;
	}

	img_pktinfo = find_packet_suffix(img_seqtbl, "active_clk_img_pkt");
	if (img_pktinfo == NULL) {
		panel_err("LIVE-CLK:ERR:%s: can't found live clock image packet\n", __func__);
		ret = -EINVAL;
		goto exit_open;
	}

	panel_info("LIVE-CLK:INFO: img-pkt-info[0] : %d, leng : %d\n",
		img_pktinfo->data[0], img_pktinfo->dlen);

	act_info->img_buf = img_pktinfo->data;
	act_info->img_buf_size = img_pktinfo->dlen;

	file->private_data = act_dev;
	act_dev->opened = 1;

exit_open:
	return ret;
}

static int active_clock_release(struct inode *inode, struct file *file)
{
	int ret = 0;
	struct act_clock_dev *act_dev = file->private_data;
	struct act_clk_info *act_info = &act_dev->act_info;

	act_info->img_buf = NULL;
	act_info->img_buf_size = 0;
	act_dev->opened = 0;

	return ret;
}

static ssize_t active_clock_write(struct file *file, const char __user *buf,
			 size_t count, loff_t *ppos)
{
	int ret = -EINVAL;
	struct act_clock_dev *act_dev = file->private_data;
	struct act_clk_info *act_info = &act_dev->act_info;

	panel_info("%s : size : %d, ppos %d\n", __func__, (int)count, (int)*ppos);

	if (!act_dev->opened) {
		panel_err("LIVE-CLK:ERR:%s: ioctl error\n", __func__);
		return -EIO;
	}

	if (!act_info->img_buf) {
		panel_err("LIVE-CLK:ERR:%s: img buf is null : %d\n", __func__, (int)count);
		goto exit_write;
	}
	if (count != act_info->img_buf_size) {
		panel_err("LIVE-CLK:ERR:%s: wrong file size : %d\n", __func__, (int)count);
		goto exit_write;
	}

	ret = -EFAULT;
	if (copy_from_user(act_info->img_buf, buf, count)) {
		panel_err("LIVE-CLK:ERR:%s: failed to copy from user data\n", __func__);
		goto exit_write;
	}

	return count;

exit_write:
	return ret;
}

static const struct file_operations act_clk_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = active_clock_ioctl,
	.open = active_clock_open,
	.release = active_clock_release,
	.write = active_clock_write,
};

int probe_live_clock_drv(struct act_clock_dev *act_dev)
{
	int ret = 0;
	struct panel_device *panel;
	struct act_clk_info *act_info;
	struct act_blink_info *blink_info;
	struct act_drawer_info *draw_info;

	if (act_dev == NULL) {
		panel_err("LIVE-CLK:ERR:%s: invalid live clk \n", __func__);
		return -EINVAL;
	}
	act_dev->dev.minor = MISC_DYNAMIC_MINOR;
	act_dev->dev.name = "act_clk";
	act_dev->dev.fops = &act_clk_fops;
	act_dev->dev.parent = NULL;

	ret = misc_register(&act_dev->dev);
	if (ret) {
		panel_err("LIVE-CLK:ERR:%s: faield to register driver : %d\n", __func__, ret);
		goto exit_probe;
	}

	panel = container_of(act_dev, struct panel_device, act_clk_dev);
	if (panel == NULL) {
		panel_err("LIVE-CLK:ERR:%s: can't found panel drvier\n", __func__);
		ret = -EINVAL;
		goto exit_probe;
	}

	act_info = &act_dev->act_info;
	blink_info = &act_dev->blink_info;
	draw_info = &act_dev->draw_info;

	act_info->en = 0;
	act_info->interval = 1000;
	act_info->time_hr = 12;
	act_info->time_min = 0;
	act_info->time_sec = 0;
	act_info->time_ms = 0;
	act_info->pos_x = 0;
	act_info->pos_y = 0;
	act_info->img_buf = NULL;
	act_info->img_buf_size = 0;

	blink_info->en = 0;
	blink_info->interval = 5;
	blink_info->radius = 10;
	blink_info->color = 0x000000;
	blink_info->line_width = 0;;
	blink_info->pos1_x = 0;
	blink_info->pos1_y = 0 ;
	blink_info->pos2_x = 0;
	blink_info->pos2_y = 0;

	draw_info->sd_line_color = 0x00F0F0F0;
	draw_info->sd2_line_color = 0x00000000;
	draw_info->sd_radius = 0x0A;

	act_dev->opened = 0;

exit_probe:
	return ret;
}
