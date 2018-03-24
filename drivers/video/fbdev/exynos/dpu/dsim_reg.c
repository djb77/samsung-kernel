/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * SFR access functions for Samsung EXYNOS SoC MIPI-DSI Master driver.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include "dsim.h"

/* These definitions are need to guide from AP team */
#define DSIM_STOP_STATE_CNT		0xA
#define DSIM_BTA_TIMEOUT		0xff
#define DSIM_LP_RX_TIMEOUT		0xffff
#define DSIM_MULTI_PACKET_CNT		0xffff
#define DSIM_PLL_STABLE_TIME		0x13880
#define DSIM_FIFOCTRL_THRESHOLD		0x20 /* 1 ~ 32 */

/* If below values depend on panel. These values wil be move to panel file.
 * And these values are valid in case of video mode only. */
#define DSIM_CMD_ALLOW_VALUE		4
#define DSIM_STABLE_VFP_VALUE		2
#define TE_PROTECT_ON_TIME		158 /* 15.8ms*/
#define TE_TIMEOUT_TIME			180 /* 18ms */


const u32 DSIM_PHY_CHARIC_VAL[][9] = {
#if defined (CONFIG_SOC_EMULATOR8895)
	/* MPLL_CTRL1, MPLL_CTRL2, B_DPHY_CTRL2, B_DPHY_CTRL3, B_DPHY_CTRL_4,
	   M_DPHY_CTRL1, M_DPHY_CTRL2, M_DPHY_CTRL3, M_DPHY_CTRL4*/
	{0x0, 0x7800000, 0x800, 0x0, 0x40, 0x0, 0x0, 0x0, 0x0},
#else
	/* MPLL_CTRL1, MPLL_CTRL2, B_DPHY_CTRL2, B_DPHY_CTRL3, B_DPHY_CTRL_4,
	   M_DPHY_CTRL1, M_DPHY_CTRL2, M_DPHY_CTRL3, M_DPHY_CTRL4*/
	//{0x0, 0x03100000, 0x0, 0x10000000, 0x00400002, 0x0, 0x0, 0x0, 0x0, 0x00020000},
	{0x0, 0x3800000, 0x800, 0x10001249, 0x500000, 0x1980, 0x0, 0x0, 0x0},
	//{0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
	{0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
	{0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
#endif
};

/* DPHY timing table */
/*below table have to be changed to meet KC DPHY spec*/
const u32 dphy_timing[][10] = {
	/* bps, clk_prepare, clk_zero, clk_post, clk_trail,
	hs_prepare, hs_zero, hs_trail, lpx, hs_exit */
	{2500, 25, 43, 14, 20, 24, 19, 24, 18, 16},
	{2490, 25, 43, 14, 20, 24, 19, 24, 18, 16},
	{2480, 24, 43, 14, 20, 24, 19, 24, 18, 16},
	{2470, 24, 43, 13, 20, 24, 18, 24, 18, 15},
	{2460, 24, 42, 13, 20, 24, 18, 24, 18, 15},
	{2450, 24, 42, 13, 20, 24, 18, 24, 18, 15},
	{2440, 24, 42, 13, 20, 23, 18, 24, 18, 15},
	{2430, 24, 42, 13, 20, 23, 18, 24, 18, 15},
	{2420, 24, 41, 13, 20, 23, 18, 24, 18, 15},
	{2410, 24, 41, 13, 20, 23, 18, 24, 18, 15},
	{2400, 24, 41, 13, 20, 23, 18, 24, 17, 15},
	{2390, 23, 41, 13, 19, 23, 18, 23, 17, 15},
	{2380, 23, 41, 13, 19, 23, 18, 23, 17, 15},
	{2370, 23, 41, 13, 19, 23, 18, 23, 17, 15},
	{2360, 23, 41, 13, 19, 23, 18, 23, 17, 15},
	{2350, 23, 40, 13, 19, 23, 18, 23, 17, 15},
	{2340, 23, 40, 13, 19, 22, 18, 23, 17, 15},
	{2330, 23, 40, 13, 19, 22, 17, 23, 17, 15},
	{2320, 23, 40, 13, 19, 22, 17, 23, 17, 14},
	{2310, 23, 39, 13, 19, 22, 17, 23, 17, 14},
	{2300, 23, 39, 13, 19, 22, 17, 23, 17, 14},
	{2290, 22, 39, 13, 19, 22, 17, 23, 17, 14},
	{2280, 22, 39, 13, 18, 22, 17, 22, 17, 14},
	{2270, 22, 39, 13, 18, 22, 17, 22, 17, 14},
	{2260, 22, 39, 13, 18, 22, 17, 22, 16, 14},
	{2250, 22, 39, 13, 18, 22, 17, 22, 16, 14},
	{2240, 22, 38, 12, 18, 21, 17, 22, 16, 14},
	{2230, 22, 38, 12, 18, 21, 17, 22, 16, 14},
	{2220, 22, 38, 12, 18, 21, 16, 22, 16, 14},
	{2210, 22, 38, 12, 18, 21, 16, 22, 16, 14},
	{2200, 22, 38, 12, 18, 21, 16, 22, 16, 14},
	{2190, 21, 38, 12, 18, 21, 16, 22, 16, 14},
	{2180, 21, 37, 12, 18, 21, 16, 22, 16, 13},
	{2170, 21, 37, 12, 18, 21, 16, 22, 16, 13},
	{2160, 21, 37, 12, 17, 21, 16, 21, 16, 13},
	{2150, 21, 37, 12, 17, 21, 16, 21, 16, 13},
	{2140, 21, 36, 12, 17, 20, 16, 21, 16, 13},
	{2130, 21, 36, 12, 17, 20, 16, 21, 15, 13},
	{2120, 21, 36, 12, 17, 20, 16, 21, 15, 13},
	{2110, 21, 36, 12, 17, 20, 15, 21, 15, 13},
	{2100, 21, 36, 12, 17, 20, 15, 21, 15, 13},
	{2090, 20, 36, 12, 17, 20, 15, 21, 15, 13},
	{2080, 20, 36, 12, 17, 20, 15, 21, 15, 13},
	{2070, 20, 35, 12, 17, 20, 15, 21, 15, 13},
	{2060, 20, 35, 12, 17, 20, 15, 21, 15, 13},
	{2050, 20, 35, 12, 16, 20, 15, 20, 15, 13},
	{2040, 20, 35, 12, 16, 20, 15, 20, 15, 13},
	{2030, 20, 34, 12, 16, 19, 15, 20, 15, 12},
	{2020, 20, 34, 12, 16, 19, 15, 20, 15, 12},
	{2010, 20, 34, 11, 16, 19, 15, 20, 15, 12},
	{2000, 20, 34, 11, 16, 19, 14, 20, 14, 12},
	{1990, 19, 34, 11, 16, 19, 14, 20, 14, 12},
	{1980, 19, 34, 11, 16, 19, 14, 20, 14, 12},
	{1970, 19, 33, 11, 16, 19, 14, 20, 14, 12},
	{1960, 19, 33, 11, 16, 19, 14, 20, 14, 12},
	{1950, 19, 33, 11, 16, 19, 14, 20, 14, 12},
	{1940, 19, 33, 11, 15, 19, 14, 19, 14, 12},
	{1930, 19, 33, 11, 15, 18, 14, 19, 14, 12},
	{1920, 19, 32, 11, 15, 18, 14, 19, 14, 12},
	{1910, 19, 32, 11, 15, 18, 14, 19, 14, 12},
	{1900, 18, 32, 11, 15, 18, 14, 19, 14, 12},
	{1890, 18, 32, 11, 15, 18, 14, 19, 14, 11},
	{1880, 18, 32, 11, 15, 18, 13, 19, 14, 11},
	{1870, 18, 32, 11, 15, 18, 13, 19, 14, 11},
	{1860, 18, 31, 11, 15, 18, 13, 19, 13, 11},
	{1850, 18, 31, 11, 15, 18, 13, 19, 13, 11},
	{1840, 18, 31, 11, 15, 18, 13, 19, 13, 11},
	{1830, 18, 31, 11, 14, 17, 13, 18, 13, 11},
	{1820, 18, 30, 11, 14, 17, 13, 18, 13, 11},
	{1810, 18, 30, 11, 14, 17, 13, 18, 13, 11},
	{1800, 17, 30, 11, 14, 17, 13, 18, 13, 11},
	{1790, 17, 30, 11, 14, 17, 13, 18, 13, 11},
	{1780, 17, 30, 10, 14, 17, 13, 18, 13, 11},
	{1770, 17, 30, 10, 14, 17, 12, 18, 13, 11},
	{1760, 17, 30, 10, 14, 17, 12, 18, 13, 11},
	{1750, 17, 29, 10, 14, 17, 12, 18, 13, 11},
	{1740, 17, 29, 10, 14, 17, 12, 18, 13, 10},
	{1730, 17, 29, 10, 14, 16, 12, 18, 12, 10},
	{1720, 17, 29, 10, 14, 16, 12, 18, 12, 10},
	{1710, 17, 29, 10, 13, 16, 12, 17, 12, 10},
	{1700, 16, 29, 10, 13, 16, 12, 17, 12, 10},
	{1690, 16, 28, 10, 13, 16, 12, 17, 12, 10},
	{1680, 16, 28, 10, 13, 16, 12, 17, 12, 10},
	{1670, 16, 28, 10, 13, 16, 12, 17, 12, 10},
	{1660, 16, 28, 10, 13, 16, 11, 17, 12, 10},
	{1650, 16, 28, 10, 13, 16, 11, 17, 12, 10},
	{1640, 16, 27, 10, 13, 16, 11, 17, 12, 10},
	{1630, 16, 27, 10, 13, 16, 11, 17, 12, 10},
	{1620, 16, 27, 10, 13, 15, 11, 17, 12, 10},
	{1610, 16, 27, 10, 13, 15, 11, 17, 12, 10},
	{1600, 15, 27, 10, 12, 15, 11, 16, 11, 9},
	{1590, 15, 27, 10, 12, 15, 11, 16, 11, 9},
	{1580, 15, 26, 10, 12, 15, 11, 16, 11, 9},
	{1570, 15, 26, 10, 12, 15, 11, 16, 11, 9},
	{1560, 15, 26, 10, 12, 15, 11, 16, 11, 9},
	{1550, 15, 26, 9, 12, 15, 10, 16, 11, 9},
	{1540, 15, 25, 9, 12, 15, 10, 16, 11, 9},
	{1530, 15, 25, 9, 12, 15, 10, 16, 11, 9},
	{1520, 15, 25, 9, 12, 14, 10, 16, 11, 9},
	{1510, 14, 25, 9, 12, 14, 10, 16, 11, 9},
	{1500, 14, 25, 9, 12, 14, 10, 16, 11, 9},
	{1490, 14, 25, 9, 11, 14, 10, 15, 11, 9},
	{1480, 14, 25, 9, 11, 14, 10, 15, 11, 9},
	{1470, 14, 24, 9, 11, 14, 10, 15, 11, 9},
	{1460, 14, 24, 9, 11, 14, 10, 15, 10, 9},
	{1450, 14, 24, 9, 11, 14, 10, 15, 10, 8},
	{1440, 14, 24, 9, 11, 14, 10, 15, 10, 8},
	{1430, 14, 23, 9, 11, 14, 10, 15, 10, 8},
	{1420, 14, 23, 9, 11, 13, 10, 15, 10, 8},
	{1410, 13, 23, 9, 11, 13, 9, 15, 10, 8},
	{1400, 13, 23, 9, 11, 13, 9, 15, 10, 8},
	{1390, 13, 23, 9, 11, 13, 9, 15, 10, 8},
	{1380, 13, 23, 9, 10, 13, 9, 14, 10, 8},
	{1370, 13, 23, 9, 10, 13, 9, 14, 10, 8},
	{1360, 13, 22, 9, 10, 13, 9, 14, 10, 8},
	{1350, 13, 22, 9, 10, 13, 9, 14, 10, 8},
	{1340, 13, 22, 9, 10, 13, 9, 14, 10, 8},
	{1330, 13, 22, 9, 10, 13, 9, 14, 9, 8},
	{1320, 13, 22, 8, 10, 12, 9, 14, 9, 8},
	{1310, 12, 22, 8, 10, 12, 9, 14, 9, 8},
	{1300, 12, 21, 8, 10, 12, 8, 14, 9, 7},
	{1290, 12, 21, 8, 10, 12, 8, 14, 9, 7},
	{1280, 12, 21, 8, 10, 12, 8, 14, 9, 7},
	{1270, 12, 21, 8, 10, 12, 8, 14, 9, 7},
	{1260, 12, 20, 8, 9, 12, 8, 13, 9, 7},
	{1250, 12, 20, 8, 9, 12, 8, 13, 9, 7},
	{1240, 12, 20, 8, 9, 12, 8, 13, 9, 7},
	{1230, 12, 20, 8, 9, 12, 8, 13, 9, 7},
	{1220, 12, 20, 8, 9, 12, 8, 13, 9, 7},
	{1210, 11, 20, 8, 9, 11, 8, 13, 9, 7},
	{1200, 11, 20, 8, 9, 11, 8, 13, 8, 7},
	{1190, 11, 19, 8, 9, 11, 7, 13, 8, 7},
	{1180, 11, 19, 8, 9, 11, 7, 13, 8, 7},
	{1170, 11, 19, 8, 9, 11, 7, 13, 8, 7},
	{1160, 11, 19, 8, 9, 11, 7, 13, 8, 6},
	{1150, 11, 18, 8, 8, 11, 7, 12, 8, 6},
	{1140, 11, 18, 8, 8, 11, 7, 12, 8, 6},
	{1130, 11, 18, 8, 8, 11, 7, 12, 8, 6},
	{1120, 10, 18, 8, 8, 11, 7, 12, 8, 6},
	{1110, 10, 18, 8, 8, 10, 7, 12, 8, 6},
	{1100, 10, 18, 8, 8, 10, 7, 12, 8, 6},
	{1090, 10, 17, 7, 8, 10, 7, 12, 8, 6},
	{1080, 10, 17, 7, 8, 10, 6, 12, 8, 6},
	{1070, 10, 17, 7, 8, 10, 6, 12, 8, 6},
	{1060, 10, 17, 7, 8, 10, 6, 12, 7, 6},
	{1050, 10, 17, 7, 8, 10, 6, 12, 7, 6},
	{1040, 10, 16, 7, 7, 10, 6, 11, 7, 6},
	{1030, 10, 16, 7, 7, 10, 6, 11, 7, 6},
	{1020, 9, 16, 7, 7, 10, 6, 11, 7, 6},
	{1010, 9, 16, 7, 7, 9, 6, 11, 7, 5},
	{1000, 9, 16, 7, 7, 9, 6, 11, 7, 5},
	{990, 9, 16, 7, 7, 9, 6, 11, 7, 5},
	{980, 9, 15, 7, 7, 9, 6, 11, 7, 5},
	{970, 9, 15, 7, 7, 9, 5, 11, 7, 5},
	{960, 9, 15, 7, 7, 9, 5, 11, 7, 5},
	{950, 9, 15, 7, 7, 9, 5, 11, 7, 5},
	{940, 9, 14, 7, 7, 9, 5, 11, 7, 5},
	{930, 9, 14, 7, 7, 9, 5, 11, 6, 5},
	{920, 8, 14, 7, 6, 9, 5, 10, 6, 5},
	{910, 8, 14, 7, 6, 8, 5, 10, 6, 5},
	{900, 8, 14, 7, 6, 8, 5, 10, 6, 5},
	{890, 8, 14, 7, 6, 8, 5, 10, 6, 5},
	{880, 8, 14, 7, 6, 8, 5, 10, 6, 5},
	{870, 8, 13, 6, 6, 8, 5, 10, 6, 4},
	{860, 8, 13, 6, 6, 8, 4, 10, 6, 4},
	{850, 8, 13, 6, 6, 8, 4, 10, 6, 4},
	{840, 8, 13, 6, 6, 8, 4, 10, 6, 4},
	{830, 8, 13, 6, 6, 8, 4, 10, 6, 4},
	{820, 7, 13, 6, 6, 8, 4, 10, 6, 4},
	{810, 7, 12, 6, 5, 8, 4, 9, 6, 4},
	{800, 7, 12, 6, 5, 7, 4, 9, 5, 4},
	{790, 7, 12, 6, 5, 7, 4, 9, 5, 4},
	{780, 7, 12, 6, 5, 7, 4, 9, 5, 4},
	{770, 7, 12, 6, 5, 7, 4, 9, 5, 4},
	{760, 7, 11, 6, 5, 7, 4, 9, 5, 4},
	{750, 7, 11, 6, 5, 7, 3, 9, 5, 4},
	{740, 7, 11, 6, 5, 7, 3, 9, 5, 4},
	{730, 6, 11, 6, 5, 7, 3, 9, 5, 4},
	{720, 6, 11, 6, 5, 7, 3, 9, 5, 3},
	{710, 6, 11, 6, 5, 7, 3, 9, 5, 3},
	{700, 6, 10, 6, 4, 6, 3, 8, 5, 3},
	{690, 6, 10, 6, 4, 6, 3, 8, 5, 3},
	{680, 6, 10, 6, 4, 6, 3, 8, 5, 3},
	{670, 6, 10, 6, 4, 6, 3, 8, 5, 3},
	{660, 6, 9, 6, 4, 6, 3, 8, 4, 3},
	{650, 6, 9, 6, 4, 6, 3, 8, 4, 3},
	{640, 6, 9, 5, 4, 6, 2, 8, 4, 3},
	{630, 5, 9, 5, 4, 6, 2, 8, 4, 3},
	{620, 5, 9, 5, 4, 6, 2, 8, 4, 3},
	{610, 5, 9, 5, 4, 6, 2, 8, 4, 3},
	{600, 5, 9, 5, 4, 5, 2, 8, 4, 3},
	{590, 5, 8, 5, 3, 5, 2, 7, 4, 3},
	{580, 5, 8, 5, 3, 5, 2, 7, 4, 2},
	{570, 5, 8, 5, 3, 5, 2, 7, 4, 2},
	{560, 5, 8, 5, 3, 5, 2, 7, 4, 2},
	{550, 5, 7, 5, 3, 5, 2, 7, 4, 2},
	{540, 5, 7, 5, 3, 5, 2, 7, 4, 2},
	{530, 4, 7, 5, 3, 5, 1, 7, 3, 2},
	{520, 4, 7, 5, 3, 5, 1, 7, 3, 2},
	{510, 4, 7, 5, 3, 5, 1, 7, 3, 2},
	{500, 4, 7, 5, 3, 5, 1, 7, 3, 2},
	{490, 4, 6, 5, 3, 4, 1, 7, 3, 2},
	{480, 4, 6, 5, 3, 4, 1, 7, 3, 2},
};

const u32 b_dphyctl[14] = {
	0x0af, 0x0c8, 0x0e1, 0x0fa,		/* esc 7 ~ 10 */
	0x113, 0x12c, 0x145, 0x15e, 0x177,	/* esc 11 ~ 15 */
	0x190, 0x1a9, 0x1c2, 0x1db, 0x1f4
};	/* esc 16 ~ 20 */


/******************* CAL raw functions implementation *************************/

void dsim_reg_sw_reset(u32 id)
{
	u32 cnt = 1000;
	u32 state;
	dsim_write_mask(id, DSIM_SWRST, ~0, DSIM_SWRST_RESET);

	do {
		state = dsim_read(id, DSIM_SWRST) & DSIM_SWRST_RESET;
		cnt--;
		udelay(10);
	} while (state && cnt);

	if (!cnt)
		dsim_err("%s is timeout.\n", __func__);
}

void dsim_reg_dphy_reset(u32 id)
{
	dsim_write_mask(id, DSIM_SWRST, 0, DSIM_DPHY_RST); /* reset active low */
	dsim_write_mask(id, DSIM_SWRST, ~0, DSIM_DPHY_RST); /* reset release */
}

void dsim_reg_dphy_resetn(u32 id, u32 en)
{
	u32 val = en ? 0 : ~0;
	dsim_write_mask(id, DSIM_SWRST, val, DSIM_DPHY_RST); /* reset active low */
}


void dsim_reg_function_reset(u32 id)
{
	//dsim_write_mask(id, DSIM_SWRST, ~0, DSIM_SWRST_FUNCRST);
	u32 cnt = 1000;
	u32 state;

	dsim_write_mask(id, DSIM_SWRST, ~0,  DSIM_SWRST_FUNCRST);
	do {
		state = dsim_read(id, DSIM_SWRST) & DSIM_SWRST_FUNCRST;
		cnt--;
		udelay(10);
	} while (state && cnt);

	if (!cnt)
		dsim_err("%s is timeout.\n", __func__);
}

u32 dsim_reg_get_clk_status(u32 id)
{
	u32 val;

	val = dsim_read(id, DSIM_LINK_STATUS2);
	return DSIM_LINK_STATUS2_CLK_LANE_STATUS_GET(val);
}

u32 dsim_reg_is_clk_stop(u32 id)
{
	if (dsim_read(id, DSIM_DPHY_STATUS) & DSIM_DPHY_STATUS_STOP_STATE_CLK)
		return 1;

	return 0;
}


void dsim_reg_dp_dn_swap(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;
	u32 mask = DSIM_PLLCTRL_DPDN_SWAP_DATA | DSIM_PLLCTRL_DPDN_SWAP_CLK;

	dsim_write_mask(id, DSIM_PLLCTRL, val, mask);
}

void dsim_reg_set_num_of_lane(u32 id, u32 lane)
{
	u32 val = DSIM_CONFIG_NUM_OF_DATA_LANE(lane);

	dsim_write_mask(id, DSIM_CONFIG, val, DSIM_CONFIG_NUM_OF_DATA_LANE_MASK);
}

void dsim_reg_enable_lane(u32 id, u32 lane, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_CONFIG, val, DSIM_CONFIG_LANES_EN(lane));
}

void dsim_reg_set_pll_freq(u32 id, u32 p, u32 m, u32 s)
{
	u32 val = (p & 0x3f) << 13 | (m & 0x3ff) << 3 | (s & 0x7) << 0;

	dsim_write_mask(id, DSIM_PLLCTRL, val, DSIM_PLLCTRL_PMS_MASK);
}

void dsim_reg_pll_stable_time(u32 id)
{
	dsim_write(id, DSIM_PLLTMR, DSIM_PLL_STABLE_TIME);
}

#if defined(CONFIG_SOC_EXYNOS8895)
void dsim_reg_set_b_dphy_ctrl1(u32 id, u32 b_dphy_ctrl1)
{
	dsim_write(id, DSIM_PHYCTRL_B1, b_dphy_ctrl1);
}

void dsim_reg_set_b_dphy_ctrl2(u32 id, u32 b_dphy_ctrl2)
{
	dsim_write(id, DSIM_PHYCTRL_B2, b_dphy_ctrl2);
}

void dsim_reg_set_b_dphy_ctrl3(u32 id, u32 b_dphy_ctrl3)
{
	dsim_write(id, DSIM_PHYCTRL_B3, b_dphy_ctrl3);
}

void dsim_reg_set_b_dphy_ctrl4(u32 id, u32 b_dphy_ctrl4)
{
	dsim_write(id, DSIM_PHYCTRL_B4, b_dphy_ctrl4);
}

void dsim_reg_set_m_dphy_ctrl1(u32 id, u32 m_dphy_ctrl1)
{
	dsim_write(id, DSIM_PHYCTRL_M1, m_dphy_ctrl1);
}

void dsim_reg_set_m_dphy_ctrl2(u32 id, u32 m_dphy_ctrl2)
{
	dsim_write(id, DSIM_PHYCTRL_M2, m_dphy_ctrl2);
}

void dsim_reg_set_m_dphy_ctrl3(u32 id, u32 m_dphy_ctrl3)
{
	dsim_write(id, DSIM_PHYCTRL_M3, m_dphy_ctrl3);
}

void dsim_reg_set_m_dphy_ctrl4(u32 id, u32 m_dphy_ctrl4)
{
	dsim_write(id, DSIM_PHYCTRL_M4, m_dphy_ctrl4);
}
#endif

void dsim_reg_set_dphy_timing_values(u32 id, struct dphy_timing_value *t)
{
	u32 val;

	val = DSIM_PHY_TIMING_M_TLPX_CTL(t->lpx) |
			DSIM_PHY_TIMING_M_THSEXIT_CTL(t->hs_exit);
	dsim_write(id, DSIM_PHY_TIMING, val);

	val = DSIM_PHY_TIMING1_M_TCLKPRPR_CTL(t->clk_prepare) |
		DSIM_PHY_TIMING1_M_TCLKZERO_CTL(t->clk_zero) |
		DSIM_PHY_TIMING1_M_TCLKPOST_CTL(t->clk_post) |
		DSIM_PHY_TIMING1_M_TCLKTRAIL_CTL(t->clk_trail);
	dsim_write(id, DSIM_PHY_TIMING1, val);

	val = DSIM_PHY_TIMING2_M_THSPRPR_CTL(t->hs_prepare) |
		DSIM_PHY_TIMING2_M_THSZERO_CTL(t->hs_zero) |
		DSIM_PHY_TIMING2_M_THSTRAIL_CTL(t->hs_trail);
	dsim_write(id, DSIM_PHY_TIMING2, val);

	val = DSIM_PHYCTRL_B1_B_DPHYCTL(t->b_dphyctl);
	dsim_reg_set_b_dphy_ctrl1(id, val);
}

void dsim_reg_clear_int(u32 id, u32 int_src)
{
	dsim_write(id, DSIM_INTSRC, int_src);
}

void dsim_reg_clear_int_all(u32 id)
{
	dsim_write(id, DSIM_INTSRC, 0xffffffff);
}

void dsim_reg_set_pll(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_PLLCTRL, val, DSIM_PLLCTRL_PLL_EN);
}

u32 dsim_reg_is_pll_stable(u32 id)
{
	u32 val;

	val = dsim_read(id, DSIM_LINK_STATUS3);
	if (val & DSIM_LINK_STATUS3_PLL_STABLE)
		return 1;

	return 0;
}

int dsim_reg_enable_pll(u32 id, u32 en)
{
	int ret;
	u32 cnt = 1000;

	if (en) {
		dsim_reg_clear_int(id, DSIM_INTSRC_PLL_STABLE);
		dsim_reg_set_pll(id, 1);
		while (cnt--) {
			cnt--;
			ret = dsim_reg_is_pll_stable(id);
			if (ret)
				return 0;

			if (cnt == 0) {
				dsim_err("ERR:%s:failed to set pll\n", __func__);
				return -EBUSY;
			}
			udelay(10);
		}
	} else {
		dsim_reg_set_pll(id, 0);
	}

	return 0;
}

void dsim_reg_set_esc_clk_prescaler(u32 id, u32 en, u32 p)
{
	u32 val = en ? DSIM_CLK_CTRL_ESCCLK_EN : 0;
	u32 mask = DSIM_CLK_CTRL_ESCCLK_EN | DSIM_CLK_CTRL_ESC_PRESCALER_MASK;

	val |= DSIM_CLK_CTRL_ESC_PRESCALER(p);
	dsim_write_mask(id, DSIM_CLK_CTRL, val, mask);
}

void dsim_reg_set_esc_clk_on_lane(u32 id, u32 en, u32 lane)
{
	u32 val = 0;

	if (en) {
		/* set for data lane */
		switch(lane) {
			case 1 : 
				val = 1;
				break;
			case 2 :
				val = 3;
				break;
			case 3 : 
				val = 7;
				break;
			case 4 :
				val = 0xf;
				break;
			default:
				dsim_err("DSIM:ERR:%s:invalid lane count : %d\n", __func__, lane);
				break;
		}
		/* set for clock lane */
		val |= (1 << 4);

		val = DSIM_CLK_CTRL_LANE_ESCCLK_EN(val);
	}	

	dsim_write_mask(id, DSIM_CLK_CTRL, val, DSIM_CLK_CTRL_LANE_ESCCLK_EN_MASK);
}

void dsim_reg_set_stop_state_cnt(u32 id)
{
	u32 val = DSIM_ESCMODE_STOP_STATE_CNT(DSIM_STOP_STATE_CNT);

	dsim_write_mask(id, DSIM_ESCMODE, val, DSIM_ESCMODE_STOP_STATE_CNT_MASK);
}

void dsim_reg_set_bta_timeout(u32 id)
{
	u32 val = DSIM_TIMEOUT_BTA_TOUT(DSIM_BTA_TIMEOUT);

	dsim_write_mask(id, DSIM_TIMEOUT, val, DSIM_TIMEOUT_BTA_TOUT_MASK);
}

void dsim_reg_set_lpdr_timeout(u32 id)
{
	u32 val = DSIM_TIMEOUT_LPDR_TOUT(DSIM_LP_RX_TIMEOUT);

	dsim_write_mask(id, DSIM_TIMEOUT, val, DSIM_TIMEOUT_LPDR_TOUT_MASK);
}

void dsim_reg_set_packet_ctrl(u32 id)
{
	u32 val = DSIM_CMD_CONFIG_MULTI_PKT_CNT(DSIM_MULTI_PACKET_CNT);
	dsim_write_mask(id, DSIM_CMD_CONFIG, val, DSIM_CMD_CONFIG_MULTI_PKT_CNT_MASK);
}

void dsim_reg_set_m_pll_ctrl1(u32 id, u32 m_pll_ctrl1)
{
	dsim_write(id, DSIM_PLL_CTRL1, m_pll_ctrl1);
}

void dsim_reg_set_m_pll_ctrl2(u32 id, u32 m_pll_ctrl2)
{
	dsim_write(id, DSIM_PLL_CTRL2, m_pll_ctrl2);
}

void dsim_reg_disable_hsa(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_CONFIG, val, DSIM_CONFIG_HSA_DISABLE);
}

void dsim_reg_disable_hbp(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_CONFIG, val, DSIM_CONFIG_HBP_DISABLE);
}

void dsim_reg_disable_hfp(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_CONFIG, val, DSIM_CONFIG_HFP_DISABLE);
}

void dsim_reg_disable_hse(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_CONFIG, val, DSIM_CONFIG_HSE_DISABLE);
}

void dsim_reg_set_burst_mode(u32 id, u32 burst)
{
	u32 val = burst ? ~0 : 0;

	dsim_write_mask(id, DSIM_CONFIG, val, DSIM_CONFIG_BURST_MODE);
}

void dsim_reg_set_sync_inform(u32 id, u32 inform)
{
	u32 val = inform ? ~0 : 0;

	dsim_write_mask(id, DSIM_CONFIG, val, DSIM_CONFIG_SYNC_INFORM);
}

void dsim_reg_set_vfp(u32 id, u32 vfp)
{
	u32 val = DSIM_VPORCH_VFP_TOTAL(vfp);

	dsim_write_mask(id, DSIM_VPORCH, val, DSIM_VPORCH_VFP_TOTAL_MASK);
}

void dsim_reg_set_cmdallow(u32 id, u32 cmdallow)
{
	u32 val = DSIM_VPORCH_VFP_CMD_ALLOW(cmdallow);

	dsim_write_mask(id, DSIM_VPORCH, val, DSIM_VPORCH_VFP_CMD_ALLOW_MASK);
}

void dsim_reg_set_stable_vfp(u32 id, u32 stablevfp)
{
	u32 val = DSIM_VPORCH_STABLE_VFP(stablevfp);

	dsim_write_mask(id, DSIM_VPORCH, val, DSIM_VPORCH_STABLE_VFP_MASK);
}

void dsim_reg_set_vbp(u32 id, u32 vbp)
{
	u32 val = DSIM_VPORCH_VBP(vbp);

	dsim_write_mask(id, DSIM_VPORCH, val, DSIM_VPORCH_VBP_MASK);
}

void dsim_reg_set_hfp(u32 id, u32 hfp)
{
	u32 val = DSIM_HPORCH_HFP(hfp);

	dsim_write_mask(id, DSIM_HPORCH, val, DSIM_HPORCH_HFP_MASK);
}

void dsim_reg_set_hbp(u32 id, u32 hbp)
{
	u32 val = DSIM_HPORCH_HBP(hbp);

	dsim_write_mask(id, DSIM_HPORCH, val, DSIM_HPORCH_HBP_MASK);
}

void dsim_reg_set_vsa(u32 id, u32 vsa)
{
	u32 val = DSIM_SYNC_VSA(vsa);

	dsim_write_mask(id, DSIM_SYNC, val, DSIM_SYNC_VSA_MASK);
}

void dsim_reg_set_hsa(u32 id, u32 hsa)
{
	u32 val = DSIM_SYNC_HSA(hsa);

	dsim_write_mask(id, DSIM_SYNC, val, DSIM_SYNC_HSA_MASK);
}

void dsim_reg_set_vresol(u32 id, u32 vresol)
{
	u32 val = DSIM_RESOL_VRESOL(vresol);

	dsim_write_mask(id, DSIM_RESOL, val, DSIM_RESOL_VRESOL_MASK);
}

void dsim_reg_set_hresol(u32 id, u32 hresol, struct decon_lcd *lcd)
{
	u32 width, val;

	if (lcd->dsc_enabled)
		width = hresol / 3;
	else
		width = hresol;

	val = DSIM_RESOL_HRESOL(width);

	dsim_write_mask(id, DSIM_RESOL, val, DSIM_RESOL_HRESOL_MASK);
}

void dsim_reg_set_porch(u32 id, struct decon_lcd *lcd)
{
	if (lcd->mode == DECON_VIDEO_MODE) {
		dsim_reg_set_vbp(id, lcd->vbp);
		dsim_reg_set_vfp(id, lcd->vfp);
		dsim_reg_set_stable_vfp(id, DSIM_STABLE_VFP_VALUE);
		dsim_reg_set_cmdallow(id, DSIM_CMD_ALLOW_VALUE);
		dsim_reg_set_hbp(id, lcd->hbp);
		dsim_reg_set_hfp(id, lcd->hfp);
		dsim_reg_set_vsa(id, lcd->vsa);
		dsim_reg_set_hsa(id, lcd->hsa);
	}
}

/* Set porch and resolution to support Partial update */
void dsim_reg_set_partial_update(u32 id, struct decon_lcd *lcd_info)
{
	dsim_reg_set_vresol(id, lcd_info->yres);
	dsim_reg_set_hresol(id, lcd_info->xres, lcd_info);
	dsim_reg_set_porch(id, lcd_info);
	dsim_reg_set_num_of_transfer(id, lcd_info->yres);
}

void dsim_reg_set_pixel_format(u32 id, u32 pixformat)
{
	u32 val = DSIM_CONFIG_PIXEL_FORMAT(pixformat);

	dsim_write_mask(id, DSIM_CONFIG, val, DSIM_CONFIG_PIXEL_FORMAT_MASK);
}

void dsim_reg_set_cmd_transfer_mode(u32 id, u32 lp)
{
	u32 val = lp ? ~0 : 0;

	dsim_write_mask(id, DSIM_ESCMODE, val, DSIM_ESCMODE_CMD_LPDT);
}

void dsim_reg_set_vc_id(u32 id, u32 vcid)
{
	u32 val = DSIM_CONFIG_VC_ID(vcid);

	dsim_write_mask(id, DSIM_CONFIG, val, DSIM_CONFIG_VC_ID_MASK);
}

void dsim_reg_set_video_mode(u32 id, u32 mode)
{
	u32 val = mode ? ~0 : 0;

	dsim_write_mask(id, DSIM_CONFIG, val, DSIM_CONFIG_VIDEO_MODE);
}

void dsim_reg_enable_dsc(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_CONFIG, val, DSIM_CONFIG_CPRS_EN);
}

void dsim_reg_set_num_of_slice(u32 id, u32 num_of_slice)
{
	u32 val = DSIM_CPRS_CTRL_NUM_OF_SLICE(num_of_slice);

	dsim_write_mask(id, DSIM_CPRS_CTRL, val, DSIM_CPRS_CTRL_NUM_OF_SLICE_MASK);
}

void dsim_reg_get_num_of_slice(u32 id, u32 *num_of_slice)
{
	u32 val = dsim_read(id, DSIM_CPRS_CTRL);

	*num_of_slice = DSIM_CPRS_CTRL_NUM_OF_SLICE_GET(val);
}

void dsim_reg_set_multi_slice(u32 id, struct decon_lcd *lcd_info)
{
	u32 multi_slice = 1;
	u32 val;

	/* if multi-slice(2~4 slices) DSC compression is used in video mode
	 * MULTI_SLICE_PACKET configuration must be matched to DDI's configuration */
	if (lcd_info->mode == DECON_MIPI_COMMAND_MODE)
		multi_slice = 1;
	else if (lcd_info->mode == DECON_VIDEO_MODE)
		multi_slice = lcd_info->dsc_slice_num > 1 ? 1 : 0;

	/* if MULTI_SLICE_PACKET is enabled, only one packet header is transferred
	 * for multi slice */
	val = multi_slice ? ~0 : 0;
	dsim_write_mask(id, DSIM_CPRS_CTRL, val, DSIM_CPRS_CTRL_MULI_SLICE_PACKET);
}

void dsim_reg_set_size_of_slice(u32 id, struct decon_lcd *lcd_info)
{
	u32 slice_w = lcd_info->xres / lcd_info->dsc_slice_num;
	u32 val_01 = 0, mask_01 = 0;
	u32 val_23 = 0, mask_23 = 0;

	if (lcd_info->dsc_slice_num == 4) {
		val_01 = DSIM_SLICE01_SIZE_OF_SLICE1(slice_w) |
			DSIM_SLICE01_SIZE_OF_SLICE0(slice_w);
		mask_01 = DSIM_SLICE01_SIZE_OF_SLICE1_MASK |
			DSIM_SLICE01_SIZE_OF_SLICE0_MASK;
		val_23 = DSIM_SLICE23_SIZE_OF_SLICE3(slice_w) |
			DSIM_SLICE23_SIZE_OF_SLICE2(slice_w);
		mask_23 = DSIM_SLICE23_SIZE_OF_SLICE3_MASK |
			DSIM_SLICE23_SIZE_OF_SLICE2_MASK;

		dsim_write_mask(id, DSIM_SLICE01, val_01, mask_01);
		dsim_write_mask(id, DSIM_SLICE23, val_23, mask_23);

	} else if (lcd_info->dsc_slice_num == 2 && lcd_info->dsc_cnt == 2) {

		val_01 = DSIM_SLICE01_SIZE_OF_SLICE1(slice_w) |
			DSIM_SLICE01_SIZE_OF_SLICE0(slice_w);
		mask_01 = DSIM_SLICE01_SIZE_OF_SLICE1_MASK |
			DSIM_SLICE01_SIZE_OF_SLICE0_MASK;
		val_23 = DSIM_SLICE23_SIZE_OF_SLICE3(slice_w) |
			DSIM_SLICE23_SIZE_OF_SLICE2(slice_w);
		mask_23 = DSIM_SLICE23_SIZE_OF_SLICE3_MASK |
			DSIM_SLICE23_SIZE_OF_SLICE2_MASK;

		dsim_write_mask(id, DSIM_SLICE01, val_01, mask_01);
		dsim_write_mask(id, DSIM_SLICE23, val_23, mask_23);

	} else if (lcd_info->dsc_slice_num == 2 && lcd_info->dsc_cnt == 1) {
		val_01 = DSIM_SLICE01_SIZE_OF_SLICE1(slice_w) |
			DSIM_SLICE01_SIZE_OF_SLICE0(slice_w);
		mask_01 = DSIM_SLICE01_SIZE_OF_SLICE1_MASK |
			DSIM_SLICE01_SIZE_OF_SLICE0_MASK;
		dsim_write_mask(id, DSIM_SLICE01, val_01, mask_01);
	} else {
		dsim_err("not supported slice mode. dsc(%d), slice(%d)\n",
				lcd_info->dsc_cnt, lcd_info->dsc_slice_num);
	}

}

void dsim_reg_print_size_of_slice(u32 id)
{
	u32 val;
	u32 slice0_w, slice1_w, slice2_w, slice3_w;

	val = dsim_read(id, DSIM_SLICE01);
	slice0_w = DSIM_SLICE01_SIZE_OF_SLICE0_GET(val);
	slice1_w = DSIM_SLICE01_SIZE_OF_SLICE1_GET(val);

	val = dsim_read(id, DSIM_SLICE23);
	slice2_w = DSIM_SLICE23_SIZE_OF_SLICE2_GET(val);
	slice3_w = DSIM_SLICE23_SIZE_OF_SLICE3_GET(val);

	dsim_dbg("dsim%d: slice0 w(%d), slice1 w(%d), slice2 w(%d), slice3(%d)\n",
			id, slice0_w, slice1_w, slice2_w, slice3_w);
}

void dsim_reg_set_multi_packet_count(u32 id, u32 multipacketcnt)
{
	u32 val = DSIM_CMD_CONFIG_MULTI_PKT_CNT(multipacketcnt);

	dsim_write_mask(id, DSIM_CMD_CONFIG, val, DSIM_CMD_CONFIG_MULTI_PKT_CNT_MASK);
}

void dsim_reg_set_time_stable_vfp(u32 id, u32 stablevfp)
{
	u32 val = DSIM_CMD_TE_CTRL0_TIME_STABLE_VFP(stablevfp);

	dsim_write_mask(id, DSIM_CMD_TE_CTRL0, val,
			DSIM_CMD_TE_CTRL0_TIME_STABLE_VFP_MASK);
}

void dsim_reg_set_time_te_protect_on(u32 id, u32 teprotecton)
{
	u32 val = DSIM_CMD_TE_CTRL1_TIME_TE_PROTECT_ON(teprotecton);

	dsim_write_mask(id, DSIM_CMD_TE_CTRL1, val,
			DSIM_CMD_TE_CTRL1_TIME_TE_PROTECT_ON_MASK);
}

void dsim_reg_set_time_te_timeout(u32 id, u32 tetout)
{
	u32 val = DSIM_CMD_TE_CTRL1_TIME_TE_TOUT(tetout);

	dsim_write_mask(id, DSIM_CMD_TE_CTRL1, val,
			DSIM_CMD_TE_CTRL1_TIME_TE_TOUT_MASK);
}

void dsim_reg_set_cmd_ctrl(u32 id, struct decon_lcd *lcd_info, struct dsim_clks *clks)
{
	unsigned int time_stable_vfp;
	unsigned int time_te_protect_on;
	unsigned int time_te_tout;

	time_stable_vfp = lcd_info->xres * DSIM_STABLE_VFP_VALUE * 3 / 100;
	time_te_protect_on = (clks->hs_clk * TE_PROTECT_ON_TIME) / 16;
	time_te_tout = (clks->hs_clk * TE_TIMEOUT_TIME) / 16;
	dsim_reg_set_time_stable_vfp(id, time_stable_vfp);
	dsim_reg_set_time_te_protect_on(id, time_te_protect_on);
	dsim_reg_set_time_te_timeout(id, time_te_tout);
}

void dsim_reg_enable_noncont_clock(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_CLK_CTRL, val, DSIM_CLK_CTRL_NONCONT_CLOCK_LANE);
}

void dsim_reg_enable_clocklane(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_CONFIG, val, DSIM_CLK_CTRL_CLKLANE_ONOFF);
}

void dsim_reg_enable_packetgo(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_CMD_CONFIG, val, DSIM_CMD_CONFIG_PKT_GO_EN);
}

void dsim_reg_set_packetgo_ready(u32 id)
{
	dsim_write_mask(id, DSIM_CMD_CONFIG, ~0, DSIM_CMD_CONFIG_PKT_GO_RDY);
}

void dsim_reg_enable_multi_cmd_packet(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_CMD_CONFIG, val, DSIM_CMD_CONFIG_MULTI_CMD_PKT_EN);
}

void dsim_reg_enable_shadow_read(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_SFR_CTRL, val, DSIM_SFR_CTRL_SHADOW_REG_READ_EN);
}

void dsim_reg_enable_shadow(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_SFR_CTRL, val, DSIM_SFR_CTRL_SHADOW_EN);
}

void dsim_reg_enable_hs_clock(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_CLK_CTRL, val, DSIM_CLK_CTRL_TX_REQUEST_HSCLK);
}

void dsim_reg_enable_word_clock(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_CLK_CTRL, val, DSIM_CLK_CTRL_WORDCLK_EN);
}

void dsim_reg_enable_loopback(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_CSIS_LB, val, DSIM_CSIS_LB_CSIS_LB_EN);
}

void dsim_reg_set_loopback_id(u32 id, u32 lb_id)
{
	u32 val = DSIM_CSIS_LB_CSIS_PH(lb_id);

	dsim_write_mask(id, DSIM_CSIS_LB, val, DSIM_CSIS_LB_CSIS_PH_MASK);
}

u32 dsim_reg_is_hs_clk_ready(u32 id)
{
	if (dsim_read(id, DSIM_DPHY_STATUS) & DSIM_DPHY_STATUS_TX_READY_HS_CLK)
		return 1;

	return 0;
}

void dsim_reg_enable_per_frame_read(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_CONFIG, val, DSIM_CONFIG_PER_FRAME_READ_EN);
}

int dsim_reg_wait_hs_clk_ready(u32 id)
{
	u32 state;
	u32 cnt = 1000;

	do {
		state = dsim_reg_is_hs_clk_ready(id);
		cnt--;
		udelay(10);
	} while (!state && cnt);

	if (!cnt) {
		dsim_err("DSI Master is not HS state.\n");
		return -EBUSY;
	}

	return 0;
}

int dsim_reg_wait_hs_clk_disable(u32 id)
{
	u32 state;
	u32 cnt = 1000;

	do {
		state = (dsim_reg_get_clk_status(id) == 0) ? 1 : 0;
		cnt--;
		udelay(10);
	} while (!state && cnt);

	if (!cnt) {
		dsim_err("DSI Master clock isn't disabled.\n");
		return -EBUSY;
	}

	return 0;
}

u32 dsim_reg_is_writable_fifo_state(u32 id)
{
	u32 val = dsim_read(id, DSIM_FIFOCTRL);
	if (DSIM_FIFOCTRL_NUMBER_OF_PH_SFR_GET(val) < DSIM_FIFOCTRL_THRESHOLD)
		return 1;
	else
		return 0;
}

u32 dsim_reg_header_fifo_is_empty(u32 id)
{
	return dsim_read_mask(id, DSIM_FIFOCTRL, DSIM_FIFOCTRL_EMPTY_PH_SFR);
}

void dsim_reg_force_dphy_stop_state(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_ESCMODE, val, DSIM_ESCMODE_FORCE_STOP_STATE);
}

void dsim_reg_wr_tx_header(u32 id, u8 data_id, u8 data0, u8 data1, u32 bta_type)
{
	u32 tmp = (u32)data0;
	u32 val = DSIM_PKTHDR_BTA_TYPE(bta_type) |DSIM_PKTHDR_ID(data_id) |
		DSIM_PKTHDR_DATA0(tmp) | DSIM_PKTHDR_DATA1(data1);

	dsim_write_mask(id, DSIM_PKTHDR, val, DSIM_PKTHDR_DATA);
}

void dsim_reg_wr_tx_payload(u32 id, u32 payload)
{
	dsim_write(id, DSIM_PAYLOAD, payload);
}

void dsim_reg_enter_ulps(u32 id, u32 enter)
{
	u32 val = enter ? ~0 : 0;
	u32 mask = DSIM_ESCMODE_TX_ULPS_CLK | DSIM_ESCMODE_TX_ULPS_DATA;

	dsim_write_mask(id, DSIM_ESCMODE, val, mask);
}

void dsim_reg_exit_ulps(u32 id, u32 exit)
{
	u32 val = exit ? ~0 : 0;
	u32 mask = DSIM_ESCMODE_TX_ULPS_CLK_EXIT | DSIM_ESCMODE_TX_ULPS_DATA_EXIT;

	dsim_write_mask(id, DSIM_ESCMODE, val, mask);
}

void dsim_reg_set_num_of_transfer(u32 id, u32 num_of_transfer)
{
	u32 val = DSIM_NUM_OF_TRANSFER_PER_FRAME(num_of_transfer);

	dsim_write_mask(id, DSIM_NUM_OF_TRANSFER, val, DSIM_NUM_OF_TRANSFER_PER_FRAME_MASK);
}

static u32 dsim_reg_is_ulps_state(u32 id, u32 lanes)
{
	u32 val = dsim_read(id, DSIM_DPHY_STATUS);
	u32 data_lane = lanes >> DSIM_LANE_CLOCK;

	if ((DSIM_DPHY_STATUS_ULPS_DATA_LANE_GET(val) == data_lane)
			&& (val & DSIM_DPHY_STATUS_ULPS_CLK))
		return 1;

	return 0;
}

static void dsim_reg_set_deskew_hw_interval(u32 id, u32 interval)
{
	u32 val = DSIM_DESKEW_CTRL_HW_INTERVAL(interval);

	dsim_write_mask(id, DSIM_DESKEW_CTRL, val,
			DSIM_DESKEW_CTRL_HW_INTERVAL_MASK);
}

static void dsim_reg_set_deskew_hw_position(u32 id, u32 position)
{
	u32 val = position ? ~0 : 0;

	dsim_write_mask(id, DSIM_DESKEW_CTRL, val, DSIM_DESKEW_CTRL_HW_POSITION);
}

static void dsim_reg_enable_deskew_hw_enable(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_DESKEW_CTRL, val, DSIM_DESKEW_CTRL_HW_EN);
}

static void dsim_reg_set_cm_underrun_lp_ref(u32 id, u32 lp_ref)
{
	u32 val = DSIM_UNDERRUN_CTRL_CM_UNDERRUN_LP_REF(lp_ref);

	dsim_write_mask(id, DSIM_UNDERRUN_CTRL, val,
			DSIM_UNDERRUN_CTRL_CM_UNDERRUN_LP_REF_MASK);
}

static void dsim_reg_set_threshold(u32 id, u32 threshold)
{
	u32 val = DSIM_THRESHOLD_LEVEL(threshold);

	dsim_write_mask(id, DSIM_THRESHOLD, val, DSIM_THRESHOLD_LEVEL_MASK);

}

static void dsim_reg_enable_eotp(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_CONFIG, val, DSIM_CONFIG_EOTP_EN);
}

static void dsim_reg_set_vt_compensate(u32 id, u32 compensate)
{
	u32 val = DSIM_VIDEO_TIMER_COMPENSATE(compensate);

	dsim_write_mask(id, DSIM_VIDEO_TIMER, val,
			DSIM_VIDEO_TIMER_COMPENSATE_MASK);
}

static void dsim_reg_set_vstatus_int(u32 id, u32 vstatus)
{
	u32 val = DSIM_VIDEO_TIMER_VSTATUS_INTR_SEL(vstatus);

	dsim_write_mask(id, DSIM_VIDEO_TIMER, val,
			DSIM_VIDEO_TIMER_VSTATUS_INTR_SEL_MASK);
}

static void dsim_reg_set_bist_mode(u32 id, u32 bist_mode)
{
	u32 val = DSIM_BIST_CTRL0_BIST_PTRN_MODE(bist_mode);

	dsim_write_mask(id, DSIM_BIST_CTRL0, val,
			DSIM_BIST_CTRL0_BIST_PTRN_MODE_MASK);
}

static void dsim_reg_enable_bist_pattern_move(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_BIST_CTRL0, val,
			DSIM_BIST_CTRL0_BIST_PTRN_MOVE_EN);
}

static void dsim_reg_enable_bist(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_BIST_CTRL0, val, DSIM_BIST_CTRL0_BIST_EN);
}

void dsim_reg_set_bist_pattern_prbs7_seed(u32 id, u32 seed)
{
	u32 val = DSIM_BIST_CTRL1_BIST_PTRN_PRBS7_SEED(seed);

	dsim_write_mask(id, DSIM_BIST_CTRL1, val,
			DSIM_BIST_CTRL1_BIST_PTRN_PRBS7_SEED_MASK);
}

void dsim_reg_set_bist_pattern_user_r(u32 id, u32 r)
{
	u32 val = DSIM_BIST_CTRL1_BIST_PTRN_USER_R(r);

	dsim_write_mask(id, DSIM_BIST_CTRL1, val,
			DSIM_BIST_CTRL1_BIST_PTRN_USER_R_MASK);
}

void dsim_reg_set_bist_pattern_user_g(u32 id, u32 g)
{
	u32 val = DSIM_BIST_CTRL1_BIST_PTRN_USER_G(g);

	dsim_write_mask(id, DSIM_BIST_CTRL1, val,
			DSIM_BIST_CTRL1_BIST_PTRN_USER_G_MASK);
}

void dsim_reg_set_bist_pattern_user_b(u32 id, u32 b)
{
	u32 val = DSIM_BIST_CTRL1_BIST_PTRN_USER_B(b);

	dsim_write_mask(id, DSIM_BIST_CTRL1, val,
			DSIM_BIST_CTRL1_BIST_PTRN_USER_B_MASK);
}

void dsim_set_bist(u32 id, u32 en)
{
	if (en) {
		dsim_reg_set_bist_mode(id, DSIM_COLOR_BAR);
		dsim_reg_enable_bist_pattern_move(id, false);
		dsim_reg_enable_bist(id, en);
	}
}

static void dsim_set_hw_deskew(u32 id, u32 en)
{
	u32 hw_interval = 1;

	if (en) {
		dsim_reg_set_deskew_hw_interval(id, hw_interval);
		/* 0 : VBP first line, 1 : VFP last line*/
		dsim_reg_set_deskew_hw_position(id, 0);
		dsim_reg_enable_deskew_hw_enable(id, en);
	} else {
		dsim_reg_enable_deskew_hw_enable(id, en);
	}
}

int dsim_reg_wait_enter_ulps_state(u32 id, u32 lanes)
{
	u32 state;
	u32 cnt = 1000;

	do {
		state = dsim_reg_is_ulps_state(id, lanes);
		cnt--;
		udelay(10);
	} while (!state && cnt);

	if (!cnt) {
		dsim_err("DSI Master is not ULPS state.\n");
		return -EBUSY;
	} else {
		dsim_dbg("DSI Master is ULPS state.\n");
	}

	return 0;
}

static u32 dsim_reg_is_not_ulps_state(u32 id)
{
	u32 val = dsim_read(id, DSIM_DPHY_STATUS);

	if (!(DSIM_DPHY_STATUS_ULPS_DATA_LANE_GET(val))
			&& !(val & DSIM_DPHY_STATUS_ULPS_CLK))
		return 1;

	return 0;
}

int dsim_reg_wait_exit_ulps_state(u32 id)
{
	u32 state;
	u32 cnt = 1000;

	do {
		state = dsim_reg_is_not_ulps_state(id);
		cnt--;
		udelay(10);
	} while (!state && cnt);

	if (!cnt) {
		dsim_err("DSI Master is not stop state.\n");
		return -EBUSY;
	} else {
		dsim_dbg("DSI Master is stop state.\n");
	}

	return 0;
}

static int dsim_reg_get_dphy_timing(u32 hs_clk, u32 esc_clk,
		struct dphy_timing_value *t)
{
	int val;

	val  = (dphy_timing[0][0] - hs_clk) / 10;

	if (val > ((sizeof(dphy_timing) / sizeof(dphy_timing[0])) - 1)) {
		dsim_err("%u Mhz hs clock can't find proper dphy timing values\n",
				hs_clk);
		return -EINVAL;
	}

	t->bps = hs_clk;
	t->clk_prepare = dphy_timing[val][1];
	t->clk_zero = dphy_timing[val][2];
	t->clk_post = dphy_timing[val][3];
	t->clk_trail = dphy_timing[val][4];
	t->hs_prepare = dphy_timing[val][5];
	t->hs_zero = dphy_timing[val][6];
	t->hs_trail = dphy_timing[val][7];
	t->lpx = dphy_timing[val][8];
	t->hs_exit = dphy_timing[val][9];

	dsim_dbg("%s: bps(%u) clk_prepare(%u) clk_zero(%u) clk_post(%u)\n",
			__func__, t->bps, t->clk_prepare, t->clk_zero, t->clk_post);
	dsim_dbg("clk_trail(%u) hs_prepare(%u) hs_zero(%u) hs_trail(%u)\n",
			t->clk_trail, t->hs_prepare, t->hs_zero, t->hs_trail);
	dsim_dbg("lpx(%u) hs_exit(%u)\n", t->lpx, t->hs_exit);

	if ((esc_clk > 20) || (esc_clk < 7)) {
		dsim_err("%u Mhz cann't be used as escape clock\n", esc_clk);
		return -EINVAL;
	}

	t->b_dphyctl = b_dphyctl[esc_clk - 7];
	dsim_dbg("b_dphyctl(%u)\n", t->b_dphyctl);

	return 0;
}

void dsim_reg_set_config(u32 id, struct decon_lcd *lcd_info, u32 data_lane_cnt,
		struct dsim_clks *clks)
{
	u32 threshold;
	u32 num_of_slice;
	u32 num_of_transfer;

	if(lcd_info->mode == DECON_VIDEO_MODE)
		dsim_reg_enable_clocklane(id, 0);
	else
		dsim_reg_enable_noncont_clock(id, 1);

	dsim_set_hw_deskew(id, 0); /* second param is to control enable bit */

	dsim_reg_set_bta_timeout(id);
	dsim_reg_set_lpdr_timeout(id);
	dsim_reg_set_stop_state_cnt(id);

	if (lcd_info->mode == DECON_MIPI_COMMAND_MODE)
		dsim_reg_set_cm_underrun_lp_ref(id, lcd_info->cmd_underrun_lp_ref);

	if (lcd_info->dsc_enabled)
		threshold = lcd_info->xres / 3;
	else
		threshold = lcd_info->xres;

	if (lcd_info->mode == DECON_MIPI_COMMAND_MODE)
		dsim_reg_set_threshold(id, threshold);

	dsim_reg_set_vresol(id, lcd_info->yres);
	dsim_reg_set_hresol(id, lcd_info->xres, lcd_info);
	dsim_reg_set_porch(id, lcd_info);

	if (lcd_info->mode == DECON_MIPI_COMMAND_MODE){
		if (lcd_info->dsc_enabled)
			num_of_transfer = lcd_info->xres * lcd_info->yres / threshold / 3;
		else
			num_of_transfer = lcd_info->xres * lcd_info->yres / threshold;

		dsim_reg_set_num_of_transfer(id, num_of_transfer);
	}

	dsim_reg_set_num_of_lane(id, (data_lane_cnt-1));
	dsim_reg_enable_eotp(id, 1);
	dsim_reg_enable_per_frame_read(id, 0);
	dsim_reg_set_pixel_format(id, DSIM_PIXEL_FORMAT_RGB24);
	dsim_reg_set_vc_id(id, 0);
	if (lcd_info->mode == DECON_VIDEO_MODE)
		dsim_reg_set_video_mode(id, DSIM_CONFIG_VIDEO_MODE);
	else
		dsim_reg_set_video_mode(id, 0);

	dsim_reg_enable_dsc(id, lcd_info->dsc_enabled);

	if (lcd_info->mode == DECON_VIDEO_MODE) {
		dsim_reg_disable_hsa(id, 0);
		dsim_reg_disable_hbp(id, 0);
		dsim_reg_disable_hfp(id, 1);
		dsim_reg_disable_hse(id, 0);
		dsim_reg_set_burst_mode(id, 1);
		dsim_reg_set_sync_inform(id, 0);
		dsim_reg_enable_clocklane(id, 1);
	}

	if (lcd_info->dsc_enabled) {
		dsim_dbg("%s: dsc configuration is set\n", __func__);
		dsim_reg_set_num_of_slice(id, lcd_info->dsc_slice_num);
		dsim_reg_set_multi_slice(id, lcd_info); /* multi slice */
		dsim_reg_set_size_of_slice(id, lcd_info);

		dsim_reg_get_num_of_slice(id, &num_of_slice);
		dsim_dbg("dsim%d: number of DSC slice(%d)\n", id, num_of_slice);
		dsim_reg_print_size_of_slice(id);
	}

	if (lcd_info->mode == DECON_VIDEO_MODE) {
		dsim_reg_set_multi_packet_count(id, 0xff);
		dsim_reg_enable_multi_cmd_packet(id, 0);
	}
	dsim_reg_enable_packetgo(id, 0);

	if (lcd_info->mode == DECON_MIPI_COMMAND_MODE) {
		dsim_reg_set_cmd_ctrl(id, lcd_info, clks);
	} else if (lcd_info->mode == DECON_VIDEO_MODE) {
		/* TODO: vt compensation value will be replaced to calculated value */
		dsim_reg_set_vt_compensate(id, lcd_info->vt_compensation);
		dsim_reg_set_vstatus_int(id, DSIM_VSYNC);
	}

	dsim_reg_enable_shadow_read(id, 1);
	dsim_reg_enable_shadow(id, 1);

	/* TODO: This is for dsim BIST mode in zebu emulator. only for test*/
	/* dsim_set_bist(id, true); */
}

#if 0 
void dsim_reg_init_probe(u32 id, struct decon_lcd *lcd_info, u32 data_lane_cnt,
		struct dsim_clks *clks)
{
	return;
}
#endif
int dsim_reg_init(u32 id, struct decon_lcd *lcd_info, u32 data_lane_cnt,
		struct dsim_clks *clks)
{
	int ret = 0;
	/*
	 * DSIM does not need to init, if DSIM is already
	 * becomes txrequest_hsclk = 1'b1 because of LCD_ON_UBOOT
	 */
#if 0 
	if (dsim_read_mask(id, DSIM_CLK_CTRL, DSIM_CLK_CTRL_TX_REQUEST_HSCLK)) {
		dsim_info("dsim%d is probed with LCD ON UBOOT\n", id);
		dsim_reg_init_probe(id, lcd_info, data_lane_cnt, clks);
		/* If reg_init_probe() sequence is not equal to reg_init()
		   then just return. */
		ret = -EBUSY;
		return ret;
	}
#endif
	/*set config*/
	dsim_reg_set_config(id, lcd_info, data_lane_cnt, clks);

	return ret;
}

/*
 * configure and set DPHY PLL, byte clock, escape clock and hs clock
 *	- PMS value have to be optained by using PMS Gen. tool (MSC_PLL_WIZARD2_00.exe)
 *	- PLL out is source clock of HS clock
 *	- byte clock = HS clock / 16
 *	- calculate divider of escape clock using requested escape clock
 *	  from driver
 *	- DPHY PLL, byte clock, escape clock are enabled.
 *	- HS clock will be enabled another function.
 *
 * Parameters
 *	- hs_clk : in/out parameter.
 *		in :  requested hs clock. out : calculated hs clock
 *	- esc_clk : in/out paramter.
 *		in : requested escape clock. out : calculated escape clock
 *	- word_clk : out parameter. byte clock = hs clock / 16
 */
int dsim_reg_set_clocks(u32 id, struct dsim_clks *clks,
		struct stdphy_pms *dphy_pms, u32 en)
{
	unsigned int esc_div;
	struct dsim_pll_param pll;
	struct dphy_timing_value t;
	int ret = 0;
	u32 fin = 26;

	if (en) {
		/*
		 * Do not need to set clocks related with PLL,
		 * if DPHY_PLL is already stabled because of LCD_ON_UBOOT.
		 */
		if (dsim_reg_is_pll_stable(id)) {
			dsim_info("dsim%d DPHY PLL is already stable\n", id);
			return -EBUSY;
		}

		/*
		 * set p, m, s to DPHY PLL
		 * PMS value has to be optained by PMS calculation tool
		 * released to customer
		 */
		pll.p = dphy_pms->p;
		pll.m = dphy_pms->m;
		pll.s = dphy_pms->s;
		/* requested DPHY PLL frequency(HS clock) */
		pll.pll_freq = (pll.m * fin) / (pll.p * (1 << pll.s));

		/* store calculated hs clock */
		clks->hs_clk = pll.pll_freq;

		/* get byte clock */
		clks->word_clk = clks->hs_clk / 16;
		dsim_dbg("byte clock is %u MHz\n", clks->word_clk);

#if defined(CONFIG_SOC_EXYNOS8895)
		clks->word_clk = clks->hs_clk / 16;
		dsim_dbg("word clock is %u MHz\n", clks->word_clk);

		/* requeseted escape clock */
		dsim_dbg("requested escape clock %u MHz\n", clks->esc_clk);

		/* escape clock divider */
		esc_div = clks->word_clk / clks->esc_clk;

		/* adjust escape clock */
		if ((clks->word_clk / esc_div) > clks->esc_clk)
			esc_div += 1;
		/* adjusted escape clock */
		clks->esc_clk = clks->word_clk / esc_div;
		dsim_dbg("escape clock divider is 0x%x\n", esc_div);
		dsim_dbg("escape clock is %u MHz\n", clks->esc_clk);
#else
		/* requeseted escape clock */
		dsim_dbg("requested escape clock %u MHz\n", clks->esc_clk);

		/* escape clock divider */
		esc_div = clks->word_clk / clks->esc_clk;

		/* adjust escape clock */
		if ((clks->word_clk / esc_div) > clks->esc_clk)
			esc_div += 1;
		/* adjusted escape clock */
		clks->esc_clk = clks->word_clk / esc_div;
		dsim_dbg("escape clock divider is 0x%x\n", esc_div);
		dsim_dbg("escape clock is %u MHz\n", clks->esc_clk);
#endif

		dsim_reg_set_esc_clk_prescaler(id, 1, esc_div);
		/* get DPHY timing values using hs clock and escape clock */
		dsim_reg_get_dphy_timing(clks->hs_clk, clks->esc_clk, &t);
		dsim_reg_set_dphy_timing_values(id, &t);

		dsim_reg_set_m_pll_ctrl1(id, DSIM_PHY_CHARIC_VAL[id][M_PLL_CTRL1]);
		dsim_reg_set_m_pll_ctrl2(id, DSIM_PHY_CHARIC_VAL[id][M_PLL_CTRL2]);
		dsim_reg_set_b_dphy_ctrl2(id, DSIM_PHY_CHARIC_VAL[id][B_DPHY_CTRL2]);
		dsim_reg_set_b_dphy_ctrl3(id, DSIM_PHY_CHARIC_VAL[id][B_DPHY_CTRL3]);
		dsim_reg_set_b_dphy_ctrl4(id, DSIM_PHY_CHARIC_VAL[id][B_DPHY_CTRL4]);
		dsim_reg_set_m_dphy_ctrl1(id, DSIM_PHY_CHARIC_VAL[id][M_DPHY_CTRL1]);
		dsim_reg_set_m_dphy_ctrl2(id, DSIM_PHY_CHARIC_VAL[id][M_DPHY_CTRL2]);
		dsim_reg_set_m_dphy_ctrl3(id, DSIM_PHY_CHARIC_VAL[id][M_DPHY_CTRL3]);
		dsim_reg_set_m_dphy_ctrl4(id, DSIM_PHY_CHARIC_VAL[id][M_DPHY_CTRL4]);

		dsim_reg_set_pll_freq(id, pll.p, pll.m, pll.s);
		/* set PLL's lock time */
		dsim_reg_pll_stable_time(id);

		/* enable PLL */
		ret = dsim_reg_enable_pll(id, 1);
	} else {
		dsim_reg_set_m_pll_ctrl1(id, 0x0);
		dsim_reg_set_m_pll_ctrl2(id, 0x0);
		dsim_reg_set_esc_clk_prescaler(id, 0, 0xff);
		dsim_reg_enable_pll(id, 0);
	}

	return ret;
}

int dsim_reg_set_lanes(u32 id, u32 lanes, u32 en)
{
	dsim_reg_enable_lane(id, lanes, en);
	udelay(400);

	return 0;
}

u32 dsim_reg_is_noncont_clk_enabled(u32 id)
{
	int ret;
	ret = dsim_read_mask(id, DSIM_CLK_CTRL, DSIM_CLK_CTRL_NONCONT_CLOCK_LANE);
	return ret;
}

int dsim_reg_set_hs_clock(u32 id, u32 en)
{
	int reg = 0;
	int is_noncont = dsim_reg_is_noncont_clk_enabled(id);

	if (en) {
		dsim_reg_enable_hs_clock(id, 1);
		if (!is_noncont)
			reg = dsim_reg_wait_hs_clk_ready(id);
	} else {
		dsim_reg_enable_hs_clock(id, 0);
		reg = dsim_reg_wait_hs_clk_disable(id);
	}
	return reg;
}

void dsim_reg_set_int(u32 id, u32 en)
{
	u32 val = en ? 0 : ~0;
	u32 mask;

	/*
	 * TODO: underrun irq will be unmasked in the future.
	 * underrun irq(dsim_reg_set_config) is ignored in zebu emulator.
	 * it's not meaningful
	 */
	mask = DSIM_INTMSK_SW_RST_RELEASE | DSIM_INTMSK_SFR_PL_FIFO_EMPTY |
		DSIM_INTMSK_SFR_PH_FIFO_EMPTY |
		DSIM_INTMSK_FRAME_DONE | DSIM_INTMSK_INVALID_SFR_VALUE | 
		DSIM_INTMSK_UNDER_RUN | DSIM_INTMSK_RX_DATA_DONE |
		DSIM_INTMSK_ERR_RX_ECC | DSIM_INTMSK_VT_STATUS;

	dsim_write_mask(id, DSIM_INTMSK, val, mask);
}

u32 dsim_reg_rx_fifo_is_empty(u32 id)
{
	return dsim_read_mask(id, DSIM_FIFOCTRL, DSIM_FIFOCTRL_EMPTY_RX);
}

u32 dsim_reg_get_rx_fifo(u32 id)
{
	return dsim_read(id, DSIM_RXFIFO);
}

int dsim_reg_rx_err_handler(u32 id, u32 rx_fifo)
{
	int ret = 0;
	u32 err_bit = rx_fifo >> 8; /* Error_Range [23:8] */

	if ((err_bit & MIPI_DSI_ERR_BIT_MASK) == 0) {
		dsim_dbg("dsim%d, Non error reporting format (rx_fifo=0x%x)\n",
				id, rx_fifo);
		return ret;
	}

	/* Parse error report bit*/
	if (err_bit & MIPI_DSI_ERR_SOT)
		dsim_err("SoT error!\n");
	if (err_bit & MIPI_DSI_ERR_SOT_SYNC)
		dsim_err("SoT sync error!\n");
	if (err_bit & MIPI_DSI_ERR_EOT_SYNC)
		dsim_err("EoT error!\n");
	if (err_bit & MIPI_DSI_ERR_ESCAPE_MODE_ENTRY_CMD)
		dsim_err("Escape mode entry command error!\n");
	if (err_bit & MIPI_DSI_ERR_LOW_POWER_TRANSMIT_SYNC)
		dsim_err("Low-power transmit sync error!\n");
	if (err_bit & MIPI_DSI_ERR_HS_RECEIVE_TIMEOUT)
		dsim_err("HS receive timeout error!\n");
	if (err_bit & MIPI_DSI_ERR_FALSE_CONTROL)
		dsim_err("False control error!\n");
	if (err_bit & MIPI_DSI_ERR_ECC_SINGLE_BIT)
		dsim_err("ECC error, single-bit(detected and corrected)!\n");
	if (err_bit & MIPI_DSI_ERR_ECC_MULTI_BIT)
		dsim_err("ECC error, multi-bit(detected, not corrected)!\n");
	if (err_bit & MIPI_DSI_ERR_CHECKSUM)
		dsim_err("Checksum error(long packet only)!\n");
	if (err_bit & MIPI_DSI_ERR_DATA_TYPE_NOT_RECOGNIZED)
		dsim_err("DSI data type not recognized!\n");
	if (err_bit & MIPI_DSI_ERR_VCHANNEL_ID_INVALID)
		dsim_err("DSI VC ID invalid!\n");
	if (err_bit & MIPI_DSI_ERR_INVALID_TRANSMIT_LENGTH)
		dsim_err("Invalid transmission length!\n");

	dsim_err("dsim%d, (rx_fifo=0x%x) Check DPHY values about HS clk.\n",
			id, rx_fifo);
	return -EINVAL;
}

/*
 * enter or exit ulps mode
 *
 * Parameter
 *	1 : enter ULPS mode
 *	0 : exit ULPS mode
 */
int dsim_reg_set_ulps(u32 id, u32 en, u32 lanes)
{
	int ret = 0;

	if (en) {
		/* Enable ULPS clock and data lane */
		dsim_reg_enter_ulps(id, 1);

		/* Check ULPS request for data lane */
		ret = dsim_reg_wait_enter_ulps_state(id, lanes);
		if (ret)
			return ret;

	} else {
		/* Exit ULPS clock and data lane */
		dsim_reg_exit_ulps(id, 1);

		ret = dsim_reg_wait_exit_ulps_state(id);
		if (ret)
			return ret;
		udelay(1000);   /* wait at least 1ms : Twakeup time for MARK1 state  */

		/* Clear ULPS exit request */
		dsim_reg_exit_ulps(id, 0);

		/* Clear ULPS enter request */
		dsim_reg_enter_ulps(id, 0);
	}

	return ret;
}

/*
 * enter or exit ulps mode for LSI DDI
 *
 * Parameter
 *	1 : enter ULPS mode
 *	0 : exit ULPS mode
 * assume that disp block power is off after ulps mode enter
 */
int dsim_reg_set_smddi_ulps(u32 id, u32 en, u32 lanes)
{
	int ret = 0;

	if (en) {
		/* Enable ULPS clock and data lane */
		dsim_reg_enter_ulps(id, 1);

		/* Check ULPS request for data lane */
		ret = dsim_reg_wait_enter_ulps_state(id, lanes);
		if (ret)
			return ret;
		/* Clear ULPS enter request */
		dsim_reg_enter_ulps(id, 0);
	} else {
		/* Enable ULPS clock and data lane */
		dsim_reg_enter_ulps(id, 1);

		/* Check ULPS request for data lane */
		ret = dsim_reg_wait_enter_ulps_state(id, lanes);
		if (ret)
			return ret;

		/* Exit ULPS clock and data lane */
		dsim_reg_exit_ulps(id, 1);

		ret = dsim_reg_wait_exit_ulps_state(id);
		if (ret)
			return ret;

		/* wait at least 1ms : Twakeup time for MARK1 state */
		udelay(1000);

		/* Clear ULPS exit request */
		dsim_reg_exit_ulps(id, 0);

		/* Clear ULPS enter request */
		dsim_reg_enter_ulps(id, 0);
	}

	return ret;
}

int dsim_reg_set_ulps_by_ddi(u32 id, u32 ddi_type, u32 lanes, u32 en)
{
	int ret;

	switch (ddi_type) {
	case TYPE_OF_SM_DDI:
		ret = dsim_reg_set_smddi_ulps(id, en, lanes);
		break;
	case TYPE_OF_MAGNA_DDI:
		dsim_err("This ddi(%d) doesn't support ULPS\n", ddi_type);
		ret = -EINVAL;
		break;
	case TYPE_OF_NORMAL_DDI:
	default:
		ret = dsim_reg_set_ulps(id, en, lanes);
		break;
	}

	if (ret < 0)
		dsim_err("%s: failed to %s ULPS", __func__,
				en ? "enter" : "exit");

	return ret;
}

/* Exit ULPS mode and set clocks and lanes */
int dsim_reg_exit_ulps_and_start(u32 id, u32 ddi_type, u32 lanes)
{
	int ret = 0;
/* just for test */
#if 0
	/*
	 * Guarantee 1.2v signal level for data lane(positive) when exit ULPS.
	 * DSIM Should be set standby. If not, lane goes to 600mv sometimes.
	*/
	dsim_reg_set_hs_clock(id, 1);
	dsim_reg_set_hs_clock(id, 0);
#endif
	/* try to exit ULPS mode. The sequence is depends on DDI type */
	ret = dsim_reg_set_ulps_by_ddi(id, ddi_type, lanes, 0);
	dsim_reg_start(id);
	return ret;
}

/* Unset clocks and lanes and enter ULPS mode */
int dsim_reg_stop_and_enter_ulps(u32 id, u32 ddi_type, u32 lanes)
{
	int ret = 0;
#if 0
	u32 cnt = 100;
	u32 val;

	val = dsim_read(id, DSIM_LINK_STATUS1);
	do {
		if (DSIM_LINK_STATUS1_CMD_MODE_STATUS_GET(val) == 0)
			break;
	} while (cnt--);
#endif

	dsim_reg_set_hs_clock(id, 0);
	ret = dsim_reg_set_hs_clock(id, 0);
	if (ret < 0) {
		dsim_err("The CLK lane doesn't be switched to LP mode\n");
	}
	dsim_reg_set_ulps_by_ddi(id, ddi_type, lanes, 1);

	dsim_reg_set_lanes(id, lanes, 0);
	dsim_reg_set_esc_clk_on_lane(id, 0, lanes);
	//dsim_reg_enable_word_clock(id, 0);
	dsim_reg_set_clocks(id, NULL, NULL, 0);
	dsim_reg_sw_reset(id);
	return ret;
}

/* Set clocks and lanes and HS ready */
void dsim_reg_start(u32 id)
{
	dsim_reg_set_hs_clock(id, 1);
	dsim_reg_set_int(id, 1);
}

/* Unset clocks and lanes and stop_state */
void dsim_reg_stop(u32 id, u32 lanes)
{
	dsim_reg_clear_int(id, 0xffffffff);
	/* disable interrupts */
	dsim_reg_set_int(id, 0);

	/* first disable HS clock */
	if (dsim_reg_set_hs_clock(id, 0) < 0) {
		dsim_err("The CLK lane doesn't be switched to LP mode\n");
	}
	dsim_reg_set_lanes(id, lanes, 0);
	dsim_reg_set_esc_clk_on_lane(id, 0, lanes);
	dsim_reg_enable_word_clock(id, 0);
	dsim_reg_set_clocks(id, NULL, NULL, 0);
	dsim_reg_sw_reset(id);
}
#ifdef CONFIG_SUPPORT_DSU
void dsim_set_dsu_update(u32 id, struct decon_lcd *lcd_info)
{
	u32 threshold;
	u32 num_of_slice;
	u32 num_of_transfer;

	threshold = lcd_info->xres;
	num_of_transfer = lcd_info->xres * lcd_info->yres / threshold;

	if (lcd_info->mode != DECON_MIPI_COMMAND_MODE)
		return;

	dsim_reg_set_cm_underrun_lp_ref(id, lcd_info->cmd_underrun_lp_ref);

	if (lcd_info->dsc_enabled) {
		threshold = lcd_info->xres / 3;
		num_of_transfer = lcd_info->xres * lcd_info->yres / threshold / 3;
	}
	dsim_reg_set_threshold(id, threshold);

	dsim_reg_set_vresol(id, lcd_info->yres);
	dsim_reg_set_hresol(id, lcd_info->xres, lcd_info);
	dsim_reg_set_porch(id, lcd_info);

	dsim_reg_set_num_of_transfer(id, num_of_transfer);
	
	dsim_reg_enable_dsc(id, lcd_info->dsc_enabled);

	if (lcd_info->dsc_enabled) {
		dsim_dbg("%s: dsc configuration is set\n", __func__);
		dsim_reg_set_num_of_slice(id, lcd_info->dsc_slice_num);
		dsim_reg_set_multi_slice(id, lcd_info); /* multi slice */
		dsim_reg_set_size_of_slice(id, lcd_info);

		dsim_reg_get_num_of_slice(id, &num_of_slice);
		dsim_dbg("dsim%d: number of DSC slice(%d)\n", id, num_of_slice);
		dsim_reg_print_size_of_slice(id);
	}
}
#endif
