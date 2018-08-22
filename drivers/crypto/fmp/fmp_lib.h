/*
 * Copyright (C) 2016 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _FMP_LIB_H_
#define _FMP_LIB_H_

int fmplib_set_algo_mode(struct exynos_fmp *fmp,
				struct fmp_table_setting *table,
				struct fmp_crypto_setting *crypto,
				enum fmp_crypto_enc_mode enc_mode,
				bool cmdq_enabled);

int fmplib_set_key(struct exynos_fmp *fmp,
			struct fmp_table_setting *table,
			struct fmp_crypto_setting *crypto,
			enum fmp_crypto_enc_mode enc_mode);

int fmplib_set_key_size(struct exynos_fmp *fmp,
			struct fmp_table_setting *table,
			struct fmp_crypto_setting *crypto,
			enum fmp_crypto_enc_mode enc_mode,
			bool cmdq_enabled);

int fmplib_clear_disk_key(struct exynos_fmp *fmp);
int fmplib_clear(struct exynos_fmp *fmp, struct fmp_table_setting *table);

int fmplib_set_iv(struct exynos_fmp *fmp,
			struct fmp_table_setting *table,
			struct fmp_crypto_setting *crypto,
			enum fmp_crypto_enc_mode enc_mode);
#endif
