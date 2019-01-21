/*
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/errno.h>
#include <linux/module.h>
#include <linux/devfreq.h>
#include <linux/math64.h>
#include <linux/pm_qos.h>
#include <linux/module.h>
#include <linux/slab.h>

#include <linux/time.h>
#include <linux/timer.h>
#include <linux/kthread.h>
#include <linux/pm_opp.h>

#include "governor.h"

static int devfreq_simple_interactive_notifier(struct notifier_block *nb, unsigned long val,
						void *v)
{
	struct devfreq_notifier_block *devfreq_nb;

	devfreq_nb = container_of(nb, struct devfreq_notifier_block, nb);

	mutex_lock(&devfreq_nb->df->lock);
	update_devfreq(devfreq_nb->df);
	mutex_unlock(&devfreq_nb->df->lock);

	return NOTIFY_OK;
}

#ifdef CONFIG_EXYNOS_WD_DVFS
#define NEXTBUF(x, b)	if (++(x) > &(b)[SIMPLE_LOAD_MAX - 1]) (x) = (b)
#define POSTBUF(x, b)	((x) = ((--(x) < (b)) ?				\
			&(b)[SIMPLE_LOAD_MAX - 1] : (x)))
static unsigned long update_load(struct devfreq_dev_status *stat,
				struct devfreq_simple_interactive_data *data)
{
	struct devfreq_simple_load *ptr;
	unsigned int targetload;
	unsigned int freq;
	int i;
	if (!stat->total_time)
		return stat->current_frequency;
	for (i = 0; i < data->ntarget_load - 1 &&
	     stat->current_frequency >= data->target_load[i + 1]; i += 2);
	targetload = data->target_load[i];

	/* if frequency is changed then reset the load */
	if (!stat->current_frequency ||
	    stat->current_frequency != data->prev_freq) {
		data->rear = data->front;
		data->front->delta = 0;
		data->total = 0;
		data->busy = 0;
		if (data->max_load >= targetload)
			data->max_load = targetload;
		else
			data->max_load = 0;

		data->max_spent = 0;
		data->min_load = targetload;
	}
	ptr = data->front;
	ptr->delta += stat->delta_time;
	data->max_spent += stat->delta_time;
	data->total += stat->total_time;
	data->busy += stat->busy_time;
	/* if too short time, then not counting */
	if (ptr->delta > data->min_sample_time * NSEC_PER_MSEC) {
		NEXTBUF(data->front, data->buffer);
		data->front->delta = 0;
		if (data->front == data->rear)
			NEXTBUF(data->rear, data->buffer);
		ptr->load = data->total ?
			data->busy * 100 / data->total : 0;
		data->busy = 0;
		data->total = 0;
		/* if ptr load is higher than pervious or too small load */
		if (data->max_load <= ptr->load) {
			data->min_load = ptr->load;
			data->max_spent = 0;
			data->max_load = ptr->load;
			goto out;
		} else if (ptr->load < data->min_load) {
			data->min_load = ptr->load;
			if (ptr->load < data->tolerance) {
				data->max_load = ptr->load;
				data->max_spent = 0;
				return 0;
			}
		}
	}
	/* a new max load */
	if (data->max_spent > data->hold_sample_time * NSEC_PER_MSEC) {
		unsigned long long spent = 0;
		/* if not valid data, then skip */
		if (data->front == ptr) {
			spent += ptr->delta;
			POSTBUF(ptr, data->buffer);
		}
		data->max_load = ptr->load;
		data->max_spent = spent;
		/* if there is downtrend, then reflect current load */
		if (ptr->load > data->min_load + data->tolerance) {
			data->min_load = ptr->load;
			spent += ptr->delta;
			POSTBUF(ptr, data->buffer);
			for (; spent < data->hold_sample_time *
			     NSEC_PER_MSEC && ptr != data->rear;
			     POSTBUF(ptr, data->buffer)) {
				if (data->max_load < ptr->load) {
					data->max_load = ptr->load;
					data->max_spent = spent;
				} else if (data->min_load > ptr->load) {
					data->min_load = ptr->load;
				}
				spent += ptr->delta;
			}
		} else {
			data->min_load = ptr->load;
		}
	}
out:
	/* a few measurement */
	if (data->max_load == targetload || data->total)
		freq = stat->current_frequency;
	else
		freq = data->max_load * stat->current_frequency / targetload;

	if (data->max_load > data->go_hispeed_load && data->hispeed_freq > freq)
		freq = data->hispeed_freq;

	return freq;
}
#endif

static int devfreq_simple_interactive_func(struct devfreq *df,
					unsigned long *freq)
{
#ifdef CONFIG_EXYNOS_WD_DVFS
	struct devfreq_dev_status *stat;
	int err;
#endif
	struct devfreq_simple_interactive_data *data = df->data;
	unsigned long pm_qos_min = 0;
	unsigned long pm_qos_max = INT_MAX;
	int delay_check = 0;
	int delay_time = 0;
	int i = 0;
	struct dev_pm_opp *limit_opp;

	if (!data) {
		pr_err("%s: failed to find governor data\n", __func__);
		return -ENODATA;
	}

	if (!df->disabled_pm_qos) {
		pm_qos_min = pm_qos_request(data->pm_qos_class);
		if (data->pm_qos_class_max) {
			pm_qos_max = pm_qos_request(data->pm_qos_class_max);
			rcu_read_lock();
			limit_opp = devfreq_recommended_opp(df->dev.parent, &pm_qos_max,
					DEVFREQ_FLAG_LEAST_UPPER_BOUND);
			rcu_read_unlock();
			if (IS_ERR(limit_opp)) {
				pr_err("%s: failed to limit by max frequency\n", __func__);
				return PTR_ERR(limit_opp);
			}
		}
	}

	*freq = pm_qos_min;

#ifdef CONFIG_EXYNOS_WD_DVFS
	stat = &df->last_status;

	if (df->profile->get_dev_status) {
		err = devfreq_update_stats(df);
		if (err)
			return err;
		*freq = max(*freq, update_load(stat, data));
	}
#endif
	if (!data->use_delay_time)
		goto out;

	if (data->prev_freq != df->previous_freq) {
		for (i = 0; i < data->ndelay_time - 1 &&
				*freq >= data->delay_time[i + 1]; i += 2)
			;

		/* unit of delay time should be 10msec */
		delay_check = data->delay_time[i] % DELAY_TIME_RANGE;
		delay_time = delay_check ?
				data->delay_time[i] - delay_check + DELAY_TIME_RANGE :
				data->delay_time[i];

		data->freq_timer.expires = data->changed_time +
			msecs_to_jiffies(delay_time);
	}

	if (pm_qos_max > df->previous_freq && *freq < df->previous_freq &&
			data->freq_timer.expires > jiffies) {
		*freq = df->previous_freq;
		if (!timer_pending(&data->freq_timer))
			/* timer is bound to cpu0 */
			add_timer_on(&data->freq_timer, BOUND_CPU_NUM);

		goto out;
	} else if (timer_pending(&data->freq_timer)) {
		del_timer_sync(&data->freq_timer);
	}

	data->changed_time = jiffies;

out:
	/*
	 * save current frequency and time
	 * to use when update_devfreq is called next
	 */
	data->prev_freq = df->previous_freq;
	*freq = min(pm_qos_max, *freq);
#ifdef CONFIG_EXYNOS_WD_DVFS
	if (df->profile->get_dev_status) {
		unsigned long expires = jiffies;
		mod_timer_pinned(&data->freq_timer, expires +
			msecs_to_jiffies(data->min_sample_time * 2));
		if (*freq > df->min_freq) {
			/* timer is bound to cpu0 */
			mod_timer_pinned(&data->freq_slack_timer, expires +
					 msecs_to_jiffies(data->hold_sample_time));
		} else if (timer_pending(&data->freq_slack_timer)) {
			del_timer(&data->freq_slack_timer);
		}
	}
#endif

	return 0;
}

static int devfreq_change_freq_task(void *data)
{
	struct devfreq *df = data;

	while (!kthread_should_stop()) {
		set_current_state(TASK_INTERRUPTIBLE);

		schedule();

		set_current_state(TASK_RUNNING);

		mutex_lock(&df->lock);
		update_devfreq(df);
		mutex_unlock(&df->lock);
	}

	return 0;
}

/*timer callback function send a signal */
static void simple_interactive_timer(unsigned long data)
{
	struct devfreq_simple_interactive_data *gov_data = (void *)data;

	wake_up_process(gov_data->change_freq_task);
}

#ifdef CONFIG_EXYNOS_WD_DVFS
static void simple_interactive_nop_timer(unsigned long data)
{
}
#endif

static int devfreq_simple_interactive_register_notifier(struct devfreq *df)
{
	int ret;
	struct devfreq_simple_interactive_data *data = df->data;

	if (!data)
		return -EINVAL;

	data->nb.df = df;
	data->nb.nb.notifier_call = devfreq_simple_interactive_notifier;

	ret = pm_qos_add_notifier(data->pm_qos_class, &data->nb.nb);
	if (ret < 0)
		goto err1;

	if (data->pm_qos_class_max) {
		data->nb_max.df = df;
		data->nb_max.nb.notifier_call = devfreq_simple_interactive_notifier;

		ret = pm_qos_add_notifier(data->pm_qos_class_max, &data->nb_max.nb);
		if (ret < 0) {
			pm_qos_remove_notifier(data->pm_qos_class, &data->nb.nb);
			goto err2;
		}
	}

	/* timer of governor for delay time initialize */
	data->freq_timer.data = (unsigned long)data;
	data->freq_timer.function = simple_interactive_timer;
#ifdef CONFIG_EXYNOS_WD_DVFS
	init_timer_deferrable(&data->freq_timer);
	data->freq_slack_timer.function = simple_interactive_nop_timer;
	init_timer(&data->freq_slack_timer);
#else
	init_timer(&data->freq_timer);
#endif

	data->change_freq_task = kthread_create(devfreq_change_freq_task, df, "simpleinteractive");

	if (IS_ERR(data->change_freq_task)) {
		pr_err("%s: failed kthread_create for simpleinteractive governor\n", __func__);
		ret = PTR_ERR(data->change_freq_task);

		destroy_timer_on_stack(&data->freq_timer);
#ifdef CONFIG_EXYNOS_WD_DVFS
		destroy_timer_on_stack(&data->freq_slack_timer);
#endif
		pm_qos_remove_notifier(data->pm_qos_class, &data->nb.nb);
		if (data->pm_qos_class_max)
			pm_qos_remove_notifier(data->pm_qos_class_max, &data->nb_max.nb);

		goto err2;
	}

#ifdef CONFIG_EXYNOS_WD_DVFS
	if (df->profile->get_dev_status) {
		data->freq_timer.expires = jiffies +
			msecs_to_jiffies(data->min_sample_time * 2);
		add_timer(&data->freq_timer);
		data->freq_slack_timer.expires = jiffies +
			msecs_to_jiffies(data->hold_sample_time);
		add_timer_on(&data->freq_slack_timer, BOUND_CPU_NUM);
	}
#else
	kthread_bind(data->change_freq_task, BOUND_CPU_NUM);
#endif

	return 0;

err2:
err1:
	return ret;
}

static int devfreq_simple_interactive_unregister_notifier(struct devfreq *df)
{
	int ret;
	struct devfreq_simple_interactive_data *data = df->data;

	if (!data)
		return -EINVAL;

	if (data->pm_qos_class_max) {
		ret = pm_qos_remove_notifier(data->pm_qos_class_max, &data->nb_max.nb);
		if (ret < 0)
			goto err;
	}

	ret = pm_qos_remove_notifier(data->pm_qos_class, &data->nb.nb);

#ifdef CONFIG_EXYNOS_WD_DVFS
	destroy_timer_on_stack(&data->freq_slack_timer);
#endif
	destroy_timer_on_stack(&data->freq_timer);
	kthread_stop(data->change_freq_task);

err:
	return ret;
}

static int devfreq_simple_interactive_handler(struct devfreq *devfreq,
				unsigned int event, void *data)
{
	int ret;

	switch (event) {
	case DEVFREQ_GOV_START:
		ret = devfreq_simple_interactive_register_notifier(devfreq);
		if (ret)
			return ret;
		break;

	case DEVFREQ_GOV_STOP:
		ret = devfreq_simple_interactive_unregister_notifier(devfreq);
		if (ret)
			return ret;
		break;

	default:
		break;
	}

	return 0;
}

static struct devfreq_governor devfreq_simple_interactive = {
	.name = "interactive",
	.get_target_freq = devfreq_simple_interactive_func,
	.event_handler = devfreq_simple_interactive_handler,
};

static int __init devfreq_simple_interactive_init(void)
{
	return devfreq_add_governor(&devfreq_simple_interactive);
}
subsys_initcall(devfreq_simple_interactive_init);

static void __exit devfreq_simple_interactive_exit(void)
{
	int ret;

	ret = devfreq_remove_governor(&devfreq_simple_interactive);
	if (ret)
		pr_err("%s: failed remove governor %d\n", __func__, ret);

	return;
}
module_exit(devfreq_simple_interactive_exit);
MODULE_LICENSE("GPL");
