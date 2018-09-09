/*
 * UFS SMU (Secure Management Unit) driver for Exynos
 *
 * Copyright (C) 2016 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/smc.h>

#include <crypto/smu.h>

static LIST_HEAD(smu_devices);

struct exynos_smu {
	struct list_head list;
	int id;
	int command;
	int desc_type;
	struct device *dev;
};

static int exynos_smu_init(struct platform_device *pdev,
					struct smu_data_setting *smu_set)
{
	int ret = 0;
	struct exynos_smu *smu = NULL;

	if (!pdev || !smu_set) {
		pr_err("%s: Invalid input parameter.\n", __func__);
		ret = -EINVAL;
		goto err;
	}

	smu = platform_get_drvdata(pdev);
	ret = exynos_smc(SMC_CMD_SMU, smu_set->command, smu_set->id, 0);
	if (ret)
		dev_err(smu->dev, "%s: Fail smc call for SMU init. ret(%d)\n",
				__func__, ret);
err:
	return ret;
}

static int exynos_smu_sec_config(struct platform_device *pdev,
					struct smu_data_setting *smu_set)
{
	int ret = 0;
	struct exynos_smu *smu = NULL;

	if (!pdev || !smu_set) {
		pr_err("%s: Invalid input parameter.\n", __func__);
		ret = -EINVAL;
		goto err;
	}

	smu = platform_get_drvdata(pdev);
	ret = exynos_smc(SMC_CMD_FMP_SECURITY, 0, smu_set->id, smu_set->desc_type);
	if (ret)
		dev_err(smu->dev, "%s: Fail smc for FMPSECURITY config. ret(%d)\n",
				__func__, ret);
err:
	return ret;
}

static int exynos_smu_resume(struct platform_device *pdev,
					struct smu_data_setting *smu_set)
{
	int ret = 0;
	struct exynos_smu *smu = NULL;

	if (!pdev || !smu_set) {
		pr_err("%s: Invalid input parameter.\n", __func__);
		ret = -EINVAL;
		goto err;
	}

	smu = platform_get_drvdata(pdev);
	ret = exynos_smc(SMC_CMD_FMP_SMU_RESUME, 0, smu_set->id, 0);
	if (ret)
		dev_err(smu->dev, "%s: Fail smc call for SMU resume. ret(%d)\n",
			__func__, ret);
err:
	return ret;
}

static int exynos_smu_abort(struct platform_device *pdev,
					struct smu_data_setting *smu_set)
{
	int ret = 0;

	if (!pdev || !smu_set) {
		pr_err("%s: Invalid input parameter.\n", __func__);
		ret = -EINVAL;
		goto err;
	}

	ret = exynos_smc(SMC_CMD_SMU, smu_set->command, smu_set->id, 0);
err:
	return ret;
}

const struct exynos_smu_variant_ops exynos_smu_ops = {
	.name		= "exynos-smu",
	.init		= exynos_smu_init,
	.sec_config	= exynos_smu_sec_config,
	.resume		= exynos_smu_resume,
	.abort		= exynos_smu_abort,
};

static struct of_device_id exynos_smu_match[] = {
	{
		.compatible = "samsung,exynos-smu",
		.data = (void *)&exynos_smu_ops,
	},
	{},
};
MODULE_DEVICE_TABLE(of, exynos_smu_match);

struct platform_device *exynos_smu_get_pdevice(struct device_node *node)
{
	struct platform_device *smu_pdev = NULL;
	struct exynos_smu *smu = NULL;

	if (!node) {
		pr_err("%s: Invalid node\n", __func__);
		goto err;
	}

	if (!of_device_is_available(node)) {
		pr_err("%s: Unavailable device\n", __func__);
		goto err;
	}

	if (list_empty(&smu_devices)) {
		pr_err("%s: Invalie device list\n", __func__);
		smu_pdev = ERR_PTR(-EPROBE_DEFER);
		goto err;
	}

	list_for_each_entry(smu, &smu_devices, list) {
		if (smu->dev->of_node == node) {
			pr_info("%s: Found smu device %p\n", __func__, smu);
			break;
		}
	}

	smu_pdev = to_platform_device(smu->dev);
	pr_info("%s: Matching platform device %p\n", __func__, smu_pdev);
err:
	return smu_pdev;
}
EXPORT_SYMBOL(exynos_smu_get_pdevice);

struct exynos_smu_variant_ops *exynos_smu_get_variant_ops(struct device_node *node)
{
	if (node) {
		const struct of_device_id *match;

		match = of_match_node(exynos_smu_match, node);
		if (match)
			return (struct exynos_smu_variant_ops *)(match->data);
		pr_err("%s: Error matching\n", __func__);
	} else {
		pr_err("%s: Invalid node\n", __func__);
	}
	return NULL;
}
EXPORT_SYMBOL(exynos_smu_get_variant_ops);

static int exynos_smu_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct exynos_smu *smu;

	if (!pdev) {
		pr_err("%s: Invalid platform_device.\n", __func__);
		ret = -EINVAL;
		goto err_pdev;
	}

	smu = kzalloc(sizeof(struct exynos_smu), GFP_KERNEL);
	if (!smu) {
		ret = -ENOMEM;
		goto err_mem;
	}

	smu->dev = &pdev->dev;
	if (!smu->dev) {
		pr_err("%s: Invalid device.\n", __func__);
		ret = -EINVAL;
		goto err_dev;
	}

	platform_set_drvdata(pdev, smu);
	list_add_tail(&smu->list, &smu_devices);

	pr_info("%s: Exynos SMU driver is proved\n", __func__);
	return ret;

err_dev:
	kfree(smu);
err_mem:
err_pdev:
	return ret;
}

static int exynos_smu_remove(struct platform_device *pdev)
{
	struct exynos_smu *smu;

	smu = (struct exynos_smu *)platform_get_drvdata(pdev);
	if (!smu)
		return 0;

	kfree(smu);
	return 0;
}

static struct platform_driver exynos_smu_driver = {
	.driver = {
		.name = "exynos-smu",
		.owner = THIS_MODULE,
		.pm = NULL,
		.of_match_table = exynos_smu_match,
	},
	.probe = exynos_smu_probe,
	.remove = exynos_smu_remove,
};

static int __init smu_init(void)
{
	return platform_driver_register(&exynos_smu_driver);
}
subsys_initcall(smu_init);

static void __exit smu_exit(void)
{
	platform_driver_unregister(&exynos_smu_driver);
}
module_exit(smu_exit);

MODULE_DESCRIPTION("Exynos Specific SMU(Secure Management Unit) driver");
