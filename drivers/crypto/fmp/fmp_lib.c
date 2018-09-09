/*
 * Exynos FMP libary
 *
 * Copyright (C) 2015 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/smc.h>
#include <crypto/fmp.h>

#include <asm/cacheflush.h>

#include "fmp_fips_info.h"

#define WORD_SIZE 4

#define byte2word(b0, b1, b2, b3)       \
			(((unsigned int)(b0) << 24) | \
			((unsigned int)(b1) << 16) | \
			((unsigned int)(b2) << 8) | (b3))
#define get_word(x, c)  byte2word(((unsigned char *)(x) + 4 * (c))[0], \
				((unsigned char *)(x) + 4 * (c))[1], \
				((unsigned char *)(x) + 4 * (c))[2], \
				((unsigned char *)(x) + 4 * (c))[3])

static int check_aes_xts_size(struct fmp_table_setting *table,
				bool cmdq_enabled)
{
	int size;

	if (cmdq_enabled)
		size = GET_CMDQ_LENGTH(table);
	else
		size = GET_LENGTH(table);
	return (size > MAX_AES_XTS_TRANSFER_SIZE) ? size : 0;
}

static int check_aes_xts_key(char *key, enum fmp_crypto_key_size key_size)
{
	char *enckey, *twkey;

	enckey = key;
	twkey = key + key_size;

	return (memcmp(enckey, twkey, key_size)) ? 0 : -1;
}

int fmplib_set_algo_mode(struct exynos_fmp *fmp,
				struct fmp_table_setting *table,
				struct fmp_crypto_setting *crypto,
				enum fmp_crypto_enc_mode enc_mode,
				bool cmdq_enabled)
{
	int ret = 0;
	enum fmp_crypto_algo_mode algo_mode;

	if (!fmp || !fmp->dev) {
		pr_err("%s: invalid fmp device\n", __func__);
		ret = -EINVAL;
		goto err;
	}

	if (!table || !crypto) {
		dev_err(fmp->dev, "%s: Invalid fmp table or crypto\n",
				__func__);
		ret = -EINVAL;
		goto err;
	}

	algo_mode = crypto->algo_mode;

	if (algo_mode == EXYNOS_FMP_ALGO_MODE_AES_XTS) {
		ret = check_aes_xts_size(table, cmdq_enabled);
		if (ret) {
			dev_err(fmp->dev,
				"%s: Fail FMP XTS due to invalid size(%d)\n",
				__func__, ret);
			ret = -EINVAL;
			goto err;
		}
	}

	switch (enc_mode) {
	case EXYNOS_FMP_FILE_ENC:
		if (cmdq_enabled)
			SET_CMDQ_FAS(table, algo_mode);
		else
			SET_FAS(table, algo_mode);
		break;
	case EXYNOS_FMP_DISK_ENC:
		if (cmdq_enabled)
			SET_CMDQ_DAS(table, algo_mode);
		else
			SET_DAS(table, algo_mode);
		break;
	default:
		dev_err(fmp->dev, "%s: Invalid fmp enc mode %d\n",
				__func__, enc_mode);
		ret = -EINVAL;
		goto err;
	}
err:
	return ret;
}

int fmplib_set_key(struct exynos_fmp *fmp,
			struct fmp_table_setting *table,
			struct fmp_crypto_setting *crypto,
			enum fmp_crypto_enc_mode enc_mode)
{
	int ret = 0;
	enum fmp_crypto_algo_mode algo_mode;
	enum fmp_crypto_key_size key_size;
	char *key;
	int idx, max;

	if (!fmp || !fmp->dev) {
		pr_err("%s: invalid fmp device\n", __func__);
		ret = -EINVAL;
		goto err;
	}

	if (!table || !crypto) {
		dev_err(fmp->dev, "%s: Invalid fmp table or crypto\n",
				__func__);
		ret = -EINVAL;
		goto err;
	}

	algo_mode = crypto->algo_mode;
	key = crypto->key;
	key_size = crypto->key_size;

	if (algo_mode == EXYNOS_FMP_ALGO_MODE_AES_XTS) {
		if (check_aes_xts_key(key, key_size)) {
			dev_err(fmp->dev,
				"%s: Fail FMP XTS due to the same enc and twkey\n",
				__func__);
			ret = -EINVAL;
			goto err;
		}
	}

	switch (enc_mode) {
	case EXYNOS_FMP_FILE_ENC:
		if (algo_mode == EXYNOS_FMP_ALGO_MODE_AES_CBC) {
			max = key_size / WORD_SIZE;
			for (idx = 0; idx < max; idx++)
				*(&table->file_enckey0 + idx) =
					get_word(key, max - (idx + 1));
		} else if (algo_mode == EXYNOS_FMP_ALGO_MODE_AES_XTS) {
			key_size *= 2;
			max = key_size / WORD_SIZE;
			for (idx = 0; idx < (max / 2); idx++)
				*(&table->file_enckey0 + idx) =
					get_word(key, (max / 2) - (idx + 1));
			for (idx = 0; idx < (max / 2); idx++)
				*(&table->file_twkey0 + idx) =
					get_word(key, max - (idx + 1));
		}
		break;
	case EXYNOS_FMP_DISK_ENC:
		__flush_dcache_area(key, (size_t)FMP_MAX_KEY_SIZE);
		ret = exynos_smc(SMC_CMD_FMP_DISK_KEY_STORED, 0,
			virt_to_phys(key), key_size);
		if (ret) {
			dev_err(fmp->dev, "%s: Fail to set FMP disk key. ret = %d\n",
				__func__, ret);
			fmp->status_disk_key = KEY_ERROR;
			ret = -EINVAL;
			goto err;
		}
		fmp->status_disk_key = KEY_STORED;
		break;
	default:
		dev_err(fmp->dev, "%s: Invalid fmp enc mode %d\n", __func__, enc_mode);
		ret = -EINVAL;
		goto err;
	}
err:
	return ret;
}

int fmplib_set_key_size(struct exynos_fmp *fmp,
			struct fmp_table_setting *table,
			struct fmp_crypto_setting *crypto,
			enum fmp_crypto_enc_mode enc_mode,
			bool cmdq_enabled)
{
	int ret = 0;
	enum fmp_crypto_key_size key_size;

	if (!fmp || !fmp->dev) {
		pr_err("%s: invalid fmp device\n", __func__);
		ret = -EINVAL;
		goto err;
	}

	if (!table || !crypto) {
		dev_err(fmp->dev, "%s: Invalid fmp table or crypto\n",
				__func__);
		ret = -EINVAL;
		goto err;
	}

	key_size = crypto->key_size;

	switch (enc_mode) {
	case EXYNOS_FMP_FILE_ENC:
		if (cmdq_enabled)
			SET_CMDQ_KEYLEN(table, (key_size == FMP_KEY_SIZE_32) ? FKL_CMDQ : 0);
		else
			SET_KEYLEN(table, (key_size == FMP_KEY_SIZE_32) ? FKL : 0);
		break;
	case EXYNOS_FMP_DISK_ENC:
		if (cmdq_enabled)
			SET_CMDQ_KEYLEN(table, (key_size == FMP_KEY_SIZE_32) ? DKL_CMDQ : 0);
		else
			SET_KEYLEN(table, (key_size == FMP_KEY_SIZE_32) ? DKL : 0);
		break;
	default:
		dev_err(fmp->dev, "%s: Invalid fmp enc mode %d\n", __func__, enc_mode);
		ret = -EINVAL;
		goto err;
	}
err:
	return ret;
}

int fmplib_clear_disk_key(struct exynos_fmp *fmp)
{
	int ret = 0;

	if (!fmp || !fmp->dev) {
		pr_err("%s: invalid fmp device\n", __func__);
		ret = -EINVAL;
		goto err;
	}

	ret = exynos_smc(SMC_CMD_FMP_DISK_KEY_CLEAR, 0, 0, 0);
	if (ret) {
		dev_err(fmp->dev, "%s: Fail to clear FMP disk key. ret = %d\n",
							__func__, ret);
		fmp->status_disk_key = KEY_ERROR;
		ret = -EPERM;
		goto err;
	}

	fmp->status_disk_key = KEY_CLEAR;
err:
	return ret;
}

int fmplib_clear(struct exynos_fmp *fmp, struct fmp_table_setting *table)
{
	int ret = 0;
	struct exynos_fmp_fips_test_vops *test_vops;

	if (!fmp || !fmp->dev) {
		pr_err("%s: invalid fmp device\n", __func__);
		ret = -EINVAL;
		goto err;
	}

	if (!table) {
		dev_err(fmp->dev, "%s: Invalid fmp table\n", __func__);
		ret = -EINVAL;
		goto err;
	}

	test_vops = (struct exynos_fmp_fips_test_vops *)fmp->test_vops;
	if (test_vops) {
		ret = test_vops->zeroization(table, "before");
		if (ret)
			dev_err(fmp->dev,
				"%s: Fail to check zeroization(before)\n",
				__func__);
	}
	memset(&table->file_iv0, 0, sizeof(__le32) * 24);
	if (test_vops) {
		ret = test_vops->zeroization(table, "after");
		if (ret)
			dev_err(fmp->dev,
				"%s: Fail to check zeroization(after)\n",
				__func__);
	}
err:
	return ret;
}

int fmplib_set_iv(struct exynos_fmp *fmp,
		struct fmp_table_setting *table,
		struct fmp_crypto_setting *crypto,
		enum fmp_crypto_enc_mode enc_mode)
{
	int idx, max;
	int ret = 0;
	uint8_t *iv;

	if (!fmp || !fmp->dev) {
		pr_err("%s: invalid fmp device\n", __func__);
		ret = -EINVAL;
		goto err;
	}

	if (!table || !crypto) {
		dev_err(fmp->dev, "%s: Invalid fmp table or crypto\n",
				__func__);
		ret = -EINVAL;
		goto err;
	}

	iv = crypto->iv;
	max = FMP_IV_SIZE_16 / WORD_SIZE;
	switch (enc_mode) {
	case EXYNOS_FMP_FILE_ENC:
		for (idx = 0; idx < max; idx++)
			*(&table->file_iv0 + idx) = get_word(iv, max - (idx + 1));
		break;
	case EXYNOS_FMP_DISK_ENC:
		for (idx = 0; idx < max; idx++)
			*(&table->disk_iv0 + idx) = get_word(iv, max - (idx + 1));
		break;
	default:
		dev_err(fmp->dev, "%s: Invalid fmp enc mode %d\n", __func__, enc_mode);
		ret = -EINVAL;
		goto err;
	}
err:
	return ret;
}
