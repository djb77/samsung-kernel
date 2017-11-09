/*
 * Samsung Exynos SoC series SCORE driver
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef SCORE_INTERFACE_H_
#define SCORE_INTERFACE_H_

#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/timer.h>

#include "score-framemgr.h"

struct score_interface {
	void __iomem			*regs;
	resource_size_t			regs_size;

	struct score_framemgr		framemgr;
	struct score_vertexmgr		*vertexmgr;

	struct kthread_worker		worker;
	struct task_struct		*task_itf;

	spinlock_t			int_slock;
};

int score_interface_probe(struct score_interface *interface,
		struct device *dev, void __iomem *regs,
		resource_size_t regs_size, u32 irq0);
void score_interface_release(struct score_interface *interface,
		struct device *dev, u32 irq0);
int score_interface_open(struct score_interface *interface);
int score_interface_close(struct score_interface *interface);

#endif
