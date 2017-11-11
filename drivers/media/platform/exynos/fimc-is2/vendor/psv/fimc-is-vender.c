/*
* Samsung Exynos SoC series FIMC-IS driver
 *
 * Exynos fimc-is PSV vender functions
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/slab.h>
#include <media/v4l2-dev.h>
#include <linux/videodev2_exynos_camera.h>

#include "fimc-is-core.h"
#include "fimc-is-vender.h"
#include "fimc-is-vender-specific.h"
#include "fimc-is-vector.h"
#include "fimc-is-asb.h"

static void *fimc_is_vender_ion_init(struct platform_device *pdev)
{
#ifdef USE_IOMMU
	long flag = VB2ION_CTX_IOMMU | VB2ION_CTX_VMCONTIG;
#else
	long flag = VB2ION_CTX_PHCONTIG;
#endif

	return vb2_ion_create_context(&pdev->dev, SZ_4K, flag);
}

int fimc_is_vender_probe(struct fimc_is_vender *vender)
{
	int ret = 0;
	struct fimc_is_core *core;
	struct fimc_is_vender_specific *priv;
#ifdef CONFIG_PSV_VECTOR_VERIFICATION
	struct vector_cfg *cfg;
#endif

	if (!vender) {
		probe_err("vender is invalid");
		return -EINVAL;
	}
	core = container_of(vender, struct fimc_is_core, vender);

	snprintf(vender->fw_path, sizeof(vender->fw_path),
			"%s", FIMC_IS_FW_SDCARD);
	snprintf(vender->request_fw_path, sizeof(vender->request_fw_path),
			"%s", FIMC_IS_FW);

	priv = (struct fimc_is_vender_specific *)kzalloc(
					sizeof(struct fimc_is_vender_specific), GFP_KERNEL);
	if (!priv) {
		probe_err("failed to allocate vender specific");
		return -ENOMEM;
	}

	priv->alloc_ctx = fimc_is_vender_ion_init(core->pdev);
	if (IS_ERR_OR_NULL(priv->alloc_ctx)) {
		probe_err("failed to init. allocation context");
		ret = IS_ERR(priv->alloc_ctx) ? PTR_ERR(priv->alloc_ctx) : -EINVAL;
		goto err_ion_init;
	}

#ifdef CONFIG_PSV_VECTOR_VERIFICATION
	cfg = &priv->vector_cfg;

#if defined(CONFIG_SOC_EXYNOS8895)
	cfg->irq_3aa0 = platform_get_irq(core->pdev, PDEV_IRQ_NUM_3AAW_0);
	pr_info("3AAW IRQ number: %d\n", cfg->irq_3aa0);
	if (cfg->irq_3aa0 < 0) {
		err_vec("failed to get irq for %d", PDEV_IRQ_NUM_3AAW_0);
		goto err_get_irq_3aa0;
	}

	cfg->irq_3aa1 = platform_get_irq(core->pdev, PDEV_IRQ_NUM_3AA_0);
	pr_info("3AA IRQ number: %d\n", cfg->irq_3aa0);
	if (cfg->irq_3aa1 < 0) {
		err_vec("failed to get irq for %d", PDEV_IRQ_NUM_3AA_0);
		goto err_get_irq_3aa1;
	}


	cfg->irq_mcsc0 = platform_get_irq(core->pdev, PDEV_IRQ_NUM_MCSC_0);
	pr_info("MCSC0 IRQ number: %d\n", cfg->irq_mcsc0);
	if (cfg->irq_mcsc0 < 0) {
		err_vec("failed to get irq for %d", PDEV_IRQ_NUM_MCSC_0);
		goto err_get_irq_mcsc0;
	}

	cfg->irq_mcsc1 = platform_get_irq(core->pdev, PDEV_IRQ_NUM_MCSC_1);
	pr_info("MCSC1 IRQ number: %d\n", cfg->irq_mcsc1);
	if (cfg->irq_mcsc1 < 0) {
		err_vec("failed to get irq for %d", PDEV_IRQ_NUM_MCSC_1);
		goto err_get_irq_mcsc1;
	}

	cfg->irq_isp1 = platform_get_irq(core->pdev, PDEV_IRQ_NUM_ISPHQ_0);
	pr_info("ISPHQ IRQ number: %d\n", cfg->irq_isp1);
	if (cfg->irq_isp1 < 0) {
		err_vec("failed to get irq for %d", PDEV_IRQ_NUM_ISPHQ_0);
		goto err_get_irq_isp1;
	}

#elif defined(CONFIG_SOC_EXYNOS7880)
	cfg->irq_mcsc = platform_get_irq(core->pdev, PDEV_IRQ_NUM_MCSC);
	pr_info("MCSC IRQ number: %d\n", cfg->irq_mcsc);
	if (cfg->irq_mcsc < 0) {
		err_vec("failed to get irq for %d", PDEV_IRQ_NUM_MCSC);
		goto err_get_irq_mcsc;
	}
	cfg->irq_vra0 = platform_get_irq(core->pdev, PDEV_IRQ_NUM_VRA0);
	pr_info("VRA IRQ number: %d\n", cfg->irq_vra0);
	if (cfg->irq_vra0 < 0) {
		err_vec("failed to get irq for %d", PDEV_IRQ_NUM_VRA0);
		goto err_get_irq_vra0;
	}

	cfg->irq_isp0 = platform_get_irq(core->pdev, PDEV_IRQ_NUM_ISP0);
	pr_info("ISP IRQ number: %d\n", cfg->irq_isp0);
	if (cfg->irq_isp0 < 0) {
		err_vec("failed to get irq for %d", PDEV_IRQ_NUM_ISP0);
		goto err_get_irq_isp0;
	}

	cfg->irq_3aa0 = platform_get_irq(core->pdev, PDEV_IRQ_NUM_3AA0);
	pr_info("ISP IRQ number: %d\n", cfg->irq_3aa0);
	if (cfg->irq_3aa0 < 0) {
		err_vec("failed to get irq for %d", PDEV_IRQ_NUM_3AA0);
		goto err_get_irq_3aa0;
	}
#elif defined(CONFIG_SOC_EXYNOS7570)
	cfg->irq_mcsc = platform_get_irq(core->pdev, PDEV_IRQ_NUM_MCSC);
	pr_info("MCSC IRQ number: %d\n", cfg->irq_mcsc);
	if (cfg->irq_mcsc < 0) {
		err_vec("failed to get irq for %d", PDEV_IRQ_NUM_MCSC);
		goto err_get_irq_mcsc;
	}
	cfg->irq_vra1 = platform_get_irq(core->pdev, PDEV_IRQ_NUM_VRA1);
	pr_info("VRA IRQ number: %d\n", cfg->irq_vra1);
	if (cfg->irq_vra1 < 0) {
		err_vec("failed to get irq for %d", PDEV_IRQ_NUM_VRA1);
		goto err_get_irq_vra0;
	}

	cfg->irq_isp0 = platform_get_irq(core->pdev, PDEV_IRQ_NUM_ISP0);
	pr_info("ISP IRQ number: %d\n", cfg->irq_isp0);
	if (cfg->irq_isp0 < 0) {
		err_vec("failed to get irq for %d", PDEV_IRQ_NUM_ISP0);
		goto err_get_irq_isp0;
	}

	cfg->irq_mfcmcsc = platform_get_irq(core->pdev, PDEV_IRQ_NUM_MFCMCSC);
	pr_info("ISP IRQ number: %d\n", cfg->irq_mfcmcsc);
	if (cfg->irq_mfcmcsc < 0) {
		err_vec("failed to get irq for %d", PDEV_IRQ_NUM_MFCMCSC);
		goto err_get_irq_mfcmcsc;
	}
#endif
	init_waitqueue_head(&cfg->wait);
#endif

	vender->private_data = priv;

	return 0;
#ifdef CONFIG_PSV_VECTOR_VERIFICATION
err_get_irq_3aa0:
err_get_irq_3aa1:
err_get_irq_mcsc0:
err_get_irq_mcsc1:
err_get_irq_isp1:
#if defined(CONFIG_SOC_EXYNOS7880)
err_get_irq_mcsc:
	/* TODO: vb2_ion_destroy_context */
err_get_irq_vra0:
	/* TODO: vb2_ion_destroy_context */
err_get_irq_isp0:
	/* TODO: vb2_ion_destroy_context */
err_get_irq_3aa0:
	/* TODO: vb2_ion_destroy_context */
#elif defined(CONFIG_SOC_EXYNOS7570)
err_get_irq_mcsc:
	/* TODO: vb2_ion_destroy_context */
err_get_irq_vra0:
	/* TODO: vb2_ion_destroy_context */
err_get_irq_isp0:
	/* TODO: vb2_ion_destroy_context */
err_get_irq_mfcmcsc:
	/* TODO: vb2_ion_destroy_context */
#endif
#endif

err_ion_init:
	kfree(priv);

	return ret;
}

int fimc_is_vender_dt(struct device_node *np)
{
	return 0;
}

int fimc_is_vender_fw_prepare(struct fimc_is_vender *vender)
{
	return 0;
}

int fimc_is_vender_fw_filp_open(struct fimc_is_vender *vender,
		struct file **fp, int bin_type)
{
	return FW_SKIP;
}

int fimc_is_vender_preproc_fw_load(struct fimc_is_vender *vender)
{
	return 0;
}

int fimc_is_vender_cal_load(struct fimc_is_vender *vender,
		void *module_data)
{
	return 0;
}

int fimc_is_vender_module_sel(struct fimc_is_vender *vender,
	void *module_data)
{
	return 0;
}

int fimc_is_vender_module_del(struct fimc_is_vender *vender, void *module_data)
{
	return 0;
}

int fimc_is_vender_fw_sel(struct fimc_is_vender *vender)
{
	return 0;
}

int fimc_is_vender_setfile_sel(struct fimc_is_vender *vender,
		char *setfile_name)
{
	int ret = 0;

	if (setfile_name) {
		snprintf(vender->setfile_path, sizeof(vender->setfile_path), "%s%s",
				FIMC_IS_SETFILE_SDCARD_PATH, setfile_name);
		snprintf(vender->request_setfile_path,
				sizeof(vender->request_setfile_path), "%s",
				setfile_name);
	} else {
		ret = -EINVAL;
	}

	return ret;
}

int fimc_is_vender_preprocessor_gpio_on_sel(struct fimc_is_vender *vender, u32 scenario, u32 *gpio_scneario)
{
	return 0;
}

int fimc_is_vender_preprocessor_gpio_on(struct fimc_is_vender *vender, u32 scenario, u32 gpio_scenario)
{
	return 0;
}

int fimc_is_vender_sensor_gpio_on_sel(struct fimc_is_vender *vender, u32 scenario, u32 *gpio_scenario)
{
	return 0;
}

int fimc_is_vender_sensor_gpio_on(struct fimc_is_vender *vender, u32 scenario, u32 gpio_scenario)
{
	return 0;
}

int fimc_is_vender_preprocessor_gpio_off_sel(struct fimc_is_vender *vender, u32 scenario, u32 *gpio_scneario,
	void *module_data)
{
	return 0;
}

int fimc_is_vender_preprocessor_gpio_off(struct fimc_is_vender *vender, u32 scenario, u32 gpio_scenario)
{
	return 0;
}

int fimc_is_vender_sensor_gpio_off_sel(struct fimc_is_vender *vender, u32 scenario, u32 *gpio_scenario)
{
	return 0;
}

int fimc_is_vender_sensor_gpio_off(struct fimc_is_vender *vender, u32 scenario, u32 gpio_scenario)
{
	return 0;
}

void fimc_is_vender_itf_open(struct fimc_is_vender *vender, struct sensor_open_extended *ext_info)
{
	return;
}

int fimc_is_vender_set_torch(u32 aeflashMode)
{
	return 0;
}

int fimc_is_vender_video_s_ctrl(struct v4l2_control *ctrl,
		void *device_data)
{
	return 0;
}

#ifdef CONFIG_PSV_SFR_VERIFICATION
extern int fimc_is_sfr_test(void);
#endif

int fimc_is_vender_ssx_video_s_ctrl(struct v4l2_control *ctrl,
		void *device_data)
{
	struct fimc_is_device_sensor *device;
	struct fimc_is_core *core;
	int ret = 0;

	device = (struct fimc_is_device_sensor *)device_data;
	core = (struct fimc_is_core *)device->private_data;

	switch (ctrl->id) {
#ifdef CONFIG_PSV_VECTOR_VERIFICATION
	case V4L2_CID_SET_VECTOR:
		ctrl->id = VENDER_S_CTRL;
		ret = fimc_is_vector_set(core, ctrl->value);
		break;
#endif
#ifdef CONFIG_PSV_ASB
	case V4L2_CID_SET_ASB_INPUT_PATH:
		ctrl->id = VENDER_S_CTRL;
		ret = fimc_is_set_asb_input_path(core, ctrl->value);
		break;
	case V4L2_CID_SET_CAM_DMA_PATH:
		ctrl->id = VENDER_S_CTRL;
		ret = fimc_is_set_cam_dma_path(device, ctrl->value);
		break;
#endif
#ifdef CONFIG_PSV_SFR_VERIFICATION
	case V4L2_CID_IS_G_SFRTEST:
		ctrl->id = VENDER_S_CTRL;
		ctrl->value = 0;
		break;
#endif
	}

	return ret;
}

int fimc_is_vender_ssx_video_g_ctrl(struct v4l2_control *ctrl,
	void *device_data)
{
	struct fimc_is_device_sensor *device;
	struct fimc_is_core *core;
	int ret = 0;

	device = (struct fimc_is_device_sensor *)device_data;
	core = (struct fimc_is_core *)device->private_data;

	switch (ctrl->id) {
#ifdef CONFIG_PSV_VECTOR_VERIFICATION
	case V4L2_CID_GET_VECTOR:
		ctrl->id = VENDER_G_CTRL;
		ret = fimc_is_vector_get(core);
		break;
#endif
#ifdef CONFIG_PSV_SFR_VERIFICATION
	case V4L2_CID_IS_G_SFRTEST:
		ctrl->id = VENDER_G_CTRL;
		ctrl->value = fimc_is_sfr_test();
		break;
#endif
	}

	return ret;
}

void fimc_is_vender_sensor_s_input(struct fimc_is_vender *vender, void *module)
{
	return;
}

bool fimc_is_vender_wdr_mode_on(void *cis_data)
{
	return false;
}

bool fimc_is_vender_enable_wdr(void *cis_data)
{
	return false;
}
