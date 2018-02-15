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

#include <linux/notifier.h>
#include <linux/slab.h>
#include "score-regs.h"
#include "score-dump.h"

struct atomic_notifier_head score_dump_notifier_list;
struct notifier_block score_dump_nb;
struct kthread_work *score_dump_work;
void __iomem *score_dump_reg;
void *score_dump_kvaddr;

static void score_dump_work_fn(struct kthread_work *work)
{
	atomic_notifier_call_chain(&score_dump_notifier_list, 0, NULL);
}

static int score_dump_notifier_start_event(struct notifier_block *nb,
		unsigned long event, void *data)
{
	score_info("@@@ SCORE dump notifier call chain!!\n");
	return NOTIFY_DONE;
}

int score_dump_notifier_register(struct notifier_block *nb)
{
	return atomic_notifier_chain_register(&score_dump_notifier_list, nb);
}
EXPORT_SYMBOL_GPL(score_dump_notifier_register);

int score_dump_notifier_unregister(struct notifier_block *nb)
{
	return atomic_notifier_chain_register(&score_dump_notifier_list, nb);
}
EXPORT_SYMBOL_GPL(score_dump_notifier_unregister);

void score_dump_queue_work(struct score_system *system)
{
	struct score_interface *itf = &system->interface;

	if (score_dump_kvaddr) {
		queue_kthread_work(&itf->worker, score_dump_work);
	}
}

int score_dump_open(struct score_system *system)
{
	score_dump_kvaddr = system->dump_kvaddr;
	return 0;
}

void score_dump_close(struct score_system *system)
{
	score_dump_kvaddr = NULL;
}

int score_dump_probe(struct score_system *system)
{
	score_dump_reg = system->regs + SCORE_DSP_INT;

	score_dump_work = kzalloc(sizeof(*score_dump_work), GFP_KERNEL);
	if (!score_dump_work) {
		probe_err("score_dump_work is not allocated\n");
		score_dump_reg = NULL;
		score_dump_kvaddr = NULL;
		return -ENOMEM;
	}

	init_kthread_work(score_dump_work, score_dump_work_fn);

	score_dump_nb.notifier_call = score_dump_notifier_start_event;
	score_dump_nb.next = NULL;
	score_dump_nb.priority = 0;
	score_dump_notifier_register(&score_dump_nb);

	return 0;
}

void score_dump_release(void)
{
	score_dump_reg = NULL;
	score_dump_notifier_unregister(&score_dump_nb);
	kfree(score_dump_work);
}
