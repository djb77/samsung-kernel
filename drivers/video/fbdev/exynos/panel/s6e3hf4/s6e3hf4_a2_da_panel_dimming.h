/*
 * linux/drivers/video/fbdev/exynos/panel/s6e3hf4/s6e3hf4_a2_da_panel_dimming.h
 *
 * Header file for S6E3HF4 Dimming Driver
 *
 * Copyright (c) 2016 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __S6E3HF4_A2_DA_PANEL_DIMMING_H___
#define __S6E3HF4_A2_DA_PANEL_DIMMING_H___
#include "../dimming.h"
#include "../panel_dimming.h"
#include "s6e3hf4_dimming.h"

/*
 * PANEL INFORMATION
 * LDI : S6E3HF4
 * PANEL : A2 DA
 */
/* gray scale offset values */
static s32 a2_da_rtbl2nit[] = {0, 0, 28, 28, 28, 25, 21, 17, 11, 7, 0};
static s32 a2_da_rtbl3nit[] = {0, 0, 25, 25, 25, 22, 18, 16, 10, 6, 0};
static s32 a2_da_rtbl4nit[] = {0, 0, 21, 21, 21, 19, 16, 14, 9, 6, 0};
static s32 a2_da_rtbl5nit[] = {0, 0, 21, 21, 21, 18, 15, 13, 7, 5, 0};
static s32 a2_da_rtbl6nit[] = {0, 0, 22, 22, 21, 18, 15, 13, 8, 6, 0};
static s32 a2_da_rtbl7nit[] = {0, 0, 21, 21, 19, 16, 13, 12, 7, 5, 0};
static s32 a2_da_rtbl8nit[] = {0, 0, 20, 20, 18, 15, 13, 11, 6, 5, 0};
static s32 a2_da_rtbl9nit[] = {0, 0, 21, 20, 17, 14, 12, 10, 5, 4, 0};
static s32 a2_da_rtbl10nit[] = {0, 0, 18, 17, 16, 13, 11, 9, 4, 4, 0};
static s32 a2_da_rtbl11nit[] = {0, 0, 18, 17, 15, 12, 10, 8, 4, 4, 0};
static s32 a2_da_rtbl12nit[] = {0, 0, 17, 16, 15, 12, 10, 8, 5, 5, 0};
static s32 a2_da_rtbl13nit[] = {0, 0, 18, 17, 15, 12, 9, 7, 3, 3, 0};
static s32 a2_da_rtbl14nit[] = {0, 0, 19, 17, 14, 11, 8, 6, 2, 2, 0};
static s32 a2_da_rtbl15nit[] = {0, 0, 18, 15, 12, 9, 7, 5, 2, 2, 0};
static s32 a2_da_rtbl16nit[] = {0, 0, 17, 15, 11, 9, 7, 5, 3, 3, 0};
static s32 a2_da_rtbl17nit[] = {0, 0, 17, 14, 11, 9, 7, 5, 2, 3, 0};
static s32 a2_da_rtbl19nit[] = {0, 0, 14, 13, 11, 8, 6, 4, 2, 3, 0};
static s32 a2_da_rtbl20nit[] = {0, 0, 13, 12, 9, 7, 5, 4, 2, 3, 0};
static s32 a2_da_rtbl21nit[] = {0, 0, 13, 12, 9, 7, 5, 4, 2, 3, 0};
static s32 a2_da_rtbl22nit[] = {0, 0, 13, 12, 9, 7, 5, 4, 2, 3, 0};
static s32 a2_da_rtbl24nit[] = {0, 0, 13, 12, 9, 7, 5, 4, 2, 3, 0};
static s32 a2_da_rtbl25nit[] = {0, 0, 13, 11, 8, 6, 5, 4, 1, 3, 0};
static s32 a2_da_rtbl27nit[] = {0, 0, 13, 11, 8, 6, 5, 4, 2, 4, 0};
static s32 a2_da_rtbl29nit[] = {0, 0, 11, 10, 7, 5, 4, 3, 1, 3, 0};
static s32 a2_da_rtbl30nit[] = {0, 0, 11, 9, 6, 4, 3, 3, 2, 3, 0};
static s32 a2_da_rtbl32nit[] = {0, 0, 11, 9, 6, 4, 3, 3, 1, 3, 0};
static s32 a2_da_rtbl34nit[] = {0, 0, 11, 9, 6, 4, 3, 3, 1, 3, 0};
static s32 a2_da_rtbl37nit[] = {0, 0, 11, 9, 6, 4, 3, 2, 1, 3, 0};
static s32 a2_da_rtbl39nit[] = {0, 0, 11, 9, 6, 4, 3, 2, 1, 3, 0};
static s32 a2_da_rtbl41nit[] = {0, 0, 10, 8, 4, 3, 2, 2, 1, 3, 0};
static s32 a2_da_rtbl44nit[] = {0, 0, 9, 7, 4, 2, 2, 1, 0, 2, 0};
static s32 a2_da_rtbl47nit[] = {0, 0, 8, 7, 4, 2, 2, 1, 0, 3, 0};
static s32 a2_da_rtbl50nit[] = {0, 0, 8, 7, 4, 2, 1, 1, 0, 0, 0};
static s32 a2_da_rtbl53nit[] = {0, 0, 8, 6, 3, 2, 1, 0, 0, 1, 0};
static s32 a2_da_rtbl56nit[] = {0, 0, 7, 5, 3, 2, 1, 0, 0, 2, 0};
static s32 a2_da_rtbl60nit[] = {0, 0, 7, 4, 2, 1, 1, 1, 0, 1, 0};
static s32 a2_da_rtbl64nit[] = {0, 0, 6, 4, 2, 0, 0, 0, 0, 1, 0};
static s32 a2_da_rtbl68nit[] = {0, 0, 6, 5, 3, 2, 2, 1, 0, 0, 0};
static s32 a2_da_rtbl72nit[] = {0, 0, 7, 5, 3, 1, 1, 1, 0, 1, 0};
static s32 a2_da_rtbl77nit[] = {0, 0, 5, 4, 2, 1, 0, 0, 0, 1, 0};
static s32 a2_da_rtbl82nit[] = {0, 0, 5, 4, 2, 1, 0, 1, 0, 1, 0};
static s32 a2_da_rtbl87nit[] = {0, 0, 4, 4, 2, 1, 0, 1, 1, 1, 0};
static s32 a2_da_rtbl93nit[] = {0, 0, 5, 4, 2, 1, 0, 1, 1, 0, 0};
static s32 a2_da_rtbl98nit[] = {0, 0, 5, 4, 2, 1, 1, 0, 1, 0, 0};
static s32 a2_da_rtbl105nit[] = {0, 0, 5, 4, 2, 1, 0, 0, 2, 0, 0};
static s32 a2_da_rtbl111nit[] = {0, 0, 5, 4, 2, 1, 1, 1, 1, 1, 0};
static s32 a2_da_rtbl119nit[] = {0, 0, 4, 3, 2, 1, 1, 2, 2, 1, 0};
static s32 a2_da_rtbl126nit[] = {0, 0, 4, 3, 1, 1, 1, 2, 1, 1, 0};
static s32 a2_da_rtbl134nit[] = {0, 0, 4, 3, 2, 1, 1, 1, 1, 1, 0};
static s32 a2_da_rtbl143nit[] = {0, 0, 4, 3, 1, 0, 1, 1, 1, 1, 0};
static s32 a2_da_rtbl152nit[] = {0, 0, 4, 3, 2, 1, 1, 1, 2, 1, 0};
static s32 a2_da_rtbl162nit[] = {0, 0, 3, 2, 1, 0, 0, 2, 3, 0, 0};
static s32 a2_da_rtbl172nit[] = {0, 0, 3, 2, 1, 1, 1, 2, 3, 0, 0};
static s32 a2_da_rtbl183nit[] = {0, 0, 3, 2, 0, 0, 0, 0, 1, 1, 0};
static s32 a2_da_rtbl195nit[] = {0, 0, 3, 1, 0, 0, 0, 0, 1, 0, 0};
static s32 a2_da_rtbl207nit[] = {0, 0, 2, 1, 0, 0, 0, 0, 2, 1, 0};
static s32 a2_da_rtbl220nit[] = {0, 0, 2, 1, 0, 0, 0, 0, 2, 0, 0};
static s32 a2_da_rtbl234nit[] = {0, 0, 2, 1, 0, 0, 0, 0, 2, 0, 0};
static s32 a2_da_rtbl249nit[] = {0, 0, 1, 0, 0, 0, 0, 0, 2, 0, 0};
static s32 a2_da_rtbl265nit[] = {0, 0, 1, 1, 1, 0, 0, 0, 2, 1, 0};
static s32 a2_da_rtbl282nit[] = {0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0};
static s32 a2_da_rtbl300nit[] = {0, 0, 1, 0, 1, 0, 0, 0, 1, 1, 0};
static s32 a2_da_rtbl316nit[] = {0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0};
static s32 a2_da_rtbl333nit[] = {0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0};
static s32 a2_da_rtbl350nit[] = {0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0};
static s32 a2_da_rtbl357nit[] = {0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0};
static s32 a2_da_rtbl365nit[] = {0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0};
static s32 a2_da_rtbl372nit[] = {0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0};
static s32 a2_da_rtbl380nit[] = {0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0};
static s32 a2_da_rtbl387nit[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static s32 a2_da_rtbl395nit[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static s32 a2_da_rtbl403nit[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static s32 a2_da_rtbl412nit[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static s32 a2_da_rtbl420nit[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

/* rgb color offset values */
static s32 a2_da_ctbl2nit[] = {0, 0, 0, 0, 0, 0, -23, -6, -28, -34, -27, -35, -6, 1, -10, -10, 1, -9, -18, 0, -10, -9, 1, -4, -4, 0, -4, -3, 0, -2, -9, 0, -8};
static s32 a2_da_ctbl3nit[] = {0, 0, 0, 0, 0, 0, -22, -7, -28, -34, -27, -34, -2, 0, -11, -15, 0, -11, -12, 1, -8, -9, -1, -7, -3, 0, -3, -3, 0, -2, -6, 0, -7};
static s32 a2_da_ctbl4nit[] = {0, 0, 0, 0, 0, 0, -21, -6, -28, -25, -17, -24, -4, 1, -9, -11, 1, -9, -11, 0, -9, -7, 0, -6, -3, 0, -3, -2, 0, -1, -5, 0, -6};
static s32 a2_da_ctbl5nit[] = {0, 0, 0, 0, 0, 0, -24, -10, -31, -9, -2, -8, -7, 0, -12, -12, 0, -9, -11, 0, -9, -6, 0, -6, -2, 0, -2, -2, 0, -1, -4, 0, -5};
static s32 a2_da_ctbl6nit[] = {0, 0, 0, 0, 0, 0, -20, -8, -27, -2, 3, -2, -8, 0, -12, -10, 0, -10, -11, 0, -9, -6, 0, -6, -2, 0, -2, -2, 0, -1, -3, 0, -4};
static s32 a2_da_ctbl7nit[] = {0, 0, 0, 0, 0, 0, -16, -4, -23, -1, 3, -2, -10, 0, -13, -11, -1, -12, -8, 1, -9, -6, 0, -5, -1, 0, -2, -2, 0, -1, -2, 0, -3};
static s32 a2_da_ctbl8nit[] = {0, 0, 0, 0, 0, 0, -14, 1, -20, -7, 0, -7, -9, 1, -12, -7, 1, -9, -9, 0, -10, -6, -1, -5, -1, 0, -2, -1, 0, 0, -2, 0, -3};
static s32 a2_da_ctbl9nit[] = {0, 0, 0, 0, 0, 0, -13, 0, -19, -6, 0, -7, -8, 1, -11, -9, 0, -11, -9, -1, -10, -5, 0, -4, -1, 0, -2, -1, 0, -1, -1, 0, -2};
static s32 a2_da_ctbl10nit[] = {0, 0, 0, 0, 0, 0, -15, -1, -22, -6, 2, -6, -6, 1, -11, -8, 0, -11, -9, 1, -10, -4, 0, -4, 0, 0, -1, -1, 0, -1, -1, 0, -2};
static s32 a2_da_ctbl11nit[] = {0, 0, 0, 0, 0, 0, -15, -1, -22, -9, -2, -9, -7, 1, -12, -6, 1, -9, -8, 0, -9, -4, 0, -4, 0, 0, -1, -1, 0, 0, 0, 0, -1};
static s32 a2_da_ctbl12nit[] = {0, 0, 0, 0, 0, 0, -14, 0, -20, -6, 2, -6, -7, 0, -13, -7, 1, -8, -8, -1, -10, -3, 0, -3, 0, 0, -1, -1, 0, 0, 0, 0, -1};
static s32 a2_da_ctbl13nit[] = {0, 0, 0, 0, 0, 0, -11, 2, -18, -5, 1, -6, -7, -1, -14, -10, -1, -12, -8, -1, -10, -3, 0, -3, 0, 0, -1, -1, 0, -1, 1, 0, 0};
static s32 a2_da_ctbl14nit[] = {0, 0, 0, 0, 0, 0, -13, 0, -18, -6, 0, -8, -6, -1, -13, -10, -1, -10, -8, -1, -10, -3, 0, -3, 0, 0, -1, 0, 0, 0, 1, 0, 0};
static s32 a2_da_ctbl15nit[] = {0, 0, 0, 0, 0, 0, -12, 0, -17, -6, -1, -10, -6, 0, -14, -7, 2, -6, -7, 0, -10, -1, 1, -1, 0, 0, -1, 0, 0, 0, 1, 0, 0};
static s32 a2_da_ctbl16nit[] = {0, 0, 0, 0, 0, 0, -9, 1, -15, -7, -2, -11, -5, 2, -13, -7, 2, -7, -7, 0, -9, -2, 1, -2, 0, 0, 0, 0, 0, -1, 1, 0, 0};
static s32 a2_da_ctbl17nit[] = {0, 0, 0, 0, 0, 0, -10, 0, -15, -3, 1, -9, -5, 2, -12, -7, 1, -7, -8, -1, -10, -1, 0, -2, 0, 0, 0, -1, 0, -1, 1, 0, 0};
static s32 a2_da_ctbl19nit[] = {0, 0, 0, 0, 0, 0, -13, 1, -20, -5, 1, -6, -9, -2, -16, -7, 0, -6, -7, -1, -9, -1, 1, -2, 0, 0, 1, 0, 1, 0, 2, 0, 1};
static s32 a2_da_ctbl20nit[] = {0, 0, 0, 0, 0, 0, -1, 1, -8, -4, 2, -5, -6, 1, -13, -6, 1, -5, -6, 1, -8, 0, 1, -1, 0, 0, 0, 0, 1, 0, 2, 0, 1};
static s32 a2_da_ctbl21nit[] = {0, 0, 0, 0, 0, 0, 3, 4, -5, -1, 5, -3, -8, -2, -16, -7, 1, -5, -6, 1, -8, -1, 1, -1, 0, 0, -1, 1, 1, 1, 2, 0, 1};
static s32 a2_da_ctbl22nit[] = {0, 0, 0, 0, 0, 0, 3, 4, -5, -1, 5, -3, -8, -2, -16, -7, 1, -5, -6, 1, -8, -1, 1, -1, 0, 0, -1, 1, 1, 1, 2, 0, 1};
static s32 a2_da_ctbl24nit[] = {0, 0, 0, 0, 0, 0, -12, 1, -16, -7, -1, -8, -9, -2, -14, -7, -1, -6, -6, 0, -8, -1, 0, -2, 0, 0, 1, 0, 0, -1, 2, 0, 1};
static s32 a2_da_ctbl25nit[] = {0, 0, 0, 0, 0, 0, -13, -1, -17, -3, 3, -5, -6, 0, -12, -5, 2, -2, -7, -1, -9, -1, 0, -2, 1, 0, 1, 0, 1, 0, 2, 0, 1};
static s32 a2_da_ctbl27nit[] = {0, 0, 0, 0, 0, 0, -9, 2, -14, -5, 0, -8, -9, -2, -15, -5, 1, -2, -6, -1, -8, -1, 0, -2, 0, 0, 0, 0, 0, 0, 2, 0, 1};
static s32 a2_da_ctbl29nit[] = {0, 0, 0, 0, 0, 0, -8, 1, -15, -6, 0, -7, -7, 0, -11, -5, 1, -4, -4, -1, -6, 0, 0, 0, 0, 0, 0, 0, 0, -1, 2, 0, 1};
static s32 a2_da_ctbl30nit[] = {0, 0, 0, 0, 0, 0, -9, -1, -15, -2, 4, -5, -5, 2, -10, -5, 1, -3, -3, 1, -4, 0, 0, -1, 0, 0, 0, 0, 0, 0, 2, 0, 1};
static s32 a2_da_ctbl32nit[] = {0, 0, 0, 0, 0, 0, -5, 2, -12, -4, 1, -8, -7, -1, -12, -6, 1, -3, -2, 1, -4, 0, 0, -1, 1, 0, 1, 0, 0, 0, 2, 0, 1};
static s32 a2_da_ctbl34nit[] = {0, 0, 0, 0, 0, 0, -3, 5, -9, -8, 1, -22, -5, 2, -9, -6, 0, -3, -3, 0, -5, 0, -1, 0, 0, 0, -1, 0, 0, 1, 2, 0, 1};
static s32 a2_da_ctbl37nit[] = {0, 0, 0, 0, 0, 0, -6, 2, -11, -10, 0, -25, -9, -2, -12, -6, 0, -4, -4, -1, -6, 0, 1, 0, 1, 0, 1, 0, 0, 0, 2, 0, 1};
static s32 a2_da_ctbl39nit[] = {0, 0, 0, 0, 0, 0, -4, 5, -8, -10, 1, -22, -8, -2, -11, -7, -1, -3, -4, -1, -6, 0, 0, 0, 1, 0, -1, 0, 0, 1, 2, 0, 1};
static s32 a2_da_ctbl41nit[] = {0, 0, 0, 0, 0, 0, -6, 2, -10, -15, -2, -24, -2, 3, -7, -5, -1, -2, -2, 1, -4, 0, 0, 0, 1, 0, 1, 0, 0, 0, 2, 0, 1};
static s32 a2_da_ctbl44nit[] = {0, 0, 0, 0, 0, 0, -3, 4, -7, -13, -2, -24, -4, 2, -7, -3, 2, 1, -2, -1, -5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 1};
static s32 a2_da_ctbl47nit[] = {0, 0, 0, 0, 0, 0, -7, 2, -13, -14, -1, -25, -5, -2, -7, -3, 1, -1, -3, -1, -4, 1, 0, 0, 1, 1, 0, -1, 0, 0, 2, 0, 1};
static s32 a2_da_ctbl50nit[] = {0, 0, 0, 0, 0, 0, -8, 2, -15, -15, -2, -26, -5, -2, -7, -6, -2, -4, 0, 2, -2, 1, 0, 1, 0, 0, -1, 0, 0, 0, 2, 0, 1};
static s32 a2_da_ctbl53nit[] = {0, 0, 0, 0, 0, 0, -10, -1, -16, -13, -1, -22, -2, 2, -5, -6, -2, -3, -3, -1, -5, 1, 1, 1, 1, 1, 1, 1, 0, 1, 2, 0, 1};
static s32 a2_da_ctbl56nit[] = {0, 0, 0, 0, 0, 0, -6, 2, -12, -10, 3, -20, 1, 4, -2, -7, -2, -3, -2, -1, -4, 0, 0, 0, 2, 1, 2, 0, 0, -1, 2, 0, 1};
static s32 a2_da_ctbl60nit[] = {0, 0, 0, 0, 0, 0, -9, 0, -14, -10, 1, -22, 0, 2, -4, -1, 2, 2, -1, 0, -3, 0, -1, 0, -1, 0, -1, 1, 0, 0, 2, 0, 2};
static s32 a2_da_ctbl64nit[] = {0, 0, 0, 0, 0, 0, -5, 2, -9, -13, -2, -24, -3, 0, -6, 0, 3, 2, 1, 1, -1, 1, 1, 1, 1, 0, 1, 0, 0, -1, 2, 0, 2};
static s32 a2_da_ctbl68nit[] = {0, 0, 0, 0, 0, 0, -1, 5, -6, -13, 0, -22, -1, 1, -4, -6, -2, -2, -2, -1, -4, 1, 0, 0, -1, -2, 0, 2, 0, 1, 1, 0, 1};
static s32 a2_da_ctbl72nit[] = {0, 0, 0, 0, 0, 0, -5, 2, -9, -16, -2, -25, -2, -1, -6, -4, -1, 0, -1, 0, -3, 1, 0, 1, 0, -1, -2, 1, 1, 1, 2, 0, 2};
static s32 a2_da_ctbl77nit[] = {0, 0, 0, 0, 0, 0, 3, 2, 1, -12, 2, -19, 0, 2, -4, -4, -2, -1, 0, 1, -1, 2, 0, 1, 0, 0, 0, 1, 1, 2, 2, 0, 1};
static s32 a2_da_ctbl82nit[] = {0, 0, 0, 0, 0, 0, 0, -1, -3, -9, 2, -16, 0, 4, -1, -4, -2, -1, 1, 1, -2, 0, 0, 1, 0, -1, 0, 2, 1, 0, 2, 0, 2};
static s32 a2_da_ctbl87nit[] = {0, 0, 0, 0, 0, 0, 6, 2, 1, -6, 3, -13, -2, 1, -3, -2, 1, 2, 1, 1, -2, 0, 0, 1, 1, 0, 0, 1, 0, 0, 2, 0, 2};
static s32 a2_da_ctbl93nit[] = {0, 0, 0, 0, 0, 0, 4, -1, -5, -7, 0, -15, -4, -1, -5, -1, 1, 3, -1, 1, -3, 1, 0, 1, 0, 0, 0, 1, 0, 1, 1, 0, 1};
static s32 a2_da_ctbl98nit[] = {0, 0, 0, 0, 0, 0, 7, 3, -2, -11, -1, -18, 1, 2, -2, -3, -1, 0, -1, 0, -3, 2, 0, 1, 1, 0, 0, 0, 0, 1, 2, 0, 1};
static s32 a2_da_ctbl105nit[] = {0, 0, 0, 0, 0, 0, 4, 1, -5, -9, 2, -15, -3, -1, -4, 0, 1, 2, -1, 0, -2, 1, 1, 1, 1, 0, 0, 1, 0, 2, 1, 0, 2};
static s32 a2_da_ctbl111nit[] = {0, 0, 0, 0, 0, 0, 7, 5, -2, -11, -1, -18, -2, 1, -2, -2, 0, 0, 0, 0, -2, 1, 1, 1, 1, 0, 1, 1, 1, 1, 2, 0, 1};
static s32 a2_da_ctbl119nit[] = {0, 0, 0, 0, 0, 0, 11, 9, 2, -6, 4, -12, -2, -1, -4, -2, -1, 0, 1, 2, 0, 1, 0, 0, 0, 0, 1, 2, 0, 1, 1, 0, 1};
static s32 a2_da_ctbl126nit[] = {0, 0, 0, 0, 0, 0, 8, 4, 0, -9, 1, -14, 1, 1, -1, 0, 2, 2, 0, 1, 0, 1, 0, -1, 2, 1, 2, 1, 0, 2, 1, 0, 1};
static s32 a2_da_ctbl134nit[] = {0, 0, 0, 0, 0, 0, 4, 1, -4, -7, 4, -11, -2, -1, -3, -2, -1, 1, -1, 0, -2, 1, 0, 2, 1, 1, 0, 1, 1, 2, 1, 0, 0};
static s32 a2_da_ctbl143nit[] = {0, 0, 0, 0, 0, 0, -5, -2, -11, -6, 2, -10, -1, -1, -3, 0, 2, 3, 0, 1, -1, 0, -1, 0, 3, 2, 2, 1, 1, 1, 1, 0, 1};
static s32 a2_da_ctbl152nit[] = {0, 0, 0, 0, 0, 0, -1, 1, -8, -6, 3, -8, -1, 0, -2, 0, 1, 2, -1, 0, -1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 0, 1};
static s32 a2_da_ctbl162nit[] = {0, 0, 0, 0, 0, 0, 3, 4, -4, -6, 1, -9, -2, -1, -2, -1, 0, 1, 1, 2, 0, 1, 0, 1, 1, 1, 0, 1, 0, 2, 0, 0, 0};
static s32 a2_da_ctbl172nit[] = {0, 0, 0, 0, 0, 0, 6, 7, 0, -9, -1, -12, -1, 0, -2, 0, 2, 2, 0, 0, 0, 0, 0, 1, 0, 0, -1, 1, 0, 1, 0, 0, 0};
static s32 a2_da_ctbl183nit[] = {0, 0, 0, 0, 0, 0, 2, 4, -3, -5, 2, -7, -2, -1, -2, 0, 2, 2, 0, 1, 0, -1, -2, -1, 5, 5, 5, 0, 0, 0, 1, 0, 1};
static s32 a2_da_ctbl195nit[] = {0, 0, 0, 0, 0, 0, -2, 1, -6, -4, 4, -4, 1, 2, 0, 1, 2, 2, -1, 0, 0, 0, 0, 1, 4, 3, 3, 0, 0, 0, 1, 0, 1};
static s32 a2_da_ctbl207nit[] = {0, 0, 0, 0, 0, 0, 3, 3, -1, -1, 6, -1, -1, -1, -2, 0, 2, 2, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1};
static s32 a2_da_ctbl220nit[] = {0, 0, 0, 0, 0, 0, 0, 1, -4, -4, 2, -4, 1, 2, 1, 0, 1, 1, -1, 0, -1, 1, 0, 1, 1, 0, 1, 0, 0, 0, 1, 0, 1};
static s32 a2_da_ctbl234nit[] = {0, 0, 0, 0, 0, 0, 3, 4, 0, -7, 0, -7, -1, -1, -2, 1, 1, 2, -1, -1, 0, 1, 0, -1, 1, 0, 2, 1, 0, 0, 0, 0, 0};
static s32 a2_da_ctbl249nit[] = {0, 0, 0, 0, 0, 0, 0, 0, -1, -2, 3, 0, 2, 2, 0, 1, 1, 2, 1, 1, 0, 1, 1, 2, 0, 0, 0, 1, 0, 1, 0, 0, 0};
static s32 a2_da_ctbl265nit[] = {0, 0, 0, 0, 0, 0, 5, 2, 2, -2, 4, 0, -1, -1, -2, 0, 1, 1, 1, 1, 2, 0, -2, -1, 1, 1, 1, 0, 0, 1, 0, 0, 0};
static s32 a2_da_ctbl282nit[] = {0, 0, 0, 0, 0, 0, 1, 1, -1, -2, 2, -1, 0, 1, 0, 0, 1, 2, -1, -2, -1, 0, 0, -1, 1, 1, 2, 2, 0, 1, 0, 0, 0};
static s32 a2_da_ctbl300nit[] = {0, 0, 0, 0, 0, 0, 3, 4, 2, -1, 3, 1, -2, -2, -2, -1, 0, 0, -1, -2, -1, 1, 0, 0, 1, 0, 2, 0, 0, 0, 0, 0, 0};
static s32 a2_da_ctbl316nit[] = {0, 0, 0, 0, 0, 0, 1, 4, 1, -3, -2, -2, -3, -2, -3, 0, 2, 2, 1, 0, 1, -1, -2, -2, 2, 1, 2, -1, 0, -1, 0, 0, 0};
static s32 a2_da_ctbl333nit[] = {0, 0, 0, 0, 0, 0, -2, 1, -1, 0, 1, 1, 0, 1, 1, -3, -2, -1, 1, 0, 1, 0, -1, -1, -1, -1, -1, 1, 1, 1, 0, 0, 0};
static s32 a2_da_ctbl350nit[] = {0, 0, 0, 0, 0, 0, 0, 4, 2, -3, -1, -2, -3, -3, -2, 0, 1, 0, -1, -1, 0, 0, -1, -1, 2, 1, 1, 0, 0, 1, 0, 0, 0};
static s32 a2_da_ctbl357nit[] = {0, 0, 0, 0, 0, 0, -2, 1, 0, -1, 2, 1, 0, 0, 0, -3, -2, -2, 2, 2, 2, 0, -1, 0, 1, 0, 1, -1, 0, -1, 0, 0, 0};
static s32 a2_da_ctbl365nit[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static s32 a2_da_ctbl372nit[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static s32 a2_da_ctbl380nit[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static s32 a2_da_ctbl387nit[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static s32 a2_da_ctbl395nit[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static s32 a2_da_ctbl403nit[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static s32 a2_da_ctbl412nit[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static s32 a2_da_ctbl420nit[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static struct dimming_lut a2_da_dimming_lut[] = {
	DIM_LUT_INIT(2, 119, GAMMA_2_15, a2_da_rtbl2nit, a2_da_ctbl2nit),
	DIM_LUT_INIT(3, 119, GAMMA_2_15, a2_da_rtbl3nit, a2_da_ctbl3nit),
	DIM_LUT_INIT(4, 119, GAMMA_2_15, a2_da_rtbl4nit, a2_da_ctbl4nit),
	DIM_LUT_INIT(5, 119, GAMMA_2_15, a2_da_rtbl5nit, a2_da_ctbl5nit),
	DIM_LUT_INIT(6, 119, GAMMA_2_15, a2_da_rtbl6nit, a2_da_ctbl6nit),
	DIM_LUT_INIT(7, 119, GAMMA_2_15, a2_da_rtbl7nit, a2_da_ctbl7nit),
	DIM_LUT_INIT(8, 119, GAMMA_2_15, a2_da_rtbl8nit, a2_da_ctbl8nit),
	DIM_LUT_INIT(9, 119, GAMMA_2_15, a2_da_rtbl9nit, a2_da_ctbl9nit),
	DIM_LUT_INIT(10, 119, GAMMA_2_15, a2_da_rtbl10nit, a2_da_ctbl10nit),
	DIM_LUT_INIT(11, 119, GAMMA_2_15, a2_da_rtbl11nit, a2_da_ctbl11nit),
	DIM_LUT_INIT(12, 119, GAMMA_2_15, a2_da_rtbl12nit, a2_da_ctbl12nit),
	DIM_LUT_INIT(13, 119, GAMMA_2_15, a2_da_rtbl13nit, a2_da_ctbl13nit),
	DIM_LUT_INIT(14, 119, GAMMA_2_15, a2_da_rtbl14nit, a2_da_ctbl14nit),
	DIM_LUT_INIT(15, 119, GAMMA_2_15, a2_da_rtbl15nit, a2_da_ctbl15nit),
	DIM_LUT_INIT(16, 119, GAMMA_2_15, a2_da_rtbl16nit, a2_da_ctbl16nit),
	DIM_LUT_INIT(17, 119, GAMMA_2_15, a2_da_rtbl17nit, a2_da_ctbl17nit),
	DIM_LUT_INIT(19, 119, GAMMA_2_15, a2_da_rtbl19nit, a2_da_ctbl19nit),
	DIM_LUT_INIT(20, 119, GAMMA_2_15, a2_da_rtbl20nit, a2_da_ctbl20nit),
	DIM_LUT_INIT(21, 119, GAMMA_2_15, a2_da_rtbl21nit, a2_da_ctbl21nit),
	DIM_LUT_INIT(22, 119, GAMMA_2_15, a2_da_rtbl22nit, a2_da_ctbl22nit),
	DIM_LUT_INIT(24, 119, GAMMA_2_15, a2_da_rtbl24nit, a2_da_ctbl24nit),
	DIM_LUT_INIT(25, 119, GAMMA_2_15, a2_da_rtbl25nit, a2_da_ctbl25nit),
	DIM_LUT_INIT(27, 119, GAMMA_2_15, a2_da_rtbl27nit, a2_da_ctbl27nit),
	DIM_LUT_INIT(29, 119, GAMMA_2_15, a2_da_rtbl29nit, a2_da_ctbl29nit),
	DIM_LUT_INIT(30, 119, GAMMA_2_15, a2_da_rtbl30nit, a2_da_ctbl30nit),
	DIM_LUT_INIT(32, 119, GAMMA_2_15, a2_da_rtbl32nit, a2_da_ctbl32nit),
	DIM_LUT_INIT(34, 119, GAMMA_2_15, a2_da_rtbl34nit, a2_da_ctbl34nit),
	DIM_LUT_INIT(37, 119, GAMMA_2_15, a2_da_rtbl37nit, a2_da_ctbl37nit),
	DIM_LUT_INIT(39, 119, GAMMA_2_15, a2_da_rtbl39nit, a2_da_ctbl39nit),
	DIM_LUT_INIT(41, 119, GAMMA_2_15, a2_da_rtbl41nit, a2_da_ctbl41nit),
	DIM_LUT_INIT(44, 119, GAMMA_2_15, a2_da_rtbl44nit, a2_da_ctbl44nit),
	DIM_LUT_INIT(47, 119, GAMMA_2_15, a2_da_rtbl47nit, a2_da_ctbl47nit),
	DIM_LUT_INIT(50, 119, GAMMA_2_15, a2_da_rtbl50nit, a2_da_ctbl50nit),
	DIM_LUT_INIT(53, 119, GAMMA_2_15, a2_da_rtbl53nit, a2_da_ctbl53nit),
	DIM_LUT_INIT(56, 119, GAMMA_2_15, a2_da_rtbl56nit, a2_da_ctbl56nit),
	DIM_LUT_INIT(60, 119, GAMMA_2_15, a2_da_rtbl60nit, a2_da_ctbl60nit),
	DIM_LUT_INIT(64, 119, GAMMA_2_15, a2_da_rtbl64nit, a2_da_ctbl64nit),
	DIM_LUT_INIT(68, 125, GAMMA_2_15, a2_da_rtbl68nit, a2_da_ctbl68nit),
	DIM_LUT_INIT(72, 131, GAMMA_2_15, a2_da_rtbl72nit, a2_da_ctbl72nit),
	DIM_LUT_INIT(77, 138, GAMMA_2_15, a2_da_rtbl77nit, a2_da_ctbl77nit),
	DIM_LUT_INIT(82, 147, GAMMA_2_15, a2_da_rtbl82nit, a2_da_ctbl82nit),
	DIM_LUT_INIT(87, 155, GAMMA_2_15, a2_da_rtbl87nit, a2_da_ctbl87nit),
	DIM_LUT_INIT(93, 166, GAMMA_2_15, a2_da_rtbl93nit, a2_da_ctbl93nit),
	DIM_LUT_INIT(98, 174, GAMMA_2_15, a2_da_rtbl98nit, a2_da_ctbl98nit),
	DIM_LUT_INIT(105, 186, GAMMA_2_15, a2_da_rtbl105nit, a2_da_ctbl105nit),
	DIM_LUT_INIT(111, 195, GAMMA_2_15, a2_da_rtbl111nit, a2_da_ctbl111nit),
	DIM_LUT_INIT(119, 207, GAMMA_2_15, a2_da_rtbl119nit, a2_da_ctbl119nit),
	DIM_LUT_INIT(126, 220, GAMMA_2_15, a2_da_rtbl126nit, a2_da_ctbl126nit),
	DIM_LUT_INIT(134, 230, GAMMA_2_15, a2_da_rtbl134nit, a2_da_ctbl134nit),
	DIM_LUT_INIT(143, 243, GAMMA_2_15, a2_da_rtbl143nit, a2_da_ctbl143nit),
	DIM_LUT_INIT(152, 254, GAMMA_2_15, a2_da_rtbl152nit, a2_da_ctbl152nit),
	DIM_LUT_INIT(162, 265, GAMMA_2_15, a2_da_rtbl162nit, a2_da_ctbl162nit),
	DIM_LUT_INIT(172, 280, GAMMA_2_15, a2_da_rtbl172nit, a2_da_ctbl172nit),
	DIM_LUT_INIT(183, 292, GAMMA_2_15, a2_da_rtbl183nit, a2_da_ctbl183nit),
	DIM_LUT_INIT(195, 292, GAMMA_2_15, a2_da_rtbl195nit, a2_da_ctbl195nit),
	DIM_LUT_INIT(207, 292, GAMMA_2_15, a2_da_rtbl207nit, a2_da_ctbl207nit),
	DIM_LUT_INIT(220, 292, GAMMA_2_15, a2_da_rtbl220nit, a2_da_ctbl220nit),
	DIM_LUT_INIT(234, 292, GAMMA_2_15, a2_da_rtbl234nit, a2_da_ctbl234nit),
	DIM_LUT_INIT(249, 300, GAMMA_2_15, a2_da_rtbl249nit, a2_da_ctbl249nit),
	DIM_LUT_INIT(265, 319, GAMMA_2_15, a2_da_rtbl265nit, a2_da_ctbl265nit),
	DIM_LUT_INIT(282, 332, GAMMA_2_15, a2_da_rtbl282nit, a2_da_ctbl282nit),
	DIM_LUT_INIT(300, 348, GAMMA_2_15, a2_da_rtbl300nit, a2_da_ctbl300nit),
	DIM_LUT_INIT(316, 364, GAMMA_2_15, a2_da_rtbl316nit, a2_da_ctbl316nit),
	DIM_LUT_INIT(333, 378, GAMMA_2_15, a2_da_rtbl333nit, a2_da_ctbl333nit),
	DIM_LUT_INIT(350, 392, GAMMA_2_15, a2_da_rtbl350nit, a2_da_ctbl350nit),
	DIM_LUT_INIT(357, 402, GAMMA_2_15, a2_da_rtbl357nit, a2_da_ctbl357nit),
	DIM_LUT_INIT(365, 406, GAMMA_2_15, a2_da_rtbl365nit, a2_da_ctbl365nit),
	DIM_LUT_INIT(372, 406, GAMMA_2_15, a2_da_rtbl372nit, a2_da_ctbl372nit),
	DIM_LUT_INIT(380, 406, GAMMA_2_15, a2_da_rtbl380nit, a2_da_ctbl380nit),
	DIM_LUT_INIT(387, 406, GAMMA_2_15, a2_da_rtbl387nit, a2_da_ctbl387nit),
	DIM_LUT_INIT(395, 406, GAMMA_2_15, a2_da_rtbl395nit, a2_da_ctbl395nit),
	DIM_LUT_INIT(403, 409, GAMMA_2_15, a2_da_rtbl403nit, a2_da_ctbl403nit),
	DIM_LUT_INIT(412, 416, GAMMA_2_15, a2_da_rtbl412nit, a2_da_ctbl412nit),
	DIM_LUT_INIT(420, 420, GAMMA_2_20, a2_da_rtbl420nit, a2_da_ctbl420nit),
};

static unsigned int a2_da_brt_tbl[S6E3HF4_MAX_BRIGHTNESS + 1] = {
	0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 3, 3,
	3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5,
	5, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12,
	12, 13, 13, 14, 14, 15, 15, 16, 16, 17, 18, 18, 19, 19, 20, 20,
	21, 21, 22, 22, 23, 23, 24, 25, 25, 26, 26, 27, 27, 28, 28, 29,
	29, 30, 30, 31, 31, 32, 32, 33, 33, 34, 34, 35, 35, 36, 36, 37,
	37, 38, 38, 39, 39, 40, 40, 41, 41, 42, 42, 43, 43, 44, 44, 45,
	45, 46, 46, 47, 47, 48, 48, 49, 49, 50, 50, 51, 51, 52, 52, 53,
	53, 54, 54, 54, 54, 54, 54, 54, 55, 55, 55, 55, 55, 55, 55, 56,
	56, 56, 56, 56, 56, 56, 57, 57, 57, 57, 57, 57, 57, 57, 58, 58,
	58, 58, 58, 58, 58, 58, 59, 59, 59, 59, 59, 59, 59, 59, 59, 60,
	60, 60, 60, 60, 60, 60, 60, 60, 61, 61, 61, 61, 61, 61, 61, 61,
	61, 61, 62, 62, 62, 62, 62, 62, 62, 62, 63, 63, 63, 63, 63, 63,
	63, 63, 63, 64, 64, 64, 64, 64, 64, 64, 64, 64, 65, 65, 65, 65,
	66, 66, 66, 66, 67, 67, 67, 68, 68, 68, 68, 69, 69, 69, 69, 70,
	70, 70, 70, 71, 71, 71, 71, 72, 72, 72, 72, 73, 73, 73, 73, 73,
	[256 ... 281] = 74,
	[282 ... 295] = 75,
	[296 ... 309] = 76,
	[310 ... 323] = 77,
	[324 ... 336] = 78,
	[337 ... 350] = 79,
	[351 ... 364] = 80,
	[365 ... 365] = 81,
};

static unsigned int a2_da_lum_tbl[S6E3HF4_TOTAL_NR_LUMINANCE] = {
	2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
	12, 13, 14, 15, 16, 17, 19, 20, 21, 22,
	24, 25, 27, 29, 30, 32, 34, 37, 39, 41, 44,
	47, 50, 53, 56, 60, 64, 68, 72, 77, 82, 87,
	93, 98, 105, 111, 119, 126, 134, 143, 152, 162, 172,
	183, 195, 207, 220, 234, 249, 265, 282, 300, 316, 333,
	350, 357, 365, 372, 380, 387, 395, 403, 412, 420,
	443, 465, 488, 510, 533, 555, 578, 600
};

struct brightness_table s6e3hf4_a2_da_panel_brightness_table = {
	.brt = a2_da_brt_tbl,
	.sz_brt = ARRAY_SIZE(a2_da_brt_tbl),
	.lum = a2_da_lum_tbl,
	.sz_lum = ARRAY_SIZE(a2_da_lum_tbl),
	.brt_to_step = a2_da_brt_tbl,
	.sz_brt_to_step = ARRAY_SIZE(a2_da_brt_tbl),
};

static struct panel_dimming_info s6e3hf4_a2_da_panel_dimming_info = {
	.name = "s6e3hf4_A2_DA",
	.dim_init_info = {
		.name = "s6e3hf4_A2_DA",
		.nr_tp = S6E3HF4_NR_TP,
		.tp = s6e3hf4_tp,
		.nr_luminance = S6E3HF4_NR_LUMINANCE,
		.vregout = 109051904LL,	/* 6.5 * 2^24*/
		.bitshift = 24,
		.vt_voltage = {
			0, 12, 24, 36, 48, 60, 72, 84, 96, 108,
			138, 148, 158, 168, 178, 186,
		},
		.target_luminance = S6E3HF4_TARGET_LUMINANCE,
		.target_gamma = 220,
		.dim_lut = a2_da_dimming_lut,
	},
	.target_luminance = S6E3HF4_TARGET_LUMINANCE,
	.nr_luminance = S6E3HF4_NR_LUMINANCE,
	.hbm_target_luminance = S6E3HF4_TARGET_HBM_LUMINANCE,
	.nr_hbm_luminance = S6E3HF4_NR_HBM_LUMINANCE,
	.extend_hbm_target_luminance = -1,
	.nr_extend_hbm_luminance = 0,
	.brt_tbl = &s6e3hf4_a2_da_panel_brightness_table,
};
#endif /* __S6E3HF4_A2_DA_PANEL_DIMMING_H___ */
