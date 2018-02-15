/*
 * drivers/media/platform/exynos/mfc/s5p_mfc_irq.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __S5P_MFC_IRQ_H
#define __S5P_MFC_IRQ_H __FILE__

#include <linux/interrupt.h>

#include "s5p_mfc_common.h"

irqreturn_t s5p_mfc_top_half_irq(int irq, void *priv);
irqreturn_t s5p_mfc_irq(int irq, void *priv);

#endif /* __S5P_MFC_IRQ_H */
