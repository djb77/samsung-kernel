/* linux/drivers/video/exynos_decon/panel/s6e3hf4_wqhd_aid_dimming.h
 *
 * Header file for Samsung AID Dimming Driver.
 *
 * Copyright (c) 2013 Samsung Electronics
 * Minwoo Kim <minwoo7945.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __S6E3HF4_WQHD_AID_DIMMING_H__
#define __S6E3HF4_WQHD_AID_DIMMING_H__

static signed char rtbl2nit[10] = {0, 9, 13, 20, 18, 18, 13, 9, 7, 3};
static signed char rtbl3nit[10] = {0, 12, 15, 24, 22, 18, 13, 8, 6, 1};
static signed char rtbl4nit[10] = {0, 13, 16, 20, 19, 16, 12, 7, 4, 1};
static signed char rtbl5nit[10] = {0, 11, 15, 18, 16, 14, 10, 5, 3, 1};
static signed char rtbl6nit[10] = {0, 8, 14, 18, 16, 14, 11, 6, 4, 1};
static signed char rtbl7nit[10] = {0, 9, 16, 17, 15, 13, 9, 5, 4, 1};
static signed char rtbl8nit[10] = {0, 9, 15, 16, 14, 12, 9, 5, 4, 1};
static signed char rtbl9nit[10] = {0, 9, 16, 15, 13, 11, 8, 4, 3, 1};
static signed char rtbl10nit[10] = {0, 10, 17, 15, 13, 11, 7, 4, 3, 1};
static signed char rtbl11nit[10] = {0, 11, 17, 14, 12, 10, 6, 3, 3, 1};
static signed char rtbl12nit[10] = {0, 11, 17, 14, 12, 10, 6, 3, 3, 1};
static signed char rtbl13nit[10] = {0, 11, 16, 13, 11, 9, 6, 3, 3, 0};
static signed char rtbl14nit[10] = {0, 15, 16, 13, 11, 9, 6, 3, 3, 0};
static signed char rtbl15nit[10] = {0, 14, 15, 11, 9, 7, 4, 1, 2, 0};
static signed char rtbl16nit[10] = {0, 14, 15, 11, 9, 7, 4, 2, 3, 0};
static signed char rtbl17nit[10] = {0, 13, 14, 11, 9, 7, 4, 1, 3, 0};
static signed char rtbl19nit[10] = {0, 12, 13, 9, 8, 6, 3, 1, 2, 0};
static signed char rtbl20nit[10] = {0, 11, 12, 8, 7, 5, 2, 1, 1, 0};
static signed char rtbl21nit[10] = {0, 11, 12, 8, 7, 5, 2, 0, 1, 0};
static signed char rtbl22nit[10] = {0, 11, 12, 8, 7, 5, 2, 1, 0, 0};
static signed char rtbl24nit[10] = {0, 12, 12, 8, 7, 5, 2, 1, 0, 0};
static signed char rtbl25nit[10] = {0, 12, 12, 8, 7, 5, 2, 1, 3, 0};
static signed char rtbl27nit[10] = {0, 11, 11, 7, 5, 4, 1, 0, 0, 0};
static signed char rtbl29nit[10] = {0, 12, 11, 7, 6, 4, 2, 1, 0, 0};
static signed char rtbl30nit[10] = {0, 9, 10, 6, 4, 3, 1, -1, -1, 0};
static signed char rtbl32nit[10] = {0, 10, 10, 6, 5, 3, 1, 1, 2, 0};
static signed char rtbl34nit[10] = {0, 9, 8, 5, 4, 3, 1, 0, 1, 0};
static signed char rtbl37nit[10] = {0, 10, 9, 5, 4, 3, 1, 0, 1, 0};
static signed char rtbl39nit[10] = {0, 8, 8, 5, 4, 3, 1, 0, 1, 0};
static signed char rtbl41nit[10] = {0, 10, 8, 5, 4, 3, 1, 0, 0, 0};
static signed char rtbl44nit[10] = {0, 10, 8, 5, 4, 3, 1, 0, 1, 0};
static signed char rtbl47nit[10] = {0, 10, 7, 4, 3, 2, 0, 1, 1, 0};
static signed char rtbl50nit[10] = {0, 6, 7, 4, 3, 2, 1, 0, 3, 0};
static signed char rtbl53nit[10] = {0, 6, 6, 3, 3, 2, 0, -1, 1, 0};
static signed char rtbl56nit[10] = {0, 7, 6, 3, 3, 2, 1, 0, 0, 0};
static signed char rtbl60nit[10] = {0, 6, 5, 3, 3, 2, 0, 0, 0, 0};
static signed char rtbl64nit[10] = {0, 6, 4, 2, 2, 1, 0, -1, 0, 0};
static signed char rtbl68nit[10] = {0, 2, 3, 2, 1, 1, 0, -1, 1, 0};
static signed char rtbl72nit[10] = {0, 4, 4, 4, 3, 2, 2, 1, 4, 0};
static signed char rtbl77nit[10] = {0, 5, 5, 3, 2, 1, 2, 0, 1, 0};
static signed char rtbl82nit[10] = {0, 6, 5, 3, 2, 1, 1, 1, 1, 0};
static signed char rtbl87nit[10] = {0, 6, 5, 3, 2, 1, 2, 2, 4, 0};
static signed char rtbl93nit[10] = {0, 6, 5, 3, 2, 2, 2, 3, 2, 0};
static signed char rtbl98nit[10] = {0, 5, 4, 3, 3, 2, 3, 3, 2, 0};
static signed char rtbl105nit[10] = {0, 5, 4, 3, 2, 2, 4, 4, 6, 0};
static signed char rtbl111nit[10] = {0, 5, 4, 3, 3, 2, 3, 4, 4, 0};
static signed char rtbl119nit[10] = {0, 5, 4, 3, 2, 2, 3, 5, 4, 0};
static signed char rtbl126nit[10] = {0, 5, 4, 2, 2, 2, 3, 4, 3, 0};
static signed char rtbl134nit[10] = {0, 4, 3, 2, 3, 3, 4, 5, 5, 0};
static signed char rtbl143nit[10] = {0, 5, 4, 3, 3, 3, 4, 7, 6, 0};
static signed char rtbl152nit[10] = {0, 5, 4, 3, 3, 4, 4, 7, 6, 0};
static signed char rtbl162nit[10] = {0, 5, 4, 3, 3, 3, 4, 8, 5, 0};
static signed char rtbl172nit[10] = {0, 6, 4, 3, 3, 4, 5, 9, 5, 0};
static signed char rtbl183nit[10] = {0, 6, 4, 3, 2, 2, 4, 6, 6, 0};
static signed char rtbl195nit[10] = {0, 4, 3, 3, 2, 2, 3, 7, 6, 0};
static signed char rtbl207nit[10] = {0, 4, 3, 2, 2, 2, 3, 7, 6, 0};
static signed char rtbl220nit[10] = {0, 4, 3, 3, 2, 2, 3, 6, 2, 0};
static signed char rtbl234nit[10] = {0, 4, 2, 2, 2, 2, 3, 7, 5, 0};
static signed char rtbl249nit[10] = {0, 3, 2, 1, 1, 2, 2, 8, 2, 0};
static signed char rtbl265nit[10] = {0, 2, 2, 1, 1, 2, 3, 5, 2, 0};
static signed char rtbl282nit[10] = {0, 1, 1, 1, 1, 1, 1, 5, 2, 0};
static signed char rtbl300nit[10] = {0, 2, 1, 1, 1, 2, 3, 5, 4, 0};
static signed char rtbl316nit[10] = {0, 2, 1, 0, 0, 1, 1, 4, 1, 0};
static signed char rtbl333nit[10] = {0, 1, 1, 1, 0, 1, 0, 5, 0, 0};
static signed char rtbl350nit[10] = {0, 2, 1, 1, 0, 1, 0, 2, 2, 0};
static signed char rtbl357nit[10] = {0, 1, 0, 0, 0, -1, 0, 4, 3, 0};
static signed char rtbl365nit[10] = {0, 2, 0, 1, 0, 1, 0, 3, -2, 0};
static signed char rtbl372nit[10] = {0, 1, 0, 0, 0, 0, 0, 6, 1, 0};
static signed char rtbl380nit[10] = {0, 1, 0, 1, 0, 1, 1, 4, 0, 0};
static signed char rtbl387nit[10] = {0, 1, 0, 0, 0, 0, 0, 5, 0, 0};
static signed char rtbl395nit[10] = {0, 1, 0, -1, -1, -1, -2, 3, 0, 0};
static signed char rtbl403nit[10] = {0, 3, 1, 1, 1, 0, 0, 2, 0, 0};
static signed char rtbl412nit[10] = {0, -1, -1, 0, 0, 0, 0, 2, -1, 0};
static signed char rtbl420nit[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static signed char ctbl2nit[30] = {0, 0, 0, -4, 6, -5, -3, 8, -3, 1, 2, -6, 4, 13, 1, -8, 6, -8, -5, 5, -1, -2, 2, -1, -1, 1, -1, -9, 0, -9};
static signed char ctbl3nit[30] = {0, 0, 0, -11, 0, -12, -4, 8, -4, -4, 0, -8, -11, -1, -11, -13, 2, -10, -7, 2, -5, -2, 1, -2, -2, 1, -2, -7, 0, -7};
static signed char ctbl4nit[30] = {0, 0, 0, -11, 0, -12, -13, -1, -13, -3, 2, -7, -10, 0, -10, -12, 0, -10, -8, 1, -5, -3, 0, -3, 0, 1, 0, -5, 1, -5};
static signed char ctbl5nit[30] = {0, 0, 0, -8, 1, -9, -10, 0, -10, -5, 1, -10, -10, 1, -9, -12, 0, -9, -7, 1, -5, -3, 0, -3, 0, 1, 0, -4, 1, -4};
static signed char ctbl6nit[30] = {0, 0, 0, 0, 7, -1, -1, 8, 0, -8, -1, -12, -11, 1, -9, -11, 1, -9, -7, 0, -6, -3, 0, -3, -1, 0, 0, -4, 0, -5};
static signed char ctbl7nit[30] = {0, 0, 0, 1, 7, -1, -9, 0, -8, -7, 1, -11, -12, -1, -11, -11, 0, -9, -5, 1, -4, -3, 0, -3, 0, 0, 0, -4, 0, -4};
static signed char ctbl8nit[30] = {0, 0, 0, -7, -1, -8, 1, 8, 1, -10, 0, -14, -9, 0, -10, -10, 0, -8, -5, 0, -5, -3, 0, -2, 0, 0, 0, -3, 0, -4};
static signed char ctbl9nit[30] = {0, 0, 0, -4, 0, -6, -5, 1, -6, -9, 0, -13, -7, 2, -10, -9, 0, -8, -6, 1, -5, -2, 0, -2, 0, 0, 0, -2, 0, -3};
static signed char ctbl10nit[30] = {0, 0, 0, -2, 1, -4, -5, 0, -7, -8, 0, -12, -9, 1, -11, -11, -1, -9, -4, 0, -4, -2, 0, -1, 0, 0, -1, -2, 0, -2};
static signed char ctbl11nit[30] = {0, 0, 0, -4, 0, -5, -5, -1, -8, -8, 0, -13, -8, 1, -11, -9, -1, -8, -3, 1, -2, -2, 0, -2, 0, 0, 0, -1, 0, -2};
static signed char ctbl12nit[30] = {0, 0, 0, 0, 3, -1, -2, 2, -12, -8, 0, -12, -8, 1, -12, -9, -1, -8, -4, 0, -3, -1, 0, -1, 0, 0, 0, -1, 0, -2};
static signed char ctbl13nit[30] = {0, 0, 0, 1, 2, 0, -2, 0, -15, -8, 0, -12, -7, 1, -11, -9, 0, -8, -3, 1, -2, -1, 0, -1, -1, 0, -1, 0, 0, -2};
static signed char ctbl14nit[30] = {0, 0, 0, 13, -11, 13, -4, 2, -17, -10, -1, -14, -6, 1, -11, -9, 0, -8, -3, 0, -2, -1, 0, -1, -1, 0, -2, 0, 0, -1};
static signed char ctbl15nit[30] = {0, 0, 0, 18, -7, 17, -7, -2, -20, -11, -1, -15, -4, 2, -9, -7, 1, -5, -3, 1, -2, 1, 0, 1, 0, 0, 0, 0, 0, -1};
static signed char ctbl16nit[30] = {0, 0, 0, 21, -1, 20, -6, -2, -19, -12, -1, -16, -5, 1, -10, -7, 0, -6, -1, 2, 0, -1, 0, -1, 0, 0, 0, 0, 0, -2};
static signed char ctbl17nit[30] = {0, 0, 0, 19, -2, 16, -4, 3, -16, -11, -1, -16, -5, 1, -10, -7, -1, -6, -4, 0, -2, 0, 0, -1, 0, 1, 0, 0, 0, -1};
static signed char ctbl19nit[30] = {0, 0, 0, 17, -2, 15, -8, -1, -19, -7, 2, -13, -5, 1, -9, -7, -1, -6, -2, 1, -1, 0, 0, 0, 0, 1, 0, 1, 0, -1};
static signed char ctbl20nit[30] = {0, 0, 0, 18, -1, 15, -8, 1, -20, -7, 2, -11, -5, 2, -8, -5, 0, -5, 0, 2, 0, 0, -1, 0, 0, 1, 1, 1, 0, -1};
static signed char ctbl21nit[30] = {0, 0, 0, 22, 2, 18, -11, -1, -23, -6, 3, -12, -7, 0, -9, -5, 1, -5, -1, 1, -1, 0, 1, 1, 0, 0, -1, 1, 0, 0};
static signed char ctbl22nit[30] = {0, 0, 0, -14, 3, -13, -14, 2, -22, -7, 3, -11, -7, 0, -9, -5, 1, -5, 0, 1, 0, 0, -1, 0, -1, 0, 0, 1, 0, -1};
static signed char ctbl24nit[30] = {0, 0, 0, -16, 11, -12, -12, 2, -19, -9, 1, -14, -8, 0, -9, -5, -1, -5, -1, 1, 0, 0, -1, -1, 0, 0, 1, 1, 0, -1};
static signed char ctbl25nit[30] = {0, 0, 0, -20, -1, -14, -15, 0, -24, -9, 0, -13, -8, 0, -9, -6, -1, -6, 0, 1, 1, 1, -1, -1, -1, 1, 0, 1, 0, -1};
static signed char ctbl27nit[30] = {0, 0, 0, -16, 3, -6, -15, -1, -25, -11, -2, -15, -4, 2, -6, -4, 0, -3, -1, 1, 0, 1, 1, 0, 0, 0, 1, 1, 0, -1};
static signed char ctbl29nit[30] = {0, 0, 0, -19, 1, -7, -13, 0, -23, -10, 0, -13, -7, -1, -9, -4, 0, -3, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1, 0, 0};
static signed char ctbl30nit[30] = {0, 0, 0, -13, -1, -11, -13, -1, -24, -11, -2, -15, -4, 2, -6, -1, 3, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, -1};
static signed char ctbl32nit[30] = {0, 0, 0, -15, -1, -13, -15, -2, -27, -10, -1, -13, -8, -1, -10, -1, 2, 0, 0, 1, 0, 0, -1, 0, 0, 1, 0, 1, 0, 0};
static signed char ctbl34nit[30] = {0, 0, 0, -16, -2, -11, -10, 3, -22, -7, 3, -7, -4, 1, -6, -1, 2, 0, 0, 1, 1, -1, 0, -1, 1, 0, 0, 1, 0, 0};
static signed char ctbl37nit[30] = {0, 0, 0, -13, 2, -8, -18, -2, -26, -9, 0, -10, -4, 1, -6, -2, 1, -1, 0, 1, 0, 0, -1, 0, 0, 0, 1, 1, 0, -1};
static signed char ctbl39nit[30] = {0, 0, 0, -16, 2, -13, -12, 2, -23, -8, 0, -9, -5, 1, -7, -2, 0, -1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, -1, -1};
static signed char ctbl41nit[30] = {0, 0, 0, -17, 0, -13, -14, 1, -25, -9, -1, -9, -4, 1, -6, -2, 0, -1, -1, 0, -1, 0, 0, 1, 0, 0, 0, 2, 0, 0};
static signed char ctbl44nit[30] = {0, 0, 0, -17, 0, -14, -14, -1, -24, -9, -1, -9, -5, 0, -7, -1, 0, -1, -1, -1, 0, 0, 1, 0, 1, 0, 1, 0, -1, -2};
static signed char ctbl47nit[30] = {0, 0, 0, -15, 0, -12, -15, -1, -26, -6, -1, -7, -5, -1, -6, -1, 1, 0, 1, 1, 1, 0, -1, 1, 1, 0, 0, 1, 0, 0};
static signed char ctbl50nit[30] = {0, 0, 0, -9, 0, -7, -17, -2, -27, -6, -1, -7, -5, -1, -7, 0, 1, 1, 0, 0, 1, 0, 0, -1, 0, 0, 0, 1, 0, 0};
static signed char ctbl53nit[30] = {0, 0, 0, -10, 1, -8, -16, 1, -23, -2, 2, -4, -6, -1, -6, -1, -1, -1, 1, 1, 2, 1, 0, 0, 0, 0, 0, 2, 0, 0};
static signed char ctbl56nit[30] = {0, 0, 0, -4, 0, -11, -19, -3, -26, 0, 4, -1, -5, -1, -6, 0, 0, 1, 0, 0, 0, -1, -1, 0, 1, 0, 1, 1, 0, -1};
static signed char ctbl60nit[30] = {0, 0, 0, 0, 2, -6, -13, 3, -21, -2, 2, -4, -7, -3, -7, 0, -1, 0, 0, 1, 1, 0, -1, 0, 1, 0, 0, 1, 0, 0};
static signed char ctbl64nit[30] = {0, 0, 0, -3, 1, -10, -15, 2, -24, -4, 1, -5, -3, -1, -5, 2, 2, 2, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, -1};
static signed char ctbl68nit[30] = {0, 0, 0, 4, 3, 1, -6, 5, -14, -1, 2, -2, 1, 3, -1, 1, 1, 2, 0, 0, 1, 0, -1, -1, 1, 0, 1, 2, 0, 1};
static signed char ctbl72nit[30] = {0, 0, 0, -3, -2, -9, -1, 14, -7, -7, -2, -8, -1, -1, -4, -2, -1, -1, 0, -1, 0, 0, -1, 1, 1, 0, 0, 2, 0, 1};
static signed char ctbl77nit[30] = {0, 0, 0, 4, 4, 0, -12, 3, -17, -5, 0, -4, -2, -1, -5, 1, 1, 1, 0, -1, 1, 1, 0, -1, 0, 0, 1, 2, 0, 0};
static signed char ctbl82nit[30] = {0, 0, 0, -1, 2, -5, -13, 1, -20, -2, 2, -2, -2, -1, -5, -2, -1, 0, 1, 1, 1, 1, 0, 1, 0, 0, 1, 2, 0, -1};
static signed char ctbl87nit[30] = {0, 0, 0, -4, 0, -8, -17, -2, -23, 1, 4, 1, -2, -1, -4, 1, 2, 2, 1, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0};
static signed char ctbl93nit[30] = {0, 0, 0, -2, 4, -6, -18, -3, -24, -3, 0, -4, -1, 0, -3, 0, -1, -1, 0, 0, 2, 1, 0, 0, 0, 0, 0, 2, 0, 0};
static signed char ctbl98nit[30] = {0, 0, 0, -1, 3, -5, -14, 3, -17, 1, 4, 0, -2, -1, -4, 0, 0, 0, 0, -1, 1, -1, 0, 0, 0, 0, 1, 2, 0, 0};
static signed char ctbl105nit[30] = {0, 0, 0, 2, 6, -1, -16, 0, -20, 0, 3, 0, 0, 1, -1, 1, 1, 1, 0, 0, 0, 0, 0, 1, -1, 0, 0, 2, -1, -1};
static signed char ctbl111nit[30] = {0, 0, 0, -1, 4, -4, -12, 3, -16, -2, 0, -3, -1, 0, -2, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 1, 1, -1, -1};
static signed char ctbl119nit[30] = {0, 0, 0, -3, 1, -7, -10, 4, -11, -4, 0, -4, 1, 2, 0, 1, 0, 2, 0, -1, 0, 1, 0, 1, -1, 0, 0, 1, -1, -1};
static signed char ctbl126nit[30] = {0, 0, 0, -7, 2, -10, -11, 0, -14, 2, 3, 1, 0, 1, -1, 0, -1, -1, 0, -1, 1, 0, 1, 0, 0, 1, 1, 1, -1, -1};
static signed char ctbl134nit[30] = {0, 0, 0, -3, 1, -7, -9, 4, -10, 1, 4, 1, 0, 0, -1, -1, -1, -1, 0, -1, 0, 0, 0, 1, 0, 1, 0, 2, -1, 0};
static signed char ctbl143nit[30] = {0, 0, 0, -6, 2, -9, -10, 1, -12, 0, 2, -1, -2, -1, -2, 0, 0, 0, 1, -1, 1, 0, 0, 1, -1, 0, 0, 1, 0, -1};
static signed char ctbl152nit[30] = {0, 0, 0, -5, 5, -6, -8, 5, -9, -1, 0, -3, -1, -1, -2, -1, -1, -1, 0, -1, 1, 0, 0, 0, 0, 0, 1, 2, 0, 0};
static signed char ctbl162nit[30] = {0, 0, 0, -7, 2, -9, -10, 2, -11, 0, 2, 0, -1, -1, -3, 0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 1, 3, 0, 0};
static signed char ctbl172nit[30] = {0, 0, 0, -9, 2, -9, -11, -1, -13, -1, 0, -3, 0, 0, -1, 0, 0, 1, 1, 0, 1, -1, 0, 0, 0, 0, 1, 1, -1, 0};
static signed char ctbl183nit[30] = {0, 0, 0, -8, 0, -10, -9, -1, -10, -3, -1, -4, 0, 0, -1, 1, 1, 1, 1, -1, 1, 0, 1, 1, 0, 0, 0, 2, -1, -1};
static signed char ctbl195nit[30] = {0, 0, 0, 0, 9, 0, -6, 2, -8, 0, 1, -1, 0, 0, 0, 2, 1, 1, 0, -1, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0};
static signed char ctbl207nit[30] = {0, 0, 0, -3, 5, -3, -11, 0, -10, 1, 3, 0, 1, 0, 0, 0, 0, -1, 1, -1, 1, 0, 0, 1, -1, 0, 0, 3, 0, 0};
static signed char ctbl220nit[30] = {0, 0, 0, -5, 2, -6, -8, 0, -8, -1, 0, -2, 0, -1, -1, 0, 0, -1, 1, -1, 1, -1, 1, 1, 0, 0, 0, 2, -1, 0};
static signed char ctbl234nit[30] = {0, 0, 0, -8, -1, -7, -10, 0, -9, 2, 2, 0, 0, -1, -1, -1, -1, 0, 2, 0, 0, -1, 0, 1, 0, 0, 0, 1, -1, -1};
static signed char ctbl249nit[30] = {0, 0, 0, -2, 1, -4, -12, -2, -9, 1, 0, -1, 2, 2, 2, 1, 1, 1, 0, -1, 0, 0, 0, 1, 0, 0, 0, 1, 0, -1};
static signed char ctbl265nit[30] = {0, 0, 0, -1, 0, -4, -9, -1, -6, 1, 3, 1, 1, 1, 0, 0, -1, 0, 0, -1, 0, -1, 0, 1, 0, 0, -1, 2, 0, 0};
static signed char ctbl282nit[30] = {0, 0, 0, 3, 3, 0, -5, 2, -2, -2, 0, -2, 2, 2, 1, 1, 0, 1, 0, 0, 0, -1, 0, 0, 0, 0, 1, 1, 0, -1};
static signed char ctbl300nit[30] = {0, 0, 0, -4, 2, -4, -4, -1, -3, 1, 1, 1, 2, 2, 2, 1, 1, 1, 0, -1, 0, -1, 0, 0, 0, 0, 0, 1, -1, -1};
static signed char ctbl316nit[30] = {0, 0, 0, -7, -1, -6, -6, -2, -4, 1, 0, -1, 2, 2, 2, 1, 1, 1, 0, 0, 0, -1, 0, 0, 0, 0, 1, 1, 0, -1};
static signed char ctbl333nit[30] = {0, 0, 0, -2, 3, -2, -5, -1, -3, 0, -1, -1, 1, 1, 1, -1, -1, 0, 1, 0, 0, -1, 0, 0, 0, 0, 0, 1, 0, 0};
static signed char ctbl350nit[30] = {0, 0, 0, -7, 1, -3, -6, 0, -2, -1, -1, -2, 0, -1, -1, 1, 1, 1, -1, -1, -1, 0, 0, 1, 0, 0, 0, 1, 0, 0};
static signed char ctbl357nit[30] = {0, 0, 0, -2, 3, 1, -5, -1, -2, 2, 1, 0, -1, -1, 0, 0, 0, 0, 1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1};
static signed char ctbl365nit[30] = {0, 0, 0, -7, -1, -3, -4, 0, -1, -1, -1, -2, 1, 0, 2, 0, 0, -2, 0, -1, 1, -1, 0, 0, 0, 0, -1, 1, 0, 1};
static signed char ctbl372nit[30] = {0, 0, 0, -2, 3, 1, -4, 0, -2, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, -1, 0, 1, 0, 0, -1, 0, -1, -1};
static signed char ctbl380nit[30] = {0, 0, 0, -3, 3, -1, -2, 1, 0, -1, -2, -2, 1, 1, 1, 0, 0, 0, 1, 0, 1, -1, 0, 1, 0, 0, -1, 0, -1, -1};
static signed char ctbl387nit[30] = {0, 0, 0, -6, 0, -3, -3, 0, -1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 0, -1, 0, 0, 1, 0, 0, 1, 0, 0};
static signed char ctbl395nit[30] = {0, 0, 0, 0, 2, 1, -4, -1, -2, 0, -1, -1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0};
static signed char ctbl403nit[30] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static signed char ctbl412nit[30] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static signed char ctbl420nit[30] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static signed char aid9795[3] = {0xB1, 0x90, 0xE3};
static signed char aid9710[3] = {0xB1, 0x90, 0xCD};
static signed char aid9598[3] = {0xB1, 0x90, 0xB0};
static signed char aid9485[3] = {0xB1, 0x90, 0x93};
static signed char aid9400[3] = {0xB1, 0x90, 0x7D};
static signed char aid9292[3] = {0xB1, 0x90, 0x61};
static signed char aid9214[3] = {0xB1, 0x90, 0x4D};
static signed char aid9106[3] = {0xB1, 0x90, 0x31};
static signed char aid9029[3] = {0xB1, 0x90, 0x1D};
static signed char aid8920[3] = {0xB1, 0x90, 0x01};
static signed char aid8843[3] = {0xB1, 0x80, 0xED};
static signed char aid8735[3] = {0xB1, 0x80, 0xD1};
static signed char aid8657[3] = {0xB1, 0x80, 0xBD};
static signed char aid8549[3] = {0xB1, 0x80, 0xA1};
static signed char aid8471[3] = {0xB1, 0x80, 0x8D};
static signed char aid8355[3] = {0xB1, 0x80, 0x6F};
static signed char aid8170[3] = {0xB1, 0x80, 0x3F};
static signed char aid8061[3] = {0xB1, 0x80, 0x23};
static signed char aid7972[3] = {0xB1, 0x80, 0x0C};
static signed char aid7860[3] = {0xB1, 0x70, 0xEF};
static signed char aid7663[3] = {0xB1, 0x70, 0xBC};
static signed char aid7558[3] = {0xB1, 0x70, 0xA1};
static signed char aid7357[3] = {0xB1, 0x70, 0x6D};
static signed char aid7132[3] = {0xB1, 0x70, 0x33};
static signed char aid7051[3] = {0xB1, 0x70, 0x1E};
static signed char aid6823[3] = {0xB1, 0x60, 0xE3};
static signed char aid6629[3] = {0xB1, 0x60, 0xB1};
static signed char aid6327[3] = {0xB1, 0x60, 0x63};
static signed char aid6118[3] = {0xB1, 0x60, 0x2D};
static signed char aid5898[3] = {0xB1, 0x50, 0xF4};
static signed char aid5577[3] = {0xB1, 0x50, 0xA1};
static signed char aid5263[3] = {0xB1, 0x50, 0x50};
static signed char aid4957[3] = {0xB1, 0x50, 0x01};
static signed char aid4636[3] = {0xB1, 0x40, 0xAE};
static signed char aid4319[3] = {0xB1, 0x40, 0x5C};
static signed char aid3874[3] = {0xB1, 0x30, 0xE9};
static signed char aid3483[3] = {0xB1, 0x30, 0x84};
static signed char aid3050[3] = {0xB1, 0x30, 0x14};
static signed char aid2612[3] = {0xB1, 0x20, 0xA3};
static signed char aid2055[3] = {0xB1, 0x20, 0x13};
static signed char aid1529[3] = {0xB1, 0x10, 0x8B};
static signed char aid1002[3] = {0xB1, 0x10, 0x03};
static signed char aid755[3] = {0xB1, 0x00, 0xC3};
static signed char aid507[3] = {0xB1, 0x00, 0x83};
static signed char aid399[3] = {0xB1, 0x00, 0x67};
static signed char aid174[3] = {0xB1, 0x00, 0x2D};
static signed char aid46[3] = {0xB1, 0x00, 0x0C};

static unsigned char elv1[3] = {
	0xAC, 0x14
};
static unsigned char elv2[3] = {
	0xAC, 0x13
};
static unsigned char elv3[3] = {
	0xAC, 0x12
};
static unsigned char elv4[3] = {
	0xAC, 0x11
};
static unsigned char elv5[3] = {
	0xAC, 0x10
};
static unsigned char elv6[3] = {
	0xAC, 0x0F
};
static unsigned char elv7[3] = {
	0xAC, 0x0E
};
static unsigned char elv8[3] = {
	0xAC, 0x0D
};
static unsigned char elv9[3] = {
	0xAC, 0x0C
};
static unsigned char elv10[3] = {
	0xAC, 0x0B
};
static unsigned char elv11[3] = {
	0xAC, 0x0A
};
static unsigned char elv12[3] = {
	0xAC, 0x00
};


static unsigned char elvCaps1[3]  = {
	0xBC, 0x14
};
static unsigned char elvCaps2[3]  = {
	0xBC, 0x13
};
static unsigned char elvCaps3[3]  = {
	0xBC, 0x12
};
static unsigned char elvCaps4[3]  = {
	0xBC, 0x11
};
static unsigned char elvCaps5[3]  = {
	0xBC, 0x10
};
static unsigned char elvCaps6[3]  = {
	0xBC, 0x0F
};
static unsigned char elvCaps7[3]  = {
	0xBC, 0x0E
};
static unsigned char elvCaps8[3]  = {
	0xBC, 0x0D
};
static unsigned char elvCaps9[3]  = {
	0xBC, 0x0C
};
static unsigned char elvCaps10[3]  = {
	0xBC, 0x0B
};
static unsigned char elvCaps11[3]  = {
	0xBC, 0x0A
};
static unsigned char elvCaps12[3] = {
	0xBC, 0x00
};


static char elvss_offset1[3] = {0x00, 0x04, 0x00};
static char elvss_offset2[3] = {0x00, 0x03, -0x01};
static char elvss_offset3[3] = {0x00, 0x02, -0x02};
static char elvss_offset4[3] = {0x00, 0x01, -0x03};
static char elvss_offset5[3] = {0x00, 0x00, -0x04};
static char elvss_offset6[3] = {-0x05, -0x05, -0x0A};



#endif
