/*
 * linux/drivers/video/fbdev/exynos/panel/s6e3ha6/s6e3ha6.h
 *
 * Header file for S6E3HA6 Dimming Driver
 *
 * Copyright (c) 2016 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __S6E3HA6_H__
#define __S6E3HA6_H__

#include <linux/types.h>
#include <linux/kernel.h>

/*
 * OFFSET ==> OFS means N-param - 1
 * <example>
 * XXX 1st param => S6E3HA6_XXX_OFS (0)
 * XXX 2nd param => S6E3HA6_XXX_OFS (1)
 * XXX 36th param => S6E3HA6_XXX_OFS (35)
 */

#define AID_INTERPOLATION
#define GAMMA_CMD_CNT 36

#define S6E3HA6_ADDR_OFS	(0)
#define S6E3HA6_ADDR_LEN	(1)
#define S6E3HA6_DATA_OFS	(S6E3HA6_ADDR_OFS + S6E3HA6_ADDR_LEN)

#ifdef CONFIG_LCD_EXTEND_HBM
#define S6E3HA6_MAX_BRIGHTNESS (EXTEND_HBM_MAX_BRIGHTNESS)
#else
#define S6E3HA6_MAX_BRIGHTNESS (HBM_MAX_BRIGHTNESS)
#endif

#define S6E3HA6_MTP_REG				0xC8
#define S6E3HA6_MTP_OFS				0
#define S6E3HA6_MTP_LEN 			35

#define S6E3HA6_DATE_REG			0xA1
#define S6E3HA6_DATE_OFS			4
#define S6E3HA6_DATE_LEN			(PANEL_DATE_LEN)

#define S6E3HA6_COORDINATE_REG		0xA1
#define S6E3HA6_COORDINATE_OFS		0
#define S6E3HA6_COORDINATE_LEN		(PANEL_COORD_LEN)

#define S6E3HA6_HBM_GAMMA_REG		0xB3
#define S6E3HA6_HBM_GAMMA_OFS		2
#define S6E3HA6_HBM_GAMMA_LEN		33

#define S6E3HA6_ID_REG				0x04
#define S6E3HA6_ID_OFS				0
#define S6E3HA6_ID_LEN				(PANEL_ID_LEN)

#define S6E3HA6_CODE_REG			0xD6
#define S6E3HA6_CODE_OFS			0
#define S6E3HA6_CODE_LEN			(PANEL_CODE_LEN)

#define S6E3HA6_ELVSS_REG			0xB5
#define S6E3HA6_ELVSS_OFS			0
#define S6E3HA6_ELVSS_LEN			24

#define S6E3HA6_ELVSS_TEMP_0_REG		0xB5
#define S6E3HA6_ELVSS_TEMP_0_OFS		23
#define S6E3HA6_ELVSS_TEMP_0_LEN		1

#define S6E3HA6_ELVSS_TEMP_1_REG		0xB5
#define S6E3HA6_ELVSS_TEMP_1_OFS		24
#define S6E3HA6_ELVSS_TEMP_1_LEN		1

#define S6E3HA6_OCTA_ID_REG			0xC9
#define S6E3HA6_OCTA_ID_OFS			1
#define S6E3HA6_OCTA_ID_LEN			(PANEL_OCTA_ID_LEN)

#define S6E3HA6_COPR_REG			0x5A
#define S6E3HA6_COPR_OFS			0
#define S6E3HA6_COPR_LEN			1

#define S6E3HA6_CHIP_ID_REG			0xD6
#define S6E3HA6_CHIP_ID_OFS			0
#define S6E3HA6_CHIP_ID_LEN			5

/* for panel dump */
#define S6E3HA6_RDDPM_REG			0x0A
#define S6E3HA6_RDDPM_OFS			0
#define S6E3HA6_RDDPM_LEN			3

#define S6E3HA6_RDDSM_REG			0x0E
#define S6E3HA6_RDDSM_OFS			0
#define S6E3HA6_RDDSM_LEN			3

#define S6E3HA6_ERR_REG				0xEA
#define S6E3HA6_ERR_OFS				0
#define S6E3HA6_ERR_LEN				5

#define S6E3HA6_ERR_FG_REG			0xEE
#define S6E3HA6_ERR_FG_OFS			0
#define S6E3HA6_ERR_FG_LEN			1

#define S6E3HA6_DSI_ERR_REG			0x05
#define S6E3HA6_DSI_ERR_OFS			0
#define S6E3HA6_DSI_ERR_LEN			1

#define S6E3HA6_SELF_DIAG_REG			0x0F
#define S6E3HA6_SELF_DIAG_OFS			0
#define S6E3HA6_SELF_DIAG_LEN			1

#ifdef CONFIG_SUPPORT_POC_FLASH
#define S6E3HA6_POC_CHKSUM_REG		0xEC
#define S6E3HA6_POC_CHKSUM_OFS		0
#define S6E3HA6_POC_CHKSUM_LEN		(PANEL_POC_CHKSUM_LEN)

#define S6E3HA6_POC_CTRL_REG		0xEB
#define S6E3HA6_POC_CTRL_OFS		0
#define S6E3HA6_POC_CTRL_LEN		(PANEL_POC_CTRL_LEN)
#endif

#ifdef CONFIG_SUPPORT_GRAM_CHECKSUM
#define S6E3HA6_GRAM_IMG_SIZE	(1440 * 2960 * 3 / 3)	/* W * H * BPP / COMP_RATIO */
#define S6E3HA6_GRAM_CHECKSUM_REG	0xAF
#define S6E3HA6_GRAM_CHECKSUM_OFS	0
#define S6E3HA6_GRAM_CHECKSUM_LEN	1
#define S6E3HA6_GRAM_CHECKSUM_VALID	0x8B
#endif

#ifdef CONFIG_EXYNOS_DECON_MDNIE_LITE
#define NR_S6E3HA6_MDNIE_REG	(3)

#define S6E3HA6_MDNIE_0_REG		(0xDF)
#define S6E3HA6_MDNIE_0_OFS		(0)
#define S6E3HA6_MDNIE_0_LEN		(67)

#define S6E3HA6_MDNIE_1_REG		(0xDE)
#define S6E3HA6_MDNIE_1_OFS		(S6E3HA6_MDNIE_0_OFS + S6E3HA6_MDNIE_0_LEN)
#define S6E3HA6_MDNIE_1_LEN		(232)

#define S6E3HA6_MDNIE_2_REG		(0xDD)
#define S6E3HA6_MDNIE_2_OFS		(S6E3HA6_MDNIE_1_OFS + S6E3HA6_MDNIE_1_LEN)
#define S6E3HA6_MDNIE_2_LEN		(19)
#define S6E3HA6_MDNIE_LEN		(S6E3HA6_MDNIE_0_LEN + S6E3HA6_MDNIE_1_LEN + S6E3HA6_MDNIE_2_LEN)

#define S6E3HA6_SCR_CR_OFS	(31)
#define S6E3HA6_SCR_WR_OFS	(49)
#define S6E3HA6_SCR_WG_OFS	(51)
#define S6E3HA6_SCR_WB_OFS	(53)
#define S6E3HA6_NIGHT_MODE_OFS	(S6E3HA6_SCR_CR_OFS)
#define S6E3HA6_NIGHT_MODE_LEN	(24)

#define S6E3HA6_TRANS_MODE_OFS	(16)
#define S6E3HA6_TRANS_MODE_LEN	(1)
#endif /* CONFIG_EXYNOS_DECON_MDNIE_LITE */

enum {
	GAMMA_MAPTBL,
	AOR_MAPTBL,
	MPS_MAPTBL,
	TSET_MAPTBL,
	ELVSS_MAPTBL,
	ELVSS_TEMP_MAPTBL,
#ifdef CONFIG_SUPPORT_XTALK_MODE
	VGH_MAPTBL,
#endif
	VINT_MAPTBL,
	ACL_ONOFF_MAPTBL,
	ACL_FRAME_AVG_MAPTBL,
	ACL_START_POINT_MAPTBL,
	ACL_OPR_MAPTBL,
	IRC_MAPTBL,
#ifdef CONFIG_SUPPORT_HMD
	/* HMD MAPTBL */
	HMD_GAMMA_MAPTBL,
	HMD_AOR_MAPTBL,
	HMD_ELVSS_MAPTBL,
#endif /* CONFIG_SUPPORT_HMD */
	DSC_MAPTBL,
	PPS_MAPTBL,
	SCALER_MAPTBL,
	CASET_MAPTBL,
	PASET_MAPTBL,
	LPM_NIT_MAPTBL,
	LPM_MODE_MAPTBL,
	LPM_AOR_OFF_MAPTBL,
#ifdef CONFIG_ACTIVE_CLOCK
	ACTIVE_CLK_CTRL_MAPTBL,
	ACTIVE_CLK_SELF_DRAWER,
	ACTIVE_CLK_CTRL_UPDATE_MAPTBL,
#endif
#ifdef CONFIG_SUPPORT_POC_FLASH
	POC_ON_MAPTBL,
#endif
#ifdef CONFIG_SUPPORT_GRAM_CHECKSUM
	VDDM_MAPTBL,
	GRAM_IMG_0_MAPTBL,
	GRAM_IMG_1_MAPTBL,
#endif
	POC_ONOFF_MAPTBL,
	IRC_ONOFF_MAPTBL,
	MAX_MAPTBL,
};

enum {
#ifdef CONFIG_EXYNOS_DECON_LCD_COPR
	READ_COPR,
#endif
	READ_ID,
	READ_COORDINATE,
	READ_CODE,
	READ_ELVSS,
	READ_ELVSS_TEMP_0,
	READ_ELVSS_TEMP_1,
	READ_MTP,
	READ_DATE,
	READ_HBM_GAMMA,
	READ_OCTA_ID,
	READ_CHIP_ID,
	READ_RDDPM,
	READ_RDDSM,
	READ_ERR,
	READ_ERR_FG,
	READ_DSI_ERR,
	READ_SELF_DIAG,
#ifdef CONFIG_SUPPORT_POC_FLASH
	READ_POC_CHKSUM,
	READ_POC_CTRL,
#endif
#ifdef CONFIG_SUPPORT_GRAM_CHECKSUM
	READ_GRAM_CHECKSUM,
#endif

};

enum {
#ifdef CONFIG_EXYNOS_DECON_LCD_COPR
	RES_COPR,
#endif
	RES_ID,
	RES_COORDINATE,
	RES_CODE,
	RES_ELVSS,
	RES_ELVSS_TEMP_0,
	RES_ELVSS_TEMP_1,
	RES_MTP,
	RES_DATE,
	RES_HBM_GAMMA,
	RES_OCTA_ID,
	RES_CHIP_ID,
	RES_RDDPM,
	RES_RDDSM,
	RES_ERR,
	RES_ERR_FG,
	RES_DSI_ERR,
	RES_SELF_DIAG,
#ifdef CONFIG_SUPPORT_POC_FLASH
	RES_POC_CHKSUM,
	RES_POC_CTRL,
#endif
#ifdef CONFIG_SUPPORT_GRAM_CHECKSUM
	RES_GRAM_CHECKSUM,
#endif

};

static u8 S6E3HA6_ID[S6E3HA6_ID_LEN];
static u8 S6E3HA6_COORDINATE[S6E3HA6_COORDINATE_LEN];
static u8 S6E3HA6_CODE[S6E3HA6_CODE_LEN];
static u8 S6E3HA6_ELVSS[S6E3HA6_ELVSS_LEN];
static u8 S6E3HA6_ELVSS_TEMP_0[S6E3HA6_ELVSS_TEMP_0_LEN];
static u8 S6E3HA6_ELVSS_TEMP_1[S6E3HA6_ELVSS_TEMP_1_LEN];
static u8 S6E3HA6_MTP[S6E3HA6_MTP_LEN];
static u8 S6E3HA6_DATE[S6E3HA6_DATE_LEN];
static u8 S6E3HA6_HBM_GAMMA[S6E3HA6_HBM_GAMMA_LEN];
static u8 S6E3HA6_OCTA_ID[S6E3HA6_OCTA_ID_LEN];
#ifdef CONFIG_EXYNOS_DECON_LCD_COPR
static u8 S6E3HA6_COPR[S6E3HA6_COPR_LEN];
#endif

static u8 S6E3HA6_CHIP_ID[S6E3HA6_CHIP_ID_LEN];
static u8 S6E3HA6_RDDPM[S6E3HA6_RDDPM_LEN];
static u8 S6E3HA6_RDDSM[S6E3HA6_RDDSM_LEN];
static u8 S6E3HA6_ERR[S6E3HA6_ERR_LEN];
static u8 S6E3HA6_ERR_FG[S6E3HA6_ERR_FG_LEN];
static u8 S6E3HA6_DSI_ERR[S6E3HA6_DSI_ERR_LEN];
static u8 S6E3HA6_SELF_DIAG[S6E3HA6_SELF_DIAG_LEN];
#ifdef CONFIG_SUPPORT_POC_FLASH
static u8 S6E3HA6_POC_CHKSUM[S6E3HA6_POC_CHKSUM_LEN];
static u8 S6E3HA6_POC_CTRL[S6E3HA6_POC_CTRL_LEN];
#endif
#ifdef CONFIG_SUPPORT_GRAM_CHECKSUM
static u8 S6E3HA6_GRAM_CHECKSUM[S6E3HA6_GRAM_CHECKSUM_LEN];
#endif

static struct rdinfo s6e3ha6_rditbl[] = {
	[READ_ID] = RDINFO_INIT(id, DSI_PKT_TYPE_RD, S6E3HA6_ID_REG, S6E3HA6_ID_OFS, S6E3HA6_ID_LEN),
	[READ_COORDINATE] = RDINFO_INIT(coordinate, DSI_PKT_TYPE_RD, S6E3HA6_COORDINATE_REG, S6E3HA6_COORDINATE_OFS, S6E3HA6_COORDINATE_LEN),
	[READ_CODE] = RDINFO_INIT(code, DSI_PKT_TYPE_RD, S6E3HA6_CODE_REG, S6E3HA6_CODE_OFS, S6E3HA6_CODE_LEN),
	[READ_ELVSS] = RDINFO_INIT(elvss, DSI_PKT_TYPE_RD, S6E3HA6_ELVSS_REG, S6E3HA6_ELVSS_OFS, S6E3HA6_ELVSS_LEN),
	[READ_ELVSS_TEMP_0] = RDINFO_INIT(elvss_temp_0, DSI_PKT_TYPE_RD, S6E3HA6_ELVSS_TEMP_0_REG, S6E3HA6_ELVSS_TEMP_0_OFS, S6E3HA6_ELVSS_TEMP_0_LEN),
	[READ_ELVSS_TEMP_1] = RDINFO_INIT(elvss_temp_1, DSI_PKT_TYPE_RD, S6E3HA6_ELVSS_TEMP_1_REG, S6E3HA6_ELVSS_TEMP_1_OFS, S6E3HA6_ELVSS_TEMP_1_LEN),
	[READ_MTP] = RDINFO_INIT(mtp, DSI_PKT_TYPE_RD, S6E3HA6_MTP_REG, S6E3HA6_MTP_OFS, S6E3HA6_MTP_LEN),
	[READ_DATE] = RDINFO_INIT(date, DSI_PKT_TYPE_RD, S6E3HA6_DATE_REG, S6E3HA6_DATE_OFS, S6E3HA6_DATE_LEN),
	[READ_HBM_GAMMA] = RDINFO_INIT(hbm_gamma, DSI_PKT_TYPE_RD, S6E3HA6_HBM_GAMMA_REG, S6E3HA6_HBM_GAMMA_OFS, S6E3HA6_HBM_GAMMA_LEN),
	[READ_OCTA_ID] = RDINFO_INIT(octa_id, DSI_PKT_TYPE_RD, S6E3HA6_OCTA_ID_REG, S6E3HA6_OCTA_ID_OFS, S6E3HA6_OCTA_ID_LEN),
#ifdef CONFIG_EXYNOS_DECON_LCD_COPR
	[READ_COPR] = RDINFO_INIT(copr, SPI_PKT_TYPE_RD, S6E3HA6_COPR_REG, S6E3HA6_COPR_OFS, S6E3HA6_COPR_LEN),
#endif
	[READ_CHIP_ID] = RDINFO_INIT(chip_id, DSI_PKT_TYPE_RD, S6E3HA6_CHIP_ID_REG, S6E3HA6_CHIP_ID_OFS, S6E3HA6_CHIP_ID_LEN),
	[READ_RDDPM] = RDINFO_INIT(rddpm, DSI_PKT_TYPE_RD, S6E3HA6_RDDPM_REG, S6E3HA6_RDDPM_OFS, S6E3HA6_RDDPM_LEN),
	[READ_RDDSM] = RDINFO_INIT(rddsm, DSI_PKT_TYPE_RD, S6E3HA6_RDDSM_REG, S6E3HA6_RDDSM_OFS, S6E3HA6_RDDSM_LEN),
	[READ_ERR] = RDINFO_INIT(err, DSI_PKT_TYPE_RD, S6E3HA6_ERR_REG, S6E3HA6_ERR_OFS, S6E3HA6_ERR_LEN),
	[READ_ERR_FG] = RDINFO_INIT(err_fg, DSI_PKT_TYPE_RD, S6E3HA6_ERR_FG_REG, S6E3HA6_ERR_FG_OFS, S6E3HA6_ERR_FG_LEN),
	[READ_DSI_ERR] = RDINFO_INIT(dsi_err, DSI_PKT_TYPE_RD, S6E3HA6_DSI_ERR_REG, S6E3HA6_DSI_ERR_OFS, S6E3HA6_DSI_ERR_LEN),
	[READ_SELF_DIAG] = RDINFO_INIT(self_diag, DSI_PKT_TYPE_RD, S6E3HA6_SELF_DIAG_REG, S6E3HA6_SELF_DIAG_OFS, S6E3HA6_SELF_DIAG_LEN),
#ifdef CONFIG_SUPPORT_POC_FLASH
	[READ_POC_CHKSUM] = RDINFO_INIT(poc_chksum, DSI_PKT_TYPE_RD, S6E3HA6_POC_CHKSUM_REG, S6E3HA6_POC_CHKSUM_OFS, S6E3HA6_POC_CHKSUM_LEN),
	[READ_POC_CTRL] = RDINFO_INIT(poc_ctrl, DSI_PKT_TYPE_RD, S6E3HA6_POC_CTRL_REG, S6E3HA6_POC_CTRL_OFS, S6E3HA6_POC_CTRL_LEN),
#endif
#ifdef CONFIG_SUPPORT_GRAM_CHECKSUM
	[READ_GRAM_CHECKSUM] = RDINFO_INIT(gram_checksum, DSI_PKT_TYPE_RD, S6E3HA6_GRAM_CHECKSUM_REG, S6E3HA6_GRAM_CHECKSUM_OFS, S6E3HA6_GRAM_CHECKSUM_LEN),
#endif
};

static DEFINE_RESUI(id, &s6e3ha6_rditbl[READ_ID], 0);
static DEFINE_RESUI(coordinate, &s6e3ha6_rditbl[READ_COORDINATE], 0);
static DEFINE_RESUI(code, &s6e3ha6_rditbl[READ_CODE], 0);
static DEFINE_RESUI(elvss, &s6e3ha6_rditbl[READ_ELVSS], 0);
static DEFINE_RESUI(elvss_temp_0, &s6e3ha6_rditbl[READ_ELVSS_TEMP_0], 0);
static DEFINE_RESUI(elvss_temp_1, &s6e3ha6_rditbl[READ_ELVSS_TEMP_1], 0);
static DEFINE_RESUI(mtp, &s6e3ha6_rditbl[READ_MTP], 0);
static DEFINE_RESUI(hbm_gamma, &s6e3ha6_rditbl[READ_HBM_GAMMA], 0);
static DEFINE_RESUI(date, &s6e3ha6_rditbl[READ_DATE], 0);
static DEFINE_RESUI(octa_id, &s6e3ha6_rditbl[READ_OCTA_ID], 0);
#ifdef CONFIG_EXYNOS_DECON_LCD_COPR
static DEFINE_RESUI(copr, &s6e3ha6_rditbl[READ_COPR], 0);
#endif
static DEFINE_RESUI(chip_id, &s6e3ha6_rditbl[READ_CHIP_ID], 0);
static DEFINE_RESUI(rddpm, &s6e3ha6_rditbl[READ_RDDPM], 0);
static DEFINE_RESUI(rddsm, &s6e3ha6_rditbl[READ_RDDSM], 0);
static DEFINE_RESUI(err, &s6e3ha6_rditbl[READ_ERR], 0);
static DEFINE_RESUI(err_fg, &s6e3ha6_rditbl[READ_ERR_FG], 0);
static DEFINE_RESUI(dsi_err, &s6e3ha6_rditbl[READ_DSI_ERR], 0);
static DEFINE_RESUI(self_diag, &s6e3ha6_rditbl[READ_SELF_DIAG], 0);
#ifdef CONFIG_SUPPORT_POC_FLASH
static DEFINE_RESUI(poc_chksum, &s6e3ha6_rditbl[READ_POC_CHKSUM], 0);
static DEFINE_RESUI(poc_ctrl, &s6e3ha6_rditbl[READ_POC_CTRL], 0);
#endif
#ifdef CONFIG_SUPPORT_GRAM_CHECKSUM
static DEFINE_RESUI(gram_checksum, &s6e3ha6_rditbl[READ_GRAM_CHECKSUM], 0);
#endif

static struct resinfo s6e3ha6_restbl[] = {
	[RES_ID] = RESINFO_INIT(id, S6E3HA6_ID, RESUI(id)),
	[RES_COORDINATE] = RESINFO_INIT(coordinate, S6E3HA6_COORDINATE, RESUI(coordinate)),
	[RES_CODE] = RESINFO_INIT(code, S6E3HA6_CODE, RESUI(code)),
	[RES_ELVSS] = RESINFO_INIT(elvss, S6E3HA6_ELVSS, RESUI(elvss)),
	[RES_ELVSS_TEMP_0] = RESINFO_INIT(elvss_temp_0, S6E3HA6_ELVSS_TEMP_0, RESUI(elvss_temp_0)),
	[RES_ELVSS_TEMP_1] = RESINFO_INIT(elvss_temp_1, S6E3HA6_ELVSS_TEMP_1, RESUI(elvss_temp_1)),
	[RES_MTP] = RESINFO_INIT(mtp, S6E3HA6_MTP, RESUI(mtp)),
	[RES_DATE] = RESINFO_INIT(date, S6E3HA6_DATE, RESUI(date)),
	[RES_HBM_GAMMA] = RESINFO_INIT(hbm_gamma, S6E3HA6_HBM_GAMMA, RESUI(hbm_gamma)),
	[RES_OCTA_ID] = RESINFO_INIT(octa_id, S6E3HA6_OCTA_ID, RESUI(octa_id)),
#ifdef CONFIG_EXYNOS_DECON_LCD_COPR
	[RES_COPR] = RESINFO_INIT(copr, S6E3HA6_COPR, RESUI(copr)),
#endif
	[RES_CHIP_ID] = RESINFO_INIT(chip_id, S6E3HA6_CHIP_ID, RESUI(chip_id)),
	[RES_RDDPM] = RESINFO_INIT(rddpm, S6E3HA6_RDDPM, RESUI(rddpm)),
	[RES_RDDSM] = RESINFO_INIT(rddsm, S6E3HA6_RDDSM, RESUI(rddsm)),
	[RES_ERR] = RESINFO_INIT(err, S6E3HA6_ERR, RESUI(err)),
	[RES_ERR_FG] = RESINFO_INIT(err_fg, S6E3HA6_ERR_FG, RESUI(err_fg)),
	[RES_DSI_ERR] = RESINFO_INIT(dsi_err, S6E3HA6_DSI_ERR, RESUI(dsi_err)),
	[RES_SELF_DIAG] = RESINFO_INIT(self_diag, S6E3HA6_SELF_DIAG, RESUI(self_diag)),
#ifdef CONFIG_SUPPORT_POC_FLASH
	[RES_POC_CHKSUM] = RESINFO_INIT(poc_chksum, S6E3HA6_POC_CHKSUM, RESUI(poc_chksum)),
	[RES_POC_CTRL] = RESINFO_INIT(poc_ctrl, S6E3HA6_POC_CTRL, RESUI(poc_ctrl)),
#endif
#ifdef CONFIG_SUPPORT_GRAM_CHECKSUM
	[RES_GRAM_CHECKSUM] = RESINFO_INIT(gram_checksum, S6E3HA6_GRAM_CHECKSUM, RESUI(gram_checksum)),
#endif
};

enum {
	DUMP_RDDPM = 0,
	DUMP_RDDSM,
	DUMP_ERR,
	DUMP_ERR_FG,
	DUMP_DSI_ERR,
	DUMP_SELF_DIAG,
};

void show_rddpm(struct dumpinfo *info);
void show_rddsm(struct dumpinfo *info);
void show_err(struct dumpinfo *info);
void show_err_fg(struct dumpinfo *info);
void show_dsi_err(struct dumpinfo *info);
void show_self_diag(struct dumpinfo *info);

static struct dumpinfo s6e3ha6_dmptbl[] = {
	[DUMP_RDDPM] = DUMPINFO_INIT(rddpm, &s6e3ha6_restbl[RES_RDDPM], show_rddpm),
	[DUMP_RDDSM] = DUMPINFO_INIT(rddsm, &s6e3ha6_restbl[RES_RDDSM], show_rddsm),
	[DUMP_ERR] = DUMPINFO_INIT(err, &s6e3ha6_restbl[RES_ERR], show_err),
	[DUMP_ERR_FG] = DUMPINFO_INIT(err_fg, &s6e3ha6_restbl[RES_ERR_FG], show_err_fg),
	[DUMP_DSI_ERR] = DUMPINFO_INIT(dsi_err, &s6e3ha6_restbl[RES_DSI_ERR], show_dsi_err),
	[DUMP_SELF_DIAG] = DUMPINFO_INIT(self_diag, &s6e3ha6_restbl[RES_SELF_DIAG], show_self_diag),
};

int init_common_table(struct maptbl *);
int getidx_common_maptbl(struct maptbl *);
int init_gamma_table(struct maptbl *);
int getidx_dimming_maptbl(struct maptbl *);
int getidx_brt_tbl(struct maptbl *tbl);
int getidx_aor_table(struct maptbl *tbl);
int getidx_irc_table(struct maptbl *tbl);
int getidx_poc_onoff_table(struct maptbl *tbl);
int getidx_irc_onoff_table(struct maptbl *tbl);

int getidx_mps_table(struct maptbl *);
int init_elvss_temp_table(struct maptbl *);
int getidx_elvss_temp_table(struct maptbl *);
#ifdef CONFIG_SUPPORT_XTALK_MODE
int getidx_vgh_table(struct maptbl *);
#endif
#ifdef CONFIG_SUPPORT_POC_FLASH
void copy_poc_maptbl(struct maptbl *, u8 *);
#endif
int getidx_vint_table(struct maptbl *);
int getidx_hbm_onoff_table(struct maptbl *);
int getidx_acl_onoff_table(struct maptbl *);
int getidx_acl_opr_table(struct maptbl *);
int getidx_dsc_table(struct maptbl *);
int getidx_resolution_table(struct maptbl *);
int getidx_lpm_table(struct maptbl *);
void copy_common_maptbl(struct maptbl *, u8 *);
void copy_tset_maptbl(struct maptbl *, u8 *);
void copy_copr_maptbl(struct maptbl *, u8 *);
#ifdef CONFIG_ACTIVE_CLOCK
void copy_self_clk_update_maptbl(struct maptbl *tbl, u8 *dst);
void copy_self_clk_maptbl(struct maptbl *, u8 *);
void copy_self_drawer(struct maptbl *tbl, u8 *dst);
#endif
#ifdef CONFIG_SUPPORT_GRAM_CHECKSUM
int s6e3ha6_getidx_vddm_table(struct maptbl *);
void copy_gram_img_pattern_0(struct maptbl *, u8 *);
void copy_gram_img_pattern_1(struct maptbl *, u8 *);
#endif
#ifdef CONFIG_SUPPORT_HMD
int init_hmd_gamma_table(struct maptbl *);
int getidx_hmd_dimming_mtptbl(struct maptbl *);
#endif /* CONFIG_SUPPORT_HMD */
#ifdef CONFIG_EXYNOS_DECON_MDNIE_LITE
int init_color_blind_table(struct maptbl *tbl);
int getidx_mdnie_scenario_maptbl(struct maptbl *tbl);
int getidx_mdnie_hmd_maptbl(struct maptbl *tbl);
int getidx_mdnie_hdr_maptbl(struct maptbl *tbl);
int getidx_mdnie_trans_mode_maptbl(struct maptbl *tbl);
int init_mdnie_night_mode_table(struct maptbl *tbl);
int getidx_mdnie_night_mode_maptbl(struct maptbl *tbl);
int init_color_coordinate_table(struct maptbl *);
int init_sensor_rgb_table(struct maptbl *tbl);
int getidx_adjust_ldu_maptbl(struct maptbl *tbl);
int getidx_color_coordinate_maptbl(struct maptbl *tbl);
void copy_color_coordinate_maptbl(struct maptbl *tbl, u8 *dst);
void copy_scr_white_maptbl(struct maptbl *tbl, u8 *dst);
void copy_adjust_ldu_maptbl(struct maptbl *tbl, u8 *dst);
int getidx_trans_maptbl(struct pkt_update_info *pktui);
int getidx_mdnie_0_maptbl(struct pkt_update_info *pktui);
int getidx_mdnie_1_maptbl(struct pkt_update_info *pktui);
int getidx_mdnie_2_maptbl(struct pkt_update_info *pktui);
int getidx_mdnie_scr_white_maptbl(struct pkt_update_info *pktui);
void update_current_scr_white(struct maptbl *tbl, u8 *dst);
#endif /* CONFIG_EXYNOS_DECON_MDNIE_LITE */
#endif /* __S6E3HA6_H__ */
