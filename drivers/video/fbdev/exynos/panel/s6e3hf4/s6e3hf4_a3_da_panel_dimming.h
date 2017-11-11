/*
 * linux/drivers/video/fbdev/exynos/panel/s6e3hf4/s6e3hf4_a3_da_panel_dimming.h
 *
 * Header file for S6E3HF4 Dimming Driver
 *
 * Copyright (c) 2016 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __S6E3HF4_A3_DA_PANEL_DIMMING_H___
#define __S6E3HF4_A3_DA_PANEL_DIMMING_H___
#include "../dimming.h"
#include "../panel_dimming.h"
#include "s6e3hf4_dimming.h"

/*
 * PANEL INFORMATION
 * LDI : S6E3HF4
 * PANEL : A3 DA
 */
static s32 a3_da_rtbl2nit[] = {0, 0, 31, 31, 31, 28, 25, 19, 13, 6, 0};
static s32 a3_da_rtbl3nit[] = {0, 0, 28, 28, 28, 25, 21, 16, 9, 5, 0};
static s32 a3_da_rtbl4nit[] = {0, 0, 27, 27, 25, 22, 19, 15, 9, 5, 0};
static s32 a3_da_rtbl5nit[] = {0, 0, 24, 24, 22, 20, 18, 14, 9, 5, 0};
static s32 a3_da_rtbl6nit[] = {0, 0, 24, 24, 21, 19, 17, 13, 8, 5, 0};
static s32 a3_da_rtbl7nit[] = {0, 0, 23, 23, 20, 18, 16, 12, 7, 4, 0};
static s32 a3_da_rtbl8nit[] = {0, 0, 22, 22, 19, 17, 15, 11, 6, 4, 0};
static s32 a3_da_rtbl9nit[] = {0, 0, 20, 20, 17, 15, 14, 10, 5, 3, 0};
static s32 a3_da_rtbl10nit[] = {0, 0, 19, 19, 16, 14, 13, 9, 5, 3, 0};
static s32 a3_da_rtbl11nit[] = {0, 0, 19, 19, 16, 14, 13, 9, 5, 4, 0};
static s32 a3_da_rtbl12nit[] = {0, 0, 18, 18, 15, 13, 12, 8, 5, 3, 0};
static s32 a3_da_rtbl13nit[] = {0, 0, 17, 17, 14, 12, 11, 7, 4, 3, 0};
static s32 a3_da_rtbl14nit[] = {0, 0, 17, 17, 14, 12, 11, 7, 4, 3, 0};
static s32 a3_da_rtbl15nit[] = {0, 0, 17, 16, 13, 11, 10, 7, 4, 2, 0};
static s32 a3_da_rtbl16nit[] = {0, 0, 17, 16, 13, 11, 10, 6, 3, 2, 0};
static s32 a3_da_rtbl17nit[] = {0, 0, 18, 16, 13, 11, 9, 5, 3, 2, 0};
static s32 a3_da_rtbl19nit[] = {0, 0, 16, 14, 11, 10, 9, 5, 3, 2, 0};
static s32 a3_da_rtbl20nit[] = {0, 0, 16, 14, 10, 9, 8, 4, 2, 2, 0};
static s32 a3_da_rtbl21nit[] = {0, 0, 16, 14, 10, 9, 8, 4, 2, 2, 0};
static s32 a3_da_rtbl22nit[] = {0, 0, 15, 13, 9, 8, 7, 4, 2, 2, 0};
static s32 a3_da_rtbl24nit[] = {0, 0, 14, 12, 8, 7, 6, 3, 2, 2, 0};
static s32 a3_da_rtbl25nit[] = {0, 0, 14, 12, 8, 7, 6, 3, 1, 2, 0};
static s32 a3_da_rtbl27nit[] = {0, 0, 13, 11, 8, 7, 6, 3, 1, 2, 0};
static s32 a3_da_rtbl29nit[] = {0, 0, 13, 11, 8, 7, 6, 3, 1, 1, 0};
static s32 a3_da_rtbl30nit[] = {0, 0, 12, 10, 7, 6, 5, 3, 1, 1, 0};
static s32 a3_da_rtbl32nit[] = {0, 0, 12, 10, 7, 6, 5, 2, 1, 1, 0};
static s32 a3_da_rtbl34nit[] = {0, 0, 11, 9, 6, 5, 4, 2, 1, 1, 0};
static s32 a3_da_rtbl37nit[] = {0, 0, 10, 8, 5, 4, 4, 2, 1, 1, 0};
static s32 a3_da_rtbl39nit[] = {0, 0, 10, 8, 5, 4, 4, 2, 1, 1, 0};
static s32 a3_da_rtbl41nit[] = {0, 0, 10, 8, 5, 4, 4, 1, 1, 1, 0};
static s32 a3_da_rtbl44nit[] = {0, 0, 8, 6, 4, 3, 3, 1, 1, 1, 0};
static s32 a3_da_rtbl47nit[] = {0, 0, 8, 6, 4, 3, 3, 1, 1, 0, 0};
static s32 a3_da_rtbl50nit[] = {0, 0, 8, 6, 4, 3, 3, 0, 0, 0, 0};
static s32 a3_da_rtbl53nit[] = {0, 0, 7, 5, 3, 2, 2, 0, 0, 0, 0};
static s32 a3_da_rtbl56nit[] = {0, 0, 6, 4, 2, 2, 2, 0, 0, 0, 0};
static s32 a3_da_rtbl60nit[] = {0, 0, 6, 4, 2, 2, 2, 0, 0, 0, 0};
static s32 a3_da_rtbl64nit[] = {0, 0, 6, 4, 2, 2, 2, 0, 0, 0, 0};
static s32 a3_da_rtbl68nit[] = {0, 0, 5, 3, 2, 1, 2, 0, 0, 2, 0};
static s32 a3_da_rtbl72nit[] = {0, 0, 5, 3, 2, 1, 1, 0, 0, 2, 0};
static s32 a3_da_rtbl77nit[] = {0, 0, 5, 4, 2, 1, 1, 1, 0, 1, 0};
static s32 a3_da_rtbl82nit[] = {0, 0, 5, 4, 2, 1, 1, 0, 1, 1, 0};
static s32 a3_da_rtbl87nit[] = {0, 0, 6, 5, 2, 1, 1, 1, 1, 0, 0};
static s32 a3_da_rtbl93nit[] = {0, 0, 4, 3, 2, 1, 1, 1, 2, 0, 0};
static s32 a3_da_rtbl98nit[] = {0, 0, 4, 3, 2, 1, 1, 1, 1, 0, 0};
static s32 a3_da_rtbl105nit[] = {0, 0, 4, 3, 1, 1, 1, 1, 2, 1, 0};
static s32 a3_da_rtbl111nit[] = {0, 0, 5, 4, 2, 1, 2, 1, 2, 0, 0};
static s32 a3_da_rtbl119nit[] = {0, 0, 4, 2, 1, 1, 1, 2, 3, 0, 0};
static s32 a3_da_rtbl126nit[] = {0, 0, 4, 3, 2, 2, 2, 2, 3, 0, 0};
static s32 a3_da_rtbl134nit[] = {0, 0, 4, 3, 2, 1, 2, 2, 2, 0, 0};
static s32 a3_da_rtbl143nit[] = {0, 0, 3, 2, 1, 1, 1, 2, 3, 0, 0};
static s32 a3_da_rtbl152nit[] = {0, 0, 4, 3, 2, 1, 1, 2, 3, 0, 0};
static s32 a3_da_rtbl162nit[] = {0, 0, 3, 1, 1, 1, 1, 2, 3, 0, 0};
static s32 a3_da_rtbl172nit[] = {0, 0, 3, 1, 1, 1, 1, 2, 4, 1, 0};
static s32 a3_da_rtbl183nit[] = {0, 0, 2, 1, 1, 1, 1, 1, 2, 1, 0};
static s32 a3_da_rtbl195nit[] = {0, 0, 2, 1, 1, 1, 1, 1, 2, 0, 0};
static s32 a3_da_rtbl207nit[] = {0, 0, 2, 1, 0, 1, 1, 1, 2, 1, 0};
static s32 a3_da_rtbl220nit[] = {0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0};
static s32 a3_da_rtbl234nit[] = {0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0};
static s32 a3_da_rtbl249nit[] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0};
static s32 a3_da_rtbl265nit[] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0};
static s32 a3_da_rtbl282nit[] = {0, 0, 1, 0, 0, 0, 0, 0, 0, 2, 0};
static s32 a3_da_rtbl300nit[] = {0, 0, 1, 0, 0, 0, 0, 0, 0, 2, 0};
static s32 a3_da_rtbl316nit[] = {0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0};
static s32 a3_da_rtbl333nit[] = {0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0};
static s32 a3_da_rtbl350nit[] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0};
static s32 a3_da_rtbl357nit[] = {0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0};
static s32 a3_da_rtbl365nit[] = {0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0};
static s32 a3_da_rtbl372nit[] = {0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0};
static s32 a3_da_rtbl380nit[] = {0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0};
static s32 a3_da_rtbl387nit[] = {0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0};
static s32 a3_da_rtbl395nit[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static s32 a3_da_rtbl403nit[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static s32 a3_da_rtbl412nit[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static s32 a3_da_rtbl420nit[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static s32 a3_da_ctbl2nit[] = {0, 0, 0, 0, 0, 0, -13, 4, -22, -9, 4, -14, -6, 1, -15, -7, 1, -12, -15, 0, -15, -9, 1, -8, -5, 0, -6, -2, 0, -2, -7, 0, -10};
static s32 a3_da_ctbl3nit[] = {0, 0, 0, 0, 0, 0, -13, 4, -20, 1, 0, -14, -9, 0, -17, -11, -1, -13, -15, 0, -15, -9, 1, -9, -4, 1, -4, -1, 1, -1, -6, 0, -9};
static s32 a3_da_ctbl4nit[] = {0, 0, 0, 0, 0, 0, -13, 4, -20, 1, 0, -14, -10, 0, -19, -10, 1, -12, -12, 2, -14, -10, 1, -10, -3, 1, -3, -2, 0, -2, -4, 0, -7};
static s32 a3_da_ctbl5nit[] = {0, 0, 0, 0, 0, 0, -15, 1, -22, 1, 5, -14, -8, 3, -17, -7, 3, -10, -12, 0, -15, -8, 1, -9, -4, 0, -3, -2, 0, -2, -3, 0, -6};
static s32 a3_da_ctbl6nit[] = {0, 0, 0, 0, 0, 0, -15, 1, -22, -2, 2, -18, -9, 3, -16, -6, 3, -11, -11, 2, -14, -8, 1, -9, -3, 0, -3, -2, 0, -2, -2, 0, -5};
static s32 a3_da_ctbl7nit[] = {0, 0, 0, 0, 0, 0, -15, 2, -22, -5, 3, -20, -8, 2, -17, -6, 3, -11, -12, 0, -15, -8, 1, -9, -2, 0, -2, -1, 0, -1, -2, 0, -5};
static s32 a3_da_ctbl8nit[] = {0, 0, 0, 0, 0, 0, -15, 2, -22, -5, 4, -21, -8, 2, -16, -8, 1, -14, -11, 0, -14, -7, 1, -8, -2, 0, -2, -1, 0, -1, -1, 0, -4};
static s32 a3_da_ctbl9nit[] = {0, 0, 0, 0, 0, 0, -12, 4, -19, -6, 5, -22, -6, 3, -15, -5, 4, -11, -10, 2, -13, -7, 0, -9, -2, 1, -2, 0, 0, 0, -1, 0, -4};
static s32 a3_da_ctbl10nit[] = {0, 0, 0, 0, 0, 0, -16, 0, -23, -5, 4, -22, -7, 4, -15, -4, 4, -10, -10, 2, -13, -5, 1, -7, -3, 0, -3, 0, 0, 0, 0, 0, -3};
static s32 a3_da_ctbl11nit[] = {0, 0, 0, 0, 0, 0, -16, 0, -23, -5, 3, -23, -7, 3, -16, -4, 3, -10, -11, 0, -14, -5, 1, -7, -2, 0, -2, 0, 0, 0, 0, 0, -3};
static s32 a3_da_ctbl12nit[] = {0, 0, 0, 0, 0, 0, -13, 2, -21, -5, 3, -21, -8, 2, -18, -2, 4, -9, -9, 2, -14, -6, 1, -6, -1, 0, -2, -1, 0, 0, 1, 0, -2};
static s32 a3_da_ctbl13nit[] = {0, 0, 0, 0, 0, 0, 3, -3, -1, -7, 2, -25, -8, 2, -18, -1, 4, -8, -10, 0, -14, -4, 2, -5, -2, 0, -2, 0, 0, 0, 1, 0, -2};
static s32 a3_da_ctbl14nit[] = {0, 0, 0, 0, 0, 0, 3, -3, -1, -7, 2, -25, -8, 2, -18, -1, 4, -8, -10, 0, -14, -4, 2, -5, -2, 0, -2, 0, 0, 0, 1, 0, -2};
static s32 a3_da_ctbl15nit[] = {0, 0, 0, 0, 0, 0, -2, 11, -2, -7, 2, -26, -9, 2, -19, -1, 4, -7, -8, 3, -12, -5, 1, -6, -2, 0, -2, 0, 0, 0, 2, 0, -1};
static s32 a3_da_ctbl16nit[] = {0, 0, 0, 0, 0, 0, 5, 9, 2, -8, 1, -27, -9, 0, -20, -3, 4, -9, -10, 1, -13, -4, 0, -5, -2, 0, -2, 1, 0, 1, 1, 0, -2};
static s32 a3_da_ctbl17nit[] = {0, 0, 0, 0, 0, 0, -2, 2, -4, -9, 1, -28, -11, -2, -22, -7, -1, -11, -9, 1, -13, -3, 1, -3, -2, 0, -2, 0, 0, 0, 2, 0, -1};
static s32 a3_da_ctbl19nit[] = {0, 0, 0, 0, 0, 0, -3, 3, -5, -7, 5, -26, -7, 3, -19, -2, 2, -8, -11, -1, -13, -4, 0, -4, -1, 0, -1, 0, 0, 0, 2, 0, -1};
static s32 a3_da_ctbl20nit[] = {0, 0, 0, 0, 0, 0, -3, 2, -5, -11, -2, -32, -8, 3, -18, -2, 3, -6, -10, 0, -13, -2, 1, -3, -2, 0, -1, 1, 0, 0, 1, 0, -1};
static s32 a3_da_ctbl21nit[] = {0, 0, 0, 0, 0, 0, 0, 2, -5, -13, -3, -33, -8, 2, -18, -3, 2, -7, -10, -1, -13, -3, 1, -3, -1, 0, -1, 1, 0, 0, 1, 0, -1};
static s32 a3_da_ctbl22nit[] = {0, 0, 0, 0, 0, 0, 1, 4, -2, -10, -1, -32, -8, 2, -18, -4, 3, -6, -7, 1, -10, -3, 1, -3, -1, 0, -1, 0, 0, 0, 2, 0, -1};
static s32 a3_da_ctbl24nit[] = {0, 0, 0, 0, 0, 0, -3, 3, -5, -12, -2, -33, -6, 3, -17, -2, 2, -4, -7, 2, -10, -2, 1, -3, -1, 0, 0, 1, 0, 0, 1, 0, -1};
static s32 a3_da_ctbl25nit[] = {0, 0, 0, 0, 0, 0, -2, 3, -5, -13, -4, -35, -7, 2, -17, -2, 2, -4, -8, 1, -11, -3, 1, -2, 1, 1, 0, 0, 0, 1, 2, 0, -1};
static s32 a3_da_ctbl27nit[] = {0, 0, 0, 0, 0, 0, -4, 2, -7, -5, 5, -27, -9, 1, -19, -3, 0, -5, -8, 0, -10, -4, 0, -3, 1, 1, 0, 0, 0, 1, 2, 0, -1};
static s32 a3_da_ctbl29nit[] = {0, 0, 0, 0, 0, 0, -4, 1, -6, -7, 3, -30, -10, -1, -19, -3, -1, -5, -9, -1, -11, -3, 0, -3, -1, 0, 0, 1, 0, 1, 2, 0, -1};
static s32 a3_da_ctbl30nit[] = {0, 0, 0, 0, 0, 0, -3, 2, -7, -6, 3, -30, -8, 1, -17, -4, 0, -4, -5, 2, -7, -3, 0, -3, 0, 0, 0, 0, 0, 1, 2, 0, -1};
static s32 a3_da_ctbl32nit[] = {0, 0, 0, 0, 0, 0, -3, 3, -6, -8, 1, -32, -9, -1, -17, -5, -1, -5, -6, 0, -9, -2, 1, -2, -1, 0, 0, 1, 0, 1, 2, 0, -1};
static s32 a3_da_ctbl34nit[] = {0, 0, 0, 0, 0, 0, -2, 2, -7, -10, 0, -33, -5, 2, -13, -5, -2, -6, -3, 3, -6, -2, 1, -2, 0, 0, 0, 0, 0, 1, 2, 0, -1};
static s32 a3_da_ctbl37nit[] = {0, 0, 0, 0, 0, 0, -3, 2, -8, -11, 0, -35, -4, 3, -11, 0, 3, -1, -4, 2, -6, -2, 0, -2, -1, 0, -1, 1, 0, 1, 2, 0, 0};
static s32 a3_da_ctbl39nit[] = {0, 0, 0, 0, 0, 0, -3, 2, -8, -13, -2, -36, -5, 1, -12, -1, 2, -2, -4, 2, -6, -2, 0, -2, -1, -1, -1, 1, 0, 1, 2, 0, 0};
static s32 a3_da_ctbl41nit[] = {0, 0, 0, 0, 0, 0, -4, 2, -9, -15, -3, -38, -6, -1, -13, -1, 2, -1, -6, -1, -8, -1, 1, -1, -1, -1, -1, 1, 0, 1, 2, 0, 0};
static s32 a3_da_ctbl44nit[] = {0, 0, 0, 0, 0, 0, -4, 3, -10, -6, 6, -28, -3, 2, -9, -2, 1, -1, -2, 3, -4, -1, 1, -1, -1, -1, -1, 1, 0, 1, 2, 0, 0};
static s32 a3_da_ctbl47nit[] = {0, 0, 0, 0, 0, 0, -4, 2, -10, -9, 4, -30, -5, 1, -10, -2, 0, -1, -3, 2, -5, -1, 0, -1, -1, -1, -1, 1, 0, 1, 2, 0, 0};
static s32 a3_da_ctbl50nit[] = {0, 0, 0, 0, 0, 0, -3, 2, -10, -10, 1, -32, -6, -1, -10, -3, -1, -2, -4, -1, -6, 0, 1, 0, -1, 0, 0, 1, 0, 1, 2, 0, 0};
static s32 a3_da_ctbl53nit[] = {0, 0, 0, 0, 0, 0, -2, 1, -10, -11, 2, -32, -6, -1, -10, 0, 0, 0, -1, 3, -2, 0, 1, 0, -1, 0, 0, 1, 0, 1, 2, 0, 0};
static s32 a3_da_ctbl56nit[] = {0, 0, 0, 0, 0, 0, -2, 0, -10, -10, 2, -31, 1, 6, -4, 0, 0, 0, -1, 3, -2, 0, 0, 0, -1, 0, 0, 1, 0, 1, 2, 0, 0};
static s32 a3_da_ctbl60nit[] = {0, 0, 0, 0, 0, 0, -3, 0, -11, -13, -1, -33, 0, 5, -4, 0, -1, 0, -2, 1, -3, 0, 1, 0, -1, -1, 0, 1, 0, 1, 2, 0, 0};
static s32 a3_da_ctbl64nit[] = {0, 0, 0, 0, 0, 0, -3, -1, -12, -16, -3, -35, 0, 4, -4, -2, -3, -1, -1, 1, -3, -1, 0, 0, -1, -1, 0, 1, 0, 1, 2, 0, 0};
static s32 a3_da_ctbl68nit[] = {0, 0, 0, 0, 0, 0, -4, 0, -12, -6, 7, -25, 0, 3, -4, 2, 2, 3, -3, -1, -5, -1, 0, 0, 0, 0, -1, 0, 0, 1, 2, 0, 0};
static s32 a3_da_ctbl72nit[] = {0, 0, 0, 0, 0, 0, -4, 0, -12, -7, 5, -26, 0, 2, -4, 3, 3, 4, -2, 1, -3, 1, 1, 1, 0, -1, 0, 0, 0, 0, 2, 0, 1};
static s32 a3_da_ctbl77nit[] = {0, 0, 0, 0, 0, 0, -3, -3, -13, -13, 1, -29, -2, 0, -6, 1, -1, 2, -2, 1, -4, 0, 0, 0, 0, 0, 0, 1, 0, 2, 2, 0, 0};
static s32 a3_da_ctbl82nit[] = {0, 0, 0, 0, 0, 0, -4, -2, -13, -15, -1, -30, -2, -1, -6, 1, -1, 2, 1, 3, -2, 0, 0, 1, 0, 1, 1, 0, 0, 1, 3, 0, 1};
static s32 a3_da_ctbl87nit[] = {0, 0, 0, 0, 0, 0, -5, -2, -14, -26, -14, -41, 4, 5, 1, -1, -3, 0, 0, 3, -1, -1, 0, -1, 1, 0, 1, 1, 0, 1, 2, 0, 0};
static s32 a3_da_ctbl93nit[] = {0, 0, 0, 0, 0, 0, 5, 5, -6, -9, 5, -22, -1, 0, -5, -1, -3, 0, 0, 2, -2, 0, 1, 1, 0, 0, 0, 2, 0, 1, 2, 0, 0};
static s32 a3_da_ctbl98nit[] = {0, 0, 0, 0, 0, 0, 4, 5, -6, -10, 3, -23, -1, 0, -4, 2, 1, 3, -1, 1, -3, 0, 0, 1, 0, 0, 1, 2, 0, 1, 1, 0, 0};
static s32 a3_da_ctbl105nit[] = {0, 0, 0, 0, 0, 0, 4, 4, -7, -11, 2, -23, 3, 5, 0, 1, 0, 1, -1, 2, -2, 1, 1, 1, 0, 0, 1, 1, 0, 0, 2, 0, 1};
static s32 a3_da_ctbl111nit[] = {0, 0, 0, 0, 0, 0, 1, 5, -8, -14, -3, -25, -1, 1, -3, 2, 2, 3, -2, -1, -4, 0, 0, 0, -1, -1, 0, 1, 0, 1, 1, 0, 0};
static s32 a3_da_ctbl119nit[] = {0, 0, 0, 0, 0, 0, 1, 4, -8, -5, 6, -16, 0, 1, -3, 2, 2, 3, 0, 2, -1, 1, 0, 2, -1, 0, -1, 1, 0, 1, 2, 0, 1};
static s32 a3_da_ctbl126nit[] = {0, 0, 0, 0, 0, 0, 1, 4, -9, -9, 1, -19, 1, 3, -1, -1, -2, -1, -1, 0, -3, -1, 0, 0, 0, -1, 0, 1, 0, 0, 1, 0, 1};
static s32 a3_da_ctbl134nit[] = {0, 0, 0, 0, 0, 0, -1, 3, -10, -11, -1, -19, 0, 2, -2, 1, 1, 2, -2, -1, -4, 1, 0, 1, -1, 0, 0, 2, 0, 0, 1, 0, 0};
static s32 a3_da_ctbl143nit[] = {0, 0, 0, 0, 0, 0, -1, 1, -9, -3, 7, -11, 1, 2, -1, 2, 2, 2, 0, 1, -1, 1, 0, 1, -1, 0, -1, 1, 0, 1, 1, 0, 1};
static s32 a3_da_ctbl152nit[] = {0, 0, 0, 0, 0, 0, -3, 1, -11, -9, 0, -14, -3, -2, -4, 0, -1, 0, 0, 2, 0, 0, 0, 0, -1, 0, -1, 1, 0, 1, 1, 0, 1};
static s32 a3_da_ctbl162nit[] = {0, 0, 0, 0, 0, 0, -3, -1, -11, -2, 7, -7, -1, 1, -1, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 2, 0, 1, 0, 0, 0};
static s32 a3_da_ctbl172nit[] = {0, 0, 0, 0, 0, 0, -4, -1, -12, -2, 6, -6, -1, 1, -1, 3, 2, 2, -1, 0, -1, 1, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0};
static s32 a3_da_ctbl183nit[] = {0, 0, 0, 0, 0, 0, 5, 9, -2, 5, 12, 1, -5, -4, -5, 1, 1, 0, -1, -1, -2, 0, 0, -1, 1, 2, 2, 1, 0, 1, 2, 0, 1};
static s32 a3_da_ctbl195nit[] = {0, 0, 0, 0, 0, 0, 4, 7, -1, 3, 9, -1, -5, -5, -6, 2, 0, 2, -1, -1, -2, 0, -1, 0, 1, 1, -1, 0, 0, 1, 3, 0, 1};
static s32 a3_da_ctbl207nit[] = {0, 0, 0, 0, 0, 0, 4, 5, -2, -5, 1, -7, 0, 1, 0, 2, 0, 1, -1, -1, -1, 0, -1, -1, 1, 2, 2, 1, 0, 1, 2, 0, 1};
static s32 a3_da_ctbl220nit[] = {0, 0, 0, 0, 0, 0, 2, 6, -2, 5, 8, 2, -3, -4, -3, 1, 0, 1, -1, -1, -2, 1, 0, 2, 1, 1, 0, 1, 0, 1, 2, 0, 1};
static s32 a3_da_ctbl234nit[] = {0, 0, 0, 0, 0, 0, 2, 6, -2, 5, 8, 2, -3, -4, -3, 1, 0, 1, -3, -3, -4, 3, 2, 3, 1, 1, 0, 1, 0, 1, 2, 0, 1};
static s32 a3_da_ctbl249nit[] = {0, 0, 0, 0, 0, 0, 1, 3, -3, 4, 6, 1, -4, -4, -4, 2, -1, 1, 1, 0, 0, -1, -1, -1, 1, 1, 1, 0, 0, 0, 2, 0, 1};
static s32 a3_da_ctbl265nit[] = {0, 0, 0, 0, 0, 0, 1, 2, -3, 5, 6, 2, 1, 0, 1, 0, -2, 0, 0, -1, 0, 1, 1, 2, 1, 0, 0, 0, 0, 0, 2, 0, 1};
static s32 a3_da_ctbl282nit[] = {0, 0, 0, 0, 0, 0, -4, -2, -5, -2, -2, -4, -3, -3, -3, 1, 0, 1, -3, -4, -2, 1, 0, 1, 0, -1, 0, 1, 1, 1, 1, 0, 0};
static s32 a3_da_ctbl300nit[] = {0, 0, 0, 0, 0, 0, -4, -3, -5, -3, -2, -4, 2, 3, 2, 0, 0, 0, -2, -4, -3, -1, -1, 1, 0, 0, 0, 2, 2, 2, 2, 0, 1};
static s32 a3_da_ctbl316nit[] = {0, 0, 0, 0, 0, 0, -5, -4, -4, 1, 0, 0, -7, -6, -6, -2, -3, -3, 0, -1, 1, 2, 2, 2, -1, -2, -1, 0, 0, 0, 2, 0, 2};
static s32 a3_da_ctbl333nit[] = {0, 0, 0, 0, 0, 0, -7, -5, -4, -2, 0, -2, -3, -4, -3, -2, -3, -2, 0, -1, 1, 1, 0, 0, 1, 1, 1, -1, 0, 0, 2, 0, 0};
static s32 a3_da_ctbl350nit[] = {0, 0, 0, 0, 0, 0, 5, 6, 4, 4, 4, 4, -3, -3, -3, -2, -3, -2, 2, 1, 2, -3, -3, -2, 2, 0, 1, -1, 0, 1, 2, 0, 0};
static s32 a3_da_ctbl357nit[] = {0, 0, 0, 0, 0, 0, 3, 4, 4, -6, -6, -5, -3, -3, -3, 0, 0, 1, -1, -2, -2, 0, -1, 1, 0, 1, 0, 0, 0, 1, 2, 0, 1};
static s32 a3_da_ctbl365nit[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static s32 a3_da_ctbl372nit[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static s32 a3_da_ctbl380nit[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static s32 a3_da_ctbl387nit[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static s32 a3_da_ctbl395nit[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static s32 a3_da_ctbl403nit[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static s32 a3_da_ctbl412nit[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static s32 a3_da_ctbl420nit[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

struct dimming_lut a3_da_dimming_lut[] = {
	DIM_LUT_INIT(2, 116, GAMMA_2_15, a3_da_rtbl2nit, a3_da_ctbl2nit),
	DIM_LUT_INIT(3, 116, GAMMA_2_15, a3_da_rtbl3nit, a3_da_ctbl3nit),
	DIM_LUT_INIT(4, 116, GAMMA_2_15, a3_da_rtbl4nit, a3_da_ctbl4nit),
	DIM_LUT_INIT(5, 116, GAMMA_2_15, a3_da_rtbl5nit, a3_da_ctbl5nit),
	DIM_LUT_INIT(6, 116, GAMMA_2_15, a3_da_rtbl6nit, a3_da_ctbl6nit),
	DIM_LUT_INIT(7, 116, GAMMA_2_15, a3_da_rtbl7nit, a3_da_ctbl7nit),
	DIM_LUT_INIT(8, 116, GAMMA_2_15, a3_da_rtbl8nit, a3_da_ctbl8nit),
	DIM_LUT_INIT(9, 116, GAMMA_2_15, a3_da_rtbl9nit, a3_da_ctbl9nit),
	DIM_LUT_INIT(10, 116, GAMMA_2_15, a3_da_rtbl10nit, a3_da_ctbl10nit),
	DIM_LUT_INIT(11, 116, GAMMA_2_15, a3_da_rtbl11nit, a3_da_ctbl11nit),
	DIM_LUT_INIT(12, 116, GAMMA_2_15, a3_da_rtbl12nit, a3_da_ctbl12nit),
	DIM_LUT_INIT(13, 116, GAMMA_2_15, a3_da_rtbl13nit, a3_da_ctbl13nit),
	DIM_LUT_INIT(14, 116, GAMMA_2_15, a3_da_rtbl14nit, a3_da_ctbl14nit),
	DIM_LUT_INIT(15, 116, GAMMA_2_15, a3_da_rtbl15nit, a3_da_ctbl15nit),
	DIM_LUT_INIT(16, 116, GAMMA_2_15, a3_da_rtbl16nit, a3_da_ctbl16nit),
	DIM_LUT_INIT(17, 116, GAMMA_2_15, a3_da_rtbl17nit, a3_da_ctbl17nit),
	DIM_LUT_INIT(19, 116, GAMMA_2_15, a3_da_rtbl19nit, a3_da_ctbl19nit),
	DIM_LUT_INIT(20, 116, GAMMA_2_15, a3_da_rtbl20nit, a3_da_ctbl20nit),
	DIM_LUT_INIT(21, 116, GAMMA_2_15, a3_da_rtbl21nit, a3_da_ctbl21nit),
	DIM_LUT_INIT(22, 116, GAMMA_2_15, a3_da_rtbl22nit, a3_da_ctbl22nit),
	DIM_LUT_INIT(24, 116, GAMMA_2_15, a3_da_rtbl24nit, a3_da_ctbl24nit),
	DIM_LUT_INIT(25, 116, GAMMA_2_15, a3_da_rtbl25nit, a3_da_ctbl25nit),
	DIM_LUT_INIT(27, 116, GAMMA_2_15, a3_da_rtbl27nit, a3_da_ctbl27nit),
	DIM_LUT_INIT(29, 116, GAMMA_2_15, a3_da_rtbl29nit, a3_da_ctbl29nit),
	DIM_LUT_INIT(30, 116, GAMMA_2_15, a3_da_rtbl30nit, a3_da_ctbl30nit),
	DIM_LUT_INIT(32, 116, GAMMA_2_15, a3_da_rtbl32nit, a3_da_ctbl32nit),
	DIM_LUT_INIT(34, 116, GAMMA_2_15, a3_da_rtbl34nit, a3_da_ctbl34nit),
	DIM_LUT_INIT(37, 116, GAMMA_2_15, a3_da_rtbl37nit, a3_da_ctbl37nit),
	DIM_LUT_INIT(39, 116, GAMMA_2_15, a3_da_rtbl39nit, a3_da_ctbl39nit),
	DIM_LUT_INIT(41, 116, GAMMA_2_15, a3_da_rtbl41nit, a3_da_ctbl41nit),
	DIM_LUT_INIT(44, 116, GAMMA_2_15, a3_da_rtbl44nit, a3_da_ctbl44nit),
	DIM_LUT_INIT(47, 116, GAMMA_2_15, a3_da_rtbl47nit, a3_da_ctbl47nit),
	DIM_LUT_INIT(50, 116, GAMMA_2_15, a3_da_rtbl50nit, a3_da_ctbl50nit),
	DIM_LUT_INIT(53, 116, GAMMA_2_15, a3_da_rtbl53nit, a3_da_ctbl53nit),
	DIM_LUT_INIT(56, 116, GAMMA_2_15, a3_da_rtbl56nit, a3_da_ctbl56nit),
	DIM_LUT_INIT(60, 116, GAMMA_2_15, a3_da_rtbl60nit, a3_da_ctbl60nit),
	DIM_LUT_INIT(64, 116, GAMMA_2_15, a3_da_rtbl64nit, a3_da_ctbl64nit),
	DIM_LUT_INIT(68, 121, GAMMA_2_15, a3_da_rtbl68nit, a3_da_ctbl68nit),
	DIM_LUT_INIT(72, 127, GAMMA_2_15, a3_da_rtbl72nit, a3_da_ctbl72nit),
	DIM_LUT_INIT(77, 137, GAMMA_2_15, a3_da_rtbl77nit, a3_da_ctbl77nit),
	DIM_LUT_INIT(82, 142, GAMMA_2_15, a3_da_rtbl82nit, a3_da_ctbl82nit),
	DIM_LUT_INIT(87, 153, GAMMA_2_15, a3_da_rtbl87nit, a3_da_ctbl87nit),
	DIM_LUT_INIT(93, 163, GAMMA_2_15, a3_da_rtbl93nit, a3_da_ctbl93nit),
	DIM_LUT_INIT(98, 172, GAMMA_2_15, a3_da_rtbl98nit, a3_da_ctbl98nit),
	DIM_LUT_INIT(105, 181, GAMMA_2_15, a3_da_rtbl105nit, a3_da_ctbl105nit),
	DIM_LUT_INIT(111, 193, GAMMA_2_15, a3_da_rtbl111nit, a3_da_ctbl111nit),
	DIM_LUT_INIT(119, 205, GAMMA_2_15, a3_da_rtbl119nit, a3_da_ctbl119nit),
	DIM_LUT_INIT(126, 217, GAMMA_2_15, a3_da_rtbl126nit, a3_da_ctbl126nit),
	DIM_LUT_INIT(134, 228, GAMMA_2_15, a3_da_rtbl134nit, a3_da_ctbl134nit),
	DIM_LUT_INIT(143, 241, GAMMA_2_15, a3_da_rtbl143nit, a3_da_ctbl143nit),
	DIM_LUT_INIT(152, 252, GAMMA_2_15, a3_da_rtbl152nit, a3_da_ctbl152nit),
	DIM_LUT_INIT(162, 265, GAMMA_2_15, a3_da_rtbl162nit, a3_da_ctbl162nit),
	DIM_LUT_INIT(172, 277, GAMMA_2_15, a3_da_rtbl172nit, a3_da_ctbl172nit),
	DIM_LUT_INIT(183, 294, GAMMA_2_15, a3_da_rtbl183nit, a3_da_ctbl183nit),
	DIM_LUT_INIT(195, 294, GAMMA_2_15, a3_da_rtbl195nit, a3_da_ctbl195nit),
	DIM_LUT_INIT(207, 294, GAMMA_2_15, a3_da_rtbl207nit, a3_da_ctbl207nit),
	DIM_LUT_INIT(220, 294, GAMMA_2_15, a3_da_rtbl220nit, a3_da_ctbl220nit),
	DIM_LUT_INIT(234, 294, GAMMA_2_15, a3_da_rtbl234nit, a3_da_ctbl234nit),
	DIM_LUT_INIT(249, 300, GAMMA_2_15, a3_da_rtbl249nit, a3_da_ctbl249nit),
	DIM_LUT_INIT(265, 316, GAMMA_2_15, a3_da_rtbl265nit, a3_da_ctbl265nit),
	DIM_LUT_INIT(282, 332, GAMMA_2_15, a3_da_rtbl282nit, a3_da_ctbl282nit),
	DIM_LUT_INIT(300, 348, GAMMA_2_15, a3_da_rtbl300nit, a3_da_ctbl300nit),
	DIM_LUT_INIT(316, 361, GAMMA_2_15, a3_da_rtbl316nit, a3_da_ctbl316nit),
	DIM_LUT_INIT(333, 378, GAMMA_2_15, a3_da_rtbl333nit, a3_da_ctbl333nit),
	DIM_LUT_INIT(350, 395, GAMMA_2_15, a3_da_rtbl350nit, a3_da_ctbl350nit),
	DIM_LUT_INIT(357, 399, GAMMA_2_15, a3_da_rtbl357nit, a3_da_ctbl357nit),
	DIM_LUT_INIT(365, 406, GAMMA_2_15, a3_da_rtbl365nit, a3_da_ctbl365nit),
	DIM_LUT_INIT(372, 406, GAMMA_2_15, a3_da_rtbl372nit, a3_da_ctbl372nit),
	DIM_LUT_INIT(380, 406, GAMMA_2_15, a3_da_rtbl380nit, a3_da_ctbl380nit),
	DIM_LUT_INIT(387, 406, GAMMA_2_15, a3_da_rtbl387nit, a3_da_ctbl387nit),
	DIM_LUT_INIT(395, 406, GAMMA_2_15, a3_da_rtbl395nit, a3_da_ctbl395nit),
	DIM_LUT_INIT(403, 409, GAMMA_2_15, a3_da_rtbl403nit, a3_da_ctbl403nit),
	DIM_LUT_INIT(412, 416, GAMMA_2_15, a3_da_rtbl412nit, a3_da_ctbl412nit),
	DIM_LUT_INIT(420, 420, GAMMA_2_20, a3_da_rtbl420nit, a3_da_ctbl420nit),
};

static unsigned int a3_da_brt_tbl[S6E3HF4_MAX_BRIGHTNESS + 1] = {
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

static unsigned int a3_da_lum_tbl[S6E3HF4_TOTAL_NR_LUMINANCE] = {
	2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
	12, 13, 14, 15, 16, 17, 19, 20, 21, 22,
	24, 25, 27, 29, 30, 32, 34, 37, 39, 41, 44,
	47, 50, 53, 56, 60, 64, 68, 72, 77, 82, 87,
	93, 98, 105, 111, 119, 126, 134, 143, 152, 162, 172,
	183, 195, 207, 220, 234, 249, 265, 282, 300, 316, 333,
	350, 357, 365, 372, 380, 387, 395, 403, 412, 420,
	443, 465, 488, 510, 533, 555, 578, 600
};

struct brightness_table s6e3hf4_a3_da_panel_brightness_table = {
	.brt = a3_da_brt_tbl,
	.sz_brt = ARRAY_SIZE(a3_da_brt_tbl),
	.lum = a3_da_lum_tbl,
	.sz_lum = ARRAY_SIZE(a3_da_lum_tbl),
	.brt_to_step = a3_da_brt_tbl,
	.sz_brt_to_step = ARRAY_SIZE(a3_da_brt_tbl),
};

static struct panel_dimming_info s6e3hf4_a3_da_panel_dimming_info = {
	.name = "s6e3hf4_a3_da",
	.dim_init_info = {
		.name = "s6e3hf4_a3_da",
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
		.dim_lut = a3_da_dimming_lut,
	},
	.target_luminance = S6E3HF4_TARGET_LUMINANCE,
	.nr_luminance = S6E3HF4_NR_LUMINANCE,
	.hbm_target_luminance = S6E3HF4_TARGET_HBM_LUMINANCE,
	.nr_hbm_luminance = S6E3HF4_NR_HBM_LUMINANCE,
	.extend_hbm_target_luminance = -1,
	.nr_extend_hbm_luminance = 0,
	.brt_tbl = &s6e3hf4_a3_da_panel_brightness_table,
};
#endif /* __S6E3HF4_A3_DA_PANEL_DIMMING_H__ */
