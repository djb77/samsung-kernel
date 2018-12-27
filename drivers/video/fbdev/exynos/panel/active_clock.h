/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *	      http://www.samsung.com/
 *
 * Samsung's Live Clock Driver
 * Author: Minwoo Kim <minwoo7945.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __ACTIVE_CLOCK_H__
#define __ACTIVE_CLOCK_H__

#include <linux/kernel.h>
#include <linux/miscdevice.h>

enum {
	IMG_UPDATE_NEED = 0,
	IMG_UPDATE_DONE,
};

struct act_clk_info {
	u32 en;
	u32 interval;
	u32 time_hr;
	u32 time_min;
	u32 time_sec;
	u32 time_ms;
	u32 pos_x;
	u32 pos_y;
/* flag to check need to update side ram img */
	u32 update_img;
	char *img_buf;
	u32 img_buf_size;
	u32 update_time;
};

struct act_blink_info {
	u32 en;
	u32 interval;
	u32 radius;
	u32 color;
	u32 line_width;
	u32 pos1_x;
	u32 pos1_y;
	u32 pos2_x;
	u32 pos2_y;
};

struct act_drawer_info {
	u32 sd_line_color;
	u32 sd2_line_color;
	u32 sd_radius;
};

struct act_clock_dev {
	struct miscdevice dev;
	struct act_clk_info act_info;
	struct act_blink_info blink_info;
	struct act_drawer_info draw_info;
	u32 opened;
};

struct ioctl_act_clk {
	unsigned int en;
	unsigned int interval;
	unsigned int time_hr;
	unsigned int time_min;
	unsigned int time_sec;
	unsigned int time_ms;
	unsigned int pos_x;
	unsigned int pos_y;
};

struct ioctl_blink_clock {
	u32 en;
	u32 interval;
	u32 radius;
	u32 color;
	u32 line_width;
	u32 pos1_x;
	u32 pos1_y;
	u32 pos2_x;
	u32 pos2_y;
};

struct ioctl_self_drawer {
	u32 sd_line_color;
	u32 sd2_line_color;
	u32 sd_raduis;
};

#define IOCTL_ACT_CLK 	_IOW('A', 100, struct ioctl_act_clk)
#define IOCTL_BLINK_CLK _IOW('A', 101, struct ioctl_blink_clock)
#define IOCTL_SELF_DRAWER_CLK _IOW('A', 102, struct ioctl_self_drawer)

int probe_live_clock_drv(struct act_clock_dev *act_dev);

#endif /* __ACTIVE_CLOCK_H__ */
