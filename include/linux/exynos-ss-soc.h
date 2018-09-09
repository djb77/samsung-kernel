/*
 * Exynos-SnapShot for Samsung's SoC's.
 *
 * Copyright (c) 2013 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef EXYNOS_SNAPSHOT_SOC_H
#define EXYNOS_SNAPSHOT_SOC_H

/* SoC Dependent Header */
#define ESS_REG_MCT_ADDR	(0)
#define ESS_REG_MCT_SIZE	(0)
#define ESS_REG_UART_ADDR	(0)
#define ESS_REG_UART_SIZE	(0)

struct exynos_ss_ops {
        int (*pd_status)(unsigned int id);
};

extern struct exynos_ss_ops ess_ops;
#endif
