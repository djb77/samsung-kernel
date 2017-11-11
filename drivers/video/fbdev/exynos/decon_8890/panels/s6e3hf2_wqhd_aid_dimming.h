/* linux/drivers/video/exynos_decon/panel/s6e3hf2_wqhd_aid_dimming.h
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

#ifndef __S6E3HF2_WQHD_AID_DIMMING_H__
#define __S6E3HF2_WQHD_AID_DIMMING_H__


static signed char rtbl2nit[10] = {
	0, 4, 24, 27, 23, 19, 17, 12, 8, 0
};

static signed char rtbl3nit[10] = {
	0, 4, 24, 21, 19, 15, 14, 9, 6, 0
};

static signed char rtbl4nit[10] = {
	0, 13, 14, 17, 14, 11, 10, 6, 5, 0
};

static signed char rtbl5nit[10] = {
	0, 0, 1, 15, 13, 10, 9, 6, 5, 0
};

static signed char rtbl6nit[10] = {
	0, 12, 17, 13, 11, 8, 8, 6, 5, 0
};

static signed char rtbl7nit[10] = {
	0, 20, 16, 12, 10, 8, 7, 5, 4, 0
};

static signed char rtbl8nit[10] = {
	0, 0, 16, 11, 9, 7, 7, 4, 4, 0
};

static signed char rtbl9nit[10] = {
	0, 17, 14, 10, 8, 6, 6, 4, 4, 0
};

static signed char rtbl10nit[10] = {
	0, 15, 14, 9, 8, 6, 5, 4, 4, 0
};

static signed char rtbl11nit[10] = {
	0, 14, 13, 9, 7, 5, 5, 3, 3, 0
};

static signed char rtbl12nit[10] = {
	0, 15, 12, 8, 7, 4, 4, 3, 3, 0
};

static signed char rtbl13nit[10] = {
	0, 12, 12, 8, 6, 4, 4, 3, 3, 0
};

static signed char rtbl14nit[10] = {
	0, 0, 12, 7, 6, 3, 4, 3, 3, 0
};

static signed char rtbl15nit[10] = {
	0, 11, 11, 7, 5, 3, 4, 3, 3, 0
};

static signed char rtbl16nit[10] = {
	0, 8, 11, 7, 5, 3, 3, 2, 2, 0
};

static signed char rtbl17nit[10] = {
	0, 8, 10, 6, 5, 3, 3, 3, 3, 0
};

static signed char rtbl19nit[10] = {
	0, 9, 10, 6, 4, 2, 3, 2, 3, 0
};

static signed char rtbl20nit[10] = {
	0, 11, 9, 6, 4, 2, 3, 2, 3, 0
};

static signed char rtbl21nit[10] = {
	0, 9, 9, 5, 4, 2, 3, 2, 3, 0
};

static signed char rtbl22nit[10] = {
	0, 5, 9, 5, 4, 2, 3, 2, 3, 0
};

static signed char rtbl24nit[10] = {
	0, 8, 8, 5, 3, 2, 2, 1, 3, 0
};

static signed char rtbl25nit[10] = {
	0, 7, 8, 4, 3, 2, 2, 2, 3, 0
};

static signed char rtbl27nit[10] = {
	0, 11, 7, 4, 3, 2, 2, 2, 3, 0
};

static signed char rtbl29nit[10] = {
	0, 7, 7, 4, 2, 1, 2, 1, 2, 0
};

static signed char rtbl30nit[10] = {
	0, 6, 7, 4, 2, 1, 2, 1, 2, 0
};

static signed char rtbl32nit[10] = {
	0, 8, 6, 4, 2, 1, 2, 2, 3, 0
};

static signed char rtbl34nit[10] = {
	0, 6, 6, 3, 2, 1, 2, 1, 2, 0
};

static signed char rtbl37nit[10] = {
	0, 8, 5, 3, 2, 1, 2, 1, 3, 0
};

static signed char rtbl39nit[10] = {
	0, 6, 5, 3, 1, 1, 2, 1, 2, 0
};

static signed char rtbl41nit[10] = {
	0, 5, 5, 3, 2, 1, 1, 1, 3, 0
};

static signed char rtbl44nit[10] = {
	0, 7, 4, 3, 1, 1, 1, 1, 3, 0
};

static signed char rtbl47nit[10] = {
	0, 5, 4, 2, 1, 0, 1, 1, 3, 0
};

static signed char rtbl50nit[10] = {
	0, 3, 4, 2, 1, 1, 1, 1, 3, 0
};

static signed char rtbl53nit[10] = {
	0, 1, 4, 2, 1, 1, 1, 1, 3, 0
};

static signed char rtbl56nit[10] = {
	0, 6, 3, 2, 1, 1, 1, 1, 3, 0
};

static signed char rtbl60nit[10] = {
	0, 3, 3, 2, 1, 0, 1, 1, 2, 0
};

static signed char rtbl64nit[10] = {
	0, 1, 3, 2, 1, 0, 1, 1, 3, 0
};

static signed char rtbl68nit[10] = {
	0, 5, 2, 1, 1, 0, 1, 1, 2, 0
};

static signed char rtbl72nit[10] = {
	0, 2, 2, 1, 0, 0, 0, 0, 2, 0
};

static signed char rtbl77nit[10] = {
	0, 1, 2, 1, 0, 0, 1, 1, 3, 0
};

static signed char rtbl82nit[10] = {
	0, 1, 2, 1, 0, 0, 1, 2, 1, 0
};

static signed char rtbl87nit[10] = {
	0, 2, 2, 2, 0, 0, 1, 1, 1, 0
};

static signed char rtbl93nit[10] = {
	0, 4, 1, 1, 0, -1, 1, 2, 1, 0
};

static signed char rtbl98nit[10] = {
	0, 0, 2, 1, 0, -1, 0, 1, 0, 0
};

static signed char rtbl105nit[10] = {
	0, 2, 2, 1, 1, -1, 0, 1, -1, 0
};

static signed char rtbl110nit[10] = {
	0, 1, 2, 1, 0, 0, 1, 2, -1, 0
};

static signed char rtbl119nit[10] = {
	0, 1, 1, 1, 0, 0, 2, 2, -1, 0
};

static signed char rtbl126nit[10] = {
	0, 1, 1, 1, 1, -1, 1, 1, -2, 0
};

static signed char rtbl134nit[10] = {
	0, 1, 1, 0, 0, 0, 2, 2, -1, 0
};

static signed char rtbl143nit[10] = {
	0, 0, 1, 1, 0, -1, 1, 2, -1, 0
};

static signed char rtbl152nit[10] = {
	0, 0, 1, 0, 0, 0, 1, 2, -1, 0
};

static signed char rtbl162nit[10] = {
	0, -1, 1, 1, 0, 0, 1, 2, 0, 0
};

static signed char rtbl172nit[10] = {
	0, -1, 1, 0, 0, 0, 1, 2, 1, 0
};

static signed char rtbl183nit[10] = {
	0, -1, 1, 0, -1, -1, 1, 1, 1, 0
};

static signed char rtbl195nit[10] = {
	0, 0, 0, 0, -1, -1, 1, 1, 1, 0
};

static signed char rtbl207nit[10] = {
	0, 0, 0, 0, -1, -1, 0, 1, 1, 0
};

static signed char rtbl220nit[10] = {
	0, -1, 0, 0, -1, -1, 0, 1, 1, 0
};

static signed char rtbl234nit[10] = {
	0, -1, 0, 0, -1, -1, 0, 1, 0, 0
};

static signed char rtbl249nit[10] = {
	0, 0, 0, 0, -1, -1, 0, 1, 1, 0
};

static signed char rtbl265nit[10] = {
	0, 0, 0, 0, 0, -1, 0, 1, 1, 0
};

static signed char rtbl282nit[10] = {
	0, 0, -1, 0, 0, -1, -1, -1, 1, 0
};

static signed char rtbl300nit[10] = {
	0, 0, 0, -1, 0, 0, 0, 0, 1, 0
};

static signed char rtbl316nit[10] = {
	0, -1, 0, -1, -1, -1, 0, 0, 2, 0
};

static signed char rtbl333nit[10] = {
	0, 0, 0, -1, -1, -1, -1, 0, 2, 0
};

static signed char rtbl360nit[10] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};


static signed char ctbl2nit[30] = {
	0, 0, 0, 0, 0, 0, 5, 4, -10, 0, 4, -7, -12, 4, -10, -13, 4, -8, -12, 0, 0, -3, 0, 0, -3, 0, -2, -2, 0, -2
};

static signed char ctbl3nit[30] = {
	0, 0, 0, 0, 0, 0, 5, 4, -10, 0, 3, -7, -4, 3, -6, -19, 3, -7, -7, 0, 0, -2, 0, -1, -1, 0, 0, 0, 0, -1
};

static signed char ctbl4nit[30] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, -7, 3, -8, -13, 3, -8, -11, 1, -3, -5, 0, -1, -3, 0, -1, 0, 0, 0, 1, 0, 0
};

static signed char ctbl5nit[30] = {
	0, 0, 0, 0, 0, 0, 3, 1, -3, -3, 3, -11, -13, 3, -7, -10, 1, -4, -4, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0
};

static signed char ctbl6nit[30] = {
	0, 0, 0, 0, 0, 0, 1, 6, -12, -7, 3, -11, -6, 3, -6, -12, 1, -3, -4, 0, 0, -1, 0, 0, 0, 0, 0, 1, 0, 0
};

static signed char ctbl7nit[30] = {
	0, 0, 0, 0, 0, 0, 1, 8, -17, -11, 3, -11, -10, 3, -6, -9, 0, -2, -3, 0, 0, -1, 0, 0, 1, 0, 0, 1, 0, 1
};

static signed char ctbl8nit[30] = {
	0, 0, 0, 0, 0, 0, -4, 10, -21, -9, 3, -10, -10, 3, -7, -8, 0, -2, -3, 0, 0, -1, 0, 0, 0, 0, 0, 1, 0, 1
};

static signed char ctbl9nit[30] = {
	0, 0, 0, 0, 0, 0, 0, 11, -22, -12, 3, -8, -11, 3, -7, -5, 0, -1, -2, 0, 0, -1, 0, 1, 1, 0, 0, 1, 0, 1
};

static signed char ctbl10nit[30] = {
	0, 0, 0, 0, 0, 0, 0, 10, -21, -11, 3, -10, -11, 3, -7, -3, 0, 0, -2, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1
};

static signed char ctbl11nit[30] = {
	0, 0, 0, 0, 0, 0, 0, 10, -22, -11, 2, -12, -8, 2, -6, -6, 0, -1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 2, 0, 1
};

static signed char ctbl12nit[30] = {
	0, 0, 0, 0, 0, 0, 1, 12, -26, -12, 2, -9, -5, 2, -5, -6, 0, -1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 2, 0, 1
};

static signed char ctbl13nit[30] = {
	0, 0, 0, 0, 0, 0, 0, 10, -21, -8, 2, -8, -6, 2, -4, -5, 0, -1, -1, 0, 1, 0, 0, 1, 1, 0, 0, 2, 0, 1
};

static signed char ctbl14nit[30] = {
	0, 0, 0, 0, 0, 0, 0, 7, -14, -8, 1, -14, -6, 1, -4, -4, 0, 0, -1, 0, 1, 1, 0, 1, 0, 0, 0, 2, 0, 1
};

static signed char ctbl15nit[30] = {
	0, 0, 0, 0, 0, 0, -3, 11, -22, -7, 2, -12, -5, 2, -4, -4, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 3, 0, 2
};

static signed char ctbl16nit[30] = {
	0, 0, 0, 0, 0, 0, -4, 11, -22, -7, 1, -10, -6, 1, -3, -3, 0, 0, -1, 0, 0, 0, 0, 1, 2, 0, 1, 2, 0, 1
};

static signed char ctbl17nit[30] = {
	0, 0, 0, 0, 0, 0, -2, 13, -27, -6, 1, -12, -6, 1, -2, -2, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 2, 0, 1
};

static signed char ctbl19nit[30] = {
	0, 0, 0, 0, 0, 0, -3, 9, -19, -3, 0, -12, -6, 0, -2, -3, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 2, 0, 2
};

static signed char ctbl20nit[30] = {
	0, 0, 0, 0, 0, 0, 0, 13, -26, -5, 0, -10, -5, 0, -2, -1, 0, 1, 0, 0, 1, 0, 0, 1, 1, 0, 0, 2, 0, 1
};

static signed char ctbl21nit[30] = {
	0, 0, 0, 0, 0, 0, 0, 6, -14, -5, 1, -12, -5, 1, -2, -1, 0, 1, 0, 0, 1, 0, 0, 1, 2, 0, 0, 2, 0, 2
};

static signed char ctbl22nit[30] = {
	0, 0, 0, 0, 0, 0, 0, 9, -20, -6, 0, -12, -5, 0, -2, -1, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 2, 0, 2
};

static signed char ctbl24nit[30] = {
	0, 0, 0, 0, 0, 0, -1, 12, -24, -1, 0, -10, -4, 0, -2, 0, 0, 2, 0, 0, 0, 0, 0, 1, 1, 0, 1, 2, 0, 1
};

static signed char ctbl25nit[30] = {
	0, 0, 0, 0, 0, 0, -6, 10, -21, -4, 1, -11, -2, 1, -2, -1, 0, 1, 0, 0, 1, 0, 0, 1, 2, 0, 0, 2, 0, 2
};

static signed char ctbl27nit[30] = {
	0, 0, 0, 0, 0, 0, -3, 12, -24, -4, 0, -12, -2, 0, -1, -1, 0, 1, 0, 0, 1, 1, 0, 1, 1, 0, 1, 2, 0, 1
};

static signed char ctbl29nit[30] = {
	0, 0, 0, 0, 0, 0, -1, 11, -23, 0, 0, -9, -4, 0, -1, 0, 0, 1, 1, 0, 2, 1, 0, 1, 1, 0, 0, 2, 0, 2
};

static signed char ctbl30nit[30] = {
	0, 0, 0, 0, 0, 0, -2, 11, -23, 1, 0, -8, -2, 0, -1, 0, 0, 2, 0, 0, 1, 1, 0, 1, 1, 0, 1, 2, 0, 1
};

static signed char ctbl32nit[30] = {
	0, 0, 0,0, 0, 0, 0, 13, -26, 0, 0, -8, -2, 0, -1, 0, 0, 2, 0, 0, 1, 1, 0, 1, 1, 0, 0, 2, 0, 2
};

static signed char ctbl34nit[30] = {
	0, 0, 0, 0, 0, 0, -2, 11, -23, 0, 0, -10, -1, 0, 0, 1, 0, 2, 0, 0, 1, 1, 0, 1, 1, 0, 0, 2, 0, 2
};

static signed char ctbl37nit[30] = {
	0, 0, 0, 0, 0, 0, 1, 11, -24, 1, 0, -8, -1, 0, 0, 0, 0, 1, 1, 0, 2, 1, 0, 1, 2, 0, 1, 1, 0, 1
};

static signed char ctbl39nit[30] = {
	0, 0, 0, 0, 0, 0, 0, 13, -27, 3, 0, -6, 0, 0, 0, 0, 0, 1, 2, 0, 2, 0, 0, 1, 2, 0, 1, 1, 0, 1
};

static signed char ctbl41nit[30] = {
	0, 0, 0, 0, 0, 0, 0, 11, -24, 1, 0, -6, 0, 0, 0, 0, 0, 1, 0, 0, 2, 2, 0, 1, 2, 0, 1, 1, 0, 1
};

static signed char ctbl44nit[30] = {
	0, 0, 0, 0, 0, 0, 0, 14, -28, 4, 0, -6, 0, 0, 0, 0, 0, 1, 1, -1, 2, 1, 0, 1, 2, 0, 1, 1, 0, 1
};

static signed char ctbl47nit[30] = {
	0, 0, 0, 0, 0, 0, -1, 12, -25, 3, 0, -7, -1, 0, -1, 1, 0, 2, 1, 0, 2, 1, 0, 1, 2, 0, 1, 1, 0, 1
};

static signed char ctbl50nit[30] = {
	0, 0, 0, 0, 0, 0, 0, 10, -21, 5, 0, -6, 0, 0, 0, 0, 0, 0, 1, -1, 2, 1, 0, 1, 2, 0, 1, 1, 0, 1
};

static signed char ctbl53nit[30] = {
	0, 0, 0, 0, 0, 0, 1, 10, -22, 4, 0, -6, 1, 0, 0, 1, 0, 1, 1, -1, 2, 0, 0, 1, 2, 0, 1, 1, 0, 1
};

static signed char ctbl56nit[30] = {
	0, 0, 0, 0, 0, 0, 3, 10, -20, 5, 0, -6, 0, 0, 0, 2, 0, 2, 1, -1, 2, 1, 0, 0, 1, 0, 1, 1, 0, 1
};

static signed char ctbl60nit[30] = {
	0, 0, 0, 0, 0, 0, 5, 9, -19, 7, 0, -5, 0, 0, 0, 0, 0, 1, 1, -1, 2, 3, 0, 1, 0, 0, 1, 1, 0, 1
};

static signed char ctbl64nit[30] = {
	0, 0, 0, 0, 0, 0, 9, 8, -16, 5, 0, -5, 0, 0, -1, 2, 0, 2, 1, -1, 2, 4, 0, 1, 0, 0, 1, 0, 0, 1
};

static signed char ctbl68nit[30] = {
	0, 0, 0, 0, 0, 0, 10, 6, -14, 8, 0, -6, 0, 0, 0, 2, 0, 2, 1, -1, 2, 2, 0, 1, 1, 0, 1, 1, 0, 1
};

static signed char ctbl72nit[30] = {
	0, 0, 0, 0, 0, 0, 16, 5, -10, 8, 0, -5, 2, 0, -1, 2, 0, 2, 1, 0, 2, 2, 0, 1, 1, 0, 1, 1, 0, 1
};

static signed char ctbl77nit[30] = {
	0, 0, 0, 0, 0, 0, 13, 4, -10, 8, 0, -5, 3, 0, 0, 2, 0, 2, 2, 0, 2, 2, 0, 1, 1, 0, 1, 0, 0, 1
};

static signed char ctbl82nit[30] = {
	0, 0, 0, 0, 0, 0, 15, 5, -11, 9, 0, -5, 1, 0, 0, 1, 0, 1, 1, 0, 2, 1, 0, 1, 1, 0, 1, 1, 0, 1
};

static signed char ctbl87nit[30] = {
	0, 0, 0, 0, 0, 0, 14, 5, -11, 7, 0, -4, 1, 0, 0, 2, 0, 2, 1, 0, 2, 2, 0, 1, 0, 0, 1, 0, 0, 1
};

static signed char ctbl93nit[30] = {
	0, 0, 0, 0, 0, 0, 12, 5, -11, 11, 0, -4, 1, 0, 0, 2, -1, 2, 0, 0, 1, 2, 0, 1, 1, 0, 1, 1, 0, 1
};

static signed char ctbl98nit[30] = {
	0, 0, 0, 0, 0, 0, 16, 5, -11, 8, 0, -4, 1, 0, 1, 2, 0, 2, 0, 0, 2, 2, 0, 1, 1, 0, 1, 0, 0, 1
};

static signed char ctbl105nit[30] = {
	0, 0, 0, 0, 0, 0, 12, 6, -12, 7, 0, -3, 1, 0, 0, 2, 0, 1, 0, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1
};

static signed char ctbl110nit[30] = {
	0, 0, 0, 0, 0, 0, 12, 4, -10, 9, 0, -3, 0, 0, 0, 1, 0, 1, 2, 0, 2, 1, 0, 1, 0, 0, 1, 1, 0, 1
};

static signed char ctbl119nit[30] = {
	0, 0, 0, 0, 0, 0, 13, 4, -10, 6, 0, -3, 1, 0, 1, 2, 0, 1, 2, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1
};

static signed char ctbl126nit[30] = {
	0, 0, 0, 0, 0, 0, 15, 4, -10, 7, 0, -2, 0, 0, 0, 1, -1, 2, 2, 0, 1, 0, 0, 1, 1, 0, 1, 1, 0, 1
};

static signed char ctbl134nit[30] = {
	0, 0, 0, 0, 0, 0, 15, 4, -8, 7, 0, -2, 1, 0, 2, 0, 0, 1, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1
};

static signed char ctbl143nit[30] = {
	0, 0, 0, 0, 0, 0, 14, 4, -10, 5, 0, 0, 1, 0, 0, 2, 0, 2, 2, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 1
};

static signed char ctbl152nit[30] = {
	0, 0, 0, 0, 0, 0, 9, 5, -11, 5, 0, -1, 1, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 1
};

static signed char ctbl162nit[30] = {
	0, 0, 0, 0, 0, 0, 9, 5, -11, 5, 0, 0, 1, 0, 0, 1, -1, 2, 1, -1, 2, 0, 0, 0, 0, 0, 0, 1, 0, 1
};

static signed char ctbl172nit[30] = {
	0, 0, 0, 0, 0, 0, 13, 4, -9, 3, 0, -1, 0, 0, 1, 2, 0, 2, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1
};

static signed char ctbl183nit[30] = {
	0, 0, 0, 0, 0, 0, 13, 4, -8, 4, 0, -1, 1, 0, 1, 1, -1, 2, 1, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static signed char ctbl195nit[30] = {
	0, 0, 0, 0, 0, 0, 16, 2, -5, 5, 0, 0, 0, 0, 0, 1, -1, 2, 1, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static signed char ctbl207nit[30] = {
 	0, 0, 0, 0, 0, 0, 18, 2, -4, 5, 0, 0, 0, 0, 0, 2, 0, 2, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static signed char ctbl220nit[30] = {
	0, 0, 0, 0, 0, 0, 20, 2, -4, 5, 0, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static signed char ctbl234nit[30] = {
	0, 0, 0, 0, 0, 0, 21, 1, -2, 5, 0, 0, 1, 0, 0, 1, 0, 2, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static signed char ctbl249nit[30] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static signed char ctbl265nit[30] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static signed char ctbl282nit[30] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static signed char ctbl300nit[30] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static signed char ctbl316nit[30] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static signed char ctbl333nit[30] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static signed char ctbl360nit[30] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static unsigned char aid9841[3] = {
	0xB2, 0xE7, 0x90
};

static unsigned char aid9779[3] = {
	0xB2, 0xD7, 0x90
};

static unsigned char aid9686[3] = {
	0xB2, 0xBF, 0x90
};

static unsigned char aid9639[3] = {
	0xB2, 0xB3, 0x90
};

static unsigned char aid9546[3] = {
	0xB2, 0x9B, 0x90
};

static unsigned char aid9464[3] = {
	0xB2, 0x86, 0x90
};

static unsigned char aid9406[3] = {
	0xB2, 0x77, 0x90
};

static unsigned char aid9309[3] = {
	0xB2, 0x5E, 0x90
};

static unsigned char aid9262[3] = {
	0xB2, 0x52, 0x90
};

static unsigned char aid9181[3] = {
	0xB2, 0x3D, 0x90
};

static unsigned char aid9092[3] = {
	0xB2, 0x26, 0x90
};

static unsigned char aid9030[3] = {
	0xB2, 0x16, 0x90
};

static unsigned char aid8964[3] = {
	0xB2, 0x05, 0x90
};

static unsigned char aid8886[3] = {
	0xB2, 0xF1, 0x80
};

static unsigned char aid8797[3] = {
	0xB2, 0xDA, 0x80
};

static unsigned char aid8719[3] = {
	0xB2, 0xC6, 0x80
};

static unsigned char aid8579[3] = {
	0xB2, 0xA2, 0x80
};

static unsigned char aid8482[3] = {
	0xB2, 0x89, 0x80
};

static unsigned char aid8381[3] = {
	0xB2, 0x6F, 0x80
};

static unsigned char aid8342[3] = {
	0xB2, 0x65, 0x80
};

static unsigned char aid8172[3] = {
	0xB2, 0x39, 0x80
};

static unsigned char aid8075[3] = {
	0xB2, 0x20, 0x80
};

static unsigned char aid7958[3] = {
	0xB2, 0x02, 0x80
};

static unsigned char aid7795[3] = {
	0xB2, 0xD8, 0x70
};

static unsigned char aid7710[3] = {
	0xB2, 0xC2, 0x70
};

static unsigned char aid7539[3] = {
	0xB2, 0x96, 0x70
};

static unsigned char aid7372[3] = {
	0xB2, 0x6B, 0x70
};

static unsigned char aid7139[3] = {
	0xB2, 0x2F, 0x70
};

static unsigned char aid7023[3] = {
	0xB2, 0x11, 0x70
};

static unsigned char aid6828[3] = {
	0xB2, 0xDF, 0x60
};

static unsigned char aid6584[3] = {
	0xB2, 0xA0, 0x60
};

static unsigned char aid6351[3] = {
	0xB2, 0x64, 0x60
};

static unsigned char aid6075[3] = {
	0xB2, 0x1D, 0x60
};

static unsigned char aid5881[3] = {
	0xB2, 0xEB, 0x50
};

static unsigned char aid5648[3] = {
	0xB2, 0xAF, 0x50
};

static unsigned char aid5299[3] = {
	0xB2, 0x55, 0x50
};

static unsigned char aid4942[3] = {
	0xB2, 0xF9, 0x40
};

static unsigned char aid4620[3] = {
	0xB2, 0xA6, 0x40
};

static unsigned char aid4220[3] = {
	0xB2, 0x3F, 0x40
};

static unsigned char aid3750[3] = {
	0xB2, 0xC6, 0x30
};

static unsigned char aid3269[3] = {
	0xB2, 0x4A, 0x30
};

static unsigned char aid2764[3] = {
	0xB2, 0xC8, 0x20
};

static unsigned char aid2220[3] = {
	0xB2, 0x3C, 0x20
};

static unsigned char aid1677[3] = {
	0xB2, 0xB0, 0x10
};

static unsigned char aid1005[3] = {
	0xB2, 0x03, 0x10
};

static unsigned char aid1005_1[3] = {
	0xB2, 0x03, 0x10
};

static unsigned char aid1005_2[3] = {
	0xB2, 0x03, 0x10
};

static unsigned char aid1005_3[3] = {
	0xB2, 0x03, 0x10
};

static unsigned char aid1005_4[3] = {
	0xB2, 0x03, 0x10
};

static unsigned char aid1005_5[3] = {
	0xB2, 0x03, 0x10
};

static unsigned char aid1005_6[3] = {
	0xB2, 0x03, 0x10
};

static unsigned char aid1005_7[3] = {
	0xB2, 0x03, 0x10
};

static unsigned char aid1005_8[3] = {
	0xB2, 0x03, 0x10
};

static unsigned char aid1005_9[3] = {
	0xB2, 0x03, 0x10
};

static unsigned char aid1005_10[3] = {
	0xB2, 0x03, 0x10
};

static unsigned char aid1005_11[3] = {
	0xB2, 0x03, 0x10
};

static unsigned char aid1005_12[3] = {
	0xB2, 0x03, 0x10
};

static unsigned char aid1005_13[3] = {
	0xB2, 0x03, 0x10
};

static unsigned char aid1005_14[3] = {
	0xB2, 0x03, 0x10
};


static unsigned char elv1 [3] = {
	0xB6, 0x8C, 0x11
};

static unsigned char elv2 [3] = {
	0xB6, 0x8C, 0x10
};

static unsigned char elv3 [3] = {
	0xB6, 0x8C, 0x0F
};

static unsigned char elv4 [3] = {
	0xB6, 0x8C, 0x0E
};

static unsigned char elv5 [3] = {
	0xB6, 0x8C, 0x0D
};

static unsigned char elv6 [3] = {
	0xB6, 0x8C, 0x0C
};


static unsigned char elv8 [3] = {
	0xB6, 0x8C, 0x0A
};

#ifdef CONFIG_MANUAL_ACL

static unsigned char elv7 [3] = {
	0xB6, 0x8C, 0x0B
};

static unsigned char elvCaps1 [3] = {
	0xB6, 0x9C, 0x11
};

static unsigned char elvCaps2 [3] = {
	0xB6, 0x9C, 0x10
};

static unsigned char elvCaps3 [3] = {
	0xB6, 0x9C, 0x0F
};

static unsigned char elvCaps4 [3] = {
	0xB6, 0x9C, 0x0E
};

static unsigned char elvCaps5 [3] = {
	0xB6, 0x9C, 0x0D
};
#endif

static unsigned char elvCaps6 [3] = {
	0xB6, 0x9C, 0x0C
};
static unsigned char elvCaps7 [3] = {
	0xB6, 0x9C, 0x0B
};
static unsigned char elvCaps8 [3] = {
	0xB6, 0x9C, 0x0A
};



/*aid parameter for daisy REV.C*/
static signed char RCdrtbl2nit[10] = {
	0, 22, 19, 30, 28, 24, 19, 14, 8, 0
};

static signed char RCdrtbl3nit[10] = {
	0, 22, 19, 24, 21, 18, 16, 11, 6, 0
};

static signed char RCdrtbl4nit[10] = {
	0, 22, 19, 21, 19, 15, 14, 10, 5, 0
};

static signed char RCdrtbl5nit[10] = {
	0, 22, 19, 19, 17, 14, 13, 9, 6, 0
};

static signed char RCdrtbl6nit[10] = {
	0, 0, 19, 17, 15, 12, 11, 8, 5, 0
};

static signed char RCdrtbl7nit[10] = {
	0, 5, 18, 15, 13, 11, 9, 7, 5, 0
};

static signed char RCdrtbl8nit[10] = {
	0, 16, 17, 14, 12, 10, 9, 6, 4, 0
};

static signed char RCdrtbl9nit[10] = {
	0, 15, 16, 13, 11, 9, 8, 6, 4, 0
};

static signed char RCdrtbl10nit[10] = {
	0, 17, 15, 12, 10, 8, 7, 5, 4, 0
};

static signed char RCdrtbl11nit[10] = {
	0, 18, 14, 11, 10, 8, 7, 5, 4, 0
};

static signed char RCdrtbl12nit[10] = {
	0, 15, 14, 11, 9, 7, 6, 5, 4, 0
};

static signed char RCdrtbl13nit[10] = {
	0, 13, 14, 10, 9, 7, 6, 5, 4, 0
};

static signed char RCdrtbl14nit[10] = {
	0, 14, 13, 9, 8, 6, 6, 5, 4, 0
};

static signed char RCdrtbl15nit[10] = {
	0, 11, 13, 9, 7, 6, 5, 4, 4, 0
};

static signed char RCdrtbl16nit[10] = {
	0, 14, 12, 8, 7, 5, 5, 4, 4, 0
};

static signed char RCdrtbl17nit[10] = {
	0, 10, 12, 8, 7, 5, 5, 4, 4, 0
};

static signed char RCdrtbl19nit[10] = {
	0, 12, 11, 7, 6, 5, 5, 4, 3, 0
};

static signed char RCdrtbl20nit[10] = {
	0, 9, 11, 7, 6, 4, 5, 4, 3, 0
};

static signed char RCdrtbl21nit[10] = {
	0, 10, 11, 7, 6, 4, 5, 4, 3, 0
};

static signed char RCdrtbl22nit[10] = {
	0, 7, 11, 7, 6, 4, 4, 4, 3, 0
};

static signed char RCdrtbl24nit[10] = {
	0, 9, 10, 6, 5, 4, 4, 4, 3, 0
};

static signed char RCdrtbl25nit[10] = {
	0, 7, 10, 6, 5, 4, 4, 4, 3, 0
};

static signed char RCdrtbl27nit[10] = {
	0, 10, 9, 6, 5, 3, 4, 4, 3, 0
};

static signed char RCdrtbl29nit[10] = {
	0, 6, 9, 5, 4, 3, 3, 4, 3, 0
};

static signed char RCdrtbl30nit[10] = {
	0, 4, 9, 5, 4, 3, 3, 4, 3, 0
};

static signed char RCdrtbl32nit[10] = {
	0, 6, 8, 5, 3, 2, 3, 3, 3, 0
};

static signed char RCdrtbl34nit[10] = {
	0, 4, 8, 5, 3, 2, 3, 3, 3, 0
};

static signed char RCdrtbl37nit[10] = {
	0, 6, 7, 4, 3, 2, 2, 3, 3, 0
};

static signed char RCdrtbl39nit[10] = {
	0, 4, 7, 4, 3, 2, 2, 3, 3, 0
};

static signed char RCdrtbl41nit[10] = {
	0, 2, 7, 4, 2, 2, 2, 3, 3, 0
};

static signed char RCdrtbl44nit[10] = {
	0, 4, 6, 3, 2, 2, 2, 3, 3, 0
};

static signed char RCdrtbl47nit[10] = {
	0, 2, 6, 3, 2, 1, 2, 3, 3, 0
};

static signed char RCdrtbl50nit[10] = {
	0, 5, 5, 3, 2, 1, 2, 3, 3, 0
};

static signed char RCdrtbl53nit[10] = {
	0, 1, 5, 3, 1, 1, 2, 3, 3, 0
};

static signed char RCdrtbl56nit[10] = {
	0, 1, 5, 3, 1, 1, 1, 2, 3, 0
};

static signed char RCdrtbl60nit[10] = {
	0, 3, 4, 2, 1, 1, 1, 2, 3, 0
};

static signed char RCdrtbl64nit[10] = {
	0, 1, 4, 2, 1, 1, 1, 2 , 3 , 0
};

static signed char RCdrtbl68nit[10] = {
	0, 1, 4, 2, 1, 1, 1, 2 , 3 , 0
};

static signed char RCdrtbl72nit[10] = {
	0, 1, 3, 2, 1, 1, 1, 2 , 2 , 0
};

static signed char RCdrtbl77nit[10] = {
	0, 1, 3, 2, 1, 0, 1, 2 , 2 , 0
};

static signed char RCdrtbl82nit[10] = {
	0, 1, 3, 2, 0, 0, 1, 2 , 2 , 0
};

static signed char RCdrtbl87nit[10] = {
	0, 1, 3, 2, 1, 1, 1, 1 , 2 , 0
};

static signed char RCdrtbl93nit[10] = {
	0, 1, 2, 2, 0, 1, 1, 3 , 1 , 0
};

static signed char RCdrtbl98nit[10] = {
	0, 1, 3, 2, 1, 1, 2, 2 , 1 , 0
};

static signed char RCdrtbl105nit[10] = {
	0, 1, 3, 1, 1, 1, 1, 3 , 1 , 0
};

static signed char RCdrtbl110nit[10] = {
	0, 1, 3, 2, 0, 1, 1, 3 , 0 , 0
};

static signed char RCdrtbl119nit[10] = {
	0, 1, 3, 2, 1, 1, 2, 3 , 1 , 0
};

static signed char RCdrtbl126nit[10] = {
	0, 1, 2, 2, 1, 1, 2, 3 , 0 , 0
};

static signed char RCdrtbl134nit[10] = {
	0, 1, 2, 2, 0, 0, 1, 3 , -1 , 0
};

static signed char RCdrtbl143nit[10] = {
	0, 0, 2, 1, 0, 1, 2, 3 , 1 , 0
};

static signed char RCdrtbl152nit[10] = {
	0, -1, 3, 1, 1, 1, 2, 3, 1, 0
};

static signed char RCdrtbl162nit[10] = {
	0, -1, 2, 1, 0, 1, 2, 3, 1, 0
};

static signed char RCdrtbl172nit[10] = {
	0, -1, 2, 1, -1, 0, 1, 2, 1, 0
};

static signed char RCdrtbl183nit[10] = {
	0, -1, 2, 0, 0, 1, 2, 3, 1, 0
};

static signed char RCdrtbl195nit[10] = {
	0, 0, 1, 0, 0, 0, 1, 3, 1, 0
};

static signed char RCdrtbl207nit[10] = {
	0, -1, 1, 0, 0, 0, 1, 2, 1, 0
};

static signed char RCdrtbl220nit[10] = {
	0, -1, 1, 0, 0, 0, 1, 2, 1, 0
};

static signed char RCdrtbl234nit[10] = {
	0, 0, 0, 0, 0, 0, 0, 1, 0, 0
};

static signed char RCdrtbl249nit[10] = {
	0, -1, 1, 0, 0, 0, 0, 1, 0, 0
};

static signed char RCdrtbl265nit[10] = {
	0, 0, 1, 0, 0, -1, 0, 0, -1, 0
};

static signed char RCdrtbl282nit[10] = {
	0, 0, 0, 0, 0, -1, 0, 1, 0, 0
};

static signed char RCdrtbl300nit[10] = {
	0, 0, 0, 0, -1, -1, 0, 0, 0, 0
};

static signed char RCdrtbl316nit[10] = {
	0, 0, 0, -1, -1, -1, -1, 0, 0, 0
};

static signed char RCdrtbl333nit[10] = {
	0, -1, 1, -1, -2, -1, -2, -1, -1, 0
};

static signed char RCdrtbl360nit[10] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};


static signed char RCdctbl2nit[30] = {
	0, 0, 0, 0, 0, 0, 12, 0, 0, 0, 4, -9, -1, 4, -8, -6, 6, -12, -1, 5, -10, -1, 1, -4, -2, 0, -3, -6, 1, -8
};

static signed char RCdctbl3nit[30] = {
	0, 0, 0, 0, 0, 0, 12, 0, 0, 0, 4, -9, -5, 5, -11, -1, 6, -12, -4, 3, -7, -3, 0, -4, -1, 0, -3, -4, 1, -5
};

static signed char RCdctbl4nit[30] = {
	0, 0, 0, 0, 0, 0, 12, 0, 0, -2, 5, -12, -3, 4, -9, -2, 6, -12, -4, 3, -6, -1, 0, -3, -1, 0, -2, -3, 1, -4
};

static signed char RCdctbl5nit[30] = {
	0, 0, 0, 0, 0, 0, 12, 0, 0, -3, 5, -12, -2, 5, -10, -3, 6, -12, -4, 2, -6, -2, 0, -3, -1, 0, -2, -3, 0, -4
};

static signed char RCdctbl6nit[30] = {
	0, 0, 0, 0, 0, 0, 12, 0, 0, -3, 6, -13, -6, 5, -10, -1, 5, -11, -5, 2, -6, -1, 0, -2, -1, 0, -2, -2, 0, -3
};

static signed char RCdctbl7nit[30] = {
	0, 0, 0, 0, 0, 0, 4, 3, -8, -2, 6, -12, -3, 5, -10, -3, 5, -10, -3, 2, -5, -1, 0, -2, 0, 0, -1, -2, 0, -3
};

static signed char RCdctbl8nit[30] = {
	0, 0, 0, 0, 0, 0, 0, 5, -11, -4, 5, -12, -2, 5, -11, -3, 4, -9, -3, 2, -5, -1, 0, -2, -1, 0, -1, -1, 0, -2
};

static signed char RCdctbl9nit[30] = {
	0, 0, 0, 0, 0, 0, 0, 6, -13, -3, 6, -12, -4, 5, -12, -2, 4, -8, -4, 2, -4, 0, 0, -1, -1, 0, -1, -1, 0, -2
};

static signed char RCdctbl10nit[30] = {
	0, 0, 0, 0, 0, 0, 0, 7, -15, -4, 5, -12, -2, 5, -12, -3, 3, -7, -4, 2, -4, -1, 0, -2, 0, 0, 0, -1, 0, -2
};

static signed char RCdctbl11nit[30] = {
	0, 0, 0, 0, 0, 0, 2, 8, -17, -3, 6, -12, -1, 5, -10, -3, 3, -7, -4, 1, -4, -1, 0, -2, 0, 0, -1, -1, 0, -1
};

static signed char RCdctbl12nit[30] = {
	0, 0, 0, 0, 0, 0, 2, 8, -17, -3, 5, -12, -2, 5, -12, -3, 3, -6, -4, 1, -4, 0, 0, -2, 0, 0, 0, -1, 0, -1
};

static signed char RCdctbl13nit[30] = {
	0, 0, 0, 0, 0, 0, 0, 8, -17, -3, 6, -13, -1, 5, -12, -4, 2, -6, -4, 1, -3, 0, 0, -2, 0, 0, 0, -1, 0, -1
};

static signed char RCdctbl14nit[30] = {
	0, 0, 0, 0, 0, 0, 2, 10, -22, -5, 6, -13, 0, 5, -10, -4, 2, -6, -3, 1, -2, 0, 0, -2, 0, 0, 0, -1, 0, -1
};

static signed char RCdctbl15nit[30] = {
	0, 0, 0, 0, 0, 0, 4, 10, -21, -5, 5, -11, 0, 5, -11, -5, 2, -6, -2, 1, -3, 0, 0, -1, 0, 0, 0, -1, 0, -1
};

static signed char RCdctbl16nit[30] = {
	0, 0, 0, 0, 0, 0, 3, 11, -24, -3, 6, -12, 0, 5, -10, -5, 2, -6, -2, 1, -3, 0, 0, -1, 0, 0, 0, -1, 0, -1
};

static signed char RCdctbl17nit[30] = {
	0, 0, 0, 0, 0, 0, 7, 11, -24, -5, 6, -13, -1, 4, -10, -4, 2, -5, -2, 1, -2, 0, 0, -2, 0, 0, 0, 0, 0, 0
};

static signed char RCdctbl19nit[30] = {
	0, 0, 0, 0, 0, 0, 4, 12, -25, -2, 6, -14, -2, 4, -10, -4, 2, -4, -1, 1, -2, -1, 0, -1, 0, 0, -1, 0, 0, 0
};

static signed char RCdctbl20nit[30] = {
	0, 0, 0, 0, 0, 0, 5, 12, -24, -3, 6, -14, -1, 4, -8, -4, 2, -4, -1, 1, -2, -1, 0, -1, 0, 0, -1, 0, 0, 0
};

static signed char RCdctbl21nit[30] = {
	0, 0, 0, 0, 0, 0, 2, 12, -25, -2, 6, -14, -2, 3, -8, -3, 2, -4, -2, 0, -2, -1, 0, -1, 0, 0, -1, 0, 0, 0
};

static signed char RCdctbl22nit[30] = {
	0, 0, 0, 0, 0, 0, 3, 12, -24, -2, 6, -14, -3, 3, -8, -3, 1, -4, -3, 0, -2, -1, 0, -1, 0, 0, -1, 0, 0, 0
};

static signed char RCdctbl24nit[30] = {
	0, 0, 0, 0, 0, 0, 5, 12, -26, -1, 7, -16, -2, 3, -8, -4, 1, -4, -3, 0, -2, 0, 0, -1, 0, 0, 0, 0, 0, 0
};

static signed char RCdctbl25nit[30] = {
	0, 0, 0, 0, 0, 0, 2, 12, -26, -2, 7, -16, -1, 3, -7, -3, 1, -3, -3, 0, -2, 0, 0, -1, 0, 0, 0, 0, 0, 0
};

static signed char RCdctbl27nit[30] = {
	0, 0, 0, 0, 0, 0, 6, 15, -30, -1, 7, -15, -2, 2, -6, -4, 1, -4, -2, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0
};

static signed char RCdctbl29nit[30] = {
	0, 0, 0, 0, 0, 0, 5, 12, -25, 0, 8, -17, -2, 3, -6, -2, 1, -2, -2, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0
};

static signed char RCdctbl30nit[30] = {
	0, 0, 0, 0, 0, 0, 4, 11, -24, 0, 8, -16, -2, 2, -6, -2, 1, -2, -2, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0
};

static signed char RCdctbl32nit[30] = {
	0, 0, 0, 0, 0, 0, 7, 15, -30, 1, 6, -14, -3, 2, -6, -2, 1, -2, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0
};

static signed char RCdctbl34nit[30] = {
	0, 0, 0, 0, 0, 0, 6, 14, -28, 1, 6, -14, -3, 2, -5, -2, 0, -2, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0
};

static signed char RCdctbl37nit[30] = {
	0, 0, 0, 0, 0, 0, 7, 15, -32, 2, 8, -16, -2, 2, -5, -2, 0, -1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0
};

static signed char RCdctbl39nit[30] = {
	0, 0, 0, 0, 0, 0, 5, 15, -30, 2, 7, -16, -1, 1, -4, -2, 0, -1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0
};

static signed char RCdctbl41nit[30] = {
	0, 0, 0, 0, 0, 0, 5, 14, -29, 1, 7, -15, -1, 1, -4, -1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0
};

static signed char RCdctbl44nit[30] = {
	0, 0, 0, 0, 0, 0, 7, 15, -32, 2, 8, -16, 0, 1, -3, -1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0
};

static signed char RCdctbl47nit[30] = {
	0, 0, 0, 0, 0, 0, 5, 15, -31, 2, 7, -15, 0, 1, -4, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0
};

static signed char RCdctbl50nit[30] = {
	0, 0, 0, 0, 0, 0, 8, 17, -36, 3, 7, -14, 0, 1, -3, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0
};

static signed char RCdctbl53nit[30] = {
	0, 0, 0, 0, 0, 0, 9, 16, -33, 4, 6, -13, 0, 1, -2, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0
};

static signed char RCdctbl56nit[30] = {
	0, 0, 0, 0, 0, 0, 8, 16, -32, 3, 5, -12, 0, 1, -3, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0
};

static signed char RCdctbl60nit[30] = {
	0, 0, 0, 0, 0, 0, 7, 17, -35, 6, 7, -14, 0, 1, -2, 0, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0
};

static signed char RCdctbl64nit[30] = {
	0, 0, 0, 0, 0, 0, 9, 16, -34, 5, 6, -13, 0, 0, -2, 0, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0
};

static signed char RCdctbl68nit[30] = {
	0, 0, 0, 0, 0, 0, 8, 15, -32, 6, 6, -13, 0, 0, -1, 0, 0, 0, 0, 0, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0
};

static signed char RCdctbl72nit[30] = {
	0, 0, 0, 0, 0, 0, 9, 14, -30, 7, 6, -12, 0, 0, -2, 0, 0, 1, 0, 0, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0
};

static signed char RCdctbl77nit[30] = {
	0, 0, 0, 0, 0, 0, 9, 13, -28, 7, 5, -12, 1, 0, -1, 0, 0, 1, 0, 0, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0
};

static signed char RCdctbl82nit[30] = {
	0, 0, 0, 0, 0, 0, 8, 13, -28, 8, 5, -11, 0, 0, -2, 1, 0, 2, 0, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0
};

static signed char RCdctbl87nit[30] = {
	0, 0, 0, 0, 0, 0, 8, 13, -27, 8, 6, -12, 1, 0, 0, 0, 0, 1, 0, 0, 1, 2, 0, 2, 0, 0, 0, 0, 0, 0
};

static signed char RCdctbl93nit[30] = {
	0, 0, 0, 0, 0, 0, 10, 14, -29, 5, 5, -10, 2, 0, 0, 0, 0, 2, 0, 0, 1, 3, 0, 2, 0, 0, 0, 0, 0, 0
};

static signed char RCdctbl98nit[30] = {
	0, 0, 0, 0, 0, 0, 8, 14, -30, 5, 4, -10, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0
};

static signed char RCdctbl105nit[30] = {
	0, 0, 0, 0, 0, 0, 8, 14, -29, 5, 4, -10, 1, 0, -1, 1, 0, 2, 0, 0, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0
};

static signed char RCdctbl110nit[30] = {
	0, 0, 0, 0, 0, 0, 9, 14, -28, 4, 4, -8, 1, 0, -1, 0, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static signed char RCdctbl119nit[30] = {
	0, 0, 0, 0, 0, 0, 9, 14, -28, 5, 3, -8, 1, 0, 0, 1, 0, 2, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static signed char RCdctbl126nit[30] = {
	0, 0, 0, 0, 0, 0, 8, 14, -29, 3, 3, -6, 1, 0, 0, 0, 0, 1, 2, -1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static signed char RCdctbl134nit[30] = {
	0, 0, 0, 0, 0, 0, 8, 13, -28, 3, 2, -6, 1, 0, 0, 0, 0, 2, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static signed char RCdctbl143nit[30] = {
	0, 0, 0, 0, 0, 0, 9, 13, -27, 3, 2, -6, 2, 0, 1, 0, 0, 1, 1, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static signed char RCdctbl152nit[30] = {
	0, 0, 0, 0, 0, 0, 9, 11, -22, 3, 3, -6, 1, 0, 0, 1, 0, 2, 1, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static signed char RCdctbl162nit[30] = {
	0, 0, 0, 0, 0, 0, 9, 11, -23, 2, 1, -4, 1, 0, 0, 0, 0, 2, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static signed char RCdctbl172nit[30] = {
	0, 0, 0, 0, 0, 0, 9, 10, -22, 1, 1, -4, 1, 0, 0, 1, 0, 2, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static signed char RCdctbl183nit[30] = {
	0, 0, 0, 0, 0, 0, 9, 10, -20, 3, 1, -4, 1, 0, 1, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static signed char RCdctbl195nit[30] = {
	0, 0, 0, 0, 0, 0, 10, 11, -23, 4, 1, -3, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static signed char RCdctbl207nit[30] = {
	0, 0, 0, 0, 0, 0, 10, 10, -22, 4, 1, -3, 0, 0, 1, 0, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static signed char RCdctbl220nit[30] = {
	0, 0, 0, 0, 0, 0, 11, 10, -20, 5, 1, -2, 0, 0, 1, 0, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static signed char RCdctbl234nit[30] = {
	0, 0, 0, 0, 0, 0, 12, 8, -17, 4, 1, -2, 1, 0, 1, 0, 0, 1, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static signed char RCdctbl249nit[30] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static signed char RCdctbl265nit[30] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static signed char RCdctbl282nit[30] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static signed char RCdctbl300nit[30] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static signed char RCdctbl316nit[30] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static signed char RCdctbl333nit[30] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static signed char RCdctbl360nit[30] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};


static unsigned char aid9833[3] = {
	0xB2, 0xE5, 0x90
};

static unsigned char aid9736[3] = {
	0xB2, 0xCC, 0x90
};

static unsigned char aid9689[3] = {
	0xB2, 0xC0, 0x90
};

static unsigned char aid9592[3] = {
	0xB2, 0xA7, 0x90
};

static unsigned char aid9523[3] = {
	0xB2, 0x95, 0x90
};

static unsigned char aid9453[3] = {
	0xB2, 0x83, 0x90
};

static unsigned char aid9379[3] = {
	0xB2, 0x70, 0x90
};

static unsigned char aid9278[3] = {
	0xB2, 0x56, 0x90
};

static unsigned char aid9185[3] = {
	0xB2, 0x3E, 0x90
};

static unsigned char aid9142[3] = {
	0xB2, 0x33, 0x90
};

static unsigned char aid9053[3] = {
	0xB2, 0x1C, 0x90
};

static unsigned char aid8967[3] = {
	0xB2, 0x06, 0x90
};

static unsigned char aid8898[3] = {
	0xB2, 0xF4, 0x80
};

static unsigned char aid8832[3] = {
	0xB2, 0xE3, 0x80
};

static unsigned char aid8738[3] = {
	0xB2, 0xCB, 0x80
};

static unsigned char aid8657[3] = {
	0xB2, 0xB6, 0x80
};

static unsigned char aid8517[3] = {
	0xB2, 0x92, 0x80
};

static unsigned char aid8432[3] = {
	0xB2, 0x7C, 0x80
};

static unsigned char aid8346[3] = {
	0xB2, 0x66, 0x80
};

static unsigned char aid8276[3] = {
	0xB2, 0x54, 0x80
};

static unsigned char aid8102[3] = {
	0xB2, 0x27, 0x80
};

static unsigned char aid8032[3] = {
	0xB2, 0x15, 0x80
};

static unsigned char aid7873[3] = {
	0xB2, 0xEC, 0x70
};

static unsigned char aid7717[3] = {
	0xB2, 0xC4, 0x70
};

static unsigned char aid7640[3] = {
	0xB2, 0xB0, 0x70
};

static unsigned char aid7469[3] = {
	0xB2, 0x84, 0x70
};

static unsigned char aid7294[3] = {
	0xB2, 0x57, 0x70
};
static unsigned char aid7050[3] = {
	0xB2, 0x18, 0x70
};

static unsigned char aid6906[3] = {
	0xB2, 0xF3, 0x60
};

static unsigned char aid6727[3] = {
	0xB2, 0xC5, 0x60
};

static unsigned char aid6479[3] = {
	0xB2, 0x85, 0x60
};

static unsigned char aid6231[3] = {
	0xB2, 0x45, 0x60
};

static unsigned char aid5986[3] = {
	0xB2, 0x06, 0x60
};

static unsigned char aid5734[3] = {
	0xB2, 0xC5, 0x50
};

static unsigned char aid5481[3] = {
	0xB2, 0x84, 0x50
};

static unsigned char aid5148[3] = {
	0xB2, 0x2E, 0x50
};

static unsigned char aid4798[3] = {
	0xB2, 0xD4, 0x40
};

static unsigned char aid4472[3] = {
	0xB2, 0x80, 0x40
};

static unsigned char aid4115[3] = {
	0xB2, 0x24, 0x40
};

static unsigned char aid3680[3] = {
	0xB2, 0xB4, 0x30
};

static unsigned char aid3241[3] = {
	0xB2, 0x43, 0x30
};

static unsigned char aid2752[3] = {
	0xB2, 0xC5, 0x20
};

static unsigned char aid2244[3] = {
	0xB2, 0x42, 0x20
};

static unsigned char aid1646[3] = {
	0xB2, 0xA8, 0x10
};


// start of rev.D
static signed char RDdrtbl2nit[10] = {0, 20, 24, 24, 20, 17, 15, 11, 7, 0};
static signed char RDdrtbl3nit[10] = {0, 20, 24, 22, 18, 16, 13, 10, 7, 0};
static signed char RDdrtbl4nit[10] = {0, 24, 22, 19, 15, 13, 12, 9, 6, 0};
static signed char RDdrtbl5nit[10] = {0, 20, 21, 18, 15, 13, 12, 9, 6, 0};
static signed char RDdrtbl6nit[10] = {0, 21, 21, 18, 14, 13, 12, 8, 6, 0};
static signed char RDdrtbl7nit[10] = {0, 22, 20, 17, 13, 12, 11, 8, 6, 0};
static signed char RDdrtbl8nit[10] = {0, 19, 19, 16, 12, 11, 10, 7, 5, 0};
static signed char RDdrtbl9nit[10] = {0, 18, 18, 15, 11, 10, 9, 7, 5, 0};
static signed char RDdrtbl10nit[10] = {0, 20, 17, 14, 11, 10, 9, 6, 5, 0};
static signed char RDdrtbl11nit[10] = {0, 17, 16, 13, 10, 9, 8, 6, 4, 0};
static signed char RDdrtbl12nit[10] = {0, 15, 16, 13, 10, 9, 8, 6, 4, 0};
static signed char RDdrtbl13nit[10] = {0, 18, 15, 12, 9, 8, 7, 5, 4, 0};
static signed char RDdrtbl14nit[10] = {0, 15, 15, 11, 9, 8, 7, 5, 4, 0};
static signed char RDdrtbl15nit[10] = {0, 17, 14, 11, 8, 7, 7, 5, 4, 0};
static signed char RDdrtbl16nit[10] = {0, 15, 14, 10, 8, 7, 6, 5, 4, 0};
static signed char RDdrtbl17nit[10] = {0, 17, 13, 10, 7, 6, 6, 5, 4, 0};
static signed char RDdrtbl19nit[10] = {0, 16, 12, 9, 6, 6, 5, 4, 3, 0};
static signed char RDdrtbl20nit[10] = {0, 15, 12, 9, 6, 6, 5, 4, 3, 0};
static signed char RDdrtbl21nit[10] = {0, 11, 12, 8, 6, 6, 5, 4, 3, 0};
static signed char RDdrtbl22nit[10] = {0, 10, 12, 8, 6, 5, 5, 4, 3, 0};
static signed char RDdrtbl24nit[10] = {0, 12, 11, 8, 5, 5, 5, 4, 3, 0};
static signed char RDdrtbl25nit[10] = {0, 14, 10, 7, 5, 4, 4, 4, 3, 0};
static signed char RDdrtbl27nit[10] = {0, 10, 10, 7, 5, 4, 4, 3, 3, 0};
static signed char RDdrtbl29nit[10] = {0, 13, 9, 6, 4, 4, 4, 3, 3, 0};
static signed char RDdrtbl30nit[10] = {0, 10, 9, 6, 4, 4, 4, 3, 3, 0};
static signed char RDdrtbl32nit[10] = {0, 9, 9, 6, 4, 3, 4, 3, 3, 0};
static signed char RDdrtbl34nit[10] = {0, 11, 8, 6, 3, 3, 3, 3, 3, 0};
static signed char RDdrtbl37nit[10] = {0, 9, 8, 6, 3, 3, 3, 3, 3, 0};
static signed char RDdrtbl39nit[10] = {0, 10, 7, 5, 3, 3, 3, 3, 3, 0};
static signed char RDdrtbl41nit[10] = {0, 8, 7, 5, 3, 3, 3, 3, 3, 0};
static signed char RDdrtbl44nit[10] = {0, 7, 7, 5, 3, 3, 3, 3, 3, 0};
static signed char RDdrtbl47nit[10] = {0, 8, 6, 4, 2, 3, 3, 2, 3, 0};
static signed char RDdrtbl50nit[10] = {0, 6, 6, 4, 2, 2, 3, 2, 3, 0};
static signed char RDdrtbl53nit[10] = {0, 9, 5, 4, 2, 2, 3, 2, 3, 0};
static signed char RDdrtbl56nit[10] = {0, 7, 5, 4, 2, 2, 2, 2, 3, 0};
static signed char RDdrtbl60nit[10] = {0, 4, 5, 4, 1, 2, 2, 1, 3, 0};
static signed char RDdrtbl64nit[10] = {0, 6, 4, 3, 1, 2, 2, 1, 2, 0};
static signed char RDdrtbl68nit[10] = {0, 4, 4, 3, 1, 2, 2, 1, 2, 0};
static signed char RDdrtbl72nit[10] = {0, 3, 4, 3, 1, 2, 2, 1, 2, 0};
static signed char RDdrtbl77nit[10] = {0, 3, 4, 2, 1, 2, 2, 2, 2, 0};
static signed char RDdrtbl82nit[10] = {0, 4, 4, 3, 1, 1, 1, 2, 2, 0};
static signed char RDdrtbl87nit[10] = {0, 3, 4, 3, 2, 1, 1, 2, 3, 0};
static signed char RDdrtbl93nit[10] = {0, 4, 3, 2, 1, 1, 2, 3, 1, 0};
static signed char RDdrtbl98nit[10] = {0, 2, 3, 2, 1, 1, 1, 3, 2, 0};
static signed char RDdrtbl105nit[10] = {0, 5, 3, 2, 1, 2, 3, 3, 2, 0};
static signed char RDdrtbl110nit[10] = {0, 4, 3, 2, 1, 1, 2, 4, 2, 0};
static signed char RDdrtbl119nit[10] = {0, 5, 2, 1, 2, 1, 2, 3, 1, 0};
static signed char RDdrtbl126nit[10] = {0, 6, 2, 2, 1, 2, 2, 3, 0, 0};
static signed char RDdrtbl134nit[10] = {0, 4, 2, 2, 1, 1, 1, 3, 0, 0};
static signed char RDdrtbl143nit[10] = {0, 4, 2, 1, 1, 1, 2, 3, 1, 0};
static signed char RDdrtbl152nit[10] = {0, 4, 2, 2, 1, 1, 2, 4, 2, 0};
static signed char RDdrtbl162nit[10] = {0, 3, 1, 1, 0, 1, 2, 4, 1, 0};
static signed char RDdrtbl172nit[10] = {0, 5, 1, 1, 0, 1, 1, 3, 1, 0};
static signed char RDdrtbl183nit[10] = {0, 2, 2, 1, 1, 1, 2, 4, 1, 0};
static signed char RDdrtbl195nit[10] = {0, 5, 1, 1, 1, 1, 1, 3, 1, 0};
static signed char RDdrtbl207nit[10] = {0, 4, 1, 1, 1, 1, 1, 3, 1, 0};
static signed char RDdrtbl220nit[10] = {0, 1, 1, 0, 1, 1, 1, 3, 1, 0};
static signed char RDdrtbl234nit[10] = {0, 0, 1, 0, 1, 1, 1, 3, 1, 0};
static signed char RDdrtbl249nit[10] = {0, 0, 1, 0, 1, 1, 1, 2, 1, 0};
static signed char RDdrtbl265nit[10] = {0, 0, 1, 1, 1, 0, 1, 2, 0, 0};
static signed char RDdrtbl282nit[10] = {0, 0, 0, 0, 0, 0, 1, 1, 0, 0};
static signed char RDdrtbl300nit[10] = {0, 1, 0, 0, 0, 0, 0, 1, 0, 0};
static signed char RDdrtbl316nit[10] = {0, 2, 0, 0, -1, 0, -1, 1, 0, 0};
static signed char RDdrtbl333nit[10] = {0, 2, 0, -1, -1, -1, -1, 0, -1, 0};
static signed char RDdrtbl360nit[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static signed char RDdctbl2nit[30] = {0, 0, 0, 0, 0, 0,-5, 3, -7, -6, 3, -6, -11, 2, -6, -16, 4, -8, -10, 1, -4, -3, 0, -2, -4, 0, -3, -5, 1, -6};
static signed char RDdctbl3nit[30] = {0, 0, 0, 0, 0, 0,-5, 3, -7, -6, 3, -6, -10, 3, -7, -12, 3, -7, -9, 1, -4, -3, 0, -2, -3, 0, -3, -4, 1, -5};
static signed char RDdctbl4nit[30] = {0, 0, 0, 0, 0, 0,-1, 3, -7, -7, 2, -6, -10, 3, -8, -10, 2, -6, -7, 2, -4, -2, 0, -2, -3, 0, -2, -2, 1, -4};
static signed char RDdctbl5nit[30] = {0, 0, 0, 0, 0, 0,-2, 3, -7, -7, 3, -8, -7, 3, -7, -9, 3, -6, -6, 2, -4, -3, 0, -2, -2, 0, -2, -2, 0, -4};
static signed char RDdctbl6nit[30] = {0, 0, 0, 0, 0, 0,-4, 4, -8, -7, 3, -7, -5, 3, -8, -7, 2, -6, -6, 2, -4, -2, 0, -2, -1, 0, -1, -2, 0, -4};
static signed char RDdctbl7nit[30] = {0, 0, 0, 0, 0, 0,-4, 3, -8, -7, 3, -8, -5, 4, -8, -7, 2, -5, -5, 1, -4, -2, 0, -2, -1, 0, -1, -1, 0, -3};
static signed char RDdctbl8nit[30] = {0, 0, 0, 0, 0, 0,-3, 4, -8, -5, 3, -8, -4, 4, -9, -7, 2, -5, -5, 1, -4, -2, 0, -2, -1, 0, -1, 0, 0, -2};
static signed char RDdctbl9nit[30] = {0, 0, 0, 0, 0, 0,-4, 3, -8, -4, 4, -8, -4, 4, -9, -5, 2, -4, -4, 1, -3, -1, 0, -1, -1, 0, -1, 0, 0, -2};
static signed char RDdctbl10nit[30] = {0, 0, 0, 0, 0, 0,-3, 4, -10, -5, 4, -10, -3, 3, -8, -6, 2, -5, -3, 1, -2, -2, 0, -2, 0, 0, -1, 0, 0, -1};
static signed char RDdctbl11nit[30] = {0, 0, 0, 0, 0, 0,-3, 4, -8, -4, 4, -10, -3, 3, -6, -5, 2, -5, -4, 1, -3, -2, 0, -2, 0, 0, -1, 1, 0, 0};
static signed char RDdctbl12nit[30] = {0, 0, 0, 0, 0, 0,-2, 4, -8, -3, 4, -10, -3, 3, -6, -5, 2, -5, -3, 1, -3, -2, 0, -1, 0, 0, -1, 1, 0, 0};
static signed char RDdctbl13nit[30] = {0, 0, 0, 0, 0, 0,-1, 5, -11, -4, 4, -10, -2, 3, -6, -5, 2, -5, -3, 1, -3, -1, 0, -1, 0, 0, 0, 1, 0, 0};
static signed char RDdctbl14nit[30] = {0, 0, 0, 0, 0, 0,-1, 3, -8, -3, 5, -11, -2, 3, -6, -4, 2, -4, -2, 1, -2, -1, 0, -1, 0, 0, 0, 1, 0, 0};
static signed char RDdctbl15nit[30] = {0, 0, 0, 0, 0, 0,-2, 6, -13, -2, 4, -10, -3, 2, -6, -4, 2, -4, -2, 1, -2, 0, 0, 0, 0, 0, 0, 1, 0, 0};
static signed char RDdctbl16nit[30] = {0, 0, 0, 0, 0, 0,-2, 4, -10, -2, 5, -12, -3, 2, -5, -4, 1, -4, -2, 1, -2, 0, 0, 0, 0, 0, 0, 1, 0, 0};
static signed char RDdctbl17nit[30] = {0, 0, 0, 0, 0, 0,0, 6, -14, -2, 4, -10, -3, 2, -5, -4, 1, -4, -2, 1, -2, 0, 0, 0, 0, 0, 0, 1, 0, 0};
static signed char RDdctbl19nit[30] = {0, 0, 0, 0, 0, 0,0, 7, -16, -3, 4, -10, -3, 1, -4, -3, 1, -3, -2, 1, -2, 0, 0, 0, 0, 0, 0, 2, 0, 1};
static signed char RDdctbl20nit[30] = {0, 0, 0, 0, 0, 0,0, 7, -16, -2, 4, -9, -3, 1, -4, -4, 1, -4, -2, 0, -2, 0, 0, 0, 0, 0, 0, 2, 0, 1};
static signed char RDdctbl21nit[30] = {0, 0, 0, 0, 0, 0,0, 5, -10, -3, 5, -10, -3, 1, -4, -3, 1, -3, -2, 0, -2, 0, 0, 0, 0, 0, 0, 2, 0, 1};
static signed char RDdctbl22nit[30] = {0, 0, 0, 0, 0, 0,0, 4, -10, -2, 5, -10, -3, 1, -4, -3, 1, -3, -2, 0, -2, 0, 0, 0, 0, 0, 0, 2, 0, 1};
static signed char RDdctbl24nit[30] = {0, 0, 0, 0, 0, 0,0, 6, -14, -3, 4, -8, -4, 1, -4, -3, 1, -3, -1, 0, -1, 0, 0, 0, 0, 0, 0, 2, 0, 1};
static signed char RDdctbl25nit[30] = {0, 0, 0, 0, 0, 0,1, 7, -16, -2, 5, -10, -2, 1, -3, -4, 1, -4, -1, 0, -1, 0, 0, 0, 0, 0, 0, 2, 0, 1};
static signed char RDdctbl27nit[30] = {0, 0, 0, 0, 0, 0,0, 6, -14, -3, 4, -9, -2, 1, -3, -3, 1, -3, -1, 0, -1, 0, 0, 0, 0, 0, 0, 2, 0, 1};
static signed char RDdctbl29nit[30] = {0, 0, 0, 0, 0, 0,0, 7, -16, -3, 5, -11, -3, 1, -3, -3, 1, -3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 1};
static signed char RDdctbl30nit[30] = {0, 0, 0, 0, 0, 0,0, 7, -14, -2, 4, -10, -3, 1, -4, -3, 1, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 1};
static signed char RDdctbl32nit[30] = {0, 0, 0, 0, 0, 0,0, 6, -13, -3, 4, -10, -2, 1, -2, -3, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 1};
static signed char RDdctbl34nit[30] = {0, 0, 0, 0, 0, 0,0, 8, -18, -2, 4, -8, -2, 1, -3, -3, 1, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 1};
static signed char RDdctbl37nit[30] = {0, 0, 0, 0, 0, 0,0, 7, -16, -3, 3, -8, -2, 1, -3, -2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1};
static signed char RDdctbl39nit[30] = {0, 0, 0, 0, 0, 0,0, 8, -17, -2, 4, -8, -1, 1, -2, -2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1};
static signed char RDdctbl41nit[30] = {0, 0, 0, 0, 0, 0,1, 7, -14, -3, 4, -8, -1, 1, -2, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1};
static signed char RDdctbl44nit[30] = {0, 0, 0, 0, 0, 0,0, 6, -14, -4, 3, -8, -1, 0, -2, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1};
static signed char RDdctbl47nit[30] = {0, 0, 0, 0, 0, 0,0, 6, -14, -1, 4, -8, -1, 1, -2, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2};
static signed char RDdctbl50nit[30] = {0, 0, 0, 0, 0, 0,0, 6, -12, -2, 3, -8, -1, 0, -2, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2};
static signed char RDdctbl53nit[30] = {0, 0, 0, 0, 0, 0,-1, 8, -17, -2, 3, -8, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2};
static signed char RDdctbl56nit[30] = {0, 0, 0, 0, 0, 0,-1, 6, -14, -1, 3, -7, -1, 0, -2, -1, 0, 0, 0, 0, 0, 0, 0, -1, 2, 0, 2, 2, 0, 2};
static signed char RDdctbl60nit[30] = {0, 0, 0, 0, 0, 0,-1, 5, -11, -1, 3, -7, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, 2, 0, 2, 2, 0, 2};
static signed char RDdctbl64nit[30] = {0, 0, 0, 0, 0, 0,0, 6, -13, -1, 4, -9, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 2, 0, 2, 2, 0, 2};
static signed char RDdctbl68nit[30] = {0, 0, 0, 0, 0, 0,0, 5, -10, 0, 4, -8, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 2, 0, 2, 2, 0, 2};
static signed char RDdctbl72nit[30] = {0, 0, 0, 0, 0, 0,-1, 4, -10, -1, 3, -8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 2, 0, 2, 2, 0, 2};
static signed char RDdctbl77nit[30] = {0, 0, 0, 0, 0, 0,0, 4, -9, -1, 3, -8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2, 2, 0, 2};
static signed char RDdctbl82nit[30] = {0, 0, 0, 0, 0, 0,-2, 4, -10, 0, 2, -6, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 2, 0, 2, 2, 0, 2};
static signed char RDdctbl87nit[30] = {0, 0, 0, 0, 0, 0,-1, 3, -8, 0, 2, -6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 2, 0, 2};
static signed char RDdctbl93nit[30] = {0, 0, 0, 0, 0, 0,-1, 4, -8, 0, 2, -6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 2, 0, 2};
static signed char RDdctbl98nit[30] = {0, 0, 0, 0, 0, 0,-1, 3, -6, 0, 2, -5, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1};
static signed char RDdctbl105nit[30] = {0, 0, 0, 0, 0, 0,-3, 4, -8, -1, 1, -4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 1};
static signed char RDdctbl110nit[30] = {0, 0, 0, 0, 0, 0,-2, 3, -8, 1, 2, -4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1};
static signed char RDdctbl119nit[30] = {0, 0, 0, 0, 0, 0,-3, 3, -7, -1, 1, -4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1};
static signed char RDdctbl126nit[30] = {0, 0, 0, 0, 0, 0,-4, 3, -8, -1, 1, -4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1};
static signed char RDdctbl134nit[30] = {0, 0, 0, 0, 0, 0,-3, 3, -6, 0, 1, -3, -1, 0, 0, 0, 0, 0, 0, 0, -1, 1, 0, 1, 1, 0, 1, 1, 0, 1};
static signed char RDdctbl143nit[30] = {0, 0, 0, 0, 0, 0,-4, 2, -6, -1, 1, -4, 0, 0, 0, -1, 0, 0, 0, 0, -1, 1, 0, 1, 1, 0, 1, 1, 0, 1};
static signed char RDdctbl152nit[30] = {0, 0, 0, 0, 0, 0,-2, 3, -6, -1, 1, -3, 0, 0, 0, 0, 0, 0, -1, 0, -1, 1, 0, 1, 1, 0, 1, 1, 0, 1};
static signed char RDdctbl162nit[30] = {0, 0, 0, 0, 0, 0,-4, 2, -6, 0, 1, -2, 1, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 0, 1, 0, 1};
static signed char RDdctbl172nit[30] = {0, 0, 0, 0, 0, 0,-3, 2, -6, 0, 1, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1};
static signed char RDdctbl183nit[30] = {0, 0, 0, 0, 0, 0,-3, 2, -4, -1, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1};
static signed char RDdctbl195nit[30] = {0, 0, 0, 0, 0, 0,-2, 2, -4, 0, 0, -2, 0, 0, 0, -1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1};
static signed char RDdctbl207nit[30] = {0, 0, 0, 0, 0, 0,-2, 1, -4, -1, 0, -2, 1, 0, 0, -1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1};
static signed char RDdctbl220nit[30] = {0, 0, 0, 0, 0, 0,-1, 1, -2, 0, 0, -2, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static signed char RDdctbl234nit[30] = {0, 0, 0, 0, 0, 0,-1, 0, -2, 0, 0, -2, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static signed char RDdctbl249nit[30] = {0, 0, 0, 0, 0, 0,0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static signed char RDdctbl265nit[30] = {0, 0, 0, 0, 0, 0,0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static signed char RDdctbl282nit[30] = {0, 0, 0, 0, 0, 0,0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static signed char RDdctbl300nit[30] = {0, 0, 0, 0, 0, 0,0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static signed char RDdctbl316nit[30] = {0, 0, 0, 0, 0, 0,0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static signed char RDdctbl333nit[30] = {0, 0, 0, 0, 0, 0,0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static signed char RDdctbl360nit[30] = {0, 0, 0, 0, 0, 0,0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static signed char aid9798[3] = {0xB2, 0xDC, 0x90};
static signed char aid9720[3] = {0xB2, 0xC8, 0x90};
static signed char aid9612[3] = {0xB2, 0xAC, 0x90};
static signed char aid9530[3] = {0xB2, 0x97, 0x90};
static signed char aid9449[3] = {0xB2, 0x82, 0x90};
static signed char aid9359[3] = {0xB2, 0x6B, 0x90};
static signed char aid9282[3] = {0xB2, 0x57, 0x90};
static signed char aid9208[3] = {0xB2, 0x44, 0x90};
static signed char aid9115[3] = {0xB2, 0x2C, 0x90};
static signed char aid8998[3] = {0xB2, 0x0E, 0x90};
static signed char aid8960[3] = {0xB2, 0x04, 0x90};
static signed char aid8894[3] = {0xB2, 0xF3, 0x80};
// static signed char aid8832[3] = {0xB2, 0xE3, 0x80}; defined in rev.C
static signed char aid8723[3] = {0xB2, 0xC7, 0x80};
static signed char aid8653[3] = {0xB2, 0xB5, 0x80};
static signed char aid8575[3] = {0xB2, 0xA1, 0x80};
// static signed char aid8381[3] = {0xB2, 0x6F, 0x80}; already defined in tulip parameter
static signed char aid8292[3] = {0xB2, 0x58, 0x80};
static signed char aid8218[3] = {0xB2, 0x45, 0x80};
static signed char aid8148[3] = {0xB2, 0x33, 0x80};
static signed char aid7966[3] = {0xB2, 0x04, 0x80};
static signed char aid7865[3] = {0xB2, 0xEA, 0x70};
// static signed char aid7710[3] = {0xB2, 0xC2, 0x70}; already defined in tulip parameter
static signed char aid7512[3] = {0xB2, 0x8F, 0x70};
static signed char aid7384[3] = {0xB2, 0x6E, 0x70};
static signed char aid7267[3] = {0xB2, 0x50, 0x70};
static signed char aid7089[3] = {0xB2, 0x22, 0x70};
static signed char aid6813[3] = {0xB2, 0xDB, 0x60};
static signed char aid6658[3] = {0xB2, 0xB3, 0x60};
static signed char aid6467[3] = {0xB2, 0x82, 0x60};
static signed char aid6188[3] = {0xB2, 0x3A, 0x60};
static signed char aid5920[3] = {0xB2, 0xF5, 0x50};
static signed char aid5637[3] = {0xB2, 0xAC, 0x50};
static signed char aid5365[3] = {0xB2, 0x66, 0x50};
static signed char aid5085[3] = {0xB2, 0x1E, 0x50};
static signed char aid4732[3] = {0xB2, 0xC3, 0x40};
static signed char aid4344[3] = {0xB2, 0x5F, 0x40};
static signed char aid3956[3] = {0xB2, 0xFB, 0x30};

static signed char aid3637[3] = {0xB2, 0xA9, 0x30};
static signed char aid3637_1[3] = {0xB2, 0xA9, 0x30};
static signed char aid3637_2[3] = {0xB2, 0xA9, 0x30};
static signed char aid3637_3[3] = {0xB2, 0xA9, 0x30};
static signed char aid3637_4[3] = {0xB2, 0xA9, 0x30};
static signed char aid3637_5[3] = {0xB2, 0xA9, 0x30};
static signed char aid3637_6[3] = {0xB2, 0xA9, 0x30};
static signed char aid3637_7[3] = {0xB2, 0xA9, 0x30};
static signed char aid3637_8[3] = {0xB2, 0xA9, 0x30};
static signed char aid3637_9[3] = {0xB2, 0xA9, 0x30};
static signed char aid3637_10[3] = {0xB2, 0xA9, 0x30};
static signed char aid3637_11[3] = {0xB2, 0xA9, 0x30};
static signed char aid3637_12[3] = {0xB2, 0xA9, 0x30};
static signed char aid3637_13[3] = {0xB2, 0xA9, 0x30};
static signed char aid3637_14[3] = {0xB2, 0xA9, 0x30};
static signed char aid3637_15[3] = {0xB2, 0xA9, 0x30};

static signed char aid3168[3] = {0xB2, 0x30, 0x30};
static signed char aid2659[3] = {0xB2, 0xAD, 0x20};
static signed char aid2186[3] = {0xB2, 0x33, 0x20};
static signed char aid1580[3] = {0xB2, 0x97, 0x10};
//	static signed char aid1005[3] = {0xB2, 0x03, 0x10};  already defined in tulip parameter

/*
   this parameter is not used. so compile disable
   finally, remove or use
*/

static unsigned char delv1 [3] = {
	0xB6, 0x8C, 0x15
};

static unsigned char delv2 [3] = {
	0xB6, 0x8C, 0x14
};

static unsigned char delv3 [3] = {
	0xB6, 0x8C, 0x13
};

static unsigned char delv4 [3] = {
	0xB6, 0x8C, 0x12
};

static unsigned char delv5 [3] = {
	0xB6, 0x8C, 0x11
};

static unsigned char delv6 [3] = {
	0xB6, 0x8C, 0x10
};

static unsigned char delv7 [3] = {
	0xB6, 0x8C, 0x0F
};

static unsigned char delv8 [3] = {
	0xB6, 0x8C, 0x0E
};

static unsigned char delv9 [3] = {
	0xB6, 0x8C, 0x0D
};

static unsigned char delv10 [3] = {
	0xB6, 0x8C, 0x0C
};

static unsigned char delv11 [3] = {
	0xB6, 0x8C, 0x0B
};

static unsigned char delv12 [3] = {
	0xB6, 0x8C, 0x0A
};

// hbm interpolation
static unsigned char delv13 [3] = {
	0xB6, 0x8C, 0x08
};
static unsigned char delv14 [3] = {
	0xB6, 0x8C, 0x06
};
static unsigned char delv15 [3] = {
	0xB6, 0x8C, 0x05
};
static unsigned char delv16 [3] = {
	0xB6, 0x8C, 0x03
};
static unsigned char delv17 [3] = {
	0xB6, 0x8C, 0x01
};



static unsigned char delvCaps1 [3] = {
	0xB6, 0x9C, 0x15
};

static unsigned char delvCaps2 [3] = {
	0xB6, 0x9C, 0x14
};

static unsigned char delvCaps3 [3] = {
	0xB6, 0x9C, 0x13
};

static unsigned char delvCaps4 [3] = {
	0xB6, 0x9C, 0x12
};

static unsigned char delvCaps5 [3] = {
	0xB6, 0x9C, 0x11
};

static unsigned char delvCaps6 [3] = {
	0xB6, 0x9C, 0x10
};

static unsigned char delvCaps7 [3] = {
	0xB6, 0x9C, 0x0F
};

static unsigned char delvCaps8 [3] = {
	0xB6, 0x9C, 0x0E
};
static unsigned char delvCaps9 [3] = {
	0xB6, 0x9C, 0x0D
};

static unsigned char delvCaps10 [3] = {
	0xB6, 0x9C, 0x0C
};

static unsigned char delvCaps11 [3] = {
	0xB6, 0x9C, 0x0B
};

static unsigned char delvCaps12 [3] = {
	0xB6, 0x9C, 0x0A
};

// hbm interpolation
static unsigned char delvCaps13 [3] = {
	0xB6, 0x9C, 0x08
};
static unsigned char delvCaps14 [3] = {
	0xB6, 0x9C, 0x06
};
static unsigned char delvCaps15 [3] = {
	0xB6, 0x9C, 0x05
};
static unsigned char delvCaps16 [3] = {
	0xB6, 0x9C, 0x03
};
static unsigned char delvCaps17 [3] = {
	0xB6, 0x9C, 0x01
};


#ifdef CONFIG_LCD_HMT

static signed char HMTrtbl10nit[10] = {
	0, 2, 11, 10, 9, 8, 6, 5, 1, 0
};
static signed char HMTrtbl11nit[10] = {
	0, 9, 12, 11, 9, 8, 5, 4, 1, 0
};
static signed char HMTrtbl12nit[10] = {
	0, 8, 12, 10, 9, 7, 6, 5, 2, 0
};
static signed char HMTrtbl13nit[10] = {
	0, 10, 12, 10, 9, 8, 6, 4, 2, 0
};
static signed char HMTrtbl14nit[10] = {
	0, 12, 11, 10, 9, 8, 6, 4, 2, 0
};
static signed char HMTrtbl15nit[10] = {
	0, 12, 11, 10, 8, 7, 6, 5, 2, 0
};
static signed char HMTrtbl16nit[10] = {
	0, 12, 11, 10, 9, 7, 5, 4, 3, 0
};
static signed char HMTrtbl17nit[10] = {
	0, 8, 12, 10, 9, 8, 5, 4, 3, 0
};
static signed char HMTrtbl19nit[10] = {
	0, 11, 12, 10, 9, 8, 6, 4, 3, 0
};
static signed char HMTrtbl20nit[10] = {
	0, 14, 12, 10, 10, 8, 8, 6, 5, 0
};
static signed char HMTrtbl21nit[10] = {
	0, 13, 11, 10, 10, 9, 8, 6, 5, 0
};
static signed char HMTrtbl22nit[10] = {
	0, 9, 12, 10, 9, 8, 8, 6, 4, 0
};
static signed char HMTrtbl23nit[10] = {
	0, 9, 12, 10, 10, 9, 9, 7, 5, 0
};
static signed char HMTrtbl25nit[10] = {
	0, 9, 12, 10, 9, 9, 9, 8, 5, 0
};
static signed char HMTrtbl27nit[10] = {
	0, 11, 12, 10, 10, 9, 9, 8, 5, 0
};
static signed char HMTrtbl29nit[10] = {
	0, 10, 12, 10, 9, 8, 8, 8, 5, 0
};
static signed char HMTrtbl31nit[10] = {
	0, 12, 11, 9, 9, 9, 10, 9, 5, 0
};
static signed char HMTrtbl33nit[10] = {
	0, 13, 11, 10, 9, 9, 10, 10, 5, 0
};
static signed char HMTrtbl35nit[10] = {
	0, 13, 11, 10, 8, 8, 9, 9, 5, 0
};
static signed char HMTrtbl37nit[10] = {
	0, 9, 12, 10, 9, 8, 9, 9, 4, 0
};
static signed char HMTrtbl39nit[10] = {
	0, 9, 12, 10, 9, 9, 10, 9, 4, 0
};
static signed char HMTrtbl41nit[10] = {
	0, 10, 12, 10, 9, 9, 10, 9, 4, 0
};
static signed char HMTrtbl44nit[10] = {
	0, 13, 11, 10, 9, 9, 10, 10, 3, 0
};
static signed char HMTrtbl47nit[10] = {
	0, 13, 11, 10, 9, 9, 11, 10, 5, 0
};
static signed char HMTrtbl50nit[10] = {
	0, 10, 12, 10, 11, 10, 11, 10, 5, 0
};
static signed char HMTrtbl53nit[10] = {
	0, 9, 12, 10, 10, 9, 10, 10, 5, 0
};
static signed char HMTrtbl56nit[10] = {
	0, 10, 11, 10, 10, 9, 11, 10, 6, 0
};
static signed char HMTrtbl60nit[10] = {
	0, 10, 11, 10, 10, 9, 10, 10, 6, 0
};
static signed char HMTrtbl64nit[10] = {
	0, 13, 11, 10, 10, 10, 10, 11, 6, 0
};
static signed char HMTrtbl68nit[10] = {
	0, 9, 12, 11, 10, 11, 11, 11, 6, 0
};
static signed char HMTrtbl72nit[10] = {
	0, 9, 11, 10, 10, 11, 11, 10, 6, 0
};
static signed char HMTrtbl77nit[10] = {
	0, 7, 8, 7, 7, 7, 9, 9, 4, 0
};
static signed char HMTrtbl82nit[10] = {
	0, 7, 7, 7, 6, 7, 9, 9, 4, 0
};
static signed char HMTrtbl87nit[10] = {
	0, 4, 8, 7, 7, 7, 9, 10, 4, 0
};
static signed char HMTrtbl93nit[10] = {
	0, 4, 8, 7, 7, 7, 9, 9, 5, 0
};
static signed char HMTrtbl99nit[10] = {
	0, 7, 8, 7, 8, 8, 9, 9, 6, 0
};
static signed char HMTrtbl105nit[10] = {
	0, 7, 8, 7, 7, 8, 10, 9, 7, 0
};


static signed char HMTctbl10nit[30] = {
	0, 0, 0, 0, 0, 0,6, 4, -8, -4, 5, -11, -7, 5, -10, -6, 4, -8, -2, 2, -5, -2, 0, -3, -1, 0, -2, 0, 0, -2
};
static signed char HMTctbl11nit[30] = {
	0, 0, 0, 0, 0, 0,-1, 6, -13, -6, 5, -11, -7, 4, -9, -6, 3, -8, -3, 2, -4, -2, 0, -2, 0, 0, 0, -1, 0, -3
};
static signed char HMTctbl12nit[30] = {
	0, 0, 0, 0, 0, 0,-2, 6, -13, -5, 5, -10, -6, 4, -8, -5, 4, -8, -2, 2, -4, -1, 0, -2, 0, 0, -1, -1, 0, -2
};
static signed char HMTctbl13nit[30] = {
	0, 0, 0, 0, 0, 0,0, 6, -13, -7, 5, -12, -6, 4, -9, -5, 4, -8, -3, 2, -4, -1, 0, -2, 0, 0, -1, 0, 0, -2
};
static signed char HMTctbl14nit[30] = {
	0, 0, 0, 0, 0, 0,-2, 7, -15, -5, 5, -10, -7, 4, -10, -4, 3, -7, -3, 1, -4, -2, 0, -2, -1, 0, -1, -1, 0, -3
};
static signed char HMTctbl15nit[30] = {
	0, 0, 0, 0, 0, 0,-7, 8, -16, -4, 4, -10, -6, 4, -9, -5, 3, -7, -3, 2, -4, -1, 0, -2, 0, 0, 0, -1, 0, -1
};
static signed char HMTctbl16nit[30] = {
	0, 0, 0, 0, 0, 0,0, 7, -15, -6, 4, -10, -6, 4, -8, -4, 3, -6, -3, 1, -4, 0, 0, -1, -1, 0, -2, 0, 0, -1
};
static signed char HMTctbl17nit[30] = {
	0, 0, 0, 0, 0, 0,-2, 6, -13, -8, 6, -12, -6, 3, -8, -5, 3, -6, -2, 1, -3, -2, 0, -3, 0, 0, -1, 0, 0, -1
};
static signed char HMTctbl19nit[30] = {
	0, 0, 0, 0, 0, 0,-5, 7, -16, -7, 5, -11, -6, 4, -8, -4, 2, -6, -3, 1, -4, -1, 0, -3, -1, 0, -1, 0, 0, -1
};
static signed char HMTctbl20nit[30] = {
	0, 0, 0, 0, 0, 0,-4, 8, -16, -9, 5, -11, -6, 3, -8, -5, 2, -6, -4, 1, -4, 0, 0, -1, -1, 0, -1, 0, 0, -1
};
static signed char HMTctbl21nit[30] = {
	0, 0, 0, 0, 0, 0,-3, 8, -16, -9, 5, -11, -6, 3, -8, -4, 2, -5, -3, 1, -3, -1, 0, -2, 0, 0, -1, 0, 0, -1
};
static signed char HMTctbl22nit[30] = {
	0, 0, 0, 0, 0, 0,-7, 7, -16, -7, 4, -10, -7, 3, -8, -4, 2, -6, -2, 1, -3, -1, 0, -2, 0, 0, -1, 0, 0, -1
};
static signed char HMTctbl23nit[30] = {
	0, 0, 0, 0, 0, 0,-6, 7, -15, -8, 5, -11, -5, 3, -7, -5, 2, -6, -2, 1, -3, 0, 0, -1, -1, 0, -2, 0, 0, -1
};
static signed char HMTctbl25nit[30] = {
	0, 0, 0, 0, 0, 0,-8, 7, -16, -8, 5, -12, -5, 3, -7, -2, 2, -4, -2, 1, -3, -1, 0, -1, 0, 0, 0, 0, 0, -1
};
static signed char HMTctbl27nit[30] = {
	0, 0, 0, 0, 0, 0,-7, 8, -18, -7, 4, -10, -6, 3, -7, -3, 2, -5, -2, 1, -2, -1, 0, -2, 0, 0, 0, 0, 0, -1
};
static signed char HMTctbl29nit[30] = {
	0, 0, 0, 0, 0, 0,-7, 8, -18, -8, 4, -10, -4, 3, -6, -3, 2, -5, -1, 1, -2, 0, 0, -2, 0, 0, 0, 0, 0, 0
};
static signed char HMTctbl31nit[30] = {
	0, 0, 0, 0, 0, 0,-8, 8, -18, -8, 5, -10, -3, 3, -6, -3, 2, -4, -2, 1, -3, -1, 0, -1, 0, 0, -1, 0, 0, 0
};
static signed char HMTctbl33nit[30] = {
	0, 0, 0, 0, 0, 0,-7, 8, -18, -7, 4, -10, -4, 3, -6, -3, 2, -4, -1, 1, -2, -1, 0, -2, 0, 0, 0, 0, 0, -1
};
static signed char HMTctbl35nit[30] = {
	0, 0, 0, 0, 0, 0,-6, 9, -18, -8, 4, -10, -4, 3, -6, -2, 2, -4, -1, 1, -2, 0, 0, -2, 0, 0, 0, 0, 0, 0
};
static signed char HMTctbl37nit[30] = {
	0, 0, 0, 0, 0, 0,-9, 8, -17, -5, 4, -9, -5, 2, -6, -3, 2, -4, -1, 1, -2, 0, 0, -2, 0, 0, 0, 0, 0, -1
};
static signed char HMTctbl39nit[30] = {
	0, 0, 0, 0, 0, 0,-7, 8, -16, -7, 4, -10, -2, 2, -4, -4, 2, -5, -1, 0, -2, 0, 0, -1, 0, 0, 0, 0, 0, -1
};
static signed char HMTctbl41nit[30] = {
	0, 0, 0, 0, 0, 0,-8, 8, -16, -7, 4, -10, -3, 2, -5, -2, 2, -4, -2, 1, -2, 0, 0, -2, 0, 0, 0, 0, 0, -1
};
static signed char HMTctbl44nit[30] = {
	0, 0, 0, 0, 0, 0,-9, 8, -18, -5, 4, -9, -4, 2, -6, -4, 1, -4, 0, 1, -2, -2, 0, -2, 0, 0, 0, 0, 0, -1
};
static signed char HMTctbl47nit[30] = {
	0, 0, 0, 0, 0, 0,-9, 9, -18, -5, 4, -9, -3, 2, -4, -3, 1, -4, -1, 1, -2, 0, 0, -1, 0, 0, 0, 0, 0, 0
};
static signed char HMTctbl50nit[30] = {
	0, 0, 0, 0, 0, 0,-9, 8, -16, -7, 4, -10, -4, 2, -5, -3, 1, -4, 0, 0, -2, -1, 0, -1, 0, 0, -1, 0, 0, -1
};
static signed char HMTctbl53nit[30] = {
	0, 0, 0, 0, 0, 0,-10, 8, -17, -6, 4, -10, -4, 2, -4, -2, 2, -4, -1, 0, -2, 0, 0, -1, 0, 0, 0, 1, 0, 0
};
static signed char HMTctbl56nit[30] = {
	0, 0, 0, 0, 0, 0,-10, 8, -17, -6, 4, -9, -2, 2, -4, -3, 1, -4, 0, 1, -2, 0, 0, 0, 0, 0, -1, 0, 0, -1
};
static signed char HMTctbl60nit[30] = {
	0, 0, 0, 0, 0, 0,-11, 8, -18, -4, 4, -8, -4, 2, -4, -3, 1, -4, 0, 1, -2, 0, 0, 0, 0, 0, 0, 1, 0, 0
};
static signed char HMTctbl64nit[30] = {
	0, 0, 0, 0, 0, 0,-10, 9, -18, -5, 3, -8, -3, 1, -4, -2, 1, -3, 0, 1, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
static signed char HMTctbl68nit[30] = {
	0, 0, 0, 0, 0, 0,-10, 8, -17, -4, 3, -7, -4, 1, -4, -1, 1, -2, 0, 1, -2, -2, 0, -1, 0, 0, -1, 1, 0, 0
};
static signed char HMTctbl72nit[30] = {
	0, 0, 0, 0, 0, 0,-9, 8, -17, -6, 3, -8, -2, 1, -3, -2, 1, -3, -1, 1, -2, -1, 0, -1, 1, 0, 0, 0, 0, 0
};
static signed char HMTctbl77nit[30] = {
	0, 0, 0, 0, 0, 0,-3, 8, -18, -5, 4, -8, -1, 1, -2, -1, 0, -2, 0, 0, -2, -1, 0, -1, 0, 0, 0, 0, 0, 0
};
static signed char HMTctbl82nit[30] = {
	0, 0, 0, 0, 0, 0,-3, 9, -18, -4, 3, -7, -2, 1, -4, -1, 0, -2, 0, 1, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
static signed char HMTctbl87nit[30] = {
	0, 0, 0, 0, 0, 0,-4, 7, -16, -3, 3, -7, -1, 1, -2, -1, 1, -3, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
static signed char HMTctbl93nit[30] = {
	0, 0, 0, 0, 0, 0,-4, 8, -16, -3, 3, -6, 0, 1, -2, -2, 0, -2, 0, 1, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
static signed char HMTctbl99nit[30] = {
	0, 0, 0, 0, 0, 0,-5, 8, -17, -3, 3, -6, -1, 1, -2, -1, 0, -2, 0, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
static signed char HMTctbl105nit[30] = {
	0, 0, 0, 0, 0, 0,-5, 8, -17, -4, 2, -6, 0, 1, -2, -1, 1, -2, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0
};


static unsigned char HMTaid8001[4] = { 0xB2, 0x03, 0x12, 0x03};

static unsigned char HMTaid8001_1[4] = { 0xB2, 0x03, 0x12, 0x03};
static unsigned char HMTaid8001_2[4] = { 0xB2, 0x03, 0x12, 0x03};
static unsigned char HMTaid8001_3[4] = { 0xB2, 0x03, 0x12, 0x03};
static unsigned char HMTaid8001_4[4] = { 0xB2, 0x03, 0x12, 0x03};
static unsigned char HMTaid8001_5[4] = { 0xB2, 0x03, 0x12, 0x03};
static unsigned char HMTaid8001_6[4] = { 0xB2, 0x03, 0x12, 0x03};
static unsigned char HMTaid8001_7[4] = { 0xB2, 0x03, 0x12, 0x03};
static unsigned char HMTaid8001_8[4] = { 0xB2, 0x03, 0x12, 0x03};
static unsigned char HMTaid8001_9[4] = { 0xB2, 0x03, 0x12, 0x03};
static unsigned char HMTaid8001_10[4] = { 0xB2, 0x03, 0x12, 0x03};
static unsigned char HMTaid8001_11[4] = { 0xB2, 0x03, 0x12, 0x03};
static unsigned char HMTaid8001_12[4] = { 0xB2, 0x03, 0x12, 0x03};
static unsigned char HMTaid8001_13[4] = { 0xB2, 0x03, 0x12, 0x03};
static unsigned char HMTaid8001_14[4] = { 0xB2, 0x03, 0x12, 0x03};
static unsigned char HMTaid8001_15[4] = { 0xB2, 0x03, 0x12, 0x03};
static unsigned char HMTaid8001_16[4] = { 0xB2, 0x03, 0x12, 0x03};
static unsigned char HMTaid8001_17[4] = { 0xB2, 0x03, 0x12, 0x03};
static unsigned char HMTaid8001_18[4] = { 0xB2, 0x03, 0x12, 0x03};
static unsigned char HMTaid8001_19[4] = { 0xB2, 0x03, 0x12, 0x03};
static unsigned char HMTaid8001_20[4] = { 0xB2, 0x03, 0x12, 0x03};
static unsigned char HMTaid8001_21[4] = { 0xB2, 0x03, 0x12, 0x03};
static unsigned char HMTaid8001_22[4] = { 0xB2, 0x03, 0x12, 0x03};
static unsigned char HMTaid8001_23[4] = { 0xB2, 0x03, 0x12, 0x03};
static unsigned char HMTaid8001_24[4] = { 0xB2, 0x03, 0x12, 0x03};
static unsigned char HMTaid8001_25[4] = { 0xB2, 0x03, 0x12, 0x03};
static unsigned char HMTaid8001_26[4] = { 0xB2, 0x03, 0x12, 0x03};
static unsigned char HMTaid8001_27[4] = { 0xB2, 0x03, 0x12, 0x03};
static unsigned char HMTaid8001_28[4] = { 0xB2, 0x03, 0x12, 0x03};
static unsigned char HMTaid8001_29[4] = { 0xB2, 0x03, 0x12, 0x03};
static unsigned char HMTaid8001_30[4] = { 0xB2, 0x03, 0x12, 0x03};

static unsigned char HMTaid7003[4] = {0xB2, 0x03, 0x13, 0x04};
static unsigned char HMTaid7003_1[4] = {0xB2, 0x03, 0x13, 0x04};
static unsigned char HMTaid7003_2[4] = {0xB2, 0x03, 0x13, 0x04};
static unsigned char HMTaid7003_3[4] = {0xB2, 0x03, 0x13, 0x04};
static unsigned char HMTaid7003_4[4] = {0xB2, 0x03, 0x13, 0x04};
static unsigned char HMTaid7003_5[4] = {0xB2, 0x03, 0x13, 0x04};
static unsigned char HMTelv[3] = {
	0xB6, 0x8C, 0x0A
};

static unsigned char HMTelvCaps[3] = {
	0xB6, 0x9C, 0x0A
};

#endif

#endif
