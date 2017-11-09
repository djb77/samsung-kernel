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

#ifndef _SCORE_DUMP_H_
#define _SCORE_DUMP_H_

#include <linux/kthread.h>
#include "score-system.h"

#define DUMP_BUF_SIZE	0x34000

extern struct atomic_notifier_head score_dump_notifier_list;

extern void *score_dump_kvaddr;
extern struct kthread_work *score_dump_work;

extern int score_dump_notifier_register(struct notifier_block *nb);
extern int score_dump_notifier_unregister(struct notifier_block *nb);

void score_dump_queue_work(struct score_system *system);
int score_dump_open(struct score_system *system);
void score_dump_close(struct score_system *system);
int score_dump_probe(struct score_system *system);
void score_dump_release(void);

#endif

