/*
 * linux/drivers/video/fbdev/exynos/panel/panel_bl.h
 *
 * Header file for Samsung Common LCD Driver.
 *
 * Copyright (c) 2016 Samsung Electronics
 * Gwanghui Lee <gwanghui.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __PANEL_BL_H__
#define __PANEL_BL_H__

struct panel_info;
struct panel_device;

#undef DEBUG_PAC
#define CONFIG_LCD_EXTEND_HBM

#ifdef CONFIG_PANEL_BACKLIGHT_PAC_3_0
#define BRT_SCALE	(100)
#define PANEL_BACKLIGHT_PAC_STEPS	(512)
#else
#define PANEL_BACKLIGHT_PAC_STEPS	(256)
#define BRT_SCALE	(1)
#endif
#define BRT_SCALE_UP(_val_)	((_val_) * BRT_SCALE)
#define BRT_SCALE_DN(_val_)	((_val_) / BRT_SCALE)
#define BRT_TBL_INDEX(_val_)	(BRT_SCALE_DN((_val_) + ((BRT_SCALE) - 1)))
#define BRT(brt)			(BRT_SCALE_UP(brt))
#define BRT_USER(brt)		(BRT_SCALE_DN(brt))

#define UI_MIN_BRIGHTNESS			(BRT(0))
#define UI_DEF_BRIGHTNESS			(BRT(128))
#define UI_MAX_BRIGHTNESS			(BRT(255))
#define UI_BRIGHTNESS_STEPS		(UI_MAX_BRIGHTNESS - UI_MIN_BRIGHTNESS + 1)
#define HBM_MIN_BRIGHTNESS			(UI_MAX_BRIGHTNESS + 1)
#define HBM_MAX_BRIGHTNESS			(BRT(365))
#define HBM_BRIGHTNESS_STEPS		(8)
#define EXTEND_HBM_MIN_BRIGHTNESS	(HBM_MAX_BRIGHTNESS + 1)
#define EXTEND_HBM_MAX_BRIGHTNESS	(BRT(366))
#define EXTEND_HBM_BRIGHTNESS_STEPS	(1)

#ifdef CONFIG_LCD_EXTEND_HBM
#define EXTEND_BRIGHTNESS			(EXTEND_HBM_MAX_BRIGHTNESS)
#else
#define EXTEND_BRIGHTNESS			(HBM_MAX_BRIGHTNESS)
#endif
#define IS_UI_BRIGHTNESS(br)            (((br) <= UI_MAX_BRIGHTNESS) ? 1 : 0)
#define IS_HBM_BRIGHTNESS(br)           ((((br) >= HBM_MIN_BRIGHTNESS) && ((br) <= HBM_MAX_BRIGHTNESS)) ? 1 : 0)
#define IS_EXT_HBM_BRIGHTNESS(br)       ((((br) >= EXTEND_HBM_MIN_BRIGHTNESS) && ((br) <= EXTEND_HBM_MAX_BRIGHTNESS)) ? 1 : 0)

enum SEARCH_TYPE {
	SEARCH_TYPE_EXACT,
	SEARCH_TYPE_LOWER,
	SEARCH_TYPE_UPPER,
	MAX_SEARCH_TYPE,
};

struct brightness_table {
	/* brightness to step mapping table */
	u32 *brt_to_step;
	u32 sz_brt_to_step;
	/* brightness to brightness mapping table */
	u32 (*brt_to_brt)[2];
	u32 sz_brt_to_brt;
	/* brightness table */
	u32 *brt;
	u32 sz_brt;
	/* actual brightness table */
	u32 *lum;
	u32 sz_lum;
	u32 sz_ui_lum;
	u32 sz_hbm_lum;
	u32 sz_ext_hbm_lum;
	u32 max_ui_lum;
	u32 max_hbm_lum;
};

struct panel_bl_ops {
    int (*set_brightness)(void *, int level);
	int (*get_brightness)(void *);
};

enum panel_bl_hw_type {
	PANEL_BL_HW_TYPE_TFT,
	PANEL_BL_HW_TYPE_OCTA,
	MAX_PANEL_BL_HW_TYPE,
};

enum panel_bl_subdev_type {
	PANEL_BL_SUBDEV_TYPE_DISP,
	PANEL_BL_SUBDEV_TYPE_HMD,
	MAX_PANEL_BL_SUBDEV,
};

struct panel_bl_properties {
	int id;
	int brightness;
	int actual_brightness;
	int actual_brightness_index;
	int actual_brightness_intrp;
	int acl_pwrsave;
	int acl_opr;
};

struct panel_bl_sub_dev {
	const char *name;
	enum panel_bl_hw_type hw_type;
	enum panel_bl_subdev_type subdev_type;
	struct brightness_table brt_tbl;
	int brightness;
};

struct panel_bl_device {
	const char *name;
	struct backlight_device *bd;
	void *bd_data;
	struct mutex ops_lock;
	const struct panel_bl_ops *ops;
	struct mutex lock;
	struct panel_bl_properties props;
	struct panel_bl_sub_dev subdev[MAX_PANEL_BL_SUBDEV];
#ifdef CONFIG_SUPPORT_INDISPLAY
	int saved_br;
	bool finger_layer;
#endif
};

int panel_bl_probe(struct panel_device *panel);
int panel_bl_set_brightness(struct panel_bl_device *, int, int);
int get_brightness_pac_step(struct panel_bl_device *, int);
int get_actual_brightness(struct panel_bl_device *, int);
int get_subdev_actual_brightness(struct panel_bl_device *, int, int);
int get_actual_brightness_index(struct panel_bl_device *, int);
int get_subdev_actual_brightness_index(struct panel_bl_device *, int, int);
int get_actual_brightness_interpolation(struct panel_bl_device *, int);
int get_subdev_actual_brightness_interpolation(struct panel_bl_device *, int, int);
int panel_bl_get_acl_pwrsave(struct panel_bl_device *);
int panel_bl_get_acl_opr(struct panel_bl_device *);
bool is_hbm_brightness(struct panel_bl_device *panel_bl, int brightness);
#endif /* __PANEL_BL_H__ */
