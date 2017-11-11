/*
 * drivers/video/decon/panels/s6e3hf2_lcd_ctrl.c
 *
 * Samsung SoC MIPI LCD CONTROL functions
 *
 * Copyright (c) 2014 Samsung Electronics
 *
 * Jiun Yu, <jiun.yu@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <video/mipi_display.h>

#include "../dsim.h"

#include "panel_info.h"
#include "dsim_panel.h"

unsigned int s6e3hf4_lcd_type = S6E3HF4_LCDTYPE_WQHD;

#ifdef CONFIG_PANEL_AID_DIMMING
#include "aid_dimming.h"
#include "dimming_core.h"
#include "s6e3hf4_wqhd_aid_dimming.h"
#endif


#ifdef CONFIG_PANEL_AID_DIMMING
static const unsigned char *ACL_CUTOFF_TABLE[ACL_STATUS_MAX] = {SEQ_ACL_OFF, SEQ_ACL_8};
static const unsigned char *ACL_OPR_TABLE[ACL_OPR_MAX] = {SEQ_ACL_OFF_OPR_AVR, SEQ_ACL_ON_OPR_AVR};

static const unsigned int br_tbl_360 [256] = {
	2, 2, 2, 3,	4, 5, 6, 7,	8,	9,	10,	11,	12,	13,	14,	15,		// 16
	16,	17,	18,	19,	20,	21,	22,	23,	25,	27,	29,	31,	33,	36,   	// 14
	39,	41,	41,	44,	44,	47,	47,	50,	50,	53,	53,	56,	56,	56,		// 14
	60,	60,	60,	64,	64,	64,	68,	68,	68,	72,	72,	72,	72,	77,		// 14
	77,	77,	82,	82,	82,	82,	87,	87,	87,	87,	93,	93,	93,	93,		// 14
	98,	98,	98,	98,	98,	105, 105, 105, 105,	111, 111, 111,		// 12
	111, 111, 111, 119, 119, 119, 119, 119, 126, 126, 126,		// 11
	126, 126, 126, 134, 134, 134, 134, 134,	134, 134, 143,
	143, 143, 143, 143, 143, 152, 152, 152, 152, 152, 152,
	152, 152, 162, 162,	162, 162, 162, 162,	162, 172, 172,
	172, 172, 172, 172,	172, 172, 183, 183, 183, 183, 183,
	183, 183, 183, 183, 195, 195, 195, 195, 195, 195, 195,
	195, 207, 207, 207, 207, 207, 207, 207, 207, 207, 207,
	220, 220, 220, 220, 220, 220, 220, 220, 220, 220, 234,
	234, 234, 234, 234,	234, 234, 234, 234,	234, 234, 249,
	249, 249, 249, 249, 249, 249, 249, 249, 249, 249, 249,
	265, 265, 265, 265, 265, 265, 265, 265, 265, 265, 265,
	265, 282, 282, 282, 282, 282, 282, 282, 282, 282, 282,
	282, 282, 282, 300, 300, 300, 300, 300,	300, 300, 300,
	300, 300, 300, 300, 316, 316, 316, 316, 316, 316, 316,
	316, 316, 316, 316, 316, 333, 333, 333, 333, 333, 333,
	333, 333, 333, 333, 333, 333, 350							//7
};


static const unsigned int br_tbl_420 [256] = {
	2,	3,	4,	4,	5,	6,	7,	8,	9,	10,  11,  12,  13,	14,  15,  16,
	17,  19,  19,  20,	21,  22,  24,  25,	27,  27,  29,  30,	32,  32,  34,  34,
	37,  37,  41,  41,	41,  44,  44,  47,	47,  47,  50,  50,	53,  53,  56,  56,
	60,  60,  60,  64,	64,  64,  68,  68,	68,  72,  72,  77,	77,  77,  77,  82,
	82,  82,  87,  87,	87,  87,  93,  93,	93,  93,  98,  98,	98,  105,  105,  105,
	105,  111,	111,  111,	111,  119,	119,  119,	119,  119,	126,  126,	126,  126,	126,  134,
	134,  134,	134,  134,	143,  143,	143,  143,	143,  152,	152,  152,	152,  152,	152,  162,
	162,  162,	162,  162,	162,  172,	172,  172,	172,  172,	183,  183,	183,  183,	183,  183,
	183,  195,	195,  195,	195,  195,	195,  195,	207,  207,	207,  207,	207,  207,	207,  220,
	220,  220,	220,  220,	220,  220,	234,  234,	234,  234,	234,  234,	234,  234,	249,  249,
	249,  249,	249,  249,	249,  249,	265,  265,	265,  265,	265,  265,	265,  265,	265,  282,
	282,  282,	282,  282,	282,  282,	282,  282,	300,  300,	300,  300,	300,  300,	300,  300,
	300,  300,	316,  316,	316,  316,	316,  316,	316,  316,	333,  333,	333,  333,	333,  333,
	333,  333,	333,  350,	350,  350,	350,  350,	350,  350,	350,  350,	357,  357,	357,  357,
	365,  365,	365,  365,	372,  372,	372,  380,	380,  380,	380,  387,	387,  387,	387,  395,
	395,  395,	395,  403,	403,  403,	403,  412,	412,  412,	412,  420,	420,  420,	420,  420,
};

static unsigned char inter_aor_tbl[512] = {
	0x90, 0xE3,  0x90, 0xCD,  0x90, 0xBF,  0x90, 0xB0,	0x90, 0x93,  0x90, 0x7D,  0x90, 0x61,  0x90, 0x4D,
	0x90, 0x31,  0x90, 0x1D,  0x90, 0x01,  0x80, 0xED,	0x80, 0xD1,  0x80, 0xBD,  0x80, 0xA1,  0x80, 0x8D,
	0x80, 0x6F,  0x80, 0x57,  0x80, 0x3F,  0x80, 0x23,	0x80, 0x0C,  0x70, 0xEF,  0x70, 0xBC,  0x70, 0xA1,
	0x70, 0x87,  0x70, 0x6D,  0x70, 0x33,  0x70, 0x1E,	0x70, 0x01,  0x60, 0xE3,  0x60, 0xCA,  0x60, 0xB1,
	0x60, 0x8A,  0x60, 0x63,  0x60, 0x2D,  0x60, 0x10,	0x50, 0xF4,  0x50, 0xCB,  0x50, 0xA1,  0x50, 0x86,
	0x50, 0x6B,  0x50, 0x50,  0x50, 0x28,  0x50, 0x01,	0x40, 0xD8,  0x40, 0xAE,  0x40, 0x85,  0x40, 0x5C,
	0x40, 0x36,  0x40, 0x0F,  0x30, 0xE9,  0x30, 0xC7,	0x30, 0xA6,  0x30, 0x84,  0x30, 0xC6,  0x30, 0xA5,
	0x30, 0x84,  0x30, 0xB3,  0x30, 0x84,  0x30, 0xD6,	0x30, 0xBB,  0x30, 0x9F,  0x30, 0x84,  0x30, 0xC8,
	0x30, 0xA6,  0x30, 0x84,  0x30, 0xCD,  0x30, 0xB4,	0x30, 0x9C,  0x30, 0x84,  0x30, 0xD5,  0x30, 0xBA,
	0x30, 0x9F,  0x30, 0x84,  0x30, 0xBD,  0x30, 0xA1,	0x30, 0x84,  0x30, 0xD8,  0x30, 0xBC,  0x30, 0xA0,
	0x30, 0x84,  0x30, 0xC8,  0x30, 0xB2,  0x30, 0x9B,	0x30, 0x84,  0x30, 0xDF,  0x30, 0xC8,  0x30, 0xB1,
	0x30, 0x9B,  0x30, 0x84,  0x30, 0xCF,  0x30, 0xBC,	0x30, 0xA9,  0x30, 0x97,  0x30, 0x84,  0x30, 0xD4,
	0x30, 0xC0,  0x30, 0xAC,  0x30, 0x98,  0x30, 0x84,	0x30, 0xD9,  0x30, 0xC4,  0x30, 0xAE,  0x30, 0x99,
	0x30, 0x84,  0x30, 0xD7,  0x30, 0xC6,  0x30, 0xB6,	0x30, 0xA5,  0x30, 0x95,  0x30, 0x84,  0x30, 0xDB,
	0x30, 0xC9,  0x30, 0xB8,  0x30, 0xA7,  0x30, 0x95,	0x30, 0x84,  0x30, 0xD2,  0x30, 0xBF,  0x30, 0xAB,
	0x30, 0x98,  0x30, 0x84,  0x30, 0xDB,  0x30, 0xCC,	0x30, 0xBE,  0x30, 0xAF,  0x30, 0xA1,  0x30, 0x92,
	0x30, 0x84,  0x30, 0x74,  0x30, 0x64,  0x30, 0x54,	0x30, 0x44,  0x30, 0x34,  0x30, 0x24,  0x30, 0x14,
	0x30, 0x04,  0x20, 0xF4,  0x20, 0xE4,  0x20, 0xD4,	0x20, 0xC3,  0x20, 0xB3,  0x20, 0xA3,  0x20, 0x8E,
	0x20, 0x7A,  0x20, 0x65,  0x20, 0x51,  0x20, 0x3C,	0x20, 0x28,  0x20, 0x13,  0x20, 0x02,  0x10, 0xF1,
	0x10, 0xE0,  0x10, 0xCF,  0x10, 0xBE,  0x10, 0xAD,	0x10, 0x9C,  0x10, 0x8B,  0x10, 0xFE,  0x10, 0xEE,
	0x10, 0xDE,  0x10, 0xCD,  0x10, 0xBD,  0x10, 0xAC,	0x10, 0x9C,  0x10, 0x03,  0x10, 0x80,  0x10, 0x70,
	0x10, 0x61,  0x10, 0x51,  0x10, 0x41,  0x10, 0x32,	0x10, 0x22,  0x10, 0x13,  0x10, 0x03,  0x10, 0x80,
	0x10, 0x70,  0x10, 0x60,  0x10, 0x51,  0x10, 0x41,	0x10, 0x32,  0x10, 0x22,  0x10, 0x12,  0x10, 0x03,
	0x10, 0x80,  0x10, 0x73,  0x10, 0x65,  0x10, 0x57,	0x10, 0x49,  0x10, 0x3B,  0x10, 0x2D,  0x10, 0x1F,
	0x10, 0x11,  0x10, 0x03,  0x10, 0x6A,  0x10, 0x5B,	0x10, 0x4C,  0x10, 0x3E,  0x10, 0x2F,  0x10, 0x20,
	0x10, 0x12,  0x10, 0x03,  0x10, 0x6C,  0x10, 0x5F,	0x10, 0x52,  0x10, 0x45,  0x10, 0x38,  0x10, 0x2A,
	0x10, 0x1D,  0x10, 0x10,  0x10, 0x03,  0x10, 0x67,	0x10, 0x5B,  0x10, 0x4E,  0x10, 0x42,  0x10, 0x35,
	0x10, 0x29,  0x10, 0x1C,  0x10, 0x0F,  0x10, 0x03,	0x10, 0x25,  0x10, 0x1A,  0x10, 0x0E,  0x10, 0x03,
	0x10, 0x29,  0x10, 0x1C,  0x10, 0x10,  0x10, 0x03,	0x00, 0xEE,  0x00, 0xD8,  0x00, 0xC3,  0x00, 0xB3,
	0x00, 0xA3,  0x00, 0x93,  0x00, 0x83,  0x00, 0x7C,	0x00, 0x75,  0x00, 0x6E,  0x00, 0x67,  0x00, 0x59,
	0x00, 0x4A,  0x00, 0x3B,  0x00, 0x2D,  0x00, 0x53,	0x00, 0x46,  0x00, 0x3A,  0x00, 0x0C,  0x00, 0x36,
	0x00, 0x28,  0x00, 0x1A,  0x00, 0x0C,  0x00, 0x31,	0x00, 0x24,  0x00, 0x18,  0x00, 0x0C,  0x00, 0x0C,
};


static const short center_gamma[NUM_VREF][CI_MAX] = {
	{0x080, 0x080, 0x080},
	{0x080, 0x080, 0x080},
	{0x080, 0x080, 0x080},
	{0x080, 0x080, 0x080},
	{0x080, 0x080, 0x080},
	{0x080, 0x080, 0x080},
	{0x080, 0x080, 0x080},
	{0x080, 0x080, 0x080},
	{0x080, 0x080, 0x080},
	{0x100, 0x100, 0x100},
};


struct SmtDimInfo dimming_info_HF4[MAX_BR_INFO] = {
	{ .br = 2, .refBr = 116, .cGma = gma2p15, .rTbl = rtbl2nit, .cTbl = ctbl2nit, .aid = aid9795, .elvCaps = elvCaps1, .elv = elv1, .way = W1, .elvss_offset = elvss_offset1 },
	{ .br = 3, .refBr = 116, .cGma = gma2p15, .rTbl = rtbl3nit, .cTbl = ctbl3nit, .aid = aid9710, .elvCaps = elvCaps1, .elv = elv1, .way = W1, .elvss_offset = elvss_offset2 },
	{ .br = 4, .refBr = 116, .cGma = gma2p15, .rTbl = rtbl4nit, .cTbl = ctbl4nit, .aid = aid9598, .elvCaps = elvCaps1, .elv = elv1, .way = W1, .elvss_offset = elvss_offset3 },
	{ .br = 5, .refBr = 116, .cGma = gma2p15, .rTbl = rtbl5nit, .cTbl = ctbl5nit, .aid = aid9485, .elvCaps = elvCaps1, .elv = elv1, .way = W1, .elvss_offset = elvss_offset4 },
	{ .br = 6, .refBr = 116, .cGma = gma2p15, .rTbl = rtbl6nit, .cTbl = ctbl6nit, .aid = aid9400, .elvCaps = elvCaps1, .elv = elv1, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 7, .refBr = 116, .cGma = gma2p15, .rTbl = rtbl7nit, .cTbl = ctbl7nit, .aid = aid9292, .elvCaps = elvCaps1, .elv = elv1, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 8, .refBr = 116, .cGma = gma2p15, .rTbl = rtbl8nit, .cTbl = ctbl8nit, .aid = aid9214, .elvCaps = elvCaps1, .elv = elv1, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 9, .refBr = 116, .cGma = gma2p15, .rTbl = rtbl9nit, .cTbl = ctbl9nit, .aid = aid9106, .elvCaps = elvCaps1, .elv = elv1, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 10, .refBr = 116, .cGma = gma2p15, .rTbl = rtbl10nit, .cTbl = ctbl10nit, .aid = aid9029, .elvCaps = elvCaps1, .elv = elv1, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 11, .refBr = 116, .cGma = gma2p15, .rTbl = rtbl11nit, .cTbl = ctbl11nit, .aid = aid8920, .elvCaps = elvCaps1, .elv = elv1, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 12, .refBr = 116, .cGma = gma2p15, .rTbl = rtbl12nit, .cTbl = ctbl12nit, .aid = aid8843, .elvCaps = elvCaps1, .elv = elv1, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 13, .refBr = 116, .cGma = gma2p15, .rTbl = rtbl13nit, .cTbl = ctbl13nit, .aid = aid8735, .elvCaps = elvCaps1, .elv = elv1, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 14, .refBr = 116, .cGma = gma2p15, .rTbl = rtbl14nit, .cTbl = ctbl14nit, .aid = aid8657, .elvCaps = elvCaps1, .elv = elv1, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 15, .refBr = 116, .cGma = gma2p15, .rTbl = rtbl15nit, .cTbl = ctbl15nit, .aid = aid8549, .elvCaps = elvCaps1, .elv = elv1, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 16, .refBr = 116, .cGma = gma2p15, .rTbl = rtbl16nit, .cTbl = ctbl16nit, .aid = aid8471, .elvCaps = elvCaps1, .elv = elv1, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 17, .refBr = 116, .cGma = gma2p15, .rTbl = rtbl17nit, .cTbl = ctbl17nit, .aid = aid8355, .elvCaps = elvCaps1, .elv = elv1, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 19, .refBr = 116, .cGma = gma2p15, .rTbl = rtbl19nit, .cTbl = ctbl19nit, .aid = aid8170, .elvCaps = elvCaps1, .elv = elv1, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 20, .refBr = 116, .cGma = gma2p15, .rTbl = rtbl20nit, .cTbl = ctbl20nit, .aid = aid8061, .elvCaps = elvCaps1, .elv = elv1, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 21, .refBr = 116, .cGma = gma2p15, .rTbl = rtbl21nit, .cTbl = ctbl21nit, .aid = aid7972, .elvCaps = elvCaps1, .elv = elv1, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 22, .refBr = 116, .cGma = gma2p15, .rTbl = rtbl22nit, .cTbl = ctbl22nit, .aid = aid7860, .elvCaps = elvCaps1, .elv = elv1, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 24, .refBr = 116, .cGma = gma2p15, .rTbl = rtbl24nit, .cTbl = ctbl24nit, .aid = aid7663, .elvCaps = elvCaps1, .elv = elv1, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 25, .refBr = 116, .cGma = gma2p15, .rTbl = rtbl25nit, .cTbl = ctbl25nit, .aid = aid7558, .elvCaps = elvCaps1, .elv = elv1, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 27, .refBr = 116, .cGma = gma2p15, .rTbl = rtbl27nit, .cTbl = ctbl27nit, .aid = aid7357, .elvCaps = elvCaps1, .elv = elv1, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 29, .refBr = 116, .cGma = gma2p15, .rTbl = rtbl29nit, .cTbl = ctbl29nit, .aid = aid7132, .elvCaps = elvCaps1, .elv = elv1, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 30, .refBr = 116, .cGma = gma2p15, .rTbl = rtbl30nit, .cTbl = ctbl30nit, .aid = aid7051, .elvCaps = elvCaps1, .elv = elv1, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 32, .refBr = 116, .cGma = gma2p15, .rTbl = rtbl32nit, .cTbl = ctbl32nit, .aid = aid6823, .elvCaps = elvCaps1, .elv = elv1, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 34, .refBr = 116, .cGma = gma2p15, .rTbl = rtbl34nit, .cTbl = ctbl34nit, .aid = aid6629, .elvCaps = elvCaps1, .elv = elv1, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 37, .refBr = 116, .cGma = gma2p15, .rTbl = rtbl37nit, .cTbl = ctbl37nit, .aid = aid6327, .elvCaps = elvCaps1, .elv = elv1, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 39, .refBr = 116, .cGma = gma2p15, .rTbl = rtbl39nit, .cTbl = ctbl39nit, .aid = aid6118, .elvCaps = elvCaps1, .elv = elv1, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 41, .refBr = 116, .cGma = gma2p15, .rTbl = rtbl41nit, .cTbl = ctbl41nit, .aid = aid5898, .elvCaps = elvCaps1, .elv = elv1, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 44, .refBr = 116, .cGma = gma2p15, .rTbl = rtbl44nit, .cTbl = ctbl44nit, .aid = aid5577, .elvCaps = elvCaps1, .elv = elv1, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 47, .refBr = 116, .cGma = gma2p15, .rTbl = rtbl47nit, .cTbl = ctbl47nit, .aid = aid5263, .elvCaps = elvCaps1, .elv = elv1, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 50, .refBr = 116, .cGma = gma2p15, .rTbl = rtbl50nit, .cTbl = ctbl50nit, .aid = aid4957, .elvCaps = elvCaps1, .elv = elv1, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 53, .refBr = 116, .cGma = gma2p15, .rTbl = rtbl53nit, .cTbl = ctbl53nit, .aid = aid4636, .elvCaps = elvCaps1, .elv = elv1, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 56, .refBr = 116, .cGma = gma2p15, .rTbl = rtbl56nit, .cTbl = ctbl56nit, .aid = aid4319, .elvCaps = elvCaps1, .elv = elv1, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 60, .refBr = 116, .cGma = gma2p15, .rTbl = rtbl60nit, .cTbl = ctbl60nit, .aid = aid3874, .elvCaps = elvCaps1, .elv = elv1, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 64, .refBr = 116, .cGma = gma2p15, .rTbl = rtbl64nit, .cTbl = ctbl64nit, .aid = aid3483, .elvCaps = elvCaps1, .elv = elv1, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 68, .refBr = 121, .cGma = gma2p15, .rTbl = rtbl68nit, .cTbl = ctbl68nit, .aid = aid3483, .elvCaps = elvCaps1, .elv = elv1, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 72, .refBr = 127, .cGma = gma2p15, .rTbl = rtbl72nit, .cTbl = ctbl72nit, .aid = aid3483, .elvCaps = elvCaps1, .elv = elv1, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 77, .refBr = 137, .cGma = gma2p15, .rTbl = rtbl77nit, .cTbl = ctbl77nit, .aid = aid3483, .elvCaps = elvCaps1, .elv = elv1, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 82, .refBr = 144, .cGma = gma2p15, .rTbl = rtbl82nit, .cTbl = ctbl82nit, .aid = aid3483, .elvCaps = elvCaps1, .elv = elv1, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 87, .refBr = 153, .cGma = gma2p15, .rTbl = rtbl87nit, .cTbl = ctbl87nit, .aid = aid3483, .elvCaps = elvCaps1, .elv = elv1, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 93, .refBr = 161, .cGma = gma2p15, .rTbl = rtbl93nit, .cTbl = ctbl93nit, .aid = aid3483, .elvCaps = elvCaps1, .elv = elv1, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 98, .refBr = 168, .cGma = gma2p15, .rTbl = rtbl98nit, .cTbl = ctbl98nit, .aid = aid3483, .elvCaps = elvCaps1, .elv = elv1, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 105, .refBr = 179, .cGma = gma2p15, .rTbl = rtbl105nit, .cTbl = ctbl105nit, .aid = aid3483, .elvCaps = elvCaps1, .elv = elv1, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 111, .refBr = 188, .cGma = gma2p15, .rTbl = rtbl111nit, .cTbl = ctbl111nit, .aid = aid3483, .elvCaps = elvCaps2, .elv = elv2, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 119, .refBr = 200, .cGma = gma2p15, .rTbl = rtbl119nit, .cTbl = ctbl119nit, .aid = aid3483, .elvCaps = elvCaps2, .elv = elv2, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 126, .refBr = 210, .cGma = gma2p15, .rTbl = rtbl126nit, .cTbl = ctbl126nit, .aid = aid3483, .elvCaps = elvCaps2, .elv = elv2, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 134, .refBr = 217, .cGma = gma2p15, .rTbl = rtbl134nit, .cTbl = ctbl134nit, .aid = aid3483, .elvCaps = elvCaps2, .elv = elv2, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 143, .refBr = 230, .cGma = gma2p15, .rTbl = rtbl143nit, .cTbl = ctbl143nit, .aid = aid3483, .elvCaps = elvCaps2, .elv = elv2, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 152, .refBr = 243, .cGma = gma2p15, .rTbl = rtbl152nit, .cTbl = ctbl152nit, .aid = aid3483, .elvCaps = elvCaps2, .elv = elv2, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 162, .refBr = 257, .cGma = gma2p15, .rTbl = rtbl162nit, .cTbl = ctbl162nit, .aid = aid3483, .elvCaps = elvCaps2, .elv = elv2, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 172, .refBr = 271, .cGma = gma2p15, .rTbl = rtbl172nit, .cTbl = ctbl172nit, .aid = aid3483, .elvCaps = elvCaps2, .elv = elv2, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 183, .refBr = 289, .cGma = gma2p15, .rTbl = rtbl183nit, .cTbl = ctbl183nit, .aid = aid3483, .elvCaps = elvCaps2, .elv = elv2, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 195, .refBr = 289, .cGma = gma2p15, .rTbl = rtbl195nit, .cTbl = ctbl195nit, .aid = aid3050, .elvCaps = elvCaps2, .elv = elv2, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 207, .refBr = 289, .cGma = gma2p15, .rTbl = rtbl207nit, .cTbl = ctbl207nit, .aid = aid2612, .elvCaps = elvCaps3, .elv = elv3, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 220, .refBr = 289, .cGma = gma2p15, .rTbl = rtbl220nit, .cTbl = ctbl220nit, .aid = aid2055, .elvCaps = elvCaps3, .elv = elv3, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 234, .refBr = 289, .cGma = gma2p15, .rTbl = rtbl234nit, .cTbl = ctbl234nit, .aid = aid1529, .elvCaps = elvCaps4, .elv = elv4, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 249, .refBr = 292, .cGma = gma2p15, .rTbl = rtbl249nit, .cTbl = ctbl249nit, .aid = aid1002, .elvCaps = elvCaps4, .elv = elv4, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 265, .refBr = 307, .cGma = gma2p15, .rTbl = rtbl265nit, .cTbl = ctbl265nit, .aid = aid1002, .elvCaps = elvCaps5, .elv = elv5, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 282, .refBr = 322, .cGma = gma2p15, .rTbl = rtbl282nit, .cTbl = ctbl282nit, .aid = aid1002, .elvCaps = elvCaps6, .elv = elv6, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 300, .refBr = 344, .cGma = gma2p15, .rTbl = rtbl300nit, .cTbl = ctbl300nit, .aid = aid1002, .elvCaps = elvCaps6, .elv = elv6, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 316, .refBr = 361, .cGma = gma2p15, .rTbl = rtbl316nit, .cTbl = ctbl316nit, .aid = aid1002, .elvCaps = elvCaps7, .elv = elv7, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 333, .refBr = 374, .cGma = gma2p15, .rTbl = rtbl333nit, .cTbl = ctbl333nit, .aid = aid1002, .elvCaps = elvCaps8, .elv = elv8, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 350, .refBr = 395, .cGma = gma2p15, .rTbl = rtbl350nit, .cTbl = ctbl350nit, .aid = aid1002, .elvCaps = elvCaps8, .elv = elv8, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 357, .refBr = 399, .cGma = gma2p15, .rTbl = rtbl357nit, .cTbl = ctbl357nit, .aid = aid1002, .elvCaps = elvCaps9, .elv = elv9, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 365, .refBr = 406, .cGma = gma2p15, .rTbl = rtbl365nit, .cTbl = ctbl365nit, .aid = aid1002, .elvCaps = elvCaps9, .elv = elv9, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 372, .refBr = 406, .cGma = gma2p15, .rTbl = rtbl372nit, .cTbl = ctbl372nit, .aid = aid755, .elvCaps = elvCaps9, .elv = elv9, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 380, .refBr = 406, .cGma = gma2p15, .rTbl = rtbl380nit, .cTbl = ctbl380nit, .aid = aid507, .elvCaps = elvCaps9, .elv = elv9, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 387, .refBr = 406, .cGma = gma2p15, .rTbl = rtbl387nit, .cTbl = ctbl387nit, .aid = aid399, .elvCaps = elvCaps10, .elv = elv10, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 395, .refBr = 406, .cGma = gma2p15, .rTbl = rtbl395nit, .cTbl = ctbl395nit, .aid = aid174, .elvCaps = elvCaps10, .elv = elv10, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 403, .refBr = 409, .cGma = gma2p15, .rTbl = rtbl403nit, .cTbl = ctbl403nit, .aid = aid46, .elvCaps = elvCaps10, .elv = elv10, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 412, .refBr = 416, .cGma = gma2p15, .rTbl = rtbl412nit, .cTbl = ctbl412nit, .aid = aid46, .elvCaps = elvCaps11, .elv = elv11, .way = W1, .elvss_offset = elvss_offset5 },
	{ .br = 420, .refBr = 420, .cGma = gma2p20, .rTbl = rtbl420nit, .cTbl = ctbl420nit, .aid = aid46, .elvCaps = elvCaps11, .elv = elv11, .way = W2, .elvss_offset = elvss_offset5 },
/*hbm interpolation */
	{ .br = 443, .refBr = 443, .cGma = gma2p20, .rTbl = rtbl420nit, .cTbl = ctbl420nit, .aid = aid46, .elvCaps = elvCaps11, .elv = elv11, .way = W3, .elvss_offset = elvss_offset5},
	{ .br = 465, .refBr = 465, .cGma = gma2p20, .rTbl = rtbl420nit, .cTbl = ctbl420nit, .aid = aid46, .elvCaps = elvCaps11, .elv = elv11, .way = W3, .elvss_offset = elvss_offset5},
	{ .br = 488, .refBr = 488, .cGma = gma2p20, .rTbl = rtbl420nit, .cTbl = ctbl420nit, .aid = aid46, .elvCaps = elvCaps11, .elv = elv11, .way = W3, .elvss_offset = elvss_offset5},
	{ .br = 510, .refBr = 510, .cGma = gma2p20, .rTbl = rtbl420nit, .cTbl = ctbl420nit, .aid = aid46, .elvCaps = elvCaps11, .elv = elv11, .way = W3, .elvss_offset = elvss_offset5},
	{ .br = 533, .refBr = 533, .cGma = gma2p20, .rTbl = rtbl420nit, .cTbl = ctbl420nit, .aid = aid46, .elvCaps = elvCaps11, .elv = elv11, .way = W3, .elvss_offset = elvss_offset5},
	{ .br = 555, .refBr = 555, .cGma = gma2p20, .rTbl = rtbl420nit, .cTbl = ctbl420nit, .aid = aid46, .elvCaps = elvCaps11, .elv = elv11, .way = W3, .elvss_offset = elvss_offset5},
	{ .br = 578, .refBr = 578, .cGma = gma2p20, .rTbl = rtbl420nit, .cTbl = ctbl420nit, .aid = aid46, .elvCaps = elvCaps11, .elv = elv11, .way = W3, .elvss_offset = elvss_offset5},
/* hbm */
	{ .br = 600, .refBr = 600, .cGma = gma2p20, .rTbl = rtbl420nit, .cTbl = ctbl420nit, .aid = aid46, .elvCaps = elvCaps12, .elv = elv12, .way = W4, .elvss_offset = elvss_offset6},
};

static int set_gamma_to_center(struct SmtDimInfo *brInfo)
{
	int i, j;
	int ret = 0;
	unsigned int index = 0;
	unsigned char *result = brInfo->gamma;

	result[index++] = OLED_CMD_GAMMA;

	for (i = V255; i >= V1; i--) {
		for (j = 0; j < CI_MAX; j++) {
			if (i == V255) {
				result[index++] = (unsigned char)((center_gamma[i][j] >> 8) & 0x01);
				result[index++] = (unsigned char)center_gamma[i][j] & 0xff;
			} else {
				result[index++] = (unsigned char)center_gamma[i][j] & 0xff;
			}
		}
	}
	result[index++] = 0x00;
	result[index++] = 0x00;

	return ret;
}

static int set_gamma_to_hbm(struct SmtDimInfo *brInfo, struct dim_data *dimData, u8 *hbm)
{
	int	ret = 0;
	unsigned int index = 0;
	unsigned char *result = brInfo->gamma;
	int i = 0;
	memset(result, 0, OLED_CMD_GAMMA_CNT);

	result[index++] = OLED_CMD_GAMMA;

	while(index < OLED_CMD_GAMMA_CNT) {
		if(((i >= 50) && (i < 65)) || ((i >= 68) && (i < 88)))
			result[index++] = hbm[i];
		i++;
	}
	for(i = 0; i < OLED_CMD_GAMMA_CNT; i++)
		dsim_info("%d : %d\n", i + 1, result[i]);
	return ret;
}
/* gamma interpolaion table */
const unsigned int tbl_hbm_inter[7] = {
	131, 256, 387, 512, 643, 768, 899
};


static int interpolation_gamma_to_hbm(struct SmtDimInfo *dimInfo, int br_idx)
{
	int i, j;
	int ret = 0;
	int idx = 0;
	int tmp = 0;
	int hbmcnt, refcnt, gap = 0;
	int ref_idx = 0;
	int hbm_idx = 0;
	int rst = 0;
	int hbm_tmp, ref_tmp;
	unsigned char *result = dimInfo[br_idx].gamma;

	for (i = 0; i < MAX_BR_INFO; i++) {
		if (dimInfo[i].br == S6E3HF4_MAX_BRIGHTNESS)
			ref_idx = i;

		if (dimInfo[i].br == S6E3HF4_HBM_BRIGHTNESS)
			hbm_idx = i;
	}

	if ((ref_idx == 0) || (hbm_idx == 0)) {
		dsim_info("%s failed to get index ref index : %d, hbm index : %d\n",
					__func__, ref_idx, hbm_idx);
		ret = -EINVAL;
		goto exit;
	}

	result[idx++] = OLED_CMD_GAMMA;
	tmp = (br_idx - ref_idx) - 1;

	hbmcnt = 1;
	refcnt = 1;

	for (i = V255; i >= V1; i--) {
		for (j = 0; j < CI_MAX; j++) {
			if (i == V255) {
				hbm_tmp = (dimInfo[hbm_idx].gamma[hbmcnt] << 8) | (dimInfo[hbm_idx].gamma[hbmcnt+1]);
				ref_tmp = (dimInfo[ref_idx].gamma[refcnt] << 8) | (dimInfo[ref_idx].gamma[refcnt+1]);

				if (hbm_tmp > ref_tmp) {
					gap = hbm_tmp - ref_tmp;
					rst = (gap * tbl_hbm_inter[tmp]) >> 10;
					rst += ref_tmp;
				}
				else {
					gap = ref_tmp - hbm_tmp;
					rst = (gap * tbl_hbm_inter[tmp]) >> 10;
					rst = ref_tmp - rst;
				}
				result[idx++] = (unsigned char)((rst >> 8) & 0x01);
				result[idx++] = (unsigned char)rst & 0xff;
				hbmcnt += 2;
				refcnt += 2;
			} else {
				hbm_tmp = dimInfo[hbm_idx].gamma[hbmcnt++];
				ref_tmp = dimInfo[ref_idx].gamma[refcnt++];

				if (hbm_tmp > ref_tmp) {
					gap = hbm_tmp - ref_tmp;
					rst = (gap * tbl_hbm_inter[tmp]) >> 10;
					rst += ref_tmp;
				}
				else {
					gap = ref_tmp - hbm_tmp;
					rst = (gap * tbl_hbm_inter[tmp]) >> 10;
					rst = ref_tmp - rst;
				}
				result[idx++] = (unsigned char)rst & 0xff;
			}
		}
	}

	dsim_info("tmp index : %d\n", tmp);

exit:
	return ret;
}


static int init_dimming(struct dsim_device *dsim, u8 *mtp, u8 *hbm)
{
	int i, j;
	int pos = 0;
	int ret = 0;
	short temp;
	int method = 0;
	struct dim_data *dimming;
	struct panel_private *panel = &dsim->priv;
	struct SmtDimInfo *diminfo = NULL;
	int     string_offset;
	char    string_buf[1024];

	dimming = (struct dim_data *)kmalloc(sizeof(struct dim_data), GFP_KERNEL);
	if (!dimming) {
		dsim_err("failed to allocate memory for dim data\n");
		ret = -ENOMEM;
		goto error;
	}
	diminfo = (void *)dimming_info_HF4;


	panel->dim_data= (void *)dimming;
	panel->dim_info = (void *)diminfo;
	panel->inter_aor_tbl = NULL;


	if((panel->panel_rev < 2) && (panel->current_model == MODEL_IS_HERO)) {
		panel->br_tbl = (unsigned int *)br_tbl_360;
	}
	else {
		panel->br_tbl = (unsigned int *)br_tbl_420;
		panel->inter_aor_tbl = (unsigned char *)inter_aor_tbl;
	}
	panel->hbm_tbl = NULL;
	panel->acl_cutoff_tbl = (unsigned char **)ACL_CUTOFF_TABLE;
	panel->acl_opr_tbl = (unsigned char **)ACL_OPR_TABLE;
	panel->hbm_index = MAX_BR_INFO - 1;

	for (j = 0; j < CI_MAX; j++) {
		temp = ((mtp[pos] & 0x01) ? -1 : 1) * mtp[pos+1];
		dimming->t_gamma[V255][j] = (int)center_gamma[V255][j] + temp;
		dimming->mtp[V255][j] = temp;
		pos += 2;
	}

	for (i = V203; i >= V1; i--) {
		for (j = 0; j < CI_MAX; j++) {
			temp = ((mtp[pos] & 0x80) ? -1 : 1) * (mtp[pos] & 0x7f);
			dimming->t_gamma[i][j] = (int)center_gamma[i][j] + temp;
			dimming->mtp[i][j] = temp;
			pos++;
		}
	}
	/* for vt */
	temp = (mtp[pos+1]) << 8 | mtp[pos];

	for(i=0;i<CI_MAX;i++)
		dimming->vt_mtp[i] = (temp >> (i*4)) &0x0f;
#ifdef SMART_DIMMING_DEBUG
	dimm_info("Center Gamma Info : \n");
	for(i = 0; i < VMAX; i++) {
		dsim_info("Gamma : %3d %3d %3d : %3x %3x %3x\n",
			dimming->t_gamma[i][CI_RED], dimming->t_gamma[i][CI_GREEN], dimming->t_gamma[i][CI_BLUE],
			dimming->t_gamma[i][CI_RED], dimming->t_gamma[i][CI_GREEN], dimming->t_gamma[i][CI_BLUE] );
	}
#endif
	dimm_info("VT MTP : \n");
	dimm_info("Gamma : %3d %3d %3d : %3x %3x %3x\n",
			dimming->vt_mtp[CI_RED], dimming->vt_mtp[CI_GREEN], dimming->vt_mtp[CI_BLUE],
			dimming->vt_mtp[CI_RED], dimming->vt_mtp[CI_GREEN], dimming->vt_mtp[CI_BLUE] );

	dimm_info("MTP Info : \n");
	for(i = 0; i < VMAX; i++) {
		dimm_info("Gamma : %3d %3d %3d : %3x %3x %3x\n",
			dimming->mtp[i][CI_RED], dimming->mtp[i][CI_GREEN], dimming->mtp[i][CI_BLUE],
			dimming->mtp[i][CI_RED], dimming->mtp[i][CI_GREEN], dimming->mtp[i][CI_BLUE] );
	}

	ret = generate_volt_table(dimming);
	if (ret) {
		dimm_err("[ERR:%s] failed to generate volt table\n", __func__);
		goto error;
	}

	for (i = 0; i < MAX_BR_INFO; i++) {
		method = diminfo[i].way;

		if (method == DIMMING_METHOD_FILL_CENTER) {
			ret = set_gamma_to_center(&diminfo[i]);
			if (ret) {
				dsim_err("%s : failed to get center gamma\n", __func__);
				goto error;
			}
		}
		else if (method == DIMMING_METHOD_FILL_HBM) {
			ret = set_gamma_to_hbm(&diminfo[i], dimming, hbm);
			if (ret) {
				dsim_err("%s : failed to get hbm gamma\n", __func__);
				goto error;
			}
		}
	}

	for (i = 0; i < MAX_BR_INFO; i++) {
		method = diminfo[i].way;
		if (method == DIMMING_METHOD_AID) {
			ret = cal_gamma_from_index(dimming, &diminfo[i]);
			if (ret) {
				dsim_err("%s : failed to calculate gamma : index : %d\n", __func__, i);
				goto error;
			}
		}
		if (method == DIMMING_METHOD_INTERPOLATION) {
			ret = interpolation_gamma_to_hbm(diminfo, i);
			if (ret) {
				dsim_err("%s : failed to calculate gamma : index : %d\n", __func__, i);
				goto error;
			}
		}
	}

	for (i = 0; i < MAX_BR_INFO; i++) {
		memset(string_buf, 0, sizeof(string_buf));
		string_offset = sprintf(string_buf, "gamma[%3d] : ",diminfo[i].br);

		for(j = 0; j < GAMMA_CMD_CNT; j++)
			string_offset += sprintf(string_buf + string_offset, "%02x ", diminfo[i].gamma[j]);

		dsim_info("%s\n", string_buf);
	}

error:
	return ret;

}


#endif

#ifdef CONFIG_DSIM_ESD_REMOVE_DISP_DET
int esd_get_ddi_status(struct dsim_device *dsim,  int reg)
{
	int ret = 0;
	unsigned char err_buf[4] = {0, };

	dsim_info("[REM_DISP_DET] %s : reg = %x \n", __func__, reg);

	if(reg == 0x0A) {
		ret = dsim_read_hl_data(dsim, 0x0A, 3, err_buf);
		if (ret != 3) {
			dsim_err("[REM_DISP_DET] %s : can't read Panel's 0A Reg\n",__func__);
			goto dump_exit;
		}

		dsim_err("[REM_DISP_DET] %s : 0x0A : buf[0] = %x\n", __func__, err_buf[0]);

		ret = err_buf[0];
	}
	else {
		dsim_dbg("[REM_DISP_DET] %s : reg = %x is not ready \n", __func__, reg);
	}

dump_exit:
	return ret;

}
#endif

// convert NEW SPEC to OLD SPEC
static int VT_RGB2GRB(unsigned char* vt_in_mtp)
{
	int ret = 0;
	int r, g, b;

	// new SPEC = RG0B
	r = (vt_in_mtp[0] & 0xF0) >>4;
	g = (vt_in_mtp[0] & 0x0F);
	b = (vt_in_mtp[1] & 0x0F);

	// old spec = GR0B
	vt_in_mtp[0] = g<<4 | r;
	vt_in_mtp[1] = b;

	return ret;
}

static int s6e3hf4_read_init_info(struct dsim_device *dsim, unsigned char* mtp, unsigned char* hbm)
{
	int i = 0;
	int ret;
	struct panel_private *panel = &dsim->priv;
	unsigned char bufForCoordi[S6E3HF4_COORDINATE_LEN] = {0,};
	unsigned char buf[S6E3HF4_MTP_DATE_SIZE] = {0, };
	unsigned char hbm_gamma[S6E3HF4_HBMGAMMA_LEN] = {0, };


	dsim_info("MDD : %s was called\n", __func__);

	ret = dsim_write_hl_data(dsim, SEQ_TEST_KEY_ON_F0, ARRAY_SIZE(SEQ_TEST_KEY_ON_F0));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : SEQ_TEST_KEY_ON_F0\n", __func__);
	}

	// id
	ret = dsim_read_hl_data(dsim, S6E3HF4_ID_REG, S6E3HF4_ID_LEN, dsim->priv.id);
	if (ret != S6E3HF4_ID_LEN) {
		dsim_err("%s : can't find connected panel. check panel connection\n",__func__);
		panel->lcdConnected = PANEL_DISCONNEDTED;
		goto read_exit;
	}

	dsim_info("READ ID : ");
	for(i = 0; i < S6E3HF4_ID_LEN; i++)
		dsim_info("%02x, ", dsim->priv.id[i]);
	dsim_info("\n");

	dsim->priv.current_model = (dsim->priv.id[1] >> 4) & 0x0F;
	dsim->priv.panel_rev = dsim->priv.id[2] & 0x0F;
	dsim_info("%s model is %d, panel rev : %d",
		__func__, dsim->priv.current_model, dsim->priv.panel_rev);


	// mtp
	ret = dsim_read_hl_data(dsim, S6E3HF4_MTP_ADDR, S6E3HF4_MTP_DATE_SIZE, buf);
	if (ret != S6E3HF4_MTP_DATE_SIZE) {
		dsim_err("failed to read mtp, check panel connection\n");
		goto read_fail;
	}
	VT_RGB2GRB(buf + S6E3HF4_MTP_VT_ADDR);
	memcpy(mtp, buf, S6E3HF4_MTP_SIZE);
	memcpy(dsim->priv.date, &buf[40], ARRAY_SIZE(dsim->priv.date));
	dsim_info("READ MTP SIZE : %d\n", S6E3HF4_MTP_SIZE);
	dsim_info("=========== MTP INFO =========== \n");
	for (i = 0; i < S6E3HF4_MTP_SIZE; i++)
		dsim_info("MTP[%2d] : %2d : %2x\n", i, mtp[i], mtp[i]);

	// coordinate
	ret = dsim_read_hl_data(dsim, S6E3HF4_COORDINATE_REG, S6E3HF4_COORDINATE_LEN, bufForCoordi);
	if (ret != S6E3HF4_COORDINATE_LEN) {
		dsim_err("fail to read coordinate on command.\n");
		goto read_fail;
	}
	dsim->priv.coordinate[0] = bufForCoordi[0] << 8 | bufForCoordi[1];	/* X */
	dsim->priv.coordinate[1] = bufForCoordi[2] << 8 | bufForCoordi[3];	/* Y */
	dsim_info("READ coordi : ");
	for(i = 0; i < 2; i++)
		dsim_info("%d, ", dsim->priv.coordinate[i]);
	dsim_info("\n");


	// code
	ret = dsim_read_hl_data(dsim, S6E3HF4_CODE_REG, S6E3HF4_CODE_LEN, dsim->priv.code);
	if (ret != S6E3HF4_CODE_LEN) {
		dsim_err("fail to read code on command.\n");
		goto read_fail;
	}
	dsim_info("READ code : ");
	for(i = 0; i < S6E3HF4_CODE_LEN; i++)
		dsim_info("%x, ", dsim->priv.code[i]);
	dsim_info("\n");

	// elvss
	ret = dsim_read_hl_data(dsim, ELVSS_REG, ELVSS_LEN, dsim->priv.elvss_set);
	if (ret != ELVSS_LEN) {
		dsim_err("fail to read elvss on command.\n");
		goto read_fail;
	}
	dsim_info("READ elvss : ");
	for(i = 0; i < ELVSS_LEN; i++)
		dsim_info("%x \n", dsim->priv.elvss_set[i]);

	ret = dsim_read_hl_data(dsim, S6E3HF4_HBMGAMMA_REG, S6E3HF4_HBMGAMMA_LEN, hbm_gamma);
	if (ret != S6E3HF4_HBMGAMMA_LEN) {
		dsim_err("fail to read elvss on command.\n");
		goto read_fail;
	}
	dsim_info("HBM Gamma : ");
	memcpy(hbm, hbm_gamma, S6E3HF4_HBMGAMMA_LEN);

	for(i = 50; i < S6E3HF4_HBMGAMMA_LEN; i++)
		dsim_info("hbm gamma[%d] : %x\n", i, hbm_gamma[i]);

	ret = dsim_write_hl_data(dsim, SEQ_TEST_KEY_OFF_F0, ARRAY_SIZE(SEQ_TEST_KEY_OFF_F0));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : SEQ_TEST_KEY_OFF_F0\n", __func__);
		goto read_fail;
	}
read_exit:
	return 0;

read_fail:
	return -ENODEV;

}

static int s6e3hf4_wqhd_dump(struct dsim_device *dsim)
{
	int ret = 0;
	int i;
	unsigned char id[S6E3HF4_ID_LEN];
	unsigned char rddpm[4];
	unsigned char rddsm[4];
	unsigned char err_buf[4];

	ret = dsim_write_hl_data(dsim, SEQ_TEST_KEY_ON_F0, ARRAY_SIZE(SEQ_TEST_KEY_ON_F0));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : SEQ_TEST_KEY_ON_F0\n", __func__);
	}

	ret = dsim_write_hl_data(dsim, SEQ_TEST_KEY_ON_FC, ARRAY_SIZE(SEQ_TEST_KEY_ON_FC));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : SEQ_TEST_KEY_ON_FC\n", __func__);
	}

	ret = dsim_read_hl_data(dsim, 0xEA, 3, err_buf);
	if (ret != 3) {
		dsim_err("%s : can't read Panel's EA Reg\n",__func__);
		goto dump_exit;
	}

	dsim_info("=== Panel's 0xEA Reg Value ===\n");
	dsim_info("* 0xEA : buf[0] = %x\n", err_buf[0]);
	dsim_info("* 0xEA : buf[1] = %x\n", err_buf[1]);

	ret = dsim_read_hl_data(dsim, S6E3HF4_RDDPM_ADDR, 3, rddpm);
	if (ret != 3) {
		dsim_err("%s : can't read RDDPM Reg\n",__func__);
		goto dump_exit;
	}

	dsim_info("=== Panel's RDDPM Reg Value : %x ===\n", rddpm[0]);

	if (rddpm[0] & 0x80)
		dsim_info("* Booster Voltage Status : ON\n");
	else
		dsim_info("* Booster Voltage Status : OFF\n");

	if (rddpm[0] & 0x40)
		dsim_info("* Idle Mode : On\n");
	else
		dsim_info("* Idle Mode : OFF\n");

	if (rddpm[0] & 0x20)
		dsim_info("* Partial Mode : On\n");
	else
		dsim_info("* Partial Mode : OFF\n");

	if (rddpm[0] & 0x10)
		dsim_info("* Sleep OUT and Working Ok\n");
	else
		dsim_info("* Sleep IN\n");

	if (rddpm[0] & 0x08)
		dsim_info("* Normal Mode On and Working Ok\n");
	else
		dsim_info("* Sleep IN\n");

	if (rddpm[0] & 0x04)
		dsim_info("* Display On and Working Ok\n");
	else
		dsim_info("* Display Off\n");

	ret = dsim_read_hl_data(dsim, S6E3HF4_RDDSM_ADDR, 3, rddsm);
	if (ret != 3) {
		dsim_err("%s : can't read RDDSM Reg\n",__func__);
		goto dump_exit;
	}

	dsim_info("=== Panel's RDDSM Reg Value : %x ===\n", rddsm[0]);

	if (rddsm[0] & 0x80)
		dsim_info("* TE On\n");
	else
		dsim_info("* TE OFF\n");

	if (rddsm[0] & 0x02)
		dsim_info("* S_DSI_ERR : Found\n");

	if (rddsm[0] & 0x01)
		dsim_info("* DSI_ERR : Found\n");

	ret = dsim_read_hl_data(dsim, S6E3HF4_ID_REG, S6E3HF4_ID_LEN, id);
	if (ret != S6E3HF4_ID_LEN) {
		dsim_err("%s : can't read panel id\n",__func__);
		goto dump_exit;
	}

	dsim_info("READ ID : ");
	for(i = 0; i < S6E3HF4_ID_LEN; i++)
		dsim_info("%02x, ", id[i]);
	dsim_info("\n");

	ret = dsim_write_hl_data(dsim, SEQ_TEST_KEY_OFF_FC, ARRAY_SIZE(SEQ_TEST_KEY_OFF_FC));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : SEQ_TEST_KEY_OFF_FC\n", __func__);
	}

	ret = dsim_write_hl_data(dsim, SEQ_TEST_KEY_OFF_F0, ARRAY_SIZE(SEQ_TEST_KEY_OFF_F0));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : SEQ_TEST_KEY_OFF_F0\n", __func__);
	}
dump_exit:
	return ret;

}
static int s6e3hf4_wqhd_probe(struct dsim_device *dsim)
{
	int ret = 0;
	struct panel_private *panel = &dsim->priv;
	unsigned char mtp[S6E3HF4_MTP_SIZE] = {0, };
	unsigned char hbm[S6E3HF4_HBMGAMMA_LEN] = {0, };

	dsim_info("MDD : %s was called\n", __func__);

	panel->dim_data = (void *)NULL;
	panel->lcdConnected = PANEL_CONNECTED;
	panel->panel_type = 0;

	ret = s6e3hf4_read_init_info(dsim, mtp, hbm);
	if (panel->lcdConnected == PANEL_DISCONNEDTED) {
		dsim_err("dsim : %s lcd was not connected\n", __func__);
		goto probe_exit;
	}

#ifdef CONFIG_LCD_ALPM
	panel->alpm = 0;
	panel->current_alpm = 0;
	panel->alpm_support = SUPPORT_30HZALPM;
	if((panel->current_model == MODEL_IS_HERO2) && (panel->panel_rev >= 1))
		panel->alpm_support = SUPPORT_1HZALPM;
	mutex_init(&panel->alpm_lock);
#endif


	if ((panel->id[1] & 0x10) && ((panel->id[2] & 0x0F) > 0))	// HERO2,  upper rev.B
		dsim->priv.esd_disable = 0;
	else
		dsim->priv.esd_disable = 1;

#ifdef CONFIG_EXYNOS_DECON_MDNIE_LITE
	panel->mdnie_support = 1;
#endif

#ifdef CONFIG_PANEL_AID_DIMMING
	ret = init_dimming(dsim, mtp, hbm);
	if (ret) {
		dsim_err("%s : failed to generate gamma tablen\n", __func__);
	}
#endif

probe_exit:
	return ret;

}


static int s6e3hf4_wqhd_displayon(struct dsim_device *dsim)
{
	int ret = 0;
#ifdef CONFIG_LCD_ALPM
	struct panel_private *panel = &dsim->priv;
#endif

	dsim_info("MDD : %s was called\n", __func__);

#ifdef CONFIG_LCD_ALPM
	if (panel->current_alpm && panel->alpm) {
		 dsim_info("%s : ALPM mode\n", __func__);
	} else if (panel->current_alpm) {
		ret = alpm_set_mode(dsim, ALPM_OFF);
		if (ret) {
			dsim_err("failed to exit alpm.\n");
			goto displayon_err;
		}
	} else {
		if (panel->alpm) {
			ret = alpm_set_mode(dsim, panel->alpm);
			if (ret) {
				dsim_err("failed to initialize alpm.\n");
				goto displayon_err;
			}
		} else {
			ret = dsim_write_hl_data(dsim, SEQ_DISPLAY_ON, ARRAY_SIZE(SEQ_DISPLAY_ON));
			if (ret < 0) {
				dsim_err("%s : fail to write CMD : DISPLAY_ON\n", __func__);
				goto displayon_err;
			}
		}
	}
#else
	ret = dsim_write_hl_data(dsim, SEQ_DISPLAY_ON, ARRAY_SIZE(SEQ_DISPLAY_ON));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : DISPLAY_ON\n", __func__);
 		goto displayon_err;
	}
#endif

displayon_err:
	return ret;

}


static int s6e3hf4_wqhd_exit(struct dsim_device *dsim)
{
	int ret = 0;
#ifdef CONFIG_LCD_ALPM
	struct panel_private *panel = &dsim->priv;
#endif
	dsim_info("MDD : %s was called\n", __func__);
#ifdef CONFIG_LCD_ALPM
	mutex_lock(&panel->alpm_lock);
	if (panel->current_alpm && panel->alpm) {
		dsim->alpm = panel->current_alpm;
		dsim_info( "%s : ALPM mode\n", __func__);
	} else {
		ret = dsim_write_hl_data(dsim, SEQ_DISPLAY_OFF, ARRAY_SIZE(SEQ_DISPLAY_OFF));
		if (ret < 0) {
			dsim_err("%s : fail to write CMD : DISPLAY_OFF\n", __func__);
			goto exit_err;
		}

		ret = dsim_write_hl_data(dsim, SEQ_SLEEP_IN, ARRAY_SIZE(SEQ_SLEEP_IN));
		if (ret < 0) {
			dsim_err("%s : fail to write CMD : SLEEP_IN\n", __func__);
			goto exit_err;
		}
		msleep(120);
	}

	dsim_info("MDD : %s was called unlock\n", __func__);
#else
	ret = dsim_write_hl_data(dsim, SEQ_DISPLAY_OFF, ARRAY_SIZE(SEQ_DISPLAY_OFF));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : DISPLAY_OFF\n", __func__);
		goto exit_err;
	}

	ret = dsim_write_hl_data(dsim, SEQ_SLEEP_IN, ARRAY_SIZE(SEQ_SLEEP_IN));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : SLEEP_IN\n", __func__);
		goto exit_err;
	}

	msleep(120);
#endif

exit_err:
#ifdef CONFIG_LCD_ALPM
	mutex_unlock(&panel->alpm_lock);
#endif
	return ret;
}

static int s6e3hf4_wqhd_init(struct dsim_device *dsim)
{
	int ret = 0;

	dsim_info("MDD : %s was called\n", __func__);

#ifdef CONFIG_LCD_ALPM
	if (dsim->priv.current_alpm) {
		dsim_info("%s : ALPM mode\n", __func__);

		return ret;
	}
#endif
	/* DSC setting */
	msleep(5);

	ret = dsim_write_data(dsim, MIPI_DSI_DSC_PRA, SEQ_DSC_EN[0], SEQ_DSC_EN[1]);
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : SEQ_DSC_EN\n", __func__);
		goto init_exit;
	}

	ret = dsim_write_data(dsim, MIPI_DSI_DSC_PPS, (unsigned long)SEQ_PPS_SLICE4, ARRAY_SIZE(SEQ_PPS_SLICE4));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : SEQ_PPS_SLICE4\n", __func__);
		goto init_exit;
	}

	/* Sleep Out(11h) */
	ret = dsim_write_hl_data(dsim, SEQ_SLEEP_OUT, ARRAY_SIZE(SEQ_SLEEP_OUT));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : SEQ_SLEEP_OUT\n", __func__);
		goto init_exit;
	}
#ifdef CONFIG_LCD_HMT
	if(dsim->priv.hmt_on != HMT_ON)
#endif
		msleep(120);

	/* Interface Setting */
	ret = dsim_write_hl_data(dsim, SEQ_TEST_KEY_ON_F0, ARRAY_SIZE(SEQ_TEST_KEY_ON_F0));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : SEQ_TEST_KEY_ON_F0\n", __func__);
		goto init_exit;
	}


	/* Common Setting */
	ret = dsim_write_hl_data(dsim, SEQ_TE_ON, ARRAY_SIZE(SEQ_TE_ON));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : SEQ_TE_ON\n", __func__);
		goto init_exit;
	}

	ret = dsim_write_hl_data(dsim, SEQ_TSP_TE, ARRAY_SIZE(SEQ_TSP_TE));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : SEQ_TSP_TE\n", __func__);
		goto init_exit;
	}

	ret = dsim_write_hl_data(dsim, SEQ_ERR_FG_SETTING, ARRAY_SIZE(SEQ_ERR_FG_SETTING));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : SEQ_ERR_FG_SETTING\n", __func__);
		goto init_exit;
	}
	ret = dsim_write_hl_data(dsim, SEQ_TE_START_SETTING, ARRAY_SIZE(SEQ_TE_START_SETTING));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : SEQ_TE_START_SETTING\n", __func__);
		goto init_exit;
	}

#ifndef CONFIG_PANEL_AID_DIMMING
	/* Brightness Setting */
	ret = dsim_write_hl_data(dsim, SEQ_GAMMA_CONDITION_SET, ARRAY_SIZE(SEQ_GAMMA_CONDITION_SET));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_GAMMA_CONDITION_SET\n", __func__);
		goto init_exit;
	}
	ret = dsim_write_hl_data(dsim, SEQ_AOR_CONTROL, ARRAY_SIZE(SEQ_AOR_CONTROL));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_AOR_CONTROL\n", __func__);
		goto init_exit;
	}
	ret = dsim_write_hl_data(dsim, SEQ_GAMMA_UPDATE, ARRAY_SIZE(SEQ_GAMMA_UPDATE));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_GAMMA_UPDATE\n", __func__);
		goto init_exit;
	}

	/* elvss */
	ret = dsim_write_hl_data(dsim, SEQ_TSET_ELVSS_SET, ARRAY_SIZE(SEQ_TSET_ELVSS_SET));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_TSET_ELVSS_SET\n", __func__);
		goto init_exit;
	}

	ret = dsim_write_hl_data(dsim, SEQ_VINT_SET, ARRAY_SIZE(SEQ_VINT_SET));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_VINT_SET\n", __func__);
		goto init_exit;
	}
	/* ACL Setting */
	ret = dsim_write_hl_data(dsim, SEQ_ACL_OFF, ARRAY_SIZE(SEQ_ACL_OFF));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_ACL_OFF\n", __func__);
		goto init_exit;
	}
	ret = dsim_write_hl_data(dsim, SEQ_ACL_OFF_OPR_AVR, ARRAY_SIZE(SEQ_ACL_OFF_OPR_AVR));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_ACL_OFF_OPR\n", __func__);
		goto init_exit;
	}
#endif
	ret = dsim_write_hl_data(dsim, SEQ_TEST_KEY_OFF_F0, ARRAY_SIZE(SEQ_TEST_KEY_OFF_F0));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : SEQ_TEST_KEY_OFF_F0\n", __func__);
		goto init_exit;
	}

init_exit:
	return ret;
}


struct dsim_panel_ops s6e3hf4_panel_ops = {
	.early_probe = NULL,
	.probe		= s6e3hf4_wqhd_probe,
	.displayon	= s6e3hf4_wqhd_displayon,
	.exit		= s6e3hf4_wqhd_exit,
	.init		= s6e3hf4_wqhd_init,
	.dump 		= s6e3hf4_wqhd_dump,
};

struct dsim_panel_ops *dsim_panel_get_priv_ops(struct dsim_device *dsim)
{
	return &s6e3hf4_panel_ops;
}

static int __init s6e3hf4_get_lcd_type(char *arg)
{
	unsigned int lcdtype;

	get_option(&arg, &lcdtype);

	dsim_info("--- Parse LCD TYPE ---\n");
	dsim_info("LCDTYPE : %x\n", lcdtype);
	return 0;
}
early_param("lcdtype", s6e3hf4_get_lcd_type);

