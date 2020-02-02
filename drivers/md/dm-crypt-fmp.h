/*
 * Copyright (C) 2016 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _DM_CRYPT_FMP_H_
#define _DM_CRYPT_FMP_H_

#ifdef CONFIG_EXYNOS_FMP
struct exynos_fmp_variant_ops *exynos_fmp_get_variant_ops(struct device_node *node);
struct platform_device *exynos_fmp_get_pdevice(struct device_node *node);
#else
struct exynos_fmp_variant_ops *exynos_fmp_get_variant_ops(struct device_node *node)
{
	return NULL;
}

struct platform_device *exynos_fmp_get_pdevice(struct device_node *node)
{
	return NULL;
}
#endif
#endif
