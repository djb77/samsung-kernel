/* linux/drivers/soc/samsung/exynos-dm.c
 *
 * Copyright (C) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Samsung Exynos SoC series DVFS Manager
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/errno.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/exynos-ss.h>
#include "acpm/acpm.h"
#include "acpm/acpm_ipc.h"

#include <soc/samsung/exynos-dm.h>

static struct list_head *get_min_constraint_list(struct exynos_dm_data *dm_data);
static struct list_head *get_max_constraint_list(struct exynos_dm_data *dm_data);
static void get_governor_min_freq(struct exynos_dm_data *dm_data, u32 *gov_min_freq);
static void get_min_max_freq(struct exynos_dm_data *dm_data, u32 *min_freq, u32 *max_freq);
static void update_min_max_freq(struct exynos_dm_data *dm_data, u32 min_freq, u32 max_freq);
static void get_policy_min_max_freq(struct exynos_dm_data *dm_data, u32 *min_freq, u32 *max_freq);
static void update_policy_min_max_freq(struct exynos_dm_data *dm_data, u32 min_freq, u32 max_freq);
static void get_current_freq(struct exynos_dm_data *dm_data, u32 *cur_freq);
static void get_target_freq(struct exynos_dm_data *dm_data, u32 *target_freq);

#define DM_EMPTY	0xFF
static struct exynos_dm_device *exynos_dm;
static enum exynos_dm_type min_order[DM_TYPE_END + 1] = {DM_EMPTY, };
static enum exynos_dm_type max_order[DM_TYPE_END + 1] = {DM_EMPTY, };

/*
 * SYSFS for Debugging
 */
static ssize_t show_available(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = container_of(dev, struct platform_device, dev);
	struct exynos_dm_device *dm = platform_get_drvdata(pdev);
	ssize_t count = 0;
	int i;

	for (i = 0; i < DM_TYPE_END; i++) {
		if (!dm->dm_data[i].available)
			continue;

		count += snprintf(buf + count, PAGE_SIZE,
				"dm_type: %d(%s), dvfs_type: %d, available = %s\n",
				dm->dm_data[i].dm_type, dm->dm_data[i].dm_type_name,
				dm->dm_data[i].dvfs_type,
				dm->dm_data[i].available ? "true" : "false");
	}

	return count;
}

#define show_constraint_tables(dm_type, type_name)					\
static ssize_t show_constraint_tables_##type_name					\
(struct device *dev, struct device_attribute *attr, char *buf)				\
{											\
	struct platform_device *pdev = container_of(dev, struct platform_device, dev);	\
	struct exynos_dm_device *dm = platform_get_drvdata(pdev);			\
	struct list_head *constraint_list;						\
	struct exynos_dm_constraint *constraint;					\
	ssize_t count = 0;								\
	int i;										\
											\
	if (!dm->dm_data[dm_type].available) {						\
		count += snprintf(buf + count, PAGE_SIZE,				\
				"This dm_type is not available\n");			\
		return count;								\
	}										\
											\
	count += snprintf(buf + count, PAGE_SIZE, "dm_type: %s\n",			\
				dm->dm_data[dm_type].dm_type_name);			\
											\
	constraint_list = get_min_constraint_list(&dm->dm_data[dm_type]);		\
	if (list_empty(constraint_list)) {						\
		count += snprintf(buf + count, PAGE_SIZE,				\
				"This dm_type have not min constraint tables\n\n");	\
		goto next;								\
	}										\
											\
	list_for_each_entry(constraint, constraint_list, node) {			\
		count += snprintf(buf + count, PAGE_SIZE,				\
				"-------------------------------------------------\n");	\
		count += snprintf(buf + count, PAGE_SIZE,				\
				"constraint_dm_type = %s\n", constraint->dm_type_name);	\
		count += snprintf(buf + count, PAGE_SIZE, "constraint_type: %s\n",	\
				constraint->constraint_type ? "MAX" : "MIN");		\
		count += snprintf(buf + count, PAGE_SIZE, "guidance: %s\n",		\
				constraint->guidance ? "true" : "false");		\
		count += snprintf(buf + count, PAGE_SIZE,				\
				"min_freq = %u, max_freq =%u\n",			\
				constraint->min_freq, constraint->max_freq);		\
		count += snprintf(buf + count, PAGE_SIZE,				\
				"master_freq\t constraint_freq\n");			\
		for (i = 0; i < constraint->table_length; i++)				\
			count += snprintf(buf + count, PAGE_SIZE, "%10u\t %10u\n",	\
					constraint->freq_table[i].master_freq,		\
					constraint->freq_table[i].constraint_freq);	\
		count += snprintf(buf + count, PAGE_SIZE,				\
				"-------------------------------------------------\n");	\
	}										\
											\
next:											\
	constraint_list = get_max_constraint_list(&dm->dm_data[dm_type]);		\
	if (list_empty(constraint_list)) {						\
		count += snprintf(buf + count, PAGE_SIZE,				\
				"This dm_type have not max constraint tables\n\n");	\
		return count;								\
	}										\
											\
	list_for_each_entry(constraint, constraint_list, node) {			\
		count += snprintf(buf + count, PAGE_SIZE,				\
				"-------------------------------------------------\n");	\
		count += snprintf(buf + count, PAGE_SIZE,				\
				"constraint_dm_type = %s\n", constraint->dm_type_name);	\
		count += snprintf(buf + count, PAGE_SIZE, "constraint_type: %s\n",	\
				constraint->constraint_type ? "MAX" : "MIN");		\
		count += snprintf(buf + count, PAGE_SIZE, "guidance: %s\n",		\
				constraint->guidance ? "true" : "false");		\
		count += snprintf(buf + count, PAGE_SIZE,				\
				"min_freq = %u, max_freq =%u\n",			\
				constraint->min_freq, constraint->max_freq);		\
		count += snprintf(buf + count, PAGE_SIZE,				\
				"master_freq\t constraint_freq\n");			\
		for (i = 0; i < constraint->table_length; i++)				\
			count += snprintf(buf + count, PAGE_SIZE, "%10u\t %10u\n",	\
					constraint->freq_table[i].master_freq,		\
					constraint->freq_table[i].constraint_freq);	\
		count += snprintf(buf + count, PAGE_SIZE,				\
				"-------------------------------------------------\n");	\
	}										\
											\
	return count;									\
}

#define show_dm_policy(dm_type, type_name)						\
static ssize_t show_dm_policy_##type_name						\
(struct device *dev, struct device_attribute *attr, char *buf)				\
{											\
	struct platform_device *pdev = container_of(dev, struct platform_device, dev);	\
	struct exynos_dm_device *dm = platform_get_drvdata(pdev);			\
	ssize_t count = 0;								\
	u32 gov_min_freq, min_freq, max_freq;						\
	u32 policy_min_freq, policy_max_freq, cur_freq, target_freq;			\
											\
	if (!dm->dm_data[dm_type].available) {						\
		count += snprintf(buf + count, PAGE_SIZE,				\
				"This dm_type is not available\n");			\
		return count;								\
	}										\
											\
	count += snprintf(buf + count, PAGE_SIZE, "dm_type: %s\n",			\
				dm->dm_data[dm_type].dm_type_name);			\
											\
	get_governor_min_freq(&dm->dm_data[dm_type], &gov_min_freq);			\
	get_min_max_freq(&dm->dm_data[dm_type], &min_freq, &max_freq);			\
	get_policy_min_max_freq(&dm->dm_data[dm_type],					\
				&policy_min_freq, &policy_max_freq);			\
	get_current_freq(&dm->dm_data[dm_type], &cur_freq);				\
	get_target_freq(&dm->dm_data[dm_type], &target_freq);				\
											\
	count += snprintf(buf + count, PAGE_SIZE,					\
			"governor_min_freq = %u\n", gov_min_freq);			\
	count += snprintf(buf + count, PAGE_SIZE,					\
			"policy_min_freq = %u, policy_max_freq = %u\n",			\
			policy_min_freq, policy_max_freq);				\
	count += snprintf(buf + count, PAGE_SIZE,					\
			"min_freq = %u, max_freq = %u\n", min_freq, max_freq);		\
	count += snprintf(buf + count, PAGE_SIZE, "current_freq = %u\n", cur_freq);	\
	count += snprintf(buf + count, PAGE_SIZE, "target_freq = %u\n", target_freq);	\
											\
	return count;									\
}

show_constraint_tables(DM_CPU_CL0, dm_cpu_cl0);
show_constraint_tables(DM_CPU_CL1, dm_cpu_cl1);
show_constraint_tables(DM_MIF, dm_mif);
show_constraint_tables(DM_INT, dm_int);
show_constraint_tables(DM_INTCAM, dm_intcam);
show_constraint_tables(DM_FSYS0, dm_fsys0);
show_constraint_tables(DM_CAM, dm_cam);
show_constraint_tables(DM_DISP, dm_disp);
show_constraint_tables(DM_AUD, dm_aud);
show_constraint_tables(DM_IVA, dm_iva);
show_constraint_tables(DM_SCORE, dm_score);
show_constraint_tables(DM_GPU, dm_gpu);

show_dm_policy(DM_CPU_CL0, dm_cpu_cl0);
show_dm_policy(DM_CPU_CL1, dm_cpu_cl1);
show_dm_policy(DM_MIF, dm_mif);
show_dm_policy(DM_INT, dm_int);
show_dm_policy(DM_INTCAM, dm_intcam);
show_dm_policy(DM_FSYS0, dm_fsys0);
show_dm_policy(DM_CAM, dm_cam);
show_dm_policy(DM_DISP, dm_disp);
show_dm_policy(DM_AUD, dm_aud);
show_dm_policy(DM_IVA, dm_iva);
show_dm_policy(DM_SCORE, dm_score);
show_dm_policy(DM_GPU, dm_gpu);

static DEVICE_ATTR(available, 0440, show_available, NULL);
static DEVICE_ATTR(constraint_tables_dm_cpu_cl0, 0440, show_constraint_tables_dm_cpu_cl0, NULL);
static DEVICE_ATTR(constraint_tables_dm_cpu_cl1, 0440, show_constraint_tables_dm_cpu_cl1, NULL);
static DEVICE_ATTR(constraint_tables_dm_mif, 0440, show_constraint_tables_dm_mif, NULL);
static DEVICE_ATTR(constraint_tables_dm_int, 0440, show_constraint_tables_dm_int, NULL);
static DEVICE_ATTR(constraint_tables_dm_intcam, 0440, show_constraint_tables_dm_intcam, NULL);
static DEVICE_ATTR(constraint_tables_dm_fsys0, 0440, show_constraint_tables_dm_fsys0, NULL);
static DEVICE_ATTR(constraint_tables_dm_cam, 0440, show_constraint_tables_dm_cam, NULL);
static DEVICE_ATTR(constraint_tables_dm_disp, 0440, show_constraint_tables_dm_disp, NULL);
static DEVICE_ATTR(constraint_tables_dm_aud, 0440, show_constraint_tables_dm_aud, NULL);
static DEVICE_ATTR(constraint_tables_dm_iva, 0440, show_constraint_tables_dm_iva, NULL);
static DEVICE_ATTR(constraint_tables_dm_score, 0440, show_constraint_tables_dm_score, NULL);
static DEVICE_ATTR(constraint_tables_dm_gpu, 0440, show_constraint_tables_dm_gpu, NULL);
static DEVICE_ATTR(dm_policy_dm_cpu_cl0, 0440, show_dm_policy_dm_cpu_cl0, NULL);
static DEVICE_ATTR(dm_policy_dm_cpu_cl1, 0440, show_dm_policy_dm_cpu_cl1, NULL);
static DEVICE_ATTR(dm_policy_dm_mif, 0440, show_dm_policy_dm_mif, NULL);
static DEVICE_ATTR(dm_policy_dm_int, 0440, show_dm_policy_dm_int, NULL);
static DEVICE_ATTR(dm_policy_dm_intcam, 0440, show_dm_policy_dm_intcam, NULL);
static DEVICE_ATTR(dm_policy_dm_fsys0, 0440, show_dm_policy_dm_fsys0, NULL);
static DEVICE_ATTR(dm_policy_dm_cam, 0440, show_dm_policy_dm_cam, NULL);
static DEVICE_ATTR(dm_policy_dm_disp, 0440, show_dm_policy_dm_disp, NULL);
static DEVICE_ATTR(dm_policy_dm_aud, 0440, show_dm_policy_dm_aud, NULL);
static DEVICE_ATTR(dm_policy_dm_iva, 0440, show_dm_policy_dm_iva, NULL);
static DEVICE_ATTR(dm_policy_dm_score, 0440, show_dm_policy_dm_score, NULL);
static DEVICE_ATTR(dm_policy_dm_gpu, 0440, show_dm_policy_dm_gpu, NULL);

static struct attribute *exynos_dm_sysfs_entries[] = {
	&dev_attr_available.attr,
	&dev_attr_constraint_tables_dm_cpu_cl0.attr,
	&dev_attr_constraint_tables_dm_cpu_cl1.attr,
	&dev_attr_constraint_tables_dm_mif.attr,
	&dev_attr_constraint_tables_dm_int.attr,
	&dev_attr_constraint_tables_dm_intcam.attr,
	&dev_attr_constraint_tables_dm_fsys0.attr,
	&dev_attr_constraint_tables_dm_cam.attr,
	&dev_attr_constraint_tables_dm_disp.attr,
	&dev_attr_constraint_tables_dm_aud.attr,
	&dev_attr_constraint_tables_dm_iva.attr,
	&dev_attr_constraint_tables_dm_score.attr,
	&dev_attr_constraint_tables_dm_gpu.attr,
	&dev_attr_dm_policy_dm_cpu_cl0.attr,
	&dev_attr_dm_policy_dm_cpu_cl1.attr,
	&dev_attr_dm_policy_dm_mif.attr,
	&dev_attr_dm_policy_dm_int.attr,
	&dev_attr_dm_policy_dm_intcam.attr,
	&dev_attr_dm_policy_dm_fsys0.attr,
	&dev_attr_dm_policy_dm_cam.attr,
	&dev_attr_dm_policy_dm_disp.attr,
	&dev_attr_dm_policy_dm_aud.attr,
	&dev_attr_dm_policy_dm_iva.attr,
	&dev_attr_dm_policy_dm_score.attr,
	&dev_attr_dm_policy_dm_gpu.attr,
	NULL,
};

static struct attribute_group exynos_dm_attr_group = {
	.name	= "exynos_dm",
	.attrs	= exynos_dm_sysfs_entries,
};
/*
 * SYSFS for Debugging end
 */

static void print_available_dm_data(struct exynos_dm_device *dm)
{
	int i;

	for (i = 0; i < DM_TYPE_END; i++) {
		if (!dm->dm_data[i].available)
			continue;

		dev_info(dm->dev, "dm_type: %d(%s), dvfs_type: %d, available = %s\n",
				dm->dm_data[i].dm_type, dm->dm_data[i].dm_type_name,
				dm->dm_data[i].dvfs_type,
				dm->dm_data[i].available ? "true" : "false");
	}
}

static int exynos_dm_index_validate(enum exynos_dm_type index)
{
	if ((index < DM_CPU_CL0) || (index >= DM_TYPE_END)) {
		dev_err(exynos_dm->dev, "invalid dm_index (%d)\n", index);
		return -EINVAL;
	}

	return 0;
}

static enum exynos_dvfs_type exynos_dm_dvfs_type(enum exynos_dm_type dm_type)
{
	enum exynos_dvfs_type dvfs_type;

	switch (dm_type) {
	case DM_CPU_CL0...DM_CPU_CL1:
		dvfs_type = DVFS_CPUFREQ;
		break;
	case DM_MIF...DM_SCORE:
		dvfs_type = DVFS_DEVFREQ;
		break;
	case DM_GPU:
		dvfs_type = DVFS_GPU;
		break;
	default:
		dvfs_type = DVFS_TYPE_END;
		dev_err(exynos_dm->dev, "invalid dm_type (%d)\n", dm_type);
		break;
	}

	return dvfs_type;
}

#ifdef CONFIG_OF
static int exynos_dm_parse_dt(struct device_node *np, struct exynos_dm_device *dm)
{
	struct device_node *child_np;
	int ret = 0;

	if (!np)
		return -ENODEV;

	for_each_child_of_node(np, child_np) {
		int index;
		const char *available;
#ifdef CONFIG_EXYNOS_ACPM
		const char *policy_use;
#endif
		if (of_property_read_u32(child_np, "dm-index", &index))
			return -ENODEV;

		ret = exynos_dm_index_validate(index);
		if (ret)
			return ret;

		if (of_property_read_string(child_np, "available", &available))
			return -ENODEV;

		if (!strcmp(available, "true")) {
			dm->dm_data[index].dm_type = index;
			dm->dm_data[index].available = true;
			dm->dm_data[index].dvfs_type = exynos_dm_dvfs_type(index);
			strncpy(dm->dm_data[index].dm_type_name, dm_type_name[index],
							EXYNOS_DM_TYPE_NAME_LEN);
			INIT_LIST_HEAD(&dm->dm_data[index].min_clist);
			INIT_LIST_HEAD(&dm->dm_data[index].max_clist);
		} else {
			dm->dm_data[index].available = false;
		}
#ifdef CONFIG_EXYNOS_ACPM
		if (of_property_read_string(child_np, "policy_use", &policy_use)) {
			dev_info(dm->dev, "This doesn't need to send policy to ACPM\n");
		} else {
			if (!strcmp(policy_use, "true"))
				dm->dm_data[index].policy_use = true;
		}

		if (of_property_read_u32(child_np, "cal_id", &dm->dm_data[index].cal_id))
			return -ENODEV;
#endif
	}

	return ret;
}
#else
static int exynos_dm_parse_dt(struct device_node *np, struct exynos_dm_device *dm)
{
	return -ENODEV;
}
#endif

static struct list_head *get_min_constraint_list(struct exynos_dm_data *dm_data)
{
	return &dm_data->min_clist;
}

static struct list_head *get_max_constraint_list(struct exynos_dm_data *dm_data)
{
	return &dm_data->max_clist;
}

/*
 * This function should be called from each DVFS drivers
 * before DVFS driver registration to DVFS framework.
 * 	Initialize sequence Step.1
 */
int exynos_dm_data_init(enum exynos_dm_type dm_type,
			u32 min_freq, u32 max_freq, u32 cur_freq)
{
	int ret = 0;

	ret = exynos_dm_index_validate(dm_type);
	if (ret)
		return ret;

	mutex_lock(&exynos_dm->lock);

	if (!exynos_dm->dm_data[dm_type].available) {
		dev_err(exynos_dm->dev,
			"This dm type(%d) is not available\n", dm_type);
		ret = -ENODEV;
		goto out;
	}

	exynos_dm->dm_data[dm_type].gov_min_freq = min_freq;
	exynos_dm->dm_data[dm_type].policy_min_freq = min_freq;
	exynos_dm->dm_data[dm_type].policy_max_freq = max_freq;
	exynos_dm->dm_data[dm_type].cur_freq = cur_freq;

	if (!exynos_dm->dm_data[dm_type].min_freq)
		exynos_dm->dm_data[dm_type].min_freq = min_freq;

	if (!exynos_dm->dm_data[dm_type].max_freq)
		exynos_dm->dm_data[dm_type].max_freq = max_freq;

out:
	mutex_unlock(&exynos_dm->lock);

	return ret;
}

/*
 * 	Initialize sequence Step.2
 */
int register_exynos_dm_constraint_table(enum exynos_dm_type dm_type,
				struct exynos_dm_constraint *constraint)
{
	struct exynos_dm_constraint *sub_constraint;
	int i, ret = 0;

	ret = exynos_dm_index_validate(dm_type);
	if (ret)
		return ret;

	if (!constraint) {
		dev_err(exynos_dm->dev, "constraint is not valid\n");
		return -EINVAL;
	}

	/* check member invalid */
	if ((constraint->constraint_type < CONSTRAINT_MIN) ||
		(constraint->constraint_type > CONSTRAINT_MAX)) {
		dev_err(exynos_dm->dev, "constraint_type is invalid\n");
		return -EINVAL;
	}

	ret = exynos_dm_index_validate(constraint->constraint_dm_type);
	if (ret)
		return ret;

	if (!constraint->freq_table) {
		dev_err(exynos_dm->dev, "No frequency table for constraint\n");
		return -EINVAL;
	}

	mutex_lock(&exynos_dm->lock);

	strncpy(constraint->dm_type_name,
			dm_type_name[constraint->constraint_dm_type],
			EXYNOS_DM_TYPE_NAME_LEN);
	constraint->min_freq = 0;
	constraint->max_freq = UINT_MAX;

	if (constraint->constraint_type == CONSTRAINT_MIN)
		list_add(&constraint->node, &exynos_dm->dm_data[dm_type].min_clist);
	else if (constraint->constraint_type == CONSTRAINT_MAX)
		list_add(&constraint->node, &exynos_dm->dm_data[dm_type].max_clist);

	/* check guidance and sub constraint table generations */
	if (constraint->guidance && (constraint->constraint_type == CONSTRAINT_MIN)) {
		sub_constraint = kzalloc(sizeof(struct exynos_dm_constraint), GFP_KERNEL);
		if (sub_constraint == NULL) {
			dev_err(exynos_dm->dev, "failed to allocate sub constraint\n");
			ret = -ENOMEM;
			goto err_sub_const;
		}

		sub_constraint->guidance = true;
		sub_constraint->table_length = constraint->table_length;
		sub_constraint->constraint_type = CONSTRAINT_MAX;
		sub_constraint->constraint_dm_type = dm_type;
		strncpy(sub_constraint->dm_type_name,
				dm_type_name[sub_constraint->constraint_dm_type],
				EXYNOS_DM_TYPE_NAME_LEN);
		sub_constraint->min_freq = 0;
		sub_constraint->max_freq = UINT_MAX;

		sub_constraint->freq_table =
			kzalloc(sizeof(struct exynos_dm_freq) * sub_constraint->table_length, GFP_KERNEL);
		if (sub_constraint->freq_table == NULL) {
			dev_err(exynos_dm->dev, "failed to allocate freq table for sub const\n");
			ret = -ENOMEM;
			goto err_freq_table;
		}

		/* generation table */
		for (i = 0; i < constraint->table_length; i++) {
			sub_constraint->freq_table[i].master_freq =
					constraint->freq_table[i].constraint_freq;
			sub_constraint->freq_table[i].constraint_freq =
					constraint->freq_table[i].master_freq;
		}

		list_add(&sub_constraint->node,
			&exynos_dm->dm_data[constraint->constraint_dm_type].max_clist);

		/* linked sub constraint */
		constraint->sub_constraint = sub_constraint;
	}

	mutex_unlock(&exynos_dm->lock);

	return 0;

err_freq_table:
	kfree(sub_constraint);
err_sub_const:
	list_del(&constraint->node);

	mutex_unlock(&exynos_dm->lock);

	return ret;
}

int unregister_exynos_dm_constraint_table(enum exynos_dm_type dm_type,
				struct exynos_dm_constraint *constraint)
{
	struct exynos_dm_constraint *sub_constraint;
	int ret = 0;

	ret = exynos_dm_index_validate(dm_type);
	if (ret)
		return ret;

	if (!constraint) {
		dev_err(exynos_dm->dev, "constraint is not valid\n");
		return -EINVAL;
	}

	mutex_lock(&exynos_dm->lock);

	if (constraint->sub_constraint) {
		sub_constraint = constraint->sub_constraint;
		list_del(&sub_constraint->node);
		kfree(sub_constraint->freq_table);
		kfree(sub_constraint);
	}

	list_del(&constraint->node);

	mutex_unlock(&exynos_dm->lock);

	return 0;
}

/*
 * This function should be called from each DVFS driver registration function
 * before return to corresponding DVFS drvier.
 * 	Initialize sequence Step.3
 */
int register_exynos_dm_freq_scaler(enum exynos_dm_type dm_type,
			int (*scaler_func)(enum exynos_dm_type dm_type, u32 target_freq, unsigned int relation))
{
	int ret = 0;

	ret = exynos_dm_index_validate(dm_type);
	if (ret)
		return ret;

	if (!scaler_func) {
		dev_err(exynos_dm->dev, "function is not valid\n");
		return -EINVAL;
	}

	mutex_lock(&exynos_dm->lock);

	if (!exynos_dm->dm_data[dm_type].available) {
		dev_err(exynos_dm->dev,
			"This dm type(%d) is not available\n", dm_type);
		ret = -ENODEV;
		goto out;
	}

	if (!exynos_dm->dm_data[dm_type].freq_scaler)
		exynos_dm->dm_data[dm_type].freq_scaler = scaler_func;

out:
	mutex_unlock(&exynos_dm->lock);

	return 0;
}

int unregister_exynos_dm_freq_scaler(enum exynos_dm_type dm_type)
{
	int ret = 0;

	ret = exynos_dm_index_validate(dm_type);
	if (ret)
		return ret;

	mutex_lock(&exynos_dm->lock);

	if (!exynos_dm->dm_data[dm_type].available) {
		dev_err(exynos_dm->dev,
			"This dm type(%d) is not available\n", dm_type);
		ret = -ENODEV;
		goto out;
	}

	if (exynos_dm->dm_data[dm_type].freq_scaler)
		exynos_dm->dm_data[dm_type].freq_scaler = NULL;

out:
	mutex_unlock(&exynos_dm->lock);

	return 0;
}

/*
 * Policy Updater
 *
 * @dm_type: DVFS domain type for updating policy
 * @min_freq: Minimum frequency decided by policy
 * @max_freq: Maximum frequency decided by policy
 *
 * In this function, policy_min_freq and policy_max_freq will be changed.
 * After that, DVFS Manager will decide min/max freq. of current domain
 * and check dependent domains whether update is necessary.
 */
static int dm_data_updater(enum exynos_dm_type dm_type);
static int constraint_checker_min(struct list_head *head, u32 freq);
static int constraint_checker_max(struct list_head *head, u32 freq);
static int constraint_data_updater(enum exynos_dm_type dm_type, int cnt);
static int max_constraint_data_updater(enum exynos_dm_type dm_type, int cnt);
static int scaling_callback(enum dvfs_direction dir, unsigned int relation);

static bool max_flag = false;

#define POLICY_REQ	4

int policy_update_call_to_DM(enum exynos_dm_type dm_type, u32 min_freq, u32 max_freq)
{
	struct exynos_dm_data *dm;
	struct timeval pre, before, after;
#ifdef CONFIG_EXYNOS_ACPM
	struct ipc_config config;
	unsigned int cmd[4];
	int size, ch_num, ret;
#endif
	s32 time = 0, pre_time = 0;

	exynos_ss_dm((int)dm_type, min_freq, max_freq, pre_time, time);

	do_gettimeofday(&pre);
	mutex_lock(&exynos_dm->lock);
	do_gettimeofday(&before);

	min_freq = min(min_freq, max_freq);

	dm = &exynos_dm->dm_data[dm_type];
	if ((dm->policy_min_freq == min_freq) && (dm->policy_max_freq == max_freq))
		goto out;

	update_policy_min_max_freq(dm, min_freq, max_freq);

	/* Check dependent domains */

	/*Send policy to FVP*/
#ifdef CONFIG_EXYNOS_ACPM
	if (dm->policy_use) {
		ret = acpm_ipc_request_channel(exynos_dm->dev->of_node, NULL, &ch_num, &size);
		if (ret) {
			dev_err(exynos_dm->dev,
					"acpm request channel is failed, id:%u, size:%u\n", ch_num, size);
			goto out;
		}
		config.cmd = cmd;
		config.response = true;
		config.indirection = false;
		config.cmd[0] = dm->cal_id;
		config.cmd[1] = max_freq;
		config.cmd[2] = POLICY_REQ;

		ret = acpm_ipc_send_data(ch_num, &config);
		if (ret) {
			dev_err(exynos_dm->dev, "Failed to send policy data to FVP");
			goto out;
		}
	}
#endif

out:
	do_gettimeofday(&after);
	mutex_unlock(&exynos_dm->lock);

	pre_time = (before.tv_sec - pre.tv_sec) * USEC_PER_SEC +
		(before.tv_usec - pre.tv_usec);
	time = (after.tv_sec - before.tv_sec) * USEC_PER_SEC +
		(after.tv_usec - before.tv_usec);

	exynos_ss_dm((int)dm_type, min_freq, max_freq, pre_time, time);

	return 0;
}

static int constraint_checker_min(struct list_head *head, u32 freq)
{
	struct exynos_dm_data *dm;
	struct exynos_dm_constraint *constraint;
	int i;

	if (!list_empty(head)) {
		list_for_each_entry(constraint, head, node) {
			for (i = constraint->table_length - 1; i >= 0; i--) {
				if (freq <= constraint->freq_table[i].master_freq) {
					constraint->min_freq = constraint->freq_table[i].constraint_freq;
					break;
				}
			}
			dm_data_updater(constraint->constraint_dm_type);
			dm = &exynos_dm->dm_data[constraint->constraint_dm_type];
			constraint_checker_min(get_min_constraint_list(dm), dm->min_freq);
		}
	}

	return 0;
}

static int constraint_checker_max(struct list_head *head, u32 freq)
{
	struct exynos_dm_data *dm;
	struct exynos_dm_constraint *constraint;
	int i;

	if (!list_empty(head)) {
		list_for_each_entry(constraint, head, node) {
			for (i = 0; i < constraint->table_length; i++) {
				if (freq >= constraint->freq_table[i].master_freq) {
					constraint->max_freq = constraint->freq_table[i].constraint_freq;
					break;
				}
			}
			dm_data_updater(constraint->constraint_dm_type);
			dm = &exynos_dm->dm_data[constraint->constraint_dm_type];
			constraint_checker_max(get_max_constraint_list(dm), dm->max_freq);
		}
	}

	return 0;
}

/*
 * DM CALL
 */
int DM_CALL(enum exynos_dm_type dm_type, unsigned long *target_freq)
{
	struct exynos_dm_data *dm;
	int i;
	int ret;
	unsigned int relation = EXYNOS_DM_RELATION_L;
	u32 old_min_freq;
	struct timeval pre, before, after;
	s32 time = 0, pre_time = 0;

	exynos_ss_dm((int)dm_type, *target_freq, 1, pre_time, time);

	do_gettimeofday(&pre);
	mutex_lock(&exynos_dm->lock);
	do_gettimeofday(&before);

	dm = &exynos_dm->dm_data[dm_type];
	old_min_freq = dm->min_freq;
	dm->gov_min_freq = (u32)(*target_freq);

	for (i = 0; i < DM_TYPE_END; i++)
		(&exynos_dm->dm_data[i])->constraint_checked = 0;

	if (dm->policy_max_freq < dm->cur_freq)
		max_flag = true;
	else
		max_flag = false;

	ret = dm_data_updater(dm_type);
	if (ret) {
		pr_err("Failed to update DM DATA!\n");
		mutex_unlock(&exynos_dm->lock);
		return -EAGAIN;
	}

	dm->target_freq = (u32)(*target_freq);

	if (dm->target_freq < dm->min_freq)
		dm->target_freq = dm->min_freq;
	if (dm->target_freq >= dm->max_freq) {
		dm->target_freq = dm->max_freq;
		relation = EXYNOS_DM_RELATION_H;
	}

	*target_freq = dm->target_freq;

	/* Constratin checker should be called to decide target frequency */
	constraint_data_updater(dm_type, 1);
	max_constraint_data_updater(dm_type, 1);

	if (dm->target_freq > dm->cur_freq)
		scaling_callback(UP, relation);
	else if (dm->target_freq < dm->cur_freq)
		scaling_callback(DOWN, relation);
	else if (dm->min_freq > old_min_freq)
		scaling_callback(UP, relation);
	else if (dm->min_freq < old_min_freq)
		scaling_callback(DOWN, relation);

	/* min/max order clear */
	for (i = 0; i <= DM_TYPE_END; i++) {
		min_order[i] = DM_EMPTY;
		max_order[i] = DM_EMPTY;
	}

	do_gettimeofday(&after);
	mutex_unlock(&exynos_dm->lock);

	pre_time = (before.tv_sec - pre.tv_sec) * USEC_PER_SEC +
		(before.tv_usec - pre.tv_usec);
	time = (after.tv_sec - before.tv_sec) * USEC_PER_SEC +
		(after.tv_usec - before.tv_usec);

	exynos_ss_dm((int)dm_type, *target_freq, 3, pre_time, time);

	return 0;
}

static int dm_data_updater(enum exynos_dm_type dm_type)
{
	struct exynos_dm_data *dm;
	struct exynos_dm_constraint *constraint;
	struct list_head *constraint_list;
	int i;
	/* Initial min/max frequency is set to policy min/max frequency */
	u32 min_freq;
	u32 max_freq;

	dm = &exynos_dm->dm_data[dm_type];
	min_freq = dm->policy_min_freq;
	max_freq = dm->policy_max_freq;

	/* Check min/max constraint conditions */
	for (i = 0; i < DM_TYPE_END; i++) {
		if (!exynos_dm->dm_data[i].available)
			continue;

		constraint_list = get_min_constraint_list(&exynos_dm->dm_data[i]);
		if (list_empty(constraint_list))
			continue;
		list_for_each_entry(constraint, constraint_list, node) {
			if (constraint->constraint_dm_type == dm_type)
				min_freq = max(min_freq, constraint->min_freq);
		}
	}
	for (i = 0; i < DM_TYPE_END; i++) {
		if (!exynos_dm->dm_data[i].available)
			continue;

		constraint_list = get_max_constraint_list(&exynos_dm->dm_data[i]);
		if (list_empty(constraint_list))
			continue;
		list_for_each_entry(constraint, constraint_list, node) {
			if (constraint->constraint_dm_type == dm_type)
				max_freq = min(max_freq, constraint->max_freq);
		}
	}

	min_freq = max(min_freq, dm->gov_min_freq); //MIN freq should be checked with gov_min_freq
	update_min_max_freq(dm, min_freq, max_freq);

	return 0;
}

static int constraint_data_updater(enum exynos_dm_type dm_type, int cnt)
{
	struct exynos_dm_data *dm;
	struct exynos_dm_constraint *constraint;
	struct list_head *constraint_list;

	dm = &exynos_dm->dm_data[dm_type];

	/* Check dependent domains */
	constraint_checker_min(get_min_constraint_list(dm), dm->min_freq);

	if (!dm->constraint_checked)
		dm->constraint_checked += cnt;

	min_order[dm->constraint_checked] = dm_type;

	constraint_list = get_min_constraint_list(dm);
	if (list_empty(constraint_list))
		return 0;

	min_order[0] = 0;

	list_for_each_entry(constraint, constraint_list, node) {
		dm = &exynos_dm->dm_data[constraint->constraint_dm_type];
		dm_data_updater(dm->dm_type);

		dm->target_freq = dm->min_freq;
		if (dm->target_freq >= dm->max_freq)
			dm->target_freq = dm->max_freq;

		constraint_data_updater(dm->dm_type, cnt + 1);
	}

	return 0;
}

static int max_constraint_data_updater(enum exynos_dm_type dm_type, int cnt)
{
	struct exynos_dm_data *dm;
	struct exynos_dm_constraint *constraint;
	struct list_head *constraint_list;

	dm = &exynos_dm->dm_data[dm_type];

	/* Check dependent domains */
	constraint_checker_max(get_max_constraint_list(dm), dm->max_freq);

	if (!dm->constraint_checked)
		dm->constraint_checked += cnt;

	max_order[dm->constraint_checked] = dm_type;

	constraint_list = get_max_constraint_list(dm);
	if (list_empty(constraint_list))
		return 0;

	max_order[0] = 0;

	list_for_each_entry(constraint, constraint_list, node) {
		dm = &exynos_dm->dm_data[constraint->constraint_dm_type];
		dm_data_updater(dm->dm_type);

		dm->target_freq = dm->min_freq;
		if (dm->target_freq >= dm->max_freq)
			dm->target_freq = dm->max_freq;

		max_constraint_data_updater(dm->dm_type, cnt + 1);
	}

	return 0;
}

/*
 * Scaling Callback
 * Call callback function in each DVFS drivers to scaling frequency
 */
static int scaling_callback(enum dvfs_direction dir, unsigned int relation)
{
	struct exynos_dm_data *dm;
	int i;

	switch (dir) {
	case DOWN:
		if (min_order[0] == 0 && max_flag == false) {
			for (i = 1; i <= DM_TYPE_END; i++) {
				if (min_order[i] == DM_EMPTY)
					continue;

				dm = &exynos_dm->dm_data[min_order[i]];
				if (dm->constraint_checked) {
					if (dm->freq_scaler) {
						dm->freq_scaler(dm->dm_type, dm->target_freq, relation);
						dm->cur_freq = dm->target_freq;
					}
					dm->constraint_checked = 0;
				}
			}
		} else if (max_order[0] == 0 && max_flag == true) {
			for (i = DM_TYPE_END; i > 0; i--) {
				if (max_order[i] == DM_EMPTY)
					continue;

				dm = &exynos_dm->dm_data[max_order[i]];
				if (dm->constraint_checked) {
					if (dm->freq_scaler) {
						dm->freq_scaler(dm->dm_type, dm->target_freq, relation);
						dm->cur_freq = dm->target_freq;
					}
					dm->constraint_checked = 0;
				}
			}
		}
		break;
	case UP:
		if (min_order[0] == 0) {
			for (i = DM_TYPE_END; i > 0; i--) {
				if (min_order[i] == DM_EMPTY)
					continue;

				dm = &exynos_dm->dm_data[min_order[i]];
				if (dm->constraint_checked) {
					if (dm->freq_scaler) {
						dm->freq_scaler(dm->dm_type, dm->target_freq, relation);
						dm->cur_freq = dm->target_freq;
					}
					dm->constraint_checked = 0;
				}
			}
		} else if (max_order[0] == 0) {
			for (i = 1; i <= DM_TYPE_END; i++) {
				if (max_order[i] == DM_EMPTY)
					continue;

				dm = &exynos_dm->dm_data[max_order[i]];
				if (dm->constraint_checked) {
					if (dm->freq_scaler) {
						dm->freq_scaler(dm->dm_type, dm->target_freq, relation);
						dm->cur_freq = dm->target_freq;
					}
					dm->constraint_checked = 0;
				}
			}
		}
		break;
	default:
		break;
	}

	for (i = 1; i <= DM_TYPE_END; i++) {
		if (min_order[i] == DM_EMPTY)
			continue;

		dm = &exynos_dm->dm_data[min_order[i]];
		if (dm->constraint_checked) {
			if (dm->freq_scaler) {
				dm->freq_scaler(dm->dm_type, dm->target_freq, relation);
				dm->cur_freq = dm->target_freq;
			}
			dm->constraint_checked = 0;
		}
	}

	max_flag = false;

	return 0;
}

static void get_governor_min_freq(struct exynos_dm_data *dm_data, u32 *gov_min_freq)
{
	*gov_min_freq = dm_data->gov_min_freq;
}

static void get_min_max_freq(struct exynos_dm_data *dm_data, u32 *min_freq, u32 *max_freq)
{
	*min_freq = dm_data->min_freq;
	*max_freq = dm_data->max_freq;
}

static void update_min_max_freq(struct exynos_dm_data *dm_data, u32 min_freq, u32 max_freq)
{
	dm_data->min_freq = min_freq;
	dm_data->max_freq = max_freq;
}

static void get_policy_min_max_freq(struct exynos_dm_data *dm_data, u32 *min_freq, u32 *max_freq)
{
	*min_freq = dm_data->policy_min_freq;
	*max_freq = dm_data->policy_max_freq;
}

static void update_policy_min_max_freq(struct exynos_dm_data *dm_data, u32 min_freq, u32 max_freq)
{
	dm_data->policy_min_freq = min_freq;
	dm_data->policy_max_freq = max_freq;
}

static void get_current_freq(struct exynos_dm_data *dm_data, u32 *cur_freq)
{
	*cur_freq = dm_data->cur_freq;
}

static void get_target_freq(struct exynos_dm_data *dm_data, u32 *target_freq)
{
	*target_freq = dm_data->target_freq;
}

static int exynos_dm_suspend(struct device *dev)
{
	/* Suspend callback function might be registered if necessary */

	return 0;
}

static int exynos_dm_resume(struct device *dev)
{
	/* Resume callback function might be registered if necessary */

	return 0;
}

static int exynos_dm_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct exynos_dm_device *dm;

	dm = kzalloc(sizeof(struct exynos_dm_device), GFP_KERNEL);
	if (dm == NULL) {
		dev_err(&pdev->dev, "failed to allocate DVFS Manager device\n");
		ret = -ENOMEM;
		goto err_device;
	}

	dm->dev = &pdev->dev;

	mutex_init(&dm->lock);

	/* parsing devfreq dts data for exynos-dvfs-manager */
	ret = exynos_dm_parse_dt(dm->dev->of_node, dm);
	if (ret) {
		dev_err(dm->dev, "failed to parse private data\n");
		goto err_parse_dt;
	}

	print_available_dm_data(dm);

	ret = sysfs_create_group(&dm->dev->kobj, &exynos_dm_attr_group);
	if (ret)
		dev_warn(dm->dev, "failed create sysfs for DVFS Manager\n");

	exynos_dm = dm;
	platform_set_drvdata(pdev, dm);

	return 0;

err_parse_dt:
	mutex_destroy(&dm->lock);
	kfree(dm);
err_device:

	return ret;
}

static int exynos_dm_remove(struct platform_device *pdev)
{
	struct exynos_dm_device *dm = platform_get_drvdata(pdev);

	sysfs_remove_group(&dm->dev->kobj, &exynos_dm_attr_group);
	mutex_destroy(&dm->lock);
	kfree(dm);

	return 0;
}

static struct platform_device_id exynos_dm_driver_ids[] = {
	{
		.name		= EXYNOS_DM_MODULE_NAME,
	},
	{},
};
MODULE_DEVICE_TABLE(platform, exynos_dm_driver_ids);

static const struct of_device_id exynos_dm_match[] = {
	{
		.compatible	= "samsung,exynos-dvfs-manager",
	},
	{},
};
MODULE_DEVICE_TABLE(of, exynos_dm_match);

static const struct dev_pm_ops exynos_dm_pm_ops = {
	.suspend	= exynos_dm_suspend,
	.resume		= exynos_dm_resume,
};

static struct platform_driver exynos_dm_driver = {
	.probe		= exynos_dm_probe,
	.remove		= exynos_dm_remove,
	.id_table	= exynos_dm_driver_ids,
	.driver	= {
		.name	= EXYNOS_DM_MODULE_NAME,
		.owner	= THIS_MODULE,
		.pm	= &exynos_dm_pm_ops,
		.of_match_table = exynos_dm_match,
	},
};

static int __init exynos_dm_init(void)
{
	return platform_driver_register(&exynos_dm_driver);
}
subsys_initcall(exynos_dm_init);

static void __exit exynos_dm_exit(void)
{
	platform_driver_unregister(&exynos_dm_driver);
}
module_exit(exynos_dm_exit);

MODULE_AUTHOR("Taekki Kim <taekki.kim@samsung.com>");
MODULE_AUTHOR("Eunok Jo <eunok25.jo@samsung.com>");
MODULE_DESCRIPTION("Samsung EXYNOS SoC series DVFS Manager");
MODULE_LICENSE("GPL");
