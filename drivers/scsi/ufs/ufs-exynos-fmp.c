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
#include <crypto/fmp.h>

#include "ufs-exynos-fmp.h"

static int check_data_equal(void *data1, void *data2)
{
	return data1 == data2;
}

static int is_valid_bio_data(struct bio *bio)
{
	if (bio->fmp_ci.private_enc_mode < 0 ||
			bio->fmp_ci.private_enc_mode > EXYNOS_FMP_DISK_ENC)
		return false;

	if (bio->fmp_ci.private_algo_mode < 0 ||
			bio->fmp_ci.private_algo_mode > EXYNOS_FMP_ALGO_MODE_AES_XTS)
		return false;

	return true;
}

static int exynos_ufs_fmp_key_size_cfg(struct fmp_crypto_setting *crypto,
					uint32_t size)
{
	int ret = 0;

	if (!crypto || !size) {
		pr_err("%s: Invalid fmp data or size.\n", __func__);
		ret = -EINVAL;
		goto out;
	}

	if (crypto->algo_mode == EXYNOS_FMP_ALGO_MODE_AES_XTS)
		size = size >> 1;

	switch (size) {
	case FMP_KEY_SIZE_16:
		crypto->key_size = EXYNOS_FMP_KEY_SIZE_16;
		break;
	case FMP_KEY_SIZE_32:
		crypto->key_size = EXYNOS_FMP_KEY_SIZE_32;
		break;
	default:
		pr_err("%s: FMP doesn't support key size %d\n", __func__, size);
		ret = -EINVAL;
		goto out;
	}
out:
	return ret;
}

static int exynos_ufs_fmp_iv_cfg(struct fmp_crypto_setting *crypto,
					sector_t sector, pgoff_t page_index,
					int sector_offset)
{
	int ret = 0;

	if (!crypto) {
		pr_err("%s: Invalid fmp data\n", __func__);
		ret = -EINVAL;
		goto out;
	}

	crypto->index = (uint32_t)page_index;
	crypto->sector = sector + (sector_t)sector_offset;
out:
	return ret;
}

static int exynos_ufs_fmp_key_cfg(struct fmp_crypto_setting *crypto,
					unsigned char *key,
					unsigned long key_length)
{
	int ret = 0;

	if (!crypto) {
		pr_err("%s: Invalid fmp data\n", __func__);
		ret = -EINVAL;
		goto out;
	}
	memset(crypto->key, 0, FMP_MAX_KEY_SIZE);
	memcpy(crypto->key, key, key_length);
out:
	return ret;
}

static int exynos_ufs_fmp_disk_cfg(struct scsi_cmnd *cmd,
					struct fmp_crypto_setting *crypto,
					int sector_offset)
{
	int ret = 0;
	struct bio *bio = cmd->request->bio;

	if (!crypto) {
		pr_err("%s: Invalid fmp data\n", __func__);
		ret = -EINVAL;
		goto out;
	}

	memset(crypto, 0, sizeof(struct fmp_crypto_setting));

	crypto->enc_mode = EXYNOS_FMP_DISK_ENC;

	if (!bio || !virt_addr_valid(bio))
		goto bypass_out;

	if ((bio->fmp_ci.private_algo_mode == EXYNOS_FMP_BYPASS_MODE) ||
			/* direct IO case */
			(bio->fmp_ci.private_enc_mode == EXYNOS_FMP_FILE_ENC))
		goto bypass_out;

	if (!is_valid_bio_data(bio))
		goto bypass_out;

	if (!bio->fmp_ci.key) {
		pr_err("%s: Invalid disk key\n", __func__);
		ret = -EINVAL;
		goto out;
	}

	crypto->algo_mode = bio->fmp_ci.private_algo_mode;
	ret = exynos_ufs_fmp_key_size_cfg(crypto, bio->fmp_ci.key_length);
	if (ret)
		goto bypass_out;

	ret = exynos_ufs_fmp_iv_cfg(crypto, bio->bi_iter.bi_sector, 0,
					sector_offset);
	if (ret) {
		pr_err("%s: Fail to configure fmp iv. ret(%d)\n",
				__func__, ret);
		ret = -EINVAL;
		goto out;
	}
out:
	return ret;
bypass_out:
	crypto->algo_mode = EXYNOS_FMP_BYPASS_MODE;
	ret = 0;
	return ret;
}

static int exynos_ufs_fmp_direct_io_cfg(struct scsi_cmnd *cmd,
					struct fmp_crypto_setting *crypto,
					int sector_offset)
{
	int ret = 0;
	struct bio *bio = cmd->request->bio;

	if (!crypto) {
		pr_err("%s: Invalid fmp data\n", __func__);
		ret = -EINVAL;
		goto out;
	}

	memset(crypto, 0, sizeof(struct fmp_crypto_setting));

	crypto->enc_mode = EXYNOS_FMP_FILE_ENC;

	if (!bio || !virt_addr_valid(bio) || (bio->fmp_ci.private_algo_mode == EXYNOS_FMP_BYPASS_MODE))
		goto bypass_out;

	if (!is_valid_bio_data(bio))
		goto bypass_out;

	crypto->algo_mode = bio->fmp_ci.private_algo_mode;
	ret = exynos_ufs_fmp_key_size_cfg(crypto, bio->fmp_ci.key_length);
	if (ret)
		goto bypass_out;

	ret = exynos_ufs_fmp_iv_cfg(crypto, bio->bi_iter.bi_sector, 0,
					sector_offset);
	if (ret) {
		pr_err("%s: Fail to configure fmp iv. ret(%d)\n",
				__func__, ret);
		ret = -EINVAL;
		goto out;
	}

	ret = exynos_ufs_fmp_key_cfg(crypto, bio->fmp_ci.key,
					bio->fmp_ci.key_length);
	if (ret) {
		pr_err("%s: Fail to configure fmp key. ret(%d)\n",
				__func__, ret);
		ret = -EINVAL;
		goto out;
	}
out:
	return ret;
bypass_out:
	crypto->algo_mode = EXYNOS_FMP_BYPASS_MODE;
	ret = 0;
	return ret;
}

static int exynos_ufs_fmp_file_cfg(struct scsi_cmnd *cmd,
					struct page *page,
					struct fmp_crypto_setting *crypto,
					int sector_offset)
{
	int ret = 0;
	struct bio *bio = cmd->request->bio;
	struct _fmp_ci *ci;

	if (!crypto) {
		pr_err("%s: Invalid fmp data\n", __func__);
		ret = -EINVAL;
		goto out;
	}

	memset(crypto, 0, sizeof(struct fmp_crypto_setting));

	if (!page || PageAnon(page))
		goto bypass_out;

	if (!page->mapping || !bio || !virt_addr_valid(bio))
		goto bypass_out;

	ci = &page->mapping->fmp_ci;

	if (ci->private_algo_mode == EXYNOS_FMP_BYPASS_MODE)
		goto bypass_out;

	crypto->algo_mode = ci->private_algo_mode;
	ret = exynos_ufs_fmp_key_size_cfg(crypto, ci->key_length);
	if (ret)
		goto bypass_out;

	ret = exynos_ufs_fmp_iv_cfg(crypto, bio->bi_iter.bi_sector, 0,
					sector_offset);
	if (ret) {
		pr_err("%s: Fail to configure fmp iv. ret(%d)\n",
				__func__, ret);
		ret = -EINVAL;
		goto out;
	}

	ret = exynos_ufs_fmp_key_cfg(crypto, ci->key, ci->key_length);
	if (ret) {
		pr_err("%s: Fail to configure fmp key. ret(%d)\n",
				__func__, ret);
		ret = -EINVAL;
		goto out;
	}
out:
	return ret;
bypass_out:
	crypto->algo_mode = EXYNOS_FMP_BYPASS_MODE;
	ret = 0;
	return ret;
}

int exynos_ufs_fmp_host_set_device(struct platform_device *host_pdev,
				struct platform_device *pdev,
				struct exynos_fmp_variant_ops *fmp_vops)
{
	struct exynos_ufs *ufs;

	if (!host_pdev || !pdev || !fmp_vops) {
		pr_err("%s: Fail to set device for fmp host\n", __func__);
		return -EINVAL;
	}

	ufs = dev_get_platdata(&host_pdev->dev);
	ufs->fmp.pdev = pdev;
	ufs->fmp.vops = fmp_vops;

	return 0;
}
EXPORT_SYMBOL(exynos_ufs_fmp_host_set_device);

static int is_ufs_fmp_test_enabled(struct scsi_cmnd *cmd,
				struct platform_device *pdev)
{
	struct bio *bio = cmd->request->bio;
	struct exynos_fmp *fmp = dev_get_drvdata(&pdev->dev);

	if (!fmp)
		return FALSE;

	if (!bio || !virt_addr_valid(bio)) {
		fmp->test_mode = 0;
		return FALSE;
	}

	if (check_data_equal((void *)bio->bi_private, (void *)fmp->test_bh)
			&& (uint64_t)fmp->test_bh) {
		fmp->test_mode = 1;
		return TRUE;
	}

	fmp->test_mode = 0;
	return FALSE;
}

static inline void exynos_ufs_fmp_bypass(void *desc)
{
	SET_DAS((struct fmp_table_setting *)desc, 0);
	SET_FAS((struct fmp_table_setting *)desc, 0);
}

int exynos_ufs_fmp_cfg(struct ufs_hba *hba,
				struct ufshcd_lrb *lrbp,
				struct scatterlist *sg,
				uint32_t index,
				int sector_offset)
{
	int ret;
	struct fmp_data_setting data;
	struct scsi_cmnd *cmd;
	struct page *page;
	struct exynos_ufs *ufs = dev_get_platdata(hba->dev);

	if (!ufs->fmp.pdev || !lrbp->cmd) {
		exynos_ufs_fmp_bypass(&lrbp->ucd_prdt_ptr[index]);
		return 0;
	}

	cmd = lrbp->cmd;
	page = sg_page(sg);

	if (cmd->cmnd[0] == REPORT_LUNS || cmd->cmnd[0] == INQUIRY ||
		cmd->cmnd[0] == MODE_SENSE_10 ||cmd->cmnd[0] == SERVICE_ACTION_IN_16 ) {	
		exynos_ufs_fmp_bypass(&lrbp->ucd_prdt_ptr[index]);
		return 0;
	}

	ret = is_ufs_fmp_test_enabled(cmd, ufs->fmp.pdev);
	if (ret == TRUE)
		goto out_test;

	ret = exynos_ufs_fmp_disk_cfg(cmd, &data.disk, sector_offset);
	if (ret) {
		pr_err("%s: Fail to configure FMP Disk Encryption. ret(%d)\n",
				__func__, ret);
		return -EINVAL;
	}

	if (data.disk.algo_mode != EXYNOS_FMP_BYPASS_MODE)
		goto file_cfg;

	ret = exynos_ufs_fmp_direct_io_cfg(cmd, &data.file, sector_offset);
	if (ret) {
		pr_err("%s: Fail to configure FMP direct IO Encryption. ret(%d)\n",
				__func__, ret);
		return -EINVAL;
	}

	if (data.file.algo_mode != EXYNOS_FMP_BYPASS_MODE)
		goto out;

file_cfg:
	ret = exynos_ufs_fmp_file_cfg(cmd, page, &data.file, sector_offset);
	if (ret) {
		pr_err("%s: Fail to configure FMP File Encryption. ret(%d)\n",
				__func__, ret);
		return -EINVAL;
	}

out:
	data.mapping = page->mapping;
out_test:
	data.table = (struct fmp_table_setting *)&lrbp->ucd_prdt_ptr[index];
	data.cmdq_enabled = 0;
	return ufs->fmp.vops->config(ufs->fmp.pdev, &data);
}
EXPORT_SYMBOL(exynos_ufs_fmp_cfg);

int exynos_ufs_fmp_clear(struct ufs_hba *hba,
				struct ufshcd_lrb *lrbp)
{
	int ret = 0;
	int sg_segments, idx;
	struct scatterlist *sg;
	struct exynos_ufs *ufs = dev_get_platdata(hba->dev);
	struct ufshcd_sg_entry *prd_table;
	struct fmp_data_setting data;

	if (ufs->fmp.pdev || !lrbp->cmd)
		goto out;

	sg_segments = scsi_sg_count(lrbp->cmd);
	if (!sg_segments)
		goto out;

	prd_table = (struct ufshcd_sg_entry *)lrbp->ucd_prdt_ptr;
	scsi_for_each_sg(lrbp->cmd, sg, sg_segments, idx) {
		data.table = (struct fmp_table_setting *)&prd_table[idx];

		if (!GET_FAS(data.table))
			continue;

		ret = ufs->fmp.vops->clear(ufs->fmp.pdev, &data);
		if (ret) {
			pr_err("%s: Fail to clear FMP desc (%d)\n",
				__func__, ret);
			ret = -EINVAL;
			goto out;
		}
	}
out:
	return ret;
}
