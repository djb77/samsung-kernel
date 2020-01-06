/*
 * Samsung Exynos SoC series FIMC-IS2 driver
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_HW_PDP_H
#define FIMC_IS_HW_PDP_H

#include "fimc-is-device-sensor-peri.h"

#ifdef DEBUG_DUMP_STAT0
#define ENABLE_PDP_IRQ
#endif

struct pdp_point_info {
	/* start point */
	u32 sx;
	u32 sy;
	/* end point */
	u32 ex;
	u32 ey;
};

#define MAX_SINGLE_WINDOW		4
struct pdp_single_window {
	u32 sroi;
	struct pdp_point_info size[MAX_SINGLE_WINDOW];
};

int pdp_hw_enable(u32 __iomem *base_reg, u32 pd_mode);
int pdp_hw_s_irq_msk(u32 __iomem *base_reg, u32 msk);
int pdp_hw_g_irq_state(u32 __iomem *base_reg, bool clear);
int pdp_hw_dump(u32 __iomem *base_reg);

void pdp_hw_clr_irq_state(u32 __iomem *base_reg, int state);
void pdp_hw_s_reset(u32 __iomem *base_reg, u32 val);
void pdp_hw_s_config(u32 __iomem *base_reg);
#ifdef DEBUG_DUMP_STAT0
void pdp_hw_g_paf_single_window_stat(struct work_struct *data);
#endif
void pdp_hw_g_pdp_reg(void __iomem *base_reg, struct pdp_read_reg_setting_t *reg_param);
void pdp_hw_g_paf_sfr_stats(void __iomem *base_reg, u32 *buf);
void pdp_hw_s_paf_mode(u32 __iomem *base_reg, bool onoff);
void pdp_hw_s_paf_active_size(u32 __iomem *base_reg, u32 width, u32 height);
void pdp_hw_s_paf_af_size(u32 __iomem *base_reg, u32 width, u32 height);
void pdp_hw_s_paf_crop_size(u32 __iomem *base_reg, struct pdp_point_info *size);
void pdp_hw_s_paf_main_window(u32 __iomem *base_reg, struct pdp_main_wininfo *winInfo);
void pdp_hw_s_paf_single_window(u32 __iomem *base_reg, struct pdp_single_wininfo *winInfo);
void pdp_hw_s_paf_multi_window(u32 __iomem *base_reg, struct pdp_multi_wininfo *winInfo);
void pdp_hw_s_paf_setting(u32 __iomem *base_reg, struct pdp_paf_setting_t *pafInfo);
void pdp_hw_s_paf_roi(u32 __iomem *base_reg, struct pdp_paf_roi_setting_t *roiInfo);
void pdp_hw_s_paf_knee(u32 __iomem *base_reg, struct pdp_knee_setting_t *kneeInfo);
void pdp_hw_s_paf_filter_cor(u32 __iomem *base_reg, struct pdp_filterCor_setting_t *corInfo);
void pdp_hw_s_paf_filter_bin(u32 __iomem *base_reg, struct pdp_filterBin_setting_t *binInfo);
void pdp_hw_s_paf_filter_band(u32 __iomem *base_reg, struct pdp_filterBand_setting_t *iirInfo,
		int filter_no);
void pdp_hw_s_wdr_setting(u32 __iomem *base_reg, struct pdp_wdr_setting_t *wdrInfo);
void pdp_hw_s_depth_setting(u32 __iomem *base_reg, struct pdp_depth_setting_t *depthInfo);
void pdp_hw_s_y_ext_setting(u32 __iomem *base_reg, struct pdp_YextParam_setting_t *yExtInfo);
void pdp_hw_s_paf_mpd_bin_shift(u32 __iomem *base_reg, u32 h, u32 v, u32 vsht);
void pdp_hw_s_paf_col(u32 __iomem *base_reg, u32 value);

#endif
