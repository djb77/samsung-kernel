/*
 * Exynos FMP derive iv for eCryptfs
 *
 * Copyright (C) 2014 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/crypto.h>
#include <linux/scatterlist.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <crypto/fmp.h>

int fmplib_derive_iv(struct exynos_fmp *fmp,
			struct fmp_crypto_setting *crypto,
			enum fmp_crypto_enc_mode enc_mode)
{
	int ret = 0;
	uint32_t extent_sector;
	uint32_t index;

	if (!fmp) {
		pr_err("%s: Invalid fmp driver platform data\n", __func__);
		ret = -ENODEV;
		goto err;
	}

	if (!crypto) {
		dev_err(fmp->dev, "%s: Invalid crypto data\n", __func__);
		ret = -EINVAL;
		goto err;
	}

	extent_sector = (uint32_t)crypto->sector;
	index = crypto->index;

	memset(crypto->iv, 0, FMP_IV_SIZE_16);
	if (crypto->algo_mode == EXYNOS_FMP_ALGO_MODE_AES_XTS)
		memcpy(crypto->iv, &extent_sector, sizeof(uint32_t));
	else if (crypto->algo_mode == EXYNOS_FMP_ALGO_MODE_AES_CBC)
		memcpy(crypto->iv, &index, sizeof(uint32_t));
	else {
		dev_err(fmp->dev, "%s: Invalid FMP algo mode (%d)\n",
				__func__, crypto->algo_mode);
		ret = -EINVAL;
		goto err;
	}
err:
	return ret;
}
