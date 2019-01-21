/*
 * Exynos FMP cipher driver
 *
 * Copyright (C) 2016 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/slab.h>
#include <linux/buffer_head.h>

#include <crypto/fmp.h>

#include "fmp_fips_info.h"

int fmp_cipher_set_key(struct exynos_fmp *fmp,
			struct fmp_fips_data *fdata,
			uint32_t mode, uint8_t *key, uint32_t key_len)
{
	int ret = 0;
	struct fmp_crypto_setting *crypto;
	struct device *dev = fmp->dev;

	if (!fdata) {
		dev_err(dev, "%s: fmp test data context is not allocated\n",
				__func__);
		ret = -ENOMEM;
		goto err;
	}

	crypto = &fdata->crypto;
	memset(crypto->key, 0, FMP_MAX_KEY_SIZE);
	if (mode == CBC_MODE) {
		crypto->algo_mode = EXYNOS_FMP_ALGO_MODE_AES_CBC;
		switch (key_len) {
		case FMP_KEY_SIZE_16:
			crypto->key_size = EXYNOS_FMP_KEY_SIZE_16;
			break;
		case FMP_KEY_SIZE_32:
			crypto->key_size = EXYNOS_FMP_KEY_SIZE_32;
			break;
		default:
			dev_err(dev, "%s: Invalid FMP CBC key size(%d)\n",
				__func__, key_len);
			ret = -EINVAL;
			goto err;
		}
		memcpy(crypto->key, key, key_len);
	} else if (mode == XTS_MODE) {
		crypto->algo_mode = EXYNOS_FMP_ALGO_MODE_AES_XTS;
		switch (key_len >> 1) {
		case FMP_KEY_SIZE_16:
			crypto->key_size = EXYNOS_FMP_KEY_SIZE_16;
			break;
		case FMP_KEY_SIZE_32:
			crypto->key_size = EXYNOS_FMP_KEY_SIZE_32;
			break;
		default:
			dev_err(dev, "%s: Invalid FMP XTS key size(%d)\n",
				__func__, key_len);
			ret = -EINVAL;
			goto err;
		}
		memcpy(crypto->key, key, key_len);
	} else if (mode == BYPASS_MODE) {
		crypto->algo_mode = EXYNOS_FMP_BYPASS_MODE;
		goto err;
	} else {
		dev_err(dev, "%s: Invalid FMP encryption mode(%d)\n",
				__func__, mode);
		ret = -EINVAL;
		goto err;
	}
err:
	return ret;
}

int fmp_cipher_set_iv(struct exynos_fmp *fmp,
			struct fmp_fips_data *fdata,
			uint32_t mode, uint8_t *iv, uint32_t iv_len)
{
	int ret = 0;
	struct fmp_crypto_setting *crypto;
	struct device *dev = fmp->dev;

	if (!fdata) {
		dev_err(dev, "%s: fmp test data context is not allocated\n",
				__func__);
		ret = -ENOMEM;
		goto err;
	}

	crypto = &fdata->crypto;
	if (iv_len != FMP_IV_SIZE_16) {
		dev_err(dev, "%s: Invalid FMP iv size(%d)\n", __func__, iv_len);
		ret = -EINVAL;
		goto err;
	}

	if (mode == CBC_MODE || mode == XTS_MODE)
		memcpy(crypto->iv, iv, iv_len);
	else if (mode == BYPASS_MODE)
		memset(crypto->iv, 0, FMP_IV_SIZE_16);
	else {
		dev_err(dev, "%s: Invalid mode(%d)\n", __func__, mode);
		ret = -EINVAL;
		goto err;
	}

err:
	return ret;
}

int fmp_cipher_run(struct exynos_fmp *fmp,
			struct fmp_fips_data *fdata, uint32_t mode,
			uint8_t *data, uint32_t len, uint32_t write)
{
	int ret = 0;
	struct device *dev = fmp->dev;
	static struct buffer_head *bh;
	struct fmp_crypto_setting *crypto;

	if (!fdata) {
		dev_err(dev, "%s: fmp test data context is not allocated\n",
				__func__);
		return -ENOMEM;
	}

	crypto = &fdata->crypto;
	bh = __getblk(fdata->bdev, fdata->sector, FMP_BLK_SIZE);
	if (!bh) {
		dev_err(dev, "%s: Fail to get block from bdev:%p sector:%d devt:%d\n",
			__func__, fdata->bdev, (int)fdata->sector, fdata->devt);
		return -ENODEV;
	}

	if (mode == CBC_MODE)
		crypto->algo_mode = EXYNOS_FMP_ALGO_MODE_AES_CBC;
	else if (mode == XTS_MODE)
		crypto->algo_mode = EXYNOS_FMP_ALGO_MODE_AES_XTS;
	else if (mode == BYPASS_MODE)
		crypto->algo_mode = EXYNOS_FMP_BYPASS_MODE;
	else {
		dev_err(dev, "%s: Invalid algo mode(%d)\n", __func__, mode);
		return -EINVAL;
	}

	fmp->fips_data = fdata;
	fmp->test_bh = bh;
	get_bh(bh);
	if (write == WRITE_MODE) {
		memcpy(bh->b_data, data, len);
		set_buffer_dirty(bh);
		sync_dirty_buffer(bh);
		if (buffer_req(bh) && !buffer_uptodate(bh)) {
			dev_err(dev, "%s: IO error syncing for write mode\n",
					__func__);
			ret = -EIO;
			goto out;
		}
		memset(bh->b_data, 0, FMP_BLK_SIZE);
	} else {
		lock_buffer(bh);
		bh->b_end_io = end_buffer_read_sync;
		submit_bh(REQ_OP_READ, READ_SYNC | REQ_PRIO, bh);
		wait_on_buffer(bh);
		if (unlikely(!buffer_uptodate(bh))) {
			ret = -EIO;
			goto out;
		}
		memcpy(data, bh->b_data, len);
	}
out:
	fmp->test_bh = NULL;
	put_bh(bh);

	return ret;
}
