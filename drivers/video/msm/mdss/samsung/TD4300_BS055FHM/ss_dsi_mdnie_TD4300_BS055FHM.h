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

#ifndef _SAMSUNG_DSI_MDNIE_TD4300_BS055FHM_
#define _SAMSUNG_DSI_MDNIE_TD4300_BS055FHM_

#include "../ss_dsi_mdnie_lite_common.h"

#define MDNIE_COLOR_BLINDE_CMD_OFFSET 0

#define ADDRESS_SCR_WHITE_RED   0x0
#define ADDRESS_SCR_WHITE_GREEN 0x0
#define ADDRESS_SCR_WHITE_BLUE  0x0

#define MDNIE_RGB_SENSOR_INDEX	0

#define MDNIE_STEP1_INDEX 0
#define MDNIE_STEP2_INDEX 1

static char DSI0_UI_MDNIE_1[] ={
	0xC7,
	0x00,
	0x16,
	0x1D,
	0x25,
	0x31,
	0x3E,
	0x48,
	0x57,
	0x3B,
	0x42,
	0x4E,
	0x5B,
	0x64,
	0x6D,
	0x7F,
	0x00,
	0x16,
	0x1D,
	0x25,
	0x31,
	0x3E,
	0x48,
	0x57,
	0x3B,
	0x42,
	0x4E,
	0x5B,
	0x64,
	0x6D,
	0x7F,
};

static char DSI0_UI_MDNIE_2[] ={
	0xC8,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0xFC,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0xFC,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0xFC,
	0x00,
};

static char DSI0_UI_MDNIE_3[] ={
	0x55,
	0x00,
};

static char DSI0_HBM_CE_MDNIE_1[] ={
	0xC7,
	0x00,
	0x15,
	0x19,
	0x20,
	0x2B,
	0x37,
	0x40,
	0x4E,
	0x33,
	0x3A,
	0x43,
	0x4D,
	0x57,
	0x5F,
	0x7F,
	0x00,
	0x15,
	0x19,
	0x20,
	0x2B,
	0x37,
	0x40,
	0x4E,
	0x33,
	0x3A,
	0x43,
	0x4D,
	0x57,
	0x5F,
	0x7F,
};

static char DSI0_HBM_CE_MDNIE_2[] ={
	0xC8,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0xFC,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0xFC,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0xFC,
	0x00,
};

static char DSI0_HBM_CE_MDNIE_3[] ={
	0x55,
	0x00,
};

static char DSI0_VIDEO_MDNIE_1[] ={
	0xC7,
	0x00,
	0x10,
	0x15,
	0x1C,
	0x29,
	0x39,
	0x44,
	0x56,
	0x3D,
	0x46,
	0x54,
	0x63,
	0x68,
	0x6D,
	0x7F,
	0x00,
	0x10,
	0x15,
	0x1C,
	0x29,
	0x39,
	0x44,
	0x56,
	0x3D,
	0x46,
	0x54,
	0x63,
	0x68,
	0x6D,
	0x7F,
};

static char DSI0_VIDEO_MDNIE_2[] = {
	0xC8,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0xFC,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0xFC,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0xFC,
	0x00,
};

static char DSI0_VIDEO_MDNIE_3[] ={
	0x55,
	0x83,
};

static char DSI0_CAMERA_MDNIE_1[] ={
	0xC7,
	0x00,
	0x16,
	0x1D,
	0x25,
	0x31,
	0x3E,
	0x48,
	0x57,
	0x3B,
	0x42,
	0x4E,
	0x5B,
	0x64,
	0x6D,
	0x7F,
	0x00,
	0x16,
	0x1D,
	0x25,
	0x31,
	0x3E,
	0x48,
	0x57,
	0x3B,
	0x42,
	0x4E,
	0x5B,
	0x64,
	0x6D,
	0x7F,
};

static char DSI0_CAMERA_MDNIE_2[] ={
	0xC8,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0xFC,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0xFC,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0xFC,
	0x00,
};

static char DSI0_CAMERA_MDNIE_3[] ={
	0x55,
	0x83,
};

static char DSI0_EBOOK_MDNIE_1[] = {
	0xC7,
	0x00,
	0x16,
	0x1D,
	0x25,
	0x31,
	0x3E,
	0x48,
	0x57,
	0x3B,
	0x42,
	0x4E,
	0x5B,
	0x64,
	0x6D,
	0x7F,
	0x00,
	0x16,
	0x1D,
	0x25,
	0x31,
	0x3E,
	0x48,
	0x57,
	0x3B,
	0x42,
	0x4E,
	0x5B,
	0x64,
	0x6D,
	0x7F,
};

static char DSI0_EBOOK_MDNIE_2[] = {
	0xC8,
	0x01,
	0x00,
	0x00,
	0x00,
	0x00,
	0xFC,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0xCA,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x4D,
	0x00,
};

static char DSI0_EBOOK_MDNIE_3[] ={
	0x55,
	0x00,
};

static char DSI0_GAME_LOW_MDNIE_1[] ={
	0xC7,
	0x00,
	0x16,
	0x1D,
	0x25,
	0x31,
	0x3E,
	0x48,
	0x57,
	0x3B,
	0x42,
	0x4E,
	0x5B,
	0x64,
	0x6D,
	0x7F,
	0x00,
	0x16,
	0x1D,
	0x25,
	0x31,
	0x3E,
	0x48,
	0x57,
	0x3B,
	0x42,
	0x4E,
	0x5B,
	0x64,
	0x6D,
	0x7F,
};

static char DSI0_GAME_LOW_MDNIE_2[] ={
	0xC8,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0xFC,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0xFC,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0xFC,
	0x00,
};

static char DSI0_GAME_LOW_MDNIE_3[] ={
	0x55,
	0x83,
};

static char DSI0_GAME_MID_MDNIE_1[] ={
	0xC7,
	0x00,
	0x16,
	0x1D,
	0x25,
	0x31,
	0x3E,
	0x48,
	0x57,
	0x3B,
	0x42,
	0x4E,
	0x5B,
	0x64,
	0x6D,
	0x7F,
	0x00,
	0x16,
	0x1D,
	0x25,
	0x31,
	0x3E,
	0x48,
	0x57,
	0x3B,
	0x42,
	0x4E,
	0x5B,
	0x64,
	0x6D,
	0x7F,
};

static char DSI0_GAME_MID_MDNIE_2[] ={
	0xC8,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0xFC,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0xFC,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0xFC,
	0x00,
};

static char DSI0_GAME_MID_MDNIE_3[] ={
	0x55,
	0x83,
};

static char DSI0_GAME_HIGH_MDNIE_1[] ={
	0xC7,
	0x00,
	0x16,
	0x1D,
	0x25,
	0x31,
	0x3E,
	0x48,
	0x57,
	0x3B,
	0x42,
	0x4E,
	0x5B,
	0x64,
	0x6D,
	0x7F,
	0x00,
	0x16,
	0x1D,
	0x25,
	0x31,
	0x3E,
	0x48,
	0x57,
	0x3B,
	0x42,
	0x4E,
	0x5B,
	0x64,
	0x6D,
	0x7F,
};

static char DSI0_GAME_HIGH_MDNIE_2[] ={
	0xC8,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0xFC,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0xFC,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0xFC,
	0x00,
};

static char DSI0_GAME_HIGH_MDNIE_3[] ={
	0x55,
	0x83,
};

static struct dsi_cmd_desc DSI0_CAMERA_MDNIE[] = {
	{{DTYPE_DCS_LWRITE, 0, 0, 0, 0, sizeof(DSI0_CAMERA_MDNIE_1)}, DSI0_CAMERA_MDNIE_1},
	{{DTYPE_DCS_LWRITE, 0, 0, 0, 0, sizeof(DSI0_CAMERA_MDNIE_2)}, DSI0_CAMERA_MDNIE_2},
	{{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(DSI0_CAMERA_MDNIE_3)}, DSI0_CAMERA_MDNIE_3},
};
static struct dsi_cmd_desc DSI0_EBOOK_MDNIE[] = {
	{{DTYPE_DCS_LWRITE, 0, 0, 0, 0, sizeof(DSI0_EBOOK_MDNIE_1)}, DSI0_EBOOK_MDNIE_1},
	{{DTYPE_DCS_LWRITE, 0, 0, 0, 0, sizeof(DSI0_EBOOK_MDNIE_2)}, DSI0_EBOOK_MDNIE_2},
	{{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(DSI0_EBOOK_MDNIE_3)}, DSI0_EBOOK_MDNIE_3},
};
static struct dsi_cmd_desc DSI0_HBM_CE_MDNIE[] = {
	{{DTYPE_DCS_LWRITE, 0, 0, 0, 0, sizeof(DSI0_HBM_CE_MDNIE_1)}, DSI0_HBM_CE_MDNIE_1},
	{{DTYPE_DCS_LWRITE, 0, 0, 0, 0, sizeof(DSI0_HBM_CE_MDNIE_2)}, DSI0_HBM_CE_MDNIE_2},
	{{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(DSI0_HBM_CE_MDNIE_3)}, DSI0_HBM_CE_MDNIE_3},
};
static struct dsi_cmd_desc DSI0_LIGHT_NOTIFICATION_MDNIE[] = {
	{{DTYPE_DCS_LWRITE, 0, 0, 0, 0, sizeof(DSI0_UI_MDNIE_1)}, DSI0_UI_MDNIE_1},
	{{DTYPE_DCS_LWRITE, 0, 0, 0, 0, sizeof(DSI0_UI_MDNIE_2)}, DSI0_UI_MDNIE_2},
	{{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(DSI0_UI_MDNIE_3)}, DSI0_UI_MDNIE_3},
};
static struct dsi_cmd_desc DSI0_UI_MDNIE[] = {
	{{DTYPE_DCS_LWRITE, 0, 0, 0, 0, sizeof(DSI0_UI_MDNIE_1)}, DSI0_UI_MDNIE_1},
	{{DTYPE_DCS_LWRITE, 0, 0, 0, 0, sizeof(DSI0_UI_MDNIE_2)}, DSI0_UI_MDNIE_2},
	{{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(DSI0_UI_MDNIE_3)}, DSI0_UI_MDNIE_3},
};
static struct dsi_cmd_desc DSI0_VIDEO_MDNIE[] = {
	{{DTYPE_DCS_LWRITE, 0, 0, 0, 0, sizeof(DSI0_VIDEO_MDNIE_1)}, DSI0_VIDEO_MDNIE_1},
	{{DTYPE_DCS_LWRITE, 0, 0, 0, 0, sizeof(DSI0_VIDEO_MDNIE_2)}, DSI0_VIDEO_MDNIE_2},
	{{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(DSI0_VIDEO_MDNIE_3)}, DSI0_VIDEO_MDNIE_3},
};
static struct dsi_cmd_desc DSI0_GAME_LOW_MDNIE[] = {
	{{DTYPE_DCS_LWRITE, 0, 0, 0, 0, sizeof(DSI0_GAME_LOW_MDNIE_1)}, DSI0_GAME_LOW_MDNIE_1},
	{{DTYPE_DCS_LWRITE, 0, 0, 0, 0, sizeof(DSI0_GAME_LOW_MDNIE_2)}, DSI0_GAME_LOW_MDNIE_2},
	{{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(DSI0_GAME_LOW_MDNIE_3)}, DSI0_GAME_LOW_MDNIE_3},
};
static struct dsi_cmd_desc DSI0_GAME_MID_MDNIE[] = {
	{{DTYPE_DCS_LWRITE, 0, 0, 0, 0, sizeof(DSI0_GAME_MID_MDNIE_1)}, DSI0_GAME_MID_MDNIE_1},
	{{DTYPE_DCS_LWRITE, 0, 0, 0, 0, sizeof(DSI0_GAME_MID_MDNIE_2)}, DSI0_GAME_MID_MDNIE_2},
	{{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(DSI0_GAME_MID_MDNIE_3)}, DSI0_GAME_MID_MDNIE_3},
};
static struct dsi_cmd_desc DSI0_GAME_HIGH_MDNIE[] = {
	{{DTYPE_DCS_LWRITE, 0, 0, 0, 0, sizeof(DSI0_GAME_HIGH_MDNIE_1)}, DSI0_GAME_HIGH_MDNIE_1},
	{{DTYPE_DCS_LWRITE, 0, 0, 0, 0, sizeof(DSI0_GAME_HIGH_MDNIE_2)}, DSI0_GAME_HIGH_MDNIE_2},
	{{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(DSI0_GAME_HIGH_MDNIE_3)}, DSI0_GAME_HIGH_MDNIE_3},
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
			{DSI0_VIDEO_MDNIE,	NULL},
			{DSI0_VIDEO_MDNIE,	NULL},
			{DSI0_VIDEO_MDNIE,	NULL},
			{DSI0_VIDEO_MDNIE,	NULL},
			{DSI0_VIDEO_MDNIE,	NULL},
			{DSI0_EBOOK_MDNIE,	NULL},
		},
		// VIDEO_COLD_APP
		{
			{DSI0_VIDEO_MDNIE,	NULL},
			{DSI0_VIDEO_MDNIE,	NULL},
			{DSI0_VIDEO_MDNIE,	NULL},
			{DSI0_VIDEO_MDNIE,	NULL},
			{DSI0_VIDEO_MDNIE,	NULL},
			{DSI0_EBOOK_MDNIE,	NULL},
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
			{DSI0_UI_MDNIE,	NULL},
			{DSI0_UI_MDNIE,	NULL},
			{DSI0_UI_MDNIE,	NULL},
			{DSI0_UI_MDNIE,	NULL},
			{DSI0_UI_MDNIE,	NULL},
			{DSI0_EBOOK_MDNIE,	NULL},
		},
		// VT_APP
		{
			{DSI0_UI_MDNIE,	NULL},
			{DSI0_UI_MDNIE,	NULL},
			{DSI0_UI_MDNIE,	NULL},
			{DSI0_UI_MDNIE,	NULL},
			{DSI0_UI_MDNIE,	NULL},
			{DSI0_EBOOK_MDNIE,	NULL},
		},
		// BROWSER_APP
		{
			{DSI0_UI_MDNIE,	NULL},
			{DSI0_UI_MDNIE,	NULL},
			{DSI0_UI_MDNIE,	NULL},
			{DSI0_UI_MDNIE,	NULL},
			{DSI0_UI_MDNIE,	NULL},
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
		// GAME_LOW_APP
		{
			{DSI0_GAME_LOW_MDNIE,	NULL},
			{DSI0_GAME_LOW_MDNIE,	NULL},
			{DSI0_GAME_LOW_MDNIE,	NULL},
			{DSI0_GAME_LOW_MDNIE,	NULL},
			{DSI0_GAME_LOW_MDNIE,	NULL},
			{DSI0_GAME_LOW_MDNIE,	NULL},
		},
		// GAME_MID_APP
		{
			{DSI0_GAME_MID_MDNIE,	NULL},
			{DSI0_GAME_MID_MDNIE,	NULL},
			{DSI0_GAME_MID_MDNIE,	NULL},
			{DSI0_GAME_MID_MDNIE,	NULL},
			{DSI0_GAME_MID_MDNIE,	NULL},
			{DSI0_GAME_MID_MDNIE,	NULL},
		},
		// GAME_HIGH_APP
		{
			{DSI0_GAME_HIGH_MDNIE,	NULL},
			{DSI0_GAME_HIGH_MDNIE,	NULL},
			{DSI0_GAME_HIGH_MDNIE,	NULL},
			{DSI0_GAME_HIGH_MDNIE,	NULL},
			{DSI0_GAME_HIGH_MDNIE,	NULL},
			{DSI0_GAME_HIGH_MDNIE,	NULL},
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

#define DSI0_RGB_SENSOR_MDNIE_1_SIZE ARRAY_SIZE(DSI0_UI_MDNIE_1)
#define DSI0_RGB_SENSOR_MDNIE_2_SIZE ARRAY_SIZE(DSI0_UI_MDNIE_2)
#define DSI0_RGB_SENSOR_MDNIE_3_SIZE ARRAY_SIZE(DSI0_UI_MDNIE_3)
#endif /*_DSI_TCON_MDNIE_LITE_DATA_FHD_TD4300_BS055FHM_H_*/
