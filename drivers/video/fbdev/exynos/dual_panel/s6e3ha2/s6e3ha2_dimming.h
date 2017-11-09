/*
 * linux/drivers/video/fbdev/exynos/panel/s6e3ha2/s6e3ha2_dimming.h
 *
 * Header file for S6E3HA2 Dimming Driver
 *
 * Copyright (c) 2016 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __S6E3HA2_DIMMING_H__
#define __S6E3HA2_DIMMING_H__
#include <linux/types.h>
#include <linux/kernel.h>
#include "../dimming.h"

#define S6E3HA2_NR_TP (10)

#define S6E3HA2_NR_LUMINANCE (65)
#define S6E3HA2_TARGET_LUMINANCE (360)

#define S6E3HA2_NR_HBM_LUMINANCE (8)
#define S6E3HA2_TARGET_HBM_LUMINANCE (600)

#define S6E3HA2_HMD_NR_LUMINANCE (37)
#define S6E3HA2_HMD_TARGET_LUMINANCE (360)

#ifdef CONFIG_LCD_EXTEND_HBM
#define S6E3HA2_NR_EXTEND_HBM_LUMINANCE	(1)
#define S6E3HA2_TARGET_EXTEND_HBM_LUMINANCE	(720)
#define S6E3HA2_TOTAL_NR_LUMINANCE (S6E3HA2_NR_LUMINANCE + S6E3HA2_NR_HBM_LUMINANCE + S6E3HA2_NR_EXTEND_HBM_LUMINANCE)
#define S6E3HA2_HMD_TOTAL_NR_LUMINANCE (S6E3HA2_HMD_NR_LUMINANCE)
#else
#define S6E3HA2_TOTAL_NR_LUMINANCE (S6E3HA2_NR_LUMINANCE + S6E3HA2_NR_HBM_LUMINANCE)
#define S6E3HA2_HMD_TOTAL_NR_LUMINANCE (S6E3HA2_HMD_NR_LUMINANCE)
#endif

#define S6E3HA2_TOTAL_PAC_STEPS		(PANEL_BACKLIGHT_PAC_STEPS + S6E3HA2_NR_HBM_LUMINANCE + S6E3HA2_NR_EXTEND_HBM_LUMINANCE)

static struct tp s6e3ha2_tp[S6E3HA2_NR_TP] = {
	{ .level = 0, .volt_src = VREG_OUT, .name = "VT", .center = { 0x0, 0x0, 0x0 }, .numerator = 0, .denominator = 860 },
	{ .level = 3, .volt_src = V0_OUT, .name = "V3", .center = { 0x80, 0x80, 0x80 }, .numerator = 64, .denominator = 320 },
	{ .level = 11, .volt_src = VT_OUT, .name = "V11", .center = { 0x80, 0x80, 0x80 }, .numerator = 64, .denominator = 320 },
	{ .level = 23, .volt_src = VT_OUT, .name = "V23", .center = { 0x80, 0x80, 0x80 }, .numerator = 64, .denominator = 320 },
	{ .level = 35, .volt_src = VT_OUT, .name = "V35", .center = { 0x80, 0x80, 0x80 }, .numerator = 64, .denominator = 320 },
	{ .level = 51, .volt_src = VT_OUT, .name = "V51", .center = { 0x80, 0x80, 0x80 }, .numerator = 64, .denominator = 320 },
	{ .level = 87, .volt_src = VT_OUT, .name = "V87", .center = { 0x80, 0x80, 0x80 }, .numerator = 64, .denominator = 320 },
	{ .level = 151, .volt_src = VT_OUT, .name = "V151", .center = { 0x80, 0x80, 0x80 }, .numerator = 64, .denominator = 320 },
	{ .level = 203, .volt_src = VT_OUT, .name = "V203", .center = { 0x80, 0x80, 0x80 }, .numerator = 64, .denominator = 320 },
	{ .level = 255, .volt_src = VREG_OUT, .name = "V255", .center = { 0x100, 0x100, 0x100 }, .numerator = 72, .denominator = 860 },
};
#endif /* __S6E3HA2_DIMMING_H__ */
