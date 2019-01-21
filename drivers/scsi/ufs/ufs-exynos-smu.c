/*
 * Copyright (C) 2016 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/of.h>
#include <crypto/smu.h>

#include "ufshcd.h"
#include "ufs-exynos.h"
#include "ufs-exynos-smu.h"

#define EXYNOS_UFS_SMU_LABEL	"ufs-exynos-smu"

static struct exynos_smu_variant_ops *exynos_ufs_smu_get_vops(struct device *dev)
{
	struct exynos_smu_variant_ops *smu_vops = NULL;
	struct device_node *node;

	node = of_parse_phandle(dev->of_node, EXYNOS_UFS_SMU_LABEL, 0);
	if (!node) {
		dev_err(dev, "%s: exynos-ufs-smu property not specified\n",
			__func__);
		goto err;
	}

	smu_vops = exynos_smu_get_variant_ops(node);
	if (!smu_vops)
		dev_err(dev, "%s: Fail to get smu_vops\n", __func__);

	of_node_put(node);
err:
	return smu_vops;
}

static struct platform_device *exynos_ufs_smu_get_pdevice(struct device *dev)
{
	struct device_node *node;
	struct platform_device *smu_pdev = NULL;

	node = of_parse_phandle(dev->of_node, EXYNOS_UFS_SMU_LABEL, 0);
	if (!node) {
		dev_err(dev, "%s: exynos-ufs-smu property not specified\n",
			__func__);
		goto err;
	}

	smu_pdev = exynos_smu_get_pdevice(node);
err:
	return smu_pdev;
}

int exynos_ufs_smu_get_dev(struct exynos_ufs *ufs)
{
	int ret;
	struct device *dev;

	if (!ufs || !ufs->dev) {
		pr_err("%s: invalid ufs host, host->dev\n", __func__);
		ret = -EINVAL;
		goto err_host;
	}

	dev = ufs->dev;
	ufs->smu.vops = exynos_ufs_smu_get_vops(dev);
	ufs->smu.pdev = exynos_ufs_smu_get_pdevice(dev);

	if (ufs->smu.pdev == ERR_PTR(-EPROBE_DEFER)) {
		dev_err(dev, "%s: SMU device not probed yet\n", __func__);
		ret = -EPROBE_DEFER;
		goto err;
	}

	if (!ufs->smu.pdev || !ufs->smu.vops) {
		dev_err(dev, "%s: Host device doesn't have SMU or fail to get SMU",
				__func__);
		ret = -ENODEV;
		goto err;
	}

	return 0;
err:
	ufs->smu.pdev = NULL;
	ufs->smu.vops = NULL;

err_host:
	return ret;
}

int exynos_ufs_smu_init(struct exynos_ufs *ufs)
{
	struct smu_data_setting smu_set;

	if (!ufs->smu.vops || !ufs->smu.pdev)
		return 0;

	smu_set.id = SMU_EMBEDDED;
	smu_set.command = SMU_INIT;
	return ufs->smu.vops->init(ufs->smu.pdev, &smu_set);
}

int exynos_ufs_smu_sec_cfg(struct exynos_ufs *ufs)
{
	struct smu_data_setting smu_set;

	if (!ufs->smu.vops || !ufs->smu.pdev)
		return 0;

	smu_set.id = SMU_EMBEDDED;
	smu_set.desc_type = CFG_DESCTYPE;

	return ufs->smu.vops->sec_config(ufs->smu.pdev, &smu_set);
}

int exynos_ufs_smu_resume(struct exynos_ufs *ufs)
{
	struct smu_data_setting smu_set;

	if (!ufs->smu.vops || !ufs->smu.pdev)
		return 0;

	smu_set.id = SMU_EMBEDDED;
	return ufs->smu.vops->resume(ufs->smu.pdev, &smu_set);
}

int exynos_ufs_smu_abort(struct exynos_ufs *ufs)
{
	struct smu_data_setting smu_set;

	if (!ufs->smu.vops || !ufs->smu.pdev)
		return 0;

	smu_set.id = SMU_EMBEDDED;
	smu_set.command = SMU_ABORT;
	return ufs->smu.vops->abort(ufs->smu.pdev, &smu_set);
}
