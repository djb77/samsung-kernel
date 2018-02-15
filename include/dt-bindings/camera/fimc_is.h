/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _DT_BINDINGS_CAMERA_FIMC_IS_H
#define _DT_BINDINGS_CAMERA_FIMC_IS_H

#define F1_0	0
#define F1_4	1
#define F1_9	2
#define F2_0	3
#define F2_2	4
#define F2_4	5
#define F2_45	6
#define F2_6	7
#define F2_7	8	/* Added for 6A3 */
#define F2_8	9
#define F4_0	10
#define F5_6	11
#define F8_0	12
#define F11_0	13
#define F16_0	14
#define F22_0	15
#define F32_0	16

#define FLITE_ID_NOTHING	100

#define GPIO_SCENARIO_ON			0
#define GPIO_SCENARIO_OFF			1
#define GPIO_SCENARIO_STANDBY_ON		2
#define GPIO_SCENARIO_STANDBY_OFF		3
#define GPIO_SCENARIO_STANDBY_OFF_SENSOR	4
#define GPIO_SCENARIO_STANDBY_OFF_PREPROCESSOR	5
#define GPIO_SCENARIO_SENSOR_RETENTION_ON	6
#define GPIO_SCENARIO_MAX			7
#define GPIO_CTRL_MAX				32

#define SENSOR_SCENARIO_NORMAL		0
#define SENSOR_SCENARIO_VISION		1
#define SENSOR_SCENARIO_EXTERNAL	2
#define SENSOR_SCENARIO_OIS_FACTORY	3
#define SENSOR_SCENARIO_READ_ROM	4
#define SENSOR_SCENARIO_STANDBY		5
#define SENSOR_SCENARIO_SECURE		6
#define SENSOR_SCENARIO_VIRTUAL		9
#define SENSOR_SCENARIO_MAX		10

#define PIN_NONE	0
#define PIN_OUTPUT	1
#define PIN_INPUT	2
#define PIN_RESET	3
#define PIN_FUNCTION	4
#define PIN_REGULATOR	5

#define DT_SET_PIN(p, n, a, v, t) \
			seq@__LINE__ { \
				pin = #p; \
				pname = #n; \
				act = <a>; \
				value = <v>; \
				delay = <t>; \
				voltage = <0>; \
			}

#define DT_SET_PIN_VOLTAGE(p, n, a, v, t, e) \
			seq@__LINE__ { \
				pin = #p; \
				pname = #n; \
				act = <a>; \
				value = <v>; \
				delay = <t>; \
				voltage = <e>; \
			}


#define VC_NOTHING		0
#define VC_TAIL_MODE_PDAF	1
#define VC_MIPI_STAT	2

#endif
