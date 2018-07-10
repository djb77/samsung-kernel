/*
 * linux/drivers/video/fbdev/exynos/panel/s6e3ha2/s6e3ha2_luge_a2_da_panel_dimming.h
 *
 * Header file for S6E3HA2 Dimming Driver
 *
 * Copyright (c) 2016 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __S6E3HA2_LUGE_A2_DA_PANEL_DIMMING_H___
#define __S6E3HA2_LUGE_A2_DA_PANEL_DIMMING_H___
#include "../dimming.h"
#include "../panel_dimming.h"
#include "s6e3ha2_dimming.h"

/*
 * PANEL INFORMATION
 * LDI : S6E3HA2
 * PANEL : LUGE_A2_DA
 */
/* gray scale offset values */
static s32 luge_a2_da_rtbl2nit[10] = { 0, 41, 42, 37, 32, 30, 22, 14, 9, 0 };
static s32 luge_a2_da_rtbl3nit[10] = { 0, 36, 36, 32, 27, 24, 19, 12, 7, 0 };
static s32 luge_a2_da_rtbl4nit[10] = { 0, 29, 32, 27, 22, 19, 15, 10, 6, 0 };
static s32 luge_a2_da_rtbl5nit[10] = { 0, 26, 28, 24, 20, 17, 13, 9, 6, 0 };
static s32 luge_a2_da_rtbl6nit[10] = { 0, 26, 25, 21, 17, 15, 13, 8, 5, 0 };
static s32 luge_a2_da_rtbl7nit[10] = { 0, 19, 24, 20, 15, 13, 11, 7, 5, 0 };
static s32 luge_a2_da_rtbl8nit[10] = { 0, 20, 22, 18, 14, 13, 10, 6, 5, 0 };
static s32 luge_a2_da_rtbl9nit[10] = { 0, 21, 20, 16, 12, 11, 9, 6, 5, 0 };
static s32 luge_a2_da_rtbl10nit[10] = { 0, 21, 20, 16, 12, 11, 9, 6, 5, 0 };
static s32 luge_a2_da_rtbl11nit[10] = { 0, 21, 18, 15, 11, 10, 9, 6, 5, 0 };
static s32 luge_a2_da_rtbl12nit[10] = { 0, 20, 18, 14, 11, 10, 8, 6, 4, 0 };
static s32 luge_a2_da_rtbl13nit[10] = { 0, 19, 16, 13, 10, 9, 7, 6, 4, 0 };
static s32 luge_a2_da_rtbl14nit[10] = { 0, 18, 15, 12, 9, 8, 7, 5, 4, 0 };
static s32 luge_a2_da_rtbl15nit[10] = { 0, 18, 15, 12, 9, 8, 6, 5, 4, 0 };
static s32 luge_a2_da_rtbl16nit[10] = { 0, 16, 15, 11, 8, 8, 6, 5, 4, 0 };
static s32 luge_a2_da_rtbl17nit[10] = { 0, 17, 14, 11, 8, 7, 5, 5, 4, 0 };
static s32 luge_a2_da_rtbl19nit[10] = { 0, 15, 13, 10, 7, 6, 5, 5, 4, 0 };
static s32 luge_a2_da_rtbl20nit[10] = { 0, 15, 13, 9, 7, 6, 5, 4, 4, 0 };
static s32 luge_a2_da_rtbl21nit[10] = { 0, 15, 13, 9, 7, 6, 5, 4, 4, 0 };
static s32 luge_a2_da_rtbl22nit[10] = { 0, 15, 12, 9, 7, 6, 5, 4, 4, 0 };
static s32 luge_a2_da_rtbl24nit[10] = { 0, 14, 12, 8, 7, 6, 5, 4, 4, 0 };
static s32 luge_a2_da_rtbl25nit[10] = { 0, 14, 12, 8, 6, 6, 5, 4, 4, 0 };
static s32 luge_a2_da_rtbl27nit[10] = { 0, 14, 12, 8, 6, 5, 5, 4, 4, 0 };
static s32 luge_a2_da_rtbl29nit[10] = { 0, 13, 11, 8, 5, 5, 4, 3, 4, 0 };
static s32 luge_a2_da_rtbl30nit[10] = { 0, 13, 11, 8, 5, 5, 4, 3, 4, 0 };
static s32 luge_a2_da_rtbl32nit[10] = { 0, 13, 10, 7, 5, 5, 4, 3, 4, 0 };
static s32 luge_a2_da_rtbl34nit[10] = { 0, 12, 10, 7, 4, 5, 4, 3, 3, 0 };
static s32 luge_a2_da_rtbl37nit[10] = { 0, 12, 10, 7, 5, 4, 4, 3, 3, 0 };
static s32 luge_a2_da_rtbl39nit[10] = { 0, 12, 9, 7, 4, 4, 3, 3, 3, 0 };
static s32 luge_a2_da_rtbl41nit[10] = { 0, 11, 9, 6, 4, 4, 4, 3, 3, 0 };
static s32 luge_a2_da_rtbl44nit[10] = { 0, 10, 8, 6, 4, 4, 4, 3, 3, 0 };
static s32 luge_a2_da_rtbl47nit[10] = { 0, 10, 8, 5, 3, 4, 4, 3, 3, 0 };
static s32 luge_a2_da_rtbl50nit[10] = { 0, 10, 8, 5, 3, 3, 4, 3, 3, 0 };
static s32 luge_a2_da_rtbl53nit[10] = { 0, 9, 8, 5, 3, 4, 4, 2, 3, 0 };
static s32 luge_a2_da_rtbl56nit[10] = { 0, 9, 8, 5, 3, 4, 4, 2, 3, 0 };
static s32 luge_a2_da_rtbl60nit[10] = { 0, 9, 7, 5, 3, 3, 3, 2, 3, 0 };
static s32 luge_a2_da_rtbl64nit[10] = { 0, 8, 6, 4, 3, 3, 3, 2, 3, 0 };
static s32 luge_a2_da_rtbl68nit[10] = { 0, 8, 6, 4, 2, 3, 2, 2, 3, 0 };
static s32 luge_a2_da_rtbl72nit[10] = { 0, 7, 5, 4, 2, 2, 2, 2, 3, 0 };
static s32 luge_a2_da_rtbl77nit[10] = { 0, 7, 5, 3, 2, 2, 2, 2, 3, 0 };
static s32 luge_a2_da_rtbl82nit[10] = { 0, 7, 5, 3, 2, 2, 1, 2, 2, 0 };
static s32 luge_a2_da_rtbl87nit[10] = { 0, 7, 5, 3, 2, 1, 2, 2, 1, 0 };
static s32 luge_a2_da_rtbl93nit[10] = { 0, 7, 4, 2, 1, 1, 2, 2, 2, 0 };
static s32 luge_a2_da_rtbl98nit[10] = { 0, 7, 4, 3, 2, 1, 2, 3, 2, 0 };
static s32 luge_a2_da_rtbl105nit[10] = { 0, 7, 5, 4, 2, 2, 2, 3, 2, 0 };
static s32 luge_a2_da_rtbl111nit[10] = { 0, 7, 5, 3, 2, 1, 2, 2, 1, 0 };
static s32 luge_a2_da_rtbl119nit[10] = { 0, 7, 5, 3, 3, 2, 2, 4, 1, 0 };
static s32 luge_a2_da_rtbl126nit[10] = { 0, 7, 4, 2, 2, 2, 2, 3, 1, 0 };
static s32 luge_a2_da_rtbl134nit[10] = { 0, 7, 4, 3, 2, 1, 2, 4, 1, 0 };
static s32 luge_a2_da_rtbl143nit[10] = { 0, 6, 4, 3, 2, 2, 2, 3, 1, 0 };
static s32 luge_a2_da_rtbl152nit[10] = { 0, 6, 4, 3, 2, 2, 2, 4, 1, 0 };
static s32 luge_a2_da_rtbl162nit[10] = { 0, 6, 3, 2, 2, 2, 3, 3, 1, 0 };
static s32 luge_a2_da_rtbl172nit[10] = { 0, 5, 3, 2, 2, 2, 3, 4, 1, 0 };
static s32 luge_a2_da_rtbl183nit[10] = { 0, 5, 3, 2, 2, 2, 2, 3, 1, 0 };
static s32 luge_a2_da_rtbl195nit[10] = { 0, 5, 3, 1, 2, 2, 2, 3, 1, 0 };
static s32 luge_a2_da_rtbl207nit[10] = { 0, 4, 3, 1, 1, 1, 1, 2, 1, 0 };
static s32 luge_a2_da_rtbl220nit[10] = { 0, 3, 2, 1, 1, 1, 1, 2, 0, 0 };
static s32 luge_a2_da_rtbl234nit[10] = { 0, 3, 2, 0, 1, 1, 1, 1, 0, 0 };
static s32 luge_a2_da_rtbl249nit[10] = { 0, 0, 0, 0, 0, 0, 1, 1, 0, 0 };
static s32 luge_a2_da_rtbl265nit[10] = { 0, 0, 0, 0, 0, 0, 0, 1, 0, 0 };
static s32 luge_a2_da_rtbl282nit[10] = { 0, 0, 0, 0, 0, 1, 0, 1, 0, 0 };
static s32 luge_a2_da_rtbl300nit[10] = { 0, 0, 0, 0, 0, 0, 0, 1, 0, 0 };
static s32 luge_a2_da_rtbl316nit[10] = { 0, 0, 0, 0, -1, 0, 0, 0, 0, 0 };
static s32 luge_a2_da_rtbl333nit[10] = { 0, 0, 0, -1, 0, 0, 0, 0, 0, 0 };
static s32 luge_a2_da_rtbl360nit[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

static s32 luge_a2_da_ctbl2nit[30] = { 0, 0, 0, 0, 0, 0, -3, 2, -4, -3, 2, -5, -4, 3, -7, -9, 3, -9, -6, 4, -7, -3, 1, -4, -4, 0, -4, -7, 0, -1 };
static s32 luge_a2_da_ctbl3nit[30] = { 0, 0, 0, 0, 0, 0, -3, 2, -6, -3, 2, -6, -7, 2, -7, -7, 3, -9, -7, 3, -8, -3, 0, -4, -4, 0, -4, -5, 0, 1 };
static s32 luge_a2_da_ctbl4nit[30] = { 0, 0, 0, 0, 0, 0, -5, 0, -8, -5, 1, -8, -7, 2, -8, -7, 3, -9, -8, 2, -8, -3, 0, -4, -2, 0, -2, -4, 0, 1 };
static s32 luge_a2_da_ctbl5nit[30] = { 0, 0, 0, 0, 0, 0, -3, 3, -6, -5, 3, -7, -6, 2, -7, -7, 3, -9, -7, 3, -7, -3, 0, -4, -2, 0, -3, -3, 0, 2 };
static s32 luge_a2_da_ctbl6nit[30] = { 0, 0, 0, 0, 0, 0, -3, 3, -8, -3, 3, -6, -5, 3, -7, -7, 3, -9, -6, 2, -6, -3, 0, -4, -2, 0, -2, -2, 0, 2 };
static s32 luge_a2_da_ctbl7nit[30] = { 0, 0, 0, 0, 0, 0, -4, 2, -10, -5, 1, -7, -5, 3, -6, -8, 3, -9, -6, 2, -6, -2, 0, -3, -2, 0, -2, -2, 0, 2 };
static s32 luge_a2_da_ctbl8nit[30] = { 0, 0, 0, 0, 0, 0, -2, 3, -8, -6, 2, -8, -6, 3, -7, -6, 2, -8, -6, 2, -6, -2, 0, -2, -2, 0, -2, -1, 0, 3 };
static s32 luge_a2_da_ctbl9nit[30] = { 0, 0, 0, 0, 0, 0, 0, 3, -7, -8, 2, -9, -5, 3, -7, -5, 3, -6, -5, 2, -5, -2, 0, -2, -1, 0, -2, -1, 0, 3 };
static s32 luge_a2_da_ctbl10nit[30] = { 0, 0, 0, 0, 0, 0, -2, 3, -8, -6, 2, -8, -5, 3, -7, -6, 3, -7, -5, 1, -5, -2, 0, -2, -1, 0, -2, -1, 0, 3 };
static s32 luge_a2_da_ctbl11nit[30] = { 0, 0, 0, 0, 0, 0, -2, 3, -11, -7, 2, -9, -5, 3, -7, -7, 2, -8, -5, 1, -5, -1, 0, -1, -1, 0, -2, -1, 0, 3 };
static s32 luge_a2_da_ctbl12nit[30] = { 0, 0, 0, 0, 0, 0, -3, 2, -10, -6, 3, -7, -4, 3, -6, -6, 2, -7, -5, 1, -5, -1, 0, -1, -1, 0, -2, -1, 0, 3 };
static s32 luge_a2_da_ctbl13nit[30] = { 0, 0, 0, 0, 0, 0, -4, 4, -13, -6, 3, -9, -8, 2, -8, -7, 2, -7, -4, 1, -4, -1, 0, -1, -1, 0, -2, -1, 0, 3 };
static s32 luge_a2_da_ctbl14nit[30] = { 0, 0, 0, 0, 0, 0, -2, 4, -11, -6, 3, -9, -7, 2, -7, -6, 2, -7, -4, 1, -4, -1, 0, -1, -1, 0, -2, -1, 0, 3 };
static s32 luge_a2_da_ctbl15nit[30] = { 0, 0, 0, 0, 0, 0, -3, 4, -12, -6, 3, -8, -5, 3, -6, -6, 2, -7, -4, 1, -4, -1, 0, -1, -1, 0, -2, -1, 0, 3 };
static s32 luge_a2_da_ctbl16nit[30] = { 0, 0, 0, 0, 0, 0, -4, 2, -11, -7, 3, -9, -4, 3, -5, -6, 2, -7, -3, 1, -3, -1, 0, -1, -1, 0, -2, -1, 0, 3 };
static s32 luge_a2_da_ctbl17nit[30] = { 0, 0, 0, 0, 0, 0, -3, 3, -14, -6, 3, -7, -5, 3, -6, -6, 2, -6, -4, 1, -4, 0, 0, -1, -1, 0, -1, 0, 0, 3 };
static s32 luge_a2_da_ctbl19nit[30] = { 0, 0, 0, 0, 0, 0, -4, 4, -14, -5, 3, -6, -4, 3, -5, -6, 2, -6, -3, 1, -3, 0, 0, -1, -1, 0, -1, 0, 0, 3 };
static s32 luge_a2_da_ctbl20nit[30] = { 0, 0, 0, 0, 0, 0, -3, 4, -12, -5, 4, -8, -6, 2, -7, -6, 2, -6, -3, 1, -3, 0, 0, -1, -1, 0, -1, 0, 0, 3 };
static s32 luge_a2_da_ctbl21nit[30] = { 0, 0, 0, 0, 0, 0, -2, 4, -11, -4, 4, -7, -6, 2, -6, -6, 1, -7, -3, 1, -3, -1, 0, -1, -1, 0, -1, 0, 0, 3 };
static s32 luge_a2_da_ctbl22nit[30] = { 0, 0, 0, 0, 0, 0, -4, 4, -15, -5, 4, -7, -5, 2, -6, -6, 1, -6, -3, 1, -3, 0, 0, 0, -1, 0, -2, 0, 0, 3 };
static s32 luge_a2_da_ctbl24nit[30] = { 0, 0, 0, 0, 0, 0, -3, 3, -12, -6, 4, -9, -5, 2, -5, -6, 1, -6, -3, 0, -3, 0, 0, 0, -1, 0, -2, 0, 0, 3 };
static s32 luge_a2_da_ctbl25nit[30] = { 0, 0, 0, 0, 0, 0, -3, 3, -12, -6, 3, -9, -5, 2, -6, -6, 1, -6, -3, 0, -3, 0, 0, 0, -1, 0, -2, 0, 0, 3 };
static s32 luge_a2_da_ctbl27nit[30] = { 0, 0, 0, 0, 0, 0, -3, 3, -11, -6, 3, -8, -4, 2, -5, -6, 1, -6, -3, 0, -3, 0, 0, 0, -1, 0, -2, 0, 0, 3 };
static s32 luge_a2_da_ctbl29nit[30] = { 0, 0, 0, 0, 0, 0, -5, 3, -15, -4, 3, -6, -4, 2, -5, -5, 1, -5, -3, 0, -3, 0, 0, 0, -1, 0, -2, 0, 0, 3 };
static s32 luge_a2_da_ctbl30nit[30] = { 0, 0, 0, 0, 0, 0, -5, 3, -16, -4, 3, -7, -4, 2, -5, -5, 1, -5, -3, 0, -3, 0, 0, 0, -1, 0, -2, 0, 0, 3 };
static s32 luge_a2_da_ctbl32nit[30] = { 0, 0, 0, 0, 0, 0, -5, 5, -15, -5, 3, -8, -4, 2, -4, -4, 1, -5, -3, 0, -3, 0, 0, 0, -1, 0, -2, 0, 0, 3 };
static s32 luge_a2_da_ctbl34nit[30] = { 0, 0, 0, 0, 0, 0, -5, 4, -14, -5, 2, -8, -4, 2, -4, -3, 1, -4, -3, 0, -3, 0, 0, 0, -1, 0, -2, 0, 0, 3 };
static s32 luge_a2_da_ctbl37nit[30] = { 0, 0, 0, 0, 0, 0, -6, 4, -15, -6, 2, -9, -4, 1, -4, -4, 1, -5, -2, 0, -2, 0, 0, 0, -1, 0, -2, 0, 0, 3 };
static s32 luge_a2_da_ctbl39nit[30] = { 0, 0, 0, 0, 0, 0, -5, 5, -17, -5, 1, -8, -5, 1, -5, -3, 1, -4, -2, 0, -2, 0, 0, 0, -1, 0, -2, 0, 0, 3 };
static s32 luge_a2_da_ctbl41nit[30] = { 0, 0, 0, 0, 0, 0, -3, 4, -15, -7, 2, -9, -3, 2, -4, -3, 1, -3, -2, 0, -3, 0, 0, 0, -1, 0, -1, 0, 0, 2 };
static s32 luge_a2_da_ctbl44nit[30] = { 0, 0, 0, 0, 0, 0, -4, 5, -16, -3, 3, -5, -3, 2, -3, -3, 1, -4, -2, 0, -2, 0, 0, 0, -1, 0, -1, 0, 0, 2 };
static s32 luge_a2_da_ctbl47nit[30] = { 0, 0, 0, 0, 0, 0, -3, 4, -13, -4, 3, -7, -3, 2, -4, -3, 1, -3, -2, 0, -2, 0, 0, 0, 0, 0, -1, 0, 0, 2 };
static s32 luge_a2_da_ctbl50nit[30] = { 0, 0, 0, 0, 0, 0, -5, 3, -15, -3, 3, -6, -2, 2, -3, -4, 1, -4, -2, 0, -2, 0, 0, 0, 0, 0, -1, 0, 0, 2 };
static s32 luge_a2_da_ctbl53nit[30] = { 0, 0, 0, 0, 0, 0, -5, 3, -14, -4, 2, -7, -2, 2, -3, -4, 0, -4, -2, 0, -2, 0, 0, 0, 0, 0, -1, 0, 0, 2 };
static s32 luge_a2_da_ctbl56nit[30] = { 0, 0, 0, 0, 0, 0, -5, 3, -14, -3, 2, -6, -2, 2, -3, -4, 0, -4, -2, 0, -2, 0, 0, 0, 0, 0, -1, 0, 0, 2 };
static s32 luge_a2_da_ctbl60nit[30] = { 0, 0, 0, 0, 0, 0, -4, 5, -15, -3, 2, -6, -3, 1, -3, -3, 0, -4, -2, 0, -2, 0, 0, 0, 0, 0, -1, 0, 0, 2 };
static s32 luge_a2_da_ctbl64nit[30] = { 0, 0, 0, 0, 0, 0, -4, 5, -17, -4, 2, -7, -3, 1, -3, -2, 0, -3, -2, 0, -2, 0, 0, 0, 0, 0, -1, 0, 0, 2 };
static s32 luge_a2_da_ctbl68nit[30] = { 0, 0, 0, 0, 0, 0, -4, 5, -15, -3, 2, -6, -3, 1, -3, -1, 0, -3, -2, 0, -2, 0, 0, 0, 0, 0, -1, 0, 0, 2 };
static s32 luge_a2_da_ctbl72nit[30] = { 0, 0, 0, 0, 0, 0, -5, 6, -18, -2, 2, -5, -4, 1, -4, -1, 0, -2, -1, 0, -1, 0, 0, 0, 0, 0, -1, -1, 0, 1 };
static s32 luge_a2_da_ctbl77nit[30] = { 0, 0, 0, 0, 0, 0, -4, 5, -15, -3, 2, -6, -3, 1, -3, -1, 0, -2, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 1 };
static s32 luge_a2_da_ctbl82nit[30] = { 0, 0, 0, 0, 0, 0, -6, 5, -17, -2, 2, -4, -2, 1, -3, -2, 0, -2, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
static s32 luge_a2_da_ctbl87nit[30] = { 0, 0, 0, 0, 0, 0, -6, 5, -16, -2, 1, -5, -2, 1, -2, -2, 0, -3, -1, 0, -1, 0, 0, -1, 0, 0, 0, 1, 0, 2 };
static s32 luge_a2_da_ctbl93nit[30] = { 0, 0, 0, 0, 0, 0, -6, 5, -16, -2, 1, -5, -2, 1, -2, -1, 0, -2, -1, 0, -1, 0, 0, 0, 0, 0, 0, 1, 0, 2 };
static s32 luge_a2_da_ctbl98nit[30] = { 0, 0, 0, 0, 0, 0, -4, 5, -15, -3, 1, -5, -1, 1, -1, 0, 0, -2, -1, 0, -1, 0, 0, 0, 0, 0, 0, 1, 0, 1 };
static s32 luge_a2_da_ctbl105nit[30] = { 0, 0, 0, 0, 0, 0, -5, 5, -14, -2, 1, -4, -3, 0, -3, 0, 0, -1, -2, 0, -2, 0, 0, -1, 0, 0, 0, 0, 0, 1 };
static s32 luge_a2_da_ctbl111nit[30] = { 0, 0, 0, 0, 0, 0, -3, 5, -12, -2, 1, -4, -3, 0, -3, -2, 0, -2, -1, 0, -1, 1, 0, 0, 0, 0, 0, 1, 0, 1 };
static s32 luge_a2_da_ctbl119nit[30] = { 0, 0, 0, 0, 0, 0, -4, 5, -14, -2, 1, -4, -2, 0, -3, -2, 0, -2, -1, 0, -1, 0, 0, -1, 0, 0, 0, 1, 0, 1 };
static s32 luge_a2_da_ctbl126nit[30] = { 0, 0, 0, 0, 0, 0, -4, 5, -11, -2, 1, -4, -2, 0, -2, -1, 0, -2, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 1 };
static s32 luge_a2_da_ctbl134nit[30] = { 0, 0, 0, 0, 0, 0, -4, 5, -13, -3, 1, -3, -2, 0, -2, 0, 0, -1, -2, 0, -2, 0, 0, -1, -1, 0, -1, 0, 0, 1 };
static s32 luge_a2_da_ctbl143nit[30] = { 0, 0, 0, 0, 0, 0, -5, 4, -13, -2, 1, -3, -3, 0, -3, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, 1, 0, 2 };
static s32 luge_a2_da_ctbl152nit[30] = { 0, 0, 0, 0, 0, 0, -6, 4, -13, -1, 1, -2, -2, 0, -3, -1, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 1 };
static s32 luge_a2_da_ctbl162nit[30] = { 0, 0, 0, 0, 0, 0, -5, 4, -12, -3, 1, -3, -1, 0, -2, -1, 0, -1, -1, 0, -1, 1, 0, -1, 0, 0, 0, 0, 0, 1 };
static s32 luge_a2_da_ctbl172nit[30] = { 0, 0, 0, 0, 0, 0, -3, 4, -10, -3, 1, -3, -1, 0, -2, -1, 0, -1, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, 1 };
static s32 luge_a2_da_ctbl183nit[30] = { 0, 0, 0, 0, 0, 0, -3, 4, -10, -3, 0, -3, -1, 0, -2, -1, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
static s32 luge_a2_da_ctbl195nit[30] = { 0, 0, 0, 0, 0, 0, -4, 3, -10, -3, 0, -3, -1, 0, -2, -1, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
static s32 luge_a2_da_ctbl207nit[30] = { 0, 0, 0, 0, 0, 0, -3, 3, -8, -3, 0, -3, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
static s32 luge_a2_da_ctbl220nit[30] = { 0, 0, 0, 0, 0, 0, -3, 3, -9, -3, 0, -3, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
static s32 luge_a2_da_ctbl234nit[30] = { 0, 0, 0, 0, 0, 0, -4, 2, -7, -1, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
static s32 luge_a2_da_ctbl249nit[30] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static s32 luge_a2_da_ctbl265nit[30] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static s32 luge_a2_da_ctbl282nit[30] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static s32 luge_a2_da_ctbl300nit[30] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static s32 luge_a2_da_ctbl316nit[30] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static s32 luge_a2_da_ctbl333nit[30] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static s32 luge_a2_da_ctbl360nit[30] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

static struct dimming_lut luge_a2_da_dimming_lut[] = {
	DIM_LUT_V0_INIT(2, 112, GAMMA_2_15, luge_a2_da_rtbl2nit, luge_a2_da_ctbl2nit),
	DIM_LUT_V0_INIT(3, 112, GAMMA_2_15, luge_a2_da_rtbl3nit, luge_a2_da_ctbl3nit),
	DIM_LUT_V0_INIT(4, 112, GAMMA_2_15, luge_a2_da_rtbl4nit, luge_a2_da_ctbl4nit),
	DIM_LUT_V0_INIT(5, 112, GAMMA_2_15, luge_a2_da_rtbl5nit, luge_a2_da_ctbl5nit),
	DIM_LUT_V0_INIT(6, 112, GAMMA_2_15, luge_a2_da_rtbl6nit, luge_a2_da_ctbl6nit),
	DIM_LUT_V0_INIT(7, 112, GAMMA_2_15, luge_a2_da_rtbl7nit, luge_a2_da_ctbl7nit),
	DIM_LUT_V0_INIT(8, 112, GAMMA_2_15, luge_a2_da_rtbl8nit, luge_a2_da_ctbl8nit),
	DIM_LUT_V0_INIT(9, 112, GAMMA_2_15, luge_a2_da_rtbl9nit, luge_a2_da_ctbl9nit),
	DIM_LUT_V0_INIT(10, 112, GAMMA_2_15, luge_a2_da_rtbl10nit, luge_a2_da_ctbl10nit),
	DIM_LUT_V0_INIT(11, 112, GAMMA_2_15, luge_a2_da_rtbl11nit, luge_a2_da_ctbl11nit),
	DIM_LUT_V0_INIT(12, 112, GAMMA_2_15, luge_a2_da_rtbl12nit, luge_a2_da_ctbl12nit),
	DIM_LUT_V0_INIT(13, 112, GAMMA_2_15, luge_a2_da_rtbl13nit, luge_a2_da_ctbl13nit),
	DIM_LUT_V0_INIT(14, 112, GAMMA_2_15, luge_a2_da_rtbl14nit, luge_a2_da_ctbl14nit),
	DIM_LUT_V0_INIT(15, 112, GAMMA_2_15, luge_a2_da_rtbl15nit, luge_a2_da_ctbl15nit),
	DIM_LUT_V0_INIT(16, 112, GAMMA_2_15, luge_a2_da_rtbl16nit, luge_a2_da_ctbl16nit),
	DIM_LUT_V0_INIT(17, 112, GAMMA_2_15, luge_a2_da_rtbl17nit, luge_a2_da_ctbl17nit),
	DIM_LUT_V0_INIT(19, 112, GAMMA_2_15, luge_a2_da_rtbl19nit, luge_a2_da_ctbl19nit),
	DIM_LUT_V0_INIT(20, 112, GAMMA_2_15, luge_a2_da_rtbl20nit, luge_a2_da_ctbl20nit),
	DIM_LUT_V0_INIT(21, 112, GAMMA_2_15, luge_a2_da_rtbl21nit, luge_a2_da_ctbl21nit),
	DIM_LUT_V0_INIT(22, 112, GAMMA_2_15, luge_a2_da_rtbl22nit, luge_a2_da_ctbl22nit),
	DIM_LUT_V0_INIT(24, 112, GAMMA_2_15, luge_a2_da_rtbl24nit, luge_a2_da_ctbl24nit),
	DIM_LUT_V0_INIT(25, 112, GAMMA_2_15, luge_a2_da_rtbl25nit, luge_a2_da_ctbl25nit),
	DIM_LUT_V0_INIT(27, 112, GAMMA_2_15, luge_a2_da_rtbl27nit, luge_a2_da_ctbl27nit),
	DIM_LUT_V0_INIT(29, 112, GAMMA_2_15, luge_a2_da_rtbl29nit, luge_a2_da_ctbl29nit),
	DIM_LUT_V0_INIT(30, 112, GAMMA_2_15, luge_a2_da_rtbl30nit, luge_a2_da_ctbl30nit),
	DIM_LUT_V0_INIT(32, 112, GAMMA_2_15, luge_a2_da_rtbl32nit, luge_a2_da_ctbl32nit),
	DIM_LUT_V0_INIT(34, 112, GAMMA_2_15, luge_a2_da_rtbl34nit, luge_a2_da_ctbl34nit),
	DIM_LUT_V0_INIT(37, 112, GAMMA_2_15, luge_a2_da_rtbl37nit, luge_a2_da_ctbl37nit),
	DIM_LUT_V0_INIT(39, 112, GAMMA_2_15, luge_a2_da_rtbl39nit, luge_a2_da_ctbl39nit),
	DIM_LUT_V0_INIT(41, 112, GAMMA_2_15, luge_a2_da_rtbl41nit, luge_a2_da_ctbl41nit),
	DIM_LUT_V0_INIT(44, 112, GAMMA_2_15, luge_a2_da_rtbl44nit, luge_a2_da_ctbl44nit),
	DIM_LUT_V0_INIT(47, 112, GAMMA_2_15, luge_a2_da_rtbl47nit, luge_a2_da_ctbl47nit),
	DIM_LUT_V0_INIT(50, 112, GAMMA_2_15, luge_a2_da_rtbl50nit, luge_a2_da_ctbl50nit),
	DIM_LUT_V0_INIT(53, 112, GAMMA_2_15, luge_a2_da_rtbl53nit, luge_a2_da_ctbl53nit),
	DIM_LUT_V0_INIT(56, 112, GAMMA_2_15, luge_a2_da_rtbl56nit, luge_a2_da_ctbl56nit),
	DIM_LUT_V0_INIT(60, 112, GAMMA_2_15, luge_a2_da_rtbl60nit, luge_a2_da_ctbl60nit),
	DIM_LUT_V0_INIT(64, 112, GAMMA_2_15, luge_a2_da_rtbl64nit, luge_a2_da_ctbl64nit),
	DIM_LUT_V0_INIT(68, 112, GAMMA_2_15, luge_a2_da_rtbl68nit, luge_a2_da_ctbl68nit),
	DIM_LUT_V0_INIT(72, 112, GAMMA_2_15, luge_a2_da_rtbl72nit, luge_a2_da_ctbl72nit),
	DIM_LUT_V0_INIT(77, 112, GAMMA_2_15, luge_a2_da_rtbl77nit, luge_a2_da_ctbl77nit),
	DIM_LUT_V0_INIT(82, 118, GAMMA_2_15, luge_a2_da_rtbl82nit, luge_a2_da_ctbl82nit),
	DIM_LUT_V0_INIT(87, 126, GAMMA_2_15, luge_a2_da_rtbl87nit, luge_a2_da_ctbl87nit),
	DIM_LUT_V0_INIT(93, 136, GAMMA_2_15, luge_a2_da_rtbl93nit, luge_a2_da_ctbl93nit),
	DIM_LUT_V0_INIT(98, 141, GAMMA_2_15, luge_a2_da_rtbl98nit, luge_a2_da_ctbl98nit),
	DIM_LUT_V0_INIT(105, 152, GAMMA_2_15, luge_a2_da_rtbl105nit, luge_a2_da_ctbl105nit),
	DIM_LUT_V0_INIT(111, 160, GAMMA_2_15, luge_a2_da_rtbl111nit, luge_a2_da_ctbl111nit),
	DIM_LUT_V0_INIT(119, 172, GAMMA_2_15, luge_a2_da_rtbl119nit, luge_a2_da_ctbl119nit),
	DIM_LUT_V0_INIT(126, 180, GAMMA_2_15, luge_a2_da_rtbl126nit, luge_a2_da_ctbl126nit),
	DIM_LUT_V0_INIT(134, 190, GAMMA_2_15, luge_a2_da_rtbl134nit, luge_a2_da_ctbl134nit),
	DIM_LUT_V0_INIT(143, 202, GAMMA_2_15, luge_a2_da_rtbl143nit, luge_a2_da_ctbl143nit),
	DIM_LUT_V0_INIT(152, 216, GAMMA_2_15, luge_a2_da_rtbl152nit, luge_a2_da_ctbl152nit),
	DIM_LUT_V0_INIT(162, 228, GAMMA_2_15, luge_a2_da_rtbl162nit, luge_a2_da_ctbl162nit),
	DIM_LUT_V0_INIT(172, 240, GAMMA_2_15, luge_a2_da_rtbl172nit, luge_a2_da_ctbl172nit),
	DIM_LUT_V0_INIT(183, 253, GAMMA_2_15, luge_a2_da_rtbl183nit, luge_a2_da_ctbl183nit),
	DIM_LUT_V0_INIT(195, 253, GAMMA_2_15, luge_a2_da_rtbl195nit, luge_a2_da_ctbl195nit),
	DIM_LUT_V0_INIT(207, 253, GAMMA_2_15, luge_a2_da_rtbl207nit, luge_a2_da_ctbl207nit),
	DIM_LUT_V0_INIT(220, 253, GAMMA_2_15, luge_a2_da_rtbl220nit, luge_a2_da_ctbl220nit),
	DIM_LUT_V0_INIT(234, 253, GAMMA_2_15, luge_a2_da_rtbl234nit, luge_a2_da_ctbl234nit),
	DIM_LUT_V0_INIT(249, 253, GAMMA_2_15, luge_a2_da_rtbl249nit, luge_a2_da_ctbl249nit),
	DIM_LUT_V0_INIT(265, 268, GAMMA_2_15, luge_a2_da_rtbl265nit, luge_a2_da_ctbl265nit),
	DIM_LUT_V0_INIT(282, 283, GAMMA_2_15, luge_a2_da_rtbl282nit, luge_a2_da_ctbl282nit),
	DIM_LUT_V0_INIT(300, 303, GAMMA_2_15, luge_a2_da_rtbl300nit, luge_a2_da_ctbl300nit),
	DIM_LUT_V0_INIT(316, 319, GAMMA_2_15, luge_a2_da_rtbl316nit, luge_a2_da_ctbl316nit),
	DIM_LUT_V0_INIT(333, 335, GAMMA_2_15, luge_a2_da_rtbl333nit, luge_a2_da_ctbl333nit),
	DIM_LUT_V0_INIT(360, 360, GAMMA_2_20, luge_a2_da_rtbl360nit, luge_a2_da_ctbl360nit),
};

#if PANEL_BACKLIGHT_PAC_STEPS == 512
static unsigned int luge_a2_da_brt_to_step_tbl[S6E3HA2_TOTAL_PAC_STEPS] = {
	0, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 1100, 1200, 1300, 1400, 1500,
	1600, 1700, 1800, 1900, 2000, 2100, 2200, 2300, 2400, 2500, 2600, 2700, 2800, 2900, 3000, 3100,
	3200, 3300, 3400, 3500, 3517, 3533, 3550, 3567, 3583, 3600, 3633, 3667, 3700, 3733, 3767, 3800,
	3833, 3867, 3900, 3933, 3967, 4000, 4029, 4057, 4086, 4114, 4143, 4171, 4200, 4233, 4267, 4300,
	4333, 4367, 4400, 4429, 4457, 4486, 4514, 4543, 4571, 4600, 4629, 4657, 4686, 4714, 4743, 4771,
	4800, 4829, 4857, 4886, 4914, 4943, 4971, 5000, 5033, 5067, 5100, 5133, 5167, 5200, 5229, 5257,
	5286, 5314, 5343, 5371, 5400, 5415, 5431, 5446, 5462, 5477, 5492, 5508, 5523, 5538, 5554, 5569,
	5585, 5600, 5614, 5629, 5643, 5657, 5671, 5686, 5700, 5729, 5757, 5786, 5814, 5843, 5871, 5900,
	5929, 5957, 5986, 6014, 6043, 6071, 6100, 6115, 6131, 6146, 6162, 6177, 6192, 6208, 6223, 6238,
	6254, 6269, 6285, 6300, 6329, 6357, 6386, 6414, 6443, 6471, 6500, 6529, 6557, 6586, 6614, 6643,
	6671, 6700, 6740, 6780, 6820, 6860, 6900, 6933, 6967, 7000, 7040, 7080, 7120, 7160, 7200, 7240,
	7280, 7320, 7360, 7400, 7429, 7457, 7486, 7514, 7543, 7571, 7600, 7650, 7700, 7750, 7800, 7840,
	7880, 7920, 7960, 8000, 8033, 8067, 8100, 8133, 8167, 8200, 8233, 8267, 8300, 8333, 8367, 8400,
	8433, 8467, 8500, 8533, 8567, 8600, 8633, 8667, 8700, 8733, 8767, 8800, 8840, 8880, 8920, 8960,
	9000, 9025, 9050, 9075, 9100, 9125, 9150, 9175, 9200, 9233, 9267, 9300, 9333, 9367, 9400, 9429,
	9457, 9486, 9514, 9543, 9571, 9600, 9633, 9667, 9700, 9733, 9767, 9800, 9825, 9850, 9875, 9900,
	9925, 9950, 9975, 10000, 10025, 10050, 10075, 10100, 10125, 10150, 10175, 10200, 10229, 10257, 10286, 10314,
	10343, 10371, 10400, 10425, 10450, 10475, 10500, 10525, 10550, 10575, 10600, 10629, 10657, 10686, 10714, 10743,
	10771, 10800, 10820, 10840, 10860, 10880, 10900, 10920, 10940, 10960, 10980, 11000, 11029, 11057, 11086, 11114,
	11143, 11171, 11200, 11220, 11240, 11260, 11280, 11300, 11320, 11340, 11360, 11380, 11400, 11425, 11450, 11475,
	11500, 11525, 11550, 11575, 11600, 11620, 11640, 11660, 11680, 11700, 11720, 11740, 11760, 11780, 11800, 11820,
	11840, 11860, 11880, 11900, 11920, 11940, 11960, 11980, 12000, 12022, 12044, 12067, 12089, 12111, 12133, 12156,
	12178, 12200, 12220, 12240, 12260, 12280, 12300, 12320, 12340, 12360, 12380, 12400, 12420, 12440, 12460, 12480,
	12500, 12520, 12540, 12560, 12580, 12600, 12618, 12636, 12655, 12673, 12691, 12709, 12727, 12745, 12764, 12782,
	12800, 12900, 13000, 13100, 13200, 13300, 13400, 13500, 13600, 13700, 13800, 13900, 14000, 14100, 14200, 14300,
	14400, 14500, 14600, 14700, 14800, 14900, 15000, 15100, 15200, 15300, 15400, 15500, 15600, 15700, 15800, 15900,
	16000, 16100, 16200, 16300, 16400, 16500, 16600, 16700, 16800, 16900, 17000, 17100, 17200, 17300, 17400, 17500,
	17600, 17700, 17800, 17900, 18000, 18100, 18200, 18300, 18400, 18500, 18600, 18700, 18800, 18900, 19000, 19100,
	19200, 19300, 19400, 19500, 19600, 19700, 19800, 19900, 20000, 20100, 20200, 20300, 20400, 20500, 20600, 20700,
	20800, 20900, 21000, 21100, 21200, 21300, 21400, 21500, 21600, 21700, 21800, 21900, 22000, 22100, 22200, 22300,
	22400, 22500, 22600, 22700, 22800, 22900, 23000, 23100, 23200, 23300, 23400, 23500, 23600, 23700, 23800, 23900,
	24000, 24100, 24200, 24300, 24400, 24500, 24600, 24700, 24800, 24900, 25000, 25100, 25200, 25300, 25400, 25500,
	/* HBM */
	28100, 29500, 30900, 32300, 33600, 35000, 36400, 36500,
#ifdef CONFIG_LCD_EXTEND_HBM
	36600,
#endif
};
#elif PANEL_BACKLIGHT_PAC_STEPS == 256
static unsigned int luge_a2_da_brt_to_step_tbl[S6E3HA2_TOTAL_PAC_STEPS] = {
	BRT(0), BRT(1), BRT(2), BRT(3), BRT(4), BRT(5), BRT(6), BRT(7), BRT(8), BRT(9), BRT(10), BRT(11), BRT(12), BRT(13), BRT(14), BRT(15),
	BRT(16), BRT(17), BRT(18), BRT(19), BRT(20), BRT(21), BRT(22), BRT(23), BRT(24), BRT(25), BRT(26), BRT(27), BRT(28), BRT(29), BRT(30), BRT(31),
	BRT(32), BRT(33), BRT(34), BRT(35), BRT(36), BRT(37), BRT(38), BRT(39), BRT(40), BRT(41), BRT(42), BRT(43), BRT(44), BRT(45), BRT(46), BRT(47),
	BRT(48), BRT(49), BRT(50), BRT(51), BRT(52), BRT(53), BRT(54), BRT(55), BRT(56), BRT(57), BRT(58), BRT(59), BRT(60), BRT(61), BRT(62), BRT(63),
	BRT(64), BRT(65), BRT(66), BRT(67), BRT(68), BRT(69), BRT(70), BRT(71), BRT(72), BRT(73), BRT(74), BRT(75), BRT(76), BRT(77), BRT(78), BRT(79),
	BRT(80), BRT(81), BRT(82), BRT(83), BRT(84), BRT(85), BRT(86), BRT(87), BRT(88), BRT(89), BRT(90), BRT(91), BRT(92), BRT(93), BRT(94), BRT(95),
	BRT(96), BRT(97), BRT(98), BRT(99), BRT(100), BRT(101), BRT(102), BRT(103), BRT(104), BRT(105), BRT(106), BRT(107), BRT(108), BRT(109), BRT(110), BRT(111),
	BRT(112), BRT(113), BRT(114), BRT(115), BRT(116), BRT(117), BRT(118), BRT(119), BRT(120), BRT(121), BRT(122), BRT(123), BRT(124), BRT(125), BRT(126), BRT(127),
	BRT(128), BRT(129), BRT(130), BRT(131), BRT(132), BRT(133), BRT(134), BRT(135), BRT(136), BRT(137), BRT(138), BRT(139), BRT(140), BRT(141), BRT(142), BRT(143),
	BRT(144), BRT(145), BRT(146), BRT(147), BRT(148), BRT(149), BRT(150), BRT(151), BRT(152), BRT(153), BRT(154), BRT(155), BRT(156), BRT(157), BRT(158), BRT(159),
	BRT(160), BRT(161), BRT(162), BRT(163), BRT(164), BRT(165), BRT(166), BRT(167), BRT(168), BRT(169), BRT(170), BRT(171), BRT(172), BRT(173), BRT(174), BRT(175),
	BRT(176), BRT(177), BRT(178), BRT(179), BRT(180), BRT(181), BRT(182), BRT(183), BRT(184), BRT(185), BRT(186), BRT(187), BRT(188), BRT(189), BRT(190), BRT(191),
	BRT(192), BRT(193), BRT(194), BRT(195), BRT(196), BRT(197), BRT(198), BRT(199), BRT(200), BRT(201), BRT(202), BRT(203), BRT(204), BRT(205), BRT(206), BRT(207),
	BRT(208), BRT(209), BRT(210), BRT(211), BRT(212), BRT(213), BRT(214), BRT(215), BRT(216), BRT(217), BRT(218), BRT(219), BRT(220), BRT(221), BRT(222), BRT(223),
	BRT(224), BRT(225), BRT(226), BRT(227), BRT(228), BRT(229), BRT(230), BRT(231), BRT(232), BRT(233), BRT(234), BRT(235), BRT(236), BRT(237), BRT(238), BRT(239),
	BRT(240), BRT(241), BRT(242), BRT(243), BRT(244), BRT(245), BRT(246), BRT(247), BRT(248), BRT(249), BRT(250), BRT(251), BRT(252), BRT(253), BRT(254), BRT(255),
	/*HBM*/
	BRT(281), BRT(295), BRT(309), BRT(323), BRT(330), BRT(340), BRT(350), BRT(355),
#ifdef CONFIG_LCD_EXTEND_HBM
	BRT(355),
#endif
};
#endif	/* PANEL_BACKLIGHT_PAC_STEPS  */

static unsigned int luge_a2_da_brt_to_brt_tbl[S6E3HA2_TOTAL_PAC_STEPS][2] = {
};

static unsigned int luge_a2_da_brt_tbl[S6E3HA2_TOTAL_NR_LUMINANCE] = {
	BRT(0), BRT(1), BRT(2), BRT(3), BRT(4), BRT(5), BRT(6), BRT(7), BRT(8), BRT(9),
	BRT(10), BRT(11), BRT(12), BRT(13), BRT(14), BRT(15), BRT(16), BRT(17), BRT(18), BRT(19),
	BRT(20), BRT(21), BRT(23), BRT(25), BRT(27), BRT(29), BRT(31), BRT(33), BRT(35), BRT(37),
	BRT(39), BRT(41), BRT(43), BRT(45), BRT(48), BRT(51), BRT(54), BRT(57), BRT(60), BRT(63),
	BRT(66), BRT(69), BRT(72), BRT(76), BRT(80), BRT(86), BRT(91), BRT(97), BRT(104), BRT(110),
	BRT(118), BRT(125), BRT(133), BRT(142), BRT(150), BRT(160), BRT(170), BRT(181), BRT(193), BRT(203),
	BRT(213), BRT(223), BRT(233), BRT(243), BRT(255),
	/* HBM */
	BRT(281), BRT(295), BRT(309), BRT(323), BRT(330), BRT(340), BRT(350), BRT(355),
#ifdef CONFIG_LCD_EXTEND_HBM
	BRT(355),
#endif
};

static unsigned int luge_a2_da_lum_tbl[S6E3HA2_TOTAL_NR_LUMINANCE] = {
	2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
	12, 13, 14, 15, 16, 17, 19, 20, 21, 22,
	24, 25, 27, 29, 30, 32, 34, 37, 39, 41,
	44, 47, 50, 53, 56, 60, 64, 68, 72, 77,
	82, 87, 93, 98, 105, 111, 119, 126, 134, 143,
	152, 162, 172, 183, 195, 207, 220, 234, 249, 265,
	282, 300, 316, 333, 360,
	/* HBM */
	420, 443, 460, 483, 500, 500, 500, 500,
#ifdef CONFIG_LCD_EXTEND_HBM
	/* EXTEND_HBM */
	500,
#endif
};

struct brightness_table s6e3ha2_luge_a2_da_panel_brightness_table = {
	.brt = luge_a2_da_brt_tbl,
	.sz_brt = ARRAY_SIZE(luge_a2_da_brt_tbl),
	.lum = luge_a2_da_lum_tbl,
	.sz_lum = ARRAY_SIZE(luge_a2_da_lum_tbl),
#if PANEL_BACKLIGHT_PAC_STEPS == 512 || PANEL_BACKLIGHT_PAC_STEPS == 256
	.brt_to_step = luge_a2_da_brt_to_step_tbl,
	.sz_brt_to_step = ARRAY_SIZE(luge_a2_da_brt_to_step_tbl),
#else
	.brt_to_step = luge_a2_da_brt_tbl,
	.sz_brt_to_step = ARRAY_SIZE(luge_a2_da_brt_tbl),
#endif
	.brt_to_brt = luge_a2_da_brt_to_brt_tbl,
	.sz_brt_to_brt = ARRAY_SIZE(luge_a2_da_brt_to_brt_tbl),
};

static struct panel_dimming_info s6e3ha2_luge_a2_da_panel_dimming_info = {
	.name = "s6e3ha2_luge_a2_da",
	.dim_init_info = {
		.name = "s6e3ha2_luge_a2_da",
		.nr_tp = S6E3HA2_NR_TP,
		.tp = s6e3ha2_tp,
		.nr_luminance = S6E3HA2_NR_LUMINANCE,
		.vregout = 114085069LL,	/* 6.8 * 2^24 */
		.bitshift = 24,
		.vt_voltage = {
			0, 12, 24, 36, 48, 60, 72, 84, 96, 108,
			138, 148, 158, 168, 178, 186,
		},
		.target_luminance = S6E3HA2_TARGET_LUMINANCE,
		.target_gamma = 220,
		.dim_lut = luge_a2_da_dimming_lut,
	},
	.target_luminance = S6E3HA2_TARGET_LUMINANCE,
	.nr_luminance = S6E3HA2_NR_LUMINANCE,
	.hbm_target_luminance = S6E3HA2_TARGET_HBM_LUMINANCE,
	.nr_hbm_luminance = S6E3HA2_NR_HBM_LUMINANCE,
	.extend_hbm_target_luminance = -1,
	.nr_extend_hbm_luminance = 0,
	.brt_tbl = &s6e3ha2_luge_a2_da_panel_brightness_table,
};
#endif /* __S6E3HA2_LUGE_A2_DA_PANEL_DIMMING_H___ */
