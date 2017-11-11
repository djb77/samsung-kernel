/*
 * Copyright (c) 2014 Samsung Electronics Co., Ltd.
 *	      http://www.samsung.com/
 *
 * EXYNOS - CHIP ID support
 * Author: Pankaj Dubey <pankaj.dubey@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/sys_soc.h>
#include <linux/soc/samsung/exynos-soc.h>

#define EXYNOS_SUBREV_MASK	(0xF << 4)
#define EXYNOS_MAINREV_MASK	(0xF << 0)
#define EXYNOS_REV_MASK		(EXYNOS_SUBREV_MASK | EXYNOS_MAINREV_MASK)

static void __iomem *exynos_chipid_base;

struct exynos_chipid_info exynos_soc_info;
EXPORT_SYMBOL(exynos_soc_info);

static const char * __init product_id_to_name(unsigned int product_id)
{
	const char *soc_name;
	unsigned int soc_id = product_id & EXYNOS_SOC_MASK;

	switch (soc_id) {
	case EXYNOS3250_SOC_ID:
		soc_name = "EXYNOS3250";
		break;
	case EXYNOS4210_SOC_ID:
		soc_name = "EXYNOS4210";
		break;
	case EXYNOS4212_SOC_ID:
		soc_name = "EXYNOS4212";
		break;
	case EXYNOS4412_SOC_ID:
		soc_name = "EXYNOS4412";
		break;
	case EXYNOS4415_SOC_ID:
		soc_name = "EXYNOS4415";
		break;
	case EXYNOS5250_SOC_ID:
		soc_name = "EXYNOS5250";
		break;
	case EXYNOS5260_SOC_ID:
		soc_name = "EXYNOS5260";
		break;
	case EXYNOS5420_SOC_ID:
		soc_name = "EXYNOS5420";
		break;
	case EXYNOS5440_SOC_ID:
		soc_name = "EXYNOS5440";
		break;
	case EXYNOS5800_SOC_ID:
		soc_name = "EXYNOS5800";
		break;
	case EXYNOS8890_SOC_ID:
		soc_name = "EXYNOS8890";
		break;
	default:
		soc_name = "UNKNOWN";
	}
	return soc_name;
}

static const struct of_device_id of_exynos_chipid_ids[] __initconst = {
	{
		.compatible	= "samsung,exynos4210-chipid",
	},
	{},
};

/**
 *  exynos_chipid_early_init: Early chipid initialization
 *  @dev: pointer to chipid device
 */
void __init exynos_chipid_early_init(struct device *dev)
{
	struct device_node *np;
	const struct of_device_id *match;

	if (exynos_chipid_base)
		return;

	if (!dev)
		np = of_find_matching_node_and_match(NULL,
			of_exynos_chipid_ids, &match);
	else
		np = dev->of_node;

	if (!np)
		panic("%s, failed to find chipid node\n", __func__);

	exynos_chipid_base = of_iomap(np, 0);

	if (!exynos_chipid_base)
		panic("%s: failed to map registers\n", __func__);

	exynos_soc_info.product_id  = __raw_readl(exynos_chipid_base);
	exynos_soc_info.unique_id  = __raw_readl(exynos_chipid_base + UNIQUE_ID1);
	exynos_soc_info.unique_id  |= (u64)__raw_readl(exynos_chipid_base + UNIQUE_ID2) << 32;
	exynos_soc_info.revision = exynos_soc_info.product_id & EXYNOS_REV_MASK;
}

static int __init exynos_chipid_probe(struct platform_device *pdev)
{
	struct soc_device_attribute *soc_dev_attr;
	struct soc_device *soc_dev;
	struct device_node *root;
	int ret;

	exynos_chipid_early_init(&pdev->dev);

	soc_dev_attr = kzalloc(sizeof(*soc_dev_attr), GFP_KERNEL);
	if (!soc_dev_attr)
		return -ENODEV;

	soc_dev_attr->family = "Samsung Exynos";

	root = of_find_node_by_path("/");
	ret = of_property_read_string(root, "model", &soc_dev_attr->machine);
	of_node_put(root);
	if (ret)
		goto free_soc;

	soc_dev_attr->revision = kasprintf(GFP_KERNEL, "%d",
					exynos_soc_info.revision);
	if (!soc_dev_attr->revision)
		goto free_soc;

	soc_dev_attr->soc_id = product_id_to_name(exynos_soc_info.product_id);

	soc_dev = soc_device_register(soc_dev_attr);
	if (IS_ERR(soc_dev))
		goto free_rev;

	soc_device_to_device(soc_dev);

	dev_info(&pdev->dev, "Exynos: CPU[%s] CPU_REV[0x%x] Detected\n",
			product_id_to_name(exynos_soc_info.product_id),
			exynos_soc_info.revision);
	return 0;
free_rev:
	kfree(soc_dev_attr->revision);
free_soc:
	kfree(soc_dev_attr);
	return -EINVAL;
}

static struct platform_driver exynos_chipid_driver __refdata = {
	.driver = {
		.name = "exynos-chipid",
		.of_match_table = of_exynos_chipid_ids,
	},
	.probe = exynos_chipid_probe,
};

static int __init exynos_chipid_init(void)
{
	return platform_driver_register(&exynos_chipid_driver);
}
core_initcall(exynos_chipid_init);

