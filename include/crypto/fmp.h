/*
 * Copyright (C) 2016 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _EXYNOS_FMP_H_
#define _EXYNOS_FMP_H_

#include <linux/platform_device.h>
#include <linux/miscdevice.h>

#define FMP_KEY_SIZE_16		16
#define FMP_KEY_SIZE_32		32
#define FMP_IV_SIZE_16		16

#define FMP_CBC_MAX_KEY_SIZE	FMP_KEY_SIZE_16
#define FMP_XTS_MAX_KEY_SIZE	((FMP_KEY_SIZE_32) * (2))
#define FMP_MAX_KEY_SIZE	FMP_XTS_MAX_KEY_SIZE

#define FMP_HOST_TYPE_NAME_LEN	8
#define FMP_BLOCK_TYPE_NAME_LEN	8

#define FMP_SECTOR_SIZE	0x1000
#define FMP_MIN_SECTOR_SIZE	0x200
#define NUM_SECTOR_UNIT	((FMP_SECTOR_SIZE)/(FMP_MIN_SECTOR_SIZE))

#define MAX_AES_XTS_TRANSFER_SIZE	0x1000

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#if defined(CONFIG_MMC_DW_EXYNOS_FMP)
#define TRANS_DESC_LEN_MULTIPLIER	4
#else
#define TRANS_DESC_LEN_MULTIPLIER	1
#endif

enum fmp_id {
	FMP_EMBEDDED = 0,
	FMP_UFSCARD = 1,
	FMP_SDCARD = 2,
};

enum fmp_crypto_algo_mode {
	EXYNOS_FMP_BYPASS_MODE = 0,
	EXYNOS_FMP_ALGO_MODE_AES_CBC = 1,
	EXYNOS_FMP_ALGO_MODE_AES_XTS = 2,
};

enum fmp_crypto_key_size {
	EXYNOS_FMP_KEY_SIZE_16 = 16,
	EXYNOS_FMP_KEY_SIZE_32 = 32,
};

enum fmp_crypto_enc_mode {
	EXYNOS_FMP_FILE_ENC = 0,
	EXYNOS_FMP_DISK_ENC = 1,
};

enum fmp_disk_key_status {
	KEY_STORED = 0,
	KEY_SET = 1,
	KEY_CLEAR = 2,
	KEY_ERROR = -1,
};

struct fmp_crypto_setting {
	enum fmp_crypto_algo_mode algo_mode;
	enum fmp_crypto_enc_mode enc_mode;
	enum fmp_crypto_key_size key_size;
	uint32_t index;
	sector_t sector;
	unsigned char key[FMP_MAX_KEY_SIZE];
	uint8_t iv[FMP_IV_SIZE_16];
};

struct fmp_table_setting {
	__le32 des0;		/* des0 */
#define GET_CMDQ_LENGTH(d) \
	(((d)->des0 & 0xffff0000) >> 16)
	__le32 des1;		/* des1 */
	__le32 des2;		/* des2 */
	__le32 des3;		/* des3 */
/* Legacy Operation */
#define FKL BIT(26)
#define DKL BIT(27)
#define SET_KEYLEN(d, v) ((d)->des3 |= (uint32_t)v)
#define SET_FAS(d, v) \
	((d)->des3 = ((d)->des3 & 0xcfffffff) | v << 28)
#define SET_DAS(d, v) \
	((d)->des3 = ((d)->des3 & 0x3fffffff) | v << 30)
#define GET_FAS(d)	((d)->des3 & 0x30000000)
#define GET_DAS(d)	((d)->des3 & 0xc0000000)
#define GET_LENGTH(d) \
	((d)->des3 & 0x3ffffff)
/* CMDQ Operation */
#define FKL_CMDQ BIT(0)
#define DKL_CMDQ BIT(1)
#define SET_CMDQ_KEYLEN(d, v) ((d)->des3 |= (uint32_t)v)
#define SET_CMDQ_FAS(d, v) \
	((d)->des3 = ((d)->des3 & 0xfffffff3) | v << 2)
#define SET_CMDQ_DAS(d, v) \
	((d)->des3 = ((d)->des3 & 0xffffffcf) | v << 4)
#define GET_CMDQ_FAS(d)	((d)->des3 & 0x0000000c)
#define GET_CMDQ_DAS(d)	((d)->des3 & 0x00000030)
	__le32 file_iv0;	/* des4 */
	__le32 file_iv1;	/* des5 */
	__le32 file_iv2;	/* des6 */
	__le32 file_iv3;	/* des7 */
	__le32 file_enckey0;	/* des8 */
	__le32 file_enckey1;	/* des9 */
	__le32 file_enckey2;	/* des10 */
	__le32 file_enckey3;	/* des11 */
	__le32 file_enckey4;	/* des12 */
	__le32 file_enckey5;	/* des13 */
	__le32 file_enckey6;	/* des14 */
	__le32 file_enckey7;	/* des15 */
	__le32 file_twkey0;	/* des16 */
	__le32 file_twkey1;	/* des17 */
	__le32 file_twkey2;	/* des18 */
	__le32 file_twkey3;	/* des19 */
	__le32 file_twkey4;	/* des20 */
	__le32 file_twkey5;	/* des21 */
	__le32 file_twkey6;	/* des22 */
	__le32 file_twkey7;	/* des23 */
	__le32 disk_iv0;	/* des24 */
	__le32 disk_iv1;	/* des25 */
	__le32 disk_iv2;	/* des26 */
	__le32 disk_iv3;	/* des27 */
	__le32 reserved0;	/* des28 */
	__le32 reserved1;	/* des29 */
	__le32 reserved2;	/* des30 */
	__le32 reserved3;	/* des31 */
};

struct fmp_data_setting {
	struct fmp_crypto_setting disk;
	struct fmp_crypto_setting file;
	struct fmp_table_setting *table;
	struct address_space *mapping;
	bool cmdq_enabled;
};

struct exynos_fmp_variant_ops {
	const char *name;
	int	(*config)(struct platform_device *, struct fmp_data_setting *);
	int	(*set_disk_key)(struct platform_device *, struct fmp_data_setting *);
	int	(*clear_disk_key)(struct platform_device *);
	int	(*clear)(struct platform_device *, struct fmp_data_setting *);
};

struct fmp_fips_data {
	char block_type[FMP_BLOCK_TYPE_NAME_LEN];
	struct block_device *bdev;
	sector_t sector;
	dev_t devt;
	struct fmp_crypto_setting crypto;
	uint32_t test_block_offset;
};

struct fips_result {
	bool overall;
	bool aes_xts;
	bool aes_cbc;
	bool sha256;
	bool hmac;
	bool integrity;
};

struct exynos_fmp {
	struct list_head list;
	int id;
	int command;
	struct device *dev;

	char host_type[FMP_HOST_TYPE_NAME_LEN];
	struct miscdevice miscdev;
	struct fmp_fips_data *fips_data;
	struct platform_device *host_pdev;

	int test_mode;
	struct buffer_head *test_bh;
	struct fips_result result;

	int status_disk_key;
	void *test_vops;
};

#endif /* _EXYNOS_FMP_H_ */
