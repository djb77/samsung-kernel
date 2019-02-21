/*
 * linux/drivers/video/fbdev/exynos/panel/s6e3hf3/s6e3hf3_vkc_a2_da_panel_dimming.h
 *
 * Header file for S6E3HF3 Dimming Driver
 *
 * Copyright (c) 2016 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __S6E3HF3_VKC_A2_DA_PANEL_DIMMING_H___
#define __S6E3HF3_VKC_A2_DA_PANEL_DIMMING_H___
#include "../dimming.h"
#include "../panel_dimming.h"
#include "s6e3hf3_dimming.h"

/*
 * PANEL INFORMATION
 * LDI : S6E3HF3
 * PANEL : VALLEY_A2_DA
 */
/* gray scale offset values */
static s32 vkc_a2_da_rtbl2nit[10] = {0, 36, 35, 31, 27, 23, 18, 10, 6, 0};
static s32 vkc_a2_da_rtbl3nit[10] = {0, 36, 33, 29, 25, 21, 18, 11, 8, 0};
static s32 vkc_a2_da_rtbl4nit[10] = {0, 32, 30, 25, 22, 19, 16, 9, 6, 0};
static s32 vkc_a2_da_rtbl5nit[10] = {0, 31, 27, 23, 20, 17, 15, 8, 6, 0};
static s32 vkc_a2_da_rtbl6nit[10] = {0, 30, 27, 23, 19, 16, 14, 7, 6, 0};
static s32 vkc_a2_da_rtbl7nit[10] = {0, 28, 25, 21, 18, 15, 13, 7, 6, 0};
static s32 vkc_a2_da_rtbl8nit[10] = {0, 26, 24, 19, 16, 14, 11, 6, 4, 0};
static s32 vkc_a2_da_rtbl9nit[10] = {0, 25, 23, 18, 15, 13, 11, 6, 5, 0};
static s32 vkc_a2_da_rtbl10nit[10] = {0, 25, 22, 17, 14, 12, 10, 5, 4, 0};
static s32 vkc_a2_da_rtbl11nit[10] = {0, 23, 21, 17, 13, 12, 9, 5, 4, 0};
static s32 vkc_a2_da_rtbl12nit[10] = {0, 22, 20, 16, 12, 11, 9, 5, 4, 0};
static s32 vkc_a2_da_rtbl13nit[10] = {0, 21, 20, 15, 12, 11, 8, 4, 4, 0};
static s32 vkc_a2_da_rtbl14nit[10] = {0, 21, 19, 15, 11, 9, 8, 3, 4, 0};
static s32 vkc_a2_da_rtbl15nit[10] = {0, 20, 18, 14, 11, 9, 8, 4, 4, 0};
static s32 vkc_a2_da_rtbl16nit[10] = {0, 18, 17, 13, 10, 8, 7, 3, 4, 0};
static s32 vkc_a2_da_rtbl17nit[10] = {0, 18, 16, 12, 10, 8, 7, 3, 4, 0};
static s32 vkc_a2_da_rtbl19nit[10] = {0, 18, 15, 11, 9, 7, 6, 3, 3, 0};
static s32 vkc_a2_da_rtbl20nit[10] = {0, 17, 14, 11, 8, 7, 6, 3, 4, 0};
static s32 vkc_a2_da_rtbl21nit[10] = {0, 16, 14, 10, 8, 6, 5, 2, 3, 0};
static s32 vkc_a2_da_rtbl22nit[10] = {0, 17, 14, 10, 8, 6, 5, 2, 3, 0};
static s32 vkc_a2_da_rtbl24nit[10] = {0, 16, 13, 9, 7, 6, 5, 2, 3, 0};
static s32 vkc_a2_da_rtbl25nit[10] = {0, 16, 13, 9, 7, 5, 5, 2, 4, 0};
static s32 vkc_a2_da_rtbl27nit[10] = {0, 14, 12, 8, 6, 5, 5, 2, 4, 0};
static s32 vkc_a2_da_rtbl29nit[10] = {0, 14, 11, 7, 6, 5, 4, 1, 3, 0};
static s32 vkc_a2_da_rtbl30nit[10] = {0, 14, 11, 7, 6, 4, 4, 3, 3, 0};
static s32 vkc_a2_da_rtbl32nit[10] = {0, 13, 10, 7, 5, 4, 3, 1, 3, 0};
static s32 vkc_a2_da_rtbl34nit[10] = {0, 12, 10, 6, 5, 4, 3, 1, 3, 0};
static s32 vkc_a2_da_rtbl37nit[10] = {0, 11, 9, 6, 4, 3, 3, 1, 4, 0};
static s32 vkc_a2_da_rtbl39nit[10] = {0, 11, 8, 5, 4, 3, 3, 1, 3, 0};
static s32 vkc_a2_da_rtbl41nit[10] = {0, 10, 8, 5, 4, 3, 2, 0, 3, 0};
static s32 vkc_a2_da_rtbl44nit[10] = {0, 10, 7, 4, 3, 2, 2, 0, 3, 0};
static s32 vkc_a2_da_rtbl47nit[10] = {0, 9, 6, 4, 3, 2, 2, 0, 3, 0};
static s32 vkc_a2_da_rtbl50nit[10] = {0, 9, 6, 4, 2, 2, 2, 0, 2, 0};
static s32 vkc_a2_da_rtbl53nit[10] = {0, 9, 5, 3, 2, 2, 1, 0, 2, 0};
static s32 vkc_a2_da_rtbl56nit[10] = {0, 8, 5, 3, 2, 2, 1, 0, 3, 0};
static s32 vkc_a2_da_rtbl60nit[10] = {0, 7, 5, 2, 1, 1, 1, 0, 3, 0};
static s32 vkc_a2_da_rtbl64nit[10] = {0, 7, 4, 2, 1, 1, 1, 0, 2, 0};
static s32 vkc_a2_da_rtbl68nit[10] = {0, 7, 4, 2, 1, 1, 1, 0, 2, 0};
static s32 vkc_a2_da_rtbl72nit[10] = {0, 7, 4, 2, 2, 2, 2, 1, 2, 0};
static s32 vkc_a2_da_rtbl77nit[10] = {0, 7, 4, 3, 1, 2, 2, 2, 2, 0};
static s32 vkc_a2_da_rtbl82nit[10] = {0, 7, 4, 2, 2, 1, 2, 2, 3, 0};
static s32 vkc_a2_da_rtbl87nit[10] = {0, 7, 4, 3, 2, 1, 2, 1, 2, 0};
static s32 vkc_a2_da_rtbl93nit[10] = {0, 7, 3, 2, 2, 1, 2, 3, 3, 0};
static s32 vkc_a2_da_rtbl98nit[10] = {0, 7, 3, 2, 2, 1, 3, 3, 1, 0};
static s32 vkc_a2_da_rtbl105nit[10] = {0, 7, 4, 3, 2, 1, 3, 3, 2, 0};
static s32 vkc_a2_da_rtbl111nit[10] = {0, 7, 4, 2, 2, 1, 3, 4, 2, 0};
static s32 vkc_a2_da_rtbl119nit[10] = {0, 7, 4, 3, 2, 2, 3, 3, 1, 0};
static s32 vkc_a2_da_rtbl126nit[10] = {0, 7, 3, 2, 2, 2, 3, 4, 1, 0};
static s32 vkc_a2_da_rtbl134nit[10] = {0, 7, 3, 2, 3, 2, 3, 4, 1, 0};
static s32 vkc_a2_da_rtbl143nit[10] = {0, 6, 3, 2, 2, 2, 4, 4, 3, 0};
static s32 vkc_a2_da_rtbl152nit[10] = {0, 6, 3, 2, 2, 1, 3, 4, 1, 0};
static s32 vkc_a2_da_rtbl162nit[10] = {0, 5, 3, 1, 2, 1, 3, 4, 2, 0};
static s32 vkc_a2_da_rtbl172nit[10] = {0, 5, 2, 2, 2, 1, 3, 3, 3, 0};
static s32 vkc_a2_da_rtbl183nit[10] = {0, 5, 2, 1, 2, 2, 3, 4, 2, 0};
static s32 vkc_a2_da_rtbl195nit[10] = {0, 5, 2, 1, 1, 1, 2, 4, 1, 0};
static s32 vkc_a2_da_rtbl207nit[10] = {0, 4, 2, 1, 1, 1, 2, 3, 1, 0};
static s32 vkc_a2_da_rtbl220nit[10] = {0, 4, 1, 0, 1, 1, 1, 3, 1, 0};
static s32 vkc_a2_da_rtbl234nit[10] = {0, 3, 1, 0, 1, 0, 1, 2, 1, 0};
static s32 vkc_a2_da_rtbl249nit[10] = {0, 3, 1, 1, 0, 0, 2, 2, 1, 0};
static s32 vkc_a2_da_rtbl265nit[10] = {0, 2, 0, 0, 0, 0, 1, 2, 0, 0};
static s32 vkc_a2_da_rtbl282nit[10] = {0, 2, 0, 0, 0, 0, 0, 1, 0, 0};
static s32 vkc_a2_da_rtbl300nit[10] = {0, 2, 0, 0, 0, 0, 0, 1, 0, 0};
static s32 vkc_a2_da_rtbl316nit[10] = {0, 1, 0, 0, 0, 0, 0, 1, 0, 0};
static s32 vkc_a2_da_rtbl333nit[10] = {0, 1, 0, 0, 0, 0, 0, 0, 0, 0};
static s32 vkc_a2_da_rtbl350nit[10] = {0, 1, 0, 0, 0, 0, 0, 0, 0, 0};
static s32 vkc_a2_da_rtbl357nit[10] = {0, 1, 0, 0, 0, 0, 0, 0, 0, 0};
static s32 vkc_a2_da_rtbl365nit[10] = {0, 1, 0, 0, 0, 0, 0, 0, 0, 0};
static s32 vkc_a2_da_rtbl372nit[10] = {0, 1, 0, 0, 0, 0, 0, 0, 0, 0};
static s32 vkc_a2_da_rtbl380nit[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static s32 vkc_a2_da_rtbl387nit[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static s32 vkc_a2_da_rtbl395nit[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static s32 vkc_a2_da_rtbl403nit[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static s32 vkc_a2_da_rtbl412nit[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static s32 vkc_a2_da_rtbl420nit[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static s32 vkc_a2_da_ctbl2nit[30] = {0, 0, 0, 0, 0, 0, -7, -1, -13, -7, 0, -11, -8, 0, -12, -17, -1, -12, -10, 0, -6, -4, 0, -4, -2, 0, -2, -10, 0, -10};
static s32 vkc_a2_da_ctbl3nit[30] = {0, 0, 0, 0, 0, 0, -6, 0, -16, -8, -1, -12, -12, -1, -12, -14, 0, -10, -10, 0, -7, -4, 0, -4, -2, 0, -2, -7, 0, -8};
static s32 vkc_a2_da_ctbl4nit[30] = {0, 0, 0, 0, 0, 0, -15, -2, -20, -11, 2, -12, -10, 0, -10, -15, -1, -11, -9, -1, -7, -2, 0, -3, -2, 0, -2, -6, 0, -7};
static s32 vkc_a2_da_ctbl5nit[30] = {0, 0, 0, 0, 0, 0, -12, 0, -18, -12, 1, -12, -9, 1, -10, -13, 0, -10, -9, -1, -8, -3, 0, -3, 0, 0, -1, -6, 0, -7};
static s32 vkc_a2_da_ctbl6nit[30] = {0, 0, 0, 0, 0, 0, -13, 0, -19, -16, -2, -16, -9, 0, -10, -10, 1, -9, -9, 0, -8, -2, 0, -2, -1, 0, -2, -4, 0, -5};
static s32 vkc_a2_da_ctbl7nit[30] = {0, 0, 0, 0, 0, 0, -14, 3, -19, -12, -1, -15, -11, -1, -12, -10, 0, -9, -7, 0, -7, -2, 0, -2, 0, 0, -1, -4, 0, -5};
static s32 vkc_a2_da_ctbl8nit[30] = {0, 0, 0, 0, 0, 0, -15, -2, -23, -11, 1, -12, -8, 1, -11, -11, -1, -10, -6, 1, -6, -3, -1, -3, 0, 0, -1, -3, 0, -4};
static s32 vkc_a2_da_ctbl9nit[30] = {0, 0, 0, 0, 0, 0, -17, -3, -26, -11, 1, -12, -7, 0, -11, -10, -1, -9, -6, 1, -6, -3, -1, -3, 1, 0, 0, -3, 0, -4};
static s32 vkc_a2_da_ctbl10nit[30] = {0, 0, 0, 0, 0, 0, -18, -3, -25, -9, 2, -12, -8, -1, -12, -10, 0, -9, -5, 0, -5, -2, 0, -2, 0, 0, -1, -2, 0, -3};
static s32 vkc_a2_da_ctbl11nit[30] = {0, 0, 0, 0, 0, 0, -14, 1, -22, -13, -3, -17, -4, 2, -9, -12, -1, -10, -5, 1, -4, -1, 0, -2, -1, 0, -1, -1, 0, -2};
static s32 vkc_a2_da_ctbl12nit[30] = {0, 0, 0, 0, 0, 0, -15, 1, -23, -13, -3, -16, -5, 2, -10, -8, 0, -8, -5, 0, -4, -2, 0, -3, 0, 0, 0, -1, 0, -2};
static s32 vkc_a2_da_ctbl13nit[30] = {0, 0, 0, 0, 0, 0, -20, -2, -27, -11, -1, -15, -4, 2, -9, -10, -2, -10, -4, 0, -4, -1, 0, -2, -1, 0, -1, 0, 0, -1};
static s32 vkc_a2_da_ctbl14nit[30] = {0, 0, 0, 0, 0, 0, -16, 0, -24, -13, -4, -17, -7, 1, -11, -6, 2, -6, -5, -1, -5, -1, 0, -2, -1, 0, -1, 1, 0, 0};
static s32 vkc_a2_da_ctbl15nit[30] = {0, 0, 0, 0, 0, 0, -16, 0, -25, -11, 0, -15, -6, 0, -11, -8, 1, -7, -4, -1, -4, -1, 0, -2, -1, 0, -1, 1, 0, 0};
static s32 vkc_a2_da_ctbl16nit[30] = {0, 0, 0, 0, 0, 0, -18, -1, -26, -10, -1, -15, -6, 1, -10, -7, 0, -6, -3, 0, -3, -1, 0, -2, -1, 0, -1, 1, 0, 0};
static s32 vkc_a2_da_ctbl17nit[30] = {0, 0, 0, 0, 0, 0, -18, 0, -26, -8, 2, -13, -7, -1, -11, -6, 0, -6, -4, 0, -3, 0, 0, -1, -1, 0, -1, 1, 0, 0};
static s32 vkc_a2_da_ctbl19nit[30] = {0, 0, 0, 0, 0, 0, -16, 0, -25, -10, 0, -15, -6, 0, -9, -7, 0, -6, -4, 0, -3, -1, 0, -2, 0, 0, 0, 1, 0, 0};
static s32 vkc_a2_da_ctbl20nit[30] = {0, 0, 0, 0, 0, 0, -11, 4, -20, -12, -3, -18, -3, 3, -6, -7, -1, -6, -4, 0, -3, -1, 0, -2, 0, 0, 0, 1, 0, 0};
static s32 vkc_a2_da_ctbl21nit[30] = {0, 0, 0, 0, 0, 0, -16, 0, -25, -10, 0, -16, -6, 0, -9, -6, 0, -5, -2, 0, -1, -1, 0, -2, 0, 0, 0, 1, 0, 0};
static s32 vkc_a2_da_ctbl22nit[30] = {0, 0, 0, 0, 0, 0, -18, -1, -26, -10, 0, -16, -5, 0, -8, -7, -1, -6, -2, 0, -1, -1, 0, -2, 0, 0, 0, 1, 0, 0};
static s32 vkc_a2_da_ctbl24nit[30] = {0, 0, 0, 0, 0, 0, -15, 0, -25, -12, -1, -17, -3, 2, -5, -7, -1, -6, -3, -1, -2, 0, 0, -1, 0, 0, 0, 1, 0, 0};
static s32 vkc_a2_da_ctbl25nit[30] = {0, 0, 0, 0, 0, 0, -16, -1, -26, -11, -1, -16, -6, -1, -9, -5, 1, -4, -3, -1, -2, 0, 0, -1, 0, 0, 0, 1, 0, 0};
static s32 vkc_a2_da_ctbl27nit[30] = {0, 0, 0, 0, 0, 0, -17, -2, -26, -10, -2, -15, -4, 2, -5, -5, 0, -4, -3, -1, -2, 0, 0, -1, -1, 0, -1, 2, 0, 1};
static s32 vkc_a2_da_ctbl29nit[30] = {0, 0, 0, 0, 0, 0, -18, -1, -27, -5, 3, -9, -4, 1, -5, -7, -2, -6, -2, 0, -1, 0, 0, -1, -1, 0, -1, 2, 0, 1};
static s32 vkc_a2_da_ctbl30nit[30] = {0, 0, 0, 0, 0, 0, -20, -3, -29, -5, 2, -9, -7, -2, -8, -4, 1, -3, -2, 0, -1, -1, 0, -2, 0, 0, 0, 2, 0, 1};
static s32 vkc_a2_da_ctbl32nit[30] = {0, 0, 0, 0, 0, 0, -13, 2, -24, -9, -2, -13, -4, 0, -6, -5, -1, -4, -2, 1, -1, 1, 0, 0, -1, 0, -1, 2, 0, 1};
static s32 vkc_a2_da_ctbl34nit[30] = {0, 0, 0, 0, 0, 0, -21, -4, -31, -4, 2, -8, -3, 0, -5, -6, -1, -5, -2, 0, -1, 0, 0, -1, 0, 0, 0, 2, 0, 1};
static s32 vkc_a2_da_ctbl37nit[30] = {0, 0, 0, 0, 0, 0, -16, 0, -26, -9, -3, -13, -4, 0, -5, -2, 1, -1, -2, 0, -1, 0, 0, -1, 0, 0, 0, 2, 0, 1};
static s32 vkc_a2_da_ctbl39nit[30] = {0, 0, 0, 0, 0, 0, -16, 0, -25, -3, 2, -6, -5, -1, -6, -2, 0, -1, -2, 0, -1, 0, 0, -1, 0, 0, 0, 2, 0, 1};
static s32 vkc_a2_da_ctbl41nit[30] = {0, 0, 0, 0, 0, 0, -17, -1, -27, -4, 1, -7, -4, -1, -5, -5, -1, -4, -1, 0, 0, 1, 0, -1, 0, 0, 0, 2, 0, 1};
static s32 vkc_a2_da_ctbl44nit[30] = {0, 0, 0, 0, 0, 0, -17, -2, -27, -2, 1, -4, -5, -1, -6, -2, 1, -1, -1, 0, 0, 1, 0, -1, 0, 0, 0, 2, 0, 1};
static s32 vkc_a2_da_ctbl47nit[30] = {0, 0, 0, 0, 0, 0, -12, 4, -21, -2, 0, -4, -5, -2, -6, -2, 1, -1, -2, -1, -2, 1, 0, 0, 0, 0, 0, 2, 0, 1};
static s32 vkc_a2_da_ctbl50nit[30] = {0, 0, 0, 0, 0, 0, -13, 3, -22, -7, -4, -9, -1, 1, -2, -2, 0, -1, -2, -1, -2, 0, 0, 0, 1, 0, 1, 2, 0, 1};
static s32 vkc_a2_da_ctbl53nit[30] = {0, 0, 0, 0, 0, 0, -14, 2, -23, -2, 1, -3, -2, 1, -3, -3, -2, -2, 0, 0, 0, 1, 0, 1, 0, 0, 0, 2, 0, 1};
static s32 vkc_a2_da_ctbl56nit[30] = {0, 0, 0, 0, 0, 0, -15, 2, -24, -2, 0, -3, -2, 0, -3, -3, -2, -2, 0, 0, 0, 1, 0, 0, 0, 0, 0, 2, 0, 1};
static s32 vkc_a2_da_ctbl60nit[30] = {0, 0, 0, 0, 0, 0, -22, -6, -30, -2, 1, -3, 1, 1, -2, -1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 2, 0, 1};
static s32 vkc_a2_da_ctbl64nit[30] = {0, 0, 0, 0, 0, 0, -17, -1, -25, -2, 0, -3, 1, 1, -1, -1, 1, 0, 0, 0, 0, 0, -1, 0, 1, 0, 1, 2, 0, 1};
static s32 vkc_a2_da_ctbl68nit[30] = {0, 0, 0, 0, 0, 0, -18, -1, -25, -3, -1, -4, 1, 0, -2, -1, 1, 1, 1, 0, 1, 0, 0, -1, 1, 0, 1, 2, 1, 1};
static s32 vkc_a2_da_ctbl72nit[30] = {0, 0, 0, 0, 0, 0, -21, -4, -28, 5, 4, 3, -2, -1, -3, -2, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 2, 1, 2};
static s32 vkc_a2_da_ctbl77nit[30] = {0, 0, 0, 0, 0, 0, -14, 1, -21, -4, -3, -5, 2, 2, 0, -1, 1, 1, 0, -1, -1, -1, -1, -1, 1, 1, 1, 2, 0, 1};
static s32 vkc_a2_da_ctbl82nit[30] = {0, 0, 0, 0, 0, 0, -15, -1, -22, 1, 0, 0, -2, -1, -3, 0, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 2, 0, 2};
static s32 vkc_a2_da_ctbl87nit[30] = {0, 0, 0, 0, 0, 0, -10, 3, -16, -2, -1, -3, -3, -3, -4, 0, 1, 1, 0, -1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1};
static s32 vkc_a2_da_ctbl93nit[30] = {0, 0, 0, 0, 0, 0, -11, 2, -16, -2, -2, -3, 2, 1, -1, -2, 0, -1, 1, 1, 2, 2, 0, 1, -1, 0, -1, 2, 0, 1};
static s32 vkc_a2_da_ctbl98nit[30] = {0, 0, 0, 0, 0, 0, -11, 0, -17, 1, 2, 0, -1, -1, -2, -2, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1};
static s32 vkc_a2_da_ctbl105nit[30] = {0, 0, 0, 0, 0, 0, -14, -2, -18, -2, -1, -3, -2, -2, -3, 1, 1, 1, 0, 0, 1, -1, 0, -1, 0, 0, 1, 2, 1, 1};
static s32 vkc_a2_da_ctbl111nit[30] = {0, 0, 0, 0, 0, 0, -14, -3, -18, -1, -1, -2, 1, 0, 0, 1, 1, 1, -1, 0, 0, 0, -1, 0, 1, 0, 1, 2, 0, 1};
static s32 vkc_a2_da_ctbl119nit[30] = {0, 0, 0, 0, 0, 0, -10, 1, -13, -3, -2, -5, 1, 2, 1, 0, 0, 0, -1, -1, 0, -1, 0, -1, 0, 0, 1, 3, 1, 1};
static s32 vkc_a2_da_ctbl126nit[30] = {0, 0, 0, 0, 0, 0, -10, 0, -14, -1, 0, -1, 0, 0, -1, -2, -1, -1, 0, 1, 1, 0, 0, 0, 0, 0, 1, 2, 1, 1};
static s32 vkc_a2_da_ctbl134nit[30] = {0, 0, 0, 0, 0, 0, -10, -1, -13, 3, 4, 2, 0, -1, -2, -1, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, 1, 3, 1, 1};
static s32 vkc_a2_da_ctbl143nit[30] = {0, 0, 0, 0, 0, 0, -8, 2, -10, -4, -2, -3, 0, -1, -2, 1, 1, 2, -1, -1, -1, 1, 1, 0, 0, 0, 1, 2, 1, 1};
static s32 vkc_a2_da_ctbl152nit[30] = {0, 0, 0, 0, 0, 0, -8, 1, -10, 0, 1, 0, -3, -2, -3, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 1, 1, 0, 0};
static s32 vkc_a2_da_ctbl162nit[30] = {0, 0, 0, 0, 0, 0, -9, -1, -10, 0, 0, 0, -1, -1, -1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0};
static s32 vkc_a2_da_ctbl172nit[30] = {0, 0, 0, 0, 0, 0, -6, 3, -6, -2, -1, -2, -1, -2, -2, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, -1, 0, 2, 1, 1};
static s32 vkc_a2_da_ctbl183nit[30] = {0, 0, 0, 0, 0, 0, -6, 1, -7, 0, 2, 1, 1, 0, 0, 0, 0, 0, -2, -1, -2, 1, 0, 1, 0, 0, 0, 2, 1, 1};
static s32 vkc_a2_da_ctbl195nit[30] = {0, 0, 0, 0, 0, 0, -6, 0, -7, -3, -1, -2, 1, 1, 0, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0};
static s32 vkc_a2_da_ctbl207nit[30] = {0, 0, 0, 0, 0, 0, -7, -1, -8, -4, -2, -3, 2, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0};
static s32 vkc_a2_da_ctbl220nit[30] = {0, 0, 0, 0, 0, 0, -6, 0, -6, 3, 2, 2, 1, 0, 0, -1, -1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 1, 1, 1, 0};
static s32 vkc_a2_da_ctbl234nit[30] = {0, 0, 0, 0, 0, 0, -6, -1, -6, 3, 2, 2, -1, -3, -2, 0, 1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 2, 1, 1, 0};
static s32 vkc_a2_da_ctbl249nit[30] = {0, 0, 0, 0, 0, 0, -7, -2, -6, -1, -2, -1, 1, 0, 0, 1, 1, 2, -1, -1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0};
static s32 vkc_a2_da_ctbl265nit[30] = {0, 0, 0, 0, 0, 0, 0, 3, 3, 2, 1, 1, 0, -1, 0, 0, 1, 1, -1, 0, 0, -1, 0, 0, 1, 0, 1, 2, 0, 0};
static s32 vkc_a2_da_ctbl282nit[30] = {0, 0, 0, 0, 0, 0, -1, 1, 2, 2, 1, 1, 1, 1, 1, 0, -1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0};
static s32 vkc_a2_da_ctbl300nit[30] = {0, 0, 0, 0, 0, 0, -2, 0, 2, 5, 3, 5, 1, 0, 0, -1, -1, 1, -1, -1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0};
static s32 vkc_a2_da_ctbl316nit[30] = {0, 0, 0, 0, 0, 0, 2, 2, 5, -1, -1, -1, 1, 0, 1, 1, 0, 1, -1, 1, 0, -1, -1, 0, 1, 0, 0, 1, 1, 0};
static s32 vkc_a2_da_ctbl333nit[30] = {0, 0, 0, 0, 0, 0, 1, 2, 5, 2, 3, 2, 0, -1, 0, 1, 0, 1, -2, -1, -1, 1, 0, 1, 0, 0, -1, 1, 1, 1};
static s32 vkc_a2_da_ctbl350nit[30] = {0, 0, 0, 0, 0, 0, -7, -6, -5, 2, 2, 2, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 1, 0, 0, 1, 1, 1};
static s32 vkc_a2_da_ctbl357nit[30] = {0, 0, 0, 0, 0, 0, -2, -1, 0, -3, -2, -3, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, -1, 1, 0, 1};
static s32 vkc_a2_da_ctbl365nit[30] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static s32 vkc_a2_da_ctbl372nit[30] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static s32 vkc_a2_da_ctbl380nit[30] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static s32 vkc_a2_da_ctbl387nit[30] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static s32 vkc_a2_da_ctbl395nit[30] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static s32 vkc_a2_da_ctbl403nit[30] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static s32 vkc_a2_da_ctbl412nit[30] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static s32 vkc_a2_da_ctbl420nit[30] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static struct dimming_lut vkc_a2_da_dimming_lut[] = {
	DIM_LUT_V0_INIT(2, 118, GAMMA_2_15, vkc_a2_da_rtbl2nit, vkc_a2_da_ctbl2nit),
	DIM_LUT_V0_INIT(3, 118, GAMMA_2_15, vkc_a2_da_rtbl3nit, vkc_a2_da_ctbl3nit),
	DIM_LUT_V0_INIT(4, 118, GAMMA_2_15, vkc_a2_da_rtbl4nit, vkc_a2_da_ctbl4nit),
	DIM_LUT_V0_INIT(5, 118, GAMMA_2_15, vkc_a2_da_rtbl5nit, vkc_a2_da_ctbl5nit),
	DIM_LUT_V0_INIT(6, 118, GAMMA_2_15, vkc_a2_da_rtbl6nit, vkc_a2_da_ctbl6nit),
	DIM_LUT_V0_INIT(7, 118, GAMMA_2_15, vkc_a2_da_rtbl7nit, vkc_a2_da_ctbl7nit),
	DIM_LUT_V0_INIT(8, 118, GAMMA_2_15, vkc_a2_da_rtbl8nit, vkc_a2_da_ctbl8nit),
	DIM_LUT_V0_INIT(9, 118, GAMMA_2_15, vkc_a2_da_rtbl9nit, vkc_a2_da_ctbl9nit),
	DIM_LUT_V0_INIT(10, 118, GAMMA_2_15, vkc_a2_da_rtbl10nit, vkc_a2_da_ctbl10nit),
	DIM_LUT_V0_INIT(11, 118, GAMMA_2_15, vkc_a2_da_rtbl11nit, vkc_a2_da_ctbl11nit),
	DIM_LUT_V0_INIT(12, 118, GAMMA_2_15, vkc_a2_da_rtbl12nit, vkc_a2_da_ctbl12nit),
	DIM_LUT_V0_INIT(13, 118, GAMMA_2_15, vkc_a2_da_rtbl13nit, vkc_a2_da_ctbl13nit),
	DIM_LUT_V0_INIT(14, 118, GAMMA_2_15, vkc_a2_da_rtbl14nit, vkc_a2_da_ctbl14nit),
	DIM_LUT_V0_INIT(15, 118, GAMMA_2_15, vkc_a2_da_rtbl15nit, vkc_a2_da_ctbl15nit),
	DIM_LUT_V0_INIT(16, 118, GAMMA_2_15, vkc_a2_da_rtbl16nit, vkc_a2_da_ctbl16nit),
	DIM_LUT_V0_INIT(17, 118, GAMMA_2_15, vkc_a2_da_rtbl17nit, vkc_a2_da_ctbl17nit),
	DIM_LUT_V0_INIT(19, 118, GAMMA_2_15, vkc_a2_da_rtbl19nit, vkc_a2_da_ctbl19nit),
	DIM_LUT_V0_INIT(20, 118, GAMMA_2_15, vkc_a2_da_rtbl20nit, vkc_a2_da_ctbl20nit),
	DIM_LUT_V0_INIT(21, 118, GAMMA_2_15, vkc_a2_da_rtbl21nit, vkc_a2_da_ctbl21nit),
	DIM_LUT_V0_INIT(22, 118, GAMMA_2_15, vkc_a2_da_rtbl22nit, vkc_a2_da_ctbl22nit),
	DIM_LUT_V0_INIT(24, 118, GAMMA_2_15, vkc_a2_da_rtbl24nit, vkc_a2_da_ctbl24nit),
	DIM_LUT_V0_INIT(25, 118, GAMMA_2_15, vkc_a2_da_rtbl25nit, vkc_a2_da_ctbl25nit),
	DIM_LUT_V0_INIT(27, 118, GAMMA_2_15, vkc_a2_da_rtbl27nit, vkc_a2_da_ctbl27nit),
	DIM_LUT_V0_INIT(29, 118, GAMMA_2_15, vkc_a2_da_rtbl29nit, vkc_a2_da_ctbl29nit),
	DIM_LUT_V0_INIT(30, 118, GAMMA_2_15, vkc_a2_da_rtbl30nit, vkc_a2_da_ctbl30nit),
	DIM_LUT_V0_INIT(32, 118, GAMMA_2_15, vkc_a2_da_rtbl32nit, vkc_a2_da_ctbl32nit),
	DIM_LUT_V0_INIT(34, 118, GAMMA_2_15, vkc_a2_da_rtbl34nit, vkc_a2_da_ctbl34nit),
	DIM_LUT_V0_INIT(37, 118, GAMMA_2_15, vkc_a2_da_rtbl37nit, vkc_a2_da_ctbl37nit),
	DIM_LUT_V0_INIT(39, 118, GAMMA_2_15, vkc_a2_da_rtbl39nit, vkc_a2_da_ctbl39nit),
	DIM_LUT_V0_INIT(41, 118, GAMMA_2_15, vkc_a2_da_rtbl41nit, vkc_a2_da_ctbl41nit),
	DIM_LUT_V0_INIT(44, 118, GAMMA_2_15, vkc_a2_da_rtbl44nit, vkc_a2_da_ctbl44nit),
	DIM_LUT_V0_INIT(47, 118, GAMMA_2_15, vkc_a2_da_rtbl47nit, vkc_a2_da_ctbl47nit),
	DIM_LUT_V0_INIT(50, 118, GAMMA_2_15, vkc_a2_da_rtbl50nit, vkc_a2_da_ctbl50nit),
	DIM_LUT_V0_INIT(53, 118, GAMMA_2_15, vkc_a2_da_rtbl53nit, vkc_a2_da_ctbl53nit),
	DIM_LUT_V0_INIT(56, 118, GAMMA_2_15, vkc_a2_da_rtbl56nit, vkc_a2_da_ctbl56nit),
	DIM_LUT_V0_INIT(60, 118, GAMMA_2_15, vkc_a2_da_rtbl60nit, vkc_a2_da_ctbl60nit),
	DIM_LUT_V0_INIT(64, 118, GAMMA_2_15, vkc_a2_da_rtbl64nit, vkc_a2_da_ctbl64nit),
	DIM_LUT_V0_INIT(68, 119, GAMMA_2_15, vkc_a2_da_rtbl68nit, vkc_a2_da_ctbl68nit),
	DIM_LUT_V0_INIT(72, 125, GAMMA_2_15, vkc_a2_da_rtbl72nit, vkc_a2_da_ctbl72nit),
	DIM_LUT_V0_INIT(77, 133, GAMMA_2_15, vkc_a2_da_rtbl77nit, vkc_a2_da_ctbl77nit),
	DIM_LUT_V0_INIT(82, 140, GAMMA_2_15, vkc_a2_da_rtbl82nit, vkc_a2_da_ctbl82nit),
	DIM_LUT_V0_INIT(87, 151, GAMMA_2_15, vkc_a2_da_rtbl87nit, vkc_a2_da_ctbl87nit),
	DIM_LUT_V0_INIT(93, 157, GAMMA_2_15, vkc_a2_da_rtbl93nit, vkc_a2_da_ctbl93nit),
	DIM_LUT_V0_INIT(98, 166, GAMMA_2_15, vkc_a2_da_rtbl98nit, vkc_a2_da_ctbl98nit),
	DIM_LUT_V0_INIT(105, 179, GAMMA_2_15, vkc_a2_da_rtbl105nit, vkc_a2_da_ctbl105nit),
	DIM_LUT_V0_INIT(111, 186, GAMMA_2_15, vkc_a2_da_rtbl111nit, vkc_a2_da_ctbl111nit),
	DIM_LUT_V0_INIT(119, 200, GAMMA_2_15, vkc_a2_da_rtbl119nit, vkc_a2_da_ctbl119nit),
	DIM_LUT_V0_INIT(126, 210, GAMMA_2_15, vkc_a2_da_rtbl126nit, vkc_a2_da_ctbl126nit),
	DIM_LUT_V0_INIT(134, 222, GAMMA_2_15, vkc_a2_da_rtbl134nit, vkc_a2_da_ctbl134nit),
	DIM_LUT_V0_INIT(143, 233, GAMMA_2_15, vkc_a2_da_rtbl143nit, vkc_a2_da_ctbl143nit),
	DIM_LUT_V0_INIT(152, 249, GAMMA_2_15, vkc_a2_da_rtbl152nit, vkc_a2_da_ctbl152nit),
	DIM_LUT_V0_INIT(162, 260, GAMMA_2_15, vkc_a2_da_rtbl162nit, vkc_a2_da_ctbl162nit),
	DIM_LUT_V0_INIT(172, 274, GAMMA_2_15, vkc_a2_da_rtbl172nit, vkc_a2_da_ctbl172nit),
	DIM_LUT_V0_INIT(183, 292, GAMMA_2_15, vkc_a2_da_rtbl183nit, vkc_a2_da_ctbl183nit),
	DIM_LUT_V0_INIT(195, 292, GAMMA_2_15, vkc_a2_da_rtbl195nit, vkc_a2_da_ctbl195nit),
	DIM_LUT_V0_INIT(207, 292, GAMMA_2_15, vkc_a2_da_rtbl207nit, vkc_a2_da_ctbl207nit),
	DIM_LUT_V0_INIT(220, 292, GAMMA_2_15, vkc_a2_da_rtbl220nit, vkc_a2_da_ctbl220nit),
	DIM_LUT_V0_INIT(234, 292, GAMMA_2_15, vkc_a2_da_rtbl234nit, vkc_a2_da_ctbl234nit),
	DIM_LUT_V0_INIT(249, 295, GAMMA_2_15, vkc_a2_da_rtbl249nit, vkc_a2_da_ctbl249nit),
	DIM_LUT_V0_INIT(265, 307, GAMMA_2_15, vkc_a2_da_rtbl265nit, vkc_a2_da_ctbl265nit),
	DIM_LUT_V0_INIT(282, 319, GAMMA_2_15, vkc_a2_da_rtbl282nit, vkc_a2_da_ctbl282nit),
	DIM_LUT_V0_INIT(300, 338, GAMMA_2_15, vkc_a2_da_rtbl300nit, vkc_a2_da_ctbl300nit),
	DIM_LUT_V0_INIT(316, 354, GAMMA_2_15, vkc_a2_da_rtbl316nit, vkc_a2_da_ctbl316nit),
	DIM_LUT_V0_INIT(333, 374, GAMMA_2_15, vkc_a2_da_rtbl333nit, vkc_a2_da_ctbl333nit),
	DIM_LUT_V0_INIT(350, 388, GAMMA_2_15, vkc_a2_da_rtbl350nit, vkc_a2_da_ctbl350nit),
	DIM_LUT_V0_INIT(357, 395, GAMMA_2_15, vkc_a2_da_rtbl357nit, vkc_a2_da_ctbl357nit),
	DIM_LUT_V0_INIT(365, 406, GAMMA_2_15, vkc_a2_da_rtbl365nit, vkc_a2_da_ctbl365nit),
	DIM_LUT_V0_INIT(372, 406, GAMMA_2_15, vkc_a2_da_rtbl372nit, vkc_a2_da_ctbl372nit),
	DIM_LUT_V0_INIT(380, 406, GAMMA_2_15, vkc_a2_da_rtbl380nit, vkc_a2_da_ctbl380nit),
	DIM_LUT_V0_INIT(387, 406, GAMMA_2_15, vkc_a2_da_rtbl387nit, vkc_a2_da_ctbl387nit),
	DIM_LUT_V0_INIT(395, 406, GAMMA_2_15, vkc_a2_da_rtbl395nit, vkc_a2_da_ctbl395nit),
	DIM_LUT_V0_INIT(403, 409, GAMMA_2_15, vkc_a2_da_rtbl403nit, vkc_a2_da_ctbl403nit),
	DIM_LUT_V0_INIT(412, 416, GAMMA_2_15, vkc_a2_da_rtbl412nit, vkc_a2_da_ctbl412nit),
	DIM_LUT_V0_INIT(420, 420, GAMMA_2_20, vkc_a2_da_rtbl420nit, vkc_a2_da_ctbl420nit),
};

#if PANEL_BACKLIGHT_PAC_STEPS == 512
static unsigned int vkc_a2_da_brt_to_step_tbl[S6E3HF3_TOTAL_PAC_STEPS] = {
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
static unsigned int vkc_a2_da_brt_to_step_tbl[S6E3HF3_TOTAL_PAC_STEPS] = {
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

static unsigned int vkc_a2_da_brt_to_brt_tbl[S6E3HF3_TOTAL_PAC_STEPS][2] = {
};

static unsigned int vkc_a2_da_brt_tbl[S6E3HF3_TOTAL_NR_LUMINANCE] = {
	BRT(0), BRT(6), BRT(13), BRT(19), BRT(27), BRT(34), BRT(36), BRT(38), BRT(40), BRT(42),
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

static unsigned int vkc_a2_da_lum_tbl[S6E3HF3_TOTAL_NR_LUMINANCE] = {
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

struct brightness_table s6e3hf3_vkc_a2_da_panel_brightness_table = {
	.brt = vkc_a2_da_brt_tbl,
	.sz_brt = ARRAY_SIZE(vkc_a2_da_brt_tbl),
	.lum = vkc_a2_da_lum_tbl,
	.sz_lum = ARRAY_SIZE(vkc_a2_da_lum_tbl),
#if PANEL_BACKLIGHT_PAC_STEPS == 512 || PANEL_BACKLIGHT_PAC_STEPS == 256
	.brt_to_step = vkc_a2_da_brt_to_step_tbl,
	.sz_brt_to_step = ARRAY_SIZE(vkc_a2_da_brt_to_step_tbl),
#else
	.brt_to_step = vkc_a2_da_brt_tbl,
	.sz_brt_to_step = ARRAY_SIZE(vkc_a2_da_brt_tbl),
#endif
	.brt_to_brt = vkc_a2_da_brt_to_brt_tbl,
	.sz_brt_to_brt = ARRAY_SIZE(vkc_a2_da_brt_to_brt_tbl),
};

static struct panel_dimming_info s6e3hf3_vkc_a2_da_panel_dimming_info = {
	.name = "s6e3hf3_vkc_a2_da",
	.dim_init_info = {
		.name = "s6e3hf3_vkc_a2_da",
		.nr_tp = S6E3HF3_NR_TP,
		.tp = s6e3hf3_tp,
		.nr_luminance = S6E3HF3_NR_LUMINANCE,
		.vregout = 107374182LL,	/* 6.4 * 2^24*/
		.bitshift = 24,
		.vt_voltage = {
			0, 12, 24, 36, 48, 60, 72, 84, 96, 108,
			138, 148, 158, 168, 178, 186,
		},
		.target_luminance = S6E3HF3_TARGET_LUMINANCE,
		.target_gamma = 220,
		.dim_lut = vkc_a2_da_dimming_lut,
	},
	.target_luminance = S6E3HF3_TARGET_LUMINANCE,
	.nr_luminance = S6E3HF3_NR_LUMINANCE,
	.hbm_target_luminance = S6E3HF3_TARGET_HBM_LUMINANCE,
	.nr_hbm_luminance = S6E3HF3_NR_HBM_LUMINANCE,
	.extend_hbm_target_luminance = -1,
	.nr_extend_hbm_luminance = 0,
	.brt_tbl = &s6e3hf3_vkc_a2_da_panel_brightness_table,
};
#endif /* __S6E3HF3_VKC_A2_DA_PANEL_DIMMING_H___ */
