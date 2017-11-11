/*
 * linux/drivers/video/fbdev/exynos/panel/s6e3hf4/s6e3hf4_a3_da_panel_hmd_dimming.h
 *
 * Header file for S6E3HF4 Dimming Driver
 *
 * Copyright (c) 2016 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __S6E3HF4_A3_DA_PANEL_HMD_DIMMING_H__
#define __S6E3HF4_A3_DA_PANEL_HMD_DIMMING_H__
#include "../dimming.h"
#include "../panel_dimming.h"
#include "s6e3hf4_dimming.h"

/*
 * PANEL INFORMATION
 * LDI : S6E3HF4
 * PANEL : A3 DA
 */
static s32 a3_da_hmd_rtbl10nit[] = {0, 0, 12, 11, 9, 7, 6, 4, 0, -1, 0};
static s32 a3_da_hmd_rtbl11nit[] = {0, 0, 12, 11, 9, 8, 6, 4, 1, -2, 0};
static s32 a3_da_hmd_rtbl12nit[] = {0, 0, 12, 11, 10, 7, 6, 4, 1, -2, 0};
static s32 a3_da_hmd_rtbl13nit[] = {0, 0, 12, 11, 9, 8, 6, 4, 0, -2, 0};
static s32 a3_da_hmd_rtbl14nit[] = {0, 0, 12, 11, 9, 7, 6, 3, 1, -2, 0};
static s32 a3_da_hmd_rtbl15nit[] = {0, 0, 12, 11, 9, 8, 6, 4, 1, -2, 0};
static s32 a3_da_hmd_rtbl16nit[] = {0, 0, 12, 11, 8, 7, 5, 3, 1, -2, 0};
static s32 a3_da_hmd_rtbl17nit[] = {0, 0, 12, 11, 9, 7, 6, 3, 0, -2, 0};
static s32 a3_da_hmd_rtbl19nit[] = {0, 0, 12, 11, 8, 7, 5, 2, -1, -1, 0};
static s32 a3_da_hmd_rtbl20nit[] = {0, 0, 12, 11, 8, 8, 6, 3, 0, -1, 0};
static s32 a3_da_hmd_rtbl21nit[] = {0, 0, 12, 11, 8, 7, 5, 3, -1, -1, 0};
static s32 a3_da_hmd_rtbl22nit[] = {0, 0, 12, 11, 9, 7, 5, 3, 0, 0, 0};
static s32 a3_da_hmd_rtbl23nit[] = {0, 0, 12, 11, 8, 7, 5, 3, 0, 0, 0};
static s32 a3_da_hmd_rtbl25nit[] = {0, 0, 12, 11, 8, 7, 5, 3, 0, 1, 0};
static s32 a3_da_hmd_rtbl27nit[] = {0, 0, 12, 11, 7, 7, 5, 3, 0, 0, 0};
static s32 a3_da_hmd_rtbl29nit[] = {0, 0, 12, 11, 8, 7, 5, 4, 0, -1, 0};
static s32 a3_da_hmd_rtbl31nit[] = {0, 0, 12, 10, 7, 7, 5, 4, 1, 0, 0};
static s32 a3_da_hmd_rtbl33nit[] = {0, 0, 12, 10, 7, 6, 5, 4, 1, 0, 0};
static s32 a3_da_hmd_rtbl35nit[] = {0, 0, 12, 10, 8, 6, 6, 4, 2, 0, 0};
static s32 a3_da_hmd_rtbl37nit[] = {0, 0, 12, 10, 7, 7, 6, 5, 2, -1, 0};
static s32 a3_da_hmd_rtbl39nit[] = {0, 0, 13, 11, 8, 6, 5, 4, 1, -2, 0};
static s32 a3_da_hmd_rtbl41nit[] = {0, 0, 13, 11, 8, 7, 5, 4, 2, -1, 0};
static s32 a3_da_hmd_rtbl44nit[] = {0, 0, 12, 10, 7, 6, 5, 4, 3, -1, 0};
static s32 a3_da_hmd_rtbl47nit[] = {0, 0, 12, 10, 8, 6, 6, 5, 2, -1, 0};
static s32 a3_da_hmd_rtbl50nit[] = {0, 0, 12, 10, 7, 7, 6, 5, 4, -1, 0};
static s32 a3_da_hmd_rtbl53nit[] = {0, 0, 12, 10, 8, 6, 5, 4, 3, -1, 0};
static s32 a3_da_hmd_rtbl56nit[] = {0, 0, 12, 10, 7, 6, 5, 5, 3, -1, 0};
static s32 a3_da_hmd_rtbl60nit[] = {0, 0, 12, 10, 8, 7, 5, 5, 4, 0, 0};
static s32 a3_da_hmd_rtbl64nit[] = {0, 0, 12, 10, 7, 7, 5, 5, 4, 0, 0};
static s32 a3_da_hmd_rtbl68nit[] = {0, 0, 11, 10, 8, 7, 5, 5, 4, 0, 0};
static s32 a3_da_hmd_rtbl72nit[] = {0, 0, 11, 9, 7, 6, 6, 5, 4, 0, 0};
static s32 a3_da_hmd_rtbl77nit[] = {0, 0, 8, 7, 4, 4, 3, 3, 2, -3, 0};
static s32 a3_da_hmd_rtbl82nit[] = {0, 0, 8, 6, 4, 4, 3, 4, 2, -2, 0};
static s32 a3_da_hmd_rtbl87nit[] = {0, 0, 8, 6, 4, 4, 3, 4, 3, -1, 0};
static s32 a3_da_hmd_rtbl93nit[] = {0, 0, 8, 6, 4, 4, 3, 4, 3, -1, 0};
static s32 a3_da_hmd_rtbl99nit[] = {0, 0, 8, 7, 5, 4, 4, 4, 4, 1, 0};
static s32 a3_da_hmd_rtbl105nit[] = {0, 0, 8, 7, 4, 5, 4, 4, 4, 0, 0};

static s32 a3_da_hmd_ctbl10nit[] = {0, 0, 0, 0, 0, 0, 6, -12, 1, -9, 6, -21, -3, 6, -12, -7, 6, -14, -8, 6, -12, -2, 3, -6, -1, 0, -1, 0, 0, 0, 0, 0, 0};
static s32 a3_da_hmd_ctbl11nit[] = {0, 0, 0, 0, 0, 0, 10, -8, 1, -7, 8, -21, -4, 7, -15, -6, 6, -13, -6, 6, -12, -5, 2, -6, -2, 0, -2, 0, 0, 0, 0, 0, 0};
static s32 a3_da_hmd_ctbl12nit[] = {0, 0, 0, 0, 0, 0, 12, -6, -2, -5, 10, -21, -5, 4, -10, -7, 6, -14, -6, 6, -12, -4, 2, -5, -2, 0, -1, 0, 0, -1, 0, 0, 0};
static s32 a3_da_hmd_ctbl13nit[] = {0, 0, 0, 0, 0, 0, 12, -6, -2, -5, 10, -21, -5, 5, -12, -6, 6, -12, -7, 5, -12, -3, 2, -4, -2, 0, -1, 0, 0, 0, 0, 0, 0};
static s32 a3_da_hmd_ctbl14nit[] = {0, 0, 0, 0, 0, 0, 2, 0, -2, -11, 9, -19, -4, 5, -12, -7, 6, -13, -6, 5, -11, -3, 2, -4, 0, 0, -1, 0, 0, 0, 0, 0, 1};
static s32 a3_da_hmd_ctbl15nit[] = {0, 0, 0, 0, 0, 0, 0, 0, -4, -15, 5, -22, -5, 6, -14, -6, 5, -12, -5, 5, -11, -4, 2, -5, -1, 0, -1, 0, 0, 0, 0, 0, 0};
static s32 a3_da_hmd_ctbl16nit[] = {0, 0, 0, 0, 0, 0, 0, 0, -4, -15, 5, -22, -5, 6, -14, -6, 5, -12, -6, 5, -11, -3, 2, -4, -1, 0, -1, 0, 0, 0, 0, 0, 1};
static s32 a3_da_hmd_ctbl17nit[] = {0, 0, 0, 0, 0, 0, 2, 2, -4, -15, 7, -22, -7, 5, -12, -6, 6, -12, -6, 4, -10, -3, 1, -4, -1, 0, -1, 0, 0, 0, 0, 0, 1};
static s32 a3_da_hmd_ctbl19nit[] = {0, 0, 0, 0, 0, 0, 2, 2, -4, -15, 7, -22, -7, 6, -14, -6, 5, -12, -6, 4, -10, -3, 1, -3, 0, 0, -2, 1, 0, 0, 0, 0, 2};
static s32 a3_da_hmd_ctbl20nit[] = {0, 0, 0, 0, 0, 0, 2, 3, -4, -17, 5, -23, -5, 6, -14, -7, 5, -11, -5, 4, -9, -3, 1, -4, 0, 0, -1, 0, 0, 0, 0, 0, 1};
static s32 a3_da_hmd_ctbl21nit[] = {0, 0, 0, 0, 0, 0, 1, 2, -5, -17, 5, -23, -5, 7, -14, -6, 5, -11, -6, 4, -9, -3, 1, -4, -1, 0, -1, 0, 0, 0, 1, 0, 2};
static s32 a3_da_hmd_ctbl22nit[] = {0, 0, 0, 0, 0, 0, 6, 8, -1, -18, 4, -24, -7, 5, -12, -7, 5, -11, -4, 3, -8, -2, 1, -3, -1, 0, -1, 0, 0, 0, 1, 0, 2};
static s32 a3_da_hmd_ctbl23nit[] = {0, 0, 0, 0, 0, 0, 6, 8, -1, -18, 4, -24, -8, 6, -13, -6, 5, -11, -3, 3, -7, -2, 1, -3, 0, 0, 0, 0, 0, 0, 0, 0, 1};
static s32 a3_da_hmd_ctbl25nit[] = {0, 0, 0, 0, 0, 0, 6, 8, -1, -17, 5, -23, -7, 6, -13, -7, 5, -11, -3, 3, -6, -1, 1, -3, 0, 0, -1, 0, 0, 0, 0, 0, 2};
static s32 a3_da_hmd_ctbl27nit[] = {0, 0, 0, 0, 0, 0, -6, 0, -9, -16, 6, -22, -8, 7, -14, -6, 4, -10, -3, 3, -6, -2, 1, -2, -1, 0, -1, 0, 0, 0, 1, 0, 2};
static s32 a3_da_hmd_ctbl29nit[] = {0, 0, 0, 0, 0, 0, -6, 1, -9, -14, 8, -20, -7, 6, -12, -6, 4, -9, -3, 2, -6, -2, 1, -3, 0, 0, 0, 0, 0, 0, 1, 0, 2};
static s32 a3_da_hmd_ctbl31nit[] = {0, 0, 0, 0, 0, 0, -4, 1, -7, -14, 8, -20, -7, 6, -13, -7, 4, -10, -4, 2, -6, -1, 1, -2, 0, 0, -1, 0, 0, 0, 1, 0, 2};
static s32 a3_da_hmd_ctbl33nit[] = {0, 0, 0, 0, 0, 0, -4, 1, -8, -14, 8, -21, -7, 6, -13, -6, 4, -10, -3, 2, -6, -1, 0, -2, 0, 0, 0, 0, 0, 0, 1, 0, 3};
static s32 a3_da_hmd_ctbl35nit[] = {0, 0, 0, 0, 0, 0, -5, 0, -8, -14, 8, -22, -7, 5, -12, -5, 4, -9, -3, 2, -5, -1, 1, -2, 0, 0, -1, 0, 0, 0, 1, 0, 2};
static s32 a3_da_hmd_ctbl37nit[] = {0, 0, 0, 0, 0, 0, -4, 2, -8, -14, 8, -23, -7, 6, -13, -5, 4, -8, -3, 2, -5, -1, 0, -2, -1, 0, -1, 0, 0, 0, 1, 0, 3};
static s32 a3_da_hmd_ctbl39nit[] = {0, 0, 0, 0, 0, 0, -4, 0, -10, -18, 5, -25, -7, 5, -10, -5, 3, -8, -4, 2, -6, 0, 0, -1, -1, 0, -1, 0, 0, 0, 2, 0, 3};
static s32 a3_da_hmd_ctbl41nit[] = {0, 0, 0, 0, 0, 0, -4, 0, -10, -17, 6, -24, -5, 5, -11, -6, 4, -8, -3, 2, -5, -2, 0, -2, 0, 0, 0, 0, 0, 0, 3, 0, 3};
static s32 a3_da_hmd_ctbl44nit[] = {0, 0, 0, 0, 0, 0, -7, -1, -13, -17, 7, -24, -6, 5, -10, -6, 4, -8, -3, 2, -5, -1, 0, -1, -1, 0, -1, 1, 0, 0, 3, 0, 4};
static s32 a3_da_hmd_ctbl47nit[] = {0, 0, 0, 0, 0, 0, -6, -1, -13, -16, 7, -23, -6, 5, -11, -5, 4, -8, -3, 1, -4, -2, 0, -2, -2, 0, -1, 1, 0, 0, 3, 0, 4};
static s32 a3_da_hmd_ctbl50nit[] = {0, 0, 0, 0, 0, 0, -6, -1, -12, -16, 7, -22, -5, 5, -11, -5, 3, -8, -3, 2, -4, -1, 0, -1, -1, 0, 0, 0, 0, 0, 3, 0, 4};
static s32 a3_da_hmd_ctbl53nit[] = {0, 0, 0, 0, 0, 0, -6, 3, -11, -16, 8, -21, -6, 4, -10, -5, 3, -8, -1, 1, -3, -2, 0, -2, 0, 0, 0, 0, 0, 0, 3, 0, 4};
static s32 a3_da_hmd_ctbl56nit[] = {0, 0, 0, 0, 0, 0, -5, 5, -11, -16, 5, -21, -6, 5, -10, -5, 3, -8, -2, 1, -4, -2, 0, -2, 0, 0, 0, 0, 0, 0, 4, 0, 4};
static s32 a3_da_hmd_ctbl60nit[] = {0, 0, 0, 0, 0, 0, -5, 6, -11, -16, 5, -21, -6, 4, -10, -4, 3, -6, -3, 1, -4, -1, 0, -1, 0, 0, 0, 1, 0, 0, 2, 0, 3};
static s32 a3_da_hmd_ctbl64nit[] = {0, 0, 0, 0, 0, 0, -5, 6, -10, -15, 5, -20, -4, 4, -9, -5, 3, -6, -2, 1, -4, -2, 0, -1, 0, 0, 0, 0, 0, 0, 3, 0, 4};
static s32 a3_da_hmd_ctbl68nit[] = {0, 0, 0, 0, 0, 0, -5, 6, -10, -15, 5, -20, -5, 4, -9, -4, 2, -6, -1, 1, -3, -1, 0, 0, 0, 0, -1, 0, 0, 0, 3, 0, 4};
static s32 a3_da_hmd_ctbl72nit[] = {0, 0, 0, 0, 0, 0, -5, 6, -10, -13, 5, -20, -5, 4, -8, -3, 3, -6, -2, 1, -3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 4};
static s32 a3_da_hmd_ctbl77nit[] = {0, 0, 0, 0, 0, 0, -5, 4, -9, -8, 4, -19, -5, 4, -7, -2, 2, -4, -3, 1, -3, -1, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 3};
static s32 a3_da_hmd_ctbl82nit[] = {0, 0, 0, 0, 0, 0, -3, 4, -9, -6, 6, -18, -6, 3, -8, -3, 2, -5, -2, 1, -3, 0, 0, -1, -1, 0, 0, 0, 0, 0, 3, 0, 3};
static s32 a3_da_hmd_ctbl87nit[] = {0, 0, 0, 0, 0, 0, -4, 4, -11, -6, 5, -18, -6, 3, -8, -3, 2, -5, -1, 1, -2, -1, 0, -1, 0, 0, 0, 1, 0, 0, 3, 0, 4};
static s32 a3_da_hmd_ctbl93nit[] = {0, 0, 0, 0, 0, 0, -4, 4, -12, -7, 7, -16, -4, 3, -7, -2, 2, -5, -1, 1, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 3};
static s32 a3_da_hmd_ctbl99nit[] = {0, 0, 0, 0, 0, 0, -7, 0, -15, -9, 4, -18, -4, 3, -6, -2, 2, -5, -1, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 3};
static s32 a3_da_hmd_ctbl105nit[] = {0, 0, 0, 0, 0, 0, -7, -1, -15, -9, 7, -16, -4, 2, -5, -2, 1, -4, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 3, 0, 4};

struct dimming_lut a3_da_panel_hmd_dimming_lut[] = {
	DIM_LUT_INIT(10, 55, GAMMA_2_15, a3_da_hmd_rtbl10nit, a3_da_hmd_ctbl10nit),
	DIM_LUT_INIT(11, 60, GAMMA_2_15, a3_da_hmd_rtbl11nit, a3_da_hmd_ctbl11nit),
	DIM_LUT_INIT(12, 66, GAMMA_2_15, a3_da_hmd_rtbl12nit, a3_da_hmd_ctbl12nit),
	DIM_LUT_INIT(13, 70, GAMMA_2_15, a3_da_hmd_rtbl13nit, a3_da_hmd_ctbl13nit),
	DIM_LUT_INIT(14, 75, GAMMA_2_15, a3_da_hmd_rtbl14nit, a3_da_hmd_ctbl14nit),
	DIM_LUT_INIT(15, 81, GAMMA_2_15, a3_da_hmd_rtbl15nit, a3_da_hmd_ctbl15nit),
	DIM_LUT_INIT(16, 86, GAMMA_2_15, a3_da_hmd_rtbl16nit, a3_da_hmd_ctbl16nit),
	DIM_LUT_INIT(17, 91, GAMMA_2_15, a3_da_hmd_rtbl17nit, a3_da_hmd_ctbl17nit),
	DIM_LUT_INIT(19, 101, GAMMA_2_15, a3_da_hmd_rtbl19nit, a3_da_hmd_ctbl19nit),
	DIM_LUT_INIT(20, 105, GAMMA_2_15, a3_da_hmd_rtbl20nit, a3_da_hmd_ctbl20nit),
	DIM_LUT_INIT(21, 109, GAMMA_2_15, a3_da_hmd_rtbl21nit, a3_da_hmd_ctbl21nit),
	DIM_LUT_INIT(22, 114, GAMMA_2_15, a3_da_hmd_rtbl22nit, a3_da_hmd_ctbl22nit),
	DIM_LUT_INIT(23, 120, GAMMA_2_15, a3_da_hmd_rtbl23nit, a3_da_hmd_ctbl23nit),
	DIM_LUT_INIT(25, 126, GAMMA_2_15, a3_da_hmd_rtbl25nit, a3_da_hmd_ctbl25nit),
	DIM_LUT_INIT(27, 139, GAMMA_2_15, a3_da_hmd_rtbl27nit, a3_da_hmd_ctbl27nit),
	DIM_LUT_INIT(29, 147, GAMMA_2_15, a3_da_hmd_rtbl29nit, a3_da_hmd_ctbl29nit),
	DIM_LUT_INIT(31, 157, GAMMA_2_15, a3_da_hmd_rtbl31nit, a3_da_hmd_ctbl31nit),
	DIM_LUT_INIT(33, 165, GAMMA_2_15, a3_da_hmd_rtbl33nit, a3_da_hmd_ctbl33nit),
	DIM_LUT_INIT(35, 174, GAMMA_2_15, a3_da_hmd_rtbl35nit, a3_da_hmd_ctbl35nit),
	DIM_LUT_INIT(37, 183, GAMMA_2_15, a3_da_hmd_rtbl37nit, a3_da_hmd_ctbl37nit),
	DIM_LUT_INIT(39, 194, GAMMA_2_15, a3_da_hmd_rtbl39nit, a3_da_hmd_ctbl39nit),
	DIM_LUT_INIT(41, 202, GAMMA_2_15, a3_da_hmd_rtbl41nit, a3_da_hmd_ctbl41nit),
	DIM_LUT_INIT(44, 214, GAMMA_2_15, a3_da_hmd_rtbl44nit, a3_da_hmd_ctbl44nit),
	DIM_LUT_INIT(47, 227, GAMMA_2_15, a3_da_hmd_rtbl47nit, a3_da_hmd_ctbl47nit),
	DIM_LUT_INIT(50, 240, GAMMA_2_15, a3_da_hmd_rtbl50nit, a3_da_hmd_ctbl50nit),
	DIM_LUT_INIT(53, 252, GAMMA_2_15, a3_da_hmd_rtbl53nit, a3_da_hmd_ctbl53nit),
	DIM_LUT_INIT(56, 266, GAMMA_2_15, a3_da_hmd_rtbl56nit, a3_da_hmd_ctbl56nit),
	DIM_LUT_INIT(60, 280, GAMMA_2_15, a3_da_hmd_rtbl60nit, a3_da_hmd_ctbl60nit),
	DIM_LUT_INIT(64, 295, GAMMA_2_15, a3_da_hmd_rtbl64nit, a3_da_hmd_ctbl64nit),
	DIM_LUT_INIT(68, 311, GAMMA_2_15, a3_da_hmd_rtbl68nit, a3_da_hmd_ctbl68nit),
	DIM_LUT_INIT(72, 326, GAMMA_2_15, a3_da_hmd_rtbl72nit, a3_da_hmd_ctbl72nit),
	DIM_LUT_INIT(77, 252, GAMMA_2_15, a3_da_hmd_rtbl77nit, a3_da_hmd_ctbl77nit),
	DIM_LUT_INIT(82, 266, GAMMA_2_15, a3_da_hmd_rtbl82nit, a3_da_hmd_ctbl82nit),
	DIM_LUT_INIT(87, 277, GAMMA_2_15, a3_da_hmd_rtbl87nit, a3_da_hmd_ctbl87nit),
	DIM_LUT_INIT(93, 291, GAMMA_2_15, a3_da_hmd_rtbl93nit, a3_da_hmd_ctbl93nit),
	DIM_LUT_INIT(99, 309, GAMMA_2_15, a3_da_hmd_rtbl99nit, a3_da_hmd_ctbl99nit),
	DIM_LUT_INIT(105, 322, GAMMA_2_15, a3_da_hmd_rtbl105nit, a3_da_hmd_ctbl105nit),
};

static unsigned int a3_da_panel_hmd_brt_tbl[S6E3HF4_MAX_BRIGHTNESS + 1] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 1, 1, 2, 2, 3, 3, 4, 4, 4, 5, 5, 6, 6, 6, 7, 7, 7, 7, 7, 8, 8, 9, 9, 10, 10,
	10, 11, 11, 12, 12, 12, 12, 12, 13, 13, 13, 13, 13, 14, 14, 14, 14, 14, 15, 15,
	15, 15, 15, 16, 16, 16, 16, 16, 17, 17, 17, 17, 18, 18, 18, 18, 18, 19, 19, 19,
	19, 19, 20, 20, 20, 20, 20, 21, 21, 21, 21, 21, 21, 21, 22, 22, 22, 22, 22, 22,
	22, 22, 23, 23, 23, 23, 23, 23, 23, 24, 24, 24, 24, 24, 24, 24, 25, 25, 25, 25,
	25, 25, 25, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 27, 27, 27, 27, 27, 27, 27,
	27, 27, 27, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 29, 29, 29, 29, 29, 29, 29,
	29, 29, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 31, 31, 31, 31, 31, 31,
	31, 31, 31, 31, 31, 31, 31, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 33,
	33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 34, 34, 34, 34, 34, 34, 34,
	34, 34, 34, 34, 34, 34, 34, 34, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35,
	35, 35, 36,
	[UI_MAX_BRIGHTNESS + 1 ... S6E3HF4_MAX_BRIGHTNESS] = 36,
};

static unsigned int a3_da_panel_hmd_lum_tbl[S6E3HF4_HMD_NR_LUMINANCE] = {
	10, 11, 12, 13, 14, 15, 16, 17, 19, 20,
	21, 22, 23, 25, 27, 29, 31, 33, 35, 37,
	39, 41, 44, 47, 50, 53, 56, 60, 64, 68,
	72, 77, 82, 87, 93, 99, 105,
};

struct brightness_table s6e3hf4_a3_da_panel_hmd_brightness_table = {
	.brt = a3_da_panel_hmd_brt_tbl,
	.sz_brt = ARRAY_SIZE(a3_da_panel_hmd_brt_tbl),
	.lum = a3_da_panel_hmd_lum_tbl,
	.sz_lum = ARRAY_SIZE(a3_da_panel_hmd_lum_tbl),
	.brt_to_step = a3_da_panel_hmd_brt_tbl,
	.sz_brt_to_step = ARRAY_SIZE(a3_da_panel_hmd_brt_tbl),
};

static struct panel_dimming_info s6e3hf4_a3_da_panel_hmd_dimming_info = {
	.name = "s6e3hf4_a3_da_hmd",
	.dim_init_info = {
		.name = "s6e3hf4_a3_da_hmd",
		.nr_tp = S6E3HF4_NR_TP,
		.tp = s6e3hf4_tp,
		.nr_luminance = S6E3HF4_HMD_NR_LUMINANCE,
		.vregout = 109051904LL,	/* 6.5 * 2^24*/
		.bitshift = 24,
		.vt_voltage = {
			0, 12, 24, 36, 48, 60, 72, 84, 96, 108,
			138, 148, 158, 168, 178, 186,
		},
		.target_luminance = S6E3HF4_HMD_TARGET_LUMINANCE,
		.target_gamma = 220,
		.dim_lut = a3_da_panel_hmd_dimming_lut,
	},
	.target_luminance = S6E3HF4_HMD_TARGET_LUMINANCE,
	.nr_luminance = S6E3HF4_HMD_NR_LUMINANCE,
	.hbm_target_luminance = -1,
	.nr_hbm_luminance = 0,
	.extend_hbm_target_luminance = -1,
	.nr_extend_hbm_luminance = 0,
	.brt_tbl = &s6e3hf4_a3_da_panel_hmd_brightness_table,
};
#endif /* __S6E3HF4_A3_DA_PANEL_HMD_DIMMING_H__ */
