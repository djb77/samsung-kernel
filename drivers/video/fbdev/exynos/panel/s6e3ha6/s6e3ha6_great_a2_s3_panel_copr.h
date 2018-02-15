/*
 * linux/drivers/video/fbdev/exynos/panel/s6e3ha6/s6e3ha6_great_a2_s3_panel_copr.h
 *
 * Header file for COPR Driver
 *
 * Copyright (c) 2016 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __S6E3HA6_GREAT_A2_S3_PANEL_COPR_H__
#define __S6E3HA6_GREAT_A2_S3_PANEL_COPR_H__

#include "../panel.h"
#include "../copr.h"
#include "s6e3ha6_great_a2_s3_panel.h"

#define S6E3HA6_GREAT_A2_S3_COPR_GAMMA	(1)		/* 0 : GAMMA_1, 1 : GAMMA_2_2 */
#define S6E3HA6_GREAT_A2_S3_COPR_ER		(0x5D)
#define S6E3HA6_GREAT_A2_S3_COPR_EG		(0x6E)
#define S6E3HA6_GREAT_A2_S3_COPR_EB		(0xB5)

static struct pktinfo PKTINFO(great_a2_s3_level2_key_enable);
static struct pktinfo PKTINFO(great_a2_s3_level2_key_disable);

/* ===================================================================================== */
/* ============================== [S6E3HA6 MAPPING TABLE] ============================== */
/* ===================================================================================== */
struct maptbl great_a2_s3_copr_maptbl[] = {
	[COPR_MAPTBL] = DEFINE_0D_MAPTBL(great_a2_s3_copr_table, init_common_table, NULL, copy_copr_maptbl),
};

/* ===================================================================================== */
/* ============================== [S6E3HA6 COMMAND TABLE] ============================== */
/* ===================================================================================== */
u8 GREAT_A2_S3_COPR[] = {
	0xE1, 0x03, 0x5D, 0x6E, 0xB5, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

DEFINE_VARIABLE_PACKET(great_a2_s3_copr, DSI_PKT_TYPE_WR, GREAT_A2_S3_COPR, &great_a2_s3_copr_maptbl[COPR_MAPTBL], 1);

static void *great_a2_s3_set_copr_cmdtbl[] = {
	&PKTINFO(great_a2_s3_level2_key_enable),
	&PKTINFO(great_a2_s3_copr),
	&PKTINFO(great_a2_s3_level2_key_disable),
};

static void *great_a2_s3_get_copr_cmdtbl[] = {
	&s6e3ha6_restbl[RES_COPR],
};

static struct seqinfo great_a2_s3_copr_seqtbl[MAX_COPR_SEQ] = {
	[COPR_SET_SEQ] = SEQINFO_INIT("set-copr-seq", great_a2_s3_set_copr_cmdtbl),
	[COPR_GET_SEQ] = SEQINFO_INIT("get-copr-seq", great_a2_s3_get_copr_cmdtbl),
};

static struct panel_copr_data s6e3ha6_great_a2_s3_copr_data = {
	.seqtbl = great_a2_s3_copr_seqtbl,
	.nr_seqtbl = ARRAY_SIZE(great_a2_s3_copr_seqtbl),
	.maptbl = (struct maptbl *)great_a2_s3_copr_maptbl,
	.nr_maptbl = (sizeof(great_a2_s3_copr_maptbl) / sizeof(struct maptbl)),
	.reg = {
		.copr_en = true,
		.copr_gamma = S6E3HA6_GREAT_A2_S3_COPR_GAMMA,
		.copr_er = S6E3HA6_GREAT_A2_S3_COPR_ER,
		.copr_eg = S6E3HA6_GREAT_A2_S3_COPR_EG,
		.copr_eb = S6E3HA6_GREAT_A2_S3_COPR_EB,
		.roi_on = false,
		.roi_xs = 0,
		.roi_ys = 0,
		.roi_xe = 0,
		.roi_ye = 0,
	},
};
#endif /* __S6E3HA6_GREAT_A2_S3_PANEL_COPR_H__ */
