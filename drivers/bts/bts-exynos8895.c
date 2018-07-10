/*
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/dma-mapping.h>
#include <linux/pm_qos.h>
#include <linux/suspend.h>
#include <linux/debugfs.h>
#include <linux/slab.h>

#include <soc/samsung/bts.h>
#include "cal_bts8895.h"

#define BTS_DBG(x...)		if (exynos_bts_log) pr_info(x)

#define NUM_CHANNEL		4
#define MIF_UTIL		65
#define INT_UTIL		70
#define INITIAL_MIF_FREQ	2093000

#define QBUSY_DEFAULT          4
#define QFULL_LOW_DEFAULT      0xa
#define QFULL_HIGH_DEFAULT     0x10
static unsigned int exynos_qbusy = QBUSY_DEFAULT;
static unsigned int exynos_qfull_low = QFULL_LOW_DEFAULT;
static unsigned int exynos_qfull_high = QFULL_HIGH_DEFAULT;


static int exynos_bts_log;
static unsigned int exynos_mif_util = MIF_UTIL;
static unsigned int exynos_int_util = INT_UTIL;
static unsigned int default_exynos_qmax_r[] = {0x18};
static unsigned int default_exynos_qmax_w[] = {0x6};
static unsigned int default_exynos_tq[] = {0};
static unsigned int *exynos_qmax_r;
static unsigned int exynos_nqmax_r;
static unsigned int *exynos_qmax_w;
static unsigned int exynos_nqmax_w;
static unsigned int *exynos_tq;
static unsigned int exynos_ntq;
static unsigned int exynos_mif_freq = INITIAL_MIF_FREQ;

enum bts_index {
	BTS_IDX_CP,
	BTS_IDX_DPU0,
	BTS_IDX_DPU1,
	BTS_IDX_DPU2,
	BTS_IDX_CAM0,
	BTS_IDX_CAM1,
	BTS_IDX_ISPLP,
	BTS_IDX_SRDZ,
#ifndef CONFIG_SOC_EMULATOR8895
	BTS_IDX_IVA,
	BTS_IDX_DSP,
#endif
	BTS_IDX_VPU,
	BTS_IDX_MFC0,
	BTS_IDX_MFC1,
	BTS_IDX_G2D0,
	BTS_IDX_G2D1,
	BTS_IDX_G2D2,
	BTS_IDX_ALIVE,
	BTS_IDX_FSYS0,
	BTS_IDX_PDMA,
	BTS_IDX_SPDMA,
	BTS_IDX_ABOX,
	BTS_IDX_VTS,
	BTS_IDX_FSYS1,
	BTS_IDX_GNSS,
	BTS_IDX_G3D0,
	BTS_IDX_G3D1,
	BTS_IDX_G3D2,
	BTS_IDX_G3D3,
};

enum exynos_bts_type {
	BT_TREX,
};

struct bts_table {
	struct bts_status stat;
	struct bts_info *next_bts;
	int prev_scen;
	int next_scen;
};

struct bts_info {
	const char *name;
	unsigned int pa_base;
	void __iomem *va_base;
	bool enable;
	enum exynos_bts_type type;
	struct bts_table table[BS_MAX];
	enum bts_scen_type top_scen;
};

struct bts_scenario {
	const char *name;
	struct bts_info *head;
};

static struct pm_qos_request exynos_mif_bts_qos;
static struct pm_qos_request exynos_int_bts_qos;
static DEFINE_MUTEX(media_mutex);

struct trex_info {
	unsigned int pa_base;
	void __iomem *va_base;
	unsigned int value;
	unsigned int read;
	unsigned int write;
};

static struct trex_info trex_sci_irps[] = {
	{ .pa_base = EXYNOS8895_PA_SCI_IRPS0, },
	{ .pa_base = EXYNOS8895_PA_SCI_IRPS1, },
};

static struct trex_info smc_config[] = {
	{ .pa_base = EXYNOS8895_PA_SMC0, },
	{ .pa_base = EXYNOS8895_PA_SMC1, },
	{ .pa_base = EXYNOS8895_PA_SMC2, },
	{ .pa_base = EXYNOS8895_PA_SMC3, },
};

static struct trex_info trex_snode[] = {
	{ .pa_base = EXYNOS8895_PA_SN_BUSC_M0, .value = 1, },
	{ .pa_base = EXYNOS8895_PA_SN_BUSC_M1, .value = 1, },
	{ .pa_base = EXYNOS8895_PA_SN_BUSC_M2, .value = 1, },
	{ .pa_base = EXYNOS8895_PA_SN_BUSC_M3, .value = 1, },
};
static struct trex_info trex_sci = {
	.pa_base = EXYNOS8895_PA_SCI,
};

static struct bts_info exynos_bts[] = {
	[BTS_IDX_CP] = {
		.name ="cp",
		.pa_base = EXYNOS8895_PA_CP,
		.type = BT_TREX,
		.enable = true,
		.table[BS_DEFAULT].stat.scen_en = true,
		.table[BS_DEFAULT].stat.priority = 0xc,
		.table[BS_DEFAULT].stat.timeout_en = true,
		.table[BS_DEFAULT].stat.timeout_r = 0x14,
		.table[BS_DEFAULT].stat.timeout_w = 0xa,
	},
	[BTS_IDX_DPU0] = {
		.name ="dpu0",
		.pa_base = EXYNOS8895_PA_DPU0,
		.type = BT_TREX,
		.enable = true,
		.table[BS_DEFAULT].stat.scen_en = true,
		.table[BS_DEFAULT].stat.priority = 0x8,
		.table[BS_DEFAULT].stat.rmo = 0x20,
		.table[BS_DEFAULT].stat.wmo = 0x20,
		.table[BS_DEFAULT].stat.timeout_en = true,
		.table[BS_DEFAULT].stat.timeout_r = 0x20,
		.table[BS_DEFAULT].stat.timeout_w = 0x20,
		.table[BS_DP_DEFAULT].stat.scen_en = true,
		.table[BS_DP_DEFAULT].stat.priority = 0x8,
		.table[BS_DP_DEFAULT].stat.rmo = 0x20,
		.table[BS_DP_DEFAULT].stat.wmo = 0x20,
		.table[BS_DP_DEFAULT].stat.timeout_en = true,
		.table[BS_DP_DEFAULT].stat.timeout_r = 0xc,
		.table[BS_DP_DEFAULT].stat.timeout_w = 0x6,
	},
	[BTS_IDX_DPU1] = {
		.name ="dpu1",
		.pa_base = EXYNOS8895_PA_DPU1,
		.type = BT_TREX,
		.enable = true,
		.table[BS_DEFAULT].stat.scen_en = true,
		.table[BS_DEFAULT].stat.priority = 0x8,
		.table[BS_DEFAULT].stat.rmo = 0x20,
		.table[BS_DEFAULT].stat.wmo = 0x20,
		.table[BS_DEFAULT].stat.timeout_en = true,
		.table[BS_DEFAULT].stat.timeout_r = 0x20,
		.table[BS_DEFAULT].stat.timeout_w = 0x20,
		.table[BS_DP_DEFAULT].stat.scen_en = true,
		.table[BS_DP_DEFAULT].stat.priority = 0x8,
		.table[BS_DP_DEFAULT].stat.rmo = 0x20,
		.table[BS_DP_DEFAULT].stat.wmo = 0x20,
		.table[BS_DP_DEFAULT].stat.timeout_en = true,
		.table[BS_DP_DEFAULT].stat.timeout_r = 0xc,
		.table[BS_DP_DEFAULT].stat.timeout_w = 0x6,
	},
	[BTS_IDX_DPU2] = {
		.name ="dpu2",
		.pa_base = EXYNOS8895_PA_DPU2,
		.type = BT_TREX,
		.enable = true,
		.table[BS_DEFAULT].stat.scen_en = true,
		.table[BS_DEFAULT].stat.priority = 0x8,
		.table[BS_DEFAULT].stat.rmo = 0x20,
		.table[BS_DEFAULT].stat.wmo = 0x20,
		.table[BS_DEFAULT].stat.timeout_en = true,
		.table[BS_DEFAULT].stat.timeout_r = 0x20,
		.table[BS_DEFAULT].stat.timeout_w = 0x20,
		.table[BS_DP_DEFAULT].stat.scen_en = true,
		.table[BS_DP_DEFAULT].stat.priority = 0x8,
		.table[BS_DP_DEFAULT].stat.rmo = 0x20,
		.table[BS_DP_DEFAULT].stat.wmo = 0x20,
		.table[BS_DP_DEFAULT].stat.timeout_en = true,
		.table[BS_DP_DEFAULT].stat.timeout_r = 0xc,
		.table[BS_DP_DEFAULT].stat.timeout_w = 0x6,
	},
	[BTS_IDX_CAM0] = {
		.name ="cam0",
		.pa_base = EXYNOS8895_PA_CAM0,
		.type = BT_TREX,
		.enable = true,
		.table[BS_DEFAULT].stat.scen_en = true,
		.table[BS_DEFAULT].stat.priority = 0xA,
		.table[BS_DEFAULT].stat.rmo = 0x10,
		.table[BS_DEFAULT].stat.wmo = 0x10,
		.table[BS_DEFAULT].stat.timeout_en = true,
		.table[BS_DEFAULT].stat.timeout_r = 0xc,
		.table[BS_DEFAULT].stat.timeout_w = 0x6,
	},
	[BTS_IDX_CAM1] = {
		.name ="cam1",
		.pa_base = EXYNOS8895_PA_CAM1,
		.type = BT_TREX,
		.enable = true,
		.table[BS_DEFAULT].stat.scen_en = true,
		.table[BS_DEFAULT].stat.priority = 0xA,
		.table[BS_DEFAULT].stat.rmo = 0x10,
		.table[BS_DEFAULT].stat.wmo = 0x10,
		.table[BS_DEFAULT].stat.timeout_en = true,
		.table[BS_DEFAULT].stat.timeout_r = 0xc,
		.table[BS_DEFAULT].stat.timeout_w = 0x6,
	},
	[BTS_IDX_ISPLP] = {
		.name ="isplp",
		.pa_base = EXYNOS8895_PA_ISPLP,
		.type = BT_TREX,
		.enable = true,
		.table[BS_DEFAULT].stat.scen_en = true,
		.table[BS_DEFAULT].stat.priority = 0xA,
		.table[BS_DEFAULT].stat.rmo = 0x10,
		.table[BS_DEFAULT].stat.wmo = 0x10,
		.table[BS_DEFAULT].stat.timeout_en = true,
		.table[BS_DEFAULT].stat.timeout_r = 0xc,
		.table[BS_DEFAULT].stat.timeout_w = 0x6,
	},
	[BTS_IDX_SRDZ] = {
		.name ="srdz",
		.pa_base = EXYNOS8895_PA_SRDZ,
		.type = BT_TREX,
		.enable = true,
		.table[BS_DEFAULT].stat.scen_en = true,
		.table[BS_DEFAULT].stat.priority = 0x0,
		.table[BS_DEFAULT].stat.rmo = 0x10,
		.table[BS_DEFAULT].stat.wmo = 0x10,
		.table[BS_DEFAULT].stat.max_rmo = 0x1,
		.table[BS_DEFAULT].stat.max_wmo = 0x1,
	},
#ifndef CONFIG_SOC_EMULATOR8895
	[BTS_IDX_IVA] = {
		.name ="iva",
		.pa_base = EXYNOS8895_PA_IVA,
		.type = BT_TREX,
		.enable = true,
		.table[BS_DEFAULT].stat.scen_en = true,
		.table[BS_DEFAULT].stat.priority = 0x0,
		.table[BS_DEFAULT].stat.rmo = 0x10,
		.table[BS_DEFAULT].stat.wmo = 0x10,
		.table[BS_DEFAULT].stat.max_rmo = 0x1,
		.table[BS_DEFAULT].stat.max_wmo = 0x1,
	},
	[BTS_IDX_DSP] = {
		.name ="dsp",
		.pa_base = EXYNOS8895_PA_DSP,
		.type = BT_TREX,
		.enable = true,
		.table[BS_DEFAULT].stat.scen_en = true,
		.table[BS_DEFAULT].stat.priority = 0x0,
		.table[BS_DEFAULT].stat.rmo = 0x10,
		.table[BS_DEFAULT].stat.wmo = 0x10,
		.table[BS_DEFAULT].stat.max_rmo = 0x1,
		.table[BS_DEFAULT].stat.max_wmo = 0x1,
	},
#endif
	[BTS_IDX_VPU] = {
		.name ="vpu",
		.pa_base = EXYNOS8895_PA_VPU,
		.type = BT_TREX,
		.enable = true,
		.table[BS_DEFAULT].stat.scen_en = true,
		.table[BS_DEFAULT].stat.priority = 0x0,
		.table[BS_DEFAULT].stat.rmo = 0x10,
		.table[BS_DEFAULT].stat.wmo = 0x10,
		.table[BS_DEFAULT].stat.max_rmo = 0x1,
		.table[BS_DEFAULT].stat.max_wmo = 0x1,
	},
	[BTS_IDX_MFC0] = {
		.name ="mfc0",
		.pa_base = EXYNOS8895_PA_MFC0,
		.type = BT_TREX,
		.enable = true,
		.table[BS_DEFAULT].stat.scen_en = true,
		.table[BS_DEFAULT].stat.priority = 0x0,
		.table[BS_DEFAULT].stat.rmo = 0x8,
		.table[BS_DEFAULT].stat.wmo = 0x8,
		.table[BS_DEFAULT].stat.max_rmo = 0x1,
		.table[BS_DEFAULT].stat.max_wmo = 0x1,
		.table[BS_MFC_UHD].stat.scen_en = true,
		.table[BS_MFC_UHD].stat.priority = 0x0,
		.table[BS_MFC_UHD].stat.rmo = 0x14,
		.table[BS_MFC_UHD].stat.wmo = 0x14,
		.table[BS_MFC_UHD].stat.max_rmo = 0x1,
		.table[BS_MFC_UHD].stat.max_wmo = 0x1,
		.table[BS_MFC_UHD_10BIT].stat.scen_en = true,
		.table[BS_MFC_UHD_10BIT].stat.priority = 0x0,
		.table[BS_MFC_UHD_10BIT].stat.rmo = 0x20,
		.table[BS_MFC_UHD_10BIT].stat.wmo = 0x20,
		.table[BS_MFC_UHD_10BIT].stat.max_rmo = 0x1,
		.table[BS_MFC_UHD_10BIT].stat.max_wmo = 0x1,
	},
	[BTS_IDX_MFC1] = {
		.name ="mfc1",
		.pa_base = EXYNOS8895_PA_MFC1,
		.type = BT_TREX,
		.enable = true,
		.table[BS_DEFAULT].stat.scen_en = true,
		.table[BS_DEFAULT].stat.priority = 0x0,
		.table[BS_DEFAULT].stat.rmo = 0x8,
		.table[BS_DEFAULT].stat.wmo = 0x8,
		.table[BS_DEFAULT].stat.max_rmo = 0x1,
		.table[BS_DEFAULT].stat.max_wmo = 0x1,
		.table[BS_MFC_UHD].stat.scen_en = true,
		.table[BS_MFC_UHD].stat.priority = 0x0,
		.table[BS_MFC_UHD].stat.rmo = 0x14,
		.table[BS_MFC_UHD].stat.wmo = 0x14,
		.table[BS_MFC_UHD].stat.max_rmo = 0x1,
		.table[BS_MFC_UHD].stat.max_wmo = 0x1,
		.table[BS_MFC_UHD_10BIT].stat.scen_en = true,
		.table[BS_MFC_UHD_10BIT].stat.priority = 0x0,
		.table[BS_MFC_UHD_10BIT].stat.rmo = 0x20,
		.table[BS_MFC_UHD_10BIT].stat.wmo = 0x20,
		.table[BS_MFC_UHD_10BIT].stat.max_rmo = 0x1,
		.table[BS_MFC_UHD_10BIT].stat.max_wmo = 0x1,
	},
	[BTS_IDX_G2D0] = {
		.name ="g2d0",
		.pa_base = EXYNOS8895_PA_G2D0,
		.type = BT_TREX,
		.enable = true,
		.table[BS_DEFAULT].stat.scen_en = true,
		.table[BS_DEFAULT].stat.priority = 0x0,
		.table[BS_DEFAULT].stat.rmo = 0x10,
		.table[BS_DEFAULT].stat.wmo = 0x10,
		.table[BS_DEFAULT].stat.max_rmo = 0x1,
		.table[BS_DEFAULT].stat.max_wmo = 0x1,
		.table[BS_CAMERA_DEFAULT].stat.scen_en = true,
		.table[BS_CAMERA_DEFAULT].stat.priority = 0x0,
		.table[BS_CAMERA_DEFAULT].stat.rmo = 0x8,
		.table[BS_CAMERA_DEFAULT].stat.wmo = 0x8,
		.table[BS_CAMERA_DEFAULT].stat.max_rmo = 0x1,
		.table[BS_CAMERA_DEFAULT].stat.max_wmo = 0x1,
	},
	[BTS_IDX_G2D1] = {
		.name ="g2d1",
		.pa_base = EXYNOS8895_PA_G2D1,
		.type = BT_TREX,
		.enable = true,
		.table[BS_DEFAULT].stat.scen_en = true,
		.table[BS_DEFAULT].stat.priority = 0x0,
		.table[BS_DEFAULT].stat.rmo = 0x10,
		.table[BS_DEFAULT].stat.wmo = 0x10,
		.table[BS_DEFAULT].stat.max_rmo = 0x1,
		.table[BS_DEFAULT].stat.max_wmo = 0x1,
		.table[BS_CAMERA_DEFAULT].stat.scen_en = true,
		.table[BS_CAMERA_DEFAULT].stat.priority = 0x0,
		.table[BS_CAMERA_DEFAULT].stat.rmo = 0x8,
		.table[BS_CAMERA_DEFAULT].stat.wmo = 0x8,
		.table[BS_CAMERA_DEFAULT].stat.max_rmo = 0x1,
		.table[BS_CAMERA_DEFAULT].stat.max_wmo = 0x1,
	},
	[BTS_IDX_G2D2] = {
		.name ="g2d2",
		.pa_base = EXYNOS8895_PA_G2D2,
		.type = BT_TREX,
		.enable = true,
		.table[BS_DEFAULT].stat.scen_en = true,
		.table[BS_DEFAULT].stat.priority = 0x0,
		.table[BS_DEFAULT].stat.rmo = 0x10,
		.table[BS_DEFAULT].stat.wmo = 0x10,
		.table[BS_DEFAULT].stat.max_rmo = 0x1,
		.table[BS_DEFAULT].stat.max_wmo = 0x1,
		.table[BS_CAMERA_DEFAULT].stat.scen_en = true,
		.table[BS_CAMERA_DEFAULT].stat.priority = 0x0,
		.table[BS_CAMERA_DEFAULT].stat.rmo = 0x8,
		.table[BS_CAMERA_DEFAULT].stat.wmo = 0x8,
		.table[BS_CAMERA_DEFAULT].stat.max_rmo = 0x1,
		.table[BS_CAMERA_DEFAULT].stat.max_wmo = 0x1,
	},
	[BTS_IDX_FSYS0] = {
		.name ="fsys0",
		.pa_base = EXYNOS8895_PA_FSYS0,
		.type = BT_TREX,
		.enable = true,
		.table[BS_DEFAULT].stat.scen_en = true,
		.table[BS_DEFAULT].stat.priority = 0x0,
		.table[BS_DEFAULT].stat.rmo = 0x4,
		.table[BS_DEFAULT].stat.wmo = 0x4,
		.table[BS_DEFAULT].stat.max_rmo = 0x1,
		.table[BS_DEFAULT].stat.max_wmo = 0x1,
	},
	[BTS_IDX_PDMA] = {
		.name ="pdma",
		.pa_base = EXYNOS8895_PA_PDMA,
		.type = BT_TREX,
		.enable = true,
		.table[BS_DEFAULT].stat.scen_en = true,
		.table[BS_DEFAULT].stat.priority = 0x0,
		.table[BS_DEFAULT].stat.rmo = 0x4,
		.table[BS_DEFAULT].stat.wmo = 0x4,
		.table[BS_DEFAULT].stat.max_rmo = 0x1,
		.table[BS_DEFAULT].stat.max_wmo = 0x1,
	},
	[BTS_IDX_SPDMA] = {
		.name ="spdma",
		.pa_base = EXYNOS8895_PA_SPDMA,
		.type = BT_TREX,
		.enable = true,
		.table[BS_DEFAULT].stat.scen_en = true,
		.table[BS_DEFAULT].stat.priority = 0x0,
		.table[BS_DEFAULT].stat.rmo = 0x4,
		.table[BS_DEFAULT].stat.wmo = 0x4,
		.table[BS_DEFAULT].stat.max_rmo = 0x1,
		.table[BS_DEFAULT].stat.max_wmo = 0x1,
	},
	[BTS_IDX_ABOX] = {
		.name ="abox",
		.pa_base = EXYNOS8895_PA_ABOX,
		.type = BT_TREX,
		.enable = true,
		.table[BS_DEFAULT].stat.scen_en = true,
		.table[BS_DEFAULT].stat.priority = 0xc,
	},
	[BTS_IDX_VTS] = {
		.name ="vts",
		.pa_base = EXYNOS8895_PA_VTS,
		.type = BT_TREX,
		.enable = true,
		.table[BS_DEFAULT].stat.scen_en = true,
		.table[BS_DEFAULT].stat.priority = 0x0,
		.table[BS_DEFAULT].stat.rmo = 0x4,
		.table[BS_DEFAULT].stat.wmo = 0x4,
		.table[BS_DEFAULT].stat.max_rmo = 0x1,
		.table[BS_DEFAULT].stat.max_wmo = 0x1,
	},
	[BTS_IDX_FSYS1] = {
		.name ="fsys1",
		.pa_base = EXYNOS8895_PA_FSYS1,
		.type = BT_TREX,
		.enable = true,
		.table[BS_DEFAULT].stat.scen_en = true,
		.table[BS_DEFAULT].stat.priority = 0x0,
		.table[BS_DEFAULT].stat.rmo = 0x4,
		.table[BS_DEFAULT].stat.wmo = 0x4,
		.table[BS_DEFAULT].stat.max_rmo = 0x1,
		.table[BS_DEFAULT].stat.max_wmo = 0x1,
	},
	[BTS_IDX_GNSS] = {
		.name ="gnss",
		.pa_base = EXYNOS8895_PA_GNSS,
		.type = BT_TREX,
		.enable = true,
		.table[BS_DEFAULT].stat.scen_en = true,
		.table[BS_DEFAULT].stat.priority = 0x0,
		.table[BS_DEFAULT].stat.rmo = 0x4,
		.table[BS_DEFAULT].stat.wmo = 0x4,
		.table[BS_DEFAULT].stat.max_rmo = 0x1,
		.table[BS_DEFAULT].stat.max_wmo = 0x1,
	},
	[BTS_IDX_ALIVE] = {
		.name ="alive",
		.pa_base = EXYNOS8895_PA_ALIVE,
		.type = BT_TREX,
		.enable = true,
		.table[BS_DEFAULT].stat.scen_en = true,
		.table[BS_DEFAULT].stat.priority = 0x0,
		.table[BS_DEFAULT].stat.rmo = 0x4,
		.table[BS_DEFAULT].stat.wmo = 0x4,
		.table[BS_DEFAULT].stat.max_rmo = 0x1,
		.table[BS_DEFAULT].stat.max_wmo = 0x1,
	},
	[BTS_IDX_G3D0] = {
		.name ="g3d0",
		.pa_base = EXYNOS8895_PA_G3D0,
		.type = BT_TREX,
		.enable = true,
		.table[BS_DEFAULT].stat.scen_en = true,
		.table[BS_DEFAULT].stat.priority = 0x0,
		.table[BS_DEFAULT].stat.rmo = 0x4,
		.table[BS_DEFAULT].stat.wmo = 0x4,
		.table[BS_G3D_PEFORMANCE].stat.scen_en = true,
		.table[BS_G3D_PEFORMANCE].stat.priority = 0x0,
		.table[BS_G3D_PEFORMANCE].stat.rmo = 0x10,
		.table[BS_G3D_PEFORMANCE].stat.wmo = 0x10,
		.table[BS_DP_DEFAULT].stat.scen_en = true,
		.table[BS_DP_DEFAULT].stat.priority = 0x0,
		.table[BS_DP_DEFAULT].stat.rmo = 0x4,
		.table[BS_DP_DEFAULT].stat.wmo = 0x4,
		.table[BS_CAMERA_DEFAULT].stat.scen_en = true,
		.table[BS_CAMERA_DEFAULT].stat.priority = 0x0,
		.table[BS_CAMERA_DEFAULT].stat.rmo = 0x2,
		.table[BS_CAMERA_DEFAULT].stat.wmo = 0x1,
	},
	[BTS_IDX_G3D1] = {
		.name ="g3d1",
		.pa_base = EXYNOS8895_PA_G3D1,
		.type = BT_TREX,
		.enable = true,
		.table[BS_DEFAULT].stat.scen_en = true,
		.table[BS_DEFAULT].stat.priority = 0x0,
		.table[BS_DEFAULT].stat.rmo = 0x4,
		.table[BS_DEFAULT].stat.wmo = 0x4,
		.table[BS_G3D_PEFORMANCE].stat.scen_en = true,
		.table[BS_G3D_PEFORMANCE].stat.priority = 0x0,
		.table[BS_G3D_PEFORMANCE].stat.rmo = 0x10,
		.table[BS_G3D_PEFORMANCE].stat.wmo = 0x10,
		.table[BS_DP_DEFAULT].stat.scen_en = true,
		.table[BS_DP_DEFAULT].stat.priority = 0x0,
		.table[BS_DP_DEFAULT].stat.rmo = 0x4,
		.table[BS_DP_DEFAULT].stat.wmo = 0x4,
		.table[BS_CAMERA_DEFAULT].stat.scen_en = true,
		.table[BS_CAMERA_DEFAULT].stat.priority = 0x0,
		.table[BS_CAMERA_DEFAULT].stat.rmo = 0x2,
		.table[BS_CAMERA_DEFAULT].stat.wmo = 0x1,
	},
	[BTS_IDX_G3D2] = {
		.name ="g3d1",
		.pa_base = EXYNOS8895_PA_G3D2,
		.type = BT_TREX,
		.enable = true,
		.table[BS_DEFAULT].stat.scen_en = true,
		.table[BS_DEFAULT].stat.priority = 0x0,
		.table[BS_DEFAULT].stat.rmo = 0x4,
		.table[BS_DEFAULT].stat.wmo = 0x4,
		.table[BS_G3D_PEFORMANCE].stat.scen_en = true,
		.table[BS_G3D_PEFORMANCE].stat.priority = 0x0,
		.table[BS_G3D_PEFORMANCE].stat.rmo = 0x10,
		.table[BS_G3D_PEFORMANCE].stat.wmo = 0x10,
		.table[BS_DP_DEFAULT].stat.scen_en = true,
		.table[BS_DP_DEFAULT].stat.priority = 0x0,
		.table[BS_DP_DEFAULT].stat.rmo = 0x4,
		.table[BS_DP_DEFAULT].stat.wmo = 0x4,
		.table[BS_CAMERA_DEFAULT].stat.scen_en = true,
		.table[BS_CAMERA_DEFAULT].stat.priority = 0x0,
		.table[BS_CAMERA_DEFAULT].stat.rmo = 0x2,
		.table[BS_CAMERA_DEFAULT].stat.wmo = 0x1,
	},
	[BTS_IDX_G3D3] = {
		.name ="g3d1",
		.pa_base = EXYNOS8895_PA_G3D3,
		.type = BT_TREX,
		.enable = true,
		.table[BS_DEFAULT].stat.scen_en = true,
		.table[BS_DEFAULT].stat.priority = 0x0,
		.table[BS_DEFAULT].stat.rmo = 0x4,
		.table[BS_DEFAULT].stat.wmo = 0x4,
		.table[BS_G3D_PEFORMANCE].stat.scen_en = true,
		.table[BS_G3D_PEFORMANCE].stat.priority = 0x0,
		.table[BS_G3D_PEFORMANCE].stat.rmo = 0x10,
		.table[BS_G3D_PEFORMANCE].stat.wmo = 0x10,
		.table[BS_DP_DEFAULT].stat.scen_en = true,
		.table[BS_DP_DEFAULT].stat.priority = 0x0,
		.table[BS_DP_DEFAULT].stat.rmo = 0x4,
		.table[BS_DP_DEFAULT].stat.wmo = 0x4,
		.table[BS_CAMERA_DEFAULT].stat.scen_en = true,
		.table[BS_CAMERA_DEFAULT].stat.priority = 0x0,
		.table[BS_CAMERA_DEFAULT].stat.rmo = 0x2,
		.table[BS_CAMERA_DEFAULT].stat.wmo = 0x1,
	},
};

static struct bts_scenario bts_scen[BS_MAX] = {
	[BS_DEFAULT] = {
		.name = "default",
	},
	[BS_MFC_UHD] = {
		.name = "mfc uhd",
	},
	[BS_MFC_UHD_10BIT] = {
		.name = "mfc uhd 10bit",
	},
	[BS_G3D_PEFORMANCE] = {
		.name = "g3d per",
	},
	[BS_DP_DEFAULT] = {
		.name = "dpscen",
	},
	[BS_CAMERA_DEFAULT] = {
		.name = "camscen",
	},
};

static DEFINE_SPINLOCK(bts_lock);
static DEFINE_SPINLOCK(qmax_lock);
static DEFINE_SPINLOCK(tq_lock);

static void bts_set_ip_table(struct bts_info *bts)
{
	enum bts_scen_type scen = bts->top_scen;

	BTS_DBG("[BTS] %s bts scen: [%s]->[%s]\n", bts->name,
			bts_scen[scen].name, bts_scen[scen].name);

	switch (bts->type) {
	case BT_TREX:
		bts_setqos(bts->va_base, &bts->table[scen].stat);
		break;
	default:
		break;
	}
}

static void bts_add_scen(enum bts_scen_type scen)
{
	struct bts_info *first = bts_scen[scen].head;
	struct bts_info *bts = bts_scen[scen].head;
	int next = 0;
	int prev = 0;

	if (!bts)
		return;

	BTS_DBG("[BTS] scen %s on\n", bts_scen[scen].name);

	do {
		if (bts->enable &&!bts->table[scen].next_scen) {
			if (scen >= bts->top_scen) {
				/* insert at top priority */
				bts->table[scen].prev_scen = bts->top_scen;
				bts->table[bts->top_scen].next_scen = scen;
				bts->top_scen = scen;
				bts->table[scen].next_scen = -1;

				bts_set_ip_table(bts);

			} else {
				/* insert at middle */
				for (prev = bts->top_scen; prev > scen;
				     prev = bts->table[prev].prev_scen)
					next = prev;

				bts->table[scen].prev_scen =
					bts->table[next].prev_scen;
				bts->table[scen].next_scen =
					bts->table[prev].next_scen;
				bts->table[next].prev_scen = scen;
				bts->table[prev].next_scen = scen;
			}
		}

		bts = bts->table[scen].next_bts;
	/* set all bts ip in the current scenario */
	} while (bts && bts != first);
}

static void bts_del_scen(enum bts_scen_type scen)
{
	struct bts_info *first = bts_scen[scen].head;
	struct bts_info *bts = bts_scen[scen].head;
	int next = 0;
	int prev = 0;

	if (!bts)
		return;

	BTS_DBG("[BTS] scen %s off\n", bts_scen[scen].name);

	do {
		if (bts->enable && bts->table[scen].next_scen) {
			if (scen == bts->top_scen) {
				/* revert to prev scenario */
				prev = bts->table[scen].prev_scen;
				bts->top_scen = prev;
				bts->table[prev].next_scen = -1;
				bts->table[scen].next_scen = 0;
				bts->table[scen].prev_scen = 0;

				bts_set_ip_table(bts);
			} else if (scen < bts->top_scen) {
				/* delete mid scenario */
				prev = bts->table[scen].prev_scen;
				next = bts->table[scen].next_scen;

				bts->table[next].prev_scen =
					bts->table[scen].prev_scen;
				bts->table[prev].next_scen =
					bts->table[scen].next_scen;

				bts->table[scen].prev_scen = 0;
				bts->table[scen].next_scen = 0;

			} else {
				BTS_DBG("[BTS]%s scenario couldn't \
					exist above top_scen\n",
					bts_scen[scen].name);
			}
		}

		bts = bts->table[scen].next_bts;
	/* revert all bts ip to prev in the current scenario */
	} while (bts && bts != first);
}

static unsigned int find_tq(unsigned int freq)
{
	int i;
	unsigned int ret;
	unsigned long flags;

	spin_lock_irqsave(&tq_lock, flags);

	for (i = 0; i < exynos_ntq - 1 &&
		    freq <= exynos_tq[i+1]; i += 2)
		;

	ret = exynos_tq[i];
	spin_unlock_irqrestore(&tq_lock, flags);
	return ret;
}

static void find_qmax(unsigned int freq, unsigned int *read_mo,
			      unsigned int *write_mo)
{
	int i;
	unsigned long flags;

	spin_lock_irqsave(&qmax_lock, flags);

	for (i = 0; i < exynos_nqmax_r - 1 &&
		    freq <= exynos_qmax_r[i+1]; i += 2)
		;
	*read_mo = exynos_qmax_r[i];
	for (i = 0; i < exynos_nqmax_w - 1 &&
		    freq <= exynos_qmax_w[i+1]; i += 2)
		;
	*write_mo = exynos_qmax_w[i];
	spin_unlock_irqrestore(&qmax_lock, flags);
}

static void exynos_bts_mif_update(unsigned int freq)
{
	int i;
	unsigned int read_mo;
	unsigned int write_mo;
	unsigned int tq;
	find_qmax(freq, &read_mo, &write_mo);
	for (i = 0; i < ARRAY_SIZE(trex_snode); i++)
		if (trex_snode[i].read != read_mo ||
		    trex_snode[i].write != write_mo) {
			bts_set_qmax(trex_snode[i].va_base,
				     read_mo * trex_snode[i].value,
				     write_mo * trex_snode[i].value);
			trex_snode[i].read = read_mo;
			trex_snode[i].write = write_mo;
		}
	tq = find_tq(freq);
	if (trex_sci.value != tq) {
		bts_set_tq(trex_sci.va_base, tq);
		trex_sci.value = tq;
	}
	exynos_mif_freq = freq;
}

void bts_update_scen(enum bts_scen_type scen, unsigned int val)
{
	bool on = val ? 1 : 0;

	if (scen <= BS_DEFAULT || scen >= BS_MAX)
		return;

	switch (scen) {
	case BS_MIF_CHANGE:
		exynos_bts_mif_update(val);
		break;
	default:
		spin_lock(&bts_lock);

		if (on)
			bts_add_scen(scen);
		else
			bts_del_scen(scen);

		spin_unlock(&bts_lock);
		break;
	}
}

static void scen_chaining(enum bts_scen_type scen)
{
	struct bts_info *prev = NULL;
	struct bts_info *first = NULL;
	struct bts_info *bts;

	for (bts = exynos_bts;
	     bts <= &exynos_bts[ARRAY_SIZE(exynos_bts) - 1]; bts++) {
		if (bts->table[scen].stat.scen_en) {
			if (!first)
				first = bts;
			if (prev)
				prev->table[scen].next_bts = bts;

			prev = bts;
		}
	}

	if (prev)
		prev->table[scen].next_bts = first;

	bts_scen[scen].head = first;
}

static unsigned int *get_tokenized_data(const char *buf, int *num_tokens)
{
	const char *cp;
	int i;
	int ntokens = 1;
	unsigned int *tokenized_data;
	int err = -EINVAL;

	cp = buf;
	while ((cp = strpbrk(cp + 1, " :")))
		ntokens++;

	if (!(ntokens & 0x1))
		goto err;

	tokenized_data = kmalloc(ntokens * sizeof(unsigned int), GFP_KERNEL);
	if (!tokenized_data) {
		err = -ENOMEM;
		goto err;
	}

	cp = buf;
	i = 0;
	while (i < ntokens) {
		if (*cp == '0' && (*(cp + 1) == 'x' || *(cp + 1) == 'X')) {
			if (sscanf(cp, "%x", &tokenized_data[i++]) != 1)
				goto err_kfree;
		} else {
			if (sscanf(cp, "%u", &tokenized_data[i++]) != 1)
				goto err_kfree;
		}

		cp = strpbrk(cp, " :");
		if (!cp)
			break;
		cp++;
	}

	if (i != ntokens)
		goto err_kfree;

	*num_tokens = ntokens;
	return tokenized_data;

err_kfree:
	kfree(tokenized_data);
err:
	return ERR_PTR(err);
}

static int exynos_qos_status_open_show(struct seq_file *buf, void *d)
{
	struct bts_info *bts;

	spin_lock(&bts_lock);

	for (bts = exynos_bts;
	     bts <= &exynos_bts[ARRAY_SIZE(exynos_bts) - 1]; bts++) {
		if (!bts->enable)
			continue;
		seq_printf(buf, "%5s(%s): ", bts->name,
				bts_scen[bts->top_scen].name);
		switch (bts->type) {
		case BT_TREX:
			bts_showqos(bts->va_base, buf);
			break;
		default:
			seq_puts(buf, "none\n");
		}
	}

	spin_unlock(&bts_lock);

	return 0;
}

static int exynos_qos_open(struct inode *inode, struct file *file)
{
	return single_open(file, exynos_qos_status_open_show, inode->i_private);
}

static const struct file_operations debug_qos_status_fops = {
	.open		= exynos_qos_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int exynos_tq_status_open_show(struct seq_file *buf, void *d)
{
	int i;
	unsigned long flags;

	spin_lock_irqsave(&tq_lock, flags);
	for (i = 0; i < exynos_ntq; i++) {
		seq_printf(buf, "%u%s", exynos_tq[i],
			   i & 0x1 ? ":" : " ");
	}
	seq_printf(buf, "\n");
	spin_unlock_irqrestore(&tq_lock, flags);

	return 0;
}

static int exynos_tq_open(struct inode *inode, struct file *file)
{
	return single_open(file, exynos_tq_status_open_show, inode->i_private);
}

static ssize_t exynos_tq_write(struct file *file, const char __user *buf, size_t count,
			 loff_t *f_pos)
{
	int ntokens;
	unsigned int *new_tq = NULL;
	unsigned long flags;

	new_tq = get_tokenized_data(buf, &ntokens);
	if (IS_ERR(new_tq))
		return PTR_RET(new_tq);

	spin_lock_irqsave(&tq_lock, flags);
	if (exynos_tq != default_exynos_tq)
		kfree(exynos_tq);
	exynos_tq = new_tq;
	exynos_ntq = ntokens;
	spin_unlock_irqrestore(&tq_lock, flags);

	exynos_bts_mif_update(exynos_mif_freq);
	return count;
}

static const struct file_operations debug_tq_status_fops = {
	.open		= exynos_tq_open,
	.write		= exynos_tq_write,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int exynos_qmax_r_status_open_show(struct seq_file *buf, void *d)
{
	int i;
	unsigned long flags;

	spin_lock_irqsave(&qmax_lock, flags);
	for (i = 0; i < exynos_nqmax_r; i++) {
		if (i & 0x1)
			seq_printf(buf, "%u%s", exynos_qmax_r[i],
				   ":");
		else
			seq_printf(buf, "0x%x%s", exynos_qmax_r[i],
				   " ");
	}
	seq_printf(buf, "\n");
	spin_unlock_irqrestore(&qmax_lock, flags);

	return 0;
}

static int exynos_qmax_r_open(struct inode *inode, struct file *file)
{
	return single_open(file, exynos_qmax_r_status_open_show, inode->i_private);
}

static ssize_t exynos_qmax_r_write(struct file *file, const char __user *buf, size_t count,
			 loff_t *f_pos)
{
	int ntokens;
	unsigned int *new_qmax = NULL;
	unsigned long flags;

	new_qmax = get_tokenized_data(buf, &ntokens);
	if (IS_ERR(new_qmax))
		return PTR_RET(new_qmax);

	spin_lock_irqsave(&qmax_lock, flags);
	if (exynos_qmax_r != default_exynos_qmax_r)
		kfree(exynos_qmax_r);
	exynos_qmax_r = new_qmax;
	exynos_nqmax_r = ntokens;
	spin_unlock_irqrestore(&qmax_lock, flags);

	exynos_bts_mif_update(exynos_mif_freq);
	return count;
}

static const struct file_operations debug_qmax_read_status_fops = {
	.open		= exynos_qmax_r_open,
	.write		= exynos_qmax_r_write,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int exynos_qmax_w_status_open_show(struct seq_file *buf, void *d)
{
	int i;
	unsigned long flags;

	spin_lock_irqsave(&qmax_lock, flags);
	for (i = 0; i < exynos_nqmax_w; i++) {
		if (i & 0x1)
			seq_printf(buf, "%u%s", exynos_qmax_w[i],
				   ":");
		else
			seq_printf(buf, "0x%x%s", exynos_qmax_w[i],
				   " ");
	}
	seq_printf(buf, "\n");
	spin_unlock_irqrestore(&qmax_lock, flags);

	return 0;
}

static int exynos_qmax_w_open(struct inode *inode, struct file *file)
{
	return single_open(file, exynos_qmax_w_status_open_show, inode->i_private);
}

static ssize_t exynos_qmax_w_write(struct file *file, const char __user *buf, size_t count,
			 loff_t *f_pos)
{
	int ntokens;
	unsigned int *new_qmax = NULL;
	unsigned long flags;

	new_qmax = get_tokenized_data(buf, &ntokens);
	if (IS_ERR(new_qmax))
		return PTR_RET(new_qmax);

	spin_lock_irqsave(&qmax_lock, flags);
	if (exynos_qmax_w != default_exynos_qmax_w)
		kfree(exynos_qmax_w);
	exynos_qmax_w = new_qmax;
	exynos_nqmax_w = ntokens;
	spin_unlock_irqrestore(&qmax_lock, flags);

	exynos_bts_mif_update(exynos_mif_freq);
	return count;
}

static const struct file_operations debug_qmax_write_status_fops = {
	.open		= exynos_qmax_w_open,
	.write		= exynos_qmax_w_write,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int exynos_qbusy_status_open_show(struct seq_file *buf, void *d)
{
	seq_printf(buf, "%u\n", exynos_qbusy);
	return 0;
}

static int exynos_qbusy_open(struct inode *inode, struct file *file)
{
	return single_open(file, exynos_qbusy_status_open_show, inode->i_private);
}

static ssize_t exynos_qbusy_write(struct file *file, const char __user *buf, size_t count,
				  loff_t *f_pos)
{
	int i;
	if (sscanf(buf, "%u", &exynos_qbusy) != 1)
		return count;

	for (i = 0; i < ARRAY_SIZE(smc_config); i++)
		bts_set_qbusy(smc_config[i].va_base, exynos_qbusy);
	return count;
}

static const struct file_operations debug_qbusy_status_fops = {
	.open		= exynos_qbusy_open,
	.write		= exynos_qbusy_write,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int bts_debugfs(void)
{
	struct dentry *den;

	den = debugfs_create_dir("bts", NULL);
	if (IS_ERR(den)) {
		pr_err("[BTS]: could't create debugfs dir\n");
		return -ENOMEM;
	}

	debugfs_create_file("qos", 0440, den, NULL, &debug_qos_status_fops);
	debugfs_create_file("qmax_read", 0440, den, NULL,
			    &debug_qmax_read_status_fops);
	debugfs_create_file("qmax_write", 0440, den, NULL,
			    &debug_qmax_write_status_fops);
	debugfs_create_file("qbusy", 0440, den, NULL,
			    &debug_qbusy_status_fops);
	debugfs_create_file("tq", 0440, den, NULL, &debug_tq_status_fops);
	if (!debugfs_create_u32("log", 0644, den, &exynos_bts_log)) {
		pr_err("[BTS]: could't create debugfs bts log\n");
	}
	if (!debugfs_create_u32("mif_util", 0644, den, &exynos_mif_util)) {
		pr_err("[BTS]: could't create debugfs mif util\n");
	}
	if (!debugfs_create_u32("int_util", 0644, den, &exynos_int_util)) {
		pr_err("[BTS]: could't create debugfs int util\n");
	}

	return 0;
}

static void bts_initialize_domains(void)
{
	unsigned long i;
	struct bts_info *bts;

	bts_set_qfull(trex_sci.va_base, exynos_qfull_low, exynos_qfull_high);
	for (i = 0; i < ARRAY_SIZE(smc_config); i++)
		bts_set_qbusy(smc_config[i].va_base, exynos_qbusy);
	for (i = 0; i < ARRAY_SIZE(trex_sci_irps); i++)
		bts_trex_init(trex_sci_irps[i].va_base);
	spin_lock(&bts_lock);
	for (bts = exynos_bts;
	     bts <= &exynos_bts[ARRAY_SIZE(exynos_bts) - 1]; bts++) {
		if (!bts->enable)
			continue;
		bts_set_ip_table(bts);
	}
	spin_unlock(&bts_lock);
	exynos_bts_mif_update(exynos_mif_freq);
}

static int exynos_bts_notifier_event(struct notifier_block *this,
		unsigned long event,
		void *ptr)
{

	switch ((unsigned int)event) {
	case PM_POST_SUSPEND:
		bts_initialize_domains();
		return NOTIFY_OK;
		break;
	case PM_SUSPEND_PREPARE:
		return NOTIFY_OK;
		break;
	}

	return NOTIFY_DONE;
}

static struct notifier_block exynos_bts_notifier = {
	.notifier_call = exynos_bts_notifier_event,
};

#define BIT_PER_BYTE		8

static unsigned int bts_bw_calc(struct bts_decon_info *decon, int idx)
{
	struct bts_dpp_info *dpp = &decon->dpp[idx];
	unsigned int bw;
	unsigned int dst_w, dst_h;

	dst_w = dpp->dst.x2 - dpp->dst.x1;
	dst_h = dpp->dst.y2 - dpp->dst.y1;
	if (!(dst_w && dst_h))
		return 0;
	/* use multifactor for KB/s */
	bw = ((u64)dpp->src_h * dpp->src_w * dpp->bpp * decon->vclk) *
	       (decon->lcd_w*11 + 480) / decon->lcd_w / 10 /
		(BIT_PER_BYTE * dst_h * decon->lcd_w);
	return bw;
}

static unsigned int bts_find_max_bw(struct bts_decon_info *decon,
			 const struct bts_layer_position *input, int idx)
{
	struct bts_layer_position output;
	struct bts_dpp_info *dpp;
	unsigned int max = 0;
	int i;

	for (i = idx; i < BTS_DPP_MAX; i++) {
		dpp = &decon->dpp[i];
		if (!dpp->used)
			continue;
		output.y1 = input->y1 < dpp->dst.y1 ? dpp->dst.y1 : input->y1;
		output.y2 = input->y2 > dpp->dst.y2 ? dpp->dst.y2 : input->y2;
		output.x1 = input->x1 < dpp->dst.x1 ? dpp->dst.x1 : input->x1;
		output.x2 = input->x2 > dpp->dst.x2 ? dpp->dst.x2 : input->x2;
		if (output.y1 < output.y2) {
			unsigned int bw;
			bw = dpp->bw + bts_find_max_bw(decon, &output, i + 1);
			if (bw > max)
				max = bw;
		}
	}
	return max;

}

static unsigned int bts_update_decon_bw(struct bts_decon_info *decon)
{
	unsigned int max = 0;
	struct bts_dpp_info *dpp;
	int i;

	for (i = 0; i < BTS_DPP_MAX; i++) {
		dpp = &decon->dpp[i];
		if (!dpp->used)
			continue;
		dpp->bw = bts_bw_calc(decon, i);
	}
	for (i = 0; i < BTS_DPP_MAX; i++) {
		unsigned int bw;
		dpp = &decon->dpp[i];
		if (!dpp->used)
			continue;
		bw = dpp->bw + bts_find_max_bw(decon, &dpp->dst, i + 1);
		if (bw > max)
			max = bw;
	}

	return max;
}

unsigned int bts_calc_bw(enum bts_bw_type type, void *data)
{
	unsigned int bw;

	switch (type) {
	case BTS_BW_DECON0:
	case BTS_BW_DECON1:
	case BTS_BW_DECON2:
		bw = bts_update_decon_bw(data);
		break;
	default:
		bw = 0;
		break;
	}

	return bw;
}

void bts_update_bw(enum bts_bw_type type, struct bts_bw bw)
{
	static struct bts_bw ip_bw[BTS_BW_MAX];
	unsigned int mif_freq;
	unsigned int int_freq;
	unsigned int total_bw = 0;
	unsigned int bw_r = 0;
	unsigned int bw_w = 0;
	unsigned int int_bw = 0;
	int i;

	if (type >= BTS_BW_MAX)
		return;
	if (ip_bw[type].peak == bw.peak
	    && ip_bw[type].read == bw.read
	    && ip_bw[type].write == bw.write)
		return;
	mutex_lock(&media_mutex);

	ip_bw[type] = bw;
	for (i = 0; i < BTS_BW_MAX; i++) {
		if (int_bw < ip_bw[i].peak)
			int_bw = ip_bw[i].peak;
		bw_r += ip_bw[i].read;
		bw_w += ip_bw[i].write;
	}
	total_bw = bw_r + bw_w;
	if (int_bw < (bw_w / NUM_CHANNEL))
		int_bw = bw_w / NUM_CHANNEL;
	if (int_bw < (bw_r / NUM_CHANNEL))
		int_bw = bw_r / NUM_CHANNEL;

	/* MIF minimum frequency calculation as per BTS guide */
	mif_freq = total_bw * 100 / BUS_WIDTH / exynos_mif_util;
	int_freq = int_bw * 100 / BUS_WIDTH / exynos_int_util;

	pm_qos_update_request(&exynos_mif_bts_qos, mif_freq);
	pm_qos_update_request(&exynos_int_bts_qos, int_freq);

	BTS_DBG("[BTS] BW(KB/s): type%i bw %up %ur %uw, "
		"int %u total %u, read %u, write %u, "
		"freq(Khz): mif %u, int %u\n",
		type, bw.peak, bw.read, bw.write, int_bw, total_bw,
		bw_r, bw_w, mif_freq, int_freq);

	mutex_unlock(&media_mutex);
}

static int __init exynos_bts_init(void)
{
	int i, ret;
	struct bts_info *bts;

	ret = bts_debugfs();
	if (ret)
		return ret;

	for (bts = exynos_bts;
	     bts <= &exynos_bts[ARRAY_SIZE(exynos_bts) - 1]; bts++) {
		bts->va_base = ioremap(bts->pa_base, SZ_2K);
		if (!bts->va_base) {
			pr_err("failed to map bts physical address\n");
			bts->enable = false;
		}
	}

	for (i = 0; i < ARRAY_SIZE(trex_sci_irps); i++) {
		trex_sci_irps[i].va_base = ioremap(trex_sci_irps[i].pa_base,
						  SZ_4K);
		if (!trex_sci_irps[i].va_base) {
			pr_err("failed to map trex irp-s physical address\n");
		}
	}

	for (i = 0; i < ARRAY_SIZE(smc_config); i++) {
		smc_config[i].va_base = ioremap(smc_config[i].pa_base,
						SZ_4K);
		if (!smc_config[i].va_base) {
			pr_err("failed to map smc physical address\n");
		}
	}

	for (i = 0; i < ARRAY_SIZE(trex_snode); i++) {
		trex_snode[i].va_base = ioremap(trex_snode[i].pa_base,
						  SZ_1K);
		if (!trex_snode[i].va_base) {
			pr_err("failed to map trex snode physical address\n");
		}
	}
	trex_sci.va_base = ioremap(trex_sci.pa_base,
					SZ_1K);
	if (!trex_sci.va_base) {
		pr_err("failed to map sci physical address\n");
	}

	for (i = BS_DEFAULT + 1; i < BS_MAX; i++) {
		scen_chaining(i);
	}
	exynos_tq = default_exynos_tq;
	exynos_ntq = ARRAY_SIZE(default_exynos_tq);
	exynos_qmax_r = default_exynos_qmax_r;
	exynos_nqmax_r = ARRAY_SIZE(default_exynos_qmax_r);
	exynos_qmax_w = default_exynos_qmax_w;
	exynos_nqmax_w = ARRAY_SIZE(default_exynos_qmax_w);

	bts_initialize_domains();

	pm_qos_add_request(&exynos_mif_bts_qos, PM_QOS_BUS_THROUGHPUT, 0);
	pm_qos_add_request(&exynos_int_bts_qos, PM_QOS_DEVICE_THROUGHPUT, 0);

	register_pm_notifier(&exynos_bts_notifier);
	pr_info("BTS: driver is initialized\n");
	return 0;
}
arch_initcall(exynos_bts_init);
