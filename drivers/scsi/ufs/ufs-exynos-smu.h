/*
 * Copyright (C) 2016 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _UFS_EXYNOS_SMU_H_
#define _UFS_EXYNOS_SMU_H_

#ifdef CONFIG_SCSI_UFS_EXYNOS_SMU
int exynos_ufs_smu_get_dev(struct exynos_ufs *ufs);
int exynos_ufs_smu_init(struct exynos_ufs *ufs);
int exynos_ufs_smu_sec_cfg(struct exynos_ufs *ufs);
int exynos_ufs_smu_resume(struct exynos_ufs *ufs);
int exynos_ufs_smu_abort(struct exynos_ufs *ufs);
#else
inline int exynos_ufs_smu_get_dev(struct exynos_ufs *ufs)
{
	if (ufs) {
		ufs->smu.pdev = NULL;
		ufs->smu.vops = NULL;
	}
	return 0;
}

inline int exynos_ufs_smu_init(struct exynos_ufs *ufs)
{
	writel(0x0, ufs->reg_ufsp + UFSPSBEGIN0);
	writel(0xffffffff, ufs->reg_ufsp + UFSPSEND0);
	writel(0xff, ufs->reg_ufsp + UFSPSLUN0);
	writel(0xf1, ufs->reg_ufsp + UFSPSCTRL0);

	return 0;
}

inline int exynos_ufs_smu_sec_cfg(struct exynos_ufs *ufs)
{
	return 0;
}

inline int exynos_ufs_smu_resume(struct exynos_ufs *ufs)
{
	writel(0x0, ufs->reg_ufsp + UFSPSBEGIN0);
	writel(0xffffffff, ufs->reg_ufsp + UFSPSEND0);
	writel(0xff, ufs->reg_ufsp + UFSPSLUN0);
	writel(0xf1, ufs->reg_ufsp + UFSPSCTRL0);

	return 0;
}

inline int exynos_ufs_smu_abort(struct exynos_ufs *ufs)
{
	return 0;
}

#endif /* CONFIG_SCSI_UFS_EXYNOS_SMU */
#endif /* _UFS_EXYNOS_SMU_H_ */
