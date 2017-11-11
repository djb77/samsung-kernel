
/* linux/drivers/video/exynos_decon/panel/dsim_panel.c
 *
 * Header file for Samsung MIPI-DSI LCD Panel driver.
 *
 * Copyright (c) 2013 Samsung Electronics
 * Minwoo Kim <minwoo7945.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/lcd.h>
#include "../dsim.h"

#include "dsim_panel.h"
#include "dsim_backlight.h"
#include "panel_info.h"

#if defined(CONFIG_EXYNOS_DECON_MDNIE_LITE)
#include "mdnie.h"
#if defined(CONFIG_PANEL_S6E3HA3_DYNAMIC)
#include "mdnie_lite_table_hero1.h"
#elif defined(CONFIG_PANEL_S6E3HF4_WQHD)
#include "mdnie_lite_table_hero2.h"
#endif
#endif

#if defined(CONFIG_EXYNOS_DECON_MDNIE_LITE)
static int mdnie_lite_write_set(struct dsim_device *dsim, struct lcd_seq_info *seq, u32 num)
{
	int ret = 0, i;

	for (i = 0; i < num; i++) {
		if (seq[i].cmd) {
			ret = dsim_write_hl_data(dsim, seq[i].cmd, seq[i].len);
			if (ret != 0) {
				dsim_err("%s failed.\n", __func__);
				return ret;
			}
		}
		if (seq[i].sleep)
			usleep_range(seq[i].sleep * 1000 , seq[i].sleep * 1000);
	}
	return ret;
}

int mdnie_lite_send_seq(struct dsim_device *dsim, struct lcd_seq_info *seq, u32 num)
{
	int ret = 0;
	struct panel_private *panel = &dsim->priv;

	if (panel->lcdConnected == PANEL_DISCONNEDTED) {
		dsim_err("%s : %d : panel was not connected\n", __func__, dsim->id);
		return ret;
	}

	if (panel->state != PANEL_STATE_RESUMED) {
		dsim_info("%s : panel is not active\n", __func__);
		return -EIO;
	}

	mutex_lock(&panel->lock);
	ret = mdnie_lite_write_set(dsim, seq, num);

	mutex_unlock(&panel->lock);

	return ret;
}

int mdnie_lite_read(struct dsim_device *dsim, u8 addr, u8 *buf, u32 size)
{
	int ret = 0;
	struct panel_private *panel = &dsim->priv;

	if (panel->lcdConnected == PANEL_DISCONNEDTED) {
		dsim_err("%s : %d : panel was not connected\n", __func__, dsim->id);
		return -EIO;
	}

	if (panel->state != PANEL_STATE_RESUMED) {
		dsim_info("%s : panel is not active\n", __func__);
		return -EIO;
	}
	mutex_lock(&panel->lock);
	ret = dsim_read_hl_data(dsim, addr, size, buf);

	mutex_unlock(&panel->lock);

	return ret;
}
#endif

static int dsim_panel_early_probe(struct dsim_device *dsim)
{
	int ret = 0;
	struct panel_private *panel = &dsim->priv;

	panel->ops = dsim_panel_get_priv_ops(dsim);

	if (panel->ops->early_probe) {
		ret = panel->ops->early_probe(dsim);
	}
	return ret;
}

static int dsim_panel_probe(struct dsim_device *dsim)
{
	int ret = 0;
	struct panel_private *panel = &dsim->priv;
#if defined(CONFIG_EXYNOS_DECON_MDNIE_LITE)
	u16 coordinate[2] = {0, };
#endif

	dsim->lcd = lcd_device_register("panel", dsim->dev, &dsim->priv, NULL);
	if (IS_ERR(dsim->lcd)) {
		dsim_err("%s : faield to register lcd device\n", __func__);
		ret = PTR_ERR(dsim->lcd);
		goto probe_err;
	}

	ret = dsim_backlight_probe(dsim);
	if (ret) {
		dsim_err("%s : failed to prbe backlight driver\n", __func__);
		goto probe_err;
	}


	panel->lcdConnected = PANEL_CONNECTED;
	panel->state = PANEL_STATE_RESUMED;
	panel->temperature = NORMAL_TEMPERATURE;
	panel->acl_enable = 0;
	panel->current_acl = 0;
	panel->auto_brightness = 0;
	panel->siop_enable = 0;
	panel->current_hbm = 0;
	panel->current_vint = 0;

	panel->weakness_hbm_comp = 0;
	dsim->glide_display_size = 0;

	mutex_init(&panel->lock);
#ifdef CONFIG_EXYNOS_DECON_LCD_MCD
	panel->mcd_on = 0;
#endif

	if (panel->ops->probe) {
		ret = panel->ops->probe(dsim);
		if (ret) {
			dsim_err("%s : failed to probe panel\n", __func__);
			goto probe_err;
		}
	}
#if defined(CONFIG_EXYNOS_DECON_LCD_SYSFS)
	lcd_init_sysfs(dsim);
#endif

#if defined(CONFIG_EXYNOS_DECON_MDNIE_LITE)
	coordinate[0] = (u16)panel->coordinate[0];
	coordinate[1] = (u16)panel->coordinate[1];
	if (panel->mdnie_support)
		mdnie_register(&dsim->lcd->dev, dsim, (mdnie_w)mdnie_lite_send_seq, (mdnie_r)mdnie_lite_read, coordinate, &tune_info);
#endif

probe_err:
	return ret;
}

static int dsim_panel_displayon(struct dsim_device *dsim)
{
	int ret = 0;
	struct panel_private *panel = &dsim->priv;

	if (panel->lcdConnected == PANEL_DISCONNEDTED) {
		dsim_err("%s : %d : panel was not connected\n", __func__, dsim->id);
		return ret;
	}

#ifdef CONFIG_LCD_ALPM
	mutex_lock(&panel->alpm_lock);
#endif

	if (panel->state == PANEL_STATE_SUSPENED) {
		if (panel->ops->init) {
			ret = panel->ops->init(dsim);
			if (ret) {
				dsim_err("%s : failed to panel init\n", __func__);
				goto displayon_err;
			}
		}
		panel->state = PANEL_STATE_RESUMED;
	}
#ifdef CONFIG_LCD_HMT
	if(panel->hmt_on == HMT_ON)
		hmt_set_mode(dsim, true);
#endif

#ifdef CONFIG_LCD_WEAKNESS_CCB
	if(panel->current_ccb != 0)
		ccb_set_mode(dsim, panel->current_ccb, 0);
#endif

	dsim_panel_set_brightness(dsim, 1);

	if (panel->ops->displayon) {
		ret = panel->ops->displayon(dsim);
		if (ret) {
			dsim_err("%s : failed to panel display on\n", __func__);
			goto displayon_err;
		}
	}

displayon_err:
#ifdef CONFIG_LCD_ALPM
	mutex_unlock(&panel->alpm_lock);
#endif
	return ret;
}

static int dsim_panel_suspend(struct dsim_device *dsim)
{
	int ret = 0;
	struct panel_private *panel = &dsim->priv;

	if (panel->lcdConnected == PANEL_DISCONNEDTED) {
			dsim_err("%s : %d : panel was not connected\n", __func__, dsim->id);
			return ret;
	}

	if (panel->state == PANEL_STATE_SUSPENED)
		goto suspend_err;

	panel->state = PANEL_STATE_SUSPENDING;

	if (panel->ops->exit) {
		ret = panel->ops->exit(dsim);
		if (ret) {
			dsim_err("%s : failed to panel exit \n", __func__);
			goto suspend_err;
		}
	}
	panel->state = PANEL_STATE_SUSPENED;

suspend_err:
	return ret;
}

static int dsim_panel_resume(struct dsim_device *dsim)
{
	int ret = 0;
	struct panel_private *panel = &dsim->priv;

	if (panel->state == PANEL_STATE_SUSPENED) {
		if (panel->ops->init) {
			ret = panel->ops->init(dsim);
			if (ret) {
				dsim_err("%s : failed to panel init\n", __func__);
				//goto displayon_err;
			}
		}
		panel->state = PANEL_STATE_RESUMED;
	}

	return ret;
}


static int dsim_panel_dump(struct dsim_device *dsim)
{
	int ret = 0;
	struct panel_private *panel = &dsim->priv;

	dsim_info("%s was called\n", __func__);

	if (panel->lcdConnected == PANEL_DISCONNEDTED) {
		dsim_err("%s : %d : panel was not connected\n", __func__, dsim->id);
		return ret;
	}

	if (panel->ops->dump)
		ret = panel->ops->dump(dsim);

	return ret;
}
#ifdef CONFIG_LCD_DOZE_MODE
static int dsim_panel_enteralpm(struct dsim_device *dsim)
{
	int ret = 0;
	struct panel_private *panel = &dsim->priv;

	dsim_info("%s was called\n", __func__);

	if (panel->alpm_support == 0) {
		dsim_info("%s:this panel does not support doze mode\n", __func__);
		return 0;
	}

	if (panel->lcdConnected == PANEL_DISCONNEDTED) {
		dsim_err("%s : %d : panel was not connected\n", __func__, dsim->id);
		return ret;
	}

	if (panel->state == PANEL_STATE_SUSPENED) {
		if (panel->ops->init) {
			ret = panel->ops->init(dsim);
			if (ret) {
				dsim_err("%s : failed to panel init\n", __func__);
				return ret;
			}
		}
		panel->state = PANEL_STATE_RESUMED;
	}

	if (panel->ops->enteralpm) {
		ret = panel->ops->enteralpm(dsim);
		if (ret) {
			dsim_err("ERR:%s:failed to enter alpm \n", __func__);
			return ret;
		}
		panel->curr_alpm_mode = panel->alpm_mode;
	}
	return ret;
}

static int dsim_panel_exitalpm(struct dsim_device *dsim)
{
	int ret = 0;
	struct panel_private *panel = &dsim->priv;

	dsim_info("%s was called\n", __func__);

	if (panel->alpm_support == 0) {
		dsim_info("%s:this panel does not support doze mode\n", __func__);
		return 0;
	}

	if (panel->lcdConnected == PANEL_DISCONNEDTED) {
		dsim_err("%s : %d : panel was not connected\n", __func__, dsim->id);
		return ret;
	}

	if (panel->state == PANEL_STATE_SUSPENED) {
		if (panel->ops->init) {
			ret = panel->ops->init(dsim);
			if (ret) {
				dsim_err("%s : failed to panel init\n", __func__);
				return ret;
			}
		}
		panel->state = PANEL_STATE_RESUMED;
	}

	if (panel->ops->enteralpm) {
		ret = panel->ops->exitalpm(dsim);
		if (ret) {
			dsim_err("ERR:%s:failed to exit alpm \n", __func__);
			return ret;
		}
	}
	return ret;
}
#endif
static struct mipi_dsim_lcd_driver mipi_lcd_driver = {
	.early_probe = dsim_panel_early_probe,
	.probe		= dsim_panel_probe,
	.displayon	= dsim_panel_displayon,
	.suspend	= dsim_panel_suspend,
	.resume		= dsim_panel_resume,
	.dump		= dsim_panel_dump,
#ifdef CONFIG_LCD_DOZE_MODE
	.enteralpm = dsim_panel_enteralpm,
	.exitalpm = dsim_panel_exitalpm,
#endif
};


int dsim_panel_ops_init(struct dsim_device *dsim)
{
	int ret = 0;

	if (dsim)
		dsim->panel_ops = &mipi_lcd_driver;

	return ret;
}

unsigned int lcdtype = 0;
EXPORT_SYMBOL(lcdtype);

static int __init get_lcd_type(char *arg)
{
	get_option(&arg, &lcdtype);

	dsim_info("--- Parse LCD TYPE ---\n");
	dsim_info("LCDTYPE : %x\n", lcdtype);

	return 0;
}
early_param("lcdtype", get_lcd_type);

