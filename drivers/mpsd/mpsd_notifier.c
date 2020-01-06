/*
 * Source file for mpsd driver event notifications.
 *
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *	   Ravi Solanki <ravi.siso@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/notifier.h>
#include <linux/cpufreq.h>

#include "mpsd_param.h"

static atomic_t notifier_init_done = ATOMIC_INIT(0);

static unsigned long long notifier_app_field_mask;
static unsigned long long notifier_dev_field_mask;

static unsigned long long app_params_supported;
static unsigned long long dev_params_supported;

static int notifier_pid = DEFAULT_PID;

static struct mutex mpsd_notifier_lock;

static struct param_info_notifier app_params_info_arr[TOTAL_APP_PARAMS];
static struct param_info_notifier dev_params_info_arr[TOTAL_DEV_PARAMS];

static struct raw_notifier_head app_params_notifier_list[TOTAL_APP_PARAMS];
static struct raw_notifier_head dev_params_notifier_list[TOTAL_DEV_PARAMS];

struct notifier_block freq_noti;
static unsigned long cpu_freq[2];

static inline int get_notifier_pid(void)
{
	return notifier_pid;
}

void mpsd_set_notifier_pid(int pid)
{
	if (pid <= 0)
		pid = DEFAULT_PID;

	notifier_pid = pid;
}

bool get_notifier_init_status(void)
{
	return (bool)atomic_read(&notifier_init_done);
}

static inline void set_notifier_init_status(int val)
{
	atomic_set(&notifier_init_done, val);
}

#ifdef CONFIG_MPSD_DEBUG
unsigned long long get_notifier_app_param_mask(void)
{
	unsigned long long ret = notifier_app_field_mask;
	return ret;
}

unsigned long long get_notifier_dev_param_mask(void)
{
	unsigned long long ret = notifier_dev_field_mask;
	return ret;
}

struct param_info_notifier *get_notifier_app_info_arr(void)
{
	if (get_debug_level_notifier() <=  get_mpsd_debug_level())
		return app_params_info_arr;

	return NULL;
}

struct param_info_notifier *get_notifier_dev_info_arr(void)
{
	if (get_debug_level_notifier() <=  get_mpsd_debug_level())
		return dev_params_info_arr;

	return NULL;
}
#endif

/**
 * mpsd_event_notification - Callback function for notfier block.
 *	   It will be called when some type of event occures.
 * @nb: Notifier block containing the callback which will be called
 *	   in case a event occures.
 * @val: Type of event.
 * @data: Contains event information and read data of param.
 *
 * This is a callback function which sends the event notification and
 * data based the type of event which has occured to mpsd driver.
 */
static int mpsd_event_notification(struct notifier_block *nb,
	unsigned long event_type, void *data)
{
	struct param_data_notifier *pdata = (struct param_data_notifier *)data;

	if (unlikely(!nb)) {
		pr_err("%s: %s: notifier_block is null\n", tag, __func__);
		return -EINVAL;
	}

	if (unlikely(!data)) {
		pr_err("%s: %s: param_info_notifier passed is null\n", tag,
			__func__);
		return -EINVAL;
	}

	if (!get_notifier_init_status()) {
		pr_err("%s: %s: Init not done!\n", tag, __func__);
		return -EPERM;
	}

	notifier_app_field_mask = 0;
	notifier_dev_field_mask = 0;

	if (is_valid_app_param(pdata->param))
		set_param_bit(&notifier_app_field_mask, pdata->param);
	else if (is_valid_dev_param(pdata->param))
		set_param_bit(&notifier_dev_field_mask, pdata->param);


#ifdef CONFIG_MPSD_DEBUG
	if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN) {
		pr_info("%s: %s: ID[%d] Event=%d Mask[%llu, %llu]\n",
			tag, __func__, pdata->param, pdata->events,
			notifier_app_field_mask, notifier_dev_field_mask);
		if (get_debug_level_notifier() <= get_mpsd_debug_level())
			print_notifier_events_data(pdata);

	}
#endif

	mpsd_populate_mmap(notifier_app_field_mask,
		notifier_dev_field_mask, get_notifier_pid(),
		NOTIFICATION_UPDATE, pdata->events);

	return 0;
}

/* Registering the callback function with the notifier block */
static struct notifier_block mpsd_param_nb = {
	.notifier_call = mpsd_event_notification,
};

/**
 * mpsd_event_notify - It will send the event and param data
 *	   based on the type of event to mpsd driver.
 * @param: param on which event has occured.
 * @prev: Previous value of param.
 * @cur: current value of param.
 *
 * This function will be called from modules where the param value is
 * getting changed and then sends the event notification
 * based the type of event which has occured to mpsd driver.
 */

void mpsd_event_notify(int param, unsigned long prev, unsigned long cur)
{
	long long delta = 0;
	struct param_data_notifier data;

	if (!get_notifier_init_status()) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_info("%s: %s: Init not done!\n", tag, __func__);
#endif
		return;
	}

	data.param = param;
	data.events = DEFAULT_EVENTS;
	data.prev = prev;
	data.cur = cur;
	delta = abs(cur - prev);

#ifdef CONFIG_MPSD_DEBUG
	if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN) {
		pr_info("%s: %s: %d: prev=%lu cur=%lu delta=%lld\n", tag,
			__func__, param, prev, cur, delta);
	}
#endif

	if (is_valid_app_param(param)) {
		if ((get_event_bit(app_params_info_arr[param -
			APP_PARAM_MIN].events, EVENT_VALUE_CHANGED)) &&
			(cur != prev) && (delta >= app_params_info_arr[
			param - APP_PARAM_MIN].delta)) {
			set_event_bit(&data.events, EVENT_VALUE_CHANGED);

			raw_notifier_call_chain(&app_params_notifier_list[
				param - APP_PARAM_MIN],
				EVENT_VALUE_CHANGED, &data);

#ifdef CONFIG_MPSD_DEBUG
			if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN) {
				pr_info("%s: %s: APP: E[%d]=> CHANGED\n",
					tag, __func__, data.events);
			}
#endif
		}

		if ((get_event_bit(app_params_info_arr[param -
			APP_PARAM_MIN].events, EVENT_REACHED_MAX_THRESHOLD)) &&
			(cur != prev) && (cur >= app_params_info_arr[
			param - APP_PARAM_MIN].max_threshold)) {
			set_event_bit(&data.events,
				EVENT_REACHED_MAX_THRESHOLD);

			raw_notifier_call_chain(&app_params_notifier_list[
				param - APP_PARAM_MIN],
				EVENT_REACHED_MAX_THRESHOLD, &data);

#ifdef CONFIG_MPSD_DEBUG
			if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN) {
				pr_info("%s: %s: APP: E[%d]=> MAX\n",
					tag, __func__, data.events);
			}
#endif
		}

		if ((get_event_bit(app_params_info_arr[param -
			APP_PARAM_MIN].events, EVENT_REACHED_MIN_THRESHOLD)) &&
			(cur != prev) && (cur <= app_params_info_arr[
			param - APP_PARAM_MIN].min_threshold)) {
			set_event_bit(&data.events,
				EVENT_REACHED_MIN_THRESHOLD);

			raw_notifier_call_chain(&app_params_notifier_list[
				param - APP_PARAM_MIN],
				EVENT_REACHED_MIN_THRESHOLD, &data);

#ifdef CONFIG_MPSD_DEBUG
			if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN) {
				pr_info("%s: %s: APP: E[%d]=> MIN\n",
					tag, __func__, data.events);
			}
#endif
		}
	}  else if (is_valid_dev_param(param)) {
		if ((get_event_bit(dev_params_info_arr[param -
			DEV_PARAM_MIN].events, EVENT_VALUE_CHANGED)) &&
			(cur != prev) && (delta >= dev_params_info_arr
			[param - DEV_PARAM_MIN].delta)) {
			set_event_bit(&data.events, EVENT_VALUE_CHANGED);

			raw_notifier_call_chain(&dev_params_notifier_list
				[param - DEV_PARAM_MIN],
				EVENT_VALUE_CHANGED, &data);

#ifdef CONFIG_MPSD_DEBUG
			if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN) {
				pr_info("%s: %s: DEV: E[%d]=> CHANGED\n",
					tag, __func__, data.events);
			}
#endif
		}

		if ((get_event_bit(dev_params_info_arr[param -
			DEV_PARAM_MIN].events, EVENT_REACHED_MAX_THRESHOLD)) &&
			(cur != prev) && (cur >= dev_params_info_arr
			[param - DEV_PARAM_MIN].max_threshold)) {
			set_event_bit(&data.events,
				EVENT_REACHED_MAX_THRESHOLD);

			raw_notifier_call_chain(&dev_params_notifier_list
				[param - DEV_PARAM_MIN],
				EVENT_REACHED_MAX_THRESHOLD, &data);

#ifdef CONFIG_MPSD_DEBUG
			if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN) {
				pr_info("%s: %s: DEV: E[%d]=> MAX\n",
					tag, __func__, data.events);
			}
#endif
		}

		if ((get_event_bit(dev_params_info_arr[param -
			DEV_PARAM_MIN].events, EVENT_REACHED_MIN_THRESHOLD)) &&
			(cur != prev) && (cur <= dev_params_info_arr
			[param - DEV_PARAM_MIN].min_threshold)) {
			set_event_bit(&data.events,
				EVENT_REACHED_MIN_THRESHOLD);

			raw_notifier_call_chain(&dev_params_notifier_list
				[param - DEV_PARAM_MIN],
				EVENT_REACHED_MIN_THRESHOLD, &data);

#ifdef CONFIG_MPSD_DEBUG
			if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN) {
				pr_info("%s: %s: DEV: E[%d]=> MIN\n",
					tag, __func__, data.events);
			}
#endif
		}
	} else {
		pr_err("%s: %s: Parameter(%d) is not valid\n", tag, __func__,
			param);
	}

}

/**
 * mpsd_register_notifier - Register a param for events.
 * @nb: Notifier block containing the callback which will be called
 *	   in case a event occures.
 * @pi: Structure containing the info like param which needs to be registered,
 *	   what event to be registered and on what limits to set for the param.
 *
 * This function registers a param for certain event types so when those
 * event occures a notoification will be sent to mpsd driver.
 */
static unsigned int mpsd_register_notifier(struct notifier_block *nb,
	struct param_info_notifier *pi)
{
	unsigned int ret = 0, param = 0;

	if (unlikely(!nb)) {
		pr_err("%s: %s: notifier_block passed is null\n", tag,
			__func__);
		return -EINVAL;
	}

	if (unlikely(!pi)) {
		pr_err("%s: %s: param_info_notifier passed is null\n", tag,
			__func__);
		return -EINVAL;
	}

	if (!get_notifier_init_status()) {
		pr_err("%s: %s: Init not done!\n", tag, __func__);
		return -EPERM;
	}

#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN) {
			pr_info("%s: %s: Req: Reg => ID=%d E[%d] (%lld %lld %lld)\n",
				tag, __func__, pi->param, pi->events, pi->delta,
				pi->max_threshold, pi->min_threshold);
		}
#endif

	mutex_lock(&mpsd_notifier_lock);
	param = pi->param;
	if (is_valid_app_param(param)) {
		if (!get_param_bit(app_params_supported, param)) {
			pr_err("%s: %s: Param not supported!\n", tag,
				__func__);
			ret = -EPERM;
			goto ret_out;
		}

		app_params_info_arr[param - APP_PARAM_MIN].events = pi->events;
		if (get_event_bit(pi->events, EVENT_VALUE_CHANGED)) {
			app_params_info_arr[param - APP_PARAM_MIN]
				.delta = pi->delta;
		}
		if (get_event_bit(pi->events,
			EVENT_REACHED_MAX_THRESHOLD)) {
			app_params_info_arr[param - APP_PARAM_MIN]
				.max_threshold = pi->max_threshold;
		}
		if (get_event_bit(pi->events,
			EVENT_REACHED_MIN_THRESHOLD)) {
			app_params_info_arr[param - APP_PARAM_MIN]
				.min_threshold = pi->min_threshold;
		}

#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN) {
			pr_info("%s: %s: Reg => ID=%d E[%d] (%lld %lld %lld) Done!\n",
				tag, __func__,
				app_params_info_arr[param - APP_PARAM_MIN]
					.param,
				app_params_info_arr[param - APP_PARAM_MIN]
					.events,
				app_params_info_arr[param - APP_PARAM_MIN]
					.delta,
				app_params_info_arr[param - APP_PARAM_MIN]
					.max_threshold,
				app_params_info_arr[param - APP_PARAM_MIN]
					.min_threshold);
		}
#endif

		ret = raw_notifier_chain_register(
			&app_params_notifier_list[param - APP_PARAM_MIN], nb);
	} else if (is_valid_dev_param(param)) {
		if (!get_param_bit(dev_params_supported, param)) {
			pr_err("%s: %s: Param not supported!\n", tag,
				__func__);
			ret = -EPERM;
			goto ret_out;
		}

		dev_params_info_arr[param - DEV_PARAM_MIN].events = pi->events;
		if (get_event_bit(pi->events, EVENT_VALUE_CHANGED)) {
			dev_params_info_arr[param - DEV_PARAM_MIN]
				.delta = pi->delta;
		}
		if (get_event_bit(pi->events,
			EVENT_REACHED_MAX_THRESHOLD)) {
			dev_params_info_arr[param - DEV_PARAM_MIN]
				.max_threshold = pi->max_threshold;
		}
		if (get_event_bit(pi->events,
			EVENT_REACHED_MIN_THRESHOLD)) {
			dev_params_info_arr[param - DEV_PARAM_MIN]
				.min_threshold = pi->min_threshold;
		}

#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN) {
			pr_info("%s: %s: Reg => ID=%d E[%d] (%lld %lld %lld) Done!\n",
				tag, __func__,
				dev_params_info_arr[param - DEV_PARAM_MIN]
					.param,
				dev_params_info_arr[param - DEV_PARAM_MIN]
					.events,
				dev_params_info_arr[param - DEV_PARAM_MIN]
					.delta,
				dev_params_info_arr[param - DEV_PARAM_MIN]
					.max_threshold,
				dev_params_info_arr[param - DEV_PARAM_MIN]
					.min_threshold);
		}
#endif

		ret = raw_notifier_chain_register(
			&dev_params_notifier_list[param - DEV_PARAM_MIN], nb);
	} else {
		pr_err("%s: %s: Parameter(%d) is not valid\n", tag, __func__,
			param);
		ret = -EINVAL;
		goto ret_out;
	}

	pr_info("%s: %s: Reg for Param(%d): success\n", tag, __func__,
		param);

#ifdef CONFIG_MPSD_DEBUG
	if (get_debug_level_notifier() <= get_mpsd_debug_level())
		print_notifier_registration_data();
#endif

ret_out:
	mutex_unlock(&mpsd_notifier_lock);
	return ret;
}

/**
 * mpsd_unregister_notifier - Unregister the param from events.
 * @nb: Notifier block containing the callback which will be called
 *	   in case a event occures.
 * @pi: Structure containing the info like param which needs to be
 *	   unregistered, on what event to be unregistered.
 *
 * This function unregister a param for certain event types.
 */
static unsigned int mpsd_unregister_notifier(struct notifier_block *nb,
	struct param_info_notifier *pi)
{
	unsigned int ret = 0, i = 0;
	unsigned int param = 0;
	unsigned int is_any_event_reg = 0;

	if (unlikely(!nb)) {
		pr_err("%s: %s: notifier_block passed is null\n", tag,
			__func__);
		return -EINVAL;
	}

	if (unlikely(!pi)) {
		pr_err("%s: %s: param_info_notifier passed is null\n", tag,
			__func__);
		return -EINVAL;
	}

	if (!get_notifier_init_status()) {
		pr_err("%s: %s: Init not done!\n", tag, __func__);
		return -EPERM;
	}

	param = pi->param;
	if (is_valid_param(param)) {
		pr_err("%s: %s: Parameter(%d) is not valid\n", tag, __func__,
			param);
		ret = -EINVAL;
	}

#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN) {
			pr_info("%s: %s: Req: Unreg => ID=%d E[%d] (%lld %lld %lld)\n",
				tag, __func__,
				pi->param, pi->events, pi->delta,
				pi->max_threshold, pi->min_threshold);
		}
#endif

	mutex_lock(&mpsd_notifier_lock);
	for (i = EVENT_TYPE_MIN; i < EVENT_TYPE_MAX; i++) {
		if (get_event_bit(pi->events, i)) {
			if (is_valid_app_param(param)) {
				clear_event_bit(&app_params_info_arr[param -
					APP_PARAM_MIN].events, i);

				if (i == EVENT_VALUE_CHANGED) {
					app_params_info_arr
						[param - APP_PARAM_MIN]
						.delta = -1;
				}

				if (i == EVENT_REACHED_MAX_THRESHOLD) {
					app_params_info_arr
						[param - APP_PARAM_MIN]
						.max_threshold = -1;
				}

				if (i == EVENT_REACHED_MIN_THRESHOLD) {
					app_params_info_arr
						[param - APP_PARAM_MIN]
						.min_threshold = -1;
				}
			} else if (is_valid_dev_param(param)) {
				clear_event_bit(&dev_params_info_arr[param -
					DEV_PARAM_MIN].events, i);

				if (i == EVENT_VALUE_CHANGED) {
					dev_params_info_arr
						[param - DEV_PARAM_MIN]
						.delta = -1;
				}

				if (i == EVENT_REACHED_MAX_THRESHOLD) {
					dev_params_info_arr[
						param - DEV_PARAM_MIN]
						.max_threshold = -1;
				}

				if (i == EVENT_REACHED_MIN_THRESHOLD) {
					dev_params_info_arr[
						param - DEV_PARAM_MIN]
						.min_threshold = -1;
				}
			}
		} else {
			is_any_event_reg = 1;
		}

		if (!is_any_event_reg) {
			if (is_valid_app_param(param)) {
				ret = raw_notifier_chain_unregister(
					&app_params_notifier_list[param -
					APP_PARAM_MIN], nb);
			} else if (is_valid_dev_param(param)) {
				ret = raw_notifier_chain_unregister(
					&dev_params_notifier_list[param -
					DEV_PARAM_MIN], nb);
			}
		}
	}
	mutex_unlock(&mpsd_notifier_lock);

	pr_info("%s: %s: Unreg for Param(%d): success\n", tag, __func__,
		param);

#ifdef CONFIG_MPSD_DEBUG
	if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN) {
		if (is_valid_app_param(param)) {
			pr_info("%s: %s: Reg => ID=%d E[%d] (%lld %lld %lld) Done!\n",
				tag, __func__,
				app_params_info_arr[param - APP_PARAM_MIN]
					.param,
				app_params_info_arr[param - APP_PARAM_MIN]
					.events,
				app_params_info_arr[param - APP_PARAM_MIN]
					.delta,
				app_params_info_arr[param - APP_PARAM_MIN]
					.max_threshold,
				app_params_info_arr[param - APP_PARAM_MIN]
					.min_threshold);
		} else if (is_valid_dev_param(param)) {
			pr_info("%s: %s: Reg => ID=%d E[%d] (%lld %lld %lld) Done!\n",
				tag, __func__,
				dev_params_info_arr[param - DEV_PARAM_MIN]
					.param,
				dev_params_info_arr[param - DEV_PARAM_MIN]
					.events,
				dev_params_info_arr[param - DEV_PARAM_MIN]
					.delta,
				dev_params_info_arr[param - DEV_PARAM_MIN]
					.max_threshold,
				dev_params_info_arr[param - DEV_PARAM_MIN]
					.min_threshold);
		}
	}

	if (get_debug_level_notifier() <=  get_mpsd_debug_level())
		print_notifier_registration_data();
#endif

	return ret;
}

/**
 * mpsd_register_for_event_notify - Register a param for events.
 * @param: Structure containing the info like param which needs to be
 *	   registered, on what event to be registered and
 *	   on what limits to set for the param.
 *
 * This function register a param for certain event types so when those
 * event occures a notoification will be sent to mpsd driver.
 */
void mpsd_register_for_event_notify(struct param_info_notifier *param)
{
	mpsd_register_notifier(&mpsd_param_nb, param);
}

/**
 * mpsd_unregister_for_event_notify - Unregister the param from events.
 * @param: Structure containing the info like param which needs
 *	   to be unregistered, on what event to be unregistered.
 *
 * This function unregister a param for certain event types.
 */
void mpsd_unregister_for_event_notify(struct param_info_notifier *param)
{
	mpsd_unregister_notifier(&mpsd_param_nb, param);
}

/**
 * mpsd_init_supported_params - Initialize bit field for the supported params.

 * This function initialize the bit field for supported params. Only these
 * params will be allowed to be registered for notification framework.
 */
static void mpsd_notifer_fill_supported_params(void)
{
	app_params_supported = 0;
	dev_params_supported = 0;

	set_param_bit(&dev_params_supported, DEV_PARAM_CPU_MAX_LIMIT);
	set_param_bit(&dev_params_supported, DEV_PARAM_CPU_MIN_LIMIT);
}

static int mpsd_cpufreq_notifier(struct notifier_block *nb, unsigned long val,
	void *data)
{
	struct cpufreq_policy *policy = data;

	switch (val) {
	case CPUFREQ_NOTIFY:
	mpsd_event_notify(DEV_PARAM_CPU_MAX_LIMIT, cpu_freq[0], policy->max);
	mpsd_event_notify(DEV_PARAM_CPU_MIN_LIMIT, cpu_freq[1], policy->min);
	cpu_freq[0] = policy->max;
	cpu_freq[1] = policy->min;
	break;
	}
	return 0;
}

/**
 * mpsd_event_notifier_init - Initialize the notification framework.
 *
 * This function initialize the notification framework.
 */
void mpsd_notifier_init(void)
{
	int i = 0;

	mutex_init(&mpsd_notifier_lock);

	pr_info("%s: %s:\n", tag, __func__);
	mutex_lock(&mpsd_notifier_lock);
	if (get_notifier_init_status()) {
		pr_info("%s: %s: init already Done!\n", tag, __func__);
		mutex_unlock(&mpsd_notifier_lock);
		return;
	}

	for (i =  0; i < (APP_PARAM_MAX - APP_PARAM_MIN); i++) {
		app_params_info_arr[i].param = i + APP_PARAM_MIN;
		app_params_info_arr[i].events = 0;
		app_params_info_arr[i].delta = -1;
		app_params_info_arr[i].max_threshold = -1;
		app_params_info_arr[i].min_threshold = -1;

		RAW_INIT_NOTIFIER_HEAD(&app_params_notifier_list[i]);
	}

	for (i = 0; i < (DEV_PARAM_MAX - DEV_PARAM_MIN); i++) {
		dev_params_info_arr[i].param = i + DEV_PARAM_MIN;
		dev_params_info_arr[i].events = 0;
		dev_params_info_arr[i].delta = -1;
		dev_params_info_arr[i].max_threshold = -1;
		dev_params_info_arr[i].min_threshold = -1;

		RAW_INIT_NOTIFIER_HEAD(&dev_params_notifier_list[i]);
	}

	mpsd_notifer_fill_supported_params();
	set_notifier_init_status(1);
	mutex_unlock(&mpsd_notifier_lock);

	freq_noti.notifier_call = mpsd_cpufreq_notifier;
	cpufreq_register_notifier(&freq_noti, CPUFREQ_POLICY_NOTIFIER);

	pr_info("%s: %s: Done!\n", tag, __func__);
#ifdef CONFIG_MPSD_DEBUG
	if (get_debug_level_notifier() <=  get_mpsd_debug_level())
		print_notifier_registration_data();
#endif
}

/**
 * mpsd_event_notifier_release - Unregister from all the notifications.
 *
 * This function unregister from all the notifications.
 * And need to be called once mpsd driver close gets called.
 */
void mpsd_notifier_release(void)
{
	int i = 0;

	pr_info("%s: %s:\n", tag, __func__);
	mutex_lock(&mpsd_notifier_lock);
	if (!get_notifier_init_status()) {
		pr_info("%s: %s: init not yet Done!\n", tag, __func__);
		mutex_unlock(&mpsd_notifier_lock);
		return;
	}

	for (i =  0; i < (APP_PARAM_MAX - APP_PARAM_MIN); i++) {
		app_params_info_arr[i].param = 0;
		app_params_info_arr[i].events = 0;
		app_params_info_arr[i].delta = -1;
		app_params_info_arr[i].max_threshold = -1;
		app_params_info_arr[i].min_threshold = -1;

		raw_notifier_chain_unregister(
			&app_params_notifier_list[i], &mpsd_param_nb);
	}

	for (i = 0; i < (DEV_PARAM_MAX - DEV_PARAM_MIN); i++) {
		dev_params_info_arr[i].param = 0;
		dev_params_info_arr[i].events = 0;
		dev_params_info_arr[i].delta = -1;
		dev_params_info_arr[i].max_threshold = -1;
		dev_params_info_arr[i].min_threshold = -1;

		raw_notifier_chain_unregister(
			&dev_params_notifier_list[i], &mpsd_param_nb);
	}

	app_params_supported = 0;
	dev_params_supported = 0;
	cpufreq_unregister_notifier(&freq_noti, CPUFREQ_POLICY_NOTIFIER);
	set_notifier_init_status(0);
	mutex_unlock(&mpsd_notifier_lock);

	mutex_destroy(&mpsd_notifier_lock);
	pr_info("%s: %s: Done!\n", tag, __func__);
#ifdef CONFIG_MPSD_DEBUG
	if (get_debug_level_notifier() <=  get_mpsd_debug_level())
		print_notifier_registration_data();
#endif
}
