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
*/
#ifndef _SAMSUNG_DSI_TCON_MDNIE_LIGHT_H_
#define _SAMSUNG_DSI_TCON_MDNIE_LIGHT_H_

#include "ss_dsi_panel_common.h"

#define NATURAL_MODE_ENABLE

#define NAME_STRING_MAX 30
#define MDNIE_COLOR_BLINDE_CMD_SIZE 18
#define MDNIE_COLOR_BLINDE_HBM_CMD_SIZE 24
#define COORDINATE_DATA_SIZE 6
#define MDNIE_SCR_CMD_SIZE 24

extern char mdnie_app_name[][NAME_STRING_MAX];
extern char mdnie_mode_name[][NAME_STRING_MAX];
extern char outdoor_name[][NAME_STRING_MAX];
extern struct mdnie_lite_tune_data mdnie_data;

#define APP_ID_TDMB (20)	/* for fake_id() */

enum BYPASS {
	BYPASS_DISABLE = 0,
	BYPASS_ENABLE,
};

enum APP {
	UI_APP = 0,
	VIDEO_APP,
	VIDEO_WARM_APP,
	VIDEO_COLD_APP,
	CAMERA_APP,
	NAVI_APP,
	GALLERY_APP,
	VT_APP,
	BROWSER_APP,
	eBOOK_APP,
	EMAIL_APP,
	GAME_LOW_APP,
	GAME_MID_APP,
	GAME_HIGH_APP,
	VIDEO_ENHANCER,
	VIDEO_ENHANCER_THIRD,
	TDMB_APP,	/* is linked to APP_ID_TDMB */
	MAX_APP_MODE,
};

enum MODE {
	DYNAMIC_MODE = 0,
	STANDARD_MODE,
#if defined(NATURAL_MODE_ENABLE)
	NATURAL_MODE,
#endif
	MOVIE_MODE,
	AUTO_MODE,
	READING_MODE,
	MAX_MODE,
};

enum OUTDOOR {
	OUTDOOR_OFF_MODE = 0,
	OUTDOOR_ON_MODE,
	MAX_OUTDOOR_MODE,
};

enum ACCESSIBILITY {
	ACCESSIBILITY_OFF = 0,
	NEGATIVE,
	COLOR_BLIND,
	CURTAIN,
	GRAYSCALE,
	GRAYSCALE_NEGATIVE,
	COLOR_BLIND_HBM,
	ACCESSIBILITY_MAX,
};

enum HMT_GRAY {
	HMT_GRAY_OFF = 0,
	HMT_GRAY_1,
	HMT_GRAY_2,
	HMT_GRAY_3,
	HMT_GRAY_4,
	HMT_GRAY_5,
	HMT_GRAY_MAX,
};

enum HMT_COLOR_TEMPERATURE {
	HMT_COLOR_TEMP_OFF = 0,
	HMT_COLOR_TEMP_3000K, /* 3000K */
	HMT_COLOR_TEMP_4000K, /* 4000K */
	HMT_COLOR_TEMP_5000K, /* 5000K */
	HMT_COLOR_TEMP_6500K, /* 6500K */
	HMT_COLOR_TEMP_7500K, /* 7500K */
	HMT_COLOR_TEMP_MAX
};

enum HDR {
	HDR_OFF = 0,
	HDR_1,
	HDR_2,
	HDR_3,
	HDR_MAX
};

enum COLOR_LENS {
	COLOR_LENS_OFF = 0,
	COLOR_LENS_ON,
	COLOR_LENS_MAX
};

enum COLOR_LENS_COLOR {
	COLOR_LENS_COLOR_BLUE = 0,
	COLOR_LENS_COLOR_AZURE,
	COLOR_LENS_COLOR_CYAN,
	COLOR_LENS_COLOR_SPRING_GREEN,
	COLOR_LENS_COLOR_GREEN,
	COLOR_LENS_COLOR_CHARTREUSE_GREEN,
	COLOR_LENS_COLOR_YELLOW,
	COLOR_LENS_COLOR_ORANGE,
	COLOR_LENS_COLOR_RED,
	COLOR_LENS_COLOR_ROSE,
	COLOR_LENS_COLOR_MAGENTA,
	COLOR_LENS_COLOR_VIOLET,
	COLOR_LENS_COLOR_MAX
};

enum COLOR_LENS_LEVEL {
	COLOR_LENS_LEVEL_20P = 0,
	COLOR_LENS_LEVEL_25P,
	COLOR_LENS_LEVEL_30P,
	COLOR_LENS_LEVEL_35P,
	COLOR_LENS_LEVEL_40P,
	COLOR_LENS_LEVEL_45P,
	COLOR_LENS_LEVEL_50P,
	COLOR_LENS_LEVEL_55P,
	COLOR_LENS_LEVEL_60P,
	COLOR_LENS_LEVEL_MAX,
};

enum LIGHT_NOTIFICATION {
	LIGHT_NOTIFICATION_OFF = 0,
	LIGHT_NOTIFICATION_ON,
	LIGHT_NOTIFICATION_MAX,
};

struct mdnie_lite_tun_type {
	enum BYPASS mdnie_bypass;
	enum BYPASS cabc_bypass;
	int hbm_enable;

	enum APP mdnie_app;
	enum MODE mdnie_mode;
	enum OUTDOOR outdoor;
	enum ACCESSIBILITY mdnie_accessibility;
	enum HMT_COLOR_TEMPERATURE hmt_color_temperature;
	enum HDR hdr;
	enum LIGHT_NOTIFICATION light_notification;

	char scr_white_red;
	char scr_white_green;
	char scr_white_blue;

	int scr_white_balanced_red;
	int scr_white_balanced_green;
	int scr_white_balanced_blue;

	int night_mode_enable;
	int night_mode_index;

	int ldu_mode_index;

	int color_lens_enable;
	int color_lens_color;
	int color_lens_level;

	int index;
	struct list_head used_list;

	struct samsung_display_driver_data *vdd;
#ifdef CONFIG_DISPLAY_USE_INFO
	struct notifier_block dpui_notif;
#endif
};

extern int config_cabc(int value);

enum {
	MDNIE_STEP1 = 0,
	MDNIE_STEP2,
	MDNIE_STEP3,
	MDNIE_STEP_MAX,
};

enum {
	ADDRESS_SCR_WHITE_RED_OFFSET = 0,
	ADDRESS_SCR_WHITE_GREEN_OFFSET,
	ADDRESS_SCR_WHITE_BLUE_OFFSET,
	ADDRESS_SCR_WHITE_MAX,
};

/* COMMON DATA THAT POINT TO MDNIE TUNING DATA */
struct mdnie_lite_tune_data {
/*******************************************
*					DSI0 DATA
********************************************/

	char *DSI0_COLOR_BLIND_MDNIE_1;
	char *DSI0_COLOR_BLIND_MDNIE_2;
	char *DSI0_COLOR_BLIND_MDNIE_SCR;
	char *DSI0_RGB_SENSOR_MDNIE_1;
	char *DSI0_RGB_SENSOR_MDNIE_2;
	char *DSI0_RGB_SENSOR_MDNIE_3;
	char *DSI0_RGB_SENSOR_MDNIE_SCR;
	char *DSI0_TRANS_DIMMING_MDNIE;
	char *DSI0_UI_DYNAMIC_MDNIE_2;
	char *DSI0_UI_STANDARD_MDNIE_2;
	char *DSI0_UI_AUTO_MDNIE_2;
	char *DSI0_VIDEO_DYNAMIC_MDNIE_2;
	char *DSI0_VIDEO_STANDARD_MDNIE_2;
	char *DSI0_VIDEO_AUTO_MDNIE_2;
	char *DSI0_CAMERA_MDNIE_2;
	char *DSI0_CAMERA_AUTO_MDNIE_2;
	char *DSI0_GALLERY_DYNAMIC_MDNIE_2;
	char *DSI0_GALLERY_STANDARD_MDNIE_2;
	char *DSI0_GALLERY_AUTO_MDNIE_2;
	char *DSI0_VT_DYNAMIC_MDNIE_2;
	char *DSI0_VT_STANDARD_MDNIE_2;
	char *DSI0_VT_AUTO_MDNIE_2;
	char *DSI0_BROWSER_DYNAMIC_MDNIE_2;
	char *DSI0_BROWSER_STANDARD_MDNIE_2;
	char *DSI0_BROWSER_AUTO_MDNIE_2;
	char *DSI0_EBOOK_DYNAMIC_MDNIE_2;
	char *DSI0_EBOOK_STANDARD_MDNIE_2;
	char *DSI0_EBOOK_AUTO_MDNIE_2;
	char *DSI0_TDMB_DYNAMIC_MDNIE_2;
	char *DSI0_TDMB_STANDARD_MDNIE_2;
	char *DSI0_TDMB_AUTO_MDNIE_2;
	char *DSI0_NIGHT_MODE_MDNIE_1;
	char *DSI0_NIGHT_MODE_MDNIE_2;
	char *DSI0_NIGHT_MODE_MDNIE_SCR;
	char *DSI0_COLOR_LENS_MDNIE_1;
	char *DSI0_COLOR_LENS_MDNIE_2;
	char *DSI0_COLOR_LENS_MDNIE_SCR;

	struct dsi_cmd_desc *DSI0_BYPASS_MDNIE;
	struct dsi_cmd_desc *DSI0_NEGATIVE_MDNIE;
	struct dsi_cmd_desc *DSI0_COLOR_BLIND_MDNIE;
	struct dsi_cmd_desc *DSI0_HBM_CE_MDNIE;
	struct dsi_cmd_desc *DSI0_HBM_CE_TEXT_MDNIE;
	struct dsi_cmd_desc *DSI0_RGB_SENSOR_MDNIE;
	struct dsi_cmd_desc *DSI0_CURTAIN;
	struct dsi_cmd_desc *DSI0_GRAYSCALE_MDNIE;
	struct dsi_cmd_desc *DSI0_GRAYSCALE_NEGATIVE_MDNIE;
	struct dsi_cmd_desc *DSI0_UI_DYNAMIC_MDNIE;
	struct dsi_cmd_desc *DSI0_UI_STANDARD_MDNIE;
	struct dsi_cmd_desc *DSI0_UI_NATURAL_MDNIE;
	struct dsi_cmd_desc *DSI0_UI_MOVIE_MDNIE;
	struct dsi_cmd_desc *DSI0_UI_AUTO_MDNIE;
	struct dsi_cmd_desc *DSI0_VIDEO_OUTDOOR_MDNIE;
	struct dsi_cmd_desc *DSI0_VIDEO_DYNAMIC_MDNIE;
	struct dsi_cmd_desc *DSI0_VIDEO_STANDARD_MDNIE;
	struct dsi_cmd_desc *DSI0_VIDEO_NATURAL_MDNIE;
	struct dsi_cmd_desc *DSI0_VIDEO_MOVIE_MDNIE;
	struct dsi_cmd_desc *DSI0_VIDEO_AUTO_MDNIE;
	struct dsi_cmd_desc *DSI0_VIDEO_WARM_OUTDOOR_MDNIE;
	struct dsi_cmd_desc *DSI0_VIDEO_WARM_MDNIE;
	struct dsi_cmd_desc *DSI0_VIDEO_COLD_OUTDOOR_MDNIE;
	struct dsi_cmd_desc *DSI0_VIDEO_COLD_MDNIE;
	struct dsi_cmd_desc *DSI0_CAMERA_OUTDOOR_MDNIE;
	struct dsi_cmd_desc *DSI0_CAMERA_MDNIE;
	struct dsi_cmd_desc *DSI0_CAMERA_AUTO_MDNIE;
	struct dsi_cmd_desc *DSI0_GALLERY_DYNAMIC_MDNIE;
	struct dsi_cmd_desc *DSI0_GALLERY_STANDARD_MDNIE;
	struct dsi_cmd_desc *DSI0_GALLERY_NATURAL_MDNIE;
	struct dsi_cmd_desc *DSI0_GALLERY_MOVIE_MDNIE;
	struct dsi_cmd_desc *DSI0_GALLERY_AUTO_MDNIE;
	struct dsi_cmd_desc *DSI0_VT_DYNAMIC_MDNIE;
	struct dsi_cmd_desc *DSI0_VT_STANDARD_MDNIE;
	struct dsi_cmd_desc *DSI0_VT_NATURAL_MDNIE;
	struct dsi_cmd_desc *DSI0_VT_MOVIE_MDNIE;
	struct dsi_cmd_desc *DSI0_VT_AUTO_MDNIE;
	struct dsi_cmd_desc *DSI0_BROWSER_DYNAMIC_MDNIE;
	struct dsi_cmd_desc *DSI0_BROWSER_STANDARD_MDNIE;
	struct dsi_cmd_desc *DSI0_BROWSER_NATURAL_MDNIE;
	struct dsi_cmd_desc *DSI0_BROWSER_MOVIE_MDNIE;
	struct dsi_cmd_desc *DSI0_BROWSER_AUTO_MDNIE;
	struct dsi_cmd_desc *DSI0_EBOOK_DYNAMIC_MDNIE;
	struct dsi_cmd_desc *DSI0_EBOOK_STANDARD_MDNIE;
	struct dsi_cmd_desc *DSI0_EBOOK_NATURAL_MDNIE;
	struct dsi_cmd_desc *DSI0_EBOOK_MOVIE_MDNIE;
	struct dsi_cmd_desc *DSI0_EBOOK_AUTO_MDNIE;
	struct dsi_cmd_desc *DSI0_EMAIL_AUTO_MDNIE;
	struct dsi_cmd_desc *DSI0_GAME_LOW_MDNIE;
	struct dsi_cmd_desc *DSI0_GAME_MID_MDNIE;
	struct dsi_cmd_desc *DSI0_GAME_HIGH_MDNIE;
	struct dsi_cmd_desc *DSI0_TDMB_DYNAMIC_MDNIE;
	struct dsi_cmd_desc *DSI0_TDMB_STANDARD_MDNIE;
	struct dsi_cmd_desc *DSI0_TDMB_NATURAL_MDNIE;
	struct dsi_cmd_desc *DSI0_TDMB_MOVIE_MDNIE;
	struct dsi_cmd_desc *DSI0_TDMB_AUTO_MDNIE;
	struct dsi_cmd_desc *DSI0_NIGHT_MODE_MDNIE;
	struct dsi_cmd_desc *DSI0_COLOR_LENS_MDNIE;

	struct dsi_cmd_desc *(*mdnie_tune_value_dsi0)[MAX_MODE][MAX_OUTDOOR_MODE];
	struct dsi_cmd_desc **hmt_color_temperature_tune_value_dsi0;
	struct dsi_cmd_desc **hdr_tune_value_dsi0;
	struct dsi_cmd_desc **light_notification_tune_value_dsi0;

	int dsi0_bypass_mdnie_size;
	int mdnie_color_blinde_cmd_offset;
	int mdnie_step_index[MDNIE_STEP_MAX];
	int address_scr_white[ADDRESS_SCR_WHITE_MAX];
	int dsi0_rgb_sensor_mdnie_1_size;
	int dsi0_rgb_sensor_mdnie_2_size;
	int dsi0_rgb_sensor_mdnie_3_size;
	int dsi0_trans_dimming_data_index;
	char **dsi0_adjust_ldu_table;
	int dsi0_max_adjust_ldu;
	char *dsi0_night_mode_table;
	int dsi0_max_night_mode_index;
	char *dsi0_color_lens_table;
	int dsi0_scr_step_index;
	char dsi0_white_default_r;
	char dsi0_white_default_g;
	char dsi0_white_default_b;
	int dsi0_white_rgb_enabled;
	char dsi0_white_ldu_r;
	char dsi0_white_ldu_g;
	char dsi0_white_ldu_b;

/*******************************************
*					DSI1 DATA
********************************************/
	char *DSI1_COLOR_BLIND_MDNIE_1;
	char *DSI1_COLOR_BLIND_MDNIE_2;
	char *DSI1_COLOR_BLIND_MDNIE_SCR;
	char *DSI1_RGB_SENSOR_MDNIE_1;
	char *DSI1_RGB_SENSOR_MDNIE_2;
	char *DSI1_RGB_SENSOR_MDNIE_3;
	char *DSI1_RGB_SENSOR_MDNIE_SCR;
	char *DSI1_TRANS_DIMMING_MDNIE;
	char *DSI1_UI_DYNAMIC_MDNIE_2;
	char *DSI1_UI_STANDARD_MDNIE_2;
	char *DSI1_UI_AUTO_MDNIE_2;
	char *DSI1_VIDEO_DYNAMIC_MDNIE_2;
	char *DSI1_VIDEO_STANDARD_MDNIE_2;
	char *DSI1_VIDEO_AUTO_MDNIE_2;
	char *DSI1_CAMERA_MDNIE_2;
	char *DSI1_CAMERA_AUTO_MDNIE_2;
	char *DSI1_GALLERY_DYNAMIC_MDNIE_2;
	char *DSI1_GALLERY_STANDARD_MDNIE_2;
	char *DSI1_GALLERY_AUTO_MDNIE_2;
	char *DSI1_VT_DYNAMIC_MDNIE_2;
	char *DSI1_VT_STANDARD_MDNIE_2;
	char *DSI1_VT_AUTO_MDNIE_2;
	char *DSI1_BROWSER_DYNAMIC_MDNIE_2;
	char *DSI1_BROWSER_STANDARD_MDNIE_2;
	char *DSI1_BROWSER_AUTO_MDNIE_2;
	char *DSI1_EBOOK_DYNAMIC_MDNIE_2;
	char *DSI1_EBOOK_STANDARD_MDNIE_2;
	char *DSI1_EBOOK_AUTO_MDNIE_2;
	char *DSI1_TDMB_DYNAMIC_MDNIE_2;
	char *DSI1_TDMB_STANDARD_MDNIE_2;
	char *DSI1_TDMB_AUTO_MDNIE_2;
	char *DSI1_NIGHT_MODE_MDNIE_1;
	char *DSI1_NIGHT_MODE_MDNIE_2;
	char *DSI1_NIGHT_MODE_MDNIE_SCR;
	char *DSI1_COLOR_LENS_MDNIE_1;
	char *DSI1_COLOR_LENS_MDNIE_2;
	char *DSI1_COLOR_LENS_MDNIE_SCR;

	struct dsi_cmd_desc *DSI1_BYPASS_MDNIE;
	struct dsi_cmd_desc *DSI1_NEGATIVE_MDNIE;
	struct dsi_cmd_desc *DSI1_COLOR_BLIND_MDNIE;
	struct dsi_cmd_desc *DSI1_HBM_CE_MDNIE;
	struct dsi_cmd_desc *DSI1_HBM_CE_TEXT_MDNIE;
	struct dsi_cmd_desc *DSI1_RGB_SENSOR_MDNIE;
	struct dsi_cmd_desc *DSI1_CURTAIN;
	struct dsi_cmd_desc *DSI1_GRAYSCALE_MDNIE;
	struct dsi_cmd_desc *DSI1_GRAYSCALE_NEGATIVE_MDNIE;
	struct dsi_cmd_desc *DSI1_UI_DYNAMIC_MDNIE;
	struct dsi_cmd_desc *DSI1_UI_STANDARD_MDNIE;
	struct dsi_cmd_desc *DSI1_UI_NATURAL_MDNIE;
	struct dsi_cmd_desc *DSI1_UI_MOVIE_MDNIE;
	struct dsi_cmd_desc *DSI1_UI_AUTO_MDNIE;
	struct dsi_cmd_desc *DSI1_VIDEO_OUTDOOR_MDNIE;
	struct dsi_cmd_desc *DSI1_VIDEO_DYNAMIC_MDNIE;
	struct dsi_cmd_desc *DSI1_VIDEO_STANDARD_MDNIE;
	struct dsi_cmd_desc *DSI1_VIDEO_NATURAL_MDNIE;
	struct dsi_cmd_desc *DSI1_VIDEO_MOVIE_MDNIE;
	struct dsi_cmd_desc *DSI1_VIDEO_AUTO_MDNIE;
	struct dsi_cmd_desc *DSI1_VIDEO_WARM_OUTDOOR_MDNIE;
	struct dsi_cmd_desc *DSI1_VIDEO_WARM_MDNIE;
	struct dsi_cmd_desc *DSI1_VIDEO_COLD_OUTDOOR_MDNIE;
	struct dsi_cmd_desc *DSI1_VIDEO_COLD_MDNIE;
	struct dsi_cmd_desc *DSI1_CAMERA_OUTDOOR_MDNIE;
	struct dsi_cmd_desc *DSI1_CAMERA_MDNIE;
	struct dsi_cmd_desc *DSI1_CAMERA_AUTO_MDNIE;
	struct dsi_cmd_desc *DSI1_GALLERY_DYNAMIC_MDNIE;
	struct dsi_cmd_desc *DSI1_GALLERY_STANDARD_MDNIE;
	struct dsi_cmd_desc *DSI1_GALLERY_NATURAL_MDNIE;
	struct dsi_cmd_desc *DSI1_GALLERY_MOVIE_MDNIE;
	struct dsi_cmd_desc *DSI1_GALLERY_AUTO_MDNIE;
	struct dsi_cmd_desc *DSI1_VT_DYNAMIC_MDNIE;
	struct dsi_cmd_desc *DSI1_VT_STANDARD_MDNIE;
	struct dsi_cmd_desc *DSI1_VT_NATURAL_MDNIE;
	struct dsi_cmd_desc *DSI1_VT_MOVIE_MDNIE;
	struct dsi_cmd_desc *DSI1_VT_AUTO_MDNIE;
	struct dsi_cmd_desc *DSI1_BROWSER_DYNAMIC_MDNIE;
	struct dsi_cmd_desc *DSI1_BROWSER_STANDARD_MDNIE;
	struct dsi_cmd_desc *DSI1_BROWSER_NATURAL_MDNIE;
	struct dsi_cmd_desc *DSI1_BROWSER_MOVIE_MDNIE;
	struct dsi_cmd_desc *DSI1_BROWSER_AUTO_MDNIE;
	struct dsi_cmd_desc *DSI1_EBOOK_DYNAMIC_MDNIE;
	struct dsi_cmd_desc *DSI1_EBOOK_STANDARD_MDNIE;
	struct dsi_cmd_desc *DSI1_EBOOK_NATURAL_MDNIE;
	struct dsi_cmd_desc *DSI1_EBOOK_MOVIE_MDNIE;
	struct dsi_cmd_desc *DSI1_EBOOK_AUTO_MDNIE;
	struct dsi_cmd_desc *DSI1_EMAIL_AUTO_MDNIE;
	struct dsi_cmd_desc *DSI1_GAME_LOW_MDNIE;
	struct dsi_cmd_desc *DSI1_GAME_MID_MDNIE;
	struct dsi_cmd_desc *DSI1_GAME_HIGH_MDNIE;
	struct dsi_cmd_desc *DSI1_TDMB_DYNAMIC_MDNIE;
	struct dsi_cmd_desc *DSI1_TDMB_STANDARD_MDNIE;
	struct dsi_cmd_desc *DSI1_TDMB_NATURAL_MDNIE;
	struct dsi_cmd_desc *DSI1_TDMB_MOVIE_MDNIE;
	struct dsi_cmd_desc *DSI1_TDMB_AUTO_MDNIE;
	struct dsi_cmd_desc *DSI1_NIGHT_MODE_MDNIE;
	struct dsi_cmd_desc *DSI1_COLOR_LENS_MDNIE;

	struct dsi_cmd_desc *(*mdnie_tune_value_dsi1)[MAX_MODE][MAX_OUTDOOR_MODE];
	struct dsi_cmd_desc **hmt_color_temperature_tune_value_dsi1;
	struct dsi_cmd_desc **hdr_tune_value_dsi1;
	struct dsi_cmd_desc **light_notification_tune_value_dsi1;

	int dsi1_bypass_mdnie_size;
	/* int mdnie_color_blinde_cmd_offset; */
	/* int mdnie_step_index[MDNIE_STEP_MAX]; */
	/* int address_scr_white[ADDRESS_SCR_WHITE_MAX]; */
	int dsi1_rgb_sensor_mdnie_1_size;
	int dsi1_rgb_sensor_mdnie_2_size;
	int dsi1_rgb_sensor_mdnie_3_size;
	int dsi1_trans_dimming_data_index;
	char **dsi1_adjust_ldu_table;
	int dsi1_max_adjust_ldu;
	char *dsi1_night_mode_table;
	int dsi1_max_night_mode_index;
	char *dsi1_color_lens_table;
	int dsi1_scr_step_index;
	char dsi1_white_default_r;
	char dsi1_white_default_g;
	char dsi1_white_default_b;
	int dsi1_white_rgb_enabled;
	char dsi1_white_ldu_r;
	char dsi1_white_ldu_g;
	char dsi1_white_ldu_b;
};

/* COMMON FUNCTION*/
struct mdnie_lite_tun_type *init_dsi_tcon_mdnie_class(int index, struct samsung_display_driver_data *vdd_data);
int update_dsi_tcon_mdnie_register(struct samsung_display_driver_data *vdd);
void coordinate_tunning(int index, char *coordinate_data, int scr_wr_addr, int data_size);
void coordinate_tunning_multi(int index, char (*coordinate_data_multi[MAX_MODE])[COORDINATE_DATA_SIZE], int mdnie_tune_index, int scr_wr_addr, int data_size);
void coordinate_tunning_calculate(int index, int x, int y, char (*coordinate_data_multi[MAX_MODE])[COORDINATE_DATA_SIZE],  int *rgb_index, int scr_wr_addr, int data_size);

/* COMMON FUNCTION END*/

#endif /*_DSI_TCON_MDNIE_H_*/
