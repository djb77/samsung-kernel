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
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA  02110-1301, USA.
 */

#ifndef _CRYPTO_SEC_H
#define _CRYPTO_SEC_H

#include <crypto/sha.h>
#ifdef CONFIG_CRYPTO_FIPS
#include <crypto/rng.h>
#endif

/*
 * CONSTANT ONLY FOR INTERNAL
 */
#define HMAC_SIZE	SHA256_DIGEST_SIZE

/*
 * CONSTANT USED BY OUT OF CRYPTO_ODE
 */
#define EXT4_ESTIMATED_NONCE_SIZE	(EXT4_AES_256_XTS_KEY_SIZE + HMAC_SIZE)
#define EXT4_KEY_DERIVATION_NONCE_SIZE	96
#define EXT4_RNG_SEED_SIZE	16

int ext4_sec_get_key_aes(const char *source_data, const char *source_key, char *raw_key);
int ext4_sec_set_key_aes(char *save_key_data, const char *master_key_desc);

#ifdef CONFIG_CRYPTO_FIPS
extern struct crypto_rng *ext4_crypto_rng;
struct crypto_rng* ext4_sec_alloc_rng(void);
void ext4_sec_free_rng(struct crypto_rng* rng);
#endif

#endif	/* _CRYPTO_SEC_H */
