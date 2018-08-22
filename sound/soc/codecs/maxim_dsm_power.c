/*
 * maxim_dsm_power.c -- Module for Rdc calibration
 *
 * Copyright 2015 Maxim Integrated Products
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/workqueue.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <sound/maxim_dsm.h>
#include <sound/maxim_dsm_cal.h>
#include <sound/maxim_dsm_power.h>

#define DEBUG_MAXIM_DSM_POWER
#ifdef DEBUG_MAXIM_DSM_POWER
#define dbg_maxdsm(format, args...)	\
pr_info("[MAXIM_DSM_POWER] %s: " format "\n", __func__, ## args)
#else
#define dbg_maxdsm(format, args...)
#endif /* DEBUG_MAXIM_DSM_POWER */

#define MAXIM_DSM_POWER_MEASUREMENT

struct maxim_dsm_power *g_mdp;

static int maxdsm_power_check(
		struct maxim_dsm_power *mdp, int action, int delay)
{
	int ret = 0;

	if (delayed_work_pending(&mdp->work))
		cancel_delayed_work(&mdp->work);

	if (action) {
		mdp->info.remaining = mdp->info.duration;
		mdp->values.count = mdp->values.avg = mdp->values.avg_r = 0;
		mdp->info.previous_jiffies = jiffies;
		queue_delayed_work(mdp->wq,
				&mdp->work,
				msecs_to_jiffies(delay));
	}

	return ret;
}

void maxdsm_power_control(int state)
{
	maxdsm_power_check(g_mdp, state, 0);
}
EXPORT_SYMBOL_GPL(maxdsm_power_control);

static void maxdsm_power_work(struct work_struct *work)
{
	struct maxim_dsm_power *mdp;
	unsigned int power = 0, power_r = 0, stereo = 0;
	unsigned long diff;

	mdp = container_of(work, struct maxim_dsm_power, work.work);

	mutex_lock(&mdp->mutex);

#ifdef CONFIG_SND_SOC_MAXIM_DSM
	stereo = maxdsm_is_stereo();
#else
	stereo = 0;
#endif

#ifdef CONFIG_SND_SOC_MAXIM_DSM
	power = maxdsm_get_power_measurement();
	if (stereo)
		power_r = maxdsm_get_power_measurement_r();
#endif /* CONFIG_SND_SOC_MAXIM_DSM */

	if (power) {
		mdp->values.avg += power;
		if (power_r)
			mdp->values.avg_r += power_r;
		mdp->values.count++;
	}

	diff = jiffies - mdp->info.previous_jiffies;
	mdp->info.remaining -= jiffies_to_msecs(diff);

	dbg_maxdsm("power=0x%08x remaining=%d duration=%d",
			power,
			mdp->info.remaining,
			mdp->info.duration);

	if (mdp->info.remaining > 0
			&& mdp->values.status) {
		mdp->info.previous_jiffies = jiffies;
		queue_delayed_work(mdp->wq,
				&mdp->work,
				msecs_to_jiffies(mdp->info.interval));
	} else {
		mdp->values.count > 0 ?
			do_div(mdp->values.avg, mdp->values.count) : 0;
		if (stereo)
			mdp->values.count > 0 ?
				do_div(mdp->values.avg_r, mdp->values.count) : 0;
		mdp->values.power = mdp->values.avg;
		mdp->values.power_r = mdp->values.avg_r;
		if (stereo)
			dbg_maxdsm("power=0x%08x power_r=0x%08x", mdp->values.power, mdp->values.power_r);
		else
			dbg_maxdsm("power=0x%08x", mdp->values.power);
#ifdef MAXIM_DSM_POWER_MEASUREMENT
		if (g_mdp->values.status) {
			if (maxdsm_power_check(g_mdp, g_mdp->values.status,
					MAXDSM_POWER_START_DELAY * 10)) {
				pr_err("%s: The codec was connected?\n",
						__func__);
				mutex_lock(&g_mdp->mutex);
				g_mdp->values.status = 0;
				mutex_unlock(&g_mdp->mutex);
			}
		}
#else
		g_mdp->values.status = 0;
#endif
	}

	mutex_unlock(&mdp->mutex);
}

static int maxdsm_power_ppr_state(struct maxim_dsm_power *mdp, int ch)
{
	unsigned long diff;
	int state = g_mdp->values_ppr[ch].status;

	dbg_maxdsm("set state %d channel %d", state, ch);

	diff = jiffies - mdp->info_ppr.amp_uptime_jiffies;

	switch (g_mdp->values_ppr[ch].status) {
	case PPR_ON:
	case PPR_OFF:
	case PPR_NORMAL:
	case PPR_RUN:
		if (jiffies_to_msecs(diff) > mdp->info_ppr.target_min_time_ms) {
			if ((mdp->values_ppr[ch].env_temp + 500) < mdp->values_ppr[ch].spk_t &&
				mdp->values_ppr[ch].spk_t > mdp->values_ppr[ch].target_temp &&
				mdp->values_ppr[ch].spk_t > mdp->values_ppr[ch].exit_temp) {
					state = PPR_RUN;
					maxdsm_set_ppr_enable(1, mdp->values_ppr[ch].cutoff_frequency, mdp->values_ppr[ch].threshold_level, ch);
			} else if (mdp->values_ppr[ch].spk_t < mdp->values_ppr[ch].exit_temp) {
				maxdsm_set_ppr_enable(0, 0x1401, mdp->values_ppr[ch].threshold_level, ch);
				state = PPR_NORMAL;
			}
		}
	}
	return state;
}

static int maxdsm_power_ppr_check(struct maxim_dsm_power *mdp, int state, int deley)
{

	if (delayed_work_pending(&mdp->work_ppr))
		cancel_delayed_work(&mdp->work_ppr);

	dbg_maxdsm("set state %d", state);
	if (state) {
		mdp->info_ppr.previous_jiffies = jiffies;
		queue_delayed_work(mdp->wq,
				&mdp->work_ppr,
				deley);
	}

	return 0;
}

void maxdsm_power_ppr_control(int state)
{
	if (state == g_mdp->values_ppr[MAXDSM_LEFT].status && state == g_mdp->values_ppr[MAXDSM_RIGHT].status) {
		dbg_maxdsm("ppr ignored. state %d Left state %d Right state %d", state, g_mdp->values_ppr[MAXDSM_LEFT].status,
				g_mdp->values_ppr[MAXDSM_RIGHT].status);
	} else if (g_mdp->info_ppr.global_enable != 0) {
		mutex_lock(&g_mdp->mutex);
		g_mdp->values_ppr[MAXDSM_LEFT].status = state;
		g_mdp->values_ppr[MAXDSM_RIGHT].status = state;
		g_mdp->info_ppr.amp_uptime_jiffies = jiffies;
		mutex_unlock(&g_mdp->mutex);
		if (maxdsm_power_ppr_check(g_mdp, state, 0)) {
			pr_err("%s: The codec was connected?\n",
			       __func__);
			mutex_lock(&g_mdp->mutex);
			g_mdp->values_ppr[MAXDSM_LEFT].status = PPR_OFF;
			g_mdp->values_ppr[MAXDSM_RIGHT].status = PPR_OFF;
			mutex_unlock(&g_mdp->mutex);
		}
	}
}
EXPORT_SYMBOL_GPL(maxdsm_power_ppr_control);

static void maxdsm_power_work_ppr(struct work_struct *work)
{
	struct maxim_dsm_power *mdp;
	int state, i;

	mdp = container_of(work, struct maxim_dsm_power, work_ppr.work);

	mutex_lock(&mdp->mutex);

	for (i = 0 ; i < MAXDSM_CHANNEL ; i++) {
		state = maxdsm_power_ppr_state(mdp, i);

		if (state != g_mdp->values_ppr[i].status)
			g_mdp->values_ppr[i].status = state;
		dbg_maxdsm("g_mdp->values_ppr[i].status %d", g_mdp->values_ppr[i].status);
	}
	if (g_mdp->values_ppr[MAXDSM_LEFT].status || g_mdp->values_ppr[MAXDSM_RIGHT].status) {
		if (maxdsm_power_ppr_check(g_mdp, 1, MAXDSM_POWER_START_DELAY * 10)) {
			pr_err("%s: The codec was connected?\n",
					__func__);
			g_mdp->values_ppr[MAXDSM_LEFT].status = 0;
			g_mdp->values_ppr[MAXDSM_RIGHT].status = 0;
		}
	}
	mutex_unlock(&mdp->mutex);
}

static ssize_t maxdsm_power_duration_show(struct device *dev,
					  struct device_attribute *attr,
					  char *buf)
{
	return sprintf(buf, "%d", g_mdp->info.duration);
}

static ssize_t maxdsm_power_duration_store(struct device *dev,
					   struct device_attribute *attr,
					   const char *buf, size_t size)
{
	if (kstrtou32(buf, 0, &g_mdp->info.duration))
		dev_err(dev, "%s: Failed converting from str to u32.\n",
			__func__);

	if (g_mdp->info.duration < 10000)
		g_mdp->info.duration = 10000;

	return size;
}
static DEVICE_ATTR(duration, 0664,
		   maxdsm_power_duration_show, maxdsm_power_duration_store);

static ssize_t maxdsm_power_value_show(struct device *dev,
				       struct device_attribute *attr,
				       char *buf)
{
	return sprintf(buf, "%x", g_mdp->values.power);
}
static DEVICE_ATTR(value, 0664, maxdsm_power_value_show, NULL);

static ssize_t maxdsm_power_r_value_show(struct device *dev,
				       struct device_attribute *attr,
				       char *buf)
{
	return sprintf(buf, "%x", g_mdp->values.power_r);
}
static DEVICE_ATTR(value_r, 0664, maxdsm_power_r_value_show, NULL);


static ssize_t maxdsm_power_interval_show(struct device *dev,
					  struct device_attribute *attr,
					  char *buf)
{
	return sprintf(buf, "%d", g_mdp->info.interval);
}

static ssize_t maxdsm_power_interval_store(struct device *dev,
					   struct device_attribute *attr,
					   const char *buf, size_t size)
{
	if (kstrtou32(buf, 0, &g_mdp->info.interval))
		dev_err(dev, "%s: Failed converting from str to u32.\n",
			__func__);
	return size;
}
static DEVICE_ATTR(interval, 0664,
		   maxdsm_power_interval_show, maxdsm_power_interval_store);

static ssize_t maxdsm_power_ppr_target_min_time_ms_show(struct device *dev,
					  struct device_attribute *attr,
					  char *buf)
{
	return sprintf(buf, "%d", g_mdp->info_ppr.target_min_time_ms);
}

static ssize_t maxdsm_power_ppr_target_min_time_ms_store(struct device *dev,
					   struct device_attribute *attr,
					   const char *buf, size_t size)
{
	if (kstrtou32(buf, 0, &g_mdp->info_ppr.target_min_time_ms))
		dev_err(dev, "%s: Failed converting from str to u32.\n",
			__func__);
	return size;
}
static DEVICE_ATTR(target_min_time_ms, 0664,
		   maxdsm_power_ppr_target_min_time_ms_show, maxdsm_power_ppr_target_min_time_ms_store);

static ssize_t maxdsm_power_ppr_target_temp_show(struct device *dev,
					  struct device_attribute *attr,
					  char *buf)
{
	return sprintf(buf, "%d", g_mdp->values_ppr[MAXDSM_LEFT].target_temp);
}

static ssize_t maxdsm_power_ppr_target_temp_store(struct device *dev,
					   struct device_attribute *attr,
					   const char *buf, size_t size)
{
	if (kstrtou32(buf, 0, &g_mdp->values_ppr[MAXDSM_LEFT].target_temp))
		dev_err(dev, "%s: Failed converting from str to u32.\n",
			__func__);
	return size;
}
static DEVICE_ATTR(target_temp, 0664,
		   maxdsm_power_ppr_target_temp_show, maxdsm_power_ppr_target_temp_store);

static ssize_t maxdsm_power_ppr_target_temp_r_show(struct device *dev,
					  struct device_attribute *attr,
					  char *buf)
{
	return sprintf(buf, "%d", g_mdp->values_ppr[MAXDSM_RIGHT].target_temp);
}

static ssize_t maxdsm_power_ppr_target_temp_r_store(struct device *dev,
					   struct device_attribute *attr,
					   const char *buf, size_t size)
{
	if (kstrtou32(buf, 0, &g_mdp->values_ppr[MAXDSM_RIGHT].target_temp))
		dev_err(dev, "%s: Failed converting from str to u32.\n",
			__func__);
	return size;
}
static DEVICE_ATTR(target_temp_r, 0664,
		   maxdsm_power_ppr_target_temp_r_show, maxdsm_power_ppr_target_temp_r_store);


static ssize_t maxdsm_power_ppr_cut_off_freq_show(struct device *dev,
					  struct device_attribute *attr,
					  char *buf)
{
	return sprintf(buf, "%d", g_mdp->values_ppr[MAXDSM_LEFT].cutoff_frequency);
}

static ssize_t maxdsm_power_ppr_cut_off_freq_store(struct device *dev,
					   struct device_attribute *attr,
					   const char *buf, size_t size)
{
	if (kstrtou32(buf, 0, &g_mdp->values_ppr[MAXDSM_LEFT].cutoff_frequency))
		dev_err(dev, "%s: Failed converting from str to u32.\n",
			__func__);
	maxdsm_set_ppr_xover_freq(g_mdp->values_ppr[MAXDSM_LEFT].cutoff_frequency, MAXDSM_LEFT);
	return size;
}
static DEVICE_ATTR(cutoff_frequency, 0664,
		   maxdsm_power_ppr_cut_off_freq_show, maxdsm_power_ppr_cut_off_freq_store);

static ssize_t maxdsm_power_ppr_cut_off_freq_r_show(struct device *dev,
					  struct device_attribute *attr,
					  char *buf)
{
	return sprintf(buf, "%d", g_mdp->values_ppr[MAXDSM_RIGHT].cutoff_frequency);
}

static ssize_t maxdsm_power_ppr_cut_off_freq_r_store(struct device *dev,
					   struct device_attribute *attr,
					   const char *buf, size_t size)
{
	if (kstrtou32(buf, 0, &g_mdp->values_ppr[MAXDSM_RIGHT].cutoff_frequency))
		dev_err(dev, "%s: Failed converting from str to u32.\n",
			__func__);
	maxdsm_set_ppr_xover_freq(g_mdp->values_ppr[MAXDSM_RIGHT].cutoff_frequency, MAXDSM_RIGHT);
	return size;
}
static DEVICE_ATTR(cutoff_frequency_r, 0664,
		   maxdsm_power_ppr_cut_off_freq_r_show, maxdsm_power_ppr_cut_off_freq_r_store);


static ssize_t maxdsm_power_ppr_threshold_level_show(struct device *dev,
					  struct device_attribute *attr,
					  char *buf)
{
	return sprintf(buf, "%d", g_mdp->values_ppr[MAXDSM_LEFT].threshold_level);
}

static ssize_t maxdsm_power_ppr_threshold_level_store(struct device *dev,
					   struct device_attribute *attr,
					   const char *buf, size_t size)
{
	if (kstrtou32(buf, 0, &g_mdp->values_ppr[MAXDSM_LEFT].threshold_level))
		dev_err(dev, "%s: Failed converting from str to u32.\n",
			__func__);
	maxdsm_set_ppr_threshold_db(g_mdp->values_ppr[MAXDSM_LEFT].threshold_level, MAXDSM_LEFT);
	return size;
}
static DEVICE_ATTR(threshold_level, 0664,
		   maxdsm_power_ppr_threshold_level_show, maxdsm_power_ppr_threshold_level_store);

static ssize_t maxdsm_power_ppr_threshold_level_r_show(struct device *dev,
					  struct device_attribute *attr,
					  char *buf)
{
	return sprintf(buf, "%d", g_mdp->values_ppr[MAXDSM_RIGHT].threshold_level);
}

static ssize_t maxdsm_power_ppr_threshold_level_r_store(struct device *dev,
					   struct device_attribute *attr,
					   const char *buf, size_t size)
{
	if (kstrtou32(buf, 0, &g_mdp->values_ppr[MAXDSM_RIGHT].threshold_level))
		dev_err(dev, "%s: Failed converting from str to u32.\n",
			__func__);
	maxdsm_set_ppr_threshold_db(g_mdp->values_ppr[MAXDSM_RIGHT].threshold_level, MAXDSM_RIGHT);
	return size;
}
static DEVICE_ATTR(threshold_level_r, 0664,
		   maxdsm_power_ppr_threshold_level_r_show, maxdsm_power_ppr_threshold_level_r_store);


static ssize_t maxdsm_power_ppr_env_temp_show(struct device *dev,
					  struct device_attribute *attr,
					  char *buf)
{
	return sprintf(buf, "%d", g_mdp->values_ppr[MAXDSM_LEFT].env_temp);
}

static ssize_t maxdsm_power_ppr_env_temp_store(struct device *dev,
					   struct device_attribute *attr,
					   const char *buf, size_t size)
{
	if (kstrtou32(buf, 0, &g_mdp->values_ppr[MAXDSM_LEFT].env_temp))
		dev_err(dev, "%s: Failed converting from str to u32.\n",
			__func__);
	return size;
}
static DEVICE_ATTR(env_temp, 0664,
		   maxdsm_power_ppr_env_temp_show, maxdsm_power_ppr_env_temp_store);

static ssize_t maxdsm_power_ppr_env_temp_r_show(struct device *dev,
					  struct device_attribute *attr,
					  char *buf)
{
	return sprintf(buf, "%d", g_mdp->values_ppr[MAXDSM_RIGHT].env_temp);
}

static ssize_t maxdsm_power_ppr_env_temp_r_store(struct device *dev,
					   struct device_attribute *attr,
					   const char *buf, size_t size)
{
	if (kstrtou32(buf, 0, &g_mdp->values_ppr[MAXDSM_RIGHT].env_temp))
		dev_err(dev, "%s: Failed converting from str to u32.\n",
			__func__);
	return size;
}
static DEVICE_ATTR(env_temp_r, 0664,
		   maxdsm_power_ppr_env_temp_r_show, maxdsm_power_ppr_env_temp_r_store);

static ssize_t maxdsm_power_ppr_spk_t_show(struct device *dev,
					  struct device_attribute *attr,
					  char *buf)
{
	return sprintf(buf, "%d", g_mdp->values_ppr[MAXDSM_LEFT].spk_t);
}

static ssize_t maxdsm_power_ppr_spk_t_store(struct device *dev,
					   struct device_attribute *attr,
					   const char *buf, size_t size)
{
	if (kstrtou32(buf, 0, &g_mdp->values_ppr[MAXDSM_LEFT].spk_t))
		dev_err(dev, "%s: Failed converting from str to u32.\n",
			__func__);
	return size;
}
static DEVICE_ATTR(spk_t, 0664,
		   maxdsm_power_ppr_spk_t_show, maxdsm_power_ppr_spk_t_store);

static ssize_t maxdsm_power_ppr_spk_t_r_show(struct device *dev,
					  struct device_attribute *attr,
					  char *buf)
{
	return sprintf(buf, "%d", g_mdp->values_ppr[MAXDSM_RIGHT].spk_t);
}

static ssize_t maxdsm_power_ppr_spk_t_r_store(struct device *dev,
					   struct device_attribute *attr,
					   const char *buf, size_t size)
{
	if (kstrtou32(buf, 0, &g_mdp->values_ppr[MAXDSM_RIGHT].spk_t))
		dev_err(dev, "%s: Failed converting from str to u32.\n",
			__func__);
	return size;
}
static DEVICE_ATTR(spk_t_r, 0664,
		   maxdsm_power_ppr_spk_t_r_show, maxdsm_power_ppr_spk_t_r_store);


static ssize_t maxdsm_power_ppr_exit_temp_show(struct device *dev,
					  struct device_attribute *attr,
					  char *buf)
{
	return sprintf(buf, "%d", g_mdp->values_ppr[MAXDSM_LEFT].exit_temp);
}

static ssize_t maxdsm_power_ppr_exit_temp_store(struct device *dev,
					   struct device_attribute *attr,
					   const char *buf, size_t size)
{
	if (kstrtou32(buf, 0, &g_mdp->values_ppr[MAXDSM_LEFT].exit_temp))
		dev_err(dev, "%s: Failed converting from str to u32.\n",
			__func__);
	return size;
}
static DEVICE_ATTR(exit_temp, 0664,
		   maxdsm_power_ppr_exit_temp_show, maxdsm_power_ppr_exit_temp_store);

static ssize_t maxdsm_power_ppr_exit_temp_r_show(struct device *dev,
					  struct device_attribute *attr,
					  char *buf)
{
	return sprintf(buf, "%d", g_mdp->values_ppr[MAXDSM_RIGHT].exit_temp);
}

static ssize_t maxdsm_power_ppr_exit_temp_r_store(struct device *dev,
					   struct device_attribute *attr,
					   const char *buf, size_t size)
{
	if (kstrtou32(buf, 0, &g_mdp->values_ppr[MAXDSM_RIGHT].exit_temp))
		dev_err(dev, "%s: Failed converting from str to u32.\n",
			__func__);
	return size;
}
static DEVICE_ATTR(exit_temp_r, 0664,
		   maxdsm_power_ppr_exit_temp_r_show, maxdsm_power_ppr_exit_temp_r_store);

static ssize_t maxdsm_power_ppr_global_enable_show(struct device *dev,
					  struct device_attribute *attr,
					  char *buf)
{
	return sprintf(buf, "%d", g_mdp->info_ppr.global_enable);
}

static ssize_t maxdsm_power_ppr_global_enable_store(struct device *dev,
					   struct device_attribute *attr,
					   const char *buf, size_t size)
{
	if (kstrtou32(buf, 0, &g_mdp->info_ppr.global_enable))
		dev_err(dev, "%s: Failed converting from str to u32.\n",
			__func__);
	return size;
}
static DEVICE_ATTR(global_enable, 0664,
		   maxdsm_power_ppr_global_enable_show, maxdsm_power_ppr_global_enable_store);


static ssize_t maxdsm_power_status_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return sprintf(buf, "%s\n",
		       g_mdp->values.status ? "Enabled" : "Disabled");
}

static ssize_t maxdsm_power_status_store(struct device *dev,
					 struct device_attribute *attr,
					 const char *buf, size_t size)
{
	int status = 0;

	if (!kstrtou32(buf, 0, &status)) {
		status = status > 0 ? 1 : 0;
		if (status == g_mdp->values.status) {
			dbg_maxdsm("Already run. It will be ignored.");
		} else {
			mutex_lock(&g_mdp->mutex);
			g_mdp->values.status = status;
			mutex_unlock(&g_mdp->mutex);
			if (maxdsm_power_check(g_mdp, status,
						MAXDSM_POWER_START_DELAY)) {
				pr_err("%s: The codec was connected?\n",
				       __func__);
				mutex_lock(&g_mdp->mutex);
				g_mdp->values.status = 0;
				mutex_unlock(&g_mdp->mutex);
			}
		}
	}

	return size;
}
static DEVICE_ATTR(status, 0664,
	maxdsm_power_status_show, maxdsm_power_status_store);

static struct attribute *maxdsm_power_attr[] = {
	&dev_attr_duration.attr,
	&dev_attr_value.attr,
	&dev_attr_value_r.attr,
	&dev_attr_interval.attr,
	&dev_attr_status.attr,
	&dev_attr_target_min_time_ms.attr,
	&dev_attr_target_temp.attr,
	&dev_attr_target_temp_r.attr,
	&dev_attr_exit_temp.attr,
	&dev_attr_exit_temp_r.attr,
	&dev_attr_cutoff_frequency.attr,
	&dev_attr_cutoff_frequency_r.attr,
	&dev_attr_threshold_level.attr,
	&dev_attr_threshold_level_r.attr,
	&dev_attr_env_temp.attr,
	&dev_attr_env_temp_r.attr,
	&dev_attr_spk_t.attr,
	&dev_attr_spk_t_r.attr,
	&dev_attr_global_enable.attr,
	NULL,
};

static struct attribute_group maxdsm_power_attr_grp = {
	.attrs = maxdsm_power_attr,
};

static int __init maxdsm_power_init(void)
{
	struct class *class = NULL;
	struct maxim_dsm_power *mdp;
	int i, ret = 0;

	g_mdp = kzalloc(sizeof(struct maxim_dsm_power), GFP_KERNEL);
	if (g_mdp == NULL)
		return -ENOMEM;
	mdp = g_mdp;

	mdp->wq = create_singlethread_workqueue(MAXDSM_POWER_WQ_NAME);
	if (mdp->wq == NULL) {
		kfree(g_mdp);
		return -ENOMEM;
	}

	INIT_DELAYED_WORK(&g_mdp->work, maxdsm_power_work);
	INIT_DELAYED_WORK(&g_mdp->work_ppr, maxdsm_power_work_ppr);
	mutex_init(&g_mdp->mutex);

	mdp->info.duration = 10000; /* 10 secs */
	mdp->info.remaining = mdp->info.duration;
	mdp->info.interval = 10000;  /* 10 secs */
	mdp->values.power = 0xFFFFFFFF;
	mdp->values.power_r = 0xFFFFFFFF;
	mdp->platform_type = 0xFFFFFFFF;

	mdp->info_ppr.target_min_time_ms = 300000;
	mdp->info_ppr.global_enable = 1;
	for (i = 0 ; i < MAXDSM_CHANNEL ; i++) {
		mdp->values_ppr[i].cutoff_frequency = 0x3E801;
		mdp->values_ppr[i].env_temp = 2500;
		mdp->values_ppr[i].target_temp = 3400;
		mdp->values_ppr[i].spk_t = 2500;
		mdp->values_ppr[i].threshold_level = 0xE20189;
		mdp->values_ppr[i].exit_temp = 3250;
	}

#ifdef CONFIG_SND_SOC_MAXIM_DSM_CAL
	class = maxdsm_cal_get_class();
#else
	if (!class)
		class = class_create(THIS_MODULE,
				MAXDSM_POWER_DSM_NAME);
#endif /* CONFIG_SND_SOC_MAXIM_DSM_CAL */
	mdp->class = class;
	if (mdp->class) {
		mdp->dev = device_create(mdp->class, NULL, 1, NULL,
				MAXDSM_POWER_CLASS_NAME);
		if (!IS_ERR(mdp->dev)) {
			if (sysfs_create_group(&mdp->dev->kobj,
						&maxdsm_power_attr_grp))
				dbg_maxdsm(
						"Failed to create sysfs group. ret=%d",
						ret);
		}
	}
	dbg_maxdsm("class=%p %p", class, mdp->class);

	dbg_maxdsm("Completed initialization");

	return ret;
}
module_init(maxdsm_power_init);

static void __exit maxdsm_power_exit(void)
{
	kfree(g_mdp);
};
module_exit(maxdsm_power_exit);

MODULE_DESCRIPTION("For power measurement of DSM");
MODULE_AUTHOR("Kyounghun Jeon<hun.jeon@maximintegrated.com");
MODULE_LICENSE("GPL");
