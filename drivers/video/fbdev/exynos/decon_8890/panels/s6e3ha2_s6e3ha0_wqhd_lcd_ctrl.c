/*
 * drivers/video/decon/panels/S6E3HA0_lcd_ctrl.c
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

#ifdef CONFIG_PANEL_AID_DIMMING
#include "aid_dimming.h"
#include "dimming_core.h"
#include "s6e3ha2_wqhd_aid_dimming.h"
#endif
#include "s6e3ha2_s6e3ha0_wqhd_param.h"
#include "../dsim.h"
#include <video/mipi_display.h>

static unsigned dynamic_lcd_type = 0;

static int s6e3ha2_s6e3ha0_early_probe(struct dsim_device *dsim)
{
	int ret = 0;
	struct panel_private *panel = &dsim->priv;

	panel->panel_type = dynamic_lcd_type;

	dsim_info("MDD %s was called\n", __func__);
	dsim_info("%s : panel type : %d\n", __func__, panel->panel_type);

	return ret;
}

static int s6e3ha0_read_init_info(struct dsim_device *dsim)
{
	int i;
	int ret = 0;
	struct panel_private *panel = &dsim->priv;

	ret = dsim_write_hl_data(dsim, S6E3HA0_SEQ_TEST_KEY_ON_F0, ARRAY_SIZE(S6E3HA0_SEQ_TEST_KEY_ON_F0));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : SEQ_TEST_KEY_ON_F0\n", __func__);
	}

	// id
	ret = dsim_read_hl_data(dsim, S6E3HA0_ID_REG, S6E3HA0_ID_LEN, dsim->priv.id);
	if (ret != S6E3HA0_ID_LEN) {
		dsim_err("%s : can't find connected panel. check panel connection\n",__func__);
		panel->lcdConnected = PANEL_DISCONNEDTED;
		goto read_exit;
	}

	dsim_info("READ ID : ");
	for(i = 0; i < S6E3HA0_ID_LEN; i++)
		dsim_info("%02x, ", dsim->priv.id[i]);
	dsim_info("\n");

	ret = dsim_write_hl_data(dsim, S6E3HA0_SEQ_TEST_KEY_OFF_F0, ARRAY_SIZE(S6E3HA0_SEQ_TEST_KEY_OFF_F0));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : SEQ_TEST_KEY_OFF_F0\n", __func__);
		goto read_fail;
	}

read_exit:
	return 0;

read_fail:
	return -ENODEV;
}
static int s6e3ha0_wqhd_probe(struct dsim_device *dsim)
{
	int ret = 0;

	struct panel_private *panel = &dsim->priv;
	panel->dim_data = (void *)NULL;
	panel->lcdConnected = PANEL_CONNECTED;

	dsim_info(" +  : %s\n", __func__);

	ret = s6e3ha0_read_init_info(dsim);
	if (panel->lcdConnected == PANEL_DISCONNEDTED) {
		dsim_err("dsim : %s lcd was not connected\n", __func__);
		goto probe_exit;
	}

	panel->dim_data = (void *)NULL;
#ifdef CONFIG_EXYNOS_DECON_MDNIE_LITE
	panel->mdnie_support = 0;
	if(panel->panel_type != LCD_TYPE_S6E3HA0_WQXGA)
		panel->mdnie_support = 1;
#endif

probe_exit:
	return ret;
}


static int s6e3ha0_wqhd_displayon(struct dsim_device *dsim)
{
	int ret = 0;

	dsim_info("MDD : %s was called\n", __func__);

	ret = dsim_write_hl_data(dsim, S6E3HA0_SEQ_DISPLAY_ON, ARRAY_SIZE(S6E3HA0_SEQ_DISPLAY_ON));
	if (ret < 0) {
 		dsim_err("%s : fail to write CMD : DISPLAY_ON\n", __func__);
 		goto displayon_err;
	}

displayon_err:
	return ret;
}


static int s6e3ha0_wqhd_exit(struct dsim_device *dsim)
{
	int ret = 0;

	dsim_info("MDD : %s was called\n", __func__);

	ret = dsim_write_hl_data(dsim, S6E3HA0_SEQ_DISPLAY_OFF, ARRAY_SIZE(S6E3HA0_SEQ_DISPLAY_OFF));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : DISPLAY_OFF\n", __func__);
		goto exit_err;
	}

	msleep(10);

	ret = dsim_write_hl_data(dsim, S6E3HA0_SEQ_SLEEP_IN, ARRAY_SIZE(S6E3HA0_SEQ_SLEEP_IN));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : SLEEP_IN\n", __func__);
		goto exit_err;
	}

	msleep(120);

exit_err:
	return ret;
}

static int s6e3ha0_wqhd_init(struct dsim_device *dsim)
{
	int ret = 0;

	dsim_info("MDD : %s was called\n", __func__);

	ret = dsim_write_hl_data(dsim, S6E3HA0_SEQ_TEST_KEY_ON_F0, ARRAY_SIZE(S6E3HA0_SEQ_TEST_KEY_ON_F0));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : SEQ_TEST_KEY_ON_F0\n", __func__);
		goto init_err;
	}
	ret = dsim_write_hl_data(dsim, S6E3HA0_SEQ_SINGLE_DSI_1, ARRAY_SIZE(S6E3HA0_SEQ_SINGLE_DSI_1));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : SEQ_SINGLE_DSI_1\n", __func__);
		goto init_err;
	}
	ret = dsim_write_hl_data(dsim, S6E3HA0_SEQ_SINGLE_DSI_2, ARRAY_SIZE(S6E3HA0_SEQ_SINGLE_DSI_2));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : SEQ_SINGLE_DSI_2\n", __func__);
		goto init_err;
	}
	msleep(5);

	ret = dsim_write_hl_data(dsim, S6E3HA0_SEQ_TEST_KEY_ON_FC, ARRAY_SIZE(S6E3HA0_SEQ_TEST_KEY_ON_FC));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_TEST_KEY_ON_FC\n", __func__);
		goto init_err;
	}
	ret = dsim_write_hl_data(dsim, S6E3HA0_SEQ_SLEEP_OUT, ARRAY_SIZE(S6E3HA0_SEQ_SLEEP_OUT));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_TEST_KEY_ON_FC\n", __func__);
		goto init_err;
	}

	msleep(20);

	/* Common Setting */
	ret = dsim_write_hl_data(dsim, S6E3HA0_SEQ_TOUCH_HSYNC, ARRAY_SIZE(S6E3HA0_SEQ_TOUCH_HSYNC));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_TOUCH_HSYNC\n", __func__);
		goto init_err;
	}
	ret = dsim_write_hl_data(dsim, S6E3HA0_SEQ_TOUCH_VSYNC, ARRAY_SIZE(S6E3HA0_SEQ_TOUCH_VSYNC));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_TOUCH_VSYNC\n", __func__);
		goto init_err;
	}
	ret = dsim_write_hl_data(dsim, S6E3HA0_SEQ_PENTILE_SETTING, ARRAY_SIZE(S6E3HA0_SEQ_PENTILE_SETTING));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_PENTILE_SETTING\n", __func__);
		goto init_err;
	}
	ret = dsim_write_hl_data(dsim, S6E3HA0_SEQ_TE_ON, ARRAY_SIZE(S6E3HA0_SEQ_TE_ON));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_TE_ON\n", __func__);
		goto init_err;
	}

	msleep(120);

	ret = dsim_write_hl_data(dsim, S6E3HA0_SEQ_TEST_KEY_OFF_FC, ARRAY_SIZE(S6E3HA0_SEQ_TEST_KEY_OFF_FC));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_TEST_KEY_OFF_FC\n", __func__);
		goto init_err;
	}

	/* Brightness Setting */
	ret = dsim_write_hl_data(dsim, S6E3HA0_SEQ_GAMMA_CONDITION_SET, ARRAY_SIZE(S6E3HA0_SEQ_GAMMA_CONDITION_SET));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_GAMMA_CONDITION_SET\n", __func__);
		goto init_err;
	}
	ret = dsim_write_hl_data(dsim, S6E3HA0_SEQ_AOR_CONTROL, ARRAY_SIZE(S6E3HA0_SEQ_AOR_CONTROL));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_AOR_CONTROL\n", __func__);
		goto init_err;
	}
	ret = dsim_write_hl_data(dsim, S6E3HA0_SEQ_GAMMA_UPDATE, ARRAY_SIZE(S6E3HA0_SEQ_GAMMA_UPDATE));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_GAMMA_UPDATE\n", __func__);
		goto init_err;
	}
	/* elvss */
	ret = dsim_write_hl_data(dsim, S6E3HA0_SEQ_TSET_GLOBAL, ARRAY_SIZE(S6E3HA0_SEQ_TSET_GLOBAL));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_TSET_GLOBAL\n", __func__);
		goto init_err;
	}
	ret = dsim_write_hl_data(dsim, S6E3HA0_SEQ_TSET, ARRAY_SIZE(S6E3HA0_SEQ_TSET));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_TSET\n", __func__);
		goto init_err;
	}
	ret = dsim_write_hl_data(dsim, S6E3HA0_SEQ_ELVSS_SET, ARRAY_SIZE(S6E3HA0_SEQ_ELVSS_SET));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_ELVSS_SET\n", __func__);
		goto init_err;
	}
	/* ACL Setting */
	ret = dsim_write_hl_data(dsim, S6E3HA0_SEQ_ACL_OFF, ARRAY_SIZE(S6E3HA0_SEQ_ACL_OFF));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_ACL_OFF\n", __func__);
		goto init_err;
	}
	ret = dsim_write_hl_data(dsim, S6E3HA0_SEQ_ACL_OFF_OPR, ARRAY_SIZE(S6E3HA0_SEQ_ACL_OFF_OPR));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_ACL_OFF_OPR\n", __func__);
		goto init_err;
	}

	ret = dsim_write_hl_data(dsim, S6E3HA0_SEQ_TEST_KEY_OFF_F0, ARRAY_SIZE(S6E3HA0_SEQ_TEST_KEY_OFF_F0));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_TEST_KEY_OFF_F0\n", __func__);
		goto init_err;
	}
init_err:
	return ret;
}



struct dsim_panel_ops s6e3ha0_panel_ops = {
	.early_probe = s6e3ha2_s6e3ha0_early_probe,
	.probe		= s6e3ha0_wqhd_probe,
	.displayon	= s6e3ha0_wqhd_displayon,
	.exit		= s6e3ha0_wqhd_exit,
	.init		= s6e3ha0_wqhd_init,
	.dump 		= NULL,
};

#ifdef CONFIG_PANEL_AID_DIMMING
static const unsigned char *HBM_TABLE[HBM_STATUS_MAX] = {S6E3HA2_SEQ_HBM_OFF, S6E3HA2_SEQ_HBM_ON};
static const unsigned char *ACL_CUTOFF_TABLE[ACL_STATUS_MAX] = {S6E3HA2_SEQ_ACL_OFF, S6E3HA2_SEQ_ACL_8};
static const unsigned char *ACL_OPR_TABLE[ACL_OPR_MAX] = {S6E3HA2_SEQ_ACL_OFF_OPR, S6E3HA2_SEQ_ACL_ON_OPR};

static const unsigned int br_tbl [256] = {
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
	333, 333, 333, 333, 333, 333, 360							//7
};


static const unsigned int hbm_interpolation_br_tbl[256] = {
	2,		2,		2,		4,		7,		9,		11,		14,		16,		19,		21,		23,		26,		28,		30,		33,
	35,		37,		40,		42,		45,		47,		49,		52,		54,		56,		59,		61,		63,		66,		68,		71,
	73,		75,		78,		80,		82,		85,		87,		89,		92,		94,		97,		99,		101,	104,	106,	108,
	111,	113,	115,	118,	120,	123,	125,	127,	130,	132,	134,	137,	139,	141,	144,	146,
	149,	151,	153,	156,	158,	160,	163,	165,	167,	170,	172,	175,	177,	179,	182,	184,
	186,	189,	191,	193,	196,	198,	201,	203,	205,	208,	210,	212,	215,	217,	219,	222,
	224,	227,	229,	231,	234,	236,	238,	241,	243,	245,	248,	250,	253,	255,	257,	260,
	262,	264,	267,	269,	271,	274,	276,	279,	281,	283,	286,	288,	290,	293,	295,	297,
	300,	302,	305,	307,	309,	312,	314,	316,	319,	321,	323,	326,	328,	331,	333,	335,
	338,	340,	342,	345,	347,	349,	352,	354,	357,	359,	361,	364,	366,	368,	371,	373,
	375,	378,	380,	383,	385,	387,	390,	392,	394,	397,	399,	401,	404,	406,	409,	411,
	413,	416,	418,	420,	423,	425,	427,	430,	432,	435,	437,	439,	442,	444,	446,	449,
	451,	453,	456,	458,	461,	463,	465,	468,	470,	472,	475,	477,	479,	482,	484,	487,
	489,	491,	494,	496,	498,	501,	503,	505,	508,	510,	513,	515,	517,	520,	522,	524,
	527,	529,	531,	534,	536,	539,	541,	543,	546,	548,	550,	553,	555,	557,	560,	562,
	565,	567,	569,	572,	574,	576,	579,	581,	583,	586,	588,	591,	593,	595,	598,	600
};

static const unsigned gallery_br_tbl[256] = {
	 2,  2,  2,  3,  5,  6,  8,  10,  11,  13,  15,  16,  18,  20,  21,  23,
	 25,  26,  28,  29,  32,  33,  34,  36,  38,  39,  41,  43,  44,  46,  48,  50,
	 51,  53,  55,  56,  57,  60,  61,  62,  64,  66,  68,  69,  71,  73,  74,  76,
	 78,  79,  81,  83,  84,  86,  88,  89,  91,  92,  94,  96,  97,  99,  101, 102,
	 104, 106, 107, 109, 111, 112, 114, 116, 117, 119, 120, 123, 124, 125, 127, 129,
	 130, 132, 134, 135, 137, 139, 141, 142, 144, 146, 147, 148, 151, 152, 153, 155,
	 157, 159, 160, 162, 164, 165, 167, 169, 170, 172, 174, 175, 177, 179, 180, 182,
	 183, 185, 187, 188, 190, 192, 193, 195, 197, 198, 200, 202, 203, 205, 207, 208,
	 210, 211, 214, 215, 216, 218, 220, 221, 223, 225, 226, 228, 230, 232, 233, 235,
	 237, 238, 239, 242, 243, 244, 246, 248, 250, 251, 253, 255, 256, 258, 260, 261,
	 263, 265, 266, 268, 270, 271, 273, 274, 276, 278, 279, 281, 283, 284, 286, 288,
	 289, 291, 293, 294, 296, 298, 299, 301, 302, 305, 306, 307, 309, 311, 312, 314,
	 316, 317, 319, 321, 323, 324, 326, 328, 329, 330, 333, 334, 335, 337, 339, 341,
	 342, 344, 346, 347, 349, 351, 352, 354, 356, 357, 359, 361, 362, 364, 365, 367,
	 369, 370, 372, 374, 375, 377, 379, 380, 382, 384, 385, 387, 389, 390, 392, 393,
	 396, 397, 398, 400, 402, 403, 405, 407, 408, 410, 412, 414, 415, 417, 419, 430,
};




static const short center_gamma[NUM_VREF][CI_MAX] = {
	{0x000, 0x000, 0x000},
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

// aid sheet in rev.C opmanual
struct SmtDimInfo dimming_info_RC[MAX_BR_INFO] = {				// add hbm array
	{.br = 2, .refBr = 115, .cGma = gma2p15, .rTbl = RCrtbl2nit, .cTbl = RCctbl2nit, .aid = aid9821, .elvCaps = elvCaps5, .elv = elv5, .way = W1},
	{.br = 3, .refBr = 115, .cGma = gma2p15, .rTbl = RCrtbl3nit, .cTbl = RCctbl3nit, .aid = aid9717, .elvCaps = elvCaps4, .elv = elv4, .way = W1},
	{.br = 4, .refBr = 115, .cGma = gma2p15, .rTbl = RCrtbl4nit, .cTbl = RCctbl4nit, .aid = aid9639, .elvCaps = elvCaps3, .elv = elv3, .way = W1},
	{.br = 5, .refBr = 115, .cGma = gma2p15, .rTbl = RCrtbl5nit, .cTbl = RCctbl5nit, .aid = aid9538, .elvCaps = elvCaps2, .elv = elv2, .way = W1},
	{.br = 6, .refBr = 115, .cGma = gma2p15, .rTbl = RCrtbl6nit, .cTbl = RCctbl6nit, .aid = aid9433, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 7, .refBr = 115, .cGma = gma2p15, .rTbl = RCrtbl7nit, .cTbl = RCctbl7nit, .aid = aid9387, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 8, .refBr = 115, .cGma = gma2p15, .rTbl = RCrtbl8nit, .cTbl = RCctbl8nit, .aid = aid9309, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 9, .refBr = 115, .cGma = gma2p15, .rTbl = RCrtbl9nit, .cTbl = RCctbl9nit, .aid = aid9216, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 10, .refBr = 115, .cGma = gma2p15, .rTbl = RCrtbl10nit, .cTbl = RCctbl10nit, .aid = aid9123, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 11, .refBr = 115, .cGma = gma2p15, .rTbl = RCrtbl11nit, .cTbl = RCctbl11nit, .aid = aid9037, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 12, .refBr = 115, .cGma = gma2p15, .rTbl = RCrtbl12nit, .cTbl = RCctbl12nit, .aid = aid8998, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 13, .refBr = 115, .cGma = gma2p15, .rTbl = RCrtbl13nit, .cTbl = RCctbl13nit, .aid = aid8909, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 14, .refBr = 115, .cGma = gma2p15, .rTbl = RCrtbl14nit, .cTbl = RCctbl14nit, .aid = aid8816, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 15, .refBr = 115, .cGma = gma2p15, .rTbl = RCrtbl15nit, .cTbl = RCctbl15nit, .aid = aid8738, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 16, .refBr = 115, .cGma = gma2p15, .rTbl = RCrtbl16nit, .cTbl = RCctbl16nit, .aid = aid8661, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 17, .refBr = 115, .cGma = gma2p15, .rTbl = RCrtbl17nit, .cTbl = RCctbl17nit, .aid = aid8587, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 19, .refBr = 115, .cGma = gma2p15, .rTbl = RCrtbl19nit, .cTbl = RCctbl19nit, .aid = aid8412, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 20, .refBr = 115, .cGma = gma2p15, .rTbl = RCrtbl20nit, .cTbl = RCctbl20nit, .aid = aid8331, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 21, .refBr = 115, .cGma = gma2p15, .rTbl = RCrtbl21nit, .cTbl = RCctbl21nit, .aid = aid8245, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 22, .refBr = 115, .cGma = gma2p15, .rTbl = RCrtbl22nit, .cTbl = RCctbl22nit, .aid = aid8160, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 24, .refBr = 115, .cGma = gma2p15, .rTbl = RCrtbl24nit, .cTbl = RCctbl24nit, .aid = aid7985, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 25, .refBr = 115, .cGma = gma2p15, .rTbl = RCrtbl25nit, .cTbl = RCctbl25nit, .aid = aid7908, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 27, .refBr = 115, .cGma = gma2p15, .rTbl = RCrtbl27nit, .cTbl = RCctbl27nit, .aid = aid7729, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 29, .refBr = 115, .cGma = gma2p15, .rTbl = RCrtbl29nit, .cTbl = RCctbl29nit, .aid = aid7550, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 30, .refBr = 115, .cGma = gma2p15, .rTbl = RCrtbl30nit, .cTbl = RCctbl30nit, .aid = aid7473, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 32, .refBr = 115, .cGma = gma2p15, .rTbl = RCrtbl32nit, .cTbl = RCctbl32nit, .aid = aid7294, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 34, .refBr = 115, .cGma = gma2p15, .rTbl = RCrtbl34nit, .cTbl = RCctbl34nit, .aid = aid7116, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 37, .refBr = 115, .cGma = gma2p15, .rTbl = RCrtbl37nit, .cTbl = RCctbl37nit, .aid = aid6825, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 39, .refBr = 115, .cGma = gma2p15, .rTbl = RCrtbl39nit, .cTbl = RCctbl39nit, .aid = aid6669, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 41, .refBr = 115, .cGma = gma2p15, .rTbl = RCrtbl41nit, .cTbl = RCctbl41nit, .aid = aid6495, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 44, .refBr = 115, .cGma = gma2p15, .rTbl = RCrtbl44nit, .cTbl = RCctbl44nit, .aid = aid6234, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 47, .refBr = 115, .cGma = gma2p15, .rTbl = RCrtbl47nit, .cTbl = RCctbl47nit, .aid = aid5974, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 50, .refBr = 115, .cGma = gma2p15, .rTbl = RCrtbl50nit, .cTbl = RCctbl50nit, .aid = aid5683, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 53, .refBr = 115, .cGma = gma2p15, .rTbl = RCrtbl53nit, .cTbl = RCctbl53nit, .aid = aid5388, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 56, .refBr = 115, .cGma = gma2p15, .rTbl = RCrtbl56nit, .cTbl = RCctbl56nit, .aid = aid5155, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 60, .refBr = 115, .cGma = gma2p15, .rTbl = RCrtbl60nit, .cTbl = RCctbl60nit, .aid = aid4767, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 64, .refBr = 115, .cGma = gma2p15, .rTbl = RCrtbl64nit, .cTbl = RCctbl64nit, .aid = aid4379, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 68, .refBr = 115, .cGma = gma2p15, .rTbl = RCrtbl68nit, .cTbl = RCctbl68nit, .aid = aid3998, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 72, .refBr = 115, .cGma = gma2p15, .rTbl = RCrtbl72nit, .cTbl = RCctbl72nit, .aid = aid3630, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 77, .refBr = 123, .cGma = gma2p15, .rTbl = RCrtbl77nit, .cTbl = RCctbl77nit, .aid = aid3630, .elvCaps = elvCaps2, .elv = elv2, .way = W1},
	{.br = 82, .refBr = 130, .cGma = gma2p15, .rTbl = RCrtbl82nit, .cTbl = RCctbl82nit, .aid = aid3630, .elvCaps = elvCaps2, .elv = elv2, .way = W1},
	{.br = 87, .refBr = 136, .cGma = gma2p15, .rTbl = RCrtbl87nit, .cTbl = RCctbl87nit, .aid = aid3630, .elvCaps = elvCaps2, .elv = elv2, .way = W1},
	{.br = 93, .refBr = 144, .cGma = gma2p15, .rTbl = RCrtbl93nit, .cTbl = RCctbl93nit, .aid = aid3630, .elvCaps = elvCaps3, .elv = elv3, .way = W1},
	{.br = 98, .refBr = 152, .cGma = gma2p15, .rTbl = RCrtbl98nit, .cTbl = RCctbl98nit, .aid = aid3630, .elvCaps = elvCaps3, .elv = elv3, .way = W1},
	{.br = 105, .refBr = 162, .cGma = gma2p15, .rTbl = RCrtbl105nit, .cTbl = RCctbl105nit, .aid = aid3630, .elvCaps = elvCaps3, .elv = elv3, .way = W1},
	{.br = 111, .refBr = 170, .cGma = gma2p15, .rTbl = RCrtbl111nit, .cTbl = RCctbl111nit, .aid = aid3630, .elvCaps = elvCaps4, .elv = elv4, .way = W1},
	{.br = 119, .refBr = 179, .cGma = gma2p15, .rTbl = RCrtbl119nit, .cTbl = RCctbl119nit, .aid = aid3630, .elvCaps = elvCaps4, .elv = elv4, .way = W1},
	{.br = 126, .refBr = 189, .cGma = gma2p15, .rTbl = RCrtbl126nit, .cTbl = RCctbl126nit, .aid = aid3630, .elvCaps = elvCaps5, .elv = elv5, .way = W1},
	{.br = 134, .refBr = 199, .cGma = gma2p15, .rTbl = RCrtbl134nit, .cTbl = RCctbl134nit, .aid = aid3630, .elvCaps = elvCaps5, .elv = elv5, .way = W1},
	{.br = 143, .refBr = 209, .cGma = gma2p15, .rTbl = RCrtbl143nit, .cTbl = RCctbl143nit, .aid = aid3630, .elvCaps = elvCaps6, .elv = elv6, .way = W1},
	{.br = 152, .refBr = 221, .cGma = gma2p15, .rTbl = RCrtbl152nit, .cTbl = RCctbl152nit, .aid = aid3630, .elvCaps = elvCaps7, .elv = elv7, .way = W1},
	{.br = 162, .refBr = 235, .cGma = gma2p15, .rTbl = RCrtbl162nit, .cTbl = RCctbl162nit, .aid = aid3630, .elvCaps = elvCaps7, .elv = elv7, .way = W1},
	{.br = 172, .refBr = 245, .cGma = gma2p15, .rTbl = RCrtbl172nit, .cTbl = RCctbl172nit, .aid = aid3630, .elvCaps = elvCaps8, .elv = elv8, .way = W1},
	{.br = 183, .refBr = 259, .cGma = gma2p15, .rTbl = RCrtbl183nit, .cTbl = RCctbl183nit, .aid = aid3630, .elvCaps = elvCaps8, .elv = elv8, .way = W1},
	{.br = 195, .refBr = 259, .cGma = gma2p15, .rTbl = RCrtbl195nit, .cTbl = RCctbl195nit, .aid = aid3141, .elvCaps = elvCaps8, .elv = elv8, .way = W1},
	{.br = 207, .refBr = 259, .cGma = gma2p15, .rTbl = RCrtbl207nit, .cTbl = RCctbl207nit, .aid = aid2663, .elvCaps = elvCaps8, .elv = elv8, .way = W1},
	{.br = 220, .refBr = 259, .cGma = gma2p15, .rTbl = RCrtbl220nit, .cTbl = RCctbl220nit, .aid = aid2120, .elvCaps = elvCaps8, .elv = elv8, .way = W1},
	{.br = 234, .refBr = 259, .cGma = gma2p15, .rTbl = RCrtbl234nit, .cTbl = RCctbl234nit, .aid = aid1518, .elvCaps = elvCaps8, .elv = elv8, .way = W1},
	{.br = 249, .refBr = 259, .cGma = gma2p15, .rTbl = RCrtbl249nit, .cTbl = RCctbl249nit, .aid = aid1005, .elvCaps = elvCaps9, .elv = elv9, .way = W1},		// 249 ~ 360 acl off
	{.br = 265, .refBr = 274, .cGma = gma2p15, .rTbl = RCrtbl265nit, .cTbl = RCctbl265nit, .aid = aid1005, .elvCaps = elvCaps10, .elv = elv10, .way = W1},
	{.br = 282, .refBr = 292, .cGma = gma2p15, .rTbl = RCrtbl282nit, .cTbl = RCctbl282nit, .aid = aid1005, .elvCaps = elvCaps10, .elv = elv10, .way = W1},
	{.br = 300, .refBr = 306, .cGma = gma2p15, .rTbl = RCrtbl300nit, .cTbl = RCctbl300nit, .aid = aid1005, .elvCaps = elvCaps11, .elv = elv11, .way = W1},
	{.br = 316, .refBr = 322, .cGma = gma2p15, .rTbl = RCrtbl316nit, .cTbl = RCctbl316nit, .aid = aid1005, .elvCaps = elvCaps12, .elv = elv12, .way = W1},
	{.br = 333, .refBr = 338, .cGma = gma2p15, .rTbl = RCrtbl333nit, .cTbl = RCctbl333nit, .aid = aid1005, .elvCaps = elvCaps12, .elv = elv12, .way = W1},
	{.br = 360, .refBr = 360, .cGma = gma2p20, .rTbl = RCrtbl360nit, .cTbl = RCctbl360nit, .aid = aid1005, .elvCaps = elvCaps13, .elv = elv13, .way = W2},
/*hbm interpolation */
	{.br = 382, .refBr = 382, .cGma = gma2p20, .rTbl = RCrtbl360nit, .cTbl = RCctbl360nit, .aid = aid1005, .elvCaps = elvCaps13, .elv = elv13, .way = W3},  // hbm is acl on
	{.br = 407, .refBr = 407, .cGma = gma2p20, .rTbl = RCrtbl360nit, .cTbl = RCctbl360nit, .aid = aid1005, .elvCaps = elvCaps13, .elv = elv13, .way = W3},  // hbm is acl on
	{.br = 433, .refBr = 433, .cGma = gma2p20, .rTbl = RCrtbl360nit, .cTbl = RCctbl360nit, .aid = aid1005, .elvCaps = elvCaps14, .elv = elv14, .way = W3},  // hbm is acl on
	{.br = 461, .refBr = 461, .cGma = gma2p20, .rTbl = RCrtbl360nit, .cTbl = RCctbl360nit, .aid = aid1005, .elvCaps = elvCaps15, .elv = elv15, .way = W3},  // hbm is acl on
	{.br = 491, .refBr = 491, .cGma = gma2p20, .rTbl = RCrtbl360nit, .cTbl = RCctbl360nit, .aid = aid1005, .elvCaps = elvCaps16, .elv = elv16, .way = W3},  // hbm is acl on
	{.br = 517, .refBr = 517, .cGma = gma2p20, .rTbl = RCrtbl360nit, .cTbl = RCctbl360nit, .aid = aid1005, .elvCaps = elvCaps17, .elv = elv17, .way = W3},  // hbm is acl on
	{.br = 545, .refBr = 545, .cGma = gma2p20, .rTbl = RCrtbl360nit, .cTbl = RCctbl360nit, .aid = aid1005, .elvCaps = elvCaps18, .elv = elv18, .way = W3},  // hbm is acl on
/* hbm */
	{.br = 600, .refBr = 600, .cGma = gma2p20, .rTbl = RCrtbl360nit, .cTbl = RCctbl360nit, .aid = aid1005, .elvCaps = elvCaps19, .elv = elv19, .way = W4}, // hbm is acl on
};





// aid sheet in rev.E opmanual
struct SmtDimInfo dimming_info_RE[MAX_BR_INFO] = {				// add hbm array
	{.br = 2, .refBr = 115, .cGma = gma2p15, .rTbl = RErtbl2nit, .cTbl = REctbl2nit, .aid = aid9825, .elvCaps = elvCaps5, .elv = elv5, .way = W1},
	{.br = 3, .refBr = 115, .cGma = gma2p15, .rTbl = RErtbl3nit, .cTbl = REctbl3nit, .aid = aid9713, .elvCaps = elvCaps4, .elv = elv4, .way = W1},
	{.br = 4, .refBr = 115, .cGma = gma2p15, .rTbl = RErtbl4nit, .cTbl = REctbl4nit, .aid = aid9635, .elvCaps = elvCaps3, .elv = elv3, .way = W1},
	{.br = 5, .refBr = 115, .cGma = gma2p15, .rTbl = RErtbl5nit, .cTbl = REctbl5nit, .aid = aid9523, .elvCaps = elvCaps2, .elv = elv2, .way = W1},
	{.br = 6, .refBr = 115, .cGma = gma2p15, .rTbl = RErtbl6nit, .cTbl = REctbl6nit, .aid = aid9445, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 7, .refBr = 115, .cGma = gma2p15, .rTbl = RErtbl7nit, .cTbl = REctbl7nit, .aid = aid9344, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 8, .refBr = 115, .cGma = gma2p15, .rTbl = RErtbl8nit, .cTbl = REctbl8nit, .aid = aid9270, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 9, .refBr = 115, .cGma = gma2p15, .rTbl = RErtbl9nit, .cTbl = REctbl9nit, .aid = aid9200, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 10, .refBr = 115, .cGma = gma2p15, .rTbl = RErtbl10nit, .cTbl = REctbl10nit, .aid = aid9130, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 11, .refBr = 115, .cGma = gma2p15, .rTbl = RErtbl11nit, .cTbl = REctbl11nit, .aid = aid9030, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 12, .refBr = 115, .cGma = gma2p15, .rTbl = RErtbl12nit, .cTbl = REctbl12nit, .aid = aid8964, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 13, .refBr = 115, .cGma = gma2p15, .rTbl = RErtbl13nit, .cTbl = REctbl13nit, .aid = aid8894, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 14, .refBr = 115, .cGma = gma2p15, .rTbl = RErtbl14nit, .cTbl = REctbl14nit, .aid = aid8832, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 15, .refBr = 115, .cGma = gma2p15, .rTbl = RErtbl15nit, .cTbl = REctbl15nit, .aid = aid8723, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 16, .refBr = 115, .cGma = gma2p15, .rTbl = RErtbl16nit, .cTbl = REctbl16nit, .aid = aid8653, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 17, .refBr = 115, .cGma = gma2p15, .rTbl = RErtbl17nit, .cTbl = REctbl17nit, .aid = aid8575, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 19, .refBr = 115, .cGma = gma2p15, .rTbl = RErtbl19nit, .cTbl = REctbl19nit, .aid = aid8393, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 20, .refBr = 115, .cGma = gma2p15, .rTbl = RErtbl20nit, .cTbl = REctbl20nit, .aid = aid8284, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 21, .refBr = 115, .cGma = gma2p15, .rTbl = RErtbl21nit, .cTbl = REctbl21nit, .aid = aid8218, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 22, .refBr = 115, .cGma = gma2p15, .rTbl = RErtbl22nit, .cTbl = REctbl22nit, .aid = aid8148, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 24, .refBr = 115, .cGma = gma2p15, .rTbl = RErtbl24nit, .cTbl = REctbl24nit, .aid = aid7974, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 25, .refBr = 115, .cGma = gma2p15, .rTbl = RErtbl25nit, .cTbl = REctbl25nit, .aid = aid7896, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 27, .refBr = 115, .cGma = gma2p15, .rTbl = RErtbl27nit, .cTbl = REctbl27nit, .aid = aid7717, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 29, .refBr = 115, .cGma = gma2p15, .rTbl = RErtbl29nit, .cTbl = REctbl29nit, .aid = aid7535, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 30, .refBr = 115, .cGma = gma2p15, .rTbl = RErtbl30nit, .cTbl = REctbl30nit, .aid = aid7469, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 32, .refBr = 115, .cGma = gma2p15, .rTbl = RErtbl32nit, .cTbl = REctbl32nit, .aid = aid7290, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 34, .refBr = 115, .cGma = gma2p15, .rTbl = RErtbl34nit, .cTbl = REctbl34nit, .aid = aid7104, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 37, .refBr = 115, .cGma = gma2p15, .rTbl = RErtbl37nit, .cTbl = REctbl37nit, .aid = aid6848, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 39, .refBr = 115, .cGma = gma2p15, .rTbl = RErtbl39nit, .cTbl = REctbl39nit, .aid = aid6669, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 41, .refBr = 115, .cGma = gma2p15, .rTbl = RErtbl41nit, .cTbl = REctbl41nit, .aid = aid6491, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 44, .refBr = 115, .cGma = gma2p15, .rTbl = RErtbl44nit, .cTbl = REctbl44nit, .aid = aid6231, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 47, .refBr = 115, .cGma = gma2p15, .rTbl = RErtbl47nit, .cTbl = REctbl47nit, .aid = aid5947, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 50, .refBr = 115, .cGma = gma2p15, .rTbl = RErtbl50nit, .cTbl = REctbl50nit, .aid = aid5675, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 53, .refBr = 115, .cGma = gma2p15, .rTbl = RErtbl53nit, .cTbl = REctbl53nit, .aid = aid5419, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 56, .refBr = 115, .cGma = gma2p15, .rTbl = RErtbl56nit, .cTbl = REctbl56nit, .aid = aid5140, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 60, .refBr = 115, .cGma = gma2p15, .rTbl = RErtbl60nit, .cTbl = REctbl60nit, .aid = aid4752, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 64, .refBr = 115, .cGma = gma2p15, .rTbl = RErtbl64nit, .cTbl = REctbl64nit, .aid = aid4383, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 68, .refBr = 115, .cGma = gma2p15, .rTbl = RErtbl68nit, .cTbl = REctbl68nit, .aid = aid4006, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 72, .refBr = 115, .cGma = gma2p15, .rTbl = RErtbl72nit, .cTbl = REctbl72nit, .aid = aid3622, .elvCaps = elvCaps1, .elv = elv1, .way = W1},
	{.br = 77, .refBr = 122, .cGma = gma2p15, .rTbl = RErtbl77nit, .cTbl = REctbl77nit, .aid = aid3622, .elvCaps = elvCaps2, .elv = elv2, .way = W1},
	{.br = 82, .refBr = 129, .cGma = gma2p15, .rTbl = RErtbl82nit, .cTbl = REctbl82nit, .aid = aid3622, .elvCaps = elvCaps2, .elv = elv2, .way = W1},
	{.br = 87, .refBr = 136, .cGma = gma2p15, .rTbl = RErtbl87nit, .cTbl = REctbl87nit, .aid = aid3622, .elvCaps = elvCaps2, .elv = elv2, .way = W1},
	{.br = 93, .refBr = 145, .cGma = gma2p15, .rTbl = RErtbl93nit, .cTbl = REctbl93nit, .aid = aid3622, .elvCaps = elvCaps3, .elv = elv3, .way = W1},
	{.br = 98, .refBr = 151, .cGma = gma2p15, .rTbl = RErtbl98nit, .cTbl = REctbl98nit, .aid = aid3622, .elvCaps = elvCaps3, .elv = elv3, .way = W1},
	{.br = 105, .refBr = 161, .cGma = gma2p15, .rTbl = RErtbl105nit, .cTbl = REctbl105nit, .aid = aid3622, .elvCaps = elvCaps3, .elv = elv3, .way = W1},
	{.br = 111, .refBr = 167, .cGma = gma2p15, .rTbl = RErtbl111nit, .cTbl = REctbl111nit, .aid = aid3622, .elvCaps = elvCaps4, .elv = elv4, .way = W1},
	{.br = 119, .refBr = 180, .cGma = gma2p15, .rTbl = RErtbl119nit, .cTbl = REctbl119nit, .aid = aid3622, .elvCaps = elvCaps4, .elv = elv4, .way = W1},
	{.br = 126, .refBr = 189, .cGma = gma2p15, .rTbl = RErtbl126nit, .cTbl = REctbl126nit, .aid = aid3622, .elvCaps = elvCaps5, .elv = elv5, .way = W1},
	{.br = 134, .refBr = 201, .cGma = gma2p15, .rTbl = RErtbl134nit, .cTbl = REctbl134nit, .aid = aid3622, .elvCaps = elvCaps5, .elv = elv5, .way = W1},
	{.br = 143, .refBr = 210, .cGma = gma2p15, .rTbl = RErtbl143nit, .cTbl = REctbl143nit, .aid = aid3622, .elvCaps = elvCaps6, .elv = elv6, .way = W1},
	{.br = 152, .refBr = 220, .cGma = gma2p15, .rTbl = RErtbl152nit, .cTbl = REctbl152nit, .aid = aid3622, .elvCaps = elvCaps7, .elv = elv7, .way = W1},
	{.br = 162, .refBr = 236, .cGma = gma2p15, .rTbl = RErtbl162nit, .cTbl = REctbl162nit, .aid = aid3622, .elvCaps = elvCaps7, .elv = elv7, .way = W1},
	{.br = 172, .refBr = 244, .cGma = gma2p15, .rTbl = RErtbl172nit, .cTbl = REctbl172nit, .aid = aid3622, .elvCaps = elvCaps8, .elv = elv8, .way = W1},
	{.br = 183, .refBr = 260, .cGma = gma2p15, .rTbl = RErtbl183nit, .cTbl = REctbl183nit, .aid = aid3622, .elvCaps = elvCaps8, .elv = elv8, .way = W1},
	{.br = 195, .refBr = 260, .cGma = gma2p15, .rTbl = RErtbl195nit, .cTbl = REctbl195nit, .aid = aid3133, .elvCaps = elvCaps8, .elv = elv8, .way = W1},
	{.br = 207, .refBr = 260, .cGma = gma2p15, .rTbl = RErtbl207nit, .cTbl = REctbl207nit, .aid = aid2620, .elvCaps = elvCaps8, .elv = elv8, .way = W1},
	{.br = 220, .refBr = 260, .cGma = gma2p15, .rTbl = RErtbl220nit, .cTbl = REctbl220nit, .aid = aid2158, .elvCaps = elvCaps8, .elv = elv8, .way = W1},
	{.br = 234, .refBr = 260, .cGma = gma2p15, .rTbl = RErtbl234nit, .cTbl = REctbl234nit, .aid = aid1611, .elvCaps = elvCaps8, .elv = elv8, .way = W1},
	{.br = 249, .refBr = 260, .cGma = gma2p15, .rTbl = RErtbl249nit, .cTbl = REctbl249nit, .aid = aid1005, .elvCaps = elvCaps9, .elv = elv9, .way = W1},		// 249 ~ 360 acl off
	{.br = 265, .refBr = 273, .cGma = gma2p15, .rTbl = RErtbl265nit, .cTbl = REctbl265nit, .aid = aid1005, .elvCaps = elvCaps10, .elv = elv10, .way = W1},
	{.br = 282, .refBr = 290, .cGma = gma2p15, .rTbl = RErtbl282nit, .cTbl = REctbl282nit, .aid = aid1005, .elvCaps = elvCaps10, .elv = elv10, .way = W1},
	{.br = 300, .refBr = 306, .cGma = gma2p15, .rTbl = RErtbl300nit, .cTbl = REctbl300nit, .aid = aid1005, .elvCaps = elvCaps11, .elv = elv11, .way = W1},
	{.br = 316, .refBr = 318, .cGma = gma2p15, .rTbl = RErtbl316nit, .cTbl = REctbl316nit, .aid = aid1005, .elvCaps = elvCaps12, .elv = elv12, .way = W1},
	{.br = 333, .refBr = 338, .cGma = gma2p15, .rTbl = RErtbl333nit, .cTbl = REctbl333nit, .aid = aid1005, .elvCaps = elvCaps12, .elv = elv12, .way = W1},
	{.br = 360, .refBr = 360, .cGma = gma2p20, .rTbl = RErtbl360nit, .cTbl = REctbl360nit, .aid = aid1005, .elvCaps = elvCaps13, .elv = elv13, .way = W2},
/*hbm interpolation */
	{.br = 382, .refBr = 382, .cGma = gma2p20, .rTbl = RErtbl360nit, .cTbl = REctbl360nit, .aid = aid1005, .elvCaps = elvCaps13, .elv = elv13, .way = W3},  // hbm is acl on
	{.br = 407, .refBr = 407, .cGma = gma2p20, .rTbl = RErtbl360nit, .cTbl = REctbl360nit, .aid = aid1005, .elvCaps = elvCaps13, .elv = elv13, .way = W3},  // hbm is acl on
	{.br = 433, .refBr = 433, .cGma = gma2p20, .rTbl = RErtbl360nit, .cTbl = REctbl360nit, .aid = aid1005, .elvCaps = elvCaps14, .elv = elv14, .way = W3},  // hbm is acl on
	{.br = 461, .refBr = 461, .cGma = gma2p20, .rTbl = RErtbl360nit, .cTbl = REctbl360nit, .aid = aid1005, .elvCaps = elvCaps15, .elv = elv15, .way = W3},  // hbm is acl on
	{.br = 491, .refBr = 491, .cGma = gma2p20, .rTbl = RErtbl360nit, .cTbl = REctbl360nit, .aid = aid1005, .elvCaps = elvCaps16, .elv = elv16, .way = W3},  // hbm is acl on
	{.br = 517, .refBr = 517, .cGma = gma2p20, .rTbl = RErtbl360nit, .cTbl = REctbl360nit, .aid = aid1005, .elvCaps = elvCaps17, .elv = elv17, .way = W3},  // hbm is acl on
	{.br = 545, .refBr = 545, .cGma = gma2p20, .rTbl = RErtbl360nit, .cTbl = REctbl360nit, .aid = aid1005, .elvCaps = elvCaps18, .elv = elv18, .way = W3},  // hbm is acl on
/* hbm */
	{.br = 600, .refBr = 600, .cGma = gma2p20, .rTbl = RErtbl360nit, .cTbl = REctbl360nit, .aid = aid1005, .elvCaps = elvCaps19, .elv = elv19, .way = W4}, // hbm is acl on
};


static int set_gamma_to_center(struct SmtDimInfo *brInfo)
{
	int i, j;
	int ret = 0;
	unsigned int index = 0;
	unsigned char *result = brInfo->gamma;

	result[index++] = OLED_CMD_GAMMA;

	for (i = V255; i >= V0; i--) {
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

static int gammaToVolt255(int gamma)
{
	int ret;

	if (gamma > vreg_element_max[V255]) {
		dsim_err("%s : gamma overflow : %d\n", __FUNCTION__, gamma);
		gamma = vreg_element_max[V255];
	}
	if (gamma < 0) {
		dsim_err("%s : gamma undeflow : %d\n", __FUNCTION__, gamma);
		gamma = 0;
	}

	ret = (int)v255_trans_volt[gamma];

	return ret;
}

static int voltToGamma(int hbm_volt_table[][3], int* vt, int tp, int color)
{
	int ret;
	int t1, t2;
	unsigned long temp;

	if(tp == V3)
	{
		t1 = DOUBLE_MULTIPLE_VREGOUT - hbm_volt_table[V3][color];
		t2 = DOUBLE_MULTIPLE_VREGOUT - hbm_volt_table[V11][color];
	}
	else
	{
		t1 = vt[color] - hbm_volt_table[tp][color];
		t2 = vt[color] - hbm_volt_table[tp + 1][color];
	}

	temp = ((unsigned long)t1 * (unsigned long)fix_const[tp].de) / (unsigned long)t2;
	ret = temp - fix_const[tp].nu;

	return ret;

}

static int set_gamma_to_hbm(struct SmtDimInfo *brInfo, struct dim_data *dimData, u8 *hbm)
{
	int ret = 0;
	unsigned int index = 0;
	unsigned char *result = brInfo->gamma;
	int i, j, idx;
	int temp = 0;
	int voltTableHbm[NUM_VREF][CI_MAX];

	memset(result, 0, OLED_CMD_GAMMA_CNT);

	result[index++] = OLED_CMD_GAMMA;

	memcpy(result+1, hbm, S6E3HA2_HBMGAMMA_LEN);


	memcpy(voltTableHbm[V0], dimData->volt[0], sizeof(voltTableHbm[V0]));
	memcpy(voltTableHbm[V3], dimData->volt[1], sizeof(voltTableHbm[V3]));
	memcpy(voltTableHbm[V11], dimData->volt[10], sizeof(voltTableHbm[V11]));
	memcpy(voltTableHbm[V23], dimData->volt[26], sizeof(voltTableHbm[V23]));
	memcpy(voltTableHbm[V35], dimData->volt[40], sizeof(voltTableHbm[V35]));
	memcpy(voltTableHbm[V51], dimData->volt[56], sizeof(voltTableHbm[V51]));
	memcpy(voltTableHbm[V87], dimData->volt[100], sizeof(voltTableHbm[V87]));
	memcpy(voltTableHbm[V151], dimData->volt[173], sizeof(voltTableHbm[V151]));
	memcpy(voltTableHbm[V203], dimData->volt[233], sizeof(voltTableHbm[V203]));


	idx = 0;
	for(i = CI_RED; i < CI_MAX; i++, idx += 2) {
		temp = (hbm[idx] << 8) | (hbm[idx + 1]);
		voltTableHbm[V255][i] = gammaToVolt255(temp + dimData->mtp[V255][i]);
	}

	idx = 1;
	for (i = V255; i >= V0; i--) {
		for (j = 0; j < CI_MAX; j++) {
			if (i == V255) {
				idx += 2;
			} else if(i == V0) {
				idx++;
			} else {
				temp = voltToGamma(voltTableHbm, dimData->volt_vt, i, j) - dimData->mtp[i][j];
				if(temp <= 0)
					temp = 0;
				result[idx] = temp;
				idx ++;
			}
		}
	}

	dsim_info("============ TUNE HBM GAMMA ========== : \n");
	for (i= 0; i < S6E3HA2_HBMGAMMA_LEN; i ++) {
		dsim_info("HBM GAMMA[%d] : %x\n", i, result[i]);
	}
	return ret;
}

/* gamma interpolaion table */
const unsigned int tbl_hbm_inter[7] = {
	94, 201, 311, 431, 559, 670, 789
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
		if (dimInfo[i].br == S6E3HA2_MAX_BRIGHTNESS)
			ref_idx = i;

		if (dimInfo[i].br == S6E3HA2_HBM_BRIGHTNESS)
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

	for (i = V255; i >= V0; i--) {
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
	unsigned char panelrev = 0x00;
	struct panel_private *panel = &dsim->priv;
	struct SmtDimInfo *diminfo = NULL;
	int string_offset;
	char string_buf[1024];

	dimming = (struct dim_data *)kmalloc(sizeof(struct dim_data), GFP_KERNEL);
	if (!dimming) {
		dsim_err("failed to allocate memory for dim data\n");
		ret = -ENOMEM;
		goto error;
	}

	panelrev = panel->id[2] & 0x0F;
	dsim_info("%s : Panel rev : %d\n", __func__, panelrev);

	switch(panelrev) {
	case 0:					// prim
	case 1:                 // A/B
	case 2:					// C/D
		dsim_info("%s init dimming info for daisy rev.A,B,C,D panel\n", __func__);
		diminfo = (void *)dimming_info_RC;
		break;
	default:			// REV E ~
		dsim_info("%s init dimming info for daisy rev.E panel\n", __func__);
		diminfo = (void *)dimming_info_RE;
		break;
	}

	panel->dim_data= (void *)dimming;
	panel->dim_info = (void *)diminfo;
	panel->br_tbl = (unsigned int *)br_tbl;
	panel->hbm_inter_br_tbl = (unsigned int *)hbm_interpolation_br_tbl;
	panel->gallery_br_tbl = (unsigned int*)gallery_br_tbl;
	panel->hbm_tbl = (unsigned char **)HBM_TABLE;
	panel->acl_cutoff_tbl = (unsigned char **)ACL_CUTOFF_TABLE;
	panel->acl_opr_tbl = (unsigned char **)ACL_OPR_TABLE;
	panel->hbm_index = MAX_BR_INFO - 1;
	panel->hbm_elvss_comp = S6E3HA2_HBM_ELVSS_COMP;

	for (j = 0; j < CI_MAX; j++) {
		temp = ((mtp[pos] & 0x01) ? -1 : 1) * mtp[pos+1];
		dimming->t_gamma[V255][j] = (int)center_gamma[V255][j] + temp;
		dimming->mtp[V255][j] = temp;
		pos += 2;
	}

	for (i = V203; i >= V0; i--) {
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
	for(i=0;i<VMAX;i++) {
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
	for(i=0;i<VMAX;i++) {
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
#ifdef CONFIG_LCD_HMT
static const unsigned int hmt_br_tbl [256] = {
	10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
	10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 11, 11, 11, 12, 12,
	13, 13, 14, 14, 14, 15, 15, 16, 16, 16, 17, 17, 17, 17, 17, 19,
	19, 20, 20, 21, 21, 21, 22, 22, 23, 23, 23, 23, 23, 25, 25, 25,
	25, 25, 27, 27, 27, 27, 27, 29, 29, 29, 29, 29, 31, 31, 31, 31,
	31, 33, 33, 33, 33, 35, 35, 35, 35, 35, 37, 37, 37, 37, 37, 39,
	39, 39, 39, 39, 41, 41, 41, 41, 41, 41, 41, 44, 44, 44, 44, 44,
	44, 44, 44, 47, 47, 47, 47, 47, 47, 47, 50, 50, 50, 50, 50, 50,
	50, 53, 53, 53, 53, 53, 53, 53, 56, 56, 56, 56, 56, 56, 56, 56,
	56, 56, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 68, 68, 68, 68, 68, 68, 68, 68, 68, 72,
	72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 77, 77, 77, 77, 77,
	77, 77, 77, 77, 77, 77, 77, 77, 82, 82, 82, 82, 82, 82, 82, 82,
	82, 82, 82, 82, 87, 87, 87, 87, 87, 87, 87, 87, 87, 87, 87, 87,
	87, 87, 93, 93, 93, 93, 93, 93, 93, 93, 93, 93, 93, 93, 93, 93,
	93, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 105
};

struct SmtDimInfo hmt_dimming_info[HMT_MAX_BR_INFO] = {
	{.br = 10, .refBr = 43, .cGma = gma2p15, .rTbl = HMTrtbl10nit, .cTbl = HMTctbl10nit, .aid = HMTaid8001, .elvCaps = HMTelvCaps, .elv = HMTelv},
	{.br = 11, .refBr = 48, .cGma = gma2p15, .rTbl = HMTrtbl11nit, .cTbl = HMTctbl11nit, .aid = HMTaid8001, .elvCaps = HMTelvCaps, .elv = HMTelv},
	{.br = 12, .refBr = 51, .cGma = gma2p15, .rTbl = HMTrtbl12nit, .cTbl = HMTctbl12nit, .aid = HMTaid8001, .elvCaps = HMTelvCaps, .elv = HMTelv},
	{.br = 13, .refBr = 55, .cGma = gma2p15, .rTbl = HMTrtbl13nit, .cTbl = HMTctbl13nit, .aid = HMTaid8001, .elvCaps = HMTelvCaps, .elv = HMTelv},
	{.br = 14, .refBr = 60, .cGma = gma2p15, .rTbl = HMTrtbl14nit, .cTbl = HMTctbl14nit, .aid = HMTaid8001, .elvCaps = HMTelvCaps, .elv = HMTelv},
	{.br = 15, .refBr = 61, .cGma = gma2p15, .rTbl = HMTrtbl15nit, .cTbl = HMTctbl15nit, .aid = HMTaid8001, .elvCaps = HMTelvCaps, .elv = HMTelv},
	{.br = 16, .refBr = 64, .cGma = gma2p15, .rTbl = HMTrtbl16nit, .cTbl = HMTctbl16nit, .aid = HMTaid8001, .elvCaps = HMTelvCaps, .elv = HMTelv},
	{.br = 17, .refBr = 68, .cGma = gma2p15, .rTbl = HMTrtbl17nit, .cTbl = HMTctbl17nit, .aid = HMTaid8001, .elvCaps = HMTelvCaps, .elv = HMTelv},
	{.br = 19, .refBr = 75, .cGma = gma2p15, .rTbl = HMTrtbl19nit, .cTbl = HMTctbl19nit, .aid = HMTaid8001, .elvCaps = HMTelvCaps, .elv = HMTelv},
	{.br = 20, .refBr = 78, .cGma = gma2p15, .rTbl = HMTrtbl20nit, .cTbl = HMTctbl20nit, .aid = HMTaid8001, .elvCaps = HMTelvCaps, .elv = HMTelv},
	{.br = 21, .refBr = 80, .cGma = gma2p15, .rTbl = HMTrtbl21nit, .cTbl = HMTctbl21nit, .aid = HMTaid8001, .elvCaps = HMTelvCaps, .elv = HMTelv},
	{.br = 22, .refBr = 86, .cGma = gma2p15, .rTbl = HMTrtbl22nit, .cTbl = HMTctbl22nit, .aid = HMTaid8001, .elvCaps = HMTelvCaps, .elv = HMTelv},
	{.br = 23, .refBr = 89, .cGma = gma2p15, .rTbl = HMTrtbl23nit, .cTbl = HMTctbl23nit, .aid = HMTaid8001, .elvCaps = HMTelvCaps, .elv = HMTelv},
	{.br = 25, .refBr = 96, .cGma = gma2p15, .rTbl = HMTrtbl25nit, .cTbl = HMTctbl25nit, .aid = HMTaid8001, .elvCaps = HMTelvCaps, .elv = HMTelv},
	{.br = 27, .refBr = 103, .cGma = gma2p15, .rTbl = HMTrtbl27nit, .cTbl = HMTctbl27nit, .aid = HMTaid8001, .elvCaps = HMTelvCaps, .elv = HMTelv},
	{.br = 29, .refBr = 108, .cGma = gma2p15, .rTbl = HMTrtbl29nit, .cTbl = HMTctbl29nit, .aid = HMTaid8001, .elvCaps = HMTelvCaps, .elv = HMTelv},
	{.br = 31, .refBr = 114, .cGma = gma2p15, .rTbl = HMTrtbl31nit, .cTbl = HMTctbl31nit, .aid = HMTaid8001, .elvCaps = HMTelvCaps, .elv = HMTelv},
	{.br = 33, .refBr = 121, .cGma = gma2p15, .rTbl = HMTrtbl33nit, .cTbl = HMTctbl33nit, .aid = HMTaid8001, .elvCaps = HMTelvCaps, .elv = HMTelv},
	{.br = 35, .refBr = 129, .cGma = gma2p15, .rTbl = HMTrtbl35nit, .cTbl = HMTctbl35nit, .aid = HMTaid8001, .elvCaps = HMTelvCaps, .elv = HMTelv},
	{.br = 37, .refBr = 137, .cGma = gma2p15, .rTbl = HMTrtbl37nit, .cTbl = HMTctbl37nit, .aid = HMTaid8001, .elvCaps = HMTelvCaps, .elv = HMTelv},
	{.br = 39, .refBr = 144, .cGma = gma2p15, .rTbl = HMTrtbl39nit, .cTbl = HMTctbl39nit, .aid = HMTaid8001, .elvCaps = HMTelvCaps, .elv = HMTelv},
	{.br = 41, .refBr = 146, .cGma = gma2p15, .rTbl = HMTrtbl41nit, .cTbl = HMTctbl41nit, .aid = HMTaid8001, .elvCaps = HMTelvCaps, .elv = HMTelv},
	{.br = 44, .refBr = 155, .cGma = gma2p15, .rTbl = HMTrtbl44nit, .cTbl = HMTctbl44nit, .aid = HMTaid8001, .elvCaps = HMTelvCaps, .elv = HMTelv},
	{.br = 47, .refBr = 166, .cGma = gma2p15, .rTbl = HMTrtbl47nit, .cTbl = HMTctbl47nit, .aid = HMTaid8001, .elvCaps = HMTelvCaps, .elv = HMTelv},
	{.br = 50, .refBr = 175, .cGma = gma2p15, .rTbl = HMTrtbl50nit, .cTbl = HMTctbl50nit, .aid = HMTaid8001, .elvCaps = HMTelvCaps, .elv = HMTelv},
	{.br = 53, .refBr = 186, .cGma = gma2p15, .rTbl = HMTrtbl53nit, .cTbl = HMTctbl53nit, .aid = HMTaid8001, .elvCaps = HMTelvCaps, .elv = HMTelv},
	{.br = 56, .refBr = 198, .cGma = gma2p15, .rTbl = HMTrtbl56nit, .cTbl = HMTctbl56nit, .aid = HMTaid8001, .elvCaps = HMTelvCaps, .elv = HMTelv},
	{.br = 60, .refBr = 211, .cGma = gma2p15, .rTbl = HMTrtbl60nit, .cTbl = HMTctbl60nit, .aid = HMTaid8001, .elvCaps = HMTelvCaps, .elv = HMTelv},
	{.br = 64, .refBr = 223, .cGma = gma2p15, .rTbl = HMTrtbl64nit, .cTbl = HMTctbl64nit, .aid = HMTaid8001, .elvCaps = HMTelvCaps, .elv = HMTelv},
	{.br = 68, .refBr = 237, .cGma = gma2p15, .rTbl = HMTrtbl68nit, .cTbl = HMTctbl68nit, .aid = HMTaid8001, .elvCaps = HMTelvCaps, .elv = HMTelv},
	{.br = 72, .refBr = 250, .cGma = gma2p15, .rTbl = HMTrtbl72nit, .cTbl = HMTctbl72nit, .aid = HMTaid8001, .elvCaps = HMTelvCaps, .elv = HMTelv},
	{.br = 77, .refBr = 189, .cGma = gma2p15, .rTbl = HMTrtbl77nit, .cTbl = HMTctbl77nit, .aid = HMTaid7003, .elvCaps = HMTelvCaps, .elv = HMTelv},
	{.br = 82, .refBr = 202, .cGma = gma2p15, .rTbl = HMTrtbl82nit, .cTbl = HMTctbl82nit, .aid = HMTaid7003, .elvCaps = HMTelvCaps, .elv = HMTelv},
	{.br = 87, .refBr = 213, .cGma = gma2p15, .rTbl = HMTrtbl87nit, .cTbl = HMTctbl87nit, .aid = HMTaid7003, .elvCaps = HMTelvCaps, .elv = HMTelv},
	{.br = 93, .refBr = 226, .cGma = gma2p15, .rTbl = HMTrtbl93nit, .cTbl = HMTctbl93nit, .aid = HMTaid7003, .elvCaps = HMTelvCaps, .elv = HMTelv},
	{.br = 99, .refBr = 238, .cGma = gma2p15, .rTbl = HMTrtbl99nit, .cTbl = HMTctbl99nit, .aid = HMTaid7003, .elvCaps = HMTelvCaps, .elv = HMTelv},
	{.br = 105, .refBr = 251, .cGma = gma2p15, .rTbl = HMTrtbl105nit, .cTbl = HMTctbl105nit, .aid = HMTaid7003, .elvCaps = HMTelvCaps, .elv = HMTelv},
};

static int hmt_init_dimming(struct dsim_device *dsim, u8 *mtp)
{
	int i, j;
	int pos = 0;
	int ret = 0;
	short temp;
	struct dim_data *dimming;
	struct panel_private *panel = &dsim->priv;
	struct SmtDimInfo *diminfo = NULL;

	dimming = (struct dim_data *)kmalloc(sizeof(struct dim_data), GFP_KERNEL);
	if (!dimming) {
		dsim_err("failed to allocate memory for dim data\n");
		ret = -ENOMEM;
		goto error;
	}

	diminfo = hmt_dimming_info;

	panel->hmt_dim_data= (void *)dimming;
	panel->hmt_dim_info = (void *)diminfo;

	panel->hmt_br_tbl = (unsigned int *)hmt_br_tbl;

	for (j = 0; j < CI_MAX; j++) {
		temp = ((mtp[pos] & 0x01) ? -1 : 1) * mtp[pos+1];
		dimming->t_gamma[V255][j] = (int)center_gamma[V255][j] + temp;
		dimming->mtp[V255][j] = temp;
		pos += 2;
	}

	for (i = V203; i >= V0; i--) {
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

	ret = generate_volt_table(dimming);
	if (ret) {
		dimm_err("[ERR:%s] failed to generate volt table\n", __func__);
		goto error;
	}

	for (i = 0; i < HMT_MAX_BR_INFO; i++) {
		ret = cal_gamma_from_index(dimming, &diminfo[i]);
		if (ret) {
			dsim_err("failed to calculate gamma : index : %d\n", i);
			goto error;
		}
	}
error:
	return ret;

}

#endif

#endif




static int s6e3ha2_read_init_info(struct dsim_device *dsim, unsigned char *mtp, unsigned char* hbm)
{
	int i = 0;
	int ret = 0;
	struct panel_private *panel = &dsim->priv;
	unsigned char buf[S6E3HA2_MTP_DATE_SIZE] = {0, };
	unsigned char bufForCoordi[S6E3HA2_COORDINATE_LEN] = {0,};
	unsigned char hbm_gamma[S6E3HA2_HBMGAMMA_LEN + 1] = {0, };

	ret = dsim_write_hl_data(dsim, S6E3HA2_SEQ_TEST_KEY_ON_F0, ARRAY_SIZE(S6E3HA2_SEQ_TEST_KEY_ON_F0));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : SEQ_TEST_KEY_ON_F0\n", __func__);
	}

	ret = dsim_read_hl_data(dsim, S6E3HA2_ID_REG, S6E3HA2_ID_LEN, dsim->priv.id);
	if (ret != S6E3HA2_ID_LEN) {
		dsim_err("%s : can't find connected panel. check panel connection\n", __func__);
		panel->lcdConnected = PANEL_DISCONNEDTED;
		goto read_exit;
	}

	dsim_info("READ ID : ");
	for(i = 0; i < S6E3HA2_ID_LEN; i++)
		dsim_info("%02x, ", dsim->priv.id[i]);
	dsim_info("\n");

	ret = dsim_read_hl_data(dsim, S6E3HA2_MTP_ADDR, S6E3HA2_MTP_DATE_SIZE, buf);
	if (ret != S6E3HA2_MTP_DATE_SIZE) {
		dsim_err("failed to read mtp, check panel connection\n");
		goto read_fail;
	}
	memcpy(mtp, buf, S6E3HA2_MTP_SIZE);
	memcpy(dsim->priv.date, &buf[40], ARRAY_SIZE(dsim->priv.date));
	dsim_info("READ MTP SIZE : %d\n", S6E3HA2_MTP_SIZE);
	dsim_info("=========== MTP INFO =========== \n");
	for (i = 0; i < S6E3HA2_MTP_SIZE; i++)
		dsim_info("MTP[%2d] : %2d : %2x\n", i, mtp[i], mtp[i]);

	// coordinate
	ret = dsim_read_hl_data(dsim, S6E3HA2_COORDINATE_REG, S6E3HA2_COORDINATE_LEN, bufForCoordi);
	if (ret != S6E3HA2_COORDINATE_LEN) {
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
	ret = dsim_read_hl_data(dsim, S6E3HA2_CODE_REG, S6E3HA2_CODE_LEN, dsim->priv.code);
	if (ret != S6E3HA2_CODE_LEN) {
		dsim_err("fail to read code on command.\n");
		goto read_fail;
	}
	dsim_info("READ code : ");
	for(i = 0; i < S6E3HA2_CODE_LEN; i++)
		dsim_info("%x, ", dsim->priv.code[i]);
	dsim_info("\n");

	// tset
	ret = dsim_read_hl_data(dsim, TSET_REG, TSET_LEN - 1, dsim->priv.tset);
	if (ret < TSET_LEN - 1) {
		dsim_err("fail to read code on command.\n");
		goto read_fail;
	}
	dsim_info("READ tset : ");
	for(i = 0; i < TSET_LEN - 1; i++)
		dsim_info("%x, ", dsim->priv.tset[i]);
	dsim_info("\n");


	// elvss
	ret = dsim_read_hl_data(dsim, ELVSS_REG, ELVSS_LEN - 1, dsim->priv.elvss_set);
	if (ret < ELVSS_LEN - 1) {
		dsim_err("fail to read elvss on command.\n");
		goto read_fail;
	}
	dsim_info("READ elvss : ");
	for(i = 0; i < ELVSS_LEN - 1; i++)
		dsim_info("%x \n", dsim->priv.elvss_set[i]);

/* read hbm elvss for hbm interpolation */
	panel->hbm_elvss = dsim->priv.elvss_set[S6E3HA2_HBM_ELVSS_INDEX];

	ret = dsim_read_hl_data(dsim, S6E3HA2_HBMGAMMA_REG, S6E3HA2_HBMGAMMA_LEN + 1, hbm_gamma);
	if (ret != S6E3HA2_HBMGAMMA_LEN + 1) {
		dsim_err("fail to read elvss on command.\n");
		goto read_fail;
	}
	dsim_info("HBM Gamma : ");
	memcpy(hbm, hbm_gamma + 1, S6E3HA2_HBMGAMMA_LEN);

	for(i = 1; i < S6E3HA2_HBMGAMMA_LEN + 1; i++)
		dsim_info("hbm gamma[%d] : %x\n", i, hbm_gamma[i]);
	ret = dsim_write_hl_data(dsim, S6E3HA2_SEQ_TEST_KEY_ON_F0, ARRAY_SIZE(S6E3HA2_SEQ_TEST_KEY_ON_F0));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : SEQ_TEST_KEY_ON_F0\n", __func__);
		goto read_exit;
	}
	ret = 0;

read_exit:
	return 0;

read_fail:
	return -ENODEV;
}
static int s6e3ha2_wqhd_dump(struct dsim_device *dsim)
{
	int ret = 0;
	int i;
	unsigned char id[S6E3HA2_ID_LEN];
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

	ret = dsim_read_hl_data(dsim, S6E3HA2_RDDPM_ADDR, 3, rddpm);
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

	ret = dsim_read_hl_data(dsim, S6E3HA2_RDDSM_ADDR, 3, rddsm);
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

	// id
	ret = dsim_read_hl_data(dsim, S6E3HA2_ID_REG, S6E3HA2_ID_LEN, id);
	if (ret != S6E3HA2_ID_LEN) {
		dsim_err("%s : can't read panel id\n",__func__);
		goto dump_exit;
	}

	dsim_info("READ ID : ");
	for(i = 0; i < S6E3HA2_ID_LEN; i++)
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
	dsim_info(" - %s\n", __func__);
	return ret;

}
static int s6e3ha2_wqhd_probe(struct dsim_device *dsim)
{
	int ret = 0;
	struct panel_private *panel = &dsim->priv;
	unsigned char mtp[S6E3HA2_MTP_SIZE] = {0, };
	unsigned char hbm[S6E3HA2_HBMGAMMA_LEN] = {0, };
	panel->dim_data = (void *)NULL;
	panel->lcdConnected = PANEL_CONNECTED;

	dsim_info(" +  : %s\n", __func__);

	ret = s6e3ha2_read_init_info(dsim, mtp, hbm);
	if (panel->lcdConnected == PANEL_DISCONNEDTED) {
		dsim_err("dsim : %s lcd was not connected\n", __func__);
		goto probe_exit;
	}

#ifdef CONFIG_PANEL_AID_DIMMING
	ret = init_dimming(dsim, mtp, hbm);
	if (ret) {
		dsim_err("%s : failed to generate gamma tablen\n", __func__);
	}
#endif
#ifdef CONFIG_LCD_HMT
	ret = hmt_init_dimming(dsim, mtp);
	if(ret) {
		dsim_err("%s : failed to generate gamma tablen\n", __func__);
	}
#endif
#ifdef CONFIG_EXYNOS_DECON_MDNIE_LITE
	panel->mdnie_support = 0;
	if(panel->panel_type != LCD_TYPE_S6E3HA0_WQXGA)
		panel->mdnie_support = 1;
#endif

probe_exit:
	return ret;

}


static int s6e3ha2_wqhd_displayon(struct dsim_device *dsim)
{
	int ret = 0;

	dsim_info("MDD : %s was called\n", __func__);

	ret = dsim_write_hl_data(dsim, S6E3HA2_SEQ_DISPLAY_ON, ARRAY_SIZE(S6E3HA2_SEQ_DISPLAY_ON));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : DISPLAY_ON\n", __func__);
 		goto displayon_err;
	}

displayon_err:
	return ret;

}

static int s6e3ha2_wqhd_exit(struct dsim_device *dsim)
{
	int ret = 0;

	dsim_info("MDD : %s was called\n", __func__);

	ret = dsim_write_hl_data(dsim, S6E3HA2_SEQ_DISPLAY_OFF, ARRAY_SIZE(S6E3HA2_SEQ_DISPLAY_OFF));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : DISPLAY_OFF\n", __func__);
		goto exit_err;
	}

	ret = dsim_write_hl_data(dsim, S6E3HA2_SEQ_SLEEP_IN, ARRAY_SIZE(S6E3HA2_SEQ_SLEEP_IN));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : SLEEP_IN\n", __func__);
		goto exit_err;
	}

	msleep(120);

exit_err:
	return ret;
}

static int s6e3ha2_wqhd_init(struct dsim_device *dsim)
{
	int ret = 0;

	dsim_info("MDD : %s was called\n", __func__);

	ret = dsim_write_hl_data(dsim, S6E3HA2_SEQ_TEST_KEY_ON_F0, ARRAY_SIZE(S6E3HA2_SEQ_TEST_KEY_ON_F0));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : SEQ_TEST_KEY_ON_F0\n", __func__);
		goto init_exit;
	}
	ret = dsim_write_hl_data(dsim, S6E3HA2_SEQ_TEST_KEY_ON_FC, ARRAY_SIZE(S6E3HA2_SEQ_TEST_KEY_ON_FC));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : SEQ_TEST_KEY_ON_FC\n", __func__);
		goto init_exit;
	}

	/* 7. Sleep Out(11h) */
	ret = dsim_write_hl_data(dsim, S6E3HA2_SEQ_SLEEP_OUT, ARRAY_SIZE(S6E3HA2_SEQ_SLEEP_OUT));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : SEQ_SLEEP_OUT\n", __func__);
		goto init_exit;
	}
	msleep(5);

	/* 9. Interface Setting */

	ret = dsim_write_hl_data(dsim, S6E3HA2_SEQ_SINGLE_DSI_1, ARRAY_SIZE(S6E3HA2_SEQ_SINGLE_DSI_1));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : SEQ_SINGLE_DSI_1\n", __func__);
		goto init_exit;
	}
	ret = dsim_write_hl_data(dsim, S6E3HA2_SEQ_SINGLE_DSI_2, ARRAY_SIZE(S6E3HA2_SEQ_SINGLE_DSI_2));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : SEQ_SINGLE_DSI_2\n", __func__);
		goto init_exit;
	}
#ifdef CONFIG_LCD_HMT
	if(dsim->priv.hmt_on != HMT_ON)
#endif
		msleep(120);

	/* Common Setting */
	ret = dsim_write_hl_data(dsim, S6E3HA2_SEQ_TE_ON, ARRAY_SIZE(S6E3HA2_SEQ_TE_ON));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : SEQ_TE_ON\n", __func__);
		goto init_exit;
	}

	ret = dsim_write_hl_data(dsim, S6E3HA2_SEQ_TSP_TE, ARRAY_SIZE(S6E3HA2_SEQ_TSP_TE));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : SEQ_TSP_TE\n", __func__);
		goto init_exit;
	}

	ret = dsim_write_hl_data(dsim, S6E3HA2_SEQ_PENTILE_SETTING, ARRAY_SIZE(S6E3HA2_SEQ_PENTILE_SETTING));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : SEQ_PENTILE_SETTING\n", __func__);
		goto init_exit;
	}

	/* POC setting */
	ret = dsim_write_hl_data(dsim, S6E3HA2_SEQ_POC_SETTING1, ARRAY_SIZE(S6E3HA2_SEQ_POC_SETTING1));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : SEQ_POC_SETTING1\n", __func__);
		goto init_exit;
	}
	ret = dsim_write_hl_data(dsim, S6E3HA2_SEQ_POC_SETTING2, ARRAY_SIZE(S6E3HA2_SEQ_POC_SETTING2));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : SEQ_POC_SETTING2\n", __func__);
		goto init_exit;
	}

	/* OSC setting */
	ret = dsim_write_hl_data(dsim, S6E3HA2_SEQ_OSC_SETTING1, ARRAY_SIZE(S6E3HA2_SEQ_OSC_SETTING1));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : S6E3HA2_SEQ_OSC1\n", __func__);
		goto init_exit;
	}
	ret = dsim_write_hl_data(dsim, S6E3HA2_SEQ_OSC_SETTING2, ARRAY_SIZE(S6E3HA2_SEQ_OSC_SETTING2));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : S6E3HA2_SEQ_OSC2\n", __func__);
		goto init_exit;
	}
	ret = dsim_write_hl_data(dsim, S6E3HA2_SEQ_OSC_SETTING3, ARRAY_SIZE(S6E3HA2_SEQ_OSC_SETTING3));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : S6E3HA2_SEQ_OSC3\n", __func__);
		goto init_exit;
	}
	ret = dsim_write_hl_data(dsim, S6E3HA2_SEQ_TEST_KEY_OFF_FC, ARRAY_SIZE(S6E3HA2_SEQ_TEST_KEY_OFF_FC));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : SEQ_TEST_KEY_OFF_FC\n", __func__);
		goto init_exit;
	}

	/* PCD setting */
	ret = dsim_write_hl_data(dsim, S6E3HA2_SEQ_PCD_SETTING, ARRAY_SIZE(S6E3HA2_SEQ_PCD_SETTING));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : SEQ_PCD_SETTING\n", __func__);
		goto init_exit;
	}

	ret = dsim_write_hl_data(dsim, S6E3HA2_SEQ_ERR_FG_SETTING, ARRAY_SIZE(S6E3HA2_SEQ_ERR_FG_SETTING));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : SEQ_ERR_FG_SETTING\n", __func__);
		goto init_exit;
	}

#ifndef CONFIG_PANEL_AID_DIMMING
	/* Brightness Setting */
	ret = dsim_write_hl_data(dsim, S6E3HA2_SEQ_GAMMA_CONDITION_SET, ARRAY_SIZE(S6E3HA2_SEQ_GAMMA_CONDITION_SET));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_GAMMA_CONDITION_SET\n", __func__);
		goto init_exit;
	}
	ret = dsim_write_hl_data(dsim, S6E3HA2_SEQ_AOR_CONTROL, ARRAY_SIZE(S6E3HA2_SEQ_AOR_CONTROL));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_AOR_CONTROL\n", __func__);
		goto init_exit;
	}
	ret = dsim_write_hl_data(dsim, S6E3HA2_SEQ_ELVSS_SET, ARRAY_SIZE(S6E3HA2_SEQ_ELVSS_SET));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_ELVSS_SET\n", __func__);
		goto init_exit;
	}
	ret = dsim_write_hl_data(dsim, S6E3HA2_SEQ_VINT_SET, ARRAY_SIZE(S6E3HA2_SEQ_VINT_SET));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_ELVSS_SET\n", __func__);
		goto init_exit;
	}

	ret = dsim_write_hl_data(dsim, S6E3HA2_SEQ_GAMMA_UPDATE, ARRAY_SIZE(S6E3HA2_SEQ_GAMMA_UPDATE));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_GAMMA_UPDATE\n", __func__);
		goto init_exit;
	}

	/* ACL Setting */
	ret = dsim_write_hl_data(dsim, S6E3HA2_SEQ_ACL_OFF, ARRAY_SIZE(S6E3HA2_SEQ_ACL_OFF));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_ACL_OFF\n", __func__);
		goto init_exit;
	}
	ret = dsim_write_hl_data(dsim, S6E3HA2_SEQ_ACL_OFF_OPR, ARRAY_SIZE(S6E3HA2_SEQ_ACL_OFF_OPR));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_ACL_OFF_OPR\n", __func__);
		goto init_exit;
	}
		/* elvss */
	ret = dsim_write_hl_data(dsim, S6E3HA2_SEQ_TSET_GLOBAL, ARRAY_SIZE(S6E3HA2_SEQ_TSET_GLOBAL));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_TSET_GLOBAL\n", __func__);
		goto init_exit;
	}
	ret = dsim_write_hl_data(dsim, S6E3HA2_SEQ_TSET, ARRAY_SIZE(S6E3HA2_SEQ_TSET));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_TSET\n", __func__);
		goto init_exit;
	}
#endif
	ret = dsim_write_hl_data(dsim, S6E3HA2_SEQ_TEST_KEY_OFF_F0, ARRAY_SIZE(S6E3HA2_SEQ_TEST_KEY_OFF_F0));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : SEQ_TEST_KEY_OFF_F0\n", __func__);
		goto init_exit;
	}

init_exit:
	return ret;
}



struct dsim_panel_ops s6e3ha2_panel_ops = {
	.probe		= s6e3ha2_wqhd_probe,
	.displayon	= s6e3ha2_wqhd_displayon,
	.exit		= s6e3ha2_wqhd_exit,
	.init		= s6e3ha2_wqhd_init,
	.dump 		= s6e3ha2_wqhd_dump,
};


struct dsim_panel_ops *dsim_panel_get_priv_ops(struct dsim_device *dsim)
{
	if (dynamic_lcd_type == LCD_TYPE_S6E3HA0_WQXGA)
		return &s6e3ha0_panel_ops;
	else
		return &s6e3ha2_panel_ops;
}


static int __init get_lcd_type(char *arg)
{
	unsigned int lcdtype;

	get_option(&arg, &lcdtype);

	dsim_info("--- Parse LCD TYPE ---\n");
	dsim_info("LCDTYPE : %x\n", lcdtype);

	if ((lcdtype & LCDTYPE_MASK)  == S6E3HA2_WQXGA_ID1) {
		dynamic_lcd_type = LCD_TYPE_S6E3HA2_WQXGA;
		dsim_info("LCD TYPE : S6E3HA2 (WQXGA) : %d\n", dynamic_lcd_type);
	} else {
		dynamic_lcd_type = LCD_TYPE_S6E3HA0_WQXGA;
		dsim_info("LCD TYPE : S6E3HA0 (WQHD) : %d \n", dynamic_lcd_type);
	}
	return 0;
}
early_param("lcdtype", get_lcd_type);

