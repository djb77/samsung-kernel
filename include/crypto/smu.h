/*
 * Copyright (C) 2016 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _EXYNOS_SMU_H_
#define _EXYNOS_SMU_H_

#include <linux/platform_device.h>

#define CFG_DESCTYPE_0	0x0
#define CFG_DESCTYPE_1	0x1
#define CFG_DESCTYPE_2	0x2
#define CFG_DESCTYPE_3	0x3

#if defined(CONFIG_EXYNOS_FMP)
#define CFG_DESCTYPE	CFG_DESCTYPE_3
#else
#define CFG_DESCTYPE	CFG_DESCTYPE_0
#endif

#define ACCESS_CONTROL_ABORT	0x14

enum smu_id {
	SMU_EMBEDDED = 0,
	SMU_UFSCARD = 1,
	SMU_SDCARD = 2,
};

enum smu_command {
	SMU_INIT = 0,
	SMU_SET = 1,
	SMU_ABORT = 2,
};

struct smu_data_setting {
	int id;
	int command;
	int desc_type;
};

struct exynos_smu_variant_ops {
	const char *name;
	int	(*init)(struct platform_device *, struct smu_data_setting *);
	int	(*sec_config)(struct platform_device *, struct smu_data_setting *);
	int	(*resume)(struct platform_device *, struct smu_data_setting *);
	int	(*abort)(struct platform_device *, struct smu_data_setting *);
};

struct exynos_smu_variant_ops *exynos_smu_get_variant_ops(struct device_node *node);
struct platform_device *exynos_smu_get_pdevice(struct device_node *node);

#endif /* _EXYNOS_SMU_H_ */
