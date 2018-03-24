/* drivers/soc/samsung/pmu_count.c
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com
 *
 * Samsung CPU Support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kobject.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/cpumask.h>

#include <soc/samsung/pmu_func.h>

/* 0x11 = ccnt
 * 0x08 = instruction
 * 0x06 = load instruction
 * 0x07 = store instruction
 * 0x19 = bus access
 * 0x17 = L2 cache refil
 * 0x23 = stall frontend
 * 0x24 = stall backend
 * 0x1d = bus cycle
 */
#define PMU_EVENT_MAX	6

static int events[PMU_EVENT_MAX] = {0x11, 0x08};

int init_counter_cpu(void)
{
	int i;

	disable_pmu();
	for (i = 0; i < ARRAY_SIZE(events); i++) {
		pmu_enable_counter(i);
		pmnc_config(i, events[i]);
	}

	return 0;
}

int start_counter_cpu(void)
{
	pmnc_reset();
	enable_pmu();

	return 0;
}

int stop_counter_cpu(void)
{
	disable_pmu();

	return 0;
}

int read_pmu_one(void *data)
{
	struct pmu_count_value *p = (struct pmu_count_value *)data;

	p->pmnc0 += pmnc_read2(0);
	p->pmnc1 += pmnc_read2(1);

	return 0;
}
int reset_pmu_one(void *data)
{
	struct pmu_count_value *p = (struct pmu_count_value *)data;

	p->pmnc0 = 0;
	p->pmnc1 = 0;
	return 0;
}
