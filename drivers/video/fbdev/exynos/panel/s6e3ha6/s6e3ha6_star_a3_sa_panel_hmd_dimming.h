/*
 * linux/drivers/video/fbdev/exynos/panel/s6e3ha6/s6e3ha6_star_a3_sa_panel_hmd_dimming.h
 *
 * Header file for S6E3HA6 Dimming Driver
 *
 * Copyright (c) 2016 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __S6E3HA6_STAR_A3_SA_PANEL_HMD_DIMMING_H__
#define __S6E3HA6_STAR_A3_SA_PANEL_HMD_DIMMING_H__
#include "../dimming.h"
#include "../panel_dimming.h"
#include "s6e3ha6_dimming.h"

/*
 * PANEL INFORMATION
 * LDI : S6E3HA6
 * PANEL : STAR_A3_SA
 */
static s32 star_a3_sa_hmd_rtbl10nit[11] = { 0, 0, 9, 4, 4, 3, 3, 0, 0, 0, 0 };
static s32 star_a3_sa_hmd_rtbl11nit[11] = { 0, 0, 9, 4, 3, 3, 3, 0, 0, 0, 0 };
static s32 star_a3_sa_hmd_rtbl12nit[11] = { 0, 0, 9, 4, 3, 3, 2, 0, 0, 0, 0 };
static s32 star_a3_sa_hmd_rtbl13nit[11] = { 0, 0, 9, 4, 3, 3, 2, 0, 0, -1, 0 };
static s32 star_a3_sa_hmd_rtbl14nit[11] = { 0, 0, 9, 4, 3, 3, 2, 0, 0, -1, 0 };
static s32 star_a3_sa_hmd_rtbl15nit[11] = { 0, 0, 9, 4, 3, 3, 1, 0, 0, -1, 0 };
static s32 star_a3_sa_hmd_rtbl16nit[11] = { 0, 0, 9, 4, 3, 3, 1, 0, 0, -1, 0 };
static s32 star_a3_sa_hmd_rtbl17nit[11] = { 0, 0, 9, 4, 3, 3, 0, 0, 0, -1, 0 };
static s32 star_a3_sa_hmd_rtbl19nit[11] = { 0, 0, 8, 4, 3, 2, 0, 0, 0, -1, 0 };
static s32 star_a3_sa_hmd_rtbl20nit[11] = { 0, 0, 9, 4, 3, 3, 0, 0, 0, 0, 0 };
static s32 star_a3_sa_hmd_rtbl21nit[11] = { 0, 0, 9, 4, 3, 3, 0, 0, 0, 0, 0 };
static s32 star_a3_sa_hmd_rtbl22nit[11] = { 0, 0, 4, 4, 3, 2, 0, 0, 0, 0, 0 };
static s32 star_a3_sa_hmd_rtbl23nit[11] = { 0, 0, 4, 4, 3, 3, 2, 0, 0, 0, 0 };
static s32 star_a3_sa_hmd_rtbl25nit[11] = { 0, 0, 4, 4, 3, 2, 0, 0, 0, 0, 0 };
static s32 star_a3_sa_hmd_rtbl27nit[11] = { 0, 0, 4, 3, 4, 3, 2, 0, 0, 2, 0 };
static s32 star_a3_sa_hmd_rtbl29nit[11] = { 0, 0, 4, 3, 3, 2, 0, 0, 0, 0, 0 };
static s32 star_a3_sa_hmd_rtbl31nit[11] = { 0, 0, 9, 4, 4, 2, 0, 0, 0, 0, 0 };
static s32 star_a3_sa_hmd_rtbl33nit[11] = { 0, 0, 4, 4, 3, 3, 0, 0, 0, 3, 0 };
static s32 star_a3_sa_hmd_rtbl35nit[11] = { 0, 0, 4, 4, 3, 0, 0, 0, 0, 0, 0 };
static s32 star_a3_sa_hmd_rtbl37nit[11] = { 0, 0, 4, 4, 3, 3, 0, 0, 0, 2, 0 };
static s32 star_a3_sa_hmd_rtbl39nit[11] = { 0, 0, 4, 3, 3, 2, 0, 0, 2, 2, 0 };
static s32 star_a3_sa_hmd_rtbl41nit[11] = { 0, 0, 7, 3, 3, 3, 0, 0, 2, 0, 0 };
static s32 star_a3_sa_hmd_rtbl44nit[11] = { 0, 0, 9, 3, 2, 2, 0, 2, 3, 0, 0 };
static s32 star_a3_sa_hmd_rtbl47nit[11] = { 0, 0, 8, 4, 3, 2, 2, 3, 4, 0, 0 };
static s32 star_a3_sa_hmd_rtbl50nit[11] = { 0, 0, 5, 3, 2, 0, 0, 0, 3, 0, 0 };
static s32 star_a3_sa_hmd_rtbl53nit[11] = { 0, 0, 4, 3, 3, 2, 2, 3, 3, 0, 0 };
static s32 star_a3_sa_hmd_rtbl56nit[11] = { 0, 0, 4, 4, 2, 2, 0, 2, 4, 2, 0 };
static s32 star_a3_sa_hmd_rtbl60nit[11] = { 0, 0, 4, 4, 2, 0, 0, 3, 3, 2, 0 };
static s32 star_a3_sa_hmd_rtbl64nit[11] = { 0, 0, 4, 4, 3, 2, 0, 2, 4, 2, 0 };
static s32 star_a3_sa_hmd_rtbl68nit[11] = { 0, 0, 4, 3, 2, 2, 2, 3, 5, 4, 0 };
static s32 star_a3_sa_hmd_rtbl72nit[11] = { 0, 0, 4, 3, 3, 0, 2, 4, 6, 4, 0 };
static s32 star_a3_sa_hmd_rtbl77nit[11] = { 0, 0, 1, 0, 0, 0, 0, 0, 2, 0, 0 };
static s32 star_a3_sa_hmd_rtbl82nit[11] = { 0, 0, 4, 0, 0, 0, 0, 0, 3, 0, 0 };
static s32 star_a3_sa_hmd_rtbl87nit[11] = { 0, 0, 4, 0, 0, 0, 0, 0, 3, 0, 0 };
static s32 star_a3_sa_hmd_rtbl93nit[11] = { 0, 0, 4, 0, 0, 0, 0, 0, 4, 3, 0 };
static s32 star_a3_sa_hmd_rtbl99nit[11] = { 0, 0, 2, 0, 0, 0, 0, 2, 4, 4, 0 };
static s32 star_a3_sa_hmd_rtbl105nit[11] = { 0, 0, 2, 0, 0, 0, 0, 3, 5, 4, 0 };

static s32 star_a3_sa_hmd_ctbl10nit[33] = { 0, 0, 0, 0, 0, 0, 2, 0, 7, 5, 0, 0, 2, 0, -5, -1, 1, -3, -3, 2, -4, -3, 1, -3, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
static s32 star_a3_sa_hmd_ctbl11nit[33] = { 0, 0, 0, 0, 0, 0, 2, 0, 6, 5, 0, 0, 2, 0, -5, -2, 1, -3, -4, 1, -4, -2, 1, -3, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
static s32 star_a3_sa_hmd_ctbl12nit[33] = { 0, 0, 0, 0, 0, 0, 2, 0, 5, 5, 0, 0, 2, 1, -4, -1, 1, -2, -5, 2, -5, -2, 1, -3, 1, 0, 0, 0, 0, 1, 1, 0, 1 };
static s32 star_a3_sa_hmd_ctbl13nit[33] = { 0, 0, 0, 0, 0, 0, 2, 0, 4, 5, 0, 0, 2, 1, -4, -1, 1, -2, -6, 2, -5, -1, 1, -3, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
static s32 star_a3_sa_hmd_ctbl14nit[33] = { 0, 0, 0, 0, 0, 0, 2, 0, 4, 5, 0, 0, 1, 1, -4, 0, 0, -2, -5, 2, -5, -1, 1, -2, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
static s32 star_a3_sa_hmd_ctbl15nit[33] = { 0, 0, 0, 0, 0, 0, 2, 0, 4, 5, 0, 0, 1, 2, -4, -2, 0, -2, -4, 2, -4, 0, 1, -2, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
static s32 star_a3_sa_hmd_ctbl16nit[33] = { 0, 0, 0, 0, 0, 0, 2, 0, 4, 5, 1, 0, 1, 2, -4, -2, 1, -3, -4, 1, -4, 0, 1, -2, 0, 0, 0, 0, 0, 1, 0, 0, 1 };
static s32 star_a3_sa_hmd_ctbl17nit[33] = { 0, 0, 0, 0, 0, 0, 2, 0, 4, 5, 1, -1, 0, 2, -4, -2, 1, -2, -5, 1, -4, 0, 1, -2, 0, 0, -1, 0, 0, 0, 0, 0, 1 };
static s32 star_a3_sa_hmd_ctbl19nit[33] = { 0, 0, 0, 0, 0, 0, 2, 0, 5, 5, 0, -1, 0, 2, -4, -3, 1, -3, -4, 1, -4, 0, 1, -2, 0, 0, 0, 0, 0, 0, 0, 0, 2 };
static s32 star_a3_sa_hmd_ctbl20nit[33] = { 0, 0, 0, 0, 0, 0, 2, 0, 5, 5, 0, -1, -1, 2, -4, -2, 1, -2, -5, 1, -4, 0, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 2 };
static s32 star_a3_sa_hmd_ctbl21nit[33] = { 0, 0, 0, 0, 0, 0, 2, 0, 5, 5, 0, -2, -1, 2, -4, -2, 1, -2, -5, 1, -4, 0, 0, -2, 0, 0, 0, 1, 0, 0, 0, 0, 2 };
static s32 star_a3_sa_hmd_ctbl22nit[33] = { 0, 0, 0, 0, 0, 0, 2, 0, 5, 5, 1, -2, -2, 0, -5, -2, 1, -3, -3, 1, -3, 0, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 2 };
static s32 star_a3_sa_hmd_ctbl23nit[33] = { 0, 0, 0, 0, 0, 0, 2, 0, 5, 5, 1, -2, -2, 1, -8, -3, 1, -4, -3, 1, -2, -2, 0, -2, 0, 0, 0, 1, 0, 0, 0, 0, 2 };
static s32 star_a3_sa_hmd_ctbl25nit[33] = { 0, 0, 0, 0, 0, 0, 2, 0, 5, 6, 0, -2, -2, 1, -7, -3, 1, -4, -3, 1, -4, -1, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 2 };
static s32 star_a3_sa_hmd_ctbl27nit[33] = { 0, 0, 0, 0, 0, 0, 3, 0, 4, 6, 1, -2, -2, 0, -6, -3, 1, -4, -2, 1, -2, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
static s32 star_a3_sa_hmd_ctbl29nit[33] = { 0, 0, 0, 0, 0, 0, 3, 0, 4, 6, 1, -2, -2, 0, -5, -4, 1, -4, -2, 1, -3, -1, 0, -2, 0, 0, 0, 0, 0, 0, 1, 0, 2 };
static s32 star_a3_sa_hmd_ctbl31nit[33] = { 0, 0, 0, 0, 0, 0, 4, 0, 4, 3, 2, -4, -2, 0, -4, -4, 1, -4, -2, 1, -3, -1, 0, -1, 0, 0, 0, 0, 0, 0, 1, 0, 2 };
static s32 star_a3_sa_hmd_ctbl33nit[33] = { 0, 0, 0, 0, 0, 0, 4, 0, 3, 3, 1, -4, -2, 1, -3, -2, 1, -3, -2, 1, -2, -2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
static s32 star_a3_sa_hmd_ctbl35nit[33] = { 0, 0, 0, 0, 0, 0, 4, 0, 3, 3, 2, -4, -2, 0, -3, -3, 1, -4, -2, 1, -3, -1, 0, -1, 0, 0, 0, 0, 0, 0, 1, 0, 2 };
static s32 star_a3_sa_hmd_ctbl37nit[33] = { 0, 0, 0, 0, 0, 0, 4, 0, 2, 2, 4, -5, -2, 3, -3, -3, 1, -3, -1, 1, -2, -1, 0, -1, 0, 0, 0, 0, 0, 0, 1, 0, 2 };
static s32 star_a3_sa_hmd_ctbl39nit[33] = { 0, 0, 0, 0, 0, 0, 4, 0, 2, 2, 4, -5, -2, 3, -3, -3, 1, -3, -1, 1, -2, -2, 0, -1, 0, 0, 0, 0, 0, 0, 1, 0, 2 };
static s32 star_a3_sa_hmd_ctbl41nit[33] = { 0, 0, 0, 0, 0, 0, 4, 1, 2, 2, 4, -5, -2, 3, -3, -2, 1, -2, -2, 1, -2, -1, 0, -1, 0, 0, 0, 0, 0, 0, 1, 0, 2 };
static s32 star_a3_sa_hmd_ctbl44nit[33] = { 0, 0, 0, 0, 0, 0, 4, 1, 2, 1, 3, -4, -2, 2, -3, -3, 1, -2, -2, 1, -2, 0, 0, -1, 0, 0, 0, -1, 0, 0, 1, 0, 2 };
static s32 star_a3_sa_hmd_ctbl47nit[33] = { 0, 0, 0, 0, 0, 0, 4, 1, 2, 1, 2, -4, -2, 2, -3, -4, 1, -3, -1, 1, -2, 0, 0, 0, -1, 0, 0, -1, 0, 0, 2, 0, 2 };
static s32 star_a3_sa_hmd_ctbl50nit[33] = { 0, 0, 0, 0, 0, 0, 4, 2, 2, 1, 1, -4, -2, 2, -3, -4, 1, -3, -1, 0, -2, -1, 0, -1, 0, 0, 0, -1, 0, 0, 2, 0, 2 };
static s32 star_a3_sa_hmd_ctbl53nit[33] = { 0, 0, 0, 0, 0, 0, 4, 2, 2, 1, 4, -5, -2, 2, -3, -3, 1, -2, -1, 0, -2, -1, 0, -1, 0, 0, 0, 0, 0, 0, 3, 0, 2 };
static s32 star_a3_sa_hmd_ctbl56nit[33] = { 0, 0, 0, 0, 0, 0, 3, 3, 2, 1, 4, -5, -2, 2, -4, -2, 0, -2, -2, 1, -2, -1, 0, -1, 0, 0, 0, 0, 0, 0, 3, 0, 3 };
static s32 star_a3_sa_hmd_ctbl60nit[33] = { 0, 0, 0, 0, 0, 0, 3, 3, 2, 0, 4, -5, -2, 2, -3, -3, 1, -2, -1, 1, -2, 0, 0, 0, -1, 0, 0, 0, 0, 0, 2, 0, 1 };
static s32 star_a3_sa_hmd_ctbl64nit[33] = { 0, 0, 0, 0, 0, 0, 2, 3, 2, 0, 4, -5, -2, 2, -4, -4, 1, -2, -2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 2 };
static s32 star_a3_sa_hmd_ctbl68nit[33] = { 0, 0, 0, 0, 0, 0, 2, 3, 2, 0, 4, -5, -2, 2, -4, -2, 1, -2, -2, 0, -2, 0, 0, 0, -1, 0, 0, 0, 0, 0, 4, 0, 3 };
static s32 star_a3_sa_hmd_ctbl72nit[33] = { 0, 0, 0, 0, 0, 0, 2, 3, 2, 1, 4, -6, -2, 2, -2, -3, 1, -2, 0, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 3 };
static s32 star_a3_sa_hmd_ctbl77nit[33] = { 0, 0, 0, 0, 0, 0, 5, 1, 5, 0, 1, -4, 0, 1, -2, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 3, 0, 2 };
static s32 star_a3_sa_hmd_ctbl82nit[33] = { 0, 0, 0, 0, 0, 0, 5, 1, 4, 3, 1, -3, 0, 1, -1, -2, 0, -2, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 2 };
static s32 star_a3_sa_hmd_ctbl87nit[33] = { 0, 0, 0, 0, 0, 0, 5, 1, 4, 4, 1, -3, 0, 1, -1, -3, 0, -2, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2 };
static s32 star_a3_sa_hmd_ctbl93nit[33] = { 0, 0, 0, 0, 0, 0, 5, 1, 4, 5, 1, -2, 0, 1, -2, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 2 };
static s32 star_a3_sa_hmd_ctbl99nit[33] = { 0, 0, 0, 0, 0, 0, 5, 1, 4, 0, 1, -3, 0, 1, -1, -2, 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 4, 0, 2 };
static s32 star_a3_sa_hmd_ctbl105nit[33] = { 0, 0, 0, 0, 0, 0, 5, 0, 5, 0, 2, -4, 0, 0, -1, -3, 0, -2, 0, 0, 0, 0, 0, 0, -2, 0, 0, 0, 0, 0, 4, 0, 3 };

struct dimming_lut star_a3_sa_hmd_dimming_lut[] = {
	DIM_LUT_V0_INIT(10, 41, GAMMA_2_15, star_a3_sa_hmd_rtbl10nit, star_a3_sa_hmd_ctbl10nit),
	DIM_LUT_V0_INIT(11, 46, GAMMA_2_15, star_a3_sa_hmd_rtbl11nit, star_a3_sa_hmd_ctbl11nit),
	DIM_LUT_V0_INIT(12, 51, GAMMA_2_15, star_a3_sa_hmd_rtbl12nit, star_a3_sa_hmd_ctbl12nit),
	DIM_LUT_V0_INIT(13, 56, GAMMA_2_15, star_a3_sa_hmd_rtbl13nit, star_a3_sa_hmd_ctbl13nit),
	DIM_LUT_V0_INIT(14, 61, GAMMA_2_15, star_a3_sa_hmd_rtbl14nit, star_a3_sa_hmd_ctbl14nit),
	DIM_LUT_V0_INIT(15, 66, GAMMA_2_15, star_a3_sa_hmd_rtbl15nit, star_a3_sa_hmd_ctbl15nit),
	DIM_LUT_V0_INIT(16, 70, GAMMA_2_15, star_a3_sa_hmd_rtbl16nit, star_a3_sa_hmd_ctbl16nit),
	DIM_LUT_V0_INIT(17, 74, GAMMA_2_15, star_a3_sa_hmd_rtbl17nit, star_a3_sa_hmd_ctbl17nit),
	DIM_LUT_V0_INIT(19, 83, GAMMA_2_15, star_a3_sa_hmd_rtbl19nit, star_a3_sa_hmd_ctbl19nit),
	DIM_LUT_V0_INIT(20, 87, GAMMA_2_15, star_a3_sa_hmd_rtbl20nit, star_a3_sa_hmd_ctbl20nit),
	DIM_LUT_V0_INIT(21, 90, GAMMA_2_15, star_a3_sa_hmd_rtbl21nit, star_a3_sa_hmd_ctbl21nit),
	DIM_LUT_V0_INIT(22, 96, GAMMA_2_15, star_a3_sa_hmd_rtbl22nit, star_a3_sa_hmd_ctbl22nit),
	DIM_LUT_V0_INIT(23, 98, GAMMA_2_15, star_a3_sa_hmd_rtbl23nit, star_a3_sa_hmd_ctbl23nit),
	DIM_LUT_V0_INIT(25, 108, GAMMA_2_15, star_a3_sa_hmd_rtbl25nit, star_a3_sa_hmd_ctbl25nit),
	DIM_LUT_V0_INIT(27, 115, GAMMA_2_15, star_a3_sa_hmd_rtbl27nit, star_a3_sa_hmd_ctbl27nit),
	DIM_LUT_V0_INIT(29, 123, GAMMA_2_15, star_a3_sa_hmd_rtbl29nit, star_a3_sa_hmd_ctbl29nit),
	DIM_LUT_V0_INIT(31, 132, GAMMA_2_15, star_a3_sa_hmd_rtbl31nit, star_a3_sa_hmd_ctbl31nit),
	DIM_LUT_V0_INIT(33, 140, GAMMA_2_15, star_a3_sa_hmd_rtbl33nit, star_a3_sa_hmd_ctbl33nit),
	DIM_LUT_V0_INIT(35, 147, GAMMA_2_15, star_a3_sa_hmd_rtbl35nit, star_a3_sa_hmd_ctbl35nit),
	DIM_LUT_V0_INIT(37, 154, GAMMA_2_15, star_a3_sa_hmd_rtbl37nit, star_a3_sa_hmd_ctbl37nit),
	DIM_LUT_V0_INIT(39, 164, GAMMA_2_15, star_a3_sa_hmd_rtbl39nit, star_a3_sa_hmd_ctbl39nit),
	DIM_LUT_V0_INIT(41, 173, GAMMA_2_15, star_a3_sa_hmd_rtbl41nit, star_a3_sa_hmd_ctbl41nit),
	DIM_LUT_V0_INIT(44, 183, GAMMA_2_15, star_a3_sa_hmd_rtbl44nit, star_a3_sa_hmd_ctbl44nit),
	DIM_LUT_V0_INIT(47, 197, GAMMA_2_15, star_a3_sa_hmd_rtbl47nit, star_a3_sa_hmd_ctbl47nit),
	DIM_LUT_V0_INIT(50, 208, GAMMA_2_15, star_a3_sa_hmd_rtbl50nit, star_a3_sa_hmd_ctbl50nit),
	DIM_LUT_V0_INIT(53, 220, GAMMA_2_15, star_a3_sa_hmd_rtbl53nit, star_a3_sa_hmd_ctbl53nit),
	DIM_LUT_V0_INIT(56, 231, GAMMA_2_15, star_a3_sa_hmd_rtbl56nit, star_a3_sa_hmd_ctbl56nit),
	DIM_LUT_V0_INIT(60, 246, GAMMA_2_15, star_a3_sa_hmd_rtbl60nit, star_a3_sa_hmd_ctbl60nit),
	DIM_LUT_V0_INIT(64, 259, GAMMA_2_15, star_a3_sa_hmd_rtbl64nit, star_a3_sa_hmd_ctbl64nit),
	DIM_LUT_V0_INIT(68, 271, GAMMA_2_15, star_a3_sa_hmd_rtbl68nit, star_a3_sa_hmd_ctbl68nit),
	DIM_LUT_V0_INIT(72, 282, GAMMA_2_15, star_a3_sa_hmd_rtbl72nit, star_a3_sa_hmd_ctbl72nit),
	DIM_LUT_V0_INIT(77, 218, GAMMA_2_15, star_a3_sa_hmd_rtbl77nit, star_a3_sa_hmd_ctbl77nit),
	DIM_LUT_V0_INIT(82, 230, GAMMA_2_15, star_a3_sa_hmd_rtbl82nit, star_a3_sa_hmd_ctbl82nit),
	DIM_LUT_V0_INIT(87, 241, GAMMA_2_15, star_a3_sa_hmd_rtbl87nit, star_a3_sa_hmd_ctbl87nit),
	DIM_LUT_V0_INIT(93, 257, GAMMA_2_15, star_a3_sa_hmd_rtbl93nit, star_a3_sa_hmd_ctbl93nit),
	DIM_LUT_V0_INIT(99, 269, GAMMA_2_15, star_a3_sa_hmd_rtbl99nit, star_a3_sa_hmd_ctbl99nit),
	DIM_LUT_V0_INIT(105, 280, GAMMA_2_15, star_a3_sa_hmd_rtbl105nit, star_a3_sa_hmd_ctbl105nit),
};
static unsigned int star_a3_sa_hmd_brt_tbl[S6E3HA6_HMD_NR_LUMINANCE] = {
	BRT(0), BRT(27), BRT(30), BRT(32), BRT(34), BRT(37), BRT(39), BRT(42), BRT(47), BRT(49),
	BRT(51), BRT(54), BRT(56), BRT(61), BRT(66), BRT(71), BRT(76), BRT(81), BRT(85), BRT(90),
	BRT(95), BRT(100), BRT(107), BRT(115), BRT(122), BRT(129), BRT(136), BRT(146), BRT(156), BRT(166),
	BRT(175), BRT(187), BRT(200), BRT(212), BRT(226), BRT(241), BRT(255),
};

static unsigned int star_a3_sa_hmd_lum_tbl[S6E3HA6_HMD_NR_LUMINANCE] = {
	10, 11, 12, 13, 14, 15, 16, 17, 19, 20,
	21, 22, 23, 25, 27, 29, 31, 33, 35, 37,
	39, 41, 44, 47, 50, 53, 56, 60, 64, 68,
	72, 77, 82, 87, 93, 99, 105,
};

struct brightness_table s6e3ha6_star_a3_sa_panel_hmd_brightness_table = {
	.brt = star_a3_sa_hmd_brt_tbl,
	.sz_brt = ARRAY_SIZE(star_a3_sa_hmd_brt_tbl),
	.lum = star_a3_sa_hmd_lum_tbl,
	.sz_lum = ARRAY_SIZE(star_a3_sa_hmd_lum_tbl),
	.sz_ui_lum = S6E3HA6_HMD_NR_LUMINANCE,
	.sz_hbm_lum = 0,
	.sz_ext_hbm_lum = 0,
	.brt_to_step = star_a3_sa_hmd_brt_tbl,
	.sz_brt_to_step = ARRAY_SIZE(star_a3_sa_hmd_brt_tbl),
};

static struct panel_dimming_info s6e3ha6_star_a3_sa_panel_hmd_dimming_info = {
	.name = "s6e3ha6_star_a3_sa_hmd",
	.dim_init_info = {
		.name = "s6e3ha6_star_a3_sa_hmd",
		.nr_tp = S6E3HA6_NR_TP,
		.tp = s6e3ha6_tp,
		.nr_luminance = S6E3HA6_HMD_NR_LUMINANCE,
		.vregout = 114085069LL,	/* 6.8 * 2^24 */
		.bitshift = 24,
		.vt_voltage = {
			0, 12, 24, 36, 48, 60, 72, 84, 96, 108,
			138, 148, 158, 168, 178, 186,
		},
		.target_luminance = S6E3HA6_HMD_TARGET_LUMINANCE,
		.target_gamma = 220,
		.dim_lut = star_a3_sa_hmd_dimming_lut,
	},
	.target_luminance = S6E3HA6_HMD_TARGET_LUMINANCE,
	.nr_luminance = S6E3HA6_HMD_NR_LUMINANCE,
	.hbm_target_luminance = -1,
	.nr_hbm_luminance = 0,
	.extend_hbm_target_luminance = -1,
	.nr_extend_hbm_luminance = 0,
	.brt_tbl = &s6e3ha6_star_a3_sa_panel_hmd_brightness_table,
};
#endif /* __S6E3HA6_STAR_A3_SA_PANEL_HMD_DIMMING_H__ */
