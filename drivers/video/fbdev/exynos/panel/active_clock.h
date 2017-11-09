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

#ifndef __LIVE_CLOCK_H__
#define __LIVE_CLOCK_H__

#include <linux/kernel.h>

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

#endif
//__LIVE_CLOCK_H__