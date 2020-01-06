/*
 * linux/drivers/video/fbdev/exynos/panel/s6e3ha8/s6e3ha8_star2_panel_poc.h
 *
 * Header file for POC Driver
 *
 * Copyright (c) 2016 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __S6E3HA8_STAR2_PANEL_POC_H__
#define __S6E3HA8_STAR2_PANEL_POC_H__

#include "../panel.h"
#include "../panel_poc.h"

#define STAR2_EXEC_USEC	(20)
#define STAR2_EXEC_DATA_USEC	(2)
#define STAR2_QD_DONE_MDELAY		(30)
#define STAR2_RD_DONE_UDELAY		(250)
#define STAR2_WR_DONE_UDELAY		(4000)
#ifdef CONFIG_SUPPORT_POC_FLASH
#define STAR2_ER_QD_DONE_MDELAY		(10)
#define STAR2_ER_DONE_MDELAY		(400)
#define STAR2_ER_4K_DONE_MDELAY        (400)
#define STAR2_ER_32K_DONE_MDELAY       (800)
#define STAR2_ER_64K_DONE_MDELAY       (1000)
#endif

#define STAR2_POC_IMG_ADDR	(0)
#define STAR2_POC_IMG_SIZE	(536706)

#ifdef CONFIG_SUPPORT_DIM_FLASH
#define STAR2_POC_DIM_DATA_ADDR	(0xA0000)
#define STAR2_POC_DIM_DATA_SIZE (S6E3HA8_DIM_FLASH_DATA_SIZE)
#define STAR2_POC_DIM_CHECKSUM_ADDR	(STAR2_POC_DIM_DATA_ADDR + S6E3HA8_DIM_FLASH_CHECKSUM_OFS)
#define STAR2_POC_DIM_CHECKSUM_SIZE (S6E3HA8_DIM_FLASH_CHECKSUM_LEN)
#define STAR2_POC_DIM_MAGICNUM_ADDR	(STAR2_POC_DIM_DATA_ADDR + S6E3HA8_DIM_FLASH_MAGICNUM_OFS)
#define STAR2_POC_DIM_MAGICNUM_SIZE (S6E3HA8_DIM_FLASH_MAGICNUM_LEN)
#define STAR2_POC_DIM_TOTAL_SIZE (S6E3HA8_DIM_FLASH_TOTAL_SIZE)

#define STAR2_POC_MTP_DATA_ADDR	(0xA2000)
#define STAR2_POC_MTP_DATA_SIZE (S6E3HA8_DIM_FLASH_MTP_DATA_SIZE)
#define STAR2_POC_MTP_CHECKSUM_ADDR	(STAR2_POC_MTP_DATA_ADDR + S6E3HA8_DIM_FLASH_MTP_CHECKSUM_OFS)
#define STAR2_POC_MTP_CHECKSUM_SIZE (S6E3HA8_DIM_FLASH_MTP_CHECKSUM_LEN)
#define STAR2_POC_MTP_TOTAL_SIZE (S6E3HA8_DIM_FLASH_MTP_TOTAL_SIZE)
#endif

#define STAR2_POC_MCD_ADDR	(0xB8000)
#define STAR2_POC_MCD_SIZE (S6E3HA8_FLASH_MCD_LEN)

/* ===================================================================================== */
/* ============================== [S6E3HA8 MAPPING TABLE] ============================== */
/* ===================================================================================== */
static struct maptbl star2_poc_maptbl[] = {
	[POC_WR_ADDR_MAPTBL] = DEFINE_0D_MAPTBL(star2_poc_wr_addr_table, init_common_table, NULL, copy_poc_wr_addr_maptbl),
	[POC_RD_ADDR_MAPTBL] = DEFINE_0D_MAPTBL(star2_poc_rd_addr_table, init_common_table, NULL, copy_poc_rd_addr_maptbl),
	[POC_WR_DATA_MAPTBL] = DEFINE_0D_MAPTBL(star2_poc_wr_data_table, init_common_table, NULL, copy_poc_wr_data_maptbl),
#ifdef CONFIG_SUPPORT_POC_FLASH
	[POC_ER_ADDR_MAPTBL] = DEFINE_0D_MAPTBL(star2_poc_er_addr_table, init_common_table, NULL, copy_poc_er_addr_maptbl),
#endif
};

/* ===================================================================================== */
/* ============================== [S6E3HA8 COMMAND TABLE] ============================== */
/* ===================================================================================== */
static u8 STAR2_KEY2_ENABLE[] = { 0xF0, 0x5A, 0x5A };
static u8 STAR2_KEY2_DISABLE[] = { 0xF0, 0xA5, 0xA5 };
static u8 STAR2_POC_KEY_ENABLE[] = { 0xF1, 0xF1, 0xA2 };
static u8 STAR2_POC_KEY_DISABLE[] = { 0xF1, 0xA5, 0xA5 };
static u8 STAR2_POC_PGM_ENABLE[] = { 0xC0, 0x02 };
static u8 STAR2_POC_PGM_DISABLE[] = { 0xC0, 0x00 };
static u8 STAR2_POC_EXEC[] = { 0xC0, 0x03 };

static u8 STAR2_POC_WR_ENABLE[] = { 0xC1, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04 };
static u8 STAR2_POC_WR_QD[] = { 0xC1, 0x00, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x10, 0x04 };
static u8 STAR2_POC_RD_QD[] = { 0xC1, 0x00, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x10 };

static u8 STAR2_POC_WR_STT[] = { 0xC1, 0x00, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x04 };
static u8 STAR2_POC_WR_END[] = { 0xC1, 0x00, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x04 };
static u8 STAR2_POC_RD_STT[] = { 0xC1, 0x00, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01 };
static u8 STAR2_POC_WR_DAT[] = { 0xC1, 0x00 };

#ifdef CONFIG_SUPPORT_POC_FLASH
static u8 STAR2_POC_ER_ENABLE[] = { 0xC1, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04 };
static u8 STAR2_POC_ER_STT[] = { 0xC1, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x04 };
static u8 STAR2_POC_ER_4K_STT[] = { 0xC1, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x04 };
static u8 STAR2_POC_ER_32K_STT[] = { 0xC1, 0x00, 0x52, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x04 };
static u8 STAR2_POC_ER_64K_STT[] = { 0xC1, 0x00, 0xD8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x04 };
#endif

static DEFINE_STATIC_PACKET(star2_level2_key_enable, DSI_PKT_TYPE_WR, STAR2_KEY2_ENABLE, 0);
static DEFINE_STATIC_PACKET(star2_level2_key_disable, DSI_PKT_TYPE_WR, STAR2_KEY2_DISABLE, 0);
static DEFINE_STATIC_PACKET(star2_poc_key_enable, DSI_PKT_TYPE_WR, STAR2_POC_KEY_ENABLE, 0);
static DEFINE_STATIC_PACKET(star2_poc_key_disable, DSI_PKT_TYPE_WR, STAR2_POC_KEY_DISABLE, 0);
static DEFINE_STATIC_PACKET(star2_poc_pgm_enable, DSI_PKT_TYPE_WR, STAR2_POC_PGM_ENABLE, 0);
static DEFINE_STATIC_PACKET(star2_poc_pgm_disable, DSI_PKT_TYPE_WR, STAR2_POC_PGM_DISABLE, 0);
#ifdef CONFIG_SUPPORT_POC_FLASH
static DEFINE_STATIC_PACKET(star2_poc_er_enable, DSI_PKT_TYPE_WR, STAR2_POC_ER_ENABLE, 0);
static DEFINE_PKTUI(star2_poc_er_stt, &star2_poc_maptbl[POC_ER_ADDR_MAPTBL], 3);
static DEFINE_VARIABLE_PACKET(star2_poc_er_stt, DSI_PKT_TYPE_WR, STAR2_POC_ER_STT, 0);
static DEFINE_PKTUI(star2_poc_er_4k_stt, &star2_poc_maptbl[POC_ER_ADDR_MAPTBL], 3);
static DEFINE_VARIABLE_PACKET(star2_poc_er_4k_stt, DSI_PKT_TYPE_WR, STAR2_POC_ER_4K_STT, 0);
static DEFINE_PKTUI(star2_poc_er_32k_stt, &star2_poc_maptbl[POC_ER_ADDR_MAPTBL], 3);
static DEFINE_VARIABLE_PACKET(star2_poc_er_32k_stt, DSI_PKT_TYPE_WR, STAR2_POC_ER_32K_STT, 0);
static DEFINE_PKTUI(star2_poc_er_64k_stt, &star2_poc_maptbl[POC_ER_ADDR_MAPTBL], 3);
static DEFINE_VARIABLE_PACKET(star2_poc_er_64k_stt, DSI_PKT_TYPE_WR, STAR2_POC_ER_64K_STT, 0);
#endif
static DEFINE_STATIC_PACKET(star2_poc_exec, DSI_PKT_TYPE_WR, STAR2_POC_EXEC, 0);
static DEFINE_STATIC_PACKET(star2_poc_wr_enable, DSI_PKT_TYPE_WR, STAR2_POC_WR_ENABLE, 0);
static DEFINE_STATIC_PACKET(star2_poc_wr_qd, DSI_PKT_TYPE_WR, STAR2_POC_WR_QD, 0);
static DEFINE_STATIC_PACKET(star2_poc_rd_qd, DSI_PKT_TYPE_WR, STAR2_POC_RD_QD, 0);
static DEFINE_PKTUI(star2_poc_rd_stt, &star2_poc_maptbl[POC_RD_ADDR_MAPTBL], 6);
static DEFINE_VARIABLE_PACKET(star2_poc_rd_stt, DSI_PKT_TYPE_WR, STAR2_POC_RD_STT, 0);
static DEFINE_PKTUI(star2_poc_wr_stt, &star2_poc_maptbl[POC_WR_ADDR_MAPTBL], 6);
static DEFINE_VARIABLE_PACKET(star2_poc_wr_stt, DSI_PKT_TYPE_WR, STAR2_POC_WR_STT, 0);
static DEFINE_STATIC_PACKET(star2_poc_wr_end, DSI_PKT_TYPE_WR, STAR2_POC_WR_END, 0);
static DEFINE_PKTUI(star2_poc_wr_dat, &star2_poc_maptbl[POC_WR_DATA_MAPTBL], 1);
static DEFINE_VARIABLE_PACKET(star2_poc_wr_dat, DSI_PKT_TYPE_WR, STAR2_POC_WR_DAT, 0);

static DEFINE_PANEL_UDELAY_NO_SLEEP(star2_poc_wait_exec, STAR2_EXEC_USEC);
static DEFINE_PANEL_UDELAY_NO_SLEEP(star2_poc_wait_exec_data, STAR2_EXEC_DATA_USEC);
static DEFINE_PANEL_UDELAY_NO_SLEEP(star2_poc_wait_rd_done, STAR2_RD_DONE_UDELAY);
static DEFINE_PANEL_UDELAY_NO_SLEEP(star2_poc_wait_wr_done, STAR2_WR_DONE_UDELAY);
static DEFINE_PANEL_MDELAY(star2_poc_wait_qd_status, STAR2_QD_DONE_MDELAY);
#ifdef CONFIG_SUPPORT_POC_FLASH
static DEFINE_PANEL_MDELAY(star2_poc_wait_er_qd_status, STAR2_ER_QD_DONE_MDELAY);
static DEFINE_PANEL_MDELAY(star2_poc_wait_er_done, STAR2_ER_DONE_MDELAY);
static DEFINE_PANEL_MDELAY(star2_poc_wait_er_4k_done, STAR2_ER_4K_DONE_MDELAY);
static DEFINE_PANEL_MDELAY(star2_poc_wait_er_32k_done, STAR2_ER_32K_DONE_MDELAY);
static DEFINE_PANEL_MDELAY(star2_poc_wait_er_64k_done, STAR2_ER_64K_DONE_MDELAY);
#endif

#ifdef CONFIG_SUPPORT_POC_FLASH
static void *star2_poc_erase_enter_cmdtbl[] = {
	&PKTINFO(star2_level2_key_enable),
	&PKTINFO(star2_poc_key_enable),
	&PKTINFO(star2_poc_pgm_enable),
	&PKTINFO(star2_poc_er_enable),
	&PKTINFO(star2_poc_exec),
	&DLYINFO(star2_poc_wait_exec),
	&PKTINFO(star2_poc_wr_qd),
	&PKTINFO(star2_poc_exec),
	&DLYINFO(star2_poc_wait_er_qd_status),
};

static void *star2_poc_erase_cmdtbl[] = {
	&PKTINFO(star2_poc_er_enable),
	&PKTINFO(star2_poc_exec),
	&DLYINFO(star2_poc_wait_exec),
	&PKTINFO(star2_poc_er_stt),
	&PKTINFO(star2_poc_exec),
	&DLYINFO(star2_poc_wait_er_done),
};

static void *star2_poc_erase_4k_cmdtbl[] = {
	&PKTINFO(star2_poc_er_enable),
	&PKTINFO(star2_poc_exec),
	&DLYINFO(star2_poc_wait_exec),
	&PKTINFO(star2_poc_er_4k_stt),
	&PKTINFO(star2_poc_exec),
	&DLYINFO(star2_poc_wait_er_4k_done),
};

static void *star2_poc_erase_32k_cmdtbl[] = {
	&PKTINFO(star2_poc_er_enable),
	&PKTINFO(star2_poc_exec),
	&DLYINFO(star2_poc_wait_exec),
	&PKTINFO(star2_poc_er_32k_stt),
	&PKTINFO(star2_poc_exec),
	&DLYINFO(star2_poc_wait_er_32k_done),
};

static void *star2_poc_erase_64k_cmdtbl[] = {
	&PKTINFO(star2_poc_er_enable),
	&PKTINFO(star2_poc_exec),
	&DLYINFO(star2_poc_wait_exec),
	&PKTINFO(star2_poc_er_64k_stt),
	&PKTINFO(star2_poc_exec),
	&DLYINFO(star2_poc_wait_er_64k_done),
};

static void *star2_poc_erase_exit_cmdtbl[] = {
	&PKTINFO(star2_poc_pgm_disable),
	&PKTINFO(star2_poc_key_disable),
	&PKTINFO(star2_level2_key_disable),
};
#endif

static void *star2_poc_wr_enter_cmdtbl[] = {
	&PKTINFO(star2_level2_key_enable),
	&PKTINFO(star2_poc_key_enable),
	&PKTINFO(star2_poc_pgm_enable),
	&PKTINFO(star2_poc_wr_enable),
	&PKTINFO(star2_poc_exec),
	&DLYINFO(star2_poc_wait_exec),
	&PKTINFO(star2_poc_wr_qd),
	&PKTINFO(star2_poc_exec),
	&DLYINFO(star2_poc_wait_qd_status),
};

static void *star2_poc_wr_stt_cmdtbl[] = {
	&PKTINFO(star2_poc_wr_enable),
	&PKTINFO(star2_poc_exec),
	&DLYINFO(star2_poc_wait_exec),
	/* address & continue write begin */
	&PKTINFO(star2_poc_wr_stt),
};

static void *star2_poc_wr_dat_stt_end_cmdtbl[] = {
	&PKTINFO(star2_poc_wr_dat),
	&PKTINFO(star2_poc_exec),
	&DLYINFO(star2_poc_wait_exec),
};

static void *star2_poc_wr_dat_cmdtbl[] = {
	&PKTINFO(star2_poc_wr_dat),
	&PKTINFO(star2_poc_exec),
	&DLYINFO(star2_poc_wait_exec_data),
};

static void *star2_poc_wr_end_cmdtbl[] = {
	/* continue write end */
	&PKTINFO(star2_poc_wr_end),
	&DLYINFO(star2_poc_wait_wr_done),
};

static void *star2_poc_wr_exit_cmdtbl[] = {
	&PKTINFO(star2_poc_pgm_disable),
	&PKTINFO(star2_poc_key_disable),
	&PKTINFO(star2_level2_key_disable),
};

static void *star2_poc_rd_enter_cmdtbl[] = {
	&PKTINFO(star2_level2_key_enable),
	&PKTINFO(star2_poc_key_enable),
	&PKTINFO(star2_poc_pgm_enable),
	&PKTINFO(star2_poc_wr_enable),
	&PKTINFO(star2_poc_exec),
	&DLYINFO(star2_poc_wait_exec),
	&PKTINFO(star2_poc_rd_qd),
	&PKTINFO(star2_poc_exec),
	&DLYINFO(star2_poc_wait_qd_status),
};

static void *star2_poc_rd_dat_cmdtbl[] = {
	&PKTINFO(star2_poc_rd_stt),
	&PKTINFO(star2_poc_exec),
	&DLYINFO(star2_poc_wait_rd_done),
	&s6e3ha8_restbl[RES_POC_DATA],
};

static void *star2_poc_rd_exit_cmdtbl[] = {
	&PKTINFO(star2_poc_pgm_disable),
	&PKTINFO(star2_poc_key_disable),
	&PKTINFO(star2_level2_key_disable),
};

static struct seqinfo star2_poc_seqtbl[MAX_POC_SEQ] = {
#ifdef CONFIG_SUPPORT_POC_FLASH
	/* poc erase */
	[POC_ERASE_ENTER_SEQ] = SEQINFO_INIT("poc-erase-enter-seq", star2_poc_erase_enter_cmdtbl),
	[POC_ERASE_SEQ] = SEQINFO_INIT("poc-erase-seq", star2_poc_erase_cmdtbl),
	[POC_ERASE_4K_SEQ] = SEQINFO_INIT("poc-erase-4k-seq", star2_poc_erase_4k_cmdtbl),
	[POC_ERASE_32K_SEQ] = SEQINFO_INIT("poc-erase-4k-seq", star2_poc_erase_32k_cmdtbl),
	[POC_ERASE_64K_SEQ] = SEQINFO_INIT("poc-erase-32k-seq", star2_poc_erase_64k_cmdtbl),
	[POC_ERASE_EXIT_SEQ] = SEQINFO_INIT("poc-erase-exit-seq", star2_poc_erase_exit_cmdtbl),
#endif

	/* poc write */
	[POC_WRITE_ENTER_SEQ] = SEQINFO_INIT("poc-wr-enter-seq", star2_poc_wr_enter_cmdtbl),
	[POC_WRITE_STT_SEQ] = SEQINFO_INIT("poc-wr-stt-seq", star2_poc_wr_stt_cmdtbl),
	[POC_WRITE_DAT_STT_END_SEQ] = SEQINFO_INIT("poc-wr-dat-stt-end-seq", star2_poc_wr_dat_stt_end_cmdtbl),
	[POC_WRITE_DAT_SEQ] = SEQINFO_INIT("poc-wr-dat-seq", star2_poc_wr_dat_cmdtbl),
	[POC_WRITE_END_SEQ] = SEQINFO_INIT("poc-wr-end-seq", star2_poc_wr_end_cmdtbl),
	[POC_WRITE_EXIT_SEQ] = SEQINFO_INIT("poc-wr-exit-seq", star2_poc_wr_exit_cmdtbl),

	/* poc read */
	[POC_READ_ENTER_SEQ] = SEQINFO_INIT("poc-rd-enter-seq", star2_poc_rd_enter_cmdtbl),
	[POC_READ_DAT_SEQ] = SEQINFO_INIT("poc-rd-dat-seq", star2_poc_rd_dat_cmdtbl),
	[POC_READ_EXIT_SEQ] = SEQINFO_INIT("poc-rd-exit-seq", star2_poc_rd_exit_cmdtbl),
};

/* partition consists of DATA, CHECKSUM and MAGICNUM */
static struct poc_partition star2_poc_partition[] = {
	[POC_IMG_PARTITION] = {
		.name = "image",
		.addr = STAR2_POC_IMG_ADDR,
		.size = STAR2_POC_IMG_SIZE,
		.need_preload = false
	},
#ifdef CONFIG_SUPPORT_DIM_FLASH
	[POC_DIM_PARTITION] = {
		.name = "dimming",
		.addr = STAR2_POC_DIM_DATA_ADDR,
		.size = STAR2_POC_DIM_TOTAL_SIZE,
		.data_addr = STAR2_POC_DIM_DATA_ADDR,
		.data_size = STAR2_POC_DIM_DATA_SIZE,
		.checksum_addr = STAR2_POC_DIM_CHECKSUM_ADDR,
		.checksum_size = STAR2_POC_DIM_CHECKSUM_SIZE,
		.magicnum_addr = STAR2_POC_DIM_MAGICNUM_ADDR,
		.magicnum_size = STAR2_POC_DIM_MAGICNUM_SIZE,
		.need_preload = false,
		.magicnum = 1,
	},
	[POC_MTP_PARTITION] = {
		.name = "mtp",
		.addr = STAR2_POC_MTP_DATA_ADDR,
		.size = STAR2_POC_MTP_TOTAL_SIZE,
		.data_addr = STAR2_POC_MTP_DATA_ADDR,
		.data_size = STAR2_POC_MTP_DATA_SIZE,
		.checksum_addr = STAR2_POC_MTP_CHECKSUM_ADDR,
		.checksum_size = STAR2_POC_MTP_CHECKSUM_SIZE,
		.magicnum_addr = 0,
		.magicnum_size = 0,
		.need_preload = false,
	},
#endif
	[POC_MCD_PARTITION] = {
		.name = "mcd",
		.addr = STAR2_POC_MCD_ADDR,
		.size = STAR2_POC_MCD_SIZE,
		.need_preload = false
	},
};

static struct panel_poc_data s6e3ha8_star2_poc_data = {
	.version = 2,
	.seqtbl = star2_poc_seqtbl,
	.nr_seqtbl = ARRAY_SIZE(star2_poc_seqtbl),
	.maptbl = star2_poc_maptbl,
	.nr_maptbl = ARRAY_SIZE(star2_poc_maptbl),
	.partition = star2_poc_partition,
	.nr_partition = ARRAY_SIZE(star2_poc_partition),
};
#endif /* __S6E3HA8_STAR2_PANEL_POC_H__ */

