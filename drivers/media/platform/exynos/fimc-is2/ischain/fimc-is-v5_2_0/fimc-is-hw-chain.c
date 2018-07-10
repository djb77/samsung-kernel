/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 * exynos5 fimc-is video functions
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#if defined(CONFIG_SECURE_CAMERA_USE)
#include <linux/smc.h>
#endif
#include <linux/videodev2.h>
#include <linux/videodev2_exynos_camera.h>

#include <asm/neon.h>

#include "fimc-is-config.h"
#include "fimc-is-param.h"
#include "fimc-is-type.h"
#include "fimc-is-regs.h"
#include "fimc-is-core.h"
#include "fimc-is-hw-chain.h"
#include "fimc-is-device-sensor.h"
#include "fimc-is-device-flite.h"
#include "fimc-is-device-csi.h"
#include "fimc-is-device-ischain.h"

#include "../../interface/fimc-is-interface-ischain.h"
#include "../../hardware/fimc-is-hw-control.h"
#include "../../hardware/fimc-is-hw-mcscaler-v2.h"

#if defined(CONFIG_SECURE_CAMERA_USE)
#define SECURE_ISCHAIN_PATH_MCSC_SRC0	0   /* ISPLP -> TPU0 -> MCSC0 */
#define TPU_BYPASS_ENABLE_BIT		0x1
#define TPU_BYPASS_ENABLE_SHIFT		0
#define SECURE_ISCHAIN_TPU_BYPASS	\
	((SECURE_ISCHAIN_PATH_MCSC_SRC0 & TPU_BYPASS_ENABLE_BIT) >> TPU_BYPASS_ENABLE_SHIFT)

#define itfc_to_core(x) \
	container_of(x, struct fimc_is_core, interface_ischain)
#endif

static struct fimc_is_reg sysreg_cam_regs[SYSREG_CAM_REG_CNT] = {
	{0x1004, "CAM_USER_CON1"},
};

static struct fimc_is_reg sysreg_isplp_regs[SYSREG_ISPLP_REG_CNT] = {
	{0x1038, "ISPLP_USER_CON"},
};

static struct fimc_is_reg sysreg_isphq_regs[SYSREG_ISPHQ_REG_CNT] = {
	{0x1038, "ISPHQ_USER_CON"},
};

static struct fimc_is_field sysreg_cam_fields[SYSREG_CAM_REG_FIELD_CNT] = {
	{"GLUEMUX_SRDZ_VAL",		23,	1,	RW,	0x0},	/* CAM_USER_CON1 */
	{"GLUEMUX_MC_SCALER_SC1_VAL",	21,	2,	RW,	0x0},	/* CAM_USER_CON1 */
	{"GLUEMUX_MC_SCALER_SC0_VAL",	19,	2,	RW,	0x0},	/* CAM_USER_CON1 */
	{"GLUEMUX_ISPLP_VAL",		13,	1,	RW,	0x0},	/* CAM_USER_CON1 */
	{"GLUEMUX_ISPHQ_VAL",		12,	1,	RW,	0x0},	/* CAM_USER_CON1 */
	{"GLUEMUX_WOBNS_VAL",		10,	2,	RW,	0x0},	/* CAM_USER_CON1 */
	{"GLUEMUX_BNS_VAL",		8,	2,	RW,	0x0},	/* CAM_USER_CON1 */
	{"RT_INFO_TPU1_VAL",		4,	1,	RW,	0x0},	/* CAM_USER_CON1 */
	{"RT_INFO_VRA_VAL",		3,	1,	RW,	0x0},	/* CAM_USER_CON1 */
	{"RT_INFO_TPU0_VAL",		2,	1,	RW,	0x0},	/* CAM_USER_CON1 */
	{"RT_INFO_MC_SCALER_VAL",	1,	1,	RW,	0x0},	/* CAM_USER_CON1 */
	{"RT_INFO_CSISX4_VAL",		0,	1,	RW,	0x1}	/* CAM_USER_CON1 */
};

static struct fimc_is_field sysreg_isplp_fields[SYSREG_ISPLP_REG_FIELD_CNT] = {
	{"RT_INFO_ISPLP",		11,	1,	RW,	0x0},	/* ISPLP_USER_CON */
	{"RT_INFO_3AAW",		10,	1,	RW,	0x1},	/* ISPLP_USER_CON */
	{"GLUEMUX_ISPLP_SEL",		8,	1,	RW,	0x0},	/* ISPLP_USER_CON */
	{"MCUCTL_ISPLP_AWCACHE_H",	4,	4,	RW,	0x2},	/* ISPLP_USER_CON */
	{"MCUCTL_ISPLP_ARCACHE_L",	0,	4,	RW,	0x2}	/* ISPLP_USER_CON */
};

static struct fimc_is_field sysreg_isphq_fields[SYSREG_ISPHQ_REG_FIELD_CNT] = {
	{"RT_INFO_ISPHQ",		10,	1,	RW,	0x0},	/* ISPHQ_USER_CON */
	{"RT_INFO_3AA",			9,	1,	RW,	0x1},	/* ISPHQ_USER_CON */
	{"GLUEMUX_ISPHQ_SEL",		8,	1,	RW,	0x1},	/* ISPHQ_USER_CON */
	{"MCUCTL_ISPHQ_AWCACHE_H",	4,	4,	RW,	0x2},	/* ISPHQ_USER_CON */
	{"MCUCTL_ISPHQ_ARCACHE_L",	0,	4,	RW,	0x2}	/* ISPHQ_USER_CON */
};

/* Define default subdev ops if there are not used subdev IP */
const struct fimc_is_subdev_ops fimc_is_subdev_scc_ops;
const struct fimc_is_subdev_ops fimc_is_subdev_scp_ops;

struct fimc_is_clk_gate clk_gate_3aa0;
struct fimc_is_clk_gate clk_gate_3aa1;
struct fimc_is_clk_gate clk_gate_isp0;
struct fimc_is_clk_gate clk_gate_isp1;
struct fimc_is_clk_gate clk_gate_tpu0;
struct fimc_is_clk_gate clk_gate_tpu1;
struct fimc_is_clk_gate clk_gate_vra;

void __iomem *hwfc_rst;

void fimc_is_enter_lib_isr(void)
{
	kernel_neon_begin();
}

void fimc_is_exit_lib_isr(void)
{
	kernel_neon_end();
}

void fimc_is_hw_group_init(struct fimc_is_group *group)
{
	group->subdev[ENTRY_SENSOR] = NULL;
	group->subdev[ENTRY_SSVC0] = NULL;
	group->subdev[ENTRY_SSVC1] = NULL;
	group->subdev[ENTRY_SSVC2] = NULL;
	group->subdev[ENTRY_SSVC3] = NULL;
	group->subdev[ENTRY_BNS] = NULL;
	group->subdev[ENTRY_3AA] = NULL;
	group->subdev[ENTRY_3AC] = NULL;
	group->subdev[ENTRY_3AP] = NULL;
	group->subdev[ENTRY_ISP] = NULL;
	group->subdev[ENTRY_IXC] = NULL;
	group->subdev[ENTRY_IXP] = NULL;
	group->subdev[ENTRY_DRC] = NULL;
	group->subdev[ENTRY_DIS] = NULL;
	group->subdev[ENTRY_DXC] = NULL;
	group->subdev[ENTRY_ODC] = NULL;
	group->subdev[ENTRY_DNR] = NULL;
	group->subdev[ENTRY_SCC] = NULL;
	group->subdev[ENTRY_SCP] = NULL;
	group->subdev[ENTRY_MCS] = NULL;
	group->subdev[ENTRY_M0P] = NULL;
	group->subdev[ENTRY_M1P] = NULL;
	group->subdev[ENTRY_M2P] = NULL;
	group->subdev[ENTRY_M3P] = NULL;
	group->subdev[ENTRY_M4P] = NULL;
	group->subdev[ENTRY_VRA] = NULL;
	group->subdev[ENTRY_DCP] = NULL;
	group->subdev[ENTRY_SRDZ] = NULL;
}

int fimc_is_hw_group_cfg(void *group_data)
{
	int ret = 0;
	struct fimc_is_group *group;
	struct fimc_is_device_sensor *sensor;
	struct fimc_is_device_ischain *device;

	BUG_ON(!group_data);

	group = (struct fimc_is_group *)group_data;

#ifdef CONFIG_USE_SENSOR_GROUP
	if (group->slot == GROUP_SLOT_SENSOR) {
		sensor = group->sensor;
		if (!sensor) {
			err("device is NULL");
			BUG();
		}

		fimc_is_hw_group_init(group);
		group->subdev[ENTRY_SENSOR] = &sensor->group_sensor.leader;
		group->subdev[ENTRY_SSVC0] = &sensor->ssvc0;
		group->subdev[ENTRY_SSVC1] = &sensor->ssvc1;
		group->subdev[ENTRY_SSVC2] = &sensor->ssvc2;
		group->subdev[ENTRY_SSVC3] = &sensor->ssvc3;
		group->subdev[ENTRY_BNS] = &sensor->bns;

		return ret;
	}
#endif

	device = group->device;

	if (!device) {
		err("device is NULL");
		BUG();
	}

	switch (group->slot) {
	case GROUP_SLOT_3AA:
		fimc_is_hw_group_init(group);
		group->subdev[ENTRY_3AA] = &device->group_3aa.leader;
		group->subdev[ENTRY_3AC] = &device->txc;
		group->subdev[ENTRY_3AP] = &device->txp;

		device->txc.param_dma_ot = PARAM_3AA_VDMA4_OUTPUT;
		device->txp.param_dma_ot = PARAM_3AA_VDMA2_OUTPUT;
		break;
	case GROUP_SLOT_ISP:
		fimc_is_hw_group_init(group);
		group->subdev[ENTRY_ISP] = &device->group_isp.leader;
		group->subdev[ENTRY_IXC] = &device->ixc;
		group->subdev[ENTRY_IXP] = &device->ixp;
		break;
	case GROUP_SLOT_DIS:
		fimc_is_hw_group_init(group);
		group->subdev[ENTRY_DIS] = &device->group_dis.leader;
		group->subdev[ENTRY_DXC] = &device->dxc;
		group->subdev[ENTRY_ODC] = NULL;
		group->subdev[ENTRY_DNR] = &device->dnr;
		break;
	case GROUP_SLOT_MCS:
		fimc_is_hw_group_init(group);
		group->subdev[ENTRY_MCS] = &device->group_mcs.leader;
		group->subdev[ENTRY_M0P] = &device->m0p;
		group->subdev[ENTRY_M1P] = &device->m1p;
		group->subdev[ENTRY_M2P] = &device->m2p;
		group->subdev[ENTRY_M3P] = &device->m3p;
		group->subdev[ENTRY_M4P] = &device->m4p;
		group->subdev[ENTRY_VRA] = &device->group_vra.leader;

		device->m0p.param_dma_ot = PARAM_MCS_OUTPUT0;
		device->m1p.param_dma_ot = PARAM_MCS_OUTPUT1;
		device->m2p.param_dma_ot = PARAM_MCS_OUTPUT2;
		device->m3p.param_dma_ot = PARAM_MCS_OUTPUT3;
		device->m4p.param_dma_ot = PARAM_MCS_OUTPUT4;
		break;
	case GROUP_SLOT_VRA:
		fimc_is_hw_group_init(group);
		group->subdev[ENTRY_VRA] = NULL;
		break;
	default:
		probe_err("group slot(%d) is invalid", group->slot);
		BUG();
		break;
	}

	/* for hwfc: reset all REGION_IDX registers and outputs */
	hwfc_rst = ioremap(HWFC_RESET_ADDR, SZ_4);

	return ret;
}

int fimc_is_hw_group_open(void *group_data)
{
	int ret = 0;
	u32 group_id;
	struct fimc_is_subdev *leader;
	struct fimc_is_group *group;
	struct fimc_is_device_ischain *device;

	BUG_ON(!group_data);

	group = group_data;
	leader = &group->leader;
	device = group->device;
	group_id = group->id;

	switch (group_id) {
#ifdef CONFIG_USE_SENSOR_GROUP
	case GROUP_ID_SS0:
	case GROUP_ID_SS1:
	case GROUP_ID_SS2:
	case GROUP_ID_SS3:
	case GROUP_ID_SS4:
	case GROUP_ID_SS5:
#endif
	case GROUP_ID_3AA0:
	case GROUP_ID_3AA1:
		leader->constraints_width = 6532;
		leader->constraints_height = 3676;
		break;
	case GROUP_ID_ISP0:
		leader->constraints_width = 5120;
		leader->constraints_height = 3024;
		break;
	case GROUP_ID_ISP1:
		leader->constraints_width = 6532;
		leader->constraints_height = 3676;
		break;
	case GROUP_ID_DIS0:
		leader->constraints_width = 5120;
		leader->constraints_height = 3024;
		break;
	case GROUP_ID_DIS1:
		leader->constraints_width = 5120;
		leader->constraints_height = 3024;
		break;
	case GROUP_ID_MCS0:
		leader->constraints_width = 4096;
		leader->constraints_height = 3204;
		break;
	case GROUP_ID_MCS1:
		leader->constraints_width = 6532;
		leader->constraints_height = 3204;
		break;
	case GROUP_ID_VRA0:
		leader->constraints_width = 4096;
		leader->constraints_height = 2160;
		break;
	default:
		merr("group id is invalid(%d)", group, group_id);
		break;
	}

	return ret;
}

int fimc_is_hw_camif_cfg(void *sensor_data)
{
	int ret = 0;
	struct fimc_is_device_sensor *sensor;
	struct fimc_is_device_flite *flite;
	struct fimc_is_device_csi *csi;
	struct fimc_is_device_ischain *ischain;
	bool is_otf = false;

	BUG_ON(!sensor_data);

	sensor = sensor_data;
	ischain = sensor->ischain;
	flite = (struct fimc_is_device_flite *)v4l2_get_subdevdata(sensor->subdev_flite);
	csi = (struct fimc_is_device_csi *)v4l2_get_subdevdata(sensor->subdev_csi);
	is_otf = (ischain && test_bit(FIMC_IS_GROUP_OTF_INPUT, &ischain->group_3aa.state));

	/* always clear csis dummy */
	clear_bit(CSIS_DUMMY, &csi->state);

	switch (csi->instance) {
	case 0:
		clear_bit(FLITE_DUMMY, &flite->state);

#ifdef SOC_SSVC0
		csi->dma_subdev[CSI_VIRTUAL_CH_0] = &sensor->ssvc0;
#else
		csi->dma_subdev[CSI_VIRTUAL_CH_0] = NULL;
#endif
#ifdef SOC_SSVC1
		csi->dma_subdev[CSI_VIRTUAL_CH_1] = &sensor->ssvc1;
#else
		csi->dma_subdev[CSI_VIRTUAL_CH_1] = NULL;
#endif
#ifdef SOC_SSVC2
		csi->dma_subdev[CSI_VIRTUAL_CH_2] = &sensor->ssvc2;
#else
		csi->dma_subdev[CSI_VIRTUAL_CH_2] = NULL;
#endif
#ifdef SOC_SSVC3
		csi->dma_subdev[CSI_VIRTUAL_CH_3] = &sensor->ssvc3;
#else
		csi->dma_subdev[CSI_VIRTUAL_CH_3] = NULL;
#endif
		break;
	case 1:
	case 2:
	case 3:
		if (is_otf)
			clear_bit(FLITE_DUMMY, &flite->state);
		else
			set_bit(FLITE_DUMMY, &flite->state);

#ifdef SOC_SSVC0
		csi->dma_subdev[CSI_VIRTUAL_CH_0] = &sensor->ssvc0;
#else
		csi->dma_subdev[CSI_VIRTUAL_CH_0] = NULL;
#endif
#ifdef SOC_SSVC1
		csi->dma_subdev[CSI_VIRTUAL_CH_1] = &sensor->ssvc1;
#else
		csi->dma_subdev[CSI_VIRTUAL_CH_1] = NULL;
#endif
#ifdef SOC_SSVC2
		csi->dma_subdev[CSI_VIRTUAL_CH_2] = &sensor->ssvc2;
#else
		csi->dma_subdev[CSI_VIRTUAL_CH_2] = NULL;
#endif
#ifdef SOC_SSVC3
		csi->dma_subdev[CSI_VIRTUAL_CH_3] = &sensor->ssvc3;
#else
		csi->dma_subdev[CSI_VIRTUAL_CH_3] = NULL;
#endif
		break;
	default:
		merr("csi channel is invalid(%d)", sensor, csi->instance);
		break;
	}

	return ret;
}

int fimc_is_hw_camif_open(void *sensor_data)
{
	int ret = 0;
	struct fimc_is_device_sensor *sensor;
	struct fimc_is_device_flite *flite;
	struct fimc_is_device_csi *csi;

	BUG_ON(!sensor_data);

	sensor = sensor_data;
	flite = (struct fimc_is_device_flite *)v4l2_get_subdevdata(sensor->subdev_flite);
	csi = (struct fimc_is_device_csi *)v4l2_get_subdevdata(sensor->subdev_csi);

	switch (csi->instance) {
	case 0:
	case 1:
	case 2:
	case 3:
		set_bit(CSIS_DMA_ENABLE, &csi->state);
		clear_bit(FLITE_DMA_ENABLE, &flite->state);
		break;
	default:
		merr("sensor id is invalid(%d)", sensor, csi->instance);
		break;
	}

	return ret;
}

void fimc_is_hw_ischain_qe_cfg(void)
{
	void __iomem *qe_3aaw_regs;
	void __iomem *qe_isplp_regs;
	void __iomem *qe_3aa_regs;
	void __iomem *qe_isphq_regs;

	qe_3aaw_regs = ioremap_nocache(QE_3AAW_BASE_ADDR, 0x80);
	qe_isplp_regs = ioremap_nocache(QE_ISPLP_BASE_ADDR, 0x80);
	qe_3aa_regs = ioremap_nocache(QE_3AA_BASE_ADDR, 0x80);
	qe_isphq_regs = ioremap_nocache(QE_ISPHQ_BASE_ADDR, 0x80);

	writel(0x00004000, qe_3aaw_regs + 0x20);
	writel(0x00000010, qe_3aaw_regs + 0x24);
	writel(0x00004000, qe_3aaw_regs + 0x40);
	writel(0x00000010, qe_3aaw_regs + 0x44);

	writel(0x00004001, qe_3aaw_regs + 0x20);
	writel(0x00004001, qe_3aaw_regs + 0x40);
	writel(1, qe_3aaw_regs);

	writel(0x00002000, qe_isplp_regs + 0x20);
	writel(0x00000010, qe_isplp_regs + 0x24);
	writel(0x00002000, qe_isplp_regs + 0x40);
	writel(0x00000010, qe_isplp_regs + 0x44);

	writel(0x00002001, qe_isplp_regs + 0x20);
	writel(0x00002001, qe_isplp_regs + 0x40);
	writel(1, qe_isplp_regs);

	writel(0x00008000, qe_3aa_regs + 0x20);
	writel(0x00000010, qe_3aa_regs + 0x24);
	writel(0x00008000, qe_3aa_regs + 0x40);
	writel(0x00000010, qe_3aa_regs + 0x44);

	writel(0x00008001, qe_3aa_regs + 0x20);
	writel(0x00008001, qe_3aa_regs + 0x40);
	writel(1, qe_3aa_regs);

	writel(0x00001000, qe_isphq_regs + 0x20);
	writel(0x00000010, qe_isphq_regs + 0x24);
	writel(0x00001000, qe_isphq_regs + 0x40);
	writel(0x00000010, qe_isphq_regs + 0x44);

	writel(0x00001001, qe_isphq_regs + 0x20);
	writel(0x00001001, qe_isphq_regs + 0x40);
	writel(1, qe_isphq_regs);

	iounmap(qe_3aaw_regs);
	iounmap(qe_isplp_regs);
	iounmap(qe_3aa_regs);
	iounmap(qe_isphq_regs);

	dbg_hw("%s() \n", __func__);
}

int fimc_is_hw_ischain_cfg(void *ischain_data)
{
	int ret = 0;
	struct fimc_is_core *core;
	struct fimc_is_resourcemgr *resourcemgr;
	struct fimc_is_device_ischain *device;
	struct fimc_is_device_sensor *sensor;
	struct fimc_is_device_csi *csi;
	int i, sensor_cnt = 0;
	void __iomem *cam_regs = NULL;
	void __iomem *isplp_regs;
	void __iomem *isphq_regs;
	u32 isplp_val = 0, isphq_val = 0, cam_val = 0;
	u32 isplp_backup = 0, isphq_backup = 0, cam_backup = 0;
	u32 input_bns = 0, input_wobns = 1;
	u32 input_3aaw = 0, input_3aa = 1;
	u32 input_isplp = 0, input_isphq = 1;
	u32 input_mcsc_src0;
	int chain_config = 0; /* 0: pure bayer, 1: processed bayer */

	BUG_ON(!ischain_data);

	device = (struct fimc_is_device_ischain *)ischain_data;
	if (test_bit(FIMC_IS_ISCHAIN_REPROCESSING, &device->state))
		return ret;

	core = (struct fimc_is_core *)platform_get_drvdata(device->pdev);
	sensor = device->sensor;
	BUG_ON(!sensor);

	csi = (struct fimc_is_device_csi *)v4l2_get_subdevdata(sensor->subdev_csi);
	BUG_ON(!csi);

	resourcemgr = device->resourcemgr;
	chain_config = core->chain_config;

	/* checked single/dual camera */
	for (i = 0; i < FIMC_IS_STREAM_COUNT; i++)
		if (test_bit(FIMC_IS_SENSOR_OPEN, &(core->sensor[i].state)))
			sensor_cnt++;

	dbg_hw("[%d]%s(%d): sensor_cnt(%d)\n", device->instance, __func__, __LINE__, sensor_cnt);

#if defined(CONFIG_SECURE_CAMERA_USE)
	if (core->secure_state != FIMC_IS_STATE_SECURED)
#endif
	{
		cam_regs   = ioremap_nocache(SYSREG_CAM_BASE_ADDR, 0x10000);
	}
#if defined(CONFIG_SECURE_CAMERA_USE)
	if (test_bit(FIMC_IS_ISCHAIN_POWER_ON, &device->state))
#endif
	{
		isplp_regs = ioremap_nocache(SYSREG_ISPLP_BASE_ADDR, 0x10000);
		isphq_regs = ioremap_nocache(SYSREG_ISPHQ_BASE_ADDR, 0x10000);
	}
#if defined(CONFIG_SECURE_CAMERA_USE)
	if (core->secure_state != FIMC_IS_STATE_SECURED)
#endif
	{
		cam_val = fimc_is_hw_get_reg(cam_regs, &sysreg_cam_regs[SYSREG_CAM_R_USER_CON1]);
	}
#if defined(CONFIG_SECURE_CAMERA_USE)
	if (test_bit(FIMC_IS_ISCHAIN_POWER_ON, &device->state))
#endif
	{
		isplp_val = fimc_is_hw_get_reg(isplp_regs, &sysreg_isplp_regs[SYSREG_ISPLP_R_USER_CON]);
		isphq_val = fimc_is_hw_get_reg(isphq_regs, &sysreg_isphq_regs[SYSREG_ISPHQ_R_USER_CON]);
	}

	cam_backup = cam_val;
	isplp_backup = isplp_val;
	isphq_backup = isphq_val;
	/*
	 * 1 &2) Select BNS & WOBNS input
	 *    CSIS0 : 0 (BACK) <= Always
	 *    CSIS1 : 1 (FRONT)
	 *    CSIS2 : 2 (TELE)
	 *    CSIS3 : 3 (IRIS)
	 */

	if (resourcemgr->hal_version == IS_HAL_VER_1_0 && chain_config) {
		/* processed bayer */
		input_3aaw = 1; /* WOBNS output -> 3AAW */
		input_3aa  = 0; /* BNS output   -> 3AA  */

		input_isplp = 1; /* 3AA  -> ISPLP */
		input_isphq = 0; /* 3AAW -> ISPHQ */

		if (sensor_cnt > 1) {
			/* PIP scenario */
			input_bns = 0;

			if (test_bit(FIMC_IS_SENSOR_OPEN, &(core->sensor[1].state))
					&& !test_bit(FIMC_IS_SENSOR_OPEN, &(core->sensor[2].state)))
				input_wobns = 1;
			else if (!test_bit(FIMC_IS_SENSOR_OPEN, &(core->sensor[1].state))
					&& test_bit(FIMC_IS_SENSOR_OPEN, &(core->sensor[2].state)))
				input_wobns = 2;
			else
				input_wobns = 1;
		} else {
			/* SINGLE scenario */
			switch (csi->instance) {
			case 0:
				input_bns = 0;
				input_wobns = 1;
				break;
			case 1: /* Fall Through */
			case 2:
				input_bns = 0;
				input_wobns = csi->instance;
				break;
			case 3:
				/* Color IRIS */
				input_bns = 0;
				input_wobns = 1;
				break;
			default:
				input_wobns = 1;
				break;
			}
		}
	} else {
		/* pure bayer scenario */
		input_3aaw = 0; /* BNS output   -> 3AAW */
		input_3aa  = 1; /* WOBNS output -> 3AA  */

		input_isplp = 0; /* 3AAW -> ISPLP */
		input_isphq = 1; /* 3AA  -> ISPHQ */

		if (sensor_cnt > 1) {
			/* PIP scenario */
			input_bns = 0;

			if (test_bit(FIMC_IS_SENSOR_OPEN, &(core->sensor[1].state))
				&& !test_bit(FIMC_IS_SENSOR_OPEN, &(core->sensor[2].state)))
				input_wobns = 1;
			else if (!test_bit(FIMC_IS_SENSOR_OPEN, &(core->sensor[1].state))
				&& test_bit(FIMC_IS_SENSOR_OPEN, &(core->sensor[2].state)))
				input_wobns = 2;
			else
				input_wobns = 1;

			if (test_bit(FIMC_IS_SENSOR_OPEN, &(core->sensor[1].state))
				&& test_bit(FIMC_IS_SENSOR_OPEN, &(core->sensor[3].state))) {
				/* Color IRIS */
				input_3aaw = 1;
				input_3aa  = 0;
			}
		} else {
			/* SINGLE scenario */
			switch (csi->instance) {
			case 0:
				input_bns = 0;
				input_wobns = 1;
				break;
			case 1:
				input_bns = 0;
				input_wobns = csi->instance;

				if (test_bit(FIMC_IS_GROUP_OTF_INPUT, &device->group_3aa.state)) {
					/* VT call, Color IRIS */
					input_3aaw = 1;
					input_3aa  = 0;
				} else {
					/* PIP */
					input_3aaw = 0;
					input_3aa  = 1;
				}
				break;
			case 2:
				input_bns = 0;
				input_wobns = csi->instance;
				break;
			case 3:
				/* Color IRIS */
				input_bns = 0;
				input_wobns = 1;

				input_3aaw = 1;
				input_3aa  = 0;
				break;
			default:
				input_wobns = 1;
				break;
			}
		}
	}
	info_itfc("%s: input[bns:%d, wobns:%d], input[3aaw:%d, 3aa:%d], sensor_cnt(%d), chain_config(%d)\n",
		__func__, input_bns, input_wobns,
		input_3aaw, input_3aa, sensor_cnt, chain_config);
	cam_val = fimc_is_hw_set_field_value(cam_val,
		&sysreg_cam_fields[SYSREG_CAM_F_GLUEMUX_BNS_VAL], input_bns);
	cam_val = fimc_is_hw_set_field_value(cam_val,
		&sysreg_cam_fields[SYSREG_CAM_F_GLUEMUX_WOBNS_VAL], input_wobns);


	/* 3) Select 3AAW, 3AA input
	 *    0: BNS output
	 *    1: without BNS output
	 */
	cam_val = fimc_is_hw_set_field_value(cam_val,
		&sysreg_cam_fields[SYSREG_CAM_F_GLUEMUX_ISPLP_VAL], input_3aaw);
	cam_val = fimc_is_hw_set_field_value(cam_val,
		&sysreg_cam_fields[SYSREG_CAM_F_GLUEMUX_ISPHQ_VAL], input_3aa);

	/*
	 * 4) Select ISPLP(ISP0) input
	 *    3AAW(3AA0) : 0 <= Always
	 *    3AA(3AA1)  : 1
	 */
	isplp_val = fimc_is_hw_set_field_value(isplp_val,
		&sysreg_isplp_fields[SYSREG_ISPLP_F_GLUEMUX_ISPLP_SEL], input_isplp);

	/*
	 * 5) Select ISPHQ(ISP1) input
	 *    3AAW(3AA0) : 0
	 *    3AA(3AA1)  : 1 <= Always
	 */
	isphq_val = fimc_is_hw_set_field_value(isphq_val,
		&sysreg_isphq_fields[SYSREG_ISPHQ_F_GLUEMUX_ISPHQ_SEL], input_isphq);

	/*
	 * 6) Select MC-Scaler SRC0 input
	 *    ISPLP -> TPU0 -> MCSC0: 0  <= basic scenario
	 *    ISPLP -> MCSC0: 1
	 *    SRDZ  -> TPU0 -> MCSC0: 2
	 *    SRDZ  -> MCSC0: 3
	 */
	if (test_bit(FIMC_IS_GROUP_OPEN, &device->group_dis.state)) {
		dbg_itfc("[%d]%s: set MCSC SC0 input select: from TPU0\n",
			device->instance, __func__);
		input_mcsc_src0 = 0;
	} else {
		dbg_itfc("[%d]%s: set MCSC SC0 input select: from ISPLP\n",
			device->instance, __func__);
		input_mcsc_src0 = 1;
	}
#if defined(CONFIG_SECURE_CAMERA_USE)
	if (core->secure_state == FIMC_IS_STATE_SECURING)
		input_mcsc_src0 = SECURE_ISCHAIN_PATH_MCSC_SRC0;
#endif
	cam_val = fimc_is_hw_set_field_value(cam_val,
				&sysreg_cam_fields[SYSREG_CAM_F_GLUEMUX_MC_SCALER_SC0_VAL],
				input_mcsc_src0);

	/*
	 * 7) Select MC-Scaler SRC1 input
	 *    ISPHQ -> TPU1 -> MCSC1: 0
	 *    ISPHQ -> MCSC1: 1		 <= Always
	 *    SRDZ  -> TPU1 -> MCSC1: 2
	 *    SRDZ  -> MCSC1: 3
	 */
	cam_val = fimc_is_hw_set_field_value(cam_val,
		&sysreg_cam_fields[SYSREG_CAM_F_GLUEMUX_MC_SCALER_SC1_VAL], 1);

	/*
	 * 8) Select SRDZ output
	 *    SRDZ -> MCSC0: 0	<= basic scenario
	 *    SRDZ -> MCSC1: 1
	 */
	cam_val = fimc_is_hw_set_field_value(cam_val,
		&sysreg_cam_fields[SYSREG_CAM_F_GLUEMUX_SRDZ_VAL], 1);

	/* 9) Write result to register */
#if defined(CONFIG_SECURE_CAMERA_USE)
	if (core->secure_state != FIMC_IS_STATE_SECURED)
#endif
	{
		if (cam_backup != cam_val) {
			minfo("SYSREG_CAM_R_USER_CON1:(0x%08X)->(0x%08X), sensor_cnt(%d)\n",
					device, cam_backup, cam_val, sensor_cnt);
			fimc_is_hw_set_reg(cam_regs, &sysreg_cam_regs[SYSREG_CAM_R_USER_CON1], cam_val);
		}
	}

#if defined(CONFIG_SECURE_CAMERA_USE)
	if (test_bit(FIMC_IS_ISCHAIN_POWER_ON, &device->state))
#endif
	{
		if (isplp_backup != isplp_val) {
			minfo("SYSREG_ISPLP_R_USER_CON:(0x%08X)->(0x%08X)\n", device, isplp_backup, isplp_val);
			fimc_is_hw_set_reg(isplp_regs, &sysreg_isplp_regs[SYSREG_ISPLP_R_USER_CON], isplp_val);
		}
		if (isphq_backup != isphq_val) {
			minfo("SYSREG_ISPHQ_R_USER_CON:(0x%08X)->(0x%08X)\n", device, isphq_backup, isphq_val);
			fimc_is_hw_set_reg(isphq_regs, &sysreg_isphq_regs[SYSREG_ISPHQ_R_USER_CON], isphq_val);
		}
	}

	iounmap(cam_regs);
#if defined(CONFIG_SECURE_CAMERA_USE)
	if (test_bit(FIMC_IS_ISCHAIN_POWER_ON, &device->state))
#endif
	{
		iounmap(isplp_regs);
		iounmap(isphq_regs);
	}
	return ret;
}

static irqreturn_t interface_3aa_isr1(int irq, void *data)
{
	struct fimc_is_interface_hwip *itf_3aa = NULL;
	struct hwip_intr_handler *intr_3aa = NULL;

	itf_3aa = (struct fimc_is_interface_hwip *)data;
	intr_3aa = &itf_3aa->handler[INTR_HWIP1];

	if (intr_3aa->valid) {
		fimc_is_enter_lib_isr();
		intr_3aa->handler(intr_3aa->id, intr_3aa->ctx);
		fimc_is_exit_lib_isr();
	} else {
		err_itfc("[%d]3aa(1) empty handler!!", itf_3aa->id);
	}

	return IRQ_HANDLED;
}

static irqreturn_t interface_3aa_isr2(int irq, void *data)
{
	struct fimc_is_interface_hwip *itf_3aa = NULL;
	struct hwip_intr_handler *intr_3aa = NULL;

	itf_3aa = (struct fimc_is_interface_hwip *)data;
	intr_3aa = &itf_3aa->handler[INTR_HWIP2];

	if (intr_3aa->valid) {
		fimc_is_enter_lib_isr();
		intr_3aa->handler(intr_3aa->id, intr_3aa->ctx);
		fimc_is_exit_lib_isr();
	} else {
		err_itfc("[%d]3aa(2) empty handler!!", itf_3aa->id);
	}

	return IRQ_HANDLED;
}

static irqreturn_t interface_isp_isr1(int irq, void *data)
{
	struct fimc_is_interface_hwip *itf_isp = NULL;
	struct hwip_intr_handler *intr_isp = NULL;

	itf_isp = (struct fimc_is_interface_hwip *)data;
	intr_isp = &itf_isp->handler[INTR_HWIP1];

	if (intr_isp->valid) {
		fimc_is_enter_lib_isr();
		intr_isp->handler(intr_isp->id, intr_isp->ctx);
		fimc_is_exit_lib_isr();
	} else {
		err_itfc("[%d]isp(1) empty handler!!", itf_isp->id);
	}

	return IRQ_HANDLED;
}

static irqreturn_t interface_isp_isr2(int irq, void *data)
{
	struct fimc_is_interface_hwip *itf_isp = NULL;
	struct hwip_intr_handler *intr_isp = NULL;

	itf_isp = (struct fimc_is_interface_hwip *)data;
	intr_isp = &itf_isp->handler[INTR_HWIP2];

	if (intr_isp->valid) {
		fimc_is_enter_lib_isr();
		intr_isp->handler(intr_isp->id, intr_isp->ctx);
		fimc_is_exit_lib_isr();
	} else {
		err_itfc("[%d]isp(2) empty handler!!", itf_isp->id);
	}

	return IRQ_HANDLED;
}

static irqreturn_t interface_tpu_isr1(int irq, void *data)
{
	struct fimc_is_interface_hwip *itf_tpu = NULL;
	struct hwip_intr_handler *intr_tpu = NULL;

	itf_tpu = (struct fimc_is_interface_hwip *)data;
	intr_tpu = &itf_tpu->handler[INTR_HWIP1];

	if (intr_tpu->valid) {
		fimc_is_enter_lib_isr();
		intr_tpu->handler(intr_tpu->id, intr_tpu->ctx);
		fimc_is_exit_lib_isr();
	} else {
		err_itfc("[%d]tpu(1) empty handler!!", itf_tpu->id);
	}

	return IRQ_HANDLED;
}

static irqreturn_t interface_tpu_isr2(int irq, void *data)
{
	struct fimc_is_interface_hwip *itf_tpu = NULL;
	struct hwip_intr_handler *intr_tpu = NULL;

	itf_tpu = (struct fimc_is_interface_hwip *)data;
	intr_tpu = &itf_tpu->handler[INTR_HWIP2];

	if (intr_tpu->valid) {
		fimc_is_enter_lib_isr();
		intr_tpu->handler(intr_tpu->id, intr_tpu->ctx);
		fimc_is_exit_lib_isr();
	} else {
		err_itfc("[%d]tpu(2) empty handler!!", itf_tpu->id);
	}

	return IRQ_HANDLED;
}

static irqreturn_t interface_scaler_isr(int irq, void *data)
{
	struct fimc_is_interface_hwip *itf_scaler = NULL;
	struct hwip_intr_handler *intr_scaler = NULL;

	itf_scaler = (struct fimc_is_interface_hwip *)data;
	intr_scaler = &itf_scaler->handler[INTR_HWIP1];

	if (intr_scaler->valid)
		intr_scaler->handler(intr_scaler->id, (void *)itf_scaler->hw_ip);
	else
		err_itfc("[%d]scaler empty handler!!", itf_scaler->id);

	return IRQ_HANDLED;
}

static irqreturn_t interface_vra_isr1(int irq, void *data)
{
	struct fimc_is_interface_hwip *itf_vra = NULL;
	struct hwip_intr_handler *intr_vra = NULL;

	itf_vra = (struct fimc_is_interface_hwip *)data;
	intr_vra = &itf_vra->handler[INTR_HWIP1];

	if (intr_vra->valid) {
		fimc_is_enter_lib_isr();
		intr_vra->handler(intr_vra->id, (void *)itf_vra->hw_ip);
		fimc_is_exit_lib_isr();
	} else {
		err_itfc("[%d]vra(1) empty handler!!", itf_vra->id);
	}

	return IRQ_HANDLED;
}

static irqreturn_t interface_vra_isr2(int irq, void *data)
{
	struct fimc_is_interface_hwip *itf_vra = NULL;
	struct hwip_intr_handler *intr_vra = NULL;

	itf_vra = (struct fimc_is_interface_hwip *)data;
	intr_vra = &itf_vra->handler[INTR_HWIP2];

	if (intr_vra->valid) {
		fimc_is_enter_lib_isr();
		intr_vra->handler(intr_vra->id, (void *)itf_vra->hw_ip);
		fimc_is_exit_lib_isr();
	} else {
		err_itfc("[%d]vra(2) empty handler!!", itf_vra->id);
	}

	return IRQ_HANDLED;
}

inline int fimc_is_hw_slot_id(int hw_id)
{
	int slot_id = -1;

	switch (hw_id) {
	case DEV_HW_3AA0: /* 3AAW */
		slot_id = 0;
		break;
	case DEV_HW_3AA1:
		slot_id = 1;
		break;
	case DEV_HW_ISP0: /* ISPLP */
		slot_id = 2;
		break;
	case DEV_HW_ISP1: /* ISPHQ */
		slot_id = 3;
		break;
	case DEV_HW_TPU0:
		slot_id = 4;
		break;
	case DEV_HW_TPU1:
		slot_id = 5;
		break;
	case DEV_HW_MCSC0:
		slot_id = 6;
		break;
	case DEV_HW_MCSC1:
		slot_id = 7;
		break;
	case DEV_HW_VRA:
		slot_id = 8;
		break;
#if defined(SOC_DCP)
#error implementation is needed!!!
	case DEV_HW_DCP:
		slot_id = 9;
		break;
#endif
#if defined(SOC_SRDZ)
#error implementation is needed!!!
	case DEV_HW_SRDZ:
		slot_id = 10;
		break;
#endif
	default:
		break;
	}

	return slot_id;
}

int fimc_is_get_hw_list(int group_id, int *hw_list)
{
	int i;
	int hw_index = 0;

	/* initialization */
	for (i = 0; i < GROUP_HW_MAX; i++)
		hw_list[i] = -1;

	switch (group_id) {
	case GROUP_ID_3AA0:
		hw_list[hw_index] = DEV_HW_3AA0; hw_index++;
		break;
	case GROUP_ID_3AA1:
		hw_list[hw_index] = DEV_HW_3AA1; hw_index++;
		break;
	case GROUP_ID_ISP0:
		hw_list[hw_index] = DEV_HW_ISP0; hw_index++;
		break;
	case GROUP_ID_ISP1:
		hw_list[hw_index] = DEV_HW_ISP1; hw_index++;
		break;
	case GROUP_ID_DIS0:
		hw_list[hw_index] = DEV_HW_TPU0; hw_index++;
		break;
	case GROUP_ID_DIS1:
		hw_list[hw_index] = DEV_HW_TPU1; hw_index++;
		break;
	case GROUP_ID_MCS0:
		hw_list[hw_index] = DEV_HW_MCSC0; hw_index++;
		hw_list[hw_index] = DEV_HW_VRA; hw_index++;
		break;
	case GROUP_ID_MCS1:
		hw_list[hw_index] = DEV_HW_MCSC1; hw_index++;
		break;
	/* TODO: DCP, SRDZ */
	default:
		break;
	}

	return hw_index;
}

static int fimc_is_hw_get_clk_gate(struct fimc_is_hw_ip *hw_ip, int hw_id)
{
	int ret = 0;
	struct fimc_is_clk_gate *clk_gate = NULL;

	switch (hw_id) {
	case DEV_HW_3AA0:
		clk_gate = &clk_gate_3aa0;
		clk_gate->regs = ioremap_nocache(0x13030084, 0x4);
		if (!clk_gate->regs) {
			probe_err("Failed to remap clk_gate regs\n");
			ret = -ENOMEM;
		}
		hw_ip->clk_gate_idx = 0;
		clk_gate->bit[hw_ip->clk_gate_idx] = 0;
		clk_gate->refcnt[hw_ip->clk_gate_idx] = 0;

		spin_lock_init(&clk_gate->slock);
		break;
	case DEV_HW_3AA1:
		clk_gate = &clk_gate_3aa1;
		clk_gate->regs = ioremap_nocache(0x13130094, 0x4);
		if (!clk_gate->regs) {
			probe_err("Failed to remap clk_gate regs\n");
			ret = -ENOMEM;
		}
		hw_ip->clk_gate_idx = 0;
		clk_gate->bit[hw_ip->clk_gate_idx] = 0;
		clk_gate->refcnt[hw_ip->clk_gate_idx] = 0;

		spin_lock_init(&clk_gate->slock);
		break;
	case DEV_HW_ISP0:
		clk_gate = &clk_gate_isp0;
		clk_gate->regs = ioremap_nocache(0x13040094, 0x4);
		if (!clk_gate->regs) {
			probe_err("Failed to remap clk_gate regs\n");
			ret = -ENOMEM;
		}
		hw_ip->clk_gate_idx = 0;
		clk_gate->bit[hw_ip->clk_gate_idx] = 0;
		clk_gate->refcnt[hw_ip->clk_gate_idx] = 0;

		spin_lock_init(&clk_gate->slock);
		break;
	case DEV_HW_ISP1:
		clk_gate = &clk_gate_isp1;
		clk_gate->regs = ioremap_nocache(0x13140094, 0x4);
		if (!clk_gate->regs) {
			probe_err("Failed to remap clk_gate regs\n");
			ret = -ENOMEM;
		}
		hw_ip->clk_gate_idx = 0;
		clk_gate->bit[hw_ip->clk_gate_idx] = 0;
		clk_gate->refcnt[hw_ip->clk_gate_idx] = 0;

		spin_lock_init(&clk_gate->slock);
		break;
	case DEV_HW_MCSC0:
	case DEV_HW_MCSC1:
		/* TODO : invisible hw ip processing register */
		break;
	case DEV_HW_TPU0:
		clk_gate = &clk_gate_tpu0;
		clk_gate->regs = ioremap_nocache(0x12c40094, 0x4);
		if (!clk_gate->regs) {
			probe_err("Failed to remap clk_gate regs\n");
			ret = -ENOMEM;
		}
		hw_ip->clk_gate_idx = 0;
		clk_gate->bit[hw_ip->clk_gate_idx] = 0;
		clk_gate->refcnt[hw_ip->clk_gate_idx] = 0;

		spin_lock_init(&clk_gate->slock);
		break;
	case DEV_HW_TPU1:
		clk_gate = &clk_gate_tpu1;
		clk_gate->regs = ioremap_nocache(0x12c90094, 0x4);
		if (!clk_gate->regs) {
			probe_err("Failed to remap clk_gate regs\n");
			ret = -ENOMEM;
		}
		hw_ip->clk_gate_idx = 0;
		clk_gate->bit[hw_ip->clk_gate_idx] = 0;
		clk_gate->refcnt[hw_ip->clk_gate_idx] = 0;

		spin_lock_init(&clk_gate->slock);
		break;
	case DEV_HW_VRA:
		/* No need to control */
		break;
	default:
		probe_err("hw_id(%d) is invalid", hw_id);
		ret = -EINVAL;
	}

	hw_ip->clk_gate = clk_gate;

	return ret;
}

int fimc_is_hw_get_address(void *itfc_data, void *pdev_data, int hw_id)
{
	int ret = 0;
	struct resource *mem_res = NULL;
	struct platform_device *pdev = NULL;
	struct fimc_is_interface_hwip *itf_hwip = NULL;

	BUG_ON(!itfc_data);
	BUG_ON(!pdev_data);

	itf_hwip = (struct fimc_is_interface_hwip *)itfc_data;
	pdev = (struct platform_device *)pdev_data;

	switch (hw_id) {
	case DEV_HW_3AA0:
		mem_res = platform_get_resource(pdev, IORESOURCE_MEM, IORESOURCE_3AAW);
		if (!mem_res) {
			dev_err(&pdev->dev, "Failed to get io memory region\n");
			return -EINVAL;
		}

		itf_hwip->hw_ip->regs_start = mem_res->start;
		itf_hwip->hw_ip->regs_end = mem_res->end;
		itf_hwip->hw_ip->regs = ioremap_nocache(mem_res->start, resource_size(mem_res));
		if (!itf_hwip->hw_ip->regs) {
			dev_err(&pdev->dev, "Failed to remap io region\n");
			return -EINVAL;
		}

		info_itfc("[ID:%2d] 3AA VA(0x%p)\n", hw_id, itf_hwip->hw_ip->regs);
		break;
	case DEV_HW_3AA1:
		mem_res = platform_get_resource(pdev, IORESOURCE_MEM, IORESOURCE_3AA);
		if (!mem_res) {
			dev_err(&pdev->dev, "Failed to get io memory region\n");
			return -EINVAL;
		}

		itf_hwip->hw_ip->regs_start = mem_res->start;
		itf_hwip->hw_ip->regs_end = mem_res->end;
		itf_hwip->hw_ip->regs = ioremap_nocache(mem_res->start, resource_size(mem_res));
		if (!itf_hwip->hw_ip->regs) {
			dev_err(&pdev->dev, "Failed to remap io region\n");
			return -EINVAL;
		}

		info_itfc("[ID:%2d] 3AA VA(0x%p)\n", hw_id, itf_hwip->hw_ip->regs);
		break;
	case DEV_HW_ISP0:
		mem_res = platform_get_resource(pdev, IORESOURCE_MEM, IORESOURCE_ISPLP);
		if (!mem_res) {
			dev_err(&pdev->dev, "Failed to get io memory region\n");
			return -EINVAL;
		}

		itf_hwip->hw_ip->regs_start = mem_res->start;
		itf_hwip->hw_ip->regs_end = mem_res->end;
		itf_hwip->hw_ip->regs = ioremap_nocache(mem_res->start, resource_size(mem_res));
		if (!itf_hwip->hw_ip->regs) {
			dev_err(&pdev->dev, "Failed to remap io region\n");
			return -EINVAL;
		}

		info_itfc("[ID:%2d] ISP VA(0x%p)\n", hw_id, itf_hwip->hw_ip->regs);
		break;
	case DEV_HW_ISP1:
		mem_res = platform_get_resource(pdev, IORESOURCE_MEM, IORESOURCE_ISPHQ);
		if (!mem_res) {
			dev_err(&pdev->dev, "Failed to get io memory region\n");
			return -EINVAL;
		}
		itf_hwip->hw_ip->regs_start = mem_res->start;
		itf_hwip->hw_ip->regs_end = mem_res->end;
		itf_hwip->hw_ip->regs = ioremap_nocache(mem_res->start, resource_size(mem_res));
		if (!itf_hwip->hw_ip->regs) {
			dev_err(&pdev->dev, "Failed to remap io region\n");
			return -EINVAL;
		}

		info_itfc("[ID:%2d] ISP1 VA(0x%p)\n", hw_id, itf_hwip->hw_ip->regs);
		break;
	case DEV_HW_TPU0:
		mem_res = platform_get_resource(pdev, IORESOURCE_MEM, IORESOURCE_TPU0);
		if (!mem_res) {
			dev_err(&pdev->dev, "Failed to get io memory region\n");
			return -EINVAL;
		}

		itf_hwip->hw_ip->regs_start = mem_res->start;
		itf_hwip->hw_ip->regs_end = mem_res->end;
		itf_hwip->hw_ip->regs = ioremap_nocache(mem_res->start, resource_size(mem_res));
		if (!itf_hwip->hw_ip->regs) {
			dev_err(&pdev->dev, "Failed to remap io region\n");
			return -EINVAL;
		}

		info_itfc("[ID:%2d] TPU VA(0x%p)\n", hw_id, itf_hwip->hw_ip->regs);
		break;
	case DEV_HW_TPU1:
		mem_res = platform_get_resource(pdev, IORESOURCE_MEM, IORESOURCE_TPU1);
		if (!mem_res) {
			dev_err(&pdev->dev, "Failed to get io memory region\n");
			return -EINVAL;
		}

		itf_hwip->hw_ip->regs_start = mem_res->start;
		itf_hwip->hw_ip->regs_end = mem_res->end;
		itf_hwip->hw_ip->regs = ioremap_nocache(mem_res->start, resource_size(mem_res));
		if (!itf_hwip->hw_ip->regs) {
			dev_err(&pdev->dev, "Failed to remap io region\n");
			return -EINVAL;
		}

		info_itfc("[ID:%2d] TPU VA(0x%p)\n", hw_id, itf_hwip->hw_ip->regs);
		break;
	case DEV_HW_MCSC0:
		mem_res = platform_get_resource(pdev, IORESOURCE_MEM, IORESOURCE_MCSC);
		if (!mem_res) {
			dev_err(&pdev->dev, "Failed to get io memory region\n");
			return -EINVAL;
		}

		itf_hwip->hw_ip->regs_start = mem_res->start;
		itf_hwip->hw_ip->regs_end = mem_res->end;
		itf_hwip->hw_ip->regs = ioremap_nocache(mem_res->start, resource_size(mem_res));
		if (!itf_hwip->hw_ip->regs) {
			dev_err(&pdev->dev, "Failed to remap io region\n");
			return -EINVAL;
		}

		info_itfc("[ID:%2d] MCSC0 VA(0x%p)\n", hw_id, itf_hwip->hw_ip->regs);
		break;
	case DEV_HW_MCSC1:
		mem_res = platform_get_resource(pdev, IORESOURCE_MEM, IORESOURCE_MCSC);
		if (!mem_res) {
			dev_err(&pdev->dev, "Failed to get io memory region\n");
			return -EINVAL;
		}

		itf_hwip->hw_ip->regs_start = mem_res->start;
		itf_hwip->hw_ip->regs_end = mem_res->end;
		itf_hwip->hw_ip->regs = ioremap_nocache(mem_res->start, resource_size(mem_res));
		if (!itf_hwip->hw_ip->regs) {
			dev_err(&pdev->dev, "Failed to remap io region\n");
			return -EINVAL;
		}

		info_itfc("[ID:%2d] MCSC1 VA(0x%p)\n", hw_id, itf_hwip->hw_ip->regs);
		break;
	case DEV_HW_VRA:
		mem_res = platform_get_resource(pdev, IORESOURCE_MEM, IORESOURCE_VRA_CH0);
		if (!mem_res) {
			dev_err(&pdev->dev, "Failed to get io memory region\n");
			return -EINVAL;
		}

		itf_hwip->hw_ip->regs_start = mem_res->start;
		itf_hwip->hw_ip->regs_end = mem_res->end;
		itf_hwip->hw_ip->regs = ioremap_nocache(mem_res->start, resource_size(mem_res));
		if (!itf_hwip->hw_ip->regs) {
			dev_err(&pdev->dev, "Failed to remap io region\n");
			return -EINVAL;
		}

		info_itfc("[ID:%2d] VRA0 VA(0x%p)\n", hw_id, itf_hwip->hw_ip->regs);

		mem_res = platform_get_resource(pdev, IORESOURCE_MEM, IORESOURCE_VRA_CH1);
		if (!mem_res) {
			dev_err(&pdev->dev, "Failed to get io memory region\n");
			return -EINVAL;
		}

		itf_hwip->hw_ip->regs_b_start = mem_res->start;
		itf_hwip->hw_ip->regs_b_end = mem_res->end;
		itf_hwip->hw_ip->regs_b = ioremap_nocache(mem_res->start, resource_size(mem_res));
		if (!itf_hwip->hw_ip->regs_b) {
			dev_err(&pdev->dev, "Failed to remap io region\n");
			return -EINVAL;
		}

		info_itfc("[ID:%2d] VRA1 VA(0x%p)\n", hw_id, itf_hwip->hw_ip->regs_b);
		break;
	case DEV_HW_DCP:
		mem_res = platform_get_resource(pdev, IORESOURCE_MEM, IORESOURCE_DCP);
		if (!mem_res) {
			dev_err(&pdev->dev, "Failed to get io memory region\n");
			return -EINVAL;
		}

		itf_hwip->hw_ip->regs_start = mem_res->start;
		itf_hwip->hw_ip->regs_end = mem_res->end;
		itf_hwip->hw_ip->regs = ioremap_nocache(mem_res->start, resource_size(mem_res));
		if (!itf_hwip->hw_ip->regs) {
			dev_err(&pdev->dev, "Failed to remap io region\n");
			return -EINVAL;
		}

		info_itfc("[ID:%2d] DCP VA(0x%p)\n", hw_id, itf_hwip->hw_ip->regs);
		break;
	case DEV_HW_SRDZ:
		mem_res = platform_get_resource(pdev, IORESOURCE_MEM, IORESOURCE_SRDZ);
		if (!mem_res) {
			dev_err(&pdev->dev, "Failed to get io memory region\n");
			return -EINVAL;
		}

		itf_hwip->hw_ip->regs_start = mem_res->start;
		itf_hwip->hw_ip->regs_end = mem_res->end;
		itf_hwip->hw_ip->regs = ioremap_nocache(mem_res->start, resource_size(mem_res));
		if (!itf_hwip->hw_ip->regs) {
			dev_err(&pdev->dev, "Failed to remap io region\n");
			return -EINVAL;
		}

		info_itfc("[ID:%2d] SRDZ VA(0x%p)\n", hw_id, itf_hwip->hw_ip->regs);
		break;
	default:
		probe_err("hw_id(%d) is invalid", hw_id);
		return -EINVAL;
		break;
	}

	ret = fimc_is_hw_get_clk_gate(itf_hwip->hw_ip, hw_id);;
	if (ret)
		dev_err(&pdev->dev, "fimc_is_hw_get_clk_gate is fail\n");

	return ret;
}

int fimc_is_hw_get_irq(void *itfc_data, void *pdev_data, int hw_id)
{
	struct fimc_is_interface_hwip *itf_hwip = NULL;
	struct platform_device *pdev = NULL;
	int ret = 0;

	BUG_ON(!itfc_data);

	itf_hwip = (struct fimc_is_interface_hwip *)itfc_data;
	pdev = (struct platform_device *)pdev_data;

	switch (hw_id) {
	case DEV_HW_3AA0: /* 3AAW */
		itf_hwip->irq[INTR_HWIP1] = platform_get_irq(pdev, 0);
		if (itf_hwip->irq[INTR_HWIP1] < 0) {
			err("Failed to get irq 3aa0-1\n");
			return -EINVAL;
		}

		itf_hwip->irq[INTR_HWIP2] = platform_get_irq(pdev, 1);
		if (itf_hwip->irq[INTR_HWIP2] < 0) {
			err("Failed to get irq 3aa0-2\n");
			return -EINVAL;
		}
		break;
	case DEV_HW_3AA1:
		itf_hwip->irq[INTR_HWIP1] = platform_get_irq(pdev, 2);
		if (itf_hwip->irq[INTR_HWIP1] < 0) {
			err("Failed to get irq 3aa1-1\n");
			return -EINVAL;
		}

		itf_hwip->irq[INTR_HWIP2] = platform_get_irq(pdev, 3);
		if (itf_hwip->irq[INTR_HWIP2] < 0) {
			err("Failed to get irq 3aa0-2\n");
			return -EINVAL;
		}
		break;
	case DEV_HW_ISP0:
		itf_hwip->irq[INTR_HWIP1] = platform_get_irq(pdev, 4);
		if (itf_hwip->irq[INTR_HWIP1] < 0) {
			err("Failed to get irq isp0-1\n");
			return -EINVAL;
		}

		itf_hwip->irq[INTR_HWIP2] = platform_get_irq(pdev, 5);
		if (itf_hwip->irq[INTR_HWIP2] < 0) {
			err("Failed to get irq isp0-2\n");
			return -EINVAL;
		}
		break;
	case DEV_HW_ISP1:
		itf_hwip->irq[INTR_HWIP1] = platform_get_irq(pdev, 6);
		if (itf_hwip->irq[INTR_HWIP1] < 0) {
			err("Failed to get irq isp1-1\n");
			return -EINVAL;
		}

		itf_hwip->irq[INTR_HWIP2] = platform_get_irq(pdev, 7);
		if (itf_hwip->irq[INTR_HWIP2] < 0) {
			err("Failed to get irq isp1-2\n");
			return -EINVAL;
		}
		break;
	case DEV_HW_TPU0:
		itf_hwip->irq[INTR_HWIP1] = platform_get_irq(pdev, 8);
		if (itf_hwip->irq[INTR_HWIP1] < 0) {
			err("Failed to get irq tpu-1\n");
			return -EINVAL;
		}

		itf_hwip->irq[INTR_HWIP2] = platform_get_irq(pdev, 9);
		if (itf_hwip->irq[INTR_HWIP2] < 0) {
			err("Failed to get irq tpu-2\n");
			return -EINVAL;
		}
		break;
	case DEV_HW_TPU1:
		itf_hwip->irq[INTR_HWIP1] = platform_get_irq(pdev, 10);
		if (itf_hwip->irq[INTR_HWIP1] < 0) {
			err("Failed to get irq tpu-1\n");
			return -EINVAL;
		}

		itf_hwip->irq[INTR_HWIP2] = platform_get_irq(pdev, 11);
		if (itf_hwip->irq[INTR_HWIP2] < 0) {
			err("Failed to get irq tpu-2\n");
			return -EINVAL;
		}
		break;
	case DEV_HW_MCSC0:
		itf_hwip->irq[INTR_HWIP1] = platform_get_irq(pdev, 12);
		if (itf_hwip->irq[INTR_HWIP1] < 0) {
			err("Failed to get irq scaler\n");
			return -EINVAL;
		}
		break;
	case DEV_HW_MCSC1:
		itf_hwip->irq[INTR_HWIP1] = platform_get_irq(pdev, 13);
		if (itf_hwip->irq[INTR_HWIP1] < 0) {
			err("Failed to get irq scaler\n");
			return -EINVAL;
		}
		break;
	case DEV_HW_VRA:
		itf_hwip->irq[INTR_HWIP1] = platform_get_irq(pdev, 14);
		if (itf_hwip->irq[INTR_HWIP1] < 0) {
			err("Failed to get irq vra \n");
			return -EINVAL;
		}

		itf_hwip->irq[INTR_HWIP2] = platform_get_irq(pdev, 15);
		if (itf_hwip->irq[INTR_HWIP2] < 0) {
			err("Failed to get irq vra \n");
			return -EINVAL;
		}
		break;
	case DEV_HW_DCP:
		itf_hwip->irq[INTR_HWIP1] = platform_get_irq(pdev, 16);
		if (itf_hwip->irq[INTR_HWIP1] < 0) {
			err("Failed to get irq dcp \n");
			return -EINVAL;
		}

		itf_hwip->irq[INTR_HWIP2] = platform_get_irq(pdev, 17);
		if (itf_hwip->irq[INTR_HWIP2] < 0) {
			err("Failed to get irq dcp \n");
			return -EINVAL;
		}
		break;
	case DEV_HW_SRDZ:
		itf_hwip->irq[INTR_HWIP1] = platform_get_irq(pdev, 18);
		if (itf_hwip->irq[INTR_HWIP1] < 0) {
			err("Failed to get irq srdz \n");
			return -EINVAL;
		}
		break;
	default:
		probe_err("hw_id(%d) is invalid", hw_id);
		return -EINVAL;
		break;
	}

	return ret;
}

int fimc_is_hw_request_irq(void *itfc_data, int hw_id)
{
	struct fimc_is_interface_hwip *itf_hwip = NULL;
	u32 name_len = 0;
	int ret = 0;

	BUG_ON(!itfc_data);


	itf_hwip = (struct fimc_is_interface_hwip *)itfc_data;

	switch (hw_id) {
	case DEV_HW_3AA0:
		name_len = sizeof(itf_hwip->irq_name[INTR_HWIP1]);
		snprintf(itf_hwip->irq_name[INTR_HWIP1], name_len, "fimc3a0-1");
		ret = request_irq(itf_hwip->irq[INTR_HWIP1], interface_3aa_isr1,
			FIMC_IS_HW_IRQ_FLAG,
			itf_hwip->irq_name[INTR_HWIP1],
			itf_hwip);
		if (ret) {
			err_itfc("[ID:%d] request_irq [1] fail", hw_id);
			return -EINVAL;
		}

		name_len = sizeof(itf_hwip->irq_name[INTR_HWIP2]);
		snprintf(itf_hwip->irq_name[INTR_HWIP2], name_len, "fimc3a0-2");
		ret = request_irq(itf_hwip->irq[INTR_HWIP2], interface_3aa_isr2,
			FIMC_IS_HW_IRQ_FLAG,
			itf_hwip->irq_name[INTR_HWIP2],
			itf_hwip);
		if (ret) {
			err_itfc("[ID:%d] request_irq [2] fail", hw_id);
			return -EINVAL;
		}

		break;
	case DEV_HW_3AA1:
		name_len = sizeof(itf_hwip->irq_name[INTR_HWIP1]);
		snprintf(itf_hwip->irq_name[INTR_HWIP1], name_len, "fimc3a1-1");
		ret = request_irq(itf_hwip->irq[INTR_HWIP1], interface_3aa_isr1,
			FIMC_IS_HW_IRQ_FLAG,
			itf_hwip->irq_name[INTR_HWIP1],
			itf_hwip);
		if (ret) {
			err_itfc("[ID:%d] request_irq [1] fail", hw_id);
			return -EINVAL;
		}

		name_len = sizeof(itf_hwip->irq_name[INTR_HWIP2]);
		snprintf(itf_hwip->irq_name[INTR_HWIP2], name_len, "fimc3a1-2");
		ret = request_irq(itf_hwip->irq[INTR_HWIP2], interface_3aa_isr2,
			FIMC_IS_HW_IRQ_FLAG,
			itf_hwip->irq_name[INTR_HWIP2],
			itf_hwip);
		if (ret) {
			err_itfc("[ID:%d] request_irq [2] fail", hw_id);
			return -EINVAL;
		}
		break;
	case DEV_HW_ISP0:
		name_len = sizeof(itf_hwip->irq_name[INTR_HWIP1]);
		snprintf(itf_hwip->irq_name[INTR_HWIP1], name_len, "fimcisp0-1");
		ret = request_irq(itf_hwip->irq[INTR_HWIP1], interface_isp_isr1,
			FIMC_IS_HW_IRQ_FLAG,
			itf_hwip->irq_name[INTR_HWIP1],
			itf_hwip);
		if (ret) {
			err_itfc("[ID:%d] request_irq [1] fail", hw_id);
			return -EINVAL;
		}

		name_len = sizeof(itf_hwip->irq_name[INTR_HWIP2]);
		snprintf(itf_hwip->irq_name[INTR_HWIP2], name_len, "fimcisp0-2");
		ret = request_irq(itf_hwip->irq[INTR_HWIP2], interface_isp_isr2,
			FIMC_IS_HW_IRQ_FLAG,
			itf_hwip->irq_name[INTR_HWIP2],
			itf_hwip);
		if (ret) {
			err_itfc("[ID:%d] request_irq [2] fail", hw_id);
			return -EINVAL;
		}
		break;
	case DEV_HW_ISP1:
		name_len = sizeof(itf_hwip->irq_name[INTR_HWIP1]);
		snprintf(itf_hwip->irq_name[INTR_HWIP1], name_len, "fimcisp1-1");
		ret = request_irq(itf_hwip->irq[INTR_HWIP1], interface_isp_isr1,
			FIMC_IS_HW_IRQ_FLAG,
			itf_hwip->irq_name[INTR_HWIP1],
			itf_hwip);
		if (ret) {
			err_itfc("[ID:%d] request_irq [1] fail", hw_id);
			return -EINVAL;
		}

		name_len = sizeof(itf_hwip->irq_name[INTR_HWIP2]);
		snprintf(itf_hwip->irq_name[INTR_HWIP2], name_len, "fimcisp1-2");
		ret = request_irq(itf_hwip->irq[INTR_HWIP2], interface_isp_isr2,
			FIMC_IS_HW_IRQ_FLAG,
			itf_hwip->irq_name[INTR_HWIP2],
			itf_hwip);
		if (ret) {
			err_itfc("[ID:%d] request_irq [2] fail", hw_id);
			return -EINVAL;
		}
		break;
	case DEV_HW_TPU0:
		name_len = sizeof(itf_hwip->irq_name[INTR_HWIP1]);
		snprintf(itf_hwip->irq_name[INTR_HWIP1], name_len, "fimctpu0-1");
		ret = request_irq(itf_hwip->irq[INTR_HWIP1], interface_tpu_isr1,
			FIMC_IS_HW_IRQ_FLAG,
			itf_hwip->irq_name[INTR_HWIP1],
			itf_hwip);
		if (ret) {
			err_itfc("[ID:%d] request_irq [1] fail", hw_id);
			return -EINVAL;
		}
		itf_hwip->handler[INTR_HWIP1].valid = true;

		name_len = sizeof(itf_hwip->irq_name[INTR_HWIP2]);
		snprintf(itf_hwip->irq_name[INTR_HWIP2], name_len, "fimctpu0-2");
		ret = request_irq(itf_hwip->irq[INTR_HWIP2], interface_tpu_isr2,
			FIMC_IS_HW_IRQ_FLAG,
			itf_hwip->irq_name[INTR_HWIP2],
			itf_hwip);
		if (ret) {
			err_itfc("[ID:%d] request_irq [2] fail", hw_id);
			return -EINVAL;
		}
		itf_hwip->handler[INTR_HWIP2].valid = true;
		break;
	case DEV_HW_TPU1:
		name_len = sizeof(itf_hwip->irq_name[INTR_HWIP1]);
		snprintf(itf_hwip->irq_name[INTR_HWIP1], name_len, "fimctpu1-1");
		ret = request_irq(itf_hwip->irq[INTR_HWIP1], interface_tpu_isr1,
			FIMC_IS_HW_IRQ_FLAG,
			itf_hwip->irq_name[INTR_HWIP1],
			itf_hwip);
		if (ret) {
			err_itfc("[ID:%d] request_irq [1] fail", hw_id);
			return -EINVAL;
		}
		itf_hwip->handler[INTR_HWIP1].valid = true;

		name_len = sizeof(itf_hwip->irq_name[INTR_HWIP2]);
		snprintf(itf_hwip->irq_name[INTR_HWIP2], name_len, "fimctpu1-2");
		ret = request_irq(itf_hwip->irq[INTR_HWIP2], interface_tpu_isr2,
			FIMC_IS_HW_IRQ_FLAG,
			itf_hwip->irq_name[INTR_HWIP2],
			itf_hwip);
		if (ret) {
			err_itfc("[ID:%d] request_irq [2] fail", hw_id);
			return -EINVAL;
		}
		itf_hwip->handler[INTR_HWIP2].valid = true;
		break;
	case DEV_HW_MCSC0:
		name_len = sizeof(itf_hwip->irq_name[INTR_HWIP1]);
		snprintf(itf_hwip->irq_name[INTR_HWIP1], name_len, "fimcmcs-0");
		ret = request_irq(itf_hwip->irq[INTR_HWIP1], interface_scaler_isr,
			FIMC_IS_HW_IRQ_FLAG,
			itf_hwip->irq_name[INTR_HWIP1],
			itf_hwip);
		if (ret) {
			err_itfc("[ID:%d] request_irq [1] fail", hw_id);
			return -EINVAL;
		}
		itf_hwip->handler[INTR_HWIP1].valid = true;
		break;
	case DEV_HW_MCSC1:
		name_len = sizeof(itf_hwip->irq_name[INTR_HWIP1]);
		snprintf(itf_hwip->irq_name[INTR_HWIP1], name_len, "fimcmcs-1");
		ret = request_irq(itf_hwip->irq[INTR_HWIP1], interface_scaler_isr,
			FIMC_IS_HW_IRQ_FLAG,
			itf_hwip->irq_name[INTR_HWIP1],
			itf_hwip);
		if (ret) {
			err_itfc("[ID:%d] request_irq [1] fail", hw_id);
			return -EINVAL;
		}
		itf_hwip->handler[INTR_HWIP1].valid = true;
		break;
	case DEV_HW_VRA:
		/* VRA CH0 */
		name_len = sizeof(itf_hwip->irq_name[INTR_HWIP1]);
		snprintf(itf_hwip->irq_name[INTR_HWIP1], name_len, "fimcvra-0");
		ret = request_irq(itf_hwip->irq[INTR_HWIP1], interface_vra_isr1,
			FIMC_IS_HW_IRQ_FLAG,
			itf_hwip->irq_name[INTR_HWIP1],
			itf_hwip);
		if (ret) {
			err_itfc("[ID:%d] request_irq [1] fail", hw_id);
			return -EINVAL;
		}
		itf_hwip->handler[INTR_HWIP1].id = INTR_HWIP1;
		itf_hwip->handler[INTR_HWIP1].valid = true;

		/* VRA CH1 */
		name_len = sizeof(itf_hwip->irq_name[INTR_HWIP2]);
		snprintf(itf_hwip->irq_name[INTR_HWIP2], name_len, "fimcvra-1");
		ret = request_irq(itf_hwip->irq[INTR_HWIP2], interface_vra_isr2,
			FIMC_IS_HW_IRQ_FLAG,
			itf_hwip->irq_name[INTR_HWIP2],
			itf_hwip);
		if (ret) {
			err_itfc("[ID:%d] request_irq [2] fail", hw_id);
			return -EINVAL;
		}
		itf_hwip->handler[INTR_HWIP2].id = INTR_HWIP2;
		itf_hwip->handler[INTR_HWIP2].valid = true;
		break;
	case DEV_HW_DCP:
		name_len = sizeof(itf_hwip->irq_name[INTR_HWIP1]);
		snprintf(itf_hwip->irq_name[INTR_HWIP1], name_len, "fimcdcp-1");
		ret = request_irq(itf_hwip->irq[INTR_HWIP1], interface_tpu_isr1,
			FIMC_IS_HW_IRQ_FLAG,
			itf_hwip->irq_name[INTR_HWIP1],
			itf_hwip);
		if (ret) {
			err_itfc("[ID:%d] request_irq [1] fail", hw_id);
			return -EINVAL;
		}
		itf_hwip->handler[INTR_HWIP1].valid = true;

		name_len = sizeof(itf_hwip->irq_name[INTR_HWIP2]);
		snprintf(itf_hwip->irq_name[INTR_HWIP2], name_len, "fimcdcp-2");
		ret = request_irq(itf_hwip->irq[INTR_HWIP2], interface_tpu_isr2,
			FIMC_IS_HW_IRQ_FLAG,
			itf_hwip->irq_name[INTR_HWIP2],
			itf_hwip);
		if (ret) {
			err_itfc("[ID:%d] request_irq [2] fail", hw_id);
			return -EINVAL;
		}
		itf_hwip->handler[INTR_HWIP2].valid = true;
		break;
	case DEV_HW_SRDZ:
		name_len = sizeof(itf_hwip->irq_name[INTR_HWIP1]);
		snprintf(itf_hwip->irq_name[INTR_HWIP1], name_len, "fimcsrdz-1");
		ret = request_irq(itf_hwip->irq[INTR_HWIP1], interface_scaler_isr,
			FIMC_IS_HW_IRQ_FLAG,
			itf_hwip->irq_name[INTR_HWIP1],
			itf_hwip);
		if (ret) {
			err_itfc("[ID:%d] request_irq [1] fail", hw_id);
			return -EINVAL;
		}
		itf_hwip->handler[INTR_HWIP1].valid = true;
		break;
	default:
		probe_err("hw_id(%d) is invalid", hw_id);
		return -EINVAL;
		break;
	}

	return ret;
}

void set_hw_tpu_bypass(void __iomem *base_addr, int hw_id,
	const struct fimc_is_reg *reg, const struct fimc_is_field *field, u32 bypass)
{
	u32 field_val = 0;
	u32 cam_val = 0;
	u32 cam_backup = 0;

	cam_val = fimc_is_hw_get_reg(base_addr, reg);
	cam_backup = cam_val;

	/*
	 *  MC-Scaler SRC Input Select
	 *    ISPLP -> TPU -> MCSC: 0
	 *    ISPLP -> MCSC: 1
	 *    SRDZ  -> TPU -> MCSC: 2
	 *    SRDZ  -> MCSC: 3
	 */

	field_val = fimc_is_hw_get_field_value(cam_val, field);

	switch (field_val) {
	case 0:
		if (bypass)
			cam_val = fimc_is_hw_set_field_value(cam_val, field, 1);
		break;
	case 1:
		if (!bypass)
			cam_val = fimc_is_hw_set_field_value(cam_val, field, 0);
		break;
	case 2:
		if (bypass)
			cam_val = fimc_is_hw_set_field_value(cam_val, field, 3);
		break;
	case 3:
		if (!bypass)
			cam_val = fimc_is_hw_set_field_value(cam_val, field, 2);
		break;
	default:
		err_itfc("[ID:%d] set_hw_tpu_bypass: value(0x%x) is invalid", hw_id, cam_val);
		break;
	}

	if (cam_backup != cam_val) {
		info_itfc("%s: SYSREG_CAM_R_USER_CON1:(0x%08X)->(0x%08X), field_value(%d), bypass(%d)\n",
			__func__, cam_backup, cam_val, field_val, bypass);
		fimc_is_hw_set_reg(base_addr, reg, cam_val);
	}
}

int fimc_is_hw_s_ctrl(void *itfc_data, int hw_id, enum hw_s_ctrl_id id, void *val)
{
	int ret = 0;

	switch (id) {
	case HW_S_CTRL_FULL_BYPASS:
		{
			struct fimc_is_interface_ischain *itfc = NULL;
			unsigned long bypass = (unsigned long)val;
			void __iomem *base_addr;

#if defined(CONFIG_SECURE_CAMERA_USE)
			struct fimc_is_core *core;
#endif

			BUG_ON(!itfc_data);

			itfc = (struct fimc_is_interface_ischain *)itfc_data;
			base_addr = itfc->regs_mcuctl;

#if defined(CONFIG_SECURE_CAMERA_USE)
			core = itfc_to_core(itfc);
			mutex_lock(&core->secure_state_lock);
			if (core->secure_state == FIMC_IS_STATE_SECURED) {
				dbg_itfc("%s: fimc-is was secured\n", __func__);

				if (bypass != SECURE_ISCHAIN_TPU_BYPASS)
					warn("changing TPU bypass will be blocked [%d -> %ld]",
						SECURE_ISCHAIN_TPU_BYPASS, bypass);

				mutex_unlock(&core->secure_state_lock);

				return 0;
			}
#endif

			switch (hw_id) {
			case DEV_HW_TPU0:
				set_hw_tpu_bypass(base_addr, hw_id,
					&sysreg_cam_regs[SYSREG_CAM_R_USER_CON1],
					&sysreg_cam_fields[SYSREG_CAM_F_GLUEMUX_MC_SCALER_SC0_VAL], bypass);
				break;
			case DEV_HW_TPU1:
#if 0 /* TPU1 is enabled as M2M for all scenario */
				set_hw_tpu_bypass(base_addr, hw_id,
					&sysreg_cam_regs[SYSREG_CAM_R_USER_CON1],
					&sysreg_cam_fields[SYSREG_CAM_F_GLUEMUX_MC_SCALER_SC1_VAL], bypass);
#endif
				break;
			default:
				err_itfc("[ID:%d] hw_s_ctrl[FULL_BYPASS]: invalid hw_id", hw_id);
				ret = -EINVAL;
			}
#if defined(CONFIG_SECURE_CAMERA_USE)
			mutex_unlock(&core->secure_state_lock);
#endif
		}
		break;
	case HW_S_CTRL_CHAIN_IRQ:
		break;
	case HW_S_CTRL_HWFC_IDX_RESET:
		if (hw_id == FIMC_IS_VIDEO_M3P_NUM) {
			struct fimc_is_video_ctx *vctx = (struct fimc_is_video_ctx *)itfc_data;
			struct fimc_is_device_ischain *device;
			unsigned long data = (unsigned long)val;

			BUG_ON(!vctx);
			BUG_ON(!GET_DEVICE(vctx));

			device = GET_DEVICE(vctx);

			/* reset if this instance is reprocessing */
			if (test_bit(FIMC_IS_ISCHAIN_REPROCESSING, &device->state))
				writel(data, hwfc_rst);
		}
		break;
	default:
		break;
	}

	return ret;
}

int fimc_is_hw_g_ctrl(void *itfc_data, int hw_id, enum hw_g_ctrl_id id, void *val)
{
	int ret = 0;

	switch (id) {
	case HW_G_CTRL_FRM_DONE_WITH_DMA:
		*(bool *)val = true;
		break;
	case HW_G_CTRL_HAS_MCSC:
		*(bool *)val = true;
		break;
	case HW_G_CTRL_HAS_VRA_CH1_ONLY:
		*(bool *)val = false;
		break;
	}

	return ret;
}

int fimc_is_hw_query_cap(void *cap_data, int hw_id)
{
	int ret = 0;

	BUG_ON(!cap_data);

	switch (hw_id) {
	case DEV_HW_MCSC0:
	case DEV_HW_MCSC1:
		{
			struct fimc_is_hw_mcsc_cap *cap = (struct fimc_is_hw_mcsc_cap *)cap_data;
			/* v2.10 */
			cap->hw_ver = HW_SET_VERSION(3, 0, 0, 0);
			cap->max_output = 5;
			cap->hwfc = MCSC_CAP_SUPPORT;
			cap->in_otf = MCSC_CAP_SUPPORT;
			cap->in_dma = MCSC_CAP_SUPPORT;
			cap->out_dma[0] = MCSC_CAP_SUPPORT;
			cap->out_dma[1] = MCSC_CAP_SUPPORT;
			cap->out_dma[2] = MCSC_CAP_SUPPORT;
			cap->out_dma[3] = MCSC_CAP_SUPPORT;
			cap->out_dma[4] = MCSC_CAP_SUPPORT;
			cap->out_otf[0] = MCSC_CAP_SUPPORT;
			cap->out_otf[1] = MCSC_CAP_SUPPORT;
			cap->out_otf[2] = MCSC_CAP_SUPPORT;
			cap->out_otf[3] = MCSC_CAP_SUPPORT;
			cap->out_otf[4] = MCSC_CAP_SUPPORT;
			cap->out_hwfc[3] = MCSC_CAP_SUPPORT;
			cap->out_hwfc[4] = MCSC_CAP_SUPPORT;
			cap->enable_shared_output = true;
		}
		break;
	default:
		break;
	}

	return ret;
}

void __iomem *fimc_is_hw_get_sysreg(ulong core_regs)
{
	if (core_regs)
		err_itfc("%s: core_regs(%p)\n", __func__, (void *)core_regs);

	return ioremap_nocache(SYSREG_CAM_BASE_ADDR, 0x10000);
}
