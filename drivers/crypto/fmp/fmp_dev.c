/*
 * Exynos FMP driver
 *
 * Copyright (C) 2015 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/smc.h>
#include <linux/crypto.h>

#include <crypto/fmp.h>

#include "fmp_lib.h"
#include "fmp_fips_main.h"
#include "fmp_dev.h"
#include "fmp_derive_iv.h"
#include "fmp_version.h"
#include "fmp_fips_func_test.h"

static LIST_HEAD(fmp_devices);

#define EXYNOS_HOST_LABEL	"exynos-host"
#define EXYNOS_HOST_TYPE_LABEL	"exynos,host-type"

static inline int is_set_fmp_disk_key(struct exynos_fmp *fmp)
{
	return (fmp->status_disk_key == KEY_SET) ? TRUE : FALSE;
}

static inline int is_stored_fmp_disk_key(struct exynos_fmp *fmp)
{
	return (fmp->status_disk_key == KEY_STORED) ? TRUE : FALSE;
}

static int fmpdev_set_disk_cfg(struct exynos_fmp *fmp,
				struct fmp_data_setting *data,
				enum fmp_crypto_enc_mode enc_mode)
{
	int ret = 0;
	struct fmp_table_setting *table = data->table;
	struct fmp_crypto_setting *crypto = &data->disk;

	ret = fmplib_set_algo_mode(fmp, table, crypto, enc_mode,
			data->cmdq_enabled);
	if (ret) {
		dev_err(fmp->dev, "%s: Fail to set FMP encryption mode\n",
				__func__);
		goto err;
	}

	if (crypto->algo_mode == EXYNOS_FMP_BYPASS_MODE)
		return 0;

	if (is_stored_fmp_disk_key(fmp)) {
		exynos_smc(SMC_CMD_FMP_DISK_KEY_SET, 0, 0, 0);
		fmp->status_disk_key = KEY_SET;
	} else if (!is_set_fmp_disk_key(fmp)) {
		dev_err(fmp->dev, "%s: Cannot configure FMP because disk key is clear\n",
				__func__);
		ret = -EINVAL;
		goto err;
	}

	ret = fmplib_set_key_size(fmp, table, crypto, enc_mode,
			data->cmdq_enabled);
	if (ret) {
		dev_err(fmp->dev, "%s: Fail to set FMP key size\n", __func__);
		goto err;
	}

	ret = fmplib_derive_iv(fmp, crypto, enc_mode);
	if (ret) {
		dev_err(fmp->dev, "%s: Fail to derive FMP IV\n", __func__);
		goto err;
	}

	ret = fmplib_set_iv(fmp, table, crypto, enc_mode);
	if (ret) {
		dev_err(fmp->dev, "%s: Fail to set FMP IV\n", __func__);
		goto err;
	}

err:
	return ret;
}

static int fmpdev_set_file_cfg(struct exynos_fmp *fmp,
					struct fmp_data_setting *data,
					enum fmp_crypto_enc_mode enc_mode)
{
	int ret = 0;
	struct fmp_table_setting *table = data->table;
	struct fmp_crypto_setting *crypto = &data->file;

	ret = fmplib_set_algo_mode(fmp, table, crypto, enc_mode,
			data->cmdq_enabled);
	if (ret) {
		dev_err(fmp->dev, "%s: Fail to set FMP encryption mode\n",
				__func__);
		goto err;
	}

	if (crypto->algo_mode == EXYNOS_FMP_BYPASS_MODE)
		return 0;

	ret = fmplib_set_key(fmp, table, crypto, enc_mode);
	if (ret) {
		dev_err(fmp->dev, "%s: Fail to set FMP key\n", __func__);
		goto err;
	}

	ret = fmplib_set_key_size(fmp, table, crypto, enc_mode,
			data->cmdq_enabled);
	if (ret) {
		dev_err(fmp->dev, "%s: Fail to set FMP key size\n", __func__);
		goto err;
	}

	ret = fmplib_derive_iv(fmp, crypto, enc_mode);
	if (ret) {
		dev_err(fmp->dev, "%s: Fail to derive FMP IV\n", __func__);
		goto err;
	}

	ret = fmplib_set_iv(fmp, table, crypto, enc_mode);
	if (ret) {
		dev_err(fmp->dev, "%s: Fail to set FMP IV\n", __func__);
		goto err;
	}

err:
	return ret;
}

static int fmpdev_set_test_cfg(struct exynos_fmp *fmp,
					struct fmp_data_setting *data)
{
	int ret = 0;
	struct fmp_crypto_setting *crypto;

	if (!fmp->fips_data) {
		dev_err(fmp->dev, "%s: Invalid test data\n", __func__);
		goto err;
	}
	crypto = &fmp->fips_data->crypto;

	if (!data || !data->table) {
		dev_err(fmp->dev, "%s: Invalid data or table data\n", __func__);
		goto err;
	}

	ret = fmplib_set_algo_mode(fmp, data->table, crypto, EXYNOS_FMP_FILE_ENC,
			data->cmdq_enabled);
	if (ret) {
		dev_err(fmp->dev, "%s: Fail to set FMP encryption mode\n",
				__func__);
		goto err;
	}

	if (crypto->algo_mode == EXYNOS_FMP_BYPASS_MODE)
		return 0;

	ret = fmplib_clear(fmp, data->table);
	if (ret) {
		pr_err("%s: Fail to clear FMP table\n", __func__);
		goto err;
	}

	ret = fmplib_set_key(fmp, data->table, crypto, EXYNOS_FMP_FILE_ENC);
	if (ret) {
		dev_err(fmp->dev, "%s: Fail to set FMP key\n", __func__);
		goto err;
	}

	ret = fmplib_set_key_size(fmp, data->table, crypto, EXYNOS_FMP_FILE_ENC,
			data->cmdq_enabled);
	if (ret) {
		dev_err(fmp->dev, "%s: Fail to set FMP key size\n", __func__);
		goto err;
	}

	ret = fmplib_set_iv(fmp, data->table, crypto, EXYNOS_FMP_FILE_ENC);
	if (ret) {
		dev_err(fmp->dev, "%s: Fail to set FMP IV\n", __func__);
		goto err;
	}
err:
	return ret;
}

static int exynos_fmp_config(struct platform_device *pdev,
					struct fmp_data_setting *data)
{
	int ret = 0;
	struct exynos_fmp *fmp;

	if (!pdev || !data) {
		pr_err("%s: Invalid fmp pdev or data\n", __func__);
		ret = -EINVAL;
		goto err;
	}

	fmp = dev_get_drvdata(&pdev->dev);
	if (!fmp) {
		pr_err("%s: Invalid fmp driver platform data\n", __func__);
		ret = -ENODEV;
		goto err;
	}

	if (unlikely(in_fmp_fips_err())) {
		dev_err(fmp->dev, "%s: Fail to work fmp config due to fips in error.\n",
				__func__);
		return -EPERM;
	}

	if (fmp->test_mode) {
		ret = fmpdev_set_test_cfg(fmp, data);
		if (ret) {
			dev_err(fmp->dev, "%s: Fail to set FMP test configuration\n",
					__func__);
			ret = -EPERM;
			goto err;
		} else
			goto err;
	}

	ret = fmpdev_set_file_cfg(fmp, data, EXYNOS_FMP_FILE_ENC);
	if (ret) {
		dev_err(fmp->dev, "%s: Fail to set FMP file configuration\n",
				__func__);
		ret = -EPERM;
		goto err;
	}

	ret = fmpdev_set_disk_cfg(fmp, data, EXYNOS_FMP_DISK_ENC);
	if (ret) {
		dev_err(fmp->dev, "%s: Fail to set FMP disk configuration\n",
				__func__);
		ret = -EPERM;
		goto err;
	}
err:
	return ret;
}

static int exynos_fmp_set_disk_key(struct platform_device *pdev,
					struct fmp_data_setting *data)
{
	int ret = 0;
	struct device *dev = &pdev->dev;
	struct exynos_fmp *fmp;
	struct fmp_crypto_setting *crypto;
	struct fmp_table_setting *table;

	if (!data) {
		pr_err("%s: Invalid fmp data\n", __func__);
		ret = -EINVAL;
		goto err;
	}

	table = data->table;
	crypto = &data->disk;

	fmp = dev_get_drvdata(dev);
	if (!fmp) {
		pr_err("%s: Invalid fmp driver platform data\n", __func__);
		ret = -ENODEV;
		goto err;
	}

	ret = fmplib_set_key(fmp, table, crypto, EXYNOS_FMP_DISK_ENC);
	if (ret) {
		dev_err(fmp->dev, "%s: Fail to set FMP disk key\n", __func__);
		goto err;
	}

err:
	return ret;
}

static int exynos_fmp_clear_disk_key(struct platform_device *pdev)
{
	int ret = 0;
	struct device *dev = &pdev->dev;
	struct exynos_fmp *fmp;

	fmp = dev_get_drvdata(dev);
	if (!fmp) {
		pr_err("%s: Invalid fmp driver platform data\n", __func__);
		ret = -ENODEV;
		goto err;
	}

	ret = fmplib_clear_disk_key(fmp);
	if (ret) {
		dev_err(fmp->dev, "%s: Fail to clear FMP disk key\n", __func__);
		goto err;
	}

err:
	return ret;
}

static int exynos_fmp_clear(struct platform_device *pdev,
					struct fmp_data_setting *data)
{
	int ret = 0;
	struct device *dev = &pdev->dev;
	struct exynos_fmp *fmp;
	struct fmp_table_setting *table;

	if (!data) {
		pr_err("%s: Invalid fmp data\n", __func__);
		ret = -EINVAL;
		goto err;
	}

	table = data->table;
	fmp = dev_get_drvdata(dev);
	if (!fmp) {
		pr_err("%s: Invalid fmp driver platform data\n", __func__);
		ret = -ENODEV;
		goto err;
	}

	ret = fmplib_clear(fmp, table);
	if (ret) {
		dev_err(fmp->dev, "%s: Fail to clear FMP disk key\n", __func__);
		goto err;
	}

err:
	return ret;
}

const struct exynos_fmp_variant_ops exynos_fmp_ops = {
	.name		= "exynos",
	.config		= exynos_fmp_config,
	.set_disk_key	= exynos_fmp_set_disk_key,
	.clear_disk_key = exynos_fmp_clear_disk_key,
	.clear		= exynos_fmp_clear,
};

static struct of_device_id exynos_fmp_match[] = {
	{
		.compatible = "samsung,exynos-fmp",
		.data = (void *)&exynos_fmp_ops,
	},
	{},
};
MODULE_DEVICE_TABLE(of, exynos_fmp_match);

struct platform_device *exynos_fmp_get_pdevice(struct device_node *node)
{
	struct platform_device *fmp_pdev = NULL;
	struct exynos_fmp *fmp = NULL;

	if (!node) {
		pr_err("%s: Invalid node\n", __func__);
		goto out;
	}

	if (!of_device_is_available(node)) {
		pr_err("%s: Unavailable device\n", __func__);
		goto out;
	}

	if (list_empty(&fmp_devices)) {
		pr_err("%s: Invalie device list\n", __func__);
		fmp_pdev = ERR_PTR(-EPROBE_DEFER);
		goto out;
	}

	list_for_each_entry(fmp, &fmp_devices, list) {
		if (fmp->dev->of_node == node) {
			pr_info("%s: Found FMP device\n", __func__);
			break;
		}
	}

	fmp_pdev = to_platform_device(fmp->dev);
	pr_info("%s: Matching platform device\n", __func__);
out:
	return fmp_pdev;
}
EXPORT_SYMBOL(exynos_fmp_get_pdevice);

struct exynos_fmp_variant_ops *exynos_fmp_get_variant_ops(struct device_node *node)
{
	if (node) {
		const struct of_device_id *match;

		match = of_match_node(exynos_fmp_match, node);
		if (match)
			return (struct exynos_fmp_variant_ops *)(match->data);
		pr_err("%s, Error matching\n", __func__);
	} else {
		pr_err("%s, Invalid node\n", __func__);
	}
	return NULL;
}
EXPORT_SYMBOL(exynos_fmp_get_variant_ops);

static struct platform_device *exynos_fmp_host_get_pdevice(struct device *dev)
{
	struct device_node *node;
	struct platform_device *host_pdev = NULL;

	node = of_parse_phandle(dev->of_node, EXYNOS_HOST_LABEL, 0);
	if (!node) {
		dev_err(dev, "%s: exynos-host property not specified\n",
			__func__);
		goto out;
	}

	host_pdev = of_find_device_by_node(node);
out:
	return host_pdev;
}

static int exynos_fmp_get_host_type(struct platform_device *pdev,
					struct exynos_fmp *fmp)
{
	int ret = 0;
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;
	const char *type;

	ret = of_property_read_string_index(node, EXYNOS_HOST_TYPE_LABEL, 0, &type);
	if (ret) {
		dev_err(fmp->dev, "%s: Could not get FMP host type\n", __func__);
		ret = -EINVAL;
		goto err;
	}
	strlcpy(fmp->host_type, type, FMP_HOST_TYPE_NAME_LEN);
err:
	return ret;
}

static int exynos_fmp_host_get_dev(struct platform_device *pdev)
{
	int ret = 0;
	struct exynos_fmp *fmp;
	struct platform_device *host_pdev;
	struct exynos_fmp_variant_ops *fmp_vops;

	fmp = dev_get_drvdata(&pdev->dev);
	if (!fmp || !fmp->dev) {
		pr_err("%s: invalid fmp context or device\n", __func__);
		ret = -EINVAL;
		goto err;
	}

	host_pdev = exynos_fmp_host_get_pdevice(fmp->dev);
	fmp_vops = exynos_fmp_get_variant_ops(fmp->dev->of_node);

	if (host_pdev == ERR_PTR(-EPROBE_DEFER)) {
		dev_err(fmp->dev, "%s: Host device not probed yet\n", __func__);
		ret = -EPROBE_DEFER;
		goto err;
	}

	if (!host_pdev || !fmp_vops) {
		dev_err(fmp->dev, "%s: invalid platform device %p or vops %p\n",
			__func__, host_pdev, fmp_vops);
		ret = -ENODEV;
		goto err;
	}

	ret = exynos_fmp_get_host_type(pdev, fmp);
	if (ret) {
		dev_err(fmp->dev, "%s: invalid Host type. ret(%d)\n", __func__, ret);
		goto err;
	}

	fmp->host_pdev = host_pdev;

	if (!strncmp(fmp->host_type, "ufs", sizeof("ufs")))
		return exynos_ufs_fmp_host_set_device(host_pdev, pdev, fmp_vops);
	else if (!strncmp(fmp->host_type, "mmc", sizeof("mmc")))
		return exynos_mmc_fmp_host_set_device(host_pdev, pdev, fmp_vops);

	dev_err(fmp->dev, "%s: Not matched Host type(%s)\n", __func__,
			fmp->host_type);
	ret = -EINVAL;
err:
	return ret;
}

static int exynos_fmp_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct exynos_fmp *fmp;
	struct device *dev = &pdev->dev;

	if (!pdev) {
		pr_err("%s: Invalid platform_device.\n", __func__);
		ret = -EINVAL;
		goto err_pdev;
	}

	fmp = kzalloc(sizeof(struct exynos_fmp), GFP_KERNEL);
	if (!fmp) {
		ret = -ENOMEM;
		goto err_mem;
	}

	fmp->dev = &pdev->dev;
	if (!fmp->dev) {
		pr_err("%s: Invalid device.\n", __func__);
		ret = -EINVAL;
		goto err_dev;
	}

	fmp->status_disk_key = KEY_CLEAR;

	dev_set_drvdata(dev, fmp);
	list_add_tail(&fmp->list, &fmp_devices);

	ret = exynos_fmp_host_get_dev(pdev);
	if (ret == -EPROBE_DEFER) {
		dev_err(fmp->dev, "%s: Host device not proved yet. ret = %d\n",
				__func__, ret);
		goto err_dev;
	} else if (ret) {
		dev_err(fmp->dev, "%s: Fail to get Host device. ret = %d\n",
				__func__, ret);
		goto err_dev;
	}

	dev_info(fmp->dev, "Exynos FMP Version: %s\n", FMP_DRV_VERSION);

	ret = exynos_fmp_func_test_KAT_case(pdev, fmp,
			exynos_fmp_get_variant_ops(fmp->dev->of_node));
	if (ret) {
		dev_err(fmp->dev, "%s: Fail to test KAT case. ret(%d)",
				__func__, ret);
		goto err_dev;
	}

	ret = exynos_fmp_fips_init(fmp);
	if (ret) {
		dev_err(fmp->dev, "%s: Fail to initialize fmp fips. ret(%d)",
				__func__, ret);
		exynos_fmp_fips_exit(fmp);
		goto err_dev;
	}

	dev_info(fmp->dev, "%s: Exynos FMP driver is proved\n", __func__);
	return ret;

err_dev:
	kzfree(fmp);
err_mem:
err_pdev:
	return ret;
}

static int exynos_fmp_remove(struct platform_device *pdev)
{
	struct exynos_fmp *fmp;
	struct device *dev = &pdev->dev;

	fmp = dev_get_drvdata(dev);
	if (!fmp)
		return 0;

	exynos_fmp_fips_exit(fmp);

	kzfree(fmp);
	return 0;
}

static struct platform_driver exynos_fmp_driver = {
	.driver = {
		.name = "exynos-fmp",
		.owner = THIS_MODULE,
		.pm = NULL,
		.of_match_table = exynos_fmp_match,
	},
	.probe = exynos_fmp_probe,
	.remove = exynos_fmp_remove,
};

static int __init fmp_init(void)
{
	return platform_driver_register(&exynos_fmp_driver);
}
late_initcall(fmp_init);

static void __exit fmp_exit(void)
{
	platform_driver_unregister(&exynos_fmp_driver);
}
module_exit(fmp_exit);

MODULE_DESCRIPTION("Exynos Spedific FMP(Flash Memory Protector) driver");
