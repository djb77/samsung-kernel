/*
 * Exynos FMP test driver
 *
 * Copyright (C) 2015 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/miscdevice.h>
#include <linux/crypto.h>
#include <linux/buffer_head.h>
#include <linux/genhd.h>
#include <linux/delay.h>

#include <crypto/authenc.h>
#include <crypto/fmp.h>

#include "fmp_fips_main.h"
#include "fmp_fips_fops.h"
#include "fmp_fips_selftest.h"
#include "fmp_fips_integrity.h"

#define MAX_SCAN_PART	(50)
#define MAX_RETRY_COUNT (0x100000)

static const char pass[] = "passed";
static const char fail[] = "failed";

#define FMP_FIPS_INIT_STATE	0
#define FMP_FIPS_PROGRESS_STATE	1
#define FMP_FIPS_ERR_STATE	2
#define FMP_FIPS_SUCCESS_STATE	3

static int fmp_fips_state = FMP_FIPS_INIT_STATE;

bool in_fmp_fips_err(void)
{
	if (fmp_fips_state == FMP_FIPS_INIT_STATE ||
			fmp_fips_state == FMP_FIPS_ERR_STATE)
		return true;
	return false;
}

static void set_fmp_fips_state(uint32_t val)
{
	fmp_fips_state = val;
}

static dev_t find_devt_for_selftest(struct exynos_fmp *fmp,
					struct fmp_fips_data *data)
{
	int i, idx = 0;
	uint32_t count = 0;
	uint64_t size;
	uint64_t size_list[MAX_SCAN_PART];
	dev_t devt_list[MAX_SCAN_PART];
	dev_t devt_scan, devt;
	struct block_device *bdev;
	struct device *dev = fmp->dev;
	fmode_t fmode = FMODE_WRITE | FMODE_READ;

	memset(size_list, 0, sizeof(size_list));
	memset(devt_list, 0, sizeof(devt_list));

	if (!data) {
		dev_err(dev, "Invalid fmp test data\n");
		return (dev_t)0;
	}

	do {
		for (i = 1; i < MAX_SCAN_PART; i++) {
			devt_scan = blk_lookup_devt(data->block_type, i);
			bdev = blkdev_get_by_dev(devt_scan, fmode, NULL);
			if (IS_ERR(bdev))
				continue;
			else {
				size_list[idx] = (uint64_t)i_size_read(bdev->bd_inode);
				devt_list[idx++] = devt_scan;
				blkdev_put(bdev, fmode);
			}
		}

		if (!idx) {
			mdelay(100);
			count++;
			continue;
		}

		for (i = 0; i < idx; i++) {
			if (i == 0) {
				size = size_list[i];
				devt = devt_list[i];
			} else {
				if (size < size_list[i])
					devt = devt_list[i];
			}
		}

		bdev = blkdev_get_by_dev(devt, fmode, NULL);
		dev_info(dev, "Found partno %d for FMP test\n",
							bdev->bd_part->partno);
		blkdev_put(bdev, fmode);
		return devt;
	} while (count < MAX_RETRY_COUNT);

	dev_err(dev, "Block device isn't initialized yet for FMP test\n");
	return (dev_t)0;
}

static int exynos_fmp_host_get_type(struct device *dev,
					struct fmp_fips_data *data)
{
	int ret = 0;
	struct device_node *node = dev->of_node;
	const char *type;

	ret = of_property_read_string_index(node, "exynos,block-type", 0, &type);
	if (ret) {
		pr_err("%s: Could not get block type\n", __func__);
		ret = -EINVAL;
		goto err;
	}
	strlcpy(data->block_type, type, FMP_BLOCK_TYPE_NAME_LEN);
err:
	return ret;
}

static int exynos_fmp_get_fips_offset(struct device *dev,
					struct fmp_fips_data *data)
{
	int ret = 0;
	struct device_node *node = dev->of_node;
	uint32_t offset;

	ret = of_property_read_u32(node, "exynos,fips-block_offset", &offset);
	if (ret) {
		pr_err("%s: Could not get fips test block offset\n", __func__);
		ret = -EINVAL;
		goto err;
	}
	data->test_block_offset = offset;
err:
	return ret;
}

struct fmp_fips_data *do_fmp_fips_init(struct exynos_fmp *fmp)
{
	int ret = 0;
	struct fmp_fips_data *data;
	struct device *dev;
	struct inode *inode;
	struct super_block *sb;
	unsigned long blocksize;
	unsigned char blocksize_bits;

	sector_t self_test_block;
	fmode_t fmode = FMODE_WRITE | FMODE_READ;

	if (!fmp) {
		pr_err("%s: Invalid exynos fmp struct\n", __func__);
		return NULL;
	}

	dev = fmp->dev;

	data = kmalloc(sizeof(struct fmp_fips_data), GFP_KERNEL);
	if (!data) {
		ret = -ENOMEM;
		goto err;
	}

	ret = exynos_fmp_host_get_type(dev, data);
	if (ret) {
		dev_err(dev, "%s: Fail to get host type. ret(%d)", __func__, ret);
		goto err;
	}

	data->devt = find_devt_for_selftest(fmp, data);
	if (!data->devt) {
		dev_err(dev, "%s: Fail to find devt for self test\n", __func__);
		ret = -ENODEV;
		goto err;
	}

	data->bdev = blkdev_get_by_dev(data->devt, fmode, NULL);
	if (IS_ERR(data->bdev)) {
		dev_err(dev, "%s: Fail to open block device\n", __func__);
		ret = -ENODEV;
		goto err;
	}

	ret = exynos_fmp_get_fips_offset(dev, data);
	if (ret) {
		dev_err(dev, "%s: Fail to get fips offset. ret(%d)\n", __func__, ret);
		goto err;
	}

	inode = data->bdev->bd_inode;
	sb = inode->i_sb;
	blocksize = sb->s_blocksize;
	blocksize_bits = sb->s_blocksize_bits;
	self_test_block = (i_size_read(inode) - (blocksize * data->test_block_offset))
			  >> blocksize_bits;
	data->sector = self_test_block;

	return data;

err:
	kzfree(data);
	return NULL;
}

void do_fmp_fips_exit(struct fmp_fips_data *data)
{
	fmode_t fmode = FMODE_WRITE | FMODE_READ;

	if (!data)
		return;

	if (data->bdev)
		blkdev_put(data->bdev, fmode);
	kzfree(data);
}

static ssize_t fmp_fips_result_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct exynos_fmp *fmp = dev_get_drvdata(dev);

	return snprintf(buf, sizeof(pass), "%s\n", fmp->result.overall ? pass : fail);
}

static ssize_t fmp_fips_aes_xts_result_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct exynos_fmp *fmp = dev_get_drvdata(dev);

	return snprintf(buf, sizeof(pass), "%s\n", fmp->result.aes_xts ? pass : fail);
}

static ssize_t fmp_fips_aes_cbc_result_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct exynos_fmp *fmp = dev_get_drvdata(dev);

	return snprintf(buf, sizeof(pass), "%s\n", fmp->result.aes_cbc ? pass : fail);
}

static ssize_t fmp_fips_sha256_result_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct exynos_fmp *fmp = dev_get_drvdata(dev);

	return snprintf(buf, sizeof(pass), "%s\n", fmp->result.sha256 ? pass : fail);
}

static ssize_t fmp_fips_hmac_result_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct exynos_fmp *fmp = dev_get_drvdata(dev);

	return snprintf(buf, sizeof(pass), "%s\n", fmp->result.hmac ? pass : fail);
}

static ssize_t fmp_fips_integrity_result_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct exynos_fmp *fmp = dev_get_drvdata(dev);

	return snprintf(buf, sizeof(pass), "%s\n", fmp->result.integrity ? pass : fail);
}

static DEVICE_ATTR(fmp_fips_status, 0444, fmp_fips_result_show, NULL);
static DEVICE_ATTR(aes_xts_status, 0444, fmp_fips_aes_xts_result_show, NULL);
static DEVICE_ATTR(aes_cbc_status, 0444, fmp_fips_aes_cbc_result_show, NULL);
static DEVICE_ATTR(sha256_status, 0444, fmp_fips_sha256_result_show, NULL);
static DEVICE_ATTR(hmac_status, 0444, fmp_fips_hmac_result_show, NULL);
static DEVICE_ATTR(integrity_status, 0444, fmp_fips_integrity_result_show, NULL);

static struct attribute *fmp_fips_attr[] = {
	&dev_attr_fmp_fips_status.attr,
	&dev_attr_aes_xts_status.attr,
	&dev_attr_aes_cbc_status.attr,
	&dev_attr_sha256_status.attr,
	&dev_attr_hmac_status.attr,
	&dev_attr_integrity_status.attr,
	NULL,
};

static struct attribute_group fmp_fips_attr_group = {
	.name	= "fmp-fips",
	.attrs	= fmp_fips_attr,
};

static const struct file_operations fmp_fips_fops = {
	.owner		= THIS_MODULE,
	.open		= fmp_fips_open,
	.release	= fmp_fips_release,
	.unlocked_ioctl = fmp_fips_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= fmp_fips_compat_ioctl,
#endif
};

int exynos_fmp_fips_init(struct exynos_fmp *fmp)
{
	int ret = 0;

	if (!fmp || !fmp->dev) {
		pr_err("%s: Invalid exynos fmp dev\n", __func__);
		return -EINVAL;
	}

	set_fmp_fips_state(FMP_FIPS_INIT_STATE);

	fmp->miscdev.minor = MISC_DYNAMIC_MINOR;
	fmp->miscdev.name = "fmp";
	fmp->miscdev.fops = &fmp_fips_fops;
	ret = misc_register(&fmp->miscdev);
	if (ret) {
		dev_err(fmp->dev, "%s: Fail to register misc device. ret(%d)\n",
				__func__, ret);
		goto misc_err;
	}

	ret = sysfs_create_group(&fmp->dev->kobj, &fmp_fips_attr_group);
	if (ret) {
		dev_err(fmp->dev, "%s: Fail to create sysfs. ret(%d)\n",
				__func__, ret);
		goto sysfs_err;
	}

	set_fmp_fips_state(FMP_FIPS_PROGRESS_STATE);

	fmp->fips_data = do_fmp_fips_init(fmp);
	if (!fmp->fips_data) {
		dev_err(fmp->dev, "%s: Fail to initialize fips test.\n",
				__func__);
		goto err;
	}

	ret = do_fmp_selftest(fmp);
	if (ret) {
		dev_err(fmp->dev, "%s: self-tests for FMP failed\n", __func__);
		goto err;
	} else {
		dev_info(fmp->dev, "%s: self-tests for FMP passed\n", __func__);
	}

	ret = do_fmp_integrity_check(fmp);
	if (ret) {
		dev_err(fmp->dev, "%s: integrity check for FMP failed\n", __func__);
		fmp->result.integrity = 0;
		goto err;
	} else {
		dev_info(fmp->dev, "%s: integrity check for FMP passed\n", __func__);
		fmp->result.integrity = 1;
	}

	set_fmp_fips_state(FMP_FIPS_SUCCESS_STATE);
	fmp->result.overall = 1;

	do_fmp_fips_exit(fmp->fips_data);

	return 0;

err:
#if defined(CONFIG_NODE_FOR_SELFTEST_FAIL)
	set_fmp_fips_state(FMP_FIPS_ERR_STATE);
	fmp->result.overall = 0;
	do_fmp_fips_exit(fmp->fips_data);

	return 0;
#else
	panic("%s: Panic due to FMP self test for FIPS KAT", __func__);
#endif
sysfs_err:
misc_err:
	return -1;
}

void exynos_fmp_fips_exit(struct exynos_fmp *fmp)
{
	sysfs_remove_group(&fmp->dev->kobj, &fmp_fips_attr_group);
	misc_deregister(&fmp->miscdev);
}
