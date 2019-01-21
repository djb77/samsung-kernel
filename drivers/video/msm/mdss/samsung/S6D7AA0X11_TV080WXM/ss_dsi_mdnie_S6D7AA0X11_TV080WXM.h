/*
 * =================================================================
 *
 *
 *	Description:  samsung display common file
 *
 *	Author: jb09.kim
 *	Company:  Samsung Electronics
 *
 * ================================================================
 */
/*
<one line to give the program's name and a brief idea of what it does.>
Copyright (C) 2012, Samsung Electronics. All rights reserved.

*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
*/

#ifndef _SAMSUNG_DSI_MDNIE_S6D7AA0X11_TV080WXM_
#define _SAMSUNG_DSI_MDNIE_S6D7AA0X11_TV080WXM_

#include "../ss_dsi_mdnie_lite_common.h"

#define MDNIE_COLOR_BLINDE_CMD_OFFSET 1

#define ADDRESS_SCR_WHITE_RED   0x13
#define ADDRESS_SCR_WHITE_GREEN 0x15
#define ADDRESS_SCR_WHITE_BLUE  0x17

#define MDNIE_RGB_SENSOR_INDEX	0

#define MDNIE_STEP1_INDEX 1
#define MDNIE_STEP2_INDEX 2


static unsigned char LEVEL_UNLOCK[] = {
	0xF0,
	0x5A, 0x5A
};

static unsigned char LEVEL_LOCK[] = {
	0xF0,
	0xA5, 0xA5
};

static char night_mode_data[] = {
	0x00, 0xff, 0xfd, 0x00, 0xf9, 0x00, 0xff, 0x00, 0x00, 0xfd, 0xf9, 0x00, 0xff, 0x00, 0xfd, 0x00, 0x00, 0xf9, 0xff, 0x00, 0xfd, 0x00, 0xf9, 0x00, /* 6500K */
	0x00, 0xff, 0xfa, 0x00, 0xf0, 0x00, 0xff, 0x00, 0x00, 0xfa, 0xf0, 0x00, 0xff, 0x00, 0xfa, 0x00, 0x00, 0xf0, 0xff, 0x00, 0xfa, 0x00, 0xf0, 0x00, /* 6100K */
	0x00, 0xff, 0xf6, 0x00, 0xe5, 0x00, 0xff, 0x00, 0x00, 0xf6, 0xe5, 0x00, 0xff, 0x00, 0xf6, 0x00, 0x00, 0xe5, 0xff, 0x00, 0xf6, 0x00, 0xe5, 0x00, /* 5700K */
	0x00, 0xff, 0xf2, 0x00, 0xda, 0x00, 0xff, 0x00, 0x00, 0xf2, 0xda, 0x00, 0xff, 0x00, 0xf2, 0x00, 0x00, 0xda, 0xff, 0x00, 0xf2, 0x00, 0xda, 0x00, /* 5300K */
	0x00, 0xff, 0xec, 0x00, 0xce, 0x00, 0xff, 0x00, 0x00, 0xec, 0xce, 0x00, 0xff, 0x00, 0xec, 0x00, 0x00, 0xce, 0xff, 0x00, 0xec, 0x00, 0xce, 0x00, /* 4900K */
	0x00, 0xff, 0xe6, 0x00, 0xbe, 0x00, 0xff, 0x00, 0x00, 0xe6, 0xbe, 0x00, 0xff, 0x00, 0xe6, 0x00, 0x00, 0xbe, 0xff, 0x00, 0xe6, 0x00, 0xbe, 0x00, /* 4500K */
	0x00, 0xff, 0xe0, 0x00, 0xad, 0x00, 0xff, 0x00, 0x00, 0xe0, 0xad, 0x00, 0xff, 0x00, 0xe0, 0x00, 0x00, 0xad, 0xff, 0x00, 0xe0, 0x00, 0xad, 0x00, /* 4100K */
	0x00, 0xff, 0xd6, 0x00, 0x9b, 0x00, 0xff, 0x00, 0x00, 0xd6, 0x9b, 0x00, 0xff, 0x00, 0xd6, 0x00, 0x00, 0x9b, 0xff, 0x00, 0xd6, 0x00, 0x9b, 0x00, /* 3700K */
	0x00, 0xff, 0xcd, 0x00, 0x85, 0x00, 0xff, 0x00, 0x00, 0xcd, 0x85, 0x00, 0xff, 0x00, 0xcd, 0x00, 0x00, 0x85, 0xff, 0x00, 0xcd, 0x00, 0x85, 0x00, /* 3300K */
	0x00, 0xff, 0xc2, 0x00, 0x71, 0x00, 0xff, 0x00, 0x00, 0xc2, 0x71, 0x00, 0xff, 0x00, 0xc2, 0x00, 0x00, 0x71, 0xff, 0x00, 0xc2, 0x00, 0x71, 0x00, /* 2900K */
	0x00, 0xff, 0xb4, 0x00, 0x5c, 0x00, 0xff, 0x00, 0x00, 0xb4, 0x5c, 0x00, 0xff, 0x00, 0xb4, 0x00, 0x00, 0x5c, 0xff, 0x00, 0xb4, 0x00, 0x5c, 0x00, /* 2500K */
};

static unsigned char BYPASS_1[] ={
//start
0xE8,
0x00, //roi0 x start
0x00,
0x00, //roi0 x end
0x00,
0x00, //roi0 y start
0x00,
0x00, //roi0 y end
0x00,
0x00, //roi1 x strat
0x00,
0x00, //roi1 x end
0x00,
0x00, //roi1 y start
0x00,
0x00, //roi1 y end
0x00,
};

static unsigned char BYPASS_2[] ={
0xE9,
0x00, //scr Cr Yb
0xff, //scr Rr Bb
0xff, //scr Cg Yg
0x00, //scr Rg Bg
0xff, //scr Cb Yr
0x00, //scr Rb Br
0xff, //scr Mr Mb
0x00, //scr Gr Gb
0x00, //scr Mg Mg
0xff, //scr Gg Gg
0xff, //scr Mb Mr
0x00, //scr Gb Gr
0xff, //scr Yr Cb
0x00, //scr Br Rb
0xff, //scr Yg Cg
0x00, //scr Bg Rg
0x00, //scr Yb Cr
0xff, //scr Bb Rr
0xff, //scr Wr Wb
0x00, //scr Kr Kb
0xff, //scr Wg Wg
0x00, //scr Kg Kg
0xff, //scr Wb Wr
0x00, //scr Kb Kr
};

static unsigned char BYPASS_3[] ={
0xEA,
0x00, //curve 1 b
0x20, //curve 1 a
0x00, //curve 2 b
0x20, //curve 2 a
0x00, //curve 3 b
0x20, //curve 3 a
0x00, //curve 4 b
0x20, //curve 4 a
0x00, //curve 5 b
0x20, //curve 5 a
0x00, //curve 6 b
0x20, //curve 6 a
0x00, //curve 7 b
0x20, //curve 7 a
0x00, //curve 8 b
0x20, //curve 8 a
0x00, //curve 9 b
0x20, //curve 9 a
0x00, //curve10 b
0x20, //curve10 a
0x00, //curve11 b
0x20, //curve11 a
0x00, //curve12 b
0x20, //curve12 a
};

static unsigned char BYPASS_4[] ={
0xEB,
0x00, //curve13 b
0x20, //curve13 a
0x00, //curve14 b
0x20, //curve14 a
0x00, //curve15 b
0x20, //curve15 a
0x00, //curve16 b
0x20, //curve16 a
0x00, //curve17 b
0x20, //curve17 a
0x00, //curve18 b
0x20, //curve18 a
0x00, //curve19 b
0x20, //curve19 a
0x00, //curve20 b
0x20, //curve20 a
0x00, //curve21 b
0x20, //curve21 a
0x00, //curve22 b
0x20, //curve22 a
0x00, //curve23 b
0x20, //curve23 a
0x00, //curve24 b
0xFF, //curve24 a
};

static unsigned char BYPASS_5[] ={
0xEC,
0x04, //cc r1
0x00,
0x00, //cc r2
0x00,
0x00, //cc r3
0x00,
0x00, //cc g1
0x00,
0x04, //cc g2
0x00,
0x00, //cc g3
0x00,
0x00, //cc b1
0x00,
0x00, //cc b2
0x00,
0x04, //cc b3
0x00,
};

static unsigned char BYPASS_6[] ={
0xE7,
0x08, //roi_ctrl rgb_if_type mdnie_en mask 00 00 0 000
0x00, //scr_roi 1 scr algo_roi 1 algo 00 1 0 00 1 0
0x03, //HSIZE
0x00,
0x04, //VSIZE
0x00,
0x00, //sharpen cc gamma 00 0 0
//end
};

static unsigned char NEGATIVE_1[] ={
//start
0xE8,
0x00, //roi0 x start
0x00,
0x00, //roi0 x end
0x00,
0x00, //roi0 y start
0x00,
0x00, //roi0 y end
0x00,
0x00, //roi1 x strat
0x00,
0x00, //roi1 x end
0x00,
0x00, //roi1 y start
0x00,
0x00, //roi1 y end
0x00,
};

static unsigned char NEGATIVE_2[] ={
0xE9,
0x00, //scr Cr Yb
0xff, //scr Rr Bb
0xff, //scr Cg Yg
0x00, //scr Rg Bg
0xff, //scr Cb Yr
0x00, //scr Rb Br
0xff, //scr Mr Mb
0x00, //scr Gr Gb
0x00, //scr Mg Mg
0xff, //scr Gg Gg
0xff, //scr Mb Mr
0x00, //scr Gb Gr
0xff, //scr Yr Cb
0x00, //scr Br Rb
0xff, //scr Yg Cg
0x00, //scr Bg Rg
0x00, //scr Yb Cr
0xff, //scr Bb Rr
0x00, //scr Wr Wb
0xff, //scr Kr Kb
0x00, //scr Wg Wg
0xff, //scr Kg Kg
0x00, //scr Wb Wr
0xff, //scr Kb Kr
};

static unsigned char NEGATIVE_3[] ={
0xEA,
0x00, //curve 1 b
0x20, //curve 1 a
0x00, //curve 2 b
0x20, //curve 2 a
0x00, //curve 3 b
0x20, //curve 3 a
0x00, //curve 4 b
0x20, //curve 4 a
0x00, //curve 5 b
0x20, //curve 5 a
0x00, //curve 6 b
0x20, //curve 6 a
0x00, //curve 7 b
0x20, //curve 7 a
0x00, //curve 8 b
0x20, //curve 8 a
0x00, //curve 9 b
0x20, //curve 9 a
0x00, //curve10 b
0x20, //curve10 a
0x00, //curve11 b
0x20, //curve11 a
0x00, //curve12 b
0x20, //curve12 a
};

static unsigned char NEGATIVE_4[] ={
0xEB,
0x00, //curve13 b
0x20, //curve13 a
0x00, //curve14 b
0x20, //curve14 a
0x00, //curve15 b
0x20, //curve15 a
0x00, //curve16 b
0x20, //curve16 a
0x00, //curve17 b
0x20, //curve17 a
0x00, //curve18 b
0x20, //curve18 a
0x00, //curve19 b
0x20, //curve19 a
0x00, //curve20 b
0x20, //curve20 a
0x00, //curve21 b
0x20, //curve21 a
0x00, //curve22 b
0x20, //curve22 a
0x00, //curve23 b
0x20, //curve23 a
0x00, //curve24 b
0xFF, //curve24 a
};

static unsigned char NEGATIVE_5[] ={
0xEC,
0x04, //cc r1
0x00,
0x00, //cc r2
0x00,
0x00, //cc r3
0x00,
0x00, //cc g1
0x00,
0x04, //cc g2
0x00,
0x00, //cc g3
0x00,
0x00, //cc b1
0x00,
0x00, //cc b2
0x00,
0x04, //cc b3
0x00,
};

static unsigned char NEGATIVE_6[] ={
0xE7,
0x08, //roi_ctrl rgb_if_type mdnie_en mask 00 00 0 000
0x30, //scr_roi 1 scr algo_roi 1 algo 00 1 0 00 1 0
0x03, //HSIZE
0x00,
0x04, //VSIZE
0x00,
0x00, //sharpen cc gamma 00 0 0
//end
};

static char COLOR_BLIND_1[] ={
//start
0xE8,
0x00, //roi0 x start
0x00,
0x00, //roi0 x end
0x00,
0x00, //roi0 y start
0x00,
0x00, //roi0 y end
0x00,
0x00, //roi1 x strat
0x00,
0x00, //roi1 x end
0x00,
0x00, //roi1 y start
0x00,
0x00, //roi1 y end
0x00,
};

static char COLOR_BLIND_2[] ={
0xE9,
0x00, //scr Cr Yb
0xff, //scr Rr Bb
0xff, //scr Cg Yg
0x00, //scr Rg Bg
0xff, //scr Cb Yr
0x00, //scr Rb Br
0xff, //scr Mr Mb
0x00, //scr Gr Gb
0x00, //scr Mg Mg
0xff, //scr Gg Gg
0xff, //scr Mb Mr
0x00, //scr Gb Gr
0xff, //scr Yr Cb
0x00, //scr Br Rb
0xff, //scr Yg Cg
0x00, //scr Bg Rg
0x00, //scr Yb Cr
0xff, //scr Bb Rr
0xff, //scr Wr Wb
0x00, //scr Kr Kb
0xff, //scr Wg Wg
0x00, //scr Kg Kg
0xff, //scr Wb Wr
0x00, //scr Kb Kr
};

static char COLOR_BLIND_3[] ={
0xEA,
0x00, //curve 1 b
0x20, //curve 1 a
0x00, //curve 2 b
0x20, //curve 2 a
0x00, //curve 3 b
0x20, //curve 3 a
0x00, //curve 4 b
0x20, //curve 4 a
0x00, //curve 5 b
0x20, //curve 5 a
0x00, //curve 6 b
0x20, //curve 6 a
0x00, //curve 7 b
0x20, //curve 7 a
0x00, //curve 8 b
0x20, //curve 8 a
0x00, //curve 9 b
0x20, //curve 9 a
0x00, //curve10 b
0x20, //curve10 a
0x00, //curve11 b
0x20, //curve11 a
0x00, //curve12 b
0x20, //curve12 a
};

static char COLOR_BLIND_4[] ={
0xEB,
0x00, //curve13 b
0x20, //curve13 a
0x00, //curve14 b
0x20, //curve14 a
0x00, //curve15 b
0x20, //curve15 a
0x00, //curve16 b
0x20, //curve16 a
0x00, //curve17 b
0x20, //curve17 a
0x00, //curve18 b
0x20, //curve18 a
0x00, //curve19 b
0x20, //curve19 a
0x00, //curve20 b
0x20, //curve20 a
0x00, //curve21 b
0x20, //curve21 a
0x00, //curve22 b
0x20, //curve22 a
0x00, //curve23 b
0x20, //curve23 a
0x00, //curve24 b
0xFF, //curve24 a
};

static char COLOR_BLIND_5[] ={
0xEC,
0x04, //cc r1
0x00,
0x00, //cc r2
0x00,
0x00, //cc r3
0x00,
0x00, //cc g1
0x00,
0x04, //cc g2
0x00,
0x00, //cc g3
0x00,
0x00, //cc b1
0x00,
0x00, //cc b2
0x00,
0x04, //cc b3
0x00,
};

static char COLOR_BLIND_6[] ={
0xE7,
0x08, //roi_ctrl rgb_if_type mdnie_en mask 00 00 0 000
0x30, //scr_roi 1 scr algo_roi 1 algo 00 1 0 00 1 0
0x03, //HSIZE
0x00,
0x04, //VSIZE
0x00,
0x00, //sharpen cc gamma 00 0 0
//end
};

static unsigned char GRAYSCALE_1[] ={
//start
0xE8,
0x00, //roi0 x start
0x00,
0x00, //roi0 x end
0x00,
0x00, //roi0 y start
0x00,
0x00, //roi0 y end
0x00,
0x00, //roi1 x strat
0x00,
0x00, //roi1 x end
0x00,
0x00, //roi1 y start
0x00,
0x00, //roi1 y end
0x00,
};

static unsigned char GRAYSCALE_2[] ={
0xE9,
0xb3, //scr Cr Yb
0x4c, //scr Rr Bb
0xb3, //scr Cg Yg
0x4c, //scr Rg Bg
0xb3, //scr Cb Yr
0x4c, //scr Rb Br
0x69, //scr Mr Mb
0x96, //scr Gr Gb
0x69, //scr Mg Mg
0x96, //scr Gg Gg
0x69, //scr Mb Mr
0x96, //scr Gb Gr
0xe2, //scr Yr Cb
0x1d, //scr Br Rb
0xe2, //scr Yg Cg
0x1d, //scr Bg Rg
0xe2, //scr Yb Cr
0x1d, //scr Bb Rr
0xff, //scr Wr Wb
0x00, //scr Kr Kb
0xff, //scr Wg Wg
0x00, //scr Kg Kg
0xff, //scr Wb Wr
0x00, //scr Kb Kr
};

static unsigned char GRAYSCALE_3[] ={
0xEA,
0x00, //curve 1 b
0x20, //curve 1 a
0x00, //curve 2 b
0x20, //curve 2 a
0x00, //curve 3 b
0x20, //curve 3 a
0x00, //curve 4 b
0x20, //curve 4 a
0x00, //curve 5 b
0x20, //curve 5 a
0x00, //curve 6 b
0x20, //curve 6 a
0x00, //curve 7 b
0x20, //curve 7 a
0x00, //curve 8 b
0x20, //curve 8 a
0x00, //curve 9 b
0x20, //curve 9 a
0x00, //curve10 b
0x20, //curve10 a
0x00, //curve11 b
0x20, //curve11 a
0x00, //curve12 b
0x20, //curve12 a
};

static unsigned char GRAYSCALE_4[] ={
0xEB,
0x00, //curve13 b
0x20, //curve13 a
0x00, //curve14 b
0x20, //curve14 a
0x00, //curve15 b
0x20, //curve15 a
0x00, //curve16 b
0x20, //curve16 a
0x00, //curve17 b
0x20, //curve17 a
0x00, //curve18 b
0x20, //curve18 a
0x00, //curve19 b
0x20, //curve19 a
0x00, //curve20 b
0x20, //curve20 a
0x00, //curve21 b
0x20, //curve21 a
0x00, //curve22 b
0x20, //curve22 a
0x00, //curve23 b
0x20, //curve23 a
0x00, //curve24 b
0xFF, //curve24 a
};

static unsigned char GRAYSCALE_5[] ={
0xEC,
0x04, //cc r1
0x00,
0x00, //cc r2
0x00,
0x00, //cc r3
0x00,
0x00, //cc g1
0x00,
0x04, //cc g2
0x00,
0x00, //cc g3
0x00,
0x00, //cc b1
0x00,
0x00, //cc b2
0x00,
0x04, //cc b3
0x00,
};

static unsigned char GRAYSCALE_6[] ={
0xE7,
0x08, //roi_ctrl rgb_if_type mdnie_en mask 00 00 0 000
0x30, //scr_roi 1 scr algo_roi 1 algo 00 1 0 00 1 0
0x03, //HSIZE
0x00,
0x04, //VSIZE
0x00,
0x00, //sharpen cc gamma 00 0 0
//end
};

static unsigned char GRAYSCALE_NEGATIVE_1[] ={
//start
0xE8,
0x00, //roi0 x start
0x00,
0x00, //roi0 x end
0x00,
0x00, //roi0 y start
0x00,
0x00, //roi0 y end
0x00,
0x00, //roi1 x strat
0x00,
0x00, //roi1 x end
0x00,
0x00, //roi1 y start
0x00,
0x00, //roi1 y end
0x00,
};

static unsigned char GRAYSCALE_NEGATIVE_2[] ={
0xE9,
0x4c, //scr Cr Yb
0xb3, //scr Rr Bb
0x4c, //scr Cg Yg
0xb3, //scr Rg Bg
0x4c, //scr Cb Yr
0xb3, //scr Rb Br
0x96, //scr Mr Mb
0x69, //scr Gr Gb
0x96, //scr Mg Mg
0x69, //scr Gg Gg
0x96, //scr Mb Mr
0x69, //scr Gb Gr
0x1d, //scr Yr Cb
0xe2, //scr Br Rb
0x1d, //scr Yg Cg
0xe2, //scr Bg Rg
0x1d, //scr Yb Cr
0xe2, //scr Bb Rr
0x00, //scr Wr Wb
0xff, //scr Kr Kb
0x00, //scr Wg Wg
0xff, //scr Kg Kg
0x00, //scr Wb Wr
0xff, //scr Kb Kr
};

static unsigned char GRAYSCALE_NEGATIVE_3[] ={
0xEA,
0x00, //curve 1 b
0x20, //curve 1 a
0x00, //curve 2 b
0x20, //curve 2 a
0x00, //curve 3 b
0x20, //curve 3 a
0x00, //curve 4 b
0x20, //curve 4 a
0x00, //curve 5 b
0x20, //curve 5 a
0x00, //curve 6 b
0x20, //curve 6 a
0x00, //curve 7 b
0x20, //curve 7 a
0x00, //curve 8 b
0x20, //curve 8 a
0x00, //curve 9 b
0x20, //curve 9 a
0x00, //curve10 b
0x20, //curve10 a
0x00, //curve11 b
0x20, //curve11 a
0x00, //curve12 b
0x20, //curve12 a
};

static unsigned char GRAYSCALE_NEGATIVE_4[] ={
0xEB,
0x00, //curve13 b
0x20, //curve13 a
0x00, //curve14 b
0x20, //curve14 a
0x00, //curve15 b
0x20, //curve15 a
0x00, //curve16 b
0x20, //curve16 a
0x00, //curve17 b
0x20, //curve17 a
0x00, //curve18 b
0x20, //curve18 a
0x00, //curve19 b
0x20, //curve19 a
0x00, //curve20 b
0x20, //curve20 a
0x00, //curve21 b
0x20, //curve21 a
0x00, //curve22 b
0x20, //curve22 a
0x00, //curve23 b
0x20, //curve23 a
0x00, //curve24 b
0xFF, //curve24 a
};

static unsigned char GRAYSCALE_NEGATIVE_5[] ={
0xEC,
0x04, //cc r1
0x00,
0x00, //cc r2
0x00,
0x00, //cc r3
0x00,
0x00, //cc g1
0x00,
0x04, //cc g2
0x00,
0x00, //cc g3
0x00,
0x00, //cc b1
0x00,
0x00, //cc b2
0x00,
0x04, //cc b3
0x00,
};

static unsigned char GRAYSCALE_NEGATIVE_6[] ={
0xE7,
0x08, //roi_ctrl rgb_if_type mdnie_en mask 00 00 0 000
0x30, //scr_roi 1 scr algo_roi 1 algo 00 1 0 00 1 0
0x03, //HSIZE
0x00,
0x04, //VSIZE
0x00,
0x00, //sharpen cc gamma 00 0 0
//end
};

static char NIGHT_MODE_1[] ={
//start
0xE8,
0x00, //roi0 x start
0x00,
0x00, //roi0 x end
0x00,
0x00, //roi0 y start
0x00,
0x00, //roi0 y end
0x00,
0x00, //roi1 x strat
0x00,
0x00, //roi1 x end
0x00,
0x00, //roi1 y start
0x00,
0x00, //roi1 y end
0x00,
};

static char NIGHT_MODE_2[] ={
0xE9,
0x00, //scr Cr Yb
0xff, //scr Rr Bb
0xff, //scr Cg Yg
0x00, //scr Rg Bg
0xff, //scr Cb Yr
0x00, //scr Rb Br
0xff, //scr Mr Mb
0x00, //scr Gr Gb
0x00, //scr Mg Mg
0xff, //scr Gg Gg
0xff, //scr Mb Mr
0x00, //scr Gb Gr
0xff, //scr Yr Cb
0x00, //scr Br Rb
0xff, //scr Yg Cg
0x00, //scr Bg Rg
0x00, //scr Yb Cr
0xff, //scr Bb Rr
0xff, //scr Wr Wb
0x00, //scr Kr Kb
0xff, //scr Wg Wg
0x00, //scr Kg Kg
0xff, //scr Wb Wr
0x00, //scr Kb Kr
};

static char NIGHT_MODE_3[] ={
0xEA,
0x00, //curve 1 b
0x20, //curve 1 a
0x00, //curve 2 b
0x20, //curve 2 a
0x00, //curve 3 b
0x20, //curve 3 a
0x00, //curve 4 b
0x20, //curve 4 a
0x00, //curve 5 b
0x20, //curve 5 a
0x00, //curve 6 b
0x20, //curve 6 a
0x00, //curve 7 b
0x20, //curve 7 a
0x00, //curve 8 b
0x20, //curve 8 a
0x00, //curve 9 b
0x20, //curve 9 a
0x00, //curve10 b
0x20, //curve10 a
0x00, //curve11 b
0x20, //curve11 a
0x00, //curve12 b
0x20, //curve12 a
};

static char NIGHT_MODE_4[] ={
0xEB,
0x00, //curve13 b
0x20, //curve13 a
0x00, //curve14 b
0x20, //curve14 a
0x00, //curve15 b
0x20, //curve15 a
0x00, //curve16 b
0x20, //curve16 a
0x00, //curve17 b
0x20, //curve17 a
0x00, //curve18 b
0x20, //curve18 a
0x00, //curve19 b
0x20, //curve19 a
0x00, //curve20 b
0x20, //curve20 a
0x00, //curve21 b
0x20, //curve21 a
0x00, //curve22 b
0x20, //curve22 a
0x00, //curve23 b
0x20, //curve23 a
0x00, //curve24 b
0xFF, //curve24 a
};

static char NIGHT_MODE_5[] ={
0xEC,
0x04, //cc r1
0x00,
0x00, //cc r2
0x00,
0x00, //cc r3
0x00,
0x00, //cc g1
0x00,
0x04, //cc g2
0x00,
0x00, //cc g3
0x00,
0x00, //cc b1
0x00,
0x00, //cc b2
0x00,
0x04, //cc b3
0x00,
};

static char NIGHT_MODE_6[] ={
0xE7,
0x08, //roi_ctrl rgb_if_type mdnie_en mask 00 00 0 000
0x30, //scr_roi 1 scr algo_roi 1 algo 00 1 0 00 1 0
0x03, //HSIZE
0x00,
0x04, //VSIZE
0x00,
0x00, //sharpen cc gamma 00 0 0
//end
};

static char LIGHT_NOTIFICATION_1[] ={
//start
0xE8,
0x00, //roi0 x start
0x00,
0x00, //roi0 x end
0x00,
0x00, //roi0 y start
0x00,
0x00, //roi0 y end
0x00,
0x00, //roi1 x strat
0x00,
0x00, //roi1 x end
0x00,
0x00, //roi1 y start
0x00,
0x00, //roi1 y end
0x00,
};

static char LIGHT_NOTIFICATION_2[] ={
0xE9,
0x66, //scr Cr Yb
0xff, //scr Rr Bb
0xf9, //scr Cg Yg
0x60, //scr Rg Bg
0xac, //scr Cb Yr
0x13, //scr Rb Br
0xff, //scr Mr Mb
0x66, //scr Gr Gb
0x60, //scr Mg Mg
0xf9, //scr Gg Gg
0xac, //scr Mb Mr
0x13, //scr Gb Gr
0xff, //scr Yr Cb
0x66, //scr Br Rb
0xf9, //scr Yg Cg
0x60, //scr Bg Rg
0x13, //scr Yb Cr
0xac, //scr Bb Rr
0xff, //scr Wr Wb
0x66, //scr Kr Kb
0xf9, //scr Wg Wg
0x60, //scr Kg Kg
0xac, //scr Wb Wr
0x13, //scr Kb Kr
};

static char LIGHT_NOTIFICATION_3[] ={
0xEA,
0x00, //curve 1 b
0x20, //curve 1 a
0x00, //curve 2 b
0x20, //curve 2 a
0x00, //curve 3 b
0x20, //curve 3 a
0x00, //curve 4 b
0x20, //curve 4 a
0x00, //curve 5 b
0x20, //curve 5 a
0x00, //curve 6 b
0x20, //curve 6 a
0x00, //curve 7 b
0x20, //curve 7 a
0x00, //curve 8 b
0x20, //curve 8 a
0x00, //curve 9 b
0x20, //curve 9 a
0x00, //curve10 b
0x20, //curve10 a
0x00, //curve11 b
0x20, //curve11 a
0x00, //curve12 b
0x20, //curve12 a
};

static char LIGHT_NOTIFICATION_4[] ={
0xEB,
0x00, //curve13 b
0x20, //curve13 a
0x00, //curve14 b
0x20, //curve14 a
0x00, //curve15 b
0x20, //curve15 a
0x00, //curve16 b
0x20, //curve16 a
0x00, //curve17 b
0x20, //curve17 a
0x00, //curve18 b
0x20, //curve18 a
0x00, //curve19 b
0x20, //curve19 a
0x00, //curve20 b
0x20, //curve20 a
0x00, //curve21 b
0x20, //curve21 a
0x00, //curve22 b
0x20, //curve22 a
0x00, //curve23 b
0x20, //curve23 a
0x00, //curve24 b
0xFF, //curve24 a
};

static char LIGHT_NOTIFICATION_5[] ={
0xEC,
0x04, //cc r1
0x00,
0x00, //cc r2
0x00,
0x00, //cc r3
0x00,
0x00, //cc g1
0x00,
0x04, //cc g2
0x00,
0x00, //cc g3
0x00,
0x00, //cc b1
0x00,
0x00, //cc b2
0x00,
0x04, //cc b3
0x00,
};

static char LIGHT_NOTIFICATION_6[] ={
0xE7,
0x08, //roi_ctrl rgb_if_type mdnie_en mask 00 00 0 000
0x30, //scr_roi 1 scr algo_roi 1 algo 00 1 0 00 1 0
0x03, //HSIZE
0x00,
0x04, //VSIZE
0x00,
0x00, //sharpen cc gamma 00 0 0
//end
};

static char OUTDOOR_1[] ={
//start
0xE8,
0x00, //roi0 x start
0x00,
0x00, //roi0 x end
0x00,
0x00, //roi0 y start
0x00,
0x00, //roi0 y end
0x00,
0x00, //roi1 x strat
0x00,
0x00, //roi1 x end
0x00,
0x00, //roi1 y start
0x00,
0x00, //roi1 y end
0x00,
};

static char OUTDOOR_2[] ={
0xE9,
0x00, //scr Cr Yb
0xff, //scr Rr Bb
0xff, //scr Cg Yg
0x00, //scr Rg Bg
0xff, //scr Cb Yr
0x00, //scr Rb Br
0xff, //scr Mr Mb
0x00, //scr Gr Gb
0x00, //scr Mg Mg
0xff, //scr Gg Gg
0xff, //scr Mb Mr
0x00, //scr Gb Gr
0xff, //scr Yr Cb
0x00, //scr Br Rb
0xff, //scr Yg Cg
0x00, //scr Bg Rg
0x00, //scr Yb Cr
0xff, //scr Bb Rr
0xff, //scr Wr Wb
0x00, //scr Kr Kb
0xff, //scr Wg Wg
0x00, //scr Kg Kg
0xff, //scr Wb Wr
0x00, //scr Kb Kr
};

static char OUTDOOR_3[] ={
0xEA,
0x00, //curve 1 b
0x7b, //curve 1 a
0x03, //curve 2 b
0x48, //curve 2 a
0x08, //curve 3 b
0x32, //curve 3 a
0x08, //curve 4 b
0x32, //curve 4 a
0x08, //curve 5 b
0x32, //curve 5 a
0x08, //curve 6 b
0x32, //curve 6 a
0x08, //curve 7 b
0x32, //curve 7 a
0x10, //curve 8 b
0x28, //curve 8 a
0x10, //curve 9 b
0x28, //curve 9 a
0x10, //curve10 b
0x28, //curve10 a
0x10, //curve11 b
0x28, //curve11 a
0x10, //curve12 b
0x28, //curve12 a
};

static char OUTDOOR_4[] ={
0xEB,
0x19, //curve13 b
0x22, //curve13 a
0x19, //curve14 b
0x22, //curve14 a
0x19, //curve15 b
0x22, //curve15 a
0x19, //curve16 b
0x22, //curve16 a
0x19, //curve17 b
0x22, //curve17 a
0x19, //curve18 b
0x22, //curve18 a
0x23, //curve19 b
0x1e, //curve19 a
0x2e, //curve20 b
0x1b, //curve20 a
0x33, //curve21 b
0x1a, //curve21 a
0x40, //curve22 b
0x18, //curve22 a
0x48, //curve23 b
0x17, //curve23 a
0x00, //curve24 b
0xFF, //curve24 a
};

static char OUTDOOR_5[] ={
0xEC,
0x04, //cc r1 0.05
0x24,
0x1f, //cc r2
0xe2,
0x1f, //cc r3
0xfa,
0x1f, //cc g1
0xf1,
0x04, //cc g2
0x15,
0x1f, //cc g3
0xfa,
0x1f, //cc b1
0xf1,
0x1f, //cc b2
0xe2,
0x04, //cc b3
0x2d,
};

static char OUTDOOR_6[] ={
0xE7,
0x08, //roi_ctrl rgb_if_type mdnie_en mask 00 00 0 000
0x03, //scr_roi 1 scr algo_roi 1 algo 00 1 0 00 1 0
0x03, //HSIZE
0x00,
0x04, //VSIZE
0x00,
0x03, //sharpen cc gamma 00 0 0
//end
};

static unsigned char UI_1[] ={
//start
0xE8,
0x00, //roi0 x start
0x00,
0x00, //roi0 x end
0x00,
0x00, //roi0 y start
0x00,
0x00, //roi0 y end
0x00,
0x00, //roi1 x strat
0x00,
0x00, //roi1 x end
0x00,
0x00, //roi1 y start
0x00,
0x00, //roi1 y end
0x00,
};

static unsigned char UI_2[] ={
0xE9,
0x00, //scr Cr Yb
0xff, //scr Rr Bb
0xff, //scr Cg Yg
0x00, //scr Rg Bg
0xff, //scr Cb Yr
0x00, //scr Rb Br
0xff, //scr Mr Mb
0x00, //scr Gr Gb
0x00, //scr Mg Mg
0xff, //scr Gg Gg
0xff, //scr Mb Mr
0x00, //scr Gb Gr
0xff, //scr Yr Cb
0x00, //scr Br Rb
0xff, //scr Yg Cg
0x00, //scr Bg Rg
0x00, //scr Yb Cr
0xff, //scr Bb Rr
0xff, //scr Wr Wb
0x00, //scr Kr Kb
0xff, //scr Wg Wg
0x00, //scr Kg Kg
0xff, //scr Wb Wr
0x00, //scr Kb Kr
};

static unsigned char UI_3[] ={
0xEA,
0x00, //curve 1 b
0x20, //curve 1 a
0x00, //curve 2 b
0x20, //curve 2 a
0x00, //curve 3 b
0x20, //curve 3 a
0x00, //curve 4 b
0x20, //curve 4 a
0x00, //curve 5 b
0x20, //curve 5 a
0x00, //curve 6 b
0x20, //curve 6 a
0x00, //curve 7 b
0x20, //curve 7 a
0x00, //curve 8 b
0x20, //curve 8 a
0x00, //curve 9 b
0x20, //curve 9 a
0x00, //curve10 b
0x20, //curve10 a
0x00, //curve11 b
0x20, //curve11 a
0x00, //curve12 b
0x20, //curve12 a
};

static unsigned char UI_4[] = {
0xEB,
0x00, //curve13 b
0x20, //curve13 a
0x00, //curve14 b
0x20, //curve14 a
0x00, //curve15 b
0x20, //curve15 a
0x00, //curve16 b
0x20, //curve16 a
0x00, //curve17 b
0x20, //curve17 a
0x00, //curve18 b
0x20, //curve18 a
0x00, //curve19 b
0x20, //curve19 a
0x00, //curve20 b
0x20, //curve20 a
0x00, //curve21 b
0x20, //curve21 a
0x00, //curve22 b
0x20, //curve22 a
0x00, //curve23 b
0x20, //curve23 a
0x00, //curve24 b
0xFF, //curve24 a
};

static unsigned char UI_5[] ={
0xEC,
0x04, //cc r1 0.05
0x24,
0x1f, //cc r2
0xe2,
0x1f, //cc r3
0xfa,
0x1f, //cc g1
0xf1,
0x04, //cc g2
0x15,
0x1f, //cc g3
0xfa,
0x1f, //cc b1
0xf1,
0x1f, //cc b2
0xe2,
0x04, //cc b3
0x2d,
};

static unsigned char UI_6[] ={
0xE7,
0x08, //roi_ctrl rgb_if_type mdnie_en mask 00 00 0 000
0x03, //scr_roi 1 scr algo_roi 1 algo 00 1 0 00 1 0
0x03, //HSIZE
0x00,
0x04, //VSIZE
0x00,
0x02, //sharpen cc gamma 00 0 0
//end
};

static unsigned char VIDEO_1[] ={
//start
0xE8,
0x00, //roi0 x start
0x00,
0x00, //roi0 x end
0x00,
0x00, //roi0 y start
0x00,
0x00, //roi0 y end
0x00,
0x00, //roi1 x strat
0x00,
0x00, //roi1 x end
0x00,
0x00, //roi1 y start
0x00,
0x00, //roi1 y end
0x00,
};

static unsigned char VIDEO_2[] = {
0xE9,
0x00, //scr Cr Yb
0xff, //scr Rr Bb
0xff, //scr Cg Yg
0x00, //scr Rg Bg
0xff, //scr Cb Yr
0x00, //scr Rb Br
0xff, //scr Mr Mb
0x00, //scr Gr Gb
0x00, //scr Mg Mg
0xff, //scr Gg Gg
0xff, //scr Mb Mr
0x00, //scr Gb Gr
0xff, //scr Yr Cb
0x00, //scr Br Rb
0xff, //scr Yg Cg
0x00, //scr Bg Rg
0x00, //scr Yb Cr
0xff, //scr Bb Rr
0xff, //scr Wr Wb
0x00, //scr Kr Kb
0xff, //scr Wg Wg
0x00, //scr Kg Kg
0xff, //scr Wb Wr
0x00, //scr Kb Kr
};

static unsigned char VIDEO_3[] ={
0xEA,
0x00, //curve 1 b
0x1c, //curve 1 a
0x00, //curve 2 b
0x1c, //curve 2 a
0x00, //curve 3 b
0x1c, //curve 3 a
0x00, //curve 4 b
0x1c, //curve 4 a
0x00, //curve 5 b
0x1c, //curve 5 a
0x00, //curve 6 b
0x1c, //curve 6 a
0x00, //curve 7 b
0x1c, //curve 7 a
0x00, //curve 8 b
0x1c, //curve 8 a
0x00, //curve 9 b
0x1c, //curve 9 a
0x00, //curve10 b
0x1c, //curve10 a
0x00, //curve11 b
0x1c, //curve11 a
0x00, //curve12 b
0x1c, //curve12 a
};

static unsigned char VIDEO_4[] = {
0xEB,
0x00, //curve13 b
0x1c, //curve13 a
0x0d, //curve14 b
0xa4, //curve14 a
0x0d, //curve15 b
0xa4, //curve15 a
0x0d, //curve16 b
0xa4, //curve16 a
0x0d, //curve17 b
0xa4, //curve17 a
0x0d, //curve18 b
0xa4, //curve18 a
0x0d, //curve19 b
0xa4, //curve19 a
0x0d, //curve20 b
0xa4, //curve20 a
0x0d, //curve21 b
0xa4, //curve21 a
0x25, //curve22 b
0x1c, //curve22 a
0x4a, //curve23 b
0x17, //curve23 a
0x00, //curve24 b
0xFF, //curve24 a
};

static unsigned char VIDEO_5[] ={
0xEC,
0x04, //cc r1 0.05
0x24,
0x1f, //cc r2
0xe2,
0x1f, //cc r3
0xfa,
0x1f, //cc g1
0xf1,
0x04, //cc g2
0x15,
0x1f, //cc g3
0xfa,
0x1f, //cc b1
0xf1,
0x1f, //cc b2
0xe2,
0x04, //cc b3
0x2d,
};

static unsigned char VIDEO_6[] = {
0xE7,
0x08, //roi_ctrl rgb_if_type mdnie_en mask 00 00 0 000
0x03, //scr_roi 1 scr algo_roi 1 algo 00 1 0 00 1 0
0x03, //HSIZE
0x00,
0x04, //VSIZE
0x00,
0x03, //sharpen cc gamma 00 0 0
//end
};

static unsigned char CAMERA_1[] ={
//start
0xE8,
0x00, //roi0 x start
0x00,
0x00, //roi0 x end
0x00,
0x00, //roi0 y start
0x00,
0x00, //roi0 y end
0x00,
0x00, //roi1 x strat
0x00,
0x00, //roi1 x end
0x00,
0x00, //roi1 y start
0x00,
0x00, //roi1 y end
0x00,
};

static unsigned char CAMERA_2[] ={
0xE9,
0x00, //scr Cr Yb
0xff, //scr Rr Bb
0xff, //scr Cg Yg
0x00, //scr Rg Bg
0xff, //scr Cb Yr
0x00, //scr Rb Br
0xff, //scr Mr Mb
0x00, //scr Gr Gb
0x00, //scr Mg Mg
0xff, //scr Gg Gg
0xff, //scr Mb Mr
0x00, //scr Gb Gr
0xff, //scr Yr Cb
0x00, //scr Br Rb
0xff, //scr Yg Cg
0x00, //scr Bg Rg
0x00, //scr Yb Cr
0xff, //scr Bb Rr
0xff, //scr Wr Wb
0x00, //scr Kr Kb
0xff, //scr Wg Wg
0x00, //scr Kg Kg
0xff, //scr Wb Wr
0x00, //scr Kb Kr
};

static unsigned char CAMERA_3[] ={
0xEA,
0x00, //curve 1 b
0x20, //curve 1 a
0x00, //curve 2 b
0x20, //curve 2 a
0x00, //curve 3 b
0x20, //curve 3 a
0x00, //curve 4 b
0x20, //curve 4 a
0x00, //curve 5 b
0x20, //curve 5 a
0x00, //curve 6 b
0x20, //curve 6 a
0x00, //curve 7 b
0x20, //curve 7 a
0x00, //curve 8 b
0x20, //curve 8 a
0x00, //curve 9 b
0x20, //curve 9 a
0x00, //curve10 b
0x20, //curve10 a
0x00, //curve11 b
0x20, //curve11 a
0x00, //curve12 b
0x20, //curve12 a
};

static unsigned char CAMERA_4[] ={
0xEB,
0x00, //curve13 b
0x20, //curve13 a
0x00, //curve14 b
0x20, //curve14 a
0x00, //curve15 b
0x20, //curve15 a
0x00, //curve16 b
0x20, //curve16 a
0x00, //curve17 b
0x20, //curve17 a
0x00, //curve18 b
0x20, //curve18 a
0x00, //curve19 b
0x20, //curve19 a
0x00, //curve20 b
0x20, //curve20 a
0x00, //curve21 b
0x20, //curve21 a
0x00, //curve22 b
0x20, //curve22 a
0x00, //curve23 b
0x20, //curve23 a
0x00, //curve24 b
0xFF, //curve24 a
};

static unsigned char CAMERA_5[] ={
0xEC,
0x04, //cc r1 0.05
0x24,
0x1f, //cc r2
0xe2,
0x1f, //cc r3
0xfa,
0x1f, //cc g1
0xf1,
0x04, //cc g2
0x15,
0x1f, //cc g3
0xfa,
0x1f, //cc b1
0xf1,
0x1f, //cc b2
0xe2,
0x04, //cc b3
0x2d,
};

static unsigned char CAMERA_6[] ={
0xE7,
0x08, //roi_ctrl rgb_if_type mdnie_en mask 00 00 0 000
0x03, //scr_roi 1 scr algo_roi 1 algo 00 1 0 00 1 0
0x03, //HSIZE
0x00,
0x04, //VSIZE
0x00,
0x02, //sharpen cc gamma 00 0 0
//end
};

static unsigned char GALLERY_1[] ={
//start
0xE8,
0x00, //roi0 x start
0x00,
0x00, //roi0 x end
0x00,
0x00, //roi0 y start
0x00,
0x00, //roi0 y end
0x00,
0x00, //roi1 x strat
0x00,
0x00, //roi1 x end
0x00,
0x00, //roi1 y start
0x00,
0x00, //roi1 y end
0x00,
};

static unsigned char GALLERY_2[] = {
0xE9,
0x00, //scr Cr Yb
0xff, //scr Rr Bb
0xff, //scr Cg Yg
0x00, //scr Rg Bg
0xff, //scr Cb Yr
0x00, //scr Rb Br
0xff, //scr Mr Mb
0x00, //scr Gr Gb
0x00, //scr Mg Mg
0xff, //scr Gg Gg
0xff, //scr Mb Mr
0x00, //scr Gb Gr
0xff, //scr Yr Cb
0x00, //scr Br Rb
0xff, //scr Yg Cg
0x00, //scr Bg Rg
0x00, //scr Yb Cr
0xff, //scr Bb Rr
0xff, //scr Wr Wb
0x00, //scr Kr Kb
0xff, //scr Wg Wg
0x00, //scr Kg Kg
0xff, //scr Wb Wr
0x00, //scr Kb Kr
};

static unsigned char GALLERY_3[] = {
0xEA,
0x00, //curve 1 b
0x20, //curve 1 a
0x00, //curve 2 b
0x20, //curve 2 a
0x00, //curve 3 b
0x20, //curve 3 a
0x00, //curve 4 b
0x20, //curve 4 a
0x00, //curve 5 b
0x20, //curve 5 a
0x00, //curve 6 b
0x20, //curve 6 a
0x00, //curve 7 b
0x20, //curve 7 a
0x00, //curve 8 b
0x20, //curve 8 a
0x00, //curve 9 b
0x20, //curve 9 a
0x00, //curve10 b
0x20, //curve10 a
0x00, //curve11 b
0x20, //curve11 a
0x00, //curve12 b
0x20, //curve12 a
};

static unsigned char GALLERY_4[] ={
0xEB,
0x00, //curve13 b
0x20, //curve13 a
0x00, //curve14 b
0x20, //curve14 a
0x00, //curve15 b
0x20, //curve15 a
0x00, //curve16 b
0x20, //curve16 a
0x00, //curve17 b
0x20, //curve17 a
0x00, //curve18 b
0x20, //curve18 a
0x00, //curve19 b
0x20, //curve19 a
0x00, //curve20 b
0x20, //curve20 a
0x00, //curve21 b
0x20, //curve21 a
0x00, //curve22 b
0x20, //curve22 a
0x00, //curve23 b
0x20, //curve23 a
0x00, //curve24 b
0xFF, //curve24 a
};

static unsigned char GALLERY_5[] = {
0xEC,
0x04, //cc r1 0.05
0x24,
0x1f, //cc r2
0xe2,
0x1f, //cc r3
0xfa,
0x1f, //cc g1
0xf1,
0x04, //cc g2
0x15,
0x1f, //cc g3
0xfa,
0x1f, //cc b1
0xf1,
0x1f, //cc b2
0xe2,
0x04, //cc b3
0x2d,
};

static unsigned char GALLERY_6[] = {
0xE7,
0x08, //roi_ctrl rgb_if_type mdnie_en mask 00 00 0 000
0x03, //scr_roi 1 scr algo_roi 1 algo 00 1 0 00 1 0
0x03, //HSIZE
0x00,
0x04, //VSIZE
0x00,
0x02, //sharpen cc gamma 00 0 0
//end
};

static unsigned char BROWSER_1[] = {
//start
0xE8,
0x00, //roi0 x start
0x00,
0x00, //roi0 x end
0x00,
0x00, //roi0 y start
0x00,
0x00, //roi0 y end
0x00,
0x00, //roi1 x strat
0x00,
0x00, //roi1 x end
0x00,
0x00, //roi1 y start
0x00,
0x00, //roi1 y end
0x00,
};

static unsigned char BROWSER_2[] = {
0xE9,
0x00, //scr Cr Yb
0xff, //scr Rr Bb
0xff, //scr Cg Yg
0x00, //scr Rg Bg
0xff, //scr Cb Yr
0x00, //scr Rb Br
0xff, //scr Mr Mb
0x00, //scr Gr Gb
0x00, //scr Mg Mg
0xff, //scr Gg Gg
0xff, //scr Mb Mr
0x00, //scr Gb Gr
0xff, //scr Yr Cb
0x00, //scr Br Rb
0xff, //scr Yg Cg
0x00, //scr Bg Rg
0x00, //scr Yb Cr
0xff, //scr Bb Rr
0xff, //scr Wr Wb
0x00, //scr Kr Kb
0xff, //scr Wg Wg
0x00, //scr Kg Kg
0xff, //scr Wb Wr
0x00, //scr Kb Kr
};

static unsigned char BROWSER_3[] = {
0xEA,
0x00, //curve 1 b
0x20, //curve 1 a
0x00, //curve 2 b
0x20, //curve 2 a
0x00, //curve 3 b
0x20, //curve 3 a
0x00, //curve 4 b
0x20, //curve 4 a
0x00, //curve 5 b
0x20, //curve 5 a
0x00, //curve 6 b
0x20, //curve 6 a
0x00, //curve 7 b
0x20, //curve 7 a
0x00, //curve 8 b
0x20, //curve 8 a
0x00, //curve 9 b
0x20, //curve 9 a
0x00, //curve10 b
0x20, //curve10 a
0x00, //curve11 b
0x20, //curve11 a
0x00, //curve12 b
0x20, //curve12 a
};

static unsigned char BROWSER_4[] = {
0xEB,
0x00, //curve13 b
0x20, //curve13 a
0x00, //curve14 b
0x20, //curve14 a
0x00, //curve15 b
0x20, //curve15 a
0x00, //curve16 b
0x20, //curve16 a
0x00, //curve17 b
0x20, //curve17 a
0x00, //curve18 b
0x20, //curve18 a
0x00, //curve19 b
0x20, //curve19 a
0x00, //curve20 b
0x20, //curve20 a
0x00, //curve21 b
0x20, //curve21 a
0x00, //curve22 b
0x20, //curve22 a
0x00, //curve23 b
0x20, //curve23 a
0x00, //curve24 b
0xFF, //curve24 a
};

static unsigned char BROWSER_5[] = {
0xEC,
0x04, //cc r1 0.05
0x24,
0x1f, //cc r2
0xe2,
0x1f, //cc r3
0xfa,
0x1f, //cc g1
0xf1,
0x04, //cc g2
0x15,
0x1f, //cc g3
0xfa,
0x1f, //cc b1
0xf1,
0x1f, //cc b2
0xe2,
0x04, //cc b3
0x2d,
};

static unsigned char BROWSER_6[] = {
0xE7,
0x08, //roi_ctrl rgb_if_type mdnie_en mask 00 00 0 000
0x33, //scr_roi 1 scr algo_roi 1 algo 00 1 0 00 1 0
0x03, //HSIZE
0x00,
0x04, //VSIZE
0x00,
0x02, //sharpen cc gamma 00 0 0
//end
};

static unsigned char EBOOK_1[] = {
//start
0xE8,
0x00, //roi0 x start
0x00,
0x00, //roi0 x end
0x00,
0x00, //roi0 y start
0x00,
0x00, //roi0 y end
0x00,
0x00, //roi1 x strat
0x00,
0x00, //roi1 x end
0x00,
0x00, //roi1 y start
0x00,
0x00, //roi1 y end
0x00,
};

static unsigned char EBOOK_2[] = {
0xE9,
0x00, //scr Cr Yb
0xff, //scr Rr Bb
0xff, //scr Cg Yg
0x00, //scr Rg Bg
0xff, //scr Cb Yr
0x00, //scr Rb Br
0xff, //scr Mr Mb
0x00, //scr Gr Gb
0x00, //scr Mg Mg
0xff, //scr Gg Gg
0xff, //scr Mb Mr
0x00, //scr Gb Gr
0xff, //scr Yr Cb
0x00, //scr Br Rb
0xff, //scr Yg Cg
0x00, //scr Bg Rg
0x00, //scr Yb Cr
0xff, //scr Bb Rr
0xff, //scr Wr Wb
0x00, //scr Kr Kb
0xfe, //scr Wg Wg
0x00, //scr Kg Kg
0xee, //scr Wb Wr
0x00, //scr Kb Kr
};

static unsigned char EBOOK_3[] = {
0xEA,
0x00, //curve 1 b
0x20, //curve 1 a
0x00, //curve 2 b
0x20, //curve 2 a
0x00, //curve 3 b
0x20, //curve 3 a
0x00, //curve 4 b
0x20, //curve 4 a
0x00, //curve 5 b
0x20, //curve 5 a
0x00, //curve 6 b
0x20, //curve 6 a
0x00, //curve 7 b
0x20, //curve 7 a
0x00, //curve 8 b
0x20, //curve 8 a
0x00, //curve 9 b
0x20, //curve 9 a
0x00, //curve10 b
0x20, //curve10 a
0x00, //curve11 b
0x20, //curve11 a
0x00, //curve12 b
0x20, //curve12 a
};

static unsigned char EBOOK_4[] = {
0xEB,
0x00, //curve13 b
0x20, //curve13 a
0x00, //curve14 b
0x20, //curve14 a
0x00, //curve15 b
0x20, //curve15 a
0x00, //curve16 b
0x20, //curve16 a
0x00, //curve17 b
0x20, //curve17 a
0x00, //curve18 b
0x20, //curve18 a
0x00, //curve19 b
0x20, //curve19 a
0x00, //curve20 b
0x20, //curve20 a
0x00, //curve21 b
0x20, //curve21 a
0x00, //curve22 b
0x20, //curve22 a
0x00, //curve23 b
0x20, //curve23 a
0x00, //curve24 b
0xFF, //curve24 a
};

static unsigned char EBOOK_5[] = {
0xEC,
0x04, //cc r1
0x00,
0x00, //cc r2
0x00,
0x00, //cc r3
0x00,
0x00, //cc g1
0x00,
0x04, //cc g2
0x00,
0x00, //cc g3
0x00,
0x00, //cc b1
0x00,
0x00, //cc b2
0x00,
0x04, //cc b3
0x00,
};

static unsigned char EBOOK_6[] = {
0xE7,
0x08, //roi_ctrl rgb_if_type mdnie_en mask 00 00 0 000
0x30, //scr_roi 1 scr algo_roi 1 algo 00 1 0 00 1 0
0x03, //HSIZE
0x00,
0x04, //VSIZE
0x00,
0x00, //sharpen cc gamma 00 0 0
//end
};

static struct dsi_cmd_desc DSI0_BYPASS_MDNIE[] = {
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(LEVEL_UNLOCK)}, LEVEL_UNLOCK},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(BYPASS_1)}, BYPASS_1},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(BYPASS_2)}, BYPASS_2},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(BYPASS_3)}, BYPASS_3},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(BYPASS_4)}, BYPASS_4},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(BYPASS_5)}, BYPASS_5},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(BYPASS_6)}, BYPASS_6},
	{{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(LEVEL_LOCK)}, LEVEL_LOCK},
};
static struct dsi_cmd_desc DSI0_NEGATIVE_MDNIE[] = {
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(LEVEL_UNLOCK)}, LEVEL_UNLOCK},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(NEGATIVE_1)}, NEGATIVE_1},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(NEGATIVE_2)}, NEGATIVE_2},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(NEGATIVE_3)}, NEGATIVE_3},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(NEGATIVE_4)}, NEGATIVE_4},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(NEGATIVE_5)}, NEGATIVE_5},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(NEGATIVE_6)}, NEGATIVE_6},
	{{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(LEVEL_LOCK)}, LEVEL_LOCK},
};
static struct dsi_cmd_desc DSI0_COLOR_BLIND_MDNIE[] = {
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(LEVEL_UNLOCK)}, LEVEL_UNLOCK},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(COLOR_BLIND_1)}, COLOR_BLIND_1},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(COLOR_BLIND_2)}, COLOR_BLIND_2},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(COLOR_BLIND_3)}, COLOR_BLIND_3},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(COLOR_BLIND_4)}, COLOR_BLIND_4},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(COLOR_BLIND_5)}, COLOR_BLIND_5},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(COLOR_BLIND_6)}, COLOR_BLIND_6},
	{{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(LEVEL_LOCK)}, LEVEL_LOCK},
};
static struct dsi_cmd_desc DSI0_GRAYSCALE_MDNIE[] = {
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(LEVEL_UNLOCK)}, LEVEL_UNLOCK},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(GRAYSCALE_1)}, GRAYSCALE_1},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(GRAYSCALE_2)}, GRAYSCALE_2},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(GRAYSCALE_3)}, GRAYSCALE_3},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(GRAYSCALE_4)}, GRAYSCALE_4},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(GRAYSCALE_5)}, GRAYSCALE_5},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(GRAYSCALE_6)}, GRAYSCALE_6},
	{{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(LEVEL_LOCK)}, LEVEL_LOCK},
};
static struct dsi_cmd_desc DSI0_GRAYSCALE_NEGATIVE_MDNIE[] = {
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(LEVEL_UNLOCK)}, LEVEL_UNLOCK},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(GRAYSCALE_NEGATIVE_1)}, GRAYSCALE_NEGATIVE_1},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(GRAYSCALE_NEGATIVE_2)}, GRAYSCALE_NEGATIVE_2},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(GRAYSCALE_NEGATIVE_3)}, GRAYSCALE_NEGATIVE_3},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(GRAYSCALE_NEGATIVE_4)}, GRAYSCALE_NEGATIVE_4},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(GRAYSCALE_NEGATIVE_5)}, GRAYSCALE_NEGATIVE_5},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(GRAYSCALE_NEGATIVE_6)}, GRAYSCALE_NEGATIVE_6},
	{{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(LEVEL_LOCK)}, LEVEL_LOCK},
};
static struct dsi_cmd_desc DSI0_NIGHT_MODE_MDNIE[] = {
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(LEVEL_UNLOCK)}, LEVEL_UNLOCK},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(NIGHT_MODE_1)}, NIGHT_MODE_1},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(NIGHT_MODE_2)}, NIGHT_MODE_2},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(NIGHT_MODE_3)}, NIGHT_MODE_3},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(NIGHT_MODE_4)}, NIGHT_MODE_4},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(NIGHT_MODE_5)}, NIGHT_MODE_5},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(NIGHT_MODE_6)}, NIGHT_MODE_6},
	{{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(LEVEL_LOCK)}, LEVEL_LOCK},
};
static struct dsi_cmd_desc DSI0_LIGHT_NOTIFICATION_MDNIE[] = {
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(LEVEL_UNLOCK)}, LEVEL_UNLOCK},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(LIGHT_NOTIFICATION_1)}, LIGHT_NOTIFICATION_1},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(LIGHT_NOTIFICATION_2)}, LIGHT_NOTIFICATION_2},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(LIGHT_NOTIFICATION_3)}, LIGHT_NOTIFICATION_3},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(LIGHT_NOTIFICATION_4)}, LIGHT_NOTIFICATION_4},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(LIGHT_NOTIFICATION_5)}, LIGHT_NOTIFICATION_5},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(LIGHT_NOTIFICATION_6)}, LIGHT_NOTIFICATION_6},
	{{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(LEVEL_LOCK)}, LEVEL_LOCK},
};
static struct dsi_cmd_desc DSI0_HBM_CE_MDNIE[] = {
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(LEVEL_UNLOCK)}, LEVEL_UNLOCK},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(OUTDOOR_1)}, OUTDOOR_1},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(OUTDOOR_2)}, OUTDOOR_2},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(OUTDOOR_3)}, OUTDOOR_3},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(OUTDOOR_4)}, OUTDOOR_4},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(OUTDOOR_5)}, OUTDOOR_5},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(OUTDOOR_6)}, OUTDOOR_6},
	{{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(LEVEL_LOCK)}, LEVEL_LOCK},
};
static struct dsi_cmd_desc DSI0_CAMERA_MDNIE[] = {
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(LEVEL_UNLOCK)}, LEVEL_UNLOCK},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(CAMERA_1)}, CAMERA_1},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(CAMERA_2)}, CAMERA_2},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(CAMERA_3)}, CAMERA_3},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(CAMERA_4)}, CAMERA_4},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(CAMERA_5)}, CAMERA_5},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(CAMERA_6)}, CAMERA_6},
	{{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(LEVEL_LOCK)}, LEVEL_LOCK},
};
static struct dsi_cmd_desc DSI0_EBOOK_MDNIE[] = {
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(LEVEL_UNLOCK)}, LEVEL_UNLOCK},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(EBOOK_1)}, EBOOK_1},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(EBOOK_2)}, EBOOK_2},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(EBOOK_3)}, EBOOK_3},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(EBOOK_4)}, EBOOK_4},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(EBOOK_5)}, EBOOK_5},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(EBOOK_6)}, EBOOK_6},
	{{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(LEVEL_LOCK)}, LEVEL_LOCK},
};
static struct dsi_cmd_desc DSI0_UI_MDNIE[] = {
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(LEVEL_UNLOCK)}, LEVEL_UNLOCK},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(UI_1)}, UI_1},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(UI_2)}, UI_2},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(UI_3)}, UI_3},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(UI_4)}, UI_4},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(UI_5)}, UI_5},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(UI_6)}, UI_6},
	{{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(LEVEL_LOCK)}, LEVEL_LOCK},
};
static struct dsi_cmd_desc DSI0_GALLERY_MDNIE[] = {
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(LEVEL_UNLOCK)}, LEVEL_UNLOCK},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(GALLERY_1)}, GALLERY_1},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(GALLERY_2)}, GALLERY_2},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(GALLERY_3)}, GALLERY_3},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(GALLERY_4)}, GALLERY_4},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(GALLERY_5)}, GALLERY_5},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(GALLERY_6)}, GALLERY_6},
	{{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(LEVEL_LOCK)}, LEVEL_LOCK},
};
static struct dsi_cmd_desc DSI0_VIDEO_MDNIE[] = {
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(LEVEL_UNLOCK)}, LEVEL_UNLOCK},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(VIDEO_1)}, VIDEO_1},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(VIDEO_2)}, VIDEO_2},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(VIDEO_3)}, VIDEO_3},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(VIDEO_4)}, VIDEO_4},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(VIDEO_5)}, VIDEO_5},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(VIDEO_6)}, VIDEO_6},
	{{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(LEVEL_LOCK)}, LEVEL_LOCK},
};
static struct dsi_cmd_desc DSI0_BROWSER_MDNIE[] = {
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(LEVEL_UNLOCK)}, LEVEL_UNLOCK},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(BROWSER_1)}, BROWSER_1},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(BROWSER_2)}, BROWSER_2},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(BROWSER_3)}, BROWSER_3},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(BROWSER_4)}, BROWSER_4},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(BROWSER_5)}, BROWSER_5},
	{{DTYPE_GEN_LWRITE, 0, 0, 0, 0, sizeof(BROWSER_6)}, BROWSER_6},
	{{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(LEVEL_LOCK)}, LEVEL_LOCK},
};

static struct dsi_cmd_desc *mdnie_tune_value_dsi0[MAX_APP_MODE][MAX_MODE][MAX_OUTDOOR_MODE] = {
		/*
			DYNAMIC_MODE
			STANDARD_MODE
			NATURAL_MODE
			MOVIE_MODE
			AUTO_MODE
			READING_MODE
		*/
		// UI_APP
		{
			{DSI0_UI_MDNIE,	NULL},
			{DSI0_UI_MDNIE,	NULL},
			{DSI0_UI_MDNIE,	NULL},
			{DSI0_UI_MDNIE,	NULL},
			{DSI0_UI_MDNIE,	NULL},
			{DSI0_EBOOK_MDNIE,	NULL},
		},
		// VIDEO_APP
		{
			{DSI0_VIDEO_MDNIE,	NULL},
			{DSI0_VIDEO_MDNIE,	NULL},
			{DSI0_VIDEO_MDNIE,	NULL},
			{DSI0_VIDEO_MDNIE,	NULL},
			{DSI0_VIDEO_MDNIE,	NULL},
			{DSI0_EBOOK_MDNIE,	NULL},
		},
		// VIDEO_WARM_APP
		{
			{NULL,	NULL},
			{NULL,	NULL},
			{NULL,	NULL},
			{NULL,	NULL},
			{NULL,	NULL},
			{NULL,	NULL},
		},
		// VIDEO_COLD_APP
		{
			{NULL,	NULL},
			{NULL,	NULL},
			{NULL,	NULL},
			{NULL,	NULL},
			{NULL,	NULL},
			{NULL,	NULL},
		},
		// CAMERA_APP
		{
			{DSI0_CAMERA_MDNIE,	NULL},
			{DSI0_CAMERA_MDNIE,	NULL},
			{DSI0_CAMERA_MDNIE,	NULL},
			{DSI0_CAMERA_MDNIE,	NULL},
			{DSI0_CAMERA_MDNIE,	NULL},
			{DSI0_EBOOK_MDNIE,	NULL},
		},
		// NAVI_APP
		{
			{NULL,	NULL},
			{NULL,	NULL},
			{NULL,	NULL},
			{NULL,	NULL},
			{NULL,	NULL},
			{NULL,	NULL},
		},
		// GALLERY_APP
		{
			{DSI0_GALLERY_MDNIE,	NULL},
			{DSI0_GALLERY_MDNIE,	NULL},
			{DSI0_GALLERY_MDNIE,	NULL},
			{DSI0_GALLERY_MDNIE,	NULL},
			{DSI0_GALLERY_MDNIE,	NULL},
			{DSI0_EBOOK_MDNIE,	NULL},
		},
		// VT_APP
		{
			{NULL,	NULL},
			{NULL,	NULL},
			{NULL,	NULL},
			{NULL,	NULL},
			{NULL,	NULL},
			{NULL,	NULL},
		},
		// BROWSER_APP
		{
			{DSI0_BROWSER_MDNIE,	NULL},
			{DSI0_BROWSER_MDNIE,	NULL},
			{DSI0_BROWSER_MDNIE,	NULL},
			{DSI0_BROWSER_MDNIE,	NULL},
			{DSI0_BROWSER_MDNIE,	NULL},
			{DSI0_EBOOK_MDNIE,	NULL},
		},
		// eBOOK_APP
		{
			{DSI0_EBOOK_MDNIE,	NULL},
			{DSI0_EBOOK_MDNIE,	NULL},
			{DSI0_EBOOK_MDNIE,	NULL},
			{DSI0_EBOOK_MDNIE,	NULL},
			{DSI0_EBOOK_MDNIE,	NULL},
			{DSI0_EBOOK_MDNIE,	NULL},
		},
		// EMAIL_APP
		{
			{DSI0_UI_MDNIE,	NULL},
			{DSI0_UI_MDNIE,	NULL},
			{DSI0_UI_MDNIE,	NULL},
			{DSI0_UI_MDNIE,	NULL},
			{DSI0_UI_MDNIE,	NULL},
			{DSI0_EBOOK_MDNIE,	NULL},
		},
		// TDMB_APP
		{
			{DSI0_UI_MDNIE,	NULL},
			{DSI0_UI_MDNIE,	NULL},
			{DSI0_UI_MDNIE,	NULL},
			{DSI0_UI_MDNIE,	NULL},
			{DSI0_UI_MDNIE,	NULL},
			{DSI0_EBOOK_MDNIE,	NULL},
		},
};

static struct dsi_cmd_desc *light_notification_tune_value[LIGHT_NOTIFICATION_MAX] = {
	NULL,
	DSI0_LIGHT_NOTIFICATION_MDNIE,
};

#define DSI0_RGB_SENSOR_MDNIE_1_SIZE ARRAY_SIZE(UI_1)
#define DSI0_RGB_SENSOR_MDNIE_2_SIZE ARRAY_SIZE(UI_2)
#define DSI0_RGB_SENSOR_MDNIE_3_SIZE ARRAY_SIZE(UI_3)
#endif /*_DSI_TCON_MDNIE_LITE_DATA_FHD_S6D7AA0X11_TV080WXM_H_*/
