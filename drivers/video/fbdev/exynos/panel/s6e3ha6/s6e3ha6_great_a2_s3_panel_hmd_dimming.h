/*
 * linux/drivers/video/fbdev/exynos/panel/s6e3ha6/s6e3ha6_great_a2_s3_panel_hmd_dimming.h
 *
 * Header file for S6E3HA6 Dimming Driver
 *
 * Copyright (c) 2016 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __S6E3HA6_GREAT_A2_S3_PANEL_HMD_DIMMING_H__
#define __S6E3HA6_GREAT_A2_S3_PANEL_HMD_DIMMING_H__
#include "../dimming.h"
#include "../panel_dimming.h"
#include "s6e3ha6_dimming.h"

/*
 * PANEL INFORMATION
 * LDI : S6E3HA6
 * PANEL : GREAT_A2_S3
 */
static s32 great_a2_s3_hmd_rtbl10nit[11] = { 0, 0, 6, 7, 5, 4, 4, 1, -2, -2, 0 };
static s32 great_a2_s3_hmd_rtbl11nit[11] = { 0, 0, 7, 7, 5, 5, 4, 0, -3, -3, 0 };
static s32 great_a2_s3_hmd_rtbl12nit[11] = { 0, 0, 7, 7, 5, 4, 3, 1, -2, -3, 0 };
static s32 great_a2_s3_hmd_rtbl13nit[11] = { 0, 0, 7, 6, 5, 4, 3, 1, -3, -4, 0 };
static s32 great_a2_s3_hmd_rtbl14nit[11] = { 0, 0, 7, 6, 5, 4, 3, 1, -2, -3, 0 };
static s32 great_a2_s3_hmd_rtbl15nit[11] = { 0, 0, 7, 6, 5, 5, 4, 0, -3, -4, 0 };
static s32 great_a2_s3_hmd_rtbl16nit[11] = { 0, 0, 7, 6, 5, 4, 3, 1, -2, -4, 0 };
static s32 great_a2_s3_hmd_rtbl17nit[11] = { 0, 0, 7, 7, 5, 4, 3, 0, -2, -5, 0 };
static s32 great_a2_s3_hmd_rtbl19nit[11] = { 0, 0, 6, 6, 5, 4, 3, 0, -3, -5, 0 };
static s32 great_a2_s3_hmd_rtbl20nit[11] = { 0, 0, 6, 6, 4, 3, 3, -1, -3, -5, 0 };
static s32 great_a2_s3_hmd_rtbl21nit[11] = { 0, 0, 7, 6, 4, 4, 3, -1, -3, -5, 0 };
static s32 great_a2_s3_hmd_rtbl22nit[11] = { 0, 0, 7, 6, 5, 4, 2, -1, -4, -5, 0 };
static s32 great_a2_s3_hmd_rtbl23nit[11] = { 0, 0, 7, 7, 5, 4, 3, -1, -3, -5, 0 };
static s32 great_a2_s3_hmd_rtbl25nit[11] = { 0, 0, 7, 6, 4, 4, 3, 0, -4, -5, 0 };
static s32 great_a2_s3_hmd_rtbl27nit[11] = { 0, 0, 7, 7, 5, 4, 3, -1, -4, -5, 0 };
static s32 great_a2_s3_hmd_rtbl29nit[11] = { 0, 0, 7, 6, 4, 3, 2, 0, -4, -4, 0 };
static s32 great_a2_s3_hmd_rtbl31nit[11] = { 0, 0, 7, 6, 4, 4, 2, -1, -4, -4, 0 };
static s32 great_a2_s3_hmd_rtbl33nit[11] = { 0, 0, 6, 6, 5, 3, 2, -1, -5, -5, 0 };
static s32 great_a2_s3_hmd_rtbl35nit[11] = { 0, 0, 7, 6, 4, 4, 2, -1, -5, -4, 0 };
static s32 great_a2_s3_hmd_rtbl37nit[11] = { 0, 0, 7, 6, 4, 3, 2, 0, -4, -4, 0 };
static s32 great_a2_s3_hmd_rtbl39nit[11] = { 0, 0, 7, 5, 4, 4, 2, -1, -3, -4, 0 };
static s32 great_a2_s3_hmd_rtbl41nit[11] = { 0, 0, 7, 5, 4, 3, 2, 0, -3, -4, 0 };
static s32 great_a2_s3_hmd_rtbl44nit[11] = { 0, 0, 7, 6, 4, 3, 3, 0, -3, -4, 0 };
static s32 great_a2_s3_hmd_rtbl47nit[11] = { 0, 0, 7, 6, 3, 3, 2, 0, -2, -3, 0 };
static s32 great_a2_s3_hmd_rtbl50nit[11] = { 0, 0, 7, 6, 4, 4, 2, 2, -2, -1, 0 };
static s32 great_a2_s3_hmd_rtbl53nit[11] = { 0, 0, 7, 6, 4, 4, 3, 2, -1, -3, 0 };
static s32 great_a2_s3_hmd_rtbl56nit[11] = { 0, 0, 6, 5, 3, 3, 2, 1, -2, -4, 0 };
static s32 great_a2_s3_hmd_rtbl60nit[11] = { 0, 0, 6, 5, 4, 3, 3, 2, 0, -3, 0 };
static s32 great_a2_s3_hmd_rtbl64nit[11] = { 0, 0, 7, 6, 3, 3, 3, 2, 0, -3, 0 };
static s32 great_a2_s3_hmd_rtbl68nit[11] = { 0, 0, 7, 6, 4, 4, 3, 2, -1, -4, 0 };
static s32 great_a2_s3_hmd_rtbl72nit[11] = { 0, 0, 7, 5, 3, 3, 3, 3, 0, -3, 0 };
static s32 great_a2_s3_hmd_rtbl77nit[11] = { 0, 0, 4, 3, 2, 1, 0, -1, -3, -4, 0 };
static s32 great_a2_s3_hmd_rtbl82nit[11] = { 0, 0, 3, 2, 1, 1, 0, -1, -3, -5, 0 };
static s32 great_a2_s3_hmd_rtbl87nit[11] = { 0, 0, 3, 2, 2, 1, 0, -1, -3, -5, 0 };
static s32 great_a2_s3_hmd_rtbl93nit[11] = { 0, 0, 3, 2, 1, 0, 1, -1, -2, -4, 0 };
static s32 great_a2_s3_hmd_rtbl99nit[11] = { 0, 0, 3, 2, 2, 1, 0, 0, -2, -5, 0 };
static s32 great_a2_s3_hmd_rtbl105nit[11] = { 0, 0, 3, 2, 1, 1, 0, 0, -2, -3, 0 };

static s32 great_a2_s3_hmd_ctbl10nit[33] = { 0, 0, 0, 0, 0, 0, 5, 0, 0, 3, 2, -5, -1, 2, -4, -1, 1, -4, -4, 2, -4, -2, 1, -3, 0, 0, -1, 0, 0, 0, 1, 0, 2 };
static s32 great_a2_s3_hmd_ctbl11nit[33] = { 0, 0, 0, 0, 0, 0, 1, 0, -1, 2, 3, -6, -1, 1, -4, -1, 1, -4, -3, 2, -4, -2, 1, -3, 0, 0, 0, 0, 0, 0, 1, 0, 2 };
static s32 great_a2_s3_hmd_ctbl12nit[33] = { 0, 0, 0, 0, 0, 0, 0, 0, -1, 1, 3, -7, -2, 1, -4, -1, 1, -4, -4, 2, -5, -1, 1, -2, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
static s32 great_a2_s3_hmd_ctbl13nit[33] = { 0, 0, 0, 0, 0, 0, 0, 0, -1, 3, 3, -8, -1, 1, -4, -1, 1, -4, -4, 2, -5, -3, 1, -2, -1, 0, -2, 0, 0, 0, 1, 0, 2 };
static s32 great_a2_s3_hmd_ctbl14nit[33] = { 0, 0, 0, 0, 0, 0, 0, 0, -2, 3, 3, -7, -2, 2, -4, -1, 1, -4, -3, 2, -4, -3, 1, -2, 0, 0, -1, 0, 0, 0, 0, 0, 1 };
static s32 great_a2_s3_hmd_ctbl15nit[33] = { 0, 0, 0, 0, 0, 0, 1, 0, 0, 2, 3, -8, -2, 2, -4, -1, 1, -4, -4, 1, -4, -2, 1, -2, 0, 0, -1, 0, 0, 0, 1, 0, 2 };
static s32 great_a2_s3_hmd_ctbl16nit[33] = { 0, 0, 0, 0, 0, 0, 1, 0, 0, 3, 3, -7, -2, 2, -4, -1, 2, -4, -4, 1, -4, -2, 0, -2, 0, 0, -1, 0, 0, 0, 1, 0, 2 };
static s32 great_a2_s3_hmd_ctbl17nit[33] = { 0, 0, 0, 0, 0, 0, 2, 1, -2, 0, 2, -6, -2, 2, -4, -1, 1, -4, -4, 2, -4, -1, 1, -2, 0, 0, -1, 0, 0, 0, 1, 0, 2 };
static s32 great_a2_s3_hmd_ctbl19nit[33] = { 0, 0, 0, 0, 0, 0, 2, 1, -3, 1, 2, -5, -2, 2, -4, -1, 2, -4, -4, 1, -4, -1, 0, -2, 0, 0, 0, 0, 0, 0, 2, 0, 2 };
static s32 great_a2_s3_hmd_ctbl20nit[33] = { 0, 0, 0, 0, 0, 0, 3, 1, -2, 0, 3, -6, -1, 2, -4, -2, 1, -4, -4, 1, -4, -2, 1, -2, 0, 0, -1, 0, 0, 0, 1, 0, 2 };
static s32 great_a2_s3_hmd_ctbl21nit[33] = { 0, 0, 0, 0, 0, 0, 0, 1, -2, 0, 2, -6, -2, 2, -4, -2, 1, -4, -4, 1, -4, -2, 0, -2, 0, 0, 0, 0, 0, 0, 2, 0, 2 };
static s32 great_a2_s3_hmd_ctbl22nit[33] = { 0, 0, 0, 0, 0, 0, 0, 0, -2, 0, 3, -7, -3, 1, -4, -1, 1, -3, -4, 1, -4, -1, 0, -2, 1, 0, 0, 0, 0, 0, 2, 0, 2 };
static s32 great_a2_s3_hmd_ctbl23nit[33] = { 0, 0, 0, 0, 0, 0, 2, 1, -3, 1, 2, -6, -2, 1, -4, -2, 1, -4, -5, 1, -4, -1, 0, -2, 0, 0, -1, 0, 0, 0, 1, 0, 3 };
static s32 great_a2_s3_hmd_ctbl25nit[33] = { 0, 0, 0, 0, 0, 0, 0, 0, -2, 0, 3, -6, -2, 1, -4, -2, 1, -4, -4, 1, -3, -1, 1, -2, 0, 0, -1, 0, 0, -1, 3, 0, 4 };
static s32 great_a2_s3_hmd_ctbl27nit[33] = { 0, 0, 0, 0, 0, 0, 0, 1, -4, 1, 3, -6, -3, 1, -4, -2, 1, -4, -4, 1, -2, -2, 0, -2, 0, 0, -1, 0, 0, 0, 2, 0, 3 };
static s32 great_a2_s3_hmd_ctbl29nit[33] = { 0, 0, 0, 0, 0, 0, 0, 1, -4, 0, 3, -7, -3, 1, -4, -2, 1, -4, -3, 1, -2, 0, 1, -2, 0, 0, -1, 0, 0, 0, 1, 0, 2 };
static s32 great_a2_s3_hmd_ctbl31nit[33] = { 0, 0, 0, 0, 0, 0, 0, 1, -4, 0, 3, -6, -2, 2, -4, -2, 1, -2, -3, 1, -3, 0, 0, -1, 0, 0, 0, 0, 0, 0, 2, 0, 3 };
static s32 great_a2_s3_hmd_ctbl33nit[33] = { 0, 0, 0, 0, 0, 0, 0, 1, -4, 0, 3, -7, -2, 1, -4, -4, 1, -4, -3, 1, -2, -2, 0, -2, 0, 0, -1, 0, 0, -1, 3, 1, 4 };
static s32 great_a2_s3_hmd_ctbl35nit[33] = { 0, 0, 0, 0, 0, 0, 0, 1, -2, -1, 3, -7, -2, 1, -4, -2, 1, -4, -4, 1, -2, -1, 0, -2, 0, 0, -1, 1, 0, 0, 2, 0, 3 };
static s32 great_a2_s3_hmd_ctbl37nit[33] = { 0, 0, 0, 0, 0, 0, 0, 0, -2, 0, 3, -7, -3, 1, -4, -2, 1, -3, -2, 1, -2, -2, 0, -2, 0, 0, -1, 1, 0, 0, 2, 0, 3 };
static s32 great_a2_s3_hmd_ctbl39nit[33] = { 0, 0, 0, 0, 0, 0, 0, 1, -2, 0, 3, -6, -3, 2, -4, -1, 1, -3, -4, 0, -2, -1, 0, -2, 0, 0, -1, 0, 0, 1, 3, 0, 3 };
static s32 great_a2_s3_hmd_ctbl41nit[33] = { 0, 0, 0, 0, 0, 0, -1, 0, -2, 0, 3, -8, -2, 1, -4, -2, 1, -3, -4, 1, -3, 0, 0, -1, 0, 0, -1, 0, 0, 0, 2, 0, 4 };
static s32 great_a2_s3_hmd_ctbl44nit[33] = { 0, 0, 0, 0, 0, 0, -1, 1, -3, -2, 3, -7, -2, 1, -3, -1, 1, -2, -3, 0, -2, -1, 0, -2, 0, 0, -1, 0, 0, 0, 3, 0, 3 };
static s32 great_a2_s3_hmd_ctbl47nit[33] = { 0, 0, 0, 0, 0, 0, -1, 1, -3, -1, 3, -6, -2, 1, -3, -1, 1, -2, -3, 1, -2, 0, 0, -2, 0, 0, 0, 0, 0, 0, 3, 0, 4 };
static s32 great_a2_s3_hmd_ctbl50nit[33] = { 0, 0, 0, 0, 0, 0, 0, 1, -3, -2, 3, -7, -3, 2, -4, -1, 1, -2, -3, 0, -2, -1, 0, -1, 0, 0, -1, 0, 0, 0, 3, 0, 4 };
static s32 great_a2_s3_hmd_ctbl53nit[33] = { 0, 0, 0, 0, 0, 0, 0, 1, -2, -3, 3, -8, -2, 2, -4, -2, 1, -2, -3, 0, -2, -1, 0, -2, -1, 0, -2, 0, 0, 0, 4, 0, 5 };
static s32 great_a2_s3_hmd_ctbl56nit[33] = { 0, 0, 0, 0, 0, 0, -1, 1, -3, -2, 3, -7, -2, 2, -4, -2, 1, -2, -2, 1, -2, 0, 0, -1, 0, 0, 0, 0, 0, 0, 3, 0, 4 };
static s32 great_a2_s3_hmd_ctbl60nit[33] = { 0, 0, 0, 0, 0, 0, -1, 1, -3, -1, 3, -7, -3, 2, -4, -1, 1, -2, -2, 1, -2, 0, 0, 0, -1, 0, -2, 0, 0, 0, 3, 0, 4 };
static s32 great_a2_s3_hmd_ctbl64nit[33] = { 0, 0, 0, 0, 0, 0, 0, 1, -2, -1, 3, -7, -3, 1, -4, -1, 1, -2, -3, 0, -2, -1, 0, -1, 0, 0, -1, 0, 0, 0, 4, 0, 4 };
static s32 great_a2_s3_hmd_ctbl68nit[33] = { 0, 0, 0, 0, 0, 0, 0, 1, -2, -1, 3, -7, -3, 1, -4, -2, 1, -2, -3, 0, -2, -1, 0, -2, 0, 0, 0, 0, 0, 0, 5, 0, 4 };
static s32 great_a2_s3_hmd_ctbl72nit[33] = { 0, 0, 0, 0, 0, 0, -1, 1, -2, -1, 3, -7, -3, 1, -4, -2, 1, -2, -3, 0, -2, 0, 0, -1, 0, 0, -1, 0, 0, 0, 5, 0, 5 };
static s32 great_a2_s3_hmd_ctbl77nit[33] = { 0, 0, 0, 0, 0, 0, 4, 0, 1, -2, 2, -6, -1, 1, -2, -1, 0, -1, -3, 0, -2, 0, 0, -1, 0, 0, -1, 0, 0, 0, 4, 0, 5 };
static s32 great_a2_s3_hmd_ctbl82nit[33] = { 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 2, -5, -2, 0, -2, -1, 0, -1, -3, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 4 };
static s32 great_a2_s3_hmd_ctbl87nit[33] = { 0, 0, 0, 0, 0, 0, 3, 0, 0, -2, 2, -6, -2, 0, -2, -1, 0, -1, -2, 0, -1, 0, 0, -1, -1, 0, -1, 0, 0, 0, 4, 0, 5 };
static s32 great_a2_s3_hmd_ctbl93nit[33] = { 0, 0, 0, 0, 0, 0, 2, 0, 0, -1, 2, -6, -2, 0, -2, -1, 0, -1, -2, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 4, 0, 4 };
static s32 great_a2_s3_hmd_ctbl99nit[33] = { 0, 0, 0, 0, 0, 0, 3, 0, 0, -1, 2, -5, -1, 1, -2, -1, 0, -1, -2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 1, 5, 0, 5 };
static s32 great_a2_s3_hmd_ctbl105nit[33] = { 0, 0, 0, 0, 0, 0, 3, 0, -2, -2, 2, -5, -1, 0, -2, -1, 0, -2, -1, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, 5, 0, 5 };

struct dimming_lut great_a2_s3_hmd_dimming_lut[] = {
	DIM_LUT_V0_INIT(10, 41, GAMMA_2_15, great_a2_s3_hmd_rtbl10nit, great_a2_s3_hmd_ctbl10nit),
	DIM_LUT_V0_INIT(11, 46, GAMMA_2_15, great_a2_s3_hmd_rtbl11nit, great_a2_s3_hmd_ctbl11nit),
	DIM_LUT_V0_INIT(12, 50, GAMMA_2_15, great_a2_s3_hmd_rtbl12nit, great_a2_s3_hmd_ctbl12nit),
	DIM_LUT_V0_INIT(13, 55, GAMMA_2_15, great_a2_s3_hmd_rtbl13nit, great_a2_s3_hmd_ctbl13nit),
	DIM_LUT_V0_INIT(14, 58, GAMMA_2_15, great_a2_s3_hmd_rtbl14nit, great_a2_s3_hmd_ctbl14nit),
	DIM_LUT_V0_INIT(15, 63, GAMMA_2_15, great_a2_s3_hmd_rtbl15nit, great_a2_s3_hmd_ctbl15nit),
	DIM_LUT_V0_INIT(16, 66, GAMMA_2_15, great_a2_s3_hmd_rtbl16nit, great_a2_s3_hmd_ctbl16nit),
	DIM_LUT_V0_INIT(17, 71, GAMMA_2_15, great_a2_s3_hmd_rtbl17nit, great_a2_s3_hmd_ctbl17nit),
	DIM_LUT_V0_INIT(19, 80, GAMMA_2_15, great_a2_s3_hmd_rtbl19nit, great_a2_s3_hmd_ctbl19nit),
	DIM_LUT_V0_INIT(20, 83, GAMMA_2_15, great_a2_s3_hmd_rtbl20nit, great_a2_s3_hmd_ctbl20nit),
	DIM_LUT_V0_INIT(21, 87, GAMMA_2_15, great_a2_s3_hmd_rtbl21nit, great_a2_s3_hmd_ctbl21nit),
	DIM_LUT_V0_INIT(22, 92, GAMMA_2_15, great_a2_s3_hmd_rtbl22nit, great_a2_s3_hmd_ctbl22nit),
	DIM_LUT_V0_INIT(23, 97, GAMMA_2_15, great_a2_s3_hmd_rtbl23nit, great_a2_s3_hmd_ctbl23nit),
	DIM_LUT_V0_INIT(25, 104, GAMMA_2_15, great_a2_s3_hmd_rtbl25nit, great_a2_s3_hmd_ctbl25nit),
	DIM_LUT_V0_INIT(27, 113, GAMMA_2_15, great_a2_s3_hmd_rtbl27nit, great_a2_s3_hmd_ctbl27nit),
	DIM_LUT_V0_INIT(29, 120, GAMMA_2_15, great_a2_s3_hmd_rtbl29nit, great_a2_s3_hmd_ctbl29nit),
	DIM_LUT_V0_INIT(31, 129, GAMMA_2_15, great_a2_s3_hmd_rtbl31nit, great_a2_s3_hmd_ctbl31nit),
	DIM_LUT_V0_INIT(33, 135, GAMMA_2_15, great_a2_s3_hmd_rtbl33nit, great_a2_s3_hmd_ctbl33nit),
	DIM_LUT_V0_INIT(35, 142, GAMMA_2_15, great_a2_s3_hmd_rtbl35nit, great_a2_s3_hmd_ctbl35nit),
	DIM_LUT_V0_INIT(37, 151, GAMMA_2_15, great_a2_s3_hmd_rtbl37nit, great_a2_s3_hmd_ctbl37nit),
	DIM_LUT_V0_INIT(39, 156, GAMMA_2_15, great_a2_s3_hmd_rtbl39nit, great_a2_s3_hmd_ctbl39nit),
	DIM_LUT_V0_INIT(41, 163, GAMMA_2_15, great_a2_s3_hmd_rtbl41nit, great_a2_s3_hmd_ctbl41nit),
	DIM_LUT_V0_INIT(44, 174, GAMMA_2_15, great_a2_s3_hmd_rtbl44nit, great_a2_s3_hmd_ctbl44nit),
	DIM_LUT_V0_INIT(47, 186, GAMMA_2_15, great_a2_s3_hmd_rtbl47nit, great_a2_s3_hmd_ctbl47nit),
	DIM_LUT_V0_INIT(50, 198, GAMMA_2_15, great_a2_s3_hmd_rtbl50nit, great_a2_s3_hmd_ctbl50nit),
	DIM_LUT_V0_INIT(53, 204, GAMMA_2_15, great_a2_s3_hmd_rtbl53nit, great_a2_s3_hmd_ctbl53nit),
	DIM_LUT_V0_INIT(56, 216, GAMMA_2_15, great_a2_s3_hmd_rtbl56nit, great_a2_s3_hmd_ctbl56nit),
	DIM_LUT_V0_INIT(60, 230, GAMMA_2_15, great_a2_s3_hmd_rtbl60nit, great_a2_s3_hmd_ctbl60nit),
	DIM_LUT_V0_INIT(64, 244, GAMMA_2_15, great_a2_s3_hmd_rtbl64nit, great_a2_s3_hmd_ctbl64nit),
	DIM_LUT_V0_INIT(68, 255, GAMMA_2_15, great_a2_s3_hmd_rtbl68nit, great_a2_s3_hmd_ctbl68nit),
	DIM_LUT_V0_INIT(72, 266, GAMMA_2_15, great_a2_s3_hmd_rtbl72nit, great_a2_s3_hmd_ctbl72nit),
	DIM_LUT_V0_INIT(77, 204, GAMMA_2_15, great_a2_s3_hmd_rtbl77nit, great_a2_s3_hmd_ctbl77nit),
	DIM_LUT_V0_INIT(82, 217, GAMMA_2_15, great_a2_s3_hmd_rtbl82nit, great_a2_s3_hmd_ctbl82nit),
	DIM_LUT_V0_INIT(87, 229, GAMMA_2_15, great_a2_s3_hmd_rtbl87nit, great_a2_s3_hmd_ctbl87nit),
	DIM_LUT_V0_INIT(93, 244, GAMMA_2_15, great_a2_s3_hmd_rtbl93nit, great_a2_s3_hmd_ctbl93nit),
	DIM_LUT_V0_INIT(99, 257, GAMMA_2_15, great_a2_s3_hmd_rtbl99nit, great_a2_s3_hmd_ctbl99nit),
	DIM_LUT_V0_INIT(105, 268, GAMMA_2_15, great_a2_s3_hmd_rtbl105nit, great_a2_s3_hmd_ctbl105nit),
};

static unsigned int great_a2_s3_hmd_brt_tbl[S6E3HA6_HMD_NR_LUMINANCE] = {
	BRT(0), BRT(27), BRT(30), BRT(32), BRT(34), BRT(37), BRT(39), BRT(42), BRT(47), BRT(49),
	BRT(51), BRT(54), BRT(56), BRT(61), BRT(66), BRT(71), BRT(76), BRT(81), BRT(85), BRT(90),
	BRT(95), BRT(100), BRT(107), BRT(115), BRT(122), BRT(129), BRT(136), BRT(146), BRT(156), BRT(166),
	BRT(175), BRT(187), BRT(200), BRT(212), BRT(226), BRT(241), BRT(255),
};

static unsigned int great_a2_s3_hmd_lum_tbl[S6E3HA6_HMD_NR_LUMINANCE] = {
	10, 11, 12, 13, 14, 15, 16, 17, 19, 20,
	21, 22, 23, 25, 27, 29, 31, 33, 35, 37,
	39, 41, 44, 47, 50, 53, 56, 60, 64, 68,
	72, 77, 82, 87, 93, 99, 105,
};

struct brightness_table s6e3ha6_great_a2_s3_panel_hmd_brightness_table = {
	.brt = great_a2_s3_hmd_brt_tbl,
	.sz_brt = ARRAY_SIZE(great_a2_s3_hmd_brt_tbl),
	.lum = great_a2_s3_hmd_lum_tbl,
	.sz_lum = ARRAY_SIZE(great_a2_s3_hmd_lum_tbl),
	.sz_ui_lum = S6E3HA6_HMD_NR_LUMINANCE,
	.sz_hbm_lum = 0,
	.sz_ext_hbm_lum = 0,
	.brt_to_step = great_a2_s3_hmd_brt_tbl,
	.sz_brt_to_step = ARRAY_SIZE(great_a2_s3_hmd_brt_tbl),
};

static struct panel_dimming_info s6e3ha6_great_a2_s3_panel_hmd_dimming_info = {
	.name = "s6e3ha6_great_a2_s3_hmd",
	.dim_init_info = {
		.name = "s6e3ha6_great_a2_s3_hmd",
		.nr_tp = S6E3HA6_NR_TP,
		.tp = s6e3ha6_tp,
		.nr_luminance = S6E3HA6_HMD_NR_LUMINANCE,
		.vregout = 109051904LL,	/* 6.5 * 2^24*/
		.bitshift = 24,
		.vt_voltage = {
			0, 12, 24, 36, 48, 60, 72, 84, 96, 108,
			138, 148, 158, 168, 178, 186,
		},
		.target_luminance = S6E3HA6_HMD_TARGET_LUMINANCE,
		.target_gamma = 220,
		.dim_lut = great_a2_s3_hmd_dimming_lut,
	},
	.target_luminance = S6E3HA6_HMD_TARGET_LUMINANCE,
	.nr_luminance = S6E3HA6_HMD_NR_LUMINANCE,
	.hbm_target_luminance = -1,
	.nr_hbm_luminance = 0,
	.extend_hbm_target_luminance = -1,
	.nr_extend_hbm_luminance = 0,
	.brt_tbl = &s6e3ha6_great_a2_s3_panel_hmd_brightness_table,
};
#endif /* __S6E3HA6_GREAT_A2_S3_PANEL_HMD_DIMMING_H__ */
