/* drivers/video/exynos/panels/s6e3fa0_mipi_lcd.c
 *
 * Samsung SoC MIPI LCD driver.
 *
 * Copyright (c) 2014 Samsung Electronics
 *
 * Haowei Li, <haowei.li@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/kernel.h>
#include <linux/backlight.h>
#include <video/mipi_display.h>
#include <linux/platform_device.h>

#include "s6e3fa0_param.h"
#include "lcd_ctrl.h"
#include "decon_lcd.h"
#include "../dsim.h"

#define MAX_VOLTAGE_PULSE	0x04
#define MAX_BRIGHTNESS		255
/* set the minimum brightness value to see the screen */
#define MIN_BRIGHTNESS		0
#define DEFAULT_BRIGHTNESS	98

static int s6e3fa0_get_brightness(struct backlight_device *bd)
{
	return bd->props.brightness;
}

static int update_brightness(struct dsim_device *dsim, int brightness)
{
	int vol_pulse;
	int id = dsim->id;
	unsigned char gamma_update[3];
	unsigned char aor_control[12];

	if (dsim->state != DSIM_STATE_HSCLKEN) {
		return 0;
	}

	memcpy(gamma_update, SEQ_GAMMA_UPDATE, 3);
	memcpy(aor_control, SEQ_AOR_CONTROL, 12);

	/*
	 * In order to change brightness to be set to one of values in the
	 * gamma_control parameter. Brightness value(real_br) from 0 to 255.
	 * This value is controlled by the control bar.
	 */
	if (brightness > 100)
		vol_pulse = MAX_VOLTAGE_PULSE + ((255 - brightness) * 10);
	else
		vol_pulse = MAX_VOLTAGE_PULSE + 0x604 + ((101 - brightness) * 4);

	aor_control[2] = aor_control[4] = 0xFF & vol_pulse;
	aor_control[1] = aor_control[3] = ((0xF00 & vol_pulse) >> 8);

	/* It updates the changed brightness value to ddi */
	gamma_update[1] = 0x00;

	if (dsim_wr_data(id, MIPI_DSI_DCS_LONG_WRITE, (unsigned long)aor_control,
				ARRAY_SIZE(aor_control)) < 0)
		dsim_err("fail to send aor_control command.\n");

	if (dsim_wr_data(id, MIPI_DSI_DCS_LONG_WRITE, (unsigned long)SEQ_GAMMA_UPDATE,
				ARRAY_SIZE(SEQ_GAMMA_UPDATE)) < 0)
		dsim_err("fail to send SEQ_GAMMA_UPDATE command.\n");

	if (dsim_wr_data(id, MIPI_DSI_DCS_LONG_WRITE, (unsigned long)gamma_update,
				ARRAY_SIZE(gamma_update)) < 0)
		dsim_err("fail to send gamma_update command.\n");

	return 0;
}

static int s6e3fa0_set_brightness(struct backlight_device *bd)
{
	struct dsim_device *dsim;
	struct v4l2_subdev *sd;
	int brightness = bd->props.brightness;

	sd = dev_get_drvdata(bd->dev.parent);
	dsim = container_of(sd, struct dsim_device, sd);

	if (brightness == 146 || brightness == 226)
		return 1;
	if (brightness < MIN_BRIGHTNESS || brightness > MAX_BRIGHTNESS) {
		printk(KERN_ALERT "Brightness should be in the range of 0 ~ 255\n");
		return -EINVAL;
	}

	update_brightness(dsim, brightness);

	return 1;
}

static const struct backlight_ops s6e3fa0_backlight_ops = {
	.get_brightness = s6e3fa0_get_brightness,
	.update_status = s6e3fa0_set_brightness,
};

static int s6e3fa0_probe(struct dsim_device *dsim)
{
	dsim->bd = backlight_device_register("panel", dsim->dev,
		NULL, &s6e3fa0_backlight_ops, NULL);
	if (IS_ERR(dsim->bd))
		printk(KERN_ALERT "failed to register backlight device!\n");

	dsim->bd->props.max_brightness = MAX_BRIGHTNESS;
	dsim->bd->props.brightness = DEFAULT_BRIGHTNESS;

	return 1;
}

static int s6e3fa0_displayon(struct dsim_device *dsim)
{
	lcd_init(dsim->id, &dsim->lcd_info);
	lcd_enable(dsim->id);
	return 1;
}

static int s6e3fa0_suspend(struct dsim_device *dsim)
{
	return 1;
}

static int s6e3fa0_resume(struct dsim_device *dsim)
{
	return 1;
}

struct mipi_dsim_lcd_driver s6e3fa0_mipi_lcd_driver = {
	.probe		= s6e3fa0_probe,
	.displayon	= s6e3fa0_displayon,
	.suspend	= s6e3fa0_suspend,
	.resume		= s6e3fa0_resume,
};
