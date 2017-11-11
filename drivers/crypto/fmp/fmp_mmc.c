/*
 * Exynos FMP MMC driver
 *
 * Copyright (C) 2015 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/pagemap.h>
#include <linux/mmc/dw_mmc.h>
#include <linux/smc.h>

#include "dw_mmc-exynos.h"
#if defined(CONFIG_MMC_DW_FMP_ECRYPT_FS) || defined(CONFIG_MMC_DW_FMP_EXT4CRYPT_FS)
#include "fmp_derive_iv.h"
#endif

extern volatile unsigned int disk_key_flag;
extern spinlock_t disk_key_lock;

#define byte2word(b0, b1, b2, b3) 	\
		((unsigned int)(b0) << 24) | ((unsigned int)(b1) << 16) | ((unsigned int)(b2) << 8) | (b3)
#define word_in(x, c)           byte2word(((unsigned char *)(x) + 4 * (c))[0], ((unsigned char *)(x) + 4 * (c))[1], \
					((unsigned char *)(x) + 4 * (c))[2], ((unsigned char *)(x) + 4 * (c))[3])

int fmp_mmc_map_sg(struct dw_mci *host, struct idmac_desc_64addr *desc, int idx,
			uint32_t enc_mode, uint32_t sector, struct mmc_data *data)
{
#if defined(CONFIG_MMC_DW_FMP_DM_CRYPT)
	if (enc_mode & MMC_DISK_ENC_MODE) {
		int ret;

		/* disk algorithm selector  */
		IDMAC_SET_DAS(desc, AES_XTS);
		desc->des2 |= IDMAC_DES2_DKL;

		/* Disk IV */
		desc->des28 = 0;
		desc->des29 = 0;
		desc->des30 = 0;
		desc->des31 = htonl(sector);

		/* Disk Enc Key, Tweak Key */
		if (disk_key_flag) {
			/* Disk Enc Key, Tweak Key */
			ret = exynos_smc(SMC_CMD_FMP_DISKENC, FMP_DISKKEY_SET, ID_FMP_UFS_MMC, 0);
			if (ret) {
				printk(KERN_ERR "Failed to smc call for FMP key setting: %x\n", ret);
				return ret;
			}
			spin_lock(&disk_key_lock);
			disk_key_flag = 0;
			spin_unlock(&disk_key_lock);
		}
	}
#endif
#if defined(CONFIG_MMC_DW_FMP_ECRYPT_FS) || defined(CONFIG_MMC_DW_FMP_EXT4CRYPT_FS)
	if (enc_mode & MMC_FILE_ENC_MODE) {
		int ret;
		unsigned int aes_alg = 0;
		unsigned int j;
		unsigned int last_index = 0;
		unsigned long last_inode = 0;
#ifdef CONFIG_CRYPTO_FIPS
		char extent_iv[SHA256_HASH_SIZE];
#else
		char extent_iv[MD5_DIGEST_SIZE];
#endif
		loff_t index;

		/* File algorithm selector*/
		if (!strncmp(sg_page(&data->sg[idx])->mapping->alg, "aes", sizeof("aes")))
			aes_alg = AES_CBC;
		else if (!strncmp(sg_page(&data->sg[idx])->mapping->alg, "aesxts", sizeof("aesxts")))
			aes_alg = AES_XTS;
		else {
			printk(KERN_ERR "Invalid file algorithm: %s\n", sg_page(&data->sg[idx])->mapping->alg);
			return -1;
		}

		IDMAC_SET_FAS(desc, aes_alg);

		/* File enc key size */
		switch (sg_page(&data->sg[idx])->mapping->key_length) {
		case 16:
			desc->des2 &= ~IDMAC_DES2_FKL;
			break;
		case 32:
		case 64:
			desc->des2 |= IDMAC_DES2_FKL;
			break;
		default:
			printk(KERN_ERR "Invalid file key length: %lx\n", sg_page(&data->sg[idx])->mapping->key_length);
			return -1;
		}

		index = sg_page(&data->sg[idx])->index;
		if ((last_index != index) || (last_inode != sg_page(&data->sg[idx])->mapping->host->i_ino)) {
			index = index - sg_page(&data->sg[idx])->mapping->sensitive_data_index;
			ret = file_enc_derive_iv(sg_page(&data->sg[idx])->mapping, index, extent_iv);
			if (ret) {
				printk(KERN_ERR "Error attemping to derive IV\n");
				return ret;
			}
		}
		last_index = sg_page(&data->sg[idx])->index;
		last_inode = sg_page(&data->sg[idx])->mapping->host->i_ino;

		/* File IV */
		desc->des8 = word_in(extent_iv, 3);
		desc->des9 = word_in(extent_iv, 2);
		desc->des10 = word_in(extent_iv, 1);
		desc->des11 = word_in(extent_iv, 0);

		/* File Enc key */
		for (j = 0; j < sg_page(&data->sg[idx])->mapping->key_length >> 2; j++)
			*(&(desc->des12) + j) =
				word_in(sg_page(&data->sg[idx])->mapping->key, (sg_page(&data->sg[idx])->mapping->key_length >> 2) - (j + 1));
	}
#endif
	return 0;
}
