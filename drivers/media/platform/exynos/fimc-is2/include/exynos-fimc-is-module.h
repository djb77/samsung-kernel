/*
 * /include/media/exynos_fimc_is_sensor.h
 *
 * Copyright (C) 2012 Samsung Electronics, Co. Ltd
 *
 * Exynos series exynos_fimc_is_sensor device support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef MEDIA_EXYNOS_MODULE_H
#define MEDIA_EXYNOS_MODULE_H

#include <linux/platform_device.h>
#include <dt-bindings/camera/fimc_is.h>

#include "fimc-is-device-sensor.h"

#define MAX_SENSOR_SHARED_RSC	10

enum shared_rsc_type_t {
	SRT_ACQUIRE = 1,
	SRT_RELEASE,
};

struct exynos_sensor_pin {
	ulong pin;
	u32 delay;
	u32 value;
	char *name;
	u32 act;
	u32 voltage;

	enum shared_rsc_type_t shared_rsc_type;
	spinlock_t *shared_rsc_slock;
	atomic_t *shared_rsc_count;
	int shared_rsc_active;
};

#define SET_PIN_INIT(d, s, c) d->pinctrl_index[s][c] = 0;

#define SET_PIN(d, s, c, p, n, a, v, t)							\
	do {										\
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c]].pin	= p;		\
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c]].name	= n;		\
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c]].act	= a;		\
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c]].value	= v;		\
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c]].delay	= t;		\
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c]].voltage	= 0;		\
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c]].shared_rsc_type  = 0;	\
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c]].shared_rsc_slock = NULL;	\
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c]].shared_rsc_count = NULL;	\
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c]].shared_rsc_active = 0;	\
		(d)->pinctrl_index[s][c]++;						\
	} while (0)

#define SET_PIN_VOLTAGE(d, s, c, p, n, a, v, t, e)					\
	do {										\
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c]].pin	= p;		\
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c]].name	= n;		\
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c]].act	= a;		\
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c]].value	= v;		\
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c]].delay	= t;		\
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c]].voltage	= e;		\
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c]].shared_rsc_type  = 0;	\
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c]].shared_rsc_slock = NULL;	\
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c]].shared_rsc_count = NULL;	\
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c]].shared_rsc_active = 0;	\
		(d)->pinctrl_index[s][c]++;						\
	} while (0)

#define SET_PIN_SHARED(d, s, c, type, slock, count, active)					\
	do {											\
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c] - 1].shared_rsc_type  = type;	\
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c] - 1].shared_rsc_slock = slock;	\
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c] - 1].shared_rsc_count = count;	\
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c] - 1].shared_rsc_active = active;	\
	} while (0)

struct exynos_platform_fimc_is_module {
	int (*gpio_cfg)(struct fimc_is_module_enum *module, u32 scenario, u32 gpio_scenario);
	int (*gpio_dbg)(struct fimc_is_module_enum *module, u32 scenario, u32 gpio_scenario);
	struct exynos_sensor_pin pin_ctrls[SENSOR_SCENARIO_MAX][GPIO_SCENARIO_MAX][GPIO_CTRL_MAX];
	u32 pinctrl_index[SENSOR_SCENARIO_MAX][GPIO_SCENARIO_MAX];
	struct pinctrl *pinctrl;
	u32 position;
	u32 mclk_ch;
	u32 id;
	u32 sensor_i2c_ch;
	u32 sensor_i2c_addr;
	u32 is_bns;
	u32 flash_product_name;
	u32 flash_first_gpio;
	u32 flash_second_gpio;
	u32 af_product_name;
	u32 af_i2c_addr;
	u32 af_i2c_ch;
	u32 ois_product_name;
	u32 ois_i2c_addr;
	u32 ois_i2c_ch;
	u32 preprocessor_product_name;
	u32 preprocessor_spi_channel;
	u32 preprocessor_i2c_addr;
	u32 preprocessor_i2c_ch;
	u32 preprocessor_dma_channel;
	bool power_seq_dt;
	u32 internal_vc[CSI_VIRTUAL_CH_MAX];
	u32 vc_buffer_offset[CSI_VIRTUAL_CH_MAX];
};

extern int exynos_fimc_is_module_pins_cfg(struct fimc_is_module_enum *module,
	u32 snenario,
	u32 gpio_scenario);
extern int exynos_fimc_is_module_pins_dbg(struct fimc_is_module_enum *module,
	u32 scenario,
	u32 gpio_scenario);


/*
< Table of HS_SETTLE_VALUE >
- reference data : High-Speed Operation Timing v1.2 (14nm 2.1Gbps MIPI)

struct hs_operate_time_cfg {
	u32 serial_clock_min;	//MHz : target freq. of bit clock (80 Mhz ~ 2100 MHz)
	u32 serial_clock_max;	//
	u32 hs_settle;		//hex
};

static struct hs_operate_time_cfg hs_timing_v1p2[] = {
	{2071,	2100,	0x2E},
	{2031,	2070,	0x2D},
	{1981,	2030,	0x2C},
	{1941,	1980,	0x2B},
	{1891,	1940,	0x2A},
	{1851,	1890,	0x29},
	{1801,	1850,	0x28},
	{1761,	1800,	0x27},
	{1711,	1760,	0x26},
	{1671,	1710,	0x25},
	{1621,	1670,	0x24},
	{1581,	1620,	0x23},
	{1531,	1580,	0x22},
	{1491,	1530,	0x21},
	{1441,	1490,	0x20},
	{1401,	1440,	0x1F},
	{1351,	1400,	0x1E},
	{1311,	1350,	0x1D},
	{1261,	1310,	0x1C},
	{1221,	1260,	0x1B},
	{1171,	1220,	0x1A},
	{1121,	1170,	0x19},
	{1081,	1120,	0x18},
	{1031,	1080,	0x17},
	{991,	1030,	0x16},
	{941,	990,	0x15},
	{901,	940,	0x14},
	{851,	900,	0x13},
	{811,	850,	0x12},
	{761,	810,	0x11},
	{721,	760,	0x10},
	{671,	720,	0xF},
	{631,	670,	0xE},
	{581,	630,	0xD},
	{541,	580,	0xC},
	{491,	540,	0xB},
	{451,	490,	0xA},
	{401,	450,	0x9},
	{361,	400,	0x8},
	{311,	360,	0x7},
	{271,	310,	0x6},
	{221,	270,	0x5},
	{181,	220,	0x4},
	{131,	180,	0x3},
	{91,	130,	0x2},
	{80,	90,	0x1},
};
*/

#endif /* MEDIA_EXYNOS_MODULE_H */
