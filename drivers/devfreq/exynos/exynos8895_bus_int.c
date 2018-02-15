/* linux/drivers/devfreq/exynos/exynos8895_bus_int.c
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com
 *
 * Samsung EXYNOS8895 SoC INT devfreq driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, either version 2 of the License,
 * or (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/list.h>
#include <linux/clk.h>

#include <soc/samsung/exynos-devfreq.h>
#include <sound/samsung/abox.h>
#include <soc/samsung/cal-if.h>
#include "../governor.h"

#include "exynos_ppmu.h"

static unsigned long origin_suspend_freq = 0;

static int exynos8895_devfreq_int_cmu_dump(struct exynos_devfreq_data *data)
{
	cal_vclk_dbg_info(data->dfs_id);

	return 0;
}

static int exynos8895_devfreq_int_reboot(struct exynos_devfreq_data *data)
{
	data->max_freq = data->reboot_freq;
	data->devfreq->max_freq = data->max_freq;

	mutex_lock(&data->devfreq->lock);
	update_devfreq(data->devfreq);
	mutex_unlock(&data->devfreq->lock);

	return 0;
}

static int exynos8895_devfreq_int_get_freq(struct device *dev, u32 *cur_freq,
		struct clk *clk, struct exynos_devfreq_data *data)
{
	*cur_freq = (u32)cal_dfs_get_rate(data->dfs_id);
	if (*cur_freq == 0) {
		dev_err(dev, "failed get frequency from CAL\n");
		return -EINVAL;
	}

        return 0;
}

static int exynos8895_devfreq_int_set_freq(struct device *dev, u32 new_freq,
		struct clk *clk, struct exynos_devfreq_data *data)
{
	if (cal_dfs_set_rate(data->dfs_id, (unsigned long)new_freq)) {
		dev_err(dev, "failed set frequency to CAL (%uKhz)\n",
				new_freq);
		return -EINVAL;
	}

	return 0;
}

static int exynos8895_devfreq_int_pm_suspend_prepare(struct exynos_devfreq_data *data)
{
#ifdef CONFIG_SND_SOC_SAMSUNG_ABOX
	unsigned long abox_freq;
#endif
	if (!origin_suspend_freq)
		origin_suspend_freq = data->devfreq_profile.suspend_freq;
#ifdef CONFIG_SND_SOC_SAMSUNG_ABOX
	if (abox_is_on()) {
		abox_freq = (unsigned long)abox_get_requiring_int_freq_in_khz();
		if (abox_freq)
			data->devfreq_profile.suspend_freq = abox_freq;
		else
			data->devfreq_profile.suspend_freq = origin_suspend_freq;
	} else {
		data->devfreq_profile.suspend_freq = origin_suspend_freq;
	}
#endif
	return 0;
}

static int exynos8895_devfreq_int_resume(struct exynos_devfreq_data *data)
{
	u32 cur_freq;

	/* for sync from resume frequency */
	if (exynos8895_devfreq_int_get_freq(data->dev, &cur_freq, data->clk, data)) {
		dev_err(data->dev, "failed get frequency when resume\n");
		return -EINVAL;
	}

	dev_info(data->dev, "Resume frequency is %u\n", cur_freq);

	return 0;
}

static int exynos8895_devfreq_int_init_freq_table(struct exynos_devfreq_data *data)
{
	u32 max_freq, min_freq;
	unsigned long tmp_max, tmp_min;
	struct dev_pm_opp *target_opp;
	u32 flags = 0;
	int i;

	max_freq = (u32)cal_dfs_get_max_freq(data->dfs_id);
	if (!max_freq) {
		dev_err(data->dev, "failed get max frequency\n");
		return -EINVAL;
	}

	dev_info(data->dev, "max_freq: %uKhz, get_max_freq: %uKhz\n",
			data->max_freq, max_freq);

	if (max_freq < data->max_freq) {
		rcu_read_lock();
		flags |= DEVFREQ_FLAG_LEAST_UPPER_BOUND;
		tmp_max = (unsigned long)max_freq;
		target_opp = devfreq_recommended_opp(data->dev, &tmp_max, flags);
		if (IS_ERR(target_opp)) {
			rcu_read_unlock();
			dev_err(data->dev, "not found valid OPP for max_freq\n");
			return PTR_ERR(target_opp);
		}

		data->max_freq = (u32)dev_pm_opp_get_freq(target_opp);
		rcu_read_unlock();
	}

	/* min ferquency must be equal or under max frequency */
	if (data->min_freq > data->max_freq)
		data->min_freq = data->max_freq;

	min_freq = (u32)cal_dfs_get_min_freq(data->dfs_id);
	if (!min_freq) {
		dev_err(data->dev, "failed get min frequency\n");
		return -EINVAL;
	}

	dev_info(data->dev, "min_freq: %uKhz, get_min_freq: %uKhz\n",
			data->min_freq, min_freq);

	if (min_freq > data->min_freq) {
		rcu_read_lock();
		flags &= ~DEVFREQ_FLAG_LEAST_UPPER_BOUND;
		tmp_min = (unsigned long)min_freq;
		target_opp = devfreq_recommended_opp(data->dev, &tmp_min, flags);
		if (IS_ERR(target_opp)) {
			rcu_read_unlock();
			dev_err(data->dev, "not found valid OPP for min_freq\n");
			return PTR_ERR(target_opp);
		}

		data->min_freq = (u32)dev_pm_opp_get_freq(target_opp);
		rcu_read_unlock();
	}

	dev_info(data->dev, "min_freq: %uKhz, max_freq: %uKhz\n",
			data->min_freq, data->max_freq);

	for (i = 0; i < data->max_state; i++) {
		if (data->opp_list[i].freq > data->max_freq ||
			data->opp_list[i].freq < data->min_freq)
			dev_pm_opp_disable(data->dev, (unsigned long)data->opp_list[i].freq);
	}

	data->devfreq_profile.initial_freq = cal_dfs_get_boot_freq(data->dfs_id);
	data->devfreq_profile.suspend_freq = cal_dfs_get_resume_freq(data->dfs_id);

	return 0;
}

static int exynos8895_devfreq_int_get_status(struct exynos_devfreq_data *data)
{
#ifdef CONFIG_EXYNOS_WD_DVFS
	int i;
	struct ppmu_data ppmu = { 0, };
	u64 max = 0;

	for (i = 0; i < data->um_data.um_count; i++)
		exynos_reset_ppmu(data->um_data.va_base[i],
				  data->um_data.channel[i]);

	for (i = 0; i < data->um_data.um_count; i++) {
		exynos_read_ppmu(&ppmu, data->um_data.va_base[i],
				 data->um_data.channel[i]);
		if (max < ppmu.pmcnt2)
			max = ppmu.pmcnt2;
		if (max < ppmu.pmcnt3)
			max = ppmu.pmcnt3;
	}
	data->um_data.val_pmcnt = max;
	data->um_data.val_ccnt = (((u64)data->last_monitor_period) *
				  data->devfreq->previous_freq) /
		(NSEC_PER_MSEC);
#endif
	return 0;
}

static int __init exynos8895_devfreq_int_init_prepare(struct exynos_devfreq_data *data)
{
	data->ops.get_dev_status = exynos8895_devfreq_int_get_status;
	data->ops.get_freq = exynos8895_devfreq_int_get_freq;
	data->ops.set_freq = exynos8895_devfreq_int_set_freq;
	data->ops.init_freq_table = exynos8895_devfreq_int_init_freq_table;
	data->ops.pm_suspend_prepare = exynos8895_devfreq_int_pm_suspend_prepare;
	data->ops.resume = exynos8895_devfreq_int_resume;
	data->ops.reboot = exynos8895_devfreq_int_reboot;
	data->ops.cmu_dump = exynos8895_devfreq_int_cmu_dump;

	return 0;
}

static int __init exynos8895_devfreq_int_initcall(void)
{
	if (register_exynos_devfreq_init_prepare(DEVFREQ_INT,
				exynos8895_devfreq_int_init_prepare))
		return -EINVAL;

	return 0;
}

fs_initcall(exynos8895_devfreq_int_initcall);
