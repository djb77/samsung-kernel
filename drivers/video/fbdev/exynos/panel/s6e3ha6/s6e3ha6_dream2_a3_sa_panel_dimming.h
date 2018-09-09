/*
 * linux/drivers/video/fbdev/exynos/panel/s6e3ha6/s6e3ha6_dream2_a3_sa_panel_dimming.h
 *
 * Header file for S6E3HA6 Dimming Driver
 *
 * Copyright (c) 2016 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __S6E3HA6_DREAM2_A3_SA_PANEL_DIMMING_H___
#define __S6E3HA6_DREAM2_A3_SA_PANEL_DIMMING_H___
#include "../dimming.h"
#include "../panel_dimming.h"
#include "s6e3ha6_dimming.h"

/*
 * PANEL INFORMATION
 * LDI : S6E3HA6
 * PANEL : DREAM2_A3_SA
 */
/* gray scale offset values */
static s32 dream2_a3_sa_rtbl2nit[11] = { 0, 0, 14, 24, 22, 19, 16, 14, 8, 7, 0 };
static s32 dream2_a3_sa_rtbl3nit[11] = { 0, 0, 14, 21, 19, 15, 13, 12, 7, 7, 0 };
static s32 dream2_a3_sa_rtbl4nit[11] = { 0, 0, 14, 17, 15, 12, 10, 8, 5, 5, 0 };
static s32 dream2_a3_sa_rtbl5nit[11] = { 0, 0, 14, 15, 12, 10, 8, 7, 4, 5, 0 };
static s32 dream2_a3_sa_rtbl6nit[11] = { 0, 0, 14, 14, 11, 9, 7, 6, 3, 5, 0 };
static s32 dream2_a3_sa_rtbl7nit[11] = { 0, 0, 13, 13, 10, 8, 6, 6, 3, 5, 0 };
static s32 dream2_a3_sa_rtbl8nit[11] = { 0, 0, 13, 12, 9, 8, 6, 6, 4, 5, 0 };
static s32 dream2_a3_sa_rtbl9nit[11] = { 0, 0, 13, 12, 9, 8, 7, 6, 4, 5, 0 };
static s32 dream2_a3_sa_rtbl10nit[11] = { 0, 0, 13, 12, 9, 8, 7, 6, 4, 4, 0 };
static s32 dream2_a3_sa_rtbl11nit[11] = { 0, 0, 13, 12, 9, 8, 7, 6, 4, 4, 0 };
static s32 dream2_a3_sa_rtbl12nit[11] = { 0, 0, 13, 12, 9, 8, 7, 7, 4, 4, 0 };
static s32 dream2_a3_sa_rtbl13nit[11] = { 0, 0, 14, 13, 10, 9, 8, 7, 4, 4, 0 };
static s32 dream2_a3_sa_rtbl14nit[11] = { 0, 0, 15, 14, 11, 10, 8, 7, 4, 4, 0 };
static s32 dream2_a3_sa_rtbl15nit[11] = { 0, 0, 15, 14, 11, 10, 8, 7, 4, 4, 0 };
static s32 dream2_a3_sa_rtbl16nit[11] = { 0, 0, 14, 14, 10, 9, 7, 6, 3, 4, 0 };
static s32 dream2_a3_sa_rtbl17nit[11] = { 0, 0, 14, 13, 10, 8, 6, 5, 3, 4, 0 };
static s32 dream2_a3_sa_rtbl19nit[11] = { 0, 0, 13, 12, 9, 8, 6, 5, 3, 4, 0 };
static s32 dream2_a3_sa_rtbl20nit[11] = { 0, 0, 13, 12, 9, 7, 6, 5, 3, 4, 0 };
static s32 dream2_a3_sa_rtbl21nit[11] = { 0, 0, 14, 13, 9, 7, 5, 4, 2, 2, 0 };
static s32 dream2_a3_sa_rtbl22nit[11] = { 0, 0, 13, 12, 9, 6, 5, 4, 2, 2, 0 };
static s32 dream2_a3_sa_rtbl24nit[11] = { 0, 0, 12, 11, 8, 6, 4, 3, 2, 2, 0 };
static s32 dream2_a3_sa_rtbl25nit[11] = { 0, 0, 12, 11, 8, 6, 4, 3, 2, 2, 0 };
static s32 dream2_a3_sa_rtbl27nit[11] = { 0, 0, 11, 10, 7, 5, 4, 3, 2, 2, 0 };
static s32 dream2_a3_sa_rtbl29nit[11] = { 0, 0, 11, 10, 7, 5, 4, 3, 2, 2, 0 };
static s32 dream2_a3_sa_rtbl30nit[11] = { 0, 0, 11, 10, 6, 4, 4, 2, 1, 2, 0 };
static s32 dream2_a3_sa_rtbl32nit[11] = { 0, 0, 10, 9, 6, 4, 3, 2, 1, 2, 0 };
static s32 dream2_a3_sa_rtbl34nit[11] = { 0, 0, 10, 9, 6, 4, 3, 2, 1, 2, 0 };
static s32 dream2_a3_sa_rtbl37nit[11] = { 0, 0, 9, 8, 5, 3, 2, 2, 1, 2, 0 };
static s32 dream2_a3_sa_rtbl39nit[11] = { 0, 0, 9, 8, 5, 3, 2, 2, 1, 2, 0 };
static s32 dream2_a3_sa_rtbl41nit[11] = { 0, 0, 7, 7, 4, 2, 1, 1, 0, 1, 0 };
static s32 dream2_a3_sa_rtbl44nit[11] = { 0, 0, 7, 7, 4, 2, 1, 1, 0, 1, 0 };
static s32 dream2_a3_sa_rtbl47nit[11] = { 0, 0, 7, 6, 4, 2, 1, 1, 0, 1, 0 };
static s32 dream2_a3_sa_rtbl50nit[11] = { 0, 0, 6, 6, 3, 2, 1, 1, 0, 1, 0 };
static s32 dream2_a3_sa_rtbl53nit[11] = { 0, 0, 6, 5, 3, 2, 1, 1, 0, 1, 0 };
static s32 dream2_a3_sa_rtbl56nit[11] = { 0, 0, 6, 5, 3, 2, 1, 1, 0, 1, 0 };
static s32 dream2_a3_sa_rtbl60nit[11] = { 0, 0, 6, 5, 3, 2, 1, 1, 0, 1, 0 };
static s32 dream2_a3_sa_rtbl64nit[11] = { 0, 0, 6, 4, 2, 1, 1, 1, 0, 1, 0 };
static s32 dream2_a3_sa_rtbl68nit[11] = { 0, 0, 6, 4, 3, 1, 1, 1, 1, 1, 0 };
static s32 dream2_a3_sa_rtbl72nit[11] = { 0, 0, 6, 4, 3, 1, 1, 1, 1, 1, 0 };
static s32 dream2_a3_sa_rtbl77nit[11] = { 0, 0, 5, 4, 2, 2, 1, 1, 1, 1, 0 };
static s32 dream2_a3_sa_rtbl82nit[11] = { 0, 0, 5, 4, 2, 1, 1, 1, 0, 1, 0 };
static s32 dream2_a3_sa_rtbl87nit[11] = { 0, 0, 5, 4, 2, 1, 1, 1, 1, 2, 0 };
static s32 dream2_a3_sa_rtbl93nit[11] = { 0, 0, 5, 4, 2, 2, 1, 2, 3, 2, 0 };
static s32 dream2_a3_sa_rtbl98nit[11] = { 0, 0, 6, 4, 3, 3, 3, 3, 3, 2, 0 };
static s32 dream2_a3_sa_rtbl105nit[11] = { 0, 0, 6, 5, 2, 2, 2, 3, 4, 1, 0 };
static s32 dream2_a3_sa_rtbl111nit[11] = { 0, 0, 6, 4, 2, 2, 2, 2, 4, 1, 0 };
static s32 dream2_a3_sa_rtbl119nit[11] = { 0, 0, 5, 4, 3, 2, 2, 2, 4, 1, 0 };
static s32 dream2_a3_sa_rtbl126nit[11] = { 0, 0, 5, 4, 3, 2, 1, 2, 3, 1, 0 };
static s32 dream2_a3_sa_rtbl134nit[11] = { 0, 0, 5, 4, 2, 2, 1, 2, 3, 1, 0 };
static s32 dream2_a3_sa_rtbl143nit[11] = { 0, 0, 5, 4, 2, 2, 1, 2, 2, 1, 0 };
static s32 dream2_a3_sa_rtbl152nit[11] = { 0, 0, 5, 4, 2, 2, 2, 3, 5, 2, 0 };
static s32 dream2_a3_sa_rtbl162nit[11] = { 0, 0, 5, 4, 2, 2, 3, 3, 5, 2, 0 };
static s32 dream2_a3_sa_rtbl172nit[11] = { 0, 0, 5, 4, 1, 2, 3, 3, 5, 2, 0 };
static s32 dream2_a3_sa_rtbl183nit[11] = { 0, 0, 3, 3, 2, 2, 3, 4, 5, 3, 0 };
static s32 dream2_a3_sa_rtbl195nit[11] = { 0, 0, 3, 3, 1, 2, 2, 1, 4, 1, 0 };
static s32 dream2_a3_sa_rtbl207nit[11] = { 0, 0, 2, 2, 1, 2, 2, 1, 3, 1, 0 };
static s32 dream2_a3_sa_rtbl220nit[11] = { 0, 0, 2, 2, 1, 1, 1, 1, 3, 1, 0 };
static s32 dream2_a3_sa_rtbl234nit[11] = { 0, 0, 2, 1, 0, 1, 1, 0, 2, 0, 0 };
static s32 dream2_a3_sa_rtbl249nit[11] = { 0, 0, 2, 1, 0, 0, 0, 0, 2, 0, 0 };
static s32 dream2_a3_sa_rtbl265nit[11] = { 0, 0, 1, 1, 0, 0, 0, 0, 2, 1, 0 };
static s32 dream2_a3_sa_rtbl282nit[11] = { 0, 0, 1, 0, 0, 0, 0, 0, 2, 1, 0 };
static s32 dream2_a3_sa_rtbl300nit[11] = { 0, 0, 1, 0, 0, -1, 0, 0, 2, 1, 0 };
static s32 dream2_a3_sa_rtbl316nit[11] = { 0, 0, 1, 0, 0, 0, 0, 0, 2, 1, 0 };
static s32 dream2_a3_sa_rtbl333nit[11] = { 0, 0, 1, 0, 0, 0, 0, 1, 1, 1, 0 };
static s32 dream2_a3_sa_rtbl350nit[11] = { 0, 0, 0, 0, -1, -1, 0, 0, 1, 1, 0 };
static s32 dream2_a3_sa_rtbl357nit[11] = { 0, 0, 0, -1, -1, -1, -1, -1, 1, 1, 0 };
static s32 dream2_a3_sa_rtbl365nit[11] = { 0, 0, 0, -1, -1, 0, -1, -1, 1, 0, 0 };
static s32 dream2_a3_sa_rtbl372nit[11] = { 0, 0, 0, -1, -1, 0, -1, -1, 1, 0, 0 };
static s32 dream2_a3_sa_rtbl380nit[11] = { 0, 0, 0, -1, 0, 0, -1, -1, 1, 0, 0 };
static s32 dream2_a3_sa_rtbl387nit[11] = { 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0 };
static s32 dream2_a3_sa_rtbl395nit[11] = { 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0 };
static s32 dream2_a3_sa_rtbl403nit[11] = { 0, 0, 1, 0, 0, 0, -1, -1, 0, 0, 0 };
static s32 dream2_a3_sa_rtbl412nit[11] = { 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0 };
static s32 dream2_a3_sa_rtbl420nit[11] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

/* rgb color offset values */
static s32 dream2_a3_sa_ctbl2nit[33] = { 0, 0, 0, 0, 0, 0, 3, 0, 3, -3, 0, 0, -6, 0, -6, -5, 0, -7, -4, 0, -6, -2, 0, -2, -2, 0, -2, -1, 0, -1, -3, 0, -3 };
static s32 dream2_a3_sa_ctbl3nit[33] = { 0, 0, 0, 0, 0, 0, 3, 0, 3, -3, 0, -1, -7, 0, -7, -7, 0, -8, -3, 0, -5, -2, 0, -4, -2, 0, -2, -2, 0, -1, -3, 0, -3 };
static s32 dream2_a3_sa_ctbl4nit[33] = { 0, 0, 0, 0, 0, 0, 3, 0, 3, -3, 0, -2, -7, 0, -8, -6, 0, -6, -3, 0, -6, -2, 0, -3, -2, 0, -2, -2, 0, 0, -2, 0, -3 };
static s32 dream2_a3_sa_ctbl5nit[33] = { 0, 0, 0, 0, 0, 0, 3, 0, 3, -2, 0, -3, -7, 0, -8, -5, 0, -5, -1, 0, -4, -2, 0, -4, -2, 0, -2, -2, 0, 0, -1, 0, -2 };
static s32 dream2_a3_sa_ctbl6nit[33] = { 0, 0, 0, 0, 0, 0, 3, 0, 3, -1, 0, -5, -6, 0, -8, -3, 0, -4, -1, 0, -5, -3, 0, -3, -2, 0, -1, -1, 0, 0, -1, 0, -2 };
static s32 dream2_a3_sa_ctbl7nit[33] = { 0, 0, 0, 0, 0, 0, 3, 0, 3, 0, 0, -6, -6, 0, -7, -2, 0, -3, -1, 0, -5, -1, 0, -3, -2, 0, 0, -1, 0, 0, -1, 0, -2 };
static s32 dream2_a3_sa_ctbl8nit[33] = { 0, 0, 0, 0, 0, 0, 3, 0, 2, 0, 0, -8, -6, 0, -6, -2, 0, -4, -2, 0, -5, -2, 0, -3, -1, 0, 0, -1, 0, 0, -1, 0, -2 };
static s32 dream2_a3_sa_ctbl9nit[33] = { 0, 0, 0, 0, 0, 0, 3, 0, 1, 1, 0, -8, -5, 0, -7, -1, 0, -4, -2, 0, -4, -2, 0, -3, -1, 0, 0, -1, 0, 0, -1, 0, -2 };
static s32 dream2_a3_sa_ctbl10nit[33] = { 0, 0, 0, 0, 0, 0, 4, 0, 1, 1, 0, -8, -3, 0, -7, -1, 0, -5, -2, 0, -4, -2, 0, -3, -1, 0, -1, -2, 0, -1, 0, 0, -1 };
static s32 dream2_a3_sa_ctbl11nit[33] = { 0, 0, 0, 0, 0, 0, 4, 0, 1, 2, 0, -8, -2, 0, -7, -1, 0, -6, -2, 0, -4, -2, 0, -3, -1, 0, -1, -2, 0, -1, 0, 0, -1 };
static s32 dream2_a3_sa_ctbl12nit[33] = { 0, 0, 0, 0, 0, 0, 4, 0, 1, 4, 0, -9, -2, 0, -8, -1, 0, -6, -2, 0, -5, -2, 0, -4, -1, 0, 0, -1, 0, -1, 0, 0, -1 };
static s32 dream2_a3_sa_ctbl13nit[33] = { 0, 0, 0, 0, 0, 0, 4, 0, 1, 3, 0, -11, -2, 0, -8, -1, 0, -8, -3, 0, -5, -1, 0, -4, -1, 0, 0, -1, 0, -1, 0, 0, -1 };
static s32 dream2_a3_sa_ctbl14nit[33] = { 0, 0, 0, 0, 0, 0, 3, 0, 0, 3, 0, -12, 0, 0, -9, -2, 0, -8, -3, 0, -6, -1, 0, -4, -2, 0, -1, -1, 0, -1, 1, 0, 0 };
static s32 dream2_a3_sa_ctbl15nit[33] = { 0, 0, 0, 0, 0, 0, 4, 0, 1, 4, 0, -13, 0, 0, -11, -2, 0, -9, -3, 0, -6, -1, 0, -4, -2, 0, -1, -1, 0, -1, 1, 0, 0 };
static s32 dream2_a3_sa_ctbl16nit[33] = { 0, 0, 0, 0, 0, 0, 4, 0, 0, 4, 0, -12, 0, 0, -10, -2, 0, -8, -2, 0, -6, -2, 0, -3, -1, 0, 0, -1, 0, -1, 1, 0, 0 };
static s32 dream2_a3_sa_ctbl17nit[33] = { 0, 0, 0, 0, 0, 0, 4, 0, 0, 5, 0, -12, 0, 0, -8, -3, 0, -9, -2, 0, -6, -1, 0, -3, -1, 0, 0, -1, 0, -1, 1, 0, 0 };
static s32 dream2_a3_sa_ctbl19nit[33] = { 0, 0, 0, 0, 0, 0, 4, 0, -1, 5, 0, -13, 0, 0, -9, -3, 0, -8, -1, 0, -4, -1, 0, -3, -1, 0, -1, -1, 0, 0, 1, 0, 0 };
static s32 dream2_a3_sa_ctbl20nit[33] = { 0, 0, 0, 0, 0, 0, 5, 0, -1, 5, 0, -12, 0, 0, -8, -3, 0, -9, -3, 0, -6, -1, 0, -4, 0, 0, 0, 0, 0, 1, 1, 0, 1 };
static s32 dream2_a3_sa_ctbl21nit[33] = { 0, 0, 0, 0, 0, 0, 3, 0, -3, 0, 0, -8, 0, 0, -7, -2, 0, -9, -2, 0, -5, -1, 0, -3, 0, 0, 1, -1, 0, 0, 1, 0, 0 };
static s32 dream2_a3_sa_ctbl22nit[33] = { 0, 0, 0, 0, 0, 0, 3, 0, -3, 2, 0, -8, 0, 0, -6, -2, 0, -9, -2, 0, -5, -1, 0, -2, 0, 0, 1, -1, 0, 0, 1, 0, 0 };
static s32 dream2_a3_sa_ctbl24nit[33] = { 0, 0, 0, 0, 0, 0, 3, 0, -3, 3, 0, -7, 0, 0, -6, -1, 0, -8, -2, 0, -5, 0, 0, -1, 0, 0, 1, -1, 0, 0, 1, 0, 0 };
static s32 dream2_a3_sa_ctbl25nit[33] = { 0, 0, 0, 0, 0, 0, 3, 0, -3, 3, 0, -6, 0, 0, -5, -1, 0, -7, -2, 0, -5, 0, 0, -1, 0, 0, 0, -1, 0, 0, 1, 0, 0 };
static s32 dream2_a3_sa_ctbl27nit[33] = { 0, 0, 0, 0, 0, 0, 3, 0, -3, 4, 0, -7, 1, 0, -7, -1, 0, -7, -2, 0, -5, 0, 0, -1, 0, 0, 1, -1, 0, 0, 1, 0, 1 };
static s32 dream2_a3_sa_ctbl29nit[33] = { 0, 0, 0, 0, 0, 0, 2, 0, -4, 2, 0, -4, 1, 0, -6, -2, 0, -8, -2, 0, -5, 1, 0, 0, 0, 0, 1, -1, 0, 0, 1, 0, 1 };
static s32 dream2_a3_sa_ctbl30nit[33] = { 0, 0, 0, 0, 0, 0, 2, 0, -3, 1, 0, -4, 1, 0, -7, -3, 0, -8, -2, 0, -5, 1, 0, 0, 0, 0, 1, -1, 0, 0, 1, 0, 1 };
static s32 dream2_a3_sa_ctbl32nit[33] = { 0, 0, 0, 0, 0, 0, 2, 0, -3, 3, 0, -6, 1, 0, -6, -3, 0, -7, -2, 0, -5, 1, 0, 0, 0, 0, 1, -1, 0, 0, 1, 0, 1 };
static s32 dream2_a3_sa_ctbl34nit[33] = { 0, 0, 0, 0, 0, 0, 2, 0, -3, 2, 0, -5, 0, 0, -6, -3, 0, -7, -1, 0, -4, 1, 0, 0, 0, 0, 1, -1, 0, 0, 1, 0, 1 };
static s32 dream2_a3_sa_ctbl37nit[33] = { 0, 0, 0, 0, 0, 0, 2, 0, -3, 2, 0, -3, 1, 0, -4, -2, 0, -6, -1, 0, -3, 0, 0, -1, 1, 0, 2, -1, 0, 0, 1, 0, 1 };
static s32 dream2_a3_sa_ctbl39nit[33] = { 0, 0, 0, 0, 0, 0, 2, 0, -3, 2, 0, -1, 1, 0, -3, -2, 0, -6, -1, 0, -3, 0, 0, -1, 1, 0, 2, -1, 0, 0, 1, 0, 1 };
static s32 dream2_a3_sa_ctbl41nit[33] = { 0, 0, 0, 0, 0, 0, 4, 0, -6, 3, 0, -2, 2, 0, -5, -1, 0, -5, -2, 0, -3, 1, 0, -1, 1, 0, 2, -1, 0, 0, 1, 0, 1 };
static s32 dream2_a3_sa_ctbl44nit[33] = { 0, 0, 0, 0, 0, 0, 4, 0, -6, 2, 0, -3, 2, 0, -2, 0, 0, -5, -2, 0, -3, 1, 0, -2, 1, 0, 2, -1, 0, 0, 1, 0, 2 };
static s32 dream2_a3_sa_ctbl47nit[33] = { 0, 0, 0, 0, 0, 0, 3, 0, -3, 2, 0, -4, 1, 0, -2, 0, 0, -4, -1, 0, -2, 1, 0, -2, 1, 0, 2, -1, 0, 0, 1, 0, 2 };
static s32 dream2_a3_sa_ctbl50nit[33] = { 0, 0, 0, 0, 0, 0, 6, 0, -4, 2, 0, 0, 1, 0, -1, 0, 0, -4, -1, 0, -3, 1, 0, -1, 1, 0, 2, -1, 0, 0, 1, 0, 2 };
static s32 dream2_a3_sa_ctbl53nit[33] = { 0, 0, 0, 0, 0, 0, 5, 0, -1, 3, 0, -2, 3, 0, -1, 0, 0, -3, -1, 0, -3, 1, 0, -1, 1, 0, 2, -1, 0, 0, 1, 0, 2 };
static s32 dream2_a3_sa_ctbl56nit[33] = { 0, 0, 0, 0, 0, 0, 5, 0, 0, 2, 0, -2, 3, 0, 0, 0, 0, -3, 0, 0, -2, 1, 0, -1, 1, 0, 2, -1, 0, 0, 1, 0, 2 };
static s32 dream2_a3_sa_ctbl60nit[33] = { 0, 0, 0, 0, 0, 0, 5, 0, 0, 2, 0, -1, 5, 0, 0, -1, 0, -3, 0, 0, -2, 1, 0, 1, 0, 0, 2, -1, 0, 0, 1, 0, 2 };
static s32 dream2_a3_sa_ctbl64nit[33] = { 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, -1, 2, 0, 2, -1, 0, -4, 1, 0, -2, 0, 0, 0, 1, 0, 1, -1, 0, 0, 2, 0, 2 };
static s32 dream2_a3_sa_ctbl68nit[33] = { 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, -1, 2, 0, 3, -1, 0, -4, 1, 0, -2, 0, 0, 0, 1, 0, 2, -1, 0, 0, 2, 0, 2 };
static s32 dream2_a3_sa_ctbl72nit[33] = { 0, 0, 0, 0, 0, 0, 5, 0, 0, -1, 0, 0, 1, 0, 4, -1, 0, -3, 1, 0, -2, 0, 0, 0, 1, 0, 1, -2, 0, 0, 2, 0, 2 };
static s32 dream2_a3_sa_ctbl77nit[33] = { 0, 0, 0, 0, 0, 0, 5, 0, -1, 0, 0, -4, 1, 0, 0, -1, 0, -3, 1, 0, -2, 1, 0, 1, 1, 0, 2, -1, 0, 0, 2, 0, 2 };
static s32 dream2_a3_sa_ctbl82nit[33] = { 0, 0, 0, 0, 0, 0, 5, 0, -1, 0, 0, -2, 2, 0, 0, -1, 0, -2, 1, 0, -2, 0, 0, 0, 0, 0, 2, 0, 0, 0, 1, 0, 2 };
static s32 dream2_a3_sa_ctbl87nit[33] = { 0, 0, 0, 0, 0, 0, 5, 0, -2, -2, 0, -6, 2, 0, -1, -1, 0, -2, 1, 0, -1, 0, 0, 0, 0, 0, 1, -1, 0, 0, 2, 0, 2 };
static s32 dream2_a3_sa_ctbl93nit[33] = { 0, 0, 0, 0, 0, 0, 5, 0, -3, 0, 0, -4, 2, 0, -1, -1, 0, -2, 1, 0, -1, 1, 0, 1, 0, 0, 1, -1, 0, -1, 1, 0, 2 };
static s32 dream2_a3_sa_ctbl98nit[33] = { 0, 0, 0, 0, 0, 0, 2, 0, -3, 2, 0, -6, 2, 0, -2, 1, 0, -2, 0, 0, -1, 1, 0, 1, -2, 0, 0, 0, 0, 0, 2, 0, 3 };
static s32 dream2_a3_sa_ctbl105nit[33] = { 0, 0, 0, 0, 0, 0, 2, 0, -5, 1, 0, -6, 1, 0, -3, -1, 0, -1, 2, 0, -1, 1, 0, 1, -1, 0, -1, 0, 0, 1, 2, 0, 3 };
static s32 dream2_a3_sa_ctbl111nit[33] = { 0, 0, 0, 0, 0, 0, 2, 0, -5, 2, 0, -4, 0, 0, -3, 1, 0, -1, 0, 0, -1, 0, 0, 0, 1, 0, 1, -1, 0, 0, 3, 0, 3 };
static s32 dream2_a3_sa_ctbl119nit[33] = { 0, 0, 0, 0, 0, 0, 2, 0, -5, 4, 0, -4, -1, 0, -3, 1, 0, -1, -1, 0, -1, 2, 0, 2, -1, 0, -1, -1, 0, 0, 3, 0, 3 };
static s32 dream2_a3_sa_ctbl126nit[33] = { 0, 0, 0, 0, 0, 0, 2, 0, -5, 1, 0, -8, -1, 0, -3, 0, 0, -1, 0, 0, 0, 1, 0, 1, -1, 0, 0, 1, 0, 1, 3, 0, 3 };
static s32 dream2_a3_sa_ctbl134nit[33] = { 0, 0, 0, 0, 0, 0, 2, 0, -5, 2, 0, -6, 0, 0, -3, 0, 0, -1, 0, 0, 0, 1, 0, 1, 0, 0, 0, -1, 0, 0, 3, 0, 4 };
static s32 dream2_a3_sa_ctbl143nit[33] = { 0, 0, 0, 0, 0, 0, 2, 0, -4, 0, 0, -7, 0, 0, -1, 0, 0, -1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 3, 0, 4 };
static s32 dream2_a3_sa_ctbl152nit[33] = { 0, 0, 0, 0, 0, 0, 3, 0, -5, 2, 0, -4, -1, 0, -2, 0, 0, -1, 0, 0, 0, 2, 0, 1, -1, 0, 0, 0, 0, 0, 3, 0, 4 };
static s32 dream2_a3_sa_ctbl162nit[33] = { 0, 0, 0, 0, 0, 0, 2, 0, -5, 5, 0, -4, 0, 0, -2, -1, 0, -1, 1, 0, 1, 0, 0, 1, -1, 0, 0, 0, 0, 0, 2, 0, 3 };
static s32 dream2_a3_sa_ctbl172nit[33] = { 0, 0, 0, 0, 0, 0, 1, 0, -6, 3, 0, -2, 0, 0, -2, -1, 0, -1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 2, 0, 3 };
static s32 dream2_a3_sa_ctbl183nit[33] = { 0, 0, 0, 0, 0, 0, 5, 0, -2, 3, 0, -2, 1, 0, -1, -1, 0, -2, 2, 0, 0, -1, 0, 1, 0, 0, 0, -1, 0, 0, 2, 0, 3 };
static s32 dream2_a3_sa_ctbl195nit[33] = { 0, 0, 0, 0, 0, 0, 4, 0, -2, 3, 0, 1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 2, 0, 3 };
static s32 dream2_a3_sa_ctbl207nit[33] = { 0, 0, 0, 0, 0, 0, 7, 0, 0, 4, 0, 1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 2, 0, 3 };
static s32 dream2_a3_sa_ctbl220nit[33] = { 0, 0, 0, 0, 0, 0, 7, 0, 0, 4, 0, 1, 0, 0, -1, 1, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 2, 0, 3 };
static s32 dream2_a3_sa_ctbl234nit[33] = { 0, 0, 0, 0, 0, 0, 6, 0, 2, 4, 0, -1, 0, 0, -1, 1, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 0, -1, 0, 0, 2, 0, 3 };
static s32 dream2_a3_sa_ctbl249nit[33] = { 0, 0, 0, 0, 0, 0, 6, 0, 2, 4, 0, 1, 0, 0, -1, 1, 0, 0, -1, 0, 0, 2, 0, 1, -1, 0, 0, -1, 0, 0, 2, 0, 3 };
static s32 dream2_a3_sa_ctbl265nit[33] = { 0, 0, 0, 0, 0, 0, 10, 0, 5, 5, 0, 2, 0, 0, -1, 1, 0, 0, -1, 0, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 1, 0, 2 };
static s32 dream2_a3_sa_ctbl282nit[33] = { 0, 0, 0, 0, 0, 0, 9, 0, 5, 4, 0, 2, 0, 0, -1, 0, 0, 0, 1, 0, 2, 2, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 2 };
static s32 dream2_a3_sa_ctbl300nit[33] = { 0, 0, 0, 0, 0, 0, 10, 0, 6, 3, 0, 4, 1, 0, -1, 2, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 0, -1, 0, -1, 1, 0, 2 };
static s32 dream2_a3_sa_ctbl316nit[33] = { 0, 0, 0, 0, 0, 0, 9, 0, 6, 6, 0, 4, 0, 0, 0, 0, 0, -1, 0, 0, 1, 2, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 2 };
static s32 dream2_a3_sa_ctbl333nit[33] = { 0, 0, 0, 0, 0, 0, 9, 0, 7, 6, 0, 5, 0, 0, -1, 1, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
static s32 dream2_a3_sa_ctbl350nit[33] = { 0, 0, 0, 0, 0, 0, 12, 0, 10, 7, 0, 6, -1, 0, -1, 2, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
static s32 dream2_a3_sa_ctbl357nit[33] = { 0, 0, 0, 0, 0, 0, 11, 0, 10, 7, 0, 6, 0, 0, 0, 1, 0, 0, -1, 0, 0, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1 };
static s32 dream2_a3_sa_ctbl365nit[33] = { 0, 0, 0, 0, 0, 0, 12, 0, 11, 7, 0, 7, 1, 0, 1, 0, 0, -1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1 };
static s32 dream2_a3_sa_ctbl372nit[33] = { 0, 0, 0, 0, 0, 0, 12, 0, 11, 7, 0, 7, 1, 0, 1, 0, 0, -1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
static s32 dream2_a3_sa_ctbl380nit[33] = { 0, 0, 0, 0, 0, 0, 11, 0, 11, 8, 0, 7, -1, 0, 0, 0, 0, -1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
static s32 dream2_a3_sa_ctbl387nit[33] = { 0, 0, 0, 0, 0, 0, 11, 0, 11, 7, 0, 7, -1, 0, 0, 0, 0, -1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
static s32 dream2_a3_sa_ctbl395nit[33] = { 0, 0, 0, 0, 0, 0, -2, 0, 1, 1, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
static s32 dream2_a3_sa_ctbl403nit[33] = { 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 0, 2, -1, 0, 0, 1, 0, 0, -1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
static s32 dream2_a3_sa_ctbl412nit[33] = { 0, 0, 0, 0, 0, 0, -1, 0, 2, 2, 0, 3, -1, 0, 0, 1, 0, 0, -1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
static s32 dream2_a3_sa_ctbl420nit[33] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

static struct dimming_lut dream2_a3_sa_dimming_lut[] = {
	DIM_LUT_V0_INIT(2, 109, GAMMA_2_15, dream2_a3_sa_rtbl2nit, dream2_a3_sa_ctbl2nit),
	DIM_LUT_V0_INIT(3, 109, GAMMA_2_15, dream2_a3_sa_rtbl3nit, dream2_a3_sa_ctbl3nit),
	DIM_LUT_V0_INIT(4, 109, GAMMA_2_15, dream2_a3_sa_rtbl4nit, dream2_a3_sa_ctbl4nit),
	DIM_LUT_V0_INIT(5, 109, GAMMA_2_15, dream2_a3_sa_rtbl5nit, dream2_a3_sa_ctbl5nit),
	DIM_LUT_V0_INIT(6, 109, GAMMA_2_15, dream2_a3_sa_rtbl6nit, dream2_a3_sa_ctbl6nit),
	DIM_LUT_V0_INIT(7, 109, GAMMA_2_15, dream2_a3_sa_rtbl7nit, dream2_a3_sa_ctbl7nit),
	DIM_LUT_V0_INIT(8, 109, GAMMA_2_15, dream2_a3_sa_rtbl8nit, dream2_a3_sa_ctbl8nit),
	DIM_LUT_V0_INIT(9, 109, GAMMA_2_15, dream2_a3_sa_rtbl9nit, dream2_a3_sa_ctbl9nit),
	DIM_LUT_V0_INIT(10, 109, GAMMA_2_15, dream2_a3_sa_rtbl10nit, dream2_a3_sa_ctbl10nit),
	DIM_LUT_V0_INIT(11, 109, GAMMA_2_15, dream2_a3_sa_rtbl11nit, dream2_a3_sa_ctbl11nit),
	DIM_LUT_V0_INIT(12, 109, GAMMA_2_15, dream2_a3_sa_rtbl12nit, dream2_a3_sa_ctbl12nit),
	DIM_LUT_V0_INIT(13, 109, GAMMA_2_15, dream2_a3_sa_rtbl13nit, dream2_a3_sa_ctbl13nit),
	DIM_LUT_V0_INIT(14, 109, GAMMA_2_15, dream2_a3_sa_rtbl14nit, dream2_a3_sa_ctbl14nit),
	DIM_LUT_V0_INIT(15, 109, GAMMA_2_15, dream2_a3_sa_rtbl15nit, dream2_a3_sa_ctbl15nit),
	DIM_LUT_V0_INIT(16, 109, GAMMA_2_15, dream2_a3_sa_rtbl16nit, dream2_a3_sa_ctbl16nit),
	DIM_LUT_V0_INIT(17, 109, GAMMA_2_15, dream2_a3_sa_rtbl17nit, dream2_a3_sa_ctbl17nit),
	DIM_LUT_V0_INIT(19, 109, GAMMA_2_15, dream2_a3_sa_rtbl19nit, dream2_a3_sa_ctbl19nit),
	DIM_LUT_V0_INIT(20, 109, GAMMA_2_15, dream2_a3_sa_rtbl20nit, dream2_a3_sa_ctbl20nit),
	DIM_LUT_V0_INIT(21, 109, GAMMA_2_15, dream2_a3_sa_rtbl21nit, dream2_a3_sa_ctbl21nit),
	DIM_LUT_V0_INIT(22, 109, GAMMA_2_15, dream2_a3_sa_rtbl22nit, dream2_a3_sa_ctbl22nit),
	DIM_LUT_V0_INIT(24, 109, GAMMA_2_15, dream2_a3_sa_rtbl24nit, dream2_a3_sa_ctbl24nit),
	DIM_LUT_V0_INIT(25, 109, GAMMA_2_15, dream2_a3_sa_rtbl25nit, dream2_a3_sa_ctbl25nit),
	DIM_LUT_V0_INIT(27, 109, GAMMA_2_15, dream2_a3_sa_rtbl27nit, dream2_a3_sa_ctbl27nit),
	DIM_LUT_V0_INIT(29, 109, GAMMA_2_15, dream2_a3_sa_rtbl29nit, dream2_a3_sa_ctbl29nit),
	DIM_LUT_V0_INIT(30, 109, GAMMA_2_15, dream2_a3_sa_rtbl30nit, dream2_a3_sa_ctbl30nit),
	DIM_LUT_V0_INIT(32, 109, GAMMA_2_15, dream2_a3_sa_rtbl32nit, dream2_a3_sa_ctbl32nit),
	DIM_LUT_V0_INIT(34, 109, GAMMA_2_15, dream2_a3_sa_rtbl34nit, dream2_a3_sa_ctbl34nit),
	DIM_LUT_V0_INIT(37, 109, GAMMA_2_15, dream2_a3_sa_rtbl37nit, dream2_a3_sa_ctbl37nit),
	DIM_LUT_V0_INIT(39, 109, GAMMA_2_15, dream2_a3_sa_rtbl39nit, dream2_a3_sa_ctbl39nit),
	DIM_LUT_V0_INIT(41, 109, GAMMA_2_15, dream2_a3_sa_rtbl41nit, dream2_a3_sa_ctbl41nit),
	DIM_LUT_V0_INIT(44, 109, GAMMA_2_15, dream2_a3_sa_rtbl44nit, dream2_a3_sa_ctbl44nit),
	DIM_LUT_V0_INIT(47, 109, GAMMA_2_15, dream2_a3_sa_rtbl47nit, dream2_a3_sa_ctbl47nit),
	DIM_LUT_V0_INIT(50, 109, GAMMA_2_15, dream2_a3_sa_rtbl50nit, dream2_a3_sa_ctbl50nit),
	DIM_LUT_V0_INIT(53, 109, GAMMA_2_15, dream2_a3_sa_rtbl53nit, dream2_a3_sa_ctbl53nit),
	DIM_LUT_V0_INIT(56, 109, GAMMA_2_15, dream2_a3_sa_rtbl56nit, dream2_a3_sa_ctbl56nit),
	DIM_LUT_V0_INIT(60, 112, GAMMA_2_15, dream2_a3_sa_rtbl60nit, dream2_a3_sa_ctbl60nit),
	DIM_LUT_V0_INIT(64, 119, GAMMA_2_15, dream2_a3_sa_rtbl64nit, dream2_a3_sa_ctbl64nit),
	DIM_LUT_V0_INIT(68, 125, GAMMA_2_15, dream2_a3_sa_rtbl68nit, dream2_a3_sa_ctbl68nit),
	DIM_LUT_V0_INIT(72, 133, GAMMA_2_15, dream2_a3_sa_rtbl72nit, dream2_a3_sa_ctbl72nit),
	DIM_LUT_V0_INIT(77, 142, GAMMA_2_15, dream2_a3_sa_rtbl77nit, dream2_a3_sa_ctbl77nit),
	DIM_LUT_V0_INIT(82, 151, GAMMA_2_15, dream2_a3_sa_rtbl82nit, dream2_a3_sa_ctbl82nit),
	DIM_LUT_V0_INIT(87, 161, GAMMA_2_15, dream2_a3_sa_rtbl87nit, dream2_a3_sa_ctbl87nit),
	DIM_LUT_V0_INIT(93, 170, GAMMA_2_15, dream2_a3_sa_rtbl93nit, dream2_a3_sa_ctbl93nit),
	DIM_LUT_V0_INIT(98, 183, GAMMA_2_15, dream2_a3_sa_rtbl98nit, dream2_a3_sa_ctbl98nit),
	DIM_LUT_V0_INIT(105, 193, GAMMA_2_15, dream2_a3_sa_rtbl105nit, dream2_a3_sa_ctbl105nit),
	DIM_LUT_V0_INIT(111, 205, GAMMA_2_15, dream2_a3_sa_rtbl111nit, dream2_a3_sa_ctbl111nit),
	DIM_LUT_V0_INIT(119, 217, GAMMA_2_15, dream2_a3_sa_rtbl119nit, dream2_a3_sa_ctbl119nit),
	DIM_LUT_V0_INIT(126, 230, GAMMA_2_15, dream2_a3_sa_rtbl126nit, dream2_a3_sa_ctbl126nit),
	DIM_LUT_V0_INIT(134, 241, GAMMA_2_15, dream2_a3_sa_rtbl134nit, dream2_a3_sa_ctbl134nit),
	DIM_LUT_V0_INIT(143, 254, GAMMA_2_15, dream2_a3_sa_rtbl143nit, dream2_a3_sa_ctbl143nit),
	DIM_LUT_V0_INIT(152, 265, GAMMA_2_15, dream2_a3_sa_rtbl152nit, dream2_a3_sa_ctbl152nit),
	DIM_LUT_V0_INIT(162, 280, GAMMA_2_15, dream2_a3_sa_rtbl162nit, dream2_a3_sa_ctbl162nit),
	DIM_LUT_V0_INIT(172, 294, GAMMA_2_15, dream2_a3_sa_rtbl172nit, dream2_a3_sa_ctbl172nit),
	DIM_LUT_V0_INIT(183, 300, GAMMA_2_15, dream2_a3_sa_rtbl183nit, dream2_a3_sa_ctbl183nit),
	DIM_LUT_V0_INIT(195, 300, GAMMA_2_15, dream2_a3_sa_rtbl195nit, dream2_a3_sa_ctbl195nit),
	DIM_LUT_V0_INIT(207, 300, GAMMA_2_15, dream2_a3_sa_rtbl207nit, dream2_a3_sa_ctbl207nit),
	DIM_LUT_V0_INIT(220, 300, GAMMA_2_15, dream2_a3_sa_rtbl220nit, dream2_a3_sa_ctbl220nit),
	DIM_LUT_V0_INIT(234, 300, GAMMA_2_15, dream2_a3_sa_rtbl234nit, dream2_a3_sa_ctbl234nit),
	DIM_LUT_V0_INIT(249, 304, GAMMA_2_15, dream2_a3_sa_rtbl249nit, dream2_a3_sa_ctbl249nit),
	DIM_LUT_V0_INIT(265, 319, GAMMA_2_15, dream2_a3_sa_rtbl265nit, dream2_a3_sa_ctbl265nit),
	DIM_LUT_V0_INIT(282, 335, GAMMA_2_15, dream2_a3_sa_rtbl282nit, dream2_a3_sa_ctbl282nit),
	DIM_LUT_V0_INIT(300, 348, GAMMA_2_15, dream2_a3_sa_rtbl300nit, dream2_a3_sa_ctbl300nit),
	DIM_LUT_V0_INIT(316, 364, GAMMA_2_15, dream2_a3_sa_rtbl316nit, dream2_a3_sa_ctbl316nit),
	DIM_LUT_V0_INIT(333, 381, GAMMA_2_15, dream2_a3_sa_rtbl333nit, dream2_a3_sa_ctbl333nit),
	DIM_LUT_V0_INIT(350, 392, GAMMA_2_15, dream2_a3_sa_rtbl350nit, dream2_a3_sa_ctbl350nit),
	DIM_LUT_V0_INIT(357, 402, GAMMA_2_15, dream2_a3_sa_rtbl357nit, dream2_a3_sa_ctbl357nit),
	DIM_LUT_V0_INIT(365, 406, GAMMA_2_15, dream2_a3_sa_rtbl365nit, dream2_a3_sa_ctbl365nit),
	DIM_LUT_V0_INIT(372, 406, GAMMA_2_15, dream2_a3_sa_rtbl372nit, dream2_a3_sa_ctbl372nit),
	DIM_LUT_V0_INIT(380, 406, GAMMA_2_15, dream2_a3_sa_rtbl380nit, dream2_a3_sa_ctbl380nit),
	DIM_LUT_V0_INIT(387, 406, GAMMA_2_15, dream2_a3_sa_rtbl387nit, dream2_a3_sa_ctbl387nit),
	DIM_LUT_V0_INIT(395, 406, GAMMA_2_15, dream2_a3_sa_rtbl395nit, dream2_a3_sa_ctbl395nit),
	DIM_LUT_V0_INIT(403, 409, GAMMA_2_15, dream2_a3_sa_rtbl403nit, dream2_a3_sa_ctbl403nit),
	DIM_LUT_V0_INIT(412, 416, GAMMA_2_15, dream2_a3_sa_rtbl412nit, dream2_a3_sa_ctbl412nit),
	DIM_LUT_V0_INIT(420, 420, GAMMA_2_20, dream2_a3_sa_rtbl420nit, dream2_a3_sa_ctbl420nit),
};

#if PANEL_BACKLIGHT_PAC_STEPS == 512
static unsigned int dream2_a3_sa_brt_to_step_tbl[S6E3HA6_TOTAL_PAC_STEPS] = {
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
static unsigned int dream2_a3_sa_brt_to_step_tbl[S6E3HA6_TOTAL_PAC_STEPS] = {
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
	BRT(281), BRT(295), BRT(309), BRT(323), BRT(336), BRT(350), BRT(364), BRT(365),
#ifdef CONFIG_LCD_EXTEND_HBM
	BRT(366),
#endif
};
#endif	/* PANEL_BACKLIGHT_PAC_STEPS  */

static unsigned int dream2_a3_sa_brt_to_brt_tbl[S6E3HA6_TOTAL_PAC_STEPS][2] = {
};

static unsigned int dream2_a3_sa_brt_tbl[S6E3HA6_TOTAL_NR_LUMINANCE] = {
	BRT(0), BRT(7), BRT(14), BRT(21), BRT(28), BRT(35), BRT(36), BRT(38), BRT(40), BRT(42),
	BRT(44), BRT(46), BRT(48), BRT(50), BRT(52), BRT(54), BRT(56), BRT(57), BRT(59), BRT(61),
	BRT(63), BRT(65), BRT(67), BRT(69), BRT(70), BRT(72), BRT(74), BRT(76), BRT(78), BRT(80),
	BRT(82), BRT(84), BRT(86), BRT(88), BRT(90), BRT(92), BRT(94), BRT(96), BRT(98), BRT(100),
	BRT(102), BRT(104), BRT(106), BRT(108), BRT(110), BRT(112), BRT(114), BRT(116), BRT(118), BRT(120),
	BRT(122), BRT(124), BRT(126), BRT(128), BRT(135), BRT(142), BRT(149), BRT(157), BRT(165), BRT(174),
	BRT(183), BRT(193), BRT(201), BRT(210), BRT(219), BRT(223), BRT(227), BRT(230), BRT(234), BRT(238),
	BRT(242), BRT(246), BRT(250), BRT(255),
	/* HBM */
	BRT(281), BRT(295), BRT(309), BRT(323), BRT(336), BRT(350), BRT(364), BRT(365),
#ifdef CONFIG_LCD_EXTEND_HBM
	BRT(366),
#endif
};

static unsigned int dream2_a3_sa_lum_tbl[S6E3HA6_TOTAL_NR_LUMINANCE] = {
	2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
	12, 13, 14, 15, 16, 17, 19, 20, 21, 22,
	24, 25, 27, 29, 30, 32, 34, 37, 39, 41,
	44, 47, 50, 53, 56, 60, 64, 68, 72, 77,
	82, 87, 93, 98, 105, 111, 119, 126, 134, 143,
	152, 162, 172, 183, 195, 207, 220, 234, 249, 265,
	282, 300, 316, 333, 350, 357, 365, 372, 380, 387,
	395, 403, 412, 420,
	/* HBM */
	443, 465, 488, 510, 533, 555, 578, 600,
#ifdef CONFIG_LCD_EXTEND_HBM
	/* EXTEND_HBM */
	720,
#endif
};

struct brightness_table s6e3ha6_dream2_a3_sa_panel_brightness_table = {
	.brt = dream2_a3_sa_brt_tbl,
	.sz_brt = ARRAY_SIZE(dream2_a3_sa_brt_tbl),
	.lum = dream2_a3_sa_lum_tbl,
	.sz_lum = ARRAY_SIZE(dream2_a3_sa_lum_tbl),
	.sz_ui_lum = S6E3HA6_NR_LUMINANCE,
	.sz_hbm_lum = S6E3HA6_NR_HBM_LUMINANCE,
	.sz_ext_hbm_lum = S6E3HA6_NR_EXTEND_HBM_LUMINANCE,
#if PANEL_BACKLIGHT_PAC_STEPS == 512 || PANEL_BACKLIGHT_PAC_STEPS == 256
	.brt_to_step = dream2_a3_sa_brt_to_step_tbl,
	.sz_brt_to_step = ARRAY_SIZE(dream2_a3_sa_brt_to_step_tbl),
#else
	.brt_to_step = dream2_a3_sa_brt_tbl,
	.sz_brt_to_step = ARRAY_SIZE(dream2_a3_sa_brt_tbl),
#endif
	.brt_to_brt = dream2_a3_sa_brt_to_brt_tbl,
	.sz_brt_to_brt = ARRAY_SIZE(dream2_a3_sa_brt_to_brt_tbl),
};

static struct panel_dimming_info s6e3ha6_dream2_a3_sa_panel_dimming_info = {
	.name = "s6e3ha6_dream2_a3_sa",
	.dim_init_info = {
		.name = "s6e3ha6_dream2_a3_sa",
		.nr_tp = S6E3HA6_NR_TP,
		.tp = s6e3ha6_tp,
		.nr_luminance = S6E3HA6_NR_LUMINANCE,
		.vregout = 114085069LL,	/* 6.8 * 2^24 */
		.bitshift = 24,
		.vt_voltage = {
			0, 12, 24, 36, 48, 60, 72, 84, 96, 108,
			138, 148, 158, 168, 178, 186,
		},
		.target_luminance = S6E3HA6_TARGET_LUMINANCE,
		.target_gamma = 220,
		.dim_lut = dream2_a3_sa_dimming_lut,
	},
	.target_luminance = S6E3HA6_TARGET_LUMINANCE,
	.nr_luminance = S6E3HA6_NR_LUMINANCE,
	.hbm_target_luminance = S6E3HA6_TARGET_HBM_LUMINANCE,
	.nr_hbm_luminance = S6E3HA6_NR_HBM_LUMINANCE,
	.extend_hbm_target_luminance = S6E3HA6_TARGET_EXTEND_HBM_LUMINANCE,
	.nr_extend_hbm_luminance = S6E3HA6_NR_EXTEND_HBM_LUMINANCE,
	.brt_tbl = &s6e3ha6_dream2_a3_sa_panel_brightness_table,
};
#endif /* __S6E3HA6_DREAM2_A3_SA_PANEL_DIMMING_H___ */
