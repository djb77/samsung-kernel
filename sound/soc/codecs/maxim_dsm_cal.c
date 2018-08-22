/*
 * maxim_dsm_cal.c -- Module for Rdc calibration
 *
 * Copyright 2014 Maxim Integrated Products
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
#include <linux/regmap.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/syscalls.h>
#include <linux/file.h>
#include <linux/fcntl.h>
#include <linux/power_supply.h>
#include <sound/maxim_dsm.h>
#include <sound/maxim_dsm_cal.h>

#define DEBUG_MAXIM_DSM_CAL
#ifdef DEBUG_MAXIM_DSM_CAL
#define dbg_maxdsm(format, args...)	\
pr_info("[MAXIM_DSM_CAL] %s: " format "\n", __func__, ## args)
#else
#define dbg_maxdsm(format, args...)
#endif /* DEBUG_MAXIM_DSM_CAL */

struct class *g_class;

struct maxim_dsm_cal *g_mdc;
static int maxdsm_cal_read_file(char *filename, char *data, size_t size)
{
	struct file *cal_filp;
	mm_segment_t old_fs = get_fs();
	int ret;

	set_fs(KERNEL_DS);
	cal_filp = filp_open(filename, O_RDONLY, 0660);
	if (IS_ERR(cal_filp)) {
		pr_err("%s: there is no dsm_cal file\n", __func__);
		set_fs(old_fs);
		return -EEXIST;
	}
	ret = vfs_read(cal_filp, data, size, &cal_filp->f_pos);
	if (ret != size) {
		pr_err("%s: can't read dsm calibration value to file\n",
				__func__);
		ret = -EIO;
	}
	filp_close(cal_filp, current->files);
	set_fs(old_fs);

	return ret;
}

static int maxdsm_cal_write_file(char *filename, char *data, size_t size)
{
	struct file *cal_filp;
	mm_segment_t old_fs = get_fs();
	int ret;

	set_fs(KERNEL_DS);
	cal_filp = filp_open(filename,
			O_CREAT | O_TRUNC | O_WRONLY, 0660);
	if (IS_ERR(cal_filp)) {
		pr_err("%s: Can't open calibration file\n", __func__);
		set_fs(old_fs);
		ret = PTR_ERR(cal_filp);
		return ret;
	}
	ret = vfs_write(cal_filp, data, size, &cal_filp->f_pos);
	if (ret != size) {
		pr_err("%s: can't write dsm calibration value to file\n",
				__func__);
		ret = -EIO;
	}
	filp_close(cal_filp, current->files);
	set_fs(old_fs);

	return ret;
}

struct class *maxdsm_cal_get_class(void)
{
	return g_mdc->class;
}
EXPORT_SYMBOL_GPL(maxdsm_cal_get_class);

struct regmap *maxdsm_cal_set_regmap(
		struct regmap *regmap)
{
	g_mdc->regmap = regmap;
#if defined(CONFIG_SND_SOC_MAXIM_DSM) && defined(USE_DSM_LOG)
	maxdsm_set_regmap(g_mdc->regmap);
#endif /* CONFIG_SND_SOC_MAXIM_DSM && USE_DSM_LOG */
	return g_mdc->regmap;
}
EXPORT_SYMBOL_GPL(maxdsm_cal_set_regmap);

int maxdsm_cal_set_temp(int value)
{
	char data[12];
	int ret;

	memset(data, 0x00, sizeof(data));
	sprintf(data, "%x", value);
	ret = maxdsm_cal_write_file(FILEPATH_TEMP_CAL,
			data, sizeof(data));
	g_mdc->values.temp = ret < 0 ? ret : value;

	return ret;
}
EXPORT_SYMBOL_GPL(maxdsm_cal_set_temp);

int maxdsm_cal_get_temp(int *value)
{
	char data[12];
	int ret;

	memset(data, 0x00, sizeof(data));
	ret = maxdsm_cal_read_file(FILEPATH_TEMP_CAL,
			data, sizeof(data));
	if (ret < 0)
		g_mdc->values.temp = ret;
	else
		ret = kstrtos32(data, 16, value);

	return ret;
}
EXPORT_SYMBOL_GPL(maxdsm_cal_get_temp);

int maxdsm_cal_get_safe_mode(void)
{
	return g_mdc->values_thermal_safe_mode.enable;
}
EXPORT_SYMBOL_GPL(maxdsm_cal_get_safe_mode);

uint32_t maxdsm_cal_get_thermal_min_gain(void)
{
	return g_mdc->values_thermal_safe_mode.thermal_min_gain;
}
EXPORT_SYMBOL_GPL(maxdsm_cal_get_thermal_min_gain);

void maxdsm_cal_set_thermal_min_gain(uint32_t thermal_min_gain)
{
	g_mdc->values_thermal_safe_mode.thermal_min_gain = thermal_min_gain;
}
EXPORT_SYMBOL_GPL(maxdsm_cal_set_thermal_min_gain);


int maxdsm_cal_set_rdc(int value)
{
	char data[12];
	int ret;

	memset(data, 0x00, sizeof(data));
	sprintf(data, "%x", value);
	ret = maxdsm_cal_write_file(FILEPATH_RDC_CAL,
			data, sizeof(data));
	g_mdc->values.rdc = ret < 0 ? ret : value;

	return ret;
}
EXPORT_SYMBOL_GPL(maxdsm_cal_set_rdc);

int maxdsm_cal_set_rdc_r(int value)
{
	char data[12];
	int ret;

	memset(data, 0x00, sizeof(data));
	sprintf(data, "%x", value);
	ret = maxdsm_cal_write_file(FILEPATH_RDC_CAL_R,
			data, sizeof(data));
	g_mdc->values.rdc_r = ret < 0 ? ret : value;

	return ret;
}
EXPORT_SYMBOL_GPL(maxdsm_cal_set_rdc_r);

int maxdsm_cal_get_rdc(int *value)
{
	char data[12];
	int ret;

	memset(data, 0x00, sizeof(data));
	ret = maxdsm_cal_read_file(FILEPATH_RDC_CAL,
			data, sizeof(data));
	if (ret < 0)
		g_mdc->values.rdc = ret;
	else
		ret = kstrtos32(data, 16, value);

	return ret;
}
EXPORT_SYMBOL_GPL(maxdsm_cal_get_rdc);

int maxdsm_cal_get_rdc_r(int *value)
{
	char data[12];
	int ret;

	memset(data, 0x00, sizeof(data));
	ret = maxdsm_cal_read_file(FILEPATH_RDC_CAL_R,
			data, sizeof(data));
	if (ret < 0)
		g_mdc->values.rdc_r = ret;
	else
		ret = kstrtos32(data, 16, value);

	return ret;
}
EXPORT_SYMBOL_GPL(maxdsm_cal_get_rdc_r);

static int maxdsm_cal_regmap_write(struct regmap *regmap,
		unsigned int reg,
		unsigned int val)
{
	return regmap ?
		regmap_write(regmap, reg, val) : -ENXIO;
}

static int maxdsm_cal_regmap_read(struct regmap *regmap,
		unsigned int reg,
		unsigned int *val)
{
	int ret;

	*val = 0;
	ret = regmap ?
		regmap_read(regmap, reg, val) : -ENXIO;
	if (ret < 0)
		*val = 0;

	return ret;
}

static int maxdsm_cal_start_calibration(
		struct maxim_dsm_cal *mdc)
{
	int ret = 0;
	uint32_t feature_en;
	uint32_t version = 0;

#ifdef CONFIG_SND_SOC_MAXIM_DSM
	mdc->platform_type = maxdsm_get_platform_type();
	version = maxdsm_get_version();
#else
	mdc->platform_type = 0;
	version = 0;
#endif /* CONFIG_SND_SOC_MAXIM_DSM */
	dbg_maxdsm("platform_type=%d, version=%d", mdc->platform_type, version);

	switch (mdc->platform_type) {
	case PLATFORM_TYPE_A:
		maxdsm_cal_regmap_read(mdc->regmap,
				ADDR_FEATURE_ENABLE,
				&feature_en);

		mdc->info.feature_en = feature_en;

		/* remove SPT bit */
		feature_en &= ~0x80;

		/* enable Rdc caliration bit */
		feature_en |= 0x20;

		maxdsm_cal_regmap_write(mdc->regmap,
				ADDR_FEATURE_ENABLE,
				feature_en);

		if (version == VERSION_4_0_A_S) {
			maxdsm_cal_regmap_write(mdc->regmap,
					ADDR_FEATURE_ENABLE+DSM_4_0_LSI_STEREO_OFFSET,
					feature_en);
		}
		break;
	case PLATFORM_TYPE_B:
#ifdef CONFIG_SND_SOC_MAXIM_DSM
		if (maxdsm_set_feature_en(1, FLAG_WRITE_FEATURE_ONLY))
			ret = -EAGAIN;
#endif /* CONFIG_SND_SOC_MAXIM_DSM */
		break;
	case PLATFORM_TYPE_C:
		if (maxdsm_set_cal_mode(1))
			ret = -EAGAIN;
		break;
	default:
		break;
	}

	mdc->info.previous_jiffies = jiffies;

	return ret;
}

static int maxdsm_start_v_validation(
		struct maxim_dsm_cal *mdc)
{
	int ret = 0;
	uint32_t version = 0;

#ifdef CONFIG_SND_SOC_MAXIM_DSM
	mdc->platform_type = maxdsm_get_platform_type();
	version = maxdsm_get_version();
#else
	mdc->platform_type = 0;
	version = 0;
#endif /* CONFIG_SND_SOC_MAXIM_DSM */
	dbg_maxdsm("platform_type=%d, version=%d", mdc->platform_type, version);

	switch (mdc->platform_type) {
	case PLATFORM_TYPE_A:

		break;
	case PLATFORM_TYPE_B:
#ifdef CONFIG_SND_SOC_MAXIM_DSM
		if (maxdsm_set_feature_en(1, PARAM_WRITE_V_VALIDATION))
			ret = -EAGAIN;
#endif /* CONFIG_SND_SOC_MAXIM_DSM */
		break;
	case PLATFORM_TYPE_C:
		if (maxdsm_set_v_validation_mode(1))
			ret = -EAGAIN;
		break;
	default:
		break;
	}

	return ret;
}

static int maxdsm_start_amp_screen_validation(
		struct maxim_dsm_cal *mdc)
{
	int ret = 0;
	uint32_t version = 0;

#ifdef CONFIG_SND_SOC_MAXIM_DSM
	mdc->platform_type = maxdsm_get_platform_type();
	version = maxdsm_get_version();
#else
	mdc->platform_type = 0;
	version = 0;
#endif /* CONFIG_SND_SOC_MAXIM_DSM */
	dbg_maxdsm("platform_type=%d, version=%d", mdc->platform_type, version);

	max98512_dc_blocker_enable(0);
	max98512_boost_bypass(0x2);

	switch (mdc->platform_type) {
	case PLATFORM_TYPE_A:
		break;
	case PLATFORM_TYPE_B:
#ifdef CONFIG_SND_SOC_MAXIM_DSM
		if (maxdsm_set_amp_screen_mode(200))
			ret = -EAGAIN;
#endif /* CONFIG_SND_SOC_MAXIM_DSM */
		break;
	case PLATFORM_TYPE_C:
		if (maxdsm_set_amp_screen_validation_mode(1))
			ret = -EAGAIN;
		break;
	default:
		break;
	}
	mdc->values_v_validation.boost_bypass = 1;
	mdc->info_v_validation.screen_remaining = mdc->info_v_validation.screen_duration;
	mdc->info_v_validation.screen_previous_jiffies = jiffies;

	return ret;
}

static uint32_t maxdsm_cal_read_dcresistance(
		struct maxim_dsm_cal *mdc)
{
	uint32_t dcresistance = 0;

	switch (mdc->platform_type) {
	case PLATFORM_TYPE_A:
		maxdsm_cal_regmap_read(mdc->regmap,
				ADDR_RDC,
				&dcresistance);
		break;
	case PLATFORM_TYPE_B:
#ifdef CONFIG_SND_SOC_MAXIM_DSM
		dcresistance = maxdsm_get_dcresistance();
#endif /* CONFIG_SND_SOC_MAXIM_DSM */
		break;
	case PLATFORM_TYPE_C:
		dcresistance = maxdsm_get_dcresistance();
		break;
	default:
		break;
	}

	return dcresistance;
}

static uint32_t maxdsm_cal_read_dcresistance_r(
		struct maxim_dsm_cal *mdc)
{
	uint32_t dcresistance = 0;

	switch (mdc->platform_type) {
	case PLATFORM_TYPE_A:
		maxdsm_cal_regmap_read(mdc->regmap,
			ADDR_RDC+DSM_4_0_LSI_STEREO_OFFSET,
			&dcresistance);
		break;
	case PLATFORM_TYPE_B:
		dcresistance = maxdsm_get_dcresistance_r();
		break;
	case PLATFORM_TYPE_C:
		dcresistance = maxdsm_get_dcresistance_r();
		break;
	default:
		break;
	}

	return dcresistance;
}

static int maxdsm_cal_end_calibration(
		struct maxim_dsm_cal *mdc)
{
	int version, ret = 0;

#ifdef CONFIG_SND_SOC_MAXIM_DSM
	version = maxdsm_get_version();
#else
	version = 0;
#endif /* CONFIG_SND_SOC_MAXIM_DSM */

	switch (mdc->platform_type) {
	case PLATFORM_TYPE_A:
		maxdsm_cal_regmap_write(mdc->regmap,
				ADDR_FEATURE_ENABLE,
				mdc->info.feature_en);
		if (version == VERSION_4_0_A_S) {
			maxdsm_cal_regmap_write(mdc->regmap,
				ADDR_FEATURE_ENABLE+DSM_4_0_LSI_STEREO_OFFSET,
				mdc->info.feature_en);
		}
		break;
	case PLATFORM_TYPE_B:
#ifdef CONFIG_SND_SOC_MAXIM_DSM
		if (maxdsm_set_feature_en(0, FLAG_WRITE_FEATURE_ONLY))
			ret = -EAGAIN;
#endif /* CONFIG_SND_SOC_MAXIM_DSM */
		break;
	case PLATFORM_TYPE_C:
		if (maxdsm_set_cal_mode(0))
			ret = -EAGAIN;
		break;
	default:
		break;
	}

	return ret;
}

static int maxdsm_v_validation_end(
		struct maxim_dsm_cal *mdc)
{
	int version, ret = 0;

#ifdef CONFIG_SND_SOC_MAXIM_DSM
	version = maxdsm_get_version();
#else
	version = 0;
#endif /* CONFIG_SND_SOC_MAXIM_DSM */

	switch (mdc->platform_type) {
	case PLATFORM_TYPE_A:
		break;
	case PLATFORM_TYPE_B:
#ifdef CONFIG_SND_SOC_MAXIM_DSM
		if (maxdsm_set_feature_en(0, PARAM_WRITE_V_VALIDATION))
			ret = -EAGAIN;
#endif /* CONFIG_SND_SOC_MAXIM_DSM */
		break;
	case PLATFORM_TYPE_C:
		if (maxdsm_set_v_validation_mode(0))
			ret = -EAGAIN;
		break;
	default:
		break;
	}

	return ret;
}

static int maxdsm_amp_screen_validation_end(
		struct maxim_dsm_cal *mdc)
{
	int version, ret = 0;

	dbg_maxdsm("maxdsm_amp_screen_validation_end");

#ifdef CONFIG_SND_SOC_MAXIM_DSM
	version = maxdsm_get_version();
#else
	version = 0;
#endif /* CONFIG_SND_SOC_MAXIM_DSM */

	switch (mdc->platform_type) {
	case PLATFORM_TYPE_A:
		break;
	case PLATFORM_TYPE_B:
#ifdef CONFIG_SND_SOC_MAXIM_DSM
		if (maxdsm_set_amp_screen_mode(100))
			ret = -EAGAIN;
#endif /* CONFIG_SND_SOC_MAXIM_DSM */
		break;
	case PLATFORM_TYPE_C:
		if (maxdsm_set_amp_screen_validation_mode(0))
			ret = -EAGAIN;
		break;
	default:
		break;
	}

	return ret;
}

int maxdsm_cal_get_temp_from_power_supply(void)
{
	union power_supply_propval value = {0,};
	struct power_supply *psy;
	int temperature = 0;

	psy = power_supply_get_by_name("battery");
	if (!psy) {
		pr_err("%s: Failed to get psy\n", __func__);
		temperature = 23 * 10;
	} else {
		if (psy->desc) {
			psy->desc->get_property(psy,
						POWER_SUPPLY_PROP_TEMP,
						&value);
			temperature = value.intval;
		} else {
			dbg_maxdsm("The description of power_supply is NULL");
		}
	}
	dbg_maxdsm("temperature=%d", temperature);

	return temperature;
}
EXPORT_SYMBOL_GPL(maxdsm_cal_get_temp_from_power_supply);

static void maxdsm_cal_completed(struct maxim_dsm_cal *mdc)
{
#ifdef WRITE_SPEAKER_CAL_VALUE_AT_KERNEL
	char rdc[12] = {0,};
	char rdc_r[12] = {0,};
	char temp[12] = {0,};
#endif
	int ret, stereo = 0;

#ifdef CONFIG_SND_SOC_MAXIM_DSM
	stereo = maxdsm_is_stereo();
#else
	stereo = 0;
#endif

	ret = maxdsm_cal_end_calibration(mdc);
	if (ret) {
		pr_err("%s: A error occurred in finishing calibration\n",
		       __func__);
		return;
	}

	/* We try to get ambient temp by using power supply core */
	mdc->values.temp = maxdsm_cal_get_temp_from_power_supply();

	if (stereo)
		dbg_maxdsm("temp=%d rdc=0x%08x, rdc_r=0x%08x",
			mdc->values.temp, mdc->values.rdc, mdc->values.rdc_r);
	else
		dbg_maxdsm("temp=%d rdc=0x%08x",
			mdc->values.temp, mdc->values.rdc);

	if (mdc->platform_type == PLATFORM_TYPE_C) {
		mdc->values.rdc = (mdc->values.rdc >> 8);
		mdc->values.rdc_r = (mdc->values.rdc_r >> 8);
	}

#ifdef WRITE_SPEAKER_CAL_VALUE_AT_KERNEL
	sprintf(rdc, "%x", mdc->values.rdc);
	sprintf(rdc_r, "%x", mdc->values.rdc_r);
	sprintf(temp, "%x",
			mdc->values.temp < 0 ? 23 * 10 : mdc->values.temp);

	ret = maxdsm_cal_write_file(
			FILEPATH_TEMP_CAL, temp, sizeof(temp));
	if (ret < 0)
		mdc->values.temp = ret;

	ret = maxdsm_cal_write_file(
			FILEPATH_RDC_CAL, rdc, sizeof(rdc));
	if (ret < 0)
		mdc->values.rdc = ret;

	if (stereo) {
		ret = maxdsm_cal_write_file(
				FILEPATH_RDC_CAL_R, rdc_r, sizeof(rdc_r));
		if (ret < 0)
			mdc->values.rdc_r = ret;
	}
#endif

	mdc->values.status = 0;

	if (stereo)
		dbg_maxdsm("temp=%d rdc=0x%08x, rdc_r=0x%08x",
			mdc->values.temp, mdc->values.rdc, mdc->values.rdc_r);
	else
		dbg_maxdsm("temp=%d rdc=0x%08x",
			mdc->values.temp, mdc->values.rdc);
}

static void maxdsm_cal_work(struct work_struct *work)
{
	struct maxim_dsm_cal *mdc;
	unsigned int dcresistance, dcresistance_r, stereo = 0;
	unsigned long diff;

	mdc = container_of(work, struct maxim_dsm_cal, work.work);

	mutex_lock(&mdc->mutex);

#ifdef CONFIG_SND_SOC_MAXIM_DSM
	stereo = maxdsm_is_stereo();
#else
	stereo = 0;
#endif

	dcresistance = maxdsm_cal_read_dcresistance(mdc);
	if (stereo) {
		dcresistance_r = maxdsm_cal_read_dcresistance_r(mdc);
		if (dcresistance
			&& !(mdc->info.min > dcresistance || mdc->info.max < dcresistance)
			&& ((mdc->info.duration - mdc->info.remaining) > mdc->info.ignored_t)) {
			mdc->values.avg += dcresistance;
			if (dcresistance_r)
				mdc->values.avg_r += dcresistance_r;
			mdc->values.count++;
		}
	} else {
		if (dcresistance
			&& !(mdc->info.min > dcresistance || mdc->info.max < dcresistance)
			&& ((mdc->info.duration - mdc->info.remaining) > mdc->info.ignored_t)) {
			mdc->values.avg += dcresistance;
			mdc->values.count++;
		}
	}

	diff = jiffies - mdc->info.previous_jiffies;
	mdc->info.remaining -= jiffies_to_msecs(diff);

	if (stereo)
		dbg_maxdsm("dcresistance=0x%08x dcresistance_r=0x%08x remaining=%d duration=%d",
			dcresistance, dcresistance_r,
			mdc->info.remaining,
			mdc->info.duration);
	else
		dbg_maxdsm("dcresistance=0x%08x remaining=%d duration=%d",
			dcresistance,
			mdc->info.remaining,
			mdc->info.duration);

	if (mdc->info.remaining > 0
			&& mdc->values.status) {
		mdc->info.previous_jiffies = jiffies;
		queue_delayed_work(mdc->wq,
				&mdc->work,
				msecs_to_jiffies(mdc->info.interval));
	} else {
		mdc->values.count > 0 ?
			do_div(mdc->values.avg, mdc->values.count) : 0;
		if (stereo) {
			mdc->values.count > 0 ?
				do_div(mdc->values.avg_r, mdc->values.count) : 0;
		}
		mdc->values.rdc = mdc->values.avg;
		mdc->values.rdc_r = mdc->values.avg_r;
		maxdsm_cal_completed(mdc);
	}

	mutex_unlock(&mdc->mutex);
}

static void maxdsm_v_validation_work(struct work_struct *work)
{
	struct maxim_dsm_cal *mdc;
	uint32_t v_validation = 0, v_validation_r;
	int ret = 0;

	mdc = container_of(work, struct maxim_dsm_cal, work_v_validation.work);

	mutex_lock(&mdc->mutex);

	ret = maxdsm_v_validation_end(mdc);
	if (ret) {
		pr_err("%s: A error occurred in finishing v validation\n",
				__func__);
		mutex_unlock(&mdc->mutex);
		return;
	}
	v_validation = maxdsm_get_v_validation();
	dbg_maxdsm("v_validation=%d ", v_validation);
	mdc->values_v_validation.v_validation = v_validation;
	if (maxdsm_is_stereo()) {
		v_validation_r = maxdsm_get_v_validation_r();
		dbg_maxdsm("v_validation_r=%d ", v_validation_r);
		mdc->values_v_validation.v_validation_r = v_validation_r;
	}

	mdc->values_v_validation.status = 0;
	mutex_unlock(&mdc->mutex);
}

static void maxdsm_amp_screen_validation_work(struct work_struct *work)
{
	struct maxim_dsm_cal *mdc;
	unsigned long diff;
	uint32_t amp_screen = 0, amp_screen_r;
	int ret = 0;

	mdc = container_of(work, struct maxim_dsm_cal, work_amp_screen_validation.work);

	mutex_lock(&mdc->mutex);

	diff = jiffies - mdc->info_v_validation.screen_previous_jiffies;
	mdc->info_v_validation.screen_remaining -= jiffies_to_msecs(diff);

	dbg_maxdsm("boost_bypass %d", mdc->values_v_validation.boost_bypass);
	max98512_boost_bypass(mdc->values_v_validation.boost_bypass);
	mdc->values_v_validation.boost_bypass = !mdc->values_v_validation.boost_bypass;

	dbg_maxdsm("screen remaining=%d screen duration=%d",
		mdc->info_v_validation.screen_remaining, mdc->info_v_validation.screen_duration);

	if (mdc->info_v_validation.screen_remaining > 0
			&& mdc->values_v_validation.amp_screen_status) {
		mdc->info_v_validation.screen_previous_jiffies = jiffies;
		queue_delayed_work(mdc->wq,
				&mdc->work_amp_screen_validation,
				msecs_to_jiffies(mdc->info_v_validation.screen_interval));
	} else {
		amp_screen = maxdsm_get_amp_screen_validation();
		dbg_maxdsm("amp_screen=%d ", amp_screen);
		mdc->values_v_validation.amp_screen_validation = amp_screen;
		if (maxdsm_is_stereo()) {
			amp_screen_r = maxdsm_get_amp_screen_validation_r();
			dbg_maxdsm("amp_screen_r=%d ", amp_screen_r);
			mdc->values_v_validation.amp_screen_validation_r = amp_screen_r;
		}
		ret = maxdsm_amp_screen_validation_end(mdc);
		max98512_dc_blocker_enable(1);
		max98512_boost_bypass(0x3);
		mdc->values_v_validation.amp_screen_status = 0;
	}

	mutex_unlock(&mdc->mutex);
}

static int maxdsm_cal_check(struct maxim_dsm_cal *mdc, int action)
{
	int ret = 0;

	if (delayed_work_pending(&mdc->work))
		cancel_delayed_work(&mdc->work);

	if (action) {
		mdc->info.remaining = mdc->info.duration;
		mdc->values.count = mdc->values.avg = 0;
		mdc->values.avg_r = 0;
		ret = maxdsm_cal_start_calibration(mdc);
		if (ret)
			return ret;
		queue_delayed_work(mdc->wq, &mdc->work, 1);
	}

	return ret;
}

static int maxdsm_v_validation_check(
		struct maxim_dsm_cal *mdc, int action)
{
	int ret = 0;

	if (delayed_work_pending(&mdc->work_v_validation))
		cancel_delayed_work(&mdc->work_v_validation);

	if (action) {
		ret = maxdsm_start_v_validation(mdc);
		if (ret)
			return ret;
		queue_delayed_work(mdc->wq,
				&mdc->work_v_validation,
				msecs_to_jiffies(mdc->info_v_validation.duration));
	}

	return ret;
}

static int maxdsm_amp_screen_validation_check(
		struct maxim_dsm_cal *mdc, int action)
{
	int ret = 0;

	if (delayed_work_pending(&mdc->work_amp_screen_validation))
		cancel_delayed_work(&mdc->work_amp_screen_validation);

	if (action) {
		ret = maxdsm_start_amp_screen_validation(mdc);
		if (ret)
			return ret;
		queue_delayed_work(mdc->wq,
				&mdc->work_amp_screen_validation,
				msecs_to_jiffies(10));
	}
	return ret;
}

static ssize_t maxdsm_cal_min_show(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	return sprintf(buf, "%d", g_mdc->info.min);
}

static ssize_t maxdsm_cal_min_store(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t size)
{
	if (kstrtou32(buf, 0, &g_mdc->info.min))
		dev_err(dev, "%s: Failed converting from str to u32.\n",
			__func__);
	return size;
}
static DEVICE_ATTR(min, 0664,
		   maxdsm_cal_min_show, maxdsm_cal_min_store);

static ssize_t maxdsm_cal_max_show(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	return sprintf(buf, "%d", g_mdc->info.max);
}

static ssize_t maxdsm_cal_max_store(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t size)
{
	if (kstrtou32(buf, 0, &g_mdc->info.max))
		dev_err(dev,
			"%s: Failed converting from str to u32.\n", __func__);
	return size;
}
static DEVICE_ATTR(max, 0664,
		   maxdsm_cal_max_show, maxdsm_cal_max_store);

static ssize_t maxdsm_cal_duration_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return sprintf(buf, "%d", g_mdc->info.duration);
}

static ssize_t maxdsm_cal_duration_store(struct device *dev,
					 struct device_attribute *attr,
					 const char *buf, size_t size)
{
	if (kstrtou32(buf, 0, &g_mdc->info.duration))
		dev_err(dev, "%s: Failed converting from str to u32.\n",
			__func__);
	return size;
}
static DEVICE_ATTR(duration, 0664,
		   maxdsm_cal_duration_show, maxdsm_cal_duration_store);

static ssize_t maxdsm_cal_v_duration_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d", g_mdc->info_v_validation.duration);
}

static ssize_t maxdsm_cal_v_duration_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	if (kstrtou32(buf, 0, &g_mdc->info_v_validation.duration))
		dev_err(dev,
			"%s: Failed converting from str to u32.\n", __func__);
	return size;
}
static DEVICE_ATTR(v_duration, 0664,
		   maxdsm_cal_v_duration_show, maxdsm_cal_v_duration_store);

static ssize_t maxdsm_cal_screen_duration_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d", g_mdc->info_v_validation.screen_duration);
}

static ssize_t maxdsm_cal_screen_duration_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	if (kstrtou32(buf, 0, &g_mdc->info_v_validation.screen_duration))
		dev_err(dev,
			"%s: Failed converting from str to u32.\n", __func__);
	return size;
}
static DEVICE_ATTR(screen_duration, 0664,
		   maxdsm_cal_screen_duration_show, maxdsm_cal_screen_duration_store);

static ssize_t maxdsm_cal_rdc_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	char rdc[12] = {0,};
	int ret;

	dbg_maxdsm("temp = %d, rdc=0x%08x",
		g_mdc->values.temp, g_mdc->values.rdc);

	if (g_mdc->values.rdc == 0xFFFFFFFF) {
		ret = maxdsm_cal_read_file(FILEPATH_RDC_CAL,
					   rdc, sizeof(rdc));
		if (ret < 0)
			g_mdc->values.rdc = ret;
		else
			ret = kstrtos32(rdc, 16, &g_mdc->values.rdc);
	}

	return sprintf(buf, "%x", g_mdc->values.rdc);
}

static ssize_t maxdsm_cal_rdc_store(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t size)
{
	char rdc[12] = {0,};
	int ret;

	if (kstrtos32(buf, 0, &g_mdc->values.rdc))
		dev_err(dev, "%s: Failed converting from str to u32.\n",
			__func__);
	else {
		sprintf(rdc, "%x", g_mdc->values.rdc);
		ret = maxdsm_cal_write_file(FILEPATH_RDC_CAL,
					    rdc, sizeof(rdc));
		if (ret < 0)
			g_mdc->values.rdc = ret;
	}

	return size;
}
static DEVICE_ATTR(rdc, 0664,
		   maxdsm_cal_rdc_show, maxdsm_cal_rdc_store);

static ssize_t maxdsm_cal_rdc_r_show(struct device *dev,
				     struct device_attribute *attr,
				     char *buf)
{
	char rdc_r[12] = {0,};
	int ret;

	dbg_maxdsm("temp = %d, rdc_r = 0x%08x",
		g_mdc->values.temp, g_mdc->values.rdc_r);

	if (g_mdc->values.rdc_r == 0xFFFFFFFF) {
		ret = maxdsm_cal_read_file(FILEPATH_RDC_CAL_R,
					   rdc_r, sizeof(rdc_r));
		if (ret < 0)
			g_mdc->values.rdc_r = ret;
		else
			ret = kstrtos32(rdc_r, 16, &g_mdc->values.rdc_r);
	}

	return sprintf(buf, "%x", g_mdc->values.rdc_r);
}

static ssize_t maxdsm_cal_rdc_r_store(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t size)
{
	char rdc_r[12] = {0,};
	int ret;

	if (kstrtos32(buf, 0, &g_mdc->values.rdc_r))
		dev_err(dev, "%s: Failed converting from str to u32.\n",
			__func__);
	else {
		sprintf(rdc_r, "%x", g_mdc->values.rdc_r);
		ret = maxdsm_cal_write_file(FILEPATH_RDC_CAL_R,
					    rdc_r, sizeof(rdc_r));
		if (ret < 0)
			g_mdc->values.rdc_r = ret;
	}

	return size;
}
static DEVICE_ATTR(rdc_r, 0664,
		   maxdsm_cal_rdc_r_show, maxdsm_cal_rdc_r_store);

static ssize_t maxdsm_cal_temp_show(struct device *dev,
				    struct device_attribute *attr,
				    char *buf)
{
	char temp[12] = {0,};
	int ret;

	if (g_mdc->values.temp == 0xFFFFFFFF) {
		ret = maxdsm_cal_read_file(FILEPATH_TEMP_CAL,
					   temp, sizeof(temp));
		if (ret < 0)
			g_mdc->values.temp = ret;
		else
			ret = kstrtos32(temp, 16, &g_mdc->values.temp);
	}

	return sprintf(buf, "%x", g_mdc->values.temp);
}

static ssize_t maxdsm_cal_temp_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t size)
{
	char temp[12] = {0,};
	int ret;

	if (kstrtos32(buf, 0, &g_mdc->values.temp))
		dev_err(dev, "%s: Failed converting from str to u32.\n",
			__func__);
	else {
		sprintf(temp, "%x", g_mdc->values.temp);
		ret = maxdsm_cal_write_file(FILEPATH_TEMP_CAL,
					    temp, sizeof(temp));
		if (ret < 0)
			g_mdc->values.temp = ret;
	}

	return size;
}
static DEVICE_ATTR(temp, 0664,
		   maxdsm_cal_temp_show, maxdsm_cal_temp_store);

static ssize_t maxdsm_cal_interval_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return sprintf(buf, "%d", g_mdc->info.interval);
}

static ssize_t maxdsm_cal_interval_store(struct device *dev,
					 struct device_attribute *attr,
					 const char *buf, size_t size)
{
	if (kstrtou32(buf, 0, &g_mdc->info.interval))
		dev_err(dev, "%s: Failed converting from str to u32.\n",
			__func__);
	return size;
}
static DEVICE_ATTR(interval, 0664,
		   maxdsm_cal_interval_show, maxdsm_cal_interval_store);

static ssize_t maxdsm_cal_amp_screen_interval_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return sprintf(buf, "%d", g_mdc->info_v_validation.screen_interval);
}

static ssize_t maxdsm_cal_amp_screen_interval_store(struct device *dev,
					 struct device_attribute *attr,
					 const char *buf, size_t size)
{
	if (kstrtou32(buf, 0, &g_mdc->info_v_validation.screen_interval))
		dev_err(dev, "%s: Failed converting from str to u32.\n",
			__func__);
	return size;
}
static DEVICE_ATTR(amp_screen_interval, 0664,
		   maxdsm_cal_amp_screen_interval_show, maxdsm_cal_amp_screen_interval_store);

static ssize_t maxdsm_cal_ignored_t_show(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	return sprintf(buf, "%d", g_mdc->info.ignored_t);
}

static ssize_t maxdsm_cal_ignored_t_store(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t size)
{
	if (kstrtou32(buf, 0, &g_mdc->info.ignored_t))
		dev_err(dev, "%s: Failed converting from str to u32.\n",
			__func__);
	return size;
}
static DEVICE_ATTR(ignored_t, 0664,
		   maxdsm_cal_ignored_t_show, maxdsm_cal_ignored_t_store);

static ssize_t maxdsm_cal_status_show(struct device *dev,
				      struct device_attribute *attr,
				      char *buf)
{
	return sprintf(buf, "%s\n",
		       g_mdc->values.status ? "Enabled" : "Disabled");
}

static ssize_t maxdsm_cal_status_store(struct device *dev,
				       struct device_attribute *attr,
				       const char *buf, size_t size)
{
	int status = 0;

	if (!kstrtou32(buf, 0, &status)) {
		status = status > 0 ? 1 : 0;
		if (status == g_mdc->values.status) {
			dbg_maxdsm("Already run. It will be ignored.");
		} else {
			mutex_lock(&g_mdc->mutex);
			g_mdc->values.status = status;
			mutex_unlock(&g_mdc->mutex);
			if (maxdsm_cal_check(g_mdc, status)) {
				pr_err("%s: The codec was connected?\n",
				       __func__);
				mutex_lock(&g_mdc->mutex);
				g_mdc->values.status = 0;
				mutex_unlock(&g_mdc->mutex);
			}
		}
	}

	return size;
}
static DEVICE_ATTR(status, 0664,
		   maxdsm_cal_status_show, maxdsm_cal_status_store);

static ssize_t maxdsm_cal_v_validation_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	/* 0 : no-implementation 1 : success 0xCC(204) : fail */
	return snprintf(buf, PAGE_SIZE, "%d",
			g_mdc->values_v_validation.v_validation);
}
static DEVICE_ATTR(v_validation, 0444, maxdsm_cal_v_validation_show, NULL);

static ssize_t maxdsm_cal_v_validation_r_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	/* 0 : no-implementation 1 : success 0xCC(204) : fail */
	return snprintf(buf, PAGE_SIZE, "%d",
			g_mdc->values_v_validation.v_validation_r);
}
static DEVICE_ATTR(v_validation_r, 0444, maxdsm_cal_v_validation_r_show, NULL);

static ssize_t maxdsm_cal_amp_screen_validation_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	/* 0 : fail 1 : success */
	return snprintf(buf, PAGE_SIZE, "%d",
			g_mdc->values_v_validation.amp_screen_validation);
}
static DEVICE_ATTR(amp_screen_ratio, 0444,
		   maxdsm_cal_amp_screen_validation_show, NULL);

static ssize_t maxdsm_cal_amp_screen_validation_r_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	/* 0 : fail 1 : success */
	return snprintf(buf, PAGE_SIZE, "%d",
			g_mdc->values_v_validation.amp_screen_validation_r);
}
static DEVICE_ATTR(amp_screen_ratio_r, 0444,
		   maxdsm_cal_amp_screen_validation_r_show, NULL);

static ssize_t maxdsm_cal_v_status_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n",
			g_mdc->values_v_validation.status ?
			"Enabled" : "Disabled");
}

static ssize_t maxdsm_cal_v_status_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int status = 0;

	if (!kstrtou32(buf, 0, &status)) {
		status = status > 0 ? 1 : 0;
		if (status == g_mdc->values_v_validation.status) {
			dbg_maxdsm("Already run. It will be ignored.");
		} else {
			mutex_lock(&g_mdc->mutex);
			g_mdc->values_v_validation.status = status;
			mutex_unlock(&g_mdc->mutex);
			if (maxdsm_v_validation_check(g_mdc, status)) {
				pr_err("%s: The codec was connected?\n",
						__func__);
				mutex_lock(&g_mdc->mutex);
				g_mdc->values_v_validation.status = 0;
				mutex_unlock(&g_mdc->mutex);
			}
		}
	}

	return size;
}

static ssize_t maxdsm_cal_thermal_enable_show(struct device *dev,
					  struct device_attribute *attr,
					  char *buf)
{
	return sprintf(buf, "%d", g_mdc->values_thermal_safe_mode.enable);
}

static ssize_t maxdsm_cal_thermal_enable_store(struct device *dev,
					   struct device_attribute *attr,
					   const char *buf, size_t size)
{
	if (kstrtou32(buf, 0, &g_mdc->values_thermal_safe_mode.enable))
		dev_err(dev, "%s: Failed converting from str to u32.\n",
			__func__);
	return size;
}
static DEVICE_ATTR(maxim_dsm_thermal_enable, 0664,
		maxdsm_cal_thermal_enable_show, maxdsm_cal_thermal_enable_store);


static DEVICE_ATTR(v_status, 0644,
	maxdsm_cal_v_status_show, maxdsm_cal_v_status_store);

static ssize_t maxdsm_cal_amp_screen_status_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n",
			g_mdc->values_v_validation.amp_screen_status ?
			"Enabled" : "Disabled");
}

static ssize_t maxdsm_cal_amp_screen_status_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int status = 0;

	if (!kstrtou32(buf, 0, &status)) {
		status = status > 0 ? 1 : 0;
		if (status == g_mdc->values_v_validation.amp_screen_status) {
			dbg_maxdsm("Already run. It will be ignored.");
		} else {
			mutex_lock(&g_mdc->mutex);
			g_mdc->values_v_validation.amp_screen_status = status;
			mutex_unlock(&g_mdc->mutex);
			if (maxdsm_amp_screen_validation_check(g_mdc, status)) {
				pr_err("%s: The codec was connected?\n", __func__);
				mutex_lock(&g_mdc->mutex);
				g_mdc->values_v_validation.amp_screen_status = 0;
				mutex_unlock(&g_mdc->mutex);
			}
		}
	}

	return size;
}
static DEVICE_ATTR(amp_screen_status, 0644,
		   maxdsm_cal_amp_screen_status_show, maxdsm_cal_amp_screen_status_store);

static struct attribute *maxdsm_cal_attr[] = {
	&dev_attr_min.attr,
	&dev_attr_max.attr,
	&dev_attr_duration.attr,
	&dev_attr_screen_duration.attr,
	&dev_attr_rdc.attr,
	&dev_attr_rdc_r.attr,
	&dev_attr_temp.attr,
	&dev_attr_interval.attr,
	&dev_attr_amp_screen_interval.attr,
	&dev_attr_ignored_t.attr,
	&dev_attr_status.attr,
	&dev_attr_v_status.attr,
	&dev_attr_amp_screen_status.attr,
	&dev_attr_v_validation.attr,
	&dev_attr_v_validation_r.attr,
	&dev_attr_amp_screen_ratio.attr,
	&dev_attr_amp_screen_ratio_r.attr,
	&dev_attr_v_duration.attr,
	&dev_attr_maxim_dsm_thermal_enable.attr,
	NULL,
};

static struct attribute_group maxdsm_cal_attr_grp = {
	.attrs = maxdsm_cal_attr,
};

static int __init maxdsm_cal_init(void)
{
	struct maxim_dsm_cal *mdc;
	int ret = 0;

	g_mdc = kzalloc(sizeof(struct maxim_dsm_cal), GFP_KERNEL);
	if (g_mdc == NULL)
		return -ENOMEM;
	mdc = g_mdc;

	mdc->wq = create_singlethread_workqueue(WQ_NAME);
	if (mdc->wq == NULL) {
		kfree(g_mdc);
		return -ENOMEM;
	}

	INIT_DELAYED_WORK(&g_mdc->work, maxdsm_cal_work);
	INIT_DELAYED_WORK(&g_mdc->work_v_validation, maxdsm_v_validation_work);
	INIT_DELAYED_WORK(&g_mdc->work_amp_screen_validation, maxdsm_amp_screen_validation_work);
	mutex_init(&g_mdc->mutex);

	mdc->info.min = 0;
	mdc->info.max = 0xFFFFFFFF;
	mdc->info.duration = 1500; /* 1.5 secs */
	mdc->info.remaining = mdc->info.duration;
	mdc->info.interval = 100;
	mdc->info.ignored_t = 1000;
	mdc->values.rdc = 0xFFFFFFFF;
	mdc->values.rdc_r = 0xFFFFFFFF;
	mdc->values.temp = 0xFFFFFFFF;
	mdc->info_v_validation.duration = 1000; /* 0.5 secs */
	mdc->info_v_validation.screen_duration = 1000; /* 1 sec */
	mdc->info_v_validation.screen_remaining = mdc->info_v_validation.screen_duration;
	mdc->info_v_validation.screen_interval = 10;
	mdc->values_v_validation.boost_bypass = 1;
	mdc->platform_type = 0xFFFFFFFF;
	mdc->values_thermal_safe_mode.thermal_min_gain = 0x26666666;
	mdc->values_thermal_safe_mode.enable = 1;

	if (!g_class)
		g_class = class_create(THIS_MODULE, DSM_NAME);
	mdc->class = g_class;
	if (mdc->class) {
		mdc->dev = device_create(mdc->class, NULL, 1, NULL, CLASS_NAME);
		if (!IS_ERR(mdc->dev)) {
			if (sysfs_create_group(&mdc->dev->kobj,
						&maxdsm_cal_attr_grp))
				dbg_maxdsm(
						"Failed to create sysfs group. ret=%d",
						ret);
		}
	}
	dbg_maxdsm("g_class=%p %p", g_class, mdc->class);

	dbg_maxdsm("Completed initialization");

	return ret;
}
module_init(maxdsm_cal_init);

static void __exit maxdsm_cal_exit(void)
{
	kfree(g_mdc);
}
module_exit(maxdsm_cal_exit);

MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_SUPPORTED_DEVICE(DRIVER_SUPPORTED);
MODULE_LICENSE("GPL");
