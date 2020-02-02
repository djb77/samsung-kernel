/*
 * linux/drivers/video/fbdev/exynos/panel/s6e3hf3/s6e3hf3.h
 *
 * Header file for S6E3HF3 Dimming Driver
 *
 * Copyright (c) 2016 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __S6E3HF3_H__
#define __S6E3HF3_H__

#include <linux/types.h>
#include <linux/kernel.h>

/*
 * OFFSET ==> OFS means N-param - 1
 * <example>
 * XXX 1st param => S6E3HF3_XXX_OFS (0)
 * XXX 2nd param => S6E3HF3_XXX_OFS (1)
 * XXX 36th param => S6E3HF3_XXX_OFS (35)
 */

#define AID_INTERPOLATION
#define GAMMA_CMD_CNT 36

#define S6E3HF3_ADDR_OFS	(0)
#define S6E3HF3_ADDR_LEN	(1)
#define S6E3HF3_DATA_OFS	(S6E3HF3_ADDR_OFS + S6E3HF3_ADDR_LEN)

#define S6E3HF3_MTP_REG				0xC8
#define S6E3HF3_MTP_OFS				0
#define S6E3HF3_MTP_LEN 			35

#define S6E3HF3_DATE_REG			0xC8
#define S6E3HF3_DATE_OFS			40
#define S6E3HF3_DATE_LEN			(PANEL_DATE_LEN)

#define S6E3HF3_COORDINATE_REG		0xA1
#define S6E3HF3_COORDINATE_OFS		0
#define S6E3HF3_COORDINATE_LEN		(PANEL_COORD_LEN)

#define S6E3HF3_HBM_GAMMA_REG		0xB3
#define S6E3HF3_HBM_GAMMA_OFS		0
#define S6E3HF3_HBM_GAMMA_LEN		(33)

#define S6E3HF3_ID_REG				0x04
#define S6E3HF3_ID_OFS				0
#define S6E3HF3_ID_LEN				(PANEL_ID_LEN)

#define S6E3HF3_CODE_REG			0xD6
#define S6E3HF3_CODE_OFS			0
#define S6E3HF3_CODE_LEN			(PANEL_CODE_LEN)

#define S6E3HF3_ELVSS_REG			0xB5
#define S6E3HF3_ELVSS_OFS			0
#define S6E3HF3_ELVSS_LEN			22

#define S6E3HF3_ELVSS_TEMP_REG		0xB5
#define S6E3HF3_ELVSS_TEMP_OFS		21
#define S6E3HF3_ELVSS_TEMP_LEN		1

#ifdef CONFIG_SEC_FACTORY
#define S6E3HF3_OCTA_ID_REG			0xC9
#define S6E3HF3_OCTA_ID_OFS			1
#define S6E3HF3_OCTA_ID_LEN			(PANEL_OCTA_ID_LEN)
#endif

#define S6E3HF3_CHIP_ID_REG			0xD6
#define S6E3HF3_CHIP_ID_OFS			0
#define S6E3HF3_CHIP_ID_LEN			5

/* for panel dump */
#define S6E3HF3_RDDPM_REG			0x0A
#define S6E3HF3_RDDPM_OFS			0
#define S6E3HF3_RDDPM_LEN			3

#define S6E3HF3_RDDSM_REG			0x0E
#define S6E3HF3_RDDSM_OFS			0
#define S6E3HF3_RDDSM_LEN			3

#define S6E3HF3_ERR_REG				0xEA
#define S6E3HF3_ERR_OFS				0
#define S6E3HF3_ERR_LEN				5

#define S6E3HF3_ERR_FG_REG			0xEE
#define S6E3HF3_ERR_FG_OFS			0
#define S6E3HF3_ERR_FG_LEN			1

#define S6E3HF3_DSI_ERR_REG			0x05
#define S6E3HF3_DSI_ERR_OFS			0
#define S6E3HF3_DSI_ERR_LEN			1

#define S6E3HF3_SELF_DIAG_REG			0x0F
#define S6E3HF3_SELF_DIAG_OFS			0
#define S6E3HF3_SELF_DIAG_LEN			1

#ifdef CONFIG_EXYNOS_DECON_MDNIE_LITE
#define NR_S6E3HF3_MDNIE_REG	(3)

#define S6E3HF3_MDNIE_0_REG		(0xDF)
#define S6E3HF3_MDNIE_0_OFS		(0)
#define S6E3HF3_MDNIE_0_LEN		(71)

#define S6E3HF3_MDNIE_1_REG		(0xDE)
#define S6E3HF3_MDNIE_1_OFS		(S6E3HF3_MDNIE_0_OFS + S6E3HF3_MDNIE_0_LEN)
#define S6E3HF3_MDNIE_1_LEN		(208)

#define S6E3HF3_MDNIE_2_REG		(0xDD)
#define S6E3HF3_MDNIE_2_OFS		(S6E3HF3_MDNIE_1_OFS + S6E3HF3_MDNIE_1_LEN)
#define S6E3HF3_MDNIE_2_LEN		(18)
#define S6E3HF3_MDNIE_LEN		(S6E3HF3_MDNIE_0_LEN + S6E3HF3_MDNIE_1_LEN + S6E3HF3_MDNIE_2_LEN)

#define S6E3HF3_SCR_CR_OFS	(31)
#define S6E3HF3_SCR_WR_OFS	(49)
#define S6E3HF3_SCR_WG_OFS	(51)
#define S6E3HF3_SCR_WB_OFS	(53)
#define S6E3HF3_NIGHT_MODE_OFS	(S6E3HF3_SCR_CR_OFS)
#define S6E3HF3_NIGHT_MODE_LEN	(24)

#define S6E3HF3_TRANS_MODE_OFS	(16)
#define S6E3HF3_TRANS_MODE_LEN	(1)
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
	DD_SEL_MAPTBL,
	DSC_MAPTBL,
	PPS_MAPTBL,
	SCALER_MAPTBL,
	CASET_MAPTBL,
	PASET_MAPTBL,
	LPM_NIT_MAPTBL,
	LPM_MODE_MAPTBL,
	MAX_MAPTBL,
};

enum {
	READ_ID,
	READ_COORDINATE,
	READ_CODE,
	READ_ELVSS,
	READ_ELVSS_TEMP,
	READ_MTP,
	READ_DATE,
	READ_HBM_GAMMA,
#ifdef CONFIG_SEC_FACTORY
	READ_OCTA_ID,
#endif
	READ_CHIP_ID,
	READ_RDDPM,
	READ_RDDSM,
	READ_ERR,
	READ_ERR_FG,
	READ_DSI_ERR,
	READ_SELF_DIAG,
};

enum {
	RES_ID,
	RES_COORDINATE,
	RES_CODE,
	RES_ELVSS,
	RES_ELVSS_TEMP,
	RES_MTP,
	RES_DATE,
	RES_HBM_GAMMA,
#ifdef CONFIG_SEC_FACTORY
	RES_OCTA_ID,
#endif
	RES_CHIP_ID,
	RES_RDDPM,
	RES_RDDSM,
	RES_ERR,
	RES_ERR_FG,
	RES_DSI_ERR,
	RES_SELF_DIAG,
};

static u8 S6E3HF3_ID[S6E3HF3_ID_LEN];
static u8 S6E3HF3_COORDINATE[S6E3HF3_COORDINATE_LEN];
static u8 S6E3HF3_CODE[S6E3HF3_CODE_LEN];
static u8 S6E3HF3_ELVSS[S6E3HF3_ELVSS_LEN];
static u8 S6E3HF3_ELVSS_TEMP[S6E3HF3_ELVSS_TEMP_LEN];
static u8 S6E3HF3_MTP[S6E3HF3_MTP_LEN];
static u8 S6E3HF3_DATE[S6E3HF3_DATE_LEN];
static u8 S6E3HF3_HBM_GAMMA[S6E3HF3_HBM_GAMMA_LEN];
#ifdef CONFIG_SEC_FACTORY
static u8 S6E3HF3_OCTA_ID[S6E3HF3_OCTA_ID_LEN];
#endif

static u8 S6E3HF3_CHIP_ID[S6E3HF3_CHIP_ID_LEN];
static u8 S6E3HF3_RDDPM[S6E3HF3_RDDPM_LEN];
static u8 S6E3HF3_RDDSM[S6E3HF3_RDDSM_LEN];
static u8 S6E3HF3_ERR[S6E3HF3_ERR_LEN];
static u8 S6E3HF3_ERR_FG[S6E3HF3_ERR_FG_LEN];
static u8 S6E3HF3_DSI_ERR[S6E3HF3_DSI_ERR_LEN];
static u8 S6E3HF3_SELF_DIAG[S6E3HF3_SELF_DIAG_LEN];

static struct rdinfo s6e3hf3_rditbl[] = {
	[READ_ID] = RDINFO_INIT(id, DSI_PKT_TYPE_RD, S6E3HF3_ID_REG, S6E3HF3_ID_OFS, S6E3HF3_ID_LEN),
	[READ_COORDINATE] = RDINFO_INIT(coordinate, DSI_PKT_TYPE_RD, S6E3HF3_COORDINATE_REG, S6E3HF3_COORDINATE_OFS, S6E3HF3_COORDINATE_LEN),
	[READ_CODE] = RDINFO_INIT(code, DSI_PKT_TYPE_RD, S6E3HF3_CODE_REG, S6E3HF3_CODE_OFS, S6E3HF3_CODE_LEN),
	[READ_ELVSS] = RDINFO_INIT(elvss, DSI_PKT_TYPE_RD, S6E3HF3_ELVSS_REG, S6E3HF3_ELVSS_OFS, S6E3HF3_ELVSS_LEN),
	[READ_ELVSS_TEMP] = RDINFO_INIT(elvss_temp, DSI_PKT_TYPE_RD, S6E3HF3_ELVSS_TEMP_REG, S6E3HF3_ELVSS_TEMP_OFS, S6E3HF3_ELVSS_TEMP_LEN),
	[READ_MTP] = RDINFO_INIT(mtp, DSI_PKT_TYPE_RD, S6E3HF3_MTP_REG, S6E3HF3_MTP_OFS, S6E3HF3_MTP_LEN),
	[READ_DATE] = RDINFO_INIT(date, DSI_PKT_TYPE_RD, S6E3HF3_DATE_REG, S6E3HF3_DATE_OFS, S6E3HF3_DATE_LEN),
	[READ_HBM_GAMMA] = RDINFO_INIT(hbm_gamma, DSI_PKT_TYPE_RD, S6E3HF3_HBM_GAMMA_REG, S6E3HF3_HBM_GAMMA_OFS, S6E3HF3_HBM_GAMMA_LEN),
#ifdef CONFIG_SEC_FACTORY
	[READ_OCTA_ID] = RDINFO_INIT(octa_id, DSI_PKT_TYPE_RD, S6E3HF3_OCTA_ID_REG, S6E3HF3_OCTA_ID_OFS, S6E3HF3_OCTA_ID_LEN),
#endif
	[READ_CHIP_ID] = RDINFO_INIT(chip_id, DSI_PKT_TYPE_RD, S6E3HF3_CHIP_ID_REG, S6E3HF3_CHIP_ID_OFS, S6E3HF3_CHIP_ID_LEN),
	[READ_RDDPM] = RDINFO_INIT(rddpm, DSI_PKT_TYPE_RD, S6E3HF3_RDDPM_REG, S6E3HF3_RDDPM_OFS, S6E3HF3_RDDPM_LEN),
	[READ_RDDSM] = RDINFO_INIT(rddsm, DSI_PKT_TYPE_RD, S6E3HF3_RDDSM_REG, S6E3HF3_RDDSM_OFS, S6E3HF3_RDDSM_LEN),
	[READ_ERR] = RDINFO_INIT(err, DSI_PKT_TYPE_RD, S6E3HF3_ERR_REG, S6E3HF3_ERR_OFS, S6E3HF3_ERR_LEN),
	[READ_ERR_FG] = RDINFO_INIT(err_fg, DSI_PKT_TYPE_RD, S6E3HF3_ERR_FG_REG, S6E3HF3_ERR_FG_OFS, S6E3HF3_ERR_FG_LEN),
	[READ_DSI_ERR] = RDINFO_INIT(dsi_err, DSI_PKT_TYPE_RD, S6E3HF3_DSI_ERR_REG, S6E3HF3_DSI_ERR_OFS, S6E3HF3_DSI_ERR_LEN),
	[READ_SELF_DIAG] = RDINFO_INIT(self_diag, DSI_PKT_TYPE_RD, S6E3HF3_SELF_DIAG_REG, S6E3HF3_SELF_DIAG_OFS, S6E3HF3_SELF_DIAG_LEN),
};

static DEFINE_RESUI(id, &s6e3hf3_rditbl[READ_ID], 0);
static DEFINE_RESUI(coordinate, &s6e3hf3_rditbl[READ_COORDINATE], 0);
static DEFINE_RESUI(code, &s6e3hf3_rditbl[READ_CODE], 0);
static DEFINE_RESUI(elvss, &s6e3hf3_rditbl[READ_ELVSS], 0);
static DEFINE_RESUI(elvss_temp, &s6e3hf3_rditbl[READ_ELVSS_TEMP], 0);
static DEFINE_RESUI(mtp, &s6e3hf3_rditbl[READ_MTP], 0);
static DEFINE_RESUI(hbm_gamma, &s6e3hf3_rditbl[READ_HBM_GAMMA], 0);
static DEFINE_RESUI(date, &s6e3hf3_rditbl[READ_DATE], 0);
#ifdef CONFIG_SEC_FACTORY
static DEFINE_RESUI(octa_id, &s6e3hf3_rditbl[READ_OCTA_ID], 0);
#endif
static DEFINE_RESUI(chip_id, &s6e3hf3_rditbl[READ_CHIP_ID], 0);
static DEFINE_RESUI(rddpm, &s6e3hf3_rditbl[READ_RDDPM], 0);
static DEFINE_RESUI(rddsm, &s6e3hf3_rditbl[READ_RDDSM], 0);
static DEFINE_RESUI(err, &s6e3hf3_rditbl[READ_ERR], 0);
static DEFINE_RESUI(err_fg, &s6e3hf3_rditbl[READ_ERR_FG], 0);
static DEFINE_RESUI(dsi_err, &s6e3hf3_rditbl[READ_DSI_ERR], 0);
static DEFINE_RESUI(self_diag, &s6e3hf3_rditbl[READ_SELF_DIAG], 0);

static struct resinfo s6e3hf3_restbl[] = {
	[RES_ID] = RESINFO_INIT(id, S6E3HF3_ID, RESUI(id)),
	[RES_COORDINATE] = RESINFO_INIT(coordinate, S6E3HF3_COORDINATE, RESUI(coordinate)),
	[RES_CODE] = RESINFO_INIT(code, S6E3HF3_CODE, RESUI(code)),
	[RES_ELVSS] = RESINFO_INIT(elvss, S6E3HF3_ELVSS, RESUI(elvss)),
	[RES_ELVSS_TEMP] = RESINFO_INIT(elvss_temp, S6E3HF3_ELVSS_TEMP, RESUI(elvss_temp)),
	[RES_MTP] = RESINFO_INIT(mtp, S6E3HF3_MTP, RESUI(mtp)),
	[RES_DATE] = RESINFO_INIT(date, S6E3HF3_DATE, RESUI(date)),
	[RES_HBM_GAMMA] = RESINFO_INIT(hbm_gamma, S6E3HF3_HBM_GAMMA, RESUI(hbm_gamma)),
#ifdef CONFIG_SEC_FACTORY
	[RES_OCTA_ID] = RESINFO_INIT(octa_id, S6E3HF3_OCTA_ID, RESUI(octa_id)),
#endif
	[RES_CHIP_ID] = RESINFO_INIT(chip_id, S6E3HF3_CHIP_ID, RESUI(chip_id)),
	[RES_RDDPM] = RESINFO_INIT(rddpm, S6E3HF3_RDDPM, RESUI(rddpm)),
	[RES_RDDSM] = RESINFO_INIT(rddsm, S6E3HF3_RDDSM, RESUI(rddsm)),
	[RES_ERR] = RESINFO_INIT(err, S6E3HF3_ERR, RESUI(err)),
	[RES_ERR_FG] = RESINFO_INIT(err_fg, S6E3HF3_ERR_FG, RESUI(err_fg)),
	[RES_DSI_ERR] = RESINFO_INIT(dsi_err, S6E3HF3_DSI_ERR, RESUI(dsi_err)),
	[RES_SELF_DIAG] = RESINFO_INIT(self_diag, S6E3HF3_SELF_DIAG, RESUI(self_diag)),
};

enum {
	DUMP_RDDPM = 0,
	DUMP_RDDSM,
	DUMP_ERR,
	DUMP_ERR_FG,
	DUMP_DSI_ERR,
	DUMP_SELF_DIAG,
};

void s6e3hf3_show_rddpm(struct dumpinfo *info);
void s6e3hf3_show_rddsm(struct dumpinfo *info);
void s6e3hf3_show_err(struct dumpinfo *info);
void s6e3hf3_show_err_fg(struct dumpinfo *info);
void s6e3hf3_show_dsi_err(struct dumpinfo *info);
void s6e3hf3_show_self_diag(struct dumpinfo *info);

static struct dumpinfo s6e3hf3_dmptbl[] = {
	[DUMP_RDDPM] = DUMPINFO_INIT(rddpm, &s6e3hf3_restbl[RES_RDDPM], s6e3hf3_show_rddpm),
	[DUMP_RDDSM] = DUMPINFO_INIT(rddsm, &s6e3hf3_restbl[RES_RDDSM], s6e3hf3_show_rddsm),
	[DUMP_ERR] = DUMPINFO_INIT(err, &s6e3hf3_restbl[RES_ERR], s6e3hf3_show_err),
	[DUMP_ERR_FG] = DUMPINFO_INIT(err_fg, &s6e3hf3_restbl[RES_ERR_FG], s6e3hf3_show_err_fg),
	[DUMP_DSI_ERR] = DUMPINFO_INIT(dsi_err, &s6e3hf3_restbl[RES_DSI_ERR], s6e3hf3_show_dsi_err),
	[DUMP_SELF_DIAG] = DUMPINFO_INIT(self_diag, &s6e3hf3_restbl[RES_SELF_DIAG], s6e3hf3_show_self_diag),
};

int s6e3hf3_init_common_table(struct maptbl *);
int s6e3hf3_getidx_common_maptbl(struct maptbl *);
int s6e3hf3_init_gamma_table(struct maptbl *);
int s6e3hf3_getidx_dimming_maptbl(struct maptbl *);
int s6e3hf3_getidx_brt_tbl(struct maptbl *tbl);
int s6e3hf3_getidx_aor_table(struct maptbl *tbl);
int s6e3hf3_getidx_irc_table(struct maptbl *tbl);
int s6e3hf3_getidx_mps_table(struct maptbl *);
int s6e3hf3_getidx_elvss_temp_table(struct maptbl *);
#ifdef CONFIG_SUPPORT_XTALK_MODE
int s6e3hf3_getidx_vgh_table(struct maptbl *);
#endif
int s6e3hf3_getidx_vint_table(struct maptbl *);
int s6e3hf3_getidx_hbm_onoff_table(struct maptbl *);
int s6e3hf3_getidx_acl_onoff_table(struct maptbl *);
int s6e3hf3_getidx_acl_opr_table(struct maptbl *);
int s6e3hf3_getidx_dsc_table(struct maptbl *);
int s6e3hf3_getidx_resolution_table(struct maptbl *);
int s6e3hf3_getidx_dd_sel_table(struct maptbl *);
int s6e3hf3_getidx_lpm_table(struct maptbl *);
void s6e3hf3_copy_common_maptbl(struct maptbl *, u8 *);
void s6e3hf3_copy_elvss_temp_maptbl(struct maptbl *, u8 *);
void s6e3hf3_copy_tset_maptbl(struct maptbl *, u8 *);
void s6e3hf3_copy_copr_maptbl(struct maptbl *, u8 *);
#ifdef CONFIG_ACTIVE_CLOCK
void s6e3hf3_copy_self_clk_update_maptbl(struct maptbl *tbl, u8 *dst);
void s6e3hf3_copy_self_clk_maptbl(struct maptbl *, u8 *);
void s6e3hf3_copy_self_drawer(struct maptbl *tbl, u8 *dst);
#endif
#ifdef CONFIG_SUPPORT_HMD
int s6e3hf3_init_hmd_gamma_table(struct maptbl *);
int s6e3hf3_getidx_hmd_dimming_mtptbl(struct maptbl *);
#endif /* CONFIG_SUPPORT_HMD */
#ifdef CONFIG_EXYNOS_DECON_MDNIE_LITE
int s6e3hf3_init_color_blind_table(struct maptbl *tbl);
int s6e3hf3_getidx_mdnie_scenario_maptbl(struct maptbl *tbl);
int s6e3hf3_getidx_mdnie_hmd_maptbl(struct maptbl *tbl);
int s6e3hf3_getidx_mdnie_hdr_maptbl(struct maptbl *tbl);
int s6e3hf3_getidx_mdnie_trans_mode_maptbl(struct maptbl *tbl);
int s6e3hf3_init_mdnie_night_mode_table(struct maptbl *tbl);
int s6e3hf3_getidx_mdnie_night_mode_maptbl(struct maptbl *tbl);
int s6e3hf3_init_color_coordinate_table(struct maptbl *);
int s6e3hf3_init_sensor_rgb_table(struct maptbl *tbl);
int s6e3hf3_getidx_adjust_ldu_maptbl(struct maptbl *tbl);
int s6e3hf3_getidx_color_coordinate_maptbl(struct maptbl *tbl);
void s6e3hf3_copy_color_coordinate_maptbl(struct maptbl *tbl, u8 *dst);
void s6e3hf3_copy_scr_white_maptbl(struct maptbl *tbl, u8 *dst);
int s6e3hf3_getidx_trans_maptbl(struct pkt_update_info *pktui);
int s6e3hf3_getidx_mdnie_0_maptbl(struct pkt_update_info *pktui);
int s6e3hf3_getidx_mdnie_1_maptbl(struct pkt_update_info *pktui);
int s6e3hf3_getidx_mdnie_2_maptbl(struct pkt_update_info *pktui);
int s6e3hf3_getidx_mdnie_scr_white_maptbl(struct pkt_update_info *pktui);
void s6e3hf3_update_current_scr_white(struct maptbl *tbl, u8 *dst);
#endif /* CONFIG_EXYNOS_DECON_MDNIE_LITE */
#endif /* __S6E3HF3_H__ */
