/* sec_abc.h
 *
 * Abnormal Behavior Catcher Driver
 *
 * Copyright (C) 2017 Samsung Electronics
 *
 * Hyeokseon Yu <hyeokseon.yu@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef SEC_ABC_H
#define SEC_ABC_H

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sec_sysfs.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/err.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/suspend.h>

#define ABC_UEVENT_MAX		20
#define ABC_BUFFER_MAX		256
#define ABC_ENABLE_MAGIC	0x2310

enum {
	ABC_EVENT_I2C = 1,
	ABC_EVENT_UNDERRUN,
	ABC_EVENT_GPU_FAULT,
};

struct abc_fault_info {
	int cur_time;
	int cur_cnt;
};

struct abc_buffer {
	int size;
	int rear;
	int front;

	struct abc_fault_info *abc_element;
};

struct abc_qdata {
	const char *desc;
	int queue_size;
	int threshold_cnt;
	int threshold_time;

	struct abc_buffer buffer;
};

struct abc_platform_data {
	struct abc_qdata *gpu_items;
	struct abc_qdata *aicl_items;

	unsigned int nItem;
	unsigned int nGpu;
	unsigned int nAicl;
};

struct abc_info {
	struct device *dev;
	struct abc_platform_data *pdata;
};

extern void sec_abc_send_event(char *str);
extern int sec_abc_get_magic(void);
#endif
