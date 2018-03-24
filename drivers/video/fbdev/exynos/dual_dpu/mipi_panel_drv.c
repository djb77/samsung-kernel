/* linux/drivers/video/fbdev/exynos/dpu/mipi_panels/mipi_panel_drv.c
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

#include <linux/lcd.h>
#include <video/mipi_display.h>
#include "dsim.h"

#include "../panel/panel.h"
#include "mipi_panel_drv.h"

#define MAX_STORED_PANEL	5
#define MAX_LDI_NAME 		32

struct stored_panel_info {
	const char *ldi_name;
	struct panel_info *panels;
};
int stored_panel_cnt = 0;

static struct stored_panel_info stored_panel[MAX_STORED_PANEL];

int register_mipi_panel(const char *name, struct panel_info *panel_info)
{
	int i;
	int ret = 0;

	if (stored_panel_cnt >= MAX_STORED_PANEL) {
		dsim_err("ERR:%s:Exceed MAX Stored Panel Count : %d\n", __func__, stored_panel_cnt);
		ret = -EINVAL;
		goto err_register;
	}
	for(i = 0; i < stored_panel_cnt; i++) {
		if (strcmp(name, stored_panel[i].ldi_name) == 0) {
			dsim_err("ERR:%s: arealdy exist %s\n", __func__, name);
			ret = -EINVAL;
			goto err_register;
		}
	}
	stored_panel[stored_panel_cnt].ldi_name = name;
	stored_panel[stored_panel_cnt].panels = panel_info;

	dsim_info("%s : %s was stored\n", __func__, stored_panel[stored_panel_cnt].ldi_name);

	stored_panel_cnt++;
	
err_register:

	return ret;	
}

static int mipi_panel_probe(struct dsim_device *dsim)
{
	int ret = 0;
	struct panel_info *panel_info;

	pr_info("%s enter\n", __func__);
	if ((dsim == NULL) || (dsim->mipi_drv->panel_info == NULL)) {
		dsim_err("ERR:%s:failed to get panel_info\n", __func__);
		return -EINVAL;
	}

	panel_info = dsim->mipi_drv->panel_info;
	panel_info->pdata = dsim;
	panel_info->ldi_name = dsim->lcd_info.ldi_name;
	
	if (panel_info->ops->probe)
		ret = panel_info->ops->probe(dsim);
	
	return ret;
}

static int mipi_panel_display_on(struct dsim_device *dsim)
{
	int ret = 0;

	struct panel_info *panel_info;

	pr_info("%s enter\n", __func__);
	if ((dsim == NULL) || (dsim->mipi_drv->panel_info == NULL)) {
		dsim_err("ERR:%s:failed to get panel_info\n", __func__);
		return -EINVAL;
	}

	panel_info = dsim->mipi_drv->panel_info;
	
	if (panel_info->ops->displayon)
		ret = panel_info->ops->displayon(dsim);
	
	return ret;
}

static int mipi_panel_suspend(struct dsim_device *dsim)
{
	int ret = 0;

	struct panel_info *panel_info;

	pr_info("%s enter\n", __func__);
	if ((dsim == NULL) || (dsim->mipi_drv->panel_info == NULL)) {
		dsim_err("ERR:%s:failed to get panel_info\n", __func__);
		return -EINVAL;
	}

	panel_info = dsim->mipi_drv->panel_info;
	
	if (panel_info->ops->suspend)
		ret = panel_info->ops->suspend(dsim);
	
	return ret;
}

static int mipi_panel_resume(struct dsim_device *dsim)
{
	int ret = 0;

	struct panel_info *panel_info;

	pr_info("%s enter\n", __func__);
	if ((dsim == NULL) || (dsim->mipi_drv->panel_info == NULL)) {
		dsim_err("ERR:%s:failed to get panel_info\n", __func__);
		return -EINVAL;
	}

	panel_info = dsim->mipi_drv->panel_info;
	
	if (panel_info->ops->resume)
		ret = panel_info->ops->resume(dsim);
	
	return ret;
}

static int mipi_panel_dump(struct dsim_device *dsim)
{
	int ret = 0;

	struct panel_info *panel_info;

	if ((dsim == NULL) || (dsim->mipi_drv->panel_info == NULL)) {
		dsim_err("ERR:%s:failed to get panel_info\n", __func__);
		return -EINVAL;
	}

	panel_info = dsim->mipi_drv->panel_info;
	
	if (panel_info->ops->dump)
		ret = panel_info->ops->dump(dsim);
	
	return ret;
}

static struct mipi_panel_drv mipi_drv = {
	.probe = mipi_panel_probe,
	.displayon = mipi_panel_display_on,
	.suspend = mipi_panel_suspend,
	.resume = mipi_panel_resume,
	.dump = mipi_panel_dump,
};

int probe_mipi_panel_drv(struct dsim_device *dsim)
{
	int i;
	int ret;
	int count;
	struct panel_info *panel_info;

	if (dsim == NULL) {
		dsim_err("ERR:%s:failed to register lcd driver\n", __func__);
		return -EINVAL;
	}

	count = stored_panel_cnt;
	for (i = 0; i < count; i++) {
		pr_info("registered : %s\n", stored_panel[i].ldi_name);
		if (!strcmp(dsim->lcd_info.ldi_name,
					stored_panel[i].ldi_name))
			break;
	}
	if (i >= count) {
		dsim_err("ERR:%s:can't found panel ops named : %s\n",
				__func__, dsim->lcd_info.ldi_name);
		return -EINVAL;
	}

	panel_info = stored_panel[i].panels;
	if (panel_info == NULL) {
		dsim_err("ERR:%s:failed to get panel's info\n", __func__);
		return -EINVAL;
	}

	mipi_drv.panel_info = stored_panel[i].panels;
	dsim->mipi_drv = &mipi_drv;

	if (panel_info->ops->early_probe)
		ret = panel_info->ops->early_probe(dsim);
	
	return ret;
}

static int __mipi_write_data(struct dsim_device *dsim, u8 cmd_id, const u8 *cmd, int size)
{
	int ret = 0;
	int retry = 5;
	unsigned char dsi_cmd = 0;

	if (cmd_id == MIPI_DSI_WRITE) {
		if (size == 1)
			dsi_cmd = MIPI_DSI_DCS_SHORT_WRITE;
		else if (size == 2)
			dsi_cmd = MIPI_DSI_DCS_SHORT_WRITE_PARAM;
		else
			dsi_cmd = MIPI_DSI_DCS_LONG_WRITE;
	} else {
		dsi_cmd = cmd_id;
	}

write_command:
	ret = dsim_write_data(dsim, dsi_cmd, (u8 *)cmd, size);
	if (ret != size) {
		retry--;
		if (retry) {
			dsim_info("INFO:%s:retry dsim write command : %d\n", __func__, retry);
			goto write_command;
		} else {
			dsim_err("ERR:%s:failed to dsim write command\n", __func__);
		}
	}
	return ret;
}

int mipi_write_data(struct dsim_device *dsim, u8 cmd_id, const u8 *cmd, int size)
{
	int ret, remained, tx_size, data_offset, tx_data_size;
	u8 cmd_buf[DSIM_FIFO_SIZE];
	u8 g_param[2] = {0xB0, 0x00};
#if 0 
	if (dsim->panel_stat == PANEL_DISCONNECTED) {
		printf("DISP:ERR:%s:panel disconnected\n");
		return -1;
	}
#endif

	data_offset = ((cmd_id == MIPI_DSI_WRITE) ? 1 : 0);
	remained = size - data_offset;

	do {
		if (g_param[1] != 0) {
			ret = __mipi_write_data(dsim, MIPI_DSI_WRITE, g_param, 2);
			if (ret != 2) {
				dsim_err("DISP:ERR:%s:failed to write global command\n", __func__);
				goto fail_write_command;
			}

			cmd_buf[0] = (cmd_id == MIPI_DSI_DSC_PPS) ? 0x9E : cmd[0];
			tx_size = min(remained + 1, DSIM_FIFO_SIZE);
			tx_data_size = (tx_size - 1);
			memcpy((u8 *)cmd_buf + 1, (u8 *)cmd + data_offset + g_param[1], tx_data_size);
			ret = __mipi_write_data(dsim, MIPI_DSI_WRITE, cmd_buf, tx_size);
			if (ret != tx_size) {
				dsim_err("DISP:ERR:%s:failed to write command\n", __func__);
				goto fail_write_command;
			}
#if 0
			pr_info("%s, addr 0x%02X, offset %d, len %d\n",
					__func__, cmd_buf[0], g_param[1], tx_size);
			print_data(cmd_buf, tx_size);
#endif
		} else {
			tx_size = min(remained + data_offset, DSIM_FIFO_SIZE);
			tx_data_size = (tx_size - data_offset);
			ret = __mipi_write_data(dsim, cmd_id, cmd, tx_size);
			if (ret != tx_size) {
				dsim_err("DISP:ERR:%s:failed to write command\n", __func__);
				goto fail_write_command;
			}
#if 0
			pr_info("%s, addr 0x%02X, offset %d, len %d\n",
					__func__, cmd[0], g_param[1], tx_size);
			print_data((u8 *)cmd, tx_size);
#endif
		}

		g_param[1] += tx_data_size;
		remained -= tx_data_size;
	} while (remained > 0);

	return size;

fail_write_command:
	return ret;
}

int mipi_read_data(struct dsim_device *dsim, u8 addr, u8 *buf, int size)
{
	int ret;
	int retry = 5;

read_command:
	ret = dsim_read_data(dsim, MIPI_DSI_DCS_READ, addr, buf, size);
	if (ret != size) {
		if (retry == 0) {
			dsim_err("ERR:%s:failed to read addr : %x\n", __func__, addr);
		} else {
			retry--;
			goto read_command;
		}
	}
	return ret;
}
