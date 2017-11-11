/*
 * linux/drivers/video/fbdev/exynos/panel/s6e3hf4/s6e3hf4_a2_da_panel_hmd_dimming.h
 *
 * Header file for S6E3HF4 Dimming Driver
 *
 * Copyright (c) 2016 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __S6E3HF4_A2_DA_PANEL_HMD_DIMMING_H__
#define __S6E3HF4_A2_DA_PANEL_HMD_DIMMING_H__
#include "../dimming.h"
#include "../panel_dimming.h"
#include "s6e3hf4_dimming.h"

/*
 * PANEL INFORMATION
 * LDI : S6E3HF4
 * PANEL : A2 DA
 */
static s32 a2_da_hmd_rtbl10nit[] = {0, 0, 12, 12, 10, 9, 7, 4, 3, 2, 0};
static s32 a2_da_hmd_rtbl11nit[] = {0, 0, 12, 12, 11, 9, 7, 5, 3, 0, 0};
static s32 a2_da_hmd_rtbl12nit[] = {0, 0, 12, 12, 11, 9, 7, 5, 3, 0, 0};
static s32 a2_da_hmd_rtbl13nit[] = {0, 0, 12, 12, 10, 9, 7, 5, 2, 0, 0};
static s32 a2_da_hmd_rtbl14nit[] = {0, 0, 12, 12, 10, 9, 7, 5, 3, 1, 0};
static s32 a2_da_hmd_rtbl15nit[] = {0, 0, 12, 12, 10, 9, 7, 5, 2, 1, 0};
static s32 a2_da_hmd_rtbl16nit[] = {0, 0, 12, 12, 10, 8, 6, 5, 3, 2, 0};
static s32 a2_da_hmd_rtbl17nit[] = {0, 0, 12, 12, 10, 9, 7, 4, 3, 3, 0};
static s32 a2_da_hmd_rtbl19nit[] = {0, 0, 12, 12, 9, 8, 7, 4, 3, 3, 0};
static s32 a2_da_hmd_rtbl20nit[] = {0, 0, 12, 12, 9, 9, 7, 4, 2, 3, 0};
static s32 a2_da_hmd_rtbl21nit[] = {0, 0, 12, 12, 9, 8, 6, 4, 2, 2, 0};
static s32 a2_da_hmd_rtbl22nit[] = {0, 0, 12, 12, 9, 8, 5, 3, 1, 3, 0};
static s32 a2_da_hmd_rtbl23nit[] = {0, 0, 12, 12, 9, 8, 6, 3, 1, 2, 0};
static s32 a2_da_hmd_rtbl25nit[] = {0, 0, 12, 12, 9, 7, 6, 3, 2, 3, 0};
static s32 a2_da_hmd_rtbl27nit[] = {0, 0, 12, 12, 9, 9, 7, 5, 3, 3, 0};
static s32 a2_da_hmd_rtbl29nit[] = {0, 0, 12, 12, 9, 8, 6, 4, 2, 3, 0};
static s32 a2_da_hmd_rtbl31nit[] = {0, 0, 12, 12, 9, 8, 6, 4, 4, 4, 0};
static s32 a2_da_hmd_rtbl33nit[] = {0, 0, 12, 12, 8, 8, 6, 4, 4, 4, 0};
static s32 a2_da_hmd_rtbl35nit[] = {0, 0, 12, 12, 9, 7, 6, 5, 5, 4, 0};
static s32 a2_da_hmd_rtbl37nit[] = {0, 0, 12, 12, 9, 8, 7, 6, 6, 5, 0};
static s32 a2_da_hmd_rtbl39nit[] = {0, 0, 12, 12, 9, 7, 6, 5, 5, 5, 0};
static s32 a2_da_hmd_rtbl41nit[] = {0, 0, 12, 12, 9, 8, 6, 5, 6, 2, 0};
static s32 a2_da_hmd_rtbl44nit[] = {0, 0, 12, 11, 8, 8, 6, 6, 6, 3, 0};
static s32 a2_da_hmd_rtbl47nit[] = {0, 0, 12, 11, 9, 8, 6, 7, 6, 4, 0};
static s32 a2_da_hmd_rtbl50nit[] = {0, 0, 12, 11, 9, 9, 6, 7, 7, 3, 0};
static s32 a2_da_hmd_rtbl53nit[] = {0, 0, 12, 11, 9, 8, 6, 6, 8, 5, 0};
static s32 a2_da_hmd_rtbl56nit[] = {0, 0, 12, 11, 8, 8, 6, 7, 8, 4, 0};
static s32 a2_da_hmd_rtbl60nit[] = {0, 0, 12, 11, 9, 9, 6, 7, 9, 5, 0};
static s32 a2_da_hmd_rtbl64nit[] = {0, 0, 12, 11, 8, 8, 6, 8, 9, 6, 0};
static s32 a2_da_hmd_rtbl68nit[] = {0, 0, 11, 11, 9, 9, 7, 8, 10, 8, 0};
static s32 a2_da_hmd_rtbl72nit[] = {0, 0, 11, 11, 10, 9, 8, 8, 10, 9, 0};
static s32 a2_da_hmd_rtbl77nit[] = {0, 0, 9, 8, 5, 4, 5, 5, 7, 4, 0};
static s32 a2_da_hmd_rtbl82nit[] = {0, 0, 9, 8, 6, 5, 5, 6, 7, 4, 0};
static s32 a2_da_hmd_rtbl87nit[] = {0, 0, 9, 7, 5, 5, 5, 6, 8, 5, 0};
static s32 a2_da_hmd_rtbl93nit[] = {0, 0, 9, 7, 6, 5, 5, 6, 8, 5, 0};
static s32 a2_da_hmd_rtbl99nit[] = {0, 0, 10, 8, 5, 5, 5, 6, 8, 7, 0};
static s32 a2_da_hmd_rtbl105nit[] = {0, 0, 9, 8, 6, 5, 5, 6, 9, 6, 0};

static s32 a2_da_hmd_ctbl10nit[] = {0, 0, 0, 0, 0, 0, 6, 5, 4, -4, 8, -6, -4, 5, -12, -6, 4, -8, -7, 4, -10, -2, 2, -6, 0, 0, -1, 0, 0, 0, 1, 0, 0};
static s32 a2_da_hmd_ctbl11nit[] = {0, 0, 0, 0, 0, 0, 6, 5, 4, -4, 10, -5, -2, 4, -8, -7, 4, -9, -6, 4, -9, -1, 2, -5, 0, 0, -1, 0, 0, 0, 1, 0, 0};
static s32 a2_da_hmd_ctbl12nit[] = {0, 0, 0, 0, 0, 0, 6, 5, 4, -2, 11, -4, -3, 4, -10, -7, 3, -8, -5, 4, -8, -2, 2, -5, 0, 0, 0, 0, 0, 0, 1, 0, 0};
static s32 a2_da_hmd_ctbl13nit[] = {0, 0, 0, 0, 0, 0, 6, 5, 6, -2, 13, -4, -3, 5, -10, -8, 4, -10, -5, 3, -8, -2, 1, -4, -1, 0, -2, 1, 0, 0, 0, 0, 0};
static s32 a2_da_hmd_ctbl14nit[] = {0, 0, 0, 0, 0, 0, 7, 6, 6, -3, 13, -5, -4, 5, -12, -6, 3, -8, -4, 4, -8, -2, 2, -4, 0, 0, 0, 0, 0, -1, 1, 0, 0};
static s32 a2_da_hmd_ctbl15nit[] = {0, 0, 0, 0, 0, 0, 5, 6, 4, -4, 8, -9, -3, 4, -8, -7, 3, -8, -4, 4, -8, -2, 1, -3, 0, 0, -2, 0, 0, 0, 1, 0, 0};
static s32 a2_da_hmd_ctbl16nit[] = {0, 0, 0, 0, 0, 0, 5, 6, 4, -4, 8, -9, -3, 3, -8, -7, 4, -8, -4, 3, -8, -2, 2, -4, 1, 0, 0, 0, 0, 0, 0, 0, 0};
static s32 a2_da_hmd_ctbl17nit[] = {0, 0, 0, 0, 0, 0, 4, 5, 4, -4, 8, -9, -5, 4, -10, -5, 3, -8, -5, 3, -8, -2, 1, -4, 1, 0, 0, 0, 0, 1, 0, 0, 0};
static s32 a2_da_hmd_ctbl19nit[] = {0, 0, 0, 0, 0, 0, 4, 6, 3, -2, 11, -7, -4, 4, -10, -7, 4, -8, -3, 3, -7, -3, 1, -4, 0, 0, 0, 0, 0, 0, 1, 0, 0};
static s32 a2_da_hmd_ctbl20nit[] = {0, 0, 0, 0, 0, 0, 6, 7, 5, -4, 8, -9, -4, 5, -11, -6, 3, -8, -4, 3, -6, -1, 1, -2, 0, 0, -1, 0, 0, 0, 1, 0, 0};
static s32 a2_da_hmd_ctbl21nit[] = {0, 0, 0, 0, 0, 0, 6, 7, 5, -6, 8, -11, -5, 5, -11, -6, 3, -8, -3, 3, -6, 0, 1, -2, 0, 0, 0, 0, 0, 0, 2, 0, 1};
static s32 a2_da_hmd_ctbl22nit[] = {0, 0, 0, 0, 0, 0, 9, 11, 7, -8, 4, -13, -4, 4, -10, -7, 3, -7, -5, 3, -8, 0, 1, -2, 0, 0, 0, 0, 0, 0, 1, 0, 0};
static s32 a2_da_hmd_ctbl23nit[] = {0, 0, 0, 0, 0, 0, 9, 11, 6, -8, 5, -14, -5, 4, -10, -6, 3, -7, -4, 3, -6, -1, 1, -3, 1, 0, 0, 0, 0, 0, 1, 0, 1};
static s32 a2_da_hmd_ctbl25nit[] = {0, 0, 0, 0, 0, 0, 8, 11, 6, -8, 5, -14, -4, 4, -9, -5, 3, -7, -3, 3, -6, -1, 1, -2, 1, 0, -1, 0, 0, 0, 1, 0, 1};
static s32 a2_da_hmd_ctbl27nit[] = {0, 0, 0, 0, 0, 0, 5, 6, 4, -8, 8, -14, -4, 5, -10, -6, 3, -7, -4, 3, -6, -2, 0, -2, 0, 0, -2, 0, 0, 0, 1, 0, 0};
static s32 a2_da_hmd_ctbl29nit[] = {0, 0, 0, 0, 0, 0, 4, 6, 2, -8, 8, -14, -5, 5, -10, -6, 3, -7, -3, 2, -6, 0, 1, -2, 0, 0, 0, 0, 0, 0, 2, 0, 2};
static s32 a2_da_hmd_ctbl31nit[] = {0, 0, 0, 0, 0, 0, 7, 11, 5, -8, 2, -14, -5, 4, -10, -4, 3, -6, -5, 2, -6, -1, 0, -2, 0, 0, -1, 0, 0, 0, 1, 0, 0};
static s32 a2_da_hmd_ctbl33nit[] = {0, 0, 0, 0, 0, 0, 6, 11, 5, -8, 3, -14, -6, 4, -10, -5, 3, -7, -3, 2, -5, 0, 1, -2, 0, 0, 0, 0, 0, 0, 1, 0, 1};
static s32 a2_da_hmd_ctbl35nit[] = {0, 0, 0, 0, 0, 0, 6, 11, 4, -8, 5, -14, -4, 4, -9, -5, 3, -6, -3, 2, -5, -1, 0, -2, 0, 0, 0, 1, 0, 0, 1, 0, 1};
static s32 a2_da_hmd_ctbl37nit[] = {0, 0, 0, 0, 0, 0, 6, 11, 4, -8, 4, -14, -4, 4, -8, -4, 3, -6, -4, 2, -5, -1, 0, -2, 0, 0, -1, 0, 0, 0, 1, 0, 1};
static s32 a2_da_hmd_ctbl39nit[] = {0, 0, 0, 0, 0, 0, 6, 11, 4, -8, 4, -14, -4, 4, -9, -5, 2, -6, -3, 2, -4, 0, 1, -2, 0, 0, 0, 1, 0, 1, 2, 0, 1};
static s32 a2_da_hmd_ctbl41nit[] = {0, 0, 0, 0, 0, 0, 5, 11, 4, -8, 7, -14, -4, 4, -8, -6, 3, -8, -3, 2, -5, 0, 0, -1, 0, 0, 0, 0, 0, 0, 2, 0, 2};
static s32 a2_da_hmd_ctbl44nit[] = {0, 0, 0, 0, 0, 0, 0, 6, -2, -6, 6, -12, -6, 4, -10, -4, 3, -6, -1, 1, -4, -1, 1, -2, 0, 0, 0, 1, 0, 0, 2, 0, 2};
static s32 a2_da_hmd_ctbl47nit[] = {0, 0, 0, 0, 0, 0, 2, 5, -1, -7, 9, -12, -4, 4, -8, -4, 3, -6, -2, 2, -4, -1, 0, -1, 0, 0, 0, 0, 0, 0, 2, 0, 2};
static s32 a2_da_hmd_ctbl50nit[] = {0, 0, 0, 0, 0, 0, 2, 5, 0, -7, 9, -12, -4, 4, -8, -4, 3, -6, -3, 2, -4, 0, 0, -1, 0, 0, -1, 1, 0, 0, 1, 0, 2};
static s32 a2_da_hmd_ctbl53nit[] = {0, 0, 0, 0, 0, 0, 4, 6, 1, -5, 9, -14, -5, 4, -8, -4, 2, -6, -3, 1, -4, -1, 0, -1, 0, 0, -1, 0, 0, -1, 2, 0, 3};
static s32 a2_da_hmd_ctbl56nit[] = {0, 0, 0, 0, 0, 0, 4, 9, 1, -9, 5, -10, -4, 4, -8, -5, 3, -6, -1, 1, -3, -1, 0, -1, 0, 0, -1, 0, 0, 0, 2, 0, 3};
static s32 a2_da_hmd_ctbl60nit[] = {0, 0, 0, 0, 0, 0, 4, 10, 2, -8, 6, -14, -4, 4, -8, -4, 3, -6, -2, 1, -3, -1, 0, -2, 0, 0, 0, 0, 0, 0, 3, 0, 3};
static s32 a2_da_hmd_ctbl64nit[] = {0, 0, 0, 0, 0, 0, 3, 10, 0, -6, 7, -11, -6, 4, -9, -4, 2, -6, -1, 1, -2, 0, 0, -1, 0, 0, -1, 0, 0, 0, 3, 0, 3};
static s32 a2_da_hmd_ctbl68nit[] = {0, 0, 0, 0, 0, 0, 3, 10, 0, -6, 7, -11, -5, 4, -8, -3, 2, -5, -3, 1, -4, 0, 0, -1, 0, 0, -1, 0, 0, 0, 2, 0, 2};
static s32 a2_da_hmd_ctbl72nit[] = {0, 0, 0, 0, 0, 0, 2, 9, 0, -6, 9, -11, -4, 3, -8, -4, 2, -5, -2, 1, -3, 0, 0, 0, 0, 0, -1, 0, 0, 0, 3, 0, 3};
static s32 a2_da_hmd_ctbl77nit[] = {0, 0, 0, 0, 0, 0, -1, 3, -4, -5, 5, -12, -3, 3, -6, -3, 2, -4, 0, 1, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 2};
static s32 a2_da_hmd_ctbl82nit[] = {0, 0, 0, 0, 0, 0, -1, 2, -4, -5, 6, -14, -3, 3, -6, -2, 2, -4, -1, 1, -2, 0, 0, 0, 0, 0, -1, 1, 0, 0, 2, 0, 3};
static s32 a2_da_hmd_ctbl87nit[] = {0, 0, 0, 0, 0, 0, -1, 2, -4, -5, 6, -14, -2, 4, -6, -3, 1, -4, 0, 1, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 2};
static s32 a2_da_hmd_ctbl93nit[] = {0, 0, 0, 0, 0, 0, -1, 3, -5, -3, 7, -12, -2, 4, -5, -2, 2, -4, 0, 1, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2};
static s32 a2_da_hmd_ctbl99nit[] = {0, 0, 0, 0, 0, 0, -3, 1, -10, -7, 3, -12, -3, 5, -5, -4, 2, -2, 0, 1, -2, 0, 0, 0, 1, 0, 0, 1, 0, 0, 2, 0, 2};
static s32 a2_da_hmd_ctbl105nit[] = {0, 0, 0, 0, 0, 0, -2, 2, -9, -6, 5, -10, -4, 3, -4, -4, 1, -2, 0, 0, -1, 0, 0, -1, 1, 0, 0, 0, 0, 0, 2, 0, 3};

struct dimming_lut a2_da_panel_hmd_dimming_lut[] = {
	DIM_LUT_INIT(10, 57, GAMMA_2_15, a2_da_hmd_rtbl10nit, a2_da_hmd_ctbl10nit),
	DIM_LUT_INIT(11, 62, GAMMA_2_15, a2_da_hmd_rtbl11nit, a2_da_hmd_ctbl11nit),
	DIM_LUT_INIT(12, 66, GAMMA_2_15, a2_da_hmd_rtbl12nit, a2_da_hmd_ctbl12nit),
	DIM_LUT_INIT(13, 72, GAMMA_2_15, a2_da_hmd_rtbl13nit, a2_da_hmd_ctbl13nit),
	DIM_LUT_INIT(14, 77, GAMMA_2_15, a2_da_hmd_rtbl14nit, a2_da_hmd_ctbl14nit),
	DIM_LUT_INIT(15, 82, GAMMA_2_15, a2_da_hmd_rtbl15nit, a2_da_hmd_ctbl15nit),
	DIM_LUT_INIT(16, 86, GAMMA_2_15, a2_da_hmd_rtbl16nit, a2_da_hmd_ctbl16nit),
	DIM_LUT_INIT(17, 90, GAMMA_2_15, a2_da_hmd_rtbl17nit, a2_da_hmd_ctbl17nit),
	DIM_LUT_INIT(19, 98, GAMMA_2_15, a2_da_hmd_rtbl19nit, a2_da_hmd_ctbl19nit),
	DIM_LUT_INIT(20, 103, GAMMA_2_15, a2_da_hmd_rtbl20nit, a2_da_hmd_ctbl20nit),
	DIM_LUT_INIT(21, 109, GAMMA_2_15, a2_da_hmd_rtbl21nit, a2_da_hmd_ctbl21nit),
	DIM_LUT_INIT(22, 119, GAMMA_2_15, a2_da_hmd_rtbl22nit, a2_da_hmd_ctbl22nit),
	DIM_LUT_INIT(23, 123, GAMMA_2_15, a2_da_hmd_rtbl23nit, a2_da_hmd_ctbl23nit),
	DIM_LUT_INIT(25, 132, GAMMA_2_15, a2_da_hmd_rtbl25nit, a2_da_hmd_ctbl25nit),
	DIM_LUT_INIT(27, 142, GAMMA_2_15, a2_da_hmd_rtbl27nit, a2_da_hmd_ctbl27nit),
	DIM_LUT_INIT(29, 151, GAMMA_2_15, a2_da_hmd_rtbl29nit, a2_da_hmd_ctbl29nit),
	DIM_LUT_INIT(31, 161, GAMMA_2_15, a2_da_hmd_rtbl31nit, a2_da_hmd_ctbl31nit),
	DIM_LUT_INIT(33, 168, GAMMA_2_15, a2_da_hmd_rtbl33nit, a2_da_hmd_ctbl33nit),
	DIM_LUT_INIT(35, 177, GAMMA_2_15, a2_da_hmd_rtbl35nit, a2_da_hmd_ctbl35nit),
	DIM_LUT_INIT(37, 185, GAMMA_2_15, a2_da_hmd_rtbl37nit, a2_da_hmd_ctbl37nit),
	DIM_LUT_INIT(39, 195, GAMMA_2_15, a2_da_hmd_rtbl39nit, a2_da_hmd_ctbl39nit),
	DIM_LUT_INIT(41, 201, GAMMA_2_15, a2_da_hmd_rtbl41nit, a2_da_hmd_ctbl41nit),
	DIM_LUT_INIT(44, 214, GAMMA_2_15, a2_da_hmd_rtbl44nit, a2_da_hmd_ctbl44nit),
	DIM_LUT_INIT(47, 227, GAMMA_2_15, a2_da_hmd_rtbl47nit, a2_da_hmd_ctbl47nit),
	DIM_LUT_INIT(50, 241, GAMMA_2_15, a2_da_hmd_rtbl50nit, a2_da_hmd_ctbl50nit),
	DIM_LUT_INIT(53, 253, GAMMA_2_15, a2_da_hmd_rtbl53nit, a2_da_hmd_ctbl53nit),
	DIM_LUT_INIT(56, 262, GAMMA_2_15, a2_da_hmd_rtbl56nit, a2_da_hmd_ctbl56nit),
	DIM_LUT_INIT(60, 276, GAMMA_2_15, a2_da_hmd_rtbl60nit, a2_da_hmd_ctbl60nit),
	DIM_LUT_INIT(64, 291, GAMMA_2_15, a2_da_hmd_rtbl64nit, a2_da_hmd_ctbl64nit),
	DIM_LUT_INIT(68, 305, GAMMA_2_15, a2_da_hmd_rtbl68nit, a2_da_hmd_ctbl68nit),
	DIM_LUT_INIT(72, 315, GAMMA_2_15, a2_da_hmd_rtbl72nit, a2_da_hmd_ctbl72nit),
	DIM_LUT_INIT(77, 243, GAMMA_2_15, a2_da_hmd_rtbl77nit, a2_da_hmd_ctbl77nit),
	DIM_LUT_INIT(82, 256, GAMMA_2_15, a2_da_hmd_rtbl82nit, a2_da_hmd_ctbl82nit),
	DIM_LUT_INIT(87, 267, GAMMA_2_15, a2_da_hmd_rtbl87nit, a2_da_hmd_ctbl87nit),
	DIM_LUT_INIT(93, 284, GAMMA_2_15, a2_da_hmd_rtbl93nit, a2_da_hmd_ctbl93nit),
	DIM_LUT_INIT(99, 296, GAMMA_2_15, a2_da_hmd_rtbl99nit, a2_da_hmd_ctbl99nit),
	DIM_LUT_INIT(105, 311, GAMMA_2_15, a2_da_hmd_rtbl105nit, a2_da_hmd_ctbl105nit),
};

static unsigned int a2_da_panel_hmd_brt_tbl[S6E3HF4_MAX_BRIGHTNESS + 1] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 2, 2,
	3, 3, 4, 4, 4, 5, 5, 6, 6, 6, 7, 7, 7, 7, 7, 8,
	8, 9, 9, 10, 10, 10, 11, 11, 12, 12, 12, 12, 12, 13, 13, 13,
	13, 13, 14, 14, 14, 14, 14, 15, 15, 15, 15, 15, 16, 16, 16, 16,
	16, 17, 17, 17, 17, 18, 18, 18, 18, 18, 19, 19, 19, 19, 19, 20,
	20, 20, 20, 20, 21, 21, 21, 21, 21, 21, 21, 22, 22, 22, 22, 22,
	22, 22, 22, 23, 23, 23, 23, 23, 23, 23, 24, 24, 24, 24, 24, 24,
	24, 25, 25, 25, 25, 25, 25, 25, 26, 26, 26, 26, 26, 26, 26, 26,
	26, 26, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 28, 28, 28, 28,
	28, 28, 28, 28, 28, 28, 29, 29, 29, 29, 29, 29, 29, 29, 29, 30,
	30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 31, 31, 31, 31, 31,
	31, 31, 31, 31, 31, 31, 31, 31, 32, 32, 32, 32, 32, 32, 32, 32,
	32, 32, 32, 32, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33,
	33, 33, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34,
	34, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 36,
	[UI_MAX_BRIGHTNESS + 1 ... S6E3HF4_MAX_BRIGHTNESS] = 36,
};

static unsigned int a2_da_panel_hmd_lum_tbl[S6E3HF4_HMD_NR_LUMINANCE] = {
	10, 11, 12, 13, 14, 15, 16, 17, 19, 20,
	21, 22, 23, 25, 27, 29, 31, 33, 35, 37,
	39, 41, 44, 47, 50, 53, 56, 60, 64, 68,
	72, 77, 82, 87, 93, 99, 105,
};

struct brightness_table s6e3hf4_a2_da_panel_hmd_brightness_table = {
	.brt = a2_da_panel_hmd_brt_tbl,
	.sz_brt = ARRAY_SIZE(a2_da_panel_hmd_brt_tbl),
	.lum = a2_da_panel_hmd_lum_tbl,
	.sz_lum = ARRAY_SIZE(a2_da_panel_hmd_lum_tbl),
	.brt_to_step = a2_da_panel_hmd_brt_tbl,
	.sz_brt_to_step = ARRAY_SIZE(a2_da_panel_hmd_brt_tbl),
};

static struct panel_dimming_info s6e3hf4_a2_da_panel_hmd_dimming_info = {
	.name = "s6e3hf4_a2_da_hmd",
	.dim_init_info = {
		.name = "s6e3hf4_a2_da_hmd",
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
		.dim_lut = a2_da_panel_hmd_dimming_lut,
	},
	.target_luminance = S6E3HF4_HMD_TARGET_LUMINANCE,
	.nr_luminance = S6E3HF4_HMD_NR_LUMINANCE,
	.hbm_target_luminance = -1,
	.nr_hbm_luminance = 0,
	.extend_hbm_target_luminance = -1,
	.nr_extend_hbm_luminance = 0,
	.brt_tbl = &s6e3hf4_a2_da_panel_hmd_brightness_table,
};
#endif /* __S6E3HF4_A2_DA_PANEL_HMD_DIMMING_H__ */
