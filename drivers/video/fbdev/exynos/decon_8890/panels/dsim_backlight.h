/* linux/drivers/video/exynos_decon/panel/dsim_backlight.h
 *
 * Header file for Samsung MIPI-DSI Backlight driver.
 *
 * Copyright (c) 2013 Samsung Electronics
 * Minwoo Kim <minwoo7945.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __DSIM_BACKLIGHT__
#define __DSIM_BACKLIGHT__

#include "../dsim.h"

int dsim_backlight_probe(struct dsim_device *dsim);
int dsim_panel_set_brightness(struct dsim_device *dsim, int force);
int dsim_panel_set_brightness_for_hmt(struct dsim_device *dsim, int force);
int get_panel_acl_on( int p_br, int weakness_hbm_comp );
int dsim_panel_set_acl_step(struct dsim_device *dsim, int old_level, int new_level, int step, int pos );

#endif

