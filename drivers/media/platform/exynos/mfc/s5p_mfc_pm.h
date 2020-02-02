/*
 * drivers/media/platform/exynos/mfc/s5p_mfc_pm.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __S5P_MFC_PM_H
#define __S5P_MFC_PM_H __FILE__

#include "s5p_mfc_common.h"

static inline int s5p_mfc_pm_get_pwr_ref_cnt(struct s5p_mfc_dev *dev)
{
	return atomic_read(&dev->pm.pwr_ref);
}

static inline int s5p_mfc_pm_get_clk_ref_cnt(struct s5p_mfc_dev *dev)
{
	return atomic_read(&dev->clk_ref);
}

void s5p_mfc_pm_init(struct s5p_mfc_dev *dev);
void s5p_mfc_pm_final(struct s5p_mfc_dev *dev);

int s5p_mfc_pm_clock_on(struct s5p_mfc_dev *dev);
int s5p_mfc_pm_clock_on_with_base(struct s5p_mfc_dev *dev,
			enum mfc_buf_usage_type buf_type);
void s5p_mfc_pm_clock_off(struct s5p_mfc_dev *dev);
int s5p_mfc_pm_power_on(struct s5p_mfc_dev *dev);
int s5p_mfc_pm_power_off(struct s5p_mfc_dev *dev);

#endif /* __S5P_MFC_PM_H */
