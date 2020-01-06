/* linux/drivers/devfreq/exynos/exynos9810_bus_intcam.c
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com
 *
 * Samsung EXYNOS9810 SoC INTCAM devfreq driver
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
#include <soc/samsung/cal-if.h>
#include "../governor.h"

static int exynos9810_devfreq_intcam_cmu_dump(struct exynos_devfreq_data *data)
{
	cal_vclk_dbg_info(data->dfs_id);

	return 0;
}

static int exynos9810_devfreq_intcam_reboot(struct exynos_devfreq_data *data)
{
	data->max_freq = data->reboot_freq;
	data->devfreq->max_freq = data->max_freq;

	mutex_lock(&data->devfreq->lock);
	update_devfreq(data->devfreq);
	mutex_unlock(&data->devfreq->lock);

	return 0;
}

static int exynos9810_devfreq_intcam_get_freq(struct device *dev, u32 *cur_freq,
		struct clk *clk, struct exynos_devfreq_data *data)
{
	*cur_freq = (u32)cal_dfs_get_rate(data->dfs_id);
	if (*cur_freq == 0) {
		dev_err(data->dev, "failed get frequency from CAL\n");
		return -EINVAL;
	}

        return 0;
}

static int exynos9810_devfreq_intcam_set_freq(struct device *dev, u32 new_freq,
		struct clk *clk, struct exynos_devfreq_data *data)
{
	if (cal_dfs_set_rate(data->dfs_id, (unsigned long)new_freq)) {
		dev_err(dev, "failed set frequency to CAL (%uKhz)\n",
				new_freq);
		return -EINVAL;
	}

	return 0;
}

static int exynos9810_devfreq_intcam_resume(struct exynos_devfreq_data *data)
{
	u32 cur_freq;

	/* for sync from resume frequency */
	if (exynos9810_devfreq_intcam_get_freq(data->dev, &cur_freq, data->clk, data)) {
		dev_err(data->dev, "failed get frequency when resume\n");
		return -EINVAL;
	}

	dev_info(data->dev, "Resume frequency is %u\n", cur_freq);

	return 0;
}

static int exynos9810_devfreq_intcam_init_freq_table(struct exynos_devfreq_data *data)
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

static int __init exynos9810_devfreq_intcam_init_prepare(struct exynos_devfreq_data *data)
{
	data->ops.get_freq = exynos9810_devfreq_intcam_get_freq;
	data->ops.set_freq = exynos9810_devfreq_intcam_set_freq;
	data->ops.init_freq_table = exynos9810_devfreq_intcam_init_freq_table;
	data->ops.resume = exynos9810_devfreq_intcam_resume;
	data->ops.reboot = exynos9810_devfreq_intcam_reboot;
	data->ops.cmu_dump = exynos9810_devfreq_intcam_cmu_dump;

	return 0;
}

static int __init exynos9810_devfreq_intcam_initcall(void)
{
	if (register_exynos_devfreq_init_prepare(DEVFREQ_INTCAM,
				exynos9810_devfreq_intcam_init_prepare))
		return -EINVAL;

	return 0;
}

fs_initcall(exynos9810_devfreq_intcam_initcall);
