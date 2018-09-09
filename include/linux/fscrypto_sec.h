/*
 *  Copyright (C) 2016 Samsung Electronics Co., Ltd.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _FSCRYPTO_SEC_H
#define _FSCRYPTO_SEC_H

#include <crypto/sha.h>
#ifdef CONFIG_CRYPTO_FIPS
#include <crypto/rng.h>
#endif /* CONFIG CRYPTO_FIPS */

/*
 * CONSTANT ONLY FOR INTERNAL
 */
#define HMAC_SIZE	SHA256_DIGEST_SIZE

/*
 * CONSTANT USED BY OUT OF CRYPTO_ODE
 */
#define FS_ESTIMATED_NONCE_SIZE	(FS_AES_256_XTS_KEY_SIZE + HMAC_SIZE)
#define FS_KEY_DERIVATION_NONCE_SIZE	96
#define FS_RNG_SEED_SIZE	16

int fscrypt_sec_get_key_aes(const char *source_data, const char *source_key, char *raw_key);
int fscrypt_sec_set_key_aes(char *save_key_data, const char *master_key_desc);

int __init fscrypt_sec_crypto_init(void);
void __exit fscrypt_sec_crypto_exit(void);

#ifdef CONFIG_CRYPTO_FIPS
struct crypto_rng *fscrypt_sec_alloc_rng(void);
void fscrypt_sec_free_rng(struct crypto_rng *rng);
#endif /* CONFIG CRYPTO_FIPS */

#endif	/* _FSCRYPTO_SEC_H */
