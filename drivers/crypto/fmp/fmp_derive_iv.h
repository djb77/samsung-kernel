/*
 * Exynos FMP derive iv for eCryptfs
 *
 * Copyright (C) 2015 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _FMP_DERIVE_IV_H_
#define _FMP_DERIVE_IV_H_

int fmplib_derive_iv(struct exynos_fmp *fmp,
			struct fmp_crypto_setting *crypto,
			enum fmp_crypto_enc_mode enc_mode);
#endif
