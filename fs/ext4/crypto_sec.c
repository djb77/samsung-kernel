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

#ifdef CONFIG_EXT4_SEC_CRYPTO_EXTENSION

#include <keys/encrypted-type.h>
#include <keys/user-type.h>
#include <linux/random.h>
#include <linux/scatterlist.h>
#include <uapi/linux/keyctl.h>
#include <crypto/hash.h>

#include "ext4.h"

static void crypt_cbc_complete(struct crypto_async_request *req, int rc)
{
	struct ext4_completion_result *ecr = req->data;

	if (rc == -EINPROGRESS)
		return;

	ecr->res = rc;
	complete(&ecr->completion);
}

static int __hmac_sha256(u8 *key, u8 ksize, char *plaintext, u8 psize, u8 *digest)
{
	struct crypto_shash *tfm;
	int res = 0;

	if (!ksize || !psize)
		return -EINVAL;

	if (key == NULL || plaintext == NULL || digest == NULL)
		return -EINVAL;

	tfm = crypto_alloc_shash("hmac(sha256)", 0, 0);
	if (IS_ERR(tfm)) {
		printk_once(KERN_DEBUG
			"crypto_alloc_ahash failed : err %ld", PTR_ERR(tfm));
		return PTR_ERR(tfm);
	}

	res = crypto_shash_setkey(tfm, key, ksize);
	if (res) {
		printk_once(KERN_DEBUG
			"crypto_ahash_setkey failed: err %d", res);
	} else {
		char desc[sizeof(struct shash_desc) +
			crypto_shash_descsize(tfm)] CRYPTO_MINALIGN_ATTR;
		struct shash_desc *shash = (struct shash_desc *)desc;

		shash->tfm = tfm;
		shash->flags = CRYPTO_TFM_REQ_MAY_SLEEP;

		res = crypto_shash_digest(shash, plaintext, psize, digest);
	}

	if (tfm)
		crypto_free_shash(tfm);

	return res;
}

/**
 * ext4_sec_get_key_aes() - Get a key using AES-256-CBC
 * @source_data: Encrypted key and HMAC.
 * @source_key:  Source key to which to apply decryption.
 * @raw_key:     Decrypted key.
 *
 * Return: Zero on success; non-zero otherwise.
 */
int ext4_sec_get_key_aes(const char *source_data, const char *source_key, char *raw_key)
{
	DECLARE_EXT4_COMPLETION_RESULT(ecr);
	struct crypto_ablkcipher *tfm = NULL;
	struct ablkcipher_request *req = NULL;
	struct scatterlist src_sg, dst_sg;

	char encrypted_key[EXT4_AES_256_XTS_KEY_SIZE];
	char fek_hmac[HMAC_SIZE];
	char cbc_key[EXT4_AES_256_CBC_KEY_SIZE];
	char cbc_iv[EXT4_XTS_TWEAK_SIZE];
	char digest[HMAC_SIZE];

	int res = 0;

	// Split encrypted key
	memcpy(encrypted_key, source_data, EXT4_AES_256_XTS_KEY_SIZE);
	memcpy(fek_hmac, source_data+EXT4_AES_256_XTS_KEY_SIZE, HMAC_SIZE);

	// Split master key
	memcpy(cbc_key, source_key, EXT4_AES_256_CBC_KEY_SIZE);
	memcpy(cbc_iv, source_key+EXT4_AES_256_CBC_KEY_SIZE, EXT4_XTS_TWEAK_SIZE);

	// Decrypt encrypted FEK
	tfm = crypto_alloc_ablkcipher("cbc(aes)", 0, 0);
	if (IS_ERR(tfm)) {
		res = PTR_ERR(tfm);
		tfm = NULL;
		goto out;
	}

	crypto_ablkcipher_clear_flags(tfm, ~0);
	crypto_ablkcipher_set_flags(tfm, CRYPTO_TFM_REQ_WEAK_KEY);
	req = ablkcipher_request_alloc(tfm, GFP_NOFS);
	if (!req) {
		res = -ENOMEM;
		goto out;
	}
	ablkcipher_request_set_callback(req,
			CRYPTO_TFM_REQ_MAY_BACKLOG | CRYPTO_TFM_REQ_MAY_SLEEP,
			crypt_cbc_complete , &ecr);
	res = crypto_ablkcipher_setkey(tfm, cbc_key, EXT4_AES_256_CBC_KEY_SIZE);
	if (res < 0)
		goto out;

	sg_init_one(&src_sg, encrypted_key, EXT4_AES_256_XTS_KEY_SIZE);
	sg_init_one(&dst_sg, raw_key, EXT4_AES_256_XTS_KEY_SIZE);
	ablkcipher_request_set_crypt(req, &src_sg, &dst_sg,
					EXT4_AES_256_XTS_KEY_SIZE, cbc_iv);
	res = crypto_ablkcipher_decrypt(req);
	if (res == -EINPROGRESS || res == -EBUSY) {
		wait_for_completion(&ecr.completion);
		res = ecr.res;
		goto out;
	}

	// Make HMAC for FEK with master key
	res = __hmac_sha256(cbc_key, EXT4_AES_256_CBC_KEY_SIZE,
				encrypted_key, EXT4_AES_256_XTS_KEY_SIZE, digest);
	if (res)
		goto out;

	// Check HMAC value
	if (memcmp(digest, fek_hmac, HMAC_SIZE)) {
		printk(KERN_DEBUG "ext4: mac mismatch!\n");
		res = -EBADMSG;
	}
out:
	if (req)
		ablkcipher_request_free(req);
	if (tfm)
		crypto_free_ablkcipher(tfm);

	memzero_explicit(encrypted_key, EXT4_AES_256_XTS_KEY_SIZE);
	memzero_explicit(cbc_key, EXT4_AES_256_CBC_KEY_SIZE);
	memzero_explicit(cbc_iv, EXT4_XTS_TWEAK_SIZE);
	memzero_explicit(digest, HMAC_SIZE);

	return res;
}

/**
 * ext4_sec_set_key_aes() - Generate and save a random key for AES-256.
 * @save_key_data:   Saving space for encrypted key and HMAC.
 * @master_key_desc: keyring descriptor for FEK and HMAC.
 *
 * Return: Zero on success; non-zero otherwise.
 */
int ext4_sec_set_key_aes(char* save_key_data, const char *master_key_desc)
{
	char full_key_descriptor[EXT4_KEY_DESC_PREFIX_SIZE +
				(EXT4_KEY_DESCRIPTOR_SIZE * 2) + 1];
	struct key *keyring_key = NULL;
	struct ext4_encryption_key *master_key;
	const struct user_key_payload *ukp;

	DECLARE_EXT4_COMPLETION_RESULT(ecr);
	struct crypto_ablkcipher *tfm = NULL;
	struct ablkcipher_request *req = NULL;
	struct scatterlist src_sg, dst_sg;

	char encrypted_key[EXT4_AES_256_XTS_KEY_SIZE];
	char cbc_key[EXT4_AES_256_CBC_KEY_SIZE];
	char cbc_iv[EXT4_XTS_TWEAK_SIZE];
	char raw_key[EXT4_AES_256_XTS_KEY_SIZE];
	char digest[HMAC_SIZE];

	int res = 0;

	// Get Keyring
	memcpy(full_key_descriptor, EXT4_KEY_DESC_PREFIX,
			EXT4_KEY_DESC_PREFIX_SIZE);
	sprintf(full_key_descriptor + EXT4_KEY_DESC_PREFIX_SIZE,
		"%*phN", EXT4_KEY_DESCRIPTOR_SIZE,
		master_key_desc);
	full_key_descriptor[EXT4_KEY_DESC_PREFIX_SIZE +
				(2 * EXT4_KEY_DESCRIPTOR_SIZE)] = '\0';
	keyring_key = request_key(&key_type_logon, full_key_descriptor, NULL);
	if (IS_ERR(keyring_key)) {
		printk(KERN_WARNING "%s: error get keyring! (%s)\n",
			__func__, full_key_descriptor);
		res = PTR_ERR(keyring_key);
		keyring_key = NULL;
		goto out;
	}

	if (keyring_key->type != &key_type_logon) {
		printk(KERN_WARNING "%s: key type must be logon\n", __func__);
		res = -ENOKEY;
		goto out;
	}

	// Get master key
	down_read(&keyring_key->sem);
	ukp = user_key_payload(keyring_key);
	if (ukp->datalen != sizeof(struct ext4_encryption_key)) {
		res = -EINVAL;
		goto out_unlock;
	}
	master_key = (struct ext4_encryption_key *)ukp->data;

	BUILD_BUG_ON(EXT4_ESTIMATED_NONCE_SIZE !=
		     EXT4_KEY_DERIVATION_NONCE_SIZE);

	if (master_key->size != EXT4_AES_256_XTS_KEY_SIZE) {
		printk(KERN_WARNING "%s: key size incorrect (%d)\n",
			__func__, master_key->size);
		res = -ENOKEY;
		goto out_unlock;
	}

	// Split master key
	memcpy(cbc_key, master_key->raw, EXT4_AES_256_CBC_KEY_SIZE);
	memcpy(cbc_iv, (master_key->raw)+EXT4_AES_256_CBC_KEY_SIZE, EXT4_XTS_TWEAK_SIZE);

	// Get FEK
#ifdef CONFIG_CRYPTO_FIPS
	ext4_crypto_rng = ext4_sec_alloc_rng();
	if (!ext4_crypto_rng) {
		printk_once(KERN_DEBUG
				"ext4: error get crypto rng handler\n");
		goto out_unlock;
	}

	res = crypto_rng_get_bytes(ext4_crypto_rng, raw_key, EXT4_AES_256_XTS_KEY_SIZE);
	if (res < 0) {
		printk(KERN_DEBUG
			"ext4: failed to generate random number (%d)\n", res);
		goto out_unlock;
	}
#else
	get_random_bytes(raw_key, EXT4_AES_256_XTS_KEY_SIZE);
#endif

	// Encrypt FEK
	tfm = crypto_alloc_ablkcipher("cbc(aes)", 0, 0);
	if (IS_ERR(tfm)) {
		res = PTR_ERR(tfm);
		tfm = NULL;
		goto out_unlock;
	}

	crypto_ablkcipher_clear_flags(tfm, ~0);
	crypto_ablkcipher_set_flags(tfm, CRYPTO_TFM_REQ_WEAK_KEY);
	req = ablkcipher_request_alloc(tfm, GFP_NOFS);
	if (!req) {
		res = -ENOMEM;
		goto out_unlock;
	}
	ablkcipher_request_set_callback(req,
			CRYPTO_TFM_REQ_MAY_BACKLOG | CRYPTO_TFM_REQ_MAY_SLEEP,
			crypt_cbc_complete , &ecr);
	res = crypto_ablkcipher_setkey(tfm, cbc_key, EXT4_AES_256_CBC_KEY_SIZE);
	if (res < 0){
		printk(KERN_DEBUG
				"ext4: can't set key for cbc\n");
		goto out_unlock;
	}

	sg_init_one(&src_sg, raw_key, EXT4_AES_256_XTS_KEY_SIZE);
	sg_init_one(&dst_sg, encrypted_key, EXT4_AES_256_XTS_KEY_SIZE);
	ablkcipher_request_set_crypt(req, &src_sg, &dst_sg,
			EXT4_AES_256_XTS_KEY_SIZE, cbc_iv);
	res = crypto_ablkcipher_encrypt(req);
	if (res == -EINPROGRESS || res == -EBUSY) {
		wait_for_completion(&ecr.completion);
		res = ecr.res;
		goto out_unlock;
	}

	// Make HMAC for FEK with master key
	res = __hmac_sha256(cbc_key, EXT4_AES_256_CBC_KEY_SIZE,
				encrypted_key, EXT4_AES_256_XTS_KEY_SIZE, digest);
	if (res) {
		printk(KERN_DEBUG
				"ext4: can't calculation hmac\n");
		goto out_unlock;
	}

	// Save encrypted FEK and HMAC value
	memcpy(save_key_data, encrypted_key, EXT4_AES_256_XTS_KEY_SIZE);
	memcpy(save_key_data+EXT4_AES_256_XTS_KEY_SIZE, digest, HMAC_SIZE);

out_unlock:
	up_read(&keyring_key->sem);
out:
	if (req)
		ablkcipher_request_free(req);
	if (tfm)
		crypto_free_ablkcipher(tfm);
	if (keyring_key)
		key_put(keyring_key);

	memzero_explicit(encrypted_key, EXT4_AES_256_XTS_KEY_SIZE);
	memzero_explicit(raw_key, EXT4_AES_256_XTS_KEY_SIZE);
	memzero_explicit(cbc_key, EXT4_AES_256_CBC_KEY_SIZE);
	memzero_explicit(cbc_iv, EXT4_XTS_TWEAK_SIZE);
	memzero_explicit(digest, HMAC_SIZE);

	return res;
}

/* DRBG: Deterministic Random Bit Generator */
#ifdef CONFIG_CRYPTO_FIPS
/**
 * ext4_sec_alloc_rng() - Setup for DRBG for random number generating.
 *
 * Return: crypto_rng pointer on success; NULL otherwise.
 */
struct crypto_rng* ext4_sec_alloc_rng()
{
	struct crypto_rng *rng = NULL;
	struct file *filp = NULL;
	char *rng_seed = NULL;
	mm_segment_t fs_type;

	int read = 0;
	int get_bytes = 0;
	int trial = 10;
	int res = -1;

	if (ext4_crypto_rng)
		goto already_initialized;

	rng_seed = kmalloc(EXT4_RNG_SEED_SIZE, GFP_KERNEL);
	if (!rng_seed) {
		printk(KERN_DEBUG
			"ext4: failed to get memory\n");
		goto out;
	}
	memset((void *)rng_seed, 0, EXT4_RNG_SEED_SIZE);

	// open random device for drbg seed number
	filp = filp_open("/dev/random", O_RDONLY, 0);
	if (IS_ERR(filp)) {
		printk(KERN_DEBUG
			"ext4: failed to open random device\n");
		goto out;
	}
	fs_type = get_fs();
	set_fs(KERNEL_DS);

	while (trial > 0) {
                if ((get_bytes = (int)filp->f_op->read(filp, &(rng_seed[read]), EXT4_RNG_SEED_SIZE-read, &filp->f_pos)) > 0)
			read += get_bytes;

		if (read == EXT4_RNG_SEED_SIZE)
			break;

		trial--;
	}
	set_fs(fs_type);

	if (read != EXT4_RNG_SEED_SIZE) {
		printk(KERN_DEBUG
			"ext4: failed to get enough random bytes (read=%d / request=%d)\n",
			read, EXT4_RNG_SEED_SIZE);
		goto out;
	}

	// create drbg for random number generation
	rng = crypto_alloc_rng("stdrng", 0, 0);
	if (IS_ERR(rng)) {
		printk(KERN_DEBUG
			"ext4: rng allocation fail, not available (%ld)\n",
			PTR_ERR(rng));
		goto out;
	}

	// push seed to drbg
	res = crypto_rng_reset(rng, rng_seed, EXT4_RNG_SEED_SIZE);
	if (res < 0) {
		printk(KERN_DEBUG
			"ext4: rng reset fail (%d)\n", res);
	}

out:
	if (rng_seed)
		kfree(rng_seed);
	if (filp)
		filp_close(filp, NULL);
	if (res && rng) {
		ext4_sec_free_rng(rng);
		rng = NULL;
	}

	return rng;

already_initialized:
	return ext4_crypto_rng;
}

/**
 * ext4_sec_free_rng() - Shutdown the DRBG
 */
void ext4_sec_free_rng(struct crypto_rng* rng)
{
	if (!rng) return;

	printk(KERN_DEBUG
		"ext4: free crypto rng!\n");
	crypto_free_rng(rng);
}
#endif

#endif /* CONFIG_EXT4_SEC_CRYPTO_EXTENSION */
