/*
 * Copyright (C) 2016 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _FMP_DEV_H_
#define _FMP_DEV_H_

#ifdef CONFIG_MMC_DW_EXYNOS_FMP
int exynos_mmc_fmp_host_set_device(struct platform_device *host_pdev,
				struct platform_device *pdev,
				struct exynos_fmp_variant_ops *fmp_vops);
#else
int exynos_mmc_fmp_host_set_device(struct platform_device *host_pdev,
				struct platform_device *pdev,
				struct exynos_fmp_variant_ops *fmp_vops)
{
	return 0;
}
#endif

#ifdef CONFIG_SCSI_UFS_EXYNOS_FMP
int exynos_ufs_fmp_host_set_device(struct platform_device *host_pdev,
				struct platform_device *pdev,
				struct exynos_fmp_variant_ops *fmp_vops);
#else
int exynos_ufs_fmp_host_set_device(struct platform_device *host_pdev,
				struct platform_device *pdev,
				struct exynos_fmp_variant_ops *fmp_vops)
{
	return 0;
}
#endif
#endif
