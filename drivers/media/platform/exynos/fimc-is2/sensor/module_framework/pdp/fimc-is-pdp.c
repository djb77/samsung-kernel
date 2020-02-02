/*
 * Samsung Exynos5 SoC series Sensor driver
 *
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/of.h>
#include <linux/mutex.h>

#include <media/v4l2-subdev.h>

#include "fimc-is-config.h"
#include "fimc-is-pdp.h"
#include "fimc-is-hw-pdp.h"
#include "fimc-is-interface-library.h"

struct general_intr_handler info_handler;

static int debug_pdp;

module_param(debug_pdp, int, 0644);

static struct fimc_is_pdp pdp_device[MAX_NUM_OF_PDP];

extern struct fimc_is_lib_support gPtr_lib_support;

#ifdef ENABLE_PDP_IRQ
static irqreturn_t fimc_is_isr_pdp(int irq, void *data)
{
	struct fimc_is_pdp *pdp;
	int state;

	pdp = data;
	state = pdp_hw_g_irq_src(pdp->base_reg, true);
	printk("PDP IRQ: %d\n", state);

#ifdef DEBUG_DUMP_STAT0
	if (state == PDP_INT_FRAME_PAF_STAT0)
		schedule_work(&pdp->wq_pdp_stat0);
#endif

	return IRQ_HANDLED;
}
#endif

int pdp_get_irq_state(struct v4l2_subdev *subdev, int *state)
{
	struct fimc_is_pdp *pdp;
	struct fimc_is_device_sensor_peri *sensor_peri;
	struct fimc_is_module_enum *module;

	WARN_ON(!subdev);

	pdp = (struct fimc_is_pdp *)v4l2_get_subdevdata(subdev);
	WARN_ON(!pdp);

	module = (struct fimc_is_module_enum *)v4l2_get_subdev_hostdata(subdev);
	WARN_ON(!module);

	sensor_peri = module->private_data;
	WARN_ON(!sensor_peri);

	if (!test_bit(FIMC_IS_SENSOR_PDP_AVAILABLE, &sensor_peri->peri_state))
		*state = -EPERM;
	else {
		mutex_lock(&pdp->control_lock);
		*state = pdp_hw_g_irq_state(pdp->base_reg, false);
		mutex_unlock(&pdp->control_lock);
	}
	return 0;
}

int pdp_clear_irq_state(struct v4l2_subdev *subdev, int state)
{
	struct fimc_is_pdp *pdp;
	struct fimc_is_device_sensor_peri *sensor_peri;
	struct fimc_is_module_enum *module;

	WARN_ON(!subdev);

	pdp = (struct fimc_is_pdp *)v4l2_get_subdevdata(subdev);
	WARN_ON(!pdp);

	module = (struct fimc_is_module_enum *)v4l2_get_subdev_hostdata(subdev);
	WARN_ON(!module);

	sensor_peri = module->private_data;
	WARN_ON(!sensor_peri);

	mutex_lock(&pdp->control_lock);
	pdp_hw_clr_irq_state(pdp->base_reg, state);
	mutex_unlock(&pdp->control_lock);

	return 0;
}

int pdp_set_sensor_info(struct fimc_is_pdp *pdp, u32 sensor_info)
{
	int width = 0, height = 0, active_height = 0;
	int af_size_x = 0, af_size_y = 0;
	int hbin = 0, vbin = 0, vsft = 0;
	struct pdp_point_info cropInfo;
	struct pdp_paf_roi_setting_t roiInfo;

	switch (sensor_info) {
	case (4032<<16|3024):	/* 4032 x 3024 */
		/*1.  Image Info */
		width = (sensor_info & 0xFFFF0000)>>16;
		height = sensor_info & 0xFFFF;
		active_height = 756;
		/* 2. AF */
		af_size_x = 504;
		af_size_y = 378;
		/* 3. ROI Info */
		roiInfo.roi_start_x = 0;
		roiInfo.roi_start_y = 0;
		roiInfo.roi_end_x = 503;
		roiInfo.roi_end_y = 377;
		/* 4. Crop Info*/
		cropInfo.sx = 0;
		cropInfo.sy = 0;
		cropInfo.ex = 503;
		cropInfo.ey = 377;
		/* 5. HBIN / VBIN /VSFT */
		hbin = 1;
		vbin = 1;
		vsft = 1;
		break;
	case (4032<<16|2268):	/* 4032 x 2268 */
		/*1.  Image Info */
		width = (sensor_info & 0xFFFF0000)>>16;
		height = sensor_info & 0xFFFF;
		active_height = 566;
		/* 2. AF */
		af_size_x = 504;
		af_size_y = 282;
		/* 3. ROI Info */
		roiInfo.roi_start_x = 0;
		roiInfo.roi_start_y = 0;
		roiInfo.roi_end_x = 503;
		roiInfo.roi_end_y = 281;
		/* 4. Crop Info*/
		cropInfo.sx = 0;
		cropInfo.sy = 0;
		cropInfo.ex = 503;
		cropInfo.ey = 281;
		/* 5. HBIN / VBIN /VSFT */
		hbin = 1;
		vbin = 1;
		vsft = 1;
		break;
	case (4032<<16|1960):	/* 4032 x 1960 */
		/*1.  Image Info */
		width = (sensor_info & 0xFFFF0000)>>16;
		height = sensor_info & 0xFFFF;
		active_height = 490;
		/* 2. AF */
		af_size_x = 504;
		af_size_y = 244;
		/* 3. ROI Info */
		roiInfo.roi_start_x = 0;
		roiInfo.roi_start_y = 0;
		roiInfo.roi_end_x = 503;
		roiInfo.roi_end_y = 243;
		/* 4. Crop Info*/
		cropInfo.sx = 0;
		cropInfo.sy = 0;
		cropInfo.ex = 503;
		cropInfo.ey = 243;
		/* 5. HBIN / VBIN /VSFT */
		hbin = 1;
		vbin = 1;
		vsft = 1;
		break;
	case (3024<<16|3024):	/* 3024 x 3024 */
		/*1.  Image Info */
		width = (sensor_info & 0xFFFF0000)>>16;
		height = sensor_info & 0xFFFF;
		active_height = 756;
		/* 2. AF */
		af_size_x = 376;
		af_size_y = 376;
		/* 3. ROI Info */
		roiInfo.roi_start_x = 0;
		roiInfo.roi_start_y = 0;
		roiInfo.roi_end_x = 375;
		roiInfo.roi_end_y = 375;
		/* 4. Crop Info*/
		cropInfo.sx = 0;
		cropInfo.sy = 0;
		cropInfo.ex = 375;
		cropInfo.ey = 375;
		/* 5. HBIN / VBIN /VSFT */
		hbin = 1;
		vbin = 1;
		vsft = 1;
		break;
	case (2016<<16|1512):	/* 2016 x 1512 */
		/*1.  Image Info */
		width = (sensor_info & 0xFFFF0000)>>16;
		height = sensor_info & 0xFFFF;
		active_height = 378;
		/* 2. AF */
		af_size_x = 504;
		af_size_y = 378;
		/* 3. ROI Info */
		roiInfo.roi_start_x = 0;
		roiInfo.roi_start_y = 0;
		roiInfo.roi_end_x = 503;
		roiInfo.roi_end_y = 377;
		/* 4. Crop Info*/
		cropInfo.sx = 0;
		cropInfo.sy = 0;
		cropInfo.ex = 503;
		cropInfo.ey = 377;
		/* 5. HBIN / VBIN /VSFT */
		hbin = 0;
		vbin = 0;
		vsft = 0;
		break;
	case (2016<<16|1134):	/* 2016 x 1134 */
		/*1.  Image Info */
		width = (sensor_info & 0xFFFF0000)>>16;
		height = sensor_info & 0xFFFF;
		active_height = 282;
		/* 2. AF */
		af_size_x = 504;
		af_size_y = 282;
		/* 3. ROI Info */
		roiInfo.roi_start_x = 0;
		roiInfo.roi_start_y = 0;
		roiInfo.roi_end_x = 503;
		roiInfo.roi_end_y = 281;
		/* 4. Crop Info*/
		cropInfo.sx = 0;
		cropInfo.sy = 0;
		cropInfo.ex = 503;
		cropInfo.ey = 281;
		/* 5. HBIN / VBIN /VSFT */
		hbin = 0;
		vbin = 0;
		vsft = 0;
		break;
	case (1504<<16|1504):	/* 1504 x 1504 */
		/*1.  Image Info */
		width = (sensor_info & 0xFFFF0000)>>16;
		height = sensor_info & 0xFFFF;
		active_height = 376;
		/* 2. AF */
		af_size_x = 376;
		af_size_y = 376;
		/* 3. ROI Info */
		roiInfo.roi_start_x = 0;
		roiInfo.roi_start_y = 0;
		roiInfo.roi_end_x = 375;
		roiInfo.roi_end_y = 375;
		/* 4. Crop Info*/
		cropInfo.sx = 0;
		cropInfo.sy = 0;
		cropInfo.ex = 503;
		cropInfo.ey = 503;
		/* 5. HBIN / VBIN /VSFT */
		hbin = 0;
		vbin = 0;
		vsft = 0;
		break;
	default:
		width = (sensor_info & 0xFFFF0000)>>16;
		height = sensor_info & 0xFFFF;
		err("Invalid Sensor Size.(%dx%d) set default size(4:3)", width, height);
		/*1.  Image Info */
		width = 4032;
		height = 3024;
		active_height = 756;
		/* 2. AF */
		af_size_x = 504;
		af_size_y = 378;
		/* 3. ROI Info */
		roiInfo.roi_start_x = 0;
		roiInfo.roi_start_y = 0;
		roiInfo.roi_end_x = 503;
		roiInfo.roi_end_y = 377;
		/* 4. Crop Info*/
		cropInfo.sx = 0;
		cropInfo.sy = 0;
		cropInfo.ex = 503;
		cropInfo.ey = 377;
		/* 5. HBIN / VBIN /VSFT*/
		hbin = 1;
		vbin = 1;
		vsft = 1;
		break;
	}

	pdp_hw_s_paf_col(pdp->base_reg, width);
	pdp_hw_s_paf_active_size(pdp->base_reg, width, active_height);
	pdp_hw_s_paf_af_size(pdp->base_reg, af_size_x, af_size_y);
	pdp_hw_s_paf_roi(pdp->base_reg, &roiInfo);
	pdp_hw_s_paf_crop_size(pdp->base_reg, &cropInfo);
	pdp_hw_s_paf_mpd_bin_shift(pdp->base_reg, hbin, vbin, vsft);

	info("[PDP]I_COL(%d), ActiveSize(%dx%d), AfSize(%dx%d)\n",
		width, width, active_height, af_size_x, af_size_y);
	info("[PDP]roi(%dx%d~%dx%d), crop(%dx%d~%dx%d)\n",
		roiInfo.roi_start_x, roiInfo.roi_start_y, roiInfo.roi_end_x, roiInfo.roi_end_y,
		cropInfo.sx, cropInfo.sy, cropInfo.ex, cropInfo.ey);

	return 0;
}

int pdp_read_paf_sfr_stat(struct v4l2_subdev *subdev, u32 *buf)
{
	struct fimc_is_pdp *pdp;

	WARN_ON(!subdev);
	WARN_ON(!buf);

	pdp = (struct fimc_is_pdp *)v4l2_get_subdevdata(subdev);
	WARN_ON(!pdp);

	pdp_hw_g_paf_sfr_stats(pdp->base_reg, buf);

	return 0;
}

int pdp_set_parameters(struct v4l2_subdev *subdev, struct pdp_total_setting_t *pdp_param)
{
	int i = 0;
	struct fimc_is_pdp *pdp;

	WARN_ON(!subdev);
	WARN_ON(!pdp_param);

	pdp = (struct fimc_is_pdp *)v4l2_get_subdevdata(subdev);
	WARN_ON(!pdp);

	dbg_pdp(1, "[%s] IN\n", __func__);

	/* PAF SETTING */
	WARN_ON(!pdp_param->paf_setting);
	if (pdp_param->paf_setting->update) {
		dbg_pdp(2, "[%s] update_paf_setting\n", __func__);

		mutex_lock(&pdp->control_lock);
		pdp_hw_s_paf_setting(pdp->base_reg, pdp_param->paf_setting);
		mutex_unlock(&pdp->control_lock);

		pdp_param->paf_setting->update = false;
	}

	/* PAF ROI */
	WARN_ON(!pdp_param->paf_roi_setting);
	if (pdp_param->paf_roi_setting->update) {
		dbg_pdp(2, "[%s] update_paf_roi_setting\n", __func__);

		mutex_lock(&pdp->control_lock);
		pdp_hw_s_paf_roi(pdp->base_reg, pdp_param->paf_roi_setting);
		mutex_unlock(&pdp->control_lock);

		pdp_param->paf_roi_setting->update = false;
	}

	/* SINGLE_WINDOW */
	WARN_ON(!pdp_param->paf_single_window);
	if (pdp_param->paf_single_window->update) {
		dbg_pdp(2, "[%s] update_single_win\n", __func__);

		mutex_lock(&pdp->control_lock);
		pdp_hw_s_paf_single_window(pdp->base_reg, pdp_param->paf_single_window);
		mutex_unlock(&pdp->control_lock);

		pdp_param->paf_single_window->update = false;
	}

	/* MAIN WINDOW */
	WARN_ON(!pdp_param->paf_main_window);
	if (pdp_param->paf_main_window->update) {
		dbg_pdp(2, "[%s] update_main_win\n", __func__);

		mutex_lock(&pdp->control_lock);
		pdp_hw_s_paf_main_window(pdp->base_reg, pdp_param->paf_main_window);
		mutex_unlock(&pdp->control_lock);
		pdp_param->paf_main_window->update = false;
	}

	/* MULTI WINDOW */
	WARN_ON(!pdp_param->paf_multi_window);
	if (pdp_param->paf_multi_window->update) {
		dbg_pdp(2, "[%s] update_multi_win\n", __func__);

		mutex_lock(&pdp->control_lock);
		pdp_hw_s_paf_multi_window(pdp->base_reg, pdp_param->paf_multi_window);
		mutex_unlock(&pdp->control_lock);

		pdp_param->paf_multi_window->update = false;
	}

	/* KNEE */
	WARN_ON(!pdp_param->paf_knee_setting);
	if (pdp_param->paf_knee_setting->update) {
		dbg_pdp(2, "[%s] update_knee\n", __func__);

		mutex_lock(&pdp->control_lock);
		pdp_hw_s_paf_knee(pdp->base_reg, pdp_param->paf_knee_setting);
		mutex_unlock(&pdp->control_lock);

		pdp_param->paf_knee_setting->update = false;
	}

	/* FILTER COR */
	WARN_ON(!pdp_param->paf_filter_cor);
	if (pdp_param->paf_filter_cor->update) {
		dbg_pdp(2, "[%s] update_filter_cor\n", __func__);

		mutex_lock(&pdp->control_lock);
		pdp_hw_s_paf_filter_cor(pdp->base_reg, pdp_param->paf_filter_cor);
		mutex_unlock(&pdp->control_lock);

		pdp_param->paf_filter_cor->update = false;
	}

	/* FILTER BIN */
	WARN_ON(!pdp_param->paf_filter_bin);
	if (pdp_param->paf_filter_bin->update) {
		dbg_pdp(2, "[%s] update_filter_bin\n", __func__);

		mutex_lock(&pdp->control_lock);
		pdp_hw_s_paf_filter_bin(pdp->base_reg, pdp_param->paf_filter_bin);
		mutex_unlock(&pdp->control_lock);

		pdp_param->paf_filter_bin->update = false;
	}

	/* FILTER IIR0, 1, 2, L */
	for (i = 0; i < MAX_FILTER_BAND; i++) {
		WARN_ON(!pdp_param->paf_filter_band[i]);
		if (pdp_param->paf_filter_band[i]->update) {
			dbg_pdp(2, "[%s] paf_filter_band[%d]\n", __func__, i);

			mutex_lock(&pdp->control_lock);
			pdp_hw_s_paf_filter_band(pdp->base_reg, pdp_param->paf_filter_band[i], i);
			mutex_unlock(&pdp->control_lock);

			pdp_param->paf_filter_band[i]->update = false;
		}
	}

	/* WDR SETTING */
	WARN_ON(!pdp_param->wdr_setting);
	if (pdp_param->wdr_setting->update) {
		dbg_pdp(2, "[%s] update_wdr_setting\n", __func__);

		mutex_lock(&pdp->control_lock);
		pdp_hw_s_wdr_setting(pdp->base_reg, pdp_param->wdr_setting);
		mutex_unlock(&pdp->control_lock);

		pdp_param->wdr_setting->update = false;
	}

	/* DEPTH SETTING */
	WARN_ON(!pdp_param->depth_setting);
	if (pdp_param->depth_setting->update) {
		dbg_pdp(2, "[%s] update_depth_setting\n", __func__);

		mutex_lock(&pdp->control_lock);
		pdp_hw_s_depth_setting(pdp->base_reg, pdp_param->depth_setting);
		mutex_unlock(&pdp->control_lock);

		pdp_param->depth_setting->update = false;
	}

	/* Y EXT */
	WARN_ON(!pdp_param->y_ext_param);
	if (pdp_param->y_ext_param->update) {
		dbg_pdp(2, "[%s] update_depth_setting\n", __func__);

		mutex_lock(&pdp->control_lock);
		pdp_hw_s_y_ext_setting(pdp->base_reg, pdp_param->y_ext_param);
		mutex_unlock(&pdp->control_lock);

		pdp_param->y_ext_param->update = false;
	}

	return 0;
}

int pdp_read_pdp_reg(struct v4l2_subdev *subdev, struct pdp_read_reg_setting_t *reg_param)
{
	struct fimc_is_pdp *pdp;

	WARN_ON(!subdev);
	WARN_ON(!reg_param);

	pdp = (struct fimc_is_pdp *)v4l2_get_subdevdata(subdev);
	WARN_ON(!pdp);

	mutex_lock(&pdp->control_lock);
	pdp_hw_g_pdp_reg(pdp->base_reg, reg_param);
	mutex_unlock(&pdp->control_lock);

	return 0;
}

int pdp_register(struct fimc_is_module_enum *module, int pdp_ch)
{
	int ret = 0;
	struct fimc_is_pdp *pdp = NULL;
	struct fimc_is_device_sensor *sensor_dev = NULL;
	struct fimc_is_device_sensor_peri *sensor_peri = module->private_data;

	if (test_bit(FIMC_IS_SENSOR_PDP_AVAILABLE, &sensor_peri->peri_state)) {
		err("already registered");
		ret = -EINVAL;
		goto p_err;
	}

	WARN_ON(!sensor_peri);
	sensor_dev = (struct fimc_is_device_sensor *)v4l2_get_subdev_hostdata(sensor_peri->subdev_cis);

	if (pdp_ch >= MAX_NUM_OF_PDP) {
		err("A pdp channel is invalide");
		ret = -EINVAL;
		goto p_err;
	}

	pdp = &pdp_device[pdp_ch];
	sensor_peri->pdp = pdp;
	sensor_peri->subdev_pdp = pdp->subdev;
	v4l2_set_subdev_hostdata(pdp->subdev, module);

	if (sensor_dev->cfg->pd_mode != PD_MOD3) {
		pdp_hw_enable(pdp->base_reg, PD_NONE);
		ret = -ENOTSUPP;
	} else {
		set_bit(FIMC_IS_SENSOR_PDP_AVAILABLE, &sensor_peri->peri_state);
		pdp_hw_s_config(pdp->base_reg);
	}

	info("[PDP:%d] %s(ret:%d)\n", pdp->id, __func__, ret);
	return ret;
p_err:
	return ret;
}

int pdp_unregister(struct fimc_is_module_enum *module)
{
	int ret = 0;
	struct fimc_is_device_sensor_peri *sensor_peri = module->private_data;
	struct fimc_is_pdp *pdp = NULL;
	struct fimc_is_device_sensor *sensor_dev = NULL;

	WARN_ON(!sensor_peri);
	sensor_dev = (struct fimc_is_device_sensor *)v4l2_get_subdev_hostdata(sensor_peri->subdev_cis);

	if (sensor_dev->cfg->pd_mode != PD_MOD3) {
		ret = -ENOTSUPP;
		goto p_err;
	}

	if (!test_bit(FIMC_IS_SENSOR_PDP_AVAILABLE, &sensor_peri->peri_state)) {
		err("already unregistered");
		ret = -EINVAL;
		goto p_err;
	}

	pdp = (struct fimc_is_pdp *)v4l2_get_subdevdata(sensor_peri->subdev_pdp);
	if (!pdp) {
		err("A subdev data of PDP is null");
		ret = -ENODEV;
		goto p_err;
	}

	sensor_peri->pdp = NULL;
	sensor_peri->subdev_pdp = NULL;
	clear_bit(FIMC_IS_SENSOR_PDP_AVAILABLE, &sensor_peri->peri_state);

	info("[PDP:%d] %s(ret:%d)\n", pdp->id, __func__, ret);
	return ret;
p_err:
	return ret;
}

int pdp_init(struct v4l2_subdev *subdev, u32 val)
{
	return 0;
}

static int pdp_s_stream(struct v4l2_subdev *subdev, int pd_mode)
{
	int ret = 0;
	int irq_state = 0;
	int enable = 0;

	struct fimc_is_pdp *pdp;
	struct fimc_is_device_sensor_peri *sensor_peri;
	struct fimc_is_module_enum *module;
	cis_shared_data *cis_data = NULL;

	module = (struct fimc_is_module_enum *)v4l2_get_subdev_hostdata(subdev);
	WARN_ON(!module);

	sensor_peri = module->private_data;
	WARN_ON(!sensor_peri);

	cis_data = sensor_peri->cis.cis_data;
	WARN_ON(!cis_data);

	pdp = (struct fimc_is_pdp *)v4l2_get_subdevdata(subdev);
	if (!pdp) {
		err("A subdev data of PDP is null");
		ret = -ENODEV;
		goto p_err;
	}

#ifdef DEBUG_DUMP_STAT0
	INIT_WORK(&pdp->wq_pdp_stat0, pdp_hw_g_pad_single_window_stat);
#endif

	enable = pdp_hw_enable(pdp->base_reg, pd_mode);

	irq_state = pdp_hw_g_irq_state(pdp->base_reg, true);

	if (debug_pdp == 3)
		pdp_hw_dump(pdp->base_reg);

	info("[PDP:%d] PD_MOD%d, HW_ENABLE(%d), IRQ(%d)\n",
		pdp->id, pd_mode, enable, irq_state);

	return ret;
p_err:
	return ret;
}

static int pdp_s_param(struct v4l2_subdev *subdev, struct v4l2_streamparm *param)
{
	return 0;
}

static int pdp_s_format(struct v4l2_subdev *subdev,
	struct v4l2_subdev_pad_config *cfg,
	struct v4l2_subdev_format *fmt)
{
	int ret = 0;
	u32 sensor_info = 0;
	size_t width, height;
	struct fimc_is_pdp *pdp;
	struct fimc_is_module_enum *module;

	pdp = (struct fimc_is_pdp *)v4l2_get_subdevdata(subdev);
	if (!pdp) {
		err("A subdev data of PDP is null");
		ret = -ENODEV;
		goto p_err;
	}

	width = fmt->format.width;
	height = fmt->format.height;
	pdp->width = width;
	pdp->height = height;

	module = (struct fimc_is_module_enum *)v4l2_get_subdev_hostdata(subdev);
	if (!module) {
		err("[PDP:%d] A host data of PDP is null", pdp->id);
		ret = -ENODEV;
		goto p_err;
	}

	sensor_info = (u32)((width<<16) | height);
	pdp_set_sensor_info(pdp, sensor_info);

	return ret;
p_err:
	return ret;
}

static const struct v4l2_subdev_core_ops core_ops = {
	.init = pdp_init
};

static const struct v4l2_subdev_video_ops video_ops = {
	.s_stream = pdp_s_stream,
	.s_parm = pdp_s_param,
};

static const struct v4l2_subdev_pad_ops pad_ops = {
	.set_fmt = pdp_s_format
};

static const struct v4l2_subdev_ops subdev_ops = {
	.core = &core_ops,
	.video = &video_ops,
	.pad = &pad_ops
};

struct fimc_is_pdp_ops pdp_ops = {
	.read_pdp_reg = pdp_read_pdp_reg,
	.read_paf_sfr_stat = pdp_read_paf_sfr_stat,
	.get_irq_state = pdp_get_irq_state,
	.clear_irq_state = pdp_clear_irq_state,
	.set_pdp_param = pdp_set_parameters,
};

static int pdp_probe(struct platform_device *pdev)
{
	int ret = 0;
	int id = -1;
	struct fimc_is_core *core;
	struct resource *mem_res;
	struct fimc_is_pdp *pdp;
	struct device_node *dnode;
	struct device *dev;

	WARN_ON(!fimc_is_dev);
	WARN_ON(!pdev);
	WARN_ON(!pdev->dev.of_node);

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	if (!core) {
		probe_err("core device is not yet probed");
		return -EPROBE_DEFER;
	}

	dev = &pdev->dev;
	dnode = dev->of_node;

	ret = of_property_read_u32(dnode, "id", &pdev->id);
	if (ret) {
		dev_err(dev, "id read is fail(%d)\n", ret);
		goto err_get_id;
	}

	id = pdev->id;
	pdp = &pdp_device[id];

	/* Get SFR base register */
	mem_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!mem_res) {
		dev_err(dev, "Failed to get io memory region(%p)\n", mem_res);
		ret = -EBUSY;
		goto err_get_resource;
	}

	pdp->regs_start = mem_res->start;
	pdp->regs_end = mem_res->end;
	pdp->base_reg =  devm_ioremap_nocache(&pdev->dev, mem_res->start, resource_size(mem_res));
	if (!pdp->base_reg) {
		dev_err(dev, "Failed to remap io region(%p)\n", pdp->base_reg);
		ret = -ENOMEM;
		goto err_ioremap;
	}

	/* Get CORE IRQ SPI number */
	pdp->irq = platform_get_irq(pdev, 0);
	if (pdp->irq < 0) {
		dev_err(dev, "Failed to get pdp_irq(%d)\n", pdp->irq);
		ret = -EBUSY;
		goto err_get_irq;
	}

#ifdef ENABLE_PDP_IRQ
	ret = request_irq(pdp->irq,
			fimc_is_isr_pdp,
			FIMC_IS_HW_IRQ_FLAG,
			"pdp",
			pdp);
	if (ret) {
		dev_err(dev, "request_irq(IRQ_PDP %d) is fail(%d)\n", pdp->irq, ret);
		goto err_get_irq;
	}
#endif

	pdp->id = id;
	platform_set_drvdata(pdev, pdp);

	pdp->subdev = devm_kzalloc(&pdev->dev, sizeof(struct v4l2_subdev), GFP_KERNEL);
	if (!pdp->subdev) {
		ret = -ENOMEM;
		goto err_subdev_alloc;
	}

	v4l2_subdev_init(pdp->subdev, &subdev_ops);

	v4l2_set_subdevdata(pdp->subdev, pdp);
	snprintf(pdp->subdev->name, V4L2_SUBDEV_NAME_SIZE, "pdp-subdev.%d", pdp->id);

	if (id == 1)
		gPtr_lib_support.intr_handler[ID_GENERAL_INTR_PDP1_STAT].irq = pdp->irq;
	else
		gPtr_lib_support.intr_handler[ID_GENERAL_INTR_PDP0_STAT].irq = pdp->irq;

	pdp->pdp_ops = &pdp_ops;
	mutex_init(&pdp->control_lock);

#ifdef DEBUG_DUMP_STAT0
	pdp->workqueue = alloc_workqueue("fimc-pdp/[H/U]", WQ_HIGHPRI | WQ_UNBOUND, 0);
	if (!pdp->workqueue)
		probe_warn("failed to alloc PDP own workqueue, will be use global one");
#endif
	probe_info("%s(%s)\n", __func__, dev_name(&pdev->dev));

	return ret;

err_subdev_alloc:
err_get_irq:
err_ioremap:
err_get_resource:
err_get_id:
	return ret;
}

static const struct of_device_id exynos_fimc_is_pdp_match[] = {
	{
		.compatible = "samsung,exynos5-fimc-is-pdp",
	},
	{},
};
MODULE_DEVICE_TABLE(of, exynos_fimc_is_pdp_match);

static struct platform_driver pdp_driver = {
	.driver = {
		.name   = "FIMC-IS-PDP",
		.owner  = THIS_MODULE,
		.of_match_table = exynos_fimc_is_pdp_match,
	}
};
builtin_platform_driver_probe(pdp_driver, pdp_probe);
