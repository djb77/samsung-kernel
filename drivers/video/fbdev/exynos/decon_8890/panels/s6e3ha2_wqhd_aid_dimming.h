/* linux/drivers/video/exynos_decon/panel/s6e3ha2_wqhd_aid_dimming.h
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

#ifndef __S6E3HA2_WQHD_AID_DIMMING_H__
#define __S6E3HA2_WQHD_AID_DIMMING_H__

// start of aid sheet in rev.C opmanual
static signed char RCrtbl2nit[10] = {0, 28, 27, 28, 24, 20, 15, 12, 8, 0};
static signed char RCrtbl3nit[10] = {0, 27, 28, 27, 23, 20, 16, 13, 7, 0};
static signed char RCrtbl4nit[10] = {0, 27, 25, 23, 19, 17, 15, 11, 7, 0};
static signed char RCrtbl5nit[10] = {0, 25, 25, 22, 18, 16, 14, 11, 8, 0};
static signed char RCrtbl6nit[10] = {0, 25, 24, 21, 17, 16, 14, 11, 7, 0};
static signed char RCrtbl7nit[10] = {0, 25, 23, 20, 17, 15, 13, 10, 7, 0};
static signed char RCrtbl8nit[10] = {0, 24, 22, 19, 16, 14, 12, 10, 7, 0};
static signed char RCrtbl9nit[10] = {0, 23, 22, 19, 15, 14, 12, 9, 6, 0};
static signed char RCrtbl10nit[10] = {0, 23, 21, 18, 14, 13, 12, 9, 6, 0};
static signed char RCrtbl11nit[10] = {0, 20, 21, 17, 14, 13, 11, 9, 6, 0};
static signed char RCrtbl12nit[10] = {0, 19, 19, 16, 13, 12, 10, 8, 6, 0};
static signed char RCrtbl13nit[10] = {0, 19, 19, 16, 12, 12, 10, 8, 6, 0};
static signed char RCrtbl14nit[10] = {0, 19, 18, 15, 12, 12, 9, 8, 6, 0};
static signed char RCrtbl15nit[10] = {0, 19, 18, 15, 11, 11, 9, 8, 6, 0};
static signed char RCrtbl16nit[10] = {0, 16, 18, 15, 11, 11, 9, 7, 6, 0};
static signed char RCrtbl17nit[10] = {0, 16, 17, 14, 11, 11, 9, 7, 6, 0};
static signed char RCrtbl19nit[10] = {0, 16, 16, 13, 10, 10, 8, 7, 6, 0};
static signed char RCrtbl20nit[10] = {0, 16, 15, 13, 10, 9, 8, 7, 6, 0};
static signed char RCrtbl21nit[10] = {0, 15, 15, 12, 9, 9, 7, 7, 6, 0};
static signed char RCrtbl22nit[10] = {0, 15, 15, 12, 9, 8, 7, 7, 6, 0};
static signed char RCrtbl24nit[10] = {0, 14, 14, 11, 8, 8, 7, 6, 5, 0};
static signed char RCrtbl25nit[10] = {0, 13, 14, 11, 8, 8, 7, 6, 5, 0};
static signed char RCrtbl27nit[10] = {0, 13, 13, 10, 7, 7, 7, 6, 5, 0};
static signed char RCrtbl29nit[10] = {0, 12, 13, 10, 8, 7, 6, 6, 5, 0};
static signed char RCrtbl30nit[10] = {0, 12, 13, 10, 7, 7, 6, 5, 5, 0};
static signed char RCrtbl32nit[10] = {0, 12, 13, 10, 7, 7, 5, 5, 5, 0};
static signed char RCrtbl34nit[10] = {0, 10, 12, 9, 7, 6, 5, 5, 5, 0};
static signed char RCrtbl37nit[10] = {0, 10, 11, 8, 6, 7, 5, 5, 5, 0};
static signed char RCrtbl39nit[10] = {0, 9, 11, 8, 6, 6, 5, 5, 5, 0};
static signed char RCrtbl41nit[10] = {0, 11, 10, 8, 6, 5, 5, 5, 5, 0};
static signed char RCrtbl44nit[10] = {0, 9, 10, 7, 5, 5, 4, 5, 5, 0};
static signed char RCrtbl47nit[10] = {0, 10, 9, 7, 5, 5, 4, 5, 5, 0};
static signed char RCrtbl50nit[10] = {0, 8, 9, 7, 5, 5, 4, 5, 5, 0};
static signed char RCrtbl53nit[10] = {0, 9, 8, 6, 4, 4, 4, 5, 5, 0};
static signed char RCrtbl56nit[10] = {0, 8, 8, 6, 4, 4, 4, 5, 4, 0};
static signed char RCrtbl60nit[10] = {0, 8, 7, 5, 4, 4, 4, 5, 5, 0};
static signed char RCrtbl64nit[10] = {0, 9, 6, 5, 3, 3, 3, 4, 4, 0};
static signed char RCrtbl68nit[10] = {0, 9, 6, 4, 3, 3, 3, 4, 4, 0};
static signed char RCrtbl72nit[10] = {0, 7, 6, 5, 3, 3, 3, 4, 4, 0};
static signed char RCrtbl77nit[10] = {0, 8, 5, 4, 3, 3, 2, 3, 2, 0};
static signed char RCrtbl82nit[10] = {0, 4, 6, 4, 3, 3, 3, 4, 3, 0};
static signed char RCrtbl87nit[10] = {0, 4, 5, 3, 2, 2, 3, 5, 3, 0};
static signed char RCrtbl93nit[10] = {0, 5, 4, 3, 3, 2, 3, 4, 2, 0};
static signed char RCrtbl98nit[10] = {0, 6, 4, 3, 2, 2, 4, 4, 1, 0};
static signed char RCrtbl105nit[10] = {0, 5, 5, 3, 3, 2, 3, 5, 2, 0};
static signed char RCrtbl111nit[10] = {0, 5, 4, 3, 2, 1, 3, 2, 0, 0};
static signed char RCrtbl119nit[10] = {0, 4, 4, 2, 1, 2, 4, 4, 0, 0};
static signed char RCrtbl126nit[10] = {0, 6, 3, 2, 2, 3, 2, 4, 0, 0};
static signed char RCrtbl134nit[10] = {0, 5, 3, 2, 1, 3, 3, 3, 0, 0};
static signed char RCrtbl143nit[10] = {0, 4, 3, 2, 2, 3, 3, 5, 2, 0};
static signed char RCrtbl152nit[10] = {0, 4, 3, 2, 2, 2, 3, 3, 1, 0};
static signed char RCrtbl162nit[10] = {0, 1, 2, 1, 1, 1, 3, 3, 0, 0};
static signed char RCrtbl172nit[10] = {0, 3, 2, 2, 1, 1, 2, 3, 1, 0};
static signed char RCrtbl183nit[10] = {0, 0, 3, 1, 0, 0, 1, 2, 0, 0};
static signed char RCrtbl195nit[10] = {0, 2, 2, 1, 0, 0, 2, 2, 0, 0};
static signed char RCrtbl207nit[10] = {0, 4, 1, 1, 0, 0, 1, 2, 0, 0};
static signed char RCrtbl220nit[10] = {0, 3, 1, 1, 0, 0, 1, 2, 0, 0};
static signed char RCrtbl234nit[10] = {0, 3, 2, 1, 1, 1, 2, 1, 0, 0};
static signed char RCrtbl249nit[10] = {0, 1, 1, 1, 0, 1, 1, 2, 0, 0};
static signed char RCrtbl265nit[10] = {0, 1, 1, 1, 1, 1, 1, 1, 0, 0};
static signed char RCrtbl282nit[10] = {0, 3, 0, 1, 1, 1, 1, 1, -1, 0};
static signed char RCrtbl300nit[10] = {0, 3, 0, 0, 0, 1, 0, 0, -1, 0};
static signed char RCrtbl316nit[10] = {0, 2, 0, 0, 0, 0, 0, 0, -1, 0};
static signed char RCrtbl333nit[10] = {0, 0, 1, 0, -1, 0, -1, -1, -1, 0};
static signed char RCrtbl360nit[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static signed char RCctbl2nit[30] = {0, 0, 0, 0, 0, 0,0, 0, 0, 0, 0, -4, 0, 1, -3, -4, 2, -5, -1, 1, -3, -1, 0, -2, -1, 0, -1, -9, 1, -9};
static signed char RCctbl3nit[30] = {0, 0, 0, 0, 0, 0,0, 0, 0, 0, 0, -4, -2, 1, -5, -3, 1, -5, -3, 1, -4, -2, 0, -3, -2, 0, -2, -7, 1, -7};
static signed char RCctbl4nit[30] = {0, 0, 0, 0, 0, 0,0, 0, 0, -1, 0, -5, -1, 1, -4, -3, 1, -6, -2, 0, -4, -1, 0, -2, -2, 0, -2, -6, 1, -6};
static signed char RCctbl5nit[30] = {0, 0, 0, 0, 0, 0,0, 0, -3, -1, 0, -5, 0, 2, -2, -1, 2, -6, -3, 1, -4, -1, 0, -2, -2, 0, -1, -5, 1, -5};
static signed char RCctbl6nit[30] = {0, 0, 0, 0, 0, 0,0, 0, -3, -1, 0, -5, 0, 2, -3, -1, 2, -6, -3, 1, -3, -1, 0, -2, -1, 0, -2, -5, 1, -5};
static signed char RCctbl7nit[30] = {0, 0, 0, 0, 0, 0,0, 0, -2, 0, 1, -5, -2, 0, -4, -1, 2, -7, -3, 0, -4, -1, 0, -1, -1, 0, -2, -4, 1, -4};
static signed char RCctbl8nit[30] = {0, 0, 0, 0, 0, 0,0, 0, -3, 0, 2, -4, -2, 0, -5, 0, 2, -6, -3, 0, -3, -1, 0, -2, -1, 0, -1, -5, 0, -5};
static signed char RCctbl9nit[30] = {0, 0, 0, 0, 0, 0,0, 0, -5, -2, 0, -5, 0, 1, -5, -1, 2, -5, -3, 1, -3, -1, 0, -2, -1, 0, -1, -4, 0, -4};
static signed char RCctbl10nit[30] = {0, 0, 0, 0, 0, 0,0, 0, -5, -1, 0, -6, -1, 1, -4, -1, 2, -6, -3, 1, -3, 0, 0, -1, -1, 0, -1, -4, 0, -4};
static signed char RCctbl11nit[30] = {0, 0, 0, 0, 0, 0,-1, 0, -5, 0, 2, -5, -1, 1, -5, -2, 1, -6, -2, 1, -3, 0, 0, -1, -1, 0, -1, -4, 0, -3};
static signed char RCctbl12nit[30] = {0, 0, 0, 0, 0, 0,0, 2, -4, -1, 2, -5, 0, 1, -5, -1, 2, -4, -2, 0, -4, 0, 0, -1, -1, 0, -1, -4, 0, -3};
static signed char RCctbl13nit[30] = {0, 0, 0, 0, 0, 0,0, 1, -7, -1, 2, -4, -1, 2, -6, -2, 1, -5, -2, 0, -4, 0, 0, -1, -1, 0, -1, -3, 0, -2};
static signed char RCctbl14nit[30] = {0, 0, 0, 0, 0, 0,0, 1, -6, 0, 2, -4, 0, 2, -6, -2, 1, -5, -2, 0, -2, 0, 0, -1, -1, 0, -1, -3, 0, -2};
static signed char RCctbl15nit[30] = {0, 0, 0, 0, 0, 0,-1, 2, -6, 0, 1, -5, -2, 1, -6, -2, 1, -6, -1, 0, -1, 0, 0, -1, -1, 0, -1, -3, 0, -2};
static signed char RCctbl16nit[30] = {0, 0, 0, 0, 0, 0,-1, 2, -6, 0, 1, -4, 0, 1, -6, -2, 1, -5, -3, 0, -3, 0, 0, 0, -1, 0, -1, -3, 0, -2};
static signed char RCctbl17nit[30] = {0, 0, 0, 0, 0, 0,0, 1, -6, -1, 1, -6, -1, 0, -6, -2, 1, -4, -2, 0, -3, 0, 0, 0, -1, 0, -1, -3, 0, -2};
static signed char RCctbl19nit[30] = {0, 0, 0, 0, 0, 0,0, 1, -7, 0, 1, -6, -1, 1, -5, -2, 1, -6, -2, 0, -2, 0, 0, 0, -1, 0, -1, -3, 0, -2};
static signed char RCctbl20nit[30] = {0, 0, 0, 0, 0, 0,0, 1, -9, -1, 1, -5, 0, 1, -4, -3, 1, -5, -2, 0, -2, 0, 0, -1, 0, 0, 0, -3, 0, -2};
static signed char RCctbl21nit[30] = {0, 0, 0, 0, 0, 0,0, 1, -7, -1, 1, -6, -1, 1, -5, -2, 1, -4, -2, 0, -2, 0, 0, -1, 0, 0, 0, -3, 0, -2};
static signed char RCctbl22nit[30] = {0, 0, 0, 0, 0, 0,0, 1, -7, -1, 1, -6, -2, 1, -4, -2, 1, -5, -1, 0, -2, 0, 0, 0, -1, 0, -1, -2, 0, -1};
static signed char RCctbl24nit[30] = {0, 0, 0, 0, 0, 0,0, 0, -9, -1, 1, -6, -2, 1, -5, -2, 1, -5, 0, 0, -1, 0, 0, 0, -1, 0, -1, -2, 0, -1};
static signed char RCctbl25nit[30] = {0, 0, 0, 0, 0, 0,0, 1, -8, -2, 0, -7, -2, 1, -5, -2, 0, -5, 0, 0, -1, 0, 0, 0, -1, 0, -1, -2, 0, -1};
static signed char RCctbl27nit[30] = {0, 0, 0, 0, 0, 0,0, 1, -9, -2, 1, -6, -2, 1, -4, -1, 0, -4, 0, 0, -1, 0, 0, 0, -1, 0, -1, -2, 0, -1};
static signed char RCctbl29nit[30] = {0, 0, 0, 0, 0, 0,0, 0, -9, -2, 1, -7, -2, 1, -4, -2, 0, -5, -1, 0, -1, 0, 0, 0, 0, 0, 0, -2, 0, -1};
static signed char RCctbl30nit[30] = {0, 0, 0, 0, 0, 0,0, 0, -9, -3, 0, -7, -2, 0, -4, -2, 0, -5, -1, 0, -1, -1, 0, -1, 0, 0, 0, -2, 0, -1};
static signed char RCctbl32nit[30] = {0, 0, 0, 0, 0, 0,-1, 0, -9, -3, 0, -7, -2, 0, -4, -2, 0, -4, -1, 0, -1, -1, 0, -1, 0, 0, 0, -2, 0, -1};
static signed char RCctbl34nit[30] = {0, 0, 0, 0, 0, 0,0, 2, -7, -2, 1, -7, -2, 0, -3, -3, 0, -5, -1, 0, 0, 0, 0, -1, 0, 0, 0, -2, 0, -1};
static signed char RCctbl37nit[30] = {0, 0, 0, 0, 0, 0,0, 0, -9, -2, 1, -6, -2, 0, -2, -1, 0, -4, -1, 0, 0, 0, 0, -1, 0, 0, 0, -2, 0, -1};
static signed char RCctbl39nit[30] = {0, 0, 0, 0, 0, 0,-1, 1, -9, -2, 1, -6, -2, 0, -3, -2, 0, -4, -1, 0, 0, 0, 0, -1, 0, 0, 0, -2, 0, -1};
static signed char RCctbl41nit[30] = {0, 0, 0, 0, 0, 0,0, 1, -12, -1, 1, -6, -1, 0, -3, -1, 0, -4, 0, 0, -1, 0, 0, 0, -1, 0, -1, -2, 0, 0};
static signed char RCctbl44nit[30] = {0, 0, 0, 0, 0, 0,0, 2, -9, -3, 1, -7, -3, 0, -2, -2, 0, -5, 0, 0, 0, 0, 0, -1, -1, 0, 0, -2, 0, 0};
static signed char RCctbl47nit[30] = {0, 0, 0, 0, 0, 0,0, 2, -10, -1, 1, -5, -3, 0, -3, -2, 0, -4, 0, 0, 0, 0, 0, 0, -1, 0, -1, -2, 0, 0};
static signed char RCctbl50nit[30] = {0, 0, 0, 0, 0, 0,0, 2, -9, -3, 1, -5, -2, 0, -2, -2, 0, -4, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0};
static signed char RCctbl53nit[30] = {0, 0, 0, 0, 0, 0,-1, 2, -10, -1, 1, -5, -2, 1, -3, -1, 0, -3, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0};
static signed char RCctbl56nit[30] = {0, 0, 0, 0, 0, 0,0, 2, -9, -2, 1, -5, -2, 0, -3, -1, 0, -3, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0};
static signed char RCctbl60nit[30] = {0, 0, 0, 0, 0, 0,0, 2, -8, -2, 1, -6, -3, 0, -3, 0, 0, -2, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0};
static signed char RCctbl64nit[30] = {0, 0, 0, 0, 0, 0,0, 3, -10, -2, 1, -4, -1, 1, -1, -1, 0, -3, 0, 0, 0, 1, 0, 0, -1, 0, 0, -1, 0, 0};
static signed char RCctbl68nit[30] = {0, 0, 0, 0, 0, 0,0, 2, -9, -2, 1, -6, -2, 0, -1, 0, 0, -3, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0};
static signed char RCctbl72nit[30] = {0, 0, 0, 0, 0, 0,0, 2, -9, -3, 0, -5, -2, 0, -2, 0, 0, -2, 0, 0, 0, 1, 0, 0, -1, 0, 0, -1, 0, 0};
static signed char RCctbl77nit[30] = {0, 0, 0, 0, 0, 0,0, 3, -9, -3, 0, -5, -1, 0, -2, 0, 0, -2, 0, 0, 1, 0, 0, -1, -1, 0, 0, 0, 0, 0};
static signed char RCctbl82nit[30] = {0, 0, 0, 0, 0, 0,-1, 2, -8, -1, 1, -3, -1, 0, -2, 1, 0, -2, 0, 0, 0, 0, 0, 0, -2, 0, 0, 0, 0, 0};
static signed char RCctbl87nit[30] = {0, 0, 0, 0, 0, 0,-1, 2, -7, -2, 0, -5, -1, 0, -1, 1, 0, -1, 0, 0, 1, 1, 0, 0, -1, 0, 0, 0, 0, 0};
static signed char RCctbl93nit[30] = {0, 0, 0, 0, 0, 0,0, 3, -7, -2, 1, -3, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0};
static signed char RCctbl98nit[30] = {0, 0, 0, 0, 0, 0,-1, 2, -8, -2, 1, -3, 0, 0, -1, 1, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0};
static signed char RCctbl105nit[30] = {0, 0, 0, 0, 0, 0,-2, 2, -8, -2, 0, -3, 0, 0, -2, 0, 0, -1, 1, 0, 1, 0, 0, 0, -2, 0, 0, 0, 0, 0};
static signed char RCctbl111nit[30] = {0, 0, 0, 0, 0, 0,-1, 3, -7, -2, 0, -3, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0};
static signed char RCctbl119nit[30] = {0, 0, 0, 0, 0, 0,-1, 2, -6, -2, 1, -4, 0, 0, -1, 0, 0, -2, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0};
static signed char RCctbl126nit[30] = {0, 0, 0, 0, 0, 0,-1, 2, -6, -2, 1, -4, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static signed char RCctbl134nit[30] = {0, 0, 0, 0, 0, 0,-3, 2, -8, -1, 0, -2, 0, 0, 0, 1, 0, -1, 0, 0, 1, 0, 0, 0, -1, 0, 0, 0, 0, 0};
static signed char RCctbl143nit[30] = {0, 0, 0, 0, 0, 0,-2, 2, -7, -2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0};
static signed char RCctbl152nit[30] = {0, 0, 0, 0, 0, 0,-3, 1, -6, -2, 0, -2, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, -1, 0, -1, 0, 0, 0};
static signed char RCctbl162nit[30] = {0, 0, 0, 0, 0, 0,-2, 2, -5, -1, 0, -2, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0};
static signed char RCctbl172nit[30] = {0, 0, 0, 0, 0, 0,-2, 2, -5, -1, 0, -1, 0, 0, 0, 0, 0, 0, 1, 0, 1, -1, 0, 0, 0, 0, 0, 0, 0, 0};
static signed char RCctbl183nit[30] = {0, 0, 0, 0, 0, 0,-2, 1, -3, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static signed char RCctbl195nit[30] = {0, 0, 0, 0, 0, 0,-2, 1, -5, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static signed char RCctbl207nit[30] = {0, 0, 0, 0, 0, 0,-1, 2, -5, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static signed char RCctbl220nit[30] = {0, 0, 0, 0, 0, 0,-3, 1, -5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static signed char RCctbl234nit[30] = {0, 0, 0, 0, 0, 0,-1, 1, -3, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static signed char RCctbl249nit[30] = {0, 0, 0, 0, 0, 0,0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static signed char RCctbl265nit[30] = {0, 0, 0, 0, 0, 0,0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static signed char RCctbl282nit[30] = {0, 0, 0, 0, 0, 0,0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static signed char RCctbl300nit[30] = {0, 0, 0, 0, 0, 0,0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static signed char RCctbl316nit[30] = {0, 0, 0, 0, 0, 0,0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static signed char RCctbl333nit[30] = {0, 0, 0, 0, 0, 0,0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static signed char RCctbl360nit[30] = {0, 0, 0, 0, 0, 0,0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static signed char aid9821[3] = {0xB2, 0xE2, 0x90};
static signed char aid9717[3] = {0xB2, 0xC6, 0x90};
static signed char aid9639[3] = {0xB2, 0xB3, 0x90};
static signed char aid9538[3] = {0xB2, 0x96, 0x90};
static signed char aid9433[3] = {0xB2, 0x83, 0x90};
static signed char aid9387[3] = {0xB2, 0x67, 0x90};
static signed char aid9309[3] = {0xB2, 0x53, 0x90};
static signed char aid9216[3] = {0xB2, 0x3D, 0x90};
static signed char aid9123[3] = {0xB2, 0x27, 0x90};
static signed char aid9037[3] = {0xB2, 0x14, 0x90};
static signed char aid8998[3] = {0xB2, 0x00, 0x90};
static signed char aid8909[3] = {0xB2, 0xE7, 0x80};
static signed char aid8816[3] = {0xB2, 0xD4, 0x80};
static signed char aid8738[3] = {0xB2, 0xC5, 0x80};
static signed char aid8661[3] = {0xB2, 0xB1, 0x80};
static signed char aid8587[3] = {0xB2, 0x92, 0x80};
static signed char aid8412[3] = {0xB2, 0x68, 0x80};
static signed char aid8331[3] = {0xB2, 0x53, 0x80};
static signed char aid8245[3] = {0xB2, 0x3C, 0x80};
static signed char aid8160[3] = {0xB2, 0x27, 0x80};
static signed char aid7985[3] = {0xB2, 0xF7, 0x70};
static signed char aid7908[3] = {0xB2, 0xE2, 0x70};
static signed char aid7729[3] = {0xB2, 0xB6, 0x70};
static signed char aid7550[3] = {0xB2, 0x85, 0x70};
static signed char aid7473[3] = {0xB2, 0x71, 0x70};
static signed char aid7294[3] = {0xB2, 0x46, 0x70};
static signed char aid7116[3] = {0xB2, 0x17, 0x70};
static signed char aid6825[3] = {0xB2, 0xD2, 0x60};
static signed char aid6669[3] = {0xB2, 0xA7, 0x60};
static signed char aid6495[3] = {0xB2, 0x75, 0x60};
static signed char aid6234[3] = {0xB2, 0x32, 0x60};
static signed char aid5974[3] = {0xB2, 0xE8, 0x50};
static signed char aid5683[3] = {0xB2, 0xAB, 0x50};
static signed char aid5388[3] = {0xB2, 0x64, 0x50};
static signed char aid5155[3] = {0xB2, 0x1E, 0x50};
static signed char aid4767[3] = {0xB2, 0xC3, 0x40};
static signed char aid4379[3] = {0xB2, 0x5C, 0x40};
static signed char aid3998[3] = {0xB2, 0x02, 0x40};
static signed char aid3630[3] = {0xB2, 0xA1, 0x30};
static signed char aid3141[3] = {0xB2, 0x1D, 0x30};
static signed char aid2663[3] = {0xB2, 0xA5, 0x20};
static signed char aid2120[3] = {0xB2, 0x18, 0x20};
static signed char aid1518[3] = {0xB2, 0x87, 0x10};
static signed char aid1005[3] = {0xB2, 0x03, 0x10};
// end of aid sheet in rev.C opmanual


// start of aid sheet in rev.E opmanual
static signed char RErtbl2nit[10] = {0, 29, 27, 26, 21, 18, 14, 11, 7, 0};
static signed char RErtbl3nit[10] = {0, 27, 26, 24, 20, 17, 14, 11, 7, 0};
static signed char RErtbl4nit[10] = {0, 31, 24, 21, 17, 15, 14, 11, 7, 0};
static signed char RErtbl5nit[10] = {0, 28, 24, 20, 17, 15, 14, 11, 7, 0};
static signed char RErtbl6nit[10] = {0, 25, 23, 20, 16, 15, 14, 11, 7, 0};
static signed char RErtbl7nit[10] = {0, 25, 23, 19, 15, 14, 12, 11, 7, 0};
static signed char RErtbl8nit[10] = {0, 25, 21, 18, 13, 13, 12, 10, 7, 0};
static signed char RErtbl9nit[10] = {0, 22, 21, 17, 13, 12, 12, 10, 7, 0};
static signed char RErtbl10nit[10] = {0, 22, 20, 16, 12, 11, 11, 9, 7, 0};
static signed char RErtbl11nit[10] = {0, 22, 19, 15, 11, 11, 10, 9, 6, 0};
static signed char RErtbl12nit[10] = {0, 17, 19, 15, 11, 10, 10, 8, 6, 0};
static signed char RErtbl13nit[10] = {0, 17, 18, 14, 10, 10, 9, 7, 6, 0};
static signed char RErtbl14nit[10] = {0, 17, 17, 14, 10, 10, 9, 8, 6, 0};
static signed char RErtbl15nit[10] = {0, 17, 17, 14, 10, 10, 8, 8, 6, 0};
static signed char RErtbl16nit[10] = {0, 17, 16, 12, 10, 9, 8, 8, 6, 0};
static signed char RErtbl17nit[10] = {0, 17, 16, 12, 9, 9, 8, 7, 6, 0};
static signed char RErtbl19nit[10] = {0, 14, 15, 11, 8, 8, 7, 7, 6, 0};
static signed char RErtbl20nit[10] = {0, 14, 14, 11, 8, 8, 7, 7, 6, 0};
static signed char RErtbl21nit[10] = {0, 14, 14, 10, 8, 8, 7, 7, 6, 0};
static signed char RErtbl22nit[10] = {0, 14, 14, 10, 8, 8, 7, 7, 6, 0};
static signed char RErtbl24nit[10] = {0, 14, 13, 9, 7, 7, 7, 7, 6, 0};
static signed char RErtbl25nit[10] = {0, 14, 13, 8, 6, 6, 6, 7, 6, 0};
static signed char RErtbl27nit[10] = {0, 14, 12, 8, 6, 6, 6, 7, 6, 0};
static signed char RErtbl29nit[10] = {0, 11, 12, 8, 6, 6, 7, 7, 6, 0};
static signed char RErtbl30nit[10] = {0, 11, 12, 8, 6, 6, 6, 7, 6, 0};
static signed char RErtbl32nit[10] = {0, 10, 11, 8, 6, 6, 6, 6, 5, 0};
static signed char RErtbl34nit[10] = {0, 10, 11, 7, 5, 5, 5, 6, 5, 0};
static signed char RErtbl37nit[10] = {0, 10, 10, 7, 5, 5, 5, 6, 5, 0};
static signed char RErtbl39nit[10] = {0, 10, 10, 7, 5, 5, 5, 6, 5, 0};
static signed char RErtbl41nit[10] = {0, 10, 9, 7, 5, 5, 5, 5, 5, 0};
static signed char RErtbl44nit[10] = {0, 9, 9, 6, 4, 4, 5, 5, 5, 0};
static signed char RErtbl47nit[10] = {0, 7, 8, 6, 4, 4, 5, 5, 5, 0};
static signed char RErtbl50nit[10] = {0, 6, 8, 6, 4, 4, 5, 5, 5, 0};
static signed char RErtbl53nit[10] = {0, 6, 8, 5, 3, 4, 5, 5, 5, 0};
static signed char RErtbl56nit[10] = {0, 6, 7, 5, 3, 4, 4, 6, 5, 0};
static signed char RErtbl60nit[10] = {0, 4, 7, 5, 3, 4, 5, 5, 5, 0};
static signed char RErtbl64nit[10] = {0, 4, 7, 5, 3, 3, 4, 6, 5, 0};
static signed char RErtbl68nit[10] = {0, 4, 6, 5, 3, 3, 4, 5, 5, 0};
static signed char RErtbl72nit[10] = {0, 3, 6, 4, 2, 3, 4, 5, 4, 0};
static signed char RErtbl77nit[10] = {0, 3, 5, 3, 3, 2, 3, 6, 4, 0};
static signed char RErtbl82nit[10] = {0, 1, 6, 4, 3, 3, 4, 5, 3, 0};
static signed char RErtbl87nit[10] = {0, 3, 5, 4, 3, 3, 4, 4, 3, 0};
static signed char RErtbl93nit[10] = {0, 2, 7, 5, 4, 4, 5, 5, 2, 0};
static signed char RErtbl98nit[10] = {0, 1, 5, 3, 2, 2, 4, 5, 3, 0};
static signed char RErtbl105nit[10] = {0, 2, 5, 3, 3, 3, 4, 5, 2, 0};
static signed char RErtbl111nit[10] = {0, 2, 5, 3, 2, 3, 4, 4, 2, 0};
static signed char RErtbl119nit[10] = {0, 2, 4, 2, 2, 2, 4, 5, 0, 0};
static signed char RErtbl126nit[10] = {0, 3, 3, 3, 2, 2, 3, 5, 0, 0};
static signed char RErtbl134nit[10] = {0, 2, 4, 2, 2, 2, 3, 5, 1, 0};
static signed char RErtbl143nit[10] = {0, 3, 6, 3, 2, 4, 4, 5, 1, 0};
static signed char RErtbl152nit[10] = {0, 1, 3, 2, 2, 2, 5, 5, 1, 0};
static signed char RErtbl162nit[10] = {0, 1, 2, 1, 2, 2, 3, 3, 1, 0};
static signed char RErtbl172nit[10] = {0, 0, 3, 2, 2, 3, 4, 4, 2, 0};
static signed char RErtbl183nit[10] = {0, 3, 3, 1, 1, 2, 4, 4, 1, 0};
static signed char RErtbl195nit[10] = {0, 4, 2, 2, 1, 1, 4, 3, 0, 0};
static signed char RErtbl207nit[10] = {0, 0, 2, 1, 1, 1, 2, 2, 0, 0};
static signed char RErtbl220nit[10] = {0, 0, 2, 1, 1, 1, 2, 2, 0, 0};
static signed char RErtbl234nit[10] = {0, 2, 1, 1, 1, 1, 2, 2, 0, 0};
static signed char RErtbl249nit[10] = {0, 0, 1, 1, 0, 1, 2, 1, -1, 0};
static signed char RErtbl265nit[10] = {0, 0, 1, 1, 1, 0, 1, 1, 0, 0};
static signed char RErtbl282nit[10] = {0, 0, 0, 0, 0, 1, 0, 0, -1, 0};
static signed char RErtbl300nit[10] = {0, 0, 0, 0, 0, 1, 1, 0, 0, 0};
static signed char RErtbl316nit[10] = {0, 0, 0, 0, -1, 0, 0, -1, 0, 0};
static signed char RErtbl333nit[10] = {0, 0, 1, -1, -1, 0, 0, 0, 0, 0};
static signed char RErtbl360nit[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static signed char REctbl2nit[30] = {0, 0, 0, 0, 0, 0,-4, 0, 0, -4, 0, -4, -6, 1, -5, -9, 1, -4, -4, 0, -3, -3, 0, -1, -1, 0, -1, -7, 0, -6};
static signed char REctbl3nit[30] = {0, 0, 0, 0, 0, 0,0, 0, -2, -5, 0, -5, -6, 0, -6, -7, 1, -5, -5, 0, -3, -2, 0, -2, -2, 0, -1, -5, 0, -5};
static signed char REctbl4nit[30] = {0, 0, 0, 0, 0, 0,-4, 0, -5, -5, 0, -4, -7, 1, -6, -6, 1, -5, -3, 0, -2, -2, 0, -1, -2, 0, -1, -4, 0, -5};
static signed char REctbl5nit[30] = {0, 0, 0, 0, 0, 0,-5, 0, -6, -6, 1, -8, -7, 0, -5, -5, 1, -5, -3, 0, -2, -2, 0, -2, -1, 0, -1, -4, 0, -5};
static signed char REctbl6nit[30] = {0, 0, 0, 0, 0, 0,-5, 1, -7, -5, 1, -5, -5, 1, -5, -4, 1, -5, -3, 0, -3, -2, 0, -1, -1, 0, -1, -3, 0, -4};
static signed char REctbl7nit[30] = {0, 0, 0, 0, 0, 0,-6, 1, -6, -5, 0, -7, -5, 1, -5, -5, 1, -6, -3, 1, -3, -1, 0, -1, -1, 0, 0, -3, 0, -3};
static signed char REctbl8nit[30] = {0, 0, 0, 0, 0, 0,-6, 1, -9, -4, 1, -5, -5, 1, -6, -5, 1, -6, -2, 1, -2, -2, 0, -1, -1, 0, -1, -3, 0, -3};
static signed char REctbl9nit[30] = {0, 0, 0, 0, 0, 0,-5, 1, -7, -5, 1, -7, -5, 1, -5, -4, 1, -6, -3, 0, -3, -1, 0, -1, -1, 0, 0, -2, 0, -3};
static signed char REctbl10nit[30] = {0, 0, 0, 0, 0, 0,-6, 1, -8, -4, 1, -7, -3, 1, -4, -5, 1, -6, -2, 0, -2, -2, 0, -1, 0, 0, -1, -2, 0, -2};
static signed char REctbl11nit[30] = {0, 0, 0, 0, 0, 0,-7, 1, -10, -3, 1, -6, -5, 1, -7, -4, 1, -5, -3, 0, -3, -1, 0, 0, 0, 0, -1, -2, 0, -2};
static signed char REctbl12nit[30] = {0, 0, 0, 0, 0, 0,-5, 1, -8, -3, 1, -5, -3, 1, -5, -3, 1, -5, -3, 0, -2, -1, 0, -1, -1, 0, 0, -1, 0, -2};
static signed char REctbl13nit[30] = {0, 0, 0, 0, 0, 0,-6, 1, -7, -4, 1, -7, -4, 1, -7, -3, 1, -4, -2, 0, -2, -1, 0, 0, -1, 0, 0, -1, 0, -1};
static signed char REctbl14nit[30] = {0, 0, 0, 0, 0, 0,-4, 2, -9, -3, 1, -5, -5, 1, -7, -3, 1, -4, -3, 0, -2, -1, 0, -1, 0, 0, 0, -1, 0, -1};
static signed char REctbl15nit[30] = {0, 0, 0, 0, 0, 0,-4, 2, -9, -3, 1, -5, -5, 1, -7, -3, 1, -4, -3, 0, -2, -1, 0, -1, 0, 0, 0, -1, 0, -1};
static signed char REctbl16nit[30] = {0, 0, 0, 0, 0, 0,-5, 2, -9, -4, 1, -8, -3, 1, -6, -4, 1, -3, -2, 0, -2, -1, 0, -1, 0, 0, 0, -1, 0, -1};
static signed char REctbl17nit[30] = {0, 0, 0, 0, 0, 0,-5, 1, -10, -4, 1, -7, -3, 1, -7, -4, 1, -4, -3, 0, -2, -1, 0, 0, 0, 0, 0, -1, 0, -1};
static signed char REctbl19nit[30] = {0, 0, 0, 0, 0, 0,-5, 1, -10, -3, 1, -6, -2, 1, -5, -4, 0, -4, -1, 0, -2, -1, 0, 0, 0, 0, 0, -1, 0, -1};
static signed char REctbl20nit[30] = {0, 0, 0, 0, 0, 0,-3, 2, -10, -2, 1, -5, -3, 1, -5, -3, 0, -4, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1};
static signed char REctbl21nit[30] = {0, 0, 0, 0, 0, 0,-3, 1, -10, -4, 1, -8, -3, 1, -5, -4, 0, -4, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1};
static signed char REctbl22nit[30] = {0, 0, 0, 0, 0, 0,-3, 1, -9, -4, 1, -7, -2, 1, -4, -3, 1, -3, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1};
static signed char REctbl24nit[30] = {0, 0, 0, 0, 0, 0,-3, 2, -10, -5, 1, -8, -2, 1, -4, -3, 1, -3, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1};
static signed char REctbl25nit[30] = {0, 0, 0, 0, 0, 0,-1, 1, -7, -4, 1, -7, -2, 1, -4, -2, 1, -2, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1};
static signed char REctbl27nit[30] = {0, 0, 0, 0, 0, 0,-4, 1, -11, -4, 1, -7, -2, 1, -4, -2, 0, -3, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1};
static signed char REctbl29nit[30] = {0, 0, 0, 0, 0, 0,-3, 1, -10, -3, 1, -6, -3, 0, -4, -3, 1, -3, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1};
static signed char REctbl30nit[30] = {0, 0, 0, 0, 0, 0,-3, 1, -10, -3, 1, -5, -3, 0, -4, -3, 0, -3, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0};
static signed char REctbl32nit[30] = {0, 0, 0, 0, 0, 0,-3, 1, -10, -3, 1, -4, -2, 0, -3, -2, 0, -2, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static signed char REctbl34nit[30] = {0, 0, 0, 0, 0, 0,-3, 1, -10, -3, 1, -7, -3, 0, -3, -3, 0, -3, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static signed char REctbl37nit[30] = {0, 0, 0, 0, 0, 0,-1, 2, -10, -3, 1, -6, -2, 0, -2, -3, 0, -3, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static signed char REctbl39nit[30] = {0, 0, 0, 0, 0, 0,-3, 1, -11, -3, 0, -6, -3, 0, -3, -2, 0, -2, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static signed char REctbl41nit[30] = {0, 0, 0, 0, 0, 0,-1, 2, -12, -4, 0, -6, -2, 0, -2, -2, 0, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static signed char REctbl44nit[30] = {0, 0, 0, 0, 0, 0,-2, 2, -9, -3, 0, -4, -2, 0, -2, -2, 0, -3, -1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0};
static signed char REctbl47nit[30] = {0, 0, 0, 0, 0, 0,-4, 2, -12, -3, 0, -4, -2, 0, -2, -1, 0, -2, -1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0};
static signed char REctbl50nit[30] = {0, 0, 0, 0, 0, 0,-3, 2, -10, -3, 0, -3, -2, 0, -2, -2, 0, -2, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0};
static signed char REctbl53nit[30] = {0, 0, 0, 0, 0, 0,-3, 1, -10, -3, 0, -5, -2, 0, -2, -2, 0, -2, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0};
static signed char REctbl56nit[30] = {0, 0, 0, 0, 0, 0,-2, 2, -11, -4, 0, -5, -2, 0, -2, -1, 0, -1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0};
static signed char REctbl60nit[30] = {0, 0, 0, 0, 0, 0,-3, 1, -10, -3, 0, -4, -2, 0, -2, -1, 0, -1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0};
static signed char REctbl64nit[30] = {0, 0, 0, 0, 0, 0,-3, 1, -9, -3, 0, -4, 0, 0, -1, -1, 0, -2, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0};
static signed char REctbl68nit[30] = {0, 0, 0, 0, 0, 0,-4, 2, -10, -2, 0, -3, -1, 0, -2, 0, 0, -1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0};
static signed char REctbl72nit[30] = {0, 0, 0, 0, 0, 0,-4, 1, -10, -2, 0, -3, -1, 0, -2, 0, 0, -2, 0, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0};
static signed char REctbl77nit[30] = {0, 0, 0, 0, 0, 0,-2, 2, -9, -3, 0, -4, 0, 0, -1, -1, 0, -2, 1, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0};
static signed char REctbl82nit[30] = {0, 0, 0, 0, 0, 0,-3, 1, -8, -2, 0, -2, -2, 0, -1, 0, 0, -1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0};
static signed char REctbl87nit[30] = {0, 0, 0, 0, 0, 0,-4, 1, -10, -3, 0, -3, -1, 0, -1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0};
static signed char REctbl93nit[30] = {0, 0, 0, 0, 0, 0,-1, 1, -10, -2, 0, -4, -1, 0, -2, 0, 0, -1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0};
static signed char REctbl98nit[30] = {0, 0, 0, 0, 0, 0,-4, 1, -7, -2, 0, -2, -1, 0, -1, 0, 0, 0, 1, 0, 0, 1, 0, 2, 0, 0, 0, 0, 0, 0};
static signed char REctbl105nit[30] = {0, 0, 0, 0, 0, 0,-3, 1, -8, -2, 0, -2, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0};
static signed char REctbl111nit[30] = {0, 0, 0, 0, 0, 0,-3, 1, -7, -2, 0, -2, -1, 0, -1, -1, 0, -1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0};
static signed char REctbl119nit[30] = {0, 0, 0, 0, 0, 0,-3, 1, -6, -2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0};
static signed char REctbl126nit[30] = {0, 0, 0, 0, 0, 0,-3, 2, -7, -1, 0, -1, -1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0};
static signed char REctbl134nit[30] = {0, 0, 0, 0, 0, 0,-2, 1, -5, -1, 0, -1, -1, 0, 0, 0, 0, -1, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0};
static signed char REctbl143nit[30] = {0, 0, 0, 0, 0, 0,-1, 1, -7, -1, 0, -2, 0, 0, -1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static signed char REctbl152nit[30] = {0, 0, 0, 0, 0, 0,-4, 1, -6, -1, 0, -2, 0, 0, 0, 0, 0, -1, 1, 0, 1, -1, 0, 0, 0, 0, 0, 0, 0, 0};
static signed char REctbl162nit[30] = {0, 0, 0, 0, 0, 0,-4, 1, -6, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static signed char REctbl172nit[30] = {0, 0, 0, 0, 0, 0,-3, 1, -5, 0, 0, 0, -1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static signed char REctbl183nit[30] = {0, 0, 0, 0, 0, 0,-3, 1, -3, -1, 0, -1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, -1, 0, 0, 0};
static signed char REctbl195nit[30] = {0, 0, 0, 0, 0, 0,-3, 1, -4, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 1, -1, 0, 0, 0, 0, 0, 0, 0, 0};
static signed char REctbl207nit[30] = {0, 0, 0, 0, 0, 0,-1, 1, -3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static signed char REctbl220nit[30] = {0, 0, 0, 0, 0, 0,-1, 1, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static signed char REctbl234nit[30] = {0, 0, 0, 0, 0, 0,-1, 1, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static signed char REctbl249nit[30] = {0, 0, 0, 0, 0, 0,0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static signed char REctbl265nit[30] = {0, 0, 0, 0, 0, 0,0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static signed char REctbl282nit[30] = {0, 0, 0, 0, 0, 0,0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static signed char REctbl300nit[30] = {0, 0, 0, 0, 0, 0,0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static signed char REctbl316nit[30] = {0, 0, 0, 0, 0, 0,0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static signed char REctbl333nit[30] = {0, 0, 0, 0, 0, 0,0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static signed char REctbl360nit[30] = {0, 0, 0, 0, 0, 0,0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};


static signed char aid9825[3] = {0xB2, 0xE3, 0x90};
static signed char aid9713[3] = {0xB2, 0xC6, 0x90};
static signed char aid9635[3] = {0xB2, 0xB2, 0x90};
static signed char aid9523[3] = {0xB2, 0x95, 0x90};
static signed char aid9445[3] = {0xB2, 0x81, 0x90};
static signed char aid9344[3] = {0xB2, 0x67, 0x90};
static signed char aid9270[3] = {0xB2, 0x54, 0x90};
static signed char aid9200[3] = {0xB2, 0x42, 0x90};
static signed char aid9130[3] = {0xB2, 0x30, 0x90};
static signed char aid9030[3] = {0xB2, 0x16, 0x90};
static signed char aid8964[3] = {0xB2, 0x05, 0x90};
static signed char aid8894[3] = {0xB2, 0xF3, 0x80};
static signed char aid8832[3] = {0xB2, 0xE3, 0x80};
static signed char aid8723[3] = {0xB2, 0xC7, 0x80};
static signed char aid8653[3] = {0xB2, 0xB5, 0x80};
static signed char aid8575[3] = {0xB2, 0xA1, 0x80};
static signed char aid8393[3] = {0xB2, 0x72, 0x80};
static signed char aid8284[3] = {0xB2, 0x56, 0x80};
static signed char aid8218[3] = {0xB2, 0x45, 0x80};
static signed char aid8148[3] = {0xB2, 0x33, 0x80};
static signed char aid7974[3] = {0xB2, 0x06, 0x80};
static signed char aid7896[3] = {0xB2, 0xF2, 0x70};
static signed char aid7717[3] = {0xB2, 0xC4, 0x70};
static signed char aid7535[3] = {0xB2, 0x95, 0x70};
static signed char aid7469[3] = {0xB2, 0x84, 0x70};
static signed char aid7290[3] = {0xB2, 0x56, 0x70};
static signed char aid7104[3] = {0xB2, 0x26, 0x70};
static signed char aid6848[3] = {0xB2, 0xE4, 0x60};
// static signed char aid6669[3] = {0xB2, 0xB6, 0x60}; defined in rev.C area
static signed char aid6491[3] = {0xB2, 0x88, 0x60};
static signed char aid6231[3] = {0xB2, 0x45, 0x60};
static signed char aid5947[3] = {0xB2, 0xFC, 0x50};
static signed char aid5675[3] = {0xB2, 0xB6, 0x50};
static signed char aid5419[3] = {0xB2, 0x74, 0x50};
static signed char aid5140[3] = {0xB2, 0x2C, 0x50};
static signed char aid4752[3] = {0xB2, 0xC8, 0x40};
static signed char aid4383[3] = {0xB2, 0x69, 0x40};
static signed char aid4006[3] = {0xB2, 0x08, 0x40};
static signed char aid3622[3] = {0xB2, 0xA5, 0x30};
static signed char aid3133[3] = {0xB2, 0x27, 0x30};
static signed char aid2620[3] = {0xB2, 0xA3, 0x20};
static signed char aid2158[3] = {0xB2, 0x2C, 0x20};
static signed char aid1611[3] = {0xB2, 0x9F, 0x10};
// static signed char aid1005[3] = {0xB2, 0x03, 0x10}; defined in rev.C area

// end of aid sheet in rev.E opmanual


static unsigned char elv1[3] = {
	0xB6, 0x8C, 0x16
};
static unsigned char elv2[3] = {
	0xB6, 0x8C, 0x15
};
static unsigned char elv3[3] = {
	0xB6, 0x8C, 0x14
};
static unsigned char elv4[3] = {
	0xB6, 0x8C, 0x13
};
static unsigned char elv5[3] = {
	0xB6, 0x8C, 0x12
};
static unsigned char elv6[3] = {
	0xB6, 0x8C, 0x11
};
static unsigned char elv7[3] = {
	0xB6, 0x8C, 0x10
};
static unsigned char elv8[3] = {
	0xB6, 0x8C, 0x0F
};
static unsigned char elv9[3] = {
	0xB6, 0x8C, 0x0E
};
static unsigned char elv10[3] = {
	0xB6, 0x8C, 0x0D
};
static unsigned char elv11[3] = {
	0xB6, 0x8C, 0x0C
};
static unsigned char elv12[3] = {
	0xB6, 0x8C, 0x0B
};
static unsigned char elv13[3] = {
	0xB6, 0x8C, 0x0A
};
// hbm interpolation
static unsigned char elv14[3]  = {
	0xB6, 0x8C, 0x09
};
static unsigned char elv15[3]  = {
	0xB6, 0x8C, 0x08
};
static unsigned char elv16[3]  = {
	0xB6, 0x8C, 0x06
};
static unsigned char elv17[3]  = {
	0xB6, 0x8C, 0x04
};
static unsigned char elv18[3]  = {
	0xB6, 0x8C, 0x02
};
static unsigned char elv19[3]  = {
	0xB6, 0x8C, 0x00
};


static unsigned char elvCaps1[3]  = {
	0xB6, 0x9C, 0x16
};
static unsigned char elvCaps2[3]  = {
	0xB6, 0x9C, 0x15
};
static unsigned char elvCaps3[3]  = {
	0xB6, 0x9C, 0x14
};
static unsigned char elvCaps4[3]  = {
	0xB6, 0x9C, 0x13
};
static unsigned char elvCaps5[3]  = {
	0xB6, 0x9C, 0x12
};
static unsigned char elvCaps6[3]  = {
	0xB6, 0x9C, 0x11
};
static unsigned char elvCaps7[3]  = {
	0xB6, 0x9C, 0x10
};
static unsigned char elvCaps8[3]  = {
	0xB6, 0x9C, 0x0F
};
static unsigned char elvCaps9[3]  = {
	0xB6, 0x9C, 0x0E
};
static unsigned char elvCaps10[3]  = {
	0xB6, 0x9C, 0x0D
};
static unsigned char elvCaps11[3]  = {
	0xB6, 0x9C, 0x0C
};
static unsigned char elvCaps12[3]  = {
	0xB6, 0x9C, 0x0B
};
static unsigned char elvCaps13[3]  = {
	0xB6, 0x9C, 0x0A
};

// hbm interpolation
static unsigned char elvCaps14[3]  = {
	0xB6, 0x9C, 0x09
};
static unsigned char elvCaps15[3]  = {
	0xB6, 0x9C, 0x08
};
static unsigned char elvCaps16[3]  = {
	0xB6, 0x9C, 0x06
};
static unsigned char elvCaps17[3]  = {
	0xB6, 0x9C, 0x04
};
static unsigned char elvCaps18[3]  = {
	0xB6, 0x9C, 0x02
};
static unsigned char elvCaps19[3]  = {
	0xB6, 0x9C, 0x00
};


#ifdef CONFIG_LCD_HMT

static signed char HMTrtbl10nit[10] = {0, 8, 10, 8, 6, 4, 2, 2, 1, 0};
static signed char HMTrtbl11nit[10] = {0, 13, 9, 7, 5, 4, 2, 2, 0, 0};
static signed char HMTrtbl12nit[10] = {0, 11, 9, 7, 5, 4, 2, 2, 0, 0};
static signed char HMTrtbl13nit[10] = {0, 11, 9, 7, 5, 3, 2, 3, 0, 0};
static signed char HMTrtbl14nit[10] = {0, 13, 9, 7, 5, 4, 3, 3, 1, 0};
static signed char HMTrtbl15nit[10] = {0, 9, 10, 7, 6, 4, 4, 4, 1, 0};
static signed char HMTrtbl16nit[10] = {0, 10, 10, 7, 5, 5, 3, 3, 1, 0};
static signed char HMTrtbl17nit[10] = {0, 8, 9, 7, 5, 4, 3, 3, 1, 0};
static signed char HMTrtbl19nit[10] = {0, 11, 9, 7, 5, 4, 3, 3, 2, 0};
static signed char HMTrtbl20nit[10] = {0, 10, 9, 7, 5, 5, 4, 3, 2, 0};
static signed char HMTrtbl21nit[10] = {0, 9, 9, 7, 5, 4, 4, 4, 3, 0};
static signed char HMTrtbl22nit[10] = {0, 9, 9, 6, 5, 4, 3, 3, 4, 0};
static signed char HMTrtbl23nit[10] = {0, 9, 9, 6, 5, 4, 3, 3, 3, 0};
static signed char HMTrtbl25nit[10] = {0, 12, 9, 7, 5, 5, 4, 4, 4, 0};
static signed char HMTrtbl27nit[10] = {0, 7, 9, 6, 5, 4, 5, 4, 3, 0};
static signed char HMTrtbl29nit[10] = {0, 10, 9, 7, 5, 5, 5, 7, 5, 0};
static signed char HMTrtbl31nit[10] = {0, 8, 9, 7, 5, 5, 5, 5, 4, 0};
static signed char HMTrtbl33nit[10] = {0, 10, 9, 6, 6, 5, 6, 7, 4, 0};
static signed char HMTrtbl35nit[10] = {0, 11, 9, 7, 6, 6, 6, 6, 4, 0};
static signed char HMTrtbl37nit[10] = {0, 6, 9, 6, 5, 5, 6, 6, 2, 0};
static signed char HMTrtbl39nit[10] = {0, 6, 9, 6, 5, 5, 6, 5, 2, 0};
static signed char HMTrtbl41nit[10] = {0, 11, 9, 7, 6, 6, 6, 6, 3, 0};
static signed char HMTrtbl44nit[10] = {0, 6, 10, 7, 6, 5, 5, 5, 1, 0};
static signed char HMTrtbl47nit[10] = {0, 8, 10, 7, 5, 5, 5, 5, 1, 0};
static signed char HMTrtbl50nit[10] = {0, 8, 10, 7, 6, 6, 6, 5, 0, 0};
static signed char HMTrtbl53nit[10] = {0, 9, 9, 7, 6, 6, 6, 6, 0, 0};
static signed char HMTrtbl56nit[10] = {0, 11, 9, 7, 5, 6, 6, 6, 2, 0};
static signed char HMTrtbl60nit[10] = {0, 11, 9, 7, 5, 6, 6, 5, 1, 0};
static signed char HMTrtbl64nit[10] = {0, 7, 10, 7, 6, 6, 6, 5, 1, 0};
static signed char HMTrtbl68nit[10] = {0, 7, 9, 7, 6, 5, 5, 5, 1, 0};
static signed char HMTrtbl72nit[10] = {0, 7, 9, 6, 5, 5, 5, 6, 2, 0};
static signed char HMTrtbl77nit[10] = {0, 6, 5, 4, 3, 3, 3, 4, -1, 0};
static signed char HMTrtbl82nit[10] = {0, 3, 6, 4, 3, 3, 5, 5, 0, 0};
static signed char HMTrtbl87nit[10] = {0, 1, 6, 4, 3, 2, 4, 5, 0, 0};
static signed char HMTrtbl93nit[10] = {0, 3, 5, 3, 2, 3, 5, 4, 2, 0};
static signed char HMTrtbl99nit[10] = {0, 3, 5, 3, 3, 3, 4, 4, 1, 0};
static signed char HMTrtbl105nit[10] = {0, 6, 5, 3, 3, 3, 4, 4, 1, 0};


static signed char HMTctbl10nit[30] = {0, 0, 0, 0, 0, 0,-3, 3, -6, -3, 3, -7, -3, 2, -5, -4, 2, -5, -1, 2, -4, -1, 0, -2, -1, 0, 0, 0, 0, -1};
static signed char HMTctbl11nit[30] = {0, 0, 0, 0, 0, 0,-6, 2, -6, -3, 3, -7, -4, 2, -6, -3, 2, -5, -2, 1, -4, -2, 0, -2, 0, 0, -1, 0, 0, 0};
static signed char HMTctbl12nit[30] = {0, 0, 0, 0, 0, 0,-8, 3, -6, -4, 3, -7, -3, 3, -6, -3, 2, -4, -1, 1, -3, -1, 0, -2, 0, 0, 0, 0, 0, 0};
static signed char HMTctbl13nit[30] = {0, 0, 0, 0, 0, 0,-7, 2, -6, -3, 4, -9, -3, 2, -4, -2, 2, -5, -1, 1, -3, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static signed char HMTctbl14nit[30] = {0, 0, 0, 0, 0, 0,-6, 4, -8, -2, 3, -6, -3, 2, -5, -2, 2, -5, -1, 1, -3, -1, 0, -1, 0, 0, -1, 0, 0, 0};
static signed char HMTctbl15nit[30] = {0, 0, 0, 0, 0, 0,-7, 3, -7, -5, 3, -8, -2, 2, -4, -2, 2, -4, -1, 1, -4, -2, 0, -2, 0, 0, 0, 0, 0, 0};
static signed char HMTctbl16nit[30] = {0, 0, 0, 0, 0, 0,-6, 2, -6, -3, 3, -8, -4, 2, -5, -3, 2, -5, -1, 1, -4, -1, 0, -1, 0, 0, 0, 0, 0, 0};
static signed char HMTctbl17nit[30] = {0, 0, 0, 0, 0, 0,-4, 2, -6, -3, 3, -7, -4, 2, -5, -2, 2, -5, 0, 1, -3, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static signed char HMTctbl19nit[30] = {0, 0, 0, 0, 0, 0,-7, 3, -8, -2, 3, -8, -5, 1, -4, -3, 2, -5, 0, 1, -2, -1, 0, -1, 0, 0, 0, 0, 0, 0};
static signed char HMTctbl20nit[30] = {0, 0, 0, 0, 0, 0,-6, 3, -8, -4, 3, -8, -3, 2, -4, -2, 2, -4, -1, 1, -3, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static signed char HMTctbl21nit[30] = {0, 0, 0, 0, 0, 0,-6, 4, -8, -3, 3, -8, -4, 2, -4, -2, 2, -5, -1, 1, -3, 0, 0, 0, -1, 0, 0, 0, 0, 0};
static signed char HMTctbl22nit[30] = {0, 0, 0, 0, 0, 0,-5, 4, -8, -5, 3, -8, -2, 2, -4, -1, 2, -4, -2, 1, -2, -1, 0, -1, 0, 0, 0, 0, 0, 0};
static signed char HMTctbl23nit[30] = {0, 0, 0, 0, 0, 0,-4, 4, -8, -4, 3, -8, -3, 2, -4, -1, 2, -4, -2, 1, -2, 0, 0, -1, 0, 0, 0, 1, 0, 0};
static signed char HMTctbl25nit[30] = {0, 0, 0, 0, 0, 0,-8, 4, -10, -4, 3, -7, -3, 1, -4, 0, 2, -4, -2, 1, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static signed char HMTctbl27nit[30] = {0, 0, 0, 0, 0, 0,-4, 4, -8, -4, 3, -7, -4, 1, -4, -1, 1, -4, -1, 1, -2, -1, 0, 0, -1, 0, 0, 1, 0, 0};
static signed char HMTctbl29nit[30] = {0, 0, 0, 0, 0, 0,-6, 4, -10, -3, 2, -6, -4, 1, -4, 0, 2, -4, -2, 0, -2, 0, 0, 0, -1, 0, 0, 1, 0, 0};
static signed char HMTctbl31nit[30] = {0, 0, 0, 0, 0, 0,-5, 4, -9, -3, 3, -6, -3, 1, -4, -1, 2, -4, -1, 0, -1, 0, 0, -1, 0, 0, 0, 1, 0, 1};
static signed char HMTctbl33nit[30] = {0, 0, 0, 0, 0, 0,-4, 3, -8, -3, 3, -7, -2, 1, -3, -1, 1, -4, -1, 1, -2, 0, 0, 0, 0, 0, 0, 1, 0, 0};
static signed char HMTctbl35nit[30] = {0, 0, 0, 0, 0, 0,-4, 4, -10, -4, 3, -6, -2, 1, -3, -1, 1, -4, -2, 0, -2, -1, 0, 0, 0, 0, 0, 1, 0, 1};
static signed char HMTctbl37nit[30] = {0, 0, 0, 0, 0, 0,-3, 3, -8, -3, 3, -6, -4, 1, -4, -2, 1, -4, -1, 0, -1, 0, 0, -1, 0, 0, 0, 1, 0, 0};
static signed char HMTctbl39nit[30] = {0, 0, 0, 0, 0, 0,-5, 4, -9, -4, 2, -6, -3, 1, -4, -1, 1, -3, -1, 0, -2, -1, 0, -1, 0, 0, 0, 2, 0, 2};
static signed char HMTctbl41nit[30] = {0, 0, 0, 0, 0, 0,-4, 4, -10, -3, 3, -6, -3, 1, -4, -2, 1, -4, 0, 0, -1, 0, 0, -1, 0, 0, 0, 1, 0, 1};
static signed char HMTctbl44nit[30] = {0, 0, 0, 0, 0, 0,-3, 4, -9, -4, 2, -6, -3, 1, -4, 0, 1, -3, -1, 0, -2, 0, 0, -1, 0, 0, 0, 2, 0, 2};
static signed char HMTctbl47nit[30] = {0, 0, 0, 0, 0, 0,-4, 4, -9, -4, 2, -6, -2, 1, -4, -3, 1, -4, 0, 0, -1, 0, 0, -1, 0, 0, 0, 1, 0, 1};
static signed char HMTctbl50nit[30] = {0, 0, 0, 0, 0, 0,-3, 4, -8, -5, 2, -6, -2, 2, -4, -1, 1, -3, -1, 0, -2, -1, 0, -1, 0, 0, 0, 2, 0, 2};
static signed char HMTctbl53nit[30] = {0, 0, 0, 0, 0, 0,-3, 4, -9, -4, 2, -6, -3, 1, -4, 0, 1, -2, -1, 0, -1, -1, 0, -1, 0, 0, 0, 2, 0, 1};
static signed char HMTctbl56nit[30] = {0, 0, 0, 0, 0, 0,-4, 5, -10, -2, 1, -4, -2, 2, -4, -2, 1, -2, -1, 0, -2, 0, 0, -1, 0, 0, 0, 0, 0, 1};
static signed char HMTctbl60nit[30] = {0, 0, 0, 0, 0, 0,-4, 5, -10, -2, 2, -5, -2, 1, -4, -1, 1, -2, -1, 0, -2, 0, 0, -1, 0, 0, 0, 1, 0, 0};
static signed char HMTctbl64nit[30] = {0, 0, 0, 0, 0, 0,-3, 4, -8, -3, 2, -5, -2, 1, -4, -1, 0, -2, -1, 0, -2, 0, 0, 0, -1, 0, 0, 1, 0, 1};
static signed char HMTctbl68nit[30] = {0, 0, 0, 0, 0, 0,-4, 4, -9, -1, 2, -4, -2, 1, -4, -1, 0, -2, 0, 0, -1, 0, 0, 0, 0, 0, 0, 1, 0, 0};
static signed char HMTctbl72nit[30] = {0, 0, 0, 0, 0, 0,-3, 4, -8, -2, 2, -4, -1, 1, -4, -2, 1, -2, -1, 0, -2, 0, 0, -1, 0, 0, 0, 1, 0, 1};
static signed char HMTctbl77nit[30] = {0, 0, 0, 0, 0, 0,-1, 4, -9, -2, 1, -4, -2, 0, -2, 0, 1, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2};
static signed char HMTctbl82nit[30] = {0, 0, 0, 0, 0, 0,-1, 3, -8, -2, 2, -4, -2, 1, -3, 0, 0, -2, -1, 0, -1, 0, 0, -1, 0, 0, 1, 1, 0, 1};
static signed char HMTctbl87nit[30] = {0, 0, 0, 0, 0, 0,-1, 3, -8, -2, 2, -4, -1, 1, -3, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1};
static signed char HMTctbl93nit[30] = {0, 0, 0, 0, 0, 0,-1, 3, -8, -2, 2, -4, -1, 1, -2, -1, 1, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 2};
static signed char HMTctbl99nit[30] = {0, 0, 0, 0, 0, 0,0, 3, -7, -2, 1, -4, 0, 1, -3, 0, 1, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2};
static signed char HMTctbl105nit[30] = {0, 0, 0, 0, 0, 0,-1, 4, -8, -4, 1, -4, 0, 1, -3, -1, 1, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 3};


static unsigned char HMTaid8001[4] = {
	0xB2, 0x03, 0x12, 0x03
};

static unsigned char HMTaid7003[4] = {
	0xB2, 0x03, 0x13, 0x04
};

static unsigned char HMTelv[3] = {
	0xB6, 0x8C, 0x0A
};
static unsigned char HMTelvCaps[3] = {
	0xB6, 0x9C, 0x0A
};

#endif

#endif
