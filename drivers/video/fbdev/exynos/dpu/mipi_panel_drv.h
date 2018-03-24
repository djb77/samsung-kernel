/* linux/drivers/video/fbdev/exynos/dpu/mipi_panels/mipi_panel_drv.h
 *
 * Driver for Samsung MIPI-DSI LCD Panel.
 *
 * Copyright (c) 2013 Samsung Electronics
 * Minwoo Kim <minwoo7945.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __MIPI_PANEL_DRV_H__
#define __MIPI_PANEL_DRV_H__

#include "dsim.h"

// Only for Exynos8895 EVT 0
//#define DSIM_FIFO_SIZE	500
//#define MIPI_DSI_WRITE	0x00

struct dsim_device;

int probe_mipi_panel_drv(struct dsim_device *dsim);
int register_mipi_panel(const char *name, struct panel_info *panel);
int mipi_read_data(struct dsim_device *dsim, u8 addr, u8 *buf, int size);
int mipi_write_data(struct dsim_device *dsim, u8 cmd_id, const u8 *cmd, int size);
#endif //__MIPI_PANEL_DRV_H__
