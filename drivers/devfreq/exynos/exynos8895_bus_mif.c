/* linux/drivers/devfreq/exynos/exynos8895_bus_mif.c
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Samsung EXYNOS8895 SoC MIF devfreq driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, either version 2 of the License,
 * or (at your option) any later version.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/list.h>
#include <linux/clk.h>
#include <linux/workqueue.h>

#include <soc/samsung/exynos-devfreq.h>
#include <soc/samsung/bts.h>
#include <linux/apm-exynos.h>
#include <soc/samsung/asv-exynos.h>
#include <linux/mcu_ipc.h>
#include <linux/mfd/samsung/core.h>
#include <soc/samsung/cal-if.h>
#include "../governor.h"

#include <soc/samsung/exynos-dm.h>
#include <soc/samsung/ect_parser.h>
#include "../../soc/samsung/acpm/acpm.h"
#include "../../soc/samsung/acpm/acpm_ipc.h"
#include "exynos_ppmu.h"

#define INT	0

static unsigned int ect_find_constraint_freq(struct ect_minlock_domain *ect_domain,
					unsigned int freq)
{
	unsigned int i;

	for (i =0; i < ect_domain->num_of_level; i++)
		if (ect_domain->level[i].main_frequencies == freq) break;

	return ect_domain->level[i].sub_frequencies;
}

static int exynos8895_mif_constraint_parse(struct exynos_devfreq_data *data,
		unsigned int min_freq, unsigned int max_freq)
{
	int i;
	int ret;
	int ch_num;
	int size;
	int use_level = 0;
	int const_flag = 1;
	unsigned int cmd[4];
	struct ipc_config config;
	void *min_block;
	void *dvfs_block;
	struct ect_dvfs_domain *dvfs_domain;
	struct ect_minlock_domain *ect_domain;
#ifdef CONFIG_EXYNOS_DVFS_MANAGER
	struct exynos_dm_freq *const_table;
#endif
	dvfs_block = ect_get_block(BLOCK_DVFS);
	if (dvfs_block == NULL)
		return -ENODEV;

	dvfs_domain = ect_dvfs_get_domain(dvfs_block, "dvfs_mif");
	if (dvfs_domain == NULL)
		return -ENODEV;

	/* Although there is not any constraint, MIF table should be sent to FVP */
	min_block = ect_get_block(BLOCK_MINLOCK);
	if (min_block == NULL) {
		dev_info(data->dev, "There is not a min block in ECT\n");
		const_flag = 0;
	}

	ect_domain = ect_minlock_get_domain(min_block, "dvfs_mif");
	if (ect_domain == NULL) {
		dev_info(data->dev, "There is not a domain in min block\n");
		const_flag = 0;
	}

#ifdef CONFIG_EXYNOS_DVFS_MANAGER
	if(const_flag) {
		data->constraint[INT] = kzalloc(sizeof(struct exynos_dm_constraint), GFP_KERNEL);
		if (data->constraint[INT] == NULL) {
			dev_err(data->dev, "failed to allocate constraint\n");
			return -ENOMEM;
		}

		const_table = kzalloc(sizeof(struct exynos_dm_freq) * ect_domain->num_of_level, GFP_KERNEL);
		if (const_table == NULL) {
			dev_err(data->dev, "failed to allocate constraint\n");
			kfree(data->constraint[INT]);
			return -ENOMEM;
		}

		data->constraint[INT]->guidance = true;
		data->constraint[INT]->constraint_type = CONSTRAINT_MIN;
		data->constraint[INT]->constraint_dm_type = DM_INT;
		data->constraint[INT]->table_length = ect_domain->num_of_level;
		data->constraint[INT]->freq_table = const_table;
	}
#endif
	ret = acpm_ipc_request_channel(data->dev->of_node, NULL, &ch_num, &size);
	if (ret) {
		dev_err(data->dev, "acpm request channel is failed, id:%u, size:%u\n", ch_num, size);
		return -EINVAL;
	}

	config.cmd = cmd;
	config.response = true;
	config.indirection = false;

	for (i = 0; i < dvfs_domain->num_of_level; i++) {
		if (data->opp_list[i].freq > max_freq ||
				data->opp_list[i].freq < min_freq)
			continue;

		config.cmd[0] = use_level;
		config.cmd[1] = data->opp_list[i].freq;
		config.cmd[2] = DATA_INIT;
		config.cmd[3] = 0;
#ifdef CONFIG_EXYNOS_DVFS_MANAGER
		if (const_flag) {
			const_table[use_level].master_freq = data->opp_list[i].freq;
			const_table[use_level].constraint_freq
				= ect_find_constraint_freq(ect_domain, data->opp_list[i].freq);
			config.cmd[3] = const_table[use_level].constraint_freq;
		}
#endif
		ret = acpm_ipc_send_data(ch_num, &config);
		if (ret) {
			dev_err(data->dev, "make constraint table is failed");
			return -EINVAL;
		}
		use_level++;
	}
	/* Send MIF initial freq and the number of constraint data to FVP */
	if (const_flag) {
		config.cmd[0] = use_level;
		config.cmd[1] = (unsigned int)data->devfreq_profile.initial_freq;
		config.cmd[2] = DATA_INIT;
		config.cmd[3] = SET_CONST;

		ret = acpm_ipc_send_data(ch_num, &config);
		if (ret) {
			dev_err(data->dev, "failed to send nr_constraint and init freq");
			return -EINVAL;
		}
	}
	return 0;
}

static int exynos8895_devfreq_mif_cmu_dump(struct exynos_devfreq_data *data)
{
	mutex_lock(&data->devfreq->lock);
	cal_vclk_dbg_info(data->dfs_id);
	mutex_unlock(&data->devfreq->lock);

	return 0;
}

static int exynos8895_devfreq_mif_resume(struct exynos_devfreq_data *data)
{
#ifndef CONFIG_EXYNOS_DVFS_MANAGER
	if (pm_qos_request_active(&data->default_pm_qos_max))
		pm_qos_update_request(&data->default_pm_qos_max, data->max_freq);
#endif

	return 0;
}

static int exynos8895_devfreq_mif_suspend(struct exynos_devfreq_data *data)
{
#ifndef CONFIG_EXYNOS_DVFS_MANAGER
	if (pm_qos_request_active(&data->default_pm_qos_max))
		pm_qos_update_request(&data->default_pm_qos_max,
				data->devfreq_profile.suspend_freq);
#endif

	return 0;
}

static int exynos8895_devfreq_mif_reboot(struct exynos_devfreq_data *data)
{
	if (pm_qos_request_active(&data->default_pm_qos_max))
		pm_qos_update_request(&data->default_pm_qos_max,
				data->reboot_freq);

	return 0;
}

static int exynos8895_devfreq_mif_get_freq(struct device *dev, u32 *cur_freq,
		struct clk *clk, struct exynos_devfreq_data *data)
{
	*cur_freq = (u32)cal_dfs_get_rate(data->dfs_id);
	if (*cur_freq == 0) {
		dev_err(dev, "failed get frequency from CAL\n");
		return -EINVAL;
	}

	return 0;
}

static int exynos8895_devfreq_mif_set_freq(struct device *dev, u32 new_freq,
		struct clk *clk, struct exynos_devfreq_data *data)
{
	if (cal_dfs_set_rate(data->dfs_id, (unsigned long)new_freq)) {
		dev_err(dev, "failed set frequency to CAL (%uKhz)\n",
				new_freq);
		return -EINVAL;
	}
	return 0;
}

static int exynos8895_devfreq_mif_init_freq_table(struct exynos_devfreq_data *data)
{
	u32 max_freq, min_freq, cur_freq;
	unsigned long tmp_max, tmp_min;
	struct dev_pm_opp *target_opp;
	u32 flags = 0;
	int i, ret;

	ret = cal_clk_enable(data->dfs_id);
	if (ret) {
		dev_err(data->dev, "failed to enable MIF\n");
		return -EINVAL;
	}

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

	cur_freq = (u32)clk_get_rate(data->clk);
	dev_info(data->dev, "current frequency: %uKhz\n", cur_freq);

	for (i = 0; i < data->max_state; i++) {
		if (data->opp_list[i].freq > data->max_freq ||
			data->opp_list[i].freq < data->min_freq)
			dev_pm_opp_disable(data->dev, (unsigned long)data->opp_list[i].freq);
	}

	data->devfreq_profile.initial_freq = cal_dfs_get_boot_freq(data->dfs_id);
	data->devfreq_profile.suspend_freq = cal_dfs_get_resume_freq(data->dfs_id);

	ret = exynos8895_mif_constraint_parse(data, min_freq, max_freq);
	if (ret) {
		dev_err(data->dev, "failed to parse constraint table\n");
		return -EINVAL;
	}

	return 0;
}

static int exynos8895_devfreq_mif_um_register(struct exynos_devfreq_data *data)
{
#ifdef CONFIG_EXYNOS_WD_DVFS
	int i;
	if (data->use_get_dev) {
		for (i = 0; i < data->um_data.um_count; i++)
			exynos_init_ppmu(data->um_data.va_base[i],
					 data->um_data.mask_v[i],
					 data->um_data.mask_a[i]);
		for (i = 0; i < data->um_data.um_count; i++)
			exynos_start_ppmu(data->um_data.va_base[i]);
	}
#endif
	return 0;
}

static int exynos8895_devfreq_mif_um_unregister(struct exynos_devfreq_data *data)
{
#ifdef CONFIG_EXYNOS_WD_DVFS
	int i;
	if (data->use_get_dev) {
		for (i = 0; i < data->um_data.um_count; i++)
			exynos_exit_ppmu(data->um_data.va_base[i]);
	}
#endif
	return 0;
}

static int exynos8895_devfreq_mif_get_status(struct exynos_devfreq_data *data)
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
		if (!i)
			data->um_data.val_ccnt = ppmu.ccnt;
		if (max < ppmu.pmcnt0)
			max = ppmu.pmcnt0;
		if (max < ppmu.pmcnt1)
			max = ppmu.pmcnt1;
	}
	data->um_data.val_pmcnt = max;
#endif
	return 0;
}

static int exynos8895_devfreq_mif_set_freq_prepare(struct exynos_devfreq_data *data)
{
	if (data->new_freq < data->old_freq)
		bts_update_scen(BS_MIF_CHANGE, data->new_freq);
	return 0;
}

static int exynos8895_devfreq_mif_set_freq_post(struct exynos_devfreq_data *data)
{
	if (data->new_freq > data->old_freq)
		bts_update_scen(BS_MIF_CHANGE, data->new_freq);
	return 0;
}

static int __init exynos8895_devfreq_mif_init_prepare(struct exynos_devfreq_data *data)
{
	data->ops.um_register = exynos8895_devfreq_mif_um_register;
	data->ops.um_unregister = exynos8895_devfreq_mif_um_unregister;
	data->ops.get_dev_status = exynos8895_devfreq_mif_get_status;
	data->ops.get_freq = exynos8895_devfreq_mif_get_freq;
	data->ops.set_freq = exynos8895_devfreq_mif_set_freq;
	data->ops.init_freq_table = exynos8895_devfreq_mif_init_freq_table;
	data->ops.suspend = exynos8895_devfreq_mif_suspend;
	data->ops.reboot = exynos8895_devfreq_mif_reboot;
	data->ops.resume = exynos8895_devfreq_mif_resume;
	data->ops.cmu_dump = exynos8895_devfreq_mif_cmu_dump;
	data->ops.set_freq_prepare = exynos8895_devfreq_mif_set_freq_prepare;
	data->ops.set_freq_post = exynos8895_devfreq_mif_set_freq_post;

	return 0;
}

static int __init exynos8895_devfreq_mif_initcall(void)
{
	if (register_exynos_devfreq_init_prepare(DEVFREQ_MIF,
				exynos8895_devfreq_mif_init_prepare))
		return -EINVAL;

	return 0;
}
fs_initcall(exynos8895_devfreq_mif_initcall);
