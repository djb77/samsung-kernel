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

#include <keys/encrypted-type.h>
#include <keys/user-type.h>
#include <linux/random.h>
#include <linux/scatterlist.h>
#include <uapi/linux/keyctl.h>
#include <crypto/hash.h>

#include <linux/fscrypto.h>

/*
 * DRBG is needed for FIPS
 *
 * DRBG: Deterministic Random Bit Generator
 */
#ifdef CONFIG_CRYPTO_FIPS
static struct crypto_rng *fscrypt_rng;

/**
 * fscrypt_sec_exit_rng() - Shutdown the DRBG
 */
static void fscrypt_sec_exit_rng(void)
{
	if (!fscrypt_rng)
		return;

	printk(KERN_DEBUG "fscrypto: free crypto rng!\n");
	crypto_free_rng(fscrypt_rng);
	fscrypt_rng = NULL;
}

/**
 * fscrypt_sec_init_rng() - Setup for DRBG for random number generating.
 *
 * This function will modify global variable named fscrypt_rng.
 * Return: zero on success, otherwise the proper error value.
 */
static int fscrypt_sec_init_rng(void)
{
	struct crypto_rng *rng = NULL;
	struct file *filp = NULL;
	char *rng_seed = NULL;
	mm_segment_t fs_type;

	int trial = 10;
	int read = 0;
	int res = 0;

	/* already initialized */
	if (fscrypt_rng)
		return 0;

	rng_seed = kmalloc(FS_RNG_SEED_SIZE, GFP_KERNEL);
	if (!rng_seed) {
		printk(KERN_ERR "fscrypto: failed to allocate rng_seed memory\n");
		res = -ENOMEM;
		goto out;
	}
	memset((void *)rng_seed, 0, FS_RNG_SEED_SIZE);

	// open random device for drbg seed number
	filp = filp_open("/dev/random", O_RDONLY, 0);
	if (IS_ERR(filp)) {
		res = PTR_ERR(filp);
		printk(KERN_ERR "fscrypto: failed to open random(err:%d)\n", res);
		filp = NULL;
		goto out;
	}

	fs_type = get_fs();
	set_fs(KERNEL_DS);
	while (trial > 0) {
		int get_bytes = (int)filp->f_op->read(filp, &(rng_seed[read]),
				FS_RNG_SEED_SIZE-read, &filp->f_pos);
		if (likely(get_bytes > 0))
			read += get_bytes;
		if (likely(read == FS_RNG_SEED_SIZE))
			break;
		trial--;
	}
	set_fs(fs_type);

	if (read != FS_RNG_SEED_SIZE) {
		printk(KERN_ERR "fscrypto: failed to get enough random bytes "
			"(read=%d / request=%d)\n", read, FS_RNG_SEED_SIZE);
		res = -EINVAL;
		goto out;
	}

	// create drbg for random number generation
	rng = crypto_alloc_rng("stdrng", 0, 0);
	if (IS_ERR(rng)) {
		printk(KERN_ERR "fscrypto: failed to allocate rng, "
			"not available (%ld)\n", PTR_ERR(rng));
		res = PTR_ERR(rng);
		rng = NULL;
		goto out;
	}

	// push seed to drbg
	res = crypto_rng_reset(rng, rng_seed, FS_RNG_SEED_SIZE);
	if (res < 0)
		printk(KERN_ERR "fscrypto: rng reset fail (%d)\n", res);
out:
	if (res && rng) {
		crypto_free_rng(rng);
		rng = NULL;
	}
	if (filp)
		filp_close(filp, NULL);
	kfree(rng_seed);

	// save rng on global variable
	fscrypt_rng = rng;
	return res;
}

static inline int generate_fek(char *raw_key)
{
	int res;

	if (likely(fscrypt_rng)) {
again:
		BUG_ON(!fscrypt_rng);
		return crypto_rng_get_bytes(fscrypt_rng,
			raw_key, FS_AES_256_XTS_KEY_SIZE);
	}

	res = fscrypt_sec_init_rng();
	if (!res)
		goto again;

	printk(KERN_ERR "fscrypto: failed to initialize "
			"crypto rng handler again (err:%d)\n", res);
	return res;
}
#else
static void fscrypt_sec_exit_rng(void) { }
static inline int generate_fek(char *raw_key)
{
	get_random_bytes(raw_key, FS_AES_256_XTS_KEY_SIZE);
	return 0;
}
#endif /* CONFIG CRYPTO_FIPS */

static void fscrypt_sec_cbc_complete(struct crypto_async_request *req, int rc)
{
	struct fscrypt_completion_result *ecr = req->data;

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
 * fscrypt_sec_get_key_aes() - Get a key using AES-256-CBC
 * @source_data: Encrypted key and HMAC.
 * @source_key:  Source key to which to apply decryption.
 * @raw_key:     Decrypted key.
 *
 * Return: Zero on success; non-zero otherwise.
 */
int fscrypt_sec_get_key_aes(const char *source_data, const char *source_key, char *raw_key)
{
	DECLARE_FS_COMPLETION_RESULT(ecr);
	struct crypto_skcipher *tfm = NULL;
	struct skcipher_request *req = NULL;
	struct scatterlist src_sg, dst_sg;

	char encrypted_key[FS_AES_256_XTS_KEY_SIZE];
	char fek_hmac[HMAC_SIZE];
	char cbc_key[FS_AES_256_CBC_KEY_SIZE];
	char cbc_iv[FS_XTS_TWEAK_SIZE];
	char digest[HMAC_SIZE];

	int res = 0;

	// Split encrypted key
	memcpy(encrypted_key, source_data, FS_AES_256_XTS_KEY_SIZE);
	memcpy(fek_hmac, source_data+FS_AES_256_XTS_KEY_SIZE, HMAC_SIZE);

	// Split master key
	memcpy(cbc_key, source_key, FS_AES_256_CBC_KEY_SIZE);
	memcpy(cbc_iv, source_key+FS_AES_256_CBC_KEY_SIZE, FS_XTS_TWEAK_SIZE);

	// Decrypt encrypted FEK
	tfm = crypto_alloc_skcipher("cbc(aes)", 0, 0);
	if (IS_ERR(tfm)) {
		res = PTR_ERR(tfm);
		tfm = NULL;
		goto out;
	}

	crypto_skcipher_clear_flags(tfm, ~0);
	crypto_skcipher_set_flags(tfm, CRYPTO_TFM_REQ_WEAK_KEY);
	req = skcipher_request_alloc(tfm, GFP_NOFS);
	if (!req) {
		res = -ENOMEM;
		goto out;
	}
	skcipher_request_set_callback(req,
			CRYPTO_TFM_REQ_MAY_BACKLOG | CRYPTO_TFM_REQ_MAY_SLEEP,
			fscrypt_sec_cbc_complete, &ecr);
	res = crypto_skcipher_setkey(tfm, cbc_key, FS_AES_256_CBC_KEY_SIZE);
	if (res < 0)
		goto out;

	sg_init_one(&src_sg, encrypted_key, FS_AES_256_XTS_KEY_SIZE);
	sg_init_one(&dst_sg, raw_key, FS_AES_256_XTS_KEY_SIZE);
	skcipher_request_set_crypt(req, &src_sg, &dst_sg,
					FS_AES_256_XTS_KEY_SIZE, cbc_iv);
	res = crypto_skcipher_decrypt(req);
	if (res == -EINPROGRESS || res == -EBUSY) {
		wait_for_completion(&ecr.completion);
		res = ecr.res;
		goto out;
	}

	// Make HMAC for FEK with master key
	res = __hmac_sha256(cbc_key, FS_AES_256_CBC_KEY_SIZE,
				encrypted_key, FS_AES_256_XTS_KEY_SIZE, digest);
	if (res)
		goto out;

	// Check HMAC value
	if (memcmp(digest, fek_hmac, HMAC_SIZE)) {
		printk(KERN_ERR "fscrypto: mac mismatch!\n");
		res = -EBADMSG;
	}
out:
	if (req)
		skcipher_request_free(req);
	if (tfm)
		crypto_free_skcipher(tfm);

	memzero_explicit(encrypted_key, FS_AES_256_XTS_KEY_SIZE);
	memzero_explicit(cbc_key, FS_AES_256_CBC_KEY_SIZE);
	memzero_explicit(cbc_iv, FS_XTS_TWEAK_SIZE);
	memzero_explicit(digest, HMAC_SIZE);

	return res;
}

/**
 * fscrypt_sec_set_key_aes() - Generate and save a random key for AES-256.
 * @save_key_data:   Saving space for encrypted key and HMAC.
 * @master_key_desc: keyring descriptor for FEK and HMAC.
 *
 * Return: Zero on success; non-zero otherwise.
 */
int fscrypt_sec_set_key_aes(char *save_key_data, const char *master_key_desc)
{
	char *full_key_desc;
	struct key *keyring_key = NULL;
	struct fscrypt_key *master_key;
	const struct user_key_payload *ukp;
	int full_key_len = FS_KEY_DESC_PREFIX_SIZE +
			(FS_KEY_DESCRIPTOR_SIZE * 2) + 1;

	DECLARE_FS_COMPLETION_RESULT(ecr);
	struct crypto_skcipher *tfm = NULL;
	struct skcipher_request *req = NULL;
	struct scatterlist src_sg, dst_sg;

	char encrypted_key[FS_AES_256_XTS_KEY_SIZE];
	char cbc_key[FS_AES_256_CBC_KEY_SIZE];
	char cbc_iv[FS_XTS_TWEAK_SIZE];
	char raw_key[FS_AES_256_XTS_KEY_SIZE];
	char digest[HMAC_SIZE];

	int res = 0;

	full_key_desc = kmalloc(full_key_len, GFP_NOFS);
	if (!full_key_desc)
		return -ENOMEM;

	// Get Keyring
	memcpy(full_key_desc, FS_KEY_DESC_PREFIX, FS_KEY_DESC_PREFIX_SIZE);
	sprintf(full_key_desc + FS_KEY_DESC_PREFIX_SIZE, "%*phN",
			FS_KEY_DESCRIPTOR_SIZE,	master_key_desc);
	full_key_desc[full_key_len - 1] = '\0';
	keyring_key = request_key(&key_type_logon, full_key_desc, NULL);
	if (IS_ERR(keyring_key)) {
		printk(KERN_WARNING "%s: error get keyring! (%s)\n",
			__func__, full_key_desc);
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
	if (ukp->datalen != sizeof(struct fscrypt_key)) {
		res = -EINVAL;
		goto out_unlock;
	}
	master_key = (struct fscrypt_key *)ukp->data;

	BUILD_BUG_ON(FS_ESTIMATED_NONCE_SIZE != FS_KEY_DERIVATION_NONCE_SIZE);

	if (master_key->size != FS_AES_256_XTS_KEY_SIZE) {
		printk(KERN_WARNING "%s: key size incorrect (%d)\n",
			__func__, master_key->size);
		res = -ENOKEY;
		goto out_unlock;
	}

	// Split master key
	memcpy(cbc_key, master_key->raw, FS_AES_256_CBC_KEY_SIZE);
	memcpy(cbc_iv, (master_key->raw)+FS_AES_256_CBC_KEY_SIZE, FS_XTS_TWEAK_SIZE);

	// Generate FEK
	res = generate_fek(raw_key);
	if (res < 0) {
		printk(KERN_ERR "fscrypto: failed to generate FEK (%d)\n", res);
		goto out_unlock;
	}

	// Encrypt FEK
	tfm = crypto_alloc_skcipher("cbc(aes)", 0, 0);
	if (IS_ERR(tfm)) {
		res = PTR_ERR(tfm);
		tfm = NULL;
		goto out_unlock;
	}

	crypto_skcipher_clear_flags(tfm, ~0);
	crypto_skcipher_set_flags(tfm, CRYPTO_TFM_REQ_WEAK_KEY);
	req = skcipher_request_alloc(tfm, GFP_NOFS);
	if (!req) {
		res = -ENOMEM;
		goto out_unlock;
	}
	skcipher_request_set_callback(req,
			CRYPTO_TFM_REQ_MAY_BACKLOG | CRYPTO_TFM_REQ_MAY_SLEEP,
			fscrypt_sec_cbc_complete, &ecr);
	res = crypto_skcipher_setkey(tfm, cbc_key, FS_AES_256_CBC_KEY_SIZE);
	if (res < 0) {
		printk(KERN_ERR "fscrypto: can't set key for cbc\n");
		goto out_unlock;
	}

	sg_init_one(&src_sg, raw_key, FS_AES_256_XTS_KEY_SIZE);
	sg_init_one(&dst_sg, encrypted_key, FS_AES_256_XTS_KEY_SIZE);
	skcipher_request_set_crypt(req, &src_sg, &dst_sg,
			FS_AES_256_XTS_KEY_SIZE, cbc_iv);
	res = crypto_skcipher_encrypt(req);
	if (res == -EINPROGRESS || res == -EBUSY) {
		wait_for_completion(&ecr.completion);
		res = ecr.res;
		goto out_unlock;
	}

	// Make HMAC for FEK with master key
	res = __hmac_sha256(cbc_key, FS_AES_256_CBC_KEY_SIZE,
				encrypted_key, FS_AES_256_XTS_KEY_SIZE, digest);
	if (res) {
		printk(KERN_ERR "fscrypto: can't calculate hmac (err:%d)\n", res);
		goto out_unlock;
	}

	// Save encrypted FEK and HMAC value
	memcpy(save_key_data, encrypted_key, FS_AES_256_XTS_KEY_SIZE);
	memcpy(save_key_data+FS_AES_256_XTS_KEY_SIZE, digest, HMAC_SIZE);

out_unlock:
	up_read(&keyring_key->sem);
out:
	if (req)
		skcipher_request_free(req);
	if (tfm)
		crypto_free_skcipher(tfm);
	if (keyring_key)
		key_put(keyring_key);

	memzero_explicit(encrypted_key, FS_AES_256_XTS_KEY_SIZE);
	memzero_explicit(raw_key, FS_AES_256_XTS_KEY_SIZE);
	memzero_explicit(cbc_key, FS_AES_256_CBC_KEY_SIZE);
	memzero_explicit(cbc_iv, FS_XTS_TWEAK_SIZE);
	memzero_explicit(digest, HMAC_SIZE);

	return res;
}

int __init fscrypt_sec_crypto_init(void)
{
/* It's able to block fscrypto-fs module initialization. is there alternative? */
#if 0
	int res;

	res = fscrypt_sec_init_rng();
	if (res) {
		printk(KERN_WARNING "fscrypto: failed to initialize "
			"crypto rng handler (err:%d), postponed\n", res);
		/* return SUCCESS, although init_rng() failed */
	}
#endif
	return 0;
}

void __exit fscrypt_sec_crypto_exit(void)
{
	fscrypt_sec_exit_rng();
}
