/*
 * linux/drivers/video/fbdev/exynos/panel/s6e3ha6/s6e3ha6_star_a3_sa_panel_copr.h
 *
 * Header file for COPR Driver
 *
 * Copyright (c) 2016 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __S6E3HA6_STAR_A3_SA_PANEL_COPR_H__
#define __S6E3HA6_STAR_A3_SA_PANEL_COPR_H__

#include "../panel.h"
#include "../copr.h"
#include "s6e3ha6_star_a3_sa_panel.h"

#define S6E3HA6_STAR_A3_SA_COPR_GAMMA	(1)		/* 0 : GAMMA_1, 1 : GAMMA_2_2 */
#define S6E3HA6_STAR_A3_SA_COPR_ER		(0x5D)
#define S6E3HA6_STAR_A3_SA_COPR_EG		(0x6E)
#define S6E3HA6_STAR_A3_SA_COPR_EB		(0xB5)

static struct pktinfo PKTINFO(star_a3_sa_level2_key_enable);
static struct pktinfo PKTINFO(star_a3_sa_level2_key_disable);

/* ===================================================================================== */
/* ============================== [S6E3HA6 MAPPING TABLE] ============================== */
/* ===================================================================================== */
struct maptbl star_a3_sa_copr_maptbl[] = {
	[COPR_MAPTBL] = DEFINE_0D_MAPTBL(star_a3_sa_copr_table, init_common_table, NULL, copy_copr_maptbl),
};

/* ===================================================================================== */
/* ============================== [S6E3HA6 COMMAND TABLE] ============================== */
/* ===================================================================================== */
u8 STAR_A3_SA_COPR[] = {
	0xE1, 0x03, 0x5D, 0x6E, 0xB5, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static DEFINE_PKTUI(star_a3_sa_copr, &star_a3_sa_copr_maptbl[COPR_MAPTBL], 1);
static DEFINE_VARIABLE_PACKET(star_a3_sa_copr, DSI_PKT_TYPE_WR, STAR_A3_SA_COPR, 0);

static void *star_a3_sa_set_copr_cmdtbl[] = {
	&PKTINFO(star_a3_sa_level2_key_enable),
	&PKTINFO(star_a3_sa_copr),
	&PKTINFO(star_a3_sa_level2_key_disable),
};

static void *star_a3_sa_get_copr_cmdtbl[] = {
	&s6e3ha6_restbl[RES_COPR],
};

static struct seqinfo star_a3_sa_copr_seqtbl[MAX_COPR_SEQ] = {
	[COPR_SET_SEQ] = SEQINFO_INIT("set-copr-seq", star_a3_sa_set_copr_cmdtbl),
	[COPR_GET_SEQ] = SEQINFO_INIT("get-copr-seq", star_a3_sa_get_copr_cmdtbl),
};

static struct panel_copr_data s6e3ha6_star_a3_sa_copr_data = {
	.seqtbl = star_a3_sa_copr_seqtbl,
	.nr_seqtbl = ARRAY_SIZE(star_a3_sa_copr_seqtbl),
	.maptbl = (struct maptbl *)star_a3_sa_copr_maptbl,
	.nr_maptbl = (sizeof(star_a3_sa_copr_maptbl) / sizeof(struct maptbl)),
	.reg = {
		.copr_en = true,
		.copr_gamma = S6E3HA6_STAR_A3_SA_COPR_GAMMA,
		.copr_er = S6E3HA6_STAR_A3_SA_COPR_ER,
		.copr_eg = S6E3HA6_STAR_A3_SA_COPR_EG,
		.copr_eb = S6E3HA6_STAR_A3_SA_COPR_EB,
		.roi_on = false,
		.roi_xs = 0,
		.roi_ys = 0,
		.roi_xe = 0,
		.roi_ye = 0,
	},
};
#endif /* __S6E3HA6_STAR_A3_SA_PANEL_COPR_H__ */
