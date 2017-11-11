/*
 * linux/drivers/video/fbdev/exynos/panel/s6e3hf4/s6e3hf4.h
 *
 * Header file for S6E3HF4 Dimming Driver
 *
 * Copyright (c) 2016 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __S6E3HF4_H__
#define __S6E3HF4_H__

#include <linux/types.h>
#include <linux/kernel.h>

#define AID_INTERPOLATION
#define GAMMA_CMD_CNT 36

#define S6E3HF4_MAX_BRIGHTNESS (HBM_MAX_BRIGHTNESS)

#define S6E3HF4_MTP_REG				0xC8
#define S6E3HF4_MTP_OFS				0
#define S6E3HF4_MTP_LEN 			35
#define S6E3HF4_MTP_VT_ADDR			33

#define S6E3HF4_DATE_REG			0xC8
#define S6E3HF4_DATE_OFS			40
#define S6E3HF4_DATE_LEN			7

#define S6E3HF4_COORDINATE_REG		0xA1
#define S6E3HF4_COORDINATE_OFS		0
#define S6E3HF4_COORDINATE_LEN		4

#define S6E3HF4_HBM_GAMMA_0_REG		0xC8
#define S6E3HF4_HBM_GAMMA_0_OFS		50
#define S6E3HF4_HBM_GAMMA_0_LEN		15

#define S6E3HF4_HBM_GAMMA_1_REG		0xC8
#define S6E3HF4_HBM_GAMMA_1_OFS		68
#define S6E3HF4_HBM_GAMMA_1_LEN		20

#define S6E3HF4_ID_REG				0x04
#define S6E3HF4_ID_OFS				0
#define S6E3HF4_ID_LEN				3

#define S6E3HF4_CODE_REG			0xD6
#define S6E3HF4_CODE_OFS			1
#define S6E3HF4_CODE_LEN			5

#define S6E3HF4_ELVSS_REG			0xB5
#define S6E3HF4_ELVSS_OFS			0
#define S6E3HF4_ELVSS_LEN			23

#define S6E3HF4_ELVSS_TEMP_REG		0xB5
#define S6E3HF4_ELVSS_TEMP_OFS		22
#define S6E3HF4_ELVSS_TEMP_LEN		1

#ifdef CONFIG_SEC_FACTORY
#define S6E3HF4_OCTA_ID_REG			0xC9
#define S6E3HF4_OCTA_ID_OFS			1
#define S6E3HF4_OCTA_ID_LEN			(OCTA_ID_LEN)
#endif

#ifdef CONFIG_EXYNOS_DECON_MDNIE_LITE
#define NR_S6E3HF4_MDNIE_REG	(3)

#define S6E3HF4_MDNIE_0_REG		(0xDF)
#define S6E3HF4_MDNIE_0_OFS		(0)
#define S6E3HF4_MDNIE_0_LEN		(71)

#define S6E3HF4_MDNIE_1_REG		(0xDE)
#define S6E3HF4_MDNIE_1_OFS		(S6E3HF4_MDNIE_0_OFS + S6E3HF4_MDNIE_0_LEN)
#define S6E3HF4_MDNIE_1_LEN		(208)

#define S6E3HF4_MDNIE_2_REG		(0xDD)
#define S6E3HF4_MDNIE_2_OFS		(S6E3HF4_MDNIE_1_OFS + S6E3HF4_MDNIE_1_LEN)
#define S6E3HF4_MDNIE_2_LEN		(18)
#define S6E3HF4_MDNIE_LEN		(S6E3HF4_MDNIE_0_LEN + S6E3HF4_MDNIE_1_LEN + S6E3HF4_MDNIE_2_LEN)

#define S6E3HF4_SCR_CB_OFS	(32)
#define S6E3HF4_SCR_WR_OFS	(50)
#define S6E3HF4_SCR_WG_OFS	(52)
#define S6E3HF4_SCR_WB_OFS	(54)
#define S6E3HF4_NIGHT_MODE_OFS	(S6E3HF4_SCR_CB_OFS)
#define S6E3HF4_NIGHT_MODE_LEN	(24)

#define S6E3HF4_TRANS_MODE_OFS	(17)
#define S6E3HF4_TRANS_MODE_LEN	(1)
#endif /* CONFIG_EXYNOS_DECON_MDNIE_LITE */

enum {
	GAMMA_MAPTBL,
	AOR_MAPTBL,
	AOR_INTER_MAPTBL,
	MPS_MAPTBL,
	TSET_MAPTBL,
	ELVSS_MAPTBL,
	ELVSS_TEMP_MAPTBL,
	VINT_MAPTBL,
	ACL_ONOFF_MAPTBL,
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
	READ_HBM_GAMMA_0,
	READ_HBM_GAMMA_1,
#ifdef CONFIG_SEC_FACTORY
	READ_OCTA_ID,
#endif
};

static u8 S6E3HF4_ID[3];
static u8 S6E3HF4_COORDINATE[4];
static u8 S6E3HF4_CODE[5];
static u8 S6E3HF4_ELVSS[23];
static u8 S6E3HF4_ELVSS_TEMP[1];
static u8 S6E3HF4_MTP[35];
static u8 S6E3HF4_DATE[7];
static u8 S6E3HF4_HBM_GAMMA[35];
#ifdef CONFIG_SEC_FACTORY
static u8 S6E3HF4_OCTA_ID[S6E3HF4_OCTA_ID_LEN];
#endif

static struct rdinfo s6e3hf4_rditbl[] = {
	RDINFO_INIT(id, DSI_PKT_TYPE_RD, S6E3HF4_ID_REG, S6E3HF4_ID_OFS, S6E3HF4_ID_LEN), // 1st ~ 3rd
	RDINFO_INIT(coordinate, DSI_PKT_TYPE_RD, S6E3HF4_COORDINATE_REG, S6E3HF4_COORDINATE_OFS, S6E3HF4_COORDINATE_LEN), // 1st ~ 4th
	RDINFO_INIT(code, DSI_PKT_TYPE_RD, S6E3HF4_CODE_REG, S6E3HF4_CODE_OFS, S6E3HF4_CODE_LEN),
	RDINFO_INIT(elvss, DSI_PKT_TYPE_RD, S6E3HF4_ELVSS_REG, S6E3HF4_ELVSS_OFS, S6E3HF4_ELVSS_LEN), // 1st ~ 23rd
	RDINFO_INIT(elvss_temp, DSI_PKT_TYPE_RD, S6E3HF4_ELVSS_TEMP_REG, S6E3HF4_ELVSS_TEMP_OFS, S6E3HF4_ELVSS_TEMP_LEN), // 1st ~ 23rd
	RDINFO_INIT(mtp, DSI_PKT_TYPE_RD, S6E3HF4_MTP_REG, S6E3HF4_MTP_OFS, S6E3HF4_MTP_LEN), // 1st ~ 35th
	RDINFO_INIT(date, DSI_PKT_TYPE_RD, S6E3HF4_DATE_REG, S6E3HF4_DATE_OFS, S6E3HF4_DATE_LEN), // 41st ~ 47th
	RDINFO_INIT(hbm_gamma_0, DSI_PKT_TYPE_RD, S6E3HF4_HBM_GAMMA_0_REG, S6E3HF4_HBM_GAMMA_0_OFS, S6E3HF4_HBM_GAMMA_0_LEN), // 51st ~ 65th
	RDINFO_INIT(hbm_gamma_1, DSI_PKT_TYPE_RD, S6E3HF4_HBM_GAMMA_1_REG, S6E3HF4_HBM_GAMMA_1_OFS, S6E3HF4_HBM_GAMMA_1_LEN), // 69th ~ 88th
#ifdef CONFIG_SEC_FACTORY
	RDINFO_INIT(octa_id, DSI_PKT_TYPE_RD, S6E3HF4_OCTA_ID_REG, S6E3HF4_OCTA_ID_OFS, S6E3HF4_OCTA_ID_LEN), // 69th ~ 88th
#endif
};

static DEFINE_RESUI(id, &s6e3hf4_rditbl[READ_ID], 0);
static DEFINE_RESUI(coordinate, &s6e3hf4_rditbl[READ_COORDINATE], 0);
static DEFINE_RESUI(code, &s6e3hf4_rditbl[READ_CODE], 0);
static DEFINE_RESUI(elvss, &s6e3hf4_rditbl[READ_ELVSS], 0);
static DEFINE_RESUI(elvss_temp, &s6e3hf4_rditbl[READ_ELVSS_TEMP], 0);
static DEFINE_RESUI(mtp, &s6e3hf4_rditbl[READ_MTP], 0);
static DEFINE_RESUI(date, &s6e3hf4_rditbl[READ_DATE], 0);
static struct res_update_info RESUI(hbm_gamma)[] = {
	RESUI_INIT(&s6e3hf4_rditbl[READ_HBM_GAMMA_0], 0),
	RESUI_INIT(&s6e3hf4_rditbl[READ_HBM_GAMMA_1], 15),
};
#ifdef CONFIG_SEC_FACTORY
static DEFINE_RESUI(octa_id, &s6e3hf4_rditbl[READ_OCTA_ID], 0);
#endif

static struct resinfo s6e3hf4_restbl[] = {
	RESINFO_INIT(id, S6E3HF4_ID, RESUI(id)),
	RESINFO_INIT(coordinate, S6E3HF4_COORDINATE, RESUI(coordinate)),
	RESINFO_INIT(code, S6E3HF4_CODE, RESUI(code)),
	RESINFO_INIT(elvss, S6E3HF4_ELVSS, RESUI(elvss)),
	RESINFO_INIT(elvss_temp, S6E3HF4_ELVSS_TEMP, RESUI(elvss_temp)),
	RESINFO_INIT(mtp, S6E3HF4_MTP, RESUI(mtp)),
	RESINFO_INIT(date, S6E3HF4_DATE, RESUI(date)),
	RESINFO_INIT(hbm_gamma, S6E3HF4_HBM_GAMMA, RESUI(hbm_gamma)),
#ifdef CONFIG_SEC_FACTORY
	RESINFO_INIT(octa_id, S6E3HF4_OCTA_ID, RESUI(octa_id)),
#endif
};

int init_common_table(struct maptbl *);
int init_gamma_table(struct maptbl *);
int getidx_common_maptbl(struct maptbl *);
int getidx_dimming_maptbl(struct maptbl *);
int getidx_aor_inter_table(struct maptbl *);
int getidx_mps_table(struct maptbl *);
int init_elvss_temp_table(struct maptbl *);
int getidx_elvss_temp_table(struct maptbl *);
int getidx_vint_table(struct maptbl *);
int getidx_acl_onoff_table(struct maptbl *);
int getidx_acl_opr_table(struct maptbl *);
int getidx_irc_table(struct maptbl *);
int getidx_dsc_table(struct maptbl *);
int getidx_resolution_table(struct maptbl *);
int getidx_lpm_table(struct maptbl *);
void copy_common_maptbl(struct maptbl *, u8 *);
void copy_tset_maptbl(struct maptbl *, u8 *);
#ifdef CONFIG_SUPPORT_HMD
int init_hmd_gamma_table(struct maptbl *);
int getidx_hmd_dimming_mtptbl(struct maptbl *);
#endif /* CONFIG_SUPPORT_HMD */
#ifdef CONFIG_EXYNOS_DECON_MDNIE_LITE
int init_color_blind_table(struct maptbl *tbl);
int getidx_mdnie_scenario_maptbl(struct maptbl *tbl);
int getidx_mdnie_hmd_maptbl(struct maptbl *tbl);
int getidx_mdnie_hdr_maptbl(struct maptbl *tbl);
void copy_mdnie_night_maptbl(struct maptbl *tbl, u8 *dst);
int getidx_color_coordinate_maptbl(struct maptbl *tbl);
void copy_color_coordinate_maptbl(struct maptbl *tbl, u8 *dst);
int getidx_adjust_ldu_maptbl(struct maptbl *tbl);
void copy_adjust_ldu_maptbl(struct maptbl *tbl, u8 *dst);
void copy_sensor_rgb_maptbl(struct maptbl *tbl, u8 *dst);
int getidx_mdnie_0_maptbl(struct pkt_update_info *pktui);
int getidx_mdnie_1_maptbl(struct pkt_update_info *pktui);
int getidx_mdnie_2_maptbl(struct pkt_update_info *pktui);
int getidx_mdnie_scr_white_maptbl(struct pkt_update_info *pktui);
void update_current_scr_white(struct maptbl *tbl, u8 *dst);
#endif /* CONFIG_EXYNOS_DECON_MDNIE_LITE */
#endif /* __S6E3HF4_H__ */
