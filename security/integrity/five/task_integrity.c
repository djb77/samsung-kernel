/*
 * Task Integrity Verifier
 *
 * Copyright (C) 2016 Samsung Electronics, Inc.
 * Egor Uleyskiy, <e.uleyskiy@samsung.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/task_integrity.h>
#include <linux/slab.h>
#include "five_porting.h"

static struct kmem_cache *task_integrity_cache;

static void init_once(void *foo)
{
	struct task_integrity *intg = foo;

	memset(intg, 0, sizeof(*intg));
	spin_lock_init(&intg->value_lock);
	spin_lock_init(&intg->list_lock);
}

static int __init task_integrity_cache_init(void)
{
	task_integrity_cache = kmem_cache_create("task_integrity_cache",
			sizeof(struct task_integrity), 0,
			SLAB_HWCACHE_ALIGN|SLAB_PANIC|SLAB_NOTRACK, init_once);

	if (!task_integrity_cache)
		return -ENOMEM;

	return 0;
}

security_initcall(task_integrity_cache_init);

struct task_integrity *task_integrity_alloc(void)
{
	struct task_integrity *intg;

	intg = kmem_cache_alloc(task_integrity_cache, GFP_KERNEL);
	if (intg) {
		atomic_set(&intg->usage_count, 1);
		INIT_LIST_HEAD(&intg->events.list);
	}

	return intg;
}

void task_integrity_free(struct task_integrity *intg)
{
	if (intg) {
		spin_lock(&intg->value_lock);
		kfree(intg->label);
		intg->label = NULL;
		intg->user_value = INTEGRITY_NONE;
		spin_unlock(&intg->value_lock);

		atomic_set(&intg->usage_count, 0);
		task_integrity_set(intg, INTEGRITY_NONE);
		kmem_cache_free(task_integrity_cache, intg);
	}
}

void task_integrity_clear(struct task_integrity *tint)
{
	task_integrity_set(tint, INTEGRITY_NONE);
	spin_lock(&tint->value_lock);
	kfree(tint->label);
	tint->label = NULL;
	spin_unlock(&tint->value_lock);
}

static int copy_label(struct task_integrity *from, struct task_integrity *to)
{
	int ret = 0;
	struct integrity_label *l = NULL;

	if (task_integrity_read(from) && from->label)
		l = from->label;

	if (l) {
		struct integrity_label *label;

		label = kmalloc(sizeof(*label) + l->len, GFP_ATOMIC);
		if (!label) {
			ret = -ENOMEM;
			goto exit;
		}

		label->len = l->len;
		memcpy(label->data, l->data, label->len);
		to->label = label;
	}

exit:
	return ret;
}

int task_integrity_copy(struct task_integrity *from, struct task_integrity *to)
{
	int rc = -EPERM;
	enum task_integrity_value value = task_integrity_read(from);

	task_integrity_set(to, value);

	if (list_empty(&from->events.list))
		to->user_value = value;

	rc = copy_label(from, to);

	return rc;
}
