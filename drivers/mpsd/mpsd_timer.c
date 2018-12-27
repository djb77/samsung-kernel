/*
 * drivers/sysintelligence/mpsd_timer.c
 *
 * Source file for mpsd param synchronous read code.
 *
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *	   Ravi Solanki <ravi.siso@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/hrtimer.h>
#include <linux/workqueue.h>

#include "mpsd_param.h"

#define NSEC_PER_MSEC 1000000L
#define MIN_TIMER_LIMIT (DEFAULT_TIMER_MIN_LIMIT)

static DEFINE_SPINLOCK(mpsd_timer_list_lock);

static struct workqueue_struct *mpsd_timer_wq;

static int timer_pid = DEFAULT_PID;

/**
 * struct mpsd_timer - Struct for timer framework.
 * @value: Value in msec for the timer to be run.
 * @node_count: Total number of timers (params reg) with same timer value.
 * @timer: Timer structure handle.
 * @list: Head node of the timer list.
 * @app_field_mask: Bit field to be update on the app params registration.
 * @dev_field_mask: Bit field to be update on the dev params registration.
 *
 * This structure contains the information of the mpsd timer framework
 * for registering the mpsd params for timed update.
 */
struct mpsd_timer {
	long long value;
	int node_count;
	struct hrtimer timer;
	struct list_head list;
	unsigned long long app_field_mask;
	unsigned long long dev_field_mask;
};

static struct mpsd_timer mpsd_timers = {
	.list = LIST_HEAD_INIT(mpsd_timers.list),
};

/**
 * struct mpsd_timer_work - Struct to pass timer work related data.
 * @timer_work: work handle for work queue.
 * @app_field_mask: Bit field to be update on the app params registration.
 * @dev_field_mask: Bit field to be update on the dev params registration.
 *
 * This structure contains the information of the mpsd timer work queue
 * related data which will be filled from timer callback.
 */
struct mpsd_timer_work {
	struct work_struct timer_work;
	unsigned long long app_field_mask;
	unsigned long long dev_field_mask;
};

struct mpsd_timer_work mpsd_timer_cb_work;

static inline int get_timer_pid(void)
{
	return timer_pid;
}

void mpsd_set_timer_pid(int pid)
{
	if (pid <= 0)
		pid = DEFAULT_PID;
	timer_pid = pid;
}

static struct mpsd_timer *mpsd_timer_set(s64 time)
{
	struct mpsd_timer *t = NULL;
	struct mpsd_timer *tl_head = &mpsd_timers;

	if (time < MIN_TIMER_LIMIT)
		time = MIN_TIMER_LIMIT;

	t = kmalloc(sizeof(*t), GFP_KERNEL);
	if (!t)
		return t;

	t->value = time;
	t->node_count = 0;
	t->app_field_mask = 0;
	t->dev_field_mask = 0;
	INIT_LIST_HEAD(&t->list);

	spin_lock(&mpsd_timer_list_lock);
	list_add_tail(&t->list, &tl_head->list);
	spin_unlock(&mpsd_timer_list_lock);

	return t;
}

static enum hrtimer_restart timer_callback(struct hrtimer *timer)
{
	struct mpsd_timer *t;

#ifdef CONFIG_MPSD_DEBUG
	if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
		pr_info("%s: %s:\n", tag, __func__);
#endif

	t = container_of(timer, struct mpsd_timer, timer);

	mpsd_timer_cb_work.app_field_mask = t->app_field_mask;
	mpsd_timer_cb_work.dev_field_mask = t->dev_field_mask;
	queue_work(mpsd_timer_wq, &mpsd_timer_cb_work.timer_work);

	hrtimer_forward(timer, hrtimer_cb_get_time(timer),
		ms_to_ktime(t->value));
#ifdef CONFIG_MPSD_DEBUG
	if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN) {
		pr_info("%s: %s: Callback Done => Val[%lld]:Masks[%llu %llu]\n",
			tag, __func__, t->value, t->app_field_mask,
			t->dev_field_mask);
	}

	if (get_debug_level_timer() <=	get_mpsd_debug_level()) {
		print_timer_events_data(t->value, t->node_count,
			t->app_field_mask, t->dev_field_mask);
	}
#endif
	return HRTIMER_RESTART;
}

static int start_timer(struct mpsd_timer *t)
{
	ktime_t ktime;

	pr_info("%s: %s: Starting the timer for Val[%lld]\n", tag, __func__,
			t->value);

	hrtimer_init(&t->timer, CLOCK_MONOTONIC,  HRTIMER_MODE_REL);
	ktime = ms_to_ktime(t->value);
	t->timer.function = &timer_callback;
	hrtimer_start(&t->timer, ktime, HRTIMER_MODE_REL);

	return 0;
}

int mpsd_timer_register(int param_id, int val)
{
	long long time = (long long int)val;
	int is_app_param = 0;
	struct mpsd_timer *t;
	struct mpsd_timer *tl_head = &mpsd_timers;

	if (is_valid_param(param_id) == 0) {
		pr_err("%s: %s: Parameter is not Valid\n", tag, __func__);
		return -ENOENT;
	}

	//delete existing param entry
	spin_lock(&mpsd_timer_list_lock);
	if (is_valid_app_param(param_id) == 1)
		is_app_param = 1;

	list_for_each_entry(t, &tl_head->list, list) {
		unsigned long long bit_field = is_app_param ?
			t->app_field_mask : t->dev_field_mask;

		if (get_param_bit(bit_field, param_id)) {
			spin_unlock(&mpsd_timer_list_lock);
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_info("%s: %s Exist, Dereg first from VAL[%lld]\n",
				tag, __func__, t->value);
#endif
			mpsd_timer_deregister(param_id);
			spin_lock(&mpsd_timer_list_lock);
			goto deleted;
		}
	}

deleted:
	list_for_each_entry(t, &tl_head->list, list) {
		if (t->value == time) {
			spin_unlock(&mpsd_timer_list_lock);
			goto found;
		}
	}
	spin_unlock(&mpsd_timer_list_lock);
	t = mpsd_timer_set(time);
	if (!t) {
		pr_err("%s: timer allocation failed\n", __func__);
		return -ENOMEM;
	}

#ifdef CONFIG_MPSD_DEBUG
	if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN) {
		pr_info("%s: %s Added Timer[%lld] in List\n", tag, __func__,
			t->value);
	}
#endif

found:
	spin_lock(&mpsd_timer_list_lock);

	if (is_app_param == 1)
		set_param_bit(&t->app_field_mask, param_id);
	else
		set_param_bit(&t->dev_field_mask, param_id);
	if ((++t->node_count) == 1) {
		pr_info("%s: %s: ID:%d Started VAL[%lld] node_count=%d\n",
			tag, __func__, param_id, t->value, t->node_count);
		start_timer(t);
	}
	spin_unlock(&mpsd_timer_list_lock);

#ifdef CONFIG_MPSD_DEBUG
	if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN) {
		pr_info("%s: %s:ID:%d Reg VAL[%lld] count=%d MASK[%llu %llu]\n",
			tag, __func__, param_id,
			t->value, t->node_count, t->app_field_mask,
			t->dev_field_mask);
	}

	if (get_debug_level_timer() <=	get_mpsd_debug_level())
		print_timer_reg_data(0, NULL);
#endif
	return 0;
}

int mpsd_timer_deregister(int param_id)
{
	int is_app_param = 0;
	struct mpsd_timer *t;
	struct mpsd_timer *tl_head = &mpsd_timers;

	if (is_valid_param(param_id) == 0) {
		pr_err("%s: %s: Parameter is not Valid\n", tag, __func__);
		return -ENOENT;
	}

#ifdef CONFIG_MPSD_DEBUG
	if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
		pr_info("%s: %s: ID:%d Unreg came\n", tag, __func__, param_id);
#endif

	spin_lock(&mpsd_timer_list_lock);
	if (is_valid_app_param(param_id) == 1)
		is_app_param = 1;

	list_for_each_entry(t, &tl_head->list, list) {
		unsigned long long bit_field = is_app_param ?
			t->app_field_mask : t->dev_field_mask;

		if (get_param_bit(bit_field, param_id)) {
			clear_param_bit(&bit_field, param_id);
			if (is_app_param == 1)
				t->app_field_mask = bit_field;
			else
				t->dev_field_mask = bit_field;
			--t->node_count;
			goto done;
		}
	}

	pr_info("%s: %s: ID[%d] Not Found\n", tag, __func__, param_id);
	spin_unlock(&mpsd_timer_list_lock);
	return -EINVAL;

done:

#ifdef CONFIG_MPSD_DEBUG
	if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN) {
		pr_info("%s: %s:ID:%d Unreg VAL[%lld] ct=%d MASK[%llu %llu]\n",
			tag, __func__, param_id,
			t->value, t->node_count, t->app_field_mask,
			t->dev_field_mask);
	}
#endif

	if (t->node_count == 0) {
		hrtimer_cancel(&t->timer);
		list_del(&t->list);
		kfree(t);
	}
	spin_unlock(&mpsd_timer_list_lock);

#ifdef CONFIG_MPSD_DEBUG
	if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
		pr_info("%s: %s: ID:%d Unreg Done!\n", tag, __func__, param_id);

	if (get_debug_level_timer() <=	get_mpsd_debug_level())
		print_timer_reg_data(0, NULL);
#endif

	return 0;
}

void mpsd_timer_deregister_all(void)
{
	struct mpsd_timer *t, *q;
	struct mpsd_timer *tl_head = &mpsd_timers;

	spin_lock(&mpsd_timer_list_lock);
	list_for_each_entry_safe(t, q, &tl_head->list, list) {
		hrtimer_cancel(&t->timer);
		list_del(&t->list);
		kfree(t);
	}
	spin_unlock(&mpsd_timer_list_lock);

	pr_info("%s: %s: Done!\n", tag, __func__);
#ifdef CONFIG_MPSD_DEBUG
	if (get_debug_level_timer() <=	get_mpsd_debug_level())
		print_timer_reg_data(0, NULL);
#endif
	flush_work(&mpsd_timer_cb_work.timer_work);
}

void mpsd_timer_suspend(void)
{
	struct mpsd_timer *t, *q;
	struct mpsd_timer *tl_head =  &mpsd_timers;

	spin_lock(&mpsd_timer_list_lock);
	list_for_each_entry_safe(t, q, &tl_head->list, list) {
		hrtimer_cancel(&t->timer);
	}
	spin_unlock(&mpsd_timer_list_lock);
	pr_info("%s: %s: Done!\n", tag, __func__);
#ifdef CONFIG_MPSD_DEBUG
	if (get_debug_level_timer() <=	get_mpsd_debug_level())
		print_timer_reg_data(0, NULL);
#endif
}

void mpsd_timer_resume(void)
{
	ktime_t ktime;
	struct mpsd_timer *t, *q;
	struct mpsd_timer *tl_head =  &mpsd_timers;

	spin_lock(&mpsd_timer_list_lock);
	list_for_each_entry_safe(t, q, &tl_head->list, list) {
		hrtimer_cancel(&t->timer);
		ktime = ms_to_ktime(t->value);
		hrtimer_start(&t->timer, ktime, HRTIMER_MODE_REL);
	}
	spin_unlock(&mpsd_timer_list_lock);
	pr_info("%s: %s: Done!\n", tag, __func__);
#ifdef CONFIG_MPSD_DEBUG
	if (get_debug_level_timer() <=	get_mpsd_debug_level())
		print_timer_reg_data(0, NULL);
#endif
}

static void mpsd_timer_cb_work_fn(struct work_struct *work)
{
	struct mpsd_timer_work *cb_work = container_of(work,
		struct mpsd_timer_work, timer_work);

#ifdef CONFIG_MPSD_DEBUG
	if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN) {
		pr_info("%s: %s: Added in queue => Masks[%llu %llu]\n",
			tag, __func__, cb_work->app_field_mask,
			cb_work->dev_field_mask);
	}
#endif

	mpsd_populate_mmap(cb_work->app_field_mask, cb_work->dev_field_mask,
		get_timer_pid(), TIMED_UPDATE, DEFAULT_EVENTS);
}

void mpsd_timer_init(void)
{
	pr_info("%s: %s:\n", tag, __func__);

	mpsd_timer_wq = alloc_ordered_workqueue("mpsd_timer_wq",
		WQ_FREEZABLE);

	BUG_ON(!mpsd_timer_wq);

	mpsd_timer_cb_work.app_field_mask = DEFAULT_APP_FIELD_MASK;
	mpsd_timer_cb_work.dev_field_mask = DEFAULT_DEV_FIELD_MASK;
	INIT_WORK(&mpsd_timer_cb_work.timer_work, mpsd_timer_cb_work_fn);
	pr_info("%s: %s: Done!\n", tag, __func__);
}

void mpsd_timer_exit(void)
{
	pr_info("%s: %s:\n", tag, __func__);

	mpsd_timer_deregister_all();
	destroy_workqueue(mpsd_timer_wq);
	pr_info("%s: %s: Done!\n", tag, __func__);
}

#ifdef CONFIG_MPSD_DEBUG
int print_timer_reg_data(int in_buf, char *buf)
{
	int ret = 0;
	struct mpsd_timer *t = NULL;
	struct mpsd_timer *tl_head = &mpsd_timers;

	spin_lock(&mpsd_timer_list_lock);
	pr_info("%s: %s: MPSD_TIMER_REG_DATA==>\n",	 tag, __func__);
	list_for_each_entry(t, &tl_head->list, list) {
		pr_info("%s: %s: VAL[%lld] COUNT[%d] MASKS[%llu %llu]\n",
			tag, __func__, t->value, t->node_count,
			t->app_field_mask, t->dev_field_mask);
	}
	spin_unlock(&mpsd_timer_list_lock);
	if (in_buf == 1) {
		if (!buf) {
			pr_err("%s: %s: Buf passed in NULL\n",	tag, __func__);
			return ret;
		}
		ret += snprintf(buf, BUFFER_LENGTH_MAX,
			"\nMPSD_TIMER_REG_DATA==>\n");
		spin_lock(&mpsd_timer_list_lock);
		list_for_each_entry(t, &tl_head->list, list) {
			ret += snprintf(buf + ret, BUFFER_LENGTH_MAX,
				"VAL[%lld] COUNT[%d] MASKS[%llu %llu]\n",
				t->value, t->node_count,
				t->app_field_mask, t->dev_field_mask);
		}
		spin_unlock(&mpsd_timer_list_lock);
	}

	return ret;
}
#endif
