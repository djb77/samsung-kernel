/*
 * drivers/media/platform/exynos/mfc/s5p_mfc_watchdog.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __S5P_MFC_WATCHDOG_H
#define __S5P_MFC_WATCHDOG_H __FILE__

#include "s5p_mfc_common.h"

void s5p_mfc_dump_buffer_info(struct s5p_mfc_dev *dev, unsigned long addr);
void s5p_mfc_dump_info_and_stop_hw(struct s5p_mfc_dev *dev);
void s5p_mfc_watchdog_worker(struct work_struct *work);

#endif /* __S5P_MFC_WATCHDOG_H */
